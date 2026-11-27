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

// Collector smu: the AMD firmware's power management table through each package's root complex, one
// KphHwSmuReadTable a package a second (the family's interval), decoded by phlib into named sensors.

#define PHP_HW_SMU_STRIKE_LIMIT 2 // consecutive busy or timed-out requests that end the table for the session
#define PHP_HW_SMU_REJECT_LIMIT 10 // consecutive rejected requests that end it; the firmware rejects after a resume

typedef struct _PHP_HW_SMU_PACKAGE
{
    ULONG Locator; // the root complex smn assigned
    BOOLEAN HasTable;
    BOOLEAN Decoded; // the last read decoded
    BOOLEAN MemoryClockValid; // the last pass decoded the DDR MEMCLK
    UCHAR Strikes;
    ULONG FirmwareVersion;
    ULONG Version; // the identify's, then the last read's
    ULONG CoreCount;
    NTSTATUS Status; // the last read's, for the report
    FLOAT MemoryClock;
    PVOID Table; // kept for the session; the last raw table, for the report
    ULONG Capacity;
    ULONG Length; // of the last table read
    PH_HW_SMU_CORE_SLOTS CoreSlots;
    PHP_HW_SMU_SENSORS Sensors;
} PHP_HW_SMU_PACKAGE, *PPHP_HW_SMU_PACKAGE;

static PPHP_HW_SMU_PACKAGE PhpHwSmuPackages = NULL; // per package
static PH_HW_SMU_DECODE PhpHwSmuDecode; // the pass's

/**
 * Finds the named sensor of a reading, adding it the first time; the readings of a source come in the same order,
 * so the reading's position is tried first.
 *
 * \return The index in the named-sensor store, or ULONG_MAX when the store is full.
 */
static ULONG PhpHwSmuSensorIndex(
    _Inout_ PPHP_HW_SMU_SENSORS Sensors,
    _In_ ULONG PackageIndex,
    _In_ ULONG Position,
    _In_ ULONG Id,
    _In_ PCPH_HW_SMU_READING Reading
    )
{
    PH_FORMAT format[3];
    ULONG formatCount = 0;
    WCHAR name[PH_HW_NAMED_SENSOR_NAME_LENGTH];
    PPHP_HW_SMU_SENSOR sensor;

    if (Position < Sensors->Count && Sensors->Sensors[Position].Kind == Reading->Kind && Sensors->Sensors[Position].Id == Id)
        return Sensors->Sensors[Position].Index;

    for (ULONG i = 0; i < Sensors->Count; i++)
    {
        if (Sensors->Sensors[i].Kind == Reading->Kind && Sensors->Sensors[i].Id == Id)
            return Sensors->Sensors[i].Index;
    }

    if (Sensors->Count >= RTL_NUMBER_OF(Sensors->Sensors))
        return ULONG_MAX;

    PhInitFormatS(&format[formatCount++], (PWSTR)Reading->Name);
    if (Reading->Index != PH_HW_SMU_NO_INDEX)
        PhInitFormatU(&format[formatCount++], Reading->Index);
    if (Reading->Suffix)
        PhInitFormatS(&format[formatCount++], (PWSTR)Reading->Suffix);

    if (!PhFormatToBuffer(format, formatCount, name, sizeof(name), NULL))
        name[0] = UNICODE_NULL;

    sensor = &Sensors->Sensors[Sensors->Count++];
    sensor->Kind = Reading->Kind;
    sensor->Id = Id;
    sensor->Index = PhpHwAddNamedSensor(Reading->Kind, PackageIndex, Id, name);

    return sensor->Index;
}

/**
 * Publishes a package's decode of one source (smu or hsmp): every sensor of the source not in it has no value.
 *
 * \param Sensors The source's sensors of the package.
 * \param PackageIndex The package.
 * \param IdFlags Set in the ID of each reading: PH_HW_SMU_ID_HSMP for hsmp, else 0.
 * \param Decode The decode.
 */
VOID PhpHwSmuPublish(
    _Inout_ PPHP_HW_SMU_SENSORS Sensors,
    _In_ ULONG PackageIndex,
    _In_ ULONG IdFlags,
    _In_ PCPH_HW_SMU_DECODE Decode
    )
{
    ULONG indices[PH_HW_SMU_MAX_READINGS];

    // Adding a sensor takes the lock, so every index is found before it is held.
    for (ULONG i = 0; i < Decode->Count; i++)
    {
        const PH_HW_SMU_READING* reading = &Decode->Readings[i];

        indices[i] = PhpHwSmuSensorIndex(Sensors, PackageIndex, i, reading->Id | IdFlags, reading);
    }

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    for (ULONG i = 0; i < Sensors->Count; i++)
    {
        if (Sensors->Sensors[i].Index < PhpHwNamedSensorCount)
            PhpHwNamedSensors[Sensors->Sensors[i].Index].Valid = FALSE;
    }

    for (ULONG i = 0; i < Decode->Count; i++)
    {
        if (indices[i] < PhpHwNamedSensorCount)
        {
            PhpHwNamedSensors[indices[i]].Value = Decode->Readings[i].Value;
            PhpHwNamedSensors[indices[i]].Valid = TRUE;
        }
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Applies the session failure policy to a package's table: contention is no value; a busy, timed-out or rejecting
 * mailbox strikes; anything else ends the table for the session.
 *
 * \return TRUE when the table was read.
 */
static BOOLEAN PhpHwSmuApplyStatus(
    _Inout_ PPHP_HW_SMU_PACKAGE Package,
    _In_ ULONG PackageIndex,
    _In_ NTSTATUS Status
    )
{
    ULONG limit = 0;

    Package->Status = Status;

    if (NT_SUCCESS(Status))
    {
        Package->Strikes = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_RETRY:
    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_NO_MEMORY:
        return FALSE;
    case STATUS_DEVICE_BUSY:
    case STATUS_IO_TIMEOUT:
        limit = PHP_HW_SMU_STRIKE_LIMIT;
        break;
    case STATUS_INTERNAL_ERROR:
        limit = PHP_HW_SMU_REJECT_LIMIT;
        break;
    }

    if (++Package->Strikes < limit)
        return FALSE;

    Package->HasTable = FALSE;
    PhTraceInfo("Hardware sensors: PM table of package %lu unavailable: 0x%lx", PackageIndex, Status);

    return FALSE;
}

/**
 * Identifies each package's firmware and sizes its table buffer. Called once a session.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED without a table on any package, or the status of the first identify.
 */
_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwSmuProbe(
    VOID
    )
{
    NTSTATUS firstStatus = STATUS_NOT_SUPPORTED;
    BOOLEAN any = FALSE;

    if (PhpHwQueryMachine() == IMAGE_FILE_MACHINE_ARM64 || !PhpHwPackageCount)
        return STATUS_NOT_SUPPORTED;

    PhpHwSmuPackages = PhAllocateZero(sizeof(PHP_HW_SMU_PACKAGE) * PhpHwPackageCount);

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SMU_PACKAGE package = &PhpHwSmuPackages[i];
        KPH_HW_SMU_IDENTITY identity;
        NTSTATUS status;

        if (!PhpHwSmnQueryRoot(i, &package->Locator))
            continue;

        status = KphHwSmuIdentify(package->Locator, &identity);

        if (i == 0)
            firstStatus = status;

        if (!NT_SUCCESS(status))
            continue;

        package->FirmwareVersion = identity.FirmwareVersion;
        package->Version = identity.TableVersion;

        // A version or length of 0: the row has no table, or does not list the version.
        if (!identity.TableVersion || !identity.TableLength)
            continue;

        package->Capacity = identity.TableLength;
        package->Table = PhAllocate(package->Capacity);
        package->HasTable = TRUE;

        // The package's distinct cores.
        for (ULONG j = 0; j < PhpHwProcessorCount; j++)
        {
            ULONG k;

            if (PhpHwCpuSensors[j].PackageIndex != i)
                continue;

            for (k = 0; k < j && PhpHwCpuSensors[k].CoreIndex != PhpHwCpuSensors[j].CoreIndex; k++)
                NOTHING;

            if (k == j)
                package->CoreCount++;
        }

        any = TRUE;
    }

    if (!any)
        return NT_SUCCESS(firstStatus) ? STATUS_NOT_SUPPORTED : firstStatus;

    return STATUS_SUCCESS;
}

/**
 * Reads, decodes and publishes each package's table.
 *
 * \return STATUS_RETRY when every read met contention, else STATUS_SUCCESS.
 */
_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwSmuPass(
    VOID
    )
{
    BOOLEAN answered = FALSE;
    BOOLEAN retried = FALSE;
    BOOLEAN any = FALSE;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SMU_PACKAGE package = &PhpHwSmuPackages[i];
        KPH_HW_SMU_TABLE header;
        ULONG length = 0;
        NTSTATUS status;

        package->MemoryClockValid = FALSE;

        if (!package->HasTable)
            continue;

        status = KphHwSmuReadTable(package->Locator, &header, package->Table, package->Capacity, &length);

        // The identify's length was short: grow to the returned length and read again, once.
        if (status == STATUS_BUFFER_TOO_SMALL && length > package->Capacity)
        {
            PhFree(package->Table);
            package->Capacity = length;
            package->Table = PhAllocate(package->Capacity);
            status = KphHwSmuReadTable(package->Locator, &header, package->Table, package->Capacity, &length);
        }

        if (status == STATUS_RETRY)
            retried = TRUE;
        else
            answered = TRUE;

        memset(&PhpHwSmuDecode, 0, FIELD_OFFSET(PH_HW_SMU_DECODE, Readings));

        if (PhpHwSmuApplyStatus(package, i, status))
        {
            package->Version = header.Version;
            package->Length = min(length, package->Capacity);
            package->Decoded = PhHwSmuDecodePmTable(package->Version, package->Table, package->Length, package->CoreCount, &package->CoreSlots, &PhpHwSmuDecode);

            // An unknown version ends the table for the session.
            if (!package->Decoded)
            {
                PhTraceInfo("Hardware sensors: PM table version 0x%lx of package %lu not decoded", package->Version, i);
                package->HasTable = FALSE;
            }
        }

        PhpHwSmuPublish(&package->Sensors, i, 0, &PhpHwSmuDecode);

        if (PhpHwSmuDecode.MemoryClockValid)
        {
            package->MemoryClock = PhpHwSmuDecode.MemoryClock;
            package->MemoryClockValid = TRUE;
        }

        any |= package->HasTable && package->Sensors.Count != 0;
    }

    PhpHwFamilies[PhpHwFamilySmu].Capabilities = any ? PH_HW_CAP_SMU : 0;

    return retried && !answered ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Retrieves the DDR memory clock the last pass decoded, which takes the UMC's place in smn.
 *
 * \param Package The package.
 * \param MemoryClock Receives MEMCLK in MHz, half the data rate.
 * \return TRUE when the last pass decoded it.
 */
_Success_(return)
BOOLEAN PhpHwSmuQueryMemoryClock(
    _In_ ULONG Package,
    _Out_ PFLOAT MemoryClock
    )
{
    if (!PhpHwSmuPackages || Package >= PhpHwPackageCount || !PhpHwSmuPackages[Package].MemoryClockValid)
        return FALSE;

    *MemoryClock = PhpHwSmuPackages[Package].MemoryClock;
    return TRUE;
}

/**
 * Ends the session.
 */
_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwSmuEnd(
    VOID
    )
{
    if (PhpHwSmuPackages)
    {
        for (ULONG i = 0; i < PhpHwPackageCount; i++)
        {
            if (PhpHwSmuPackages[i].Table)
                PhFree(PhpHwSmuPackages[i].Table);
        }

        PhFree(PhpHwSmuPackages);
        PhpHwSmuPackages = NULL;
    }
}

/**
 * Writes the collector's section of the hardware report: each package's firmware, table and its last raw copy.
 *
 * \param Builder The report.
 */
_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwSmuDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    if (!PhpHwSmuPackages)
    {
        PhAppendStringBuilder2(Builder, L"  Packages: 0 identified\n");
        return;
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SMU_PACKAGE package = &PhpHwSmuPackages[i];
        PCWSTR table;

        if (!package->HasTable)
            table = L"not read";
        else if (package->Decoded)
            table = L"decoded";
        else
            table = L"read";

        PhAppendFormatStringBuilder(
            Builder,
            L"  Package %lu: firmware 0x%08lx, table 0x%06lx of %lu bytes, %s, last 0x%08lx, %lu sensors\n",
            i,
            package->FirmwareVersion,
            package->Version,
            package->Length,
            table,
            package->Status,
            package->Sensors.Count
            );

        for (ULONG j = 0; package->Table && j + 4 <= package->Length; j += 4 * 8)
        {
            PhAppendFormatStringBuilder(Builder, L"    %04lx:", j);

            for (ULONG k = j; k < j + 4 * 8 && k + 4 <= package->Length; k += 4)
                PhAppendFormatStringBuilder(Builder, L" %08lx", *(PULONG)PTR_ADD_OFFSET(package->Table, k));

            PhAppendStringBuilder2(Builder, L"\n");
        }
    }
}
