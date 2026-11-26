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
#include <sistatus.h>
#include <trace.h>

// Collector cpu: the processors' model-specific registers through KphHwCpuRead, one plan of registers per processor
// class, read once per update in calls of at most PHP_HW_CPU_BATCH items.

#define PHP_HW_CPU_BATCH 512 // items a call; halved for the session at the script's limits (D8)
#define PHP_HW_CPU_STRIKE_LIMIT 3 // failures of a register before it leaves the plan for the session
#define PHP_HW_CPU_BUS_MHZ 100.0f
#define PHP_HW_CPU_ENERGY_MAX_WATTS 10000.0f // above this an energy counter restarted
#define PHP_HW_CPU_MAX_INTERVAL 10.0f // seconds; a longer interval is a gap
#define PHP_HW_CPU_PACKAGE_CAPS (PH_HW_CAP_RAPL_PACKAGE | PH_HW_CAP_RAPL_CORE | PH_HW_CAP_RAPL_DRAM | PH_HW_CAP_RAPL_PSYS | \
    PH_HW_CAP_POWER_LIMIT1 | PH_HW_CAP_POWER_LIMIT2) // package bits this collector always owns
#define PHP_HW_CPU_ENERGY_IDLE_TICKS 5 // samples a RAPL energy counter may stay still before it leaves the plan
#define PHP_HW_CPU_UNCORE_ID 0x40000000 // named sensor id of the MSR uncore clock, apart from the TPMI domains
#define PHP_HW_CPU_LIMIT_PL4 0 // named limit sensors of a package; the index is the sensor's id
#define PHP_HW_CPU_LIMIT_PLATFORM_PL1 1
#define PHP_HW_CPU_LIMIT_PLATFORM_PL2 2
#define PHP_HW_CPU_LIMIT_SENSORS 3
#define PHP_HW_CPU_ARM_AMU_BUILD 26100 // from this build Windows reads AMCNTENSET0_EL0 at boot: the AMU does not trap
#define PHP_HW_CPU_ARM_AMU_COUNTERS 0x3 // AMCNTENSET0_EL0: CPU_CYCLES and CNT_CYCLES enabled
#define PHP_HW_CPU_ARM_AMU_ACTIVITY 0xC // AMCNTENSET0_EL0: INST_RETIRED and STALL_BACKEND_MEM enabled
#define PHP_HW_CPU_ARM_STALL_LIMIT 3 // updates a cycle counter may stand still before it leaves the processor's plan
#define PHP_HW_CPU_THERMAL_CAPS (PH_HW_CAP_PACKAGE_TEMP | PH_HW_CAP_CORE_TEMP | PH_HW_CAP_THROTTLE) // with a thermal plan

typedef enum _PHP_HW_CPU_CLASS
{
    PhpHwCpuNone,
    PhpHwCpuIntel,
    PhpHwCpuAmdZen, // AuthenticAMD or HygonGenuine, family 17h and later
    PhpHwCpuAmdLegacy, // AuthenticAMD, families 10h to 16h
    PhpHwCpuCentaur, // CentaurHauls or "  Shanghai  "
    PhpHwCpuArm64 // the application runs on ARM64
} PHP_HW_CPU_CLASS;

typedef enum _PHP_HW_REG
{
    PhpHwRegAperf,
    PhpHwRegMperf,
    PhpHwRegPerfStatus, // Intel IA32_PERF_STATUS (thread), AMD HW_PSTATE_STATUS (core)
    PhpHwRegThermStatus, // Intel IA32_THERM_STATUS (core)
    PhpHwRegCoreEnergy,
    PhpHwRegPackageThermStatus, // Intel IA32_PACKAGE_THERM_STATUS
    PhpHwRegPackageEnergy,
    PhpHwRegPp0Energy, // Intel
    PhpHwRegDramEnergy,
    PhpHwRegPlatformEnergy,
    PhpHwRegPowerLimit, // Intel MSR_PKG_POWER_LIMIT, read every update: firmware and DTT change it at runtime
    PhpHwRegLimitReasons, // Intel MSR_CORE_PERF_LIMIT_REASONS
    PhpHwRegVrCurrentConfig, // Intel MSR_VR_CURRENT_CONFIG, PL4
    PhpHwRegPlatformPowerLimit, // Intel MSR_PLATFORM_POWER_LIMIT
    PhpHwRegUncorePerfStatus, // Intel MSR_UNCORE_PERF_STATUS
    PhpHwRegCuPowerAccumulator, // AMD 15h and 16h CU_PWR_ACCUMULATOR
    PhpHwRegPtsc, // AMD 15h and 16h PTSC, read with the accumulator
    PhpHwRegCentaurTemperature, // Centaur and Zhaoxin 1423h (core)
    PhpHwRegArmCounter, // ARM64 CNTVCT_EL0
    PhpHwRegArmCycles, // ARM64 AMEVCNTR00_EL0 (CPU_CYCLES)
    PhpHwRegArmConstantCycles, // ARM64 AMEVCNTR01_EL0 (CNT_CYCLES)
    PhpHwRegArmPmuCycles, // ARM64 PMCCNTR_EL0, without the AMU
    PhpHwRegArmInstructions, // ARM64 AMEVCNTR02_EL0 (INST_RETIRED)
    PhpHwRegArmMemoryStalls, // ARM64 AMEVCNTR03_EL0 (STALL_BACKEND_MEM)
    PhpHwRegCoreC3Residency, // Intel MSR_CORE_C3_RESIDENCY, at the TSC rate, to PhpHwRegPackageC10Residency
    PhpHwRegCoreC6Residency,
    PhpHwRegCoreC7Residency,
    PhpHwRegPackageC2Residency,
    PhpHwRegPackageC3Residency,
    PhpHwRegPackageC6Residency,
    PhpHwRegPackageC7Residency,
    PhpHwRegPackageC8Residency,
    PhpHwRegPackageC9Residency,
    PhpHwRegPackageC10Residency,
    PhpHwRegPp1Energy, // Intel MSR_PP1_ENERGY_STATUS, the integrated graphics
    PhpHwRegRaplUnit, // probe only from here
    PhpHwRegPStateDef0,
    PhpHwRegTemperatureTarget, // Intel MSR_TEMPERATURE_TARGET, the Centaur and Zhaoxin critical temperature
    PhpHwRegPlatformInfo,
    PhpHwRegFsbFreq, // Intel Silvermont and Airmont MSR_FSB_FREQ
    PhpHwRegCuMaxPowerAccumulator, // AMD 15h and 16h CU_MAX_PWR_ACCUMULATOR
    PhpHwRegArmFrequency, // ARM64 CNTFRQ_EL0
    PhpHwRegArmAmuEnable, // ARM64 AMCNTENSET0_EL0
    PhpHwRegPmEnable, // Intel IA32_PM_ENABLE, the report only
    PhpHwRegHwpCapabilities, // Intel IA32_HWP_CAPABILITIES
    PhpHwRegHwpRequest, // Intel IA32_HWP_REQUEST
    PhpHwRegEnergyPerfBias, // Intel IA32_ENERGY_PERF_BIAS
    PhpHwRegSmiCount, // Intel MSR_SMI_COUNT, the report only
    PhpHwRegTurboRatioLimit, // Intel MSR_TURBO_RATIO_LIMIT
    PhpHwRegTurboRatioCores, // Intel MSR_TURBO_RATIO_LIMIT_CORES, the core counts of the groups
    PhpHwRegConfigTdpNominal, // Intel MSR_CONFIG_TDP_NOMINAL
    PhpHwRegConfigTdpLevel1, // Intel MSR_CONFIG_TDP_LEVEL_1
    PhpHwRegConfigTdpLevel2, // Intel MSR_CONFIG_TDP_LEVEL_2
    PhpHwRegConfigTdpControl, // Intel MSR_CONFIG_TDP_CONTROL
    PhpHwRegTurboActivationRatio, // Intel MSR_TURBO_ACTIVATION_RATIO
    PhpHwRegCppcCapability1, // AMD CPPC_CAP1, read on every core: its highest performance ranks the preferred cores
    PhpHwRegCppcEnable, // AMD CPPC_ENABLE
    PhpHwRegCppcRequest, // AMD CPPC_REQ
    PhpHwRegMaximum
} PHP_HW_REG;
C_ASSERT(PhpHwRegMaximum <= 64);

#define PHP_HW_CPU_CORE_STATES 3 // PhpHwRegCoreC3Residency to PhpHwRegCoreC7Residency
#define PHP_HW_CPU_PACKAGE_STATES 7 // PhpHwRegPackageC2Residency to PhpHwRegPackageC10Residency

// The Intel C-state residency counters: the register, its MSR and its state, cores' first.
static CONST struct { PHP_HW_REG Reg; ULONG Msr; UCHAR State; } PhpHwCpuResidencyRegs[PHP_HW_CPU_CORE_STATES + PHP_HW_CPU_PACKAGE_STATES] =
{
    { PhpHwRegCoreC3Residency, 0x3FC, 3 },
    { PhpHwRegCoreC6Residency, 0x3FD, 6 },
    { PhpHwRegCoreC7Residency, 0x3FE, 7 },
    { PhpHwRegPackageC2Residency, 0x60D, 2 },
    { PhpHwRegPackageC3Residency, 0x3F8, 3 },
    { PhpHwRegPackageC6Residency, 0x3F9, 6 },
    { PhpHwRegPackageC7Residency, 0x3FA, 7 },
    { PhpHwRegPackageC8Residency, 0x630, 8 },
    { PhpHwRegPackageC9Residency, 0x631, 9 },
    { PhpHwRegPackageC10Residency, 0x632, 10 },
};

typedef enum _PHP_HW_SCOPE
{
    PhpHwScopeThread,
    PhpHwScopeCore, // the first processor of each core
    PhpHwScopePackage, // the first processor of each package
    PhpHwScopeComputeUnit // AMD 15h and 16h, the first processor of each compute unit
} PHP_HW_SCOPE;

typedef struct _PHP_HW_REGISTER
{
    ULONG Register; // 0 when not in the plan
    PHP_HW_SCOPE Scope;
    BOOLEAN Disabled;
    UCHAR Strikes;
} PHP_HW_REGISTER, *PPHP_HW_REGISTER;

typedef struct _PHP_HW_SAMPLE
{
    ULONG64 Valid; // 1 << PHP_HW_REG
    ULONG64 LastValid;
    ULONG64 Value[PhpHwRegMaximum];
    ULONG64 LastValue[PhpHwRegMaximum];
} PHP_HW_SAMPLE, *PPHP_HW_SAMPLE;

typedef struct _PHP_HW_CPU_TAG
{
    PHP_HW_REG Reg;
    ULONG Processor; // the item's processor; its core and package come from the sensor
    BOOLEAN Unanswered; // its call failed as a whole: no value and no strike
} PHP_HW_CPU_TAG, *PPHP_HW_CPU_TAG;

static PHP_HW_CPU_CLASS PhpHwCpuClass = PhpHwCpuNone;
static PH_CPU_IDENTITY PhpHwCpuIdentity;
static ULONG PhpHwCpuSignature = 0;
static PHP_HW_REGISTER PhpHwCpuRegisters[PhpHwRegMaximum];
static ULONG PhpHwCpuBatch = PHP_HW_CPU_BATCH;
static ULONG PhpHwCpuCapacity = 0; // items of the whole plan
static PKPH_HW_CPU_ITEM PhpHwCpuItems = NULL;
static PKPH_HW_VALUE64 PhpHwCpuValues = NULL;
static PPHP_HW_CPU_TAG PhpHwCpuTags = NULL;
static PPHP_HW_SAMPLE PhpHwCpuSamples = NULL; // per processor
static PBOOLEAN PhpHwCpuFirstOfCore = NULL; // per processor
static PBOOLEAN PhpHwCpuFirstOfPackage = NULL; // per processor
static ULONG PhpHwCpuCoreCount = 0;
static PULONG PhpHwCpuCoreFirst = NULL; // per core, its first processor; ULONG_MAX for an index no processor has
static PULONG PhpHwCpuPackageFirst = NULL; // per package
static PULONG PhpHwCpuPackageCores = NULL; // per package
static PFLOAT PhpHwCpuTickCorePower = NULL; // per package
static PULONG PhpHwCpuTickCoreCount = NULL; // per package
static FLOAT PhpHwCpuTscMhz = 0.0f; // the base clock MPERF counts at; 0 when unknown
static ULONG PhpHwCpuClockControl = 0; // family 14h D18F3xD4: the main PLL its P-states divide; 0 when unknown
static FLOAT PhpHwCpuEnergyUnit = 0.0f; // joules
static FLOAT PhpHwCpuDramEnergyUnit = 0.0f;
static FLOAT PhpHwCpuPlatformEnergyUnit = 0.0f;
static FLOAT PhpHwCpuPowerUnit = 0.0f; // watts
static PFLOAT PhpHwCpuMsrPowerLimit1 = NULL; // per package, watts; 0 when unavailable
static PFLOAT PhpHwCpuMsrPowerLimit2 = NULL;
static PUCHAR PhpHwCpuPackageIdleTicks = NULL; // per package: samples whose energy did not move, until it does
static PUCHAR PhpHwCpuPp0IdleTicks = NULL;
static PUCHAR PhpHwCpuDramIdleTicks = NULL;
static PUCHAR PhpHwCpuPlatformIdleTicks = NULL;
static PBOOLEAN PhpHwCpuPackageEnergySeen = NULL; // per package: the counter moved
static PBOOLEAN PhpHwCpuPp0EnergySeen = NULL;
static PBOOLEAN PhpHwCpuDramEnergySeen = NULL;
static PBOOLEAN PhpHwCpuPlatformEnergySeen = NULL;
static PULONG PhpHwCpuUncoreIndex = NULL; // per package, the uncore clock's named sensor; ULONG_MAX when not listed
static PULONG PhpHwCpuLimitIndex = NULL; // per package, PHP_HW_CPU_LIMIT_SENSORS named sensors; ULONG_MAX when not listed
static ULONG64 PhpHwCpuPolicy[12] = { 0 }; // Intel, processor 0 at the probe: PhpHwRegPmEnable to PhpHwRegTurboActivationRatio
static ULONG PhpHwCpuTccOffset = ULONG_MAX; // Intel MSR_TEMPERATURE_TARGET 29:24; ULONG_MAX when unread
static PULONG PhpHwCpuPp1Index = NULL; // per package, the integrated graphics power's named sensor; ULONG_MAX when not listed
static ULONG PhpHwCpuPolicyValid = 0; // 1 << the index into PhpHwCpuPolicy
static KPH_HW_OC_PLANE PhpHwCpuOc[MaxKphHwOcPlane]; // Intel, the overclocking mailbox at the probe
static BOOLEAN PhpHwCpuOcValid = FALSE;
static ULONG64 PhpHwCpuCppc[3] = { 0 }; // AMD, processor 0 at the probe: PhpHwRegCppcCapability1 to PhpHwRegCppcRequest
static ULONG PhpHwCpuCppcValid = 0; // 1 << the index into PhpHwCpuCppc
static PUCHAR PhpHwCpuCppcHighest = NULL; // AMD, per core: CPPC_CAP1 highest performance; 0 when unread
static PULONG PhpHwCpuResidencyIndex = NULL; // the C-state named sensors, per core then per package; ULONG_MAX when not listed
static PBOOLEAN PhpHwCpuFirstOfUnit = NULL; // per processor; NULL without compute units
static ULONG PhpHwCpuUnitSampleRatio = 0; // CPUID 8000_0007h ECX
static ULONG64 PhpHwCpuUnitMaxAccumulator = 0; // CU_MAX_PWR_ACCUMULATOR; 0 when unread
static PKPH_HW_CPU_IDENTITY PhpHwCpuPackageIdentity = NULL; // x86, per package: what identify gave; zero when it failed
static PULONG PhpHwCpuArmMidr = NULL; // ARM64, per processor: the MIDR identify gave; 0 when not identified
static PULONG64 PhpHwCpuArmDisabled = NULL; // ARM64, per processor: 1 << PHP_HW_REG of the registers out of its plan
static PUCHAR PhpHwCpuArmStalls = NULL; // ARM64, per processor: updates its cycle counter stood still
static PULONG PhpHwCpuArmActivityIndex = NULL; // ARM64, per processor: its IPC and memory stall named sensors; ULONG_MAX when not listed
static ULONG64 PhpHwCpuArmCounterHz = 0; // CNTFRQ_EL0
static BOOLEAN PhpHwCpuVoltageSupported = FALSE;
static ULONG PhpHwCpuCoreReadoutMask = 0x7F; // IA32_THERM_STATUS readout width
static ULONG PhpHwCpuPackageReadoutMask = 0x7F; // IA32_PACKAGE_THERM_STATUS readout width
static ULONG PhpHwCpuPackageCaps = PHP_HW_CPU_PACKAGE_CAPS; // the package bits this collector owns this session
static PULONG PhpHwCpuTjMax = NULL; // per package, degrees Celsius; 0 when unknown
static PULONG PhpHwCpuTickValid = NULL; // per package: PH_HW_CAP_* the processors of the package gave this pass
static PULONG PhpHwCpuTickThrottle = NULL; // per package
static PFLOAT PhpHwCpuTickMaxCore = NULL; // per package
static ULONG PhpHwCpuItemCount = 0; // items of the last pass
static ULONG64 PhpHwCpuSampleTime = 0;
static ULONG64 PhpHwCpuLastSampleTime = 0;

static VOID PhpHwCpuSetRegister(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Register,
    _In_ PHP_HW_SCOPE Scope
    )
{
    PhpHwCpuRegisters[Reg].Register = Register;
    PhpHwCpuRegisters[Reg].Scope = Scope;
    PhpHwCpuRegisters[Reg].Disabled = FALSE;
    PhpHwCpuRegisters[Reg].Strikes = 0;
}

static BOOLEAN PhpHwCpuRegisterActive(
    _In_ PHP_HW_REG Reg
    )
{
    return PhpHwCpuRegisters[Reg].Register && !PhpHwCpuRegisters[Reg].Disabled;
}

static BOOLEAN PhpHwCpuProbeOnly(
    _In_ PHP_HW_REG Reg
    )
{
    return Reg >= PhpHwRegRaplUnit;
}

/**
 * Builds the plan of registers of the processor class.
 */
static VOID PhpHwCpuInitializePlan(
    VOID
    )
{
    ULONG p0Definition;

    memset(PhpHwCpuRegisters, 0, sizeof(PhpHwCpuRegisters));
    PhpHwCpuClockControl = 0;

    if (PhpHwCpuClass == PhpHwCpuAmdZen)
    {
        PhpHwCpuSetRegister(PhpHwRegAperf, PH_AMD_MSR_APERF_READONLY, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegMperf, PH_AMD_MSR_MPERF_READONLY, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegPerfStatus, PH_AMD_MSR_HW_PSTATE_STATUS, PhpHwScopeCore);
        PhpHwCpuSetRegister(PhpHwRegCoreEnergy, PH_AMD_MSR_CORE_ENERGY_STAT, PhpHwScopeCore);
        PhpHwCpuSetRegister(PhpHwRegPackageEnergy, PH_AMD_MSR_PKG_ENERGY_STAT, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegRaplUnit, PH_AMD_MSR_RAPL_PWR_UNIT, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegPStateDef0, PH_AMD_MSR_PSTATE_DEF0, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegCppcCapability1, 0xC00102B0, PhpHwScopeCore);
        PhpHwCpuSetRegister(PhpHwRegCppcEnable, 0xC00102B1, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegCppcRequest, 0xC00102B3, PhpHwScopeThread);
    }
    else if (PhpHwCpuClass == PhpHwCpuAmdLegacy)
    {
        PhpHwCpuSetRegister(PhpHwRegAperf, PH_INTEL_MSR_APERF, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegMperf, PH_INTEL_MSR_MPERF, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegPerfStatus, PH_AMD_MSR_COFVID_STATUS, PhpHwScopeCore);
        PhpHwPciQueryLegacyClock(&PhpHwCpuIdentity, &p0Definition, &PhpHwCpuClockControl);

        if (p0Definition)
            PhpHwCpuSetRegister(PhpHwRegPStateDef0, p0Definition, PhpHwScopePackage);

        if (PhpHwCpuFirstOfUnit)
        {
            PhpHwCpuSetRegister(PhpHwRegCuPowerAccumulator, PH_AMD_MSR_CU_PWR_ACCUMULATOR, PhpHwScopeComputeUnit);
            PhpHwCpuSetRegister(PhpHwRegPtsc, PH_AMD_MSR_PTSC, PhpHwScopeComputeUnit);
            PhpHwCpuSetRegister(PhpHwRegCuMaxPowerAccumulator, PH_AMD_MSR_CU_MAX_PWR_ACCUMULATOR, PhpHwScopeComputeUnit);
        }
    }
    else if (PhpHwCpuClass == PhpHwCpuArm64)
    {
        // System register codes are ARM64_SYSREG(op0, op1, CRn, CRm, op2), which only ARM64 builds define.
        PhpHwCpuSetRegister(PhpHwRegArmCounter, 0x5F02, PhpHwScopeThread); // CNTVCT_EL0
        PhpHwCpuSetRegister(PhpHwRegArmPmuCycles, 0x5CE8, PhpHwScopeThread); // PMCCNTR_EL0
        PhpHwCpuSetRegister(PhpHwRegArmFrequency, 0x5F00, PhpHwScopeThread); // CNTFRQ_EL0

        if (PhpHwQueryBuildNumber() >= PHP_HW_CPU_ARM_AMU_BUILD)
        {
            PhpHwCpuSetRegister(PhpHwRegArmCycles, 0x5EA0, PhpHwScopeThread); // AMEVCNTR00_EL0
            PhpHwCpuSetRegister(PhpHwRegArmConstantCycles, 0x5EA1, PhpHwScopeThread); // AMEVCNTR01_EL0
            PhpHwCpuSetRegister(PhpHwRegArmInstructions, 0x5EA2, PhpHwScopeThread); // AMEVCNTR02_EL0
            PhpHwCpuSetRegister(PhpHwRegArmMemoryStalls, 0x5EA3, PhpHwScopeThread); // AMEVCNTR03_EL0
            PhpHwCpuSetRegister(PhpHwRegArmAmuEnable, 0x5E95, PhpHwScopeThread); // AMCNTENSET0_EL0
        }
    }
    else if (PhpHwCpuClass == PhpHwCpuCentaur)
    {
        ULONG critical = PhCentaurCriticalTemperatureMsr(&PhpHwCpuIdentity);

        PhpHwCpuSetRegister(PhpHwRegAperf, PH_INTEL_MSR_APERF, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegMperf, PH_INTEL_MSR_MPERF, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegCentaurTemperature, PH_CENTAUR_MSR_TEMPERATURE, PhpHwScopeCore);

        if (critical)
            PhpHwCpuSetRegister(PhpHwRegTemperatureTarget, critical, PhpHwScopePackage);
    }
    else if (PhpHwCpuClass == PhpHwCpuIntel)
    {
        PhpHwCpuSetRegister(PhpHwRegAperf, PH_INTEL_MSR_APERF, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegMperf, PH_INTEL_MSR_MPERF, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegPerfStatus, PH_INTEL_MSR_PERF_STATUS, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegThermStatus, PH_INTEL_MSR_THERM_STATUS, PhpHwScopeCore);
        PhpHwCpuSetRegister(PhpHwRegPackageThermStatus, PH_INTEL_MSR_PACKAGE_THERM_STATUS, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegTemperatureTarget, PH_INTEL_MSR_TEMPERATURE_TARGET, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegPlatformInfo, PH_INTEL_MSR_PLATFORM_INFO, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegFsbFreq, PhIntelFsbFreqMsr(&PhpHwCpuIdentity), PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegPackageEnergy, PH_INTEL_MSR_PKG_ENERGY_STATUS, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegPp0Energy, PH_INTEL_MSR_PP0_ENERGY_STATUS, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegDramEnergy, PH_INTEL_MSR_DRAM_ENERGY_STATUS, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegPlatformEnergy, PH_INTEL_MSR_PLATFORM_ENERGY_STATUS, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegPowerLimit, PH_INTEL_MSR_PKG_POWER_LIMIT, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegLimitReasons, PhIntelLimitReasonsMsr(&PhpHwCpuIdentity), PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegPlatformPowerLimit, PH_INTEL_MSR_PLATFORM_POWER_LIMIT, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegUncorePerfStatus, PH_INTEL_MSR_UNCORE_PERF_STATUS, PhpHwScopePackage);

        PhpHwCpuSetRegister(PhpHwRegPmEnable, 0x770, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegHwpCapabilities, 0x771, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegHwpRequest, 0x774, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegEnergyPerfBias, 0x1B0, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegSmiCount, 0x34, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegTurboRatioLimit, PhIntelTurboRatioLimitMsr(&PhpHwCpuIdentity), PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegTurboRatioCores, PhIntelTurboRatioCoresMsr(&PhpHwCpuIdentity), PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegConfigTdpNominal, 0x648, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegConfigTdpLevel1, 0x649, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegConfigTdpLevel2, 0x64A, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegConfigTdpControl, 0x64B, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegTurboActivationRatio, 0x64C, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegPp1Energy, 0x641, PhpHwScopePackage);

        // A model without a state faults its counter, which leaves the plan at the probe.
        for (ULONG r = 0; r < RTL_NUMBER_OF(PhpHwCpuResidencyRegs); r++)
        {
            PhpHwCpuSetRegister(
                PhpHwCpuResidencyRegs[r].Reg,
                PhpHwCpuResidencyRegs[r].Msr,
                r < PHP_HW_CPU_CORE_STATES ? PhpHwScopeCore : PhpHwScopePackage
                );
        }

        if (PhIntelHasPl4(&PhpHwCpuIdentity))
            PhpHwCpuSetRegister(PhpHwRegVrCurrentConfig, PH_INTEL_MSR_VR_CURRENT_CONFIG, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegRaplUnit, PH_INTEL_MSR_RAPL_POWER_UNIT, PhpHwScopePackage);

        PhpHwCpuCoreReadoutMask = PhIntelThermReadoutMask(&PhpHwCpuIdentity, FALSE);
        PhpHwCpuPackageReadoutMask = PhIntelThermReadoutMask(&PhpHwCpuIdentity, TRUE);
    }
}

/**
 * Determines whether a register is read on a processor: every processor for thread scope, the first of its core
 * or package otherwise.
 */
static BOOLEAN PhpHwCpuRegisterOnProcessor(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Processor
    )
{
    switch (PhpHwCpuRegisters[Reg].Scope)
    {
    case PhpHwScopeCore:
        return PhpHwCpuFirstOfCore[Processor];
    case PhpHwScopePackage:
        return PhpHwCpuFirstOfPackage[Processor];
    case PhpHwScopeComputeUnit:
        return PhpHwCpuFirstOfUnit && PhpHwCpuFirstOfUnit[Processor];
    }

    return TRUE;
}

/**
 * Determines whether a register is in the plan of a processor: on ARM64 each kind of core has its own.
 */
static BOOLEAN PhpHwCpuRegisterUsable(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Processor
    )
{
    if (!PhpHwCpuRegisterActive(Reg))
        return FALSE;
    if (PhpHwCpuArmDisabled && (PhpHwCpuArmDisabled[Processor] & (1ULL << Reg)))
        return FALSE;

    return TRUE;
}

/**
 * Takes an ARM64 register out of the plan of a processor; out of the session's plan once no processor reads it.
 */
static VOID PhpHwCpuArmDisable(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Processor
    )
{
    PhpHwCpuArmDisabled[Processor] |= 1ULL << Reg;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        if (!(PhpHwCpuArmDisabled[i] & (1ULL << Reg)))
            return;
    }

    if (PhpHwCpuRegisterActive(Reg))
    {
        PhpHwCpuRegisters[Reg].Disabled = TRUE;
        PhTraceInfo("Hardware sensor register 0x%lx unavailable on every processor", PhpHwCpuRegisters[Reg].Register);
    }
}

/**
 * Determines the processor class from the identity the driver gave.
 */
static PHP_HW_CPU_CLASS PhpHwCpuClassify(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    if (PhCpuIdentityIsIntel(Identity))
        return PhpHwCpuIntel;
    if (PhCpuIdentityIsAmd(Identity) && Identity->DisplayFamily >= 0x17)
        return PhpHwCpuAmdZen;
    if (PhCpuIdentityIsAmd(Identity) && Identity->DisplayFamily >= 0x10)
        return PhpHwCpuAmdLegacy;
    if (PhCpuIdentityIsCentaur(Identity))
        return PhpHwCpuCentaur;

    return PhpHwCpuNone;
}

/**
 * Applies the session failure policy to an item: a register the script does not permit, or one that faults, leaves
 * the plan at once; an inactive processor, contention and "nothing this time" are skipped; anything else strikes.
 *
 * \return TRUE when the value is usable.
 */
static BOOLEAN PhpHwCpuApplyStatus(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Processor,
    _In_ NTSTATUS Status
    )
{
    PPHP_HW_REGISTER reg = &PhpHwCpuRegisters[Reg];

    if (NT_SUCCESS(Status))
    {
        reg->Strikes = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_INVALID_PARAMETER: // an inactive processor
    case STATUS_RETRY:
    case STATUS_DEVICE_NOT_READY:
        break;
    case STATUS_NOT_FOUND: // not permitted
    case STATUS_NOT_SUPPORTED: // a faulting register, or on ARM64 one the processor lacks
        if (PhpHwCpuArmDisabled)
        {
            PhpHwCpuArmDisable(Reg, Processor);
            break;
        }

        reg->Disabled = TRUE;
        PhTraceInfo("Hardware sensor register 0x%lx unavailable: 0x%lx", reg->Register, Status);
        break;
    default:
        if (++reg->Strikes >= PHP_HW_CPU_STRIKE_LIMIT)
        {
            reg->Disabled = TRUE;
            PhTraceInfo("Hardware sensor register 0x%lx unavailable: 0x%lx", reg->Register, Status);
        }
        break;
    }

    return FALSE;
}

static NTSTATUS PhpHwCpuReadItems(
    VOID
    );

/**
 * Stores the answered items of the last read in the samples, applying each item's status.
 */
static VOID PhpHwCpuStoreValues(
    VOID
    )
{
    for (ULONG i = 0; i < PhpHwCpuItemCount; i++)
    {
        PPHP_HW_CPU_TAG tag = &PhpHwCpuTags[i];

        if (tag->Unanswered || !PhpHwCpuApplyStatus(tag->Reg, tag->Processor, PhpHwCpuValues[i].Status))
            continue;

        PhpHwCpuSamples[tag->Processor].Value[tag->Reg] = PhpHwCpuValues[i].Value;
        PhpHwCpuSamples[tag->Processor].Valid |= 1ULL << tag->Reg;
    }
}

static VOID PhpHwCpuDecodeMsrPowerLimit(
    _In_ ULONG Package,
    _In_ ULONG64 PowerLimit
    )
{
    PhpHwCpuMsrPowerLimit1[Package] = (PowerLimit & 0x8000) ? (FLOAT)(PowerLimit & 0x7FFF) * PhpHwCpuPowerUnit : 0.0f;
    PhpHwCpuMsrPowerLimit2[Package] = (PowerLimit & (1ULL << 47)) ? (FLOAT)((PowerLimit >> 32) & 0x7FFF) * PhpHwCpuPowerUnit : 0.0f;
}

/**
 * Lists the named sensors of each package: PL4 and the platform PL1 and PL2 that the probe read shows set or
 * enabled, and the MSR uncore clock.
 */
static VOID PhpHwCpuProbeNamed(
    VOID
    )
{
    static CONST PCWSTR names[PHP_HW_CPU_LIMIT_SENSORS] = { L"PL4", L"Platform PL1", L"Platform PL2" };

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[PhpHwCpuPackageFirst[i]];
        BOOLEAN listed[PHP_HW_CPU_LIMIT_SENSORS] = { 0 };

        if (PhpHwCpuPowerUnit > 0.0f)
        {
            if (sample->Valid & (1ULL << PhpHwRegVrCurrentConfig))
                listed[PHP_HW_CPU_LIMIT_PL4] = !!(sample->Value[PhpHwRegVrCurrentConfig] & 0x1FFF);

            if (sample->Valid & (1ULL << PhpHwRegPlatformPowerLimit))
            {
                listed[PHP_HW_CPU_LIMIT_PLATFORM_PL1] = !!(sample->Value[PhpHwRegPlatformPowerLimit] & 0x8000);
                listed[PHP_HW_CPU_LIMIT_PLATFORM_PL2] = !!(sample->Value[PhpHwRegPlatformPowerLimit] & (1ULL << 47));
            }

            for (ULONG j = 0; j < PHP_HW_CPU_LIMIT_SENSORS; j++)
            {
                if (listed[j])
                    PhpHwCpuLimitIndex[i * PHP_HW_CPU_LIMIT_SENSORS + j] = PhpHwAddNamedSensor(PhHwNamedSensorPowerLimit, i, j, names[j]);
            }
        }

        if (PhpHwCpuRegisterActive(PhpHwRegUncorePerfStatus))
            PhpHwCpuUncoreIndex[i] = PhpHwAddNamedSensor(PhHwNamedSensorUncoreClock, i, PHP_HW_CPU_UNCORE_ID, L"Uncore");
    }
}

static BOOLEAN PhpHwCpuIsIntelNehalem(
    VOID
    )
{
    static CONST UCHAR models[] = { 0x1A, 0x1E, 0x1F, 0x25, 0x2C, 0x2E, 0x2F };

    if (PhpHwCpuIdentity.DisplayFamily != 0x06)
        return FALSE;

    for (ULONG i = 0; i < RTL_NUMBER_OF(models); i++)
    {
        if (PhpHwCpuIdentity.DisplayModel == models[i])
            return TRUE;
    }

    return FALSE;
}

/**
 * Reads the whole ARM64 plan once on every processor, since each kind of core has its own registers, and chooses
 * each processor's clock source: the AMU when both its counters answer and firmware enabled them, else the cycle
 * counter. Deltas start at the next update.
 */
static VOID PhpHwCpuProbeReadArm64(
    VOID
    )
{
    PhpHwCpuItemCount = 0;
    PhpHwCpuArmCounterHz = 0;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        for (ULONG reg = 0; reg < PhpHwRegMaximum; reg++)
        {
            if (!PhpHwCpuRegisters[reg].Register || PhpHwCpuItemCount >= PhpHwCpuCapacity)
                continue;

            PhpHwCpuItems[PhpHwCpuItemCount].Group = PhpHwCpuSensors[i].ProcessorNumber.Group;
            PhpHwCpuItems[PhpHwCpuItemCount].Number = PhpHwCpuSensors[i].ProcessorNumber.Number;
            PhpHwCpuItems[PhpHwCpuItemCount].Msr = PhpHwCpuRegisters[reg].Register;
            PhpHwCpuTags[PhpHwCpuItemCount].Reg = reg;
            PhpHwCpuTags[PhpHwCpuItemCount].Processor = i;
            PhpHwCpuTags[PhpHwCpuItemCount].Unanswered = FALSE;
            PhpHwCpuItemCount++;
        }
    }

    if (PhpHwCpuItemCount)
    {
        PhpHwCpuReadItems();
        PhpHwCpuStoreValues();
    }

    // Firmware programs CNTFRQ_EL0 once for the whole system.
    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        if ((PhpHwCpuSamples[i].Valid & (1ULL << PhpHwRegArmFrequency)) && PhpHwCpuSamples[i].Value[PhpHwRegArmFrequency])
        {
            PhpHwCpuArmCounterHz = PhpHwCpuSamples[i].Value[PhpHwRegArmFrequency];
            break;
        }
    }

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[i];

        // The driver never writes AMCNTENSET0_EL0.
        if (
            !(sample->Valid & (1ULL << PhpHwRegArmAmuEnable)) ||
            (sample->Value[PhpHwRegArmAmuEnable] & PHP_HW_CPU_ARM_AMU_COUNTERS) != PHP_HW_CPU_ARM_AMU_COUNTERS ||
            !PhpHwCpuRegisterUsable(PhpHwRegArmCycles, i) || !PhpHwCpuRegisterUsable(PhpHwRegArmConstantCycles, i)
            )
        {
            PhpHwCpuArmDisable(PhpHwRegArmCycles, i);
            PhpHwCpuArmDisable(PhpHwRegArmConstantCycles, i);
        }
        else
        {
            PhpHwCpuArmDisable(PhpHwRegArmPmuCycles, i);
        }

        if (
            !(sample->Valid & (1ULL << PhpHwRegArmAmuEnable)) ||
            (sample->Value[PhpHwRegArmAmuEnable] & PHP_HW_CPU_ARM_AMU_ACTIVITY) != PHP_HW_CPU_ARM_AMU_ACTIVITY ||
            !PhpHwCpuRegisterUsable(PhpHwRegArmInstructions, i) || !PhpHwCpuRegisterUsable(PhpHwRegArmMemoryStalls, i)
            )
        {
            PhpHwCpuArmDisable(PhpHwRegArmInstructions, i);
            PhpHwCpuArmDisable(PhpHwRegArmMemoryStalls, i);
        }

        sample->Valid = 0;
    }

    PhpHwCpuRegisters[PhpHwRegArmFrequency].Disabled = TRUE;
    PhpHwCpuRegisters[PhpHwRegArmAmuEnable].Disabled = !!PhpHwCpuRegisters[PhpHwRegArmAmuEnable].Register;

    // No ARM64 part has a package register: the package values are tsens's and spbm's.
    PhpHwCpuPackageCaps = 0;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PhpHwPackageSensors[i].TjMax = 0.0f;
        PhpHwPackageSensors[i].BaseMhz = 0.0f;
        PhpHwPackageSensors[i].BusMhz = 0.0f;
        PhpHwPackageSensors[i].PowerLimit = 0.0f;
    }
}

/**
 * Reads the whole plan once, the thread and core registers on the first processor and the package registers on the
 * first processor of each package, and decides from it what the plan keeps and what it decodes with.
 */
static VOID PhpHwCpuProbeRead(
    VOID
    )
{
    PPHP_HW_SAMPLE first = &PhpHwCpuSamples[0];
    ULONG64 value;
    FLOAT busMhz;
    BOOLEAN anyTjMax = FALSE;
    BOOLEAN raplUnitValid;

    PhpHwCpuItemCount = 0;
    PhpHwCpuTscMhz = 0.0f;
    PhpHwCpuEnergyUnit = 0.0f;
    PhpHwCpuVoltageSupported = FALSE;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        for (ULONG reg = 0; reg < PhpHwRegMaximum; reg++)
        {
            BOOLEAN package = PhpHwCpuRegisters[reg].Scope == PhpHwScopePackage;

            if (!PhpHwCpuRegisters[reg].Register || PhpHwCpuItemCount >= PhpHwCpuCapacity)
                continue;

            if (PhpHwCpuRegisters[reg].Scope == PhpHwScopeCore && PhpHwCpuProbeOnly(reg))
            {
                if (!PhpHwCpuFirstOfCore[i])
                    continue;
            }
            else if (package ? !PhpHwCpuFirstOfPackage[i] : i != 0)
            {
                continue;
            }

            PhpHwCpuItems[PhpHwCpuItemCount].Group = PhpHwCpuSensors[i].ProcessorNumber.Group;
            PhpHwCpuItems[PhpHwCpuItemCount].Number = PhpHwCpuSensors[i].ProcessorNumber.Number;
            PhpHwCpuItems[PhpHwCpuItemCount].Msr = PhpHwCpuRegisters[reg].Register;
            PhpHwCpuTags[PhpHwCpuItemCount].Reg = reg;
            PhpHwCpuTags[PhpHwCpuItemCount].Processor = i;
            PhpHwCpuTags[PhpHwCpuItemCount].Unanswered = FALSE;
            PhpHwCpuItemCount++;
        }
    }

    if (PhpHwCpuItemCount)
    {
        PhpHwCpuReadItems();
        PhpHwCpuStoreValues();
    }

    for (ULONG k = 0; k < RTL_NUMBER_OF(PhpHwCpuPolicy); k++)
    {
        PHP_HW_REG reg = PhpHwRegPmEnable + k;

        if (PhpHwCpuClass == PhpHwCpuIntel && (first->Valid & (1ULL << reg)))
        {
            PhpHwCpuPolicy[k] = first->Value[reg];
            PhpHwCpuPolicyValid |= 1UL << k;
        }
    }

    if (PhpHwCpuClass == PhpHwCpuIntel)
        PhpHwCpuOcValid = NT_SUCCESS(KphHwOcMailboxRead(PhpHwCpuOc));

    for (ULONG k = 0; k < RTL_NUMBER_OF(PhpHwCpuCppc); k++)
    {
        PHP_HW_REG reg = PhpHwRegCppcCapability1 + k;

        if (PhpHwCpuClass == PhpHwCpuAmdZen && (first->Valid & (1ULL << reg)))
        {
            PhpHwCpuCppc[k] = first->Value[reg];
            PhpHwCpuCppcValid |= 1UL << k;
        }
    }

    for (ULONG c = 0; PhpHwCpuClass == PhpHwCpuAmdZen && c < PhpHwCpuCoreCount; c++)
    {
        ULONG processor = PhpHwCpuCoreFirst[c];

        if (processor != ULONG_MAX && (PhpHwCpuSamples[processor].Valid & (1ULL << PhpHwRegCppcCapability1)))
            PhpHwCpuCppcHighest[c] = (UCHAR)(PhpHwCpuSamples[processor].Value[PhpHwRegCppcCapability1] >> 24);
    }

    // The read-only APERF and MPERF aliases may fault or read zero under a hypervisor.
    if (PhpHwCpuClass == PhpHwCpuAmdZen && (
        !(first->Valid & (1ULL << PhpHwRegAperf)) || first->Value[PhpHwRegAperf] == 0 || !PhpHwCpuRegisterActive(PhpHwRegMperf)
        ))
    {
        PhpHwCpuSetRegister(PhpHwRegAperf, PH_INTEL_MSR_APERF, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegMperf, PH_INTEL_MSR_MPERF, PhpHwScopeThread);
    }

    if (raplUnitValid = !!(PhpHwCpuSamples[PhpHwCpuPackageFirst[0]].Valid & (1ULL << PhpHwRegRaplUnit)))
    {
        ULONG64 raplUnit = PhpHwCpuSamples[PhpHwCpuPackageFirst[0]].Value[PhpHwRegRaplUnit];

        PhpHwCpuEnergyUnit = PhRaplEnergyUnitJoules(raplUnit, &PhpHwCpuIdentity);

        if (PhpHwCpuClass == PhpHwCpuIntel)
        {
            PhpHwCpuDramEnergyUnit = PhIntelRaplDramEnergyUnitJoules(raplUnit, &PhpHwCpuIdentity);
            PhpHwCpuPlatformEnergyUnit = PhIntelRaplPlatformEnergyUnitJoules(raplUnit, &PhpHwCpuIdentity);
            PhpHwCpuPowerUnit = PhRaplPowerUnitWatts(raplUnit, &PhpHwCpuIdentity);
        }
    }

    // Silvermont and Airmont select the bus clock in MSR_FSB_FREQ; undecoded, the base clock stays unknown.
    busMhz = (PhpHwCpuClass == PhpHwCpuIntel && PhpHwCpuIsIntelNehalem()) ? 133.33f : PHP_HW_CPU_BUS_MHZ;

    if (PhpHwCpuRegisters[PhpHwRegFsbFreq].Register)
    {
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[PhpHwCpuPackageFirst[0]];

        busMhz = (sample->Valid & (1ULL << PhpHwRegFsbFreq)) ? PhIntelFsbFreqToMhz(sample->Value[PhpHwRegFsbFreq], &PhpHwCpuIdentity) : 0.0f;
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[PhpHwCpuPackageFirst[i]];
        FLOAT baseMhz = 0.0f;
        FLOAT powerLimit = 0.0f;
        ULONG tjMax = 0;

        if (PhpHwCpuClass == PhpHwCpuIntel)
        {
            if (sample->Valid & (1ULL << PhpHwRegTemperatureTarget))
            {
                tjMax = PhIntelTemperatureTargetTjMax(sample->Value[PhpHwRegTemperatureTarget]);

                if (i == 0)
                    PhpHwCpuTccOffset = (ULONG)((sample->Value[PhpHwRegTemperatureTarget] >> 24) & 0x3F);
            }
            if (!tjMax && (PhpHwCpuRegisterActive(PhpHwRegThermStatus) || PhpHwCpuRegisterActive(PhpHwRegPackageThermStatus)))
                tjMax = PhIntelFixedTjMax(&PhpHwCpuIdentity);
            if (sample->Valid & (1ULL << PhpHwRegPlatformInfo))
                baseMhz = (FLOAT)((sample->Value[PhpHwRegPlatformInfo] >> 8) & 0xFF) * busMhz;
            if (raplUnitValid && (sample->Valid & (1ULL << PhpHwRegPowerLimit)))
            {
                powerLimit = (FLOAT)(sample->Value[PhpHwRegPowerLimit] & 0x7FFF) * PhpHwCpuPowerUnit;
                PhpHwCpuDecodeMsrPowerLimit(i, sample->Value[PhpHwRegPowerLimit]);
            }
        }
        else if (PhpHwCpuClass == PhpHwCpuAmdZen || PhpHwCpuClass == PhpHwCpuAmdLegacy)
        {
            // MPERF counts at the P0 frequency.
            if (sample->Valid & (1ULL << PhpHwRegPStateDef0))
                baseMhz = PhAmdPStateDefToMhz(sample->Value[PhpHwRegPStateDef0], &PhpHwCpuIdentity, PhpHwCpuClockControl);
        }
        else if (PhpHwCpuClass == PhpHwCpuCentaur)
        {
            // Shown only: the temperature register reads degrees itself.
            if (sample->Valid & (1ULL << PhpHwRegTemperatureTarget))
                tjMax = (ULONG)(sample->Value[PhpHwRegTemperatureTarget] & 0xFF);
        }

        if (i == 0)
            PhpHwCpuTscMhz = baseMhz;
        if (tjMax)
            anyTjMax = TRUE;

        PhpHwCpuTjMax[i] = tjMax;
        PhpHwPackageSensors[i].TjMax = (FLOAT)tjMax;
        PhpHwPackageSensors[i].BaseMhz = baseMhz;
        PhpHwPackageSensors[i].BusMhz = busMhz;
        PhpHwPackageSensors[i].PowerLimit = powerLimit;
    }

    if (first->Valid & (1ULL << PhpHwRegPerfStatus))
    {
        UCHAR vid;

        value = first->Value[PhpHwRegPerfStatus];

        if (PhpHwCpuClass == PhpHwCpuIntel)
            PhpHwCpuVoltageSupported = PhIntelPerfStatusToVolts(value, &PhpHwCpuIdentity) > 0.0f;
        else
            PhpHwCpuVoltageSupported = PhAmdPStateVid(value, &PhpHwCpuIdentity, &vid) && PhAmdCpuVidToVolts(vid, &PhpHwCpuIdentity) > 0.0f;
    }

    if (!anyTjMax)
    {
        PhpHwCpuRegisters[PhpHwRegThermStatus].Disabled = TRUE;
        PhpHwCpuRegisters[PhpHwRegPackageThermStatus].Disabled = TRUE;
    }

    PhpHwCpuPackageCaps = PHP_HW_CPU_PACKAGE_CAPS;

    if (PhpHwCpuRegisterActive(PhpHwRegThermStatus) || PhpHwCpuRegisterActive(PhpHwRegPackageThermStatus))
        PhpHwCpuPackageCaps |= PHP_HW_CPU_THERMAL_CAPS;
    if (PhpHwCpuRegisterActive(PhpHwRegLimitReasons))
        PhpHwCpuPackageCaps |= PH_HW_CAP_THROTTLE;
    if (PhpHwCpuRegisterActive(PhpHwRegCentaurTemperature))
        PhpHwCpuPackageCaps |= PH_HW_CAP_CORE_TEMP | PH_HW_CAP_PACKAGE_TEMP;

    PhpHwCpuProbeNamed();

    // MPERF is only needed for the active clock; APERF alone yields the effective clock.
    if (PhpHwCpuTscMhz <= 0.0f)
        PhpHwCpuRegisters[PhpHwRegMperf].Disabled = TRUE;

    if (PhpHwCpuEnergyUnit <= 0.0f)
    {
        PhpHwCpuRegisters[PhpHwRegPackageEnergy].Disabled = TRUE;
        PhpHwCpuRegisters[PhpHwRegPp0Energy].Disabled = TRUE;
        PhpHwCpuRegisters[PhpHwRegCoreEnergy].Disabled = TRUE;
    }

    // Every compute unit wraps at the same maximum.
    PhpHwCpuUnitMaxAccumulator = (first->Valid & (1ULL << PhpHwRegCuMaxPowerAccumulator)) ? first->Value[PhpHwRegCuMaxPowerAccumulator] : 0;

    if (!PhpHwCpuUnitMaxAccumulator)
    {
        PhpHwCpuRegisters[PhpHwRegCuPowerAccumulator].Disabled = TRUE;
        PhpHwCpuRegisters[PhpHwRegPtsc].Disabled = TRUE;
    }

    if (PhpHwCpuDramEnergyUnit <= 0.0f)
        PhpHwCpuRegisters[PhpHwRegDramEnergy].Disabled = TRUE;
    if (PhpHwCpuPlatformEnergyUnit <= 0.0f)
        PhpHwCpuRegisters[PhpHwRegPlatformEnergy].Disabled = TRUE;
}

/**
 * Converts the change of a 32-bit energy counter over the sample interval to watts.
 */
_Success_(return)
static BOOLEAN PhpHwCpuSampleWatts(
    _In_ PPHP_HW_SAMPLE Sample,
    _In_ PHP_HW_REG Reg,
    _In_ FLOAT EnergyUnit,
    _In_ FLOAT Seconds,
    _Out_ PFLOAT Watts
    )
{
    FLOAT watts;

    if (Seconds <= 0.0f || EnergyUnit <= 0.0f)
        return FALSE;
    if (!(Sample->Valid & Sample->LastValid & (1ULL << Reg)))
        return FALSE;

    watts = (FLOAT)(ULONG)(Sample->Value[Reg] - Sample->LastValue[Reg]) * EnergyUnit / Seconds;

    if (watts > PHP_HW_CPU_ENERGY_MAX_WATTS)
        return FALSE;

    *Watts = watts;
    return TRUE;
}

/**
 * Finds the first processor of each compute unit of an AMD family 15h or 16h processor; without them the compute-unit
 * power is not planned.
 */
static VOID PhpHwCpuQueryComputeUnits(
    VOID
    )
{
    PHP_HW_TOPOLOGY topology;

    topology.ProcessorCount = PhpHwProcessorCount;
    topology.CoreCount = 0;
    topology.PackageCount = PhpHwPackageCount;
    topology.Processors = PhAllocate(sizeof(PHP_HW_PROCESSOR) * PhpHwProcessorCount);

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        topology.Processors[i].Number = PhpHwCpuSensors[i].ProcessorNumber;
        topology.Processors[i].CoreIndex = PhpHwCpuSensors[i].CoreIndex;
        topology.Processors[i].PackageIndex = PhpHwCpuSensors[i].PackageIndex;
    }

    PhpHwCpuFirstOfUnit = PhAllocate(sizeof(BOOLEAN) * PhpHwProcessorCount);

    if (!PhpHwQueryComputeUnits(&topology, PhpHwCpuFirstOfUnit, &PhpHwCpuUnitSampleRatio))
    {
        PhFree(PhpHwCpuFirstOfUnit);
        PhpHwCpuFirstOfUnit = NULL;
        PhpHwCpuUnitSampleRatio = 0;
    }

    PhFree(topology.Processors);
}

/**
 * Identifies the processor of the first package and builds the plan. Called once a session.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED for a machine or processor the collector does not serve, or the
 * status of the identify call.
 */
NTSTATUS PhpHwCpuProbe(
    VOID
    )
{
    PPH_HW_CPU_SENSOR first;
    KPH_HW_CPU_IDENTITY identity;
    NTSTATUS status;
    ULONG lastCore = ULONG_MAX;
    ULONG lastPackage = ULONG_MAX;

    if (!(first = PhHwGetCpuSensor(0)))
        return STATUS_NOT_SUPPORTED;

    status = KphHwCpuIdentify(first->ProcessorNumber.Group, first->ProcessorNumber.Number, &identity);

    if (!NT_SUCCESS(status))
        return status;

    // On ARM64 identify gives only the MIDR, and each kind of core has its own.
    if (PhpHwQueryMachine() == IMAGE_FILE_MACHINE_ARM64)
    {
        PhpHwCpuArmMidr = PhAllocateZero(sizeof(ULONG) * PhpHwProcessorCount);
        PhpHwCpuArmDisabled = PhAllocateZero(sizeof(ULONG64) * PhpHwProcessorCount);
        PhpHwCpuArmStalls = PhAllocateZero(sizeof(UCHAR) * PhpHwProcessorCount);
        PhpHwCpuArmActivityIndex = PhAllocate(sizeof(ULONG) * 2 * PhpHwProcessorCount);
        memset(PhpHwCpuArmActivityIndex, 0xFF, sizeof(ULONG) * 2 * PhpHwProcessorCount);
        PhpHwCpuArmMidr[0] = identity.Signature;

        for (ULONG i = 1; i < PhpHwProcessorCount; i++)
        {
            KPH_HW_CPU_IDENTITY other;

            if (NT_SUCCESS(KphHwCpuIdentify(PhpHwCpuSensors[i].ProcessorNumber.Group, PhpHwCpuSensors[i].ProcessorNumber.Number, &other)))
                PhpHwCpuArmMidr[i] = other.Signature;
        }

        memset(&identity, 0, sizeof(identity));
        identity.Signature = PhpHwCpuArmMidr[0];
    }
    else
    {
        ULONG lastIdentified = 0;

        // The plan follows the first package; the report names another that differs.
        PhpHwCpuPackageIdentity = PhAllocateZero(sizeof(KPH_HW_CPU_IDENTITY) * max(PhpHwPackageCount, 1));
        PhpHwCpuPackageIdentity[0] = identity;

        for (ULONG i = 1; i < PhpHwProcessorCount; i++)
        {
            ULONG package = PhpHwCpuSensors[i].PackageIndex;

            if (package == lastIdentified || package >= PhpHwPackageCount)
                continue;

            lastIdentified = package;
            KphHwCpuIdentify(PhpHwCpuSensors[i].ProcessorNumber.Group, PhpHwCpuSensors[i].ProcessorNumber.Number, &PhpHwCpuPackageIdentity[package]);
        }
    }

    memset(&PhpHwCpuIdentity, 0, sizeof(PH_CPU_IDENTITY));
    memcpy(PhpHwCpuIdentity.Vendor, identity.Vendor, sizeof(identity.Vendor));
    PhpHwCpuIdentity.DisplayFamily = (USHORT)identity.Family;
    PhpHwCpuIdentity.DisplayModel = (UCHAR)identity.Model;
    PhpHwCpuIdentity.Stepping = (UCHAR)identity.Stepping;
    PhpHwCpuIdentity.Hybrid = !!identity.Hybrid;
    PhpHwCpuSignature = identity.Signature;

    if (PhpHwCpuArmMidr)
        PhpHwCpuClass = PhpHwCpuArm64;
    else if ((PhpHwCpuClass = PhpHwCpuClassify(&PhpHwCpuIdentity)) == PhpHwCpuNone)
        return STATUS_NOT_SUPPORTED;

    if (PhpHwCpuClass == PhpHwCpuAmdLegacy && (PhpHwCpuIdentity.DisplayFamily == 0x15 || PhpHwCpuIdentity.DisplayFamily == 0x16))
        PhpHwCpuQueryComputeUnits();

    PhpHwCpuInitializePlan();

    PhpHwCpuSamples = PhAllocateZero(sizeof(PHP_HW_SAMPLE) * PhpHwProcessorCount);
    PhpHwCpuFirstOfCore = PhAllocateZero(sizeof(BOOLEAN) * PhpHwProcessorCount);
    PhpHwCpuFirstOfPackage = PhAllocateZero(sizeof(BOOLEAN) * PhpHwProcessorCount);

    // processors of a core or a package are listed together
    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        PhpHwCpuFirstOfCore[i] = PhpHwCpuSensors[i].CoreIndex != lastCore;
        PhpHwCpuFirstOfPackage[i] = PhpHwCpuSensors[i].PackageIndex != lastPackage;
        lastCore = PhpHwCpuSensors[i].CoreIndex;
        lastPackage = PhpHwCpuSensors[i].PackageIndex;
        PhpHwCpuCoreCount = max(PhpHwCpuCoreCount, PhpHwCpuSensors[i].CoreIndex + 1);
    }

    PhpHwCpuCoreFirst = PhAllocate(sizeof(ULONG) * PhpHwCpuCoreCount);
    PhpHwCpuCppcHighest = PhAllocateZero(sizeof(UCHAR) * max(PhpHwCpuCoreCount, 1));
    PhpHwCpuPackageFirst = PhAllocate(sizeof(ULONG) * max(PhpHwPackageCount, 1));
    PhpHwCpuPackageCores = PhAllocateZero(sizeof(ULONG) * max(PhpHwPackageCount, 1));
    PhpHwCpuTickCorePower = PhAllocateZero(sizeof(FLOAT) * max(PhpHwPackageCount, 1));
    PhpHwCpuTickCoreCount = PhAllocateZero(sizeof(ULONG) * max(PhpHwPackageCount, 1));
    PhpHwCpuTjMax = PhAllocateZero(sizeof(ULONG) * max(PhpHwPackageCount, 1));
    PhpHwCpuTickValid = PhAllocateZero(sizeof(ULONG) * max(PhpHwPackageCount, 1));
    PhpHwCpuTickThrottle = PhAllocateZero(sizeof(ULONG) * max(PhpHwPackageCount, 1));
    PhpHwCpuTickMaxCore = PhAllocateZero(sizeof(FLOAT) * max(PhpHwPackageCount, 1));
    PhpHwCpuMsrPowerLimit1 = PhAllocateZero(sizeof(FLOAT) * max(PhpHwPackageCount, 1));
    PhpHwCpuMsrPowerLimit2 = PhAllocateZero(sizeof(FLOAT) * max(PhpHwPackageCount, 1));
    PhpHwCpuPackageIdleTicks = PhAllocateZero(sizeof(UCHAR) * max(PhpHwPackageCount, 1));
    PhpHwCpuPp0IdleTicks = PhAllocateZero(sizeof(UCHAR) * max(PhpHwPackageCount, 1));
    PhpHwCpuDramIdleTicks = PhAllocateZero(sizeof(UCHAR) * max(PhpHwPackageCount, 1));
    PhpHwCpuPlatformIdleTicks = PhAllocateZero(sizeof(UCHAR) * max(PhpHwPackageCount, 1));
    PhpHwCpuPackageEnergySeen = PhAllocateZero(sizeof(BOOLEAN) * max(PhpHwPackageCount, 1));
    PhpHwCpuPp0EnergySeen = PhAllocateZero(sizeof(BOOLEAN) * max(PhpHwPackageCount, 1));
    PhpHwCpuDramEnergySeen = PhAllocateZero(sizeof(BOOLEAN) * max(PhpHwPackageCount, 1));
    PhpHwCpuPlatformEnergySeen = PhAllocateZero(sizeof(BOOLEAN) * max(PhpHwPackageCount, 1));
    PhpHwCpuUncoreIndex = PhAllocate(sizeof(ULONG) * max(PhpHwPackageCount, 1));
    PhpHwCpuLimitIndex = PhAllocate(sizeof(ULONG) * PHP_HW_CPU_LIMIT_SENSORS * max(PhpHwPackageCount, 1));
    PhpHwCpuPp1Index = PhAllocate(sizeof(ULONG) * max(PhpHwPackageCount, 1));
    memset(PhpHwCpuPp1Index, 0xFF, sizeof(ULONG) * max(PhpHwPackageCount, 1));
    PhpHwCpuResidencyIndex = PhAllocate(sizeof(ULONG) * (PhpHwCpuCoreCount * PHP_HW_CPU_CORE_STATES + PhpHwPackageCount * PHP_HW_CPU_PACKAGE_STATES + 1));
    memset(PhpHwCpuResidencyIndex, 0xFF, sizeof(ULONG) * (PhpHwCpuCoreCount * PHP_HW_CPU_CORE_STATES + PhpHwPackageCount * PHP_HW_CPU_PACKAGE_STATES + 1));
    memset(PhpHwCpuUncoreIndex, 0xFF, sizeof(ULONG) * max(PhpHwPackageCount, 1));
    memset(PhpHwCpuLimitIndex, 0xFF, sizeof(ULONG) * PHP_HW_CPU_LIMIT_SENSORS * max(PhpHwPackageCount, 1));
    memset(PhpHwCpuCoreFirst, 0xFF, sizeof(ULONG) * PhpHwCpuCoreCount);
    memset(PhpHwCpuPackageFirst, 0, sizeof(ULONG) * max(PhpHwPackageCount, 1));

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        if (PhpHwCpuFirstOfCore[i])
        {
            PhpHwCpuCoreFirst[PhpHwCpuSensors[i].CoreIndex] = i;

            if (PhpHwCpuSensors[i].PackageIndex < PhpHwPackageCount)
                PhpHwCpuPackageCores[PhpHwCpuSensors[i].PackageIndex]++;
        }

        if (PhpHwCpuFirstOfPackage[i] && PhpHwCpuSensors[i].PackageIndex < PhpHwPackageCount)
            PhpHwCpuPackageFirst[PhpHwCpuSensors[i].PackageIndex] = i;
    }

    PhpHwCpuCapacity = 0;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        for (ULONG reg = 0; reg < PhpHwRegMaximum; reg++)
        {
            if (PhpHwCpuRegisters[reg].Register && PhpHwCpuRegisterOnProcessor(reg, i))
                PhpHwCpuCapacity++;
        }
    }

    PhpHwCpuItems = PhAllocate(sizeof(KPH_HW_CPU_ITEM) * max(PhpHwCpuCapacity, 1));
    PhpHwCpuValues = PhAllocate(sizeof(KPH_HW_VALUE64) * max(PhpHwCpuCapacity, 1));
    PhpHwCpuTags = PhAllocate(sizeof(PHP_HW_CPU_TAG) * max(PhpHwCpuCapacity, 1));
    PhpHwCpuBatch = PHP_HW_CPU_BATCH;
    PhpHwCpuSampleTime = 0;
    PhpHwCpuLastSampleTime = 0;

    if (PhpHwCpuClass == PhpHwCpuArm64)
        PhpHwCpuProbeReadArm64();
    else
        PhpHwCpuProbeRead();

    return STATUS_SUCCESS;
}

/**
 * Reads the items from Start in calls of at most PhpHwCpuBatch items, cut between processors. A call that meets the
 * script's limits halves the cut for the session and the rest is read again, once.
 *
 * \return The status of the last call, STATUS_RETRY when every call was told to retry.
 */
static NTSTATUS PhpHwCpuReadItems(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN retried = FALSE;
    BOOLEAN answered = FALSE;
    ULONG start = 0;

    while (start < PhpHwCpuItemCount)
    {
        ULONG end = start;

        // whole processors up to the cut; at least one processor
        while (end < PhpHwCpuItemCount)
        {
            ULONG next = end;
            ULONG processor = PhpHwCpuTags[end].Processor;

            while (next < PhpHwCpuItemCount && PhpHwCpuTags[next].Processor == processor)
                next++;

            if (end > start && next - start > PhpHwCpuBatch)
                break;

            end = next;
        }

        status = KphHwCpuRead(&PhpHwCpuItems[start], end - start, &PhpHwCpuValues[start]);

        if ((status == STATUS_SI_SCRIPT_LIMIT || status == STATUS_INSUFFICIENT_RESOURCES) && !retried)
        {
            ULONG processorItems = 0;

            for (ULONG i = start; i < PhpHwCpuItemCount && PhpHwCpuTags[i].Processor == PhpHwCpuTags[start].Processor; i++)
                processorItems++;

            if (PhpHwCpuBatch / 2 >= processorItems)
            {
                PhpHwCpuBatch /= 2;
                retried = TRUE;
                PhTraceInfo("Hardware sensors: processor reads cut to %lu items", PhpHwCpuBatch);
                continue;
            }
        }

        if (!NT_SUCCESS(status))
        {
            for (ULONG i = start; i < end; i++)
                PhpHwCpuTags[i].Unanswered = TRUE;
        }
        else
        {
            answered = TRUE;
        }

        start = end;
    }

    if (!answered && status == STATUS_RETRY)
        return STATUS_RETRY;

    return answered ? STATUS_SUCCESS : status;
}

static ULONG PhpHwCpuDecodeThrottle(
    _In_ ULONG64 ThermStatus
    )
{
    ULONG flags = 0;

    if (ThermStatus & (1ULL << 0))
        flags |= PH_HW_THROTTLE_THERMAL;
    if (ThermStatus & (1ULL << 2))
        flags |= PH_HW_THROTTLE_PROCHOT;
    if (ThermStatus & (1ULL << 4))
        flags |= PH_HW_THROTTLE_CRITICAL;
    if (ThermStatus & (1ULL << 10))
        flags |= PH_HW_THROTTLE_POWER_LIMIT;

    return flags;
}

static ULONG PhpHwCpuDecodeLimitReasons(
    _In_ ULONG64 LimitReasons
    )
{
    static CONST struct { UCHAR Bit; ULONG Flag; } skylake[] =
    {
        { 0, PH_HW_THROTTLE_PROCHOT }, { 1, PH_HW_THROTTLE_THERMAL }, { 4, PH_HW_THROTTLE_RESIDENCY },
        { 5, PH_HW_THROTTLE_RUNNING_AVERAGE_THERMAL }, { 6, PH_HW_THROTTLE_VR_THERMAL }, { 7, PH_HW_THROTTLE_VR_CURRENT },
        { 8, PH_HW_THROTTLE_ELECTRICAL }, { 10, PH_HW_THROTTLE_PL1 }, { 11, PH_HW_THROTTLE_PL2 },
        { 12, PH_HW_THROTTLE_MAX_TURBO }, { 13, PH_HW_THROTTLE_TURBO_ATTENUATION },
    };
    static CONST struct { UCHAR Bit; ULONG Flag; } haswell[] =
    {
        { 0, PH_HW_THROTTLE_PROCHOT }, { 1, PH_HW_THROTTLE_THERMAL }, { 4, PH_HW_THROTTLE_GRAPHICS },
        { 5, PH_HW_THROTTLE_AUTONOMOUS }, { 6, PH_HW_THROTTLE_VR_THERMAL }, { 8, PH_HW_THROTTLE_ELECTRICAL },
        { 9, PH_HW_THROTTLE_POWER_LIMIT }, { 10, PH_HW_THROTTLE_PL1 }, { 11, PH_HW_THROTTLE_PL2 },
        { 12, PH_HW_THROTTLE_MAX_TURBO }, { 13, PH_HW_THROTTLE_TURBO_ATTENUATION },
    };
    ULONG flags = 0;

    if (PhpHwCpuRegisters[PhpHwRegLimitReasons].Register == PH_INTEL_MSR_HSW_CORE_PERF_LIMIT_REASONS)
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(haswell); i++)
        {
            if (LimitReasons & (1ULL << haswell[i].Bit))
                flags |= haswell[i].Flag;
        }
    }
    else
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(skylake); i++)
        {
            if (LimitReasons & (1ULL << skylake[i].Bit))
                flags |= skylake[i].Flag;
        }
    }

    return flags;
}

/**
 * Drops the RAPL energy registers when no package's counter moved in PHP_HW_CPU_ENERGY_IDLE_TICKS samples.
 */
static VOID PhpHwCpuDropIdleEnergy(
    VOID
    )
{
    BOOLEAN packageIdle = PhpHwCpuRegisterActive(PhpHwRegPackageEnergy) && PhpHwPackageCount;
    BOOLEAN pp0Idle = PhpHwCpuRegisterActive(PhpHwRegPp0Energy) && PhpHwPackageCount;
    BOOLEAN dramIdle = PhpHwCpuRegisterActive(PhpHwRegDramEnergy) && PhpHwPackageCount;
    BOOLEAN platformIdle = PhpHwCpuRegisterActive(PhpHwRegPlatformEnergy) && PhpHwPackageCount;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        packageIdle &= !PhpHwCpuPackageEnergySeen[i] && PhpHwCpuPackageIdleTicks[i] >= PHP_HW_CPU_ENERGY_IDLE_TICKS;
        pp0Idle &= !PhpHwCpuPp0EnergySeen[i] && PhpHwCpuPp0IdleTicks[i] >= PHP_HW_CPU_ENERGY_IDLE_TICKS;
        dramIdle &= !PhpHwCpuDramEnergySeen[i] && PhpHwCpuDramIdleTicks[i] >= PHP_HW_CPU_ENERGY_IDLE_TICKS;
        platformIdle &= !PhpHwCpuPlatformEnergySeen[i] && PhpHwCpuPlatformIdleTicks[i] >= PHP_HW_CPU_ENERGY_IDLE_TICKS;
    }

    if (packageIdle)
    {
        PhTraceInfo("Hardware sensors: RAPL package energy does not count; dropped");
        PhpHwCpuRegisters[PhpHwRegPackageEnergy].Disabled = TRUE;
    }

    if (pp0Idle)
    {
        PhTraceInfo("Hardware sensors: RAPL PP0 energy does not count; dropped");
        PhpHwCpuRegisters[PhpHwRegPp0Energy].Disabled = TRUE;
    }

    if (dramIdle)
    {
        PhTraceInfo("Hardware sensors: RAPL DRAM energy does not count; dropped");
        PhpHwCpuRegisters[PhpHwRegDramEnergy].Disabled = TRUE;
    }

    if (platformIdle)
    {
        PhTraceInfo("Hardware sensors: RAPL platform energy does not count; dropped");
        PhpHwCpuRegisters[PhpHwRegPlatformEnergy].Disabled = TRUE;
    }
}

/**
 * Decodes the pass and publishes it: the values first, then their bits.
 */
/**
 * Sums the accumulator power of the compute units of an AMD family 15h or 16h package over the sample interval.
 *
 * \return FALSE unless every compute unit of the package has two samples.
 */
_Success_(return)
static BOOLEAN PhpHwCpuComputeUnitPower(
    _In_ ULONG Package,
    _Out_ PFLOAT Watts
    )
{
    FLOAT total = 0.0f;
    BOOLEAN found = FALSE;

    if (!PhpHwCpuFirstOfUnit || !PhpHwCpuRegisterActive(PhpHwRegCuPowerAccumulator) || !PhpHwCpuRegisterActive(PhpHwRegPtsc))
        return FALSE;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[i];
        ULONG64 both = (1ULL << PhpHwRegCuPowerAccumulator) | (1ULL << PhpHwRegPtsc);
        ULONG64 current;
        ULONG64 last;

        if (!PhpHwCpuFirstOfUnit[i] || PhpHwCpuSensors[i].PackageIndex != Package)
            continue;
        if ((sample->Valid & sample->LastValid & both) != both)
            return FALSE;

        // The accumulator wraps at CU_MAX_PWR_ACCUMULATOR.
        current = sample->Value[PhpHwRegCuPowerAccumulator];
        last = sample->LastValue[PhpHwRegCuPowerAccumulator];
        total += PhAmdComputeUnitPowerToWatts(
            current >= last ? current - last : PhpHwCpuUnitMaxAccumulator + current - last,
            sample->Value[PhpHwRegPtsc] - sample->LastValue[PhpHwRegPtsc],
            PhpHwCpuUnitSampleRatio
            );
        found = TRUE;
    }

    *Watts = total;
    return found;
}

/**
 * Publishes an ARM64 processor's instructions per cycle and memory stall share from its AMU, as named sensors added
 * the first time they have a value.
 *
 * \param Index The flat processor index.
 * \param Cycles The processor's CPU_CYCLES over the update.
 */
static VOID PhpHwCpuArmActivity(
    _In_ ULONG Index,
    _In_ ULONG64 Cycles
    )
{
    PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[Index];
    ULONG64 mask = (1ULL << PhpHwRegArmInstructions) | (1ULL << PhpHwRegArmMemoryStalls);
    PULONG ipcIndex = &PhpHwCpuArmActivityIndex[Index * 2];
    PULONG stallIndex = &PhpHwCpuArmActivityIndex[Index * 2 + 1];
    ULONG64 instructions;
    ULONG64 stalls;

    if (
        !Cycles ||
        !PhpHwCpuRegisterUsable(PhpHwRegArmInstructions, Index) ||
        (sample->Valid & sample->LastValid & mask) != mask
        )
    {
        PhpHwSetNamedSensor(*ipcIndex, FALSE, 0.0f);
        PhpHwSetNamedSensor(*stallIndex, FALSE, 0.0f);
        return;
    }

    instructions = sample->Value[PhpHwRegArmInstructions] - sample->LastValue[PhpHwRegArmInstructions];
    stalls = sample->Value[PhpHwRegArmMemoryStalls] - sample->LastValue[PhpHwRegArmMemoryStalls];

    if (*ipcIndex == ULONG_MAX)
    {
        PH_FORMAT format[3];
        WCHAR name[PH_HW_NAMED_SENSOR_NAME_LENGTH];

        PhInitFormatS(&format[0], L"CPU ");
        PhInitFormatU(&format[1], Index);
        PhInitFormatS(&format[2], L" IPC");
        PhFormatToBuffer(format, RTL_NUMBER_OF(format), name, sizeof(name), NULL);
        *ipcIndex = PhpHwAddNamedSensor(PhHwNamedSensorCoreIpc, PhpHwCpuSensors[Index].PackageIndex, Index, name);

        PhInitFormatS(&format[2], L" memory stalls");
        PhFormatToBuffer(format, RTL_NUMBER_OF(format), name, sizeof(name), NULL);
        *stallIndex = PhpHwAddNamedSensor(PhHwNamedSensorCoreMemoryStall, PhpHwCpuSensors[Index].PackageIndex, Index, name);
    }

    PhpHwSetNamedSensor(*ipcIndex, TRUE, (FLOAT)instructions / (FLOAT)Cycles);
    PhpHwSetNamedSensor(*stallIndex, TRUE, min((FLOAT)stalls * 100.0f / (FLOAT)Cycles, 100.0f));
}

/**
 * Decodes the clocks of an ARM64 processor. A cycle counter that stands still for PHP_HW_CPU_ARM_STALL_LIMIT updates
 * leaves the processor's plan for the session.
 *
 * \return The PH_HW_CAP_* bits of the processor.
 */
static ULONG PhpHwCpuDecodeArm64(
    _In_ ULONG Index
    )
{
    PPH_HW_CPU_SENSOR sensor = &PhpHwCpuSensors[Index];
    PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[Index];
    BOOLEAN amu = PhpHwCpuRegisterUsable(PhpHwRegArmCycles, Index) && PhpHwCpuRegisterUsable(PhpHwRegArmConstantCycles, Index);
    PHP_HW_REG cyclesReg = amu ? PhpHwRegArmCycles : PhpHwRegArmPmuCycles;
    ULONG64 mask = (1ULL << PhpHwRegArmCounter) | (1ULL << cyclesReg) | (amu ? (1ULL << PhpHwRegArmConstantCycles) : 0);
    PH_ARM_CYCLE_SAMPLE last;
    PH_ARM_CYCLE_SAMPLE current;
    PH_ARM_CLOCK_STATUS status;
    FLOAT effectiveMhz;
    FLOAT activeMhz;
    ULONG valid = 0;

    if (!PhpHwCpuArmCounterHz || (!amu && !PhpHwCpuRegisterUsable(PhpHwRegArmPmuCycles, Index)))
        return 0;
    if ((sample->Valid & sample->LastValid & mask) != mask)
        return 0;

    last.Counter = sample->LastValue[PhpHwRegArmCounter];
    last.Cycles = sample->LastValue[cyclesReg];
    last.ConstantCycles = amu ? sample->LastValue[PhpHwRegArmConstantCycles] : 0;
    current.Counter = sample->Value[PhpHwRegArmCounter];
    current.Cycles = sample->Value[cyclesReg];
    current.ConstantCycles = amu ? sample->Value[PhpHwRegArmConstantCycles] : 0;

    status = PhArmCycleClocks(&last, &current, PhpHwCpuArmCounterHz, amu, &effectiveMhz, &activeMhz);

    if (status == PhArmClockStalled)
    {
        if (++PhpHwCpuArmStalls[Index] >= PHP_HW_CPU_ARM_STALL_LIMIT)
        {
            PhTraceInfo("Hardware sensor register 0x%lx unavailable on processor %lu: the cycle counter stalled", PhpHwCpuRegisters[cyclesReg].Register, Index);
            PhpHwCpuArmStalls[Index] = 0;
            PhpHwCpuArmDisable(cyclesReg, Index);

            if (amu)
                PhpHwCpuArmDisable(PhpHwRegArmConstantCycles, Index);
        }

        return 0;
    }

    PhpHwCpuArmStalls[Index] = 0;

    if (status != PhArmClockValid)
        return 0;

    sensor->EffectiveMhz = effectiveMhz;
    valid |= PH_HW_CAP_EFFECTIVE_CLOCK;

    if (amu)
    {
        sensor->ActiveMhz = activeMhz;
        valid |= PH_HW_CAP_ACTIVE_CLOCK;

        PhpHwCpuArmActivity(Index, current.Cycles - last.Cycles);
    }

    return valid;
}

/**
 * \return The lower of two limits, ignoring a limit of 0 (unavailable).
 */
static FLOAT PhpHwCpuLowerLimit(
    _In_ FLOAT Limit1,
    _In_ FLOAT Limit2
    )
{
    if (Limit1 <= 0.0f)
        return Limit2;
    if (Limit2 <= 0.0f)
        return Limit1;

    return min(Limit1, Limit2);
}

/**
 * Publishes one C-state residency counter as the percent of the update's time, listing its named sensor the first
 * time it has a value.
 *
 * \param Sample The sample of the core's or package's first processor.
 * \param Residency The index into PhpHwCpuResidencyRegs.
 * \param Slot The index into PhpHwCpuResidencyIndex.
 * \param PackageIndex The package of the counter.
 * \param Core The core index, ULONG_MAX for a package counter.
 * \param Seconds The update's time.
 */
static VOID PhpHwCpuPublishResidency(
    _In_ PPHP_HW_SAMPLE Sample,
    _In_ ULONG Residency,
    _In_ ULONG Slot,
    _In_ ULONG PackageIndex,
    _In_ ULONG Core,
    _In_ FLOAT Seconds
    )
{
    PHP_HW_REG reg = PhpHwCpuResidencyRegs[Residency].Reg;
    UCHAR state = PhpHwCpuResidencyRegs[Residency].State;
    FLOAT ticks = Seconds * PhpHwCpuTscMhz * 1000000.0f;
    FLOAT percent;

    // Silvermont and Airmont (the models with MSR_FSB_FREQ) count package C6 at 3FAh.
    if (reg == PhpHwRegPackageC7Residency && PhpHwCpuRegisters[PhpHwRegFsbFreq].Register)
        state = 6;

    if (ticks <= 0.0f || !(Sample->Valid & Sample->LastValid & (1ULL << reg)))
    {
        PhpHwSetNamedSensor(PhpHwCpuResidencyIndex[Slot], FALSE, 0.0f);
        return;
    }

    percent = min((FLOAT)(Sample->Value[reg] - Sample->LastValue[reg]) * 100.0f / ticks, 100.0f);

    if (PhpHwCpuResidencyIndex[Slot] == ULONG_MAX)
    {
        PH_FORMAT format[4];
        WCHAR name[PH_HW_NAMED_SENSOR_NAME_LENGTH];
        ULONG count = 0;

        if (Core == ULONG_MAX)
        {
            PhInitFormatS(&format[count++], L"Package C");
        }
        else
        {
            PhInitFormatS(&format[count++], L"Core ");
            PhInitFormatU(&format[count++], Core);
            PhInitFormatS(&format[count++], L" C");
        }

        PhInitFormatU(&format[count++], state);
        PhFormatToBuffer(format, count, name, sizeof(name), NULL);
        PhpHwCpuResidencyIndex[Slot] = PhpHwAddNamedSensor(
            PhHwNamedSensorCStateResidency,
            PackageIndex,
            Core == ULONG_MAX ? state : 0x100 + 16 * Core + state,
            name
            );
    }

    PhpHwSetNamedSensor(PhpHwCpuResidencyIndex[Slot], TRUE, percent);
}

/**
 * Publishes the C-state residency of every core and package whose counters the plan reads.
 */
static VOID PhpHwCpuPublishResidencies(
    _In_ FLOAT Seconds
    )
{
    for (ULONG r = 0; r < RTL_NUMBER_OF(PhpHwCpuResidencyRegs); r++)
    {
        if (!PhpHwCpuRegisterActive(PhpHwCpuResidencyRegs[r].Reg))
            continue;

        if (r < PHP_HW_CPU_CORE_STATES)
        {
            for (ULONG c = 0; c < PhpHwCpuCoreCount; c++)
            {
                ULONG first = PhpHwCpuCoreFirst[c];

                if (first != ULONG_MAX)
                    PhpHwCpuPublishResidency(&PhpHwCpuSamples[first], r, c * PHP_HW_CPU_CORE_STATES + r, PhpHwCpuSensors[first].PackageIndex, c, Seconds);
            }
        }
        else
        {
            for (ULONG p = 0; p < PhpHwPackageCount; p++)
            {
                ULONG first = PhpHwCpuPackageFirst[p];

                if (first != ULONG_MAX)
                {
                    PhpHwCpuPublishResidency(
                        &PhpHwCpuSamples[first],
                        r,
                        PhpHwCpuCoreCount * PHP_HW_CPU_CORE_STATES + p * PHP_HW_CPU_PACKAGE_STATES + (r - PHP_HW_CPU_CORE_STATES),
                        p,
                        ULONG_MAX,
                        Seconds
                        );
                }
            }
        }
    }
}

static VOID PhpHwCpuPublish(
    VOID
    )
{
    FLOAT seconds = 0.0f;
    ULONG capabilities = 0;
    ULONG index;
    BOOLEAN restarted;

    if (PhpHwCpuLastSampleTime && PhpHwCpuSampleTime > PhpHwCpuLastSampleTime)
        seconds = (FLOAT)(PhpHwCpuSampleTime - PhpHwCpuLastSampleTime) / (FLOAT)PHP_HW_SECOND;

    // After a gap or a counter restart (a resume) the pass is a first sample.
    restarted = seconds > PHP_HW_CPU_MAX_INTERVAL;

    for (ULONG i = 0; i < PhpHwProcessorCount && !restarted; i++)
    {
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[i];

        if ((sample->Valid & sample->LastValid & (1ULL << PhpHwRegAperf)) && sample->Value[PhpHwRegAperf] < sample->LastValue[PhpHwRegAperf])
            restarted = TRUE;
    }

    if (restarted)
    {
        for (ULONG i = 0; i < PhpHwProcessorCount; i++)
            PhpHwCpuSamples[i].LastValid = 0;
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PhpHwCpuTickCorePower[i] = 0.0f;
        PhpHwCpuTickCoreCount[i] = 0;
        PhpHwCpuTickValid[i] = 0;
        PhpHwCpuTickThrottle[i] = 0;
        PhpHwCpuTickMaxCore[i] = 0.0f;
    }

    for (ULONG c = 0; c < PhpHwCpuCoreCount; c++)
    {
        ULONG first = PhpHwCpuCoreFirst[c];
        ULONG package;
        FLOAT watts;

        if (first == ULONG_MAX || (package = PhpHwCpuSensors[first].PackageIndex) >= PhpHwPackageCount)
            continue;

        if (PhpHwCpuSampleWatts(&PhpHwCpuSamples[first], PhpHwRegCoreEnergy, PhpHwCpuEnergyUnit, seconds, &watts))
        {
            PhpHwCpuTickCorePower[package] += watts;
            PhpHwCpuTickCoreCount[package]++;
        }
    }

    PhpHwCpuPublishResidencies(seconds);

    // The integrated graphics' energy, listed once it has moved (a part with its graphics off never counts).
    for (ULONG p = 0; p < PhpHwPackageCount; p++)
    {
        ULONG first = PhpHwCpuPackageFirst[p];
        FLOAT graphicsWatts;
        BOOLEAN graphicsValid;

        if (first == ULONG_MAX || !PhpHwCpuRegisterActive(PhpHwRegPp1Energy))
            continue;

        graphicsValid = PhpHwCpuSampleWatts(&PhpHwCpuSamples[first], PhpHwRegPp1Energy, PhpHwCpuEnergyUnit, seconds, &graphicsWatts);

        if (graphicsValid && graphicsWatts > 0.0f && PhpHwCpuPp1Index[p] == ULONG_MAX)
            PhpHwCpuPp1Index[p] = PhpHwAddNamedSensor(PhHwNamedSensorRaplPower, p, 1, L"Graphics (PP1)");

        PhpHwSetNamedSensor(PhpHwCpuPp1Index[p], graphicsValid, graphicsValid ? graphicsWatts : 0.0f);
    }

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        PPH_HW_CPU_SENSOR sensor = &PhpHwCpuSensors[i];
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[i];
        PPHP_HW_SAMPLE core = &PhpHwCpuSamples[PhpHwCpuCoreFirst[sensor->CoreIndex]];
        FLOAT busMhz = PhpHwPackageSensors[sensor->PackageIndex].BusMhz;
        ULONG valid = 0;

        if (seconds > 0.0f && (sample->Valid & sample->LastValid & (1ULL << PhpHwRegAperf)))
        {
            ULONG64 deltaAperf = sample->Value[PhpHwRegAperf] - sample->LastValue[PhpHwRegAperf];

            sensor->EffectiveMhz = PhEffectiveMhzFromAperf(deltaAperf, seconds);
            valid |= PH_HW_CAP_EFFECTIVE_CLOCK;

            // MPERF counts at the base clock; without one there is no active clock.
            if (PhpHwCpuTscMhz > 0.0f && (sample->Valid & sample->LastValid & (1ULL << PhpHwRegMperf)))
            {
                ULONG64 deltaMperf = sample->Value[PhpHwRegMperf] - sample->LastValue[PhpHwRegMperf];

                if (deltaMperf)
                {
                    sensor->ActiveMhz = PhActiveMhzFromAperfMperf(deltaAperf, deltaMperf, PhpHwCpuTscMhz);
                    valid |= PH_HW_CAP_ACTIVE_CLOCK;
                }
            }
        }

        if (PhpHwCpuClass == PhpHwCpuIntel)
        {
            ULONG package = sensor->PackageIndex;

            if (sample->Valid & (1ULL << PhpHwRegPerfStatus))
            {
                ULONG64 value = sample->Value[PhpHwRegPerfStatus];
                ULONG ratio = PhIntelPerfStatusRatio(value, &PhpHwCpuIdentity);

                if (ratio)
                {
                    sensor->Multiplier = (FLOAT)ratio;
                    valid |= PH_HW_CAP_MULTIPLIER;
                }

                if (PhpHwCpuVoltageSupported)
                {
                    sensor->Voltage = PhIntelPerfStatusToVolts(value, &PhpHwCpuIdentity);
                    valid |= PH_HW_CAP_VOLTAGE;
                }
            }

            if ((core->Valid & (1ULL << PhpHwRegThermStatus)) && package < PhpHwPackageCount && PhpHwCpuTjMax[package])
            {
                ULONG64 value = core->Value[PhpHwRegThermStatus];
                FLOAT celsius;

                if (PhIntelThermStatusToCelsius(value, PhpHwCpuTjMax[package], PhpHwCpuCoreReadoutMask, &celsius))
                {
                    sensor->Temperature = celsius;
                    valid |= PH_HW_CAP_CORE_TEMP;

                    if (!(PhpHwCpuTickValid[package] & PH_HW_CAP_CORE_TEMP) || celsius > PhpHwCpuTickMaxCore[package])
                        PhpHwCpuTickMaxCore[package] = celsius;

                    PhpHwCpuTickValid[package] |= PH_HW_CAP_CORE_TEMP;
                }

                sensor->ThrottleFlags = PhpHwCpuDecodeThrottle(value);
                valid |= PH_HW_CAP_THROTTLE;
                PhpHwCpuTickThrottle[package] |= sensor->ThrottleFlags;
                PhpHwCpuTickValid[package] |= PH_HW_CAP_THROTTLE;
            }
        }
        else if (PhpHwCpuClass == PhpHwCpuAmdZen || PhpHwCpuClass == PhpHwCpuAmdLegacy)
        {
            // HwPstateStatus and COFVID_STATUS report the requested P-state, not the boosted ratio.
            if ((valid & PH_HW_CAP_ACTIVE_CLOCK) && busMhz > 0.0f)
            {
                sensor->Multiplier = sensor->ActiveMhz / busMhz;
                valid |= PH_HW_CAP_MULTIPLIER;
            }

            if (core->Valid & (1ULL << PhpHwRegPerfStatus))
            {
                ULONG64 value = core->Value[PhpHwRegPerfStatus];
                FLOAT mhz = PhAmdHwPStateStatusToMhz(value, &PhpHwCpuIdentity, PhpHwCpuClockControl);
                UCHAR vid;

                if (!(valid & PH_HW_CAP_MULTIPLIER) && mhz > 0.0f && busMhz > 0.0f)
                {
                    sensor->Multiplier = mhz / busMhz;
                    valid |= PH_HW_CAP_MULTIPLIER;
                }

                if (PhpHwCpuVoltageSupported && PhAmdPStateVid(value, &PhpHwCpuIdentity, &vid))
                {
                    sensor->Voltage = PhAmdCpuVidToVolts(vid, &PhpHwCpuIdentity);
                    valid |= PH_HW_CAP_VOLTAGE;
                }
            }
        }
        else if (PhpHwCpuClass == PhpHwCpuArm64)
        {
            valid = PhpHwCpuDecodeArm64(i) | (ReadULongAcquire(&sensor->Valid) & PH_HW_CAP_CORE_TEMP);
        }
        else if (PhpHwCpuClass == PhpHwCpuCentaur)
        {
            ULONG package = sensor->PackageIndex;
            FLOAT celsius;

            if (
                (core->Valid & (1ULL << PhpHwRegCentaurTemperature)) && package < PhpHwPackageCount &&
                PhCentaurTemperatureToCelsius(core->Value[PhpHwRegCentaurTemperature], &celsius)
                )
            {
                sensor->Temperature = celsius;
                valid |= PH_HW_CAP_CORE_TEMP;

                if (!(PhpHwCpuTickValid[package] & PH_HW_CAP_CORE_TEMP) || celsius > PhpHwCpuTickMaxCore[package])
                    PhpHwCpuTickMaxCore[package] = celsius;

                PhpHwCpuTickValid[package] |= PH_HW_CAP_CORE_TEMP;
            }
        }

        WriteULongRelease(&sensor->Valid, valid);
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[PhpHwCpuPackageFirst[i]];
        ULONG valid = PhpHwCpuTickValid[i];
        ULONG owned;
        FLOAT watts;
        FLOAT limit;

        // The package's own temperature, else on Intel and Centaur the hottest core; the mmio family comes after
        // (spec 5.1).
        if ((sample->Valid & (1ULL << PhpHwRegPackageThermStatus)) && PhpHwCpuTjMax[i])
        {
            ULONG64 value = sample->Value[PhpHwRegPackageThermStatus];

            sensor->Temperature = PhIntelPackageThermStatusToCelsius(value, PhpHwCpuTjMax[i], PhpHwCpuPackageReadoutMask);
            valid |= PH_HW_CAP_PACKAGE_TEMP | PH_HW_CAP_THROTTLE;
            PhpHwCpuTickThrottle[i] |= PhpHwCpuDecodeThrottle(value);
        }
        else if ((PhpHwCpuClass == PhpHwCpuIntel || PhpHwCpuClass == PhpHwCpuCentaur) && (valid & PH_HW_CAP_CORE_TEMP))
        {
            sensor->Temperature = PhpHwCpuTickMaxCore[i];
            valid |= PH_HW_CAP_PACKAGE_TEMP;
        }

        // A failed read of the power limit keeps the last value.
        if (PhpHwCpuPowerUnit > 0.0f && (sample->Valid & (1ULL << PhpHwRegPowerLimit)))
            PhpHwCpuDecodeMsrPowerLimit(i, sample->Value[PhpHwRegPowerLimit]);

        if (sample->Valid & (1ULL << PhpHwRegLimitReasons))
        {
            PhpHwCpuTickThrottle[i] |= PhpHwCpuDecodeLimitReasons(sample->Value[PhpHwRegLimitReasons]);
            valid |= PH_HW_CAP_THROTTLE;
        }

        // A disabled limit is not valid.
        if ((index = PhpHwCpuLimitIndex[i * PHP_HW_CPU_LIMIT_SENSORS + PHP_HW_CPU_LIMIT_PL4]) != ULONG_MAX)
        {
            BOOLEAN limitValid = (sample->Valid & (1ULL << PhpHwRegVrCurrentConfig)) && (sample->Value[PhpHwRegVrCurrentConfig] & 0x1FFF);

            PhpHwSetNamedSensor(index, limitValid, limitValid ? (FLOAT)(sample->Value[PhpHwRegVrCurrentConfig] & 0x1FFF) * PhpHwCpuPowerUnit : 0.0f);
            capabilities |= PhpHwCpuRegisterActive(PhpHwRegVrCurrentConfig) ? PH_HW_CAP_PLATFORM_LIMITS : 0;
        }

        if (sample->Valid & (1ULL << PhpHwRegPlatformPowerLimit))
        {
            ULONG64 value = sample->Value[PhpHwRegPlatformPowerLimit];

            if ((index = PhpHwCpuLimitIndex[i * PHP_HW_CPU_LIMIT_SENSORS + PHP_HW_CPU_LIMIT_PLATFORM_PL1]) != ULONG_MAX)
                PhpHwSetNamedSensor(index, !!(value & 0x8000), (FLOAT)(value & 0x7FFF) * PhpHwCpuPowerUnit);
            if ((index = PhpHwCpuLimitIndex[i * PHP_HW_CPU_LIMIT_SENSORS + PHP_HW_CPU_LIMIT_PLATFORM_PL2]) != ULONG_MAX)
                PhpHwSetNamedSensor(index, !!(value & (1ULL << 47)), (FLOAT)((value >> 32) & 0x7FFF) * PhpHwCpuPowerUnit);
        }
        else
        {
            PhpHwSetNamedSensor(PhpHwCpuLimitIndex[i * PHP_HW_CPU_LIMIT_SENSORS + PHP_HW_CPU_LIMIT_PLATFORM_PL1], FALSE, 0.0f);
            PhpHwSetNamedSensor(PhpHwCpuLimitIndex[i * PHP_HW_CPU_LIMIT_SENSORS + PHP_HW_CPU_LIMIT_PLATFORM_PL2], FALSE, 0.0f);
        }

        if (PhpHwCpuRegisterActive(PhpHwRegPlatformPowerLimit) && (
            PhpHwCpuLimitIndex[i * PHP_HW_CPU_LIMIT_SENSORS + PHP_HW_CPU_LIMIT_PLATFORM_PL1] != ULONG_MAX ||
            PhpHwCpuLimitIndex[i * PHP_HW_CPU_LIMIT_SENSORS + PHP_HW_CPU_LIMIT_PLATFORM_PL2] != ULONG_MAX
            ))
        {
            capabilities |= PH_HW_CAP_PLATFORM_LIMITS;
        }

        // The uncore ratio is in units of the bus clock.
        if (PhpHwCpuUncoreIndex[i] != ULONG_MAX)
        {
            BOOLEAN uncoreValid = (sample->Valid & (1ULL << PhpHwRegUncorePerfStatus)) && PhIntelUncoreRatio(sample->Value[PhpHwRegUncorePerfStatus]);
            FLOAT busMhz = sensor->BusMhz > 0.0f ? sensor->BusMhz : PHP_HW_CPU_BUS_MHZ;

            PhpHwSetNamedSensor(PhpHwCpuUncoreIndex[i], uncoreValid, uncoreValid ? (FLOAT)PhIntelUncoreRatio(sample->Value[PhpHwRegUncorePerfStatus]) * busMhz : 0.0f);
            capabilities |= PhpHwCpuRegisterActive(PhpHwRegUncorePerfStatus) ? PH_HW_CAP_UNCORE_CLOCK : 0;
        }

        if (valid & PH_HW_CAP_CORE_TEMP)
            sensor->MaxCoreTemperature = PhpHwCpuTickMaxCore[i];

        sensor->ThrottleFlags = PhpHwCpuTickThrottle[i];

        // The active limit is the lower of the MSR and MMIO limits (mmio's of the last update).
        if (limit = PhpHwCpuLowerLimit(PhpHwCpuMsrPowerLimit1[i], PhpHwMmioPowerLimit(i, FALSE)))
        {
            sensor->ActivePowerLimit1 = limit;
            valid |= PH_HW_CAP_POWER_LIMIT1;
        }

        if (limit = PhpHwCpuLowerLimit(PhpHwCpuMsrPowerLimit2[i], PhpHwMmioPowerLimit(i, TRUE)))
        {
            sensor->ActivePowerLimit2 = limit;
            valid |= PH_HW_CAP_POWER_LIMIT2;
        }

        if (PhpHwCpuSampleWatts(sample, PhpHwRegPackageEnergy, PhpHwCpuEnergyUnit, seconds, &watts))
        {
            PhpHwCpuPackageEnergySeen[i] |= watts > 0.0f;

            if (PhpHwCpuPackageEnergySeen[i])
            {
                sensor->PackagePower = watts;
                valid |= PH_HW_CAP_RAPL_PACKAGE;
            }
            else if (PhpHwCpuPackageIdleTicks[i] < PHP_HW_CPU_ENERGY_IDLE_TICKS)
            {
                PhpHwCpuPackageIdleTicks[i]++;
            }
        }
        else if (PhpHwCpuComputeUnitPower(i, &watts))
        {
            sensor->PackagePower = watts;
            valid |= PH_HW_CAP_RAPL_PACKAGE;
        }

        if (PhpHwCpuSampleWatts(sample, PhpHwRegPp0Energy, PhpHwCpuEnergyUnit, seconds, &watts))
        {
            PhpHwCpuPp0EnergySeen[i] |= watts > 0.0f;

            if (PhpHwCpuPp0EnergySeen[i])
            {
                sensor->CorePower = watts;
                valid |= PH_HW_CAP_RAPL_CORE;
            }
            else if (PhpHwCpuPp0IdleTicks[i] < PHP_HW_CPU_ENERGY_IDLE_TICKS)
            {
                PhpHwCpuPp0IdleTicks[i]++;
            }
        }
        else if (PhpHwCpuRegisterActive(PhpHwRegCoreEnergy) && PhpHwCpuPackageCores[i] && PhpHwCpuTickCoreCount[i] == PhpHwCpuPackageCores[i])
        {
            sensor->CorePower = PhpHwCpuTickCorePower[i];
            valid |= PH_HW_CAP_RAPL_CORE;
        }

        if (PhpHwCpuSampleWatts(sample, PhpHwRegDramEnergy, PhpHwCpuDramEnergyUnit, seconds, &watts))
        {
            PhpHwCpuDramEnergySeen[i] |= watts > 0.0f;

            if (PhpHwCpuDramEnergySeen[i])
            {
                sensor->DramPower = watts;
                valid |= PH_HW_CAP_RAPL_DRAM;
            }
            else if (PhpHwCpuDramIdleTicks[i] < PHP_HW_CPU_ENERGY_IDLE_TICKS)
            {
                PhpHwCpuDramIdleTicks[i]++;
            }
        }

        if (PhpHwCpuSampleWatts(sample, PhpHwRegPlatformEnergy, PhpHwCpuPlatformEnergyUnit, seconds, &watts))
        {
            PhpHwCpuPlatformEnergySeen[i] |= watts > 0.0f;

            if (PhpHwCpuPlatformEnergySeen[i])
            {
                sensor->PlatformPower = watts;
                valid |= PH_HW_CAP_RAPL_PSYS;
            }
            else if (PhpHwCpuPlatformIdleTicks[i] < PHP_HW_CPU_ENERGY_IDLE_TICKS)
            {
                PhpHwCpuPlatformIdleTicks[i]++;
            }
        }

        // A bit another family owns for the package is that family's to clear or set.
        owned = PhpHwCpuPackageCaps & ~(PhpHwVsecPackageCaps(i) | PhpHwPciPackageCaps(i));
        PhpHwCpuTickValid[i] = valid;
        WriteULongRelease(&sensor->Valid, (ReadULongAcquire(&sensor->Valid) & ~owned) | (valid & owned));
    }

    PhpHwFamilies[PhpHwFamilyCpu].Capabilities = capabilities;

    PhpHwCpuDropIdleEnergy();
}

/**
 * Reads every active register once and publishes what it decodes.
 *
 * \return STATUS_SUCCESS, STATUS_RETRY, or the status of a call when no call was answered.
 */
NTSTATUS PhpHwCpuPass(
    VOID
    )
{
    NTSTATUS status;

    if (PhpHwCpuClass == PhpHwCpuNone)
        return STATUS_NOT_SUPPORTED;

    PhpHwCpuItemCount = 0;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[i];

        sample->LastValid = sample->Valid;
        memcpy(sample->LastValue, sample->Value, sizeof(sample->Value));
        sample->Valid = 0;

        for (ULONG reg = 0; reg < PhpHwRegMaximum; reg++)
        {
            if (!PhpHwCpuRegisterUsable(reg, i) || PhpHwCpuProbeOnly(reg) || !PhpHwCpuRegisterOnProcessor(reg, i) ||
                PhpHwCpuItemCount >= PhpHwCpuCapacity)
            {
                continue;
            }

            PhpHwCpuItems[PhpHwCpuItemCount].Group = PhpHwCpuSensors[i].ProcessorNumber.Group;
            PhpHwCpuItems[PhpHwCpuItemCount].Number = PhpHwCpuSensors[i].ProcessorNumber.Number;
            PhpHwCpuItems[PhpHwCpuItemCount].Msr = PhpHwCpuRegisters[reg].Register;
            PhpHwCpuTags[PhpHwCpuItemCount].Reg = reg;
            PhpHwCpuTags[PhpHwCpuItemCount].Processor = i;
            PhpHwCpuTags[PhpHwCpuItemCount].Unanswered = FALSE;
            PhpHwCpuItemCount++;
        }
    }

    if (!PhpHwCpuItemCount)
        return STATUS_SUCCESS;

    status = PhpHwCpuReadItems();

    PhpHwCpuLastSampleTime = PhpHwCpuSampleTime;
    PhpHwCpuSampleTime = PhpHwQueryTime();

    PhpHwCpuStoreValues();
    PhpHwCpuPublish();

    return status;
}

static VOID PhpHwCpuFree(
    _Inout_ PVOID Pointer
    )
{
    PVOID* pointer = Pointer;

    if (*pointer)
    {
        PhFree(*pointer);
        *pointer = NULL;
    }
}

/**
 * Forgets the session: the plan, the samples and the identity.
 */
VOID PhpHwCpuEnd(
    VOID
    )
{
    PhpHwCpuClass = PhpHwCpuNone;
    PhpHwCpuSignature = 0;
    PhpHwCpuClockControl = 0;
    PhpHwCpuItemCount = 0;
    PhpHwCpuCapacity = 0;
    memset(PhpHwCpuRegisters, 0, sizeof(PhpHwCpuRegisters));

    PhpHwCpuFree(&PhpHwCpuItems);
    PhpHwCpuFree(&PhpHwCpuValues);
    PhpHwCpuFree(&PhpHwCpuTags);
    PhpHwCpuFree(&PhpHwCpuSamples);
    PhpHwCpuFree(&PhpHwCpuFirstOfCore);
    PhpHwCpuFree(&PhpHwCpuFirstOfPackage);
    PhpHwCpuFree(&PhpHwCpuCoreFirst);
    PhpHwCpuFree(&PhpHwCpuCppcHighest);
    PhpHwCpuFree(&PhpHwCpuPackageFirst);
    PhpHwCpuFree(&PhpHwCpuPackageCores);
    PhpHwCpuFree(&PhpHwCpuTickCorePower);
    PhpHwCpuFree(&PhpHwCpuTickCoreCount);
    PhpHwCpuFree(&PhpHwCpuTjMax);
    PhpHwCpuFree(&PhpHwCpuTickValid);
    PhpHwCpuFree(&PhpHwCpuTickThrottle);
    PhpHwCpuFree(&PhpHwCpuTickMaxCore);
    PhpHwCpuFree(&PhpHwCpuMsrPowerLimit1);
    PhpHwCpuFree(&PhpHwCpuMsrPowerLimit2);
    PhpHwCpuFree(&PhpHwCpuPackageIdleTicks);
    PhpHwCpuFree(&PhpHwCpuPp0IdleTicks);
    PhpHwCpuFree(&PhpHwCpuDramIdleTicks);
    PhpHwCpuFree(&PhpHwCpuPlatformIdleTicks);
    PhpHwCpuFree(&PhpHwCpuPackageEnergySeen);
    PhpHwCpuFree(&PhpHwCpuPp0EnergySeen);
    PhpHwCpuFree(&PhpHwCpuDramEnergySeen);
    PhpHwCpuFree(&PhpHwCpuPlatformEnergySeen);
    PhpHwCpuFree(&PhpHwCpuUncoreIndex);
    PhpHwCpuFree(&PhpHwCpuLimitIndex);
    PhpHwCpuFree(&PhpHwCpuResidencyIndex);
    PhpHwCpuFree(&PhpHwCpuPp1Index);
    memset(PhpHwCpuPolicy, 0, sizeof(PhpHwCpuPolicy));
    PhpHwCpuPolicyValid = 0;
    memset(PhpHwCpuOc, 0, sizeof(PhpHwCpuOc));
    PhpHwCpuOcValid = FALSE;
    memset(PhpHwCpuCppc, 0, sizeof(PhpHwCpuCppc));
    PhpHwCpuCppcValid = 0;
    PhpHwCpuTccOffset = ULONG_MAX;
    PhpHwCpuFree(&PhpHwCpuFirstOfUnit);
    PhpHwCpuFree(&PhpHwCpuPackageIdentity);
    PhpHwCpuFree(&PhpHwCpuArmMidr);
    PhpHwCpuFree(&PhpHwCpuArmDisabled);
    PhpHwCpuFree(&PhpHwCpuArmStalls);
    PhpHwCpuFree(&PhpHwCpuArmActivityIndex);
    PhpHwCpuArmCounterHz = 0;
    PhpHwCpuUnitSampleRatio = 0;
    PhpHwCpuUnitMaxAccumulator = 0;
    PhpHwCpuDramEnergyUnit = 0.0f;
    PhpHwCpuPlatformEnergyUnit = 0.0f;
    PhpHwCpuPowerUnit = 0.0f;
    PhpHwCpuPackageCaps = PHP_HW_CPU_PACKAGE_CAPS;
    PhpHwCpuCoreCount = 0;
    PhpHwCpuTscMhz = 0.0f;
    PhpHwCpuEnergyUnit = 0.0f;
    PhpHwCpuVoltageSupported = FALSE;
}

/**
 * Determines whether the session identified an ARM64 processor.
 *
 * \return TRUE for an ARM64 processor.
 */
BOOLEAN PhpHwCpuIsArm64(
    VOID
    )
{
    return PhpHwCpuClass == PhpHwCpuArm64;
}

/**
 * Retrieves the processor identity of the session when the processor is Intel.
 *
 * \param Identity Receives the identity.
 * \return TRUE for an Intel processor this session identified.
 */
_Success_(return)
BOOLEAN PhpHwCpuQueryIntel(
    _Out_ PPH_CPU_IDENTITY Identity
    )
{
    if (PhpHwCpuClass != PhpHwCpuIntel)
        return FALSE;

    *Identity = PhpHwCpuIdentity;
    return TRUE;
}

/**
 * Retrieves the processor identity of the session when the processor is AMD family 10h to 16h.
 *
 * \param Identity Receives the identity.
 * \return TRUE for such a processor this session identified.
 */
_Success_(return)
BOOLEAN PhpHwCpuQueryAmdLegacy(
    _Out_ PPH_CPU_IDENTITY Identity
    )
{
    if (PhpHwCpuClass != PhpHwCpuAmdLegacy)
        return FALSE;

    *Identity = PhpHwCpuIdentity;
    return TRUE;
}

/**
 * Determines whether the session reads the processor's thermal registers.
 */
BOOLEAN PhpHwCpuThermalPlanned(
    VOID
    )
{
    return PhpHwCpuRegisterActive(PhpHwRegThermStatus) || PhpHwCpuRegisterActive(PhpHwRegPackageThermStatus) ||
        PhpHwCpuRegisterActive(PhpHwRegCentaurTemperature);
}

/**
 * Determines whether the processor's RAPL MSR of an energy kind serves this session; TPMI stands in where it does
 * not.
 *
 * \param Kind 0 the package, 1 DRAM, 2 the platform.
 */
BOOLEAN PhpHwCpuEnergyPlanned(
    _In_ ULONG Kind
    )
{
    static CONST PHP_HW_REG registers[] = { PhpHwRegPackageEnergy, PhpHwRegDramEnergy, PhpHwRegPlatformEnergy };

    return Kind < RTL_NUMBER_OF(registers) && PhpHwCpuRegisterActive(registers[Kind]);
}

/**
 * \return The PH_HW_CAP_* bits cpu decoded for the package in its last pass, 0 without a pass.
 */
ULONG PhpHwCpuPackageValid(
    _In_ ULONG PackageIndex
    )
{
    if (!PhpHwCpuTickValid || PackageIndex >= PhpHwPackageCount)
        return 0;

    return PhpHwCpuTickValid[PackageIndex];
}

/**
 * \return The RAPL power unit of the session in watts, or 0 when unknown.
 */
FLOAT PhpHwCpuQueryPowerUnit(
    VOID
    )
{
    return PhpHwCpuPowerUnit;
}

/**
 * Appends the overclocking mailbox planes that answered the probe's read; a refused or failed plane is left out.
 */
static VOID PhpHwCpuAppendOc(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PCWSTR names[MaxKphHwOcPlane] = { L"core", L"GPU", L"cache", L"system agent", L"analog I/O" };

    if (!PhpHwCpuOcValid)
        return;

    for (ULONG i = 0; i < MaxKphHwOcPlane; i++)
    {
        ULONG data = PhpHwCpuOc[i].Data;
        LONG offset = (LONG)((data >> 21) & 0x7FF);

        if (!NT_SUCCESS(PhpHwCpuOc[i].Status) || PhpHwCpuOc[i].Code)
            continue;

        if (offset > 0x3FF)
            offset -= 0x800;

        if (data & (1UL << 20))
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"  OC %s: offset %.1f mV, override %.3f V, max ratio %lu\n",
                names[i],
                (FLOAT)offset * 1000.0f / 1024.0f,
                (FLOAT)((data >> 8) & 0xFFF) / 1024.0f,
                data & 0xFF
                );
        }
        else
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"  OC %s: offset %.1f mV, adaptive, max ratio %lu\n",
                names[i],
                (FLOAT)offset * 1000.0f / 1024.0f,
                data & 0xFF
                );
        }
    }
}

/**
 * Appends AMD CPPC as processor 0 had it at the probe, and the cores by their highest performance, the preferred first.
 */
static VOID PhpHwCpuAppendCppc(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PULONG cores;
    ULONG count = 0;
    UCHAR highest = 0;
    BOOLEAN ranked = FALSE;

    if (PhpHwCpuCppcValid & 0x1)
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"  CPPC: highest %lu, nominal %lu, lowest nonlinear %lu, lowest %lu",
            (ULONG)((PhpHwCpuCppc[0] >> 24) & 0xFF),
            (ULONG)((PhpHwCpuCppc[0] >> 16) & 0xFF),
            (ULONG)((PhpHwCpuCppc[0] >> 8) & 0xFF),
            (ULONG)(PhpHwCpuCppc[0] & 0xFF)
            );

        if (PhpHwCpuCppcValid & 0x2)
            PhAppendFormatStringBuilder(Builder, L"; enabled %lu", (ULONG)(PhpHwCpuCppc[1] & 1));

        if (PhpHwCpuCppcValid & 0x4)
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"; request minimum %lu, maximum %lu, desired %lu, EPP %lu",
                (ULONG)((PhpHwCpuCppc[2] >> 8) & 0xFF),
                (ULONG)(PhpHwCpuCppc[2] & 0xFF),
                (ULONG)((PhpHwCpuCppc[2] >> 16) & 0xFF),
                (ULONG)((PhpHwCpuCppc[2] >> 24) & 0xFF)
                );
        }

        PhAppendStringBuilder2(Builder, L"\n");
    }

    if (!PhpHwCpuCppcHighest || !PhpHwCpuCoreCount)
        return;

    cores = PhAllocate(sizeof(ULONG) * PhpHwCpuCoreCount);

    // Insertion by highest performance, descending; equal cores keep their order.
    for (ULONG c = 0; c < PhpHwCpuCoreCount; c++)
    {
        ULONG at;

        if (!PhpHwCpuCppcHighest[c])
            continue;

        if (count && PhpHwCpuCppcHighest[c] != highest)
            ranked = TRUE;

        highest = PhpHwCpuCppcHighest[c];

        for (at = count; at && PhpHwCpuCppcHighest[cores[at - 1]] < PhpHwCpuCppcHighest[c]; at--)
            cores[at] = cores[at - 1];

        cores[at] = c;
        count++;
    }

    if (count && !ranked)
    {
        PhAppendFormatStringBuilder(Builder, L"  Preferred cores: not ranked (every core %lu)\n", (ULONG)highest);
    }
    else if (count)
    {
        PhAppendStringBuilder2(Builder, L"  Preferred cores (CPPC highest performance):");

        for (ULONG k = 0; k < count; k++)
            PhAppendFormatStringBuilder(Builder, L"%s %lu (%lu)", k ? L"," : L"", cores[k], (ULONG)PhpHwCpuCppcHighest[cores[k]]);

        PhAppendStringBuilder2(Builder, L"\n");
    }

    PhFree(cores);
}

/**
 * Appends the microcode revision Windows recorded for processor 0 (Update Revision: 8 bytes with the revision in the
 * high half on Intel, 4 bytes elsewhere).
 */
static VOID PhpHwCpuAppendMicrocode(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PH_STRINGREF processorKeyName = PH_STRINGREF_INIT(L"Hardware\\Description\\System\\CentralProcessor\\0");
    HANDLE keyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION value;

    if (!NT_SUCCESS(PhOpenKey(&keyHandle, KEY_READ, PH_KEY_LOCAL_MACHINE, &processorKeyName, 0)))
        return;

    if (NT_SUCCESS(PhQueryValueKeyZ(keyHandle, L"Update Revision", KeyValuePartialInformation, &value)))
    {
        if (value->Type == REG_BINARY && value->DataLength == sizeof(ULONG64))
            PhAppendFormatStringBuilder(Builder, L"  Microcode: 0x%08lx\n", (ULONG)(*(PULONG64)value->Data >> 32));
        else if (value->Type == REG_BINARY && value->DataLength == sizeof(ULONG))
            PhAppendFormatStringBuilder(Builder, L"  Microcode: 0x%08lx\n", *(PULONG)value->Data);

        PhFree(value);
    }

    NtClose(keyHandle);
}

/**
 * Writes the collector's section of the hardware report: the identity, the plan and each register's state.
 *
 * \param Builder The report.
 */
VOID PhpHwCpuDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PCWSTR classNames[] = { L"none", L"Intel", L"AMD Zen", L"AMD 10h to 16h", L"Centaur", L"ARM64" };
    static CONST PCWSTR scopeNames[] = { L"thread", L"core", L"package", L"compute unit" };
    CHAR vendor[13];

    if (PhpHwCpuClass == PhpHwCpuNone)
    {
        PhAppendStringBuilder2(Builder, L"  Processor: not identified\n");
        return;
    }

    // Each kind of core has its own MIDR; consecutive processors of a kind share a line.
    if (PhpHwCpuClass == PhpHwCpuArm64)
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"  Processor: class ARM64, counter %llu Hz\n  Plan: %lu items a pass, calls of at most %lu\n",
            PhpHwCpuArmCounterHz,
            PhpHwCpuItemCount,
            PhpHwCpuBatch
            );

        for (ULONG i = 0; i < PhpHwProcessorCount; i++)
        {
            ULONG last = i;

            while (last + 1 < PhpHwProcessorCount && PhpHwCpuArmMidr[last + 1] == PhpHwCpuArmMidr[i])
                last++;

            PhAppendFormatStringBuilder(Builder, L"  Processors %lu to %lu: MIDR 0x%08lx\n", i, last, PhpHwCpuArmMidr[i]);
            i = last;
        }
    }
    else
    {
        memcpy(vendor, PhpHwCpuIdentity.Vendor, 12);
        vendor[12] = ANSI_NULL;

        PhAppendFormatStringBuilder(
            Builder,
            L"  Processor: %hs family 0x%x model 0x%x stepping 0x%x, signature 0x%08lx, class %s%s\n"
            L"  Plan: %lu items a pass, calls of at most %lu\n"
            L"  Base clock: %.0f MHz, energy unit %g J, voltage %s\n",
            vendor,
            PhpHwCpuIdentity.DisplayFamily,
            PhpHwCpuIdentity.DisplayModel,
            PhpHwCpuIdentity.Stepping,
            PhpHwCpuSignature,
            classNames[PhpHwCpuClass],
            PhpHwCpuIdentity.Hybrid ? L", hybrid" : L"",
            PhpHwCpuItemCount,
            PhpHwCpuBatch,
            PhpHwCpuTscMhz,
            PhpHwCpuEnergyUnit,
            PhpHwCpuVoltageSupported ? L"decoded" : L"not decoded"
            );

        for (ULONG i = 1; PhpHwCpuPackageIdentity && i < PhpHwPackageCount; i++)
        {
            PKPH_HW_CPU_IDENTITY other = &PhpHwCpuPackageIdentity[i];

            if (!memcmp(other, &PhpHwCpuPackageIdentity[0], sizeof(KPH_HW_CPU_IDENTITY)))
                continue;

            memcpy(vendor, other->Vendor, 12);
            vendor[12] = ANSI_NULL;

            PhAppendFormatStringBuilder(
                Builder,
                L"  Package %lu: %hs family 0x%lx model 0x%lx stepping 0x%lx, signature 0x%08lx; the plan follows package 0\n",
                i,
                vendor,
                other->Family,
                other->Model,
                other->Stepping,
                other->Signature
                );
        }
    }

    // HWP and the energy/performance bias as processor 0 had them at the probe.
    if (PhpHwCpuPolicyValid & 0x7)
    {
        PCWSTR separator = L" ";

        PhAppendStringBuilder2(Builder, L"  HWP:");

        if (PhpHwCpuPolicyValid & 0x1)
        {
            PhAppendFormatStringBuilder(Builder, L"%senabled %lu", separator, (ULONG)(PhpHwCpuPolicy[0] & 1));
            separator = L"; ";
        }

        if (PhpHwCpuPolicyValid & 0x2)
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"%shighest %lu, guaranteed %lu, efficient %lu, lowest %lu",
                separator,
                (ULONG)(PhpHwCpuPolicy[1] & 0xFF),
                (ULONG)((PhpHwCpuPolicy[1] >> 8) & 0xFF),
                (ULONG)((PhpHwCpuPolicy[1] >> 16) & 0xFF),
                (ULONG)((PhpHwCpuPolicy[1] >> 24) & 0xFF)
                );
            separator = L"; ";
        }

        if (PhpHwCpuPolicyValid & 0x4)
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"%srequest minimum %lu, maximum %lu, desired %lu, EPP %lu",
                separator,
                (ULONG)(PhpHwCpuPolicy[2] & 0xFF),
                (ULONG)((PhpHwCpuPolicy[2] >> 8) & 0xFF),
                (ULONG)((PhpHwCpuPolicy[2] >> 16) & 0xFF),
                (ULONG)((PhpHwCpuPolicy[2] >> 24) & 0xFF)
                );
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }

    if (PhpHwCpuPolicyValid & 0x8)
        PhAppendFormatStringBuilder(Builder, L"  Energy/performance bias: %lu\n", (ULONG)(PhpHwCpuPolicy[3] & 0xF));
    if (PhpHwCpuPolicyValid & 0x10)
        PhAppendFormatStringBuilder(Builder, L"  SMI count: %lu (processor 0, at the probe)\n", (ULONG)PhpHwCpuPolicy[4]);
    if (PhpHwCpuTccOffset != ULONG_MAX)
        PhAppendFormatStringBuilder(Builder, L"  TCC offset: %lu\n", PhpHwCpuTccOffset);

    // Ratio limits by active core count: bytes of TURBO_RATIO_LIMIT, the counts from TURBO_RATIO_LIMIT1 where it reads.
    if (PhpHwCpuPolicyValid & 0x20)
    {
        PhAppendStringBuilder2(Builder, L"  Turbo ratio limits:");

        for (ULONG b = 0; b < 8; b++)
        {
            ULONG ratio = (ULONG)((PhpHwCpuPolicy[5] >> (b * 8)) & 0xFF);
            ULONG cores = (PhpHwCpuPolicyValid & 0x40) ? (ULONG)((PhpHwCpuPolicy[6] >> (b * 8)) & 0xFF) : b + 1;

            if (ratio)
                PhAppendFormatStringBuilder(Builder, L" %luC %lu", cores, ratio);
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }

    // Configurable TDP: the nominal ratio, each level's TDP (power units) and ratio, the level selected.
    if ((PhpHwCpuPolicyValid & 0x780) && PhpHwCpuPowerUnit > 0.0f)
    {
        PCWSTR separator = L" ";

        PhAppendStringBuilder2(Builder, L"  Configurable TDP:");

        if (PhpHwCpuPolicyValid & 0x80)
        {
            PhAppendFormatStringBuilder(Builder, L"%snominal ratio %lu", separator, (ULONG)(PhpHwCpuPolicy[7] & 0xFF));
            separator = L"; ";
        }

        for (ULONG level = 1; level <= 2; level++)
        {
            if (PhpHwCpuPolicyValid & (0x80UL << level))
            {
                PhAppendFormatStringBuilder(
                    Builder,
                    L"%slevel %lu %.1f W ratio %lu",
                    separator,
                    level,
                    (FLOAT)(PhpHwCpuPolicy[7 + level] & 0x7FFF) * PhpHwCpuPowerUnit,
                    (ULONG)((PhpHwCpuPolicy[7 + level] >> 16) & 0xFF)
                    );
                separator = L"; ";
            }
        }

        if (PhpHwCpuPolicyValid & 0x400)
        {
            PCWSTR locked = L"";

            if (PhpHwCpuPolicy[10] & 0x80000000)
                locked = L", locked";

            PhAppendFormatStringBuilder(
                Builder,
                L"%slevel %lu selected%s",
                separator,
                (ULONG)(PhpHwCpuPolicy[10] & 0x3),
                locked
                );
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }

    if (PhpHwCpuPolicyValid & 0x800)
        PhAppendFormatStringBuilder(Builder, L"  Turbo activation ratio: %lu\n", (ULONG)(PhpHwCpuPolicy[11] & 0xFF));

    PhpHwCpuAppendOc(Builder);
    PhpHwCpuAppendCppc(Builder);
    PhpHwCpuAppendMicrocode(Builder);

    for (ULONG reg = 0; reg < PhpHwRegMaximum; reg++)
    {
        PPHP_HW_REGISTER r = &PhpHwCpuRegisters[reg];

        if (!r->Register)
            continue;

        PhAppendFormatStringBuilder(
            Builder,
            L"  Register 0x%08lx (%s): %s\n",
            r->Register,
            scopeNames[r->Scope],
            r->Disabled ? L"left out" : PhpHwCpuProbeOnly(reg) ? L"probe" : L"read"
            );
    }
}
