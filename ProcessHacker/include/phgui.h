#ifndef PHGUI_H
#define PHGUI_H

#include <phbase.h>
#include <ph.h>
#include <commctrl.h>
#include <shlobj.h>
#include "resource.h"
#include "prsht.h"

// main

INT WINAPI WinMain(
    __in HINSTANCE hInstance,
    __in HINSTANCE hPrevInstance,
    __in LPSTR lpCmdLine,
    __in INT nCmdShow
    );

INT PhMainMessageLoop();

VOID PhInitializeCommonControls();

VOID PhInitializeFont(
    __in HWND hWnd
    );

VOID PhInitializeKph();

BOOLEAN PhInitializeSystem();

VOID PhInitializeSystemInformation();

ATOM PhRegisterWindowClass();

VOID PhInitializeWindowsVersion();

// guisup

typedef HRESULT (* _SetWindowTheme)(
    __in HWND hwnd,
    __in LPCWSTR pszSubAppName,
    __in LPCWSTR pszSubIdList
    );

VOID PhGuiSupportInitialization();

VOID PhSetControlTheme(
    __in HWND Handle,
    __in PWSTR Theme
    );

HWND PhCreateListViewControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    );

INT PhAddListViewColumn(
    __in HWND ListViewHandle,
    __in INT Index,
    __in INT DisplayIndex,
    __in INT SubItemIndex,
    __in INT Format,
    __in INT Width,
    __in PWSTR Text
    );

INT PhAddListViewItem(
    __in HWND ListViewHandle,
    __in INT Index,
    __in PWSTR Text,
    __in PVOID Param
    );

INT PhFindListViewItemByFlags(
    __in HWND ListViewHandle,
    __in INT StartIndex,
    __in ULONG Flags
    );

INT PhFindListViewItemByParam(
    __in HWND ListViewHandle,
    __in INT StartIndex,
    __in PVOID Param
    );

LOGICAL PhGetListViewItemParam(
    __in HWND ListViewHandle,
    __in INT Index,
    __out PPVOID Param
    );

VOID PhRemoveListViewItem(
    __in HWND ListViewHandle,
    __in INT Index
    );

VOID PhSetListViewSubItem(
    __in HWND ListViewHandle,
    __in INT Index,
    __in INT SubItemIndex,
    __in PWSTR Text
    );

HWND PhCreateTabControl(
    __in HWND ParentHandle
    );

INT PhAddTabControlTab(
    __in HWND TabControlHandle,
    __in INT Index,
    __in PWSTR Text
    );

PPH_STRING PhGetWindowText(
    __in HWND hwnd
    );

VOID PhShowContextMenu(
    __in HWND hwnd,
    __in HWND subHwnd,
    __in HMENU menu,
    __in POINT point
    );

// mainwnd

#define WM_PH_PROCESS_ADDED (WM_APP + 101)
#define WM_PH_PROCESS_MODIFIED (WM_APP + 102)
#define WM_PH_PROCESS_REMOVED (WM_APP + 103)

#define WM_PH_SERVICE_ADDED (WM_APP + 104)
#define WM_PH_SERVICE_MODIFIED (WM_APP + 105)
#define WM_PH_SERVICE_REMOVED (WM_APP + 106)

BOOLEAN PhMainWndInitialization(
    __in INT ShowCommand
    );

LRESULT CALLBACK PhMainWndProc(      
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

// procprp

#define PH_PROCESS_PROPCONTEXT_MAXPAGES 20

typedef struct _DLGTEMPLATEEX
{
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATEEX, *PDLGTEMPLATEEX;

typedef struct _PH_PROCESS_PROPCONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    HANDLE WindowHandle;
    PH_EVENT CreatedEvent;
    PROPSHEETHEADER PropSheetHeader;
    HPROPSHEETPAGE *PropSheetPages;
} PH_PROCESS_PROPCONTEXT, *PPH_PROCESS_PROPCONTEXT;

typedef struct _PH_PROCESS_PROPPAGECONTEXT
{
    PPH_PROCESS_PROPCONTEXT PropContext;
    PVOID Context;
    PROPSHEETPAGE PropSheetPage;
} PH_PROCESS_PROPPAGECONTEXT, *PPH_PROCESS_PROPPAGECONTEXT;

BOOLEAN PhProcessPropInitialization();

PPH_PROCESS_PROPCONTEXT PhCreateProcessPropContext(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    );

BOOLEAN PhAddProcessPropPage(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    );

PPH_PROCESS_PROPPAGECONTEXT PhCreateProcessPropPageContext(
    __in LPCWSTR Template,
    __in DLGPROC DlgProc,
    __in PVOID Context
    );

BOOLEAN PhShowProcessProperties(
    __in PPH_PROCESS_PROPCONTEXT Context
    );

#endif