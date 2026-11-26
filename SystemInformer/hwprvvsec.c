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

// Collector vsec: the Intel OOBMSM functions' TPMI regions (RAPL energy where the processor's MSRs do not serve, the
// uncore clocks and their limits) and the PMT functions' telemetry groups the sample table knows; one KphHwVsecRead a
// kept region or group every update. TPMI SST and PLR are read only for a report.

#define PHP_HW_VSEC_MAX_DEVICES 16
#define PHP_HW_VSEC_LIST_REGIONS 128 // a region list's first answer; more are asked for once
#define PHP_HW_VSEC_MAX_REGIONS 64
#define PHP_HW_VSEC_REGION_ENTRIES 32 // RAPL domains of a 4 KiB instance, samples of a PMT group
#define PHP_HW_VSEC_STRIKE_LIMIT 3 // failures of a region or an entry before it ends for the session
#define PHP_HW_VSEC_ENERGY_MAX_WATTS 10000.0f // above this an energy counter restarted
#define PHP_HW_VSEC_UNMAPPED_ID 0x80000000 // named sensor id bit of uncore domains not mapped to a package
#define PHP_HW_VSEC_PMT_ID_BASE 0x50000000 // named sensor ids of the PMT samples: | device << 16 | group << 8 | sample
#define PHP_HW_VSEC_UNCORE_MIN_ID 0x01000000 // named sensor id bit of an uncore cluster's minimum
#define PHP_HW_VSEC_UNCORE_MAX_ID 0x02000000 // named sensor id bit of an uncore cluster's maximum

typedef enum _PHP_HW_VSEC_KIND
{
    PhpHwVsecPackageEnergy,
    PhpHwVsecDramEnergy,
    PhpHwVsecPlatformEnergy,
    PhpHwVsecEnergyMaximum,
    PhpHwVsecUncoreStatus = PhpHwVsecEnergyMaximum,
    PhpHwVsecUncoreControl,
    PhpHwVsecPmtSample
} PHP_HW_VSEC_KIND;

typedef enum _PHP_HW_PMT_TRANSFORM
{
    PhpHwPmtTemperature, // signed 8-bit degrees Celsius
    PhpHwPmtRatio100, // x 100 MHz
    PhpHwPmtRatio33 // x 33.33 MHz
} PHP_HW_PMT_TRANSFORM;

// Intel PMT telemetry samples by aggregator GUID: qword index, field, transform.
typedef struct _PHP_HW_PMT_SAMPLE
{
    ULONG Guid;
    USHORT Qword;
    UCHAR Lsb;
    UCHAR Transform; // PHP_HW_PMT_TRANSFORM
    PCWSTR Name;
} PHP_HW_PMT_SAMPLE, *PPHP_HW_PMT_SAMPLE;
typedef const PHP_HW_PMT_SAMPLE* PCPHP_HW_PMT_SAMPLE;

static CONST PHP_HW_PMT_SAMPLE PhpHwPmtSamples[] =
{
    // MTL PUNIT normal aggregator.
    { 0x130670B2, 11, 48, PhpHwPmtRatio33, L"Memory clock (QCLK)" },
    { 0x130670B2, 19, 0, PhpHwPmtTemperature, L"PCH" },
    { 0x130670B2, 19, 8, PhpHwPmtTemperature, L"DRAM" },
    { 0x130670B2, 19, 16, PhpHwPmtTemperature, L"System agent" },
    { 0x130670B2, 19, 24, PhpHwPmtTemperature, L"IPU" },
    { 0x130670B2, 19, 32, PhpHwPmtTemperature, L"Display engine" },
    { 0x130670B2, 19, 40, PhpHwPmtTemperature, L"VPU" },
    { 0x130670B2, 19, 48, PhpHwPmtTemperature, L"Media" },
    // MTL DMU normal aggregator: CORE_TEMPERATURE0-5, 32-bit DTS0-3 groups from q9 [63:32]; DTS1 is the maximum.
    { 0x1A067002, 9, 40, PhpHwPmtTemperature, L"P-core 0" },
    { 0x1A067002, 10, 8, PhpHwPmtTemperature, L"P-core 1" },
    { 0x1A067002, 10, 40, PhpHwPmtTemperature, L"P-core 2" },
    { 0x1A067002, 11, 8, PhpHwPmtTemperature, L"P-core 3" },
    { 0x1A067002, 11, 40, PhpHwPmtTemperature, L"P-core 4" },
    { 0x1A067002, 12, 8, PhpHwPmtTemperature, L"P-core 5" },
    // LNL.
    { 0x03072005, 2, 0, PhpHwPmtRatio33, L"Memory clock (MEMSS)" },
    { 0x03072005, 14, 0, PhpHwPmtTemperature, L"PCH" },
    { 0x03072005, 14, 16, PhpHwPmtTemperature, L"System agent" },
    { 0x03072005, 14, 24, PhpHwPmtTemperature, L"IPU" },
    { 0x03072005, 14, 32, PhpHwPmtTemperature, L"Display engine" },
    { 0x03072005, 14, 40, PhpHwPmtTemperature, L"VPU" },
    { 0x03072005, 14, 48, PhpHwPmtTemperature, L"Media" },
    { 0x03072005, 28, 0, PhpHwPmtRatio100, L"Core 0 average" },
    { 0x03072005, 28, 8, PhpHwPmtRatio100, L"Core 1 average" },
    { 0x03072005, 28, 16, PhpHwPmtRatio100, L"Core 2 average" },
    { 0x03072005, 28, 24, PhpHwPmtRatio100, L"Core 3 average" },
    { 0x03072005, 28, 32, PhpHwPmtRatio100, L"Core 4 average" },
    { 0x03072005, 28, 40, PhpHwPmtRatio100, L"Core 5 average" },
    { 0x03072005, 28, 48, PhpHwPmtRatio100, L"Core 6 average" },
    { 0x03072005, 28, 56, PhpHwPmtRatio100, L"Core 7 average" },
    // PTL and WCL share the q15 temperatures; PTL q2 is reserved.
    { 0x03086000, 15, 0, PhpHwPmtTemperature, L"PCH" },
    { 0x03086000, 15, 8, PhpHwPmtTemperature, L"System agent" },
    { 0x03086000, 15, 16, PhpHwPmtTemperature, L"IPU" },
    { 0x03086000, 15, 24, PhpHwPmtTemperature, L"Display engine" },
    { 0x03086000, 15, 32, PhpHwPmtTemperature, L"VPU" },
    { 0x03086000, 15, 40, PhpHwPmtTemperature, L"Media" },
    { 0x0308D000, 2, 0, PhpHwPmtRatio33, L"Memory clock (MEMSS)" },
    { 0x0308D000, 15, 0, PhpHwPmtTemperature, L"PCH" },
    { 0x0308D000, 15, 8, PhpHwPmtTemperature, L"System agent" },
    { 0x0308D000, 15, 16, PhpHwPmtTemperature, L"IPU" },
    { 0x0308D000, 15, 24, PhpHwPmtTemperature, L"Display engine" },
    { 0x0308D000, 15, 32, PhpHwPmtTemperature, L"VPU" },
    { 0x0308D000, 15, 40, PhpHwPmtTemperature, L"Media" },
};

typedef struct _PHP_HW_VSEC_ENTRY
{
    ULONG Offset; // qword, relative to the region
    PHP_HW_VSEC_KIND Kind;
    ULONG PackageIndex; // ULONG_MAX when not mapped to a package
    ULONG NamedIndex; // ULONG_MAX when the value feeds a package sensor; an uncore control's minimum
    ULONG MaximumIndex; // an uncore control's maximum
    FLOAT EnergyUnit; // joules
    ULONG Sample; // PMT: index in PhpHwPmtSamples
    BOOLEAN Disabled;
    UCHAR Strikes;
    BOOLEAN Valid;
    BOOLEAN LastValid;
    ULONG64 Value;
    ULONG64 LastValue;
} PHP_HW_VSEC_ENTRY, *PPHP_HW_VSEC_ENTRY;

// A package's RAPL sums of a pass: each kind's watts, its domains still read and the domains that delivered.
typedef struct _PHP_HW_VSEC_PACKAGE
{
    FLOAT Power[PhpHwVsecEnergyMaximum];
    ULONG Required[PhpHwVsecEnergyMaximum];
    ULONG Delivered[PhpHwVsecEnergyMaximum];
} PHP_HW_VSEC_PACKAGE, *PPHP_HW_VSEC_PACKAGE;

typedef struct _PHP_HW_VSEC_REGION
{
    ULONG Device; // in PhpHwVsecLocators
    ULONG Operation;
    ULONG Discovery;
    ULONG Id;
    ULONG Position;
    BOOLEAN Disabled;
    BOOLEAN Missed; // answered STATUS_CONTEXT_MISMATCH since its device was last listed
    UCHAR Strikes;
    ULONG EntryCount;
    PHP_HW_VSEC_ENTRY Entries[PHP_HW_VSEC_REGION_ENTRIES];
    KPH_HW_WINDOW_ITEM Items[PHP_HW_VSEC_REGION_ENTRIES];
} PHP_HW_VSEC_REGION, *PPHP_HW_VSEC_REGION;

static ULONG PhpHwVsecLocators[PHP_HW_VSEC_MAX_DEVICES];
static ULONG PhpHwVsecDeviceCount = 0;
static ULONG PhpHwVsecOobmsmCount = 0; // the OOBMSM functions lead PhpHwVsecLocators; the PMT functions follow
static PPHP_HW_VSEC_REGION PhpHwVsecRegions = NULL;
static ULONG PhpHwVsecRegionCount = 0;
static BOOLEAN PhpHwVsecRelist[PHP_HW_VSEC_MAX_DEVICES]; // a region of the device missed: list it again next update
static PPHP_HW_VSEC_PACKAGE PhpHwVsecPackages = NULL; // per package

// A TPMI SST or PLR instance, read for the report.
typedef struct _PHP_HW_VSEC_REPORT
{
    ULONG Device; // in PhpHwVsecLocators
    KPH_HW_VSEC_REGION Region;
    ULONG PackageIndex; // ULONG_MAX when not mapped to a package
    BOOLEAN Read; // the last report's reads gave a reading
    UCHAR Version; // SST interface version
    UCHAR Level; // SST-PP current level
    BOOLEAN Locked;
    ULONG64 Value; // SST: level INFO_1 (TDP, TjMax); PLR: die-level reasons
    ULONG64 Ratios; // SST: level INFO_11
} PHP_HW_VSEC_REPORT, *PPHP_HW_VSEC_REPORT;

static PHP_HW_VSEC_REPORT PhpHwVsecReports[PHP_HW_VSEC_MAX_REGIONS];
static ULONG PhpHwVsecReportCount = 0;
static ULONG PhpHwVsecReportsOmitted = 0;
static ULONG64 PhpHwVsecSampleTime = 0;
static ULONG64 PhpHwVsecLastSampleTime = 0;
static KPH_HW_VALUE64 PhpHwVsecValues[PHP_HW_VSEC_REGION_ENTRIES * 2];

/**
 * Applies the session failure policy to a region or an entry: a refused one ends at once; an asleep device or
 * contention is no value, not a failure; anything else strikes.
 *
 * \return TRUE when the value is usable.
 */
static BOOLEAN PhpHwVsecApplyStatus(
    _Inout_ PBOOLEAN Disabled,
    _Inout_ PUCHAR Strikes,
    _In_ NTSTATUS Status
    )
{
    if (NT_SUCCESS(Status))
    {
        *Strikes = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_RETRY:
    case STATUS_DEVICE_NOT_READY:
    case STATUS_DEVICE_POWERED_OFF:
    case STATUS_CANCELLED:
        break;
    case STATUS_ACCESS_DENIED:
    case STATUS_NOT_SUPPORTED:
    case STATUS_DEVICE_CONFIGURATION_ERROR:
        *Disabled = TRUE;
        break;
    default:
        if (++*Strikes >= PHP_HW_VSEC_STRIKE_LIMIT)
            *Disabled = TRUE;
        break;
    }

    return FALSE;
}

/**
 * Reads qwords of a region at the probe; a call that fails gives its status to every item.
 */
static VOID PhpHwVsecReadOnce(
    _In_ ULONG Locator,
    _In_ const KPH_HW_VSEC_REGION* Region,
    _In_reads_(Count) const ULONG* Offsets,
    _In_ ULONG Count
    )
{
    KPH_HW_WINDOW_ITEM items[PHP_HW_VSEC_REGION_ENTRIES * 2];
    NTSTATUS status;

    for (ULONG i = 0; i < Count; i++)
    {
        items[i].Offset = Offsets[i];
        items[i].Width = sizeof(ULONG64);
    }

    status = KphHwVsecRead(Locator, Region->Operation, Region->Position, Region->Id, items, Count, PhpHwVsecValues);

    for (ULONG i = 0; !NT_SUCCESS(status) && i < Count; i++)
    {
        PhpHwVsecValues[i].Status = status;
        PhpHwVsecValues[i].Value = 0;
    }
}

static BOOLEAN PhpHwVsecValue(
    _In_ ULONG Index,
    _Out_ PULONG64 Value
    )
{
    *Value = PhpHwVsecValues[Index].Value;
    return NT_SUCCESS(PhpHwVsecValues[Index].Status) && *Value != ULONG64_MAX;
}

/**
 * Reads the values of a region for the report; FALSE when any is missing or reads all ones.
 */
static BOOLEAN PhpHwVsecReadReport(
    _In_ PPHP_HW_VSEC_REPORT Report,
    _In_reads_(Count) const ULONG* Offsets,
    _In_ ULONG Count
    )
{
    ULONG64 value;

    PhpHwVsecReadOnce(PhpHwVsecLocators[Report->Device], &Report->Region, Offsets, Count);

    for (ULONG i = 0; i < Count; i++)
    {
        if (!PhpHwVsecValue(i, &value))
            return FALSE;
    }

    return TRUE;
}

/**
 * Reads the active SST-PP level of a TPMI SST instance: its TDP, TjMax and core ratios.
 */
static VOID PhpHwVsecReadSst(
    _Inout_ PPHP_HW_VSEC_REPORT Report
    )
{
    static CONST ULONG offset = 0;
    ULONG offsets[3];
    ULONG64 header;
    ULONG pp;
    ULONG level;

    PhpHwVsecReadOnce(PhpHwVsecLocators[Report->Device], &Report->Region, &offset, 1);

    // Interface version [7:0]: FFh is invalid, major [7:5] must be 0. SST-PP is cap bit 9.
    if (!NT_SUCCESS(PhpHwVsecValues[0].Status))
        return;

    header = PhpHwVsecValues[0].Value;

    if ((UCHAR)header == 0xFF || ((header >> 5) & 0x7) || !(header & 0x200))
        return;

    pp = (ULONG)((header >> 24) & 0xFF) * 8;
    offsets[0] = pp + PH_INTEL_TPMI_SST_PP_FEATURES;
    offsets[1] = pp + PH_INTEL_TPMI_SST_PP_LEVELS;
    offsets[2] = pp + PH_INTEL_TPMI_SST_PP_STATUS;

    if (!PhpHwVsecReadReport(Report, offsets, 3))
        return;

    level = (ULONG)(PhpHwVsecValues[2].Value & 0x7);

    if (level > PH_INTEL_TPMI_SST_PP_MAX_LEVEL)
        return;

    Report->Version = (UCHAR)header;
    Report->Level = (UCHAR)level;
    Report->Locked = !!(PhpHwVsecValues[2].Value & 0x8);

    // Level n is at byte n of the level offsets, the PP feature at byte 0 of the feature offsets; both in qwords.
    pp += (ULONG)((PhpHwVsecValues[1].Value >> (level * 8)) & 0xFF) * 8 + (ULONG)(PhpHwVsecValues[0].Value & 0xFF) * 8;
    offsets[0] = pp + PH_INTEL_TPMI_SST_PP_INFO_1;
    offsets[1] = pp + PH_INTEL_TPMI_SST_PP_INFO_11;

    if (!PhpHwVsecReadReport(Report, offsets, 2))
        return;

    Report->Value = PhpHwVsecValues[0].Value;
    Report->Ratios = PhpHwVsecValues[1].Value;
    Report->Read = TRUE;
}

/**
 * Reads the die-level reasons of a TPMI PLR instance; they hold until cleared. An all-ones header is an invalid
 * instance.
 */
static VOID PhpHwVsecReadPlr(
    _Inout_ PPHP_HW_VSEC_REPORT Report
    )
{
    static CONST ULONG offsets[2] = { 0, PH_INTEL_TPMI_PLR_DIE_LEVEL };

    PhpHwVsecReadOnce(PhpHwVsecLocators[Report->Device], &Report->Region, offsets, 2);

    if (!PhpHwVsecValue(0, &Report->Value) || !NT_SUCCESS(PhpHwVsecValues[1].Status))
        return;

    Report->Value = PhpHwVsecValues[1].Value;
    Report->Read = TRUE;
}

/**
 * Lists a device's regions, once, or twice when the first answer did not fit.
 *
 * \return The regions, to free; NULL when the list failed.
 */
static PKPH_HW_VSEC_REGION PhpHwVsecListRegions(
    _In_ ULONG Locator,
    _Out_ PULONG Count
    )
{
    PKPH_HW_VSEC_REGION regions;
    ULONG capacity = PHP_HW_VSEC_LIST_REGIONS;
    NTSTATUS status;

    *Count = 0;
    regions = PhAllocate(sizeof(KPH_HW_VSEC_REGION) * capacity);
    status = KphHwVsecRegions(Locator, regions, capacity, Count);

    if (status == STATUS_BUFFER_TOO_SMALL && *Count > capacity)
    {
        PhFree(regions);
        capacity = *Count;
        regions = PhAllocate(sizeof(KPH_HW_VSEC_REGION) * capacity);
        status = KphHwVsecRegions(Locator, regions, capacity, Count);
    }

    if (!NT_SUCCESS(status))
    {
        PhTraceInfo("Hardware sensors: TPMI device 0x%lx unavailable: 0x%lx", Locator, status);
        PhFree(regions);
        *Count = 0;
        return NULL;
    }

    *Count = min(*Count, capacity);
    return regions;
}

/**
 * Maps a TPMI device to a package through the INFO feature (package ID [23:16]).
 *
 * \return The package index, or ULONG_MAX when it cannot be determined.
 */
static ULONG PhpHwVsecDevicePackage(
    _In_ ULONG Locator,
    _In_reads_(Count) const KPH_HW_VSEC_REGION* Regions,
    _In_ ULONG Count
    )
{
    static CONST ULONG offset = PH_INTEL_TPMI_INFO_BUS_INFO;
    ULONG package = ULONG_MAX;

    for (ULONG i = 0; i < Count; i++)
    {
        ULONG64 value;
        ULONG id;

        if (Regions[i].Discovery != KPH_HW_VSEC_DISCOVERY_TPMI || Regions[i].Id != PH_INTEL_TPMI_ID_INFO)
            continue;

        PhpHwVsecReadOnce(Locator, &Regions[i], &offset, 1);

        if (!PhpHwVsecValue(0, &value))
            return ULONG_MAX;

        id = (ULONG)((value >> 16) & 0xFF);

        if (package != ULONG_MAX && package != id)
            return ULONG_MAX;

        package = id;
    }

    return package < PhpHwPackageCount ? package : ULONG_MAX;
}

static PPHP_HW_VSEC_REGION PhpHwVsecBeginRegion(
    _In_ ULONG Device,
    _In_ const KPH_HW_VSEC_REGION* Region
    )
{
    PPHP_HW_VSEC_REGION region;

    if (PhpHwVsecRegionCount >= PHP_HW_VSEC_MAX_REGIONS)
        return NULL;

    region = &PhpHwVsecRegions[PhpHwVsecRegionCount];
    memset(region, 0, sizeof(PHP_HW_VSEC_REGION));
    region->Device = Device;
    region->Operation = Region->Operation;
    region->Discovery = Region->Discovery;
    region->Id = Region->Id;
    region->Position = Region->Position;

    return region;
}

static PPHP_HW_VSEC_ENTRY PhpHwVsecAddEntry(
    _Inout_ PPHP_HW_VSEC_REGION Region,
    _In_ ULONG Offset,
    _In_ PHP_HW_VSEC_KIND Kind,
    _In_ ULONG PackageIndex
    )
{
    PPHP_HW_VSEC_ENTRY entry;

    if (Region->EntryCount >= PHP_HW_VSEC_REGION_ENTRIES)
        return NULL;

    Region->Items[Region->EntryCount].Offset = Offset;
    Region->Items[Region->EntryCount].Width = sizeof(ULONG64);
    entry = &Region->Entries[Region->EntryCount++];
    memset(entry, 0, sizeof(PHP_HW_VSEC_ENTRY));
    entry->Offset = Offset;
    entry->Kind = Kind;
    entry->PackageIndex = PackageIndex;
    entry->NamedIndex = ULONG_MAX;
    entry->MaximumIndex = ULONG_MAX;

    return entry;
}

static ULONG PhpHwVsecEntryOrdinal(
    _In_ PPHP_HW_VSEC_REGION Region
    )
{
    ULONG ordinal = 0;

    for (ULONG i = 0; i <= PhpHwVsecRegionCount; i++)
    {
        PPHP_HW_VSEC_REGION region = i < PhpHwVsecRegionCount ? &PhpHwVsecRegions[i] : Region;

        // Uncore control entries do not count, so TPMI power ids do not depend on them.
        for (ULONG j = 0; j < region->EntryCount; j++)
        {
            if (region->Entries[j].Kind != PhpHwVsecUncoreControl)
                ordinal++;
        }
    }

    return ordinal;
}

/**
 * Adds the energy counters of a TPMI RAPL instance whose MSRs do not serve: 128-byte domains, each a header qword
 * (version [7:0], type [15:8], size [23:16] in 128 bytes, register flags [47:32]) followed by register n at +8n.
 */
static VOID PhpHwVsecProbeRapl(
    _In_ ULONG Device,
    _In_ const KPH_HW_VSEC_REGION* Region,
    _In_ ULONG PackageIndex
    )
{
    static CONST PCWSTR names[PhpHwVsecEnergyMaximum] = { L"Package", L"DRAM", L"Platform" };
    ULONG locator = PhpHwVsecLocators[Device];
    ULONG offsets[PHP_HW_VSEC_REGION_ENTRIES * 2];
    ULONG domains[PHP_HW_VSEC_REGION_ENTRIES];
    PHP_HW_VSEC_KIND kinds[PHP_HW_VSEC_REGION_ENTRIES];
    ULONG domainCount = min(Region->Size / PH_INTEL_TPMI_RAPL_DOMAIN_SIZE, PHP_HW_VSEC_REGION_ENTRIES);
    PPHP_HW_VSEC_REGION region;
    ULONG count = 0;

    if (domainCount == 0)
        return;

    for (ULONG i = 0; i < domainCount; i++)
        offsets[i] = i * PH_INTEL_TPMI_RAPL_DOMAIN_SIZE;

    PhpHwVsecReadOnce(locator, Region, offsets, domainCount);

    // Parsing stops at the first invalid or unknown domain.
    for (ULONG i = 0; i < domainCount; i++)
    {
        ULONG64 header;
        ULONG flags;
        PHP_HW_VSEC_KIND kind;

        if (!PhpHwVsecValue(i, &header) || (header & 0xFF) == 0xFF)
            break;
        if (((header & 0xFF) >> 5) != 0 || ((header >> 16) & 0xFF) != 1)
            break;

        flags = (ULONG)((header >> 32) & 0xFFFF);

        if (!(flags & (1UL << PH_INTEL_TPMI_RAPL_REG_UNIT)) || !(flags & (1UL << PH_INTEL_TPMI_RAPL_REG_ENERGY_STATUS)))
            break;

        switch ((header >> 8) & 0xFF)
        {
        case PH_INTEL_TPMI_RAPL_DOMAIN_SYSTEM:
            if (!(flags & (1UL << PH_INTEL_TPMI_RAPL_REG_DOMAIN_INFO)))
                continue;
            kind = PhpHwVsecPlatformEnergy;
            break;
        case PH_INTEL_TPMI_RAPL_DOMAIN_PACKAGE:
            kind = PhpHwVsecPackageEnergy;
            break;
        case PH_INTEL_TPMI_RAPL_DOMAIN_MEMORY:
            kind = PhpHwVsecDramEnergy;
            break;
        default:
            kind = PhpHwVsecEnergyMaximum;
            break;
        }

        if (kind == PhpHwVsecEnergyMaximum)
            break;

        // The MSRs serve the package totals where they exist (GNR/SRF); TPMI stands in without them (DMR).
        if (PhpHwCpuEnergyPlanned(kind))
            continue;

        domains[count] = i * PH_INTEL_TPMI_RAPL_DOMAIN_SIZE;
        kinds[count] = kind;
        count++;
    }

    if (count == 0)
        return;

    for (ULONG i = 0; i < count; i++)
    {
        offsets[i * 2] = domains[i] + PH_INTEL_TPMI_RAPL_REG_UNIT * 8;
        offsets[i * 2 + 1] = domains[i] + PH_INTEL_TPMI_RAPL_REG_DOMAIN_INFO * 8;
    }

    PhpHwVsecReadOnce(locator, Region, offsets, count * 2);

    if (!(region = PhpHwVsecBeginRegion(Device, Region)))
        return;

    for (ULONG i = 0; i < count; i++)
    {
        PPHP_HW_VSEC_ENTRY entry;
        ULONG64 unit;
        ULONG64 domainInfo;

        if (!PhpHwVsecValue(i * 2, &unit))
            continue;
        // Only the root domain enumerates the platform (PSYS) domain.
        if (kinds[i] == PhpHwVsecPlatformEnergy && (!PhpHwVsecValue(i * 2 + 1, &domainInfo) || !(domainInfo & 0x1)))
            continue;
        if (!(entry = PhpHwVsecAddEntry(region, domains[i] + PH_INTEL_TPMI_RAPL_REG_ENERGY_STATUS * 8, kinds[i], PackageIndex)))
            break;

        entry->EnergyUnit = 1.0f / (FLOAT)(1ULL << ((unit >> PH_INTEL_TPMI_RAPL_ENERGY_UNIT_SHIFT) & 0x1F));

        if (PackageIndex == ULONG_MAX)
        {
            PPH_STRING name = PhFormatString(L"%s (TPMI %lu)", names[kinds[i]], Device);

            entry->NamedIndex = PhpHwAddNamedSensor(PhHwNamedSensorTpmiPower, 0, PhpHwVsecEntryOrdinal(region) - 1, name->Buffer);
            PhDereferenceObject(name);

            if (entry->NamedIndex == ULONG_MAX)
            {
                region->EntryCount--;
                break;
            }
        }
    }

    if (region->EntryCount)
        PhpHwVsecRegionCount++;
}

/**
 * Adds the cluster status and control registers of a TPMI uncore instance: header qword (version [7:0], cluster mask
 * [15:8]), cluster offsets at +08h (a byte per cluster, in qwords); status ratio [6:0], control (+08h) maximum
 * [14:8] and minimum [21:15], in 100 MHz.
 */
static VOID PhpHwVsecProbeUncore(
    _In_ ULONG Device,
    _In_ const KPH_HW_VSEC_REGION* Region,
    _In_ ULONG PackageIndex,
    _Inout_ PULONG DomainIndex
    )
{
    static CONST ULONG offsets[2] = { 0, PH_INTEL_TPMI_UNCORE_CLUSTER_OFFSET };
    PPHP_HW_VSEC_REGION region;
    ULONG64 header;
    ULONG64 clusterOffsets;
    PCWSTR prefix = L"Uncore ";
    ULONG namedPackage = PackageIndex;
    ULONG unmappedId = 0;
    ULONG clusterCount = 0;
    ULONG domain;

    PhpHwVsecReadOnce(PhpHwVsecLocators[Device], Region, offsets, 2);

    // A read-blocked feature reads all ones.
    if (!PhpHwVsecValue(0, &header) || !PhpHwVsecValue(1, &clusterOffsets))
        return;
    if (((header & 0xFF) >> 5) != 0)
        return;

    for (ULONG i = 0; i < 8; i++)
    {
        if (header & (1ULL << (8 + i)))
            clusterCount++;
    }

    if (clusterCount == 0 || !(region = PhpHwVsecBeginRegion(Device, Region)))
        return;

    domain = (*DomainIndex)++;

    if (PackageIndex == ULONG_MAX)
    {
        prefix = L"Uncore (unmapped) ";
        namedPackage = 0;
        unmappedId = PHP_HW_VSEC_UNMAPPED_ID;
    }

    for (ULONG i = 0; i < clusterCount; i++)
    {
        ULONG offset = (ULONG)((clusterOffsets >> (i * 8)) & 0xFF) * 8;
        PPHP_HW_VSEC_ENTRY entry;
        PPH_STRING name;

        if (offset == 0 || offset + 8 > Region->Size)
            continue;
        if (!(entry = PhpHwVsecAddEntry(region, offset, PhpHwVsecUncoreStatus, PackageIndex)))
            break;

        if (clusterCount > 1)
            name = PhFormatString(L"%s%lu.%lu", prefix, domain, i);
        else
            name = PhFormatString(L"%s%lu", prefix, domain);

        // Unmapped domains share package 0 with its mapped ones; their ids are kept apart.
        entry->NamedIndex = PhpHwAddNamedSensor(
            PhHwNamedSensorUncoreClock,
            namedPackage,
            unmappedId | (domain << 3) | i,
            name->Buffer
            );
        PhDereferenceObject(name);

        if (entry->NamedIndex == ULONG_MAX)
        {
            region->EntryCount--;
            break;
        }

        if (offset + 16 > Region->Size || !(entry = PhpHwVsecAddEntry(region, offset + 8, PhpHwVsecUncoreControl, PackageIndex)))
            continue;

        for (ULONG limit = 0; limit < 2; limit++)
        {
            ULONG id = (PackageIndex == ULONG_MAX ? PHP_HW_VSEC_UNMAPPED_ID : 0) | (limit ? PHP_HW_VSEC_UNCORE_MAX_ID : PHP_HW_VSEC_UNCORE_MIN_ID) | (domain << 3) | i;
            PCWSTR bound = L"min";

            if (limit)
                bound = L"max";

            if (clusterCount > 1)
                name = PhFormatString(L"%s%lu.%lu %s", prefix, domain, i, bound);
            else
                name = PhFormatString(L"%s%lu %s", prefix, domain, bound);

            *(limit ? &entry->MaximumIndex : &entry->NamedIndex) = PhpHwAddNamedSensor(PhHwNamedSensorUncoreClock, namedPackage, id, name->Buffer);
            PhDereferenceObject(name);
        }

        if (entry->NamedIndex == ULONG_MAX || entry->MaximumIndex == ULONG_MAX)
            region->EntryCount--;
    }

    if (region->EntryCount)
        PhpHwVsecRegionCount++;
}

/**
 * Applies the transform of a PMT sample to its qword.
 *
 * \return FALSE when the field is not a reading: a temperature outside 1-125 C, or a zero ratio.
 */
_Success_(return)
static BOOLEAN PhpHwVsecDecodePmtSample(
    _In_ PCPHP_HW_PMT_SAMPLE Sample,
    _In_ ULONG64 Qword,
    _Out_ PFLOAT Value
    )
{
    UCHAR field = (UCHAR)(Qword >> Sample->Lsb);

    switch (Sample->Transform)
    {
    case PhpHwPmtTemperature:
        if ((SCHAR)field <= 0 || (SCHAR)field > 125)
            return FALSE;
        *Value = (FLOAT)(SCHAR)field;
        return TRUE;
    case PhpHwPmtRatio100:
        if (!field)
            return FALSE;
        *Value = (FLOAT)field * 100.0f;
        return TRUE;
    case PhpHwPmtRatio33:
        if (!field)
            return FALSE;
        *Value = (FLOAT)field * 100.0f / 3.0f;
        return TRUE;
    }

    return FALSE;
}

/**
 * Lists the samples of a PMT group the table knows by its GUID; a sample that does not read, or reads all ones, at
 * the probe is not listed.
 */
static VOID PhpHwVsecProbePmt(
    _In_ ULONG Device,
    _In_ const KPH_HW_VSEC_REGION* Region
    )
{
    ULONG offsets[PHP_HW_VSEC_REGION_ENTRIES];
    ULONG samples[PHP_HW_VSEC_REGION_ENTRIES];
    PPHP_HW_VSEC_REGION region;
    ULONG count = 0;

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwPmtSamples) && count < RTL_NUMBER_OF(samples); i++)
    {
        if (PhpHwPmtSamples[i].Guid != Region->Id || (ULONG)PhpHwPmtSamples[i].Qword * 8 + 8 > Region->Size)
            continue;

        offsets[count] = PhpHwPmtSamples[i].Qword * 8;
        samples[count++] = i;
    }

    if (count == 0)
        return;

    PhpHwVsecReadOnce(PhpHwVsecLocators[Device], Region, offsets, count);

    if (!(region = PhpHwVsecBeginRegion(Device, Region)))
        return;

    for (ULONG i = 0; i < count; i++)
    {
        PCPHP_HW_PMT_SAMPLE sample = &PhpHwPmtSamples[samples[i]];
        PPHP_HW_VSEC_ENTRY entry;
        ULONG64 value;
        PH_HW_NAMED_SENSOR_KIND kind = PhHwNamedSensorUncoreClock;

        // A field that reads 0 at the probe may be idle; it is listed and shown once it reads.
        if (!PhpHwVsecValue(i, &value))
            continue;
        if (!(entry = PhpHwVsecAddEntry(region, offsets[i], PhpHwVsecPmtSample, 0)))
            break;

        entry->Sample = samples[i];

        if (sample->Transform == PhpHwPmtTemperature)
            kind = PhHwNamedSensorPmtTemperature;

        entry->NamedIndex = PhpHwAddNamedSensor(
            kind,
            0,
            PHP_HW_VSEC_PMT_ID_BASE | ((Device - PhpHwVsecOobmsmCount) << 16) | ((ULONG)(Region->Index & 0xFF) << 8) | samples[i],
            sample->Name
            );

        if (entry->NamedIndex == ULONG_MAX)
        {
            region->EntryCount--;
            break;
        }
    }

    if (region->EntryCount)
        PhpHwVsecRegionCount++;
}

/**
 * Finds the OOBMSM and PMT functions, lists each one's regions and reads the TPMI headers and the known PMT samples.
 * Called once a session.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED without a region to read, or the first identify's status.
 */
_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwVsecProbe(
    VOID
    )
{
    static CONST USHORT oobmsmIds[] = { 0x09A7, 0x09A1 }; // SPR/EMR/GNR/SRF, DMR
    static CONST USHORT pmtIds[] = { 0x7D0D, 0xAD0D, 0x647D, 0xB07D, 0xFD7D, 0xD70D }; // MTL-M, MTL-S/ARL-S, LNL, PTL, WCL, NVL
    NTSTATUS firstStatus = STATUS_NOT_SUPPORTED;
    PULONG domainIndex;
    ULONG unmappedDomainIndex = 0;

    PhpHwVsecOobmsmCount = PhpHwEnumeratePciFunctions(
        0x8086,
        0x8086,
        PHP_HW_ANY_CLASS_CODE,
        oobmsmIds,
        RTL_NUMBER_OF(oobmsmIds),
        FALSE,
        PhpHwVsecLocators,
        RTL_NUMBER_OF(PhpHwVsecLocators)
        );
    PhpHwVsecDeviceCount = PhpHwVsecOobmsmCount + PhpHwEnumeratePciFunctions(
        0x8086,
        0x8086,
        PHP_HW_ANY_CLASS_CODE,
        pmtIds,
        RTL_NUMBER_OF(pmtIds),
        FALSE,
        &PhpHwVsecLocators[PhpHwVsecOobmsmCount],
        RTL_NUMBER_OF(PhpHwVsecLocators) - PhpHwVsecOobmsmCount
        );

    if (!PhpHwVsecDeviceCount)
        return STATUS_NOT_SUPPORTED;

    PhpHwVsecRegions = PhAllocate(sizeof(PHP_HW_VSEC_REGION) * PHP_HW_VSEC_MAX_REGIONS);
    PhpHwVsecPackages = PhAllocateZero(sizeof(PHP_HW_VSEC_PACKAGE) * max(PhpHwPackageCount, 1));
    domainIndex = PhAllocateZero(sizeof(ULONG) * max(PhpHwPackageCount, 1));

    for (ULONG i = 0; i < PhpHwVsecDeviceCount; i++)
    {
        ULONG locator = PhpHwVsecLocators[i];
        KPH_HW_VSEC device;
        PKPH_HW_VSEC_REGION regions;
        ULONG regionCount;
        ULONG packageIndex;
        NTSTATUS status;

        status = KphHwVsecIdentify(locator, &device);

        if (i == 0)
            firstStatus = status;
        if (!NT_SUCCESS(status))
            continue;
        if (!(regions = PhpHwVsecListRegions(locator, &regionCount)))
            continue;

        // A PMT function whose groups match no sample of the table ends here.
        if (i >= PhpHwVsecOobmsmCount)
        {
            for (ULONG j = 0; j < regionCount; j++)
            {
                if (regions[j].Discovery == KPH_HW_VSEC_DISCOVERY_PMT)
                    PhpHwVsecProbePmt(i, &regions[j]);
            }

            PhFree(regions);
            continue;
        }

        packageIndex = PhpHwVsecDevicePackage(locator, regions, regionCount);

        if (packageIndex == ULONG_MAX && PhpHwPackageCount == 1)
            packageIndex = 0;

        for (ULONG j = 0; j < regionCount; j++)
        {
            if (regions[j].Discovery != KPH_HW_VSEC_DISCOVERY_TPMI)
                continue;

            switch (regions[j].Id)
            {
            case PH_INTEL_TPMI_ID_RAPL:
                PhpHwVsecProbeRapl(i, &regions[j], packageIndex);
                break;
            case PH_INTEL_TPMI_ID_UNCORE:
                if (packageIndex == ULONG_MAX)
                    PhpHwVsecProbeUncore(i, &regions[j], packageIndex, &unmappedDomainIndex);
                else
                    PhpHwVsecProbeUncore(i, &regions[j], packageIndex, &domainIndex[packageIndex]);
                break;
            case PH_INTEL_TPMI_ID_SST:
            case PH_INTEL_TPMI_ID_PLR:
                if (PhpHwVsecReportCount < RTL_NUMBER_OF(PhpHwVsecReports))
                {
                    PPHP_HW_VSEC_REPORT report = &PhpHwVsecReports[PhpHwVsecReportCount++];

                    memset(report, 0, sizeof(PHP_HW_VSEC_REPORT));
                    report->Device = i;
                    report->Region = regions[j];
                    report->PackageIndex = packageIndex;
                }
                else
                {
                    PhpHwVsecReportsOmitted++;
                }
                break;
            }
        }

        PhFree(regions);
    }

    PhFree(domainIndex);

    if (!PhpHwVsecRegionCount && !PhpHwVsecReportCount)
        return NT_SUCCESS(firstStatus) ? STATUS_NOT_SUPPORTED : firstStatus;

    return STATUS_SUCCESS;
}

/**
 * Lists a device's regions again after one missed: a region found at its position with its id goes on; a second miss
 * ends it.
 */
static VOID PhpHwVsecRelistDevice(
    _In_ ULONG Device
    )
{
    PKPH_HW_VSEC_REGION regions;
    ULONG regionCount;

    PhpHwVsecRelist[Device] = FALSE;
    regions = PhpHwVsecListRegions(PhpHwVsecLocators[Device], &regionCount);

    for (ULONG r = 0; r < PhpHwVsecRegionCount; r++)
    {
        PPHP_HW_VSEC_REGION region = &PhpHwVsecRegions[r];
        BOOLEAN found = FALSE;

        if (region->Device != Device || !region->Missed)
            continue;

        for (ULONG j = 0; regions && j < regionCount; j++)
        {
            if (regions[j].Discovery == region->Discovery && regions[j].Id == region->Id && regions[j].Operation == region->Operation)
            {
                region->Position = regions[j].Position;
                found = TRUE;
                break;
            }
        }

        if (!found)
            region->Disabled = TRUE;
    }

    if (regions)
        PhFree(regions);
}

/**
 * Reads each kept region in one call: the uncore ratios; the RAPL energy counters' watts, into the package sensor
 * only when every domain of that kind on the package delivered.
 *
 * \return STATUS_RETRY when every read met contention, else STATUS_SUCCESS.
 */
_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwVsecPass(
    VOID
    )
{
    static CONST ULONG kindCapabilities[PhpHwVsecEnergyMaximum] = { PH_HW_CAP_RAPL_PACKAGE, PH_HW_CAP_RAPL_DRAM, PH_HW_CAP_RAPL_PSYS };
    ULONG capabilities = 0;
    BOOLEAN answered = FALSE;
    BOOLEAN retried = FALSE;
    FLOAT seconds = 0.0f;

    memset(PhpHwVsecPackages, 0, sizeof(PHP_HW_VSEC_PACKAGE) * max(PhpHwPackageCount, 1));

    for (ULONG i = 0; i < PhpHwVsecDeviceCount; i++)
    {
        if (PhpHwVsecRelist[i])
            PhpHwVsecRelistDevice(i);
    }

    for (ULONG i = 0; PhpHwReportReads && i < PhpHwVsecReportCount; i++)
    {
        PhpHwVsecReports[i].Read = FALSE;

        if (PhpHwVsecReports[i].Region.Id == PH_INTEL_TPMI_ID_SST)
            PhpHwVsecReadSst(&PhpHwVsecReports[i]);
        else
            PhpHwVsecReadPlr(&PhpHwVsecReports[i]);
    }

    PhpHwVsecLastSampleTime = PhpHwVsecSampleTime;
    PhpHwVsecSampleTime = PhpHwQueryTime();

    if (PhpHwVsecLastSampleTime && PhpHwVsecSampleTime > PhpHwVsecLastSampleTime)
        seconds = (FLOAT)(PhpHwVsecSampleTime - PhpHwVsecLastSampleTime) / (FLOAT)PHP_HW_SECOND;

    for (ULONG r = 0; r < PhpHwVsecRegionCount; r++)
    {
        PPHP_HW_VSEC_REGION region = &PhpHwVsecRegions[r];
        KPH_HW_WINDOW_ITEM items[PHP_HW_VSEC_REGION_ENTRIES];
        ULONG indices[PHP_HW_VSEC_REGION_ENTRIES];
        ULONG count = 0;
        NTSTATUS status;

        for (ULONG i = 0; i < region->EntryCount; i++)
        {
            PPHP_HW_VSEC_ENTRY entry = &region->Entries[i];

            entry->LastValid = entry->Valid;
            entry->LastValue = entry->Value;
            entry->Valid = FALSE;

            if (region->Disabled || entry->Disabled)
            {
                PhpHwSetNamedSensor(entry->NamedIndex, FALSE, 0.0f);
                PhpHwSetNamedSensor(entry->MaximumIndex, FALSE, 0.0f);
                continue;
            }

            // A package value needs every domain still read.
            if (entry->Kind < PhpHwVsecEnergyMaximum && entry->NamedIndex == ULONG_MAX && entry->PackageIndex < PhpHwPackageCount)
                PhpHwVsecPackages[entry->PackageIndex].Required[entry->Kind]++;

            items[count] = region->Items[i];
            indices[count++] = i;
        }

        if (!count)
        {
            region->Disabled = TRUE;
            continue;
        }

        status = KphHwVsecRead(PhpHwVsecLocators[region->Device], region->Operation, region->Position, region->Id, items, count, PhpHwVsecValues);

        if (status == STATUS_CONTEXT_MISMATCH)
        {
            // The region moved: its device is listed again at the next update, once.
            if (region->Missed)
            {
                region->Disabled = TRUE;
                PhTraceInfo("Hardware sensors: TPMI region 0x%lx of device %lu gone", region->Position, region->Device);
            }
            else
            {
                region->Missed = TRUE;
                PhpHwVsecRelist[region->Device] = TRUE;
            }

            answered = TRUE;
        }
        else if (!NT_SUCCESS(status))
        {
            if (status == STATUS_RETRY)
                retried = TRUE;
            else
                answered = TRUE;

            PhpHwVsecApplyStatus(&region->Disabled, &region->Strikes, status);
        }
        else
        {
            answered = TRUE;
            region->Strikes = 0;
            region->Missed = FALSE;

            for (ULONG i = 0; i < count; i++)
            {
                PPHP_HW_VSEC_ENTRY entry = &region->Entries[indices[i]];

                if (PhpHwVsecApplyStatus(&entry->Disabled, &entry->Strikes, PhpHwVsecValues[i].Status) && PhpHwVsecValues[i].Value != ULONG64_MAX)
                {
                    entry->Value = PhpHwVsecValues[i].Value;
                    entry->Valid = TRUE;
                }
            }
        }

        for (ULONG i = 0; i < count; i++)
        {
            PPHP_HW_VSEC_ENTRY entry = &region->Entries[indices[i]];
            BOOLEAN valid = FALSE;
            FLOAT value = 0.0f;

            if (entry->Kind == PhpHwVsecUncoreStatus)
            {
                if (entry->Valid)
                {
                    value = (FLOAT)(entry->Value & 0x7F) * 100.0f;
                    valid = TRUE;
                }
            }
            else if (entry->Kind == PhpHwVsecUncoreControl)
            {
                FLOAT maximum = (FLOAT)((entry->Value >> 8) & 0x7F) * 100.0f;

                value = (FLOAT)((entry->Value >> 15) & 0x7F) * 100.0f;
                valid = entry->Valid && value > 0.0f;
                PhpHwSetNamedSensor(entry->MaximumIndex, entry->Valid && maximum > 0.0f, maximum);
            }
            else if (entry->Kind == PhpHwVsecPmtSample)
            {
                valid = entry->Valid && PhpHwVsecDecodePmtSample(&PhpHwPmtSamples[entry->Sample], entry->Value, &value);
            }
            else if (entry->Valid && entry->LastValid && seconds > 0.0f)
            {
                // ENERGY_STATUS [31:0] wraps.
                value = (FLOAT)((ULONG)entry->Value - (ULONG)entry->LastValue) * entry->EnergyUnit / seconds;

                if (value > PHP_HW_VSEC_ENERGY_MAX_WATTS)
                {
                    NOTHING;
                }
                else if (entry->NamedIndex != ULONG_MAX)
                {
                    valid = TRUE;
                }
                else if (entry->PackageIndex < PhpHwPackageCount)
                {
                    PhpHwVsecPackages[entry->PackageIndex].Power[entry->Kind] += value;
                    PhpHwVsecPackages[entry->PackageIndex].Delivered[entry->Kind]++;
                }
            }

            PhpHwSetNamedSensor(entry->NamedIndex, valid, value);
        }
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_VSEC_PACKAGE package = &PhpHwVsecPackages[i];
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];
        ULONG owned = 0;
        ULONG valid = 0;

        for (ULONG kind = 0; kind < PhpHwVsecEnergyMaximum; kind++)
        {
            if (!package->Required[kind])
                continue;

            owned |= kindCapabilities[kind];

            if (package->Delivered[kind] != package->Required[kind])
                continue;

            switch (kind)
            {
            case PhpHwVsecPackageEnergy:
                sensor->PackagePower = package->Power[kind];
                break;
            case PhpHwVsecDramEnergy:
                sensor->DramPower = package->Power[kind];
                break;
            case PhpHwVsecPlatformEnergy:
                sensor->PlatformPower = package->Power[kind];
                break;
            }

            valid |= kindCapabilities[kind];
        }

        capabilities |= owned;
        WriteULongRelease(&sensor->Valid, (ReadULongAcquire(&sensor->Valid) & ~owned) | (valid & owned));
    }

    for (ULONG r = 0; r < PhpHwVsecRegionCount; r++)
    {
        PPHP_HW_VSEC_REGION region = &PhpHwVsecRegions[r];

        for (ULONG i = 0; !region->Disabled && i < region->EntryCount; i++)
        {
            if (region->Entries[i].Disabled)
                continue;

            if (region->Entries[i].Kind == PhpHwVsecUncoreStatus || region->Entries[i].Kind == PhpHwVsecUncoreControl)
                capabilities |= PH_HW_CAP_UNCORE_CLOCK;
            else if (region->Entries[i].Kind == PhpHwVsecPmtSample)
                capabilities |= PhpHwPmtSamples[region->Entries[i].Sample].Transform == PhpHwPmtTemperature ? PH_HW_CAP_TPMI_POWER : PH_HW_CAP_UNCORE_CLOCK;
            else if (region->Entries[i].Kind < PhpHwVsecEnergyMaximum && region->Entries[i].NamedIndex != ULONG_MAX)
                capabilities |= PH_HW_CAP_TPMI_POWER;
        }
    }

    PhpHwFamilies[PhpHwFamilyVsec].Capabilities = capabilities;

    return retried && !answered ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Ends the session: the functions are found and listed again at the next probe.
 */
_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwVsecEnd(
    VOID
    )
{
    if (PhpHwVsecRegions)
    {
        PhFree(PhpHwVsecRegions);
        PhpHwVsecRegions = NULL;
    }

    if (PhpHwVsecPackages)
    {
        PhFree(PhpHwVsecPackages);
        PhpHwVsecPackages = NULL;
    }

    PhpHwVsecRegionCount = 0;
    PhpHwVsecReportCount = 0;
    PhpHwVsecReportsOmitted = 0;
    PhpHwVsecDeviceCount = 0;
    PhpHwVsecOobmsmCount = 0;
    PhpHwVsecSampleTime = 0;
    PhpHwVsecLastSampleTime = 0;
    memset(PhpHwVsecRelist, 0, sizeof(PhpHwVsecRelist));
}

/**
 * \return The package bits the collector owns for the package (its last pass's energy kinds), 0 without one.
 */
ULONG PhpHwVsecPackageCaps(
    _In_ ULONG PackageIndex
    )
{
    ULONG owned = 0;

    if (!PhpHwVsecPackages || PackageIndex >= PhpHwPackageCount)
        return 0;

    if (PhpHwVsecPackages[PackageIndex].Required[PhpHwVsecPackageEnergy])
        owned |= PH_HW_CAP_RAPL_PACKAGE;
    if (PhpHwVsecPackages[PackageIndex].Required[PhpHwVsecDramEnergy])
        owned |= PH_HW_CAP_RAPL_DRAM;
    if (PhpHwVsecPackages[PackageIndex].Required[PhpHwVsecPlatformEnergy])
        owned |= PH_HW_CAP_RAPL_PSYS;

    return owned;
}

/**
 * Writes the collector's section of the hardware report: the functions and the regions read.
 *
 * \param Builder The report.
 */
_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwVsecDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PhAppendFormatStringBuilder(Builder, L"  Devices: %lu\n", PhpHwVsecDeviceCount);

    for (ULONG r = 0; r < PhpHwVsecRegionCount; r++)
    {
        PPHP_HW_VSEC_REGION region = &PhpHwVsecRegions[r];
        PCWSTR ended = L"";

        if (region->Disabled)
            ended = L", ended";

        if (region->Discovery == KPH_HW_VSEC_DISCOVERY_PMT)
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"  Device %lu region 0x%lx (PMT 0x%08lx): %lu entries%s\n",
                region->Device,
                region->Position,
                region->Id,
                region->EntryCount,
                ended
                );
        }
        else
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"  Device %lu region 0x%lx (TPMI 0x%02lx): %lu entries%s\n",
                region->Device,
                region->Position,
                region->Id,
                region->EntryCount,
                ended
                );
        }
    }

    for (ULONG i = 0; i < PhpHwVsecReportCount; i++)
    {
        PPHP_HW_VSEC_REPORT report = &PhpHwVsecReports[i];

        if (report->Region.Id == PH_INTEL_TPMI_ID_SST)
            PhAppendStringBuilder2(Builder, L"  TPMI SST ");
        else
            PhAppendStringBuilder2(Builder, L"  TPMI PLR ");

        if (report->PackageIndex == ULONG_MAX)
            PhAppendStringBuilder2(Builder, L"(unmapped)");
        else
            PhAppendFormatStringBuilder(Builder, L"package %lu", report->PackageIndex);

        PhAppendFormatStringBuilder(Builder, L" instance %u: ", report->Region.Index);

        if (!report->Read)
        {
            PhAppendStringBuilder2(Builder, L"no reading\n");
        }
        else if (report->Region.Id == PH_INTEL_TPMI_ID_SST)
        {
            PCWSTR locked = L"";

            if (report->Locked)
                locked = L" (locked)";

            PhAppendFormatStringBuilder(
                Builder,
                L"version %u.%u, level %u%s, TDP %.3f W, TjMax %lu C, P0 %lu MHz, P1 %lu MHz, Pn %lu MHz, Pm %lu MHz\n",
                (report->Version >> 5) & 0x7,
                report->Version & 0x1F,
                report->Level,
                locked,
                (DOUBLE)((report->Value >> 32) & 0x7FFF) / 8.0,
                (ULONG)((report->Value >> 47) & 0xFF),
                (ULONG)(report->Ratios & 0xFF) * 100,
                (ULONG)((report->Ratios >> 8) & 0xFF) * 100,
                (ULONG)((report->Ratios >> 16) & 0xFF) * 100,
                (ULONG)((report->Ratios >> 24) & 0xFF) * 100
                );
        }
        else
        {
            static CONST PCWSTR reasons[] = { L"frequency", L"current", L"power", L"thermal", L"platform", L"MCP", L"RAS", L"misc", L"QoS", L"DFC" };

            PhAppendFormatStringBuilder(Builder, L"reasons since cleared 0x%016I64x", report->Value);

            for (ULONG j = 0; j < RTL_NUMBER_OF(reasons); j++)
            {
                if (report->Value & (1ULL << j))
                    PhAppendFormatStringBuilder(Builder, L", %s", reasons[j]);
            }

            PhAppendCharStringBuilder(Builder, L'\n');
        }
    }

    if (PhpHwVsecReportsOmitted)
        PhAppendFormatStringBuilder(Builder, L"  TPMI: %lu SST/PLR instances omitted\n", PhpHwVsecReportsOmitted);
}
