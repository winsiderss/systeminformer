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
#include <vsstyle.h>
#include <treenew.h>
#include <treenewp.h>

static PVOID ComCtl32Handle;
static LONG SmallIconWidth;
static LONG SmallIconHeight;

BOOLEAN PhTreeNewInitialization()
{
    WNDCLASSEX c = { sizeof(c) };

    c.style = CS_DBLCLKS | CS_GLOBALCLASS;
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

    ComCtl32Handle = GetModuleHandle(L"comctl32.dll");
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

    if (context->Tracking && (GetAsyncKeyState(VK_ESCAPE) & 0x1))
    {
        PhTnpCancelTrack(context);
    }

    switch (uMsg)
    {
    case WM_CREATE:
        {
            if (!PhTnpOnCreate(hwnd, context, (CREATESTRUCT *)lParam))
                return -1;
        }
        return 0;
    case WM_DESTROY:
        {
            PhTnpDestroyTreeNewContext(context);
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)NULL);
        }
        return 0;
    case WM_SIZE:
        {
            PhTnpOnSize(hwnd, context);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PhTnpOnPaint(hwnd, context);
        }
        return 0;
    case WM_PRINTCLIENT:
        {
            PhTnpOnPrintClient(hwnd, context, (HDC)wParam, (ULONG)lParam);
        }
        return 0;
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
    case WM_SETTINGCHANGE:
        {
            PhTnpOnSettingChange(hwnd, context);
        }
        break;
    case WM_THEMECHANGED:
        {
            PhTnpOnThemeChanged(hwnd, context);
        }
        break;
    case WM_SETFOCUS:
        {
            context->HasFocus = TRUE;
            InvalidateRect(context->Handle, NULL, FALSE);
        }
        return 0;
    case WM_KILLFOCUS:
        {
            context->HasFocus = FALSE;
            InvalidateRect(context->Handle, NULL, FALSE);
        }
        return 0;
    case WM_SETCURSOR:
        {
            if (PhTnpOnSetCursor(hwnd, context, (HWND)wParam))
                return TRUE;
        }
        break;
    case WM_MOUSEMOVE:
        {
            PhTnpOnMouseMove(hwnd, context, (ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_MOUSELEAVE:
        {
            PhTnpOnMouseLeave(hwnd, context);
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
    case WM_CHAR:
        {
            PhTnpOnChar(hwnd, context, (ULONG)wParam, (ULONG)lParam);
        }
        return 0;
    case WM_MOUSEWHEEL:
        {
            PhTnpOnMouseWheel(hwnd, context, (SHORT)HIWORD(wParam), LOWORD(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_CONTEXTMENU:
        {
            PhTnpOnContextMenu(hwnd, context, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        return 0;
    case WM_VSCROLL:
        {
            PhTnpOnVScroll(hwnd, context, LOWORD(wParam));
        }
        return 0;
    case WM_HSCROLL:
        {
            PhTnpOnHScroll(hwnd, context, LOWORD(wParam));
        }
        return 0;
    case WM_NOTIFY:
        {
            LRESULT result;

            if (PhTnpOnNotify(hwnd, context, (NMHDR *)lParam, &result))
                return result;
        }
        break;
    }

    if (uMsg >= TNM_FIRST && uMsg <= TNM_LAST)
    {
        return PhTnpOnUserMessage(hwnd, context, uMsg, wParam, lParam);
    }

    switch (uMsg)
    {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        {
            if (context->TooltipsHandle)
            {
                MSG message;

                message.hwnd = hwnd;
                message.message = uMsg;
                message.wParam = wParam;
                message.lParam = lParam;
                SendMessage(context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }
        }
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

BOOLEAN NTAPI PhTnpNullCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    return FALSE;
}

VOID PhTnpCreateTreeNewContext(
    __out PPH_TREENEW_CONTEXT *Context
    )
{
    PPH_TREENEW_CONTEXT context;

    context = PhAllocate(sizeof(PH_TREENEW_CONTEXT));
    memset(context, 0, sizeof(PH_TREENEW_CONTEXT));

    context->FixedWidthMinimum = 20;
    context->RowHeight = 1; // must never be 0
    context->Callback = PhTnpNullCallback;
    context->FlatList = PhCreateList(64);
    context->TooltipIndex = -1;
    context->TooltipId = -1;

    *Context = context;
}

VOID PhTnpDestroyTreeNewContext(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    ULONG i;

    if (Context->Columns)
    {
        for (i = 0; i < Context->NextId; i++)
        {
            if (Context->Columns[i])
                PhFree(Context->Columns[i]);
        }

        PhFree(Context->Columns);
    }

    if (Context->ColumnsByDisplay)
        PhFree(Context->ColumnsByDisplay);

    PhDereferenceObject(Context->FlatList);

    if (Context->ThemeData)
        CloseThemeData_I(Context->ThemeData);

    if (Context->SearchString)
        PhFree(Context->SearchString);

    if (Context->TooltipText)
        PhDereferenceObject(Context->TooltipText);

    PhFree(Context);
}

BOOLEAN PhTnpOnCreate(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in CREATESTRUCT *CreateStruct
    )
{
    Context->Handle = hwnd;
    Context->InstanceHandle = CreateStruct->hInstance;
    Context->Style = CreateStruct->style;
    Context->ExtendedStyle = CreateStruct->dwExStyle;

    if (!(Context->FixedHeaderHandle = CreateWindow(
        WC_HEADER,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | HDS_HORZ | HDS_FULLDRAG | HDS_BUTTONS,
        0,
        0,
        0,
        0,
        hwnd,
        NULL,
        CreateStruct->hInstance,
        NULL
        )))
    {
        return FALSE;
    }

    if (!(Context->HeaderHandle = CreateWindow(
        WC_HEADER,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | HDS_HORZ | HDS_FULLDRAG | HDS_BUTTONS | HDS_DRAGDROP,
        0,
        0,
        0,
        0,
        hwnd,
        NULL,
        CreateStruct->hInstance,
        NULL
        )))
    {
        return FALSE;
    }

    Context->VScrollWidth = GetSystemMetrics(SM_CXVSCROLL);
    Context->HScrollHeight = GetSystemMetrics(SM_CYHSCROLL);

    if (!(Context->VScrollHandle = CreateWindow(
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
        )))
    {
        return FALSE;
    }

    if (!(Context->HScrollHandle = CreateWindow(
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
        )))
    {
        return FALSE;
    }

    if (!(Context->FillerBoxHandle = CreateWindow(
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
        )))
    {
        return FALSE;
    }

    PhTnpInitializeTooltips(Context);

    return TRUE;
}

VOID PhTnpOnSize(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context
    )
{
    GetClientRect(hwnd, &Context->ClientRect);
    PhTnpLayout(Context);

    if (Context->TooltipsHandle)
    {
        TOOLINFO toolInfo;

        memset(&toolInfo, 0, sizeof(TOOLINFO));
        toolInfo.cbSize = sizeof(TOOLINFO);
        toolInfo.hwnd = hwnd;
        toolInfo.uId = 0;
        toolInfo.rect = Context->ClientRect;
        SendMessage(Context->TooltipsHandle, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);
    }
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

    if (Context->TooltipsHandle)
    {
        SendMessage(Context->TooltipsHandle, WM_SETFONT, (WPARAM)Context->Font, FALSE);
    }

    PhTnpUpdateTextMetrics(Context);
}

VOID PhTnpOnSettingChange(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context
    )
{
    PhTnpUpdateTextMetrics(Context);
}

VOID PhTnpOnThemeChanged(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context
    )
{
    PhTnpUpdateThemeData(Context);
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
        SetCursor(LoadCursor(ComCtl32Handle, MAKEINTRESOURCE(106))); // HACK (the divider icon resource has been 106 for quite a while...)
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
    __in PPH_TREENEW_CONTEXT Context
    )
{
    RECT updateRect;
    HDC hdc;
    PAINTSTRUCT paintStruct;

    if (GetUpdateRect(hwnd, &updateRect, FALSE))
    {
        if (hdc = BeginPaint(hwnd, &paintStruct))
        {
            PhTnpPaint(hwnd, Context, hdc, &paintStruct.rcPaint);
            EndPaint(hwnd, &paintStruct);
        }
    }
}

VOID PhTnpOnPrintClient(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in HDC hdc,
    __in ULONG Flags
    )
{
    PhTnpPaint(hwnd, Context, hdc, &Context->ClientRect);
}

VOID PhTnpOnMouseMove(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG VirtualKeys,
    __in LONG CursorX,
    __in LONG CursorY
    )
{
    TRACKMOUSEEVENT trackMouseEvent;
    PH_TREENEW_HIT_TEST hitTest;
    PPH_TREENEW_NODE hotNode;

    trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
    trackMouseEvent.dwFlags = TME_LEAVE;
    trackMouseEvent.hwndTrack = hwnd;
    trackMouseEvent.dwHoverTime = 0;
    TrackMouseEvent(&trackMouseEvent);

    if (Context->Tracking)
    {
        ULONG newFixedWidth;

        newFixedWidth = Context->TrackOldFixedWidth + (CursorX - Context->TrackStartX);
        PhTnpSetFixedWidth(Context, newFixedWidth);
    }

    hitTest.Point.x = CursorX;
    hitTest.Point.y = CursorY;
    hitTest.InFlags = TN_TEST_COLUMN | TN_TEST_SUBITEM;
    PhTnpHitTest(Context, &hitTest);

    if (hitTest.Flags & TN_HIT_ITEM)
        hotNode = hitTest.Node;
    else
        hotNode = NULL;

    PhTnpSetHotNode(Context, hotNode, !!(hitTest.Flags & TN_HIT_ITEM_PLUSMINUS));

    if (Context->TooltipsHandle)
    {
        ULONG index;
        ULONG id;

        if (!(hitTest.Flags & TN_HIT_DIVIDER))
        {
            index = hitTest.Node ? hitTest.Node->Index : -1;
            id = hitTest.Column ? hitTest.Column->Id : -1;
        }
        else
        {
            index = -1;
            id = -1;
        }

        // This pops unnecessarily - when the cell has no tooltip text, and the user is 
        // moving the mouse over it. However these unnecessary calls seem to fix a 
        // certain tooltip bug (move the mouse around very quickly over the last column and 
        // the blank space to the right, and no more tooltips will appear).
        if (Context->TooltipIndex != index || Context->TooltipId != id)
        {
            PhTnpPopTooltip(Context);
        }
    }
}

VOID PhTnpOnMouseLeave(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context
    )
{
    RECT rect;

    if (Context->HotNode && Context->ThemeData)
    {
        // Update the old hot node because it may have a different non-hot background and plus minus part.
        if (PhTnpGetRowRects(Context, Context->HotNode->Index, Context->HotNode->Index, TRUE, &rect))
        {
            InvalidateRect(Context->Handle, &rect, FALSE);
        }
    }

    Context->HotNode = NULL;
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
    BOOLEAN startingTracking;
    PH_TREENEW_HIT_TEST hitTest;
    BOOLEAN plusMinusProcessed;
    LOGICAL controlKey;
    LOGICAL shiftKey;
    RECT rect;
    ULONG changedStart;
    ULONG changedEnd;
    PH_TREENEW_MESSAGE clickMessage;

    // Focus

    if (Message == WM_LBUTTONDOWN || Message == WM_RBUTTONDOWN)
        SetFocus(hwnd);

    // Divider tracking

    startingTracking = FALSE;

    switch (Message)
    {
    case WM_LBUTTONDOWN:
        {
            if (TNP_HIT_TEST_FIXED_DIVIDER(CursorX, Context))
            {
                startingTracking = TRUE;
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

    if (!startingTracking && Context->Tracking) // still OK to process further if the user is only starting to drag the divider
        return;

    hitTest.Point.x = CursorX;
    hitTest.Point.y = CursorY;
    hitTest.InFlags = TN_TEST_COLUMN | TN_TEST_SUBITEM;
    PhTnpHitTest(Context, &hitTest);

    controlKey = VirtualKeys & MK_CONTROL;
    shiftKey = VirtualKeys & MK_SHIFT;

    // Plus minus glyph

    plusMinusProcessed = FALSE;

    if ((hitTest.Flags & TN_HIT_ITEM_PLUSMINUS) && Message == WM_LBUTTONDOWN)
    {
        PhTnpSetExpandedNode(Context, hitTest.Node, !hitTest.Node->Expanded);
        plusMinusProcessed = TRUE;
    }

    // Selection

    if (!plusMinusProcessed && (Message == WM_LBUTTONDOWN || Message == WM_RBUTTONDOWN))
    {
        if (hitTest.Flags & TN_HIT_ITEM)
        {
            Context->FocusNode = hitTest.Node;

            if (shiftKey && Context->MarkNode)
            {
                ULONG start;
                ULONG end;

                // Shift key: select a range from the selection mark node to the current node.

                if (hitTest.Node->Index > Context->MarkNode->Index)
                {
                    start = Context->MarkNode->Index;
                    end = hitTest.Node->Index;
                }
                else
                {
                    start = hitTest.Node->Index;
                    end = Context->MarkNode->Index;
                }

                PhTnpSelectRange(Context, start, end, TN_SELECT_RESET, &changedStart, &changedEnd);

                if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                {
                    InvalidateRect(hwnd, &rect, FALSE);
                }
            }
            else if (controlKey)
            {
                // Control key: toggle the selection on the current node, and also make it the selection mark.

                PhTnpSelectRange(Context, hitTest.Node->Index, hitTest.Node->Index, TN_SELECT_TOGGLE, NULL, NULL);
                Context->MarkNode = hitTest.Node;

                if (PhTnpGetRowRects(Context, hitTest.Node->Index, hitTest.Node->Index, TRUE, &rect))
                {
                    InvalidateRect(hwnd, &rect, FALSE);
                }
            }
            else
            {
                // Normal: select the current node, and also make it the selection mark.

                PhTnpSelectRange(Context, hitTest.Node->Index, hitTest.Node->Index, TN_SELECT_RESET, &changedStart, &changedEnd);
                Context->MarkNode = hitTest.Node;

                if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                {
                    InvalidateRect(hwnd, &rect, FALSE);
                }
            }

            PhTnpPopTooltip(Context);
        }
        else if (!controlKey && !shiftKey)
        {
            // Nothing: deselect everything.

            PhTnpSelectRange(Context, -1, -1, TN_SELECT_RESET, &changedStart, &changedEnd);

            if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
            {
                InvalidateRect(hwnd, &rect, FALSE);
            }
        }
    }

    // Click, double-click

    clickMessage = -1;

    if (Message == WM_LBUTTONDOWN || Message == WM_RBUTTONDOWN)
    {
        if (Context->MouseDownLast != 0 && Context->MouseDownLast != Message)
        {
            // User pressed one button and pressed the other without letting go of the first one.
            // This counts as a click.

            if (Context->MouseDownLast == WM_LBUTTONDOWN)
                clickMessage = TreeNewLeftClick;
            else
                clickMessage = TreeNewRightClick;
        }

        Context->MouseDownLast = Message;
        Context->MouseDownLocation.x = CursorX;
        Context->MouseDownLocation.y = CursorY;
    }
    else if (Message == WM_LBUTTONUP || Message == WM_RBUTTONUP)
    {
        if (Context->MouseDownLast != 0 &&
            Context->MouseDownLocation.x == CursorX && Context->MouseDownLocation.y == CursorY)
        {
            if (Context->MouseDownLast == WM_LBUTTONDOWN)
                clickMessage = TreeNewLeftClick;
            else
                clickMessage = TreeNewRightClick;
        }

        Context->MouseDownLast = 0;
    }
    else if (Message == WM_LBUTTONDBLCLK)
    {
        clickMessage = TreeNewLeftDoubleClick;
    }
    else if (Message == WM_RBUTTONDBLCLK)
    {
        clickMessage = TreeNewRightDoubleClick;
    }

    if (!plusMinusProcessed && clickMessage != -1)
    {
        PH_TREENEW_MOUSE_EVENT mouseEvent;

        mouseEvent.Location.x = CursorX;
        mouseEvent.Location.y = CursorY;
        mouseEvent.Node = hitTest.Node;
        mouseEvent.Column = hitTest.Column;
        mouseEvent.KeyFlags = VirtualKeys;
        Context->Callback(Context->Handle, clickMessage, &mouseEvent, NULL, Context->CallbackContext);
    }

    // TODO: Drag selection
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
    PH_TREENEW_KEY_EVENT keyEvent;

    keyEvent.Handled = FALSE;
    keyEvent.VirtualKey = VirtualKey;
    keyEvent.Data = Data;
    Context->Callback(Context->Handle, TreeNewKeyDown, &keyEvent, NULL, Context->CallbackContext);

    if (keyEvent.Handled)
        return;

    if (PhTnpProcessFocusKey(Context, VirtualKey))
        return;
    if (PhTnpProcessNodeKey(Context, VirtualKey))
        return;
}

VOID PhTnpOnChar(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Character,
    __in ULONG Data
    )
{
    // Make sure the character is printable.
    if (Character >= ' ' && Character <= '~')
    {
        PhTnpProcessSearchKey(Context, Character);
    }
}

VOID PhTnpOnMouseWheel(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in LONG Distance,
    __in ULONG VirtualKeys,
    __in LONG CursorX,
    __in LONG CursorY
    )
{
    LONG wheelScrollLines;
    SCROLLINFO scrollInfo;
    LONG oldPosition;

    if (!SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &wheelScrollLines, 0))
        wheelScrollLines = 3;

    // The mouse wheel can affect both the vertical scrollbar and the horizontal scrollbar, 
    // but the vertical scrollbar takes precedence.

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_ALL;

    if (Context->VScrollVisible)
    {
        GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);
        oldPosition = scrollInfo.nPos;

        scrollInfo.nPos += wheelScrollLines * -Distance / WHEEL_DELTA;

        scrollInfo.fMask = SIF_POS;
        SetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo, TRUE);
        GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);

        if (scrollInfo.nPos != oldPosition)
        {
            // TODO
            Context->VScrollPosition = scrollInfo.nPos;
            InvalidateRect(hwnd, NULL, FALSE);

            if (Context->TooltipsHandle)
            {
                MSG message;
                POINT point;

                PhTnpPopTooltip(Context);
                PhTnpGetMessagePos(hwnd, &point);

                if (point.x >= 0 && point.y >= 0 && point.x < Context->ClientRect.right && point.y < Context->ClientRect.bottom)
                {
                    // Send a fake mouse move message for the new node that the mouse may be hovering over.
                    message.hwnd = hwnd;
                    message.message = WM_MOUSEMOVE;
                    message.wParam = 0;
                    message.lParam = MAKELPARAM(point.x, point.y);
                    SendMessage(Context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
                }
            }
        }
    }
    else if (Context->HScrollVisible)
    {
        GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);
        oldPosition = scrollInfo.nPos;

        scrollInfo.nPos += Context->TextMetrics.tmAveCharWidth * wheelScrollLines * -Distance / WHEEL_DELTA;

        scrollInfo.fMask = SIF_POS;
        SetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo, TRUE);
        GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);

        if (scrollInfo.nPos != oldPosition)
        {
            // TODO
            Context->HScrollPosition = scrollInfo.nPos;
            PhTnpLayout(Context);
            InvalidateRect(hwnd, NULL, FALSE);
        }
    }
}

VOID PhTnpOnContextMenu(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in LONG CursorScreenX,
    __in LONG CursorScreenY
    )
{
    POINT location;

    if (CursorScreenX == -1 && CursorScreenX == -1)
    {
        ULONG i;
        BOOLEAN found;
        RECT windowRect;
        RECT rect;

        // Context menu was invoked via keyboard. Display the context menu at 
        // the selected item.

        found = FALSE;

        for (i = 0; i < Context->FlatList->Count; i++)
        {
            if (((PPH_TREENEW_NODE)Context->FlatList->Items[i])->Selected)
            {
                found = TRUE;
                break;
            }
        }

        GetWindowRect(hwnd, &windowRect);
        CursorScreenX = windowRect.left;
        CursorScreenY = windowRect.top;

        if (found && PhTnpGetRowRects(Context, i, i, FALSE, &rect) &&
            rect.top >= Context->ClientRect.top && rect.top < Context->ClientRect.bottom)
        {
            CursorScreenX += rect.left + SmallIconWidth / 2;
            CursorScreenY += rect.top + Context->RowHeight / 2;
        }
    }
    else
    {
        POINT clientPoint;

        clientPoint.x = CursorScreenX;
        clientPoint.y = CursorScreenY;
        ScreenToClient(hwnd, &clientPoint);

        if (clientPoint.y < Context->HeaderHeight)
        {
            // Already handled by TreeNewHeaderRightClick.
            return;
        }
    }

    location.x = CursorScreenX;
    location.y = CursorScreenY;

    Context->Callback(hwnd, TreeNewContextMenu, &location, NULL, Context->CallbackContext);
}

VOID PhTnpOnVScroll(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Request
    )
{
    SCROLLINFO scrollInfo;
    LONG oldPosition;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_ALL;
    GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);
    oldPosition = scrollInfo.nPos;

    switch (Request)
    {
    case SB_LINEUP:
        scrollInfo.nPos--;
        break;
    case SB_LINEDOWN:
        scrollInfo.nPos++;
        break;
    case SB_PAGEUP:
        scrollInfo.nPos -= scrollInfo.nPage;
        break;
    case SB_PAGEDOWN:
        scrollInfo.nPos += scrollInfo.nPage;
        break;
    case SB_THUMBTRACK:
        scrollInfo.nPos = scrollInfo.nTrackPos;
        break;
    case SB_TOP:
        scrollInfo.nPos = 0;
        break;
    case SB_BOTTOM:
        scrollInfo.nPos = MAXINT;
        break;
    }

    scrollInfo.fMask = SIF_POS;
    SetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo, TRUE);
    GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);

    if (scrollInfo.nPos != oldPosition)
    {
        // TODO
        Context->VScrollPosition = scrollInfo.nPos;
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

VOID PhTnpOnHScroll(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Request
    )
{
    SCROLLINFO scrollInfo;
    LONG oldPosition;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_ALL;
    GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);
    oldPosition = scrollInfo.nPos;

    switch (Request)
    {
    case SB_LINELEFT:
        scrollInfo.nPos -= Context->TextMetrics.tmAveCharWidth;
        break;
    case SB_LINERIGHT:
        scrollInfo.nPos += Context->TextMetrics.tmAveCharWidth;
        break;
    case SB_PAGELEFT:
        scrollInfo.nPos -= scrollInfo.nPage;
        break;
    case SB_PAGERIGHT:
        scrollInfo.nPos += scrollInfo.nPage;
        break;
    case SB_THUMBTRACK:
        scrollInfo.nPos = scrollInfo.nTrackPos;
        break;
    case SB_LEFT:
        scrollInfo.nPos = 0;
        break;
    case SB_RIGHT:
        scrollInfo.nPos = MAXINT;
        break;
    }

    scrollInfo.fMask = SIF_POS;
    SetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo, TRUE);
    GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);

    if (scrollInfo.nPos != oldPosition)
    {
        // TODO
        Context->HScrollPosition = scrollInfo.nPos;
        PhTnpLayout(Context);
        InvalidateRect(hwnd, NULL, FALSE);
    }
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
    case HDN_ITEMCHANGED:
        {
            NMHEADER *nmHeader = (NMHEADER *)Header;

            if (Header->hwndFrom == Context->FixedHeaderHandle)
            {
                if (nmHeader->pitem->mask & HDI_WIDTH)
                {
                    if (Context->FixedColumnVisible)
                    {
                        Context->FixedWidth = nmHeader->pitem->cxy - 1;

                        if (Context->FixedWidth < Context->FixedWidthMinimum)
                            Context->FixedWidth = Context->FixedWidthMinimum;

                        Context->NormalLeft = Context->FixedWidth + 1;
                        nmHeader->pitem->cxy = Context->FixedWidth + 1;
                    }
                    else
                    {
                        Context->FixedWidth = 0;
                        Context->NormalLeft = 0;
                    }
                }
            }

            if (Header->hwndFrom == Context->FixedHeaderHandle || Header->hwndFrom == Context->HeaderHandle)
            {
                if (nmHeader->pitem->mask & (HDI_WIDTH | HDI_ORDER))
                {
                    // A column has been re-ordered or resized. Update our stored information.
                    PhTnpUpdateColumnHeaders(Context);
                    PhTnpUpdateColumnMaps(Context);
                    PhTnpLayout(Context);
                }

                RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
            }
        }
        break;
    case HDN_ITEMCLICK:
        {
            if (Header->hwndFrom == Context->FixedHeaderHandle || Header->hwndFrom == Context->HeaderHandle)
            {
                NMHEADER *nmHeader = (NMHEADER *)Header;
                HDITEM item;
                PPH_TREENEW_COLUMN column;

                // A column has been clicked, so update the sort state.

                item.mask = HDI_LPARAM;

                if (Header_GetItem(Header->hwndFrom, nmHeader->iItem, &item))
                {
                    column = (PPH_TREENEW_COLUMN)item.lParam;

                    if (column->Id == Context->SortColumn)
                    {
                        if (Context->TriState)
                        {
                            if (Context->SortOrder == AscendingSortOrder)
                                Context->SortOrder = DescendingSortOrder;
                            else if (Context->SortOrder == DescendingSortOrder)
                                Context->SortOrder = NoSortOrder;
                            else
                                Context->SortOrder = AscendingSortOrder;
                        }
                        else
                        {
                            if (Context->SortOrder == AscendingSortOrder)
                                Context->SortOrder = DescendingSortOrder;
                            else
                                Context->SortOrder = AscendingSortOrder;
                        }
                    }
                    else
                    {
                        Context->SortColumn = column->Id;
                        Context->SortOrder = AscendingSortOrder;
                    }

                    PhTnpSetColumnHeaderSortIcon(Context, column);

                    Context->Callback(Context->Handle, TreeNewSortChanged, NULL, NULL, Context->CallbackContext);
                }
            }
        }
        break;
    case HDN_ENDDRAG:
    case NM_RELEASEDCAPTURE:
        {
            if (Header->hwndFrom == Context->HeaderHandle)
            {
                // Columns have been re-ordered, so refresh our information.
                // Note: The fixed column cannot be re-ordered.
                PhTnpUpdateColumnHeaders(Context);
                PhTnpUpdateColumnMaps(Context);
            }
        }
        break;
    case NM_RCLICK:
        {
            Context->Callback(Context->Handle, TreeNewHeaderRightClick, NULL, NULL, Context->CallbackContext);
        }
        break;
    case TTN_GETDISPINFO:
        {
            if (Header->hwndFrom == Context->TooltipsHandle)
            {
                NMTTDISPINFO *info = (NMTTDISPINFO *)Header;
                POINT point;

                PhTnpGetMessagePos(hwnd, &point);
                PhTnpGetTooltipText(Context, &point, &info->lpszText);
            }
        }
        break;
    case TTN_SHOW:
        {
            if (Header->hwndFrom == Context->TooltipsHandle)
            {
                *Result = PhTnpPrepareTooltipShow(Context);
                return TRUE;
            }
        }
        break;
    case TTN_POP:
        {
            if (Header->hwndFrom == Context->TooltipsHandle)
            {
                PhTnpPrepareTooltipPop(Context);
            }
        }
        break;
    }

    return FALSE;
}

ULONG_PTR PhTnpOnUserMessage(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Message,
    __in ULONG_PTR WParam,
    __in ULONG_PTR LParam
    )
{
    switch (Message)
    {
    case TNM_SETCALLBACK:
        {
            Context->Callback = (PPH_TREENEW_CALLBACK)LParam;
            Context->CallbackContext = (PVOID)WParam;

            if (!Context->Callback)
                Context->Callback = PhTnpNullCallback;
        }
        return TRUE;
    case TNM_NODESSTRUCTURED:
        {
            PhTnpRestructureNodes(Context);
        }
        return TRUE;
    case TNM_ADDCOLUMN:
        return PhTnpAddColumn(Context, (PPH_TREENEW_COLUMN)LParam);
    case TNM_REMOVECOLUMN:
        return PhTnpRemoveColumn(Context, (ULONG)WParam);
    case TNM_GETCOLUMN:
        return PhTnpCopyColumn(Context, (ULONG)WParam, (PPH_TREENEW_COLUMN)LParam);
    case TNM_SETCOLUMN:
        {
            PPH_TREENEW_COLUMN column = (PPH_TREENEW_COLUMN)LParam;

            return PhTnpChangeColumn(Context, (ULONG)WParam, column->Id, column);
        }
        break;
    case TNM_GETCOLUMNORDERARRAY:
        {
            // TODO
            return Header_GetOrderArray(Context->HeaderHandle, (ULONG)WParam, (PLONG)LParam);
        }
        break;
    case TNM_SETCOLUMNORDERARRAY:
        {
            // TODO
            if (!Header_SetOrderArray(Context->HeaderHandle, (ULONG)WParam, (PLONG)LParam))
                return FALSE;

            PhTnpUpdateColumnHeaders(Context);
            PhTnpUpdateColumnMaps(Context);
        }
        break;
    case TNM_SETCURSOR:
        {
            Context->Cursor = (HCURSOR)LParam;
        }
        return TRUE;
    case TNM_GETSORT:
        {
            PULONG sortColumn = (PULONG)WParam;
            PPH_SORT_ORDER sortOrder = (PPH_SORT_ORDER)LParam;

            if (sortColumn)
                *sortColumn = Context->SortColumn;
            if (sortOrder)
                *sortOrder = Context->SortOrder;
        }
        return TRUE;
    case TNM_SETSORT:
        {
            ULONG sortColumn = (ULONG)WParam;
            PH_SORT_ORDER sortOrder = (PH_SORT_ORDER)LParam;
            PPH_TREENEW_COLUMN column;

            if (sortOrder != NoSortOrder)
            {
                if (!(column = PhTnpLookupColumnById(Context, sortColumn)))
                    return FALSE;
            }
            else
            {
                column = NULL;
            }

            Context->SortColumn = sortColumn;
            Context->SortOrder = sortOrder;

            PhTnpSetColumnHeaderSortIcon(Context, column);

            Context->Callback(Context->Handle, TreeNewSortChanged, NULL, NULL, Context->CallbackContext);
        }
        return TRUE;
    case TNM_SETTRISTATE:
        Context->TriState = !!WParam;
        return TRUE;
    case TNM_ENSUREVISIBLE:
        return PhTnpEnsureVisibleNode(Context, ((PPH_TREENEW_NODE)LParam)->Index);
    case TNM_SCROLL:
        PhTnpScroll(Context, (LONG)WParam, (LONG)LParam);
        return TRUE;
    case TNM_GETFLATNODECOUNT:
        return (LRESULT)Context->FlatList->Count;
    case TNM_GETFLATNODE:
        {
            ULONG index = (ULONG)WParam;

            if (index >= Context->FlatList->Count)
                return (LRESULT)NULL;

            return (LRESULT)Context->FlatList->Items[index];
        }
        break;
    case TNM_GETCELLTEXT:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)LParam;

            return PhTnpGetCellText(
                Context,
                getCellText->Node,
                getCellText->Id,
                &getCellText->Text
                );
        }
        break;
    case TNM_SETNODEEXPANDED:
        PhTnpSetExpandedNode(Context, (PPH_TREENEW_NODE)LParam, !!WParam);
        return TRUE;
    case TNM_GETMAXID:
        return (LRESULT)(Context->NextId - 1);
    case TNM_SETMAXID:
        {
            ULONG maxId = (ULONG)WParam;

            if (Context->NextId < maxId + 1)
            {
                Context->NextId = maxId + 1;

                if (Context->AllocatedColumns < Context->NextId)
                {
                    PhTnpExpandAllocatedColumns(Context);
                }
            }
        }
        return TRUE;
    case TNM_INVALIDATENODE:
        {
            PPH_TREENEW_NODE node = (PPH_TREENEW_NODE)LParam;
            RECT rect;

            if (!PhTnpGetRowRects(Context, node->Index, node->Index, TRUE, &rect))
                return FALSE;

            InvalidateRect(hwnd, &rect, FALSE);
        }
        return TRUE;
    case TNM_INVALIDATENODES:
        {
            RECT rect;

            if (!PhTnpGetRowRects(Context, (ULONG)WParam, (ULONG)LParam, TRUE, &rect))
                return FALSE;

            InvalidateRect(hwnd, &rect, FALSE);
        }
        return TRUE;
    case TNM_GETFIXEDHEADER:
        return (LRESULT)Context->FixedHeaderHandle;
    case TNM_GETHEADER:
        return (LRESULT)Context->HeaderHandle;
    case TNM_GETTOOLTIPS:
        return (LRESULT)Context->TooltipsHandle;
    case TNM_SETREDRAW:
        return FALSE; // TODO
    case TNM_SELECTRANGE:
    case TNM_DESELECTRANGE:
        {
            ULONG flags;
            ULONG changedStart;
            ULONG changedEnd;
            RECT rect;

            flags = 0;

            if (Message == TNM_DESELECTRANGE)
                flags |= TN_SELECT_DESELECT;

            PhTnpSelectRange(Context, (ULONG)WParam, (ULONG)LParam, flags, &changedStart, &changedEnd);

            if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
            {
                InvalidateRect(hwnd, &rect, FALSE);
            }
        }
        return TRUE;
    case TNM_GETCOLUMNCOUNT:
        return (LRESULT)Context->NumberOfColumns;
    }

    return 0;
}

VOID PhTnpUpdateTextMetrics(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    HDC hdc;

    if (hdc = GetDC(HWND_DESKTOP))
    {
        GetTextMetrics(hdc, &Context->TextMetrics);

        Context->RowHeight = Context->TextMetrics.tmHeight;

        if (Context->Style & TN_STYLE_ICONS)
        {
            if (Context->RowHeight < SmallIconHeight)
                Context->RowHeight = SmallIconHeight;

            Context->RowHeight += 3; // TODO: Fix value
        }

        ReleaseDC(HWND_DESKTOP, hdc);
    }
}

VOID PhTnpUpdateThemeData(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    if (
        IsThemeActive_I &&
        OpenThemeData_I &&
        CloseThemeData_I &&
        IsThemePartDefined_I &&
        DrawThemeBackground_I
        )
    {
        Context->ThemeActive = !!IsThemeActive_I();

        if (Context->ThemeData)
        {
            CloseThemeData_I(Context->ThemeData);
            Context->ThemeData = NULL;
        }

        Context->ThemeData = OpenThemeData_I(Context->Handle, L"TREEVIEW");
        Context->ThemeHasItemBackground = !!IsThemePartDefined_I(Context->ThemeData, TVP_TREEITEM, 0);
        Context->ThemeHasGlyph = !!IsThemePartDefined_I(Context->ThemeData, TVP_GLYPH, 0);
        Context->ThemeHasHotGlyph = !!IsThemePartDefined_I(Context->ThemeData, TVP_HOTGLYPH, 0);
    }
    else
    {
        Context->ThemeData = NULL;
        Context->ThemeActive = FALSE;
        Context->ThemeHasItemBackground = FALSE;
        Context->ThemeHasGlyph = FALSE;
        Context->ThemeHasHotGlyph = FALSE;
    }
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

    clientRect = Context->ClientRect;

    PhTnpUpdateScrollBars(Context);

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
            Context->NormalLeft,
            clientRect.bottom - Context->HScrollHeight,
            clientRect.right - Context->NormalLeft - (Context->VScrollVisible ? Context->VScrollWidth : 0),
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

    hdl.prc = &rect;
    hdl.pwpos = &windowPos;

    // Fixed portion header control
    rect.left = 0;
    rect.top = 0;
    rect.right = Context->NormalLeft;
    rect.bottom = clientRect.bottom;
    Header_Layout(Context->FixedHeaderHandle, &hdl);
    SetWindowPos(Context->FixedHeaderHandle, NULL, windowPos.x, windowPos.y, windowPos.cx, windowPos.cy, windowPos.flags);
    Context->HeaderHeight = windowPos.cy;

    // Normal portion header control
    rect.left = Context->NormalLeft - Context->HScrollPosition;
    rect.top = 0;
    rect.right = clientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);
    rect.bottom = clientRect.bottom;
    Header_Layout(Context->HeaderHandle, &hdl);
    SetWindowPos(Context->HeaderHandle, NULL, windowPos.x, windowPos.y, windowPos.cx, windowPos.cy, windowPos.flags);
    UpdateWindow(Context->HeaderHandle);
}

VOID PhTnpSetFixedWidth(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG FixedWidth
    )
{
    HDITEM item;

    if (Context->FixedColumnVisible)
    {
        Context->FixedWidth = FixedWidth;

        if (Context->FixedWidth < Context->FixedWidthMinimum)
            Context->FixedWidth = Context->FixedWidthMinimum;

        Context->NormalLeft = Context->FixedWidth + 1;

        item.mask = HDI_WIDTH;
        item.cxy = Context->FixedWidth + 1;
        Header_SetItem(Context->FixedHeaderHandle, 0, &item);
    }
    else
    {
        Context->FixedWidth = 0;
        Context->NormalLeft = 0;
    }
}

PPH_TREENEW_COLUMN PhTnpLookupColumnById(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Id
    )
{
    if (Id >= Context->AllocatedColumns)
        return NULL;

    return Context->Columns[Id];
}

BOOLEAN PhTnpAddColumn(
    __in PPH_TREENEW_CONTEXT Context,
    __in PPH_TREENEW_COLUMN Column
    )
{
    PPH_TREENEW_COLUMN realColumn;

    // Check if a column with the same ID already exists.
    if (Column->Id < Context->AllocatedColumns && Context->Columns[Column->Id])
        return FALSE;

    if (Context->NextId < Column->Id + 1)
        Context->NextId = Column->Id + 1;

    realColumn = PhAllocateCopy(Column, sizeof(PH_TREENEW_COLUMN));

    if (Context->AllocatedColumns < Context->NextId)
    {
        PhTnpExpandAllocatedColumns(Context);
    }

    Context->Columns[Column->Id] = realColumn;
    Context->NumberOfColumns++;

    if (realColumn->Fixed)
    {
        if (Context->FixedColumn)
        {
            // We already have a fixed column, and we can't have two. Make this new column un-fixed.
            realColumn->Fixed = FALSE;
        }
        else
        {
            Context->FixedColumn = realColumn;
        }
    }

    if (realColumn->Visible)
    {
        realColumn->s.ViewIndex = PhTnpInsertColumnHeader(Context, realColumn);
    }
    else
    {
        realColumn->s.ViewIndex = -1;
    }

    PhTnpUpdateColumnMaps(Context);

    if (realColumn->Visible)
        PhTnpLayout(Context);

    return TRUE;
}

BOOLEAN PhTnpRemoveColumn(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Id
    )
{
    PPH_TREENEW_COLUMN realColumn;
    BOOLEAN updateLayout;

    if (!(realColumn = PhTnpLookupColumnById(Context, Id)))
        return FALSE;

    updateLayout = FALSE;

    if (realColumn->Visible)
        updateLayout = TRUE;

    PhTnpDeleteColumnHeader(Context, realColumn);
    Context->Columns[realColumn->Id] = NULL;
    PhFree(realColumn);
    PhTnpUpdateColumnMaps(Context);

    if (updateLayout)
        PhTnpLayout(Context);

    Context->NumberOfColumns--;

    return TRUE;
}

BOOLEAN PhTnpCopyColumn(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Id,
    __out PPH_TREENEW_COLUMN Column
    )
{
    PPH_TREENEW_COLUMN realColumn;

    if (!(realColumn = PhTnpLookupColumnById(Context, Id)))
        return FALSE;

    memcpy(Column, realColumn, sizeof(PH_TREENEW_COLUMN));

    return TRUE;
}

BOOLEAN PhTnpChangeColumn(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Mask,
    __in ULONG Id,
    __in PPH_TREENEW_COLUMN Column
    )
{
    PPH_TREENEW_COLUMN realColumn;
    BOOLEAN addedOrRemoved;

    if (!(realColumn = PhTnpLookupColumnById(Context, Id)))
        return FALSE;

    addedOrRemoved = FALSE;

    if (Mask & TN_COLUMN_FLAG_VISIBLE)
    {
        if (realColumn->Visible != Column->Visible)
        {
            if (Column->Visible)
            {
                if (realColumn->Fixed)
                    realColumn->DisplayIndex = 0;
                else
                    realColumn->DisplayIndex = Header_GetItemCount(Context->HeaderHandle);

                realColumn->s.ViewIndex = PhTnpInsertColumnHeader(Context, realColumn);
            }
            else
            {
                PhTnpDeleteColumnHeader(Context, realColumn);
            }

            addedOrRemoved = TRUE;
            PhTnpUpdateColumnMaps(Context);
            PhTnpLayout(Context);
        }
    }

    if (Mask & TN_COLUMN_FLAG_CUSTOMDRAW)
    {
        realColumn->CustomDraw = Column->CustomDraw;
    }

    if (Mask & (TN_COLUMN_TEXT | TN_COLUMN_WIDTH | TN_COLUMN_ALIGNMENT | TN_COLUMN_DISPLAYINDEX))
    {
        BOOLEAN updateHeaders;
        BOOLEAN updateMaps;
        BOOLEAN updateLayout;

        updateHeaders = FALSE;
        updateMaps = FALSE;
        updateLayout = FALSE;

        if (Mask & TN_COLUMN_TEXT)
        {
            realColumn->Text = Column->Text;
        }

        if (Mask & TN_COLUMN_WIDTH)
        {
            realColumn->Width = Column->Width;
            updateMaps = TRUE;
        }

        if (Mask & TN_COLUMN_ALIGNMENT)
        {
            realColumn->Alignment = Column->Alignment;
        }

        if (Mask & TN_COLUMN_DISPLAYINDEX)
        {
            realColumn->DisplayIndex = Column->DisplayIndex;
            updateHeaders = TRUE;
            updateMaps = TRUE;
            updateLayout = TRUE;
        }

        if (!addedOrRemoved)
        {
            PhTnpChangeColumnHeader(Context, Mask, realColumn);

            if (updateHeaders)
                PhTnpUpdateColumnHeaders(Context);
            if (updateMaps)
                PhTnpUpdateColumnMaps(Context);
            if (updateLayout)
                PhTnpLayout(Context);
        }
    }

    if (Mask & TN_COLUMN_CONTEXT)
    {
        realColumn->Context = Column->Context;
    }

    if (Mask & TN_COLUMN_TEXTFLAGS)
    {
        realColumn->TextFlags = Column->TextFlags;
    }

    return TRUE;
}

VOID PhTnpExpandAllocatedColumns(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    if (Context->Columns)
    {
        ULONG oldAllocatedColumns;

        oldAllocatedColumns = Context->AllocatedColumns;
        Context->AllocatedColumns *= 2;

        if (Context->AllocatedColumns < Context->NextId)
            Context->AllocatedColumns = Context->NextId;

        Context->Columns = PhReAllocate(
            Context->Columns,
            Context->AllocatedColumns * sizeof(PPH_TREENEW_COLUMN)
            );

        // Zero the newly allocated portion.
        memset(
            &Context->Columns[oldAllocatedColumns],
            0,
            (Context->AllocatedColumns - oldAllocatedColumns) * sizeof(PPH_TREENEW_COLUMN)
            );
    }
    else
    {
        Context->AllocatedColumns = 16;

        if (Context->AllocatedColumns < Context->NextId)
            Context->AllocatedColumns = Context->NextId;

        Context->Columns = PhAllocate(
            Context->AllocatedColumns * sizeof(PPH_TREENEW_COLUMN)
            );
        memset(Context->Columns, 0, Context->AllocatedColumns * sizeof(PPH_TREENEW_COLUMN));
    }
}

VOID PhTnpUpdateColumnMaps(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    ULONG i;
    LONG x;

    if (Context->AllocatedColumnsByDisplay < Context->NumberOfColumns)
    {
        if (Context->ColumnsByDisplay)
            PhFree(Context->ColumnsByDisplay);

        Context->ColumnsByDisplay = PhAllocate(sizeof(PPH_TREENEW_COLUMN) * Context->NumberOfColumns);
        Context->AllocatedColumnsByDisplay = Context->NumberOfColumns;
    }

    memset(Context->ColumnsByDisplay, 0, sizeof(PPH_TREENEW_COLUMN) * Context->AllocatedColumnsByDisplay);

    for (i = 0; i < Context->NextId; i++)
    {
        if (!Context->Columns[i])
            continue;

        if (Context->Columns[i]->Visible && !Context->Columns[i]->Fixed && Context->Columns[i]->DisplayIndex != -1)
        {
            if (Context->Columns[i]->DisplayIndex >= Context->NumberOfColumns)
                PhRaiseStatus(STATUS_INTERNAL_ERROR);

            Context->ColumnsByDisplay[Context->Columns[i]->DisplayIndex] = Context->Columns[i];
        }
    }

    x = 0;

    for (i = 0; i < Context->AllocatedColumnsByDisplay; i++)
    {
        if (!Context->ColumnsByDisplay[i])
            break;

        Context->ColumnsByDisplay[i]->s.ViewX = x;
        x += Context->ColumnsByDisplay[i]->Width;
    }

    Context->NumberOfColumnsByDisplay = i;
    Context->TotalViewX = x;

    if (Context->FixedColumnVisible)
    {
        Context->FirstColumn = Context->FixedColumn;
    }
    else if (Context->NumberOfColumnsByDisplay != 0)
    {
        Context->FirstColumn = Context->ColumnsByDisplay[0];
    }
    else
    {
        Context->FirstColumn = NULL;
    }
}

LONG PhTnpInsertColumnHeader(
    __in PPH_TREENEW_CONTEXT Context,
    __in PPH_TREENEW_COLUMN Column
    )
{
    HDITEM item;

    if (Column->Fixed)
    {
        if (Column->Width < Context->FixedWidthMinimum)
            Column->Width = Context->FixedWidthMinimum;

        Context->FixedWidth = Column->Width;
        Context->NormalLeft = Context->FixedWidth + 1;
        Context->FixedColumnVisible = TRUE;
    }

    memset(&item, 0, sizeof(HDITEM));
    item.mask = HDI_WIDTH | HDI_TEXT | HDI_FORMAT | HDI_LPARAM | HDI_ORDER;
    item.cxy = Column->Width;
    item.pszText = Column->Text;
    item.fmt = 0;
    item.lParam = (LPARAM)Column;

    if (Column->Fixed)
        item.cxy++;

    if (Column->Fixed)
        item.iOrder = 0;
    else
        item.iOrder = Column->DisplayIndex;

    if (Column->Alignment & PH_ALIGN_LEFT)
        item.fmt |= HDF_LEFT;
    else if (Column->Alignment & PH_ALIGN_RIGHT)
        item.fmt |= HDF_RIGHT;
    else
        item.fmt |= HDF_CENTER;

    if (Column->Id == Context->SortColumn)
    {
        if (Context->SortOrder == AscendingSortOrder)
            item.fmt |= HDF_SORTUP;
        else if (Context->SortOrder == DescendingSortOrder)
            item.fmt |= HDF_SORTDOWN;
    }

    Column->Visible = TRUE;

    if (Column->Fixed)
        return Header_InsertItem(Context->FixedHeaderHandle, 0, &item);
    else
        return Header_InsertItem(Context->HeaderHandle, MAXINT, &item);
}

VOID PhTnpChangeColumnHeader(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Mask,
    __in PPH_TREENEW_COLUMN Column
    )
{
    HDITEM item;

    memset(&item, 0, sizeof(HDITEM));
    item.mask = 0;

    if (Mask & TN_COLUMN_TEXT)
    {
        item.mask |= HDI_TEXT;
        item.pszText = Column->Text;
    }

    if (Mask & TN_COLUMN_WIDTH)
    {
        item.mask |= HDI_WIDTH;
        item.cxy = Column->Width;

        if (Column->Fixed)
            item.cxy++;
    }

    if (Mask & TN_COLUMN_ALIGNMENT)
    {
        item.mask |= HDI_FORMAT;
        item.fmt = 0;

        if (Column->Alignment & PH_ALIGN_LEFT)
            item.fmt |= HDF_LEFT;
        else if (Column->Alignment & PH_ALIGN_RIGHT)
            item.fmt |= HDF_RIGHT;
        else
            item.fmt |= HDF_CENTER;

        if (Column->Id == Context->SortColumn)
        {
            if (Context->SortOrder == AscendingSortOrder)
                item.fmt |= HDF_SORTUP;
            else if (Context->SortOrder == DescendingSortOrder)
                item.fmt |= HDF_SORTDOWN;
        }
    }

    if (Mask & TN_COLUMN_DISPLAYINDEX)
    {
        item.mask |= HDI_ORDER;

        if (Column->Fixed)
            item.iOrder = 0;
        else
            item.iOrder = Column->DisplayIndex;
    }

    if (Column->Fixed)
        Header_SetItem(Context->FixedHeaderHandle, 0, &item);
    else
        Header_SetItem(Context->FixedHeaderHandle, Column->s.ViewIndex, &item);
}

VOID PhTnpDeleteColumnHeader(
    __in PPH_TREENEW_CONTEXT Context,
    __inout PPH_TREENEW_COLUMN Column
    )
{
    if (Column->Fixed)
    {
        Context->FixedColumn = NULL;
        Context->FixedWidth = 0;
        Context->NormalLeft = 0;
        Context->FixedColumnVisible = FALSE;
    }

    if (Column->Fixed)
        Header_DeleteItem(Context->FixedHeaderHandle, Column->s.ViewIndex);
    else
        Header_DeleteItem(Context->HeaderHandle, Column->s.ViewIndex);

    Column->Visible = FALSE;
    Column->s.ViewIndex = -1;
    PhTnpUpdateColumnHeaders(Context);
}

VOID PhTnpUpdateColumnHeaders(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    ULONG count;
    ULONG i;
    HDITEM item;
    PPH_TREENEW_COLUMN column;

    item.mask = HDI_WIDTH | HDI_LPARAM | HDI_ORDER;

    // Fixed column

    if (Context->FixedColumnVisible && Header_GetItem(Context->FixedHeaderHandle, 0, &item))
    {
        column = Context->FixedColumn;
        column->Width = item.cxy;
    }

    // Normal columns

    count = Header_GetItemCount(Context->HeaderHandle);

    if (count != -1)
    {
        for (i = 0; i < count; i++)
        {
            if (Header_GetItem(Context->HeaderHandle, i, &item))
            {
                column = (PPH_TREENEW_COLUMN)item.lParam;
                column->s.ViewIndex = i;
                column->Width = item.cxy;
                column->DisplayIndex = item.iOrder;
            }
        }
    }
}

BOOLEAN PhTnpSetColumnHeaderSortIcon(
    __in PPH_TREENEW_CONTEXT Context,
    __in_opt PPH_TREENEW_COLUMN SortColumnPointer
    )
{
    if (Context->SortOrder == NoSortOrder)
    {
        PhSetHeaderSortIcon(
            Context->FixedHeaderHandle,
            -1,
            NoSortOrder
            );
        PhSetHeaderSortIcon(
            Context->HeaderHandle,
            -1,
            NoSortOrder
            );

        return TRUE;
    }

    if (!SortColumnPointer)
    {
        if (!(SortColumnPointer = PhTnpLookupColumnById(Context, Context->SortColumn)))
            return FALSE;
    }

    if (SortColumnPointer->Fixed)
    {
        PhSetHeaderSortIcon(
            Context->FixedHeaderHandle,
            0,
            Context->SortOrder
            );
        PhSetHeaderSortIcon(
            Context->HeaderHandle,
            -1,
            NoSortOrder
            );
    }
    else
    {
        PhSetHeaderSortIcon(
            Context->FixedHeaderHandle,
            -1,
            NoSortOrder
            );
        PhSetHeaderSortIcon(
            Context->HeaderHandle,
            SortColumnPointer->s.ViewIndex,
            Context->SortOrder
            );
    }

    return TRUE;
}

BOOLEAN PhTnpGetNodeChildren(
    __in PPH_TREENEW_CONTEXT Context,
    __in_opt PPH_TREENEW_NODE Node,
    __out PPH_TREENEW_NODE **Children,
    __out PULONG NumberOfChildren
    )
{
    PH_TREENEW_GET_CHILDREN getChildren;

    getChildren.Flags = 0;
    getChildren.Node = Node;
    getChildren.Children = NULL;
    getChildren.NumberOfChildren = 0;

    if (Context->Callback(
        Context->Handle,
        TreeNewGetChildren,
        &getChildren,
        NULL,
        Context->CallbackContext
        ))
    {
        *Children = getChildren.Children;
        *NumberOfChildren = getChildren.NumberOfChildren;

        return TRUE;
    }

    return FALSE;
}

BOOLEAN PhTnpIsNodeLeaf(
    __in PPH_TREENEW_CONTEXT Context,
    __in PPH_TREENEW_NODE Node
    )
{
    PH_TREENEW_IS_LEAF isLeaf;

    isLeaf.Flags = 0;
    isLeaf.Node = Node;
    isLeaf.IsLeaf = TRUE;

    if (Context->Callback(
        Context->Handle,
        TreeNewIsLeaf,
        &isLeaf,
        NULL,
        Context->CallbackContext
        ))
    {
        return isLeaf.IsLeaf;
    }

    // Doesn't matter, decide when we do the get-children callback.
    return FALSE;
}

BOOLEAN PhTnpGetCellText(
    __in PPH_TREENEW_CONTEXT Context,
    __in PPH_TREENEW_NODE Node,
    __in ULONG Id,
    __out PPH_STRINGREF Text
    )
{
    PH_TREENEW_GET_CELL_TEXT getCellText;

    if (Id < Node->TextCacheSize && Node->TextCache[Id].Buffer)
    {
        *Text = Node->TextCache[Id];
        return TRUE;
    }

    getCellText.Flags = 0;
    getCellText.Node = Node;
    getCellText.Id = Id;
    PhInitializeEmptyStringRef(&getCellText.Text);

    if (Context->Callback(
        Context->Handle,
        TreeNewGetCellText,
        &getCellText,
        NULL,
        Context->CallbackContext
        ) && getCellText.Text.Buffer)
    {
        *Text = getCellText.Text;

        if ((getCellText.Flags & TN_CACHE) && Id < Node->TextCacheSize)
            Node->TextCache[Id] = getCellText.Text;

        return TRUE;
    }

    return FALSE;
}

VOID PhTnpRestructureNodes(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    PPH_TREENEW_NODE *children;
    ULONG numberOfChildren;
    ULONG i;
    ULONG hotIndex;
    ULONG markIndex;

    if (!PhTnpGetNodeChildren(Context, NULL, &children, &numberOfChildren))
        return;

    // We try to preserve the hot node, the focused node and the selection mark node.

    if (Context->HotNode)
        hotIndex = Context->HotNode->Index;
    else
        hotIndex = -1;

    Context->FocusNodeFound = FALSE;

    if (Context->MarkNode)
        markIndex = Context->MarkNode->Index;
    else
        markIndex = -1;

    PhClearList(Context->FlatList);
    Context->CanAnyExpand = FALSE;

    for (i = 0; i < numberOfChildren; i++)
    {
        PhTnpInsertNodeChildren(Context, children[i], 0);
    }

    if (hotIndex < Context->FlatList->Count)
        Context->HotNode = Context->FlatList->Items[hotIndex];
    else
        Context->HotNode = NULL;

    if (!Context->FocusNodeFound)
        Context->FocusNode = NULL; // focused node is no longer present

    if (markIndex < Context->FlatList->Count)
        Context->MarkNode = Context->FlatList->Items[markIndex];
    else
        Context->MarkNode = NULL;

    PhTnpLayout(Context);
    InvalidateRect(Context->Handle, NULL, FALSE);
}

VOID PhTnpInsertNodeChildren(
    __in PPH_TREENEW_CONTEXT Context,
    __in PPH_TREENEW_NODE Node,
    __in ULONG Level
    )
{
    PPH_TREENEW_NODE *children;
    ULONG numberOfChildren;
    ULONG i;
    ULONG nextLevel;

    if (Node->Visible)
    {
        Node->Level = Level;

        Node->Index = Context->FlatList->Count;
        PhAddItemList(Context->FlatList, Node);

        if (Context->FocusNode == Node)
            Context->FocusNodeFound = TRUE;

        nextLevel = Level + 1;
    }
    else
    {
        nextLevel = 0; // children of this node should be level 0
    }

    if (!(Node->s.IsLeaf = PhTnpIsNodeLeaf(Context, Node)))
    {
        Context->CanAnyExpand = TRUE;

        if (Node->Expanded)
        {
            if (PhTnpGetNodeChildren(Context, Node, &children, &numberOfChildren))
            {
                for (i = 0; i < numberOfChildren; i++)
                {
                    PhTnpInsertNodeChildren(Context, children[i], nextLevel);
                }

                if (numberOfChildren == 0)
                    Node->s.IsLeaf = TRUE;
            }
        }
    }
}

VOID PhTnpSetExpandedNode(
    __in PPH_TREENEW_CONTEXT Context,
    __in PPH_TREENEW_NODE Node,
    __in BOOLEAN Expanded
    )
{
    if (Node->Expanded != Expanded)
    {
        PH_TREENEW_NODE_EVENT nodeEvent;

        memset(&nodeEvent, 0, sizeof(PH_TREENEW_NODE_EVENT));
        Context->Callback(Context->Handle, TreeNewNodeExpanding, Node, &nodeEvent, Context->CallbackContext);

        if (!nodeEvent.Handled)
        {
            Node->Expanded = Expanded;
            PhTnpRestructureNodes(Context);
        }
    }
}

BOOLEAN PhTnpGetCellParts(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Index,
    __in_opt PPH_TREENEW_COLUMN Column,
    __in ULONG Flags,
    __out PPH_TREENEW_CELL_PARTS Parts
    )
{
    PPH_TREENEW_NODE node;
    LONG viewWidth;
    LONG nodeY;
    LONG iconVerticalMargin;
    BOOLEAN isFirstColumn;
    LONG currentX;

    // This function must be kept in sync with PhTnpDrawCell.

    if (Index >= Context->FlatList->Count)
        return FALSE;

    node = Context->FlatList->Items[Index];
    nodeY = Context->HeaderHeight + ((LONG)Index - Context->VScrollPosition) * Context->RowHeight;

    Parts->Flags = 0;
    Parts->RowRect.left = 0;
    Parts->RowRect.right = Context->NormalLeft + Context->TotalViewX - Context->HScrollPosition;
    Parts->RowRect.top = nodeY;
    Parts->RowRect.bottom = nodeY + Context->RowHeight;

    viewWidth = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);

    if (Parts->RowRect.right > viewWidth)
        Parts->RowRect.right = viewWidth;

    if (!Column)
        return TRUE;
    if (!Column->Visible)
        return FALSE;

    iconVerticalMargin = (Context->RowHeight - SmallIconHeight) / 2;

    isFirstColumn = Column == Context->FirstColumn;
    currentX = Column->s.ViewX;

    if (!Column->Fixed)
    {
        currentX += Context->NormalLeft - Context->HScrollPosition;
    }

    Parts->Flags |= TN_PART_CELL;
    Parts->CellRect.left = currentX;
    Parts->CellRect.right = currentX + Column->Width;
    Parts->CellRect.top = Parts->RowRect.top;
    Parts->CellRect.bottom = Parts->RowRect.bottom;

    currentX += 6;

    if (isFirstColumn)
    {
        currentX += (LONG)node->Level * SmallIconWidth;

        if (!node->s.IsLeaf)
        {
            Parts->Flags |= TN_PART_PLUSMINUS;
            Parts->PlusMinusRect.left = currentX;
            Parts->PlusMinusRect.right = currentX + SmallIconWidth;
            Parts->PlusMinusRect.top = Parts->RowRect.top + iconVerticalMargin;
            Parts->PlusMinusRect.bottom = Parts->RowRect.bottom - iconVerticalMargin;

            currentX += SmallIconWidth;
        }

        if (node->Icon)
        {
            Parts->Flags |= TN_PART_ICON;
            Parts->IconRect.left = currentX;
            Parts->IconRect.right = currentX + SmallIconWidth;
            Parts->IconRect.top = Parts->RowRect.top + iconVerticalMargin;
            Parts->IconRect.bottom = Parts->RowRect.bottom - iconVerticalMargin;

            currentX += SmallIconWidth + 4;
        }
    }

    Parts->Flags |= TN_PART_CONTENT;
    Parts->ContentRect.left = currentX;
    Parts->ContentRect.right = Parts->CellRect.right - 6;
    Parts->ContentRect.top = Parts->RowRect.top;
    Parts->ContentRect.bottom = Parts->RowRect.bottom;

    if (Flags & TN_MEASURE_TEXT)
    {
        HDC hdc;
        PH_STRINGREF text;
        HFONT font;
        SIZE textSize;

        if (hdc = GetDC(Context->Handle))
        {
            PhTnpPrepareRowForDraw(Context, hdc, node);

            if (PhTnpGetCellText(Context, node, Column->Id, &text))
            {
                if (node->Font)
                    font = node->Font;
                else
                    font = Context->Font;

                SelectObject(hdc, font);

                if (GetTextExtentPoint32(hdc, text.Buffer, text.Length / sizeof(WCHAR), &textSize))
                {
                    Parts->Flags |= TN_PART_TEXT;
                    Parts->TextRect.left = currentX;
                    Parts->TextRect.right = currentX + textSize.cx;
                    Parts->TextRect.top = Parts->RowRect.top + (Context->RowHeight - textSize.cy) / 2;
                    Parts->TextRect.bottom = Parts->RowRect.bottom - (Context->RowHeight - textSize.cy) / 2;

                    Parts->Text = text;
                    Parts->Font = font;
                }
            }

            ReleaseDC(Context->Handle, hdc);
        }
    }

    return TRUE;
}

BOOLEAN PhTnpGetRowRects(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Start,
    __in ULONG End,
    __in BOOLEAN Clip,
    __out PRECT Rect
    )
{
    LONG startY;
    LONG endY;
    LONG viewWidth;

    if (End >= Context->FlatList->Count)
        return FALSE;
    if (Start > End)
        return FALSE;

    startY = Context->HeaderHeight + ((LONG)Start - Context->VScrollPosition) * Context->RowHeight;
    endY = Context->HeaderHeight + ((LONG)End - Context->VScrollPosition) * Context->RowHeight;

    Rect->left = 0;
    Rect->right = Context->NormalLeft + Context->TotalViewX - Context->HScrollPosition;
    Rect->top = startY;
    Rect->bottom = endY + Context->RowHeight;

    viewWidth = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);

    if (Rect->right > viewWidth)
        Rect->right = viewWidth;

    if (Clip)
    {
        if (Rect->top < Context->HeaderHeight)
            Rect->top = Context->HeaderHeight;
        if (Rect->bottom > Context->ClientRect.bottom)
            Rect->bottom = Context->ClientRect.bottom;
    }

    return TRUE;
}

VOID PhTnpHitTest(
    __in PPH_TREENEW_CONTEXT Context,
    __inout PPH_TREENEW_HIT_TEST HitTest
    )
{
    RECT clientRect;
    LONG x;
    LONG y;
    ULONG index;
    PPH_TREENEW_NODE node;

    HitTest->Flags = 0;
    HitTest->Node = NULL;
    HitTest->Column = NULL;

    clientRect = Context->ClientRect;
    x = HitTest->Point.x;
    y = HitTest->Point.y;

    if (x < 0)
        HitTest->Flags |= TN_HIT_LEFT;
    if (x >= clientRect.right)
        HitTest->Flags |= TN_HIT_RIGHT;
    if (y < 0)
        HitTest->Flags |= TN_HIT_ABOVE;
    if (y >= clientRect.bottom)
        HitTest->Flags |= TN_HIT_BELOW;

    if (HitTest->Flags == 0)
    {
        if (TNP_HIT_TEST_FIXED_DIVIDER(x, Context))
        {
            HitTest->Flags |= TN_HIT_DIVIDER;
        }

        if (y >= Context->HeaderHeight && x < Context->FixedWidth + Context->TotalViewX)
        {
            index = (y - Context->HeaderHeight) / Context->RowHeight + Context->VScrollPosition;

            if (index < Context->FlatList->Count)
            {
                HitTest->Flags |= TN_HIT_ITEM;
                node = Context->FlatList->Items[index];
                HitTest->Node = node;

                if (HitTest->InFlags & TN_TEST_COLUMN)
                {
                    PPH_TREENEW_COLUMN column;
                    LONG columnX;

                    column = NULL;

                    if (x < Context->FixedWidth && Context->FixedColumnVisible)
                    {
                        column = Context->FixedColumn;
                        columnX = 0;
                    }
                    else
                    {
                        LONG currentX;
                        ULONG i;
                        PPH_TREENEW_COLUMN currentColumn;

                        currentX = Context->NormalLeft - Context->HScrollPosition;

                        for (i = 0; i < Context->NumberOfColumnsByDisplay; i++)
                        {
                            currentColumn = Context->ColumnsByDisplay[i];

                            if (x >= currentX && x < currentX + currentColumn->Width)
                            {
                                column = currentColumn;
                                columnX = currentX;
                                break;
                            }

                            currentX += currentColumn->Width;
                        }
                    }

                    HitTest->Column = column;

                    if (column && (HitTest->InFlags & TN_TEST_SUBITEM))
                    {
                        BOOLEAN isFirstColumn;
                        LONG currentX;

                        isFirstColumn = HitTest->Column == Context->FirstColumn;

                        currentX = columnX;
                        currentX += 6;

                        if (isFirstColumn)
                        {
                            currentX += (LONG)node->Level * SmallIconWidth;

                            if (!node->s.IsLeaf)
                            {
                                if (x >= currentX && x < currentX + SmallIconWidth)
                                    HitTest->Flags |= TN_HIT_ITEM_PLUSMINUS;

                                currentX += SmallIconWidth;
                            }

                            if (node->Icon)
                            {
                                if (x >= currentX && x < currentX + SmallIconWidth)
                                    HitTest->Flags |= TN_HIT_ITEM_ICON;

                                currentX += SmallIconWidth + 4;
                            }
                        }

                        if (x >= currentX)
                        {
                            HitTest->Flags |= TN_HIT_ITEM_CONTENT;
                        }
                    }
                }
            }
        }
    }
}

VOID PhTnpSelectRange(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Start,
    __in ULONG End,
    __in ULONG Flags,
    __out_opt PULONG ChangedStart,
    __out_opt PULONG ChangedEnd
    )
{
    ULONG maximum;
    ULONG i;
    PPH_TREENEW_NODE node;
    BOOLEAN targetValue;
    ULONG changedStart;
    ULONG changedEnd;

    if (Context->FlatList->Count == 0)
        return;

    maximum = Context->FlatList->Count - 1;

    if (End > maximum)
    {
        End = maximum;
    }

    if (Start > End)
    {
        // Start is too big, so the selection range becomes empty.
        // Set it to max + 1 so Reset still works.
        Start = maximum + 1;
        End = 0;
    }

    targetValue = !(Flags & TN_SELECT_DESELECT);
    changedStart = maximum;
    changedEnd = 0;

    if (Flags & TN_SELECT_RESET)
    {
        for (i = 0; i < Start; i++)
        {
            node = Context->FlatList->Items[i];

            if (node->Selected)
            {
                node->Selected = FALSE;

                if (changedStart > i)
                    changedStart = i;
                if (changedEnd < i)
                    changedEnd = i;
            }
        }
    }

    for (i = Start; i <= End; i++)
    {
        node = Context->FlatList->Items[i];

        if ((Flags & TN_SELECT_TOGGLE) || node->Selected != targetValue)
        {
            node->Selected = !node->Selected;

            if (changedStart > i)
                changedStart = i;
            if (changedEnd < i)
                changedEnd = i;
        }
    }

    if (Flags & TN_SELECT_RESET)
    {
        for (i = End + 1; i <= maximum; i++)
        {
            node = Context->FlatList->Items[i];

            if (node->Selected)
            {
                node->Selected = FALSE;

                if (changedStart > i)
                    changedStart = i;
                if (changedEnd < i)
                    changedEnd = i;
            }
        }
    }

    Context->Callback(Context->Handle, TreeNewSelectionChanged, NULL, NULL, Context->CallbackContext);

    if (ChangedStart)
        *ChangedStart = changedStart;
    if (ChangedEnd)
        *ChangedEnd = changedEnd;
}

VOID PhTnpSetHotNode(
    __in PPH_TREENEW_CONTEXT Context,
    __in PPH_TREENEW_NODE NewHotNode,
    __in BOOLEAN NewPlusMinusHot
    )
{
    RECT rowRect;
    BOOLEAN needsInvalidate;

    needsInvalidate = FALSE;

    if (Context->HotNode != NewHotNode)
    {
        if (Context->HotNode)
        {
            if (Context->ThemeData && PhTnpGetRowRects(Context, Context->HotNode->Index, Context->HotNode->Index, TRUE, &rowRect))
            {
                // Update the old hot node because it may have a different non-hot background and plus minus part.
                InvalidateRect(Context->Handle, &rowRect, FALSE);
            }
        }

        Context->HotNode = NewHotNode;

        if (Context->HotNode)
        {
            needsInvalidate = TRUE;
        }
    }

    if (Context->HotNode)
    {
        if (Context->HotNode->s.PlusMinusHot != NewPlusMinusHot)
        {
            Context->HotNode->s.PlusMinusHot = NewPlusMinusHot;
            needsInvalidate = TRUE;
        }

        if (needsInvalidate && Context->ThemeData && PhTnpGetRowRects(Context, Context->HotNode->Index, Context->HotNode->Index, TRUE, &rowRect))
        {
            InvalidateRect(Context->Handle, &rowRect, FALSE);
        }
    }
}

BOOLEAN PhTnpEnsureVisibleNode(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Index
    )
{
    LONG viewTop;
    LONG viewBottom;
    LONG rowTop;
    LONG rowBottom;
    LONG deltaY;
    LONG deltaRows;

    if (Index >= Context->FlatList->Count)
        return FALSE;

    viewTop = Context->HeaderHeight;
    viewBottom = Context->ClientRect.bottom - (Context->HScrollVisible ? Context->HScrollHeight : 0);
    rowTop = Context->HeaderHeight + ((LONG)Index - Context->VScrollPosition) * Context->RowHeight;
    rowBottom = rowTop + Context->RowHeight;

    // Check if the row is fully visible.
    if (rowTop >= viewTop && rowBottom <= viewBottom)
        return TRUE;

    deltaY = rowTop - viewTop;

    if (deltaY > 0)
    {
        // The row is below the view area. We want to scroll the row into view at the bottom of the screen.
        // We need to round up when dividing to make sure the node becomes fully visible.
        deltaY = rowBottom - viewBottom;
        deltaRows = (deltaY + Context->RowHeight + 1) / Context->RowHeight; // divide, round up
    }
    else
    {
        deltaRows = deltaY / Context->RowHeight;
    }

    PhTnpScroll(Context, deltaRows, 0);

    return TRUE;
}

BOOLEAN PhTnpProcessFocusKey(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG VirtualKey
    )
{
    ULONG count;
    ULONG index;
    BOOLEAN controlKey;
    BOOLEAN shiftKey;
    ULONG start;
    ULONG end;
    ULONG changedStart;
    ULONG changedEnd;
    RECT rect;

    if (VirtualKey != VK_UP && VirtualKey != VK_DOWN &&
        VirtualKey != VK_HOME && VirtualKey != VK_END &&
        VirtualKey != VK_PRIOR && VirtualKey != VK_NEXT)
    {
        return FALSE;
    }

    count = Context->FlatList->Count;

    if (count == 0)
        return TRUE;

    // Find the new node to focus.

    switch (VirtualKey)
    {
    case VK_UP:
        {
            if (Context->FocusNode && Context->FocusNode->Index > 0)
            {
                index = Context->FocusNode->Index - 1;
            }
            else
            {
                index = 0;
            }
        }
        break;
    case VK_DOWN:
        {
            if (Context->FocusNode)
            {
                index = Context->FocusNode->Index + 1;

                if (index >= count)
                    index = count - 1;
            }
            else
            {
                index = 0;
            }
        }
        break;
    case VK_HOME:
        index = 0;
        break;
    case VK_END:
        index = count - 1;
        break;
    case VK_PRIOR:
    case VK_NEXT:
        {
            LONG rowsPerPage;

            if (Context->FocusNode)
                index = Context->FocusNode->Index;
            else
                index = 0;

            rowsPerPage = Context->ClientRect.bottom - Context->HeaderHeight - (Context->HScrollVisible ? Context->HScrollHeight : 0);

            if (rowsPerPage < 0)
                return TRUE;

            rowsPerPage = rowsPerPage / Context->RowHeight - 1;

            if (rowsPerPage < 0)
                return TRUE;

            if (VirtualKey == VK_PRIOR)
            {
                ULONG startOfPageIndex;

                startOfPageIndex = Context->VScrollPosition;

                if (index > startOfPageIndex)
                {
                    index = startOfPageIndex;
                }
                else
                {
                    // Already at or before the start of the page. Go back a page.
                    if (index >= (ULONG)rowsPerPage)
                        index -= rowsPerPage;
                    else
                        index = 0;
                }
            }
            else
            {
                ULONG endOfPageIndex;

                endOfPageIndex = Context->VScrollPosition + rowsPerPage;

                if (endOfPageIndex >= count)
                    endOfPageIndex = count - 1;

                if (index < endOfPageIndex)
                {
                    index = endOfPageIndex;
                }
                else
                {
                    // Already at or after the end of the page. Go forward a page.
                    index += rowsPerPage;

                    if (index >= count)
                        index = count - 1;
                }
            }
        }
        break;
    }

    // Select the relevant nodes.

    controlKey = GetKeyState(VK_CONTROL) < 0;
    shiftKey = GetKeyState(VK_SHIFT) < 0;

    Context->FocusNode = Context->FlatList->Items[index];
    PhTnpEnsureVisibleNode(Context, index);
    PhTnpSetHotNode(Context, Context->FocusNode, FALSE);

    if (shiftKey && Context->MarkNode)
    {
        if (index > Context->MarkNode->Index)
        {
            start = Context->MarkNode->Index;
            end = index;
        }
        else
        {
            start = index;
            end = Context->MarkNode->Index;
        }

        PhTnpSelectRange(Context, start, end, TN_SELECT_RESET, &changedStart, &changedEnd);

        if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
        {
            InvalidateRect(Context->Handle, &rect, FALSE);
        }
    }
    else if (!controlKey)
    {
        Context->MarkNode = Context->FocusNode;
        PhTnpSelectRange(Context, index, index, TN_SELECT_RESET, &changedStart, &changedEnd);

        if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
        {
            InvalidateRect(Context->Handle, &rect, FALSE);
        }
    }

    PhTnpPopTooltip(Context);

    return TRUE;
}

BOOLEAN PhTnpProcessNodeKey(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG VirtualKey
    )
{
    BOOLEAN controlKey;
    BOOLEAN shiftKey;
    ULONG changedStart;
    ULONG changedEnd;
    RECT rect;

    if (VirtualKey != VK_SPACE && VirtualKey != VK_LEFT && VirtualKey != VK_RIGHT)
    {
        return FALSE;
    }

    if (!Context->FocusNode)
        return TRUE;

    controlKey = GetKeyState(VK_CONTROL) < 0;
    shiftKey = GetKeyState(VK_SHIFT) < 0;

    switch (VirtualKey)
    {
    case VK_SPACE:
        {
            if (controlKey)
            {
                // Control key: toggle the selection on the focused node.

                Context->MarkNode = Context->FocusNode;
                PhTnpSelectRange(Context, Context->FocusNode->Index, Context->FocusNode->Index, TN_SELECT_TOGGLE, &changedStart, &changedEnd);

                if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                {
                    InvalidateRect(Context->Handle, &rect, FALSE);
                }
            }
            else if (shiftKey)
            {
                ULONG start;
                ULONG end;

                // Shift key: select a range from the selection mark node to the focused node.

                if (Context->FocusNode->Index > Context->MarkNode->Index)
                {
                    start = Context->MarkNode->Index;
                    end = Context->FocusNode->Index;
                }
                else
                {
                    start = Context->FocusNode->Index;
                    end = Context->MarkNode->Index;
                }

                PhTnpSelectRange(Context, start, end, TN_SELECT_RESET, &changedStart, &changedEnd);

                if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                {
                    InvalidateRect(Context->Handle, &rect, FALSE);
                }
            }
        }
        break;
    case VK_LEFT:
        {
            ULONG i;
            ULONG targetLevel;
            PPH_TREENEW_NODE newNode;

            // If the node is expanded, collapse it. Otherwise, select the node's 
            // parent.
            if (!Context->FocusNode->s.IsLeaf && Context->FocusNode->Expanded)
            {
                PhTnpSetExpandedNode(Context, Context->FocusNode, FALSE);
            }
            else if (Context->FocusNode->Level != 0)
            {
                i = Context->FocusNode->Index;
                targetLevel = Context->FocusNode->Level - 1;

                while (i != 0)
                {
                    i--;
                    newNode = Context->FlatList->Items[i];

                    if (newNode->Level == targetLevel)
                    {
                        Context->FocusNode = newNode;
                        Context->MarkNode = newNode;
                        PhTnpEnsureVisibleNode(Context, i);
                        PhTnpSetHotNode(Context, newNode, FALSE);
                        PhTnpSelectRange(Context, i, i, TN_SELECT_RESET, &changedStart, &changedEnd);

                        if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                        {
                            InvalidateRect(Context->Handle, &rect, FALSE);
                        }

                        PhTnpPopTooltip(Context);

                        break;
                    }
                }
            }
        }
        break;
    case VK_RIGHT:
        {
            PPH_TREENEW_NODE newNode;

            if (!Context->FocusNode->s.IsLeaf)
            {
                // If the node is collapsed, expand it. Otherwise, select the node's 
                // first child.
                if (!Context->FocusNode->Expanded)
                {
                    PhTnpSetExpandedNode(Context, Context->FocusNode, TRUE);
                }
                else
                {
                    if (Context->FocusNode->Index + 1 < Context->FlatList->Count)
                    {
                        newNode = Context->FlatList->Items[Context->FocusNode->Index + 1];

                        if (newNode->Level == Context->FocusNode->Level + 1)
                        {
                            Context->FocusNode = newNode;
                            Context->MarkNode = newNode;
                            PhTnpEnsureVisibleNode(Context, Context->FocusNode->Index + 1);
                            PhTnpSetHotNode(Context, newNode, FALSE);
                            PhTnpSelectRange(Context, Context->FocusNode->Index, Context->FocusNode->Index, TN_SELECT_RESET, &changedStart, &changedEnd);

                            if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                            {
                                InvalidateRect(Context->Handle, &rect, FALSE);
                            }

                            PhTnpPopTooltip(Context);
                        }
                    }
                }
            }
        }
        break;
    }

    return TRUE;
}

VOID PhTnpProcessSearchKey(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Character
    )
{
    LONG messageTime;
    BOOLEAN newSearch;
    PH_TREENEW_SEARCH_EVENT searchEvent;
    PPH_TREENEW_NODE foundNode;
    ULONG changedStart;
    ULONG changedEnd;
    RECT rect;

    if (Context->FlatList->Count == 0)
        return;

    messageTime = GetMessageTime();
    newSearch = FALSE;

    // Check if the search timed out.
    if (messageTime - Context->SearchMessageTime > PH_TREENEW_SEARCH_TIMEOUT)
    {
        Context->SearchStringCount = 0;
        Context->SearchFailed = FALSE;
        newSearch = TRUE;
        Context->SearchSingleCharMode = TRUE;
    }

    Context->SearchMessageTime = messageTime;

    // Append the character to the search buffer.

    if (!Context->SearchString)
    {
        Context->AllocatedSearchString = 32;
        Context->SearchString = PhAllocate(Context->AllocatedSearchString * sizeof(WCHAR));
        newSearch = TRUE;
        Context->SearchSingleCharMode = TRUE;
    }

    if (Context->SearchStringCount > PH_TREENEW_SEARCH_MAXIMUM_LENGTH)
    {
        // The search string has become too long. Fail the search.
        if (!Context->SearchFailed)
        {
            MessageBeep(0);
            Context->SearchFailed = TRUE;
            return;
        }
    }
    else if (Context->SearchStringCount == Context->AllocatedSearchString)
    {
        Context->AllocatedSearchString *= 2;
        Context->SearchString = PhReAllocate(Context->SearchString, Context->AllocatedSearchString * sizeof(WCHAR));
    }

    Context->SearchString[Context->SearchStringCount++] = (WCHAR)Character;

    if (Context->SearchString[Context->SearchStringCount - 1] != Context->SearchString[0])
    {
        // The user has stopped typing the same character (or never started). Turn single-character search 
        // off.
        Context->SearchSingleCharMode = FALSE;
    }

    searchEvent.FoundIndex = -1;

    if (Context->FocusNode)
    {
        searchEvent.StartIndex = Context->FocusNode->Index;

        if (newSearch || Context->SearchSingleCharMode)
        {
            // If it's a new search, start at the next item so the user doesn't find the same item again.
            searchEvent.StartIndex++;

            if (searchEvent.StartIndex == Context->FlatList->Count)
                searchEvent.StartIndex = 0;
        }
    }
    else
    {
        searchEvent.StartIndex = 0;
    }

    searchEvent.String.Buffer = Context->SearchString;

    if (!Context->SearchSingleCharMode)
        searchEvent.String.Length = (USHORT)(Context->SearchStringCount * sizeof(WCHAR));
    else
        searchEvent.String.Length = sizeof(WCHAR);

    // Give the user a chance to modify how the search is performed.
    if (!Context->Callback(Context->Handle, TreeNewIncrementalSearch, &searchEvent, NULL, Context->CallbackContext))
    {
        // Use the default search function.
        if (!PhTnpDefaultIncrementalSearch(Context, &searchEvent, TRUE, TRUE))
        {
            return;
        }
    }

    if (searchEvent.FoundIndex == -1 && !Context->SearchFailed)
    {
        // No search result. Beep to indicate an error, and set the flag so we don't beep again.
        // But don't beep if the first character was a space, because that's used for other purposes 
        // elsewhere (see PhTnpProcessNodeKey).
        if (searchEvent.String.Buffer[0] != ' ')
        {
            MessageBeep(0);
        }

        Context->SearchFailed = TRUE;
        return;
    }

    if (searchEvent.FoundIndex < 0 || searchEvent.FoundIndex >= (LONG)Context->FlatList->Count)
        return;

    foundNode = Context->FlatList->Items[searchEvent.FoundIndex];
    Context->FocusNode = foundNode;
    PhTnpEnsureVisibleNode(Context, searchEvent.FoundIndex);
    PhTnpSetHotNode(Context, foundNode, FALSE);
    PhTnpSelectRange(Context, searchEvent.FoundIndex, searchEvent.FoundIndex, TN_SELECT_RESET, &changedStart, &changedEnd);

    if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
    {
        InvalidateRect(Context->Handle, &rect, FALSE);
    }

    PhTnpPopTooltip(Context);
}

BOOLEAN PhTnpDefaultIncrementalSearch(
    __in PPH_TREENEW_CONTEXT Context,
    __inout PPH_TREENEW_SEARCH_EVENT SearchEvent,
    __in BOOLEAN Partial,
    __in BOOLEAN Wrap
    )
{
    LONG startIndex;
    LONG currentIndex;
    LONG foundIndex;
    BOOLEAN firstTime;

    if (Context->FlatList->Count == 0)
        return FALSE;
    if (!Context->FirstColumn)
        return FALSE;

    startIndex = SearchEvent->StartIndex;
    currentIndex = startIndex;
    foundIndex = -1;
    firstTime = TRUE;

    while (TRUE)
    {
        PH_STRINGREF text;

        if (currentIndex >= (LONG)Context->FlatList->Count)
        {
            if (Wrap)
                currentIndex = 0;
            else
                break;
        }

        // We use the firstTime variable instead of a simpler check because 
        // we want to include the current item in the search. E.g. the 
        // current item is the only item beginning with "Z". If the user 
        // searches for "Z", we want to return the current item as being 
        // found.
        if (!firstTime && currentIndex == startIndex)
            break;

        if (PhTnpGetCellText(Context, Context->FlatList->Items[currentIndex], Context->FirstColumn->Id, &text))
        {
            if (Partial)
            {
                if (PhStartsWithStringRef(&text, &SearchEvent->String, TRUE))
                {
                    foundIndex = currentIndex;
                    break;
                }
            }
            else
            {
                if (PhEqualStringRef(&text, &SearchEvent->String, TRUE))
                {
                    foundIndex = currentIndex;
                    break;
                }
            }
        }

        currentIndex++;
        firstTime = FALSE;
    }

    SearchEvent->FoundIndex = foundIndex;

    return TRUE;
}

VOID PhTnpUpdateScrollBars(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    RECT clientRect;
    LONG width;
    LONG height;
    LONG contentWidth;
    LONG contentHeight;
    SCROLLINFO scrollInfo;
    LONG oldPosition;

    clientRect = Context->ClientRect;
    width = clientRect.right - Context->FixedWidth;
    height = clientRect.bottom - Context->HeaderHeight;

    contentWidth = Context->TotalViewX;
    contentHeight = (LONG)Context->FlatList->Count * Context->RowHeight;

    if (contentHeight > height)
    {
        // We need a vertical scrollbar, so we can't use that area of the screen for content.
        width -= Context->VScrollWidth;
    }

    if (contentWidth > width)
    {
        height -= Context->HScrollHeight;
    }

    // Vertical scroll bar
    if (contentHeight > height && contentHeight != 0)
    {
        scrollInfo.cbSize = sizeof(SCROLLINFO);
        scrollInfo.fMask = SIF_POS;
        GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);
        oldPosition = scrollInfo.nPos;

        scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
        scrollInfo.nMin = 0;
        scrollInfo.nMax = Context->FlatList->Count - 1;
        scrollInfo.nPage = height / Context->RowHeight;
        SetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo, TRUE);

        // The scroll position may have changed due to the modified scroll range.
        scrollInfo.fMask = SIF_POS;
        GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);

        if (scrollInfo.nPos != oldPosition)
        {
            Context->VScrollPosition = scrollInfo.nPos;
            InvalidateRect(Context->Handle, NULL, FALSE); // TODO: Optimize
        }

        ShowWindow(Context->VScrollHandle, SW_SHOW);
        Context->VScrollVisible = TRUE;
    }
    else
    {
        ShowWindow(Context->VScrollHandle, SW_HIDE);
        Context->VScrollPosition = 0;
        Context->VScrollVisible = FALSE;
    }

    // Horizontal scroll bar
    if (contentWidth > width && contentWidth != 0)
    {
        scrollInfo.cbSize = sizeof(SCROLLINFO);
        scrollInfo.fMask = SIF_POS;
        GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);
        oldPosition = scrollInfo.nPos;

        scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
        scrollInfo.nMin = 0;
        scrollInfo.nMax = contentWidth - 1;
        scrollInfo.nPage = width;
        SetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo, TRUE);

        scrollInfo.fMask = SIF_POS;
        GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);

        if (scrollInfo.nPos != oldPosition)
        {
            Context->HScrollPosition = scrollInfo.nPos;
            InvalidateRect(Context->Handle, NULL, FALSE); // TODO: Optimize
        }

        ShowWindow(Context->HScrollHandle, SW_SHOW);
        Context->HScrollVisible = TRUE;
    }
    else
    {
        ShowWindow(Context->HScrollHandle, SW_HIDE);
        Context->HScrollPosition = 0;
        Context->HScrollVisible = FALSE;
    }

    ShowWindow(Context->FillerBoxHandle, (Context->VScrollVisible && Context->HScrollVisible) ? SW_SHOW : SW_HIDE);
}

VOID PhTnpScroll(
    __in PPH_TREENEW_CONTEXT Context,
    __in LONG DeltaRows,
    __in LONG DeltaX
    )
{
    SCROLLINFO scrollInfo;
    LONG oldPosition;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;

    if (DeltaRows != 0 && Context->VScrollVisible)
    {
        GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);
        oldPosition = scrollInfo.nPos;

        if (DeltaRows == MINLONG)
            scrollInfo.nPos = 0;
        else if (DeltaRows == MAXLONG)
            scrollInfo.nPos = Context->FlatList->Count - 1;
        else
            scrollInfo.nPos += DeltaRows;

        SetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo, TRUE);
        GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);

        if (scrollInfo.nPos != oldPosition)
        {
            Context->VScrollPosition = scrollInfo.nPos;
            InvalidateRect(Context->Handle, NULL, FALSE); // TODO: Optimize
        }
    }

    if (DeltaX != 0 && Context->HScrollVisible)
    {
        GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);
        oldPosition = scrollInfo.nPos;

        if (DeltaX == MINLONG)
            scrollInfo.nPos = 0;
        else if (DeltaX == MAXLONG)
            scrollInfo.nPos = Context->TotalViewX;
        else
            scrollInfo.nPos += DeltaX;

        SetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo, TRUE);
        GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);

        if (scrollInfo.nPos != oldPosition)
        {
            Context->HScrollPosition = scrollInfo.nPos;
            InvalidateRect(Context->Handle, NULL, FALSE); // TODO: Optimize
        }
    }
}

VOID PhTnpPaint(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in HDC hdc,
    __in PRECT PaintRect
    )
{
    RECT viewRect;
    LONG vScrollPosition;
    LONG hScrollPosition;
    LONG firstRowToUpdate;
    LONG lastRowToUpdate;
    LONG i;
    LONG j;
    PPH_TREENEW_NODE node;
    PPH_TREENEW_COLUMN column;
    RECT rowRect;
    LONG x;
    BOOLEAN fixedUpdate;
    LONG normalUpdateLeftX;
    LONG normalUpdateRightX;
    LONG normalUpdateLeftIndex;
    LONG normalUpdateRightIndex;
    LONG normalTotalX;
    RECT cellRect;
    HBRUSH backBrush;
    HRGN oldClipRegion;

    if (!Context->ThemeInitialized)
    {
        PhTnpUpdateThemeData(Context);
        Context->ThemeInitialized = TRUE;
    }

    viewRect = Context->ClientRect;

    if (Context->VScrollVisible)
        viewRect.right -= Context->VScrollWidth;

    vScrollPosition = Context->VScrollPosition;
    hScrollPosition = Context->HScrollPosition;

    // Calculate the indicies of the first and last rows that need painting. These indicies are relative to the top of the view area.

    firstRowToUpdate = (PaintRect->top - Context->HeaderHeight) / Context->RowHeight;
    lastRowToUpdate = (PaintRect->bottom - Context->HeaderHeight - 1) / Context->RowHeight; // minus one since bottom is exclusive

    if (firstRowToUpdate < 0)
        firstRowToUpdate = 0;

    rowRect.left = 0;
    rowRect.top = Context->HeaderHeight + firstRowToUpdate * Context->RowHeight;
    rowRect.right = Context->NormalLeft + Context->TotalViewX - Context->HScrollPosition;
    rowRect.bottom = rowRect.top + Context->RowHeight;

    // Change the indicies to absolute row indicies.

    firstRowToUpdate += vScrollPosition;
    lastRowToUpdate += vScrollPosition;

    if (lastRowToUpdate >= (LONG)Context->FlatList->Count)
        lastRowToUpdate = Context->FlatList->Count - 1;

    // Determine whether the fixed column needs painting, and which normal columns need painting.

    fixedUpdate = FALSE;

    if (Context->FixedColumnVisible && PaintRect->left < Context->FixedWidth)
        fixedUpdate = TRUE;

    x = Context->NormalLeft - hScrollPosition;
    normalUpdateLeftX = viewRect.right;
    normalUpdateLeftIndex = 0;
    normalUpdateRightX = 0;
    normalUpdateRightIndex = -1;

    for (j = 0; j < (LONG)Context->NumberOfColumnsByDisplay; j++)
    {
        column = Context->ColumnsByDisplay[j];

        if (x + column->Width >= Context->NormalLeft && x + column->Width >= PaintRect->left && x < PaintRect->right)
        {
            if (normalUpdateLeftX > x)
            {
                normalUpdateLeftX = x;
                normalUpdateLeftIndex = j;
            }

            if (normalUpdateRightX < x + column->Width)
            {
                normalUpdateRightX = x + column->Width;
                normalUpdateRightIndex = j;
            }
        }

        x += column->Width;
    }

    normalTotalX = x;

    if (normalUpdateRightIndex >= (LONG)Context->NumberOfColumnsByDisplay)
        normalUpdateRightIndex = Context->NumberOfColumnsByDisplay - 1;

    // Paint the rows.

    SelectObject(hdc, Context->Font);
    SetBkMode(hdc, TRANSPARENT);

    for (i = firstRowToUpdate; i <= lastRowToUpdate; i++)
    {
        node = Context->FlatList->Items[i];

        // Prepare the row for drawing.

        PhTnpPrepareRowForDraw(Context, hdc, node);

        if (node->Selected && !Context->ThemeHasItemBackground)
        {
            // Non-themed background
            if (Context->HasFocus)
            {
                SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                backBrush = GetSysColorBrush(COLOR_HIGHLIGHT);
            }
            else
            {
                SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
                backBrush = GetSysColorBrush(COLOR_BTNFACE);
            }
        }
        else
        {
            SetTextColor(hdc, node->s.DrawForeColor);
            SetDCBrushColor(hdc, node->s.DrawBackColor);
            backBrush = GetStockObject(DC_BRUSH);
        }

        FillRect(hdc, &rowRect, backBrush);

        if (Context->ThemeHasItemBackground)
        {
            INT stateId;

            // Themed background

            if (node->Selected)
            {
                if (node == Context->HotNode)
                    stateId = TREIS_HOTSELECTED;
                else if (!Context->HasFocus)
                    stateId = TREIS_SELECTEDNOTFOCUS;
                else
                    stateId = TREIS_SELECTED;
            }
            else
            {
                if (node == Context->HotNode)
                    stateId = TREIS_HOT;
                else
                    stateId = -1;
            }

            if (stateId != -1)
            {
                DrawThemeBackground_I(
                    Context->ThemeData,
                    hdc,
                    TVP_TREEITEM,
                    stateId,
                    &rowRect,
                    PaintRect
                    );
            }
        }

        // Paint the fixed column.

        cellRect.top = rowRect.top;
        cellRect.bottom = rowRect.bottom;

        if (fixedUpdate)
        {
            cellRect.left = 0;
            cellRect.right = Context->FixedWidth;
            PhTnpDrawCell(Context, hdc, &cellRect, node, Context->FixedColumn, i, -1);
        }

        // Paint the normal columns.

        if (normalUpdateLeftX < normalUpdateRightX)
        {
            cellRect.left = normalUpdateLeftX;
            cellRect.right = cellRect.left;

            oldClipRegion = CreateRectRgn(0, 0, 0, 0);

            if (GetClipRgn(hdc, oldClipRegion) != 1)
            {
                DeleteObject(oldClipRegion);
                oldClipRegion = NULL;
            }

            IntersectClipRect(hdc, Context->NormalLeft, cellRect.top, viewRect.right, cellRect.bottom);

            for (j = normalUpdateLeftIndex; j <= normalUpdateRightIndex; j++)
            {
                column = Context->ColumnsByDisplay[j];

                cellRect.left = cellRect.right;
                cellRect.right = cellRect.left + column->Width;
                PhTnpDrawCell(Context, hdc, &cellRect, node, column, i, j);
            }

            SelectClipRgn(hdc, oldClipRegion);

            if (oldClipRegion)
            {
                DeleteObject(oldClipRegion);
            }
        }

        rowRect.top += Context->RowHeight;
        rowRect.bottom += Context->RowHeight;
    }

    if (i == Context->FlatList->Count)
    {
        // Fill the rest of the space on the bottom with the window color.
        rowRect.bottom = viewRect.bottom;
        FillRect(hdc, &rowRect, GetSysColorBrush(COLOR_WINDOW));
    }

    if (normalTotalX < viewRect.right && viewRect.right >= PaintRect->left && normalTotalX < PaintRect->right)
    {
        // Fill the rest of the space on the right with the window color.
        rowRect.left = normalTotalX;
        rowRect.top = Context->HeaderHeight;
        rowRect.right = viewRect.right;
        rowRect.bottom = viewRect.bottom;
        FillRect(hdc, &rowRect, GetSysColorBrush(COLOR_WINDOW));
    }

    if (Context->FixedColumnVisible && Context->FixedWidth >= PaintRect->left && Context->FixedWidth < PaintRect->right)
    {
        PhTnpDrawDivider(Context, hdc, &viewRect);
    }
}

VOID PhTnpPrepareRowForDraw(
    __in PPH_TREENEW_CONTEXT Context,
    __in HDC hdc,
    __inout PPH_TREENEW_NODE Node
    )
{
    if (!Node->s.CachedColorValid)
    {
        PH_TREENEW_GET_NODE_COLOR getNodeColor;

        getNodeColor.Flags = 0;
        getNodeColor.Node = Node;
        getNodeColor.BackColor = RGB(0xff, 0xff, 0xff);
        getNodeColor.ForeColor = RGB(0x00, 0x00, 0x00);

        if (Context->Callback(
            Context->Handle,
            TreeNewGetNodeColor,
            &getNodeColor,
            NULL,
            Context->CallbackContext
            ))
        {
            Node->BackColor = getNodeColor.BackColor;
            Node->ForeColor = getNodeColor.ForeColor;
            Node->UseAutoForeColor = !!(getNodeColor.Flags & TN_AUTO_FORECOLOR);

            if (getNodeColor.Flags & TN_CACHE)
                Node->s.CachedColorValid = TRUE;
        }
        else
        {
            Node->BackColor = getNodeColor.BackColor;
            Node->ForeColor = getNodeColor.ForeColor;
        }
    }

    Node->s.DrawForeColor = Node->ForeColor;

    if (Node->UseTempBackColor)
        Node->s.DrawBackColor = Node->TempBackColor;
    else
        Node->s.DrawBackColor = Node->BackColor;

    if (!Node->s.CachedFontValid)
    {
        PH_TREENEW_GET_NODE_FONT getNodeFont;

        getNodeFont.Flags = 0;
        getNodeFont.Node = Node;
        getNodeFont.Font = NULL;

        if (Context->Callback(
            Context->Handle,
            TreeNewGetNodeFont,
            &getNodeFont,
            NULL,
            Context->CallbackContext
            ))
        {
            Node->Font = getNodeFont.Font;

            if (getNodeFont.Flags & TN_CACHE)
                Node->s.CachedFontValid = TRUE;
        }
        else
        {
            Node->Font = NULL;
        }
    }

    if (!Node->s.CachedIconValid)
    {
        PH_TREENEW_GET_NODE_ICON getNodeIcon;

        getNodeIcon.Flags = 0;
        getNodeIcon.Node = Node;
        getNodeIcon.Icon = NULL;

        if (Context->Callback(
            Context->Handle,
            TreeNewGetNodeIcon,
            &getNodeIcon,
            NULL,
            Context->CallbackContext
            ))
        {
            Node->Icon = getNodeIcon.Icon;

            if (getNodeIcon.Flags & TN_CACHE)
                Node->s.CachedIconValid = TRUE;
        }
        else
        {
            Node->Icon = NULL;
        }
    }

    if (Node->UseAutoForeColor || Node->UseTempBackColor)
    {
        if (PhGetColorBrightness(Node->s.DrawBackColor) > 100) // slightly less than half
            Node->s.DrawForeColor = RGB(0x00, 0x00, 0x00);
        else
            Node->s.DrawForeColor = RGB(0xff, 0xff, 0xff);
    }
}

VOID PhTnpDrawCell(
    __in PPH_TREENEW_CONTEXT Context,
    __in HDC hdc,
    __in PRECT CellRect,
    __in PPH_TREENEW_NODE Node,
    __in PPH_TREENEW_COLUMN Column,
    __in LONG RowIndex,
    __in LONG ColumnIndex
    )
{
    HFONT font; // font to use
    HFONT oldFont;
    PH_STRINGREF text; // text to draw
    RECT textRect; // working rectangle, modified as needed
    ULONG textFlags; // DT_* flags
    LONG iconVerticalMargin; // top/bottom margin for icons (determined using height of small icon)

    font = Node->Font;
    textFlags = Column->TextFlags;

    textRect = *CellRect;

    // Initial margins used by default list view
    textRect.left += 6;
    textRect.right -= 6;

    // icon margin = (height of row - height of small icon) / 2
    iconVerticalMargin = ((textRect.bottom - textRect.top) - SmallIconHeight) / 2;

    textRect.top += iconVerticalMargin;
    textRect.bottom -= iconVerticalMargin;

    if (Column == Context->FirstColumn)
    {
        BOOLEAN needsClip;
        HRGN oldClipRegion;

        textRect.left += Node->Level * SmallIconWidth;

        // The icon may need to be clipped if the column is too small.
        needsClip = Column->Width < textRect.left + (Context->CanAnyExpand ? SmallIconWidth : 0) + (Node->Icon ? SmallIconWidth : 0);

        if (needsClip)
        {
            oldClipRegion = CreateRectRgn(0, 0, 0, 0);

            if (GetClipRgn(hdc, oldClipRegion) != 1)
            {
                DeleteObject(oldClipRegion);
                oldClipRegion = NULL;
            }

            // Clip contents to the column.
            IntersectClipRect(hdc, CellRect->left, textRect.top, CellRect->right, textRect.bottom);
        }

        if (Context->CanAnyExpand) // flag is used so we can avoid indenting when it's a flat list
        {
            BOOLEAN drewUsingTheme = FALSE;
            RECT themeRect;

            if (!Node->s.IsLeaf)
            {
                // Draw the plus/minus glyph.

                themeRect.left = textRect.left;
                themeRect.right = themeRect.left + SmallIconWidth;
                themeRect.top = textRect.top;
                themeRect.bottom = themeRect.top + SmallIconHeight;

                if (Context->ThemeHasGlyph)
                {
                    INT partId;
                    INT stateId;

                    partId = (Node == Context->HotNode && Node->s.PlusMinusHot && Context->ThemeHasHotGlyph) ? TVP_HOTGLYPH : TVP_GLYPH;
                    stateId = Node->Expanded ? GLPS_OPENED : GLPS_CLOSED;

                    if (SUCCEEDED(DrawThemeBackground_I(
                        Context->ThemeData,
                        hdc,
                        partId,
                        stateId,
                        &themeRect,
                        NULL
                        )))
                        drewUsingTheme = TRUE;
                }

                if (!drewUsingTheme)
                {
                    ULONG glyphWidth;
                    ULONG glyphHeight;
                    RECT glyphRect;

                    glyphWidth = SmallIconWidth / 2;
                    glyphHeight = SmallIconHeight / 2;

                    glyphRect.left = textRect.left + (SmallIconWidth - glyphWidth) / 2;
                    glyphRect.right = glyphRect.left + glyphWidth;
                    glyphRect.top = textRect.top + (SmallIconHeight - glyphHeight) / 2;
                    glyphRect.bottom = glyphRect.top + glyphHeight;

                    PhTnpDrawPlusMinusGlyph(hdc, &glyphRect, !Node->Expanded);
                }
            }

            textRect.left += SmallIconWidth;
        }

        // Draw the icon.
        if (Node->Icon)
        {
            DrawIconEx(
                hdc,
                textRect.left,
                textRect.top,
                Node->Icon,
                SmallIconWidth,
                SmallIconHeight,
                0,
                NULL,
                DI_NORMAL
                );

            textRect.left += SmallIconWidth + 4; // 4px margin
        }

        if (needsClip)
        {
            SelectClipRgn(hdc, oldClipRegion);

            if (oldClipRegion)
                DeleteObject(oldClipRegion);
        }

        if (textRect.left > textRect.right)
            textRect.left = textRect.right;
    }

    if (Column->CustomDraw)
    {
        BOOLEAN result;
        PH_TREENEW_CUSTOM_DRAW customDraw;
        INT savedDc;

        customDraw.Node = Node;
        customDraw.Column = Column;
        customDraw.Dc = hdc;
        customDraw.CellRect = *CellRect;
        customDraw.TextRect = textRect;

        savedDc = SaveDC(hdc);
        result = Context->Callback(Context->Handle, TreeNewCustomDraw, &customDraw, NULL, Context->CallbackContext);
        RestoreDC(hdc, savedDc);

        if (result)
            return;
    }

    if (PhTnpGetCellText(Context, Node, Column->Id, &text))
    {
        if (!(textFlags & (DT_PATH_ELLIPSIS | DT_WORD_ELLIPSIS)))
            textFlags |= DT_END_ELLIPSIS;

        textFlags |= DT_NOPREFIX | DT_VCENTER | DT_SINGLELINE;

        textRect.top = CellRect->top;
        textRect.bottom = CellRect->bottom;

        if (font)
            oldFont = SelectObject(hdc, font);

        DrawText(
            hdc,
            text.Buffer,
            text.Length / 2,
            &textRect,
            textFlags
            );

        if (font)
            SelectObject(hdc, oldFont);
    }
}

VOID PhTnpDrawDivider(
    __in PPH_TREENEW_CONTEXT Context,
    __in HDC hdc,
    __in PRECT ClientRect
    )
{
    POINT points[2];

    points[0].x = Context->FixedWidth;
    points[0].y = 0;
    points[1].x = Context->FixedWidth;
    points[1].y = ClientRect->bottom;
    SetDCPenColor(hdc, RGB(0x77, 0x77, 0x77));
    SelectObject(hdc, GetStockObject(DC_PEN));
    Polyline(hdc, points, 2);
}

VOID PhTnpDrawPlusMinusGlyph(
    __in HDC hdc,
    __in PRECT Rect,
    __in BOOLEAN Plus
    )
{
    INT savedDc;
    ULONG width;
    ULONG height;
    POINT points[2];

    savedDc = SaveDC(hdc);

    SelectObject(hdc, GetStockObject(DC_PEN));
    SetDCPenColor(hdc, RGB(0x55, 0x55, 0x55));
    SelectObject(hdc, GetStockObject(DC_BRUSH));
    SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff));

    width = Rect->right - Rect->left;
    height = Rect->bottom - Rect->top;

    // Draw the rectangle.
    Rectangle(hdc, Rect->left, Rect->top, Rect->right + 1, Rect->bottom + 1);

    SetDCPenColor(hdc, RGB(0x00, 0x00, 0x00));

    // Draw the horizontal line.
    points[0].x = Rect->left + 2;
    points[0].y = Rect->top + height / 2;
    points[1].x = Rect->right - 2 + 1;
    points[1].y = points[0].y;
    Polyline(hdc, points, 2);

    if (Plus)
    {
        // Draw the vertical line.
        points[0].x = Rect->left + width / 2;
        points[0].y = Rect->top + 2;
        points[1].x = points[0].x;
        points[1].y = Rect->bottom - 2 + 1;
        Polyline(hdc, points, 2);
    }

    RestoreDC(hdc, savedDc);
}

VOID PhTnpInitializeTooltips(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    TOOLINFO toolInfo;

    Context->TooltipsHandle = CreateWindowEx(
        WS_EX_TRANSPARENT, // solves double-click problem
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_NOPREFIX,
        0,
        0,
        0,
        0,
        NULL,
        NULL,
        Context->InstanceHandle,
        NULL
        );

    if (!Context->TooltipsHandle)
        return;

    memset(&toolInfo, 0, sizeof(TOOLINFO));
    toolInfo.cbSize = sizeof(TOOLINFO);
    toolInfo.uFlags = TTF_TRANSPARENT;
    toolInfo.hwnd = Context->Handle;
    toolInfo.uId = 0;
    toolInfo.lpszText = LPSTR_TEXTCALLBACK;
    SendMessage(Context->TooltipsHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

    SendMessage(Context->TooltipsHandle, WM_SETFONT, (WPARAM)Context->Font, FALSE);
    Context->TooltipFont = Context->Font;
    SendMessage(Context->TooltipsHandle, TTM_SETMAXTIPWIDTH, 0, MAXSHORT);
}

VOID PhTnpGetTooltipText(
    __in PPH_TREENEW_CONTEXT Context,
    __in PPOINT Point,
    __out PWSTR *Text
    )
{
    PH_TREENEW_HIT_TEST hitTest;
    PH_TREENEW_CELL_PARTS parts;
    PH_TREENEW_GET_CELL_TOOLTIP getCellTooltip;

    hitTest.Point = *Point;
    hitTest.InFlags = TN_TEST_COLUMN | TN_TEST_SUBITEM;
    PhTnpHitTest(Context, &hitTest);

    if (!(hitTest.Flags & TN_HIT_ITEM))
        return;
    if (hitTest.Flags & TN_HIT_DIVIDER)
        return;
    if (!hitTest.Column)
        return;

    if (Context->TooltipIndex != hitTest.Node->Index || Context->TooltipId != hitTest.Column->Id)
    {
        Context->TooltipIndex = hitTest.Node->Index;
        Context->TooltipId = hitTest.Column->Id;
        Context->TooltipUnfolding = FALSE;

        getCellTooltip.Flags = 0;
        getCellTooltip.Node = hitTest.Node;
        getCellTooltip.Column = hitTest.Column;
        getCellTooltip.Unfolding = FALSE;
        PhInitializeEmptyStringRef(&getCellTooltip.Text);
        getCellTooltip.Font = Context->Font;

        if (PhTnpGetCellParts(Context, hitTest.Node->Index, hitTest.Column, TN_MEASURE_TEXT, &parts) &&
            (parts.Flags & TN_PART_TEXT))
        {
            if (parts.TextRect.right > parts.ContentRect.right)
            {
                getCellTooltip.Unfolding = TRUE;
                getCellTooltip.Text = parts.Text;
                getCellTooltip.Font = parts.Font; // try to use the same font as the cell

                Context->TooltipUnfolding = TRUE;
                Context->TooltipRect = parts.TextRect;
            }
        }

        Context->Callback(Context->Handle, TreeNewGetCellTooltip, &getCellTooltip, NULL, Context->CallbackContext);

        if (getCellTooltip.Text.Buffer && getCellTooltip.Text.Length != 0)
            PhSwapReference(&Context->TooltipText, PhCreateStringEx(getCellTooltip.Text.Buffer, getCellTooltip.Text.Length));
        else
            PhSwapReference(&Context->TooltipText, NULL);

        Context->NewTooltipFont = getCellTooltip.Font;
    }

    if (Context->TooltipText)
        *Text = Context->TooltipText->Buffer;
}

BOOLEAN PhTnpPrepareTooltipShow(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    RECT rect;

    if (Context->TooltipFont != Context->NewTooltipFont)
    {
        Context->TooltipFont = Context->NewTooltipFont;
        SendMessage(Context->TooltipsHandle, WM_SETFONT, (WPARAM)Context->TooltipFont, FALSE);
    }

    if (!Context->TooltipUnfolding)
        return FALSE;

    rect = Context->TooltipRect;
    SendMessage(Context->TooltipsHandle, TTM_ADJUSTRECT, TRUE, (LPARAM)&rect);
    MapWindowPoints(Context->Handle, NULL, (POINT *)&rect, 2);
    SetWindowPos(
        Context->TooltipsHandle,
        HWND_TOP,
        rect.left,
        rect.top,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_HIDEWINDOW
        );

    return TRUE;
}

VOID PhTnpPrepareTooltipPop(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    Context->TooltipIndex = -1;
    Context->TooltipId = -1;
}

VOID PhTnpPopTooltip(
    __in PPH_TREENEW_CONTEXT Context
    )
{
    if (Context->TooltipsHandle)
    {
        SendMessage(Context->TooltipsHandle, TTM_POP, 0, 0);
        Context->TooltipIndex = -1;
        Context->TooltipId = -1;
    }
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
