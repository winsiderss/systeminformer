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
    _In_ HWND DialogWindowHandle
    );

PHAPPAPI
VOID
NTAPI
PhUnregisterDialog(
    _In_ HWND DialogWindowHandle
    );

typedef BOOLEAN (NTAPI *PPH_MESSAGE_LOOP_FILTER)(
    _In_ PMSG Message,
    _In_ PVOID Context
    );

PHAPPAPI
struct _PH_MESSAGE_LOOP_FILTER_ENTRY *PhRegisterMessageLoopFilter(
    _In_ PPH_MESSAGE_LOOP_FILTER Filter,
    _In_opt_ PVOID Context
    );

PHAPPAPI
VOID PhUnregisterMessageLoopFilter(
    _In_ struct _PH_MESSAGE_LOOP_FILTER_ENTRY *FilterEntry
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

    struct _PH_PROCESS_NODE *Parent;
    PPH_LIST Children;

    // ...
} PH_PROCESS_NODE, *PPH_PROCESS_NODE;

typedef struct _PH_SERVICE_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    PPH_SERVICE_ITEM ServiceItem;

    // ...
} PH_SERVICE_NODE, *PPH_SERVICE_NODE;

typedef struct _PH_NETWORK_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    PPH_NETWORK_ITEM NetworkItem;

    // ...
} PH_NETWORK_NODE, *PPH_NETWORK_NODE;

typedef struct _PH_THREAD_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    HANDLE ThreadId;
    PPH_THREAD_ITEM ThreadItem;

    // ...
} PH_THREAD_NODE, *PPH_THREAD_NODE;

typedef struct _PH_MODULE_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    PPH_MODULE_ITEM ModuleItem;

    // ...
} PH_MODULE_NODE, *PPH_MODULE_NODE;

typedef struct _PH_HANDLE_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    HANDLE Handle;
    PPH_HANDLE_ITEM HandleItem;

    // ...
} PH_HANDLE_NODE, *PPH_HANDLE_NODE;

// procprpp

typedef struct _PH_THREADS_CONTEXT
{
    PPH_THREAD_PROVIDER Provider;
    PH_CALLBACK_REGISTRATION Private1;
    PH_CALLBACK_REGISTRATION Private2;
    PH_CALLBACK_REGISTRATION Private3;
    PH_CALLBACK_REGISTRATION Private4;
    PH_CALLBACK_REGISTRATION Private5;
    PH_CALLBACK_REGISTRATION Private6;

    HWND WindowHandle;

    HWND Private7;
    HWND TreeNewHandle;

    // ...
} PH_THREADS_CONTEXT, *PPH_THREADS_CONTEXT;

typedef struct _PH_MODULES_CONTEXT
{
    PPH_MODULE_PROVIDER Provider;
    PH_PROVIDER_REGISTRATION Private1;
    PH_CALLBACK_REGISTRATION Private2;
    PH_CALLBACK_REGISTRATION Private3;
    PH_CALLBACK_REGISTRATION Private4;
    PH_CALLBACK_REGISTRATION Private5;

    HWND WindowHandle;

    HWND Private6;
    HWND TreeNewHandle;

    // ...
} PH_MODULES_CONTEXT, *PPH_MODULES_CONTEXT;

typedef struct _PH_HANDLES_CONTEXT
{
    PPH_HANDLE_PROVIDER Provider;
    PH_PROVIDER_REGISTRATION Private1;
    PH_CALLBACK_REGISTRATION Private2;
    PH_CALLBACK_REGISTRATION Private3;
    PH_CALLBACK_REGISTRATION Private4;
    PH_CALLBACK_REGISTRATION Private5;

    HWND WindowHandle;

    HWND Private6;
    HWND TreeNewHandle;

    // ...
} PH_HANDLES_CONTEXT, *PPH_HANDLES_CONTEXT;

// proctree

PHAPPAPI
struct _PH_TN_FILTER_SUPPORT *
NTAPI
PhGetFilterSupportProcessTreeList(
    VOID
    );

PHAPPAPI
PPH_PROCESS_NODE
NTAPI
PhFindProcessNode(
   _In_ HANDLE ProcessId
   );

PHAPPAPI
VOID
NTAPI
PhUpdateProcessNode(
    _In_ PPH_PROCESS_NODE ProcessNode
    );

PHAPPAPI
PPH_PROCESS_ITEM
NTAPI
PhGetSelectedProcessItem(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhGetSelectedProcessItems(
    _Out_ PPH_PROCESS_ITEM **Processes,
    _Out_ PULONG NumberOfProcesses
    );

PHAPPAPI
VOID
NTAPI
PhDeselectAllProcessNodes(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhExpandAllProcessNodes(
    _In_ BOOLEAN Expand
    );

PHAPPAPI
VOID
NTAPI
PhInvalidateAllProcessNodes(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhSelectAndEnsureVisibleProcessNode(
    _In_ PPH_PROCESS_NODE ProcessNode
    );

typedef BOOLEAN (NTAPI *PPH_PROCESS_TREE_FILTER)(
    _In_ PPH_PROCESS_NODE ProcessNode,
    _In_opt_ PVOID Context
    );

typedef struct _PH_PROCESS_TREE_FILTER_ENTRY *PPH_PROCESS_TREE_FILTER_ENTRY;

PHAPPAPI
PPH_PROCESS_TREE_FILTER_ENTRY
NTAPI
PhAddProcessTreeFilter(
    _In_ PPH_PROCESS_TREE_FILTER Filter,
    _In_opt_ PVOID Context
    );

PHAPPAPI
VOID
NTAPI
PhRemoveProcessTreeFilter(
    _In_ PPH_PROCESS_TREE_FILTER_ENTRY Entry
    );

PHAPPAPI
VOID
NTAPI
PhApplyProcessTreeFilters(
    VOID
    );

// srvlist

PHAPPAPI
struct _PH_TN_FILTER_SUPPORT *
NTAPI
PhGetFilterSupportServiceTreeList(
    VOID
    );

PHAPPAPI
PPH_SERVICE_NODE
NTAPI
PhFindServiceNode(
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

PHAPPAPI
VOID
NTAPI
PhUpdateServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    );

PHAPPAPI
PPH_SERVICE_ITEM
NTAPI
PhGetSelectedServiceItem(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhGetSelectedServiceItems(
    _Out_ PPH_SERVICE_ITEM **Services,
    _Out_ PULONG NumberOfServices
    );

PHAPPAPI
VOID
NTAPI
PhDeselectAllServiceNodes(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhSelectAndEnsureVisibleServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    );

// netlist

PHAPPAPI
struct _PH_TN_FILTER_SUPPORT *
NTAPI
PhGetFilterSupportNetworkTreeList(
    VOID
    );

PHAPPAPI
PPH_NETWORK_NODE
NTAPI
PhFindNetworkNode(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

// appsup

PHAPPAPI
BOOLEAN
NTAPI
PhGetProcessIsSuspended(
    _In_ PSYSTEM_PROCESS_INFORMATION Process
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
    TaskHostProcessType, // taskeng, taskhost, taskhostex
    ExplorerProcessType, // explorer
    UmdfHostProcessType, // wudfhost
    MaximumProcessType,
    KnownProcessTypeMask = 0xffff,

    KnownProcessWow64 = 0x20000
} PH_KNOWN_PROCESS_TYPE, *PPH_KNOWN_PROCESS_TYPE;

PHAPPAPI
NTSTATUS
NTAPI
PhGetProcessKnownType(
    _In_ HANDLE ProcessHandle,
    _Out_ PH_KNOWN_PROCESS_TYPE *KnownProcessType
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
    _In_ PPH_STRING CommandLine,
    _In_ PH_KNOWN_PROCESS_TYPE KnownProcessType,
    _Out_ PPH_KNOWN_PROCESS_COMMAND_LINE KnownCommandLine
    );

PHAPPAPI
VOID
NTAPI
PhSearchOnlineString(
    _In_ HWND hWnd,
    _In_ PWSTR String
    );

PHAPPAPI
VOID
NTAPI
PhShellExecuteUserString(
    _In_ HWND hWnd,
    _In_ PWSTR Setting,
    _In_ PWSTR String,
    _In_ BOOLEAN UseShellExecute,
    _In_opt_ PWSTR ErrorMessage
    );

PHAPPAPI
VOID
NTAPI
PhLoadSymbolProviderOptions(
    _Inout_ PPH_SYMBOL_PROVIDER SymbolProvider
    );

PHAPPAPI
VOID
NTAPI
PhCopyListViewInfoTip(
    _Inout_ LPNMLVGETINFOTIP GetInfoTip,
    _In_ PPH_STRINGREF Tip
    );

PHAPPAPI
VOID
NTAPI
PhCopyListView(
    _In_ HWND ListViewHandle
    );

PHAPPAPI
VOID
NTAPI
PhHandleListViewNotifyForCopy(
    _In_ LPARAM lParam,
    _In_ HWND ListViewHandle
    );

PHAPPAPI
BOOLEAN
NTAPI
PhGetListViewContextMenuPoint(
    _In_ HWND ListViewHandle,
    _Out_ PPOINT Point
    );

PHAPPAPI
VOID
NTAPI
PhLoadWindowPlacementFromSetting(
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName,
    _In_ HWND WindowHandle
    );

PHAPPAPI
VOID
NTAPI
PhSaveWindowPlacementToSetting(
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName,
    _In_ HWND WindowHandle
    );

PHAPPAPI
VOID
NTAPI
PhLoadListViewColumnsFromSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    );

PHAPPAPI
VOID
NTAPI
PhSaveListViewColumnsToSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    );

PHAPPAPI
PPH_STRING
NTAPI
PhGetPhVersion(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhGetPhVersionNumbers(
    _Out_opt_ PULONG MajorVersion,
    _Out_opt_ PULONG MinorVersion,
    _Reserved_ PULONG Reserved,
    _Out_opt_ PULONG RevisionNumber
    );

PHAPPAPI
VOID
NTAPI
PhWritePhTextHeader(
    _Inout_ PPH_FILE_STREAM FileStream
    );

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

PHAPPAPI
VOID
NTAPI
PhInitializeTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    );

PHAPPAPI
BOOLEAN
NTAPI
PhHandleTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    );

PHAPPAPI
VOID
NTAPI
PhDeleteTreeNewColumnMenu(
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

PHAPPAPI
PPH_TN_FILTER_ENTRY
NTAPI
PhAddTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_FUNCTION Filter,
    _In_opt_ PVOID Context
    );

PHAPPAPI
VOID
NTAPI
PhRemoveTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_ENTRY Entry
    );

PHAPPAPI
VOID
NTAPI
PhApplyTreeNewFilters(
    _In_ PPH_TN_FILTER_SUPPORT Support
    );

// mainwnd

#define PH_MAINWND_CLASS_NAME L"ProcessHacker"

PHAPPAPI extern HWND PhMainWndHandle;

#define WM_PH_SHOW_PROCESS_PROPERTIES (WM_APP + 120)
#define WM_PH_DESTROY (WM_APP + 121)
#define WM_PH_SAVE_ALL_SETTINGS (WM_APP + 122)
#define WM_PH_PREPARE_FOR_EARLY_SHUTDOWN (WM_APP + 123)
#define WM_PH_CANCEL_EARLY_SHUTDOWN (WM_APP + 124)
#define WM_PH_TOGGLE_VISIBLE (WM_APP + 127)
#define WM_PH_SELECT_TAB_PAGE (WM_APP + 130)
#define WM_PH_GET_CALLBACK_LAYOUT_PADDING (WM_APP + 131)
#define WM_PH_INVALIDATE_LAYOUT_PADDING (WM_APP + 132)
#define WM_PH_SELECT_PROCESS_NODE (WM_APP + 133)
#define WM_PH_SELECT_SERVICE_ITEM (WM_APP + 134)
#define WM_PH_SELECT_NETWORK_ITEM (WM_APP + 135)
#define WM_PH_INVOKE (WM_APP + 138)
#define WM_PH_ADD_MENU_ITEM (WM_APP + 139)
#define WM_PH_ADD_TAB_PAGE (WM_APP + 140)
#define WM_PH_REFRESH (WM_APP + 141)
#define WM_PH_GET_UPDATE_AUTOMATICALLY (WM_APP + 142)
#define WM_PH_SET_UPDATE_AUTOMATICALLY (WM_APP + 143)

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
#define ProcessHacker_ToggleVisible(hWnd, AlwaysShow) \
    SendMessage(hWnd, WM_PH_TOGGLE_VISIBLE, (WPARAM)(AlwaysShow), 0)
#define ProcessHacker_SelectTabPage(hWnd, Index) \
    SendMessage(hWnd, WM_PH_SELECT_TAB_PAGE, (WPARAM)(Index), 0)
#define ProcessHacker_GetCallbackLayoutPadding(hWnd) \
    ((PPH_CALLBACK)SendMessage(hWnd, WM_PH_GET_CALLBACK_LAYOUT_PADDING, 0, 0))
#define ProcessHacker_InvalidateLayoutPadding(hWnd) \
    SendMessage(hWnd, WM_PH_INVALIDATE_LAYOUT_PADDING, 0, 0)
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
    ((PPH_ADDITIONAL_TAB_PAGE)SendMessage(hWnd, WM_PH_ADD_TAB_PAGE, 0, (LPARAM)(TabPage)))
#define ProcessHacker_Refresh(hWnd) \
    SendMessage(hWnd, WM_PH_REFRESH, 0, 0)
#define ProcessHacker_GetUpdateAutomatically(hWnd) \
    ((BOOLEAN)SendMessage(hWnd, WM_PH_GET_UPDATE_AUTOMATICALLY, 0, 0))
#define ProcessHacker_SetUpdateAutomatically(hWnd, Value) \
    SendMessage(hWnd, WM_PH_SET_UPDATE_AUTOMATICALLY, (WPARAM)(Value), 0)

typedef struct _PH_LAYOUT_PADDING_DATA
{
    RECT Padding;
} PH_LAYOUT_PADDING_DATA, *PPH_LAYOUT_PADDING_DATA;

typedef struct _PH_ADDMENUITEM
{
    _In_ PVOID Plugin;
    _In_ ULONG Location;
    _In_opt_ PWSTR InsertAfter;
    _In_ ULONG Flags;
    _In_ ULONG Id;
    _In_ PWSTR Text;
    _In_opt_ PVOID Context;
} PH_ADDMENUITEM, *PPH_ADDMENUITEM;

typedef HWND (NTAPI *PPH_TAB_PAGE_CREATE_FUNCTION)(
    _In_ PVOID Context
    );

typedef VOID (NTAPI *PPH_TAB_PAGE_CALLBACK_FUNCTION)(
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3,
    _In_ PVOID Context
    );

typedef struct _PH_ADDITIONAL_TAB_PAGE
{
    PWSTR Text;
    PVOID Context;
    PPH_TAB_PAGE_CREATE_FUNCTION CreateFunction;
    HWND WindowHandle;
    INT Index;
    PPH_TAB_PAGE_CALLBACK_FUNCTION SelectionChangedCallback;
    PPH_TAB_PAGE_CALLBACK_FUNCTION SaveContentCallback;
    PPH_TAB_PAGE_CALLBACK_FUNCTION FontChangedCallback;
    PPH_TAB_PAGE_CALLBACK_FUNCTION InitializeSectionMenuItemsCallback;
    PVOID Reserved[3];
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
    _In_ PWSTR Title,
    _In_ PWSTR Text,
    _In_ ULONG Flags
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
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

PHAPPAPI
VOID
NTAPI
PhSetSelectThreadIdProcessPropContext(
    _Inout_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ HANDLE ThreadId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhAddProcessPropPage(
    _Inout_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ _Assume_refs_(1) PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    );

PHAPPAPI
BOOLEAN
NTAPI
PhAddProcessPropPage2(
    _Inout_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ HPROPSHEETPAGE PropSheetPageHandle
    );

PHAPPAPI
PPH_PROCESS_PROPPAGECONTEXT
NTAPI
PhCreateProcessPropPageContextEx(
    _In_opt_ PVOID InstanceHandle,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    );

PHAPPAPI
BOOLEAN
NTAPI
PhPropPageDlgProcHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam,
    _Out_ LPPROPSHEETPAGE *PropSheetPage,
    _Out_ PPH_PROCESS_PROPPAGECONTEXT *PropPageContext,
    _Out_ PPH_PROCESS_ITEM *ProcessItem
    );

PHAPPAPI
VOID
NTAPI
PhPropPageDlgProcDestroy(
    _In_ HWND hwndDlg
    );

#define PH_PROP_PAGE_TAB_CONTROL_PARENT ((PPH_LAYOUT_ITEM)0x1)

PHAPPAPI
PPH_LAYOUT_ITEM
NTAPI
PhAddPropPageLayoutItem(
    _In_ HWND hwnd,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    );

PHAPPAPI
VOID
NTAPI
PhDoPropPageLayout(
    _In_ HWND hwnd
    );

FORCEINLINE PPH_LAYOUT_ITEM PhBeginPropPageLayout(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_PROPPAGECONTEXT PropPageContext
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
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    PhDoPropPageLayout(hwndDlg);
    PropPageContext->LayoutInitialized = TRUE;
}

PHAPPAPI
BOOLEAN
NTAPI
PhShowProcessProperties(
    _In_ PPH_PROCESS_PROPCONTEXT Context
    );

// sysinfo

typedef enum _PH_SYSINFO_VIEW_TYPE
{
    SysInfoSummaryView,
    SysInfoSectionView
} PH_SYSINFO_VIEW_TYPE;

typedef VOID (NTAPI *PPH_SYSINFO_COLOR_SETUP_FUNCTION)(
    _Out_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ COLORREF Color1,
    _In_ COLORREF Color2
    );

typedef struct _PH_SYSINFO_PARAMETERS
{
    HWND SysInfoWindowHandle;
    HWND ContainerWindowHandle;

    HFONT Font;
    HFONT MediumFont;
    HFONT LargeFont;
    ULONG FontHeight;
    ULONG FontAverageWidth;
    ULONG MediumFontHeight;
    ULONG MediumFontAverageWidth;
    COLORREF GraphBackColor;
    COLORREF PanelForeColor;
    PPH_SYSINFO_COLOR_SETUP_FUNCTION ColorSetupFunction;

    // ...
} PH_SYSINFO_PARAMETERS, *PPH_SYSINFO_PARAMETERS;

typedef enum _PH_SYSINFO_SECTION_MESSAGE
{
    SysInfoCreate,
    SysInfoDestroy,
    SysInfoTick,
    SysInfoViewChanging, // PH_SYSINFO_VIEW_TYPE Parameter1, PPH_SYSINFO_SECTION Parameter2
    SysInfoCreateDialog, // PPH_SYSINFO_CREATE_DIALOG Parameter1
    SysInfoGraphGetDrawInfo, // PPH_GRAPH_DRAW_INFO Parameter1
    SysInfoGraphGetTooltipText, // PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT Parameter1
    SysInfoGraphDrawPanel, // PPH_SYSINFO_DRAW_PANEL Parameter1
    MaxSysInfoMessage
} PH_SYSINFO_SECTION_MESSAGE;

typedef BOOLEAN (NTAPI *PPH_SYSINFO_SECTION_CALLBACK)(
    _In_ struct _PH_SYSINFO_SECTION *Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

typedef struct _PH_SYSINFO_CREATE_DIALOG
{
    BOOLEAN CustomCreate;

    // Parameters for default create
    PVOID Instance;
    PWSTR Template;
    DLGPROC DialogProc;
    PVOID Parameter;
} PH_SYSINFO_CREATE_DIALOG, *PPH_SYSINFO_CREATE_DIALOG;

typedef struct _PH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT
{
    ULONG Index;
    PH_STRINGREF Text;
} PH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT, *PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT;

typedef struct _PH_SYSINFO_DRAW_PANEL
{
    HDC hdc;
    RECT Rect;
    BOOLEAN CustomDraw;

    // Parameters for default draw
    PPH_STRING Title;
    PPH_STRING SubTitle;
    PPH_STRING SubTitleOverflow;
} PH_SYSINFO_DRAW_PANEL, *PPH_SYSINFO_DRAW_PANEL;

typedef struct _PH_SYSINFO_SECTION
{
    // Public

    // Initialization
    PH_STRINGREF Name;
    ULONG Flags;
    PPH_SYSINFO_SECTION_CALLBACK Callback;
    PVOID Context;
    PVOID Reserved[3];

    // State
    HWND GraphHandle;
    PH_GRAPH_STATE GraphState;
    PPH_SYSINFO_PARAMETERS Parameters;
    PVOID Reserved2[3];

    // ...
} PH_SYSINFO_SECTION, *PPH_SYSINFO_SECTION;

PHAPPAPI
VOID PhSiSetColorsGraphDrawInfo(
    _Out_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ COLORREF Color1,
    _In_ COLORREF Color2
    );

// log

#define PH_LOG_ENTRY_MESSAGE 9

typedef struct _PH_LOG_ENTRY *PPH_LOG_ENTRY;

PHAPPAPI extern PH_CALLBACK PhLoggedCallback;

PHAPPAPI
VOID
NTAPI
PhLogMessageEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Message
    );

PHAPPAPI
PPH_STRING
NTAPI
PhFormatLogEntry(
    _In_ PPH_LOG_ENTRY Entry
    );

// actions

PHAPPAPI
BOOLEAN
NTAPI
PhUiLockComputer(
    _In_ HWND hWnd
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiLogoffComputer(
    _In_ HWND hWnd
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSleepComputer(
    _In_ HWND hWnd
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiHibernateComputer(
    _In_ HWND hWnd
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiRestartComputer(
    _In_ HWND hWnd,
    _In_ ULONG Flags
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiShutdownComputer(
    _In_ HWND hWnd,
    _In_ ULONG Flags
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiConnectSession(
    _In_ HWND hWnd,
    _In_ ULONG SessionId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDisconnectSession(
    _In_ HWND hWnd,
    _In_ ULONG SessionId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiLogoffSession(
    _In_ HWND hWnd,
    _In_ ULONG SessionId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiTerminateProcesses(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiTerminateTreeProcess(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSuspendProcesses(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiResumeProcesses(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiRestartProcess(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDebugProcess(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiReduceWorkingSetProcesses(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetVirtualizationProcess(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ BOOLEAN Enable
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDetachFromDebuggerProcess(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiInjectDllProcess(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetIoPriorityProcess(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ ULONG IoPriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPagePriorityProcess(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ ULONG PagePriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPriorityProcess(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ ULONG PriorityClass
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetDepStatusProcess(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiStartService(
    _In_ HWND hWnd,
    _In_ PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiContinueService(
    _In_ HWND hWnd,
    _In_ PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiPauseService(
    _In_ HWND hWnd,
    _In_ PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiStopService(
    _In_ HWND hWnd,
    _In_ PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDeleteService(
    _In_ HWND hWnd,
    _In_ PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiCloseConnections(
    _In_ HWND hWnd,
    _In_ PPH_NETWORK_ITEM *Connections,
    _In_ ULONG NumberOfConnections
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiTerminateThreads(
    _In_ HWND hWnd,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiForceTerminateThreads(
    _In_ HWND hWnd,
    _In_ HANDLE ProcessId,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSuspendThreads(
    _In_ HWND hWnd,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiResumeThreads(
    _In_ HWND hWnd,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPriorityThread(
    _In_ HWND hWnd,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ ULONG ThreadPriorityWin32
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetIoPriorityThread(
    _In_ HWND hWnd,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ ULONG IoPriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPagePriorityThread(
    _In_ HWND hWnd,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ ULONG PagePriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiUnloadModule(
    _In_ HWND hWnd,
    _In_ HANDLE ProcessId,
    _In_ PPH_MODULE_ITEM Module
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiFreeMemory(
    _In_ HWND hWnd,
    _In_ HANDLE ProcessId,
    _In_ PPH_MEMORY_ITEM MemoryItem,
    _In_ BOOLEAN Free
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiCloseHandles(
    _In_ HWND hWnd,
    _In_ HANDLE ProcessId,
    _In_ PPH_HANDLE_ITEM *Handles,
    _In_ ULONG NumberOfHandles,
    _In_ BOOLEAN Warn
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetAttributesHandle(
    _In_ HWND hWnd,
    _In_ HANDLE ProcessId,
    _In_ PPH_HANDLE_ITEM Handle,
    _In_ ULONG Attributes
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDestroyHeap(
    _In_ HWND hWnd,
    _In_ HANDLE ProcessId,
    _In_ PVOID HeapHandle
    );

// affinity

PHAPPAPI
BOOLEAN
NTAPI
PhShowProcessAffinityDialog2(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG_PTR AffinityMask,
    _Out_ PULONG_PTR NewAffinityMask
    );

// chdlg

#define PH_CHOICE_DIALOG_SAVED_CHOICES 10

#define PH_CHOICE_DIALOG_CHOICE 0x0
#define PH_CHOICE_DIALOG_USER_CHOICE 0x1
#define PH_CHOICE_DIALOG_PASSWORD 0x2
#define PH_CHOICE_DIALOG_TYPE_MASK 0x3

PHAPPAPI
BOOLEAN PhaChoiceDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PWSTR Title,
    _In_ PWSTR Message,
    _In_opt_ PWSTR *Choices,
    _In_opt_ ULONG NumberOfChoices,
    _In_opt_ PWSTR Option,
    _In_ ULONG Flags,
    _Inout_ PPH_STRING *SelectedChoice,
    _Inout_opt_ PBOOLEAN SelectedOption,
    _In_opt_ PWSTR SavedChoicesSettingName
    );

// chproc

PHAPPAPI
BOOLEAN
NTAPI
PhShowChooseProcessDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PWSTR Message,
    _Out_ PHANDLE ProcessId
    );

// procrec

PHAPPAPI
VOID
NTAPI
PhShowProcessRecordDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_RECORD Record
    );

// srvctl

#define WM_PH_SET_LIST_VIEW_SETTINGS (WM_APP + 701)

PHAPPAPI
HWND
NTAPI
PhCreateServiceListControl(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_SERVICE_ITEM *Services,
    _In_ ULONG NumberOfServices
    );

// settings

typedef enum _PH_SETTING_TYPE
{
    StringSettingType,
    IntegerSettingType,
    IntegerPairSettingType
} PH_SETTING_TYPE, PPH_SETTING_TYPE;

PHAPPAPI
_May_raise_ ULONG
NTAPI
PhGetIntegerSetting(
    _In_ PWSTR Name
    );

PHAPPAPI
_May_raise_ PH_INTEGER_PAIR
NTAPI
PhGetIntegerPairSetting(
    _In_ PWSTR Name
    );

PHAPPAPI
_May_raise_ PPH_STRING
NTAPI
PhGetStringSetting(
    _In_ PWSTR Name
    );

PHAPPAPI
_May_raise_ VOID
NTAPI
PhSetIntegerSetting(
    _In_ PWSTR Name,
    _In_ ULONG Value
    );

PHAPPAPI
_May_raise_ VOID
NTAPI
PhSetIntegerPairSetting(
    _In_ PWSTR Name,
    _In_ PH_INTEGER_PAIR Value
    );

PHAPPAPI
_May_raise_ VOID
NTAPI
PhSetStringSetting(
    _In_ PWSTR Name,
    _In_ PWSTR Value
    );

PHAPPAPI
_May_raise_ VOID
NTAPI
PhSetStringSetting2(
    _In_ PWSTR Name,
    _In_ PPH_STRINGREF Value
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
    _In_ PPH_SETTING_CREATE Settings,
    _In_ ULONG NumberOfSettings
    );

#ifdef __cplusplus
}
#endif

#endif
