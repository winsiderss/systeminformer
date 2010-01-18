#ifndef PHGUI_H
#define PHGUI_H

#include <phbase.h>
#include <ph.h>
#include <commctrl.h>
#define COBJMACROS
#include <shlobj.h>
#include <prsht.h>
#include "resource.h"

// main

INT WINAPI WinMain(
    __in HINSTANCE hInstance,
    __in HINSTANCE hPrevInstance,
    __in LPSTR lpCmdLine,
    __in INT nCmdShow
    );

INT PhMainMessageLoop();

VOID PhActivatePreviousInstance();

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

typedef BOOL (WINAPI *_ChangeWindowMessageFilter)(
    __in UINT message,
    __in DWORD dwFlag
    );

typedef HRESULT (WINAPI *_SetWindowTheme)(
    __in HWND hwnd,
    __in LPCWSTR pszSubAppName,
    __in LPCWSTR pszSubIdList
    );

typedef HRESULT (WINAPI *_TaskDialogIndirect)(      
    __in const TASKDIALOGCONFIG *pTaskConfig,
    __in int *pnButton,
    __in int *pnRadioButton,
    __in BOOL *pfVerificationFlagChecked
    );

#ifndef GUISUP_PRIVATE
extern _ChangeWindowMessageFilter ChangeWindowMessageFilter_I;
extern _TaskDialogIndirect TaskDialogIndirect_I;
#endif

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

VOID PhGetSelectedListViewItemParams(
    __in HWND hWnd,
    __out PVOID **Items,
    __out PULONG NumberOfItems
    );

#define PH_ANCHOR_LEFT 0x1
#define PH_ANCHOR_TOP 0x2
#define PH_ANCHOR_RIGHT 0x4
#define PH_ANCHOR_BOTTOM 0x8
#define PH_ANCHOR_ALL 0xf

typedef struct _PH_LAYOUT_ITEM
{
    HWND Handle;
    struct _PH_LAYOUT_ITEM *ParentItem;
    ULONG LayoutNumber;
    ULONG NumberOfChildren;
    HDWP DeferHandle;

    RECT Rect;
    RECT OrigRect;
    RECT Margin;
    ULONG Anchor;
} PH_LAYOUT_ITEM, *PPH_LAYOUT_ITEM;

typedef struct _PH_LAYOUT_MANAGER
{
    PPH_LIST List;
    PH_LAYOUT_ITEM RootItem;

    ULONG LayoutNumber;
} PH_LAYOUT_MANAGER, *PPH_LAYOUT_MANAGER;

VOID PhInitializeLayoutManager(
    __out PPH_LAYOUT_MANAGER Manager,
    __in HWND RootWindowHandle
    );

VOID PhDeleteLayoutManager(
    __inout PPH_LAYOUT_MANAGER Manager
    );

PPH_LAYOUT_ITEM PhAddLayoutItem(
    __inout PPH_LAYOUT_MANAGER Manager,
    __in HWND Handle,
    __in PPH_LAYOUT_ITEM ParentItem,
    __in ULONG Anchor
    );

PPH_LAYOUT_ITEM PhAddLayoutItemEx(
    __inout PPH_LAYOUT_MANAGER Manager,
    __in HWND Handle,
    __in PPH_LAYOUT_ITEM ParentItem,
    __in ULONG Anchor,
    __in RECT Margin
    );

VOID PhLayoutManagerLayout(
    __inout PPH_LAYOUT_MANAGER Manager
    );

FORCEINLINE VOID PhResizingMinimumSize(
    __inout PRECT Rect,
    __in WPARAM Edge,
    __in LONG MinimumWidth,
    __in LONG MinimumHeight
    )
{
    if (Edge == WMSZ_BOTTOMRIGHT || Edge == WMSZ_RIGHT || Edge == WMSZ_TOPRIGHT)
    {
        if (Rect->right - Rect->left < MinimumWidth)
            Rect->right = Rect->left + MinimumWidth;
    }
    else if (Edge == WMSZ_BOTTOMLEFT || Edge == WMSZ_LEFT || Edge == WMSZ_TOPLEFT)
    {
        if (Rect->right - Rect->left < MinimumWidth)
            Rect->left = Rect->right - MinimumWidth;
    }

    if (Edge == WMSZ_BOTTOMRIGHT || Edge == WMSZ_BOTTOM || Edge == WMSZ_BOTTOMLEFT)
    {
        if (Rect->bottom - Rect->top < MinimumHeight)
            Rect->bottom = Rect->top + MinimumHeight;
    }
    else if (Edge == WMSZ_TOPRIGHT || Edge == WMSZ_TOP || Edge == WMSZ_TOPLEFT)
    {
        if (Rect->bottom - Rect->top < MinimumHeight)
            Rect->top = Rect->bottom - MinimumHeight;
    }
}

// mainwnd

#define WM_PH_ACTIVATE (WM_APP + 99)
#define PH_ACTIVATE_REPLY 0x1119

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
    BOOLEAN LayoutInitialized;
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

// secedit

typedef NTSTATUS (NTAPI *PPH_GET_OBJECT_SECURITY)(
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Context
    );

typedef NTSTATUS (NTAPI *PPH_SET_OBJECT_SECURITY)(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Context
    );

typedef struct _PH_ACCESS_ENTRY
{
    PWSTR Name;
    ACCESS_MASK Access;
    BOOLEAN General;
    BOOLEAN Specific;
} PH_ACCESS_ENTRY, *PPH_ACCESS_ENTRY;

VOID PhSecurityEditorInitialization();

VOID PhEditSecurity(
    __in HWND hWnd,
    __in PWSTR ObjectName,
    __in PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    __in PPH_SET_OBJECT_SECURITY SetObjectSecurity,
    __in PVOID Context,
    __in PPH_ACCESS_ENTRY AccessEntries,
    __in ULONG NumberOfAccessEntries
    );

typedef NTSTATUS (NTAPI *PPH_OPEN_OBJECT)(
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in PVOID Context
    );

typedef struct _PH_STD_OBJECT_SECURITY
{
    PPH_OPEN_OBJECT OpenObject;
    PVOID Context;
} PH_STD_OBJECT_SECURITY, *PPH_STD_OBJECT_SECURITY;

NTSTATUS PhStdGetObjectSecurity(
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Context
    );

NTSTATUS PhStdSetObjectSecurity(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Context
    );

// secdata

BOOLEAN PhGetAccessEntries(
    __in PWSTR Type,
    __out PPH_ACCESS_ENTRY *AccessEntries,
    __out PULONG NumberOfAccessEntries
    );

// actions

BOOLEAN PhUiTerminateProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

BOOLEAN PhUiSuspendProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

BOOLEAN PhUiResumeProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

// thrdstk

VOID PhShowThreadStackDialog(
    __in HWND ParentWindowHandle,
    __in HANDLE ProcessId,
    __in HANDLE ThreadId,
    __in PPH_SYMBOL_PROVIDER SymbolProvider
    );

#endif