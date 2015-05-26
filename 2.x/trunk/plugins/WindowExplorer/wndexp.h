#ifndef WNDEXP_H
#define WNDEXP_H

#include <phdk.h>
#include "wndtree.h"

extern BOOLEAN IsHookClient;
extern PPH_PLUGIN PluginInstance;

#define PLUGIN_NAME L"ProcessHacker.WindowExplorer"
#define SETTING_NAME_SHOW_DESKTOP_WINDOWS (PLUGIN_NAME L".ShowDesktopWindows")
#define SETTING_NAME_WINDOW_TREE_LIST_COLUMNS (PLUGIN_NAME L".WindowTreeListColumns")
#define SETTING_NAME_WINDOWS_WINDOW_POSITION (PLUGIN_NAME L".WindowsWindowPosition")
#define SETTING_NAME_WINDOWS_WINDOW_SIZE (PLUGIN_NAME L".WindowsWindowSize")

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
    _Out_ PWE_HOOK_SHARED_DATA *Data
    );

VOID WeUnlockServerSharedData(
    VOID
    );

BOOLEAN WeSendServerRequest(
    _In_ HWND hWnd
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
    _In_ HWND ParentWindowHandle,
    _In_ PWE_WINDOW_SELECTOR Selector
    );

#define WM_WE_PLUSMINUS (WM_APP + 1)

// wndprp

VOID WeShowWindowProperties(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle
    );

// utils

#define WE_PhMainWndHandle (*(HWND *)WeGetProcedureAddress("PhMainWndHandle"))
#define WE_WindowsVersion (*(ULONG *)WeGetProcedureAddress("WindowsVersion"))

PVOID WeGetProcedureAddress(
    _In_ PSTR Name
    );

VOID WeFormatLocalObjectName(
    _In_ PWSTR OriginalName,
    _Inout_updates_(256) PWCHAR Buffer,
    _Out_ PUNICODE_STRING ObjectName
    );

VOID WeInvertWindowBorder(
    _In_ HWND hWnd
    );

#endif
