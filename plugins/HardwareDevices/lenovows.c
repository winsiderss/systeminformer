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
// A Lenovo ThinkStation's embedded controller (P620, P7, P5, P8) as a hardware sensor source: its temperatures and fan
// speeds through the lenovows script every LW_INTERVAL_MS while the driver is connected.
// A machine the script does not list, or LW_STRIKE_LIMIT failures in a row other than contention or missing hardware
// data, end the source for the session.
//

#define LW_INTERVAL_MS 2000
#define LW_STRIKE_LIMIT 3
#define LW_NOINTERFACE_POLLS 10 // polls between tries while there is no hardware data yet
#define LW_MAX_TEMPS 17
#define LW_MAX_FANS 16

typedef struct _LW_MODEL
{
    PCWSTR Name;
    ULONG TempCount;
    const PCWSTR* Temps;
    ULONG FanCount;
    const PCWSTR* Fans;
} LW_MODEL;

static CONST PCWSTR LenovoWsPxTemps[] =
{
    L"CPU1", L"CPU2", L"R_DIMM1", L"L_DIMM1", L"R_DIMM2", L"L_DIMM2", L"PCH", L"M2_R", L"M2_Z1R", L"M2_Z2R", L"PCI_Z1",
    L"PCI_Z2", L"PCI_Z3", L"PCI_Z4", L"AMB", L"PSU1", L"PSU2",
};
static CONST PCWSTR LenovoWsGenTemps[] =
{
    L"CPU1", L"R_DIMM", L"L_DIMM", L"PCH", L"M2_R", L"M2_Z1R", L"M2_Z2R", L"PCI_Z1", L"PCI_Z2", L"PCI_Z3", L"PCI_Z4",
    L"AMB", L"PSU",
};
static CONST PCWSTR LenovoWsP8Temps[] =
{
    L"CPU1", L"CPU_DIMM_BANK1", L"CPU_DIMM_BANK2", L"M2_Z2R", L"M2_Z3R", L"DIMM_RIGHT", L"DIMM_LEFT", L"PCI_Z1", L"PCI_Z2",
    L"PCI_Z3", L"AMB", L"REAR_VR", L"PSU",
};
static CONST PCWSTR LenovoWsPxFans[] =
{
    L"CPU1_Fan", L"CPU2_Fan", L"Front_Fan1-1", L"Front_Fan1-2", L"Front_Fan2", L"Front_Fan3", L"MEM_Fan1", L"MEM_Fan2",
    L"Rear_Fan1", L"Rear_Fan2", L"Flex_Bay_Fan1", L"Flex_Bay_Fan2", L"Flex_Bay_Fan2", L"PSU_HDD_Fan", L"PSU1_Fan",
    L"PSU2_Fan",
};
static CONST PCWSTR LenovoWsP7Fans[] =
{
    L"CPU1_Fan", L"HP_CPU_Fan1", L"HP_CPU_Fan2", L"PCIE1_4_Fan", L"PCIE5_7_Fan", L"MEM_Fan1", L"MEM_Fan2", L"Rear_Fan1",
    L"BCB_Fan", L"Flex_Bay_Fan", L"PSU_Fan",
};
static CONST PCWSTR LenovoWsP5Fans[] =
{
    L"CPU_Fan", L"HDD_Fan", L"Duct_Fan1", L"MEM_Fan", L"Rear_Fan", L"Front_Fan", L"Flex_Bay_Fan", L"PSU_Fan",
};
static CONST PCWSTR LenovoWsP8Fans[] =
{
    L"CPU1_Fan", L"CPU2_Fan", L"HP_CPU_Fan1", L"HP_CPU_Fan2", L"PCIE1_4_Fan", L"PCIE5_7_Fan", L"DIMM1_Fan1", L"DIMM1_Fan2",
    L"DIMM2_Fan1", L"DIMM2_Fan2", L"Rear_Fan", L"HDD_Bay_Fan", L"Flex_Bay_Fan", L"PSU_Fan",
};

// In the script's model order.
static CONST LW_MODEL LenovoWsModels[] =
{
    { L"ThinkStation P620", RTL_NUMBER_OF(LenovoWsPxTemps), LenovoWsPxTemps, RTL_NUMBER_OF(LenovoWsPxFans), LenovoWsPxFans },
    { L"ThinkStation P7", RTL_NUMBER_OF(LenovoWsGenTemps), LenovoWsGenTemps, RTL_NUMBER_OF(LenovoWsP7Fans), LenovoWsP7Fans },
    { L"ThinkStation P5", RTL_NUMBER_OF(LenovoWsGenTemps), LenovoWsGenTemps, RTL_NUMBER_OF(LenovoWsP5Fans), LenovoWsP5Fans },
    { L"ThinkStation P8", RTL_NUMBER_OF(LenovoWsP8Temps), LenovoWsP8Temps, RTL_NUMBER_OF(LenovoWsP8Fans), LenovoWsP8Fans },
};

typedef struct _LW_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    BOOLEAN Unsupported;
    BOOLEAN Valid;
    UCHAR Strikes;
    UCHAR Backoff; // polls left before the next try after STATUS_NOINTERFACE
    ULONG Model;
    ULONG64 LastPollTime;
    NTSTATUS LastStatus;
    LONG Temps[LW_MAX_TEMPS];
    ULONG Fans[LW_MAX_FANS];
} LW_CONTEXT, *PLW_CONTEXT;

static PLW_CONTEXT LenovoWsContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION LenovoWsRegistration = NULL;
// GUID_DEVCLASS_SYSTEM
static CONST GUID LenovoWsClass = { 0x4d36e97d, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };

/**
 * Reads the controller and keeps the values when the reply has the model's shape.
 */
static VOID LenovoWsPoll(
    _Inout_ PLW_CONTEXT Context
    )
{
    ULONG buffer[1 + LW_MAX_TEMPS + LW_MAX_FANS];
    ULONG returnLength = 0;
    NTSTATUS status;
    const LW_MODEL* model;

    status = KphHwLenovoWsRead(buffer, sizeof(buffer), &returnLength);

    PhAcquireQueuedLockExclusive(&Context->Lock);

    Context->Valid = FALSE;

    if (NT_SUCCESS(status) && (returnLength < sizeof(ULONG) || buffer[0] >= RTL_NUMBER_OF(LenovoWsModels)))
        status = STATUS_INFO_LENGTH_MISMATCH;

    if (NT_SUCCESS(status))
    {
        model = &LenovoWsModels[buffer[0]];

        if (returnLength != sizeof(ULONG) * (1 + model->TempCount + model->FanCount))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            Context->Model = buffer[0];
            memcpy(Context->Temps, &buffer[1], sizeof(ULONG) * model->TempCount);
            memcpy(Context->Fans, &buffer[1 + model->TempCount], sizeof(ULONG) * model->FanCount);
            Context->Valid = TRUE;
        }
    }

    if (NT_SUCCESS(status))
        Context->Strikes = 0;
    else if (status == STATUS_NOINTERFACE) // a later activation can supply the data
        Context->Backoff = LW_NOINTERFACE_POLLS - 1;
    else if (status == STATUS_NOT_SUPPORTED || (status != STATUS_RETRY && ++Context->Strikes >= LW_STRIKE_LIMIT))
        Context->Unsupported = TRUE;

    Context->LastStatus = status;

    PhReleaseQueuedLockExclusive(&Context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI LenovoWsSourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PLW_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;
    const LW_MODEL* model;
    ULONG64 now;

    if (!context || context->Unsupported || KsiLevel() != KphLevelMax)
        return;

    now = NtGetTickCount64();

    if (!context->LastPollTime || now - context->LastPollTime >= LW_INTERVAL_MS)
    {
        context->LastPollTime = now;

        if (context->Backoff)
            context->Backoff--;
        else
            LenovoWsPoll(context);
    }

    PhAcquireQueuedLockShared(&context->Lock);

    if (context->Valid)
    {
        model = &LenovoWsModels[context->Model];

        memset(&report, 0, sizeof(PH_HW_SENSOR_REPORT));
        report.Size = sizeof(PH_HW_SENSOR_REPORT);
        report.Flags = PH_HW_SENSOR_REPORT_HISTORY;
        report.ComponentId = 0;
        report.ComponentName = model->Name;
        report.ComponentClass = &LenovoWsClass;

        // A temperature the controller has no reading for is not reported.
        for (ULONG i = 0; i < model->TempCount; i++)
        {
            if (context->Temps[i] < 0)
                continue;

            report.Type = PhHwSensorTypeTemperature;
            report.SensorName = model->Temps[i];
            report.SensorId = i;
            report.Value = (FLOAT)context->Temps[i];
            Report(ReportContext, &report);
        }

        for (ULONG i = 0; i < model->FanCount; i++)
        {
            report.Type = PhHwSensorTypeFan;
            report.SensorName = model->Fans[i];
            report.SensorId = i;
            report.Value = (FLOAT)context->Fans[i];
            Report(ReportContext, &report);
        }
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_DIAGNOSTICS_CALLBACK)
static VOID NTAPI LenovoWsSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_opt_ PVOID Context
    )
{
    PLW_CONTEXT context = Context;
    PCWSTR ended = L"";

    if (!context)
        return;

    PhAcquireQueuedLockShared(&context->Lock);

    if (context->Unsupported)
    {
        ended = L" (stopped)";

        if (context->LastStatus == STATUS_NOT_SUPPORTED)
            ended = L" (not a listed workstation)";
    }

    if (!context->LastPollTime)
        PhAppendStringBuilder2(Builder, L"ThinkStation controller: not read (no driver)\n");
    else if (context->Valid)
        PhAppendFormatStringBuilder(Builder, L"ThinkStation controller: %s, status 0x%08lx\n", LenovoWsModels[context->Model].Name, context->LastStatus);
    else
        PhAppendFormatStringBuilder(Builder, L"ThinkStation controller: status 0x%08lx%s\n", context->LastStatus, ended);

    PhReleaseQueuedLockShared(&context->Lock);
}

/**
 * Registers the ThinkStation source. Called on plugin load.
 */
VOID LenovoWsInitialize(
    VOID
    )
{
    PH_HW_SENSOR_SOURCE source;

    if (LenovoWsContext)
        return;

    LenovoWsContext = PhAllocateZero(sizeof(LW_CONTEXT));
    PhInitializeQueuedLock(&LenovoWsContext->Lock);

    memset(&source, 0, sizeof(PH_HW_SENSOR_SOURCE));
    source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    source.Callback = LenovoWsSourceCallback;
    source.Context = LenovoWsContext;
    source.Diagnostics = LenovoWsSourceDiagnostics;

    if (!NT_SUCCESS(PhRegisterHardwareSensorSource(PluginInstance, &source, &LenovoWsRegistration)))
    {
        LenovoWsRegistration = NULL;
        LenovoWsUninitialize();
    }
}

/**
 * Unregisters the source. Called on plugin unload.
 */
VOID LenovoWsUninitialize(
    VOID
    )
{
    // Unregistering waits for a callback in progress.
    if (LenovoWsRegistration)
    {
        PhUnregisterHardwareSensorSource(LenovoWsRegistration);
        LenovoWsRegistration = NULL;
    }

    if (LenovoWsContext)
    {
        PhFree(LenovoWsContext);
        LenovoWsContext = NULL;
    }
}
