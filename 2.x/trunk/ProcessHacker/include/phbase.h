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

#endif