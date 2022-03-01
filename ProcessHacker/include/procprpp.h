#ifndef PH_PROCPRPP_H
#define PH_PROCPRPP_H

#include <thrdlist.h>
#include <modlist.h>
#include <hndllist.h>
#include <memlist.h>
#include <memprv.h>

typedef struct _PH_PROCESS_PROPSHEETCONTEXT
{
    WNDPROC PropSheetWindowHookProc;
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LAYOUT_ITEM TabPageItem;
    BOOLEAN LayoutInitialized;
} PH_PROCESS_PROPSHEETCONTEXT, *PPH_PROCESS_PROPSHEETCONTEXT;

VOID NTAPI PhpProcessPropContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

INT CALLBACK PhpPropSheetProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam
    );

PPH_PROCESS_PROPSHEETCONTEXT PhpGetPropSheetContext(
    _In_ HWND hwnd
    );

LRESULT CALLBACK PhpPropSheetWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID NTAPI PhpProcessPropPageContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

INT CALLBACK PhpStandardPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

VOID NTAPI PhpProcessPropPageWaitContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

VOID PhpCreateProcessPropSheetWaitContext(
    _In_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ HWND WindowHandle
    );

VOID PhpFlushProcessPropSheetWaitContextData(
    VOID
    );

VOID CALLBACK PhpProcessPropertiesWaitCallback(
    _In_ PVOID Context,
    _In_ BOOLEAN TimerOrWaitFired
    );

#define SET_BUTTON_ICON(Id, Icon) \
    SendMessage(GetDlgItem(hwndDlg, (Id)), BM_SETIMAGE, IMAGE_ICON, (LPARAM)(Icon))

INT_PTR CALLBACK PhpProcessGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessStatisticsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessPerformanceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessThreadsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

NTSTATUS NTAPI PhpOpenProcessTokenForPage(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    );

INT_PTR CALLBACK PhpProcessTokenHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessModulesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessMemoryDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessEnvironmentDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessHandlesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

NTSTATUS NTAPI PhpOpenProcessJobForPage(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    );

INT_PTR CALLBACK PhpProcessJobHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessServicesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessWmiProvidersDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#ifdef _M_IX86
INT_PTR CALLBACK PhpProcessVdmHostProcessDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );
#endif

extern PH_STRINGREF PhpLoadingText;

#define WM_PH_THREADS_UPDATED (WM_APP + 200)
#define WM_PH_THREAD_SELECTION_CHANGED (WM_APP + 201)

// begin_phapppub
typedef struct _PH_THREADS_CONTEXT
{
    PPH_THREAD_PROVIDER Provider;
    PH_CALLBACK_REGISTRATION ProviderRegistration;
    PH_CALLBACK_REGISTRATION AddedEventRegistration;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;
    PH_CALLBACK_REGISTRATION RemovedEventRegistration;
    PH_CALLBACK_REGISTRATION UpdatedEventRegistration;
    PH_CALLBACK_REGISTRATION LoadingStateChangedEventRegistration;

    HWND WindowHandle;
// end_phapppub
    HWND TreeNewHandle;
    HWND StatusHandle;
    HWND SearchboxHandle;
    PPH_STRING SearchboxText;
    PPH_TN_FILTER_ENTRY FilterEntry;
    union
    {
        PH_THREAD_LIST_CONTEXT ListContext;
        struct
        {
            HWND Private; // phapppub
            HWND TreeNewHandle; // phapppub
        } PublicUse;
    };
    PH_PROVIDER_EVENT_QUEUE EventQueue;

    PH_CALLBACK_REGISTRATION SymbolProviderEventRegistration;
// begin_phapppub
} PH_THREADS_CONTEXT, *PPH_THREADS_CONTEXT;
// end_phapppub

#define WM_PH_MODULES_UPDATED (WM_APP + 210)

// begin_phapppub
typedef struct _PH_MODULES_CONTEXT
{
    PPH_MODULE_PROVIDER Provider;
    PH_PROVIDER_REGISTRATION ProviderRegistration;
    PH_CALLBACK_REGISTRATION AddedEventRegistration;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;
    PH_CALLBACK_REGISTRATION RemovedEventRegistration;
    PH_CALLBACK_REGISTRATION UpdatedEventRegistration;

    HWND WindowHandle;
// end_phapppub
    HWND SearchboxHandle;
    HWND TreeNewHandle;
    union
    {
        PH_MODULE_LIST_CONTEXT ListContext;
        struct
        {
            HWND Private; // phapppub
            HWND TreeNewHandle; // phapppub
        } PublicUse;
    };
    PH_PROVIDER_EVENT_QUEUE EventQueue;
    NTSTATUS LastRunStatus;
    PPH_STRING ErrorMessage;
    PPH_STRING SearchboxText;
    PPH_TN_FILTER_ENTRY FilterEntry;
// begin_phapppub
} PH_MODULES_CONTEXT, *PPH_MODULES_CONTEXT;
// end_phapppub

#define WM_PH_HANDLES_UPDATED (WM_APP + 220)

// begin_phapppub
typedef struct _PH_HANDLES_CONTEXT
{
    PPH_HANDLE_PROVIDER Provider;
    PH_PROVIDER_REGISTRATION ProviderRegistration;
    PH_CALLBACK_REGISTRATION AddedEventRegistration;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;
    PH_CALLBACK_REGISTRATION RemovedEventRegistration;
    PH_CALLBACK_REGISTRATION UpdatedEventRegistration;

    HWND WindowHandle;
// end_phapppub
    HWND TreeNewHandle;
    HWND SearchWindowHandle;

    union
    {
        PH_HANDLE_LIST_CONTEXT ListContext;
        struct
        {
            HWND Private; // phapppub
            HWND TreeNewHandle; // phapppub
        } PublicUse;
    };
    PH_PROVIDER_EVENT_QUEUE EventQueue;
    BOOLEAN SelectedHandleProtected;
    BOOLEAN SelectedHandleInherit;
    NTSTATUS LastRunStatus;
    PPH_STRING ErrorMessage;

    PPH_STRING SearchboxText;
    PPH_TN_FILTER_ENTRY FilterEntry;
    ULONG64 SearchPointer;
    BOOLEAN UseSearchPointer;
// begin_phapppub
} PH_HANDLES_CONTEXT, *PPH_HANDLES_CONTEXT;
// end_phapppub

// begin_phapppub
typedef struct _PH_MEMORY_CONTEXT
{
    HANDLE ProcessId;
    HWND WindowHandle;
// end_phapppub
    HWND TreeNewHandle;
    HWND SearchboxHandle;

    union
    {
        PH_MEMORY_LIST_CONTEXT ListContext;
        struct
        {
            HWND Private; // phapppub
            HWND TreeNewHandle; // phapppub
        } PublicUse;
    };
    PH_MEMORY_ITEM_LIST MemoryItemList;
    BOOLEAN MemoryItemListValid;
    NTSTATUS LastRunStatus;
    PPH_STRING ErrorMessage;

    BOOLEAN UseSearchPointer;
    ULONG64 SearchPointer;
    PPH_STRING SearchboxText;
    PPH_TN_FILTER_ENTRY AllocationFilterEntry;
    PPH_TN_FILTER_ENTRY FilterEntry;
// begin_phapppub
} PH_MEMORY_CONTEXT, *PPH_MEMORY_CONTEXT;
// end_phapppub

#define WM_PH_STATISTICS_UPDATE (WM_APP + 231)

typedef struct _PH_STATISTICS_CONTEXT
{
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    HWND WindowHandle;
    HWND ListViewHandle;
    BOOLEAN Enabled;
    HANDLE ProcessHandle;
    PPH_PROCESS_ITEM ProcessItem;

    BOOLEAN GotCycles;
    BOOLEAN GotWsCounters;
    BOOLEAN GotUptime;

    ULONG PagePriority;
    IO_PRIORITY_HINT IoPriority;
    ULONG PeakHandleCount;
    ULONG HangCount;
    ULONG GhostCount;
    ULONGLONG RunningTime;
    ULONGLONG SuspendedTime;

    PPH_STRING Cycles;
    PPH_STRING PrivateWs;
    PPH_STRING ShareableWs;
    PPH_STRING SharedWs;
    PPH_STRING PeakHandles;
    PPH_STRING GdiHandles;
    PPH_STRING UserHandles;
    PSYSTEM_PROCESS_INFORMATION_EXTENSION ProcessExtension;

    PPH_STRING SharedCommitUsage;
    PPH_STRING PrivateCommitUsage;
    PPH_STRING PeakPrivateCommitUsage;
    //PPH_STRING PrivateCommitLimit;
    //PPH_STRING TotalCommitLimit;
} PH_STATISTICS_CONTEXT, *PPH_STATISTICS_CONTEXT;

#define WM_PH_PERFORMANCE_UPDATE (WM_APP + 241)

typedef struct _PH_PERFORMANCE_CONTEXT
{
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    HWND WindowHandle;
    BOOLEAN Enabled;

    PH_GRAPH_STATE CpuGraphState;
    PH_GRAPH_STATE PrivateGraphState;
    PH_GRAPH_STATE IoGraphState;

    HWND CpuGraphHandle;
    HWND PrivateGraphHandle;
    HWND IoGraphHandle;

    HWND CpuGroupBox;
    HWND PrivateBytesGroupBox;
    HWND IoGroupBox;
} PH_PERFORMANCE_CONTEXT, *PPH_PERFORMANCE_CONTEXT;

typedef struct _PH_ENVIRONMENT_ITEM
{
    PPH_STRING Name;
    PPH_STRING Value;
} PH_ENVIRONMENT_ITEM, *PPH_ENVIRONMENT_ITEM;

typedef struct _PH_ENVIRONMENT_CONTEXT
{
    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND SearchWindowHandle;

    BOOLEAN EnableStateHighlighting;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG Reserved : 1;
            ULONG HideProcessEnvironment : 1;
            ULONG HideUserEnvironment : 1;
            ULONG HideSystemEnvironment : 1;
            ULONG HighlightProcessEnvironment : 1;
            ULONG HighlightUserEnvironment : 1;
            ULONG HighlightSystemEnvironment : 1;
            ULONG HideCmdTypeEnvironment : 1;
            ULONG HighlightCmdEnvironment : 1;
            ULONG Spare : 23;
        };
    };

    PPH_PROCESS_ITEM ProcessItem;   
    PPH_STRING SearchboxText;
    PPH_STRING StatusMessage;

    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
    PPH_HASHTABLE NodeHashtable;
    PPH_TN_FILTER_ENTRY TreeFilterEntry;
    ULONG TreeNewSortColumn;
    PH_TN_FILTER_SUPPORT TreeFilterSupport;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;

    PH_ARRAY Items;
} PH_ENVIRONMENT_CONTEXT, *PPH_ENVIRONMENT_CONTEXT;

#endif
