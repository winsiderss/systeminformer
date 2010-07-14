#ifndef MEMSRCH_H
#define MEMSRCH_H

typedef struct _PH_MEMORY_RESULT
{
    LONG RefCount;
    PVOID Address;
    SIZE_T Length;
    PH_STRINGREF Display;
} PH_MEMORY_RESULT, *PPH_MEMORY_RESULT;

typedef VOID (NTAPI *PPH_MEMORY_RESULT_CALLBACK)(
    __in __assumeRefs(1) PPH_MEMORY_RESULT Result,
    __in PVOID Context
    );

typedef struct _PH_MEMORY_SEARCH_OPTIONS
{
    BOOLEAN Cancel;
    PPH_MEMORY_RESULT_CALLBACK Callback;
    PVOID Context;
} PH_MEMORY_SEARCH_OPTIONS, *PPH_MEMORY_SEARCH_OPTIONS;

typedef struct _PH_MEMORY_STRING_OPTIONS
{
    PH_MEMORY_SEARCH_OPTIONS Header;

    ULONG MinimumLength;
    BOOLEAN DetectUnicode;
    ULONG MemoryTypeMask;
} PH_MEMORY_STRING_OPTIONS, *PPH_MEMORY_STRING_OPTIONS;

PVOID PhAllocateForMemorySearch(
    __in SIZE_T Size
    );

VOID PhFreeForMemorySearch(
    __in __post_invalid PVOID Memory
    );

PVOID PhCreateMemoryResult(
    __in PVOID Address,
    __in SIZE_T Length
    );

VOID PhReferenceMemoryResult(
    __in PPH_MEMORY_RESULT Result
    );

VOID PhDereferenceMemoryResult(
    __in PPH_MEMORY_RESULT Result
    );

VOID PhDereferenceMemoryResults(
    __in_ecount(NumberOfResults) PPH_MEMORY_RESULT *Results,
    __in ULONG NumberOfResults
    );

#endif
