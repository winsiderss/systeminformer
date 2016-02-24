#ifndef PH_MEMLIST_H
#define PH_MEMLIST_H

#include <phuisup.h>
#include <colmgr.h>

// Columns

#define PHMMTLC_BASEADDRESS 0
#define PHMMTLC_TYPE 1
#define PHMMTLC_SIZE 2
#define PHMMTLC_PROTECTION 3
#define PHMMTLC_USE 4
#define PHMMTLC_TOTALWS 5
#define PHMMTLC_PRIVATEWS 6
#define PHMMTLC_SHAREABLEWS 7
#define PHMMTLC_SHAREDWS 8
#define PHMMTLC_LOCKEDWS 9
#define PHMMTLC_COMMITTED 10
#define PHMMTLC_PRIVATE 11

#define PHMMTLC_MAXIMUM 12

// begin_phapppub
typedef struct _PH_MEMORY_NODE
{
    PH_TREENEW_NODE Node;

    BOOLEAN IsAllocationBase;
    BOOLEAN Reserved1;
    USHORT Reserved2;
    PPH_MEMORY_ITEM MemoryItem;

    struct _PH_MEMORY_NODE *Parent;
    PPH_LIST Children;
// end_phapppub

    PH_STRINGREF TextCache[PHMMTLC_MAXIMUM];

    WCHAR BaseAddressText[PH_PTR_STR_LEN_1];
    WCHAR TypeText[30];
    PPH_STRING SizeText;
    WCHAR ProtectionText[17];
    PPH_STRING UseText;
    PPH_STRING TotalWsText;
    PPH_STRING PrivateWsText;
    PPH_STRING ShareableWsText;
    PPH_STRING SharedWsText;
    PPH_STRING LockedWsText;
    PPH_STRING CommittedText;
    PPH_STRING PrivateText;
// begin_phapppub
} PH_MEMORY_NODE, *PPH_MEMORY_NODE;
// end_phapppub

typedef struct _PH_MEMORY_LIST_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;
    BOOLEAN HideFreeRegions;

    PPH_LIST AllocationBaseNodeList; // Allocation base nodes (list should always be sorted by base address)
    PPH_LIST RegionNodeList; // Memory region nodes
} PH_MEMORY_LIST_CONTEXT, *PPH_MEMORY_LIST_CONTEXT;

VOID PhInitializeMemoryList(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PPH_MEMORY_LIST_CONTEXT Context
    );

VOID PhDeleteMemoryList(
    _In_ PPH_MEMORY_LIST_CONTEXT Context
    );

VOID PhLoadSettingsMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context
    );

VOID PhSaveSettingsMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context
    );

VOID PhSetOptionsMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ BOOLEAN HideFreeRegions
    );

VOID PhReplaceMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ PPH_MEMORY_ITEM_LIST List
    );

VOID PhUpdateMemoryNode(
    _In_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ PPH_MEMORY_NODE MemoryNode
    );

PPH_MEMORY_NODE PhGetSelectedMemoryNode(
    _In_ PPH_MEMORY_LIST_CONTEXT Context
    );

VOID PhGetSelectedMemoryNodes(
    _In_ PPH_MEMORY_LIST_CONTEXT Context,
    _Out_ PPH_MEMORY_NODE **MemoryNodes,
    _Out_ PULONG NumberOfMemoryNodes
    );

VOID PhDeselectAllMemoryNodes(
    _In_ PPH_MEMORY_LIST_CONTEXT Context
    );

#endif
