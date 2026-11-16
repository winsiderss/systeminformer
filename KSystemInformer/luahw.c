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

#if defined(_M_X64)

typedef struct _KPH_LUA_HW_CPU
{
    CHAR Vendor[12];
    ULONG Family;
    ULONG Model;
    ULONG Stepping;
    ULONG Signature;
    ULONG PhysicalBits;
} KPH_LUA_HW_CPU, *PKPH_LUA_HW_CPU;

KPH_PROTECTED_DATA_SECTION_PUSH();
static KPH_LUA_HW_CPU KphpLuaHwCpuIdentity = { 0 };
KPH_PROTECTED_DATA_SECTION_POP();

static NPAGED_LOOKASIDE_LIST KphpLuaHwPinLookaside = { 0 };

#endif

KPH_PAGED_FILE();

static LIST_ENTRY KphpLuaHwLocks;
static KPH_RWLOCK KphpLuaHwLocksLock;
static BOOLEAN KphpLuaHwInitialized = FALSE;

#if defined(_M_X64)

/**
 * \brief Captures the identity of the processor.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaHwInitializeCpu(
    VOID
    )
{
    int basic[4];
    int version[4];
    int extended[4];
    int addresses[4];
    int vendor[3];
    ULONG signature;
    ULONG baseFamily;
    ULONG family;
    ULONG model;
    ULONG physicalBits;

    KPH_PAGED_CODE_PASSIVE();

    __cpuidex(basic, CPUID_BASIC, 0);
    __cpuidex(version, CPUID_VERSION, 0);
    __cpuidex(extended, CPUID_EXTENDED, 0);

    vendor[0] = basic[1];
    vendor[1] = basic[3];
    vendor[2] = basic[2];

    signature = (ULONG)version[0];

    baseFamily = (signature >> 8) & 0xF;

    family = baseFamily;
    if (baseFamily == 0xF)
    {
        family += (signature >> 20) & 0xFF;
    }

    model = (signature >> 4) & 0xF;
    if (baseFamily >= 0x6)
    {
        model |= ((signature >> 16) & 0xF) << 4;
    }

    physicalBits = 36;
    if ((ULONG)extended[0] >= CPUID_ADDRESS_SIZES)
    {
        __cpuidex(addresses, CPUID_ADDRESS_SIZES, 0);

        physicalBits = (ULONG)addresses[0] & 0xFF;
    }

    C_ASSERT(sizeof(vendor) == sizeof(KphpLuaHwCpuIdentity.Vendor));

    RtlCopyMemory(KphpLuaHwCpuIdentity.Vendor, vendor, sizeof(vendor));
    KphpLuaHwCpuIdentity.Family = family;
    KphpLuaHwCpuIdentity.Model = model;
    KphpLuaHwCpuIdentity.Stepping = signature & 0xF;
    KphpLuaHwCpuIdentity.Signature = signature;
    KphpLuaHwCpuIdentity.PhysicalBits = physicalBits;
}

#endif

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

#if defined(_M_X64)
    KphpLuaHwInitializeCpu();

    KphInitializeNPagedLookaside(&KphpLuaHwPinLookaside,
                                 sizeof(GROUP_AFFINITY),
                                 KPH_TAG_LUA_HARDWARE_PIN);
#endif

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

#if defined(_M_X64)
    KphDeleteNPagedLookaside(&KphpLuaHwPinLookaside);
#endif
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

#if defined(_M_X64)

/**
 * \brief Checks that an argument is an integer from 0 to ULONG_MAX.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The argument.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG KphpLuaHwCheckUlong(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    lua_Integer value;

    KPH_PAGED_CODE_PASSIVE();

    value = luaL_checkinteger(State, Index);

    luaL_argcheck(State,
                  (value >= 0) && (value <= ULONG_MAX),
                  Index,
                  "out of range");

    return (ULONG)value;
}

/**
 * \brief Executes CPUID on the current processor (hw.cpuid).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return Four, the registers EAX, EBX, ECX and EDX.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwCpuid(
    _In_ lua_State* State
    )
{
    ULONG leaf;
    ULONG subleaf;
    int registers[4];

    KPH_PAGED_CODE_PASSIVE();

    leaf = KphpLuaHwCheckUlong(State, 1);
    subleaf = KphpLuaHwCheckUlong(State, 2);

    __cpuidex(registers, (int)leaf, (int)subleaf);

    for (ULONG i = 0; i < ARRAYSIZE(registers); i++)
    {
        lua_pushinteger(State, (lua_Integer)(ULONG)registers[i]);
    }

    return ARRAYSIZE(registers);
}

/**
 * \brief Describes the processor, as captured when the driver loaded
 * (hw.cpu).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return One, a table of the vendor, the display family, model and
 * stepping, the signature and the physical address width.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwCpu(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    lua_createtable(State, 0, 6);

    lua_pushlstring(State,
                    KphpLuaHwCpuIdentity.Vendor,
                    sizeof(KphpLuaHwCpuIdentity.Vendor));
    lua_setfield(State, -2, "vendor");
    lua_pushinteger(State, KphpLuaHwCpuIdentity.Family);
    lua_setfield(State, -2, "family");
    lua_pushinteger(State, KphpLuaHwCpuIdentity.Model);
    lua_setfield(State, -2, "model");
    lua_pushinteger(State, KphpLuaHwCpuIdentity.Stepping);
    lua_setfield(State, -2, "stepping");
    lua_pushinteger(State, KphpLuaHwCpuIdentity.Signature);
    lua_setfield(State, -2, "signature");
    lua_pushinteger(State, KphpLuaHwCpuIdentity.PhysicalBits);
    lua_setfield(State, -2, "physical_bits");

    return 1;
}

/**
 * \brief Restores the affinity a pin replaced.
 *
 * \param[in] Context The affinity to restore.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwReleasePin(
    _In_opt_ PVOID Context
    )
{
    PGROUP_AFFINITY previous;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    previous = Context;

    KeRevertToUserGroupAffinityThread(previous);

    KphFreeToNPagedLookaside(&KphpLuaHwPinLookaside, previous);
}

/**
 * \brief Runs the calling thread on one processor until the pin is released
 * (hw.pin).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the pin object, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPin(
    _In_ lua_State* State
    )
{
    lua_Integer group;
    lua_Integer number;
    PROCESSOR_NUMBER processor;
    GROUP_AFFINITY affinity;
    PGROUP_AFFINITY previous;

    KPH_PAGED_CODE_PASSIVE();

    group = luaL_checkinteger(State, 1);
    number = luaL_checkinteger(State, 2);

    luaL_argcheck(State,
                  (group >= 0) && (group <= USHORT_MAX),
                  1,
                  "out of range");
    luaL_argcheck(State,
                  (number >= 0) && (number <= UCHAR_MAX),
                  2,
                  "out of range");

    RtlZeroMemory(&processor, sizeof(processor));
    processor.Group = (USHORT)group;
    processor.Number = (UCHAR)number;

    if (KeGetProcessorIndexFromNumber(&processor) == INVALID_PROCESSOR_INDEX)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Processor %lu:%lu is not active",
                      (ULONG)processor.Group,
                      (ULONG)processor.Number);

        lua_pushnil(State);
        lua_pushinteger(State, (lua_Integer)(ULONG)STATUS_INVALID_PARAMETER);
        return 2;
    }

    previous = KphAllocateFromNPagedLookaside(&KphpLuaHwPinLookaside);
    if (!previous)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate pin");

        lua_pushnil(State);
        lua_pushinteger(State,
                        (lua_Integer)(ULONG)STATUS_INSUFFICIENT_RESOURCES);
        return 2;
    }

    NT_ASSERT(sizeof(GROUP_AFFINITY) <= KphpLuaHwPinLookaside.L.Size);

    RtlZeroMemory(&affinity, sizeof(affinity));
    affinity.Group = processor.Group;
    affinity.Mask = (KAFFINITY)1 << processor.Number;

    KeSetSystemGroupAffinityThread(&affinity, previous);

    KphScriptPushCleanup(State, KphpLuaHwReleasePin, previous);

    return 1;
}

/**
 * \brief Reads an MSR of the current processor.
 *
 * \param[in] Index The MSR.
 * \param[out] Value Receives the value, zero when the read faults.
 *
 * \return TRUE when the read did not fault, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphpLuaHwReadMsr(
    _In_ ULONG Index,
    _Out_ PULONG64 Value
    )
{
    ULONG64 value;

    KPH_PAGED_CODE_PASSIVE();

    *Value = 0;

    __try
    {
        value = __readmsr(Index);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }

    *Value = value;

    return TRUE;
}

/**
 * \brief Writes an MSR of the current processor.
 *
 * \param[in] Index The MSR.
 * \param[in] Value The value.
 *
 * \return TRUE when the write did not fault, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphpLuaHwWriteMsr(
    _In_ ULONG Index,
    _In_ ULONG64 Value
    )
{
    KPH_PAGED_CODE_PASSIVE();

    __try
    {
        __writemsr(Index, Value);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief Reads an MSR of the current processor (hw.msr_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the value, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwMsrRead(
    _In_ lua_State* State
    )
{
    ULONG index;
    ULONG64 value;

    KPH_PAGED_CODE_PASSIVE();

    index = KphpLuaHwCheckUlong(State, 1);

    if (!KphpLuaHwReadMsr(index, &value))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "MSR 0x%08lx read faulted",
                      index);

        lua_pushnil(State);
        lua_pushinteger(State, (lua_Integer)(ULONG)STATUS_NOT_SUPPORTED);
        return 2;
    }

    lua_pushinteger(State, (lua_Integer)value);

    return 1;
}

/**
 * \brief Writes an MSR of the current processor (hw.msr_write).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: true, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwMsrWrite(
    _In_ lua_State* State
    )
{
    ULONG index;
    ULONG64 value;

    KPH_PAGED_CODE_PASSIVE();

    index = KphpLuaHwCheckUlong(State, 1);
    value = (ULONG64)luaL_checkinteger(State, 2);

    if (!KphpLuaHwWriteMsr(index, value))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "MSR 0x%08lx write faulted",
                      index);

        lua_pushnil(State);
        lua_pushinteger(State, (lua_Integer)(ULONG)STATUS_NOT_SUPPORTED);
        return 2;
    }

    lua_pushboolean(State, TRUE);

    return 1;
}

/**
 * \brief Checks the width argument of a port access: 1, 2 or 4.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The width in bytes.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG KphpLuaHwCheckWidth(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    lua_Integer width;

    KPH_PAGED_CODE_PASSIVE();

    width = luaL_checkinteger(State, Index);

    luaL_argcheck(State,
                  (width == 1) || (width == 2) || (width == 4),
                  Index,
                  "not 1, 2 or 4");

    return (ULONG)width;
}

/**
 * \brief Checks the port argument of a port access.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The port.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
USHORT KphpLuaHwCheckPort(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    lua_Integer port;

    KPH_PAGED_CODE_PASSIVE();

    port = luaL_checkinteger(State, Index);

    luaL_argcheck(State,
                  (port >= 0) && (port <= USHORT_MAX),
                  Index,
                  "out of range");

    return (USHORT)port;
}

/**
 * \brief Reads an I/O port (hw.port_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return One, the value read.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPortRead(
    _In_ lua_State* State
    )
{
    USHORT port;
    ULONG width;
    ULONG value;

    KPH_PAGED_CODE_PASSIVE();

    port = KphpLuaHwCheckPort(State, 1);
    width = KphpLuaHwCheckWidth(State, 2);

    switch (width)
    {
        case 1:
        {
            value = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)port);
            break;
        }
        case 2:
        {
            value = READ_PORT_USHORT((PUSHORT)(ULONG_PTR)port);
            break;
        }
        case 4:
        {
            value = READ_PORT_ULONG((PULONG)(ULONG_PTR)port);
            break;
        }
        DEFAULT_UNREACHABLE;
    }

    lua_pushinteger(State, (lua_Integer)value);

    return 1;
}

/**
 * \brief Writes an I/O port (hw.port_write).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return Zero, the number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPortWrite(
    _In_ lua_State* State
    )
{
    USHORT port;
    ULONG width;
    lua_Integer value;
    lua_Integer maximum;

    KPH_PAGED_CODE_PASSIVE();

    port = KphpLuaHwCheckPort(State, 1);
    width = KphpLuaHwCheckWidth(State, 2);
    value = luaL_checkinteger(State, 3);

    maximum = (lua_Integer)((1ull << (width * 8)) - 1);

    luaL_argcheck(State, (value >= 0) && (value <= maximum), 3, "out of range");

    switch (width)
    {
        case 1:
        {
            WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)port, (UCHAR)value);
            break;
        }
        case 2:
        {
            WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)port, (USHORT)value);
            break;
        }
        case 4:
        {
            WRITE_PORT_ULONG((PULONG)(ULONG_PTR)port, (ULONG)value);
            break;
        }
        DEFAULT_UNREACHABLE;
    }

    return 0;
}

#endif

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpLuaHwLibrary[] =
{
#if defined(_M_X64)
    { "cpu", KphpLuaHwCpu },
    { "cpuid", KphpLuaHwCpuid },
    { "msr_read", KphpLuaHwMsrRead },
    { "msr_write", KphpLuaHwMsrWrite },
    { "pin", KphpLuaHwPin },
    { "port_read", KphpLuaHwPortRead },
    { "port_write", KphpLuaHwPortWrite },
#endif
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
