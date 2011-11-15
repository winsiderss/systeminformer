#ifndef PHAPP_H
#define PHAPP_H

#ifdef PHAPP_EXPORT
#define PHAPPAPI __declspec(dllexport)
#else
#define PHAPPAPI
#endif

#include <phgui.h>
#include <treenew.h>
#include <graph.h>
#include <circbuf.h>
#include <phnet.h>
#include <providers.h>
#include <colmgr.h>
#include <uimodels.h>
#include "../resource.h"

#define KPH_ERROR_MESSAGE (L"KProcessHacker does not support your operating system " \
            L"or could not be loaded. Make sure Process Hacker is running " \
            L"with administrative privileges.")

#ifndef PH_MAIN_PRIVATE

extern PPH_STRING PhApplicationDirectory;
extern PPH_STRING PhApplicationFileName;
extern HFONT PhApplicationFont;
extern PPH_STRING PhCurrentUserName;
extern HINSTANCE PhInstanceHandle;
extern PPH_STRING PhLocalSystemName;
extern BOOLEAN PhPluginsEnabled;
extern PPH_STRING PhProcDbFileName;
extern PPH_STRING PhSettingsFileName;
extern PH_INTEGER_PAIR PhSmallIconSize;
extern PH_STARTUP_PARAMETERS PhStartupParameters;

extern PH_PROVIDER_THREAD PhPrimaryProviderThread;
extern PH_PROVIDER_THREAD PhSecondaryProviderThread;

extern COLORREF PhSysWindowColor;

#endif

// main

typedef struct _PH_STARTUP_PARAMETERS
{
    union
    {
        struct
        {
            ULONG NoKph : 1;
            ULONG NoSettings : 1;
            ULONG NoPlugins : 1;
            ULONG ShowHidden : 1;
            ULONG ShowVisible : 1;
            ULONG ShowOptions : 1;
            ULONG NewInstance : 1;
            ULONG Elevate : 1;
            ULONG Silent : 1;
            ULONG CommandMode : 1;
            ULONG PhSvc : 1;
            ULONG InstallKph : 1;
            ULONG UninstallKph : 1;
            ULONG Debug : 1;
            ULONG Help : 1;
            ULONG Spare : 17;
        };
        ULONG Flags;
    };

    PPH_STRING SettingsFileName;

    HWND WindowHandle;
    POINT Point;

    PPH_STRING CommandType;
    PPH_STRING CommandObject;
    PPH_STRING CommandAction;
    PPH_STRING CommandValue;

    PPH_STRING RunAsServiceMode;
} PH_STARTUP_PARAMETERS, *PPH_STARTUP_PARAMETERS;

PHAPPAPI
VOID PhRegisterDialog(
    __in HWND DialogWindowHandle
    );

PHAPPAPI
VOID PhUnregisterDialog(
    __in HWND DialogWindowHandle
    );

VOID PhApplyUpdateInterval(
    __in ULONG Interval
    );

VOID PhInitializeFont(
    __in HWND hWnd
    );

// appsup

extern GUID XP_CONTEXT_GUID;
extern GUID VISTA_CONTEXT_GUID;
extern GUID WIN7_CONTEXT_GUID;
extern GUID WIN8_CONTEXT_GUID;

PHAPPAPI
BOOLEAN PhGetProcessIsSuspended(
    __in PSYSTEM_PROCESS_INFORMATION Process
    );

NTSTATUS PhGetProcessSwitchContext(
    __in HANDLE ProcessHandle,
    __out PGUID Guid
    );

PPH_STRING PhGetProcessPackageFullName(
    __in HANDLE ProcessHandle
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
} PH_KNOWN_PROCESS_TYPE;

PHAPPAPI
NTSTATUS PhGetProcessKnownType(
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
BOOLEAN PhaGetProcessKnownCommandLine(
    __in PPH_STRING CommandLine,
    __in PH_KNOWN_PROCESS_TYPE KnownProcessType,
    __out PPH_KNOWN_PROCESS_COMMAND_LINE KnownCommandLine
    );

PPH_STRING PhEscapeStringForDelimiter(
    __in PPH_STRING String,
    __in WCHAR Delimiter
    );

PPH_STRING PhUnescapeStringForDelimiter(
    __in PPH_STRING String,
    __in WCHAR Delimiter
    );

typedef struct mxml_node_s mxml_node_t;

PPH_STRING PhGetOpaqueXmlNodeText(
    __in mxml_node_t *node
    );

PHAPPAPI
VOID PhSearchOnlineString(
    __in HWND hWnd,
    __in PWSTR String
    );

PHAPPAPI
VOID PhShellExecuteUserString(
    __in HWND hWnd,
    __in PWSTR Setting,
    __in PWSTR String,
    __in BOOLEAN UseShellExecute,
    __in_opt PWSTR ErrorMessage
    );

PHAPPAPI
VOID PhLoadSymbolProviderOptions(
    __inout PPH_SYMBOL_PROVIDER SymbolProvider
    );

PHAPPAPI
VOID PhSetExtendedListViewWithSettings(
    __in HWND hWnd
    );

PWSTR PhMakeContextAtom(
    VOID
    );

PHAPPAPI
VOID PhCopyListViewInfoTip(
    __inout LPNMLVGETINFOTIP GetInfoTip,
    __in PPH_STRINGREF Tip
    );

PHAPPAPI
VOID PhCopyListView(
    __in HWND ListViewHandle
    );

PHAPPAPI
VOID PhHandleListViewNotifyForCopy(
    __in LPARAM lParam,
    __in HWND ListViewHandle
    );

PHAPPAPI
BOOLEAN PhGetListViewContextMenuPoint(
    __in HWND ListViewHandle,
    __out PPOINT Point
    );

HFONT PhDuplicateFontWithNewWeight(
    __in HFONT Font,
    __in LONG NewWeight
    );

PHAPPAPI
VOID PhLoadWindowPlacementFromSetting(
    __in_opt PWSTR PositionSettingName,
    __in_opt PWSTR SizeSettingName,
    __in HWND WindowHandle
    );

PHAPPAPI
VOID PhSaveWindowPlacementToSetting(
    __in_opt PWSTR PositionSettingName,
    __in_opt PWSTR SizeSettingName,
    __in HWND WindowHandle
    );

PHAPPAPI
VOID PhLoadListViewColumnsFromSetting(
    __in PWSTR Name,
    __in HWND ListViewHandle
    );

PHAPPAPI
VOID PhSaveListViewColumnsToSetting(
    __in PWSTR Name,
    __in HWND ListViewHandle
    );

PHAPPAPI
PPH_STRING PhGetPhVersion(
    VOID
    );

PHAPPAPI
VOID PhWritePhTextHeader(
    __inout PPH_FILE_STREAM FileStream
    );

#define PH_SHELL_APP_PROPAGATE_PARAMETERS 0x1
#define PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY 0x2

BOOLEAN PhShellProcessHacker(
    __in HWND hWnd,
    __in_opt PWSTR Parameters,
    __in ULONG ShowWindowType,
    __in ULONG Flags,
    __in ULONG AppFlags,
    __in_opt ULONG Timeout,
    __out_opt PHANDLE ProcessHandle
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
VOID PhInitializeTreeNewColumnMenu(
    __inout PPH_TN_COLUMN_MENU_DATA Data
    );

#define PH_TN_COLUMN_MENU_NO_VISIBILITY 0x1

VOID PhInitializeTreeNewColumnMenuEx(
    __inout PPH_TN_COLUMN_MENU_DATA Data,
    __in ULONG Flags
    );

PHAPPAPI
BOOLEAN PhHandleTreeNewColumnMenu(
    __inout PPH_TN_COLUMN_MENU_DATA Data
    );

PHAPPAPI
VOID PhDeleteTreeNewColumnMenu(
    __in PPH_TN_COLUMN_MENU_DATA Data
    );

#define PH_LOAD_SHARED_IMAGE(Name, Type) LoadImage(PhInstanceHandle, (Name), (Type), 0, 0, LR_SHARED)

FORCEINLINE PVOID PhpGenericPropertyPageHeader(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam,
    __in PWSTR ContextName
    )
{
    PVOID context;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;

            context = (PVOID)propSheetPage->lParam;
            SetProp(hwndDlg, ContextName, (HANDLE)context);
        }
        break;
    case WM_DESTROY:
        {
            context = (PVOID)GetProp(hwndDlg, ContextName);
            RemoveProp(hwndDlg, ContextName);
        }
        break;
    default:
        {
            context = (PVOID)GetProp(hwndDlg, ContextName);
        }
        break;
    }

    return context;
}

// mainwnd

#define PH_MAINWND_CLASSNAME L"ProcessHacker"

#ifndef PH_MAINWND_PRIVATE
extern HWND PhMainWndHandle;
extern BOOLEAN PhMainWndExiting;
#endif

#define WM_PH_FIRST (WM_APP + 99)
#define WM_PH_ACTIVATE (WM_APP + 99)
#define PH_ACTIVATE_REPLY 0x1119

#define WM_PH_PROCESS_ADDED (WM_APP + 101)
#define WM_PH_PROCESS_MODIFIED (WM_APP + 102)
#define WM_PH_PROCESS_REMOVED (WM_APP + 103)
#define WM_PH_PROCESSES_UPDATED (WM_APP + 104)

#define WM_PH_SERVICE_ADDED (WM_APP + 105)
#define WM_PH_SERVICE_MODIFIED (WM_APP + 106)
#define WM_PH_SERVICE_REMOVED (WM_APP + 107)
#define WM_PH_SERVICES_UPDATED (WM_APP + 108)

#define WM_PH_NETWORK_ITEM_ADDED (WM_APP + 109)
#define WM_PH_NETWORK_ITEM_MODIFIED (WM_APP + 110)
#define WM_PH_NETWORK_ITEM_REMOVED (WM_APP + 111)
#define WM_PH_NETWORK_ITEMS_UPDATED (WM_APP + 112)

#define WM_PH_SHOW_PROCESS_PROPERTIES (WM_APP + 120)
#define WM_PH_DESTROY (WM_APP + 121)
#define WM_PH_SAVE_ALL_SETTINGS (WM_APP + 122)
#define WM_PH_PREPARE_FOR_EARLY_SHUTDOWN (WM_APP + 123)
#define WM_PH_CANCEL_EARLY_SHUTDOWN (WM_APP + 124)
#define WM_PH_DELAYED_LOAD_COMPLETED (WM_APP + 125)
#define WM_PH_NOTIFY_ICON_MESSAGE (WM_APP + 126)
#define WM_PH_TOGGLE_VISIBLE (WM_APP + 127)
#define WM_PH_SHOW_MEMORY_EDITOR (WM_APP + 128)
#define WM_PH_SHOW_MEMORY_RESULTS (WM_APP + 129)
#define WM_PH_SELECT_TAB_PAGE (WM_APP + 130)
#define WM_PH_GET_CALLBACK_LAYOUT_PADDING (WM_APP + 131)
#define WM_PH_INVALIDATE_LAYOUT_PADDING (WM_APP + 132)
#define WM_PH_SELECT_PROCESS_NODE (WM_APP + 133)
#define WM_PH_SELECT_SERVICE_ITEM (WM_APP + 134)
#define WM_PH_SELECT_NETWORK_ITEM (WM_APP + 135)
#define WM_PH_UPDATE_FONT (WM_APP + 136)
#define WM_PH_GET_FONT (WM_APP + 137)
#define WM_PH_INVOKE (WM_APP + 138)
#define WM_PH_ADD_MENU_ITEM (WM_APP + 139)
#define WM_PH_ADD_TAB_PAGE (WM_APP + 140)
#define WM_PH_REFRESH (WM_APP + 141)
#define WM_PH_GET_UPDATE_AUTOMATICALLY (WM_APP + 142)
#define WM_PH_SET_UPDATE_AUTOMATICALLY (WM_APP + 143)
#define WM_PH_LAST (WM_APP + 143)

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
#define ProcessHacker_ShowMemoryEditor(hWnd, ShowMemoryEditor) \
    PostMessage(hWnd, WM_PH_SHOW_MEMORY_EDITOR, 0, (LPARAM)(ShowMemoryEditor))
#define ProcessHacker_ShowMemoryResults(hWnd, ShowMemoryResults) \
    PostMessage(hWnd, WM_PH_SHOW_MEMORY_RESULTS, 0, (LPARAM)(ShowMemoryResults))
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
    ((ULONG_PTR)SendMessage(hWnd, WM_PH_ADD_MENU_ITEM, 0, (LPARAM)(AddMenuItem)))
#define ProcessHacker_AddTabPage(hWnd, TabPage) \
    SendMessage(hWnd, WM_PH_ADD_TAB_PAGE, 0, (LPARAM)(TabPage))
#define ProcessHacker_Refresh(hWnd) \
    SendMessage(hWnd, WM_PH_REFRESH, 0, 0)
#define ProcessHacker_GetUpdateAutomatically(hWnd) \
    ((BOOLEAN)SendMessage(hWnd, WM_PH_GET_UPDATE_AUTOMATICALLY, 0, 0))
#define ProcessHacker_SetUpdateAutomatically(hWnd, Value) \
    SendMessage(hWnd, WM_PH_SET_UPDATE_AUTOMATICALLY, (WPARAM)(Value), 0)

typedef struct _PH_SHOWMEMORYEDITOR
{
    HANDLE ProcessId;
    PVOID BaseAddress;
    SIZE_T RegionSize;
    ULONG SelectOffset;
    ULONG SelectLength;
} PH_SHOWMEMORYEDITOR, *PPH_SHOWMEMORYEDITOR;

typedef struct _PH_SHOWMEMORYRESULTS
{
    HANDLE ProcessId;
    PPH_LIST Results;
} PH_SHOWMEMORYRESULTS, *PPH_SHOWMEMORYRESULTS;

typedef struct _PH_LAYOUT_PADDING_DATA
{
    RECT Padding;
} PH_LAYOUT_PADDING_DATA, *PPH_LAYOUT_PADDING_DATA;

typedef struct _PH_ADDMENUITEM
{
    __in PVOID Plugin;
    __in HMENU ParentMenu;
    __in_opt PWSTR InsertAfter;
    __in ULONG Flags;
    __in ULONG Id;
    __in PWSTR Text;
    __in_opt PVOID Context;
} PH_ADDMENUITEM, *PPH_ADDMENUITEM;

typedef HWND (NTAPI *PPH_TAB_PAGE_CREATE_FUNCTION)(
    __in PVOID Context
    );

typedef VOID (NTAPI *PPH_TAB_PAGE_CALLBACK_FUNCTION)(
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Parameter3,
    __in PVOID Context
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

BOOLEAN PhMainWndInitialization(
    __in INT ShowCommand
    );

VOID PhShowIconContextMenu(
    __in POINT Location
    );

PHAPPAPI
VOID PhShowIconNotification(
    __in PWSTR Title,
    __in PWSTR Text,
    __in ULONG Flags
    );

VOID PhShowProcessContextMenu(
    __in POINT Location
    );

VOID PhShowServiceContextMenu(
    __in POINT Location
    );

VOID PhShowNetworkContextMenu(
    __in POINT Location
    );

// plugins

#ifndef PLUGINS_PRIVATE
extern PH_AVL_TREE PhPluginsByName;
#endif

VOID PhPluginsInitialization(
    VOID
    );

BOOLEAN PhIsPluginDisabled(
    __in PPH_STRINGREF BaseName
    );

VOID PhSetPluginDisabled(
    __in PPH_STRINGREF BaseName,
    __in BOOLEAN Disable
    );

VOID PhLoadPlugins(
    VOID
    );

VOID PhUnloadPlugins(
    VOID
    );

// procprp

#define PH_PROCESS_PROPCONTEXT_MAXPAGES 20

#ifndef DLGTEMPLATEEX
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
#endif

typedef struct _PH_PROCESS_PROPCONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    HWND WindowHandle;
    PH_EVENT CreatedEvent;
    PPH_STRING Title;
    PROPSHEETHEADER PropSheetHeader;
    HPROPSHEETPAGE *PropSheetPages;

    HANDLE SelectThreadId;
} PH_PROCESS_PROPCONTEXT, *PPH_PROCESS_PROPCONTEXT;

typedef struct _PH_PROCESS_PROPPAGECONTEXT
{
    PPH_PROCESS_PROPCONTEXT PropContext;
    PVOID Context;
    PROPSHEETPAGE PropSheetPage;

    BOOLEAN LayoutInitialized;
} PH_PROCESS_PROPPAGECONTEXT, *PPH_PROCESS_PROPPAGECONTEXT;

BOOLEAN PhProcessPropInitialization(
    VOID
    );

PHAPPAPI
PPH_PROCESS_PROPCONTEXT PhCreateProcessPropContext(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhRefreshProcessPropContext(
    __inout PPH_PROCESS_PROPCONTEXT PropContext
    );

PHAPPAPI
VOID PhSetSelectThreadIdProcessPropContext(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in HANDLE ThreadId
    );

PHAPPAPI
BOOLEAN PhAddProcessPropPage(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in __assumeRefs(1) PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    );

PHAPPAPI
BOOLEAN PhAddProcessPropPage2(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in HPROPSHEETPAGE PropSheetPageHandle
    );

PHAPPAPI
PPH_PROCESS_PROPPAGECONTEXT PhCreateProcessPropPageContext(
    __in LPCWSTR Template,
    __in DLGPROC DlgProc,
    __in_opt PVOID Context
    );

PHAPPAPI
PPH_PROCESS_PROPPAGECONTEXT PhCreateProcessPropPageContextEx(
    __in_opt PVOID InstanceHandle,
    __in LPCWSTR Template,
    __in DLGPROC DlgProc,
    __in_opt PVOID Context
    );

PHAPPAPI
BOOLEAN PhPropPageDlgProcHeader(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam,
    __out LPPROPSHEETPAGE *PropSheetPage,
    __out PPH_PROCESS_PROPPAGECONTEXT *PropPageContext,
    __out PPH_PROCESS_ITEM *ProcessItem
    );

PHAPPAPI
VOID PhPropPageDlgProcDestroy(
    __in HWND hwndDlg
    );

#define PH_PROP_PAGE_TAB_CONTROL_PARENT ((PPH_LAYOUT_ITEM)0x1)

PHAPPAPI
PPH_LAYOUT_ITEM PhAddPropPageLayoutItem(
    __in HWND hwnd,
    __in HWND Handle,
    __in PPH_LAYOUT_ITEM ParentItem,
    __in ULONG Anchor
    );

PHAPPAPI
VOID PhDoPropPageLayout(
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
BOOLEAN PhShowProcessProperties(
    __in PPH_PROCESS_PROPCONTEXT Context
    );

// sysinfo

typedef enum _PH_SYSINFO_VIEW_TYPE
{
    SysInfoSummaryView,
    SysInfoSectionView
} PH_SYSINFO_VIEW_TYPE;

typedef VOID (NTAPI *PPH_SYSINFO_COLOR_SETUP_FUNCTION)(
    __out PPH_GRAPH_DRAW_INFO DrawInfo,
    __in COLORREF Color1,
    __in COLORREF Color2
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

    ULONG MinimumGraphHeight;
    ULONG SectionViewGraphHeight;
    ULONG PanelWidth;
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
    __in struct _PH_SYSINFO_SECTION *Section,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    );

typedef struct _PH_SYSINFO_CREATE_DIALOG
{
    BOOLEAN CustomCreate;

    // Parameters for default create
    PVOID Instance;
    PWSTR Template;
    DLGPROC DialogProc;
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

    // Private

    struct
    {
        ULONG GraphHot : 1;
        ULONG PanelHot : 1;
        ULONG HasFocus : 1;
        ULONG HideFocus : 1;
        ULONG SpareFlags : 28;
    };
    HWND DialogHandle;
    HWND PanelHandle;
    ULONG PanelId;
    WNDPROC GraphOldWndProc;
    WNDPROC PanelOldWndProc;
} PH_SYSINFO_SECTION, *PPH_SYSINFO_SECTION;

VOID PhSiNotifyChangeSettings(
    VOID
    );

VOID PhSiSetColorsGraphDrawInfo(
    __out PPH_GRAPH_DRAW_INFO DrawInfo,
    __in COLORREF Color1,
    __in COLORREF Color2
    );

VOID PhShowSystemInformationDialog(
    __in_opt PWSTR SectionName
    );

// log

#define PH_LOG_ENTRY_PROCESS_FIRST 1
#define PH_LOG_ENTRY_PROCESS_CREATE 1
#define PH_LOG_ENTRY_PROCESS_DELETE 2
#define PH_LOG_ENTRY_PROCESS_LAST 2

#define PH_LOG_ENTRY_SERVICE_FIRST 3
#define PH_LOG_ENTRY_SERVICE_CREATE 3
#define PH_LOG_ENTRY_SERVICE_DELETE 4
#define PH_LOG_ENTRY_SERVICE_START 5
#define PH_LOG_ENTRY_SERVICE_STOP 6
#define PH_LOG_ENTRY_SERVICE_CONTINUE 7
#define PH_LOG_ENTRY_SERVICE_PAUSE 8
#define PH_LOG_ENTRY_SERVICE_LAST 8

#define PH_LOG_ENTRY_MESSAGE 9

typedef struct _PH_LOG_ENTRY
{
    UCHAR Type;
    UCHAR Reserved1;
    USHORT Flags;
    LARGE_INTEGER Time;
    union
    {
        struct
        {
            HANDLE ProcessId;
            PPH_STRING Name;
            HANDLE ParentProcessId;
            PPH_STRING ParentName;
        } Process;
        struct
        {
            PPH_STRING Name;
            PPH_STRING DisplayName;
        } Service;
        PPH_STRING Message;
    };
    UCHAR Buffer[1];
} PH_LOG_ENTRY, *PPH_LOG_ENTRY;

#ifndef PH_LOG_PRIVATE
PH_CIRCULAR_BUFFER_PVOID PhLogBuffer;
PH_CALLBACK PhLoggedCallback;
#endif

VOID PhLogInitialization(
    VOID
    );

VOID PhClearLogEntries(
    VOID
    );

VOID PhLogProcessEntry(
    __in UCHAR Type,
    __in HANDLE ProcessId,
    __in PPH_STRING Name,
    __in_opt HANDLE ParentProcessId,
    __in_opt PPH_STRING ParentName
    );

VOID PhLogServiceEntry(
    __in UCHAR Type,
    __in PPH_STRING Name,
    __in PPH_STRING DisplayName
    );

PHAPPAPI
VOID PhLogMessageEntry(
    __in UCHAR Type,
    __in PPH_STRING Message
    );

PHAPPAPI
PPH_STRING PhFormatLogEntry(
    __in PPH_LOG_ENTRY Entry
    );

// dbgcon

VOID PhShowDebugConsole(
    VOID
    );

// actions

typedef enum _PH_ACTION_ELEVATION_LEVEL
{
    NeverElevateAction = 0,
    PromptElevateAction = 1,
    AlwaysElevateAction = 2
} PH_ACTION_ELEVATION_LEVEL;

BOOLEAN PhUiConnectToPhSvc(
    __in HWND hWnd,
    __in BOOLEAN ConnectOnly
    );

VOID PhUiDisconnectFromPhSvc(
    VOID
    );

PHAPPAPI
BOOLEAN PhUiLockComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiLogoffComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiSleepComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiHibernateComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiRestartComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiShutdownComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiPoweroffComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiConnectSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

PHAPPAPI
BOOLEAN PhUiDisconnectSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

PHAPPAPI
BOOLEAN PhUiLogoffSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

PHAPPAPI
BOOLEAN PhUiTerminateProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN PhUiTerminateTreeProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiSuspendProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN PhUiResumeProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN PhUiRestartProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiDebugProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiReduceWorkingSetProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN PhUiSetVirtualizationProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in BOOLEAN Enable
    );

PHAPPAPI
BOOLEAN PhUiDetachFromDebuggerProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiInjectDllProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiSetIoPriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG IoPriority
    );

PHAPPAPI
BOOLEAN PhUiSetPagePriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG PagePriority
    );

PHAPPAPI
BOOLEAN PhUiSetPriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG PriorityClass
    );

PHAPPAPI
BOOLEAN PhUiSetDepStatusProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiSetProtectionProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiStartService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN PhUiContinueService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN PhUiPauseService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN PhUiStopService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN PhUiDeleteService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN PhUiCloseConnections(
    __in HWND hWnd,
    __in PPH_NETWORK_ITEM *Connections,
    __in ULONG NumberOfConnections
    );

PHAPPAPI
BOOLEAN PhUiTerminateThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN PhUiForceTerminateThreads(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN PhUiSuspendThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN PhUiResumeThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN PhUiSetPriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG ThreadPriorityWin32
    );

PHAPPAPI
BOOLEAN PhUiSetIoPriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG IoPriority
    );

PHAPPAPI
BOOLEAN PhUiSetPagePriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG PagePriority
    );

PHAPPAPI
BOOLEAN PhUiUnloadModule(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_MODULE_ITEM Module
    );

PHAPPAPI
BOOLEAN PhUiFreeMemory(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_MEMORY_ITEM MemoryItem,
    __in BOOLEAN Free
    );

PHAPPAPI
BOOLEAN PhUiCloseHandles(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM *Handles,
    __in ULONG NumberOfHandles,
    __in BOOLEAN Warn
    );

PHAPPAPI
BOOLEAN PhUiSetAttributesHandle(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM Handle,
    __in ULONG Attributes
    );

PHAPPAPI
BOOLEAN PhUiDestroyHeap(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PVOID HeapHandle
    );

// itemtips

PPH_STRING PhGetProcessTooltipText(
    __in PPH_PROCESS_ITEM Process
    );

PPH_STRING PhGetServiceTooltipText(
    __in PPH_SERVICE_ITEM Service
    );

// cmdmode

NTSTATUS PhCommandModeStart(
    VOID
    );

// anawait

VOID PhUiAnalyzeWaitThread(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in HANDLE ThreadId,
    __in PPH_SYMBOL_PROVIDER SymbolProvider
    );

// mdump

BOOLEAN PhUiCreateDumpFileProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

// about

VOID PhShowAboutDialog(
    __in HWND ParentWindowHandle
    );

PPH_STRING PhGetDiagnosticsString(
    VOID
    );

// affinity

VOID PhShowProcessAffinityDialog(
    __in HWND ParentWindowHandle,
    __in_opt PPH_PROCESS_ITEM ProcessItem,
    __in_opt PPH_THREAD_ITEM ThreadItem
    );

PHAPPAPI
BOOLEAN PhShowProcessAffinityDialog2(
    __in HWND ParentWindowHandle,
    __in ULONG_PTR AffinityMask,
    __out PULONG_PTR NewAffinityMask
    );

// chcol

#define PH_CONTROL_TYPE_TREE_NEW 1

VOID PhShowChooseColumnsDialog(
    __in HWND ParentWindowHandle,
    __in HWND ControlHandle,
    __in ULONG Type
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
BOOLEAN PhShowChooseProcessDialog(
    __in HWND ParentWindowHandle,
    __in PWSTR Message,
    __out PHANDLE ProcessId
    );

// findobj

VOID PhShowFindObjectsDialog(
    VOID
    );

// gdihndl

VOID PhShowGdiHandlesDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    );

// heapinfo

VOID PhShowProcessHeapsDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    );

NTSTATUS PhGetProcessDefaultHeap(
    __in HANDLE ProcessHandle,
    __out PPVOID Heap
    );

// hidnproc

VOID PhShowHiddenProcessesDialog(
    VOID
    );

// hndlprp

VOID PhShowHandleProperties(
    __in HWND ParentWindowHandle,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM HandleItem
    );

// hndlstat

VOID PhShowHandleStatisticsDialog(
    __in HWND ParentWindowHandle,
    __in HANDLE ProcessId
    );

// infodlg

VOID PhShowInformationDialog(
    __in HWND ParentWindowHandle,
    __in PWSTR String
    );

// jobprp

VOID PhShowJobProperties(
    __in HWND ParentWindowHandle,
    __in PPH_OPEN_OBJECT OpenObject,
    __in_opt PVOID Context,
    __in_opt PWSTR Title
    );

HPROPSHEETPAGE PhCreateJobPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in_opt PVOID Context,
    __in_opt DLGPROC HookProc
    );

// logwnd

VOID PhShowLogDialog(
    VOID
    );

// memedit

VOID PhShowMemoryEditorDialog(
    __in HANDLE ProcessId,
    __in PVOID BaseAddress,
    __in SIZE_T RegionSize,
    __in ULONG SelectOffset,
    __in ULONG SelectLength
    );

// memlists

VOID PhShowMemoryListsDialog(
    __in HWND ParentWindowHandle,
    __in_opt VOID (NTAPI *RegisterDialog)(HWND),
    __in_opt VOID (NTAPI *UnregisterDialog)(HWND)
    );

// memprot

VOID PhShowMemoryProtectDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem,
    __in PPH_MEMORY_ITEM MemoryItem
    );

// memrslt

VOID PhShowMemoryResultsDialog(
    __in HANDLE ProcessId,
    __in PPH_LIST Results
    );

// memsrch

VOID PhShowMemoryStringDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    );

// netstk

VOID PhShowNetworkStackDialog(
    __in HWND ParentWindowHandle,
    __in PPH_NETWORK_ITEM NetworkItem
    );

// ntobjprp

HPROPSHEETPAGE PhCreateEventPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in_opt PVOID Context
    );

HPROPSHEETPAGE PhCreateEventPairPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in_opt PVOID Context
    );

HPROPSHEETPAGE PhCreateMutantPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in_opt PVOID Context
    );

HPROPSHEETPAGE PhCreateSectionPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in_opt PVOID Context
    );

HPROPSHEETPAGE PhCreateSemaphorePage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in_opt PVOID Context
    );

HPROPSHEETPAGE PhCreateTimerPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in_opt PVOID Context
    );

// options

VOID PhShowOptionsDialog(
    __in HWND ParentWindowHandle
    );

// pagfiles

VOID PhShowPagefilesDialog(
    __in HWND ParentWindowHandle
    );

// plugman

VOID PhShowPluginsDialog(
    __in HWND ParentWindowHandle
    );

// procrec

PHAPPAPI
VOID PhShowProcessRecordDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_RECORD Record
    );

// runas

typedef struct _PH_RUNAS_SERVICE_PARAMETERS
{
    ULONG ProcessId;
    PWSTR UserName;
    PWSTR Password;
    ULONG LogonType;
    ULONG SessionId;
    PWSTR CurrentDirectory;
    PWSTR CommandLine;
    PWSTR FileName;
    PWSTR DesktopName;
    BOOLEAN UseLinkedToken;
    PWSTR ServiceName;
} PH_RUNAS_SERVICE_PARAMETERS, *PPH_RUNAS_SERVICE_PARAMETERS;

VOID PhShowRunAsDialog(
    __in HWND ParentWindowHandle,
    __in_opt HANDLE ProcessId
    );

NTSTATUS PhExecuteRunAsCommand(
    __in PPH_RUNAS_SERVICE_PARAMETERS Parameters
    );

NTSTATUS PhExecuteRunAsCommand2(
    __in HWND hWnd,
    __in PWSTR Program,
    __in_opt PWSTR UserName,
    __in_opt PWSTR Password,
    __in_opt ULONG LogonType,
    __in_opt HANDLE ProcessIdWithToken,
    __in ULONG SessionId,
    __in PWSTR DesktopName,
    __in BOOLEAN UseLinkedToken
    );

NTSTATUS PhRunAsServiceStart(
    __in PPH_STRING ServiceName
    );

NTSTATUS PhInvokeRunAsService(
    __in PPH_RUNAS_SERVICE_PARAMETERS Parameters
    );

// sessmsg

VOID PhShowSessionSendMessageDialog(
    __in HWND ParentWindowHandle,
    __in ULONG SessionId
    );

// sessprp

VOID PhShowSessionProperties(
    __in HWND ParentWindowHandle,
    __in ULONG SessionId
    );

// sessshad

VOID PhShowSessionShadowDialog(
    __in HWND ParentWindowHandle,
    __in ULONG SessionId
    );

// srvcr

VOID PhShowCreateServiceDialog(
    __in HWND ParentWindowHandle
    );

// srvctl

#define WM_PH_SET_LIST_VIEW_SETTINGS (WM_APP + 701)

PHAPPAPI
HWND PhCreateServiceListControl(
    __in HWND ParentWindowHandle,
    __in PPH_SERVICE_ITEM *Services,
    __in ULONG NumberOfServices
    );

// srvprp

VOID PhShowServiceProperties(
    __in HWND ParentWindowHandle,
    __in PPH_SERVICE_ITEM ServiceItem
    );

// termator

VOID PhShowProcessTerminatorDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    );

// thrdstk

VOID PhShowThreadStackDialog(
    __in HWND ParentWindowHandle,
    __in HANDLE ProcessId,
    __in HANDLE ThreadId,
    __in PPH_SYMBOL_PROVIDER SymbolProvider
    );

// tokprp

PPH_STRING PhGetGroupAttributesString(
    __in ULONG Attributes
    );

PWSTR PhGetPrivilegeAttributesString(
    __in ULONG Attributes
    );

VOID PhShowTokenProperties(
    __in HWND ParentWindowHandle,
    __in PPH_OPEN_OBJECT OpenObject,
    __in_opt PVOID Context,
    __in_opt PWSTR Title
    );

HPROPSHEETPAGE PhCreateTokenPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in_opt PVOID Context,
    __in_opt DLGPROC HookProc
    );

#endif
