#ifndef _PH_GRAPH_H
#define _PH_GRAPH_H

// Graph drawing

#ifndef GRAPH_PRIVATE
extern RECT PhNormalGraphTextMargin;
extern RECT PhNormalGraphTextPadding;
#endif

#define PH_GRAPH_USE_GRID 0x1
#define PH_GRAPH_USE_LINE_2 0x10
#define PH_GRAPH_OVERLAY_LINE_2 0x20

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
    ULONG GridHeight;
    ULONG GridStart;

    // Text
    PH_STRINGREF Text;
    RECT TextRect;
    RECT TextBoxRect;
    COLORREF TextColor;
    COLORREF TextBoxColor;
} PH_GRAPH_DRAW_INFO, *PPH_GRAPH_DRAW_INFO;

// Graph control

#define PH_GRAPH_CLASSNAME L"PhGraph"

BOOLEAN PhGraphControlInitialization();

VOID PhDrawGraph(
    __in HDC hdc,
    __in PPH_GRAPH_DRAW_INFO DrawInfo
    );

VOID PhSetGraphText(
    __in HDC hdc,
    __inout PPH_GRAPH_DRAW_INFO DrawInfo,
    __in PPH_STRINGREF Text,
    __in PRECT Margin,
    __in PRECT Padding,
    __in ULONG Align
    );

HWND PhCreateGraphControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    );

// Messages

#define GCM_GETDRAWINFO (WM_APP + 1301)
#define GCM_SETDRAWINFO (WM_APP + 1302)
#define GCM_DRAW (WM_APP + 1303)
#define GCM_MOVEGRID (WM_APP + 1304)
#define GCM_GETBUFFEREDCONTEXT (WM_APP + 1305)
#define GCM_SETTOOLTIP (WM_APP + 1306)
#define GCM_UPDATETOOLTIP (WM_APP + 1307)

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

// Notifications

#define GCN_GETDRAWINFO (WM_APP + 1351)
#define GCN_GETTOOLTIPTEXT (WM_APP + 1352)

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

#define PH_GRAPH_DATA_COUNT(Width, Step) (((Width) + (Step) - 1) / (Step) + 1) // round up in division

// Graph control state

typedef struct _PH_GRAPH_STATE
{
    PFLOAT Data1; // invalidate by setting Valid to FALSE
    PFLOAT Data2; // invalidate by setting Valid to FALSE
    ULONG AllocatedCount;
    BOOLEAN Valid; // indicates the data is valid
    PPH_STRING Text;

    PPH_STRING TooltipText; // invalidate by setting TooltipIndex to -1
    ULONG TooltipIndex; // indicates the tooltip text is valid for this index
} PH_GRAPH_STATE, *PPH_GRAPH_STATE;

VOID PhInitializeGraphState(
    __out PPH_GRAPH_STATE State
    );

VOID PhDeleteGraphState(
    __inout PPH_GRAPH_STATE State
    );

VOID PhGraphStateGetDrawInfo(
    __inout PPH_GRAPH_STATE State,
    __in PPH_GRAPH_GETDRAWINFO GetDrawInfo,
    __in ULONG DataCount
    );

#endif
