#ifndef PHBASE_H
#define PHBASE_H

#include <base.h>
#include <ntimport.h>
#include <ref.h>
#include <fastlock.h>
#include <queuedlock.h>

#ifndef PHLIB_GLOBAL_PRIVATE

struct _PH_STRING;
typedef struct _PH_STRING *PPH_STRING;

struct _PH_PROVIDER_THREAD;
typedef struct _PH_PROVIDER_THREAD PH_PROVIDER_THREAD;

struct _PH_STARTUP_PARAMETERS;
typedef struct _PH_STARTUP_PARAMETERS PH_STARTUP_PARAMETERS;

#define __userSet

extern __userSet HFONT PhApplicationFont;
extern __userSet PWSTR PhApplicationName;
extern __userSet HFONT PhBoldListViewFont;
extern __userSet HFONT PhBoldMessageFont;
extern BOOLEAN PhElevated;
extern HANDLE PhHeapHandle;
extern __userSet HFONT PhIconTitleFont;
extern HINSTANCE PhInstanceHandle;
extern __userSet HANDLE PhKphHandle;
extern __userSet ULONG PhKphFeatures;
extern SYSTEM_BASIC_INFORMATION PhSystemBasicInformation;
extern ULONG WindowsVersion;

extern ACCESS_MASK ProcessQueryAccess;
extern ACCESS_MASK ProcessAllAccess;
extern ACCESS_MASK ThreadQueryAccess;
extern ACCESS_MASK ThreadSetAccess;
extern ACCESS_MASK ThreadAllAccess;

#endif

#define WINDOWS_ANCIENT 0
#define WINDOWS_XP 51
#define WINDOWS_SERVER_2003 52
#define WINDOWS_VISTA 60
#define WINDOWS_7 61
#define WINDOWS_NEW MAXLONG

#define WINDOWS_HAS_IFILEDIALOG (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_LIMITED_ACCESS (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_PSSUSPENDRESUMEPROCESS (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_THREAD_CYCLES (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_UAC (WindowsVersion >= WINDOWS_VISTA)

// global

NTSTATUS PhInitializePhLib();

// basesup

struct _PH_OBJECT_TYPE;
typedef struct _PH_OBJECT_TYPE *PPH_OBJECT_TYPE;

BOOLEAN PhInitializeBase();

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
#endif

#ifndef BASESUP_PRIVATE
#ifdef DEBUG
extern ULONG PhDbgThreadDbgTlsIndex;
extern LIST_ENTRY PhDbgThreadListHead;
extern PH_FAST_LOCK PhDbgThreadListLock;
#endif
#endif

HANDLE PhCreateThread(
    __in_opt SIZE_T StackSize,
    __in PUSER_THREAD_START_ROUTINE StartAddress,
    __in PVOID Parameter
    );

// heap

__mayRaise PVOID PhAllocate(
    __in SIZE_T Size
    );

VOID PhFree(
    __in PVOID Memory
    );

PVOID PhReAlloc(
    __in PVOID Memory,
    __in SIZE_T Size
    );

// mutex

#define PH_MUTEX_IS_CRITICAL_SECTION

#ifdef PH_MUTEX_IS_CRITICAL_SECTION
typedef RTL_CRITICAL_SECTION PH_MUTEX, *PPH_MUTEX;
#else
typedef PH_FAST_LOCK PH_MUTEX, *PPH_MUTEX;
#endif

/**
 * Initializes a mutex object.
 *
 * \param Mutex A pointer to a mutex object.
 */
FORCEINLINE VOID PhInitializeMutex(
    __out PPH_MUTEX Mutex
    )
{
#ifdef PH_MUTEX_IS_CRITICAL_SECTION
    RtlInitializeCriticalSection(Mutex);
#else
    PhInitializeFastLock(Mutex);
#endif
}

/**
 * Frees resources used by a mutex object.
 *
 * \param Mutex A pointer to a mutex object.
 */
FORCEINLINE VOID PhDeleteMutex(
    __inout PPH_MUTEX Mutex
    )
{
#ifdef PH_MUTEX_IS_CRITICAL_SECTION
    RtlDeleteCriticalSection(Mutex);
#else
    PhDeleteFastLock(Mutex);
#endif
}

/**
 * Acquires a mutex.
 *
 * \param Mutex A pointer to a mutex object.
 */
FORCEINLINE VOID PhAcquireMutex(
    __inout PPH_MUTEX Mutex
    )
{
#ifdef PH_MUTEX_IS_CRITICAL_SECTION
    RtlEnterCriticalSection(Mutex);
#else
    PhAcquireFastLockExclusive(Mutex);
#endif
}

/**
 * Releases a mutex.
 *
 * \param Mutex A pointer to a mutex object.
 */
FORCEINLINE VOID PhReleaseMutex(
    __inout PPH_MUTEX Mutex
    )
{
#ifdef PH_MUTEX_IS_CRITICAL_SECTION
    RtlLeaveCriticalSection(Mutex);
#else
    PhReleaseFastLockExclusive(Mutex);
#endif
}

// event

#define PH_EVENT_SET 0x1
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
    ULONG Value;
    HANDLE EventHandle;
} PH_EVENT, *PPH_EVENT;

VOID PhInitializeEvent(
    __out PPH_EVENT Event
    );

VOID PhSetEvent(
    __inout PPH_EVENT Event
    );

BOOLEAN PhWaitForEvent(
    __inout PPH_EVENT Event,
    __in ULONG Timeout
    );

VOID PhResetEvent(
    __inout PPH_EVENT Event
    );

/**
 * Determines whether an event object is set.
 *
 * \param Event A pointer to an event object.
 *
 * \return TRUE if the event object is set, 
 * otherwise FALSE.
 */
FORCEINLINE BOOLEAN PhTestEvent(
    __in PPH_EVENT Event
    )
{
    return !!(Event->Value & PH_EVENT_SET);
}

// rundown protection

#define PH_RUNDOWN_ACTIVE 0x1
#define PH_RUNDOWN_REF_SHIFT 1
#define PH_RUNDOWN_REF_INC 0x2

typedef struct _PH_RUNDOWN_PROTECT
{
    ULONG_PTR Value;
} PH_RUNDOWN_PROTECT, *PPH_RUNDOWN_PROTECT;

typedef struct _PH_RUNDOWN_WAIT_BLOCK
{
    ULONG_PTR Count;
    PH_EVENT WakeEvent;
} PH_RUNDOWN_WAIT_BLOCK, *PPH_RUNDOWN_WAIT_BLOCK;

VOID PhInitializeRundownProtection(
    __out PPH_RUNDOWN_PROTECT Protection
    );

BOOLEAN PhAcquireRundownProtection(
    __inout PPH_RUNDOWN_PROTECT Protection
    );

VOID PhReleaseRundownProtection(
    __inout PPH_RUNDOWN_PROTECT Protection
    );

VOID PhWaitForRundownProtection(
    __inout PPH_RUNDOWN_PROTECT Protection
    );

// string

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhStringType;
#endif

BOOLEAN PhCopyAnsiStringZ(
    __in PSTR InputBuffer,
    __in ULONG InputCount,
    __out_ecount_z_opt(OutputCount) PSTR OutputBuffer,
    __in ULONG OutputCount,
    __out_opt PULONG ReturnCount
    );

BOOLEAN PhCopyUnicodeStringZ(
    __in PWSTR InputBuffer,
    __in ULONG InputCount,
    __out_ecount_z_opt(OutputCount) PWSTR OutputBuffer,
    __in ULONG OutputCount,
    __out_opt PULONG ReturnCount
    );

BOOLEAN PhCopyUnicodeStringZFromAnsi(
    __in PSTR InputBuffer,
    __in ULONG InputCount,
    __out_ecount_z_opt(OutputCount) PWSTR OutputBuffer,
    __in ULONG OutputCount,
    __out_opt PULONG ReturnCount
    );

#define PH_STRING_MAXLEN MAXUINT16

typedef struct _PH_STRINGREF
{
    union
    {
        struct
        {
            USHORT Length;
            USHORT Reserved;
            PWSTR Buffer;
        };
        UNICODE_STRING us;
    };
} PH_STRINGREF, *PPH_STRINGREF;

FORCEINLINE VOID PhInitializeStringRef(
    __out PPH_STRINGREF String,
    __in PWSTR Buffer
    )
{
    String->Length = (USHORT)(wcslen(Buffer) * sizeof(WCHAR));
    String->Buffer = Buffer;
}

FORCEINLINE PH_STRINGREF PhCreateStringRef(
    __in PWSTR Buffer
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, Buffer);

    return string;
}

FORCEINLINE INT PhStringRefCompare2(
    __in PPH_STRINGREF String1,
    __in PWSTR String2,
    __in BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return wcsncmp(String1->Buffer, String2, String1->Length / sizeof(WCHAR));
    else
        return wcsnicmp(String1->Buffer, String2, String1->Length / sizeof(WCHAR));
}

FORCEINLINE BOOLEAN PhStringRefEquals2(
    __in PPH_STRINGREF String1,
    __in PWSTR String2,
    __in BOOLEAN IgnoreCase
    )
{
    return PhStringRefCompare2(String1, String2, IgnoreCase) == 0;
}

/**
 * A Unicode string object.
 */
typedef struct _PH_STRING
{
    union
    {
        /** An embedded UNICODE_STRING structure. */
        UNICODE_STRING us;
        PH_STRINGREF sr;
        struct
        {
            /** The length, in bytes, of the string. */
            USHORT Length;
            /** Unused and always equal to \ref Length. */
            USHORT MaximumLength;
            /** Unused and always a pointer to \ref Buffer. */
            PWSTR Pointer;
        };
    };
    /** The buffer containing the contents of the string. */
    WCHAR Buffer[1];
} PH_STRING, *PPH_STRING;

PPH_STRING PhCreateString(
    __in PWSTR Buffer
    );

PPH_STRING PhCreateStringEx(
    __in_opt PWSTR Buffer,
    __in SIZE_T Length
    );

PPH_STRING PhCreateStringFromAnsi(
    __in PCHAR Buffer
    );

PPH_STRING PhCreateStringFromAnsiEx(
    __in PCHAR Buffer,
    __in SIZE_T Length
    );

PPH_STRING PhConcatStrings(
    __in ULONG Count,
    ...
    );

#define PH_CONCAT_STRINGS_LENGTH_CACHE_SIZE 16

PPH_STRING PhConcatStrings_V(
    __in ULONG Count,
    __in va_list ArgPtr
    );

PPH_STRING PhConcatStrings2(
    __in PWSTR String1,
    __in PWSTR String2
    );

PPH_STRING PhFormatString(
    __in __format_string PWSTR Format,
    ...
    );

PPH_STRING PhFormatString_V(
    __in __format_string PWSTR Format,
    __in va_list ArgPtr
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
    __in_opt PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return NULL;
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
    __in_opt PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return L"";
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
    __in_opt PPH_STRING String,
    __in PWSTR DefaultString
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
FORCEINLINE BOOLEAN PhIsStringNullOrEmpty(
    __in_opt PPH_STRING String
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
    __in PPH_STRING String
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
FORCEINLINE INT PhStringCompare(
    __in PPH_STRING String1,
    __in PPH_STRING String2,
    __in BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return wcscmp(String1->Buffer, String2->Buffer);
    else
        return wcsicmp(String1->Buffer, String2->Buffer);
}

/**
 * Compares two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
FORCEINLINE INT PhStringCompare2(
    __in PPH_STRING String1,
    __in PWSTR String2,
    __in BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return wcscmp(String1->Buffer, String2);
    else
        return wcsicmp(String1->Buffer, String2);
}

/**
 * Determines whether two strings are equal.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
FORCEINLINE BOOLEAN PhStringEquals(
    __in PPH_STRING String1,
    __in PPH_STRING String2,
    __in BOOLEAN IgnoreCase
    )
{
    return PhStringCompare(String1, String2, IgnoreCase) == 0;
}

/**
 * Determines whether two strings are equal.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
FORCEINLINE BOOLEAN PhStringEquals2(
    __in PPH_STRING String1,
    __in PWSTR String2,
    __in BOOLEAN IgnoreCase
    )
{
    return PhStringCompare2(String1, String2, IgnoreCase) == 0;
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
FORCEINLINE BOOLEAN PhStringStartsWith(
    __in PPH_STRING String1,
    __in PPH_STRING String2,
    __in BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return wcsncmp(String1->Buffer, String2->Buffer, String2->Length / sizeof(WCHAR)) == 0;
    else
        return wcsnicmp(String1->Buffer, String2->Buffer, String2->Length / sizeof(WCHAR)) == 0;
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
FORCEINLINE BOOLEAN PhStringStartsWith2(
    __in PPH_STRING String1,
    __in PWSTR String2,
    __in BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return wcsncmp(String1->Buffer, String2, wcslen(String2)) == 0;
    else
        return wcsnicmp(String1->Buffer, String2, wcslen(String2)) == 0;
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
FORCEINLINE BOOLEAN PhStringEndsWith(
    __in PPH_STRING String1,
    __in PPH_STRING String2,
    __in BOOLEAN IgnoreCase
    )
{
    if (String2->Length > String1->Length)
        return FALSE;

    if (!IgnoreCase)
    {
        return wcsncmp(
            &String1->Buffer[(String1->Length - String2->Length) / sizeof(WCHAR)],
            String2->Buffer,
            String2->Length / sizeof(WCHAR)
            ) == 0;
    }
    else
    {
        return wcsnicmp(
            &String1->Buffer[(String1->Length - String2->Length) / sizeof(WCHAR)],
            String2->Buffer,
            String2->Length / sizeof(WCHAR)
            ) == 0;
    }
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
FORCEINLINE BOOLEAN PhStringEndsWith2(
    __in PPH_STRING String1,
    __in PWSTR String2,
    __in BOOLEAN IgnoreCase
    )
{
    SIZE_T length = wcslen(String2);

    if (length * sizeof(WCHAR) > String1->Length)
        return FALSE;

    if (!IgnoreCase)
    {
        return wcsncmp(
            &String1->Buffer[String1->Length / sizeof(WCHAR) - length],
            String2,
            length
            ) == 0;
    }
    else
    {
        return wcsnicmp(
            &String1->Buffer[String1->Length / sizeof(WCHAR) - length],
            String2,
            length
            ) == 0;
    }
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
FORCEINLINE ULONG PhStringIndexOfChar(
    __in PPH_STRING String,
    __in ULONG StartIndex,
    __in WCHAR Char
    )
{
    PWSTR location;

    location = wcschr(&String->Buffer[StartIndex], Char);

    if (location)
        return (ULONG)(location - String->Buffer);
    else
        return -1;
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
FORCEINLINE ULONG PhStringIndexOfString(
    __in PPH_STRING String1,
    __in ULONG StartIndex,
    __in PWSTR String2
    )
{
    PWSTR location;

    location = wcsstr(&String1->Buffer[StartIndex], String2);

    if (location)
        return (ULONG)(location - String1->Buffer);
    else
        return -1;
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
FORCEINLINE ULONG PhStringLastIndexOfChar(
    __in PPH_STRING String,
    __in ULONG StartIndex,
    __in WCHAR Char
    )
{
    PWSTR location;

    location = wcsrchr(&String->Buffer[StartIndex], Char);

    if (location)
        return (ULONG)(location - String->Buffer);
    else
        return -1;
}

/**
 * Converts a string to lowercase in-place.
 *
 * \param String The string to convert.
 */
FORCEINLINE VOID PhLowerString(
    __inout PPH_STRING String
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
    __inout PPH_STRING String
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
    __in PPH_STRING String,
    __in ULONG StartIndex,
    __in ULONG Count
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
FORCEINLINE VOID PhTrimStringToNullTerminator(
    __inout PPH_STRING String
    )
{
    String->Length = String->MaximumLength = (USHORT)(wcslen(String->Buffer) * sizeof(WCHAR));
}

// ansi string

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhAnsiStringType;
#endif

/**
 * An ANSI string structure.
 */
typedef struct _PH_ANSI_STRING
{
    union
    {
        /** An embedded ANSI_STRING structure. */
        ANSI_STRING as;
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

PPH_ANSI_STRING PhCreateAnsiString(
    __in PSTR Buffer
    );

PPH_ANSI_STRING PhCreateAnsiStringEx(
    __in_opt PSTR Buffer,
    __in SIZE_T Length
    );

PPH_ANSI_STRING PhCreateAnsiStringFromUnicode(
    __in PWSTR Buffer
    );

PPH_ANSI_STRING PhCreateAnsiStringFromUnicodeEx(
    __in PWSTR Buffer,
    __in SIZE_T Length
    );

// stringbuilder

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhStringBuilderType;
#endif

/**
 * A string builder structure.
 * The string builder object allows you to easily
 * construct complex strings without allocating 
 * a great number of strings in the process.
 */
typedef struct _PH_STRING_BUILDER
{
    /** Allocated length of the string. */
    ULONG AllocatedLength;
    /**
     * The constructed string.
     * \a String will be allocated for \a AllocatedLength, 
     * but we will modify the \a Length field to be the 
     * correct length.
     */
    PPH_STRING String;
} PH_STRING_BUILDER, *PPH_STRING_BUILDER;

PPH_STRING_BUILDER PhCreateStringBuilder(
    __in ULONG InitialCapacity
    );

PPH_STRING PhReferenceStringBuilderString(
    __in PPH_STRING_BUILDER StringBuilder
    );

VOID PhStringBuilderAppend(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in PPH_STRING String
    );

VOID PhStringBuilderAppend2(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in PWSTR String
    );

VOID PhStringBuilderAppendEx(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in PWSTR String,
    __in ULONG Length
    );

VOID PhStringBuilderAppendChar(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in WCHAR Character
    );

VOID PhStringBuilderAppendFormat(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in __format_string PWSTR Format,
    ...
    );

VOID PhStringBuilderInsert(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG Index,
    __in PPH_STRING String
    );

VOID PhStringBuilderInsert2(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG Index,
    __in PWSTR String
    );

VOID PhStringBuilderInsertEx(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG Index,
    __in PWSTR String,
    __in ULONG Length
    );

VOID PhStringBuilderRemove(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG StartIndex,
    __in ULONG Count
    );

// list

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhListType;
#endif

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
    PPVOID Items;
} PH_LIST, *PPH_LIST;

PPH_LIST PhCreateList(
    __in ULONG InitialCapacity
    );

VOID PhAddListItem(
    __in PPH_LIST List,
    __in PVOID Item
    );

VOID PhClearList(
    __inout PPH_LIST List
    );

ULONG PhIndexOfListItem(
    __in PPH_LIST List,
    __in PVOID Item
    );

VOID PhRemoveListItem(
    __in PPH_LIST List,
    __in ULONG Index
    );

VOID PhRemoveListItems(
    __in PPH_LIST List,
    __in ULONG StartIndex,
    __in ULONG Count
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
typedef INT (NTAPI *PPH_COMPARE_FUNCTION)(
    __in PVOID Item1,
    __in PVOID Item2,
    __in PVOID Context
    );

VOID PhSortList(
    __in PPH_LIST List,
    __in PPH_COMPARE_FUNCTION CompareFunction,
    __in PVOID Context
    );

// pointer list

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhPointerListType;
#endif

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
    PPVOID Items;
} PH_POINTER_LIST, *PPH_POINTER_LIST;

#define PH_IS_LIST_POINTER_VALID(Pointer) (!((ULONG_PTR)(Pointer) & 0x1))

PPH_POINTER_LIST PhCreatePointerList(
    __in ULONG InitialCapacity
    );

HANDLE PhAddPointerListItem(
    __inout PPH_POINTER_LIST PointerList,
    __in PVOID Pointer
    );

HANDLE PhFindPointerListItem(
    __in PPH_POINTER_LIST PointerList,
    __in PVOID Pointer
    );

VOID PhRemovePointerListItem(
    __inout PPH_POINTER_LIST PointerList,
    __in HANDLE PointerHandle
    );

FORCEINLINE BOOLEAN PhEnumPointerList(
    __in PPH_POINTER_LIST PointerList,
    __inout PULONG EnumerationKey,
    __out PPVOID Pointer
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

// queue

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhQueueType;
#endif

/**
 * A queue structure.
 * Storage is automatically allocated for new 
 * elements.
 */
typedef struct _PH_QUEUE
{
    /** The number of items in the queue. */
    ULONG Count;
    /** The number of items for which storage is allocated. */
    ULONG AllocatedCount;
    /** The array of queue items. */
    PPVOID Items;
    /** The index of the first slot in the queue. */
    ULONG Head;
    /** The index of the last available slot in the queue. */
    ULONG Tail;
} PH_QUEUE, *PPH_QUEUE;

PPH_QUEUE PhCreateQueue(
    __in ULONG InitialCapacity
    );

VOID PhEnqueueQueueItem(
    __inout PPH_QUEUE Queue,
    __in PVOID Item
    );

BOOLEAN PhDequeueQueueItem(
    __inout PPH_QUEUE Queue,
    __out PPVOID Item
    );

BOOLEAN PhPeekQueueItem(
    __in PPH_QUEUE Queue,
    __out PPVOID Item
    );

// hashtable

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhHashtableType;
#endif

ULONG PhGetPrimeNumber(
    __in ULONG Minimum
    );

ULONG PhRoundUpToPowerOfTwo(
    __in ULONG Number
    );

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
    __in PVOID Entry1,
    __in PVOID Entry2
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
    __in PVOID Entry
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

#define PH_HASHTABLE_ENTRY_SIZE(InnerSize) (sizeof(PH_HASHTABLE_ENTRY) + (InnerSize))
#define PH_HASHTABLE_GET_ENTRY(Hashtable, Index) \
    ((PPH_HASHTABLE_ENTRY)PTR_ADD_OFFSET((Hashtable)->Entries, \
    PH_HASHTABLE_ENTRY_SIZE((Hashtable)->EntrySize) * (Index)))
#define PH_HASHTABLE_GET_ENTRY_INDEX(Hashtable, Entry) \
    ((ULONG)(PTR_ADD_OFFSET(Entry, -(Hashtable)->Entries) / \
    PH_HASHTABLE_ENTRY_SIZE((Hashtable)->EntrySize)))

PPH_HASHTABLE PhCreateHashtable(
    __in ULONG EntrySize,
    __in PPH_HASHTABLE_COMPARE_FUNCTION CompareFunction,
    __in PPH_HASHTABLE_HASH_FUNCTION HashFunction,
    __in ULONG InitialCapacity
    );

PVOID PhAddHashtableEntry(
    __inout PPH_HASHTABLE Hashtable,
    __in PVOID Entry
    );

VOID PhClearHashtable(
    __inout PPH_HASHTABLE Hashtable
    );

BOOLEAN PhEnumHashtable(
    __in PPH_HASHTABLE Hashtable,
    __out PPVOID Entry,
    __inout PULONG EnumerationKey
    );

PVOID PhGetHashtableEntry(
    __in PPH_HASHTABLE Hashtable,
    __in PVOID Entry
    );

BOOLEAN PhRemoveHashtableEntry(
    __inout PPH_HASHTABLE Hashtable,
    __in PVOID Entry
    );

#define PhHashBytes PhHashBytesSdbm

#define PhHashBytesHsieh PhfHashBytesHsieh
ULONG FASTCALL PhfHashBytesHsieh(
    __in PUCHAR Bytes,
    __in ULONG Length
    );

#define PhHashBytesMurmur PhfHashBytesMurmur
ULONG FASTCALL PhfHashBytesMurmur(
    __in PUCHAR Bytes,
    __in ULONG Length
    );

#define PhHashBytesSdbm PhfHashBytesSdbm
ULONG FASTCALL PhfHashBytesSdbm(
    __in PUCHAR Bytes,
    __in ULONG Length
    );

FORCEINLINE ULONG PhHashInt32(
    __in ULONG Value
    )
{
    // Java style.
    Value ^= (Value >> 20) ^ (Value >> 12);
    return Value ^ (Value >> 7) ^ (Value >> 4);
}

FORCEINLINE ULONG PhHashInt64(
    __in ULONG64 Value
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

PPH_HASHTABLE PhCreateSimpleHashtable(
    __in ULONG InitialCapacity
    );

PVOID PhAddSimpleHashtableItem(
    __inout PPH_HASHTABLE SimpleHashtable,
    __in PVOID Key,
    __in PVOID Value
    );

PPVOID PhGetSimpleHashtableItem(
    __in PPH_HASHTABLE SimpleHashtable,
    __in PVOID Key
    );

BOOLEAN PhRemoveSimpleHashtableItem(
    __inout PPH_HASHTABLE SimpleHashtable,
    __in PVOID Key
    );

// free list

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhFreeListType;
#endif

typedef struct _PH_FREE_LIST
{
    ULONG Count;
    PH_QUEUED_LOCK Lock;

    ULONG MaximumCount;
    SIZE_T Size;
    PVOID List[1];
} PH_FREE_LIST, *PPH_FREE_LIST;

PPH_FREE_LIST PhCreateFreeList(
    __in SIZE_T Size,
    __in ULONG MaximumCount
    );

PVOID PhAllocateFromFreeList(
    __inout PPH_FREE_LIST FreeList
    );

VOID PhFreeToFreeList(
    __inout PPH_FREE_LIST FreeList,
    __in PVOID Memory
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
    __in PVOID Parameter,
    __in PVOID Context
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

VOID PhInitializeCallback(
    __out PPH_CALLBACK Callback
    );

VOID PhDeleteCallback(
    __inout PPH_CALLBACK Callback
    );

VOID PhRegisterCallback(
    __inout PPH_CALLBACK Callback,
    __in PPH_CALLBACK_FUNCTION Function,
    __in PVOID Context,
    __out PPH_CALLBACK_REGISTRATION Registration
    );

VOID PhRegisterCallbackEx(
    __inout PPH_CALLBACK Callback,
    __in PPH_CALLBACK_FUNCTION Function,
    __in PVOID Context,
    __in USHORT Flags,
    __out PPH_CALLBACK_REGISTRATION Registration
    );

VOID PhUnregisterCallback(
    __inout PPH_CALLBACK Callback,
    __inout PPH_CALLBACK_REGISTRATION Registration
    );

VOID PhInvokeCallback(
    __in PPH_CALLBACK Callback,
    __in PVOID Parameter
    );

// callbacksync

typedef struct _PH_CALLBACK_SYNC
{
    PH_CALLBACK Callback;
    ULONG Target;
    PVOID Parameter;
    ULONG Value;
} PH_CALLBACK_SYNC, *PPH_CALLBACK_SYNC;

FORCEINLINE VOID PhInitializeCallbackSync(
    __out PPH_CALLBACK_SYNC CallbackSync,
    __in ULONG Target,
    __in PVOID Parameter
    )
{
    PhInitializeCallback(&CallbackSync->Callback);
    CallbackSync->Target = Target;
    CallbackSync->Parameter = Parameter;
    CallbackSync->Value = 0;
}

FORCEINLINE VOID PhIncrementCallbackSync(
    __in PPH_CALLBACK_SYNC CallbackSync
    )
{
    if ((ULONG)_InterlockedIncrement(&CallbackSync->Value) == CallbackSync->Target)
        PhInvokeCallback(&CallbackSync->Callback, CallbackSync->Parameter); 
}

FORCEINLINE VOID PhIncrementMultipleCallbackSync(
    __in PPH_CALLBACK_SYNC CallbackSync
    )
{
    if ((ULONG)_InterlockedIncrement(&CallbackSync->Value) >= CallbackSync->Target)
        PhInvokeCallback(&CallbackSync->Callback, CallbackSync->Parameter); 
}

// general

#define PH_TIMESPAN_STR_LEN 30
#define PH_TIMESPAN_STR_LEN_1 (PH_TIMESPAN_STR_LEN + 1)

#define PH_TIMESPAN_HMS 0
#define PH_TIMESPAN_HMSM 1
#define PH_TIMESPAN_DHMS 2

VOID PhPrintTimeSpan(
    __out_ecount(PH_TIMESPAN_STR_LEN_1) PWSTR Destination,
    __in ULONG64 Ticks,
    __in_opt ULONG Mode
    );

ULONG PhExponentiate(
    __in ULONG Base,
    __in ULONG Exponent
    );

ULONG64 PhExponentiate64(
    __in ULONG64 Base,
    __in ULONG Exponent
    );

BOOLEAN PhStringToInteger64(
    __in PWSTR String,
    __in_opt ULONG Base,
    __out PLONG64 Integer
    );

// basesupa

PPH_STRING PhaCreateString(
    __in PWSTR Buffer
    );

PPH_STRING PhaCreateStringEx(
    __in PWSTR Buffer,
    __in SIZE_T Length
    );

PPH_STRING PhaDuplicateString(
    __in PPH_STRING String
    );

PPH_STRING PhaConcatStrings(
    __in ULONG Count,
    ...
    );

PPH_STRING PhaConcatStrings2(
    __in PWSTR String1,
    __in PWSTR String2
    );

PPH_STRING PhaFormatString(
    __in __format_string PWSTR Format,
    ...
    );

PPH_STRING PhaLowerString(
    __in PPH_STRING String
    );

PPH_STRING PhaUpperString(
    __in PPH_STRING String
    );

PPH_STRING PhaSubstring(
    __in PPH_STRING String,
    __in ULONG StartIndex,
    __in ULONG Count
    );

// workqueue

typedef struct _PH_WORK_QUEUE
{
    PH_RUNDOWN_PROTECT RundownProtect;
    BOOLEAN Terminating;

    PPH_QUEUE Queue;
    PH_QUEUED_LOCK QueueLock;

    ULONG MaximumThreads;
    ULONG MinimumThreads;
    ULONG NoWorkTimeout;

    PH_QUEUED_LOCK StateLock;
    HANDLE SemaphoreHandle;
    ULONG CurrentThreads;
    ULONG BusyThreads;
} PH_WORK_QUEUE, *PPH_WORK_QUEUE;

VOID PhWorkQueueInitialization();

VOID PhInitializeWorkQueue(
    __out PPH_WORK_QUEUE WorkQueue,
    __in ULONG MinimumThreads,
    __in ULONG MaximumThreads,
    __in ULONG NoWorkTimeout
    );

VOID PhDeleteWorkQueue(
    __inout PPH_WORK_QUEUE WorkQueue
    );

VOID PhQueueWorkQueueItem(
    __inout PPH_WORK_QUEUE WorkQueue,
    __in PTHREAD_START_ROUTINE Function,
    __in PVOID Context
    );

VOID PhQueueGlobalWorkQueueItem(
    __in PTHREAD_START_ROUTINE Function,
    __in PVOID Context
    );

#endif