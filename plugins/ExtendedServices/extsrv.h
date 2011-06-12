#ifndef ES_EXTSRV_H
#define ES_EXTSRV_H

// main

#ifndef MAIN_PRIVATE
extern PPH_PLUGIN PluginInstance;
#endif

#define SETTING_PREFIX L"ProcessHacker.ExtendedServices."
#define SETTING_NAME_ENABLE_SERVICES_MENU (SETTING_PREFIX L"EnableServicesMenu")

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

// options

VOID EsShowOptionsDialog(
    __in HWND ParentWindowHandle
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
    __in PPH_SERVICE_ITEM ServiceItem,
    __in SC_HANDLE ServiceHandle
    );

// trigger

struct _ES_TRIGGER_CONTEXT;

struct _ES_TRIGGER_CONTEXT *EsCreateServiceTriggerContext(
    __in PPH_SERVICE_ITEM ServiceItem,
    __in HWND WindowHandle,
    __in HWND TriggersLv
    );

VOID EsDestroyServiceTriggerContext(
    __in struct _ES_TRIGGER_CONTEXT *Context
    );

VOID EsLoadServiceTriggerInfo(
    __in struct _ES_TRIGGER_CONTEXT *Context,
    __in SC_HANDLE ServiceHandle
    );

BOOLEAN EsSaveServiceTriggerInfo(
    __in struct _ES_TRIGGER_CONTEXT *Context,
    __out PULONG Win32Result
    );

#define ES_TRIGGER_EVENT_NEW 1
#define ES_TRIGGER_EVENT_EDIT 2
#define ES_TRIGGER_EVENT_DELETE 3
#define ES_TRIGGER_EVENT_SELECTIONCHANGED 4

VOID EsHandleEventServiceTrigger(
    __in struct _ES_TRIGGER_CONTEXT *Context,
    __in ULONG Event
    );

#endif
