/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2012-2022
 *
 */

#include <peview.h>
#include <vssym32.h>
//#include <wincodec.h>

typedef struct _EDIT_CONTEXT
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG ThemeSupport : 1;
            ULONG Hot : 1;
            ULONG ButtonHot : 1;
            ULONG Pushed : 1;
            ULONG HotTrack : 1;
            ULONG ColorMode : 8;
            ULONG Spare : 19;
        };
    };

    LONG CXWidth;
    INT CXBorder;
    INT ImageWidth;
    INT ImageHeight;
    WNDPROC DefaultWindowProc;
    HFONT WindowFont;
    HIMAGELIST ImageListHandle;
    PPH_STRING CueBannerText;
} EDIT_CONTEXT, *PEDIT_CONTEXT;

VOID PhpSearchFreeTheme(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    NOTHING;
}

VOID PhpSearchInitializeFont(
    _Inout_ PEDIT_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    LOGFONT logFont;
    LONG dpiValue;

    if (Context->WindowFont)
        DeleteFont(Context->WindowFont);

    dpiValue = PhGetWindowDpi(WindowHandle);

    if (!PhGetSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, dpiValue))
        return;

    Context->WindowFont = CreateFont(
        -PhMultiplyDivideSigned(10, dpiValue, 72),
        0,
        0,
        0,
        FW_MEDIUM,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY | ANTIALIASED_QUALITY,
        DEFAULT_PITCH,
        logFont.lfFaceName
        );

    SetWindowFont(WindowHandle, Context->WindowFont, TRUE);
}

VOID PhpSearchInitializeTheme(
    _Inout_ PEDIT_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    LONG dpiValue;
    INT borderX;

    dpiValue = PhGetWindowDpi(WindowHandle);

    Context->CXWidth = PhGetDpi(20, dpiValue);
    Context->ColorMode = PhGetIntegerSetting(L"GraphColorMode");

    borderX = PhGetSystemMetrics(SM_CXBORDER, dpiValue);

    if (PhIsThemeActive())
    {
        HTHEME themeDataHandle;

        if (themeDataHandle = PhOpenThemeData(WindowHandle, VSCLASS_EDIT, dpiValue))
        {
            //IsThemePartDefined_I(themeDataHandle, EP_EDITBORDER_NOSCROLL, EPSHV_NORMAL);

            if (!PhGetThemeInt(
                themeDataHandle,
                EP_EDITBORDER_NOSCROLL,
                EPSHV_NORMAL,
                TMT_BORDERSIZE,
                &Context->CXBorder
                ))
            {
                Context->CXBorder = borderX * 2;
            }

            PhCloseThemeData(themeDataHandle);
        }
        else
        {
            Context->CXBorder = borderX * 2;
        }
    }
    else
    {
        Context->CXBorder = borderX * 2;
    }
}

VOID PhpSearchInitializeImages(
    _Inout_ PEDIT_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    HBITMAP bitmap;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(WindowHandle);

    Context->ImageWidth = PhGetSystemMetrics(SM_CXSMICON, dpiValue) + PhGetDpi(4, dpiValue);
    Context->ImageHeight = PhGetSystemMetrics(SM_CYSMICON, dpiValue) + PhGetDpi(4, dpiValue);

    if (Context->ImageListHandle) PhImageListDestroy(Context->ImageListHandle);
    Context->ImageListHandle = PhImageListCreate(
        Context->ImageWidth,
        Context->ImageHeight,
        ILC_MASK | ILC_COLOR32,
        2,
        0
        );
    PhImageListSetImageCount(Context->ImageListHandle, 2);

    if (bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight))
    {
        PhImageListReplace(Context->ImageListHandle, 0, bitmap, NULL);
        DeleteBitmap(bitmap);
    }
    else
    {
        PhSetImageListBitmap(Context->ImageListHandle, 0, PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_BMP));
    }

    if (bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight))
    {
        PhImageListReplace(Context->ImageListHandle, 1, bitmap, NULL);
        DeleteBitmap(bitmap);
    }
    else
    {
        PhSetImageListBitmap(Context->ImageListHandle, 1, PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_BMP));
    }
}

VOID PhpSearchGetButtonRect(
    _Inout_ PEDIT_CONTEXT Context,
    _Inout_ PRECT ButtonRect
    )
{
    ButtonRect->left = (ButtonRect->right - Context->CXWidth) - Context->CXBorder - 1; // offset left border by 1
    ButtonRect->bottom -= Context->CXBorder;
    ButtonRect->right -= Context->CXBorder;
    ButtonRect->top += Context->CXBorder;
}

VOID PhpSearchDrawButton(
    _Inout_ PEDIT_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ RECT WindowRect,
    _In_ RECT ButtonRect
    )
{
    if (Context->ThemeSupport) // HACK
    {
        if (GetFocus() == WindowHandle)
        {
            //switch (Context->ColorMode)
            //{
            //case 0: // New colors
            //    SetDCBrushColor(Hdc, RGB(0, 0, 0));
            //    break;
            //case 1: // Old colors
            SetDCBrushColor(Hdc, RGB(65, 65, 65));
            //    break;
            //}

            SelectBrush(Hdc, GetStockBrush(DC_BRUSH));
            PatBlt(Hdc, WindowRect.left, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(Hdc, WindowRect.right - 1, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(Hdc, WindowRect.left, WindowRect.top, WindowRect.right - WindowRect.left, 1, PATCOPY);
            PatBlt(Hdc, WindowRect.left, WindowRect.bottom - 1, WindowRect.right - WindowRect.left, 1, PATCOPY);

            //switch (Context->ColorMode)
            //{
            //case 0: // New colors
            //    SetDCBrushColor(Hdc, RGB(0xff, 0xff, 0xff));
            //    break;
            //case 1: // Old colors
            SetDCBrushColor(Hdc, RGB(60, 60, 60));
            //    break;
            //}

            SelectBrush(Hdc, GetStockBrush(DC_BRUSH));
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(Hdc, WindowRect.right - 2, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.top + 1, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.bottom - 2, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
        }
        else
        {
            //switch (Context->ColorMode)
            //{
            //case 0: // New colors
            //    SetDCBrushColor(Hdc, RGB(0, 0, 0));
            //    break;
            //case 1: // Old colors
            SetDCBrushColor(Hdc, RGB(65, 65, 65));
            //    break;
            //}

            SelectBrush(Hdc, GetStockBrush(DC_BRUSH));
            PatBlt(Hdc, WindowRect.left, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(Hdc, WindowRect.right - 1, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(Hdc, WindowRect.left, WindowRect.top, WindowRect.right - WindowRect.left, 1, PATCOPY);
            PatBlt(Hdc, WindowRect.left, WindowRect.bottom - 1, WindowRect.right - WindowRect.left, 1, PATCOPY);

            //switch (Context->ColorMode)
            //{
            //case 0: // New colors
            //    SetDCBrushColor(Hdc, RGB(0xff, 0xff, 0xff));
            //    break;
            //case 1: // Old colors
            SetDCBrushColor(Hdc, RGB(60, 60, 60));
            //    break;
            //}

            SelectBrush(Hdc, GetStockBrush(DC_BRUSH));
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(Hdc, WindowRect.right - 2, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.top + 1, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.bottom - 2, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
        }
    }

    if (Context->Pushed)
    {
        if (Context->ThemeSupport)
        {
            switch (Context->ColorMode)
            {
            case 0: // New colors
                SetDCBrushColor(Hdc, RGB(153, 209, 255));
                FillRect(Hdc, &ButtonRect, GetStockBrush(DC_BRUSH));
                break;
            case 1: // Old colors
                //SetTextColor(Hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetDCBrushColor(Hdc, RGB(99, 99, 99));
                FillRect(Hdc, &ButtonRect, GetStockBrush(DC_BRUSH));
                break;
            }
        }
        else
        {
            SetDCBrushColor(Hdc, RGB(153, 209, 255));
            FillRect(Hdc, &ButtonRect, GetStockBrush(DC_BRUSH));
            //FrameRect(bufferDc, &bufferRect, CreateSolidBrush(RGB(0xff, 0, 0)));
        }
    }
    else if (Context->ButtonHot)
    {
        if (Context->ThemeSupport)
        {
            switch (Context->ColorMode)
            {
            case 0: // New colors
                SetDCBrushColor(Hdc, RGB(205, 232, 255));
                FillRect(Hdc, &ButtonRect, GetStockBrush(DC_BRUSH));
                break;
            case 1: // Old colors
                //SetTextColor(Hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetDCBrushColor(Hdc, RGB(78, 78, 78));
                FillRect(Hdc, &ButtonRect, GetStockBrush(DC_BRUSH));
                break;
            }
        }
        else
        {
            SetDCBrushColor(Hdc, RGB(205, 232, 255));
            FillRect(Hdc, &ButtonRect, GetStockBrush(DC_BRUSH));
            //FrameRect(bufferDc, &bufferRect, CreateSolidBrush(RGB(38, 160, 218)));
        }
    }
    else
    {
        if (Context->ThemeSupport)
        {
            switch (Context->ColorMode)
            {
            case 0: // New colors
                FillRect(Hdc, &ButtonRect, GetSysColorBrush(COLOR_WINDOW));
                break;
            case 1: // Old colors
                //SetTextColor(Hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetDCBrushColor(Hdc, RGB(60, 60, 60));
                FillRect(Hdc, &ButtonRect, GetStockBrush(DC_BRUSH));
                break;
            }

        }
        else
        {
            FillRect(Hdc, &ButtonRect, GetSysColorBrush(COLOR_WINDOW));
        }
    }

    if (Edit_GetTextLength(WindowHandle) > 0)
    {
        PhImageListDrawIcon(
            Context->ImageListHandle,
            0,
            Hdc,
            ButtonRect.left + 1 /*offset*/ + ((ButtonRect.right - ButtonRect.left) - Context->ImageWidth) / 2,
            ButtonRect.top + ((ButtonRect.bottom - ButtonRect.top) - Context->ImageHeight) / 2,
            ILD_TRANSPARENT,
            FALSE
            );
    }
    else
    {
        PhImageListDrawIcon(
            Context->ImageListHandle,
            1,
            Hdc,
            ButtonRect.left + 2 /*offset*/ + ((ButtonRect.right - ButtonRect.left) - Context->ImageWidth) / 2,
            ButtonRect.top + 1 /*offset*/ + ((ButtonRect.bottom - ButtonRect.top) - Context->ImageHeight) / 2,
            ILD_TRANSPARENT,
            FALSE
            );
    }
}

LRESULT CALLBACK PhpSearchWndSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PEDIT_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hWnd, SHRT_MAX)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            PhpSearchFreeTheme(context);

            if (context->WindowFont)
            {
                DeleteFont(context->WindowFont);
                context->WindowFont = NULL;
            }

            if (context->ImageListHandle)
            {
                PhImageListDestroy(context->ImageListHandle);
                context->ImageListHandle = NULL;
            }

            if (context->CueBannerText)
            {
                PhDereferenceObject(context->CueBannerText);
                context->CueBannerText = NULL;
            }

            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hWnd, SHRT_MAX);
            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_DPICHANGED:
        {
            PhpSearchFreeTheme(context);
            PhpSearchInitializeTheme(context, hWnd);
            PhpSearchInitializeFont(context, hWnd);
            PhpSearchInitializeImages(context, hWnd);

            // Refresh the non-client area.
            SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

            // Force the edit control to update its non-client area.
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_NCCALCSIZE:
        {
            LPNCCALCSIZE_PARAMS ncCalcSize = (NCCALCSIZE_PARAMS*)lParam;

            // Let Windows handle the non-client defaults.
            CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);

            // Deflate the client area to accommodate the custom button.
            ncCalcSize->rgrc[0].right -= context->CXWidth;
        }
        return 0;
    case WM_NCPAINT:
        {
            RECT windowRect;
            RECT buttonRect;
            HDC hdc;
            ULONG flags;
            HRGN updateRegion;

            updateRegion = (HRGN)wParam;

            if (updateRegion == (HRGN)1) // HRGN_FULL
                updateRegion = NULL;

            flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | DCX_USESTYLE;

            if (updateRegion)
                flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

            if (hdc = GetDCEx(hWnd, updateRegion, flags))
            {
                HDC bufferDc;
                RECT bufferRect;
                HBITMAP bufferBitmap;
                HBITMAP oldBufferBitmap;

                // Get the screen coordinates of the window.
                GetWindowRect(hWnd, &windowRect);
                // Adjust the coordinates (start from 0,0).
                PhOffsetRect(&windowRect, -windowRect.left, -windowRect.top);
                buttonRect = windowRect;
                // Get the position of the inserted button.
                PhpSearchGetButtonRect(context, &buttonRect);
                // Exclude client area.
                ExcludeClipRect(
                    hdc,
                    windowRect.left + 2,
                    windowRect.top + 2,
                    windowRect.right - context->CXWidth - 2,
                    windowRect.bottom - 2
                    );

                bufferRect.left = 0;
                bufferRect.top = 0;
                bufferRect.right = windowRect.right - windowRect.left;
                bufferRect.bottom = windowRect.bottom - windowRect.top;

                bufferDc = CreateCompatibleDC(hdc);
                bufferBitmap = CreateCompatibleBitmap(hdc, bufferRect.right, bufferRect.bottom);
                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

                if (GetFocus() == hWnd)
                {
                    FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_HOTLIGHT));
                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_WINDOW));
                }
                else if (context->Hot || context->ButtonHot)
                {
                    if (context->ThemeSupport)
                    {
                        SetDCBrushColor(bufferDc, RGB(0x8f, 0x8f, 0x8f));
                        FrameRect(bufferDc, &windowRect, GetStockBrush(DC_BRUSH));
                    }
                    else
                    {
                        SetDCBrushColor(bufferDc, RGB(43, 43, 43));
                        FrameRect(bufferDc, &windowRect, GetStockBrush(DC_BRUSH));
                    }

                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_WINDOW));
                }
                else
                {
                    if (context->ThemeSupport)
                    {
                        //SetDCBrushColor(bufferDc, RGB(43, 43, 43));
                        FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_WINDOWFRAME));
                    }
                    else
                    {
                        FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_WINDOWFRAME));
                    }

                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(bufferDc, &windowRect, GetSysColorBrush(COLOR_WINDOW));
                }

                // Draw the button.
                PhpSearchDrawButton(context, hWnd, bufferDc, windowRect, buttonRect);

                BitBlt(hdc, bufferRect.left, bufferRect.top, bufferRect.right, bufferRect.bottom, bufferDc, 0, 0, SRCCOPY);
                SelectBitmap(bufferDc, oldBufferBitmap);
                DeleteBitmap(bufferBitmap);
                DeleteDC(bufferDc);

                ReleaseDC(hWnd, hdc);
            }
        }
        return 0;
    case WM_NCHITTEST:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted button.
            PhpSearchGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
            {
                return HTBORDER;
            }
        }
        break;
    case WM_NCLBUTTONDOWN:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted button.
            PhpSearchGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
            {
                context->Pushed = TRUE;

                SetCapture(hWnd);

                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted button.
            PhpSearchGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
            {
                // Forward click notification.
                //SendMessage(GetParent(context->WindowHandle), WM_COMMAND, MAKEWPARAM(context->CommandID, BN_CLICKED), 0);

                SetFocus(hWnd);
                PhSetWindowText(hWnd, L"");
            }

            if (GetCapture() == hWnd)
            {
                context->Pushed = FALSE;
                ReleaseCapture();
            }

            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_CUT:
    case WM_CLEAR:
    case WM_PASTE:
    case WM_UNDO:
    case WM_KEYUP:
    case WM_SETTEXT:
    case WM_KILLFOCUS:
        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        break;
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
        {
            PhpSearchFreeTheme(context);
            PhpSearchInitializeTheme(context, hWnd);
            PhpSearchInitializeFont(context, hWnd);

            // Reset the client area margins.
            SendMessage(hWnd, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));

            // Refresh the non-client area.
            SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

            // Force the edit control to update its non-client area.
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);
            context->Hot = PtInRect(&windowRect, windowPoint);

            // Get the position of the inserted button.
            PhpSearchGetButtonRect(context, &windowRect);
            context->ButtonHot = PtInRect(&windowRect, windowPoint);

            if ((wParam & MK_LBUTTON) && GetCapture() == hWnd)
            {
                // Check that the mouse is within the inserted button.
                context->Pushed = PtInRect(&windowRect, windowPoint);
            }

            // Check that the mouse is within the inserted button.
            if (!context->HotTrack)
            {
                TRACKMOUSEEVENT trackMouseEvent;
                trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                trackMouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
                trackMouseEvent.hwndTrack = hWnd;
                trackMouseEvent.dwHoverTime = 0;

                context->HotTrack = TRUE;

                TrackMouseEvent(&trackMouseEvent);
            }

            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_MOUSELEAVE:
    case WM_NCMOUSELEAVE:
        {
            POINT windowPoint;
            RECT windowRect;

            context->HotTrack = FALSE;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);
            context->Hot = PtInRect(&windowRect, windowPoint);

            // Get the position of the inserted button.
            PhpSearchGetButtonRect(context, &windowRect);
            context->ButtonHot = PtInRect(&windowRect, windowPoint);

            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_PAINT:
        {
            if (
                PhIsNullOrEmptyString(context->CueBannerText) ||
                GetFocus() == hWnd ||
                CallWindowProc(oldWndProc, hWnd, WM_GETTEXTLENGTH, 0, 0) > 0 // Edit_GetTextLength
                )
            {
                return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
            }

            HDC hdc = (HDC)wParam ? (HDC)wParam : GetDC(hWnd);

            if (hdc)
            {
                HDC bufferDc;
                RECT clientRect;
                HFONT oldFont;
                HBITMAP bufferBitmap;
                HBITMAP oldBufferBitmap;

                GetClientRect(hWnd, &clientRect);

                bufferDc = CreateCompatibleDC(hdc);
                bufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

                SetBkMode(bufferDc, TRANSPARENT);

                if (context->ThemeSupport)
                {
                    SetTextColor(bufferDc, RGB(170, 170, 170));
                    SetDCBrushColor(bufferDc, RGB(60, 60, 60));
                    FillRect(bufferDc, &clientRect, GetStockBrush(DC_BRUSH));
                }
                else
                {
                    SetTextColor(bufferDc, GetSysColor(COLOR_GRAYTEXT));
                    FillRect(bufferDc, &clientRect, GetSysColorBrush(COLOR_WINDOW));
                }

                oldFont = SelectFont(bufferDc, GetWindowFont(hWnd));
                clientRect.left += 2;
                DrawText(
                    bufferDc,
                    context->CueBannerText->Buffer,
                    (UINT)context->CueBannerText->Length / sizeof(WCHAR),
                    &clientRect,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP
                    );
                clientRect.left -= 2;
                SelectFont(bufferDc, oldFont);

                BitBlt(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, bufferDc, 0, 0, SRCCOPY);
                SelectBitmap(bufferDc, oldBufferBitmap);
                DeleteBitmap(bufferBitmap);
                DeleteDC(bufferDc);

                if (!(HDC)wParam)
                {
                    ReleaseDC(hWnd, hdc);
                }
            }

            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_KEYDOWN:
        {
            // Delete previous word for ctrl+backspace (thanks to Katayama Hirofumi MZ) (modified) (dmex)
            if (wParam == VK_BACK && GetAsyncKeyState(VK_CONTROL) < 0)
            {
                UINT textStart = 0;
                UINT textEnd = 0;
                UINT textLength;

                textLength = (UINT)CallWindowProc(oldWndProc, hWnd, WM_GETTEXTLENGTH, 0, 0);
                CallWindowProc(oldWndProc, hWnd, EM_GETSEL, (WPARAM)&textStart, (LPARAM)&textEnd);

                if (textLength > 0 && textStart == textEnd)
                {
                    PWSTR textBuffer;

                    textBuffer = PhAllocateZero((textLength + 1) * sizeof(WCHAR));
                    GetWindowText(hWnd, textBuffer, textLength);

                    for (; 0 < textStart; --textStart)
                    {
                        if (textBuffer[textStart - 1] == L' ' && iswalnum(textBuffer[textStart]))
                        {
                            CallWindowProc(oldWndProc, hWnd, EM_SETSEL, textStart, textEnd);
                            CallWindowProc(oldWndProc, hWnd, EM_REPLACESEL, TRUE, (LPARAM)L"");
                            PhFree(textBuffer);
                            return 1;
                        }
                    }

                    if (textStart == 0)
                    {
                        //CallWindowProc(oldWndProc, hWnd, WM_SETTEXT, 0, (LPARAM)L"");
                        PhSetWindowText(hWnd, L"");
                        PhFree(textBuffer);
                        return 1;
                    }

                    PhFree(textBuffer);
                }
            }
        }
        break;
    case WM_CHAR:
        {
            // Delete previous word for ctrl+backspace (dmex)
            if (wParam == VK_F16 && GetAsyncKeyState(VK_CONTROL) < 0)
                return 1;
        }
        break;
    case EM_SETCUEBANNER:
        {
            PWSTR text = (PWSTR)lParam;

            PhMoveReference(&context->CueBannerText, PhCreateString(text));

            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        return TRUE;
    }

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
}

VOID PvCreateSearchControl(
    _In_ HWND WindowHandle,
    _In_opt_ PWSTR BannerText
    )
{
    PEDIT_CONTEXT context;

    context = PhAllocateZero(sizeof(EDIT_CONTEXT));
    context->ThemeSupport = !!PhGetIntegerSetting(L"EnableThemeSupport"); // HACK
    context->ColorMode = PhGetIntegerSetting(L"GraphColorMode");

    //PhpSearchInitializeTheme(context);
    PhpSearchInitializeImages(context, WindowHandle);

    // Set initial text
    if (BannerText)
        context->CueBannerText = PhCreateString(BannerText);

    // Subclass the Edit control window procedure.
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
    PhSetWindowContext(WindowHandle, SHRT_MAX, context);
    SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)PhpSearchWndSubclassProc);

    // Initialize the theme parameters.
    SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);
}
