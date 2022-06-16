#ifndef PH_MEMSRCH_H
#define PH_MEMSRCH_H

typedef struct _PH_MEMORY_RESULT
{
    LONG RefCount;
    PVOID Address;
    PVOID BaseAddress;
    SIZE_T Length;
    PH_STRINGREF Display;
} PH_MEMORY_RESULT, *PPH_MEMORY_RESULT;

typedef VOID (NTAPI *PPH_MEMORY_RESULT_CALLBACK)(
    _In_ _Assume_refs_(1) PPH_MEMORY_RESULT Result,
    _In_opt_ PVOID Context
    );

#define PH_DISPLAY_BUFFER_COUNT (PAGE_SIZE * 2 - 1)

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
    ULONG MemoryTypeMask;
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN DetectUnicode : 1;
            BOOLEAN ExtendedUnicode : 1;
            BOOLEAN Spare : 6;
        };
    };
} PH_MEMORY_STRING_OPTIONS, *PPH_MEMORY_STRING_OPTIONS;

PVOID PhAllocateForMemorySearch(
    _In_ SIZE_T Size
    );

VOID PhFreeForMemorySearch(
    _In_ _Post_invalid_ PVOID Memory
    );

PVOID PhCreateMemoryResult(
    _In_ PVOID Address,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Length
    );

VOID PhReferenceMemoryResult(
    _In_ PPH_MEMORY_RESULT Result
    );

VOID PhDereferenceMemoryResult(
    _In_ PPH_MEMORY_RESULT Result
    );

VOID PhDereferenceMemoryResults(
    _In_reads_(NumberOfResults) PPH_MEMORY_RESULT *Results,
    _In_ ULONG NumberOfResults
    );

#endif
