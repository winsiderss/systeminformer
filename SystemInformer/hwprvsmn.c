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

// Collector smn: the AMD system management network through each package's root complex, KphHwSmnRead once per package
// per update: Tctl and the populated CCD temperatures (Zen), or ReportedTempCtrl (family 15h models 60h to 7Fh).

#define PHP_HW_SMN_MAX_ROOTS 64
#define PHP_HW_SMN_AMD_VENDOR_ID 0x1022
#define PHP_HW_SMN_HYGON_VENDOR_ID 0x1D94
#define PHP_HW_SMN_HOST_BRIDGE_CLASS 0x0600
#define PHP_HW_SMN_STRIKE_LIMIT 3 // failures of an item before it leaves the plan for the session
#define PHP_HW_SMN_ITEMS (1 + PH_HW_MAX_CCD) // the temperature, then each CCD

typedef enum _PHP_HW_SMN_CLASS
{
    PhpHwSmnNone,
    PhpHwSmnZen, // family 17h and later: THM_TCON_CUR_TMP and the CCD registers
    PhpHwSmn15h // family 15h models 60h to 7Fh: D0F0xBC_xD8200CA4
} PHP_HW_SMN_CLASS;

typedef struct _PHP_HW_SMN_ITEM
{
    ULONG Address; // 0 when not in the plan
    BOOLEAN Disabled;
    UCHAR Strikes;
} PHP_HW_SMN_ITEM, *PPHP_HW_SMN_ITEM;

typedef struct _PHP_HW_SMN_PACKAGE
{
    ULONG Locator; // the root complex
    BOOLEAN HasRoot;
    PHP_HW_SMN_ITEM Items[PHP_HW_SMN_ITEMS];
} PHP_HW_SMN_PACKAGE, *PPHP_HW_SMN_PACKAGE;

static PHP_HW_SMN_CLASS PhpHwSmnClass = PhpHwSmnNone;
static KPH_HW_SMN_IDENTITY PhpHwSmnRootIdentity;
static PH_CPU_IDENTITY PhpHwSmnIdentity;
static PPHP_HW_SMN_PACKAGE PhpHwSmnPackages = NULL; // per package
static ULONG PhpHwSmnRootCount = 0; // discovered
static ULONG PhpHwSmnCcdBase = 0;
static ULONG PhpHwSmnCcdCount = 0;
static FLOAT PhpHwSmnTctlOffset = 0.0f;

/**
 * Applies the session failure policy to an item: a refused or misaligned address leaves the plan at once; contention
 * and the all-ones answer are no value, not a failure; anything else strikes.
 *
 * \return TRUE when the value is usable.
 */
static BOOLEAN PhpHwSmnApplyStatus(
    _Inout_ PPHP_HW_SMN_ITEM Item,
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
    case STATUS_RETRY: // the index moved
    case STATUS_DEVICE_NOT_READY: // the all-ones answer
        break;
    case STATUS_ACCESS_DENIED: // not permitted
    case STATUS_INVALID_PARAMETER: // not aligned
        Item->Disabled = TRUE;
        PhTraceInfo("Hardware sensor SMN 0x%08lx unavailable: 0x%lx", Item->Address, Status);
        break;
    default:
        if (++Item->Strikes >= PHP_HW_SMN_STRIKE_LIMIT)
        {
            Item->Disabled = TRUE;
            PhTraceInfo("Hardware sensor SMN 0x%08lx unavailable: 0x%lx", Item->Address, Status);
        }
        break;
    }

    return FALSE;
}

/**
 * Reads the planned items of a package in one call.
 *
 * \param Package The package.
 * \param Values Receives a value per item; a value is usable when its bit is set in the return.
 * \param Status Receives the status of the call.
 * \return The PHP_HW_SMN_ITEMS bits of the items that gave a value.
 */
static ULONG PhpHwSmnReadPackage(
    _Inout_ PPHP_HW_SMN_PACKAGE Package,
    _Out_writes_(PHP_HW_SMN_ITEMS) PULONG Values,
    _Out_ PNTSTATUS Status
    )
{
    ULONG addresses[PHP_HW_SMN_ITEMS];
    ULONG items[PHP_HW_SMN_ITEMS];
    KPH_HW_VALUE32 values[PHP_HW_SMN_ITEMS];
    ULONG count = 0;
    ULONG answered = 0;
    NTSTATUS status;

    memset(Values, 0, sizeof(ULONG) * PHP_HW_SMN_ITEMS);
    *Status = STATUS_SUCCESS;

    for (ULONG i = 0; i < PHP_HW_SMN_ITEMS; i++)
    {
        if (!Package->Items[i].Address || Package->Items[i].Disabled)
            continue;

        addresses[count] = Package->Items[i].Address;
        items[count] = i;
        count++;
    }

    if (!count)
        return 0;

    status = KphHwSmnRead(Package->Locator, addresses, count, values);
    *Status = status;

    for (ULONG i = 0; i < count; i++)
    {
        PPHP_HW_SMN_ITEM item = &Package->Items[items[i]];

        // A failed call is each item's status; contention backs the whole family off instead (D9).
        if (status == STATUS_RETRY)
            break;
        if (!PhpHwSmnApplyStatus(item, NT_SUCCESS(status) ? values[i].Status : status))
            continue;

        Values[items[i]] = values[i].Value;
        answered |= 1UL << items[i];
    }

    return answered;
}

/**
 * Splits the root complexes evenly over the packages in bus order; when they do not split evenly, the first package
 * takes the first.
 */
static VOID PhpHwSmnAssignRootComplexes(
    VOID
    )
{
    ULONG roots[PHP_HW_SMN_MAX_ROOTS];

    PhpHwSmnRootCount = PhpHwEnumeratePciFunctions(
        PHP_HW_SMN_AMD_VENDOR_ID,
        PHP_HW_SMN_HYGON_VENDOR_ID,
        PHP_HW_SMN_HOST_BRIDGE_CLASS,
        NULL,
        0,
        TRUE,
        roots,
        RTL_NUMBER_OF(roots)
        );

    if (!PhpHwSmnRootCount)
        return;

    if (PhpHwSmnRootCount % PhpHwPackageCount == 0)
    {
        ULONG step = PhpHwSmnRootCount / PhpHwPackageCount;

        for (ULONG i = 0; i < PhpHwPackageCount; i++)
        {
            PhpHwSmnPackages[i].HasRoot = TRUE;
            PhpHwSmnPackages[i].Locator = roots[i * step];
        }
    }
    else
    {
        PhpHwSmnPackages[0].HasRoot = TRUE;
        PhpHwSmnPackages[0].Locator = roots[0];
    }
}

/**
 * Identifies the root complex of the first package and plans the temperatures of every package. Called once a
 * session.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED for a machine or processor the collector does not serve, or the
 * status of the identify call.
 */
NTSTATUS PhpHwSmnProbe(
    VOID
    )
{
    NTSTATUS status;
    PPH_STRING brand;

    if (PhpHwQueryMachine() == IMAGE_FILE_MACHINE_ARM64 || !PhpHwPackageCount)
        return STATUS_NOT_SUPPORTED;

    PhpHwSmnPackages = PhAllocateZero(sizeof(PHP_HW_SMN_PACKAGE) * PhpHwPackageCount);
    PhpHwSmnAssignRootComplexes();

    if (!PhpHwSmnPackages[0].HasRoot)
        return STATUS_NOT_SUPPORTED;

    // The identity of the processor comes from the script, so this family does not depend on cpu.
    status = KphHwSmnIdentify(PhpHwSmnPackages[0].Locator, &PhpHwSmnRootIdentity);

    if (!NT_SUCCESS(status))
        return status;

    memset(&PhpHwSmnIdentity, 0, sizeof(PH_CPU_IDENTITY));
    memcpy(PhpHwSmnIdentity.Vendor, PhpHwSmnRootIdentity.Vendor == PHP_HW_SMN_HYGON_VENDOR_ID ? "HygonGenuine" : "AuthenticAMD", 12);
    PhpHwSmnIdentity.DisplayFamily = (USHORT)PhpHwSmnRootIdentity.Family;
    PhpHwSmnIdentity.DisplayModel = (UCHAR)PhpHwSmnRootIdentity.Model;
    PhpHwSmnIdentity.Stepping = (UCHAR)PhpHwSmnRootIdentity.Stepping;

    if (PhpHwSmnIdentity.DisplayFamily >= 0x17)
        PhpHwSmnClass = PhpHwSmnZen;
    else if (PhpHwSmnIdentity.DisplayFamily == 0x15 && PhpHwSmnIdentity.DisplayModel >= 0x60 && PhpHwSmnIdentity.DisplayModel <= 0x7F)
        PhpHwSmnClass = PhpHwSmn15h;
    else
        return STATUS_NOT_SUPPORTED;

    if (PhpHwSmnClass == PhpHwSmnZen)
    {
        if (!PhAmdTccdAddressRange(&PhpHwSmnIdentity, &PhpHwSmnCcdBase, &PhpHwSmnCcdCount))
        {
            PhpHwSmnCcdBase = 0;
            PhpHwSmnCcdCount = 0;
        }

        PhpHwSmnCcdCount = min(PhpHwSmnCcdCount, PH_HW_MAX_CCD);

        if (brand = PhpHwQueryProcessorName())
        {
            PhpHwSmnTctlOffset = PhAmdTctlOffset(&PhpHwSmnIdentity, &brand->sr);
            PhDereferenceObject(brand);
        }
    }

    // The probe reads every CCD slot once; a slot whose valid bit is clear is not populated.
    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SMN_PACKAGE package = &PhpHwSmnPackages[i];
        ULONG values[PHP_HW_SMN_ITEMS];
        ULONG answered;
        FLOAT celsius;

        if (!package->HasRoot)
            continue;

        if (PhpHwSmnClass == PhpHwSmnZen)
        {
            package->Items[0].Address = PH_AMD_SMN_THM_TCON_CUR_TMP;

            for (ULONG j = 0; j < PhpHwSmnCcdCount; j++)
                package->Items[1 + j].Address = PhpHwSmnCcdBase + j * 4;
        }
        else
        {
            package->Items[0].Address = PH_AMD_D0F0XBC_REPORTED_TEMP_CTRL;
        }

        answered = PhpHwSmnReadPackage(package, values, &status);

        for (ULONG j = 0; j < PhpHwSmnCcdCount; j++)
        {
            if ((answered & (1UL << (1 + j))) && !PhAmdTccdToCelsius(values[1 + j], &celsius))
                package->Items[1 + j].Disabled = TRUE;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Reads and publishes the temperatures of every package, one call a package.
 *
 * \return STATUS_RETRY when every call met contention, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwSmnPass(
    VOID
    )
{
    BOOLEAN answered = FALSE;
    BOOLEAN retried = FALSE;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SMN_PACKAGE package = &PhpHwSmnPackages[i];
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];
        ULONG values[PHP_HW_SMN_ITEMS];
        ULONG owned = 0;
        ULONG valid = 0;
        ULONG ccdValidMask = 0;
        ULONG got;
        NTSTATUS status;
        FLOAT celsius;

        if (!package->HasRoot)
            continue;

        for (ULONG j = 0; j < PHP_HW_SMN_ITEMS; j++)
        {
            if (package->Items[j].Address && !package->Items[j].Disabled)
                owned |= j ? PH_HW_CAP_CCD_TEMP : PH_HW_CAP_PACKAGE_TEMP;
        }

        if (!owned)
            continue;

        got = PhpHwSmnReadPackage(package, values, &status);

        if (status == STATUS_RETRY)
            retried = TRUE;
        else
            answered = TRUE;

        if (got & 1)
        {
            BOOLEAN decoded = PhpHwSmnClass == PhpHwSmnZen ?
                PhAmdTctlToCelsius(values[0], &celsius) :
                PhAmdReportedTempCtrlToCelsius(values[0], &PhpHwSmnIdentity, &celsius);

            if (decoded)
            {
                sensor->Temperature = celsius - PhpHwSmnTctlOffset;
                valid |= PH_HW_CAP_PACKAGE_TEMP;
            }
        }

        for (ULONG j = 0; j < PhpHwSmnCcdCount; j++)
        {
            if (!(got & (1UL << (1 + j))) || !PhAmdTccdToCelsius(values[1 + j], &celsius))
                continue;

            sensor->CcdTemperature[j] = celsius;

            if (!ccdValidMask || celsius > sensor->MaxCoreTemperature)
                sensor->MaxCoreTemperature = celsius;

            ccdValidMask |= 1UL << j;
            valid |= PH_HW_CAP_CCD_TEMP;
        }

        sensor->CcdValidMask = ccdValidMask;

        // Only the bits this collector owns are replaced; the others belong to cpu.
        WriteULongRelease(&sensor->Valid, (ReadULongAcquire(&sensor->Valid) & ~owned) | (valid & owned));
    }

    return retried && !answered ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Ends the session: the plan and the root complexes are found again at the next probe.
 */
VOID PhpHwSmnEnd(
    VOID
    )
{
    if (PhpHwSmnPackages)
    {
        PhFree(PhpHwSmnPackages);
        PhpHwSmnPackages = NULL;
    }

    PhpHwSmnClass = PhpHwSmnNone;
    PhpHwSmnRootCount = 0;
    PhpHwSmnCcdBase = 0;
    PhpHwSmnCcdCount = 0;
    PhpHwSmnTctlOffset = 0.0f;
}

/**
 * Writes the collector's section of the hardware report: the root complexes, the identity and each item's state.
 *
 * \param Builder The report.
 */
VOID PhpHwSmnDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    if (PhpHwSmnClass == PhpHwSmnNone)
    {
        PhAppendFormatStringBuilder(Builder, L"  Root complexes: %lu found\n", PhpHwSmnRootCount);
        return;
    }

    PhAppendFormatStringBuilder(
        Builder,
        L"  Root complexes: %lu found; %04x:%04x, family 0x%lx model 0x%lx stepping 0x%lx\n"
        L"  CCD registers: %lu from 0x%08lx; Tctl offset %.0f\n",
        PhpHwSmnRootCount,
        PhpHwSmnRootIdentity.Vendor,
        PhpHwSmnRootIdentity.Device,
        PhpHwSmnRootIdentity.Family,
        PhpHwSmnRootIdentity.Model,
        PhpHwSmnRootIdentity.Stepping,
        PhpHwSmnCcdCount,
        PhpHwSmnCcdBase,
        PhpHwSmnTctlOffset
        );

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SMN_PACKAGE package = &PhpHwSmnPackages[i];

        if (!package->HasRoot)
        {
            PhAppendFormatStringBuilder(Builder, L"  Package %lu: no root complex\n", i);
            continue;
        }

        PhAppendFormatStringBuilder(Builder, L"  Package %lu: root complex 0x%06lx\n", i, package->Locator);

        for (ULONG j = 0; j < PHP_HW_SMN_ITEMS; j++)
        {
            if (!package->Items[j].Address)
                continue;

            PhAppendFormatStringBuilder(
                Builder,
                L"    SMN 0x%08lx: %s\n",
                package->Items[j].Address,
                package->Items[j].Disabled ? L"left out" : L"read"
                );
        }
    }
}
