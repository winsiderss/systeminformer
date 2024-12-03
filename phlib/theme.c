/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex         2022-2023
 *     Dart Vanya   2024
 *
 */

#include <ph.h>
#include <guisup.h>
#include <emenu.h>
#include <treenew.h>
#include <mapldr.h>
#include <apiimport.h>

#include <dwmapi.h>
#include <vsstyle.h>
#include <vssym32.h>

typedef struct _PHP_THEME_WINDOW_TAB_CONTEXT
{
    WNDPROC DefaultWindowProc;
    BOOLEAN MouseActive;
    POINT CursorPos;
} PHP_THEME_WINDOW_TAB_CONTEXT, *PPHP_THEME_WINDOW_TAB_CONTEXT;

typedef struct _PHP_THEME_WINDOW_STATUSBAR_CONTEXT
{
    WNDPROC DefaultWindowProc;

    struct
    {
       BOOLEAN Flags;
       union
       {
            BOOLEAN NonMouseActive : 1;
            BOOLEAN MouseActive : 1;
            BOOLEAN HotTrack : 1;
            BOOLEAN Hot : 1;
            BOOLEAN Spare : 4;
       };
    };

    HTHEME ThemeHandle;
    POINT CursorPos;

    HDC BufferedDc;
    HBITMAP BufferedOldBitmap;
    HBITMAP BufferedBitmap;
    RECT BufferedContextRect;
} PHP_THEME_WINDOW_STATUSBAR_CONTEXT, *PPHP_THEME_WINDOW_STATUSBAR_CONTEXT;

typedef struct _PHP_THEME_WINDOW_COMBO_CONTEXT
{
    WNDPROC DefaultWindowProc;
    HTHEME ThemeHandle;
    POINT CursorPos;
} PHP_THEME_WINDOW_COMBO_CONTEXT, *PPHP_THEME_WINDOW_COMBO_CONTEXT;

BOOLEAN CALLBACK PhpThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    );

LRESULT CALLBACK PhpThemeWindowSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );
LRESULT CALLBACK PhpThemeWindowGroupBoxSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );
LRESULT CALLBACK PhpThemeWindowTabControlWndSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhpThemeWindowListBoxControlSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhpThemeWindowComboBoxControlSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhpThemeWindowACLUISubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhThemeUpdateWindowProcedure(
    _In_ HWND WindowHandle,
    _In_ WNDPROC SubclassProcedure,
    _In_ BOOLEAN EnableThemeSupport
    );

// Win10-RS5 (uxtheme.dll ordinal 138)
BOOL (WINAPI *ShouldSystemUseDarkMode_I)(
    VOID
    ) = NULL;

// Win10-RS5 (uxtheme.dll ordinal 139)
BOOL (WINAPI *IsDarkModeAllowedForApp_I)(
    _In_ HWND WindowHandle
    ) = NULL;

//HRESULT (WINAPI* DwmGetColorizationColor_I)(
//    _Out_ PULONG Colorization,
//    _Out_ PBOOL OpaqueBlend
//    );

#ifdef DEBUG
#define DEBUG_BEGINPAINT_RECT(WindowHandle, RcPaint) \
{\
    RECT rect;\
    GetClientRect((WindowHandle), &rect);\
    assert(EqualRect(&rect, &(RcPaint)));\
}
#else
#define DEBUG_BEGINPAINT_RECT(RcPaint)
#endif

BOOLEAN PhEnableThemeSupport = FALSE;
BOOLEAN PhEnableThemeAcrylicSupport = FALSE;
BOOLEAN PhEnableThemeAcrylicWindowSupport = FALSE;
BOOLEAN PhEnableThemeNativeButtons = FALSE;
BOOLEAN PhEnableThemeTabBorders = FALSE;
BOOLEAN PhEnableThemeAnimation = FALSE;
BOOLEAN PhEnableStreamerMode = FALSE;
BOOLEAN PhEnableThemeListviewBorder = FALSE;
HBRUSH PhThemeWindowBackgroundBrush = NULL;
COLORREF PhThemeWindowForegroundColor = RGB(28, 28, 28);
COLORREF PhThemeWindowBackgroundColor = RGB(43, 43, 43);
COLORREF PhThemeWindowBackground2Color = RGB(65, 65, 65);
COLORREF PhThemeWindowHighlightColor = RGB(128, 128, 128);
COLORREF PhThemeWindowHighlight2Color = RGB(143, 143, 143);
COLORREF PhThemeWindowTextColor = RGB(255, 255, 255);

// This wrapper designed for general use from any places including plugins.
// No need te explicitly pass PhEnableThemeSupport or PhIsThemeSupportEnabled()
VOID PhInitializeWindowTheme(
    _In_ HWND WindowHandle
    )
{
    if (PhEnableThemeSupport)
    {
        PhInitializeWindowThemeEx(WindowHandle, TRUE);
    }
}

VOID PhInitializeWindowThemeEx(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    WCHAR windowClassName[MAX_PATH];

    GETCLASSNAME_OR_NULL(WindowHandle, windowClassName);

    if (PhEqualStringZ(windowClassName, PH_TREENEW_CLASSNAME, FALSE)) // HACK for dynamically generated plugin tabs
    {
        PhInitializeTreeNewTheme(WindowHandle, EnableThemeSupport);
    }
    else
    {
        PhInitializeThemeWindowFrameEx(WindowHandle, EnableThemeSupport);

        PhThemeUpdateWindowProcedure(WindowHandle, PhpThemeWindowSubclassProc, EnableThemeSupport);

        PhInitializeWindowThemeMenu(WindowHandle, EnableThemeSupport);
    }

    PhEnumChildWindows(
        WindowHandle,
        0x1000,
        PhpThemeWindowEnumChildWindows,
        (PVOID)EnableThemeSupport
        );

    //InvalidateRect(WindowHandle, NULL, FALSE);          // HACK
    RedrawWindow(WindowHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN /*| RDW_ERASENOW | RDW_UPDATENOW*/);
    SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);   // Redraws buttons background in dialogs
}

BOOLEAN PhGetAppsUseLightTheme(
    VOID
    )
{
    static PH_STRINGREF keyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");
    HANDLE keyHandle;
    BOOLEAN appsUseLightTheme = TRUE;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &keyPath,
        0
        )))
    {
        appsUseLightTheme = !PhQueryRegistryUlongZ(keyHandle, L"AppsUseLightTheme");
        NtClose(keyHandle);
    }

    return appsUseLightTheme;
}

VOID PhReInitializeTheme(
    BOOLEAN EnableThemeSupport
    )
{
    HWND currentWindow = NULL;

    while (currentWindow = FindWindowEx(NULL, currentWindow, NULL, NULL))
    {
        ULONG processID = 0;

        GetWindowThreadProcessId(currentWindow, &processID);

        WCHAR windowClassName[MAX_PATH];

        if (UlongToHandle(processID) == NtCurrentProcessId())
        {
            GETCLASSNAME_OR_NULL(currentWindow, windowClassName);

            if (PhEqualStringZ(windowClassName, TOOLTIPS_CLASS, FALSE))
            {
                PhWindowThemeSetDarkMode(currentWindow, EnableThemeSupport);    // handle tooltips
            }
            // Update theme on all visible top-level System Informer windows.
            else if ((PhGetWindowStyle(currentWindow) & WS_VISIBLE) == WS_VISIBLE)
            {
                dprintf("PhReInitializeTheme: %S\r\n", windowClassName);

                PhInitializeWindowThemeEx(currentWindow, EnableThemeSupport);
            }
        }
    }
}

VOID PhReInitializeStreamerMode(
    BOOLEAN Enable
    )
{
    HWND currentWindow = NULL;

    while (currentWindow = FindWindowEx(NULL, currentWindow, NULL, NULL))
    {
        ULONG processID = 0;

        GetWindowThreadProcessId(currentWindow, &processID);

        if (UlongToHandle(processID) == NtCurrentProcessId())
        {
            if ((PhGetWindowStyle(currentWindow) & WS_VISIBLE) == WS_VISIBLE)
            {
                SetWindowDisplayAffinity(currentWindow, Enable ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
            }
        }
    }
}

#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
//#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
//#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
//#endif
//#ifndef DWMWA_CAPTION_COLOR
//#define DWMWA_CAPTION_COLOR 35
//#endif
//#ifndef DWMWA_SYSTEMBACKDROP_TYPE
//#define DWMWA_SYSTEMBACKDROP_TYPE 38
//#endif

HRESULT PhSetWindowThemeAttribute(
    _In_ HWND WindowHandle,
    _In_ ULONG AttributeId,
    _In_reads_bytes_(AttributeLength) PVOID Attribute,
    _In_ ULONG AttributeLength
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI* DwmSetWindowAttribute_I)(
        _In_ HWND WindowHandle,
        _In_ ULONG AttributeId,
        _In_reads_bytes_(AttributeLength) PVOID Attribute,
        _In_ ULONG AttributeLength
        );

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"dwmapi.dll"))
        {
            DwmSetWindowAttribute_I = PhGetDllBaseProcedureAddress(baseAddress, "DwmSetWindowAttribute", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!DwmSetWindowAttribute_I)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return DwmSetWindowAttribute_I(WindowHandle, AttributeId, Attribute, AttributeLength);
}

VOID PhInitializeThemeWindowFrame(
    _In_ HWND WindowHandle
    )
{
    if (PhEnableThemeSupport)
    {
        PhInitializeThemeWindowFrameEx(WindowHandle, TRUE);
    }
}

VOID PhInitializeThemeWindowFrameEx(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    if (WindowsVersion >= WINDOWS_10_RS5)
    {
        if (FAILED(PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &(BOOL){ EnableThemeSupport }, sizeof(BOOL))))
        {
            PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &(BOOL){ EnableThemeSupport }, sizeof(BOOL));
        }

        if (WindowsVersion >= WINDOWS_11)
        {
            PhSetWindowThemeAttribute(WindowHandle, DWMWA_CAPTION_COLOR, &(COLORREF){ EnableThemeSupport ? PhThemeWindowBackgroundColor : DWMWA_COLOR_DEFAULT },
                sizeof(COLORREF));

            PhSetWindowAcrylicCompositionColor(WindowHandle, MakeABGRFromCOLORREF(0, RGB(10, 10, 10)), PhEnableThemeAcrylicWindowSupport);
        }

        if (WindowsVersion >= WINDOWS_11_22H2)
        {
            PhSetWindowThemeAttribute(WindowHandle, DWMWA_SYSTEMBACKDROP_TYPE, &(ULONG){ EnableThemeSupport ? DWMSBT_NONE : DWMSBT_AUTO }, sizeof(ULONG));
        }
    }
}

VOID PhWindowThemeSetDarkMode(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableDarkMode
    )
{
    if (EnableDarkMode)
    {
        PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
        //PhSetControlTheme(WindowHandle, L"DarkMode_ItemsView");

        //if (WindowsVersion >= WINDOWS_11)
        //{
        //    if (FAILED(PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &(BOOL){ TRUE }, sizeof(BOOL))))
        //    {
        //        PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &(BOOL){ TRUE }, sizeof(BOOL));
        //    }
        //}
    }
    else
    {
        PhSetControlTheme(WindowHandle, L"Explorer");
        //PhSetControlTheme(WindowHandle, L"ItemsView");

        //if (WindowsVersion >= WINDOWS_11)
        //{
        //    if (FAILED(PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &(BOOL){ FALSE }, sizeof(BOOL))))
        //    {
        //        PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &(BOOL){ FALSE }, sizeof(BOOL));
        //    }
        //}
    }
}

VOID PhThemeUpdateWindowProcedure(
    _In_ HWND WindowHandle,
    _In_ WNDPROC SubclassProcedure,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    WNDPROC currentWindowProc;
    WNDPROC defaultWindowProc;

    defaultWindowProc = (WNDPROC)PhGetWindowContext(WindowHandle, LONG_MAX);

    if (EnableThemeSupport)
    {
        if ((currentWindowProc = PhGetWindowProcedure(WindowHandle)) != SubclassProcedure)
        {
            PhSetWindowContext(WindowHandle, LONG_MAX, currentWindowProc);
            PhSetWindowProcedure(WindowHandle, SubclassProcedure);
        }
    }
    else
    {
        if (defaultWindowProc)
        {
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
            PhSetWindowProcedure(WindowHandle, defaultWindowProc);
        }
    }
}

HBRUSH PhWindowThemeControlColor(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ HWND ChildWindowHandle,
    _In_ LONG Type
    )
{
    SetBkMode(Hdc, TRANSPARENT);

    switch (Type)
    {
    case CTLCOLOR_EDIT:
        {
            if (PhEnableThemeSupport)
            {
                SetTextColor(Hdc, PhThemeWindowTextColor);
                SetDCBrushColor(Hdc, PhMakeColorBrighter(PhThemeWindowBackgroundColor, 17));   // RGB(60, 60, 60)
                return PhGetStockBrush(DC_BRUSH);
            }
            else
            {
                SetTextColor(Hdc, GetSysColor(COLOR_WINDOWTEXT));
                return GetSysColorBrush(COLOR_WINDOW);
            }
        }
        break;
    case CTLCOLOR_SCROLLBAR:
        {
            if (PhEnableThemeSupport)
            {
                SetDCBrushColor(Hdc, RGB(23, 23, 23));
                return PhGetStockBrush(DC_BRUSH);
            }
            else
            {
                return GetSysColorBrush(COLOR_SCROLLBAR);
            }
        }
        break;
    case CTLCOLOR_MSGBOX:
    case CTLCOLOR_LISTBOX:
    case CTLCOLOR_BTN:
    case CTLCOLOR_DLG:
    case CTLCOLOR_STATIC:
        {
            if (PhEnableThemeSupport)
            {
                SetTextColor(Hdc, PhThemeWindowTextColor);
                return PhThemeWindowBackgroundBrush;
            }
            else
            {
                SetTextColor(Hdc, GetSysColor(COLOR_WINDOWTEXT));
                return GetSysColorBrush(COLOR_WINDOW);
            }
        }
        break;
    }

    return (HBRUSH)DefWindowProc(WindowHandle, Type, (WPARAM)Hdc, (LPARAM)ChildWindowHandle);
}

VOID PhWindowThemeMainMenuBorder(
    _In_ HWND WindowHandle
    )
{
    if (GetMenu(WindowHandle))
    {
        RECT clientRect;
        RECT windowRect;
        HDC hdc;

        GetClientRect(WindowHandle, &clientRect);
        GetWindowRect(WindowHandle, &windowRect);

        MapWindowPoints(WindowHandle, NULL, (PPOINT)&clientRect, 2);
        PhOffsetRect(&clientRect, -windowRect.left, -windowRect.top);

        // the rcBar is offset by the window rect (thanks to adzm) (dmex)
        RECT rcAnnoyingLine = clientRect;
        rcAnnoyingLine.bottom = rcAnnoyingLine.top;
        rcAnnoyingLine.top--;

        if (hdc = GetWindowDC(WindowHandle))
        {
            if (PhEnableThemeSupport)
            {
                FillRect(hdc, &rcAnnoyingLine, PhThemeWindowBackgroundBrush);
            }
            else
            {
                FillRect(hdc, &rcAnnoyingLine, (HBRUSH)(COLOR_WINDOW + 1));
            }

            ReleaseDC(WindowHandle, hdc);
        }
    }
}

VOID PhInitializeThemeWindowTabControl(
    _In_ HWND TabControlWindow,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    PPHP_THEME_WINDOW_TAB_CONTEXT context;
    WNDPROC currentWindowProc;

    context = (PPHP_THEME_WINDOW_TAB_CONTEXT)PhGetWindowContext(TabControlWindow, LONG_MAX);

    if (EnableThemeSupport)
    {
        if (!context && (currentWindowProc = PhGetWindowProcedure(TabControlWindow)) != PhpThemeWindowTabControlWndSubclassProc)
        {
            context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_TAB_CONTEXT));
            context->DefaultWindowProc = currentWindowProc;
            context->CursorPos.x = LONG_MIN;
            context->CursorPos.y = LONG_MIN;

            PhSetWindowContext(TabControlWindow, LONG_MAX, context);
            PhSetWindowProcedure(TabControlWindow, PhpThemeWindowTabControlWndSubclassProc);
        }
    }
    else
    {
        if (context && context->DefaultWindowProc)
        {
            PhRemoveWindowContext(TabControlWindow, LONG_MAX);
            PhSetWindowProcedure(TabControlWindow, context->DefaultWindowProc);
            PhFree(context);
        }
    }
}

VOID PhInitializeThemeWindowGroupBox(
    _In_ HWND GroupBoxHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    PhThemeUpdateWindowProcedure(GroupBoxHandle, PhpThemeWindowGroupBoxSubclassProc, EnableThemeSupport);
}

VOID PhInitializeWindowThemeMenu(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    HMENU menuHandle;

    if (menuHandle = GetMenu(WindowHandle))
    {
        MENUINFO menuInfo;
        MENUITEMINFO menuItemInfo;

        memset(&menuInfo, 0, sizeof(MENUINFO));
        menuInfo.cbSize = sizeof(MENUINFO);
        menuInfo.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
        menuInfo.hbrBack = EnableThemeSupport ? PhThemeWindowBackgroundBrush : NULL;

        SetMenuInfo(menuHandle, &menuInfo);

        memset(&menuItemInfo, 0, sizeof(MENUITEMINFO));
        menuItemInfo.cbSize = sizeof(MENUITEMINFO);
        menuItemInfo.fMask = MIIM_FTYPE;

        for (INT i = 0; i < GetMenuItemCount(menuHandle); i++)
        {
            GetMenuItemInfo(menuHandle, i, TRUE, &menuItemInfo);
            if (EnableThemeSupport)
                SetFlag(menuItemInfo.fType, MFT_OWNERDRAW);
            else
                ClearFlag(menuItemInfo.fType, MFT_OWNERDRAW);
            SetMenuItemInfo(menuHandle, i, TRUE, &menuItemInfo);
        }

        DrawMenuBar(WindowHandle);
    }
}

VOID PhInitializeWindowThemeListboxControl(
    _In_ HWND ListBoxControl,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    PPHP_THEME_WINDOW_STATUSBAR_CONTEXT context;
    WNDPROC currentWindowProc;

    context = (PPHP_THEME_WINDOW_STATUSBAR_CONTEXT)PhGetWindowContext(ListBoxControl, LONG_MAX);

    if (EnableThemeSupport)
    {
        if (!context && (currentWindowProc = PhGetWindowProcedure(ListBoxControl)) != PhpThemeWindowListBoxControlSubclassProc)
        {
            context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_STATUSBAR_CONTEXT));
            context->DefaultWindowProc = currentWindowProc;
            context->CursorPos.x = LONG_MIN;
            context->CursorPos.y = LONG_MIN;

            PhSetWindowContext(ListBoxControl, LONG_MAX, context);
            PhSetWindowProcedure(ListBoxControl, PhpThemeWindowListBoxControlSubclassProc);
        }
    }
    else
    {
        if (context && context->DefaultWindowProc)
        {
            PhRemoveWindowContext(ListBoxControl, LONG_MAX);
            PhSetWindowProcedure(ListBoxControl, context->DefaultWindowProc);
            PhFree(context);
        }
    }

    SetWindowPos(ListBoxControl, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

VOID PhInitializeWindowThemeComboboxControl(
    _In_ HWND ComboBoxControl,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    PPHP_THEME_WINDOW_COMBO_CONTEXT context;
    WNDPROC currentWindowProc;

    context = (PPHP_THEME_WINDOW_COMBO_CONTEXT)PhGetWindowContext(ComboBoxControl, LONG_MAX);

    if (EnableThemeSupport)
    {
        if (!context && (currentWindowProc = PhGetWindowProcedure(ComboBoxControl)) != PhpThemeWindowComboBoxControlSubclassProc)
        {
            context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_COMBO_CONTEXT));
            context->DefaultWindowProc = currentWindowProc;
            context->ThemeHandle = PhOpenThemeData(ComboBoxControl, VSCLASS_COMBOBOX, PhGetWindowDpi(ComboBoxControl));
            context->CursorPos.x = LONG_MIN;
            context->CursorPos.y = LONG_MIN;

            PhSetWindowContext(ComboBoxControl, LONG_MAX, context);
            PhSetWindowProcedure(ComboBoxControl, PhpThemeWindowComboBoxControlSubclassProc);
        }
    }
    else
    {
        if (context && context->DefaultWindowProc)
        {
            PhRemoveWindowContext(ComboBoxControl, LONG_MAX);
            PhSetWindowProcedure(ComboBoxControl, context->DefaultWindowProc);
            if (context->ThemeHandle)
                PhCloseThemeData(context->ThemeHandle);
            PhFree(context);
        }
    }
}

VOID PhInitializeWindowThemeACLUI(
    _In_ HWND ACLUIControl,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    PhThemeUpdateWindowProcedure(ACLUIControl, PhpThemeWindowACLUISubclassProc, EnableThemeSupport);
}

BOOLEAN CALLBACK PhpThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    WCHAR windowClassName[MAX_PATH];
    BOOLEAN enableThemeSupport = !!Context;

    PhEnumChildWindows(
        WindowHandle,
        0x1000,
        PhpThemeWindowEnumChildWindows,
        Context
        );

    if (enableThemeSupport && PhGetWindowContext(WindowHandle, LONG_MAX)) // HACK
        return TRUE;

    GETCLASSNAME_OR_NULL(WindowHandle, windowClassName);

    //dprintf("PhpThemeWindowEnumChildWindows: %S\r\n", windowClassName);

    if (PhEqualStringZ(windowClassName, L"#32770", TRUE))
    {
        PhInitializeWindowThemeEx(WindowHandle, enableThemeSupport);
    }
    else if (PhEqualStringZ(windowClassName, WC_BUTTON, FALSE))
    {
        if ((PhGetWindowStyle(WindowHandle) & BS_GROUPBOX) == BS_GROUPBOX)
        {
            PhInitializeThemeWindowGroupBox(WindowHandle, enableThemeSupport);
        }
        else    // apply theme for CheckBox, Radio (Dart Vanya)
        {
            PhWindowThemeSetDarkMode(WindowHandle, enableThemeSupport);
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_TABCONTROL, FALSE))
    {
        PhInitializeThemeWindowTabControl(WindowHandle, enableThemeSupport);
    }
    else if (PhEqualStringZ(windowClassName, WC_SCROLLBAR, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhWindowThemeSetDarkMode(WindowHandle, enableThemeSupport);
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_LISTVIEW, FALSE))
    {
        PhInitializeListViewTheme(WindowHandle, enableThemeSupport);
    }
    else if (PhEqualStringZ(windowClassName, WC_TREEVIEW, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND tooltipWindow = TreeView_GetToolTips(WindowHandle);

            PhWindowThemeSetDarkMode(WindowHandle, enableThemeSupport);
			if (tooltipWindow)
				PhWindowThemeSetDarkMode(tooltipWindow, enableThemeSupport);
        }

        TreeView_SetBkColor(WindowHandle, enableThemeSupport ? PhThemeWindowBackgroundColor : GetSysColor(COLOR_WINDOW));// RGB(30, 30, 30));
        //TreeView_SetTextBkColor(WindowHandle, enableThemeSupport ? PhThemeWindowBackgroundColor : GetSysColor(COLOR_WINDOW));
        TreeView_SetTextColor(WindowHandle, enableThemeSupport ? PhThemeWindowTextColor : GetSysColor(COLOR_WINDOWTEXT));
    }
    else if (PhEqualStringZ(windowClassName, L"RICHEDIT50W", FALSE))
    {
        if (enableThemeSupport && PhEnableThemeListviewBorder)
            PhSetWindowStyle(WindowHandle, WS_BORDER, WS_BORDER);
        else if (enableThemeSupport)
            PhSetWindowStyle(WindowHandle, WS_BORDER, 0);

        SetWindowPos(WindowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

#define EM_SETBKGNDCOLOR (WM_USER + 67)
        SendMessage(WindowHandle, EM_SETBKGNDCOLOR, 0, enableThemeSupport ? PhMakeColorBrighter(PhThemeWindowForegroundColor, 2) : GetSysColor(COLOR_WINDOW));  // RGB(30, 30, 30)
        PhWindowThemeSetDarkMode(WindowHandle, enableThemeSupport);
    }
    else if (PhEqualStringZ(windowClassName, PH_TREENEW_CLASSNAME, FALSE))
    {
        PhInitializeTreeNewTheme(WindowHandle, enableThemeSupport);
    }
    else if (
        PhEqualStringZ(windowClassName, WC_LISTBOX, FALSE) ||
        PhEqualStringZ(windowClassName, L"ComboLBox", FALSE)
        )
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhWindowThemeSetDarkMode(WindowHandle, enableThemeSupport);
        }

        PhInitializeWindowThemeListboxControl(WindowHandle, enableThemeSupport);
    }
    else if (PhEqualStringZ(windowClassName, WC_COMBOBOX, FALSE))
    {
        COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };

        if (SendMessage(WindowHandle, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info))
        {
            //if (info.hwndItem)
            //{
            //    SendMessage(info.hwndItem, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));
            //}

            if (info.hwndList)
            {
                PhWindowThemeSetDarkMode(info.hwndList, enableThemeSupport);
            }
        }

        //if ((PhGetWindowStyle(WindowHandle) & CBS_DROPDOWNLIST) != CBS_DROPDOWNLIST)
        {
            PhInitializeWindowThemeComboboxControl(WindowHandle, enableThemeSupport);
        }
    }
    else if (PhEqualStringZ(windowClassName, L"CHECKLIST_ACLUI", FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhWindowThemeSetDarkMode(WindowHandle, enableThemeSupport);
        }

        PhInitializeWindowThemeACLUI(WindowHandle, enableThemeSupport);
    }
    else if (PhEqualStringZ(windowClassName, WC_EDIT, FALSE))
    {
        // Fix scrollbar on multiline edit (Dart Vanya)
        if ((PhGetWindowStyle(WindowHandle) & ES_MULTILINE) == ES_MULTILINE)
        {
            PhWindowThemeSetDarkMode(WindowHandle, enableThemeSupport);
        }
        else if (PhGetWindowContext(WindowHandle, SHRT_MAX))    // HACK
        {
            SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0); // searchbox.c
        }

        PhAllowDarkModeForWindow(WindowHandle, enableThemeSupport); // fix for unsupported system dialogs (ex. Advanced Security)
    }
    else if (PhEqualStringZ(windowClassName, WC_LINK, FALSE))   // SysLink theme support (Dart Vanya)
    {
        PhAllowDarkModeForWindow(WindowHandle, enableThemeSupport);
    }
    else if (PhEqualStringZ(windowClassName, L"DirectUIHWND", FALSE))  // TaskDialog
    {
        PhWindowThemeSetDarkMode(WindowHandle, enableThemeSupport);
    }

    return TRUE;
}

VOID PhInitializeTreeNewTheme(
    _In_ HWND TreeNewHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    if (WindowsVersion >= WINDOWS_10_RS5)
    {
        HWND headerHandle = TreeNew_GetHeader(TreeNewHandle);
        HWND fixedHeaderHandle = TreeNew_GetFixedHeader(TreeNewHandle);
        HWND tooltipWindow = TreeNew_GetTooltips(TreeNewHandle);

        PhAllowDarkModeForWindow(TreeNewHandle, EnableThemeSupport);
        PhWindowThemeSetDarkMode(TreeNewHandle, EnableThemeSupport);
        if (headerHandle)
            PhSetControlTheme(headerHandle, EnableThemeSupport ? L"DarkMode_ItemsView" : L"Explorer");
        if (fixedHeaderHandle)
            PhSetControlTheme(fixedHeaderHandle, EnableThemeSupport ? L"DarkMode_ItemsView" : L"Explorer");
        if (tooltipWindow)
            PhWindowThemeSetDarkMode(tooltipWindow, EnableThemeSupport);
    }

    if (EnableThemeSupport && PhEnableThemeListviewBorder)
        PhSetWindowExStyle(TreeNewHandle, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
    else if (EnableThemeSupport)
        PhSetWindowExStyle(TreeNewHandle, WS_EX_CLIENTEDGE, 0);

    SetWindowPos(TreeNewHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

    TreeNew_ThemeSupport(TreeNewHandle, EnableThemeSupport);
}

VOID PhInitializeListViewTheme(
    _In_ HWND ListViewHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    if (WindowsVersion >= WINDOWS_10_RS5)
    {
        HWND tooltipWindow = ListView_GetToolTips(ListViewHandle);
        HWND headerHandle = ListView_GetHeader(ListViewHandle);

        PhAllowDarkModeForWindow(ListViewHandle, EnableThemeSupport);
        PhSetControlTheme(ListViewHandle, EnableThemeSupport ? L"DarkMode_ItemsView" : L"Explorer");
        if (headerHandle)
            PhSetControlTheme(headerHandle, EnableThemeSupport ? L"DarkMode_ItemsView" : L"Explorer");
        if (tooltipWindow)
            PhWindowThemeSetDarkMode(tooltipWindow, EnableThemeSupport);
    }

    if (EnableThemeSupport && PhEnableThemeListviewBorder)
    {
        PhSetWindowStyle(ListViewHandle, WS_BORDER, WS_BORDER);
        PhSetWindowExStyle(ListViewHandle, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
    }
    else if (EnableThemeSupport)
    {
        PhSetWindowStyle(ListViewHandle, WS_BORDER, 0);
        PhSetWindowExStyle(ListViewHandle, WS_EX_CLIENTEDGE, 0);
    }

    SetWindowPos(ListViewHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

    ListView_SetBkColor(ListViewHandle, EnableThemeSupport ? PhThemeWindowBackgroundColor : GetSysColor(COLOR_WINDOW)); // RGB(30, 30, 30)
    ListView_SetTextBkColor(ListViewHandle, EnableThemeSupport ? PhThemeWindowBackgroundColor : GetSysColor(COLOR_WINDOW)); // RGB(30, 30, 30)
    ListView_SetTextColor(ListViewHandle, EnableThemeSupport ? PhThemeWindowTextColor : GetSysColor(COLOR_WINDOWTEXT));
}

BOOLEAN PhThemeWindowDrawItem(
    _In_ HWND WindowHandle,
    _In_ PDRAWITEMSTRUCT DrawInfo
    )
{
    BOOLEAN isGrayed = (DrawInfo->itemState & CDIS_GRAYED) == CDIS_GRAYED;
    BOOLEAN isChecked = (DrawInfo->itemState & CDIS_CHECKED) == CDIS_CHECKED;
    BOOLEAN isDisabled = (DrawInfo->itemState & CDIS_DISABLED) == CDIS_DISABLED;
    BOOLEAN isSelected = (DrawInfo->itemState & CDIS_SELECTED) == CDIS_SELECTED;
    //BOOLEAN isHighlighted = (DrawInfo->itemState & CDIS_HOT) == CDIS_HOT;
    BOOLEAN isFocused = (DrawInfo->itemState & CDIS_FOCUS) == CDIS_FOCUS;
    //BOOLEAN isGrayed = (DrawInfo->itemState & ODS_GRAYED) == ODS_GRAYED;
    //BOOLEAN isChecked = (DrawInfo->itemState & ODS_CHECKED) == ODS_CHECKED;
    //BOOLEAN isDisabled = (DrawInfo->itemState & ODS_DISABLED) == ODS_DISABLED;
    //BOOLEAN isSelected = (DrawInfo->itemState & ODS_SELECTED) == ODS_SELECTED;
    BOOLEAN isHighlighted = (DrawInfo->itemState & ODS_HOTLIGHT) == ODS_HOTLIGHT;

    SetBkMode(DrawInfo->hDC, TRANSPARENT);

    switch (DrawInfo->CtlType)
    {
    case ODT_MENU:
        {
            PPH_EMENU_ITEM menuItemInfo = (PPH_EMENU_ITEM)DrawInfo->itemData;
            RECT rect = DrawInfo->rcItem;
            LONG dpiValue = PhGetWindowDpi(WindowHandle);
            ULONG drawTextFlags = DT_SINGLELINE | DT_NOCLIP;
            //HFONT fontHandle;
            //HFONT oldFont = NULL;

            if (DrawInfo->itemState & ODS_NOACCEL)
            {
                drawTextFlags |= DT_HIDEPREFIX;
            }

            //if (fontHandle = PhCreateMessageFont(dpiValue))
            //{
            //    oldFont = SelectFont(DrawInfo->hDC, fontHandle);
            //}
            //
            //FillRect(
            //    DrawInfo->hDC,
            //    &DrawInfo->rcItem,
            //    CreateSolidBrush(RGB(0, 0, 0))
            //    );
            //SetTextColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));

            if (DrawInfo->itemState & ODS_HOTLIGHT)
            {
                SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->hDC, PhThemeWindowHighlightColor);
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhGetStockBrush(DC_BRUSH));
            }
            else if (isDisabled)
            {
                SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_GRAYTEXT));
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhThemeWindowBackgroundBrush);
            }
            else if (isSelected)
            {
                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);
                //    SetDCBrushColor(DrawInfo->hDC, PhThemeWindowBackgroundColor);
                //    break;

                SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->hDC, PhThemeWindowHighlightColor);
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhGetStockBrush(DC_BRUSH));
            }
            else
            {
                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_WINDOWTEXT));
                //    SetDCBrushColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));
                //    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhGetStockBrush(DC_BRUSH));
                //    break;

                SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhThemeWindowBackgroundBrush);
            }

            if (isChecked)
            {
                static PH_STRINGREF menuCheckText = PH_STRINGREF_INIT(L"\u2713");
                COLORREF oldTextColor;

                //HFONT marlettFontHandle = CreateFont(
                //    0, 0, 0, 0,
                //    FW_DONTCARE,
                //    FALSE,
                //    FALSE,
                //    FALSE,
                //    DEFAULT_CHARSET,
                //    OUT_OUTLINE_PRECIS,
                //    CLIP_DEFAULT_PRECIS,
                //    CLEARTYPE_QUALITY,
                //    VARIABLE_PITCH,
                //    L"Arial Unicode MS"
                //    );

                oldTextColor = SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);

                DrawInfo->rcItem.left += PhGetDpi(8, dpiValue);
                DrawInfo->rcItem.top += PhGetDpi(3, dpiValue);
                DrawText(
                    DrawInfo->hDC,
                    menuCheckText.Buffer,
                    (UINT)menuCheckText.Length / sizeof(WCHAR),
                    &DrawInfo->rcItem,
                    DT_VCENTER | DT_NOCLIP
                    );
                DrawInfo->rcItem.left -= PhGetDpi(8, dpiValue);
                DrawInfo->rcItem.top -= PhGetDpi(3, dpiValue);

                SetTextColor(DrawInfo->hDC, oldTextColor);
            }

            if (menuItemInfo->Flags & PH_EMENU_SEPARATOR)
            {
                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    SetDCBrushColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));
                //    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhGetStockBrush(DC_BRUSH));
                //    break;
                //case 1: // Old colors
                //SetDCBrushColor(DrawInfo->hDC, PhThemeWindowBackgroundColor); // PhThemeWindowForegroundColor
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhThemeWindowBackgroundBrush);

                //DrawInfo->rcItem.top += PhGetDpi(1, dpiValue);
                //DrawInfo->rcItem.bottom -= PhGetDpi(2, dpiValue);
                //DrawFocusRect(drawInfo->hDC, &drawInfo->rcItem);

                // +5 font margin, +1 extra padding
                //INT cxMenuCheck = GetSystemMetrics(SM_CXMENUCHECK) + (GetSystemMetrics(SM_CXEDGE) * 2) + 5 + 1;
                INT cyEdge = PhGetSystemMetrics(SM_CYEDGE, dpiValue);
                //
                //SetRect(
                //    &DrawInfo->rcItem,
                //    DrawInfo->rcItem.left + cxMenuCheck, // 25
                //    DrawInfo->rcItem.top + cyEdge,
                //    DrawInfo->rcItem.right,
                //    DrawInfo->rcItem.bottom - cyEdge
                //    );

                SetDCBrushColor(DrawInfo->hDC, RGB(0x5f, 0x5f, 0x5f));
                SelectBrush(DrawInfo->hDC, PhGetStockBrush(DC_BRUSH));
                PatBlt(DrawInfo->hDC, DrawInfo->rcItem.left, DrawInfo->rcItem.top + cyEdge, DrawInfo->rcItem.right - DrawInfo->rcItem.left, 1, PATCOPY);

                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    SetDCBrushColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));
                //    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhGetStockBrush(DC_BRUSH));
                //    break;
                //case 1: // Old colors
                //    SetDCBrushColor(DrawInfo->hDC, RGB(78, 78, 78));
                //    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhGetStockBrush(DC_BRUSH));
                //    break;
                //}

                //DrawEdge(drawInfo->hDC, &drawInfo->rcItem, BDR_RAISEDINNER, BF_TOP);
            }
            else
            {
                PH_STRINGREF part = { 0 };
                PH_STRINGREF firstPart = { 0 };
                PH_STRINGREF secondPart = { 0 };

                PhInitializeStringRefLongHint(&part, menuItemInfo->Text);
                PhSplitStringRefAtLastChar(&part, L'\b', &firstPart, &secondPart);

                //SetDCBrushColor(DrawInfo->hDC, PhThemeWindowForegroundColor);
                //FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhGetStockBrush(DC_BRUSH));

                if (menuItemInfo->Bitmap)
                {
                    HDC bufferDc;
                    BLENDFUNCTION blendFunction;

                    blendFunction.BlendOp = AC_SRC_OVER;
                    blendFunction.BlendFlags = 0;
                    blendFunction.SourceConstantAlpha = 255;
                    blendFunction.AlphaFormat = AC_SRC_ALPHA;

                    bufferDc = CreateCompatibleDC(DrawInfo->hDC);
                    SelectBitmap(bufferDc, menuItemInfo->Bitmap);

                    GdiAlphaBlend(
                        DrawInfo->hDC,
                        DrawInfo->rcItem.left + 4,
                        DrawInfo->rcItem.top + 4,
                        PhGetSystemMetrics(SM_CXSMICON, dpiValue),
                        PhGetSystemMetrics(SM_CYSMICON, dpiValue),
                        bufferDc,
                        0,
                        0,
                        PhGetSystemMetrics(SM_CXSMICON, dpiValue),
                        PhGetSystemMetrics(SM_CYSMICON, dpiValue),
                        blendFunction
                        );

                    DeleteDC(bufferDc);
                }

                DrawInfo->rcItem.left += PhGetDpi(25, dpiValue);
                DrawInfo->rcItem.right -= PhGetDpi(25, dpiValue);

                if ((menuItemInfo->Flags & PH_EMENU_MAINMENU) == PH_EMENU_MAINMENU)
                {
                    if (firstPart.Length)
                    {
                        DrawText(
                            DrawInfo->hDC,
                            firstPart.Buffer,
                            (UINT)firstPart.Length / sizeof(WCHAR),
                            &DrawInfo->rcItem,
                            DT_LEFT | DT_SINGLELINE | DT_CENTER | DT_VCENTER | drawTextFlags
                            );
                    }
                }
                else
                {
                    if (firstPart.Length)
                    {
                        DrawText(
                            DrawInfo->hDC,
                            firstPart.Buffer,
                            (UINT)firstPart.Length / sizeof(WCHAR),
                            &DrawInfo->rcItem,
                            DT_LEFT | DT_VCENTER | drawTextFlags
                            );
                    }
                }

                if (secondPart.Length)
                {
                    DrawText(
                        DrawInfo->hDC,
                        secondPart.Buffer,
                        (UINT)secondPart.Length / sizeof(WCHAR),
                        &DrawInfo->rcItem,
                        DT_RIGHT | DT_VCENTER | drawTextFlags
                        );
                }
            }

            //if (oldFont)
            //{
            //    SelectFont(DrawInfo->hDC, oldFont);
            //}

            if (menuItemInfo->Items && menuItemInfo->Items->Count && (menuItemInfo->Flags & PH_EMENU_MAINMENU) != PH_EMENU_MAINMENU)
            {
                HTHEME themeHandle;

                if (themeHandle = PhOpenThemeData(DrawInfo->hwndItem, VSCLASS_MENU, dpiValue))
                {
                    //if (IsThemeBackgroundPartiallyTransparent(themeHandle, MENU_POPUPSUBMENU, isDisabled ? MSM_DISABLED : MSM_NORMAL))
                    //    DrawThemeParentBackground(DrawInfo->hwndItem, DrawInfo->hDC, NULL);

                    rect.left = rect.right - PhGetDpi(25, dpiValue);

                    PhDrawThemeBackground(
                        themeHandle,
                        DrawInfo->hDC,
                        MENU_POPUPSUBMENU,
                        isDisabled ? MSM_DISABLED : MSM_NORMAL,
                        &rect,
                        NULL
                        );

                    PhCloseThemeData(themeHandle);
                }
            }

            ExcludeClipRect(DrawInfo->hDC, rect.left, rect.top, rect.right, rect.bottom); // exclude last

            //if (fontHandle)
            //{
            //    DeleteFont(fontHandle);
            //}
        }
        return TRUE;
    case ODT_COMBOBOX:
        {
            SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);
            SetDCBrushColor(DrawInfo->hDC, isSelected ? PhMakeColorBrighter(PhThemeWindowBackground2Color, 15) : PhThemeWindowForegroundColor); // RGB(80, 80, 80)
            FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhGetStockBrush(DC_BRUSH));

            INT length = ComboBox_GetLBTextLen(DrawInfo->hwndItem, DrawInfo->itemID);

            if (length == CB_ERR)
                break;

            if (length < MAX_PATH)
            {
                WCHAR comboText[MAX_PATH] = L"";

                if (ComboBox_GetLBText(DrawInfo->hwndItem, DrawInfo->itemID, comboText) == CB_ERR)
                    break;

                RECT rect = DrawInfo->rcItem;
                rect.left += 2;

                DrawText(
                    DrawInfo->hDC,
                    comboText,
                    (UINT)PhCountStringZ(comboText),
                    &rect,
                    DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER
                    );
            }
        }
        return TRUE;
    }

    return FALSE;
}

BOOLEAN PhThemeWindowMeasureItem(
    _In_ HWND WindowHandle,
    _In_ PMEASUREITEMSTRUCT DrawInfo
    )
{
    if (DrawInfo->CtlType == ODT_MENU)
    {
        PPH_EMENU_ITEM menuItemInfo = (PPH_EMENU_ITEM)DrawInfo->itemData;
        LONG dpiValue = PhGetWindowDpi(WindowHandle);

        DrawInfo->itemWidth = PhGetDpi(100, dpiValue);
        DrawInfo->itemHeight = PhGetDpi(100, dpiValue);

        if ((menuItemInfo->Flags & PH_EMENU_SEPARATOR) == PH_EMENU_SEPARATOR)
        {
            DrawInfo->itemHeight = PhGetSystemMetrics(SM_CYMENU, dpiValue) >> 2;
        }
        else if (menuItemInfo->Text)
        {
            //HFONT fontHandle;
            HDC hdc;

            //fontHandle = PhCreateMessageFont(dpiValue);

            if (hdc = GetDC(WindowHandle))
            {
                PCWSTR text;
                SIZE_T textCount;
                SIZE textSize;
                //HFONT oldFont = NULL;
                INT cyborder = PhGetSystemMetrics(SM_CYBORDER, dpiValue);
                INT cymenu = PhGetSystemMetrics(SM_CYMENU, dpiValue);

                text = menuItemInfo->Text;
                textCount = PhCountStringZ(text);

                //if (fontHandle)
                //{
                //    oldFont = SelectFont(hdc, fontHandle);
                //}

                if ((menuItemInfo->Flags & PH_EMENU_MAINMENU) == PH_EMENU_MAINMENU)
                {
                    if (GetTextExtentPoint32(hdc, text, (ULONG)textCount, &textSize))
                    {
                        DrawInfo->itemWidth = textSize.cx + (cyborder * 2);
                        DrawInfo->itemHeight = cymenu + (cyborder * 2) + 1;
                    }
                }
                else
                {
                    if (GetTextExtentPoint32(hdc, text, (ULONG)textCount, &textSize))
                    {
                        DrawInfo->itemWidth = textSize.cx + (cyborder * 2) + PhGetDpi(90, dpiValue); // HACK
                        DrawInfo->itemHeight = cymenu + (cyborder * 2) + PhGetDpi(1, dpiValue);
                    }
                }

                //if (oldFont)
                //{
                //    SelectFont(hdc, oldFont);
                //}

                ReleaseDC(WindowHandle, hdc);
            }

            //if (fontHandle)
            //{
            //    DeleteFont(fontHandle);
            //}
        }

        return TRUE;
    }

    return FALSE;
}

// TODO: Use imagelist instead of loading images from uxtheme.
//HIMAGELIST CreateTreeViewCheckBoxes(HWND hwnd, int cx, int cy)
//{
//    const int frames = 6;
//
//    // Get a DC for our window.
//    HDC hdcScreen = GetDC(hwnd);
//
//    // Get a button theme for the window, if available.
//    HTHEME htheme = OpenThemeData(hwnd, L"button");
//
//    // If there is a theme, then ask it for the size
//    // of a checkbox and use that size.
//    if (htheme)
//    {
//        SIZE siz;
//        PhGetThemePartSize(htheme, hdcScreen, BP_CHECKBOX, CBS_UNCHECKEDNORMAL, NULL, TS_DRAW, &siz);
//        cx = siz.cx;
//        cy = siz.cy;
//    }
//
//    // Create a 32bpp bitmap that holds the desired number of frames.
//    BITMAPINFO bi = { sizeof(BITMAPINFOHEADER), cx * frames, cy, 1, 32 };
//    void* p;
//    HBITMAP hbmCheckboxes = CreateDIBSection(hdcScreen, &bi, DIB_RGB_COLORS, &p, NULL, 0);
//
//    // Create a compatible memory DC.
//    HDC hdcMem = CreateCompatibleDC(hdcScreen);
//
//    // Select our bitmap into it so we can draw to it.
//    HBITMAP hbmOld = SelectBitmap(hdcMem, hbmCheckboxes);
//
//    // Set up the rectangle into which we do our drawing.
//    RECT rc = { 0, 0, cx, cy };
//
//    // Frame 0 is not used. Draw nothing.
//    PhOffsetRect(&rc, cx, 0);
//
//    if (htheme)
//    {
//        // Frame 1: Unchecked.
//        PhDrawThemeBackground(htheme, hdcMem, BP_CHECKBOX, CBS_UNCHECKEDNORMAL, &rc, NULL);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 2: Checked.
//        PhDrawThemeBackground(htheme, hdcMem, BP_CHECKBOX, CBS_CHECKEDNORMAL, &rc, NULL);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 3: Indeterminate.
//        PhDrawThemeBackground(htheme, hdcMem, BP_CHECKBOX, CBS_MIXEDNORMAL, &rc, NULL);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 4: Disabled, unchecked.
//        PhDrawThemeBackground(htheme, hdcMem, BP_CHECKBOX, CBS_UNCHECKEDDISABLED, &rc, NULL);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 5: Disabled, checked.
//        PhDrawThemeBackground(htheme, hdcMem, BP_CHECKBOX, CBS_CHECKEDDISABLED, &rc, NULL);
//
//        // Done with the theme.
//        PhCloseThemeData(htheme);
//    }
//    else
//    {
//        // Flags common to all of our DrawFrameControl calls:
//        // Draw a flat checkbox.
//        UINT baseFlags = DFCS_FLAT | DFCS_BUTTONCHECK;
//
//        // Frame 1: Unchecked.
//        DrawFrameControl(hdcMem, &rc, DFC_BUTTON, baseFlags);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 2: Checked.
//        DrawFrameControl(hdcMem, &rc, DFC_BUTTON, baseFlags | DFCS_CHECKED);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 3: Indeterminate.
//        DrawFrameControl(hdcMem, &rc, DFC_BUTTON, baseFlags | DFCS_CHECKED | DFCS_BUTTON3STATE);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 4: Disabled, unchecked.
//        DrawFrameControl(hdcMem, &rc, DFC_BUTTON, baseFlags | DFCS_INACTIVE);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 5: Disabled, checked.
//        DrawFrameControl(hdcMem, &rc, DFC_BUTTON, baseFlags | DFCS_INACTIVE | DFCS_CHECKED);
//    }
//
//    // The bitmap is ready. Clean up.
//    SelectBitmap(hdcMem, hbmOld);
//    DeleteDC(hdcMem);
//    ReleaseDC(hwnd, hdcScreen);
//
//    // Create an imagelist from this bitmap.
//    HIMAGELIST himl = PhImageListCreate(cx, cy, ILC_COLOR, frames, frames);
//    PhImageListAddBitmap(himl, hbmCheckboxes, NULL);
//
//    // Don't need the bitmap any more.
//    DeleteObject(hbmCheckboxes);
//
//    return himl;
//}
//
//VOID OnThemeChange(HWND hwnd) // WM_THEMECHANGE
//{
//    // Rebuild the state images to match the new theme.
//    HIMAGELIST himl = CreateTreeViewCheckBoxes(g_hwndChild, 16, 16);
//    ImageList_Destroy(TreeView_SetImageList(g_hwndChild, himl, TVSIL_STATE));
//}

VOID PhThemeDrawButtonIcon(
    _In_ LPNMCUSTOMDRAW DrawInfo,
    _In_ HICON ButtonIcon,
    _In_ PRECT ButtonRect,
    _In_ LONG WindowDpi
    )
{
    BOOL result;
    ICONINFO iconInfo;
    BITMAP bmp;
    LONG width = PhGetSystemMetrics(SM_CXSMICON, WindowDpi);
    LONG height = PhGetSystemMetrics(SM_CYSMICON, WindowDpi);

    memset(&iconInfo, 0, sizeof(ICONINFO));
    memset(&bmp, 0, sizeof(BITMAP));

    result = GetIconInfo(ButtonIcon, &iconInfo);

    if (iconInfo.hbmColor)
    {
        if (GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp))
        {
            width = bmp.bmWidth;
            height = bmp.bmHeight;
        }

        DeleteBitmap(iconInfo.hbmColor);
    }
    else if (iconInfo.hbmMask)
    {
        if (GetObject(iconInfo.hbmMask, sizeof(BITMAP), &bmp))
        {
            width = bmp.bmWidth;
            height = bmp.bmHeight / 2;
        }

        DeleteBitmap(iconInfo.hbmMask);
    }

    DrawIconEx(
        DrawInfo->hdc,
        ButtonRect->left + ((ButtonRect->right - ButtonRect->left) - width) / 2,
        ButtonRect->top + ((ButtonRect->bottom - ButtonRect->top) - height) / 2,
        ButtonIcon,
        width,
        height,
        0,
        NULL,
        DI_NORMAL
        );

    if (!result) // HACK
    {
        BUTTON_IMAGELIST imageList = { 0 };

        if (Button_GetImageList(DrawInfo->hdr.hwndFrom, &imageList) && imageList.himl)
        {
            ButtonRect->left += PhGetDpi(1, WindowDpi);

            PhImageListDrawIcon(
                imageList.himl,
                0,
                DrawInfo->hdc,
                ButtonRect->left, // + ((ButtonRect->right - ButtonRect->left) - width) / 2,
                ButtonRect->top + ((ButtonRect->bottom - ButtonRect->top) - height) / 2,
                ILD_NORMAL,
                FALSE
                );

            ButtonRect->left += PhGetDpi(5, WindowDpi);
        }
    }
}

LRESULT CALLBACK PhThemeWindowDrawButton(
    _In_ LPNMCUSTOMDRAW DrawInfo
    )
{
    LONG_PTR buttonStyle;

    buttonStyle = PhGetWindowStyle(DrawInfo->hdr.hwndFrom);
    // COMMANDLINK unsupported
    if ((buttonStyle & BS_COMMANDLINK) == BS_COMMANDLINK || (buttonStyle & BS_DEFCOMMANDLINK) == BS_DEFCOMMANDLINK)
        return CDRF_DODEFAULT;

    BOOLEAN isGrayed = (DrawInfo->uItemState & CDIS_GRAYED) == CDIS_GRAYED;
    BOOLEAN isChecked = (DrawInfo->uItemState & CDIS_CHECKED) == CDIS_CHECKED;
    BOOLEAN isMixed = (DrawInfo->uItemState & CDIS_INDETERMINATE) == CDIS_INDETERMINATE;
    BOOLEAN isDisabled = (DrawInfo->uItemState & CDIS_DISABLED) == CDIS_DISABLED;
    BOOLEAN isSelected = (DrawInfo->uItemState & CDIS_SELECTED) == CDIS_SELECTED;
    BOOLEAN isHighlighted = (DrawInfo->uItemState & CDIS_HOT) == CDIS_HOT;
    BOOLEAN isFocused = (DrawInfo->uItemState & CDIS_FOCUS) == CDIS_FOCUS;
    BOOLEAN showKeyboardCues = (DrawInfo->uItemState & CDIS_SHOWKEYBOARDCUES) == CDIS_SHOWKEYBOARDCUES;
    UINT hidePrefixFlag = showKeyboardCues ? 0 : DT_HIDEPREFIX;
    RECT bufferRect =
    {
        0, 0,
        DrawInfo->rc.right - DrawInfo->rc.left,
        DrawInfo->rc.bottom - DrawInfo->rc.top
    };

    switch (DrawInfo->dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            PPH_STRING buttonText;
            HICON buttonIcon;
            LONG dpiValue;

            BOOLEAN isCheckbox = (buttonStyle & BS_AUTOCHECKBOX) == BS_AUTOCHECKBOX || (buttonStyle & BS_AUTO3STATE) == BS_AUTO3STATE;
            BOOLEAN isRadio = (buttonStyle & BS_AUTORADIOBUTTON) == BS_AUTORADIOBUTTON;

            if (!isCheckbox && !isRadio && PhEnableThemeNativeButtons && !PhEnableThemeAcrylicWindowSupport)
                return CDRF_DODEFAULT;

            dpiValue = PhGetWindowDpi(DrawInfo->hdr.hwndFrom);
            buttonText = PhGetWindowText(DrawInfo->hdr.hwndFrom);

            if (!(buttonIcon = Static_GetIcon(DrawInfo->hdr.hwndFrom, 0)))
                buttonIcon = (HICON)SendMessage(DrawInfo->hdr.hwndFrom, BM_GETIMAGE, IMAGE_ICON, 0);

            // Add support for disabled and tristate checkboxes, support for radio with multiline (ex. TaskDialog) (Dart Vanya)
            if (isCheckbox || isRadio)
            {
                INT state = isCheckbox ? CBS_UNCHECKEDNORMAL : RBS_UNCHECKEDNORMAL;
                HTHEME themeHandle;

                isChecked = Button_GetCheck(DrawInfo->hdr.hwndFrom) & BST_CHECKED;
                isMixed = Button_GetCheck(DrawInfo->hdr.hwndFrom) & BST_INDETERMINATE;

                if (isCheckbox)
                {
                    if (isDisabled)
                        state = isChecked ? CBS_CHECKEDDISABLED : isMixed ? CBS_MIXEDDISABLED : CBS_UNCHECKEDDISABLED;
                    else if (isSelected)
                        state = isChecked ? CBS_CHECKEDPRESSED : isMixed ? CBS_MIXEDPRESSED : CBS_UNCHECKEDPRESSED;
                    else if (isHighlighted)
                        state = isChecked ? CBS_CHECKEDHOT : isMixed ? CBS_MIXEDHOT : CBS_UNCHECKEDHOT;
                    else
                        state = isChecked ? CBS_CHECKEDNORMAL : isMixed ? CBS_MIXEDNORMAL : CBS_UNCHECKEDNORMAL;
                }
                else
                {
                    if (isDisabled)
                        state = isChecked ? RBS_CHECKEDDISABLED : RBS_UNCHECKEDDISABLED;
                    else if (isSelected)
                        state = isChecked ? RBS_CHECKEDPRESSED : RBS_UNCHECKEDPRESSED;
                    else if (isHighlighted)
                        state = isChecked ? RBS_CHECKEDHOT : RBS_UNCHECKEDHOT;
                    else
                        state = isChecked ? RBS_CHECKEDNORMAL : RBS_UNCHECKEDNORMAL;
                }

                if (buttonIcon)
                {
                    if (isSelected || isChecked)
                    {
                        //switch (PhpThemeColorMode)
                        //{
                        //case 0: // New colors
                        //    //SetTextColor(DrawInfo->hdc, RGB(0, 0, 0xff));
                        //    SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHT));
                        //    break;
                        //case 1: // Old colors
                        SetTextColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                        SetDCBrushColor(DrawInfo->hdc, PhMakeColorBrighter(PhThemeWindowBackground2Color, 13)); // RGB(78, 78, 78)
                        FillRect(DrawInfo->hdc, &DrawInfo->rc, PhGetStockBrush(DC_BRUSH));
                    }
                    else if (isHighlighted)
                    {
                        //switch (PhpThemeColorMode)
                        //{
                        //case 0: // New colors
                        //    //SetTextColor(DrawInfo->hdc, RGB(0, 0, 0xff));
                        //    SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHT));
                        //    break;
                        //case 1: // Old colors
                        SetTextColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                        SetDCBrushColor(DrawInfo->hdc, PhThemeWindowBackground2Color);
                        FillRect(DrawInfo->hdc, &DrawInfo->rc, PhGetStockBrush(DC_BRUSH));
                    }
                    else
                    {
                        SetTextColor(DrawInfo->hdc, !isDisabled ? PhThemeWindowTextColor : RGB(0x9B, 0x9B, 0x9B));
                        //SetDCBrushColor(DrawInfo->hdc, PhThemeWindowBackgroundColor); // WindowForegroundColor
                        FillRect(DrawInfo->hdc, &DrawInfo->rc, PhThemeWindowBackgroundBrush);
                    }

                    SetDCBrushColor(DrawInfo->hdc, PhThemeWindowBackground2Color);
                    FrameRect(DrawInfo->hdc, &DrawInfo->rc, PhGetStockBrush(DC_BRUSH));

                    PhThemeDrawButtonIcon(DrawInfo, buttonIcon, &bufferRect, dpiValue);
                }
                else
                {
                    SetBkMode(DrawInfo->hdc, TRANSPARENT);
                    if (showKeyboardCues)   // fix artefacts on Alt press
                    {
                        SetDCBrushColor(DrawInfo->hdc, PhGetWindowContext(GetAncestor(DrawInfo->hdr.hwndFrom, GA_ROOT), LONG_MAX) ?
							PhThemeWindowBackgroundColor : PhThemeWindowBackground2Color);	// TaskDialog HACK
                        FillRect(DrawInfo->hdc, &DrawInfo->rc, PhGetStockBrush(DC_BRUSH));
                    }
                    SetTextColor(DrawInfo->hdc, !isDisabled ? PhThemeWindowTextColor : RGB(0x9B, 0x9B, 0x9B));

                    if (themeHandle = PhOpenThemeData(DrawInfo->hdr.hwndFrom, VSCLASS_BUTTON, dpiValue))
                    {
                        SIZE checkBoxSize = { 0 };
                        SIZE textSize = { 0 };
                        INT linesCount;

                        PhGetThemePartSize(
                            themeHandle,
                            DrawInfo->hdc,
                            isCheckbox ? BP_CHECKBOX : BP_RADIOBUTTON,
                            state,
                            &bufferRect,
                            TS_TRUE,
                            &checkBoxSize
                            );
                        GetTextExtentPoint32W(DrawInfo->hdc, L"T", 1, &textSize);

                        bufferRect.left = 0;
                        bufferRect.right = checkBoxSize.cx;
                        linesCount = (bufferRect.bottom - bufferRect.top) / textSize.cy;
                        if (linesCount > 1)
                            bufferRect.bottom -= textSize.cy * (linesCount - 1);    // HACK (very sensitive value)

                        //if (IsThemeBackgroundPartiallyTransparent(themeHandle, isCheckbox ? BP_CHECKBOX : BP_RADIOBUTTON, state))
                        //    DrawThemeParentBackground(DrawInfo->hdr.hwndFrom, DrawInfo->hdc, NULL);

                        PhDrawThemeBackground(
                            themeHandle,
                            DrawInfo->hdc,
                            isCheckbox ? BP_CHECKBOX : BP_RADIOBUTTON,
                            state,
                            &bufferRect,
                            NULL
                            );

                        bufferRect = DrawInfo->rc;
                        bufferRect.left = checkBoxSize.cx + 4; // TNP_ICON_RIGHT_PADDING

                        if (linesCount == 1)
                        {
                            DrawText(
                                DrawInfo->hdc,
                                buttonText->Buffer,
                                (UINT)buttonText->Length / sizeof(WCHAR),
                                &bufferRect,
                                DT_LEFT | DT_SINGLELINE | DT_VCENTER | hidePrefixFlag
                                );
                        }
                        else
                        {
                            DrawText(
                                DrawInfo->hdc,
                                buttonText->Buffer,
                                (UINT)buttonText->Length / sizeof(WCHAR),
                                &bufferRect,
                                DT_LEFT | DT_TOP | DT_CALCRECT | hidePrefixFlag
                                );

                            bufferRect.top = (DrawInfo->rc.bottom - DrawInfo->rc.top) / 2 - (bufferRect.bottom - bufferRect.top) / 2 - 1;
                            bufferRect.bottom = DrawInfo->rc.bottom, bufferRect.right = DrawInfo->rc.right;

                            DrawText(
                                DrawInfo->hdc,
                                buttonText->Buffer,
                                (UINT)buttonText->Length / sizeof(WCHAR),
                                &bufferRect,
                                DT_LEFT | DT_TOP | hidePrefixFlag
                                );
                        }

                        if (showKeyboardCues && isFocused)
                        {
                            DrawText(
                                DrawInfo->hdc,
                                buttonText->Buffer,
                                (UINT)buttonText->Length / sizeof(WCHAR),
                                &bufferRect,
                                DT_LEFT | DT_TOP | DT_CALCRECT | hidePrefixFlag
                                );
                            PhInflateRect(&bufferRect, 1, 0);
                            bufferRect.top += 1, bufferRect.bottom += 2;
                            if (bufferRect.bottom > DrawInfo->rc.bottom - 1) bufferRect.bottom = DrawInfo->rc.bottom - 1;

                            for (INT i = 0; i < bufferRect.right - bufferRect.left - 1; i += 2)
                                SetPixel(DrawInfo->hdc, bufferRect.left + i + 1, bufferRect.bottom, PhThemeWindowHighlight2Color);
                            for (INT i = 0; i < bufferRect.bottom - bufferRect.top - 1; i += 2)
                                SetPixel(DrawInfo->hdc, bufferRect.right, bufferRect.bottom - i - 1, PhThemeWindowHighlight2Color);
                            for (INT i = 0; i < bufferRect.right - bufferRect.left - 1; i += 2)
                                SetPixel(DrawInfo->hdc, bufferRect.right - i - 1, bufferRect.top, PhThemeWindowHighlight2Color);
                            for (INT i = 0; i < bufferRect.bottom - bufferRect.top - 1; i += 2)
                                SetPixel(DrawInfo->hdc, bufferRect.left, bufferRect.top + i + 1, PhThemeWindowHighlight2Color);
                        }

                        PhCloseThemeData(themeHandle);
                    }
                    else
                    {
                        if (isChecked)
                        {
                            HFONT newFont = PhDuplicateFontWithNewHeight(PhApplicationFont, 16, dpiValue);
                            HFONT oldFont;

                            oldFont = SelectFont(DrawInfo->hdc, newFont);
                            DrawText(
                                DrawInfo->hdc,
                                L"\u2611",
                                1,
                                &DrawInfo->rc,
                                DT_LEFT | DT_SINGLELINE | DT_VCENTER
                                );
                            SelectFont(DrawInfo->hdc, oldFont);
                            DeleteFont(newFont);
                        }
                        else
                        {
                            ODS_DISABLED;

                            HFONT newFont = PhDuplicateFontWithNewHeight(PhApplicationFont, 22, dpiValue);
                            HFONT oldFont;

                            oldFont = SelectFont(DrawInfo->hdc, newFont);
                            DrawText(
                                DrawInfo->hdc,
                                L"\u2610",
                                1,
                                &DrawInfo->rc,
                                DT_LEFT | DT_SINGLELINE | DT_VCENTER
                                );
                            SelectFont(DrawInfo->hdc, oldFont);
                            DeleteFont(newFont);
                        }

                        bufferRect.left = 17;
                        bufferRect.right = DrawInfo->rc.right;

                        DrawText(
                            DrawInfo->hdc,
                            buttonText->Buffer,
                            (UINT)buttonText->Length / sizeof(WCHAR),
                            &bufferRect,
                            DT_LEFT | DT_VCENTER | DT_SINGLELINE | hidePrefixFlag
                            );
                    }
                }
            }
            else
            {
                if (isSelected)
                {
                    //switch (PhpThemeColorMode)
                    //{
                    //case 0: // New colors
                    //    //SetTextColor(DrawInfo->hdc, RGB(0, 0, 0xff));
                    //    SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHT));
                    //    break;
                    //case 1: // Old colors
                    SetTextColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    SetDCBrushColor(DrawInfo->hdc, PhMakeColorBrighter(PhThemeWindowBackground2Color, 13)); // RGB(78, 78, 78)
                    FillRect(DrawInfo->hdc, &DrawInfo->rc, PhGetStockBrush(DC_BRUSH));
                }
                else if (isHighlighted)
                {
                    //switch (PhpThemeColorMode)
                    //{
                    //case 0: // New colors
                    //    //SetTextColor(DrawInfo->hdc, RGB(0, 0, 0xff));
                    //    SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHT));
                    //    break;
                    //case 1: // Old colors
                    SetTextColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    SetDCBrushColor(DrawInfo->hdc, PhThemeWindowBackground2Color);
                    FillRect(DrawInfo->hdc, &DrawInfo->rc, PhGetStockBrush(DC_BRUSH));
                }
                else
                {
                    SetTextColor(DrawInfo->hdc, !isDisabled ? PhThemeWindowTextColor : RGB(0x9B, 0x9B, 0x9B));
                    //SetDCBrushColor(DrawInfo->hdc, PhThemeWindowBackgroundColor); // WindowForegroundColor
                    FillRect(DrawInfo->hdc, &DrawInfo->rc, PhThemeWindowBackgroundBrush);
                }

                SetBkMode(DrawInfo->hdc, TRANSPARENT);
                SetDCBrushColor(DrawInfo->hdc, !isFocused ? PhThemeWindowBackground2Color : PhThemeWindowHighlightColor);
                FrameRect(DrawInfo->hdc, &DrawInfo->rc, PhGetStockBrush(DC_BRUSH));

                PhThemeDrawButtonIcon(DrawInfo, buttonIcon, &bufferRect, dpiValue);

                if ((buttonStyle & BS_ICON) != BS_ICON)
                {
                    DrawText(
                        DrawInfo->hdc,
                        buttonText->Buffer,
                        (UINT)buttonText->Length / sizeof(WCHAR),
                        &bufferRect,
                        DT_CENTER | DT_SINGLELINE | DT_VCENTER | hidePrefixFlag
                        );
                }
            }
            PhDereferenceObject(buttonText);
        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

LRESULT CALLBACK PhThemeWindowDrawRebar(
    _In_ LPNMCUSTOMDRAW DrawInfo
    )
{
    // temp chevron workaround until location/state supported.
    if (DrawInfo->dwDrawStage != CDDS_PREPAINT)
    {
        // Skip erasing of background to remove flickering when close settings or lock/unlock toolbar (Dart Vanya)
        return DrawInfo->dwDrawStage == CDDS_PREERASE ? CDRF_SKIPDEFAULT : CDRF_DODEFAULT;
    }

    switch (DrawInfo->dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            //REBARBANDINFO bandinfo = { sizeof(REBARBANDINFO), RBBIM_CHILD | RBBIM_STYLE | RBBIM_CHEVRONLOCATION | RBBIM_CHEVRONSTATE };
            //BOOL havebandinfo = (BOOL)SendMessage(DrawInfo->hdr.hwndFrom, RB_GETBANDINFO, DrawInfo->dwItemSpec, (LPARAM)&bandinfo);

            SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor);
            FillRect(DrawInfo->hdc, &DrawInfo->rc, PhThemeWindowBackgroundBrush);
        }
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
        {

        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

LRESULT CALLBACK PhThemeWindowDrawToolbar(
    _In_ LPNMTBCUSTOMDRAW DrawInfo
    )
{
    switch (DrawInfo->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
    case CDDS_ITEMPREPAINT:
        {
            TBBUTTONINFO buttonInfo =
            {
                sizeof(TBBUTTONINFO),
                TBIF_STYLE | TBIF_COMMAND | TBIF_STATE | TBIF_IMAGE
            };

            SetBkMode(DrawInfo->nmcd.hdc, TRANSPARENT);

            LONG dpiValue;
            INT bitmapWidth = 0;
            INT bitmapHeight = 0;

            ULONG currentIndex = (ULONG)SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_COMMANDTOINDEX,
                DrawInfo->nmcd.dwItemSpec,
                0
                );
            BOOLEAN isHighlighted = SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_GETHOTITEM,
                0,
                0
                ) == currentIndex;
            BOOLEAN isMouseDown = SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_ISBUTTONPRESSED,
                DrawInfo->nmcd.dwItemSpec,
                0
                ) == 0;
            BOOLEAN isEnabled = SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_ISBUTTONENABLED,
                DrawInfo->nmcd.dwItemSpec,
                0
                ) != 0;

            if (SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_GETBUTTONINFO,
                (ULONG)DrawInfo->nmcd.dwItemSpec,
                (LPARAM)&buttonInfo
                ) == INT_ERROR)
            {
                break;
            }

            BOOLEAN isDropDown = !!(buttonInfo.fsStyle & BTNS_WHOLEDROPDOWN);

            //if (isMouseDown)
            //{
            //    SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
            //    SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(0xff, 0xff, 0xff));
            //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhGetStockBrush(DC_BRUSH));
            //}

            if (isHighlighted)
            {
                //INT stateId;
                //// Themed background
                //if (node->Selected)
                //{
                //    if (i == Context->HotNodeIndex)
                //        stateId = TREIS_HOTSELECTED;
                //    else if (!Context->HasFocus)
                //        stateId = TREIS_SELECTEDNOTFOCUS;
                //    else
                //        stateId = TREIS_SELECTED;
                //}
                //else
                //{
                //    if (i == Context->HotNodeIndex)
                //        stateId = TREIS_HOT;
                //    else
                //        stateId = INT_MAX;
                //}
                //// Themed background
                //if (stateId != INT_MAX)
                //if (!Context->FixedColumnVisible)
                //{
                //    rowRect.left = Context->NormalLeft - hScrollPosition;
                //}
                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                //    SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowBackgroundColor);
                //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhGetStockBrush(DC_BRUSH));
                //    break;
                //case 1: // Old colors
                //    SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                //    SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowHighlightColor);
                //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhGetStockBrush(DC_BRUSH));
                //    break;
                //}

                SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowHighlightColor);
                FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhGetStockBrush(DC_BRUSH));
            }
            else
            {
                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor); // RGB(0x0, 0x0, 0x0));
                //    SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowBackgroundColor); // GetSysColor(COLOR_3DFACE));// RGB(0xff, 0xff, 0xff));
                //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhGetStockBrush(DC_BRUSH));
                //    break;
                //case 1: // Old colors
                //    SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                //    SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowBackgroundColor); //PhThemeWindowForegroundColor);
                //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhGetStockBrush(DC_BRUSH));
                //    break;
                //}

                BOOLEAN isPressed = buttonInfo.fsState & TBSTATE_PRESSED;
                SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor); // RGB(0x0, 0x0, 0x0));
                //SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowBackgroundColor); // GetSysColor(COLOR_3DFACE));// RGB(0xff, 0xff, 0xff));
                if (!isPressed)
                    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhThemeWindowBackgroundBrush);
                else
                {
                    SetDCBrushColor(DrawInfo->nmcd.hdc, PhMakeColorBrighter(PhThemeWindowBackground2Color, 31)); // RGB(96, 96, 96)
                    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhGetStockBrush(DC_BRUSH));
                }
            }

            dpiValue = PhGetWindowDpi(DrawInfo->nmcd.hdr.hwndFrom);

            SelectFont(DrawInfo->nmcd.hdc, GetWindowFont(DrawInfo->nmcd.hdr.hwndFrom));

            if (buttonInfo.iImage != I_IMAGECALLBACK)
            {
                HIMAGELIST toolbarImageList;

                if (toolbarImageList = (HIMAGELIST)SendMessage(
                    DrawInfo->nmcd.hdr.hwndFrom,
                    TB_GETIMAGELIST,
                    0,
                    0
                    ))
                {
                    LONG x;
                    LONG y;

                    PhImageListGetIconSize(toolbarImageList, &bitmapWidth, &bitmapHeight);

                    if (buttonInfo.fsStyle & BTNS_SHOWTEXT)
                    {
                        DrawInfo->nmcd.rc.left += PhGetSystemMetrics(SM_CXEDGE, dpiValue); // PhGetDpi(5, dpiValue);
                        x = DrawInfo->nmcd.rc.left;// + ((DrawInfo->nmcd.rc.right - DrawInfo->nmcd.rc.left) - PhSmallIconSize.X) / 2;
                        y = DrawInfo->nmcd.rc.top + ((DrawInfo->nmcd.rc.bottom - DrawInfo->nmcd.rc.top) - bitmapHeight) / 2;
                    }
                    else
                    {
                        x = DrawInfo->nmcd.rc.left + ((DrawInfo->nmcd.rc.right - DrawInfo->nmcd.rc.left) - bitmapWidth) / 2 - (isDropDown * 4);
                        y = DrawInfo->nmcd.rc.top + ((DrawInfo->nmcd.rc.bottom - DrawInfo->nmcd.rc.top) - bitmapHeight) / 2;
                    }

                    PhImageListDrawIcon(
                        toolbarImageList,
                        buttonInfo.iImage,
                        DrawInfo->nmcd.hdc,
                        x,
                        y,
                        ILD_NORMAL,
                        !isEnabled
                        );

                    if (isDropDown)
                    {
                        HDC hdc = DrawInfo->nmcd.hdc;
                        LPRECT rc = &DrawInfo->nmcd.rc;
                        int triangleLeft = rc->right - 11, triangleTop = (rc->bottom - rc->top) / 2 - 2;
                        POINT vertices[] = { {triangleLeft, triangleTop}, {triangleLeft + 6, triangleTop}, {triangleLeft + 3, triangleTop + 3} };
                        SetDCPenColor(hdc, RGB(0xDE, 0xDE, 0xDE));
                        SetDCBrushColor(hdc, RGB(0xDE, 0xDE, 0xDE));
                        SelectPen(hdc, PhGetStockPen(DC_PEN));
                        SelectBrush(hdc, PhGetStockBrush(DC_BRUSH));
                        Polygon(hdc, vertices, _countof(vertices));
                    }

                    //return CDRF_SKIPDEFAULT | CDRF_NOTIFYPOSTPAINT;
                }
            }
            else
            {
                return CDRF_DODEFAULT; // Required for I_IMAGECALLBACK (dmex)
            }

            if (buttonInfo.fsStyle & BTNS_SHOWTEXT)
            {
                RECT textRect = DrawInfo->nmcd.rc;
                WCHAR buttonText[MAX_PATH] = L"";

                SendMessage(
                    DrawInfo->nmcd.hdr.hwndFrom,
                    TB_GETBUTTONTEXT,
                    (ULONG)DrawInfo->nmcd.dwItemSpec,
                    (LPARAM)buttonText
                    );

                textRect.left += bitmapWidth - (isDropDown * 12); // PhGetDpi(10, dpiValue);
                DrawText(
                    DrawInfo->nmcd.hdc,
                    buttonText,
                    (UINT)PhCountStringZ(buttonText),
                    &textRect,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                    );
            }

            //DrawInfo->clrText = RGB(0x0, 0xff, 0);
            //return TBCDRF_USECDCOLORS | CDRF_NEWFONT;
        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

LRESULT CALLBACK PhpThemeWindowDrawListViewGroup(
    _In_ LPNMLVCUSTOMDRAW DrawInfo
    )
{
    switch (DrawInfo->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            LONG dpiValue = PhGetWindowDpi(DrawInfo->nmcd.hdr.hwndFrom);
            HFONT fontHandle = NULL;
            LVGROUP groupInfo;

            {
                NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

                if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, dpiValue))
                {
                    metrics.lfMessageFont.lfHeight = PhGetDpi(-11, dpiValue);
                    metrics.lfMessageFont.lfWeight = FW_BOLD;

                    fontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }

            SetBkMode(DrawInfo->nmcd.hdc, TRANSPARENT);
            //SelectFont(DrawInfo->nmcd.hdc, GetWindowFont(DrawInfo->nmcd.hdr.hwndFrom));
            if (fontHandle) SelectFont(DrawInfo->nmcd.hdc, fontHandle);

            memset(&groupInfo, 0, sizeof(LVGROUP));
            groupInfo.cbSize = sizeof(LVGROUP);
            groupInfo.mask = LVGF_HEADER;

            if (ListView_GetGroupInfo(DrawInfo->nmcd.hdr.hwndFrom, (ULONG)DrawInfo->nmcd.dwItemSpec, &groupInfo) != INT_ERROR)
            {
                SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowBackground2Color);

                DrawInfo->rcText.top += PhGetDpi(2, dpiValue);
                DrawInfo->rcText.bottom -= PhGetDpi(2, dpiValue);
                FillRect(DrawInfo->nmcd.hdc, &DrawInfo->rcText, PhGetStockBrush(DC_BRUSH));
                DrawInfo->rcText.top -= PhGetDpi(2, dpiValue);
                DrawInfo->rcText.bottom += PhGetDpi(2, dpiValue);

                if (groupInfo.pszHeader)
                {
                    DrawInfo->rcText.left += PhGetDpi(10, dpiValue);
                    DrawText(
                        DrawInfo->nmcd.hdc,
                        groupInfo.pszHeader,
                        (UINT)PhCountStringZ(groupInfo.pszHeader),
                        &DrawInfo->rcText,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                        );
                    DrawInfo->rcText.left -= PhGetDpi(10, dpiValue);
                }
            }

            if (fontHandle)
            {
                DeleteFont(fontHandle);
            }
        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

LRESULT CALLBACK PhpThemeWindowSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    if (!(oldWndProc = PhGetWindowContext(hWnd, LONG_MAX)))
        return FALSE;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(hWnd, LONG_MAX);
            PhSetWindowProcedure(hWnd, oldWndProc);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;

            switch (data->code)
            {
            case NM_CUSTOMDRAW:
                {
                    LPNMCUSTOMDRAW customDraw = (LPNMCUSTOMDRAW)lParam;
                    WCHAR className[MAX_PATH];

                    GETCLASSNAME_OR_NULL(customDraw->hdr.hwndFrom, className);

                    if (PhEqualStringZ(className, WC_BUTTON, FALSE))
                    {
                        return PhThemeWindowDrawButton(customDraw);
                    }
                    else if (PhEqualStringZ(className, REBARCLASSNAME, FALSE))
                    {
                        return PhThemeWindowDrawRebar(customDraw);
                    }
                    else if (PhEqualStringZ(className, TOOLBARCLASSNAME, FALSE))
                    {
                        return PhThemeWindowDrawToolbar((LPNMTBCUSTOMDRAW)customDraw);
                    }
                    else if (PhEqualStringZ(className, WC_LISTVIEW, FALSE))
                    {
                        LPNMLVCUSTOMDRAW listViewCustomDraw = (LPNMLVCUSTOMDRAW)customDraw;

                        if (listViewCustomDraw->dwItemType == LVCDI_GROUP)
                        {
                            return PhpThemeWindowDrawListViewGroup(listViewCustomDraw);
                        }
                    }
                    else
                    {
                        //dprintf("NM_CUSTOMDRAW: %S\r\n", className);
                    }
                }
            }
        }
        break;
    case WM_CTLCOLOREDIT:
        {
            HDC hdc = (HDC)wParam;

            //Fix typing in multiline edit (Dart Vanya)
            if (PhGetWindowStyle((HWND)lParam) & ES_MULTILINE)
                SetBkColor(hdc, PhThemeWindowBackground2Color);
            else
                SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, PhThemeWindowTextColor);
            SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            HDC hdc = (HDC)wParam;

            if (uMsg == WM_CTLCOLORBTN)     // for correct drawing of system KEYBOARDCUES
                SetBkColor(hdc, PhThemeWindowBackground2Color);
            else
                SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, PhThemeWindowTextColor);
            return (INT_PTR)PhThemeWindowBackgroundBrush;
        }
        break;
    case WM_MEASUREITEM:
        if (PhThemeWindowMeasureItem(hWnd, (LPMEASUREITEMSTRUCT)lParam))
            return TRUE;
        break;
    case WM_DRAWITEM:
        if (PhThemeWindowDrawItem(hWnd, (LPDRAWITEMSTRUCT)lParam))
            return TRUE;
        break;
    case WM_NCPAINT:
    case WM_NCACTIVATE:
        {
            LRESULT result = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);

            PhWindowThemeMainMenuBorder(hWnd);

            return result;
        }
        break;
    }

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
}

VOID ThemeWindowRenderGroupBoxControl(
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect,
    _In_ WNDPROC WindowProcedure
    )
{
    ULONG returnLength;
    WCHAR text[0x80];
    LONG dpiValue;

    SetBkMode(bufferDc, TRANSPARENT);
    SelectFont(bufferDc, GetWindowFont(WindowHandle));
    SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
    FrameRect(bufferDc, clientRect, PhGetStockBrush(DC_BRUSH));

    dpiValue = PhGetWindowDpi(WindowHandle);

    if (NT_SUCCESS(PhGetWindowTextToBuffer(WindowHandle, PH_GET_WINDOW_TEXT_INTERNAL, text, RTL_NUMBER_OF(text), &returnLength)))
    {
        SIZE nameSize = { 0 };
        RECT bufferRect;

        GetTextExtentPoint32(
            bufferDc,
            text,
            returnLength,
            &nameSize
            );

        bufferRect.left = 0;
        bufferRect.top = 0;
        bufferRect.right = clientRect->right;
        bufferRect.bottom = nameSize.cy;

        SetTextColor(bufferDc, PhThemeWindowTextColor);
        SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
        FillRect(bufferDc, &bufferRect, PhGetStockBrush(DC_BRUSH));

        bufferRect.left += PhGetDpi(10, dpiValue);
        DrawText(
            bufferDc,
            text,
            returnLength,
            &bufferRect,
            DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE
            );
        bufferRect.left -= PhGetDpi(10, dpiValue);
    }
}

LRESULT CALLBACK PhpThemeWindowGroupBoxSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    if (!(oldWndProc = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    switch (uMsg)
    {
        case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
            PhSetWindowProcedure(WindowHandle, oldWndProc);
        }
        break;
    case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT clientRect;

            GetClientRect(WindowHandle, &clientRect);

            ThemeWindowRenderGroupBoxControl(WindowHandle, hdc, &clientRect, oldWndProc);
        }
        return TRUE;
    case WM_ENABLE:
        if (!wParam)    // fix drawing when window visible and switches to disabled
            return 0;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(WindowHandle, &ps);
            EndPaint(WindowHandle, &ps);
        }
        return 0;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

VOID ThemeWindowRenderTabControl(
    _In_ PPHP_THEME_WINDOW_TAB_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect,
    _In_ WNDPROC WindowProcedure
    )
{
    //RECT windowRect;

    //GetWindowRect(WindowHandle, &windowRect);

    //CallWindowProc(WindowProcedure, WindowHandle, WM_PRINTCLIENT, (WPARAM)bufferDc, PRF_CLIENT);

    //TabCtrl_AdjustRect(WindowHandle, FALSE, clientRect); // Make sure we don't paint in the client area.
    //ExcludeClipRect(bufferDc, clientRect->left, clientRect->top, clientRect->right, clientRect->bottom);

    //windowRect.right -= windowRect.left;
    //windowRect.bottom -= windowRect.top;
    //windowRect.left = 0;
    //windowRect.top = 0;

    //clientRect->left = windowRect.left;
    //clientRect->top = windowRect.top;
    //clientRect->right = windowRect.right;
    //clientRect->bottom = windowRect.bottom;

    SetBkMode(bufferDc, TRANSPARENT);
    SelectFont(bufferDc, GetWindowFont(WindowHandle));

    SetTextColor(bufferDc, PhThemeWindowTextColor);
    FillRect(bufferDc, clientRect, PhThemeWindowBackgroundBrush);

    //switch (PhpThemeColorMode)
    //{
    //case 0: // New colors
    //    {
    //        //SetTextColor(DrawInfo->hdc, RGB(0x0, 0x0, 0x0));
    //        //SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_3DFACE)); // RGB(0xff, 0xff, 0xff));
    //    }
    //    break;
    //case 1: // Old colors
    //    {
    //        //SetTextColor(DrawInfo->hdc, RGB(0xff, 0xff, 0xff));
    //        //SetDCBrushColor(DrawInfo->hdc, PhThemeWindowBackground2Color);
    //    }
    //    break;
    //}

    //switch (PhpThemeColorMode)
    //{
    //case 0: // New colors
    //    //SetTextColor(hdc, RGB(0x0, 0xff, 0x0));
    //    SetDCBrushColor(hdc, GetSysColor(COLOR_3DFACE));// PhThemeWindowTextColor);
    //    FillRect(hdc, &clientRect, PhGetStockBrush(DC_BRUSH));
    //    break;
    //case 1: // Old colors
    //    //SetTextColor(hdc, PhThemeWindowTextColor);
    //    SetDCBrushColor(hdc, RPhThemeWindowBackground2Color);
    //    FillRect(hdc, &clientRect, PhGetStockBrush(DC_BRUSH));
    //    break;
    //}

    INT currentSelection = TabCtrl_GetCurSel(WindowHandle);
    INT count = TabCtrl_GetItemCount(WindowHandle);
    RECT itemRect = { 0 };
    //RECT itemRectHighlighted;
    //INT itemHighlighted = INT_ERROR;
    INT headerBottom;
    INT oldTop;

    oldTop = clientRect->top;
    TabCtrl_GetItemRect(WindowHandle, 0, &itemRect);
    clientRect->top += (itemRect.bottom - itemRect.top) * TabCtrl_GetRowCount(WindowHandle) + 2;

    if (PhEnableThemeTabBorders)
    {
        SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
        FrameRect(bufferDc, clientRect, PhGetStockBrush(DC_BRUSH));
    }

    headerBottom = clientRect->top;
    clientRect->top = oldTop;

    TCITEM tabItem;
    WCHAR tabHeaderText[MAX_PATH] = L"";

    memset(&tabItem, 0, sizeof(TCITEM));

    tabItem.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_STATE;
    tabItem.dwStateMask = TCIS_BUTTONPRESSED | TCIS_HIGHLIGHTED;
    tabItem.cchTextMax = RTL_NUMBER_OF(tabHeaderText);
    tabItem.pszText = tabHeaderText;

    for (INT i = 0; i < count; i++)
    {
        if (i == currentSelection)
            continue;

        TabCtrl_GetItemRect(WindowHandle, i, &itemRect);

        PhOffsetRect(&itemRect, 2, 2);
        itemRect.bottom += itemRect.bottom + 1 < headerBottom ? 1 : -1;
        itemRect.right += itemRect.right + 1 < clientRect->right;

        if (PhPtInRect(&itemRect, Context->CursorPos))
        {
            //switch (PhpThemeColorMode)
            //{
            //case 0: // New colors
            //    {
            //        if (currentSelection == i)
            //        {
            //            SetTextColor(bufferDc, RGB(0xff, 0xff, 0xff));
            //            SetDCBrushColor(bufferDc, PhThemeWindowHighlightColor);
            //            FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
            //        }
            //        else
            //        {
            //            SetTextColor(bufferDc, PhThemeWindowTextColor);
            //            SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
            //            FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
            //        }
            //    }
            //    break;
            //case 1: // Old colors
            //SetTextColor(bufferDc, PhThemeWindowTextColor);
            SetDCBrushColor(bufferDc, PhThemeWindowHighlightColor);
            FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));

            //itemRectHighlighted = itemRect;
            //itemHighlighted = i;
            //continue;
        }
        else
        {
            //switch (PhpThemeColorMode)
            //{
            //case 0: // New colors
            //    {
            //        if (currentSelection == i)
            //        {
            //            SetTextColor(bufferDc, RGB(0x0, 0x0, 0x0));
            //            SetDCBrushColor(bufferDc, RGB(0xff, 0xff, 0xff));
            //            FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
            //        }
            //        else
            //        {
            //            SetTextColor(bufferDc, RGB(0, 0, 0));
            //            SetDCBrushColor(bufferDc, GetSysColor(COLOR_3DFACE));// PhThemeWindowTextColor);
            //            FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
            //        }
            //    }
            //    break;
            //case 1: // Old colors
            {
                // SetTextColor(bufferDc, PhThemeWindowTextColor);
                SetDCBrushColor(bufferDc, PhEnableThemeTabBorders ? PhMakeColorBrighter(PhThemeWindowBackgroundColor, 6) : PhThemeWindowBackgroundColor);
                FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
                if (PhEnableThemeTabBorders)
                {
                    SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
                    FrameRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
                }
            }
        }

        {
            if (TabCtrl_GetItem(WindowHandle, i, &tabItem))
            {
                DrawText(
                    bufferDc,
                    tabItem.pszText,
                    (UINT)PhCountStringZ(tabItem.pszText),
                    &itemRect,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                    );
            }
        }
    }

    {
        TabCtrl_GetItemRect(WindowHandle, currentSelection, &itemRect);

        PhOffsetRect(&itemRect, 2, 2);
        itemRect.bottom += itemRect.bottom + 1 < headerBottom ? 1 : -1;
        itemRect.right += itemRect.right + 1 < clientRect->right;
        PhInflateRect(&itemRect, 1, 1);     // draw selected tab slightly bigger
        itemRect.bottom -= 1;
        SetDCBrushColor(bufferDc, PhPtInRect(&itemRect, Context->CursorPos) ? PhThemeWindowHighlightColor :
            PhMakeColorBrighter(PhThemeWindowBackground2Color, 15)); // RGB(80, 80, 80)
        FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));

        if (TabCtrl_GetItem(WindowHandle, currentSelection, &tabItem))
        {
            DrawText(
                bufferDc,
                tabItem.pszText,
                (UINT)PhCountStringZ(tabItem.pszText),
                &itemRect,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                );
        }

        //if (itemHighlighted != INT_ERROR)
        //{
        //    SetDCBrushColor(bufferDc, PhThemeWindowHighlightColor);
        //    FillRect(bufferDc, &itemRectHighlighted, PhGetStockBrush(DC_BRUSH));

        //    if (TabCtrl_GetItem(WindowHandle, itemHighlighted, &tabItem))
        //    {
        //        DrawText(
        //            bufferDc,
        //            tabItem.pszText,
        //            (UINT)PhCountStringZ(tabItem.pszText),
        //            &itemRectHighlighted,
        //            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
        //            );
        //    }
        //}
    }
}

LRESULT CALLBACK PhpThemeWindowTabControlWndSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_TAB_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
            PhSetWindowProcedure(WindowHandle, oldWndProc);

            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_MOUSEMOVE:
        {
            //INT count;
            //INT i;
            //
            //count = TabCtrl_GetItemCount(WindowHandle);
            //
            //for (i = 0; i < count; i++)
            //{
            //    RECT rect = { 0, 0, 0, 0 };
            //    TCITEM entry =
            //    {
            //        TCIF_STATE,
            //        0,
            //        TCIS_HIGHLIGHTED
            //    };
            //
            //    TabCtrl_GetItemRect(WindowHandle, i, &rect);
            //    entry.dwState = PhPtInRect(&rect, context->CursorPos) ? TCIS_HIGHLIGHTED : 0;
            //    TabCtrl_SetItem(WindowHandle, i, &entry);
            //}

            if (!context->MouseActive)
            {
                TRACKMOUSEEVENT trackEvent =
                {
                    sizeof(TRACKMOUSEEVENT),
                    TME_LEAVE,
                    WindowHandle,
                    0
                };

                TrackMouseEvent(&trackEvent);
                context->MouseActive = TRUE;
            }

            context->CursorPos.x = GET_X_LPARAM(lParam);
            context->CursorPos.y = GET_Y_LPARAM(lParam);

            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case WM_MOUSELEAVE:
        {
            //INT count;
            //INT i;
            //
            //count = TabCtrl_GetItemCount(WindowHandle);
            //
            //for (i = 0; i < count; i++)
            //{
            //    TCITEM entry =
            //    {
            //        TCIF_STATE,
            //        0,
            //        TCIS_HIGHLIGHTED
            //    };
            //
            //    TabCtrl_SetItem(WindowHandle, i, &entry);
            //}

            context->CursorPos.x = LONG_MIN;
            context->CursorPos.y = LONG_MIN;

            context->MouseActive = FALSE;
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case WM_PAINT:
        {
            //PAINTSTRUCT ps;
            //HDC BufferedHDC;
            //HPAINTBUFFER BufferedPaint;
            //
            //if (!BeginPaint(WindowHandle, &ps))
            //    break;
            //
            //DEBUG_BEGINPAINT_RECT(WindowHandle, ps.rcPaint);
            //
            //{
            //    RECT clientRect;
            //    GetClientRect(WindowHandle, &clientRect);
            //    //CallWindowProc(oldWndProc, WindowHandle, WM_PAINT, wParam, lParam);
            //    TabCtrl_AdjustRect(WindowHandle, FALSE, &clientRect);
            //    ExcludeClipRect(ps.hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
            //}
            //
            //if (BufferedPaint = BeginBufferedPaint(ps.hdc, &ps.rcPaint, BPBF_COMPATIBLEBITMAP, NULL, &BufferedHDC))
            //{
            //    ThemeWindowRenderTabControl(context, WindowHandle, BufferedHDC, &ps.rcPaint, oldWndProc);
            //    EndBufferedPaint(BufferedPaint, TRUE);
            //}
            //else
            {
                RECT clientRect;
                HDC hdc;
                HDC bufferDc;
                HBITMAP bufferBitmap;
                HBITMAP oldBufferBitmap;

                GetClientRect(WindowHandle, &clientRect);

                hdc = GetDC(WindowHandle);
                bufferDc = CreateCompatibleDC(hdc);
                bufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

                {
                    RECT clientRect2 = clientRect;
                    TabCtrl_AdjustRect(WindowHandle, FALSE, &clientRect2);
                    ExcludeClipRect(hdc, clientRect2.left, clientRect2.top, clientRect2.right, clientRect2.bottom);
                }

                ThemeWindowRenderTabControl(context, WindowHandle, bufferDc, &clientRect, oldWndProc);

                BitBlt(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, bufferDc, 0, 0, SRCCOPY);
                SelectBitmap(bufferDc, oldBufferBitmap);
                DeleteBitmap(bufferBitmap);
                DeleteDC(bufferDc);
                ReleaseDC(WindowHandle, hdc);
            }

            //EndPaint(WindowHandle, &ps);
        }
        goto DefaultWndProc;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowListBoxControlSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_STATUSBAR_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
        case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhFree(context);
        }
        break;
    //case WM_MOUSEMOVE:
    //case WM_NCMOUSEMOVE:
    //    {
    //        POINT windowPoint;
    //        RECT windowRect;
    //
    //        GetCursorPos(&windowPoint);
    //        GetWindowRect(WindowHandle, &windowRect);
    //        context->Hot = PhPtInRect(&windowRect, windowPoint);
    //
    //        if (!context->HotTrack)
    //        {
    //            TRACKMOUSEEVENT trackMouseEvent;
    //            trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
    //            trackMouseEvent.dwFlags = TME_LEAVE;
    //            trackMouseEvent.hwndTrack = WindowHandle;
    //            trackMouseEvent.dwHoverTime = 0;
    //
    //            context->HotTrack = TRUE;
    //
    //            TrackMouseEvent(&trackMouseEvent);
    //        }
    //
    //        RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
    //    }
    //    break;
    //case WM_MOUSELEAVE:
    //case WM_NCMOUSELEAVE:
    //    {
    //        POINT windowPoint;
    //        RECT windowRect;
    //
    //        context->HotTrack = FALSE;
    //
    //        GetCursorPos(&windowPoint);
    //        GetWindowRect(WindowHandle, &windowRect);
    //        context->Hot = PhPtInRect(&windowRect, windowPoint);
    //
    //        RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
    //    }
    //    break;
    //case WM_CAPTURECHANGED:
    //    {
    //        POINT windowPoint;
    //        RECT windowRect;
    //
    //        GetCursorPos(&windowPoint);
    //        GetWindowRect(WindowHandle, &windowRect);
    //        context->Hot = PhPtInRect(&windowRect, windowPoint);
    //
    //        RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
    //    }
    //    break;
    case WM_NCCALCSIZE:
        {
            LPNCCALCSIZE_PARAMS ncCalcSize = (NCCALCSIZE_PARAMS*)lParam;

            CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);

            PhInflateRect(&ncCalcSize->rgrc[0], 1, 1);
        }
        return 0;
    case WM_NCPAINT:
        {
            HDC hdc;
            ULONG flags;
            RECT clientRect;
            RECT windowRect;
            HRGN updateRegion;
            LONG dpiValue = PhGetWindowDpi(WindowHandle);
            INT cxEdge = PhGetSystemMetrics(SM_CXEDGE, dpiValue);
            INT cyEdge = PhGetSystemMetrics(SM_CYEDGE, dpiValue);

            updateRegion = (HRGN)wParam;

            GetWindowRect(WindowHandle, &windowRect);

            // draw the scrollbar without the border.
            {
                HRGN rectregion = CreateRectRgn(
                    windowRect.left + cxEdge,
                    windowRect.top + cyEdge,
                    windowRect.right - cxEdge,
                    windowRect.bottom - cyEdge
                    );

                if (updateRegion != HRGN_FULL)
                    CombineRgn(rectregion, rectregion, updateRegion, RGN_AND);
                DefWindowProc(WindowHandle, WM_NCPAINT, (WPARAM)rectregion, 0);
                DeleteRgn(rectregion);
            }

            if (updateRegion == HRGN_FULL)
                updateRegion = NULL;

            flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | DCX_USESTYLE;

            if (updateRegion)
                flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

            if (hdc = GetDCEx(WindowHandle, updateRegion, flags))
            {
                PhOffsetRect(&windowRect, -windowRect.left, -windowRect.top);
                clientRect = windowRect;
                clientRect.left += cxEdge;
                clientRect.top += cyEdge;
                clientRect.right -= cxEdge;
                clientRect.bottom -= cyEdge;

                {
                    SCROLLBARINFO scrollInfo = { sizeof(SCROLLBARINFO) };

                    if (GetScrollBarInfo(WindowHandle, OBJID_VSCROLL, &scrollInfo))
                    {
                        if ((scrollInfo.rgstate[0] & STATE_SYSTEM_INVISIBLE) == 0)
                        {
                            clientRect.right -= PhGetSystemMetrics(SM_CXVSCROLL, dpiValue);
                        }
                    }
                }

                ExcludeClipRect(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

                if (context->Hot)
                {
                    SetDCBrushColor(hdc, PhThemeWindowHighlightColor);
                    FrameRect(hdc, &windowRect, PhGetStockBrush(DC_BRUSH));
                }
                else
                {
                    SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
                    FrameRect(hdc, &windowRect, GetSysColorBrush(COLOR_WINDOWFRAME));
                }

                ReleaseDC(WindowHandle, hdc);
                return TRUE;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

VOID ThemeWindowRenderComboBox(
    _In_ PPHP_THEME_WINDOW_COMBO_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect,
    _In_ WNDPROC WindowProcedure
    )
{
    RECT bufferRect =
    {
        0, 0,
       clientRect->right - clientRect->left,
       clientRect->bottom - clientRect->top
    };
    LONG_PTR windowStyle = PhGetWindowStyle(WindowHandle);
    //BOOLEAN isFocused = GetFocus() == WindowHandle;

    SetBkMode(bufferDc, TRANSPARENT);
    SelectFont(bufferDc, CallWindowProc(WindowProcedure, WindowHandle, WM_GETFONT, 0, 0));
    SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
    FillRect(bufferDc, clientRect, PhGetStockBrush(DC_BRUSH));

    if (PhPtInRect(clientRect, Context->CursorPos))
    {
        SetDCBrushColor(bufferDc, PhThemeWindowHighlight2Color); // RGB(0, 120, 212) : RGB(68, 68, 68));
    }
    else
    {
        SetDCBrushColor(bufferDc, GetSysColor(COLOR_WINDOWFRAME));// RGB(0, 120, 212) : RGB(68, 68, 68));
    }

    SetTextColor(bufferDc, PhThemeWindowTextColor);
    FrameRect(bufferDc, clientRect, PhGetStockBrush(DC_BRUSH));

    if (Context->ThemeHandle)
    {
        SIZE dropdownSize = { 0 };

        PhGetThemePartSize(
            Context->ThemeHandle,
            bufferDc,
            CP_DROPDOWNBUTTONRIGHT,
            CBXSR_NORMAL,
            NULL,
            TS_TRUE,
            &dropdownSize
            );

        bufferRect.left = clientRect->right - dropdownSize.cy;

        PhDrawThemeBackground(
            Context->ThemeHandle,
            bufferDc,
            CP_DROPDOWNBUTTONRIGHT,
            CBXSR_DISABLED,
            &bufferRect,
            NULL
            );

        bufferRect.left = 0;
    }
    else
    {
        DrawFrameControl(bufferDc, &bufferRect, DFC_SCROLL, DFCS_SCROLLDOWN);
    }

    if ((windowStyle & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST)
    {
        INT index = ComboBox_GetCurSel(WindowHandle);

        if (index == CB_ERR)
            return;

        INT length = ComboBox_GetLBTextLen(WindowHandle, index);

        if (length == CB_ERR)
            return;

        if (length < MAX_PATH)
        {
            WCHAR comboText[MAX_PATH] = L"";

            if (ComboBox_GetLBText(WindowHandle, index, comboText) == CB_ERR)
                return;

            bufferRect.left += 5;
            bufferRect.right -= 15; // info.rcItem.right
            DrawText(
                bufferDc,
                comboText,
                (UINT)PhCountStringZ(comboText),
                &bufferRect,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
                );
        }
    }
}

LRESULT CALLBACK PhpThemeWindowComboBoxControlSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_COMBO_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
            PhSetWindowProcedure(WindowHandle, oldWndProc);

            if (context->ThemeHandle)
            {
                PhCloseThemeData(context->ThemeHandle);
            }

            PhFree(context);
        }
        break;
    case WM_THEMECHANGED:
        {
            if (context->ThemeHandle)
            {
                PhCloseThemeData(context->ThemeHandle);
                context->ThemeHandle = NULL;
            }

            context->ThemeHandle = PhOpenThemeData(WindowHandle, VSCLASS_COMBOBOX, PhGetWindowDpi(WindowHandle));
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_MOUSEMOVE:
        {
            context->CursorPos.x = GET_X_LPARAM(lParam);
            context->CursorPos.y = GET_Y_LPARAM(lParam);
        }
        break;
    case WM_MOUSELEAVE:
        {
            context->CursorPos.x = LONG_MIN;
            context->CursorPos.y = LONG_MIN;
        }
        break;
    case WM_PAINT:
        {
            //PAINTSTRUCT ps;
            //HDC BufferedHDC;
            //HPAINTBUFFER BufferedPaint;
            //
            //if (!BeginPaint(WindowHandle, &ps))
            //    break;
            //
            //DEBUG_BEGINPAINT_RECT(WindowHandle, ps.rcPaint);
            //
            //if (BufferedPaint = BeginBufferedPaint(ps.hdc, &ps.rcPaint, BPBF_COMPATIBLEBITMAP, NULL, &BufferedHDC))
            //{
            //    ThemeWindowComboBoxExcludeRect(WindowHandle, ps.hdc, &ps.rcPaint, oldWndProc, context);
            //    ThemeWindowRenderComboBox(WindowHandle, BufferedHDC, &ps.rcPaint, oldWndProc, context);
            //    EndBufferedPaint(BufferedPaint, TRUE);
            //}
            //else
            {
                RECT clientRect;
                HDC hdc;
                HDC bufferDc;
                HBITMAP bufferBitmap;
                HBITMAP oldBufferBitmap;

                LONG_PTR windowStyle = PhGetWindowStyle(WindowHandle);
                GetClientRect(WindowHandle, &clientRect);
                hdc = GetDC(WindowHandle);

                if ((windowStyle & CBS_DROPDOWNLIST) != CBS_DROPDOWNLIST || (windowStyle & CBS_DROPDOWN) != CBS_DROPDOWN)
                {
                    COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };
                    RECT windowRect;
                    RECT listRect;

                    if (CallWindowProc(oldWndProc, WindowHandle, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info) && info.hwndList)
                    {
                        if ((windowStyle & CBS_SIMPLE) == CBS_SIMPLE)
                        {
                            GetWindowRect(WindowHandle, &windowRect);
                            GetWindowRect(info.hwndList, &listRect);
                            clientRect.bottom -= (windowRect.bottom - listRect.bottom);
                        }   

                        //INT borderSize = 0;
                        //if (Context->ThemeHandle)
                        //{
                        //    if (PhGetThemeInt(Context->ThemeHandle, 0, 0, TMT_BORDERSIZE, &borderSize))
                        //    {
                        //        borderSize = borderSize * 2;
                        //    }
                        //}

                        ExcludeClipRect(
                            hdc,
                            info.rcItem.left,
                            info.rcItem.top,
                            info.rcItem.right,
                            info.rcItem.bottom
                            );
                    }
                }

                bufferDc = CreateCompatibleDC(hdc);
                bufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

                ThemeWindowRenderComboBox(context, WindowHandle, bufferDc, &clientRect, oldWndProc);

                BitBlt(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, bufferDc, 0, 0, SRCCOPY);
                SelectBitmap(bufferDc, oldBufferBitmap);
                DeleteBitmap(bufferBitmap);
                DeleteDC(bufferDc);
                ReleaseDC(WindowHandle, hdc);
            }

            //EndPaint(WindowHandle, &ps);
        }
        goto DefaultWndProc;
    }

    // BUG BUG BUG fix 
    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowACLUISubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    if (!(oldWndProc = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
            PhSetWindowProcedure(WindowHandle, oldWndProc);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;

            if (data->code == NM_CUSTOMDRAW)
            {
                LPNMCUSTOMDRAW customDraw = (LPNMCUSTOMDRAW)lParam;
                WCHAR className[MAX_PATH];

                if (customDraw->dwDrawStage == CDDS_PREPAINT && !(customDraw->uItemState & CDIS_FOCUS))
                {
                    GETCLASSNAME_OR_NULL(customDraw->hdr.hwndFrom, className);

                    if (PhEqualStringZ(className, WC_BUTTON, FALSE))
                    {
                        HDC hdc = GetDC(WindowHandle);
                        RECT rectControl = customDraw->rc;
                        PhInflateRect(&rectControl, 2, 2);
                        MapWindowRect(customDraw->hdr.hwndFrom, WindowHandle, &rectControl);
                        FillRect(hdc, &rectControl, PhThemeWindowBackgroundBrush);   // fix the annoying white border left by the previous active control
                        ReleaseDC(WindowHandle, hdc);
                    }
                }
            }
        }
        break;
    case WM_VSCROLL:
    case WM_MOUSEWHEEL:
        InvalidateRect(WindowHandle, NULL, FALSE);
        break;
    case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT clientRect;

            GetClipBox(hdc, &clientRect);
            FillRect(hdc, &clientRect, PhThemeWindowBackgroundBrush);
        }
        return TRUE;
    case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, PhThemeWindowTextColor);
            return (INT_PTR)PhThemeWindowBackgroundBrush;
        }
    }
    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}
