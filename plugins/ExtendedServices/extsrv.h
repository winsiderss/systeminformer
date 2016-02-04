#ifndef ES_EXTSRV_H
#define ES_EXTSRV_H

#include <phdk.h>
#include <windowsx.h>

#include "resource.h"

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

typedef struct _SERVICE_LIST_CONTEXT
{
    HWND ServiceListHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} SERVICE_LIST_CONTEXT, *PSERVICE_LIST_CONTEXT;

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

typedef struct _SERVICE_OTHER_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;

    struct
    {
        ULONG Ready : 1;
        ULONG Dirty : 1;
        ULONG PreshutdownTimeoutValid : 1;
        ULONG RequiredPrivilegesValid : 1;
        ULONG SidTypeValid : 1;
        ULONG LaunchProtectedValid : 1;
    };
    HWND PrivilegesLv;
    PPH_LIST PrivilegeList;

    ULONG OriginalLaunchProtected;
} SERVICE_OTHER_CONTEXT, *PSERVICE_OTHER_CONTEXT;

#define SIP(String, Integer) { (String), (PVOID)(Integer) }

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

typedef struct _RESTART_SERVICE_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    SC_HANDLE ServiceHandle;
    BOOLEAN Starting;
    BOOLEAN DisableTimer;
} RESTART_SERVICE_CONTEXT, *PRESTART_SERVICE_CONTEXT;

VOID EsRestartServiceWithProgress(
    _In_ HWND hWnd,
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ SC_HANDLE ServiceHandle
    );

// trigger

typedef struct _ES_TRIGGER_DATA
{
    ULONG Type;
    union
    {
        PPH_STRING String;
        struct
        {
            PVOID Binary;
            ULONG BinaryLength;
        };
        UCHAR Byte;
        ULONG64 UInt64;
    };
} ES_TRIGGER_DATA, *PES_TRIGGER_DATA;

typedef struct _ES_TRIGGER_INFO
{
    ULONG Type;
    PGUID Subtype;
    ULONG Action;
    PPH_LIST DataList;
    GUID SubtypeBuffer;
} ES_TRIGGER_INFO, *PES_TRIGGER_INFO;

typedef struct _ES_TRIGGER_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    HWND WindowHandle;
    HWND TriggersLv;
    BOOLEAN Dirty;
    ULONG InitialNumberOfTriggers;
    PPH_LIST InfoList;

    // Trigger dialog box
    PES_TRIGGER_INFO EditingInfo;
    ULONG LastSelectedType;
    PPH_STRING LastCustomSubType;

    // Value dialog box
    PPH_STRING EditingValue;
} ES_TRIGGER_CONTEXT, *PES_TRIGGER_CONTEXT;

typedef struct _TYPE_ENTRY
{
    ULONG Type;
    PWSTR Name;
} TYPE_ENTRY, PTYPE_ENTRY;

typedef struct _SUBTYPE_ENTRY
{
    ULONG Type;
    PGUID Guid;
    PWSTR Name;
} SUBTYPE_ENTRY, PSUBTYPE_ENTRY;

typedef struct _ETW_PUBLISHER_ENTRY
{
    PPH_STRING PublisherName;
    GUID Guid;
} ETW_PUBLISHER_ENTRY, *PETW_PUBLISHER_ENTRY;

PES_TRIGGER_CONTEXT EsCreateServiceTriggerContext(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ HWND WindowHandle,
    _In_ HWND TriggersLv
    );

VOID EsDestroyServiceTriggerContext(
    _In_ PES_TRIGGER_CONTEXT Context
    );

VOID EsLoadServiceTriggerInfo(
    _In_ PES_TRIGGER_CONTEXT Context,
    _In_ SC_HANDLE ServiceHandle
    );

BOOLEAN EsSaveServiceTriggerInfo(
    _In_ PES_TRIGGER_CONTEXT Context,
    _Out_ PULONG Win32Result
    );

#define ES_TRIGGER_EVENT_NEW 1
#define ES_TRIGGER_EVENT_EDIT 2
#define ES_TRIGGER_EVENT_DELETE 3
#define ES_TRIGGER_EVENT_SELECTIONCHANGED 4

VOID EsHandleEventServiceTrigger(
    _In_ PES_TRIGGER_CONTEXT Context,
    _In_ ULONG Event
    );

// triggpg

typedef struct _SERVICE_TRIGGERS_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    HWND TriggersLv;
    PES_TRIGGER_CONTEXT TriggerContext;
} SERVICE_TRIGGERS_CONTEXT, *PSERVICE_TRIGGERS_CONTEXT;

INT_PTR CALLBACK EspServiceTriggersDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif
