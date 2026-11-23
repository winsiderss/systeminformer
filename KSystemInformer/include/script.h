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

// script file

#define KPH_SCRIPT_FILE_MAGIC ((ULONG)'SISK')

typedef struct _KPH_SCRIPT_LIMITS
{
    ULONG64 Instructions;
    ULONG64 CleanupInstructions;
    ULONG Milliseconds;
    ULONG CleanupMilliseconds;
    ULONG Memory;
    ULONG CleanupMemory;
} KPH_SCRIPT_LIMITS, *PKPH_SCRIPT_LIMITS;

typedef struct _KPH_SCRIPT_ROUTINE
{
    ULONG NameOffset;
    USHORT NameLength;
    USHORT Level;
} KPH_SCRIPT_ROUTINE, *PKPH_SCRIPT_ROUTINE;

typedef struct _KPH_SCRIPT_FILE
{
    ULONG Magic;
    ULONG Version;
    ULONG Length;
    ULONG Level;
    KPH_SCRIPT_LIMITS Limits;
    ULONG NameOffset;
    ULONG RoutinesOffset;
    ULONG ChunkOffset;
    ULONG ChunkLength;
    USHORT NameLength;
    USHORT RoutineCount;
    // KPH_SCRIPT_ROUTINE Routines[RoutineCount];
    // CHAR Names[];
    // BYTE Chunk[ChunkLength];
} KPH_SCRIPT_FILE, *PKPH_SCRIPT_FILE;

C_ASSERT(sizeof(KPH_SCRIPT_LIMITS) == 32);
C_ASSERT(sizeof(KPH_SCRIPT_ROUTINE) == 8);
C_ASSERT(sizeof(KPH_SCRIPT_FILE) == 72);

// lua

typedef struct _KPH_LUA_ALLOCATOR
{
    LIST_ENTRY Blocks;
    SIZE_T Limit;
    SIZE_T Reserve;
    SIZE_T Used;
    SIZE_T CollectAt;
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
VOID KphLuaRestoreReserve(
    _Inout_ PKPH_LUA_ALLOCATOR Allocator,
    _In_ SIZE_T Reserve
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

// lualib

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenLibraries(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaRaiseAbort(
    _In_ lua_State* State
    );

FORCEINLINE
VOID KphLuaPushStatus(
    _In_ lua_State* State,
    _In_ NTSTATUS Status
    )
{
    lua_pushinteger(State, (lua_Integer)Status);
}

FORCEINLINE
int KphLuaNilErrorStatus(
    _In_ lua_State* State,
    _In_ NTSTATUS Status
    )
{
    NT_ASSERT(Status != STATUS_SUCCESS);

    lua_pushnil(State);
    KphLuaPushStatus(State, Status);

    return 2;
}

// luahw

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeLocks(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupLocks(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardware(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardware(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareLibrary(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaHwReferenceDriverDevices(
    _In_ PDRIVER_OBJECT Driver,
    _Outptr_result_buffer_maybenull_(*Count) PDEVICE_OBJECT** Devices,
    _Out_ PULONG Count
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
PDEVICE_OBJECT KphLuaHwTestDevice(
    _In_ lua_State* State,
    _In_ int Index
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
PDEVICE_OBJECT KphLuaHwPciTestDevice(
    _In_ lua_State* State,
    _In_ int Index
    );

_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphLuaHwReleaseRequest(
    _In_opt_ PVOID Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaHwSendRequest(
    _In_ lua_State* State,
    _In_ PDEVICE_OBJECT Device,
    _In_opt_ PFILE_OBJECT FileObject,
    _In_ ULONG IoControlCode,
    _In_ BOOLEAN Internal,
    _In_reads_bytes_(InputLength) PCVOID Input,
    _In_ ULONG InputLength,
    _In_ ULONG OutputLength,
    _In_ ULONG Milliseconds,
    _Outptr_result_maybenull_ PVOID* Request,
    _Outptr_result_maybenull_ PVOID* Output,
    _Out_ PULONG Information
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardwareCpu(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardwareCpu(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareCpu(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardwarePci(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardwarePci(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwarePci(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardwareMemory(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardwareMemory(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareMemory(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardwareDevice(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardwareDevice(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareDevice(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardwareSysreg(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardwareSysreg(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareSysreg(
    _In_ lua_State* State
    );

#if defined(_M_X64)

_IRQL_requires_(DISPATCH_LEVEL)
VOID KphpLuaHwCallSmi(
    _In_ USHORT Port,
    _Inout_updates_(7) PULONG64 Registers
    );

#endif

#if defined(_M_ARM64)

_IRQL_requires_max_(HIGH_LEVEL)
BOOLEAN KphpLuaHwIsSysregListed(
    _In_ ULONG Register
    );

_IRQL_requires_max_(HIGH_LEVEL)
BOOLEAN KphpLuaHwReadSysreg(
    _In_ ULONG Register,
    _Out_ PULONG64 Value,
    _Out_ PBOOLEAN Listed
    );

_IRQL_requires_max_(HIGH_LEVEL)
BOOLEAN KphpLuaHwWriteSysreg(
    _In_ ULONG Register,
    _In_ ULONG64 Value,
    _Out_ PBOOLEAN Listed
    );

#endif

// script

typedef
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
KSIAPI
KPH_SCRIPT_CLEANUP_ROUTINE(
    _In_opt_ PVOID Context
    );
typedef KPH_SCRIPT_CLEANUP_ROUTINE* PKPH_SCRIPT_CLEANUP_ROUTINE;

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptPushCleanup(
    _In_ lua_State* State,
    _In_ PKPH_SCRIPT_CLEANUP_ROUTINE Routine,
    _In_opt_ PVOID Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptPushObject(
    _In_ lua_State* State,
    _In_ PKPH_SCRIPT_CLEANUP_ROUTINE Routine,
    _In_opt_ PVOID Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphScriptTestCleanup(
    _In_ lua_State* State,
    _In_ int Index,
    _In_ PKPH_SCRIPT_CLEANUP_ROUTINE Routine
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphScriptCheckCleanup(
    _In_ lua_State* State,
    _In_ int Index,
    _In_ PKPH_SCRIPT_CLEANUP_ROUTINE Routine
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptReleaseCleanup(
    _In_ lua_State* State,
    _In_ int Index
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
PCANSI_STRING KphScriptGetName(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphScriptIsVolatile(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG64 KphScriptGetAbortCount(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptCharge(
    _In_ lua_State* State,
    _In_ ULONG64 Count
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptEnterHandler(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptLeaveHandler(
    _In_ lua_State* State
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphScriptWait(
    _In_ lua_State* State,
    _In_opt_ PVOID Object,
    _In_ ULONG Milliseconds
    );
