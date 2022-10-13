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
    SystemDefaultActivationContextDataRegion
} PH_MEMORY_REGION_TYPE;

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

    WCHAR BaseAddressString[PH_PTR_STR_LEN_1];

    struct _PH_MEMORY_ITEM *AllocationBaseItem;

    SIZE_T CommittedSize;
    SIZE_T PrivateSize;

    SIZE_T TotalWorkingSetPages;
    SIZE_T PrivateWorkingSetPages;
    SIZE_T SharedWorkingSetPages;
    SIZE_T ShareableWorkingSetPages;
    SIZE_T LockedWorkingSetPages;

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
        } Heap;
        struct
        {
            struct _PH_MEMORY_ITEM *HeapItem;
        } HeapSegment;
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
    _Out_writes_(17) PWSTR String
    );

PWSTR PhGetMemoryStateString(
    _In_ ULONG State
    );

PWSTR PhGetMemoryTypeString(
    _In_ ULONG Type
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
