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

//
// The count hook runs once per this many instructions: the grain of the
// instruction limit and of the clock check.
//
#define KPH_SCRIPT_HOOK_INTERVAL 1000
#define KPH_SCRIPT_CLEANUP_NAME "ksi.cleanup"
#define KPH_SCRIPT_OBJECT_NAME "ksi.object"

typedef struct _KPH_SCRIPT_CLEANUP
{
    LIST_ENTRY Entry;
    PKPH_SCRIPT_CLEANUP_ROUTINE Routine;
    PVOID Context;
    BOOLEAN Held;
} KPH_SCRIPT_CLEANUP, *PKPH_SCRIPT_CLEANUP;

typedef enum _KPH_SCRIPT_PHASE
{
    KphScriptPhaseBody,
    KphScriptPhaseCleanup,
    KphScriptPhaseStopped,
} KPH_SCRIPT_PHASE, *PKPH_SCRIPT_PHASE;

typedef struct _KPH_SCRIPT_CALL
{
    PKPH_LUA_ALLOCATOR Allocator;
    PKPH_SCRIPT_LIMITS Limits;
    PCVOID Chunk;
    ULONG ChunkLength;
    PCANSI_STRING Script;
    PCANSI_STRING Routine;
    PCVOID Input;
    ULONG InputLength;
    PVOID Output;
    ULONG OutputLength;
    ULONG ReturnLength;
    KPROCESSOR_MODE AccessMode;
    BOOLEAN Volatile;
    LIST_ENTRY Cleanups;
    PLIST_ENTRY Objects;
    BOOLEAN Opened;
    KPH_SCRIPT_PHASE Phase;
    ULONG64 Instructions;
    ULONG64 Deadline;
    ULONG64 Aborts;
    BOOLEAN Aborted;
    NTSTATUS AbortStatus;
    ULONG Handlers;
    SIZE_T HandlerReserve;
    BOOLEAN AbortDeferred;
    NTSTATUS Status;
} KPH_SCRIPT_CALL, *PKPH_SCRIPT_CALL;

typedef struct _KPH_SCRIPT
{
    LIST_ENTRY Entry;
    LIST_ENTRY Objects;
    ULONG Flags;
    PKPH_SCRIPT_FILE File;
    ANSI_STRING Name;
    KPH_RWLOCK Lock;
    BOOLEAN Faulted;
    KPH_LUA_ALLOCATOR Allocator;
    lua_State* State;
} KPH_SCRIPT, *PKPH_SCRIPT;

typedef enum _KPH_SCRIPT_END
{
    KphScriptEndClean,
    KphScriptEndError,
    KphScriptEndFatal,
} KPH_SCRIPT_END, *PKPH_SCRIPT_END;

typedef struct _KPH_SCRIPT_KEPT_CALL
{
    PKPH_SCRIPT Script;
    PKPH_SCRIPT_CALL Call;
    KPH_SCRIPT_END End;
} KPH_SCRIPT_KEPT_CALL, *PKPH_SCRIPT_KEPT_CALL;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
DECLARE_CONST_UNICODE_STRING(KphpScriptTypeName, L"KphScript");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphpScriptType = NULL;
KPH_PROTECTED_DATA_SECTION_POP();
static KPH_RUNDOWN KphpScriptRundown;
static LIST_ENTRY KphpScripts;
static KPH_RWLOCK KphpScriptsLock;
static BOOLEAN KphpScriptInitialized = FALSE;

KPH_PAGED_FILE();

/**
 * \brief Retrieves the call a Lua state is running.
 *
 * \param[in] State The Lua state.
 *
 * \return The call.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PKPH_SCRIPT_CALL KphpScriptGetCall(
    _In_ lua_State* State
    )
{
    PKPH_SCRIPT_CALL* slot;

    KPH_PAGED_CODE_PASSIVE();

    slot = lua_getextraspace(State);

    NT_ASSERT(*slot);

    return *slot;
}

/**
 * \brief Retrieves the name of the script a Lua state is running.
 *
 * \param[in] State The Lua state.
 *
 * \return The name of the script.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PCANSI_STRING KphScriptGetName(
    _In_ lua_State* State
    )
{
    PKPH_SCRIPT_CALL call;

    KPH_PAGED_CODE_PASSIVE();

    call = KphpScriptGetCall(State);

    return call->Script;
}

/**
 * \brief Retrieves whether the script a Lua state is running is volatile.
 *
 * \param[in] State The Lua state.
 *
 * \return TRUE in volatile mode, FALSE in non-volatile mode.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphScriptIsVolatile(
    _In_ lua_State* State
    )
{
    PKPH_SCRIPT_CALL call;

    KPH_PAGED_CODE_PASSIVE();

    call = KphpScriptGetCall(State);

    return call->Volatile;
}

/**
 * \brief Retrieves how many times the call of a Lua state has been aborted:
 * by a limit, or by a memory error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of aborts.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG64 KphScriptGetAbortCount(
    _In_ lua_State* State
    )
{
    PKPH_SCRIPT_CALL call;

    KPH_PAGED_CODE_PASSIVE();

    call = KphpScriptGetCall(State);

    return (call->Aborts + call->Allocator->Failures);
}

/**
 * \brief Runs the cleanups a call still holds, newest first.
 *
 * \param[in,out] Call The call.
 * \param[in] State The Lua state of the call when it outlives the call, NULL
 * otherwise. It no longer holds the cleanup objects afterwards.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptRunCleanups(
    _Inout_ PKPH_SCRIPT_CALL Call,
    _In_opt_ lua_State* State
    )
{
    PLIST_ENTRY entry;
    PKPH_SCRIPT_CLEANUP cleanup;
    PKPH_SCRIPT_CLEANUP_ROUTINE routine;
    PVOID context;

    KPH_PAGED_CODE_PASSIVE();

    while (!IsListEmpty(&Call->Cleanups))
    {
        entry = RemoveHeadList(&Call->Cleanups);

        cleanup = CONTAINING_RECORD(entry, KPH_SCRIPT_CLEANUP, Entry);

        routine = cleanup->Routine;
        context = cleanup->Context;

        NT_ASSERT(routine);

        cleanup->Routine = NULL;

        if (State && cleanup->Held)
        {
            cleanup->Held = FALSE;

            //
            // Neither allocates, so neither can raise.
            //
            lua_pushnil(State);
            lua_rawsetp(State, LUA_REGISTRYINDEX, cleanup);
        }

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z left a cleanup behind",
                      Call->Script);

        routine(context);
    }
}

/**
 * \brief Runs the cleanups of the objects a script holds, newest first.
 *
 * \param[in,out] Script The script.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptRunObjects(
    _Inout_ PKPH_SCRIPT Script
    )
{
    PLIST_ENTRY entry;
    PKPH_SCRIPT_CLEANUP cleanup;
    PKPH_SCRIPT_CLEANUP_ROUTINE routine;
    PVOID context;

    KPH_PAGED_CODE_PASSIVE();

    while (!IsListEmpty(&Script->Objects))
    {
        entry = RemoveHeadList(&Script->Objects);

        cleanup = CONTAINING_RECORD(entry, KPH_SCRIPT_CLEANUP, Entry);

        routine = cleanup->Routine;
        context = cleanup->Context;

        NT_ASSERT(routine);

        cleanup->Routine = NULL;

        routine(context);
    }
}

/**
 * \brief Retrieves the cleanup object at an index, of either kind.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the value.
 *
 * \return The cleanup object, NULL when the value is not one.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PKPH_SCRIPT_CLEANUP KphpScriptToCleanup(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    PKPH_SCRIPT_CLEANUP cleanup;

    KPH_PAGED_CODE_PASSIVE();

    cleanup = luaL_testudata(State, Index, KPH_SCRIPT_OBJECT_NAME);
    if (!cleanup)
    {
        cleanup = luaL_testudata(State, Index, KPH_SCRIPT_CLEANUP_NAME);
    }

    return cleanup;
}

/**
 * \brief Checks that an argument is a cleanup object, of either kind.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The cleanup object.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PKPH_SCRIPT_CLEANUP KphpScriptRequireCleanup(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    PKPH_SCRIPT_CLEANUP cleanup;

    KPH_PAGED_CODE_PASSIVE();

    cleanup = luaL_testudata(State, Index, KPH_SCRIPT_OBJECT_NAME);
    if (!cleanup)
    {
        cleanup = luaL_checkudata(State, Index, KPH_SCRIPT_CLEANUP_NAME);
    }

    return cleanup;
}

/**
 * \brief Runs the cleanup of a cleanup object now, if it has not run.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the cleanup object.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptReleaseCleanup(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    PKPH_SCRIPT_CLEANUP cleanup;
    PKPH_SCRIPT_CLEANUP_ROUTINE routine;
    PVOID context;

    KPH_PAGED_CODE_PASSIVE();

    cleanup = KphpScriptRequireCleanup(State, Index);

    routine = cleanup->Routine;
    context = cleanup->Context;

    if (!routine)
    {
        return;
    }

    RemoveEntryList(&cleanup->Entry);

    cleanup->Routine = NULL;

    routine(context);

    if (cleanup->Held)
    {
        cleanup->Held = FALSE;

        lua_pushnil(State);
        lua_rawsetp(State, LUA_REGISTRYINDEX, cleanup);
    }
}

/**
 * \brief Runs the cleanup of a cleanup object, if it has not run: its release
 * method, its close handler and its finalizer.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state, with the cleanup object.
 *
 * \return Zero, the number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpScriptReleaseCleanup(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphScriptReleaseCleanup(State, 1);

    return 0;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpScriptObjectMetamethods[] =
{
    { "__close", KphpScriptReleaseCleanup },
    { "__gc", KphpScriptReleaseCleanup },
    { NULL, NULL }
};
KPH_PROTECTED_DATA_SECTION_RO_POP();

/**
 * \brief Sets the metatable of a cleanup object that has a finalizer.
 *
 * \details May raise a Lua error. A state has the metatable from its first
 * such object on, and never has it half made.
 *
 * \param[in] State The Lua state, with the cleanup object.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptSetObjectMetatable(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (luaL_getmetatable(State, KPH_SCRIPT_OBJECT_NAME) == LUA_TNIL)
    {
        lua_settop(State, -2);

        lua_createtable(State, 0, 5);
        luaL_setfuncs(State, KphpScriptObjectMetamethods, 0);

        luaL_getmetatable(State, KPH_SCRIPT_CLEANUP_NAME);
        lua_getfield(State, -1, "__index");
        lua_setfield(State, -3, "__index");
        lua_getfield(State, -1, "__name");
        lua_setfield(State, -3, "__name");
        lua_settop(State, -2);

        lua_pushboolean(State, FALSE);
        lua_setfield(State, -2, "__metatable");

        lua_pushvalue(State, -1);
        lua_setfield(State, LUA_REGISTRYINDEX, KPH_SCRIPT_OBJECT_NAME);
    }

    lua_setmetatable(State, -2);
}

/**
 * \brief Creates a cleanup object that holds no cleanup yet.
 *
 * \details May raise a Lua error. When its argument is true the registry
 * holds the object until its cleanup has run; otherwise the object has a
 * finalizer.
 *
 * \param[in] State The Lua state, with whether the registry holds the object.
 *
 * \return One, the cleanup object.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpScriptNewCleanup(
    _In_ lua_State* State
    )
{
    PKPH_SCRIPT_CLEANUP cleanup;
    BOOLEAN held;

    KPH_PAGED_CODE_PASSIVE();

    held = !!lua_toboolean(State, 1);

    cleanup = lua_newuserdatauv(State, sizeof(KPH_SCRIPT_CLEANUP), 0);

    InitializeListHead(&cleanup->Entry);
    cleanup->Routine = NULL;
    cleanup->Context = NULL;
    cleanup->Held = held;

    if (held)
    {
        luaL_setmetatable(State, KPH_SCRIPT_CLEANUP_NAME);

        lua_pushvalue(State, -1);
        lua_rawsetp(State, LUA_REGISTRYINDEX, cleanup);
    }
    else
    {
        KphpScriptSetObjectMetatable(State);
    }

    return 1;
}

/**
 * \brief Records what undoes something a script was just handed, and pushes
 * the object that stands for it onto the Lua stack.
 *
 * \details The routine runs exactly once, here when it cannot be recorded, in
 * which case this raises a Lua error and does not return.
 *
 * \param[in] State The Lua state.
 * \param[in] Routine The routine that undoes it.
 * \param[in] Context The context for the routine.
 * \param[in] Held TRUE when the registry holds the object until its cleanup
 * has run, FALSE when the object has a finalizer.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptPushCleanup(
    _In_ lua_State* State,
    _In_ PKPH_SCRIPT_CLEANUP_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_ BOOLEAN Held
    )
{
    PKPH_SCRIPT_CALL call;
    PKPH_SCRIPT_CLEANUP cleanup;
    int result;

    KPH_PAGED_CODE_PASSIVE();

    call = KphpScriptGetCall(State);

    if (!lua_checkstack(State, 2))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z has no stack to record a cleanup",
                      call->Script);

        Routine(Context);

        luaL_error(State, "stack overflow");
        return;
    }

    lua_pushcfunction(State, KphpScriptNewCleanup);
    lua_pushboolean(State, Held);

    result = lua_pcall(State, 1, 1, 0);
    if (result != LUA_OK)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z failed to record a cleanup: %ld",
                      call->Script,
                      (LONG)result);

        Routine(Context);

        lua_error(State);
        return;
    }

    cleanup = lua_touserdata(State, -1);

    cleanup->Routine = Routine;
    cleanup->Context = Context;

    if (Held)
    {
        InsertHeadList(&call->Cleanups, &cleanup->Entry);
    }
    else
    {
        InsertHeadList(call->Objects, &cleanup->Entry);
        call->Opened = TRUE;
    }
}

/**
 * \brief Records what undoes something a script was just handed, and pushes
 * the object that stands for it onto the Lua stack.
 *
 * \details The routine runs exactly once: when the script releases or closes
 * the object, when the call returns, or here when it cannot be recorded, in
 * which case this raises a Lua error and does not return.
 *
 * \param[in] State The Lua state.
 * \param[in] Routine The routine that undoes it.
 * \param[in] Context The context for the routine.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptPushCleanup(
    _In_ lua_State* State,
    _In_ PKPH_SCRIPT_CLEANUP_ROUTINE Routine,
    _In_opt_ PVOID Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphpScriptPushCleanup(State, Routine, Context, TRUE);
}

/**
 * \brief Records what undoes an object a script was just handed and may
 * keep, and pushes the object onto the Lua stack.
 *
 * \details The routine runs exactly once: when the script releases or closes
 * the object, when nothing refers to the object any more, when the call of a
 * volatile script returns, when a kept script faults or is deleted, or here
 * when it cannot be recorded, in which case this raises a Lua error and does
 * not return.
 *
 * \param[in] State The Lua state.
 * \param[in] Routine The routine that undoes it.
 * \param[in] Context The context for the routine.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptPushObject(
    _In_ lua_State* State,
    _In_ PKPH_SCRIPT_CLEANUP_ROUTINE Routine,
    _In_opt_ PVOID Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphpScriptPushCleanup(State, Routine, Context, FALSE);
}

/**
 * \brief Retrieves the context of a cleanup object of the caller's kind that
 * has not been released.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the value.
 * \param[in] Routine The routine the caller recorded its objects with.
 *
 * \return The context the object was recorded with, NULL when the value is
 * not such an object.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphScriptTestCleanup(
    _In_ lua_State* State,
    _In_ int Index,
    _In_ PKPH_SCRIPT_CLEANUP_ROUTINE Routine
    )
{
    PKPH_SCRIPT_CLEANUP cleanup;

    KPH_PAGED_CODE_PASSIVE();

    cleanup = KphpScriptToCleanup(State, Index);
    if (!cleanup || (cleanup->Routine != Routine))
    {
        return NULL;
    }

    return cleanup->Context;
}

/**
 * \brief Checks that an argument is a cleanup object of the caller's kind
 * that has not been released.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 * \param[in] Routine The routine the caller recorded its objects with.
 *
 * \return The context the object was recorded with.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphScriptCheckCleanup(
    _In_ lua_State* State,
    _In_ int Index,
    _In_ PKPH_SCRIPT_CLEANUP_ROUTINE Routine
    )
{
    PKPH_SCRIPT_CLEANUP cleanup;

    KPH_PAGED_CODE_PASSIVE();

    cleanup = KphpScriptRequireCleanup(State, Index);

    if (cleanup->Routine != Routine)
    {
        luaL_argerror(State, Index, "released, or another kind of object");
    }

    return cleanup->Context;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpScriptCleanupMethods[] =
{
    { "release", KphpScriptReleaseCleanup },
    { NULL, NULL }
};
static const luaL_Reg KphpScriptCleanupMetamethods[] =
{
    { "__close", KphpScriptReleaseCleanup },
    { NULL, NULL }
};
KPH_PROTECTED_DATA_SECTION_RO_POP();

/**
 * \brief Creates the metatable of cleanup objects.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptOpenCleanup(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    luaL_newmetatable(State, KPH_SCRIPT_CLEANUP_NAME);
    luaL_setfuncs(State, KphpScriptCleanupMetamethods, 0);

    luaL_newlib(State, KphpScriptCleanupMethods);
    lua_setfield(State, -2, "__index");

    //
    // A script cannot reach the metatable.
    //
    lua_pushboolean(State, FALSE);
    lua_setfield(State, -2, "__metatable");

    lua_settop(State, -2);
}

/**
 * \brief Computes the interrupt time a number of milliseconds from a time.
 *
 * \param[in] Now The interrupt time to count from.
 * \param[in] Milliseconds The milliseconds to add.
 *
 * \return The deadline, the time counted from if it does not fit.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG64 KphpScriptDeadline(
    _In_ ULONG64 Now,
    _In_ ULONG Milliseconds
    )
{
    NTSTATUS status;
    ULONG64 ticks;
    ULONG64 deadline;

    KPH_PAGED_CODE_PASSIVE();

    status = RtlULongLongMult(Milliseconds, 10000, &ticks);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "RtlULongLongMult failed: %!STATUS!",
                      status);

        return Now;
    }

    status = RtlULongLongAdd(Now, ticks, &deadline);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "RtlULongLongAdd failed: %!STATUS!",
                      status);

        return Now;
    }

    return deadline;
}

/**
 * \brief Moves a call from its body to its cleanup: what runs from here on
 * runs under the cleanup allowance, and the call ends with the given status.
 *
 * \param[in,out] Call The call.
 * \param[in] AbortStatus The status the call ends with.
 * \param[in] Now The interrupt time.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptBeginCleanup(
    _Inout_ PKPH_SCRIPT_CALL Call,
    _In_ NTSTATUS AbortStatus,
    _In_ ULONG64 Now
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Call->Phase == KphScriptPhaseBody);

    Call->Phase = KphScriptPhaseCleanup;
    Call->Aborted = TRUE;
    Call->AbortStatus = AbortStatus;
    Call->Instructions = 0;
    Call->Deadline = KphpScriptDeadline(Now, Call->Limits->CleanupMilliseconds);

    KphLuaReleaseReserve(Call->Allocator);
}

/**
 * \brief Adds to the instructions a call has used in its phase.
 *
 * \param[in,out] Call The call.
 * \param[in] Count The instructions to add.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptAddInstructions(
    _Inout_ PKPH_SCRIPT_CALL Call,
    _In_ ULONG64 Count
    )
{
    NTSTATUS status;
    ULONG64 instructions;

    KPH_PAGED_CODE_PASSIVE();

    status = RtlULongLongAdd(Call->Instructions, Count, &instructions);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "RtlULongLongAdd failed: %!STATUS!",
                      status);

        instructions = ULONG64_MAX;
    }

    Call->Instructions = instructions;
}

/**
 * \brief Counts instructions and checks the clock for the body of a call.
 *
 * \param[in,out] Call The call.
 * \param[in] Now The interrupt time.
 * \param[in] Count The instructions to count.
 *
 * \return TRUE if the body reached a limit and must end, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpScriptCountBody(
    _Inout_ PKPH_SCRIPT_CALL Call,
    _In_ ULONG64 Now,
    _In_ ULONG64 Count
    )
{
    BOOLEAN reached;

    KPH_PAGED_CODE_PASSIVE();

    KphpScriptAddInstructions(Call, Count);

    reached = FALSE;
    if (Call->Instructions >= Call->Limits->Instructions)
    {
        reached = TRUE;
    }
    if (Now >= Call->Deadline)
    {
        reached = TRUE;
    }

    if (!reached)
    {
        return FALSE;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  SCRIPT,
                  "Script %Z reached a limit",
                  Call->Script);

    KphpScriptBeginCleanup(Call, STATUS_SI_SCRIPT_LIMIT, Now);

    return TRUE;
}

/**
 * \brief Counts instructions and checks the clock for the cleanup of a call.
 *
 * \param[in,out] Call The call.
 * \param[in] Now The interrupt time.
 * \param[in] Count The instructions to count.
 *
 * \return TRUE if the cleanup used up its allowance and no further script
 * code may run, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpScriptCountCleanup(
    _Inout_ PKPH_SCRIPT_CALL Call,
    _In_ ULONG64 Now,
    _In_ ULONG64 Count
    )
{
    BOOLEAN reached;

    KPH_PAGED_CODE_PASSIVE();

    KphpScriptAddInstructions(Call, Count);

    reached = FALSE;
    if (Call->Instructions >= Call->Limits->CleanupInstructions)
    {
        reached = TRUE;
    }
    if (Now >= Call->Deadline)
    {
        reached = TRUE;
    }

    if (!reached)
    {
        return FALSE;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  SCRIPT,
                  "Script %Z used up its cleanup allowance",
                  Call->Script);

    Call->Phase = KphScriptPhaseStopped;

    return TRUE;
}

/**
 * \brief Counts instructions and checks the clock for a call, and ends what
 * is running when it is over its limit.
 *
 * \details The body of a routine that reaches a limit ends with an error. Its
 * to-be-closed variables are then closed under the cleanup allowance. When
 * that is used up too, no further script code runs. A close handler of the
 * script that is running when the body reaches a limit finishes under the
 * cleanup allowance first.
 *
 * \param[in] State The Lua state.
 * \param[in] Count The instructions to count.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptCount(
    _In_ lua_State* State,
    _In_ ULONG64 Count
    )
{
    PKPH_SCRIPT_CALL call;
    ULONG64 now;
    BOOLEAN abort;
    lua_Hook hook;

    KPH_PAGED_CODE_PASSIVE();

    call = KphpScriptGetCall(State);

    now = KeQueryUnbiasedInterruptTime();

    //
    // A memory error is already unwinding the body: what runs is cleanup.
    //
    if ((call->Phase == KphScriptPhaseBody) &&
        (call->Allocator->Failures != 0))
    {
        KphpScriptBeginCleanup(call, STATUS_INSUFFICIENT_RESOURCES, now);
    }

    switch (call->Phase)
    {
        case KphScriptPhaseBody:
        {
            abort = KphpScriptCountBody(call, now, Count);
            if (abort && (call->Handlers != 0))
            {
                NT_ASSERT(call->Phase == KphScriptPhaseCleanup);

                call->AbortDeferred = TRUE;
                abort = KphpScriptCountCleanup(call, now, Count);
            }
            break;
        }
        case KphScriptPhaseCleanup:
        {
            abort = KphpScriptCountCleanup(call, now, Count);
            break;
        }
        case KphScriptPhaseStopped:
        {
            abort = TRUE;
            break;
        }
        DEFAULT_UNREACHABLE;
    }

    if (abort)
    {
        if (call->Phase == KphScriptPhaseStopped)
        {
            //
            // From here on the hook runs before every instruction.
            //
            hook = lua_gethook(State);
            lua_sethook(State, hook, LUA_MASKCOUNT, 1);
        }

        call->Aborts++;

        KphLuaRaiseAbort(State);
    }
}

/**
 * \brief Runs a full garbage collection when the state of a call uses
 * more than three quarters of its memory limit, or when the caller wants one.
 *
 * \details After a collection that leaves the state above three quarters the
 * next runs when use has grown half-way into what is left.
 *
 * \param[in,out] Call The call.
 * \param[in] State The Lua state.
 * \param[in] Always TRUE to collect whatever the state uses.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptCollect(
    _Inout_ PKPH_SCRIPT_CALL Call,
    _In_ lua_State* State,
    _In_ BOOLEAN Always
    )
{
    PKPH_LUA_ALLOCATOR allocator;
    SIZE_T limit;
    SIZE_T used;

    KPH_PAGED_CODE_PASSIVE();

    allocator = Call->Allocator;

    if (!Always && (allocator->Used <= allocator->CollectAt))
    {
        return;
    }

    lua_gc(State, LUA_GCCOLLECT);

    limit = Call->Limits->Memory;
    used = allocator->Used;

    allocator->CollectAt = limit - (limit / 4);

    if ((used > allocator->CollectAt) && (used < limit))
    {
        allocator->CollectAt = used + ((limit - used) / 2);
    }
}

/**
 * \brief Counts the instructions the count hook of a call stands for, and
 * collects when the state is near its memory limit.
 *
 * \param[in] State The Lua state.
 * \param[in] Debug Unused.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptHook(
    _In_ lua_State* State,
    _In_ lua_Debug* Debug
    )
{
    PKPH_SCRIPT_CALL call;

    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Debug);

    KphpScriptCount(State, KPH_SCRIPT_HOOK_INTERVAL);

    call = KphpScriptGetCall(State);

    if ((call->Phase == KphScriptPhaseBody) && (call->Handlers == 0))
    {
        KphpScriptCollect(call, State, FALSE);
    }
}

/**
 * \brief Charges a call for work a library function is about to do outside
 * the count hook. Raises the abort of the call when that is over its limit.
 *
 * \details Inside a close handler of the script the body's limit is raised
 * when the handler returns, and the work counts against the cleanup allowance.
 *
 * \param[in] State The Lua state.
 * \param[in] Count The instructions the work counts as.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptCharge(
    _In_ lua_State* State,
    _In_ ULONG64 Count
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphpScriptCount(State, Count);
}

/**
 * \brief Notes that a close handler of the script begins. In the body of a
 * call the handler has the memory reserve.
 *
 * \param[in] State The Lua state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptEnterHandler(
    _In_ lua_State* State
    )
{
    PKPH_SCRIPT_CALL call;
    SIZE_T limit;

    KPH_PAGED_CODE_PASSIVE();

    call = KphpScriptGetCall(State);

    if ((call->Handlers == 0) && (call->Phase == KphScriptPhaseBody))
    {
        limit = call->Allocator->Limit;

        KphLuaReleaseReserve(call->Allocator);

        call->HandlerReserve = call->Allocator->Limit - limit;
    }

    call->Handlers++;
}

/**
 * \brief Notes that a close handler of the script ended.
 *
 * \details May raise a Lua error: the abort of the call, when the body reached
 * a limit while its handlers ran.
 *
 * \param[in] State The Lua state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphScriptLeaveHandler(
    _In_ lua_State* State
    )
{
    PKPH_SCRIPT_CALL call;

    KPH_PAGED_CODE_PASSIVE();

    call = KphpScriptGetCall(State);

    NT_ASSERT(call->Handlers > 0);

    call->Handlers--;

    if (call->Handlers != 0)
    {
        return;
    }

    if ((call->Phase == KphScriptPhaseBody) &&
        (call->Allocator->Failures == 0))
    {
        KphLuaRestoreReserve(call->Allocator, call->HandlerReserve);
    }

    call->HandlerReserve = 0;

    if (call->AbortDeferred)
    {
        call->AbortDeferred = FALSE;
        call->Aborts++;

        KphLuaRaiseAbort(State);
    }
}

/**
 * \brief Tells whether the time of the phase a call is in is up.
 *
 * \details The time of the body running out inside a close handler of the
 * script begins the cleanup, whose time the handler then has.
 *
 * \param[in,out] Call The call.
 * \param[in] Now The interrupt time.
 *
 * \return TRUE if the time is up, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpScriptTimeIsUp(
    _Inout_ PKPH_SCRIPT_CALL Call,
    _In_ ULONG64 Now
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Now < Call->Deadline)
    {
        return FALSE;
    }

    if ((Call->Phase != KphScriptPhaseBody) || (Call->Handlers == 0))
    {
        return TRUE;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  SCRIPT,
                  "Script %Z reached a limit",
                  Call->Script);

    KphpScriptBeginCleanup(Call, STATUS_SI_SCRIPT_LIMIT, Now);

    Call->AbortDeferred = TRUE;

    return (Now >= Call->Deadline);
}

/**
 * \brief Waits for an object, or for a time, no longer than the current
 * phase of the call may still run.
 *
 * \details Does not raise. At the deadline the caller releases what it must
 * and then charges the call, which raises.
 *
 * \param[in] State The Lua state.
 * \param[in] Object The object to wait for, NULL to wait for the time only.
 * \param[in] Milliseconds The caller's own timeout, ULONG_MAX for none.
 *
 * \return Successful or errant status, STATUS_TIMEOUT when the caller's
 * timeout passed, STATUS_SI_SCRIPT_LIMIT when the call's time is up.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphScriptWait(
    _In_ lua_State* State,
    _In_opt_ PVOID Object,
    _In_ ULONG Milliseconds
    )
{
    NTSTATUS status;
    PKPH_SCRIPT_CALL call;
    ULONG64 now;
    ULONG64 end;
    ULONG64 until;
    LARGE_INTEGER timeout;

    KPH_PAGED_CODE_PASSIVE();

    call = KphpScriptGetCall(State);

    if (call->Phase == KphScriptPhaseStopped)
    {
        return STATUS_SI_SCRIPT_LIMIT;
    }

    now = KeQueryUnbiasedInterruptTime();

    end = ULONG64_MAX;
    if (Milliseconds != ULONG_MAX)
    {
        end = KphpScriptDeadline(now, Milliseconds);
    }

    if (KphpScriptTimeIsUp(call, now))
    {
        return STATUS_SI_SCRIPT_LIMIT;
    }

    for (;;)
    {
        until = min(call->Deadline, end);

        timeout.QuadPart = -(LONG64)(until - now);

        if (Object)
        {
            status = KeWaitForSingleObject(Object,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           &timeout);
            if (status == STATUS_SUCCESS)
            {
                return STATUS_SUCCESS;
            }
        }
        else
        {
            status = KeDelayExecutionThread(KernelMode, FALSE, &timeout);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              SCRIPT,
                              "KeDelayExecutionThread failed: %!STATUS!",
                              status);

                return status;
            }
        }

        now = KeQueryUnbiasedInterruptTime();

        if (KphpScriptTimeIsUp(call, now))
        {
            return STATUS_SI_SCRIPT_LIMIT;
        }

        if (now >= end)
        {
            return STATUS_TIMEOUT;
        }
    }
}

/**
 * \brief Takes the status a routine returned.
 *
 * \param[in] State The Lua state, with the status at the given index.
 * \param[in] Index The stack index of the status.
 * \param[out] RoutineStatus Receives the status the routine returned.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpScriptTakeStatus(
    _In_ lua_State* State,
    _In_ int Index,
    _Out_ PNTSTATUS RoutineStatus
    )
{
    lua_Integer value;
    int type;
    PCSTR typeName;

    KPH_PAGED_CODE_PASSIVE();

    *RoutineStatus = STATUS_UNSUCCESSFUL;

    if (!lua_isinteger(State, Index))
    {
        type = lua_type(State, Index);
        typeName = lua_typename(State, type);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Routine returned a %hs for its status",
                      typeName);

        return STATUS_SI_SCRIPT_ERROR;
    }

    value = lua_tointegerx(State, Index, NULL);

    if ((value < LONG_MIN) || (value > LONG_MAX))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Routine returned a status out of range");

        return STATUS_SI_SCRIPT_ERROR;
    }

    *RoutineStatus = (NTSTATUS)value;

    return STATUS_SUCCESS;
}

/**
 * \brief Copies the output a routine returned to the caller.
 *
 * \param[in] State The Lua state, with the output at the given index.
 * \param[in] Index The stack index of the output.
 * \param[in,out] Call The call, which receives the output and its length.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpScriptTakeOutput(
    _In_ lua_State* State,
    _In_ int Index,
    _Inout_ PKPH_SCRIPT_CALL Call
    )
{
    NTSTATUS status;
    int type;
    PCSTR typeName;
    PCSTR output;
    SIZE_T length;

    KPH_PAGED_CODE_PASSIVE();

    type = lua_type(State, Index);

    if (type == LUA_TNIL)
    {
        return STATUS_SUCCESS;
    }

    if (type != LUA_TSTRING)
    {
        typeName = lua_typename(State, type);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Routine returned a %hs for its output",
                      typeName);

        return STATUS_SI_SCRIPT_ERROR;
    }

    output = lua_tolstring(State, Index, &length);

    if (length > ULONG_MAX)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Routine returned an output over the maximum length");

        return STATUS_SI_SCRIPT_ERROR;
    }

    Call->ReturnLength = (ULONG)length;

    if (length > Call->OutputLength)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Output of %lu bytes does not fit %lu",
                      (ULONG)length,
                      Call->OutputLength);

        return STATUS_BUFFER_TOO_SMALL;
    }

    if (length == 0)
    {
        return STATUS_SUCCESS;
    }

    NT_ASSERT(Call->Output);

    status = KphCopyToMode(Call->Output, output, length, Call->AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphCopyToMode failed: %!STATUS!",
                      status);

        Call->ReturnLength = 0;

        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Loads the script of a call and runs its top-level code. A Lua error
 * ends it.
 *
 * \param[in] State The Lua state.
 * \param[in,out] Context The call.
 */
_Function_class_(KPH_LUA_PROTECTED_CALL_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpScriptProtectedLoad(
    _In_ lua_State* State,
    _Inout_opt_ PVOID Context
    )
{
    PKPH_SCRIPT_CALL call;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    call = Context;

    KphLuaOpenLibraries(State);

    KphpScriptOpenCleanup(State);

    lua_sethook(State,
                KphpScriptHook,
                LUA_MASKCOUNT,
                KPH_SCRIPT_HOOK_INTERVAL);

    KphLuaLoadChunk(State, call->Chunk, call->ChunkLength, call->Script);

    lua_call(State, 0, 0);
}

/**
 * \brief Calls the routine of a call and takes the results. A Lua error ends
 * it.
 *
 * \param[in] State The Lua state, with the script loaded.
 * \param[in,out] Context The call.
 */
_Function_class_(KPH_LUA_PROTECTED_CALL_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpScriptProtectedRoutine(
    _In_ lua_State* State,
    _Inout_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PKPH_SCRIPT_CALL call;
    NTSTATUS routineStatus;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    call = Context;

    //
    // A routine is a global function of the script.
    //
    lua_pushglobaltable(State);
    lua_pushlstring(State, call->Routine->Buffer, call->Routine->Length);

    if (lua_rawget(State, -2) != LUA_TFUNCTION)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z has no routine %Z",
                      call->Script,
                      call->Routine);

        call->Status = STATUS_NOT_FOUND;
        return;
    }

    if (call->InputLength > 0)
    {
        lua_pushlstring(State, call->Input, call->InputLength);
    }
    else
    {
        lua_pushliteral(State, "");
    }

    lua_call(State, 1, 2);

    status = KphpScriptTakeStatus(State, -2, &routineStatus);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphpScriptTakeStatus failed: %!STATUS!",
                      status);

        call->Status = status;
        return;
    }

    status = KphpScriptTakeOutput(State, -1, call);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphpScriptTakeOutput failed: %!STATUS!",
                      status);

        call->Status = status;
        return;
    }

    call->Status = routineStatus;
}

/**
 * \brief Does what the end of a call does: takes a failed protected call's
 * status, the abort checks, the cleanups.
 *
 * \param[in,out] Call The call.
 * \param[in] Status The status of the protected call.
 * \param[in] State The Lua state of the call when it outlives the call, NULL
 * otherwise.
 *
 * \return How the call ended: cleanly, with a Lua error, or at a limit.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
KPH_SCRIPT_END KphpScriptEndCall(
    _Inout_ PKPH_SCRIPT_CALL Call,
    _In_ NTSTATUS Status,
    _In_opt_ lua_State* State
    )
{
    KPH_SCRIPT_END end;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT((Call->Handlers == 0) && !Call->AbortDeferred);

    if (Call->Aborted ||
        (Call->Allocator->Failures != 0) ||
        (Call->Phase != KphScriptPhaseBody))
    {
        end = KphScriptEndFatal;
    }
    else if (!NT_SUCCESS(Status))
    {
        end = KphScriptEndError;
    }
    else
    {
        end = KphScriptEndClean;
    }

    if (!NT_SUCCESS(Status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphLuaProtectedCall failed: %!STATUS!",
                      Status);

        Call->Status = Status;
    }

    if (!Call->Aborted && (Call->Allocator->Failures != 0))
    {
        Call->Aborted = TRUE;
        Call->AbortStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Call->Aborted)
    {
        Call->Status = Call->AbortStatus;
        Call->ReturnLength = 0;
    }

    if (Call->Phase == KphScriptPhaseStopped)
    {
        Call->Status = STATUS_SI_SCRIPT_CLEANUP_LIMIT;
        Call->ReturnLength = 0;
    }

    KphpScriptRunCleanups(Call, State);

    return end;
}

/**
 * \brief Runs a call in a Lua state of its own.
 *
 * \param[in,out] Context The call.
 */
_Function_class_(KPH_LUA_CALLOUT_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpScriptCallout(
    _Inout_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PKPH_SCRIPT_CALL call;
    lua_State* state;
    PKPH_SCRIPT_CALL* slot;
    ULONG64 now;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    call = Context;

    status = KphLuaCreateState(call->Allocator, &state);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphLuaCreateState failed: %!STATUS!",
                      status);

        call->Status = status;
        return;
    }

    slot = lua_getextraspace(state);
    *slot = call;

    now = KeQueryUnbiasedInterruptTime();

    call->Deadline = KphpScriptDeadline(now, call->Limits->Milliseconds);

    status = KphLuaProtectedCall(state, KphpScriptProtectedLoad, call);
    if (NT_SUCCESS(status))
    {
        status = KphLuaProtectedCall(state, KphpScriptProtectedRoutine, call);
    }

    KphpScriptEndCall(call, status, NULL);

    KphLuaCloseState(state);
}

/**
 * \brief Creates the kept state of a non-volatile script and runs its
 * top-level code in it. The state is kept only when the load ended cleanly.
 *
 * \param[in,out] Context The kept call.
 */
_Function_class_(KPH_LUA_CALLOUT_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpScriptLoadCallout(
    _Inout_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PKPH_SCRIPT_KEPT_CALL kept;
    PKPH_SCRIPT_CALL call;
    lua_State* state;
    PKPH_SCRIPT_CALL* slot;
    ULONG64 now;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    kept = Context;
    call = kept->Call;

    status = KphLuaCreateState(call->Allocator, &state);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphLuaCreateState failed: %!STATUS!",
                      status);

        call->Status = status;
        return;
    }

    slot = lua_getextraspace(state);
    *slot = call;

    now = KeQueryUnbiasedInterruptTime();

    call->Deadline = KphpScriptDeadline(now, call->Limits->Milliseconds);

    status = KphLuaProtectedCall(state, KphpScriptProtectedLoad, call);

    kept->End = KphpScriptEndCall(call, status, state);

    *slot = NULL;
    lua_settop(state, 0);

    if (kept->End != KphScriptEndClean)
    {
        KphpScriptRunObjects(kept->Script);
        KphLuaCloseState(state);
        return;
    }

    KphpScriptCollect(call, state, call->Opened);

    kept->Script->State = state;
}

/**
 * \brief Runs a call in the kept state of a non-volatile script.
 *
 * \param[in,out] Context The kept call.
 */
_Function_class_(KPH_LUA_CALLOUT_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpScriptRoutineCallout(
    _Inout_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PKPH_SCRIPT_KEPT_CALL kept;
    PKPH_SCRIPT_CALL call;
    lua_State* state;
    PKPH_SCRIPT_CALL* slot;
    ULONG64 now;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    kept = Context;
    call = kept->Call;
    state = kept->Script->State;

    slot = lua_getextraspace(state);
    *slot = call;

    now = KeQueryUnbiasedInterruptTime();

    call->Deadline = KphpScriptDeadline(now, call->Limits->Milliseconds);

    status = KphLuaProtectedCall(state, KphpScriptProtectedRoutine, call);

    kept->End = KphpScriptEndCall(call, status, state);

    *slot = NULL;
    lua_settop(state, 0);

    if (kept->End != KphScriptEndFatal)
    {
        KphpScriptCollect(call, state, call->Opened);
    }
}

/**
 * \brief Closes the kept state of a non-volatile script.
 *
 * \param[in,out] Context The script.
 */
_Function_class_(KPH_LUA_CALLOUT_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpScriptCloseCallout(
    _Inout_opt_ PVOID Context
    )
{
    PKPH_SCRIPT script;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    script = Context;

    KphLuaCloseState(script->State);

    script->State = NULL;
}

/**
 * \brief Prepares a call.
 *
 * \param[out] Call The call.
 * \param[in] Allocator The allocator of the call's state.
 * \param[in] Limits The limits of the call.
 * \param[in] Chunk The precompiled script.
 * \param[in] ChunkLength The length of the script in bytes.
 * \param[in] Script The name of the script.
 * \param[in] Routine The name of the routine, NULL for a load.
 * \param[in] Input The input for the routine.
 * \param[in] InputLength The length of the input in bytes.
 * \param[out] Output Receives the output of the routine.
 * \param[in] OutputLength The length of the output buffer in bytes.
 * \param[in] Volatile Whether the script is volatile.
 * \param[in] AccessMode The mode in which to write the output.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptInitializeCall(
    _Out_ PKPH_SCRIPT_CALL Call,
    _In_ PKPH_LUA_ALLOCATOR Allocator,
    _In_ PKPH_SCRIPT_LIMITS Limits,
    _In_reads_bytes_(ChunkLength) PCVOID Chunk,
    _In_ ULONG ChunkLength,
    _In_ PCANSI_STRING Script,
    _In_opt_ PCANSI_STRING Routine,
    _In_reads_bytes_opt_(InputLength) PCVOID Input,
    _In_ ULONG InputLength,
    _Out_writes_bytes_opt_(OutputLength) PVOID Output,
    _In_ ULONG OutputLength,
    _In_ BOOLEAN Volatile,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE_PASSIVE();

    Call->Allocator = Allocator;
    Call->Limits = Limits;
    Call->Chunk = Chunk;
    Call->ChunkLength = ChunkLength;
    Call->Script = Script;
    Call->Routine = Routine;
    Call->Input = Input;
    Call->InputLength = InputLength;
    Call->Output = Output;
    Call->OutputLength = OutputLength;
    Call->ReturnLength = 0;
    Call->AccessMode = AccessMode;
    Call->Volatile = Volatile;
    InitializeListHead(&Call->Cleanups);
    Call->Objects = &Call->Cleanups;
    Call->Opened = FALSE;
    Call->Phase = KphScriptPhaseBody;
    Call->Instructions = 0;
    Call->Deadline = ULONG64_MAX;
    Call->Aborts = 0;
    Call->Aborted = FALSE;
    Call->AbortStatus = STATUS_SUCCESS;
    Call->Handlers = 0;
    Call->HandlerReserve = 0;
    Call->AbortDeferred = FALSE;
    Call->Status = STATUS_UNSUCCESSFUL;
}

/**
 * \brief Calls a routine of a precompiled script.
 *
 * \param[in] Chunk The precompiled script.
 * \param[in] ChunkLength The length of the script in bytes.
 * \param[in] Script The name of the script.
 * \param[in] Routine The name of the routine.
 * \param[in] Input The input for the routine.
 * \param[in] InputLength The length of the input in bytes.
 * \param[out] Output Receives the output of the routine.
 * \param[in] OutputLength The length of the output buffer in bytes.
 * \param[out] ReturnLength Receives the length of the output of the routine.
 * \param[in] Limits The limits of the call.
 * \param[in] AccessMode The mode in which to write the output.
 *
 * \return The status the routine returned, or an errant status when the
 * routine could not be called, failed or reached a limit.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpScriptRunChunk(
    _In_reads_bytes_(ChunkLength) PCVOID Chunk,
    _In_ ULONG ChunkLength,
    _In_ PCANSI_STRING Script,
    _In_ PCANSI_STRING Routine,
    _In_reads_bytes_opt_(InputLength) PCVOID Input,
    _In_ ULONG InputLength,
    _Out_writes_bytes_to_opt_(OutputLength, *ReturnLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_ PULONG ReturnLength,
    _In_ PKPH_SCRIPT_LIMITS Limits,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    KPH_SCRIPT_CALL call;
    KPH_LUA_ALLOCATOR allocator;

    KPH_PAGED_CODE_PASSIVE();

    *ReturnLength = 0;

    KphLuaInitializeAllocator(&allocator,
                              Limits->Memory,
                              Limits->CleanupMemory);

    KphpScriptInitializeCall(&call,
                             &allocator,
                             Limits,
                             Chunk,
                             ChunkLength,
                             Script,
                             Routine,
                             Input,
                             InputLength,
                             Output,
                             OutputLength,
                             TRUE,
                             AccessMode);

    KeEnterCriticalRegion();

    status = KphLuaCallout(KphpScriptCallout, &call);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphLuaCallout failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = call.Status;
    *ReturnLength = call.ReturnLength;

Exit:

    KphpScriptRunCleanups(&call, NULL);

    KeLeaveCriticalRegion();

    KphLuaDeleteAllocator(&allocator);

    return status;
}

/**
 * \brief Builds the kept state of a non-volatile script and runs its
 * top-level code, as the start of a call does.
 *
 * \param[in,out] Script The script object.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpScriptLoadKept(
    _Inout_ PKPH_SCRIPT Script
    )
{
    NTSTATUS status;
    PKPH_SCRIPT_FILE file;
    KPH_SCRIPT_CALL call;
    KPH_SCRIPT_KEPT_CALL kept;

    KPH_PAGED_CODE_PASSIVE();

    file = Script->File;

    KphLuaInitializeAllocator(&Script->Allocator,
                              file->Limits.Memory,
                              file->Limits.CleanupMemory);

    KphpScriptInitializeCall(&call,
                             &Script->Allocator,
                             &file->Limits,
                             Add2Ptr(file, file->ChunkOffset),
                             file->ChunkLength,
                             &Script->Name,
                             NULL,
                             NULL,
                             0,
                             NULL,
                             0,
                             FALSE,
                             KernelMode);

    call.Objects = &Script->Objects;

    kept.Script = Script;
    kept.Call = &call;
    kept.End = KphScriptEndFatal;

    KeEnterCriticalRegion();

    status = KphLuaCallout(KphpScriptLoadCallout, &kept);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphLuaCallout failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (kept.End != KphScriptEndClean)
    {
        status = call.Status;

        NT_ASSERT(!NT_SUCCESS(status));

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z did not load cleanly: %!STATUS!",
                      &Script->Name,
                      status);

        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    KphpScriptRunCleanups(&call, NULL);

    KeLeaveCriticalRegion();

    if (!Script->State)
    {
        KphpScriptRunObjects(Script);

        KphLuaDeleteAllocator(&Script->Allocator);
    }

    return status;
}

/**
 * \brief Calls a routine of a non-volatile script in its kept state.
 *
 * \param[in,out] Script The script object.
 * \param[in] Routine The name of the routine.
 * \param[in] Input The input for the routine.
 * \param[in] InputLength The length of the input in bytes.
 * \param[out] Output Receives the output of the routine.
 * \param[in] OutputLength The length of the output buffer in bytes.
 * \param[out] ReturnLength Receives the length of the output of the routine.
 * \param[in] AccessMode The mode in which to write the output.
 *
 * \return The status the routine returned, or an errant status when the
 * routine could not be called, failed or reached a limit, and
 * STATUS_SI_SCRIPT_FAULTED once a call did not end cleanly.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpScriptRunKept(
    _Inout_ PKPH_SCRIPT Script,
    _In_ PCANSI_STRING Routine,
    _In_reads_bytes_opt_(InputLength) PCVOID Input,
    _In_ ULONG InputLength,
    _Out_writes_bytes_to_opt_(OutputLength, *ReturnLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_SCRIPT_FILE file;
    KPH_SCRIPT_CALL call;
    KPH_SCRIPT_KEPT_CALL kept;

    KPH_PAGED_CODE_PASSIVE();

    *ReturnLength = 0;

    file = Script->File;

    KphpScriptInitializeCall(&call,
                             &Script->Allocator,
                             &file->Limits,
                             Add2Ptr(file, file->ChunkOffset),
                             file->ChunkLength,
                             &Script->Name,
                             Routine,
                             Input,
                             InputLength,
                             Output,
                             OutputLength,
                             FALSE,
                             AccessMode);

    call.Objects = &Script->Objects;

    kept.Script = Script;
    kept.Call = &call;
    kept.End = KphScriptEndFatal;

    KeEnterCriticalRegion();
    KphAcquireRWLockExclusive(&Script->Lock);

    if (Script->Faulted)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z is faulted",
                      &Script->Name);

        status = STATUS_SI_SCRIPT_FAULTED;
        goto Exit;
    }

    status = KphLuaCallout(KphpScriptRoutineCallout, &kept);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphLuaCallout failed: %!STATUS!",
                      status);

        Script->Faulted = TRUE;
        goto Exit;
    }

    if (kept.End == KphScriptEndFatal)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z faulted: %!STATUS!",
                      &Script->Name,
                      call.Status);

        Script->Faulted = TRUE;
    }

    status = call.Status;
    *ReturnLength = call.ReturnLength;

Exit:

    KphpScriptRunCleanups(&call, NULL);

    if (Script->Faulted && Script->State)
    {
        KphpScriptRunObjects(Script);

        KphLuaDeleteAllocator(&Script->Allocator);
        Script->State = NULL;
    }

    KphReleaseRWLock(&Script->Lock);
    KeLeaveCriticalRegion();

    return status;
}

/**
 * \brief Retrieves the process state a script level requires of a caller.
 *
 * \param[in] Level The level of a script or of a routine.
 * \param[out] RequiredState Receives the process state the level requires.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpScriptRequiredState(
    _In_ ULONG Level,
    _Out_ PKPH_PROCESS_STATE RequiredState
    )
{
    KPH_PAGED_CODE_PASSIVE();

    *RequiredState = KPH_PROCESS_STATE_MAXIMUM;

    switch (Level)
    {
        case KPH_SCRIPT_LEVEL_LOW:
        {
            *RequiredState = KPH_PROCESS_STATE_LOW;
            break;
        }
        case KPH_SCRIPT_LEVEL_MEDIUM:
        {
            *RequiredState = KPH_PROCESS_STATE_MEDIUM;
            break;
        }
        case KPH_SCRIPT_LEVEL_HIGH:
        {
            *RequiredState = KPH_PROCESS_STATE_HIGH;
            break;
        }
        case KPH_SCRIPT_LEVEL_MAXIMUM:
        {
            *RequiredState = KPH_PROCESS_STATE_MAXIMUM;
            break;
        }
        default:
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          SCRIPT,
                          "Unknown script level: %lu",
                          Level);

            return STATUS_ACCESS_DENIED;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Captures the input of a call.
 *
 * \param[in] Input The input.
 * \param[in] InputLength The length of the input in bytes.
 * \param[in] Limits The limits of the call.
 * \param[in] AccessMode The mode in which to perform access checks.
 * \param[out] CapturedInput Receives the captured input, NULL when there is
 * none, to be freed with the tag KPH_TAG_SCRIPT_INPUT.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpScriptCaptureInput(
    _In_reads_bytes_opt_(InputLength) PVOID Input,
    _In_ ULONG InputLength,
    _In_ PKPH_SCRIPT_LIMITS Limits,
    _In_ KPROCESSOR_MODE AccessMode,
    _Outptr_result_maybenull_ PVOID* CapturedInput
    )
{
    NTSTATUS status;
    PVOID input;

    KPH_PAGED_CODE_PASSIVE();

    *CapturedInput = NULL;

    input = NULL;

    if (InputLength == 0)
    {
        status = STATUS_SUCCESS;
        goto Exit;
    }

    if (!Input)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script input length without an input");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // An input over the memory limit could never be handed to the script.
    //
    if (InputLength > Limits->Memory)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script input of %lu bytes is over the memory limit",
                      InputLength);

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    input = KphAllocatePaged(InputLength, KPH_TAG_SCRIPT_INPUT);
    if (!input)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Failed to allocate script input");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphCopyFromMode(input, Input, InputLength, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphCopyFromMode failed: %!STATUS!",
                      status);

        goto Exit;
    }

    *CapturedInput = input;
    input = NULL;

Exit:

    if (input)
    {
        KphFree(input, KPH_TAG_SCRIPT_INPUT);
    }

    return status;
}

/**
 * \brief Checks that a span of a script file lies within it.
 *
 * \param[in] FileLength The length of the script file.
 * \param[in] Offset The offset of the span from the start of the file.
 * \param[in] Length The length of the span.
 *
 * \return TRUE if the span lies within the file, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpScriptFileSpan(
    _In_ ULONG FileLength,
    _In_ ULONG Offset,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    ULONG end;

    KPH_PAGED_CODE_PASSIVE();

    status = RtlULongAdd(Offset, Length, &end);
    if (!NT_SUCCESS(status) || (end > FileLength))
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief Compares the name of a routine of a script file with a name.
 *
 * \param[in] File The script file.
 * \param[in] Routine The routine.
 * \param[in] Name The name to compare with.
 *
 * \return Negative if the routine orders before the name, zero if they are
 * equal, positive otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
LONG KphpScriptCompareRoutine(
    _In_ PKPH_SCRIPT_FILE File,
    _In_ PKPH_SCRIPT_ROUTINE Routine,
    _In_ PCANSI_STRING Name
    )
{
    ANSI_STRING name;

    KPH_PAGED_CODE_PASSIVE();

    name.Buffer = Add2Ptr(File, Routine->NameOffset);
    name.Length = Routine->NameLength;
    name.MaximumLength = Routine->NameLength;

    return RtlCompareString(&name, Name, FALSE);
}

/**
 * \brief Validates a script file.
 *
 * \param[in] File The script file.
 * \param[in] Length The length of the script file in bytes.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpScriptValidate(
    _In_reads_bytes_(Length) PKPH_SCRIPT_FILE File,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    KPH_PROCESS_STATE state;
    PKPH_SCRIPT_ROUTINE routines;
    PKPH_SCRIPT_ROUTINE routine;
    ANSI_STRING name;
    ULONG length;

    KPH_PAGED_CODE_PASSIVE();

    if ((Length < sizeof(KPH_SCRIPT_FILE)) ||
        (File->Magic != KPH_SCRIPT_FILE_MAGIC))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script file header is not valid");

        return STATUS_INVALID_PARAMETER;
    }

    if (File->Version != KPH_SCRIPT_VERSION)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script file version %lu is not %lu",
                      File->Version,
                      KPH_SCRIPT_VERSION);

        return STATUS_REVISION_MISMATCH;
    }

    if (File->Length != Length)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script file header is not valid");

        return STATUS_INVALID_PARAMETER;
    }

    if ((File->NameLength == 0) ||
        !KphpScriptFileSpan(Length, File->NameOffset, File->NameLength))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script file name is not valid");

        return STATUS_INVALID_PARAMETER;
    }

    NT_VERIFY(NT_SUCCESS(RtlULongMult(File->RoutineCount,
                                      sizeof(KPH_SCRIPT_ROUTINE),
                                      &length)));

    if ((File->RoutineCount == 0) ||
        !KphpScriptFileSpan(Length, File->RoutinesOffset, length))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script file routine table is not valid");

        return STATUS_INVALID_PARAMETER;
    }

    status = KphpScriptRequiredState(File->Level, &state);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphpScriptRequiredState failed: %!STATUS!",
                      status);

        return status;
    }

    routines = Add2Ptr(File, File->RoutinesOffset);

    for (ULONG i = 0; i < File->RoutineCount; i++)
    {
        routine = &routines[i];

        if ((routine->NameLength == 0) ||
            !KphpScriptFileSpan(Length,
                                routine->NameOffset,
                                routine->NameLength))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          SCRIPT,
                          "Script file routine %lu name is not valid",
                          i);

            return STATUS_INVALID_PARAMETER;
        }

        status = KphpScriptRequiredState(routine->Level, &state);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          SCRIPT,
                          "Script file routine %lu level is not valid: "
                          "%!STATUS!",
                          i,
                          status);

            return status;
        }

        if (routine->Level < File->Level)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          SCRIPT,
                          "Script file routine %lu level is not valid",
                          i);

            return STATUS_INVALID_PARAMETER;
        }

        if (i == 0)
        {
            continue;
        }

        name.Buffer = Add2Ptr(File, routine->NameOffset);
        name.Length = routine->NameLength;
        name.MaximumLength = routine->NameLength;

        if (KphpScriptCompareRoutine(File, &routines[i - 1], &name) >= 0)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          SCRIPT,
                          "Script file routine %lu is out of order",
                          i);

            return STATUS_INVALID_PARAMETER;
        }
    }

    if ((File->ChunkLength == 0) ||
        !KphpScriptFileSpan(Length, File->ChunkOffset, File->ChunkLength))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script file chunk is not valid");

        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Searches the routine table of a script file.
 *
 * \param[in] File The script file, validated.
 * \param[in] Name The name of the routine.
 *
 * \return The routine, NULL when the file has no routine of that name.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PKPH_SCRIPT_ROUTINE KphpScriptLookupRoutine(
    _In_ PKPH_SCRIPT_FILE File,
    _In_ PCANSI_STRING Name
    )
{
    PKPH_SCRIPT_ROUTINE routines;
    PKPH_SCRIPT_ROUTINE routine;
    ULONG low;
    ULONG high;
    ULONG middle;
    LONG order;

    KPH_PAGED_CODE_PASSIVE();

    routines = Add2Ptr(File, File->RoutinesOffset);

    low = 0;
    high = File->RoutineCount;

    while (low < high)
    {
        middle = low + ((high - low) / 2);

        routine = &routines[middle];

        order = KphpScriptCompareRoutine(File, routine, Name);
        if (order < 0)
        {
            low = middle + 1;
            continue;
        }

        if (order > 0)
        {
            high = middle;
            continue;
        }

        return routine;
    }

    return NULL;
}

/**
 * \brief Allocates a script object.
 *
 * \param[in] Size The size of the object to allocate.
 *
 * \return Pointer to the allocated object, NULL on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateScript(
    _In_ SIZE_T Size
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphAllocateNPaged(Size, KPH_TAG_SCRIPT_FILE);
}

/**
 * \brief Initializes a script object.
 *
 * \param[in,out] Object The script object to initialize.
 * \param[in] Parameter The script file, validated, which the object takes.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitializeScript(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_SCRIPT script;
    PKPH_SCRIPT_FILE file;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Parameter);

    script = Object;
    file = Parameter;

    script->File = file;
    script->Name.Buffer = Add2Ptr(file, file->NameOffset);
    script->Name.Length = file->NameLength;
    script->Name.MaximumLength = file->NameLength;
    KphInitializeRWLock(&script->Lock);
    InitializeListHead(&script->Objects);

    KphAcquireRWLockExclusive(&KphpScriptsLock);
    InsertTailList(&KphpScripts, &script->Entry);
    KphReleaseRWLock(&KphpScriptsLock);

    return STATUS_SUCCESS;
}

/**
 * \brief Runs down a script object, releasing the objects and the state it
 * holds.
 *
 * \details The caller holds the lock of the list of script objects and has
 * unlinked the script object.
 *
 * \param[in,out] Script The script object.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRundownScript(
    _Inout_ PKPH_SCRIPT Script
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    KphpScriptRunObjects(Script);

    if (!Script->State)
    {
        return;
    }

    status = KphLuaCallout(KphpScriptCloseCallout, Script);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphLuaCallout failed: %!STATUS!",
                      status);
    }

    KphLuaDeleteAllocator(&Script->Allocator);
    Script->State = NULL;
}

/**
 * \brief Deletes a script object.
 *
 * \details May run after KphCleanupScript, which has then run the script
 * object down.
 *
 * \param[in] Object The script object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpDeleteScript(
    _In_freesMem_ PVOID Object
    )
{
    PKPH_SCRIPT script;

    KPH_PAGED_CODE_PASSIVE();

    script = Object;

    KphAcquireRWLockExclusive(&KphpScriptsLock);
    if (!IsListEmpty(&script->Entry))
    {
        RemoveEntryList(&script->Entry);
        KphpRundownScript(script);
    }
    KphReleaseRWLock(&KphpScriptsLock);

    KphDeleteRWLock(&script->Lock);

    KphFree(script->File, KPH_TAG_SCRIPT_FILE);
}

/**
 * \brief Frees a script object.
 *
 * \param[in] Object The script object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpFreeScript(
    _In_freesMem_ PVOID Object
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphFree(Object, KPH_TAG_SCRIPT_FILE);
}

/**
 * \brief Creates a script object from a signed script file.
 *
 * \param[in] Script The script file.
 * \param[in] ScriptLength The length of the script file in bytes.
 * \param[in] Signature The signature of the script file.
 * \param[in] SignatureLength The length of the signature in bytes.
 * \param[in] Flags KPH_SCRIPT_VOLATILE or KPH_SCRIPT_NON_VOLATILE.
 * \param[out] ScriptHandle Receives the handle to the script object.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCreateScript(
    _In_reads_bytes_(ScriptLength) PVOID Script,
    _In_ ULONG ScriptLength,
    _In_reads_bytes_(SignatureLength) PVOID Signature,
    _In_ ULONG SignatureLength,
    _In_ ULONG Flags,
    _Out_ PHANDLE ScriptHandle,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_PROCESS_CONTEXT process;
    PKPH_SCRIPT_FILE file;
    PVOID signature;
    PKPH_DYN dyn;
    KPH_PROCESS_STATE requiredState;
    PKPH_SCRIPT script;
    HANDLE scriptHandle;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphAcquireRundown(&KphpScriptRundown))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Failed to acquire rundown.");

        return STATUS_TOO_LATE;
    }

    file = NULL;
    signature = NULL;
    dyn = NULL;
    script = NULL;
    scriptHandle = NULL;

    process = KphGetProcessContextForMode(AccessMode);
    if (!process)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphGetProcessContextForMode failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (Flags & ~KPH_SCRIPT_VOLATILE)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script flags 0x%08lx are not valid",
                      Flags);

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!Script || !ScriptLength || !Signature || !SignatureLength)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script or signature is empty");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    file = KphAllocatePaged(ScriptLength, KPH_TAG_SCRIPT_FILE);
    if (!file)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Failed to allocate script file");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    signature = KphAllocatePaged(SignatureLength, KPH_TAG_SCRIPT_FILE);
    if (!signature)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Failed to allocate script signature");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphCopyFromMode(file, Script, ScriptLength, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphCopyFromMode failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphCopyFromMode(signature, Signature, SignatureLength, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphCopyFromMode failed: %!STATUS!",
                      status);

        goto Exit;
    }

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->ScriptPublicKeyHandle)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "No script key is active");

        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = KphVerifyBufferEx(dyn->ScriptPublicKeyHandle,
                               (PBYTE)file,
                               ScriptLength,
                               signature,
                               SignatureLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphVerifyBufferEx failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpScriptValidate(file, ScriptLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphpScriptValidate failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpScriptRequiredState(file->Level, &requiredState);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphpScriptRequiredState failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!KphTestProcessContextState(process, requiredState))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script requires 0x%08x",
                      requiredState);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    status = KphCreateObject(KphpScriptType, sizeof(KPH_SCRIPT), &script, file);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        script = NULL;
        goto Exit;
    }

    file = NULL;

    script->Flags = Flags;

    if (Flags == KPH_SCRIPT_NON_VOLATILE)
    {
        status = KphpScriptLoadKept(script);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          SCRIPT,
                          "KphpScriptLoadKept failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    status = KphCreateHandle(script, AccessMode, &scriptHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphCreateHandle failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphWriteKphHandleToMode(ScriptHandle, scriptHandle, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphWriteKphHandleToMode failed: %!STATUS!",
                      status);
    }

Exit:

    if (script)
    {
        KphDereferenceObject(script);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (signature)
    {
        KphFree(signature, KPH_TAG_SCRIPT_FILE);
    }

    if (file)
    {
        KphFree(file, KPH_TAG_SCRIPT_FILE);
    }

    if (process)
    {
        KphDereferenceObject(process);
    }

    KphReleaseRundown(&KphpScriptRundown);

    return status;
}

/**
 * \brief Calls a routine of a script object.
 *
 * \details The routine is resolved, and the state of the calling process is
 * checked against what the routine requires, before the input is captured and
 * before any script code runs.
 *
 * \param[in] Script The script object.
 * \param[in] Routine The name of the routine.
 * \param[in] Input The input for the routine.
 * \param[in] InputLength The length of the input in bytes.
 * \param[out] Output Receives the output of the routine.
 * \param[in] OutputLength The length of the output buffer in bytes.
 * \param[out] ReturnLength Receives the length of the output of the routine.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return The status the routine returned, or an errant status when the
 * routine could not be called, failed or reached a limit.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpScriptCall(
    _In_ PKPH_SCRIPT Script,
    _In_ PCANSI_STRING Routine,
    _In_reads_bytes_opt_(InputLength) PVOID Input,
    _In_ ULONG InputLength,
    _Out_writes_bytes_to_opt_(OutputLength, *ReturnLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_PROCESS_CONTEXT process;
    PKPH_SCRIPT_FILE file;
    PKPH_SCRIPT_ROUTINE routine;
    KPH_PROCESS_STATE requiredState;
    PVOID input;

    KPH_PAGED_CODE_PASSIVE();

    *ReturnLength = 0;

    input = NULL;
    file = Script->File;

    process = KphGetProcessContextForMode(AccessMode);
    if (!process)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphGetProcessContextForMode failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    routine = KphpScriptLookupRoutine(file, Routine);
    if (!routine)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z has no routine %Z",
                      &Script->Name,
                      Routine);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    status = KphpScriptRequiredState(routine->Level, &requiredState);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphpScriptRequiredState failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!KphTestProcessContextState(process, requiredState))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z routine %Z requires 0x%08x",
                      &Script->Name,
                      Routine,
                      requiredState);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    status = KphpScriptCaptureInput(Input,
                                    InputLength,
                                    &file->Limits,
                                    AccessMode,
                                    &input);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphpScriptCaptureInput failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (Script->Flags == KPH_SCRIPT_NON_VOLATILE)
    {
        status = KphpScriptRunKept(Script,
                                   Routine,
                                   input,
                                   InputLength,
                                   Output,
                                   OutputLength,
                                   ReturnLength,
                                   AccessMode);
    }
    else
    {
        status = KphpScriptRunChunk(Add2Ptr(file, file->ChunkOffset),
                                    file->ChunkLength,
                                    &Script->Name,
                                    Routine,
                                    input,
                                    InputLength,
                                    Output,
                                    OutputLength,
                                    ReturnLength,
                                    &file->Limits,
                                    AccessMode);
    }
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z routine %Z failed: %!STATUS!",
                      &Script->Name,
                      Routine,
                      status);
    }

Exit:

    if (input)
    {
        KphFree(input, KPH_TAG_SCRIPT_INPUT);
    }

    if (process)
    {
        KphDereferenceObject(process);
    }

    return status;
}

/**
 * \brief Calls a routine of a script object.
 *
 * \param[in] ScriptHandle The handle to the script object.
 * \param[in] Routine The name of the routine.
 * \param[in] Input The input for the routine.
 * \param[in] InputLength The length of the input in bytes.
 * \param[out] Output Receives the output of the routine.
 * \param[in] OutputLength The length of the output buffer in bytes.
 * \param[out] ReturnLength Receives the length of the output of the routine,
 * the length required when the output buffer is too small.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return The status the routine returned, or an errant status when the
 * routine could not be called, failed or reached a limit.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphScriptCall(
    _In_ HANDLE ScriptHandle,
    _In_ PCANSI_STRING Routine,
    _In_reads_bytes_opt_(InputLength) PVOID Input,
    _In_ ULONG InputLength,
    _Out_writes_bytes_opt_(OutputLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_SCRIPT script;
    PANSI_STRING routine;
    ULONG returnLength;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphAcquireRundown(&KphpScriptRundown))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Failed to acquire rundown.");

        return STATUS_TOO_LATE;
    }

    script = NULL;
    routine = NULL;
    returnLength = 0;

    if ((OutputLength > 0) && !Output)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script output length without an output");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = KphReferenceObjectByHandle(ScriptHandle,
                                        KphpScriptType,
                                        AccessMode,
                                        &script);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphReferenceObjectByHandle failed: %!STATUS!",
                      status);

        script = NULL;
        goto Exit;
    }

    status = KphCaptureAnsiString(Routine, &routine, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphCaptureAnsiString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpScriptCall(script,
                            routine,
                            Input,
                            InputLength,
                            Output,
                            OutputLength,
                            &returnLength,
                            AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphpScriptCall failed: %!STATUS!",
                      status);
    }

Exit:

    if (ReturnLength)
    {
        KphWriteULongToMode(ReturnLength, returnLength, AccessMode);
    }

    if (routine)
    {
        KphReleaseAnsiString(routine);
    }

    if (script)
    {
        KphDereferenceObject(script);
    }

    KphReleaseRundown(&KphpScriptRundown);

    return status;
}

/**
 * \brief Retrieves the process state a routine of a script object requires.
 *
 * \details Advisory. KphScriptCall checks again.
 *
 * \param[in] Process The process that holds the handle.
 * \param[in] ScriptHandle The handle to the script object.
 * \param[in] Routine The name of the routine.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return The process state the routine requires, KPH_PROCESS_STATE_LOW when
 * the routine cannot be resolved.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KphScriptGetRequiredState(
    _In_ HANDLE ScriptHandle,
    _In_ PCANSI_STRING Routine,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    KPH_PROCESS_STATE requiredState;
    KPH_PROCESS_STATE routineState;
    PKPH_SCRIPT script;
    PANSI_STRING routineName;
    PKPH_SCRIPT_ROUTINE routine;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphAcquireRundown(&KphpScriptRundown))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Failed to acquire rundown.");

        return KPH_PROCESS_STATE_LOW;
    }

    requiredState = KPH_PROCESS_STATE_LOW;
    script = NULL;
    routineName = NULL;

    status = KphReferenceObjectByHandle(ScriptHandle,
                                        KphpScriptType,
                                        AccessMode,
                                        &script);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphReferenceObjectByHandle failed: %!STATUS!",
                      status);

        script = NULL;
        goto Exit;
    }

    status = KphCaptureAnsiString(Routine, &routineName, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphCaptureAnsiString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    routine = KphpScriptLookupRoutine(script->File, routineName);
    if (!routine)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z has no routine %Z",
                      &script->Name,
                      routineName);

        goto Exit;
    }

    status = KphpScriptRequiredState(routine->Level, &routineState);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphpScriptRequiredState failed: %!STATUS!",
                      status);

        goto Exit;
    }

    requiredState = routineState;

Exit:

    if (routineName)
    {
        KphReleaseAnsiString(routineName);
    }

    if (script)
    {
        KphDereferenceObject(script);
    }

    KphReleaseRundown(&KphpScriptRundown);

    return requiredState;
}

/**
 * \brief Initializes the script infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeScript(
    VOID
    )
{
    KPH_OBJECT_TYPE_INFO typeInfo;

    KPH_PAGED_CODE_PASSIVE();

    typeInfo.Allocate = KphpAllocateScript;
    typeInfo.Initialize = KphpInitializeScript;
    typeInfo.Delete = KphpDeleteScript;
    typeInfo.Free = KphpFreeScript;
    typeInfo.Flags = 0;
    typeInfo.DeferDelete = TRUE;

    KphCreateObjectType(&KphpScriptTypeName, &typeInfo, &KphpScriptType);

    KphInitializeRundown(&KphpScriptRundown);
    InitializeListHead(&KphpScripts);
    KphInitializeRWLock(&KphpScriptsLock);

    KphpScriptInitialized = TRUE;

    KphLuaInitializeLocks();
    KphLuaInitializeHardware();
}

/**
 * \brief Cleans up the script infrastructure.
 *
 * \details Script objects that are still referenced are run down here. The
 * lock of the list of script objects is not deleted, a script object deleted
 * later acquires it.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupScript(
    VOID
    )
{
    PLIST_ENTRY entry;
    PKPH_SCRIPT script;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphpScriptInitialized)
    {
        return;
    }

    KphWaitForRundown(&KphpScriptRundown);

    KphAcquireRWLockExclusive(&KphpScriptsLock);
    while (!IsListEmpty(&KphpScripts))
    {
        entry = RemoveHeadList(&KphpScripts);
        script = CONTAINING_RECORD(entry, KPH_SCRIPT, Entry);
        InitializeListHead(&script->Entry);
        KphpRundownScript(script);
    }
    KphReleaseRWLock(&KphpScriptsLock);

    KphLuaCleanupHardware();
    KphLuaCleanupLocks();
}
