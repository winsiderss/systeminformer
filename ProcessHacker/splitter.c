/*
 * Process Hacker -
 *   Splitter control
 *
 * Copyright (C) 2017 dmex
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

#include <phapp.h>
#include <splitter.h>
#include <settings.h>
#include <windowsx.h>

static INT nSplitterPos = 250;
static INT SplitterOffset = -4;

VOID DrawXorBar(HDC hdc, INT x1, INT y1, INT width, INT height);

PPH_HSPLITTER_CONTEXT PhInitializeHSplitter(
    _In_ HWND Parent,
    _In_ HWND TopChild,
    _In_ HWND BottomChild
    )
{
    PPH_HSPLITTER_CONTEXT context;

    context = PhAllocate(sizeof(PH_HSPLITTER_CONTEXT));
    memset(context, 0, sizeof(PH_HSPLITTER_CONTEXT));

    PhInitializeLayoutManager(&context->LayoutManager, Parent);
    context->Topitem = PhAddLayoutItem(&context->LayoutManager, TopChild, NULL, PH_ANCHOR_ALL);
    context->Bottomitem = PhAddLayoutItem(&context->LayoutManager, BottomChild, NULL, PH_ANCHOR_ALL);

    return context;
}

VOID PhDeleteHSplitter(
    _Inout_ PPH_HSPLITTER_CONTEXT Context
    )
{
    PhDeleteLayoutManager(&Context->LayoutManager);
}

VOID PhHSplitterHandleWmSize(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ INT Width,
    _In_ INT Height
    )
{
    // HACK: Use the PH layout manager as the 'splitter' control by abusing layout margins.
#define SPLITTER_PADDING 6
    // Set the bottom margin of the top control.
    Context->Topitem->Margin.bottom = Height - nSplitterPos - SPLITTER_PADDING;
    // Set the top margin of the bottom control.
    Context->Bottomitem->Margin.top = nSplitterPos + SPLITTER_PADDING * 2;

    PhLayoutManagerLayout(&Context->LayoutManager);
}

VOID PhHSplitterHandleLButtonDown(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ HWND hwnd,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    POINT pt;
    HDC hdc;
    RECT rect;

    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    GetWindowRect(hwnd, &rect);
    ClientToScreen(hwnd, &pt);

    pt.x -= rect.left;
    pt.y -= rect.top;

    // Adjust the coordinates (start from 0,0).
    OffsetRect(&rect, -rect.left, -rect.top);

    if (pt.y < 0) 
        pt.y = 0;
    if (pt.y > rect.bottom - 4)
        pt.y = rect.bottom - 4;

    Context->DragMode = TRUE;

    SetCapture(hwnd);

    hdc = GetWindowDC(hwnd);
    DrawXorBar(hdc, 1, pt.y - 2, rect.right - 2, 4);
    ReleaseDC(hwnd, hdc);

    SplitterOffset = pt.y;
}

VOID PhHSplitterHandleLButtonUp(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ HWND hwnd, 
    _In_ WPARAM wParam, 
    _In_ LPARAM lParam
    )
{
    HDC hdc;
    RECT rect;
    POINT pt;

    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);
    
    if (Context->DragMode == FALSE)
        return;

    GetWindowRect(hwnd, &rect);
    ClientToScreen(hwnd, &pt);

    pt.x -= rect.left;
    pt.y -= rect.top;

    // Adjust the coordinates (start from 0,0).
    OffsetRect(&rect, -rect.left, -rect.top);

    if (pt.y < 0) 
        pt.y = 0;
    if (pt.y > rect.bottom - 4)
        pt.y = rect.bottom - 4;

    hdc = GetWindowDC(hwnd);
    DrawXorBar(hdc, 1, SplitterOffset - 2, rect.right - 2, 4);
    ReleaseDC(hwnd, hdc);

    SplitterOffset = pt.y;

    Context->DragMode = FALSE;

    GetWindowRect(hwnd, &rect);

    pt.x += rect.left;
    pt.y += rect.top;

    ScreenToClient(hwnd, &pt);
    GetClientRect(hwnd, &rect);

    nSplitterPos = pt.y;

    // position the child controls
    PhHSplitterHandleWmSize(Context, rect.right, rect.bottom);

    ReleaseCapture();
}

VOID PhHSplitterHandleMouseMove(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ HWND hwnd,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    HDC hdc;
    RECT windowRect;
    POINT windowPoint;

    //if (Context->DragMode == FALSE)
    //    return;
    
    windowPoint.x = GET_X_LPARAM(lParam);
    windowPoint.y = GET_Y_LPARAM(lParam);

    GetWindowRect(hwnd, &windowRect);
    ClientToScreen(hwnd, &windowPoint);

    windowPoint.x -= windowRect.left;
    windowPoint.y -= windowRect.top;

    OffsetRect(&windowRect, -windowRect.left, -windowRect.top);

    if (windowPoint.y < 0)
        windowPoint.y = 0;
    if (windowPoint.y > windowRect.bottom - 4)
        windowPoint.y = windowRect.bottom - 4;

    if (windowPoint.y != SplitterOffset)
    {
        hdc = GetWindowDC(hwnd);

        if (wParam & MK_LBUTTON)
        {
            DrawXorBar(hdc, 1, SplitterOffset - 2, windowRect.right - 2, 4);
            DrawXorBar(hdc, 1, windowPoint.y - 2, windowRect.right - 2, 4);
        }
        else
        {
            TRACKMOUSEEVENT trackMouseEvent;

            trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
            trackMouseEvent.dwFlags = TME_LEAVE;
            trackMouseEvent.hwndTrack = hwnd;
            trackMouseEvent.dwHoverTime = 0;

            SetCursor(LoadCursor(NULL, IDC_SIZENS));

            TrackMouseEvent(&trackMouseEvent);
        }

        ReleaseDC(hwnd, hdc);
        SplitterOffset = windowPoint.y;
    }
}

VOID PhHSplitterHandleMouseLeave(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ HWND hwnd,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    // Reset the original cursor.
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

// http://www.catch22.net/tuts/splitter-windows
VOID DrawXorBar(HDC hdc, INT x1, INT y1, INT width, INT height)
{
    static WORD _dotPatternBmp[8] =
    {
        0x00aa, 0x0055, 0x00aa, 0x0055,
        0x00aa, 0x0055, 0x00aa, 0x0055
    };

    HBITMAP hbm;
    HBRUSH  hbr, hbrushOld;

    hbm = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
    hbr = CreatePatternBrush(hbm);

    SetBrushOrgEx(hdc, x1, y1, 0);
    hbrushOld = (HBRUSH)SelectObject(hdc, hbr);

    PatBlt(hdc, x1, y1, width, height, PATINVERT);

    SelectObject(hdc, hbrushOld);

    DeleteObject(hbr);
    DeleteObject(hbm);
}

