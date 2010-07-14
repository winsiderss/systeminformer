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
    BOOLEAN ShowAddress;
    BOOLEAN ShowAscii;
    BOOLEAN ShowHex;
    BOOLEAN AddressIsWide;
    BOOLEAN AllowLengthChange;

    BOOLEAN NoAddressChange;
    BOOLEAN HalfPage;

    HFONT Font;
    LONG LineHeight;
    LONG NullWidth;
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
    __out PPHP_HEXEDIT_CONTEXT *Context
    );

VOID PhpFreeHexEditContext(
    __inout __post_invalid PPHP_HEXEDIT_CONTEXT Context
    );

LRESULT CALLBACK PhpHexEditWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhpHexEditOnPaint(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in PAINTSTRUCT *PaintStruct,
    __in HDC hdc
    );

VOID PhpHexEditUpdateScrollbars(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    );

FORCEINLINE BOOLEAN PhpHexEditHasSelected(
    __in PPHP_HEXEDIT_CONTEXT Context
    )
{
    return Context->SelStart != -1;
}

VOID PhpHexEditCreateAddressCaret(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditCreateEditCaret(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditRepositionCaret(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG Position
    );

VOID PhpHexEditCalculatePosition(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG X,
    __in LONG Y,
    __out POINT *Point
    );

VOID PhpHexEditMove(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG X,
    __in LONG Y
    );

VOID PhpHexEditSetSel(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG S,
    __in LONG E
    );

VOID PhpHexEditScrollTo(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG Position
    );

VOID PhpHexEditClearEdit(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditCopyEdit(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditCutEdit(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditPasteEdit(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditSelectAll(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditUndoEdit(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditNormalizeSel(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    );

VOID PhpHexEditSelDelete(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG S,
    __in LONG E
    );

VOID PhpHexEditSelInsert(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG S,
    __in LONG L
    );

VOID PhpHexEditSetBuffer(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in PUCHAR Data,
    __in ULONG Length
    );

VOID PhpHexEditSetData(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in PUCHAR Data,
    __in ULONG Length
    );

#endif
