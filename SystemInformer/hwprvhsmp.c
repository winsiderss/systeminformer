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
#include <hwsmu.h>
#include <trace.h>

// Collector hsmp: the AMD server firmware's HSMP mailbox of each socket, one KphHwHsmpCommand a package a second with
// the commands of phlib's PhHwSmuHsmpCommands still accepted, decoded by phlib into named sensors. On protocol 5 and 7
// a second call every PHP_HW_HSMP_DIMM_INTERVAL passes reads the DIMMs: its first sends GET_DIMM_THERMAL to each of
// the PHP_HW_HSMP_DIMM_PROBES mode-1 addresses 80h | sensor << 6 | DIMM << 4 | UMC and keeps those
// answered with the address echoed; later ones read their temperature, and the power of each sensor-0 address.

#define PHP_HW_HSMP_STRIKE_LIMIT 2 // consecutive busy or timed-out mailbox failures that end a package's mailbox
#define PHP_HW_HSMP_COMMAND_LIMIT 3 // consecutive firmware failures that drop a command
#define PHP_HW_HSMP_DIMM_INTERVAL 5 // passes between DIMM reads
#define PHP_HW_HSMP_DIMM_PROBES (PH_HW_HSMP_DIMM_CHANNELS * 4) // two DIMMs a channel, two sensors a DIMM

typedef struct _PHP_HW_HSMP_PACKAGE
{
    ULONG Locator; // the root complex smn assigned
    BOOLEAN Identified;
    BOOLEAN Ended;
    BOOLEAN MemoryClockValid; // the last pass decoded MCLK
    BOOLEAN LimitSourcesValid;
    UCHAR Strikes;
    ULONG Version;
    ULONG Dropped; // bit per PhHwSmuHsmpCommands entry
    ULONG LimitSources; // PH_HW_HSMP_LIMIT_*, for the report
    NTSTATUS Status; // the last call's, for the report
    FLOAT MemoryClock;
    UCHAR Failures[PH_HW_HSMP_MAX_COMMANDS];
    PHP_HW_SMU_SENSORS Sensors;
    BOOLEAN DimmProbed;
    BOOLEAN DimmPowerDropped;
    UCHAR DimmPowerFailures;
    UCHAR DimmCount;
    ULONG DimmPasses;
    UCHAR DimmAddresses[PHP_HW_HSMP_DIMM_PROBES];
    ULONG DimmThermal[PHP_HW_HSMP_DIMM_PROBES]; // the last answer, 0 for none
    ULONG DimmPower[PHP_HW_HSMP_DIMM_PROBES];
} PHP_HW_HSMP_PACKAGE, *PPHP_HW_HSMP_PACKAGE;

static PPHP_HW_HSMP_PACKAGE PhpHwHsmpPackages = NULL; // per package
static BOOLEAN PhpHwHsmpProbedThisUpdate = FALSE; // the probe's identify answers stand until the next update
static PH_HW_SMU_DECODE PhpHwHsmpDecode; // the pass's

/**
 * Applies the session failure policy to a package's mailbox: contention and nothing to read are no value; a busy,
 * timed-out or disturbed handshake strikes; anything else ends the mailbox for the session.
 *
 * \return TRUE when the call was answered.
 */
static BOOLEAN PhpHwHsmpApplyStatus(
    _Inout_ PPHP_HW_HSMP_PACKAGE Package,
    _In_ ULONG PackageIndex,
    _In_ NTSTATUS Status
    )
{
    Package->Status = Status;

    if (NT_SUCCESS(Status))
    {
        Package->Strikes = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_RETRY:
    case STATUS_DEVICE_NOT_READY:
    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_NO_MEMORY:
        break;
    case STATUS_DEVICE_BUSY:
    case STATUS_IO_TIMEOUT:
    case STATUS_DEVICE_DATA_ERROR:
        if (++Package->Strikes < PHP_HW_HSMP_STRIKE_LIMIT)
            break;
        __fallthrough;
    default:
        Package->Ended = TRUE;
        PhTraceInfo("Hardware sensors: HSMP of package %lu unavailable: 0x%lx", PackageIndex, Status);
        break;
    }

    return FALSE;
}

/**
 * Identifies a package's mailbox through its socket and root complex.
 */
static VOID PhpHwHsmpIdentify(
    _Inout_ PPHP_HW_HSMP_PACKAGE Package,
    _In_ ULONG PackageIndex
    )
{
    KPH_HW_HSMP_IDENTITY identity;

    if (PhpHwHsmpApplyStatus(Package, PackageIndex, KphHwHsmpIdentify(Package->Locator, PackageIndex, &identity)))
    {
        Package->Version = identity.Version;
        Package->Identified = TRUE;
    }
}

/**
 * Identifies each package's mailbox; the socket is the package's index in bus order. Called once a session.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED when no package has a mailbox, or the status of the first identify.
 */
NTSTATUS PhpHwHsmpProbe(
    VOID
    )
{
    NTSTATUS firstStatus = STATUS_NOT_SUPPORTED;
    BOOLEAN any = FALSE;

    if (PhpHwQueryMachine() == IMAGE_FILE_MACHINE_ARM64 || !PhpHwPackageCount)
        return STATUS_NOT_SUPPORTED;

    PhpHwHsmpPackages = PhAllocateZero(sizeof(PHP_HW_HSMP_PACKAGE) * PhpHwPackageCount);
    PhpHwHsmpProbedThisUpdate = TRUE;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_HSMP_PACKAGE package = &PhpHwHsmpPackages[i];

        if (!PhpHwSmnQueryRoot(i, &package->Locator))
        {
            package->Ended = TRUE;
            continue;
        }

        PhpHwHsmpIdentify(package, i);

        if (i == 0)
            firstStatus = package->Status;

        any |= !package->Ended;
    }

    if (!any)
        return NT_SUCCESS(firstStatus) ? STATUS_NOT_SUPPORTED : firstStatus;

    return STATUS_SUCCESS;
}

/**
 * Sends one batch to a package's mailbox and decodes its answers: a refused command is dropped for the session, a
 * command the firmware fails PHP_HW_HSMP_COMMAND_LIMIT times in a row is dropped, a cancelled one is no value.
 */
static VOID PhpHwHsmpRead(
    _Inout_ PPHP_HW_HSMP_PACKAGE Package,
    _In_ ULONG PackageIndex,
    _Inout_ PPH_HW_SMU_DECODE Decode
    )
{
    KPH_HW_HSMP_COMMAND commands[PH_HW_HSMP_MAX_COMMANDS];
    KPH_HW_HSMP_RESULT results[PH_HW_HSMP_MAX_COMMANDS];
    UCHAR entries[PH_HW_HSMP_MAX_COMMANDS];
    NTSTATUS mailbox = STATUS_SUCCESS; // the first handshake failure of the batch
    ULONG count = 0;
    NTSTATUS status;

    for (ULONG i = 0; i < PhHwSmuHsmpCommandCount; i++)
    {
        if (Package->Dropped & (1UL << i))
            continue;

        memset(&commands[count], 0, sizeof(KPH_HW_HSMP_COMMAND));
        commands[count].Opcode = PhHwSmuHsmpCommands[i];
        entries[count++] = (UCHAR)i;
    }

    if (!count)
    {
        Package->Ended = TRUE;
        return;
    }

    status = KphHwHsmpCommand(Package->Locator, PackageIndex, commands, count, results);

    if (!NT_SUCCESS(status))
    {
        PhpHwHsmpApplyStatus(Package, PackageIndex, status);
        return;
    }

    for (ULONG i = 0; i < count; i++)
    {
        ULONG entry = entries[i];
        NTSTATUS entryStatus = results[i].Status;

        if (NT_SUCCESS(entryStatus))
        {
            Package->Failures[entry] = 0;
            PhHwSmuDecodeHsmp(commands[i].Opcode, results[i].Results, Decode);
            continue;
        }

        switch (entryStatus)
        {
        case STATUS_CANCELLED:
            break;
        case STATUS_RETRY:
        case STATUS_DEVICE_NOT_READY:
        case STATUS_DEVICE_BUSY:
        case STATUS_IO_TIMEOUT:
        case STATUS_DEVICE_DATA_ERROR:
        case STATUS_INSUFFICIENT_RESOURCES:
        case STATUS_NO_MEMORY:
            if (NT_SUCCESS(mailbox))
                mailbox = entryStatus;
            break;
        case STATUS_INTERNAL_ERROR:
        case STATUS_UNSUCCESSFUL:
            if (++Package->Failures[entry] < PHP_HW_HSMP_COMMAND_LIMIT)
                break;
            __fallthrough;
        default:
            Package->Dropped |= 1UL << entry;
            PhTraceInfo("Hardware sensors: HSMP command 0x%lx of package %lu unavailable: 0x%lx", commands[i].Opcode, PackageIndex, entryStatus);
            break;
        }
    }

    PhpHwHsmpApplyStatus(Package, PackageIndex, mailbox);
}

/**
 * Sends a package's DIMM call: the probe until a probe call is answered, then each kept address's temperature and power.
 */
static VOID PhpHwHsmpReadDimms(
    _Inout_ PPHP_HW_HSMP_PACKAGE Package,
    _In_ ULONG PackageIndex
    )
{
    PKPH_HW_HSMP_COMMAND commands;
    PKPH_HW_HSMP_RESULT results;
    UCHAR entries[PHP_HW_HSMP_DIMM_PROBES * 2];
    NTSTATUS mailbox = STATUS_SUCCESS;
    ULONG count = 0;
    NTSTATUS status;

    commands = PhAllocateZero(sizeof(KPH_HW_HSMP_COMMAND) * PHP_HW_HSMP_DIMM_PROBES * 2);
    results = PhAllocateZero(sizeof(KPH_HW_HSMP_RESULT) * PHP_HW_HSMP_DIMM_PROBES * 2);

    if (!Package->DimmProbed)
    {
        for (ULONG i = 0; i < PHP_HW_HSMP_DIMM_PROBES; i++)
        {
            commands[count].Opcode = PH_HW_HSMP_GET_DIMM_THERMAL;
            commands[count].Arguments[0] = 0x80 | ((i / (PH_HW_HSMP_DIMM_CHANNELS * 2)) << 6) |
                (((i / PH_HW_HSMP_DIMM_CHANNELS) & 1) << 4) | (i % PH_HW_HSMP_DIMM_CHANNELS);
            entries[count++] = (UCHAR)i;
        }
    }
    else
    {
        for (ULONG i = 0; i < Package->DimmCount; i++)
        {
            commands[count].Opcode = PH_HW_HSMP_GET_DIMM_THERMAL;
            commands[count].Arguments[0] = Package->DimmAddresses[i];
            entries[count++] = (UCHAR)i;

            if (!Package->DimmPowerDropped && !(Package->DimmAddresses[i] & 0x40))
            {
                commands[count].Opcode = PH_HW_HSMP_GET_DIMM_POWER;
                commands[count].Arguments[0] = Package->DimmAddresses[i];
                entries[count++] = (UCHAR)i;
            }
        }
    }

    status = count ? KphHwHsmpCommand(Package->Locator, PackageIndex, commands, count, results) : STATUS_SUCCESS;

    memset(Package->DimmThermal, 0, sizeof(Package->DimmThermal));
    memset(Package->DimmPower, 0, sizeof(Package->DimmPower));

    if (!NT_SUCCESS(status))
    {
        PhpHwHsmpApplyStatus(Package, PackageIndex, status);
        goto CleanupExit;
    }

    for (ULONG i = 0; i < count; i++)
    {
        ULONG address = commands[i].Arguments[0];
        NTSTATUS entryStatus = results[i].Status;
        BOOLEAN echoed = NT_SUCCESS(entryStatus) && (results[i].Results[0] & 0xFF) == address;

        switch (entryStatus)
        {
        case STATUS_RETRY:
        case STATUS_DEVICE_NOT_READY:
        case STATUS_DEVICE_BUSY:
        case STATUS_IO_TIMEOUT:
        case STATUS_DEVICE_DATA_ERROR:
        case STATUS_INSUFFICIENT_RESOURCES:
        case STATUS_NO_MEMORY:
            if (NT_SUCCESS(mailbox))
                mailbox = entryStatus;
            continue;
        }

        if (!Package->DimmProbed)
        {
            if (echoed)
            {
                Package->DimmAddresses[Package->DimmCount] = (UCHAR)address;
                Package->DimmThermal[Package->DimmCount] = results[i].Results[0];
                Package->DimmCount++;
            }
        }
        else if (commands[i].Opcode == PH_HW_HSMP_GET_DIMM_THERMAL)
        {
            if (echoed)
                Package->DimmThermal[entries[i]] = results[i].Results[0];
        }
        else if (echoed)
        {
            Package->DimmPowerFailures = 0;
            Package->DimmPower[entries[i]] = results[i].Results[0];
        }
        else if (entryStatus != STATUS_CANCELLED && ++Package->DimmPowerFailures >= PHP_HW_HSMP_COMMAND_LIMIT)
        {
            Package->DimmPowerDropped = TRUE;
            PhTraceInfo("Hardware sensors: HSMP DIMM power of package %lu unavailable: 0x%lx", PackageIndex, entryStatus);
        }
    }

    if (!Package->DimmProbed && NT_SUCCESS(mailbox))
    {
        Package->DimmProbed = TRUE;
        PhTraceInfo("Hardware sensors: HSMP package %lu answers %lu DIMM sensors", PackageIndex, Package->DimmCount);
    }

    PhpHwHsmpApplyStatus(Package, PackageIndex, mailbox);

CleanupExit:
    PhFree(results);
    PhFree(commands);
}

/**
 * Identifies the packages whose identify met contention, then reads, decodes and publishes each package's batch.
 *
 * \return STATUS_RETRY when every call met contention, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwHsmpPass(
    VOID
    )
{
    BOOLEAN answered = FALSE;
    BOOLEAN retried = FALSE;
    BOOLEAN any = FALSE;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_HSMP_PACKAGE package = &PhpHwHsmpPackages[i];

        package->MemoryClockValid = FALSE;
        memset(&PhpHwHsmpDecode, 0, FIELD_OFFSET(PH_HW_SMU_DECODE, Readings));

        if (!package->Ended && !package->Identified && !PhpHwHsmpProbedThisUpdate)
        {
            PhpHwHsmpIdentify(package, i);

            if (package->Status == STATUS_RETRY)
                retried = TRUE;
            else
                answered = TRUE;
        }

        if (!package->Ended && package->Identified)
        {
            PhpHwHsmpRead(package, i, &PhpHwHsmpDecode);

            if (package->Status == STATUS_RETRY)
                retried = TRUE;
            else
                answered = TRUE;

            if (!package->Ended && (package->Version == 5 || package->Version == 7) &&
                ++package->DimmPasses >= PHP_HW_HSMP_DIMM_INTERVAL)
            {
                package->DimmPasses = 0;
                PhpHwHsmpReadDimms(package, i);
            }

            for (ULONG j = 0; j < package->DimmCount; j++)
            {
                if (package->DimmThermal[j])
                    PhHwSmuDecodeHsmpDimm(PH_HW_HSMP_GET_DIMM_THERMAL, package->DimmThermal[j], &PhpHwHsmpDecode);
                if (package->DimmPower[j])
                    PhHwSmuDecodeHsmpDimm(PH_HW_HSMP_GET_DIMM_POWER, package->DimmPower[j], &PhpHwHsmpDecode);
            }
        }

        PhpHwSmuPublish(&package->Sensors, i, PH_HW_SMU_ID_HSMP, &PhpHwHsmpDecode);

        if (PhpHwHsmpDecode.MemoryClockValid)
        {
            package->MemoryClock = PhpHwHsmpDecode.MemoryClock;
            package->MemoryClockValid = TRUE;
        }

        if (PhpHwHsmpDecode.FrequencyLimitSourcesValid)
        {
            package->LimitSources = PhpHwHsmpDecode.FrequencyLimitSources;
            package->LimitSourcesValid = TRUE;
        }

        any |= !package->Ended && package->Sensors.Count != 0;
    }

    PhpHwHsmpProbedThisUpdate = FALSE;
    PhpHwFamilies[PhpHwFamilyHsmp].Capabilities = any ? PH_HW_CAP_SMU : 0;

    return retried && !answered ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Retrieves the memory clock the last pass decoded, which takes the UMC's place in smn when the SMU has none.
 *
 * \param Package The package.
 * \param MemoryClock Receives MCLK in MHz, half the data rate.
 * \return TRUE when the last pass decoded it.
 */
_Success_(return)
BOOLEAN PhpHwHsmpQueryMemoryClock(
    _In_ ULONG Package,
    _Out_ PFLOAT MemoryClock
    )
{
    if (!PhpHwHsmpPackages || Package >= PhpHwPackageCount || !PhpHwHsmpPackages[Package].MemoryClockValid)
        return FALSE;

    *MemoryClock = PhpHwHsmpPackages[Package].MemoryClock;
    return TRUE;
}

/**
 * Ends the session.
 */
VOID PhpHwHsmpEnd(
    VOID
    )
{
    if (PhpHwHsmpPackages)
    {
        PhFree(PhpHwHsmpPackages);
        PhpHwHsmpPackages = NULL;
    }

    PhpHwHsmpProbedThisUpdate = FALSE;
}

/**
 * Writes the collector's section of the hardware report: each package's mailbox and the commands it accepts.
 *
 * \param Builder The report.
 */
VOID PhpHwHsmpDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    if (!PhpHwHsmpPackages)
    {
        PhAppendStringBuilder2(Builder, L"  Packages: 0 identified\n");
        return;
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_HSMP_PACKAGE package = &PhpHwHsmpPackages[i];
        ULONG accepted = 0;

        for (ULONG j = 0; j < PhHwSmuHsmpCommandCount; j++)
        {
            if (!(package->Dropped & (1UL << j)))
                accepted++;
        }

        PhAppendFormatStringBuilder(
            Builder,
            L"  Package %lu: socket %lu, root 0x%05lx, %s, version %lu, commands %lu of %lu, last 0x%08lx, %lu sensors",
            i,
            i,
            package->Locator,
            package->Ended ? L"ended" : (package->Identified ? L"identified" : L"not identified"),
            package->Version,
            accepted,
            PhHwSmuHsmpCommandCount,
            package->Status,
            package->Sensors.Count
            );

        if (package->LimitSourcesValid)
            PhAppendFormatStringBuilder(Builder, L", limit sources 0x%04lx", package->LimitSources);

        if (package->DimmProbed)
            PhAppendFormatStringBuilder(Builder, L", DIMM sensors %lu%s", package->DimmCount, package->DimmPowerDropped ? L" (no power)" : L"");

        PhAppendStringBuilder2(Builder, L"\n");
    }
}
