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
// The NPU of AMD family 1Ah models 80h to 8Fh from the metrics table of socket 0's HSMP mailbox (the hsmp script's
// metrics) as a hardware sensor source: the table holds accumulators, so each value is the change of an accumulator
// over the change of the table's counter between two reads AP_INTERVAL_MS apart, in Q10. Only the fields with a known
// scale are reported. A processor the script does not list, or AP_STRIKE_LIMIT failures in a row other than
// contention or missing hardware data, end the source for the session.
//

#define AP_INTERVAL_MS 2000
#define AP_STRIKE_LIMIT 3
#define AP_NOINTERFACE_POLLS 10 // polls between tries while there is no hardware data yet
#define AP_COUNTER 0x000
#define AP_NPU_POWER 0x1D0
#define AP_AIE_CLOCK 0x210
#define AP_NPUH_CLOCK 0x218
#define AP_NPU_BUSY 0x240
#define AP_NPU_BUSY_COUNT 3
#define AP_NPU_TEMPERATURE 0x4A0
#define AP_SENSORS (4 + AP_NPU_BUSY_COUNT)

typedef struct _AP_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    BOOLEAN Stopped;
    BOOLEAN HavePrevious;
    BOOLEAN Valid;
    UCHAR Strikes;
    UCHAR Backoff; // polls left before the next try after STATUS_NOINTERFACE
    ULONG64 LastPollTime;
    HD_WAKE Wake;
    NTSTATUS LastStatus;
    FLOAT Values[AP_SENSORS];
    UCHAR Previous[KPH_HW_HSMP_METRICS_SIZE];
    UCHAR Current[KPH_HW_HSMP_METRICS_SIZE];
} AP_CONTEXT, *PAP_CONTEXT;

static PAP_CONTEXT AmdNpuContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION AmdNpuRegistration = NULL;
// GUID_DEVCLASS_COMPUTEACCELERATOR
static CONST GUID AmdNpuClass = { 0xf01a9d53, 0x3ff6, 0x48d2, { 0x9f, 0x97, 0xc8, 0xa7, 0x00, 0x4b, 0xe1, 0x0c } };

static CONST struct
{
    ULONG Type;
    PCWSTR Name;
    ULONG Offset;
} AmdNpuSensors[AP_SENSORS] =
{
    { PhHwSensorTypeTemperature, L"NPU", AP_NPU_TEMPERATURE },
    { PhHwSensorTypePower, L"NPU", AP_NPU_POWER },
    { PhHwSensorTypeClock, L"NPU", AP_AIE_CLOCK },
    { PhHwSensorTypeClock, L"NPU host", AP_NPUH_CLOCK },
    { PhHwSensorTypeControl, L"NPU busy 0", AP_NPU_BUSY + 0 * sizeof(ULONG64) },
    { PhHwSensorTypeControl, L"NPU busy 1", AP_NPU_BUSY + 1 * sizeof(ULONG64) },
    { PhHwSensorTypeControl, L"NPU busy 2", AP_NPU_BUSY + 2 * sizeof(ULONG64) },
};

/**
 * The average of an accumulator between two tables in Q10 units, or 0 when the counter or the value went backwards.
 */
static DOUBLE AmdNpuAverage(
    _In_reads_bytes_(KPH_HW_HSMP_METRICS_SIZE) PCUCHAR Previous,
    _In_reads_bytes_(KPH_HW_HSMP_METRICS_SIZE) PCUCHAR Current,
    _In_ ULONG Offset
    )
{
    ULONG previousCounter;
    ULONG currentCounter;
    ULONG64 previousValue;
    ULONG64 currentValue;

    memcpy(&previousCounter, Previous + AP_COUNTER, sizeof(ULONG));
    memcpy(&currentCounter, Current + AP_COUNTER, sizeof(ULONG));
    memcpy(&previousValue, Previous + Offset, sizeof(ULONG64));
    memcpy(&currentValue, Current + Offset, sizeof(ULONG64));

    if (currentCounter <= previousCounter || currentValue < previousValue)
        return 0.0;

    return (DOUBLE)(currentValue - previousValue) / (DOUBLE)(currentCounter - previousCounter) / 1024.0;
}

/**
 * Reads the table and averages it against the read before.
 */
static VOID AmdNpuPoll(
    _Inout_ PAP_CONTEXT Context
    )
{
    NTSTATUS status;

    status = KphHwHsmpReadMetrics(0, Context->Current);

    PhAcquireQueuedLockExclusive(&Context->Lock);

    Context->LastStatus = status;

    if (NT_SUCCESS(status))
        Context->Strikes = 0;
    else if (status == STATUS_NOINTERFACE) // a later activation can supply the data
        Context->Backoff = AP_NOINTERFACE_POLLS - 1;
    else if (status == STATUS_NOT_SUPPORTED || (status != STATUS_RETRY && ++Context->Strikes >= AP_STRIKE_LIMIT))
        Context->Stopped = TRUE;

    if (NT_SUCCESS(status))
    {
        if (Context->HavePrevious)
        {
            for (ULONG i = 0; i < AP_SENSORS; i++)
            {
                Context->Values[i] = (FLOAT)AmdNpuAverage(
                    Context->Previous,
                    Context->Current,
                    AmdNpuSensors[i].Offset
                    );
            }

            Context->Valid = TRUE;
        }

        memcpy(Context->Previous, Context->Current, KPH_HW_HSMP_METRICS_SIZE);
        Context->HavePrevious = TRUE;
    }
    else
    {
        Context->Valid = FALSE;
        Context->HavePrevious = FALSE;
    }

    PhReleaseQueuedLockExclusive(&Context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI AmdNpuSourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PAP_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;
    ULONG64 now;

    if (!context)
        return;

    now = NtGetTickCount64();

    if (HardwareDeviceWoke(&context->Wake, now))
    {
        PhAcquireQueuedLockExclusive(&context->Lock);
        context->Stopped = FALSE;
        context->Strikes = 0;
        context->Backoff = 0;
        PhReleaseQueuedLockExclusive(&context->Lock);
    }

    if (context->Stopped || KsiLevel() != KphLevelMax)
        return;

    if (!context->LastPollTime || now - context->LastPollTime >= AP_INTERVAL_MS)
    {
        context->LastPollTime = now;

        if (context->Backoff)
            context->Backoff--;
        else
            AmdNpuPoll(context);
    }

    PhAcquireQueuedLockShared(&context->Lock);

    if (context->Valid)
    {
        memset(&report, 0, sizeof(PH_HW_SENSOR_REPORT));
        report.Size = sizeof(PH_HW_SENSOR_REPORT);
        report.Flags = PH_HW_SENSOR_REPORT_HISTORY;
        report.ComponentId = 0;
        report.ComponentName = L"AMD NPU";
        report.ComponentClass = &AmdNpuClass;

        for (ULONG i = 0; i < AP_SENSORS; i++)
        {
            report.Type = AmdNpuSensors[i].Type;
            report.SensorName = AmdNpuSensors[i].Name;
            report.SensorId = i;
            report.Value = context->Values[i];
            Report(ReportContext, &report);
        }
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_DIAGNOSTICS_CALLBACK)
static VOID NTAPI AmdNpuSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_opt_ PVOID Context
    )
{
    PAP_CONTEXT context = Context;

    if (!context)
        return;

    PhAcquireQueuedLockShared(&context->Lock);

    if (!context->LastPollTime)
    {
        PhAppendStringBuilder2(Builder, L"AMD NPU: not read (no driver)\n");
    }
    else
    {
        PCWSTR stopped = L"";

        if (context->Stopped)
        {
            stopped = L" (stopped)";

            if (context->LastStatus == STATUS_NOT_SUPPORTED)
                stopped = L" (not a listed processor)";
        }

        PhAppendFormatStringBuilder(
            Builder,
            L"AMD NPU: status 0x%08lx%s\n",
            context->LastStatus,
            stopped
            );
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

/**
 * Registers the AMD NPU source. Called on plugin load.
 */
VOID AmdNpuInitialize(
    VOID
    )
{
    PH_HW_SENSOR_SOURCE source;

    if (AmdNpuContext)
        return;

    AmdNpuContext = PhAllocateZero(sizeof(AP_CONTEXT));
    PhInitializeQueuedLock(&AmdNpuContext->Lock);

    memset(&source, 0, sizeof(PH_HW_SENSOR_SOURCE));
    source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    source.Callback = AmdNpuSourceCallback;
    source.Context = AmdNpuContext;
    source.Diagnostics = AmdNpuSourceDiagnostics;

    if (!NT_SUCCESS(PhRegisterHardwareSensorSource(PluginInstance, &source, &AmdNpuRegistration)))
    {
        AmdNpuRegistration = NULL;
        AmdNpuUninitialize();
    }
}

/**
 * Unregisters the source. Called on plugin unload.
 */
VOID AmdNpuUninitialize(
    VOID
    )
{
    // Unregistering waits for a callback in progress.
    if (AmdNpuRegistration)
    {
        PhUnregisterHardwareSensorSource(AmdNpuRegistration);
        AmdNpuRegistration = NULL;
    }

    if (AmdNpuContext)
    {
        PhFree(AmdNpuContext);
        AmdNpuContext = NULL;
    }
}
