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

#define WM_WE_RECEIVE_REQUEST (WM_APP + 100)
#define WM_WE_SEND_REPLY (WM_APP + 101)

#define WE_CLIENT_MESSAGE_TIMEOUT 5000
#define WE_CLIENT_MESSAGE_MAGIC ('amEW')

typedef enum _WE_HOOK_REQUEST_TYPE
{
    GetWindowLongHookRequest, // call GetWindowLongPtr, store result in Data
    MaximumHookRequest
} WE_HOOK_REQUEST_TYPE, *PWE_HOOK_REQUEST_TYPE;

typedef struct _WE_HOOK_REQUEST
{
    WE_HOOK_REQUEST_TYPE Type;
    ULONG_PTR Parameter1;
    ULONG_PTR Parameter2;
    ULONG_PTR Parameter3;
    ULONG_PTR Parameter4;
} WE_HOOK_REQUEST, *PWE_HOOK_REQUEST;

typedef struct _WE_HOOK_REPLY
{
    union
    {
        LONG_PTR Data;
    } u;
} WE_HOOK_REPLY, *PWE_HOOK_REPLY;

VOID WeHookServerInitialization();

VOID WeHookServerUninitialization();

BOOLEAN WeSendServerRequest(
    __in HWND hWnd,
    __in PWE_HOOK_REQUEST Request,
    __out PWE_HOOK_REPLY Reply
    );

VOID WeHookClientInitialization();

VOID WeHookClientUninitialization();

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

PVOID WeGetProcedureAddress(
    __in PSTR Name
    );

VOID WeInvertWindowBorder(
    __in HWND hWnd
    );

#endif
