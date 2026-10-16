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

#ifndef _PH_CPUSENSOR_H
#define _PH_CPUSENSOR_H

EXTERN_C_START

typedef struct _PH_CPU_IDENTITY
{
    ULONG Vendor[3];
    USHORT DisplayFamily;
    UCHAR DisplayModel;
    UCHAR Stepping;
    BOOLEAN Hybrid;
} PH_CPU_IDENTITY, *PPH_CPU_IDENTITY;
typedef const PH_CPU_IDENTITY* PCPH_CPU_IDENTITY;

// Intel MSR
#define PH_INTEL_MSR_MPERF 0xE7
#define PH_INTEL_MSR_APERF 0xE8
#define PH_INTEL_MSR_PLATFORM_INFO 0xCE
#define PH_INTEL_MSR_PERF_STATUS 0x198
#define PH_INTEL_MSR_THERM_STATUS 0x19C
#define PH_INTEL_MSR_TEMPERATURE_TARGET 0x1A2
#define PH_INTEL_MSR_TURBO_RATIO_LIMIT 0x1AD
#define PH_INTEL_MSR_PACKAGE_THERM_STATUS 0x1B1
#define PH_INTEL_MSR_RAPL_POWER_UNIT 0x606
#define PH_INTEL_MSR_PKG_POWER_LIMIT 0x610
#define PH_INTEL_MSR_PKG_ENERGY_STATUS 0x611
#define PH_INTEL_MSR_PKG_POWER_INFO 0x614
#define PH_INTEL_MSR_DRAM_ENERGY_STATUS 0x619
#define PH_INTEL_MSR_UNCORE_PERF_STATUS 0x621
#define PH_INTEL_MSR_PP0_ENERGY_STATUS 0x639
#define PH_INTEL_MSR_PP1_ENERGY_STATUS 0x641
#define PH_INTEL_MSR_PLATFORM_ENERGY_STATUS 0x64D
#define PH_INTEL_MSR_VR_CURRENT_CONFIG 0x601 // PL4 [12:0] in RAPL power units on PhIntelHasPl4 models
#define PH_INTEL_MSR_CORE_PERF_LIMIT_REASONS 0x64F // SKL and later; status bits, log = bit + 16
#define PH_INTEL_MSR_PLATFORM_POWER_LIMIT 0x65C // PSYS PL1/PL2, the MSR_PKG_POWER_LIMIT layout
#define PH_INTEL_MSR_HSW_CORE_PERF_LIMIT_REASONS 0x690 // Haswell and Broadwell client

// AMD MSR
#define PH_AMD_MSR_MPERF_READONLY 0xC00000E7
#define PH_AMD_MSR_APERF_READONLY 0xC00000E8
#define PH_AMD_MSR_PSTATE_STAT 0xC0010063
#define PH_AMD_MSR_PSTATE_DEF0 0xC0010064
#define PH_AMD_MSR_COFVID_STATUS 0xC0010071
#define PH_AMD_MSR_HW_PSTATE_STATUS 0xC0010293
#define PH_AMD_MSR_RAPL_PWR_UNIT 0xC0010299
#define PH_AMD_MSR_CORE_ENERGY_STAT 0xC001029A
#define PH_AMD_MSR_PKG_ENERGY_STAT 0xC001029B

// AMD SMN
#define PH_AMD_SMN_THM_TCON_CUR_TMP 0x00059800
#define PH_AMD_SMN_SVI_TFN 0x0005A008
#define PH_AMD_SMN_SVI_PLANE0 0x0005A00C
#define PH_AMD_SMN_SVI_PLANE1 0x0005A010
#define PH_AMD_SMN_SVI_PLANE0_M30H 0x0005A014

// AMD UMC; channel n is at PH_AMD_SMN_UMC_BASE + (n << PH_AMD_SMN_UMC_CHANNEL_SHIFT)
#define PH_AMD_SMN_UMC_BASE 0x00050000
#define PH_AMD_SMN_UMC_CHANNEL_SHIFT 20
#define PH_AMD_UMC_BASE_ADDR_CS0 0x000 // bit 0 set when chip select 0 (DIMM 0) is enabled
#define PH_AMD_UMC_BASE_ADDR_CS2 0x008 // bit 0 set when chip select 2 (DIMM 1) is enabled
#define PH_AMD_UMC_CONFIGURATION 0x100 // DDR5 layout: [2:0] 1 for DDR5
#define PH_AMD_UMC_DRAM_CONFIGURATION 0x200 // DDR5 layout: [18] gear down, [17] 2T command rate
#define PH_AMD_UMC_DRAM_TIMING1 0x204 // DDR5 layout: [21:16] tRCDRD, [14:8] tRAS, [5:0] tCL
#define PH_AMD_UMC_DRAM_TIMING2 0x208 // DDR5 layout: [21:16] tRP, [7:0] tRC
#define PH_AMD_UMC_DRAM_TIMING_RFC 0x260 // DDR5 layout: [15:0] tRFC1; four registers, the first in use applies
#define PH_AMD_UMC_CAP 0xDF0 // bit 19 set when the channel is disabled

// AMD D18F3 / D0F0xBC index
#define PH_AMD_D18F3_REPORTED_TEMP_CTRL 0xA4
#define PH_AMD_D0F0XBC_REPORTED_TEMP_CTRL 0xD8200CA4

// Intel MCHBAR (host bridge) and processor-thermal BAR0 offsets
#define PH_INTEL_MCHBAR_IMC_DATA_READS 0x5050 // 32-bit, 64-byte units (SNB-ICL)
#define PH_INTEL_MCHBAR_IMC_DATA_WRITES 0x5054
#define PH_INTEL_MCHBAR_IMC_READS 0x5058 // 64-bit, 64-byte units (TGL+), per IMC at PH_INTEL_MCHBAR_IMC_STRIDE
#define PH_INTEL_MCHBAR_IMC_WRITES 0x50A0
#define PH_INTEL_MCHBAR_IMC_READS_ALT 0xD858
#define PH_INTEL_MCHBAR_IMC_WRITES_ALT 0xD8A0
#define PH_INTEL_MCHBAR_IMC_STRIDE 0x10000
#define PH_INTEL_MCHBAR_IMC_REQUESTS 0xD840 // 64-bit PWM_TOTAL_REQCOUNT, 32 or 64 B requests, per IMC (MTL+)
#define PH_INTEL_MCHBAR_MAD_INTER_CHANNEL 0xD800 // per IMC (MTL+)
#define PH_INTEL_MCHBAR_MAD_DIMM 0xD80C // MAD_DIMM_CH0; CH1 at +4, per IMC (MTL+)
#define PH_INTEL_MCHBAR_RANK_TEMPERATURE 0xE424 // channel 0; channel 1 at +0x800, per IMC (MTL+)
#define PH_INTEL_MCHBAR_RAPL_PERF_STATUS 0x58F0 // PACKAGE_RAPL_PERF_STATUS, 1/1024 s units of power limiting
#define PH_INTEL_MCHBAR_SA_PERF_STATUS 0x5918
#define PH_INTEL_MCHBAR_PKG_POWER_UNIT 0x5938
#define PH_INTEL_MCHBAR_GT_PERF_STATUS 0x5948 // render ratio [15:8], [16:8] from Skylake, [19:11] from Ice Lake (SNB..RPL)
#define PH_INTEL_MCHBAR_PKG_TEMPERATURE 0x5978
#define PH_INTEL_MCHBAR_PP0_TEMPERATURE 0x597C
#define PH_INTEL_MCHBAR_PP1_TEMPERATURE 0x5980
#define PH_INTEL_MCHBAR_TEMPERATURE_TARGET 0x599C
#define PH_INTEL_MCHBAR_RP_STATE_CAP 0x5998 // RP0 [7:0], RP1 [15:8], RPn [23:16], 50 MHz units
#define PH_INTEL_MCHBAR_PKG_POWER_LIMIT 0x59A0 // PL1; PL2 in the dword at +4
#define PH_INTEL_MCHBAR_MC_BIOS_REQ 0x5E00
#define PH_INTEL_MCHBAR_MC_BIOS_DATA 0x5E04
#define PH_INTEL_MCHBAR_FREQ_INFO_REC 0x5EF0 // RPE [15:8], Ice Lake and later
#define PH_INTEL_MCHBAR_BCLK_FREQ 0x5F60
#define PH_INTEL_MCHBAR_MEMSS_BIOS_DATA 0x13D10 // MEMSS_PMA_CR_BIOS_DATA (MTL, ARL)
#define PH_INTEL_MCHBAR_MEMSS_MEM_CONFIG 0x13D04 // MEMSS_PMA_CR_MEM_CONFIG: DDR type [8:6], MC1/MC0 enable [1:0] (MTL+)
#define PH_INTEL_MCHBAR_MEMSS_MEM_CONFIG_NVL 0x12904 // the same register on Nova Lake

// Intel PMC PWRM offsets (TGL+ PCH thermal behind the PMC)
#define PH_INTEL_PWRM_THERMAL_MINTEMP 0x1500 // S9 1 C, since boot
#define PH_INTEL_PWRM_THERMAL_MAXTEMP 0x1504 // S9 1 C, since boot
#define PH_INTEL_PWRM_THERMAL_TSS0 0x1560 // TSR [8:0] S9 1 C, TSRV bit 9

// Intel PCH thermal BAR0 offsets
#define PH_INTEL_PCH_THERMAL_TEMP 0x00
#define PH_INTEL_PCH_THERMAL_TSEL 0x08

// Intel TPMI features
#define PH_INTEL_TPMI_ID_RAPL 0x00
#define PH_INTEL_TPMI_ID_UNCORE 0x02
#define PH_INTEL_TPMI_ID_SST 0x05
#define PH_INTEL_TPMI_ID_PLR 0x0C
#define PH_INTEL_TPMI_ID_INFO 0x81
#define PH_INTEL_TPMI_INFO_BUS_INFO 0x08 // function [2:0], device [7:3], bus [15:8], package [23:16]
#define PH_INTEL_TPMI_RAPL_DOMAIN_SIZE 128
#define PH_INTEL_TPMI_RAPL_DOMAIN_SYSTEM 1
#define PH_INTEL_TPMI_RAPL_DOMAIN_PACKAGE 2
#define PH_INTEL_TPMI_RAPL_DOMAIN_MEMORY 4
#define PH_INTEL_TPMI_RAPL_REG_UNIT 1 // register n of a domain is at +8n
#define PH_INTEL_TPMI_RAPL_REG_ENERGY_STATUS 7
#define PH_INTEL_TPMI_RAPL_REG_DOMAIN_INFO 10 // bit 0: root domain
#define PH_INTEL_TPMI_RAPL_ENERGY_UNIT_SHIFT 6 // bits 10:6
#define PH_INTEL_TPMI_UNCORE_CLUSTER_OFFSET 0x08
#define PH_INTEL_TPMI_SST_PP_FEATURES 0x08 // from the PP block: PP [7:0] in qwords inside a level block
#define PH_INTEL_TPMI_SST_PP_LEVELS 0x10 // byte n: level n in qwords from the PP block
#define PH_INTEL_TPMI_SST_PP_STATUS 0x20 // current level [2:0], lock bit 3
#define PH_INTEL_TPMI_SST_PP_MAX_LEVEL 4
#define PH_INTEL_TPMI_SST_PP_INFO_1 0x08 // from the level's PP feature: TDP [46:32] 1/8 W, TjMax [54:47]
#define PH_INTEL_TPMI_SST_PP_INFO_11 0x58 // P0 [7:0], P1 [15:8], Pn [23:16], Pm [31:24], x 100 MHz
#define PH_INTEL_TPMI_PLR_DIE_LEVEL 0x18 // reasons: coarse [9:0], fine from bit 32

PHLIBAPI
BOOLEAN
NTAPI
PhGetCpuIdentity(
    _Out_ PPH_CPU_IDENTITY Identity
    );

PHLIBAPI
BOOLEAN
NTAPI
PhCpuIdentityIsIntel(
    _In_ PCPH_CPU_IDENTITY Identity
    );

PHLIBAPI
BOOLEAN
NTAPI
PhCpuIdentityIsAmd(
    _In_ PCPH_CPU_IDENTITY Identity
    );

PHLIBAPI
ULONG
NTAPI
PhIntelThermReadoutMask(
    _In_ PCPH_CPU_IDENTITY Identity,
    _In_ BOOLEAN Package
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhIntelThermStatusToCelsius(
    _In_ ULONG64 ThermStatus,
    _In_ ULONG TjMax,
    _In_ ULONG ReadoutMask,
    _Out_ PFLOAT Celsius
    );

PHLIBAPI
FLOAT
NTAPI
PhIntelPackageThermStatusToCelsius(
    _In_ ULONG64 PackageThermStatus,
    _In_ ULONG TjMax,
    _In_ ULONG ReadoutMask
    );

PHLIBAPI
ULONG
NTAPI
PhIntelTemperatureTargetTjMax(
    _In_ ULONG64 TemperatureTarget
    );

PHLIBAPI
ULONG
NTAPI
PhIntelFixedTjMax(
    _In_ PCPH_CPU_IDENTITY Identity
    );

PHLIBAPI
ULONG
NTAPI
PhIntelLimitReasonsMsr(
    _In_ PCPH_CPU_IDENTITY Identity
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIntelHasPl4(
    _In_ PCPH_CPU_IDENTITY Identity
    );

PHLIBAPI
ULONG
NTAPI
PhIntelUncoreRatio(
    _In_ ULONG64 UncorePerfStatus
    );

PHLIBAPI
FLOAT
NTAPI
PhRaplEnergyUnitJoules(
    _In_ ULONG64 RaplPowerUnit,
    _In_ PCPH_CPU_IDENTITY Identity
    );

PHLIBAPI
FLOAT
NTAPI
PhRaplPowerUnitWatts(
    _In_ ULONG64 RaplPowerUnit,
    _In_ PCPH_CPU_IDENTITY Identity
    );

PHLIBAPI
FLOAT
NTAPI
PhIntelRaplDramEnergyUnitJoules(
    _In_ ULONG64 RaplPowerUnit,
    _In_ PCPH_CPU_IDENTITY Identity
    );

PHLIBAPI
FLOAT
NTAPI
PhIntelRaplPlatformEnergyUnitJoules(
    _In_ ULONG64 RaplPowerUnit,
    _In_ PCPH_CPU_IDENTITY Identity
    );

PHLIBAPI
ULONG
NTAPI
PhIntelPerfStatusRatio(
    _In_ ULONG64 PerfStatus,
    _In_ PCPH_CPU_IDENTITY Identity
    );

PHLIBAPI
FLOAT
NTAPI
PhIntelPerfStatusToVolts(
    _In_ ULONG64 PerfStatus,
    _In_ PCPH_CPU_IDENTITY Identity
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhAmdTctlToCelsius(
    _In_ ULONG ThmTconCurTmp,
    _Out_ PFLOAT Celsius
    );

PHLIBAPI
FLOAT
NTAPI
PhAmdTctlOffset(
    _In_ PCPH_CPU_IDENTITY Identity,
    _In_ PCPH_STRINGREF BrandString
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhAmdTccdToCelsius(
    _In_ ULONG Tccd,
    _Out_ PFLOAT Celsius
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhAmdTccdAddressRange(
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PULONG BaseAddress,
    _Out_ PULONG MaximumCount
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhAmdReportedTempCtrlToCelsius(
    _In_ ULONG ReportedTempCtrl,
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PFLOAT Celsius
    );

PHLIBAPI
FLOAT
NTAPI
PhAmdPStateDefToMhz(
    _In_ ULONG64 PStateDef,
    _In_ PCPH_CPU_IDENTITY Identity
    );

PHLIBAPI
FLOAT
NTAPI
PhAmdHwPStateStatusToMhz(
    _In_ ULONG64 HwPStateStatus,
    _In_ PCPH_CPU_IDENTITY Identity
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhAmdPStateVid(
    _In_ ULONG64 PState,
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PUCHAR Vid
    );

PHLIBAPI
FLOAT
NTAPI
PhAmdCpuVidToVolts(
    _In_ UCHAR Vid,
    _In_ PCPH_CPU_IDENTITY Identity
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhAmdSviPlaneAddresses(
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PULONG CoreAddress,
    _Out_ PULONG SocAddress
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhAmdSviTelemetryToVolts(
    _In_ ULONG SviTfn,
    _In_ ULONG PlaneAddress,
    _In_ ULONG PlaneTelemetry,
    _Out_ PFLOAT Volts
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhAmdUmcChannelCount(
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PULONG Count
    );

PHLIBAPI
BOOLEAN
NTAPI
PhAmdUmcIsDdr5Layout(
    _In_ PCPH_CPU_IDENTITY Identity
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhAmdDramConfigurationToMhz(
    _In_ ULONG DramConfiguration,
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PFLOAT MemoryClockMhz,
    _Out_ PFLOAT DataRate
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhIntelMchbarToMemoryClock(
    _In_ PCPH_CPU_IDENTITY Identity,
    _In_opt_ const ULONG* McBiosReq,
    _In_opt_ const ULONG* McBiosData,
    _In_opt_ const ULONG* SaPerfStatus,
    _In_opt_ const ULONG* BclkFreq,
    _In_opt_ const ULONG* MemssBiosData,
    _Out_ PFLOAT MemoryClockMhz,
    _Out_ PFLOAT DataRate,
    _Out_ PULONG Gear
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhIntelMmioTemperatureToCelsius(
    _In_ ULONG Temperature,
    _Out_ PFLOAT Celsius
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhIntelPchTemperatureToCelsius(
    _In_ USHORT Temp,
    _Out_ PFLOAT Celsius
    );

PHLIBAPI
FLOAT
NTAPI
PhEffectiveMhzFromAperf(
    _In_ ULONG64 DeltaAperf,
    _In_ FLOAT Seconds
    );

PHLIBAPI
FLOAT
NTAPI
PhActiveMhzFromAperfMperf(
    _In_ ULONG64 DeltaAperf,
    _In_ ULONG64 DeltaMperf,
    _In_ FLOAT TscMhz
    );

#define PH_ARM_MAX_CLOCK_MHZ 10000.0f // a larger clock is a bad sample
typedef struct _PH_ARM_CYCLE_SAMPLE
{
    ULONG64 Counter; // CNTVCT_EL0
    ULONG64 Cycles; // AMEVCNTR00_EL0 (CPU_CYCLES), or PMCCNTR_EL0
    ULONG64 ConstantCycles; // AMEVCNTR01_EL0 (CNT_CYCLES)
} PH_ARM_CYCLE_SAMPLE, *PPH_ARM_CYCLE_SAMPLE;
typedef const PH_ARM_CYCLE_SAMPLE* PCPH_ARM_CYCLE_SAMPLE;

typedef enum _PH_ARM_CLOCK_STATUS
{
    PhArmClockValid,
    PhArmClockInvalid, // a counter went backwards or the result is implausible
    PhArmClockStalled // a counter did not advance on a core that ran the reads
} PH_ARM_CLOCK_STATUS;

_Success_(return == PhArmClockValid)
PHLIBAPI
PH_ARM_CLOCK_STATUS
NTAPI
PhArmCycleClocks(
    _In_ PCPH_ARM_CYCLE_SAMPLE Last,
    _In_ PCPH_ARM_CYCLE_SAMPLE Current,
    _In_ ULONG64 CounterHz,
    _In_ BOOLEAN Constant,
    _Out_ PFLOAT EffectiveMhz,
    _Out_ PFLOAT ActiveMhz
    );

EXTERN_C_END

#endif
