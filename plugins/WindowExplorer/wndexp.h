#ifndef WNDEXP_H
#define WNDEXP_H

#include <phdk.h>
#include <phapppub.h>
#include <phappresource.h>
#include <settings.h>

#include <commonutil.h>

#include "resource.h"
#include "wndtree.h"
#include "prpsh.h"

extern PPH_PLUGIN PluginInstance;

#define PLUGIN_NAME L"ProcessHacker.WindowExplorer"
#define SETTING_NAME_SHOW_DESKTOP_WINDOWS (PLUGIN_NAME L".ShowDesktopWindows")
#define SETTING_NAME_WINDOW_TREE_LIST_COLUMNS (PLUGIN_NAME L".WindowTreeListColumns")
#define SETTING_NAME_WINDOWS_WINDOW_POSITION (PLUGIN_NAME L".WindowsWindowPosition")
#define SETTING_NAME_WINDOWS_WINDOW_SIZE (PLUGIN_NAME L".WindowsWindowSize")
#define SETTING_NAME_WINDOWS_PROPERTY_COLUMNS (PLUGIN_NAME L".WindowsPropertyColumns")
#define SETTING_NAME_WINDOWS_PROPERTY_POSITION (PLUGIN_NAME L".WindowsPropertyPosition")
#define SETTING_NAME_WINDOWS_PROPERTY_SIZE (PLUGIN_NAME L".WindowsPropertySize")
#define SETTING_NAME_WINDOWS_PROPLIST_COLUMNS (PLUGIN_NAME L".WindowsPropListColumns")
#define SETTING_NAME_WINDOWS_PROPSTORAGE_COLUMNS (PLUGIN_NAME L".WindowsPropStorageColumns")
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

VOID WeShowWindowsPropPage(
    _In_ PPH_PLUGIN_PROCESS_PROPCONTEXT Context,
    _In_ PWE_WINDOW_SELECTOR Selector
    );

// wndprp

VOID WeShowWindowProperties(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle
    );

// utils

#define WE_PhMainWndHandle (*(HWND *)WeGetProcedureAddress("PhMainWndHandle"))
#define WE_WindowsVersion (*(ULONG *)WeGetProcedureAddress("WindowsVersion"))
#define WE_PhInstanceHandle (*(HINSTANCE *)WeGetProcedureAddress("PhInstanceHandle"))

PVOID WeGetProcedureAddress(
    _In_ PSTR Name
    );

VOID WeInvertWindowBorder(
    _In_ HWND hWnd
    );

#endif
