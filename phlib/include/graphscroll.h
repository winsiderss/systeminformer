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
#ifndef _PH_SCROLLNEW_H
#define _PH_SCROLLNEW_H

#include <guisup.h>

EXTERN_C_START

#define PH_SCROLLNEW_CLASSNAME L"PhScrollNew"

typedef enum _PH_SCROLLNEW_PART
{
    PhScrollNewPartNone,
    PhScrollNewPartUpButton,
    PhScrollNewPartDownButton,
    PhScrollNewPartPageUp,
    PhScrollNewPartPageDown,
    PhScrollNewPartThumb
} PH_SCROLLNEW_PART;

typedef struct _PH_SCROLLNEW_STATE
{
    // Configuration

    BOOLEAN Horizontal;
    LONG Minimum;
    LONG Maximum;
    LONG Page;
    LONG Position;

    // Internal State

    RECT Rect;
    PH_SCROLLNEW_PART HotPart;
    PH_SCROLLNEW_PART PressedPart;

    // TRUE while the mouse cursor is inside the scrollbar window's client area.
    // Used by the uxtheme draw path to gate arrow-button visibility (arrows are
    // only painted while the mouse is hovering the scrollbar). Maintained by
    // WM_MOUSEMOVE (set TRUE + TrackMouseEvent) and WM_MOUSELEAVE (cleared).

    BOOLEAN MouseInClient;
    POINT DragStartPoint;
    LONG DragStartPosition;

    // Metrics (calculated)

    RECT UpButtonRect;
    RECT DownButtonRect;
    RECT ThumbRect;
    RECT GutterRect;

    HANDLE ThemeHandle;

    // During thumb drag: visual drag position (ahead of committed Position).
    // Equals Position when not dragging.

    LONG TrackPosition;

    // TRUE while the WM_POINTER* handler is re-entering the wndproc with a
    // translated WM_LBUTTON*/WM_MOUSEMOVE; suppresses the touch-synthesized
    // mouse-message gate during that synchronous re-entry.
    
    BOOLEAN PointerReentrant;
} PH_SCROLLNEW_STATE, *PPH_SCROLLNEW_STATE;

static VOID PhScrollNewWndProcNotify(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE State,
    _In_ UINT ScrollCode,
    _In_ LONG NewPosition
    );

static VOID PhScrollNewPaintNow(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE State
    );

static HTHEME PhScrollNewOpenThemeData(
    _In_ HWND WindowHandle
    );

static VOID PhScrollNewCancelRepeat(
    _In_ HWND WindowHandle
    );

static VOID PhScrollNewRepeatPress(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE State
    );

static UINT PhScrollNewPartToScrollCode(
    _In_ PPH_SCROLLNEW_STATE State,
    _In_ PH_SCROLLNEW_PART Part
    );

static LRESULT CALLBACK PhScrollNewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

/**
 * Registers the PhScrollNew window class.
 * The class behaves as a drop-in replacement for WC_SCROLLBAR: it responds to
 * SBM_SETSCROLLINFO / SBM_GETSCROLLINFO and notifies its parent via WM_VSCROLL
 * or WM_HSCROLL. Thumb positions use SIF_TRACKPOS for the full 32-bit value.
 * Orientation is determined by the SBS_HORZ window style.
 */
RTL_ATOM PhScrollNewWindowInitialization(
    VOID
    );

VOID PhScrollNewInitialize(
    _Out_ PPH_SCROLLNEW_STATE State,
    _In_ BOOLEAN Horizontal
    );

VOID PhScrollNewUpdate(
    _Inout_ PPH_SCROLLNEW_STATE State,
    _In_ LONG Minimum,
    _In_ LONG Maximum,
    _In_ LONG Page,
    _In_ LONG Position
    );

VOID PhScrollNewLayout(
    _Inout_ PPH_SCROLLNEW_STATE State,
    _In_ LPCRECT Rect
    );

PH_SCROLLNEW_PART PhScrollNewHitTest(
    _In_ PPH_SCROLLNEW_STATE State,
    _In_ POINT Point
    );

VOID PhScrollNewDraw(
    _In_ PPH_SCROLLNEW_STATE State,
    _In_ HDC hdc,
    _In_opt_ HANDLE Theme
    );

BOOLEAN PhScrollNewHandleMessage(
    _Inout_ PPH_SCROLLNEW_STATE State,
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ PLONG NewPosition
    );

#if defined(_PHLIB_)
LRESULT PhScrollNewSendMessage(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _Pre_maybenull_ _Post_valid_ WPARAM wParam,
    _Pre_maybenull_ _Post_valid_ LPARAM lParam
    );
#define ScrollNew_SetScrollInfo(hWnd, Redraw, ScrollInfo) \
    PhScrollNewSendMessage((hWnd), SBM_SETSCROLLINFO, (WPARAM)!!(Redraw), (LPARAM)(ScrollInfo))
#define ScrollNew_GetScrollInfo(hWnd, ScrollInfo) \
    PhScrollNewSendMessage((hWnd), SBM_GETSCROLLINFO, 0, (LPARAM)(ScrollInfo))
#define ScrollNew_SetPos(hWnd, Pos, Redraw) \
    ((LONG)PhScrollNewSendMessage((hWnd), SBM_SETPOS, (WPARAM)(Pos), (LPARAM)!!(Redraw)))
#define ScrollNew_GetPos(hWnd) \
    ((LONG)PhScrollNewSendMessage((hWnd), SBM_GETPOS, 0, 0))
#define ScrollNew_SetRange(hWnd, Min, Max) \
    PhScrollNewSendMessage((hWnd), SBM_SETRANGE, (WPARAM)(Min), (LPARAM)(Max))
#define ScrollNew_SetRangeRedraw(hWnd, Min, Max) \
    PhScrollNewSendMessage((hWnd), SBM_SETRANGEREDRAW, (WPARAM)(Min), (LPARAM)(Max))
#define ScrollNew_GetRange(hWnd, PMin, PMax) \
    PhScrollNewSendMessage((hWnd), SBM_GETRANGE, (WPARAM)(PMin), (LPARAM)(PMax))
#if defined(PH_SCROLLNEW_MARKS)
#define ScrollNew_SetMark(hWnd, Position, Color) \
    PhScrollNewSendMessage((hWnd), PH_SCROLLNEW_SETMARK, (WPARAM)(Position), (LPARAM)(Color))
#define ScrollNew_ClearMarks(hWnd) \
    PhScrollNewSendMessage((hWnd), PH_SCROLLNEW_CLEARMARKS, 0, 0)
#endif
#else
#define ScrollNew_SetScrollInfo(hWnd, Redraw, ScrollInfo) \
    SendMessage((hWnd), SBM_SETSCROLLINFO, (WPARAM)!!(Redraw), (LPARAM)(ScrollInfo))
#define ScrollNew_GetScrollInfo(hWnd, ScrollInfo) \
    SendMessage((hWnd), SBM_GETSCROLLINFO, 0, (LPARAM)(ScrollInfo))
#define ScrollNew_SetPos(hWnd, Pos, Redraw) \
    ((LONG)SendMessage((hWnd), SBM_SETPOS, (WPARAM)(Pos), (LPARAM)!!(Redraw)))
#define ScrollNew_GetPos(hWnd) \
    ((LONG)SendMessage((hWnd), SBM_GETPOS, 0, 0))
#define ScrollNew_SetRange(hWnd, Min, Max) \
    SendMessage((hWnd), SBM_SETRANGE, (WPARAM)(Min), (LPARAM)(Max))
#define ScrollNew_SetRangeRedraw(hWnd, Min, Max) \
    SendMessage((hWnd), SBM_SETRANGEREDRAW, (WPARAM)(Min), (LPARAM)(Max))
#define ScrollNew_GetRange(hWnd, PMin, PMax) \
    SendMessage((hWnd), SBM_GETRANGE, (WPARAM)(PMin), (LPARAM)(PMax))
#if defined(PH_SCROLLNEW_MARKS)
#define ScrollNew_SetMark(hWnd, Position, Color) \
    SendMessage((hWnd), PH_SCROLLNEW_SETMARK, (WPARAM)(Position), (LPARAM)(Color))
#define ScrollNew_ClearMarks(hWnd) \
    SendMessage((hWnd), PH_SCROLLNEW_CLEARMARKS, 0, 0)
#endif
#endif

EXTERN_C_END

#endif
