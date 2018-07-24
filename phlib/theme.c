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

typedef struct _PHP_THEME_WINDOW_TAB_CONTEXT
{
    WNDPROC DefaultWindowProc;
    BOOLEAN MouseActive;
} PHP_THEME_WINDOW_TAB_CONTEXT, *PPHP_THEME_WINDOW_TAB_CONTEXT;

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

LRESULT PhpWindowThemeDrawListViewGroup(
    _In_ LPNMLVCUSTOMDRAW DrawInfo
    );

COLORREF PhpThemeWindowForegroundColor = RGB(28, 28, 28);
COLORREF PhpThemeWindowBackgroundColor = RGB(64, 64, 64);
COLORREF PhpThemeWindowTextColor = RGB(0xff, 0xff, 0xff);
HFONT PhpTabControlFontHandle = NULL;
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
                        LPNMLVCUSTOMDRAW listViewCustomDraw = (LPNMLVCUSTOMDRAW)customDraw;

                        if (listViewCustomDraw->dwItemType == LVCDI_GROUP)
                        {
                            return PhpWindowThemeDrawListViewGroup(listViewCustomDraw);
                        }
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

LRESULT CALLBACK PhpThemeWindowTabControlWndSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
    )
{
    PPHP_THEME_WINDOW_TAB_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, SHRT_MAX)))
        return 0;

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
        return 1;
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
            RECT clientRect, itemRect;
            TCITEMW tcItem;
            PAINTSTRUCT ps;
            WCHAR szText[MAX_PATH];

            BeginPaint(WindowHandle, &ps);

            GetClientRect(WindowHandle, &clientRect);

            ZeroMemory(&tcItem, sizeof(TCITEMW));
            tcItem.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_STATE;
            tcItem.dwStateMask = TCIS_BUTTONPRESSED | TCIS_HIGHLIGHTED;
            tcItem.pszText = szText;
            tcItem.cchTextMax = MAX_PATH;

            RECT rcSpin = clientRect;
            HWND hWndSpin = GetDlgItem(WindowHandle, 1);
            if (hWndSpin)
            {
                GetWindowRect(hWndSpin, &rcSpin);
                MapWindowPoints(NULL, WindowHandle, (LPPOINT)&rcSpin, 2);
                rcSpin.right = rcSpin.left;
                rcSpin.left = rcSpin.top = 0;
                rcSpin.bottom = clientRect.bottom;
            }

            HDC hdc = CreateCompatibleDC(ps.hdc);
            HBITMAP hbm = CreateCompatibleBitmap(ps.hdc, clientRect.right, clientRect.bottom);
            SelectObject(hdc, hbm);

            SetBkMode(hdc, TRANSPARENT);

            static HFONT fontHandle = NULL;
            if (!fontHandle)
            {
                NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
                if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
                {
                    fontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }

            SelectObject(hdc, fontHandle);

            switch (PhGetIntegerSetting(L"GraphColorMode"))
            {
            case 0: // New colors
                //SetTextColor(hdc, RGB(0x0, 0xff, 0x0));
                SetDCBrushColor(hdc, PhpThemeWindowTextColor);
                FillRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                break;
            case 1: // Old colors
                //SetTextColor(hdc, PhpThemeWindowTextColor);
                SetDCBrushColor(hdc, RGB(65, 65, 65)); //WindowForegroundColor); // RGB(65, 65, 65));
                FillRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                break;
            }
    

            HDC hdcTab = CreateCompatibleDC(hdc);

            INT count = TabCtrl_GetItemCount(WindowHandle);
            for (INT i = 0; i < count; i++)
            {
                TabCtrl_GetItemRect(WindowHandle, i, &itemRect);
                TabCtrl_GetItem(WindowHandle, i, &tcItem);

                POINT pt;
                GetCursorPos(&pt);
                MapWindowPoints(NULL, WindowHandle, &pt, 1);

                if (PtInRect(&itemRect, pt))
                {
                    OffsetRect(&itemRect, 2, 2);

                    switch (PhGetIntegerSetting(L"GraphColorMode"))
                    {
                    case 0: // New colors
                        {
                            if (TabCtrl_GetCurSel(WindowHandle) == i)
                            {
                                SetDCBrushColor(hdc, RGB(128, 128, 128));
                                FillRect(hdc, &itemRect, GetStockObject(DC_BRUSH));
                            }
                            else
                            {
                                //SetTextColor(hdc, PhpThemeWindowTextColor);
                                SetDCBrushColor(hdc, PhpThemeWindowTextColor);
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

                    switch (PhGetIntegerSetting(L"GraphColorMode"))
                    {
                    case 0: // New colors
                        {
                            if (TabCtrl_GetCurSel(WindowHandle) == i)
                            {
                                SetDCBrushColor(hdc, RGB(128, 128, 128));
                                FillRect(hdc, &itemRect, GetStockObject(DC_BRUSH));
                            }
                            else
                            {
                                //SetTextColor(hdc, PhpThemeWindowTextColor);
                                SetDCBrushColor(hdc, PhpThemeWindowTextColor);
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

                DrawText(
                    hdc,
                    tcItem.pszText,
                    -1,
                    &itemRect,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX //| DT_WORD_ELLIPSIS
                    );
            }

            DeleteDC(hdcTab);

            BitBlt(ps.hdc, 0, 0, clientRect.right, clientRect.bottom, hdc, 0, 0, SRCCOPY);

            DeleteDC(hdc);
            DeleteObject(hbm);
            EndPaint(WindowHandle, &ps);

            InvalidateRect(WindowHandle, NULL, FALSE); // HACK
        }
        return DefWindowProc(WindowHandle, uMsg, wParam, lParam);
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowHeaderSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static BOOLEAN MouseActive = FALSE;
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
            //MapWindowPoints(NULL, hWnd, &pt, 1);
            //int nCount = TabCtrl_GetItemCount(hWnd);
            //for (int i = 0; i < nCount; i++)
            //{
            //    TabCtrl_GetItemRect(hWnd, i, &rcItem);
            //    tcItem.dwState = PtInRect(&rcItem, pt) ? TCIS_HIGHLIGHTED : 0;
            //    TabCtrl_SetItem(hWnd, i, &tcItem);
            //}

            if (!MouseActive)
            {
                TRACKMOUSEEVENT trackEvent =
                {
                    sizeof(TRACKMOUSEEVENT),
                    TME_LEAVE,
                    hWnd,
                    0
                };

                TrackMouseEvent(&trackEvent);
                MouseActive = TRUE;
            }

            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

            InvalidateRect(hWnd, NULL, TRUE);

            return result;
        }
        break;
    case WM_MOUSELEAVE:
        {
            // TCITEM tcItem = { TCIF_STATE, 0, TCIS_HIGHLIGHTED };
            //// int nCount = TabCtrl_GetItemCount(hWnd);
            // for (int i = 0; i < nCount; i++)
            // {
            //     //TabCtrl_SetItem(hWnd, i, &tcItem);
            // }

            InvalidateRect(hWnd, NULL, TRUE);
            MouseActive = FALSE;
        }
        break;
    case WM_PAINT:
        {
            //HTHEME hTheme = OpenThemeData(hWnd, L"HEADER");
            WCHAR szText[MAX_PATH + 1];
            RECT rcClient;
            HDITEM tcItem;
            PAINTSTRUCT ps;

            //InvalidateRect(hWnd, NULL, FALSE);
            BeginPaint(hWnd, &ps);

            GetClientRect(hWnd, &rcClient);

            ZeroMemory(&tcItem, sizeof(tcItem));
            tcItem.mask = HDI_TEXT | HDI_IMAGE | HDI_STATE;
            tcItem.pszText = szText;
            tcItem.cchTextMax = MAX_PATH;

            HDC hdc = CreateCompatibleDC(ps.hdc);
            HBITMAP hbm = CreateCompatibleBitmap(ps.hdc, rcClient.right, rcClient.bottom);

            SelectObject(hdc, hbm);

            static HFONT fontHandle = NULL;
            if (!fontHandle)
            {
                NONCLIENTMETRICS metrics = { sizeof(metrics) };
                if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
                {
                    fontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }
            SelectObject(hdc, fontHandle);

            SetBkMode(hdc, TRANSPARENT);

            switch (PhGetIntegerSetting(L"GraphColorMode"))
            {
            case 0: // New colors
                SetTextColor(hdc, RGB(0x0, 0x0, 0x0));
                SetDCBrushColor(hdc, PhpThemeWindowTextColor);
                FillRect(hdc, &rcClient, GetStockObject(DC_BRUSH));
                break;
            case 1: // Old colors
                SetTextColor(hdc, PhpThemeWindowTextColor);
                SetDCBrushColor(hdc, RGB(65, 65, 65)); //WindowForegroundColor); // RGB(65, 65, 65));
                FillRect(hdc, &rcClient, GetStockObject(DC_BRUSH));
                break;
            }

            HDC hdcTab = CreateCompatibleDC(hdc);

            INT nCount = Header_GetItemCount(hWnd);
            for (INT i = 0; i < nCount; i++)
            {
                RECT headerRect;
                POINT pt;

                Header_GetItemRect(hWnd, i, &headerRect);
                Header_GetItem(hWnd, i, &tcItem);

                GetCursorPos(&pt);
                MapWindowPoints(NULL, hWnd, &pt, 1);

                if (PtInRect(&headerRect, pt))
                {
                    switch (PhGetIntegerSetting(L"GraphColorMode"))
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
                    switch (PhGetIntegerSetting(L"GraphColorMode"))
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

                DrawText(
                    hdc,
                    tcItem.pszText,
                    -1,
                    &headerRect,
                    DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_HIDEPREFIX
                    );
            }

            DeleteDC(hdcTab);

            BitBlt(ps.hdc, 0, 0, rcClient.right, rcClient.bottom, hdc, 0, 0, SRCCOPY);

            DeleteDC(hdc);
            DeleteObject(hbm);
            EndPaint(hWnd, &ps);
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
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
    HWND currentWindow = NULL;

    PhEnumChildWindows(
        WindowHandle,
        0x1000,
        PhpReInitializeWindowThemeEnumChildWindows,
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
        PPHP_THEME_WINDOW_TAB_CONTEXT tabControlContext;

        tabControlContext = PhAllocateZero(sizeof(PHP_THEME_WINDOW_TAB_CONTEXT));
        tabControlContext->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
        PhSetWindowContext(WindowHandle, SHRT_MAX, tabControlContext);
        SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowTabControlWndSubclassProc);

        PhSetWindowStyle(WindowHandle, TCS_OWNERDRAWFIXED, TCS_OWNERDRAWFIXED);

        if (!PhpTabControlFontHandle)
        {
            PhpTabControlFontHandle = CreateFont(
                -(LONG)PhMultiplyDivide(15, PhGlobalDpi, 96),
                0,
                0,
                0,
                FW_SEMIBOLD,
                FALSE,
                FALSE,
                FALSE,
                ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY,
                DEFAULT_PITCH,
                L"Tahoma"
                );
        }

        SendMessage(WindowHandle, WM_SETFONT, (WPARAM)PhpTabControlFontHandle, FALSE);
        InvalidateRect(WindowHandle, NULL, FALSE);
    }
    else if (PhEqualStringZ(className, L"msctls_statusbar32", FALSE))
    {
        NOTHING;
    }
    else if (PhEqualStringZ(className, L"SysHeader32", FALSE))
    {
        WNDPROC headerWindowProc;

        headerWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
        PhSetWindowContext(WindowHandle, SHRT_MAX, headerWindowProc);
        SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowHeaderSubclassProc);
    }
    else if (PhEqualStringZ(className, L"SysListView32", FALSE))
    {
        PhSetWindowStyle(WindowHandle, WS_BORDER, 0);
        PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, 0);

        ListView_SetBkColor(WindowHandle, RGB(30, 30, 30));
        ListView_SetTextBkColor(WindowHandle, RGB(30, 30, 30));
        ListView_SetTextColor(WindowHandle, RGB(0xff, 0xff, 0xff));
    }
    else if (PhEqualStringZ(className, L"ScrollBar", FALSE))
    {
        NOTHING;
    }
    else if (PhEqualStringZ(className, L"PhTreeNew", FALSE))
    {
        PhSetWindowStyle(WindowHandle, WS_BORDER, 0);
        PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, 0);
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

LRESULT PhpWindowThemeDrawListViewGroup(
    _In_ LPNMLVCUSTOMDRAW DrawInfo
    )
{
    switch (DrawInfo->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            SetBkMode(DrawInfo->nmcd.hdc, TRANSPARENT);

            switch (PhGetIntegerSetting(L"GraphColorMode"))
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

            static HFONT fontHandle = NULL;
            if (!fontHandle)
            {
                NONCLIENTMETRICS metrics = { sizeof(metrics) };
                if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
                {
                    metrics.lfMessageFont.lfHeight = -11;
                    metrics.lfMessageFont.lfWeight = FW_BOLD;
                    fontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }
            SelectObject(DrawInfo->nmcd.hdc, fontHandle);

            LVGROUP groupInfo = { sizeof(LVGROUP) };
            groupInfo.mask = LVGF_HEADER;
            if (ListView_GetGroupInfo(DrawInfo->nmcd.hdr.hwndFrom, (ULONG)DrawInfo->nmcd.dwItemSpec, &groupInfo) == -1)
            {
                break;
            }


            SetTextColor(DrawInfo->nmcd.hdc, RGB(255, 69, 0));
            SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(65, 65, 65));

            //GetClientRect(hWnd, &clientRect);
            //clientRect.top += 6;
            //FrameRect(DrawInfo->nmcd.hdc, &clientRect, GetStockObject(DC_BRUSH));
            //clientRect.top -= 6;
            //PPH_STRING windowText = PhGetWindowText(hWnd);
            //clientRect.left += 10;
            DrawInfo->rcText.top += 2;
            DrawInfo->rcText.bottom -= 2;
            FillRect(DrawInfo->nmcd.hdc, &DrawInfo->rcText, GetStockObject(DC_BRUSH));
            DrawInfo->rcText.top -= 2;
            DrawInfo->rcText.bottom += 2;

            SetTextColor(DrawInfo->nmcd.hdc, RGB(255, 69, 0));
            SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(65, 65, 65));

            DrawInfo->rcText.left += 10;
            DrawText(
                DrawInfo->nmcd.hdc,
                groupInfo.pszHeader,
                -1,
                &DrawInfo->rcText,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                );
            DrawInfo->rcText.left -= 10;
            //DrawInfo->clrText = RGB(0x0, 0xff, 0);
            //return TBCDRF_USECDCOLORS | CDRF_NEWFONT;
        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}