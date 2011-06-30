/*
 * Process Hacker - 
 *   tree list
 * 
 * Copyright (C) 2011 wj32
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
#include <windowsx.h>
#include <treenew.h>
#include <treenewp.h>

static ULONG SmallIconWidth;
static ULONG SmallIconHeight;

BOOLEAN PhTreeNewInitialization()
{
    WNDCLASSEX c = { sizeof(c) };

    c.style = CS_GLOBALCLASS;
    c.lpfnWndProc = PhTnpWndProc;
    c.cbClsExtra = 0;
    c.cbWndExtra = sizeof(PVOID);
    c.hInstance = PhLibImageBase;
    c.hIcon = NULL;
    c.hCursor = LoadCursor(NULL, IDC_ARROW);
    c.hbrBackground = NULL;
    c.lpszMenuName = NULL;
    c.lpszClassName = PH_TREENEW_CLASSNAME;
    c.hIconSm = NULL;

    if (!RegisterClassEx(&c))
        return FALSE;

    SmallIconWidth = GetSystemMetrics(SM_CXSMICON);
    SmallIconHeight = GetSystemMetrics(SM_CYSMICON);

    return TRUE;
}

LRESULT CALLBACK PhTnpWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PPH_TREENEW_CONTEXT context;

    context = (PPH_TREENEW_CONTEXT)GetWindowLongPtr(hwnd, 0);

    if (uMsg == WM_CREATE)
    {
        PhTnpCreateTreeNewContext(&context);
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)context);
    }

    if (!context)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_CREATE:
        {
            PhTnpOnCreate(hwnd, context, (CREATESTRUCT *)lParam);
        }
        return 0;
    case WM_DESTROY:
        {
            PhTnpDestroyTreeNewContext(context);
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)NULL);
        }
        break;
    case WM_SIZE:
        {
            PhTnpOnSize(hwnd, context);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        { 
            PAINTSTRUCT paintStruct;
            HDC hdc;

            if (hdc = BeginPaint(hwnd, &paintStruct))
            {
                PhTnpOnPaint(hwnd, context, &paintStruct, hdc);
                EndPaint(hwnd, &paintStruct);
            }
        }
        break;
    case WM_GETFONT:
        {
            return (LRESULT)context->Font;
        }
        break;
    case WM_SETFONT:
        {
            PhTnpOnSetFont(hwnd, context, (HFONT)wParam, LOWORD(lParam));
        }
        break;
    case WM_THEMECHANGED:
        {
            // TODO
        }
        break;
    case WM_SETTINGCHANGE:
        {
            // TODO
        }
        break;
    case WM_SETCURSOR:
        {
            if (PhTnpOnSetCursor(hwnd, context, (HWND)wParam))
                return TRUE;
        }
        break;
    case WM_SETFOCUS:
        {
            // TODO
        }
        break;
    case WM_MOUSEMOVE:
        {
            PhTnpOnMouseMove(hwnd, context, (ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
        {
            PhTnpOnXxxButtonXxx(hwnd, context, uMsg, (ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_CAPTURECHANGED:
        {
            PhTnpOnCaptureChanged(hwnd, context);
        }
        break;
    case WM_KEYDOWN:
        {
            PhTnpOnKeyDown(hwnd, context, (ULONG)wParam, (ULONG)lParam);
        }
        break;
    case WM_NOTIFY:
        {
            LRESULT result;

            if (PhTnpOnNotify(hwnd, context, (NMHDR *)lParam, &result))
                return result;
        }
        break;
    }

    if (context->Tracking && (GetAsyncKeyState(VK_ESCAPE) & 0x1))
    {
        PhTnpCancelTrack(context);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

VOID PhTnpCreateTreeNewContext(
    __out PPH_TREENEW_CONTEXT *Context
    )
{
    PPH_TREENEW_CONTEXT context;

    context = PhAllocate(sizeof(PH_TREENEW_CONTEXT));
    memset(context, 0, sizeof(PH_TREENEW_CONTEXT));

    context->FixedWidthMinimum = 20;

    *Context = context;
}

VOID PhTnpDestroyTreeNewContext(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    PhFree(Context);
}

VOID PhTnpOnCreate(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in CREATESTRUCT *CreateStruct
    )
{
    Context->Handle = hwnd;
    Context->InstanceHandle = CreateStruct->hInstance;
    Context->FixedHeaderHandle = CreateWindow(
        WC_HEADER,
        NULL,
        WS_CHILD | WS_VISIBLE | HDS_HORZ | HDS_FULLDRAG | HDS_BUTTONS,
        0,
        0,
        0,
        0,
        hwnd,
        NULL,
        CreateStruct->hInstance,
        NULL
        );
    Context->HeaderHandle = CreateWindow(
        WC_HEADER,
        NULL,
        WS_CHILD | WS_VISIBLE | HDS_HORZ | HDS_FULLDRAG | HDS_BUTTONS | HDS_DRAGDROP,
        0,
        0,
        0,
        0,
        hwnd,
        NULL,
        CreateStruct->hInstance,
        NULL
        );

    Context->VScrollWidth = GetSystemMetrics(SM_CXVSCROLL);
    Context->HScrollHeight = GetSystemMetrics(SM_CYHSCROLL);

    Context->VScrollHandle = CreateWindow(
        L"SCROLLBAR",
        NULL,
        WS_CHILD | WS_VISIBLE | SBS_VERT,
        0,
        0,
        0,
        0,
        hwnd,
        NULL,
        CreateStruct->hInstance,
        NULL
        );
    Context->HScrollHandle = CreateWindow(
        L"SCROLLBAR",
        NULL,
        WS_CHILD | WS_VISIBLE | SBS_HORZ,
        0,
        0,
        0,
        0,
        hwnd,
        NULL,
        CreateStruct->hInstance,
        NULL
        );
    Context->FillerBoxHandle = CreateWindow(
        L"STATIC",
        NULL,
        WS_CHILD | WS_VISIBLE,
        0,
        0,
        0,
        0,
        hwnd,
        NULL,
        CreateStruct->hInstance,
        NULL
        );

    Context->VScrollVisible = TRUE;
    Context->HScrollVisible = TRUE;

    {
        HDITEM item = { 0 };

        item.mask = HDI_TEXT | HDI_FORMAT | HDI_WIDTH;
        item.cxy = 101;
        item.pszText = L"Test Column";
        item.cchTextMax = sizeof(L"Test Column") / sizeof(WCHAR);
        item.fmt = HDF_LEFT | HDF_STRING | HDF_SORTUP;

        Header_InsertItem(Context->FixedHeaderHandle, 0, &item);

        item.pszText = L"Others Test";
        Header_InsertItem(Context->HeaderHandle, 0, &item);
    }

    Context->FixedWidth = 100;
}

VOID PhTnpOnSize(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context
    )
{
    PhTnpLayout(Context);
}

VOID PhTnpOnSetFont(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in HFONT Font,
    __in LOGICAL Redraw
    )
{
    if (Context->FontOwned)
    {
        DeleteObject(Context->Font);
        Context->FontOwned = FALSE;
    }

    Context->Font = Font;

    if (!Context->Font)
    {
        LOGFONT logFont;

        if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, 0))
        {
            Context->Font = CreateFontIndirect(&logFont);
            Context->FontOwned = TRUE;
        }
    }

    SendMessage(Context->FixedHeaderHandle, WM_SETFONT, (WPARAM)Context->Font, Redraw);
    SendMessage(Context->HeaderHandle, WM_SETFONT, (WPARAM)Context->Font, Redraw);
}

BOOLEAN PhTnpOnSetCursor(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in HWND CursorWindowHandle
    )
{
    POINT point;

    PhTnpGetMessagePos(hwnd, &point);

    if (TNP_HIT_TEST_FIXED_DIVIDER(point.x, Context))
    {
        SetCursor(LoadCursor(NULL, IDC_UPARROW)); // TODO: Use real divider cursor
        return TRUE;
    }

    if (Context->Cursor)
    {
        SetCursor(Context->Cursor);
        return TRUE;
    }

    return FALSE;
}

VOID PhTnpOnPaint(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in PAINTSTRUCT *PaintStruct,
    __in HDC hdc
    )
{
    RECT clientRect;
    POINT points[2];

    GetClientRect(hwnd, &clientRect);

    SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff));
    FillRect(hdc, &PaintStruct->rcPaint, GetStockObject(DC_BRUSH));

    points[0].x = Context->FixedWidth;
    points[0].y = 0;
    points[1].x = Context->FixedWidth;
    points[1].y = clientRect.bottom;
    SetDCPenColor(hdc, RGB(0x77, 0x77, 0x77));
    SelectObject(hdc, GetStockObject(DC_PEN));
    Polyline(hdc, points, 2);
}

VOID PhTnpOnMouseMove(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG VirtualKeys,
    __in LONG CursorX,
    __in LONG CursorY
    )
{
    if (Context->Tracking)
    {
        ULONG newFixedWidth;

        newFixedWidth = Context->TrackOldFixedWidth + (CursorX - Context->TrackStartX);
        PhTnpSetFixedWidth(Context, newFixedWidth);
    }
}

VOID PhTnpOnXxxButtonXxx(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Message,
    __in ULONG VirtualKeys,
    __in LONG CursorX,
    __in LONG CursorY
    )
{
    switch (Message)
    {
    case WM_LBUTTONDOWN:
        {
            if (TNP_HIT_TEST_FIXED_DIVIDER(CursorX, Context))
            {
                Context->Tracking = TRUE;
                Context->TrackStartX = CursorX;
                Context->TrackOldFixedWidth = Context->FixedWidth;
                SetCapture(hwnd);

                SetTimer(hwnd, 1, 100, NULL); // make sure we get messages once in a while so we can detect the escape key
                GetAsyncKeyState(VK_ESCAPE);
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if (Context->Tracking)
            {
                ReleaseCapture();
            }
        }
        break;
    case WM_RBUTTONDOWN:
        {
            if (Context->Tracking)
            {
                PhTnpCancelTrack(Context);
            }
        }
        break;
    }
}

VOID PhTnpOnCaptureChanged(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context
    )
{
    Context->Tracking = FALSE;
    KillTimer(hwnd, 1);
}

VOID PhTnpOnKeyDown(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG VirtualKey,
    __in ULONG Data
    )
{
    NOTHING;
}

BOOLEAN PhTnpOnNotify(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in NMHDR *Header,
    __out LRESULT *Result
    )
{
    switch (Header->code)
    {
    case HDN_ITEMCHANGING:
        {
            if (Header->hwndFrom == Context->FixedHeaderHandle)
            {
                NMHEADER *nmHeader = (NMHEADER *)Header;

                Context->FixedWidth = nmHeader->pitem->cxy - 1;

                if (Context->FixedWidth < Context->FixedWidthMinimum)
                    Context->FixedWidth = Context->FixedWidthMinimum;

                nmHeader->pitem->cxy = Context->FixedWidth + 1;

                PhTnpLayout(Context);
                RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
            }
            else if (Header->hwndFrom == Context->HeaderHandle)
            {
                RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
            }
        }
        break;
    }

    return FALSE;
}

VOID PhTnpCancelTrack(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    PhTnpSetFixedWidth(Context, Context->TrackOldFixedWidth);
    ReleaseCapture();
}

VOID PhTnpLayout(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    RECT clientRect;
    RECT rect;
    HDLAYOUT hdl;
    WINDOWPOS windowPos;

    GetClientRect(Context->Handle, &clientRect);

    hdl.prc = &rect;
    hdl.pwpos = &windowPos;

    // Fixed portion header control
    rect.left = 0;
    rect.top = 0;
    rect.right = Context->FixedWidth + 1;
    rect.bottom = clientRect.bottom;
    Header_Layout(Context->FixedHeaderHandle, &hdl);
    SetWindowPos(Context->FixedHeaderHandle, NULL, windowPos.x, windowPos.y, windowPos.cx, windowPos.cy, windowPos.flags);
    Context->HeaderHeight = windowPos.cy;

    // Normal portion header control
    rect.left = Context->FixedWidth;
    rect.top = 0;
    rect.right = clientRect.right - Context->VScrollWidth;
    rect.bottom = clientRect.bottom;
    Header_Layout(Context->HeaderHandle, &hdl);
    SetWindowPos(Context->HeaderHandle, NULL, windowPos.x, windowPos.y, windowPos.cx, windowPos.cy, windowPos.flags);

    // Vertical scroll bar
    if (Context->VScrollVisible)
    {
        MoveWindow(
            Context->VScrollHandle,
            clientRect.right - Context->VScrollWidth,
            0,
            Context->VScrollWidth,
            clientRect.bottom - (Context->HScrollVisible ? Context->HScrollHeight : 0),
            TRUE
            );
    }

    // Horizontal scroll bar
    if (Context->HScrollVisible)
    {
        MoveWindow(
            Context->HScrollHandle,
            Context->FixedWidth + 1,
            clientRect.bottom - Context->HScrollHeight,
            clientRect.right - Context->FixedWidth - 1 - (Context->VScrollVisible ? Context->VScrollWidth : 0),
            Context->HScrollHeight,
            TRUE
            );
    }

    // Filler box
    if (Context->VScrollVisible && Context->HScrollVisible)
    {
        MoveWindow(
            Context->FillerBoxHandle,
            clientRect.right - Context->VScrollWidth,
            clientRect.bottom - Context->HScrollHeight,
            Context->VScrollWidth,
            Context->HScrollHeight,
            TRUE
            );
    }
}

VOID PhTnpSetFixedWidth(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG FixedWidth
    )
{
    HDITEM item;

    Context->FixedWidth = FixedWidth;

    if (Context->FixedWidth < Context->FixedWidthMinimum)
        Context->FixedWidth = Context->FixedWidthMinimum;

    item.mask = HDI_WIDTH;
    item.cxy = Context->FixedWidth + 1;
    Header_SetItem(Context->FixedHeaderHandle, 0, &item);
}

VOID PhTnpGetMessagePos(
    __in HWND hwnd,
    __out PPOINT ClientPoint
    )
{
    ULONG position;
    POINT point;

    position = GetMessagePos();
    point.x = GET_X_LPARAM(position);
    point.y = GET_Y_LPARAM(position);
    ScreenToClient(hwnd, &point);

    *ClientPoint = point;
}
