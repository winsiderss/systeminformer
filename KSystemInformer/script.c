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

typedef struct _KPH_SCRIPT_CLEANUP
{
    LIST_ENTRY Entry;
    PKPH_SCRIPT_CLEANUP_ROUTINE Routine;
    PVOID Context;
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
    LIST_ENTRY Cleanups;
    KPH_SCRIPT_PHASE Phase;
    ULONG64 Instructions;
    ULONG64 Deadline;
    ULONG64 Aborts;
    BOOLEAN Aborted;
    NTSTATUS AbortStatus;
    NTSTATUS Status;
} KPH_SCRIPT_CALL, *PKPH_SCRIPT_CALL;

typedef struct _KPH_SCRIPT
{
    ULONG Flags;
    PKPH_SCRIPT_FILE File;
    ANSI_STRING Name;
} KPH_SCRIPT, *PKPH_SCRIPT;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
DECLARE_CONST_UNICODE_STRING(KphpScriptTypeName, L"KphScript");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphpScriptType = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

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
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpScriptRunCleanups(
    _Inout_ PKPH_SCRIPT_CALL Call
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

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z left a cleanup behind",
                      Call->Script);

        routine(context);
    }
}

/**
 * \brief Runs the cleanup of a cleanup object, if it has not run: its release
 * method and its close handler.
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
    PKPH_SCRIPT_CLEANUP cleanup;
    PKPH_SCRIPT_CLEANUP_ROUTINE routine;
    PVOID context;

    KPH_PAGED_CODE_PASSIVE();

    cleanup = luaL_checkudata(State, 1, KPH_SCRIPT_CLEANUP_NAME);

    routine = cleanup->Routine;
    context = cleanup->Context;

    if (!routine)
    {
        return 0;
    }

    RemoveEntryList(&cleanup->Entry);

    cleanup->Routine = NULL;

    routine(context);

    //
    // The registry held the object for as long as it held a cleanup.
    //
    lua_pushnil(State);
    lua_rawsetp(State, LUA_REGISTRYINDEX, cleanup);

    return 0;
}

/**
 * \brief Creates a cleanup object that holds no cleanup yet.
 *
 * \details May raise a Lua error. The registry holds the object until its
 * cleanup has run.
 *
 * \param[in] State The Lua state.
 *
 * \return One, the cleanup object.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpScriptNewCleanup(
    _In_ lua_State* State
    )
{
    PKPH_SCRIPT_CLEANUP cleanup;

    KPH_PAGED_CODE_PASSIVE();

    cleanup = lua_newuserdatauv(State, sizeof(KPH_SCRIPT_CLEANUP), 0);

    InitializeListHead(&cleanup->Entry);
    cleanup->Routine = NULL;
    cleanup->Context = NULL;

    luaL_setmetatable(State, KPH_SCRIPT_CLEANUP_NAME);

    lua_pushvalue(State, -1);
    lua_rawsetp(State, LUA_REGISTRYINDEX, cleanup);

    return 1;
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

    result = lua_pcall(State, 0, 1, 0);
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

    InsertHeadList(&call->Cleanups, &cleanup->Entry);
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
 * that is used up too, no further script code runs.
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

    now = KeQueryInterruptTime();

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
            break;
        }
        case KphScriptPhaseCleanup:
        {
            abort = KphpScriptCountCleanup(call, now, Count);
            if (abort)
            {
                //
                // From here on the hook runs before every instruction.
                //
                hook = lua_gethook(State);
                lua_sethook(State, hook, LUA_MASKCOUNT, 1);
            }
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
        call->Aborts++;

        KphLuaRaiseAbort(State);
    }
}

/**
 * \brief Counts the instructions the count hook of a call stands for.
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
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Debug);

    KphpScriptCount(State, KPH_SCRIPT_HOOK_INTERVAL);
}

/**
 * \brief Charges a call for work a library function is about to do outside
 * the count hook. Raises the abort of the call when that is over its limit.
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

    now = KeQueryInterruptTime();

    end = ULONG64_MAX;
    if (Milliseconds != ULONG_MAX)
    {
        end = KphpScriptDeadline(now, Milliseconds);
    }

    if (now >= call->Deadline)
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

        now = KeQueryInterruptTime();

        if (now >= call->Deadline)
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

    //
    // A script may write a status as a signed or as an unsigned 32-bit value.
    //
    if ((value < LONG_MIN) || (value > ULONG_MAX))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Routine returned a status out of range");

        return STATUS_SI_SCRIPT_ERROR;
    }

    *RoutineStatus = (NTSTATUS)(ULONG)(value & ULONG_MAX);

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

    now = KeQueryInterruptTime();

    call->Deadline = KphpScriptDeadline(now, call->Limits->Milliseconds);

    status = KphLuaProtectedCall(state, KphpScriptProtectedLoad, call);
    if (NT_SUCCESS(status))
    {
        status = KphLuaProtectedCall(state, KphpScriptProtectedRoutine, call);
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphLuaProtectedCall failed: %!STATUS!",
                      status);

        call->Status = status;
    }

    if (!call->Aborted && (call->Allocator->Failures != 0))
    {
        call->Aborted = TRUE;
        call->AbortStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (call->Aborted)
    {
        call->Status = call->AbortStatus;
        call->ReturnLength = 0;
    }

    if (call->Phase == KphScriptPhaseStopped)
    {
        call->Status = STATUS_SI_SCRIPT_CLEANUP_LIMIT;
        call->ReturnLength = 0;
    }

    KphpScriptRunCleanups(call);

    KphLuaCloseState(state);
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
    call.Allocator = &allocator;
    call.Limits = Limits;
    call.Chunk = Chunk;
    call.ChunkLength = ChunkLength;
    call.Script = Script;
    call.Routine = Routine;
    call.Input = Input;
    call.InputLength = InputLength;
    call.Output = Output;
    call.OutputLength = OutputLength;
    call.ReturnLength = 0;
    call.AccessMode = AccessMode;
    InitializeListHead(&call.Cleanups);
    call.Phase = KphScriptPhaseBody;
    call.Instructions = 0;
    call.Deadline = ULONG64_MAX;
    call.Aborts = 0;
    call.Aborted = FALSE;
    call.AbortStatus = STATUS_SUCCESS;
    call.Status = STATUS_UNSUCCESSFUL;

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

    KphpScriptRunCleanups(&call);

    KeLeaveCriticalRegion();

    KphLuaDeleteAllocator(&allocator);

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
        case KPH_DYN_SCRIPT_LEVEL_LOW:
        {
            *RequiredState = KPH_PROCESS_STATE_LOW;
            break;
        }
        case KPH_DYN_SCRIPT_LEVEL_MEDIUM:
        {
            *RequiredState = KPH_PROCESS_STATE_MEDIUM;
            break;
        }
        case KPH_DYN_SCRIPT_LEVEL_HIGH:
        {
            *RequiredState = KPH_PROCESS_STATE_HIGH;
            break;
        }
        case KPH_DYN_SCRIPT_LEVEL_MAXIMUM:
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
NTSTATUS KphpScriptValidateFile(
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
        (File->Magic != KPH_SCRIPT_FILE_MAGIC) ||
        (File->Length != Length))
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

        return STATUS_INVALID_PARAMETER;
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
        if (!NT_SUCCESS(status) || (routine->Level < File->Level))
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

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes a script object.
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

    if (Flags != KPH_SCRIPT_VOLATILE)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script flags 0x%08lx are not valid",
                      Flags);

        status = (Flags == KPH_SCRIPT_NON_VOLATILE) ?
                 STATUS_NOT_SUPPORTED : STATUS_INVALID_PARAMETER;
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

    status = KphpScriptValidateFile(file, ScriptLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphpScriptValidateFile failed: %!STATUS!",
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
    _In_ PANSI_STRING Routine,
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
    _In_ PANSI_STRING Routine,
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

    KphLuaInitializeHardware();
}

/**
 * \brief Cleans up the script infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupScript(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphLuaCleanupHardware();
}
