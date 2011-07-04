#ifndef PH_PHAPPPUB_H
#define PH_PHAPPPUB_H

// Process Hacker application functions exposed to plugins

#ifdef __cplusplus
extern "C" {
#endif 

// main

PHAPPAPI extern HFONT PhApplicationFont;

PHAPPAPI
VOID
NTAPI
PhRegisterDialog(
    __in HWND DialogWindowHandle
    );

PHAPPAPI
VOID
NTAPI
PhUnregisterDialog(
    __in HWND DialogWindowHandle
    );

// Common state highlighting support

typedef struct _PH_SH_STATE
{
    PH_ITEM_STATE State;
    ULONG_PTR Private1;
    ULONG Private2;
} PH_SH_STATE, *PPH_SH_STATE;

// uimodels

typedef struct _PH_PROCESS_NODE
{
    PH_TREENEW_NODE Node;

    PH_HASH_ENTRY HashEntry;

    PH_SH_STATE ShState;

    HANDLE ProcessId;
    PPH_PROCESS_ITEM ProcessItem;

    // Other members are not exposed.
} PH_PROCESS_NODE, *PPH_PROCESS_NODE;

typedef struct _PH_SERVICE_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    PPH_SERVICE_ITEM ServiceItem;

    // Other members are not exposed.
} PH_SERVICE_NODE, *PPH_SERVICE_NODE;

typedef struct _PH_MODULE_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    PPH_MODULE_ITEM ModuleItem;

    // Other members are not exposed.
} PH_MODULE_NODE, *PPH_MODULE_NODE;

// proctree

PHAPPAPI
PPH_PROCESS_NODE
NTAPI
PhFindProcessNode(
   __in HANDLE ProcessId
   );

PHAPPAPI
VOID
NTAPI
PhUpdateProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    );

PHAPPAPI
PPH_PROCESS_ITEM
NTAPI
PhGetSelectedProcessItem();

PHAPPAPI
VOID
NTAPI
PhGetSelectedProcessItems(
    __out PPH_PROCESS_ITEM **Processes,
    __out PULONG NumberOfProcesses
    );

PHAPPAPI
VOID
NTAPI
PhDeselectAllProcessNodes();

PHAPPAPI
VOID
NTAPI
PhInvalidateAllProcessNodes();

PHAPPAPI
VOID
NTAPI
PhSelectAndEnsureVisibleProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    );

typedef BOOLEAN (NTAPI *PPH_PROCESS_TREE_FILTER)(
    __in PPH_PROCESS_NODE ProcessNode,
    __in_opt PVOID Context
    );

typedef struct _PH_PROCESS_TREE_FILTER_ENTRY *PPH_PROCESS_TREE_FILTER_ENTRY;

PHAPPAPI
PPH_PROCESS_TREE_FILTER_ENTRY
NTAPI
PhAddProcessTreeFilter(
    __in PPH_PROCESS_TREE_FILTER Filter,
    __in_opt PVOID Context
    );

PHAPPAPI
VOID
NTAPI
PhRemoveProcessTreeFilter(
    __in PPH_PROCESS_TREE_FILTER_ENTRY Entry
    );

PHAPPAPI
VOID
NTAPI
PhApplyProcessTreeFilters();

// srvlist

PHAPPAPI
PPH_SERVICE_NODE
NTAPI
PhFindServiceNode(
    __in PPH_SERVICE_ITEM ServiceItem
    );

PHAPPAPI
VOID
NTAPI
PhUpdateServiceNode(
    __in PPH_SERVICE_NODE ServiceNode
    );

PHAPPAPI
PPH_SERVICE_ITEM
NTAPI
PhGetSelectedServiceItem();

PHAPPAPI
VOID
NTAPI
PhGetSelectedServiceItems(
    __out PPH_SERVICE_ITEM **Services,
    __out PULONG NumberOfServices
    );

PHAPPAPI
VOID
NTAPI
PhDeselectAllServiceNodes();

PHAPPAPI
VOID
NTAPI
PhSelectAndEnsureVisibleServiceNode(
    __in PPH_SERVICE_NODE ServiceNode
    );

// appsup

PHAPPAPI
BOOLEAN
NTAPI
PhGetProcessIsSuspended(
    __in PSYSTEM_PROCESS_INFORMATION Process
    );

typedef enum _PH_KNOWN_PROCESS_TYPE
{
    UnknownProcessType,
    SystemProcessType, // ntoskrnl/ntkrnlpa/...
    SessionManagerProcessType, // smss
    WindowsSubsystemProcessType, // csrss
    WindowsStartupProcessType, // wininit
    ServiceControlManagerProcessType, // services
    LocalSecurityAuthorityProcessType, // lsass
    LocalSessionManagerProcessType, // lsm
    WindowsLogonProcessType, // winlogon
    ServiceHostProcessType, // svchost
    RunDllAsAppProcessType, // rundll32
    ComSurrogateProcessType, // dllhost
    TaskHostProcessType, // taskeng, taskhost
    ExplorerProcessType, // explorer
    MaximumProcessType,
    KnownProcessTypeMask = 0xffff,

    KnownProcessWow64 = 0x20000
} PH_KNOWN_PROCESS_TYPE, *PPH_KNOWN_PROCESS_TYPE;

PHAPPAPI
NTSTATUS
NTAPI
PhGetProcessKnownType(
    __in HANDLE ProcessHandle,
    __out PH_KNOWN_PROCESS_TYPE *KnownProcessType
    );

typedef union _PH_KNOWN_PROCESS_COMMAND_LINE
{
    struct
    {
        PPH_STRING GroupName;
    } ServiceHost;
    struct
    {
        PPH_STRING FileName;
        PPH_STRING ProcedureName;
    } RunDllAsApp;
    struct
    {
        GUID Guid;
        PPH_STRING Name; // optional
        PPH_STRING FileName; // optional
    } ComSurrogate;
} PH_KNOWN_PROCESS_COMMAND_LINE, *PPH_KNOWN_PROCESS_COMMAND_LINE;

PHAPPAPI
BOOLEAN
NTAPI
PhaGetProcessKnownCommandLine(
    __in PPH_STRING CommandLine,
    __in PH_KNOWN_PROCESS_TYPE KnownProcessType,
    __out PPH_KNOWN_PROCESS_COMMAND_LINE KnownCommandLine
    );

PHAPPAPI
VOID
NTAPI
PhSearchOnlineString(
    __in HWND hWnd,
    __in PWSTR String
    );

PHAPPAPI
VOID
NTAPI
PhShellExecuteUserString(
    __in HWND hWnd,
    __in PWSTR Setting,
    __in PWSTR String,
    __in BOOLEAN UseShellExecute,
    __in_opt PWSTR ErrorMessage
    );

PHAPPAPI
VOID
NTAPI
PhLoadSymbolProviderOptions(
    __inout PPH_SYMBOL_PROVIDER SymbolProvider
    );

PHAPPAPI
VOID
NTAPI
PhSetExtendedListViewWithSettings(
    __in HWND hWnd
    );

PHAPPAPI
VOID
NTAPI
PhCopyListViewInfoTip(
    __inout LPNMLVGETINFOTIP GetInfoTip,
    __in PPH_STRINGREF Tip
    );

PHAPPAPI
VOID
NTAPI
PhCopyListView(
    __in HWND ListViewHandle
    );

PHAPPAPI
VOID
NTAPI
PhHandleListViewNotifyForCopy(
    __in LPARAM lParam,
    __in HWND ListViewHandle
    );

PHAPPAPI
BOOLEAN
NTAPI
PhGetListViewContextMenuPoint(
    __in HWND ListViewHandle,
    __out PPOINT Point
    );

PHAPPAPI
VOID
NTAPI
PhLoadWindowPlacementFromSetting(
    __in_opt PWSTR PositionSettingName,
    __in_opt PWSTR SizeSettingName,
    __in HWND WindowHandle
    );

PHAPPAPI
VOID
NTAPI
PhSaveWindowPlacementToSetting(
    __in_opt PWSTR PositionSettingName,
    __in_opt PWSTR SizeSettingName,
    __in HWND WindowHandle
    );

PHAPPAPI
VOID
NTAPI
PhLoadListViewColumnsFromSetting(
    __in PWSTR Name,
    __in HWND ListViewHandle
    );

PHAPPAPI
VOID
NTAPI
PhSaveListViewColumnsToSetting(
    __in PWSTR Name,
    __in HWND ListViewHandle
    );

PHAPPAPI
PPH_STRING
NTAPI
PhGetPhVersion();

PHAPPAPI
VOID
NTAPI
PhWritePhTextHeader(
    __inout PPH_FILE_STREAM FileStream
    );

// mainwnd

PHAPPAPI extern HWND PhMainWndHandle;

#define WM_PH_SHOW_PROCESS_PROPERTIES (WM_APP + 120)
#define WM_PH_DESTROY (WM_APP + 121)
#define WM_PH_SAVE_ALL_SETTINGS (WM_APP + 122)
#define WM_PH_PREPARE_FOR_EARLY_SHUTDOWN (WM_APP + 123)
#define WM_PH_CANCEL_EARLY_SHUTDOWN (WM_APP + 124)
#define WM_PH_TOGGLE_VISIBLE (WM_APP + 127)
#define WM_PH_SELECT_TAB_PAGE (WM_APP + 130)
#define WM_PH_GET_LAYOUT_PADDING (WM_APP + 131)
#define WM_PH_SET_LAYOUT_PADDING (WM_APP + 132)
#define WM_PH_SELECT_PROCESS_NODE (WM_APP + 133)
#define WM_PH_SELECT_SERVICE_ITEM (WM_APP + 134)
#define WM_PH_SELECT_NETWORK_ITEM (WM_APP + 135)
#define WM_PH_INVOKE (WM_APP + 138)
#define WM_PH_ADD_MENU_ITEM (WM_APP + 139)
#define WM_PH_ADD_TAB_PAGE (WM_APP + 140)

#define ProcessHacker_ShowProcessProperties(hWnd, ProcessItem) \
    SendMessage(hWnd, WM_PH_SHOW_PROCESS_PROPERTIES, 0, (LPARAM)(ProcessItem))
#define ProcessHacker_Destroy(hWnd) \
    SendMessage(hWnd, WM_PH_DESTROY, 0, 0)
#define ProcessHacker_SaveAllSettings(hWnd) \
    SendMessage(hWnd, WM_PH_SAVE_ALL_SETTINGS, 0, 0)
#define ProcessHacker_PrepareForEarlyShutdown(hWnd) \
    SendMessage(hWnd, WM_PH_PREPARE_FOR_EARLY_SHUTDOWN, 0, 0)
#define ProcessHacker_CancelEarlyShutdown(hWnd) \
    SendMessage(hWnd, WM_PH_CANCEL_EARLY_SHUTDOWN, 0, 0)
#define ProcessHacker_ToggleVisible(hWnd) \
    SendMessage(hWnd, WM_PH_TOGGLE_VISIBLE, 0, 0)
#define ProcessHacker_SelectTabPage(hWnd, Index) \
    SendMessage(hWnd, WM_PH_SELECT_TAB_PAGE, (WPARAM)(Index), 0)
#define ProcessHacker_GetLayoutPadding(hWnd, Rect) \
    SendMessage(hWnd, WM_PH_GET_LAYOUT_PADDING, 0, (LPARAM)(Rect))
#define ProcessHacker_SetLayoutPadding(hWnd, Rect) \
    SendMessage(hWnd, WM_PH_SET_LAYOUT_PADDING, 0, (LPARAM)(Rect))
#define ProcessHacker_SelectProcessNode(hWnd, ProcessNode) \
    SendMessage(hWnd, WM_PH_SELECT_PROCESS_NODE, 0, (LPARAM)(ProcessNode))
#define ProcessHacker_SelectServiceItem(hWnd, ServiceItem) \
    SendMessage(hWnd, WM_PH_SELECT_SERVICE_ITEM, 0, (LPARAM)(ServiceItem))
#define ProcessHacker_SelectNetworkItem(hWnd, NetworkItem) \
    SendMessage(hWnd, WM_PH_SELECT_NETWORK_ITEM, 0, (LPARAM)(NetworkItem))
#define ProcessHacker_Invoke(hWnd, Function, Parameter) \
    PostMessage(hWnd, WM_PH_INVOKE, (WPARAM)(Parameter), (LPARAM)(Function))
#define ProcessHacker_AddMenuItem(hWnd, AddMenuItem) \
    ((BOOLEAN)SendMessage(hWnd, WM_PH_ADD_MENU_ITEM, 0, (LPARAM)(AddMenuItem)))
#define ProcessHacker_AddTabPage(hWnd, TabPage) \
    SendMessage(hWnd, WM_PH_ADD_TAB_PAGE, 0, (LPARAM)(TabPage))

typedef struct _PH_ADDMENUITEM
{
    __in PVOID Plugin;
    __in ULONG Location;
    __in_opt PWSTR InsertAfter;
    __in ULONG Id;
    __in PWSTR Text;
    __in_opt PVOID Context;
} PH_ADDMENUITEM, *PPH_ADDMENUITEM;

typedef HWND (NTAPI *PPH_TAB_PAGE_CREATE_FUNCTION)(
    __in PVOID Context
    );

typedef struct _PH_ADDITIONAL_TAB_PAGE
{
    PWSTR Text;
    PVOID Context;
    PPH_TAB_PAGE_CREATE_FUNCTION CreateFunction;
    HWND WindowHandle;
    INT Index;
} PH_ADDITIONAL_TAB_PAGE, *PPH_ADDITIONAL_TAB_PAGE;

#define PH_NOTIFY_MINIMUM 0x1
#define PH_NOTIFY_PROCESS_CREATE 0x1
#define PH_NOTIFY_PROCESS_DELETE 0x2
#define PH_NOTIFY_SERVICE_CREATE 0x4
#define PH_NOTIFY_SERVICE_DELETE 0x8
#define PH_NOTIFY_SERVICE_START 0x10
#define PH_NOTIFY_SERVICE_STOP 0x20
#define PH_NOTIFY_MAXIMUM 0x40
#define PH_NOTIFY_VALID_MASK 0x3f

PHAPPAPI
VOID
NTAPI
PhShowIconNotification(
    __in PWSTR Title,
    __in PWSTR Text,
    __in ULONG Flags
    );

// procprp

typedef struct _PH_PROCESS_PROPCONTEXT PH_PROCESS_PROPCONTEXT, *PPH_PROCESS_PROPCONTEXT;

typedef struct _PH_PROCESS_PROPPAGECONTEXT
{
    PPH_PROCESS_PROPCONTEXT PropContext;
    PVOID Context;
    PROPSHEETPAGE PropSheetPage;

    BOOLEAN LayoutInitialized;
} PH_PROCESS_PROPPAGECONTEXT, *PPH_PROCESS_PROPPAGECONTEXT;

PHAPPAPI
PPH_PROCESS_PROPCONTEXT
NTAPI
PhCreateProcessPropContext(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    );

PHAPPAPI
VOID
NTAPI
PhSetSelectThreadIdProcessPropContext(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in HANDLE ThreadId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhAddProcessPropPage(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in __assumeRefs(1) PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    );

PHAPPAPI
BOOLEAN
NTAPI
PhAddProcessPropPage2(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in HPROPSHEETPAGE PropSheetPageHandle
    );

PHAPPAPI
PPH_PROCESS_PROPPAGECONTEXT
NTAPI
PhCreateProcessPropPageContextEx(
    __in_opt PVOID InstanceHandle,
    __in LPCWSTR Template,
    __in DLGPROC DlgProc,
    __in_opt PVOID Context
    );

PHAPPAPI
BOOLEAN
NTAPI
PhPropPageDlgProcHeader(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam,
    __out LPPROPSHEETPAGE *PropSheetPage,
    __out PPH_PROCESS_PROPPAGECONTEXT *PropPageContext,
    __out PPH_PROCESS_ITEM *ProcessItem
    );

PHAPPAPI
VOID
NTAPI
PhPropPageDlgProcDestroy(
    __in HWND hwndDlg
    );

#define PH_PROP_PAGE_TAB_CONTROL_PARENT ((PPH_LAYOUT_ITEM)0x1)

PHAPPAPI
PPH_LAYOUT_ITEM
NTAPI
PhAddPropPageLayoutItem(
    __in HWND hwnd,
    __in HWND Handle,
    __in PPH_LAYOUT_ITEM ParentItem,
    __in ULONG Anchor
    );

PHAPPAPI
VOID
NTAPI
PhDoPropPageLayout(
    __in HWND hwnd
    );

FORCEINLINE PPH_LAYOUT_ITEM PhBeginPropPageLayout(
    __in HWND hwndDlg,
    __in PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    if (!PropPageContext->LayoutInitialized)
    {
        return PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
            PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
    }
    else
    {
        return NULL;
    }
}

FORCEINLINE VOID PhEndPropPageLayout(
    __in HWND hwndDlg,
    __in PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    PhDoPropPageLayout(hwndDlg);
    PropPageContext->LayoutInitialized = TRUE;
}

PHAPPAPI
BOOLEAN
NTAPI
PhShowProcessProperties(
    __in PPH_PROCESS_PROPCONTEXT Context
    );

// log

#define PH_LOG_ENTRY_MESSAGE 9

typedef struct _PH_LOG_ENTRY *PPH_LOG_ENTRY;

PHAPPAPI extern PH_CALLBACK PhLoggedCallback;

PHAPPAPI
VOID
NTAPI
PhLogMessageEntry(
    __in UCHAR Type,
    __in PPH_STRING Message
    );

PHAPPAPI
PPH_STRING
NTAPI
PhFormatLogEntry(
    __in PPH_LOG_ENTRY Entry
    );

// actions

PHAPPAPI
BOOLEAN
NTAPI
PhUiLockComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiLogoffComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSleepComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiHibernateComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiRestartComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiShutdownComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiPoweroffComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiConnectSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDisconnectSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiLogoffSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiTerminateProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiTerminateTreeProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSuspendProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiResumeProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiRestartProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDebugProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiReduceWorkingSetProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetVirtualizationProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in BOOLEAN Enable
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDetachFromDebuggerProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiInjectDllProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetIoPriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG IoPriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPagePriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG PagePriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG PriorityClass
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetDepStatusProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetProtectionProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiStartService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiContinueService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiPauseService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiStopService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDeleteService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiCloseConnections(
    __in HWND hWnd,
    __in PPH_NETWORK_ITEM *Connections,
    __in ULONG NumberOfConnections
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiTerminateThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiForceTerminateThreads(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSuspendThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiResumeThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG ThreadPriorityWin32
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetIoPriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG IoPriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPagePriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG PagePriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiUnloadModule(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_MODULE_ITEM Module
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiFreeMemory(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_MEMORY_ITEM MemoryItem,
    __in BOOLEAN Free
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiCloseHandles(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM *Handles,
    __in ULONG NumberOfHandles,
    __in BOOLEAN Warn
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetAttributesHandle(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM Handle,
    __in ULONG Attributes
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDestroyHeap(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PVOID HeapHandle
    );

// affinity

PHAPPAPI
BOOLEAN
NTAPI
PhShowProcessAffinityDialog2(
    __in HWND ParentWindowHandle,
    __in ULONG_PTR AffinityMask,
    __out PULONG_PTR NewAffinityMask
    );

// chdlg

#define PH_CHOICE_DIALOG_SAVED_CHOICES 10

#define PH_CHOICE_DIALOG_CHOICE 0x0
#define PH_CHOICE_DIALOG_USER_CHOICE 0x1
#define PH_CHOICE_DIALOG_PASSWORD 0x2
#define PH_CHOICE_DIALOG_TYPE_MASK 0x3

PHAPPAPI
BOOLEAN PhaChoiceDialog(
    __in HWND ParentWindowHandle,
    __in PWSTR Title,
    __in PWSTR Message,
    __in_opt PWSTR *Choices,
    __in_opt ULONG NumberOfChoices,
    __in_opt PWSTR Option,
    __in ULONG Flags,
    __inout PPH_STRING *SelectedChoice,
    __inout_opt PBOOLEAN SelectedOption,
    __in_opt PWSTR SavedChoicesSettingName
    );

// chproc

PHAPPAPI
BOOLEAN
NTAPI
PhShowChooseProcessDialog(
    __in HWND ParentWindowHandle,
    __in PWSTR Message,
    __out PHANDLE ProcessId
    );

// procrec

PHAPPAPI
VOID
NTAPI
PhShowProcessRecordDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_RECORD Record
    );

// srvctl

#define WM_PH_SET_LIST_VIEW_SETTINGS (WM_APP + 701)

PHAPPAPI
HWND
NTAPI
PhCreateServiceListControl(
    __in HWND ParentWindowHandle,
    __in PPH_SERVICE_ITEM *Services,
    __in ULONG NumberOfServices
    );

// settings

typedef enum _PH_SETTING_TYPE
{
    StringSettingType,
    IntegerSettingType,
    IntegerPairSettingType
} PH_SETTING_TYPE, PPH_SETTING_TYPE;

PHAPPAPI
__mayRaise ULONG
NTAPI
PhGetIntegerSetting(
    __in PWSTR Name
    );

PHAPPAPI
__mayRaise PH_INTEGER_PAIR
NTAPI
PhGetIntegerPairSetting(
    __in PWSTR Name
    );

PHAPPAPI
__mayRaise PPH_STRING
NTAPI
PhGetStringSetting(
    __in PWSTR Name
    );

PHAPPAPI
__mayRaise VOID
NTAPI
PhSetIntegerSetting(
    __in PWSTR Name,
    __in ULONG Value
    );

PHAPPAPI
__mayRaise VOID
NTAPI
PhSetIntegerPairSetting(
    __in PWSTR Name,
    __in PH_INTEGER_PAIR Value
    );

PHAPPAPI
__mayRaise VOID
NTAPI
PhSetStringSetting(
    __in PWSTR Name,
    __in PWSTR Value
    );

PHAPPAPI
__mayRaise VOID
NTAPI
PhSetStringSetting2(
    __in PWSTR Name,
    __in PPH_STRINGREF Value
    );

// High-level settings creation

typedef struct _PH_CREATE_SETTING
{
    PH_SETTING_TYPE Type;
    PWSTR Name;
    PWSTR DefaultValue;
} PH_SETTING_CREATE, *PPH_SETTING_CREATE;

PHAPPAPI
VOID
NTAPI
PhAddSettings(
    __in PPH_SETTING_CREATE Settings,
    __in ULONG NumberOfSettings
    );

#ifdef __cplusplus
}
#endif

#endif
