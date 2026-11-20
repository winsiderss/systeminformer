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
#include <script.h>
#include <acpitabl.h>

#include <trace.h>

typedef struct _KPH_LUA_HW_MEMORY_RANGE
{
    ULONG64 Begin;
    ULONG64 End;
} KPH_LUA_HW_MEMORY_RANGE, *PKPH_LUA_HW_MEMORY_RANGE;

typedef struct _KPH_LUA_HW_MAPPING
{
    PVOID Address;
    ULONG Length;
    BOOLEAN Writable;
} KPH_LUA_HW_MAPPING, *PKPH_LUA_HW_MAPPING;

KPH_PROTECTED_DATA_SECTION_PUSH();
static BOOLEAN KphpLuaHwNvdimmKnown = FALSE;
static ULONG KphpLuaHwNvdimmCount = 0;
static PKPH_LUA_HW_MEMORY_RANGE KphpLuaHwNvdimmRanges = NULL;
KPH_PROTECTED_DATA_SECTION_POP();
static PAGED_LOOKASIDE_LIST KphpLuaHwMappingLookaside = { 0 };

KPH_PAGED_FILE();

/**
 * \brief Queries the ACPI firmware table provider.
 *
 * \param[in] Action Enumerate or get.
 * \param[in] TableId The table signature, zero to enumerate.
 * \param[out] Info Receives the answer, freed with
 * KPH_TAG_LUA_HARDWARE_FIRMWARE.
 *
 * \return Successful or errant status; STATUS_NOT_FOUND when the provider
 * answers with nothing.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwQueryAcpiProvider(
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

    info = KphAllocatePaged(infoLength, KPH_TAG_LUA_HARDWARE_FIRMWARE);
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
        KphFree(info, KPH_TAG_LUA_HARDWARE_FIRMWARE);
    }

    return status;
}

/**
 * \brief Retrieves an ACPI table the provider enumerates.
 *
 * \param[in] Tables The enumerated table signatures.
 * \param[in] Signature The table signature.
 * \param[out] Info Receives the table, freed with
 * KPH_TAG_LUA_HARDWARE_FIRMWARE.
 * \param[out] Length Receives the length of the table.
 *
 * \return Successful or errant status; STATUS_NOT_FOUND when the table is
 * not enumerated.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwQueryAcpiTable(
    _In_ PSYSTEM_FIRMWARE_TABLE_INFORMATION Tables,
    _In_ ULONG Signature,
    _Outptr_ PSYSTEM_FIRMWARE_TABLE_INFORMATION* Info,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;
    BOOLEAN present;
    PDESCRIPTION_HEADER header;

    KPH_PAGED_CODE_PASSIVE();

    *Info = NULL;
    *Length = 0;

    info = NULL;
    present = FALSE;

    for (ULONG i = 0; i < (Tables->TableBufferLength / sizeof(ULONG)); i++)
    {
        if (((PULONG)Tables->TableBuffer)[i] == Signature)
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

    status = KphpLuaHwQueryAcpiProvider(SystemFirmwareTable_Get,
                                        Signature,
                                        &info);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwQueryAcpiProvider failed: %!STATUS!",
                      status);

        if (status == STATUS_NOT_FOUND)
        {
            status = STATUS_ACPI_INVALID_TABLE;
        }

        goto Exit;
    }

    if (info->TableBufferLength < sizeof(DESCRIPTION_HEADER))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI table 0x%08lx length %lu is below the header",
                      Signature,
                      info->TableBufferLength);

        status = STATUS_ACPI_INVALID_TABLE;
        goto Exit;
    }

    header = (PDESCRIPTION_HEADER)info->TableBuffer;
    if (header->Length != info->TableBufferLength)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI table 0x%08lx length %lu, header says %lu",
                      Signature,
                      info->TableBufferLength,
                      header->Length);

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
        KphFree(info, KPH_TAG_LUA_HARDWARE_FIRMWARE);
    }

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
NTSTATUS KphpLuaHwParseNfit(
    _In_reads_bytes_(Length) PVOID Table,
    _In_ ULONG Length,
    _Out_writes_to_opt_(Capacity, *Count) PKPH_LUA_HW_MEMORY_RANGE Ranges,
    _In_ ULONG Capacity,
    _Out_ PULONG Count
    )
{
    NTSTATUS status;
    ULONG count;
    ULONG headerLength;

    KPH_PAGED_CODE_PASSIVE();

    *Count = 0;

    headerLength = FIELD_OFFSET(NFIT_TABLE, Structures);

    if (Length < headerLength)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "NFIT length %lu is below the header",
                      Length);

        status = STATUS_ACPI_INVALID_TABLE;
        goto Exit;
    }

    count = 0;

    for (ULONG offset = headerLength; offset < Length; )
    {
        USHORT type;
        USHORT structureLength;
        PNFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE spa;
        ULONG64 end;

        if ((Length - offset) < sizeof(NFIT_STRUCT_HEADER))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "NFIT structure at %lu of %lu truncated",
                          offset,
                          Length);

            status = STATUS_ACPI_INVALID_TABLE;
            goto Exit;
        }

        spa = Add2Ptr(Table, offset);
        type = spa->Header.Type;
        structureLength = spa->Header.Length;

        if ((structureLength < sizeof(NFIT_STRUCT_HEADER)) ||
            (structureLength > (Length - offset)))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "NFIT structure at %lu length %lu malformed, "
                          "table %lu",
                          offset,
                          (ULONG)structureLength,
                          Length);

            status = STATUS_ACPI_INVALID_TABLE;
            goto Exit;
        }

        NT_VERIFY(NT_SUCCESS(RtlULongAdd(offset, structureLength, &offset)));

        if (type != NfitSystemPhysicalAddressRange)
        {
            continue;
        }

        if (structureLength < sizeof(NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "NFIT SPA range length %lu truncated",
                          (ULONG)structureLength);

            status = STATUS_ACPI_INVALID_TABLE;
            goto Exit;
        }

        status = RtlULongLongAdd(spa->SPARangeBase,
                                 spa->SPARangeLength,
                                 &end);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "NFIT SPA range 0x%llx length 0x%llx overflows",
                          spa->SPARangeBase,
                          spa->SPARangeLength);

            status = STATUS_ACPI_INVALID_TABLE;
            goto Exit;
        }

        if (Ranges && (count >= Capacity))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "NFIT SPA range beyond %lu",
                          Capacity);

            status = STATUS_BUFFER_TOO_SMALL;
            goto Exit;
        }

        if (Ranges)
        {
            Ranges[count].Begin = spa->SPARangeBase;
            Ranges[count].End = end;
        }

        count++;
    }

    *Count = count;

    status = STATUS_SUCCESS;

Exit:

    return status;
}

/**
 * \brief Reads the system physical address ranges of the NFIT table.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphpLuaHwInitializeNfit(
    VOID
    )
{
    NTSTATUS status;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION tables;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;
    ULONG length;
    ULONG count;
    ULONG rangesLength;
    PKPH_LUA_HW_MEMORY_RANGE ranges;

    KPH_PAGED_CODE_PASSIVE();

    info = NULL;
    ranges = NULL;

    status = KphpLuaHwQueryAcpiProvider(SystemFirmwareTable_Enumerate,
                                        0,
                                        &tables);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwQueryAcpiProvider failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpLuaHwQueryAcpiTable(tables,
                                     NFIT_TABLE_SIGNATURE,
                                     &info,
                                     &length);
    if (status == STATUS_NOT_FOUND)
    {
        KphpLuaHwNvdimmKnown = TRUE;
        status = STATUS_SUCCESS;
        goto Exit;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwQueryAcpiTable failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpLuaHwParseNfit(info->TableBuffer, length, NULL, 0, &count);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwParseNfit failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!count)
    {
        KphpLuaHwNvdimmKnown = TRUE;
        goto Exit;
    }

    status = RtlULongMult(count,
                          sizeof(KPH_LUA_HW_MEMORY_RANGE),
                          &rangesLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    ranges = KphAllocatePaged(rangesLength, KPH_TAG_LUA_HARDWARE_NVDIMM);
    if (!ranges)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate NVDIMM ranges");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpLuaHwParseNfit(info->TableBuffer,
                                length,
                                ranges,
                                count,
                                &count);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwParseNfit failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphpLuaHwNvdimmRanges = ranges;
    KphpLuaHwNvdimmCount = count;
    KphpLuaHwNvdimmKnown = TRUE;
    ranges = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (ranges)
    {
        KphFree(ranges, KPH_TAG_LUA_HARDWARE_NVDIMM);
    }

    if (info)
    {
        KphFree(info, KPH_TAG_LUA_HARDWARE_FIRMWARE);
    }

    if (tables)
    {
        KphFree(tables, KPH_TAG_LUA_HARDWARE_FIRMWARE);
    }

    return status;
}

/**
 * \brief Checks a base and a length argument of a physical range.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[out] Begin Receives the start of the range.
 * \param[out] End Receives the end of the range, exclusive, at most
 * ULONG64_MAX.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaHwCheckRange(
    _In_ lua_State* State,
    _Out_ PULONG64 Begin,
    _Out_ PULONG64 End
    )
{
    NTSTATUS status;
    ULONG64 begin;
    ULONG64 length;

    KPH_PAGED_CODE_PASSIVE();

    *Begin = 0;
    *End = 0;

    begin = (ULONG64)luaL_checkinteger(State, 1);
    length = (ULONG64)luaL_checkinteger(State, 2);

    status = RtlULongLongAdd(begin, length, End);
    if (!NT_SUCCESS(status))
    {
        *End = ULONG64_MAX;
    }

    *Begin = begin;
}

/**
 * \brief Tells whether a physical range overlaps memory the operating system
 * owns (ksi.hw.is_ram).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: a boolean, or nil and a status when the
 * memory ranges are not available.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwIsRam(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    ULONG64 begin;
    ULONG64 end;
    PPHYSICAL_MEMORY_RANGE ranges;
    BOOLEAN overlaps;
    ULONG count;

    KPH_PAGED_CODE_PASSIVE();

    KphpLuaHwCheckRange(State, &begin, &end);

    ranges = MmGetPhysicalMemoryRanges();
    if (!ranges)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Physical memory ranges unavailable");

        results = KphLuaNilErrorStatus(State, STATUS_INSUFFICIENT_RESOURCES);
        goto Exit;
    }

    overlaps = FALSE;
    count = 0;

    for (PPHYSICAL_MEMORY_RANGE range = ranges;
         range->BaseAddress.QuadPart || range->NumberOfBytes.QuadPart;
         range++)
    {
        ULONG64 rangeBegin;
        ULONG64 rangeEnd;

        count++;

        rangeBegin = (ULONG64)range->BaseAddress.QuadPart;

        status = RtlULongLongAdd(rangeBegin,
                                 (ULONG64)range->NumberOfBytes.QuadPart,
                                 &rangeEnd);
        if (!NT_SUCCESS(status))
        {
            rangeEnd = ULONG64_MAX;
        }

        if ((begin < rangeEnd) && (rangeBegin < end))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Range 0x%llx-0x%llx overlaps memory "
                          "0x%llx-0x%llx",
                          begin,
                          end,
                          rangeBegin,
                          rangeEnd);

            overlaps = TRUE;
            break;
        }
    }

    KphFreePool(ranges);

    KphScriptCharge(State, count);

    lua_pushboolean(State, overlaps);

    results = 1;

Exit:

    return results;
}

/**
 * \brief Tells whether a physical range overlaps memory the firmware
 * describes as non-volatile (ksi.hw.is_nvdimm).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return One: a boolean, or nil when the firmware table could not be read.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwIsNvdimm(
    _In_ lua_State* State
    )
{
    ULONG64 begin;
    ULONG64 end;
    BOOLEAN overlaps;

    KPH_PAGED_CODE_PASSIVE();

    KphpLuaHwCheckRange(State, &begin, &end);

    if (!KphpLuaHwNvdimmKnown)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "NVDIMM ranges unknown");

        lua_pushnil(State);
        return 1;
    }

    KphScriptCharge(State, KphpLuaHwNvdimmCount);

    overlaps = FALSE;

    for (ULONG i = 0; i < KphpLuaHwNvdimmCount; i++)
    {
        PKPH_LUA_HW_MEMORY_RANGE range;

        range = &KphpLuaHwNvdimmRanges[i];

        if ((begin < range->End) && (range->Begin < end))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Range 0x%llx-0x%llx overlaps NVDIMM "
                          "0x%llx-0x%llx",
                          begin,
                          end,
                          range->Begin,
                          range->End);

            overlaps = TRUE;
            break;
        }
    }

    lua_pushboolean(State, overlaps);

    return 1;
}

/**
 * \brief Unmaps a mapping of ksi.hw.map.
 *
 * \param[in] Context The mapping.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwUnmap(
    _In_opt_ PVOID Context
    )
{
    PKPH_LUA_HW_MAPPING mapping;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    mapping = Context;

    MmUnmapIoSpace(mapping->Address, mapping->Length);

    KphFreeToPagedLookaside(&KphpLuaHwMappingLookaside, mapping);
}

/**
 * \brief Maps a range of physical memory (ksi.hw.map).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the mapping, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwMap(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    ULONG64 base;
    lua_Integer length;
    BOOLEAN writable;
    ULONG64 end;
    PHYSICAL_ADDRESS address;
    ULONG protect;
    PKPH_LUA_HW_MAPPING mapping;

    KPH_PAGED_CODE_PASSIVE();

    base = (ULONG64)luaL_checkinteger(State, 1);
    length = luaL_checkinteger(State, 2);
    writable = !!lua_toboolean(State, 3);

    luaL_argcheck(State,
                  (length >= 1) && (length <= ULONG_MAX),
                  2,
                  "out of range");
    status = RtlULongLongAdd(base, (ULONG64)length, &end);

    luaL_argcheck(State, NT_SUCCESS(status), 2, "wraps");

    mapping = KphAllocateFromPagedLookaside(&KphpLuaHwMappingLookaside);
    if (!mapping)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate a mapping");

        status = STATUS_INSUFFICIENT_RESOURCES;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    NT_ASSERT(sizeof(KPH_LUA_HW_MAPPING) <= KphpLuaHwMappingLookaside.L.Size);

    address.QuadPart = (LONGLONG)base;
    protect = (PAGE_NOCACHE | (writable ? PAGE_READWRITE : PAGE_READONLY));

    mapping->Address = MmMapIoSpaceEx(address, (SIZE_T)length, protect);
    if (!mapping->Address)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "MmMapIoSpaceEx 0x%llx length %lu failed",
                      base,
                      (ULONG)length);

        status = STATUS_INSUFFICIENT_RESOURCES;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    mapping->Length = (ULONG)length;
    mapping->Writable = writable;

    KphScriptPushObject(State, KphpLuaHwUnmap, mapping);
    mapping = NULL;

    results = 1;

Exit:

    if (mapping)
    {
        KphFreeToPagedLookaside(&KphpLuaHwMappingLookaside, mapping);
    }

    return results;
}

/**
 * \brief Checks an offset and a width within a mapping.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Mapping The mapping.
 * \param[out] Width Receives the width: 1, 2, 4 or 8.
 *
 * \return The address in the mapping.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphpLuaHwCheckAccess(
    _In_ lua_State* State,
    _In_ PKPH_LUA_HW_MAPPING Mapping,
    _Out_ PULONG Width
    )
{
    lua_Integer offset;
    lua_Integer width;

    KPH_PAGED_CODE_PASSIVE();

    *Width = 0;

    offset = luaL_checkinteger(State, 2);
    width = luaL_checkinteger(State, 3);

    luaL_argcheck(State,
                  (width == 1) || (width == 2) || (width == 4) || (width == 8),
                  3,
                  "1, 2, 4 or 8");
    luaL_argcheck(State,
                  (offset >= 0) &&
                  ((offset % width) == 0) &&
                  (offset <= Mapping->Length) &&
                  (width <= (Mapping->Length - offset)),
                  2,
                  "out of range");

    *Width = (ULONG)width;

    return Add2Ptr(Mapping->Address, (ULONG)offset);
}

/**
 * \brief Reads a mapping (ksi.hw.map_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return One, the value.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwMapRead(
    _In_ lua_State* State
    )
{
    PKPH_LUA_HW_MAPPING mapping;
    PVOID address;
    ULONG width;
    ULONG64 value;

    KPH_PAGED_CODE_PASSIVE();

    mapping = KphScriptCheckCleanup(State, 1, KphpLuaHwUnmap);
    address = KphpLuaHwCheckAccess(State, mapping, &width);

    switch (width)
    {
        case 1:
        {
            value = READ_REGISTER_UCHAR(address);
            break;
        }
        case 2:
        {
            value = READ_REGISTER_USHORT(address);
            break;
        }
        case 4:
        {
            value = READ_REGISTER_ULONG(address);
            break;
        }
        case 8:
        {
            value = READ_REGISTER_ULONG64(address);
            break;
        }
        DEFAULT_UNREACHABLE;
    }

    lua_pushinteger(State, (lua_Integer)value);

    return 1;
}

/**
 * \brief Writes a mapping (ksi.hw.map_write).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return Zero.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwMapWrite(
    _In_ lua_State* State
    )
{
    PKPH_LUA_HW_MAPPING mapping;
    PVOID address;
    ULONG width;
    ULONG64 value;

    KPH_PAGED_CODE_PASSIVE();

    mapping = KphScriptCheckCleanup(State, 1, KphpLuaHwUnmap);
    address = KphpLuaHwCheckAccess(State, mapping, &width);
    value = (ULONG64)luaL_checkinteger(State, 4);

    luaL_argcheck(State, mapping->Writable, 1, "not writable");

    switch (width)
    {
        case 1:
        {
            WRITE_REGISTER_UCHAR(address, (UCHAR)value);
            break;
        }
        case 2:
        {
            WRITE_REGISTER_USHORT(address, (USHORT)value);
            break;
        }
        case 4:
        {
            WRITE_REGISTER_ULONG(address, (ULONG)value);
            break;
        }
        case 8:
        {
            WRITE_REGISTER_ULONG64(address, value);
            break;
        }
        DEFAULT_UNREACHABLE;
    }

    return 0;
}

/**
 * \brief Reads a block of a mapping (ksi.hw.map_read_block).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return One, the bytes.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwMapReadBlock(
    _In_ lua_State* State
    )
{
    PKPH_LUA_HW_MAPPING mapping;
    lua_Integer offset;
    lua_Integer length;
    lua_Integer width;
    PVOID address;
    luaL_Buffer buffer;
    PCHAR data;

    KPH_PAGED_CODE_PASSIVE();

    mapping = KphScriptCheckCleanup(State, 1, KphpLuaHwUnmap);
    offset = luaL_checkinteger(State, 2);
    length = luaL_checkinteger(State, 3);
    width = luaL_optinteger(State, 4, 1);

    luaL_argcheck(State, (width == 1) || (width == 4), 4, "1 or 4");
    luaL_argcheck(State,
                  (length >= 0) &&
                  (length <= mapping->Length) &&
                  ((length % width) == 0),
                  3,
                  "out of range");
    luaL_argcheck(State,
                  (offset >= 0) &&
                  ((offset % width) == 0) &&
                  (offset <= (mapping->Length - length)),
                  2,
                  "out of range");

    KphScriptCharge(State, (ULONG64)(length / width));

    data = luaL_buffinitsize(State, &buffer, (SIZE_T)length);

    address = Add2Ptr(mapping->Address, (ULONG)offset);

    if (width == 4)
    {
        READ_REGISTER_BUFFER_ULONG(address, (PULONG)data, (ULONG)(length / 4));
    }
    else
    {
        READ_REGISTER_BUFFER_UCHAR(address, (PUCHAR)data, (ULONG)length);
    }

    luaL_pushresultsize(&buffer, (SIZE_T)length);

    return 1;
}

/**
 * \brief Writes a block of a mapping (ksi.hw.map_write_block).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return Zero.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwMapWriteBlock(
    _In_ lua_State* State
    )
{
    PKPH_LUA_HW_MAPPING mapping;
    lua_Integer offset;
    PCSTR data;
    SIZE_T length;
    lua_Integer width;
    PVOID address;

    KPH_PAGED_CODE_PASSIVE();

    mapping = KphScriptCheckCleanup(State, 1, KphpLuaHwUnmap);
    offset = luaL_checkinteger(State, 2);
    data = luaL_checklstring(State, 3, &length);
    width = luaL_optinteger(State, 4, 1);

    luaL_argcheck(State, mapping->Writable, 1, "not writable");
    luaL_argcheck(State, (width == 1) || (width == 4), 4, "1 or 4");
    luaL_argcheck(State,
                  (length <= mapping->Length) && ((length % width) == 0),
                  3,
                  "out of range");
    luaL_argcheck(State,
                  (offset >= 0) &&
                  ((offset % width) == 0) &&
                  (offset <= (lua_Integer)(mapping->Length - length)),
                  2,
                  "out of range");

    KphScriptCharge(State, (ULONG64)(length / width));

    address = Add2Ptr(mapping->Address, (ULONG)offset);

    if (width == 4)
    {
        WRITE_REGISTER_BUFFER_ULONG(address, (PULONG)data, (ULONG)(length / 4));
    }
    else
    {
        WRITE_REGISTER_BUFFER_UCHAR(address, (PUCHAR)data, (ULONG)length);
    }

    return 0;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpLuaHwMemoryLibrary[] =
{
    { "is_nvdimm", KphpLuaHwIsNvdimm },
    { "is_ram", KphpLuaHwIsRam },
    { "map", KphpLuaHwMap },
    { "map_read", KphpLuaHwMapRead },
    { "map_read_block", KphpLuaHwMapReadBlock },
    { "map_write", KphpLuaHwMapWrite },
    { "map_write_block", KphpLuaHwMapWriteBlock },
    { NULL, NULL }
};
KPH_PROTECTED_DATA_SECTION_RO_POP();

/**
 * \brief Initializes the physical memory functions of the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardwareMemory(
    VOID
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    KphInitializePagedLookaside(&KphpLuaHwMappingLookaside,
                                sizeof(KPH_LUA_HW_MAPPING),
                                KPH_TAG_LUA_HARDWARE_MAPPING);

    status = KphpLuaHwInitializeNfit();

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "NVDIMM ranges: %lu, known %!bool!: %!STATUS!",
                  KphpLuaHwNvdimmCount,
                  KphpLuaHwNvdimmKnown,
                  status);
}

/**
 * \brief Cleans up the physical memory functions of the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardwareMemory(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (KphpLuaHwNvdimmRanges)
    {
        KphFree(KphpLuaHwNvdimmRanges, KPH_TAG_LUA_HARDWARE_NVDIMM);
    }

    KphDeletePagedLookaside(&KphpLuaHwMappingLookaside);
}

/**
 * \brief Adds the physical memory functions to the hardware library.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state, with the library table on top.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareMemory(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    luaL_setfuncs(State, KphpLuaHwMemoryLibrary, 0);
}
