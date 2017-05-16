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

#include <uxtheme.h>

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


typedef enum _WCT_TREE_COLUMN_ITEM_NAME
{
    TREE_COLUMN_ITEM_TYPE,
    TREE_COLUMN_ITEM_VA,
    TREE_COLUMN_ITEM_NAME,
    TREE_COLUMN_ITEM_SYMBOL,
    TREE_COLUMN_ITEM_MAXIMUM
} WCT_TREE_COLUMN_ITEM_NAME;

typedef enum _PV_SYMBOL_TYPE
{
    PV_SYMBOL_TYPE_NONE,
    PV_SYMBOL_TYPE_FUNCTION,
    PV_SYMBOL_TYPE_SYMBOL,
    PV_SYMBOL_TYPE_LOCAL_VAR,
    PV_SYMBOL_TYPE_STATIC_LOCAL_VAR,
    PV_SYMBOL_TYPE_PARAMETER,
    PV_SYMBOL_TYPE_OBJECT_PTR,
    PV_SYMBOL_TYPE_STATIC_VAR,
    PV_SYMBOL_TYPE_GLOBAL_VAR,
    PV_SYMBOL_TYPE_MEMBER,
    PV_SYMBOL_TYPE_STATIC_MEMBER,
    PV_SYMBOL_TYPE_CONSTANT
} PV_SYMBOL_TYPE;

typedef struct _PV_SYMBOL_NODE
{
    PH_TREENEW_NODE Node;

    PV_SYMBOL_TYPE Type;
    ULONG Size;
    ULONG64 Address;    
    PPH_STRING Name;
    PPH_STRING Data;
    WCHAR Pointer[PH_PTR_STR_LEN_1];

    PH_STRINGREF TextCache[TREE_COLUMN_ITEM_MAXIMUM];
} PV_SYMBOL_NODE, *PPV_SYMBOL_NODE;


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

VOID PluginsAddTreeNode(
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
