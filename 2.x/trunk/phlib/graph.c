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
            if (flags & PH_GRAPH_USE_LINE_2)
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
