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

#ifndef _PH_HEXEDITP_H
#define _PH_HEXEDITP_H

typedef struct _PHP_HEXEDIT_CONTEXT
{
    PUCHAR Data;
    LONG Length;
    BOOLEAN UserBuffer;
    LONG TopIndex; // index of first visible byte on screen

    LONG CurrentAddress;
    LONG CurrentMode;
    LONG SelStart;
    LONG SelEnd;

    LONG BytesPerRow;
    LONG LinesPerPage;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN ShowAddress : 1;
            BOOLEAN ShowAscii : 1;
            BOOLEAN ShowHex : 1;
            BOOLEAN AddressIsWide : 1;
            BOOLEAN AllowLengthChange : 1;
            BOOLEAN NoAddressChange : 1;
            BOOLEAN HalfPage : 1;
            BOOLEAN ExtendedUnicode : 1;
        };
    };

    HFONT Font;
    LONG LineHeight;
    LONG NullWidth;
    PWCHAR CharBuffer;
    ULONG CharBufferLength;
    BOOLEAN Update;

    LONG HexOffset;
    LONG AsciiOffset;
    LONG AddressOffset;

    BOOLEAN HasCapture;
    POINT EditPosition;
} PHP_HEXEDIT_CONTEXT, *PPHP_HEXEDIT_CONTEXT;

#define TO_HEX(Buffer, Byte) \
{ \
    *(Buffer)++ = PhIntegerToChar[(Byte) >> 4]; \
    *(Buffer)++ = PhIntegerToChar[(Byte) & 0xf]; \
}

#define REDRAW_WINDOW(hwnd) \
    RedrawWindow((hwnd), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE)

VOID PhpCreateHexEditContext(
    _Out_ PPHP_HEXEDIT_CONTEXT *Context
    );

VOID PhpFreeHexEditContext(
    _In_ _Post_invalid_ PPHP_HEXEDIT_CONTEXT Context
    );

LRESULT CALLBACK PhpHexEditWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhpHexEditUpdateMetrics(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ BOOLEAN UpdateLineHeight,
    _In_opt_ HDC hdc
    );

VOID PhpHexEditOnPaint(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ PAINTSTRUCT *PaintStruct,
    _In_ HDC hdc
    );

VOID PhpHexEditUpdateScrollbars(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    );

FORCEINLINE BOOLEAN PhpHexEditHasSelected(
    _In_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    return Context->SelStart != -1;
}

VOID PhpHexEditCreateAddressCaret(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditCreateEditCaret(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditRepositionCaret(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG Position
    );

VOID PhpHexEditCalculatePosition(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG X,
    _In_ LONG Y,
    _Out_ POINT *Point
    );

VOID PhpHexEditMove(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG X,
    _In_ LONG Y
    );

VOID PhpHexEditSetSel(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG S,
    _In_ LONG E
    );

VOID PhpHexEditScrollTo(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG Position
    );

VOID PhpHexEditClearEdit(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditCopyEdit(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditCutEdit(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditPasteEdit(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditSelectAll(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditUndoEdit(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditNormalizeSel(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditSelDelete(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG S,
    _In_ LONG E
    );

VOID PhpHexEditSelInsert(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG S,
    _In_ LONG L
    );

VOID PhpHexEditSetBuffer(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ PUCHAR Data,
    _In_ ULONG Length
    );

VOID PhpHexEditSetData(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ PUCHAR Data,
    _In_ ULONG Length
    );

#endif
