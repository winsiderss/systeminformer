/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2021
 *
 */

#ifndef PEVIEW_H
#define PEVIEW_H

#include <ph.h>
#include <cpysave.h>
#include <emenu.h>
#include <guisup.h>
#include <mapimg.h>
#include <hexedit.h>
#include <prsht.h>
#include <prpsh.h>
#include <treenew.h>
#include <settings.h>
#include <symprv.h>
#include <workqueue.h>

#include <shlobj.h>
#include <uxtheme.h>

#include "..\resource.h"

extern PPH_STRING PvFileName;
EXTERN_C PH_MAPPED_IMAGE PvMappedImage;
extern PIMAGE_COR20_HEADER PvImageCor20Header;
extern PPH_SYMBOL_PROVIDER PvSymbolProvider;
extern HICON PvImageSmallIcon;
extern HICON PvImageLargeIcon;
extern PH_IMAGE_VERSION_INFO PvImageVersionInfo;

#define PV_SCALE_DPI(Value) PhMultiplyDivide(Value, PhGlobalDpi, 96) // phapppub

FORCEINLINE PWSTR PvpGetStringOrNa(
    _In_ PPH_STRING String
    )
{
    return PhGetStringOrDefault(String, (PWSTR)L"N/A");
}

BOOLEAN PvpLoadDbgHelp(
    _Inout_ PPH_SYMBOL_PROVIDER* SymbolProvider
    );

// peprp

VOID PvPeProperties(
    VOID
    );

VOID PvShowPePropertiesWindow(
    VOID
    );

NTSTATUS PhpOpenFileSecurity(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    );

DOUBLE PvCalculateEntropyBuffer(
    _In_ PBYTE Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ DOUBLE* BufferVariance
    );

PPH_STRING PvFormatDoubleCropZero(
    _In_ DOUBLE Value,
    _In_ USHORT Precision
    );

// libprp

VOID PvLibProperties(
    VOID
    );

// exlfprp

VOID PvExlfProperties(
    VOID
    );

// misc

PPH_STRING PvResolveShortcutTarget(
    _In_ PPH_STRING ShortcutFileName
    );

PPH_STRING PvResolveReparsePointTarget(
    _In_ PPH_STRING FileName
    );

typedef NTSTATUS (NTAPI* PV_FILE_ALLOCATION_CALLBACK)(
    _In_ PFILE_ALLOCATED_RANGE_BUFFER Entry,
    _In_ PVOID Context
    );

NTSTATUS PvGetFileAllocatedRanges(
    _In_ HANDLE FileHandle,
    _In_ PV_FILE_ALLOCATION_CALLBACK Callback,
    _In_ PVOID Context
    );

VOID PvCopyListView(
    _In_ HWND ListViewHandle
    );

BOOLEAN PvHandleCopyListViewEMenuItem(
    _In_ struct _PH_EMENU_ITEM* SelectedItem
    );

BOOLEAN PvInsertCopyListViewEMenuItem(
    _In_ struct _PH_EMENU_ITEM* Menu,
    _In_ ULONG InsertAfterId,
    _In_ HWND ListViewHandle
    );

BOOLEAN PvGetListViewContextMenuPoint(
    _In_ HWND ListViewHandle,
    _Out_ PPOINT Point
    );

VOID PvHandleListViewNotifyForCopy(
    _In_ LPARAM lParam,
    _In_ HWND ListViewHandle
    );

VOID PvHandleListViewCommandCopy(
    _In_ HWND WindowHandle,
    _In_ LPARAM lParam,
    _In_ WPARAM wParam,
    _In_ HWND ListViewHandle
    );

VOID PvSaveWindowState(
    _In_ HWND WindowHandle
    );

VOID PvConfigTreeBorders(
    _In_ HWND WindowHandle
    );

// settings

extern BOOLEAN PeEnableThemeSupport;

VOID PvInitializeSettings(
    VOID
    );

VOID PvSaveSettings(
    VOID
    );

VOID PvUpdateCachedSettings(
    VOID
    );

VOID PvShowOptionsWindow(
    _In_ HWND ParentWindow
    );

// symbols

#define WM_PV_SEARCH_FINISHED (WM_APP + 701)
#define WM_PV_SEARCH_SHOWMENU (WM_APP + 702)

extern ULONG SearchResultsAddIndex;
extern PPH_LIST SearchResults;
extern PH_QUEUED_LOCK SearchResultsLock;

VOID PvCreateSearchControl(
    _In_ HWND WindowHandle,
    _In_opt_ PWSTR BannerText
    );

typedef enum _WCT_TREE_COLUMN_ITEM_NAME
{
    TREE_COLUMN_ITEM_INDEX,
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

    ULONG64 UniqueId;
    ULONG TypeId;
    PV_SYMBOL_TYPE Type;
    ULONG64 Size;
    ULONG64 Address;
    PPH_STRING Name;
    PPH_STRING Data;
    PPH_STRING SizeText;
    WCHAR Index[PH_INT64_STR_LEN_1];
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

BOOLEAN PhInsertCopyCellEMenuItem(
    _In_ struct _PH_EMENU_ITEM* Menu,
    _In_ ULONG InsertAfterId,
    _In_ HWND TreeNewHandle,
    _In_ PPH_TREENEW_COLUMN Column
    );

BOOLEAN PhHandleCopyCellEMenuItem(
    _In_ struct _PH_EMENU_ITEM* SelectedItem
    );

// chcol.c

#define PV_CONTROL_TYPE_TREE_NEW 1

VOID PvShowChooseColumnsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HWND ControlHandle,
    _In_ ULONG Type
    );

// pdbprp.c

typedef struct _PDB_SYMBOL_CONTEXT
{
    HWND WindowHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    HWND ParentWindowHandle;
    HANDLE UpdateTimerHandle;

    ULONG64 Count;
    ULONG64 BaseAddress;
    PPH_STRING FileName;
    PPH_STRING SearchboxText;
    PPH_STRING TreeText;

    PPH_LIST SymbolList;
    PPH_LIST UdtList;

    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;

    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;

    PVOID IDiaSession;
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

VOID PdbDumpAddress(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Rva
    );

PPH_STRING PdbGetSymbolDetails(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_opt_ PPH_STRING Name,
    _In_opt_ ULONG Rva
    );

VOID PvPdbProperties(
    VOID
    );

VOID PvSymbolAddTreeNode(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ PPV_SYMBOL_NODE Entry
    );

//

INT_PTR CALLBACK PvPeGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvPeSectionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvPeImportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvPeExportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvPeHeadersDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvPeLoadConfigDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvPeDirectoryDlgProc(
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

INT_PTR CALLBACK PvpPeClrImportsDlgProc(
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

INT_PTR CALLBACK PvPeResourcesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPePropStoreDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeExtendedAttributesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeStreamsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeLayoutDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeLinksDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeProcessesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeTlsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPePreviewDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeProdIdDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeExceptionDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeRelocationDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeSecurityDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeDebugDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeEhContDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeDebugPogoDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeDebugCrtDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeHashesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// ELF

PWSTR PvpGetSymbolTypeName(
    _In_ UCHAR TypeInfo
    );

PWSTR PvpGetSymbolBindingName(
    _In_ UCHAR TypeInfo
    );

PWSTR PvpGetSymbolVisibility(
    _In_ UCHAR OtherInfo
    );

PPH_STRING PvpGetSymbolSectionName(
    _In_ ULONG Index
    );

INT_PTR CALLBACK PvpExlfGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpExlfImportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpExlfExportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpExlfDynamicDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// clrtableimport.cpp

typedef struct _PV_CLR_IMAGE_IMPORT_FUNCTION
{
    ULONG Flags;
    PPH_STRING FunctionName;
    PVOID Offset;
} PV_CLR_IMAGE_IMPORT_FUNCTION, *PPV_CLR_IMAGE_IMPORT_FUNCTION;

typedef struct _PV_CLR_IMAGE_IMPORT_DLL
{
    ULONG ImportToken;
    PPH_STRING ImportName;
    PPH_LIST Functions;
} PV_CLR_IMAGE_IMPORT_DLL, *PPV_CLR_IMAGE_IMPORT_DLL;

EXTERN_C
PPH_STRING
NTAPI
PvClrImportFlagsToString(
    _In_ ULONG Flags
    );

EXTERN_C
HRESULT
NTAPI
PvGetClrImageImports(
    _In_ PVOID ClrMetaDataDispenser,
    _In_ PWSTR FileName,
    _Out_ PPH_LIST* ClrImportsList
    );

#endif
