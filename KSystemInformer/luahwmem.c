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

#include <trace.h>

typedef struct _KPH_LUA_HW_MAPPING
{
    PVOID Address;
    ULONG Length;
    BOOLEAN Writable;
} KPH_LUA_HW_MAPPING, *PKPH_LUA_HW_MAPPING;

static PAGED_LOOKASIDE_LIST KphpLuaHwMappingLookaside = { 0 };

KPH_PAGED_FILE();

/**
 * \brief Frees the physical memory ranges of MmGetPhysicalMemoryRanges.
 *
 * \param[in] Context The ranges.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwFreeMemoryRanges(
    _In_opt_ PVOID Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    KphFreePool(Context);
}

/**
 * \brief Lists the physical memory ranges the operating system owns
 * (ksi.hw.memory_ranges).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the ranges, each its base and its length as
 * two little-endian 64-bit values, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwMemoryRanges(
    _In_ lua_State* State
    )
{
    PPHYSICAL_MEMORY_RANGE ranges;
    PPHYSICAL_MEMORY_RANGE range;
    ULONG count;
    SIZE_T size;
    int cleanup;
    luaL_Buffer buffer;
    PCHAR data;

    KPH_PAGED_CODE_PASSIVE();

    ranges = MmGetPhysicalMemoryRanges();
    if (!ranges)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Physical memory ranges unavailable");

        return KphLuaNilErrorStatus(State, STATUS_INSUFFICIENT_RESOURCES);
    }

    KphScriptPushCleanup(State, KphpLuaHwFreeMemoryRanges, ranges);
    cleanup = lua_gettop(State);

    count = 0;

    for (range = ranges;
         range->BaseAddress.QuadPart || range->NumberOfBytes.QuadPart;
         range++)
    {
        count++;
    }

    KphScriptCharge(State, count);

    NT_VERIFY(NT_SUCCESS(RtlSizeTMult(count, 16, &size)));

    data = luaL_buffinitsize(State, &buffer, size);

    for (ULONG i = 0; i < count; i++)
    {
        RtlCopyMemory(data, &ranges[i].BaseAddress.QuadPart, 8);
        RtlCopyMemory(data + 8, &ranges[i].NumberOfBytes.QuadPart, 8);
        data += 16;
    }

    luaL_pushresultsize(&buffer, size);

    KphScriptReleaseCleanup(State, cleanup);
    lua_remove(State, cleanup);

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
                  ((((ULONG_PTR)Mapping->Address + offset) % width) == 0) &&
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
                  ((((ULONG_PTR)mapping->Address + offset) % width) == 0) &&
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
                  ((((ULONG_PTR)mapping->Address + offset) % width) == 0) &&
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
    { "map", KphpLuaHwMap },
    { "map_read", KphpLuaHwMapRead },
    { "map_read_block", KphpLuaHwMapReadBlock },
    { "map_write", KphpLuaHwMapWrite },
    { "map_write_block", KphpLuaHwMapWriteBlock },
    { "memory_ranges", KphpLuaHwMemoryRanges },
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
    KPH_PAGED_CODE_PASSIVE();

    KphInitializePagedLookaside(&KphpLuaHwMappingLookaside,
                                sizeof(KPH_LUA_HW_MAPPING),
                                KPH_TAG_LUA_HARDWARE_MAPPING);
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
