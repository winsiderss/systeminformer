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
// NVMe drive temperatures as a hardware sensor source: each NVMe disk's composite temperature and its temperature
// sensors 1 to 8 from the SMART / health information log page, read every DT_INTERVAL_MS.
//

#define DT_INTERVAL_MS 5000
#define DT_MAX_DISKS 32
#define DT_SENSORS 9 // the composite temperature, then sensors 1 to 8

typedef struct _DT_DISK
{
    HANDLE Handle;
    ULONG Index;
    PPH_STRING Name;
    NTSTATUS LastStatus;
    BOOLEAN Valid[DT_SENSORS];
    FLOAT Celsius[DT_SENSORS];
} DT_DISK, *PDT_DISK;

typedef struct _DT_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    BOOLEAN Enumerated;
    ULONG64 LastPollTime;
    ULONG Count;
    DT_DISK Disks[DT_MAX_DISKS];
} DT_CONTEXT, *PDT_CONTEXT;

static PDT_CONTEXT DiskTemperatureContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION DiskTemperatureRegistration = NULL;

static PCWSTR DiskTemperatureSensorNames[DT_SENSORS] =
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
 * Lists the NVMe disks: those whose health information log answers. Called on the provider thread.
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

    for (ULONG i = 0; i < deviceCount && Context->Count < DT_MAX_DISKS; i++)
    {
        PPH_STRING description = NULL;
        PPH_STRING path;
        HANDLE handle;
        ULONG diskIndex = ULONG_MAX;
        NVME_HEALTH_INFO_LOG health;
        PDT_DISK disk;
        PH_FORMAT format[5];

        path = PhCreateString((PWSTR)deviceObjects[i].pszObjectId);

        // \\?\ for an NT path, as FindDiskDrives does.
        if (path->Length >= 2 * sizeof(WCHAR) && path->Buffer[1] == OBJ_NAME_PATH_SEPARATOR)
            path->Buffer[1] = L'?';

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
            PhDereferenceObject(path);
            continue;
        }

        if (!NT_SUCCESS(DiskDriveQueryNvmeHealthInfo(handle, &health)))
        {
            NtClose(handle);
            PhDereferenceObject(path);
            continue;
        }

        DiskDriveQueryDeviceTypeAndNumber(handle, &diskIndex, NULL);
        QueryDiskDeviceInterfaceDescription((PWSTR)deviceObjects[i].pszObjectId, &description);

        disk = &Context->Disks[Context->Count++];
        disk->Handle = handle;
        disk->Index = diskIndex;
        disk->LastStatus = STATUS_PENDING;

        PhInitFormatS(&format[0], L"Disk ");
        PhInitFormatU(&format[1], diskIndex);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatSR(&format[3], description ? description->sr : path->sr);
        PhInitFormatC(&format[4], L')');
        disk->Name = PhFormat(format, RTL_NUMBER_OF(format), 0);

        PhClearReference(&description);
        PhDereferenceObject(path);
    }

    PhDevFreeObjects(deviceCount, deviceObjects);
}

/**
 * Reads one disk's temperatures. A sensor that reads 0 K is not implemented.
 */
static VOID DiskTemperaturePoll(
    _Inout_ PDT_DISK Disk
    )
{
    NVME_HEALTH_INFO_LOG health;
    USHORT kelvin[DT_SENSORS];

    Disk->LastStatus = DiskDriveQueryNvmeHealthInfo(Disk->Handle, &health);

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

        if (!context->Enumerated)
        {
            context->Enumerated = TRUE;
            DiskTemperatureEnumerate(context);
        }

        context->LastPollTime = now;

        for (ULONG i = 0; i < context->Count; i++)
            DiskTemperaturePoll(&context->Disks[i]);

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
            report.SensorName = DiskTemperatureSensorNames[s];
            report.SensorId = s;
            report.Value = disk->Celsius[s];

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

    PhAppendFormatStringBuilder(Builder, L"NVMe disks: %lu\n", context->Count);

    for (ULONG i = 0; i < context->Count; i++)
    {
        PDT_DISK disk = &context->Disks[i];

        PhAppendFormatStringBuilder(Builder, L"  %s: status 0x%08lx", disk->Name->Buffer, disk->LastStatus);

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
        NtClose(context->Disks[i].Handle);
        PhClearReference(&context->Disks[i].Name);
    }

    PhFree(context);
    DiskTemperatureContext = NULL;
}
