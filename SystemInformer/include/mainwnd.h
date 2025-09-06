/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2017-2023
 *
 */

#ifndef PH_MAINWND_H
#define PH_MAINWND_H

extern HWND PhMainWndHandle;
extern BOOLEAN PhMainWndExiting;

// begin_phapppub
#define WM_PH_FIRST (WM_APP + 99)
#define WM_PH_ACTIVATE (WM_APP + 99)
#define WM_PH_SHOW_DIALOG (WM_APP + 100) // unused (plugins only)
#define WM_PH_UPDATE_DIALOG (WM_APP + 101) // unused (plugins only)
#define PH_ACTIVATE_REPLY 0x1119
#define WM_PH_NOTIFY_ICON_MESSAGE (WM_APP + 126)
#define WM_PH_UPDATE_FONT (WM_APP + 136)
// end_phapppub
#define WM_PH_DESTROY (WM_APP + 145)
#define WM_PH_INVOKE (WM_APP + 146)
#define WM_PH_PREPARE_FOR_EARLY_SHUTDOWN (WM_APP + 147)
#define WM_PH_SAVE_ALL_SETTINGS (WM_APP + 148)
#define WM_PH_SHOW_PROPERTIES (WM_APP + 149)
#define WM_PH_ACTIVATE_WINDOW (WM_APP + 150)
#define WM_PH_SELECT_NODE (WM_APP + 151)
#define WM_PH_SHOW_EDITOR (WM_APP + 152)
#define WM_PH_SHOW_RESULT (WM_APP + 153)
#define WM_PH_LAST (WM_APP + 154)

// begin_phapppub
typedef enum _PH_MAINWINDOW_CALLBACK_TYPE
{
    PH_MAINWINDOW_CALLBACK_TYPE_DESTROY,
    PH_MAINWINDOW_CALLBACK_TYPE_SHOW_PROPERTIES,
    PH_MAINWINDOW_CALLBACK_TYPE_SAVE_ALL_SETTINGS,
    PH_MAINWINDOW_CALLBACK_TYPE_PREPARE_FOR_EARLY_SHUTDOWN,
    PH_MAINWINDOW_CALLBACK_TYPE_CANCEL_EARLY_SHUTDOWN,
    PH_MAINWINDOW_CALLBACK_TYPE_TOGGLE_VISIBLE,
    PH_MAINWINDOW_CALLBACK_TYPE_SHOW_MEMORY_EDITOR,
    PH_MAINWINDOW_CALLBACK_TYPE_SHOW_MEMORY_RESULTS,
    PH_MAINWINDOW_CALLBACK_TYPE_SELECT_TAB_PAGE,
    PH_MAINWINDOW_CALLBACK_TYPE_GET_CALLBACK_LAYOUT_PADDING,
    PH_MAINWINDOW_CALLBACK_TYPE_INVALIDATE_LAYOUT_PADDING,
    PH_MAINWINDOW_CALLBACK_TYPE_SELECT_PROCESS_NODE,
    PH_MAINWINDOW_CALLBACK_TYPE_SELECT_SERVICE_ITEM,
    PH_MAINWINDOW_CALLBACK_TYPE_SELECT_NETWORK_ITEM,
    PH_MAINWINDOW_CALLBACK_TYPE_UPDATE_FONT,
    PH_MAINWINDOW_CALLBACK_TYPE_GET_FONT,
    PH_MAINWINDOW_CALLBACK_TYPE_INVOKE,
    PH_MAINWINDOW_CALLBACK_TYPE_POST,
    PH_MAINWINDOW_CALLBACK_TYPE_REFRESH,
    PH_MAINWINDOW_CALLBACK_TYPE_CREATE_TAB_PAGE,
    PH_MAINWINDOW_CALLBACK_TYPE_GET_UPDATE_AUTOMATICALLY,
    PH_MAINWINDOW_CALLBACK_TYPE_SET_UPDATE_AUTOMATICALLY,
    PH_MAINWINDOW_CALLBACK_TYPE_ICON_CLICK,
    PH_MAINWINDOW_CALLBACK_TYPE_WINDOW_BASE,
    PH_MAINWINDOW_CALLBACK_TYPE_GETWINDOW_PROCEDURE,
    PH_MAINWINDOW_CALLBACK_TYPE_SETWINDOW_PROCEDURE,
    PH_MAINWINDOW_CALLBACK_TYPE_WINDOW_HANDLE,
    PH_MAINWINDOW_CALLBACK_TYPE_VERSION,
    PH_MAINWINDOW_CALLBACK_TYPE_PORTABLE,
    PH_MAINWINDOW_CALLBACK_TYPE_PAGEINDEX,
    PH_MAINWINDOW_CALLBACK_TYPE_WINDOWDPI,
    PH_MAINWINDOW_CALLBACK_TYPE_WINDOWNAME,
    PH_MAINWINDOW_CALLBACK_TYPE_MAXIMUM
} PH_MAINWINDOW_CALLBACK_TYPE;

PHAPPAPI
PVOID
NTAPI
PhPluginInvokeWindowCallback(
    _In_ PH_MAINWINDOW_CALLBACK_TYPE Event,
    _In_opt_ PVOID wparam,
    _In_opt_ PVOID lparam
    );

#define SystemInformer_Destroy() \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_DESTROY, 0, 0)
#define SystemInformer_ShowProcessProperties(ProcessItem) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_SHOW_PROPERTIES, 0, (PVOID)(ULONG_PTR)(ProcessItem))
#define SystemInformer_SaveAllSettings() \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_SAVE_ALL_SETTINGS, 0, 0)
#define SystemInformer_PrepareForEarlyShutdown() \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_PREPARE_FOR_EARLY_SHUTDOWN, 0, 0)
#define SystemInformer_CancelEarlyShutdown() \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_CANCEL_EARLY_SHUTDOWN, 0, 0)
#define SystemInformer_ToggleVisible(AlwaysShow) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_TOGGLE_VISIBLE, (PVOID)(ULONG_PTR)(AlwaysShow), 0)
#define SystemInformer_ShowMemoryEditor(ShowMemoryEditor) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_SHOW_MEMORY_EDITOR, 0, (PVOID)(ULONG_PTR)(ShowMemoryEditor))
#define SystemInformer_ShowMemoryResults(ShowMemoryResults) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_SHOW_MEMORY_RESULTS, 0, (PVOID)(ULONG_PTR)(ShowMemoryResults))
#define SystemInformer_SelectTabPage(Index) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_SELECT_TAB_PAGE, 0, (PVOID)(ULONG_PTR)(Index))
#define SystemInformer_GetCallbackLayoutPadding() \
    ((PPH_CALLBACK)PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_GET_CALLBACK_LAYOUT_PADDING, 0, 0))
#define SystemInformer_InvalidateLayoutPadding() \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_INVALIDATE_LAYOUT_PADDING, 0, 0)
#define SystemInformer_SelectProcessNode(ProcessNode) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_SELECT_PROCESS_NODE, 0, (PVOID)(ULONG_PTR)(ProcessNode))
#define SystemInformer_SelectServiceItem(ServiceItem) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_SELECT_SERVICE_ITEM, 0, (PVOID)(ULONG_PTR)(ServiceItem))
#define SystemInformer_SelectNetworkItem(NetworkItem) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_SELECT_NETWORK_ITEM, 0, (PVOID)(ULONG_PTR)(NetworkItem))
#define SystemInformer_UpdateFont() \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_UPDATE_FONT, 0, 0)
#define SystemInformer_GetFont() \
    ((HFONT)PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_GET_FONT, 0, 0))
#define SystemInformer_Invoke(Function, Parameter) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_INVOKE, (PVOID)(ULONG_PTR)(Parameter), (PVOID)(ULONG_PTR)(Function))
#define SystemInformer_Post(Function, Parameter) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_POST, (PVOID)(ULONG_PTR)(Parameter), (PVOID)(ULONG_PTR)(Function))
//#define SystemInformer_CreateTabPage(Template) \
//    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_CREATE_TAB_PAGE, 0, (PVOID)(ULONG_PTR)(Template))
#define SystemInformer_Refresh() \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_REFRESH, 0, 0)
#define SystemInformer_GetUpdateAutomatically() \
    ((BOOLEAN)PtrToUlong(PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_GET_UPDATE_AUTOMATICALLY, 0, 0)))
#define SystemInformer_SetUpdateAutomatically(Value) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_SET_UPDATE_AUTOMATICALLY, (PVOID)(ULONG_PTR)(Value), 0)
#define SystemInformer_IconClick() \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_ICON_CLICK, 0, 0)
#define SystemInformer_GetInstanceHandle() \
    ((PVOID)PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_WINDOW_BASE, 0, 0))
#define SystemInformer_GetWindowProcedure() \
    ((WNDPROC)PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_GETWINDOW_PROCEDURE, 0, 0))
#define SystemInformer_SetWindowProcedure(Value) \
    PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_SETWINDOW_PROCEDURE, (PVOID)(ULONG_PTR)(Value), 0)
#define SystemInformer_GetWindowHandle() \
    ((HWND)PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_WINDOW_HANDLE, 0, 0))
#define SystemInformer_GetWindowsVersion() \
    (PtrToUlong(PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_VERSION, 0, 0)))
#define SystemInformer_IsPortableMode() \
    ((BOOLEAN)PtrToUlong(PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_PORTABLE, 0, 0)))
#define SystemInformer_GetTabIndex(Value) \
    (PtrToUlong(PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_PAGEINDEX, (PVOID)(ULONG_PTR)(Value), 0)))
#define SystemInformer_GetWindowDpi() \
    (PtrToUlong(PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_WINDOWDPI, 0, 0)))
#define SystemInformer_GetWindowName() \
    (PWSTR)(PhPluginInvokeWindowCallback(PH_MAINWINDOW_CALLBACK_TYPE_WINDOWNAME, 0, 0))

#define PhWindowsVersion SystemInformer_GetWindowsVersion() // Temporary backwards compat (dmex)
// end_phapppub

// begin_phapppub
PHAPPAPI
PVOID
NTAPI
PhPluginCreateTabPage(
    _In_ PVOID Page
    );
// end_phapppub

typedef struct _PH_SHOW_MEMORY_EDITOR
{
    HWND OwnerWindow;
    HANDLE ProcessId;
    PVOID BaseAddress;
    SIZE_T RegionSize;
    ULONG SelectOffset;
    ULONG SelectLength;
    PPH_STRING Title;
    ULONG Flags;
} PH_SHOW_MEMORY_EDITOR, *PPH_SHOW_MEMORY_EDITOR;

typedef struct _PH_SHOW_MEMORY_RESULTS
{
    HANDLE ProcessId;
    PPH_LIST Results;
} PH_SHOW_MEMORY_RESULTS, *PPH_SHOW_MEMORY_RESULTS;

// begin_phapppub
typedef struct _PH_LAYOUT_PADDING_DATA
{
    RECT Padding;
} PH_LAYOUT_PADDING_DATA, *PPH_LAYOUT_PADDING_DATA;
// end_phapppub

// begin_phapppub
typedef enum _PH_MAIN_TAB_PAGE_MESSAGE
{
    MainTabPageCreate,
    MainTabPageDestroy,
    MainTabPageCreateWindow, // HWND *Parameter1 (WindowHandle), HWND Parameter2 (ParentWindow)
    MainTabPageSelected, // BOOLEAN Parameter1 (Selected)
    MainTabPageInitializeSectionMenuItems, // PPH_MAIN_TAB_PAGE_MENU_INFORMATION Parameter1

    MainTabPageLoadSettings,
    MainTabPageSaveSettings,
    MainTabPageExportContent, // PPH_MAIN_TAB_PAGE_EXPORT_CONTENT Parameter1
    MainTabPageFontChanged, // HFONT Parameter1 (Font)
    MainTabPageUpdateAutomaticallyChanged, // BOOLEAN Parameter1 (UpdateAutomatically)
    MainTabPageDpiChanged,

    MaxMainTabPageMessage
} PH_MAIN_TAB_PAGE_MESSAGE;

typedef struct _PH_MAIN_TAB_PAGE *PPH_MAIN_TAB_PAGE;

typedef BOOLEAN (NTAPI *PPH_MAIN_TAB_PAGE_CALLBACK)(
    _In_ PPH_MAIN_TAB_PAGE Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

typedef struct _PH_MAIN_TAB_PAGE_EXPORT_CONTENT
{
    PPH_FILE_STREAM FileStream;
    ULONG Mode;
} PH_MAIN_TAB_PAGE_EXPORT_CONTENT, *PPH_MAIN_TAB_PAGE_EXPORT_CONTENT;

typedef struct _PH_MAIN_TAB_PAGE_MENU_INFORMATION
{
    PPH_EMENU_ITEM Menu;
    ULONG StartIndex;
} PH_MAIN_TAB_PAGE_MENU_INFORMATION, *PPH_MAIN_TAB_PAGE_MENU_INFORMATION;

typedef struct _PH_MAIN_TAB_PAGE
{
    // Public

    PH_STRINGREF Name;
    ULONG Flags;
    PPH_MAIN_TAB_PAGE_CALLBACK Callback;
    PVOID Context;

    LONG Index;
    union
    {
        ULONG StateFlags;
        struct
        {
            ULONG Selected : 1;
            ULONG CreateWindowCalled : 1;
            ULONG SpareStateFlags : 30;
        };
    };
    PVOID Reserved[2];
// end_phapppub

    // Private

    HWND WindowHandle;
// begin_phapppub
} PH_MAIN_TAB_PAGE, *PPH_MAIN_TAB_PAGE;
// end_phapppub

// begin_phapppub
#define PH_NOTIFY_MINIMUM 0x1
#define PH_NOTIFY_PROCESS_CREATE 0x1
#define PH_NOTIFY_PROCESS_DELETE 0x2
#define PH_NOTIFY_SERVICE_CREATE 0x4
#define PH_NOTIFY_SERVICE_DELETE 0x8
#define PH_NOTIFY_SERVICE_START 0x10
#define PH_NOTIFY_SERVICE_STOP 0x20
#define PH_NOTIFY_SERVICE_MODIFIED 0x40
#define PH_NOTIFY_DEVICE_ARRIVED 0x80
#define PH_NOTIFY_DEVICE_REMOVED 0x100
#define PH_NOTIFY_MAXIMUM 0x200
#define PH_NOTIFY_VALID_MASK 0x1ff
// end_phapppub

BOOLEAN PhMainWndInitialization(
    _In_ LONG ShowCommand
    );

VOID PhAddMiniProcessMenuItems(
    _Inout_ PPH_EMENU_ITEM Menu,
    _In_ HANDLE ProcessId
    );

BOOLEAN PhHandleMiniProcessMenuItem(
    _Inout_ PPH_EMENU_ITEM MenuItem
    );

VOID PhShowIconContextMenu(
    _In_ HWND WindowHandle,
    _In_ POINT Location
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhShowIconNotification(
    _In_ PCWSTR Title,
    _In_ PCWSTR Text
    );

typedef enum _PH_TOAST_REASON
{
    PhToastReasonUserCanceled,
    PhToastReasonApplicationHidden,
    PhToastReasonTimedOut,
    PhToastReasonActivated,
    PhToastReasonError,
    PhToastReasonUnknown
} PH_TOAST_REASON;

typedef _Function_class_(PH_TOAST_CALLBACK)
VOID NTAPI PH_TOAST_CALLBACK(
    _In_ HRESULT Result,
    _In_ PH_TOAST_REASON Reason,
    _In_ PVOID Context
);
typedef PH_TOAST_CALLBACK* PPH_TOAST_CALLBACK;

PHAPPAPI
HRESULT
NTAPI
PhShowIconNotificationEx(
    _In_ PCWSTR Title,
    _In_ PCWSTR Text,
    _In_ ULONG Timeout,
    _In_opt_ PPH_TOAST_CALLBACK Callback,
    _In_opt_ PVOID Context
    );
// end_phapppub

VOID PhShowDetailsForIconNotification(
    VOID
    );

VOID PhShowOptionsRestartRequired(
    _In_ HWND WindowHandle
    );

BOOLEAN PhShowOptionsDefaultInstallLocation(
    _In_ HWND ParentWindowHandle,
    _In_ PCWSTR Message
    );

VOID PhShowProcessContextMenu(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    );

VOID PhShowServiceContextMenu(
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    );

VOID PhShowNetworkContextMenu(
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    );

typedef struct _PH_EMENU_ITEM PH_EMENU, *PPH_EMENU;

VOID PhServiceListInsertContextMenu(
    _In_ HWND ParentWindow,
    _In_ PPH_EMENU Menu,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices
    );

#endif
