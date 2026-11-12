/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2026
 *
 */

#include <ph.h>
#include <math.h>
#include <graph.h>
#include <guisup.h>

#define COLORREF_TO_BITS(Color) (_byteswap_ulong(Color) >> 8)

#define PH_GRAPH_TEXT_MASK_SCALE 4

typedef struct _PHP_GRAPH_TEXT_MASK
{
    HDC Context;
    HFONT Font;
    HFONT OldFont;
    HBITMAP Bitmap;
    HBITMAP OldBitmap;
    PULONG Bits;
    LONG Width; // mask pixels, PH_GRAPH_TEXT_MASK_SCALE per surface pixel
    LONG Height;
    FLOAT TextWidth; // surface pixels
    FLOAT TextHeight;
} PHP_GRAPH_TEXT_MASK, *PPHP_GRAPH_TEXT_MASK;
// The peak label of a graph control: rendered once per change of the text or font instead of
// on every animation frame.
typedef struct _PHP_GRAPH_LABEL_CACHE
{
    PPH_STRING Text; // NULL when empty
    LOGFONT Font;
    PHP_GRAPH_TEXT_MASK Mask;
} PHP_GRAPH_LABEL_CACHE, *PPHP_GRAPH_LABEL_CACHE;
typedef struct _PHP_GRAPH_CONTEXT
{
    HWND Handle;
    HWND ParentHandle;
    ULONG Style;
    ULONG ExStyle;
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

    PPH_GRAPH_MESSAGE_CALLBACK Callback;
    PVOID Context;

    // Smooth renderer only: between updates the graph scrolls left at a constant speed while
    // the data morphs from the previous values (shifted by the new samples) to the current ones.
    ULONG64 AnimationStartTime; // 0 when not animating
    ULONG64 AnimationLastStartTime;
    ULONG AnimationDuration; // ms, the measured update interval
    LONG AnimationShift; // samples added by this update
    FLOAT AnimationProgress; // 0 to 1
    ULONG AnimationCapacity; // elements in each buffer below
    ULONG AnimationTargetCount;
    ULONG AnimationFromCount;
    PFLOAT AnimationTarget1; // the data of the last update
    PFLOAT AnimationTarget2;
    PFLOAT AnimationFrom1; // the data of the update before it
    PFLOAT AnimationFrom2;
    PFLOAT AnimationFrame1; // the data being drawn
    PFLOAT AnimationFrame2;

    PH_GRAPH_TEXT_BOX_STATE TextBoxState;
    PH_GRAPH_TEXT_BOX_STATE TextBoxState2;
    PHP_GRAPH_LABEL_CACHE LabelCache;
} PHP_GRAPH_CONTEXT, *PPHP_GRAPH_CONTEXT;

#define PH_GRAPH_ANIMATION_TIMER_ID 1
#define PH_GRAPH_ANIMATION_FRAMES 12 // frames per update
#define PH_GRAPH_ANIMATION_MIN_FRAME_INTERVAL 16 // ms
#define PH_GRAPH_ANIMATION_MAX_FRAME_INTERVAL 100 // ms
#define PH_GRAPH_ANIMATION_DEFAULT_DURATION 1000 // ms, until the update interval is known
#define PH_GRAPH_ANIMATION_MIN_DURATION 100 // ms
#define PH_GRAPH_ANIMATION_MAX_DURATION 5000 // ms; longer gaps are pauses, not the interval

LRESULT CALLBACK PhpGraphWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

RECT PhNormalGraphTextMargin = { 5, 5, 5, 5 };
RECT PhNormalGraphTextPadding = { 3, 3, 3, 3 };
BOOLEAN PhEnableGraphSmoothing = TRUE;

RTL_ATOM PhGraphControlInitialization(
    VOID
    )
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_GLOBALCLASS | CS_DBLCLKS;
    wcex.lpfnWndProc = PhpGraphWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(PVOID);
    wcex.hInstance = NtCurrentImageBase();
    wcex.hCursor = PhLoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = PH_GRAPH_CLASSNAME;

    return RegisterClassEx(&wcex);
}

#ifndef PH_GRAPH_LABEL_MAX_Y_PIXEL
FORCEINLINE FLOAT PhpGetGraphLabelValue(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG Index
    )
{
    FLOAT value;

    if (Index >= DrawInfo->LineDataCount)
        return 0;

    value = DrawInfo->LineData1[Index];

    if (DrawInfo->Flags & PH_GRAPH_USE_LINE_2)
        value += DrawInfo->LineData2[Index];

    return value;
}
#endif

FORCEINLINE VOID PhpGetGraphPoint(
    _In_ PPH_GRAPH_DRAW_INFO PH_RESTRICT DrawInfo,
    _In_ ULONG Index,
    _Out_ PULONG PH_RESTRICT H1,
    _Out_ PULONG PH_RESTRICT H2
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

// Fills a rectangle with a solid color at the given constant alpha, so the graph
// behind a text box shows through. (semi-transparent overlay background)
static VOID PhpFillRectAlpha(
    _In_ HDC hdc,
    _In_ PRECT Rect,
    _In_ COLORREF Color,
    _In_ BYTE Alpha
    )
{
    LONG width = Rect->right - Rect->left;
    LONG height = Rect->bottom - Rect->top;
    HDC sourceDC;
    HBITMAP sourceBitmap;
    HBITMAP oldBitmap;
    BLENDFUNCTION blendFunction;

    if (width <= 0 || height <= 0)
        return;

    if (!(sourceDC = CreateCompatibleDC(hdc)))
        return;

    if (!(sourceBitmap = CreateCompatibleBitmap(hdc, 1, 1)))
    {
        DeleteDC(sourceDC);
        return;
    }

    oldBitmap = SelectBitmap(sourceDC, sourceBitmap);
    SetPixelV(sourceDC, 0, 0, Color);

    blendFunction.BlendOp = AC_SRC_OVER;
    blendFunction.BlendFlags = 0;
    blendFunction.SourceConstantAlpha = Alpha;
    blendFunction.AlphaFormat = 0;

    GdiAlphaBlend(hdc, Rect->left, Rect->top, width, height, sourceDC, 0, 0, 1, 1, blendFunction);

    SelectBitmap(sourceDC, oldBitmap);
    DeleteObject(sourceBitmap);
    DeleteDC(sourceDC);
}

static VOID PhpDrawGraphClassic(
    _In_ PVOID PH_RESTRICT Bits,
    _In_ PPH_GRAPH_DRAW_INFO PH_RESTRICT DrawInfo,
    _Out_ PULONG LabelMaxY,
    _Out_ PULONG LabelDataIndex
    )
{
    PULONG PH_RESTRICT bits = Bits;
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

    ULONG yLabelMax = 0;
    ULONG yLabelDataIndex = 0;
#ifndef PH_GRAPH_LABEL_MAX_Y_PIXEL
    FLOAT yLabelValue = 0;
#endif

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
#ifndef PH_GRAPH_LABEL_MAX_Y_PIXEL
        yLabelValue = PhpGetGraphLabelValue(DrawInfo, 0);
#endif
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
#ifndef PH_GRAPH_LABEL_MAX_Y_PIXEL
                FLOAT value;

                // Compare the raw data instead of the scaled pixel height. Peaks with
                // similar values round to the same height and the selected peak would
                // change with every resize. Ties keep the newest point. (dmex)
                value = PhpGetGraphLabelValue(DrawInfo, dataIndex);

                if (yLabelValue < value)
                {
                    yLabelValue = value;
                    yLabelMax = h2_i;
                    yLabelDataIndex = dataIndex;
                }
#else
                if (yLabelMax <= h2_i)
                {
                    yLabelMax = h2_i;
                    yLabelDataIndex = dataIndex;
                }
#endif
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

    *LabelMaxY = yLabelMax;
    *LabelDataIndex = yLabelDataIndex;
}

typedef struct _PHP_GRAPH_SURFACE
{
    PULONG Bits;
    LONG Width;
    LONG Height;
    BOOLEAN BottomUp;
} PHP_GRAPH_SURFACE, *PPHP_GRAPH_SURFACE;

static COLORREF PhpMixGraphColor(
    _In_ COLORREF Color1,
    _In_ COLORREF Color2,
    _In_ FLOAT Weight
    );

static VOID PhpDrawGraphRoundRect(
    _In_ PPHP_GRAPH_SURFACE Surface,
    _In_ FLOAT Left,
    _In_ FLOAT Top,
    _In_ FLOAT Right,
    _In_ FLOAT Bottom,
    _In_ FLOAT Radius,
    _In_ COLORREF FillColor,
    _In_ FLOAT FillOpacity,
    _In_ COLORREF BorderColor,
    _In_ FLOAT BorderOpacity
    );

#define PH_GRAPH_CHIP_MAX_RADIUS 9.0f // single lines get fully rounded ends
#define PH_GRAPH_CHIP_EXTRA_PADDING_X 3.0f // rounded ends need more room beside the text than above it
#define PH_GRAPH_LABEL_PADDING_X 7.0f // peak label pill: space beside the text
#define PH_GRAPH_BAR_LINE_WIDTH 1.5f // stacked bar: line on the right edge of each segment
#define PH_GRAPH_CHIP_FILL_OPACITY 0.88f
#define PH_GRAPH_CHIP_BORDER_MIX 0.25f // border color: background mixed toward the text color

static VOID PhpDrawGraphChip(
    _In_ PPHP_GRAPH_SURFACE Surface,
    _In_ PRECT Rect,
    _In_ COLORREF FillColor,
    _In_ COLORREF BorderColor
    )
{
    FLOAT radius = min(PH_GRAPH_CHIP_MAX_RADIUS, (FLOAT)(Rect->bottom - Rect->top) * 0.5f);

    PhpDrawGraphRoundRect(
        Surface,
        (FLOAT)Rect->left - PH_GRAPH_CHIP_EXTRA_PADDING_X,
        (FLOAT)Rect->top,
        (FLOAT)Rect->right + PH_GRAPH_CHIP_EXTRA_PADDING_X,
        (FLOAT)Rect->bottom,
        radius,
        FillColor,
        PH_GRAPH_CHIP_FILL_OPACITY,
        BorderColor,
        1.0f
        );
}


static BOOLEAN PhpCreateGraphTextMask(
    _In_ HDC hdc,
    _In_ PPH_STRING Text,
    _Out_ PPHP_GRAPH_TEXT_MASK Mask
    );

static VOID PhpDeleteGraphTextMask(
    _Inout_ PPHP_GRAPH_TEXT_MASK Mask
    );

static VOID PhpDrawGraphTextMask(
    _In_ PPHP_GRAPH_SURFACE Surface,
    _In_ PPHP_GRAPH_TEXT_MASK Mask,
    _In_ FLOAT Left,
    _In_ FLOAT Top,
    _In_ COLORREF Color
    );


static VOID PhpDeleteGraphLabelCache(
    _Inout_ PPHP_GRAPH_LABEL_CACHE Cache
    )
{
    if (Cache->Text)
        PhpDeleteGraphTextMask(&Cache->Mask);

    PhClearReference(&Cache->Text);
}

// Returns the mask for Text in the font selected into hdc, or NULL.
static PPHP_GRAPH_TEXT_MASK PhpGetGraphLabelCacheMask(
    _Inout_ PPHP_GRAPH_LABEL_CACHE Cache,
    _In_ HDC hdc,
    _In_ PPH_STRING Text
    )
{
    LOGFONT logFont;

    if (!GetObject(GetCurrentObject(hdc, OBJ_FONT), sizeof(LOGFONT), &logFont))
        return NULL;

    if (Cache->Text && PhEqualString(Cache->Text, Text, FALSE) && memcmp(&Cache->Font, &logFont, sizeof(LOGFONT)) == 0)
        return &Cache->Mask;

    PhpDeleteGraphLabelCache(Cache);

    if (!PhpCreateGraphTextMask(hdc, Text, &Cache->Mask))
        return NULL;

    Cache->Text = PhReferenceObject(Text);
    Cache->Font = logFont;

    return &Cache->Mask;
}

// Surface is NULL for the classic text style. ValueDrawInfo, when set, holds the data the label
// values are read from (an animation frame is drawn from data between two updates).
static VOID PhpDrawGraphText(
    _In_ HDC hdc,
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_opt_ PPHP_GRAPH_SURFACE Surface,
    _In_ FLOAT Offset,
    _In_opt_ PPH_GRAPH_DRAW_INFO ValueDrawInfo,
    _Inout_opt_ PPHP_GRAPH_LABEL_CACHE LabelCache,
    _In_ ULONG yLabelMax,
    _In_ ULONG yLabelDataIndex
    )
{
    LONG width = DrawInfo->Width;
    LONG height = DrawInfo->Height;
    ULONG flags = DrawInfo->Flags;

    if ((flags & PH_GRAPH_LABEL_MAX_Y) && yLabelDataIndex < DrawInfo->LineDataCount)
    {
        FLOAT value;
        FLOAT labelValue;
        PPH_STRING label;

        value = DrawInfo->LineData1[yLabelDataIndex];

        if (flags & PH_GRAPH_USE_LINE_2)
            value += DrawInfo->LineData2[yLabelDataIndex];

        labelValue = value;

        if (ValueDrawInfo && yLabelDataIndex < ValueDrawInfo->LineDataCount)
        {
            labelValue = ValueDrawInfo->LineData1[yLabelDataIndex];

            if ((flags & PH_GRAPH_USE_LINE_2) && ValueDrawInfo->LineData2)
                labelValue += ValueDrawInfo->LineData2[yLabelDataIndex];
        }

        if (label = DrawInfo->LabelYFunction(DrawInfo, yLabelDataIndex, labelValue, DrawInfo->LabelYFunctionParameter))
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

            PHP_GRAPH_TEXT_MASK labelMask;
            PPHP_GRAPH_TEXT_MASK mask = NULL;

            if (Surface)
            {
                if (LabelCache)
                    mask = PhpGetGraphLabelCacheMask(LabelCache, hdc, label);
                else if (PhpCreateGraphTextMask(hdc, label, &labelMask))
                    mask = &labelMask;
            }

            if (mask)
            {
                FLOAT peakValue = max(min(value, 1.0f), 0.0f);
                FLOAT peakX = (FLOAT)(width - min((LONG)yLabelDataIndex * 2, width)) - 0.5f + Offset;
                FLOAT peakY = (FLOAT)height - (peakValue * (height - 1) + 0.5f); // top-down
                COLORREF borderColor = PhpMixGraphColor(DrawInfo->BackColor, DrawInfo->TextColor, PH_GRAPH_CHIP_BORDER_MIX);
                FLOAT textRight;
                FLOAT textTop;

                // Fractional positions so the label moves with the graph instead of in whole pixels;
                // the pill sits above and to the left of the peak marker, clear of it.
                textRight = peakX - 4.5f - PH_GRAPH_LABEL_PADDING_X; // pill ends 1 px clear of the marker ring
                textTop = peakY - 3.0f - mask->TextHeight;

                // Near the left edge there's no room for the pill: flip it to the right of the marker,
                // and keep it on the graph.
                if (textRight - mask->TextWidth - PH_GRAPH_LABEL_PADDING_X < (FLOAT)PhNormalGraphTextPadding.left)
                {
                    FLOAT textLeft = max(peakX + 4.5f + PH_GRAPH_LABEL_PADDING_X, (FLOAT)PhNormalGraphTextPadding.left + PH_GRAPH_LABEL_PADDING_X);

                    textRight = textLeft + mask->TextWidth;
                }

                // Keep the pill out from under the text box in the top left corner.
                if (
                    DrawInfo->Text.Buffer &&
                    textRight - mask->TextWidth - PH_GRAPH_LABEL_PADDING_X < (FLOAT)DrawInfo->TextBoxRect.right + PH_GRAPH_CHIP_EXTRA_PADDING_X &&
                    textTop - 1.0f < (FLOAT)DrawInfo->TextBoxRect.bottom
                    )
                {
                    textTop = (FLOAT)DrawInfo->TextBoxRect.bottom + 3.0f;
                }

                if (textTop < (FLOAT)PhNormalGraphTextPadding.top)
                    textTop = (FLOAT)PhNormalGraphTextPadding.top;

                PhpDrawGraphRoundRect(
                    Surface,
                    textRight - mask->TextWidth - PH_GRAPH_LABEL_PADDING_X,
                    textTop - 1.0f,
                    textRight + PH_GRAPH_LABEL_PADDING_X,
                    textTop + mask->TextHeight + 1.0f,
                    (mask->TextHeight + 2.0f) * 0.5f,
                    DrawInfo->BackColor,
                    PH_GRAPH_CHIP_FILL_OPACITY,
                    borderColor,
                    1.0f
                    );

                // Peak marker: a dot in the top line's color with a ring in the background color.
                PhpDrawGraphRoundRect(Surface, peakX - 3.5f, peakY - 3.5f, peakX + 3.5f, peakY + 3.5f, 3.5f, DrawInfo->BackColor, 1.0f, 0, 0.0f);
                PhpDrawGraphRoundRect(
                    Surface,
                    peakX - 2.5f,
                    peakY - 2.5f,
                    peakX + 2.5f,
                    peakY + 2.5f,
                    2.5f,
                    (flags & PH_GRAPH_USE_LINE_2) ? DrawInfo->LineColor2 : DrawInfo->LineColor1,
                    1.0f,
                    0,
                    0.0f
                    );

                PhpDrawGraphTextMask(Surface, mask, textRight - mask->TextWidth, textTop, DrawInfo->LabelYColor);

                if (mask == &labelMask)
                    PhpDeleteGraphTextMask(&labelMask);
            }
            else
            {
                DrawText(hdc, label->Buffer, (ULONG)label->Length / sizeof(WCHAR), &rect, DT_NOCLIP | DT_RIGHT);
            }

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
        if (Surface)
        {
            PhpDrawGraphChip(
                Surface,
                &DrawInfo->TextBoxRect,
                DrawInfo->BackColor,
                PhpMixGraphColor(DrawInfo->BackColor, DrawInfo->TextColor, PH_GRAPH_CHIP_BORDER_MIX)
                );
        }
        else
        {
            // Classic renderer: semi-transparent so the graph shows through.
            PhpFillRectAlpha(hdc, &DrawInfo->TextBoxRect, DrawInfo->TextBoxColor, 160);
        }

        // Draw the text.
        SetTextColor(hdc, DrawInfo->TextColor);
        DrawText(hdc, DrawInfo->Text.Buffer, (ULONG)DrawInfo->Text.Length / sizeof(WCHAR), &DrawInfo->TextRect, DT_NOCLIP);

        if (oldFont)
            SelectFont(hdc, oldFont);
    }

    if (DrawInfo->Text2.Buffer)
    {
        HFONT oldFont = NULL;

        if (DrawInfo->TextFont)
            oldFont = SelectFont(hdc, DrawInfo->TextFont);

        SetBkMode(hdc, TRANSPARENT);

        // Fill in the text box.
        if (Surface)
        {
            PhpDrawGraphChip(
                Surface,
                &DrawInfo->TextBoxRect2,
                DrawInfo->BackColor,
                PhpMixGraphColor(DrawInfo->BackColor, DrawInfo->TextColor, PH_GRAPH_CHIP_BORDER_MIX)
                );
        }
        else
        {
            // Classic renderer: semi-transparent so the graph shows through.
            PhpFillRectAlpha(hdc, &DrawInfo->TextBoxRect2, DrawInfo->TextBoxColor, 160);
        }

        // Draw the text.
        SetTextColor(hdc, DrawInfo->TextColor);
        DrawText(
            hdc,
            DrawInfo->Text2.Buffer,
            (ULONG)DrawInfo->Text2.Length / sizeof(WCHAR),
            &DrawInfo->TextRect2,
            DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK | DT_EDITCONTROL
            );

        if (oldFont)
            SelectFont(hdc, oldFont);
    }
}

#define PH_GRAPH_SMOOTH_SUBSAMPLES 4
#define PH_GRAPH_SMOOTH_FILL_TOP 1.0f
#define PH_GRAPH_SMOOTH_FILL_BOTTOM 0.30f // fill opacity at the bottom of the graph
#define PH_GRAPH_SMOOTH_GRID_UNDER_FILL 0.35f

// Rec. 601 luma of a bits-format (0x00RRGGBB) color.
FORCEINLINE BOOLEAN PhpIsLightGraphColor(
    _In_ ULONG Color
    )
{
    return ((Color >> 16) & 0xff) * 299 + ((Color >> 8) & 0xff) * 587 + (Color & 0xff) * 114 >= 128 * 1000;
}

// Fill opacity for row y (0 is the bottom), from FillBottom up to the top opacity. Curve keeps
// the upper part stronger and drops off lower down (dark backgrounds); otherwise linear.
FORCEINLINE FLOAT PhpGraphSmoothFillOpacity(
    _In_ LONG y,
    _In_ LONG Height,
    _In_ FLOAT FillBottom,
    _In_ BOOLEAN Curve
    )
{
    FLOAT t = ((FLOAT)y + 0.5f) / (FLOAT)Height;

    return FillBottom + (PH_GRAPH_SMOOTH_FILL_TOP - FillBottom) * (Curve ? t * sqrtf(t) : t);
}

typedef struct _PHP_GRAPH_SMOOTH_CONTEXT
{
    LONG Width;
    LONG Height;
    LONG PointCount;
    FLOAT Origin; // x of data point 0
    PFLOAT Y1;
    PFLOAT Y2;
} PHP_GRAPH_SMOOTH_CONTEXT, *PPHP_GRAPH_SMOOTH_CONTEXT;

FORCEINLINE ULONG PhpGraphCoverageToWeight(
    _In_ FLOAT Coverage
    )
{
    if (Coverage <= 0.0f)
        return 0;
    if (Coverage >= 1.0f)
        return 256;

    return (ULONG)(Coverage * 256.0f + 0.5f);
}

FORCEINLINE ULONG PhpBlendGraphColor(
    _In_ ULONG Destination,
    _In_ ULONG Source,
    _In_ ULONG Weight
    )
{
    ULONG inverse = 256 - Weight;
    ULONG rb;
    ULONG g;

    rb = ((Destination & 0x00ff00ff) * inverse + (Source & 0x00ff00ff) * Weight) >> 8;
    g = ((Destination & 0x0000ff00) * inverse + (Source & 0x0000ff00) * Weight) >> 8;

    return (rb & 0x00ff00ff) | (g & 0x0000ff00);
}

FORCEINLINE ULONG PhpBlendGraphColor3(
    _In_ ULONG Color0,
    _In_ ULONG Color1,
    _In_ ULONG Weight1,
    _In_ ULONG Color2,
    _In_ ULONG Weight2
    )
{
    ULONG weight0 = 256 - Weight1 - Weight2;
    ULONG rb;
    ULONG g;

    rb = ((Color0 & 0x00ff00ff) * weight0 + (Color1 & 0x00ff00ff) * Weight1 + (Color2 & 0x00ff00ff) * Weight2) >> 8;
    g = ((Color0 & 0x0000ff00) * weight0 + (Color1 & 0x0000ff00) * Weight1 + (Color2 & 0x0000ff00) * Weight2) >> 8;

    return (rb & 0x00ff00ff) | (g & 0x0000ff00);
}

// Grid lines are blended in linear light: a line moving between two pixels then emits the same
// light as when it covers one, so its brightness doesn't pulse as the graph scrolls.
static FLOAT PhpGraphSrgbToLinear[256];
static UCHAR PhpGraphLinearToSrgb[4096];

static VOID PhpInitializeGraphLinearTables(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        for (ULONG i = 0; i < 256; i++)
        {
            FLOAT value = (FLOAT)i / 255.0f;

            PhpGraphSrgbToLinear[i] = value <= 0.04045f ? value / 12.92f : powf((value + 0.055f) / 1.055f, 2.4f);
        }

        for (ULONG i = 0; i < 4096; i++)
        {
            FLOAT value = (FLOAT)i / 4095.0f;
            FLOAT srgb = value <= 0.0031308f ? value * 12.92f : 1.055f * powf(value, 1.0f / 2.4f) - 0.055f;

            PhpGraphLinearToSrgb[i] = (UCHAR)(srgb * 255.0f + 0.5f);
        }

        PhEndInitOnce(&initOnce);
    }
}

FORCEINLINE ULONG PhpBlendGraphColorLinear(
    _In_ ULONG Destination,
    _In_ ULONG Source,
    _In_ FLOAT Coverage
    )
{
    ULONG result = 0;

    if (Coverage <= 0)
        return Destination;
    if (Coverage >= 1)
        return Source;

    for (ULONG shift = 0; shift <= 16; shift += 8)
    {
        FLOAT destination = PhpGraphSrgbToLinear[(Destination >> shift) & 0xff];
        FLOAT source = PhpGraphSrgbToLinear[(Source >> shift) & 0xff];

        result |= (ULONG)PhpGraphLinearToSrgb[(ULONG)((destination + (source - destination) * Coverage) * 4095.0f + 0.5f)] << shift;
    }

    return result;
}
// Mixes Weight of Color2 into Color1.
static COLORREF PhpMixGraphColor(
    _In_ COLORREF Color1,
    _In_ COLORREF Color2,
    _In_ FLOAT Weight
    )
{
    return COLORREF_TO_BITS(PhpBlendGraphColor(
        COLORREF_TO_BITS(Color1),
        COLORREF_TO_BITS(Color2),
        PhpGraphCoverageToWeight(Weight)
        )); // the byte swap is its own inverse
}

// Anti-aliased rounded rectangle with a one pixel inner border; coordinates are top-down.
static VOID PhpDrawGraphRoundRect(
    _In_ PPHP_GRAPH_SURFACE Surface,
    _In_ FLOAT Left,
    _In_ FLOAT Top,
    _In_ FLOAT Right,
    _In_ FLOAT Bottom,
    _In_ FLOAT Radius,
    _In_ COLORREF FillColor,
    _In_ FLOAT FillOpacity,
    _In_ COLORREF BorderColor,
    _In_ FLOAT BorderOpacity
    )
{
    FLOAT centerX = (Left + Right) * 0.5f;
    FLOAT centerY = (Top + Bottom) * 0.5f;
    FLOAT halfX;
    FLOAT halfY;
    ULONG fillColor = COLORREF_TO_BITS(FillColor);
    ULONG borderColor = COLORREF_TO_BITS(BorderColor);
    LONG left = max((LONG)floorf(Left), 0);
    LONG top = max((LONG)floorf(Top), 0);
    LONG right = min((LONG)ceilf(Right), Surface->Width);
    LONG bottom = min((LONG)ceilf(Bottom), Surface->Height);

    Radius = max(min(Radius, min(Right - Left, Bottom - Top) * 0.5f), 0.0f);
    halfX = (Right - Left) * 0.5f - Radius;
    halfY = (Bottom - Top) * 0.5f - Radius;

    // GDI batches drawing; flush it before writing to the bits.
    GdiFlush();

    for (LONG y = top; y < bottom; y++)
    {
        PULONG row = Surface->Bits + (Surface->BottomUp ? Surface->Height - 1 - y : y) * Surface->Width;
        FLOAT qy = fabsf((FLOAT)y + 0.5f - centerY) - halfY;

        for (LONG x = left; x < right; x++)
        {
            FLOAT qx = fabsf((FLOAT)x + 0.5f - centerX) - halfX;
            FLOAT outsideX = max(qx, 0.0f);
            FLOAT outsideY = max(qy, 0.0f);
            FLOAT distance = sqrtf(outsideX * outsideX + outsideY * outsideY) + min(max(qx, qy), 0.0f) - Radius;
            FLOAT outer = max(min(0.5f - distance, 1.0f), 0.0f);
            FLOAT inner;
            ULONG color;

            if (outer <= 0)
                continue;

            inner = max(min(-0.5f - distance, 1.0f), 0.0f);
            color = PhpBlendGraphColor(row[x], fillColor, PhpGraphCoverageToWeight(outer * FillOpacity));

            if (BorderOpacity > 0)
                color = PhpBlendGraphColor(color, borderColor, PhpGraphCoverageToWeight((outer - inner) * BorderOpacity));

            row[x] = color;
        }
    }
}

// Text drawn at a fractional position: GDI renders it white on black at PH_GRAPH_TEXT_MASK_SCALE
// times the size, and each surface pixel takes the average of the mask area it covers. The box
// filter is the same at every position, so moving text keeps the same sharpness.
static BOOLEAN PhpCreateGraphTextMask(
    _In_ HDC hdc,
    _In_ PPH_STRING Text,
    _Out_ PPHP_GRAPH_TEXT_MASK Mask
    )
{
    LOGFONT logFont;
    SIZE textSize;

    memset(Mask, 0, sizeof(PHP_GRAPH_TEXT_MASK));

    if (!GetObject(GetCurrentObject(hdc, OBJ_FONT), sizeof(LOGFONT), &logFont))
        return FALSE;

    logFont.lfHeight *= PH_GRAPH_TEXT_MASK_SCALE;
    logFont.lfWidth *= PH_GRAPH_TEXT_MASK_SCALE;
    logFont.lfQuality = ANTIALIASED_QUALITY;

    if (!(Mask->Font = CreateFontIndirect(&logFont)))
        return FALSE;

    if (!(Mask->Context = CreateCompatibleDC(hdc)))
        goto CleanupExit;

    Mask->OldFont = SelectFont(Mask->Context, Mask->Font);

    if (!GetTextExtentPoint32(Mask->Context, Text->Buffer, (ULONG)Text->Length / sizeof(WCHAR), &textSize))
        goto CleanupExit;

    // One surface pixel of margin on each side.
    Mask->Width = textSize.cx + PH_GRAPH_TEXT_MASK_SCALE * 2;
    Mask->Height = textSize.cy + PH_GRAPH_TEXT_MASK_SCALE * 2;
    Mask->TextWidth = (FLOAT)textSize.cx / PH_GRAPH_TEXT_MASK_SCALE;
    Mask->TextHeight = (FLOAT)textSize.cy / PH_GRAPH_TEXT_MASK_SCALE;

    if (!(Mask->Bitmap = PhCreateDIBSection(Mask->Context, PHBF_TOPDOWNDIB, Mask->Width, Mask->Height, &Mask->Bits)))
        goto CleanupExit;

    Mask->OldBitmap = SelectBitmap(Mask->Context, Mask->Bitmap);
    memset(Mask->Bits, 0, Mask->Width * Mask->Height * sizeof(ULONG));
    SetTextColor(Mask->Context, RGB(0xff, 0xff, 0xff));
    SetBkMode(Mask->Context, TRANSPARENT);
    TextOut(Mask->Context, PH_GRAPH_TEXT_MASK_SCALE, PH_GRAPH_TEXT_MASK_SCALE, Text->Buffer, (ULONG)Text->Length / sizeof(WCHAR));
    GdiFlush();

    // Only the bits are needed from here on.
    SelectBitmap(Mask->Context, Mask->OldBitmap);
    SelectFont(Mask->Context, Mask->OldFont);
    DeleteDC(Mask->Context);
    DeleteFont(Mask->Font);
    Mask->Context = NULL;
    Mask->OldBitmap = NULL;
    Mask->OldFont = NULL;
    Mask->Font = NULL;

    return TRUE;

CleanupExit:
    PhpDeleteGraphTextMask(Mask);
    return FALSE;
}

static VOID PhpDeleteGraphTextMask(
    _Inout_ PPHP_GRAPH_TEXT_MASK Mask
    )
{
    if (Mask->Context)
    {
        if (Mask->OldBitmap)
            SelectBitmap(Mask->Context, Mask->OldBitmap);
        if (Mask->OldFont)
            SelectFont(Mask->Context, Mask->OldFont);

        DeleteDC(Mask->Context);
    }

    if (Mask->Bitmap)
        DeleteBitmap(Mask->Bitmap);
    if (Mask->Font)
        DeleteFont(Mask->Font);

    memset(Mask, 0, sizeof(PHP_GRAPH_TEXT_MASK));
}

// Left and Top are the position of the text (not the margin) on the surface.
static VOID PhpDrawGraphTextMask(
    _In_ PPHP_GRAPH_SURFACE Surface,
    _In_ PPHP_GRAPH_TEXT_MASK Mask,
    _In_ FLOAT Left,
    _In_ FLOAT Top,
    _In_ COLORREF Color
    )
{
    const FLOAT scale = PH_GRAPH_TEXT_MASK_SCALE;
    ULONG color = COLORREF_TO_BITS(Color);
    FLOAT originX = Left - 1.0f; // surface position of mask pixel (0, 0)
    FLOAT originY = Top - 1.0f;
    LONG firstX = max((LONG)floorf(originX), 0);
    LONG firstY = max((LONG)floorf(originY), 0);
    LONG lastX = min((LONG)ceilf(originX + Mask->Width / scale), Surface->Width - 1);
    LONG lastY = min((LONG)ceilf(originY + Mask->Height / scale), Surface->Height - 1);

    for (LONG y = firstY; y <= lastY; y++)
    {
        PULONG row = Surface->Bits + (Surface->BottomUp ? Surface->Height - 1 - y : y) * Surface->Width;
        FLOAT maskTop = ((FLOAT)y - originY) * scale; // mask rows covered by this surface row
        FLOAT maskBottom = maskTop + scale;
        LONG maskFirstY = max((LONG)floorf(maskTop), 0);
        LONG maskLastY = min((LONG)ceilf(maskBottom) - 1, Mask->Height - 1);

        for (LONG x = firstX; x <= lastX; x++)
        {
            FLOAT maskLeft = ((FLOAT)x - originX) * scale;
            FLOAT maskRight = maskLeft + scale;
            LONG maskFirstX = max((LONG)floorf(maskLeft), 0);
            LONG maskLastX = min((LONG)ceilf(maskRight) - 1, Mask->Width - 1);
            FLOAT coverage = 0;

            for (LONG my = maskFirstY; my <= maskLastY; my++)
            {
                FLOAT weightY = min((FLOAT)(my + 1), maskBottom) - max((FLOAT)my, maskTop);
                PULONG maskRow = Mask->Bits + my * Mask->Width;

                for (LONG mx = maskFirstX; mx <= maskLastX; mx++)
                {
                    FLOAT weightX = min((FLOAT)(mx + 1), maskRight) - max((FLOAT)mx, maskLeft);

                    coverage += weightX * weightY * (FLOAT)((maskRow[mx] >> 8) & 0xff);
                }
            }

            coverage /= scale * scale * 255.0f;

            if (coverage > 0)
                row[x] = PhpBlendGraphColor(row[x], color, PhpGraphCoverageToWeight(coverage));
        }
    }
}

/**
 * Draws a smooth graph text box: a translucent rounded rectangle.
 *
 * \param hdc A DC with a 32bpp DIB section selected; other DCs get a plain filled rectangle.
 * \param Rect The box, in the DC's coordinates.
 * \param FillColor The fill color.
 * \param BorderColor The border color.
 */
// Gets the 32bpp DIB section selected into a DC, and converts Rect from the DC's logical
// coordinates to bitmap coordinates (buffered paint DCs can have a moved origin).
static BOOLEAN PhpGetGraphSurfaceFromDC(
    _In_ HDC hdc,
    _Out_ PPHP_GRAPH_SURFACE Surface,
    _Inout_ PRECT Rect
    )
{
    DIBSECTION section;
    HBITMAP bitmap;
    POINT windowOrigin;
    POINT viewportOrigin;

    if (!(
        (bitmap = GetCurrentObject(hdc, OBJ_BITMAP)) &&
        GetObject(bitmap, sizeof(DIBSECTION), &section) == sizeof(DIBSECTION) &&
        section.dsBm.bmBitsPixel == 32 &&
        section.dsBm.bmBits &&
        GetWindowOrgEx(hdc, &windowOrigin) &&
        GetViewportOrgEx(hdc, &viewportOrigin)
        ))
    {
        return FALSE;
    }

    Surface->Bits = section.dsBm.bmBits;
    Surface->Width = section.dsBm.bmWidth;
    Surface->Height = section.dsBm.bmHeight;
    Surface->BottomUp = section.dsBmih.biHeight > 0;
    OffsetRect(Rect, viewportOrigin.x - windowOrigin.x, viewportOrigin.y - windowOrigin.y);

    return TRUE;
}

VOID PhDrawGraphTextBox(
    _In_ HDC hdc,
    _In_ PRECT Rect,
    _In_ COLORREF FillColor,
    _In_ COLORREF BorderColor
    )
{
    PHP_GRAPH_SURFACE surface;
    RECT rect = *Rect;

    if (PhpGetGraphSurfaceFromDC(hdc, &surface, &rect))
    {
        PhpDrawGraphChip(&surface, &rect, FillColor, BorderColor);
    }
    else
    {
        SetDCBrushColor(hdc, FillColor);
        FillRect(hdc, Rect, PhGetStockBrush(DC_BRUSH));
    }
}

/**
 * Draws a horizontal stacked bar in the smooth graph style: each segment ends in a line on its
 * right edge, which also separates it from the next, and its fill fades to the left like the graph fills.
 *
 * \param hdc A DC with a 32bpp DIB section selected; other DCs get plain filled segments.
 * \param Rect The bar, in the DC's coordinates.
 * \param BackColor The background color, also used for the gaps.
 * \param Count The number of segments.
 * \param Fractions The width of each segment as a fraction of the bar.
 * \param LineColors The line color of each segment.
 * \param FillColors The fill color of each segment.
 */
VOID PhDrawGraphBar(
    _In_ HDC hdc,
    _In_ PRECT Rect,
    _In_ COLORREF BackColor,
    _In_ ULONG Count,
    _In_reads_(Count) PFLOAT Fractions,
    _In_reads_(Count) const COLORREF* LineColors,
    _In_reads_(Count) const COLORREF* FillColors
    )
{
    PHP_GRAPH_SURFACE surface;
    RECT rect = *Rect;
    LONG width = rect.right - rect.left;
    LONG height = rect.bottom - rect.top;
    ULONG backColor = COLORREF_TO_BITS(BackColor);
    BOOLEAN fadeCurve = !PhpIsLightGraphColor(backColor); // same fade as the graph fills
    FLOAT start = 0;

    if (width <= 0 || height <= 0)
        return;

    if (!PhpGetGraphSurfaceFromDC(hdc, &surface, &rect))
    {
        RECT fillRect = *Rect;

        SetDCBrushColor(hdc, BackColor);
        FillRect(hdc, Rect, PhGetStockBrush(DC_BRUSH));

        for (ULONG i = 0; i < Count; i++)
        {
            fillRect.left = Rect->left + (LONG)(start * width);
            start += Fractions[i];
            fillRect.right = Rect->left + (LONG)(start * width);
            SetDCBrushColor(hdc, FillColors[i]);
            FillRect(hdc, &fillRect, PhGetStockBrush(DC_BRUSH));
        }

        return;
    }

    GdiFlush();

    // Each segment ends in a line on its right edge and its fill fades out to the left, so the
    // colors are the same down every column: compute one row and copy it.
    {
        LONG first = max(rect.left, 0);
        LONG last = min(rect.right, surface.Width); // exclusive
        PULONG columns;

        if (last <= first || !(columns = PhAllocateStack((last - first) * sizeof(ULONG))))
            return;

        for (LONG x = first; x < last; x++)
            columns[x - first] = backColor;

        for (ULONG i = 0; i < Count; i++)
        {
            FLOAT left = (FLOAT)rect.left + start * width;
            FLOAT right;

            start += Fractions[i];
            right = (FLOAT)rect.left + start * width;

            if (right <= left)
                continue;

            for (LONG x = max((LONG)floorf(left), first); x < min((LONG)ceilf(right), last); x++)
            {
                FLOAT coverage = min((FLOAT)(x + 1), right) - max((FLOAT)x, left);
                FLOAT position = ((FLOAT)x + 0.5f - left) / (right - left); // 0 at the left, 1 at the right
                FLOAT shaped = fadeCurve ? position * sqrtf(max(position, 0.0f)) : position;
                FLOAT opacity = PH_GRAPH_SMOOTH_FILL_BOTTOM + (PH_GRAPH_SMOOTH_FILL_TOP - PH_GRAPH_SMOOTH_FILL_BOTTOM) * shaped;
                FLOAT stroke = min((FLOAT)(x + 1), right) - max((FLOAT)x, right - PH_GRAPH_BAR_LINE_WIDTH); // line coverage
                ULONG color;

                if (coverage <= 0)
                    continue;

                color = PhpBlendGraphColor(columns[x - first], COLORREF_TO_BITS(FillColors[i]), PhpGraphCoverageToWeight(coverage * opacity));

                if (stroke > 0)
                    color = PhpBlendGraphColor(color, COLORREF_TO_BITS(LineColors[i]), PhpGraphCoverageToWeight(min(stroke, 1.0f)));

                columns[x - first] = color;
            }
        }

        for (LONG y = max(rect.top, 0); y < min(rect.bottom, surface.Height); y++)
        {
            PULONG row = surface.Bits + (surface.BottomUp ? surface.Height - 1 - y : y) * surface.Width;

            memcpy(row + first, columns, (last - first) * sizeof(ULONG));
        }

        PhFreeStack(columns);
    }
}
// Data point i is centered on column (Width - 1 - 2 * i), the same as the classic renderer,
// shifted right by the animation offset.
FORCEINLINE FLOAT PhpGraphSmoothPointX(
    _In_ PPHP_GRAPH_SMOOTH_CONTEXT Context,
    _In_ LONG Index
    )
{
    return Context->Origin - (FLOAT)Index * 2.0f;
}

FORCEINLINE FLOAT PhpGraphSmoothCurveY(
    _In_ PPHP_GRAPH_SMOOTH_CONTEXT Context,
    _In_ PFLOAT Points,
    _In_ FLOAT X
    )
{
    FLOAT t;
    LONG index;

    t = (Context->Origin - X) * 0.5f;

    if (t <= 0.0f)
        return Points[0];

    index = (LONG)t;

    if (index + 1 >= Context->PointCount)
        return Points[Context->PointCount - 1];

    t -= (FLOAT)index;

    return Points[index] + (Points[index + 1] - Points[index]) * t;
}

static FLOAT PhpGraphSmoothStrokeCoverage(
    _In_ PPHP_GRAPH_SMOOTH_CONTEXT Context,
    _In_ PFLOAT Points,
    _In_ LONG FirstSegment,
    _In_ LONG LastSegment,
    _In_ FLOAT X,
    _In_ FLOAT Y,
    _In_ FLOAT HalfWidth
    )
{
    FLOAT reach = HalfWidth + 0.5f; // beyond this distance the coverage is zero
    FLOAT distanceSquared = reach * reach;

    for (LONG i = FirstSegment; i <= LastSegment; i++)
    {
        FLOAT ax = PhpGraphSmoothPointX(Context, i);
        FLOAT ay = Points[i];
        FLOAT by = Points[i + 1];
        FLOAT dx = -2.0f;
        FLOAT dy = by - ay;
        FLOAT t;
        FLOAT px;
        FLOAT py;

        if (Y < min(ay, by) - reach || Y > max(ay, by) + reach)
            continue;

        t = ((X - ax) * dx + (Y - ay) * dy) / (dx * dx + dy * dy);

        if (t < 0.0f)
            t = 0.0f;
        if (t > 1.0f)
            t = 1.0f;

        px = X - (ax + dx * t);
        py = Y - (ay + dy * t);

        if (distanceSquared > px * px + py * py)
            distanceSquared = px * px + py * py;
    }

    return reach - sqrtf(distanceSquared);
}

static VOID PhpGetGraphSmoothGridRows(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _Out_writes_(DrawInfo->Height) PBOOLEAN GridRows
    )
{
    LONG height = DrawInfo->Height;
    LONG gridYThreshold = DrawInfo->GridYThreshold;
    FLOAT gridHeight = max(DrawInfo->GridHeight, 0);
    FLOAT level;
    LONG h;
    LONG h_last;

    memset(GridRows, 0, height * sizeof(BOOLEAN));

    if (!(DrawInfo->Flags & PH_GRAPH_USE_GRID_Y) || gridHeight <= 0)
        return;

    if (DrawInfo->Flags & PH_GRAPH_LOGARITHMIC_GRID_Y)
    {
        FLOAT gridBase = DrawInfo->GridBase;

        if (gridBase <= 1)
            return;

        // Find the largest integer n such that GridHeight*GridBase^n < 1.
        level = (FLOAT)(gridHeight * exp((ceil(-log(gridHeight) / log(gridBase)) - 1) * log(gridBase)));

        if (level < 0 || !isfinite(level))
            level = 0;
        if (level > 1)
            level = 1;

        h = (LONG)(level * (height - 1));
        h_last = height + gridYThreshold - 1;

        while (h <= h_last - gridYThreshold)
        {
            if (h >= 0 && h < height)
                GridRows[h] = TRUE;

            h_last = h;
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
                GridRows[h] = TRUE;
                h_last = h;
            }

            level += gridHeight;
            h = (LONG)(level * (height - 1));
        }
    }
}

static VOID PhpGetGraphSmoothLabel(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _Out_ PULONG LabelMaxY,
    _Out_ PULONG LabelDataIndex
    )
{
    ULONG count = (DrawInfo->Width + 1) / 2; // data points visited by the classic renderer
    ULONG h1;
    ULONG h2;
    ULONG yLabelMax;
    ULONG yLabelDataIndex = 0;
#ifndef PH_GRAPH_LABEL_MAX_Y_PIXEL
    FLOAT yLabelValue;
#endif

    PhpGetGraphPoint(DrawInfo, 0, &h1, &h2);
    yLabelMax = h2;
#ifndef PH_GRAPH_LABEL_MAX_Y_PIXEL
    yLabelValue = PhpGetGraphLabelValue(DrawInfo, 0);
#endif

    for (ULONG i = 1; i <= count && i < DrawInfo->LabelMaxYIndexLimit; i++)
    {
        PhpGetGraphPoint(DrawInfo, i, &h1, &h2);
#ifndef PH_GRAPH_LABEL_MAX_Y_PIXEL
        FLOAT value = PhpGetGraphLabelValue(DrawInfo, i);

        if (yLabelValue < value)
        {
            yLabelValue = value;
            yLabelMax = h2;
            yLabelDataIndex = i;
        }
#else
        if (yLabelMax <= h2)
        {
            yLabelMax = h2;
            yLabelDataIndex = i;
        }
#endif
    }

    *LabelMaxY = yLabelMax;
    *LabelDataIndex = yLabelDataIndex;
}

// Returns TRUE when grid column Column (before the animation offset) has a vertical grid line.
FORCEINLINE BOOLEAN PhpIsGraphSmoothGridColumn(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ LONG FirstCounter,
    _In_ LONG Column
    )
{
    LONG counter = (FirstCounter + DrawInfo->Width - 1 - Column) % (LONG)DrawInfo->GridWidth;

    return counter == 0;
}

// Per-column data computed before the pixels are written.
typedef struct _PHP_GRAPH_SMOOTH_SAMPLES
{
    FLOAT Fill1[PH_GRAPH_SMOOTH_SUBSAMPLES]; // curve heights across the column
    FLOAT Fill2[PH_GRAPH_SMOOTH_SUBSAMPLES];
} PHP_GRAPH_SMOOTH_SAMPLES, *PPHP_GRAPH_SMOOTH_SAMPLES;

// Kept small: every pixel reads its column's entry. The curve samples (only needed near the
// curves) are in a separate array.
typedef struct _PHP_GRAPH_SMOOTH_COLUMN
{
    LONG FillLow; // rows below are fully covered by both fills
    LONG FillHigh; // rows above are not covered
    LONG Fill1High; // rows above are not covered by line 1's fill
    LONG Fill2Low; // rows below are fully covered by line 2's fill
    LONG Stroke1Low; // rows the outlines can touch
    LONG Stroke1High;
    LONG Stroke2Low;
    LONG Stroke2High;
    LONG FirstSegment;
    LONG LastSegment;
    FLOAT Grid; // vertical grid line coverage
} PHP_GRAPH_SMOOTH_COLUMN, *PPHP_GRAPH_SMOOTH_COLUMN;

// Returns FALSE when the scratch buffer can't be allocated.
static BOOLEAN PhpDrawGraphSmooth(
    _In_ PVOID PH_RESTRICT Bits,
    _In_ PPH_GRAPH_DRAW_INFO PH_RESTRICT DrawInfo,
    _In_ FLOAT Offset,
    _Out_ PULONG LabelMaxY,
    _Out_ PULONG LabelDataIndex
    )
{
    SIZE_T bufferSize;
    LONG offsetColumns = (LONG)Offset;
    FLOAT offsetFraction = Offset - (FLOAT)offsetColumns;
    PULONG PH_RESTRICT bits = Bits;
    LONG width = DrawInfo->Width;
    LONG height = DrawInfo->Height;
    ULONG flags = DrawInfo->Flags;
    BOOLEAN useLine2 = !!(flags & PH_GRAPH_USE_LINE_2);
    PHP_GRAPH_SMOOTH_CONTEXT context;
    PVOID buffer;
    PPHP_GRAPH_SMOOTH_COLUMN columns;
    PPHP_GRAPH_SMOOTH_SAMPLES samples;
    PBOOLEAN gridRows;
    PULONG filledRows; // fill color of each row where line 1 covers it completely
    PULONG filledRows2; // fill color of each row where only line 2 covers it, completely
    PFLOAT opacityRows; // fill opacity of each row
    ULONG backColor;
    ULONG lineColor1;
    ULONG lineColor2;
    ULONG lineBackColor1;
    ULONG lineBackColor2;
    ULONG gridColor;
    FLOAT halfWidth;
    FLOAT fillBottom;
    FLOAT gridUnderFill;
    BOOLEAN fillCurve;
    LONG gridXCounter = 0;

    *LabelMaxY = 0;
    *LabelDataIndex = 0;

    if (width <= 0 || height <= 0)
        return TRUE;

    PhpInitializeGraphLinearTables();

    context.Width = width;
    context.Height = height;
    context.PointCount = width / 2 + 4;
    context.Origin = (FLOAT)width - 0.5f + Offset;

    bufferSize =
        width * (sizeof(PHP_GRAPH_SMOOTH_COLUMN) + sizeof(PHP_GRAPH_SMOOTH_SAMPLES)) +
        context.PointCount * sizeof(FLOAT) * 2 +
        height * (sizeof(ULONG) * 2 + sizeof(FLOAT) + sizeof(BOOLEAN));
    buffer = PhAllocateStack(bufferSize);

    if (!buffer)
        return FALSE;

    samples = buffer;
    columns = (PPHP_GRAPH_SMOOTH_COLUMN)(samples + width);
    context.Y1 = (PFLOAT)(columns + width);
    context.Y2 = context.Y1 + context.PointCount;
    filledRows = (PULONG)(context.Y2 + context.PointCount);
    filledRows2 = filledRows + height;
    opacityRows = (PFLOAT)(filledRows2 + height);
    gridRows = (PBOOLEAN)(opacityRows + height);

    for (LONG i = 0; i < context.PointCount; i++)
    {
        FLOAT f1 = 0;
        FLOAT f2 = 0;

        if ((ULONG)i < DrawInfo->LineDataCount)
        {
            f1 = DrawInfo->LineData1[i];

            if (useLine2)
                f2 = f1 + DrawInfo->LineData2[i];

            if (!(f1 >= 0)) // also catches NaN
                f1 = 0;
            if (f1 > 1)
                f1 = 1;
            if (!(f2 >= f1)) // line 2 is stacked on line 1
                f2 = f1;
            if (f2 > 1)
                f2 = 1;
        }

        context.Y1[i] = f1 * (height - 1) + 0.5f;
        context.Y2[i] = useLine2 ? f2 * (height - 1) + 0.5f : context.Y1[i];
    }

    PhpGetGraphSmoothGridRows(DrawInfo, gridRows);

    if ((flags & PH_GRAPH_USE_GRID_X) && DrawInfo->GridWidth)
        gridXCounter = DrawInfo->GridWidth - (DrawInfo->GridXOffset * DrawInfo->Step) % DrawInfo->GridWidth - 1;

    backColor = COLORREF_TO_BITS(DrawInfo->BackColor);
    lineColor1 = COLORREF_TO_BITS(DrawInfo->LineColor1);
    lineColor2 = COLORREF_TO_BITS(DrawInfo->LineColor2);
    lineBackColor1 = COLORREF_TO_BITS(DrawInfo->LineBackColor1);
    lineBackColor2 = COLORREF_TO_BITS(DrawInfo->LineBackColor2);
    gridColor = COLORREF_TO_BITS(DrawInfo->GridColor);
    gridUnderFill = 1.0f - (1.0f - PH_GRAPH_SMOOTH_GRID_UNDER_FILL) * 1.0f; // grid weight where the fill is complete

    // The fill fades linearly on light backgrounds and along a curve on dark ones. Small graphs
    // (tray icons, tree cells) keep a one pixel stroke.
    fillCurve = !PhpIsLightGraphColor(backColor);
    fillBottom = PH_GRAPH_SMOOTH_FILL_BOTTOM;
    halfWidth = height < 40 ? 0.5f : 0.75f;

    for (LONG y = 0; y < height; y++)
    {
        FLOAT opacity = PhpGraphSmoothFillOpacity(y, height, fillBottom, fillCurve);

        opacityRows[y] = opacity;
        filledRows[y] = PhpBlendGraphColor(backColor, lineBackColor1, PhpGraphCoverageToWeight(opacity));
        filledRows2[y] = PhpBlendGraphColor3(backColor, lineBackColor1, 0, lineBackColor2, min(PhpGraphCoverageToWeight(opacity), 256));
    }

    // Pass 1: the curve and grid data of each column.

    for (LONG x = 0; x < width; x++)
    {
        PPHP_GRAPH_SMOOTH_COLUMN column = &columns[x];
        FLOAT fillMin = (FLOAT)height;
        FLOAT fillMax = 0;
        FLOAT fill1Max = 0;
        FLOAT fill2Min = (FLOAT)height;
        FLOAT stroke1Min = (FLOAT)height;
        FLOAT stroke1Max = 0;
        FLOAT stroke2Min = (FLOAT)height;
        FLOAT stroke2Max = 0;
        FLOAT centerX = (FLOAT)x + 0.5f;

        for (ULONG s = 0; s < PH_GRAPH_SMOOTH_SUBSAMPLES; s++)
        {
            FLOAT sampleX = (FLOAT)x + ((FLOAT)s + 0.5f) / PH_GRAPH_SMOOTH_SUBSAMPLES;

            samples[x].Fill1[s] = PhpGraphSmoothCurveY(&context, context.Y1, sampleX);
            samples[x].Fill2[s] = useLine2 ? PhpGraphSmoothCurveY(&context, context.Y2, sampleX) : samples[x].Fill1[s];
            fillMin = min(fillMin, samples[x].Fill1[s]);
            fillMax = max(fillMax, samples[x].Fill2[s]);
            fill1Max = max(fill1Max, samples[x].Fill1[s]);
            fill2Min = min(fill2Min, samples[x].Fill2[s]);
        }

        // The outlines reach this column's pixel centers only from the part of the curve within
        // reach horizontally: those segments, and the rows that part of the curve spans.
        {
            FLOAT reach = halfWidth + 0.5f;
            FLOAT left = centerX - reach;
            FLOAT right = centerX + reach;
            LONG firstVertex;
            LONG lastVertex;

            column->FirstSegment = max((LONG)floorf((context.Origin - right) * 0.5f), 0);
            column->LastSegment = min((LONG)floorf((context.Origin - left) * 0.5f), context.PointCount - 2);

            stroke1Min = min(PhpGraphSmoothCurveY(&context, context.Y1, left), PhpGraphSmoothCurveY(&context, context.Y1, right));
            stroke1Max = max(PhpGraphSmoothCurveY(&context, context.Y1, left), PhpGraphSmoothCurveY(&context, context.Y1, right));
            stroke2Min = min(PhpGraphSmoothCurveY(&context, context.Y2, left), PhpGraphSmoothCurveY(&context, context.Y2, right));
            stroke2Max = max(PhpGraphSmoothCurveY(&context, context.Y2, left), PhpGraphSmoothCurveY(&context, context.Y2, right));

            firstVertex = max((LONG)ceilf((context.Origin - right) * 0.5f), 0);
            lastVertex = min((LONG)floorf((context.Origin - left) * 0.5f), context.PointCount - 1);

            for (LONG i = firstVertex; i <= lastVertex; i++)
            {
                stroke1Min = min(stroke1Min, context.Y1[i]);
                stroke1Max = max(stroke1Max, context.Y1[i]);
                stroke2Min = min(stroke2Min, context.Y2[i]);
                stroke2Max = max(stroke2Max, context.Y2[i]);
            }

            if (column->FirstSegment > column->LastSegment)
            {
                // Right of the newest sample: nothing to draw.
                column->Stroke1Low = MAXLONG;
                column->Stroke1High = -1;
                column->Stroke2Low = MAXLONG;
                column->Stroke2High = -1;
            }
            else
            {
                column->Stroke1Low = max((LONG)floorf(stroke1Min - reach - 0.5f), 0);
                column->Stroke1High = min((LONG)ceilf(stroke1Max + reach - 0.5f), height - 1);
                column->Stroke2Low = useLine2 ? max((LONG)floorf(stroke2Min - reach - 0.5f), 0) : MAXLONG;
                column->Stroke2High = useLine2 ? min((LONG)ceilf(stroke2Max + reach - 0.5f), height - 1) : -1;
            }
        }

        column->FillLow = (LONG)fillMin;
        column->FillHigh = min((LONG)fillMax, height - 1);
        column->Fill1High = (LONG)fill1Max;
        column->Fill2Low = (LONG)fill2Min;
        column->Grid = 0;

        // A grid line at column c is drawn at c + Offset, split between two columns (blended in
        // linear light so the split line is as bright as a whole one).
        if ((flags & PH_GRAPH_USE_GRID_X) && DrawInfo->GridWidth)
        {
            if (PhpIsGraphSmoothGridColumn(DrawInfo, gridXCounter, x - offsetColumns))
                column->Grid += 1.0f - offsetFraction;
            if (PhpIsGraphSmoothGridColumn(DrawInfo, gridXCounter, x - offsetColumns - 1))
                column->Grid += offsetFraction;
        }
    }

    // Pass 2: the pixels, row by row. Pixels fully inside or fully outside the fill and away
    // from the outlines only need the row color and the grid.

    for (LONG y = 0; y < height; y++)
    {
        PULONG PH_RESTRICT row = bits + y * width;
        BOOLEAN gridRow = gridRows[y];
        ULONG filledColor = filledRows[y];
        ULONG filledRowColor = gridRow ? PhpBlendGraphColorLinear(filledColor, gridColor, gridUnderFill) : filledColor;
        ULONG backRowColor = gridRow ? gridColor : backColor;

        for (LONG x = 0; x < width; x++)
        {
            PPHP_GRAPH_SMOOTH_COLUMN column = &columns[x];
            FLOAT coverage1;
            FLOAT coverage2;
            FLOAT opacity;
            ULONG color;

            if (
                (y < column->Stroke1Low || y > column->Stroke1High) &&
                (y < column->Stroke2Low || y > column->Stroke2High)
                )
            {
                if (y < column->FillLow)
                {
                    row[x] = (gridRow || column->Grid <= 0) ? filledRowColor :
                        PhpBlendGraphColorLinear(filledColor, gridColor, column->Grid * gridUnderFill);
                    continue;
                }

                if (y > column->FillHigh)
                {
                    row[x] = (gridRow || column->Grid <= 0) ? backRowColor :
                        PhpBlendGraphColorLinear(backColor, gridColor, column->Grid);
                    continue;
                }

                // Between the curves: only line 2's fill, complete.
                if (y > column->Fill1High && y < column->Fill2Low)
                {
                    if (!gridRow && column->Grid <= 0)
                        row[x] = filledRows2[y];
                    else
                        row[x] = PhpBlendGraphColorLinear(filledRows2[y], gridColor, (gridRow ? 1.0f : column->Grid) * gridUnderFill);

                    continue;
                }
            }

            if (y < column->FillLow)
            {
                coverage1 = 1;
                coverage2 = 1;
            }
            else if (y > column->FillHigh)
            {
                coverage1 = 0;
                coverage2 = 0;
            }
            else
            {
                coverage1 = 0;
                coverage2 = 0;

                for (ULONG s = 0; s < PH_GRAPH_SMOOTH_SUBSAMPLES; s++)
                {
                    coverage1 += max(min(samples[x].Fill1[s] - (FLOAT)y, 1.0f), 0.0f);
                    coverage2 += max(min(samples[x].Fill2[s] - (FLOAT)y, 1.0f), 0.0f);
                }

                coverage1 /= PH_GRAPH_SMOOTH_SUBSAMPLES;
                coverage2 /= PH_GRAPH_SMOOTH_SUBSAMPLES;
            }

            // Fill: line 1 below the first curve, line 2 between the curves, with a vertical gradient.

            if (coverage1 >= 1)
            {
                color = filledColor;
            }
            else if (coverage2 > 0)
            {
                ULONG weight1;
                ULONG weight2;

                opacity = opacityRows[y];
                weight1 = PhpGraphCoverageToWeight(coverage1 * opacity);
                weight2 = min(PhpGraphCoverageToWeight(coverage2 * opacity) - weight1, 256 - weight1);
                color = PhpBlendGraphColor3(backColor, lineBackColor1, weight1, lineBackColor2, weight2);
            }
            else
            {
                color = backColor;
            }

            // Grid: full strength on the background, softened where it crosses the fill.

            if (column->Grid > 0 || gridRow)
            {
                color = PhpBlendGraphColorLinear(
                    color,
                    gridColor,
                    (gridRow ? 1.0f : column->Grid) * (1.0f - (1.0f - PH_GRAPH_SMOOTH_GRID_UNDER_FILL) * coverage2)
                    );
            }

            // Outline (line 1 is allowed to paint over line 2).

            if (y >= column->Stroke2Low && y <= column->Stroke2High)
            {
                color = PhpBlendGraphColor(color, lineColor2, PhpGraphCoverageToWeight(
                    PhpGraphSmoothStrokeCoverage(&context, context.Y2, column->FirstSegment, column->LastSegment, (FLOAT)x + 0.5f, (FLOAT)y + 0.5f, halfWidth)
                    ));
            }

            if (y >= column->Stroke1Low && y <= column->Stroke1High)
            {
                color = PhpBlendGraphColor(color, lineColor1, PhpGraphCoverageToWeight(
                    PhpGraphSmoothStrokeCoverage(&context, context.Y1, column->FirstSegment, column->LastSegment, (FLOAT)x + 0.5f, (FLOAT)y + 0.5f, halfWidth)
                    ));
            }

            row[x] = color;
        }
    }

    PhFreeStack(buffer);

    if (flags & PH_GRAPH_LABEL_MAX_Y)
        PhpGetGraphSmoothLabel(DrawInfo, LabelMaxY, LabelDataIndex);

    return TRUE;
}

static VOID PhpDrawGraphDirect(
    _In_ HDC hdc,
    _In_ PVOID PH_RESTRICT Bits,
    _In_ PPH_GRAPH_DRAW_INFO PH_RESTRICT DrawInfo,
    _In_ FLOAT Offset,
    _In_opt_ PPH_GRAPH_DRAW_INFO ValueDrawInfo,
    _Inout_opt_ PPHP_GRAPH_LABEL_CACHE LabelCache
    );

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
    _In_ PVOID PH_RESTRICT Bits,
    _In_ PPH_GRAPH_DRAW_INFO PH_RESTRICT DrawInfo
    )
{
    PhpDrawGraphDirect(hdc, Bits, DrawInfo, 0, NULL, NULL);
}

/**
 * Draws a graph directly to memory, shifted right.
 *
 * \param hdc The DC to draw to. This is only used when drawing text.
 * \param Bits The bits in a bitmap.
 * \param DrawInfo A structure which contains graphing information.
 * \param Offset The shift in pixels, from PhGetGraphScrollOffset. The classic renderer ignores it.
 */
VOID PhDrawGraphDirectEx(
    _In_ HDC hdc,
    _In_ PVOID PH_RESTRICT Bits,
    _In_ PPH_GRAPH_DRAW_INFO PH_RESTRICT DrawInfo,
    _In_ FLOAT Offset
    )
{
    PhpDrawGraphDirect(hdc, Bits, DrawInfo, Offset, NULL, NULL);
}

// The animation lasts until the next update is expected, so the scrolling never stops.
static ULONG PhpMeasureGraphAnimationDuration(
    _Inout_ PULONG64 LastStartTime,
    _In_ ULONG64 Now
    )
{
    ULONG duration = PH_GRAPH_ANIMATION_DEFAULT_DURATION;

    if (*LastStartTime && Now - *LastStartTime <= PH_GRAPH_ANIMATION_MAX_DURATION)
        duration = max((ULONG)(Now - *LastStartTime), PH_GRAPH_ANIMATION_MIN_DURATION);

    *LastStartTime = Now;

    return duration;
}

static BOOLEAN PhpGraphAnimationEnabled(
    VOID
    )
{
    BOOL clientAreaAnimation = TRUE;

    if (!PhEnableGraphSmoothing)
        return FALSE;
    if (SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &clientAreaAnimation, 0) && !clientAreaAnimation)
        return FALSE;

    return TRUE;
}

static ULONG PhpGraphAnimationFrameInterval(
    _In_ ULONG Duration
    )
{
    return min(max(Duration / PH_GRAPH_ANIMATION_FRAMES, PH_GRAPH_ANIMATION_MIN_FRAME_INTERVAL), PH_GRAPH_ANIMATION_MAX_FRAME_INTERVAL);
}

/**
 * Starts scrolling graphs that just gained a sample.
 *
 * \param State The scroll state shared by the graphs.
 *
 * \return The frame interval in milliseconds, or 0 when scrolling is disabled.
 */
ULONG PhStartGraphScroll(
    _Inout_ PPH_GRAPH_SCROLL_STATE State
    )
{
    ULONG64 now;

    if (!PhpGraphAnimationEnabled())
    {
        State->StartTime = 0;
        return 0;
    }

    now = NtGetTickCount64();
    State->Duration = PhpMeasureGraphAnimationDuration(&State->LastStartTime, now);
    State->StartTime = now;

    return PhpGraphAnimationFrameInterval(State->Duration);
}

/**
 * Gets the current offset of scrolling graphs.
 *
 * \param State The scroll state shared by the graphs.
 * \param Step The \a Step of the graphs.
 * \param Offset The offset to pass to PhDrawGraphDirectEx.
 *
 * \return TRUE while scrolling, FALSE once the scroll has finished.
 */
BOOLEAN PhGetGraphScrollOffset(
    _Inout_ PPH_GRAPH_SCROLL_STATE State,
    _In_ ULONG Step,
    _Out_ PFLOAT Offset
    )
{
    ULONG64 elapsed;

    *Offset = 0;

    if (!State->StartTime)
        return FALSE;

    elapsed = NtGetTickCount64() - State->StartTime;

    if (elapsed >= State->Duration)
    {
        State->StartTime = 0;
        return FALSE;
    }

    *Offset = (FLOAT)Step * (1.0f - (FLOAT)elapsed / State->Duration);

    return TRUE;
}

// Offset shifts the data and the vertical grid right (smooth renderer only). ValueDrawInfo holds
// the data that label values are read from when DrawInfo is an animation frame.
static VOID PhpDrawGraphDirect(
    _In_ HDC hdc,
    _In_ PVOID PH_RESTRICT Bits,
    _In_ PPH_GRAPH_DRAW_INFO PH_RESTRICT DrawInfo,
    _In_ FLOAT Offset,
    _In_opt_ PPH_GRAPH_DRAW_INFO ValueDrawInfo,
    _Inout_opt_ PPHP_GRAPH_LABEL_CACHE LabelCache
    )
{
    ULONG labelMaxY;
    ULONG labelDataIndex;

    if (PhEnableGraphSmoothing && PhpDrawGraphSmooth(Bits, DrawInfo, Offset, &labelMaxY, &labelDataIndex))
    {
        PHP_GRAPH_SURFACE surface;

        surface.Bits = Bits;
        surface.Width = DrawInfo->Width;
        surface.Height = DrawInfo->Height;
        surface.BottomUp = TRUE;
        PhpDrawGraphText(hdc, DrawInfo, &surface, Offset, ValueDrawInfo, LabelCache, labelMaxY, labelDataIndex);
    }
    else
    {
        PhpDrawGraphClassic(Bits, DrawInfo, &labelMaxY, &labelDataIndex);
        PhpDrawGraphText(hdc, DrawInfo, NULL, 0, NULL, NULL, labelMaxY, labelDataIndex);
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
    PhRectangleToRect(&DrawInfo->TextRect, &textRectangle);
    PhRectangleToRect(&DrawInfo->TextBoxRect, &boxRectangle);
}

VOID PhSetGraphText2(
    _In_ HDC hdc,
    _Inout_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ PPH_STRINGREF Text,
    _In_ PRECT Margin,
    _In_ PRECT Padding,
    _In_ ULONG Align
    )
{
    HFONT oldFont = NULL;
    RECT calcRect =
    {
        0,
        0,
        max(0L, DrawInfo->Width - Margin->left - Margin->right - Padding->left - Padding->right),
        DrawInfo->Height
    };
    UINT flags = DT_NOPREFIX | DT_WORDBREAK | DT_EDITCONTROL;
    PH_RECTANGLE boxRectangle;
    PH_RECTANGLE textRectangle;

    // Horizontal alignment
    if (FlagOn(Align, PH_ALIGN_LEFT))
        flags |= DT_LEFT;
    else if (FlagOn(Align, PH_ALIGN_RIGHT))
        flags |= DT_RIGHT;
    else
        flags |= DT_CENTER;

    // Vertical alignment (handled manually after measuring)
    // DT_TOP is default; DT_VCENTER/DT_BOTTOM don't work with CALCRECT
    flags |= DT_TOP;

    // Select font for measurement
    if (DrawInfo->TextFont)
        oldFont = SelectFont(hdc, DrawInfo->TextFont);

    // Measure multi-line text
    DrawInfo->Text2 = *Text;
    DrawText(
        hdc,
        Text->Buffer,
        (LONG)(Text->Length / sizeof(WCHAR)),
        &calcRect,
        flags | DT_CALCRECT
        );

    if (oldFont)
        SelectFont(hdc, oldFont);

    // Compute box size
    LONG textWidth = calcRect.right - calcRect.left;
    LONG textHeight = calcRect.bottom - calcRect.top;

    boxRectangle.Width = textWidth + Padding->left + Padding->right;
    boxRectangle.Height = textHeight + Padding->top + Padding->bottom;

    // Horizontal positioning
    if (FlagOn(Align, PH_ALIGN_LEFT))
        boxRectangle.Left = Margin->left;
    else if (FlagOn(Align, PH_ALIGN_RIGHT))
        boxRectangle.Left = DrawInfo->Width - boxRectangle.Width - Margin->right;
    else
        boxRectangle.Left = (DrawInfo->Width - boxRectangle.Width) / 2;

    // Vertical positioning
    if (FlagOn(Align, PH_ALIGN_TOP))
        boxRectangle.Top = Margin->top;
    else if (FlagOn(Align, PH_ALIGN_BOTTOM))
        boxRectangle.Top = DrawInfo->Height - boxRectangle.Height - Margin->bottom;
    else
        boxRectangle.Top = (DrawInfo->Height - boxRectangle.Height) / 2;

    // Compute final text rectangle
    textRectangle.Left = boxRectangle.Left + Padding->left;
    textRectangle.Top = boxRectangle.Top + Padding->top;
    textRectangle.Width = textWidth;
    textRectangle.Height = textHeight;

    // Save rectangles
    PhRectangleToRect(&DrawInfo->TextRect2, &textRectangle);
    PhRectangleToRect(&DrawInfo->TextBoxRect2, &boxRectangle);
}

PPHP_GRAPH_CONTEXT PhCreateGraphContext(
    VOID
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

    return context;
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
    if (Context->BufferedContext && Context->BufferedOldBitmap)
    {
        SelectBitmap(Context->BufferedContext, Context->BufferedOldBitmap);
        Context->BufferedOldBitmap = NULL;
    }

    if (Context->BufferedBitmap)
    {
        DeleteBitmap(Context->BufferedBitmap);
        Context->BufferedBitmap = NULL;
    }

    if (Context->BufferedContext)
    {
        DeleteDC(Context->BufferedContext);
        Context->BufferedContext = NULL;
    }

    Context->BufferedBits = NULL;
}

static VOID PhpCreateBufferedContext(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    PhpDeleteBufferedContext(Context);

    if (!PhGetClientRect(Context->Handle, &Context->BufferedContextRect))
        return;

    Context->BufferedContext = CreateCompatibleDC(NULL);

    if (!Context->BufferedContext)
        return;

    Context->BufferedBitmap = PhCreateDIBSection(
        Context->BufferedContext,
        PHBF_DIB,
        Context->BufferedContextRect.right,
        Context->BufferedContextRect.bottom,
        &Context->BufferedBits
        );

    if (!Context->BufferedBitmap)
        return;

    Context->BufferedOldBitmap = SelectBitmap(
        Context->BufferedContext,
        Context->BufferedBitmap
        );
}

static VOID PhpDeleteFadeOutContext(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    if (Context->FadeOutContext && Context->FadeOutOldBitmap)
    {
        SelectBitmap(Context->FadeOutContext, Context->FadeOutOldBitmap);
        Context->FadeOutOldBitmap = NULL;
    }

    if (Context->FadeOutBitmap)
    {
        DeleteBitmap(Context->FadeOutBitmap);
        Context->FadeOutBitmap = NULL;
    }

    if (Context->FadeOutContext)
    {
        DeleteDC(Context->FadeOutContext);
        Context->FadeOutContext = NULL;
    }

    Context->FadeOutBits = NULL;
}

static VOID PhpDeleteDrawInfoFonts(
    _Inout_ PPH_GRAPH_DRAW_INFO DrawInfo
    )
{
    HFONT textFont = DrawInfo->CachedTextFont;
    HFONT labelYFont = DrawInfo->CachedLabelYFont;

    if (DrawInfo->CachedTextFont)
    {
        DeleteFont(textFont);
        DrawInfo->CachedTextFont = NULL;
    }

    if (DrawInfo->CachedLabelYFont)
    {
        if (labelYFont != textFont)
            DeleteFont(labelYFont);

        DrawInfo->CachedLabelYFont = NULL;
    }

    DrawInfo->CachedFontDpi = 0;
}

static VOID PhpCreateFadeOutContext(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    ULONG i;
    ULONG j;
    ULONG height;
    COLORREF backColor;
    ULONG fadeOutWidth;
    FLOAT fadeOutWidthSquared;
    ULONG currentAlpha;
    ULONG currentColor;

    PhpDeleteFadeOutContext(Context);

    if (!PhGetClientRect(Context->Handle, &Context->FadeOutContextRect))
        return;
    Context->FadeOutContextRect.right = Context->Options.FadeOutWidth;

    Context->FadeOutContext = CreateCompatibleDC(NULL);

    if (!Context->FadeOutContext)
        return;

    Context->FadeOutBitmap = PhCreateDIBSection(
        Context->FadeOutContext,
        PHBF_DIB,
        Context->FadeOutContextRect.right,
        Context->FadeOutContextRect.bottom,
        &Context->FadeOutBits
        );

    if (!Context->FadeOutBitmap || !Context->FadeOutBits)
        return;

    Context->FadeOutOldBitmap = SelectBitmap(Context->FadeOutContext, Context->FadeOutBitmap);

    if (!Context->FadeOutOldBitmap)
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

static VOID PhpCaptureGraphAnimationTarget(
    _In_ PPHP_GRAPH_CONTEXT Context
    );

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

    memset(&getDrawInfo, 0, sizeof(PH_GRAPH_GETDRAWINFO));
    getDrawInfo.Header.hwndFrom = hwnd;
    getDrawInfo.Header.idFrom = Context->Id;
    getDrawInfo.Header.code = GCN_GETDRAWINFO;
    getDrawInfo.DrawInfo = &Context->DrawInfo;

    if (Context->Callback)
        Context->Callback(hwnd, GCN_GETDRAWINFO, &getDrawInfo, NULL, Context->Context);
    else
        SendMessage(Context->ParentHandle, WM_NOTIFY, 0, (LPARAM)&getDrawInfo);

    PhpCaptureGraphAnimationTarget(Context);

    // Smooth graphs: text boxes don't resize with every small change of the text.
    if (PhEnableGraphSmoothing)
    {
        PPH_GRAPH_DRAW_INFO drawInfo = &Context->DrawInfo;

        if (drawInfo->Text.Buffer)
            PhStabilizeGraphTextBox(&Context->TextBoxState, &drawInfo->TextBoxRect, drawInfo->TextBoxRect.left > drawInfo->Width / 2);
        else
            memset(&Context->TextBoxState, 0, sizeof(PH_GRAPH_TEXT_BOX_STATE));

        if (drawInfo->Text2.Buffer)
            PhStabilizeGraphTextBox(&Context->TextBoxState2, &drawInfo->TextBoxRect2, drawInfo->TextBoxRect2.left > drawInfo->Width / 2);
        else
            memset(&Context->TextBoxState2, 0, sizeof(PH_GRAPH_TEXT_BOX_STATE));
    }
}

static VOID PhpFreeGraphAnimationBuffers(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    if (Context->AnimationTarget1)
        PhFree(Context->AnimationTarget1); // one allocation holds all six buffers

    Context->AnimationTarget1 = NULL;
    Context->AnimationCapacity = 0;
    Context->AnimationTargetCount = 0;
    Context->AnimationFromCount = 0;
}

// Keeps a copy of the data just received; it is what the next update animates from.
static VOID PhpCaptureGraphAnimationTarget(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    ULONG count = Context->DrawInfo.LineDataCount;

    if (!PhEnableGraphSmoothing || !Context->DrawInfo.LineData1)
    {
        Context->AnimationTargetCount = 0;
        return;
    }

    if (count > Context->AnimationCapacity)
    {
        PFLOAT buffer;
        ULONG capacity = max(count, 64);

        capacity += 8; // the frame also holds the samples scrolling out on the left
        buffer = PhAllocate(capacity * sizeof(FLOAT) * 6);
        memset(buffer, 0, capacity * sizeof(FLOAT) * 6);

        if (Context->AnimationTarget1)
        {
            // Keep the previous data so an animation can start from it.
            memcpy(buffer + capacity * 2, Context->AnimationFrom1, Context->AnimationFromCount * sizeof(FLOAT));
            memcpy(buffer + capacity * 3, Context->AnimationFrom2, Context->AnimationFromCount * sizeof(FLOAT));
            memcpy(buffer, Context->AnimationTarget1, Context->AnimationTargetCount * sizeof(FLOAT));
            memcpy(buffer + capacity, Context->AnimationTarget2, Context->AnimationTargetCount * sizeof(FLOAT));
            PhFree(Context->AnimationTarget1);
        }

        Context->AnimationCapacity = capacity;
        Context->AnimationTarget1 = buffer;
        Context->AnimationTarget2 = buffer + capacity;
        Context->AnimationFrom1 = buffer + capacity * 2;
        Context->AnimationFrom2 = buffer + capacity * 3;
        Context->AnimationFrame1 = buffer + capacity * 4;
        Context->AnimationFrame2 = buffer + capacity * 5;
    }

    memcpy(Context->AnimationTarget1, Context->DrawInfo.LineData1, count * sizeof(FLOAT));

    if ((Context->DrawInfo.Flags & PH_GRAPH_USE_LINE_2) && Context->DrawInfo.LineData2)
        memcpy(Context->AnimationTarget2, Context->DrawInfo.LineData2, count * sizeof(FLOAT));
    else
        memset(Context->AnimationTarget2, 0, count * sizeof(FLOAT));

    Context->AnimationTargetCount = count;
}

static VOID PhpStopGraphAnimation(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    if (Context->AnimationStartTime)
    {
        KillTimer(Context->Handle, PH_GRAPH_ANIMATION_TIMER_ID);
        Context->AnimationStartTime = 0;
    }

    Context->AnimationProgress = 1;
}

// Called when Increment new samples were added, before the data is refreshed: the current
// target becomes the starting point of the animation.
static VOID PhpStartGraphAnimation(
    _In_ PPHP_GRAPH_CONTEXT Context,
    _In_ LONG Increment
    )
{
    HWND rootWindow;
    ULONG64 now;

    if (
        Increment <= 0 ||
        !Context->AnimationTargetCount ||
        !IsWindowVisible(Context->Handle) ||
        ((rootWindow = GetAncestor(Context->Handle, GA_ROOT)) && IsIconic(rootWindow)) ||
        !PhpGraphAnimationEnabled()
        )
    {
        PhpStopGraphAnimation(Context);
        return;
    }

    memcpy(Context->AnimationFrom1, Context->AnimationTarget1, Context->AnimationTargetCount * sizeof(FLOAT));
    memcpy(Context->AnimationFrom2, Context->AnimationTarget2, Context->AnimationTargetCount * sizeof(FLOAT));
    Context->AnimationFromCount = Context->AnimationTargetCount;
    Context->AnimationShift = Increment;
    Context->AnimationProgress = 0;

    now = NtGetTickCount64();
    Context->AnimationDuration = PhpMeasureGraphAnimationDuration(&Context->AnimationLastStartTime, now);
    Context->AnimationStartTime = now;

    SetTimer(
        Context->Handle,
        PH_GRAPH_ANIMATION_TIMER_ID,
        PhpGraphAnimationFrameInterval(Context->AnimationDuration),
        NULL
        );
}

static VOID PhpStepGraphAnimation(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    ULONG64 elapsed = NtGetTickCount64() - Context->AnimationStartTime;

    if (elapsed >= Context->AnimationDuration)
    {
        PhpStopGraphAnimation(Context);
        return;
    }

    // Linear: the scrolling speed is constant.
    Context->AnimationProgress = (FLOAT)elapsed / Context->AnimationDuration;
}

// Draws the frame between the previous data and the current data.
static VOID PhpDrawGraphAnimationFrame(
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    PH_GRAPH_DRAW_INFO drawInfo;
    FLOAT progress = Context->AnimationProgress;
    LONG shift = Context->AnimationShift;
    ULONG targetCount = min(Context->AnimationTargetCount, Context->DrawInfo.LineDataCount);
    ULONG count;

    // While the graph is shifted right the oldest samples are still on screen: keep the ones
    // scrolling out on the left instead of drawing zeros there.
    count = min(max(targetCount, min(Context->AnimationFromCount + shift, targetCount + shift)), Context->AnimationCapacity);

    for (ULONG i = 0; i < count; i++)
    {
        // Sample i was sample i - shift before this update; new samples grow from the newest old one.
        LONG from = max((LONG)i - shift, 0);
        BOOLEAN hasFrom = (ULONG)from < Context->AnimationFromCount;
        FLOAT from1 = hasFrom ? Context->AnimationFrom1[from] : Context->AnimationTarget1[i];
        FLOAT from2 = hasFrom ? Context->AnimationFrom2[from] : Context->AnimationTarget2[i];
        FLOAT to1 = i < targetCount ? Context->AnimationTarget1[i] : from1;
        FLOAT to2 = i < targetCount ? Context->AnimationTarget2[i] : from2;

        Context->AnimationFrame1[i] = from1 + (to1 - from1) * progress;
        Context->AnimationFrame2[i] = from2 + (to2 - from2) * progress;
    }

    drawInfo = Context->DrawInfo;
    drawInfo.LineDataCount = count;
    drawInfo.LineData1 = Context->AnimationFrame1;
    drawInfo.LineData2 = Context->AnimationFrame2;

    PhpDrawGraphDirect(
        Context->BufferedContext,
        Context->BufferedBits,
        &drawInfo,
        (FLOAT)(shift * 2) * (1.0f - progress),
        &Context->DrawInfo, // label values come from the update, not the in-between frame
        &Context->LabelCache
        );
}

VOID PhpDrawGraphControl(
    _In_ HWND hwnd,
    _In_ PPHP_GRAPH_CONTEXT Context
    )
{
    if (Context->BufferedBits)
    {
        if (Context->AnimationStartTime && PhEnableGraphSmoothing && Context->AnimationTargetCount)
            PhpDrawGraphAnimationFrame(Context);
        else
            PhpDrawGraphDirect(Context->BufferedContext, Context->BufferedBits, &Context->DrawInfo, 0, NULL, &Context->LabelCache);
    }

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

        memset(&drawPanel, 0, sizeof(PH_GRAPH_DRAWPANEL));
        drawPanel.Header.hwndFrom = hwnd;
        drawPanel.Header.idFrom = Context->Id;
        drawPanel.Header.code = GCN_DRAWPANEL;
        drawPanel.hdc = Context->BufferedContext;
        drawPanel.Rect = Context->BufferedContextRect;

        if (Context->Callback)
            Context->Callback(hwnd, GCN_DRAWPANEL, &drawPanel, NULL, Context->Context);
        else
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

    if (uMsg == WM_NCCREATE)
    {
        context = PhCreateGraphContext();
        PhSetWindowContextEx(hwnd, context);
    }
    else
    {
        context = PhGetWindowContextEx(hwnd);
    }

    switch (uMsg)
    {
    case WM_NCCREATE:
        {
            CREATESTRUCT *createStruct = (CREATESTRUCT *)lParam;

            context->Handle = hwnd;
            context->ParentHandle = createStruct->hwndParent;
            context->Style = createStruct->style;
            context->Id = (ULONG_PTR)createStruct->hMenu;

            if (createStruct->lpCreateParams)
            {
                PPH_GRAPH_CREATEPARAMS params = createStruct->lpCreateParams;

                if (RTL_CONTAINS_FIELD(params, params->Size, Options))
                {
                    if (params->Options.DefaultCursor || params->Options.FadeOutBackColor || params->Options.FadeOutWidth)
                    {
                        memcpy(&context->Options, &params->Options, sizeof(PH_GRAPH_OPTIONS));
                    }
                }

                if (RTL_CONTAINS_FIELD(params, params->Size, Callback))
                {
                    if (params->Callback)
                    {
                        context->Callback = params->Callback;
                    }
                }

                if (RTL_CONTAINS_FIELD(params, params->Size, Context))
                {
                    if (params->Context)
                    {
                        context->Context = params->Context;
                    }
                }
            }

            PhSetWindowContextEx(hwnd, context);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContextEx(hwnd);
            PhpStopGraphAnimation(context);
            PhpFreeGraphAnimationBuffers(context);
            PhpDeleteGraphLabelCache(&context->LabelCache);

            if (context->TooltipHandle)
                DestroyWindow(context->TooltipHandle);

            PhpDeleteFadeOutContext(context);
            PhpDeleteDrawInfoFonts(&context->DrawInfo);
            PhpDeleteBufferedContext(context);
            PhpFreeGraphContext(context);
        }
        break;
    case WM_TIMER:
        {
            if (wParam == PH_GRAPH_ANIMATION_TIMER_ID)
            {
                PhpStepGraphAnimation(context);
                context->NeedsDraw = TRUE;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
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

            if (wParam == GWL_EXSTYLE)
            {
                context->ExStyle = styleStruct->styleNew;
                context->NeedsDraw = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhpStopGraphAnimation(context);

            // Force a re-create of the buffered context.
            PhpDeleteBufferedContext(context);
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

                if (PhGetClientRect(hwnd, &toolInfo.rect))
                {
                    SendMessage(context->TooltipHandle, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);
                }
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
    //case WM_NCPAINT:
    //    {
    //        HRGN updateRegion;
    //
    //        updateRegion = (HRGN)wParam;
    //
    //        if (updateRegion == HRGN_FULL)
    //            updateRegion = NULL;
    //
    //        // Themed border
    //        if (context->Style & WS_BORDER)
    //        {
    //            HDC hdc;
    //            ULONG flags;
    //            RECT rect;
    //
    //            // Note the use of undocumented flags below. GetDCEx doesn't work without these.
    //
    //            flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | 0x10000;
    //
    //            if (updateRegion)
    //                flags |= DCX_INTERSECTRGN | 0x40000;
    //
    //            if (hdc = GetDCEx(hwnd, updateRegion, flags))
    //            {
    //                GetClientRect(hwnd, &rect);
    //                rect.right += 2;
    //                rect.bottom += 2;
    //                SetDCBrushColor(hdc, RGB(0x8f, 0x8f, 0x8f));
    //                FrameRect(hdc, &rect, PhGetStockBrush(DC_BRUSH));
    //
    //                ReleaseDC(hwnd, hdc);
    //                return 0;
    //            }
    //        }
    //    }
    //    break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (!context->TooltipHandle)
                break;

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

                        if (!PhGetClientPos(hwnd, &point))
                            break;
                        if (!PhGetClientRect(hwnd, &clientRect))
                            break;

                        memset(&getTooltipText, 0, sizeof(PH_GRAPH_GETTOOLTIPTEXT));
                        getTooltipText.Header.hwndFrom = hwnd;
                        getTooltipText.Header.idFrom = context->Id;
                        getTooltipText.Header.code = GCN_GETTOOLTIPTEXT;
                        getTooltipText.Index = (clientRect.right - point.x - 1) / context->DrawInfo.Step;
                        getTooltipText.TotalCount = context->DrawInfo.LineDataCount;
                        getTooltipText.Text.Buffer = NULL;
                        getTooltipText.Text.Length = 0;

                        if (context->Callback)
                            context->Callback(hwnd, GCN_GETTOOLTIPTEXT, &getTooltipText, NULL, context->Context);
                        else
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
                PhSetCursor(context->Options.DefaultCursor);
                return TRUE;
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            POINT cursorPos;

            cursorPos.x = GET_X_LPARAM(lParam);
            cursorPos.y = GET_Y_LPARAM(lParam);

            if (context->TooltipHandle)
            {
                MSG message;

                memset(&message, 0, sizeof(message));
                message.hwnd = hwnd;
                message.message = uMsg;
                message.wParam = wParam;
                message.lParam = lParam;

                SendMessage(context->TooltipHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }

            if (context->LastCursorLocation.x != cursorPos.x || context->LastCursorLocation.y != cursorPos.y)
            {
                if (context->TooltipHandle)
                    SendMessage(context->TooltipHandle, TTM_UPDATE, 0, 0);

                context->LastCursorLocation = cursorPos;
            }

            if (!context->Hot)
            {
                TRACKMOUSEEVENT trackMouseEvent;

                context->Hot = TRUE;

                memset(&trackMouseEvent, 0, sizeof(TRACKMOUSEEVENT));
                trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                trackMouseEvent.dwFlags = TME_LEAVE;
                trackMouseEvent.hwndTrack = hwnd;
                trackMouseEvent.dwHoverTime = 0;

                TrackMouseEvent(&trackMouseEvent);
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

            if (context->TooltipHandle)
            {
                MSG message;

                memset(&message, 0, sizeof(message));
                message.hwnd = hwnd;
                message.message = uMsg;
                message.wParam = wParam;
                message.lParam = lParam;

                SendMessage(context->TooltipHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }

            if (!PhGetClientRect(hwnd, &clientRect))
                break;

            memset(&mouseEvent, 0, sizeof(PH_GRAPH_MOUSEEVENT));
            mouseEvent.Header.hwndFrom = hwnd;
            mouseEvent.Header.idFrom = context->Id;
            mouseEvent.Header.code = GCN_MOUSEEVENT;
            mouseEvent.Message = uMsg;
            mouseEvent.Keys = (ULONG)wParam;
            mouseEvent.Point.x = GET_X_LPARAM(lParam);
            mouseEvent.Point.y = GET_Y_LPARAM(lParam);

            mouseEvent.Index = (clientRect.right - mouseEvent.Point.x - 1) / context->DrawInfo.Step;
            mouseEvent.TotalCount = context->DrawInfo.LineDataCount;

            if (context->Callback)
                context->Callback(hwnd, GCN_MOUSEEVENT, &mouseEvent, NULL, context->Context);
            else
                SendMessage(context->ParentHandle, WM_NOTIFY, 0, (LPARAM)&mouseEvent);
        }
        break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        {
            if (context->TooltipHandle)
            {
                MSG message;

                memset(&message, 0, sizeof(message));
                message.hwnd = hwnd;
                message.message = uMsg;
                message.wParam = wParam;
                message.lParam = lParam;

                SendMessage(context->TooltipHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }
        }
        break;
    case GCM_GETDRAWINFO:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)lParam;

            // Copy only the public draw-info fields; the trailing cache fields are not copied.
            memcpy(drawInfo, &context->DrawInfo, FIELD_OFFSET(PH_GRAPH_DRAW_INFO, CachedFontDpi));
        }
        return TRUE;
    case GCM_SETDRAWINFO:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)lParam;
            ULONG width;
            ULONG height;

            width = context->DrawInfo.Width;
            height = context->DrawInfo.Height;
            // Copy only the public draw-info fields; the trailing cache fields are not copied.
            memcpy(&context->DrawInfo, drawInfo, FIELD_OFFSET(PH_GRAPH_DRAW_INFO, CachedFontDpi));
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
            PhpStartGraphAnimation(context, increment);
        }
        return TRUE;
    case GCM_GETBUFFEREDCONTEXT:
        return (LRESULT)context->BufferedContext;
    case GCM_SETTOOLTIP:
        {
            if (wParam)
            {
                TOOLINFO toolInfo;

                if (context->TooltipHandle)
                {
                    DestroyWindow(context->TooltipHandle);
                    context->TooltipHandle = NULL;
                }

                context->TooltipHandle = PhCreateWindowEx(
                    TOOLTIPS_CLASS,
                    NULL,
                    WS_POPUP | TTS_NOPREFIX | TTS_NOANIMATE | TTS_NOFADE | TTS_ALWAYSTIP,
                    WS_EX_TOPMOST | WS_EX_TRANSPARENT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

                SetWindowPos(context->TooltipHandle, HWND_TOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);

                memset(&toolInfo, 0, sizeof(TOOLINFO));
                toolInfo.cbSize = sizeof(TOOLINFO);
                toolInfo.uFlags = 0;
                toolInfo.hwnd = hwnd;
                toolInfo.uId = 1;
                toolInfo.lpszText = LPSTR_TEXTCALLBACK;

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
                if (context->TooltipHandle)
                {
                    DestroyWindow(context->TooltipHandle);
                    context->TooltipHandle = NULL;
                }
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
    case GCM_UPDATE:
        {
            #define INCREMENT_OFFSET 1

            // GCM_MOVEGRID

            context->DrawInfo.GridXOffset += INCREMENT_OFFSET;
            PhpStartGraphAnimation(context, INCREMENT_OFFSET);

            // GCM_DRAW

            PhpUpdateDrawInfo(hwnd, context);
            context->NeedsDraw = TRUE;

            // GCM_UPDATETOOLTIP

            if (context->TooltipHandle)
            {
                SendMessage(context->TooltipHandle, TTM_UPDATE, 0, 0);
            }

            InvalidateRect(hwnd, NULL, FALSE);
        }
        return TRUE;
    case GCM_SETCALLBACK:
        {
            context->Callback = (PVOID)wParam;
            context->Context = (PVOID)lParam;
        }
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

    Buffers->AllocatedCount = 0;
    Buffers->Data1 = NULL;
    Buffers->Data2 = NULL;
    Buffers->Valid = FALSE;
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
    State->TooltipIndex = ULONG_MAX;
    State->TextLine2 = NULL;
}

VOID PhDeleteGraphState(
    _Inout_ PPH_GRAPH_STATE State
    )
{
    PhDeleteGraphBuffers(&State->Buffers);
    if (State->Text) PhDereferenceObject(State->Text);
    if (State->TooltipText) PhDereferenceObject(State->TooltipText);
    if (State->TextLine2) PhDereferenceObject(State->TextLine2);
}

VOID PhGraphStateGetDrawInfo(
    _Inout_ PPH_GRAPH_STATE State,
    _In_ PPH_GRAPH_GETDRAWINFO GetDrawInfo,
    _In_ ULONG DataCount
    )
{
    PhGetDrawInfoGraphBuffers(&State->Buffers, GetDrawInfo->DrawInfo, DataCount);
}
