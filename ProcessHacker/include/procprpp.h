#ifndef PROCPRPP_H
#define PROCPRPP_H

LRESULT CALLBACK PhpPropSheetWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID NTAPI PhpProcessPropContextDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

INT CALLBACK PhpPropSheetProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam
    );

VOID NTAPI PhpProcessPropPageContextDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

INT CALLBACK PhpStandardPropPageProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
    );

INT_PTR CALLBACK PhpProcessGeneralDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessStatisticsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessPerformanceDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessThreadsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessTokenHookProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessModulesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessMemoryDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessEnvironmentDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessHandlesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessServicesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#define WM_PH_THREAD_ADDED (WM_APP + 201)
#define WM_PH_THREAD_MODIFIED (WM_APP + 202)
#define WM_PH_THREAD_REMOVED (WM_APP + 203)
#define WM_PH_THREADS_UPDATED (WM_APP + 204)
#define WM_PH_THREAD_SELECTION_CHANGED (WM_APP + 205)

typedef struct _PH_THREADS_CONTEXT
{
    PPH_THREAD_PROVIDER Provider;
    PH_PROVIDER_REGISTRATION ProviderRegistration;
    PH_CALLBACK_REGISTRATION AddedEventRegistration;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;
    PH_CALLBACK_REGISTRATION RemovedEventRegistration;
    PH_CALLBACK_REGISTRATION UpdatedEventRegistration;
    PH_CALLBACK_REGISTRATION LoadingStateChangedEventRegistration;

    HWND WindowHandle;

    PH_THREAD_LIST_CONTEXT ListContext;
    BOOLEAN NeedsRedraw;
} PH_THREADS_CONTEXT, *PPH_THREADS_CONTEXT;

#define WM_PH_MODULE_ADDED (WM_APP + 211)
#define WM_PH_MODULE_MODIFIED (WM_APP + 212)
#define WM_PH_MODULE_REMOVED (WM_APP + 213)
#define WM_PH_MODULES_UPDATED (WM_APP + 214)

typedef struct _PH_MODULES_CONTEXT
{
    PPH_MODULE_PROVIDER Provider;
    PH_PROVIDER_REGISTRATION ProviderRegistration;
    PH_CALLBACK_REGISTRATION AddedEventRegistration;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;
    PH_CALLBACK_REGISTRATION RemovedEventRegistration;
    PH_CALLBACK_REGISTRATION UpdatedEventRegistration;

    HWND WindowHandle;

    PH_MODULE_LIST_CONTEXT ListContext;
    BOOLEAN NeedsRedraw;
} PH_MODULES_CONTEXT, *PPH_MODULES_CONTEXT;

#define WM_PH_HANDLE_ADDED (WM_APP + 221)
#define WM_PH_HANDLE_MODIFIED (WM_APP + 222)
#define WM_PH_HANDLE_REMOVED (WM_APP + 223)
#define WM_PH_HANDLES_UPDATED (WM_APP + 224)

typedef struct _PH_HANDLES_CONTEXT
{
    PPH_HANDLE_PROVIDER Provider;
    PH_PROVIDER_REGISTRATION ProviderRegistration;
    PH_CALLBACK_REGISTRATION AddedEventRegistration;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;
    PH_CALLBACK_REGISTRATION RemovedEventRegistration;
    PH_CALLBACK_REGISTRATION UpdatedEventRegistration;

    HWND WindowHandle;

    PPH_POINTER_LIST HandleList;
    BOOLEAN NeedsRedraw;
    BOOLEAN NeedsSort;
    BOOLEAN SelectedHandleProtected;
    BOOLEAN SelectedHandleInherit;
    BOOLEAN HideUnnamedHandles;
} PH_HANDLES_CONTEXT, *PPH_HANDLES_CONTEXT;

typedef struct _PH_MEMORY_CONTEXT
{
    PH_MEMORY_PROVIDER Provider;

    PPH_LIST MemoryList;
    HWND ListViewHandle;
} PH_MEMORY_CONTEXT, *PPH_MEMORY_CONTEXT;

#define WM_PH_STATISTICS_UPDATE (WM_APP + 231)

typedef struct _PH_STATISTICS_CONTEXT
{
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    HWND WindowHandle;
    BOOLEAN Enabled;
    HANDLE ProcessHandle;
} PH_STATISTICS_CONTEXT, *PPH_STATISTICS_CONTEXT;

#define WM_PH_PERFORMANCE_UPDATE (WM_APP + 241)

typedef struct _PH_PERFORMANCE_CONTEXT
{
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    HWND WindowHandle;

    PH_GRAPH_STATE CpuGraphState;
    PH_GRAPH_STATE PrivateGraphState;
    PH_GRAPH_STATE IoGraphState;

    HWND CpuGraphHandle;
    HWND PrivateGraphHandle;
    HWND IoGraphHandle;
} PH_PERFORMANCE_CONTEXT, *PPH_PERFORMANCE_CONTEXT;

#endif
