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

// Collector mmio: the memory windows of the Intel host bridge (MCHBAR) and the processor thermal device (BAR0), one
// KphHwPciMmioRead per window that has registers left per update. Every register of the table is offered to every
// window; the items' statuses prune it, and a register one window answers is not asked of a later one.

#define PHP_HW_MMIO_INTEL_VENDOR_ID 0x8086
#define PHP_HW_MMIO_HOST_BRIDGE_CLASS 0x0600
#define PHP_HW_MMIO_SIGNAL_PROCESSING_CLASS 0x1180
#define PHP_HW_MMIO_MAX_CANDIDATES 16
#define PHP_HW_MMIO_MAX_WINDOWS (PHP_HW_MMIO_MAX_CANDIDATES * 2)
#define PHP_HW_MMIO_STRIKE_LIMIT 3 // failures of a register before it leaves the window for the session
#define PHP_HW_MMIO_RAPL_LIMITED_ID 1 // named residency sensor id of PACKAGE_RAPL_PERF_STATUS; LPIT S0ix is 0

typedef enum _PHP_HW_MMIO_REG
{
    PhpHwMmioPackageTemperature,
    PhpHwMmioPp0Temperature,
    PhpHwMmioPp1Temperature,
    PhpHwMmioPowerLimit1,
    PhpHwMmioPowerLimit2,
    PhpHwMmioRaplPerfStatus,
    PhpHwMmioPowerUnit, // probe only from here
    PhpHwMmioTemperatureTarget,
    PhpHwMmioMaximum
} PHP_HW_MMIO_REG;

typedef struct _PHP_HW_MMIO_REG_INFO
{
    ULONG Offset;
    ULONG Width;
} PHP_HW_MMIO_REG_INFO;

static CONST PHP_HW_MMIO_REG_INFO PhpHwMmioRegInfo[PhpHwMmioMaximum] =
{
    { PH_INTEL_MCHBAR_PKG_TEMPERATURE, 4 },
    { PH_INTEL_MCHBAR_PP0_TEMPERATURE, 4 },
    { PH_INTEL_MCHBAR_PP1_TEMPERATURE, 4 },
    { PH_INTEL_MCHBAR_PKG_POWER_LIMIT, 4 },
    { PH_INTEL_MCHBAR_PKG_POWER_LIMIT + 4, 4 },
    { PH_INTEL_MCHBAR_RAPL_PERF_STATUS, 4 },
    { PH_INTEL_MCHBAR_PKG_POWER_UNIT, 4 },
    { PH_INTEL_MCHBAR_TEMPERATURE_TARGET, 4 },
};

typedef struct _PHP_HW_MMIO_REGISTER
{
    BOOLEAN Disabled; // not asked of this window
    UCHAR Strikes;
} PHP_HW_MMIO_REGISTER, *PPHP_HW_MMIO_REGISTER;

typedef struct _PHP_HW_MMIO_WINDOW
{
    ULONG Locator;
    ULONG Window; // KPH_HW_WINDOW_MCHBAR or KPH_HW_WINDOW_BAR0
    USHORT Device;
    BOOLEAN Disabled; // the whole window
    ULONG Valid; // 1 << PHP_HW_MMIO_REG answered this pass
    ULONG LastValid;
    ULONG64 Value[PhpHwMmioMaximum];
    ULONG64 LastValue[PhpHwMmioMaximum];
    PHP_HW_MMIO_REGISTER Registers[PhpHwMmioMaximum];
} PHP_HW_MMIO_WINDOW, *PPHP_HW_MMIO_WINDOW;

static PH_CPU_IDENTITY PhpHwMmioIdentity;
static ULONG PhpHwMmioCandidateCount = 0;
static ULONG PhpHwMmioWindowCount = 0;
static PPHP_HW_MMIO_WINDOW PhpHwMmioWindows = NULL; // PHP_HW_MMIO_MAX_WINDOWS
static FLOAT PhpHwMmioUnitWatts = 0.0f; // watts a power limit increment
static FLOAT PhpHwMmioLimit1 = 0.0f; // watts, 0 when unavailable or disabled
static FLOAT PhpHwMmioLimit2 = 0.0f;
static ULONG PhpHwMmioRaplLimitedIndex = ULONG_MAX; // the named residency sensor
static ULONG64 PhpHwMmioSampleTime = 0;
static ULONG64 PhpHwMmioLastSampleTime = 0;

static BOOLEAN PhpHwMmioProbeOnly(
    _In_ PHP_HW_MMIO_REG Reg
    )
{
    return Reg >= PhpHwMmioPowerUnit;
}

/**
 * Applies the session failure policy to a register of a window: a refused one leaves the window at once; an asleep
 * device is no value, not a failure; anything else strikes.
 *
 * \return TRUE when the value is usable.
 */
static BOOLEAN PhpHwMmioApplyStatus(
    _Inout_ PPHP_HW_MMIO_WINDOW Window,
    _In_ PHP_HW_MMIO_REG Reg,
    _In_ NTSTATUS Status
    )
{
    PPHP_HW_MMIO_REGISTER reg = &Window->Registers[Reg];

    if (NT_SUCCESS(Status))
    {
        reg->Strikes = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_RETRY:
    case STATUS_DEVICE_NOT_READY: // asleep, or all ones
    case STATUS_DEVICE_POWERED_OFF:
        break;
    case STATUS_ACCESS_DENIED: // not permitted
        reg->Disabled = TRUE;
        break;
    default:
        if (++reg->Strikes >= PHP_HW_MMIO_STRIKE_LIMIT)
        {
            reg->Disabled = TRUE;
            PhTraceInfo("Hardware sensor window 0x%lx:%lu offset 0x%lx unavailable: 0x%lx", Window->Locator, Window->Window, PhpHwMmioRegInfo[Reg].Offset, Status);
        }
        break;
    }

    return FALSE;
}

/**
 * Reads the registers a window still has, in one call.
 *
 * \param Window The window.
 * \param Probe Read the probe-only registers too.
 * \return The status of the call; STATUS_SUCCESS when the window had nothing to read.
 */
static NTSTATUS PhpHwMmioReadWindow(
    _Inout_ PPHP_HW_MMIO_WINDOW Window,
    _In_ BOOLEAN Probe
    )
{
    KPH_HW_WINDOW_ITEM items[PhpHwMmioMaximum];
    KPH_HW_VALUE64 values[PhpHwMmioMaximum];
    PHP_HW_MMIO_REG regs[PhpHwMmioMaximum];
    ULONG count = 0;
    NTSTATUS status;

    Window->LastValid = Window->Valid;
    memcpy(Window->LastValue, Window->Value, sizeof(Window->Value));
    Window->Valid = 0;

    if (Window->Disabled)
        return STATUS_SUCCESS;

    for (PHP_HW_MMIO_REG reg = 0; reg < PhpHwMmioMaximum; reg++)
    {
        if (Window->Registers[reg].Disabled || (!Probe && PhpHwMmioProbeOnly(reg)))
            continue;

        items[count].Offset = PhpHwMmioRegInfo[reg].Offset;
        items[count].Width = PhpHwMmioRegInfo[reg].Width;
        regs[count] = reg;
        count++;
    }

    if (!count)
        return STATUS_SUCCESS;

    status = KphHwPciMmioRead(Window->Locator, Window->Window, items, count, values);

    // Contention backs the whole family off (D9); a window that is gone, removable or misconfigured leaves the plan.
    if (status == STATUS_RETRY || status == STATUS_DEVICE_NOT_READY || status == STATUS_DEVICE_POWERED_OFF)
        return status;

    if (!NT_SUCCESS(status))
    {
        Window->Disabled = TRUE;
        PhTraceInfo("Hardware sensor window 0x%lx:%lu unavailable: 0x%lx", Window->Locator, Window->Window, status);
        return status;
    }

    for (ULONG i = 0; i < count; i++)
    {
        if (!PhpHwMmioApplyStatus(Window, regs[i], values[i].Status))
            continue;

        Window->Value[regs[i]] = values[i].Value;
        Window->Valid |= 1UL << regs[i];
    }

    return status;
}

/**
 * Retrieves a register from the first window that answered it this pass.
 */
_Success_(return)
static BOOLEAN PhpHwMmioValue(
    _In_ PHP_HW_MMIO_REG Reg,
    _Out_ PULONG Value
    )
{
    for (ULONG i = 0; i < PhpHwMmioWindowCount; i++)
    {
        if (PhpHwMmioWindows[i].Valid & (1UL << Reg))
        {
            *Value = (ULONG)PhpHwMmioWindows[i].Value[Reg];
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Takes a register out of every window.
 */
static VOID PhpHwMmioDisable(
    _In_ PHP_HW_MMIO_REG Reg
    )
{
    for (ULONG i = 0; i < PhpHwMmioWindowCount; i++)
        PhpHwMmioWindows[i].Registers[Reg].Disabled = TRUE;
}

/**
 * Determines whether a window still reads a register.
 */
static BOOLEAN PhpHwMmioActive(
    _In_ PHP_HW_MMIO_REG Reg
    )
{
    for (ULONG i = 0; i < PhpHwMmioWindowCount; i++)
    {
        if (!PhpHwMmioWindows[i].Disabled && !PhpHwMmioWindows[i].Registers[Reg].Disabled)
            return TRUE;
    }

    return FALSE;
}

/**
 * Decodes an MMIO power limit dword (limit [14:0], enable bit 15).
 *
 * \return The limit in watts, or 0 when disabled or the unit is unknown.
 */
static FLOAT PhpHwMmioDecodePowerLimit(
    _In_ PHP_HW_MMIO_REG Reg
    )
{
    FLOAT unit = PhpHwMmioUnitWatts > 0.0f ? PhpHwMmioUnitWatts : PhpHwCpuQueryPowerUnit();
    ULONG value;

    if (unit <= 0.0f || !PhpHwMmioValue(Reg, &value) || !(value & 0x8000))
        return 0.0f;

    return (FLOAT)(value & 0x7FFF) * unit;
}

/**
 * Converts the PACKAGE_RAPL_PERF_STATUS delta (1/1024 s units) to the percent of the interval RAPL limited the package.
 */
_Success_(return)
static BOOLEAN PhpHwMmioDecodeRaplLimited(
    _Out_ PFLOAT Percent
    )
{
    for (ULONG i = 0; i < PhpHwMmioWindowCount; i++)
    {
        PPHP_HW_MMIO_WINDOW window = &PhpHwMmioWindows[i];
        FLOAT seconds;
        FLOAT percent;

        if (!(window->Valid & window->LastValid & (1UL << PhpHwMmioRaplPerfStatus)))
            continue;
        if (!PhpHwMmioLastSampleTime || PhpHwMmioSampleTime <= PhpHwMmioLastSampleTime)
            return FALSE;

        seconds = (FLOAT)(PhpHwMmioSampleTime - PhpHwMmioLastSampleTime) / (FLOAT)PHP_HW_SECOND;
        percent = (FLOAT)(ULONG)(window->Value[PhpHwMmioRaplPerfStatus] - window->LastValue[PhpHwMmioRaplPerfStatus]) / 1024.0f / seconds * 100.0f;

        if (percent > 100.5f)
            return FALSE;

        *Percent = min(percent, 100.0f);
        return TRUE;
    }

    return FALSE;
}

/**
 * Finds the windows of the Intel host bridge and the signal processing functions and reads each register once.
 * Called once a session.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED without an Intel processor or a window, or the status of the identify
 * calls when every one was not shipped.
 */
NTSTATUS PhpHwMmioProbe(
    VOID
    )
{
    ULONG candidates[PHP_HW_MMIO_MAX_CANDIDATES];
    NTSTATUS lastStatus = STATUS_NOT_SUPPORTED;
    ULONG value;
    FLOAT celsius;

    // The family needs the processor identity of cpu: the window decoders depend on the model.
    if (!PhpHwCpuQueryIntel(&PhpHwMmioIdentity) || !PhpHwPackageCount)
        return STATUS_NOT_SUPPORTED;

    // The host bridge first; the processor thermal device mirrors part of its MCHBAR.
    PhpHwMmioCandidateCount = PhpHwEnumeratePciFunctions(
        PHP_HW_MMIO_INTEL_VENDOR_ID,
        PHP_HW_MMIO_INTEL_VENDOR_ID,
        PHP_HW_MMIO_HOST_BRIDGE_CLASS,
        NULL,
        0,
        TRUE,
        candidates,
        1
        );
    PhpHwMmioCandidateCount += PhpHwEnumeratePciFunctions(
        PHP_HW_MMIO_INTEL_VENDOR_ID,
        PHP_HW_MMIO_INTEL_VENDOR_ID,
        PHP_HW_MMIO_SIGNAL_PROCESSING_CLASS,
        NULL,
        0,
        FALSE,
        &candidates[PhpHwMmioCandidateCount],
        PHP_HW_MMIO_MAX_CANDIDATES - PhpHwMmioCandidateCount
        );

    PhpHwMmioWindows = PhAllocateZero(sizeof(PHP_HW_MMIO_WINDOW) * PHP_HW_MMIO_MAX_WINDOWS);
    PhpHwMmioWindowCount = 0;

    for (ULONG i = 0; i < PhpHwMmioCandidateCount; i++)
    {
        KPH_HW_PCI_MMIO device;
        NTSTATUS status;

        status = KphHwPciMmioIdentify(candidates[i], &device);

        if (!NT_SUCCESS(status))
        {
            if (PhpHwStatusNotShipped(status))
                lastStatus = status;

            continue;
        }

        for (ULONG window = KPH_HW_WINDOW_MCHBAR; window <= KPH_HW_WINDOW_BAR0; window++)
        {
            PPHP_HW_MMIO_WINDOW entry;

            if (!(device.Windows & (1UL << (window - 1))))
                continue;

            entry = &PhpHwMmioWindows[PhpHwMmioWindowCount];
            entry->Locator = candidates[i];
            entry->Window = window;
            entry->Device = device.Device;

            // A register an earlier window answered is not asked of this one.
            for (PHP_HW_MMIO_REG reg = 0; reg < PhpHwMmioMaximum; reg++)
            {
                for (ULONG j = 0; j < PhpHwMmioWindowCount; j++)
                {
                    if (PhpHwMmioWindows[j].Valid & (1UL << reg))
                        entry->Registers[reg].Disabled = TRUE;
                }
            }

            PhpHwMmioWindowCount++;
            PhpHwMmioReadWindow(entry, TRUE);
        }
    }

    if (!PhpHwMmioWindowCount)
        return lastStatus;

    // The MMIO package temperature and TjMax stand in for the processor's registers only.
    if (PhpHwCpuThermalPlanned())
    {
        PhpHwMmioDisable(PhpHwMmioPackageTemperature);
    }
    else if (PhpHwMmioValue(PhpHwMmioTemperatureTarget, &value))
    {
        ULONG tjMax = PhIntelTemperatureTargetTjMax(value);

        if (tjMax && PhpHwPackageSensors[0].TjMax <= 0.0f)
            PhpHwPackageSensors[0].TjMax = (FLOAT)tjMax;
    }

    if (!PhpHwMmioValue(PhpHwMmioPackageTemperature, &value) || !PhIntelMmioTemperatureToCelsius(value, &celsius))
        PhpHwMmioDisable(PhpHwMmioPackageTemperature);
    if (!PhpHwMmioValue(PhpHwMmioPp0Temperature, &value) || !PhIntelMmioTemperatureToCelsius(value, &celsius))
        PhpHwMmioDisable(PhpHwMmioPp0Temperature);
    if (!PhpHwMmioValue(PhpHwMmioPp1Temperature, &value) || !PhIntelMmioTemperatureToCelsius(value, &celsius))
        PhpHwMmioDisable(PhpHwMmioPp1Temperature);

    // The power unit is constant; the limits change at runtime (DTT).
    if (PhpHwMmioValue(PhpHwMmioPowerUnit, &value))
        PhpHwMmioUnitWatts = PhRaplPowerUnitWatts(value, &PhpHwMmioIdentity);

    if (PhpHwMmioUnitWatts <= 0.0f && PhpHwCpuQueryPowerUnit() <= 0.0f)
    {
        PhpHwMmioDisable(PhpHwMmioPowerLimit1);
        PhpHwMmioDisable(PhpHwMmioPowerLimit2);
    }

    if (PhpHwMmioActive(PhpHwMmioRaplPerfStatus))
        PhpHwMmioRaplLimitedIndex = PhpHwAddNamedSensor(PhHwNamedSensorPlatformResidency, 0, PHP_HW_MMIO_RAPL_LIMITED_ID, L"Power limited (RAPL)");

    // Deltas start at the first pass.
    for (ULONG i = 0; i < PhpHwMmioWindowCount; i++)
        PhpHwMmioWindows[i].Valid = 0;

    return STATUS_SUCCESS;
}

/**
 * Reads every window that has registers left and publishes package 0's MMIO values.
 *
 * \return STATUS_RETRY when every call met contention, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwMmioPass(
    VOID
    )
{
    PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[0];
    BOOLEAN answered = FALSE;
    BOOLEAN retried = FALSE;
    ULONG owned = 0;
    ULONG valid = 0;
    ULONG value;
    FLOAT celsius;
    FLOAT percent = 0.0f;

    for (ULONG i = 0; i < PhpHwMmioWindowCount; i++)
    {
        NTSTATUS status = PhpHwMmioReadWindow(&PhpHwMmioWindows[i], FALSE);

        if (status == STATUS_RETRY)
            retried = TRUE;
        else
            answered = TRUE;
    }

    PhpHwMmioLastSampleTime = PhpHwMmioSampleTime;
    PhpHwMmioSampleTime = PhpHwQueryTime();

    if (PhpHwMmioActive(PhpHwMmioPackageTemperature))
        owned |= PH_HW_CAP_PACKAGE_TEMP;
    if (PhpHwMmioActive(PhpHwMmioPp0Temperature))
        owned |= PH_HW_CAP_CORES_TEMP;
    if (PhpHwMmioActive(PhpHwMmioPp1Temperature))
        owned |= PH_HW_CAP_GRAPHICS_TEMP;

    if (PhpHwMmioValue(PhpHwMmioPackageTemperature, &value) && PhIntelMmioTemperatureToCelsius(value, &celsius))
    {
        sensor->Temperature = celsius;
        valid |= PH_HW_CAP_PACKAGE_TEMP;
    }

    if (PhpHwMmioValue(PhpHwMmioPp0Temperature, &value) && PhIntelMmioTemperatureToCelsius(value, &celsius))
    {
        sensor->CoresTemperature = celsius;
        valid |= PH_HW_CAP_CORES_TEMP;
    }

    if (PhpHwMmioValue(PhpHwMmioPp1Temperature, &value) && PhIntelMmioTemperatureToCelsius(value, &celsius))
    {
        sensor->GraphicsTemperature = celsius;
        valid |= PH_HW_CAP_GRAPHICS_TEMP;
    }

    // cpu publishes the active limits, the lower of these and the MSR's.
    PhpHwMmioLimit1 = PhpHwMmioDecodePowerLimit(PhpHwMmioPowerLimit1);
    PhpHwMmioLimit2 = PhpHwMmioDecodePowerLimit(PhpHwMmioPowerLimit2);

    if (PhpHwMmioRaplLimitedIndex != ULONG_MAX)
    {
        BOOLEAN limited = PhpHwMmioDecodeRaplLimited(&percent);

        PhpHwSetNamedSensor(PhpHwMmioRaplLimitedIndex, limited, percent);
    }

    PhpHwFamilies[PhpHwFamilyMmio].Capabilities = (PhpHwMmioRaplLimitedIndex != ULONG_MAX && PhpHwMmioActive(PhpHwMmioRaplPerfStatus)) ?
        PH_HW_CAP_RESIDENCY : 0;

    // Only the bits this collector owns are replaced.
    WriteULongRelease(&sensor->Valid, (ReadULongAcquire(&sensor->Valid) & ~owned) | (valid & owned));

    return retried && !answered ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Retrieves an MMIO power limit of a package for cpu, which publishes the lower of it and the MSR's.
 *
 * \param Package The package.
 * \param Second PL2 instead of PL1.
 * \return The limit in watts, or 0 when there is none.
 */
FLOAT PhpHwMmioPowerLimit(
    _In_ ULONG Package,
    _In_ BOOLEAN Second
    )
{
    if (Package != 0)
        return 0.0f;

    return Second ? PhpHwMmioLimit2 : PhpHwMmioLimit1;
}

/**
 * Ends the session: the windows are found again at the next probe.
 */
VOID PhpHwMmioEnd(
    VOID
    )
{
    if (PhpHwMmioWindows)
    {
        PhFree(PhpHwMmioWindows);
        PhpHwMmioWindows = NULL;
    }

    PhpHwMmioCandidateCount = 0;
    PhpHwMmioWindowCount = 0;
    PhpHwMmioUnitWatts = 0.0f;
    PhpHwMmioLimit1 = 0.0f;
    PhpHwMmioLimit2 = 0.0f;
    PhpHwMmioRaplLimitedIndex = ULONG_MAX;
    PhpHwMmioSampleTime = 0;
    PhpHwMmioLastSampleTime = 0;
}

/**
 * Writes the collector's section of the hardware report: the candidates, the windows and each register's state.
 *
 * \param Builder The report.
 */
VOID PhpHwMmioDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PhAppendFormatStringBuilder(Builder, L"  Candidates: %lu; windows: %lu; power unit %g W\n", PhpHwMmioCandidateCount, PhpHwMmioWindowCount, PhpHwMmioUnitWatts);

    for (ULONG i = 0; i < PhpHwMmioWindowCount; i++)
    {
        PPHP_HW_MMIO_WINDOW window = &PhpHwMmioWindows[i];

        PhAppendFormatStringBuilder(
            Builder,
            L"  Window %s of 0x%06lx (%04x): %s\n",
            window->Window == KPH_HW_WINDOW_MCHBAR ? L"MCHBAR" : L"BAR0",
            window->Locator,
            window->Device,
            window->Disabled ? L"left out" : L"read"
            );

        for (PHP_HW_MMIO_REG reg = 0; reg < PhpHwMmioMaximum; reg++)
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"    Offset 0x%05lx: %s\n",
                PhpHwMmioRegInfo[reg].Offset,
                window->Registers[reg].Disabled ? L"left out" : PhpHwMmioProbeOnly(reg) ? L"probe" : L"read"
                );
        }
    }
}
