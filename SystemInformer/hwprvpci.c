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

// Collector pciconfig: the device 18h functions of AMD families 10h to 16h, read through pciconfig.lua, and for a
// report the configuration space of the AMD bridges the script lists.

#define PHP_HW_PCI_AMD_VENDOR_ID 0x1022
#define PHP_HW_PCI_HOST_BRIDGE_CLASS 0x0600
#define PHP_HW_PCI_MAX_NODES 8
#define PHP_HW_PCI_MAX_IDENTIFIED (PHP_HW_PCI_MAX_NODES * 3)
#define PHP_HW_PCI_PACKAGE_NODES 2 // D18F3 functions of a package
#define PHP_HW_PCI_STRIKE_LIMIT 3 // failures of a register before it ends for the session
#define PHP_HW_PCI_MAX_BRIDGES 16
#define PHP_HW_PCI_CONFIG_LENGTH 0x1000
#define PHP_HW_PCI_CONFIG_HEADER 0x100
#define PHP_HW_PCI_STATUS_CAPABILITIES 0x0010
#define PHP_HW_PCI_CAP_EXPRESS 0x10
#define PHP_HW_PCI_EXT_CAP_AER 0x0001
#define PHP_HW_PCI_EXT_CAP_DPC 0x001D

// A bridge's configuration space as the last report read it: the bytes and which of them were read.
typedef struct _PHP_HW_PCI_BRIDGE_CONFIG
{
    NTSTATUS Status; // the header range's
    UCHAR Bytes[PHP_HW_PCI_CONFIG_LENGTH];
    BOOLEAN Readable[PHP_HW_PCI_CONFIG_LENGTH];
} PHP_HW_PCI_BRIDGE_CONFIG, *PPHP_HW_PCI_BRIDGE_CONFIG;

// A register read every update.
typedef struct _PHP_HW_PCI_ITEM
{
    BOOLEAN Enabled;
    UCHAR Strikes;
} PHP_HW_PCI_ITEM, *PPHP_HW_PCI_ITEM;

typedef struct _PHP_HW_PCI_PACKAGE
{
    ULONG NodeCount;
    ULONG Nodes[PHP_HW_PCI_PACKAGE_NODES]; // D18F3, ReportedTempCtrl
    PHP_HW_PCI_ITEM Temperature[PHP_HW_PCI_PACKAGE_NODES];
    BOOLEAN HasPowerNode;
    ULONG PowerNode; // D18F4; function 5 is the next locator
    ULONG ProcessorTdp; // D18F4x1B8
    ULONG TdpLimit3; // D18F5xE8
    PHP_HW_PCI_ITEM RunningAverage; // D18F5xE0
} PHP_HW_PCI_PACKAGE, *PPHP_HW_PCI_PACKAGE;

// A function identified this session: the script's row answer, kept so a function is identified once.
typedef struct _PHP_HW_PCI_IDENTIFIED
{
    ULONG Locator;
    NTSTATUS Status;
    KPH_HW_PCI_CONFIG Device;
} PHP_HW_PCI_IDENTIFIED, *PPHP_HW_PCI_IDENTIFIED;

static BOOLEAN PhpHwPciNodesFound = FALSE;
static ULONG PhpHwPciMiscNodes[PHP_HW_PCI_MAX_NODES]; // D18F3, in bus order
static ULONG PhpHwPciMiscNodeCount = 0;
static ULONG PhpHwPciLinkNodes[PHP_HW_PCI_MAX_NODES]; // D18F4, in bus order
static ULONG PhpHwPciLinkNodeCount = 0;
static PHP_HW_PCI_IDENTIFIED PhpHwPciIdentified[PHP_HW_PCI_MAX_IDENTIFIED];
static ULONG PhpHwPciIdentifiedCount = 0;
static PH_CPU_IDENTITY PhpHwPciIdentity;
static PPHP_HW_PCI_PACKAGE PhpHwPciPackages = NULL; // per package
static BOOLEAN PhpHwPciProbed = FALSE;
static ULONG PhpHwPciBridges[PHP_HW_PCI_MAX_BRIDGES];
static ULONG PhpHwPciBridgeCount = 0;
static PPHP_HW_PCI_BRIDGE_CONFIG PhpHwPciBridgeConfigs = NULL; // per bridge, once a report read them

/**
 * Finds the D18F3 and D18F4 functions, once a session.
 */
static VOID PhpHwPciFindNodes(
    VOID
    )
{
    static CONST USHORT miscControlIds[] = { 0x1203, 0x1303, 0x1703, 0x1603, 0x1403, 0x141D, 0x1533, 0x1583 }; // D18F3
    static CONST USHORT linkControlIds[] = { 0x1204, 0x1704, 0x1404, 0x1604, 0x141E, 0x1574, 0x15B4, 0x1534, 0x1584 }; // D18F4

    if (PhpHwPciNodesFound)
        return;

    PhpHwPciNodesFound = TRUE;
    PhpHwPciMiscNodeCount = PhpHwEnumeratePciFunctions(
        PHP_HW_PCI_AMD_VENDOR_ID,
        PHP_HW_PCI_AMD_VENDOR_ID,
        PHP_HW_PCI_HOST_BRIDGE_CLASS,
        miscControlIds,
        RTL_NUMBER_OF(miscControlIds),
        FALSE,
        PhpHwPciMiscNodes,
        RTL_NUMBER_OF(PhpHwPciMiscNodes)
        );
    PhpHwPciLinkNodeCount = PhpHwEnumeratePciFunctions(
        PHP_HW_PCI_AMD_VENDOR_ID,
        PHP_HW_PCI_AMD_VENDOR_ID,
        PHP_HW_PCI_HOST_BRIDGE_CLASS,
        linkControlIds,
        RTL_NUMBER_OF(linkControlIds),
        FALSE,
        PhpHwPciLinkNodes,
        RTL_NUMBER_OF(PhpHwPciLinkNodes)
        );

    PhTraceInfo("Hardware sensors: %lu D18F3 and %lu D18F4 functions", PhpHwPciMiscNodeCount, PhpHwPciLinkNodeCount);
}

/**
 * Identifies a function once a session.
 *
 * \return The status of its identify.
 */
static NTSTATUS PhpHwPciIdentify(
    _In_ ULONG Locator
    )
{
    PPHP_HW_PCI_IDENTIFIED identified;

    for (ULONG i = 0; i < PhpHwPciIdentifiedCount; i++)
    {
        if (PhpHwPciIdentified[i].Locator == Locator)
            return PhpHwPciIdentified[i].Status;
    }

    if (PhpHwPciIdentifiedCount >= RTL_NUMBER_OF(PhpHwPciIdentified))
        return STATUS_INSUFFICIENT_RESOURCES;

    identified = &PhpHwPciIdentified[PhpHwPciIdentifiedCount++];
    identified->Locator = Locator;
    identified->Status = KphHwPciConfigIdentify(Locator, &identified->Device);

    if (!NT_SUCCESS(identified->Status))
        PhTraceInfo("Hardware sensors: PCI function 0x%lx not identified: 0x%lx", Locator, identified->Status);

    return identified->Status;
}

/**
 * Reads registers of a function, identified first; a call that fails gives its status to every item.
 */
static VOID PhpHwPciRead(
    _In_ ULONG Locator,
    _In_reads_(Count) const ULONG* Offsets,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    KPH_HW_WINDOW_ITEM items[2];
    NTSTATUS status;

    status = PhpHwPciIdentify(Locator);

    if (NT_SUCCESS(status))
    {
        for (ULONG i = 0; i < Count; i++)
        {
            items[i].Offset = Offsets[i];
            items[i].Width = sizeof(ULONG);
        }

        status = KphHwPciConfigRead(Locator, items, Count, Values);
    }

    for (ULONG i = 0; !NT_SUCCESS(status) && i < Count; i++)
    {
        Values[i].Status = status;
        Values[i].Value = 0;
    }
}

/**
 * Applies the session failure policy to a register: a refused one ends at once; contention is no value, not a
 * failure; anything else strikes.
 *
 * \return TRUE when the value is usable.
 */
static BOOLEAN PhpHwPciApplyStatus(
    _Inout_ PPHP_HW_PCI_ITEM Item,
    _In_ NTSTATUS Status
    )
{
    if (NT_SUCCESS(Status))
    {
        Item->Strikes = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_RETRY:
    case STATUS_CANCELLED:
        break;
    case STATUS_ACCESS_DENIED:
    case STATUS_NOT_SUPPORTED:
    case STATUS_NOT_FOUND:
        Item->Enabled = FALSE;
        break;
    default:
        if (++Item->Strikes >= PHP_HW_PCI_STRIKE_LIMIT)
            Item->Enabled = FALSE;
        break;
    }

    return FALSE;
}

/**
 * Reads one register of an identified function.
 */
_Success_(return)
static BOOLEAN PhpHwPciReadOnce(
    _In_ ULONG Locator,
    _In_ ULONG Offset,
    _Out_ PULONG Value
    )
{
    KPH_HW_WINDOW_ITEM item;
    KPH_HW_VALUE32 value;

    if (!NT_SUCCESS(PhpHwPciIdentify(Locator)))
        return FALSE;

    item.Offset = Offset;
    item.Width = sizeof(ULONG);

    if (!NT_SUCCESS(KphHwPciConfigRead(Locator, &item, 1, &value)) || !NT_SUCCESS(value.Status))
        return FALSE;

    *Value = value.Value;
    return TRUE;
}

/**
 * Finds the PStateDef MSR of P0 of a pre-Zen processor and, on family 14h, the main PLL its P-states divide. Core
 * Performance Boost puts the boosted P-states first and D18F4x15C of the first node counts them; unread, P0 stays
 * unknown. Called by the cpu family's probe.
 *
 * \param Identity The processor identity.
 * \param P0Definition Receives the MSR, or 0 when unknown.
 * \param ClockPowerTimingControl0 Receives D18F3xD4 of the first node on family 14h, else 0.
 */
VOID PhpHwPciQueryLegacyClock(
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PULONG P0Definition,
    _Out_ PULONG ClockPowerTimingControl0
    )
{
    ULONG value;

    *P0Definition = PH_AMD_MSR_PSTATE_DEF0;
    *ClockPowerTimingControl0 = 0;

    if (Identity->DisplayFamily == 0x14)
    {
        PhpHwPciFindNodes();

        if (PhpHwPciMiscNodeCount && PhpHwPciReadOnce(PhpHwPciMiscNodes[0], PH_AMD_D18F3_CLOCK_POWER_TIMING_CONTROL0, &value))
            *ClockPowerTimingControl0 = value;
    }

    if (!PhpHwQueryCoreBoost())
        return;

    *P0Definition = 0;
    PhpHwPciFindNodes();

    if (PhpHwPciLinkNodeCount && PhpHwPciReadOnce(PhpHwPciLinkNodes[0], PH_AMD_D18F4_CPB_CONTROL, &value))
        *P0Definition = PH_AMD_MSR_PSTATE_DEF0 + PhAmdBoostStateCount(value, Identity);
}

/**
 * Splits the D18F3 functions evenly over the packages in bus order, two at most each; otherwise package 0 takes the
 * first.
 */
static VOID PhpHwPciAssignNodes(
    VOID
    )
{
    ULONG step;

    if (!PhpHwPciMiscNodeCount)
        return;

    step = PhpHwPciMiscNodeCount / PhpHwPackageCount;

    if (PhpHwPciMiscNodeCount % PhpHwPackageCount == 0 && step <= PHP_HW_PCI_PACKAGE_NODES)
    {
        for (ULONG i = 0; i < PhpHwPackageCount; i++)
        {
            PhpHwPciPackages[i].NodeCount = step;

            for (ULONG j = 0; j < step; j++)
                PhpHwPciPackages[i].Nodes[j] = PhpHwPciMiscNodes[i * step + j];
        }
    }
    else
    {
        PhpHwPciPackages[0].NodeCount = 1;
        PhpHwPciPackages[0].Nodes[0] = PhpHwPciMiscNodes[0];
    }
}

/**
 * Gives each package the first D18F4 of its share, the functions split evenly over the packages in bus order;
 * otherwise package 0 takes the first.
 */
static VOID PhpHwPciAssignPowerNodes(
    VOID
    )
{
    if (!PhpHwPciLinkNodeCount)
        return;

    if (PhpHwPciLinkNodeCount % PhpHwPackageCount == 0)
    {
        ULONG step = PhpHwPciLinkNodeCount / PhpHwPackageCount;

        for (ULONG i = 0; i < PhpHwPackageCount; i++)
        {
            PhpHwPciPackages[i].HasPowerNode = TRUE;
            PhpHwPciPackages[i].PowerNode = PhpHwPciLinkNodes[i * step];
        }
    }
    else
    {
        PhpHwPciPackages[0].HasPowerNode = TRUE;
        PhpHwPciPackages[0].PowerNode = PhpHwPciLinkNodes[0];
    }
}

/**
 * Finds the nodes of each package and keeps the registers that read and decode: ReportedTempCtrl of each node
 * (not on 15h models 60h to 7Fh, whose temperature is the smn family's), and on 15h and 16h the TDP of the power
 * node, as its power limit, and the TDP running average. Called once a session.
 *
 * \return STATUS_SUCCESS, or STATUS_NOT_SUPPORTED without such a processor or a register to read.
 */
NTSTATUS PhpHwPciProbe(
    VOID
    )
{
    static CONST USHORT bridgeIds[] = { 0x14DB, 0x14DD };
    BOOLEAN kept = FALSE;

    // The bridges are read for the report whatever the processor.
    PhpHwPciProbed = TRUE;
    PhpHwPciBridgeCount = PhpHwEnumeratePciFunctions(
        PHP_HW_PCI_AMD_VENDOR_ID,
        PHP_HW_PCI_AMD_VENDOR_ID,
        0x0604,
        bridgeIds,
        RTL_NUMBER_OF(bridgeIds),
        FALSE,
        PhpHwPciBridges,
        RTL_NUMBER_OF(PhpHwPciBridges)
        );

    if (!PhpHwCpuQueryAmdLegacy(&PhpHwPciIdentity))
        return PhpHwPciBridgeCount ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;

    PhpHwPciFindNodes();
    PhpHwPciPackages = PhAllocateZero(sizeof(PHP_HW_PCI_PACKAGE) * max(PhpHwPackageCount, 1));

    if (!(PhpHwPciIdentity.DisplayFamily == 0x15 && PhpHwPciIdentity.DisplayModel >= 0x60))
        PhpHwPciAssignNodes();

    if (PhpHwPciIdentity.DisplayFamily == 0x15 || PhpHwPciIdentity.DisplayFamily == 0x16)
        PhpHwPciAssignPowerNodes();

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_PCI_PACKAGE package = &PhpHwPciPackages[i];
        static CONST ULONG temperatureOffset = PH_AMD_D18F3_REPORTED_TEMP_CTRL;
        static CONST ULONG tdpOffset = PH_AMD_D18F4_PROCESSOR_TDP;
        static CONST ULONG powerOffsets[2] = { PH_AMD_D18F5_TDP_LIMIT3, PH_AMD_D18F5_TDP_RUNNING_AVERAGE };
        KPH_HW_VALUE32 values[2];
        FLOAT value;

        for (ULONG j = 0; j < package->NodeCount; j++)
        {
            PhpHwPciRead(package->Nodes[j], &temperatureOffset, 1, values);

            if (NT_SUCCESS(values[0].Status) && PhAmdReportedTempCtrlToCelsius(values[0].Value, &PhpHwPciIdentity, &value))
            {
                package->Temperature[j].Enabled = TRUE;
                kept = TRUE;
            }
        }

        if (!package->HasPowerNode)
            continue;

        PhpHwPciRead(package->PowerNode, &tdpOffset, 1, values);

        if (!NT_SUCCESS(values[0].Status))
            continue;

        package->ProcessorTdp = values[0].Value;
        PhpHwPciRead(package->PowerNode + 1, powerOffsets, 2, values);

        if (!NT_SUCCESS(values[0].Status))
            continue;

        package->TdpLimit3 = values[0].Value;

        if (PhAmdProcessorTdpToWatts(package->ProcessorTdp, package->TdpLimit3, &value))
            PhpHwPackageSensors[i].PowerLimit = value;

        if (
            NT_SUCCESS(values[1].Status) &&
            PhAmdTdpRunningAverageToWatts(values[1].Value, package->ProcessorTdp, package->TdpLimit3, &PhpHwPciIdentity, &value)
            )
        {
            package->RunningAverage.Enabled = TRUE;
            kept = TRUE;
        }
    }

    return kept || PhpHwPciBridgeCount ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

/**
 * Reads the configuration space of each bridge over the ranges its row permits, for the report.
 */
static VOID PhpHwPciReadBridges(
    VOID
    )
{
    static CONST ULONG ranges[][2] = { { 0, 0xC4 }, { 0xCC, 0x34 }, { PHP_HW_PCI_CONFIG_HEADER, PHP_HW_PCI_CONFIG_LENGTH - PHP_HW_PCI_CONFIG_HEADER } };

    if (!PhpHwPciBridgeConfigs)
        PhpHwPciBridgeConfigs = PhAllocate(sizeof(PHP_HW_PCI_BRIDGE_CONFIG) * PhpHwPciBridgeCount);

    for (ULONG i = 0; i < PhpHwPciBridgeCount; i++)
    {
        PPHP_HW_PCI_BRIDGE_CONFIG config = &PhpHwPciBridgeConfigs[i];

        memset(config, 0, sizeof(PHP_HW_PCI_BRIDGE_CONFIG));

        for (ULONG j = 0; j < RTL_NUMBER_OF(ranges); j++)
        {
            NTSTATUS status = KphHwPciConfigReadBlock(PhpHwPciBridges[i], ranges[j][0], &config->Bytes[ranges[j][0]], ranges[j][1]);

            if (NT_SUCCESS(status))
                memset(&config->Readable[ranges[j][0]], TRUE, ranges[j][1]);
            else
                memset(&config->Bytes[ranges[j][0]], 0, ranges[j][1]);

            if (j == 0)
                config->Status = status;
        }
    }
}

/**
 * Reads each kept register in one call per function: the package temperature, the hottest of its nodes; the package
 * power from the running average, in place of the compute units' sum when it decodes.
 *
 * \return STATUS_SUCCESS.
 */
NTSTATUS PhpHwPciPass(
    VOID
    )
{
    static CONST ULONG temperatureOffset = PH_AMD_D18F3_REPORTED_TEMP_CTRL;
    static CONST ULONG averageOffset = PH_AMD_D18F5_TDP_RUNNING_AVERAGE;
    ULONG capabilities = 0;

    if (PhpHwReportReads && PhpHwPciBridgeCount)
        PhpHwPciReadBridges();

    for (ULONG i = 0; PhpHwPciPackages && i < PhpHwPackageCount; i++)
    {
        PPHP_HW_PCI_PACKAGE package = &PhpHwPciPackages[i];
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];
        KPH_HW_VALUE32 value;
        ULONG owned = 0;
        ULONG valid = 0;
        FLOAT maximum = 0.0f;
        FLOAT celsius;
        FLOAT watts;

        for (ULONG j = 0; j < package->NodeCount; j++)
        {
            if (!package->Temperature[j].Enabled)
                continue;

            owned = PH_HW_CAP_PACKAGE_TEMP;
            PhpHwPciRead(package->Nodes[j], &temperatureOffset, 1, &value);

            if (
                PhpHwPciApplyStatus(&package->Temperature[j], value.Status) &&
                PhAmdReportedTempCtrlToCelsius(value.Value, &PhpHwPciIdentity, &celsius) &&
                (!(valid & PH_HW_CAP_PACKAGE_TEMP) || celsius > maximum)
                )
            {
                maximum = celsius;
                valid = PH_HW_CAP_PACKAGE_TEMP;
            }
        }

        if (valid)
            sensor->Temperature = maximum;
        if (owned)
            WriteULongRelease(&sensor->Valid, (ReadULongAcquire(&sensor->Valid) & ~owned) | valid);

        capabilities |= owned;

        if (!package->RunningAverage.Enabled)
            continue;

        capabilities |= PH_HW_CAP_RAPL_PACKAGE;
        PhpHwPciRead(package->PowerNode + 1, &averageOffset, 1, &value);

        // A value that does not decode leaves the compute units' power of this update.
        if (
            PhpHwPciApplyStatus(&package->RunningAverage, value.Status) &&
            PhAmdTdpRunningAverageToWatts(value.Value, package->ProcessorTdp, package->TdpLimit3, &PhpHwPciIdentity, &watts)
            )
        {
            sensor->PackagePower = watts;
            WriteULongRelease(&sensor->Valid, ReadULongAcquire(&sensor->Valid) | PH_HW_CAP_RAPL_PACKAGE);
        }
    }

    PhpHwFamilies[PhpHwFamilyPciConfig].Capabilities = capabilities;

    return STATUS_SUCCESS;
}

/**
 * Ends the session: the functions are found and identified again at the next.
 */
VOID PhpHwPciEnd(
    VOID
    )
{
    if (PhpHwPciPackages)
    {
        PhFree(PhpHwPciPackages);
        PhpHwPciPackages = NULL;
    }

    if (PhpHwPciBridgeConfigs)
    {
        PhFree(PhpHwPciBridgeConfigs);
        PhpHwPciBridgeConfigs = NULL;
    }

    PhpHwPciProbed = FALSE;
    PhpHwPciBridgeCount = 0;
    PhpHwPciNodesFound = FALSE;
    PhpHwPciMiscNodeCount = 0;
    PhpHwPciLinkNodeCount = 0;
    PhpHwPciIdentifiedCount = 0;
}

/**
 * Writes the collector's section of the hardware report: the functions found and each package's registers.
 *
 * \param Builder The report.
 */
VOID PhpHwPciDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PhAppendFormatStringBuilder(Builder, L"  Functions: %lu D18F3, %lu D18F4, %lu bridges\n", PhpHwPciMiscNodeCount, PhpHwPciLinkNodeCount, PhpHwPciBridgeCount);

    for (ULONG i = 0; PhpHwPciPackages && i < PhpHwPackageCount; i++)
    {
        PPHP_HW_PCI_PACKAGE package = &PhpHwPciPackages[i];

        for (ULONG j = 0; j < package->NodeCount; j++)
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"  Package %lu node 0x%lx: ReportedTempCtrl %s\n",
                i,
                package->Nodes[j],
                package->Temperature[j].Enabled ? L"read" : L"not read"
                );
        }

        if (package->HasPowerNode)
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"  Package %lu power node 0x%lx: TDP 0x%08lx, TDP limit 3 0x%08lx, running average %s\n",
                i,
                package->PowerNode,
                package->ProcessorTdp,
                package->TdpLimit3,
                package->RunningAverage.Enabled ? L"read" : L"not read"
                );
        }
    }
}

/**
 * Reads a little-endian configuration register from a report read.
 *
 * \return TRUE if every byte of the register was read.
 */
static BOOLEAN PhpHwPciConfigValue(
    _In_ PPHP_HW_PCI_BRIDGE_CONFIG Config,
    _In_ ULONG Offset,
    _In_range_(1, 4) ULONG Size,
    _Out_ PULONG Value
    )
{
    *Value = 0;

    if (Offset > PHP_HW_PCI_CONFIG_LENGTH || Size > PHP_HW_PCI_CONFIG_LENGTH - Offset)
        return FALSE;

    for (ULONG i = 0; i < Size; i++)
    {
        if (!Config->Readable[Offset + i])
            return FALSE;
    }

    memcpy(Value, &Config->Bytes[Offset], Size);
    return TRUE;
}

/**
 * Finds a capability of the standard list.
 *
 * \return The capability offset, zero if absent or not readable.
 */
static ULONG PhpHwPciFindCapability(
    _In_ PPHP_HW_PCI_BRIDGE_CONFIG Config,
    _In_ UCHAR Id
    )
{
    ULONG value;
    ULONG offset;

    if (!PhpHwPciConfigValue(Config, 0x06, 2, &value) || !(value & PHP_HW_PCI_STATUS_CAPABILITIES))
        return 0;
    if (!PhpHwPciConfigValue(Config, 0x34, 1, &offset))
        return 0;

    for (ULONG ttl = 48; ttl && (offset &= 0xFC) >= 0x40; ttl--)
    {
        if (!PhpHwPciConfigValue(Config, offset, 2, &value))
            return 0;
        if ((value & 0xFF) == Id)
            return offset;

        offset = value >> 8;
    }

    return 0;
}

/**
 * Finds a capability of the extended list.
 *
 * \param Read Receives TRUE when the chain was read to its end or to the capability.
 * \return The capability offset, zero if absent or not readable.
 */
static ULONG PhpHwPciFindExtendedCapability(
    _In_ PPHP_HW_PCI_BRIDGE_CONFIG Config,
    _In_ USHORT Id,
    _Out_ PBOOLEAN Read
    )
{
    ULONG header;
    ULONG offset = PHP_HW_PCI_CONFIG_HEADER;

    *Read = FALSE;

    for (ULONG ttl = (PHP_HW_PCI_CONFIG_LENGTH - PHP_HW_PCI_CONFIG_HEADER) / sizeof(ULONG); ttl; ttl--)
    {
        if (!PhpHwPciConfigValue(Config, offset, 4, &header))
            return 0;

        *Read = TRUE;

        if (header == 0 || header == ULONG_MAX)
            return 0;
        if ((header & 0xFFFF) == Id)
            return offset;

        offset = (header >> 20) & 0xFFC;

        if (offset < PHP_HW_PCI_CONFIG_HEADER)
            return 0;

        *Read = FALSE;
    }

    return 0;
}

/**
 * Appends the extended capability chain as offset:ID pairs, or what stopped the walk.
 */
static VOID PhpHwPciAppendExtendedList(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_ PPHP_HW_PCI_BRIDGE_CONFIG Config
    )
{
    ULONG header;
    ULONG offset = PHP_HW_PCI_CONFIG_HEADER;

    PhAppendStringBuilder2(Builder, L"\n    Extended capabilities:");

    for (ULONG ttl = (PHP_HW_PCI_CONFIG_LENGTH - PHP_HW_PCI_CONFIG_HEADER) / sizeof(ULONG); ttl; ttl--)
    {
        if (!PhpHwPciConfigValue(Config, offset, 4, &header))
        {
            PhAppendFormatStringBuilder(Builder, L" 0x%03lx not read", offset);
            return;
        }

        if (header == 0 || header == ULONG_MAX)
        {
            if (offset == PHP_HW_PCI_CONFIG_HEADER)
                PhAppendFormatStringBuilder(Builder, L" none (0x100 reads 0x%08lx)", header);
            else
                PhAppendFormatStringBuilder(Builder, L" 0x%03lx reads 0x%08lx", offset, header);

            return;
        }

        PhAppendFormatStringBuilder(Builder, L" 0x%03lx:%04lx", offset, header & 0xFFFF);

        offset = (header >> 20) & 0xFFC;

        if (offset < PHP_HW_PCI_CONFIG_HEADER)
            return;
    }
}

/**
 * Appends one register of the error-policy census, or that it was not read.
 */
static VOID PhpHwPciAppendRegister(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_ PPHP_HW_PCI_BRIDGE_CONFIG Config,
    _In_ PCWSTR Name,
    _In_ ULONG Offset,
    _In_range_(1, 4) ULONG Size,
    _Out_ PULONG Value
    )
{
    if (PhpHwPciConfigValue(Config, Offset, Size, Value))
        PhAppendFormatStringBuilder(Builder, Size == 4 ? L"\n    %s 0x%08lx" : L"\n    %s 0x%04lx", Name, *Value);
    else
        PhAppendFormatStringBuilder(Builder, L"\n    %s not read", Name);
}

/**
 * Writes the error policy of the bridges the pciconfig script lists, as the last report request read them: the PCI
 * Express error-reporting enables, Root Control, the AER handling of Unsupported Request and Advisory Non-Fatal, and
 * DPC Control.
 *
 * \param Builder The report.
 */
VOID PhHwWriteBridgeDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PCWSTR portTypes[] =
    {
        L"endpoint", L"legacy endpoint", L"type 2", L"type 3", L"root port", L"upstream port", L"downstream port",
        L"PCI Express to PCI bridge", L"PCI to PCI Express bridge", L"root complex integrated endpoint", L"root complex event collector"
    };

    if (!PhpHwPciProbed)
    {
        PhAppendStringBuilder2(Builder, L"Not read: hardware sensors are off\n");
        return;
    }

    if (!PhpHwPciBridgeCount)
    {
        PhAppendStringBuilder2(Builder, L"None\n");
        return;
    }

    if (!PhpHwPciBridgeConfigs)
    {
        PhAppendStringBuilder2(Builder, L"Not read: no update made the report reads\n");
        return;
    }

    for (ULONG i = 0; i < PhpHwPciBridgeCount; i++)
    {
        PPHP_HW_PCI_BRIDGE_CONFIG config = &PhpHwPciBridgeConfigs[i];
        ULONG locator = PhpHwPciBridges[i];
        ULONG ids;
        ULONG value;
        ULONG express;
        ULONG aer;
        ULONG dpc;
        BOOLEAN chainRead;
        ULONG portType = ULONG_MAX;

        PhAppendFormatStringBuilder(
            Builder,
            L"%04lx:%02lx:%02lx.%lx",
            locator >> 16,
            (locator >> 8) & 0xFF,
            (locator >> 3) & 0x1F,
            locator & 0x7
            );

        if (!NT_SUCCESS(config->Status))
        {
            PhAppendFormatStringBuilder(Builder, L": status 0x%08lx\n", config->Status);
            continue;
        }

        if (PhpHwPciConfigValue(config, 0x00, 4, &ids))
            PhAppendFormatStringBuilder(Builder, L" %04lx:%04lx", ids & 0xFFFF, ids >> 16);

        express = PhpHwPciFindCapability(config, PHP_HW_PCI_CAP_EXPRESS);

        if (!express)
        {
            PhAppendStringBuilder2(Builder, L": no PCI Express capability read\n");
            continue;
        }

        if (PhpHwPciConfigValue(config, express + 0x02, 2, &value))
        {
            portType = (value >> 4) & 0xF;
            PhAppendFormatStringBuilder(Builder, L", %s", portType < RTL_NUMBER_OF(portTypes) ? portTypes[portType] : L"reserved type");
        }

        PhpHwPciAppendRegister(Builder, config, L"Device Control", express + 0x08, 2, &value);

        if (PhpHwPciConfigValue(config, express + 0x08, 2, &value))
        {
            PhAppendFormatStringBuilder(
                Builder,
                L": reporting correctable %lu, non-fatal %lu, fatal %lu, unsupported request %lu",
                value & 1,
                (value >> 1) & 1,
                (value >> 2) & 1,
                (value >> 3) & 1
                );
        }

        if (portType == 4 || portType == 10)
        {
            PhpHwPciAppendRegister(Builder, config, L"Root Control", express + 0x1C, 2, &value);

            if (PhpHwPciConfigValue(config, express + 0x1C, 2, &value))
            {
                PhAppendFormatStringBuilder(
                    Builder,
                    L": system error on correctable %lu, non-fatal %lu, fatal %lu",
                    value & 1,
                    (value >> 1) & 1,
                    (value >> 2) & 1
                    );
            }
        }

        PhpHwPciAppendExtendedList(Builder, config);

        aer = PhpHwPciFindExtendedCapability(config, PHP_HW_PCI_EXT_CAP_AER, &chainRead);

        if (aer)
        {
            ULONG mask;
            ULONG severity;
            ULONG correctable;

            if (
                PhpHwPciConfigValue(config, aer + 0x08, 4, &mask) &&
                PhpHwPciConfigValue(config, aer + 0x0C, 4, &severity) &&
                PhpHwPciConfigValue(config, aer + 0x14, 4, &correctable)
                )
            {
                PhAppendFormatStringBuilder(
                    Builder,
                    L"\n    AER uncorrectable mask 0x%08lx, severity 0x%08lx, correctable mask 0x%08lx: "
                    L"unsupported request %s, %s; advisory non-fatal %s",
                    mask,
                    severity,
                    correctable,
                    (mask & (1UL << 20)) ? L"masked" : L"unmasked",
                    (severity & (1UL << 20)) ? L"fatal" : L"non-fatal",
                    (correctable & (1UL << 13)) ? L"masked" : L"unmasked"
                    );
            }
            else
            {
                PhAppendStringBuilder2(Builder, L"\n    AER not read");
            }

            if (portType == 4 || portType == 10)
                PhpHwPciAppendRegister(Builder, config, L"AER Root Error Command", aer + 0x2C, 4, &value);
        }
        else
        {
            PhAppendStringBuilder2(Builder, chainRead ? L"\n    AER absent" : L"\n    AER not read");
        }

        dpc = PhpHwPciFindExtendedCapability(config, PHP_HW_PCI_EXT_CAP_DPC, &chainRead);

        if (dpc)
        {
            static CONST PCWSTR triggers[] = { L"disabled", L"on ERR_FATAL", L"on ERR_NONFATAL or ERR_FATAL", L"reserved" };

            PhpHwPciAppendRegister(Builder, config, L"DPC Control", dpc + 0x06, 2, &value);

            if (PhpHwPciConfigValue(config, dpc + 0x06, 2, &value))
                PhAppendFormatStringBuilder(Builder, L": trigger %s, interrupt %lu, ERR_COR %lu", triggers[value & 3], (value >> 3) & 1, (value >> 4) & 1);
        }
        else
        {
            PhAppendStringBuilder2(Builder, chainRead ? L"\n    DPC absent" : L"\n    DPC not read");
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }
}

#define PHP_HW_PROM21_ID_BASE 0x50320000 // named sensor ids of the second and later chipset dies: + die

static CONST USHORT PhpHwProm21Devices[] = { 0x43F7, 0x43F9, 0x43FA, 0x43FC, 0x43FD };
static ULONG PhpHwProm21Locators[KPH_HW_PROM21_DIES];
static ULONG PhpHwProm21NamedIndex[KPH_HW_PROM21_DIES]; // the second and later dies
static ULONG PhpHwProm21Count = 0;
static NTSTATUS PhpHwProm21Status = STATUS_NOT_SUPPORTED; // the last read, for the report
static UCHAR PhpHwProm21Strikes = 0;
static BOOLEAN PhpHwProm21Ended = FALSE;

/**
 * Finds the USB controllers of AMD Promontory 21 chipset dies, sorted by location: the first die is the chipset
 * temperature, each other a named temperature.
 *
 * \return STATUS_SUCCESS, or STATUS_NOT_SUPPORTED when there is none.
 */
NTSTATUS PhpHwProm21Probe(
    VOID
    )
{
    PhpHwProm21Count = PhpHwEnumeratePciFunctions(
        0x1022,
        0x1022,
        0x0C03,
        PhpHwProm21Devices,
        RTL_NUMBER_OF(PhpHwProm21Devices),
        FALSE,
        PhpHwProm21Locators,
        RTL_NUMBER_OF(PhpHwProm21Locators)
        );

    if (!PhpHwProm21Count)
        return STATUS_NOT_SUPPORTED;

    for (ULONG i = 1; i < PhpHwProm21Count; i++)
    {
        PPH_STRING name = PhFormatString(L"Chipset die %lu", i + 1);

        PhpHwProm21NamedIndex[i] = PhpHwAddNamedSensor(PhHwNamedSensorAcpiTemperature, 0, PHP_HW_PROM21_ID_BASE + i, name->Buffer);
        PhDereferenceObject(name);
    }

    return STATUS_SUCCESS;
}

/**
 * Publishes one read of every die: C = raw x 0.9066 - 78.624, a raw 0 or a die that is not in D0 no value. Three failed
 * reads in a row end the source; a bus another tool holds is a retry.
 *
 * \return STATUS_RETRY when another tool holds the bus, STATUS_NOT_SUPPORTED once ended, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwProm21Pass(
    VOID
    )
{
    KPH_HW_VALUE32 values[KPH_HW_PROM21_DIES];
    ULONG capabilities = 0;
    BOOLEAN read;

    if (PhpHwProm21Ended || !PhpHwProm21Count)
        return STATUS_NOT_SUPPORTED;

    PhpHwProm21Status = KphHwProm21Read(PhpHwProm21Locators, PhpHwProm21Count, values);

    if (PhpHwProm21Status == STATUS_RETRY)
        return STATUS_RETRY;

    read = NT_SUCCESS(PhpHwProm21Status);

    if (read)
        PhpHwProm21Strikes = 0;
    else if (++PhpHwProm21Strikes >= 3)
        PhpHwProm21Ended = TRUE;

    for (ULONG i = 0; i < PhpHwProm21Count; i++)
    {
        BOOLEAN valid = read && NT_SUCCESS(values[i].Status) && (values[i].Value & 0xFF) != 0;
        FLOAT celsius = valid ? (FLOAT)(values[i].Value & 0xFF) * 0.9066f - 78.624f : 0.0f;

        if (i == 0)
        {
            PhpHwChipsetTemperature = celsius;
            PhpHwChipsetTemperatureValid = valid;

            if (valid)
                capabilities |= PH_HW_CAP_CHIPSET_TEMP;
        }
        else
        {
            PhpHwSetNamedSensor(PhpHwProm21NamedIndex[i], valid, celsius);

            if (valid)
                capabilities |= PH_HW_CAP_ACPI_TEMP;
        }
    }

    PhpHwFamilies[PhpHwFamilyProm21].Capabilities = PhpHwProm21Ended ? 0 : capabilities;

    return PhpHwProm21Ended ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}

/**
 * Ends the session: the controllers are found again at the next.
 */
VOID PhpHwProm21End(
    VOID
    )
{
    PhpHwProm21Status = STATUS_NOT_SUPPORTED;
    PhpHwProm21Count = 0;
    PhpHwProm21Strikes = 0;
    PhpHwProm21Ended = FALSE;
}

/**
 * Writes the Promontory 21 source's section of the hardware report.
 *
 * \param Builder The report.
 */
VOID PhpHwProm21Diagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PCWSTR ended = L"";

    if (PhpHwProm21Ended)
        ended = L", ended";

    PhAppendFormatStringBuilder(
        Builder,
        L"  Chipset dies: last read 0x%08lx, %lu found%s\n",
        PhpHwProm21Status,
        PhpHwProm21Count,
        ended
        );
}

#define PHP_HW_AQTION_ID_BASE 0x41510000 // named sensor ids of AQtion network controllers: + controller

static CONST USHORT PhpHwAqtionDevices[] =
{
    0x07B1, 0x08B1, 0x09B1, 0x87B1, 0x88B1, 0x89B1, // first generation
    0x00C0, 0x04C0, 0x94C0, 0x93C0, 0x14C0, 0x34C0, 0x12C0, 0x11C0, // second generation
};
static ULONG PhpHwAqtionLocators[KPH_HW_AQTION_CONTROLLERS];
static ULONG PhpHwAqtionNamedIndex[KPH_HW_AQTION_CONTROLLERS];
static ULONG PhpHwAqtionCount = 0;
static NTSTATUS PhpHwAqtionStatus = STATUS_NOT_SUPPORTED; // the last read, for the report
static UCHAR PhpHwAqtionStrikes = 0;
static BOOLEAN PhpHwAqtionEnded = FALSE;

/**
 * Finds the AQtion network controllers, sorted by location, each a named temperature.
 *
 * \return STATUS_SUCCESS, or STATUS_NOT_SUPPORTED when there is none.
 */
NTSTATUS PhpHwAqtionProbe(
    VOID
    )
{
    PhpHwAqtionCount = PhpHwEnumeratePciFunctions(
        0x1D6A,
        0x1D6A,
        0x0200,
        PhpHwAqtionDevices,
        RTL_NUMBER_OF(PhpHwAqtionDevices),
        FALSE,
        PhpHwAqtionLocators,
        RTL_NUMBER_OF(PhpHwAqtionLocators)
        );

    if (!PhpHwAqtionCount)
        return STATUS_NOT_SUPPORTED;

    for (ULONG i = 0; i < PhpHwAqtionCount; i++)
    {
        PPH_STRING name = PhFormatString(L"Network controller %lu", i + 1);

        PhpHwAqtionNamedIndex[i] = PhpHwAddNamedSensor(PhHwNamedSensorAcpiTemperature, 0, PHP_HW_AQTION_ID_BASE + i, name->Buffer);
        PhDereferenceObject(name);
    }

    return STATUS_SUCCESS;
}

/**
 * Publishes one read of every controller: millidegrees C, a controller whose read failed no value. Three failed
 * reads in a row end the source; a bus another tool holds is a retry.
 *
 * \return STATUS_RETRY when another tool holds the bus, STATUS_NOT_SUPPORTED once ended, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwAqtionPass(
    VOID
    )
{
    KPH_HW_VALUE32 values[KPH_HW_AQTION_CONTROLLERS];
    ULONG capabilities = 0;
    BOOLEAN read;

    if (PhpHwAqtionEnded || !PhpHwAqtionCount)
        return STATUS_NOT_SUPPORTED;

    PhpHwAqtionStatus = KphHwAqtionRead(PhpHwAqtionLocators, PhpHwAqtionCount, values);

    if (PhpHwAqtionStatus == STATUS_RETRY)
        return STATUS_RETRY;

    read = NT_SUCCESS(PhpHwAqtionStatus);

    if (read)
        PhpHwAqtionStrikes = 0;
    else if (++PhpHwAqtionStrikes >= 3)
        PhpHwAqtionEnded = TRUE;

    for (ULONG i = 0; i < PhpHwAqtionCount; i++)
    {
        BOOLEAN valid = read && NT_SUCCESS(values[i].Status);
        FLOAT celsius = 0.0f;

        if (valid)
            celsius = (FLOAT)(LONG)values[i].Value / 1000.0f;

        PhpHwSetNamedSensor(PhpHwAqtionNamedIndex[i], valid, celsius);

        if (valid)
            capabilities |= PH_HW_CAP_ACPI_TEMP;
    }

    PhpHwFamilies[PhpHwFamilyAqtion].Capabilities = PhpHwAqtionEnded ? 0 : capabilities;

    return PhpHwAqtionEnded ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}

/**
 * Ends the session: the controllers are found again at the next.
 */
VOID PhpHwAqtionEnd(
    VOID
    )
{
    PhpHwAqtionStatus = STATUS_NOT_SUPPORTED;
    PhpHwAqtionCount = 0;
    PhpHwAqtionStrikes = 0;
    PhpHwAqtionEnded = FALSE;
}

/**
 * Writes the AQtion source's section of the hardware report.
 *
 * \param Builder The report.
 */
VOID PhpHwAqtionDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PCWSTR ended = L"";

    if (PhpHwAqtionEnded)
        ended = L", ended";

    PhAppendFormatStringBuilder(
        Builder,
        L"  Network controllers: last read 0x%08lx, %lu found%s\n",
        PhpHwAqtionStatus,
        PhpHwAqtionCount,
        ended
        );
}
