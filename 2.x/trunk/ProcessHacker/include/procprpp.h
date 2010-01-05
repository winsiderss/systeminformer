#ifndef PROCPRPP_H
#define PROCPRPP_H

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

INT_PTR CALLBACK PhpProcessModulesDlgProc(
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

#define WM_PH_MODULE_ADDED (WM_APP + 201)
#define WM_PH_MODULE_REMOVED (WM_APP + 202)

typedef struct _PH_MODULES_CONTEXT
{
    PPH_MODULE_PROVIDER Provider;
    PH_PROVIDER_REGISTRATION ProviderRegistration;
    PH_CALLBACK_REGISTRATION AddedEventRegistration;
    PH_CALLBACK_REGISTRATION RemovedEventRegistration;

    HWND WindowHandle;
} PH_MODULES_CONTEXT, *PPH_MODULES_CONTEXT;

#define WM_PH_THREAD_ADDED (WM_APP + 211)
#define WM_PH_THREAD_MODIFIED (WM_APP + 212)
#define WM_PH_THREAD_REMOVED (WM_APP + 213)

typedef struct _PH_THREADS_CONTEXT
{
    PPH_THREAD_PROVIDER Provider;
    PH_PROVIDER_REGISTRATION ProviderRegistration;
    PH_CALLBACK_REGISTRATION AddedEventRegistration;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;
    PH_CALLBACK_REGISTRATION RemovedEventRegistration;

    HWND WindowHandle;
} PH_THREADS_CONTEXT, *PPH_THREADS_CONTEXT;

#endif
