/*
 * Process Hacker Plugins - 
 *   CommonUtil Plugin
 *
 * Copyright (C) 2016 dmex
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

/**
 * Creates a Ansi string using format specifiers.
 *
 * \param Format The format-control string.
 * \param ArgPtr A pointer to the list of arguments.
 */
FORCEINLINE
PPH_BYTES 
FormatAnsiString_V(
    _In_ _Printf_format_string_ PSTR Format,
    _In_ va_list ArgPtr
    )
{
    PPH_BYTES string;
    int length;

    length = _vscprintf(Format, ArgPtr);

    if (length == -1)
        return NULL;

    string = PhCreateBytesEx(NULL, length * sizeof(CHAR));

    _vsnprintf(
        string->Buffer,
        length,
        Format, ArgPtr
        );

    return string;
}

/**
 * Creates a Ansi string using format specifiers.
 *
 * \param Format The format-control string.
 */
FORCEINLINE
PPH_BYTES 
FormatAnsiString(
    _In_ _Printf_format_string_ PSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);

    return FormatAnsiString_V(Format, argptr);
}


#endif
