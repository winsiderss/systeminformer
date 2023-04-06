/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022-2023
 *
 */

#include <ph.h>
#include <guisup.h>

#define GDIPVER 0x0110
#include <unknwn.h>
//#include <wtypes.h>
#include <gdiplus.h>

using namespace Gdiplus;

static Bitmap* PhGdiplusCreateBitmapFromDIB(
    _In_ HBITMAP OriginalBitmap
    )
{
    DIBSECTION dib = {};

    if (GetObject(OriginalBitmap, sizeof(DIBSECTION), &dib) == sizeof(DIBSECTION))
    {
        LONG width = dib.dsBmih.biWidth;
        LONG height = dib.dsBmih.biHeight;
        LONG pitch = dib.dsBm.bmWidthBytes;
        BYTE* bitmapBuffer = static_cast<BYTE*>(dib.dsBm.bmBits);

        return new Bitmap(width, height, pitch, PixelFormat32bppARGB, bitmapBuffer);
    }

    return nullptr;
}

// Note: This function is used to draw bitmaps with the current accent color
// as the background of the bitmap similar to what Task Manager does (dmex)
HICON PhGdiplusConvertBitmapToIcon(
    _In_ HBITMAP OriginalBitmap,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ COLORREF Background
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN initialized = FALSE;
    HICON icon;

    if (PhBeginInitOnce(&initOnce))
    {
        ULONG_PTR gdiplusToken = 0;
        GdiplusStartupInput gdiplusStartupInput{};

        if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) == Status::Ok)
        {
            initialized = TRUE;
        }

        PhEndInitOnce(&initOnce);
    }

    if (initialized)
    {
        Bitmap* image = PhGdiplusCreateBitmapFromDIB(OriginalBitmap);

        if (image != nullptr)
        {
            Bitmap* buffer = new Bitmap(Width, Height, PixelFormat32bppARGB);
            Graphics* graphics = Graphics::FromImage(buffer);

            //if (Background)
            //{
            //    Color color(Color::DodgerBlue);
            //    color.SetFromCOLORREF(Background); // accent color
            //    graphics->Clear(color);
            //}

            graphics->Clear(Color::DodgerBlue);

            if (graphics->DrawImage(image, 0, 0) == Status::Ok)
            {
                if (buffer->GetHICON(&icon) == Status::Ok)
                    return icon;
            }
        }
    }

    return nullptr;
}

#ifdef PHNT_TRANSPARENT_BITMAP
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

VOID PhUpdateTransparentBackgroundWindow(
    _In_ HWND WindowHandle,
    _In_ PRECT ClientRect
    )
{
    HDC hdc;
    SIZE windowSize;
    POINT windowPoint = { 0, 0 };
    BLENDFUNCTION blendFunction = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    BP_PAINTPARAMS paintParams = { sizeof(paintParams) };
    HDC bufferHdc;
    HPAINTBUFFER paintBuffer;

    hdc = GetDC(WindowHandle);

    windowSize.cx = ClientRect->right - ClientRect->left;
    windowSize.cy = ClientRect->bottom - ClientRect->top;

    paintParams.dwFlags = BPPF_ERASE; // Clear rectangle to ARGB 0,0,0,0
    paintParams.pBlendFunction = &blendFunction;

    if (paintBuffer = BeginBufferedPaint(hdc, ClientRect, BPBF_TOPDOWNDIB, &paintParams, &bufferHdc))
    {
        BufferedPaintSetAlpha(paintBuffer, ClientRect, 128);

        UpdateLayeredWindow(
            WindowHandle,
            nullptr,
            &windowPoint,
            &windowSize,
            bufferHdc,
            &windowPoint,
            0,
            &blendFunction,
            ULW_ALPHA
            );

        EndBufferedPaint(paintBuffer, FALSE);
    }
    else
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        static BOOLEAN initialized = FALSE;
        HBITMAP bitmapHandle;
        HGDIOBJ oldBitmapHandle;

        if (PhBeginInitOnce(&initOnce))
        {
            ULONG_PTR gdiplusToken = 0;
            GdiplusStartupInput gdiplusStartupInput{};

            if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) == Status::Ok)
            {
                initialized = TRUE;
            }

            PhEndInitOnce(&initOnce);
        }

        bufferHdc = CreateCompatibleDC(hdc);
        bitmapHandle = CreateCompatibleBitmap(hdc, ClientRect->right, ClientRect->bottom);
        oldBitmapHandle = SelectBitmap(bufferHdc, bitmapHandle);

        if (initialized)
        {
            Graphics graphics(bufferHdc);
            graphics.Clear(Color(128, 0, 0, 0));
        }

        UpdateLayeredWindow(
            WindowHandle,
            nullptr,
            &windowPoint,
            &windowSize,
            bufferHdc,
            &windowPoint,
            0,
            &blendFunction,
            ULW_ALPHA
            );
        
        SelectBitmap(bufferHdc, oldBitmapHandle);
        DeleteBitmap(bitmapHandle);
        DeleteDC(bufferHdc);
    }

    ReleaseDC(WindowHandle, hdc);
}
#endif

LRESULT CALLBACK PhTransparentBackgroundWindowCallback(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_CREATE:
        {
            LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);

            if (createStruct->hwndParent)
            {
                HMENU menu = GetMenu(createStruct->hwndParent);

                if (menu)
                {
                    PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, menu);
                    SetMenu(createStruct->hwndParent, nullptr);
                }
            }

#ifdef PHNT_TRANSPARENT_BITMAP
            RECT clientRect;

            clientRect.left = 0;
            clientRect.top = 0;
            clientRect.right = createStruct->cx;
            clientRect.bottom = createStruct->cy;
            
            PhUpdateTransparentBackgroundWindow(WindowHandle, &clientRect);
#else
            constexpr ULONG OpacityPercent = 50;

            // The opacity value is backwards - 0 means opaque, 100 means transparent.
            SetLayeredWindowAttributes(
                WindowHandle,
                0,
                static_cast<BYTE>(255 * (100 - OpacityPercent) / 100),
                LWA_ALPHA
                );
#endif
        }
        break;
    case WM_DESTROY:
        {
            HMENU menu = static_cast<HMENU>(PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT));

            if (menu)
            {
                SetMenu(GetParent(WindowHandle), menu);
            }
            
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
#ifndef PHNT_TRANSPARENT_BITMAP
    case WM_ERASEBKGND:
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            RECT clientRect;

            GetClientRect(WindowHandle, &clientRect);
            FillRect(hdc, &clientRect, GetStockBrush(BLACK_BRUSH));
        }
        return TRUE;
#endif
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

RTL_ATOM PhInitializeBackgroundWindowClass(
    VOID
    )
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = PhTransparentBackgroundWindowCallback;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"TransparentBackgroundWindowClass";

    return RegisterClassEx(&wcex);
}

HWND PhCreateBackgroundWindow(
    _In_ HWND ParentWindowHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static RTL_ATOM windowAtom = 0;
    HWND windowHandle;
    RECT windowRect = { 0 };

    if (WindowsVersion < WINDOWS_8)
        return nullptr;

    if (!GetClientRect(ParentWindowHandle, &windowRect))
        return nullptr;

    {
        MENUBARINFO menuInfo;

        memset(&menuInfo, 0, sizeof(MENUBARINFO));
        menuInfo.cbSize = sizeof(MENUBARINFO);

        if (GetMenuBarInfo(ParentWindowHandle, OBJID_MENU, 0, &menuInfo))
        {
            windowRect.bottom += menuInfo.rcBar.bottom;
        }
    }

    if (PhBeginInitOnce(&initOnce))
    {
        windowAtom = PhInitializeBackgroundWindowClass();
        PhEndInitOnce(&initOnce);
    }

    if (windowAtom == INVALID_ATOM)
        return nullptr;

    windowHandle = CreateWindowEx(
#ifdef PHNT_TRANSPARENT_BITMAP
        WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP,
#else
        WS_EX_LAYERED,
#endif
        MAKEINTATOM(windowAtom),
        nullptr,
        WS_CHILD,
        0,
        0,
        windowRect.right,
        windowRect.bottom,
        ParentWindowHandle,
        nullptr,
        nullptr,
        nullptr
        );

    SetWindowPos(
        windowHandle,
        HWND_TOP,
        0,
        0,
        0,
        0,
        SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW
        );

    return windowHandle;
}

//#pragma comment(lib, "d2d1.lib")
//#pragma comment(lib, "dwrite.lib")
//
//#include <d2d1.h>
//#include <dwrite.h>
//#include <d2d1_3.h>
//
//static IDWriteTextFormat* PhDirectWriteTextFormat = nullptr;
//static ID2D1DCRenderTarget* PhDirectWriteRenderTarget = nullptr;
//static ID2D1SolidColorBrush* PhDirectWriteSolidBrush = nullptr;
//
//BOOLEAN PhCreateDirectWriteTextResources(
//    VOID
//    )
//{
//    D2D1_RENDER_TARGET_PROPERTIES render_target_properties =
//    {
//        .type = D2D1_RENDER_TARGET_TYPE_SOFTWARE,
//        .pixelFormat =
//        {
//            .format = DXGI_FORMAT_B8G8R8A8_UNORM,
//            .alphaMode = D2D1_ALPHA_MODE_IGNORE, // D2D1_ALPHA_MODE_PREMULTIPLIED
//        },
//        .dpiX = 0, .dpiY = 0,
//        .usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE,
//        .minLevel = D2D1_FEATURE_LEVEL_DEFAULT
//    };
//    HRESULT status;
//    ID2D1Factory* directFactory = nullptr;
//    IDWriteFactory* dwriteFactory = nullptr;
//
//    status = DWriteCreateFactory(
//        DWRITE_FACTORY_TYPE_SHARED,
//        __uuidof(IDWriteFactory),
//        reinterpret_cast<IUnknown**>(&dwriteFactory)
//        );
//
//    if (FAILED(status))
//        return FALSE;
//
//    status = dwriteFactory->CreateTextFormat(
//        L"Segoe UI",
//        nullptr,
//        DWRITE_FONT_WEIGHT_NORMAL,
//        DWRITE_FONT_STYLE_NORMAL,
//        DWRITE_FONT_STRETCH_NORMAL,
//        12,
//        L"", // locale
//        &PhDirectWriteTextFormat
//        );
//
//    if (FAILED(status))
//        return FALSE;
//
//    status = D2D1CreateFactory(
//        D2D1_FACTORY_TYPE_SINGLE_THREADED,
//        reinterpret_cast<IUnknown**>(&directFactory)
//        );
//
//    if (FAILED(status))
//        return FALSE;
//
//    status = directFactory->CreateDCRenderTarget(
//        &render_target_properties,
//        &PhDirectWriteRenderTarget
//        );
//
//    if (FAILED(status))
//        return FALSE;
//
//    status = PhDirectWriteRenderTarget->CreateSolidColorBrush(
//        D2D1::ColorF(D2D1::ColorF::Black, 1.0f),
//        &PhDirectWriteSolidBrush
//        );
//
//    if (FAILED(status))
//        return FALSE;
//
//    return TRUE;
//}
//
//VOID DirectWriteDrawText(
//    _In_ HDC Hdc,
//    _In_ PRECT ClientRect,
//    _In_ PWSTR Text,
//    _In_ SIZE_T TextLength
//    )
//{
//    static PH_INITONCE initOnce = PH_INITONCE_INIT;
//
//    if (PhBeginInitOnce(&initOnce))
//    {
//        PhCreateDirectWriteTextResources();
//        PhEndInitOnce(&initOnce);
//    }
//
//    if (FAILED(PhDirectWriteRenderTarget->BindDC(Hdc, ClientRect)))
//        return;
//
//    PhDirectWriteRenderTarget->BeginDraw();
//    PhDirectWriteRenderTarget->DrawText(
//        Text,
//        static_cast<UINT32>(TextLength),
//        PhDirectWriteTextFormat,
//        D2D1::RectF(0, 0, static_cast<FLOAT>(ClientRect->right), static_cast<FLOAT>(ClientRect->bottom)),
//        PhDirectWriteSolidBrush,
//        D2D1_DRAW_TEXT_OPTIONS_CLIP,
//        DWRITE_MEASURING_MODE_GDI_CLASSIC
//        );
//    PhDirectWriteRenderTarget->EndDraw();
//}
