/*
 * Process Hacker -
 *   graph control
 *
 * Copyright (C) 2010-2011 wj32
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

#define _PH_GRAPH_PRIVATE
#include <phgui.h>
#include <graph.h>

#define COLORREF_TO_BITS(Color) (_byteswap_ulong(Color) >> 8)

typedef struct _PHP_GRAPH_CONTEXT
{
    HWND Handle;
    ULONG Style;
    ULONG_PTR Id;
    PH_GRAPH_DRAW_INFO DrawInfo;
    PH_GRAPH_OPTIONS Options;
    BOOLEAN NeedsUpdate;
    BOOLEAN NeedsDraw;

    HDC BufferedContext;
    HBITMAP BufferedOldBitmap;
    HBITMAP BufferedBitmap;
    PVOID BufferedBits;
    RECT BufferedContextRect;

    HDC FadeOutContext;
    HBITMAP FadeOutOldBitmap;
    HBITMAP FadeOutBitmap;
    PVOID FadeOutBits;
    RECT FadeOutContextRect;

    HWND TooltipHandle;
    WNDPROC TooltipOldWndProc;
    BOOLEAN TooltipVisible;
    HMONITOR ValidMonitor;
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

BOOLEAN PhGraphControlInitialization(
    VOID
    )
{
    WNDCLASSEX c = { sizeof(c) };

    c.style = CS_GLOBALCLASS | CS_DBLCLKS;
    c.lpfnWndProc = PhpGraphWndProc;
    c.cbClsExtra = 0;
    c.cbWndExtra = sizeof(PVOID);
    c.hInstance = PhLibImageBase;
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

/**
 * Draws a graph.
 *
 * \param hdc The DC to draw to.
 * \param DrawInfo A structure which contains graphing information.
 *
 * \remarks This function is extremely slow. Use PhDrawGraphDirect()
 * whenever possible.
 */
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
            PhAddItemList(lineList1, (PVOID)h0);
            if (willBreak) PhAddItemList(lineList1, (PVOID)h1);

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
                PhAddItemList(lineList2, (PVOID)h0);
                if (willBreak) PhAddItemList(lineList2, (PVOID)h1);
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

FORCEINLINE VOID PhpGetGraphPoint(
    __in PPH_GRAPH_DRAW_INFO DrawInfo,
    __in ULONG Index,
    __out PULONG H1,
    __out PULONG H2
    )
{
    if (Index < DrawInfo->LineDataCount)
    {
        FLOAT f1;
        FLOAT f2;

        f1 = DrawInfo->LineData1[Index];

        if (f1 < 0)
            f1 = 0;
        if (f1 > 1)
            f1 = 1;

        *H1 = (ULONG)(f1 * (DrawInfo->Height - 1));

        if (DrawInfo->Flags & PH_GRAPH_USE_LINE_2)
        {
            f2 = f1 + DrawInfo->LineData2[Index];

            if (f2 < 0)
                f2 = 0;
            if (f2 > 1)
                f2 = 1;

            *H2 = (ULONG)(f2 * (DrawInfo->Height - 1));
        }
    }
    else
    {
        *H1 = 0;
        *H2 = 0;
    }
}

/**
 * Draws a graph directly to memory.
 *
 * \param hdc The DC to draw to. This is only used when drawing text.
 * \param Bits The bits in a bitmap.
 * \param DrawInfo A structure which contains graphing information.
 *
 * \remarks The following information is fixed:
 * \li The graph is fixed to the origin (0, 0).
 * \li The total size of the bitmap is assumed to be \a Width and \a Height in \a DrawInfo.
 * \li \a Step is fixed at 2.
 * \li If \ref PH_GRAPH_USE_LINE_2 is specified in \a Flags, \ref PH_GRAPH_OVERLAY_LINE_2
 * is never used.
 */
VOID PhDrawGraphDirect(
    __in HDC hdc,
    __in PVOID Bits,
    __in PPH_GRAPH_DRAW_INFO DrawInfo
    )
{
    PULONG bits;
    LONG width;
    LONG height;
    LONG numberOfPixels;
    ULONG flags;
    LONG i;
    LONG x;

    BOOLEAN intermediate; // whether we are currently between two data positions
    ULONG dataIndex; // the data index of the current position
    ULONG h1_i; // the line 1 height value to the left of the current position
    ULONG h1_o; // the line 1 height value at the current position
    ULONG h2_i; // the line 1 + line 2 height value to the left of the current position
    ULONG h2_o; // the line 1 + line 2 height value at the current position
    ULONG h1; // current pixel
    ULONG h1_left; // current pixel
    ULONG h2; // current pixel
    ULONG h2_left; // current pixel

    LONG mid;
    LONG h1_low1;
    LONG h1_high1;
    LONG h1_low2;
    LONG h1_high2;
    LONG h2_low1;
    LONG h2_high1;
    LONG h2_low2;
    LONG h2_high2;
    LONG old_low2;
    LONG old_high2;

    ULONG lineColor1;
    ULONG lineBackColor1;
    ULONG lineColor2;
    ULONG lineBackColor2;
    ULONG gridYCounter;
    ULONG gridXIncrement;
    ULONG gridColor;

    bits = Bits;
    width = DrawInfo->Width;
    height = DrawInfo->Height;
    numberOfPixels = width * height;
    flags = DrawInfo->Flags;
    lineColor1 = COLORREF_TO_BITS(DrawInfo->LineColor1);
    lineBackColor1 = COLORREF_TO_BITS(DrawInfo->LineBackColor1);
    lineColor2 = COLORREF_TO_BITS(DrawInfo->LineColor2);
    lineBackColor2 = COLORREF_TO_BITS(DrawInfo->LineBackColor2);

    if (DrawInfo->BackColor == 0)
    {
        memset(bits, 0, numberOfPixels * 4);
    }
    else
    {
        PhxfFillMemoryUlong(bits, COLORREF_TO_BITS(DrawInfo->BackColor), numberOfPixels);
    }

    x = width - 1;
    intermediate = FALSE;
    dataIndex = 0;
    h1_low2 = MAXLONG;
    h1_high2 = 0;
    h2_low2 = MAXLONG;
    h2_high2 = 0;

    PhpGetGraphPoint(DrawInfo, 0, &h1_i, &h2_i);

    if (flags & PH_GRAPH_USE_GRID)
    {
        gridYCounter = DrawInfo->GridWidth - (DrawInfo->GridStart * DrawInfo->Step) % DrawInfo->GridWidth - 1;
        gridXIncrement = width * DrawInfo->GridHeight;
        gridColor = COLORREF_TO_BITS(DrawInfo->GridColor);
    }

    while (x >= 0)
    {
        // Calculate the height of the graph at this point.

        if (!intermediate)
        {
            h1_o = h1_i;
            h2_o = h2_i;

            // Pull in new data.
            dataIndex++;
            PhpGetGraphPoint(DrawInfo, dataIndex, &h1_i, &h2_i);

            h1 = h1_o;
            h1_left = (h1_i + h1_o) / 2;
            h2 = h2_o;
            h2_left = (h2_i + h2_o) / 2;
        }
        else
        {
            h1 = h1_left;
            h1_left = h1_i;
            h2 = h2_left;
            h2_left = h2_i;
        }

        // The graph is drawn right-to-left. There is one iteration of the loop per horizontal pixel.
        // There is a fixed step value of 2, so every other iteration is a mid-point (intermediate)
        // iteration with a height value of (left + right) / 2. In order to rasterize the outline,
        // effectively in each iteration half of the line is drawn at the current column and the other
        // half is drawn in the column to the left.

        // Rasterize the data outline.
        // h?_low2 to h?_high2 is the vertical line to the left of the current pixel.
        // h?_low1 to h?_high1 is the vertical line at the current pixel.
        // We merge (union) the old h?_low2 to h?_high2 line with the current line in each iteration.
        //
        // For example:
        //
        // X represents a data point. M represents the mid-point between two data points ("intermediate").
        // X, M and x are all part of the outline. # represents the background filled in during
        // the current loop iteration.
        //
        // slope > 0:                                     slope < 0:
        //
        //     X  < high1                                   X    < high2 (of next loop iteration)
        //     x                                            x
        //     x  < low1                                    x    < low2 (of next loop iteration)
        //    x#  < high2                                    x   < high1 (of next loop iteration)
        //    M#  < low2                                     M   < low1 (of next loop iteration)
        //    x#  < high1 (of next loop iteration)           x   < high2
        //    x#  < low1 (of next loop iteration)            x   < low2
        //   x #  < high2 (of next loop iteration)            x  < high1
        //   x #                                              x
        //   X #  < low2 (of next loop iteration)             X  < low1
        //     #                                              #
        //     ^                                              ^
        //    ^| current pixel                               ^| current pixel
        //    |                                              |
        //    | left of current pixel                        | left of current pixel
        //
        // In both examples above, the line low2-high2 will be merged with the line low1-high1 of the next
        // iteration.

        mid = ((h1_left + h1) / 2) * width;
        old_low2 = h1_low2;
        old_high2 = h1_high2;

        if (h1_left < h1) // slope > 0
        {
            h1_low2 = h1_left * width;
            h1_high2 = mid;
            h1_low1 = mid + width;
            h1_high1 = h1 * width;
        }
        else // slope < 0
        {
            h1_high2 = h1_left * width;
            h1_low2 = mid + width;
            h1_high1 = mid;
            h1_low1 = h1 * width;
        }

        // Merge the lines.
        if (h1_low1 > old_low2)
            h1_low1 = old_low2;
        if (h1_high1 < old_high2)
            h1_high1 = old_high2;

        // Fix up values for the current horizontal offset.
        h1_low1 += x;
        h1_high1 += x;

        if (flags & PH_GRAPH_USE_LINE_2)
        {
            mid = ((h2_left + h2) / 2) * width;
            old_low2 = h2_low2;
            old_high2 = h2_high2;

            if (h2_left < h2) // slope > 0
            {
                h2_low2 = h2_left * width;
                h2_high2 = mid;
                h2_low1 = mid + width;
                h2_high1 = h2 * width;
            }
            else // slope < 0
            {
                h2_high2 = h2_left * width;
                h2_low2 = mid + width;
                h2_high1 = mid;
                h2_low1 = h2 * width;
            }

            // Merge the lines.
            if (h2_low1 > old_low2)
                h2_low1 = old_low2;
            if (h2_high1 < old_high2)
                h2_high1 = old_high2;

            // Fix up values for the current horizontal offset.
            h2_low1 += x;
            h2_high1 += x;
        }

        // Fill in the background.

        if (flags & PH_GRAPH_USE_LINE_2)
        {
            for (i = h1_high1 + width; i < h2_low1; i += width)
            {
                bits[i] = lineBackColor2;
            }
        }

        for (i = x; i < h1_low1; i += width)
        {
            bits[i] = lineBackColor1;
        }

        // Draw the grid.

        if (flags & PH_GRAPH_USE_GRID)
        {
            // Draw the vertical grid line.
            if (gridYCounter == 0)
            {
                for (i = x; i < numberOfPixels; i += width)
                {
                    bits[i] = gridColor;
                }
            }

            gridYCounter++;

            if (gridYCounter == DrawInfo->GridWidth)
                gridYCounter = 0;

            // Draw the horizontal grid line.
            for (i = x + gridXIncrement; i < numberOfPixels; i += gridXIncrement)
            {
                bits[i] = gridColor;
            }
        }

        // Draw the outline (line 1 is allowed to paint over line 2).

        if (flags & PH_GRAPH_USE_LINE_2)
        {
            for (i = h2_low1; i <= h2_high1; i += width) // exclude pixel in the middle
            {
                bits[i] = lineColor2;
            }
        }

        for (i = h1_low1; i <= h1_high1; i += width)
        {
            bits[i] = lineColor1;
        }

        intermediate = !intermediate;
        x--;
    }

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

/**
 * Sets the text in a graphing information structure.
 *
 * \param hdc The DC to perform calculations from.
 * \param DrawInfo A structure which contains graphing information.
 * The structure is modified to contain the new text information.
 * \param Text The text.
 * \param Margin The margins of the text box from the edges of the
 * graph.
 * \param Padding The padding within the text box.
 * \param Align The alignment of the text box.
 */
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

VOID PhpCreateGraphContext(
    __out PPHP_GRAPH_CONTEXT *Context
    )
{
    PPHP_GRAPH_CONTEXT context;

    context = PhAllocate(sizeof(PHP_GRAPH_CONTEXT));
    memset(context, 0, sizeof(PHP_GRAPH_CONTEXT));

    context->DrawInfo.Width = 3;
    context->DrawInfo.Height = 3;
    context->DrawInfo.Flags = PH_GRAPH_USE_GRID;
    context->DrawInfo.Step = 2;
    context->DrawInfo.BackColor = RGB(0xef, 0xef, 0xef);
    context->DrawInfo.LineDataCount = 0;
    context->DrawInfo.LineData1 = NULL;
    context->DrawInfo.LineData2 = NULL;
    context->DrawInfo.LineColor1 = RGB(0x00, 0xff, 0x00);
    context->DrawInfo.LineColor2 = RGB(0xff, 0x00, 0x00);
    context->DrawInfo.LineBackColor1 = RGB(0x00, 0x77, 0x00);
    context->DrawInfo.LineBackColor2 = RGB(0x77, 0x00, 0x00);
    context->DrawInfo.GridColor = RGB(0xc7, 0xc7, 0xc7);
    context->DrawInfo.GridWidth = 20;
    context->DrawInfo.GridHeight = 40;
    context->DrawInfo.GridStart = 0;
    context->DrawInfo.TextColor = RGB(0x00, 0xff, 0x00);
    context->DrawInfo.TextBoxColor = RGB(0x00, 0x22, 0x00);

    context->Options.FadeOutBackColor = RGB(0xef, 0xef, 0xef);
    context->Options.FadeOutWidth = 100;

    context->MonitorInfo.cbSize = sizeof(MONITORINFO);

    *Context = context;
}

VOID PhpFreeGraphContext(
    __inout __post_invalid PPHP_GRAPH_CONTEXT Context
    )
{
    PhFree(Context);
}

static PWSTR PhpMakeGraphTooltipContextAtom(
    VOID
    )
{
    PH_DEFINE_MAKE_ATOM(L"PhLib_GraphTooltipContext");
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

        Context->BufferedContext = NULL;
        Context->BufferedBitmap = NULL;
        Context->BufferedBits = NULL;
    }
}

static VOID PhpCreateBufferedContext(
    __in PPHP_GRAPH_CONTEXT Context
    )
{
    HDC hdc;
    BITMAPINFOHEADER header;

    PhpDeleteBufferedContext(Context);

    GetClientRect(Context->Handle, &Context->BufferedContextRect);

    hdc = GetDC(Context->Handle);
    Context->BufferedContext = CreateCompatibleDC(hdc);

    memset(&header, 0, sizeof(BITMAPINFOHEADER));
    header.biSize = sizeof(BITMAPINFOHEADER);
    header.biWidth = Context->BufferedContextRect.right;
    header.biHeight = Context->BufferedContextRect.bottom;
    header.biPlanes = 1;
    header.biBitCount = 32;

    Context->BufferedBitmap = CreateDIBSection(hdc, (BITMAPINFO *)&header, DIB_RGB_COLORS, &Context->BufferedBits, NULL, 0);

    ReleaseDC(Context->Handle, hdc);
    Context->BufferedOldBitmap = SelectObject(Context->BufferedContext, Context->BufferedBitmap);
}

static VOID PhpDeleteFadeOutContext(
    __in PPHP_GRAPH_CONTEXT Context
    )
{
    if (Context->FadeOutContext)
    {
        SelectObject(Context->FadeOutContext, Context->FadeOutOldBitmap);
        DeleteObject(Context->FadeOutBitmap);
        DeleteDC(Context->FadeOutContext);

        Context->FadeOutContext = NULL;
        Context->FadeOutBitmap = NULL;
        Context->FadeOutBits = NULL;
    }
}

static VOID PhpCreateFadeOutContext(
    __in PPHP_GRAPH_CONTEXT Context
    )
{
    HDC hdc;
    BITMAPINFOHEADER header;
    ULONG i;
    ULONG j;
    ULONG height;
    COLORREF backColor;
    ULONG fadeOutWidth;
    FLOAT fadeOutWidthSquared;
    ULONG currentAlpha;
    ULONG currentColor;

    PhpDeleteFadeOutContext(Context);

    GetClientRect(Context->Handle, &Context->FadeOutContextRect);
    Context->FadeOutContextRect.right = Context->Options.FadeOutWidth;

    hdc = GetDC(Context->Handle);
    Context->FadeOutContext = CreateCompatibleDC(hdc);

    memset(&header, 0, sizeof(BITMAPINFOHEADER));
    header.biSize = sizeof(BITMAPINFOHEADER);
    header.biWidth = Context->FadeOutContextRect.right;
    header.biHeight = Context->FadeOutContextRect.bottom;
    header.biPlanes = 1;
    header.biBitCount = 32;

    Context->FadeOutBitmap = CreateDIBSection(hdc, (BITMAPINFO *)&header, DIB_RGB_COLORS, &Context->FadeOutBits, NULL, 0);

    ReleaseDC(Context->Handle, hdc);
    Context->FadeOutOldBitmap = SelectObject(Context->FadeOutContext, Context->FadeOutBitmap);

    if (!Context->FadeOutBits)
        return;

    height = Context->FadeOutContextRect.bottom;
    backColor = Context->Options.FadeOutBackColor;
    fadeOutWidth = Context->Options.FadeOutWidth;
    fadeOutWidthSquared = (FLOAT)fadeOutWidth * fadeOutWidth;

    for (i = 0; i < fadeOutWidth; i++)
    {
        currentAlpha = 255 - (ULONG)((FLOAT)(i * i) / fadeOutWidthSquared * 255);
        currentColor =
            ((backColor & 0xff) * currentAlpha / 255) +
            ((((backColor >> 8) & 0xff) * currentAlpha / 255) << 8) +
            ((((backColor >> 16) & 0xff) * currentAlpha / 255) << 16) +
            (currentAlpha << 24);

        for (j = i; j < height * fadeOutWidth; j += fadeOutWidth)
        {
            ((PULONG)Context->FadeOutBits)[j] = currentColor;
        }
    }
}

static VOID PhpUpdateTooltip(
    __in PPHP_GRAPH_CONTEXT Context
    )
{
    POINT point;
    RECT windowRect;
    HWND hwnd;
    TOOLINFO toolInfo = { sizeof(toolInfo) };
    HMONITOR monitor;

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

        trackMouseEvent.dwFlags = TME_LEAVE;
        trackMouseEvent.hwndTrack = Context->Handle;
        TrackMouseEvent(&trackMouseEvent);

        Context->ValidMonitor = NULL; // force a refresh of monitor info
    }

    // Update monitor information only if the tooltip has moved onto another monitor.

    monitor = MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST);

    if (Context->ValidMonitor != monitor)
    {
        GetMonitorInfo(monitor, &Context->MonitorInfo);
        Context->ValidMonitor = monitor;
    }

    // Add an offset to fix the case where the user moves the mouse to the bottom-right.
    point.x += 12;
    point.y += 12;

    SendMessage(Context->TooltipHandle, TTM_TRACKPOSITION, 0, MAKELONG(point.x, point.y));

    // HACK for buggy tooltip control
    if (!IsWindowVisible(Context->TooltipHandle))
        SendMessage(Context->TooltipHandle, TTM_TRACKACTIVATE, FALSE, (LPARAM)&toolInfo);

    SendMessage(Context->TooltipHandle, TTM_TRACKACTIVATE, TRUE, (LPARAM)&toolInfo);
}

static LRESULT CALLBACK PhpTooltipWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PPHP_GRAPH_CONTEXT context;
    WNDPROC oldWndProc;

    context = (PPHP_GRAPH_CONTEXT)GetProp(hwnd, PhpMakeGraphTooltipContextAtom());
    oldWndProc = context->TooltipOldWndProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

            RemoveProp(hwnd, PhpMakeGraphTooltipContextAtom());
        }
        break;
    case WM_MOVE:
        {
            TOOLINFO toolInfo = { sizeof(toolInfo) };
            RECT windowRect;
            BOOLEAN needsMove;

            if (CallWindowProc(oldWndProc, hwnd, TTM_GETCURRENTTOOL, 0, (LPARAM)&toolInfo))
            {
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
                    windowRect.top = context->MonitorInfo.rcWork.bottom - (windowRect.bottom - windowRect.top);
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

VOID PhpUpdateDrawInfo(
    __in HWND hwnd,
    __in PPHP_GRAPH_CONTEXT Context
    )
{
    PH_GRAPH_GETDRAWINFO getDrawInfo;

    Context->DrawInfo.Width = Context->BufferedContextRect.right;
    Context->DrawInfo.Height = Context->BufferedContextRect.bottom;

    getDrawInfo.Header.hwndFrom = hwnd;
    getDrawInfo.Header.idFrom = Context->Id;
    getDrawInfo.Header.code = GCN_GETDRAWINFO;
    getDrawInfo.DrawInfo = &Context->DrawInfo;

    SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM)&getDrawInfo);
}

VOID PhpDrawGraphControl(
    __in HWND hwnd,
    __in PPHP_GRAPH_CONTEXT Context
    )
{
    if (Context->BufferedBits)
        PhDrawGraphDirect(Context->BufferedContext, Context->BufferedBits, &Context->DrawInfo);

    if (Context->Style & GC_STYLE_FADEOUT)
    {
        BLENDFUNCTION blendFunction;

        if (!Context->FadeOutContext)
            PhpCreateFadeOutContext(Context);

        blendFunction.BlendOp = AC_SRC_OVER;
        blendFunction.BlendFlags = 0;
        blendFunction.SourceConstantAlpha = 255;
        blendFunction.AlphaFormat = AC_SRC_ALPHA;
        GdiAlphaBlend(
            Context->BufferedContext,
            0,
            0,
            Context->Options.FadeOutWidth,
            Context->FadeOutContextRect.bottom,
            Context->FadeOutContext,
            0,
            0,
            Context->Options.FadeOutWidth,
            Context->FadeOutContextRect.bottom,
            blendFunction
            );
    }

    if (Context->Style & GC_STYLE_DRAW_PANEL)
    {
        PH_GRAPH_DRAWPANEL drawPanel;

        drawPanel.Header.hwndFrom = hwnd;
        drawPanel.Header.idFrom = Context->Id;
        drawPanel.Header.code = GCN_DRAWPANEL;
        drawPanel.hdc = Context->BufferedContext;
        drawPanel.Rect = Context->BufferedContextRect;

        SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM)&drawPanel);
    }
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
            CREATESTRUCT *createStruct = (CREATESTRUCT *)lParam;

            context->Handle = hwnd;
            context->Style = createStruct->style;
            context->Id = (ULONG_PTR)createStruct->hMenu;
        }
        break;
    case WM_DESTROY:
        {
            if (context->TooltipHandle)
                DestroyWindow(context->TooltipHandle);

            PhpDeleteFadeOutContext(context);
            PhpDeleteBufferedContext(context);
            PhpFreeGraphContext(context);
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)NULL);
        }
        break;
    case WM_STYLECHANGED:
        {
            STYLESTRUCT *styleStruct = (STYLESTRUCT *)lParam;

            if (wParam == GWL_STYLE)
            {
                context->Style = styleStruct->styleNew;
                context->NeedsDraw = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            // Force a re-create of the buffered context.
            PhpCreateBufferedContext(context);
            PhpDeleteFadeOutContext(context);

            PhpUpdateDrawInfo(hwnd, context);
            context->NeedsDraw = TRUE;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            HDC hdc;

            if (hdc = BeginPaint(hwnd, &paintStruct))
            {
                if (!context->BufferedContext)
                    PhpCreateBufferedContext(context);

                if (context->NeedsUpdate)
                {
                    PhpUpdateDrawInfo(hwnd, context);
                    context->NeedsUpdate = FALSE;
                }

                if (context->NeedsDraw)
                {
                    PhpDrawGraphControl(hwnd, context);
                    context->NeedsDraw = FALSE;
                }

                BitBlt(
                    hdc,
                    paintStruct.rcPaint.left,
                    paintStruct.rcPaint.top,
                    paintStruct.rcPaint.right - paintStruct.rcPaint.left,
                    paintStruct.rcPaint.bottom - paintStruct.rcPaint.top,
                    context->BufferedContext,
                    paintStruct.rcPaint.left,
                    paintStruct.rcPaint.top,
                    SRCCOPY
                    );

                EndPaint(hwnd, &paintStruct);
            }
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_NCPAINT:
        {
            HRGN updateRegion;

            updateRegion = (HRGN)wParam;

            if (updateRegion == (HRGN)1) // HRGN_FULL
                updateRegion = NULL;

            // Themed border
            if (context->Style & WS_BORDER)
            {
                HDC hdc;
                ULONG flags;
                RECT rect;

                // Note the use of undocumented flags below. GetDCEx doesn't work without these.

                flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | 0x10000;

                if (updateRegion)
                    flags |= DCX_INTERSECTRGN | 0x40000;

                if (hdc = GetDCEx(hwnd, updateRegion, flags))
                {
                    GetClientRect(hwnd, &rect);
                    rect.right += 2;
                    rect.bottom += 2;
                    SetDCBrushColor(hdc, RGB(0x8f, 0x8f, 0x8f));
                    FrameRect(hdc, &rect, GetStockObject(DC_BRUSH));

                    ReleaseDC(hwnd, hdc);
                    return 0;
                }
            }
        }
        break;
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
                    ScreenToClient(hwnd, &point);
                    GetClientRect(hwnd, &clientRect);

                    getTooltipText.Header.hwndFrom = hwnd;
                    getTooltipText.Header.idFrom = context->Id;
                    getTooltipText.Header.code = GCN_GETTOOLTIPTEXT;
                    getTooltipText.Index = (clientRect.right - point.x - 1) / context->DrawInfo.Step;
                    getTooltipText.TotalCount = context->DrawInfo.LineDataCount;
                    getTooltipText.Text.Buffer = NULL;
                    getTooltipText.Text.Length = 0;

                    SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM)&getTooltipText);

                    if (getTooltipText.Text.Buffer)
                    {
                        dispInfo->lpszText = getTooltipText.Text.Buffer;
                    }
                }
                break;
            case TTN_SHOW:
                context->TooltipVisible = TRUE;
                break;
            case TTN_POP:
                context->TooltipVisible = FALSE;
                break;
            }
        }
        break;
    case WM_SETCURSOR:
        {
            if (context->Options.DefaultCursor)
            {
                SetCursor(context->Options.DefaultCursor);
                return TRUE;
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
            }
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
        {
            PH_GRAPH_MOUSEEVENT mouseEvent;
            RECT clientRect;

            GetClientRect(hwnd, &clientRect);

            mouseEvent.Header.hwndFrom = hwnd;
            mouseEvent.Header.idFrom = context->Id;
            mouseEvent.Header.code = GCN_MOUSEEVENT;
            mouseEvent.Message = uMsg;
            mouseEvent.Keys = (ULONG)wParam;
            mouseEvent.Point.x = LOWORD(lParam);
            mouseEvent.Point.y = HIWORD(lParam);

            mouseEvent.Index = (clientRect.right - mouseEvent.Point.x - 1) / context->DrawInfo.Step;
            mouseEvent.TotalCount = context->DrawInfo.LineDataCount;

            SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM)&mouseEvent);
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
            PhpUpdateDrawInfo(hwnd, context);
            context->NeedsDraw = TRUE;
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

                context->TooltipHandle = CreateWindow(
                    TOOLTIPS_CLASS,
                    NULL,
                    WS_POPUP | WS_EX_TRANSPARENT | TTS_NOPREFIX,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    NULL,
                    NULL,
                    PhLibImageBase,
                    NULL
                    );
                context->TooltipOldWndProc = (WNDPROC)GetWindowLongPtr(context->TooltipHandle, GWLP_WNDPROC);
                SetProp(context->TooltipHandle, PhpMakeGraphTooltipContextAtom(), (HANDLE)context);
                SetWindowLongPtr(context->TooltipHandle, GWLP_WNDPROC, (LONG_PTR)PhpTooltipWndProc);

                SetWindowPos(context->TooltipHandle, HWND_TOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                toolInfo.uFlags = TTF_ABSOLUTE | TTF_TRACK;
                toolInfo.hwnd = hwnd;
                toolInfo.uId = 1;
                toolInfo.lpszText = LPSTR_TEXTCALLBACK;
                SendMessage(context->TooltipHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

                // Allow newlines (-1 doesn't work)
                // MAXINT doesn't work either on high DPI configurations for some reason.
                SendMessage(context->TooltipHandle, TTM_SETMAXTIPWIDTH, 0, 4096);
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
    case GCM_GETOPTIONS:
        memcpy((PPH_GRAPH_OPTIONS)lParam, &context->Options, sizeof(PH_GRAPH_OPTIONS));
        return TRUE;
    case GCM_SETOPTIONS:
        memcpy(&context->Options, (PPH_GRAPH_OPTIONS)lParam, sizeof(PH_GRAPH_OPTIONS));
        PhpDeleteFadeOutContext(context);
        return TRUE;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * Initializes a graph buffer management structure.
 *
 * \param Buffers The buffer management structure.
 */
VOID PhInitializeGraphBuffers(
    __out PPH_GRAPH_BUFFERS Buffers
    )
{
    Buffers->AllocatedCount = 0;
    Buffers->Data1 = NULL;
    Buffers->Data2 = NULL;
    Buffers->Valid = FALSE;
}

/**
 * Frees resources used by a graph buffer management structure.
 *
 * \param Buffers The buffer management structure.
 */
VOID PhDeleteGraphBuffers(
    __inout PPH_GRAPH_BUFFERS Buffers
    )
{
    if (Buffers->Data1) PhFree(Buffers->Data1);
    if (Buffers->Data2) PhFree(Buffers->Data2);
}

/**
 * Sets up a graphing information structure with information
 * from a graph buffer management structure.
 *
 * \param Buffers The buffer management structure.
 * \param DrawInfo The graphing information structure.
 * \param DataCount The number of data points currently required.
 * The buffers are resized if needed.
 */
VOID PhGetDrawInfoGraphBuffers(
    __inout PPH_GRAPH_BUFFERS Buffers,
    __inout PPH_GRAPH_DRAW_INFO DrawInfo,
    __in ULONG DataCount
    )
{
    DrawInfo->LineDataCount = min(DataCount, PH_GRAPH_DATA_COUNT(DrawInfo->Width, DrawInfo->Step));

    // Do we need to allocate or re-allocate the data buffers?
    if (Buffers->AllocatedCount < DrawInfo->LineDataCount)
    {
        if (Buffers->Data1)
            PhFree(Buffers->Data1);
        if ((DrawInfo->Flags & PH_GRAPH_USE_LINE_2) && Buffers->Data2)
            PhFree(Buffers->Data2);

        Buffers->AllocatedCount *= 2;

        if (Buffers->AllocatedCount < DrawInfo->LineDataCount)
            Buffers->AllocatedCount = DrawInfo->LineDataCount;

        Buffers->Data1 = PhAllocate(Buffers->AllocatedCount * sizeof(FLOAT));

        if (DrawInfo->Flags & PH_GRAPH_USE_LINE_2)
        {
            Buffers->Data2 = PhAllocate(Buffers->AllocatedCount * sizeof(FLOAT));
        }

        Buffers->Valid = FALSE;
    }

    DrawInfo->LineData1 = Buffers->Data1;
    DrawInfo->LineData2 = Buffers->Data2;
}

VOID PhInitializeGraphState(
    __out PPH_GRAPH_STATE State
    )
{
    PhInitializeGraphBuffers(&State->Buffers);
    State->Text = NULL;
    State->TooltipText = NULL;
    State->TooltipIndex = -1;
}

VOID PhDeleteGraphState(
    __inout PPH_GRAPH_STATE State
    )
{
    PhDeleteGraphBuffers(&State->Buffers);
    if (State->Text) PhDereferenceObject(State->Text);
    if (State->TooltipText) PhDereferenceObject(State->TooltipText);
}

VOID PhGraphStateGetDrawInfo(
    __inout PPH_GRAPH_STATE State,
    __in PPH_GRAPH_GETDRAWINFO GetDrawInfo,
    __in ULONG DataCount
    )
{
    PhGetDrawInfoGraphBuffers(&State->Buffers, GetDrawInfo->DrawInfo, DataCount);
}
