#ifndef PH_PROCPRPP_H
#define PH_PROCPRPP_H

typedef struct _PH_PROCESS_PROPSHEETCONTEXT
{
    WNDPROC OldWndProc;
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

typedef struct _PH_HANDLE_ITEM_INFO
{
    HANDLE ProcessId;
    HANDLE Handle;
    PPH_STRING TypeName;
    PPH_STRING BestObjectName;
} PH_HANDLE_ITEM_INFO, *PPH_HANDLE_ITEM_INFO;

#define PhaAppendCtrlEnter(Text, Enable) ((Enable) ? PhaConcatStrings2((Text), L"\tCtrl+Enter")->Buffer : (Text))

VOID PhInsertHandleObjectPropertiesEMenuItems(
    _In_ struct _PH_EMENU_ITEM *Menu,
    _In_ ULONG InsertBeforeId,
    _In_ BOOLEAN EnableShortcut,
    _In_ PPH_HANDLE_ITEM_INFO Info
    );

#define PH_MAX_SECTION_EDIT_SIZE (32 * 1024 * 1024) // 32 MB

VOID PhShowHandleObjectProperties1(
    _In_ HWND hWnd,
    _In_ PPH_HANDLE_ITEM_INFO Info
    );

VOID PhShowHandleObjectProperties2(
    _In_ HWND hWnd,
    _In_ PPH_HANDLE_ITEM_INFO Info
    );

INT_PTR CALLBACK PhpProcessHandlesDlgProc(
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

#define WM_PH_THREAD_ADDED (WM_APP + 201)
#define WM_PH_THREAD_MODIFIED (WM_APP + 202)
#define WM_PH_THREAD_REMOVED (WM_APP + 203)
#define WM_PH_THREADS_UPDATED (WM_APP + 204)
#define WM_PH_THREAD_SELECTION_CHANGED (WM_APP + 205)

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

    union
    {
        PH_THREAD_LIST_CONTEXT ListContext;
        struct
        {
            HWND Private; // phapppub
            HWND TreeNewHandle; // phapppub
        } PublicUse;
    };
    BOOLEAN NeedsRedraw;
// begin_phapppub
} PH_THREADS_CONTEXT, *PPH_THREADS_CONTEXT;
// end_phapppub

#define WM_PH_MODULE_ADDED (WM_APP + 211)
#define WM_PH_MODULE_MODIFIED (WM_APP + 212)
#define WM_PH_MODULE_REMOVED (WM_APP + 213)
#define WM_PH_MODULES_UPDATED (WM_APP + 214)

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

    union
    {
        PH_MODULE_LIST_CONTEXT ListContext;
        struct
        {
            HWND Private; // phapppub
            HWND TreeNewHandle; // phapppub
        } PublicUse;
    };
    BOOLEAN NeedsRedraw;
    NTSTATUS LastRunStatus;
    PPH_STRING ErrorMessage;
// begin_phapppub
} PH_MODULES_CONTEXT, *PPH_MODULES_CONTEXT;
// end_phapppub

#define WM_PH_HANDLE_ADDED (WM_APP + 221)
#define WM_PH_HANDLE_MODIFIED (WM_APP + 222)
#define WM_PH_HANDLE_REMOVED (WM_APP + 223)
#define WM_PH_HANDLES_UPDATED (WM_APP + 224)

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

    union
    {
        PH_HANDLE_LIST_CONTEXT ListContext;
        struct
        {
            HWND Private; // phapppub
            HWND TreeNewHandle; // phapppub
        } PublicUse;
    };
    BOOLEAN NeedsRedraw;
    BOOLEAN SelectedHandleProtected;
    BOOLEAN SelectedHandleInherit;
    NTSTATUS LastRunStatus;
    PPH_STRING ErrorMessage;
// begin_phapppub
} PH_HANDLES_CONTEXT, *PPH_HANDLES_CONTEXT;
// end_phapppub

// begin_phapppub
typedef struct _PH_MEMORY_CONTEXT
{
    HANDLE ProcessId;
    HWND WindowHandle;
// end_phapppub

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
// begin_phapppub
} PH_MEMORY_CONTEXT, *PPH_MEMORY_CONTEXT;
// end_phapppub

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
