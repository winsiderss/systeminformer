/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#include <ph.h>
#include <math.h>

#include <graph.h>
#include <guisup.h>

#define COLORREF_TO_BITS(Color) (_byteswap_ulong(Color) >> 8)

typedef struct _PHP_GRAPH_CONTEXT
{
    HWND Handle;
    HWND ParentHandle;
    ULONG Style;
    ULONG_PTR Id;
    PH_GRAPH_DRAW_INFO DrawInfo;
    PH_GRAPH_OPTIONS Options;

    union
    {
        USHORT Flags;
        struct
        {
            USHORT NeedsUpdate : 1;
            USHORT NeedsDraw : 1;
            USHORT Hot : 1;
            USHORT Spare : 13;
        };
    };

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
    POINT LastCursorLocation;
} PHP_GRAPH_CONTEXT, *PPHP_GRAPH_CONTEXT;

LRESULT CALLBACK PhpGraphWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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

FORCEINLINE VOID PhpGetGraphPoint(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG Index,
    _Out_ PULONG H1,
    _Out_ PULONG H2
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
        else
        {
            *H2 = *H1;
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
 * \li If \ref PH_GRAPH_USE_LINE_2 is specified in \a Flags, \ref PH_GRAPH_OVERLAY_LINE_2 is never
 * used.
 */
VOID PhDrawGraphDirect(
    _In_ HDC hdc,
    _In_ PVOID Bits,
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo
    )
{
    PULONG bits = Bits;
    LONG width = DrawInfo->Width;
    LONG height = DrawInfo->Height;
    LONG numberOfPixels = width * height;
    ULONG flags = DrawInfo->Flags;
    LONG i;
    LONG x;

    BOOLEAN intermediate = FALSE; // whether we are currently between two data positions
    ULONG dataIndex = 0; // the data index of the current position
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
    FLOAT gridHeight;
    LONG gridYThreshold;
    ULONG gridYCounter;
    ULONG gridColor;
    FLOAT gridBase;
    FLOAT gridLevel;

    ULONG yLabelMax;
    ULONG yLabelDataIndex;

    lineColor1 = COLORREF_TO_BITS(DrawInfo->LineColor1);
    lineBackColor1 = COLORREF_TO_BITS(DrawInfo->LineBackColor1);
    lineColor2 = COLORREF_TO_BITS(DrawInfo->LineColor2);
    lineBackColor2 = COLORREF_TO_BITS(DrawInfo->LineBackColor2);

    if (DrawInfo->BackColor == 0)
    {
        memset(bits, 0, numberOfPixels * sizeof(RGBQUAD));
    }
    else
    {
        PhFillMemoryUlong(bits, COLORREF_TO_BITS(DrawInfo->BackColor), numberOfPixels);
    }

    x = width - 1;
    h1_low2 = MAXLONG;
    h1_high2 = 0;
    h2_low2 = MAXLONG;
    h2_high2 = 0;

    PhpGetGraphPoint(DrawInfo, 0, &h1_i, &h2_i);

    if (flags & (PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y))
    {
        gridHeight = max(DrawInfo->GridHeight, 0);
        gridLevel = gridHeight;
        gridYThreshold = DrawInfo->GridYThreshold;
        gridYCounter = DrawInfo->GridWidth - (DrawInfo->GridXOffset * DrawInfo->Step) % DrawInfo->GridWidth - 1;
        gridColor = COLORREF_TO_BITS(DrawInfo->GridColor);
    }

    if ((flags & (PH_GRAPH_USE_GRID_Y | PH_GRAPH_LOGARITHMIC_GRID_Y)) == (PH_GRAPH_USE_GRID_Y | PH_GRAPH_LOGARITHMIC_GRID_Y))
    {
        // Pre-process to find the largest integer n such that GridHeight*GridBase^n < 1.

        gridBase = DrawInfo->GridBase;

        if (gridBase > 1)
        {
            DOUBLE logBase;
            DOUBLE exponent;
            DOUBLE high;

            logBase = log(gridBase);
            exponent = ceil(-log(gridHeight) / logBase) - 1; // Works for both GridHeight > 1 and GridHeight < 1
            high = exp(exponent * logBase);
            gridLevel = (FLOAT)(gridHeight * high);

            if (gridLevel < 0 || !isfinite(gridLevel))
                gridLevel = 0;
            if (gridLevel > 1)
                gridLevel = 1;
        }
        else
        {
            // This is an error.
            gridLevel = 0;
        }
    }

    if (flags & PH_GRAPH_LABEL_MAX_Y)
    {
        yLabelMax = h2_i;
        yLabelDataIndex = 0;
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

            if ((flags & PH_GRAPH_LABEL_MAX_Y) && dataIndex < DrawInfo->LabelMaxYIndexLimit)
            {
                if (yLabelMax <= h2_i)
                {
                    yLabelMax = h2_i;
                    yLabelDataIndex = dataIndex;
                }
            }
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
        // In both examples above, the line low2-high2 will be merged with the line low1-high1 of
        // the next iteration.

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

        if (flags & PH_GRAPH_USE_GRID_X)
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
        }

        if (flags & PH_GRAPH_USE_GRID_Y)
        {
            FLOAT level;
            LONG h;
            LONG h_last;

            // Draw the horizontal grid line.
            if (flags & PH_GRAPH_LOGARITHMIC_GRID_Y)
            {
                level = gridLevel;
                h = (LONG)(level * (height - 1));
                h_last = height + gridYThreshold - 1;

                while (TRUE)
                {
                    if (h <= h_last - gridYThreshold)
                    {
                        bits[x + h * width] = gridColor;
                        h_last = h;
                    }
                    else
                    {
                        break;
                    }

                    level /= gridBase;
                    h = (LONG)(level * (height - 1));
                }
            }
            else
            {
                level = gridHeight;
                h = (LONG)(level * (height - 1));
                h_last = 0;

                while (h < height - 1)
                {
                    if (h >= h_last + gridYThreshold)
                    {
                        bits[x + h * width] = gridColor;
                        h_last = h;
                    }

                    level += gridHeight;
                    h = (LONG)(level * (height - 1));
                }
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

    if ((flags & PH_GRAPH_LABEL_MAX_Y) && yLabelDataIndex < DrawInfo->LineDataCount)
    {
        FLOAT value;
        PPH_STRING label;

        value = DrawInfo->LineData1[yLabelDataIndex];

        if (flags & PH_GRAPH_USE_LINE_2)
            value += DrawInfo->LineData2[yLabelDataIndex];

        if (label = DrawInfo->LabelYFunction(DrawInfo, yLabelDataIndex, value, DrawInfo->LabelYFunctionParameter))
        {
            HFONT oldFont = NULL;
            SIZE textSize;
            RECT rect;

            if (DrawInfo->LabelYFont)
                oldFont = SelectFont(hdc, DrawInfo->LabelYFont);

            SetTextColor(hdc, DrawInfo->LabelYColor);
            SetBkMode(hdc, TRANSPARENT);

            GetTextExtentPoint32(hdc, label->Buffer, (ULONG)label->Length / sizeof(WCHAR), &textSize);

            rect.bottom = height - yLabelMax - PhNormalGraphTextPadding.top;
            rect.top = rect.bottom - textSize.cy;

            if (rect.top < PhNormalGraphTextPadding.top)
            {
                rect.top = PhNormalGraphTextPadding.top;
                rect.bottom = rect.top + textSize.cy;
            }

            rect.left = 0;
            rect.right = width - min((LONG)yLabelDataIndex * 2, width) - PhNormalGraphTextPadding.right;
            DrawText(hdc, label->Buffer, (ULONG)label->Length / sizeof(WCHAR), &rect, DT_NOCLIP | DT_RIGHT);

            if (oldFont)
                SelectFont(hdc, oldFont);

            PhDereferenceObject(label);
        }
    }

    if (DrawInfo->Text.Buffer)
    {
        HFONT oldFont = NULL;

        if (DrawInfo->TextFont)
            oldFont = SelectFont(hdc, DrawInfo->TextFont);

        SetBkMode(hdc, TRANSPARENT);

        // Fill in the text box.
        SetDCBrushColor(hdc, DrawInfo->TextBoxColor);
        FillRect(hdc, &DrawInfo->TextBoxRect, GetStockBrush(DC_BRUSH));

        // Draw the text.
        SetTextColor(hdc, DrawInfo->TextColor);
        DrawText(hdc, DrawInfo->Text.Buffer, (ULONG)DrawInfo->Text.Length / sizeof(WCHAR), &DrawInfo->TextRect, DT_NOCLIP);

        if (oldFont)
            SelectFont(hdc, oldFont);
    }
}

/**
 * Sets the text in a graphing information structure.
 *
 * \param hdc The DC to perform calculations from.
 * \param DrawInfo A structure which contains graphing information. The structure is modified to
 * contain the new text information.
 * \param Text The text.
 * \param Margin The margins of the text box from the edges of the graph.
 * \param Padding The padding within the text box.
 * \param Align The alignment of the text box.
 */
VOID PhSetGraphText(
    _In_ HDC hdc,
    _Inout_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ PPH_STRINGREF Text,
    _In_ PRECT Margin,
    _In_ PRECT Padding,
    _In_ ULONG Align
    )
{
    HFONT oldFont = NULL;
    SIZE textSize;
    PH_RECTANGLE boxRectangle;
    PH_RECTANGLE textRectangle;

    if (DrawInfo->TextFont)
        oldFont = SelectFont(hdc, DrawInfo->TextFont);

    DrawInfo->Text = *Text;
    GetTextExtentPoint32(hdc, Text->Buffer, (ULONG)Text->Length / sizeof(WCHAR), &textSize);

    if (oldFont)
        SelectFont(hdc, oldFont);

    // Calculate the box rectangle.

    boxRectangle.Width = textSize.cx + Padding->left + Padding->right;
    boxRectangle.Height = textSize.cy + Padding->top + Padding->bottom;

    if (Align & PH_ALIGN_LEFT)
        boxRectangle.Left = Margin->left;
    else if (Align & PH_ALIGN_RIGHT)
        boxRectangle.Left = DrawInfo->Width - boxRectangle.Width - Margin->right;
    else
        boxRectangle.Left = (DrawInfo->Width - boxRectangle.Width) / (LONG)sizeof(WCHAR);

    if (Align & PH_ALIGN_TOP)
        boxRectangle.Top = Margin->top;
    else if (Align & PH_ALIGN_BOTTOM)
        boxRectangle.Top = DrawInfo->Height - boxRectangle.Height - Margin->bottom;
    else
        boxRectangle.Top = (DrawInfo->Height - boxRectangle.Height) / (LONG)sizeof(WCHAR);

    // Calculate the text rectangle.

    textRectangle.Left = boxRectangle.Left + Padding->left;
    textRectangle.Top = boxRectangle.Top + Padding->top;
    textRectangle.Width = textSize.cx;
    textRectangle.Height = textSize.cy;

    // Save the rectangles.
    DrawInfo->TextRect = PhRectangleToRect(textRectangle);
    DrawInfo->TextBoxRect = PhRectangleToRect(boxRectangle);
}

VOID PhDrawTrayIconText(
    _In_ HDC hdc,
    _In_ PVOID Bits,
    _Inout_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ PPH_STRINGREF Text
    )
{
    PULONG bits = Bits;
    LONG width = DrawInfo->Width;
    LONG height = DrawInfo->Height;
    LONG numberOfPixels = width * height;
    ULONG flags = DrawInfo->Flags;
    HFONT oldFont = NULL;
    SIZE textSize;
    PH_RECTANGLE boxRectangle;
    PH_RECTANGLE textRectangle;

    if (DrawInfo->BackColor == 0)
    {
        memset(bits, 0, numberOfPixels * sizeof(RGBQUAD));
    }
    else
    {
        PhFillMemoryUlong(bits, COLORREF_TO_BITS(DrawInfo->BackColor), numberOfPixels);
    }

    if (DrawInfo->TextFont)
        oldFont = SelectFont(hdc, DrawInfo->TextFont);

    DrawInfo->Text = *Text;
    GetTextExtentPoint32(hdc, Text->Buffer, (ULONG)Text->Length / sizeof(WCHAR), &textSize);

    // Calculate the box rectangle.

    boxRectangle.Width = textSize.cx;
    boxRectangle.Height = textSize.cy;
    boxRectangle.Left = (DrawInfo->Width - boxRectangle.Width) / (LONG)sizeof(WCHAR);
    boxRectangle.Top = (DrawInfo->Height - boxRectangle.Height) / (LONG)sizeof(WCHAR);

    // Calculate the text rectangle.

    textRectangle.Left = boxRectangle.Left;
    textRectangle.Top = boxRectangle.Top;
    textRectangle.Width = textSize.cx;
    textRectangle.Height = textSize.cy;

    // Save the rectangles.
    DrawInfo->TextRect = PhRectangleToRect(textRectangle);
    DrawInfo->TextBoxRect = PhRectangleToRect(boxRectangle);

    SetBkMode(hdc, TRANSPARENT);

    // Fill in the text box.
    //SetDCBrushColor(hdc, DrawInfo->TextBoxColor);
    //FillRect(hdc, &DrawInfo->TextBoxRect, GetStockBrush(DC_BRUSH));

    // Draw the text.
    SetTextColor(hdc, DrawInfo->TextColor);

    DrawText(hdc, DrawInfo->Text.Buffer, (ULONG)DrawInfo->Text.Length / sizeof(WCHAR), &DrawInfo->TextRect, DT_NOCLIP | DT_SINGLELINE);

    if (oldFont)
        SelectFont(hdc, oldFont);
}

VOID PhpCreateGraphContext(
    _Out_ PPHP_GRAPH_CONTEXT *Context
    )
{
    PPHP_GRAPH_CONTEXT context;

    context = PhAllocate(sizeof(PHP_GRAPH_CONTEXT));
    memset(context, 0, sizeof(PHP_GRAPH_CONTEXT));

    context->DrawInfo.Width = 3;
    context->DrawInfo.Height = 3;
    context->DrawInfo.Flags = PH_GRAPH_USE_GRID_X;
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
    context->DrawInfo.GridHeight = 0.25f;
    context->DrawInfo.GridXOffset = 0;
    context->DrawInfo.GridYThreshold = 10;
    context->DrawInfo.GridBase = 2.0f;
    context->DrawInfo.LabelYColor = RGB(0x77, 0x77, 0x77);
    context->DrawInfo.LabelMaxYIndexLimit = -1;
    context->DrawInfo.TextColor = RGB(0x00, 0xff, 0x00);
    context->DrawInfo.TextBoxColor = RGB(0x00, 0x22, 0x00);

    context->Options.FadeOutBackColor = RGB(0xef, 0xef, 0xef);
    context->Options.FadeOutWidth = 100;

    *Context = context;
}

VOID PhpFreeGraphContext(
    _Inout_ _Post_invalid_ PPHP_GRAPH_CONTEXT Context
    )
{
    PhFree(Context);
}

static VOID PhpDeleteBufferedContext(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    if (Context->BufferedContext)
    {
        // The original bitmap must be selected back into the context, otherwise the bitmap can't be
        // deleted.
        SelectBitmap(Context->BufferedContext, Context->BufferedOldBitmap);
        DeleteBitmap(Context->BufferedBitmap);
        DeleteDC(Context->BufferedContext);

        Context->BufferedContext = NULL;
        Context->BufferedBitmap = NULL;
        Context->BufferedBits = NULL;
    }
}

static VOID PhpCreateBufferedContext(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    HDC hdc;
    BITMAPINFO bitmapInfo;

    PhpDeleteBufferedContext(Context);

    if (!GetClientRect(Context->Handle, &Context->BufferedContextRect))
        return;
    if (!(Context->BufferedContextRect.right && Context->BufferedContextRect.bottom))
        return;

    memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biWidth = Context->BufferedContextRect.right;
    bitmapInfo.bmiHeader.biHeight = Context->BufferedContextRect.bottom;
    bitmapInfo.bmiHeader.biBitCount = 32;

    hdc = GetDC(Context->Handle);
    Context->BufferedContext = CreateCompatibleDC(hdc);
    Context->BufferedBitmap = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, &Context->BufferedBits, NULL, 0);
    Context->BufferedOldBitmap = SelectBitmap(Context->BufferedContext, Context->BufferedBitmap);
    ReleaseDC(Context->Handle, hdc);
}

static VOID PhpDeleteFadeOutContext(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    if (Context->FadeOutContext)
    {
        SelectBitmap(Context->FadeOutContext, Context->FadeOutOldBitmap);
        DeleteBitmap(Context->FadeOutBitmap);
        DeleteDC(Context->FadeOutContext);

        Context->FadeOutContext = NULL;
        Context->FadeOutBitmap = NULL;
        Context->FadeOutBits = NULL;
    }
}

static VOID PhpCreateFadeOutContext(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    HDC hdc;
    BITMAPINFO bitmapInfo;
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

    memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biWidth = Context->FadeOutContextRect.right;
    bitmapInfo.bmiHeader.biHeight = Context->FadeOutContextRect.bottom;
    bitmapInfo.bmiHeader.biBitCount = 32;

    hdc = GetDC(Context->Handle);
    Context->FadeOutContext = CreateCompatibleDC(hdc);
    Context->FadeOutBitmap = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, &Context->FadeOutBits, NULL, 0);
    Context->FadeOutOldBitmap = SelectBitmap(Context->FadeOutContext, Context->FadeOutBitmap);
    ReleaseDC(Context->Handle, hdc);

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

VOID PhpUpdateDrawInfo(
    _In_ HWND hwnd,
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    PH_GRAPH_GETDRAWINFO getDrawInfo;

    if (!(Context->BufferedContextRect.right && Context->BufferedContextRect.bottom))
        return;

    Context->DrawInfo.Width = Context->BufferedContextRect.right;
    Context->DrawInfo.Height = Context->BufferedContextRect.bottom;

    getDrawInfo.Header.hwndFrom = hwnd;
    getDrawInfo.Header.idFrom = Context->Id;
    getDrawInfo.Header.code = GCN_GETDRAWINFO;
    getDrawInfo.DrawInfo = &Context->DrawInfo;

    SendMessage(Context->ParentHandle, WM_NOTIFY, 0, (LPARAM)&getDrawInfo);
}

VOID PhpDrawGraphControl(
    _In_ HWND hwnd,
    _In_ PPHP_GRAPH_CONTEXT Context
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

        if (Context->FadeOutContext)
        {
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
    }

    if (Context->Style & GC_STYLE_DRAW_PANEL)
    {
        PH_GRAPH_DRAWPANEL drawPanel;

        drawPanel.Header.hwndFrom = hwnd;
        drawPanel.Header.idFrom = Context->Id;
        drawPanel.Header.code = GCN_DRAWPANEL;
        drawPanel.hdc = Context->BufferedContext;
        drawPanel.Rect = Context->BufferedContextRect;

        SendMessage(Context->ParentHandle, WM_NOTIFY, 0, (LPARAM)&drawPanel);
    }
}

LRESULT CALLBACK PhpGraphWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_GRAPH_CONTEXT context;

    context = PhGetWindowContextEx(hwnd);

    if (uMsg == WM_CREATE)
    {
        PhpCreateGraphContext(&context);
        PhSetWindowContextEx(hwnd, context);
    }

    if (!context)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

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
            if (context->TooltipHandle)
            {
                MSG message;

                message.hwnd = hwnd;
                message.message = uMsg;
                message.wParam = wParam;
                message.lParam = lParam;
                SendMessage(context->TooltipHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }
        }
        break;
    }

    switch (uMsg)
    {
    case WM_CREATE:
        {
            CREATESTRUCT *createStruct = (CREATESTRUCT *)lParam;

            context->Handle = hwnd;
            context->ParentHandle = createStruct->hwndParent;
            context->Style = createStruct->style;
            context->Id = (ULONG_PTR)createStruct->hMenu;
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContextEx(hwnd);

            if (context->TooltipHandle)
                DestroyWindow(context->TooltipHandle);

            PhpDeleteFadeOutContext(context);
            PhpDeleteBufferedContext(context);
            PhpFreeGraphContext(context);
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

            if (context->TooltipHandle)
            {
                TOOLINFO toolInfo;

                memset(&toolInfo, 0, sizeof(TOOLINFO));
                toolInfo.cbSize = sizeof(TOOLINFO);
                toolInfo.hwnd = hwnd;
                toolInfo.uId = 1;
                GetClientRect(hwnd, &toolInfo.rect);
                SendMessage(context->TooltipHandle, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);
            }
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

            if (updateRegion == HRGN_FULL)
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
                    FrameRect(hdc, &rect, GetStockBrush(DC_BRUSH));

                    ReleaseDC(hwnd, hdc);
                    return 0;
                }
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->hwndFrom == context->TooltipHandle)
            {
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

                        SendMessage(context->ParentHandle, WM_NOTIFY, 0, (LPARAM)&getTooltipText);

                        if (getTooltipText.Text.Buffer)
                        {
                            dispInfo->lpszText = getTooltipText.Text.Buffer;
                        }
                    }
                    break;
                }
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
                POINT point;

                GetCursorPos(&point);
                ScreenToClient(hwnd, &point);

                if (context->LastCursorLocation.x != point.x || context->LastCursorLocation.y != point.y)
                {
                    SendMessage(context->TooltipHandle, TTM_UPDATE, 0, 0);
                    context->LastCursorLocation = point;
                }

                if (!context->Hot)
                {
                    TRACKMOUSEEVENT trackMouseEvent;

                    context->Hot = TRUE;
                    trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                    trackMouseEvent.dwFlags = TME_LEAVE;
                    trackMouseEvent.hwndTrack = hwnd;
                    trackMouseEvent.dwHoverTime = 0;
                    TrackMouseEvent(&trackMouseEvent);
                }
            }
        }
        break;
    case WM_MOUSELEAVE:
        {
            context->Hot = FALSE;

            if (context->TooltipHandle)
            {
                SendMessage(context->TooltipHandle, TTM_POP, 0, 0);
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
            mouseEvent.Point.x = GET_X_LPARAM(lParam);
            mouseEvent.Point.y = GET_Y_LPARAM(lParam);

            mouseEvent.Index = (clientRect.right - mouseEvent.Point.x - 1) / context->DrawInfo.Step;
            mouseEvent.TotalCount = context->DrawInfo.LineDataCount;

            SendMessage(context->ParentHandle, WM_NOTIFY, 0, (LPARAM)&mouseEvent);
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

            context->DrawInfo.GridXOffset += increment;
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
                    WS_POPUP | WS_EX_TRANSPARENT | TTS_NOPREFIX | TTS_ALWAYSTIP,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    NULL,
                    NULL,
                    PhInstanceHandle,
                    NULL
                    );

                SetWindowPos(context->TooltipHandle, HWND_TOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                toolInfo.uFlags = 0;
                toolInfo.hwnd = hwnd;
                toolInfo.uId = 1;
                toolInfo.lpszText = LPSTR_TEXTCALLBACK;
                GetClientRect(hwnd, &toolInfo.rect);
                SendMessage(context->TooltipHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

                SendMessage(context->TooltipHandle, TTM_SETDELAYTIME, TTDT_INITIAL, 0);
                SendMessage(context->TooltipHandle, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);
                // Allow newlines (-1 doesn't work)
                SendMessage(context->TooltipHandle, TTM_SETMAXTIPWIDTH, 0, MAXSHORT);

                if (PhEnableThemeSupport)
                {
                    PhSetControlTheme(context->TooltipHandle, L"DarkMode_Explorer");
                    //SendMessage(context->TooltipHandle, TTM_SETWINDOWTHEME, 0, (LPARAM)L"DarkMode_Explorer");
                }
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

            SendMessage(context->TooltipHandle, TTM_UPDATE, 0, 0);
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
    _Out_ PPH_GRAPH_BUFFERS Buffers
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
    _Inout_ PPH_GRAPH_BUFFERS Buffers
    )
{
    if (Buffers->Data1) PhFree(Buffers->Data1);
    if (Buffers->Data2) PhFree(Buffers->Data2);
}

/**
 * Sets up a graphing information structure with information from a graph buffer management
 * structure.
 *
 * \param Buffers The buffer management structure.
 * \param DrawInfo The graphing information structure.
 * \param DataCount The number of data points currently required. The buffers are resized if needed.
 */
VOID PhGetDrawInfoGraphBuffers(
    _Inout_ PPH_GRAPH_BUFFERS Buffers,
    _Inout_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataCount
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
    _Out_ PPH_GRAPH_STATE State
    )
{
    PhInitializeGraphBuffers(&State->Buffers);
    State->Text = NULL;
    State->TooltipText = NULL;
    State->TooltipIndex = -1;
}

VOID PhDeleteGraphState(
    _Inout_ PPH_GRAPH_STATE State
    )
{
    PhDeleteGraphBuffers(&State->Buffers);
    if (State->Text) PhDereferenceObject(State->Text);
    if (State->TooltipText) PhDereferenceObject(State->TooltipText);
}

VOID PhGraphStateGetDrawInfo(
    _Inout_ PPH_GRAPH_STATE State,
    _In_ PPH_GRAPH_GETDRAWINFO GetDrawInfo,
    _In_ ULONG DataCount
    )
{
    PhGetDrawInfoGraphBuffers(&State->Buffers, GetDrawInfo->DrawInfo, DataCount);
}
