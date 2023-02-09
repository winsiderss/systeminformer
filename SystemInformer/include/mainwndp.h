/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2017-2023
 *
 */

#ifndef PH_MAINWNDP_H
#define PH_MAINWNDP_H

#define PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_1 250
#define PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_2 750
#define PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_LONG_TERM 1000

#define TIMER_FLUSH_PROCESS_QUERY_DATA 1
#define TIMER_ICON_CLICK_ACTIVATE 2
#define TIMER_ICON_RESTORE_HOVER 3

typedef union _PH_MWP_NOTIFICATION_DETAILS
{
    HANDLE ProcessId;
    PPH_STRING ServiceName;
} PH_MWP_NOTIFICATION_DETAILS, *PPH_MWP_NOTIFICATION_DETAILS;

extern PH_PROVIDER_REGISTRATION PhMwpProcessProviderRegistration;
extern PH_PROVIDER_REGISTRATION PhMwpServiceProviderRegistration;
extern PH_PROVIDER_REGISTRATION PhMwpNetworkProviderRegistration;
extern BOOLEAN PhMwpUpdateAutomatically;

extern ULONG PhMwpNotifyIconNotifyMask;
extern ULONG PhMwpLastNotificationType;
extern PH_MWP_NOTIFICATION_DETAILS PhMwpLastNotificationDetails;

LRESULT CALLBACK PhMwpWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// Initialization

RTL_ATOM PhMwpInitializeWindowClass(
    VOID
    );

VOID PhMwpInitializeProviders(
    VOID
    );

VOID PhMwpApplyUpdateInterval(
    _In_ ULONG Interval
    );

VOID PhMwpInitializeControls(
    _In_ HWND WindowHandle
    );

NTSTATUS PhMwpLoadStage1Worker(
    _In_ PVOID Parameter
    );

VOID PhMwpInvokeUpdateWindowFont(
    _In_opt_ PVOID Parameter
    );

VOID PhMwpInvokeUpdateWindowFontMonospace(
    _In_ HWND hwnd,
    _In_opt_ PVOID Parameter
    );

// Event handlers

VOID PhMwpOnDestroy(
    _In_ HWND WindowHandle
    );

VOID PhMwpOnEndSession(
    _In_ HWND WindowHandle
    );

VOID PhMwpOnSettingChange(
    _In_ HWND hwnd
    );

VOID PhMwpOnCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Id
    );

VOID PhMwpOnShowWindow(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    );

BOOLEAN PhMwpOnSysCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Type,
    _In_ LONG CursorScreenX,
    _In_ LONG CursorScreenY
    );

VOID PhMwpOnMenuCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Index,
    _In_ HMENU Menu
    );

VOID PhMwpOnInitMenuPopup(
    _In_ HWND WindowHandle,
    _In_ HMENU Menu,
    _In_ ULONG Index,
    _In_ BOOLEAN IsWindowMenu
    );

VOID PhMwpOnSize(
    _In_ HWND WindowHandle
    );

VOID PhMwpOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    );

VOID PhMwpOnSetFocus(
    VOID
    );

_Success_(return)
BOOLEAN PhMwpOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    );

ULONG_PTR PhMwpOnUserMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    );

// Settings

VOID PhMwpLoadSettings(
    _In_ HWND WindowHandle
    );

VOID PhMwpSaveSettings(
    _In_ HWND WindowHandle
    );

VOID PhMwpSaveWindowState(
    _In_ HWND WindowHandle
    );

// Misc.

VOID PhMwpUpdateLayoutPadding(
    VOID
    );

VOID PhMwpApplyLayoutPadding(
    _Inout_ PRECT Rect,
    _In_ PRECT Padding
    );

VOID PhMwpLayout(
    _Inout_ HDWP *DeferHandle
    );

VOID PhMwpSetupComputerMenu(
    _In_ PPH_EMENU_ITEM Root
    );

BOOLEAN PhMwpExecuteComputerCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Id
    );

VOID PhMwpActivateWindow(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Toggle
    );

// Main menu

PPH_EMENU PhpCreateMainMenu(
    _In_ ULONG SubMenuIndex
    );

VOID PhMwpInitializeMainMenu(
    _In_ HMENU Menu
    );

VOID PhMwpDispatchMenuCommand(
    _In_ HWND WindowHandle,
    _In_ HMENU MenuHandle,
    _In_ ULONG ItemIndex,
    _In_ ULONG ItemId,
    _In_ ULONG_PTR ItemData
    );

VOID PhMwpInitializeSubMenu(
    _In_ HWND hwnd,
    _In_ PPH_EMENU Menu,
    _In_ ULONG Index
    );

VOID PhMwpInitializeSectionMenuItems(
    _In_ PPH_EMENU Menu,
    _In_ ULONG StartIndex
    );

BOOLEAN PhMwpExecuteNotificationMenuCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Id
    );

// Tab control

VOID PhMwpLayoutTabControl(
    _Inout_ HDWP *DeferHandle
    );

VOID PhMwpNotifyTabControl(
    _In_ NMHDR *Header
    );

VOID PhMwpSelectionChangedTabControl(
    _In_ ULONG OldIndex
    );

PPH_MAIN_TAB_PAGE PhMwpCreatePage(
    _In_ PPH_MAIN_TAB_PAGE Template
    );

VOID PhMwpSelectPage(
    _In_ ULONG Index
    );

PPH_MAIN_TAB_PAGE PhMwpFindPage(
    _In_ PPH_STRINGREF Name
    );

PPH_MAIN_TAB_PAGE PhMwpCreateInternalPage(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MAIN_TAB_PAGE_CALLBACK Callback
    );

VOID PhMwpNotifyAllPages(
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

// Notifications

VOID PhMwpAddIconProcesses(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG NumberOfProcesses
    );

VOID PhMwpClearLastNotificationDetails(
    VOID
    );

BOOLEAN PhMwpPluginNotifyEvent(
    _In_ ULONG Type,
    _In_ PVOID Parameter
    );

// Processes

extern PPH_MAIN_TAB_PAGE PhMwpProcessesPage;
extern HWND PhMwpProcessTreeNewHandle;
extern HWND PhMwpSelectedProcessWindowHandle;
extern BOOLEAN PhMwpSelectedProcessVirtualizationEnabled;
extern struct _PH_PROVIDER_EVENT_QUEUE PhMwpProcessEventQueue;

BOOLEAN PhMwpProcessesPageCallback(
    _In_ struct _PH_MAIN_TAB_PAGE *Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

VOID PhMwpShowProcessProperties(
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMwpToggleCurrentUserProcessTreeFilter(
    VOID
    );

BOOLEAN PhMwpCurrentUserProcessTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

VOID PhMwpToggleSignedProcessTreeFilter(
    VOID
    );

BOOLEAN PhMwpSignedProcessTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

BOOLEAN PhMwpExecuteProcessPriorityCommand(
    _In_ ULONG Id,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

BOOLEAN PhMwpExecuteProcessIoPriorityCommand(
    _In_ ULONG Id,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

VOID PhMwpSetProcessMenuPriorityChecks(
    _In_ PPH_EMENU Menu,
    _In_ HANDLE ProcessId,
    _In_ BOOLEAN SetPriority,
    _In_ BOOLEAN SetIoPriority,
    _In_ BOOLEAN SetPagePriority
    );

VOID PhMwpInitializeProcessMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

VOID NTAPI PhMwpProcessAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI PhMwpProcessModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI PhMwpProcessRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI PhMwpProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID PhMwpOnProcessesUpdated(
    _In_ ULONG RunId
    );

// Services

extern PPH_MAIN_TAB_PAGE PhMwpServicesPage;
extern HWND PhMwpServiceTreeNewHandle;
extern struct _PH_PROVIDER_EVENT_QUEUE PhMwpServiceEventQueue;

BOOLEAN PhMwpServicesPageCallback(
    _In_ struct _PH_MAIN_TAB_PAGE *Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

VOID PhMwpNeedServiceTreeList(
    VOID
    );

VOID PhMwpToggleDriverServiceTreeFilter(
    VOID
    );

VOID PhMwpToggleMicrosoftServiceTreeFilter(
    VOID
    );

BOOLEAN PhMwpDriverServiceTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

BOOLEAN PhMwpMicrosoftServiceTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

VOID PhMwpInitializeServiceMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPH_SERVICE_ITEM *Services,
    _In_ ULONG NumberOfServices
    );

VOID NTAPI PhMwpServiceAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI PhMwpServiceModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI PhMwpServiceRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI PhMwpServicesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID PhMwpOnServicesUpdated(
    _In_ ULONG RunId
    );

// Network

extern PPH_MAIN_TAB_PAGE PhMwpNetworkPage;
extern HWND PhMwpNetworkTreeNewHandle;
extern struct _PH_PROVIDER_EVENT_QUEUE PhMwpNetworkEventQueue;

BOOLEAN PhMwpNetworkPageCallback(
    _In_ struct _PH_MAIN_TAB_PAGE *Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

VOID PhMwpNeedNetworkTreeList(
    VOID
    );

VOID PhMwpToggleNetworkWaitingConnectionTreeFilter(
    VOID
    );

BOOLEAN PhMwpNetworkTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

VOID PhMwpInitializeNetworkMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPH_NETWORK_ITEM *NetworkItems,
    _In_ ULONG NumberOfNetworkItems
    );

VOID PhMwpNetworkItemAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID PhMwpNetworkItemModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID PhMwpNetworkItemRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID PhMwpNetworkItemsUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID PhMwpOnNetworkItemsUpdated(
    _In_ ULONG RunId
    );

#endif
