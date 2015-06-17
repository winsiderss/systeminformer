#ifndef PH_MAINWNDP_H
#define PH_MAINWNDP_H

#define PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_1 250
#define PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_2 750
#define PH_FLUSH_PROCESS_QUERY_DATA_INTERVAL_LONG_TERM 1000

#define TIMER_FLUSH_PROCESS_QUERY_DATA 1

LRESULT CALLBACK PhMwpWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// Initialization

BOOLEAN PhMwpInitializeWindowClass(
    VOID
    );

VOID PhMwpInitializeProviders(
    VOID
    );

VOID PhMwpApplyUpdateInterval(
    _In_ ULONG Interval
    );

VOID PhMwpInitializeControls(
    VOID
    );

NTSTATUS PhMwpDelayedLoadFunction(
    _In_ PVOID Parameter
    );

PPH_STRING PhMwpFindDbghelpPath(
    VOID
    );

// Event handlers

VOID PhMwpOnDestroy(
    VOID
    );

VOID PhMwpOnEndSession(
    VOID
    );

VOID PhMwpOnSettingChange(
    VOID
    );

VOID PhMwpOnCommand(
    _In_ ULONG Id
    );

VOID PhMwpOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    );

BOOLEAN PhMwpOnSysCommand(
    _In_ ULONG Type,
    _In_ LONG CursorScreenX,
    _In_ LONG CursorScreenY
    );

VOID PhMwpOnMenuCommand(
    _In_ ULONG Index,
    _In_ HMENU Menu
    );

VOID PhMwpOnInitMenuPopup(
    _In_ HMENU Menu,
    _In_ ULONG Index,
    _In_ BOOLEAN IsWindowMenu
    );

VOID PhMwpOnSize(
    VOID
    );

VOID PhMwpOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    );

VOID PhMwpOnSetFocus(
    VOID
    );

VOID PhMwpOnTimer(
    _In_ ULONG Id
    );

BOOLEAN PhMwpOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    );

VOID PhMwpOnWtsSessionChange(
    _In_ ULONG Reason,
    _In_ ULONG SessionId
    );

ULONG_PTR PhMwpOnUserMessage(
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    );

// Callbacks

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

VOID NTAPI PhMwpNetworkItemAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI PhMwpNetworkItemModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI PhMwpNetworkItemRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI PhMwpNetworkItemsUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

// Settings

VOID PhMwpLoadSettings(
    VOID
    );

VOID PhMwpSaveSettings(
    VOID
    );

VOID PhMwpSaveWindowSettings(
    VOID
    );

// Misc.

VOID PhMwpSymInitHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

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

VOID PhMwpSetCheckOpacityMenu(
    _In_ BOOLEAN AssumeAllUnchecked,
    _In_ ULONG Opacity
    );

VOID PhMwpSetupComputerMenu(
    _In_ PPH_EMENU_ITEM Root
    );

BOOLEAN PhMwpExecuteComputerCommand(
    _In_ ULONG Id
    );

VOID PhMwpActivateWindow(
    _In_ BOOLEAN Toggle
    );

// Main menu

VOID PhMwpInitializeMainMenu(
    _In_ HMENU Menu
    );

VOID PhMwpDispatchMenuCommand(
    _In_ HMENU MenuHandle,
    _In_ ULONG ItemIndex,
    _In_ ULONG ItemId,
    _In_ ULONG_PTR ItemData
    );

ULONG_PTR PhMwpLegacyAddPluginMenuItem(
    _In_ PPH_ADDMENUITEM AddMenuItem
    );

HBITMAP PhMwpGetShieldBitmap(
    VOID
    );

VOID PhMwpInitializeSubMenu(
    _In_ PPH_EMENU Menu,
    _In_ ULONG Index
    );

PPH_EMENU_ITEM PhMwpFindTrayIconsMenuItem(
    _In_ PPH_EMENU Menu
    );

VOID PhMwpInitializeSectionMenuItems(
    _In_ PPH_EMENU Menu,
    _In_ ULONG StartIndex
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

PPH_ADDITIONAL_TAB_PAGE PhMwpAddTabPage(
    _In_ PPH_ADDITIONAL_TAB_PAGE TabPage
    );

VOID PhMwpSelectTabPage(
    _In_ ULONG Index
    );

INT PhMwpFindTabPageIndex(
    _In_ PWSTR Text
    );

// Notifications

VOID PhMwpAddIconProcesses(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG NumberOfProcesses
    );

VOID PhMwpShowIconContextMenu(
    _In_ POINT Location
    );

VOID PhMwpClearLastNotificationDetails(
    VOID
    );

BOOLEAN PhMwpPluginNotifyEvent(
    _In_ ULONG Type,
    _In_ PVOID Parameter
    );

// Processes

VOID PhMwpShowProcessProperties(
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

BOOLEAN PhMwpCurrentUserProcessTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
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

VOID PhMwpOnProcessAdded(
    _In_ _Assume_refs_(1) PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG RunId
    );

VOID PhMwpOnProcessModified(
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMwpOnProcessRemoved(
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMwpOnProcessesUpdated(
    VOID
    );

// Services

VOID PhMwpNeedServiceTreeList(
    VOID
    );

BOOLEAN PhMwpDriverServiceTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

VOID PhMwpInitializeServiceMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPH_SERVICE_ITEM *Services,
    _In_ ULONG NumberOfServices
    );

VOID PhMwpOnServiceAdded(
    _In_ _Assume_refs_(1) PPH_SERVICE_ITEM ServiceItem,
    _In_ ULONG RunId
    );

VOID PhMwpOnServiceModified(
    _In_ PPH_SERVICE_MODIFIED_DATA ServiceModifiedData
    );

VOID PhMwpOnServiceRemoved(
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

VOID PhMwpOnServicesUpdated(
    VOID
    );

// Network

VOID PhMwpNeedNetworkTreeList(
    VOID
    );

BOOLEAN PhMwpCurrentUserNetworkTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

BOOLEAN PhMwpSignedNetworkTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

VOID PhMwpInitializeNetworkMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPH_NETWORK_ITEM *NetworkItems,
    _In_ ULONG NumberOfNetworkItems
    );

VOID PhMwpOnNetworkItemAdded(
    _In_ ULONG RunId,
    _In_ _Assume_refs_(1) PPH_NETWORK_ITEM NetworkItem
    );

VOID PhMwpOnNetworkItemModified(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

VOID PhMwpOnNetworkItemRemoved(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

VOID PhMwpOnNetworkItemsUpdated(
    VOID
    );

// Users

VOID PhMwpUpdateUsersMenu(
    VOID
    );

#endif
