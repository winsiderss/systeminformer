/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2016-2026
 *
 */

#include <ph.h>
#include <uxtheme.h>
#include <mapldr.h>
#include <guisup.h>
#include <phintrin.h>

// code from http://msdn.microsoft.com/en-us/library/bb757020.aspx

typedef HPAINTBUFFER (WINAPI* _BeginBufferedPaint)(
    _In_ HDC hdcTarget,
    _In_ const RECT *prcTarget,
    _In_ BP_BUFFERFORMAT dwFormat,
    _In_ BP_PAINTPARAMS *pPaintParams,
    _Out_ HDC *phdc
    );

typedef HRESULT (WINAPI* _EndBufferedPaint)(
    _In_ HPAINTBUFFER hBufferedPaint,
    _In_ BOOL fUpdateTarget
    );

typedef HRESULT (WINAPI* _GetBufferedPaintBits)(
    _In_ HPAINTBUFFER hBufferedPaint,
    _Out_ RGBQUAD **ppbBuffer,
    _Out_ int *pcxRow
    );

static BOOLEAN ImportsInitialized = FALSE;
static _BeginBufferedPaint BeginBufferedPaint_I = NULL;
static _EndBufferedPaint EndBufferedPaint_I = NULL;
static _GetBufferedPaintBits GetBufferedPaintBits_I = NULL;

static HBITMAP PhpCreateBitmap32(
    _In_ HDC hdc,
    _In_ LONG Width,
    _In_ LONG Height,
    _Out_ PVOID *Bits
    )
{
    BITMAPINFO bitmapInfo;

    memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = Width;
    bitmapInfo.bmiHeader.biHeight = Height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = Width * Height * sizeof(RGBQUAD);

    return CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, Bits, NULL, 0);
}

static BOOLEAN PhpHasAlpha(
    _In_ PULONG Argb,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ ULONG RowWidth
    )
{
    ULONG delta;
    ULONG x;
    ULONG y;

    delta = RowWidth - Width;

    for (y = Height; y; y--)
    {
        x = Width;

#ifndef _ARM64_
        if (PhHasAVX)
        {
            __m256i alpha = _mm256_set1_epi32((int)0xff000000);

            while (x >= 8)
            {
                __m256i v = _mm256_loadu_si256((__m256i const *)Argb);
                __m256i masked = _mm256_and_si256(v, alpha);

                if (!_mm256_testz_si256(masked, masked))
                {
                    _mm256_zeroupper();
                    return TRUE;
                }

                Argb += 8;
                x -= 8;
            }

            _mm256_zeroupper();
        }
#endif

        for (; x; x--)
        {
            if (*Argb++ & 0xff000000)
                return TRUE;
        }

        Argb += delta;
    }

    return FALSE;
}

static VOID PhpConvertToPArgb32(
    _In_ HDC hdc,
    _Inout_ PULONG Argb,
    _In_ HBITMAP Bitmap,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ ULONG RowWidth
    )
{
    BITMAPINFO bitmapInfo;
    PVOID bits;

    memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = Width;
    bitmapInfo.bmiHeader.biHeight = Height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = Width * Height * sizeof(RGBQUAD);

    bits = PhAllocate(Width * sizeof(RGBQUAD) * Height);

    if (GetDIBits(hdc, Bitmap, 0, Height, bits, &bitmapInfo, DIB_RGB_COLORS) == Height)
    {
        PULONG argbMask;
        ULONG delta;
        ULONG x;
        ULONG y;

        argbMask = (PULONG)bits;
        delta = RowWidth - Width;

        for (y = Height; y; y--)
        {
            x = Width;

#ifndef _ARM64_
            if (PhHasAVX)
            {
                // Where the mask pixel is zero, OR in opaque alpha; otherwise the
                // result is zero (transparent). cmpeq yields all-ones for mask==0
                // and is AND'd against the opaque value to select per lane.

                __m256i zero = _mm256_setzero_si256();
                __m256i alpha = _mm256_set1_epi32((int)0xff000000);

                while (x >= 8)
                {
                    __m256i mask;
                    __m256i argv;
                    __m256i maskZero;
                    __m256i result;

                    mask = _mm256_loadu_si256((__m256i const *)argbMask);
                    argv = _mm256_loadu_si256((__m256i const *)Argb);
                    maskZero = _mm256_cmpeq_epi32(mask, zero);
                    result = _mm256_and_si256(_mm256_or_si256(argv, alpha), maskZero);
                    _mm256_storeu_si256((__m256i *)Argb, result);

                    argbMask += 8;
                    Argb += 8;
                    x -= 8;
                }

                _mm256_zeroupper();
            }
#endif

            for (; x; x--)
            {
                if (*argbMask++)
                {
                    *Argb++ = 0; // transparent
                }
                else
                {
                    *Argb++ |= 0xff000000; // opaque
                }
            }

            Argb += delta;
        }
    }

    PhFree(bits);
}

static VOID PhpConvertToPArgb32IfNeeded(
    _In_ HPAINTBUFFER PaintBuffer,
    _In_ HDC hdc,
    _In_ HICON Icon,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    RGBQUAD *quad;
    ULONG rowWidth;

    if (SUCCEEDED(GetBufferedPaintBits_I(PaintBuffer, &quad, &rowWidth)))
    {
        PULONG argb = (PULONG)quad;

        if (!PhpHasAlpha(argb, Width, Height, rowWidth))
        {
            ICONINFO iconInfo;

            if (GetIconInfo(Icon, &iconInfo))
            {
                if (iconInfo.hbmMask)
                {
                    PhpConvertToPArgb32(hdc, argb, iconInfo.hbmMask, Width, Height, rowWidth);
                    DeleteBitmap(iconInfo.hbmMask);
                }

                DeleteBitmap(iconInfo.hbmColor);
            }
        }
    }
}

HBITMAP PhIconToBitmap(
    _In_ HICON Icon,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    HBITMAP bitmap;
    RECT iconRectangle;
    HDC screenHdc;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    BLENDFUNCTION blendFunction = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    BP_PAINTPARAMS paintParams = { sizeof(paintParams) };
    HDC bufferHdc;
    HPAINTBUFFER bufferedPaint;

    iconRectangle.left = 0;
    iconRectangle.top = 0;
    iconRectangle.right = Width;
    iconRectangle.bottom = Height;

    if (!ImportsInitialized)
    {
        PVOID uxtheme;

        uxtheme = PhLoadLibrary(L"uxtheme.dll");
        BeginBufferedPaint_I = PhGetDllBaseProcedureAddress(uxtheme, "BeginBufferedPaint", 0);
        EndBufferedPaint_I = PhGetDllBaseProcedureAddress(uxtheme, "EndBufferedPaint", 0);
        GetBufferedPaintBits_I = PhGetDllBaseProcedureAddress(uxtheme, "GetBufferedPaintBits", 0);
        ImportsInitialized = TRUE;
    }

    if (!BeginBufferedPaint_I || !EndBufferedPaint_I || !GetBufferedPaintBits_I)
    {
        // Probably XP.

        screenHdc = GetDC(NULL);
        hdc = CreateCompatibleDC(screenHdc);
        bitmap = CreateCompatibleBitmap(screenHdc, Width, Height);
        ReleaseDC(NULL, screenHdc);

        oldBitmap = SelectObject(hdc, bitmap);
        FillRect(hdc, &iconRectangle, (HBRUSH)(COLOR_WINDOW + 1));
        DrawIconEx(hdc, 0, 0, Icon, Width, Height, 0, NULL, DI_NORMAL);
        SelectObject(hdc, oldBitmap);

        DeleteDC(hdc);

        return bitmap;
    }

    hdc = CreateCompatibleDC(NULL);
    bitmap = PhpCreateBitmap32(hdc, Width, Height, &bits);
    oldBitmap = SelectBitmap(hdc, bitmap);

    paintParams.dwFlags = BPPF_ERASE;
    paintParams.pBlendFunction = &blendFunction;

    if (bufferedPaint = BeginBufferedPaint_I(hdc, &iconRectangle, BPBF_DIB, &paintParams, &bufferHdc))
    {
        DrawIconEx(bufferHdc, 0, 0, Icon, Width, Height, 0, NULL, DI_NORMAL);
        // If the icon did not have an alpha channel, we need to convert the buffer to PARGB.
        PhpConvertToPArgb32IfNeeded(bufferedPaint, hdc, Icon, Width, Height);
        // This will write the buffer contents to the destination bitmap.
        EndBufferedPaint_I(bufferedPaint, TRUE);
    }
    else
    {
        // Default to unbuffered painting.
        FillRect(hdc, &iconRectangle, (HBRUSH)(COLOR_WINDOW + 1));
        DrawIconEx(hdc, 0, 0, Icon, Width, Height, 0, NULL, DI_NORMAL);
    }

    SelectBitmap(hdc, oldBitmap);
    DeleteDC(hdc);

    return bitmap;
}

// based on BufferedPaintSetAlpha/BufferedPaintMakeOpaque (dmex)
VOID PhBitmapSetAlpha(
    _In_ PVOID Bits,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    ULONG count = Width * Height;
    PULONG pixels = (PULONG)Bits;

#ifndef _ARM64_
    if (PhHasAVX && count >= 8)
    {
        // Set the alpha byte to 0xff for every non-zero pixel. Where the pixel
        // is zero, cmpeq yields all-ones and andnot clears the alpha mask so the
        // pixel stays zero; elsewhere alpha bits are OR'd in.

        __m256i zero = _mm256_setzero_si256();
        __m256i alpha = _mm256_set1_epi32((int)0xff000000);

        while (count >= 8)
        {
            __m256i argv;
            __m256i opaque;

            argv = _mm256_loadu_si256((__m256i const *)pixels);
            opaque = _mm256_andnot_si256(_mm256_cmpeq_epi32(argv, zero), alpha);
            _mm256_storeu_si256((__m256i *)pixels, _mm256_or_si256(argv, opaque));

            pixels += 8;
            count -= 8;
        }

        _mm256_zeroupper();
    }
#endif

    if (PhHasIntrinsics && count >= 4)
    {
        PH_INT128 zero = PhSetZeroINT128();
        PH_INT128 alpha = PhSetINT128by32((int)0xff000000);

        while (count >= 4)
        {
            PH_INT128 argv;
            PH_INT128 opaque;

            argv = PhLoadINT128U((const PLONG)pixels);
            opaque = PhAndNotINT128(PhCompareEqINT128by32(argv, zero), alpha);
            PhStoreINT128U((PLONG)pixels, PhOrINT128(argv, opaque));

            pixels += 4;
            count -= 4;
        }
    }

    for (; count; count--)
    {
        if (*pixels != 0)
        {
            *pixels |= 0xff000000; // opaque
        }

        pixels++;
    }
}
