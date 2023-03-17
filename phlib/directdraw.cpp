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
#include <wtypes.h>
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
