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

#pragma once

#include <kphluaconf.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// lua

typedef struct _KPH_LUA_ALLOCATOR
{
    LIST_ENTRY Blocks;
    SIZE_T Limit;
    SIZE_T Reserve;
    SIZE_T Used;
    ULONG Failures;
} KPH_LUA_ALLOCATOR, *PKPH_LUA_ALLOCATOR;

typedef
_Function_class_(KPH_LUA_CALLOUT_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
KSIAPI
KPH_LUA_CALLOUT_ROUTINE(
    _Inout_opt_ PVOID Context
    );
typedef KPH_LUA_CALLOUT_ROUTINE* PKPH_LUA_CALLOUT_ROUTINE;

typedef
_Function_class_(KPH_LUA_PROTECTED_CALL_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
KSIAPI
KPH_LUA_PROTECTED_CALL_ROUTINE(
    _In_ lua_State* State,
    _Inout_opt_ PVOID Context
    );
typedef KPH_LUA_PROTECTED_CALL_ROUTINE* PKPH_LUA_PROTECTED_CALL_ROUTINE;

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeAllocator(
    _Out_ PKPH_LUA_ALLOCATOR Allocator,
    _In_ SIZE_T Limit,
    _In_ SIZE_T Reserve
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaDeleteAllocator(
    _Inout_ PKPH_LUA_ALLOCATOR Allocator
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaReleaseReserve(
    _Inout_ PKPH_LUA_ALLOCATOR Allocator
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaCallout(
    _In_ PKPH_LUA_CALLOUT_ROUTINE Routine,
    _Inout_opt_ PVOID Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaCreateState(
    _Inout_ PKPH_LUA_ALLOCATOR Allocator,
    _Outptr_ lua_State** State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCloseState(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaProtectedCall(
    _In_ lua_State* State,
    _In_ PKPH_LUA_PROTECTED_CALL_ROUTINE Routine,
    _Inout_opt_ PVOID Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaLoadChunk(
    _In_ lua_State* State,
    _In_reads_bytes_(Length) PCVOID Chunk,
    _In_ SIZE_T Length,
    _In_ PCANSI_STRING Name
    );
