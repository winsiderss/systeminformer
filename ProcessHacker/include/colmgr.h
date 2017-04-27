#ifndef PH_COLMGR_H
#define PH_COLMGR_H

#define PH_CM_ORDER_LIMIT 160

// begin_phapppub
typedef LONG (NTAPI *PPH_CM_POST_SORT_FUNCTION)(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    );
// end_phapppub

typedef struct _PH_CM_MANAGER
{
    HWND Handle;
    ULONG MinId;
    ULONG NextId;
    PPH_CM_POST_SORT_FUNCTION PostSortFunction;
    LIST_ENTRY ColumnListHead;
    PPH_LIST NotifyList;
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
    _Out_ PPH_CM_MANAGER Manager,
    _In_ HWND Handle,
    _In_ ULONG MinId,
    _In_ PPH_CM_POST_SORT_FUNCTION PostSortFunction
    );

VOID PhCmDeleteManager(
    _In_ PPH_CM_MANAGER Manager
    );

PPH_CM_COLUMN PhCmCreateColumn(
    _Inout_ PPH_CM_MANAGER Manager,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ struct _PH_PLUGIN *Plugin,
    _In_ ULONG SubId,
    _In_opt_ PVOID Context,
    _In_ PVOID SortFunction
    );

PPH_CM_COLUMN PhCmFindColumn(
    _In_ PPH_CM_MANAGER Manager,
    _In_ PPH_STRINGREF PluginName,
    _In_ ULONG SubId
    );

VOID PhCmSetNotifyPlugin(
    _In_ PPH_CM_MANAGER Manager,
    _In_ struct _PH_PLUGIN *Plugin
    );

BOOLEAN PhCmForwardMessage(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_ PPH_CM_MANAGER Manager
    );

BOOLEAN PhCmForwardSort(
    _In_ PPH_TREENEW_NODE *Nodes,
    _In_ ULONG NumberOfNodes,
    _In_ ULONG SortColumn,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PPH_CM_MANAGER Manager
    );

// begin_phapppub
PHAPPAPI
BOOLEAN
NTAPI
PhCmLoadSettings(
    _In_ HWND TreeNewHandle,
    _In_ PPH_STRINGREF Settings
    );
// end_phapppub

#define PH_CM_COLUMN_WIDTHS_ONLY 0x1

BOOLEAN PhCmLoadSettingsEx(
    _In_ HWND TreeNewHandle,
    _In_opt_ PPH_CM_MANAGER Manager,
    _In_ ULONG Flags,
    _In_ PPH_STRINGREF Settings,
    _In_opt_ PPH_STRINGREF SortSettings
    );

// begin_phapppub
PHAPPAPI
PPH_STRING
NTAPI
PhCmSaveSettings(
    _In_ HWND TreeNewHandle
    );
// end_phapppub

PPH_STRING PhCmSaveSettingsEx(
    _In_ HWND TreeNewHandle,
    _In_opt_ PPH_CM_MANAGER Manager,
    _In_ ULONG Flags,
    _Out_opt_ PPH_STRING *SortSettings
    );

#endif
