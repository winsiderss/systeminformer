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

// Collector pciconfig: the device 18h functions of AMD families 10h to 16h, read through pciconfig.lua.

#define PHP_HW_PCI_AMD_VENDOR_ID 0x1022
#define PHP_HW_PCI_HOST_BRIDGE_CLASS 0x0600
#define PHP_HW_PCI_MAX_NODES 8
#define PHP_HW_PCI_MAX_IDENTIFIED (PHP_HW_PCI_MAX_NODES * 3)
#define PHP_HW_PCI_PACKAGE_NODES 2 // D18F3 functions of a package
#define PHP_HW_PCI_STRIKE_LIMIT 3 // failures of a register before it ends for the session

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
    BOOLEAN kept = FALSE;

    if (!PhpHwCpuQueryAmdLegacy(&PhpHwPciIdentity))
        return STATUS_NOT_SUPPORTED;

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

    return kept ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
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

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
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
            if (ReadULongAcquire(&sensor->Valid) & PH_HW_CAP_RAPL_PACKAGE)
                PhHwCpuPower -= sensor->PackagePower;

            sensor->PackagePower = watts;
            PhHwCpuPower += watts;
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
    PhAppendFormatStringBuilder(Builder, L"  Functions: %lu D18F3, %lu D18F4\n", PhpHwPciMiscNodeCount, PhpHwPciLinkNodeCount);

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
 * Writes the PCI error policy of the bridges the pciconfig script lists, for the hardware report.
 *
 * \param Builder The report.
 */
VOID PhHwWriteBridgeDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PhAppendStringBuilder2(Builder, L"Not collected\n");
}
