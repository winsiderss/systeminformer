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
// A Dell machine's fans and temperatures through its firmware's SMM interface as a hardware sensor source, identified
// once and read every DS_INTERVAL_MS while the driver is connected, labeled by
// type. Each command stalls every processor while the firmware runs, so the interval is long, and a machine the script
// does not list or whose firmware was slow ends the source for the session.
//

#define DS_INTERVAL_MS 3000

typedef struct _DS_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    BOOLEAN Identified;
    BOOLEAN Stopped;
    BOOLEAN Valid;
    ULONG64 LastPollTime;
    NTSTATUS LastStatus;
    KPH_HW_DELL_IDENTITY Identity;
    ULONG FanMask;
    ULONG TempMask;
    LONG Values[KPH_HW_DELL_FANS + KPH_HW_DELL_TEMPS];
    WCHAR Names[KPH_HW_DELL_FANS + KPH_HW_DELL_TEMPS][32];
} DS_CONTEXT, *PDS_CONTEXT;

static PDS_CONTEXT DellSmmContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION DellSmmRegistration = NULL;
// GUID_DEVCLASS_SYSTEM
static CONST GUID DellSmmClass = { 0x4d36e97d, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };

static CONST PCWSTR DellSmmFanLabels[] =
{
    L"Processor Fan", L"Motherboard Fan", L"Video Fan", L"Power Supply Fan", L"Chipset Fan", L"Other Fan",
};
static CONST PCWSTR DellSmmTempLabels[] =
{
    L"CPU", L"GPU", L"SODIMM", L"Other", L"Ambient", L"Other",
};

/**
 * Names each present fan and sensor by its type; a type that repeats, or a fan of no known type, by its number too.
 */
static VOID DellSmmNameSensors(
    _Inout_ PDS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < KPH_HW_DELL_FANS + KPH_HW_DELL_TEMPS; i++)
    {
        BOOLEAN fan = i < KPH_HW_DELL_FANS;
        ULONG type = fan ? Context->Identity.FanTypes[i] : Context->Identity.TempTypes[i - KPH_HW_DELL_FANS];
        PCWSTR label;
        BOOLEAN repeated = FALSE;
        PCWSTR docking = L"";
        ULONG ordinal;

        if (type == 0xFF)
            continue;

        if (fan && type == 0xFE)
        {
            _snwprintf_s(Context->Names[i], RTL_NUMBER_OF(Context->Names[i]), _TRUNCATE, L"Fan %lu", i + 1);
            continue;
        }

        if (fan)
            label = DellSmmFanLabels[min(type & 0x0F, RTL_NUMBER_OF(DellSmmFanLabels) - 1)];
        else
            label = DellSmmTempLabels[min(type, RTL_NUMBER_OF(DellSmmTempLabels) - 1)];

        ULONG first = fan ? 0 : KPH_HW_DELL_FANS;
        ULONG end = fan ? KPH_HW_DELL_FANS : KPH_HW_DELL_FANS + KPH_HW_DELL_TEMPS;

        for (ULONG j = first; j < end; j++)
        {
            ULONG other = fan ? Context->Identity.FanTypes[j] : Context->Identity.TempTypes[j - KPH_HW_DELL_FANS];

            if (j != i && other == type)
                repeated = TRUE;
        }

        // Bit 4 of a fan's type is a docking station's fan.
        if (fan && (type & 0x10))
            docking = L"Docking ";

        if (fan)
            ordinal = i + 1;
        else
            ordinal = i - KPH_HW_DELL_FANS + 1;

        if (repeated)
        {
            _snwprintf_s(
                Context->Names[i],
                RTL_NUMBER_OF(Context->Names[i]),
                _TRUNCATE,
                L"%s%s %lu",
                docking,
                label,
                ordinal
                );
        }
        else
        {
            _snwprintf_s(
                Context->Names[i],
                RTL_NUMBER_OF(Context->Names[i]),
                _TRUNCATE,
                L"%s%s",
                docking,
                label
                );
        }
    }
}

/**
 * Identifies the machine once, then reads it.
 */
static VOID DellSmmPoll(
    _Inout_ PDS_CONTEXT Context
    )
{
    NTSTATUS status;
    LONG values[KPH_HW_DELL_FANS + KPH_HW_DELL_TEMPS];

    if (!Context->Identified)
    {
        KPH_HW_DELL_IDENTITY identity;

        status = KphHwDellIdentify(&identity);

        PhAcquireQueuedLockExclusive(&Context->Lock);

        if (NT_SUCCESS(status))
        {
            Context->Identified = TRUE;
            Context->Identity = identity;

            for (ULONG i = 0; i < KPH_HW_DELL_FANS; i++)
            {
                if (identity.FanTypes[i] != 0xFF)
                    Context->FanMask |= 1UL << i;
            }

            for (ULONG i = 0; i < KPH_HW_DELL_TEMPS; i++)
            {
                if (identity.TempTypes[i] != 0xFF)
                    Context->TempMask |= 1UL << i;
            }

            DellSmmNameSensors(Context);
        }

        Context->LastStatus = status;

        if (status == STATUS_NOT_SUPPORTED || status == STATUS_IO_TIMEOUT || status == STATUS_NOT_FOUND)
            Context->Stopped = TRUE;

        PhReleaseQueuedLockExclusive(&Context->Lock);

        if (!NT_SUCCESS(status))
            return;
    }

    status = KphHwDellRead(Context->FanMask, Context->TempMask, values);

    PhAcquireQueuedLockExclusive(&Context->Lock);

    Context->LastStatus = status;
    Context->Valid = NT_SUCCESS(status);

    if (NT_SUCCESS(status))
        memcpy(Context->Values, values, sizeof(values));
    if (status == STATUS_IO_TIMEOUT)
        Context->Stopped = TRUE;

    PhReleaseQueuedLockExclusive(&Context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI DellSmmSourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PDS_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;
    ULONG64 now;

    if (!context || context->Stopped || KsiLevel() != KphLevelMax)
        return;

    now = NtGetTickCount64();

    if (!context->LastPollTime || now - context->LastPollTime >= DS_INTERVAL_MS)
    {
        context->LastPollTime = now;
        DellSmmPoll(context);
    }

    PhAcquireQueuedLockShared(&context->Lock);

    if (context->Valid)
    {
        memset(&report, 0, sizeof(PH_HW_SENSOR_REPORT));
        report.Size = sizeof(PH_HW_SENSOR_REPORT);
        report.Flags = PH_HW_SENSOR_REPORT_HISTORY;
        report.ComponentId = 0;
        report.ComponentName = L"Dell system management";
        report.ComponentClass = &DellSmmClass;

        for (ULONG i = 0; i < KPH_HW_DELL_FANS + KPH_HW_DELL_TEMPS; i++)
        {
            BOOLEAN fan = i < KPH_HW_DELL_FANS;

            if (!(fan ? context->FanMask & (1UL << i) : context->TempMask & (1UL << (i - KPH_HW_DELL_FANS))))
                continue;
            if (context->Values[i] < 0)
                continue;

            report.Type = fan ? PhHwSensorTypeFan : PhHwSensorTypeTemperature;
            report.SensorName = context->Names[i];
            report.SensorId = i;
            report.Value = fan ? (FLOAT)((ULONG)context->Values[i] * context->Identity.Multiplier) : (FLOAT)context->Values[i];
            Report(ReportContext, &report);
        }
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_DIAGNOSTICS_CALLBACK)
static VOID NTAPI DellSmmSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_opt_ PVOID Context
    )
{
    PDS_CONTEXT context = Context;

    if (!context)
        return;

    PhAcquireQueuedLockShared(&context->Lock);

    if (!context->LastPollTime)
    {
        PhAppendStringBuilder2(Builder, L"Dell SMM: not read (no driver)\n");
    }
    else
    {
        PCWSTR stopped = L"";

        if (context->LastStatus == STATUS_IO_TIMEOUT)
            stopped = L" (slow firmware: stopped)";
        else if (context->Stopped)
            stopped = L" (stopped)";

        PhAppendFormatStringBuilder(
            Builder,
            L"Dell SMM: status 0x%08lx%s; fans 0x%lx, sensors 0x%lx, multiplier %lu\n",
            context->LastStatus,
            stopped,
            context->FanMask,
            context->TempMask,
            context->Identity.Multiplier
            );
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

/**
 * Registers the Dell SMM source. Called on plugin load.
 */
VOID DellSmmInitialize(
    VOID
    )
{
    PH_HW_SENSOR_SOURCE source;

    if (DellSmmContext)
        return;

    DellSmmContext = PhAllocateZero(sizeof(DS_CONTEXT));
    PhInitializeQueuedLock(&DellSmmContext->Lock);

    memset(&source, 0, sizeof(PH_HW_SENSOR_SOURCE));
    source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    source.Callback = DellSmmSourceCallback;
    source.Context = DellSmmContext;
    source.Diagnostics = DellSmmSourceDiagnostics;

    if (!NT_SUCCESS(PhRegisterHardwareSensorSource(PluginInstance, &source, &DellSmmRegistration)))
    {
        DellSmmRegistration = NULL;
        DellSmmUninitialize();
    }
}

/**
 * Unregisters the source. Called on plugin unload.
 */
VOID DellSmmUninitialize(
    VOID
    )
{
    // Unregistering waits for a callback in progress.
    if (DellSmmRegistration)
    {
        PhUnregisterHardwareSensorSource(DellSmmRegistration);
        DellSmmRegistration = NULL;
    }

    if (DellSmmContext)
    {
        PhFree(DellSmmContext);
        DellSmmContext = NULL;
    }
}
