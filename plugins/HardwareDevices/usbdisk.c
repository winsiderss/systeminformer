/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "devices.h"

//
// Drives behind USB bridges as a hardware sensor source, read through the driver every UB_INTERVAL_MS while it is
// connected: an ATA drive's SMART temperature (attribute 194, else 190), an NVMe drive's composite temperature, the
// percentage of its rated life used and its available spare. A drive in standby keeps its rows without values; the
// script does not wake it. Each disk keeps its component id while its disk unit stays.
//

#define UB_INTERVAL_MS 60000
#define UB_SENSORS 3 // temperature, life used, available spare

typedef struct _UB_DISK
{
    WCHAR InstanceId[KPH_HW_USB_DISK_ID];
    PPH_STRING Name;
    USHORT VendorId;
    USHORT ProductId;
    UCHAR Kind;
    BOOLEAN Present;
    NTSTATUS LastStatus;
    BOOLEAN Valid[UB_SENSORS];
    FLOAT Values[UB_SENSORS];
} UB_DISK, *PUB_DISK;

typedef struct _UB_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    ULONG64 LastPollTime;
    NTSTATUS LastStatus;
    ULONG Count;
    UB_DISK Disks[KPH_HW_USB_DISKS];
} UB_CONTEXT, *PUB_CONTEXT;

static PUB_CONTEXT UsbDiskContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION UsbDiskRegistration = NULL;
// GUID_DEVCLASS_DISKDRIVE
static CONST GUID UsbDiskClass = { 0x4d36e967, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };

static CONST PCWSTR UsbDiskSensorNames[UB_SENSORS] = { L"Temperature", L"Life used", L"Available spare" };
static CONST PH_HW_SENSOR_TYPE UsbDiskSensorTypes[UB_SENSORS] =
{
    PhHwSensorTypeTemperature, PhHwSensorTypeControl, PhHwSensorTypeControl
};

/**
 * Decodes a record's SMART data: an ATA drive's temperature from attribute 194, else 190 (the current degrees in
 * the low raw byte); an NVMe drive's composite temperature (kelvin), life used and available spare (percent).
 */
static VOID UsbDiskDecode(
    _In_ PKPH_HW_USB_DISK Record,
    _Out_writes_(UB_SENSORS) PBOOLEAN Valid,
    _Out_writes_(UB_SENSORS) PFLOAT Values
    )
{
    memset(Valid, 0, UB_SENSORS * sizeof(BOOLEAN));
    memset(Values, 0, UB_SENSORS * sizeof(FLOAT));

    if (!NT_SUCCESS(Record->Status))
        return;

    if (Record->Kind == KPH_HW_USB_DISK_ATA)
    {
        ULONG found = 0;

        // 30 attributes of 12 bytes from offset 2: id, flags (2), current, worst, raw (6), reserved.
        for (ULONG i = 0; i < 30; i++)
        {
            PUCHAR attribute = &Record->Data[2 + i * 12];
            ULONG celsius = attribute[5];

            if ((attribute[0] == 194 || (attribute[0] == 190 && found != 194)) && celsius > 0 && celsius < 128)
            {
                found = attribute[0];
                Values[0] = (FLOAT)celsius;
            }
        }

        Valid[0] = !!found;
    }
    else if (Record->Kind == KPH_HW_USB_DISK_NVME)
    {
        USHORT kelvin = (USHORT)(Record->Data[1] | (Record->Data[2] << 8));

        Valid[0] = kelvin != 0;
        Values[0] = (FLOAT)kelvin - 273.15f;
        Valid[1] = TRUE;
        Values[1] = (FLOAT)Record->Data[5];
        Valid[2] = Record->Data[3] <= 100;
        Values[2] = (FLOAT)Record->Data[3];
    }
}

/**
 * Names a disk by its disk unit's friendly name, else by its bridge.
 */
static PPH_STRING UsbDiskName(
    _In_ PKPH_HW_USB_DISK Record
    )
{
    DEVINST unit;
    WCHAR friendlyName[128];
    DEVPROPTYPE type;
    ULONG size = sizeof(friendlyName);

    if (CM_Locate_DevNodeW(&unit, Record->InstanceId, CM_LOCATE_DEVNODE_NORMAL) == CR_SUCCESS &&
        CM_Get_DevNode_PropertyW(unit, &DEVPKEY_Device_FriendlyName, &type, (PBYTE)friendlyName, &size, 0) == CR_SUCCESS &&
        type == DEVPROP_TYPE_STRING && size >= sizeof(WCHAR))
    {
        friendlyName[RTL_NUMBER_OF(friendlyName) - 1] = UNICODE_NULL;
        return PhFormatString(L"%s (USB %04x:%04x)", friendlyName, Record->VendorId, Record->ProductId);
    }

    return PhFormatString(L"USB disk %04x:%04x", Record->VendorId, Record->ProductId);
}

/**
 * Reads every bridge disk and merges the records into the disk slots by instance ID. Called on the provider thread.
 */
static VOID UsbDiskPoll(
    _Inout_ PUB_CONTEXT Context
    )
{
    PKPH_HW_USB_DISK records;
    ULONG count = 0;
    NTSTATUS status;

    records = PhAllocate(KPH_HW_USB_DISKS * sizeof(KPH_HW_USB_DISK));
    status = KphHwUsbDiskRead(records, &count);
    Context->LastStatus = status;

    // Another tool holds the bridges: the next callback tries again.
    if (status == STATUS_RETRY)
    {
        PhFree(records);
        return;
    }

    Context->LastPollTime = NtGetTickCount64();

    for (ULONG i = 0; i < Context->Count; i++)
        Context->Disks[i].Present = FALSE;

    for (ULONG r = 0; NT_SUCCESS(status) && r < count; r++)
    {
        PKPH_HW_USB_DISK record = &records[r];
        PUB_DISK disk = NULL;

        // A bridge the script does not know is not listed.
        if (record->Status == STATUS_NOT_SUPPORTED || !record->InstanceId[0])
            continue;

        record->InstanceId[KPH_HW_USB_DISK_ID - 1] = UNICODE_NULL;

        for (ULONG i = 0; i < Context->Count; i++)
        {
            if (PhEqualStringZ(Context->Disks[i].InstanceId, record->InstanceId, TRUE))
            {
                disk = &Context->Disks[i];
                break;
            }
        }

        if (!disk)
        {
            if (Context->Count == RTL_NUMBER_OF(Context->Disks))
                continue;

            disk = &Context->Disks[Context->Count++];
            memcpy(disk->InstanceId, record->InstanceId, sizeof(disk->InstanceId));
            disk->Name = UsbDiskName(record);
        }

        disk->VendorId = record->VendorId;
        disk->ProductId = record->ProductId;
        disk->Kind = record->Kind;
        disk->Present = TRUE;
        disk->LastStatus = record->Status;
        UsbDiskDecode(record, disk->Valid, disk->Values);
    }

    PhFree(records);
}

_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI UsbDiskSourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PUB_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;

    if (!context || KsiLevel() != KphLevelMax)
        return;

    if (!context->LastPollTime || NtGetTickCount64() - context->LastPollTime >= UB_INTERVAL_MS)
    {
        PhAcquireQueuedLockExclusive(&context->Lock);
        UsbDiskPoll(context);
        PhReleaseQueuedLockExclusive(&context->Lock);
    }

    PhAcquireQueuedLockShared(&context->Lock);

    for (ULONG i = 0; i < context->Count; i++)
    {
        PUB_DISK disk = &context->Disks[i];

        if (!disk->Present)
            continue;

        for (ULONG s = 0; s < UB_SENSORS; s++)
        {
            if (!disk->Valid[s])
                continue;

            memset(&report, 0, sizeof(PH_HW_SENSOR_REPORT));
            report.Size = sizeof(PH_HW_SENSOR_REPORT);
            report.Flags = PH_HW_SENSOR_REPORT_HISTORY;
            report.ComponentId = i;
            report.Type = UsbDiskSensorTypes[s];
            report.ComponentName = disk->Name->Buffer;
            report.SensorName = UsbDiskSensorNames[s];
            report.SensorId = s;
            report.Value = disk->Values[s];
            report.ComponentClass = &UsbDiskClass;

            Report(ReportContext, &report);
        }
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_DIAGNOSTICS_CALLBACK)
static VOID NTAPI UsbDiskSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_opt_ PVOID Context
    )
{
    PUB_CONTEXT context = Context;

    if (!context)
        return;

    PhAcquireQueuedLockShared(&context->Lock);

    PhAppendFormatStringBuilder(Builder, L"Last read: status 0x%08lx, disks %lu\n", context->LastStatus, context->Count);

    for (ULONG i = 0; i < context->Count; i++)
    {
        PUB_DISK disk = &context->Disks[i];
        PCWSTR kind = L"ATA";
        PCWSTR presence = L"gone";

        if (disk->Kind == KPH_HW_USB_DISK_NVME)
            kind = L"NVMe";
        if (disk->Present)
            presence = L"present";

        PhAppendFormatStringBuilder(
            Builder,
            L"  %s: %s, %s, status 0x%08lx",
            disk->Name->Buffer,
            kind,
            presence,
            disk->LastStatus
            );

        for (ULONG s = 0; s < UB_SENSORS; s++)
        {
            if (disk->Valid[s])
                PhAppendFormatStringBuilder(Builder, L", %s %.1f", UsbDiskSensorNames[s], disk->Values[s]);
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

/**
 * Registers the USB bridge disk source. Called on plugin load.
 */
VOID UsbDiskInitialize(
    VOID
    )
{
    PH_HW_SENSOR_SOURCE source;

    if (UsbDiskContext)
        return;

    UsbDiskContext = PhAllocateZero(sizeof(UB_CONTEXT));
    PhInitializeQueuedLock(&UsbDiskContext->Lock);

    memset(&source, 0, sizeof(PH_HW_SENSOR_SOURCE));
    source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    source.Callback = UsbDiskSourceCallback;
    source.Context = UsbDiskContext;
    source.Diagnostics = UsbDiskSourceDiagnostics;

    if (!NT_SUCCESS(PhRegisterHardwareSensorSource(PluginInstance, &source, &UsbDiskRegistration)))
    {
        UsbDiskRegistration = NULL;
        UsbDiskUninitialize();
    }
}

/**
 * Unregisters the source. Called on plugin unload.
 */
VOID UsbDiskUninitialize(
    VOID
    )
{
    PUB_CONTEXT context = UsbDiskContext;

    // Unregistering waits for a callback in progress.
    if (UsbDiskRegistration)
    {
        PhUnregisterHardwareSensorSource(UsbDiskRegistration);
        UsbDiskRegistration = NULL;
    }

    if (!context)
        return;

    for (ULONG i = 0; i < context->Count; i++)
        PhClearReference(&context->Disks[i].Name);

    PhFree(context);
    UsbDiskContext = NULL;
}
