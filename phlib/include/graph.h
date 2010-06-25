#ifndef _PH_GRAPH_H
#define _PH_GRAPH_H

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
    PWSTR Text;
    RECT TextRect;
    RECT TextBoxRect;
    COLORREF TextColor;
    COLORREF TextBoxColor;
} PH_GRAPH_DRAW_INFO, *PPH_GRAPH_DRAW_INFO;

#define PH_GRAPH_CLASSNAME L"PhGraph"

BOOLEAN PhGraphControlInitialization();

VOID PhDrawGraph(
    __in HDC hdc,
    __in PPH_GRAPH_DRAW_INFO DrawInfo
    );

HWND PhCreateGraphControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    );

#define GCM_GETDRAWINFO (WM_APP + 1301)
#define GCM_SETDRAWINFO (WM_APP + 1302)
#define GCM_DRAW (WM_APP + 1303)
#define GCM_MOVEGRID (WM_APP + 1304)

#define Graph_GetDrawInfo(hWnd, DrawInfo) \
    SendMessage((hWnd), GCM_GETDRAWINFO, 0, (LPARAM)(DrawInfo))
#define Graph_SetDrawInfo(hWnd, DrawInfo) \
    SendMessage((hWnd), GCM_SETDRAWINFO, 0, (LPARAM)(DrawInfo))
#define Graph_Draw(hWnd) \
    SendMessage((hWnd), GCM_DRAW, 0, 0)
#define Graph_MoveGrid(hWnd, Increment) \
    SendMessage((hWnd), GCM_MOVEGRID, (WPARAM)(Increment), 0)

#define PH_GRAPH_DATA_COUNT(Width, Step) ((Width) / (Step))

#endif
