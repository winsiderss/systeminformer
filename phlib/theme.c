/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2018-2022
 *
 */

#include <ph.h>
#include <guisup.h>
#include <emenu.h>
#include <settings.h>
#include <treenew.h>

#include <dwmapi.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

typedef struct _PHP_THEME_WINDOW_TAB_CONTEXT
{
    WNDPROC DefaultWindowProc;
    BOOLEAN MouseActive;
} PHP_THEME_WINDOW_TAB_CONTEXT, *PPHP_THEME_WINDOW_TAB_CONTEXT;

typedef struct _PHP_THEME_WINDOW_HEADER_CONTEXT
{
    WNDPROC DefaultWindowProc;
    BOOLEAN MouseActive;
} PHP_THEME_WINDOW_HEADER_CONTEXT, *PPHP_THEME_WINDOW_HEADER_CONTEXT;

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

    HTHEME StatusThemeData;
} PHP_THEME_WINDOW_STATUSBAR_CONTEXT, *PPHP_THEME_WINDOW_STATUSBAR_CONTEXT;

typedef struct _PHP_THEME_WINDOW_STATIC_CONTEXT
{
    WNDPROC DefaultWindowProc;
} PHP_THEME_WINDOW_STATIC_CONTEXT, *PPHP_THEME_WINDOW_STATIC_CONTEXT;

BOOLEAN CALLBACK PhpThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    );
BOOLEAN CALLBACK PhpReInitializeThemeWindowEnumChildWindows(
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
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );
LRESULT CALLBACK PhpThemeWindowEditSubclassProc(
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
LRESULT CALLBACK PhpThemeWindowHeaderSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );
LRESULT CALLBACK PhpThemeWindowStatusbarWndSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );
LRESULT CALLBACK PhpThemeWindowRebarToolbarSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhpThemeWindowStaticControlSubclassProc(
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

// Win10-RS5 (uxtheme.dll ordinal 132)
BOOL (WINAPI *ShouldAppsUseDarkMode_I)(
    VOID
    ) = NULL;
// Win10-RS5 (uxtheme.dll ordinal 138)
BOOL (WINAPI *ShouldSystemUseDarkMode_I)(
    VOID
    ) = NULL;
// Win10-RS5 (uxtheme.dll ordinal 133)
BOOL (WINAPI *AllowDarkModeForWindow_I)(
    _In_ HWND WindowHandle,
    _In_ BOOL Enabled
    ) = NULL;
// Win10-RS5 (uxtheme.dll ordinal 136)
BOOL (WINAPI* FlushMenuThemes_I)(
    VOID
    ) = NULL;

typedef enum _PreferredAppMode
{
    PreferredAppModeDisabled,
    PreferredAppModeDarkOnDark,
    PreferredAppModeDarkAlways
} PreferredAppMode;

// Win10-RS5 (uxtheme.dll ordinal 135)
// Win10 build 17763: AllowDarkModeForApp(BOOL)
// Win10 build 18334: SetPreferredAppMode(enum PreferredAppMode)
BOOL (WINAPI* SetPreferredAppMode_I)(
    _In_ PreferredAppMode AppMode
    ) = NULL;

// Win10-RS5 (uxtheme.dll ordinal 137)
BOOL (WINAPI *IsDarkModeAllowedForWindow_I)(
    _In_ HWND WindowHandle
    ) = NULL;

// Win10-RS5 (uxtheme.dll ordinal 139)
BOOL (WINAPI *IsDarkModeAllowedForApp_I)(
    _In_ HWND WindowHandle
    ) = NULL;

//HRESULT (WINAPI* DwmGetColorizationColor_I)(
//    _Out_ PULONG Colorization,
//    _Out_ PBOOL OpaqueBlend
//    );

ULONG PhpThemeColorMode = 0;
BOOLEAN PhpThemeEnable = FALSE;
BOOLEAN PhpThemeBorderEnable = TRUE;
HBRUSH PhMenuBackgroundBrush = NULL;
COLORREF PhThemeWindowForegroundColor = RGB(28, 28, 28);
COLORREF PhThemeWindowBackgroundColor = RGB(43, 43, 43);
COLORREF PhThemeWindowTextColor = RGB(0xff, 0xff, 0xff);
HFONT PhpListViewFontHandle = NULL;
HFONT PhpMenuFontHandle = NULL;
HFONT PhpGroupboxFontHandle = NULL;

VOID PhInitializeWindowTheme(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    PhpThemeColorMode = 1;// PhGetIntegerSetting(L"GraphColorMode");
    PhpThemeEnable = !!PhGetIntegerSetting(L"EnableThemeSupport");
    PhpThemeBorderEnable = !!PhGetIntegerSetting(L"TreeListBorderEnable");

    if (EnableThemeSupport && WindowsVersion >= WINDOWS_10_RS5)
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;

        if (PhBeginInitOnce(&initOnce))
        {
            PVOID module;

            if (WindowsVersion >= WINDOWS_10_19H2)
            {
                if (module = PhLoadLibrary(L"uxtheme.dll"))
                {
                    AllowDarkModeForWindow_I = PhGetDllBaseProcedureAddress(module, NULL, 133);
                    SetPreferredAppMode_I = PhGetDllBaseProcedureAddress(module, NULL, 135);
                    //FlushMenuThemes_I = PhGetDllBaseProcedureAddress(module, NULL, 136);
                }

                if (SetPreferredAppMode_I)
                {
                    switch (PhpThemeColorMode)
                    {
                    case 0: // New colors
                        SetPreferredAppMode_I(PreferredAppModeDisabled);
                        break;
                    case 1: // Old colors
                        SetPreferredAppMode_I(PreferredAppModeDarkAlways);
                        break;
                    }
                }

                //if (FlushMenuThemes_I)
                //    FlushMenuThemes_I();
            }

            PhEndInitOnce(&initOnce);
        }
    }

    PhInitializeThemeWindowFrame(WindowHandle);

    switch (PhpThemeColorMode)
    {
    case 0: // New colors
        {
            HBRUSH brush = PhMenuBackgroundBrush;
            PhMenuBackgroundBrush = CreateSolidBrush(PhThemeWindowTextColor);
            if (brush) DeleteBrush(brush);
        }
        break;
    case 1: // Old colors
        {
            HBRUSH brush = PhMenuBackgroundBrush;
            PhMenuBackgroundBrush = CreateSolidBrush(PhThemeWindowBackgroundColor);
            if (brush) DeleteBrush(brush);
        }
        break;
    }

    if (EnableThemeSupport)
    {
        WNDPROC defaultWindowProc;

        defaultWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);

        if (defaultWindowProc != PhpThemeWindowSubclassProc)
        {
            PhSetWindowContext(WindowHandle, LONG_MAX, defaultWindowProc);
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowSubclassProc);
        }

        PhEnumChildWindows(
            WindowHandle,
            0x1000,
            PhpThemeWindowEnumChildWindows,
            NULL
            );

        InvalidateRect(WindowHandle, NULL, FALSE); // HACK
    }
    else
    {
        //EnableThemeDialogTexture(WindowHandle, ETDT_ENABLETAB);
    }
}

VOID PhInitializeWindowThemeEx(
    _In_ HWND WindowHandle
    )
{
    static PH_STRINGREF keyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");
    HANDLE keyHandle;
    BOOLEAN enableThemeSupport = FALSE;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &keyPath,
        0
        )))
    {
        enableThemeSupport = !PhQueryRegistryUlong(keyHandle, L"AppsUseLightTheme");
        NtClose(keyHandle);
    }

    PhInitializeWindowTheme(WindowHandle, enableThemeSupport);
}

VOID PhReInitializeWindowTheme(
    _In_ HWND WindowHandle
    )
{
    HWND currentWindow = NULL;

    PhpThemeEnable = !!PhGetIntegerSetting(L"EnableThemeSupport");
    PhpThemeColorMode = 1;// PhGetIntegerSetting(L"GraphColorMode");
    PhpThemeBorderEnable = !!PhGetIntegerSetting(L"TreeListBorderEnable");

    PhInitializeThemeWindowFrame(WindowHandle);

    if (!PhpThemeEnable)
        return;

    switch (PhpThemeColorMode)
    {
    case 0: // New colors
        {
            HBRUSH brush = PhMenuBackgroundBrush;
            PhMenuBackgroundBrush = CreateSolidBrush(PhThemeWindowTextColor);
            if (brush) DeleteBrush(brush);
        }
        break;
    case 1: // Old colors
        {
            HBRUSH brush = PhMenuBackgroundBrush;
            PhMenuBackgroundBrush = CreateSolidBrush(PhThemeWindowBackgroundColor);
            if (brush) DeleteBrush(brush);
        }
        break;
    }

    PhEnumChildWindows(
        WindowHandle,
        0x1000,
        PhpReInitializeThemeWindowEnumChildWindows,
        NULL
        );

    do
    {
        if (currentWindow = FindWindowEx(NULL, currentWindow, NULL, NULL))
        {
            ULONG processID = 0;

            GetWindowThreadProcessId(currentWindow, &processID);

            if (UlongToHandle(processID) == NtCurrentProcessId())
            {
                WCHAR windowClassName[MAX_PATH];

                if (!GetClassName(currentWindow, windowClassName, RTL_NUMBER_OF(windowClassName)))
                    windowClassName[0] = UNICODE_NULL;

                //dprintf("PhReInitializeWindowTheme: %S\r\n", windowClassName);

                if (currentWindow != WindowHandle)
                {
                    if (PhEqualStringZ(windowClassName, L"#32770", FALSE))
                    {
                        PhEnumChildWindows(
                            currentWindow,
                            0x1000,
                            PhpReInitializeThemeWindowEnumChildWindows,
                            NULL
                            );
                        //PhReInitializeWindowTheme(currentWindow);
                    }

                    InvalidateRect(currentWindow, NULL, TRUE);
                }
            }
        }
    } while (currentWindow);

    InvalidateRect(WindowHandle, NULL, FALSE);
}

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
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

    if (WindowsVersion >= WINDOWS_10_RS5)
    {
        if (PhpThemeEnable)
        {
            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                {
                    if (FAILED(PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &(BOOL){ FALSE }, sizeof(BOOL))))
                    {
                        PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &(BOOL){ FALSE }, sizeof(BOOL));
                    }

                    //if (WindowsVersion > WINDOWS_11)
                    //{
                    //    PhSetWindowThemeAttribute(WindowHandle, DWMWA_CAPTION_COLOR, NULL, 0);
                    //}
                }
                break;
            case 1: // Old colors
                {
                    if (FAILED(PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &(BOOL){ TRUE }, sizeof(BOOL))))
                    {
                        PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &(BOOL){ TRUE }, sizeof(BOOL));
                    }

                    if (WindowsVersion >= WINDOWS_11)
                    {
                        PhSetWindowThemeAttribute(WindowHandle, DWMWA_CAPTION_COLOR, &PhThemeWindowBackgroundColor, sizeof(COLORREF));
                    }
                }
                break;
            }
        }

        if (WindowsVersion >= WINDOWS_11_22H1)
        {
            PhSetWindowThemeAttribute(WindowHandle, DWMWA_SYSTEMBACKDROP_TYPE, &(ULONG){ 1 }, sizeof(ULONG));
        }
    }
}

HBRUSH PhWindowThemeControlColor(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ HWND ChildWindowHandle,
    _In_ INT Type
    )
{
    SetBkMode(Hdc, TRANSPARENT);

    switch (Type)
    {
    case CTLCOLOR_EDIT:
        {
            if (PhpThemeEnable)
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetTextColor(Hdc, RGB(0x0, 0x0, 0x0));
                    SetDCBrushColor(Hdc, PhThemeWindowTextColor);
                    break;
                case 1: // Old colors
                    SetTextColor(Hdc, PhThemeWindowTextColor);
                    SetDCBrushColor(Hdc, RGB(60, 60, 60));
                    break;
                }
            }
            else
            {
                SetTextColor(Hdc, GetSysColor(COLOR_WINDOWTEXT));
                SetDCBrushColor(Hdc, GetSysColor(COLOR_WINDOW));
            }

            return GetStockBrush(DC_BRUSH);
        }
        break;
    case CTLCOLOR_SCROLLBAR:
        {
            if (PhpThemeEnable)
            {
                SetDCBrushColor(Hdc, RGB(23, 23, 23));
            }
            else
            {
                SetDCBrushColor(Hdc, GetSysColor(COLOR_SCROLLBAR));
            }

            return GetStockBrush(DC_BRUSH);
        }
        break;
    case CTLCOLOR_MSGBOX:
    case CTLCOLOR_LISTBOX:
    case CTLCOLOR_BTN:
    case CTLCOLOR_DLG:
    case CTLCOLOR_STATIC:
        {
            if (PhpThemeEnable)
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetTextColor(Hdc, RGB(0x00, 0x00, 0x00));
                    SetDCBrushColor(Hdc, PhThemeWindowTextColor);
                    break;
                case 1: // Old colors
                    SetTextColor(Hdc, PhThemeWindowTextColor);
                    SetDCBrushColor(Hdc, PhThemeWindowBackgroundColor);
                    break;
                }
            }
            else
            {
                SetTextColor(Hdc, GetSysColor(COLOR_WINDOWTEXT));
                SetDCBrushColor(Hdc, GetSysColor(COLOR_WINDOW));
            }

            return GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return NULL;
}

VOID PhInitializeThemeWindowHeader(
    _In_ HWND HeaderWindow
    )
{
    PPHP_THEME_WINDOW_HEADER_CONTEXT context;

    //if (PhGetWindowContext(HeaderWindow, LONG_MAX))
    //    return;

    if (PhGetWindowContext(HeaderWindow, 0xF))
    {
        HANDLE parentWindow;
        LONG_PTR windowStyle;

        if (parentWindow = GetParent(HeaderWindow))
        {
            windowStyle = PhGetWindowStyle(parentWindow);

            if (windowStyle & TN_STYLE_CUSTOM_HEADERDRAW)
                return;
        }
    }

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_HEADER_CONTEXT));
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(HeaderWindow, GWLP_WNDPROC);

    PhSetWindowContext(HeaderWindow, LONG_MAX, context);
    SetWindowLongPtr(HeaderWindow, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowHeaderSubclassProc);

    InvalidateRect(HeaderWindow, NULL, FALSE);
}

VOID PhInitializeThemeWindowTabControl(
    _In_ HWND TabControlWindow
    )
{
    PPHP_THEME_WINDOW_TAB_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_TAB_CONTEXT));
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(TabControlWindow, GWLP_WNDPROC);

    PhSetWindowContext(TabControlWindow, LONG_MAX, context);
    SetWindowLongPtr(TabControlWindow, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowTabControlWndSubclassProc);

    PhSetWindowStyle(TabControlWindow, TCS_OWNERDRAWFIXED, TCS_OWNERDRAWFIXED);

    InvalidateRect(TabControlWindow, NULL, FALSE);
}

VOID PhInitializeWindowThemeStatusBar(
    _In_ HWND StatusBarHandle
    )
{
    PPHP_THEME_WINDOW_STATUSBAR_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_STATUSBAR_CONTEXT));
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(StatusBarHandle, GWLP_WNDPROC);
    context->StatusThemeData = OpenThemeData(StatusBarHandle, VSCLASS_STATUS);

    PhSetWindowContext(StatusBarHandle, LONG_MAX, context);
    SetWindowLongPtr(StatusBarHandle, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowStatusbarWndSubclassProc);

    InvalidateRect(StatusBarHandle, NULL, FALSE);
}

VOID PhInitializeThemeWindowGroupBox(
    _In_ HWND GroupBoxHandle
    )
{
    WNDPROC groupboxWindowProc;

    groupboxWindowProc = (WNDPROC)GetWindowLongPtr(GroupBoxHandle, GWLP_WNDPROC);
    PhSetWindowContext(GroupBoxHandle, LONG_MAX, groupboxWindowProc);
    SetWindowLongPtr(GroupBoxHandle, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowGroupBoxSubclassProc);

    InvalidateRect(GroupBoxHandle, NULL, FALSE);
}

VOID PhInitializeThemeWindowEditControl(
    _In_ HWND EditControlHandle
    )
{
    WNDPROC editControlWindowProc;

    // HACK: The searchbox control does its own themed drawing and it uses the
    // same window context value so we know when to ignore theming.
    if (PhGetWindowContext(EditControlHandle, SHRT_MAX))
        return;

    editControlWindowProc = (WNDPROC)GetWindowLongPtr(EditControlHandle, GWLP_WNDPROC);
    PhSetWindowContext(EditControlHandle, LONG_MAX, editControlWindowProc);
    SetWindowLongPtr(EditControlHandle, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowEditSubclassProc);

    InvalidateRect(EditControlHandle, NULL, FALSE);
    //SetWindowPos(EditControlHandle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME);
}

VOID PhInitializeWindowThemeRebar(
    _In_ HWND HeaderWindow
    )
{
    PPHP_THEME_WINDOW_HEADER_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_HEADER_CONTEXT));
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(HeaderWindow, GWLP_WNDPROC);

    PhSetWindowContext(HeaderWindow, LONG_MAX, context);
    SetWindowLongPtr(HeaderWindow, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowRebarToolbarSubclassProc);

    InvalidateRect(HeaderWindow, NULL, FALSE);
}

VOID PhInitializeWindowThemeMainMenu(
    _In_ HMENU MenuHandle
    )
{
    static HBRUSH themeBrush = NULL;
    MENUINFO menuInfo;

    if (!themeBrush)
        themeBrush = CreateSolidBrush(PhThemeWindowBackgroundColor);

    memset(&menuInfo, 0, sizeof(MENUINFO));
    menuInfo.cbSize = sizeof(MENUINFO);
    menuInfo.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
    menuInfo.hbrBack = themeBrush;

    SetMenuInfo(MenuHandle, &menuInfo);
}

VOID PhInitializeWindowThemeStaticControl(
    _In_ HWND StaticControl
    )
{
    PPHP_THEME_WINDOW_STATIC_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_STATIC_CONTEXT));
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(StaticControl, GWLP_WNDPROC);

    PhSetWindowContext(StaticControl, LONG_MAX, context);
    SetWindowLongPtr(StaticControl, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowStaticControlSubclassProc);

    InvalidateRect(StaticControl, NULL, FALSE);
}

VOID PhInitializeWindowThemeListboxControl(
    _In_ HWND ListBoxControl
    )
{
    PPHP_THEME_WINDOW_STATUSBAR_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_STATUSBAR_CONTEXT));
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(ListBoxControl, GWLP_WNDPROC);

    PhSetWindowContext(ListBoxControl, LONG_MAX, context);
    SetWindowLongPtr(ListBoxControl, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowListBoxControlSubclassProc);

    InvalidateRect(ListBoxControl, NULL, FALSE);
    SetWindowPos(ListBoxControl, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

VOID PhInitializeWindowThemeComboboxControl(
    _In_ HWND ComboBoxControl
    )
{
    PhSetWindowContext(ComboBoxControl, LONG_MAX, (PVOID)GetWindowLongPtr(ComboBoxControl, GWLP_WNDPROC));
    SetWindowLongPtr(ComboBoxControl, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowComboBoxControlSubclassProc);

    InvalidateRect(ComboBoxControl, NULL, FALSE);
}

VOID PhInitializeWindowThemeACLUI(
    _In_ HWND ACLUIControl
)
{
    PhSetWindowContext(ACLUIControl, LONG_MAX, (PVOID)GetWindowLongPtr(ACLUIControl, GWLP_WNDPROC));
    SetWindowLongPtr(ACLUIControl, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowACLUISubclassProc);

    InvalidateRect(ACLUIControl, NULL, FALSE);
}

BOOLEAN CALLBACK PhpThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    WCHAR windowClassName[MAX_PATH];

    PhEnumChildWindows(
        WindowHandle,
        0x1000,
        PhpThemeWindowEnumChildWindows,
        NULL
        );

    if (PhGetWindowContext(WindowHandle, LONG_MAX)) // HACK
        return TRUE;

    if (!GetClassName(WindowHandle, windowClassName, RTL_NUMBER_OF(windowClassName)))
        windowClassName[0] = UNICODE_NULL;

    //dprintf("PhpThemeWindowEnumChildWindows: %S\r\n", windowClassName);

    if (PhEqualStringZ(windowClassName, L"#32770", TRUE))
    {
        PhInitializeWindowTheme(WindowHandle, TRUE);
    }
    else if (PhEqualStringZ(windowClassName, WC_BUTTON, FALSE))
    {
        if ((PhGetWindowStyle(WindowHandle) & BS_GROUPBOX) == BS_GROUPBOX)
        {
            PhInitializeThemeWindowGroupBox(WindowHandle);
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_TABCONTROL, FALSE))
    {
        PhInitializeThemeWindowTabControl(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, STATUSCLASSNAME, FALSE))
    {
        PhInitializeWindowThemeStatusBar(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, WC_EDIT, TRUE))
    {
        PhInitializeThemeWindowEditControl(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, WC_SCROLLBAR, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                PhSetControlTheme(WindowHandle, L"");
                break;
            case 1: // Old colors
                PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
                break;
            }
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_HEADER, TRUE))
    {
        PhInitializeThemeWindowHeader(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, WC_LISTVIEW, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND tooltipWindow = ListView_GetToolTips(WindowHandle);

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                PhSetControlTheme(WindowHandle, L"explorer");
                PhSetControlTheme(tooltipWindow, L"");
                break;
            case 1: // Old colors
                PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
                PhSetControlTheme(tooltipWindow, L"DarkMode_Explorer");
                break;
            }
        }

        if (PhpThemeBorderEnable)
        {
            PhSetWindowStyle(WindowHandle, WS_BORDER, WS_BORDER);
            PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
        }
        else
        {
            PhSetWindowStyle(WindowHandle, WS_BORDER, 0);
            PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, 0);
        }

        SetWindowPos(WindowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        switch (PhpThemeColorMode)
        {
        case 0: // New colors
            ListView_SetBkColor(WindowHandle, RGB(0xff, 0xff, 0xff));
            ListView_SetTextBkColor(WindowHandle, RGB(0xff, 0xff, 0xff));
            ListView_SetTextColor(WindowHandle, RGB(0x0, 0x0, 0x0));
            break;
        case 1: // Old colors
            ListView_SetBkColor(WindowHandle, PhThemeWindowBackgroundColor); // RGB(30, 30, 30)
            ListView_SetTextBkColor(WindowHandle, PhThemeWindowBackgroundColor); // RGB(30, 30, 30)
            ListView_SetTextColor(WindowHandle, PhThemeWindowTextColor);
            break;
        }

        //InvalidateRect(WindowHandle, NULL, FALSE);
    }
    else if (PhEqualStringZ(windowClassName, WC_TREEVIEW, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND tooltipWindow = TreeView_GetToolTips(WindowHandle);

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                PhSetControlTheme(WindowHandle, L"explorer");
                PhSetControlTheme(tooltipWindow, L"");
                break;
            case 1: // Old colors
                PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
                PhSetControlTheme(tooltipWindow, L"DarkMode_Explorer");
                break;
            }
        }

        switch (PhpThemeColorMode)
        {
        case 0: // New colors
            break;
        case 1: // Old colors
            TreeView_SetBkColor(WindowHandle, PhThemeWindowBackgroundColor);// RGB(30, 30, 30));
            //TreeView_SetTextBkColor(WindowHandle, RGB(30, 30, 30));
            TreeView_SetTextColor(WindowHandle, PhThemeWindowTextColor);
            break;
        }

        //InvalidateRect(WindowHandle, NULL, FALSE);
    }
    else if (PhEqualStringZ(windowClassName, L"RICHEDIT50W", FALSE))
    {
        if (PhpThemeBorderEnable)
            PhSetWindowStyle(WindowHandle, WS_BORDER, WS_BORDER);
        else
            PhSetWindowStyle(WindowHandle, WS_BORDER, 0);

        SetWindowPos(WindowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        #define EM_SETBKGNDCOLOR (WM_USER + 67)
        switch (PhpThemeColorMode)
        {
        case 0: // New colors
            SendMessage(WindowHandle, EM_SETBKGNDCOLOR, 0, RGB(0xff, 0xff, 0xff));
            break;
        case 1: // Old colors
            SendMessage(WindowHandle, EM_SETBKGNDCOLOR, 0, RGB(30, 30, 30));
            break;
        }

        PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");

        //InvalidateRect(WindowHandle, NULL, FALSE);
    }
    else if (PhEqualStringZ(windowClassName, L"PhTreeNew", FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND tooltipWindow = TreeNew_GetTooltips(WindowHandle);

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                PhSetControlTheme(tooltipWindow, L"");
                PhSetControlTheme(WindowHandle, L"");
                break;
            case 1: // Old colors
                PhSetControlTheme(tooltipWindow, L"DarkMode_Explorer");
                PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
                break;
            }
        }

        if (PhpThemeBorderEnable)
            PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
        else
            PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, 0);

        SetWindowPos(WindowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        switch (PhpThemeColorMode)
        {
        case 0: // New colors
            TreeNew_ThemeSupport(WindowHandle, FALSE);
            break;
        case 1: // Old colors
            TreeNew_ThemeSupport(WindowHandle, TRUE);
            break;
        }

        //InvalidateRect(WindowHandle, NULL, TRUE);
    }
    else if (
        PhEqualStringZ(windowClassName, WC_LISTBOX, FALSE) ||
        PhEqualStringZ(windowClassName, L"ComboLBox", FALSE)
        )
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                PhSetControlTheme(WindowHandle, L"explorer");
                break;
            case 1: // Old colors
                PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
                break;
            }
        }

        PhInitializeWindowThemeListboxControl(WindowHandle);
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
                PhSetControlTheme(info.hwndList, L"DarkMode_Explorer");
            }
        }

        //if ((PhGetWindowStyle(WindowHandle) & CBS_DROPDOWNLIST) != CBS_DROPDOWNLIST)
        {
            PhInitializeWindowThemeComboboxControl(WindowHandle);
        }
    }
    else if (PhEqualStringZ(windowClassName, L"CHECKLIST_ACLUI", FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                PhSetControlTheme(WindowHandle, L"explorer");
                break;
            case 1: // Old colors
                PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
                break;
            }
        }

        PhInitializeWindowThemeACLUI(WindowHandle);
    }

    return TRUE;
}

BOOLEAN CALLBACK PhpReInitializeThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    WCHAR windowClassName[MAX_PATH];

    PhEnumChildWindows(
        WindowHandle,
        0x1000,
        PhpReInitializeThemeWindowEnumChildWindows,
        NULL
        );

    if (!GetClassName(WindowHandle, windowClassName, RTL_NUMBER_OF(windowClassName)))
        windowClassName[0] = UNICODE_NULL;

    //dprintf("PhpReInitializeThemeWindowEnumChildWindows: %S\r\n", windowClassName);

    if (PhEqualStringZ(windowClassName, WC_LISTVIEW, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                PhSetControlTheme(WindowHandle, L"explorer");
                break;
            case 1: // Old colors
                PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
                break;
            }
        }

        switch (PhpThemeColorMode)
        {
        case 0: // New colors
            ListView_SetBkColor(WindowHandle, RGB(0xff, 0xff, 0xff));
            ListView_SetTextBkColor(WindowHandle, RGB(0xff, 0xff, 0xff));
            ListView_SetTextColor(WindowHandle, RGB(0x0, 0x0, 0x0));
            break;
        case 1: // Old colors
            ListView_SetBkColor(WindowHandle, PhThemeWindowBackgroundColor); // RGB(30, 30, 30)
            ListView_SetTextBkColor(WindowHandle, PhThemeWindowBackgroundColor); // RGB(30, 30, 30)
            ListView_SetTextColor(WindowHandle, PhThemeWindowTextColor);
            break;
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_SCROLLBAR, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                PhSetControlTheme(WindowHandle, L"");
                break;
            case 1: // Old colors
                PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
                break;
            }
        }
    }
    else if (PhEqualStringZ(windowClassName, L"PhTreeNew", FALSE))
    {
        switch (PhpThemeColorMode)
        {
        case 0: // New colors
            TreeNew_ThemeSupport(WindowHandle, FALSE);
            PhSetControlTheme(WindowHandle, L"");
            break;
        case 1: // Old colors
            TreeNew_ThemeSupport(WindowHandle, TRUE);
            PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
            break;
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_EDIT, FALSE))
    {
        SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0); // searchbox.c
    }

    InvalidateRect(WindowHandle, NULL, TRUE);

    return TRUE;
}

BOOLEAN PhThemeWindowDrawItem(
    _In_ PDRAWITEMSTRUCT DrawInfo
    )
{
    LONG dpiValue = PhGetWindowDpi(DrawInfo->hwndItem);
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
    case ODT_STATIC:
        break;
    case ODT_HEADER:
        break;
    case ODT_MENU:
        {
            PPH_EMENU_ITEM menuItemInfo = (PPH_EMENU_ITEM)DrawInfo->itemData;
            RECT rect = DrawInfo->rcItem;
            ULONG drawTextFlags = DT_SINGLELINE | DT_NOCLIP;
            HFONT oldFont = NULL;

            if (DrawInfo->itemState & ODS_NOACCEL)
            {
                drawTextFlags |= DT_HIDEPREFIX;
            }

            if (PhpMenuFontHandle)
            {
                oldFont = SelectFont(DrawInfo->hDC, PhpMenuFontHandle);
            }

            //FillRect(
            //    DrawInfo->hDC,
            //    &DrawInfo->rcItem,
            //    CreateSolidBrush(RGB(0, 0, 0))
            //    );
            //SetTextColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));

            if (DrawInfo->itemState & ODS_HOTLIGHT)
            {
                SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->hDC, RGB(128, 128, 128));
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));
            }
            else if (isDisabled)
            {
                SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_GRAYTEXT));
                SetDCBrushColor(DrawInfo->hDC, PhThemeWindowBackgroundColor);
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));
            }
            else if (isSelected)
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->hDC, PhThemeWindowBackgroundColor);
                    break;
                case 1: // Old colors
                    SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->hDC, RGB(128, 128, 128));
                    break;
                }

                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));
            }
            else
            {
                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_WINDOWTEXT));
                //    SetDCBrushColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));
                //    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));
                //    break;

                SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetDCBrushColor(DrawInfo->hDC, PhThemeWindowBackgroundColor);
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));
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

                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    oldTextColor = SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_WINDOWTEXT));
                    break;
                case 1: // Old colors
                    oldTextColor = SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);
                    break;
                }

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
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetDCBrushColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));
                    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));
                    break;
                case 1: // Old colors
                    SetDCBrushColor(DrawInfo->hDC, PhThemeWindowBackgroundColor); // RGB(28, 28, 28)
                    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));
                    break;
                }

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
                SelectBrush(DrawInfo->hDC, GetStockBrush(DC_BRUSH));
                PatBlt(DrawInfo->hDC, DrawInfo->rcItem.left, DrawInfo->rcItem.top + cyEdge, DrawInfo->rcItem.right - DrawInfo->rcItem.left, 1, PATCOPY);

                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    SetDCBrushColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));
                //    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));
                //    break;
                //case 1: // Old colors
                //    SetDCBrushColor(DrawInfo->hDC, RGB(78, 78, 78));
                //    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));
                //    break;
                //}

                //DrawEdge(drawInfo->hDC, &drawInfo->rcItem, BDR_RAISEDINNER, BF_TOP);
            }
            else
            {
                PH_STRINGREF part;
                PH_STRINGREF firstPart;
                PH_STRINGREF secondPart;

                PhInitializeStringRefLongHint(&part, menuItemInfo->Text);
                PhSplitStringRefAtLastChar(&part, L'\b', &firstPart, &secondPart);

                //SetDCBrushColor(DrawInfo->hDC, RGB(28, 28, 28));
                //FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));

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
                    DrawText(
                        DrawInfo->hDC,
                        firstPart.Buffer,
                        (UINT)firstPart.Length / sizeof(WCHAR),
                        &DrawInfo->rcItem,
                        DT_LEFT | DT_SINGLELINE | DT_CENTER | DT_VCENTER | drawTextFlags
                        );
                }
                else
                {
                    DrawText(
                        DrawInfo->hDC,
                        firstPart.Buffer,
                        (UINT)firstPart.Length / sizeof(WCHAR),
                        &DrawInfo->rcItem,
                        DT_LEFT | DT_VCENTER | drawTextFlags
                        );
                }


                DrawText(
                    DrawInfo->hDC,
                    secondPart.Buffer,
                    (UINT)secondPart.Length / sizeof(WCHAR),
                    &DrawInfo->rcItem,
                    DT_RIGHT | DT_VCENTER | drawTextFlags
                    );
            }

            if (oldFont)
            {
                SelectFont(DrawInfo->hDC, oldFont);
            }

            if (menuItemInfo->Items && menuItemInfo->Items->Count && (menuItemInfo->Flags & PH_EMENU_MAINMENU) != PH_EMENU_MAINMENU)
            {
                static HTHEME menuThemeHandle = NULL;

                if (!menuThemeHandle)
                {
                    menuThemeHandle = OpenThemeData(NULL, L"Menu");
                }

                rect.left = rect.right - PhGetDpi(25, dpiValue);

                DrawThemeBackground(menuThemeHandle, DrawInfo->hDC, MENU_POPUPSUBMENU, isDisabled ? MSM_DISABLED : MSM_NORMAL, &rect, NULL);
            }

            ExcludeClipRect(DrawInfo->hDC, rect.left, rect.top, rect.right, rect.bottom);

            return TRUE;
        }
    case ODT_COMBOBOX:
        {
            WCHAR comboText[MAX_PATH];

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_WINDOWTEXT));
                SetDCBrushColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));
                break;
            case 1: // Old colors
                SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetDCBrushColor(DrawInfo->hDC, RGB(28, 28, 28));
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockBrush(DC_BRUSH));
                break;
            }

            if (ComboBox_GetLBText(DrawInfo->hwndItem, DrawInfo->itemID, (LPARAM)comboText) != CB_ERR)
            {
                DrawText(
                    DrawInfo->hDC,
                    comboText,
                    (UINT)PhCountStringZ(comboText),
                    &DrawInfo->rcItem,
                    DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE
                    );
            }
            return TRUE;
        }
        break;
    }

    return FALSE;
}

BOOLEAN PhThemeWindowMeasureItem(
    _In_ HWND WindowHandle,
    _In_ PMEASUREITEMSTRUCT DrawInfo
    )
{
    LONG dpiValue = PhGetWindowDpi(WindowHandle);

    if (DrawInfo->CtlType == ODT_MENU)
    {
        PPH_EMENU_ITEM menuItemInfo = (PPH_EMENU_ITEM)DrawInfo->itemData;

        DrawInfo->itemWidth = PhGetDpi(150, dpiValue);
        DrawInfo->itemHeight = PhGetDpi(100, dpiValue);

        if ((menuItemInfo->Flags & PH_EMENU_SEPARATOR) == PH_EMENU_SEPARATOR)
        {
            DrawInfo->itemHeight = PhGetSystemMetrics(SM_CYMENU, dpiValue) >> 2;
        }
        else if (menuItemInfo->Text)
        {
            HDC hdc;

            if (!PhpMenuFontHandle)
            {
                NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

                if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, dpiValue))
                {
                    PhpMenuFontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }

            if (hdc = GetDC(WindowHandle))
            {
                PWSTR text;
                SIZE_T textCount;
                SIZE textSize;
                HFONT oldFont = NULL;
                INT cyborder = PhGetSystemMetrics(SM_CYBORDER, dpiValue);
                INT cymenu = PhGetSystemMetrics(SM_CYMENU, dpiValue);

                text = menuItemInfo->Text;
                textCount = PhCountStringZ(text);

                if (PhpMenuFontHandle)
                {
                    oldFont = SelectFont(hdc, PhpMenuFontHandle);
                }

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

                if (oldFont)
                {
                    SelectFont(hdc, oldFont);
                }

                ReleaseDC(WindowHandle, hdc);
            }
        }

        return TRUE;
    }

    return FALSE;
}

LRESULT CALLBACK PhpThemeWindowDrawButton(
    _In_ LPNMCUSTOMDRAW DrawInfo
    )
{
    BOOLEAN isGrayed = (DrawInfo->uItemState & CDIS_GRAYED) == CDIS_GRAYED;
    BOOLEAN isChecked = (DrawInfo->uItemState & CDIS_CHECKED) == CDIS_CHECKED;
    BOOLEAN isDisabled = (DrawInfo->uItemState & CDIS_DISABLED) == CDIS_DISABLED;
    BOOLEAN isSelected = (DrawInfo->uItemState & CDIS_SELECTED) == CDIS_SELECTED;
    BOOLEAN isHighlighted = (DrawInfo->uItemState & CDIS_HOT) == CDIS_HOT;
    BOOLEAN isFocused = (DrawInfo->uItemState & CDIS_FOCUS) == CDIS_FOCUS;

    switch (DrawInfo->dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            PPH_STRING buttonText;
            LONG_PTR buttonStyle;
            HFONT oldFont;
            LONG dpiValue;

            buttonText = PhGetWindowText(DrawInfo->hdr.hwndFrom);
            buttonStyle = PhGetWindowStyle(DrawInfo->hdr.hwndFrom);

            if (isSelected)
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    //SetTextColor(DrawInfo->hdc, RGB(0, 0, 0xff));
                    SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHT));
                    break;
                case 1: // Old colors
                    SetTextColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    SetDCBrushColor(DrawInfo->hdc, RGB(78, 78, 78));
                    break;
                }
                FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockBrush(DC_BRUSH));
            }
            else if (isHighlighted)
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    //SetTextColor(DrawInfo->hdc, RGB(0, 0, 0xff));
                    SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHT));
                    break;
                case 1: // Old colors
                    SetTextColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    SetDCBrushColor(DrawInfo->hdc, RGB(65, 65, 65));
                    break;
                }
                FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockBrush(DC_BRUSH));
            }
            else
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    //SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->hdc, PhThemeWindowTextColor);
                    break;
                case 1: // Old colors
                    SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->hdc, RGB(42, 42, 42)); // WindowForegroundColor
                    break;
                }
                FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockBrush(DC_BRUSH));
            }

            dpiValue = PhGetWindowDpi(DrawInfo->hdr.hwndFrom);

            if ((buttonStyle & BS_ICON) == BS_ICON)
            {
                RECT bufferRect =
                {
                    0, 0,
                    DrawInfo->rc.right - DrawInfo->rc.left,
                    DrawInfo->rc.bottom - DrawInfo->rc.top
                };
                HICON buttonIcon;

                SetDCBrushColor(DrawInfo->hdc, RGB(65, 65, 65));
                FrameRect(DrawInfo->hdc, &DrawInfo->rc, GetStockBrush(DC_BRUSH));

                if (!(buttonIcon = Static_GetIcon(DrawInfo->hdr.hwndFrom, 0)))
                    buttonIcon = (HICON)SendMessage(DrawInfo->hdr.hwndFrom, BM_GETIMAGE, IMAGE_ICON, 0);

                if (buttonIcon)
                {
                    ICONINFO iconInfo;
                    BITMAP bmp;
                    LONG width = PhGetSystemMetrics(SM_CXSMICON, dpiValue);
                    LONG height = PhGetSystemMetrics(SM_CYSMICON, dpiValue);

                    memset(&iconInfo, 0, sizeof(ICONINFO));
                    memset(&bmp, 0, sizeof(BITMAP));

                    GetIconInfo(buttonIcon, &iconInfo);

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

                    bufferRect.left += PhGetDpi(1, dpiValue); // HACK
                    bufferRect.top += PhGetDpi(1, dpiValue); // HACK

                    DrawIconEx(
                        DrawInfo->hdc,
                        bufferRect.left + ((bufferRect.right - bufferRect.left) - width) / 2,
                        bufferRect.top + ((bufferRect.bottom - bufferRect.top) - height) / 2,
                        buttonIcon,
                        width,
                        height,
                        0,
                        NULL,
                        DI_NORMAL
                        );
                }
            }
            else if ((buttonStyle & BS_CHECKBOX) == BS_CHECKBOX)
            {
                if (Button_GetCheck(DrawInfo->hdr.hwndFrom) == BST_CHECKED)
                {
                    HFONT newFont = PhDuplicateFontWithNewHeight(PhApplicationFont, 16, dpiValue);

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
                    HFONT newFont = PhDuplicateFontWithNewHeight(PhApplicationFont, 22, dpiValue);

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

                DrawInfo->rc.left += PhGetDpi(10, dpiValue);
                DrawText(
                    DrawInfo->hdc,
                    buttonText->Buffer,
                    (UINT)buttonText->Length / sizeof(WCHAR),
                    &DrawInfo->rc,
                    DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_HIDEPREFIX
                    );
                DrawInfo->rc.left -= PhGetDpi(10, dpiValue);
            }
            else
            {
                RECT bufferRect =
                {
                    0, 0,
                    DrawInfo->rc.right - DrawInfo->rc.left,
                    DrawInfo->rc.bottom - DrawInfo->rc.top
                };
                HICON buttonIcon;
                INT width;
                INT height;

                SetDCBrushColor(DrawInfo->hdc, RGB(65, 65, 65));
                FrameRect(DrawInfo->hdc, &DrawInfo->rc, GetStockBrush(DC_BRUSH));

                if (!(buttonIcon = Static_GetIcon(DrawInfo->hdr.hwndFrom, 0)))
                    buttonIcon = (HICON)SendMessage(DrawInfo->hdr.hwndFrom, BM_GETIMAGE, IMAGE_ICON, 0);

                if (buttonIcon)
                {
                    width = PhGetSystemMetrics(SM_CXSMICON, dpiValue);
                    height = PhGetSystemMetrics(SM_CYSMICON, dpiValue);

                    DrawIconEx(
                        DrawInfo->hdc,
                        bufferRect.left + ((bufferRect.right - bufferRect.left) - width) / 2,
                        bufferRect.top + ((bufferRect.bottom - bufferRect.top) - height) / 2,
                        buttonIcon,
                        width,
                        height,
                        0,
                        NULL,
                        DI_NORMAL
                        );
                }

                DrawText(
                    DrawInfo->hdc,
                    buttonText->Buffer,
                    (UINT)buttonText->Length / sizeof(WCHAR),
                    &DrawInfo->rc,
                    DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_HIDEPREFIX
                    );
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
        return CDRF_DODEFAULT;

    switch (DrawInfo->dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            //REBARBANDINFO bandinfo = { sizeof(REBARBANDINFO), RBBIM_CHILD | RBBIM_STYLE | RBBIM_CHEVRONLOCATION | RBBIM_CHEVRONSTATE };
            //BOOL havebandinfo = (BOOL)SendMessage(DrawInfo->hdr.hwndFrom, RB_GETBANDINFO, DrawInfo->dwItemSpec, (LPARAM)&bandinfo);

            SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor);
            SetDCBrushColor(DrawInfo->hdc, PhThemeWindowBackgroundColor);
            FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockBrush(DC_BRUSH));

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                {
                    //SetTextColor(DrawInfo->hdc, RGB(0x0, 0x0, 0x0));
                    //SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_3DFACE)); // RGB(0xff, 0xff, 0xff));
                }
                break;
            case 1: // Old colors
                {
                    //SetTextColor(DrawInfo->hdc, RGB(0xff, 0xff, 0xff));
                    //SetDCBrushColor(DrawInfo->hdc, RGB(65, 65, 65));
                }
                break;
            }
        }
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
        {
            /*SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor);
            SetDCBrushColor(DrawInfo->hdc, PhThemeWindowBackgroundColor);
            FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockBrush(DC_BRUSH));*/

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                {
                    //SetTextColor(DrawInfo->hdc, RGB(0x0, 0x0, 0x0));
                    //SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_3DFACE)); // RGB(0x0, 0x0, 0x0));
                }
            case 1: // Old colors
                {
                    //SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor);
                    //SetDCBrushColor(DrawInfo->hdc, RGB(65, 65, 65));
                }
                break;
            }
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
                ) == -1)
            {
                break;
            }

            //if (isMouseDown)
            //{
            //    SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
            //    SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(0xff, 0xff, 0xff));
            //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockBrush(DC_BRUSH));
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
                {
                    //if (!Context->FixedColumnVisible)
                    //{
                    //    rowRect.left = Context->NormalLeft - hScrollPosition;
                    //}

                }

                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                //    SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowBackgroundColor);
                //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockBrush(DC_BRUSH));
                //    break;
                //case 1: // Old colors
                //    SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                //    SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(128, 128, 128));
                //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockBrush(DC_BRUSH));
                //    break;
                //}

                SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(128, 128, 128));
                FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockBrush(DC_BRUSH));
            }
            else
            {
                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor); // RGB(0x0, 0x0, 0x0));
                //    SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowBackgroundColor); // GetSysColor(COLOR_3DFACE));// RGB(0xff, 0xff, 0xff));
                //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockBrush(DC_BRUSH));
                //    break;
                //case 1: // Old colors
                //    SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                //    SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowBackgroundColor); //RGB(65, 65, 65)); //RGB(28, 28, 28)); // RGB(65, 65, 65));
                //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockBrush(DC_BRUSH));
                //    break;
                //}

                SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor); // RGB(0x0, 0x0, 0x0));
                SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowBackgroundColor); // GetSysColor(COLOR_3DFACE));// RGB(0xff, 0xff, 0xff));
                FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockBrush(DC_BRUSH));
            }

            dpiValue = PhGetWindowDpi(DrawInfo->nmcd.hdr.hwndFrom);

            SelectFont(DrawInfo->nmcd.hdc, GetWindowFont(DrawInfo->nmcd.hdr.hwndFrom));

            if (buttonInfo.iImage != -1)
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

                    if (buttonInfo.fsStyle & BTNS_SHOWTEXT)
                    {
                        DrawInfo->nmcd.rc.left += PhGetDpi(5, dpiValue);

                        x = DrawInfo->nmcd.rc.left;// + ((DrawInfo->nmcd.rc.right - DrawInfo->nmcd.rc.left) - PhSmallIconSize.X) / 2;
                        y = DrawInfo->nmcd.rc.top + ((DrawInfo->nmcd.rc.bottom - DrawInfo->nmcd.rc.top) - PhGetSystemMetrics(SM_CYSMICON, dpiValue)) / 2;
                    }
                    else
                    {
                        x = DrawInfo->nmcd.rc.left + ((DrawInfo->nmcd.rc.right - DrawInfo->nmcd.rc.left) - PhGetSystemMetrics(SM_CXSMICON, dpiValue)) / 2;
                        y = DrawInfo->nmcd.rc.top + ((DrawInfo->nmcd.rc.bottom - DrawInfo->nmcd.rc.top) - PhGetSystemMetrics(SM_CYSMICON, dpiValue)) / 2;
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
                }
            }

            if (buttonInfo.fsStyle & BTNS_SHOWTEXT)
            {
                WCHAR buttonText[MAX_PATH] = L"";

                SendMessage(
                    DrawInfo->nmcd.hdr.hwndFrom,
                    TB_GETBUTTONTEXT,
                    (ULONG)DrawInfo->nmcd.dwItemSpec,
                    (LPARAM)buttonText
                    );

                DrawInfo->nmcd.rc.left += PhGetDpi(10, dpiValue);
                DrawText(
                    DrawInfo->nmcd.hdc,
                    buttonText,
                    (UINT)PhCountStringZ(buttonText),
                    &DrawInfo->nmcd.rc,
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
            LONG dpiValue;

            dpiValue = PhGetWindowDpi(DrawInfo->nmcd.hdr.hwndFrom);

            SetBkMode(DrawInfo->nmcd.hdc, TRANSPARENT);

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                SetTextColor(DrawInfo->nmcd.hdc, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                //FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockBrush(DC_BRUSH));
                break;
            case 1: // Old colors
                SetTextColor(DrawInfo->nmcd.hdc, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(128, 128, 128));
                //FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockBrush(DC_BRUSH));
                break;
            }

            if (!PhpListViewFontHandle)
            {
                NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

                if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, dpiValue))
                {
                    metrics.lfMessageFont.lfHeight = PhGetDpi(-11, dpiValue);
                    metrics.lfMessageFont.lfWeight = FW_BOLD;

                    PhpListViewFontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }

            SelectFont(DrawInfo->nmcd.hdc, PhpListViewFontHandle);

            LVGROUP groupInfo = { sizeof(LVGROUP) };
            groupInfo.mask = LVGF_HEADER;
            if (ListView_GetGroupInfo(
                DrawInfo->nmcd.hdr.hwndFrom,
                (ULONG)DrawInfo->nmcd.dwItemSpec,
                &groupInfo
                ) == -1)
            {
                break;
            }

            SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor); // RGB(255, 69, 0)
            SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(65, 65, 65));

            DrawInfo->rcText.top += PhGetDpi(2, dpiValue);
            DrawInfo->rcText.bottom -= PhGetDpi(2, dpiValue);
            FillRect(DrawInfo->nmcd.hdc, &DrawInfo->rcText, GetStockBrush(DC_BRUSH));
            DrawInfo->rcText.top -= PhGetDpi(2, dpiValue);
            DrawInfo->rcText.bottom += PhGetDpi(2, dpiValue);

            //SetTextColor(DrawInfo->nmcd.hdc, RGB(255, 69, 0));
            //SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(65, 65, 65));

            DrawInfo->rcText.left += PhGetDpi(10, dpiValue);
            DrawText(
                DrawInfo->nmcd.hdc,
                groupInfo.pszHeader,
                -1,
                &DrawInfo->rcText,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                );
            DrawInfo->rcText.left -= PhGetDpi(10, dpiValue);
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
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hWnd, LONG_MAX);
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
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

                    if (!GetClassName(customDraw->hdr.hwndFrom, className, RTL_NUMBER_OF(className)))
                        className[0] = UNICODE_NULL;

                    //dprintf("NM_CUSTOMDRAW: %S\r\n", className);

                    if (PhEqualStringZ(className, L"Button", FALSE))
                    {
                        return PhpThemeWindowDrawButton(customDraw);
                    }
                    else if (PhEqualStringZ(className, L"ReBarWindow32", FALSE))
                    {
                        return PhThemeWindowDrawRebar(customDraw);
                    }
                    else if (PhEqualStringZ(className, L"ToolbarWindow32", FALSE))
                    {
                        return PhThemeWindowDrawToolbar((LPNMTBCUSTOMDRAW)customDraw);
                    }
                    else if (PhEqualStringZ(className, L"SysListView32", FALSE))
                    {
                        LPNMLVCUSTOMDRAW listViewCustomDraw = (LPNMLVCUSTOMDRAW)customDraw;

                        if (listViewCustomDraw->dwItemType == LVCDI_GROUP)
                        {
                            return PhpThemeWindowDrawListViewGroup(listViewCustomDraw);
                        }
                    }
                }
            }
        }
        break;
    case WM_CTLCOLOREDIT:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                SetTextColor((HDC)wParam, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor((HDC)wParam, PhThemeWindowTextColor);
                return (INT_PTR)GetStockBrush(DC_BRUSH);
            case 1: // Old colors
                SetTextColor((HDC)wParam, PhThemeWindowTextColor);
                SetDCBrushColor((HDC)wParam, RGB(60, 60, 60));
                return (INT_PTR)GetStockBrush(DC_BRUSH);
            }
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, PhThemeWindowTextColor);
            SetDCBrushColor((HDC)wParam, PhThemeWindowBackgroundColor);
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    case WM_MEASUREITEM:
        if (PhThemeWindowMeasureItem(hWnd, (LPMEASUREITEMSTRUCT)lParam))
            return TRUE;
        break;
    case WM_DRAWITEM:
        if (PhThemeWindowDrawItem((LPDRAWITEMSTRUCT)lParam))
            return TRUE;
        break;
    case WM_NCPAINT:
    case WM_NCACTIVATE:
        {
            LRESULT result = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);

            if (GetMenu(hWnd))
            {
                RECT clientRect;
                RECT windowRect;
                HDC hdc;

                GetClientRect(hWnd, &clientRect);
                GetWindowRect(hWnd, &windowRect);

                MapWindowPoints(hWnd, NULL, (PPOINT)&clientRect, 2);
                PhOffsetRect(&clientRect, -windowRect.left, -windowRect.top);

                // the rcBar is offset by the window rect (thanks to adzm) (dmex)
                RECT rcAnnoyingLine = clientRect;
                rcAnnoyingLine.bottom = rcAnnoyingLine.top;
                rcAnnoyingLine.top--;

                if (hdc = GetWindowDC(hWnd))
                {
                    SetDCBrushColor(hdc, PhThemeWindowBackgroundColor);
                    FillRect(hdc, &rcAnnoyingLine, GetStockBrush(DC_BRUSH));
                    ReleaseDC(hWnd, hdc);
                }
            }

            return result;
        }
        break;
    }

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
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
    case WM_DESTROY:
        {
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            SIZE nameSize;
            RECT textRect;
            ULONG returnLength;
            WCHAR text[MAX_PATH];
            LONG dpiValue;

            if (!(hdc = BeginPaint(WindowHandle, &ps)))
                break;

            dpiValue = PhGetWindowDpi(WindowHandle);

            if (!PhpGroupboxFontHandle) // HACK
            {
                NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

                if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, dpiValue))
                {
                    metrics.lfMessageFont.lfHeight = PhGetDpi(-11, dpiValue);
                    metrics.lfMessageFont.lfWeight = FW_BOLD;

                    PhpGroupboxFontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }

            if (PhGetWindowTextToBuffer(WindowHandle, PH_GET_WINDOW_TEXT_INTERNAL, text, RTL_NUMBER_OF(text), &returnLength) == ERROR_SUCCESS)
            {
                HFONT oldFont;

                SetBkMode(hdc, TRANSPARENT);

                oldFont = SelectFont(hdc, PhpGroupboxFontHandle);
                GetTextExtentPoint32(
                    hdc,
                    text,
                    returnLength,
                    &nameSize
                    );

                //if (PhpThemeBorderEnable)
                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
                //    SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff));
                //    FrameRect(hdc, &clientRect, GetStockBrush(DC_BRUSH));
                //    break;
                //case 1: // Old colors
                //    SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
                //    SetDCBrushColor(hdc, RGB(0, 0, 0)); // RGB(28, 28, 28)); // RGB(65, 65, 65));
                //    FrameRect(hdc, &clientRect, GetStockBrush(DC_BRUSH));
                //    break;
                //}

                textRect.left = 0;
                textRect.top = 0;
                textRect.right = ps.rcPaint.right;
                textRect.bottom = nameSize.cy;

                SetTextColor(hdc, PhThemeWindowTextColor); // RGB(255, 69, 0)
                SetDCBrushColor(hdc, RGB(65, 65, 65));
                FillRect(hdc, &textRect, GetStockBrush(DC_BRUSH));

                //switch (PhpThemeColorMode)
                //{
                //case 0: // New colors
                //    //SetTextColor(hdc, RGB(255, 69, 0));
                //    //SetDCBrushColor(hdc, RGB(65, 65, 65));
                //    //SetTextColor(hdc, RGB(0x0, 0x0, 0x0));
                //    //SetDCBrushColor(hdc, PhThemeWindowTextColor);
                //    //FrameRect(hdc, &clientRect, GetStockBrush(DC_BRUSH));
                //    break;
                //case 1: // Old colors
                //    //SetTextColor(hdc, RGB(255, 69, 0));
                //    //SetDCBrushColor(hdc, RGB(65, 65, 65));
                //    //SetTextColor(hdc, PhThemeWindowTextColor);
                //    //SetDCBrushColor(hdc, PhThemeWindowBackgroundColor);
                //    //FrameRect(hdc, &clientRect, GetStockBrush(DC_BRUSH));
                //    break;
                //}

                textRect.left += PhGetDpi(10, dpiValue);
                DrawText(
                    hdc,
                    text,
                    returnLength,
                    &textRect,
                    DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE
                    );
                textRect.left -= PhGetDpi(10, dpiValue);

                SelectFont(hdc, oldFont);
            }

            EndPaint(WindowHandle, &ps);
        }
        return TRUE;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowEditSubclassProc(
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
    case WM_DESTROY:
        {
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
        }
        break;
    case WM_NCPAINT:
        {
            HDC hdc;
            ULONG flags;
            RECT windowRect;
            HRGN updateRegion;

            updateRegion = (HRGN)wParam;

            if (updateRegion == (HRGN)1) // HRGN_FULL
                updateRegion = NULL;

            flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | DCX_USESTYLE;

            if (updateRegion)
                flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

            if (hdc = GetDCEx(WindowHandle, updateRegion, flags))
            {
                GetWindowRect(WindowHandle, &windowRect);
                PhOffsetRect(&windowRect, -windowRect.left, -windowRect.top);

                if (GetFocus() == WindowHandle)
                {
                    SetDCBrushColor(hdc, GetSysColor(COLOR_HOTLIGHT));
                    FrameRect(hdc, &windowRect, GetStockBrush(DC_BRUSH));
                }
                else
                {
                    SetDCBrushColor(hdc, RGB(65, 65, 65));
                    FrameRect(hdc, &windowRect, GetStockBrush(DC_BRUSH));
                }

                ReleaseDC(WindowHandle, hdc);
                return 0;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
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
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        {
            //ExcludeClipRect(ps.hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
        }
        return TRUE;
    case WM_MOUSEMOVE:
        {
            //POINT pt;
            //INT count;
            //INT i;
            //
            //GetCursorPos(&pt);
            //MapWindowPoints(NULL, WindowHandle, &pt, 1);
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
            //    entry.dwState = PtInRect(&rect, pt) ? TCIS_HIGHLIGHTED : 0;
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

            context->MouseActive = FALSE;
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case WM_PAINT:
        {
            RECT windowRect;
            RECT clientRect;
            HDC hdc;
            HDC bufferDc;
            HBITMAP bufferBitmap;
            HBITMAP oldBufferBitmap;

            GetWindowRect(WindowHandle, &windowRect);
            GetClientRect(WindowHandle, &clientRect);

            hdc = GetDC(WindowHandle);
            bufferDc = CreateCompatibleDC(hdc);
            bufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
            oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

            TabCtrl_AdjustRect(WindowHandle, FALSE, &clientRect); // Make sure we don't paint in the client area.
            ExcludeClipRect(bufferDc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

            windowRect.right -= windowRect.left;
            windowRect.bottom -= windowRect.top;
            windowRect.left = 0;
            windowRect.top = 0;

            clientRect.left = windowRect.left;
            clientRect.top = windowRect.top;
            clientRect.right = windowRect.right;
            clientRect.bottom = windowRect.bottom;

            SetBkMode(bufferDc, TRANSPARENT);
            SelectFont(bufferDc, GetWindowFont(WindowHandle));

            SetTextColor(bufferDc, PhThemeWindowTextColor);
            SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
            FillRect(bufferDc, &clientRect, GetStockBrush(DC_BRUSH));

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
            //        //SetDCBrushColor(DrawInfo->hdc, RGB(65, 65, 65));
            //    }
            //    break;
            //}

            //switch (PhpThemeColorMode)
            //{
            //case 0: // New colors
            //    //SetTextColor(hdc, RGB(0x0, 0xff, 0x0));
            //    SetDCBrushColor(hdc, GetSysColor(COLOR_3DFACE));// PhThemeWindowTextColor);
            //    FillRect(hdc, &clientRect, GetStockBrush(DC_BRUSH));
            //    break;
            //case 1: // Old colors
            //    //SetTextColor(hdc, PhThemeWindowTextColor);
            //    SetDCBrushColor(hdc, RGB(65, 65, 65)); //WindowForegroundColor); // RGB(65, 65, 65));
            //    FillRect(hdc, &clientRect, GetStockBrush(DC_BRUSH));
            //    break;
            //}

            POINT pt;
            GetCursorPos(&pt);
            MapWindowPoints(NULL, WindowHandle, &pt, 1);

            INT currentSelection = TabCtrl_GetCurSel(WindowHandle);
            INT count = TabCtrl_GetItemCount(WindowHandle);

            for (INT i = 0; i < count; i++)
            {
                RECT itemRect;

                TabCtrl_GetItemRect(WindowHandle, i, &itemRect);

                if (PtInRect(&itemRect, pt))
                {
                    PhOffsetRect(&itemRect, 2, 2);

                    switch (PhpThemeColorMode)
                    {
                    case 0: // New colors
                        {
                            if (currentSelection == i)
                            {
                                SetTextColor(bufferDc, RGB(0xff, 0xff, 0xff));
                                SetDCBrushColor(bufferDc, RGB(128, 128, 128));
                                FillRect(bufferDc, &itemRect, GetStockBrush(DC_BRUSH));
                            }
                            else
                            {
                                SetTextColor(bufferDc, PhThemeWindowTextColor);
                                SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
                                FillRect(bufferDc, &itemRect, GetStockBrush(DC_BRUSH));
                            }
                        }
                        break;
                    case 1: // Old colors
                        SetTextColor(bufferDc, PhThemeWindowTextColor);
                        SetDCBrushColor(bufferDc, RGB(128, 128, 128));
                        FillRect(bufferDc, &itemRect, GetStockBrush(DC_BRUSH));
                        break;
                    }

                    //FrameRect(bufferDc, &itemRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                }
                else
                {
                    PhOffsetRect(&itemRect, 2, 2);

                    switch (PhpThemeColorMode)
                    {
                    case 0: // New colors
                        {
                            if (currentSelection == i)
                            {
                                SetTextColor(bufferDc, RGB(0x0, 0x0, 0x0));
                                SetDCBrushColor(bufferDc, RGB(0xff, 0xff, 0xff));
                                FillRect(bufferDc, &itemRect, GetStockBrush(DC_BRUSH));
                            }
                            else
                            {
                                SetTextColor(bufferDc, RGB(0, 0, 0));
                                SetDCBrushColor(bufferDc, GetSysColor(COLOR_3DFACE));// PhThemeWindowTextColor);
                                FillRect(bufferDc, &itemRect, GetStockBrush(DC_BRUSH));
                            }
                        }
                        break;
                    case 1: // Old colors
                        {
                            if (currentSelection == i)
                            {
                                SetTextColor(bufferDc, PhThemeWindowTextColor);
                                SetDCBrushColor(bufferDc, PhThemeWindowForegroundColor);
                                FillRect(bufferDc, &itemRect, GetStockBrush(DC_BRUSH));
                            }
                            else
                            {
                                SetTextColor(bufferDc, PhThemeWindowTextColor);
                                SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
                                FillRect(bufferDc, &itemRect, GetStockBrush(DC_BRUSH));
                            }
                        }
                        break;
                    }
                }

                {
                    TCITEM tabItem;
                    WCHAR tabHeaderText[MAX_PATH];

                    memset(tabHeaderText, 0, sizeof(tabHeaderText));
                    memset(&tabItem, 0, sizeof(TCITEM));

                    tabItem.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_STATE;
                    tabItem.dwStateMask = TCIS_BUTTONPRESSED | TCIS_HIGHLIGHTED;
                    tabItem.cchTextMax = RTL_NUMBER_OF(tabHeaderText);
                    tabItem.pszText = tabHeaderText;

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

            BitBlt(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, bufferDc, 0, 0, SRCCOPY);
            SelectBitmap(bufferDc, oldBufferBitmap);
            DeleteBitmap(bufferBitmap);
            DeleteDC(bufferDc);
            ReleaseDC(WindowHandle, hdc);
        }
        return DefWindowProc(WindowHandle, uMsg, wParam, lParam);
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowHeaderSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_HEADER_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

            PhFree(context);
        }
        break;
    //case WM_ERASEBKGND:
    //    return 1;
    case WM_MOUSEMOVE:
        {
            if (GetCapture() == WindowHandle)
                break;

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

            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            LRESULT result = CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);

            InvalidateRect(WindowHandle, NULL, TRUE);

            return result;
        }
        break;
    case WM_MOUSELEAVE:
        {
            LRESULT result = CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);

            context->MouseActive = FALSE;

            InvalidateRect(WindowHandle, NULL, TRUE);

            return result;
        }
        break;
    case WM_PAINT:
        {
            WCHAR headerText[0x100] = { 0 };
            HDITEM headerItem;
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


            SelectFont(bufferDc, GetWindowFont(WindowHandle));

            SetBkMode(bufferDc, TRANSPARENT);

            //switch (PhpThemeColorMode)
            //{
            //case 0: // New colors
                //SetTextColor(hdc, RGB(0x0, 0x0, 0x0));
                //SetDCBrushColor(hdc, PhThemeWindowTextColor);
                //FillRect(hdc, &clientRect, GetStockBrush(DC_BRUSH));
                //break;
            //case 1: // Old colors
            //    //SetTextColor(hdc, PhThemeWindowTextColor);
                SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor); //WindowForegroundColor); // RGB(65, 65, 65));
                FillRect(bufferDc, &clientRect, GetStockBrush(DC_BRUSH));
            //    break;
            //}

            INT headerItemCount = Header_GetItemCount(WindowHandle);
            POINT pt;

            GetCursorPos(&pt);
            MapWindowPoints(NULL, WindowHandle, &pt, 1);

            for (INT i = 0; i < headerItemCount; i++)
            {
                INT drawTextFlags = DT_SINGLELINE | DT_HIDEPREFIX | DT_WORD_ELLIPSIS;
                RECT headerRect;

                Header_GetItemRect(WindowHandle, i, &headerRect);

                if (PtInRect(&headerRect, pt))
                {
                    switch (PhpThemeColorMode)
                    {
                    case 0: // New colors
                        SetTextColor(bufferDc, PhThemeWindowTextColor);
                        SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
                        FillRect(bufferDc, &headerRect, GetStockBrush(DC_BRUSH));
                        break;
                    case 1: // Old colors
                        SetTextColor(bufferDc, PhThemeWindowTextColor);
                        SetDCBrushColor(bufferDc, RGB(128, 128, 128));
                        FillRect(bufferDc, &headerRect, GetStockBrush(DC_BRUSH));
                        break;
                    }

                    //FrameRect(hdc, &headerRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                }
                else
                {
                    //switch (PhpThemeColorMode)
                   // {
                   // case 0: // New colors

                    //SetTextColor(hdc, RGB(0x0, 0x0, 0x0));
                    //SetDCBrushColor(hdc, PhThemeWindowTextColor);
                    //FillRect(hdc, &headerRect, GetStockBrush(DC_BRUSH));
                    //    break;
                    //case 1: // Old colors
                        SetTextColor(bufferDc, PhThemeWindowTextColor);
                        SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
                        FillRect(bufferDc, &headerRect, GetStockBrush(DC_BRUSH));
                    //    break;
                    //}

                    //FrameRect(hdc, &headerRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                    //SetDCPenColor(hdc, RGB(0, 255, 0));
                    //SetDCBrushColor(hdc, RGB(0, 255, 0));
                    //DrawEdge(hdc, &headerRect, BDR_RAISEDOUTER | BF_FLAT, BF_RIGHT);

                    //RECT frameRect;
                    //frameRect.bottom = headerRect.bottom - 2;
                    //frameRect.left = headerRect.right - 1;
                    //frameRect.right = headerRect.right;
                    //frameRect.top = headerRect.top;
                    //SetDCBrushColor(hdc, RGB(68, 68, 68)); // RGB(0x77, 0x77, 0x77));
                    //FrameRect(hdc, &headerRect, GetStockBrush(DC_BRUSH));

                    //PatBlt(DrawInfo->hDC, DrawInfo->rcItem.right - 1, DrawInfo->rcItem.top, 1, DrawInfo->rcItem.bottom - DrawInfo->rcItem.top, PATCOPY);
                    //PatBlt(DrawInfo->hDC, DrawInfo->rcItem.left, DrawInfo->rcItem.bottom - 1, DrawInfo->rcItem.right - DrawInfo->rcItem.left, 1, PATCOPY);
                    DrawEdge(bufferDc, &headerRect, BDR_RAISEDOUTER, BF_RIGHT);
                }

                ZeroMemory(&headerItem, sizeof(HDITEM));
                headerItem.mask = HDI_TEXT | HDI_FORMAT;
                headerItem.cchTextMax = MAX_PATH;
                headerItem.pszText = headerText;

                if (!Header_GetItem(WindowHandle, i, &headerItem))
                    continue;

                if (headerItem.fmt & HDF_RIGHT)
                    drawTextFlags |= DT_VCENTER | DT_RIGHT;
                else
                    drawTextFlags |= DT_VCENTER | DT_LEFT;

                headerRect.left += 4;
                headerRect.right -= 8;
                DrawText(
                    bufferDc,
                    headerText,
                    (UINT)PhCountStringZ(headerText),
                    &headerRect,
                    drawTextFlags
                    );
            }

            BitBlt(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, bufferDc, 0, 0, SRCCOPY);
            SelectBitmap(bufferDc, oldBufferBitmap);
            DeleteBitmap(bufferBitmap);
            DeleteDC(bufferDc);
            ReleaseDC(WindowHandle, hdc);
        }
        goto DefaultWndProc;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowStatusbarWndSubclassProc(
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
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return FALSE;
    case WM_MOUSEMOVE:
        {
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

            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case WM_MOUSELEAVE:
        {
            context->MouseActive = FALSE;
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case WM_PAINT:
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

            SetBkMode(bufferDc, TRANSPARENT);
            SelectFont(bufferDc, GetWindowFont(WindowHandle));
            SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
            FillRect(bufferDc, &clientRect, GetStockBrush(DC_BRUSH));

            //switch (PhpThemeColorMode)
            //{
            //case 0: // New colors
            //    SetTextColor(bufferDc, RGB(0x0, 0x0, 0x0));
            //    SetDCBrushColor(bufferDc, GetSysColor(COLOR_3DFACE));  // RGB(0xff, 0xff, 0xff));
            //    FillRect(bufferDc, &clientRect, GetStockBrush(DC_BRUSH));
            //    break;
            //case 1: // Old colors
            //    SetTextColor(bufferDc, RGB(0xff, 0xff, 0xff));
            //    SetDCBrushColor(bufferDc, RGB(65, 65, 65)); //RGB(28, 28, 28)); // RGB(65, 65, 65));
            //    FillRect(bufferDc, &clientRect, GetStockBrush(DC_BRUSH));
            //    break;
            //}

            POINT windowPoint;
            GetCursorPos(&windowPoint);
            MapWindowPoints(NULL, WindowHandle, &windowPoint, 1);

            UINT blockCount = (UINT)CallWindowProc(oldWndProc, WindowHandle, SB_GETPARTS, 0, 0);

            for (UINT i = 0; i < blockCount; i++)
            {
                RECT blockRect = { 0 };
                WCHAR buffer[0x100] = { 0 };

                if (!CallWindowProc(oldWndProc, WindowHandle, SB_GETRECT, (WPARAM)i, (WPARAM)&blockRect))
                    continue;
                if (!RectVisible(hdc, &blockRect))
                    continue;
                if (CallWindowProc(oldWndProc, WindowHandle, SB_GETTEXTLENGTH, (WPARAM)i, 0) >= RTL_NUMBER_OF(buffer))
                    continue;
                if (!CallWindowProc(oldWndProc, WindowHandle, SB_GETTEXT, (WPARAM)i, (LPARAM)buffer))
                    continue;

                if (PtInRect(&blockRect, windowPoint))
                {
                    switch (PhpThemeColorMode)
                    {
                    case 0: // New colors
                        SetTextColor(bufferDc, RGB(0xff, 0xff, 0xff));
                        SetDCBrushColor(bufferDc, RGB(64, 64, 64));
                        FillRect(bufferDc, &blockRect, GetStockBrush(DC_BRUSH));
                        break;
                    case 1: // Old colors
                        SetTextColor(bufferDc, RGB(0xff, 0xff, 0xff));
                        SetDCBrushColor(bufferDc, RGB(128, 128, 128));
                        FillRect(bufferDc, &blockRect, GetStockBrush(DC_BRUSH));
                        break;
                    }

                    //FrameRect(bufferDc, &blockRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                }
                else
                {
                    SetTextColor(bufferDc, PhThemeWindowTextColor);
                    SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
                    FillRect(bufferDc, &blockRect, GetStockBrush(DC_BRUSH));

                    //switch (PhpThemeColorMode)
                    //{
                    //case 0: // New colors
                    //    SetTextColor(bufferDc, PhThemeWindowTextColor);
                    //    SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
                    //    //SetTextColor(bufferDc, RGB(0x0, 0x0, 0x0));
                    //    //SetDCBrushColor(bufferDc, GetSysColor(COLOR_3DFACE)); // RGB(0xff, 0xff, 0xff));
                    //    FillRect(bufferDc, &blockRect, GetStockBrush(DC_BRUSH));
                    //    break;
                    //case 1: // Old colors
                    //    SetTextColor(bufferDc, RGB(0xff, 0xff, 0xff));
                    //    SetDCBrushColor(bufferDc, RGB(64, 64, 64));
                    //    FillRect(bufferDc, &blockRect, GetStockBrush(DC_BRUSH));
                    //    break;
                    //}
                    //
                    //FrameRect(bufferDc, &blockRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                }

                blockRect.left += 2;
                DrawText(
                    bufferDc,
                    buffer,
                    (UINT)PhCountStringZ(buffer),
                    &blockRect,
                    DT_LEFT |  DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                    );
                blockRect.left -= 2;

                //SetDCBrushColor(bufferDc, RGB(0x5f, 0x5f, 0x5f));
                //SelectBrush(bufferDc, GetStockBrush(DC_BRUSH));
                //PatBlt(bufferDc, blockRect.right - 1, blockRect.top, 1, blockRect.bottom - blockRect.top, PATCOPY);
                //DrawEdge(bufferDc, &blockRect, BDR_RAISEDOUTER, BF_RIGHT);
            }

            {
                RECT sizeGripRect;
                LONG dpiValue;

                dpiValue = PhGetWindowDpi(WindowHandle);

                sizeGripRect.left = clientRect.right - PhGetSystemMetrics(SM_CXHSCROLL, dpiValue);
                sizeGripRect.top = clientRect.bottom - PhGetSystemMetrics(SM_CYVSCROLL, dpiValue);
                sizeGripRect.right = clientRect.right;
                sizeGripRect.bottom = clientRect.bottom;

                if (context->StatusThemeData)
                    DrawThemeBackground(context->StatusThemeData, bufferDc, SP_GRIPPER, 0, &sizeGripRect, &sizeGripRect);
                else
                    DrawFrameControl(bufferDc, &sizeGripRect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
            }

            BitBlt(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, bufferDc, 0, 0, SRCCOPY);
            SelectBitmap(bufferDc, oldBufferBitmap);
            DeleteBitmap(bufferBitmap);
            DeleteDC(bufferDc);
            ReleaseDC(WindowHandle, hdc);
        }
        return DefWindowProc(WindowHandle, uMsg, wParam, lParam);
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowRebarToolbarSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_HEADER_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

            PhFree(context);
        }
        break;
    case WM_CTLCOLOREDIT:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                SetTextColor((HDC)wParam, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor((HDC)wParam, PhThemeWindowTextColor);
                return (INT_PTR)GetStockBrush(DC_BRUSH);
            case 1: // Old colors
                SetTextColor((HDC)wParam, PhThemeWindowTextColor);
                SetDCBrushColor((HDC)wParam, RGB(60, 60, 60));
                return (INT_PTR)GetStockBrush(DC_BRUSH);
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowStaticControlSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_STATIC_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;

            if (hdc = BeginPaint(WindowHandle, &ps))
            {
                HICON iconHandle;

                SetDCBrushColor(hdc, RGB(42, 42, 42));
                FillRect(hdc, &ps.rcPaint, GetStockBrush(DC_BRUSH));

                iconHandle = Static_GetIcon(WindowHandle, 0);
                DrawIconEx(
                    hdc,
                    ps.rcPaint.left,
                    ps.rcPaint.top,
                    iconHandle,
                    ps.rcPaint.right - ps.rcPaint.left,
                    ps.rcPaint.bottom - ps.rcPaint.top,
                    0,
                    NULL,
                    DI_NORMAL
                    );

                EndPaint(WindowHandle, &ps);
            }
        }
        return TRUE;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
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
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
        }
        break;
    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:
        {
            POINT windowPoint;
            RECT windowRect;

            GetCursorPos(&windowPoint);
            GetWindowRect(WindowHandle, &windowRect);
            context->Hot = PtInRect(&windowRect, windowPoint);

            if (!context->HotTrack)
            {
                TRACKMOUSEEVENT trackMouseEvent;
                trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                trackMouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
                trackMouseEvent.hwndTrack = WindowHandle;
                trackMouseEvent.dwHoverTime = 0;

                context->HotTrack = TRUE;

                TrackMouseEvent(&trackMouseEvent);
            }

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_MOUSELEAVE:
    case WM_NCMOUSELEAVE:
        {
            POINT windowPoint;
            RECT windowRect;

            context->HotTrack = FALSE;

            GetCursorPos(&windowPoint);
            GetWindowRect(WindowHandle, &windowRect);
            context->Hot = PtInRect(&windowRect, windowPoint);

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_CAPTURECHANGED:
        {
            POINT windowPoint;
            RECT windowRect;

            GetCursorPos(&windowPoint);
            GetWindowRect(WindowHandle, &windowRect);
            context->Hot = PtInRect(&windowRect, windowPoint);

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
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

                if (updateRegion != (HRGN)1)
                    CombineRgn(rectregion, rectregion, updateRegion, RGN_AND);
                DefWindowProc(WindowHandle, WM_NCPAINT, (WPARAM)rectregion, 0);
                DeleteRgn(rectregion);
            }

            if (updateRegion == (HRGN)1) // HRGN_FULL
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
                    SetDCBrushColor(hdc, RGB(0x8f, 0x8f, 0x8f));// GetSysColor(COLOR_HOTLIGHT));
                    FrameRect(hdc, &windowRect, GetStockBrush(DC_BRUSH));
                    //windowRect.left += 1;
                    //windowRect.top += 1;
                    //windowRect.right -= 1;
                    //windowRect.bottom -= 1;
                    //FrameRect(hdc, &windowRect, GetStockBrush(DC_BRUSH));
                }
                else
                {
                    SetDCBrushColor(hdc, RGB(65, 65, 65));
                    FrameRect(hdc, &windowRect, GetSysColorBrush(COLOR_WINDOWFRAME));
                    //windowRect.left += 1;
                    //windowRect.top += 1;
                    //windowRect.right -= 1;
                    //windowRect.bottom -= 1;
                    //FrameRect(hdc, &windowRect, GetSysColorBrush(COLOR_WINDOWFRAME));
                }

                ReleaseDC(WindowHandle, hdc);
                return TRUE;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowComboBoxControlSubclassProc(
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
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;

            if (hdc = GetDC(WindowHandle))
            {
                BOOLEAN isFocused = GetFocus() == WindowHandle;
                BOOLEAN isMouseover = FALSE;
                RECT clientRect;

                GetClientRect(WindowHandle, &clientRect);

                SetBkMode(hdc, TRANSPARENT);

                LONG_PTR windowStyle = PhGetWindowStyle(WindowHandle);

                if ((windowStyle & CBS_DROPDOWNLIST) != CBS_DROPDOWNLIST)
                {
                    COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };

                    if (CallWindowProc(oldWndProc, WindowHandle, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info))
                    {
                        ExcludeClipRect(hdc, info.rcItem.left + 1, info.rcItem.top + 1, info.rcItem.right + 1, info.rcItem.bottom + 1);
                    }
                }

                POINT clientPoint;
                GetCursorPos(&clientPoint);
                MapWindowPoints(NULL, WindowHandle, &clientPoint, 1);

                if (PtInRect(&clientRect, clientPoint))
                {
                    isMouseover = TRUE;
                }

                SetDCBrushColor(hdc, PhThemeWindowBackgroundColor);
                FillRect(hdc, &clientRect, GetStockBrush(DC_BRUSH));

                SelectFont(hdc, CallWindowProc(oldWndProc, WindowHandle, WM_GETFONT, 0, 0));
                SetTextColor(hdc, PhThemeWindowTextColor);
                SetDCBrushColor(hdc, isMouseover ? RGB(0x8f, 0x8f, 0x8f) : GetSysColor(COLOR_WINDOWFRAME));// RGB(0, 120, 212) : RGB(68, 68, 68));
                FrameRect(hdc, &clientRect, GetStockBrush(DC_BRUSH));

                //{
                //    static HTHEME themeHandle = NULL;
                //    if (!themeHandle)
                //        themeHandle = OpenThemeData(NULL, L"ComboBox");
                //
                //    RECT themeRect = clientRect;
                //    themeRect.left = themeRect.right - GetSystemMetrics(SM_CYHSCROLL);
                //
                //    DrawThemeBackground(themeHandle, hdc, CP_DROPDOWNBUTTONRIGHT, CBXSR_NORMAL, &themeRect, NULL);
                //}

                UINT index = ComboBox_GetCurSel(WindowHandle);
                if (index != CB_ERR)
                {
                    WCHAR text[MAX_PATH];

                    ComboBox_GetLBText(WindowHandle, index, text);

                    clientRect.left += 5;
                    DrawText(
                        hdc,
                        text,
                        (UINT)PhCountStringZ(text),
                        &clientRect,
                        DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE
                        );
                    clientRect.left -= 5;
                }

                ReleaseDC(WindowHandle, hdc);
            }
        }
        return DefWindowProc(WindowHandle, uMsg, wParam, lParam);
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
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
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
        }
        break;
    case WM_ERASEBKGND:
        {
            RECT rc;
            GetClientRect(WindowHandle, &rc);
            FillRect((HDC)wParam, &rc, PhMenuBackgroundBrush);
            return 1;
        }
    case WM_CTLCOLORSTATIC:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, PhThemeWindowTextColor);
            SetDCBrushColor((HDC)wParam, PhThemeWindowBackgroundColor);
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}
