/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016
 *
 */

#ifndef _COMMONUTIL_H
#define _COMMONUTIL_H

FORCEINLINE
HICON
CommonBitmapToIcon(
    _In_ HBITMAP BitmapHandle,
    _In_ INT Width,
    _In_ INT Height
    )
{
    HICON icon;
    HDC screenDc;
    HBITMAP screenBitmap;
    ICONINFO iconInfo = { 0 };

    screenDc = GetDC(NULL);
    screenBitmap = CreateCompatibleBitmap(screenDc, Width, Height);

    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = BitmapHandle;
    iconInfo.hbmMask = screenBitmap;

    icon = CreateIconIndirect(&iconInfo);

    DeleteBitmap(screenBitmap);
    ReleaseDC(NULL, screenDc);

    return icon;
}

#endif
