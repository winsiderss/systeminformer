#ifndef WNDEXP_H
#define WNDEXP_H

#include <phdk.h>
#include "wndtree.h"

extern PPH_PLUGIN PluginInstance;

#define SETTING_PREFIX L"ProcessHacker.WindowExplorer."
#define SETTING_NAME_WINDOW_TREE_LIST_COLUMNS (SETTING_PREFIX L"WindowTreeListColumns")
#define SETTING_NAME_WINDOWS_WINDOW_SIZE (SETTING_PREFIX L"WindowsWindowSize")

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

VOID PhShowWindowProperties(
    __in HWND ParentWindowHandle,
    __in HWND WindowHandle
    );

// utils

VOID WeInvertWindowBorder(
    __in HWND hWnd
    );

#endif
