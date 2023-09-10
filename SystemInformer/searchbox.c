/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2012-2023
 *
 */

#include <phapp.h>
#include <settings.h>
#include <vssym32.h>

typedef struct _PH_SEARCHCONTROL_CONTEXT
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Hot : 1;
            ULONG ButtonHot : 1;
            ULONG Pushed : 1;
            ULONG HotTrack : 1;
            ULONG Spare : 28;
        };
    };

    HWND ParentWindowHandle;
    LONG WindowDpi;

    LONG ButtonWidth;
    INT BorderSize;
    INT ImageWidth;
    INT ImageHeight;
    WNDPROC DefaultWindowProc;
    HFONT WindowFont;
    HIMAGELIST ImageListHandle;
    PPH_STRING CueBannerText;

    HDC BufferedDc;
    HBITMAP BufferedOldBitmap;
    HBITMAP BufferedBitmap;
    RECT BufferedContextRect;

    HBRUSH DCBrush;
    HBRUSH WindowBrush;
} PH_SEARCHCONTROL_CONTEXT, *PPH_SEARCHCONTROL_CONTEXT;

VOID PhSearchControlCreateBufferedContext(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ RECT BufferRect
    )
{
    Context->BufferedDc = CreateCompatibleDC(Hdc);

    if (!Context->BufferedDc)
        return;

    Context->BufferedContextRect = BufferRect;
    Context->BufferedBitmap = CreateCompatibleBitmap(
        Hdc,
        Context->BufferedContextRect.right,
        Context->BufferedContextRect.bottom
        );

    Context->BufferedOldBitmap = SelectBitmap(Context->BufferedDc, Context->BufferedBitmap);
}

VOID PhSearchControlDestroyBufferedContext(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context
    )
{
    if (Context->BufferedDc && Context->BufferedOldBitmap)
    {
        SelectBitmap(Context->BufferedDc, Context->BufferedOldBitmap);
    }

    if (Context->BufferedBitmap)
    {
        DeleteBitmap(Context->BufferedBitmap);
        Context->BufferedBitmap = NULL;
    }

    if (Context->BufferedDc)
    {
        DeleteDC(Context->BufferedDc);
        Context->BufferedDc = NULL;
    }
}

VOID PhSearchControlInitializeFont(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    if (Context->WindowFont)
    {
        DeleteFont(Context->WindowFont);
        Context->WindowFont = NULL;
    }

    Context->WindowFont = PhCreateCommonFont(10, FW_MEDIUM, WindowHandle, Context->WindowDpi);
}

VOID PhSearchControlInitializeTheme(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    LONG borderSize;

    borderSize = PhGetSystemMetrics(SM_CXBORDER, Context->WindowDpi);

    Context->ButtonWidth = PhGetDpi(20, Context->WindowDpi);
    Context->BorderSize = borderSize;
    Context->DCBrush = GetStockBrush(DC_BRUSH);
    Context->WindowBrush = GetSysColorBrush(COLOR_WINDOW);

    if (PhIsThemeActive())
    {
        HTHEME themeHandle;

        if (themeHandle = PhOpenThemeData(WindowHandle, VSCLASS_EDIT, Context->WindowDpi))
        {
            if (PhGetThemeInt(themeHandle, 0, 0, TMT_BORDERSIZE, &borderSize))
            {
                Context->BorderSize = borderSize;
            }

            PhCloseThemeData(themeHandle);
        }
    }
}

VOID PhSearchControlInitializeImages(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    HBITMAP bitmap;

    Context->ImageWidth = PhGetSystemMetrics(SM_CXSMICON, Context->WindowDpi) + PhGetDpi(4, Context->WindowDpi);
    Context->ImageHeight = PhGetSystemMetrics(SM_CYSMICON, Context->WindowDpi) + PhGetDpi(4, Context->WindowDpi);

    if (Context->ImageListHandle)
    {
        PhImageListSetIconSize(
            Context->ImageListHandle,
            Context->ImageWidth,
            Context->ImageHeight
            );
    }
    else
    {
        Context->ImageListHandle = PhImageListCreate(
            Context->ImageWidth,
            Context->ImageHeight,
            ILC_MASK | ILC_COLOR32,
            2, 0
            );
    }
    PhImageListSetImageCount(Context->ImageListHandle, 2);

    if (Context->ImageWidth == 20 && Context->ImageHeight == 20) // Avoids bitmap scaling on startup at default DPI (dmex)
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_SMALL), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
    else
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);

    if (bitmap)
    {
        PhImageListReplace(Context->ImageListHandle, 0, bitmap, NULL);
        DeleteBitmap(bitmap);
    }
    else
    {
        PhSetImageListBitmap(Context->ImageListHandle, 0, PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_BMP));
    }

    if (Context->ImageWidth == 20 && Context->ImageHeight == 20) // Avoids bitmap scaling on startup at default DPI (dmex)
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_SMALL), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);
    else
        bitmap = PhLoadImageFormatFromResource(PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Context->ImageWidth, Context->ImageHeight);

    if (bitmap)
    {
        PhImageListReplace(Context->ImageListHandle, 1, bitmap, NULL);
        DeleteBitmap(bitmap);
    }
    else
    {
        PhSetImageListBitmap(Context->ImageListHandle, 1, PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_BMP));
    }
}

VOID PhSearchControlButtonRect(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _Inout_ PRECT ButtonRect
    )
{
    ButtonRect->left = (ButtonRect->right - Context->ButtonWidth) - (Context->BorderSize + 1);
    ButtonRect->top += Context->BorderSize;
    ButtonRect->right -= Context->BorderSize;
    ButtonRect->bottom -= Context->BorderSize;
}

VOID PhSearchControlThemeChanged(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    PhSearchControlInitializeFont(Context, WindowHandle);
    PhSearchControlInitializeTheme(Context, WindowHandle);
    PhSearchControlInitializeImages(Context, WindowHandle);

    // Reset the client area margins.
    SendMessage(WindowHandle, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));

    // Refresh the non-client area.
    SetWindowPos(WindowHandle, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

    // Force the edit control to update its non-client area.
    RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
}

VOID PhpSearchDrawButton(
    _In_ PPH_SEARCHCONTROL_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ RECT WindowRect,
    _In_ RECT ButtonRect,
    _In_ WNDPROC DefaultWindowProc
    )
{
    if (PhEnableThemeSupport)
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
            SelectBrush(Hdc, Context->DCBrush);
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
            SelectBrush(Hdc, Context->DCBrush);
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
            SelectBrush(Hdc, Context->DCBrush);
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
            SelectBrush(Hdc, Context->DCBrush);
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(Hdc, WindowRect.right - 2, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.top + 1, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
            PatBlt(Hdc, WindowRect.left + 1, WindowRect.bottom - 2, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
        }
    }

    if (Context->Pushed)
    {
        if (PhEnableThemeSupport)
        {
            //switch (Context->ColorMode)
            //{
            //case 0: // New colors
            //    SetDCBrushColor(Hdc, RGB(153, 209, 255));
            //    FillRect(Hdc, &ButtonRect, GetStockBrush(DC_BRUSH));
            //    break;
            //case 1: // Old colors
            //SetTextColor(Hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
            SetDCBrushColor(Hdc, RGB(99, 99, 99));
            FillRect(Hdc, &ButtonRect, Context->DCBrush);
        }
        else
        {
            SetDCBrushColor(Hdc, RGB(153, 209, 255));
            FillRect(Hdc, &ButtonRect, Context->DCBrush);
            //FrameRect(bufferDc, &bufferRect, CreateSolidBrush(RGB(0xff, 0, 0)));
        }
    }
    else if (Context->ButtonHot)
    {
        if (PhEnableThemeSupport)
        {
            //switch (Context->ColorMode)
            //{
            //case 0: // New colors
            //    SetDCBrushColor(Hdc, RGB(205, 232, 255));
            //    FillRect(Hdc, &ButtonRect, GetStockBrush(DC_BRUSH));
            //    break;
            //case 1: // Old colors
            //SetTextColor(Hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
            SetDCBrushColor(Hdc, RGB(78, 78, 78));
            FillRect(Hdc, &ButtonRect, Context->DCBrush);
        }
        else
        {
            SetDCBrushColor(Hdc, RGB(205, 232, 255));
            FillRect(Hdc, &ButtonRect, Context->DCBrush);
            //FrameRect(bufferDc, &bufferRect, CreateSolidBrush(RGB(38, 160, 218)));
        }
    }
    else
    {
        if (PhEnableThemeSupport)
        {
            //switch (Context->ColorMode)
            //{
            //case 0: // New colors
            //    FillRect(Hdc, &ButtonRect, GetSysColorBrush(COLOR_WINDOW));
            //    break;
            //case 1: // Old colors
            //SetTextColor(Hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
            SetDCBrushColor(Hdc, RGB(60, 60, 60));
            FillRect(Hdc, &ButtonRect, Context->DCBrush);
        }
        else
        {
            FillRect(Hdc, &ButtonRect, Context->WindowBrush);
        }
    }

    if ((ULONG)CallWindowProc(DefaultWindowProc, WindowHandle, WM_GETTEXTLENGTH, 0, 0) > 0)
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
    PPH_SEARCHCONTROL_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hWnd, SHRT_MAX)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hWnd, SHRT_MAX);

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

            PhSearchControlDestroyBufferedContext(context);

            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_NCCALCSIZE:
        {
            LPNCCALCSIZE_PARAMS ncCalcSize = (NCCALCSIZE_PARAMS*)lParam;

            // Let Windows handle the non-client defaults.
            CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);

            // Deflate the client area to accommodate the custom button.
            ncCalcSize->rgrc[0].right -= context->ButtonWidth;
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

            if (updateRegion == HRGN_FULL)
                updateRegion = NULL;

            flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | DCX_USESTYLE;

            if (updateRegion)
                flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

            if (hdc = GetDCEx(hWnd, updateRegion, flags))
            {
                RECT bufferRect;

                // Get the screen coordinates of the window.
                GetWindowRect(hWnd, &windowRect);
                // Adjust the coordinates (start from 0,0).
                PhOffsetRect(&windowRect, -windowRect.left, -windowRect.top);
                buttonRect = windowRect;

                // Exclude client area.
                ExcludeClipRect(
                    hdc,
                    windowRect.left + (context->BorderSize + 1),
                    windowRect.top + (context->BorderSize + 1),
                    windowRect.right - context->ButtonWidth - (context->BorderSize + 1),
                    windowRect.bottom - (context->BorderSize + 1)
                    );

                bufferRect.left = 0;
                bufferRect.top = 0;
                bufferRect.right = windowRect.right - windowRect.left;
                bufferRect.bottom = windowRect.bottom - windowRect.top;

                if (context->BufferedDc && (
                    context->BufferedContextRect.right < bufferRect.right ||
                    context->BufferedContextRect.bottom < bufferRect.bottom))
                {
                    PhSearchControlDestroyBufferedContext(context);
                }

                if (!context->BufferedDc)
                {
                    PhSearchControlCreateBufferedContext(context, hdc, bufferRect);
                }

                if (!context->BufferedDc)
                    break;

                if (GetFocus() == hWnd)
                {
                    FrameRect(context->BufferedDc, &windowRect, GetSysColorBrush(COLOR_HOTLIGHT));
                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(context->BufferedDc, &windowRect, context->WindowBrush);
                }
                else if (context->Hot || context->ButtonHot)
                {
                    if (PhEnableThemeSupport)
                    {
                        SetDCBrushColor(context->BufferedDc, RGB(0x8f, 0x8f, 0x8f));
                        FrameRect(context->BufferedDc, &windowRect, context->DCBrush);
                    }
                    else
                    {
                        SetDCBrushColor(context->BufferedDc, RGB(43, 43, 43));
                        FrameRect(context->BufferedDc, &windowRect, context->DCBrush);
                    }

                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(context->BufferedDc, &windowRect, context->WindowBrush);
                }
                else
                {
                    FrameRect(context->BufferedDc, &windowRect, GetSysColorBrush(COLOR_WINDOWFRAME));
                    PhInflateRect(&windowRect, -1, -1);
                    FrameRect(context->BufferedDc, &windowRect, context->WindowBrush);
                }

                PhSearchControlButtonRect(context, &buttonRect);
                PhpSearchDrawButton(
                    context,
                    hWnd,
                    context->BufferedDc,
                    windowRect,
                    buttonRect,
                    oldWndProc
                    );

                BitBlt(
                    hdc,
                    bufferRect.left,
                    bufferRect.top,
                    bufferRect.right,
                    bufferRect.bottom,
                    context->BufferedDc,
                    0,
                    0,
                    SRCCOPY
                    );

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
            PhSearchControlButtonRect(context, &windowRect);

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
            PhSearchControlButtonRect(context, &windowRect);

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
            PhSearchControlButtonRect(context, &windowRect);

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
            PhSearchControlThemeChanged(context, hWnd);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            context->WindowDpi = PhGetWindowDpi(context->ParentWindowHandle);

            PhSearchControlThemeChanged(context, hWnd);
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
            PhSearchControlButtonRect(context, &windowRect);
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
            PhSearchControlButtonRect(context, &windowRect);
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

                if (PhEnableThemeSupport)
                {
                    SetTextColor(bufferDc, RGB(170, 170, 170));
                    SetDCBrushColor(bufferDc, RGB(60, 60, 60));
                    FillRect(bufferDc, &clientRect, context->DCBrush);
                }
                else
                {
                    SetTextColor(bufferDc, GetSysColor(COLOR_GRAYTEXT));
                    FillRect(bufferDc, &clientRect, context->WindowBrush);
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
                INT textStart = 0;
                INT textEnd = 0;
                INT textLength;

                textLength = (INT)CallWindowProc(oldWndProc, hWnd, WM_GETTEXTLENGTH, 0, 0);
                CallWindowProc(oldWndProc, hWnd, EM_GETSEL, (WPARAM)&textStart, (LPARAM)&textEnd);

                if (textLength > 0 && textStart == textEnd)
                {
                    ULONG textBufferLength;
                    WCHAR textBuffer[0x100];

                    if (!NT_SUCCESS(PhGetWindowTextToBuffer(hWnd, 0, textBuffer, RTL_NUMBER_OF(textBuffer), &textBufferLength)))
                    {
                        break;
                    }

                    for (; 0 < textStart; --textStart)
                    {
                        if (textBuffer[textStart - 1] == L' ' && iswalnum(textBuffer[textStart]))
                        {
                            CallWindowProc(oldWndProc, hWnd, EM_SETSEL, textStart, textEnd);
                            CallWindowProc(oldWndProc, hWnd, EM_REPLACESEL, TRUE, (LPARAM)L"");
                            return 1;
                        }
                    }

                    if (textStart == 0)
                    {
                        //CallWindowProc(oldWndProc, hWnd, WM_SETTEXT, 0, (LPARAM)L"");
                        PhSetWindowText(hWnd, L"");
                        return 1;
                    }
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

VOID PhCreateSearchControl(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle,
    _In_opt_ PWSTR BannerText
    )
{
    PPH_SEARCHCONTROL_CONTEXT context;

    context = PhAllocateZero(sizeof(PH_SEARCHCONTROL_CONTEXT));
    context->ParentWindowHandle = ParentWindowHandle;
    context->CueBannerText = BannerText ? PhCreateString(BannerText) : NULL;
    context->WindowDpi = PhGetWindowDpi(ParentWindowHandle);

    // Subclass the Edit control window procedure.
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
    PhSetWindowContext(WindowHandle, SHRT_MAX, context);
    SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)PhpSearchWndSubclassProc);

    // Initialize the theme parameters.
    PhSearchControlThemeChanged(context, WindowHandle);
}
