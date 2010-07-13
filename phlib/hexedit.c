/*
 * Process Hacker - 
 *   hex editor control
 * 
 * Copyright (C) 2010 wj32
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
 */

#include <phgui.h>
#include <hexedit.h>
#include <hexeditp.h>

// Code originally from http://www.codeguru.com/Cpp/controls/editctrl/article.php/c539

BOOLEAN PhHexEditInitialization()
{
    WNDCLASSEX c = { sizeof(c) };

    c.style = 0;
    c.lpfnWndProc = PhpHexEditWndProc;
    c.cbClsExtra = 0;
    c.cbWndExtra = sizeof(PVOID);
    c.hInstance = PhInstanceHandle;
    c.hIcon = NULL;
    c.hCursor = LoadCursor(NULL, IDC_ARROW);
    c.hbrBackground = NULL;
    c.lpszMenuName = NULL;
    c.lpszClassName = PH_HEXEDIT_CLASSNAME;
    c.hIconSm = NULL;

    if (!RegisterClassEx(&c))
        return FALSE;

    return TRUE;
}

HWND PhCreateHexEditControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    )
{
    return CreateWindow(
        PH_HEXEDIT_CLASSNAME,
        L"",
        WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL,
        0,
        0,
        3,
        3,
        ParentHandle,
        (HMENU)Id,
        PhInstanceHandle,
        NULL
        );
}

VOID PhpCreateHexEditContext(
    __out PPHP_HEXEDIT_CONTEXT *Context
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
    __inout __post_invalid PPHP_HEXEDIT_CONTEXT Context
    )
{
    if (!Context->UserBuffer && Context->Data)
        PhFree(Context->Data);

    if (Context->Font) DeleteObject(Context->Font);
    PhFree(Context);
}

LRESULT CALLBACK PhpHexEditWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PPHP_HEXEDIT_CONTEXT context;

    context = (PPHP_HEXEDIT_CONTEXT)GetWindowLongPtr(hwnd, 0);

    if (uMsg == WM_CREATE)
    {
        PhpCreateHexEditContext(&context);
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)context);
    }

    if (!context)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_CREATE:
        {
            context->Font = CreateFont(-12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Courier New");
        }
        break;
    case WM_DESTROY:
        {
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
                        context->TopIndex += mult;

                        if (context->TopIndex > context->Length - mult)
                            context->TopIndex = context->Length - mult;

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
                context->TopIndex += context->BytesPerRow * -wheelDelta / WHEEL_DELTA;

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

            cursorPos.x = (LONG)(SHORT)LOWORD(lParam);
            cursorPos.y = (LONG)(SHORT)HIWORD(lParam);

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
            if (PhpHexEditHasSelected(context))
                ReleaseCapture();
        }
        break;
    case WM_MOUSEMOVE:
        {
            ULONG flags = (ULONG)wParam;
            POINT cursorPos;

            cursorPos.x = (LONG)(SHORT)LOWORD(lParam);
            cursorPos.y = (LONG)(SHORT)HIWORD(lParam);

            if (
                context->Data &&
                (flags & MK_LBUTTON) &&
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
                break;
            case VK_UP:
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
                break;
            case VK_LEFT:
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
                break;
            case VK_RIGHT:
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
        return (LPARAM)context->Data;
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
    }

DefaultHandler:
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

FORCEINLINE VOID PhpPrintHex(
    __in HDC hdc,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __inout PWCHAR Buffer,
    __in UCHAR Byte,
    __inout PLONG X,
    __inout PLONG Y,
    __inout PULONG N
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
    __in HDC hdc,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in UCHAR Byte,
    __inout PLONG X,
    __inout PLONG Y,
    __inout PULONG N
    )
{
    WCHAR c;

    c = isprint(Byte) ? Byte : '.';
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

VOID PhpHexEditOnPaint(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in PAINTSTRUCT *PaintStruct,
    __in HDC hdc
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
    WCHAR buffer[256];

    GetClientRect(hwnd, &clientRect);

    bufferDc = CreateCompatibleDC(hdc);
    bufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
    oldBufferBitmap = SelectObject(bufferDc, bufferBitmap);

    SetDCBrushColor(bufferDc, GetSysColor(COLOR_WINDOW));
    FillRect(bufferDc, &clientRect, GetStockObject(DC_BRUSH));

    SelectObject(bufferDc, Context->Font);

    SetBoundsRect(bufferDc, &clientRect, DCB_DISABLE);

    if (Context->Data)
    {
        // Get character dimensions.
        if (Context->Update)
        {
            SIZE size;

            GetCharWidth(bufferDc, '0', '0', &Context->NullWidth);
            GetTextExtentPoint32(bufferDc, L"0", 1, &size);
            Context->LineHeight = size.cy;

            Context->HexOffset = Context->ShowAddress ? (Context->AddressIsWide ? Context->NullWidth * 9 : Context->NullWidth * 5) : 0;
            Context->AsciiOffset = Context->HexOffset + (Context->ShowHex ? (Context->BytesPerRow * 3 * Context->NullWidth) : 0);

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

            Context->Update = FALSE;
            PhpHexEditUpdateScrollbars(hwnd, Context);
        }

        height = (clientRect.bottom + Context->LineHeight - 1) / Context->LineHeight * Context->LineHeight; // round up to height

        if (Context->ShowAddress)
        {
            WCHAR format[8] = L"%08x";
            ULONG w;
            RECT rect;

            format[2] = Context->AddressIsWide ? '8' : '4';
            w = Context->AddressIsWide ? 8 : 4;

            rect = clientRect;
            rect.left = Context->AddressOffset;
            rect.top = 0;

            for (i = Context->TopIndex; i < Context->Length && rect.top < height; i += Context->BytesPerRow)
            {
                swprintf(buffer, sizeof(buffer) / sizeof(WCHAR), format, i);
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

            if (Context->SelStart != -1 && (Context->CurrentMode == EDIT_HIGH || Context->CurrentMode == EDIT_LOW))
            {
                LONG selStart;
                LONG selEnd;

                selStart = Context->SelStart;
                selEnd = Context->SelEnd;

                if (selStart > selEnd)
                {
                    ULONG t;

                    t = selEnd;
                    selEnd = selStart;
                    selStart = t;
                }

                // Bytes before the selection

                for (i = Context->TopIndex; i < selStart && y < height; i++)
                {
                    PhpPrintHex(bufferDc, Context, buffer, Context->Data[i], &x, &y, &n);
                }

                // Bytes in the selection

                SetTextColor(bufferDc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetBkColor(bufferDc, GetSysColor(COLOR_HIGHLIGHT));

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

            if (Context->SelStart != -1 && Context->CurrentMode == EDIT_ASCII)
            {
                LONG selStart;
                LONG selEnd;

                selStart = Context->SelStart;
                selEnd = Context->SelEnd;

                if (selStart > selEnd)
                {
                    LONG t;

                    t = selEnd;
                    selEnd = selStart;
                    selStart = t;
                }

                // Bytes before the selection

                for (i = Context->TopIndex; i < selStart && y < height; i++)
                {
                    PhpPrintAscii(bufferDc, Context, Context->Data[i], &x, &y, &n);
                }

                // Bytes in the selection

                SetTextColor(bufferDc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetBkColor(bufferDc, GetSysColor(COLOR_HIGHLIGHT));

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
                        *p++ = isprint(Context->Data[i]) ? Context->Data[i] : '.'; 
                        i++;
                    }

                    DrawText(bufferDc, buffer, n, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
                    rect.top += Context->LineHeight;
                }
            }
        }
    }

    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, bufferDc, 0, 0, SRCCOPY);
    SelectObject(bufferDc, oldBufferBitmap);
    DeleteObject(bufferBitmap);
    DeleteDC(bufferDc);
}

VOID PhpHexEditUpdateScrollbars(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    )
{
    SCROLLINFO si = { sizeof(si) };

    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = (Context->Length / Context->BytesPerRow) - 1;
    si.nPage = Context->LinesPerPage;
    si.nPos = Context->TopIndex / Context->BytesPerRow;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

    if (si.nMax > (LONG)si.nPage)
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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    )
{
    DestroyCaret();
    CreateCaret(hwnd, NULL, Context->NullWidth * (Context->AddressIsWide ? 8 : 4), Context->LineHeight);
}

VOID PhpHexEditCreateEditCaret(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    )
{
    DestroyCaret();
    CreateCaret(hwnd, NULL, Context->NullWidth, Context->LineHeight);
}

VOID PhpHexEditRepositionCaret(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG Position
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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG X,
    __in LONG Y,
    __out POINT *Point
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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG X,
    __in LONG Y
    )
{
    switch (Context->CurrentMode)
    {
    case EDIT_NONE:
        return;
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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG S,
    __in LONG E
    )
{
    DestroyCaret();
    Context->SelStart = S;
    Context->SelEnd = E;
    REDRAW_WINDOW(hwnd);

    if (Context->EditPosition.x == 0 && Context->ShowAddress)
        PhpHexEditCreateAddressCaret(hwnd, Context);
    else
        PhpHexEditCreateEditCaret(hwnd, Context);

    SetCaretPos(Context->EditPosition.x, Context->EditPosition.y);
    ShowCaret(hwnd);
}

VOID PhpHexEditScrollTo(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG Position
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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    )
{
    if (OpenClipboard(hwnd))
    {
        EmptyClipboard();

        if (Context->CurrentMode != EDIT_ASCII)
        {
            ULONG length = Context->SelEnd - Context->SelStart;
            HGLOBAL binaryMemory;
            HGLOBAL hexMemory;

            binaryMemory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, length);
            hexMemory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, length * 3 * sizeof(WCHAR));

            if (binaryMemory)
            {
                PUCHAR p = GlobalLock(binaryMemory);
                memcpy(p, &Context->Data[Context->SelStart], length);
                GlobalUnlock(binaryMemory);

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
            asciiMemory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, length);

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
                        if (!isprint(*p))
                            *p = '.';
                        p++;
                    }

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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
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
            ULONG length = GlobalSize(memory);
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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    )
{
    Context->SelStart = 0;
    Context->SelEnd = Context->Length;
    DestroyCaret();
    REDRAW_WINDOW(hwnd);
}

VOID PhpHexEditUndoEdit(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
    )
{
    // TODO
}

VOID PhpHexEditNormalizeSel(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context
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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG S,
    __in LONG E
    )
{
    if (Context->AllowLengthChange)
    {
        PUCHAR p = PhAllocate(Context->Length - (E - S) + 1);

        memcpy(p, Context->Data, S);

        if (S < Context->Length - (E - S))
            memcpy(&p[S], &Context->Data[E], Context->Length - E);

        PhFree(Context->Data);
        PhpHexEditSetSel(hwnd, Context, -1, -1);
        Context->Data = p;
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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in LONG S,
    __in LONG L
    )
{
    if (Context->AllowLengthChange)
    {
        PUCHAR p = PhAllocate(Context->Length + L);

        memset(p, 0, Context->Length + L);
        memcpy(p, Context->Data, S);
        memcpy(&p[S + L], &Context->Data[S], Context->Length - S);

        PhFree(Context->Data);
        PhpHexEditSetSel(hwnd, Context, -1, -1);
        Context->Data = p;
        Context->Length += L;

        Context->Update = TRUE;
    }
}

VOID PhpHexEditSetBuffer(
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in PUCHAR Data,
    __in ULONG Length
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
    __in HWND hwnd,
    __in PPHP_HEXEDIT_CONTEXT Context,
    __in PUCHAR Data,
    __in ULONG Length
    )
{
    PhFree(Context->Data);
    Context->Data = PhAllocate(Length);
    memcpy(Context->Data, Data, Length);
    PhpHexEditSetBuffer(hwnd, Context, Context->Data, Length);
    Context->UserBuffer = FALSE;
    Context->AllowLengthChange = TRUE;
}
