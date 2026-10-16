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

KPH_PAGED_FILE();

/**
 * \brief Maps a physical range read-only and uncached after the range checks.
 *
 * \param[in] Locator The locator of the device the range belongs to.
 * \param[in] Begin The start of the range.
 * \param[in] Length The length of the range, non-zero.
 * \param[in] CheckBegin The start of the checked range, at or below Begin.
 * \param[in] CheckEnd The end of the checked range, at or above the range end.
 * \param[out] Mapping Receives the mapping; unmap with MmUnmapIoSpace.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwVsecMap(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG64 Begin,
    _In_ ULONG Length,
    _In_ ULONG64 CheckBegin,
    _In_ ULONG64 CheckEnd,
    _Outptr_result_bytebuffer_(Length) PVOID* Mapping
    )
{
    NTSTATUS status;
    ULONG64 checkBegin;
    ULONG64 checkEnd;
    PHYSICAL_ADDRESS physical;
    PVOID mapping;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Length &&
              (CheckBegin <= Begin) &&
              (Begin <= CheckEnd) &&
              (Length <= (CheckEnd - Begin)));

    *Mapping = NULL;

    if (CheckEnd > (MAXULONG64 - PAGE_SIZE))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "VSEC check end 0x%llx out of range",
                      CheckEnd);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    checkBegin = (ULONG64)PAGE_ALIGN(CheckBegin);
    checkEnd = ROUND_TO_PAGES(CheckEnd);

    status = KphHwWindowCheckRange(checkBegin, checkEnd);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "VSEC %04lx:%02lx:%02lx.%lx refused: %!STATUS!",
                      (ULONG)Locator->Segment,
                      (ULONG)Locator->Bus,
                      (ULONG)Locator->Device,
                      (ULONG)Locator->Function,
                      status);

        return status;
    }

    physical.QuadPart = (LONGLONG)Begin;

    mapping = MmMapIoSpaceEx(physical, Length, PAGE_READONLY | PAGE_NOCACHE);
    if (!mapping)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "MmMapIoSpaceEx failed");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *Mapping = mapping;

    return STATUS_SUCCESS;
}

/**
 * \brief Copies the leading dwords of each entry of a discovery table.
 *
 * \details Caller holds the PCI lock with the device open.
 *
 * \param[in] Locator The locator of the device of the table.
 * \param[in] Base The physical base of the table, 4-aligned.
 * \param[in] EntryCount The number of entries.
 * \param[in] EntryStride The size of an entry in bytes.
 * \param[in] EntryDwords The dwords to copy of each entry.
 * \param[out] Buffer Receives EntryDwords dwords per entry.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwVsecCopyTable(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG64 Base,
    _In_ ULONG EntryCount,
    _In_ ULONG EntryStride,
    _In_ ULONG EntryDwords,
    _Out_writes_(EntryCount * EntryDwords) PULONG Buffer
    )
{
    NTSTATUS status;
    ULONG length;
    ULONG64 end;
    PVOID mapping;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT((EntryCount >= 1) && (EntryCount <= MAXUCHAR));
    NT_ASSERT(((EntryStride % sizeof(ULONG)) == 0) &&
              (EntryStride <= (MAXUCHAR * sizeof(ULONG))));
    NT_ASSERT((EntryDwords >= 1) &&
              ((EntryDwords * sizeof(ULONG)) <= EntryStride));
    NT_ASSERT((Base % sizeof(ULONG)) == 0);

    NT_VERIFY(NT_SUCCESS(RtlULongMult(EntryCount, EntryStride, &length)));

    status = RtlULongLongAdd(Base, length, &end);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongLongAdd failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphpHwVsecMap(Locator, Base, length, Base, end, &mapping);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwVsecMap failed: %!STATUS!",
                      status);

        return status;
    }

    for (ULONG i = 0; i < EntryCount; i++)
    {
        SIZE_T entryOffset;
        SIZE_T bufferIndex;
        PULONG entry;
        PULONG buffer;

        NT_VERIFY(NT_SUCCESS(RtlSizeTMult(i, EntryStride, &entryOffset)));
        NT_VERIFY(NT_SUCCESS(RtlSizeTMult(i, EntryDwords, &bufferIndex)));

        entry = Add2Ptr(mapping, entryOffset);
        buffer = &Buffer[bufferIndex];

        for (ULONG j = 0; j < EntryDwords; j++)
        {
            buffer[j] = READ_REGISTER_ULONG(&entry[j]);
        }
    }

    MmUnmapIoSpace(mapping, length);

    return STATUS_SUCCESS;
}

/**
 * \brief Validates batch entries against a vsec_read op and a located region.
 *
 * \param[in] Op The vsec_read op.
 * \param[in] Location The region.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 * \param[out] SpanBegin Receives the lowest region offset to read.
 * \param[out] SpanEnd Receives the end of the highest read, exclusive.
 *
 * \return TRUE if any entry is to be read, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwVsecValidateEntries(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ PKPH_HW_VSEC_LOCATION Location,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _Out_ PULONG SpanBegin,
    _Out_ PULONG SpanEnd
    )
{
    BOOLEAN anyAllowed;
    ULONG spanBegin;
    ULONG spanEnd;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_VSEC_READ);

    anyAllowed = FALSE;
    spanBegin = MAXULONG;
    spanEnd = 0;

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_WINDOW_BATCH_ENTRY entry;

        entry = &Entries[i];

        entry->Value = 0;

        if (((entry->Width != 4) && (entry->Width != 8)) ||
            ((entry->Offset % entry->Width) != 0))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "VSEC offset 0x%lx width %lu is invalid",
                          entry->Offset,
                          (ULONG)entry->Width);

            entry->Status = STATUS_INVALID_PARAMETER;
            continue;
        }

        if (!KphDynHwVsecReadAllowed(Op, entry->Offset, entry->Width))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "VSEC offset 0x%lx width %lu refused",
                          entry->Offset,
                          (ULONG)entry->Width);

            entry->Status = STATUS_ACCESS_DENIED;
            continue;
        }

        NT_ASSERT((entry->Offset + entry->Width) <=
                  ((PKPH_DYN_HW_VSEC_READ)Op->Body)->MaxRegionSize);

        if ((entry->Offset + entry->Width) > Location->Size)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "VSEC offset 0x%lx width %lu exceeds region size "
                          "0x%lx",
                          entry->Offset,
                          (ULONG)entry->Width,
                          Location->Size);

            entry->Status = STATUS_ACCESS_DENIED;
            continue;
        }

        if (((Location->Base + entry->Offset) % entry->Width) != 0)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "VSEC offset 0x%lx is misaligned in the region",
                          entry->Offset);

            entry->Status = STATUS_DATATYPE_MISALIGNMENT;
            continue;
        }

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
 * \brief Maps the span the prepared entries cover, reads them, and unmaps it.
 *
 * \param[in] Locator The locator of the device of the region.
 * \param[in] Location The region.
 * \param[in] SpanBegin The lowest region offset to read.
 * \param[in] SpanEnd The end of the highest read, exclusive.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The prepared batch entries.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwVsecMapAndRead(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_HW_VSEC_LOCATION Location,
    _In_ ULONG SpanBegin,
    _In_ ULONG SpanEnd,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    )
{
    NTSTATUS status;
    ULONG64 regionEnd;
    ULONG64 spanBase;
    ULONG spanLength;
    PVOID mapping;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT((SpanBegin < SpanEnd) &&
              (SpanEnd <= Location->Size));

    status = RtlULongLongAdd(Location->Base, Location->Size, &regionEnd);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongLongAdd failed: %!STATUS!",
                      status);

        return status;
    }

    NT_VERIFY(NT_SUCCESS(RtlULongLongAdd(Location->Base,
                                         SpanBegin,
                                         &spanBase)));
    spanLength = (SpanEnd - SpanBegin);

    status = KphpHwVsecMap(Locator,
                           spanBase,
                           spanLength,
                           Location->Base,
                           regionEnd,
                           &mapping);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwVsecMap failed: %!STATUS!",
                      status);

        return status;
    }

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_WINDOW_BATCH_ENTRY entry;
        PVOID address;

        entry = &Entries[i];

        if (entry->Status != STATUS_PENDING)
        {
            continue;
        }

        address = Add2Ptr(mapping, entry->Offset - SpanBegin);

        switch (entry->Width)
        {
            case 4:
            {
                entry->Value = READ_REGISTER_ULONG((PULONG)address);
                entry->Status = STATUS_SUCCESS;
                break;
            }
            case 8:
            {
                entry->Value = READ_REGISTER_ULONG64((PULONG64)address);
                entry->Status = STATUS_SUCCESS;
                break;
            }
            default:
            {
                NT_ASSERT(FALSE);
                entry->Status = STATUS_INVALID_PARAMETER;
                break;
            }
        }
    }

    MmUnmapIoSpace(mapping, spanLength);

    return STATUS_SUCCESS;
}

/**
 * \brief Reads a batch of allow-listed registers of an Intel VSEC/DVSEC region.
 *
 * \param[in] Locator The locator of the device.
 * \param[in] OpIndex The index of the vsec_read op among those of the device.
 * \param[in] Instance The index of the region among those the op exposes.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, offsets relative to the region.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadVsecBatch(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Instance,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "VSEC batch is empty");

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

    entries = KphAllocatePaged(entriesLength, KPH_TAG_HW_VSEC_BATCH);
    if (!entries)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate VSEC batch buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphCopyFromMode(entries, Entries, entriesLength, AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = KphHwPciReadVsec(&locator,
                              dyn->Hardware.Section,
                              OpIndex,
                              Instance,
                              Count,
                              entries);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwPciReadVsec failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphCopyToMode(Entries, entries, entriesLength, AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (entries)
    {
        KphFree(entries, KPH_TAG_HW_VSEC_BATCH);
    }

    return status;
}

/**
 * \brief Lists the regions the vsec_read ops of a device expose.
 *
 * \param[in] Locator The locator of the device.
 * \param[in] Count The capacity of Regions.
 * \param[out] Regions Receives the regions; NULL only when Count is zero.
 * \param[out] ReturnCount Receives the number of regions, also when they do
 * not fit.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status; STATUS_BUFFER_TOO_SMALL when the
 * regions do not fit.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwQueryVsecRegions(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG Count,
    _Out_writes_to_opt_(Count, *ReturnCount) PKPH_HW_VSEC_REGION Regions,
    _Out_ PULONG ReturnCount,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    ULONG capacity;
    ULONG regionsLength;
    PKPH_HW_VSEC_REGION regions;
    ULONG total;
    ULONG copyLength;
    PKPH_DYN dyn;

    KPH_PAGED_CODE_PASSIVE();

    regions = NULL;
    dyn = NULL;

    locator = *Locator;

    if (Count && !Regions)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "VSEC region buffer missing for count %lu",
                      Count);

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->Hardware.Section)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    capacity = min(max(Count, 1), KPH_DYN_HW_VSEC_MAX_TABLE_REGIONS);

    status = RtlULongMult(capacity, sizeof(KPH_HW_VSEC_REGION), &regionsLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    regions = KphAllocatePaged(regionsLength, KPH_TAG_HW_VSEC_BATCH);
    if (!regions)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate VSEC region buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphHwPciQueryVsecRegions(&locator,
                                      dyn->Hardware.Section,
                                      capacity,
                                      regions,
                                      &total);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwPciQueryVsecRegions failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphWriteULongToMode(ReturnCount, total, AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    if ((total > Count) || (total > capacity))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "VSEC regions %lu exceed count %lu capacity %lu",
                      total,
                      Count,
                      capacity);

        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    if (Regions && total)
    {
        NT_VERIFY(NT_SUCCESS(RtlULongMult(total,
                                          sizeof(KPH_HW_VSEC_REGION),
                                          &copyLength)));

        status = KphCopyToMode(Regions, regions, copyLength, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (regions)
    {
        KphFree(regions, KPH_TAG_HW_VSEC_BATCH);
    }

    return status;
}
