/*
 * Process Hacker - 
 *   graph control
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

#define GRAPH_PRIVATE
#include <phgui.h>
#include <graph.h>

typedef struct _PHP_GRAPH_CONTEXT
{
    HWND Handle;
    PH_GRAPH_DRAW_INFO DrawInfo;

    HDC BufferedContext;
    HBITMAP BufferedOldBitmap;
    HBITMAP BufferedBitmap;
    RECT BufferedContextRect;

    HWND TooltipHandle;
    BOOLEAN TooltipVisible;
    MONITORINFO MonitorInfo;
} PHP_GRAPH_CONTEXT, *PPHP_GRAPH_CONTEXT;

LRESULT CALLBACK PhpGraphWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

RECT PhNormalGraphTextMargin = { 5, 5, 5, 5 };
RECT PhNormalGraphTextPadding = { 3, 3, 3, 3 };

BOOLEAN PhGraphControlInitialization()
{
    WNDCLASSEX c = { sizeof(c) };

    c.style = 0;
    c.lpfnWndProc = PhpGraphWndProc;
    c.cbClsExtra = 0;
    c.cbWndExtra = sizeof(PVOID);
    c.hInstance = PhInstanceHandle;
    c.hIcon = NULL;
    c.hCursor = LoadCursor(NULL, IDC_ARROW);
    c.hbrBackground = NULL;
    c.lpszMenuName = NULL;
    c.lpszClassName = PH_GRAPH_CLASSNAME;
    c.hIconSm = NULL;

    if (!RegisterClassEx(&c))
        return FALSE;

    return TRUE;
}

VOID PhDrawGraph(
    __in HDC hdc,
    __in PPH_GRAPH_DRAW_INFO DrawInfo
    )
{
    ULONG width;
    ULONG height;
    ULONG height1;
    ULONG flags;
    LONG step;
    RECT rect;
    POINT points[4];
    HPEN nullPen;
    HPEN dcPen;
    HBRUSH dcBrush;
    PPH_LIST lineList1 = NULL;
    PPH_LIST lineList2 = NULL;

    width = DrawInfo->Width;
    height = DrawInfo->Height;
    height1 = DrawInfo->Height - 1;
    flags = DrawInfo->Flags;
    step = DrawInfo->Step;

    nullPen = GetStockObject(NULL_PEN);
    dcPen = GetStockObject(DC_PEN);
    dcBrush = GetStockObject(DC_BRUSH);

    // Fill in the background.
    rect.left = 0;
    rect.top = 0;
    rect.right = width;
    rect.bottom = height;
    SetDCBrushColor(hdc, DrawInfo->BackColor);
    FillRect(hdc, &rect, dcBrush);

    // Draw the data (this section only fills in the background).

    if (DrawInfo->LineData1 && DrawInfo->LineDataCount >= 2)
    {
        LONG x = width - step;
        ULONG index = 0;
        BOOLEAN willBreak = FALSE;

        SelectObject(hdc, dcBrush);
        SelectObject(hdc, nullPen);

        lineList1 = PhCreateList(DrawInfo->LineDataCount);

        if (DrawInfo->LineData2)
            lineList2 = PhCreateList(DrawInfo->LineDataCount);

        while (index < DrawInfo->LineDataCount - 1)
        {
            FLOAT f0; // 0..1
            FLOAT f1;
            ULONG h0; // height..0
            ULONG h1;

            if (x < 0 || index == DrawInfo->LineDataCount - 2)
                willBreak = TRUE;

            // Draw line 1.

            f0 = DrawInfo->LineData1[index];
            f1 = DrawInfo->LineData1[index + 1];
            h0 = (ULONG)((1 - f0) * height);
            h1 = (ULONG)((1 - f1) * height);

            // top-left, top-right, bottom-right, bottom-left
            points[0].x = x;
            points[0].y = h1;
            points[1].x = x + step;
            points[1].y = h0;
            points[2].x = x + step;
            points[2].y = height;
            points[3].x = x;
            points[3].y = height;

            // Fill in the area below the line.
            SetDCBrushColor(hdc, DrawInfo->LineBackColor1);
            Polygon(hdc, points, 4);

            // Add the line height values to the list for drawing later.
            if (h0 > height1) h0 = height1;
            if (h1 > height1) h1 = height1;
            PhAddListItem(lineList1, (PVOID)h0);
            if (willBreak) PhAddListItem(lineList1, (PVOID)h1);

            // Draw line 2 (either stacked or overlayed).
            if (DrawInfo->LineData2 && (flags & PH_GRAPH_USE_LINE_2))
            {
                if (flags & PH_GRAPH_OVERLAY_LINE_2)
                {
                    f0 = DrawInfo->LineData2[index];
                    f1 = DrawInfo->LineData2[index + 1];
                }
                else
                {
                    f0 += DrawInfo->LineData2[index];
                    f1 += DrawInfo->LineData2[index + 1];

                    if (f0 > 1) f0 = 1;
                    if (f1 > 1) f1 = 1;
                }

                h0 = (ULONG)((1 - f0) * height);
                h1 = (ULONG)((1 - f1) * height);

                if (flags & PH_GRAPH_OVERLAY_LINE_2)
                {
                    points[0].y = h1;
                    points[1].y = h0;
                }
                else
                {
                    // Fix the bottom points so we don't fill in the line 1 area.
                    points[2].y = points[1].y; // p2.y = h0(line1)
                    points[3].y = points[0].y; // p3.y = h1(line1)
                    points[0].y = h1;
                    points[1].y = h0;
                }

                SetDCBrushColor(hdc, DrawInfo->LineBackColor2);
                Polygon(hdc, points, 4);

                if (h0 > height1) h0 = height1;
                if (h1 > height1) h1 = height1;
                PhAddListItem(lineList2, (PVOID)h0);
                if (willBreak) PhAddListItem(lineList2, (PVOID)h1);
            }

            if (x < 0)
                break;

            x -= step;
            index++;
        }
    }

    // Draw the grid.
    if (flags & PH_GRAPH_USE_GRID)
    {
        ULONG x = width / DrawInfo->GridWidth;
        ULONG y = height / DrawInfo->GridHeight;
        ULONG i;
        ULONG pos;
        ULONG gridStart;

        SetDCPenColor(hdc, DrawInfo->GridColor);
        SelectObject(hdc, dcPen);

        // Draw the vertical lines.

        points[0].y = 0;
        points[1].y = height;
        gridStart = (DrawInfo->GridStart * DrawInfo->Step) % DrawInfo->GridWidth;

        for (i = 0; i <= x; i++)
        {
            pos = width - (i * DrawInfo->GridWidth + gridStart) - 1;
            points[0].x = pos;
            points[1].x = pos;
            Polyline(hdc, points, 2);
        }

        // Draw the horizontal lines.

        points[0].x = 0;
        points[1].x = width;

        for (i = 0; i <= y; i++)
        {
            pos = i * DrawInfo->GridHeight - 1;
            points[0].y = pos;
            points[1].y = pos;
            Polyline(hdc, points, 2);
        }
    }

    // Draw the data (this section draws the lines).
    // This is done to get a clearer graph.
    if (lineList1)
    {
        LONG x = width - step;
        ULONG index;
        ULONG previousHeight1;
        ULONG previousHeight2;

        previousHeight1 = (ULONG)lineList1->Items[0];
        index = 1;

        if (lineList2)
            previousHeight2 = (ULONG)lineList2->Items[0];

        while (index < lineList1->Count)
        {
            points[0].x = x;
            points[1].x = x + step;

            // Draw line 2 first so it doesn't draw over line 1.
            if (lineList2)
            {
                points[0].y = (ULONG)lineList2->Items[index];
                points[1].y = previousHeight2;

                SelectObject(hdc, dcPen);
                SetDCPenColor(hdc, DrawInfo->LineColor2);
                Polyline(hdc, points, 2);

                previousHeight2 = points[0].y;

                points[0].y = (ULONG)lineList1->Items[index];
                points[1].y = previousHeight1;

                SelectObject(hdc, dcPen);
                SetDCPenColor(hdc, DrawInfo->LineColor1);
                Polyline(hdc, points, 2);

                previousHeight1 = points[0].y;
            }
            else
            {
                points[0].x = x;
                points[0].y = (ULONG)lineList1->Items[index];
                points[1].x = x + step;
                points[1].y = previousHeight1;

                SelectObject(hdc, dcPen);
                SetDCPenColor(hdc, DrawInfo->LineColor1);
                Polyline(hdc, points, 2);

                previousHeight1 = points[0].y;
            }

            if (x < 0)
                break;

            x -= step;
            index++;
        }

        PhDereferenceObject(lineList1);
        if (lineList2) PhDereferenceObject(lineList2);
    }

    // Draw the text.
    if (DrawInfo->Text.Buffer)
    {
        // Fill in the text box.
        SetDCBrushColor(hdc, DrawInfo->TextBoxColor);
        FillRect(hdc, &DrawInfo->TextBoxRect, GetStockObject(DC_BRUSH));

        // Draw the text.
        SetTextColor(hdc, DrawInfo->TextColor);
        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc, DrawInfo->Text.Buffer, DrawInfo->Text.Length / 2, &DrawInfo->TextRect, DT_NOCLIP);
    }
}

VOID PhSetGraphText(
    __in HDC hdc,
    __inout PPH_GRAPH_DRAW_INFO DrawInfo,
    __in PPH_STRINGREF Text,
    __in PRECT Margin,
    __in PRECT Padding,
    __in ULONG Align
    )
{
    SIZE textSize;
    PH_RECTANGLE boxRectangle;
    PH_RECTANGLE textRectangle;

    DrawInfo->Text = *Text;
    GetTextExtentPoint32(hdc, Text->Buffer, Text->Length / 2, &textSize);

    // Calculate the box rectangle.

    boxRectangle.Width = textSize.cx + Padding->left + Padding->right;
    boxRectangle.Height = textSize.cy + Padding->top + Padding->bottom;

    if (Align & PH_ALIGN_LEFT)
        boxRectangle.Left = Margin->left;
    else if (Align & PH_ALIGN_RIGHT)
        boxRectangle.Left = DrawInfo->Width - boxRectangle.Width - Margin->right;
    else
        boxRectangle.Left = (DrawInfo->Width - boxRectangle.Width) / 2;

    if (Align & PH_ALIGN_TOP)
        boxRectangle.Top = Margin->top;
    else if (Align & PH_ALIGN_BOTTOM)
        boxRectangle.Top = DrawInfo->Height - boxRectangle.Height - Margin->bottom;
    else
        boxRectangle.Top = (DrawInfo->Height - boxRectangle.Height) / 2;

    // Calculate the text rectangle.

    textRectangle.Left = boxRectangle.Left + Padding->left;
    textRectangle.Top = boxRectangle.Top + Padding->top;
    textRectangle.Width = textSize.cx;
    textRectangle.Height = textSize.cy;

    // Save the rectangles.
    DrawInfo->TextRect = PhRectangleToRect(textRectangle);
    DrawInfo->TextBoxRect = PhRectangleToRect(boxRectangle);
}

HWND PhCreateGraphControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    )
{
    return CreateWindow(
        PH_GRAPH_CLASSNAME,
        L"",
        WS_CHILD | WS_CLIPSIBLINGS,
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

VOID PhpCreateGraphContext(
    __out PPHP_GRAPH_CONTEXT *Context
    )
{
    PPHP_GRAPH_CONTEXT context;

    context = PhAllocate(sizeof(PHP_GRAPH_CONTEXT));

    memset(&context->DrawInfo, 0, sizeof(PH_GRAPH_DRAW_INFO));
    context->DrawInfo.Width = 3;
    context->DrawInfo.Height = 3;
    context->DrawInfo.Flags = PH_GRAPH_USE_GRID;
    context->DrawInfo.Step = 2;
    context->DrawInfo.BackColor = RGB(0x00, 0x00, 0x00);
    context->DrawInfo.LineDataCount = 0;
    context->DrawInfo.LineData1 = NULL;
    context->DrawInfo.LineData2 = NULL;
    context->DrawInfo.LineColor1 = RGB(0x00, 0xff, 0x00);
    context->DrawInfo.LineColor2 = RGB(0xff, 0x00, 0x00);
    context->DrawInfo.LineBackColor1 = RGB(0x00, 0x77, 0x00);
    context->DrawInfo.LineBackColor2 = RGB(0x77, 0x00, 0x00);
    context->DrawInfo.GridColor = RGB(0x00, 0x99, 0x00);
    context->DrawInfo.GridWidth = 12;
    context->DrawInfo.GridHeight = 12;
    context->DrawInfo.GridStart = 0;
    context->DrawInfo.TextColor = RGB(0x00, 0xff, 0x00);
    context->DrawInfo.TextBoxColor = RGB(0x00, 0x22, 0x00);

    context->BufferedContext = NULL;

    context->TooltipHandle = NULL;
    context->TooltipVisible = FALSE;
    memset(&context->MonitorInfo, 0, sizeof(MONITORINFO));
    context->MonitorInfo.cbSize = sizeof(MONITORINFO);

    *Context = context;
}

VOID PhpFreeGraphContext(
    __inout __post_invalid PPHP_GRAPH_CONTEXT Context
    )
{
    PhFree(Context);
}

static VOID PhpDeleteBufferedContext(
    __in PPHP_GRAPH_CONTEXT Context
    )
{
    if (Context->BufferedContext)
    {
        // The original bitmap must be selected back into the context, otherwise 
        // the bitmap can't be deleted.
        SelectObject(Context->BufferedContext, Context->BufferedOldBitmap);
        DeleteObject(Context->BufferedBitmap);
        DeleteDC(Context->BufferedContext);
    }
}

static VOID PhpCreateBufferedContext(
    __in PPHP_GRAPH_CONTEXT Context
    )
{
    HDC hdc;

    PhpDeleteBufferedContext(Context);

    GetClientRect(Context->Handle, &Context->BufferedContextRect);

    hdc = GetDC(Context->Handle);
    Context->BufferedContext = CreateCompatibleDC(hdc);
    Context->BufferedBitmap = CreateCompatibleBitmap(
        hdc,
        Context->BufferedContextRect.right,
        Context->BufferedContextRect.bottom
        );
    ReleaseDC(Context->Handle, hdc);
    Context->BufferedOldBitmap = SelectObject(Context->BufferedContext, Context->BufferedBitmap);
}

static VOID PhpUpdateTooltip(
    __in PPHP_GRAPH_CONTEXT Context
    )
{
    POINT point;
    RECT windowRect;
    HWND hwnd;
    TOOLINFO toolInfo = { sizeof(toolInfo) };

    GetCursorPos(&point);
    GetWindowRect(Context->Handle, &windowRect);

    if (
        point.x < windowRect.left || point.x >= windowRect.right ||
        point.y < windowRect.top || point.y >= windowRect.bottom
        )
        return;

    hwnd = WindowFromPoint(point);

    if (hwnd != Context->Handle)
        return;

    toolInfo.hwnd = Context->Handle;
    toolInfo.uId = 1;

    if (!Context->TooltipVisible)
    {
        TRACKMOUSEEVENT trackMouseEvent = { sizeof(trackMouseEvent) };

        // this must go *before* SendMessage; our TTN_GETDISPINFO might reset TooltipVisible
        Context->TooltipVisible = TRUE;
        SendMessage(Context->TooltipHandle, TTM_TRACKACTIVATE, TRUE, (LPARAM)&toolInfo);

        trackMouseEvent.dwFlags = TME_LEAVE;
        trackMouseEvent.hwndTrack = Context->Handle;
        TrackMouseEvent(&trackMouseEvent);

        GetMonitorInfo(
            MonitorFromWindow(Context->TooltipHandle, MONITOR_DEFAULTTONEAREST),
            &Context->MonitorInfo
            );
    }

    // Add an offset to fix the case where the user moves the mouse to the bottom-right.
    point.x += 12;
    point.y += 12;

    SendMessage(Context->TooltipHandle, TTM_TRACKPOSITION, 0, MAKELONG(point.x, point.y));
}

static LRESULT CALLBACK PhpTooltipWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    oldWndProc = (WNDPROC)GetProp(hwnd, L"OldWndProc");

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

            RemoveProp(hwnd, L"OldWndProc");
            RemoveProp(hwnd, L"Context");
        }
        break;
    case WM_MOVE:
        {
            PPHP_GRAPH_CONTEXT context;
            TOOLINFO toolInfo = { sizeof(toolInfo) };
            RECT windowRect;
            BOOLEAN needsMove;

            if (CallWindowProc(oldWndProc, hwnd, TTM_GETCURRENTTOOL, 0, (LPARAM)&toolInfo))
            {
                context = (PPHP_GRAPH_CONTEXT)GetProp(hwnd, L"Context");

                GetWindowRect(hwnd, &windowRect);
                needsMove = FALSE;

                // Make sure the tooltip isn't off-screen.
                if (windowRect.right > context->MonitorInfo.rcWork.right)
                {
                    windowRect.left = context->MonitorInfo.rcWork.right - (windowRect.right - windowRect.left);
                    needsMove = TRUE;
                }

                if (windowRect.bottom >context->MonitorInfo.rcWork.bottom)
                {
                    windowRect.top = context->MonitorInfo.rcWork.bottom - (windowRect.bottom - windowRect.left);
                    needsMove = TRUE;
                }

                if (needsMove)
                {
                    SetWindowPos(hwnd, HWND_TOPMOST, windowRect.left, windowRect.top,
                        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
                        SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);
                }
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpGraphWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PPHP_GRAPH_CONTEXT context;

    context = (PPHP_GRAPH_CONTEXT)GetWindowLongPtr(hwnd, 0);

    if (uMsg == WM_CREATE)
    {
        PhpCreateGraphContext(&context);
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)context);
    }

    if (!context)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_CREATE:
        {
            context->Handle = hwnd;
        }
        break;
    case WM_DESTROY:
        {
            if (context->TooltipHandle)
                DestroyWindow(context->TooltipHandle);

            PhpDeleteBufferedContext(context);
            PhpFreeGraphContext(context);
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)NULL);
        }
        break;
    case WM_SIZE:
        {
            // Force a re-create of the buffered context.
            PhpCreateBufferedContext(context);
            SendMessage(hwnd, GCM_DRAW, 0, 0);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            RECT clientRect;
            HDC hdc;

            if (hdc = BeginPaint(hwnd, &paintStruct))
            {
                GetClientRect(hwnd, &clientRect);

                if (context->BufferedContext)
                {
                    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, context->BufferedContext, 0, 0, SRCCOPY);
                }

                EndPaint(hwnd, &paintStruct);
            }
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case TTN_GETDISPINFO:
                {
                    LPNMTTDISPINFO dispInfo = (LPNMTTDISPINFO)header;
                    POINT point;
                    RECT clientRect;
                    PH_GRAPH_GETTOOLTIPTEXT getTooltipText;

                    GetCursorPos(&point);
                    MapWindowPoints(NULL, hwnd, &point, 1);
                    GetClientRect(hwnd, &clientRect);

                    getTooltipText.Header.hwndFrom = hwnd;
                    getTooltipText.Header.code = GCN_GETTOOLTIPTEXT;
                    getTooltipText.Index = (clientRect.right - point.x) / context->DrawInfo.Step;
                    getTooltipText.TotalCount = context->DrawInfo.LineDataCount;
                    getTooltipText.Text.Buffer = NULL;
                    getTooltipText.Text.Length = 0;

                    SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM)&getTooltipText);

                    if (getTooltipText.Text.Buffer)
                    {
                        dispInfo->lpszText = getTooltipText.Text.Buffer;
                    }
                    else
                    {
                        if (dispInfo->lpszText[0] == 0)
                        {
                            // No text, so the tooltip will be closed. Update our boolean.
                            context->TooltipVisible = FALSE;
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (context->TooltipHandle)
            {
                PhpUpdateTooltip(context);
            }
        }
        break;
    case WM_MOUSELEAVE:
        {
            if (context->TooltipHandle)
            {
                TOOLINFO toolInfo = { sizeof(toolInfo) };

                toolInfo.hwnd = hwnd;
                toolInfo.uId = 1;

                SendMessage(context->TooltipHandle, TTM_TRACKACTIVATE, FALSE, (LPARAM)&toolInfo);
                context->TooltipVisible = FALSE;
            }
        }
        break;
    case GCM_GETDRAWINFO:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)lParam;

            memcpy(drawInfo, &context->DrawInfo, sizeof(PH_GRAPH_DRAW_INFO));
        }
        return TRUE;
    case GCM_SETDRAWINFO:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)lParam;
            ULONG width;
            ULONG height;

            width = context->DrawInfo.Width;
            height = context->DrawInfo.Height;
            memcpy(&context->DrawInfo, drawInfo, sizeof(PH_GRAPH_DRAW_INFO));
            context->DrawInfo.Width = width;
            context->DrawInfo.Height = height;
        }
        return TRUE;
    case GCM_DRAW:
        {
            if (!context->BufferedContext)
                PhpCreateBufferedContext(context);

            context->DrawInfo.Width = context->BufferedContextRect.right;
            context->DrawInfo.Height = context->BufferedContextRect.bottom;

            {
                PH_GRAPH_GETDRAWINFO getDrawInfo;

                getDrawInfo.Header.hwndFrom = hwnd;
                getDrawInfo.Header.code = GCN_GETDRAWINFO;
                getDrawInfo.DrawInfo = &context->DrawInfo;

                SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM)&getDrawInfo);
            }

            PhDrawGraph(context->BufferedContext, &context->DrawInfo);
        }
        return TRUE;
    case GCM_MOVEGRID:
        {
            LONG increment = (LONG)wParam;

            context->DrawInfo.GridStart += increment;
        }
        return TRUE;
    case GCM_GETBUFFEREDCONTEXT:
        return (LRESULT)context->BufferedContext;
    case GCM_SETTOOLTIP:
        {
            if (wParam)
            {
                TOOLINFO toolInfo = { sizeof(toolInfo) };
                WNDPROC oldWndProc;

                context->TooltipHandle = CreateWindow(
                    TOOLTIPS_CLASS,
                    L"",
                    WS_POPUP | TTS_NOPREFIX,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    NULL,
                    NULL,
                    PhInstanceHandle,
                    NULL
                    );
                oldWndProc = (WNDPROC)GetWindowLongPtr(context->TooltipHandle, GWLP_WNDPROC);
                SetProp(context->TooltipHandle, L"OldWndProc", (HANDLE)oldWndProc);
                SetProp(context->TooltipHandle, L"Context", (HANDLE)context);
                SetWindowLongPtr(context->TooltipHandle, GWLP_WNDPROC, (LONG_PTR)PhpTooltipWndProc);

                SetWindowPos(context->TooltipHandle, HWND_TOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                toolInfo.uFlags = TTF_ABSOLUTE | TTF_TRACK;
                toolInfo.hwnd = hwnd;
                toolInfo.uId = 1;
                toolInfo.lpszText = LPSTR_TEXTCALLBACK;
                SendMessage(context->TooltipHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
                SendMessage(context->TooltipHandle, TTM_SETMAXTIPWIDTH, 0, MAXINT); // allow newlines (-1 doesn't work)
            }
            else
            {
                DestroyWindow(context->TooltipHandle);
                context->TooltipHandle = NULL;
            }
        }
        return TRUE;
    case GCM_UPDATETOOLTIP:
        {
            if (!context->TooltipHandle)
                return FALSE;

            if (context->TooltipVisible)
            {
                TOOLINFO toolInfo = { sizeof(toolInfo) };

                toolInfo.hwnd = hwnd;
                toolInfo.uId = 1;
                toolInfo.lpszText = LPSTR_TEXTCALLBACK;

                SendMessage(context->TooltipHandle, TTM_UPDATETIPTEXT, 0, (LPARAM)&toolInfo);
            }
            else
            {
                PhpUpdateTooltip(context);
            }
        }
        return TRUE;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

VOID PhInitializeGraphState(
    __out PPH_GRAPH_STATE State
    )
{
    State->AllocatedCount = 0;
    State->Data1 = NULL;
    State->Data2 = NULL;
    State->Valid = FALSE;
    State->Text = NULL;
    State->TooltipText = NULL;
    State->TooltipIndex = -1;
}

VOID PhDeleteGraphState(
    __inout PPH_GRAPH_STATE State
    )
{
    if (State->Data1) PhFree(State->Data1);
    if (State->Data2) PhFree(State->Data2);
    if (State->Text) PhDereferenceObject(State->Text);
    if (State->TooltipText) PhDereferenceObject(State->TooltipText);
}

VOID PhGraphStateGetDrawInfo(
    __inout PPH_GRAPH_STATE State,
    __in PPH_GRAPH_GETDRAWINFO GetDrawInfo,
    __in ULONG DataCount
    )
{
    PPH_GRAPH_DRAW_INFO drawInfo;

    drawInfo = GetDrawInfo->DrawInfo;

    drawInfo->LineDataCount = min(DataCount, PH_GRAPH_DATA_COUNT(drawInfo->Width, drawInfo->Step));

    // Do we need to allocate or re-allocate the data buffers?
    if (State->AllocatedCount < drawInfo->LineDataCount)
    {
        if (State->Data1)
            PhFree(State->Data1);
        if ((drawInfo->Flags & PH_GRAPH_USE_LINE_2) && State->Data2)
            PhFree(State->Data2);

        State->AllocatedCount *= 2;

        if (State->AllocatedCount < drawInfo->LineDataCount)
            State->AllocatedCount = drawInfo->LineDataCount;

        State->Data1 = PhAllocate(State->AllocatedCount * sizeof(FLOAT));
        drawInfo->LineData1 = State->Data1;

        if (drawInfo->Flags & PH_GRAPH_USE_LINE_2)
        {
            State->Data2 = PhAllocate(State->AllocatedCount * sizeof(FLOAT));
            drawInfo->LineData2 = State->Data2;
        }

        State->Valid = FALSE;
    }
}
