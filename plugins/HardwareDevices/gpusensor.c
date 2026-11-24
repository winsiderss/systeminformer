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
// Graphics adapters as a hardware sensor source: each adapter's temperature, fan speed, power (percent of its limit),
// memory clock (D3DKMT adapter performance data) and its first 3D engine's clock and voltage (node performance data),
// read every GS_INTERVAL_MS. A sensor is listed once its adapter reports it non-zero; adapters whose performance
// data fails (Microsoft Basic Render) are left out.
//

#define GS_INTERVAL_MS 1000
#define GS_REENUMERATE_MS 30000
#define GS_MAX_ADAPTERS 16
#define GS_MAX_NODES 64

typedef enum _GS_SENSOR
{
    GsSensorTemperature, // degrees Celsius
    GsSensorFan, // RPM
    GsSensorPower, // percent of the adapter's power limit
    GsSensorCoreClock, // MHz, the first 3D engine
    GsSensorCoreVoltage, // volts, the first 3D engine
    GsSensorMemoryClock, // MHz
    GsSensorMaximum
} GS_SENSOR;

typedef struct _GS_ADAPTER
{
    D3DKMT_HANDLE Handle;
    ULONG Node; // the first 3D engine; ULONG_MAX when none reports a clock
    PPH_STRING Name;
    NTSTATUS LastStatus;
    BOOLEAN Seen[GsSensorMaximum]; // reported non-zero once
    BOOLEAN Valid[GsSensorMaximum];
    FLOAT Value[GsSensorMaximum];
} GS_ADAPTER, *PGS_ADAPTER;

typedef struct _GS_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    BOOLEAN Enumerated;
    BOOLEAN Failed; // an adapter's query failed: enumerate again after GS_REENUMERATE_MS
    ULONG64 LastPollTime;
    ULONG64 LastEnumerateTime;
    ULONG Count;
    GS_ADAPTER Adapters[GS_MAX_ADAPTERS];
} GS_CONTEXT, *PGS_CONTEXT;

static PGS_CONTEXT GpuSensorContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION GpuSensorRegistration = NULL;
// GUID_DEVCLASS_DISPLAY
static CONST GUID GpuSensorClass = { 0x4d36e968, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };

static CONST struct { ULONG Type; PCWSTR Name; } GpuSensors[GsSensorMaximum] =
{
    { PhHwSensorTypeTemperature, L"Temperature" },
    { PhHwSensorTypeFan, L"Fan" },
    { PhHwSensorTypeControl, L"Power (of limit)" },
    { PhHwSensorTypeClock, L"Core clock" },
    { PhHwSensorTypeVoltage, L"Core voltage" },
    { PhHwSensorTypeClock, L"Memory clock" },
};

/**
 * Closes the adapters.
 */
static VOID GpuSensorClose(
    _Inout_ PGS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->Count; i++)
    {
        GraphicsCloseAdapterHandle(Context->Adapters[i].Handle);
        PhClearReference(&Context->Adapters[i].Name);
    }

    Context->Count = 0;
}

/**
 * Finds an adapter's first 3D engine that reports a clock.
 */
static ULONG GpuSensorFindNode(
    _In_ D3DKMT_HANDLE Handle,
    _In_ LUID Luid
    )
{
    ULONG segments;
    ULONG nodes;

    if (!NT_SUCCESS(GraphicsQueryAdapterNodeInformation(Luid, &segments, &nodes)))
        return ULONG_MAX;

    for (ULONG n = 0; n < nodes && n < GS_MAX_NODES; n++)
    {
        D3DKMT_NODEMETADATA metadata;
        ULONG64 frequency = 0;
        ULONG64 maximum = 0;

        memset(&metadata, 0, sizeof(D3DKMT_NODEMETADATA));
        metadata.NodeOrdinalAndAdapterIndex = MAKEWORD(n, 0);

        if (!NT_SUCCESS(GraphicsQueryAdapterInformation(Handle, KMTQAITYPE_NODEMETADATA, &metadata, sizeof(D3DKMT_NODEMETADATA))))
            continue;
        if (metadata.NodeData.EngineType != DXGK_ENGINE_TYPE_3D)
            continue;

        if (NT_SUCCESS(GraphicsQueryAdapterDeviceNodePerfData(Handle, n, &frequency, &maximum, NULL, NULL, NULL, NULL, NULL)) && (frequency || maximum))
            return n;
    }

    return ULONG_MAX;
}

/**
 * Opens every adapter whose performance data answers.
 */
static VOID GpuSensorEnumerate(
    _Inout_ PGS_CONTEXT Context
    )
{
    D3DKMT_ENUMADAPTERS2 enumAdapters;
    D3DKMT_ADAPTERINFO* adapters;

    memset(&enumAdapters, 0, sizeof(D3DKMT_ENUMADAPTERS2));

    if (!NT_SUCCESS(D3DKMTEnumAdapters2(&enumAdapters)) || !enumAdapters.NumAdapters)
        return;

    adapters = PhAllocateZero(sizeof(D3DKMT_ADAPTERINFO) * enumAdapters.NumAdapters);
    enumAdapters.pAdapters = adapters;

    if (!NT_SUCCESS(D3DKMTEnumAdapters2(&enumAdapters)))
        enumAdapters.NumAdapters = 0;

    for (ULONG i = 0; i < enumAdapters.NumAdapters; i++)
    {
        D3DKMT_ADAPTER_PERFDATA perfData;
        D3DKMT_ADAPTERREGISTRYINFO registryInfo;
        PGS_ADAPTER adapter;

        memset(&perfData, 0, sizeof(D3DKMT_ADAPTER_PERFDATA));

        if (Context->Count >= GS_MAX_ADAPTERS ||
            !NT_SUCCESS(GraphicsQueryAdapterInformation(adapters[i].hAdapter, KMTQAITYPE_ADAPTERPERFDATA, &perfData, sizeof(D3DKMT_ADAPTER_PERFDATA))))
        {
            GraphicsCloseAdapterHandle(adapters[i].hAdapter);
            continue;
        }

        adapter = &Context->Adapters[Context->Count++];
        memset(adapter, 0, sizeof(GS_ADAPTER));
        adapter->Handle = adapters[i].hAdapter;
        adapter->Node = GpuSensorFindNode(adapters[i].hAdapter, adapters[i].AdapterLuid);
        adapter->LastStatus = STATUS_PENDING;

        memset(&registryInfo, 0, sizeof(D3DKMT_ADAPTERREGISTRYINFO));

        if (NT_SUCCESS(GraphicsQueryAdapterInformation(adapters[i].hAdapter, KMTQAITYPE_ADAPTERREGISTRYINFO, &registryInfo, sizeof(D3DKMT_ADAPTERREGISTRYINFO))) &&
            registryInfo.AdapterString[0])
        {
            registryInfo.AdapterString[RTL_NUMBER_OF(registryInfo.AdapterString) - 1] = UNICODE_NULL;
            adapter->Name = PhCreateString(registryInfo.AdapterString);
        }
        else
        {
            adapter->Name = PhFormatString(L"GPU %lu", Context->Count - 1);
        }
    }

    PhFree(adapters);
}

/**
 * Reads one adapter's sensors.
 */
static VOID GpuSensorPoll(
    _Inout_ PGS_ADAPTER Adapter,
    _Inout_ PGS_CONTEXT Context
    )
{
    D3DKMT_ADAPTER_PERFDATA perfData;
    FLOAT value[GsSensorMaximum] = { 0 };
    BOOLEAN read[GsSensorMaximum] = { 0 };
    ULONG64 frequency;
    ULONG voltage;

    memset(&perfData, 0, sizeof(D3DKMT_ADAPTER_PERFDATA));
    Adapter->LastStatus = GraphicsQueryAdapterInformation(Adapter->Handle, KMTQAITYPE_ADAPTERPERFDATA, &perfData, sizeof(D3DKMT_ADAPTER_PERFDATA));

    if (NT_SUCCESS(Adapter->LastStatus))
    {
        value[GsSensorTemperature] = (FLOAT)perfData.Temperature / 10.0f;
        value[GsSensorFan] = (FLOAT)perfData.FanRPM;
        value[GsSensorPower] = (FLOAT)perfData.Power / 10.0f;
        value[GsSensorMemoryClock] = (FLOAT)((DOUBLE)perfData.MemoryFrequency / 1000000.0);
        read[GsSensorTemperature] = read[GsSensorFan] = read[GsSensorPower] = read[GsSensorMemoryClock] = TRUE;
    }
    else
    {
        Context->Failed = TRUE;
    }

    if (Adapter->Node != ULONG_MAX &&
        NT_SUCCESS(GraphicsQueryAdapterDeviceNodePerfData(Adapter->Handle, Adapter->Node, &frequency, NULL, NULL, &voltage, NULL, NULL, NULL)))
    {
        value[GsSensorCoreClock] = (FLOAT)((DOUBLE)frequency / 1000000.0);
        value[GsSensorCoreVoltage] = (FLOAT)voltage / 1000.0f;
        read[GsSensorCoreClock] = read[GsSensorCoreVoltage] = TRUE;
    }

    for (ULONG s = 0; s < GsSensorMaximum; s++)
    {
        if (read[s] && value[s] != 0.0f)
            Adapter->Seen[s] = TRUE;

        // A sensor that reported once keeps reporting, a stopped fan's 0 RPM included.
        Adapter->Valid[s] = read[s] && Adapter->Seen[s];
        Adapter->Value[s] = value[s];
    }
}

_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI GpuSensorSourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PGS_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;
    ULONG64 now;

    if (!context)
        return;

    now = NtGetTickCount64();

    if (!context->Enumerated || !context->LastPollTime || now - context->LastPollTime >= GS_INTERVAL_MS)
    {
        PhAcquireQueuedLockExclusive(&context->Lock);

        if (!context->Enumerated || (context->Failed && now - context->LastEnumerateTime >= GS_REENUMERATE_MS))
        {
            GpuSensorClose(context);
            context->Enumerated = TRUE;
            context->Failed = FALSE;
            context->LastEnumerateTime = now;
            GpuSensorEnumerate(context);
        }

        context->LastPollTime = now;

        for (ULONG i = 0; i < context->Count; i++)
            GpuSensorPoll(&context->Adapters[i], context);

        PhReleaseQueuedLockExclusive(&context->Lock);
    }

    PhAcquireQueuedLockShared(&context->Lock);

    for (ULONG i = 0; i < context->Count; i++)
    {
        PGS_ADAPTER adapter = &context->Adapters[i];

        for (ULONG s = 0; s < GsSensorMaximum; s++)
        {
            if (!adapter->Valid[s])
                continue;

            memset(&report, 0, sizeof(PH_HW_SENSOR_REPORT));
            report.Size = sizeof(PH_HW_SENSOR_REPORT);
            report.Flags = PH_HW_SENSOR_REPORT_HISTORY;
            report.ComponentId = i;
            report.Type = GpuSensors[s].Type;
            report.ComponentName = adapter->Name->Buffer;
            report.SensorName = GpuSensors[s].Name;
            report.SensorId = s;
            report.Value = adapter->Value[s];
            report.ComponentClass = &GpuSensorClass;

            Report(ReportContext, &report);
        }
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_DIAGNOSTICS_CALLBACK)
static VOID NTAPI GpuSensorSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_opt_ PVOID Context
    )
{
    PGS_CONTEXT context = Context;

    if (!context)
        return;

    PhAcquireQueuedLockShared(&context->Lock);

    PhAppendFormatStringBuilder(Builder, L"Graphics adapters: %lu\n", context->Count);

    for (ULONG i = 0; i < context->Count; i++)
    {
        PGS_ADAPTER adapter = &context->Adapters[i];

        if (adapter->Node != ULONG_MAX)
            PhAppendFormatStringBuilder(Builder, L"  %s: 3D engine node %lu, status 0x%08lx\n", adapter->Name->Buffer, adapter->Node, adapter->LastStatus);
        else
            PhAppendFormatStringBuilder(Builder, L"  %s: no engine clock, status 0x%08lx\n", adapter->Name->Buffer, adapter->LastStatus);
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

/**
 * Registers the graphics adapter source. Called on plugin load.
 */
VOID GpuSensorInitialize(
    VOID
    )
{
    PH_HW_SENSOR_SOURCE source;

    if (GpuSensorContext)
        return;

    GpuSensorContext = PhAllocateZero(sizeof(GS_CONTEXT));
    PhInitializeQueuedLock(&GpuSensorContext->Lock);

    memset(&source, 0, sizeof(PH_HW_SENSOR_SOURCE));
    source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    source.Callback = GpuSensorSourceCallback;
    source.Context = GpuSensorContext;
    source.Diagnostics = GpuSensorSourceDiagnostics;

    if (!NT_SUCCESS(PhRegisterHardwareSensorSource(PluginInstance, &source, &GpuSensorRegistration)))
    {
        GpuSensorRegistration = NULL;
        GpuSensorUninitialize();
    }
}

/**
 * Unregisters the source and closes the adapters. Called on plugin unload.
 */
VOID GpuSensorUninitialize(
    VOID
    )
{
    PGS_CONTEXT context = GpuSensorContext;

    // Unregistering waits for a callback in progress.
    if (GpuSensorRegistration)
    {
        PhUnregisterHardwareSensorSource(GpuSensorRegistration);
        GpuSensorRegistration = NULL;
    }

    if (!context)
        return;

    GpuSensorClose(context);
    PhFree(context);
    GpuSensorContext = NULL;
}
