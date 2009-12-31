#ifndef PHBASE_H
#define PHBASE_H

#include <ntimport.h>
#include <ref.h>
#include <fastlock.h>

#define PH_APP_NAME (L"Process Hacker")

#ifndef MAIN_PRIVATE

struct _PH_STRING;
typedef struct _PH_STRING *PPH_STRING;

extern PPH_STRING PhApplicationDirectory;
extern PPH_STRING PhApplicationFileName;
extern HFONT PhApplicationFont;
extern HANDLE PhHeapHandle;
extern HINSTANCE PhInstanceHandle;
extern HANDLE PhKphHandle;
extern PWSTR PhWindowClassName;
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

// basesup

struct _PH_OBJECT_TYPE;
typedef struct _PH_OBJECT_TYPE *PPH_OBJECT_TYPE;

BOOLEAN PhInitializeBase();

// heap

PVOID PhAllocate(
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

typedef RTL_CRITICAL_SECTION PH_MUTEX, *PPH_MUTEX;

FORCEINLINE VOID PhInitializeMutex(
    __out PPH_MUTEX Mutex
    )
{
    InitializeCriticalSection(Mutex);
}

FORCEINLINE VOID PhDeleteMutex(
    __inout PPH_MUTEX Mutex
    )
{
    DeleteCriticalSection(Mutex);
}

FORCEINLINE VOID PhAcquireMutex(
    __inout PPH_MUTEX Mutex
    )
{
    EnterCriticalSection(Mutex);
}

FORCEINLINE VOID PhReleaseMutex(
    __inout PPH_MUTEX Mutex
    )
{
    LeaveCriticalSection(Mutex);
}

// event

#define PH_EVENT_SET 0x1
#define PH_EVENT_REFCOUNT_SHIFT 1
#define PH_EVENT_REFCOUNT_INC 0x2

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

FORCEINLINE BOOLEAN PhTestEvent(
    __in PPH_EVENT Event
    )
{
    return !!(Event->Value & PH_EVENT_SET);
}

// string

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhStringType;
#endif

#define PH_STRING_MAXLEN MAXUINT16

typedef struct _PH_STRING
{
    union
    {
        UNICODE_STRING us;
        struct
        {
            USHORT Length;
            USHORT MaximumLength;
            PWSTR Pointer;
        };
    };
    WCHAR Buffer[1];
} PH_STRING, *PPH_STRING;

PPH_STRING PhCreateString(
    __in PWSTR Buffer
    );

PPH_STRING PhCreateStringEx(
    __in_opt PWSTR Buffer,
    __in SIZE_T Length
    );

PPH_STRING PhCreateStringFromMultiByte(
    __in PCHAR Buffer
    );

PPH_STRING PhCreateStringFromMultiByteEx(
    __in PCHAR Buffer,
    __in SIZE_T Length
    );

PPH_STRING PhConcatStrings(
    __in ULONG Count,
    ...
    );

FORCEINLINE PWSTR PhGetString(
    __in_opt PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return NULL;
}

FORCEINLINE PPH_STRING PhConcatStrings2(
    __in PWSTR String1,
    __in PWSTR String2
    )
{
    PPH_STRING string;
    SIZE_T length1;
    SIZE_T length2;

    length1 = wcslen(String1) * sizeof(WCHAR);
    length2 = wcslen(String2) * sizeof(WCHAR);
    string = PhCreateStringEx(NULL, length1 + length2);
    memcpy(
        string->Buffer,
        String1,
        length1
        );
    memcpy(
        &string->Buffer[length1 / sizeof(WCHAR)],
        String2,
        length2
        );

    return string;
}

FORCEINLINE BOOLEAN PhStringEquals(
    __in PPH_STRING String1,
    __in PPH_STRING String2,
    __in BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return wcscmp(String1->Buffer, String2->Buffer) == 0;
    else
        return wcsicmp(String1->Buffer, String2->Buffer) == 0;
}

FORCEINLINE BOOLEAN PhStringEquals2(
    __in PPH_STRING String1,
    __in PWSTR String2,
    __in BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return wcscmp(String1->Buffer, String2) == 0;
    else
        return wcsicmp(String1->Buffer, String2) == 0;
}

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

FORCEINLINE ULONG PhStringIndexOfChar(
    __in PPH_STRING String1,
    __in ULONG StartIndex,
    __in WCHAR Char
    )
{
    PWSTR location;

    location = wcschr(&String1->Buffer[StartIndex], Char);

    if (location)
        return (ULONG)(location - String1->Buffer);
    else
        return -1;
}

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

FORCEINLINE PPH_STRING PhSubstring(
    __in PPH_STRING String,
    __in ULONG StartIndex,
    __in ULONG Count
    )
{
    return PhCreateStringEx(&String->Buffer[StartIndex], Count * sizeof(WCHAR));
}

// stringbuilder

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhStringBuilderType;
#endif

typedef struct _PH_STRING_BUILDER
{
    // Allocated length of the string.
    ULONG AllocatedLength;
    // String will be allocated for AllocatedLength, 
    // but we will modify the Length field to be the 
    // correct length.
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
    __in ULONG Length
    );

// list

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhListType;
#endif

typedef struct _PH_LIST
{
    ULONG Count;
    ULONG AllocatedCount;
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

// queue

#ifndef BASESUP_PRIVATE
extern PPH_OBJECT_TYPE PhQueueType;
#endif

typedef struct _PH_QUEUE
{
    ULONG Count;
    ULONG AllocatedCount;
    PPVOID Items;
    ULONG Head;
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

typedef struct _PH_HASHTABLE_ENTRY
{
    /* Hash code of the entry. -1 if entry is unused. */
    ULONG HashCode;
    /* Either the index of the next entry in the bucket, 
     * the index of the next free entry, or -1 for invalid.
     */
    ULONG Next;
    QUAD Body;
} PH_HASHTABLE_ENTRY, *PPH_HASHTABLE_ENTRY;

typedef BOOLEAN (NTAPI *PPH_HASHTABLE_COMPARE_FUNCTION)(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

typedef ULONG (NTAPI *PPH_HASHTABLE_HASH_FUNCTION)(
    __in PVOID Entry
    );

// Enables collection of statistics
#define PH_HASHTABLE_ENABLE_STATS
// Enables 2^32-1 possible hash codes instead of only 2^31
#define PH_HASHTABLE_ENABLE_FULL_HASH

typedef struct _PH_HASHTABLE
{
    /* Size of user data in each entry. */
    ULONG EntrySize;
    PPH_HASHTABLE_COMPARE_FUNCTION CompareFunction;
    PPH_HASHTABLE_HASH_FUNCTION HashFunction;

    ULONG AllocatedBuckets;
    PULONG Buckets;
    ULONG AllocatedEntries;
    PVOID Entries;

    /* Number of entries in the hashtable. */
    ULONG Count;
    /* Index into entry array for free list. */
    ULONG FreeEntry;
    /* Index of next usable index into entry array, a.k.a the 
     * count of entries that were ever allocated.
     */
    ULONG NextEntry;

#ifdef PH_HASHTABLE_ENABLE_STATS
    ULONG TotalCollisions;
#endif
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

ULONG PhHashBytes(
    __in PUCHAR Bytes,
    __in ULONG Length
    );

ULONG PhHashBytesSdbm(
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

// callback

typedef VOID (NTAPI *PPH_CALLBACK_FUNCTION)(
    __in PVOID Parameter,
    __in PVOID Context
    );

typedef struct _PH_CALLBACK_REGISTRATION
{
    LIST_ENTRY ListEntry;
    PPH_CALLBACK_FUNCTION Function;
    PVOID Context;
    BOOLEAN Unregistering;
} PH_CALLBACK_REGISTRATION, *PPH_CALLBACK_REGISTRATION;

typedef struct _PH_CALLBACK
{
    LIST_ENTRY ListHead;
    PH_FAST_LOCK ListLock;
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

VOID PhUnregisterCallback(
    __inout PPH_CALLBACK Callback,
    __inout PPH_CALLBACK_REGISTRATION Registration
    );

VOID PhInvokeCallback(
    __in PPH_CALLBACK Callback,
    __in PVOID Parameter
    );

FORCEINLINE VOID PhInvokeCallbackRegistration(
    __in PPH_CALLBACK_REGISTRATION Registration,
    __in PVOID Parameter
    )
{
    Registration->Function(
        Parameter,
        Registration->Context
        );
}

// workqueue

typedef struct _PH_WORK_QUEUE
{
    PPH_QUEUE Queue;
    PH_MUTEX QueueLock;

    ULONG MaximumThreads;
    ULONG MinimumThreads;
    ULONG NoWorkTimeout;

    PH_MUTEX StateLock;
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