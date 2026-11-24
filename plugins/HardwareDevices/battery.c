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
// phnt defines ThermalStandby (a power information level), a field name in poclass.h.
#pragma push_macro("ThermalStandby")
#undef ThermalStandby
#include <poclass.h>
#pragma pop_macro("ThermalStandby")

//
// Batteries as a hardware sensor source: each battery's charge level, charge or discharge rate, voltage,
// temperature (when it reports one) and wear level (full charged over design capacity), read every BT_INTERVAL_MS
// from the battery class driver.
//

#define BT_INTERVAL_MS 5000
#define BT_MAX_BATTERIES 4

typedef enum _BT_SENSOR
{
    BtSensorCharge, // percent of the full charged capacity
    BtSensorWear, // percent: full charged over design capacity
    BtSensorRate, // watts, positive charging
    BtSensorVoltage, // volts
    BtSensorTemperature, // degrees Celsius
    BtSensorMaximum
} BT_SENSOR;

typedef struct _BT_BATTERY
{
    HANDLE Handle;
    ULONG Tag;
    ULONG DesignedCapacity; // mWh
    ULONG FullChargedCapacity; // mWh
    PPH_STRING Name;
    NTSTATUS LastStatus;
    BOOLEAN Valid[BtSensorMaximum];
    FLOAT Value[BtSensorMaximum];
} BT_BATTERY, *PBT_BATTERY;

typedef struct _BT_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    BOOLEAN Enumerated;
    ULONG64 LastPollTime;
    ULONG Count;
    BT_BATTERY Batteries[BT_MAX_BATTERIES];
} BT_CONTEXT, *PBT_CONTEXT;

// GUID_DEVICE_BATTERY, also GUID_DEVCLASS_BATTERY
static CONST GUID BatteryInterfaceGuid = { 0x72631e54, 0x78a4, 0x11d0, { 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a } };

static PBT_CONTEXT BatteryContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION BatteryRegistration = NULL;

static CONST struct { ULONG Type; PCWSTR Name; } BatterySensors[BtSensorMaximum] =
{
    { PhHwSensorTypeControl, L"Charge level" },
    { PhHwSensorTypeControl, L"Wear level" },
    { PhHwSensorTypePower, L"Charge rate" },
    { PhHwSensorTypeVoltage, L"Voltage" },
    { PhHwSensorTypeTemperature, L"Temperature" },
};

static NTSTATUS BatteryQueryInformation(
    _In_ PBT_BATTERY Battery,
    _In_ BATTERY_QUERY_INFORMATION_LEVEL Level,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    BATTERY_QUERY_INFORMATION query;

    memset(&query, 0, sizeof(BATTERY_QUERY_INFORMATION));
    query.BatteryTag = Battery->Tag;
    query.InformationLevel = Level;

    return PhDeviceIoControlFile(Battery->Handle, IOCTL_BATTERY_QUERY_INFORMATION, &query, sizeof(query), Buffer, Length, NULL);
}

/**
 * Lists the batteries whose capacity is in mWh. Called on the provider thread.
 */
static VOID BatteryEnumerate(
    _Inout_ PBT_CONTEXT Context
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
        { DEVPROP_OPERATOR_EQUALS, {{ DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL }, DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&BatteryInterfaceGuid } }
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

    for (ULONG i = 0; i < deviceCount && Context->Count < BT_MAX_BATTERIES; i++)
    {
        PPH_STRING path;
        BT_BATTERY battery;
        ULONG timeout = 0;
        BATTERY_INFORMATION information;
        WCHAR deviceName[64];
        PH_FORMAT format[4];

        memset(&battery, 0, sizeof(BT_BATTERY));
        path = PhCreateString((PWSTR)deviceObjects[i].pszObjectId);

        // \\?\ for an NT path, as FindDiskDrives does.
        if (path->Length >= 2 * sizeof(WCHAR) && path->Buffer[1] == OBJ_NAME_PATH_SEPARATOR)
            path->Buffer[1] = L'?';

        if (!NT_SUCCESS(PhCreateFile(
            &battery.Handle,
            &path->sr,
            FILE_READ_DATA | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            PhDereferenceObject(path);
            continue;
        }

        PhDereferenceObject(path);

        memset(&information, 0, sizeof(BATTERY_INFORMATION));

        // A tag of 0 is no battery in the slot.
        if (
            !NT_SUCCESS(PhDeviceIoControlFile(battery.Handle, IOCTL_BATTERY_QUERY_TAG, &timeout, sizeof(timeout), &battery.Tag, sizeof(battery.Tag), NULL)) ||
            !battery.Tag ||
            !NT_SUCCESS(BatteryQueryInformation(&battery, BatteryInformation, &information, sizeof(information))) ||
            (information.Capabilities & BATTERY_CAPACITY_RELATIVE) ||
            !information.FullChargedCapacity || information.FullChargedCapacity == BATTERY_UNKNOWN_CAPACITY
            )
        {
            NtClose(battery.Handle);
            continue;
        }

        battery.DesignedCapacity = information.DesignedCapacity;
        battery.FullChargedCapacity = information.FullChargedCapacity;
        battery.LastStatus = STATUS_PENDING;

        memset(deviceName, 0, sizeof(deviceName));
        BatteryQueryInformation(&battery, BatteryDeviceName, deviceName, sizeof(deviceName) - sizeof(WCHAR));

        PhInitFormatS(&format[0], L"Battery ");
        PhInitFormatU(&format[1], Context->Count);

        if (deviceName[0])
        {
            PhInitFormatS(&format[2], L" ");
            PhInitFormatS(&format[3], deviceName);
            battery.Name = PhFormat(format, 4, 0);
        }
        else
        {
            battery.Name = PhFormat(format, 2, 0);
        }

        Context->Batteries[Context->Count++] = battery;
    }

    PhDevFreeObjects(deviceCount, deviceObjects);
}

/**
 * Reads one battery's status and temperature.
 */
static VOID BatteryPoll(
    _Inout_ PBT_BATTERY Battery
    )
{
    BATTERY_WAIT_STATUS wait;
    BATTERY_STATUS status;
    ULONG temperature = 0;

    memset(Battery->Valid, 0, sizeof(Battery->Valid));
    memset(&wait, 0, sizeof(BATTERY_WAIT_STATUS));
    wait.BatteryTag = Battery->Tag;

    Battery->LastStatus = PhDeviceIoControlFile(Battery->Handle, IOCTL_BATTERY_QUERY_STATUS, &wait, sizeof(wait), &status, sizeof(status), NULL);

    if (!NT_SUCCESS(Battery->LastStatus))
        return;

    if (status.Capacity != BATTERY_UNKNOWN_CAPACITY)
    {
        Battery->Valid[BtSensorCharge] = TRUE;
        Battery->Value[BtSensorCharge] = min((FLOAT)status.Capacity * 100.0f / (FLOAT)Battery->FullChargedCapacity, 100.0f);
    }

    if (Battery->DesignedCapacity && Battery->DesignedCapacity != BATTERY_UNKNOWN_CAPACITY)
    {
        Battery->Valid[BtSensorWear] = TRUE;
        Battery->Value[BtSensorWear] = (FLOAT)Battery->FullChargedCapacity * 100.0f / (FLOAT)Battery->DesignedCapacity;
    }

    if ((ULONG)status.Rate != BATTERY_UNKNOWN_RATE)
    {
        Battery->Valid[BtSensorRate] = TRUE;
        Battery->Value[BtSensorRate] = (FLOAT)status.Rate / 1000.0f;
    }

    if (status.Voltage != BATTERY_UNKNOWN_VOLTAGE)
    {
        Battery->Valid[BtSensorVoltage] = TRUE;
        Battery->Value[BtSensorVoltage] = (FLOAT)status.Voltage / 1000.0f;
    }

    // Tenths of a kelvin; most batteries do not answer.
    if (NT_SUCCESS(BatteryQueryInformation(Battery, BatteryTemperature, &temperature, sizeof(temperature))) && temperature)
    {
        Battery->Valid[BtSensorTemperature] = TRUE;
        Battery->Value[BtSensorTemperature] = (FLOAT)temperature / 10.0f - 273.15f;
    }
}

_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI BatterySourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PBT_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;
    ULONG64 now;

    if (!context)
        return;

    now = NtGetTickCount64();

    if (!context->Enumerated || !context->LastPollTime || now - context->LastPollTime >= BT_INTERVAL_MS)
    {
        PhAcquireQueuedLockExclusive(&context->Lock);

        if (!context->Enumerated)
        {
            context->Enumerated = TRUE;
            BatteryEnumerate(context);
        }

        context->LastPollTime = now;

        for (ULONG i = 0; i < context->Count; i++)
            BatteryPoll(&context->Batteries[i]);

        PhReleaseQueuedLockExclusive(&context->Lock);
    }

    PhAcquireQueuedLockShared(&context->Lock);

    for (ULONG i = 0; i < context->Count; i++)
    {
        PBT_BATTERY battery = &context->Batteries[i];

        for (ULONG s = 0; s < BtSensorMaximum; s++)
        {
            if (!battery->Valid[s])
                continue;

            memset(&report, 0, sizeof(PH_HW_SENSOR_REPORT));
            report.Size = sizeof(PH_HW_SENSOR_REPORT);
            report.Flags = PH_HW_SENSOR_REPORT_HISTORY;
            report.ComponentId = i;
            report.Type = BatterySensors[s].Type;
            report.ComponentName = battery->Name->Buffer;
            report.SensorName = BatterySensors[s].Name;
            report.SensorId = s;
            report.Value = battery->Value[s];
            report.ComponentClass = &BatteryInterfaceGuid;

            Report(ReportContext, &report);
        }
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_DIAGNOSTICS_CALLBACK)
static VOID NTAPI BatterySourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_opt_ PVOID Context
    )
{
    PBT_CONTEXT context = Context;

    if (!context)
        return;

    PhAcquireQueuedLockShared(&context->Lock);

    PhAppendFormatStringBuilder(Builder, L"Batteries: %lu\n", context->Count);

    for (ULONG i = 0; i < context->Count; i++)
    {
        PBT_BATTERY battery = &context->Batteries[i];

        PhAppendFormatStringBuilder(
            Builder,
            L"  %s: tag %lu, design %lu mWh, full charged %lu mWh, status 0x%08lx\n",
            battery->Name->Buffer,
            battery->Tag,
            battery->DesignedCapacity,
            battery->FullChargedCapacity,
            battery->LastStatus
            );
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

/**
 * Registers the battery source. Called on plugin load.
 */
VOID BatteryInitialize(
    VOID
    )
{
    PH_HW_SENSOR_SOURCE source;

    if (BatteryContext)
        return;

    BatteryContext = PhAllocateZero(sizeof(BT_CONTEXT));
    PhInitializeQueuedLock(&BatteryContext->Lock);

    memset(&source, 0, sizeof(PH_HW_SENSOR_SOURCE));
    source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    source.Callback = BatterySourceCallback;
    source.Context = BatteryContext;
    source.Diagnostics = BatterySourceDiagnostics;

    if (!NT_SUCCESS(PhRegisterHardwareSensorSource(PluginInstance, &source, &BatteryRegistration)))
    {
        BatteryRegistration = NULL;
        BatteryUninitialize();
    }
}

/**
 * Unregisters the source and closes the batteries. Called on plugin unload.
 */
VOID BatteryUninitialize(
    VOID
    )
{
    PBT_CONTEXT context = BatteryContext;

    // Unregistering waits for a callback in progress.
    if (BatteryRegistration)
    {
        PhUnregisterHardwareSensorSource(BatteryRegistration);
        BatteryRegistration = NULL;
    }

    if (!context)
        return;

    for (ULONG i = 0; i < context->Count; i++)
    {
        NtClose(context->Batteries[i].Handle);
        PhClearReference(&context->Batteries[i].Name);
    }

    PhFree(context);
    BatteryContext = NULL;
}
