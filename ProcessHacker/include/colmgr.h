#ifndef PH_COLMGR_H
#define PH_COLMGR_H

#define PH_CM_ORDER_LIMIT 160

typedef LONG (NTAPI *PPH_CM_POST_SORT_FUNCTION)(
    __in LONG Result,
    __in PVOID Node1,
    __in PVOID Node2,
    __in PH_SORT_ORDER SortOrder
    );

typedef struct _PH_CM_MANAGER
{
    HWND Handle;
    ULONG MinId;
    ULONG NextId;
    PPH_CM_POST_SORT_FUNCTION PostSortFunction;
    LIST_ENTRY ColumnListHead;
} PH_CM_MANAGER, *PPH_CM_MANAGER;

typedef struct _PH_CM_COLUMN
{
    LIST_ENTRY ListEntry;
    ULONG Id;
    struct _PH_PLUGIN *Plugin;
    ULONG SubId;
    PVOID Context;
    PVOID SortFunction;
} PH_CM_COLUMN, *PPH_CM_COLUMN;

VOID PhCmInitializeManager(
    __out PPH_CM_MANAGER Manager,
    __in HWND Handle,
    __in ULONG MinId,
    __in PPH_CM_POST_SORT_FUNCTION PostSortFunction
    );

VOID PhCmDeleteManager(
    __in PPH_CM_MANAGER Manager
    );

PPH_CM_COLUMN PhCmCreateColumn(
    __inout PPH_CM_MANAGER Manager,
    __in PPH_TREENEW_COLUMN Column,
    __in struct _PH_PLUGIN *Plugin,
    __in ULONG SubId,
    __in_opt PVOID Context,
    __in PVOID SortFunction
    );

PPH_CM_COLUMN PhCmFindColumn(
    __in PPH_CM_MANAGER Manager,
    __in PPH_STRINGREF PluginName,
    __in ULONG SubId
    );

BOOLEAN PhCmForwardMessage(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in PPH_CM_MANAGER Manager
    );

BOOLEAN PhCmForwardSort(
    __in PPH_TREENEW_NODE *Nodes,
    __in ULONG NumberOfNodes,
    __in ULONG SortColumn,
    __in PH_SORT_ORDER SortOrder,
    __in PPH_CM_MANAGER Manager
    );

PHAPPAPI
BOOLEAN
NTAPI
PhCmLoadSettings(
    __in_opt PPH_CM_MANAGER Manager,
    __in PPH_STRINGREF Settings
    );

PHAPPAPI
PPH_STRING
NTAPI
PhCmSaveSettings(
    __in_opt PPH_CM_MANAGER Manager
    );

#endif
