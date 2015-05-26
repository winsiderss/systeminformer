#ifndef ES_EXTSRV_H
#define ES_EXTSRV_H

// main

typedef NTSTATUS (NTAPI *_RtlCreateServiceSid)(
    _In_ PUNICODE_STRING ServiceName,
    _Out_writes_bytes_opt_(*ServiceSidLength) PSID ServiceSid,
    _Inout_ PULONG ServiceSidLength
    );

extern PPH_PLUGIN PluginInstance;
extern _RtlCreateServiceSid RtlCreateServiceSid_I;

#define PLUGIN_NAME L"ProcessHacker.ExtendedServices"
#define SETTING_NAME_ENABLE_SERVICES_MENU (PLUGIN_NAME L".EnableServicesMenu")

// depend

LPENUM_SERVICE_STATUS EsEnumDependentServices(
    _In_ SC_HANDLE ServiceHandle,
    _In_opt_ ULONG State,
    _Out_ PULONG Count
    );

INT_PTR CALLBACK EspServiceDependenciesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK EspServiceDependentsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// options

VOID EsShowOptionsDialog(
    _In_ HWND ParentWindowHandle
    );

// other

INT_PTR CALLBACK EspServiceOtherDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// recovery

INT_PTR CALLBACK EspServiceRecoveryDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK EspServiceRecovery2DlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// srvprgrs

VOID EsRestartServiceWithProgress(
    _In_ HWND hWnd,
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ SC_HANDLE ServiceHandle
    );

// trigger

struct _ES_TRIGGER_CONTEXT;

struct _ES_TRIGGER_CONTEXT *EsCreateServiceTriggerContext(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ HWND WindowHandle,
    _In_ HWND TriggersLv
    );

VOID EsDestroyServiceTriggerContext(
    _In_ struct _ES_TRIGGER_CONTEXT *Context
    );

VOID EsLoadServiceTriggerInfo(
    _In_ struct _ES_TRIGGER_CONTEXT *Context,
    _In_ SC_HANDLE ServiceHandle
    );

BOOLEAN EsSaveServiceTriggerInfo(
    _In_ struct _ES_TRIGGER_CONTEXT *Context,
    _Out_ PULONG Win32Result
    );

#define ES_TRIGGER_EVENT_NEW 1
#define ES_TRIGGER_EVENT_EDIT 2
#define ES_TRIGGER_EVENT_DELETE 3
#define ES_TRIGGER_EVENT_SELECTIONCHANGED 4

VOID EsHandleEventServiceTrigger(
    _In_ struct _ES_TRIGGER_CONTEXT *Context,
    _In_ ULONG Event
    );

// triggpg

INT_PTR CALLBACK EspServiceTriggersDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif
