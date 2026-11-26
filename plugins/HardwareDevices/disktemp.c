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
// Drive temperatures as a hardware sensor source: each NVMe disk's composite temperature and its temperature sensors 1
// to 8 from the SMART / health information log page, every DT_INTERVAL_MS; each ATA disk's SMART temperature
// (attribute 194, else 190), every DT_ATA_INTERVAL_MS. A SMART command wakes a sleeping hard disk, so a rotational
// disk is read only while Windows has it in D0.
//

#define DT_INTERVAL_MS 5000
#define DT_ATA_INTERVAL_MS 15000
#define DT_MAX_DISKS 32
#define DT_SENSORS 9 // the composite temperature, then sensors 1 to 8
#define DT_ENUMERATE_POLLS 12 // the disks are listed again every this many polls: new ones added, gone ones dropped

typedef struct _DT_DISK
{
    PPH_STRING Path; // the disk interface, opened for each read and closed after
    BOOLEAN Present; // listed by the last enumeration
    ULONG Index;
    PPH_STRING Name;
    BOOLEAN Ata; // SMART attributes, not the NVMe health log
    BOOLEAN Rotational; // incurs a seek penalty: read only in D0
    DEVINST DevInst; // 0 when not found
    ULONG64 LastPollTime;
    NTSTATUS LastStatus;
    BOOLEAN Valid[DT_SENSORS];
    FLOAT Celsius[DT_SENSORS];
} DT_DISK, *PDT_DISK;

typedef struct _DT_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    BOOLEAN Enumerated;
    ULONG Polls; // since the last enumeration
    PPH_LIST Probed; // the interface paths tried, a disk or not, while they stay listed
    ULONG64 LastPollTime;
    ULONG Count;
    DT_DISK Disks[DT_MAX_DISKS];
} DT_CONTEXT, *PDT_CONTEXT;

static PDT_CONTEXT DiskTemperatureContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION DiskTemperatureRegistration = NULL;
// GUID_DEVCLASS_DISKDRIVE
static CONST GUID DiskTemperatureClass = { 0x4d36e967, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };

static CONST PCWSTR DiskTemperatureSensorNames[DT_SENSORS] =
{
    L"Composite", L"Sensor 1", L"Sensor 2", L"Sensor 3", L"Sensor 4", L"Sensor 5", L"Sensor 6", L"Sensor 7", L"Sensor 8"
};

static ULONG64 DiskTemperatureQueryTime(
    VOID
    )
{
    return NtGetTickCount64();
}

/**
 * Reads an ATA disk's SMART temperature: attribute 194, else 190, the current degrees in the low raw byte.
 */
static NTSTATUS DiskTemperatureQuerySmart(
    _In_ HANDLE Handle,
    _Out_ PFLOAT Celsius
    )
{
    NTSTATUS status;
    PPH_LIST attributes;
    ULONG found = 0;
    ULONG value = 0;

    if (!NT_SUCCESS(status = DiskDriveQueryImminentFailure(Handle, &attributes)))
        return status;

    for (ULONG i = 0; i < attributes->Count; i++)
    {
        PSMART_ATTRIBUTES attribute = attributes->Items[i];
        ULONG celsius = attribute->RawValue & 0xFF;

        if ((attribute->AttributeId == 194 || (attribute->AttributeId == 190 && found != 194)) && celsius > 0 && celsius < 128)
        {
            found = attribute->AttributeId;
            value = celsius;
        }

        PhFree(attribute);
    }

    PhDereferenceObject(attributes);

    if (!found)
        return STATUS_NOT_FOUND;

    *Celsius = (FLOAT)value;
    return STATUS_SUCCESS;
}

/**
 * Determines whether a disk incurs a seek penalty (spins).
 */
static BOOLEAN DiskTemperatureIsRotational(
    _In_ HANDLE Handle
    )
{
    STORAGE_PROPERTY_QUERY query;
    DEVICE_SEEK_PENALTY_DESCRIPTOR penalty;

    memset(&query, 0, sizeof(STORAGE_PROPERTY_QUERY));
    query.PropertyId = StorageDeviceSeekPenaltyProperty;
    query.QueryType = PropertyStandardQuery;
    memset(&penalty, 0, sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR));

    // Unknown counts as rotational: never risk waking a disk.
    if (!NT_SUCCESS(PhDeviceIoControlFile(Handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &penalty, sizeof(penalty), NULL)))
        return TRUE;

    return penalty.IncursSeekPenalty;
}

/**
 * Determines whether Windows has a disk in D0 (spinning), from its PnP power data.
 */
static BOOLEAN DiskTemperatureIsAwake(
    _In_ DEVINST DevInst
    )
{
    CM_POWER_DATA power;
    DEVPROPTYPE type;
    ULONG size = sizeof(CM_POWER_DATA);

    memset(&power, 0, sizeof(CM_POWER_DATA));

    if (!DevInst || CM_Get_DevNode_PropertyW(DevInst, &DEVPKEY_Device_PowerData, &type, (PBYTE)&power, &size, 0) != CR_SUCCESS)
        return FALSE;

    return power.PD_MostRecentPowerState == PowerDeviceD0;
}

/**
 * The position of a path in a list of paths, or ULONG_MAX.
 */
static ULONG DiskTemperatureFindPath(
    _In_ PPH_LIST Paths,
    _In_ PPH_STRING Path
    )
{
    for (ULONG i = 0; i < Paths->Count; i++)
    {
        if (PhEqualString(Paths->Items[i], Path, TRUE))
            return i;
    }

    return ULONG_MAX;
}

/**
 * Lists the disks: NVMe ones whose health log answers, ATA ones with a SMART temperature. A path already tried is not
 * tried again while it stays listed; a disk no longer listed keeps its slot without values. Called on the provider
 * thread.
 */
static VOID DiskTemperatureEnumerate(
    _Inout_ PDT_CONTEXT Context
    )
{
    ULONG deviceCount = 0;
    const DEV_OBJECT* deviceObjects = NULL;
    const DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
    };
    const DEVPROP_FILTER_EXPRESSION deviceFilter[] =
    {
        { DEVPROP_OPERATOR_EQUALS, {{ DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL }, DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&GUID_DEVINTERFACE_DISK } }
    };
    PPH_LIST listed;

    if (HR_FAILED(PhDevGetObjects(
        DevObjectTypeDeviceInterface,
        DevQueryFlagNone,
        RTL_NUMBER_OF(requestedProperties),
        requestedProperties,
        RTL_NUMBER_OF(deviceFilter),
        deviceFilter,
        &deviceCount,
        &deviceObjects
        )))
    {
        return;
    }

    if (!Context->Probed)
        Context->Probed = PhCreateList(DT_MAX_DISKS);

    listed = PhCreateList(deviceCount);

    for (ULONG i = 0; i < Context->Count; i++)
        Context->Disks[i].Present = FALSE;

    for (ULONG i = 0; i < deviceCount; i++)
    {
        PPH_STRING description = NULL;
        PPH_STRING path;
        HANDLE handle;
        ULONG diskIndex = ULONG_MAX;
        NVME_HEALTH_INFO_LOG health;
        PDT_DISK disk;
        PH_FORMAT format[5];
        BOOLEAN ata;
        BOOLEAN rotational;
        BOOLEAN known;
        DEVINST devInst;

        path = PhCreateString((PWSTR)deviceObjects[i].pszObjectId);

        // \\?\ for an NT path, as FindDiskDrives does.
        if (path->Length >= 2 * sizeof(WCHAR) && path->Buffer[1] == OBJ_NAME_PATH_SEPARATOR)
            path->Buffer[1] = L'?';

        PhAddItemList(listed, path);

        known = FALSE;

        for (ULONG d = 0; d < Context->Count; d++)
        {
            if (PhEqualString(Context->Disks[d].Path, path, TRUE))
            {
                Context->Disks[d].Present = TRUE;
                known = TRUE;
            }
        }

        if (known || Context->Count == DT_MAX_DISKS || DiskTemperatureFindPath(Context->Probed, path) != ULONG_MAX)
            continue;

        PhAddItemList(Context->Probed, PhReferenceObject(path));

        if (!NT_SUCCESS(PhCreateFile(
            &handle,
            &path->sr,
            FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            continue;
        }

        ata = FALSE;
        rotational = FALSE;
        devInst = 0;

        if (!NT_SUCCESS(DiskDriveQueryNvmeHealthInfo(handle, &health)))
        {
            FLOAT celsius;

            // An ATA disk is read once here only when it is solid state or awake.
            rotational = DiskTemperatureIsRotational(handle);

            if (deviceObjects[i].cPropertyCount && deviceObjects[i].pProperties[0].Buffer)
                CM_Locate_DevNodeW(&devInst, (PWSTR)deviceObjects[i].pProperties[0].Buffer, CM_LOCATE_DEVNODE_NORMAL);

            if ((rotational && !DiskTemperatureIsAwake(devInst)) || !NT_SUCCESS(DiskTemperatureQuerySmart(handle, &celsius)))
            {
                NtClose(handle);
                continue;
            }

            ata = TRUE;
        }

        DiskDriveQueryDeviceTypeAndNumber(handle, &diskIndex, NULL);
        NtClose(handle);
        QueryDiskDeviceInterfaceDescription((PWSTR)deviceObjects[i].pszObjectId, &description);

        disk = &Context->Disks[Context->Count++];
        disk->Path = PhReferenceObject(path);
        disk->Present = TRUE;
        disk->Index = diskIndex;
        disk->Ata = ata;
        disk->Rotational = rotational;
        disk->DevInst = devInst;
        disk->LastStatus = STATUS_PENDING;

        PhInitFormatS(&format[0], L"Disk ");
        PhInitFormatU(&format[1], diskIndex);
        PhInitFormatS(&format[2], L" (");
        if (description)
            PhInitFormatSR(&format[3], description->sr);
        else
            PhInitFormatSR(&format[3], path->sr);

        PhInitFormatC(&format[4], L')');
        disk->Name = PhFormat(format, RTL_NUMBER_OF(format), 0);

        PhClearReference(&description);
    }

    // A path no longer listed is tried again when it returns.
    for (ULONG i = Context->Probed->Count; i > 0; i--)
    {
        if (DiskTemperatureFindPath(listed, Context->Probed->Items[i - 1]) == ULONG_MAX)
        {
            PhDereferenceObject(Context->Probed->Items[i - 1]);
            PhRemoveItemList(Context->Probed, i - 1);
        }
    }

    for (ULONG i = 0; i < Context->Count; i++)
    {
        if (!Context->Disks[i].Present)
            memset(Context->Disks[i].Valid, 0, sizeof(Context->Disks[i].Valid));
    }

    PhDereferenceObjects(listed->Items, listed->Count);
    PhDereferenceObject(listed);
    PhDevFreeObjects(deviceCount, deviceObjects);
}

/**
 * Reads one disk's temperatures when its interval has passed. A sensor that reads 0 K is not implemented.
 */
static VOID DiskTemperaturePoll(
    _Inout_ PDT_DISK Disk,
    _In_ ULONG64 Now
    )
{
    NVME_HEALTH_INFO_LOG health;
    USHORT kelvin[DT_SENSORS];
    HANDLE handle;

    if (!Disk->Present || (Disk->LastPollTime && Now - Disk->LastPollTime < (Disk->Ata ? DT_ATA_INTERVAL_MS : DT_INTERVAL_MS)))
        return;

    Disk->LastPollTime = Now;

    // A sleeping hard disk keeps its row, without a value, until Windows wakes it.
    if (Disk->Ata && Disk->Rotational && !DiskTemperatureIsAwake(Disk->DevInst))
    {
        memset(Disk->Valid, 0, sizeof(Disk->Valid));
        Disk->LastStatus = STATUS_DEVICE_POWERED_OFF;
        return;
    }

    Disk->LastStatus = PhCreateFile(
        &handle,
        &Disk->Path->sr,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(Disk->LastStatus))
    {
        memset(Disk->Valid, 0, sizeof(Disk->Valid));
        return;
    }

    if (Disk->Ata)
    {
        memset(Disk->Valid, 0, sizeof(Disk->Valid));
        Disk->LastStatus = DiskTemperatureQuerySmart(handle, &Disk->Celsius[0]);
        Disk->Valid[0] = NT_SUCCESS(Disk->LastStatus);
        NtClose(handle);
        return;
    }

    Disk->LastStatus = DiskDriveQueryNvmeHealthInfo(handle, &health);
    NtClose(handle);

    if (!NT_SUCCESS(Disk->LastStatus))
    {
        memset(Disk->Valid, 0, sizeof(Disk->Valid));
        return;
    }

    kelvin[0] = *(PUSHORT)health.Temperature;
    kelvin[1] = health.TemperatureSensor1;
    kelvin[2] = health.TemperatureSensor2;
    kelvin[3] = health.TemperatureSensor3;
    kelvin[4] = health.TemperatureSensor4;
    kelvin[5] = health.TemperatureSensor5;
    kelvin[6] = health.TemperatureSensor6;
    kelvin[7] = health.TemperatureSensor7;
    kelvin[8] = health.TemperatureSensor8;

    for (ULONG s = 0; s < DT_SENSORS; s++)
    {
        Disk->Valid[s] = kelvin[s] != 0;
        Disk->Celsius[s] = (FLOAT)kelvin[s] - 273.15f;
    }
}

_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI DiskTemperatureSourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PDT_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;
    ULONG64 now;

    if (!context)
        return;

    now = DiskTemperatureQueryTime();

    if (!context->Enumerated || !context->LastPollTime || now - context->LastPollTime >= DT_INTERVAL_MS)
    {
        PhAcquireQueuedLockExclusive(&context->Lock);

        if (!context->Enumerated || ++context->Polls >= DT_ENUMERATE_POLLS)
        {
            context->Enumerated = TRUE;
            context->Polls = 0;
            DiskTemperatureEnumerate(context);
        }

        context->LastPollTime = now;

        for (ULONG i = 0; i < context->Count; i++)
            DiskTemperaturePoll(&context->Disks[i], now);

        PhReleaseQueuedLockExclusive(&context->Lock);
    }

    PhAcquireQueuedLockShared(&context->Lock);

    for (ULONG i = 0; i < context->Count; i++)
    {
        PDT_DISK disk = &context->Disks[i];

        for (ULONG s = 0; s < DT_SENSORS; s++)
        {
            if (!disk->Valid[s])
                continue;

            memset(&report, 0, sizeof(PH_HW_SENSOR_REPORT));
            report.Size = sizeof(PH_HW_SENSOR_REPORT);
            report.Flags = PH_HW_SENSOR_REPORT_HISTORY;
            report.ComponentId = i;
            report.Type = PhHwSensorTypeTemperature;
            report.ComponentName = disk->Name->Buffer;
            report.SensorName = disk->Ata ? L"Temperature" : DiskTemperatureSensorNames[s];
            report.SensorId = s;
            report.Value = disk->Celsius[s];
            report.ComponentClass = &DiskTemperatureClass;

            Report(ReportContext, &report);
        }
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_DIAGNOSTICS_CALLBACK)
static VOID NTAPI DiskTemperatureSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_opt_ PVOID Context
    )
{
    PDT_CONTEXT context = Context;

    if (!context)
        return;

    PhAcquireQueuedLockShared(&context->Lock);

    PhAppendFormatStringBuilder(Builder, L"Disks: %lu\n", context->Count);

    for (ULONG i = 0; i < context->Count; i++)
    {
        PDT_DISK disk = &context->Disks[i];
        PCWSTR bus = L"NVMe";
        PCWSTR rotational = L"";

        if (disk->Ata)
            bus = L"ATA";
        if (disk->Rotational)
            rotational = L", rotational";

        PhAppendFormatStringBuilder(
            Builder,
            L"  %s: %s%s, status 0x%08lx",
            disk->Name->Buffer,
            bus,
            rotational,
            disk->LastStatus
            );

        for (ULONG s = 0; s < DT_SENSORS; s++)
        {
            if (disk->Valid[s])
                PhAppendFormatStringBuilder(Builder, L", %s %.1f C", DiskTemperatureSensorNames[s], disk->Celsius[s]);
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

/**
 * Registers the NVMe temperature source. Called on plugin load.
 */
VOID DiskTemperatureInitialize(
    VOID
    )
{
    PH_HW_SENSOR_SOURCE source;

    if (DiskTemperatureContext)
        return;

    DiskTemperatureContext = PhAllocateZero(sizeof(DT_CONTEXT));
    PhInitializeQueuedLock(&DiskTemperatureContext->Lock);

    memset(&source, 0, sizeof(PH_HW_SENSOR_SOURCE));
    source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    source.Callback = DiskTemperatureSourceCallback;
    source.Context = DiskTemperatureContext;
    source.Diagnostics = DiskTemperatureSourceDiagnostics;

    if (!NT_SUCCESS(PhRegisterHardwareSensorSource(PluginInstance, &source, &DiskTemperatureRegistration)))
    {
        DiskTemperatureRegistration = NULL;
        DiskTemperatureUninitialize();
    }
}

/**
 * Unregisters the source and closes the disks. Called on plugin unload.
 */
VOID DiskTemperatureUninitialize(
    VOID
    )
{
    PDT_CONTEXT context = DiskTemperatureContext;

    // Unregistering waits for a callback in progress.
    if (DiskTemperatureRegistration)
    {
        PhUnregisterHardwareSensorSource(DiskTemperatureRegistration);
        DiskTemperatureRegistration = NULL;
    }

    if (!context)
        return;

    for (ULONG i = 0; i < context->Count; i++)
    {
        PhClearReference(&context->Disks[i].Path);
        PhClearReference(&context->Disks[i].Name);
    }

    if (context->Probed)
    {
        PhDereferenceObjects(context->Probed->Items, context->Probed->Count);
        PhDereferenceObject(context->Probed);
    }

    PhFree(context);
    DiskTemperatureContext = NULL;
}
