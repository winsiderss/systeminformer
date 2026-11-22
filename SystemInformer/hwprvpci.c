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
 * Ends the session: the functions are found and identified again at the next.
 */
VOID PhpHwPciEnd(
    VOID
    )
{
    PhpHwPciNodesFound = FALSE;
    PhpHwPciMiscNodeCount = 0;
    PhpHwPciLinkNodeCount = 0;
    PhpHwPciIdentifiedCount = 0;
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
