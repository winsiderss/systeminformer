#ifndef PHBASE_H
#define PHBASE_H

#ifndef UNICODE
#define UNICODE
#endif

#include <ntwin.h>
#include <ntimport.h>
#include <ref.h>

// We don't care about "deprecation".
#pragma warning(disable: 4996)

#define PH_APP_NAME (L"Process Hacker")

#define PH_INT_STR_LEN 10
#define PH_INT_STR_LEN_1 (PH_INT_STR_LEN + 1)

#ifndef MAIN_PRIVATE

extern HFONT PhApplicationFont;
extern HANDLE PhHeapHandle;
extern HINSTANCE PhInstanceHandle;
extern PWSTR PhWindowClassName;

#endif

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))

// basesup

struct _PH_OBJECT_TYPE;
typedef struct _PH_OBJECT_TYPE *PPH_OBJECT_TYPE;

#define PhRaiseStatus(Status) RaiseException(Status, 0, 0, NULL)

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

VOID FORCEINLINE PhInitializeMutex(
    __out PPH_MUTEX Mutex
    )
{
    InitializeCriticalSection(Mutex);
}

VOID FORCEINLINE PhAcquireMutex(
    __inout PPH_MUTEX Mutex
    )
{
    EnterCriticalSection(Mutex);
}

VOID FORCEINLINE PhReleaseMutex(
    __inout PPH_MUTEX Mutex
    )
{
    LeaveCriticalSection(Mutex);
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

PWSTR FORCEINLINE PhGetString(
    __in_opt PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return NULL;
}

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

typedef VOID (NTAPI *PPH_HASHTABLE_DELETE_FUNCTION)(
    __in PVOID Entry
    );

typedef struct _PH_HASHTABLE
{
    /* Size of user data in each entry. */
    ULONG EntrySize;
    PPH_HASHTABLE_COMPARE_FUNCTION CompareFunction;
    PPH_HASHTABLE_HASH_FUNCTION HashFunction;
    PPH_HASHTABLE_DELETE_FUNCTION DeleteFunction;

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
} PH_HASHTABLE, *PPH_HASHTABLE;

#define PH_HASHTABLE_ENTRY_SIZE(InnerSize) (sizeof(PH_HASHTABLE_ENTRY) + (InnerSize))
#define PH_HASHTABLE_GET_ENTRY(Hashtable, Index) \
    ((PPH_HASHTABLE_ENTRY)PTR_ADD_OFFSET((Hashtable)->Entries, \
    PH_HASHTABLE_ENTRY_SIZE((Hashtable)->EntrySize) * (Index)))
#define PH_HASHTABLE_GET_ENTRY_INDEX(Hashtable, Entry) \
    ((ULONG)(PTR_ADD_OFFSET(Entry, -(Hashtable)->Entries) / \
    PH_HASHTABLE_ENTRY_SIZE((Hashtable)->EntrySize)))

PVOID PhAddHashtableEntry(
    __inout PPH_HASHTABLE Hashtable,
    __in PVOID Entry
    );

PVOID PhGetHashtableEntry(
    __inout PPH_HASHTABLE Hashtable,
    __in PVOID Entry
    );

BOOLEAN PhRemoveHashtableEntry(
    __inout PPH_HASHTABLE Hashtable,
    __in PVOID Entry
    );

#endif