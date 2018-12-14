/*
* Process Hacker -
*   Window theme support functions
*
* Copyright (C) 2018 dmex
*
* This file is part of Process Hacker.
*
* Process Hacker is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Process Hacker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
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
    BOOLEAN MouseActive;
    HTHEME StatusThemeData;
} PHP_THEME_WINDOW_STATUSBAR_CONTEXT, *PPHP_THEME_WINDOW_STATUSBAR_CONTEXT;

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

// rev // Win10-RS5 (uxtheme.dll ordinal 132)
BOOLEAN (WINAPI *ShouldAppsUseDarkMode_I)(
    VOID
    ) = NULL;
// rev // Win10-RS5 (uxtheme.dll ordinal 133)
BOOLEAN (WINAPI *AllowDarkModeForWindow_I)(
    _In_ HWND WindowHandle,
    _In_ BOOL Enabled
    ) = NULL;
// rev // Win10-RS5 (uxtheme.dll ordinal 137)
BOOLEAN (WINAPI *IsDarkModeAllowedForWindow_I)(
    _In_ HWND WindowHandle
    ) = NULL;

ULONG PhpThemeColorMode = 0;
BOOLEAN PhpThemeEnable = FALSE;
BOOLEAN PhpThemeBorderEnable = TRUE;
HBRUSH PhMenuBackgroundBrush = NULL;
COLORREF PhpThemeWindowForegroundColor = RGB(28, 28, 28);
COLORREF PhpThemeWindowBackgroundColor = RGB(64, 64, 64);
COLORREF PhpThemeWindowTextColor = RGB(0xff, 0xff, 0xff);
HFONT PhpTabControlFontHandle = NULL;
HFONT PhpToolBarFontHandle = NULL;
HFONT PhpHeaderFontHandle = NULL;
HFONT PhpListViewFontHandle = NULL;
HFONT PhpMenuFontHandle = NULL;
HFONT PhpGroupboxFontHandle = NULL;
HFONT PhpStatusBarFontHandle = NULL;

VOID PhInitializeWindowTheme(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    PhpThemeColorMode = PhGetIntegerSetting(L"GraphColorMode");
    PhpThemeEnable = !!PhGetIntegerSetting(L"EnableThemeSupport");
    PhpThemeBorderEnable = !!PhGetIntegerSetting(L"TreeListBorderEnable");

    switch (PhpThemeColorMode)
    {
    case 0: // New colors
        {
            if (PhMenuBackgroundBrush)
                DeleteBrush(PhMenuBackgroundBrush);

            PhMenuBackgroundBrush = CreateSolidBrush(PhpThemeWindowTextColor);
        }
        break;
    case 1: // Old colors
        {
            if (PhMenuBackgroundBrush)
                DeleteBrush(PhMenuBackgroundBrush);

            PhMenuBackgroundBrush = CreateSolidBrush(PhpThemeWindowForegroundColor);
        }
        break;
    }

    if (EnableThemeSupport)
    {
        WNDPROC defaultWindowProc;

        defaultWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);

        if (defaultWindowProc != PhpThemeWindowSubclassProc)
        {
            PhSetWindowContext(WindowHandle, SHRT_MAX, defaultWindowProc);
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowSubclassProc);
        }

        PhEnumChildWindows(
            WindowHandle,
            0x1000,
            PhpThemeWindowEnumChildWindows,
            0
            );

        InvalidateRect(WindowHandle, NULL, FALSE); // HACK
    }
    else
    {
        EnableThemeDialogTexture(WindowHandle, ETDT_ENABLETAB);
    }
}

VOID PhReInitializeWindowTheme(
    _In_ HWND WindowHandle
    )
{
    HWND currentWindow = NULL;

    if (!PhpThemeEnable)
        return;

    // HACK
    { 
        if (PhMenuBackgroundBrush)
        {
            DeleteBrush(PhMenuBackgroundBrush);
        }

        PhpThemeColorMode = PhGetIntegerSetting(L"GraphColorMode");
        PhpThemeBorderEnable = !!PhGetIntegerSetting(L"TreeListBorderEnable");

        switch (PhpThemeColorMode)
        {
        case 0: // New colors
            PhMenuBackgroundBrush = CreateSolidBrush(PhpThemeWindowTextColor);
            break;
        case 1: // Old colors
            PhMenuBackgroundBrush = CreateSolidBrush(PhpThemeWindowForegroundColor);
            break;
        }
    }

    PhEnumChildWindows(
        WindowHandle,
        0x1000,
        PhpReInitializeThemeWindowEnumChildWindows,
        0
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
                    windowClassName[0] = 0;

                //dprintf("PhReInitializeWindowTheme: %S\r\n", windowClassName);

                if (currentWindow != WindowHandle)
                {
                    if (PhEqualStringZ(windowClassName, L"#32770", FALSE))
                    {
                        PhEnumChildWindows(
                            currentWindow,
                            0x1000,
                            PhpReInitializeThemeWindowEnumChildWindows,
                            0
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

VOID PhInitializeThemeWindowHeader(
    _In_ HWND HeaderWindow
    )
{
    PPHP_THEME_WINDOW_HEADER_CONTEXT headerControlContext;

    headerControlContext = PhAllocateZero(sizeof(PHP_THEME_WINDOW_HEADER_CONTEXT));
    headerControlContext->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(HeaderWindow, GWLP_WNDPROC);

    PhSetWindowContext(HeaderWindow, SHRT_MAX, headerControlContext);
    SetWindowLongPtr(HeaderWindow, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowHeaderSubclassProc);

    InvalidateRect(HeaderWindow, NULL, FALSE);
}

VOID PhInitializeThemeWindowTabControl(
    _In_ HWND TabControlWindow
    )
{
    PPHP_THEME_WINDOW_TAB_CONTEXT tabControlContext;

    if (!PhpTabControlFontHandle)
        PhpTabControlFontHandle = PhDuplicateFontWithNewHeight(PhApplicationFont, 15);

    tabControlContext = PhAllocateZero(sizeof(PHP_THEME_WINDOW_TAB_CONTEXT));
    tabControlContext->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(TabControlWindow, GWLP_WNDPROC);

    PhSetWindowContext(TabControlWindow, SHRT_MAX, tabControlContext);
    SetWindowLongPtr(TabControlWindow, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowTabControlWndSubclassProc);

    PhSetWindowStyle(TabControlWindow, TCS_OWNERDRAWFIXED, TCS_OWNERDRAWFIXED);

    SendMessage(TabControlWindow, WM_SETFONT, (WPARAM)PhpTabControlFontHandle, FALSE);

    InvalidateRect(TabControlWindow, NULL, FALSE);
}

VOID PhInitializeWindowThemeStatusBar(
    _In_ HWND StatusBarHandle
    )
{
    PPHP_THEME_WINDOW_STATUSBAR_CONTEXT statusbarContext;

    statusbarContext = PhAllocateZero(sizeof(PHP_THEME_WINDOW_STATUSBAR_CONTEXT));
    statusbarContext->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(StatusBarHandle, GWLP_WNDPROC);
    statusbarContext->StatusThemeData = OpenThemeData(StatusBarHandle, VSCLASS_STATUS);

    PhSetWindowContext(StatusBarHandle, SHRT_MAX, statusbarContext);
    SetWindowLongPtr(StatusBarHandle, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowStatusbarWndSubclassProc);

    InvalidateRect(StatusBarHandle, NULL, FALSE);
}

VOID PhInitializeThemeWindowGroupBox(
    _In_ HWND GroupBoxHandle
    )
{
    WNDPROC groupboxWindowProc;

    groupboxWindowProc = (WNDPROC)GetWindowLongPtr(GroupBoxHandle, GWLP_WNDPROC);
    PhSetWindowContext(GroupBoxHandle, SHRT_MAX, groupboxWindowProc);
    SetWindowLongPtr(GroupBoxHandle, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowGroupBoxSubclassProc);

    InvalidateRect(GroupBoxHandle, NULL, FALSE);
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
        0
        );

    if (PhGetWindowContext(WindowHandle, SHRT_MAX)) // HACK
        return TRUE;

    if (!GetClassName(WindowHandle, windowClassName, RTL_NUMBER_OF(windowClassName)))
        windowClassName[0] = 0;

    //dprintf("PhpThemeWindowEnumChildWindows: %S\r\n", windowClassName);

    if (PhEqualStringZ(windowClassName, L"#32770", TRUE))
    {
        PhInitializeWindowTheme(WindowHandle, TRUE);
    }
    else if (PhEqualStringZ(windowClassName, L"Button", FALSE))
    {
        if ((PhGetWindowStyle(WindowHandle) & BS_GROUPBOX) == BS_GROUPBOX)
        {
            PhInitializeThemeWindowGroupBox(WindowHandle);
        }
    }
    else if (PhEqualStringZ(windowClassName, L"SysTabControl32", FALSE))
    {
        PhInitializeThemeWindowTabControl(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, L"msctls_statusbar32", FALSE))
    {
        PhInitializeWindowThemeStatusBar(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, L"Edit", TRUE))
    {
        NOTHING;
    }
    else if (PhEqualStringZ(windowClassName, L"ScrollBar", FALSE))
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
    else if (PhEqualStringZ(windowClassName, L"SysHeader32", TRUE))
    {
        PhInitializeThemeWindowHeader(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, L"SysListView32", FALSE))
    {
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
            ListView_SetBkColor(WindowHandle, RGB(30, 30, 30));
            ListView_SetTextBkColor(WindowHandle, RGB(30, 30, 30));
            ListView_SetTextColor(WindowHandle, PhpThemeWindowTextColor);
            break;
        }

        //InvalidateRect(WindowHandle, NULL, FALSE);
    }
    else if (PhEqualStringZ(windowClassName, L"SysTreeView32", FALSE))
    {
        switch (PhpThemeColorMode)
        {
        case 0: // New colors
            break;
        case 1: // Old colors
            TreeView_SetBkColor(WindowHandle, RGB(30, 30, 30));
            //TreeView_SetTextBkColor(WindowHandle, RGB(30, 30, 30));
            TreeView_SetTextColor(WindowHandle, PhpThemeWindowTextColor);
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

        //InvalidateRect(WindowHandle, NULL, FALSE);
    }
    else if (PhEqualStringZ(windowClassName, L"PhTreeNew", FALSE))
    {
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
        0
        );

    if (!GetClassName(WindowHandle, windowClassName, RTL_NUMBER_OF(windowClassName)))
        windowClassName[0] = 0;

    //dprintf("PhpReInitializeThemeWindowEnumChildWindows: %S\r\n", windowClassName);

    if (PhEqualStringZ(windowClassName, L"SysListView32", FALSE))
    {
        switch (PhpThemeColorMode)
        {
        case 0: // New colors
            ListView_SetBkColor(WindowHandle, RGB(0xff, 0xff, 0xff));
            ListView_SetTextBkColor(WindowHandle, RGB(0xff, 0xff, 0xff));
            ListView_SetTextColor(WindowHandle, RGB(0x0, 0x0, 0x0));
            break;
        case 1: // Old colors
            ListView_SetBkColor(WindowHandle, RGB(30, 30, 30));
            ListView_SetTextBkColor(WindowHandle, RGB(30, 30, 30));
            ListView_SetTextColor(WindowHandle, PhpThemeWindowTextColor);
            break;
        }
    }
    else if (PhEqualStringZ(windowClassName, L"ScrollBar", FALSE))
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
            break;
        case 1: // Old colors
            TreeNew_ThemeSupport(WindowHandle, TRUE);
            break;
        }
    }

    InvalidateRect(WindowHandle, NULL, TRUE);

    return TRUE;
}

BOOLEAN PhThemeWindowDrawItem(
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
    case ODT_STATIC:
        break;
    case ODT_HEADER:
        {
            HDITEM headerItem;
            WCHAR headerText[MAX_PATH];

            SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            SetDCBrushColor(DrawInfo->hDC, RGB(30, 30, 30));

            FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockObject(DC_BRUSH));
            DrawEdge(DrawInfo->hDC, &DrawInfo->rcItem, BDR_RAISEDOUTER, BF_RIGHT);

            memset(headerText, 0, sizeof(headerText));
            memset(&headerItem, 0, sizeof(HDITEM));
            headerItem.mask = HDI_TEXT;
            headerItem.cchTextMax = RTL_NUMBER_OF(headerText);
            headerItem.pszText = headerText;

            if (Header_GetItem(DrawInfo->hwndItem, DrawInfo->itemID, &headerItem))
            {
                DrawText(
                    DrawInfo->hDC,
                    headerText,
                    (UINT)PhCountStringZ(headerText),
                    &DrawInfo->rcItem,
                    DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE //DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX | DT_NOCLIP
                    );
            }
        }
        return TRUE;
    case ODT_MENU:
        {
            PPH_EMENU_ITEM menuItemInfo = (PPH_EMENU_ITEM)DrawInfo->itemData;

            //FillRect(
            //    DrawInfo->hDC, 
            //    &DrawInfo->rcItem, 
            //    CreateSolidBrush(RGB(0, 0, 0))
            //    );
            //SetTextColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));

            if (isDisabled)
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_GRAYTEXT));
                    SetDCBrushColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));
                    break;
                case 1: // Old colors
                    SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_GRAYTEXT));
                    SetDCBrushColor(DrawInfo->hDC, RGB(28, 28, 28));
                    break;
                }

                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockObject(DC_BRUSH));
            }
            else if (isSelected)
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetTextColor(DrawInfo->hDC, PhpThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->hDC, PhpThemeWindowBackgroundColor);
                    break;
                case 1: // Old colors
                    SetTextColor(DrawInfo->hDC, PhpThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->hDC, RGB(128, 128, 128));
                    break;
                }

                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockObject(DC_BRUSH));
            }
            else
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_WINDOWTEXT));
                    SetDCBrushColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));
                    break;
                case 1: // Old colors
                    SetTextColor(DrawInfo->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    SetDCBrushColor(DrawInfo->hDC, RGB(28, 28, 28));
                    break;
                }

                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockObject(DC_BRUSH));
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
                    oldTextColor = SetTextColor(DrawInfo->hDC, PhpThemeWindowTextColor);
                    //FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockObject(DC_BRUSH));
                    //SetTextColor(DrawInfo->hDC, PhpThemeWindowTextColor);
                    break;
                }

                DrawInfo->rcItem.left += 8;
                DrawInfo->rcItem.top += 3;
                DrawText(
                    DrawInfo->hDC,
                    menuCheckText.Buffer,
                    (UINT)menuCheckText.Length / sizeof(WCHAR),
                    &DrawInfo->rcItem,
                    DT_VCENTER | DT_NOCLIP
                    );
                DrawInfo->rcItem.left -= 8;
                DrawInfo->rcItem.top -= 3;

                SetTextColor(DrawInfo->hDC, oldTextColor);
            }

            if (menuItemInfo->Flags & PH_EMENU_SEPARATOR)
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetDCBrushColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));
                    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockObject(DC_BRUSH));
                    break;
                case 1: // Old colors
                    SetDCBrushColor(DrawInfo->hDC, RGB(28, 28, 28));
                    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockObject(DC_BRUSH));
                    break;
                }

                //DrawInfo->rcItem.top += 1;
                //DrawInfo->rcItem.bottom -= 2;
                //DrawFocusRect(drawInfo->hDC, &drawInfo->rcItem);

                SetRect(
                    &DrawInfo->rcItem,
                    DrawInfo->rcItem.left + 25,
                    DrawInfo->rcItem.top + 2,
                    DrawInfo->rcItem.right,
                    DrawInfo->rcItem.bottom - 2
                    );

                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetDCBrushColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));
                    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockObject(DC_BRUSH));
                    break;
                case 1: // Old colors
                    SetDCBrushColor(DrawInfo->hDC, RGB(78, 78, 78));
                    FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockObject(DC_BRUSH));
                    break;
                }

                //DrawEdge(drawInfo->hDC, &drawInfo->rcItem, BDR_RAISEDINNER, BF_TOP);
            }
            else
            {
                PH_STRINGREF part;
                PH_STRINGREF firstPart;
                PH_STRINGREF secondPart;

                PhInitializeStringRef(&part, menuItemInfo->Text);
                PhSplitStringRefAtLastChar(&part, '\b', &firstPart, &secondPart);

                //SetDCBrushColor(DrawInfo->hDC, RGB(28, 28, 28));
                //FillRect(DrawInfo->hDC, &DrawInfo->rcItem, GetStockObject(DC_BRUSH));

                if (menuItemInfo->Bitmap)
                {
                    HDC hdcMem;
                    BLENDFUNCTION blendFunction;

                    blendFunction.BlendOp = AC_SRC_OVER;
                    blendFunction.BlendFlags = 0;
                    blendFunction.SourceConstantAlpha = 255;
                    blendFunction.AlphaFormat = AC_SRC_ALPHA;

                    hdcMem = CreateCompatibleDC(DrawInfo->hDC);
                    SelectBitmap(hdcMem, menuItemInfo->Bitmap);

                    GdiAlphaBlend(
                        DrawInfo->hDC,
                        DrawInfo->rcItem.left + 4,
                        DrawInfo->rcItem.top + 4,
                        16,
                        16,
                        hdcMem,
                        0,
                        0,
                        16,
                        16,
                        blendFunction
                        );

                    DeleteDC(hdcMem);
                }

                DrawInfo->rcItem.left += 25;
                DrawInfo->rcItem.right -= 25;

                DrawText(
                    DrawInfo->hDC,
                    firstPart.Buffer,
                    (UINT)firstPart.Length / sizeof(WCHAR),
                    &DrawInfo->rcItem,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX | DT_NOCLIP
                    );

                DrawText(
                    DrawInfo->hDC,
                    secondPart.Buffer,
                    (UINT)secondPart.Length / sizeof(WCHAR),
                    &DrawInfo->rcItem,
                    DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX | DT_NOCLIP
                    );
            }

            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN PhThemeWindowMeasureItem(
    _In_ PMEASUREITEMSTRUCT DrawInfo
    )
{
    if (DrawInfo->CtlType == ODT_MENU)
    {
        PPH_EMENU_ITEM menuItemInfo = (PPH_EMENU_ITEM)DrawInfo->itemData;

        DrawInfo->itemWidth = 150;
        DrawInfo->itemHeight = 26;

        if ((menuItemInfo->Flags & PH_EMENU_SEPARATOR) == PH_EMENU_SEPARATOR)
        {
            DrawInfo->itemHeight = GetSystemMetrics(SM_CYMENU) >> 2;
        }
        else if (menuItemInfo->Text)
        {
            HDC hdc;

            if (!PhpMenuFontHandle)
            {
                NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

                if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
                {
                    PhpMenuFontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }

            if (hdc = GetDC(NULL))
            {
                PWSTR text;
                SIZE_T textCount;
                SIZE textSize;
                HFONT oldFont;

                text = menuItemInfo->Text;
                textCount = PhCountStringZ(text);
                oldFont = SelectFont(hdc, PhpMenuFontHandle);

                if (GetTextExtentPoint32(hdc, text, (ULONG)textCount, &textSize))
                {
                    DrawInfo->itemWidth = textSize.cx + (GetSystemMetrics(SM_CYBORDER) * 2) + 90; // HACK
                    DrawInfo->itemHeight = GetSystemMetrics(SM_CYMENU) + (GetSystemMetrics(SM_CYBORDER) * 2) + 1;
                }

                SelectFont(hdc, oldFont);
                ReleaseDC(NULL, hdc);
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
            ULONG_PTR buttonStyle;
            HFONT oldFont;

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
                FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockObject(DC_BRUSH));
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
                FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockObject(DC_BRUSH));
            }
            else
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    //SetTextColor(DrawInfo->hdc, PhpThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->hdc, PhpThemeWindowTextColor);
                    break;
                case 1: // Old colors
                    SetTextColor(DrawInfo->hdc, PhpThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->hdc, RGB(42, 42, 42)); // WindowForegroundColor
                    break;
                }
                FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockObject(DC_BRUSH));
            }

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
                FrameRect(DrawInfo->hdc, &DrawInfo->rc, GetStockObject(DC_BRUSH));

                if (!(buttonIcon = Static_GetIcon(DrawInfo->hdr.hwndFrom, 0)))
                    buttonIcon = (HICON)SendMessage(DrawInfo->hdr.hwndFrom, BM_GETIMAGE, IMAGE_ICON, 0);

                if (buttonIcon)
                {
                    ICONINFO iconInfo;
                    BITMAP bmp;
                    LONG width;
                    LONG height;

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

                        DeleteObject(iconInfo.hbmColor);
                    }
                    else if (iconInfo.hbmMask)
                    {
                        if (GetObject(iconInfo.hbmMask, sizeof(BITMAP), &bmp))
                        {
                            width = bmp.bmWidth;
                            height = bmp.bmHeight / 2;
                        }

                        DeleteObject(iconInfo.hbmMask);
                    }

                    bufferRect.left += 1; // HACK
                    bufferRect.top += 1; // HACK

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
                HFONT newFont = PhDuplicateFontWithNewHeight(PhApplicationFont, 22);
                oldFont = SelectFont(DrawInfo->hdc, newFont);

                if (Button_GetCheck(DrawInfo->hdr.hwndFrom) == BST_CHECKED)
                {
                    DrawText(
                        DrawInfo->hdc,
                        L"\u2611",
                        -1,
                        &DrawInfo->rc,
                        DT_LEFT | DT_SINGLELINE | DT_VCENTER
                        );
                }
                else
                {
                    DrawText(
                        DrawInfo->hdc,
                        L"\u2610",
                        -1,
                        &DrawInfo->rc,
                        DT_LEFT | DT_SINGLELINE | DT_VCENTER
                        );
                }

                SelectFont(DrawInfo->hdc, oldFont);
                DeleteFont(newFont);

                DrawInfo->rc.left += 10;
                DrawText(
                    DrawInfo->hdc,
                    buttonText->Buffer,
                    (UINT)buttonText->Length / sizeof(WCHAR),
                    &DrawInfo->rc,
                    DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_HIDEPREFIX
                    );
                DrawInfo->rc.left -= 10;
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

                SetDCBrushColor(DrawInfo->hdc, RGB(65, 65, 65));
                FrameRect(DrawInfo->hdc, &DrawInfo->rc, GetStockObject(DC_BRUSH));

                if (!(buttonIcon = Static_GetIcon(DrawInfo->hdr.hwndFrom, 0)))
                    buttonIcon = (HICON)SendMessage(DrawInfo->hdr.hwndFrom, BM_GETIMAGE, IMAGE_ICON, 0);

                if (buttonIcon)
                {
                    DrawIconEx(
                        DrawInfo->hdc,
                        bufferRect.left + ((bufferRect.right - bufferRect.left) - 16) / 2,
                        bufferRect.top + ((bufferRect.bottom - bufferRect.top) - 16) / 2,
                        buttonIcon, 
                        16,
                        16,
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
    switch (DrawInfo->dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                SetTextColor(DrawInfo->hdc, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_3DFACE)); // RGB(0xff, 0xff, 0xff));
                break;
            case 1: // Old colors
                SetTextColor(DrawInfo->hdc, RGB(0xff, 0xff, 0xff));
                SetDCBrushColor(DrawInfo->hdc, RGB(65, 65, 65));
                break;
            }

            FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockObject(DC_BRUSH));
        }
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
        {
            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                SetTextColor(DrawInfo->hdc, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_3DFACE)); // RGB(0x0, 0x0, 0x0));
            case 1: // Old colors
                SetTextColor(DrawInfo->hdc, PhpThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->hdc, RGB(65, 65, 65));
                break;
            }

            FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockObject(DC_BRUSH));
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
            //    SetTextColor(DrawInfo->nmcd.hdc, PhpThemeWindowTextColor);
            //    SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(0xff, 0xff, 0xff));
            //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockObject(DC_BRUSH));
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

                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetTextColor(DrawInfo->nmcd.hdc, PhpThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->nmcd.hdc, PhpThemeWindowBackgroundColor);
                    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockObject(DC_BRUSH));
                    break;
                case 1: // Old colors
                    SetTextColor(DrawInfo->nmcd.hdc, PhpThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(128, 128, 128));
                    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockObject(DC_BRUSH));
                    break;
                }
            }
            else
            {
                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetTextColor(DrawInfo->nmcd.hdc, RGB(0x0, 0x0, 0x0));
                    SetDCBrushColor(DrawInfo->nmcd.hdc, GetSysColor(COLOR_3DFACE));// RGB(0xff, 0xff, 0xff));
                    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockObject(DC_BRUSH));
                    break;
                case 1: // Old colors
                    SetTextColor(DrawInfo->nmcd.hdc, PhpThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(65, 65, 65)); //RGB(28, 28, 28)); // RGB(65, 65, 65));
                    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockObject(DC_BRUSH));
                    break;
                }
            }

            if (!PhpToolBarFontHandle)
            {
                NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

                if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
                {
                    PhpToolBarFontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }

            SelectFont(DrawInfo->nmcd.hdc, PhpToolBarFontHandle);

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
                    DrawInfo->nmcd.rc.left += 5;

                    ImageList_Draw(
                        toolbarImageList,
                        buttonInfo.iImage,
                        DrawInfo->nmcd.hdc,
                        DrawInfo->nmcd.rc.left,
                        DrawInfo->nmcd.rc.top + 3,
                        ILD_NORMAL
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

                DrawInfo->nmcd.rc.left += 10;
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
            SetBkMode(DrawInfo->nmcd.hdc, TRANSPARENT);

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                SetTextColor(DrawInfo->nmcd.hdc, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor(DrawInfo->nmcd.hdc, PhpThemeWindowTextColor);
                //FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockObject(DC_BRUSH));
                break;
            case 1: // Old colors
                SetTextColor(DrawInfo->nmcd.hdc, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(128, 128, 128));
                //FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, GetStockObject(DC_BRUSH));
                break;
            }

            if (!PhpListViewFontHandle)
            {
                NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

                if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
                {
                    metrics.lfMessageFont.lfHeight = -11;
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

            SetTextColor(DrawInfo->nmcd.hdc, RGB(255, 69, 0));
            SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(65, 65, 65));

            DrawInfo->rcText.top += 2;
            DrawInfo->rcText.bottom -= 2;
            FillRect(DrawInfo->nmcd.hdc, &DrawInfo->rcText, GetStockObject(DC_BRUSH));
            DrawInfo->rcText.top -= 2;
            DrawInfo->rcText.bottom += 2;

            //SetTextColor(DrawInfo->nmcd.hdc, RGB(255, 69, 0));
            //SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(65, 65, 65));

            DrawInfo->rcText.left += 10;
            DrawText(
                DrawInfo->nmcd.hdc,
                groupInfo.pszHeader,
                -1,
                &DrawInfo->rcText,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                );
            DrawInfo->rcText.left -= 10;
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

    if (!(oldWndProc = PhGetWindowContext(hWnd, SHRT_MAX)))
        return FALSE;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hWnd, SHRT_MAX);
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
                        className[0] = 0;

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
                SetDCBrushColor((HDC)wParam, PhpThemeWindowTextColor);
                return (INT_PTR)GetStockObject(DC_BRUSH);
            case 1: // Old colors
                SetTextColor((HDC)wParam, PhpThemeWindowTextColor);
                SetDCBrushColor((HDC)wParam, RGB(60, 60, 60));
                return (INT_PTR)GetStockObject(DC_BRUSH);
            }
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                SetTextColor((HDC)wParam, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor((HDC)wParam, PhpThemeWindowTextColor);
                return (INT_PTR)GetStockObject(DC_BRUSH);
            case 1: // Old colors
                SetTextColor((HDC)wParam, PhpThemeWindowTextColor);
                SetDCBrushColor((HDC)wParam, RGB(30, 30, 30));
                return (INT_PTR)GetStockObject(DC_BRUSH);
            }
        }
        break;
    case WM_MEASUREITEM:
        if (PhThemeWindowMeasureItem((LPMEASUREITEMSTRUCT)lParam))
            return TRUE;
        break;
    case WM_DRAWITEM:
        if (PhThemeWindowDrawItem((LPDRAWITEMSTRUCT)lParam))
            return TRUE;
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

    if (!(oldWndProc = PhGetWindowContext(WindowHandle, SHRT_MAX)))
        return FALSE;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(WindowHandle, SHRT_MAX);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            SIZE nameSize;
            RECT clientRect;
            RECT textRect;
            ULONG returnLength;
            WCHAR text[MAX_PATH];

            if (!(hdc = BeginPaint(WindowHandle, &ps)))
                break;

            GetClientRect(WindowHandle, &clientRect);

            if (!PhpGroupboxFontHandle) // HACK
            {
                NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

                if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
                {
                    metrics.lfMessageFont.lfHeight = -12;
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
                //    FrameRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                //    break;
                //case 1: // Old colors
                //    SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
                //    SetDCBrushColor(hdc, RGB(0, 0, 0)); // RGB(28, 28, 28)); // RGB(65, 65, 65));
                //    FrameRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                //    break;
                //}

                textRect.left = 0;
                textRect.top = 0;
                textRect.right = clientRect.right;
                textRect.bottom = nameSize.cy;

                SetTextColor(hdc, RGB(255, 69, 0));
                SetDCBrushColor(hdc, RGB(65, 65, 65));
                FillRect(hdc, &textRect, GetStockObject(DC_BRUSH));

                switch (PhpThemeColorMode)
                {
                case 0: // New colors
                    SetTextColor(hdc, RGB(255, 69, 0));
                    SetDCBrushColor(hdc, RGB(65, 65, 65));
                    //SetTextColor(hdc, RGB(0x0, 0x0, 0x0));
                    //SetDCBrushColor(hdc, PhpThemeWindowTextColor);
                    //FrameRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                    break;
                case 1: // Old colors
                    SetTextColor(hdc, RGB(255, 69, 0));
                    SetDCBrushColor(hdc, RGB(65, 65, 65));
                    //SetTextColor(hdc, PhpThemeWindowTextColor);
                    //SetDCBrushColor(hdc, PhpThemeWindowBackgroundColor);
                    //FrameRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                    break;
                }

                textRect.left += 10;
                DrawText(
                    hdc,
                    text,
                    returnLength,
                    &textRect,
                    DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE
                    );
                textRect.left -= 10;

                SelectFont(hdc, oldFont);
            }

            EndPaint(WindowHandle, &ps);
        }
        return TRUE;
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

    if (!(context = PhGetWindowContext(WindowHandle, SHRT_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, SHRT_MAX);
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
            POINT pt;
            INT count;
            INT i;

            GetCursorPos(&pt);
            MapWindowPoints(NULL, WindowHandle, &pt, 1);

            count = TabCtrl_GetItemCount(WindowHandle);

            for (i = 0; i < count; i++)
            {
                RECT rect = { 0, 0, 0, 0 };
                TCITEM entry =
                {
                    TCIF_STATE,
                    0,
                    TCIS_HIGHLIGHTED
                };

                TabCtrl_GetItemRect(WindowHandle, i, &rect);
                entry.dwState = PtInRect(&rect, pt) ? TCIS_HIGHLIGHTED : 0;
                TabCtrl_SetItem(WindowHandle, i, &entry);
            }

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
            INT count;
            INT i;

            count = TabCtrl_GetItemCount(WindowHandle);

            for (i = 0; i < count; i++)
            {
                TCITEM entry =
                {
                    TCIF_STATE,
                    0,
                    TCIS_HIGHLIGHTED
                };

                TabCtrl_SetItem(WindowHandle, i, &entry);
            }

            InvalidateRect(WindowHandle, NULL, FALSE);
            context->MouseActive = FALSE;
        }
        break;
    case WM_PAINT:
        {
            RECT itemRect;
            RECT windowRect;
            RECT clientRect;
            PAINTSTRUCT ps;

            if (!BeginPaint(WindowHandle, &ps))
                break;

            GetWindowRect(WindowHandle, &windowRect);
            GetClientRect(WindowHandle, &clientRect);

            TabCtrl_AdjustRect(WindowHandle, FALSE, &clientRect); // Make sure we don't paint in the client area.
            ExcludeClipRect(ps.hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

            windowRect.right -= windowRect.left;
            windowRect.bottom -= windowRect.top;
            windowRect.left = 0;
            windowRect.top = 0;

            clientRect.left = windowRect.left;
            clientRect.top = windowRect.top;
            clientRect.right = windowRect.right;
            clientRect.bottom = windowRect.bottom;

            HDC hdc = CreateCompatibleDC(ps.hdc);
            HDC hdcTab = CreateCompatibleDC(hdc);
            HBITMAP hbm = CreateCompatibleBitmap(ps.hdc, clientRect.right, clientRect.bottom);
            SelectBitmap(hdc, hbm);

            SetBkMode(hdc, TRANSPARENT);
            SelectFont(hdc, PhpTabControlFontHandle);

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                //SetTextColor(hdc, RGB(0x0, 0xff, 0x0));
                SetDCBrushColor(hdc, GetSysColor(COLOR_3DFACE));// PhpThemeWindowTextColor);
                FillRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                break;
            case 1: // Old colors
                //SetTextColor(hdc, PhpThemeWindowTextColor);
                SetDCBrushColor(hdc, RGB(65, 65, 65)); //WindowForegroundColor); // RGB(65, 65, 65));
                FillRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                break;
            }
    
            POINT pt;
            GetCursorPos(&pt);
            MapWindowPoints(NULL, WindowHandle, &pt, 1);

            INT count = TabCtrl_GetItemCount(WindowHandle);
            for (INT i = 0; i < count; i++)
            {
                TabCtrl_GetItemRect(WindowHandle, i, &itemRect);

                if (PtInRect(&itemRect, pt))
                {
                    OffsetRect(&itemRect, 2, 2);

                    switch (PhpThemeColorMode)
                    {
                    case 0: // New colors
                        {
                            if (TabCtrl_GetCurSel(WindowHandle) == i)
                            {
                                SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
                                SetDCBrushColor(hdc, RGB(128, 128, 128));
                                FillRect(hdc, &itemRect, GetStockObject(DC_BRUSH));
                            }
                            else
                            {
                                SetTextColor(hdc, PhpThemeWindowTextColor);
                                SetDCBrushColor(hdc, PhpThemeWindowBackgroundColor);
                                FillRect(hdc, &itemRect, GetStockObject(DC_BRUSH));
                            }
                        }
                        break;
                    case 1: // Old colors
                        SetTextColor(hdc, PhpThemeWindowTextColor);
                        SetDCBrushColor(hdc, RGB(128, 128, 128));
                        FillRect(hdc, &itemRect, GetStockObject(DC_BRUSH));
                        break;
                    }

                    //FrameRect(hdc, &itemRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                }
                else
                {
                    OffsetRect(&itemRect, 2, 2);

                    switch (PhpThemeColorMode)
                    {
                    case 0: // New colors
                        {
                            if (TabCtrl_GetCurSel(WindowHandle) == i)
                            {
                                SetTextColor(hdc, RGB(0x0, 0x0, 0x0));
                                SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff));
                                FillRect(hdc, &itemRect, GetStockObject(DC_BRUSH));
                            }
                            else
                            {
                                SetTextColor(hdc, RGB(0, 0, 0));
                                SetDCBrushColor(hdc, GetSysColor(COLOR_3DFACE));// PhpThemeWindowTextColor);
                                FillRect(hdc, &itemRect, GetStockObject(DC_BRUSH));
                            }
                        }
                        break;
                    case 1: // Old colors
                        {
                            if (TabCtrl_GetCurSel(WindowHandle) == i)
                            {
                                SetTextColor(hdc, PhpThemeWindowTextColor);
                                SetDCBrushColor(hdc, PhpThemeWindowForegroundColor);
                                FillRect(hdc, &itemRect, GetStockObject(DC_BRUSH));
                            }
                            else
                            {
                                SetTextColor(hdc, PhpThemeWindowTextColor);
                                SetDCBrushColor(hdc, PhpThemeWindowBackgroundColor);
                                FillRect(hdc, &itemRect, GetStockObject(DC_BRUSH));
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
                            hdc,
                            tabItem.pszText,
                            (UINT)PhCountStringZ(tabItem.pszText),
                            &itemRect,
                            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                            );
                    }
                }
            }

            BitBlt(ps.hdc, 0, 0, clientRect.right, clientRect.bottom, hdc, 0, 0, SRCCOPY);

            DeleteDC(hdcTab);
            DeleteDC(hdc);
            DeleteBitmap(hbm);
            EndPaint(WindowHandle, &ps);
        }
        goto DefaultWndProc;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(WindowHandle, uMsg, wParam, lParam);
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

    if (!(context = PhGetWindowContext(WindowHandle, SHRT_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, SHRT_MAX);
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

            PhFree(context);
        }
        break;
    //case WM_ERASEBKGND:
    //    return 1;
    case WM_MOUSEMOVE:
        {
            //POINT pt;
            //RECT rcItem;
            //TCITEM tcItem;
            //tcItem.mask = TCIF_STATE;
            //tcItem.dwStateMask = TCIS_HIGHLIGHTED;
            //GetCursorPos(&pt);
            //MapWindowPoints(NULL, WindowHandle, &pt, 1);
            //int nCount = TabCtrl_GetItemCount(WindowHandle);
            //for (int i = 0; i < nCount; i++)
            //{
            //    TabCtrl_GetItemRect(WindowHandle, i, &rcItem);
            //    tcItem.dwState = PtInRect(&rcItem, pt) ? TCIS_HIGHLIGHTED : 0;
            //    TabCtrl_SetItem(WindowHandle, i, &tcItem);
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
    case WM_CONTEXTMENU:
        {
            LRESULT result = CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);

            InvalidateRect(WindowHandle, NULL, TRUE);

            return result;
        }
        break;
    case WM_MOUSELEAVE:
        {
            // TCITEM tcItem = { TCIF_STATE, 0, TCIS_HIGHLIGHTED };
            //// int nCount = TabCtrl_GetItemCount(WindowHandle);
            // for (int i = 0; i < nCount; i++)
            // {
            //     //TabCtrl_SetItem(WindowHandle, i, &tcItem);
            // }

            InvalidateRect(WindowHandle, NULL, TRUE);
            context->MouseActive = FALSE;
        }
        break;
    case WM_PAINT:
        {
            //HTHEME hTheme = OpenThemeData(WindowHandle, L"HEADER");
            WCHAR szText[MAX_PATH];
            RECT clientRect;
            HDITEM tcItem;
            PAINTSTRUCT ps;

            //InvalidateRect(WindowHandle, NULL, FALSE);
            BeginPaint(WindowHandle, &ps);
            GetClientRect(WindowHandle, &clientRect);

            HDC hdc = CreateCompatibleDC(ps.hdc);
            HDC hdcTab = CreateCompatibleDC(hdc);
            HBITMAP hbm = CreateCompatibleBitmap(ps.hdc, clientRect.right, clientRect.bottom);
            SelectBitmap(hdc, hbm);

            if (!PhpHeaderFontHandle)
            {
                NONCLIENTMETRICS metrics = { sizeof(metrics) };
                if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
                {
                    PhpHeaderFontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }
            SelectFont(hdc, PhpHeaderFontHandle);

            SetBkMode(hdc, TRANSPARENT);

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                SetTextColor(hdc, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor(hdc, PhpThemeWindowTextColor);
                FillRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                break;
            case 1: // Old colors
                SetTextColor(hdc, PhpThemeWindowTextColor);
                SetDCBrushColor(hdc, RGB(65, 65, 65)); //WindowForegroundColor); // RGB(65, 65, 65));
                FillRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                break;
            }

            INT nCount = Header_GetItemCount(WindowHandle);

            for (INT i = 0; i < nCount; i++)
            {
                RECT headerRect;
                POINT pt;

                Header_GetItemRect(WindowHandle, i, &headerRect);

                GetCursorPos(&pt);
                MapWindowPoints(NULL, WindowHandle, &pt, 1);

                if (PtInRect(&headerRect, pt))
                {
                    switch (PhpThemeColorMode)
                    {
                    case 0: // New colors
                        SetTextColor(hdc, PhpThemeWindowTextColor);
                        SetDCBrushColor(hdc, PhpThemeWindowBackgroundColor);
                        FillRect(hdc, &headerRect, GetStockObject(DC_BRUSH));
                        break;
                    case 1: // Old colors
                        SetTextColor(hdc, PhpThemeWindowTextColor);
                        SetDCBrushColor(hdc, RGB(128, 128, 128));
                        FillRect(hdc, &headerRect, GetStockObject(DC_BRUSH));
                        break;
                    }

                    //FrameRect(hdc, &headerRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                }
                else
                {
                    switch (PhpThemeColorMode)
                    {
                    case 0: // New colors
                        SetTextColor(hdc, RGB(0x0, 0x0, 0x0));
                        SetDCBrushColor(hdc, PhpThemeWindowTextColor);
                        FillRect(hdc, &headerRect, GetStockObject(DC_BRUSH));
                        break;
                    case 1: // Old colors
                        SetTextColor(hdc, PhpThemeWindowTextColor);
                        SetDCBrushColor(hdc, PhpThemeWindowBackgroundColor);
                        FillRect(hdc, &headerRect, GetStockObject(DC_BRUSH));
                        break;
                    }

                    //FrameRect(hdc, &headerRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                }

                ZeroMemory(&tcItem, sizeof(HDITEM));
                tcItem.mask = HDI_TEXT | HDI_IMAGE | HDI_STATE;
                tcItem.pszText = szText;
                tcItem.cchTextMax = MAX_PATH;

                Header_GetItem(WindowHandle, i, &tcItem);

                DrawText(
                    hdc,
                    tcItem.pszText,
                    -1,
                    &headerRect,
                    DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_HIDEPREFIX
                    );
            }

            BitBlt(ps.hdc, 0, 0, clientRect.right, clientRect.bottom, hdc, 0, 0, SRCCOPY);

            DeleteDC(hdcTab);
            DeleteDC(hdc);
            DeleteBitmap(hbm);
            EndPaint(WindowHandle, &ps);
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

    if (!(context = PhGetWindowContext(WindowHandle, SHRT_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, SHRT_MAX);
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
            InvalidateRect(WindowHandle, NULL, FALSE);
            context->MouseActive = FALSE;
        }
        break;
    case WM_PAINT:
        {
            RECT clientRect;
            PAINTSTRUCT ps;
            INT blockCoord[128];
            INT blockCount = (INT)SendMessage(WindowHandle, (UINT)SB_GETPARTS, (WPARAM)ARRAYSIZE(blockCoord), (WPARAM)blockCoord);

            // InvalidateRect(WindowHandle, NULL, FALSE);
            BeginPaint(WindowHandle, &ps);
            GetClientRect(WindowHandle, &clientRect);

            HDC hdc = CreateCompatibleDC(ps.hdc);
            HBITMAP hbm = CreateCompatibleBitmap(ps.hdc, clientRect.right, clientRect.bottom);
            SelectBitmap(hdc, hbm);

            SetBkMode(hdc, TRANSPARENT);

            if (!PhpStatusBarFontHandle)
            {
                NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

                if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
                {
                    PhpStatusBarFontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }

            SelectFont(hdc, PhpStatusBarFontHandle);

            switch (PhpThemeColorMode)
            {
            case 0: // New colors
                SetTextColor(hdc, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor(hdc, GetSysColor(COLOR_3DFACE)); // RGB(0xff, 0xff, 0xff));
                FillRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                break;
            case 1: // Old colors
                SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
                SetDCBrushColor(hdc, RGB(65, 65, 65)); //RGB(28, 28, 28)); // RGB(65, 65, 65));
                FillRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                break;
            }

            for (INT i = 0; i < blockCount; i++)
            {
                RECT blockRect = { 0, 0, 0, 0 };
                WCHAR buffer[MAX_PATH] = L"";

                if (!SendMessage(WindowHandle, SB_GETRECT, (WPARAM)i, (WPARAM)&blockRect))
                    continue;
                if (!SendMessage(WindowHandle, SB_GETTEXT, (WPARAM)i, (LPARAM)buffer))
                    continue;

                POINT pt;
                GetCursorPos(&pt);
                MapWindowPoints(NULL, WindowHandle, &pt, 1);

                if (PtInRect(&blockRect, pt))
                {
                    switch (PhpThemeColorMode)
                    {
                    case 0: // New colors
                        SetTextColor(hdc, PhpThemeWindowTextColor);
                        SetDCBrushColor(hdc, PhpThemeWindowBackgroundColor);
                        FillRect(hdc, &blockRect, GetStockObject(DC_BRUSH));
                        break;
                    case 1: // Old colors
                        SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
                        SetDCBrushColor(hdc, RGB(128, 128, 128));
                        FillRect(hdc, &blockRect, GetStockObject(DC_BRUSH));
                        //FrameRect(hdc, &blockRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                        break;
                    }
                }
                else
                {
                    switch (PhpThemeColorMode)
                    {
                    case 0: // New colors
                        SetTextColor(hdc, RGB(0x0, 0x0, 0x0));
                        SetDCBrushColor(hdc, GetSysColor(COLOR_3DFACE)); // RGB(0xff, 0xff, 0xff));
                        FillRect(hdc, &blockRect, GetStockObject(DC_BRUSH));
                        //FrameRect(hdc, &blockRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                        break;
                    case 1: // Old colors
                        SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
                        SetDCBrushColor(hdc, RGB(64, 64, 64));
                        FillRect(hdc, &blockRect, GetStockObject(DC_BRUSH));
                        //FrameRect(hdc, &blockRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                        break;
                    }
                }

                DrawText(
                    hdc,
                    buffer,
                    -1,
                    &blockRect,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                    );
            }

            {
                RECT sizeGripRect;

                sizeGripRect.left = clientRect.right - GetSystemMetrics(SM_CXHSCROLL);
                sizeGripRect.top = clientRect.bottom - GetSystemMetrics(SM_CYVSCROLL);
                sizeGripRect.right = clientRect.right;
                sizeGripRect.bottom = clientRect.bottom;

                if (context->StatusThemeData)
                    DrawThemeBackground(context->StatusThemeData, hdc, SP_GRIPPER, 0, &sizeGripRect, &sizeGripRect);
                else
                    DrawFrameControl(hdc, &sizeGripRect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
            }

            BitBlt(ps.hdc, 0, 0, clientRect.right, clientRect.bottom, hdc, 0, 0, SRCCOPY);

            DeleteDC(hdc);
            DeleteBitmap(hbm);
            EndPaint(WindowHandle, &ps);
        }
        goto DefaultWndProc;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(WindowHandle, uMsg, wParam, lParam);
}
