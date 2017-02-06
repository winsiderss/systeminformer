/*
 * Process Hacker CommonUtil Plugin
 *   PNG image control
 *
 * Copyright (C) 2012-2016 dmex
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

#include "main.h"
#include <wincodec.h>

HBITMAP UtilLoadImageFromResources(
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
    HGLOBAL resourceHandle = NULL;
    HRSRC resourceHandleSource = NULL;
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

    // Create the ImagingFactory
    if (FAILED(CoCreateInstance(&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &wicFactory)))
        goto CleanupExit;

    // Find the resource
    if ((resourceHandleSource = FindResource(DllBase, Name, L"PNG")) == NULL)
        goto CleanupExit;

    // Get the resource length
    resourceLength = SizeofResource(DllBase, resourceHandleSource);

    // Load the resource
    if ((resourceHandle = LoadResource(DllBase, resourceHandleSource)) == NULL)
        goto CleanupExit;

    if ((resourceBuffer = (WICInProcPointer)LockResource(resourceHandle)) == NULL)
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

    screenHdc = CreateIC(L"DISPLAY", NULL, NULL, NULL);
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
        DeleteDC(screenHdc);

    if (wicBitmapSource)
        IWICBitmapSource_Release(wicBitmapSource);

    if (wicStream)
        IWICStream_Release(wicStream);

    if (wicDecoder)
        IWICBitmapDecoder_Release(wicDecoder);

    if (wicFactory)
        IWICImagingFactory_Release(wicFactory);

    if (resourceHandle)
        FreeResource(resourceHandle);

    if (success)
    {
        return bitmapHandle;
    }

    DeleteObject(bitmapHandle);
    return NULL;
}