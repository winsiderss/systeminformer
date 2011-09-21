#ifndef MAINWNDP_H
#define MAINWNDP_H

LRESULT CALLBACK PhMwpWndProc(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

// Initialization

BOOLEAN PhMwpInitializeWindowClass(
    VOID
    );

VOID PhMwpInitializeProviders(
    VOID
    );

VOID PhMwpInitializeControls(
    VOID
    );

NTSTATUS PhMwpDelayedLoadFunction(
    __in PVOID Parameter
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
    __in ULONG Id
    );

VOID PhMwpOnShowWindow(
    __in BOOLEAN Showing,
    __in ULONG State
    );

BOOLEAN PhMwpOnSysCommand(
    __in ULONG Type,
    __in LONG CursorScreenX,
    __in LONG CursorScreenY
    );

VOID PhMwpOnMenuCommand(
    __in ULONG Index,
    __in HMENU Menu
    );

VOID PhMwpOnInitMenuPopup(
    __in HMENU Menu,
    __in ULONG Index,
    __in BOOLEAN IsWindowMenu
    );

VOID PhMwpOnSize(
    VOID
    );

VOID PhMwpOnSizing(
    __in ULONG Edge,
    __in PRECT DragRectangle
    );

VOID PhMwpOnSetFocus(
    VOID
    );

BOOLEAN PhMwpOnNotify(
    __in NMHDR *Header,
    __out LRESULT *Result
    );

VOID PhMwpOnWtsSessionChange(
    __in ULONG Reason,
    __in ULONG SessionId
    );

ULONG_PTR PhMwpOnUserMessage(
    __in ULONG Message,
    __in ULONG_PTR WParam,
    __in ULONG_PTR LParam
    );

// Callbacks

VOID NTAPI PhMwpProcessAddedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpProcessModifiedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpProcessRemovedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpProcessesUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpProcessesUpdatedForIconsHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpServiceAddedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpServiceModifiedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpServiceRemovedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpServicesUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpNetworkItemAddedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpNetworkItemModifiedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpNetworkItemRemovedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI PhMwpNetworkItemsUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID PhMwpApplyLayoutPadding(
    __inout PRECT Rect,
    __in PRECT Padding
    );

VOID PhMwpLayout(
    __inout HDWP *DeferHandle
    );

VOID PhMwpSetCheckOpacityMenu(
    __in BOOLEAN AssumeAllUnchecked,
    __in ULONG Opacity
    );

VOID PhMwpSetWindowOpacity(
    __in ULONG Opacity
    );

BOOLEAN PhMwpExecuteComputerCommand(
    __in ULONG Id
    );

// Main menu

VOID PhMwpInitializeMainMenu(
    __in HMENU Menu
    );

VOID PhMwpDispatchMenuCommand(
    __in HMENU MenuHandle,
    __in ULONG ItemIndex,
    __in ULONG ItemId,
    __in ULONG_PTR ItemData
    );

ULONG_PTR PhMwpAddPluginMenuItem(
    __in PPH_PLUGIN Plugin,
    __in HMENU ParentMenu,
    __in_opt PWSTR InsertAfter,
    __in ULONG Flags,
    __in ULONG Id,
    __in PWSTR Text,
    __in_opt PVOID Context
    );

// Tab control

VOID PhMwpLayoutTabControl(
    __inout HDWP *DeferHandle
    );

VOID PhMwpNotifyTabControl(
    __in NMHDR *Header
    );

VOID PhMwpSelectionChangedTabControl(
    __in ULONG OldIndex
    );

PPH_ADDITIONAL_TAB_PAGE PhMwpAddTabPage(
    __in PPH_ADDITIONAL_TAB_PAGE TabPage
    );

VOID PhMwpSelectTabPage(
    __in ULONG Index
    );

// Notifications

VOID PhMwpAddIconProcesses(
    __in PPH_EMENU_ITEM Menu,
    __in ULONG NumberOfProcesses
    );

VOID PhMwpShowIconContextMenu(
    __in POINT Location
    );

BOOLEAN PhMwpPluginNotifyEvent(
    __in ULONG Type,
    __in PVOID Parameter
    );

// Processes

VOID PhMwpShowProcessProperties(
    __in PPH_PROCESS_ITEM ProcessItem
    );

BOOLEAN PhMwpCurrentUserProcessTreeFilter(
    __in PPH_PROCESS_NODE ProcessNode,
    __in_opt PVOID Context
    );

BOOLEAN PhMwpSignedProcessTreeFilter(
    __in PPH_PROCESS_NODE ProcessNode,
    __in_opt PVOID Context
    );

BOOLEAN PhMwpExecuteProcessPriorityCommand(
    __in ULONG Id,
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMwpSetProcessMenuPriorityChecks(
    __in PPH_EMENU Menu,
    __in PPH_PROCESS_ITEM Process,
    __in BOOLEAN SetPriority,
    __in BOOLEAN SetIoPriority,
    __in BOOLEAN SetPagePriority
    );

VOID PhMwpInitializeProcessMenu(
    __in PPH_EMENU Menu,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

VOID PhMwpOnProcessAdded(
    __in __assumeRefs(1) PPH_PROCESS_ITEM ProcessItem,
    __in ULONG RunId
    );

VOID PhMwpOnProcessModified(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMwpOnProcessRemoved(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMwpOnProcessesUpdated(
    VOID
    );

// Services

VOID PhMwpNeedServiceTreeList(
    VOID
    );

VOID PhMwpInitializeServiceMenu(
    __in PPH_EMENU Menu,
    __in PPH_SERVICE_ITEM *Services,
    __in ULONG NumberOfServices
    );

VOID PhMwpOnServiceAdded(
    __in __assumeRefs(1) PPH_SERVICE_ITEM ServiceItem,
    __in ULONG RunId
    );

VOID PhMwpOnServiceModified(
    __in PPH_SERVICE_MODIFIED_DATA ServiceModifiedData
    );

VOID PhMwpOnServiceRemoved(
    __in PPH_SERVICE_ITEM ServiceItem
    );

VOID PhMwpOnServicesUpdated(
    VOID
    );

// Network

VOID PhMwpNeedNetworkTreeList(
    VOID
    );

VOID PhMwpInitializeNetworkMenu(
    __in PPH_EMENU Menu,
    __in PPH_NETWORK_ITEM *NetworkItems,
    __in ULONG NumberOfNetworkItems
    );

VOID PhMwpOnNetworkItemAdded(
    __in ULONG RunId,
    __in __assumeRefs(1) PPH_NETWORK_ITEM NetworkItem
    );

VOID PhMwpOnNetworkItemModified(
    __in PPH_NETWORK_ITEM NetworkItem
    );

VOID PhMwpOnNetworkItemRemoved(
    __in PPH_NETWORK_ITEM NetworkItem
    );

VOID PhMwpOnNetworkItemsUpdated(
    VOID
    );

// Users

VOID PhMwpUpdateUsersMenu(
    VOID
    );

#endif
