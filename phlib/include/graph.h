/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2022
 *
 */

#ifndef _PH_GRAPH_H
#define _PH_GRAPH_H

#ifdef __cplusplus
extern "C" {
#endif

// Graph drawing

extern RECT PhNormalGraphTextMargin;
extern RECT PhNormalGraphTextPadding;

#define PH_GRAPH_USE_GRID_X 0x1
#define PH_GRAPH_USE_GRID_Y 0x2
#define PH_GRAPH_LOGARITHMIC_GRID_Y 0x4
#define PH_GRAPH_USE_LINE_2 0x10
#define PH_GRAPH_OVERLAY_LINE_2 0x20
#define PH_GRAPH_LABEL_MAX_Y 0x1000

typedef struct _PH_GRAPH_DRAW_INFO *PPH_GRAPH_DRAW_INFO;

typedef PPH_STRING (NTAPI *PPH_GRAPH_LABEL_Y_FUNCTION)(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    );

typedef struct _PH_GRAPH_DRAW_INFO
{
    // Basic
    ULONG Width;
    ULONG Height;
    ULONG Flags;
    ULONG Step;
    COLORREF BackColor;

    // Data/lines
    ULONG LineDataCount;
    PFLOAT LineData1;
    PFLOAT LineData2;
    COLORREF LineColor1;
    COLORREF LineColor2;
    COLORREF LineBackColor1;
    COLORREF LineBackColor2;

    // Grid
    COLORREF GridColor;
    ULONG GridWidth;
    FLOAT GridHeight;
    ULONG GridXOffset;
    ULONG GridYThreshold;
    FLOAT GridBase; // Base for logarithmic grid

    // y-axis label
    PPH_GRAPH_LABEL_Y_FUNCTION LabelYFunction;
    FLOAT LabelYFunctionParameter;
    HFONT LabelYFont;
    COLORREF LabelYColor;
    ULONG LabelMaxYIndexLimit;

    // Text
    PH_STRINGREF Text;
    RECT TextRect;
    RECT TextBoxRect;
    HFONT TextFont;
    COLORREF TextColor;
    COLORREF TextBoxColor;
} PH_GRAPH_DRAW_INFO, *PPH_GRAPH_DRAW_INFO;

// Graph control

#define PH_GRAPH_CLASSNAME L"PhGraph"

PHLIBAPI
BOOLEAN PhGraphControlInitialization(
    VOID
    );

PHLIBAPI
VOID PhDrawGraphDirect(
    _In_ HDC hdc,
    _In_ PVOID Bits,
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo
    );

PHLIBAPI
VOID PhSetGraphText(
    _In_ HDC hdc,
    _Inout_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ PPH_STRINGREF Text,
    _In_ PRECT Margin,
    _In_ PRECT Padding,
    _In_ ULONG Align
    );

PHLIBAPI
VOID PhDrawTrayIconText(
    _In_ HDC hdc,
    _In_ PVOID Bits,
    _Inout_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ PPH_STRINGREF Text
    );

PHLIBAPI
HFONT PhNfGetTrayIconFont( // Note: Exported from notifico.c (dmex)
    _In_opt_ LONG DpiValue
    );

// Configuration

typedef struct _PH_GRAPH_OPTIONS
{
    COLORREF FadeOutBackColor;
    ULONG FadeOutWidth;
    HCURSOR DefaultCursor;
} PH_GRAPH_OPTIONS, *PPH_GRAPH_OPTIONS;

// Styles

#define GC_STYLE_FADEOUT 0x1
#define GC_STYLE_DRAW_PANEL 0x2

// Messages

#define GCM_GETDRAWINFO (WM_USER + 1301)
#define GCM_SETDRAWINFO (WM_USER + 1302)
#define GCM_DRAW (WM_USER + 1303)
#define GCM_MOVEGRID (WM_USER + 1304)
#define GCM_GETBUFFEREDCONTEXT (WM_USER + 1305)
#define GCM_SETTOOLTIP (WM_USER + 1306)
#define GCM_UPDATETOOLTIP (WM_USER + 1307)
#define GCM_GETOPTIONS (WM_USER + 1308)
#define GCM_SETOPTIONS (WM_USER + 1309)

#define Graph_GetDrawInfo(hWnd, DrawInfo) \
    SendMessage((hWnd), GCM_GETDRAWINFO, 0, (LPARAM)(DrawInfo))
#define Graph_SetDrawInfo(hWnd, DrawInfo) \
    SendMessage((hWnd), GCM_SETDRAWINFO, 0, (LPARAM)(DrawInfo))
#define Graph_Draw(hWnd) \
    SendMessage((hWnd), GCM_DRAW, 0, 0)
#define Graph_MoveGrid(hWnd, Increment) \
    SendMessage((hWnd), GCM_MOVEGRID, (WPARAM)(Increment), 0)
#define Graph_GetBufferedContext(hWnd) \
    ((HDC)SendMessage((hWnd), GCM_GETBUFFEREDCONTEXT, 0, 0))
#define Graph_SetTooltip(hWnd, Enable) \
    ((HDC)SendMessage((hWnd), GCM_SETTOOLTIP, (WPARAM)(Enable), 0))
#define Graph_UpdateTooltip(hWnd) \
    ((HDC)SendMessage((hWnd), GCM_UPDATETOOLTIP, 0, 0))
#define Graph_GetOptions(hWnd, Options) \
    SendMessage((hWnd), GCM_GETOPTIONS, 0, (LPARAM)(Options))
#define Graph_SetOptions(hWnd, Options) \
    SendMessage((hWnd), GCM_SETOPTIONS, 0, (LPARAM)(Options))

// Notifications

#define GCN_GETDRAWINFO (WM_USER + 1351)
#define GCN_GETTOOLTIPTEXT (WM_USER + 1352)
#define GCN_MOUSEEVENT (WM_USER + 1353)
#define GCN_DRAWPANEL (WM_USER + 1354)

typedef struct _PH_GRAPH_GETDRAWINFO
{
    NMHDR Header;
    PPH_GRAPH_DRAW_INFO DrawInfo;
} PH_GRAPH_GETDRAWINFO, *PPH_GRAPH_GETDRAWINFO;

typedef struct _PH_GRAPH_GETTOOLTIPTEXT
{
    NMHDR Header;
    ULONG Index;
    ULONG TotalCount;

    PH_STRINGREF Text; // must be null-terminated
} PH_GRAPH_GETTOOLTIPTEXT, *PPH_GRAPH_GETTOOLTIPTEXT;

typedef struct _PH_GRAPH_MOUSEEVENT
{
    NMHDR Header;
    ULONG Index;
    ULONG TotalCount;

    ULONG Message;
    ULONG Keys;
    POINT Point;
} PH_GRAPH_MOUSEEVENT, *PPH_GRAPH_MOUSEEVENT;

typedef struct _PH_GRAPH_DRAWPANEL
{
    NMHDR Header;
    HDC hdc;
    RECT Rect;
} PH_GRAPH_DRAWPANEL, *PPH_GRAPH_DRAWPANEL;

// Graph buffer management

#define PH_GRAPH_DATA_COUNT(Width, Step) (((Width) + (Step) - 1) / (Step) + 1) // round up in division

typedef struct _PH_GRAPH_BUFFERS
{
    PFLOAT Data1; // invalidate by setting Valid to FALSE
    PFLOAT Data2; // invalidate by setting Valid to FALSE
    ULONG AllocatedCount;
    BOOLEAN Valid; // indicates the data is valid
} PH_GRAPH_BUFFERS, *PPH_GRAPH_BUFFERS;

PHLIBAPI
VOID PhInitializeGraphBuffers(
    _Out_ PPH_GRAPH_BUFFERS Buffers
    );

PHLIBAPI
VOID PhDeleteGraphBuffers(
    _Inout_ PPH_GRAPH_BUFFERS Buffers
    );

PHLIBAPI
VOID PhGetDrawInfoGraphBuffers(
    _Inout_ PPH_GRAPH_BUFFERS Buffers,
    _Inout_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataCount
    );

// Graph control state

// The basic buffer management structure was moved out of this section because
// the text management is not needed for most cases.

typedef struct _PH_GRAPH_STATE
{
    // Union for compatibility
    union
    {
        struct
        {
            PFLOAT Data1; // invalidate by setting Valid to FALSE
            PFLOAT Data2; // invalidate by setting Valid to FALSE
            ULONG AllocatedCount;
            BOOLEAN Valid; // indicates the data is valid
        };
        PH_GRAPH_BUFFERS Buffers;
    };

    PPH_STRING Text;
    PPH_STRING TooltipText; // invalidate by setting TooltipIndex to -1
    ULONG TooltipIndex; // indicates the tooltip text is valid for this index
} PH_GRAPH_STATE, *PPH_GRAPH_STATE;

PHLIBAPI
VOID PhInitializeGraphState(
    _Out_ PPH_GRAPH_STATE State
    );

PHLIBAPI
VOID PhDeleteGraphState(
    _Inout_ PPH_GRAPH_STATE State
    );

PHLIBAPI
VOID PhGraphStateGetDrawInfo(
    _Inout_ PPH_GRAPH_STATE State,
    _In_ PPH_GRAPH_GETDRAWINFO GetDrawInfo,
    _In_ ULONG DataCount
    );

#ifdef __cplusplus
}
#endif

#endif
