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
// window; the items' statuses prune it, and a register one window answers is not asked of a later one. A server's
// memory controller functions (one a package, in bus order) add their controllers' free-running read and write
// counters, each package's own bandwidth.

#define PHP_HW_MMIO_INTEL_VENDOR_ID 0x8086
#define PHP_HW_MMIO_HOST_BRIDGE_CLASS 0x0600
#define PHP_HW_MMIO_SIGNAL_PROCESSING_CLASS 0x1180
#define PHP_HW_MMIO_MAX_CANDIDATES 16
#define PHP_HW_MMIO_MAX_WINDOWS (PHP_HW_MMIO_MAX_CANDIDATES * 2)
#define PHP_HW_MMIO_STRIKE_LIMIT 3 // failures of a register before it leaves the window for the session
#define PHP_HW_MMIO_MC_DEVICE_ID 0x3451 // Ice Lake-SP/D memory controller function
#define PHP_HW_MMIO_MC_FIRST_WINDOW 3 // the pcimmio window of controller 0
#define PHP_HW_MMIO_MC_CONTROLLERS 4
#define PHP_HW_MMIO_MC_READS 0x2290 // 48-bit free-running counters, 64 bytes a count
#define PHP_HW_MMIO_MC_WRITES 0x2298
#define PHP_HW_MMIO_MC_COUNTER_MASK 0xFFFFFFFFFFFFULL
#define PHP_HW_MMIO_RAPL_LIMITED_ID 1 // named residency sensor id of PACKAGE_RAPL_PERF_STATUS; LPIT S0ix is 0
#define PHP_HW_MMIO_GT_CLOCK_ID 0x47540000 // named sensor id of the MCHBAR graphics clock
#define PHP_HW_MMIO_IMC_COUNT_BYTES 64 // bytes per IMC data read/write count
#define PHP_HW_MMIO_IMC_BYTES_PER_TRANSFER_BOUND 32 // twice the 16 bytes per transfer of two 64-bit client channels
#define PHP_HW_MMIO_IMC_DATA_RATE_BOUND 12000.0f // MT/s when the data rate is unknown
#define PHP_HW_MMIO_IMC_DATA_RATE_BOUND_32 4800.0f // MT/s when unknown, parts with 32-bit counters (DDR3/DDR4)
#define PHP_HW_MMIO_IMC_MAX_INTERVAL 10.0f // seconds; a longer interval is a gap

typedef enum _PHP_HW_MMIO_REG
{
    PhpHwMmioPackageTemperature,
    PhpHwMmioPp0Temperature,
    PhpHwMmioPp1Temperature,
    PhpHwMmioPowerLimit1,
    PhpHwMmioPowerLimit2,
    PhpHwMmioRaplPerfStatus,
    PhpHwMmioMcBiosReq,
    PhpHwMmioMcBiosData,
    PhpHwMmioSaPerfStatus,
    PhpHwMmioBclkFreq,
    PhpHwMmioMemssBiosData,
    PhpHwMmioImcReads32,
    PhpHwMmioImcWrites32,
    PhpHwMmioImc0Reads,
    PhpHwMmioImc0Writes,
    PhpHwMmioImc0ReadsAlt,
    PhpHwMmioImc0WritesAlt,
    PhpHwMmioImc1Reads,
    PhpHwMmioImc1Writes,
    PhpHwMmioImc1ReadsAlt,
    PhpHwMmioImc1WritesAlt,
    PhpHwMmioGtPerfStatus,
    PhpHwMmioGtPerfStatusLp, // Apollo Lake, Gemini Lake: GT_PERF_STATUS at 7070h, in place of PhpHwMmioGtPerfStatus
    PhpHwMmioPchTemp, // the PCH thermal function's TEMP
    PhpHwMmioPowerUnit, // probe only from here
    PhpHwMmioTemperatureTarget,
    PhpHwMmioRpStateCap,
    PhpHwMmioFreqInfo,
    PhpHwMmioPchTsel, // the PCH thermal function's TSEL
    PhpHwMmioMaximum
} PHP_HW_MMIO_REG;

C_ASSERT(PhpHwMmioMaximum <= 32);

typedef struct _PHP_HW_MMIO_REG_INFO
{
    ULONG Offset;
    ULONG Width;
    CHAR Imc; // 'R' data reads, 'W' data writes, 0 otherwise
} PHP_HW_MMIO_REG_INFO;

static CONST PHP_HW_MMIO_REG_INFO PhpHwMmioRegInfo[PhpHwMmioMaximum] =
{
    { PH_INTEL_MCHBAR_PKG_TEMPERATURE, 4, 0 },
    { PH_INTEL_MCHBAR_PP0_TEMPERATURE, 4, 0 },
    { PH_INTEL_MCHBAR_PP1_TEMPERATURE, 4, 0 },
    { PH_INTEL_MCHBAR_PKG_POWER_LIMIT, 4, 0 },
    { PH_INTEL_MCHBAR_PKG_POWER_LIMIT + 4, 4, 0 },
    { PH_INTEL_MCHBAR_RAPL_PERF_STATUS, 4, 0 },
    { PH_INTEL_MCHBAR_MC_BIOS_REQ, 4, 0 },
    { PH_INTEL_MCHBAR_MC_BIOS_DATA, 4, 0 },
    { PH_INTEL_MCHBAR_SA_PERF_STATUS, 4, 0 },
    { PH_INTEL_MCHBAR_BCLK_FREQ, 4, 0 },
    { PH_INTEL_MCHBAR_MEMSS_BIOS_DATA, 4, 0 },
    { PH_INTEL_MCHBAR_IMC_DATA_READS, 4, 'R' },
    { PH_INTEL_MCHBAR_IMC_DATA_WRITES, 4, 'W' },
    { PH_INTEL_MCHBAR_IMC_READS, 8, 'R' },
    { PH_INTEL_MCHBAR_IMC_WRITES, 8, 'W' },
    { PH_INTEL_MCHBAR_IMC_READS_ALT, 8, 'R' },
    { PH_INTEL_MCHBAR_IMC_WRITES_ALT, 8, 'W' },
    { PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_IMC_READS, 8, 'R' },
    { PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_IMC_WRITES, 8, 'W' },
    { PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_IMC_READS_ALT, 8, 'R' },
    { PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_IMC_WRITES_ALT, 8, 'W' },
    { PH_INTEL_MCHBAR_GT_PERF_STATUS, 4, 0 },
    { PH_INTEL_MCHBAR_GT_PERF_STATUS_LP, 4, 0 },
    { PH_INTEL_PCH_THERMAL_TEMP, 2, 0 },
    { PH_INTEL_MCHBAR_PKG_POWER_UNIT, 4, 0 },
    { PH_INTEL_MCHBAR_TEMPERATURE_TARGET, 4, 0 },
    { PH_INTEL_MCHBAR_RP_STATE_CAP, 4, 0 },
    { PH_INTEL_MCHBAR_FREQ_INFO_REC, 4, 0 },
    { PH_INTEL_PCH_THERMAL_TSEL, 1, 0 },
};

// MCHBAR memory controller register layouts.
typedef enum _PHP_HW_MMIO_TIMINGS_FORMAT
{
    PhpHwMmioTimingsNone,
    PhpHwMmioTimingsSnb, // Sandy Bridge, Ivy Bridge
    PhpHwMmioTimingsHsw, // Haswell, Broadwell
    PhpHwMmioTimingsSkl, // Skylake through Comet Lake
    PhpHwMmioTimingsIcl, // Ice Lake, Rocket Lake
    PhpHwMmioTimingsTglU, // Tiger Lake U
    PhpHwMmioTimingsTglH, // Tiger Lake H
    PhpHwMmioTimingsAdl, // Alder Lake, Raptor Lake
    PhpHwMmioTimingsMtl, // Meteor, Arrow, Panther and Wildcat Lake
    PhpHwMmioTimingsLnl // Lunar Lake: the Meteor Lake layout without SC_GS_CFG
} PHP_HW_MMIO_TIMINGS_FORMAT;

#define PHP_HW_MMIO_TIMINGS_REGISTERS 5 // TC_PRE, SC_GS_CFG, TC_RFTP, TC_ODT, TC_ACT
#define PHP_HW_MMIO_TIMINGS_CHANNELS 4
#define PHP_HW_MMIO_CONFIG_REGISTERS (2 + 2 * 3 + 3 + PH_INTEL_MCHBAR_DUNITS)

typedef struct _PHP_HW_MMIO_MEMORY_CONFIG
{
    ULONG Valid; // bit 0 MemConfig, bits 1-6 Mad, bits 7-10 Drp0
    ULONG MemConfig; // MEMSS_PMA_CR_MEM_CONFIG
    ULONG Mad[2][3]; // per IMC: MAD_INTER_CHANNEL, MAD_DIMM_CH0, MAD_DIMM_CH1
    ULONG Drp0[PH_INTEL_MCHBAR_DUNITS]; // Apollo Lake, Gemini Lake
} PHP_HW_MMIO_MEMORY_CONFIG;

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

// A server memory controller function: its controllers' counters of the last two passes.
typedef struct _PHP_HW_MMIO_MC
{
    ULONG Locator;
    ULONG Windows; // bit per controller the function has
    ULONG Controllers; // bit per controller still read
    ULONG Valid; // bit per controller read this pass
    ULONG LastValid;
    UCHAR Strikes[PHP_HW_MMIO_MC_CONTROLLERS];
    ULONG64 Reads[PHP_HW_MMIO_MC_CONTROLLERS];
    ULONG64 Writes[PHP_HW_MMIO_MC_CONTROLLERS];
    ULONG64 LastReads[PHP_HW_MMIO_MC_CONTROLLERS];
    ULONG64 LastWrites[PHP_HW_MMIO_MC_CONTROLLERS];
} PHP_HW_MMIO_MC, *PPHP_HW_MMIO_MC;

static PH_CPU_IDENTITY PhpHwMmioIdentity;
static PPHP_HW_MMIO_MC PhpHwMmioMcs = NULL; // one a package, or none
static ULONG PhpHwMmioMcCount = 0;
static ULONG PhpHwMmioCandidateCount = 0;
static ULONG PhpHwMmioWindowCount = 0;
static PPHP_HW_MMIO_WINDOW PhpHwMmioWindows = NULL; // PHP_HW_MMIO_MAX_WINDOWS
static FLOAT PhpHwMmioUnitWatts = 0.0f; // watts a power limit increment
static FLOAT PhpHwMmioLimit1 = 0.0f; // watts, 0 when unavailable or disabled
static FLOAT PhpHwMmioLimit2 = 0.0f;
static ULONG PhpHwMmioRaplLimitedIndex = ULONG_MAX; // the named residency sensor
static ULONG PhpHwMmioGtIndex = ULONG_MAX; // the named graphics clock
static ULONG PhpHwMmioSmbiosDataRate = 0; // the SMBIOS configured speed, MT/s; 0 when unknown
static ULONG PhpHwMmioGtCaps = 0; // RP_STATE_CAP at the probe, for the report
static ULONG PhpHwMmioGtRpe = 0; // FREQ_INFO_REC at the probe, for the report
static ULONG PhpHwMmioGtPerfStatusLast = 0; // GT_PERF_STATUS, for the report
static ULONG PhpHwMmioGtPerfStatusPeak = 0; // the GT_PERF_STATUS with the highest decoded clock, for the report
static BOOLEAN PhpHwMmioDataRateRejected = FALSE; // the decoded data rate disagreed with SMBIOS
static BOOLEAN PhpHwMmioHostBridge = FALSE; // the first candidate is the host bridge
static PHP_HW_MMIO_TIMINGS_FORMAT PhpHwMmioTimingsFormat = PhpHwMmioTimingsNone;
static BOOLEAN PhpHwMmioTimingsValid = FALSE;
static PH_HW_MEMORY_TIMINGS PhpHwMmioTimings;
static PHP_HW_MMIO_MEMORY_CONFIG PhpHwMmioMemoryConfig;
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
 * Decodes the memory clock of the MCHBAR clock registers; a data rate within 7% of half or twice the SMBIOS configured
 * speed is a wrong unit and is rejected.
 */
_Success_(return)
static BOOLEAN PhpHwMmioDecodeMemoryClock(
    _Out_ PFLOAT MemoryClockMhz,
    _Out_ PFLOAT DataRate,
    _Out_ PULONG Gear
    )
{
    ULONG req;
    ULONG data;
    ULONG saPerfStatus;
    ULONG bclk;
    ULONG memss;
    BOOLEAN reqValid = PhpHwMmioValue(PhpHwMmioMcBiosReq, &req);
    BOOLEAN dataValid = PhpHwMmioValue(PhpHwMmioMcBiosData, &data);
    BOOLEAN saPerfStatusValid = PhpHwMmioValue(PhpHwMmioSaPerfStatus, &saPerfStatus);
    BOOLEAN bclkValid = PhpHwMmioValue(PhpHwMmioBclkFreq, &bclk);
    BOOLEAN memssValid = PhpHwMmioValue(PhpHwMmioMemssBiosData, &memss);
    FLOAT mhz;
    FLOAT dataRate;
    ULONG gear;

    if (!PhIntelMchbarToMemoryClock(
        &PhpHwMmioIdentity,
        reqValid ? &req : NULL,
        dataValid ? &data : NULL,
        saPerfStatusValid ? &saPerfStatus : NULL,
        bclkValid ? &bclk : NULL,
        memssValid ? &memss : NULL,
        &mhz,
        &dataRate,
        &gear
        ))
    {
        return FALSE;
    }

    if (PhpHwMmioSmbiosDataRate)
    {
        FLOAT ratio = dataRate / (FLOAT)PhpHwMmioSmbiosDataRate;

        if ((ratio >= 0.5f * 0.93f && ratio <= 0.5f * 1.07f) || (ratio >= 2.0f * 0.93f && ratio <= 2.0f * 1.07f))
        {
            if (!PhpHwMmioDataRateRejected)
                PhTraceWarning("Hardware sensors: memory data rate %lu MT/s disagrees with SMBIOS %lu MT/s", (ULONG)dataRate, PhpHwMmioSmbiosDataRate);

            PhpHwMmioDataRateRejected = TRUE;
            return FALSE;
        }
    }

    *MemoryClockMhz = mhz;
    *DataRate = dataRate;
    *Gear = gear;
    return TRUE;
}

/**
 * Converts the IMC data read and write count deltas to MB/s (10^6 bytes).
 *
 * \param DataRate The data rate in MT/s, or 0 when unknown.
 * \return FALSE for the pass when an interval or a counter delta is implausible.
 */
_Success_(return)
static BOOLEAN PhpHwMmioDecodeBandwidth(
    _In_ FLOAT DataRate,
    _Out_ PFLOAT ReadBandwidth,
    _Out_ PFLOAT WriteBandwidth
    )
{
    FLOAT readBandwidth = 0.0f;
    FLOAT writeBandwidth = 0.0f;
    BOOLEAN valid = FALSE;
    FLOAT seconds;

    if (!PhpHwMmioLastSampleTime || PhpHwMmioSampleTime <= PhpHwMmioLastSampleTime)
        return FALSE;

    seconds = (FLOAT)(PhpHwMmioSampleTime - PhpHwMmioLastSampleTime) / (FLOAT)PHP_HW_SECOND;

    // The first sample after a gap is dropped.
    if (seconds > PHP_HW_MMIO_IMC_MAX_INTERVAL)
        return FALSE;

    for (ULONG i = 0; i < PhpHwMmioWindowCount; i++)
    {
        PPHP_HW_MMIO_WINDOW window = &PhpHwMmioWindows[i];
        ULONG64 reads = 0;
        ULONG64 writes = 0;
        BOOLEAN counted = FALSE;

        for (PHP_HW_MMIO_REG reg = 0; reg < PhpHwMmioMaximum; reg++)
        {
            FLOAT ceiling;
            ULONG64 delta;

            if (!PhpHwMmioRegInfo[reg].Imc || !(window->Valid & window->LastValid & (1UL << reg)))
                continue;

            delta = window->Value[reg] - window->LastValue[reg];

            if (PhpHwMmioRegInfo[reg].Width == 4)
            {
                // 32-bit counters are DDR3 and DDR4 parts; one that could wrap twice in the interval is no value.
                ceiling = (DataRate > 0.0f ? DataRate : PHP_HW_MMIO_IMC_DATA_RATE_BOUND_32) * 1000000.0f * PHP_HW_MMIO_IMC_BYTES_PER_TRANSFER_BOUND;

                if (seconds >= 4294967296.0f * PHP_HW_MMIO_IMC_COUNT_BYTES / ceiling)
                    return FALSE;

                delta = (ULONG)delta;
            }
            else
            {
                ceiling = (DataRate > 0.0f ? DataRate : PHP_HW_MMIO_IMC_DATA_RATE_BOUND) * 1000000.0f * PHP_HW_MMIO_IMC_BYTES_PER_TRANSFER_BOUND;
            }

            if ((FLOAT)delta * PHP_HW_MMIO_IMC_COUNT_BYTES / seconds > ceiling)
                return FALSE;

            if (PhpHwMmioRegInfo[reg].Imc == 'R')
                reads += delta;
            else
                writes += delta;

            counted = TRUE;
        }

        if (!counted)
            continue;

        readBandwidth += (FLOAT)reads * PHP_HW_MMIO_IMC_COUNT_BYTES / seconds / 1000000.0f;
        writeBandwidth += (FLOAT)writes * PHP_HW_MMIO_IMC_COUNT_BYTES / seconds / 1000000.0f;
        valid = TRUE;
    }

    if (!valid)
        return FALSE;

    *ReadBandwidth = readBandwidth;
    *WriteBandwidth = writeBandwidth;
    return TRUE;
}

/**
 * Finds the memory controller functions: kept only when there is one a package and each has a controller window.
 */
static VOID PhpHwMmioProbeMcs(
    VOID
    )
{
    static CONST USHORT deviceIds[] = { PHP_HW_MMIO_MC_DEVICE_ID };
    ULONG locators[PHP_HW_MMIO_MAX_CANDIDATES];
    ULONG count;

    count = PhpHwEnumeratePciFunctions(
        PHP_HW_MMIO_INTEL_VENDOR_ID,
        PHP_HW_MMIO_INTEL_VENDOR_ID,
        PHP_HW_ANY_CLASS_CODE,
        deviceIds,
        RTL_NUMBER_OF(deviceIds),
        FALSE,
        locators,
        RTL_NUMBER_OF(locators)
        );

    if (count == 0 || count != PhpHwPackageCount)
        return;

    // Bus order is package order.
    for (ULONG i = 1; i < count; i++)
    {
        for (ULONG j = i; j > 0 && locators[j - 1] > locators[j]; j--)
        {
            ULONG locator = locators[j];

            locators[j] = locators[j - 1];
            locators[j - 1] = locator;
        }
    }

    PhpHwMmioMcs = PhAllocateZero(sizeof(PHP_HW_MMIO_MC) * count);

    for (ULONG i = 0; i < count; i++)
    {
        KPH_HW_PCI_MMIO device;

        if (!NT_SUCCESS(KphHwPciMmioIdentify(locators[i], &device)))
            break;

        PhpHwMmioMcs[i].Locator = locators[i];
        PhpHwMmioMcs[i].Windows = (device.Windows >> (PHP_HW_MMIO_MC_FIRST_WINDOW - 1)) & ((1UL << PHP_HW_MMIO_MC_CONTROLLERS) - 1);
        PhpHwMmioMcs[i].Controllers = PhpHwMmioMcs[i].Windows;

        if (!PhpHwMmioMcs[i].Controllers)
            break;

        PhpHwMmioMcCount++;
    }

    if (PhpHwMmioMcCount != count)
    {
        PhFree(PhpHwMmioMcs);
        PhpHwMmioMcs = NULL;
        PhpHwMmioMcCount = 0;
    }
}

/**
 * Reads each kept controller's read and write counters, one call a controller.
 *
 * \return TRUE when a call met contention.
 */
static BOOLEAN PhpHwMmioReadMcs(
    VOID
    )
{
    static CONST KPH_HW_WINDOW_ITEM items[2] = { { PHP_HW_MMIO_MC_READS, 8 }, { PHP_HW_MMIO_MC_WRITES, 8 } };
    BOOLEAN retried = FALSE;

    for (ULONG i = 0; i < PhpHwMmioMcCount; i++)
    {
        PPHP_HW_MMIO_MC mc = &PhpHwMmioMcs[i];

        mc->LastValid = mc->Valid;
        mc->Valid = 0;
        memcpy(mc->LastReads, mc->Reads, sizeof(mc->Reads));
        memcpy(mc->LastWrites, mc->Writes, sizeof(mc->Writes));

        for (ULONG c = 0; c < PHP_HW_MMIO_MC_CONTROLLERS; c++)
        {
            KPH_HW_VALUE64 values[2];
            NTSTATUS status;

            if (!(mc->Controllers & (1UL << c)))
                continue;

            status = KphHwPciMmioRead(mc->Locator, PHP_HW_MMIO_MC_FIRST_WINDOW + c, items, 2, values);

            if (NT_SUCCESS(status) && NT_SUCCESS(values[0].Status) && NT_SUCCESS(values[1].Status))
            {
                mc->Strikes[c] = 0;
                mc->Reads[c] = values[0].Value & PHP_HW_MMIO_MC_COUNTER_MASK;
                mc->Writes[c] = values[1].Value & PHP_HW_MMIO_MC_COUNTER_MASK;
                mc->Valid |= 1UL << c;
                continue;
            }

            if (NT_SUCCESS(status))
                status = NT_SUCCESS(values[0].Status) ? values[1].Status : values[0].Status;

            if (status == STATUS_RETRY)
            {
                retried = TRUE;
            }
            else if (status == STATUS_ACCESS_DENIED || ++mc->Strikes[c] >= PHP_HW_MMIO_STRIKE_LIMIT)
            {
                mc->Controllers &= ~(1UL << c);
                PhTraceInfo("Hardware sensor memory controller 0x%lx:%lu unavailable: 0x%lx", mc->Locator, c, status);
            }
        }
    }

    return retried;
}

/**
 * Publishes each package's bandwidth from its controllers read both this pass and the last.
 *
 * \param Seconds The time between the passes.
 */
static VOID PhpHwMmioPublishMcs(
    _In_ FLOAT Seconds
    )
{
    for (ULONG i = 0; i < PhpHwMmioMcCount; i++)
    {
        PPHP_HW_MMIO_MC mc = &PhpHwMmioMcs[i];
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];
        ULONG both = mc->Valid & mc->LastValid & mc->Controllers;
        ULONG64 reads = 0;
        ULONG64 writes = 0;
        ULONG valid = 0;

        for (ULONG c = 0; c < PHP_HW_MMIO_MC_CONTROLLERS; c++)
        {
            if (!(both & (1UL << c)))
                continue;

            reads += (mc->Reads[c] - mc->LastReads[c]) & PHP_HW_MMIO_MC_COUNTER_MASK;
            writes += (mc->Writes[c] - mc->LastWrites[c]) & PHP_HW_MMIO_MC_COUNTER_MASK;
        }

        // Every controller the function has, or no value: a partial sum is not the package's bandwidth.
        if (both && both == mc->Windows && Seconds > 0.0f && Seconds <= PHP_HW_MMIO_IMC_MAX_INTERVAL)
        {
            sensor->MemoryReadBandwidth = (FLOAT)reads * PHP_HW_MMIO_IMC_COUNT_BYTES / Seconds / 1000000.0f;
            sensor->MemoryWriteBandwidth = (FLOAT)writes * PHP_HW_MMIO_IMC_COUNT_BYTES / Seconds / 1000000.0f;
            valid = PH_HW_CAP_MEMORY_BANDWIDTH;
        }

        WriteULongRelease(&sensor->Valid, (ReadULongAcquire(&sensor->Valid) & ~PH_HW_CAP_MEMORY_BANDWIDTH) | valid);
    }
}

/**
 * Decodes the current render clock of GT_PERF_STATUS: [15:8] in 50 MHz steps before Skylake, unslice ratio [16:8]
 * from Skylake, unslice ratio [19:11] from Ice Lake, both in 50/3 MHz steps. A GT in RC6 reads 0.
 */
static FLOAT PhpHwMmioDecodeGtClock(
    _In_ ULONG GtPerfStatus
    )
{
    static CONST UCHAR legacyModels[] = { 0x2A, 0x3A, 0x3C, 0x45, 0x46, 0x3D, 0x47 }; // SNB..BDW client, the rows that read it
    static CONST UCHAR gen11Models[] = { 0x7D, 0x7E, 0x9D, 0x8C, 0x8D, 0xA7, 0xD7, 0x97, 0x9A, 0xB7, 0xBA, 0xBF, 0xBE, 0x96, 0x9C }; // ICL..RPL, ADL-N, Elkhart and Jasper Lake

    if (PhpHwMmioIdentity.DisplayFamily == 0x06)
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(legacyModels); i++)
        {
            if (PhpHwMmioIdentity.DisplayModel == legacyModels[i])
                return (FLOAT)((GtPerfStatus >> 8) & 0xFF) * 50.0f;
        }

        for (ULONG i = 0; i < RTL_NUMBER_OF(gen11Models); i++)
        {
            if (PhpHwMmioIdentity.DisplayModel == gen11Models[i])
                return (FLOAT)((GtPerfStatus >> 11) & 0x1FF) * 50.0f / 3.0f;
        }
    }

    return (FLOAT)((GtPerfStatus >> 8) & 0x1FF) * 50.0f / 3.0f;
}

/**
 * The GT_PERF_STATUS of the part: the client register, else the Apollo and Gemini Lake one at 7070h.
 */
static PHP_HW_MMIO_REG PhpHwMmioGtPerfStatusRegister(
    VOID
    )
{
    return PhpHwMmioActive(PhpHwMmioGtPerfStatus) ? PhpHwMmioGtPerfStatus : PhpHwMmioGtPerfStatusLp;
}

/**
 * Determines the memory controller register layout of the processor model.
 */
static PHP_HW_MMIO_TIMINGS_FORMAT PhpHwMmioQueryTimingsFormat(
    VOID
    )
{
    if (PhpHwMmioIdentity.DisplayFamily != 0x06)
        return PhpHwMmioTimingsNone;

    switch (PhpHwMmioIdentity.DisplayModel)
    {
    case 0x2a:
    case 0x3a:
        return PhpHwMmioTimingsSnb;
    case 0x3c:
    case 0x45:
    case 0x46:
    case 0x3d:
    case 0x47:
        return PhpHwMmioTimingsHsw;
    case 0x4e:
    case 0x5e:
    case 0x8e:
    case 0x9e:
    case 0xa5:
    case 0xa6:
        return PhpHwMmioTimingsSkl;
    case 0x7d:
    case 0x7e:
    case 0xa7:
        return PhpHwMmioTimingsIcl;
    case 0x8c:
        return PhpHwMmioTimingsTglU;
    case 0x8d:
        return PhpHwMmioTimingsTglH;
    case 0x97:
    case 0x9a:
    case 0xb7:
    case 0xba:
    case 0xbf:
    case 0xbe:
    case 0xd7:
        return PhpHwMmioTimingsAdl;
    case 0xaa:
    case 0xac:
    case 0xb5:
    case 0xc5:
    case 0xc6:
    case 0xcc:
    case 0xd5:
        return PhpHwMmioTimingsMtl;
    case 0xbd:
        return PhpHwMmioTimingsLnl;
    }

    return PhpHwMmioTimingsNone;
}

/**
 * Decodes the timings of one channel; Registers holds TC_PRE, SC_GS_CFG, TC_RFTP, TC_ODT and TC_ACT in the layout of
 * the format (unused entries are zero).
 *
 * \return FALSE when the values are implausible, as on an unpopulated channel.
 */
_Success_(return)
static BOOLEAN PhpHwMmioDecodeTimings(
    _In_ PHP_HW_MMIO_TIMINGS_FORMAT Format,
    _In_reads_(PHP_HW_MMIO_TIMINGS_REGISTERS) const ULONG64* Registers,
    _Out_ PPH_HW_MEMORY_TIMINGS Timings
    )
{
    ULONG64 pre = Registers[0];
    ULONG64 config = Registers[1];
    ULONG64 refresh = Registers[2];
    ULONG64 odt = Registers[3];
    ULONG64 act = Registers[4];
    PH_HW_MEMORY_TIMINGS timings;
    ULONG stretch;

    memset(&timings, 0, sizeof(PH_HW_MEMORY_TIMINGS));

    switch (Format)
    {
    case PhpHwMmioTimingsSnb:
        timings.RasToCasDelay = (ULONG)(pre & 0xF);
        timings.RowPrecharge = (ULONG)((pre >> 4) & 0xF);
        timings.CasLatency = (ULONG)((pre >> 8) & 0xF);
        timings.RowActive = (ULONG)((pre >> 16) & 0xFF);
        timings.RefreshCycle = (ULONG)((refresh >> 16) & 0x1FF);
        stretch = (ULONG)((config >> 30) & 0x3);
        timings.CommandRate = stretch <= 1 ? 1 : stretch;
        break;
    case PhpHwMmioTimingsHsw:
        timings.RasToCasDelay = (ULONG)(pre & 0x1F);
        timings.RowPrecharge = (ULONG)((pre >> 5) & 0x1F);
        timings.RowActive = (ULONG)((pre >> 10) & 0x3F);
        timings.CasLatency = (ULONG)(odt & 0x1F);
        timings.RefreshCycle = (ULONG)((refresh >> 16) & 0x1FF);
        stretch = (ULONG)((config >> 30) & 0x3);
        timings.CommandRate = stretch <= 1 ? 1 : stretch;
        break;
    case PhpHwMmioTimingsSkl:
        if ((config & 0x3) == 0x3) // SC_GS_CFG DRAM type: 0 DDR4, 1 DDR3, 2 LPDDR3
            return FALSE;

        timings.RowPrecharge = (ULONG)(pre & 0x3F);
        timings.RasToCasDelay = timings.RowPrecharge;
        timings.RowActive = (ULONG)((pre >> 8) & 0x7F);
        timings.CasLatency = (ULONG)((odt >> 16) & 0x1F);
        timings.RefreshCycle = (ULONG)((refresh >> 16) & 0x3FF);
        stretch = (ULONG)((config >> 2) & 0x3);
        timings.CommandRate = stretch == 3 ? 0 : stretch + 1;
        break;
    case PhpHwMmioTimingsIcl:
        timings.RowPrecharge = (ULONG)(pre & 0x3F);
        timings.RasToCasDelay = timings.RowPrecharge;
        timings.RowActive = (ULONG)((pre >> 9) & 0x7F);
        timings.CasLatency = (ULONG)((odt >> 16) & 0x3F);
        timings.RefreshCycle = (ULONG)((refresh >> 16) & 0x3FF);
        stretch = (ULONG)((config >> 3) & 0x3);
        timings.CommandRate = stretch == 3 ? 0 : stretch + 1;
        break;
    case PhpHwMmioTimingsTglU:
    case PhpHwMmioTimingsTglH:
        timings.RowPrecharge = (ULONG)(pre & 0x7F);
        timings.RowActive = (ULONG)((pre >> 33) & 0xFF);
        timings.RasToCasDelay = (ULONG)((pre >> 41) & 0x7F);
        timings.CasLatency = (ULONG)((odt >> 16) & 0x7F);
        timings.RefreshCycle = (ULONG)((refresh >> 17) & 0xFFF);
        stretch = (ULONG)((config >> 3) & 0x3);
        timings.CommandRate = stretch == 3 ? 0 : stretch + 1;
        break;
    case PhpHwMmioTimingsAdl:
        timings.RowPrecharge = (ULONG)(pre & 0xFF);
        timings.RowActive = (ULONG)((pre >> 42) & 0x1FF);
        timings.RasToCasDelay = (ULONG)((pre >> 51) & 0xFF);
        timings.CasLatency = (ULONG)((odt >> 16) & 0x7F);
        timings.RefreshCycle = (ULONG)((refresh >> 18) & 0x1FFF);
        stretch = (ULONG)((config >> 3) & 0x3);
        timings.CommandRate = stretch == 3 ? 0 : stretch + 1;
        break;
    case PhpHwMmioTimingsMtl:
    case PhpHwMmioTimingsLnl:
        timings.RowPrecharge = (ULONG)(pre & 0xFF);
        timings.RowActive = (ULONG)((pre >> 45) & 0x1FF);
        timings.RasToCasDelay = (ULONG)((act >> 22) & 0xFF);
        timings.CasLatency = (ULONG)((odt >> 16) & 0x7F);
        timings.RefreshCycle = (ULONG)((refresh >> 18) & 0x1FFF);
        timings.CommandRate = (config & (1ULL << 3)) ? 2 : 1;
        break;
    default:
        return FALSE;
    }

    // An unpopulated channel can keep plausible reset values; the TGL TC_PRE default decodes to 8-8-28.
    if ((Format == PhpHwMmioTimingsTglU || Format == PhpHwMmioTimingsTglH) && pre == 0x103803083008)
        return FALSE;

    if (
        timings.RefreshCycle == 0 ||
        timings.CasLatency < 4 || timings.CasLatency > 120 ||
        timings.RasToCasDelay < 4 || timings.RasToCasDelay > 200 ||
        timings.RowPrecharge < 4 || timings.RowPrecharge > 200 ||
        timings.RowActive < timings.RasToCasDelay
        )
    {
        return FALSE;
    }

    *Timings = timings;
    return TRUE;
}

/**
 * Reads, in one call through the host bridge's MCHBAR, the timing registers of every channel and keeps those of the
 * first populated one.
 */
static VOID PhpHwMmioReadTimings(
    _In_ PPHP_HW_MMIO_WINDOW Window
    )
{
    static CONST USHORT offsets[][PHP_HW_MMIO_TIMINGS_REGISTERS] =
    {
        { 0 }, // None
        { 0x000, 0x004, 0x298, 0, 0 }, // Snb: TC_DBP, TC_RAP, TC_RFTP
        { 0x000, 0x004, 0x298, 0x014, 0 }, // Hsw: TC_BANK, TC_BANK_RANK_A, TC_RFTP, TC_BANK_RANK_D
        { 0x000, 0x01c, 0x23c, 0x070, 0 }, // Skl
        { 0x000, 0x088, 0x23c, 0x070, 0 }, // Icl
        { 0x000, 0x088, 0x23c, 0x070, 0 }, // TglU
        { 0x000, 0x088, 0x43c, 0x070, 0 }, // TglH
        { 0x000, 0x088, 0x43c, 0x070, 0 }, // Adl
        { 0x000, 0x088, 0x4a0, 0x070, 0x138 }, // Mtl
        { 0x000, 0, 0x4a0, 0x070, 0x138 }, // Lnl
    };
    PHP_HW_MMIO_TIMINGS_FORMAT format = PhpHwMmioTimingsFormat;
    KPH_HW_WINDOW_ITEM items[PHP_HW_MMIO_TIMINGS_CHANNELS * PHP_HW_MMIO_TIMINGS_REGISTERS];
    KPH_HW_VALUE64 values[PHP_HW_MMIO_TIMINGS_CHANNELS * PHP_HW_MMIO_TIMINGS_REGISTERS];
    ULONG channels[PHP_HW_MMIO_TIMINGS_CHANNELS];
    ULONG channelCount;
    ULONG count = 0;
    ULONG base;
    ULONG stride;

    C_ASSERT(RTL_NUMBER_OF(offsets) == PhpHwMmioTimingsLnl + 1);

    if (format == PhpHwMmioTimingsNone)
        return;

    if (format >= PhpHwMmioTimingsTglH)
    {
        base = 0xE000;
        stride = 0x800;
    }
    else
    {
        base = 0x4000;
        stride = 0x400;
    }

    // Channels 0 and 1 of the first controller, then of the second at +10000h.
    channels[0] = base;
    channels[1] = base + stride;
    channels[2] = 0x10000 + base;
    channels[3] = 0x10000 + base + stride;
    channelCount = format >= PhpHwMmioTimingsTglU ? 4 : 2;

    for (ULONG i = 0; i < channelCount; i++)
    {
        // A register the layout does not use (offset 0 past TC_PRE) is not asked for.
        for (ULONG j = 0; j < PHP_HW_MMIO_TIMINGS_REGISTERS; j++)
        {
            if (j && !offsets[format][j])
                continue;

            items[count].Offset = channels[i] + offsets[format][j];
            items[count].Width = (j == 0 && format >= PhpHwMmioTimingsTglU) ? 8 : 4;
            count++;
        }
    }

    if (!NT_SUCCESS(KphHwPciMmioRead(Window->Locator, Window->Window, items, count, values)))
        return;

    for (ULONG i = 0, e = 0; i < channelCount; i++)
    {
        ULONG64 registers[PHP_HW_MMIO_TIMINGS_REGISTERS] = { 0 };
        BOOLEAN valid = TRUE;

        for (ULONG j = 0; j < PHP_HW_MMIO_TIMINGS_REGISTERS; j++)
        {
            if (j && !offsets[format][j])
                continue;

            if (NT_SUCCESS(values[e].Status))
                registers[j] = values[e].Value;
            else
                valid = FALSE;

            e++;
        }

        if (valid && PhpHwMmioDecodeTimings(format, registers, &PhpHwMmioTimings))
        {
            PhpHwMmioTimingsValid = TRUE;
            break;
        }
    }
}

/**
 * Reads, in one call through the host bridge's MCHBAR, MEMSS_PMA_CR_MEM_CONFIG and the MAD registers; the counters of
 * an IMC that MEM_CONFIG shows disabled leave the plan.
 */
static VOID PhpHwMmioReadMemoryConfig(
    _In_ PPHP_HW_MMIO_WINDOW Window
    )
{
    static CONST PHP_HW_MMIO_REG imcRegisters[2][4] =
    {
        { PhpHwMmioImc0Reads, PhpHwMmioImc0Writes, PhpHwMmioImc0ReadsAlt, PhpHwMmioImc0WritesAlt },
        { PhpHwMmioImc1Reads, PhpHwMmioImc1Writes, PhpHwMmioImc1ReadsAlt, PhpHwMmioImc1WritesAlt },
    };
    KPH_HW_WINDOW_ITEM items[PHP_HW_MMIO_CONFIG_REGISTERS];
    KPH_HW_VALUE64 values[PHP_HW_MMIO_CONFIG_REGISTERS];
    PHP_HW_MMIO_MEMORY_CONFIG* config = &PhpHwMmioMemoryConfig;

    items[0].Offset = PH_INTEL_MCHBAR_MEMSS_MEM_CONFIG;
    items[1].Offset = PH_INTEL_MCHBAR_MEMSS_MEM_CONFIG_NVL;

    for (ULONG i = 0; i < 2; i++)
    {
        items[2 + i * 3].Offset = i * PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_MAD_INTER_CHANNEL;
        items[3 + i * 3].Offset = i * PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_MAD_DIMM;
        items[4 + i * 3].Offset = i * PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_MAD_DIMM + 4;
    }

    // A part permits the MAD registers of one layout; DRP0 only on Apollo and Gemini Lake.
    items[8].Offset = PH_INTEL_MCHBAR_MAD_INTER_CHANNEL_ICL;
    items[9].Offset = PH_INTEL_MCHBAR_MAD_DIMM_ICL;
    items[10].Offset = PH_INTEL_MCHBAR_MAD_DIMM_ICL + 4;

    for (ULONG i = 0; i < PH_INTEL_MCHBAR_DUNITS; i++)
        items[11 + i].Offset = PH_INTEL_MCHBAR_DUNIT_DRP0 + i * PH_INTEL_MCHBAR_DUNIT_STRIDE;

    for (ULONG i = 0; i < RTL_NUMBER_OF(items); i++)
        items[i].Width = 4;

    if (!NT_SUCCESS(KphHwPciMmioRead(Window->Locator, Window->Window, items, RTL_NUMBER_OF(items), values)))
        return;

    // A part permits one of the two MEM_CONFIG offsets.
    for (ULONG i = 0; i < 2; i++)
    {
        if (NT_SUCCESS(values[i].Status) && !(config->Valid & 0x1))
        {
            config->MemConfig = (ULONG)values[i].Value;
            config->Valid |= 0x1;
        }
    }

    for (ULONG i = 0; i < 2 * 3; i++)
    {
        if (NT_SUCCESS(values[2 + i].Status))
        {
            config->Mad[i / 3][i % 3] = (ULONG)values[2 + i].Value;
            config->Valid |= 0x2UL << i;
        }
    }

    for (ULONG i = 0; i < 3; i++)
    {
        if (NT_SUCCESS(values[8 + i].Status) && !(config->Valid & (0x2UL << i)))
        {
            config->Mad[0][i] = (ULONG)values[8 + i].Value;
            config->Valid |= 0x2UL << i;
        }
    }

    for (ULONG i = 0; i < PH_INTEL_MCHBAR_DUNITS; i++)
    {
        if (NT_SUCCESS(values[11 + i].Status))
        {
            config->Drp0[i] = (ULONG)values[11 + i].Value;
            config->Valid |= 0x80UL << i;
        }
    }

    // MC0_EN [0], MC1_EN [1]; neither set is not a configuration.
    if ((config->Valid & 0x1) && (config->MemConfig & 0x3))
    {
        for (ULONG i = 0; i < 2; i++)
        {
            if (config->MemConfig & (1UL << i))
                continue;

            for (ULONG j = 0; j < RTL_NUMBER_OF(imcRegisters[i]); j++)
                PhpHwMmioDisable(imcRegisters[i][j]);
        }
    }
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

    if (!PhpHwPackageCount)
        return STATUS_NOT_SUPPORTED;

    // The script gates the controllers by model; they need no processor identity here.
    PhpHwMmioProbeMcs();

    // The windows need the processor identity of cpu: their decoders depend on the model.
    if (!PhpHwCpuQueryIntel(&PhpHwMmioIdentity))
        return PhpHwMmioMcCount ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;

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
    PhpHwMmioHostBridge = PhpHwMmioCandidateCount != 0;
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
        return PhpHwMmioMcCount ? STATUS_SUCCESS : lastStatus;

    // The memory configuration and the timings are read once, through the host bridge's MCHBAR.
    PhpHwMmioTimingsFormat = PhpHwMmioQueryTimingsFormat();

    if (PhpHwMmioHostBridge && PhpHwMmioWindows[0].Locator == candidates[0] && PhpHwMmioWindows[0].Window == KPH_HW_WINDOW_MCHBAR)
    {
        PhpHwMmioReadMemoryConfig(&PhpHwMmioWindows[0]);
        PhpHwMmioReadTimings(&PhpHwMmioWindows[0]);
    }

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

    // The memory clock registers stay only when they decode, and agree with SMBIOS.
    {
        PHP_HW_MEMORY_DEVICES devices;
        FLOAT mhz;
        FLOAT dataRate;
        ULONG gear;

        PhpHwQueryMemoryDevices(&devices);
        PhpHwMmioSmbiosDataRate = devices.ConfiguredSpeed;
        PhpHwFreeMemoryDevices(&devices);

        if (!PhpHwMmioDecodeMemoryClock(&mhz, &dataRate, &gear))
        {
            PhpHwMmioDisable(PhpHwMmioMcBiosReq);
            PhpHwMmioDisable(PhpHwMmioMcBiosData);
            PhpHwMmioDisable(PhpHwMmioSaPerfStatus);
            PhpHwMmioDisable(PhpHwMmioBclkFreq);
            PhpHwMmioDisable(PhpHwMmioMemssBiosData);
        }
    }

    // RP_STATE_CAP RP0 of 0 or FFh: no integrated graphics (dGPU-only SKU or iGPU disabled).
    if (PhpHwMmioValue(PhpHwMmioRpStateCap, &value) && (value & 0xFF) && (value & 0xFF) != 0xFF)
    {
        PhpHwMmioGtCaps = value;

        if (PhpHwMmioValue(PhpHwMmioFreqInfo, &value))
            PhpHwMmioGtRpe = value;
    }
    else
    {
        PhpHwMmioDisable(PhpHwMmioGtPerfStatus);
        PhpHwMmioDisable(PhpHwMmioGtPerfStatusLp);
    }

    if (PhpHwMmioActive(PhpHwMmioGtPerfStatusRegister()))
        PhpHwMmioGtIndex = PhpHwAddNamedSensor(PhHwNamedSensorUncoreClock, 0, PHP_HW_MMIO_GT_CLOCK_ID, L"Graphics");

    // Firmware enables the PCH sensor (TSEL.ETS); it is never enabled here.
    if (
        !PhpHwMmioValue(PhpHwMmioPchTsel, &value) || !(value & 0x1) ||
        !PhpHwMmioValue(PhpHwMmioPchTemp, &value) || !PhIntelPchTemperatureToCelsius((USHORT)value, &celsius)
        )
    {
        PhpHwMmioDisable(PhpHwMmioPchTemp);
    }

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

    if (PhpHwMmioReadMcs())
        retried = TRUE;
    else if (PhpHwMmioMcCount)
        answered = TRUE;

    PhpHwMmioLastSampleTime = PhpHwMmioSampleTime;
    PhpHwMmioSampleTime = PhpHwQueryTime();

    if (PhpHwMmioMcCount)
    {
        PhpHwMmioPublishMcs(PhpHwMmioLastSampleTime && PhpHwMmioSampleTime > PhpHwMmioLastSampleTime ?
            (FLOAT)(PhpHwMmioSampleTime - PhpHwMmioLastSampleTime) / (FLOAT)PHP_HW_SECOND : 0.0f);
    }

    if (PhpHwMmioActive(PhpHwMmioPackageTemperature))
        owned |= PH_HW_CAP_PACKAGE_TEMP;
    if (PhpHwMmioActive(PhpHwMmioPp0Temperature))
        owned |= PH_HW_CAP_CORES_TEMP;
    if (PhpHwMmioActive(PhpHwMmioPp1Temperature))
        owned |= PH_HW_CAP_GRAPHICS_TEMP;
    if (PhpHwMmioActive(PhpHwMmioMcBiosData) || PhpHwMmioActive(PhpHwMmioSaPerfStatus) || PhpHwMmioActive(PhpHwMmioMcBiosReq) ||
        PhpHwMmioActive(PhpHwMmioMemssBiosData))
        owned |= PH_HW_CAP_MEMORY_CLOCK;

    for (PHP_HW_MMIO_REG reg = 0; reg < PhpHwMmioMaximum; reg++)
    {
        if (PhpHwMmioRegInfo[reg].Imc && PhpHwMmioActive(reg))
            owned |= PH_HW_CAP_MEMORY_BANDWIDTH;
    }

    // The timings are the probe's.
    if (PhpHwMmioTimingsValid)
    {
        sensor->MemoryTimings = PhpHwMmioTimings;
        owned |= PH_HW_CAP_MEMORY_TIMINGS;
        valid |= PH_HW_CAP_MEMORY_TIMINGS;
    }

    {
        FLOAT mhz;
        FLOAT dataRate;
        FLOAT readBandwidth;
        FLOAT writeBandwidth;
        ULONG gear;

        if (PhpHwMmioDecodeMemoryClock(&mhz, &dataRate, &gear))
        {
            sensor->MemoryClockMhz = mhz;
            sensor->MemoryDataRate = dataRate;
            sensor->MemoryGear = gear;
            valid |= PH_HW_CAP_MEMORY_CLOCK;
        }
        else
        {
            dataRate = 0.0f;
        }

        if (PhpHwMmioDecodeBandwidth(dataRate, &readBandwidth, &writeBandwidth))
        {
            sensor->MemoryReadBandwidth = readBandwidth;
            sensor->MemoryWriteBandwidth = writeBandwidth;
            valid |= PH_HW_CAP_MEMORY_BANDWIDTH;
        }
    }

    if (PhpHwMmioGtIndex != ULONG_MAX)
    {
        BOOLEAN gtValid = PhpHwMmioValue(PhpHwMmioGtPerfStatusRegister(), &value);
        FLOAT mhz = 0.0f;

        if (gtValid)
        {
            PhpHwMmioGtPerfStatusLast = value;
            mhz = PhpHwMmioDecodeGtClock(value);

            if (mhz >= PhpHwMmioDecodeGtClock(PhpHwMmioGtPerfStatusPeak))
                PhpHwMmioGtPerfStatusPeak = value;

            // RP0 bounds the render clock; above it the decode is wrong for the part.
            if (PhpHwMmioGtCaps && mhz > (FLOAT)((PhpHwMmioGtCaps & 0xFF) * 50))
                gtValid = FALSE;
        }

        PhpHwSetNamedSensor(PhpHwMmioGtIndex, gtValid, mhz);
    }

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

    // The chipset temperature's first source; without it, platform's PWRM.
    if (PhpHwMmioActive(PhpHwMmioPchTemp))
    {
        PhpHwChipsetTemperatureValid = FALSE;

        if (PhpHwMmioValue(PhpHwMmioPchTemp, &value) && PhIntelPchTemperatureToCelsius((USHORT)value, &celsius))
        {
            PhpHwChipsetTemperature = celsius;
            PhpHwChipsetTemperatureValid = TRUE;
        }
    }

    // cpu publishes the active limits, the lower of these and the MSR's.
    PhpHwMmioLimit1 = PhpHwMmioDecodePowerLimit(PhpHwMmioPowerLimit1);
    PhpHwMmioLimit2 = PhpHwMmioDecodePowerLimit(PhpHwMmioPowerLimit2);

    if (PhpHwMmioRaplLimitedIndex != ULONG_MAX)
    {
        BOOLEAN limited = PhpHwMmioDecodeRaplLimited(&percent);

        PhpHwSetNamedSensor(PhpHwMmioRaplLimitedIndex, limited, percent);
    }

    PhpHwFamilies[PhpHwFamilyMmio].Capabilities =
        (PhpHwMmioActive(PhpHwMmioPchTemp) ? PH_HW_CAP_CHIPSET_TEMP : 0) |
        ((PhpHwMmioRaplLimitedIndex != ULONG_MAX && PhpHwMmioActive(PhpHwMmioRaplPerfStatus)) ? PH_HW_CAP_RESIDENCY : 0) |
        ((PhpHwMmioGtIndex != ULONG_MAX && PhpHwMmioActive(PhpHwMmioGtPerfStatusRegister())) ? PH_HW_CAP_UNCORE_CLOCK : 0);

    // Only the bits this collector owns are replaced.
    WriteULongRelease(&sensor->Valid, (ReadULongAcquire(&sensor->Valid) & ~owned) | (valid & owned));

    return retried && !answered ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Determines whether the PCH thermal function's window is the chipset temperature's source, for platform.
 */
BOOLEAN PhpHwMmioChipsetPlanned(
    VOID
    )
{
    return PhpHwMmioWindows && PhpHwMmioActive(PhpHwMmioPchTemp);
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

    if (PhpHwMmioMcs)
    {
        PhFree(PhpHwMmioMcs);
        PhpHwMmioMcs = NULL;
    }

    PhpHwMmioMcCount = 0;
    PhpHwMmioCandidateCount = 0;
    PhpHwMmioWindowCount = 0;
    PhpHwMmioUnitWatts = 0.0f;
    PhpHwMmioLimit1 = 0.0f;
    PhpHwMmioLimit2 = 0.0f;
    PhpHwMmioRaplLimitedIndex = ULONG_MAX;
    PhpHwMmioGtIndex = ULONG_MAX;
    PhpHwMmioSmbiosDataRate = 0;
    PhpHwMmioGtCaps = 0;
    PhpHwMmioGtRpe = 0;
    PhpHwMmioGtPerfStatusLast = 0;
    PhpHwMmioGtPerfStatusPeak = 0;
    PhpHwMmioDataRateRejected = FALSE;
    PhpHwMmioHostBridge = FALSE;
    PhpHwMmioTimingsFormat = PhpHwMmioTimingsNone;
    PhpHwMmioTimingsValid = FALSE;
    memset(&PhpHwMmioMemoryConfig, 0, sizeof(PhpHwMmioMemoryConfig));
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

    for (ULONG i = 0; i < PhpHwMmioMcCount; i++)
        PhAppendFormatStringBuilder(Builder, L"  Memory controller function 0x%lx (package %lu): controllers 0x%lx\n", PhpHwMmioMcs[i].Locator, i, PhpHwMmioMcs[i].Controllers);

    if (PhpHwMmioWindowCount)
    {
        static CONST PCWSTR formatNames[] = { L"none", L"SNB", L"HSW", L"SKL", L"ICL", L"TGL-U", L"TGL-H", L"ADL", L"MTL", L"LNL" };

        if (PhpHwMmioMemoryConfig.Valid)
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"  Memory configuration: valid 0x%lx, MEM_CONFIG 0x%08lx, MAD 0x%08lx 0x%08lx 0x%08lx / 0x%08lx 0x%08lx 0x%08lx; ",
                PhpHwMmioMemoryConfig.Valid,
                PhpHwMmioMemoryConfig.MemConfig,
                PhpHwMmioMemoryConfig.Mad[0][0],
                PhpHwMmioMemoryConfig.Mad[0][1],
                PhpHwMmioMemoryConfig.Mad[0][2],
                PhpHwMmioMemoryConfig.Mad[1][0],
                PhpHwMmioMemoryConfig.Mad[1][1],
                PhpHwMmioMemoryConfig.Mad[1][2]
                );
        }
        else
        {
            PhAppendStringBuilder2(Builder, L"  Memory configuration: none; ");
        }

        PhAppendFormatStringBuilder(
            Builder,
            L"timings format %s, %s\n",
            formatNames[PhpHwMmioTimingsFormat],
            PhpHwMmioTimingsValid ? L"read" : L"none"
            );

        PhAppendFormatStringBuilder(
            Builder,
            L"  Memory: SMBIOS %lu MT/s%s; GT: RP_STATE_CAP 0x%08lx, FREQ_INFO_REC 0x%08lx, GT_PERF_STATUS 0x%08lx, peak 0x%08lx\n",
            PhpHwMmioSmbiosDataRate,
            PhpHwMmioDataRateRejected ? L", the decoded data rate disagreed" : L"",
            PhpHwMmioGtCaps,
            PhpHwMmioGtRpe,
            PhpHwMmioGtPerfStatusLast,
            PhpHwMmioGtPerfStatusPeak
            );
    }

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
