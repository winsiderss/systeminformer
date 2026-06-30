/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022-2026
 *
 */

#include <ph.h>
#include <guisup.h>
#include <settings.h>

#define GDIPVER 0x0110
#include <unknwn.h>
//#include <wtypes.h>
#include <gdiplus.h>

using namespace Gdiplus;

//std::unique_ptr<Bitmap> make_bitmap(
//    _In_ LONG Width,
//    _In_ LONG Height,
//    _In_ PixelFormat Format
//    )
//{
//    return std::make_unique<Bitmap>(Width, Height, Format);
//}
//
//std::unique_ptr<Bitmap> make_bitmap(
//    _In_ LONG Width,
//    _In_ LONG Height,
//    _In_ LONG Stride,
//    _In_ PixelFormat Format,
//    _In_reads_opt_(_Inexpressible_("height * stride")) PBYTE Buffer
//    )
//{
//    return std::make_unique<Bitmap>(Width, Height, Stride, Format, Buffer);
//}
//
//std::unique_ptr<Graphics> make_graphics(
//    _In_ const std::unique_ptr<Bitmap>& image
//    )
//{
//    return std::unique_ptr<Graphics>(Graphics::FromImage(image.get()));
//}

BOOLEAN PhInitializeGDIPlus(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN initialized = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        static ULONG_PTR gdiplusToken = 0;
        static GdiplusStartupInput gdiplusStartupInput = { nullptr, false, true };

        if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) == Status::Ok)
        {
            initialized = TRUE;
        }

        PhEndInitOnce(&initOnce);
    }

    return initialized;
}

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
    HICON icon;

    if (PhInitializeGDIPlus())
    {
        DIBSECTION dib;

        RtlZeroMemory(&dib, sizeof(DIBSECTION));

        if (GetObject(OriginalBitmap, sizeof(DIBSECTION), &dib) != sizeof(DIBSECTION))
            return nullptr;

        LONG width = dib.dsBmih.biWidth;
        LONG height = dib.dsBmih.biHeight;
        LONG pitch = dib.dsBm.bmWidthBytes;
        BYTE* bitmapBuffer = static_cast<BYTE*>(dib.dsBm.bmBits);

        Bitmap image(width, height, pitch, PixelFormat32bppARGB, bitmapBuffer);
        Bitmap buffer(Width, Height, PixelFormat32bppARGB);
        Graphics graphics(&buffer);

        if (Background)
        {
            Color color(Color::DodgerBlue);
            color.SetFromCOLORREF(Background);
            graphics.Clear(color);
        }
        else
        {
            graphics.Clear(Color::DodgerBlue);
        }

        if (graphics.DrawImage(&image, 0, 0) == Status::Ok)
        {
            if (buffer.GetHICON(&icon) == Status::Ok)
            {
                return icon;
            }
        }
    }

    return nullptr;
}

HICON PhGdiplusConvertHBitmapToHIcon(
    _In_ HBITMAP BitmapHandle
    )
{
    HICON iconHandle = nullptr;

    if (PhInitializeGDIPlus())
    {
        Bitmap bitmap(BitmapHandle, nullptr);

        bitmap.GetHICON(&iconHandle);
    }

    return iconHandle;
}

HICON PhConvertBitmapToIcon(
    _In_ HBITMAP OriginalBitmap,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ COLORREF Background
    )
{
    DIBSECTION dib;
    HDC screenDc;
    HDC srcDc;
    HDC dstDc;
    HBITMAP colorBitmap;
    HBITMAP maskBitmap;
    HBITMAP oldSrcBitmap;
    HBITMAP oldDstBitmap;
    PVOID colorBits;
    BITMAPINFO bitmapInfo;
    ICONINFO iconInfo;
    HICON icon;

    RtlZeroMemory(&dib, sizeof(DIBSECTION));

    if (GetObject(OriginalBitmap, sizeof(DIBSECTION), &dib) != sizeof(DIBSECTION))
        return NULL;

    screenDc = GetDC(NULL);
    if (!screenDc)
        return NULL;

    // Create a 32bpp ARGB DIB section for the output color bitmap
    RtlZeroMemory(&bitmapInfo, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = Width;
    bitmapInfo.bmiHeader.biHeight = -Height; // top-down DIB
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    colorBitmap = CreateDIBSection(screenDc, &bitmapInfo, DIB_RGB_COLORS, &colorBits, NULL, 0);
    if (!colorBitmap)
    {
        ReleaseDC(NULL, screenDc);
        return NULL;
    }

    // Fill the color bitmap with the background color (as premultiplied ARGB)
    {
        PULONG pixels = (PULONG)colorBits;
        ULONG pixelCount = (ULONG)Width * (ULONG)Height;
        ULONG bgArgb;

        // COLORREF is 0x00BBGGRR; convert to ARGB 0xAARRGGBB for the DIB
        bgArgb = (0xFF << 24) | // fully opaque
            ((ULONG)GetRValue(Background) << 16) |
            ((ULONG)GetGValue(Background) << 8) |
            ((ULONG)GetBValue(Background));

        for (ULONG i = 0; i < pixelCount; i++)
        {
            pixels[i] = bgArgb;
        }
    }

    // AlphaBlend the source bitmap onto the background
    srcDc = CreateCompatibleDC(screenDc);
    dstDc = CreateCompatibleDC(screenDc);
    oldSrcBitmap = SelectBitmap(srcDc, OriginalBitmap);
    oldDstBitmap = SelectBitmap(dstDc, colorBitmap);

    {
        BLENDFUNCTION blendFunc;

        blendFunc.BlendOp = AC_SRC_OVER;
        blendFunc.BlendFlags = 0;
        blendFunc.SourceConstantAlpha = 255;
        blendFunc.AlphaFormat = AC_SRC_ALPHA; // source has per-pixel alpha

        GdiAlphaBlend(
            dstDc,
            0, 0,
            Width,
            Height,                         // dest rect
            srcDc,
            0, 0,                           // source origin
            dib.dsBmih.biWidth,             // source width
            dib.dsBmih.biHeight,            // source height (abs(dib.dsBmih.biHeight) for safety)
            blendFunc
            );
    }

    SelectBitmap(srcDc, oldSrcBitmap);
    SelectBitmap(dstDc, oldDstBitmap);
    DeleteDC(srcDc);
    DeleteDC(dstDc);

    // Create the monochrome mask bitmap (all zeros = fully opaque icon)
    maskBitmap = CreateBitmap(Width, Height, 1, 1, NULL);
    if (!maskBitmap)
    {
        DeleteBitmap(colorBitmap);
        ReleaseDC(NULL, screenDc);
        return NULL;
    }

    // Zero the mask (0 = opaque everywhere)
    {
        HDC maskDc = CreateCompatibleDC(screenDc);
        HBITMAP oldMask = SelectBitmap(maskDc, maskBitmap);
        RECT r = { 0, 0, Width, Height };

        FillRect(maskDc, &r, GetStockBrush(BLACK_BRUSH));
        SelectBitmap(maskDc, oldMask);
        DeleteDC(maskDc);
    }

    ReleaseDC(NULL, screenDc);

    // Create the icon from color + mask
    RtlZeroMemory(&iconInfo, sizeof(ICONINFO));
    iconInfo.fIcon = TRUE;
    iconInfo.xHotspot = 0;
    iconInfo.yHotspot = 0;
    iconInfo.hbmMask = maskBitmap;
    iconInfo.hbmColor = colorBitmap;

    icon = CreateIconIndirect(&iconInfo);

    // CreateIconIndirect copies the bitmaps, so we can delete them
    DeleteBitmap(colorBitmap);
    DeleteBitmap(maskBitmap);

    return icon;
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

            if (PhGetIntegerSetting(L"EnableStreamerMode"))
            {
                SetWindowDisplayAffinity(WindowHandle, WDA_EXCLUDEFROMCAPTURE);
            }

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

            if (GetClipBox(hdc, &clientRect) == ERROR)
                break;

            FillRect(hdc, &clientRect, PhGetStockBrush(BLACK_BRUSH));
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
    wcex.hCursor = PhLoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"TransparentBackgroundWindowClass";

    return RegisterClassEx(&wcex);
}

HWND PhCreateBackgroundWindow(
    _In_ HWND ParentWindowHandle,
    _In_ BOOLEAN DesktopWindow
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static RTL_ATOM windowAtom = 0;
    HWND windowHandle;
    RECT windowRect = { 0 };

    if (WindowsVersion < WINDOWS_8)
        return nullptr;

    if (!PhGetClientRect(ParentWindowHandle, &windowRect))
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
        DesktopWindow ? WS_POPUPWINDOW : WS_CHILD,
        0,
        0,
        windowRect.right,
        windowRect.bottom,
        DesktopWindow ? nullptr : ParentWindowHandle,
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

typedef struct _PH_WINDOW_SNAPSHOT_CONTEXT
{
    HWND SnapshotWindow;
    HWND TargetWindow;
    HBITMAP ScreenBitmap;
    HBITMAP BlurredBitmap;
    HDC MemoryDC;
    HDC BlurredDC;
    HBITMAP OldScreenBitmap;
    HBITMAP OldBlurredBitmap;
    RECT ScreenRect;
    RECT HighlightRect;
    BOOLEAN IsActive;
} PH_WINDOW_SNAPSHOT_CONTEXT, *PPH_WINDOW_SNAPSHOT_CONTEXT;

typedef struct _PH_WINDOW_FROM_POINT_CONTEXT
{
    HWND SnapshotWindow;
    HWND WindowHandle;
    POINT Point;
} PH_WINDOW_FROM_POINT_CONTEXT, *PPH_WINDOW_FROM_POINT_CONTEXT;

static BOOLEAN PhIntersectRect(
    _Out_ PRECT Result,
    _In_ PRECT Rect1,
    _In_ PRECT Rect2
    )
{
    Result->left = Rect1->left > Rect2->left ? Rect1->left : Rect2->left;
    Result->top = Rect1->top > Rect2->top ? Rect1->top : Rect2->top;
    Result->right = Rect1->right < Rect2->right ? Rect1->right : Rect2->right;
    Result->bottom = Rect1->bottom < Rect2->bottom ? Rect1->bottom : Rect2->bottom;

    return Result->right > Result->left && Result->bottom > Result->top;
}

static BOOL CALLBACK PhWindowFromPointEnumProc(
    _In_ HWND WindowHandle,
    _In_ LPARAM lParam
    )
{
    PPH_WINDOW_FROM_POINT_CONTEXT context;
    RECT windowRect;

    context = (PPH_WINDOW_FROM_POINT_CONTEXT)lParam;

    if (WindowHandle == context->SnapshotWindow)
        return TRUE;

    if (!IsWindowVisible(WindowHandle))
        return TRUE;

    if (!PhGetWindowRect(WindowHandle, &windowRect))
        return TRUE;

    if (!PtInRect(&windowRect, context->Point))
        return TRUE;

    context->WindowHandle = WindowHandle;

    return FALSE;
}

static HWND PhWindowFromPointBelowSnapshot(
    _In_ HWND SnapshotWindow,
    _In_ POINT Point
    )
{
    PH_WINDOW_FROM_POINT_CONTEXT context;
    HWND windowHandle;

    context.SnapshotWindow = SnapshotWindow;
    context.WindowHandle = nullptr;
    context.Point = Point;

    EnumWindows(PhWindowFromPointEnumProc, (LPARAM)&context);

    windowHandle = context.WindowHandle;

    while (windowHandle)
    {
        POINT clientPoint;
        HWND childWindowHandle;

        clientPoint = Point;
        ScreenToClient(windowHandle, &clientPoint);

        childWindowHandle = ChildWindowFromPointEx(
            windowHandle,
            clientPoint,
            CWP_SKIPINVISIBLE | CWP_SKIPDISABLED
            );

        if (!childWindowHandle || childWindowHandle == windowHandle)
            break;

        windowHandle = childWindowHandle;
    }

    return windowHandle;
}

static VOID PhDestroyWindowSnapshotSelection(
    _Inout_ PPH_WINDOW_SNAPSHOT_CONTEXT Context
    )
{
    if (Context->SnapshotWindow)
    {
        DestroyWindow(Context->SnapshotWindow);
        Context->SnapshotWindow = nullptr;
    }

    if (Context->ScreenBitmap)
    {
        if (Context->MemoryDC && Context->OldScreenBitmap)
        {
            SelectBitmap(Context->MemoryDC, Context->OldScreenBitmap);
            Context->OldScreenBitmap = nullptr;
        }

        DeleteBitmap(Context->ScreenBitmap);
        Context->ScreenBitmap = nullptr;
    }

    if (Context->BlurredBitmap)
    {
        if (Context->BlurredDC && Context->OldBlurredBitmap)
        {
            SelectBitmap(Context->BlurredDC, Context->OldBlurredBitmap);
            Context->OldBlurredBitmap = nullptr;
        }

        DeleteBitmap(Context->BlurredBitmap);
        Context->BlurredBitmap = nullptr;
    }

    if (Context->MemoryDC)
    {
        DeleteDC(Context->MemoryDC);
        Context->MemoryDC = nullptr;
    }

    if (Context->BlurredDC)
    {
        DeleteDC(Context->BlurredDC);
        Context->BlurredDC = nullptr;
    }

    Context->IsActive = FALSE;
}

static BOOLEAN PhCreateBlurredWindowSnapshotBitmap(
    _Inout_ PPH_WINDOW_SNAPSHOT_CONTEXT Context,
    _In_ HDC ScreenDC
    )
{
    LONG screenWidth;
    LONG screenHeight;
    LONG smallWidth;
    LONG smallHeight;
    HDC smallDC;
    HBITMAP smallBitmap;
    HBITMAP oldSmallBitmap;
    INT oldStretchMode;

    screenWidth = Context->ScreenRect.right - Context->ScreenRect.left;
    screenHeight = Context->ScreenRect.bottom - Context->ScreenRect.top;

    if (screenWidth <= 0 || screenHeight <= 0)
        return FALSE;

    Context->BlurredDC = CreateCompatibleDC(ScreenDC);
    if (!Context->BlurredDC)
        return FALSE;

    Context->BlurredBitmap = CreateCompatibleBitmap(ScreenDC, screenWidth, screenHeight);
    if (!Context->BlurredBitmap)
        return FALSE;

    Context->OldBlurredBitmap = SelectBitmap(Context->BlurredDC, Context->BlurredBitmap);

    smallWidth = screenWidth / 24;
    smallHeight = screenHeight / 24;

    if (smallWidth < 1)
        smallWidth = 1;

    if (smallHeight < 1)
        smallHeight = 1;

    smallDC = CreateCompatibleDC(ScreenDC);
    if (!smallDC)
        return FALSE;

    smallBitmap = CreateCompatibleBitmap(ScreenDC, smallWidth, smallHeight);
    if (!smallBitmap)
    {
        DeleteDC(smallDC);
        return FALSE;
    }

    oldSmallBitmap = SelectBitmap(smallDC, smallBitmap);

    oldStretchMode = SetStretchBltMode(smallDC, HALFTONE);
    SetBrushOrgEx(smallDC, 0, 0, nullptr);

    StretchBlt(
        smallDC,
        0,
        0,
        smallWidth,
        smallHeight,
        Context->MemoryDC,
        0,
        0,
        screenWidth,
        screenHeight,
        SRCCOPY
        );

    if (oldStretchMode)
        SetStretchBltMode(smallDC, oldStretchMode);

    oldStretchMode = SetStretchBltMode(Context->BlurredDC, HALFTONE);
    SetBrushOrgEx(Context->BlurredDC, 0, 0, nullptr);

    StretchBlt(
        Context->BlurredDC,
        0,
        0,
        screenWidth,
        screenHeight,
        smallDC,
        0,
        0,
        smallWidth,
        smallHeight,
        SRCCOPY
        );

    if (oldStretchMode)
        SetStretchBltMode(Context->BlurredDC, oldStretchMode);

    SelectBitmap(smallDC, oldSmallBitmap);
    DeleteBitmap(smallBitmap);
    DeleteDC(smallDC);

    return TRUE;
}

static LRESULT CALLBACK PhWindowSnapshotSelectionWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_WINDOW_SNAPSHOT_CONTEXT context;

    if (WindowMessage == WM_CREATE)
    {
        context = (PPH_WINDOW_SNAPSHOT_CONTEXT)((CREATESTRUCT*)lParam)->lpCreateParams;
        PhSetWindowContext(WindowHandle, 0xFF, context);
    }
    else
    {
        context = (PPH_WINDOW_SNAPSHOT_CONTEXT)PhGetWindowContext(WindowHandle, 0xFF);
    }

    if (!context)
        return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);

    switch (WindowMessage)
    {
    case WM_DESTROY:
        context->IsActive = FALSE;
        PhRemoveWindowContext(WindowHandle, 0xFF);
        break;
    case WM_SHOWWINDOW:
        if (wParam)
        {
            InvalidateRect(WindowHandle, nullptr, TRUE);
            UpdateWindow(WindowHandle);
        }
        break;
    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE)
            InvalidateRect(WindowHandle, nullptr, FALSE);
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC hdc;

            hdc = BeginPaint(WindowHandle, &paint);

            if (hdc)
            {
                RECT clientRect;

                GetClientRect(WindowHandle, &clientRect);

                if (context->BlurredDC && context->BlurredBitmap)
                {
                    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, context->BlurredDC, 0, 0, SRCCOPY);
                }
                else if (context->MemoryDC && context->ScreenBitmap)
                {
                    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, context->MemoryDC, 0, 0, SRCCOPY);
                }
                else
                {
                    FillRect(hdc, &clientRect, (HBRUSH)PhGetStockBrush(BLACK_BRUSH));
                }

                if (
                    context->HighlightRect.right > context->HighlightRect.left &&
                    context->HighlightRect.bottom > context->HighlightRect.top
                    )
                {
                    RECT highlightRect;
                    HPEN pen;
                    HPEN oldPen;
                    HBRUSH oldBrush;

                    highlightRect.left = context->HighlightRect.left - context->ScreenRect.left;
                    highlightRect.top = context->HighlightRect.top - context->ScreenRect.top;
                    highlightRect.right = context->HighlightRect.right - context->ScreenRect.left;
                    highlightRect.bottom = context->HighlightRect.bottom - context->ScreenRect.top;

                    if (context->MemoryDC && context->ScreenBitmap)
                    {
                        BitBlt(
                            hdc,
                            highlightRect.left,
                            highlightRect.top,
                            highlightRect.right - highlightRect.left,
                            highlightRect.bottom - highlightRect.top,
                            context->MemoryDC,
                            highlightRect.left,
                            highlightRect.top,
                            SRCCOPY
                            );
                    }

                    pen = CreatePen(PS_SOLID, 4, RGB(0, 120, 215));
                    oldPen = SelectPen(hdc, pen);
                    oldBrush = SelectBrush(hdc, GetStockBrush(NULL_BRUSH));

                    Rectangle(
                        hdc,
                        highlightRect.left,
                        highlightRect.top,
                        highlightRect.right,
                        highlightRect.bottom
                        );

                    SelectBrush(hdc, oldBrush);
                    SelectPen(hdc, oldPen);
                    DeletePen(pen);
                }
            }

            EndPaint(WindowHandle, &paint);
        }
        return 0;
    case WM_MOUSEMOVE:
        {
            POINT point;
            HWND windowUnderCursor;
            RECT windowRect;
            RECT clippedRect;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);

            ClientToScreen(WindowHandle, &point);
            windowUnderCursor = PhWindowFromPointBelowSnapshot(WindowHandle, point);

            if (
                windowUnderCursor &&
                PhGetWindowRect(windowUnderCursor, &windowRect) &&
                PhIntersectRect(&clippedRect, &windowRect, &context->ScreenRect)
                )
            {
                if (
                    context->TargetWindow != windowUnderCursor ||
                    !EqualRect(&context->HighlightRect, &clippedRect)
                    )
                {
                    context->TargetWindow = windowUnderCursor;
                    context->HighlightRect = clippedRect;

                    InvalidateRect(WindowHandle, nullptr, FALSE);
                    UpdateWindow(WindowHandle);
                }
            }
            else if (context->TargetWindow)
            {
                context->TargetWindow = nullptr;
                memset(&context->HighlightRect, 0, sizeof(RECT));
                InvalidateRect(WindowHandle, nullptr, FALSE);
                UpdateWindow(WindowHandle);
            }
        }
        return 0;
    case WM_LBUTTONDOWN:
        {
            POINT point;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);

            ClientToScreen(WindowHandle, &point);
            context->TargetWindow = PhWindowFromPointBelowSnapshot(WindowHandle, point);

            context->IsActive = FALSE;
            DestroyWindow(WindowHandle);
        }
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            context->TargetWindow = nullptr;
            context->IsActive = FALSE;
            DestroyWindow(WindowHandle);
            return 0;
        }
        break;
    case WM_SETCURSOR:
        SetCursor(PhLoadCursor(nullptr, IDC_CROSS));
        return TRUE;
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

static BOOLEAN PhCreateWindowSnapshotSelection(
    _Out_ PPH_WINDOW_SNAPSHOT_CONTEXT Context
    )
{
    RECT screenRect;
    HDC screenDC;
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    memset(Context, 0, sizeof(PH_WINDOW_SNAPSHOT_CONTEXT));

    screenRect.left = PhGetSystemMetrics(SM_XVIRTUALSCREEN, 0);
    screenRect.top = PhGetSystemMetrics(SM_YVIRTUALSCREEN, 0);
    screenRect.right = screenRect.left + PhGetSystemMetrics(SM_CXVIRTUALSCREEN, 0);
    screenRect.bottom = screenRect.top + PhGetSystemMetrics(SM_CYVIRTUALSCREEN, 0);
    Context->ScreenRect = screenRect;

    screenDC = GetDC(nullptr);
    if (!screenDC)
        return FALSE;

    Context->MemoryDC = CreateCompatibleDC(screenDC);
    if (!Context->MemoryDC)
    {
        ReleaseDC(nullptr, screenDC);
        return FALSE;
    }

    Context->ScreenBitmap = CreateCompatibleBitmap(
        screenDC,
        screenRect.right - screenRect.left,
        screenRect.bottom - screenRect.top
        );

    if (!Context->ScreenBitmap)
    {
        PhDestroyWindowSnapshotSelection(Context);
        ReleaseDC(nullptr, screenDC);
        return FALSE;
    }

    Context->OldScreenBitmap = SelectBitmap(Context->MemoryDC, Context->ScreenBitmap);

    BitBlt(
        Context->MemoryDC,
        0,
        0,
        screenRect.right - screenRect.left,
        screenRect.bottom - screenRect.top,
        screenDC,
        screenRect.left,
        screenRect.top,
        SRCCOPY
        );

    if (!PhCreateBlurredWindowSnapshotBitmap(Context, screenDC))
    {
        ReleaseDC(nullptr, screenDC);
        PhDestroyWindowSnapshotSelection(Context);
        return FALSE;
    }

    ReleaseDC(nullptr, screenDC);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = PhWindowSnapshotSelectionWndProc;
    wcex.hCursor = PhLoadCursor(nullptr, IDC_CROSS);
    wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wcex.lpszClassName = L"PhWindowSnapshotSelectionWindow";

    if (!RegisterClassEx(&wcex) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        PhDestroyWindowSnapshotSelection(Context);
        return FALSE;
    }

    Context->SnapshotWindow = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"PhWindowSnapshotSelectionWindow",
        nullptr,
        WS_POPUP | WS_VISIBLE,
        screenRect.left,
        screenRect.top,
        screenRect.right - screenRect.left,
        screenRect.bottom - screenRect.top,
        nullptr,
        nullptr,
        nullptr,
        Context
        );

    if (!Context->SnapshotWindow)
    {
        PhDestroyWindowSnapshotSelection(Context);
        return FALSE;
    }

    Context->IsActive = TRUE;

    ShowWindow(Context->SnapshotWindow, SW_SHOWNA);
    SetWindowPos(
        Context->SnapshotWindow,
        HWND_TOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
        );
    UpdateWindow(Context->SnapshotWindow);
    SetForegroundWindow(Context->SnapshotWindow);
    SetFocus(Context->SnapshotWindow);

    return TRUE;
}

HWND PhSelectWindowFromScreenSnapshot(
    VOID
    )
{
    PH_WINDOW_SNAPSHOT_CONTEXT context;
    HWND selectedWindow;

    if (!PhCreateWindowSnapshotSelection(&context))
        return nullptr;

    while (context.IsActive)
    {
        MSG message;
        BOOL result;

        result = GetMessage(&message, nullptr, 0, 0);

        if (result <= 0)
        {
            context.IsActive = FALSE;
            break;
        }

        if (!IsWindow(context.SnapshotWindow))
        {
            context.IsActive = FALSE;
            break;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    selectedWindow = context.TargetWindow;
    PhDestroyWindowSnapshotSelection(&context);

    return selectedWindow;
}

#define PH_WINDOW_TARGETING_OVERLAY_CLASS L"PhWindowTargetingOverlayWindow"

static LRESULT CALLBACK PhWindowTargetingOverlayWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_WINDOW_TARGETING_CONTEXT context;

    if (WindowMessage == WM_CREATE)
    {
        context = static_cast<PPH_WINDOW_TARGETING_CONTEXT>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
        PhSetWindowContext(WindowHandle, 0xFF, context);
    }
    else
    {
        context = static_cast<PPH_WINDOW_TARGETING_CONTEXT>(PhGetWindowContext(WindowHandle, 0xFF));
    }

    switch (WindowMessage)
    {
    case WM_DESTROY:
        PhRemoveWindowContext(WindowHandle, 0xFF);
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC hdc;

            hdc = BeginPaint(WindowHandle, &paint);

            if (context && !PhRectEmpty(&context->TargetRect))
            {
                RECT borderRect = context->TargetRect;
                HPEN pen;
                HBRUSH brush;
                LONG oldDc;

                PhOffsetRect(&borderRect, -context->OverlayBounds.left, -context->OverlayBounds.top);
                PhInflateRect(&borderRect, 1, 1);

                oldDc = SaveDC(hdc);
                pen = CreatePen(PS_INSIDEFRAME, 2, RGB(0x60, 0x60, 0x60));
                SelectPen(hdc, pen);
                brush = PhGetStockBrush(NULL_BRUSH);
                SelectBrush(hdc, brush);
                Rectangle(hdc, borderRect.left, borderRect.top, borderRect.right, borderRect.bottom);
                DeletePen(pen);
                RestoreDC(hdc, oldDc);
            }

            EndPaint(WindowHandle, &paint);
        }
        return 0;
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

BOOLEAN PhEnsureWindowTargetingOverlayWindow(
    _Inout_ PPH_WINDOW_TARGETING_CONTEXT Context
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static RTL_ATOM windowAtom = RTL_ATOM_INVALID_ATOM;

    if (PhBeginInitOnce(&initOnce))
    {
        WNDCLASSEX wcex = { 0 };

        memset(&wcex, 0, sizeof(WNDCLASSEX));
        wcex.lpfnWndProc = PhWindowTargetingOverlayWndProc;
        wcex.lpszClassName = PH_WINDOW_TARGETING_OVERLAY_CLASS;
        wcex.hCursor = PhLoadCursor(nullptr, IDC_CROSS);

        windowAtom = RegisterClassEx(&wcex);

        PhEndInitOnce(&initOnce);
    }

    if (!Context->OverlayWindowHandle)
    {
        Context->OverlayWindowHandle = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            MAKEINTATOM(windowAtom),
            nullptr,
            WS_POPUP,
            0,
            0,
            0,
            0,
            nullptr,
            nullptr,
            nullptr,
            Context
            );

        if (!Context->OverlayWindowHandle)
            return FALSE;

        SetLayeredWindowAttributes(Context->OverlayWindowHandle, 0, 120, LWA_ALPHA);
    }

    return TRUE;
}

VOID PhUpdateWindowTargetingOverlay(
    _Inout_ PPH_WINDOW_TARGETING_CONTEXT Context,
    _In_opt_ HWND TargetWindowHandle
    )
{
    INT x;
    INT y;
    INT width;
    INT height;
    RECT targetRect = { 0 };

    if (!Context->OverlayHighlight)
        return;

    if (!PhEnsureWindowTargetingOverlayWindow(Context))
        return;

    x = PhGetSystemMetrics(SM_XVIRTUALSCREEN, 0);
    y = PhGetSystemMetrics(SM_YVIRTUALSCREEN, 0);
    width = PhGetSystemMetrics(SM_CXVIRTUALSCREEN, 0);
    height = PhGetSystemMetrics(SM_CYVIRTUALSCREEN, 0);

    Context->OverlayBounds.left = x;
    Context->OverlayBounds.top = y;
    Context->OverlayBounds.right = x + width;
    Context->OverlayBounds.bottom = y + height;

    if (TargetWindowHandle && PhGetWindowRect(TargetWindowHandle, &targetRect))
        Context->TargetRect = targetRect;
    else
        memset(&Context->TargetRect, 0, sizeof(RECT));

    SetWindowPos(
        Context->OverlayWindowHandle,
        HWND_TOPMOST,
        x,
        y,
        width,
        height,
        SWP_NOACTIVATE | SWP_SHOWWINDOW
        );

    if (!PhRectEmpty(&Context->TargetRect))
    {
        HRGN overlayRegion;
        HRGN targetRegion;

        overlayRegion = CreateRectRgn(0, 0, width, height);
        targetRegion = CreateRectRgn(
            Context->TargetRect.left - x,
            Context->TargetRect.top - y,
            Context->TargetRect.right - x,
            Context->TargetRect.bottom - y
            );

        CombineRgn(overlayRegion, overlayRegion, targetRegion, RGN_DIFF);

        if (!SetWindowRgn(Context->OverlayWindowHandle, overlayRegion, TRUE))
            DeleteRgn(overlayRegion);

        DeleteRgn(targetRegion);
    }
    else
    {
        SetWindowRgn(Context->OverlayWindowHandle, nullptr, TRUE);
    }

    ShowWindow(Context->OverlayWindowHandle, SW_SHOWNOACTIVATE);
    InvalidateRect(Context->OverlayWindowHandle, nullptr, TRUE);
}

VOID PhHideWindowTargetingOverlayForHitTest(
    _Inout_ PPH_WINDOW_TARGETING_CONTEXT Context
    )
{
    if (Context->OverlayWindowHandle)
        ShowWindow(Context->OverlayWindowHandle, SW_HIDE);
}

VOID PhDestroyWindowTargetingOverlay(
    _Inout_ PPH_WINDOW_TARGETING_CONTEXT Context
    )
{
    if (Context->OverlayWindowHandle)
    {
        DestroyWindow(Context->OverlayWindowHandle);
        Context->OverlayWindowHandle = nullptr;
    }
}

VOID PhDrawWindowBorderForTargeting(
    _In_ HWND WindowHandle
    )
{
    RECT rect;
    HDC hdc;
    LONG dpiValue;
    LONG penWidth;

    if (!PhGetWindowRect(WindowHandle, &rect))
        return;

    hdc = GetWindowDC(WindowHandle);

    if (hdc)
    {
        LONG oldDc;
        HPEN pen;
        HBRUSH brush;

        dpiValue = PhGetWindowDpi(WindowHandle);
        penWidth = PhGetSystemMetrics(SM_CXBORDER, dpiValue) * 3;

        oldDc = SaveDC(hdc);

        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        SelectPen(hdc, pen);

        brush = PhGetStockBrush(NULL_BRUSH);
        SelectBrush(hdc, brush);

        Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        DeletePen(pen);

        RestoreDC(hdc, oldDc);
        ReleaseDC(WindowHandle, hdc);
    }
}

VOID PhEndWindowTargetingVisuals(
    _Inout_ PPH_WINDOW_TARGETING_CONTEXT Context
    )
{
    if (Context->OverlayHighlight)
    {
        if (Context->OverlayWindowHandle)
            ShowWindow(Context->OverlayWindowHandle, SW_HIDE);
    }
    else if (Context->TargetWindowHandle && Context->TargetWindowDraw)
    {
        PhDrawWindowBorderForTargeting(Context->TargetWindowHandle);
    }

    Context->TargetWindowDraw = FALSE;
    memset(&Context->TargetRect, 0, sizeof(RECT));
}

VOID PhUpdateWindowTargetingVisuals(
    _Inout_ PPH_WINDOW_TARGETING_CONTEXT Context,
    _In_opt_ HWND TargetWindowHandle
    )
{
    if (Context->OverlayHighlight)
    {
        PhUpdateWindowTargetingOverlay(Context, TargetWindowHandle);
        Context->TargetWindowDraw = !!TargetWindowHandle;
    }
    else if (TargetWindowHandle)
    {
        PhDrawWindowBorderForTargeting(TargetWindowHandle);
        Context->TargetWindowDraw = TRUE;
    }
    else
    {
        Context->TargetWindowDraw = FALSE;
    }
}

VOID PhRestoreWindowTargetingOwner(
    _In_ PPH_WINDOW_TARGETING_CONTEXT Context
    )
{
    if (Context->OwnerWindowHandle && Context->OwnerWindowHandle != GetDesktopWindow())
    {
        SetWindowPos(
            Context->OwnerWindowHandle,
            Context->OwnerWindowTopMost ? HWND_TOPMOST : HWND_TOP,
            0,
            0,
            0,
            0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
            );
    }
}

PPH_WINDOW_TARGETING_CONTEXT PhCreateWindowTargeting(
    _In_opt_ HWND OwnerWindowHandle,
    _In_ BOOLEAN OverlayHighlight,
    _In_opt_ PPH_WINDOW_TARGETING_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PPH_WINDOW_TARGETING_CONTEXT targetingContext;

    targetingContext = static_cast<PPH_WINDOW_TARGETING_CONTEXT>(PhAllocate(sizeof(PH_WINDOW_TARGETING_CONTEXT)));
    memset(targetingContext, 0, sizeof(PH_WINDOW_TARGETING_CONTEXT));

    targetingContext->OwnerWindowHandle = OwnerWindowHandle ? OwnerWindowHandle : GetDesktopWindow();
    targetingContext->OwnerWindowTopMost = !!(PhGetWindowStyleEx(targetingContext->OwnerWindowHandle) & WS_EX_TOPMOST);
    targetingContext->OverlayHighlight = OverlayHighlight;
    targetingContext->Callback = Callback;
    targetingContext->CallbackContext = Context;

    SetCapture(targetingContext->OwnerWindowHandle);
    PhSetCursor(PhLoadCursor(nullptr, IDC_CROSS));

    if (OwnerWindowHandle)
    {
        SetWindowPos(
            OwnerWindowHandle,
            HWND_BOTTOM,
            0,
            0,
            0,
            0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
            );
    }

    if (OverlayHighlight)
    {
        PhUpdateWindowTargetingOverlay(targetingContext, nullptr);
    }

    return targetingContext;
}

PH_WINDOW_TARGETING_RESULT PhProcessWindowTargetingMessage(
    _Inout_ PPH_WINDOW_TARGETING_CONTEXT Context,
    _In_ UINT WindowMessage,
    _Out_opt_ HWND* TargetWindowHandle
    )
{
    if (TargetWindowHandle)
        *TargetWindowHandle = nullptr;

    switch (WindowMessage)
    {
    case WM_MOUSEMOVE:
        {
            POINT cursorPos;
            HWND windowHandle;

            if (!PhGetMessagePos(&cursorPos))
                break;

            if (Context->OverlayHighlight)
            {
                PhHideWindowTargetingOverlayForHitTest(Context);
            }

            windowHandle = WindowFromPoint(cursorPos);

            if (windowHandle && Context->Callback && !Context->Callback(windowHandle, Context->CallbackContext))
                windowHandle = nullptr;

            if (Context->TargetWindowHandle != windowHandle)
            {
                PhEndWindowTargetingVisuals(Context);
                Context->TargetWindowHandle = windowHandle;
                PhUpdateWindowTargetingVisuals(Context, windowHandle);
            }
            else if (Context->OverlayHighlight)
            {
                PhUpdateWindowTargetingVisuals(Context, windowHandle);
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            Context->Completed = TRUE;

            PhSetCursor(PhLoadCursor(nullptr, IDC_ARROW));
            PhRestoreWindowTargetingOwner(Context);
            ReleaseCapture();
            PhEndWindowTargetingVisuals(Context);

            if (TargetWindowHandle)
            {
                *TargetWindowHandle = Context->TargetWindowHandle;
            }
        }
        return PhWindowTargetingCompleted;
    case WM_CAPTURECHANGED:
        {
            if (!Context->Completed)
            {
                Context->Completed = TRUE;
                Context->TargetWindowHandle = nullptr;
                PhEndWindowTargetingVisuals(Context);
                PhRestoreWindowTargetingOwner(Context);
                return PhWindowTargetingCancelled;
            }
        }
        break;
    }

    return PhWindowTargetingContinue;
}

VOID PhDestroyWindowTargeting(
    _In_opt_ PPH_WINDOW_TARGETING_CONTEXT Context
    )
{
    if (!Context)
        return;

    PhEndWindowTargetingVisuals(Context);
    PhDestroyWindowTargetingOverlay(Context);

    if (GetCapture() == Context->OwnerWindowHandle)
        ReleaseCapture();

    PhRestoreWindowTargetingOwner(Context);
    PhFree(Context);
}

HWND PhSelectWindowFromScreenTargeting(
    _In_opt_ HWND OwnerWindowHandle,
    _In_ BOOLEAN OverlayHighlight
    )
{
    PH_WINDOW_TARGETING_CONTEXT context;

    memset(&context, 0, sizeof(PH_WINDOW_TARGETING_CONTEXT));
    context.OwnerWindowHandle = OwnerWindowHandle ? OwnerWindowHandle : GetDesktopWindow();
    context.OverlayHighlight = OverlayHighlight;

    SetCapture(context.OwnerWindowHandle);
    PhSetCursor(PhLoadCursor(nullptr, IDC_CROSS));

    if (OwnerWindowHandle)
    {
        SetWindowPos(
            OwnerWindowHandle,
            HWND_BOTTOM,
            0,
            0,
            0,
            0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
            );
    }

    PhUpdateWindowTargetingOverlay(&context, nullptr);

    while (!context.Completed)
    {
        MSG message;
        BOOL result;

        result = GetMessage(&message, nullptr, 0, 0);

        if (result <= 0)
            break;

        if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE)
        {
            context.TargetWindowHandle = nullptr;
            context.Completed = TRUE;
            break;
        }

        if (message.message == WM_MOUSEMOVE)
        {
            POINT cursorPos;
            HWND windowHandle;

            GetCursorPos(&cursorPos);

            if (OverlayHighlight)
                PhHideWindowTargetingOverlayForHitTest(&context);

            windowHandle = WindowFromPoint(cursorPos);

            if (context.TargetWindowHandle != windowHandle)
            {
                if (!OverlayHighlight && context.TargetWindowHandle)
                    PhDrawWindowBorderForTargeting(context.TargetWindowHandle);

                context.TargetWindowHandle = windowHandle;

                if (OverlayHighlight)
                    PhUpdateWindowTargetingOverlay(&context, context.TargetWindowHandle);
                else if (context.TargetWindowHandle)
                    PhDrawWindowBorderForTargeting(context.TargetWindowHandle);
            }
            else if (OverlayHighlight)
            {
                PhUpdateWindowTargetingOverlay(&context, context.TargetWindowHandle);
            }

            continue;
        }

        if (message.message == WM_LBUTTONUP || message.message == WM_LBUTTONDOWN)
        {
            POINT cursorPos;

            GetCursorPos(&cursorPos);

            if (OverlayHighlight)
                PhHideWindowTargetingOverlayForHitTest(&context);

            context.TargetWindowHandle = WindowFromPoint(cursorPos);
            context.Completed = TRUE;
            break;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    ReleaseCapture();
    PhSetCursor(PhLoadCursor(nullptr, IDC_ARROW));

    if (!OverlayHighlight && context.TargetWindowHandle)
        PhDrawWindowBorderForTargeting(context.TargetWindowHandle);

    if (OwnerWindowHandle)
    {
        SetWindowPos(
            OwnerWindowHandle,
            HWND_TOP,
            0,
            0,
            0,
            0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
            );
    }

    PhDestroyWindowTargetingOverlay(&context);

    return context.TargetWindowHandle;
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
