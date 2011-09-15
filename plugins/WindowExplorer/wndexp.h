#ifndef WNDEXP_H
#define WNDEXP_H

#include <phdk.h>
#include "wndtree.h"

extern BOOLEAN IsHookClient;
extern PPH_PLUGIN PluginInstance;

#define SETTING_PREFIX L"ProcessHacker.WindowExplorer."
#define SETTING_NAME_WINDOW_TREE_LIST_COLUMNS (SETTING_PREFIX L"WindowTreeListColumns")
#define SETTING_NAME_WINDOWS_WINDOW_POSITION (SETTING_PREFIX L"WindowsWindowPosition")
#define SETTING_NAME_WINDOWS_WINDOW_SIZE (SETTING_PREFIX L"WindowsWindowSize")

// hook

#define WE_SERVER_MESSAGE_NAME L"WE_ServerMessage"
#define WE_SERVER_SHARED_SECTION_NAME L"\\BaseNamedObjects\\WeSharedSection"
#define WE_SERVER_SHARED_SECTION_LOCK_NAME L"\\BaseNamedObjects\\WeSharedSectionLock"
#define WE_SERVER_SHARED_SECTION_EVENT_NAME L"\\BaseNamedObjects\\WeSharedSectionEvent"
#define WE_CLIENT_MESSAGE_TIMEOUT 2000

typedef struct _WE_HOOK_SHARED_DATA
{
    ULONG MessageId;

    struct
    {
        ULONG_PTR WndProc;
        ULONG_PTR DlgProc;
        WNDCLASSEX ClassInfo;
    } c;
} WE_HOOK_SHARED_DATA, *PWE_HOOK_SHARED_DATA;

VOID WeHookServerInitialization(
    VOID
    );

VOID WeHookServerUninitialization(
    VOID
    );

BOOLEAN WeIsServerActive(
    VOID
    );

BOOLEAN WeLockServerSharedData(
    __out PWE_HOOK_SHARED_DATA *Data
    );

VOID WeUnlockServerSharedData(
    VOID
    );

BOOLEAN WeSendServerRequest(
    __in HWND hWnd
    );

VOID WeHookClientInitialization(
    VOID
    );

VOID WeHookClientUninitialization(
    VOID
    );

// wnddlg

typedef enum _WE_WINDOW_SELECTOR_TYPE
{
    WeWindowSelectorAll,
    WeWindowSelectorProcess,
    WeWindowSelectorThread,
    WeWindowSelectorDesktop
} WE_WINDOW_SELECTOR_TYPE;

typedef struct _WE_WINDOW_SELECTOR
{
    WE_WINDOW_SELECTOR_TYPE Type;
    union
    {
        struct
        {
            HANDLE ProcessId;
        } Process;
        struct
        {
            HANDLE ThreadId;
        } Thread;
        struct
        {
            PPH_STRING DesktopName;
        } Desktop;
    };
} WE_WINDOW_SELECTOR, *PWE_WINDOW_SELECTOR;

VOID WeShowWindowsDialog(
    __in HWND ParentWindowHandle,
    __in PWE_WINDOW_SELECTOR Selector
    );

#define WM_WE_PLUSMINUS (WM_APP + 1)

// wndprp

VOID WeShowWindowProperties(
    __in HWND ParentWindowHandle,
    __in HWND WindowHandle
    );

// utils

#define WE_PhMainWndHandle (*(HWND *)WeGetProcedureAddress("PhMainWndHandle"))
#define WE_WindowsVersion (*(ULONG *)WeGetProcedureAddress("WindowsVersion"))

PVOID WeGetProcedureAddress(
    __in PSTR Name
    );

VOID WeFormatLocalObjectName(
    __in PWSTR OriginalName,
    __inout_ecount(256) PWCHAR Buffer,
    __out PUNICODE_STRING ObjectName
    );

VOID WeInvertWindowBorder(
    __in HWND hWnd
    );

#endif
