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

#define COMMONUTIL_PLUGIN_NAME L"ProcessHacker.CommonUtil"
#define COMMONUTIL_INTERFACE_VERSION 1

typedef VOID (NTAPI *PUTIL_CREATE_SEARCHBOX_CONTROL)(
    _In_ HWND Parent,
    _In_ HWND WindowHandle,
    _In_ UINT CommandID
    );

typedef HBITMAP (NTAPI *PUTIL_CREATE_IMAGE_FROM_RESOURCE)(
    _In_ PVOID DllBase,
    _In_ UINT Width,
    _In_ UINT Height,
    _In_ PCWSTR Name,
    _In_ BOOLEAN RGBAImage
    );

typedef struct _COMMONUTIL_INTERFACE
{
    ULONG Version;
    PUTIL_CREATE_SEARCHBOX_CONTROL CreateSearchControl;
    PUTIL_CREATE_IMAGE_FROM_RESOURCE CreateImageFromResource;
} COMMONUTIL_INTERFACE, *P_COMMONUTIL_INTERFACE;

/**
 * Creates a Ansi string using format specifiers.
 *
 * \param Format The format-control string.
 * \param ArgPtr A pointer to the list of arguments.
 */
FORCEINLINE
VOID 
CreateSearchControl(
    _In_ HWND Parent,
    _In_ HWND WindowHandle,
    _In_ UINT CommandID
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version == COMMONUTIL_INTERFACE_VERSION)
            {
                Interface->CreateSearchControl(Parent, WindowHandle, CommandID);
            }
        }
    }
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

FORCEINLINE
HFONT
CommonCreateFont(
    _In_ LONG Size,
    _In_ HWND hwnd
    )
{
    LOGFONT logFont;

    // Create the font handle
    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, 0))
    {
        HFONT fontHandle = CreateFont(
            -PhMultiplyDivideSigned(Size, PhGlobalDpi, 72),
            0,
            0,
            0,
            FW_MEDIUM,
            FALSE,
            FALSE,
            FALSE,
            ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY | ANTIALIASED_QUALITY,
            DEFAULT_PITCH,
            logFont.lfFaceName
            );

        SendMessage(hwnd, WM_SETFONT, (WPARAM)fontHandle, TRUE);

        return fontHandle;
    }

    return NULL;
}

FORCEINLINE
HFONT
CommonDuplicateFont(
    _In_ HFONT Font
    )
{
    LOGFONT logFont;

    if (GetObject(Font, sizeof(LOGFONT), &logFont))
        return CreateFontIndirect(&logFont);
    else
        return NULL;
}

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

    screenDc = CreateIC(L"DISPLAY", NULL, NULL, NULL);
    screenBitmap = CreateCompatibleBitmap(screenDc, Width, Height);

    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = BitmapHandle;
    iconInfo.hbmMask = screenBitmap;

    icon = CreateIconIndirect(&iconInfo);

    DeleteObject(screenBitmap);
    DeleteDC(screenDc);

    return icon;
}

FORCEINLINE
HBITMAP LoadImageFromResources(
    _In_ PVOID DllBase,
    _In_ UINT Width,
    _In_ UINT Height,
    _In_ PCWSTR Name,
    _In_ BOOLEAN RGBAImage
    )
{
    PUTIL_CREATE_IMAGE_FROM_RESOURCE createImageFromResourcePtr = NULL;

    if (!createImageFromResourcePtr)
    {
        PPH_PLUGIN toolStatusPlugin;

        if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
        {
            P_COMMONUTIL_INTERFACE Interface;

            if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
            {
                if (Interface->Version == COMMONUTIL_INTERFACE_VERSION)
                {
                    createImageFromResourcePtr = Interface->CreateImageFromResource;
                }
            }
        }
    }

    if (createImageFromResourcePtr)
    {
        return createImageFromResourcePtr(DllBase, Width, Height, Name, RGBAImage);
    }

    return NULL;
}

#endif