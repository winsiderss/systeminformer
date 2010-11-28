#ifndef ES_EXTSRV_H
#define ES_EXTSRV_H

// main

#ifndef MAIN_PRIVATE
extern PPH_PLUGIN PluginInstance;
#endif

// depend

LPENUM_SERVICE_STATUS EsEnumDependentServices(
    __in SC_HANDLE ServiceHandle,
    __in_opt ULONG State,
    __out PULONG Count
    );

INT_PTR CALLBACK EspServiceDependenciesDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK EspServiceDependentsDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

// other

INT_PTR CALLBACK EspServiceOtherDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

// recovery

INT_PTR CALLBACK EspServiceRecoveryDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK EspServiceRecovery2DlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

// srvprgrs

VOID EsRestartServiceWithProgress(
    __in HWND hWnd,
    __in PWSTR ServiceName,
    __in SC_HANDLE ServiceHandle
    );

#endif
