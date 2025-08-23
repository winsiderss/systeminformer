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
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    HICON icon;
    ICONINFO iconInfo = { 0 };
    HBITMAP maskBitmap;

    maskBitmap = CreateBitmap(Width, Height, 1, 1, NULL);

    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = BitmapHandle;
    iconInfo.hbmMask = maskBitmap;

    icon = CreateIconIndirect(&iconInfo);
    DeleteBitmap(maskBitmap);

    return icon;
}

#endif
