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
#define PHP_HW_CPU_PACKAGE_CAPS (PH_HW_CAP_RAPL_PACKAGE | PH_HW_CAP_RAPL_CORE | PH_HW_CAP_RAPL_DRAM | PH_HW_CAP_RAPL_PSYS | \
    PH_HW_CAP_POWER_LIMIT1 | PH_HW_CAP_POWER_LIMIT2) // package bits this collector always owns
#define PHP_HW_CPU_ENERGY_IDLE_TICKS 5 // samples a DRAM or platform counter may stay still before it leaves the plan
#define PHP_HW_CPU_UNCORE_ID 0x40000000 // named sensor id of the MSR uncore clock, apart from the TPMI domains
#define PHP_HW_CPU_LIMIT_PL4 0 // named limit sensors of a package; the index is the sensor's id
#define PHP_HW_CPU_LIMIT_PLATFORM_PL1 1
#define PHP_HW_CPU_LIMIT_PLATFORM_PL2 2
#define PHP_HW_CPU_LIMIT_SENSORS 3
#define PHP_HW_CPU_ARM_AMU_BUILD 26100 // from this build Windows reads AMCNTENSET0_EL0 at boot: the AMU does not trap
#define PHP_HW_CPU_ARM_AMU_COUNTERS 0x3 // AMCNTENSET0_EL0: CPU_CYCLES and CNT_CYCLES enabled
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
    PhpHwRegRaplUnit, // probe only from here
    PhpHwRegPStateDef0,
    PhpHwRegTemperatureTarget, // Intel MSR_TEMPERATURE_TARGET, the Centaur and Zhaoxin critical temperature
    PhpHwRegPlatformInfo,
    PhpHwRegFsbFreq, // Intel Silvermont and Airmont MSR_FSB_FREQ
    PhpHwRegCuMaxPowerAccumulator, // AMD 15h and 16h CU_MAX_PWR_ACCUMULATOR
    PhpHwRegArmFrequency, // ARM64 CNTFRQ_EL0
    PhpHwRegArmAmuEnable, // ARM64 AMCNTENSET0_EL0
    PhpHwRegMaximum
} PHP_HW_REG;
C_ASSERT(PhpHwRegMaximum <= 32);

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
    ULONG Valid;
    ULONG LastValid;
    ULONG64 Value[PhpHwRegMaximum];
    ULONG64 LastValue[PhpHwRegMaximum];
} PHP_HW_SAMPLE, *PPHP_HW_SAMPLE;

typedef struct _PHP_HW_CPU_TAG
{
    PHP_HW_REG Reg;
    ULONG Processor; // the item's processor; its core and package come from the sensor
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
static FLOAT PhpHwCpuEnergyUnit = 0.0f; // joules
static FLOAT PhpHwCpuDramEnergyUnit = 0.0f;
static FLOAT PhpHwCpuPlatformEnergyUnit = 0.0f;
static FLOAT PhpHwCpuPowerUnit = 0.0f; // watts
static PFLOAT PhpHwCpuMsrPowerLimit1 = NULL; // per package, watts; 0 when unavailable
static PFLOAT PhpHwCpuMsrPowerLimit2 = NULL;
static PUCHAR PhpHwCpuDramIdleTicks = NULL; // per package: samples whose DRAM energy did not move, until it does
static PUCHAR PhpHwCpuPlatformIdleTicks = NULL;
static PBOOLEAN PhpHwCpuDramEnergySeen = NULL; // per package: the counter moved
static PBOOLEAN PhpHwCpuPlatformEnergySeen = NULL;
static PULONG PhpHwCpuUncoreIndex = NULL; // per package, the uncore clock's named sensor; ULONG_MAX when not listed
static PULONG PhpHwCpuLimitIndex = NULL; // per package, PHP_HW_CPU_LIMIT_SENSORS named sensors; ULONG_MAX when not listed
static PBOOLEAN PhpHwCpuFirstOfUnit = NULL; // per processor; NULL without compute units
static ULONG PhpHwCpuUnitSampleRatio = 0; // CPUID 8000_0007h ECX
static ULONG64 PhpHwCpuUnitMaxAccumulator = 0; // CU_MAX_PWR_ACCUMULATOR; 0 when unread
static PKPH_HW_CPU_IDENTITY PhpHwCpuPackageIdentity = NULL; // x86, per package: what identify gave; zero when it failed
static PULONG PhpHwCpuArmMidr = NULL; // ARM64, per processor: the MIDR identify gave; 0 when not identified
static PULONG PhpHwCpuArmDisabled = NULL; // ARM64, per processor: 1 << PHP_HW_REG of the registers out of its plan
static PUCHAR PhpHwCpuArmStalls = NULL; // ARM64, per processor: updates its cycle counter stood still
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
    memset(PhpHwCpuRegisters, 0, sizeof(PhpHwCpuRegisters));

    if (PhpHwCpuClass == PhpHwCpuAmdZen)
    {
        PhpHwCpuSetRegister(PhpHwRegAperf, PH_AMD_MSR_APERF_READONLY, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegMperf, PH_AMD_MSR_MPERF_READONLY, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegPerfStatus, PH_AMD_MSR_HW_PSTATE_STATUS, PhpHwScopeCore);
        PhpHwCpuSetRegister(PhpHwRegCoreEnergy, PH_AMD_MSR_CORE_ENERGY_STAT, PhpHwScopeCore);
        PhpHwCpuSetRegister(PhpHwRegPackageEnergy, PH_AMD_MSR_PKG_ENERGY_STAT, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegRaplUnit, PH_AMD_MSR_RAPL_PWR_UNIT, PhpHwScopePackage);
        PhpHwCpuSetRegister(PhpHwRegPStateDef0, PH_AMD_MSR_PSTATE_DEF0, PhpHwScopePackage);
    }
    else if (PhpHwCpuClass == PhpHwCpuAmdLegacy)
    {
        PhpHwCpuSetRegister(PhpHwRegAperf, PH_INTEL_MSR_APERF, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegMperf, PH_INTEL_MSR_MPERF, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegPerfStatus, PH_AMD_MSR_COFVID_STATUS, PhpHwScopeCore);

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
    if (PhpHwCpuArmDisabled && (PhpHwCpuArmDisabled[Processor] & (1UL << Reg)))
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
    PhpHwCpuArmDisabled[Processor] |= 1UL << Reg;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        if (!(PhpHwCpuArmDisabled[i] & (1UL << Reg)))
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

        if (!PhpHwCpuApplyStatus(tag->Reg, tag->Processor, PhpHwCpuValues[i].Status))
            continue;

        PhpHwCpuSamples[tag->Processor].Value[tag->Reg] = PhpHwCpuValues[i].Value;
        PhpHwCpuSamples[tag->Processor].Valid |= 1UL << tag->Reg;
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
            if (sample->Valid & (1UL << PhpHwRegVrCurrentConfig))
                listed[PHP_HW_CPU_LIMIT_PL4] = !!(sample->Value[PhpHwRegVrCurrentConfig] & 0x1FFF);

            if (sample->Valid & (1UL << PhpHwRegPlatformPowerLimit))
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
        if ((PhpHwCpuSamples[i].Valid & (1UL << PhpHwRegArmFrequency)) && PhpHwCpuSamples[i].Value[PhpHwRegArmFrequency])
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
            !(sample->Valid & (1UL << PhpHwRegArmAmuEnable)) ||
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

        sample->Valid = 0;
    }

    PhpHwCpuRegisters[PhpHwRegArmFrequency].Disabled = TRUE;
    PhpHwCpuRegisters[PhpHwRegArmAmuEnable].Disabled = !!PhpHwCpuRegisters[PhpHwRegArmAmuEnable].Register;

    // No ARM64 part has a package temperature register: the counters' hottest thermal zone stands in.
    PhpHwCpuPackageCaps = PHP_HW_CPU_PACKAGE_CAPS | PH_HW_CAP_PACKAGE_TEMP;

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
            if (package ? !PhpHwCpuFirstOfPackage[i] : i != 0)
                continue;

            PhpHwCpuItems[PhpHwCpuItemCount].Group = PhpHwCpuSensors[i].ProcessorNumber.Group;
            PhpHwCpuItems[PhpHwCpuItemCount].Number = PhpHwCpuSensors[i].ProcessorNumber.Number;
            PhpHwCpuItems[PhpHwCpuItemCount].Msr = PhpHwCpuRegisters[reg].Register;
            PhpHwCpuTags[PhpHwCpuItemCount].Reg = reg;
            PhpHwCpuTags[PhpHwCpuItemCount].Processor = i;
            PhpHwCpuItemCount++;
        }
    }

    if (PhpHwCpuItemCount)
    {
        PhpHwCpuReadItems();
        PhpHwCpuStoreValues();
    }

    // The read-only APERF and MPERF aliases may fault or read zero under a hypervisor.
    if (PhpHwCpuClass == PhpHwCpuAmdZen && (
        !(first->Valid & (1UL << PhpHwRegAperf)) || first->Value[PhpHwRegAperf] == 0 || !PhpHwCpuRegisterActive(PhpHwRegMperf)
        ))
    {
        PhpHwCpuSetRegister(PhpHwRegAperf, PH_INTEL_MSR_APERF, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegMperf, PH_INTEL_MSR_MPERF, PhpHwScopeThread);
    }

    if (raplUnitValid = !!(PhpHwCpuSamples[PhpHwCpuPackageFirst[0]].Valid & (1UL << PhpHwRegRaplUnit)))
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

        busMhz = (sample->Valid & (1UL << PhpHwRegFsbFreq)) ? PhIntelFsbFreqToMhz(sample->Value[PhpHwRegFsbFreq], &PhpHwCpuIdentity) : 0.0f;
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[PhpHwCpuPackageFirst[i]];
        FLOAT baseMhz = 0.0f;
        FLOAT powerLimit = 0.0f;
        ULONG tjMax = 0;

        if (PhpHwCpuClass == PhpHwCpuIntel)
        {
            if (sample->Valid & (1UL << PhpHwRegTemperatureTarget))
                tjMax = PhIntelTemperatureTargetTjMax(sample->Value[PhpHwRegTemperatureTarget]);
            if (!tjMax && (PhpHwCpuRegisterActive(PhpHwRegThermStatus) || PhpHwCpuRegisterActive(PhpHwRegPackageThermStatus)))
                tjMax = PhIntelFixedTjMax(&PhpHwCpuIdentity);
            if (sample->Valid & (1UL << PhpHwRegPlatformInfo))
                baseMhz = (FLOAT)((sample->Value[PhpHwRegPlatformInfo] >> 8) & 0xFF) * busMhz;
            if (raplUnitValid && (sample->Valid & (1UL << PhpHwRegPowerLimit)))
            {
                powerLimit = (FLOAT)(sample->Value[PhpHwRegPowerLimit] & 0x7FFF) * PhpHwCpuPowerUnit;
                PhpHwCpuDecodeMsrPowerLimit(i, sample->Value[PhpHwRegPowerLimit]);
            }
        }
        else if (PhpHwCpuClass == PhpHwCpuAmdZen || PhpHwCpuClass == PhpHwCpuAmdLegacy)
        {
            // MPERF counts at the P0 frequency.
            if (sample->Valid & (1UL << PhpHwRegPStateDef0))
                baseMhz = PhAmdPStateDefToMhz(sample->Value[PhpHwRegPStateDef0], &PhpHwCpuIdentity, 0);
        }
        else if (PhpHwCpuClass == PhpHwCpuCentaur)
        {
            // Shown only: the temperature register reads degrees itself.
            if (sample->Valid & (1UL << PhpHwRegTemperatureTarget))
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

    if (first->Valid & (1UL << PhpHwRegPerfStatus))
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
    PhpHwCpuUnitMaxAccumulator = (first->Valid & (1UL << PhpHwRegCuMaxPowerAccumulator)) ? first->Value[PhpHwRegCuMaxPowerAccumulator] : 0;

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
    if (!(Sample->Valid & Sample->LastValid & (1UL << Reg)))
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
        PhpHwCpuArmDisabled = PhAllocateZero(sizeof(ULONG) * PhpHwProcessorCount);
        PhpHwCpuArmStalls = PhAllocateZero(sizeof(UCHAR) * PhpHwProcessorCount);
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
    PhpHwCpuDramIdleTicks = PhAllocateZero(sizeof(UCHAR) * max(PhpHwPackageCount, 1));
    PhpHwCpuPlatformIdleTicks = PhAllocateZero(sizeof(UCHAR) * max(PhpHwPackageCount, 1));
    PhpHwCpuDramEnergySeen = PhAllocateZero(sizeof(BOOLEAN) * max(PhpHwPackageCount, 1));
    PhpHwCpuPlatformEnergySeen = PhAllocateZero(sizeof(BOOLEAN) * max(PhpHwPackageCount, 1));
    PhpHwCpuUncoreIndex = PhAllocate(sizeof(ULONG) * max(PhpHwPackageCount, 1));
    PhpHwCpuLimitIndex = PhAllocate(sizeof(ULONG) * PHP_HW_CPU_LIMIT_SENSORS * max(PhpHwPackageCount, 1));
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
                PhpHwCpuValues[i].Status = status;
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
 * Drops the RAPL DRAM and platform registers when no package's counter moved in PHP_HW_CPU_ENERGY_IDLE_TICKS samples.
 */
static VOID PhpHwCpuDropIdleEnergy(
    VOID
    )
{
    BOOLEAN dramIdle = PhpHwCpuRegisterActive(PhpHwRegDramEnergy) && PhpHwPackageCount;
    BOOLEAN platformIdle = PhpHwCpuRegisterActive(PhpHwRegPlatformEnergy) && PhpHwPackageCount;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        dramIdle &= !PhpHwCpuDramEnergySeen[i] && PhpHwCpuDramIdleTicks[i] >= PHP_HW_CPU_ENERGY_IDLE_TICKS;
        platformIdle &= !PhpHwCpuPlatformEnergySeen[i] && PhpHwCpuPlatformIdleTicks[i] >= PHP_HW_CPU_ENERGY_IDLE_TICKS;
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
        ULONG both = (1UL << PhpHwRegCuPowerAccumulator) | (1UL << PhpHwRegPtsc);
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
    ULONG mask = (1UL << PhpHwRegArmCounter) | (1UL << cyclesReg) | (amu ? (1UL << PhpHwRegArmConstantCycles) : 0);
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
    }

    return valid;
}

static VOID PhpHwCpuPublish(
    VOID
    )
{
    FLOAT seconds = 0.0f;
    FLOAT cpuPower = 0.0f;
    ULONG capabilities = 0;
    ULONG index;

    if (PhpHwCpuLastSampleTime && PhpHwCpuSampleTime > PhpHwCpuLastSampleTime)
        seconds = (FLOAT)(PhpHwCpuSampleTime - PhpHwCpuLastSampleTime) / (FLOAT)PHP_HW_SECOND;

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

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        PPH_HW_CPU_SENSOR sensor = &PhpHwCpuSensors[i];
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[i];
        PPHP_HW_SAMPLE core = &PhpHwCpuSamples[PhpHwCpuCoreFirst[sensor->CoreIndex]];
        FLOAT busMhz = PhpHwPackageSensors[sensor->PackageIndex].BusMhz;
        ULONG valid = 0;

        if (seconds > 0.0f && (sample->Valid & sample->LastValid & (1UL << PhpHwRegAperf)))
        {
            ULONG64 deltaAperf = sample->Value[PhpHwRegAperf] - sample->LastValue[PhpHwRegAperf];

            sensor->EffectiveMhz = PhEffectiveMhzFromAperf(deltaAperf, seconds);
            valid |= PH_HW_CAP_EFFECTIVE_CLOCK;

            // MPERF counts at the base clock; without one there is no active clock.
            if (PhpHwCpuTscMhz > 0.0f && (sample->Valid & sample->LastValid & (1UL << PhpHwRegMperf)))
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

            if (sample->Valid & (1UL << PhpHwRegPerfStatus))
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

            if ((core->Valid & (1UL << PhpHwRegThermStatus)) && package < PhpHwPackageCount && PhpHwCpuTjMax[package])
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

            if (core->Valid & (1UL << PhpHwRegPerfStatus))
            {
                ULONG64 value = core->Value[PhpHwRegPerfStatus];
                FLOAT mhz = PhAmdHwPStateStatusToMhz(value, &PhpHwCpuIdentity, 0);
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
            valid = PhpHwCpuDecodeArm64(i);
        }
        else if (PhpHwCpuClass == PhpHwCpuCentaur)
        {
            ULONG package = sensor->PackageIndex;
            FLOAT celsius;

            if (
                (core->Valid & (1UL << PhpHwRegCentaurTemperature)) && package < PhpHwPackageCount &&
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
        FLOAT watts;
        FLOAT celsius;

        // The package's own temperature, else on Intel and Centaur the hottest core; the mmio family and, on ARM64,
        // the counters' hottest zone come after (spec 5.1).
        if ((sample->Valid & (1UL << PhpHwRegPackageThermStatus)) && PhpHwCpuTjMax[i])
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
        else if (PhpHwCpuClass == PhpHwCpuArm64 && PhpHwCounterHottestZone(&celsius))
        {
            // On x64 a zone is often a board sensor or a constant, and would mislead as the processor's.
            sensor->Temperature = celsius;
            valid |= PH_HW_CAP_PACKAGE_TEMP;
        }

        // A failed read of the power limit keeps the last value.
        if (PhpHwCpuPowerUnit > 0.0f && (sample->Valid & (1UL << PhpHwRegPowerLimit)))
            PhpHwCpuDecodeMsrPowerLimit(i, sample->Value[PhpHwRegPowerLimit]);

        if (sample->Valid & (1UL << PhpHwRegLimitReasons))
        {
            PhpHwCpuTickThrottle[i] |= PhpHwCpuDecodeLimitReasons(sample->Value[PhpHwRegLimitReasons]);
            valid |= PH_HW_CAP_THROTTLE;
        }

        // A disabled limit is not valid.
        if ((index = PhpHwCpuLimitIndex[i * PHP_HW_CPU_LIMIT_SENSORS + PHP_HW_CPU_LIMIT_PL4]) != ULONG_MAX)
        {
            BOOLEAN limitValid = (sample->Valid & (1UL << PhpHwRegVrCurrentConfig)) && (sample->Value[PhpHwRegVrCurrentConfig] & 0x1FFF);

            PhpHwSetNamedSensor(index, limitValid, limitValid ? (FLOAT)(sample->Value[PhpHwRegVrCurrentConfig] & 0x1FFF) * PhpHwCpuPowerUnit : 0.0f);
            capabilities |= PhpHwCpuRegisterActive(PhpHwRegVrCurrentConfig) ? PH_HW_CAP_PLATFORM_LIMITS : 0;
        }

        if (sample->Valid & (1UL << PhpHwRegPlatformPowerLimit))
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
            BOOLEAN uncoreValid = (sample->Valid & (1UL << PhpHwRegUncorePerfStatus)) && PhIntelUncoreRatio(sample->Value[PhpHwRegUncorePerfStatus]);
            FLOAT busMhz = sensor->BusMhz > 0.0f ? sensor->BusMhz : PHP_HW_CPU_BUS_MHZ;

            PhpHwSetNamedSensor(PhpHwCpuUncoreIndex[i], uncoreValid, uncoreValid ? (FLOAT)PhIntelUncoreRatio(sample->Value[PhpHwRegUncorePerfStatus]) * busMhz : 0.0f);
            capabilities |= PhpHwCpuRegisterActive(PhpHwRegUncorePerfStatus) ? PH_HW_CAP_UNCORE_CLOCK : 0;
        }

        if (valid & PH_HW_CAP_CORE_TEMP)
            sensor->MaxCoreTemperature = PhpHwCpuTickMaxCore[i];

        sensor->ThrottleFlags = PhpHwCpuTickThrottle[i];

        if (PhpHwCpuMsrPowerLimit1[i] > 0.0f)
        {
            sensor->ActivePowerLimit1 = PhpHwCpuMsrPowerLimit1[i];
            valid |= PH_HW_CAP_POWER_LIMIT1;
        }

        if (PhpHwCpuMsrPowerLimit2[i] > 0.0f)
        {
            sensor->ActivePowerLimit2 = PhpHwCpuMsrPowerLimit2[i];
            valid |= PH_HW_CAP_POWER_LIMIT2;
        }

        if (
            PhpHwCpuSampleWatts(sample, PhpHwRegPackageEnergy, PhpHwCpuEnergyUnit, seconds, &watts) ||
            PhpHwCpuComputeUnitPower(i, &watts)
            )
        {
            sensor->PackagePower = watts;
            valid |= PH_HW_CAP_RAPL_PACKAGE;
            cpuPower += watts;
        }

        if (PhpHwCpuSampleWatts(sample, PhpHwRegPp0Energy, PhpHwCpuEnergyUnit, seconds, &watts))
        {
            sensor->CorePower = watts;
            valid |= PH_HW_CAP_RAPL_CORE;
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

        WriteULongRelease(&sensor->Valid, (ReadULongAcquire(&sensor->Valid) & ~PhpHwCpuPackageCaps) | (valid & PhpHwCpuPackageCaps));
    }

    PhHwCpuPower = cpuPower;
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
    PhpHwCpuFree(&PhpHwCpuDramIdleTicks);
    PhpHwCpuFree(&PhpHwCpuPlatformIdleTicks);
    PhpHwCpuFree(&PhpHwCpuDramEnergySeen);
    PhpHwCpuFree(&PhpHwCpuPlatformEnergySeen);
    PhpHwCpuFree(&PhpHwCpuUncoreIndex);
    PhpHwCpuFree(&PhpHwCpuLimitIndex);
    PhpHwCpuFree(&PhpHwCpuFirstOfUnit);
    PhpHwCpuFree(&PhpHwCpuPackageIdentity);
    PhpHwCpuFree(&PhpHwCpuArmMidr);
    PhpHwCpuFree(&PhpHwCpuArmDisabled);
    PhpHwCpuFree(&PhpHwCpuArmStalls);
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
