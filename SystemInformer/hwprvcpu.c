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
#define PHP_HW_CPU_PACKAGE_CAPS (PH_HW_CAP_RAPL_PACKAGE | PH_HW_CAP_RAPL_CORE) // package bits this collector always owns
#define PHP_HW_CPU_THERMAL_CAPS (PH_HW_CAP_PACKAGE_TEMP | PH_HW_CAP_CORE_TEMP | PH_HW_CAP_THROTTLE) // with a thermal plan

typedef enum _PHP_HW_CPU_CLASS
{
    PhpHwCpuNone,
    PhpHwCpuIntel,
    PhpHwCpuAmdZen, // AuthenticAMD or HygonGenuine, family 17h and later
    PhpHwCpuAmdLegacy, // AuthenticAMD, families 10h to 16h
    PhpHwCpuCentaur // CentaurHauls or "  Shanghai  "
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
    PhpHwRegRaplUnit, // probe only from here
    PhpHwRegPStateDef0,
    PhpHwRegTemperatureTarget, // Intel MSR_TEMPERATURE_TARGET
    PhpHwRegPlatformInfo,
    PhpHwRegFsbFreq, // Intel Silvermont and Airmont MSR_FSB_FREQ
    PhpHwRegMaximum
} PHP_HW_REG;
C_ASSERT(PhpHwRegMaximum <= 32);

typedef enum _PHP_HW_SCOPE
{
    PhpHwScopeThread,
    PhpHwScopeCore, // the first processor of each core
    PhpHwScopePackage // the first processor of each package
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
    }

    return TRUE;
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
    case STATUS_NOT_SUPPORTED: // a faulting register
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

        if (!PhpHwCpuApplyStatus(tag->Reg, PhpHwCpuValues[i].Status))
            continue;

        PhpHwCpuSamples[tag->Processor].Value[tag->Reg] = PhpHwCpuValues[i].Value;
        PhpHwCpuSamples[tag->Processor].Valid |= 1UL << tag->Reg;
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

    if (PhpHwCpuSamples[PhpHwCpuPackageFirst[0]].Valid & (1UL << PhpHwRegRaplUnit))
        PhpHwCpuEnergyUnit = PhRaplEnergyUnitJoules(PhpHwCpuSamples[PhpHwCpuPackageFirst[0]].Value[PhpHwRegRaplUnit], &PhpHwCpuIdentity);

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
        ULONG tjMax = 0;

        if (PhpHwCpuClass == PhpHwCpuIntel)
        {
            if (sample->Valid & (1UL << PhpHwRegTemperatureTarget))
                tjMax = PhIntelTemperatureTargetTjMax(sample->Value[PhpHwRegTemperatureTarget]);
            if (!tjMax && (PhpHwCpuRegisterActive(PhpHwRegThermStatus) || PhpHwCpuRegisterActive(PhpHwRegPackageThermStatus)))
                tjMax = PhIntelFixedTjMax(&PhpHwCpuIdentity);
            if (sample->Valid & (1UL << PhpHwRegPlatformInfo))
                baseMhz = (FLOAT)((sample->Value[PhpHwRegPlatformInfo] >> 8) & 0xFF) * busMhz;
        }
        else if (PhpHwCpuClass == PhpHwCpuAmdZen || PhpHwCpuClass == PhpHwCpuAmdLegacy)
        {
            // MPERF counts at the P0 frequency.
            if (sample->Valid & (1UL << PhpHwRegPStateDef0))
                baseMhz = PhAmdPStateDefToMhz(sample->Value[PhpHwRegPStateDef0], &PhpHwCpuIdentity, 0);
        }

        if (i == 0)
            PhpHwCpuTscMhz = baseMhz;
        if (tjMax)
            anyTjMax = TRUE;

        PhpHwCpuTjMax[i] = tjMax;
        PhpHwPackageSensors[i].TjMax = (FLOAT)tjMax;
        PhpHwPackageSensors[i].BaseMhz = baseMhz;
        PhpHwPackageSensors[i].BusMhz = busMhz;
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

    // MPERF is only needed for the active clock; APERF alone yields the effective clock.
    if (PhpHwCpuTscMhz <= 0.0f)
        PhpHwCpuRegisters[PhpHwRegMperf].Disabled = TRUE;

    if (PhpHwCpuEnergyUnit <= 0.0f)
    {
        PhpHwCpuRegisters[PhpHwRegPackageEnergy].Disabled = TRUE;
        PhpHwCpuRegisters[PhpHwRegCoreEnergy].Disabled = TRUE;
    }
}

/**
 * Converts the change of a 32-bit energy counter over the sample interval to watts.
 */
_Success_(return)
static BOOLEAN PhpHwCpuSampleWatts(
    _In_ PPHP_HW_SAMPLE Sample,
    _In_ PHP_HW_REG Reg,
    _In_ FLOAT Seconds,
    _Out_ PFLOAT Watts
    )
{
    FLOAT watts;

    if (Seconds <= 0.0f || PhpHwCpuEnergyUnit <= 0.0f)
        return FALSE;
    if (!(Sample->Valid & Sample->LastValid & (1UL << Reg)))
        return FALSE;

    watts = (FLOAT)(ULONG)(Sample->Value[Reg] - Sample->LastValue[Reg]) * PhpHwCpuEnergyUnit / Seconds;

    if (watts > PHP_HW_CPU_ENERGY_MAX_WATTS)
        return FALSE;

    *Watts = watts;
    return TRUE;
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

    if (PhpHwQueryMachine() == IMAGE_FILE_MACHINE_ARM64)
        return STATUS_NOT_SUPPORTED;
    if (!(first = PhHwGetCpuSensor(0)))
        return STATUS_NOT_SUPPORTED;

    status = KphHwCpuIdentify(first->ProcessorNumber.Group, first->ProcessorNumber.Number, &identity);

    if (!NT_SUCCESS(status))
        return status;

    memset(&PhpHwCpuIdentity, 0, sizeof(PH_CPU_IDENTITY));
    memcpy(PhpHwCpuIdentity.Vendor, identity.Vendor, sizeof(identity.Vendor));
    PhpHwCpuIdentity.DisplayFamily = (USHORT)identity.Family;
    PhpHwCpuIdentity.DisplayModel = (UCHAR)identity.Model;
    PhpHwCpuIdentity.Stepping = (UCHAR)identity.Stepping;
    PhpHwCpuIdentity.Hybrid = !!identity.Hybrid;
    PhpHwCpuSignature = identity.Signature;

    if ((PhpHwCpuClass = PhpHwCpuClassify(&PhpHwCpuIdentity)) == PhpHwCpuNone)
        return STATUS_NOT_SUPPORTED;

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

/**
 * Decodes the pass and publishes it: the values first, then their bits.
 */
static VOID PhpHwCpuPublish(
    VOID
    )
{
    FLOAT seconds = 0.0f;
    FLOAT cpuPower = 0.0f;

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

        if (PhpHwCpuSampleWatts(&PhpHwCpuSamples[first], PhpHwRegCoreEnergy, seconds, &watts))
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

        WriteULongRelease(&sensor->Valid, valid);
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[PhpHwCpuPackageFirst[i]];
        ULONG valid = PhpHwCpuTickValid[i];
        FLOAT watts;

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

        if (valid & PH_HW_CAP_CORE_TEMP)
            sensor->MaxCoreTemperature = PhpHwCpuTickMaxCore[i];

        sensor->ThrottleFlags = PhpHwCpuTickThrottle[i];

        if (PhpHwCpuSampleWatts(sample, PhpHwRegPackageEnergy, seconds, &watts))
        {
            sensor->PackagePower = watts;
            valid |= PH_HW_CAP_RAPL_PACKAGE;
            cpuPower += watts;
        }

        if (PhpHwCpuRegisterActive(PhpHwRegCoreEnergy) && PhpHwCpuPackageCores[i] && PhpHwCpuTickCoreCount[i] == PhpHwCpuPackageCores[i])
        {
            sensor->CorePower = PhpHwCpuTickCorePower[i];
            valid |= PH_HW_CAP_RAPL_CORE;
        }

        WriteULongRelease(&sensor->Valid, (ReadULongAcquire(&sensor->Valid) & ~PhpHwCpuPackageCaps) | (valid & PhpHwCpuPackageCaps));
    }

    PhHwCpuPower = cpuPower;
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
            if (!PhpHwCpuRegisterActive(reg) || PhpHwCpuProbeOnly(reg) || !PhpHwCpuRegisterOnProcessor(reg, i) ||
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
    static CONST PCWSTR classNames[] = { L"none", L"Intel", L"AMD Zen", L"AMD 10h to 16h", L"Centaur" };
    static CONST PCWSTR scopeNames[] = { L"thread", L"core", L"package" };
    CHAR vendor[13];

    if (PhpHwCpuClass == PhpHwCpuNone)
    {
        PhAppendStringBuilder2(Builder, L"  Processor: not identified\n");
        return;
    }

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
