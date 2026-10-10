/* Resource page records for the native tree. */
#ifndef PVRESOURCES_H
#define PVRESOURCES_H

#include <peview.h>
#include "colmgr.h"

typedef enum _PV_RESOURCES_TREE_COLUMN_ITEM
{
    PV_RESOURCES_TREE_COLUMN_ITEM_INDEX,
    PV_RESOURCES_TREE_COLUMN_ITEM_TYPE,
    PV_RESOURCES_TREE_COLUMN_ITEM_NAME,
    PV_RESOURCES_TREE_COLUMN_ITEM_RVA_START,
    PV_RESOURCES_TREE_COLUMN_ITEM_RVA_END,
    PV_RESOURCES_TREE_COLUMN_ITEM_RVA_SIZE,
    PV_RESOURCES_TREE_COLUMN_ITEM_LCID,
    PV_RESOURCES_TREE_COLUMN_ITEM_HASH,
    PV_RESOURCES_TREE_COLUMN_ITEM_ENTROPY,
    PV_RESOURCES_TREE_COLUMN_ITEM_MAXIMUM
} PV_RESOURCES_TREE_COLUMN_ITEM;

typedef enum _PV_RESOURCES_TREE_NODE_TYPE
{
    PV_RESOURCES_TREE_NODE_TYPE_IMAGE,
    PV_RESOURCES_TREE_NODE_TYPE_MUI,
    PV_RESOURCES_TREE_NODE_TYPE_MAXIMUM
} PV_RESOURCES_TREE_NODE_TYPE;

typedef struct _PV_RESOURCE_NODE
{
    PH_TREENEW_NODE Node;

    struct _PV_RESOURCE_NODE* Parent;
    PPH_LIST Children;
    BOOLEAN HasChildren;

    ULONG64 UniqueId;
    PV_RESOURCES_TREE_NODE_TYPE NodeType;

    PVOID RvaStart;
    PVOID RvaEnd;
    ULONG RvaSize;
    FLOAT ResourcesEntropy;
    PPH_STRING UniqueIdString;
    PPH_STRING TypeString;
    PPH_STRING NameString;
    PPH_STRING RvaStartString;
    PPH_STRING RvaEndString;
    PPH_STRING RvaSizeString;
    PPH_STRING LcidString;
    PPH_STRING HashString;
    PPH_STRING EntropyString;

    PH_STRINGREF TextCache[PV_RESOURCES_TREE_COLUMN_ITEM_MAXIMUM];
} PV_RESOURCE_NODE, *PPV_RESOURCE_NODE;

typedef struct _PV_RESOURCES_CONTEXT
{
    HWND DialogHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    HWND ParentWindowHandle;

    ULONG_PTR SearchMatchHandle;
    PPH_STRING TreeText;

    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;

    ULONG SearchResultsAddIndex;
    PPH_LIST SearchResults;
    PH_QUEUED_LOCK SearchResultsLock;

    PH_CM_MANAGER Cm;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;

    PH_MAPPED_IMAGE MuiMappedImage;
} PV_RESOURCES_CONTEXT, *PPV_RESOURCES_CONTEXT;

VOID PvpPeResourceSaveToFile(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ PPV_RESOURCE_NODE ResourceNode
    );

#endif
