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

#include <dwmapi.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

BOOLEAN CALLBACK PhpThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    );

BOOLEAN CALLBACK PhpReInitializeWindowThemeEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    );

LRESULT PhpWindowThemeDrawButton(
    _In_ LPNMCUSTOMDRAW DrawInfo
    );

COLORREF PhpThemeWindowForegroundColor = RGB(28, 28, 28);
COLORREF PhpThemeWindowBackgroundColor = RGB(64, 64, 64);
COLORREF PhpThemeWindowTextColor = RGB(0xff, 0xff, 0xff);
HFONT PhpGroupboxFontHandle = NULL;

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

                    if (PhEqualStringZ(className, L"Button", FALSE))
                    {
                        return PhpWindowThemeDrawButton(customDraw);
                    }
                    else if (PhEqualStringZ(className, L"ReBarWindow32", FALSE))
                    {
                        NOTHING;
                    }
                    else if (PhEqualStringZ(className, L"ToolbarWindow32", FALSE))
                    {
                        NOTHING;
                    }
                    else if (PhEqualStringZ(className, L"SysTabControl32", FALSE))
                    {
                        NOTHING;
                    }
                    else if (PhEqualStringZ(className, L"SysListView32", FALSE))
                    {
                        NOTHING;
                    }
                }
            }
        }
        break;
    case WM_CTLCOLOREDIT:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);

            switch (PhGetIntegerSetting(L"GraphColorMode"))
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

            switch (PhGetIntegerSetting(L"GraphColorMode"))
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
    }

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowGroupBoxSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    if (!(oldWndProc = PhGetWindowContext(hWnd, SHRT_MAX)))
        return 0;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hWnd, SHRT_MAX);
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        {
            RECT clientRect;
            HDC hdc;
            PAINTSTRUCT ps;
            PPH_STRING windowText;
            SIZE nameSize;
            RECT textRect;

            if (!(hdc = BeginPaint(hWnd, &ps)))
                break;

            GetClientRect(hWnd, &clientRect);

            SetBkMode(hdc, TRANSPARENT);

            switch (PhGetIntegerSetting(L"GraphColorMode"))
            {
            case 0: // New colors
                SetTextColor(hdc, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff));
                //FillRect(hdc, &windowRect, GetStockObject(DC_BRUSH));
                break;
            case 1: // Old colors
                SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
                SetDCBrushColor(hdc, RGB(0, 0, 0)); // RGB(28, 28, 28)); // RGB(65, 65, 65));
                //FillRect(hdc, &windowRect, GetStockObject(DC_BRUSH));
                break;
            }

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

            SelectObject(hdc, PhpGroupboxFontHandle);
            SetTextColor(hdc, RGB(255, 69, 0));
            SetDCBrushColor(hdc, RGB(65, 65, 65));
            //clientRect.top += 6;
            FrameRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
            //clientRect.top -= 6;

            windowText = PhGetWindowText(hWnd);
            GetTextExtentPoint32(
                hdc, 
                windowText->Buffer, 
                (ULONG)windowText->Length / sizeof(WCHAR), 
                &nameSize
                );

            textRect.left = 0;
            textRect.top = 0;
            textRect.right = clientRect.right;
            textRect.bottom = nameSize.cy;
            FillRect(hdc, &textRect, GetStockObject(DC_BRUSH));

            textRect.left += 10;
            DrawText(
                hdc, 
                windowText->Buffer,
                (ULONG)windowText->Length / sizeof(WCHAR), 
                &textRect, 
                DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE
                );
            textRect.left -= 10;

            PhDereferenceObject(windowText);

            EndPaint(hWnd, &ps);
        }
        return FALSE;
    }

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
}

VOID PhInitializeWindowTheme(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    if (EnableThemeSupport)
    {
        WNDPROC defaultWindowProc;

        defaultWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
        PhSetWindowContext(WindowHandle, SHRT_MAX, defaultWindowProc);
        SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowSubclassProc);

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
    PhEnumChildWindows(
        WindowHandle,
        0x1000,
        PhpReInitializeWindowThemeEnumChildWindows,
        0
        );

    HWND currentWindow = NULL;
    
    do
    {
        if (currentWindow = FindWindowEx(NULL, currentWindow, NULL, NULL))
        {
            ULONG processID = 0;
    
            GetWindowThreadProcessId(currentWindow, &processID);
    
            if (UlongToHandle(processID) == NtCurrentProcessId())
            {
                if (currentWindow != WindowHandle)
                {
                    InvalidateRect(currentWindow, NULL, TRUE);
                }
            }
        }
    } while (currentWindow);

    InvalidateRect(WindowHandle, NULL, FALSE);
}

BOOLEAN CALLBACK PhpThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    WCHAR className[MAX_PATH];

    PhEnumChildWindows(
        WindowHandle,
        0x1000,
        PhpThemeWindowEnumChildWindows,
        0
        );

    if (!GetClassName(WindowHandle, className, RTL_NUMBER_OF(className)))
        className[0] = 0;

    if (PhEqualStringZ(className, L"Button", FALSE))
    {
        ULONG buttonWindowStyle = (ULONG)GetWindowLongPtr(WindowHandle, GWL_STYLE);

        if ((buttonWindowStyle & BS_GROUPBOX) == BS_GROUPBOX)
        {
            WNDPROC groupboxWindowProc;

            groupboxWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
            PhSetWindowContext(WindowHandle, SHRT_MAX, groupboxWindowProc);
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowGroupBoxSubclassProc);
        }
    }

    if (PhEqualStringZ(className, L"Edit", FALSE))
    {
        PhSetWindowStyle(WindowHandle, WS_BORDER, 0);
        PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, 0);
    }
    else if (PhEqualStringZ(className, L"#32770", FALSE))
    {
        PhInitializeWindowTheme(WindowHandle, TRUE);
    }
    else if (PhEqualStringZ(className, L"SysTabControl32", FALSE))
    {
        NOTHING;
    }
    else if (PhEqualStringZ(className, L"msctls_statusbar32", FALSE))
    {
        NOTHING;
    }
    else if (PhEqualStringZ(className, L"SysHeader32", FALSE))
    {
        NOTHING;
    }
    else if (PhEqualStringZ(className, L"SysListView32", FALSE))
    {
        ListView_SetBkColor(WindowHandle, RGB(30, 30, 30));
        ListView_SetTextBkColor(WindowHandle, RGB(30, 30, 30));
        ListView_SetTextColor(WindowHandle, RGB(0xff, 0xff, 0xff));
    }
    else if (PhEqualStringZ(className, L"ScrollBar", FALSE))
    {
        NOTHING;
    }

    return TRUE;
}

BOOLEAN CALLBACK PhpReInitializeWindowThemeEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    ULONG processID = 0;

    PhEnumChildWindows(
        WindowHandle,
        0x1000,
        PhpReInitializeWindowThemeEnumChildWindows,
        0
        );

    GetWindowThreadProcessId(WindowHandle, &processID);

    if (UlongToHandle(processID) == NtCurrentProcessId())
    {
        InvalidateRect(WindowHandle, NULL, FALSE);
    }

    return TRUE;
}

LRESULT PhpWindowThemeDrawButton(
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
            HFONT oldFont;
            COLORREF oldColor;

            buttonText = PhGetWindowText(DrawInfo->hdr.hwndFrom);

            if (isSelected)
            {
                switch (PhGetIntegerSetting(L"GraphColorMode"))
                {
                case 0: // New colors
                    oldColor = SetTextColor(DrawInfo->hdc, RGB(0, 0, 0xff));
                    SetDCBrushColor(DrawInfo->hdc, PhpThemeWindowTextColor);
                    break;
                case 1: // Old colors
                    //SetBkColor(drawInfo->hdc, GetSysColor(COLOR_HIGHLIGHT));
                    oldColor = SetTextColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    SetDCBrushColor(DrawInfo->hdc, RGB(78, 78, 78));
                    FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockObject(DC_BRUSH));
                    break;
                }
            }
            else if (isHighlighted)
            {
                switch (PhGetIntegerSetting(L"GraphColorMode"))
                {
                case 0: // New colors
                    oldColor = SetTextColor(DrawInfo->hdc, RGB(0, 0, 0xff));
                    SetDCBrushColor(DrawInfo->hdc, PhpThemeWindowTextColor);
                    break;
                case 1: // Old colors
                    //SetBkColor(drawInfo->hdc, GetSysColor(COLOR_HIGHLIGHT));
                    oldColor = SetTextColor(DrawInfo->hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    SetDCBrushColor(DrawInfo->hdc, RGB(65, 65, 65));
                    break;
                }

                FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockObject(DC_BRUSH));
            }
            else
            {
                switch (PhGetIntegerSetting(L"GraphColorMode"))
                {
                case 0: // New colors
                    oldColor = SetTextColor(DrawInfo->hdc, RGB(0x0, 0x0, 0x0));
                    SetDCBrushColor(DrawInfo->hdc, PhpThemeWindowTextColor);
                    break;
                case 1: // Old colors
                    //SetBkColor(DrawInfo->hdc, RGB(0xff, 0xff, 0xff));
                    oldColor = SetTextColor(DrawInfo->hdc, PhpThemeWindowTextColor);
                    SetDCBrushColor(DrawInfo->hdc, RGB(42, 42, 42)); // WindowForegroundColor
                    break;
                }

                FillRect(DrawInfo->hdc, &DrawInfo->rc, GetStockObject(DC_BRUSH));
            }

            if ((GetWindowLongPtr(DrawInfo->hdr.hwndFrom, GWL_STYLE) & BS_CHECKBOX) != 0)
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

                SetTextColor(DrawInfo->hdc, oldColor);
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

                if (!(buttonIcon = Static_GetIcon(DrawInfo->hdr.hwndFrom, 0)))
                {
                    buttonIcon = (HICON)SendMessage(DrawInfo->hdr.hwndFrom, BM_GETIMAGE, IMAGE_ICON, 0);
                }

                if (buttonIcon)
                {
                    DrawIconEx(
                        DrawInfo->hdc,
                        bufferRect.left + ((bufferRect.right - bufferRect.left) - 16) / 2,
                        bufferRect.top + ((bufferRect.bottom - bufferRect.top) - 16 ) / 2,
                        buttonIcon, 
                        16,
                        16,
                        0, 
                        NULL,
                        DI_NORMAL
                        );
                }
                else
                {
                    DrawText(
                        DrawInfo->hdc,
                        buttonText->Buffer,
                        (UINT)buttonText->Length / sizeof(WCHAR),
                        &DrawInfo->rc,
                        DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_HIDEPREFIX
                        );
                }
            }

            PhDereferenceObject(buttonText);
        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}
