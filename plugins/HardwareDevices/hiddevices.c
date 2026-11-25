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
// Coolers, fan controllers and power supplies on USB HID as a hardware sensor source, read through the driver while
// it is connected: the coolers that push their status every HD_PASSIVE_INTERVAL_MS (each read waits for the device's
// next report), the devices that answer requests every HD_ACTIVE_INTERVAL_MS under their tool mutex (another tool
// holding it is tried again at the next callback). Each device keeps its component id while it stays: the slot of its
// vendor, product and order among devices of that model.
//

#define HD_PASSIVE_INTERVAL_MS 10000
#define HD_ACTIVE_INTERVAL_MS 5000
#define HD_SLOTS (KPH_HW_HID_DEVICES * 2)

typedef struct _HD_DEVICE
{
    ULONG Key; // vendor << 16 | product
    ULONG Order; // among the devices of the same key in one read
    BOOLEAN Active;
    BOOLEAN Present;
    NTSTATUS LastStatus;
    PPH_STRING Name;
    ULONG Count;
    KPH_HW_HID_VALUE Values[KPH_HW_HID_VALUES];
} HD_DEVICE, *PHD_DEVICE;

typedef struct _HD_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    ULONG64 LastPassiveTime;
    ULONG64 LastActiveTime;
    NTSTATUS LastPassiveStatus;
    NTSTATUS LastActiveStatus;
    ULONG Count;
    HD_DEVICE Devices[HD_SLOTS];
} HD_CONTEXT, *PHD_CONTEXT;

typedef struct _HD_NAME
{
    ULONG Key;
    PCWSTR Name;
} HD_NAME;

static PHD_CONTEXT HidDevicesContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION HidDevicesRegistration = NULL;
// GUID_DEVCLASS_HIDCLASS
static CONST GUID HidDevicesClass = { 0x745a17a0, 0x74d3, 0x11d0, { 0xb6, 0xfe, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda } };

static CONST HD_NAME HidDevicesNames[] =
{
    { 0x1e71170e, L"NZXT Kraken X" },
    { 0x0c70f00e, L"Aquacomputer D5 Next" },
    { 0x0c70f00a, L"Aquacomputer Farbwerk" },
    { 0x0c70f010, L"Aquacomputer Farbwerk 360" },
    { 0x0c70f011, L"Aquacomputer Octo" },
    { 0x0c70f00d, L"Aquacomputer Quadro" },
    { 0x0c70f012, L"Aquacomputer High Flow Next" },
    { 0x0c70f00b, L"Aquacomputer Aquastream Ultimate" },
    { 0x1b1c0c10, L"Corsair Commander Pro" },
    { 0x1b1c1d00, L"Corsair Obsidian 1000D" },
};

static CONST PCWSTR HidDevicesKindNames[] =
{
    NULL, L"Temperature", L"Fan", NULL, L"Voltage", L"Current", L"Power"
};

static CONST ULONG HidDevicesKindTypes[] =
{
    0, PhHwSensorTypeTemperature, PhHwSensorTypeFan, 0, PhHwSensorTypeVoltage, PhHwSensorTypeCurrent,
    PhHwSensorTypePower
};

/**
 * Names a device by its model, a Corsair power supply by its maker, else by its vendor and product.
 */
static PPH_STRING HidDevicesName(
    _In_ ULONG Key
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(HidDevicesNames); i++)
    {
        if (HidDevicesNames[i].Key == Key)
            return PhCreateString((PWSTR)HidDevicesNames[i].Name);
    }

    if ((Key >> 16) == 0x1b1c)
        return PhFormatString(L"Corsair power supply (%04x)", Key & 0xFFFF);

    return PhFormatString(L"HID device %04x:%04x", Key >> 16, Key & 0xFFFF);
}

/**
 * Merges one read's records into the slots of their kind (Active), by key and order. Called with the lock held.
 */
static VOID HidDevicesMerge(
    _Inout_ PHD_CONTEXT Context,
    _In_ BOOLEAN Active,
    _In_reads_(Count) PKPH_HW_HID_DEVICE Records,
    _In_ ULONG Count
    )
{
    for (ULONG i = 0; i < Context->Count; i++)
    {
        if (Context->Devices[i].Active == Active)
            Context->Devices[i].Present = FALSE;
    }

    for (ULONG r = 0; r < Count; r++)
    {
        PKPH_HW_HID_DEVICE record = &Records[r];
        ULONG key = ((ULONG)record->VendorId << 16) | record->ProductId;
        ULONG order = 0;
        PHD_DEVICE device = NULL;

        for (ULONG p = 0; p < r; p++)
        {
            if ((((ULONG)Records[p].VendorId << 16) | Records[p].ProductId) == key)
                order++;
        }

        for (ULONG i = 0; i < Context->Count; i++)
        {
            if (Context->Devices[i].Active == Active && Context->Devices[i].Key == key &&
                Context->Devices[i].Order == order)
            {
                device = &Context->Devices[i];
                break;
            }
        }

        if (!device)
        {
            if (Context->Count == RTL_NUMBER_OF(Context->Devices))
                continue;

            device = &Context->Devices[Context->Count++];
            device->Key = key;
            device->Order = order;
            device->Active = Active;
            device->Name = HidDevicesName(key);
        }

        device->Present = TRUE;
        device->LastStatus = record->Status;
        device->Count = NT_SUCCESS(record->Status) ? min(record->Count, KPH_HW_HID_VALUES) : 0;
        memcpy(device->Values, record->Values, sizeof(device->Values));
    }
}

/**
 * Reads one kind of device when its interval has passed. Called on the provider thread with the lock held.
 */
static VOID HidDevicesPoll(
    _Inout_ PHD_CONTEXT Context,
    _In_ BOOLEAN Active,
    _In_ ULONG64 Now
    )
{
    PULONG64 lastTime = Active ? &Context->LastActiveTime : &Context->LastPassiveTime;
    PNTSTATUS lastStatus = Active ? &Context->LastActiveStatus : &Context->LastPassiveStatus;
    PKPH_HW_HID_DEVICE records;
    ULONG count = 0;
    NTSTATUS status;

    if (*lastTime && Now - *lastTime < (Active ? HD_ACTIVE_INTERVAL_MS : HD_PASSIVE_INTERVAL_MS))
        return;

    records = PhAllocate(KPH_HW_HID_DEVICES * sizeof(KPH_HW_HID_DEVICE));
    status = Active ? KphHwCorsairRead(records, &count) : KphHwHidRead(records, &count);
    *lastStatus = status;

    // Another tool holds the devices: the next callback tries again.
    if (status != STATUS_RETRY)
    {
        *lastTime = Now;
        HidDevicesMerge(Context, Active, records, NT_SUCCESS(status) ? count : 0);
    }

    PhFree(records);
}

_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI HidDevicesSourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PHD_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;
    ULONG64 now;

    if (!context || KsiLevel() != KphLevelMax)
        return;

    now = NtGetTickCount64();

    PhAcquireQueuedLockExclusive(&context->Lock);
    HidDevicesPoll(context, FALSE, now);
    HidDevicesPoll(context, TRUE, now);
    PhReleaseQueuedLockExclusive(&context->Lock);

    PhAcquireQueuedLockShared(&context->Lock);

    for (ULONG i = 0; i < context->Count; i++)
    {
        PHD_DEVICE device = &context->Devices[i];

        if (!device->Present)
            continue;

        for (ULONG v = 0; v < device->Count; v++)
        {
            PKPH_HW_HID_VALUE value = &device->Values[v];
            WCHAR sensorName[32];
            FLOAT scale;

            if (value->Kind >= RTL_NUMBER_OF(HidDevicesKindNames) || !HidDevicesKindNames[value->Kind])
                continue;

            scale = (value->Kind == KPH_HW_HID_FAN) ? 1.0f : 0.001f;
            _snwprintf_s(sensorName, RTL_NUMBER_OF(sensorName), _TRUNCATE, L"%s %u", HidDevicesKindNames[value->Kind],
                value->Index + 1);

            memset(&report, 0, sizeof(PH_HW_SENSOR_REPORT));
            report.Size = sizeof(PH_HW_SENSOR_REPORT);
            report.Flags = PH_HW_SENSOR_REPORT_HISTORY;
            report.ComponentId = i;
            report.Type = HidDevicesKindTypes[value->Kind];
            report.ComponentName = device->Name->Buffer;
            report.SensorName = sensorName;
            report.SensorId = ((ULONG)value->Kind << 8) | value->Index;
            report.Value = (FLOAT)value->Value * scale;
            report.ComponentClass = &HidDevicesClass;

            Report(ReportContext, &report);
        }
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_DIAGNOSTICS_CALLBACK)
static VOID NTAPI HidDevicesSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_opt_ PVOID Context
    )
{
    PHD_CONTEXT context = Context;

    if (!context)
        return;

    PhAcquireQueuedLockShared(&context->Lock);

    PhAppendFormatStringBuilder(
        Builder,
        L"Last reads: coolers 0x%08lx, request devices 0x%08lx; devices %lu\n",
        context->LastPassiveStatus,
        context->LastActiveStatus,
        context->Count
        );

    for (ULONG i = 0; i < context->Count; i++)
    {
        PHD_DEVICE device = &context->Devices[i];

        PhAppendFormatStringBuilder(
            Builder,
            L"  %s: %s, status 0x%08lx, %lu values\n",
            device->Name->Buffer,
            device->Present ? L"present" : L"gone",
            device->LastStatus,
            device->Count
            );
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

/**
 * Registers the HID device source. Called on plugin load.
 */
VOID HidDevicesInitialize(
    VOID
    )
{
    PH_HW_SENSOR_SOURCE source;

    if (HidDevicesContext)
        return;

    HidDevicesContext = PhAllocateZero(sizeof(HD_CONTEXT));
    PhInitializeQueuedLock(&HidDevicesContext->Lock);

    memset(&source, 0, sizeof(PH_HW_SENSOR_SOURCE));
    source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    source.Callback = HidDevicesSourceCallback;
    source.Context = HidDevicesContext;
    source.Diagnostics = HidDevicesSourceDiagnostics;

    if (!NT_SUCCESS(PhRegisterHardwareSensorSource(PluginInstance, &source, &HidDevicesRegistration)))
    {
        HidDevicesRegistration = NULL;
        HidDevicesUninitialize();
    }
}

/**
 * Unregisters the source. Called on plugin unload.
 */
VOID HidDevicesUninitialize(
    VOID
    )
{
    PHD_CONTEXT context = HidDevicesContext;

    // Unregistering waits for a callback in progress.
    if (HidDevicesRegistration)
    {
        PhUnregisterHardwareSensorSource(HidDevicesRegistration);
        HidDevicesRegistration = NULL;
    }

    if (!context)
        return;

    for (ULONG i = 0; i < context->Count; i++)
        PhClearReference(&context->Devices[i].Name);

    PhFree(context);
    HidDevicesContext = NULL;
}
