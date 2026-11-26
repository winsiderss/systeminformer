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

#include <hwprvp.h>
#include <kphhw.h>
#include <cpusensor.h>
#include <trace.h>

// Collector platform: the Intel platform windows the ACPI low power idle table leads to, one KphHwPlatformRead each
// per update: the LPIT S0ix residency counter, and the PMC's THERMAL_TSS0 as the chipset temperature when the PCH
// thermal function's window (collector mmio) is not its source.

#define PHP_HW_PLATFORM_STRIKE_LIMIT 3 // failures of a window before it leaves the plan for the session
#define PHP_HW_PLATFORM_LPIT_ID 0 // named residency sensor id of the LPIT; PACKAGE_RAPL_PERF_STATUS is 1

typedef struct _PHP_HW_PLATFORM_WINDOW
{
    BOOLEAN Planned;
    UCHAR Strikes;
    NTSTATUS Status; // the last call's, for the report
} PHP_HW_PLATFORM_WINDOW, *PPHP_HW_PLATFORM_WINDOW;

static PHP_HW_PLATFORM_WINDOW PhpHwPlatformLpit;
static PHP_HW_PLATFORM_WINDOW PhpHwPlatformPwrm;
static DOUBLE PhpHwPlatformLpitFrequency = 0.0; // counter Hz
static ULONG PhpHwPlatformLpitIndex = ULONG_MAX; // the named residency sensor
static BOOLEAN PhpHwPlatformLpitSampled = FALSE;
static ULONG PhpHwPlatformLpitCount = 0;
static ULONG64 PhpHwPlatformLpitTime = 0;
static ULONG PhpHwPlatformPwrmMinMax[2] = { ULONG_MAX, ULONG_MAX }; // THERMAL_MINTEMP and THERMAL_MAXTEMP at the probe

/**
 * Decodes THERMAL_TSS0 of the PMC: valid when TSRV (bit 9), TSR [8:0] two's complement whole degrees.
 */
_Success_(return)
static BOOLEAN PhpHwPlatformDecodePwrmTemperature(
    _In_ ULONG Value,
    _Out_ PFLOAT Celsius
    )
{
    LONG celsius;

    if (Value == ULONG_MAX || !(Value & 0x200))
        return FALSE;

    celsius = (LONG)(Value & 0x1FF);

    if (celsius & 0x100)
        celsius -= 0x200;
    if (celsius < -40 || celsius > 150)
        return FALSE;

    *Celsius = (FLOAT)celsius;
    return TRUE;
}

/**
 * Reads registers of a platform window and applies the session failure policy to the window: one the row or the
 * machine does not have leaves the plan at once; an asleep or contended window is no value; anything else strikes.
 *
 * \return TRUE when the values carry their own statuses.
 */
static BOOLEAN PhpHwPlatformRead(
    _Inout_ PPHP_HW_PLATFORM_WINDOW Window,
    _In_ ULONG Platform,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    )
{
    NTSTATUS status;

    status = KphHwPlatformRead(Platform, Items, Count, Values);

    // The first item stands for the window.
    if (NT_SUCCESS(status))
        status = Values[0].Status;

    Window->Status = status;

    if (NT_SUCCESS(status))
    {
        Window->Strikes = 0;
        return TRUE;
    }

    switch (status)
    {
    case STATUS_RETRY:
    case STATUS_DEVICE_NOT_READY:
        break;
    case STATUS_NOT_SUPPORTED: // no row, or no window of this kind
    case STATUS_NOT_FOUND: // no LPIT counter
    case STATUS_ACCESS_DENIED:
    case STATUS_DEVICE_CONFIGURATION_ERROR:
        Window->Planned = FALSE;
        break;
    default:
        if (++Window->Strikes >= PHP_HW_PLATFORM_STRIKE_LIMIT)
        {
            Window->Planned = FALSE;
            PhTraceInfo("Hardware sensors: platform window %lu unavailable: 0x%lx", Platform, status);
        }
        break;
    }

    return FALSE;
}

/**
 * Finds the LPIT counter and, without the PCH thermal window, the PWRM temperature. Called once a session.
 *
 * \return STATUS_SUCCESS, or STATUS_NOT_SUPPORTED without an Intel processor or a window.
 */
_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwPlatformProbe(
    VOID
    )
{
    PH_CPU_IDENTITY identity;
    KPH_HW_WINDOW_ITEM items[3];
    KPH_HW_VALUE64 values[3];
    ULONG64 frequency;
    FLOAT celsius;

    if (PhpHwCpuPending())
        return STATUS_RETRY;

    if (PhpHwQueryMachine() == IMAGE_FILE_MACHINE_ARM64 || !PhpHwCpuQueryIntel(&identity) || !PhpHwPackageCount)
        return STATUS_NOT_SUPPORTED;

    memset(&PhpHwPlatformLpit, 0, sizeof(PHP_HW_PLATFORM_WINDOW));
    memset(&PhpHwPlatformPwrm, 0, sizeof(PHP_HW_PLATFORM_WINDOW));

    // A table without a frequency counts at the base clock.
    if (PhpHwQueryLpitFrequency(&frequency))
    {
        PhpHwPlatformLpitFrequency = frequency ? (DOUBLE)frequency : (DOUBLE)PhpHwPackageSensors[0].BaseMhz * 1000000.0;
        PhpHwPlatformLpit.Planned = PhpHwPlatformLpitFrequency > 0.0;
    }

    if (PhpHwPlatformLpit.Planned)
    {
        items[0].Offset = 0;
        items[0].Width = 4;

        if (PhpHwPlatformRead(&PhpHwPlatformLpit, KPH_HW_PLATFORM_LPIT, items, 1, values) || PhpHwPlatformLpit.Planned)
            PhpHwPlatformLpitIndex = PhpHwAddNamedSensor(PhHwNamedSensorPlatformResidency, 0, PHP_HW_PLATFORM_LPIT_ID, L"S0ix (SLP_S0)");
    }

    // The PCH thermal function's window is the chipset temperature's first source.
    if (!PhpHwMmioChipsetPlanned())
    {
        items[0].Offset = PH_INTEL_PWRM_THERMAL_TSS0;
        items[1].Offset = PH_INTEL_PWRM_THERMAL_MINTEMP;
        items[2].Offset = PH_INTEL_PWRM_THERMAL_MAXTEMP;
        items[0].Width = items[1].Width = items[2].Width = 4;
        PhpHwPlatformPwrm.Planned = TRUE;

        if (PhpHwPlatformRead(&PhpHwPlatformPwrm, KPH_HW_PLATFORM_PWRM, items, 3, values))
        {
            if (!PhpHwPlatformDecodePwrmTemperature((ULONG)values[0].Value, &celsius))
                PhpHwPlatformPwrm.Planned = FALSE;

            PhpHwPlatformPwrmMinMax[0] = NT_SUCCESS(values[1].Status) ? (ULONG)values[1].Value : ULONG_MAX;
            PhpHwPlatformPwrmMinMax[1] = NT_SUCCESS(values[2].Status) ? (ULONG)values[2].Value : ULONG_MAX;
        }
    }

    if (!PhpHwPlatformLpit.Planned && !PhpHwPlatformPwrm.Planned)
        return STATUS_NOT_SUPPORTED;

    return STATUS_SUCCESS;
}

/**
 * Reads the LPIT counter and the PWRM temperature that remain, one call each.
 *
 * \return STATUS_RETRY when every call met contention, else STATUS_SUCCESS.
 */
_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwPlatformPass(
    VOID
    )
{
    KPH_HW_WINDOW_ITEM item;
    KPH_HW_VALUE64 value;
    BOOLEAN answered = FALSE;
    BOOLEAN retried = FALSE;
    ULONG capabilities = 0;

    if (PhpHwPlatformLpit.Planned)
    {
        BOOLEAN valid = FALSE;
        FLOAT percent = 0.0f;

        item.Offset = 0;
        item.Width = 4;

        if (PhpHwPlatformRead(&PhpHwPlatformLpit, KPH_HW_PLATFORM_LPIT, &item, 1, &value))
        {
            ULONG count = (ULONG)value.Value;
            ULONG64 now = PhpHwQueryTime();

            if (PhpHwPlatformLpitSampled && now > PhpHwPlatformLpitTime)
            {
                DOUBLE seconds = (DOUBLE)(now - PhpHwPlatformLpitTime) / (DOUBLE)PHP_HW_SECOND;
                DOUBLE residency = (DOUBLE)(ULONG)(count - PhpHwPlatformLpitCount) / PhpHwPlatformLpitFrequency;

                percent = (FLOAT)(residency / seconds * 100.0);

                // An interval longer than one 32-bit wrap of the counter is ambiguous.
                valid = seconds * PhpHwPlatformLpitFrequency < 4294967296.0 && percent >= 0.0f && percent <= 100.5f;
                percent = min(percent, 100.0f);
            }

            PhpHwPlatformLpitCount = count;
            PhpHwPlatformLpitTime = now;
            PhpHwPlatformLpitSampled = TRUE;
            answered = TRUE;
        }
        else
        {
            PhpHwPlatformLpitSampled = FALSE;
            retried |= PhpHwPlatformLpit.Status == STATUS_RETRY;
            answered |= PhpHwPlatformLpit.Status != STATUS_RETRY;
        }

        PhpHwSetNamedSensor(PhpHwPlatformLpitIndex, valid, percent);
    }
    else
    {
        PhpHwSetNamedSensor(PhpHwPlatformLpitIndex, FALSE, 0.0f);
    }

    if (PhpHwPlatformPwrm.Planned)
    {
        FLOAT celsius;

        item.Offset = PH_INTEL_PWRM_THERMAL_TSS0;
        item.Width = 4;

        // TSRV clear is a reading in progress, not a failure.
        if (PhpHwPlatformRead(&PhpHwPlatformPwrm, KPH_HW_PLATFORM_PWRM, &item, 1, &value))
        {
            if (PhpHwPlatformDecodePwrmTemperature((ULONG)value.Value, &celsius))
            {
                PhpHwChipsetTemperature = celsius;
                PhpHwChipsetTemperatureValid = TRUE;
            }
            else
            {
                PhpHwChipsetTemperatureValid = FALSE;
            }

            answered = TRUE;
        }
        else
        {
            PhpHwChipsetTemperatureValid = FALSE;
            retried |= PhpHwPlatformPwrm.Status == STATUS_RETRY;
            answered |= PhpHwPlatformPwrm.Status != STATUS_RETRY;
        }
    }

    if (PhpHwPlatformLpit.Planned && PhpHwPlatformLpitIndex != ULONG_MAX)
        capabilities |= PH_HW_CAP_RESIDENCY;
    if (PhpHwPlatformPwrm.Planned)
        capabilities |= PH_HW_CAP_CHIPSET_TEMP;

    PhpHwFamilies[PhpHwFamilyPlatform].Capabilities = capabilities;

    return retried && !answered ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Ends the session.
 */
_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwPlatformEnd(
    VOID
    )
{
    memset(&PhpHwPlatformLpit, 0, sizeof(PHP_HW_PLATFORM_WINDOW));
    memset(&PhpHwPlatformPwrm, 0, sizeof(PHP_HW_PLATFORM_WINDOW));
    PhpHwPlatformLpitFrequency = 0.0;
    PhpHwPlatformLpitIndex = ULONG_MAX;
    PhpHwPlatformLpitSampled = FALSE;
    PhpHwPlatformLpitCount = 0;
    PhpHwPlatformLpitTime = 0;
    PhpHwPlatformPwrmMinMax[0] = ULONG_MAX;
    PhpHwPlatformPwrmMinMax[1] = ULONG_MAX;
}

/**
 * Writes the collector's section of the hardware report: each window's state and last status.
 *
 * \param Builder The report.
 */
_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwPlatformDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PCWSTR lpit = L"not read";
    PCWSTR pwrm = L"not read";

    if (PhpHwPlatformLpit.Planned)
        lpit = L"read";
    if (PhpHwPlatformPwrm.Planned)
        pwrm = L"read";

    PhAppendFormatStringBuilder(
        Builder,
        L"  LPIT: %s, %.0f Hz, last 0x%08lx\n  PWRM: %s, last 0x%08lx, MINTEMP 0x%08lx, MAXTEMP 0x%08lx\n",
        lpit,
        PhpHwPlatformLpitFrequency,
        PhpHwPlatformLpit.Status,
        pwrm,
        PhpHwPlatformPwrm.Status,
        PhpHwPlatformPwrmMinMax[0],
        PhpHwPlatformPwrmMinMax[1]
        );
}
