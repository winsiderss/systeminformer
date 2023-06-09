/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2023
 *
 */

#include <ph.h>
#include <hexedit.h>
#include <guisup.h>
#include <hexeditp.h>

// Code originally from http://www.codeguru.com/Cpp/controls/editctrl/article.php/c539

BOOLEAN PhHexEditInitialization(
    VOID
    )
{
    WNDCLASSEX c;

    memset(&c, 0, sizeof(WNDCLASSEX));
    c.cbSize = sizeof(WNDCLASSEX);
    c.lpszClassName = PH_HEXEDIT_CLASSNAME;
    c.style = CS_GLOBALCLASS;
    c.cbWndExtra = sizeof(PVOID);
    c.hInstance = PhInstanceHandle;
    c.lpfnWndProc = PhpHexEditWndProc;
    c.hCursor = PhLoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassEx(&c))
        return FALSE;

    return TRUE;
}

VOID PhpCreateHexEditContext(
    _Out_ PPHP_HEXEDIT_CONTEXT *Context
    )
{
    PPHP_HEXEDIT_CONTEXT context;

    context = PhAllocate(sizeof(PHP_HEXEDIT_CONTEXT));
    memset(context, 0, sizeof(PHP_HEXEDIT_CONTEXT)); // important, set NullWidth to 0

    context->Data = NULL;
    context->Length = 0;
    context->TopIndex = 0;
    context->BytesPerRow = 16;
    context->LinesPerPage = 1;

    context->ShowHex = TRUE;
    context->ShowAscii = TRUE;
    context->ShowAddress = TRUE;
    context->AddressIsWide = TRUE;
    context->AllowLengthChange = FALSE;

    context->AddressOffset = 0;
    context->HexOffset = 0;
    context->AsciiOffset = 0;

    context->Update = TRUE;
    context->NoAddressChange = FALSE;
    context->CurrentMode = EDIT_NONE;

    context->EditPosition.x = 0;
    context->EditPosition.y = 0;
    context->CurrentAddress = 0;
    context->HalfPage = TRUE;

    context->SelStart = -1;
    context->SelEnd = -1;

    *Context = context;
}

VOID PhpFreeHexEditContext(
    _In_ _Post_invalid_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    if (!Context->UserBuffer && Context->Data) PhFree(Context->Data);
    if (Context->CharBuffer) PhFree(Context->CharBuffer);
    if (Context->Font) DeleteFont(Context->Font);
    PhFree(Context);
}

LRESULT CALLBACK PhpHexEditWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_HEXEDIT_CONTEXT context;

    context = PhGetWindowContextEx(hwnd);

    if (uMsg == WM_CREATE)
    {
        PhpCreateHexEditContext(&context);
        PhSetWindowContextEx(hwnd, context);
    }

    if (!context)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_CREATE:
        {
            LONG dpiValue;

            dpiValue = PhGetWindowDpi(hwnd);

            context->Font = CreateFont(
                -(LONG)PhGetDpi(12, dpiValue),
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                L"Courier New"
                );
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContextEx(hwnd);
            PhpFreeHexEditContext(context);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            HDC hdc;

            if (hdc = BeginPaint(hwnd, &paintStruct))
            {
                PhpHexEditOnPaint(hwnd, context, &paintStruct, hdc);
                EndPaint(hwnd, &paintStruct);
            }
        }
        break;
    case WM_SIZE:
        {
            PhpHexEditUpdateMetrics(hwnd, context, FALSE, NULL);
        }
        break;
    case WM_SETFOCUS:
        {
            if (context->Data && !PhpHexEditHasSelected(context))
            {
                if (context->EditPosition.x == 0 && context->ShowAddress)
                    PhpHexEditCreateAddressCaret(hwnd, context);
                else
                    PhpHexEditCreateEditCaret(hwnd, context);

                SetCaretPos(context->EditPosition.x, context->EditPosition.y);
                ShowCaret(hwnd);
            }
        }
        break;
    case WM_KILLFOCUS:
        {
            DestroyCaret();
        }
        break;
    case WM_VSCROLL:
        {
            SHORT scrollRequest = LOWORD(wParam);
            LONG currentPosition;
            LONG originalTopIndex;
            SCROLLINFO scrollInfo = { sizeof(scrollInfo) };

            originalTopIndex = context->TopIndex;

            scrollInfo.fMask = SIF_TRACKPOS;
            GetScrollInfo(hwnd, SB_VERT, &scrollInfo);
            currentPosition = scrollInfo.nTrackPos;

            if (context->Data)
            {
                LONG mult;

                mult = context->LinesPerPage * context->BytesPerRow;

                switch (scrollRequest)
                {
                case SB_LINEDOWN:
                    if (context->TopIndex < context->Length - mult)
                    {
                        context->TopIndex += context->BytesPerRow;
                        REDRAW_WINDOW(hwnd);
                    }
                    break;
                case SB_LINEUP:
                    if (context->TopIndex >= context->BytesPerRow)
                    {
                        context->TopIndex -= context->BytesPerRow;
                        REDRAW_WINDOW(hwnd);
                    }
                    break;
                case SB_PAGEDOWN:
                    if (context->TopIndex < context->Length - mult)
                    {
                        LONG pageEnd = 0;

                        while (pageEnd < context->Length - mult)
                            pageEnd += context->BytesPerRow;

                        context->TopIndex += mult;

                        if (context->TopIndex > pageEnd)
                            context->TopIndex = pageEnd;

                        REDRAW_WINDOW(hwnd);
                    }
                    break;
                case SB_PAGEUP:
                    if (context->TopIndex > 0)
                    {
                        context->TopIndex -= mult;

                        if (context->TopIndex < 0)
                            context->TopIndex = 0;

                        REDRAW_WINDOW(hwnd);
                    }
                    break;
                case SB_THUMBTRACK:
                    context->TopIndex = currentPosition * context->BytesPerRow;
                    REDRAW_WINDOW(hwnd);
                    break;
                case SB_TOP:
                    context->TopIndex = 0;
                    REDRAW_WINDOW(hwnd);
                    break;
                case SB_BOTTOM:
                    while (context->TopIndex < context->Length - mult)
                        context->TopIndex += context->BytesPerRow;
                    REDRAW_WINDOW(hwnd);
                    break;
                }

                SetScrollPos(hwnd, SB_VERT, context->TopIndex / context->BytesPerRow, TRUE);

                if (!context->NoAddressChange && FALSE) // this behaviour sucks, so just leave it out
                    context->CurrentAddress += context->TopIndex - originalTopIndex;

                PhpHexEditRepositionCaret(hwnd, context, context->CurrentAddress);
            }
        }
        break;
    case WM_MOUSEWHEEL:
        {
            SHORT wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

            if (context->Data)
            {
                ULONG wheelScrollLines;
                LONG dpiValue;

                dpiValue = PhGetWindowDpi(hwnd);

                if (!PhGetSystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &wheelScrollLines, dpiValue))
                    wheelScrollLines = PhGetDpi(3, dpiValue);

                context->TopIndex += context->BytesPerRow * (LONG)wheelScrollLines * -wheelDelta / WHEEL_DELTA;

                if (context->TopIndex < 0)
                    context->TopIndex = 0;

                if (context->Length >= context->LinesPerPage * context->BytesPerRow)
                {
                    if (context->TopIndex > context->Length - context->LinesPerPage * context->BytesPerRow)
                        context->TopIndex = context->Length - context->LinesPerPage * context->BytesPerRow;
                }

                REDRAW_WINDOW(hwnd);

                SetScrollPos(hwnd, SB_VERT, context->TopIndex / context->BytesPerRow, TRUE);

                PhpHexEditRepositionCaret(hwnd, context, context->CurrentAddress);
            }
        }
        break;
    case WM_GETDLGCODE:
        if (wParam != VK_ESCAPE)
            return DLGC_WANTALLKEYS;
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_LBUTTONDOWN:
        {
            ULONG flags = (ULONG)wParam;
            POINT cursorPos;

            cursorPos.x = GET_X_LPARAM(lParam);
            cursorPos.y = GET_Y_LPARAM(lParam);

            SetFocus(hwnd);

            if (context->Data)
            {
                POINT point;

                if (wParam & MK_SHIFT)
                    context->SelStart = context->CurrentAddress;

                PhpHexEditCalculatePosition(hwnd, context, cursorPos.x, cursorPos.y, &point);

                if (point.x > -1)
                {
                    context->EditPosition = point;

                    point.x *= context->NullWidth;
                    point.y *= context->LineHeight;

                    if (point.x == 0 && context->ShowAddress)
                        PhpHexEditCreateAddressCaret(hwnd, context);
                    else
                        PhpHexEditCreateEditCaret(hwnd, context);

                    SetCaretPos(point.x, point.y);

                    if (flags & MK_SHIFT)
                    {
                        context->SelEnd = context->CurrentAddress;

                        if (context->CurrentMode == EDIT_HIGH || context->CurrentMode == EDIT_LOW)
                            context->SelEnd++;

                        REDRAW_WINDOW(hwnd);
                    }
                }

                if (!(flags & MK_SHIFT))
                {
                    if (DragDetect(hwnd, cursorPos))
                    {
                        context->SelStart = context->CurrentAddress;
                        context->SelEnd = context->SelStart;
                        SetCapture(hwnd);
                        context->HasCapture = TRUE;
                    }
                    else
                    {
                        BOOLEAN selected;

                        selected = context->SelStart != -1;
                        context->SelStart = -1;
                        context->SelEnd = -1;

                        if (selected)
                            REDRAW_WINDOW(hwnd);
                    }
                }

                if (!PhpHexEditHasSelected(context))
                    ShowCaret(hwnd);
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if (context->HasCapture && PhpHexEditHasSelected(context))
                ReleaseCapture();

            context->HasCapture = FALSE;
        }
        break;
    case WM_MOUSEMOVE:
        {
            ULONG flags = (ULONG)wParam;
            POINT cursorPos;

            cursorPos.x = GET_X_LPARAM(lParam);
            cursorPos.y = GET_Y_LPARAM(lParam);

            if (
                context->Data &&
                context->HasCapture &&
                context->SelStart != -1
                )
            {
                RECT rect;
                POINT point;
                ULONG oldSelEnd;

                // User is dragging.

                GetClientRect(hwnd, &rect);

                if (!PtInRect(&rect, cursorPos))
                {
                    if (cursorPos.y < 0)
                    {
                        SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
                        cursorPos.y = 0;
                    }
                    else if (cursorPos.y > rect.bottom)
                    {
                        SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
                        cursorPos.y = rect.bottom - 1;
                    }
                }

                oldSelEnd = context->SelEnd;
                PhpHexEditCalculatePosition(hwnd, context, cursorPos.x, cursorPos.y, &point);

                if (point.x > -1)
                {
                    context->SelEnd = context->CurrentAddress;

                    if (context->CurrentMode == EDIT_HIGH || context->CurrentMode == EDIT_LOW)
                        context->SelEnd++;
                }

                if (PhpHexEditHasSelected(context))
                    DestroyCaret();

                if (context->SelEnd != oldSelEnd)
                    REDRAW_WINDOW(hwnd);
            }
        }
        break;
    case WM_CHAR:
        {
            ULONG c = (ULONG)wParam;

            if (!context->Data)
                goto DefaultHandler;
            if (c == '\t')
                goto DefaultHandler;

            if (GetKeyState(VK_CONTROL) < 0)
            {
                switch (c)
                {
                case 0x3:
                    if (PhpHexEditHasSelected(context))
                        PhpHexEditCopyEdit(hwnd, context);
                    goto DefaultHandler;
                case 0x16:
                    PhpHexEditPasteEdit(hwnd, context);
                    goto DefaultHandler;
                case 0x18:
                    if (PhpHexEditHasSelected(context))
                        PhpHexEditCutEdit(hwnd, context);
                    goto DefaultHandler;
                case 0x1a:
                    PhpHexEditUndoEdit(hwnd, context);
                    goto DefaultHandler;
                }
            }

            // Disallow editing beyond the end of the data.
            if (context->CurrentAddress >= context->Length)
                goto DefaultHandler;

            if (c == 0x8)
            {
                if (context->CurrentAddress != 0)
                {
                    context->CurrentAddress--;
                    PhpHexEditSelDelete(hwnd, context, context->CurrentAddress, context->CurrentAddress + 1);
                    PhpHexEditRepositionCaret(hwnd, context, context->CurrentAddress);
                    REDRAW_WINDOW(hwnd);
                }

                goto DefaultHandler;
            }

            PhpHexEditSetSel(hwnd, context, -1, -1);

            switch (context->CurrentMode)
            {
            case EDIT_NONE:
                goto DefaultHandler;
            case EDIT_HIGH:
            case EDIT_LOW:
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
                {
                    ULONG b = c - '0';

                    if (b > 9)
                        b = 10 + c - 'a';

                    if (context->CurrentMode == EDIT_HIGH)
                    {
                        context->Data[context->CurrentAddress] =
                            (UCHAR)((context->Data[context->CurrentAddress] & 0x0f) | (b << 4));
                    }
                    else
                    {
                        context->Data[context->CurrentAddress] =
                            (UCHAR)((context->Data[context->CurrentAddress] & 0xf0) | b);
                    }

                    PhpHexEditMove(hwnd, context, 1, 0);
                }
                break;
            case EDIT_ASCII:
                context->Data[context->CurrentAddress] = (UCHAR)c;
                PhpHexEditMove(hwnd, context, 1, 0);
                break;
            }

            REDRAW_WINDOW(hwnd);
        }
        break;
    case WM_KEYDOWN:
        {
            ULONG vk = (ULONG)wParam;
            BOOLEAN shift = GetKeyState(VK_SHIFT) < 0;
            BOOLEAN oldNoAddressChange = context->NoAddressChange;
            BOOLEAN noScrollIntoView = FALSE;

            context->NoAddressChange = TRUE;

            switch (vk)
            {
            case VK_DOWN:
                if (context->CurrentMode != EDIT_NONE)
                {
                    if (shift)
                    {
                        if (!PhpHexEditHasSelected(context))
                            context->SelStart = context->CurrentAddress;

                        PhpHexEditMove(hwnd, context, 0, 1);
                        context->SelEnd = context->CurrentAddress;

                        if (context->CurrentMode == EDIT_HIGH || context->CurrentMode == EDIT_LOW)
                            context->SelEnd++;

                        REDRAW_WINDOW(hwnd);
                        break;
                    }
                    else
                    {
                        PhpHexEditSetSel(hwnd, context, -1, -1);
                    }

                    PhpHexEditMove(hwnd, context, 0, 1);
                    noScrollIntoView = TRUE;
                }
                else
                {
                    PhpHexEditMove(hwnd, context, 0, 1);
                }
                break;
            case VK_UP:
                if (context->CurrentMode != EDIT_NONE)
                {
                    if (shift)
                    {
                        if (!PhpHexEditHasSelected(context))
                            context->SelStart = context->CurrentAddress;

                        PhpHexEditMove(hwnd, context, 0, -1);
                        context->SelEnd = context->CurrentAddress;

                        REDRAW_WINDOW(hwnd);
                        break;
                    }
                    else
                    {
                        PhpHexEditSetSel(hwnd, context, -1, -1);
                    }

                    PhpHexEditMove(hwnd, context, 0, -1);
                    noScrollIntoView = TRUE;
                }
                else
                {
                    PhpHexEditMove(hwnd, context, 0, -1);
                }
                break;
            case VK_LEFT:
                if (context->CurrentMode != EDIT_NONE)
                {
                    if (shift)
                    {
                        if (!PhpHexEditHasSelected(context))
                            context->SelStart = context->CurrentAddress;

                        PhpHexEditMove(hwnd, context, -1, 0);
                        context->SelEnd = context->CurrentAddress;

                        REDRAW_WINDOW(hwnd);
                        break;
                    }
                    else
                    {
                        PhpHexEditSetSel(hwnd, context, -1, -1);
                    }

                    PhpHexEditMove(hwnd, context, -1, 0);
                    noScrollIntoView = TRUE;
                }
                break;
            case VK_RIGHT:
                if (context->CurrentMode != EDIT_NONE)
                {
                    if (shift)
                    {
                        if (!PhpHexEditHasSelected(context))
                            context->SelStart = context->CurrentAddress;

                        PhpHexEditMove(hwnd, context, 1, 0);
                        context->SelEnd = context->CurrentAddress;

                        if (context->CurrentMode == EDIT_HIGH || context->CurrentMode == EDIT_LOW)
                            context->SelEnd++;

                        REDRAW_WINDOW(hwnd);
                        break;
                    }
                    else
                    {
                        PhpHexEditSetSel(hwnd, context, -1, -1);
                    }

                    PhpHexEditMove(hwnd, context, 1, 0);
                    noScrollIntoView = TRUE;
                }
                break;
            case VK_PRIOR:
                if (shift)
                {
                    if (!PhpHexEditHasSelected(context))
                        context->SelStart = context->CurrentAddress;

                    SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0);
                    PhpHexEditMove(hwnd, context, 0, 0);
                    context->SelEnd = context->CurrentAddress;

                    REDRAW_WINDOW(hwnd);
                    break;
                }
                else
                {
                    PhpHexEditSetSel(hwnd, context, -1, -1);
                }

                SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0);
                PhpHexEditMove(hwnd, context, 0, 0);
                noScrollIntoView = TRUE;
                break;
            case VK_NEXT:
                if (shift)
                {
                    if (!PhpHexEditHasSelected(context))
                        context->SelStart = context->CurrentAddress;

                    SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0);
                    PhpHexEditMove(hwnd, context, 0, 0);
                    context->SelEnd = context->CurrentAddress;

                    REDRAW_WINDOW(hwnd);
                    break;
                }
                else
                {
                    PhpHexEditSetSel(hwnd, context, -1, -1);
                }

                SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0);
                PhpHexEditMove(hwnd, context, 0, 0);
                noScrollIntoView = TRUE;
                break;
            case VK_HOME:
                if (shift)
                {
                    if (!PhpHexEditHasSelected(context))
                        context->SelStart = context->CurrentAddress;

                    if (GetKeyState(VK_CONTROL) < 0)
                    {
                        SendMessage(hwnd, WM_VSCROLL, SB_THUMBTRACK, 0);
                    }
                    else
                    {
                        // Round down.
                        context->CurrentAddress /= context->BytesPerRow;
                        context->CurrentAddress *= context->BytesPerRow;
                    }

                    PhpHexEditMove(hwnd, context, 0, 0);
                    context->SelEnd = context->CurrentAddress;

                    REDRAW_WINDOW(hwnd);
                    break;
                }
                else
                {
                    PhpHexEditSetSel(hwnd, context, -1, -1);
                }

                if (GetKeyState(VK_CONTROL) < 0)
                {
                    SendMessage(hwnd, WM_VSCROLL, SB_THUMBTRACK, 0);
                    context->CurrentAddress = 0;
                }
                else
                {
                    // Round down.
                    context->CurrentAddress /= context->BytesPerRow;
                    context->CurrentAddress *= context->BytesPerRow;
                }

                PhpHexEditMove(hwnd, context, 0, 0);
                noScrollIntoView = TRUE;

                break;
            case VK_END:
                if (shift)
                {
                    if (!PhpHexEditHasSelected(context))
                        context->SelStart = context->CurrentAddress;

                    if (GetKeyState(VK_CONTROL) < 0)
                    {
                        context->CurrentAddress = context->Length - 1;
                        SendMessage(hwnd, WM_VSCROLL,
                            MAKEWPARAM(SB_THUMBTRACK, ((context->Length + (context->BytesPerRow / 2)) / context->BytesPerRow) - context->LinesPerPage),
                            0);
                    }
                    else
                    {
                        context->CurrentAddress /= context->BytesPerRow;
                        context->CurrentAddress *= context->BytesPerRow;
                        context->CurrentAddress += context->BytesPerRow - 1;

                        if (context->CurrentAddress > context->Length)
                            context->CurrentAddress = context->Length - 1;
                    }

                    PhpHexEditMove(hwnd, context, 0, 0);
                    context->SelEnd = context->CurrentAddress;

                    REDRAW_WINDOW(hwnd);
                    break;
                }
                else
                {
                    PhpHexEditSetSel(hwnd, context, -1, -1);
                }

                if (GetKeyState(VK_CONTROL) < 0)
                {
                    context->CurrentAddress = context->Length - 1;

                    if (context->HalfPage)
                    {
                        SendMessage(hwnd, WM_VSCROLL, 0, 0);
                    }
                    else
                    {
                        SendMessage(hwnd, WM_VSCROLL,
                            MAKEWPARAM(SB_THUMBTRACK, ((context->Length + (context->BytesPerRow / 2)) / context->BytesPerRow) - context->LinesPerPage),
                            0);
                    }
                }
                else
                {
                    context->CurrentAddress /= context->BytesPerRow;
                    context->CurrentAddress *= context->BytesPerRow;
                    context->CurrentAddress += context->BytesPerRow - 1;

                    if (context->CurrentAddress > context->Length)
                        context->CurrentAddress = context->Length - 1;
                }

                PhpHexEditMove(hwnd, context, 0, 0);
                noScrollIntoView = TRUE;

                break;
            case VK_INSERT:
                PhpHexEditSelInsert(hwnd, context, context->CurrentAddress,
                    max(1, context->SelEnd - context->SelStart));
                REDRAW_WINDOW(hwnd);
                break;
            case VK_DELETE:
                if (PhpHexEditHasSelected(context))
                {
                    PhpHexEditClearEdit(hwnd, context);
                }
                else
                {
                   PhpHexEditSelDelete(hwnd, context, context->CurrentAddress, context->CurrentAddress + 1);
                   REDRAW_WINDOW(hwnd);
                }
                break;
            case '\t':
                switch (context->CurrentMode)
                {
                case EDIT_NONE:
                    context->CurrentMode = EDIT_HIGH;
                    break;
                case EDIT_HIGH:
                case EDIT_LOW:
                    context->CurrentMode = EDIT_ASCII;
                    break;
                case EDIT_ASCII:
                    context->CurrentMode = EDIT_HIGH;
                    break;
                }

                PhpHexEditMove(hwnd, context, 0, 0);

                break;
            }

            // Scroll into view if not in view.
            if (
                !noScrollIntoView &&
                (context->CurrentAddress < context->TopIndex ||
                context->CurrentAddress >= context->TopIndex + context->LinesPerPage * context->BytesPerRow)
                )
            {
                PhpHexEditScrollTo(hwnd, context, context->CurrentAddress);
            }

            context->NoAddressChange = oldNoAddressChange;
        }
        break;
    case HEM_SETBUFFER:
        {
            PhpHexEditSetBuffer(hwnd, context, (PUCHAR)lParam, (ULONG)wParam);
        }
        return TRUE;
    case HEM_SETDATA:
        {
            PhpHexEditSetData(hwnd, context, (PUCHAR)lParam, (ULONG)wParam);
        }
        return TRUE;
    case HEM_GETBUFFER:
        {
            PULONG length = (PULONG)wParam;

            if (length)
                *length = context->Length;

            return (LPARAM)context->Data;
        }
    case HEM_SETSEL:
        {
            LONG selStart = (LONG)wParam;
            LONG selEnd = (LONG)lParam;

            if (selStart <= 0)
                return FALSE;
            if (selEnd > context->Length)
                return FALSE;

            PhpHexEditScrollTo(hwnd, context, selStart);
            PhpHexEditSetSel(hwnd, context, selStart, selEnd);
            PhpHexEditRepositionCaret(hwnd, context, selStart);
            REDRAW_WINDOW(hwnd);
        }
        return TRUE;
    case HEM_SETEDITMODE:
        {
            context->CurrentMode = (LONG)wParam;
            REDRAW_WINDOW(hwnd);
        }
        return TRUE;
    case HEM_SETBYTESPERROW:
        {
            LONG bytesPerRow = (LONG)wParam;

            if (bytesPerRow >= 4)
            {
                context->BytesPerRow = bytesPerRow;
                PhpHexEditUpdateMetrics(hwnd, context, TRUE, NULL);
                PhpHexEditUpdateScrollbars(hwnd, context);
                PhpHexEditScrollTo(hwnd, context, context->CurrentAddress);
                PhpHexEditRepositionCaret(hwnd, context, context->CurrentAddress);
                REDRAW_WINDOW(hwnd);
            }
        }
        return TRUE;
    case HEM_SETEXTENDEDUNICODE:
        {
            context->ExtendedUnicode = !!(LONG)wParam;
        }
        return TRUE;
    }

DefaultHandler:
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

FORCEINLINE INT PhpIsPrintable(
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ UCHAR Byte
    )
{
    if (Context->ExtendedUnicode)
        return iswctype(Byte, _PUNCT | _ALPHA | _DIGIT); // iswprint
    else
        return ((ULONG)((Byte)-' ') <= (ULONG)('~' - ' '));
}

FORCEINLINE VOID PhpPrintHex(
    _In_ HDC hdc,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _Inout_ PWCHAR Buffer,
    _In_ UCHAR Byte,
    _Inout_ PLONG X,
    _Inout_ PLONG Y,
    _Inout_ PULONG N
    )
{
    PWCHAR p = Buffer;

    TO_HEX(p, Byte);
    *p++ = ' ';
    TextOut(hdc, *X, *Y, Buffer, 3);
    *X += Context->NullWidth * 3;
    (*N)++;

    if (*N == Context->BytesPerRow)
    {
        *N = 0;
        *X = Context->HexOffset;
        *Y += Context->LineHeight;
    }
}

FORCEINLINE VOID PhpPrintAscii(
    _In_ HDC hdc,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ UCHAR Byte,
    _Inout_ PLONG X,
    _Inout_ PLONG Y,
    _Inout_ PULONG N
    )
{
    WCHAR c;

    c = PhpIsPrintable(Context, Byte) ? Byte : '.';
    TextOut(hdc, *X, *Y, &c, 1);
    *X += Context->NullWidth;
    (*N)++;

    if (*N == Context->BytesPerRow)
    {
        *N = 0;
        *X = Context->AsciiOffset;
        *Y += Context->LineHeight;
    }
}

FORCEINLINE COLORREF GetLighterHighlightColor(
    VOID
    )
{
    COLORREF color;
    UCHAR r;
    UCHAR g;
    UCHAR b;

    color = GetSysColor(COLOR_HIGHLIGHT);
    r = (UCHAR)color;
    g = (UCHAR)(color >> 8);
    b = (UCHAR)(color >> 16);

    if (r <= 255 - 64)
        r += 64;
    else
        r = 255;

    if (g <= 255 - 64)
        g += 64;
    else
        g = 255;

    if (b <= 255 - 64)
        b += 64;
    else
        b = 255;

    return RGB(r, g, b);
}

VOID PhpHexEditUpdateMetrics(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ BOOLEAN UpdateLineHeight,
    _In_opt_ HDC hdc
    )
{
    BOOLEAN freeHdc = FALSE;
    RECT clientRect;
    SIZE size;

    if (!hdc && UpdateLineHeight)
    {
        hdc = CreateCompatibleDC(hdc);
        SelectFont(hdc, Context->Font);
        freeHdc = TRUE;
    }

    GetClientRect(hwnd, &clientRect);

    if (UpdateLineHeight)
    {
        GetCharWidth(hdc, '0', '0', &Context->NullWidth);
        GetTextExtentPoint32(hdc, L"0", 1, &size);
        Context->LineHeight = size.cy;
    }

    Context->HexOffset = Context->ShowAddress ? (Context->AddressIsWide ? Context->NullWidth * 9 : Context->NullWidth * 5) : 0;
    Context->AsciiOffset = Context->HexOffset + (Context->ShowHex ? (Context->BytesPerRow * 3 * Context->NullWidth) : 0);

    if (Context->LineHeight != 0)
    {
        Context->LinesPerPage = clientRect.bottom / Context->LineHeight;
        Context->HalfPage = FALSE;

        if (Context->LinesPerPage * Context->BytesPerRow > Context->Length)
        {
            Context->LinesPerPage = (Context->Length + Context->BytesPerRow / 2) / Context->BytesPerRow;

            if (Context->Length % Context->BytesPerRow != 0)
            {
                Context->HalfPage = TRUE;
                Context->LinesPerPage++;
            }
        }
    }

    if (freeHdc && hdc)
        DeleteDC(hdc);
}

VOID PhpHexEditOnPaint(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ PAINTSTRUCT *PaintStruct,
    _In_ HDC hdc
    )
{
    RECT clientRect;
    HDC bufferDc;
    HBITMAP bufferBitmap;
    HBITMAP oldBufferBitmap;
    LONG height;
    LONG x;
    LONG y;
    LONG i;
    ULONG requiredBufferLength;
    PWCHAR buffer;

    GetClientRect(hwnd, &clientRect);

    bufferDc = CreateCompatibleDC(hdc);
    bufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
    oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

    SetDCBrushColor(bufferDc, GetSysColor(COLOR_WINDOW));
    FillRect(bufferDc, &clientRect, GetStockBrush(DC_BRUSH));
    SelectFont(bufferDc, Context->Font);
    SetBoundsRect(bufferDc, &clientRect, DCB_DISABLE);

    requiredBufferLength = (max(8, Context->BytesPerRow * 3) + 1) * sizeof(WCHAR);

    if (Context->CharBufferLength < requiredBufferLength)
    {
        if (Context->CharBuffer)
            PhFree(Context->CharBuffer);

        Context->CharBuffer = PhAllocate(requiredBufferLength);
        Context->CharBufferLength = requiredBufferLength;
        buffer = Context->CharBuffer;
    }

    buffer = Context->CharBuffer;

    if (Context->Data)
    {
        // Get character dimensions.
        if (Context->Update)
        {
            PhpHexEditUpdateMetrics(hwnd, Context, TRUE, bufferDc);
            Context->Update = FALSE;
            PhpHexEditUpdateScrollbars(hwnd, Context);
        }

        height = (clientRect.bottom + Context->LineHeight - 1) / Context->LineHeight * Context->LineHeight; // round up to height

        if (Context->ShowAddress)
        {
            PH_FORMAT format;
            ULONG w;
            RECT rect;

            PhInitFormatX(&format, 0);
            format.Type |= FormatPadZeros;
            format.Width = Context->AddressIsWide ? 8 : 4;

            w = Context->AddressIsWide ? 8 : 4;

            rect = clientRect;
            rect.left = Context->AddressOffset;
            rect.top = 0;

            for (i = Context->TopIndex; i < Context->Length && rect.top < height; i += Context->BytesPerRow)
            {
                format.u.Int32 = i;
                PhFormatToBuffer(&format, 1, buffer, requiredBufferLength, NULL);
                DrawText(bufferDc, buffer, w, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
                rect.top += Context->LineHeight;
            }
        }

        if (Context->ShowHex)
        {
            RECT rect;
            LONG n = 0;

            x = Context->HexOffset;
            y = 0;
            rect = clientRect;
            rect.left = x;
            rect.top = 0;

            if (Context->SelStart != -1)
            {
                COLORREF highlightColor;
                LONG selStart;
                LONG selEnd;

                if (Context->CurrentMode == EDIT_HIGH || Context->CurrentMode == EDIT_LOW)
                    highlightColor = GetSysColor(COLOR_HIGHLIGHT);
                else
                    highlightColor = GetLighterHighlightColor();

                selStart = Context->SelStart;
                selEnd = Context->SelEnd;

                if (selStart > selEnd)
                {
                    ULONG t;

                    t = selEnd;
                    selEnd = selStart;
                    selStart = t;
                }

                if (selStart >= Context->Length)
                    selStart = Context->Length - 1;
                if (selEnd > Context->Length)
                    selEnd = Context->Length;

                // Bytes before the selection

                for (i = Context->TopIndex; i < selStart && y < height; i++)
                {
                    PhpPrintHex(bufferDc, Context, buffer, Context->Data[i], &x, &y, &n);
                }

                // Bytes in the selection

                SetTextColor(bufferDc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetBkColor(bufferDc, highlightColor);

                for (; i < selEnd && i < Context->Length && y < height; i++)
                {
                    PhpPrintHex(bufferDc, Context, buffer, Context->Data[i], &x, &y, &n);
                }

                // Bytes after the selection

                SetTextColor(bufferDc, GetSysColor(COLOR_WINDOWTEXT));
                SetBkColor(bufferDc, GetSysColor(COLOR_WINDOW));

                for (; i < Context->Length && y < height; i++)
                {
                    PhpPrintHex(bufferDc, Context, buffer, Context->Data[i], &x, &y, &n);
                }
            }
            else
            {
                i = Context->TopIndex;

                while (i < Context->Length && rect.top < height)
                {
                    PWCHAR p = buffer;

                    for (n = 0; n < Context->BytesPerRow && i < Context->Length; n++)
                    {
                        TO_HEX(p, Context->Data[i]);
                        *p++ = ' ';
                        i++;
                    }

                    while (n < Context->BytesPerRow)
                    {
                        p[0] = ' ';
                        p[1] = ' ';
                        p[2] = ' ';
                        p += 3;
                        n++;
                    }

                    DrawText(bufferDc, buffer, Context->BytesPerRow * 3, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
                    rect.top += Context->LineHeight;
                }
            }
        }

        if (Context->ShowAscii)
        {
            RECT rect;
            LONG n = 0;

            x = Context->AsciiOffset;
            y = 0;
            rect = clientRect;
            rect.left = x;
            rect.top = 0;

            if (Context->SelStart != -1)
            {
                COLORREF highlightColor;
                LONG selStart;
                LONG selEnd;

                if (Context->CurrentMode == EDIT_ASCII)
                    highlightColor = GetSysColor(COLOR_HIGHLIGHT);
                else
                    highlightColor = GetLighterHighlightColor();

                selStart = Context->SelStart;
                selEnd = Context->SelEnd;

                if (selStart > selEnd)
                {
                    LONG t;

                    t = selEnd;
                    selEnd = selStart;
                    selStart = t;
                }

                if (selStart >= Context->Length)
                    selStart = Context->Length - 1;
                if (selEnd > Context->Length)
                    selEnd = Context->Length;

                // Bytes before the selection

                for (i = Context->TopIndex; i < selStart && y < height; i++)
                {
                    PhpPrintAscii(bufferDc, Context, Context->Data[i], &x, &y, &n);
                }

                // Bytes in the selection

                SetTextColor(bufferDc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetBkColor(bufferDc, highlightColor);

                for (; i < selEnd && i < Context->Length && y < height; i++)
                {
                    PhpPrintAscii(bufferDc, Context, Context->Data[i], &x, &y, &n);
                }

                // Bytes after the selection

                SetTextColor(bufferDc, GetSysColor(COLOR_WINDOWTEXT));
                SetBkColor(bufferDc, GetSysColor(COLOR_WINDOW));

                for (; i < Context->Length && y < height; i++)
                {
                    PhpPrintAscii(bufferDc, Context, Context->Data[i], &x, &y, &n);
                }
            }
            else
            {
                i = Context->TopIndex;

                while (i < Context->Length && rect.top < height)
                {
                    PWCHAR p = buffer;

                    for (n = 0; n < Context->BytesPerRow && i < Context->Length; n++)
                    {
                        *p++ = PhpIsPrintable(Context, Context->Data[i]) ? Context->Data[i] : '.'; // 1
                        i++;
                    }

                    DrawText(bufferDc, buffer, n, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
                    rect.top += Context->LineHeight;
                }
            }
        }
    }

    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, bufferDc, 0, 0, SRCCOPY);
    SelectBitmap(bufferDc, oldBufferBitmap);
    DeleteBitmap(bufferBitmap);
    DeleteDC(bufferDc);
}

VOID PhpHexEditUpdateScrollbars(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    SCROLLINFO si = { sizeof(si) };

    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = Context->Length / Context->BytesPerRow;
    si.nPage = Context->LinesPerPage;
    si.nPos = Context->TopIndex / Context->BytesPerRow;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

    if (si.nMax > (LONG)si.nPage - 1)
        EnableScrollBar(hwnd, SB_VERT, ESB_ENABLE_BOTH);

    // No horizontal scrollbar please.
    /*si.nMin = 0;
    si.nMax = ((Context->ShowAddress ? (Context->AddressIsWide ? 8 : 4) : 0) +
        (Context->ShowHex ? Context->BytesPerRow * 3 : 0) +
        (Context->ShowAscii ? Context->BytesPerRow : 0)) * Context->NullWidth;
    si.nPage = 1;
    si.nPos = 0;
    SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);*/
}

VOID PhpHexEditCreateAddressCaret(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    DestroyCaret();
    CreateCaret(hwnd, NULL, Context->NullWidth * (Context->AddressIsWide ? 8 : 4), Context->LineHeight);
}

VOID PhpHexEditCreateEditCaret(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    DestroyCaret();
    CreateCaret(hwnd, NULL, Context->NullWidth, Context->LineHeight);
}

VOID PhpHexEditRepositionCaret(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG Position
    )
{
    ULONG x;
    ULONG y;
    RECT rect;

    x = (Position - Context->TopIndex) % Context->BytesPerRow;
    y = (Position - Context->TopIndex) / Context->BytesPerRow;

    switch (Context->CurrentMode)
    {
    case EDIT_NONE:
        PhpHexEditCreateAddressCaret(hwnd, Context);
        x = 0;
        break;
    case EDIT_HIGH:
        PhpHexEditCreateEditCaret(hwnd, Context);
        x *= Context->NullWidth * 3;
        x += Context->HexOffset;
        break;
    case EDIT_LOW:
        PhpHexEditCreateEditCaret(hwnd, Context);
        x *= Context->NullWidth * 3;
        x += Context->NullWidth;
        x += Context->HexOffset;
        break;
    case EDIT_ASCII:
        PhpHexEditCreateEditCaret(hwnd, Context);
        x *= Context->NullWidth;
        x += Context->AsciiOffset;
        break;
    }

    Context->EditPosition.x = x;
    Context->EditPosition.y = y * Context->LineHeight;

    GetClientRect(hwnd, &rect);

    if (PtInRect(&rect, Context->EditPosition))
    {
        SetCaretPos(Context->EditPosition.x, Context->EditPosition.y);
        ShowCaret(hwnd);
    }
}

VOID PhpHexEditCalculatePosition(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG X,
    _In_ LONG Y,
    _Out_ POINT *Point
    )
{
    LONG xp;

    Y /= Context->LineHeight;

    if (Y < 0 || Y >= Context->LinesPerPage)
    {
        Point->x = -1;
        Point->y = -1;
        return;
    }

    if (Y * Context->BytesPerRow >= Context->Length)
    {
        Point->x = -1;
        Point->y = -1;
        return;
    }

    X += Context->NullWidth;
    X /= Context->NullWidth;

    if (Context->ShowAddress && X <= (Context->AddressIsWide ? 8 : 4))
    {
        Context->CurrentAddress = Context->TopIndex + Context->BytesPerRow * Y;
        Context->CurrentMode = EDIT_NONE;

        Point->x = 0;
        Point->y = Y;
        return;
    }

    xp = Context->HexOffset / Context->NullWidth + Context->BytesPerRow * 3;

    if (Context->ShowHex && X < xp)
    {
        if (X % 3)
            X--;

        Context->CurrentAddress = Context->TopIndex +
            Context->BytesPerRow * Y +
            (X - (Context->HexOffset / Context->NullWidth)) / 3;
        Context->CurrentMode = ((X % 3) & 1) ? EDIT_LOW : EDIT_HIGH;

        Point->x = X;
        Point->y = Y;
        return;
    }

    X--; // fix selection problem

    xp = Context->AsciiOffset / Context->NullWidth + Context->BytesPerRow;

    if (Context->ShowAscii && X * Context->NullWidth >= Context->AsciiOffset && X <= xp)
    {
        Context->CurrentAddress = Context->TopIndex +
            Context->BytesPerRow * Y +
            (X - (Context->AsciiOffset / Context->NullWidth));
        Context->CurrentMode = EDIT_ASCII;

        Point->x = X;
        Point->y = Y;
        return;
    }

    Point->x = -1;
    Point->y = -1;
}

VOID PhpHexEditMove(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG X,
    _In_ LONG Y
    )
{
    switch (Context->CurrentMode)
    {
    case EDIT_NONE:
        Context->CurrentAddress += Y * Context->BytesPerRow;
        break;
    case EDIT_HIGH:
        if (X != 0)
            Context->CurrentMode = EDIT_LOW;
        if (X == -1)
            Context->CurrentAddress--;
        Context->CurrentAddress += Y * Context->BytesPerRow;
        break;
    case EDIT_LOW:
        if (X != 0)
            Context->CurrentMode = EDIT_HIGH;
        if (X == 1)
            Context->CurrentAddress++;
        Context->CurrentAddress += Y * Context->BytesPerRow;
        break;
    case EDIT_ASCII:
        Context->CurrentAddress += X;
        Context->CurrentAddress += Y * Context->BytesPerRow;
        break;
    }

    if (Context->CurrentAddress < 0)
        Context->CurrentAddress = 0;

    if (Context->CurrentAddress >= Context->Length)
    {
        Context->CurrentAddress -= X;
        Context->CurrentAddress -= Y * Context->BytesPerRow;
    }

    Context->NoAddressChange = TRUE;

    if (Context->CurrentAddress < Context->TopIndex)
        SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
    if (Context->CurrentAddress >= Context->TopIndex + Context->LinesPerPage * Context->BytesPerRow)
        SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);

    Context->NoAddressChange = FALSE;
    PhpHexEditRepositionCaret(hwnd, Context, Context->CurrentAddress);
}

VOID PhpHexEditSetSel(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG S,
    _In_ LONG E
    )
{
    DestroyCaret();
    Context->SelStart = S;
    Context->SelEnd = E;
    REDRAW_WINDOW(hwnd);

    if (S != -1 && E != -1)
    {
        Context->CurrentAddress = S;
    }
    else
    {
        if (Context->EditPosition.x == 0 && Context->ShowAddress)
            PhpHexEditCreateAddressCaret(hwnd, Context);
        else
            PhpHexEditCreateEditCaret(hwnd, Context);

        SetCaretPos(Context->EditPosition.x, Context->EditPosition.y);
        ShowCaret(hwnd);
    }
}

VOID PhpHexEditScrollTo(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG Position
    )
{
    if (Position < Context->TopIndex || Position > Context->TopIndex + Context->LinesPerPage * Context->BytesPerRow)
    {
        Context->TopIndex = Position / Context->BytesPerRow * Context->BytesPerRow; // round down
        Context->TopIndex -= Context->LinesPerPage / 3 * Context->BytesPerRow;

        if (Context->TopIndex < 0)
            Context->TopIndex = 0;

        if (Context->Length >= Context->LinesPerPage * Context->BytesPerRow)
        {
            if (Context->TopIndex > Context->Length - Context->LinesPerPage * Context->BytesPerRow)
                Context->TopIndex = Context->Length - Context->LinesPerPage * Context->BytesPerRow;
        }

        PhpHexEditUpdateScrollbars(hwnd, Context);
        REDRAW_WINDOW(hwnd);
    }
}

VOID PhpHexEditClearEdit(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    if (Context->AllowLengthChange)
    {
        Context->CurrentAddress = Context->SelStart;
        PhpHexEditSelDelete(hwnd, Context, Context->SelStart, Context->SelEnd);
        PhpHexEditRepositionCaret(hwnd, Context, Context->CurrentAddress);
        REDRAW_WINDOW(hwnd);
    }
}

VOID PhpHexEditCopyEdit(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    if (OpenClipboard(hwnd))
    {
        EmptyClipboard();
        PhpHexEditNormalizeSel(hwnd, Context);

        if (Context->CurrentMode != EDIT_ASCII)
        {
            ULONG length = Context->SelEnd - Context->SelStart;
            HGLOBAL binaryMemory;
            HGLOBAL hexMemory;

            binaryMemory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, length);

            if (binaryMemory)
            {
                PUCHAR p = GlobalLock(binaryMemory);
                memcpy(p, &Context->Data[Context->SelStart], length);
                GlobalUnlock(binaryMemory);

                hexMemory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (length * 3 + 1) * sizeof(WCHAR));

                if (hexMemory)
                {
                    PWCHAR pw;
                    ULONG i;

                    pw = GlobalLock(hexMemory);

                    for (i = 0; i < length; i++)
                    {
                        TO_HEX(pw, Context->Data[Context->SelStart + i]);
                        *pw++ = ' ';
                    }
                    *pw = 0;

                    GlobalUnlock(hexMemory);

                    SetClipboardData(CF_UNICODETEXT, hexMemory);
                }

                SetClipboardData(RegisterClipboardFormat(L"BinaryData"), binaryMemory);
            }
        }
        else
        {
            ULONG length = Context->SelEnd - Context->SelStart;
            HGLOBAL binaryMemory;
            HGLOBAL asciiMemory;

            binaryMemory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, length);
            asciiMemory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, length + 1);

            if (binaryMemory)
            {
                PUCHAR p = GlobalLock(binaryMemory);
                memcpy(p, &Context->Data[Context->SelStart], length);
                GlobalUnlock(binaryMemory);

                if (asciiMemory)
                {
                    ULONG i;

                    p = GlobalLock(asciiMemory);
                    memcpy(p, &Context->Data[Context->SelStart], length);

                    for (i = 0; i < length; i++)
                    {
                        if (!PhpIsPrintable(Context, *p))
                            *p = '.';
                        p++;
                    }
                    *p = 0;

                    GlobalUnlock(asciiMemory);

                    SetClipboardData(CF_TEXT, asciiMemory);
                }

                SetClipboardData(RegisterClipboardFormat(L"BinaryData"), binaryMemory);
            }
        }

        CloseClipboard();
    }
}

VOID PhpHexEditCutEdit(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    if (Context->AllowLengthChange)
    {
        PhpHexEditCopyEdit(hwnd, Context);
        PhpHexEditSelDelete(hwnd, Context, Context->SelStart, Context->SelEnd);
        REDRAW_WINDOW(hwnd);
    }
}

VOID PhpHexEditPasteEdit(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    if (OpenClipboard(hwnd))
    {
        HANDLE memory;

        memory = GetClipboardData(RegisterClipboardFormat(L"BinaryData"));

        if (!memory)
            memory = GetClipboardData(CF_TEXT);

        if (memory)
        {
            PUCHAR p = GlobalLock(memory);
            ULONG length = (ULONG)GlobalSize(memory);
            ULONG paste;
            ULONG oldCurrentAddress = Context->CurrentAddress;

            PhpHexEditNormalizeSel(hwnd, Context);

            if (Context->AllowLengthChange)
            {
                if (Context->SelStart == -1)
                {
                    if (Context->CurrentMode == EDIT_LOW)
                        Context->CurrentAddress++;

                    paste = Context->CurrentAddress;
                    PhpHexEditSelInsert(hwnd, Context, Context->CurrentAddress, length);
                }
                else
                {
                    paste = Context->SelStart;
                    PhpHexEditSelDelete(hwnd, Context, Context->SelStart, Context->SelEnd);
                    PhpHexEditSelInsert(hwnd, Context, paste, length);
                    PhpHexEditSetSel(hwnd, Context, -1, -1);
                }
            }
            else
            {
                if (Context->SelStart == -1)
                {
                    if (Context->CurrentMode == EDIT_LOW)
                        Context->CurrentAddress++;

                    paste = Context->CurrentAddress;
                }
                else
                {
                    paste = Context->SelStart;
                }

                if (length > Context->Length - paste)
                    length = Context->Length - paste;
            }

            memcpy(&Context->Data[paste], p, length);
            GlobalUnlock(memory);

            Context->CurrentAddress = oldCurrentAddress;
            REDRAW_WINDOW(hwnd);
        }

        CloseClipboard();
    }
}

VOID PhpHexEditSelectAll(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    Context->SelStart = 0;
    Context->SelEnd = Context->Length;
    DestroyCaret();
    REDRAW_WINDOW(hwnd);
}

VOID PhpHexEditUndoEdit(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    // TODO
}

VOID PhpHexEditNormalizeSel(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context
    )
{
    if (Context->SelStart > Context->SelEnd)
    {
        LONG t;

        t = Context->SelEnd;
        Context->SelEnd = Context->SelStart;
        Context->SelStart = t;
    }
}

VOID PhpHexEditSelDelete(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG S,
    _In_ LONG E
    )
{
    if (Context->AllowLengthChange && Context->Length > 0)
    {
        PUCHAR p = PhAllocate(Context->Length - (E - S) + 1);

        memcpy(p, Context->Data, S);

        if (S < Context->Length - (E - S))
            memcpy(&p[S], &Context->Data[E], Context->Length - E);

        PhFree(Context->Data);
        Context->Data = p;
        PhpHexEditSetSel(hwnd, Context, -1, -1);
        Context->Length -= E - S;

        if (Context->CurrentAddress > Context->Length)
        {
            Context->CurrentAddress = Context->Length;
            PhpHexEditRepositionCaret(hwnd, Context, Context->CurrentAddress);
        }

        Context->Update = TRUE;
    }
}

VOID PhpHexEditSelInsert(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ LONG S,
    _In_ LONG L
    )
{
    if (Context->AllowLengthChange)
    {
        PUCHAR p = PhAllocate(Context->Length + L);

        memset(p, 0, Context->Length + L);
        memcpy(p, Context->Data, S);
        memcpy(&p[S + L], &Context->Data[S], Context->Length - S);

        PhFree(Context->Data);
        Context->Data = p;
        PhpHexEditSetSel(hwnd, Context, -1, -1);
        Context->Length += L;

        Context->Update = TRUE;
    }
}

VOID PhpHexEditSetBuffer(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ PUCHAR Data,
    _In_ ULONG Length
    )
{
    Context->Data = Data;
    PhpHexEditSetSel(hwnd, Context, -1, -1);
    Context->Length = Length;
    Context->CurrentAddress = 0;
    Context->EditPosition.x = Context->EditPosition.y = 0;
    Context->CurrentMode = EDIT_HIGH;
    Context->TopIndex = 0;
    Context->Update = TRUE;

    Context->UserBuffer = TRUE;
    Context->AllowLengthChange = FALSE;
}

VOID PhpHexEditSetData(
    _In_ HWND hwnd,
    _In_ PPHP_HEXEDIT_CONTEXT Context,
    _In_ PUCHAR Data,
    _In_ ULONG Length
    )
{
    if (Context->Data) PhFree(Context->Data);
    Context->Data = PhAllocate(Length);
    memcpy(Context->Data, Data, Length);
    PhpHexEditSetBuffer(hwnd, Context, Context->Data, Length);
    Context->UserBuffer = FALSE;
    Context->AllowLengthChange = TRUE;
}
