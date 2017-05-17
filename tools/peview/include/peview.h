#ifndef PEVIEW_H
#define PEVIEW_H

#include <ph.h>
#include <cpysave.h>
#include <guisup.h>
#include <mapimg.h>
#include <prsht.h>
#include <prpsh.h>
#include <treenew.h>
#include <settings.h>
#include <symprv.h>
#include <workqueue.h>

#include <shlobj.h>
#include <uxtheme.h>
#include <windowsx.h>

#include "resource.h"

extern PPH_STRING PvFileName;
extern PH_MAPPED_IMAGE PvMappedImage;
extern PIMAGE_COR20_HEADER PvImageCor20Header;
extern PPH_SYMBOL_PROVIDER PvSymbolProvider;

// peprp

VOID PvPeProperties(
    VOID
    );

// libprp

VOID PvLibProperties(
    VOID
    );

// misc

PPH_STRING PvResolveShortcutTarget(
    _In_ PPH_STRING ShortcutFileName
    );

VOID PvCopyListView(
    _In_ HWND ListViewHandle
    );

VOID PvHandleListViewNotifyForCopy(
    _In_ LPARAM lParam,
    _In_ HWND ListViewHandle
    );

// settings

VOID PeInitializeSettings(
    VOID
    );

VOID PeSaveSettings(
    VOID
    );

// symbols

#define WM_PV_SEARCH_FINISHED (WM_APP + 701)

extern ULONG SearchResultsAddIndex;
extern PPH_LIST SearchResults;
extern PH_QUEUED_LOCK SearchResultsLock;

VOID PvCreateSearchControl(
    _In_ HWND WindowHandle,
    _In_opt_ PWSTR BannerText
    );

typedef enum _WCT_TREE_COLUMN_ITEM_NAME
{
    TREE_COLUMN_ITEM_TYPE,
    TREE_COLUMN_ITEM_VA,
    TREE_COLUMN_ITEM_NAME,
    TREE_COLUMN_ITEM_SYMBOL,
    TREE_COLUMN_ITEM_SIZE,
    TREE_COLUMN_ITEM_MAXIMUM
} WCT_TREE_COLUMN_ITEM_NAME;

typedef enum _PV_SYMBOL_TYPE
{
    PV_SYMBOL_TYPE_UNKNOWN,
    PV_SYMBOL_TYPE_FUNCTION,
    PV_SYMBOL_TYPE_SYMBOL,
    PV_SYMBOL_TYPE_LOCAL_VAR,
    PV_SYMBOL_TYPE_STATIC_LOCAL_VAR,
    PV_SYMBOL_TYPE_PARAMETER,
    PV_SYMBOL_TYPE_OBJECT_PTR,
    PV_SYMBOL_TYPE_STATIC_VAR,
    PV_SYMBOL_TYPE_GLOBAL_VAR,
    PV_SYMBOL_TYPE_STRUCT,
    PV_SYMBOL_TYPE_STATIC_MEMBER,
    PV_SYMBOL_TYPE_CONSTANT,
    PV_SYMBOL_TYPE_MEMBER,
    PV_SYMBOL_TYPE_CLASS,
} PV_SYMBOL_TYPE;

typedef struct _PV_SYMBOL_NODE
{
    PH_TREENEW_NODE Node;

    ULONG64 Index;
    PV_SYMBOL_TYPE Type;
    ULONG Size;
    ULONG64 Address;    
    PPH_STRING Name;
    PPH_STRING Data;
    PPH_STRING SizeText;
    WCHAR Pointer[PH_PTR_STR_LEN_1];

    PH_STRINGREF TextCache[TREE_COLUMN_ITEM_MAXIMUM];
} PV_SYMBOL_NODE, *PPV_SYMBOL_NODE;

typedef struct _PH_TN_COLUMN_MENU_DATA
{
    HWND TreeNewHandle;
    PPH_TREENEW_HEADER_MOUSE_EVENT MouseEvent;
    ULONG DefaultSortColumn;
    PH_SORT_ORDER DefaultSortOrder;

    struct _PH_EMENU_ITEM *Menu;
    struct _PH_EMENU_ITEM *Selection;
    ULONG ProcessedId;
} PH_TN_COLUMN_MENU_DATA, *PPH_TN_COLUMN_MENU_DATA;

#define PH_TN_COLUMN_MENU_HIDE_COLUMN_ID ((ULONG)-1)
#define PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID ((ULONG)-2)
#define PH_TN_COLUMN_MENU_SIZE_COLUMN_TO_FIT_ID ((ULONG)-3)
#define PH_TN_COLUMN_MENU_SIZE_ALL_COLUMNS_TO_FIT_ID ((ULONG)-4)
#define PH_TN_COLUMN_MENU_RESET_SORT_ID ((ULONG)-5)

 VOID PhInitializeTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    );

#define PH_TN_COLUMN_MENU_NO_VISIBILITY 0x1
#define PH_TN_COLUMN_MENU_SHOW_RESET_SORT 0x2

VOID PhInitializeTreeNewColumnMenuEx(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data,
    _In_ ULONG Flags
    );

BOOLEAN PhHandleTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    );

VOID PhDeleteTreeNewColumnMenu(
    _In_ PPH_TN_COLUMN_MENU_DATA Data
    );

typedef struct _PH_TN_FILTER_SUPPORT
{
    PPH_LIST FilterList;
    HWND TreeNewHandle;
    PPH_LIST NodeList;
} PH_TN_FILTER_SUPPORT, *PPH_TN_FILTER_SUPPORT;

typedef BOOLEAN (NTAPI *PPH_TN_FILTER_FUNCTION)(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

typedef struct _PH_TN_FILTER_ENTRY
{
    PPH_TN_FILTER_FUNCTION Filter;
    PVOID Context;
} PH_TN_FILTER_ENTRY, *PPH_TN_FILTER_ENTRY;

VOID PhInitializeTreeNewFilterSupport(
    _Out_ PPH_TN_FILTER_SUPPORT Support,
    _In_ HWND TreeNewHandle,
    _In_ PPH_LIST NodeList
    );

VOID PhDeleteTreeNewFilterSupport(
    _In_ PPH_TN_FILTER_SUPPORT Support
    );

PPH_TN_FILTER_ENTRY PhAddTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_FUNCTION Filter,
    _In_opt_ PVOID Context
    );

VOID PhRemoveTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_ENTRY Entry
    );

BOOLEAN PhApplyTreeNewFiltersToNode(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TREENEW_NODE Node
    );

VOID PhApplyTreeNewFilters(
    _In_ PPH_TN_FILTER_SUPPORT Support
    );

typedef struct _PDB_SYMBOL_CONTEXT
{
    HWND DialogHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    HWND ParentWindowHandle;
    HANDLE SearchThreadHandle;
    HANDLE UpdateTimer;

    ULONG64 BaseAddress;
    PPH_STRING FileName;
    PPH_STRING SearchboxText;
    PPH_STRING TreeText;

    PPH_LIST SymbolList;
    PPH_LIST UdtList;
    
    PH_LAYOUT_MANAGER LayoutManager;

    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
} PDB_SYMBOL_CONTEXT, *PPDB_SYMBOL_CONTEXT;

INT_PTR CALLBACK PvpSymbolsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

NTSTATUS PeDumpFileSymbols(
    _In_ PPDB_SYMBOL_CONTEXT Context
    );

VOID PvPdbProperties(
    VOID
    );

VOID PvSymbolAddTreeNode(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ PPV_SYMBOL_NODE Entry
    );

// 

INT_PTR CALLBACK PvpPeImportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeExportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeLoadConfigDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeClrDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeCgfDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif
