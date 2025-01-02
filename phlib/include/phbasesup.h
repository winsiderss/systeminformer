/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#ifndef _PH_PHBASESUP_H
#define _PH_PHBASESUP_H

EXTERN_C_START

PHLIBAPI
BOOLEAN
NTAPI
PhBaseInitialization(
    VOID
    );

// Threads

#ifdef DEBUG
typedef struct _PH_AUTO_POOL *PPH_AUTO_POOL;

typedef struct _PHP_BASE_THREAD_DBG
{
    CLIENT_ID ClientId;
    LIST_ENTRY ListEntry;
    PVOID StartAddress;
    PVOID Parameter;

    PPH_AUTO_POOL CurrentAutoPool;
} PHP_BASE_THREAD_DBG, *PPHP_BASE_THREAD_DBG;

extern ULONG PhDbgThreadDbgTlsIndex;
extern LIST_ENTRY PhDbgThreadListHead;
extern PH_QUEUED_LOCK PhDbgThreadListLock;
#endif

PHLIBAPI
NTSTATUS
NTAPI
PhCreateUserThread(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG CreateFlags,
    _In_opt_ SIZE_T ZeroBits,
    _In_opt_ SIZE_T StackSize,
    _In_opt_ SIZE_T MaximumStackSize,
    _In_ PUSER_THREAD_START_ROUTINE StartRoutine,
    _In_opt_ PVOID Argument,
    _Out_opt_ PHANDLE ThreadHandle,
    _Out_opt_ PCLIENT_ID ClientId
    );

PHLIBAPI
HANDLE
NTAPI
PhCreateThread(
    _In_opt_ SIZE_T StackSize,
    _In_ PUSER_THREAD_START_ROUTINE StartAddress,
    _In_opt_ PVOID Parameter
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateThreadEx(
    _Out_ PHANDLE ThreadHandle,
    _In_ PUSER_THREAD_START_ROUTINE StartAddress,
    _In_opt_ PVOID Parameter
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateThread2(
    _In_ PUSER_THREAD_START_ROUTINE StartAddress,
    _In_opt_ PVOID Parameter
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueueUserWorkItem(
    _In_ PUSER_THREAD_START_ROUTINE StartRoutine,
    _In_opt_ PVOID Argument
    );

// Misc. system

PHLIBAPI
ULONGLONG
NTAPI
PhReadTimeStampCounter(
    VOID
    );

PHLIBAPI
BOOLEAN
NTAPI
PhQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER PerformanceCounter
    );

PHLIBAPI
BOOLEAN
NTAPI
PhQueryPerformanceFrequency(
    _Out_ PLARGE_INTEGER PerformanceFrequency
    );

FORCEINLINE
ULONGLONG
NTAPI
PhReadPerformanceCounter(
    VOID
    )
{
    LARGE_INTEGER counter;
    PhQueryPerformanceCounter(&counter);
    return (ULONGLONG)counter.QuadPart;
}

FORCEINLINE
ULONGLONG
NTAPI
PhReadPerformanceFrequency(
    VOID
    )
{
    LARGE_INTEGER counter;
    PhQueryPerformanceCounter(&counter);
    return (ULONGLONG)counter.QuadPart;
}

PHLIBAPI
VOID
NTAPI
PhQueryInterruptTime(
    _Out_ PULARGE_INTEGER InterruptTime
    );

PHLIBAPI
VOID
NTAPI
PhQuerySystemTime(
    _Out_ PLARGE_INTEGER SystemTime
    );

PHLIBAPI
VOID
NTAPI
PhQueryTimeZoneBias(
    _Out_ PLARGE_INTEGER TimeZoneBias
    );

PHLIBAPI
VOID
NTAPI
PhSystemTimeToLocalTime(
    _In_ PLARGE_INTEGER SystemTime,
    _Out_ PLARGE_INTEGER LocalTime
    );

PHLIBAPI
VOID
NTAPI
PhLocalTimeToSystemTime(
    _In_ PLARGE_INTEGER LocalTime,
    _Out_ PLARGE_INTEGER SystemTime
    );

PHLIBAPI
BOOLEAN
NTAPI
PhTimeToSecondsSince1980(
    _In_ PLARGE_INTEGER Time,
    _Out_ PULONG ElapsedSeconds
    );

PHLIBAPI
BOOLEAN
NTAPI
PhTimeToSecondsSince1970(
    _In_ PLARGE_INTEGER Time,
    _Out_ PULONG ElapsedSeconds
    );

PHLIBAPI
VOID
NTAPI
PhSecondsSince1980ToTime(
    _In_ ULONG ElapsedSeconds,
    _Out_ PLARGE_INTEGER Time
    );

PHLIBAPI
VOID
NTAPI
PhSecondsSince1970ToTime(
    _In_ ULONG ElapsedSeconds,
    _Out_ PLARGE_INTEGER Time
    );

// Heap

PHLIBAPI
_May_raise_
_Post_writable_byte_size_(Size)
DECLSPEC_ALLOCATOR
DECLSPEC_NOALIAS
DECLSPEC_RESTRICT
PVOID
NTAPI
PhAllocate(
    _In_ SIZE_T Size
    );

PHLIBAPI
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
DECLSPEC_ALLOCATOR
DECLSPEC_NOALIAS
DECLSPEC_RESTRICT
PVOID
NTAPI
PhAllocateSafe(
    _In_ SIZE_T Size
    );

PHLIBAPI
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
DECLSPEC_ALLOCATOR
DECLSPEC_NOALIAS
DECLSPEC_RESTRICT
PVOID
NTAPI
PhAllocateExSafe(
    _In_ SIZE_T Size,
    _In_ ULONG Flags
    );

PHLIBAPI
VOID
NTAPI
PhFree(
    _Frees_ptr_opt_ PVOID Memory
    );

PHLIBAPI
_May_raise_
_Post_writable_byte_size_(Size)
_Success_(return != NULL)
DECLSPEC_ALLOCATOR
DECLSPEC_NOALIAS
DECLSPEC_RESTRICT
PVOID
NTAPI
PhReAllocate(
    _Frees_ptr_opt_ PVOID Memory,
    _In_ SIZE_T Size
    );

PHLIBAPI
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
_Success_(return != NULL)
DECLSPEC_ALLOCATOR
DECLSPEC_NOALIAS
DECLSPEC_RESTRICT
PVOID
NTAPI
PhReAllocateSafe(
    _In_opt_ PVOID Memory,
    _In_ SIZE_T Size
    );

PHLIBAPI
SIZE_T
NTAPI
PhSizeHeap(
    _In_ PVOID Memory
    );

PHLIBAPI
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
_Success_(return != NULL)
DECLSPEC_ALLOCATOR
DECLSPEC_NOALIAS
DECLSPEC_RESTRICT
PVOID
NTAPI
PhAllocatePage(
    _In_ SIZE_T Size,
    _Out_opt_ PSIZE_T NewSize
    );

PHLIBAPI
VOID
NTAPI
PhFreePage(
    _In_ _Frees_ptr_ PVOID Memory
    );

PHLIBAPI
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
_Success_(return != NULL)
DECLSPEC_ALLOCATOR
DECLSPEC_NOALIAS
DECLSPEC_RESTRICT
PVOID
NTAPI
PhAllocatePageAligned(
    _In_ SIZE_T Size,
    _In_ SIZE_T Alignment
    );

PHLIBAPI
VOID
NTAPI
PhFreePageAligned(
    _In_ _Frees_ptr_ PVOID Memory
    );

PHLIBAPI
NTSTATUS
NTAPI
PhAllocateVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* BaseAddress,
    _In_ SIZE_T RegionSize,
    _In_ ULONG AllocationType,
    _In_ ULONG Protection
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFreeVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG FreeType
    );

FORCEINLINE
PVOID
PhAllocateCopy(
    _In_ PVOID Data,
    _In_ SIZE_T Size
    )
{
    PVOID copy;

    copy = PhAllocate(Size);
    memcpy(copy, Data, Size);

    return copy;
}

FORCEINLINE
PVOID
PhAllocateZero(
    _In_ SIZE_T Size
    )
{
    PVOID buffer;

    buffer = PhAllocate(Size);
    memset(buffer, 0, Size);

    return buffer;
}

FORCEINLINE
PVOID
PhAllocateZeroSafe(
    _In_ SIZE_T Size
    )
{
    PVOID buffer;

    if (buffer = PhAllocateSafe(Size))
    {
        memset(buffer, 0, Size);
        return buffer;
    }

    return NULL;
}

// Singly linked list

// rev from RtlInitializeSListHead (dmex)
FORCEINLINE
VOID
NTAPI
PhInitializeSListHead(
    _Out_ PSLIST_HEADER ListHead
    )
{
#if defined(PHNT_NATIVE_SLIST)
    RtlInitializeSListHead(ListHead);
#else
    if (IS_ALIGNED(ListHead, MEMORY_ALLOCATION_ALIGNMENT))
        memset(ListHead, 0, sizeof(SLIST_HEADER));
    else
        PhRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
#endif
}

// rev from RtlQueryDepthSList (dmex)
FORCEINLINE
USHORT
NTAPI
PhQueryDepthSList(
    _In_ PSLIST_HEADER ListHead
    )
{
#if defined(PHNT_NATIVE_SLIST)
    return RtlQueryDepthSList(ListHead);
#else
#ifdef _M_X64
    return (USHORT)ListHead->HeaderX64.Depth;
#elif _M_ARM64
    return (USHORT)ListHead->HeaderArm64.Depth;
#else
    return ListHead->Depth;
#endif
#endif
}

// Event

#define PH_EVENT_SET 0x1
#define PH_EVENT_SET_SHIFT 0
#define PH_EVENT_REFCOUNT_SHIFT 1
#define PH_EVENT_REFCOUNT_INC 0x2
#define PH_EVENT_REFCOUNT_MASK (((ULONG_PTR)1 << 15) - 1)

/**
 * A fast event object.
 *
 * \remarks This event object does not use a kernel event object until necessary, and frees the
 * object automatically when it is no longer needed.
 */
typedef struct _PH_EVENT
{
    union
    {
        ULONG_PTR Value;
        struct
        {
            USHORT Set : 1;
            USHORT RefCount : 15;
            UCHAR Reserved;
            UCHAR AvailableForUse;
#ifdef _WIN64
            ULONG Spare;
#endif
        };
    };
    HANDLE EventHandle;
} PH_EVENT, *PPH_EVENT;

static_assert(sizeof(PH_EVENT) == sizeof(ULONG_PTR) + sizeof(HANDLE));

#define PH_EVENT_INIT { { PH_EVENT_REFCOUNT_INC }, NULL }

PHLIBAPI
VOID
FASTCALL
PhfInitializeEvent(
    _Out_ PPH_EVENT Event
    );

#define PhSetEvent PhfSetEvent
PHLIBAPI
VOID
FASTCALL
PhfSetEvent(
    _Inout_ PPH_EVENT Event
    );

PHLIBAPI
BOOLEAN
FASTCALL
PhfWaitForEvent(
    _Inout_ PPH_EVENT Event,
    _In_opt_ PLARGE_INTEGER Timeout
    );

FORCEINLINE
BOOLEAN
PhWaitForEvent(
    _Inout_ PPH_EVENT Event,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    if (Event->Set)
        return TRUE;

    return PhfWaitForEvent(Event, Timeout);
}

#define PhResetEvent PhfResetEvent
PHLIBAPI
VOID
FASTCALL
PhfResetEvent(
    _Inout_ PPH_EVENT Event
    );

FORCEINLINE
VOID
PhInitializeEvent(
    _Out_ PPH_EVENT Event
    )
{
    WriteULongPtrRelease(&Event->Value, PH_EVENT_REFCOUNT_INC);
    WritePointerRelease(&Event->EventHandle, UlongToPtr(0));
}

/**
 * Determines whether an event object is set.
 *
 * \param Event A pointer to an event object.
 *
 * \return TRUE if the event object is set, otherwise FALSE.
 */
FORCEINLINE
BOOLEAN
PhTestEvent(
    _In_ PPH_EVENT Event
    )
{
    return (BOOLEAN)Event->Set;
}

// Barrier

#define PH_BARRIER_COUNT_SHIFT 0
#define PH_BARRIER_COUNT_MASK (((LONG_PTR)1 << (sizeof(ULONG_PTR) * 8 / 2 - 1)) - 1)
#define PH_BARRIER_COUNT_INC ((LONG_PTR)1 << PH_BARRIER_COUNT_SHIFT)
#define PH_BARRIER_TARGET_SHIFT (sizeof(ULONG_PTR) * 8 / 2)
#define PH_BARRIER_TARGET_MASK (((LONG_PTR)1 << (sizeof(ULONG_PTR) * 8 / 2 - 1)) - 1)
#define PH_BARRIER_TARGET_INC ((LONG_PTR)1 << PH_BARRIER_TARGET_SHIFT)
#define PH_BARRIER_WAKING ((LONG_PTR)1 << (sizeof(ULONG_PTR) * 8 - 1))

#define PH_BARRIER_MASTER 1
#define PH_BARRIER_SLAVE 2
#define PH_BARRIER_OBSERVER 3

typedef struct _PH_BARRIER
{
    ULONG_PTR Value;
    PH_WAKE_EVENT WakeEvent;
} PH_BARRIER, *PPH_BARRIER;

#define PH_BARRIER_INIT(Target) { (ULONG_PTR)(Target) << PH_BARRIER_TARGET_SHIFT, PH_WAKE_EVENT_INIT }

PHLIBAPI
VOID
FASTCALL
PhfInitializeBarrier(
    _Out_ PPH_BARRIER Barrier,
    _In_ ULONG_PTR Target
    );

#define PhWaitForBarrier PhfWaitForBarrier
PHLIBAPI
BOOLEAN
FASTCALL
PhfWaitForBarrier(
    _Inout_ PPH_BARRIER Barrier,
    _In_ BOOLEAN Spin
    );

FORCEINLINE
VOID
PhInitializeBarrier(
    _Out_ PPH_BARRIER Barrier,
    _In_ ULONG_PTR Target
    )
{
    Barrier->Value = Target << PH_BARRIER_TARGET_SHIFT;
    PhInitializeQueuedLock(&Barrier->WakeEvent);
}

// Rundown protection

#define PH_RUNDOWN_ACTIVE 0x1
#define PH_RUNDOWN_REF_SHIFT 1
#define PH_RUNDOWN_REF_INC 0x2

typedef struct _PH_RUNDOWN_PROTECT
{
    ULONG_PTR Value;
} PH_RUNDOWN_PROTECT, *PPH_RUNDOWN_PROTECT;

#define PH_RUNDOWN_PROTECT_INIT { 0 }

typedef struct _PH_RUNDOWN_WAIT_BLOCK
{
    ULONG_PTR Count;
    PH_EVENT WakeEvent;
} PH_RUNDOWN_WAIT_BLOCK, *PPH_RUNDOWN_WAIT_BLOCK;

PHLIBAPI
VOID
FASTCALL
PhfInitializeRundownProtection(
    _Out_ PPH_RUNDOWN_PROTECT Protection
    );

PHLIBAPI
BOOLEAN
FASTCALL
PhfAcquireRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    );

PHLIBAPI
VOID
FASTCALL
PhfReleaseRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    );

PHLIBAPI
VOID
FASTCALL
PhfWaitForRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    );

FORCEINLINE
VOID
PhInitializeRundownProtection(
    _Out_ PPH_RUNDOWN_PROTECT Protection
    )
{
    Protection->Value = 0;
}

FORCEINLINE
BOOLEAN
PhAcquireRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;

    value = ReadULongPtrAcquire(&Protection->Value) & ~PH_RUNDOWN_ACTIVE; // fail fast path when rundown is active

    if ((ULONG_PTR)_InterlockedCompareExchangePointer(
        (PVOID *)&Protection->Value,
        (PVOID)(value + PH_RUNDOWN_REF_INC),
        (PVOID)value
        ) == value)
    {
        return TRUE;
    }
    else
    {
        return PhfAcquireRundownProtection(Protection);
    }
}

FORCEINLINE
VOID
PhReleaseRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;

    value = ReadULongPtrAcquire(&Protection->Value) & ~PH_RUNDOWN_ACTIVE; // Fail fast path when rundown is active

    if ((ULONG_PTR)_InterlockedCompareExchangePointer(
        (PVOID *)&Protection->Value,
        (PVOID)(value - PH_RUNDOWN_REF_INC),
        (PVOID)value
        ) != value)
    {
        PhfReleaseRundownProtection(Protection);
    }
}

FORCEINLINE
VOID
PhWaitForRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;

    value = (ULONG_PTR)_InterlockedCompareExchangePointer(
        (PVOID *)&Protection->Value,
        (PVOID)PH_RUNDOWN_ACTIVE,
        (PVOID)0
        );

    if (value != 0 && value != PH_RUNDOWN_ACTIVE)
        PhfWaitForRundownProtection(Protection);
}

// One-time initialization

#define PH_INITONCE_SHIFT 31
#define PH_INITONCE_INITIALIZING (0x1 << PH_INITONCE_SHIFT)
#define PH_INITONCE_INITIALIZING_SHIFT PH_INITONCE_SHIFT

typedef struct _PH_INITONCE
{
    PH_EVENT Event;
} PH_INITONCE, *PPH_INITONCE;

C_ASSERT(PH_INITONCE_SHIFT >= FIELD_OFFSET(PH_EVENT, AvailableForUse) * 8);

#define PH_INITONCE_INIT { PH_EVENT_INIT }

#define PhInitializeInitOnce PhfInitializeInitOnce
PHLIBAPI
VOID
FASTCALL
PhfInitializeInitOnce(
    _Out_ PPH_INITONCE InitOnce
    );

PHLIBAPI
BOOLEAN
FASTCALL
PhfBeginInitOnce(
    _Inout_ PPH_INITONCE InitOnce
    );

#define PhEndInitOnce PhfEndInitOnce
PHLIBAPI
VOID
FASTCALL
PhfEndInitOnce(
    _Inout_ PPH_INITONCE InitOnce
    );

FORCEINLINE
BOOLEAN
PhBeginInitOnce(
    _Inout_ PPH_INITONCE InitOnce
    )
{
    if (InitOnce->Event.Set)
        return FALSE;
    else
        return PhfBeginInitOnce(InitOnce);
}

FORCEINLINE
BOOLEAN
PhTestInitOnce(
    _In_ PPH_INITONCE InitOnce
    )
{
    return (BOOLEAN)InitOnce->Event.Set;
}

// String

PHLIBAPI
SIZE_T
NTAPI
PhCountStringZ(
    _In_ PCWSTR String
    );

FORCEINLINE
SIZE_T
PhCountBytesZ(
    _In_ PCSTR String
    )
{
    return (SIZE_T)strlen(String);
}

PHLIBAPI
PSTR
NTAPI
PhDuplicateBytesZ(
    _In_ PCSTR String
    );

PHLIBAPI
PSTR
NTAPI
PhDuplicateBytesZSafe(
    _In_ PCSTR String
    );

PHLIBAPI
PWSTR
NTAPI
PhDuplicateStringZ(
    _In_ PCWSTR String
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhCopyBytesZ(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhCopyStringZ(
    _In_ PCWSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhCopyStringZFromBytes(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhCopyStringZFromMultiByte(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhCopyStringZFromUtf8(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    );

PHLIBAPI
LONG
NTAPI
PhCompareStringZNatural(
    _In_ PCWSTR A,
    _In_ PCWSTR B,
    _In_ BOOLEAN IgnoreCase
    );

FORCEINLINE
BOOLEAN
PhAreCharactersDifferent(
    _In_ WCHAR Char1,
    _In_ WCHAR Char2
    )
{
    WCHAR d;

    d = Char1 ^ Char2;

    // We ignore bits beyond bit 5 because bit 6 is the case bit, and also we
    // don't support localization here.
    if (d & 0x1f)
        return TRUE;

    return FALSE;
}

FORCEINLINE
BOOLEAN
PhIsDigitCharacter(
    _In_ WCHAR Char
    )
{
    return (USHORT)(Char - '0') < 10;
}

FORCEINLINE
LONG
PhCompareBytesZ(
    _In_ PCSTR String1,
    _In_ PCSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (IgnoreCase)
        return _stricmp(String1, String2);
    else
        return strcmp(String1, String2);
}

FORCEINLINE
BOOLEAN
PhEqualBytesZ(
    _In_ PCSTR String1,
    _In_ PCSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (IgnoreCase)
        return _stricmp(String1, String2) == 0;
    else
        return strcmp(String1, String2) == 0;
}

FORCEINLINE
LONG
PhCompareStringZ(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (IgnoreCase)
        return _wcsicmp(String1, String2);
    else
        return wcscmp(String1, String2);
}

FORCEINLINE
BOOLEAN
PhEqualStringZ(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (IgnoreCase)
    {
        // wcsicmp is very expensive, so we do a quick check for negatives first.
        if (PhAreCharactersDifferent(String1[0], String2[0]))
            return FALSE;

        return _wcsicmp(String1, String2) == 0;
    }
    else
    {
        return wcscmp(String1, String2) == 0;
    }
}

typedef struct _PH_STRINGREF
{
    /** The length, in bytes, of the string. */
    SIZE_T Length;
    /** The buffer containing the contents of the string. */
    PWCH Buffer;
} PH_STRINGREF, *PPH_STRINGREF;

typedef struct _PH_BYTESREF
{
    /** The length, in bytes, of the string. */
    SIZE_T Length;
    /** The buffer containing the contents of the string. */
    PCH Buffer;
} PH_BYTESREF, *PPH_BYTESREF;

typedef struct _PH_RELATIVE_BYTESREF
{
    /** The length, in bytes, of the string. */
    ULONG Length;
    /** A user-defined offset. */
    ULONG Offset;
} PH_RELATIVE_BYTESREF, *PPH_RELATIVE_BYTESREF, PH_RELATIVE_STRINGREF, *PPH_RELATIVE_STRINGREF;

#ifdef __cplusplus
#define PH_STRINGREF_INIT(String) { sizeof(String) - sizeof(UNICODE_NULL), const_cast<PWCH>(String) }
#define PH_BYTESREF_INIT(String) { sizeof(String) - sizeof(ANSI_NULL), const_cast<PCH>(String) }
#else
#define PH_STRINGREF_INIT(String) { sizeof(String) - sizeof(UNICODE_NULL), (String) }
#define PH_BYTESREF_INIT(String) { sizeof(String) - sizeof(ANSI_NULL), (String) }
#endif

FORCEINLINE
VOID
PhInitializeStringRef(
    _Out_ PPH_STRINGREF String,
    _In_ PCWSTR Buffer
    )
{
    String->Length = wcslen(Buffer) * sizeof(WCHAR);
    String->Buffer = (PWCH)Buffer;
}

FORCEINLINE
VOID
PhInitializeStringRefLongHint(
    _Out_ PPH_STRINGREF String,
    _In_ PCWSTR Buffer
    )
{
    String->Length = PhCountStringZ(Buffer) * sizeof(WCHAR);
    String->Buffer = (PWCH)Buffer;
}

FORCEINLINE
VOID
PhInitializeBytesRef(
    _Out_ PPH_BYTESREF Bytes,
    _In_ PCSTR Buffer
    )
{
    Bytes->Length = strlen(Buffer) * sizeof(CHAR);
    Bytes->Buffer = (PCH)Buffer;
}

FORCEINLINE
VOID
PhInitializeEmptyStringRef(
    _Out_ PPH_STRINGREF String
    )
{
    String->Length = 0;
    String->Buffer = NULL;
}

FORCEINLINE
VOID
NTAPI
PhInitializeBufferStringRef(
    _Out_ PPH_STRINGREF String,
    _Writable_bytes_(Length) _When_(Length != 0, _Notnull_) PWCHAR Buffer,
    _In_ SIZE_T Length
    )
{
    memset(String, 0, sizeof(PH_STRINGREF));
    String->Length = Length;
    String->Buffer = Buffer;
}

FORCEINLINE
BOOLEAN
PhStringRefToUnicodeString(
    _In_ PPH_STRINGREF String,
    _Out_ PUNICODE_STRING UnicodeString
    )
{
    UnicodeString->Length = (USHORT)String->Length;
    UnicodeString->MaximumLength = (USHORT)String->Length + sizeof(UNICODE_NULL);
    UnicodeString->Buffer = String->Buffer;

    return String->Length <= UNICODE_STRING_MAX_BYTES;
}

FORCEINLINE
VOID
PhUnicodeStringToStringRef(
    _In_ PUNICODE_STRING UnicodeString,
    _Out_ PPH_STRINGREF String
    )
{
    String->Length = UnicodeString->Length;
    String->Buffer = UnicodeString->Buffer;
}

PHLIBAPI
LONG
NTAPI
PhCompareStringRef(
    _In_ PPH_STRINGREF String1,
    _In_ PPH_STRINGREF String2,
    _In_ BOOLEAN IgnoreCase
    );

PHLIBAPI
BOOLEAN
NTAPI
PhEqualStringRef(
    _In_ PPH_STRINGREF String1,
    _In_ PPH_STRINGREF String2,
    _In_ BOOLEAN IgnoreCase
    );

PHLIBAPI
ULONG_PTR
NTAPI
PhFindCharInStringRef(
    _In_ PPH_STRINGREF String,
    _In_ WCHAR Character,
    _In_ BOOLEAN IgnoreCase
    );

PHLIBAPI
ULONG_PTR
NTAPI
PhFindLastCharInStringRef(
    _In_ PPH_STRINGREF String,
    _In_ WCHAR Character,
    _In_ BOOLEAN IgnoreCase
    );

PHLIBAPI
ULONG_PTR
NTAPI
PhFindStringInStringRef(
    _In_ PPH_STRINGREF String,
    _In_ PPH_STRINGREF SubString,
    _In_ BOOLEAN IgnoreCase
    );

FORCEINLINE
ULONG_PTR
PhFindStringInStringRefZ(
    _In_ PPH_STRINGREF String,
    _In_ PCWSTR SubString,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr2;

    PhInitializeStringRef(&sr2, SubString);

    return PhFindStringInStringRef(String, &sr2, IgnoreCase);
}

PHLIBAPI
BOOLEAN
NTAPI
PhSplitStringRefAtChar(
    _In_ PPH_STRINGREF Input,
    _In_ WCHAR Separator,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSplitStringRefAtLastChar(
    _In_ PPH_STRINGREF Input,
    _In_ WCHAR Separator,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSplitStringRefAtString(
    _In_ PPH_STRINGREF Input,
    _In_ PPH_STRINGREF Separator,
    _In_ BOOLEAN IgnoreCase,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart
    );

#define PH_SPLIT_AT_CHAR_SET 0x0 // default
#define PH_SPLIT_AT_STRING 0x1
#define PH_SPLIT_AT_RANGE 0x2
#define PH_SPLIT_CASE_INSENSITIVE 0x1000
#define PH_SPLIT_COMPLEMENT_CHAR_SET 0x2000
#define PH_SPLIT_START_AT_END 0x4000
#define PH_SPLIT_CHAR_SET_IS_UPPERCASE 0x8000

PHLIBAPI
BOOLEAN
NTAPI
PhSplitStringRefEx(
    _In_ PPH_STRINGREF Input,
    _In_ PPH_STRINGREF Separator,
    _In_ ULONG Flags,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart,
    _Out_opt_ PPH_STRINGREF SeparatorPart
    );

#define PH_TRIM_START_ONLY 0x1
#define PH_TRIM_END_ONLY 0x2

PHLIBAPI
VOID
NTAPI
PhTrimStringRef(
    _Inout_ PPH_STRINGREF String,
    _In_ PPH_STRINGREF CharSet,
    _In_ ULONG Flags
    );

FORCEINLINE
LONG
PhCompareStringRef2(
    _In_ PPH_STRINGREF String1,
    _In_ PCWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr2;

    PhInitializeStringRef(&sr2, String2);

    return PhCompareStringRef(String1, &sr2, IgnoreCase);
}

FORCEINLINE
BOOLEAN
PhEqualStringRef2(
    _In_ PPH_STRINGREF String1,
    _In_ PCWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr2;

    PhInitializeStringRef(&sr2, String2);

    return PhEqualStringRef(String1, &sr2, IgnoreCase);
}

FORCEINLINE
BOOLEAN
PhStartsWithStringRef(
    _In_ PPH_STRINGREF String,
    _In_ PPH_STRINGREF Prefix,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr;

    sr.Buffer = String->Buffer;
    sr.Length = Prefix->Length;

    if (String->Length < sr.Length)
        return FALSE;

    return PhEqualStringRef(&sr, Prefix, IgnoreCase);
}

FORCEINLINE
BOOLEAN
PhStartsWithStringRef2(
    _In_ PPH_STRINGREF String,
    _In_ PCWSTR Prefix,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF prefix;

    PhInitializeStringRef(&prefix, Prefix);

    return PhStartsWithStringRef(String, &prefix, IgnoreCase);
}

FORCEINLINE
BOOLEAN
PhEndsWithStringRef(
    _In_ PPH_STRINGREF String,
    _In_ PPH_STRINGREF Suffix,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr;

    if (Suffix->Length > String->Length)
        return FALSE;

    sr.Buffer = (PWCH)PTR_ADD_OFFSET(String->Buffer, String->Length - Suffix->Length);
    sr.Length = Suffix->Length;

    return PhEqualStringRef(&sr, Suffix, IgnoreCase);
}

FORCEINLINE
BOOLEAN
PhEndsWithStringRef2(
    _In_ PPH_STRINGREF String,
    _In_ PCWSTR Suffix,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF suffix;

    PhInitializeStringRef(&suffix, Suffix);

    return PhEndsWithStringRef(String, &suffix, IgnoreCase);
}

FORCEINLINE
VOID
PhSkipStringRef(
    _Inout_ PPH_STRINGREF String,
    _In_ SIZE_T Length
    )
{
    String->Buffer = (PWCH)PTR_ADD_OFFSET(String->Buffer, Length);
    String->Length -= Length;
}

FORCEINLINE
VOID
PhReverseStringRef(
    _In_ PPH_STRINGREF String
    )
{
    SIZE_T i;
    SIZE_T j;
    WCHAR t;

    for (i = 0, j = String->Length / sizeof(WCHAR) - 1; i <= j; i++, j--)
    {
        t = String->Buffer[i];
        String->Buffer[i] = String->Buffer[j];
        String->Buffer[j] = t;
    }
}

extern PPH_OBJECT_TYPE PhStringType;

/**
 * A 16-bit string object, which supports UTF-16.
 *
 * \remarks The \a Length never includes the null terminator. Every string must have a null
 * terminator at the end, for compatibility reasons. The invariant is:
 * \code Buffer[Length / sizeof(WCHAR)] = 0 \endcode
 */
typedef struct _PH_STRING
{
    // Header
    union
    {
        PH_STRINGREF sr;
        struct
        {
            /** The length, in bytes, of the string. */
            SIZE_T Length;
            /** The buffer containing the contents of the string. */
            PWCH Buffer;
        };
    };

    // Data
    union
    {
        WCHAR Data[1];
        struct
        {
            /** Reserved. */
            ULONG AllocationFlags;
            /** Reserved. */
            PVOID Allocation;
        };
    };
} PH_STRING, *PPH_STRING;

PHLIBAPI
PPH_STRING
NTAPI
PhCreateStringEx(
    _In_opt_ PCWCHAR Buffer,
    _In_ SIZE_T Length
    );

/**
 * Creates a string object from an existing null-terminated string.
 *
 * \param Buffer A null-terminated Unicode string.
 */
FORCEINLINE
PPH_STRING
NTAPI
PhCreateString(
    _In_ PCWSTR Buffer
    )
{
    return PhCreateStringEx(Buffer, PhCountStringZ(Buffer) * sizeof(WCHAR));
}

PHLIBAPI
PPH_STRING
NTAPI
PhReferenceEmptyString(
    VOID
    );

FORCEINLINE
PPH_STRING
PhCreateString2(
    _In_ PPH_STRINGREF String
    )
{
    if (String->Length == 0)
        return PhReferenceEmptyString();

    return PhCreateStringEx(String->Buffer, String->Length);
}

FORCEINLINE
PPH_STRING
NTAPI
PhCreateStringZ(
    _In_ PCWSTR String
    )
{
    PH_STRINGREF string;

    string.Length = wcslen(String) * sizeof(WCHAR);
    string.Buffer = (PWSTR)String;
    //PhInitializeStringRef(&string, (PWSTR)String);

    return PhCreateString2(&string);
}

#define PH_STRING_TRIM_START_ONLY PH_TRIM_START_ONLY
#define PH_STRING_TRIM_END_ONLY   PH_TRIM_END_ONLY
#define PH_STRING_TRIM_MASK       (PH_STRING_TRIM_START_ONLY | PH_STRING_TRIM_END_ONLY)
#define PH_STRING_UPPER_CASE      0x4
#define PH_STRING_LOWER_CASE      0x8
#define PH_STRING_CASE_MASK       (PH_STRING_UPPER_CASE | PH_STRING_LOWER_CASE)

PHLIBAPI
PPH_STRING
NTAPI
PhCreateString3(
    _In_ PPH_STRINGREF String,
    _In_ ULONG Flags,
    _In_opt_ PPH_STRINGREF TrimCharSet
    );

FORCEINLINE
PPH_STRING
NTAPI
PhTrimStringZ(
    _In_ PPH_STRINGREF String,
    _In_ ULONG Flags,
    _In_ PCWSTR TrimCharSet
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, TrimCharSet);

    return PhCreateString3(String, Flags, &string);
}

FORCEINLINE
WCHAR
NTAPI
PhUpcaseUnicodeChar(
    _In_ WCHAR SourceCharacter
    )
{
    WCHAR c;

    //c = towupper(c);
    //c = __ascii_towupper(c);
    c = RtlUpcaseUnicodeChar(SourceCharacter);

    return c;
}

FORCEINLINE
WCHAR
NTAPI
PhDowncaseUnicodeChar(
    _In_ WCHAR SourceCharacter
    )
{
    WCHAR c;

    c = RtlDowncaseUnicodeChar(SourceCharacter);

    return c;
}

FORCEINLINE
VOID
PhUpperStringRefInto(
    _Inout_ PPH_STRINGREF Destination,
    _In_ PPH_STRINGREF Source
    )
{
    assert(Destination->Length >= Source->Length);
    SIZE_T numberOfChars = Source->Length / sizeof(WCHAR);
    for (SIZE_T i = 0; i < numberOfChars; i++)
        Destination->Buffer[i] = PhUpcaseUnicodeChar(Source->Buffer[i]);
    Destination->Length = Source->Length;
}

FORCEINLINE
VOID
PhUpperStringRef(
    _Inout_ PPH_STRINGREF String
    )
{
    PhUpperStringRefInto(String, String);
}

FORCEINLINE
VOID
PhLowerStringRefInto(
    _Inout_ PPH_STRINGREF Destination,
    _In_ PPH_STRINGREF Source
    )
{
    assert(Destination->Length >= Source->Length);
    SIZE_T numberOfChars = Source->Length / sizeof(WCHAR);
    for (SIZE_T i = 0; i < numberOfChars; i++)
        Destination->Buffer[i] = PhDowncaseUnicodeChar(Source->Buffer[i]);
    Destination->Length = Source->Length;
}

FORCEINLINE
VOID
PhLowerStringRef(
    _Inout_ PPH_STRINGREF String
    )
{
    PhLowerStringRefInto(String, String);
}

FORCEINLINE
PPH_STRING
PhCreateStringFromUnicodeString(
    _In_ PUNICODE_STRING UnicodeString
    )
{
    if (UnicodeString->Length == 0)
        return PhReferenceEmptyString();

    return PhCreateStringEx(UnicodeString->Buffer, UnicodeString->Length);
}

PHLIBAPI
PPH_STRING
NTAPI
PhConcatStrings(
    _In_ ULONG Count,
    ...
    );

#define PH_CONCAT_STRINGS_LENGTH_CACHE_SIZE 16

PHLIBAPI
PPH_STRING
NTAPI
PhConcatStrings_V(
    _In_ ULONG Count,
    _In_ va_list ArgPtr
    );

PHLIBAPI
PPH_STRING
NTAPI
PhConcatStrings2(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2
    );

PHLIBAPI
PPH_STRING
NTAPI
PhConcatStringRef2(
    _In_ PPH_STRINGREF String1,
    _In_ PPH_STRINGREF String2
    );

PHLIBAPI
PPH_STRING
NTAPI
PhConcatStringRef3(
    _In_ PPH_STRINGREF String1,
    _In_ PPH_STRINGREF String2,
    _In_ PPH_STRINGREF String3
    );

PHLIBAPI
PPH_STRING
NTAPI
PhConcatStringRef4(
    _In_ PPH_STRINGREF String1,
    _In_ PPH_STRINGREF String2,
    _In_ PPH_STRINGREF String3,
    _In_ PPH_STRINGREF String4
    );

FORCEINLINE
PPH_STRING
PhConcatStringRefZ(
    _In_ PPH_STRINGREF String1,
    _In_ PCWSTR String2
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, String2);

    return PhConcatStringRef2(String1, &string);
}

PHLIBAPI
PPH_STRING
NTAPI
PhFormatString(
    _In_ _Printf_format_string_ PCWSTR Format,
    ...
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatString_V(
    _In_ _Printf_format_string_ PCWSTR Format,
    _In_ va_list ArgPtr
    );

/**
 * Retrieves a pointer to a string object's buffer or returns NULL.
 *
 * \param String A pointer to a string object.
 *
 * \return A pointer to the string object's buffer if the supplied pointer is non-NULL, otherwise
 * NULL.
 */
FORCEINLINE
PWSTR
PhGetString(
    _In_opt_ PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return NULL;
}

/**
 * Retrieves a string reference from a given string object.
 *
 * This function returns a `PH_STRINGREF` structure that references the string
 * contained in the provided `PPH_STRING` object. If the input string object is
 * `NULL`, it initializes and returns an empty `PH_STRINGREF`.
 *
 * @param String A pointer to a `PPH_STRING` object. This parameter is optional and can be `NULL`.
 * @return A `PH_STRINGREF` structure that references the string in the provided `PPH_STRING` object,
 *         or an empty `PH_STRINGREF` if the input is `NULL`.
 */
FORCEINLINE
PH_STRINGREF
PhGetStringRef(
    _In_opt_ PPH_STRING String
    )
{
    PH_STRINGREF sr;

    if (String)
        sr = String->sr;
    else
        PhInitializeEmptyStringRef(&sr);

    return sr;
}

/**
 * Retrieves the buffer from a PPH_STRINGREF structure or returns an empty string if the input is NULL.
 *
 * This function checks if the provided PPH_STRINGREF structure is not NULL. If it is not NULL, it returns the buffer
 * contained within the structure. If the structure is NULL, it returns an empty string.
 *
 * @param String A pointer to a PPH_STRINGREF structure. This parameter is optional and can be NULL.
 * @return A pointer to the buffer contained within the PPH_STRINGREF structure if it is not NULL, otherwise an empty string.
 */
FORCEINLINE
PCWSTR
PhGetStringRefZ(
    _In_opt_ PPH_STRINGREF String
    )
{
    if (String)
        return String->Buffer;
    else
        return L"";
}

/**
 * Retrieves a pointer to a string object's buffer or returns an empty string.
 *
 * \param String A pointer to a string object.
 * \return A pointer to the string object's buffer if the supplied pointer is non-NULL, otherwise an empty string.
 */
FORCEINLINE
PCWSTR
PhGetStringOrEmpty(
    _In_opt_ PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return L"";
}

/**
 * Retrieves a pointer to a string object's buffer or returns the specified alternative string.
 *
 * \param String A pointer to a string object.
 * \param DefaultString The alternative string.
 *
 * \return A pointer to the string object's buffer if the supplied pointer is non-NULL, otherwise
 * the specified alternative string.
 */
FORCEINLINE
PCWSTR
PhGetStringOrDefault(
    _In_opt_ PPH_STRING String,
    _In_ PCWSTR DefaultString
    )
{
    if (String)
        return String->Buffer;
    else
        return DefaultString;
}

/**
 * Determines whether a string is null or empty.
 *
 * \param String A pointer to a string object.
 */
//_Check_return_
//_Success_(((String != NULL && String->Length != 0) && return == 1))
//_When_(String != NULL && String->Length != 0 && return == 1, _At_(String, _Post_valid_ _Post_notnull_))
//FORCEINLINE
//BOOLEAN
//PhIsNullOrEmptyString(
//    _In_opt_ PPH_STRING String
//    )
//{
//    return !String || String->Length == 0;
//}

FORCEINLINE
BOOLEAN
PhIsNullOrEmptyStringRef(
    _Pre_maybenull_ PPH_STRINGREF String
    )
{
    return !(String && String->Length);
}

// VS2019 can't parse the inline bool check for the above PhIsNullOrEmptyString
// inline function creating invalid C6387 warnings using the input string (dmex)
#define PhIsNullOrEmptyString(string) \
    (!(string) || (string)->Length == 0)

/**
 * Duplicates a string.
 *
 * \param String A string to duplicate.
 */
FORCEINLINE
PPH_STRING
PhDuplicateString(
    _In_ PPH_STRING String
    )
{
    return PhCreateStringEx(String->Buffer, String->Length);
}

/**
 * Compares two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
FORCEINLINE
LONG
PhCompareString(
    _In_ PPH_STRING String1,
    _In_ PPH_STRING String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (IgnoreCase)
        return PhCompareStringRef(&String1->sr, &String2->sr, IgnoreCase); // faster than wcsicmp
    else
        return wcscmp(String1->Buffer, String2->Buffer);
}

/**
 * Compares two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
FORCEINLINE
LONG
PhCompareString2(
    _In_ PPH_STRING String1,
    _In_ PCWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (IgnoreCase)
        return PhCompareStringRef2(&String1->sr, String2, IgnoreCase);
    else
        return wcscmp(String1->Buffer, String2);
}

/**
 * Compares two strings, handling NULL strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
FORCEINLINE
LONG
PhCompareStringWithNull(
    _In_opt_ PPH_STRING String1,
    _In_opt_ PPH_STRING String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (String1 && String2)
    {
        return PhCompareString(String1, String2, IgnoreCase);
    }
    else if (!String1)
    {
        return !String2 ? 0 : 1;
    }
    else
    {
        return -1;
    }
}

/**
 * Compares two strings with support for null values and customizable sort order.
 *
 * This function compares two strings, taking into account the possibility of null values and allowing for customizable sort order.
 * If both `String1` and `String2` are not null, the function calls `PhCompareString` to perform the comparison.
 * If `String1` is null and `String2` is not null, the function returns 0 if `String2` is also null, or 1 or -1 based on the specified `Order` (AscendingSortOrder or DescendingSortOrder).
 * If `String1` is not null and `String2` is null, the function returns -1 or 1 based on the specified `Order`.
 *
 * @param String1 The first string to compare.
 * @param String2 The second string to compare.
 * @param Order The sort order to use for the comparison (AscendingSortOrder or DescendingSortOrder).
 * @param IgnoreCase Specifies whether the comparison should be case-insensitive (TRUE) or case-sensitive (FALSE).
 * @return Returns a negative value if `String1` is less than `String2`, a positive value if `String1` is greater than `String2`, or 0 if they are equal.
 */
FORCEINLINE
LONG
PhCompareStringWithNullSortOrder(
    _In_opt_ PPH_STRING String1,
    _In_opt_ PPH_STRING String2,
    _In_ PH_SORT_ORDER Order,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (String1 && String2)
    {
        return PhCompareString(String1, String2, IgnoreCase);
    }
    else if (!String1)
    {
        return !String2 ? 0 : (Order == AscendingSortOrder ? 1 : -1);
    }
    else
    {
        return (Order == AscendingSortOrder ? -1 : 1);
    }
}

/**
 * Compares two strings with support for null values and customizable sort order.
 *
 * This function compares two strings, taking into account the possibility of null values and allowing for customizable sort order.
 * If both `String1` and `String2` are not null, the function calls `PhCompareString` to perform the comparison.
 * If `String1` is null and `String2` is not null, the function returns 0 if `String2` is also null, or 1 if `String2` is not null, depending on the specified sort order.
 * If `String1` is not null and `String2` is null, the function returns -1 or 1, depending on the specified sort order.
 *
 * @param String1 The first string to compare. Can be null.
 * @param String2 The second string to compare. Can be null.
 * @param Order The sort order to use for the comparison. Must be either `AscendingSortOrder` or `DescendingSortOrder`.
 * @param IgnoreCase Specifies whether the comparison should be case-insensitive (`true`) or case-sensitive (`false`).
 * @return The result of the comparison. Returns a negative value if `String1` is less than `String2`, 0 if they are equal, or a positive value if `String1` is greater than `String2`.
 */
FORCEINLINE
LONG
PhCompareStringRefWithNullSortOrder(
    _In_opt_ PPH_STRINGREF String1,
    _In_opt_ PPH_STRINGREF String2,
    _In_ PH_SORT_ORDER Order,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (String1 && String2)
    {
        return PhCompareStringRef(String1, String2, IgnoreCase);
    }
    else if (!String1)
    {
        return !String2 ? 0 : (Order == AscendingSortOrder ? 1 : -1);
    }
    else
    {
        return (Order == AscendingSortOrder ? -1 : 1);
    }
}

/**
 * Determines whether two strings are equal.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
FORCEINLINE
BOOLEAN
PhEqualString(
    _In_ PPH_STRING String1,
    _In_ PPH_STRING String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    return PhEqualStringRef(&String1->sr, &String2->sr, IgnoreCase);
}

/**
 * Determines whether two strings are equal.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
FORCEINLINE
BOOLEAN
PhEqualString2(
    _In_ PPH_STRING String1,
    _In_ PCWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (IgnoreCase)
        return PhEqualStringRef2(&String1->sr, String2, IgnoreCase);
    else
        return wcscmp(String1->Buffer, String2) == 0;
}

/**
 * Determines whether a string starts with another.
 *
 * \param String The first string.
 * \param Prefix The second string.
 * \param IgnoreCase Whether to ignore character cases.
 *
 * \return TRUE if \a String starts with \a Prefix, otherwise FALSE.
 */
FORCEINLINE
BOOLEAN
PhStartsWithString(
    _In_ PPH_STRING String,
    _In_ PPH_STRING Prefix,
    _In_ BOOLEAN IgnoreCase
    )
{
    return PhStartsWithStringRef(&String->sr, &Prefix->sr, IgnoreCase);
}

/**
 * Determines whether a string starts with another.
 *
 * \param String The first string.
 * \param Prefix The second string.
 * \param IgnoreCase Whether to ignore character cases.
 *
 * \return TRUE if \a String starts with \a Prefix, otherwise FALSE.
 */
FORCEINLINE
BOOLEAN
PhStartsWithString2(
    _In_ PPH_STRING String,
    _In_ PCWSTR Prefix,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF prefix;

    PhInitializeStringRef(&prefix, Prefix);

    return PhStartsWithStringRef(&String->sr, &prefix, IgnoreCase);
}

/**
 * Determines whether a string ends with another.
 *
 * \param String The first string.
 * \param Suffix The second string.
 * \param IgnoreCase Whether to ignore character cases.
 *
 * \return TRUE if \a String ends with \a Suffix, otherwise FALSE.
 */
FORCEINLINE
BOOLEAN
PhEndsWithString(
    _In_ PPH_STRING String,
    _In_ PPH_STRING Suffix,
    _In_ BOOLEAN IgnoreCase
    )
{
    return PhEndsWithStringRef(&String->sr, &Suffix->sr, IgnoreCase);
}

/**
 * Determines whether a string ends with another.
 *
 * \param String The first string.
 * \param Suffix The second string.
 * \param IgnoreCase Whether to ignore character cases.
 *
 * \return TRUE if \a String ends with \a Suffix, otherwise FALSE.
 */
FORCEINLINE
BOOLEAN
PhEndsWithString2(
    _In_ PPH_STRING String,
    _In_ PCWSTR Suffix,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF suffix;

    PhInitializeStringRef(&suffix, Suffix);

    return PhEndsWithStringRef(&String->sr, &suffix, IgnoreCase);
}

/**
 * Locates a character in a string.
 *
 * \param String The string to search.
 * \param StartIndex The index, in characters, to start searching at.
 * \param Char The character to search for.
 *
 * \return The index, in characters, of the first occurrence of \a Char in \a String after
 * \a StartIndex. If \a Char was not found, -1 is returned.
 */
FORCEINLINE
ULONG_PTR
PhFindCharInString(
    _In_ PPH_STRING String,
    _In_ SIZE_T StartIndex,
    _In_ WCHAR Char
    )
{
    if (StartIndex != 0)
    {
        ULONG_PTR r;
        PH_STRINGREF sr;

        sr = String->sr;
        PhSkipStringRef(&sr, StartIndex * sizeof(WCHAR));
        r = PhFindCharInStringRef(&sr, Char, FALSE);

        if (r != SIZE_MAX)
            return r + StartIndex;
        else
            return SIZE_MAX;
    }
    else
    {
        return PhFindCharInStringRef(&String->sr, Char, FALSE);
    }
}

/**
 * Locates a character in a string, backwards.
 *
 * \param String The string to search.
 * \param StartIndex The index, in characters, to start searching at.
 * \param Char The character to search for.
 *
 * \return The index, in characters, of the last occurrence of \a Char in \a String after
 * \a StartIndex. If \a Char was not found, -1 is returned.
 */
FORCEINLINE
ULONG_PTR
PhFindLastCharInString(
    _In_ PPH_STRING String,
    _In_ SIZE_T StartIndex,
    _In_ WCHAR Char
    )
{
    if (StartIndex != 0)
    {
        ULONG_PTR r;
        PH_STRINGREF sr;

        sr = String->sr;
        PhSkipStringRef(&sr, StartIndex * sizeof(WCHAR));
        r = PhFindLastCharInStringRef(&sr, Char, FALSE);

        if (r != SIZE_MAX)
            return r + StartIndex;
        else
            return SIZE_MAX;
    }
    else
    {
        return PhFindLastCharInStringRef(&String->sr, Char, FALSE);
    }
}

/**
 * Locates a string in a string.
 *
 * \param String The string to search.
 * \param StartIndex The index, in characters, to start searching at.
 * \param SubString The string to search for.
 *
 * \return The index, in characters, of the first occurrence of \a SubString in \a String after
 * \a StartIndex. If \a SubString was not found, -1 is returned.
 */
FORCEINLINE
ULONG_PTR
PhFindStringInString(
    _In_ PPH_STRING String,
    _In_ SIZE_T StartIndex,
    _In_ PCWSTR SubString
    )
{
    PH_STRINGREF sr2;

    PhInitializeStringRef(&sr2, SubString);

    if (StartIndex != 0)
    {
        ULONG_PTR r;
        PH_STRINGREF sr1;

        sr1 = String->sr;
        PhSkipStringRef(&sr1, StartIndex * sizeof(WCHAR));
        r = PhFindStringInStringRef(&sr1, &sr2, FALSE);

        if (r != SIZE_MAX)
            return r + StartIndex;
        else
            return SIZE_MAX;
    }
    else
    {
        return PhFindStringInStringRef(&String->sr, &sr2, FALSE);
    }
}

/**
 * Creates a substring of a string.
 *
 * \param String The original string.
 * \param StartIndex The start index, in characters.
 * \param Count The number of characters to use.
 */
FORCEINLINE
PPH_STRING
PhSubstring(
    _In_ PPH_STRING String,
    _In_ SIZE_T StartIndex,
    _In_ SIZE_T Count
    )
{
    return PhCreateStringEx(&String->Buffer[StartIndex], Count * sizeof(WCHAR));
}

/**
 * Updates a string object's length with its true length as determined by an embedded null
 * terminator.
 *
 * \param String The string to modify.
 *
 * \remarks Use this function after modifying a string object's buffer manually.
 */
FORCEINLINE
VOID
PhTrimToNullTerminatorString(
    _Inout_ PPH_STRING String
    )
{
    String->Length = PhCountStringZ(String->Buffer) * sizeof(WCHAR);
}

// Byte string

extern PPH_OBJECT_TYPE PhBytesType;

/**
 * An 8-bit string object, which supports ASCII, UTF-8 and Windows multi-byte encodings, as well as
 * binary data.
 */
typedef struct _PH_BYTES
{
    // Header
    union
    {
        PH_BYTESREF br;
        struct
        {
            /** The length, in bytes, of the string. */
            SIZE_T Length;
            /** The buffer containing the contents of the string. */
            PCH Buffer;
        };
    };

    // Data
    union
    {
        CHAR Data[1];
        struct
        {
            /** Reserved. */
            ULONG AllocationFlags;
            /** Reserved. */
            PVOID Allocation;
        };
    };
} PH_BYTES, *PPH_BYTES;

PHLIBAPI
PPH_BYTES
NTAPI
PhCreateBytes(
    _In_ PCSTR Buffer
    );

PHLIBAPI
PPH_BYTES
NTAPI
PhCreateBytesEx(
    _In_opt_ PCCH Buffer,
    _In_ SIZE_T Length
    );

FORCEINLINE
PPH_BYTES
PhCreateBytes2(
    _In_ PPH_BYTESREF Bytes
    )
{
    return PhCreateBytesEx(Bytes->Buffer, Bytes->Length);
}

PPH_BYTES PhFormatBytes_V(
    _In_ _Printf_format_string_ PCSTR Format,
    _In_ va_list ArgPtr
    );

PPH_BYTES PhFormatBytes(
    _In_ _Printf_format_string_ PCSTR Format,
    ...
    );

// Unicode

#define PH_UNICODE_BYTE_ORDER_MARK 0xfeff
#define PH_UNICODE_MAX_CODE_POINT 0x10ffff

#define PH_UNICODE_UTF16_TO_HIGH_SURROGATE(CodePoint) ((USHORT)((CodePoint) >> 10) + 0xd7c0)
#define PH_UNICODE_UTF16_TO_LOW_SURROGATE(CodePoint) ((USHORT)((CodePoint) & 0x3ff) + 0xdc00)
#define PH_UNICODE_UTF16_IS_HIGH_SURROGATE(CodeUnit) ((CodeUnit) >= 0xd800 && (CodeUnit) <= 0xdbff)
#define PH_UNICODE_UTF16_IS_LOW_SURROGATE(CodeUnit) ((CodeUnit) >= 0xdc00 && (CodeUnit) <= 0xdfff)
#define PH_UNICODE_UTF16_TO_CODE_POINT(HighSurrogate, LowSurrogate) (((ULONG)(HighSurrogate) << 10) + (ULONG)(LowSurrogate) - 0x35fdc00)

#define PH_UNICODE_UTF8 0
#define PH_UNICODE_UTF16 1
#define PH_UNICODE_UTF32 2

typedef struct _PH_UNICODE_DECODER
{
    UCHAR Encoding; // PH_UNICODE_*
    UCHAR State;
    UCHAR InputCount;
    UCHAR Reserved;
    union
    {
        UCHAR Utf8[4];
        USHORT Utf16[2];
        ULONG Utf32;
    } Input;
    union
    {
        struct
        {
            UCHAR Input[4];
            UCHAR CodeUnit1;
            UCHAR CodeUnit2;
            UCHAR CodeUnit3;
            UCHAR CodeUnit4;
        } Utf8;
        struct
        {
            USHORT Input[2];
            USHORT CodeUnit;
        } Utf16;
        struct
        {
            ULONG Input;
        } Utf32;
    } u;
} PH_UNICODE_DECODER, *PPH_UNICODE_DECODER;

FORCEINLINE
VOID
PhInitializeUnicodeDecoder(
    _Out_ PPH_UNICODE_DECODER Decoder,
    _In_ UCHAR Encoding
    )
{
    memset(Decoder, 0, sizeof(PH_UNICODE_DECODER));
    Decoder->Encoding = Encoding;
}

PHLIBAPI
BOOLEAN
NTAPI
PhWriteUnicodeDecoder(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _In_ ULONG CodeUnit
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhDecodeUnicodeDecoder(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _Out_ PULONG CodePoint
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhEncodeUnicode(
    _In_ UCHAR Encoding,
    _In_ ULONG CodePoint,
    _Out_opt_ PVOID CodeUnits,
    _Out_ PULONG NumberOfCodeUnits
    );

// 8-bit to UTF-16

PHLIBAPI
VOID
NTAPI
PhZeroExtendToUtf16Buffer(
    _In_reads_bytes_(InputLength) PCCH Input,
    _In_ SIZE_T InputLength,
    _Out_writes_bytes_(InputLength * sizeof(WCHAR)) PWCH Output
    );

PHLIBAPI
PPH_STRING
NTAPI
PhZeroExtendToUtf16Ex(
    _In_reads_bytes_(InputLength) PCCH Input,
    _In_ SIZE_T InputLength
    );

FORCEINLINE
PPH_STRING
PhZeroExtendToUtf16(
    _In_ PCSTR Input
    )
{
    return PhZeroExtendToUtf16Ex(Input, strlen(Input));
}

// UTF-16 to ASCII

PHLIBAPI
PPH_BYTES
NTAPI
PhConvertUtf16ToAsciiEx(
    _In_ PCWCH Buffer,
    _In_ SIZE_T Length,
    _In_opt_ CHAR Replacement
    );

FORCEINLINE
PPH_BYTES
PhConvertUtf16ToAscii(
    _In_ PCWSTR Buffer,
    _In_opt_ CHAR Replacement
    )
{
    return PhConvertUtf16ToAsciiEx(Buffer, PhCountStringZ(Buffer) * sizeof(WCHAR), Replacement);
}

// Multi-byte to UTF-16
// In-place: RtlMultiByteToUnicodeN, RtlMultiByteToUnicodeSize

PHLIBAPI
PPH_STRING
NTAPI
PhConvertMultiByteToUtf16(
    _In_ PCSTR Buffer
    );

PHLIBAPI
PPH_STRING
NTAPI
PhConvertMultiByteToUtf16Ex(
    _In_ PCSTR Buffer,
    _In_ SIZE_T Length
    );

// UTF-16 to multi-byte
// In-place: RtlUnicodeToMultiByteN, RtlUnicodeToMultiByteSize

PHLIBAPI
PPH_BYTES
NTAPI
PhConvertUtf16ToMultiByte(
    _In_ PCWSTR Buffer
    );

PHLIBAPI
PPH_BYTES
NTAPI
PhConvertUtf16ToMultiByteEx(
    _In_ PCWCH Buffer,
    _In_ SIZE_T Length
    );

// UTF-8 to UTF-16
// In-place: RtlUTF8ToUnicodeN

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhConvertUtf8ToUtf16Size(
    _Out_ PSIZE_T BytesInUtf16String,
    _In_reads_bytes_(BytesInUtf8String) PCCH Utf8String,
    _In_ SIZE_T BytesInUtf8String
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhConvertUtf8ToUtf16Buffer(
    _Out_writes_bytes_to_(MaxBytesInUtf16String, *BytesInUtf16String) PWCH Utf16String,
    _In_ SIZE_T MaxBytesInUtf16String,
    _Out_opt_ PSIZE_T BytesInUtf16String,
    _In_reads_bytes_(BytesInUtf8String) PCCH Utf8String,
    _In_ SIZE_T BytesInUtf8String
    );

PHLIBAPI
PPH_STRING
NTAPI
PhConvertUtf8ToUtf16(
    _In_ PCSTR Buffer
    );

PHLIBAPI
PPH_STRING
NTAPI
PhConvertUtf8ToUtf16Ex(
    _In_ PCCH Buffer,
    _In_ SIZE_T Length
    );

// UTF-16 to UTF-8
// In-place: RtlUnicodeToUTF8N

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhConvertUtf16ToUtf8Size(
    _Out_ PSIZE_T BytesInUtf8String,
    _In_reads_bytes_(BytesInUtf16String) PCWCH Utf16String,
    _In_ SIZE_T BytesInUtf16String
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhConvertUtf16ToUtf8Buffer(
    _Out_writes_bytes_to_(MaxBytesInUtf8String, *BytesInUtf8String) PCH Utf8String,
    _In_ SIZE_T MaxBytesInUtf8String,
    _Out_opt_ PSIZE_T BytesInUtf8String,
    _In_reads_bytes_(BytesInUtf16String) PCWCH Utf16String,
    _In_ SIZE_T BytesInUtf16String
    );

PHLIBAPI
PPH_BYTES
NTAPI
PhConvertUtf16ToUtf8(
    _In_ PCWSTR Buffer
    );

PHLIBAPI
PPH_BYTES
NTAPI
PhConvertUtf16ToUtf8Ex(
    _In_ PCWCH Buffer,
    _In_ SIZE_T Length
    );

FORCEINLINE
PPH_BYTES
NTAPI
PhConvertStringToUtf8(
    _In_ PPH_STRING String
    )
{
    return PhConvertUtf16ToUtf8Ex(String->Buffer, String->Length);
}

FORCEINLINE
PPH_BYTES
NTAPI
PhConvertStringRefToUtf8(
    _In_ PPH_STRINGREF String
    )
{
    return PhConvertUtf16ToUtf8Ex(String->Buffer, String->Length);
}

FORCEINLINE
PPH_STRING
NTAPI
PhConvertBytesToUtf16(
    _In_ PPH_BYTES String
    )
{
    return PhConvertUtf8ToUtf16Ex(String->Buffer, String->Length);
}

// String builder

/**
 * A string builder structure.
 * The string builder object allows you to easily construct complex strings without allocating
 * a great number of strings in the process.
 */
typedef struct _PH_STRING_BUILDER
{
    /** Allocated length of the string, not including the null terminator. */
    SIZE_T AllocatedLength;
    /**
     * The constructed string.
     * \a String will be allocated for \a AllocatedLength, we will modify the \a Length field to be
     * the correct length.
     */
    PPH_STRING String;
} PH_STRING_BUILDER, *PPH_STRING_BUILDER;

PHLIBAPI
VOID
NTAPI
PhInitializeStringBuilder(
    _Out_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T InitialCapacity
    );

PHLIBAPI
VOID
NTAPI
PhDeleteStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFinalStringBuilderString(
    _Inout_ PPH_STRING_BUILDER StringBuilder
    );

PHLIBAPI
VOID
NTAPI
PhAppendStringBuilderEx(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_opt_ PWCHAR String,
    _In_ SIZE_T Length
    );

/**
 * Appends a string to the end of a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param String The string to append.
 */
FORCEINLINE
VOID
NTAPI
PhAppendStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ PPH_STRINGREF String
    )
{
    PhAppendStringBuilderEx(
        StringBuilder,
        String->Buffer,
        String->Length
        );
}

/**
 * Appends a string to the end of a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param String The string to append.
 */
FORCEINLINE
VOID
NTAPI
PhAppendStringBuilder2(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ PCWSTR String
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, String);
    PhAppendStringBuilder(StringBuilder, &string);
}

PHLIBAPI
VOID
NTAPI
PhAppendCharStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ WCHAR Character
    );

PHLIBAPI
VOID
NTAPI
PhAppendCharStringBuilder2(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ WCHAR Character,
    _In_ SIZE_T Count
    );

PHLIBAPI
VOID
NTAPI
PhAppendFormatStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ _Printf_format_string_ PCWSTR Format,
    ...
    );

VOID
NTAPI
PhAppendFormatStringBuilder_V(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ _Printf_format_string_ PCWSTR Format,
    _In_ va_list ArgPtr
    );

PHLIBAPI
VOID
NTAPI
PhInsertStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
VOID
NTAPI
PhInsertStringBuilder2(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_ PCWSTR String
    );

PHLIBAPI
VOID
NTAPI
PhInsertStringBuilderEx(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_opt_ PCWCHAR String,
    _In_ SIZE_T Length
    );

PHLIBAPI
VOID
NTAPI
PhRemoveStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T StartIndex,
    _In_ SIZE_T Count
    );

FORCEINLINE
VOID
PhRemoveEndStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Count
    )
{
    PhRemoveStringBuilder(
        StringBuilder,
        StringBuilder->String->Length / sizeof(WCHAR) - Count,
        Count
        );
}

// Byte string builder

/**
 * A byte string builder structure.
 * This is similar to string builder, but is based on PH_BYTES and is suitable for general binary
 * data.
 */
typedef struct _PH_BYTES_BUILDER
{
    /** Allocated length of the byte string, not including the null terminator. */
    SIZE_T AllocatedLength;
    /**
     * The constructed byte string.
     * \a Bytes will be allocated for \a AllocatedLength, we will modify the \a Length field to be
     * the correct length.
     */
    PPH_BYTES Bytes;
} PH_BYTES_BUILDER, *PPH_BYTES_BUILDER;

PHLIBAPI
VOID
NTAPI
PhInitializeBytesBuilder(
    _Out_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ SIZE_T InitialCapacity
    );

PHLIBAPI
VOID
NTAPI
PhDeleteBytesBuilder(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder
    );

PHLIBAPI
PPH_BYTES
NTAPI
PhFinalBytesBuilderBytes(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder
    );

FORCEINLINE
PVOID
PhOffsetBytesBuilder(
    _In_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ SIZE_T Offset
    )
{
    return BytesBuilder->Bytes->Buffer + Offset;
}

PHLIBAPI
VOID
NTAPI
PhAppendBytesBuilder(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ PPH_BYTESREF Bytes
    );

PHLIBAPI
VOID
NTAPI
PhAppendBytesBuilder2(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ PCHAR Bytes
    );

PHLIBAPI
PVOID
NTAPI
PhAppendBytesBuilderEx(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_opt_ PVOID Buffer,
    _In_ SIZE_T Length,
    _In_opt_ SIZE_T Alignment,
    _Out_opt_ PSIZE_T Offset
    );

PHLIBAPI
VOID
NTAPI
PhAppendFormatBytesBuilder_V(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ _Printf_format_string_ PCSTR Format,
    _In_ va_list ArgPtr
    );

PHLIBAPI
VOID
NTAPI
PhAppendFormatBytesBuilder(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ _Printf_format_string_ PCSTR Format,
    ...
    );

// Array

/** An array structure. Storage is automatically allocated for new elements. */
typedef struct _PH_ARRAY
{
    /** The number of items in the list. */
    SIZE_T Count;
    /** The number of items for which storage is allocated. */
    SIZE_T AllocatedCount;
    /** The size of each item, in bytes. */
    SIZE_T ItemSize;
    /** The base address of the array. */
    PVOID Items;
} PH_ARRAY, *PPH_ARRAY;

PHLIBAPI
VOID
NTAPI
PhInitializeArray(
    _Out_ PPH_ARRAY Array,
    _In_ SIZE_T ItemSize,
    _In_ SIZE_T InitialCapacity
    );

PHLIBAPI
VOID
NTAPI
PhDeleteArray(
    _Inout_ PPH_ARRAY Array
    );

FORCEINLINE
PVOID
PhItemArray(
    _In_ PPH_ARRAY Array,
    _In_ SIZE_T Index
    )
{
    return PTR_ADD_OFFSET(Array->Items, Index * Array->ItemSize);
}

FORCEINLINE
PVOID
PhFinalArrayItems(
    _Inout_ PPH_ARRAY Array
    )
{
    return Array->Items;
}

FORCEINLINE
SIZE_T
PhFinalArrayCount(
    _In_ PPH_ARRAY Array
    )
{
    return Array->Count;
}

PHLIBAPI
VOID
NTAPI
PhResizeArray(
    _Inout_ PPH_ARRAY Array,
    _In_ SIZE_T NewCapacity
    );

PHLIBAPI
VOID
NTAPI
PhAddItemArray(
    _Inout_ PPH_ARRAY Array,
    _In_ PVOID Item
    );

PHLIBAPI
VOID
NTAPI
PhAddItemsArray(
    _Inout_ PPH_ARRAY Array,
    _In_ PVOID Items,
    _In_ SIZE_T Count
    );

PHLIBAPI
VOID
NTAPI
PhClearArray(
    _Inout_ PPH_ARRAY Array
    );

PHLIBAPI
VOID
NTAPI
PhRemoveItemArray(
    _Inout_ PPH_ARRAY Array,
    _In_ SIZE_T Index
    );

PHLIBAPI
VOID
NTAPI
PhRemoveItemsArray(
    _Inout_ PPH_ARRAY Array,
    _In_ SIZE_T StartIndex,
    _In_ SIZE_T Count
    );

// List

extern PPH_OBJECT_TYPE PhListType;

/** A list structure. Storage is automatically allocated for new elements. */
typedef struct _PH_LIST
{
    /** The number of items in the list. */
    ULONG Count;
    /** The number of items for which storage is allocated. */
    ULONG AllocatedCount;
    /** The array of list items. */
    PVOID *Items;
} PH_LIST, *PPH_LIST;

FORCEINLINE
PVOID
PhItemList(
    _In_ PPH_LIST List,
    _In_ ULONG Index
    )
{
    return List->Items[Index];
}

PHLIBAPI
PPH_LIST
NTAPI
PhCreateList(
    _In_ ULONG InitialCapacity
    );

PHLIBAPI
VOID
NTAPI
PhResizeList(
    _Inout_ PPH_LIST List,
    _In_ ULONG NewCapacity
    );

PHLIBAPI
VOID
NTAPI
PhAddItemList(
    _Inout_ PPH_LIST List,
    _In_ PVOID Item
    );

PHLIBAPI
VOID
NTAPI
PhAddItemsList(
    _Inout_ PPH_LIST List,
    _In_ PVOID *Items,
    _In_ ULONG Count
    );

PHLIBAPI
VOID
NTAPI
PhClearList(
    _Inout_ PPH_LIST List
    );

_Success_(return != -1)
PHLIBAPI
ULONG
NTAPI
PhFindItemList(
    _In_ PPH_LIST List,
    _In_ PVOID Item
    );

PHLIBAPI
VOID
NTAPI
PhInsertItemList(
    _Inout_ PPH_LIST List,
    _In_ ULONG Index,
    _In_ PVOID Item
    );

PHLIBAPI
VOID
NTAPI
PhInsertItemsList(
    _Inout_ PPH_LIST List,
    _In_ ULONG Index,
    _In_ PVOID *Items,
    _In_ ULONG Count
    );

PHLIBAPI
VOID
NTAPI
PhRemoveItemList(
    _Inout_ PPH_LIST List,
    _In_ ULONG Index
    );

PHLIBAPI
VOID
NTAPI
PhRemoveItemsList(
    _Inout_ PPH_LIST List,
    _In_ ULONG StartIndex,
    _In_ ULONG Count
    );

/**
 * A comparison function.
 *
 * \param Item1 The first item.
 * \param Item2 The second item.
 * \param Context A user-defined value.
 *
 * \return
 * \li A positive value if \a Item1 > \a Item2,
 * \li A negative value if \a Item1 < \a Item2, and
 * \li 0 if \a Item1 = \a Item2.
 */
typedef LONG (NTAPI *PPH_COMPARE_FUNCTION)(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    );

// Pointer list

extern PPH_OBJECT_TYPE PhPointerListType;

/**
 * A pointer list structure. The pointer list is similar to the normal list structure, but both
 * insertions and deletions occur in constant time. The list is not ordered.
 */
typedef struct _PH_POINTER_LIST
{
    /** The number of pointers in the list. */
    ULONG Count;
    /** The number of pointers for which storage is allocated. */
    ULONG AllocatedCount;
    /** Index into pointer array for free list. */
    ULONG FreeEntry;
    /** Index of next usable index into pointer array. */
    ULONG NextEntry;
    /** The array of pointers. */
    PVOID *Items;
} PH_POINTER_LIST, *PPH_POINTER_LIST;

#define PH_IS_LIST_POINTER_VALID(Pointer) (!((ULONG_PTR)(Pointer) & 0x1))

PHLIBAPI
PPH_POINTER_LIST
NTAPI
PhCreatePointerList(
    _In_ ULONG InitialCapacity
    );

PHLIBAPI
HANDLE
NTAPI
PhAddItemPointerList(
    _Inout_ PPH_POINTER_LIST PointerList,
    _In_ PVOID Pointer
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhEnumPointerListEx(
    _In_ PPH_POINTER_LIST PointerList,
    _Inout_ PULONG EnumerationKey,
    _Out_ PVOID *Pointer,
    _Out_ PHANDLE PointerHandle
    );

PHLIBAPI
HANDLE
NTAPI
PhFindItemPointerList(
    _In_ PPH_POINTER_LIST PointerList,
    _In_ PVOID Pointer
    );

PHLIBAPI
VOID
NTAPI
PhRemoveItemPointerList(
    _Inout_ PPH_POINTER_LIST PointerList,
    _In_ HANDLE PointerHandle
    );

_Success_(return != FALSE)
FORCEINLINE
BOOLEAN
PhEnumPointerList(
    _In_ PPH_POINTER_LIST PointerList,
    _Inout_ PULONG EnumerationKey,
    _Out_ PVOID *Pointer
    )
{
    while (*EnumerationKey < PointerList->NextEntry)
    {
        PVOID pointer = PointerList->Items[*EnumerationKey];

        (*EnumerationKey)++;

        if (PH_IS_LIST_POINTER_VALID(pointer))
        {
            *Pointer = pointer;
            return TRUE;
        }
    }

    return FALSE;
}

// Hash

typedef struct _PH_HASH_ENTRY
{
    struct _PH_HASH_ENTRY *Next;
    ULONG Hash;
} PH_HASH_ENTRY, *PPH_HASH_ENTRY;

#define PH_HASH_SET_INIT { 0 }
#define PH_HASH_SET_SIZE(Buckets) (sizeof(Buckets) / sizeof(PPH_HASH_ENTRY))

/**
 * Initializes a hash set.
 *
 * \param Buckets The bucket array.
 * \param NumberOfBuckets The number of buckets.
 */
FORCEINLINE
VOID
PhInitializeHashSet(
    _Out_ PPH_HASH_ENTRY *Buckets,
    _In_ ULONG NumberOfBuckets
    )
{
    memset(Buckets, 0, sizeof(PPH_HASH_ENTRY) * NumberOfBuckets);
}

/**
 * Allocates and initializes a hash set.
 *
 * \param NumberOfBuckets The number of buckets.
 *
 * \return The allocated hash set. You must free it with PhFree() when you no longer need it.
 */
FORCEINLINE
PPH_HASH_ENTRY *
PhCreateHashSet(
    _In_ ULONG NumberOfBuckets
    )
{
    PPH_HASH_ENTRY *buckets;

    buckets = (PPH_HASH_ENTRY *)PhAllocate(sizeof(PPH_HASH_ENTRY) * NumberOfBuckets);
    PhInitializeHashSet(buckets, NumberOfBuckets);

    return buckets;
}

/**
 * Determines the number of entries in a hash set.
 *
 * \param Buckets The bucket array.
 * \param NumberOfBuckets The number of buckets.
 *
 * \return The number of entries in the hash set.
 */
FORCEINLINE
ULONG
PhCountHashSet(
    _In_ PPH_HASH_ENTRY *Buckets,
    _In_ ULONG NumberOfBuckets
    )
{
    ULONG i;
    PPH_HASH_ENTRY entry;
    ULONG count;

    count = 0;

    for (i = 0; i < NumberOfBuckets; i++)
    {
        for (entry = Buckets[i]; entry; entry = entry->Next)
            count++;
    }

    return count;
}

/**
 * Moves entries from one hash set to another.
 *
 * \param NewBuckets The new bucket array.
 * \param NumberOfNewBuckets The number of buckets in \a NewBuckets.
 * \param OldBuckets The old bucket array.
 * \param NumberOfOldBuckets The number of buckets in \a OldBuckets.
 *
 * \remarks \a NewBuckets and \a OldBuckets must be different.
 */
FORCEINLINE
VOID
PhDistributeHashSet(
    _Inout_ PPH_HASH_ENTRY *NewBuckets,
    _In_ ULONG NumberOfNewBuckets,
    _In_ CONST PPH_HASH_ENTRY *OldBuckets,
    _In_ ULONG NumberOfOldBuckets
    )
{
    ULONG i;
    PPH_HASH_ENTRY entry;
    PPH_HASH_ENTRY nextEntry;
    ULONG index;

    for (i = 0; i < NumberOfOldBuckets; i++)
    {
        entry = OldBuckets[i];

        while (entry)
        {
            nextEntry = entry->Next;

            index = entry->Hash & (NumberOfNewBuckets - 1);
            entry->Next = NewBuckets[index];
            NewBuckets[index] = entry;

            entry = nextEntry;
        }
    }
}

/**
 * Adds an entry to a hash set.
 *
 * \param Buckets The bucket array.
 * \param NumberOfBuckets The number of buckets.
 * \param Entry The entry.
 * \param Hash The hash for the entry.
 *
 * \remarks This function does not check for duplicates.
 */
FORCEINLINE
VOID
PhAddEntryHashSet(
    _Inout_ PPH_HASH_ENTRY *Buckets,
    _In_ ULONG NumberOfBuckets,
    _Out_ PPH_HASH_ENTRY Entry,
    _In_ ULONG Hash
    )
{
    ULONG index;

    index = Hash & (NumberOfBuckets - 1);

    Entry->Hash = Hash;
    Entry->Next = Buckets[index];
    Buckets[index] = Entry;
}

/**
 * Begins the process of finding an entry in a hash set.
 *
 * \param Buckets The bucket array.
 * \param NumberOfBuckets The number of buckets.
 * \param Hash The hash for the entry.
 *
 * \return The first entry in the chain.
 *
 * \remarks If the function returns NULL, the entry does not exist in the hash set.
 */
FORCEINLINE
PPH_HASH_ENTRY
PhFindEntryHashSet(
    _In_ CONST PPH_HASH_ENTRY *Buckets,
    _In_ ULONG NumberOfBuckets,
    _In_ ULONG Hash
    )
{
    return Buckets[Hash & (NumberOfBuckets - 1)];
}

/**
 * Removes an entry from a hash set.
 *
 * \param Buckets The bucket array.
 * \param NumberOfBuckets The number of buckets.
 * \param Entry An entry present in the hash set.
 */
FORCEINLINE
VOID
PhRemoveEntryHashSet(
    _Inout_ PPH_HASH_ENTRY *Buckets,
    _In_ ULONG NumberOfBuckets,
    _Inout_ PPH_HASH_ENTRY Entry
    )
{
    ULONG index;
    PPH_HASH_ENTRY entry;
    PPH_HASH_ENTRY previousEntry;

    index = Entry->Hash & (NumberOfBuckets - 1);
    previousEntry = NULL;

    entry = Buckets[index];

    do
    {
        if (entry == Entry)
        {
            if (!previousEntry)
                Buckets[index] = entry->Next;
            else
                previousEntry->Next = entry->Next;

            return;
        }

        previousEntry = entry;
        entry = entry->Next;
    } while (entry);

    // Entry doesn't actually exist in the set. This is a fatal logic error.
    PhRaiseStatus(STATUS_INTERNAL_ERROR);
}

/**
 * Resizes a hash set.
 *
 * \param Buckets A pointer to the bucket array. On return the new bucket array is stored in this
 * variable.
 * \param NumberOfBuckets A pointer to the number of buckets. On return the new number of buckets is
 * stored in this variable.
 * \param NewNumberOfBuckets The new number of buckets.
 */
FORCEINLINE
VOID
PhResizeHashSet(
    _Inout_ PPH_HASH_ENTRY **Buckets,
    _Inout_ PULONG NumberOfBuckets,
    _In_ ULONG NewNumberOfBuckets
    )
{
    PPH_HASH_ENTRY *newBuckets;

    newBuckets = PhCreateHashSet(NewNumberOfBuckets);
    PhDistributeHashSet(newBuckets, NewNumberOfBuckets, *Buckets, *NumberOfBuckets);

    PhFree(*Buckets);
    *Buckets = newBuckets;
    *NumberOfBuckets = NewNumberOfBuckets;
}

// Hashtable

extern PPH_OBJECT_TYPE PhHashtableType;

typedef struct _PH_HASHTABLE_ENTRY
{
    /** Hash code of the entry. -1 if entry is unused. */
    ULONG HashCode;
    /**
     * Either the index of the next entry in the bucket, the index of the next free entry, or -1 for
     * invalid.
     */
    ULONG Next;
    /** The beginning of user data. */
    QUAD_PTR Body;
} PH_HASHTABLE_ENTRY, *PPH_HASHTABLE_ENTRY;

C_ASSERT((FIELD_OFFSET(PH_HASHTABLE_ENTRY, Body) % MEMORY_ALLOCATION_ALIGNMENT) == 0);

/**
 * A comparison function used by a hashtable.
 *
 * \param Entry1 The first entry.
 * \param Entry2 The second entry.
 *
 * \return TRUE if the entries are equal, otherwise FALSE.
 */
typedef BOOLEAN (NTAPI *PPH_HASHTABLE_EQUAL_FUNCTION)(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

/**
 * A hash function used by a hashtable.
 *
 * \param Entry The entry.
 *
 * \return A hash code for the entry.
 *
 * \remarks
 * \li Two entries which are considered to be equal by the comparison function must be given the
 * same hash code.
 * \li Two different entries do not have to be given different hash codes.
 */
typedef ULONG (NTAPI *PPH_HASHTABLE_HASH_FUNCTION)(
    _In_ PVOID Entry
    );

// Use power-of-two sizes instead of primes
#define PH_HASHTABLE_POWER_OF_TWO_SIZE

// Enables 2^32-1 possible hash codes instead of only 2^31
//#define PH_HASHTABLE_FULL_HASH

/**
 * A hashtable structure.
 */
typedef struct _PH_HASHTABLE
{
    /** Size of user data in each entry. */
    ULONG EntrySize;
    /** The comparison function. */
    PPH_HASHTABLE_EQUAL_FUNCTION EqualFunction;
    /** The hash function. */
    PPH_HASHTABLE_HASH_FUNCTION HashFunction;

    /** The number of allocated buckets. */
    ULONG AllocatedBuckets;
    /** The bucket array. */
    PULONG Buckets;
    /** The number of allocated entries. */
    ULONG AllocatedEntries;
    /** The entry array. */
    PVOID Entries;

    /** Number of entries in the hashtable. */
    ULONG Count;
    /** Index into entry array for free list. */
    ULONG FreeEntry;
    /**
     * Index of next usable index into entry array, a.k.a. the count of entries that were ever
     * allocated.
     */
    ULONG NextEntry;
} PH_HASHTABLE, *PPH_HASHTABLE;

#define PH_HASHTABLE_ENTRY_SIZE(InnerSize) (UInt32Add32To64(UFIELD_OFFSET(PH_HASHTABLE_ENTRY, Body), (InnerSize)))
#define PH_HASHTABLE_GET_ENTRY(Hashtable, Index) ((PPH_HASHTABLE_ENTRY)PTR_ADD_OFFSET((Hashtable)->Entries, UInt32x32To64(PH_HASHTABLE_ENTRY_SIZE((Hashtable)->EntrySize), (Index))))
#define PH_HASHTABLE_GET_ENTRY_INDEX(Hashtable, Entry) ((ULONG)(PTR_ADD_OFFSET(Entry, -(Hashtable)->Entries) / PH_HASHTABLE_ENTRY_SIZE((Hashtable)->EntrySize)))

PHLIBAPI
PPH_HASHTABLE
NTAPI
PhCreateHashtable(
    _In_ ULONG EntrySize,
    _In_ PPH_HASHTABLE_EQUAL_FUNCTION EqualFunction,
    _In_ PPH_HASHTABLE_HASH_FUNCTION HashFunction,
    _In_ ULONG InitialCapacity
    );

PHLIBAPI
PVOID
NTAPI
PhAddEntryHashtable(
    _Inout_ PPH_HASHTABLE Hashtable,
    _In_ PVOID Entry
    );

PHLIBAPI
PVOID
NTAPI
PhAddEntryHashtableEx(
    _Inout_ PPH_HASHTABLE Hashtable,
    _In_ PVOID Entry,
    _Out_opt_ PBOOLEAN Added
    );

PHLIBAPI
VOID
NTAPI
PhClearHashtable(
    _Inout_ PPH_HASHTABLE Hashtable
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhEnumHashtable(
    _In_ PPH_HASHTABLE Hashtable,
    _Out_ PVOID *Entry,
    _Inout_ PULONG EnumerationKey
    );

PHLIBAPI
PVOID
NTAPI
PhFindEntryHashtable(
    _In_ PPH_HASHTABLE Hashtable,
    _In_ PVOID Entry
    );

PHLIBAPI
BOOLEAN
NTAPI
PhRemoveEntryHashtable(
    _Inout_ PPH_HASHTABLE Hashtable,
    _In_ PVOID Entry
    );

// New faster enumeration method

typedef struct _PH_HASHTABLE_ENUM_CONTEXT
{
    ULONG_PTR Current;
    ULONG_PTR End;
    ULONG_PTR Step;
} PH_HASHTABLE_ENUM_CONTEXT, *PPH_HASHTABLE_ENUM_CONTEXT;

FORCEINLINE
VOID
PhBeginEnumHashtable(
    _In_ PPH_HASHTABLE Hashtable,
    _Out_ PPH_HASHTABLE_ENUM_CONTEXT Context
    )
{
    Context->Current = (ULONG_PTR)Hashtable->Entries;
    Context->Step = PH_HASHTABLE_ENTRY_SIZE((ULONG_PTR)Hashtable->EntrySize);
    Context->End = Context->Current + (ULONG_PTR)Hashtable->NextEntry * Context->Step;
}

FORCEINLINE
PVOID
PhNextEnumHashtable(
    _Inout_ PPH_HASHTABLE_ENUM_CONTEXT Context
    )
{
    PPH_HASHTABLE_ENTRY entry;

    while (Context->Current != Context->End)
    {
        entry = (PPH_HASHTABLE_ENTRY)Context->Current;
        Context->Current += Context->Step;

        if (entry->HashCode != ULONG_MAX)
            return &entry->Body;
    }

    return NULL;
}

PHLIBAPI
ULONG
NTAPI
PhHashBytes(
    _In_reads_(Length) PUCHAR Bytes,
    _In_ SIZE_T Length
    );

PHLIBAPI
ULONG
NTAPI
PhHashStringRef(
    _In_ PPH_STRINGREF String,
    _In_ BOOLEAN IgnoreCase
    );

typedef enum _PH_STRING_HASH
{
    PH_STRING_HASH_DEFAULT,
    PH_STRING_HASH_FNV1A,
    PH_STRING_HASH_X65599,
} PH_STRING_HASH;

PHLIBAPI
ULONG
NTAPI
PhHashStringRefEx(
    _In_ PPH_STRINGREF String,
    _In_ BOOLEAN IgnoreCase,
    _In_ PH_STRING_HASH HashAlgorithm
    );

FORCEINLINE
ULONG
PhHashInt32(
    _In_ ULONG Value
    )
{
    // Java style.
    Value ^= (Value >> 20) ^ (Value >> 12);
    return Value ^ (Value >> 7) ^ (Value >> 4);
}

FORCEINLINE
ULONG
PhHashInt64(
    _In_ ULONG64 Value
    )
{
    // http://www.concentric.net/~Ttwang/tech/inthash.htm

    Value = ~Value + (Value << 18);
    Value ^= Value >> 31;
    Value *= 21;
    Value ^= Value >> 11;
    Value += Value << 6;
    Value ^= Value >> 22;

    return (ULONG)Value;
}

FORCEINLINE
ULONG
PhHashIntPtr(
    _In_ ULONG_PTR Value
    )
{
#ifdef _WIN64
    return PhHashInt64(Value);
#else
    return PhHashInt32(Value);
#endif
}

// Simple hashtable

#define SIP(String, Integer) { (String), (PVOID)(Integer) }
#define SREF(String) ((PVOID)&(PH_STRINGREF)PH_STRINGREF_INIT((String)))

typedef struct _PH_KEY_VALUE_PAIR
{
    PVOID Key;
    PVOID Value;
} PH_KEY_VALUE_PAIR, *PPH_KEY_VALUE_PAIR;

typedef CONST PH_KEY_VALUE_PAIR *PPCH_KEY_VALUE_PAIR;

PHLIBAPI
PPH_HASHTABLE
NTAPI
PhCreateSimpleHashtable(
    _In_ ULONG InitialCapacity
    );

PHLIBAPI
PVOID
NTAPI
PhAddItemSimpleHashtable(
    _Inout_ PPH_HASHTABLE SimpleHashtable,
    _In_opt_ PVOID Key,
    _In_opt_ PVOID Value
    );

PHLIBAPI
PVOID *
NTAPI
PhFindItemSimpleHashtable(
    _In_ PPH_HASHTABLE SimpleHashtable,
    _In_opt_ PVOID Key
    );

FORCEINLINE
PVOID
NTAPI
PhFindItemSimpleHashtable2(
    _In_ PPH_HASHTABLE SimpleHashtable,
    _In_opt_ PVOID Key
    )
{
    PVOID *item;

    item = PhFindItemSimpleHashtable(SimpleHashtable, Key);

    if (item)
        return *item;
    else
        return NULL;
}

PHLIBAPI
BOOLEAN
NTAPI
PhRemoveItemSimpleHashtable(
    _Inout_ PPH_HASHTABLE SimpleHashtable,
    _In_opt_ PVOID Key
    );

// Free list

typedef struct _PH_FREE_LIST
{
    SLIST_HEADER ListHead;

    ULONG Count;
    ULONG MaximumCount;
    SIZE_T Size;
} PH_FREE_LIST, *PPH_FREE_LIST;

typedef struct _PH_FREE_LIST_ENTRY
{
    SLIST_ENTRY ListEntry;
    QUAD_PTR Body;
} PH_FREE_LIST_ENTRY, *PPH_FREE_LIST_ENTRY;

C_ASSERT((FIELD_OFFSET(PH_FREE_LIST_ENTRY, Body) % MEMORY_ALLOCATION_ALIGNMENT) == 0);

#ifdef _WIN64
C_ASSERT(FIELD_OFFSET(PH_FREE_LIST_ENTRY, ListEntry) == 0x0);
C_ASSERT(FIELD_OFFSET(PH_FREE_LIST_ENTRY, Body) == 0x10);
#else
C_ASSERT(FIELD_OFFSET(PH_FREE_LIST_ENTRY, ListEntry) == 0x0);
C_ASSERT(FIELD_OFFSET(PH_FREE_LIST_ENTRY, Body) == 0x8);
#endif

PHLIBAPI
VOID
NTAPI
PhInitializeFreeList(
    _Out_ PPH_FREE_LIST FreeList,
    _In_ SIZE_T Size,
    _In_ ULONG MaximumCount
    );

PHLIBAPI
VOID
NTAPI
PhDeleteFreeList(
    _Inout_ PPH_FREE_LIST FreeList
    );

PHLIBAPI
PVOID
NTAPI
PhAllocateFromFreeList(
    _Inout_ PPH_FREE_LIST FreeList
    );

PHLIBAPI
VOID
NTAPI
PhFreeToFreeList(
    _Inout_ PPH_FREE_LIST FreeList,
    _In_ PVOID Memory
    );

// Callback

/**
 * A callback function.
 *
 * \param Parameter A value given to all callback functions being notified.
 * \param Context A user-defined value passed to PhRegisterCallback().
 */
typedef VOID (NTAPI *PPH_CALLBACK_FUNCTION)(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

/** A callback registration structure. */
typedef struct _PH_CALLBACK_REGISTRATION
{
    /** The list entry in the callbacks list. */
    LIST_ENTRY ListEntry;
    /** The callback function. */
    PPH_CALLBACK_FUNCTION Function;
    /** A user-defined value to be passed to the callback function. */
    PVOID Context;
    /** A value indicating whether the registration structure is being used. */
    LONG Busy;
    /** Whether the registration structure is being removed. */
    BOOLEAN Unregistering;
    BOOLEAN Reserved;
    /** Flags controlling the callback. */
    USHORT Flags;
} PH_CALLBACK_REGISTRATION, *PPH_CALLBACK_REGISTRATION;

/**
 * A callback structure. The callback object allows multiple callback functions to be registered and
 * notified in a thread-safe way.
 */
typedef struct _PH_CALLBACK
{
    /** The list of registered callbacks. */
    LIST_ENTRY ListHead;
    /** A lock protecting the callbacks list. */
    PH_QUEUED_LOCK ListLock;
    /** A condition variable pulsed when the callback becomes free. */
    PH_CONDITION BusyCondition;
} PH_CALLBACK, *PPH_CALLBACK;

#define PH_CALLBACK_DECLARE(Name) PH_CALLBACK Name = { &(Name).ListHead, &(Name).ListHead, PH_QUEUED_LOCK_INIT, PH_CONDITION_INIT }

PHLIBAPI
VOID
NTAPI
PhInitializeCallback(
    _Out_ PPH_CALLBACK Callback
    );

PHLIBAPI
VOID
NTAPI
PhDeleteCallback(
    _Inout_ PPH_CALLBACK Callback
    );

PHLIBAPI
VOID
NTAPI
PhRegisterCallback(
    _Inout_ PPH_CALLBACK Callback,
    _In_ PPH_CALLBACK_FUNCTION Function,
    _In_opt_ PVOID Context,
    _Out_ PPH_CALLBACK_REGISTRATION Registration
    );

PHLIBAPI
VOID
NTAPI
PhRegisterCallbackEx(
    _Inout_ PPH_CALLBACK Callback,
    _In_ PPH_CALLBACK_FUNCTION Function,
    _In_opt_ PVOID Context,
    _In_ USHORT Flags,
    _Out_ PPH_CALLBACK_REGISTRATION Registration
    );

PHLIBAPI
VOID
NTAPI
PhUnregisterCallback(
    _Inout_ PPH_CALLBACK Callback,
    _Inout_ PPH_CALLBACK_REGISTRATION Registration
    );

PHLIBAPI
VOID
NTAPI
PhInvokeCallback(
    _In_ PPH_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    );

// General

PHLIBAPI
ULONG
NTAPI
PhGetPrimeNumber(
    _In_ ULONG Minimum
    );

PHLIBAPI
ULONG
NTAPI
PhRoundUpToPowerOfTwo(
    _In_ ULONG Number
    );

PHLIBAPI
ULONG
NTAPI
PhExponentiate(
    _In_ ULONG Base,
    _In_ ULONG Exponent
    );

PHLIBAPI
ULONG64
NTAPI
PhExponentiate64(
    _In_ ULONG64 Base,
    _In_ ULONG Exponent
    );

PHLIBAPI
BOOLEAN
NTAPI
PhHexStringToBuffer(
    _In_ PPH_STRINGREF String,
    _Out_writes_bytes_(String->Length / sizeof(WCHAR) / 2) PUCHAR Buffer
    );

PHLIBAPI
BOOLEAN
NTAPI
PhHexStringToBufferEx(
    _In_ PPH_STRINGREF String,
    _In_ SIZE_T BufferLength,
    _Out_writes_bytes_(BufferLength) PVOID Buffer
    );

PHLIBAPI
PPH_STRING
NTAPI
PhBufferToHexString(
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ SIZE_T Length
    );

PPH_STRING
NTAPI
PhBufferToHexStringEx(
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ SIZE_T Length,
    _In_ BOOLEAN UpperCase
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhBufferToHexStringBuffer(
    _In_reads_bytes_(InputLength) PUCHAR InputBuffer,
    _In_ SIZE_T InputLength,
    _In_ BOOLEAN UpperCase,
    _Out_writes_bytes_to_(OutputLength, *ReturnLength) PWSTR OutputBuffer,
    _In_ SIZE_T OutputLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhStringToInteger64(
    _In_ PPH_STRINGREF String,
    _In_opt_ ULONG Base,
    _Out_opt_ PLONG64 Integer
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhStringToUInt64(
    _In_ PPH_STRINGREF String,
    _In_opt_ ULONG Base,
    _Out_opt_ PULONG64 Integer
    );

PHLIBAPI
BOOLEAN
NTAPI
PhStringToDouble(
    _In_ PPH_STRINGREF String,
    _Reserved_ ULONG Base,
    _Out_opt_ DOUBLE *Double
    );

PHLIBAPI
PPH_STRING
NTAPI
PhIntegerToString64(
    _In_ LONG64 Integer,
    _In_opt_ ULONG Base,
    _In_ BOOLEAN Signed
    );

#define PH_TIMESPAN_STR_LEN 30
#define PH_TIMESPAN_STR_LEN_1 (PH_TIMESPAN_STR_LEN + 1)

#define PH_TIMESPAN_HMS 0
#define PH_TIMESPAN_HMSM 1
#define PH_TIMESPAN_DHMS 2
#define PH_TIMESPAN_DHMSM 3

PHLIBAPI
VOID
NTAPI
PhPrintTimeSpan(
    _Out_writes_(PH_TIMESPAN_STR_LEN_1) PWSTR Destination,
    _In_ ULONG64 Ticks,
    _In_opt_ ULONG Mode
    );

PHLIBAPI
BOOLEAN
NTAPI
PhPrintTimeSpanToBuffer(
    _In_ ULONG64 Ticks,
    _In_opt_ ULONG Mode,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

PHLIBAPI
BOOLEAN
NTAPI
PhCalculateEntropy(
    _In_ PBYTE Buffer,
    _In_ ULONG64 BufferLength,
    _Out_opt_ FLOAT *Entropy,
    _Out_opt_ FLOAT *Variance
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatEntropy(
    _In_ FLOAT Entropy,
    _In_ USHORT EntropyPrecision,
    _In_opt_ FLOAT Variance,
    _In_opt_ USHORT VariancePrecision
    );

DECLSPEC_NOALIAS
PHLIBAPI
VOID
NTAPI
PhFillMemoryUlong(
    _Inout_updates_(Count) _Needs_align_(4) PULONG Memory,
    _In_ ULONG Value,
    _In_ SIZE_T Count
    );

DECLSPEC_NOALIAS
PHLIBAPI
VOID
NTAPI
PhDivideSinglesBySingle(
    _Inout_updates_(Count) PFLOAT A,
    _In_ FLOAT B,
    _In_ SIZE_T Count
    );

DECLSPEC_NOALIAS
PHLIBAPI
FLOAT
NTAPI
PhMaxMemorySingles(
    _In_ PFLOAT A,
    _In_ SIZE_T Count
    );

DECLSPEC_NOALIAS
PHLIBAPI
FLOAT
NTAPI
PhAddPlusMaxMemorySingles(
    _Inout_updates_(Count) PFLOAT A,
    _Inout_updates_(Count) PFLOAT B,
    _In_ SIZE_T Count
    );

DECLSPEC_NOALIAS
PHLIBAPI
VOID
NTAPI
PhConvertCopyMemoryUlong(
    _Inout_updates_(Count) PULONG From,
    _Inout_updates_(Count) PFLOAT To,
    _In_ SIZE_T Count
    );

DECLSPEC_NOALIAS
PHLIBAPI
VOID
NTAPI
PhConvertCopyMemorySingles(
    _Inout_updates_(Count) PFLOAT From,
    _Inout_updates_(Count) PULONG To,
    _In_ SIZE_T Count
    );

typedef struct _PH_CIRCULAR_BUFFER_ULONG* PPH_CIRCULAR_BUFFER_ULONG;

DECLSPEC_NOALIAS
PHLIBAPI
VOID
NTAPI
PhCopyConvertCircularBufferULONG(
    _Inout_ PPH_CIRCULAR_BUFFER_ULONG Buffer,
    _Out_writes_(Count) FLOAT* Destination,
    _In_ ULONG Count
    );

PHLIBAPI
ULONG
NTAPI
PhCountBits(
    _In_ ULONG Value
    );

PHLIBAPI
ULONG
NTAPI
PhCountBitsUlongPtr(
    _In_ ULONG_PTR Value
    );

// Thread Local Storage (TLS)

PHLIBAPI
ULONG
NTAPI
PhTlsAlloc(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
PhTlsFree(
    _In_ ULONG Index
    );

PHLIBAPI
PVOID
NTAPI
PhTlsGetValue(
    _In_ ULONG Index
    );

PHLIBAPI
NTSTATUS
NTAPI
PhTlsGetValueEx(
    _In_ ULONG Index,
    _Out_ PVOID* Value
    );

PHLIBAPI
NTSTATUS
NTAPI
PhTlsSetValue(
    _In_ ULONG Index,
    _In_opt_ PVOID Value
    );

PHLIBAPI
ULONG
NTAPI
PhGetLastError(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhSetLastError(
    _In_ ULONG ErrorValue
    );

// Auto-dereference convenience functions

FORCEINLINE
PPH_STRING
PhaCreateString(
    _In_ PCWSTR Buffer
    )
{
    return PH_AUTO_T(PH_STRING, PhCreateString(Buffer));
}

FORCEINLINE
PPH_STRING
PhaCreateStringEx(
    _In_opt_ PCWSTR Buffer,
    _In_ SIZE_T Length
    )
{
    return PH_AUTO_T(PH_STRING, PhCreateStringEx(Buffer, Length));
}

FORCEINLINE
PPH_STRING
PhaDuplicateString(
    _In_ PPH_STRING String
    )
{
    return PH_AUTO_T(PH_STRING, PhDuplicateString(String));
}

FORCEINLINE
PPH_STRING
PhaConcatStrings(
    _In_ ULONG Count,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Count);

    return PH_AUTO_T(PH_STRING, PhConcatStrings_V(Count, argptr));
}

FORCEINLINE
PPH_STRING
PhaConcatStrings2(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2
    )
{
    return PH_AUTO_T(PH_STRING, PhConcatStrings2(String1, String2));
}

FORCEINLINE
PPH_STRING
PhaFormatString(
    _In_ _Printf_format_string_ PCWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);

    return PH_AUTO_T(PH_STRING, PhFormatString_V(Format, argptr));
}

FORCEINLINE
PPH_STRING
PhLowerString(
    _In_ PPH_STRING String
    )
{
    return PhCreateString3(&String->sr, PH_STRING_LOWER_CASE, NULL);
}

FORCEINLINE
PPH_STRING
PhaLowerString(
    _In_ PPH_STRING String
    )
{
    return PH_AUTO_T(PH_STRING, PhCreateString3(&String->sr, PH_STRING_LOWER_CASE, NULL));
}

FORCEINLINE
PPH_STRING
NTAPI
PhUpperString(
    _In_ PPH_STRING String
    )
{
    return PhCreateString3(&String->sr, PH_STRING_UPPER_CASE, NULL);
}

FORCEINLINE
PPH_STRING
PhaUpperString(
    _In_ PPH_STRING String
    )
{
    return PH_AUTO_T(PH_STRING, PhCreateString3(&String->sr, PH_STRING_UPPER_CASE, NULL));
}

FORCEINLINE
PPH_STRING
PhaSubstring(
    _In_ PPH_STRING String,
    _In_ SIZE_T StartIndex,
    _In_ SIZE_T Count
    )
{
    return PH_AUTO_T(PH_STRING, PhSubstring(String, StartIndex, Count));
}

// Format

typedef enum _PH_FORMAT_TYPE
{
    CharFormatType,
    StringFormatType,
    StringZFormatType,
    MultiByteStringFormatType,
    MultiByteStringZFormatType,
    Int32FormatType,
    Int64FormatType,
    IntPtrFormatType,
    UInt32FormatType,
    UInt64FormatType,
    UIntPtrFormatType,
    SingleFormatType,
    DoubleFormatType,
    SizeFormatType,
    FormatTypeMask = 0x3f,

    /** If not specified, for floating-point 6 is assumed **/
    FormatUsePrecision = 0x40,
    /** If not specified, ' ' is assumed */
    FormatUsePad = 0x80,
    /** If not specified, 10 is assumed */
    FormatUseRadix = 0x100,
    /** If not specified, the default value is assumed */
    FormatUseParameter = 0x200,

    //
    // Floating-point flags
    //

    /** Use standard form instead of normal form */
    FormatStandardForm = 0x1000,
    /** Use hexadecimal form instead of normal form */
    FormatHexadecimalForm = 0x2000,
    /** Reserved */
    FormatForceDecimalPoint = 0x4000,
    /** Trailing zeros and possibly the decimal point are trimmed */
    FormatCropZeros = 0x8000,

    //
    // Floating-point and integer flags
    //

    /** Group digits (with floating-point, only works when in normal form) */
    FormatGroupDigits = 0x10000,
    /** Always insert a prefix, '+' for positive and '-' for negative */
    FormatPrefixSign = 0x20000,
    /**
     * Pad left with zeros, taking into consideration the sign. Width must be specified.
     * Format*Align cannot be used in conjunction with this flag. If FormatGroupDigits is specified,
     * this flag is ignored.
     */
    FormatPadZeros = 0x40000,

    //
    // General flags
    //

    /** Make characters uppercase (only available for some types) */
    FormatUpperCase = 0x20000000,
    /** Applies right alignment. Width must be specified. */
    FormatRightAlign = 0x40000000,
    /** Applies left alignment. Width must be specified. */
    FormatLeftAlign = 0x80000000,
} PH_FORMAT_TYPE;

/** Describes an element to be formatted to a string. */
typedef struct _PH_FORMAT
{
    /** Specifies the type of the element and optional flags. */
    ULONG Type;
    /**
     * The precision of the element. The meaning of this field depends on the element type. For
     * \a Double and \a Size, this field specifies the number of decimal points to include.
     */
    USHORT Precision;
    /**
     * The width of the element. This field specifies the minimum number of characters to output.
     * The remaining space is padded with either spaces, zeros, or a custom character.
     */
    USHORT Width;
    /** The pad character. */
    WCHAR Pad;
    /**
     * The meaning of this field depends on the element type. For integer types, this field
     * specifies the base to convert the number into. For \a Size, this field specifies the maximum
     * size unit.
     */
    UCHAR Radix;
    /**
     * The meaning of this field depends on the element type. For \a Size, this field specifies the
     * minimum size unit.
     */
    UCHAR Parameter;
    union
    {
        WCHAR Char;
        PH_STRINGREF String;
        PWSTR StringZ;
        PH_BYTESREF MultiByteString;
        PSTR MultiByteStringZ;
        LONG Int32;
        LONG64 Int64;
        LONG_PTR IntPtr;
        ULONG UInt32;
        ULONG64 UInt64;
        ULONG_PTR UIntPtr;
        FLOAT Single;
        DOUBLE Double;
        ULONG64 Size;
    } u;
} PH_FORMAT, *PPH_FORMAT;

// Convenience functions

FORCEINLINE
VOID
PhInitFormatC(
    _Out_ PPH_FORMAT Format,
    _In_ WCHAR Char
    )
{
    Format->Type = CharFormatType;
    Format->u.Char = Char;
}

FORCEINLINE
VOID
PhInitFormatS(
    _Out_ PPH_FORMAT Format,
    _In_ PCWSTR String
    )
{
    Format->Type = StringFormatType;
    PhInitializeStringRef(&Format->u.String, String);
}

FORCEINLINE
VOID
PhInitFormatSR(
    _Out_ PPH_FORMAT Format,
    _In_ PH_STRINGREF String
    )
{
    Format->Type = StringFormatType;
    Format->u.String = String;
}

FORCEINLINE
VOID
PhInitFormatUCS(
    _Out_ PPH_FORMAT Format,
    _In_ PUNICODE_STRING String
    )
{
    Format->Type = StringFormatType;
    PhUnicodeStringToStringRef(String, &Format->u.String);
}

FORCEINLINE
VOID
PhInitFormatMultiByteS(
    _Out_ PPH_FORMAT Format,
    _In_ PCSTR String
    )
{
    Format->Type = MultiByteStringFormatType;
    PhInitializeBytesRef(&Format->u.MultiByteString, String);
}

FORCEINLINE
VOID
PhInitFormatD(
    _Out_ PPH_FORMAT Format,
    _In_ LONG Int32
    )
{
    Format->Type = Int32FormatType;
    Format->u.Int32 = Int32;
}

FORCEINLINE
VOID
PhInitFormatU(
    _Out_ PPH_FORMAT Format,
    _In_ ULONG UInt32
    )
{
    Format->Type = UInt32FormatType;
    Format->u.UInt32 = UInt32;
}

FORCEINLINE
VOID
PhInitFormatX(
    _Out_ PPH_FORMAT Format,
    _In_ ULONG UInt32
    )
{
    Format->Type = UInt32FormatType | FormatUseRadix;
    Format->u.UInt32 = UInt32;
    Format->Radix = 16;
}

FORCEINLINE
VOID
PhInitFormatI64D(
    _Out_ PPH_FORMAT Format,
    _In_ LONG64 Int64
    )
{
    Format->Type = Int64FormatType;
    Format->u.Int64 = Int64;
}

FORCEINLINE
VOID
PhInitFormatI64U(
    _Out_ PPH_FORMAT Format,
    _In_ ULONG64 UInt64
    )
{
    Format->Type = UInt64FormatType;
    Format->u.UInt64 = UInt64;
}

FORCEINLINE
VOID
PhInitFormatI64UGroupDigits(
    _Out_ PPH_FORMAT Format,
    _In_ ULONG64 UInt64
    )
{
    Format->Type = UInt64FormatType | FormatGroupDigits;
    Format->u.UInt64 = UInt64;
}

FORCEINLINE
VOID
PhInitFormatI64UWithWidth(
    _Out_ PPH_FORMAT Format,
    _In_ ULONG64 UInt64,
    _In_ USHORT Width
    )
{
    Format->Type = UInt64FormatType | FormatPadZeros;
    Format->u.UInt64 = UInt64;
    Format->Width = Width;
}

FORCEINLINE
VOID
PhInitFormatI64X(
    _Out_ PPH_FORMAT Format,
    _In_ ULONG64 UInt64
    )
{
    Format->Type = UInt64FormatType | FormatUseRadix;
    Format->u.UInt64 = UInt64;
    Format->Radix = 16;
}

FORCEINLINE
VOID
PhInitFormatIU(
    _Out_ PPH_FORMAT Format,
    _In_ ULONG_PTR UIntPtr
    )
{
    Format->Type = UIntPtrFormatType;
    Format->u.UIntPtr = UIntPtr;
}

FORCEINLINE
VOID
PhInitFormatIX(
    _Out_ PPH_FORMAT Format,
    _In_ ULONG_PTR UIntPtr
    )
{
    Format->Type = UIntPtrFormatType | FormatUseRadix;
    Format->u.UIntPtr = UIntPtr;
    Format->Radix = 16;
}

FORCEINLINE
VOID
PhInitFormatIXPadZeros(
    _Out_ PPH_FORMAT Format,
    _In_ ULONG_PTR UIntPtr
    )
{
    Format->Type = UIntPtrFormatType | FormatUseRadix | FormatPadZeros;
    Format->u.UIntPtr = UIntPtr;
    Format->Radix = 16;
    Format->Width = sizeof(ULONG_PTR) * 2;
}

FORCEINLINE
VOID
PhInitFormatF(
    _Out_ PPH_FORMAT Format,
    _In_ FLOAT Single,
    _In_ USHORT Precision
    )
{
    Format->Type = SingleFormatType | FormatUsePrecision;
    Format->u.Single = Single;
    Format->Precision = Precision;
}

FORCEINLINE
VOID
PhInitFormatFD(
    _Out_ PPH_FORMAT Format,
    _In_ DOUBLE Double,
    _In_ USHORT Precision
    )
{
    Format->Type = DoubleFormatType | FormatUsePrecision;
    Format->u.Double = Double;
    Format->Precision = Precision;
}

FORCEINLINE
VOID
PhInitFormatE(
    _Out_ PPH_FORMAT Format,
    _In_ DOUBLE Double,
    _In_ USHORT Precision
    )
{
    Format->Type = DoubleFormatType | FormatStandardForm | FormatUsePrecision;
    Format->u.Double = Double;
    Format->Precision = Precision;
}

FORCEINLINE
VOID
PhInitFormatA(
    _Out_ PPH_FORMAT Format,
    _In_ DOUBLE Double,
    _In_ USHORT Precision
    )
{
    Format->Type = DoubleFormatType | FormatHexadecimalForm | FormatUsePrecision;
    Format->u.Double = Double;
    Format->Precision = Precision;
}

FORCEINLINE
VOID
PhInitFormatSize(
    _Out_ PPH_FORMAT Format,
    _In_ ULONG64 Size
    )
{
    Format->Type = SizeFormatType;
    Format->u.Size = Size;
}

FORCEINLINE
VOID
PhInitFormatSizeWithPrecision(
    _Out_ PPH_FORMAT Format,
    _In_ ULONG64 Size,
    _In_ USHORT Precision
    )
{
    Format->Type = SizeFormatType | FormatUsePrecision;
    Format->u.Size = Size;
    Format->Precision = Precision;
}

PHLIBAPI
PPH_STRING
NTAPI
PhFormat(
    _In_reads_(Count) PPH_FORMAT Format,
    _In_ ULONG Count,
    _In_opt_ SIZE_T InitialCapacity
    );

PHLIBAPI
BOOLEAN
NTAPI
PhFormatToBuffer(
    _In_reads_(Count) PPH_FORMAT Format,
    _In_ ULONG Count,
    _Out_writes_bytes_opt_(BufferLength) PWSTR Buffer,
    _In_opt_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhFormatSingleToUtf8(
    _In_ FLOAT Value,
    _In_ ULONG Type,
    _In_ ULONG Precision,
    _Out_writes_bytes_(BufferLength) PSTR Buffer,
    _In_opt_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhFormatDoubleToUtf8(
    _In_ DOUBLE Value,
    _In_ ULONG Type,
    _In_ ULONG Precision,
    _Out_writes_bytes_(BufferLength) PSTR Buffer,
    _In_opt_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

// error

#define HRESULT_CUSTOMER(hr) (((ULONG)(hr) >> 29) & 0x1)
#define HRESULT_NTSTATUS(hr) (((ULONG)(hr) >> 28) & 0x1)

PHLIBAPI
ULONG
NTAPI
PhNtStatusToDosError(
    _In_ NTSTATUS Status
    );

PHLIBAPI
ULONG
NTAPI
PhNtStatusToServiceStatus(
    _In_ NTSTATUS Status
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDosErrorToNtStatus(
    _In_ ULONG DosError
    );

PHLIBAPI
BOOLEAN
NTAPI
PhNtStatusFileNotFound(
    _In_ NTSTATUS Status
    );

PHLIBAPI
NTSTATUS
NTAPI
PhNtStatusFromHResult(
    _In_ HRESULT Result
    );

FORCEINLINE
NTSTATUS
NTAPI
PhGetLastWin32ErrorAsNtStatus(
    VOID
    )
{
    return PhDosErrorToNtStatus(PhGetLastError());
}

// Generic tree definitions

typedef enum _PH_TREE_ENUMERATION_ORDER
{
    TreeEnumerateInOrder,
    TreeEnumerateInReverseOrder
} PH_TREE_ENUMERATION_ORDER;

#define PhIsLeftChildElement(Links) ((Links)->Parent->Left == (Links))
#define PhIsRightChildElement(Links) ((Links)->Parent->Right == (Links))

// avltree

typedef struct _PH_AVL_LINKS
{
    struct _PH_AVL_LINKS *Parent;
    struct _PH_AVL_LINKS *Left;
    struct _PH_AVL_LINKS *Right;
    LONG Balance;
} PH_AVL_LINKS, *PPH_AVL_LINKS;

struct _PH_AVL_TREE;

typedef LONG (NTAPI *PPH_AVL_TREE_COMPARE_FUNCTION)(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    );

typedef struct _PH_AVL_TREE
{
    PH_AVL_LINKS Root; // Right contains real root
    ULONG Count;

    PPH_AVL_TREE_COMPARE_FUNCTION CompareFunction;
} PH_AVL_TREE, *PPH_AVL_TREE;

#define PH_AVL_TREE_INIT(CompareFunction) { { NULL, NULL, NULL, 0 }, 0, CompareFunction }

#define PhRootElementAvlTree(Tree) ((Tree)->Root.Right)

PHLIBAPI
VOID
NTAPI
PhInitializeAvlTree(
    _Out_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_TREE_COMPARE_FUNCTION CompareFunction
    );

PHLIBAPI
PPH_AVL_LINKS
NTAPI
PhAddElementAvlTree(
    _Inout_ PPH_AVL_TREE Tree,
    _Out_ PPH_AVL_LINKS Element
    );

PHLIBAPI
VOID
NTAPI
PhRemoveElementAvlTree(
    _Inout_ PPH_AVL_TREE Tree,
    _Inout_ PPH_AVL_LINKS Element
    );

PHLIBAPI
PPH_AVL_LINKS
NTAPI
PhFindElementAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element
    );

PHLIBAPI
PPH_AVL_LINKS
NTAPI
PhLowerBoundElementAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element
    );

PHLIBAPI
PPH_AVL_LINKS
NTAPI
PhUpperBoundElementAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element
    );

PHLIBAPI
PPH_AVL_LINKS
NTAPI
PhLowerDualBoundElementAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element
    );

PHLIBAPI
PPH_AVL_LINKS
NTAPI
PhUpperDualBoundElementAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element
    );

PHLIBAPI
PPH_AVL_LINKS
NTAPI
PhMinimumElementAvlTree(
    _In_ PPH_AVL_TREE Tree
    );

PHLIBAPI
PPH_AVL_LINKS
NTAPI
PhMaximumElementAvlTree(
    _In_ PPH_AVL_TREE Tree
    );

PHLIBAPI
PPH_AVL_LINKS
NTAPI
PhSuccessorElementAvlTree(
    _In_ PPH_AVL_LINKS Element
    );

PHLIBAPI
PPH_AVL_LINKS
NTAPI
PhPredecessorElementAvlTree(
    _In_ PPH_AVL_LINKS Element
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_AVL_TREE_CALLBACK)(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element,
    _In_opt_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhEnumAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PH_TREE_ENUMERATION_ORDER Order,
    _In_ PPH_ENUM_AVL_TREE_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

EXTERN_C_END

#endif
