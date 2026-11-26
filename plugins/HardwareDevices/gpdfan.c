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
// The fan of a GPD handheld as a hardware sensor source, read from its embedded controller through the gpd script
// every GF_INTERVAL_MS while the driver is connected. A machine the script does not list, or GF_STRIKE_LIMIT failures
// in a row other than contention, end the source for the session.
//

#define GF_INTERVAL_MS 2000
#define GF_STRIKE_LIMIT 3

typedef struct _GF_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    BOOLEAN Unsupported;
    BOOLEAN Valid;
    UCHAR Strikes;
    ULONG64 LastPollTime;
    ULONG Rpm;
    NTSTATUS LastStatus;
} GF_CONTEXT, *PGF_CONTEXT;

static PGF_CONTEXT GpdFanContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION GpdFanRegistration = NULL;
// GUID_DEVCLASS_SYSTEM
static CONST GUID GpdFanClass = { 0x4d36e97d, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };

_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI GpdFanSourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PGF_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;
    ULONG64 now;

    if (!context || context->Unsupported || KsiLevel() != KphLevelMax)
        return;

    now = NtGetTickCount64();

    if (!context->LastPollTime || now - context->LastPollTime >= GF_INTERVAL_MS)
    {
        ULONG rpm = 0;
        NTSTATUS status;

        context->LastPollTime = now;
        status = KphHwGpdReadFan(&rpm);

        PhAcquireQueuedLockExclusive(&context->Lock);
        context->LastStatus = status;
        context->Valid = NT_SUCCESS(status);
        context->Rpm = rpm;

        if (NT_SUCCESS(status))
            context->Strikes = 0;
        else if (status == STATUS_NOT_SUPPORTED || (status != STATUS_RETRY && ++context->Strikes >= GF_STRIKE_LIMIT))
            context->Unsupported = TRUE;

        PhReleaseQueuedLockExclusive(&context->Lock);
    }

    if (!context->Valid)
        return;

    memset(&report, 0, sizeof(PH_HW_SENSOR_REPORT));
    report.Size = sizeof(PH_HW_SENSOR_REPORT);
    report.Flags = PH_HW_SENSOR_REPORT_HISTORY;
    report.ComponentId = 0;
    report.Type = PhHwSensorTypeFan;
    report.ComponentName = L"GPD embedded controller";
    report.SensorName = L"Fan";
    report.SensorId = 0;
    report.Value = (FLOAT)context->Rpm;
    report.ComponentClass = &GpdFanClass;

    Report(ReportContext, &report);
}

_Function_class_(PH_HW_SENSOR_SOURCE_DIAGNOSTICS_CALLBACK)
static VOID NTAPI GpdFanSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_opt_ PVOID Context
    )
{
    PGF_CONTEXT context = Context;

    if (!context)
        return;

    PhAcquireQueuedLockShared(&context->Lock);

    if (context->LastPollTime && context->Unsupported)
        PhAppendFormatStringBuilder(Builder, L"GPD fan: status 0x%08lx (not a listed handheld)\n", context->LastStatus);
    else if (context->LastPollTime)
        PhAppendFormatStringBuilder(Builder, L"GPD fan: status 0x%08lx\n", context->LastStatus);
    else
        PhAppendStringBuilder2(Builder, L"GPD fan: not read (no driver)\n");

    PhReleaseQueuedLockShared(&context->Lock);
}

/**
 * Registers the GPD fan source. Called on plugin load.
 */
VOID GpdFanInitialize(
    VOID
    )
{
    PH_HW_SENSOR_SOURCE source;

    if (GpdFanContext)
        return;

    GpdFanContext = PhAllocateZero(sizeof(GF_CONTEXT));
    PhInitializeQueuedLock(&GpdFanContext->Lock);

    memset(&source, 0, sizeof(PH_HW_SENSOR_SOURCE));
    source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    source.Callback = GpdFanSourceCallback;
    source.Context = GpdFanContext;
    source.Diagnostics = GpdFanSourceDiagnostics;

    if (!NT_SUCCESS(PhRegisterHardwareSensorSource(PluginInstance, &source, &GpdFanRegistration)))
    {
        GpdFanRegistration = NULL;
        GpdFanUninitialize();
    }
}

/**
 * Unregisters the source. Called on plugin unload.
 */
VOID GpdFanUninitialize(
    VOID
    )
{
    // Unregistering waits for a callback in progress.
    if (GpdFanRegistration)
    {
        PhUnregisterHardwareSensorSource(GpdFanRegistration);
        GpdFanRegistration = NULL;
    }

    if (GpdFanContext)
    {
        PhFree(GpdFanContext);
        GpdFanContext = NULL;
    }
}
