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
// An ASUS board's hardware monitor through its firmware's WMI method (the asuswmi script) as a hardware sensor source:
// the sensors the firmware lists, identified once and read every AW_INTERVAL_MS while the driver is connected, each
// read running the board's AML. Water flow sensors are left out (no sensor type for their unit). A board the script
// does not list ends the source for the session.
//

#define AW_INTERVAL_MS 3000

typedef struct _AW_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    BOOLEAN Identified;
    BOOLEAN Stopped;
    BOOLEAN Valid;
    ULONG64 LastPollTime;
    NTSTATUS LastStatus;
    KPH_HW_ASUSWMI_IDENTITY Identity;
    UCHAR Sources[KPH_HW_ASUSWMI_SENSORS];
    KPH_HW_VALUE32 Values[KPH_HW_ASUSWMI_SENSORS];
    WCHAR Names[KPH_HW_ASUSWMI_SENSORS][RTL_FIELD_SIZE(KPH_HW_ASUSWMI_SENSOR, Name) + 1];
} AW_CONTEXT, *PAW_CONTEXT;

static PAW_CONTEXT AsusWmiContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION AsusWmiRegistration = NULL;
// GUID_DEVCLASS_SYSTEM
static CONST GUID AsusWmiClass = { 0x4d36e97d, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };

/**
 * Identifies the board once, then reads it.
 */
static VOID AsusWmiPoll(
    _Inout_ PAW_CONTEXT Context
    )
{
    NTSTATUS status;
    KPH_HW_VALUE32 values[KPH_HW_ASUSWMI_SENSORS];

    if (!Context->Identified)
    {
        status = KphHwAsusWmiIdentify(&Context->Identity);

        PhAcquireQueuedLockExclusive(&Context->Lock);

        if (NT_SUCCESS(status))
        {
            Context->Identified = TRUE;

            for (ULONG i = 0; i < Context->Identity.Count; i++)
            {
                PKPH_HW_ASUSWMI_SENSOR sensor = &Context->Identity.Sensors[i];
                ULONG j;

                Context->Sources[i] = sensor->Source;

                for (j = 0; j < RTL_NUMBER_OF(sensor->Name) && sensor->Name[j]; j++)
                    Context->Names[i][j] = (WCHAR)(UCHAR)sensor->Name[j];

                Context->Names[i][j] = UNICODE_NULL;
            }
        }

        Context->LastStatus = status;

        if (status == STATUS_NOT_SUPPORTED || status == STATUS_NOT_FOUND)
            Context->Stopped = TRUE;

        PhReleaseQueuedLockExclusive(&Context->Lock);

        if (!NT_SUCCESS(status))
            return;
    }

    status = KphHwAsusWmiRead(Context->Sources, Context->Identity.Count, values);

    PhAcquireQueuedLockExclusive(&Context->Lock);

    Context->LastStatus = status;
    Context->Valid = NT_SUCCESS(status);

    if (NT_SUCCESS(status))
        memcpy(Context->Values, values, sizeof(KPH_HW_VALUE32) * Context->Identity.Count);

    PhReleaseQueuedLockExclusive(&Context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI AsusWmiSourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PAW_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;
    ULONG64 now;

    if (!context || context->Stopped || KsiLevel() != KphLevelMax)
        return;

    now = NtGetTickCount64();

    if (!context->LastPollTime || now - context->LastPollTime >= AW_INTERVAL_MS)
    {
        context->LastPollTime = now;
        AsusWmiPoll(context);
    }

    PhAcquireQueuedLockShared(&context->Lock);

    if (context->Valid)
    {
        memset(&report, 0, sizeof(PH_HW_SENSOR_REPORT));
        report.Size = sizeof(PH_HW_SENSOR_REPORT);
        report.Flags = PH_HW_SENSOR_REPORT_HISTORY;
        report.ComponentId = 0;
        report.ComponentName = L"ASUS hardware monitor";
        report.ComponentClass = &AsusWmiClass;

        for (ULONG i = 0; i < context->Identity.Count; i++)
        {
            PKPH_HW_ASUSWMI_SENSOR sensor = &context->Identity.Sensors[i];
            ULONG raw = context->Values[i].Value;
            DOUBLE value = sensor->Type == KPH_HW_ASUSWMI_UNSIGNED ? (DOUBLE)raw : (DOUBLE)(LONG)raw;

            if (!NT_SUCCESS(context->Values[i].Status))
                continue;

            switch (sensor->DataType)
            {
            case KPH_HW_ASUSWMI_VOLTAGE:
                report.Type = PhHwSensorTypeVoltage;
                value /= 1000000.0;
                break;
            case KPH_HW_ASUSWMI_TEMPERATURE:
                report.Type = PhHwSensorTypeTemperature;
                break;
            case KPH_HW_ASUSWMI_FAN:
                report.Type = PhHwSensorTypeFan;
                break;
            case KPH_HW_ASUSWMI_CURRENT:
                report.Type = PhHwSensorTypeCurrent;
                break;
            default:
                continue;
            }

            report.SensorName = context->Names[i];
            report.SensorId = i;
            report.Value = (FLOAT)value;
            Report(ReportContext, &report);
        }
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_DIAGNOSTICS_CALLBACK)
static VOID NTAPI AsusWmiSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_opt_ PVOID Context
    )
{
    PAW_CONTEXT context = Context;

    if (!context)
        return;

    PhAcquireQueuedLockShared(&context->Lock);

    if (!context->LastPollTime)
    {
        PhAppendStringBuilder2(Builder, L"ASUS hardware monitor: not read (no driver)\n");
    }
    else
    {
        PCWSTR stopped = L"";

        if (context->Stopped)
            stopped = L" (stopped)";

        PhAppendFormatStringBuilder(
            Builder,
            L"ASUS hardware monitor: status 0x%08lx%s; version %lu, %lu sensors\n",
            context->LastStatus,
            stopped,
            context->Identity.Version,
            context->Identity.Count
            );
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

/**
 * Registers the ASUS hardware monitor source. Called on plugin load.
 */
VOID AsusWmiInitialize(
    VOID
    )
{
    PH_HW_SENSOR_SOURCE source;

    if (AsusWmiContext)
        return;

    AsusWmiContext = PhAllocateZero(sizeof(AW_CONTEXT));
    PhInitializeQueuedLock(&AsusWmiContext->Lock);

    memset(&source, 0, sizeof(PH_HW_SENSOR_SOURCE));
    source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    source.Callback = AsusWmiSourceCallback;
    source.Context = AsusWmiContext;
    source.Diagnostics = AsusWmiSourceDiagnostics;

    if (!NT_SUCCESS(PhRegisterHardwareSensorSource(PluginInstance, &source, &AsusWmiRegistration)))
    {
        AsusWmiRegistration = NULL;
        AsusWmiUninitialize();
    }
}

/**
 * Unregisters the source. Called on plugin unload.
 */
VOID AsusWmiUninitialize(
    VOID
    )
{
    // Unregistering waits for a callback in progress.
    if (AsusWmiRegistration)
    {
        PhUnregisterHardwareSensorSource(AsusWmiRegistration);
        AsusWmiRegistration = NULL;
    }

    if (AsusWmiContext)
    {
        PhFree(AsusWmiContext);
        AsusWmiContext = NULL;
    }
}
