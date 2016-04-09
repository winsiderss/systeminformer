#ifndef _COMMONUTIL_H
#define _COMMONUTIL_H

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

#endif