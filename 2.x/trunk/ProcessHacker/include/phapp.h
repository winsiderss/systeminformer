#ifndef PHAPP_H
#define PHAPP_H

#include <phgui.h>
#include <treelist.h>
#include <circbuf.h>
#include <providers.h>
#include <procdb.h>
#include "resource.h"

#define KPH_ERROR_MESSAGE (L"KProcessHacker does not support your operating system " \
            L"or could not be loaded. Make sure Process Hacker is running " \
            L"on a 32-bit system and with administrative privileges.")

#ifndef MAIN_PRIVATE

extern PPH_STRING PhApplicationDirectory;
extern PPH_STRING PhApplicationFileName;
extern PPH_STRING PhLocalSystemName;
extern PPH_STRING PhProcDbFileName;
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
    BOOLEAN ShowOptions;

    HWND WindowHandle;
    POINT Point;

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

// Default columns should go first
#define PHTLC_NAME 0
#define PHTLC_PID 1
#define PHTLC_CPU 2
#define PHTLC_IOTOTAL 3
#define PHTLC_PVTMEMORY 4
#define PHTLC_USERNAME 5
#define PHTLC_DESCRIPTION 6

#define PHTLC_COMPANYNAME 7
#define PHTLC_VERSION 8
#define PHTLC_FILENAME 9
#define PHTLC_COMMANDLINE 10

#define PHTLC_PEAKPVTMEMORY 11
#define PHTLC_WORKINGSET 12
#define PHTLC_PEAKWORKINGSET 13
#define PHTLC_PRIVATEWS 14
#define PHTLC_SHAREDWS 15
#define PHTLC_SHAREABLEWS 16
#define PHTLC_VIRTUALSIZE 17
#define PHTLC_PEAKVIRTUALSIZE 18
#define PHTLC_PAGEFAULTS 19
#define PHTLC_SESSIONID 20
#define PHTLC_PRIORITYCLASS 21
#define PHTLC_BASEPRIORITY 22

#define PHTLC_THREADS 23
#define PHTLC_HANDLES 24
#define PHTLC_GDIHANDLES 25
#define PHTLC_USERHANDLES 26
#define PHTLC_IORO 27
#define PHTLC_IOW 28
#define PHTLC_INTEGRITY 29
#define PHTLC_IOPRIORITY 30
#define PHTLC_PAGEPRIORITY 31
#define PHTLC_STARTTIME 32
#define PHTLC_TOTALCPUTIME 33
#define PHTLC_KERNELCPUTIME 34
#define PHTLC_USERCPUTIME 35
#define PHTLC_VERIFICATIONSTATUS 36
#define PHTLC_VERIFIEDSIGNER 37
#define PHTLC_SAFE 38

#define PHTLC_MAXIMUM 39

#define PHPN_WSCOUNTERS 0x1
#define PHPN_GDIUSERHANDLES 0x2
#define PHPN_IOPAGEPRIORITY 0x4

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

    PPH_PROCDB_ENTRY DbEntry;

    // If the user has selected certain columns we need extra information 
    // that isn't retrieved by the process provider.
    ULONG ValidMask;

    // WS counters
    PH_PROCESS_WS_COUNTERS WsCounters;
    // GDI, USER handles
    ULONG GdiHandles;
    ULONG UserHandles;
    // I/O, Page priority
    ULONG IoPriority;
    ULONG PagePriority;

    PPH_STRING TooltipText;

    PPH_STRING IoTotalText;
    PPH_STRING PrivateMemoryText;
    PPH_STRING PeakPrivateMemoryText;
    PPH_STRING WorkingSetText;
    PPH_STRING PeakWorkingSetText;
    PPH_STRING PrivateWsText;
    PPH_STRING SharedWsText;
    PPH_STRING ShareableWsText;
    PPH_STRING VirtualSizeText;
    PPH_STRING PeakVirtualSizeText;
    PPH_STRING PageFaultsText;
    WCHAR BasePriorityText[PH_INT32_STR_LEN_1];
    WCHAR ThreadsText[PH_INT32_STR_LEN_1];
    WCHAR HandlesText[PH_INT32_STR_LEN_1];
    WCHAR GdiHandlesText[PH_INT32_STR_LEN_1];
    WCHAR UserHandlesText[PH_INT32_STR_LEN_1];
    PPH_STRING IoRoText;
    PPH_STRING IoWText;
    WCHAR IoPriorityText[PH_INT32_STR_LEN_1];
    WCHAR PagePriorityText[PH_INT32_STR_LEN_1];
    PPH_STRING StartTimeText;
    WCHAR TotalCpuTimeText[PH_TIMESPAN_STR_LEN_1];
    WCHAR KernelCpuTimeText[PH_TIMESPAN_STR_LEN_1];
    WCHAR UserCpuTimeText[PH_TIMESPAN_STR_LEN_1];
} PH_PROCESS_NODE, *PPH_PROCESS_NODE;

VOID PhProcessTreeListInitialization();

VOID PhInitializeProcessTreeList(
    __in HWND hwnd
    );

PPH_PROCESS_NODE PhCreateProcessNode(
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

VOID PhCopyProcessTree();

VOID PhWriteProcessTree(
    __inout PPH_FILE_STREAM FileStream
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

PPH_STRING PhUnescapeStringForDelimiter(
    __in PPH_STRING String,
    __in WCHAR Delimiter
    );

PPH_STRING PhEscapeStringForDelimiter(
    __in PPH_STRING String,
    __in WCHAR Delimiter
    );

typedef struct mxml_node_s mxml_node_t;

PPH_STRING PhGetOpaqueXmlNodeText(
    __in mxml_node_t *node
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

VOID PhLoadSymbolProviderOptions(
    __inout PPH_SYMBOL_PROVIDER SymbolProvider
    );

VOID PhSetExtendedListViewWithSettings(
    __in HWND hWnd
    );

VOID PhCopyListViewInfoTip(
    __inout LPNMLVGETINFOTIP GetInfoTip,
    __in PPH_STRINGREF Tip
    );

VOID PhCopyListView(
    __in HWND ListViewHandle
    );

VOID PhHandleListViewNotifyForCopy(
    __in LPARAM lParam,
    __in HWND ListViewHandle
    );

VOID PhLoadWindowPlacementFromSetting(
    __in PWSTR PositionSettingName,
    __in PWSTR SizeSettingName,
    __in HWND WindowHandle
    );

VOID PhSaveWindowPlacementToSetting(
    __in PWSTR PositionSettingName,
    __in PWSTR SizeSettingName,
    __in HWND WindowHandle
    );

VOID PhLoadListViewColumnsFromSetting(
    __in PWSTR Name,
    __in HWND ListViewHandle
    );

VOID PhSaveListViewColumnsToSetting(
    __in PWSTR Name,
    __in HWND ListViewHandle
    );

VOID PhLoadTreeListColumnsFromSetting(
    __in PWSTR Name,
    __in HWND TreeListHandle
    );

VOID PhSaveTreeListColumnsToSetting(
    __in PWSTR Name,
    __in HWND TreeListHandle
    );

PPH_STRING PhGetPhVersion();

VOID PhWritePhTextHeader(
    __inout PPH_FILE_STREAM FileStream
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
#define WM_PH_NOTIFY_ICON_MESSAGE (WM_APP + 126)
#define WM_PH_TOGGLE_VISIBLE (WM_APP + 127)
#define WM_PH_SHOW_MEMORY_EDITOR (WM_APP + 128)
#define WM_PH_SHOW_MEMORY_RESULTS (WM_APP + 129)

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
#define ProcessHacker_ToggleVisible(hWnd) \
    SendMessage(hWnd, WM_PH_TOGGLE_VISIBLE, 0, 0)
#define ProcessHacker_ShowMemoryEditor(hWnd, ShowMemoryEditor) \
    PostMessage(hWnd, WM_PH_SHOW_MEMORY_EDITOR, 0, (LPARAM)(ShowMemoryEditor))
#define ProcessHacker_ShowMemoryResults(hWnd, ShowMemoryResults) \
    PostMessage(hWnd, WM_PH_SHOW_MEMORY_RESULTS, 0, (LPARAM)(ShowMemoryResults))

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

VOID PhModifyNotifyIcon(
    __in ULONG Id,
    __in ULONG Flags,
    __in_opt PWSTR Text,
    __in_opt HICON Icon
    );

VOID PhShowBalloonTipNotifyIcon(
    __in ULONG Id,
    __in PWSTR Title,
    __in PWSTR Text,
    __in ULONG Timeout,
    __in ULONG Flags
    );

HICON PhBitmapToIcon(
    __in HBITMAP Bitmap
    );

VOID PhUpdateIconCpuHistory();

VOID PhUpdateIconIoHistory();

VOID PhUpdateIconCommitHistory();

VOID PhUpdateIconPhysicalHistory();

VOID PhUpdateIconCpuUsage();

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

#ifndef LOG_PRIVATE
PH_CIRCULAR_BUFFER_PVOID PhLogBuffer;
PH_CALLBACK PhLoggedCallback;
#endif

VOID PhLogInitialization();

VOID PhClearLogEntries();

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

VOID PhLogMessageEntry(
    __in UCHAR Type,
    __in PPH_STRING Message
    );

PPH_STRING PhFormatLogEntry(
    __in PPH_LOG_ENTRY Entry
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

BOOLEAN PhUiFreeMemory(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_MEMORY_ITEM MemoryItem,
    __in BOOLEAN Free
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

// chcol

VOID PhShowChooseColumnsDialog(
    __in HWND ParentWindowHandle,
    __in HWND TreeListHandle
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

// cpysave

PPH_FULL_STRING PhGetProcessTreeListText(
    __in HWND TreeListHandle
    );

PPH_LIST PhGetProcessTreeListLines(
    __in HWND TreeListHandle,
    __in ULONG NumberOfNodes,
    __in PPH_LIST RootNodes,
    __in BOOLEAN UseTabs
    );

PPH_FULL_STRING PhGetListViewText(
    __in HWND ListViewHandle
    );

// findobj

VOID PhShowFindObjectsDialog();

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

// logwnd

VOID PhShowLogDialog();

// memedit

VOID PhShowMemoryEditorDialog(
    __in HANDLE ProcessId,
    __in PVOID BaseAddress,
    __in SIZE_T RegionSize,
    __in ULONG SelectOffset,
    __in ULONG SelectLength
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
    __in ULONG SessionId,
    __in PWSTR DesktopName
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
    __in_opt PWSTR DesktopName,
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

// sysinfo

VOID PhShowSystemInformationDialog();

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
