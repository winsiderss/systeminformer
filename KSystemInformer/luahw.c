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

#define KPH_LUA_HARDWARE_LIBRARY_NAME "hw"

typedef struct _KPH_LUA_HW_LOCK
{
    LIST_ENTRY Entry;
    KEVENT Event;
    ANSI_STRING Name;
} KPH_LUA_HW_LOCK, *PKPH_LUA_HW_LOCK;

KPH_PAGED_FILE();

static LIST_ENTRY KphpLuaHwLocks;
static KPH_RWLOCK KphpLuaHwLocksLock;
static BOOLEAN KphpLuaHwInitialized = FALSE;

/**
 * \brief Initializes the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardware(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    InitializeListHead(&KphpLuaHwLocks);
    KphInitializeRWLock(&KphpLuaHwLocksLock);

    KphpLuaHwInitialized = TRUE;
}

/**
 * \brief Cleans up the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardware(
    VOID
    )
{
    PLIST_ENTRY entry;
    PKPH_LUA_HW_LOCK lock;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphpLuaHwInitialized)
    {
        return;
    }

    while (!IsListEmpty(&KphpLuaHwLocks))
    {
        entry = RemoveHeadList(&KphpLuaHwLocks);

        lock = CONTAINING_RECORD(entry, KPH_LUA_HW_LOCK, Entry);

        KphFree(lock, KPH_TAG_LUA_HARDWARE_LOCK);
    }

    KphDeleteRWLock(&KphpLuaHwLocksLock);
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
PKPH_LUA_HW_LOCK KphpLuaHwFindLock(
    _In_ PCANSI_STRING Name
    )
{
    PLIST_ENTRY entry;
    PKPH_LUA_HW_LOCK lock;
    SIZE_T size;

    KPH_PAGED_CODE_PASSIVE();

    lock = NULL;

    KphAcquireRWLockExclusive(&KphpLuaHwLocksLock);

    for (entry = KphpLuaHwLocks.Flink;
         entry != &KphpLuaHwLocks;
         entry = entry->Flink)
    {
        lock = CONTAINING_RECORD(entry, KPH_LUA_HW_LOCK, Entry);

        if (RtlEqualString(&lock->Name, Name, FALSE))
        {
            goto Exit;
        }
    }

    NT_VERIFY(NT_SUCCESS(RtlSIZETAdd(sizeof(KPH_LUA_HW_LOCK),
                                     Name->Length,
                                     &size)));

    lock = KphAllocateNPaged(size, KPH_TAG_LUA_HARDWARE_LOCK);
    if (!lock)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate lock %Z",
                      Name);

        goto Exit;
    }

    KeInitializeEvent(&lock->Event, SynchronizationEvent, TRUE);

    lock->Name.Buffer = Add2Ptr(lock, sizeof(KPH_LUA_HW_LOCK));
    lock->Name.Length = Name->Length;
    lock->Name.MaximumLength = Name->Length;
    RtlCopyMemory(lock->Name.Buffer, Name->Buffer, Name->Length);

    InsertTailList(&KphpLuaHwLocks, &lock->Entry);

Exit:

    KphReleaseRWLock(&KphpLuaHwLocksLock);

    return lock;
}

/**
 * \brief Releases a lock a script holds.
 *
 * \param[in] Context The lock.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwReleaseLock(
    _In_opt_ PVOID Context
    )
{
    PKPH_LUA_HW_LOCK lock;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    lock = Context;

    KeSetEvent(&lock->Event, IO_NO_INCREMENT, FALSE);

    KeLeaveCriticalRegion();
}

/**
 * \brief Takes a lock by name (hw.lock).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the lock object, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwLock(
    _In_ lua_State* State
    )
{
    NTSTATUS status;
    PCSTR buffer;
    size_t length;
    lua_Integer milliseconds;
    ANSI_STRING name;
    PKPH_LUA_HW_LOCK lock;

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

    lock = KphpLuaHwFindLock(&name);
    if (!lock)
    {
        lua_pushnil(State);
        lua_pushinteger(State,
                        (lua_Integer)(ULONG)STATUS_INSUFFICIENT_RESOURCES);
        return 2;
    }

    KeEnterCriticalRegion();

    status = KphScriptWait(State, &lock->Event, (ULONG)milliseconds);
    if (status == STATUS_SUCCESS)
    {
        KphScriptPushCleanup(State, KphpLuaHwReleaseLock, lock);
        return 1;
    }

    KeLeaveCriticalRegion();

    if (status == STATUS_SI_SCRIPT_LIMIT)
    {
        KphScriptCharge(State, 0);
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "KphScriptWait failed: %!STATUS!",
                  status);

    lua_pushnil(State);
    lua_pushinteger(State, (lua_Integer)(ULONG)status);
    return 2;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpLuaHwLibrary[] =
{
    { "lock", KphpLuaHwLock },
    { NULL, NULL }
};
KPH_PROTECTED_DATA_SECTION_RO_POP();

/**
 * \brief Opens the hardware library.
 *
 * \param[in] State The Lua state.
 *
 * \return One, the library table.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwOpen(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    luaL_newlib(State, KphpLuaHwLibrary);

    return 1;
}

/**
 * \brief Opens the hardware library in a Lua state, as the global hw.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareLibrary(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    luaL_requiref(State, KPH_LUA_HARDWARE_LIBRARY_NAME, KphpLuaHwOpen, 1);
    lua_settop(State, 0);
}
