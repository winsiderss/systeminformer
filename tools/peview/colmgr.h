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
    struct _PV_SYMBOL_NODE *Symbol;
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
    _In_ struct _PV_SYMBOL_NODE *Plugin,
    _In_ ULONG SubId,
    _In_opt_ PVOID Context,
    _In_ PVOID SortFunction
    );

PPH_CM_COLUMN PhCmFindColumn(
    _In_ PPH_CM_MANAGER Manager,
    _In_ PPH_STRINGREF PluginName,
    _In_ ULONG SubId
    );

BOOLEAN PhCmLoadSettings(
    _In_ HWND TreeNewHandle,
    _In_ PPH_STRINGREF Settings
    );

#define PH_CM_COLUMN_WIDTHS_ONLY 0x1

BOOLEAN PhCmLoadSettingsEx(
    _In_ HWND TreeNewHandle,
    _In_opt_ PPH_CM_MANAGER Manager,
    _In_ ULONG Flags,
    _In_ PPH_STRINGREF Settings,
    _In_opt_ PPH_STRINGREF SortSettings
    );

PPH_STRING PhCmSaveSettings(
    _In_ HWND TreeNewHandle
    );

PPH_STRING PhCmSaveSettingsEx(
    _In_ HWND TreeNewHandle,
    _In_opt_ PPH_CM_MANAGER Manager,
    _In_ ULONG Flags,
    _Out_opt_ PPH_STRING *SortSettings
    );

#endif
