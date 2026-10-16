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

#include <ph.h>

#include <cpusensor.h>

static BOOLEAN PhpCpuIdentityIsVendor(
    _In_ PCPH_CPU_IDENTITY Identity,
    _In_reads_bytes_(12) PCSTR Vendor
    )
{
    return RtlEqualMemory(Identity->Vendor, Vendor, sizeof(Identity->Vendor));
}

static BOOLEAN PhpCpuIdentityIsModel(
    _In_ PCPH_CPU_IDENTITY Identity,
    _In_ USHORT Family,
    _In_reads_(Count) const UCHAR* Models,
    _In_ ULONG Count
    )
{
    if (Identity->DisplayFamily != Family)
        return FALSE;

    for (ULONG i = 0; i < Count; i++)
    {
        if (Identity->DisplayModel == Models[i])
            return TRUE;
    }

    return FALSE;
}

static BOOLEAN PhpCpuIdentityIsModelRange(
    _In_ PCPH_CPU_IDENTITY Identity,
    _In_ USHORT Family,
    _In_ UCHAR FirstModel,
    _In_ UCHAR LastModel
    )
{
    return Identity->DisplayFamily == Family &&
        Identity->DisplayModel >= FirstModel &&
        Identity->DisplayModel <= LastModel;
}

static BOOLEAN PhpIsIntelNehalem(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    static const UCHAR models[] = { 0x1A, 0x1E, 0x1F, 0x25, 0x2C, 0x2E, 0x2F };

    return PhCpuIdentityIsIntel(Identity) && PhpCpuIdentityIsModel(Identity, 0x06, models, RTL_NUMBER_OF(models));
}

static BOOLEAN PhpIsIntelRaplAtom(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    static const UCHAR models[] = { 0x37, 0x4A, 0x5A, 0x4C };

    return PhCpuIdentityIsIntel(Identity) && PhpCpuIdentityIsModel(Identity, 0x06, models, RTL_NUMBER_OF(models));
}

static BOOLEAN PhpIsAmdZen(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    return Identity->DisplayFamily == 0x17 ||
        Identity->DisplayFamily == 0x18 ||
        Identity->DisplayFamily == 0x19 ||
        Identity->DisplayFamily == 0x1A;
}

static BOOLEAN PhpIsAmdPreZenPStateLayout(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    return Identity->DisplayFamily == 0x10 ||
        Identity->DisplayFamily == 0x11 ||
        Identity->DisplayFamily == 0x15 ||
        Identity->DisplayFamily == 0x16;
}

/**
 * Reads the CPU identity of the current processor.
 *
 * \param Identity Receives the identity. Zeroed when the function fails.
 * \return TRUE on x86/x64, FALSE on other architectures.
 */
BOOLEAN NTAPI PhGetCpuIdentity(
    _Out_ PPH_CPU_IDENTITY Identity
    )
{
    RtlZeroMemory(Identity, sizeof(PH_CPU_IDENTITY));

#if defined(_M_IX86) || defined(_M_X64)
    INT cpuInfo[4];
    ULONG maximumLeaf;
    ULONG signature;
    ULONG family;
    ULONG model;

    __cpuid(cpuInfo, 0);
    maximumLeaf = (ULONG)cpuInfo[0];

    if (maximumLeaf < 1)
        return FALSE;

    Identity->Vendor[0] = (ULONG)cpuInfo[1];
    Identity->Vendor[1] = (ULONG)cpuInfo[3];
    Identity->Vendor[2] = (ULONG)cpuInfo[2];

    __cpuid(cpuInfo, 1);
    signature = (ULONG)cpuInfo[0];
    family = (signature >> 8) & 0xF;
    model = (signature >> 4) & 0xF;

    Identity->DisplayFamily = (USHORT)family;
    if (family == 0xF)
        Identity->DisplayFamily += (USHORT)((signature >> 20) & 0xFF);

    Identity->DisplayModel = (UCHAR)model;
    if (family == 0x6 || family == 0xF)
        Identity->DisplayModel |= (UCHAR)(((signature >> 16) & 0xF) << 4);

    Identity->Stepping = (UCHAR)(signature & 0xF);

    if (maximumLeaf >= 7)
    {
        __cpuidex(cpuInfo, 7, 0);
        Identity->Hybrid = !!(cpuInfo[3] & (1 << 15));
    }

    return TRUE;
#else
    return FALSE;
#endif
}

/**
 * Determines whether the identity is a GenuineIntel processor.
 *
 * \param Identity The CPU identity.
 * \return TRUE for GenuineIntel.
 */
BOOLEAN NTAPI PhCpuIdentityIsIntel(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    return PhpCpuIdentityIsVendor(Identity, "GenuineIntel");
}

/**
 * Determines whether the identity is an AuthenticAMD or HygonGenuine processor.
 *
 * \param Identity The CPU identity.
 * \return TRUE for AuthenticAMD or HygonGenuine.
 */
BOOLEAN NTAPI PhCpuIdentityIsAmd(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    return PhpCpuIdentityIsVendor(Identity, "AuthenticAMD") || PhpCpuIdentityIsVendor(Identity, "HygonGenuine");
}

/**
 * Retrieves the width of the IA32_THERM_STATUS or IA32_PACKAGE_THERM_STATUS digital readout.
 *
 * \param Identity The CPU identity.
 * \param Package TRUE for IA32_PACKAGE_THERM_STATUS.
 * \return 0x7F (bits 22:16) through Ice Lake, on the Atom cores before Gracemont and for the Sapphire
 * and Emerald Rapids core readout; 0xFF (bits 23:16) otherwise.
 */
ULONG NTAPI PhIntelThermReadoutMask(
    _In_ PCPH_CPU_IDENTITY Identity,
    _In_ BOOLEAN Package
    )
{
    static const UCHAR models7[] =
    {
        0x0E, 0x0F, 0x16, 0x17, 0x1D, // Yonah, Core 2
        0x1A, 0x1E, 0x1F, 0x2E, 0x25, 0x2C, 0x2F, // Nehalem, Westmere
        0x2A, 0x2D, 0x3A, 0x3E, 0x3C, 0x3F, 0x45, 0x46, // Sandy Bridge, Ivy Bridge, Haswell
        0x3D, 0x47, 0x4F, 0x56, // Broadwell
        0x4E, 0x5E, 0x55, 0x8E, 0x9E, 0xA5, 0xA6, 0x66, // Skylake, Kaby/Coffee/Comet Lake, Cannon Lake
        0x7D, 0x7E, 0x9D, 0x6A, 0x6C, // Ice Lake
        0x27, 0x35, 0x36, // Saltwell
        0x37, 0x4D, 0x4A, 0x5A, 0x5D, 0x4C, 0x75, // Silvermont, Airmont
        0x5C, 0x5F, 0x7A, 0x86, 0x96, 0x9C, // Goldmont, Goldmont Plus, Tremont
        0x57, 0x85, // Xeon Phi
    };
    static const UCHAR modelsCore7[] = { 0x8F, 0xCF }; // Sapphire Rapids, Emerald Rapids

    if (!PhCpuIdentityIsIntel(Identity))
        return 0x7F;
    if (PhpCpuIdentityIsModel(Identity, 0x06, models7, RTL_NUMBER_OF(models7)))
        return 0x7F;
    if (!Package && PhpCpuIdentityIsModel(Identity, 0x06, modelsCore7, RTL_NUMBER_OF(modelsCore7)))
        return 0x7F;

    return 0xFF;
}

/**
 * Converts IA32_THERM_STATUS to degrees Celsius.
 *
 * \param ThermStatus The MSR value.
 * \param TjMax The TjMax in degrees Celsius.
 * \param ReadoutMask The readout width from PhIntelThermReadoutMask.
 * \param Celsius Receives TjMax minus the digital readout (from bit 16).
 * \return FALSE when the reading-valid bit (31) is clear.
 */
_Success_(return)
BOOLEAN NTAPI PhIntelThermStatusToCelsius(
    _In_ ULONG64 ThermStatus,
    _In_ ULONG TjMax,
    _In_ ULONG ReadoutMask,
    _Out_ PFLOAT Celsius
    )
{
    ULONG readout;

    if (!(ThermStatus & (1ULL << 31)))
        return FALSE;

    readout = (ULONG)(ThermStatus >> 16) & ReadoutMask;
    *Celsius = (FLOAT)TjMax - (FLOAT)readout;

    return TRUE;
}

/**
 * Converts IA32_PACKAGE_THERM_STATUS to degrees Celsius.
 *
 * \param PackageThermStatus The MSR value.
 * \param TjMax The TjMax in degrees Celsius.
 * \param ReadoutMask The readout width from PhIntelThermReadoutMask.
 * \return TjMax minus the digital readout (from bit 16).
 */
FLOAT NTAPI PhIntelPackageThermStatusToCelsius(
    _In_ ULONG64 PackageThermStatus,
    _In_ ULONG TjMax,
    _In_ ULONG ReadoutMask
    )
{
    ULONG readout;

    readout = (ULONG)(PackageThermStatus >> 16) & ReadoutMask;

    return (FLOAT)TjMax - (FLOAT)readout;
}

/**
 * Extracts TjMax from MSR_TEMPERATURE_TARGET.
 *
 * \param TemperatureTarget The MSR value.
 * \return TjMax (bits 23:16) in degrees Celsius, or 0 when unavailable or outside 50-150.
 */
ULONG NTAPI PhIntelTemperatureTargetTjMax(
    _In_ ULONG64 TemperatureTarget
    )
{
    ULONG tjMax;

    tjMax = (ULONG)(TemperatureTarget >> 16) & 0xFF;

    if (tjMax < 50 || tjMax > 150)
        return 0;

    return tjMax;
}

/**
 * Retrieves the fixed TjMax of an Atom whose MSR_TEMPERATURE_TARGET is absent or zero.
 *
 * \param Identity The CPU identity.
 * \return TjMax in degrees Celsius, or 0 when the model has no fixed value.
 */
ULONG NTAPI PhIntelFixedTjMax(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    static const UCHAR models90[] = { 0x27, 0x35, 0x37, 0x4A, 0x5A, 0x4C }; // Saltwell, Silvermont, Airmont
    static const UCHAR models100[] = { 0x36 }; // Cedarview
    static const UCHAR models105[] = { 0x5C, 0x5F, 0x7A, 0x86, 0x96, 0x9C }; // Goldmont, Goldmont Plus, Tremont

    if (!PhCpuIdentityIsIntel(Identity))
        return 0;

    // Bay Trail E38xx (Silvermont stepping 9) is the 110 C part.
    if (Identity->DisplayFamily == 0x06 && Identity->DisplayModel == 0x37 && Identity->Stepping == 9)
        return 110;
    if (PhpCpuIdentityIsModel(Identity, 0x06, models90, RTL_NUMBER_OF(models90)))
        return 90;
    if (PhpCpuIdentityIsModel(Identity, 0x06, models100, RTL_NUMBER_OF(models100)))
        return 100;
    if (PhpCpuIdentityIsModel(Identity, 0x06, models105, RTL_NUMBER_OF(models105)))
        return 105;

    return 0;
}

/**
 * Selects the core performance limit reasons MSR of a processor.
 *
 * \param Identity The processor identity.
 * \return MSR_CORE_PERF_LIMIT_REASONS (690h on Haswell and Broadwell client, 64Fh on Skylake and later), or 0.
 */
ULONG NTAPI PhIntelLimitReasonsMsr(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    static const UCHAR models690[] = { 0x3C, 0x3D, 0x45, 0x46, 0x47 }; // Haswell, Broadwell client (EDK2 HaswellMsr.h)

    if (!PhCpuIdentityIsIntel(Identity))
        return 0;
    if (PhpCpuIdentityIsModel(Identity, 0x06, models690, RTL_NUMBER_OF(models690)))
        return PH_INTEL_MSR_HSW_CORE_PERF_LIMIT_REASONS;

    return PH_INTEL_MSR_CORE_PERF_LIMIT_REASONS;
}

/**
 * Determines whether MSR_VR_CURRENT_CONFIG holds PL4 in RAPL power units. Earlier parts and
 * models outside the list hold a VR current limit in the same field.
 *
 * \param Identity The processor identity.
 * \return TRUE when [12:0] is PL4.
 */
BOOLEAN NTAPI PhIntelHasPl4(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    // ICL-L, TGL-L, ADL, ADL-N, RPL, MTL, ARL-H, ARL-U, PTL, WCL; not ARL-S, LNL, RPL-S, BTL.
    static const UCHAR models6[] = { 0x7E, 0x8C, 0x97, 0x9A, 0xBE, 0xB7, 0xBA, 0xAC, 0xAA, 0xC5, 0xB5, 0xCC, 0xD5 };
    static const UCHAR models18[] = { 0x01, 0x03 }; // Nova Lake

    if (!PhCpuIdentityIsIntel(Identity))
        return FALSE;

    return PhpCpuIdentityIsModel(Identity, 0x06, models6, RTL_NUMBER_OF(models6)) ||
        PhpCpuIdentityIsModel(Identity, 0x12, models18, RTL_NUMBER_OF(models18));
}

/**
 * Extracts the current uncore ratio from MSR_UNCORE_PERF_STATUS.
 *
 * \param UncorePerfStatus The MSR value.
 * \return The ratio (bits 6:0), in units of the bus clock.
 */
ULONG NTAPI PhIntelUncoreRatio(
    _In_ ULONG64 UncorePerfStatus
    )
{
    return (ULONG)UncorePerfStatus & 0x7F;
}

/**
 * Decodes the energy status unit from MSR_RAPL_POWER_UNIT (Intel) or RAPL_PWR_UNIT (AMD).
 *
 * \param RaplPowerUnit The MSR value.
 * \param Identity The CPU identity.
 * \return Joules per energy counter increment.
 */
FLOAT NTAPI PhRaplEnergyUnitJoules(
    _In_ ULONG64 RaplPowerUnit,
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    ULONG energyUnit;

    energyUnit = (ULONG)(RaplPowerUnit >> 8) & 0x1F;

    if (PhpIsIntelRaplAtom(Identity))
        return (FLOAT)(1UL << energyUnit) * 0.000001f;

    return 1.0f / (FLOAT)(1UL << energyUnit);
}

/**
 * Decodes the power unit from MSR_RAPL_POWER_UNIT (Intel) or RAPL_PWR_UNIT (AMD).
 *
 * \param RaplPowerUnit The MSR value.
 * \param Identity The CPU identity.
 * \return Watts per power field increment.
 */
FLOAT NTAPI PhRaplPowerUnitWatts(
    _In_ ULONG64 RaplPowerUnit,
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    ULONG powerUnit;

    powerUnit = (ULONG)RaplPowerUnit & 0xF;

    if (PhpIsIntelRaplAtom(Identity))
        return (FLOAT)(1UL << powerUnit) * 0.001f;

    return 1.0f / (FLOAT)(1UL << powerUnit);
}

/**
 * Decodes the energy unit for MSR_DRAM_ENERGY_STATUS.
 *
 * \param RaplPowerUnit The MSR_RAPL_POWER_UNIT value.
 * \param Identity The CPU identity.
 * \return Joules per DRAM energy counter increment.
 */
FLOAT NTAPI PhIntelRaplDramEnergyUnitJoules(
    _In_ ULONG64 RaplPowerUnit,
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    static const UCHAR models[] = { 0x3F, 0x4F, 0x56, 0x55, 0x6A, 0x6C, 0x8F, 0xCF, 0x57, 0x85 };

    if (PhCpuIdentityIsIntel(Identity) && PhpCpuIdentityIsModel(Identity, 0x06, models, RTL_NUMBER_OF(models)))
        return 0.0000153f;

    return PhRaplEnergyUnitJoules(RaplPowerUnit, Identity);
}

/**
 * Decodes the energy unit for MSR_PLATFORM_ENERGY_STATUS (PSYS).
 *
 * \param RaplPowerUnit The MSR_RAPL_POWER_UNIT value.
 * \param Identity The CPU identity.
 * \return Joules per platform energy counter increment.
 */
FLOAT NTAPI PhIntelRaplPlatformEnergyUnitJoules(
    _In_ ULONG64 RaplPowerUnit,
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    static const UCHAR models[] = { 0x8F, 0xCF };

    if (PhCpuIdentityIsIntel(Identity) && PhpCpuIdentityIsModel(Identity, 0x06, models, RTL_NUMBER_OF(models)))
        return 1.0f;

    return PhRaplEnergyUnitJoules(RaplPowerUnit, Identity);
}

/**
 * Extracts the current core ratio from IA32_PERF_STATUS.
 *
 * \param PerfStatus The MSR value.
 * \param Identity The CPU identity.
 * \return The ratio (multiply by the bus clock for MHz), or 0 for a non-Intel identity.
 */
ULONG NTAPI PhIntelPerfStatusRatio(
    _In_ ULONG64 PerfStatus,
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    if (!PhCpuIdentityIsIntel(Identity))
        return 0;

    if (PhpIsIntelNehalem(Identity))
        return (ULONG)PerfStatus & 0xFF;

    return (ULONG)(PerfStatus >> 8) & 0xFF;
}

/**
 * Extracts the core voltage from IA32_PERF_STATUS.
 *
 * \param PerfStatus The MSR value.
 * \param Identity The CPU identity.
 * \return Bits 47:32 in 1/8192 V, or 0 where the field is not documented.
 */
FLOAT NTAPI PhIntelPerfStatusToVolts(
    _In_ ULONG64 PerfStatus,
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    static const UCHAR atomModels[] = { 0x37, 0x4D, 0x4A, 0x5A, 0x4C, 0x75, 0x5C, 0x5F, 0x7A, 0x86, 0x96, 0x9C };

    if (!PhCpuIdentityIsIntel(Identity))
        return 0.0f;
    if (PhpIsIntelNehalem(Identity))
        return 0.0f;
    if (PhpCpuIdentityIsModel(Identity, 0x06, atomModels, RTL_NUMBER_OF(atomModels)))
        return 0.0f;

    return (FLOAT)((ULONG)(PerfStatus >> 32) & 0xFFFF) / 8192.0f;
}

/**
 * Converts SMN THM_TCON_CUR_TMP to Tctl in degrees Celsius.
 *
 * \param ThmTconCurTmp The register value.
 * \param Celsius Receives CUR_TEMP (bits 31:21) x 0.125, minus 49 when
 * CUR_TEMP_RANGE_SEL (bit 19) is set or CUR_TEMP_TJ_SEL (bits 17:16) is 0b11.
 * \return FALSE when the read returned the PCI error response.
 */
_Success_(return)
BOOLEAN NTAPI PhAmdTctlToCelsius(
    _In_ ULONG ThmTconCurTmp,
    _Out_ PFLOAT Celsius
    )
{
    FLOAT celsius;

    if (ThmTconCurTmp == ULONG_MAX)
        return FALSE;

    celsius = (FLOAT)(ThmTconCurTmp >> 21) * 0.125f;

    if ((ThmTconCurTmp & (1UL << 19)) || ((ThmTconCurTmp >> 16) & 0x3) == 0x3)
        celsius -= 49.0f;

    *Celsius = celsius;

    return TRUE;
}

/**
 * Returns the Tctl to Tdie offset for Family 17h SKUs that report an offset Tctl.
 *
 * \param Identity The CPU identity.
 * \param BrandString The processor brand string.
 * \return Degrees Celsius to subtract from Tctl to obtain Tdie, or 0 when Tctl equals Tdie.
 */
FLOAT NTAPI PhAmdTctlOffset(
    _In_ PCPH_CPU_IDENTITY Identity,
    _In_ PCPH_STRINGREF BrandString
    )
{
    static const struct
    {
        PCWSTR Name;
        FLOAT Offset;
    } offsets[] =
    {
        { L"Ryzen 5 1600X", 20.0f },
        { L"Ryzen 7 1700X", 20.0f },
        { L"Ryzen 7 1800X", 20.0f },
        { L"Ryzen 7 2700X", 10.0f },
        { L"Ryzen Threadripper 19", 27.0f },
        { L"Ryzen Threadripper 29", 27.0f },
    };

    if (!PhpCpuIdentityIsVendor(Identity, "AuthenticAMD") || Identity->DisplayFamily != 0x17)
        return 0.0f;

    for (ULONG i = 0; i < RTL_NUMBER_OF(offsets); i++)
    {
        if (PhFindStringInStringRefZ(BrandString, offsets[i].Name, FALSE) != SIZE_MAX)
            return offsets[i].Offset;
    }

    return 0.0f;
}

/**
 * Converts an SMN per-CCD temperature register to degrees Celsius.
 *
 * \param Tccd The register value.
 * \param Celsius Receives bits 10:0 x 0.125 minus 49.
 * \return FALSE when the valid bit (11) is clear or the read returned the PCI error response.
 */
_Success_(return)
BOOLEAN NTAPI PhAmdTccdToCelsius(
    _In_ ULONG Tccd,
    _Out_ PFLOAT Celsius
    )
{
    if (Tccd == ULONG_MAX || !(Tccd & (1UL << 11)))
        return FALSE;

    *Celsius = (FLOAT)(Tccd & 0x7FF) * 0.125f - 49.0f;

    return TRUE;
}

/**
 * Locates the SMN per-CCD temperature registers.
 *
 * \param Identity The CPU identity.
 * \param BaseAddress Receives the SMN address of CCD 0; CCD i is at BaseAddress + 4 * i.
 * \param MaximumCount Receives the number of CCD registers to probe.
 * \return FALSE when the model has no known per-CCD temperature registers.
 */
_Success_(return)
BOOLEAN NTAPI PhAmdTccdAddressRange(
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PULONG BaseAddress,
    _Out_ PULONG MaximumCount
    )
{
    static const UCHAR zen1Models[] = { 0x01, 0x08, 0x11, 0x18 };
    static const UCHAR zen2Models[] = { 0x31, 0x47, 0x60, 0x68, 0x71 };
    static const UCHAR zen3Models[] = { 0x00, 0x01, 0x08, 0x21 };
    ULONG offset;
    ULONG count;

    if (!PhCpuIdentityIsAmd(Identity))
        return FALSE;

    if (
        PhpCpuIdentityIsModel(Identity, 0x17, zen1Models, RTL_NUMBER_OF(zen1Models)) ||
        PhpCpuIdentityIsModel(Identity, 0x18, zen1Models, RTL_NUMBER_OF(zen1Models))
        )
    {
        offset = 0x154;
        count = 4;
    }
    else if (PhpCpuIdentityIsModel(Identity, 0x17, zen2Models, RTL_NUMBER_OF(zen2Models)))
    {
        offset = 0x154;
        count = 8;
    }
    else if (PhpCpuIdentityIsModelRange(Identity, 0x17, 0xA0, 0xAF))
    {
        offset = 0x300;
        count = 8;
    }
    else if (
        PhpCpuIdentityIsModel(Identity, 0x19, zen3Models, RTL_NUMBER_OF(zen3Models)) ||
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0x50, 0x5F)
        )
    {
        offset = 0x154;
        count = 8;
    }
    else if (PhpCpuIdentityIsModelRange(Identity, 0x19, 0x40, 0x4F))
    {
        offset = 0x300;
        count = 8;
    }
    else if (
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0x10, 0x1F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0xA0, 0xAF)
        )
    {
        offset = 0x300;
        count = 12;
    }
    else if (PhpCpuIdentityIsModelRange(Identity, 0x19, 0x60, 0x7F))
    {
        offset = 0x308;
        count = 8;
    }
    else if (PhpCpuIdentityIsModelRange(Identity, 0x1A, 0x00, 0x1F))
    {
        offset = 0x1F0;
        count = 16;
    }
    else if (PhpCpuIdentityIsModelRange(Identity, 0x1A, 0x40, 0x4F))
    {
        offset = 0x308;
        count = 8;
    }
    else
    {
        return FALSE;
    }

    *BaseAddress = PH_AMD_SMN_THM_TCON_CUR_TMP + offset;
    *MaximumCount = count;

    return TRUE;
}

/**
 * Converts pre-Zen ReportedTempCtrl (D18F3xA4 or D0F0xBC_xD8200CA4) to degrees Celsius.
 *
 * \param ReportedTempCtrl The register value.
 * \param Identity The CPU identity.
 * \param Celsius Receives CurTmp (bits 31:21) x 0.125; on Family 15h/16h minus 49
 * when CurTmpTjSel (bits 17:16) is 0b11.
 * \return FALSE for an unsupported family or the PCI error response.
 */
_Success_(return)
BOOLEAN NTAPI PhAmdReportedTempCtrlToCelsius(
    _In_ ULONG ReportedTempCtrl,
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PFLOAT Celsius
    )
{
    ULONG curTmp;
    FLOAT celsius;

    if (!PhpCpuIdentityIsVendor(Identity, "AuthenticAMD"))
        return FALSE;
    if (ReportedTempCtrl == ULONG_MAX)
        return FALSE;

    switch (Identity->DisplayFamily)
    {
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x14:
    case 0x15:
    case 0x16:
        break;
    default:
        return FALSE;
    }

    curTmp = ReportedTempCtrl >> 21;

    if (PhpCpuIdentityIsModelRange(Identity, 0x15, 0x00, 0x0F))
        curTmp &= 0x7FC;

    celsius = (FLOAT)curTmp * 0.125f;

    if ((Identity->DisplayFamily == 0x15 || Identity->DisplayFamily == 0x16) && ((ReportedTempCtrl >> 16) & 0x3) == 0x3)
        celsius -= 49.0f;

    *Celsius = celsius;

    return TRUE;
}

/**
 * Decodes the core frequency from a P-state FID/DID field pair.
 *
 * \param PState PStateDef, HwPstateStatus (Zen) or COFVID_STATUS (pre-Zen).
 * \param Identity The CPU identity.
 * \return The core frequency in MHz, or 0 when undecodable.
 */
static FLOAT PhpAmdCoreCofMhz(
    _In_ ULONG64 PState,
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    ULONG fid;
    ULONG did;

    if (!PhCpuIdentityIsAmd(Identity))
        return 0.0f;

    switch (Identity->DisplayFamily)
    {
    case 0x17:
    case 0x18:
    case 0x19:
        fid = (ULONG)PState & 0xFF;
        did = (ULONG)(PState >> 8) & 0x3F;

        if (did == 0)
            return 0.0f;

        return (FLOAT)fid * 200.0f / (FLOAT)did;
    case 0x1A:
        fid = (ULONG)PState & 0xFFF;

        if (fid <= 0x0F)
            return 0.0f;

        return (FLOAT)fid * 5.0f;
    case 0x10:
    case 0x11:
    case 0x15:
    case 0x16:
        fid = (ULONG)PState & 0x3F;
        did = (ULONG)(PState >> 6) & 0x7;

        return 100.0f * (FLOAT)(fid + 0x10) / (FLOAT)(1UL << did);
    default:
        return 0.0f;
    }
}

/**
 * Decodes the core frequency of a PStateDef MSR.
 *
 * \param PStateDef The MSR value.
 * \param Identity The CPU identity.
 * \return The core frequency in MHz, or 0 when PstateEn (bit 63) is clear or undecodable.
 */
FLOAT NTAPI PhAmdPStateDefToMhz(
    _In_ ULONG64 PStateDef,
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    if (!(PStateDef & (1ULL << 63)))
        return 0.0f;

    return PhpAmdCoreCofMhz(PStateDef, Identity);
}

/**
 * Decodes the current core frequency of HwPstateStatus (Zen) or COFVID_STATUS (pre-Zen).
 *
 * \param HwPStateStatus The MSR value.
 * \param Identity The CPU identity.
 * \return The core frequency in MHz, or 0 when undecodable.
 */
FLOAT NTAPI PhAmdHwPStateStatusToMhz(
    _In_ ULONG64 HwPStateStatus,
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    return PhpAmdCoreCofMhz(HwPStateStatus, Identity);
}

/**
 * Extracts CpuVid from PStateDef, HwPstateStatus (Zen) or COFVID_STATUS (pre-Zen).
 *
 * \param PState The MSR value.
 * \param Identity The CPU identity.
 * \param Vid Receives bits 21:14 (Zen) or 15:9 (pre-Zen).
 * \return FALSE for an unsupported family.
 */
_Success_(return)
BOOLEAN NTAPI PhAmdPStateVid(
    _In_ ULONG64 PState,
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PUCHAR Vid
    )
{
    if (!PhCpuIdentityIsAmd(Identity))
        return FALSE;

    if (PhpIsAmdZen(Identity))
    {
        *Vid = (UCHAR)((PState >> 14) & 0xFF);
        return TRUE;
    }

    if (PhpIsAmdPreZenPStateLayout(Identity))
    {
        *Vid = (UCHAR)((PState >> 9) & 0x7F);
        return TRUE;
    }

    return FALSE;
}

/**
 * Converts a CPU VID to volts.
 *
 * \param Vid The VID.
 * \param Identity The CPU identity.
 * \return The voltage, or 0 where the encoding is not documented.
 */
FLOAT NTAPI PhAmdCpuVidToVolts(
    _In_ UCHAR Vid,
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    if (!PhCpuIdentityIsAmd(Identity))
        return 0.0f;

    if (
        Identity->DisplayFamily == 0x17 ||
        Identity->DisplayFamily == 0x18 ||
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0x00, 0x0F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0x20, 0x5F) ||
        (Identity->DisplayFamily == 0x15 && Identity->DisplayModel >= 0x10) ||
        Identity->DisplayFamily == 0x16
        )
    {
        return 1.55f - 0.00625f * (FLOAT)Vid;
    }

    if (Identity->DisplayFamily == 0x10)
        return 1.55f - 0.0125f * (FLOAT)Vid;

    return 0.0f;
}

/**
 * Returns the SMN SVI2 telemetry plane addresses for the core and SoC rails.
 *
 * \param Identity The CPU identity.
 * \param CoreAddress Receives the core (VDDCR_CPU) plane address.
 * \param SocAddress Receives the SoC (VDDCR_SOC) plane address.
 * \return FALSE when the model has no known SVI2 plane mapping.
 */
_Success_(return)
BOOLEAN NTAPI PhAmdSviPlaneAddresses(
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PULONG CoreAddress,
    _Out_ PULONG SocAddress
    )
{
    static const UCHAR zen1Models[] = { 0x01, 0x08, 0x11, 0x18 };

    if (!PhCpuIdentityIsAmd(Identity))
        return FALSE;

    if (
        PhpCpuIdentityIsModel(Identity, 0x17, zen1Models, RTL_NUMBER_OF(zen1Models)) ||
        PhpCpuIdentityIsModel(Identity, 0x18, zen1Models, RTL_NUMBER_OF(zen1Models))
        )
    {
        *CoreAddress = PH_AMD_SMN_SVI_PLANE0;
        *SocAddress = PH_AMD_SMN_SVI_PLANE1;
        return TRUE;
    }

    if (Identity->DisplayFamily == 0x17 && Identity->DisplayModel == 0x31)
    {
        *CoreAddress = PH_AMD_SMN_SVI_PLANE0_M30H;
        *SocAddress = PH_AMD_SMN_SVI_PLANE1;
        return TRUE;
    }

    if (
        (Identity->DisplayFamily == 0x17 && Identity->DisplayModel == 0x71) ||
        (Identity->DisplayFamily == 0x19 && Identity->DisplayModel == 0x21)
        )
    {
        *CoreAddress = PH_AMD_SMN_SVI_PLANE1;
        *SocAddress = PH_AMD_SMN_SVI_PLANE0;
        return TRUE;
    }

    return FALSE;
}

/**
 * Converts an SMN SVI2 telemetry plane register to volts.
 *
 * \param SviTfn The SVI0_TFN register value.
 * \param PlaneAddress The SMN address the telemetry was read from.
 * \param PlaneTelemetry The telemetry register value.
 * \param Volts Receives 1.55 - 0.00625 x VID (bits 23:16).
 * \return FALSE when the plane is flagged unavailable in SVI0_TFN or the read returned the PCI error response.
 */
_Success_(return)
BOOLEAN NTAPI PhAmdSviTelemetryToVolts(
    _In_ ULONG SviTfn,
    _In_ ULONG PlaneAddress,
    _In_ ULONG PlaneTelemetry,
    _Out_ PFLOAT Volts
    )
{
    ULONG unavailableBit;

    switch (PlaneAddress)
    {
    case PH_AMD_SMN_SVI_PLANE0:
    case PH_AMD_SMN_SVI_PLANE0_M30H:
        unavailableBit = 1UL << 0;
        break;
    case PH_AMD_SMN_SVI_PLANE1:
        unavailableBit = 1UL << 1;
        break;
    default:
        return FALSE;
    }

    if (SviTfn == ULONG_MAX || PlaneTelemetry == ULONG_MAX)
        return FALSE;
    if (SviTfn & unavailableBit)
        return FALSE;

    *Volts = 1.55f - 0.00625f * (FLOAT)((PlaneTelemetry >> 16) & 0xFF);

    return TRUE;
}

typedef enum _PHP_AMD_UMC_FORMAT
{
    PhpAmdUmcFormatUnknown,
    PhpAmdUmcFormatDdr4, // DDR4 and LPDDR4: MEMCLK ratio in bits 6:0
    PhpAmdUmcFormatDdr5 // DDR5 and LPDDR5: MEMCLK in bits 15:0, WckRatioMode in bits 27:26
} PHP_AMD_UMC_FORMAT;

/**
 * Determines the DramConfiguration layout of the memory controller generation.
 */
static PHP_AMD_UMC_FORMAT PhpAmdUmcFormat(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    if (!PhCpuIdentityIsAmd(Identity))
        return PhpAmdUmcFormatUnknown;

    if (
        PhpCpuIdentityIsModelRange(Identity, 0x17, 0x00, 0x3F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x18, 0x00, 0x0F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x17, 0x60, 0x7F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0x00, 0x0F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0x20, 0x2F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0x50, 0x5F)
        )
    {
        return PhpAmdUmcFormatDdr4;
    }

    if (
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0x10, 0x1F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0x40, 0x4F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0x60, 0x7F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0xA0, 0xAF) ||
        PhpCpuIdentityIsModelRange(Identity, 0x1A, 0x00, 0x2F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x1A, 0x40, 0x4F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x1A, 0x60, 0x7F)
        )
    {
        return PhpAmdUmcFormatDdr5;
    }

    return PhpAmdUmcFormatUnknown;
}

/**
 * Retrieves the number of UMC channels to search for an active channel.
 *
 * \param Identity The CPU identity.
 * \param Count Receives the number of channels.
 * \return FALSE when the DramConfiguration layout of the model is unknown.
 */
_Success_(return)
BOOLEAN NTAPI PhAmdUmcChannelCount(
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PULONG Count
    )
{
    if (PhpAmdUmcFormat(Identity) == PhpAmdUmcFormatUnknown)
        return FALSE;

    if (
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0x10, 0x1F) ||
        PhpCpuIdentityIsModelRange(Identity, 0x19, 0xA0, 0xAF) ||
        PhpCpuIdentityIsModelRange(Identity, 0x1A, 0x00, 0x1F)
        )
    {
        *Count = 12;
    }
    else
    {
        *Count = 8;
    }

    return TRUE;
}

/**
 * Determines whether the memory controller generation has the DDR5 register layout.
 *
 * \param Identity The CPU identity.
 * \return TRUE for the DDR5 and LPDDR5 generations.
 */
BOOLEAN NTAPI PhAmdUmcIsDdr5Layout(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    return PhpAmdUmcFormat(Identity) == PhpAmdUmcFormatDdr5;
}

/**
 * Converts UMC DramConfiguration to the memory clock, assuming the nominal 100 MHz reference clock.
 *
 * \param DramConfiguration The register value.
 * \param Identity The CPU identity.
 * \param MemoryClockMhz Receives MEMCLK in MHz.
 * \param DataRate Receives the data rate in MT/s.
 * \return FALSE for an unknown layout, a zero clock or the PCI error response.
 */
_Success_(return)
BOOLEAN NTAPI PhAmdDramConfigurationToMhz(
    _In_ ULONG DramConfiguration,
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PFLOAT MemoryClockMhz,
    _Out_ PFLOAT DataRate
    )
{
    FLOAT mhz;
    ULONG transfers;

    if (DramConfiguration == ULONG_MAX)
        return FALSE;

    switch (PhpAmdUmcFormat(Identity))
    {
    case PhpAmdUmcFormatDdr4:
        {
            mhz = (FLOAT)(DramConfiguration & 0x7F) * 100.0f / 3.0f;
            transfers = 2;
        }
        break;
    case PhpAmdUmcFormatDdr5:
        {
            mhz = (FLOAT)(DramConfiguration & 0xFFFF);

            switch ((DramConfiguration >> 26) & 0x3)
            {
            case 0:
                transfers = 2;
                break;
            case 1:
                transfers = 4;
                break;
            case 2:
                transfers = 8;
                break;
            default:
                return FALSE;
            }
        }
        break;
    default:
        return FALSE;
    }

    if (mhz <= 0.0f)
        return FALSE;

    *MemoryClockMhz = mhz;
    *DataRate = mhz * (FLOAT)transfers;

    return TRUE;
}

// Intel memory clock formats (memtest86+ get_imc_config_intel_*)
typedef enum _PHP_INTEL_MEMORY_CLOCK_FORMAT
{
    PhpIntelMemoryClockFormatNone,
    PhpIntelMemoryClockFormatSnb,
    PhpIntelMemoryClockFormatSkl,
    PhpIntelMemoryClockFormatIcl,
    PhpIntelMemoryClockFormatAdl,
    PhpIntelMemoryClockFormatMtl,
    PhpIntelMemoryClockFormatLnl
} PHP_INTEL_MEMORY_CLOCK_FORMAT;

static PHP_INTEL_MEMORY_CLOCK_FORMAT PhpIntelMemoryClockFormat(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    static const UCHAR snbModels[] = { 0x2A, 0x3A, 0x3C, 0x45, 0x46, 0x3D, 0x47 };
    static const UCHAR sklModels[] = { 0x4E, 0x5E, 0x8E, 0x9E, 0xA5, 0xA6, 0x66 };
    static const UCHAR iclModels[] = { 0x7D, 0x7E, 0x9D, 0x8C, 0x8D, 0xA7 };
    static const UCHAR adlModels[] = { 0x97, 0x9A, 0xB7, 0xBA, 0xBF };
    static const UCHAR mtlModels[] = { 0xAA, 0xAC, 0xC5, 0xC6, 0xB5, 0xCC, 0xD5 }; // Meteor, Arrow, Panther and Wildcat Lake
    static const UCHAR lnlModels[] = { 0xBD }; // Lunar Lake

    if (!PhCpuIdentityIsIntel(Identity))
        return PhpIntelMemoryClockFormatNone;

    if (PhpCpuIdentityIsModel(Identity, 0x06, snbModels, RTL_NUMBER_OF(snbModels)))
        return PhpIntelMemoryClockFormatSnb;
    if (PhpCpuIdentityIsModel(Identity, 0x06, sklModels, RTL_NUMBER_OF(sklModels)))
        return PhpIntelMemoryClockFormatSkl;
    if (PhpCpuIdentityIsModel(Identity, 0x06, iclModels, RTL_NUMBER_OF(iclModels)))
        return PhpIntelMemoryClockFormatIcl;
    if (PhpCpuIdentityIsModel(Identity, 0x06, adlModels, RTL_NUMBER_OF(adlModels)))
        return PhpIntelMemoryClockFormatAdl;
    if (PhpCpuIdentityIsModel(Identity, 0x06, mtlModels, RTL_NUMBER_OF(mtlModels)))
        return PhpIntelMemoryClockFormatMtl;
    if (PhpCpuIdentityIsModel(Identity, 0x06, lnlModels, RTL_NUMBER_OF(lnlModels)))
        return PhpIntelMemoryClockFormatLnl;

    return PhpIntelMemoryClockFormatNone;
}

/**
 * Converts BCLK_FREQ (MCHBAR 0x5F60, kHz) to MHz.
 *
 * \return The BCLK in MHz, or 100 when the register is unavailable or outside 50 to 300 MHz.
 */
static FLOAT PhpIntelBclkMhz(
    _In_opt_ const ULONG* BclkFreq
    )
{
    if (BclkFreq && *BclkFreq >= 50000 && *BclkFreq <= 300000)
        return (FLOAT)*BclkFreq / 1000.0f;

    return 100.0f;
}

/**
 * Decodes the memory clock from the MCHBAR clock registers of an Intel client host bridge.
 *
 * \param Identity The CPU identity.
 * \param McBiosReq MC_BIOS_REQ (MCHBAR 0x5E00), or NULL when unavailable.
 * \param McBiosData MC_BIOS_DATA (MCHBAR 0x5E04), or NULL when unavailable.
 * \param SaPerfStatus The low dword of SA_PERF_STATUS (MCHBAR 0x5918), or NULL when unavailable.
 * \param BclkFreq BCLK_FREQ (MCHBAR 0x5F60) in kHz, or NULL to assume 100 MHz. Used by the ICL and ADL formats only.
 * \param MemssBiosData MEMSS_PMA_CR_BIOS_DATA (MCHBAR 0x13D10), or NULL when unavailable. Used by the MTL format only.
 * \param MemoryClockMhz Receives the DRAM clock, half the data rate.
 * \param DataRate Receives the data rate in MT/s.
 * \param Gear Receives the gear (1, 2 or 4), or 0 when the format has none.
 * \return FALSE when the format is unknown, a needed register is unavailable, or the result is implausible.
 */
_Success_(return)
BOOLEAN NTAPI PhIntelMchbarToMemoryClock(
    _In_ PCPH_CPU_IDENTITY Identity,
    _In_opt_ const ULONG* McBiosReq,
    _In_opt_ const ULONG* McBiosData,
    _In_opt_ const ULONG* SaPerfStatus,
    _In_opt_ const ULONG* BclkFreq,
    _In_opt_ const ULONG* MemssBiosData,
    _Out_ PFLOAT MemoryClockMhz,
    _Out_ PFLOAT DataRate,
    _Out_ PULONG Gear
    )
{
    FLOAT dataRate;
    ULONG gear = 0;

    switch (PhpIntelMemoryClockFormat(Identity))
    {
    case PhpIntelMemoryClockFormatSnb:
        {
            // MT/s = MC_BIOS_DATA[4:0] x (bit 8 ? 100 : 133.33) / 100 x BCLK x 2
            if (!McBiosData)
                return FALSE;

            dataRate = (FLOAT)(*McBiosData & 0x1F) * ((*McBiosData & 0x100) ? 100.0f : 400.0f / 3.0f) * 2.0f;
        }
        break;
    case PhpIntelMemoryClockFormatSkl:
        {
            // MT/s = MC_BIOS_REQ[3:0] x 133.33 / 100 x BCLK x 2, ratio below 3 invalid
            if (!McBiosReq || (*McBiosReq & 0xF) < 3)
                return FALSE;

            dataRate = (FLOAT)(*McBiosReq & 0xF) * (400.0f / 3.0f) * 2.0f;
        }
        break;
    case PhpIntelMemoryClockFormatIcl:
        {
            // MT/s = MC_BIOS_DATA[7:0] x BCLK x (bit 16 ? 2 : 1) x ([11:8] == 0 ? 133.33 / 100 : 1)
            if (!McBiosData)
                return FALSE;

            gear = (*McBiosData & 0x10000) ? 2 : 1;
            dataRate = (FLOAT)(*McBiosData & 0xFF) * PhpIntelBclkMhz(BclkFreq) * (FLOAT)gear;

            if (!(*McBiosData & 0xF00))
                dataRate *= 4.0f / 3.0f;
        }
        break;
    case PhpIntelMemoryClockFormatAdl:
        {
            // MT/s = SA_PERF_STATUS[9:2] x BCLK << MC_BIOS_DATA[13:12] x (MC_BIOS_DATA[11:8] == 0 ? 133.33 / 100 : 1)
            if (!SaPerfStatus || !McBiosData || ((*McBiosData >> 12) & 0x3) == 0x3)
                return FALSE;

            gear = 1UL << ((*McBiosData >> 12) & 0x3);
            dataRate = (FLOAT)((*SaPerfStatus >> 2) & 0xFF) * PhpIntelBclkMhz(BclkFreq) * (FLOAT)gear;

            if (!(*McBiosData & 0xF00))
                dataRate *= 4.0f / 3.0f;
        }
        break;
    case PhpIntelMemoryClockFormatMtl:
        {
            // MT/s = MEMSS_PMA_CR_BIOS_DATA[7:0] x 33.33 (QCLK) x (bit 8 ? 4 : 2) x 2; locked after training
            if (!MemssBiosData)
                return FALSE;

            gear = (*MemssBiosData & 0x100) ? 4 : 2;
            dataRate = (FLOAT)(*MemssBiosData & 0xFF) * (100.0f / 3.0f) * (FLOAT)gear * 2.0f;
        }
        break;
    case PhpIntelMemoryClockFormatLnl:
        {
            // MT/s = SA_PERF_STATUS0[9:2] x 33.33 (QCLK) x (1 << [11:10]) x 2; valid after training
            if (!SaPerfStatus || ((*SaPerfStatus >> 10) & 0x3) == 0x3)
                return FALSE;

            gear = 1UL << ((*SaPerfStatus >> 10) & 0x3);
            dataRate = (FLOAT)((*SaPerfStatus >> 2) & 0xFF) * (100.0f / 3.0f) * (FLOAT)gear * 2.0f;
        }
        break;
    default:
        return FALSE;
    }

    if (dataRate < 800.0f || dataRate > 12000.0f)
        return FALSE;

    *MemoryClockMhz = dataRate / 2.0f;
    *DataRate = dataRate;
    *Gear = gear;

    return TRUE;
}

/**
 * Decodes a processor-thermal temperature register (package, PP0 or PP1).
 *
 * \param Temperature The register value; bits 7:0 are degrees Celsius.
 * \param Celsius Receives the temperature.
 * \return FALSE for zero, all ones or a value above 150 degrees.
 */
_Success_(return)
BOOLEAN NTAPI PhIntelMmioTemperatureToCelsius(
    _In_ ULONG Temperature,
    _Out_ PFLOAT Celsius
    )
{
    ULONG value;

    if (Temperature == ULONG_MAX)
        return FALSE;

    value = Temperature & 0xFF;

    if (value == 0 || value > 150)
        return FALSE;

    *Celsius = (FLOAT)value;
    return TRUE;
}

/**
 * Decodes the TEMP register of the Intel PCH thermal sensor.
 *
 * \param Temp The register value.
 * \param Celsius Receives (Temp & 0x1FF) / 2 - 50.
 * \return FALSE for all ones or a value outside -40 to 150 degrees.
 */
_Success_(return)
BOOLEAN NTAPI PhIntelPchTemperatureToCelsius(
    _In_ USHORT Temp,
    _Out_ PFLOAT Celsius
    )
{
    FLOAT celsius;

    if (Temp == USHRT_MAX)
        return FALSE;

    celsius = (FLOAT)(Temp & 0x1FF) / 2.0f - 50.0f;

    if (celsius < -40.0f || celsius > 150.0f)
        return FALSE;

    *Celsius = celsius;
    return TRUE;
}

/**
 * Computes the effective clock, including idle time, from an APERF delta.
 *
 * \param DeltaAperf The APERF delta.
 * \param Seconds The interval of the delta in seconds.
 * \return DeltaAperf / Seconds in MHz, or 0 when Seconds is not positive.
 */
FLOAT NTAPI PhEffectiveMhzFromAperf(
    _In_ ULONG64 DeltaAperf,
    _In_ FLOAT Seconds
    )
{
    if (Seconds <= 0.0f)
        return 0.0f;

    return (FLOAT)DeltaAperf / Seconds / 1000000.0f;
}

/**
 * Computes the active (non-idle) clock from APERF/MPERF deltas.
 *
 * \param DeltaAperf The APERF delta.
 * \param DeltaMperf The MPERF delta.
 * \param TscMhz The MPERF frequency in MHz.
 * \return TscMhz x DeltaAperf / DeltaMperf, or 0 when DeltaMperf is 0.
 */
FLOAT NTAPI PhActiveMhzFromAperfMperf(
    _In_ ULONG64 DeltaAperf,
    _In_ ULONG64 DeltaMperf,
    _In_ FLOAT TscMhz
    )
{
    if (DeltaMperf == 0)
        return 0.0f;

    return TscMhz * (FLOAT)DeltaAperf / (FLOAT)DeltaMperf;
}

/**
 * Computes the clocks of an Arm core from two samples of its cycle counters.
 *
 * \details Each sample must be read back to back on the core.
 *
 * \param Last The earlier sample.
 * \param Current The later sample.
 * \param CounterHz The system counter frequency (CNTFRQ_EL0).
 * \param Constant TRUE when ConstantCycles holds CNT_CYCLES, FALSE when only Cycles is read.
 * \param EffectiveMhz Receives the cycles over the interval, including idle time.
 * \param ActiveMhz Receives the clock while not idle; 0 without Constant.
 * \return PhArmClockValid, PhArmClockStalled or PhArmClockInvalid.
 */
_Success_(return == PhArmClockValid)
PH_ARM_CLOCK_STATUS NTAPI PhArmCycleClocks(
    _In_ PCPH_ARM_CYCLE_SAMPLE Last,
    _In_ PCPH_ARM_CYCLE_SAMPLE Current,
    _In_ ULONG64 CounterHz,
    _In_ BOOLEAN Constant,
    _Out_ PFLOAT EffectiveMhz,
    _Out_ PFLOAT ActiveMhz
    )
{
    ULONG64 deltaCounter;
    ULONG64 deltaCycles;
    ULONG64 deltaConstant;
    FLOAT effectiveMhz;
    FLOAT activeMhz;

    if (CounterHz == 0)
        return PhArmClockInvalid;
    if (Current->Counter <= Last->Counter || Current->Cycles < Last->Cycles)
        return PhArmClockInvalid;
    if (Constant && Current->ConstantCycles < Last->ConstantCycles)
        return PhArmClockInvalid;

    deltaCounter = Current->Counter - Last->Counter;
    deltaCycles = Current->Cycles - Last->Cycles;
    deltaConstant = Constant ? Current->ConstantCycles - Last->ConstantCycles : 0;

    if (deltaCycles == 0 || (Constant && deltaConstant == 0))
        return PhArmClockStalled;

    // CNT_CYCLES cannot outrun the system counter; allow 1% read skew.
    if (deltaConstant > deltaCounter + deltaCounter / 100)
        return PhArmClockInvalid;

    effectiveMhz = PhEffectiveMhzFromAperf(deltaCycles, (FLOAT)deltaCounter / (FLOAT)CounterHz);
    activeMhz = Constant ? PhActiveMhzFromAperfMperf(deltaCycles, deltaConstant, (FLOAT)CounterHz / 1000000.0f) : 0.0f;

    if (effectiveMhz > PH_ARM_MAX_CLOCK_MHZ || activeMhz > PH_ARM_MAX_CLOCK_MHZ)
        return PhArmClockInvalid;

    *EffectiveMhz = effectiveMhz;
    *ActiveMhz = activeMhz;

    return PhArmClockValid;
}
