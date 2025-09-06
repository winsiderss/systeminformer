/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2018-2023
 *
 */

#ifndef PH_MEMPRV_H
#define PH_MEMPRV_H

extern PPH_OBJECT_TYPE PhMemoryItemType;

// begin_phapppub
typedef enum _PH_MEMORY_REGION_TYPE
{
    UnknownRegion,
    CustomRegion,
    UnusableRegion,
    MappedFileRegion,
    UserSharedDataRegion,
    PebRegion,
    Peb32Region,
    TebRegion,
    Teb32Region, // Not used
    StackRegion,
    Stack32Region,
    HeapRegion,
    Heap32Region,
    HeapSegmentRegion,
    HeapSegment32Region,
    CfgBitmapRegion,
    CfgBitmap32Region,
    ApiSetMapRegion,
    HypervisorSharedDataRegion,
    ReadOnlySharedMemoryRegion,
    CodePageDataRegion,
    GdiSharedHandleTableRegion,
    ShimDataRegion,
    ActivationContextDataRegion,
    WerRegistrationDataRegion,
    SiloSharedDataRegion,
    TelemetryCoverageRegion,
    ProcessParametersRegion,
    LeapSecondDataRegion,
} PH_MEMORY_REGION_TYPE;

typedef enum _PH_ACTIVATION_CONTEXT_DATA_TYPE
{
    CustomActivationContext,
    ProcessActivationContext,
    SystemActivationContext
} PH_ACTIVATION_CONTEXT_DATA_TYPE;

typedef struct _PH_MEMORY_ITEM
{
    LIST_ENTRY ListEntry;
    PH_AVL_LINKS Links;

    union
    {
        MEMORY_BASIC_INFORMATION BasicInfo;
        struct
        {
            PVOID BaseAddress;
            PVOID AllocationBase;
            ULONG AllocationProtect;
            SIZE_T RegionSize;
            ULONG State;
            ULONG Protect;
            ULONG Type;
        };
    };

    union
    {
        BOOLEAN Attributes;
        struct
        {
            BOOLEAN Valid : 1;
            BOOLEAN Bad : 1;
            BOOLEAN Spare : 6;
        };
    };
    union
    {
        ULONG RegionTypeEx;
        struct
        {
            ULONG Private : 1;
            ULONG MappedDataFile : 1;
            ULONG MappedImage : 1;
            ULONG MappedPageFile : 1;
            ULONG MappedPhysical : 1;
            ULONG DirectMapped : 1;
            ULONG SoftwareEnclave : 1; // REDSTONE3
            ULONG PageSize64K : 1;
            ULONG PlaceholderReservation : 1; // REDSTONE4
            ULONG MappedAwe : 1; // 21H1
            ULONG MappedWriteWatch : 1;
            ULONG PageSizeLarge : 1;
            ULONG PageSizeHuge : 1;
            ULONG Reserved : 19; // Sync with MemoryRegionInformationEx (dmex)
        };
    };

    WCHAR BaseAddressString[PH_PTR_STR_LEN_1];

    struct _PH_MEMORY_ITEM *AllocationBaseItem;

    SIZE_T CommittedSize;
    SIZE_T PrivateSize;

    SIZE_T TotalWorkingSetPages;
    SIZE_T PrivateWorkingSetPages;
    SIZE_T SharedWorkingSetPages;
    SIZE_T ShareableWorkingSetPages;
    SIZE_T LockedWorkingSetPages;

    SIZE_T SharedOriginalPages;

    ULONG_PTR Priority;
    PH_MEMORY_REGION_TYPE RegionType;

    union
    {
        struct
        {
            PPH_STRING Text;
            BOOLEAN PropertyOfAllocationBase;
        } Custom;
        struct
        {
            PPH_STRING FileName;
            BOOLEAN SigningLevelValid;
            SE_SIGNING_LEVEL SigningLevel;
        } MappedFile;
        struct
        {
            HANDLE ThreadId;
        } Teb;
        struct
        {
            HANDLE ThreadId;
        } Stack;
        struct
        {
            ULONG Index;
            BOOLEAN ClassValid;
            ULONG Class;
        } Heap;
        struct
        {
            struct _PH_MEMORY_ITEM *HeapItem;
        } HeapSegment;
        struct
        {
            PH_ACTIVATION_CONTEXT_DATA_TYPE Type;
        } ActivationContextData;
    } u;
} PH_MEMORY_ITEM, *PPH_MEMORY_ITEM;

typedef struct _PH_MEMORY_ITEM_LIST
{
    HANDLE ProcessId;
    PH_AVL_TREE Set;
    LIST_ENTRY ListHead;
} PH_MEMORY_ITEM_LIST, *PPH_MEMORY_ITEM_LIST;
// end_phapppub

VOID PhGetMemoryProtectionString(
    _In_ ULONG Protection,
    _Inout_z_ PWSTR String
    );

PCPH_STRINGREF PhGetMemoryStateString(
    _In_ ULONG State
    );

PCPH_STRINGREF PhGetMemoryTypeString(
    _In_ ULONG Type
    );

PCPH_STRINGREF PhGetSigningLevelString(
    _In_ SE_SIGNING_LEVEL SigningLevel
    );

PCPH_STRINGREF PhGetMemoryPagePriorityString(
    _In_ ULONG PagePriority
    );

PPH_STRING PhGetMemoryRegionTypeExString(
    _In_ PPH_MEMORY_ITEM MemoryItem
    );

PPH_MEMORY_ITEM PhCreateMemoryItem(
    VOID
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhDeleteMemoryItemList(
    _In_ PPH_MEMORY_ITEM_LIST List
    );

PHAPPAPI
PPH_MEMORY_ITEM
NTAPI
PhLookupMemoryItemList(
    _In_ PPH_MEMORY_ITEM_LIST List,
    _In_ PVOID Address
    );

#define PH_QUERY_MEMORY_IGNORE_FREE        0x1
#define PH_QUERY_MEMORY_REGION_TYPE        0x2
#define PH_QUERY_MEMORY_WS_COUNTERS        0x4
#define PH_QUERY_MEMORY_ZERO_PAD_ADDRESSES 0x8

PHAPPAPI
NTSTATUS
NTAPI
PhQueryMemoryItemList(
    _In_ HANDLE ProcessId,
    _In_ ULONG Flags,
    _Out_ PPH_MEMORY_ITEM_LIST List
    );
// end_phapppub

#endif
