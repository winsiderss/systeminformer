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
    KPH_LUA_ALLOCATOR Allocator;
    PKPH_DYN_SCRIPT_LIMITS Limits;
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

    return (call->Aborts + call->Allocator.Failures);
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

    KphLuaReleaseReserve(&Call->Allocator);
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
        (call->Allocator.Failures != 0))
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
 * \brief Loads the script of a call, calls its routine and takes the results.
 * A Lua error ends it.
 *
 * \param[in] State The Lua state.
 * \param[in,out] Context The call.
 */
_Function_class_(KPH_LUA_PROTECTED_CALL_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpScriptProtectedCall(
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

    KphLuaOpenLibraries(State);

    KphpScriptOpenCleanup(State);

    lua_sethook(State,
                KphpScriptHook,
                LUA_MASKCOUNT,
                KPH_SCRIPT_HOOK_INTERVAL);

    KphLuaLoadChunk(State, call->Chunk, call->ChunkLength, call->Script);

    lua_call(State, 0, 0);

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

    status = KphLuaCreateState(&call->Allocator, &state);
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

    status = KphLuaProtectedCall(state, KphpScriptProtectedCall, call);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphLuaProtectedCall failed: %!STATUS!",
                      status);

        call->Status = status;
    }

    if (!call->Aborted && (call->Allocator.Failures != 0))
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
    _In_ PKPH_DYN_SCRIPT_LIMITS Limits,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    KPH_SCRIPT_CALL call;

    KPH_PAGED_CODE_PASSIVE();

    *ReturnLength = 0;

    KphLuaInitializeAllocator(&call.Allocator,
                              Limits->Memory,
                              Limits->CleanupMemory);
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

    KphLuaDeleteAllocator(&call.Allocator);

    return status;
}

/**
 * \brief Retrieves the process state a routine requires of its caller.
 *
 * \param[in] Routine The routine.
 * \param[out] RequiredState Receives the process state the routine requires.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpScriptRequiredState(
    _In_ PKPH_DYN_SCRIPT_ROUTINE Routine,
    _Out_ PKPH_PROCESS_STATE RequiredState
    )
{
    KPH_PAGED_CODE_PASSIVE();

    *RequiredState = KPH_PROCESS_STATE_MAXIMUM;

    switch (Routine->Level)
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
                          "Routine has an unknown level: %lu",
                          (ULONG)Routine->Level);

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
    _In_ PKPH_DYN_SCRIPT_LIMITS Limits,
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
 * \brief Calls a routine of a script of the active dynamic data.
 *
 * \details The script and the routine are resolved, and the state of the
 * calling process is checked against what the routine requires, before the
 * input is captured and before any script code runs.
 *
 * \param[in] Script The name of the script.
 * \param[in] Routine The name of the routine.
 * \param[in] Input The input for the routine.
 * \param[in] InputLength The length of the input in bytes.
 * \param[out] Output Receives the output of the routine.
 * \param[in] OutputLength The length of the output buffer in bytes.
 * \param[out] ReturnLength Receives the length of the output of the routine.
 * \param[in] ProcessState The state of the calling process.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return The status the routine returned, or an errant status when the
 * routine could not be called, failed or reached a limit.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpScriptCall(
    _In_ PCANSI_STRING Script,
    _In_ PCANSI_STRING Routine,
    _In_reads_bytes_opt_(InputLength) PVOID Input,
    _In_ ULONG InputLength,
    _Out_writes_bytes_to_opt_(OutputLength, *ReturnLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_ PULONG ReturnLength,
    _In_ KPH_PROCESS_STATE ProcessState,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;
    PKPH_DYN_SCRIPT script;
    PKPH_DYN_SCRIPT_ROUTINE routine;
    KPH_PROCESS_STATE requiredState;
    PKPH_DYN_SCRIPT_LIMITS limits;
    PVOID input;
    PVOID chunk;

    KPH_PAGED_CODE_PASSIVE();

    *ReturnLength = 0;

    input = NULL;

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->ScriptSection)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = KphDynDataLookupScript(dyn->ScriptSection,
                                    dyn->ScriptSectionLength,
                                    Script,
                                    &script);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphDynDataLookupScript failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphDynDataLookupScriptRoutine(dyn->ScriptSection,
                                           dyn->ScriptSectionLength,
                                           script,
                                           Routine,
                                           &routine);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphDynDataLookupScriptRoutine failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpScriptRequiredState(routine, &requiredState);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphpScriptRequiredState failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!KphTestProcessState(ProcessState, requiredState))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z routine %Z requires 0x%08x, caller has 0x%08x",
                      Script,
                      Routine,
                      requiredState,
                      ProcessState);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    limits = &dyn->ScriptSection->Limits;

    status = KphpScriptCaptureInput(Input,
                                    InputLength,
                                    limits,
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

    chunk = Add2Ptr(dyn->ScriptSection, script->ChunkOffset);

    status = KphpScriptRunChunk(chunk,
                                script->ChunkLength,
                                Script,
                                Routine,
                                input,
                                InputLength,
                                Output,
                                OutputLength,
                                ReturnLength,
                                limits,
                                AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z routine %Z failed: %!STATUS!",
                      Script,
                      Routine,
                      status);
    }

Exit:

    if (input)
    {
        KphFree(input, KPH_TAG_SCRIPT_INPUT);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    return status;
}

/**
 * \brief Calls a routine of a script of the active dynamic data.
 *
 * \param[in] Script The name of the script.
 * \param[in] Routine The name of the routine.
 * \param[in] Input The input for the routine.
 * \param[in] InputLength The length of the input in bytes.
 * \param[out] Output Receives the output of the routine.
 * \param[in] OutputLength The length of the output buffer in bytes.
 * \param[out] ReturnLength Receives the length of the output of the routine,
 * the length required when the output buffer is too small.
 * \param[in] ProcessState The state of the calling process.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return The status the routine returned, or an errant status when the
 * routine could not be called, failed or reached a limit.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphScriptCall(
    _In_ PANSI_STRING Script,
    _In_ PANSI_STRING Routine,
    _In_reads_bytes_opt_(InputLength) PVOID Input,
    _In_ ULONG InputLength,
    _Out_writes_bytes_opt_(OutputLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPH_PROCESS_STATE ProcessState,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PANSI_STRING script;
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

    status = KphCaptureAnsiString(Script, &script, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphCaptureAnsiString failed: %!STATUS!",
                      status);

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
                            ProcessState,
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
        KphReleaseAnsiString(script);
    }

    return status;
}

/**
 * \brief Retrieves the process state a routine of a script requires.
 *
 * \details Advisory. KphScriptCall checks again against the dynamic data it
 * then runs from.
 *
 * \param[in] Script The name of the script.
 * \param[in] Routine The name of the routine.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return The process state the routine requires, KPH_PROCESS_STATE_LOW when
 * the routine cannot be resolved.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KphScriptGetRequiredState(
    _In_ PANSI_STRING Script,
    _In_ PANSI_STRING Routine,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    KPH_PROCESS_STATE requiredState;
    KPH_PROCESS_STATE routineState;
    PANSI_STRING scriptName;
    PANSI_STRING routineName;
    PKPH_DYN dyn;
    PKPH_DYN_SCRIPT script;
    PKPH_DYN_SCRIPT_ROUTINE routine;

    KPH_PAGED_CODE_PASSIVE();

    requiredState = KPH_PROCESS_STATE_LOW;

    scriptName = NULL;
    routineName = NULL;
    dyn = NULL;

    status = KphCaptureAnsiString(Script, &scriptName, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphCaptureAnsiString failed: %!STATUS!",
                      status);

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

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->ScriptSection)
    {
        goto Exit;
    }

    status = KphDynDataLookupScript(dyn->ScriptSection,
                                    dyn->ScriptSectionLength,
                                    scriptName,
                                    &script);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphDynDataLookupScript failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphDynDataLookupScriptRoutine(dyn->ScriptSection,
                                           dyn->ScriptSectionLength,
                                           script,
                                           routineName,
                                           &routine);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KphDynDataLookupScriptRoutine failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpScriptRequiredState(routine, &routineState);
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

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (routineName)
    {
        KphReleaseAnsiString(routineName);
    }

    if (scriptName)
    {
        KphReleaseAnsiString(scriptName);
    }

    return requiredState;
}
