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

#define KPH_LUA_LIBRARY_NAME "ksi"
#define KPH_LUA_STALL_PIECE 50
#define KphLuaStackIsNearLimit()                                               \
    (IoGetRemainingStackSize() < KERNEL_STACK_SIZE)

typedef struct _KPH_LUA_REMOVED
{
    PCSTR Library;
    PCSTR Name;
} KPH_LUA_REMOVED, *PKPH_LUA_REMOVED;

typedef struct _KPH_LUA_LOCK
{
    LIST_ENTRY Entry;
    KMUTEX Mutex;
    ANSI_STRING Name;
} KPH_LUA_LOCK, *PKPH_LUA_LOCK;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const CHAR KphpLuaAbortKey = 0;
static const KPH_LUA_REMOVED KphpLuaRemoved[] =
{
    { LUA_GNAME, "collectgarbage" },
    { LUA_GNAME, "dofile" },
    { LUA_GNAME, "load" },
    { LUA_GNAME, "loadfile" },
    { LUA_GNAME, "print" },
    { LUA_GNAME, "warn" },
    { LUA_GNAME, "xpcall" },
    { LUA_STRLIBNAME, "dump" },
    { LUA_STRLIBNAME, "gmatch" },
    { LUA_STRLIBNAME, "gsub" },
    { LUA_STRLIBNAME, "match" },
    { LUA_TABLIBNAME, "move" },
};
KPH_PROTECTED_DATA_SECTION_RO_POP();
static LIST_ENTRY KphpLuaLocks;
static KPH_RWLOCK KphpLuaLocksLock;
static BOOLEAN KphpLuaLocksInitialized = FALSE;

KPH_PAGED_FILE();

/**
 * \brief Raises the abort error of a call. Does not return.
 *
 * \details The error object was created when the libraries were opened, so
 * raising it allocates nothing.
 *
 * \param[in] State The Lua state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaRaiseAbort(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    lua_rawgetp(State, LUA_REGISTRYINDEX, &KphpLuaAbortKey);
    lua_error(State);
}

/**
 * \brief Raises a Lua error when less than a default kernel stack remains.
 *
 * \param[in] State The Lua state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaCheckStack(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (KphLuaStackIsNearLimit())
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z is short of stack",
                      KphScriptGetName(State));

        luaL_error(State, "C stack overflow");
    }
}

/**
 * \brief Calls a function in protected mode, for scripts: pcall.
 *
 * \details Unlike the pcall of Lua, an abort raised inside the call is raised
 * again: a script cannot catch the end of its call and carry on.
 *
 * \param[in] State The Lua state, with the function and its arguments.
 *
 * \return The number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaProtectedCallForScript(
    _In_ lua_State* State
    )
{
    ULONG64 aborts;
    int arguments;
    int result;

    KPH_PAGED_CODE_PASSIVE();

    luaL_checkany(State, 1);

    KphpLuaCheckStack(State);

    aborts = KphScriptGetAbortCount(State);

    lua_pushboolean(State, TRUE);
    lua_rotate(State, 1, 1);

    arguments = lua_gettop(State) - 2;

    result = lua_pcall(State, arguments, LUA_MULTRET, 0);

    if (KphScriptGetAbortCount(State) != aborts)
    {
        KphLuaRaiseAbort(State);
    }

    if (result != LUA_OK)
    {
        lua_pushboolean(State, FALSE);
        lua_rotate(State, -2, 1);

        return 2;
    }

    return lua_gettop(State);
}

/**
 * \brief Calls a close handler of a script.
 *
 * \details May raise a Lua error. A limit the body reaches inside the handler
 * ends the call when the handler returns, not before.
 *
 * \param[in] State The Lua state, with the arguments of the handler. The
 * first upvalue is the handler.
 *
 * \return Zero, the number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaCloseForScript(
    _In_ lua_State* State
    )
{
    int arguments;
    int result;

    KPH_PAGED_CODE_PASSIVE();

    arguments = lua_gettop(State);

    lua_pushvalue(State, lua_upvalueindex(1));
    lua_rotate(State, 1, 1);

    KphScriptEnterHandler(State);

    result = lua_pcall(State, arguments, 0, 0);

    KphScriptLeaveHandler(State);

    if (result != LUA_OK)
    {
        lua_error(State);
    }

    return 0;
}

/**
 * \brief Sets the metatable of a table, for scripts: setmetatable.
 *
 * \details Unlike the setmetatable of Lua, a metatable with a finalizer is
 * refused, and the close handler of a metatable is called through
 * KphpLuaCloseForScript.
 *
 * \param[in] State The Lua state, with the table and the metatable. The first
 * upvalue is the setmetatable of Lua.
 *
 * \return The number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaSetMetatableForScript(
    _In_ lua_State* State
    )
{
    int arguments;

    KPH_PAGED_CODE_PASSIVE();

    if (lua_type(State, 2) == LUA_TTABLE)
    {
        lua_pushliteral(State, "__gc");

        if (lua_rawget(State, 2) != LUA_TNIL)
        {
            luaL_error(State, "a metatable may not have a finalizer");
        }

        lua_settop(State, -2);

        lua_pushliteral(State, "__close");
        lua_pushliteral(State, "__close");

        if ((lua_rawget(State, 2) != LUA_TNIL) &&
            (lua_tocfunction(State, -1) != KphpLuaCloseForScript))
        {
            lua_pushcclosure(State, KphpLuaCloseForScript, 1);
            lua_rawset(State, 2);
        }
        else
        {
            lua_settop(State, -3);
        }
    }

    arguments = lua_gettop(State);

    lua_pushvalue(State, lua_upvalueindex(1));
    lua_rotate(State, 1, 1);

    lua_call(State, arguments, LUA_MULTRET);

    return lua_gettop(State);
}

/**
 * \brief Finds text in a string, for scripts: string.find.
 *
 * \details Unlike the find of Lua, the text is never a pattern.
 *
 * \param[in] State The Lua state, with the string, the text and the optional
 * start. The first upvalue is the find of Lua.
 *
 * \return The number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaFindForScript(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    lua_settop(State, 3);
    lua_pushboolean(State, TRUE);

    lua_pushvalue(State, lua_upvalueindex(1));
    lua_rotate(State, 1, 1);

    lua_call(State, 4, LUA_MULTRET);

    return lua_gettop(State);
}

/**
 * \brief Calls the function of Lua that a function for scripts stands in
 * front of, with the arguments on the stack.
 *
 * \param[in] State The Lua state. The first upvalue is the function of Lua.
 *
 * \return The number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaCallStock(
    _In_ lua_State* State
    )
{
    int arguments;

    KPH_PAGED_CODE_PASSIVE();

    arguments = lua_gettop(State);

    lua_pushvalue(State, lua_upvalueindex(1));
    lua_rotate(State, 1, 1);

    lua_call(State, arguments, LUA_MULTRET);

    return lua_gettop(State);
}

/**
 * \brief Checks that the first argument is a table without a metatable, and
 * retrieves its length.
 *
 * \details Raises a Lua error otherwise. The table functions of Lua loop in C
 * over a length and through metamethods, outside the count hook. For scripts
 * they take plain tables.
 *
 * \param[in] State The Lua state.
 *
 * \return The length of the table.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
lua_Unsigned KphpLuaCheckPlainTable(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    luaL_checktype(State, 1, LUA_TTABLE);

    if (lua_getmetatable(State, 1))
    {
        luaL_error(State, "the table has a metatable");
    }

    return lua_rawlen(State, 1);
}

/**
 * \brief Calls a table function of Lua on a plain table, for scripts:
 * table.concat and table.unpack.
 *
 * \param[in] State The Lua state, with the table and the other arguments. The
 * first upvalue is the function of Lua.
 *
 * \return The number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaPlainTableForScript(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphpLuaCheckPlainTable(State);

    return KphpLuaCallStock(State);
}

/**
 * \brief Inserts into a plain table, for scripts: table.insert.
 *
 * \details The elements an insert moves are charged to the call.
 *
 * \param[in] State The Lua state, with the table, the optional position and the
 * value. The first upvalue is the insert of Lua.
 *
 * \return The number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaInsertForScript(
    _In_ lua_State* State
    )
{
    lua_Unsigned length;
    lua_Unsigned position;
    lua_Unsigned count;
    int arguments;

    KPH_PAGED_CODE_PASSIVE();

    length = KphpLuaCheckPlainTable(State);

    arguments = lua_gettop(State);

    if (arguments == 3)
    {
        position = (lua_Unsigned)luaL_checkinteger(State, 2);

        if ((position >= 1) && (position <= length))
        {
            count = length - position + 1;

            KphScriptCharge(State, count);
        }
    }

    return KphpLuaCallStock(State);
}

/**
 * \brief Removes from a plain table, for scripts: table.remove.
 *
 * \details The elements a remove moves are charged to the call.
 *
 * \param[in] State The Lua state, with the table and the optional position. The
 * first upvalue is the remove of Lua.
 *
 * \return The number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaRemoveForScript(
    _In_ lua_State* State
    )
{
    lua_Unsigned length;
    lua_Unsigned position;
    lua_Unsigned count;

    KPH_PAGED_CODE_PASSIVE();

    length = KphpLuaCheckPlainTable(State);

    //
    // No position, or nil, removes the last element.
    //
    if (!lua_isnoneornil(State, 2))
    {
        position = (lua_Unsigned)luaL_checkinteger(State, 2);

        if ((position >= 1) && (position < length))
        {
            count = length - position;

            KphScriptCharge(State, count);
        }
    }

    return KphpLuaCallStock(State);
}

/**
 * \brief Sorts a plain table, for scripts: table.sort.
 *
 * \details The comparisons a sort of that length makes are charged to the
 * call: the length times the number of its halvings.
 *
 * \param[in] State The Lua state, with the table and the optional comparison
 * function. The first upvalue is the sort of Lua.
 *
 * \return The number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaSortForScript(
    _In_ lua_State* State
    )
{
    NTSTATUS status;
    lua_Unsigned length;
    ULONG64 halvings;
    ULONG64 count;

    KPH_PAGED_CODE_PASSIVE();

    KphpLuaCheckStack(State);

    length = KphpLuaCheckPlainTable(State);

    halvings = 1;
    for (lua_Unsigned remaining = length; remaining > 1; remaining >>= 1)
    {
        halvings++;
    }

    status = RtlULongLongMult(length, halvings, &count);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "RtlULongLongMult failed: %!STATUS!",
                      status);

        count = ULONG64_MAX;
    }

    if (count > 0)
    {
        KphScriptCharge(State, count);
    }

    return KphpLuaCallStock(State);
}

/**
 * \brief Retrieves the absolute value of an integer: math.abs.
 *
 * \param[in] State The Lua state, with the integer.
 *
 * \return One, the absolute value.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaAbs(
    _In_ lua_State* State
    )
{
    lua_Integer value;

    KPH_PAGED_CODE_PASSIVE();

    value = luaL_checkinteger(State, 1);

    if (value < 0)
    {
        value = (lua_Integer)(0 - (lua_Unsigned)value);
    }

    lua_pushinteger(State, value);

    return 1;
}

/**
 * \brief Retrieves the largest of its integers: math.max.
 *
 * \param[in] State The Lua state, with one or more integers.
 *
 * \return One, the largest.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaMax(
    _In_ lua_State* State
    )
{
    lua_Integer result;
    lua_Integer value;
    int arguments;

    KPH_PAGED_CODE_PASSIVE();

    arguments = lua_gettop(State);

    result = luaL_checkinteger(State, 1);

    for (int i = 2; i <= arguments; i++)
    {
        value = luaL_checkinteger(State, i);

        if (value > result)
        {
            result = value;
        }
    }

    lua_pushinteger(State, result);

    return 1;
}

/**
 * \brief Retrieves the smallest of its integers: math.min.
 *
 * \param[in] State The Lua state, with one or more integers.
 *
 * \return One, the smallest.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaMin(
    _In_ lua_State* State
    )
{
    lua_Integer result;
    lua_Integer value;
    int arguments;

    KPH_PAGED_CODE_PASSIVE();

    arguments = lua_gettop(State);

    result = luaL_checkinteger(State, 1);

    for (int i = 2; i <= arguments; i++)
    {
        value = luaL_checkinteger(State, i);

        if (value < result)
        {
            result = value;
        }
    }

    lua_pushinteger(State, result);

    return 1;
}

/**
 * \brief Compares two integers as unsigned: math.ult.
 *
 * \param[in] State The Lua state, with two integers.
 *
 * \return One, whether the first is below the second.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaUlt(
    _In_ lua_State* State
    )
{
    lua_Unsigned first;
    lua_Unsigned second;
    BOOLEAN below;

    KPH_PAGED_CODE_PASSIVE();

    first = (lua_Unsigned)luaL_checkinteger(State, 1);
    second = (lua_Unsigned)luaL_checkinteger(State, 2);

    below = FALSE;
    if (first < second)
    {
        below = TRUE;
    }

    lua_pushboolean(State, below);

    return 1;
}

/**
 * \brief Traces a line for a script: ksi.trace.
 *
 * \details With one argument the text is traced as it is. With more, the
 * first is a format for the rest, as for string.format. Nothing is formatted,
 * and nothing can fail, unless the trace is being collected.
 *
 * \param[in] State The Lua state, with the text or the format and its
 * arguments. The first upvalue is the format of Lua.
 *
 * \return Zero, the number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaTrace(
    _In_ lua_State* State
    )
{
    PCANSI_STRING script;
    ANSI_STRING text;
    PCSTR buffer;
    SIZE_T length;
    ULONG64 aborts;
    int arguments;
    int result;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphTraceEnabled(TRACE_LEVEL_VERBOSE, SCRIPT))
    {
        return 0;
    }

    script = KphScriptGetName(State);

    arguments = lua_gettop(State);

    if (arguments > 1)
    {
        //
        // Formatting can call back into the script. A trace is not worth
        // the stack, and may not raise.
        //
        if (KphLuaStackIsNearLimit())
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          SCRIPT,
                          "Script %Z trace skipped: the stack is short",
                          script);

            return 0;
        }

        aborts = KphScriptGetAbortCount(State);

        lua_pushvalue(State, lua_upvalueindex(1));
        lua_rotate(State, 1, 1);

        result = lua_pcall(State, arguments, 1, 0);

        if (KphScriptGetAbortCount(State) != aborts)
        {
            KphLuaRaiseAbort(State);
        }

        //
        // Whether the trace is collected must not change what a script
        // does, so a format that fails is traced and not raised.
        //
        if (result != LUA_OK)
        {
            buffer = lua_tolstring(State, -1, NULL);

            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          SCRIPT,
                          "Script %Z trace failed: %hs",
                          script,
                          buffer);

            return 0;
        }
    }

    buffer = lua_tolstring(State, 1, &length);
    if (!buffer)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Script %Z traced a value that is not text",
                      script);

        return 0;
    }

    if (length > USHORT_MAX)
    {
        length = USHORT_MAX;
    }

    text.Buffer = (PCHAR)buffer;
    text.Length = (USHORT)length;
    text.MaximumLength = (USHORT)length;

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  SCRIPT,
                  "Script %Z: %Z",
                  script,
                  &text);

    return 0;
}

/**
 * \brief Retrieves the interrupt time: ksi.time.
 *
 * \param[in] State The Lua state.
 *
 * \return One, the interrupt time in 100 nanosecond units.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaTime(
    _In_ lua_State* State
    )
{
    ULONG64 now;

    KPH_PAGED_CODE_PASSIVE();

    now = KeQueryInterruptTime();

    lua_pushinteger(State, (lua_Integer)now);

    return 1;
}

/**
 * \brief Waits a number of milliseconds, no longer than the call may still
 * run (ksi.sleep).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return Zero, the number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaSleep(
    _In_ lua_State* State
    )
{
    NTSTATUS status;
    lua_Integer milliseconds;

    KPH_PAGED_CODE_PASSIVE();

    milliseconds = luaL_checkinteger(State, 1);

    luaL_argcheck(State,
                  (milliseconds >= 0) && (milliseconds < ULONG_MAX),
                  1,
                  "out of range");

    status = KphScriptWait(State, NULL, (ULONG)milliseconds);
    if (status == STATUS_SI_SCRIPT_LIMIT)
    {
        KphScriptCharge(State, 0);
    }

    return 0;
}

/**
 * \brief Waits a number of microseconds without giving up the processor, no
 * longer than the call may still run (ksi.stall).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return Zero, the number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaStall(
    _In_ lua_State* State
    )
{
    lua_Integer microseconds;
    ULONG remaining;
    ULONG piece;

    KPH_PAGED_CODE_PASSIVE();

    microseconds = luaL_checkinteger(State, 1);

    luaL_argcheck(State,
                  (microseconds >= 0) && (microseconds <= ULONG_MAX),
                  1,
                  "out of range");

    remaining = (ULONG)microseconds;

    while (remaining > 0)
    {
        piece = min(remaining, KPH_LUA_STALL_PIECE);

        KeStallExecutionProcessor(piece);

        remaining -= piece;

        KphScriptCharge(State, 0);
    }

    return 0;
}

/**
 * \brief Initializes the locks of scripts.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeLocks(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    InitializeListHead(&KphpLuaLocks);
    KphInitializeRWLock(&KphpLuaLocksLock);

    KphpLuaLocksInitialized = TRUE;
}

/**
 * \brief Cleans up the locks of scripts.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupLocks(
    VOID
    )
{
    PLIST_ENTRY entry;
    PKPH_LUA_LOCK lock;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphpLuaLocksInitialized)
    {
        return;
    }

    while (!IsListEmpty(&KphpLuaLocks))
    {
        entry = RemoveHeadList(&KphpLuaLocks);

        lock = CONTAINING_RECORD(entry, KPH_LUA_LOCK, Entry);

        KphFree(lock, KPH_TAG_LUA_LOCK);
    }

    KphDeleteRWLock(&KphpLuaLocksLock);
}

/**
 * \brief Finds the lock of a name, creating it the first time the name is
 * used.
 *
 * \param[in] Name The name of the lock.
 *
 * \return The lock, NULL when it could not be created.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PKPH_LUA_LOCK KphpLuaFindLock(
    _In_ PCANSI_STRING Name
    )
{
    PLIST_ENTRY entry;
    PKPH_LUA_LOCK lock;
    SIZE_T size;

    KPH_PAGED_CODE_PASSIVE();

    lock = NULL;

    KphAcquireRWLockExclusive(&KphpLuaLocksLock);

    for (entry = KphpLuaLocks.Flink;
         entry != &KphpLuaLocks;
         entry = entry->Flink)
    {
        lock = CONTAINING_RECORD(entry, KPH_LUA_LOCK, Entry);

        if (RtlEqualString(&lock->Name, Name, FALSE))
        {
            goto Exit;
        }
    }

    NT_VERIFY(NT_SUCCESS(RtlSIZETAdd(sizeof(KPH_LUA_LOCK),
                                     Name->Length,
                                     &size)));

    lock = KphAllocateNPaged(size, KPH_TAG_LUA_LOCK);
    if (!lock)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Failed to allocate lock %Z",
                      Name);

        goto Exit;
    }

    KeInitializeMutex(&lock->Mutex, 0);

    lock->Name.Buffer = Add2Ptr(lock, sizeof(KPH_LUA_LOCK));
    lock->Name.Length = Name->Length;
    lock->Name.MaximumLength = Name->Length;
    RtlCopyMemory(lock->Name.Buffer, Name->Buffer, Name->Length);

    InsertTailList(&KphpLuaLocks, &lock->Entry);

Exit:

    KphReleaseRWLock(&KphpLuaLocksLock);

    return lock;
}

/**
 * \brief Releases a lock a script holds.
 *
 * \param[in] Context The lock.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaReleaseLock(
    _In_opt_ PVOID Context
    )
{
    PKPH_LUA_LOCK lock;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    lock = Context;

    KeReleaseMutex(&lock->Mutex, FALSE);

    KeLeaveCriticalRegion();
}

/**
 * \brief Takes a lock by name (ksi.lock).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the lock object, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaLock(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PCSTR buffer;
    size_t length;
    lua_Integer milliseconds;
    ANSI_STRING name;
    PKPH_LUA_LOCK lock;

    KPH_PAGED_CODE_PASSIVE();

    buffer = luaL_checklstring(State, 1, &length);

    luaL_argcheck(State,
                  (length > 0) && (length <= USHORT_MAX),
                  1,
                  "empty or too long");

    milliseconds = ULONG_MAX;
    if (!lua_isnoneornil(State, 2))
    {
        milliseconds = luaL_checkinteger(State, 2);

        luaL_argcheck(State,
                      (milliseconds >= 0) && (milliseconds < ULONG_MAX),
                      2,
                      "out of range");
    }

    name.Buffer = (PCHAR)buffer;
    name.Length = (USHORT)length;
    name.MaximumLength = (USHORT)length;

    lock = KphpLuaFindLock(&name);
    if (!lock)
    {
        results = KphLuaNilErrorStatus(State, STATUS_INSUFFICIENT_RESOURCES);
        goto Exit;
    }

    if (lock->Mutex.OwnerThread == KeGetCurrentThread())
    {
        luaL_error(State, "the lock is already held");
    }

    KeEnterCriticalRegion();

    status = KphScriptWait(State, &lock->Mutex, (ULONG)milliseconds);
    if (status == STATUS_SUCCESS)
    {
        KphScriptPushCleanup(State, KphpLuaReleaseLock, lock);
        results = 1;
        goto Exit;
    }

    KeLeaveCriticalRegion();

    if (status == STATUS_SI_SCRIPT_LIMIT)
    {
        KphScriptCharge(State, 0);
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  SCRIPT,
                  "KphScriptWait failed: %!STATUS!",
                  status);

    results = KphLuaNilErrorStatus(State, status);

Exit:

    return results;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpLuaLibrary[] =
{
    { "lock", KphpLuaLock },
    { "sleep", KphpLuaSleep },
    { "stall", KphpLuaStall },
    { "time", KphpLuaTime },
    { NULL, NULL }
};
static const luaL_Reg KphpLuaMathLibrary[] =
{
    { "abs", KphpLuaAbs },
    { "max", KphpLuaMax },
    { "min", KphpLuaMin },
    { "ult", KphpLuaUlt },
    { NULL, NULL }
};
KPH_PROTECTED_DATA_SECTION_RO_POP();

/**
 * \brief Opens the library of the driver.
 *
 * \param[in] State The Lua state.
 *
 * \return One, the library table.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaOpenLibrary(
    _In_ lua_State* State
    )
{
    BOOLEAN tracing;

    KPH_PAGED_CODE_PASSIVE();

    luaL_newlib(State, KphpLuaLibrary);

    lua_getglobal(State, LUA_STRLIBNAME);
    lua_getfield(State, -1, "format");
    lua_pushcclosure(State, KphpLuaTrace, 1);
    lua_setfield(State, -3, "trace");
    lua_settop(State, -2);

    //
    // A script tests this to skip work that only a trace needs.
    //
    tracing = FALSE;
    if (KphTraceEnabled(TRACE_LEVEL_VERBOSE, SCRIPT))
    {
        tracing = TRUE;
    }

    lua_pushboolean(State, tracing);
    lua_setfield(State, -2, "tracing");

    lua_pushboolean(State, KphScriptIsVolatile(State));
    lua_setfield(State, -2, "volatile");

    return 1;
}

/**
 * \brief Opens the integer mathematics library.
 *
 * \param[in] State The Lua state.
 *
 * \return One, the library table.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaOpenMath(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    luaL_newlib(State, KphpLuaMathLibrary);

    lua_pushinteger(State, LUA_MAXINTEGER);
    lua_setfield(State, -2, "maxinteger");

    lua_pushinteger(State, LUA_MININTEGER);
    lua_setfield(State, -2, "mininteger");

    return 1;
}

/**
 * \brief Removes from the opened libraries what a script does not get.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaRemove(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    for (ULONG i = 0; i < ARRAYSIZE(KphpLuaRemoved); i++)
    {
        lua_getglobal(State, KphpLuaRemoved[i].Library);
        lua_pushnil(State);
        lua_setfield(State, -2, KphpLuaRemoved[i].Name);
        lua_settop(State, -2);
    }
}

/**
 * \brief Replaces a function of the table on top of the Lua stack with one
 * that receives the replaced function as its first upvalue.
 *
 * \param[in] State The Lua state.
 * \param[in] Name The name of the function.
 * \param[in] Function The replacement.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaWrap(
    _In_ lua_State* State,
    _In_z_ PCSTR Name,
    _In_ lua_CFunction Function
    )
{
    KPH_PAGED_CODE_PASSIVE();

    lua_getfield(State, -1, Name);
    lua_pushcclosure(State, Function, 1);
    lua_setfield(State, -2, Name);
}

/**
 * \brief Opens the libraries a script sees.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenLibraries(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    lua_pushliteral(State, "limit reached");
    lua_rawsetp(State, LUA_REGISTRYINDEX, &KphpLuaAbortKey);

    luaL_requiref(State, LUA_GNAME, luaopen_base, 1);
    lua_pushcfunction(State, KphpLuaProtectedCallForScript);
    lua_setfield(State, -2, "pcall");
    KphpLuaWrap(State, "setmetatable", KphpLuaSetMetatableForScript);
    lua_settop(State, 0);

    luaL_requiref(State, LUA_STRLIBNAME, luaopen_string, 1);
    KphpLuaWrap(State, "find", KphpLuaFindForScript);
    lua_settop(State, 0);

    luaL_requiref(State, LUA_TABLIBNAME, luaopen_table, 1);
    KphpLuaWrap(State, "concat", KphpLuaPlainTableForScript);
    KphpLuaWrap(State, "insert", KphpLuaInsertForScript);
    KphpLuaWrap(State, "remove", KphpLuaRemoveForScript);
    KphpLuaWrap(State, "sort", KphpLuaSortForScript);
    KphpLuaWrap(State, "unpack", KphpLuaPlainTableForScript);
    lua_settop(State, 0);

    luaL_requiref(State, LUA_MATHLIBNAME, KphpLuaOpenMath, 1);
    luaL_requiref(State, KPH_LUA_LIBRARY_NAME, KphpLuaOpenLibrary, 1);
    KphLuaOpenHardwareLibrary(State);
    lua_settop(State, 0);

    KphpLuaRemove(State);
}
