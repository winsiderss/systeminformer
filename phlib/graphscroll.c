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
#include <vssym32.h>
#include <emenu.h>
#include <graphscroll.h>

// Context menu command IDs
#define PH_SCROLLNEW_IDM_SCROLL_HERE  1
#define PH_SCROLLNEW_IDM_TOP          2
#define PH_SCROLLNEW_IDM_BOTTOM       3
#define PH_SCROLLNEW_IDM_PAGE_UP      4
#define PH_SCROLLNEW_IDM_PAGE_DOWN    5
#define PH_SCROLLNEW_IDM_SCROLL_UP    6
#define PH_SCROLLNEW_IDM_SCROLL_DOWN  7

#define PH_SCROLLNEW_TIMER_ID         1
#define PH_SCROLLNEW_TIMER_DELAY      100
#define PH_SCROLLNEW_DARK_CLASSNAME   L"Explorer::Scrollbar"

// Touch/pen mouse-message synthesis signature (see GetMessageExtraInfo).
// Used to suppress synthesized mouse messages so the WM_POINTER* path can
// own touch/pen input without double-handling.
#define PH_SCROLLNEW_MI_WP_SIGNATURE  0xFF515700
#define PH_SCROLLNEW_MI_WP_MASK       0xFFFFFF00

static BOOLEAN PhScrollNewIsMouseFromTouch(
    _In_opt_ PPH_SCROLLNEW_STATE State
    )
{
    if (State && State->PointerReentrant)
        return FALSE;

    return (((ULONG_PTR)GetMessageExtraInfo()) & PH_SCROLLNEW_MI_WP_MASK) == PH_SCROLLNEW_MI_WP_SIGNATURE;
}

/**
 * Registers the PhScrollNew window class.
 */
RTL_ATOM PhScrollNewWindowInitialization(
    VOID
    )
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_GLOBALCLASS;// | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_PARENTDC;
    wcex.lpfnWndProc = PhScrollNewWndProc;
    wcex.cbWndExtra = sizeof(PVOID);
    wcex.hInstance = NtCurrentImageBase();
    wcex.hCursor = PhLoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = PH_SCROLLNEW_CLASSNAME;

    return RegisterClassEx(&wcex);
}

static HTHEME PhScrollNewOpenThemeData(
    _In_ HWND WindowHandle
    )
{
    if (PhEnableThemeSupport)
    {
        PhAllowDarkModeForWindow(WindowHandle, TRUE);
        return PhOpenThemeData(WindowHandle, PH_SCROLLNEW_DARK_CLASSNAME, 0);
    }

    return PhOpenThemeData(WindowHandle, VSCLASS_SCROLLBAR, 0);
}

/**
 * Window procedure for the PhScrollNew window class.
 *
 * Responds to SBM_SETSCROLLINFO / SBM_GETSCROLLINFO so that callers can use
 * the standard SetScrollInfo / GetScrollInfo API (with nBar = SB_CTL) as a
 * drop-in replacement for WC_SCROLLBAR child windows. Notifies the parent via
 * WM_VSCROLL or WM_HSCROLL, with 32-bit thumb positions exposed through
 * SBM_GETSCROLLINFO/SIF_TRACKPOS.
 */
static LRESULT CALLBACK PhScrollNewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SCROLLNEW_STATE state = (PPH_SCROLLNEW_STATE)PhGetWindowContextEx(WindowHandle);

    switch (WindowMessage)
    {
    case WM_NCCREATE:
        {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            BOOLEAN horizontal = !(cs->style & SBS_VERT); // SBS_HORZ == 0, so test for absence of SBS_VERT

            state = PhAllocateZero(sizeof(PH_SCROLLNEW_STATE));
            PhScrollNewInitialize(state, horizontal);
            PhSetWindowContextEx(WindowHandle, state);
        }
        return TRUE;
    case WM_CREATE:
        {
            state->ThemeHandle = PhScrollNewOpenThemeData(WindowHandle);
        }
        return 0;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContextEx(WindowHandle);

            if (state->ThemeHandle)
                PhCloseThemeData((HTHEME)state->ThemeHandle);

            PhFree(state);
        }
        break;
    case WM_SIZE:
        {
            RECT clientRect;

            PhGetClientRect(WindowHandle, &clientRect);

            PhScrollNewLayout(state, &clientRect);
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        return 0;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            HDC hdc;
            HDC bufferDc;
            PAINTSTRUCT paintStruct;
            PH_BUFFERED_PAINT bufferedPaint;

            if (hdc = BeginPaint(WindowHandle, &paintStruct))
            {
                // Only invoke the draw machinery when there is a non-empty update rectangle.
                if (paintStruct.rcPaint.right  > paintStruct.rcPaint.left &&
                    paintStruct.rcPaint.bottom > paintStruct.rcPaint.top)
                {
                    if (PhBeginBufferedPaint(hdc, &paintStruct.rcPaint, &bufferedPaint, &bufferDc))
                    {
                        PhScrollNewDraw(state, bufferDc, state->ThemeHandle);
                        PhEndBufferedPaint(&bufferedPaint, TRUE);
                    }
                    else
                    {
                        PhScrollNewDraw(state, hdc, state->ThemeHandle);
                    }
                }

                EndPaint(WindowHandle, &paintStruct);
            }
        }
        return 0;
    case WM_THEMECHANGED:
        {
            if (state->ThemeHandle)
            {
                PhCloseThemeData((HTHEME)state->ThemeHandle);
            }

            state->ThemeHandle = PhScrollNewOpenThemeData(WindowHandle);

            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        return 0;
    case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, WindowHandle, 0 };

            // Touch/pen-synthesized mouse messages are handled by WM_POINTER*.
            if (PhScrollNewIsMouseFromTouch(state))
                return 0;

            TrackMouseEvent(&tme);

            if (state)
            {
                BOOLEAN isDragging = (state->PressedPart == PhScrollNewPartThumb);
                LONG newPos;

                // Mark the mouse as inside the client area so the uxtheme draw
                // path can render the arrow buttons. Repaint on the rising edge.
                if (!state->MouseInClient)
                {
                    state->MouseInClient = TRUE;
                    InvalidateRect(WindowHandle, NULL, FALSE);
                }

                if (PhScrollNewHandleMessage(state, WindowHandle, WM_MOUSEMOVE, wParam, lParam, &newPos))
                {
                    if (isDragging)
                        PhScrollNewPaintNow(WindowHandle, state);

                    PhScrollNewWndProcNotify(WindowHandle, state, isDragging ? SB_THUMBTRACK : SB_THUMBPOSITION, newPos);
                }
            }
        }
        return 0;
    case WM_LBUTTONDOWN:
        {
            if (PhScrollNewIsMouseFromTouch(state))
                return 0;

            if (state)
            {
                LONG newPos;

                if (PhScrollNewHandleMessage(state, WindowHandle, WM_LBUTTONDOWN, wParam, lParam, &newPos))
                {
                    PhScrollNewWndProcNotify(
                        WindowHandle,
                        state,
                        PhScrollNewPartToScrollCode(state, state->PressedPart),
                        newPos
                        );

                    if (state->PressedPart != PhScrollNewPartThumb)
                        SetTimer(WindowHandle, PH_SCROLLNEW_TIMER_ID, PH_SCROLLNEW_TIMER_DELAY, NULL);
                }
            }
        }
        return 0;
    case WM_LBUTTONUP:
        {
            if (PhScrollNewIsMouseFromTouch(state))
                return 0;

            if (state)
            {
                BOOLEAN wasDragging = (state->PressedPart == PhScrollNewPartThumb);
                LONG finalPos = state->TrackPosition;
                LONG newPos = 0;

                PhScrollNewHandleMessage(state, WindowHandle, WM_LBUTTONUP, wParam, lParam, &newPos);
                PhScrollNewCancelRepeat(WindowHandle);

                if (wasDragging)
                    PhScrollNewWndProcNotify(WindowHandle, state, SB_THUMBPOSITION, finalPos);
            }
        }
        return 0;
    case WM_CAPTURECHANGED:
        {
            if (state)
            {
                PhScrollNewCancelRepeat(WindowHandle);
                state->PressedPart = PhScrollNewPartNone;
            }
        }
        return 0;
    case WM_MOUSELEAVE:
        {
            LONG newPos = 0;

            if (state)
            {
                // Mouse has left the client area; the uxtheme draw path will
                // stop rendering the arrow buttons on the next paint.
                if (state->MouseInClient)
                {
                    state->MouseInClient = FALSE;
                    InvalidateRect(WindowHandle, NULL, FALSE);
                }

                PhScrollNewHandleMessage(
                    state,
                    WindowHandle,
                    WM_MOUSELEAVE,
                    wParam,
                    lParam,
                    &newPos
                );
            }
        }
        return 0;
    case WM_TIMER:
        {
            if (state && wParam == PH_SCROLLNEW_TIMER_ID && state->PressedPart != PhScrollNewPartNone)
                PhScrollNewRepeatPress(WindowHandle, state);
        }
        return 0;
    case WM_POINTERDOWN:
    case WM_POINTERUPDATE:
    case WM_POINTERUP:
        {
            // Route touch/pen pointer input through the existing mouse state machine.
            // Synthesized WM_LBUTTON*/WM_MOUSEMOVE messages for the same input are
            // suppressed via the PhScrollNewIsMouseFromTouch(state) gates above.
            UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
            POINTER_INFO info;

            if (state &&
                GetPointerInfo(pointerId, &info) &&
                (info.pointerType == PT_TOUCH || info.pointerType == PT_PEN))
            {
                POINT pt = info.ptPixelLocation;
                UINT mappedMsg;

                ScreenToClient(WindowHandle, &pt);

                if (WindowMessage == WM_POINTERDOWN)
                    mappedMsg = WM_LBUTTONDOWN;
                else if (WindowMessage == WM_POINTERUP)
                    mappedMsg = WM_LBUTTONUP;
                else
                    mappedMsg = WM_MOUSEMOVE;

                LRESULT result;

                // Pointer contacts are implicitly captured by the window they
                // were pressed in until lifted, so no explicit capture call is
                // needed for thumb-drag past the scrollbar's client area.
                state->PointerReentrant = TRUE;
                result = PhScrollNewWndProc(
                    WindowHandle,
                    mappedMsg,
                    MK_LBUTTON,
                    MAKELPARAM((WORD)pt.x, (WORD)pt.y)
                    );
                state->PointerReentrant = FALSE;

                return result;
            }
        }
        break;
    case WM_POINTERCAPTURECHANGED:
        {
            if (state)
            {
                PhScrollNewCancelRepeat(WindowHandle);
                state->PressedPart = PhScrollNewPartNone;
                InvalidateRect(WindowHandle, NULL, FALSE);
            }
        }
        return 0;
    case SBM_SETSCROLLINFO:
        {
            LPSCROLLINFO si = (LPSCROLLINFO)lParam;
            BOOLEAN redraw = !!wParam;
            LONG min = state->Minimum;
            LONG max = state->Maximum;
            LONG page = state->Page;
            LONG pos = state->Position;

            if (si->fMask & SIF_RANGE) { min = si->nMin; max = si->nMax; }
            if (si->fMask & SIF_PAGE)  page = (LONG)si->nPage;
            if (si->fMask & SIF_POS)   pos = si->nPos;

            PhScrollNewUpdate(state, min, max, page, pos);

            if (redraw)
            {
                // WM_ERASEBKGND is suppressed unconditionally; FALSE avoids the extra erase dispatch.
                InvalidateRect(WindowHandle, NULL, FALSE);
            }

            return state->Position;
        }
        break;
    case SBM_GETSCROLLINFO:
        {
            if (state)
            {
                LPSCROLLINFO si = (LPSCROLLINFO)lParam;
                if (si->fMask & SIF_RANGE) { si->nMin = state->Minimum; si->nMax = state->Maximum; }
                if (si->fMask & SIF_PAGE)       si->nPage = (UINT)state->Page;
                if (si->fMask & SIF_POS)        si->nPos = state->Position;
                if (si->fMask & SIF_TRACKPOS)   si->nTrackPos = state->TrackPosition;
                return TRUE;
            }
        }
        break;
    case SBM_SETPOS:
        {
            LONG oldPos = state->Position;

            PhScrollNewUpdate(
                state,
                state->Minimum,
                state->Maximum,
                state->Page,
                (LONG)wParam
                );

            if (lParam)
            {
                InvalidateRect(WindowHandle, NULL, FALSE);
            }

            return oldPos;
        }
        break;
    case SBM_GETPOS:
        {
            return state->Position;
        }
        break;
    case SBM_SETRANGE:
        {
            PhScrollNewUpdate(
                state,
                (LONG)wParam,
                (LONG)lParam,
                state->Page,
                state->Position
                );
        }
        break;
    case SBM_SETRANGEREDRAW:
        {
            PhScrollNewUpdate(
                state,
                (LONG)wParam,
                (LONG)lParam,
                state->Page,
                state->Position
                );
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case SBM_GETRANGE:
        {
            if (wParam)
            {
                *(PLONG)wParam = state->Minimum;
            }
            if (lParam)
            {
                *(PLONG)lParam = state->Maximum;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if (state)
            {
                BOOLEAN isHorz = state->Horizontal;
                POINT screenPt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                POINT clientPt = screenPt;
                LONG scrollHerePos;
                PPH_EMENU menu;
                PPH_EMENU_ITEM selectedItem;

                ScreenToClient(WindowHandle, &clientPt);

                // Calculate "Scroll Here" position from click point within the gutter.
                {
                    LONG gutterLen = isHorz
                        ? (state->GutterRect.right  - state->GutterRect.left)
                        : (state->GutterRect.bottom - state->GutterRect.top);
                    LONG clickOff = isHorz
                        ? (clientPt.x - state->GutterRect.left)
                        : (clientPt.y - state->GutterRect.top);
                    LONG scrollRange = state->Maximum - state->Minimum - state->Page + 1;

                    if (gutterLen > 0 && scrollRange > 0)
                        scrollHerePos = state->Minimum + (LONG)((__int64)clickOff * scrollRange / gutterLen);
                    else
                        scrollHerePos = state->Position;

                    if (scrollHerePos < state->Minimum) scrollHerePos = state->Minimum;
                    if (scrollHerePos > state->Maximum - state->Page + 1) scrollHerePos = state->Maximum - state->Page + 1;
                }

                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_SCROLLNEW_IDM_SCROLL_HERE, L"Scroll Here", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_SCROLLNEW_IDM_TOP,         isHorz ? L"Left Edge"    : L"Top",         NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_SCROLLNEW_IDM_BOTTOM,      isHorz ? L"Right Edge"   : L"Bottom",      NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_SCROLLNEW_IDM_PAGE_UP,     isHorz ? L"Page Left"    : L"Page Up",     NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_SCROLLNEW_IDM_PAGE_DOWN,   isHorz ? L"Page Right"   : L"Page Down",   NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_SCROLLNEW_IDM_SCROLL_UP,   isHorz ? L"Scroll Left"  : L"Scroll Up",   NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_SCROLLNEW_IDM_SCROLL_DOWN, isHorz ? L"Scroll Right" : L"Scroll Down", NULL, NULL), ULONG_MAX);

                selectedItem = PhShowEMenu(
                    menu,
                    WindowHandle,
                    PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    screenPt.x,
                    screenPt.y
                    );

                if (selectedItem)
                {
                    switch (selectedItem->Id)
                    {
                    case PH_SCROLLNEW_IDM_SCROLL_HERE:
                        PhScrollNewWndProcNotify(WindowHandle, state, SB_THUMBTRACK, scrollHerePos);
                        break;
                    case PH_SCROLLNEW_IDM_TOP:
                        PhScrollNewWndProcNotify(WindowHandle, state, SB_TOP, 0);
                        break;
                    case PH_SCROLLNEW_IDM_BOTTOM:
                        PhScrollNewWndProcNotify(WindowHandle, state, SB_BOTTOM, 0);
                        break;
                    case PH_SCROLLNEW_IDM_PAGE_UP:
                        PhScrollNewWndProcNotify(WindowHandle, state, SB_PAGEUP, 0);
                        break;
                    case PH_SCROLLNEW_IDM_PAGE_DOWN:
                        PhScrollNewWndProcNotify(WindowHandle, state, SB_PAGEDOWN, 0);
                        break;
                    case PH_SCROLLNEW_IDM_SCROLL_UP:
                        PhScrollNewWndProcNotify(WindowHandle, state, SB_LINEUP, 0);
                        break;
                    case PH_SCROLLNEW_IDM_SCROLL_DOWN:
                        PhScrollNewWndProcNotify(WindowHandle, state, SB_LINEDOWN, 0);
                        break;
                    }
                }

                PhDestroyEMenu(menu);
            }
        }
        return 0;
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

static VOID PhScrollNewPaintNow(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE State
    )
{
    HDC hdc;
    HDC bufferDc;
    PH_BUFFERED_PAINT bufferedPaint;

    if (PhRectEmpty(&State->Rect))
        return;

    hdc = GetDC(WindowHandle);

    if (!hdc)
        return;

    if (PhBeginBufferedPaint(hdc, &State->Rect, &bufferedPaint, &bufferDc))
    {
        PhScrollNewDraw(State, bufferDc, State->ThemeHandle);
        PhEndBufferedPaint(&bufferedPaint, TRUE);
    }
    else
    {
        PhScrollNewDraw(State, hdc, State->ThemeHandle);
    }

    ReleaseDC(WindowHandle, hdc);
    ValidateRect(WindowHandle, &State->Rect);
}

static VOID PhScrollNewCancelRepeat(
    _In_ HWND WindowHandle
    )
{
    KillTimer(WindowHandle, PH_SCROLLNEW_TIMER_ID);
}

static VOID PhScrollNewRepeatPress(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE State
    )
{
    LONG newPos;
    UINT scrollCode;

    // For page-up/page-down auto-repeat, stop when the thumb has scrolled
    // under (or past) the mouse cursor. This matches the standard Windows
    // scrollbar behavior — otherwise the thumb keeps marching past the mouse.
    if (State->PressedPart == PhScrollNewPartPageUp ||
        State->PressedPart == PhScrollNewPartPageDown)
    {
        POINT cursor;

        if (GetCursorPos(&cursor) && ScreenToClient(WindowHandle, &cursor))
        {
            PH_SCROLLNEW_PART hit = PhScrollNewHitTest(State, cursor);

            if (hit != State->PressedPart)
            {
                PhScrollNewCancelRepeat(WindowHandle);
                return;
            }
        }
    }

    switch (State->PressedPart)
    {
    case PhScrollNewPartUpButton:
        scrollCode = PhScrollNewPartToScrollCode(State, State->PressedPart);
        newPos = State->Position - 1;
        break;
    case PhScrollNewPartDownButton:
        scrollCode = PhScrollNewPartToScrollCode(State, State->PressedPart);
        newPos = State->Position + 1;
        break;
    case PhScrollNewPartPageUp:
        scrollCode = PhScrollNewPartToScrollCode(State, State->PressedPart);
        newPos = State->Position - State->Page;
        break;
    case PhScrollNewPartPageDown:
        scrollCode = PhScrollNewPartToScrollCode(State, State->PressedPart);
        newPos = State->Position + State->Page;
        break;
    default:
        return;
    }

    PhScrollNewWndProcNotify(WindowHandle, State, scrollCode, newPos);
}

// Internal helper: notify parent window of a position change.
static VOID PhScrollNewWndProcNotify(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE State,
    _In_ UINT ScrollCode,
    _In_ LONG NewPosition
    )
{
    HWND parent = GetParent(WindowHandle);

    if (!parent)
        return;

    if (ScrollCode == SB_THUMBTRACK || ScrollCode == SB_THUMBPOSITION)
        State->TrackPosition = NewPosition;

    if (State->Horizontal)
        SendMessage(parent, WM_HSCROLL, MAKEWPARAM(ScrollCode, (WORD)NewPosition), (LPARAM)WindowHandle);
    else
        SendMessage(parent, WM_VSCROLL, MAKEWPARAM(ScrollCode, (WORD)NewPosition), (LPARAM)WindowHandle);
}

static UINT PhScrollNewPartToScrollCode(
    _In_ PPH_SCROLLNEW_STATE State,
    _In_ PH_SCROLLNEW_PART Part
    )
{
    switch (Part)
    {
    case PhScrollNewPartUpButton:
        return State->Horizontal ? SB_LINELEFT : SB_LINEUP;
    case PhScrollNewPartDownButton:
        return State->Horizontal ? SB_LINERIGHT : SB_LINEDOWN;
    case PhScrollNewPartPageUp:
        return State->Horizontal ? SB_PAGELEFT : SB_PAGEUP;
    case PhScrollNewPartPageDown:
        return State->Horizontal ? SB_PAGERIGHT : SB_PAGEDOWN;
    default:
        return SB_THUMBPOSITION;
    }
}

VOID PhScrollNewInitialize(
    _Out_ PPH_SCROLLNEW_STATE State,
    _In_ BOOLEAN Horizontal
    )
{
    memset(State, 0, sizeof(PH_SCROLLNEW_STATE));
    State->Horizontal = Horizontal;

    State->HotPart = PhScrollNewPartNone;
    State->PressedPart = PhScrollNewPartNone;
}

VOID PhScrollNewUpdate(
    _Inout_ PPH_SCROLLNEW_STATE State,
    _In_ LONG Minimum,
    _In_ LONG Maximum,
    _In_ LONG Page,
    _In_ LONG Position
    )
{
    State->Minimum = Minimum;
    State->Maximum = Maximum;
    State->Page = Page;
    State->Position = Position;

    if (State->Position < State->Minimum)
        State->Position = State->Minimum;
    if (State->Position > State->Maximum - State->Page + 1)
        State->Position = State->Maximum - State->Page + 1;
    if (State->Position < State->Minimum)
        State->Position = State->Minimum;

    // Keep TrackPosition in sync unless a thumb drag is active.
    if (State->PressedPart != PhScrollNewPartThumb)
        State->TrackPosition = State->Position;

    PhScrollNewLayout(State, &State->Rect);
}

VOID PhScrollNewLayout(
    _Inout_ PPH_SCROLLNEW_STATE State,
    _In_ LPCRECT Rect
    )
{
    LONG width, height;
    LONG buttonSize;
    LONG range;
    LONG gutterLength;
    LONG thumbLength;

    State->Rect = *Rect;
    width = Rect->right - Rect->left;
    height = Rect->bottom - Rect->top;

    if (State->Horizontal)
    {
        buttonSize = height;
        State->UpButtonRect = (RECT){ Rect->left, Rect->top, Rect->left + buttonSize, Rect->bottom };
        State->DownButtonRect = (RECT){ Rect->right - buttonSize, Rect->top, Rect->right, Rect->bottom };
        State->GutterRect = (RECT){ State->UpButtonRect.right, Rect->top, State->DownButtonRect.left, Rect->bottom };
        gutterLength = State->GutterRect.right - State->GutterRect.left;
    }
    else
    {
        buttonSize = width;
        State->UpButtonRect = (RECT){ Rect->left, Rect->top, Rect->right, Rect->top + buttonSize };
        State->DownButtonRect = (RECT){ Rect->left, Rect->bottom - buttonSize, Rect->right, Rect->bottom };
        State->GutterRect = (RECT){ Rect->left, State->UpButtonRect.bottom, Rect->right, State->DownButtonRect.top };
        gutterLength = State->GutterRect.bottom - State->GutterRect.top;
    }

    range = State->Maximum - State->Minimum + 1;

    if (range > 0 && State->Page > 0 && range > State->Page)
    {
        thumbLength = PhMultiplyDivideSigned(gutterLength, State->Page, range);

        if (thumbLength < buttonSize)
            thumbLength = buttonSize;

        LONG scrollRange = range - State->Page;
        LONG thumbScrollArea = gutterLength - thumbLength;
        LONG displayPos = (State->PressedPart == PhScrollNewPartThumb) ? State->TrackPosition : State->Position;
        LONG thumbOffset = (scrollRange > 0) ? PhMultiplyDivideSigned(displayPos, thumbScrollArea, scrollRange) : 0;

        if (State->Horizontal)
        {
            State->ThumbRect = (RECT)
            {
                State->GutterRect.left + thumbOffset,
                Rect->top,
                State->GutterRect.left + thumbOffset + thumbLength,
                Rect->bottom
            };
        }
        else
        {
            State->ThumbRect = (RECT)
            {
                Rect->left,
                State->GutterRect.top + thumbOffset,
                Rect->right,
                State->GutterRect.top + thumbOffset + thumbLength
            };
        }
    }
    else
    {
        memset(&State->ThumbRect, 0, sizeof(RECT));
    }
}

PH_SCROLLNEW_PART PhScrollNewHitTest(
    _In_ PPH_SCROLLNEW_STATE State,
    _In_ POINT Point
    )
{
    if (PhPtInRect(&State->ThumbRect, &Point))
        return PhScrollNewPartThumb;

    if (PhPtInRect(&State->UpButtonRect, &Point))
        return PhScrollNewPartUpButton;

    if (PhPtInRect(&State->DownButtonRect, &Point))
        return PhScrollNewPartDownButton;

    if (PhPtInRect(&State->GutterRect, &Point))
    {
        if (State->Horizontal)
            return (Point.x < State->ThumbRect.left) ? PhScrollNewPartPageUp : PhScrollNewPartPageDown;
        else
            return (Point.y < State->ThumbRect.top) ? PhScrollNewPartPageUp : PhScrollNewPartPageDown;
    }

    return PhScrollNewPartNone;
}

#if defined(PH_SCROLLNEW_MARKS)
static VOID PhScrollNewDrawMarks(
    _In_ PPH_SCROLLNEW_STATE State,
    _In_ HDC Hdc
    )
{
    LONG range;
    LONG gutterLength;
    LONG markerArea;
    HBRUSH dcBrush;

    if (!State->NumberOfMarks)
        return;

    if (PhRectEmpty(&State->GutterRect))
        return;

    range = State->Maximum - State->Minimum + 1;

    if (range <= 0)
        return;

    gutterLength = State->Horizontal ?
        State->GutterRect.right - State->GutterRect.left :
        State->GutterRect.bottom - State->GutterRect.top;

    if (gutterLength <= 0)
        return;

    markerArea = gutterLength - 1;
    dcBrush = PhGetStockBrush(DC_BRUSH);

    for (ULONG i = 0; i < State->NumberOfMarks; i++)
    {
        LONG position = State->Marks[i].Position;
        LONG offset;
        RECT markerRect;

        if (position < State->Minimum)
            position = State->Minimum;
        if (position > State->Maximum)
            position = State->Maximum;

        offset = range > 1 ? PhMultiplyDivideSigned(position - State->Minimum, markerArea, range - 1) : 0;

        if (State->Horizontal)
        {
            markerRect.left = State->GutterRect.left + offset;
            markerRect.top = State->GutterRect.top;
            markerRect.right = markerRect.left + 2;
            markerRect.bottom = State->GutterRect.bottom;
        }
        else
        {
            markerRect.left = State->GutterRect.left;
            markerRect.top = State->GutterRect.top + offset;
            markerRect.right = State->GutterRect.right;
            markerRect.bottom = markerRect.top + 2;
        }

        if (markerRect.right > State->GutterRect.right)
            markerRect.right = State->GutterRect.right;
        if (markerRect.bottom > State->GutterRect.bottom)
            markerRect.bottom = State->GutterRect.bottom;

        SetDCBrushColor(Hdc, State->Marks[i].Color);
        FillRect(Hdc, &markerRect, dcBrush);
    }
}
#endif

// Windows 10 flat scrollbar colors
#define PH_SCROLLNEW_COLOR_TRACK          RGB(240, 240, 240)
#define PH_SCROLLNEW_COLOR_ARROW_BTN_HOT  RGB(218, 218, 218)
#define PH_SCROLLNEW_COLOR_ARROW_BTN_PRESS RGB(198, 198, 198)
#define PH_SCROLLNEW_COLOR_THUMB_NORMAL   RGB(205, 205, 205)
#define PH_SCROLLNEW_COLOR_THUMB_HOT      RGB(166, 166, 166)
#define PH_SCROLLNEW_COLOR_THUMB_PRESSED  RGB(96,  96,  96)
#define PH_SCROLLNEW_COLOR_ARROW_NORMAL   RGB(198, 198, 198)
#define PH_SCROLLNEW_COLOR_ARROW_HOT      RGB(0,   0,   0)

#define PH_SCROLLNEW_DARK_COLOR_TRACK           RGB(23, 23, 23)
#define PH_SCROLLNEW_DARK_COLOR_ARROW_BTN_HOT   RGB(65, 65, 65)
#define PH_SCROLLNEW_DARK_COLOR_ARROW_BTN_PRESS RGB(78, 78, 78)
#define PH_SCROLLNEW_DARK_COLOR_THUMB_NORMAL    RGB(95, 95, 95)
#define PH_SCROLLNEW_DARK_COLOR_THUMB_HOT       RGB(128, 128, 128)
#define PH_SCROLLNEW_DARK_COLOR_THUMB_PRESSED   RGB(143, 143, 143)
#define PH_SCROLLNEW_DARK_COLOR_ARROW_NORMAL    RGB(155, 155, 155)
#define PH_SCROLLNEW_DARK_COLOR_ARROW_HOT       RGB(255, 255, 255)

/**
 * Draws a small filled triangle arrow centered in ButtonRect.
 * Requires DC_BRUSH and DC_PEN to already be selected into hdc by the caller.
 * Uses SetDCBrushColor/SetDCPenColor so no GDI object allocation occurs.
 */
static VOID PhScrollNewDrawArrow(
    _In_ HDC hdc,
    _In_ const RECT *ButtonRect,
    _In_ BOOLEAN Horizontal,
    _In_ BOOLEAN Forward,
    _In_ COLORREF Color
    )
{
    const LONG s = 4; // triangle half-base / depth
    LONG cx = (ButtonRect->left + ButtonRect->right) / 2;
    LONG cy = (ButtonRect->top  + ButtonRect->bottom) / 2;
    POINT pts[3];

    if (!Horizontal)
    {
        if (!Forward) // up
        {
            pts[0].x = cx;     pts[0].y = cy - s + 1;
            pts[1].x = cx - s; pts[1].y = cy + s / 2;
            pts[2].x = cx + s; pts[2].y = cy + s / 2;
        }
        else // down
        {
            pts[0].x = cx;     pts[0].y = cy + s - 1;
            pts[1].x = cx - s; pts[1].y = cy - s / 2;
            pts[2].x = cx + s; pts[2].y = cy - s / 2;
        }
    }
    else
    {
        if (!Forward) // left
        {
            pts[0].x = cx - s + 1; pts[0].y = cy;
            pts[1].x = cx + s / 2; pts[1].y = cy - s;
            pts[2].x = cx + s / 2; pts[2].y = cy + s;
        }
        else // right
        {
            pts[0].x = cx + s - 1; pts[0].y = cy;
            pts[1].x = cx - s / 2; pts[1].y = cy - s;
            pts[2].x = cx - s / 2; pts[2].y = cy + s;
        }
    }

    SetDCBrushColor(hdc, Color);
    SetDCPenColor(hdc, Color);
    Polygon(hdc, pts, 3);
}

VOID PhScrollNewDraw(
    _In_ PPH_SCROLLNEW_STATE State,
    _In_ HDC hdc,
    _In_opt_ HANDLE Theme
    )
{
    HTHEME hTheme = (HTHEME)Theme;
    BOOLEAN vertical = !State->Horizontal;
    BOOLEAN hasThumb = State->ThumbRect.right > State->ThumbRect.left &&
                       State->ThumbRect.bottom > State->ThumbRect.top;

    if (hTheme)
    {
        INT upBtnState;
        INT downBtnState;
        INT trackLeadPart;
        INT trackTrailPart;
        INT thumbPart;
        BOOLEAN themeDrawSucceeded = TRUE;

        // LOWER = leading track (before the thumb); UPPER = trailing track (after the thumb).
        trackLeadPart  = vertical ? SBP_LOWERTRACKVERT : SBP_LOWERTRACKHORZ;
        trackTrailPart = vertical ? SBP_UPPERTRACKVERT : SBP_UPPERTRACKHORZ;
        thumbPart      = vertical ? SBP_THUMBBTNVERT   : SBP_THUMBBTNHORZ;

        // Arrows are only painted while the mouse is hovering anywhere inside
        // the scrollbar's client area (or a part is captured via a press).
        // While hovering, draw the arrows in the HOT-tier state (the modern
        // Explorer::Scrollbar theme renders the chevron near-invisible in the
        // NORMAL state, so we promote to HOT to keep the glyphs visible) and
        // only escalate to PRESSED when the specific button is captured.
        // While idle, fill the button rects with the gutter track so no arrow
        // glyph is drawn.
        if (State->MouseInClient || State->PressedPart != PhScrollNewPartNone)
        {
            if (vertical)
            {
                upBtnState   = (State->PressedPart == PhScrollNewPartUpButton)   ? ABS_UPPRESSED   : ABS_UPHOT;
                downBtnState = (State->PressedPart == PhScrollNewPartDownButton) ? ABS_DOWNPRESSED : ABS_DOWNHOT;
            }
            else
            {
                upBtnState   = (State->PressedPart == PhScrollNewPartUpButton)   ? ABS_LEFTPRESSED  : ABS_LEFTHOT;
                downBtnState = (State->PressedPart == PhScrollNewPartDownButton) ? ABS_RIGHTPRESSED : ABS_RIGHTHOT;
            }

            themeDrawSucceeded &= PhDrawThemeBackground(hTheme, hdc, SBP_ARROWBTN, upBtnState, &State->UpButtonRect, NULL);
            themeDrawSucceeded &= PhDrawThemeBackground(hTheme, hdc, SBP_ARROWBTN, downBtnState, &State->DownButtonRect, NULL);
        }
        else
        {
            themeDrawSucceeded &= PhDrawThemeBackground(hTheme, hdc, trackLeadPart, SCRBS_NORMAL, &State->UpButtonRect, NULL);
            themeDrawSucceeded &= PhDrawThemeBackground(hTheme, hdc, trackTrailPart, SCRBS_NORMAL, &State->DownButtonRect, NULL);
        }

        if (hasThumb)
        {
            RECT leadRect;
            RECT trailRect;
            INT trackState;
            INT thumbState;

            //
            // Leading track (above / left of thumb)
            //

            if (vertical)
                PhSetRect(&leadRect, State->GutterRect.left, State->GutterRect.top, State->GutterRect.right, State->ThumbRect.top);
            else
                PhSetRect(&leadRect, State->GutterRect.left, State->GutterRect.top, State->ThumbRect.left, State->GutterRect.bottom);

            if (!PhRectEmpty(&leadRect))
            {
                trackState = (State->HotPart == PhScrollNewPartPageUp) ? SCRBS_HOT : SCRBS_NORMAL;
                themeDrawSucceeded &= PhDrawThemeBackground(hTheme, hdc, trackLeadPart, trackState, &leadRect, NULL);
            }

            //
            // Trailing track (below / right of thumb)
            //

            if (vertical)
                PhSetRect(&trailRect, State->GutterRect.left, State->ThumbRect.bottom, State->GutterRect.right, State->GutterRect.bottom);
            else
                PhSetRect(&trailRect, State->ThumbRect.right, State->GutterRect.top, State->GutterRect.right, State->GutterRect.bottom);

            if (!PhRectEmpty(&trailRect))
            {
                trackState = (State->HotPart == PhScrollNewPartPageDown) ? SCRBS_HOT : SCRBS_NORMAL;
                themeDrawSucceeded &= PhDrawThemeBackground(hTheme, hdc, trackTrailPart, trackState, &trailRect, NULL);
            }

            //
            // Thumb
            //

            thumbState = (State->PressedPart == PhScrollNewPartThumb) ? SCRBS_PRESSED :
                         (State->HotPart     == PhScrollNewPartThumb) ? SCRBS_HOT     : SCRBS_NORMAL;
            themeDrawSucceeded &= PhDrawThemeBackground(hTheme, hdc, thumbPart, thumbState, &State->ThumbRect, NULL);
        }
        else
        {
            // No thumb — fill the full gutter as a disabled track.
            if (!PhRectEmpty(&State->GutterRect))
            {
                themeDrawSucceeded &= PhDrawThemeBackground(hTheme, hdc, trackLeadPart, SCRBS_DISABLED, &State->GutterRect, NULL);
            }
        }

        if (themeDrawSucceeded)
            return;
    }

    {
        // Windows 10 flat scrollbar — no 3D borders, muted grays, DC_BRUSH/DC_PEN for zero allocation.
        HBRUSH dcBrush = PhGetStockBrush(DC_BRUSH);
        BOOLEAN upHot   = (State->HotPart    == PhScrollNewPartUpButton);
        BOOLEAN upPress = (State->PressedPart == PhScrollNewPartUpButton);
        BOOLEAN dnHot   = (State->HotPart    == PhScrollNewPartDownButton);
        BOOLEAN dnPress = (State->PressedPart == PhScrollNewPartDownButton);
        COLORREF trackColor = PhEnableThemeSupport ? PH_SCROLLNEW_DARK_COLOR_TRACK : PH_SCROLLNEW_COLOR_TRACK;
        COLORREF arrowButtonHotColor = PhEnableThemeSupport ? PH_SCROLLNEW_DARK_COLOR_ARROW_BTN_HOT : PH_SCROLLNEW_COLOR_ARROW_BTN_HOT;
        COLORREF arrowButtonPressColor = PhEnableThemeSupport ? PH_SCROLLNEW_DARK_COLOR_ARROW_BTN_PRESS : PH_SCROLLNEW_COLOR_ARROW_BTN_PRESS;
        COLORREF arrowNormalColor = PhEnableThemeSupport ? PH_SCROLLNEW_DARK_COLOR_ARROW_NORMAL : PH_SCROLLNEW_COLOR_ARROW_NORMAL;
        COLORREF arrowHotColor = PhEnableThemeSupport ? PH_SCROLLNEW_DARK_COLOR_ARROW_HOT : PH_SCROLLNEW_COLOR_ARROW_HOT;
        COLORREF thumbNormalColor = PhEnableThemeSupport ? PH_SCROLLNEW_DARK_COLOR_THUMB_NORMAL : PH_SCROLLNEW_COLOR_THUMB_NORMAL;
        COLORREF thumbHotColor = PhEnableThemeSupport ? PH_SCROLLNEW_DARK_COLOR_THUMB_HOT : PH_SCROLLNEW_COLOR_THUMB_HOT;
        COLORREF thumbPressedColor = PhEnableThemeSupport ? PH_SCROLLNEW_DARK_COLOR_THUMB_PRESSED : PH_SCROLLNEW_COLOR_THUMB_PRESSED;

        // Track background
        SetDCBrushColor(hdc, trackColor);
        FillRect(hdc, &State->GutterRect, dcBrush);

        // Up / Left arrow button background
        SetDCBrushColor(hdc, upPress ? arrowButtonPressColor : upHot ? arrowButtonHotColor : trackColor);
        FillRect(hdc, &State->UpButtonRect, dcBrush);

        // Down / Right arrow button background
        SetDCBrushColor(hdc, dnPress ? arrowButtonPressColor : dnHot ? arrowButtonHotColor : trackColor);
        FillRect(hdc, &State->DownButtonRect, dcBrush);

        // Pre-select DC_BRUSH and DC_PEN once for both Polygon calls; avoids 6 redundant
        // SelectObject round-trips (4 select + 4 restore -> 2 select + 2 restore per frame).
        {
            HGDIOBJ prevBrush = SelectBrush(hdc, dcBrush);
            HGDIOBJ prevPen   = SelectPen(hdc, PhGetStockPen(DC_PEN));

            PhScrollNewDrawArrow(hdc, &State->UpButtonRect, State->Horizontal, FALSE, (upHot || upPress) ? arrowHotColor : arrowNormalColor);
            PhScrollNewDrawArrow(hdc, &State->DownButtonRect, State->Horizontal, TRUE, (dnHot || dnPress) ? arrowHotColor : arrowNormalColor);

            SelectBrush(hdc, prevBrush);
            SelectPen(hdc, prevPen);
        }

        // Thumb
        if (hasThumb)
        {
            COLORREF thumbColor =
                (State->PressedPart == PhScrollNewPartThumb) ? thumbPressedColor :
                (State->HotPart     == PhScrollNewPartThumb) ? thumbHotColor     :
                                                               thumbNormalColor;
            SetDCBrushColor(hdc, thumbColor);
            FillRect(hdc, &State->ThumbRect, dcBrush);
        }
    }
}

BOOLEAN PhScrollNewHandleMessage(
    _Inout_ PPH_SCROLLNEW_STATE State,
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ PLONG NewPosition
    )
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    BOOLEAN changed = FALSE;

    switch (WindowMessage)
    {
    case WM_MOUSEMOVE:
        {
            PH_SCROLLNEW_PART hit = PhScrollNewHitTest(State, pt);

            if (State->HotPart != hit)
            {
                State->HotPart = hit;
                InvalidateRect(WindowHandle, &State->Rect, FALSE);
            }

            if (State->PressedPart == PhScrollNewPartThumb)
            {
                LONG gutterLength, thumbLength, thumbScrollArea, range, scrollRange;
                LONG deltaPixels;

                range = State->Maximum - State->Minimum + 1;
                scrollRange = range - State->Page;

                if (State->Horizontal)
                {
                    gutterLength = State->GutterRect.right - State->GutterRect.left;
                    thumbLength = State->ThumbRect.right - State->ThumbRect.left;
                    deltaPixels = pt.x - State->DragStartPoint.x;
                }
                else
                {
                    gutterLength = State->GutterRect.bottom - State->GutterRect.top;
                    thumbLength = State->ThumbRect.bottom - State->ThumbRect.top;
                    deltaPixels = pt.y - State->DragStartPoint.y;
                }

                thumbScrollArea = gutterLength - thumbLength;
                if (thumbScrollArea > 0)
                {
                    LONG deltaPos = PhMultiplyDivideSigned(deltaPixels, scrollRange, thumbScrollArea);
                    LONG pos = State->DragStartPosition + deltaPos;

                    if (pos < State->Minimum) pos = State->Minimum;
                    if (pos > State->Maximum - State->Page + 1) pos = State->Maximum - State->Page + 1;

                    if (State->TrackPosition != pos)
                    {
                        *NewPosition = pos;
                        changed = TRUE;
                    }
                }
            }
        }
        break;

    case WM_LBUTTONDOWN:
        {
            State->PressedPart = PhScrollNewHitTest(State, pt);

            if (State->PressedPart != PhScrollNewPartNone)
            {
                SetCapture(WindowHandle);
                State->DragStartPoint = pt;
                State->DragStartPosition = State->Position;
                State->TrackPosition = State->Position;

                if (State->PressedPart == PhScrollNewPartUpButton)
                {
                    *NewPosition = State->Position - 1;
                    changed = TRUE;
                }
                else if (State->PressedPart == PhScrollNewPartDownButton)
                {
                    *NewPosition = State->Position + 1;
                    changed = TRUE;
                }
                else if (State->PressedPart == PhScrollNewPartPageUp)
                {
                    *NewPosition = State->Position - State->Page;
                    changed = TRUE;
                }
                else if (State->PressedPart == PhScrollNewPartPageDown)
                {
                    *NewPosition = State->Position + State->Page;
                    changed = TRUE;
                }

                InvalidateRect(WindowHandle, &State->Rect, FALSE);
            }
        }
        break;

    case WM_LBUTTONUP:
        {
            if (State->PressedPart != PhScrollNewPartNone)
            {
                ReleaseCapture();
                State->PressedPart = PhScrollNewPartNone;
                InvalidateRect(WindowHandle, &State->Rect, FALSE);
            }
        }
        break;

    case WM_MOUSELEAVE:
        {
            if (State->HotPart != PhScrollNewPartNone)
            {
                State->HotPart = PhScrollNewPartNone;
                InvalidateRect(WindowHandle, &State->Rect, FALSE);
            }
        }
        break;
    }

    if (changed)
    {
        if (*NewPosition < State->Minimum) *NewPosition = State->Minimum;
        if (*NewPosition > State->Maximum - State->Page + 1) *NewPosition = State->Maximum - State->Page + 1;
        if (*NewPosition < State->Minimum) *NewPosition = State->Minimum;

        // Update visual track position. During thumb drag this is the ahead-of-commit
        // position; committed Position is updated by the parent after SB_THUMBTRACK.
        State->TrackPosition = *NewPosition;

        PhScrollNewLayout(State, &State->Rect);
    }

    return changed;
}

LRESULT PhScrollNewSendMessage(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _Pre_maybenull_ _Post_valid_ WPARAM wParam,
    _Pre_maybenull_ _Post_valid_ LPARAM lParam
    )
{
    if (PhGetWindowContextEx(WindowHandle))
    {
        return PhScrollNewWndProc(WindowHandle, WindowMessage, wParam, lParam);
    }

    return SendMessage(WindowHandle, WindowMessage, wParam, lParam);
}
