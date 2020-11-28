/*
 * Process Hacker -
 *   Searchbox control
 *
 * Copyright (C) 2012-2017 dmex
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
 *
 */

#include <phapp.h>
#include <settings.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <wincodec.h>

typedef struct _EDIT_CONTEXT
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG ThemeSupport : 1;
            ULONG Hot : 1;
            ULONG Pushed : 1;
            ULONG ColorMode : 8;
            ULONG Spare : 21;
        };
    };

    LONG CXWidth;
    INT CXBorder;
    INT ImageWidth;
    INT ImageHeight;
    HWND WindowHandle;
    WNDPROC DefaultWindowProc;
    HFONT WindowFont;
    HIMAGELIST ImageListHandle;
    HBRUSH BrushNormal;
    HBRUSH BrushPushed;
    HBRUSH BrushHot;
} EDIT_CONTEXT, *PEDIT_CONTEXT;

HICON PhpSearchBitmapToIcon(
    _In_ HBITMAP BitmapHandle,
    _In_ INT Width,
    _In_ INT Height
    );

VOID PhpSearchFreeTheme(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    if (Context->BrushNormal)
        DeleteBrush(Context->BrushNormal);
    if (Context->BrushHot)
        DeleteBrush(Context->BrushHot);
    if (Context->BrushPushed)
        DeleteBrush(Context->BrushPushed);
}

VOID PhpSearchInitializeFont(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    if (Context->WindowFont) 
        DeleteFont(Context->WindowFont);

    Context->WindowFont = PhCreateCommonFont(10, FW_MEDIUM, Context->WindowHandle);
}

VOID PhpSearchInitializeTheme(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    Context->CXWidth = PH_SCALE_DPI(20);
    Context->BrushNormal = GetSysColorBrush(COLOR_WINDOW);
    Context->BrushHot = CreateSolidBrush(RGB(205, 232, 255));
    Context->BrushPushed = CreateSolidBrush(RGB(153, 209, 255));
    Context->ColorMode = PhGetIntegerSetting(L"GraphColorMode");

    if (IsThemeActive())
    {
        HTHEME themeDataHandle;

        if (themeDataHandle = OpenThemeData(Context->WindowHandle, VSCLASS_EDIT))
        {
            //IsThemePartDefined_I(themeDataHandle, EP_EDITBORDER_NOSCROLL, EPSHV_NORMAL);

            if (!SUCCEEDED(GetThemeInt(
                themeDataHandle,
                EP_EDITBORDER_NOSCROLL,
                EPSHV_NORMAL,
                TMT_BORDERSIZE,
                &Context->CXBorder
                )))
            {
                Context->CXBorder = GetSystemMetrics(SM_CXBORDER) * 2;
            }

            CloseThemeData(themeDataHandle);
        }
        else
        {
            Context->CXBorder = GetSystemMetrics(SM_CXBORDER) * 2;
        }
    }
    else
    {
        Context->CXBorder = GetSystemMetrics(SM_CXBORDER) * 2;
    }
}

VOID PhpSearchInitializeImages(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    HBITMAP bitmap;

    Context->ImageWidth = GetSystemMetrics(SM_CXSMICON) + 4;
    Context->ImageHeight = GetSystemMetrics(SM_CYSMICON) + 4;
    Context->ImageListHandle = ImageList_Create(
        Context->ImageWidth,
        Context->ImageHeight,
        ILC_MASK | ILC_COLOR32,
        2,
        0
        );
    ImageList_SetImageCount(Context->ImageListHandle, 2);

    if (bitmap = PhLoadPngImageFromResource(PhInstanceHandle, Context->ImageWidth, Context->ImageHeight, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE), TRUE))
    {
        ImageList_Replace(Context->ImageListHandle, 0, bitmap, NULL);
        DeleteBitmap(bitmap);
    }
    else
    {
        PhSetImageListBitmap(Context->ImageListHandle, 0, PhInstanceHandle, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_BMP));
    }

    if (bitmap = PhLoadPngImageFromResource(PhInstanceHandle, Context->ImageWidth, Context->ImageHeight, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE), TRUE))
    {
        ImageList_Replace(Context->ImageListHandle, 1, bitmap, NULL);
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
    _In_ RECT WindowRect,
    _In_ RECT ButtonRect
    )
{
    HDC hdc;
    HDC bufferDc;
    HBITMAP bufferBitmap;
    HBITMAP oldBufferBitmap;
    RECT bufferRect =
    {
        0, 0,
        ButtonRect.right - ButtonRect.left,
        ButtonRect.bottom - ButtonRect.top
    };

    if (!(hdc = GetWindowDC(Context->WindowHandle)))
        return;

    bufferDc = CreateCompatibleDC(hdc);
    bufferBitmap = CreateCompatibleBitmap(hdc, bufferRect.right, bufferRect.bottom);
    oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

    if (Context->Pushed)
    {
        if (Context->ThemeSupport)
        {
            switch (Context->ColorMode)
            {
            case 0: // New colors
                FillRect(bufferDc, &bufferRect, Context->BrushPushed);
                break;
            case 1: // Old colors
                SetTextColor(bufferDc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetDCBrushColor(bufferDc, RGB(99, 99, 99));
                FillRect(bufferDc, &bufferRect, GetStockBrush(DC_BRUSH));
                break;
            }
        }
        else
        {
            FillRect(bufferDc, &bufferRect, Context->BrushPushed);
            //FrameRect(bufferDc, &bufferRect, CreateSolidBrush(RGB(0xff, 0, 0)));
        }
    }
    else if (Context->Hot)
    {
        if (Context->ThemeSupport)
        {
            switch (Context->ColorMode)
            {
            case 0: // New colors
                FillRect(bufferDc, &bufferRect, Context->BrushHot);
                break;
            case 1: // Old colors
                SetTextColor(bufferDc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetDCBrushColor(bufferDc, RGB(78, 78, 78));
                FillRect(bufferDc, &bufferRect, GetStockBrush(DC_BRUSH));
                break;
            }
        }
        else
        {
            FillRect(bufferDc, &bufferRect, Context->BrushHot);
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
                FillRect(bufferDc, &bufferRect, Context->BrushNormal);
                break;
            case 1: // Old colors
                SetTextColor(bufferDc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetDCBrushColor(bufferDc, RGB(60, 60, 60));
                FillRect(bufferDc, &bufferRect, GetStockBrush(DC_BRUSH));
                break;
            }

        }
        else
        {
            FillRect(bufferDc, &bufferRect, Context->BrushNormal);
        }
    }

    if (Edit_GetTextLength(Context->WindowHandle) > 0)
    {
        ImageList_Draw(
            Context->ImageListHandle,
            0,
            bufferDc,
            bufferRect.left + 1, // offset
            bufferRect.top,
            ILD_TRANSPARENT
            );
    }
    else
    {
        ImageList_Draw(
            Context->ImageListHandle,
            1,
            bufferDc,
            bufferRect.left + 2, // offset
            bufferRect.top + 1, // offset
            ILD_TRANSPARENT
            );
    }

    BitBlt(hdc, ButtonRect.left, ButtonRect.top, ButtonRect.right, ButtonRect.bottom, bufferDc, 0, 0, SRCCOPY);
    SelectBitmap(bufferDc, oldBufferBitmap);
    DeleteBitmap(bufferBitmap);
    DeleteDC(bufferDc);

    if (Context->ThemeSupport) // HACK
    {
        if (GetFocus() == Context->WindowHandle)
        {
            switch (Context->ColorMode)
            {
            case 0: // New colors
                SetDCBrushColor(hdc, RGB(0, 0, 0));
                break;
            case 1: // Old colors
                SetDCBrushColor(hdc, RGB(65, 65, 65));
                break;
            }

            SelectBrush(hdc, GetStockBrush(DC_BRUSH));
            PatBlt(hdc, WindowRect.left, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(hdc, WindowRect.right - 1, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(hdc, WindowRect.left, WindowRect.top, WindowRect.right - WindowRect.left, 1, PATCOPY);
            PatBlt(hdc, WindowRect.left, WindowRect.bottom - 1, WindowRect.right - WindowRect.left, 1, PATCOPY);

            switch (Context->ColorMode)
            {
            case 0: // New colors
                SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff));
                break;
            case 1: // Old colors
                SetDCBrushColor(hdc, RGB(60, 60, 60));
                break;
            }

            SelectBrush(hdc, GetStockBrush(DC_BRUSH));
            PatBlt(hdc, WindowRect.left + 1, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(hdc, WindowRect.right - 2, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(hdc, WindowRect.left + 1, WindowRect.top + 1, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
            PatBlt(hdc, WindowRect.left + 1, WindowRect.bottom - 2, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
        }
        else
        {       
            switch (Context->ColorMode)
            {
            case 0: // New colors
                SetDCBrushColor(hdc, RGB(0, 0, 0));
                break;
            case 1: // Old colors
                SetDCBrushColor(hdc, RGB(65, 65, 65));
                break;
            }

            SelectBrush(hdc, GetStockBrush(DC_BRUSH));
            PatBlt(hdc, WindowRect.left, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(hdc, WindowRect.right - 1, WindowRect.top, 1, WindowRect.bottom - WindowRect.top, PATCOPY);
            PatBlt(hdc, WindowRect.left, WindowRect.top, WindowRect.right - WindowRect.left, 1, PATCOPY);
            PatBlt(hdc, WindowRect.left, WindowRect.bottom - 1, WindowRect.right - WindowRect.left, 1, PATCOPY);

            switch (Context->ColorMode)
            {
            case 0: // New colors
                SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff));
                break;
            case 1: // Old colors
                SetDCBrushColor(hdc, RGB(60, 60, 60));
                break;
            }

            SelectBrush(hdc, GetStockBrush(DC_BRUSH));
            PatBlt(hdc, WindowRect.left + 1, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(hdc, WindowRect.right - 2, WindowRect.top + 1, 1, WindowRect.bottom - WindowRect.top - 2, PATCOPY);
            PatBlt(hdc, WindowRect.left + 1, WindowRect.top + 1, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
            PatBlt(hdc, WindowRect.left + 1, WindowRect.bottom - 2, WindowRect.right - WindowRect.left - 2, 1, PATCOPY);
        }
    }

    ReleaseDC(Context->WindowHandle, hdc);
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
                ImageList_Destroy(context->ImageListHandle);
                context->ImageListHandle = NULL;
            }

            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hWnd, SHRT_MAX);
            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return 1;
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

            // Let Windows handle the non-client defaults.
            CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Adjust the coordinates (start from 0,0).
            OffsetRect(&windowRect, -windowRect.left, -windowRect.top);
            buttonRect = windowRect;

            // Get the position of the inserted button.
            PhpSearchGetButtonRect(context, &buttonRect);

            // Draw the button.
            PhpSearchDrawButton(context, windowRect, buttonRect);
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
                Static_SetText(hWnd, L"");
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
            PhpSearchInitializeTheme(context);
            PhpSearchInitializeFont(context);

            // Reset the client area margins.
            SendMessage(hWnd, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));

            // Refresh the non-client area.
            SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

            // Force the edit control to update its non-client area.
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_NCMOUSEMOVE:
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
            if (PtInRect(&windowRect, windowPoint) && !context->Hot)
            {
                TRACKMOUSEEVENT trackMouseEvent;

                trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                trackMouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
                trackMouseEvent.hwndTrack = hWnd;
                trackMouseEvent.dwHoverTime = 0;

                context->Hot = TRUE;

                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

                TrackMouseEvent(&trackMouseEvent);
            }
        }
        break;
    case WM_NCMOUSELEAVE:
        {
            if (context->Hot)
            {
                context->Hot = FALSE;
                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if ((wParam & MK_LBUTTON) && GetCapture() == hWnd)
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
                context->Pushed = PtInRect(&windowRect, windowPoint);

                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
}

HICON PhpSearchBitmapToIcon(
    _In_ HBITMAP BitmapHandle,
    _In_ INT Width,
    _In_ INT Height
    )
{
    HICON icon;
    HDC screenDc;
    HBITMAP screenBitmap;
    ICONINFO iconInfo = { 0 };

    screenDc = CreateCompatibleDC(NULL);
    screenBitmap = CreateCompatibleBitmap(screenDc, Width, Height);

    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = BitmapHandle;
    iconInfo.hbmMask = screenBitmap;

    icon = CreateIconIndirect(&iconInfo);

    DeleteBitmap(screenBitmap);
    DeleteDC(screenDc);

    return icon;
}

VOID PhCreateSearchControl(
    _In_ HWND Parent,
    _In_ HWND WindowHandle,
    _In_opt_ PWSTR BannerText
    )
{
    PEDIT_CONTEXT context;

    context = (PEDIT_CONTEXT)PhAllocate(sizeof(EDIT_CONTEXT));
    memset(context, 0, sizeof(EDIT_CONTEXT));

    context->WindowHandle = WindowHandle;
    context->ThemeSupport = !!PhGetIntegerSetting(L"EnableThemeSupport"); // HACK
    context->ColorMode = PhGetIntegerSetting(L"GraphColorMode");

    //PhpSearchInitializeTheme(context);
    PhpSearchInitializeImages(context);

    // Set initial text
    if (BannerText)
        Edit_SetCueBannerText(WindowHandle, BannerText);

    // Subclass the Edit control window procedure.
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
    PhSetWindowContext(WindowHandle, SHRT_MAX, context);
    SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)PhpSearchWndSubclassProc);

    // Initialize the theme parameters.
    SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);
}

HBITMAP PhLoadPngImageFromResource(
    _In_ PVOID DllBase,
    _In_ UINT Width,
    _In_ UINT Height,
    _In_ PCWSTR Name,
    _In_ BOOLEAN RGBAImage
    )
{
    BOOLEAN success = FALSE;
    UINT frameCount = 0;
    ULONG resourceLength = 0;
    WICInProcPointer resourceBuffer = NULL;
    HDC screenHdc = NULL;
    HDC bufferDc = NULL;
    BITMAPINFO bitmapInfo = { 0 };
    HBITMAP bitmapHandle = NULL;
    PVOID bitmapBuffer = NULL;
    IWICStream* wicStream = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicDecoder = NULL;
    IWICBitmapFrameDecode* wicFrame = NULL;
    IWICImagingFactory* wicFactory = NULL;
    IWICBitmapScaler* wicScaler = NULL;
    WICPixelFormatGUID pixelFormat;
    WICRect rect = { 0, 0, Width, Height };

    // Load the resource
    if (!PhLoadResource(DllBase, Name, L"PNG", &resourceLength, &resourceBuffer))
        goto CleanupExit;

    // Create the ImagingFactory
    if (FAILED(PhGetClassObject(L"windowscodecs.dll", &CLSID_WICImagingFactory1, &IID_IWICImagingFactory, &wicFactory)))
        goto CleanupExit;

    // Create the Stream
    if (FAILED(IWICImagingFactory_CreateStream(wicFactory, &wicStream)))
        goto CleanupExit;

    // Initialize the Stream from Memory
    if (FAILED(IWICStream_InitializeFromMemory(wicStream, resourceBuffer, resourceLength)))
        goto CleanupExit;

    if (FAILED(IWICImagingFactory_CreateDecoder(wicFactory, &GUID_ContainerFormatPng, NULL, &wicDecoder)))
        goto CleanupExit;

    if (FAILED(IWICBitmapDecoder_Initialize(wicDecoder, (IStream*)wicStream, WICDecodeMetadataCacheOnLoad)))
        goto CleanupExit;

    // Get the Frame count
    if (FAILED(IWICBitmapDecoder_GetFrameCount(wicDecoder, &frameCount)) || frameCount < 1)
        goto CleanupExit;

    // Get the Frame
    if (FAILED(IWICBitmapDecoder_GetFrame(wicDecoder, 0, &wicFrame)))
        goto CleanupExit;

    // Get the WicFrame image format
    if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicFrame, &pixelFormat)))
        goto CleanupExit;

    // Check if the image format is supported:
    if (IsEqualGUID(&pixelFormat, RGBAImage ? &GUID_WICPixelFormat32bppPRGBA : &GUID_WICPixelFormat32bppPBGRA))
    {
        wicBitmapSource = (IWICBitmapSource*)wicFrame;
    }
    else
    {
        IWICFormatConverter* wicFormatConverter = NULL;

        if (FAILED(IWICImagingFactory_CreateFormatConverter(wicFactory, &wicFormatConverter)))
            goto CleanupExit;

        if (FAILED(IWICFormatConverter_Initialize(
            wicFormatConverter,
            (IWICBitmapSource*)wicFrame,
            RGBAImage ? &GUID_WICPixelFormat32bppPRGBA : &GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.0,
            WICBitmapPaletteTypeCustom
            )))
        {
            IWICFormatConverter_Release(wicFormatConverter);
            goto CleanupExit;
        }

        // Convert the image to the correct format:
        IWICFormatConverter_QueryInterface(wicFormatConverter, &IID_IWICBitmapSource, &wicBitmapSource);

        // Cleanup the converter.
        IWICFormatConverter_Release(wicFormatConverter);

        // Dispose the old frame now that the converted frame is in wicBitmapSource.
        IWICBitmapFrameDecode_Release(wicFrame);
    }

    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = rect.Width;
    bitmapInfo.bmiHeader.biHeight = -((LONG)rect.Height);
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    screenHdc = GetDC(NULL);
    bufferDc = CreateCompatibleDC(screenHdc);
    bitmapHandle = CreateDIBSection(screenHdc, &bitmapInfo, DIB_RGB_COLORS, &bitmapBuffer, NULL, 0);

    // Check if it's the same rect as the requested size.
    //if (width != rect.Width || height != rect.Height)
    if (FAILED(IWICImagingFactory_CreateBitmapScaler(wicFactory, &wicScaler)))
        goto CleanupExit;
    if (FAILED(IWICBitmapScaler_Initialize(wicScaler, wicBitmapSource, rect.Width, rect.Height, WICBitmapInterpolationModeFant)))
        goto CleanupExit;
    if (FAILED(IWICBitmapScaler_CopyPixels(wicScaler, &rect, rect.Width * 4, rect.Width * rect.Height * 4, (PBYTE)bitmapBuffer)))
        goto CleanupExit;

    success = TRUE;

CleanupExit:

    if (wicScaler)
        IWICBitmapScaler_Release(wicScaler);

    if (bufferDc)
        DeleteDC(bufferDc);

    if (screenHdc)
        ReleaseDC(NULL, screenHdc);

    if (wicBitmapSource)
        IWICBitmapSource_Release(wicBitmapSource);

    if (wicStream)
        IWICStream_Release(wicStream);

    if (wicDecoder)
        IWICBitmapDecoder_Release(wicDecoder);

    if (wicFactory)
        IWICImagingFactory_Release(wicFactory);

    if (resourceBuffer)
        PhFree(resourceBuffer);

    if (success)
        return bitmapHandle;

    DeleteBitmap(bitmapHandle);
    return NULL;
}

HBITMAP PhLoadPngImageFromFile(
    _In_ PWSTR FileName,
    _In_ UINT Width,
    _In_ UINT Height,
    _In_ BOOLEAN RGBAImage
    )
{
    BOOLEAN success = FALSE;
    UINT frameCount = 0;
    HDC screenHdc = NULL;
    HDC bufferDc = NULL;
    BITMAPINFO bitmapInfo = { 0 };
    HBITMAP bitmapHandle = NULL;
    PVOID bitmapBuffer = NULL;
    IWICStream* wicStream = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicDecoder = NULL;
    IWICBitmapFrameDecode* wicFrame = NULL;
    IWICImagingFactory* wicFactory = NULL;
    IWICBitmapScaler* wicScaler = NULL;
    WICPixelFormatGUID pixelFormat;
    WICRect rect = { 0, 0, Width, Height };

    // Create the ImagingFactory
    if (FAILED(PhGetClassObject(L"windowscodecs.dll", &CLSID_WICImagingFactory1, &IID_IWICImagingFactory, &wicFactory)))
        goto CleanupExit;

    // Create the Stream
    if (FAILED(IWICImagingFactory_CreateStream(wicFactory, &wicStream)))
        goto CleanupExit;

    // Initialize the Stream from Memory
    if (FAILED(IWICStream_InitializeFromFilename(wicStream, FileName, GENERIC_READ)))
        goto CleanupExit;

    if (FAILED(IWICImagingFactory_CreateDecoder(wicFactory, &GUID_ContainerFormatPng, NULL, &wicDecoder)))
        goto CleanupExit;

    if (FAILED(IWICBitmapDecoder_Initialize(wicDecoder, (IStream*)wicStream, WICDecodeMetadataCacheOnLoad)))
        goto CleanupExit;

    // Get the Frame count
    if (FAILED(IWICBitmapDecoder_GetFrameCount(wicDecoder, &frameCount)) || frameCount < 1)
        goto CleanupExit;

    // Get the Frame
    if (FAILED(IWICBitmapDecoder_GetFrame(wicDecoder, 0, &wicFrame)))
        goto CleanupExit;

    // Get the WicFrame image format
    if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicFrame, &pixelFormat)))
        goto CleanupExit;

    // Check if the image format is supported:
    if (IsEqualGUID(&pixelFormat, RGBAImage ? &GUID_WICPixelFormat32bppPRGBA : &GUID_WICPixelFormat32bppPBGRA))
    {
        wicBitmapSource = (IWICBitmapSource*)wicFrame;
    }
    else
    {
        IWICFormatConverter* wicFormatConverter = NULL;

        if (FAILED(IWICImagingFactory_CreateFormatConverter(wicFactory, &wicFormatConverter)))
            goto CleanupExit;

        if (FAILED(IWICFormatConverter_Initialize(
            wicFormatConverter,
            (IWICBitmapSource*)wicFrame,
            RGBAImage ? &GUID_WICPixelFormat32bppPRGBA : &GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.0,
            WICBitmapPaletteTypeCustom
            )))
        {
            IWICFormatConverter_Release(wicFormatConverter);
            goto CleanupExit;
        }

        // Convert the image to the correct format:
        IWICFormatConverter_QueryInterface(wicFormatConverter, &IID_IWICBitmapSource, &wicBitmapSource);

        // Cleanup the converter.
        IWICFormatConverter_Release(wicFormatConverter);

        // Dispose the old frame now that the converted frame is in wicBitmapSource.
        IWICBitmapFrameDecode_Release(wicFrame);
    }

    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = rect.Width;
    bitmapInfo.bmiHeader.biHeight = -((LONG)rect.Height);
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    screenHdc = GetDC(NULL);
    bufferDc = CreateCompatibleDC(screenHdc);
    bitmapHandle = CreateDIBSection(screenHdc, &bitmapInfo, DIB_RGB_COLORS, &bitmapBuffer, NULL, 0);

    // Check if it's the same rect as the requested size.
    //if (width != rect.Width || height != rect.Height)
    if (FAILED(IWICImagingFactory_CreateBitmapScaler(wicFactory, &wicScaler)))
        goto CleanupExit;
    if (FAILED(IWICBitmapScaler_Initialize(wicScaler, wicBitmapSource, rect.Width, rect.Height, WICBitmapInterpolationModeFant)))
        goto CleanupExit;
    if (FAILED(IWICBitmapScaler_CopyPixels(wicScaler, &rect, rect.Width * 4, rect.Width * rect.Height * 4, (PBYTE)bitmapBuffer)))
        goto CleanupExit;

    success = TRUE;

CleanupExit:

    if (wicScaler)
        IWICBitmapScaler_Release(wicScaler);

    if (bufferDc)
        DeleteDC(bufferDc);

    if (screenHdc)
        ReleaseDC(NULL, screenHdc);

    if (wicBitmapSource)
        IWICBitmapSource_Release(wicBitmapSource);

    if (wicStream)
        IWICStream_Release(wicStream);

    if (wicDecoder)
        IWICBitmapDecoder_Release(wicDecoder);

    if (wicFactory)
        IWICImagingFactory_Release(wicFactory);

    if (success)
        return bitmapHandle;

    DeleteBitmap(bitmapHandle);
    return NULL;
}
