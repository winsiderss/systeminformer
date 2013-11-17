#ifndef _PH_PHBASE_H
#define _PH_PHBASE_H

#pragma once

#ifndef PHLIB_NO_DEFAULT_LIB
#pragma comment(lib, "ntdll.lib")

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "version.lib")
#endif

// nonstandard extension used : nameless struct/union
#pragma warning(disable: 4201)
// nonstandard extension used : bit field types other than int
#pragma warning(disable: 4214)
// 'function': attributes not present on previous declaration
#pragma warning(disable: 4985)
// 'function': was declared deprecated
#pragma warning(disable: 4996)

#ifndef UNICODE
#define UNICODE
#endif

#if defined(PHLIB_EXPORT)
#define PHLIBAPI __declspec(dllexport)
#elif defined(PHLIB_IMPORT)
#define PHLIBAPI __declspec(dllimport)
#else
#define PHLIBAPI
#endif

#include <ntwin.h>
#include <ntbasic.h>
#include <phnt.h>
#include <phsup.h>
#include <ntimport.h>
#include <ref.h>
#include <fastlock.h>
#include <queuedlock.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _PH_STRING;
typedef struct _PH_STRING *PPH_STRING;

struct _PH_PROVIDER_THREAD;
typedef struct _PH_PROVIDER_THREAD PH_PROVIDER_THREAD;

struct _PH_STARTUP_PARAMETERS;
typedef struct _PH_STARTUP_PARAMETERS PH_STARTUP_PARAMETERS;

#define _User_set_

PHLIBAPI extern _User_set_ PVOID PhLibImageBase;

PHLIBAPI extern _User_set_ PWSTR PhApplicationName;
PHLIBAPI extern ULONG PhCurrentSessionId;
PHLIBAPI extern HANDLE PhCurrentTokenQueryHandle;
PHLIBAPI extern BOOLEAN PhElevated;
PHLIBAPI extern TOKEN_ELEVATION_TYPE PhElevationType;
PHLIBAPI extern PVOID PhHeapHandle;
PHLIBAPI extern RTL_OSVERSIONINFOEXW PhOsVersion;
PHLIBAPI extern SYSTEM_BASIC_INFORMATION PhSystemBasicInformation;
PHLIBAPI extern ULONG WindowsVersion;

PHLIBAPI extern ACCESS_MASK ProcessQueryAccess;
PHLIBAPI extern ACCESS_MASK ProcessAllAccess;
PHLIBAPI extern ACCESS_MASK ThreadQueryAccess;
PHLIBAPI extern ACCESS_MASK ThreadSetAccess;
PHLIBAPI extern ACCESS_MASK ThreadAllAccess;

#define WINDOWS_ANCIENT 0
#define WINDOWS_XP 51
#define WINDOWS_SERVER_2003 52
#define WINDOWS_VISTA 60
#define WINDOWS_7 61
#define WINDOWS_8 62
#define WINDOWS_81 63
#define WINDOWS_NEW MAXLONG

#define WINDOWS_HAS_CONSOLE_HOST (WindowsVersion >= WINDOWS_7)
#define WINDOWS_HAS_CYCLE_TIME (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_IFILEDIALOG (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_IMAGE_FILE_NAME_BY_PROCESS_ID (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_IMMERSIVE (WindowsVersion >= WINDOWS_8)
#define WINDOWS_HAS_LIMITED_ACCESS (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_PSSUSPENDRESUMEPROCESS (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_SERVICE_TAGS (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_UAC (WindowsVersion >= WINDOWS_VISTA)

// data

// SIDs

extern SID PhSeNobodySid;

extern SID PhSeEveryoneSid;

extern SID PhSeLocalSid;

extern SID PhSeCreatorOwnerSid;
extern SID PhSeCreatorGroupSid;

extern SID PhSeDialupSid;
extern SID PhSeNetworkSid;
extern SID PhSeBatchSid;
extern SID PhSeInteractiveSid;
extern SID PhSeServiceSid;
extern SID PhSeAnonymousLogonSid;
extern SID PhSeProxySid;
extern SID PhSeAuthenticatedUserSid;
extern SID PhSeRestrictedCodeSid;
extern SID PhSeTerminalServerUserSid;
extern SID PhSeRemoteInteractiveLogonSid;
extern SID PhSeLocalSystemSid;
extern SID PhSeLocalServiceSid;
extern SID PhSeNetworkServiceSid;

// Characters

extern BOOLEAN PhCharIsPrintable[256];
extern ULONG PhCharToInteger[256];
extern CHAR PhIntegerToChar[69];
extern CHAR PhIntegerToCharUpper[69];

// CRC32

extern ULONG PhCrc32Table[256];

// Enums

extern WCHAR *PhIoPriorityHintNames[MaxIoPriorityTypes];
extern WCHAR *PhKThreadStateNames[MaximumThreadState];
extern WCHAR *PhKWaitReasonNames[MaximumWaitReason];

// debugging

#ifdef DEBUG
#define dprintf(format, ...) DbgPrint(format, __VA_ARGS__)
#else
#define dprintf(format, ...)
#endif

// global

// Initialization flags

// Features

// Imports

/** Needed to use supplied dynamically imported functions from ntdll.dll. */
#define PHLIB_INIT_MODULE_NTIMPORTS 0x1
/** Needed to use fast locks. */
#define PHLIB_INIT_MODULE_FAST_LOCK 0x2
/** Needed to use work queues. */
#define PHLIB_INIT_MODULE_WORK_QUEUE 0x4
/** Needed to use handle tables. */
#define PHLIB_INIT_MODULE_HANDLE_TABLE 0x8
/** Needed to use file streams. Basic I/O functions do not require this in order to work. */
#define PHLIB_INIT_MODULE_IO_SUPPORT 0x10
/** Needed to use symbol providers. */
#define PHLIB_INIT_MODULE_SYMBOL_PROVIDER 0x20
/** Needed to use the handle information module. */
#define PHLIB_INIT_MODULE_HANDLE_INFO 0x40

// Misc.
/** Retrieves token information (e.g. elevation status). */
#define PHLIB_INIT_TOKEN_INFO 0x100000

NTSTATUS PhInitializePhLib(
    VOID
    );

NTSTATUS PhInitializePhLibEx(
    _In_ ULONG Flags,
    _In_opt_ SIZE_T HeapReserveSize,
    _In_opt_ SIZE_T HeapCommitSize
    );

// basesup

struct _PH_OBJECT_TYPE;
typedef struct _PH_OBJECT_TYPE *PPH_OBJECT_TYPE;

BOOLEAN PhInitializeBase(
    _In_ ULONG Flags
    );

// threads

#ifdef DEBUG
struct _PH_AUTO_POOL;
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
HANDLE
NTAPI
PhCreateThread(
    _In_opt_ SIZE_T StackSize,
    _In_ PUSER_THREAD_START_ROUTINE StartAddress,
    _In_opt_ PVOID Parameter
    );

// DLLs

FORCEINLINE PVOID PhGetDllHandle(
    _In_ PWSTR DllName
    )
{
    UNICODE_STRING dllName;
    PVOID dllHandle;

    RtlInitUnicodeString(&dllName, DllName);

    if (NT_SUCCESS(LdrGetDllHandle(NULL, NULL, &dllName, &dllHandle)))
        return dllHandle;
    else
        return NULL;
}

FORCEINLINE PVOID PhGetProcedureAddress(
    _In_ PVOID DllHandle,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ ULONG ProcedureNumber
    )
{
    NTSTATUS status;
    ANSI_STRING procedureName;
    PVOID procedureAddress;

    if (ProcedureName)
    {
        RtlInitAnsiString(&procedureName, ProcedureName);
        status = LdrGetProcedureAddress(
            DllHandle,
            &procedureName,
            0,
            &procedureAddress
            );
    }
    else
    {
        status = LdrGetProcedureAddress(
            DllHandle,
            NULL,
            ProcedureNumber,
            &procedureAddress
            );
    }

    if (!NT_SUCCESS(status))
        return NULL;

    return procedureAddress;
}

// misc. system

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

// heap

_May_raise_
_Check_return_
_Ret_notnull_
_Post_writable_byte_size_(Size)
PHLIBAPI
PVOID
NTAPI
PhAllocate(
    _In_ SIZE_T Size
    );

PHLIBAPI
PVOID
NTAPI
PhAllocateSafe(
    _In_ SIZE_T Size
    );

PHLIBAPI
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

_May_raise_
_Post_writable_byte_size_(Size)
PHLIBAPI
PVOID
NTAPI
PhReAllocate(
    _Frees_ptr_opt_ PVOID Memory,
    _In_ SIZE_T Size
    );

PHLIBAPI
PVOID
NTAPI
PhReAllocateSafe(
    _In_ PVOID Memory,
    _In_ SIZE_T Size
    );

_Check_return_
_Ret_maybenull_
PHLIBAPI
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
    _Frees_ptr_opt_ PVOID Memory
    );

FORCEINLINE PVOID PhAllocateAligned(
    _In_ SIZE_T Size,
    _In_ ULONG Align,
    _In_ SIZE_T AllocationBaseOffset
    )
{
    PVOID memory;
    PVOID allocationBase;

    allocationBase = PhAllocate(Size + Align - 1);
    memory = PTR_ALIGN(allocationBase, Align);
    *(PVOID *)PTR_ADD_OFFSET(memory, AllocationBaseOffset) = allocationBase;

    return memory;
}

FORCEINLINE VOID PhFreeAligned(
    _In_ _Post_invalid_ PVOID Memory,
    _In_ SIZE_T AllocationBaseOffset
    )
{
    PhFree(*(PVOID *)PTR_ADD_OFFSET(Memory, AllocationBaseOffset));
}

FORCEINLINE PVOID PhAllocateCopy(
    _In_ PVOID Data,
    _In_ SIZE_T Size
    )
{
    PVOID copy;

    copy = PhAllocate(Size);
    memcpy(copy, Data, Size);

    return copy;
}

// basesupx (basic)

PHLIBAPI
int
__cdecl
ph_equal_string(wchar_t *s1, wchar_t *s2, size_t len);

PHLIBAPI
unsigned long
__cdecl
ph_crc32(unsigned long crc, char *buf, size_t len);

unsigned short
__cdecl
ph_chksum(unsigned long sum, unsigned short *buf, unsigned long count);

// event

#define PH_EVENT_SET 0x1
#define PH_EVENT_SET_SHIFT 0
#define PH_EVENT_REFCOUNT_SHIFT 1
#define PH_EVENT_REFCOUNT_INC 0x2

/**
 * A fast event object.
 *
 * \remarks
 * This event object does not use a kernel event object
 * until necessary, and frees the object automatically
 * when it is no longer needed.
 */
typedef struct _PH_EVENT
{
    ULONG_PTR Value;
    HANDLE EventHandle;
} PH_EVENT, *PPH_EVENT;

#define PH_EVENT_INIT { PH_EVENT_REFCOUNT_INC, NULL }

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

#define PhWaitForEvent PhfWaitForEvent
PHLIBAPI
BOOLEAN
FASTCALL
PhfWaitForEvent(
    _Inout_ PPH_EVENT Event,
    _In_opt_ PLARGE_INTEGER Timeout
    );

#define PhResetEvent PhfResetEvent
PHLIBAPI
VOID
FASTCALL
PhfResetEvent(
    _Inout_ PPH_EVENT Event
    );

FORCEINLINE VOID PhInitializeEvent(
    _Out_ PPH_EVENT Event
    )
{
    Event->Value = PH_EVENT_REFCOUNT_INC;
    Event->EventHandle = NULL;
}

/**
 * Determines whether an event object is set.
 *
 * \param Event A pointer to an event object.
 *
 * \return TRUE if the event object is set,
 * otherwise FALSE.
 */
FORCEINLINE BOOLEAN PhTestEvent(
    _In_ PPH_EVENT Event
    )
{
    return !!(Event->Value & PH_EVENT_SET);
}

// barrier

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
    PH_QUEUED_LOCK WakeEvent;
} PH_BARRIER, *PPH_BARRIER;

#define PH_BARRIER_INIT(Target) { (ULONG_PTR)(Target) << PH_BARRIER_TARGET_SHIFT, PH_QUEUED_LOCK_INIT }

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

FORCEINLINE VOID PhInitializeBarrier(
    _Out_ PPH_BARRIER Barrier,
    _In_ ULONG_PTR Target
    )
{
    Barrier->Value = Target << PH_BARRIER_TARGET_SHIFT;
    PhInitializeQueuedLock(&Barrier->WakeEvent);
}

// rundown protection

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

FORCEINLINE VOID PhInitializeRundownProtection(
    _Out_ PPH_RUNDOWN_PROTECT Protection
    )
{
    Protection->Value = 0;
}

FORCEINLINE BOOLEAN PhAcquireRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;

    value = Protection->Value & ~PH_RUNDOWN_ACTIVE; // fail fast path when rundown is active

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

FORCEINLINE VOID PhReleaseRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;

    value = Protection->Value & ~PH_RUNDOWN_ACTIVE; // fail fast path when rundown is active

    if ((ULONG_PTR)_InterlockedCompareExchangePointer(
        (PVOID *)&Protection->Value,
        (PVOID)(value - PH_RUNDOWN_REF_INC),
        (PVOID)value
        ) != value)
    {
        PhfReleaseRundownProtection(Protection);
    }
}

FORCEINLINE VOID PhWaitForRundownProtection(
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

// one-time initialization

#define PH_INITONCE_UNINITIALIZED 0
#define PH_INITONCE_INITIALIZED 1
#define PH_INITONCE_INITIALIZING 2

typedef struct _PH_INITONCE
{
    LONG State;
    PH_EVENT WakeEvent;
} PH_INITONCE, *PPH_INITONCE;

#define PH_INITONCE_INIT { PH_INITONCE_UNINITIALIZED, PH_EVENT_INIT }

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

FORCEINLINE BOOLEAN PhBeginInitOnce(
    _Inout_ PPH_INITONCE InitOnce
    )
{
    if (InitOnce->State == PH_INITONCE_INITIALIZED)
        return FALSE;
    else
        return PhfBeginInitOnce(InitOnce);
}

FORCEINLINE BOOLEAN PhTestInitOnce(
    _In_ PPH_INITONCE InitOnce
    )
{
    return InitOnce->State == PH_INITONCE_INITIALIZED;
}

// string

PHLIBAPI
PSTR
NTAPI
PhDuplicateAnsiStringZ(
    _In_ PSTR String
    );

PHLIBAPI
PSTR
NTAPI
PhDuplicateAnsiStringZSafe(
    _In_ PSTR String
    );

PHLIBAPI
PWSTR
NTAPI
PhDuplicateUnicodeStringZ(
    _In_ PWSTR String
    );

PHLIBAPI
BOOLEAN
NTAPI
PhCopyAnsiStringZ(
    _In_ PSTR InputBuffer,
    _In_ ULONG InputCount,
    _Out_writes_opt_z_(OutputCount) PSTR OutputBuffer,
    _In_ ULONG OutputCount,
    _Out_opt_ PULONG ReturnCount
    );

PHLIBAPI
BOOLEAN
NTAPI
PhCopyUnicodeStringZ(
    _In_ PWSTR InputBuffer,
    _In_ ULONG InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ ULONG OutputCount,
    _Out_opt_ PULONG ReturnCount
    );

PHLIBAPI
BOOLEAN
NTAPI
PhCopyUnicodeStringZFromAnsi(
    _In_ PSTR InputBuffer,
    _In_ ULONG InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ ULONG OutputCount,
    _Out_opt_ PULONG ReturnCount
    );

PHLIBAPI
LONG
NTAPI
PhCompareUnicodeStringZNatural(
    _In_ PWSTR A,
    _In_ PWSTR B,
    _In_ BOOLEAN IgnoreCase
    );

FORCEINLINE BOOLEAN PhAreCharactersDifferent(
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

FORCEINLINE BOOLEAN PhIsDigitCharacter(
    _In_ WCHAR Char
    )
{
    return (USHORT)(Char - '0') < 10;
}

FORCEINLINE LONG PhCompareStringZ(
    _In_ PWSTR String1,
    _In_ PWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return wcscmp(String1, String2);
    else
        return wcsicmp(String1, String2);
}

FORCEINLINE BOOLEAN PhEqualStringZ(
    _In_ PWSTR String1,
    _In_ PWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
    {
        return wcscmp(String1, String2) == 0;
    }
    else
    {
        // wcsicmp is very expensive, so we do a quick check for negatives first.
        if (PhAreCharactersDifferent(String1[0], String2[0]))
            return FALSE;

        return wcsicmp(String1, String2) == 0;
    }
}

#define PH_STRING_MAXLEN MAXUINT16

typedef struct _PH_STRINGREF
{
    /** The length, in bytes, of the string. */
    SIZE_T Length;
    /** The buffer containing the contents of the string. */
    PWCH Buffer;
} PH_STRINGREF, *PPH_STRINGREF;

typedef struct _PH_ANSI_STRINGREF
{
    union
    {
        struct
        {
            /** The length, in bytes, of the string. */
            USHORT Length;
            /** Unused and of an undefined value. */
            USHORT Reserved;
            /** The buffer containing the contents of the string. */
            PCHAR Buffer;
        };
        ANSI_STRING as;
    };
} PH_ANSI_STRINGREF, *PPH_ANSI_STRINGREF;

typedef struct _PH_RELATIVE_STRINGREF
{
    /** The length, in bytes, of the string. */
    ULONG Length;
    /** A user-defined offset. */
    ULONG Offset;
} PH_RELATIVE_STRINGREF, *PPH_RELATIVE_STRINGREF;

#define PH_STRINGREF_INIT(String) { sizeof(String) - sizeof(WCHAR), (String) }

FORCEINLINE VOID PhInitializeStringRef(
    _Out_ PPH_STRINGREF String,
    _In_ PWSTR Buffer
    )
{
    String->Length = wcslen(Buffer) * sizeof(WCHAR);
    String->Buffer = Buffer;
}

FORCEINLINE VOID PhInitializeEmptyStringRef(
    _Out_ PPH_STRINGREF String
    )
{
    String->Length = 0;
    String->Buffer = NULL;
}

FORCEINLINE BOOLEAN PhIsNullOrEmptyStringRef(
    _In_opt_ PPH_STRINGREF String
    )
{
    return !String || String->Length == 0;
}

FORCEINLINE BOOLEAN PhStringRefToUnicodeString(
    _In_ PPH_STRINGREF String,
    _Out_ PUNICODE_STRING UnicodeString
    )
{
    UnicodeString->Length = (USHORT)String->Length;
    UnicodeString->MaximumLength = (USHORT)String->Length;
    UnicodeString->Buffer = String->Buffer;

    return String->Length <= UNICODE_STRING_MAX_BYTES;
}

FORCEINLINE VOID PhUnicodeStringToStringRef(
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
    _In_ PPH_STRINGREF String1,
    _In_ PPH_STRINGREF String2,
    _In_ BOOLEAN IgnoreCase
    );

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

FORCEINLINE LONG PhCompareStringRef2(
    _In_ PPH_STRINGREF String1,
    _In_ PWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr2;

    PhInitializeStringRef(&sr2, String2);

    return PhCompareStringRef(String1, &sr2, IgnoreCase);
}

FORCEINLINE BOOLEAN PhEqualStringRef2(
    _In_ PPH_STRINGREF String1,
    _In_ PWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr2;

    PhInitializeStringRef(&sr2, String2);

    return PhEqualStringRef(String1, &sr2, IgnoreCase);
}

FORCEINLINE BOOLEAN PhStartsWithStringRef(
    _In_ PPH_STRINGREF String1,
    _In_ PPH_STRINGREF String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr1;

    sr1.Buffer = String1->Buffer;
    sr1.Length = String2->Length;

    if (String1->Length < sr1.Length)
        return FALSE;

    return PhEqualStringRef(&sr1, String2, IgnoreCase);
}

FORCEINLINE BOOLEAN PhStartsWithStringRef2(
    _In_ PPH_STRINGREF String1,
    _In_ PWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr2;

    PhInitializeStringRef(&sr2, String2);

    return PhStartsWithStringRef(String1, &sr2, IgnoreCase);
}

FORCEINLINE BOOLEAN PhEndsWithStringRef(
    _In_ PPH_STRINGREF String1,
    _In_ PPH_STRINGREF String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr1;

    if (String2->Length > String1->Length)
        return FALSE;

    sr1.Buffer = (PWCHAR)((PCHAR)String1->Buffer + String1->Length - String2->Length);
    sr1.Length = String2->Length;

    return PhEqualStringRef(&sr1, String2, IgnoreCase);
}

FORCEINLINE BOOLEAN PhEndsWithStringRef2(
    _In_ PPH_STRINGREF String1,
    _In_ PWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr2;

    PhInitializeStringRef(&sr2, String2);

    return PhEndsWithStringRef(String1, &sr2, IgnoreCase);
}

FORCEINLINE VOID PhReverseStringRef(
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
 * A Unicode string object.
 *
 * \remarks The \a Length never includes the null terminator. Every
 * string must have a null terminator at the end, for compatibility
 * reasons. Thus the invariant is:
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
PhCreateString(
    _In_ PWSTR Buffer
    );

PHLIBAPI
PPH_STRING
NTAPI
PhCreateStringEx(
    _In_opt_ PWCHAR Buffer,
    _In_ SIZE_T Length
    );

PHLIBAPI
PPH_STRING
NTAPI
PhCreateStringFromAnsi(
    _In_ PSTR Buffer
    );

PHLIBAPI
PPH_STRING
NTAPI
PhCreateStringFromAnsiEx(
    _In_ PCHAR Buffer,
    _In_ SIZE_T Length
    );

PHLIBAPI
PPH_STRING
NTAPI
PhReferenceEmptyString(
    VOID
    );

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
    _In_ PWSTR String1,
    _In_ PWSTR String2
    );

PHLIBAPI
PPH_STRING
NTAPI
PhConcatStringRef2(
    _In_ PPH_STRINGREF String1,
    _In_ PPH_STRINGREF String2
    );

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
PhFormatString(
    _In_ _Printf_format_string_ PWSTR Format,
    ...
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatString_V(
    _In_ _Printf_format_string_ PWSTR Format,
    _In_ va_list ArgPtr
    );

/**
 * Retrieves a pointer to a string object's buffer
 * or returns NULL.
 *
 * \param String A pointer to a string object.
 *
 * \return A pointer to the string object's buffer
 * if the supplied pointer is non-NULL, otherwise
 * NULL.
 */
FORCEINLINE PWSTR PhGetString(
    _In_opt_ PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return NULL;
}

FORCEINLINE PH_STRINGREF PhGetStringRef(
    _In_opt_ PPH_STRING String
    )
{
    PH_STRINGREF sr;

    if (String)
    {
        sr = String->sr;
    }
    else
    {
        PhInitializeEmptyStringRef(&sr);
    }

    return sr;
}

/**
 * Retrieves a pointer to a string object's buffer
 * or returns an empty string.
 *
 * \param String A pointer to a string object.
 *
 * \return A pointer to the string object's buffer
 * if the supplied pointer is non-NULL, otherwise
 * an empty string.
 */
FORCEINLINE PWSTR PhGetStringOrEmpty(
    _In_opt_ PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return L"";
}

FORCEINLINE PH_STRINGREF PhGetStringRefOrEmpty(
    _In_opt_ PPH_STRING String
    )
{
    PH_STRINGREF sr;

    if (String)
    {
        sr = String->sr;
    }
    else
    {
        sr.Length = 0;
        sr.Buffer = L"";
    }

    return sr;
}

/**
 * Retrieves a pointer to a string object's buffer
 * or returns the specified alternative string.
 *
 * \param String A pointer to a string object.
 * \param DefaultString The alternative string.
 *
 * \return A pointer to the string object's buffer
 * if the supplied pointer is non-NULL, otherwise
 * the specified alternative string.
 */
FORCEINLINE PWSTR PhGetStringOrDefault(
    _In_opt_ PPH_STRING String,
    _In_ PWSTR DefaultString
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
FORCEINLINE BOOLEAN PhIsNullOrEmptyString(
    _In_opt_ PPH_STRING String
    )
{
    return !String || String->Length == 0;
}

/**
 * Duplicates a string.
 *
 * \param String A string to duplicate.
 */
FORCEINLINE PPH_STRING PhDuplicateString(
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
FORCEINLINE LONG PhCompareString(
    _In_ PPH_STRING String1,
    _In_ PPH_STRING String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return wcscmp(String1->Buffer, String2->Buffer);
    else
        return PhCompareStringRef(&String1->sr, &String2->sr, IgnoreCase); // faster than wcsicmp
}

/**
 * Compares two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
FORCEINLINE LONG PhCompareString2(
    _In_ PPH_STRING String1,
    _In_ PWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
    {
        return wcscmp(String1->Buffer, String2);
    }
    else
    {
        return PhCompareStringRef2(&String1->sr, String2, IgnoreCase);
    }
}

/**
 * Compares two strings, handling NULL strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
FORCEINLINE LONG PhCompareStringWithNull(
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
        return !String2 ? 0 : -1;
    }
    else
    {
        return 1;
    }
}

/**
 * Determines whether two strings are equal.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
FORCEINLINE BOOLEAN PhEqualString(
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
FORCEINLINE BOOLEAN PhEqualString2(
    _In_ PPH_STRING String1,
    _In_ PWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
    {
        return wcscmp(String1->Buffer, String2) == 0;
    }
    else
    {
        return PhEqualStringRef2(&String1->sr, String2, IgnoreCase);
    }
}

/**
 * Determines whether a string starts with another.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 *
 * \return TRUE if \a String1 starts with \a String2,
 * otherwise FALSE.
 */
FORCEINLINE BOOLEAN PhStartsWithString(
    _In_ PPH_STRING String1,
    _In_ PPH_STRING String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    return PhStartsWithStringRef(&String1->sr, &String2->sr, IgnoreCase);
}

/**
 * Determines whether a string starts with another.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 *
 * \return TRUE if \a String1 starts with \a String2,
 * otherwise FALSE.
 */
FORCEINLINE BOOLEAN PhStartsWithString2(
    _In_ PPH_STRING String1,
    _In_ PWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr2;

    PhInitializeStringRef(&sr2, String2);

    return PhStartsWithStringRef(&String1->sr, &sr2, IgnoreCase);
}

/**
 * Determines whether a string ends with another.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 *
 * \return TRUE if \a String1 ends with \a String2,
 * otherwise FALSE.
 */
FORCEINLINE BOOLEAN PhEndsWithString(
    _In_ PPH_STRING String1,
    _In_ PPH_STRING String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    return PhEndsWithStringRef(&String1->sr, &String2->sr, IgnoreCase);
}

/**
 * Determines whether a string ends with another.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 *
 * \return TRUE if \a String1 ends with \a String2,
 * otherwise FALSE.
 */
FORCEINLINE BOOLEAN PhEndsWithString2(
    _In_ PPH_STRING String1,
    _In_ PWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF sr2;

    PhInitializeStringRef(&sr2, String2);

    return PhEndsWithStringRef(&String1->sr, &sr2, IgnoreCase);
}

/**
 * Locates a character in a string.
 *
 * \param String The string to search.
 * \param StartIndex The index, in characters, to start searching at.
 * \param Char The character to search for.
 *
 * \return The index, in characters, of the first occurrence of
 * \a Char in \a String after \a StartIndex. If \a Char was not
 * found, -1 is returned.
 */
FORCEINLINE ULONG_PTR PhFindCharInString(
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

        sr.Buffer += StartIndex;
        sr.Length -= StartIndex * sizeof(WCHAR);

        r = PhFindCharInStringRef(&sr, Char, FALSE);

        if (r != -1)
            return r + StartIndex;
        else
            return -1;
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
 * \return The index, in characters, of the last occurrence of
 * \a Char in \a String after \a StartIndex. If \a Char was not
 * found, -1 is returned.
 */
FORCEINLINE ULONG_PTR PhFindLastCharInString(
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

        sr.Buffer += StartIndex;
        sr.Length -= StartIndex * sizeof(WCHAR);

        r = PhFindLastCharInStringRef(&sr, Char, FALSE);

        if (r != -1)
            return r + StartIndex;
        else
            return -1;
    }
    else
    {
        return PhFindLastCharInStringRef(&String->sr, Char, FALSE);
    }
}

/**
 * Locates a string in a string.
 *
 * \param String1 The string to search.
 * \param StartIndex The index, in characters, to start searching at.
 * \param String2 The string to search for.
 *
 * \return The index, in characters, of the first occurrence of
 * \a String2 in \a String1 after \a StartIndex. If \a String2 was not
 * found, -1 is returned.
 */
FORCEINLINE ULONG_PTR PhFindStringInString(
    _In_ PPH_STRING String1,
    _In_ SIZE_T StartIndex,
    _In_ PWSTR String2
    )
{
    PH_STRINGREF sr2;

    PhInitializeStringRef(&sr2, String2);

    if (StartIndex != 0)
    {
        ULONG_PTR r;
        PH_STRINGREF sr1;

        sr1 = String1->sr;

        sr1.Buffer += StartIndex;
        sr1.Length -= StartIndex * sizeof(WCHAR);

        r = PhFindStringInStringRef(&sr1, &sr2, FALSE);

        if (r != -1)
            return r + StartIndex;
        else
            return -1;
    }
    else
    {
        return PhFindStringInStringRef(&String1->sr, &sr2, FALSE);
    }
}

/**
 * Converts a string to lowercase in-place.
 *
 * \param String The string to convert.
 */
FORCEINLINE VOID PhLowerString(
    _Inout_ PPH_STRING String
    )
{
    _wcslwr(String->Buffer);
}

/**
 * Converts a string to uppercase in-place.
 *
 * \param String The string to convert.
 */
FORCEINLINE VOID PhUpperString(
    _Inout_ PPH_STRING String
    )
{
    _wcsupr(String->Buffer);
}

/**
 * Creates a substring of a string.
 *
 * \param String The original string.
 * \param StartIndex The start index, in characters.
 * \param Count The number of characters to use.
 */
FORCEINLINE PPH_STRING PhSubstring(
    _In_ PPH_STRING String,
    _In_ SIZE_T StartIndex,
    _In_ SIZE_T Count
    )
{
    return PhCreateStringEx(&String->Buffer[StartIndex], Count * sizeof(WCHAR));
}

/**
 * Updates a string object's length with
 * its true length as determined by an
 * embedded null terminator.
 *
 * \param String The string to modify.
 *
 * \remarks Use this function after modifying a string
 * object's buffer manually.
 */
FORCEINLINE VOID PhTrimToNullTerminatorString(
    _Inout_ PPH_STRING String
    )
{
    String->Length = wcslen(String->Buffer) * sizeof(WCHAR);
}

// ansi string

extern PPH_OBJECT_TYPE PhAnsiStringType;

/**
 * An ANSI string structure.
 */
typedef struct _PH_ANSI_STRING
{
    union
    {
        /** An embedded ANSI_STRING structure. */
        ANSI_STRING as;
        PH_ANSI_STRINGREF asr;
        struct
        {
            /** The length, in bytes, of the string. */
            USHORT Length;
            /** Unused and always equal to \ref Length. */
            USHORT MaximumLength;
            /** Unused and always a pointer to \ref Buffer. */
            PSTR Pointer;
        };
    };
    /** The buffer containing the contents of the string. */
    CHAR Buffer[1];
} PH_ANSI_STRING, *PPH_ANSI_STRING;

PHLIBAPI
PPH_ANSI_STRING
NTAPI
PhCreateAnsiString(
    _In_ PSTR Buffer
    );

PHLIBAPI
PPH_ANSI_STRING
NTAPI
PhCreateAnsiStringEx(
    _In_opt_ PCHAR Buffer,
    _In_ SIZE_T Length
    );

PHLIBAPI
PPH_ANSI_STRING
NTAPI
PhCreateAnsiStringFromUnicode(
    _In_ PWSTR Buffer
    );

PHLIBAPI
PPH_ANSI_STRING
NTAPI
PhCreateAnsiStringFromUnicodeEx(
    _In_ PWCHAR Buffer,
    _In_ SIZE_T Length
    );

// stringbuilder

/**
 * A string builder structure.
 * The string builder object allows you to easily
 * construct complex strings without allocating
 * a great number of strings in the process.
 */
typedef struct _PH_STRING_BUILDER
{
    /** Allocated length of the string, not including the null terminator. */
    SIZE_T AllocatedLength;
    /**
     * The constructed string.
     * \a String will be allocated for \a AllocatedLength,
     * but we will modify the \a Length field to be the
     * correct length.
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
PhReferenceStringBuilderString(
    _In_ PPH_STRING_BUILDER StringBuilder
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
PhAppendStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ PPH_STRING String
    );

PHLIBAPI
VOID
NTAPI
PhAppendStringBuilder2(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ PWSTR String
    );

PHLIBAPI
VOID
NTAPI
PhAppendStringBuilderEx(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_opt_ PWCHAR String,
    _In_ SIZE_T Length
    );

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
    _In_ _Printf_format_string_ PWSTR Format,
    ...
    );

VOID
NTAPI
PhAppendFormatStringBuilder_V(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ _Printf_format_string_ PWSTR Format,
    _In_ va_list ArgPtr
    );

PHLIBAPI
VOID
NTAPI
PhInsertStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_ PPH_STRING String
    );

PHLIBAPI
VOID
NTAPI
PhInsertStringBuilder2(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_ PWSTR String
    );

PHLIBAPI
VOID
NTAPI
PhInsertStringBuilderEx(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_opt_ PWCHAR String,
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

// list

extern PPH_OBJECT_TYPE PhListType;

/**
 * A list structure.
 * Storage is automatically allocated for new
 * elements.
 */
typedef struct _PH_LIST
{
    /** The number of items in the list. */
    ULONG Count;
    /** The number of items for which storage is allocated. */
    ULONG AllocatedCount;
    /** The array of list items. */
    PVOID *Items;
} PH_LIST, *PPH_LIST;

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

// pointer list

extern PPH_OBJECT_TYPE PhPointerListType;

/**
 * A pointer list structure.
 * The pointer list is similar to the normal list
 * structure, but both insertions and deletions
 * occur in constant time. The list is not ordered.
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

FORCEINLINE BOOLEAN PhEnumPointerList(
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

// hash

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
FORCEINLINE VOID PhInitializeHashSet(
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
 * \return The allocated hash set. You must free it with
 * PhFree() when you no longer need it.
 */
FORCEINLINE PPH_HASH_ENTRY *PhCreateHashSet(
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
FORCEINLINE ULONG PhCountHashSet(
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
FORCEINLINE VOID PhDistributeHashSet(
    _Inout_ PPH_HASH_ENTRY *NewBuckets,
    _In_ ULONG NumberOfNewBuckets,
    _In_ PPH_HASH_ENTRY *OldBuckets,
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
FORCEINLINE VOID PhAddEntryHashSet(
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
 * \remarks If the function returns NULL, the entry
 * does not exist in the hash set.
 */
FORCEINLINE PPH_HASH_ENTRY PhFindEntryHashSet(
    _In_ PPH_HASH_ENTRY *Buckets,
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
FORCEINLINE VOID PhRemoveEntryHashSet(
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
 * \param Buckets A pointer to the bucket array. On return the new bucket
 * array is stored in this variable.
 * \param NumberOfBuckets A pointer to the number of buckets. On return the
 * new number of buckets is stored in this variable.
 * \param NewNumberOfBuckets The new number of buckets.
 */
FORCEINLINE VOID PhResizeHashSet(
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

// hashtable

extern PPH_OBJECT_TYPE PhHashtableType;

typedef struct _PH_HASHTABLE_ENTRY
{
    /** Hash code of the entry. -1 if entry is unused. */
    ULONG HashCode;
    /** Either the index of the next entry in the bucket,
     * the index of the next free entry, or -1 for invalid.
     */
    ULONG Next;
    /** The beginning of user data. */
    QUAD Body;
} PH_HASHTABLE_ENTRY, *PPH_HASHTABLE_ENTRY;

/**
 * A comparison function used by a hashtable.
 *
 * \param Entry1 The first entry.
 * \param Entry2 The second entry.
 *
 * \return TRUE if the entries are equal, otherwise
 * FALSE.
 */
typedef BOOLEAN (NTAPI *PPH_HASHTABLE_COMPARE_FUNCTION)(
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
 * \li Two entries which are considered to be equal
 * by the comparison function must be given the same
 * hash code.
 * \li Two different entries do not have to be given
 * different hash codes.
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
    PPH_HASHTABLE_COMPARE_FUNCTION CompareFunction;
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
    /** Index of next usable index into entry array, a.k.a. the
     * count of entries that were ever allocated.
     */
    ULONG NextEntry;
} PH_HASHTABLE, *PPH_HASHTABLE;

#define PH_HASHTABLE_ENTRY_SIZE(InnerSize) (FIELD_OFFSET(PH_HASHTABLE_ENTRY, Body) + (InnerSize))
#define PH_HASHTABLE_GET_ENTRY(Hashtable, Index) \
    ((PPH_HASHTABLE_ENTRY)PTR_ADD_OFFSET((Hashtable)->Entries, \
    PH_HASHTABLE_ENTRY_SIZE((Hashtable)->EntrySize) * (Index)))
#define PH_HASHTABLE_GET_ENTRY_INDEX(Hashtable, Entry) \
    ((ULONG)(PTR_ADD_OFFSET(Entry, -(Hashtable)->Entries) / \
    PH_HASHTABLE_ENTRY_SIZE((Hashtable)->EntrySize)))

PHLIBAPI
PPH_HASHTABLE
NTAPI
PhCreateHashtable(
    _In_ ULONG EntrySize,
    _In_ PPH_HASHTABLE_COMPARE_FUNCTION CompareFunction,
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

FORCEINLINE VOID PhBeginEnumHashtable(
    _In_ PPH_HASHTABLE Hashtable,
    _Out_ PPH_HASHTABLE_ENUM_CONTEXT Context
    )
{
    Context->Current = (ULONG_PTR)Hashtable->Entries;
    Context->Step = PH_HASHTABLE_ENTRY_SIZE(Hashtable->EntrySize);
    Context->End = Context->Current + (ULONG_PTR)Hashtable->NextEntry * Context->Step;
}

FORCEINLINE PVOID PhNextEnumHashtable(
    _Inout_ PPH_HASHTABLE_ENUM_CONTEXT Context
    )
{
    PPH_HASHTABLE_ENTRY entry;

    while (Context->Current != Context->End)
    {
        entry = (PPH_HASHTABLE_ENTRY)Context->Current;
        Context->Current += Context->Step;

        if (entry->HashCode != -1)
            return &entry->Body;
    }

    return NULL;
}

#define PhHashBytes PhHashBytesSdbm

#define PhHashBytesHsieh PhfHashBytesHsieh
PHLIBAPI
ULONG
FASTCALL
PhfHashBytesHsieh(
    _In_ PUCHAR Bytes,
    _In_ SIZE_T Length
    );

#define PhHashBytesMurmur PhfHashBytesMurmur
PHLIBAPI
ULONG
FASTCALL
PhfHashBytesMurmur(
    _In_ PUCHAR Bytes,
    _In_ SIZE_T Length
    );

#define PhHashBytesSdbm PhfHashBytesSdbm
PHLIBAPI
ULONG
FASTCALL
PhfHashBytesSdbm(
    _In_ PUCHAR Bytes,
    _In_ SIZE_T Length
    );

FORCEINLINE ULONG PhHashInt32(
    _In_ ULONG Value
    )
{
    // Java style.
    Value ^= (Value >> 20) ^ (Value >> 12);
    return Value ^ (Value >> 7) ^ (Value >> 4);
}

FORCEINLINE ULONG PhHashInt64(
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

// simple hashtable

typedef struct _PH_KEY_VALUE_PAIR
{
    PVOID Key;
    PVOID Value;
} PH_KEY_VALUE_PAIR, *PPH_KEY_VALUE_PAIR;

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

PHLIBAPI
BOOLEAN
NTAPI
PhRemoveItemSimpleHashtable(
    _Inout_ PPH_HASHTABLE SimpleHashtable,
    _In_opt_ PVOID Key
    );

// free list

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

#ifdef _M_IX86
C_ASSERT(FIELD_OFFSET(PH_FREE_LIST_ENTRY, ListEntry) == 0x0);
C_ASSERT(FIELD_OFFSET(PH_FREE_LIST_ENTRY, Body) == 0x8);
#else
C_ASSERT(FIELD_OFFSET(PH_FREE_LIST_ENTRY, ListEntry) == 0x0);
C_ASSERT(FIELD_OFFSET(PH_FREE_LIST_ENTRY, Body) == 0x10);
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

// callback

/**
 * A callback function.
 *
 * \param Parameter A value given to all callback
 * functions being notified.
 * \param Context A user-defined value passed
 * to PhRegisterCallback().
 */
typedef VOID (NTAPI *PPH_CALLBACK_FUNCTION)(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

/**
 * A callback registration structure.
 */
typedef struct _PH_CALLBACK_REGISTRATION
{
    /** The list entry in the callbacks list. */
    LIST_ENTRY ListEntry;
    /** The callback function. */
    PPH_CALLBACK_FUNCTION Function;
    /** A user-defined value to be passed to the
     * callback function. */
    PVOID Context;
    /** A value indicating whether the registration
     * structure is being used. */
    LONG Busy;
    /** Whether the registration structure is being
     * removed. */
    BOOLEAN Unregistering;
    BOOLEAN Reserved;
    /** Flags controlling the callback. */
    USHORT Flags;
} PH_CALLBACK_REGISTRATION, *PPH_CALLBACK_REGISTRATION;

/**
 * A callback structure.
 * The callback object allows multiple callback
 * functions to be registered and notified in a
 * thread-safe way.
 */
typedef struct _PH_CALLBACK
{
    /** The list of registered callbacks. */
    LIST_ENTRY ListHead;
    /** A lock protecting the callbacks list. */
    PH_QUEUED_LOCK ListLock;
    /** A condition variable pulsed when
     * the callback becomes free. */
    PH_QUEUED_LOCK BusyCondition;
} PH_CALLBACK, *PPH_CALLBACK;

#define PH_CALLBACK_DECLARE(Name) PH_CALLBACK Name = { &Name.ListHead, &Name.ListHead, PH_QUEUED_LOCK_INIT, PH_QUEUED_LOCK_INIT }

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

// general

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
ULONG
NTAPI
PhLog2(
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
PPH_STRING
NTAPI
PhBufferToHexString(
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ ULONG Length
    );

PPH_STRING
NTAPI
PhBufferToHexStringEx(
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN UpperCase
    );

PHLIBAPI
BOOLEAN
NTAPI
PhStringToInteger64(
    _In_ PPH_STRINGREF String,
    _In_opt_ ULONG Base,
    _Out_opt_ PLONG64 Integer
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

PHLIBAPI
VOID
NTAPI
PhPrintTimeSpan(
    _Out_writes_(PH_TIMESPAN_STR_LEN_1) PWSTR Destination,
    _In_ ULONG64 Ticks,
    _In_opt_ ULONG Mode
    );

// format

VOID
PhZeroExtendToUnicode(
    _In_reads_bytes_(InputLength) PSTR Input,
    _In_ ULONG InputLength,
    _Out_writes_bytes_(InputLength * 2) PWSTR Output
    );

typedef enum _PH_FORMAT_TYPE
{
    CharFormatType,
    StringFormatType,
    StringZFormatType,
    AnsiStringFormatType,
    AnsiStringZFormatType,
    Int32FormatType,
    Int64FormatType,
    IntPtrFormatType,
    UInt32FormatType,
    UInt64FormatType,
    UIntPtrFormatType,
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

    // Floating-point flags
    /** Use standard form instead of normal form */
    FormatStandardForm = 0x1000,
    /** Use hexadecimal form instead of normal form */
    FormatHexadecimalForm = 0x2000,
    /** Reserved */
    FormatForceDecimalPoint = 0x4000,
    /** Trailing zeros and possibly the decimal point are trimmed */
    FormatCropZeros = 0x8000,

    // Floating-point and integer flags
    /** Group digits (with floating-point, only works when in normal form) */
    FormatGroupDigits = 0x10000,
    /** Always insert a prefix, '+' for positive and '-' for negative */
    FormatPrefixSign = 0x20000,
    /** Pad left with zeros, taking into consideration the sign. Width must be specified.
     * Format*Align cannot be used in conjunction with this flag. If FormatGroupDigits is specified,
     * this flag is ignored. */
    FormatPadZeros = 0x40000,

    // General flags
    /** Applies left alignment. Width must be specified. */
    FormatLeftAlign = 0x80000000,
    /** Applies right alignment. Width must be specified. */
    FormatRightAlign = 0x40000000,
    /** Make characters uppercase (only available for some types) */
    FormatUpperCase = 0x20000000
} PH_FORMAT_TYPE;

/**
 * Describes an element to be formatted to a string.
 */
typedef struct _PH_FORMAT
{
    /** Specifies the type of the element and optional flags. */
    PH_FORMAT_TYPE Type;
    /** The precision of the element. The meaning of this field depends on
     * the element type. For \a Double and \a Size, this field specifies
     * the number of decimal points to include. */
    USHORT Precision;
    /** The width of the element. This field specifies the minimum
     * number of characters to output. The remaining space is
     * padded with either spaces, zeros, or a custom character. */
    USHORT Width;
    /** The pad character. */
    WCHAR Pad;
    /** The meaning of this field depends on the element type. For integer
     * types, this field specifies the base to convert the number into.
     * For \a Size, this field specifies the maximum size unit. */
    UCHAR Radix;
    /** The meaning of this field depends on the element type. For \a Size,
     * this field specifies the minimum size unit. */
    UCHAR Parameter;
    union
    {
        WCHAR Char;
        PH_STRINGREF String;
        PWSTR StringZ;
        PH_ANSI_STRINGREF AnsiString;
        PSTR AnsiStringZ;
        LONG Int32;
        LONG64 Int64;
        LONG_PTR IntPtr;
        ULONG UInt32;
        ULONG64 UInt64;
        ULONG_PTR UIntPtr;
        DOUBLE Double;

        ULONG64 Size;
    } u;
} PH_FORMAT, *PPH_FORMAT;

// Convenience macros
#define PhInitFormatC(f, v) do { (f)->Type = CharFormatType; (f)->u.Char = (v); } while (0)
#define PhInitFormatS(f, v) do { (f)->Type = StringFormatType; PhInitializeStringRef(&(f)->u.String, (v)); } while (0)
#define PhInitFormatSR(f, v) do { (f)->Type = StringFormatType; (f)->u.String = (v); } while (0)
#define PhInitFormatAnsiS(f, v) do { (f)->Type = AnsiStringFormatType; PhInitializeAnsiStringRef(&(f)->u.AnsiString, (v)); } while (0)
#define PhInitFormatD(f, v) do { (f)->Type = Int32FormatType; (f)->u.Int32 = (v); } while (0)
#define PhInitFormatU(f, v) do { (f)->Type = UInt32FormatType; (f)->u.UInt32 = (v); } while (0)
#define PhInitFormatX(f, v) do { (f)->Type = UInt32FormatType | FormatUseRadix; (f)->u.UInt32 = (v); (f)->Radix = 16; } while (0)
#define PhInitFormatI64D(f, v) do { (f)->Type = Int64FormatType; (f)->u.Int64 = (v); } while (0)
#define PhInitFormatI64U(f, v) do { (f)->Type = UInt64FormatType; (f)->u.UInt64 = (v); } while (0)
#define PhInitFormatI64X(f, v) do { (f)->Type = UInt64FormatType | FormatUseRadix; (f)->u.UInt64 = (v); (f)->Radix = 16; } while (0)
#define PhInitFormatIU(f, v) do { (f)->Type = UIntPtrFormatType; (f)->u.UIntPtr = (v); } while (0)
#define PhInitFormatIX(f, v) do { (f)->Type = UIntPtrFormatType | FormatUseRadix; (f)->u.UIntPtr = (v); (f)->Radix = 16; } while (0)
#define PhInitFormatF(f, v, p) do { (f)->Type = DoubleFormatType | FormatUsePrecision; (f)->u.Double = (v); (f)->Precision = (p); } while (0)
#define PhInitFormatE(f, v, p) do { (f)->Type = DoubleFormatType | FormatStandardForm | FormatUsePrecision; (f)->u.Double = (v); (f)->Precision = (p); } while (0)
#define PhInitFormatA(f, v, p) do { (f)->Type = DoubleFormatType | FormatHexadecimalForm | FormatUsePrecision; (f)->u.Double = (v); (f)->Precision = (p); } while (0)
#define PhInitFormatSize(f, v) do { (f)->Type = SizeFormatType; (f)->u.Size = (v); } while (0)

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

// basesupa

PHLIBAPI
PPH_STRING
NTAPI
PhaCreateString(
    _In_ PWSTR Buffer
    );

PHLIBAPI
PPH_STRING
NTAPI
PhaCreateStringEx(
    _In_opt_ PWSTR Buffer,
    _In_ SIZE_T Length
    );

PHLIBAPI
PPH_STRING
NTAPI
PhaDuplicateString(
    _In_ PPH_STRING String
    );

PHLIBAPI
PPH_STRING
NTAPI
PhaConcatStrings(
    _In_ ULONG Count,
    ...
    );

PHLIBAPI
PPH_STRING
NTAPI
PhaConcatStrings2(
    _In_ PWSTR String1,
    _In_ PWSTR String2
    );

PHLIBAPI
PPH_STRING
NTAPI
PhaFormatString(
    _In_ _Printf_format_string_ PWSTR Format,
    ...
    );

PHLIBAPI
PPH_STRING
NTAPI
PhaLowerString(
    _In_ PPH_STRING String
    );

PHLIBAPI
PPH_STRING
NTAPI
PhaUpperString(
    _In_ PPH_STRING String
    );

PHLIBAPI
PPH_STRING
NTAPI
PhaSubstring(
    _In_ PPH_STRING String,
    _In_ SIZE_T StartIndex,
    _In_ SIZE_T Count
    );

// basesupx (extra)

PHLIBAPI
VOID
FASTCALL
PhxfFillMemoryUlong(
    _Inout_ PULONG Memory,
    _In_ ULONG Value,
    _In_ ULONG Count
    );

PHLIBAPI
VOID
FASTCALL
PhxfAddInt32(
    _Inout_ _Needs_align_(16) PLONG A,
    _In_ _Needs_align_(16) PLONG B,
    _In_ ULONG Count
    );

PHLIBAPI
VOID
FASTCALL
PhxfAddInt32U(
    _Inout_ PLONG A,
    _In_ PLONG B,
    _In_ ULONG Count
    );

PHLIBAPI
VOID
FASTCALL
PhxfDivideSingleU(
    _Inout_ PFLOAT A,
    _In_ PFLOAT B,
    _In_ ULONG Count
    );

PHLIBAPI
VOID
FASTCALL
PhxfDivideSingle2U(
    _Inout_ PFLOAT A,
    _In_ FLOAT B,
    _In_ ULONG Count
    );

// error

PHLIBAPI
ULONG
NTAPI
PhNtStatusToDosError(
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

// collect

// Generic tree definitions

typedef enum _PH_TREE_ENUMERATION_ORDER
{
    TreeEnumerateInOrder,
    TreeEnumerateInReverseOrder
} PH_TREE_ENUMERATION_ORDER;

#define PhIsLeftChildElement(Links) ((Links)->Parent->Left == (Links))
#define PhIsRightChildElement(Links) ((Links)->Parent->Right == (Links))

// AVL trees

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
PhFindElementAvlTree2(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element,
    _Out_ PLONG Result
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

// handle

struct _PH_HANDLE_TABLE;
typedef struct _PH_HANDLE_TABLE *PPH_HANDLE_TABLE;

typedef struct _PH_HANDLE_TABLE_ENTRY
{
    union
    {
        PVOID Object;
        ULONG_PTR Value;
        struct
        {
            /** The type of the entry; 1 if the entry is free,
             * otherwise 0 if the entry is in use.
             */
            ULONG_PTR Type : 1;
            /** Whether the entry is not locked; 1 if the entry
             * is not locked, otherwise 0 if the entry is locked.
             */
            ULONG_PTR Locked : 1;
            ULONG_PTR Value : sizeof(ULONG_PTR) * 8 - 2;
        } TypeAndValue;
    };
    union
    {
        ACCESS_MASK GrantedAccess;
        ULONG NextFreeValue;
        ULONG_PTR Value2;
    };
} PH_HANDLE_TABLE_ENTRY, *PPH_HANDLE_TABLE_ENTRY;

#define PH_HANDLE_TABLE_SAFE
#define PH_HANDLE_TABLE_FREE_COUNT 64

#define PH_HANDLE_TABLE_STRICT_FIFO 0x1
#define PH_HANDLE_TABLE_VALID_FLAGS 0x1

VOID PhHandleTableInitialization(
    VOID
    );

PHLIBAPI
PPH_HANDLE_TABLE
NTAPI
PhCreateHandleTable(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhDestroyHandleTable(
    _In_ _Post_invalid_ PPH_HANDLE_TABLE HandleTable
    );

PHLIBAPI
BOOLEAN
NTAPI
PhLockHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _Inout_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    );

PHLIBAPI
VOID
NTAPI
PhUnlockHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _Inout_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    );

PHLIBAPI
HANDLE
NTAPI
PhCreateHandle(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDestroyHandle(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ HANDLE Handle,
    _In_opt_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    );

PHLIBAPI
PPH_HANDLE_TABLE_ENTRY
NTAPI
PhLookupHandleTableEntry(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ HANDLE Handle
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_HANDLE_TABLE_CALLBACK)(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ HANDLE Handle,
    _In_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry,
    _In_opt_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhEnumHandleTable(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ PPH_ENUM_HANDLE_TABLE_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhSweepHandleTable(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ PPH_ENUM_HANDLE_TABLE_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

typedef enum _PH_HANDLE_TABLE_INFORMATION_CLASS
{
    HandleTableBasicInformation,
    HandleTableFlagsInformation,
    MaxHandleTableInfoClass
} PH_HANDLE_TABLE_INFORMATION_CLASS;

typedef struct _PH_HANDLE_TABLE_BASIC_INFORMATION
{
    ULONG Count;
    ULONG Flags;
    ULONG TableLevel;
} PH_HANDLE_TABLE_BASIC_INFORMATION, *PPH_HANDLE_TABLE_BASIC_INFORMATION;

typedef struct _PH_HANDLE_TABLE_FLAGS_INFORMATION
{
    ULONG Flags;
} PH_HANDLE_TABLE_FLAGS_INFORMATION, *PPH_HANDLE_TABLE_FLAGS_INFORMATION;

PHLIBAPI
NTSTATUS
NTAPI
PhQueryInformationHandleTable(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ PH_HANDLE_TABLE_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetInformationHandleTable(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ PH_HANDLE_TABLE_INFORMATION_CLASS InformationClass,
    _In_reads_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    );

// workqueue

#if !defined(_PH_WORKQUEUE_PRIVATE) && defined(DEBUG)
extern PPH_LIST PhDbgWorkQueueList;
extern PH_QUEUED_LOCK PhDbgWorkQueueListLock;
#endif

typedef struct _PH_WORK_QUEUE
{
    PH_RUNDOWN_PROTECT RundownProtect;
    BOOLEAN Terminating;

    LIST_ENTRY QueueListHead;
    PH_QUEUED_LOCK QueueLock;

    ULONG MaximumThreads;
    ULONG MinimumThreads;
    ULONG NoWorkTimeout;

    PH_QUEUED_LOCK StateLock;
    HANDLE SemaphoreHandle;
    ULONG CurrentThreads;
    ULONG BusyThreads;
} PH_WORK_QUEUE, *PPH_WORK_QUEUE;

typedef struct _PH_WORK_QUEUE_ITEM
{
    LIST_ENTRY ListEntry;
    PTHREAD_START_ROUTINE Function;
    PVOID Context;
} PH_WORK_QUEUE_ITEM, *PPH_WORK_QUEUE_ITEM;

VOID PhWorkQueueInitialization(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhInitializeWorkQueue(
    _Out_ PPH_WORK_QUEUE WorkQueue,
    _In_ ULONG MinimumThreads,
    _In_ ULONG MaximumThreads,
    _In_ ULONG NoWorkTimeout
    );

PHLIBAPI
VOID
NTAPI
PhDeleteWorkQueue(
    _Inout_ PPH_WORK_QUEUE WorkQueue
    );

PHLIBAPI
VOID
NTAPI
PhQueueItemWorkQueue(
    _Inout_ PPH_WORK_QUEUE WorkQueue,
    _In_ PTHREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhQueueItemGlobalWorkQueue(
    _In_ PTHREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context
    );

#ifdef __cplusplus
}
#endif

#endif