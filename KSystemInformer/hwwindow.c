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

#include <kph.h>
#include <kphdyndata.h>
#include <kphhw.h>

#include <trace.h>

#define KPH_HW_WINDOW_SIGNATURE_LPIT        'TIPL'
#define KPH_HW_WINDOW_LPIT_TYPE_NATIVE      0
#define KPH_HW_WINDOW_LPIT_DISABLED         0x00000001
#define KPH_HW_WINDOW_LPIT_NO_COUNTER       0x00000002
#define KPH_HW_WINDOW_GAS_SYSTEM_MEMORY     0
#define KPH_HW_NFIT_SIGNATURE               'TIFN'
#define KPH_HW_NFIT_HEADER_LENGTH           40
#define KPH_HW_NFIT_TYPE_SPA_RANGE          0
#define KPH_HW_MCFG_SIGNATURE               'GFCM'
#define KPH_HW_MCFG_HEADER_LENGTH           44
#define KPH_HW_ECAM_BUS_SHIFT               20
#define KPH_HW_ECAM_DEVICE_SHIFT            15
#define KPH_HW_ECAM_FUNCTION_SHIFT          12
#define KPH_HW_AMD_IORR_COUNT               2
#define KPH_HW_X86_LEGACY_END               0x100000ull
#define KPH_HW_4GB                          0x100000000ull

#include <pshpack1.h>

typedef struct _KPH_HW_NFIT_SPA_RANGE
{
    USHORT Type;
    USHORT Length;
    USHORT RangeIndex;
    USHORT Flags;
    ULONG Reserved;
    ULONG ProximityDomain;
    GUID AddressRangeType;
    ULONG64 RangeBase;
    ULONG64 RangeLength;
    ULONG64 MappingAttribute;
} KPH_HW_NFIT_SPA_RANGE, *PKPH_HW_NFIT_SPA_RANGE;

typedef struct _KPH_HW_LPIT_NATIVE_ENTRY
{
    ULONG Type;
    ULONG Length;
    USHORT UniqueId;
    USHORT Reserved;
    ULONG Flags;
    KPH_HW_ACPI_GAS EntryTrigger;
    ULONG Residency;
    ULONG Latency;
    KPH_HW_ACPI_GAS ResidencyCounter;
    ULONG64 CounterFrequency;
} KPH_HW_LPIT_NATIVE_ENTRY, *PKPH_HW_LPIT_NATIVE_ENTRY;

typedef struct _KPH_HW_MCFG_ENTRY
{
    ULONG64 BaseAddress;
    USHORT Segment;
    UCHAR StartBus;
    UCHAR EndBus;
    ULONG Reserved;
} KPH_HW_MCFG_ENTRY, *PKPH_HW_MCFG_ENTRY;

#include <poppack.h>

C_ASSERT(sizeof(KPH_HW_MCFG_ENTRY) == 16);
C_ASSERT(FIELD_OFFSET(KPH_HW_LPIT_NATIVE_ENTRY, ResidencyCounter) == 36);
C_ASSERT(sizeof(KPH_HW_LPIT_NATIVE_ENTRY) == 56);
C_ASSERT(FIELD_OFFSET(KPH_HW_NFIT_SPA_RANGE, RangeBase) == 32);
C_ASSERT(sizeof(KPH_HW_NFIT_SPA_RANGE) == 56);

typedef struct _KPH_HW_ACPI_TABLES
{
    PSYSTEM_FIRMWARE_TABLE_INFORMATION Info;
    ULONG Count;
} KPH_HW_ACPI_TABLES, *PKPH_HW_ACPI_TABLES;

typedef struct _KPH_HW_LPIT_COUNTER
{
    ULONG64 Address;
    UCHAR Width;
} KPH_HW_LPIT_COUNTER, *PKPH_HW_LPIT_COUNTER;

typedef struct _KPH_HW_MEMORY_RANGE
{
    ULONG64 Begin;
    ULONG64 End;
} KPH_HW_MEMORY_RANGE, *PKPH_HW_MEMORY_RANGE;

KPH_PROTECTED_DATA_SECTION_PUSH();
static ULONG KphpHwLpitCounterCount = 0;
static KPH_HW_LPIT_COUNTER
KphpHwLpitCounters[KPH_DYN_HW_ACPI_LPIT_MAX_ENTRIES] = { 0 };
static BOOLEAN KphpHwNvdimmRangesKnown = FALSE;
static ULONG KphpHwNvdimmRangeCount = 0;
static PKPH_HW_MEMORY_RANGE KphpHwNvdimmRanges = NULL;
static ULONG KphpHwEcamRangeCount = 0;
static PKPH_HW_MCFG_ENTRY KphpHwEcamRanges = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

KPH_PAGED_FILE();

/**
 * \brief Queries the firmware table provider.
 *
 * \param[in] Action SystemFirmwareTable_Enumerate or SystemFirmwareTable_Get.
 * \param[in] TableId The table signature for SystemFirmwareTable_Get.
 * \param[out] Info Receives the result, freed with KPH_TAG_HW_FIRMWARE.
 *
 * \return Successful or errant status; STATUS_NOT_FOUND when the provider
 * returns nothing.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwQueryAcpiProvider(
    _In_ SYSTEM_FIRMWARE_TABLE_ACTION Action,
    _In_ ULONG TableId,
    _Outptr_ PSYSTEM_FIRMWARE_TABLE_INFORMATION* Info
    )
{
    NTSTATUS status;
    SYSTEM_FIRMWARE_TABLE_INFORMATION query;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;
    ULONG tableLength;
    ULONG headerLength;
    ULONG infoLength;

    KPH_PAGED_CODE_PASSIVE();

    *Info = NULL;
    info = NULL;

    RtlZeroMemory(&query, sizeof(query));
    query.ProviderSignature = FIRMWARE_TABLE_PROVIDER_ACPI;
    query.Action = Action;
    query.TableID = TableId;
    query.TableBufferLength = 0;

    status = ZwQuerySystemInformation(SystemFirmwareTableInformation,
                                      &query,
                                      sizeof(query),
                                      NULL);
    if (NT_SUCCESS(status) && (query.TableBufferLength == 0))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI provider action %lu table 0x%08lx is empty",
                      (ULONG)Action,
                      TableId);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    if ((status != STATUS_BUFFER_TOO_SMALL) || (query.TableBufferLength == 0))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI provider action %lu table 0x%08lx: %!STATUS! "
                      "length %lu",
                      (ULONG)Action,
                      TableId,
                      status,
                      query.TableBufferLength);

        if (NT_SUCCESS(status))
        {
            status = STATUS_UNSUCCESSFUL;
        }

        goto Exit;
    }

    tableLength = query.TableBufferLength;
    headerLength = FIELD_OFFSET(SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer);

    status = RtlULongAdd(headerLength, tableLength, &infoLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    info = KphAllocatePaged(infoLength, KPH_TAG_HW_FIRMWARE);
    if (!info)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate ACPI provider buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    info->ProviderSignature = FIRMWARE_TABLE_PROVIDER_ACPI;
    info->Action = Action;
    info->TableID = TableId;
    info->TableBufferLength = tableLength;

    status = ZwQuerySystemInformation(SystemFirmwareTableInformation,
                                      info,
                                      infoLength,
                                      NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI provider action %lu table 0x%08lx: %!STATUS!",
                      (ULONG)Action,
                      TableId,
                      status);

        goto Exit;
    }

    if (info->TableBufferLength > tableLength)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI provider action %lu table 0x%08lx grew to %lu",
                      (ULONG)Action,
                      TableId,
                      info->TableBufferLength);

        status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    *Info = info;
    info = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (info)
    {
        KphFree(info, KPH_TAG_HW_FIRMWARE);
    }

    return status;
}

/**
 * \brief Retrieves an ACPI table the provider enumerates.
 *
 * \param[in] Tables The enumerated table signatures.
 * \param[in] Signature The table signature.
 * \param[out] Info Receives the table, freed with KPH_TAG_HW_FIRMWARE.
 * \param[out] Length Receives the length of the table.
 *
 * \return Successful or errant status; STATUS_NOT_FOUND when the table is
 * not enumerated.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwQueryAcpiTable(
    _In_ PKPH_HW_ACPI_TABLES Tables,
    _In_ ULONG Signature,
    _Outptr_ PSYSTEM_FIRMWARE_TABLE_INFORMATION* Info,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;
    BOOLEAN present;
    ULONG embeddedLength;

    KPH_PAGED_CODE_PASSIVE();

    *Info = NULL;
    *Length = 0;

    info = NULL;
    present = FALSE;

    for (ULONG i = 0; i < Tables->Count; i++)
    {
        if (((PULONG)Tables->Info->TableBuffer)[i] == Signature)
        {
            present = TRUE;
            break;
        }
    }

    if (!present)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI table 0x%08lx is not enumerated",
                      Signature);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    status = KphpHwQueryAcpiProvider(SystemFirmwareTable_Get, Signature, &info);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwQueryAcpiProvider failed: %!STATUS!",
                      status);

        //
        // An enumerated table that reads back empty is malformed, not absent.
        //
        if (status == STATUS_NOT_FOUND)
        {
            status = STATUS_ACPI_INVALID_TABLE;
        }

        goto Exit;
    }

    if (info->TableBufferLength < KPH_HW_ACPI_HEADER_LENGTH)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI table 0x%08lx length %lu is below the header",
                      Signature,
                      info->TableBufferLength);

        status = STATUS_ACPI_INVALID_TABLE;
        goto Exit;
    }

    embeddedLength = *(PULONG)Add2Ptr(info->TableBuffer, sizeof(ULONG));
    if (embeddedLength != info->TableBufferLength)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI table 0x%08lx length %lu, header says %lu",
                      Signature,
                      info->TableBufferLength,
                      embeddedLength);

        status = STATUS_ACPI_INVALID_TABLE;
        goto Exit;
    }

    *Length = info->TableBufferLength;
    *Info = info;
    info = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (info)
    {
        KphFree(info, KPH_TAG_HW_FIRMWARE);
    }

    return status;
}

/**
 * \brief Caches the native C-state residency counters of the LPIT table.
 *
 * \param[in] Tables The enumerated ACPI table signatures.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwInitializeLpit(
    _In_ PKPH_HW_ACPI_TABLES Tables
    )
{
    NTSTATUS status;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;
    ULONG length;
    ULONG offset;
    ULONG count;
    ULONG headerLength;

    KPH_PAGED_CODE_PASSIVE();

    status = KphpHwQueryAcpiTable(Tables,
                                  KPH_HW_WINDOW_SIGNATURE_LPIT,
                                  &info,
                                  &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwQueryAcpiTable failed: %!STATUS!",
                      status);

        return status;
    }

    count = 0;
    offset = KPH_HW_ACPI_HEADER_LENGTH;
    headerLength = RTL_SIZEOF_THROUGH_FIELD(KPH_HW_LPIT_NATIVE_ENTRY, Length);

    while (offset < length)
    {
        PKPH_HW_LPIT_NATIVE_ENTRY entry;
        ULONG end;
        UCHAR spaceId;
        UCHAR width;

        if ((length - offset) < headerLength)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "LPIT entry at %lu of %lu truncated",
                          offset,
                          length);

            status = STATUS_ACPI_INVALID_TABLE;
            goto Exit;
        }

        entry = Add2Ptr(info->TableBuffer, offset);

        status = RtlULongAdd(offset, entry->Length, &end);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "RtlULongAdd failed: %!STATUS!",
                          status);

            goto Exit;
        }

        if ((entry->Length < headerLength) || (end > length))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "LPIT entry at %lu length %lu malformed, table %lu",
                          offset,
                          entry->Length,
                          length);

            status = STATUS_ACPI_INVALID_TABLE;
            goto Exit;
        }

        offset = end;

        if ((entry->Type != KPH_HW_WINDOW_LPIT_TYPE_NATIVE) ||
            (entry->Length < sizeof(KPH_HW_LPIT_NATIVE_ENTRY)) ||
            BooleanFlagOn(entry->Flags, KPH_HW_WINDOW_LPIT_DISABLED) ||
            BooleanFlagOn(entry->Flags, KPH_HW_WINDOW_LPIT_NO_COUNTER))
        {
            continue;
        }

        if (count >= ARRAYSIZE(KphpHwLpitCounters))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "LPIT counter beyond %lu dropped",
                          count);

            continue;
        }

        spaceId = entry->ResidencyCounter.SpaceId;
        width = entry->ResidencyCounter.BitWidth;

        if ((spaceId != KPH_HW_WINDOW_GAS_SYSTEM_MEMORY) ||
            (entry->ResidencyCounter.BitOffset != 0) ||
            ((width != 32) && (width != 64)))
        {
            continue;
        }

        KphpHwLpitCounters[count].Address = entry->ResidencyCounter.Address;
        KphpHwLpitCounters[count].Width = (width / 8);
        count++;
    }

    KphpHwLpitCounterCount = count;
    status = STATUS_SUCCESS;

Exit:

    KphFree(info, KPH_TAG_HW_FIRMWARE);

    return status;
}

/**
 * \brief Parses the system physical address ranges of the NFIT table.
 *
 * \param[in] Table The NFIT table.
 * \param[in] Length The length of the table.
 * \param[out] Ranges Optionally receives the ranges.
 * \param[in] Capacity The number of entries Ranges can hold.
 * \param[out] Count Receives the number of ranges.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwWindowParseNfit(
    _In_reads_bytes_(Length) PVOID Table,
    _In_ ULONG Length,
    _Out_writes_opt_(Capacity) PKPH_HW_MEMORY_RANGE Ranges,
    _In_ ULONG Capacity,
    _Out_ PULONG Count
    )
{
    NTSTATUS status;
    ULONG count;

    KPH_PAGED_CODE_PASSIVE();

    *Count = 0;

    if (Length < KPH_HW_NFIT_HEADER_LENGTH)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "NFIT length %lu is below the header",
                      Length);

        return STATUS_ACPI_INVALID_TABLE;
    }

    count = 0;

    for (ULONG offset = KPH_HW_NFIT_HEADER_LENGTH; offset < Length; )
    {
        USHORT type;
        USHORT structureLength;
        PKPH_HW_NFIT_SPA_RANGE spa;
        ULONG64 end;

        if ((Length - offset) < (sizeof(USHORT) * 2))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "NFIT structure at %lu of %lu truncated",
                          offset,
                          Length);

            return STATUS_ACPI_INVALID_TABLE;
        }

        type = *(UNALIGNED USHORT*)Add2Ptr(Table, offset);
        structureLength = *(UNALIGNED USHORT*)Add2Ptr(Table, offset + sizeof(USHORT));

        if ((structureLength < (sizeof(USHORT) * 2)) ||
            (structureLength > (Length - offset)))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "NFIT structure at %lu length %lu malformed, "
                          "table %lu",
                          offset,
                          (ULONG)structureLength,
                          Length);

            return STATUS_ACPI_INVALID_TABLE;
        }

        spa = Add2Ptr(Table, offset);
        NT_VERIFY(NT_SUCCESS(RtlULongAdd(offset, structureLength, &offset)));

        if (type != KPH_HW_NFIT_TYPE_SPA_RANGE)
        {
            continue;
        }

        if (structureLength < sizeof(KPH_HW_NFIT_SPA_RANGE))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "NFIT SPA range length %lu truncated",
                          (ULONG)structureLength);

            return STATUS_ACPI_INVALID_TABLE;
        }

        status = RtlULongLongAdd(spa->RangeBase, spa->RangeLength, &end);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "NFIT SPA range 0x%llx length 0x%llx overflows",
                          spa->RangeBase,
                          spa->RangeLength);

            return STATUS_ACPI_INVALID_TABLE;
        }

        if (Ranges && (count >= Capacity))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "NFIT SPA range beyond %lu",
                          Capacity);

            return STATUS_BUFFER_TOO_SMALL;
        }

        if (Ranges)
        {
            Ranges[count].Begin = spa->RangeBase;
            Ranges[count].End = end;
        }

        count++;
    }

    *Count = count;

    return STATUS_SUCCESS;
}

/**
 * \brief Caches the system physical address ranges of the NFIT table.
 *
 * \param[in] Tables The enumerated ACPI table signatures.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphpHwInitializeNfit(
    _In_ PKPH_HW_ACPI_TABLES Tables
    )
{
    NTSTATUS status;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;
    ULONG length;
    ULONG count;
    ULONG rangesLength;
    PKPH_HW_MEMORY_RANGE ranges;

    KPH_PAGED_CODE_PASSIVE();

    ranges = NULL;

    status = KphpHwQueryAcpiTable(Tables,
                                  KPH_HW_NFIT_SIGNATURE,
                                  &info,
                                  &length);
    if (status == STATUS_NOT_FOUND)
    {
        KphpHwNvdimmRangesKnown = TRUE;
        return STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwQueryAcpiTable failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphpHwWindowParseNfit(info->TableBuffer,
                                   length,
                                   NULL,
                                   0,
                                   &count);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwWindowParseNfit failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!count)
    {
        KphpHwNvdimmRangesKnown = TRUE;
        goto Exit;
    }

    status = RtlULongMult(count, sizeof(KPH_HW_MEMORY_RANGE), &rangesLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    ranges = KphAllocatePaged(rangesLength, KPH_TAG_HW_NVDIMM_RANGES);
    if (!ranges)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate NVDIMM ranges");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpHwWindowParseNfit(info->TableBuffer,
                                   length,
                                   ranges,
                                   count,
                                   &count);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwWindowParseNfit failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphpHwNvdimmRanges = ranges;
    KphpHwNvdimmRangeCount = count;
    KphpHwNvdimmRangesKnown = TRUE;
    ranges = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (ranges)
    {
        KphFree(ranges, KPH_TAG_HW_NVDIMM_RANGES);
    }

    KphFree(info, KPH_TAG_HW_FIRMWARE);

    return status;
}

/**
 * \brief Caches the ECAM ranges of the MCFG table.
 *
 * \param[in] Tables The enumerated ACPI table signatures.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphpHwInitializeMcfg(
    _In_ PKPH_HW_ACPI_TABLES Tables
    )
{
    NTSTATUS status;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;
    ULONG length;
    ULONG count;
    ULONG rangesLength;
    PKPH_HW_MCFG_ENTRY entries;
    PKPH_HW_MCFG_ENTRY ranges;

    KPH_PAGED_CODE_PASSIVE();

    status = KphpHwQueryAcpiTable(Tables,
                                  KPH_HW_MCFG_SIGNATURE,
                                  &info,
                                  &length);
    if (status == STATUS_NOT_FOUND)
    {
        return STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwQueryAcpiTable failed: %!STATUS!",
                      status);

        return status;
    }

    if ((length < KPH_HW_MCFG_HEADER_LENGTH) ||
        ((length - KPH_HW_MCFG_HEADER_LENGTH) % sizeof(KPH_HW_MCFG_ENTRY)))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "MCFG length %lu malformed",
                      length);

        status = STATUS_ACPI_INVALID_TABLE;
        goto Exit;
    }

    count = ((length - KPH_HW_MCFG_HEADER_LENGTH) / sizeof(KPH_HW_MCFG_ENTRY));

    if (!count)
    {
        status = STATUS_SUCCESS;
        goto Exit;
    }

    entries = Add2Ptr(info->TableBuffer, KPH_HW_MCFG_HEADER_LENGTH);

    for (ULONG i = 0; i < count; i++)
    {
        ULONG64 end;

        status = RtlULongLongAdd(entries[i].BaseAddress,
                                 (((ULONG64)entries[i].EndBus + 1) <<
                                  KPH_HW_ECAM_BUS_SHIFT),
                                 &end);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "MCFG range 0x%llx end bus %lu overflows",
                          entries[i].BaseAddress,
                          (ULONG)entries[i].EndBus);

            status = STATUS_ACPI_INVALID_TABLE;
            goto Exit;
        }
    }

    NT_VERIFY(NT_SUCCESS(RtlULongMult(count,
                                      sizeof(KPH_HW_MCFG_ENTRY),
                                      &rangesLength)));

    ranges = KphAllocatePaged(rangesLength, KPH_TAG_HW_ECAM_RANGES);
    if (!ranges)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate ECAM ranges");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    RtlCopyMemory(ranges, entries, rangesLength);

    KphpHwEcamRanges = ranges;
    KphpHwEcamRangeCount = count;
    status = STATUS_SUCCESS;

Exit:

    KphFree(info, KPH_TAG_HW_FIRMWARE);

    return status;
}

/**
 * \brief Initializes the window infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwareWindow(
    VOID
    )
{
    NTSTATUS status;
    KPH_HW_ACPI_TABLES tables;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(&tables, sizeof(tables));

    status = KphpHwQueryAcpiProvider(SystemFirmwareTable_Enumerate,
                                     0,
                                     &tables.Info);
    if (NT_SUCCESS(status))
    {
        tables.Count = (tables.Info->TableBufferLength / sizeof(ULONG));

        status = KphpHwInitializeLpit(&tables);
    }

    if (!NT_SUCCESS(status))
    {
        KphpHwLpitCounterCount = 0;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "LPIT residency counters: %lu: %!STATUS!",
                  KphpHwLpitCounterCount,
                  status);

    if (tables.Info)
    {
        status = KphpHwInitializeNfit(&tables);
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "NVDIMM ranges: %lu, known %!bool!: %!STATUS!",
                  KphpHwNvdimmRangeCount,
                  KphpHwNvdimmRangesKnown,
                  status);

    if (tables.Info)
    {
        status = KphpHwInitializeMcfg(&tables);
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "ECAM ranges: %lu: %!STATUS!",
                  KphpHwEcamRangeCount,
                  status);

    if (tables.Info)
    {
        KphFree(tables.Info, KPH_TAG_HW_FIRMWARE);
    }
}

/**
 * \brief Cleans up the window infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardwareWindow(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (KphpHwNvdimmRanges)
    {
        KphFree(KphpHwNvdimmRanges, KPH_TAG_HW_NVDIMM_RANGES);
    }

    if (KphpHwEcamRanges)
    {
        KphFree(KphpHwEcamRanges, KPH_TAG_HW_ECAM_RANGES);
    }
}

/**
 * \brief Checks that a physical range is not OS memory.
 *
 * \param[in] Begin The start of the range.
 * \param[in] End The end of the range, exclusive.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwWindowCheckMemory(
    _In_ ULONG64 Begin,
    _In_ ULONG64 End
    )
{
    NTSTATUS status;
    PPHYSICAL_MEMORY_RANGE ranges;

    KPH_PAGED_CODE_PASSIVE();

    ranges = MmGetPhysicalMemoryRanges();

    if (!ranges)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Physical memory ranges unavailable");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (PPHYSICAL_MEMORY_RANGE range = ranges;
         range->BaseAddress.QuadPart || range->NumberOfBytes.QuadPart;
         range++)
    {
        ULONG64 begin;
        ULONG64 length;
        ULONG64 end;

        begin = (ULONG64)range->BaseAddress.QuadPart;
        length = (ULONG64)range->NumberOfBytes.QuadPart;

        status = RtlULongLongAdd(begin, length, &end);
        if (!NT_SUCCESS(status))
        {
            end = MAXULONG64;
        }

        if ((Begin < end) && (begin < End))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Range 0x%llx-0x%llx overlaps memory "
                          "0x%llx-0x%llx",
                          Begin,
                          End,
                          begin,
                          end);

            status = STATUS_CONFLICTING_ADDRESSES;
            goto Exit;
        }
    }

    status = STATUS_SUCCESS;

Exit:

    KphFreePool(ranges);

    return status;
}

/**
 * \brief Checks that a physical range is not NVDIMM memory.
 *
 * \param[in] Begin The start of the range.
 * \param[in] End The end of the range, exclusive.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwWindowCheckNvdimm(
    _In_ ULONG64 Begin,
    _In_ ULONG64 End
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (!KphpHwNvdimmRangesKnown)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "NVDIMM ranges unknown");

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    for (ULONG i = 0; i < KphpHwNvdimmRangeCount; i++)
    {
        PKPH_HW_MEMORY_RANGE range;

        range = &KphpHwNvdimmRanges[i];

        if ((Begin < range->End) && (range->Begin < End))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Range 0x%llx-0x%llx overlaps NVDIMM "
                          "0x%llx-0x%llx",
                          Begin,
                          End,
                          range->Begin,
                          range->End);

            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Checks that a physical range is safe to map.
 *
 * \param[in] Begin The page-aligned start of the range.
 * \param[in] End The page-aligned end of the range, exclusive.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwWindowCheckRange(
    _In_ ULONG64 Begin,
    _In_ ULONG64 End
    )
{
    NTSTATUS status;
    PCKPH_HW_CPU_IDENTITY cpu;

    KPH_PAGED_CODE_PASSIVE();

    cpu = KphGetHardwareCpuIdentity();

    NT_ASSERT((cpu->PhysicalAddressBits > 0) &&
              (cpu->PhysicalAddressBits < 64));

    if (End > (1ull << cpu->PhysicalAddressBits))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Range end 0x%llx beyond %lu address bits",
                      End,
                      (ULONG)cpu->PhysicalAddressBits);

        return STATUS_CONFLICTING_ADDRESSES;
    }

    status = KphpHwWindowCheckNvdimm(Begin, End);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwWindowCheckNvdimm failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphpHwWindowCheckMemory(Begin, End);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwWindowCheckMemory failed: %!STATUS!",
                      status);

        return status;
    }

    return STATUS_SUCCESS;
}

#if defined(_M_X64)

/**
 * \brief Reads an MSR on the current processor.
 *
 * \param[in] Msr The MSR.
 * \param[out] Value Receives the value, zero when the read faults.
 *
 * \return TRUE if the read did not fault, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwWindowReadMsr(
    _In_ ULONG Msr,
    _Out_ PULONG64 Value
    )
{
    KPH_PAGED_CODE_PASSIVE();

    *Value = 0;

    __try
    {
        *Value = __readmsr(Msr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "MSR 0x%08lx read faulted",
                      Msr);

        *Value = 0;
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief Computes the region an AMD base/mask register pair decodes.
 *
 * \param[in] Base The base register.
 * \param[in] Mask The mask register.
 * \param[in] LowBits The bits below the pair's granularity.
 * \param[in] Limit The mask of the physical address bits.
 * \param[out] Begin Receives the start of the region.
 * \param[out] End Receives the end of the region, exclusive.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwWindowAmdRegion(
    _In_ ULONG64 Base,
    _In_ ULONG64 Mask,
    _In_ ULONG64 LowBits,
    _In_ ULONG64 Limit,
    _Out_ PULONG64 Begin,
    _Out_ PULONG64 End
    )
{
    ULONG64 mask;

    KPH_PAGED_CODE_PASSIVE();

    mask = (Mask & Limit & ~LowBits);

    *Begin = (Base & mask);
    *End = (*Begin + ((~mask & Limit) + 1));
}

/**
 * \brief Checks that a physical range is DRAM below TOP_MEM or TOP_MEM2.
 *
 * \param[in] Begin The page-aligned start of the range.
 * \param[in] End The page-aligned end of the range, exclusive.
 * \param[in] Limit The mask of the physical address bits.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwWindowCheckAmdTopMem(
    _In_ ULONG64 Begin,
    _In_ ULONG64 End,
    _In_ ULONG64 Limit
    )
{
    ULONG64 syscfg;
    ULONG64 topMem;
    ULONG64 topMem2;
    BOOLEAN inDram;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphpHwWindowReadMsr(MSR_AMD_SYSCFG, &syscfg))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "DRAM bound of 0x%llx-0x%llx skipped: SYSCFG hidden",
                      Begin,
                      End);

        return STATUS_SUCCESS;
    }

    if (!BooleanFlagOn(syscfg, MSR_AMD_SYSCFG_VAR_DRAM_EN))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Range 0x%llx-0x%llx refused: SYSCFG 0x%llx has no "
                      "TOP_MEM",
                      Begin,
                      End,
                      syscfg);

        return STATUS_CONFLICTING_ADDRESSES;
    }

    if (!KphpHwWindowReadMsr(MSR_AMD_TOP_MEM, &topMem))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "DRAM bound of 0x%llx-0x%llx skipped: TOP_MEM hidden",
                      Begin,
                      End);

        return STATUS_SUCCESS;
    }

    topMem &= (Limit & ~MSR_AMD_TOP_MEM_LOW_BITS);

    if (!topMem)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Range 0x%llx-0x%llx refused: TOP_MEM is zero",
                      Begin,
                      End);

        return STATUS_CONFLICTING_ADDRESSES;
    }

    inDram = ((Begin >= KPH_HW_X86_LEGACY_END) && (End <= topMem));

    if (!inDram && BooleanFlagOn(syscfg, MSR_AMD_SYSCFG_TOM2_EN))
    {
        if (!KphpHwWindowReadMsr(MSR_AMD_TOP_MEM2, &topMem2))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "DRAM bound of 0x%llx-0x%llx skipped: TOP_MEM2 "
                          "hidden",
                          Begin,
                          End);

            return STATUS_SUCCESS;
        }

        topMem2 &= (Limit & ~MSR_AMD_TOP_MEM_LOW_BITS);

        inDram = ((Begin >= KPH_HW_4GB) && (End <= topMem2));
    }

    if (!inDram)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Range 0x%llx-0x%llx refused: not below TOP_MEM 0x%llx "
                      "or TOP_MEM2",
                      Begin,
                      End,
                      topMem);

        return STATUS_CONFLICTING_ADDRESSES;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Checks that a physical range does not overlap TSEG.
 *
 * \param[in] Begin The page-aligned start of the range.
 * \param[in] End The page-aligned end of the range, exclusive.
 * \param[in] Limit The mask of the physical address bits.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwWindowCheckAmdTseg(
    _In_ ULONG64 Begin,
    _In_ ULONG64 End,
    _In_ ULONG64 Limit
    )
{
    ULONG64 base;
    ULONG64 mask;
    ULONG64 regionBegin;
    ULONG64 regionEnd;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphpHwWindowReadMsr(MSR_AMD_SMM_ADDR, &base) ||
        !KphpHwWindowReadMsr(MSR_AMD_SMM_MASK, &mask))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "TSEG check of 0x%llx-0x%llx skipped: TSEG hidden",
                      Begin,
                      End);

        return STATUS_SUCCESS;
    }

    KphpHwWindowAmdRegion(base,
                          mask,
                          MSR_AMD_SMM_LOW_BITS,
                          Limit,
                          &regionBegin,
                          &regionEnd);

    if (BooleanFlagOn(mask, MSR_AMD_SMM_MASK_TVALID) &&
        (Begin < regionEnd) &&
        (regionBegin < End))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Range 0x%llx-0x%llx refused: TSEG 0x%llx-0x%llx",
                      Begin,
                      End,
                      regionBegin,
                      regionEnd);

        return STATUS_CONFLICTING_ADDRESSES;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Checks that a physical range is not routed to I/O by an IORR.
 *
 * \param[in] Begin The page-aligned start of the range.
 * \param[in] End The page-aligned end of the range, exclusive.
 * \param[in] Limit The mask of the physical address bits.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwWindowCheckAmdIorr(
    _In_ ULONG64 Begin,
    _In_ ULONG64 End,
    _In_ ULONG64 Limit
    )
{
    ULONG64 base;
    ULONG64 mask;
    ULONG64 regionBegin;
    ULONG64 regionEnd;

    KPH_PAGED_CODE_PASSIVE();

    for (ULONG i = 0; i < KPH_HW_AMD_IORR_COUNT; i++)
    {
        if (!KphpHwWindowReadMsr(MSR_AMD_IORR_BASE0 + (i * 2), &base) ||
            !KphpHwWindowReadMsr(MSR_AMD_IORR_BASE0 + (i * 2) + 1,
                                 &mask))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "IORR %lu check of 0x%llx-0x%llx skipped: IORR "
                          "hidden",
                          i,
                          Begin,
                          End);

            continue;
        }

        if (!BooleanFlagOn(mask, MSR_AMD_IORR_MASK_VALID) ||
            BooleanFlagOn(base, MSR_AMD_IORR_BASE_RDMEM))
        {
            continue;
        }

        KphpHwWindowAmdRegion(base,
                              mask,
                              MSR_AMD_IORR_LOW_BITS,
                              Limit,
                              &regionBegin,
                              &regionEnd);

        if ((Begin < regionEnd) && (regionBegin < End))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Range 0x%llx-0x%llx refused: IORR %lu reads I/O "
                          "at 0x%llx-0x%llx",
                          Begin,
                          End,
                          i,
                          regionBegin,
                          regionEnd);

            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Checks that a physical range is DRAM as this AMD processor decodes
 * it and is not routed to SMM or to I/O.
 *
 * \details The registers are the AMD64 architectural address map. A register
 * a hypervisor hides faults; the check that needs it is skipped and the
 * range is left to KphHwWindowCheckRange.
 *
 * \param[in] Begin The page-aligned start of the range.
 * \param[in] End The page-aligned end of the range, exclusive.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwWindowCheckAmdDram(
    _In_ ULONG64 Begin,
    _In_ ULONG64 End
    )
{
    NTSTATUS status;
    PCKPH_HW_CPU_IDENTITY cpu;
    ULONG64 limit;

    KPH_PAGED_CODE_PASSIVE();

    cpu = KphGetHardwareCpuIdentity();

    NT_ASSERT((cpu->PhysicalAddressBits > 0) &&
              (cpu->PhysicalAddressBits < 64));
    NT_ASSERT(Begin < End);

    limit = ((1ull << cpu->PhysicalAddressBits) - 1);

    status = KphpHwWindowCheckAmdTopMem(Begin, End, limit);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwWindowCheckAmdTopMem failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphpHwWindowCheckAmdTseg(Begin, End, limit);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwWindowCheckAmdTseg failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphpHwWindowCheckAmdIorr(Begin, End, limit);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwWindowCheckAmdIorr failed: %!STATUS!",
                      status);

        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Finds the ECAM page of a PCI function in the cached MCFG ranges.
 *
 * \param[in] Segment The segment.
 * \param[in] Bus The bus.
 * \param[in] Device The device.
 * \param[in] Function The function.
 * \param[out] Address Receives the physical address of the page, zero when
 * no range decodes the bus.
 *
 * \return TRUE if a range decodes the bus, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwWindowEcamPage(
    _In_ USHORT Segment,
    _In_ UCHAR Bus,
    _In_ UCHAR Device,
    _In_ UCHAR Function,
    _Out_ PULONG64 Address
    )
{
    KPH_PAGED_CODE_PASSIVE();

    *Address = 0;

    for (ULONG i = 0; i < KphpHwEcamRangeCount; i++)
    {
        PKPH_HW_MCFG_ENTRY range;

        range = &KphpHwEcamRanges[i];

        if ((range->Segment != Segment) ||
            (Bus < range->StartBus) ||
            (Bus > range->EndBus))
        {
            continue;
        }

        *Address = (range->BaseAddress +
                    ((ULONG64)Bus << KPH_HW_ECAM_BUS_SHIFT) +
                    ((ULONG64)Device << KPH_HW_ECAM_DEVICE_SHIFT) +
                    ((ULONG64)Function << KPH_HW_ECAM_FUNCTION_SHIFT));

        return TRUE;
    }

    return FALSE;
}

/**
 * \brief Checks that a physical range is DRAM as the AMD Data Fabric decodes
 * it.
 *
 * \details The DRAM address map is read through ECAM, which a hypervisor
 * does not hide. Without an MCFG range for the fabric the check is skipped
 * and the range is left to KphHwWindowCheckRange.
 *
 * \param[in] Op The df_map op.
 * \param[in] Locator The locator of the root complex.
 * \param[in] Begin The page-aligned start of the range.
 * \param[in] End The page-aligned end of the range, exclusive.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwWindowCheckAmdDf(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG64 Begin,
    _In_ ULONG64 End
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_DF_MAP map;
    ULONG64 page;
    PHYSICAL_ADDRESS physical;
    PUCHAR mapping;
    ULONG hole;
    BOOLEAN holeValid;
    ULONG64 holeBase;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_DF_MAP);
    NT_ASSERT(Begin < End);

    map = (PKPH_DYN_HW_DF_MAP)Op->Body;
    mapping = NULL;

    if (!KphpHwWindowEcamPage(Locator->Segment,
                              Locator->Bus,
                              map->Device,
                              map->Function,
                              &page))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Data fabric bound of 0x%llx-0x%llx skipped: no ECAM "
                      "range for %04lx:%02lx",
                      Begin,
                      End,
                      (ULONG)Locator->Segment,
                      (ULONG)Locator->Bus);

        return STATUS_SUCCESS;
    }

    status = KphHwWindowCheckRange(page, page + PAGE_SIZE);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ECAM page 0x%llx refused: %!STATUS!",
                      page,
                      status);

        goto Exit;
    }

    physical.QuadPart = (LONGLONG)page;

    mapping = MmMapIoSpaceEx(physical,
                             PAGE_SIZE,
                             PAGE_READONLY | PAGE_NOCACHE);
    if (!mapping)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "MmMapIoSpaceEx failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (READ_REGISTER_ULONG((PULONG)mapping) == MAXULONG)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Data fabric at ECAM page 0x%llx reads all ones",
                      page);

        status = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }

    holeValid = FALSE;
    holeBase = 0;

    if (map->HoleOffset)
    {
        hole = READ_REGISTER_ULONG((PULONG)Add2Ptr(mapping, map->HoleOffset));

        holeValid = BooleanFlagOn(hole, (1ul << map->HoleValidBit));
        holeBase = (hole & map->HoleBaseMask);
    }

    for (ULONG i = 0; i < map->MapCount; i++)
    {
        ULONG offset;
        ULONG baseRegister;
        ULONG limitRegister;
        ULONG control;
        ULONG64 mapBegin;
        ULONG64 mapEnd;

        offset = (map->MapOffset + (i * map->MapStride));

        control = READ_REGISTER_ULONG(
            (PULONG)Add2Ptr(mapping, (SIZE_T)offset + map->ControlOffset));

        if (!BooleanFlagOn(control, (1ul << map->ValidBit)))
        {
            continue;
        }

        baseRegister = READ_REGISTER_ULONG((PULONG)Add2Ptr(mapping, offset));
        limitRegister = READ_REGISTER_ULONG(
            (PULONG)Add2Ptr(mapping, (SIZE_T)offset + map->LimitOffset));

        mapBegin = ((ULONG64)(baseRegister & map->AddressMask) <<
                    map->AddressShift);
        mapEnd = (((ULONG64)(limitRegister & map->AddressMask) <<
                   map->AddressShift) + (1ull << map->AddressShift));

        if ((Begin < mapBegin) || (End > mapEnd))
        {
            continue;
        }

        if (Begin < KPH_HW_X86_LEGACY_END)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Range 0x%llx-0x%llx refused: in the legacy region",
                          Begin,
                          End);

            status = STATUS_CONFLICTING_ADDRESSES;
            goto Exit;
        }

        if (holeValid &&
            BooleanFlagOn(control, (1ul << map->HoleRemapBit)) &&
            (Begin < KPH_HW_4GB) &&
            (holeBase < End))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Range 0x%llx-0x%llx refused: DRAM hole at 0x%llx",
                          Begin,
                          End,
                          holeBase);

            status = STATUS_CONFLICTING_ADDRESSES;
            goto Exit;
        }

        status = STATUS_SUCCESS;
        goto Exit;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "Range 0x%llx-0x%llx refused: no data fabric DRAM map",
                  Begin,
                  End);

    status = STATUS_CONFLICTING_ADDRESSES;

Exit:

    if (mapping)
    {
        MmUnmapIoSpace(mapping, PAGE_SIZE);
    }

    return status;
}

#endif

/**
 * \brief Retrieves an LPIT residency counter that has an address.
 *
 * \param[in] Entry The ordinal of the counter.
 * \param[out] Counter Receives the counter.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwWindowLpitCounter(
    _In_ UCHAR Entry,
    _Outptr_ PKPH_HW_LPIT_COUNTER* Counter
    )
{
    PKPH_HW_LPIT_COUNTER counter;

    KPH_PAGED_CODE_PASSIVE();

    *Counter = NULL;

    if (Entry >= KphpHwLpitCounterCount)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "LPIT counter %lu of %lu not found",
                      (ULONG)Entry,
                      KphpHwLpitCounterCount);

        return STATUS_NOT_FOUND;
    }

    counter = &KphpHwLpitCounters[Entry];

    if (!counter->Address)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "LPIT counter %lu has no address",
                      (ULONG)Entry);

        return STATUS_DEVICE_NOT_READY;
    }

    *Counter = counter;

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves the physical base of an LPIT residency counter.
 *
 * \param[in] Entry The ordinal of the counter.
 * \param[in] Width The width of the window, in bytes.
 * \param[out] Base Receives the base.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwWindowLpitBase(
    _In_ UCHAR Entry,
    _In_ ULONG Width,
    _Out_ PULONG64 Base
    )
{
    NTSTATUS status;
    PKPH_HW_LPIT_COUNTER counter;

    KPH_PAGED_CODE_PASSIVE();

    *Base = 0;

    status = KphpHwWindowLpitCounter(Entry, &counter);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if ((counter->Width != Width) || !IS_ALIGNED(counter->Address, Width))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "LPIT counter %lu at 0x%llx width %lu, window %lu",
                      (ULONG)Entry,
                      counter->Address,
                      (ULONG)counter->Width,
                      Width);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    *Base = counter->Address;

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves the physical base of a block that holds an LPIT residency
 * counter at a known offset.
 *
 * \param[in] Entry The ordinal of the counter.
 * \param[in] Offset The offset of the counter in the block.
 * \param[in] Size The size of the block, a power of two; the base is aligned
 * to it.
 * \param[out] Base Receives the base.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwWindowLpitRelativeBase(
    _In_ UCHAR Entry,
    _In_ ULONG64 Offset,
    _In_ ULONG Size,
    _Out_ PULONG64 Base
    )
{
    NTSTATUS status;
    PKPH_HW_LPIT_COUNTER counter;
    ULONG64 base;

    KPH_PAGED_CODE_PASSIVE();

    *Base = 0;

    status = KphpHwWindowLpitCounter(Entry, &counter);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (!NT_SUCCESS(RtlULongLongSub(counter->Address, Offset, &base)) ||
        !IS_ALIGNED(base, Size))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "LPIT counter %lu at 0x%llx, block offset 0x%llx "
                      "size %lu",
                      (ULONG)Entry,
                      counter->Address,
                      Offset,
                      Size);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    *Base = base;

    return STATUS_SUCCESS;
}

/**
 * \brief Reads one register of a mapped window.
 *
 * \param[in] Address The mapped address of the register.
 * \param[in] Width The access width, 1, 2, 4 or 8.
 *
 * \return The register value.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG64 KphpHwWindowReadRegister(
    _In_ PVOID Address,
    _In_ UCHAR Width
    )
{
    KPH_PAGED_CODE_PASSIVE();

    switch (Width)
    {
        case 1:
        {
            return READ_REGISTER_UCHAR((PUCHAR)Address);
        }
        case 2:
        {
            return READ_REGISTER_USHORT((PUSHORT)Address);
        }
        case 4:
        {
            return READ_REGISTER_ULONG((PULONG)Address);
        }
        default:
        {
            NT_ASSERT(Width == 8);
            return READ_REGISTER_ULONG64((PULONG64)Address);
        }
    }
}

/**
 * \brief Validates batch entries against a window_read op.
 *
 * \details Entries to be read receive STATUS_PENDING.
 *
 * \param[in] Op The window_read op.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 * \param[out] SpanBegin Receives the lowest window offset to read.
 * \param[out] SpanEnd Receives the end of the highest read, exclusive.
 *
 * \return TRUE if any entry is to be read, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwWindowValidateEntries(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _Out_ PULONG SpanBegin,
    _Out_ PULONG SpanEnd
    )
{
    PKPH_DYN_HW_WINDOW_READ window;
    BOOLEAN anyAllowed;
    ULONG spanBegin;
    ULONG spanEnd;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_WINDOW_READ);

    window = (PKPH_DYN_HW_WINDOW_READ)Op->Body;

    anyAllowed = FALSE;
    spanBegin = MAXULONG;
    spanEnd = 0;

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_WINDOW_BATCH_ENTRY entry;

        entry = &Entries[i];

        entry->Value = 0;

        if (((entry->Width != 1) && (entry->Width != 2) &&
             (entry->Width != 4) && (entry->Width != 8)) ||
            ((entry->Offset % entry->Width) != 0))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Window offset 0x%lx width %lu is invalid",
                          entry->Offset,
                          (ULONG)entry->Width);

            entry->Status = STATUS_INVALID_PARAMETER;
            continue;
        }

        if (!KphDynHwWindowReadAllowed(Op, entry->Offset, entry->Width))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Window offset 0x%lx width %lu refused",
                          entry->Offset,
                          (ULONG)entry->Width);

            entry->Status = STATUS_ACCESS_DENIED;
            continue;
        }

        NT_ASSERT((entry->Offset + entry->Width) <= window->WindowSize);

        entry->Status = STATUS_PENDING;
        anyAllowed = TRUE;
        spanBegin = min(spanBegin, entry->Offset);
        spanEnd = max(spanEnd, entry->Offset + entry->Width);
    }

    *SpanBegin = spanBegin;
    *SpanEnd = spanEnd;

    return anyAllowed;
}

/**
 * \brief Retrieves the all-ones value of an access width.
 *
 * \param[in] Width The access width, 1, 2, 4 or 8.
 *
 * \return The value with every bit of the width set.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG64 KphHwWindowAllOnes(
    _In_ UCHAR Width
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT((Width == 1) || (Width == 2) || (Width == 4) || (Width == 8));

    if (Width == sizeof(ULONG64))
    {
        return MAXULONG64;
    }

    return ((1ull << (Width * RTL_BITS_OF(UCHAR))) - 1);
}

/**
 * \brief Stops a batch: every entry still pending is not read.
 *
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphHwWindowStopEntries(
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    )
{
    KPH_PAGED_CODE_PASSIVE();

    for (ULONG i = 0; i < Count; i++)
    {
        if (Entries[i].Status == STATUS_PENDING)
        {
            Entries[i].Status = STATUS_DEVICE_NOT_READY;
        }
    }
}

/**
 * \brief Maps the span of a window the prepared entries cover and reads them.
 *
 * \param[in] Op The window_read op.
 * \param[in] Base The physical base of the window.
 * \param[in] SpanBegin The lowest window offset to read.
 * \param[in] SpanEnd The end of the highest read, exclusive.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The prepared batch entries.
 * \param[in] Recheck Optionally re-checks the device after an all-ones read;
 * on failure the batch stops.
 * \param[in] Context The context of Recheck.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwWindowMapAndRead(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG64 Base,
    _In_ ULONG SpanBegin,
    _In_ ULONG SpanEnd,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _In_opt_ PKPH_HW_WINDOW_RECHECK Recheck,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_WINDOW_READ window;
    ULONG64 windowEnd;
    ULONG64 checkBegin;
    ULONG64 checkEnd;
    ULONG64 spanBase;
    ULONG spanLength;
    PHYSICAL_ADDRESS physical;
    PVOID mapping;

    KPH_PAGED_CODE_PASSIVE();

    window = (PKPH_DYN_HW_WINDOW_READ)Op->Body;

    NT_ASSERT((SpanBegin < SpanEnd) &&
              (SpanEnd <= window->WindowSize));

    status = RtlULongLongAdd(Base, window->WindowSize, &windowEnd);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongLongAdd failed: %!STATUS!",
                      status);

        return status;
    }

    if (windowEnd > (MAXULONG64 - PAGE_SIZE))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Window end 0x%llx cannot be page rounded",
                      windowEnd);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    checkBegin = (ULONG64)PAGE_ALIGN(Base);
    checkEnd = ROUND_TO_PAGES(windowEnd);

    status = KphHwWindowCheckRange(checkBegin, checkEnd);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwWindowCheckRange failed: %!STATUS!",
                      status);

        return status;
    }

    NT_VERIFY(NT_SUCCESS(RtlULongLongAdd(Base, SpanBegin, &spanBase)));
    spanLength = (SpanEnd - SpanBegin);

    physical.QuadPart = (LONGLONG)spanBase;

    mapping = MmMapIoSpaceEx(physical,
                             spanLength,
                             PAGE_READONLY | PAGE_NOCACHE);
    if (!mapping)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "MmMapIoSpaceEx failed");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_WINDOW_BATCH_ENTRY entry;
        PVOID registerAddress;

        entry = &Entries[i];

        if (entry->Status != STATUS_PENDING)
        {
            continue;
        }

        registerAddress = Add2Ptr(mapping, entry->Offset - SpanBegin);
        entry->Value = KphpHwWindowReadRegister(registerAddress, entry->Width);
        entry->Status = STATUS_SUCCESS;

        if (!Recheck || (entry->Value != KphHwWindowAllOnes(entry->Width)))
        {
            continue;
        }

        status = Recheck(Context);
        if (NT_SUCCESS(status))
        {
            continue;
        }

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Window offset 0x%lx read all-ones, device re-check "
                      "failed: %!STATUS!",
                      entry->Offset,
                      status);

        entry->Value = 0;
        entry->Status = STATUS_DEVICE_NOT_READY;

        KphHwWindowStopEntries(Count, Entries);
        break;
    }

    MmUnmapIoSpace(mapping, spanLength);

    return STATUS_SUCCESS;
}

/**
 * \brief Reads a batch of allow-listed registers of a device window.
 *
 * \param[in] Locator The locator of the device.
 * \param[in] OpIndex The index of the window_read op of the device.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadWindowBatch(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    ULONG entriesLength;
    PKPH_HW_WINDOW_BATCH_ENTRY entries;
    PKPH_DYN dyn;

    KPH_PAGED_CODE_PASSIVE();

    entries = NULL;
    dyn = NULL;

    locator = *Locator;

    if (Count == 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Window batch is empty");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->Hardware.Section)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = RtlULongMult(Count,
                          sizeof(KPH_HW_WINDOW_BATCH_ENTRY),
                          &entriesLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    entries = KphAllocatePaged(entriesLength, KPH_TAG_HW_WINDOW_BATCH);
    if (!entries)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate window batch buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphCopyFromMode(entries, Entries, entriesLength, AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = KphHwPciReadWindow(&locator,
                                dyn->Hardware.Section,
                                OpIndex,
                                Count,
                                entries);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwPciReadWindow failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphCopyToMode(Entries, entries, entriesLength, AccessMode);

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (entries)
    {
        KphFree(entries, KPH_TAG_HW_WINDOW_BATCH);
    }

    return status;
}

/**
 * \brief Reads a batch of an ACPI_LPIT or ACPI_LPIT_RELATIVE window of this
 * CPU's row.
 *
 * \param[in] Op The window_read op, base kind ACPI_LPIT or ACPI_LPIT_RELATIVE.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwWindowReadLpit(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_WINDOW_READ window;
    ULONG spanBegin;
    ULONG spanEnd;
    ULONG64 base;

    KPH_PAGED_CODE_PASSIVE();

    window = (PKPH_DYN_HW_WINDOW_READ)Op->Body;

    NT_ASSERT((window->BaseKind == KPH_DYN_HW_WINDOW_BASE_ACPI_LPIT) ||
              (window->BaseKind == KPH_DYN_HW_WINDOW_BASE_ACPI_LPIT_RELATIVE));

    if (!KphHwWindowValidateEntries(Op, Count, Entries, &spanBegin, &spanEnd))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No window entry is allowed");

        return STATUS_INVALID_PARAMETER;
    }

    if (window->BaseKind == KPH_DYN_HW_WINDOW_BASE_ACPI_LPIT)
    {
        status = KphHwWindowLpitBase(window->BarIndex,
                                     window->WindowSize,
                                     &base);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphHwWindowLpitBase failed: %!STATUS!",
                          status);

            return status;
        }
    }
    else
    {
        status = KphHwWindowLpitRelativeBase(window->BarIndex,
                                             window->FixedBase,
                                             window->WindowSize,
                                             &base);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphHwWindowLpitRelativeBase failed: %!STATUS!",
                          status);

            return status;
        }
    }

    status = KphHwWindowMapAndRead(Op,
                                   base,
                                   spanBegin,
                                   spanEnd,
                                   Count,
                                   Entries,
                                   NULL,
                                   NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwWindowMapAndRead failed: %!STATUS!",
                      status);

        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Reads a batch of a FIXED_MMIO window.
 *
 * \param[in] Op The window_read op, base kind FIXED_MMIO.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwWindowReadFixedMmio(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_WINDOW_READ window;
    ULONG spanBegin;
    ULONG spanEnd;

    KPH_PAGED_CODE_PASSIVE();

    window = (PKPH_DYN_HW_WINDOW_READ)Op->Body;

    NT_ASSERT(window->BaseKind == KPH_DYN_HW_WINDOW_BASE_FIXED_MMIO);

    if (window->AuxCount)
    {
        ULONG device;

        device = KphDynHwWindowAux(Op, 0);

        status = KphHwPciIdPresent((USHORT)device, (USHORT)(device >> 16));
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphHwPciIdPresent failed: %!STATUS!",
                          status);

            return status;
        }
    }

    if (!KphHwWindowValidateEntries(Op, Count, Entries, &spanBegin, &spanEnd))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No window entry is allowed");

        return STATUS_INVALID_PARAMETER;
    }

    status = KphHwWindowMapAndRead(Op,
                                   window->FixedBase,
                                   spanBegin,
                                   spanEnd,
                                   Count,
                                   Entries,
                                   NULL,
                                   NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwWindowMapAndRead failed: %!STATUS!",
                      status);

        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Reads a batch of allow-listed registers of a CPU or board row window.
 *
 * \param[in] Source The row, KPH_HW_PLATFORM_CPU or KPH_HW_PLATFORM_BOARD.
 * \param[in] OpIndex The index of the window_read op of the row.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadPlatformWindowBatch(
    _In_ UCHAR Source,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    ULONG entriesLength;
    PKPH_HW_WINDOW_BATCH_ENTRY entries;
    PKPH_DYN dyn;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP op;
    PKPH_DYN_HW_WINDOW_READ window;

    KPH_PAGED_CODE_PASSIVE();

    entries = NULL;
    dyn = NULL;

    if (Count == 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Platform window batch is empty");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((Source != KPH_HW_PLATFORM_CPU) && (Source != KPH_HW_PLATFORM_BOARD))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Platform window source %lu invalid",
                      (ULONG)Source);

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->Hardware.Section)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = RtlULongMult(Count,
                          sizeof(KPH_HW_WINDOW_BATCH_ENTRY),
                          &entriesLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    entries = KphAllocatePaged(entriesLength, KPH_TAG_HW_WINDOW_BATCH);
    if (!entries)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate platform window batch buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphCopyFromMode(entries, Entries, entriesLength, AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    if (Source == KPH_HW_PLATFORM_CPU)
    {
        table = dyn->Hardware.CpuTable;
    }
    else
    {
        table = dyn->Hardware.BoardTable;
    }

    if (!table)
    {
        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    status = KphDynHwFindWindowOp(table, OpIndex, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindWindowOp failed: %!STATUS!",
                      status);

        goto Exit;
    }

    window = (PKPH_DYN_HW_WINDOW_READ)op->Body;

    switch (window->BaseKind)
    {
        case KPH_DYN_HW_WINDOW_BASE_ACPI_LPIT:
        case KPH_DYN_HW_WINDOW_BASE_ACPI_LPIT_RELATIVE:
        {
            NT_ASSERT(Source == KPH_HW_PLATFORM_CPU);

            status = KphpHwWindowReadLpit(op, Count, entries);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "KphpHwWindowReadLpit failed: %!STATUS!",
                              status);

                goto Exit;
            }

            break;
        }
        case KPH_DYN_HW_WINDOW_BASE_FIXED_MMIO:
        {
            status = KphpHwWindowReadFixedMmio(op, Count, entries);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "KphpHwWindowReadFixedMmio failed: %!STATUS!",
                              status);

                goto Exit;
            }

            break;
        }
        case KPH_DYN_HW_WINDOW_BASE_FIXED_IO:
        {
            NT_ASSERT(Source == KPH_HW_PLATFORM_BOARD);

            status = KphHwReadFixedIoWindow(op, Count, entries);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "KphHwReadFixedIoWindow failed: %!STATUS!",
                              status);

                goto Exit;
            }

            break;
        }
        default:
        {
            NT_ASSERT(FALSE);

            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Platform window base kind %lu unknown",
                          (ULONG)window->BaseKind);

            status = STATUS_NOT_FOUND;
            goto Exit;
        }
    }

    status = KphCopyToMode(Entries, entries, entriesLength, AccessMode);

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (entries)
    {
        KphFree(entries, KPH_TAG_HW_WINDOW_BATCH);
    }

    return status;
}
