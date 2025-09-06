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
#define TIMER_ICON_POPUPOPEN 4

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

PPH_STRING PhMwpInitializeWindowTitle(
    _In_ ULONG KphLevel
    );

VOID PhMwpInitializeProviders(
    VOID
    );

VOID PhMwpShowWindow(
    _In_ LONG ShowCommand
    );

VOID PhMwpApplyUpdateInterval(
    _In_ ULONG Interval
    );

VOID PhMwpInitializeMetrics(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDpi
    );

VOID PhMwpInitializeControls(
    _In_ HWND WindowHandle
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhMwpLoadStage1Worker(
    _In_ PVOID Parameter
    );

VOID PhMwpInvokePrepareEarlyExit(
    _In_ HWND WindowHandle
    );

VOID PhMwpInvokeActivateWindow(
    _In_ BOOLEAN Toggle
    );

VOID PhMwpInvokeSelectTabPage(
    _In_ PVOID Parameter
    );

VOID PhMwpInvokeSelectServiceItem(
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

VOID PhMwpInvokeSelectNetworkItem(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

VOID PhMwpInvokeUpdateWindowFont(
    _In_opt_ PVOID Parameter
    );

VOID PhMwpInvokeUpdateWindowFontMonospace(
    _In_ HWND hwnd,
    _In_opt_ PVOID Parameter
    );

// main

LONG PhMainMessageLoop(
    VOID
    );

VOID PhInitializePreviousInstance(
    VOID
    );

VOID PhActivatePreviousInstance(
    VOID
    );

VOID PhInitializeCommonControls(
    VOID
    );

VOID PhInitializeSuperclassControls( // delayhook.c
    VOID
    );

NTSTATUS PhInitializeAppSystem(
    VOID
    );

VOID PhInitializeAppSettings(
    VOID
    );

VOID PhpProcessStartupParameters(
    VOID
    );

VOID PhpEnablePrivileges(
    VOID
    );

VOID PhEnableTerminationPolicy(
    _In_ BOOLEAN Enabled
    );

NTSTATUS PhInitializeDirectoryPolicy(
    VOID
    );

NTSTATUS PhInitializeExceptionPolicy(
    VOID
    );

NTSTATUS PhInitializeNamespacePolicy(
    VOID
    );

NTSTATUS PhInitializeMitigationPolicy(
    VOID
    );

NTSTATUS PhInitializeComPolicy(
    VOID
    );

NTSTATUS PhInitializeTimerPolicy(
    VOID
    );

// Event handlers

VOID PhMwpOnDestroy(
    _In_ HWND WindowHandle
    );

VOID PhMwpOnEndSession(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN SessionEnding,
    _In_ ULONG Reason
    );

VOID PhMwpOnSettingChange(
    _In_ HWND WindowHandle,
    _In_opt_ ULONG Action,
    _In_opt_ PCWSTR Metric
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
    _In_ HWND WindowHandle,
    _In_ UINT State,
    _In_ LONG Width,
    _In_ LONG Height
    );

VOID PhMwpOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    );

VOID PhMwpOnSetFocus(
    _In_ HWND WindowHandle
    );

_Success_(return)
BOOLEAN PhMwpOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    );

VOID PhMwpOnDeviceChanged(
    _In_ HWND WindowHandle,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhMwpOnDpiChanged(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDpi
    );

LRESULT PhMwpOnUserMessage(
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
    _In_ HWND WindowHandle
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

BOOLEAN PhMwpExecuteNotificationSettingsMenuCommand(
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
    _In_ LONG OldIndex
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
    _In_ PCWSTR Name,
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

typedef struct _PH_MAIN_TAB_PAGE *PPH_MAIN_TAB_PAGE;
typedef struct _PH_PROVIDER_EVENT_QUEUE PH_PROVIDER_EVENT_QUEUE, *PPH_PROVIDER_EVENT_QUEUE;

// Processes

extern PPH_MAIN_TAB_PAGE PhMwpProcessesPage;
extern HWND PhMwpProcessTreeNewHandle;
extern HWND PhMwpSelectedProcessWindowHandle;
extern BOOLEAN PhMwpSelectedProcessVirtualizationEnabled;
extern PH_PROVIDER_EVENT_QUEUE PhMwpProcessEventQueue;

BOOLEAN PhMwpProcessesPageCallback(
    _In_ PPH_MAIN_TAB_PAGE Page,
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
    _In_ HWND WindowHandle
    );

VOID PhMwpToggleMicrosoftProcessTreeFilter(
    VOID
    );

BOOLEAN PhMwpSignedProcessTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

BOOLEAN PhMwpMicrosoftProcessTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

BOOLEAN PhMwpExecuteProcessPriorityClassCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Id,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

BOOLEAN PhMwpExecuteProcessIoPriorityCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Id,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

VOID PhMwpSetProcessMenuPriorityChecks(
    _In_ PPH_EMENU Menu,
    _In_opt_ HANDLE ProcessId,
    _In_ BOOLEAN SetPriority,
    _In_ BOOLEAN SetIoPriority,
    _In_ BOOLEAN SetPagePriority
    );

VOID PhMwpInitializeProcessMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhMwpProcessAddedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhMwpProcessModifiedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhMwpProcessRemovedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhMwpProcessesUpdatedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

VOID PhMwpOnProcessesUpdated(
    _In_ ULONG RunId
    );

// Services

extern PPH_MAIN_TAB_PAGE PhMwpServicesPage;
extern HWND PhMwpServiceTreeNewHandle;
extern PH_PROVIDER_EVENT_QUEUE PhMwpServiceEventQueue;

BOOLEAN PhMwpServicesPageCallback(
    _In_ PPH_MAIN_TAB_PAGE Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
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

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhMwpServiceAddedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhMwpServiceModifiedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhMwpServiceRemovedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhMwpServicesUpdatedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

VOID PhMwpOnServicesUpdated(
    _In_ ULONG RunId
    );

// Network

extern PPH_MAIN_TAB_PAGE PhMwpNetworkPage;
extern HWND PhMwpNetworkTreeNewHandle;
extern PH_PROVIDER_EVENT_QUEUE PhMwpNetworkEventQueue;

BOOLEAN PhMwpNetworkPageCallback(
    _In_ PPH_MAIN_TAB_PAGE Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
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

_Function_class_(PH_CALLBACK_FUNCTION)
VOID PhMwpNetworkItemAddedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID PhMwpNetworkItemModifiedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID PhMwpNetworkItemRemovedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID PhMwpNetworkItemsUpdatedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

VOID PhMwpOnNetworkItemsUpdated(
    _In_ ULONG RunId
    );

// Devices

VOID PhMwpInitializeDeviceNotifications(
    VOID
    );

#endif
