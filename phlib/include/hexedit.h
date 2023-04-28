/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#ifndef _PH_HEXEDIT_H
#define _PH_HEXEDIT_H

#ifdef __cplusplus
extern "C" {
#endif

#define PH_HEXEDIT_CLASSNAME L"PhHexEdit"

#define EDIT_NONE 0
#define EDIT_ASCII 1
#define EDIT_HIGH 2
#define EDIT_LOW 3

PHLIBAPI
BOOLEAN PhHexEditInitialization(
    VOID
    );

#define HEM_SETBUFFER (WM_USER + 1)
#define HEM_SETDATA (WM_USER + 2)
#define HEM_GETBUFFER (WM_USER + 3)
#define HEM_SETSEL (WM_USER + 4)
#define HEM_SETEDITMODE (WM_USER + 5)
#define HEM_SETBYTESPERROW (WM_USER + 6)
#define HEM_SETEXTENDEDUNICODE (WM_USER + 7)

#define HexEdit_SetBuffer(hWnd, Buffer, Length) \
    SendMessage((hWnd), HEM_SETBUFFER, (WPARAM)(Length), (LPARAM)(Buffer))

#define HexEdit_SetData(hWnd, Buffer, Length) \
    SendMessage((hWnd), HEM_SETDATA, (WPARAM)(Length), (LPARAM)(Buffer))

#define HexEdit_GetBuffer(hWnd, Length) \
    ((PUCHAR)SendMessage((hWnd), HEM_GETBUFFER, (WPARAM)(Length), 0))

#define HexEdit_SetSel(hWnd, Start, End) \
    SendMessage((hWnd), HEM_SETSEL, (WPARAM)(Start), (LPARAM)(End))

#define HexEdit_SetEditMode(hWnd, Mode) \
    SendMessage((hWnd), HEM_SETEDITMODE, (WPARAM)(Mode), 0)

#define HexEdit_SetBytesPerRow(hWnd, BytesPerRow) \
    SendMessage((hWnd), HEM_SETBYTESPERROW, (WPARAM)(BytesPerRow), 0)

#define HexEdit_SetExtendedUnicode(hWnd, ExtendedUnicode) \
    SendMessage((hWnd), HEM_SETEXTENDEDUNICODE, (WPARAM)(ExtendedUnicode), 0)

#ifdef __cplusplus
}
#endif

#endif
