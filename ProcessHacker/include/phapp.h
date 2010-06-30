#ifndef PHAPP_H
#define PHAPP_H

#include <phgui.h>
#include <treelist.h>
#include <circbuf.h>
#include <providers.h>
#include "resource.h"

#define KPH_ERROR_MESSAGE (L"KProcessHacker does not support your operating system " \
            L"or could not be loaded. Make sure Process Hacker is running " \
            L"on a 32-bit system and with administrative privileges.")

#ifndef MAIN_PRIVATE

extern PPH_STRING PhApplicationDirectory;
extern PPH_STRING PhApplicationFileName;
extern PPH_STRING PhLocalSystemName;
extern PPH_STRING PhSettingsFileName;
extern PH_STARTUP_PARAMETERS PhStartupParameters;
extern PWSTR PhWindowClassName;

extern PH_PROVIDER_THREAD PhPrimaryProviderThread;
extern PH_PROVIDER_THREAD PhSecondaryProviderThread;

extern COLORREF PhSysWindowColor;

#endif

// debugging

#ifdef DEBUG
#define dwprintf(format, ...) fwprintf(stderr, format, __VA_ARGS__)
#define dwlprintf(format, ...) PhDebugPrintLine(format, __VA_ARGS__)
#else
#define dwprintf(format, ...)
#define dwlprintf(format, ...)
#endif

// main

VOID PhDebugPrintLine(
    __in __format_string PWSTR Format,
    ...
    );

typedef struct _PH_STARTUP_PARAMETERS
{
    BOOLEAN NoKph;
    BOOLEAN NoSettings;
    PPH_STRING SettingsFileName;
    BOOLEAN ShowHidden;
    BOOLEAN ShowVisible;

    BOOLEAN CommandMode;
    PPH_STRING CommandType;
    PPH_STRING CommandObject;
    PPH_STRING CommandAction;

    BOOLEAN RunAsServiceMode;

    BOOLEAN InstallKph;
    BOOLEAN UninstallKph;

    BOOLEAN Debug;
} PH_STARTUP_PARAMETERS, *PPH_STARTUP_PARAMETERS;

INT PhMainMessageLoop();

VOID PhRegisterDialog(
    __in HWND DialogWindowHandle
    );

VOID PhUnregisterDialog(
    __in HWND DialogWindowHandle
    );

VOID PhApplyUpdateInterval(
    __in ULONG Interval
    );

VOID PhActivatePreviousInstance();

VOID PhInitializeCommonControls();

VOID PhInitializeFont(
    __in HWND hWnd
    );

VOID PhInitializeKph();

BOOLEAN PhInitializeAppSystem();

ATOM PhRegisterWindowClass();

// proctree

// Columns
#define PHTLC_NAME 0
#define PHTLC_PID 1
#define PHTLC_CPU 2
#define PHTLC_PVTMEMORY 3
#define PHTLC_USERNAME 4
#define PHTLC_MAXIMUM 5

typedef struct _PH_PROCESS_NODE
{
    PH_TREELIST_NODE Node;

    PH_ITEM_STATE State;
    HANDLE StateListHandle;
    ULONG TickCount;

    HANDLE ProcessId;
    PPH_PROCESS_ITEM ProcessItem;

    struct _PH_PROCESS_NODE *Parent;
    PPH_LIST Children;

    PH_STRINGREF TextCache[PHTLC_MAXIMUM];

    PPH_STRING PrivateMemoryText;
    PPH_STRING TooltipText;
} PH_PROCESS_NODE, *PPH_PROCESS_NODE;

VOID PhProcessTreeListInitialization();

VOID PhInitializeProcessTreeList(
    __in HWND hwnd
    );

VOID PhCreateProcessNode(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in ULONG RunId
    );

PPH_PROCESS_NODE PhFindProcessNode(
   __in HANDLE ProcessId
   );

VOID PhRemoveProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    );

VOID PhUpdateProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    );

VOID PhTickProcessNodes();

HICON PhGetStockAppIcon();

PPH_PROCESS_ITEM PhGetSelectedProcessItem();

VOID PhGetSelectedProcessItems(
    __out PPH_PROCESS_ITEM **Processes,
    __out PULONG NumberOfProcesses
    );

VOID PhDeselectAllProcessNodes();

VOID PhInvalidateAllProcessNodes();

VOID PhSelectAndEnsureVisibleProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    );

// support

BOOLEAN PhGetProcessIsSuspended(
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
    ServiceHostProcessType, // svchost
    RunDllAsAppProcessType, // rundll32
    ComSurrogateProcessType // dllhost
} PH_KNOWN_PROCESS_TYPE, *PPH_KNOWN_PROCESS_TYPE;

NTSTATUS PhGetProcessKnownType(
    __in HANDLE ProcessHandle,
    __out PPH_KNOWN_PROCESS_TYPE KnownProcessType
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

BOOLEAN PhaGetProcessKnownCommandLine(
    __in PPH_STRING CommandLine,
    __in PH_KNOWN_PROCESS_TYPE KnownProcessType,
    __out PPH_KNOWN_PROCESS_COMMAND_LINE KnownCommandLine
    );

PPH_STRING PhGetSessionInformationString(
    __in HANDLE ServerHandle,
    __in ULONG SessionId,
    __in ULONG InformationClass
    );

VOID PhSearchOnlineString(
    __in HWND hWnd,
    __in PWSTR String
    );

VOID PhShellExecuteUserString(
    __in HWND hWnd,
    __in PWSTR Setting,
    __in PWSTR String,
    __in BOOLEAN UseShellExecute
    );

VOID PhCopyListViewInfoTip(
    __inout LPNMLVGETINFOTIP GetInfoTip,
    __in PPH_STRINGREF Tip
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

#ifndef MAINWND_PRIVATE
extern HWND PhMainWndHandle;
extern BOOLEAN PhMainWndExiting;
#endif

#define WM_PH_ACTIVATE (WM_APP + 99)
#define PH_ACTIVATE_REPLY 0x1119

#define WM_PH_SHOW_PROCESS_PROPERTIES (WM_APP + 120)
#define WM_PH_DESTROY (WM_APP + 121)
#define WM_PH_SAVE_ALL_SETTINGS (WM_APP + 122)
#define WM_PH_PREPARE_FOR_EARLY_SHUTDOWN (WM_APP + 123)
#define WM_PH_CANCEL_EARLY_SHUTDOWN (WM_APP + 124)
#define WM_PH_DELAYED_LOAD_COMPLETED (WM_APP + 125)

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

BOOLEAN PhMainWndInitialization(
    __in INT ShowCommand
    );

LRESULT CALLBACK PhMainWndProc(      
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowProcessContextMenu(
    __in POINT Location
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
    PPH_STRING Title;
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

VOID PhRefreshProcessPropContext(
    __inout PPH_PROCESS_PROPCONTEXT PropContext
    );

BOOLEAN PhAddProcessPropPage(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in __assumeRefs(1) PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    );

BOOLEAN PhAddProcessPropPage2(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in HPROPSHEETPAGE PropSheetPageHandle
    );

PPH_PROCESS_PROPPAGECONTEXT PhCreateProcessPropPageContext(
    __in LPCWSTR Template,
    __in DLGPROC DlgProc,
    __in PVOID Context
    );

BOOLEAN PhShowProcessProperties(
    __in PPH_PROCESS_PROPCONTEXT Context
    );

// notifico

#define PH_ICON_MINIMUM 0x1
#define PH_ICON_CPU_HISTORY 0x1
#define PH_ICON_IO_HISTORY 0x2
#define PH_ICON_COMMIT_HISTORY 0x4
#define PH_ICON_PHYSICAL_HISTORY 0x8
#define PH_ICON_CPU_USAGE 0x10
#define PH_ICON_MAXIMUM 0x20

VOID PhAddNotifyIcon(
    __in ULONG Id
    );

VOID PhRemoveNotifyIcon(
    __in ULONG Id
    );

// dbgcon

VOID PhShowDebugConsole();

// actions

typedef enum _PH_ACTION_ELEVATION_LEVEL
{
    NeverElevateAction = 0,
    PromptElevateAction = 1,
    AlwaysElevateAction = 2
} PH_ACTION_ELEVATION_LEVEL;

BOOLEAN PhUiLockComputer(
    __in HWND hWnd
    );

BOOLEAN PhUiLogoffComputer(
    __in HWND hWnd
    );

BOOLEAN PhUiSleepComputer(
    __in HWND hWnd
    );

BOOLEAN PhUiHibernateComputer(
    __in HWND hWnd
    );

BOOLEAN PhUiRestartComputer(
    __in HWND hWnd
    );

BOOLEAN PhUiShutdownComputer(
    __in HWND hWnd
    );

BOOLEAN PhUiPoweroffComputer(
    __in HWND hWnd
    );

BOOLEAN PhUiConnectSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

BOOLEAN PhUiDisconnectSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

BOOLEAN PhUiLogoffSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

BOOLEAN PhUiTerminateProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

BOOLEAN PhUiTerminateTreeProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
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

BOOLEAN PhUiRestartProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

BOOLEAN PhUiDebugProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

BOOLEAN PhUiReduceWorkingSetProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

BOOLEAN PhUiSetVirtualizationProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in BOOLEAN Enable
    );

BOOLEAN PhUiDetachFromDebuggerProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

BOOLEAN PhUiInjectDllProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

BOOLEAN PhUiSetIoPriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG IoPriority
    );

BOOLEAN PhUiSetPriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG PriorityClassWin32
    );

BOOLEAN PhUiSetDepStatusProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

BOOLEAN PhUiSetProtectionProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

BOOLEAN PhUiStartService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

BOOLEAN PhUiContinueService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

BOOLEAN PhUiPauseService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

BOOLEAN PhUiStopService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

BOOLEAN PhUiDeleteService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

BOOLEAN PhUiCloseConnections(
    __in HWND hWnd,
    __in PPH_NETWORK_ITEM *Connections,
    __in ULONG NumberOfConnections
    );

BOOLEAN PhUiTerminateThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

BOOLEAN PhUiForceTerminateThreads(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

BOOLEAN PhUiSuspendThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

BOOLEAN PhUiResumeThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

BOOLEAN PhUiSetPriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG ThreadPriorityWin32
    );

BOOLEAN PhUiSetIoPriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG IoPriority
    );

BOOLEAN PhUiUnloadModule(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_MODULE_ITEM Module
    );

BOOLEAN PhUiCloseHandles(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM *Handles,
    __in ULONG NumberOfHandles,
    __in BOOLEAN Warn
    );

BOOLEAN PhUiSetAttributesHandle(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM Handle,
    __in ULONG Attributes
    );

// itemtips

PPH_STRING PhGetProcessTooltipText(
    __in PPH_PROCESS_ITEM Process
    );

PPH_STRING PhGetServiceTooltipText(
    __in PPH_SERVICE_ITEM Service
    );

// cmdmode

NTSTATUS PhCommandModeStart();

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

PPH_STRING PhGetDiagnosticsString();

// affinity

VOID PhShowProcessAffinityDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    );

// chdlg

#define PH_CHOICE_DIALOG_SAVED_CHOICES 10

#define PH_CHOICE_DIALOG_CHOICE 0x0
#define PH_CHOICE_DIALOG_USER_CHOICE 0x1
#define PH_CHOICE_DIALOG_PASSWORD 0x2
#define PH_CHOICE_DIALOG_TYPE_MASK 0x3

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

// findobj

VOID PhShowFindObjectsDialog();

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

VOID PhShowHiddenProcessesDialog();

// hndlprp

VOID PhShowHandleProperties(
    __in HWND ParentWindowHandle,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM HandleItem
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
    __in PVOID Context,
    __in_opt PWSTR Title
    );

HPROPSHEETPAGE PhCreateJobPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context,
    __in_opt DLGPROC HookProc
    );

// netstk

VOID PhShowNetworkStackDialog(
    __in HWND ParentWindowHandle,
    __in PPH_NETWORK_ITEM NetworkItem
    );

// ntobjprp

HPROPSHEETPAGE PhCreateEventPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context
    );

HPROPSHEETPAGE PhCreateEventPairPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context
    );

HPROPSHEETPAGE PhCreateMutantPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context
    );

HPROPSHEETPAGE PhCreateSectionPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context
    );

HPROPSHEETPAGE PhCreateSemaphorePage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context
    );

HPROPSHEETPAGE PhCreateTimerPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context
    );

// options

VOID PhShowOptionsDialog(
    __in HWND ParentWindowHandle
    );

// pagfiles

VOID PhShowPagefilesDialog(
    __in HWND ParentWindowHandle
    );

// runas

VOID PhShowRunAsDialog(
    __in HWND ParentWindowHandle,
    __in_opt HANDLE ProcessId
    );

NTSTATUS PhRunAsCommandStart(
    __in PWSTR ServiceCommandLine,
    __in PWSTR ServiceName
    );

NTSTATUS PhRunAsCommandStart2(
    __in HWND hWnd,
    __in PWSTR Program,
    __in_opt PWSTR UserName,
    __in_opt PWSTR Password,
    __in_opt ULONG LogonType,
    __in_opt HANDLE ProcessIdWithToken,
    __in ULONG SessionId
    );

VOID PhRunAsServiceStart();

NTSTATUS PhCreateProcessAsUser(
    __in_opt PWSTR ApplicationName,
    __in_opt PWSTR CommandLine,
    __in_opt PWSTR CurrentDirectory,
    __in_opt PVOID Environment,
    __in_opt PWSTR DomainName,
    __in_opt PWSTR UserName,
    __in_opt PWSTR Password,
    __in_opt ULONG LogonType,
    __in_opt HANDLE ProcessIdWithToken,
    __in ULONG SessionId,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
    );

// sessprp

VOID PhShowSessionProperties(
    __in HWND ParentWindowHandle,
    __in ULONG SessionId
    );

// srvcr

VOID PhShowCreateServiceDialog(
    __in HWND ParentWindowHandle
    );

// srvprp

VOID PhShowServiceProperties(
    __in HWND ParentWindowHandle,
    __in PPH_SERVICE_ITEM ServiceItem
    );

// ssndmsg

VOID PhShowSessionSendMessageDialog(
    __in HWND ParentWindowHandle,
    __in ULONG SessionId
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
    __in PVOID Context,
    __in_opt PWSTR Title
    );

HPROPSHEETPAGE PhCreateTokenPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context,
    __in_opt DLGPROC HookProc
    );

#endif
