/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2024-2026
 *
 */

#include <ph.h>
#include <guisup.h>
#include <guisupp.h>
#include <graphbar.h>
#include <math.h>

#define PH_GRAPHBAR_SEGMENT_COUNT 5
#define PH_GRAPHBAR_BAR_HEIGHT 18
#define PH_GRAPHBAR_LEGEND_PADDING 6
#define PH_GRAPHBAR_LEGEND_SWATCH 10
#define PH_GRAPHBAR_LEGEND_GAP 6
#define PH_GRAPHBAR_LEGEND_LINE_GAP 2
#define PH_GRAPHBAR_LEGEND_SPACING 18

// Memory bar palette (Telegram storage palette)
#define PH_GRAPHBAR_COLOR_HARDWARE RGB(154, 154, 151) // gray
#define PH_GRAPHBAR_COLOR_INUSE RGB(103, 187, 70) // green
#define PH_GRAPHBAR_COLOR_MODIFIED RGB(238, 137, 61) // orange
#define PH_GRAPHBAR_COLOR_STANDBY RGB(78, 164, 224) // blue
#define PH_GRAPHBAR_COLOR_FREE RGB(72, 192, 196) // teal
#define PH_GRAPHBAR_COLOR_TEXT RGB(28, 28, 28)
#define PH_GRAPHBAR_COLOR_BACKGROUND RGB(254, 254, 254)

RTL_ATOM NTAPI PhGraphBarInitialization(
    VOID
    )
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_GLOBALCLASS | CS_HREDRAW;
    wcex.lpfnWndProc = PhGraphBarWndProc;
    wcex.cbWndExtra = sizeof(PVOID);
    wcex.hInstance = NtCurrentImageBase();
    wcex.hCursor = PhLoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = PH_GRAPHBAR_CLASSNAME;

    return RegisterClassEx(&wcex);
}

VOID PhpGraphBarBuildSegments(
    _In_ const PH_GRAPHBAR_MEMORY* Memory,
    _Out_writes_(PH_GRAPHBAR_SEGMENT_COUNT) PH_GRAPHBAR_SEGMENT* Segments
    )
{
    Segments[0].Value = Memory->HardwareReserved;
    Segments[0].Color = PH_GRAPHBAR_COLOR_HARDWARE;
    Segments[0].Name = L"Hardware Reserved";
    Segments[1].Value = Memory->InUse;
    Segments[1].Color = PH_GRAPHBAR_COLOR_INUSE;
    Segments[1].Name = L"In Use";
    Segments[2].Value = Memory->Modified;
    Segments[2].Color = PH_GRAPHBAR_COLOR_MODIFIED;
    Segments[2].Name = L"Modified";
    Segments[3].Value = Memory->Standby;
    Segments[3].Color = PH_GRAPHBAR_COLOR_STANDBY;
    Segments[3].Name = L"Standby";
    Segments[4].Value = Memory->Free;
    Segments[4].Color = PH_GRAPHBAR_COLOR_FREE;
    Segments[4].Name = L"Free";
}

/**
 * Computes integer pixel widths for each segment. Rounding remainders are
 * distributed one pixel at a time so the widths always sum to TotalWidth.
 */
VOID PhpGraphBarComputeWidths(
    _In_reads_(PH_GRAPHBAR_SEGMENT_COUNT) const PH_GRAPHBAR_SEGMENT* Segments,
    _In_ LONG TotalWidth,
    _Out_writes_(PH_GRAPHBAR_SEGMENT_COUNT) PLONG Widths
    )
{
    ULONG64 total = 0;
    LONG used = 0;
    LONG remaining;
    ULONG i;

    for (i = 0; i < PH_GRAPHBAR_SEGMENT_COUNT; i++)
        total += Segments[i].Value;

    for (i = 0; i < PH_GRAPHBAR_SEGMENT_COUNT; i++)
    {
        if (total)
            Widths[i] = (LONG)floor(((DOUBLE)Segments[i].Value / (DOUBLE)total) * TotalWidth);
        else
            Widths[i] = 0;

        used += Widths[i];
    }

    if (!total)
        return;

    remaining = TotalWidth - used;

    for (i = 0; remaining > 0 && i < PH_GRAPHBAR_SEGMENT_COUNT; i++, remaining--)
        Widths[i]++;
}

VOID PhpGraphBarFillRect(
    _In_ HDC hdc,
    _In_ const RECT* Rect,
    _In_ COLORREF Color
    )
{
    SetDCBrushColor(hdc, Color);
    FillRect(hdc, Rect, PhGetStockBrush(DC_BRUSH));
}

VOID PhpGraphBarFormatSize(
    _In_ ULONG64 Bytes,
    _Out_writes_(BufferLength) PWSTR Buffer,
    _In_ SIZE_T BufferLength
    )
{
    PH_FORMAT format[1];

    PhInitFormatSize(&format[0], Bytes);

    if (!PhFormatToBuffer(format, RTL_NUMBER_OF(format), Buffer, BufferLength * sizeof(WCHAR), NULL))
    {
        Buffer[0] = UNICODE_NULL;
    }
}

VOID PhpGraphBarGetBarRect(
    _In_ const RECT* ClientRect,
    _Out_ PRECT BarRect
    )
{
    BarRect->left = 0;
    BarRect->top = 0;
    BarRect->right = ClientRect->right - ClientRect->left;
    BarRect->bottom = __min(ClientRect->bottom - ClientRect->top, PH_GRAPHBAR_BAR_HEIGHT);
}

VOID PhpGraphBarDrawLegend(
    _In_ HDC hdc,
    _In_ const RECT* BarRect,
    _In_reads_(PH_GRAPHBAR_SEGMENT_COUNT) const PH_GRAPHBAR_SEGMENT* Segments
    )
{
    HFONT oldFont;
    INT oldBkMode;
    COLORREF oldTextColor;
    LONG x = BarRect->left;
    LONG y = BarRect->bottom + PH_GRAPHBAR_LEGEND_PADDING;

    oldFont = SelectFont(hdc, GetStockFont(DEFAULT_GUI_FONT));
    oldBkMode = SetBkMode(hdc, TRANSPARENT);
    oldTextColor = SetTextColor(hdc, PH_GRAPHBAR_COLOR_TEXT);

    for (ULONG i = 0; i < PH_GRAPHBAR_SEGMENT_COUNT; i++)
    {
        RECT swatchRect;
        SIZE nameSize = { 0 };
        SIZE valueSize = { 0 };
        WCHAR valueBuffer[64];
        ULONG nameLength;
        ULONG valueLength;

        swatchRect.left = x;
        swatchRect.top = y;
        swatchRect.right = x + PH_GRAPHBAR_LEGEND_SWATCH;
        swatchRect.bottom = y + PH_GRAPHBAR_LEGEND_SWATCH;
        PhpGraphBarFillRect(hdc, &swatchRect, Segments[i].Color);
        x += PH_GRAPHBAR_LEGEND_SWATCH + PH_GRAPHBAR_LEGEND_GAP;

        PhpGraphBarFormatSize(Segments[i].Value, valueBuffer, RTL_NUMBER_OF(valueBuffer));
        nameLength = (ULONG)PhCountStringZ((PWSTR)Segments[i].Name);
        valueLength = (ULONG)PhCountStringZ(valueBuffer);

        GetTextExtentPoint32(hdc, Segments[i].Name, nameLength, &nameSize);
        GetTextExtentPoint32(hdc, valueBuffer, valueLength, &valueSize);
        TextOut(hdc, x, y, Segments[i].Name, nameLength);
        TextOut(hdc, x, y + nameSize.cy + PH_GRAPHBAR_LEGEND_LINE_GAP, valueBuffer, valueLength);

        x += __max(nameSize.cx, valueSize.cx) + PH_GRAPHBAR_LEGEND_SPACING;
    }

    SetTextColor(hdc, oldTextColor);
    SetBkMode(hdc, oldBkMode);
    SelectFont(hdc, oldFont);
}

VOID PhpGraphBarDrawMemory(
    _In_ PPH_GRAPHBAR_CONTEXT Context,
    _In_ HDC TargetHdc,
    _In_ const RECT* ClientRect
    )
{
    PH_BUFFERED_PAINT bufferedPaint;
    HDC hdc;
    BOOLEAN buffered;
    INT savedDc;
    RECT rect;
    RECT barRect;
    PH_GRAPHBAR_SEGMENT segments[PH_GRAPHBAR_SEGMENT_COUNT];
    LONG widths[PH_GRAPHBAR_SEGMENT_COUNT];
    LONG x;

    buffered = PhBeginBufferedPaint(TargetHdc, ClientRect, PHBF_TOPDOWNDIB, NULL, &bufferedPaint, &hdc);

    if (!buffered)
        hdc = TargetHdc;

    savedDc = SaveDC(hdc);
    SetViewportOrgEx(hdc, ClientRect->left, ClientRect->top, NULL);

    rect.left = 0;
    rect.top = 0;
    rect.right = ClientRect->right - ClientRect->left;
    rect.bottom = ClientRect->bottom - ClientRect->top;
    PhpGraphBarFillRect(hdc, &rect, PH_GRAPHBAR_COLOR_BACKGROUND);

    PhpGraphBarGetBarRect(ClientRect, &barRect);
    PhpGraphBarBuildSegments(&Context->Memory, segments);
    PhpGraphBarComputeWidths(segments, barRect.right - barRect.left, widths);

    x = barRect.left;

    for (ULONG i = 0; i < PH_GRAPHBAR_SEGMENT_COUNT; i++)
    {
        if (widths[i] > 0)
        {
            RECT segmentRect;

            segmentRect.left = x;
            segmentRect.top = barRect.top;
            segmentRect.right = x + widths[i];
            segmentRect.bottom = barRect.bottom;
            PhpGraphBarFillRect(hdc, &segmentRect, segments[i].Color);
        }

        x += widths[i];
    }

    if (Context->Style & PH_GRAPHBAR_STYLE_LEGEND)
    {
        PhpGraphBarDrawLegend(hdc, &barRect, segments);
    }

    RestoreDC(hdc, savedDc);

    if (buffered)
    {
        PhEndBufferedPaint(&bufferedPaint, TRUE);
    }
}

VOID PhpGraphBarEnsureTooltip(
    _In_ HWND WindowHandle,
    _Inout_ PPH_GRAPHBAR_CONTEXT Context
    )
{
    TOOLINFO toolInfo = { sizeof(TOOLINFO) };
    HWND tooltipHandle;

    if (Context->TooltipHandle)
        return;

    tooltipHandle = PhCreateWindowEx(
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_NOANIMATE | TTS_NOFADE | TTS_ALWAYSTIP,
        WS_EX_TOPMOST | WS_EX_TRANSPARENT,
        0,
        0,
        0,
        0,
        WindowHandle,
        NULL,
        NULL,
        NULL
        );

    if (!tooltipHandle)
        return;

    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.hwnd = WindowHandle;
    toolInfo.uId = (UINT_PTR)WindowHandle;
    toolInfo.lpszText = LPSTR_TEXTCALLBACK;

    SendMessage(tooltipHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
    SendMessage(tooltipHandle, TTM_SETDELAYTIME, TTDT_INITIAL, 0);
    SendMessage(tooltipHandle, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);
    SendMessage(tooltipHandle, TTM_SETMAXTIPWIDTH, 0, MAXSHORT);

    Context->TooltipHandle = tooltipHandle;
}

VOID PhpGraphBarOnMouseMove(
    _In_ HWND WindowHandle,
    _Inout_ PPH_GRAPHBAR_CONTEXT Context,
    _In_ PPOINT CursorPos
    )
{
    if (Context->LastCursorLocation.x != CursorPos->x || Context->LastCursorLocation.y != CursorPos->y)
    {
        PhpGraphBarEnsureTooltip(WindowHandle, Context);
        Context->LastCursorLocation = *CursorPos;

        if (Context->TooltipHandle)
        {
            SendMessage(Context->TooltipHandle, TTM_UPDATE, 0, 0);
        }
    }

    if (!Context->Hot)
    {
        TRACKMOUSEEVENT trackMouseEvent = { sizeof(TRACKMOUSEEVENT) };

        trackMouseEvent.dwFlags = TME_LEAVE;
        trackMouseEvent.hwndTrack = WindowHandle;
        TrackMouseEvent(&trackMouseEvent);

        Context->Hot = TRUE;
    }
}

BOOLEAN PhpGraphBarHitTest(
    _In_ PPH_GRAPHBAR_CONTEXT Context,
    _In_ LONG X,
    _Out_ PULONG SegmentIndex
    )
{
    RECT clientRect;
    RECT barRect;
    PH_GRAPHBAR_SEGMENT segments[PH_GRAPHBAR_SEGMENT_COUNT];
    LONG widths[PH_GRAPHBAR_SEGMENT_COUNT];
    LONG position;

    if (!GetClientRect(Context->WindowHandle, &clientRect))
        return FALSE;

    // Only the bar is segmented; the legend is below.
    PhpGraphBarGetBarRect(&clientRect, &barRect);

    if (X < barRect.left || X >= barRect.right)
        return FALSE;

    PhpGraphBarBuildSegments(&Context->Memory, segments);
    PhpGraphBarComputeWidths(segments, barRect.right - barRect.left, widths);

    position = barRect.left;

    for (ULONG i = 0; i < PH_GRAPHBAR_SEGMENT_COUNT; i++)
    {
        if (X >= position && X < position + widths[i])
        {
            *SegmentIndex = i;
            return TRUE;
        }

        position += widths[i];
    }

    return FALSE;
}

VOID PhpGraphBarQueryMemory(
    _In_ HWND WindowHandle,
    _Inout_ PPH_GRAPHBAR_CONTEXT Context
    )
{
    PH_GRAPHBAR_GETMEMORY getMemory;
    BOOLEAN handled = FALSE;

    memset(&getMemory, 0, sizeof(PH_GRAPHBAR_GETMEMORY));
    getMemory.Header.hwndFrom = WindowHandle;
    getMemory.Header.idFrom = Context->Id;
    getMemory.Header.code = GBN_GETMEMORY;
    getMemory.Memory = Context->Memory;

    if (Context->Callback)
        handled = Context->Callback(WindowHandle, GBN_GETMEMORY, &getMemory, NULL, Context->Context);
    else if (Context->ParentHandle)
        handled = !!SendMessage(Context->ParentHandle, WM_NOTIFY, 0, (LPARAM)&getMemory);

    if (handled)
    {
        Context->Memory = getMemory.Memory;
    }
}

VOID PhpGraphBarUpdateTooltipText(
    _In_ HWND WindowHandle,
    _Inout_ PPH_GRAPHBAR_CONTEXT Context
    )
{
    PH_GRAPHBAR_SEGMENT segments[PH_GRAPHBAR_SEGMENT_COUNT];
    ULONG index;
    WCHAR sizeBuffer[64];
    PH_FORMAT format[3];
    WCHAR buffer[128];
    SIZE_T returnLength;

    PhpGraphBarQueryMemory(WindowHandle, Context);

    if (!PhpGraphBarHitTest(Context, Context->LastCursorLocation.x, &index))
    {
        PhClearReference(&Context->LastTooltipText);
        return;
    }

    PhpGraphBarBuildSegments(&Context->Memory, segments);
    PhpGraphBarFormatSize(segments[index].Value, sizeBuffer, RTL_NUMBER_OF(sizeBuffer));

    PhInitFormatS(&format[0], (PWSTR)segments[index].Name);
    PhInitFormatS(&format[1], L": ");
    PhInitFormatS(&format[2], sizeBuffer);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), &returnLength))
    {
        PhMoveReference(&Context->LastTooltipText, PhCreateStringEx(buffer, returnLength - sizeof(UNICODE_NULL)));
    }
    else
    {
        PhClearReference(&Context->LastTooltipText);
    }
}

BOOLEAN PhpGraphBarNotifyDraw(
    _In_ HWND WindowHandle,
    _In_ PPH_GRAPHBAR_CONTEXT Context,
    _In_ HDC hdc,
    _In_ const RECT* ClientRect
    )
{
    PH_GRAPHBAR_DRAWBAR drawBar;

    memset(&drawBar, 0, sizeof(PH_GRAPHBAR_DRAWBAR));
    drawBar.Header.hwndFrom = WindowHandle;
    drawBar.Header.idFrom = Context->Id;
    drawBar.Header.code = GBN_DRAWBAR;
    drawBar.hdc = hdc;
    drawBar.Rect = *ClientRect;

    if (Context->Callback)
        return Context->Callback(WindowHandle, GBN_DRAWBAR, &drawBar, NULL, Context->Context);
    else if (Context->ParentHandle)
        return !!SendMessage(Context->ParentHandle, WM_NOTIFY, 0, (LPARAM)&drawBar);

    return FALSE;
}

PPH_GRAPHBAR_CONTEXT PhpCreateGraphBarContext(
    VOID
    )
{
    PPH_GRAPHBAR_CONTEXT context;

    context = PhAllocateZero(sizeof(PH_GRAPHBAR_CONTEXT));
    context->EnableRedraw = TRUE;

    return context;
}

VOID PhpDestroyGraphBarContext(
    _In_ _Post_invalid_ PPH_GRAPHBAR_CONTEXT Context
    )
{
    PhClearReference(&Context->LastTooltipText);
    PhFree(Context);
}

VOID PhpGraphBarOnPaint(
    _In_ HWND WindowHandle,
    _In_ PPH_GRAPHBAR_CONTEXT Context
    )
{
    PAINTSTRUCT paintStruct;
    RECT clientRect;
    HDC hdc;

    if (hdc = BeginPaint(WindowHandle, &paintStruct))
    {
        GetClientRect(WindowHandle, &clientRect);

        PhpGraphBarQueryMemory(WindowHandle, Context);

        if (!PhpGraphBarNotifyDraw(WindowHandle, Context, hdc, &clientRect))
        {
            PhpGraphBarDrawMemory(Context, hdc, &clientRect);
        }

        EndPaint(WindowHandle, &paintStruct);
    }
}

LRESULT CALLBACK PhGraphBarWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_GRAPHBAR_CONTEXT context;

    if (WindowMessage == WM_NCCREATE)
    {
        LPCREATESTRUCT createStruct = (LPCREATESTRUCT)lParam;

        context = PhpCreateGraphBarContext();
        context->WindowHandle = WindowHandle;
        context->ParentHandle = createStruct->hwndParent;
        context->Id = (ULONG_PTR)createStruct->hMenu;

        if (createStruct->lpCreateParams)
        {
            PPH_GRAPHBAR_CREATEPARAMS createParams = createStruct->lpCreateParams;

            if (RTL_CONTAINS_FIELD(createParams, createParams->Size, Context))
            {
                context->Callback = createParams->Callback;
                context->Context = createParams->Context;
            }
        }

        PhSetWindowContextEx(WindowHandle, context);
        return TRUE;
    }

    context = PhGetWindowContextEx(WindowHandle);

    if (!context)
        return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);

    switch (WindowMessage)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContextEx(WindowHandle);
            PhpDestroyGraphBarContext(context);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PhpGraphBarOnPaint(WindowHandle, context);
        }
        return 0;
    case WM_MOUSEMOVE:
        {
            POINT cursorPos;

            cursorPos.x = GET_X_LPARAM(lParam);
            cursorPos.y = GET_Y_LPARAM(lParam);

            PhpGraphBarOnMouseMove(WindowHandle, context, &cursorPos);
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
        return 0;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->hwndFrom == context->TooltipHandle && header->code == TTN_GETDISPINFO)
            {
                LPNMTTDISPINFO dispInfo = (LPNMTTDISPINFO)lParam;
                PH_GRAPHBAR_GETTOOLTIPTEXT getTooltipText;

                memset(&getTooltipText, 0, sizeof(PH_GRAPHBAR_GETTOOLTIPTEXT));
                getTooltipText.Header.hwndFrom = WindowHandle;
                getTooltipText.Header.idFrom = context->Id;
                getTooltipText.Header.code = GBN_GETTOOLTIPTEXT;
                getTooltipText.CursorX = context->LastCursorLocation.x;
                GetClientRect(WindowHandle, &getTooltipText.Rect);

                // Ask the owner first, then fall back to the default text.
                if (context->Callback)
                    context->Callback(WindowHandle, GBN_GETTOOLTIPTEXT, &getTooltipText, NULL, context->Context);
                else if (context->ParentHandle)
                    SendMessage(context->ParentHandle, WM_NOTIFY, 0, (LPARAM)&getTooltipText);

                if (getTooltipText.Text)
                {
                    PhMoveReference(&context->LastTooltipText, getTooltipText.Text);
                }
                else
                {
                    PhpGraphBarUpdateTooltipText(WindowHandle, context);
                }

                if (context->LastTooltipText)
                {
                    dispInfo->lpszText = PhGetString(context->LastTooltipText);
                }
            }
        }
        return 0;
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}
