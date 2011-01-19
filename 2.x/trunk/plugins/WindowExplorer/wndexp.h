#ifndef WNDEXP_H
#define WNDEXP_H

#include <phdk.h>
#include "wndtree.h"

extern PPH_PLUGIN PluginInstance;

#define SETTING_PREFIX L"ProcessHacker.WindowExplorer."

// wnddlg

typedef enum _WE_WINDOW_SELECTOR_TYPE
{
    WeWindowSelectorAll,
    WeWindowSelectorProcess,
    WeWindowSelectorThread
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
    };
} WE_WINDOW_SELECTOR, *PWE_WINDOW_SELECTOR;

HWND WeCreateWindowsDialog(
    __in HWND ParentWindowHandle,
    __in PWE_WINDOW_SELECTOR Selector
    );

#define WM_WE_PLUSMINUS (WM_APP + 1)

// utils

VOID WeInvertWindowBorder(
    __in HWND hWnd
    );

#endif
