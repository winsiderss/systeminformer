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
} PHP_GRAPH_CONTEXT, *PPHP_GRAPH_CONTEXT;

LRESULT CALLBACK PhpGraphWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

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
    ULONG flags;
    ULONG step;
    RECT rect;
    POINT points[4];
    HPEN nullPen;
    HPEN dcPen;
    HBRUSH dcBrush;

    width = DrawInfo->Width;
    height = DrawInfo->Height;
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

    // Draw the data.

    if (DrawInfo->LineData1)
    {
        ULONG x = width - DrawInfo->Step;
        ULONG index = 0;

        SelectObject(hdc, dcBrush);

        while (index < DrawInfo->LineDataCount - 1)
        {
            FLOAT f0; // 0..1
            FLOAT f1;
            ULONG h0; // height..0
            ULONG h1;

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
            SelectObject(hdc, nullPen);
            SetDCBrushColor(hdc, DrawInfo->LineBackColor1);
            Polygon(hdc, points, 4);

            // Draw the line.

            SelectObject(hdc, dcPen);
            SetDCPenColor(hdc, DrawInfo->LineColor1);
            Polyline(hdc, points, 2);

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

                    SelectObject(hdc, nullPen);
                    SetDCBrushColor(hdc, DrawInfo->LineBackColor2);
                    Polygon(hdc, points, 4);

                    SelectObject(hdc, dcPen);
                    SetDCPenColor(hdc, DrawInfo->LineColor2);
                    Polyline(hdc, points, 2);
                }
                else
                {
                    // Fix the bottom points so we don't fill in the line 1 area.
                    points[2].y = points[1].y; // p2.y = h0(line1)
                    points[3].y = points[0].y; // p3.y = h1(line1)
                    points[0].y = h1;
                    points[1].y = h0;

                    SelectObject(hdc, nullPen);
                    SetDCBrushColor(hdc, DrawInfo->LineBackColor2);
                    Polygon(hdc, points, 4);

                    SelectObject(hdc, dcPen);
                    SetDCPenColor(hdc, DrawInfo->LineColor2);
                    Polyline(hdc, points, 2);
                }
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

        SetDCPenColor(hdc, DrawInfo->GridColor);
        SelectObject(hdc, dcPen);

        // Draw the vertical lines.

        points[0].y = 0;
        points[1].y = height;

        for (i = 0; i <= x; i++)
        {
            pos = width - (i * DrawInfo->GridWidth + DrawInfo->GridStart);
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
}

HWND PhCreateGraphControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    )
{
    return CreateWindow(
        PH_GRAPH_CLASSNAME,
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
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
    context->DrawInfo.GridColor = RGB(0x00, 0xaa, 0x00);
    context->DrawInfo.GridWidth = 12;
    context->DrawInfo.GridHeight = 12;
    context->DrawInfo.GridStart = 0;

    context->BufferedContext = NULL;

    *Context = context;
}

VOID PhpDeleteGraphContext(
    __inout PPHP_GRAPH_CONTEXT Context
    )
{
    PhFree(Context);
}

static VOID PhpCreateBufferedContext(
    __in PPHP_GRAPH_CONTEXT Context
    )
{
    HDC hdc;

    if (Context->BufferedContext)
    {
        // The original bitmap must be selected back into the context, otherwise 
        // the bitmap can't be deleted.
        SelectObject(Context->BufferedContext, Context->BufferedOldBitmap);
        DeleteObject(Context->BufferedBitmap);
        DeleteDC(Context->BufferedContext);
    }

    GetClientRect(Context->Handle, &Context->BufferedContextRect);

    hdc = GetDC(Context->Handle);
    Context->BufferedContext = CreateCompatibleDC(hdc);
    Context->BufferedBitmap = CreateCompatibleBitmap(
        hdc,
        Context->BufferedContextRect.right,
        Context->BufferedContextRect.bottom
        );
    Context->BufferedOldBitmap = SelectObject(Context->BufferedContext, Context->BufferedBitmap);
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
            PhpDeleteGraphContext(context);
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
            PhDrawGraph(context->BufferedContext, &context->DrawInfo);
        }
        return TRUE;
    case GCM_MOVEGRID:
        {
            LONG increment = (LONG)wParam;

            context->DrawInfo.GridStart += increment;
        }
        return TRUE;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
