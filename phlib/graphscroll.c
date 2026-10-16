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
#define PH_SCROLLNEW_RESIZE_TIMER_ID  2
#define PH_SCROLLNEW_RESIZE_DELAY     50
#define PH_SCROLLNEW_DARK_CLASSNAME   L"Explorer::Scrollbar"

// Touch/pen mouse-message synthesis signature (see GetMessageExtraInfo).
// Used to suppress synthesized mouse messages so the WM_POINTER* path can
// own touch/pen input without double-handling.
#define PH_SCROLLNEW_MI_WP_SIGNATURE  0xFF515700
#define PH_SCROLLNEW_MI_WP_MASK       0xFFFFFF00

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

#define PH_SCROLLNEW_MAX_POS(Context) ((Context)->Maximum - (((Context)->Page > 0) ? ((Context)->Page - 1) : 0))

// Windows 11 scrollbar palette. The track is intentionally transparent-looking
// and the thumb is drawn as a rounded pill, matching Explorer's compact style.
#define PH_SCROLLNEW_WIN11_TRACK_LIGHT          RGB(250, 250, 250)
#define PH_SCROLLNEW_WIN11_THUMB_LIGHT          RGB(190, 190, 190)
#define PH_SCROLLNEW_WIN11_THUMB_HOT_LIGHT      RGB(128, 128, 128)
#define PH_SCROLLNEW_WIN11_THUMB_PRESS_LIGHT    RGB(96, 96, 96)
#define PH_SCROLLNEW_WIN11_TRACK_DARK           RGB(32, 32, 32)
#define PH_SCROLLNEW_WIN11_THUMB_DARK           RGB(100, 100, 100)
#define PH_SCROLLNEW_WIN11_THUMB_HOT_DARK       RGB(150, 150, 150)
#define PH_SCROLLNEW_WIN11_THUMB_PRESS_DARK     RGB(180, 180, 180)

static VOID PhpScrollNewOnPaint(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE Context
    );

static HTHEME PhpScrollNewOpenThemeData(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE Context
    );
static LRESULT PhpScrollNewOnUserMessage(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE Context,
    _In_ UINT Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );
static LRESULT CALLBACK PhpScrollNewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );
static VOID PhpScrollNewPaintNow(
    _In_ PPH_SCROLLNEW_STATE Context,
    _In_ HDC hdc
    );
static VOID PhpScrollNewCancelRepeat(_In_ HWND WindowHandle);
static VOID PhpScrollNewRepeatPress(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE Context
    );
static VOID PhpScrollNewWndProcNotify(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE Context,
    _In_ UINT ScrollCode,
    _In_ LONG NewPosition
    );
static UINT PhpScrollNewPartToScrollCode(
    _In_ PPH_SCROLLNEW_STATE Context,
    _In_ PH_SCROLLNEW_PART Part
    );

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
    wcex.style = CS_GLOBALCLASS | CS_PARENTDC;// | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = PhpScrollNewWndProc;
    wcex.cbWndExtra = sizeof(PVOID);
    wcex.hInstance = NtCurrentImageBase();
    wcex.hCursor = PhLoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = PH_SCROLLNEW_CLASSNAME;

    return RegisterClassEx(&wcex);
}

static HTHEME PhpScrollNewOpenThemeData(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE Context
    )
{
    // Win10 theme uses the flat custom-painted path (no theme handle).
    if (Context->Theme == PhScrollNewThemeWin10 ||
        Context->Theme == PhScrollNewThemeWin11 ||
        Context->Theme == PhScrollNewThemeWin11Overlay)
        return NULL;

    // Win7 theme forces the classic uxtheme scrollbar regardless of theme mode.
    if (Context->Theme == PhScrollNewThemeWin7)
        return PhOpenThemeData(WindowHandle, VSCLASS_SCROLLBAR, 0);

    if (PhEnableThemeSupport)
    {
        PhAllowDarkModeForWindow(WindowHandle, TRUE);
        return PhOpenThemeData(WindowHandle, PH_SCROLLNEW_DARK_CLASSNAME, 0);
    }

    PhAllowDarkModeForWindow(WindowHandle, FALSE);
    return PhOpenThemeData(WindowHandle, VSCLASS_SCROLLBAR, 0);
}

BOOLEAN PhScrollNewIsMouseFromTouch(
    _In_opt_ PPH_SCROLLNEW_STATE Context
    )
{
    if (Context && Context->PointerReentrant)
        return FALSE;

    if (Context && Context->PointerActive)
        return TRUE;

    return (((ULONG_PTR)GetMessageExtraInfo()) & PH_SCROLLNEW_MI_WP_MASK) == PH_SCROLLNEW_MI_WP_SIGNATURE;
}

static LRESULT PhpScrollNewOnUserMessage(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE Context,
    _In_ UINT Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (Message)
    {
    case SBM_SETSCROLLINFO:
        {
            LPSCROLLINFO si = (LPSCROLLINFO)lParam;
            BOOLEAN redraw = !!wParam;
            LONG min = Context->Minimum;
            LONG max = Context->Maximum;
            LONG page = Context->Page;
            LONG pos = Context->Position;

            if (si->fMask & SIF_RANGE) { min = si->nMin; max = si->nMax; }
            if (si->fMask & SIF_PAGE)  page = (LONG)si->nPage;
            if (si->fMask & SIF_POS)   pos = si->nPos;

            PhScrollNewUpdate(Context, min, max, page, pos);

            if (redraw)
            {
                InvalidateRect(WindowHandle, NULL, FALSE);
            }

            return Context->Position;
        }
        break;
    case SBM_GETSCROLLINFO:
        {
            LPSCROLLINFO si = (LPSCROLLINFO)lParam;
            if (si->fMask & SIF_RANGE) { si->nMin = Context->Minimum; si->nMax = Context->Maximum; }
            if (si->fMask & SIF_PAGE)       si->nPage = (UINT)Context->Page;
            if (si->fMask & SIF_POS)        si->nPos = Context->Position;
            if (si->fMask & SIF_TRACKPOS)   si->nTrackPos = Context->TrackPosition;
            return TRUE;
        }
        break;
    case SBM_SETPOS:
        {
            LONG oldPos = Context->Position;

            PhScrollNewUpdate(Context, Context->Minimum, Context->Maximum, Context->Page, (LONG)wParam);

            if (lParam)
                InvalidateRect(WindowHandle, NULL, FALSE);

            return oldPos;
        }
        break;
    case SBM_GETPOS:
        return Context->Position;
    case SBM_SETRANGE:
        {
            PhScrollNewUpdate(Context, (LONG)wParam, (LONG)lParam, Context->Page, Context->Position);
        }
        break;
    case SBM_SETRANGEREDRAW:
        {
            PhScrollNewUpdate(Context, (LONG)wParam, (LONG)lParam, Context->Page, Context->Position);
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case SBM_GETRANGE:
        {
            if (wParam)
            {
                *(PLONG)wParam = Context->Minimum;
            }

            if (lParam)
            {
                *(PLONG)lParam = Context->Maximum;
            }
        }
        break;
    }

    return 0;
}

/**
 * Window procedure for the PhScrollNew window class.
 *
 * Responds to SBM_SETSCROLLINFO / SBM_GETSCROLLINFO so that callers can use
 * the standard SetScrollInfo / GetScrollInfo API (with nBar = SB_CTL) as a
 * drop-in replacement for WC_SCROLLBAR child windows. Notifies the parent via
 * WM_VSCROLL or WM_HSCROLL, with 32-bit thumb positions exposed through
 * SBM_GETSCROLLINFO/SIF_TRACKPOS.
 *
 * \param WindowHandle Handle to the window receiving the message.
 * \param WindowMessage The message identifier.
 * \param wParam Additional message-specific information (depends on the message).
 * \param lParam Additional message-specific information (depends on the message).
 * \return The result of the message processing (depends on the message).
 */
static LRESULT CALLBACK PhpScrollNewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SCROLLNEW_STATE context = (PPH_SCROLLNEW_STATE)PhGetWindowContextEx(WindowHandle);

    // PhLogWindowMessage(L"ScrollNew", WindowHandle, WindowMessage, wParam, lParam);

    switch (WindowMessage)
    {
    case SBM_SETPOS:
    case SBM_GETPOS:
    case SBM_SETRANGE:
    case SBM_SETRANGEREDRAW:
    case SBM_GETRANGE:
    case SBM_SETSCROLLINFO:
    case SBM_GETSCROLLINFO:
        return PhpScrollNewOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);
    }

    switch (WindowMessage)
    {
    case WM_NCCREATE:
        {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;

            context = PhAllocateZero(sizeof(PH_SCROLLNEW_STATE));
            PhScrollNewInitialize(context, cs);

            PhSetWindowContextEx(WindowHandle, context);
        }
        break;
    case WM_CREATE:
        {
            context->ThemeHandle = PhpScrollNewOpenThemeData(WindowHandle, context);
        }
        break;
    case WM_NCDESTROY:
        {
            KillTimer(WindowHandle, PH_SCROLLNEW_RESIZE_TIMER_ID);
            PhRemoveWindowContextEx(WindowHandle);

            if (context->ThemeHandle)
                PhCloseThemeData(context->ThemeHandle);

            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            RECT clientRect;

            PhGetClientRect(WindowHandle, &clientRect);

            PhScrollNewLayout(context, &clientRect);
            context->ResizePending = TRUE;
            PhSetTimer(WindowHandle, PH_SCROLLNEW_RESIZE_TIMER_ID, PH_SCROLLNEW_RESIZE_DELAY, NULL);

            // Force a full repaint after re-laying out the buttons/thumb/track. WM_ERASEBKGND
            // unconditionally claims the background is already erased, and the implicit repaint
            // from the parent's MoveWindow call only covers the size delta - not necessarily the
            // union of the old and new button rects - so without this, stale button glyphs from
            // the previous layout are left behind as the window is resized. (dmex)
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PhpScrollNewOnPaint(WindowHandle, context);
        }
        return 0;
    case WM_THEMECHANGED:
        {
            if (context->ThemeHandle)
            {
                PhCloseThemeData((HTHEME)context->ThemeHandle);
            }

            context->ThemeHandle = PhpScrollNewOpenThemeData(WindowHandle, context);

            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, WindowHandle, 0 };

            // Touch/pen-synthesized mouse messages are handled by WM_POINTER*.
            if (PhScrollNewIsMouseFromTouch(context))
                return 0;

            TrackMouseEvent(&tme);

            if (context)
            {
                BOOLEAN isDragging = (context->PressedPart == PhScrollNewPartThumb);
                LONG newPos;

                // Mark the mouse as inside the client area so the uxtheme draw
                // path can render the arrow buttons. Repaint on the rising edge.
                if (!context->MouseInClient)
                {
                    context->MouseInClient = TRUE;
                    PhScrollNewLayout(context, &context->Rect);
                    InvalidateRect(WindowHandle, NULL, FALSE);
                }

                if (PhScrollNewHandleMessage(context, WindowHandle, WM_MOUSEMOVE, wParam, lParam, &newPos))
                {
                    if (isDragging)
                    {
                        HDC hdc;

                        if (hdc = PhGetDC(WindowHandle))
                        {
                            PhpScrollNewPaintNow(context, hdc);
                            ReleaseDC(WindowHandle, hdc);
                        }
                    }

                    PhpScrollNewWndProcNotify(WindowHandle, context, isDragging ? SB_THUMBTRACK : SB_THUMBPOSITION, newPos);
                }
            }
        }
        return 0;
    case WM_LBUTTONDOWN:
        {
            if (PhScrollNewIsMouseFromTouch(context))
                return 0;

            if (context)
            {
                LONG newPos;

                if (PhScrollNewHandleMessage(context, WindowHandle, WM_LBUTTONDOWN, wParam, lParam, &newPos))
                {
                    PhpScrollNewWndProcNotify(
                        WindowHandle,
                        context,
                        PhpScrollNewPartToScrollCode(context, context->PressedPart),
                        newPos
                        );

                    if (context->PressedPart != PhScrollNewPartThumb)
                    {
                        PhSetTimer(WindowHandle, PH_SCROLLNEW_TIMER_ID, PH_SCROLLNEW_TIMER_DELAY, NULL);
                    }
                }
            }
        }
        return 0;
    case WM_LBUTTONUP:
        {
            if (PhScrollNewIsMouseFromTouch(context))
                return 0;

            if (context)
            {
                BOOLEAN interactionActive = context->PressedPart != PhScrollNewPartNone;
                BOOLEAN wasDragging = context->PressedPart == PhScrollNewPartThumb;
                LONG finalPos = context->TrackPosition;
                LONG newPos = 0;

                PhScrollNewHandleMessage(context, WindowHandle, WM_LBUTTONUP, wParam, lParam, &newPos);
                PhpScrollNewCancelRepeat(WindowHandle);

                if (wasDragging)
                {
                    PhpScrollNewWndProcNotify(WindowHandle, context, SB_THUMBPOSITION, finalPos);
                }

                if (interactionActive)
                {
                    PhpScrollNewWndProcNotify(WindowHandle, context, SB_ENDSCROLL, 0);
                }
            }
        }
        return 0;
    case WM_CAPTURECHANGED:
        {
            if (context)
            {
                BOOLEAN interactionActive = context->PressedPart != PhScrollNewPartNone;

                PhpScrollNewCancelRepeat(WindowHandle);
                context->PressedPart = PhScrollNewPartNone;

                if (interactionActive)
                {
                    PhpScrollNewWndProcNotify(WindowHandle, context, SB_ENDSCROLL, 0);
                }
            }
        }
        return 0;
    case WM_MOUSELEAVE:
        {
            LONG newPos = 0;

            if (context)
            {
                // Mouse has left the client area; the uxtheme draw path will
                // stop rendering the arrow buttons on the next paint.
                if (context->MouseInClient)
                {
                    context->MouseInClient = FALSE;
                    PhScrollNewLayout(context, &context->Rect);
                    InvalidateRect(WindowHandle, NULL, FALSE);
                }

                PhScrollNewHandleMessage(
                    context,
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
            if ((ULONG)wParam == PH_SCROLLNEW_RESIZE_TIMER_ID)
            {
                KillTimer(WindowHandle, PH_SCROLLNEW_RESIZE_TIMER_ID);
                context->ResizePending = FALSE;
                InvalidateRect(WindowHandle, NULL, FALSE);
                UpdateWindow(WindowHandle);
            }
            else if ((ULONG)wParam == PH_SCROLLNEW_TIMER_ID && context->PressedPart != PhScrollNewPartNone)
            {
                PhpScrollNewRepeatPress(WindowHandle, context);
            }
        }
        return 0;
    case WM_POINTERDOWN:
    case WM_POINTERUPDATE:
    case WM_POINTERUP:
        {
            // Route touch/pen pointer input through the existing mouse context machine.
            // Synthesized WM_LBUTTON*/WM_MOUSEMOVE messages for the same input are
            // suppressed via the PhScrollNewIsMouseFromTouch(context) gates above.
            UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
            POINTER_INFO info;
            LRESULT result;

            if (context && GetPointerInfo(pointerId, &info) && (info.pointerType == PT_TOUCH || info.pointerType == PT_PEN))
            {
                POINT pt = info.ptPixelLocation;
                UINT mappedMsg;

                ScreenToClient(WindowHandle, &pt);

                if (WindowMessage == WM_POINTERDOWN)
                    context->PointerActive = TRUE;

                if (WindowMessage == WM_POINTERDOWN)
                    mappedMsg = WM_LBUTTONDOWN;
                else if (WindowMessage == WM_POINTERUP)
                    mappedMsg = WM_LBUTTONUP;
                else
                    mappedMsg = WM_MOUSEMOVE;

                // Pointer contacts are implicitly captured by the window they
                // were pressed in until lifted, so no explicit capture call is
                // needed for thumb-drag past the scrollbar's client area.
                context->PointerReentrant = TRUE;
                result = PhpScrollNewWndProc(
                    WindowHandle,
                    mappedMsg,
                    MK_LBUTTON,
                    MAKELPARAM((WORD)pt.x, (WORD)pt.y)
                    );
                context->PointerReentrant = FALSE;

                if (WindowMessage == WM_POINTERUP)
                    context->PointerActive = FALSE;

                return result;
            }
        }
        break;
    case WM_POINTERCAPTURECHANGED:
        {
            if (context)
            {
                BOOLEAN interactionActive = context->PressedPart != PhScrollNewPartNone;

                PhpScrollNewCancelRepeat(WindowHandle);
                context->PressedPart = PhScrollNewPartNone;
                context->PointerActive = FALSE;
                InvalidateRect(WindowHandle, NULL, FALSE);

                if (interactionActive)
                {
                    PhpScrollNewWndProcNotify(WindowHandle, context, SB_ENDSCROLL, 0);
                }
            }
        }
        return 0;
    case WM_CONTEXTMENU:
        {
            if (context)
            {
                BOOLEAN isHorz = context->Horizontal;
                BOOLEAN hasThumb = context->ThumbRect.right > context->ThumbRect.left &&
                                   context->ThumbRect.bottom > context->ThumbRect.top;
                POINT screenPt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                POINT clientPt = screenPt;
                LONG scrollHerePos;
                PPH_EMENU menu;
                PPH_EMENU_ITEM selectedItem;

                if (!hasThumb)
                    return 0;

                ScreenToClient(WindowHandle, &clientPt);

                // Calculate "Scroll Here" position from click point within the gutter.
                {
                    LONG maxPos = PH_SCROLLNEW_MAX_POS(context);
                    LONG scrollRange = maxPos - context->Minimum;
                    LONG gutterLen = isHorz
                        ? (context->GutterRect.right  - context->GutterRect.left)
                        : (context->GutterRect.bottom - context->GutterRect.top);
                    LONG clickOff = isHorz
                        ? (clientPt.x - context->GutterRect.left)
                        : (clientPt.y - context->GutterRect.top);

                    if (gutterLen > 0 && scrollRange > 0)
                        scrollHerePos = context->Minimum + (LONG)((__int64)clickOff * scrollRange / gutterLen);
                    else
                        scrollHerePos = context->Position;

                    if (scrollHerePos < context->Minimum) scrollHerePos = context->Minimum;
                    if (scrollHerePos > maxPos) scrollHerePos = maxPos;
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
                        PhpScrollNewWndProcNotify(WindowHandle, context, SB_THUMBTRACK, scrollHerePos);
                        break;
                    case PH_SCROLLNEW_IDM_TOP:
                        PhpScrollNewWndProcNotify(WindowHandle, context, SB_TOP, 0);
                        break;
                    case PH_SCROLLNEW_IDM_BOTTOM:
                        PhpScrollNewWndProcNotify(WindowHandle, context, SB_BOTTOM, 0);
                        break;
                    case PH_SCROLLNEW_IDM_PAGE_UP:
                        PhpScrollNewWndProcNotify(WindowHandle, context, SB_PAGEUP, 0);
                        break;
                    case PH_SCROLLNEW_IDM_PAGE_DOWN:
                        PhpScrollNewWndProcNotify(WindowHandle, context, SB_PAGEDOWN, 0);
                        break;
                    case PH_SCROLLNEW_IDM_SCROLL_UP:
                        PhpScrollNewWndProcNotify(WindowHandle, context, SB_LINEUP, 0);
                        break;
                    case PH_SCROLLNEW_IDM_SCROLL_DOWN:
                        PhpScrollNewWndProcNotify(WindowHandle, context, SB_LINEDOWN, 0);
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

static VOID PhpScrollNewPaintNow(
    _In_ PPH_SCROLLNEW_STATE Context,
    _In_ HDC hdc
    )
{
    PH_BUFFERED_PAINT bufferedPaint;
    HDC bufferDc;

    if (PhRectEmpty(&Context->Rect))
        return;

    if (PhBeginBufferedPaint(hdc, &Context->Rect, PHBF_TOPDOWNDIB, NULL, &bufferedPaint, &bufferDc))
    {
        PhScrollNewDraw(Context, bufferDc);
        PhEndBufferedPaint(&bufferedPaint, TRUE);
    }
    else
    {
        PhScrollNewDraw(Context, hdc);
    }
}

static VOID PhpScrollNewCancelRepeat(
    _In_ HWND WindowHandle
    )
{
    KillTimer(WindowHandle, PH_SCROLLNEW_TIMER_ID);
}

static VOID PhpScrollNewRepeatPress(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE Context
    )
{
    LONG newPos;
    UINT scrollCode;

    // For page-up/page-down auto-repeat, stop when the thumb has scrolled
    // under (or past) the mouse cursor. This matches the standard Windows
    // scrollbar behavior — otherwise the thumb keeps marching past the mouse.
    if (Context->PressedPart == PhScrollNewPartPageUp ||
        Context->PressedPart == PhScrollNewPartPageDown)
    {
        POINT cursor;

        if (GetCursorPos(&cursor) && ScreenToClient(WindowHandle, &cursor))
        {
            PH_SCROLLNEW_PART hit = PhScrollNewHitTest(Context, cursor);

            if (hit != Context->PressedPart)
            {
                PhpScrollNewCancelRepeat(WindowHandle);
                return;
            }
        }
    }

    switch (Context->PressedPart)
    {
    case PhScrollNewPartUpButton:
        scrollCode = PhpScrollNewPartToScrollCode(Context, Context->PressedPart);
        newPos = Context->Position - 1;
        break;
    case PhScrollNewPartDownButton:
        scrollCode = PhpScrollNewPartToScrollCode(Context, Context->PressedPart);
        newPos = Context->Position + 1;
        break;
    case PhScrollNewPartPageUp:
        scrollCode = PhpScrollNewPartToScrollCode(Context, Context->PressedPart);
        newPos = Context->Position - Context->Page;
        break;
    case PhScrollNewPartPageDown:
        scrollCode = PhpScrollNewPartToScrollCode(Context, Context->PressedPart);
        newPos = Context->Position + Context->Page;
        break;
    default:
        return;
    }

    PhpScrollNewWndProcNotify(WindowHandle, Context, scrollCode, newPos);
}

// Internal helper: notify parent window of a position change.
static VOID PhpScrollNewWndProcNotify(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE Context,
    _In_ UINT ScrollCode,
    _In_ LONG NewPosition
    )
{
    HWND parent = GetParent(WindowHandle);

    if (!parent)
        return;

    if (ScrollCode == SB_THUMBTRACK || ScrollCode == SB_THUMBPOSITION)
        Context->TrackPosition = NewPosition;

    if (Context->Horizontal)
        SendMessage(parent, WM_HSCROLL, MAKEWPARAM(ScrollCode, (WORD)NewPosition), (LPARAM)WindowHandle);
    else
        SendMessage(parent, WM_VSCROLL, MAKEWPARAM(ScrollCode, (WORD)NewPosition), (LPARAM)WindowHandle);
}

static UINT PhpScrollNewPartToScrollCode(
    _In_ PPH_SCROLLNEW_STATE Context,
    _In_ PH_SCROLLNEW_PART Part
    )
{
    switch (Part)
    {
    case PhScrollNewPartUpButton:
        return Context->Horizontal ? SB_LINELEFT : SB_LINEUP;
    case PhScrollNewPartDownButton:
        return Context->Horizontal ? SB_LINERIGHT : SB_LINEDOWN;
    case PhScrollNewPartPageUp:
        return Context->Horizontal ? SB_PAGELEFT : SB_PAGEUP;
    case PhScrollNewPartPageDown:
        return Context->Horizontal ? SB_PAGERIGHT : SB_PAGEDOWN;
    default:
        return SB_THUMBPOSITION;
    }
}

VOID PhScrollNewInitialize(
    _Out_ PPH_SCROLLNEW_STATE Context,
    _In_ LPCREATESTRUCT CreateStruct
    )
{
    memset(Context, 0, sizeof(PH_SCROLLNEW_STATE));
    Context->Horizontal = !(CreateStruct->style & SBS_VERT);
    Context->Theme = PhScrollNewThemeWin10;
    Context->HotPart = PhScrollNewPartNone;
    Context->PressedPart = PhScrollNewPartNone;
}

VOID PhScrollNewUpdate(
    _Inout_ PPH_SCROLLNEW_STATE Context,
    _In_ LONG Minimum,
    _In_ LONG Maximum,
    _In_ LONG Page,
    _In_ LONG Position
    )
{
    LONG maxPos;

    Context->Minimum = Minimum;
    Context->Maximum = Maximum;
    Context->Page = Page;
    Context->Position = Position;

    maxPos = PH_SCROLLNEW_MAX_POS(Context);

    if (Context->Position < Context->Minimum)
        Context->Position = Context->Minimum;
    if (Context->Position > maxPos)
        Context->Position = maxPos;
    if (Context->Position < Context->Minimum)
        Context->Position = Context->Minimum;

    // Keep TrackPosition in sync unless a thumb drag is active.
    if (Context->PressedPart != PhScrollNewPartThumb)
        Context->TrackPosition = Context->Position;

    PhScrollNewLayout(Context, &Context->Rect);
}

VOID PhScrollNewLayout(
    _Inout_ PPH_SCROLLNEW_STATE Context,
    _In_ LPCRECT Rect
    )
{
    LONG width, height;
    LONG buttonSize;
    LONG range;
    LONG gutterLength;
    LONG thumbLength;

    RECT layoutRect = *Rect;
    Context->Rect = *Rect;
    width = Rect->right - Rect->left;
    height = Rect->bottom - Rect->top;

    if (Context->Theme == PhScrollNewThemeWin11 ||
        Context->Theme == PhScrollNewThemeWin11Overlay)
    {
        LONG thickness = Context->Theme == PhScrollNewThemeWin11Overlay &&
            !Context->MouseInClient && Context->PressedPart == PhScrollNewPartNone ? 4 : 12;

        if (Context->Horizontal)
        {
            LONG inset = __max(0, (height - thickness) / 2);
            layoutRect.top += inset;
            layoutRect.bottom = layoutRect.top + __min(thickness, height);
        }
        else
        {
            LONG inset = __max(0, (width - thickness) / 2);
            layoutRect.left += inset;
            layoutRect.right = layoutRect.left + __min(thickness, width);
        }
    }

    width = layoutRect.right - layoutRect.left;
    height = layoutRect.bottom - layoutRect.top;

    if (Context->Horizontal)
    {
        buttonSize = height;
        Context->UpButtonRect = (RECT){ layoutRect.left, layoutRect.top, layoutRect.left + buttonSize, layoutRect.bottom };
        Context->DownButtonRect = (RECT){ layoutRect.right - buttonSize, layoutRect.top, layoutRect.right, layoutRect.bottom };
        Context->GutterRect = (RECT){ Context->UpButtonRect.right, layoutRect.top, Context->DownButtonRect.left, layoutRect.bottom };
        gutterLength = Context->GutterRect.right - Context->GutterRect.left;
    }
    else
    {
        buttonSize = width;
        Context->UpButtonRect = (RECT){ layoutRect.left, layoutRect.top, layoutRect.right, layoutRect.top + buttonSize };
        Context->DownButtonRect = (RECT){ layoutRect.left, layoutRect.bottom - buttonSize, layoutRect.right, layoutRect.bottom };
        Context->GutterRect = (RECT){ layoutRect.left, Context->UpButtonRect.bottom, layoutRect.right, Context->DownButtonRect.top };
        gutterLength = Context->GutterRect.bottom - Context->GutterRect.top;
    }

    range = Context->Maximum - Context->Minimum + 1;

    if (range > 0 && Context->Page > 0 && range > Context->Page)
    {
        thumbLength = PhMultiplyDivideSigned(gutterLength, Context->Page, range);

        if (thumbLength < buttonSize)
            thumbLength = buttonSize;

        LONG scrollRange = range - Context->Page;
        LONG thumbScrollArea = gutterLength - thumbLength;
        LONG displayPos = (Context->PressedPart == PhScrollNewPartThumb) ? Context->TrackPosition : Context->Position;
        LONG normalizedPosition = displayPos - Context->Minimum;
        normalizedPosition = __max(0, __min(normalizedPosition, scrollRange));
        LONG thumbOffset = (scrollRange > 0) ? PhMultiplyDivideSigned(normalizedPosition, thumbScrollArea, scrollRange) : 0;

        if (Context->Horizontal)
        {
            Context->ThumbRect = (RECT)
            {
                Context->GutterRect.left + thumbOffset,
                layoutRect.top,
                Context->GutterRect.left + thumbOffset + thumbLength,
                layoutRect.bottom
            };
        }
        else
        {
            Context->ThumbRect = (RECT)
            {
                layoutRect.left,
                Context->GutterRect.top + thumbOffset,
                layoutRect.right,
                Context->GutterRect.top + thumbOffset + thumbLength
            };
        }
    }
    else
    {
        memset(&Context->ThumbRect, 0, sizeof(RECT));
    }
}

PH_SCROLLNEW_PART PhScrollNewHitTest(
    _In_ PPH_SCROLLNEW_STATE Context,
    _In_ POINT Point
    )
{
    BOOLEAN hasThumb = Context->ThumbRect.right > Context->ThumbRect.left &&
                       Context->ThumbRect.bottom > Context->ThumbRect.top;

    if (!hasThumb)
        return PhScrollNewPartNone;

    if (PhPtInRect(&Context->ThumbRect, &Point))
        return PhScrollNewPartThumb;

    if (PhPtInRect(&Context->UpButtonRect, &Point))
        return PhScrollNewPartUpButton;

    if (PhPtInRect(&Context->DownButtonRect, &Point))
        return PhScrollNewPartDownButton;

    if (PhPtInRect(&Context->GutterRect, &Point))
    {
        if (Context->Horizontal)
            return (Point.x < Context->ThumbRect.left) ? PhScrollNewPartPageUp : PhScrollNewPartPageDown;
        else
            return (Point.y < Context->ThumbRect.top) ? PhScrollNewPartPageUp : PhScrollNewPartPageDown;
    }

    return PhScrollNewPartNone;
}

#if defined(PH_SCROLLNEW_MARKS)
VOID PhScrollNewDrawMarks(
    _In_ PPH_SCROLLNEW_STATE Context,
    _In_ HDC Hdc
    )
{
    LONG range;
    LONG gutterLength;
    LONG markerArea;
    HBRUSH dcBrush;

    if (!Context->NumberOfMarks)
        return;

    if (PhRectEmpty(&Context->GutterRect))
        return;

    range = Context->Maximum - Context->Minimum + 1;

    if (range <= 0)
        return;

    gutterLength = Context->Horizontal ?
        Context->GutterRect.right - Context->GutterRect.left :
        Context->GutterRect.bottom - Context->GutterRect.top;

    if (gutterLength <= 0)
        return;

    markerArea = gutterLength - 1;
    dcBrush = PhGetStockBrush(DC_BRUSH);

    for (ULONG i = 0; i < Context->NumberOfMarks; i++)
    {
        LONG position = Context->Marks[i].Position;
        LONG offset;
        RECT markerRect;

        if (position < Context->Minimum)
            position = Context->Minimum;
        if (position > Context->Maximum)
            position = Context->Maximum;

        offset = range > 1 ? PhMultiplyDivideSigned(position - Context->Minimum, markerArea, range - 1) : 0;

        if (Context->Horizontal)
        {
            markerRect.left = Context->GutterRect.left + offset;
            markerRect.top = Context->GutterRect.top;
            markerRect.right = markerRect.left + 2;
            markerRect.bottom = Context->GutterRect.bottom;
        }
        else
        {
            markerRect.left = Context->GutterRect.left;
            markerRect.top = Context->GutterRect.top + offset;
            markerRect.right = Context->GutterRect.right;
            markerRect.bottom = markerRect.top + 2;
        }

        if (markerRect.right > Context->GutterRect.right)
            markerRect.right = Context->GutterRect.right;
        if (markerRect.bottom > Context->GutterRect.bottom)
            markerRect.bottom = Context->GutterRect.bottom;

        SetDCBrushColor(Hdc, Context->Marks[i].Color);
        FillRect(Hdc, &markerRect, dcBrush);
    }
}
#endif

/**
 * Draws a small filled triangle arrow centered in ButtonRect.
 * Requires DC_BRUSH and DC_PEN to already be selected into hdc by the caller.
 * Uses SetDCBrushColor/SetDCPenColor so no GDI object allocation occurs.
 */
VOID PhScrollNewDrawArrow(
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
    _In_ PPH_SCROLLNEW_STATE Context,
    _In_ HDC hdc
    )
{
    BOOLEAN vertical = !Context->Horizontal;
    BOOLEAN hasThumb = Context->ThumbRect.right > Context->ThumbRect.left &&
                       Context->ThumbRect.bottom > Context->ThumbRect.top;

    if (Context->Theme == PhScrollNewThemeWin11 ||
        Context->Theme == PhScrollNewThemeWin11Overlay)
    {
        HBRUSH brush = PhGetStockBrush(DC_BRUSH);
        COLORREF track = PhEnableThemeSupport ? PH_SCROLLNEW_WIN11_TRACK_DARK : PH_SCROLLNEW_WIN11_TRACK_LIGHT;
        COLORREF thumb = PhEnableThemeSupport ? PH_SCROLLNEW_WIN11_THUMB_DARK : PH_SCROLLNEW_WIN11_THUMB_LIGHT;

        if (Context->PressedPart == PhScrollNewPartThumb)
            thumb = PhEnableThemeSupport ? PH_SCROLLNEW_WIN11_THUMB_PRESS_DARK : PH_SCROLLNEW_WIN11_THUMB_PRESS_LIGHT;
        else if (Context->HotPart == PhScrollNewPartThumb)
            thumb = PhEnableThemeSupport ? PH_SCROLLNEW_WIN11_THUMB_HOT_DARK : PH_SCROLLNEW_WIN11_THUMB_HOT_LIGHT;

        SetDCBrushColor(hdc, track);
        FillRect(hdc, &Context->Rect, brush);

        if (hasThumb)
        {
            RECT thumbRect = Context->ThumbRect;
            LONG radius = __min(thumbRect.right - thumbRect.left, thumbRect.bottom - thumbRect.top) / 2;
            HGDIOBJ oldBrush = SelectBrush(hdc, brush);
            HGDIOBJ oldPen = SelectPen(hdc, PhGetStockPen(NULL_PEN));
            SetDCBrushColor(hdc, thumb);
            RoundRect(hdc, thumbRect.left, thumbRect.top, thumbRect.right, thumbRect.bottom, radius * 2, radius * 2);
            SelectPen(hdc, oldPen);
            SelectBrush(hdc, oldBrush);
        }

        return;
    }

    if (Context->ThemeHandle)
    {
        LONG upBtnState;
        LONG downBtnState;
        LONG trackLeadPart;
        LONG trackTrailPart;
        LONG thumbPart;
        BOOLEAN themeDrawSucceeded = TRUE;

        // LOWER = leading track (before the thumb); UPPER = trailing track (after the thumb).
        trackLeadPart  = vertical ? SBP_LOWERTRACKVERT : SBP_LOWERTRACKHORZ;
        trackTrailPart = vertical ? SBP_UPPERTRACKVERT : SBP_UPPERTRACKHORZ;
        thumbPart      = vertical ? SBP_THUMBBTNVERT   : SBP_THUMBBTNHORZ;

        // Arrows are only painted while the mouse is hovering anywhere inside
        // the scrollbar's client area (or a part is captured via a press).
        // While hovering, draw the arrows in the HOT-tier context (the modern
        // Explorer::Scrollbar theme renders the chevron near-invisible in the
        // NORMAL context, so we promote to HOT to keep the glyphs visible) and
        // only escalate to PRESSED when the specific button is captured.
        // While idle, fill the button rects with the gutter track so no arrow
        // glyph is drawn.
        if (Context->MouseInClient || Context->PressedPart != PhScrollNewPartNone)
        {
            if (vertical)
            {
                upBtnState   = (Context->PressedPart == PhScrollNewPartUpButton)   ? ABS_UPPRESSED   : ABS_UPHOT;
                downBtnState = (Context->PressedPart == PhScrollNewPartDownButton) ? ABS_DOWNPRESSED : ABS_DOWNHOT;
            }
            else
            {
                upBtnState   = (Context->PressedPart == PhScrollNewPartUpButton)   ? ABS_LEFTPRESSED  : ABS_LEFTHOT;
                downBtnState = (Context->PressedPart == PhScrollNewPartDownButton) ? ABS_RIGHTPRESSED : ABS_RIGHTHOT;
            }

            themeDrawSucceeded &= PhDrawThemeBackground(Context->ThemeHandle, hdc, SBP_ARROWBTN, upBtnState, &Context->UpButtonRect, NULL);
            themeDrawSucceeded &= PhDrawThemeBackground(Context->ThemeHandle, hdc, SBP_ARROWBTN, downBtnState, &Context->DownButtonRect, NULL);
        }
        else
        {
            themeDrawSucceeded &= PhDrawThemeBackground(Context->ThemeHandle, hdc, trackLeadPart, SCRBS_NORMAL, &Context->UpButtonRect, NULL);
            themeDrawSucceeded &= PhDrawThemeBackground(Context->ThemeHandle, hdc, trackTrailPart, SCRBS_NORMAL, &Context->DownButtonRect, NULL);
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
                PhSetRect(&leadRect, Context->GutterRect.left, Context->GutterRect.top, Context->GutterRect.right, Context->ThumbRect.top);
            else
                PhSetRect(&leadRect, Context->GutterRect.left, Context->GutterRect.top, Context->ThumbRect.left, Context->GutterRect.bottom);

            if (!PhRectEmpty(&leadRect))
            {
                trackState = (Context->HotPart == PhScrollNewPartPageUp) ? SCRBS_HOT : SCRBS_NORMAL;
                themeDrawSucceeded &= PhDrawThemeBackground(Context->ThemeHandle, hdc, trackLeadPart, trackState, &leadRect, NULL);
            }

            //
            // Trailing track (below / right of thumb)
            //

            if (vertical)
                PhSetRect(&trailRect, Context->GutterRect.left, Context->ThumbRect.bottom, Context->GutterRect.right, Context->GutterRect.bottom);
            else
                PhSetRect(&trailRect, Context->ThumbRect.right, Context->GutterRect.top, Context->GutterRect.right, Context->GutterRect.bottom);

            if (!PhRectEmpty(&trailRect))
            {
                trackState = (Context->HotPart == PhScrollNewPartPageDown) ? SCRBS_HOT : SCRBS_NORMAL;
                themeDrawSucceeded &= PhDrawThemeBackground(Context->ThemeHandle, hdc, trackTrailPart, trackState, &trailRect, NULL);
            }

            //
            // Thumb
            //

            thumbState = (Context->PressedPart == PhScrollNewPartThumb) ? SCRBS_PRESSED :
                         (Context->HotPart     == PhScrollNewPartThumb) ? SCRBS_HOT     : SCRBS_NORMAL;
            themeDrawSucceeded &= PhDrawThemeBackground(Context->ThemeHandle, hdc, thumbPart, thumbState, &Context->ThumbRect, NULL);
        }
        else
        {
            // No thumb — fill the full gutter as a disabled track.
            if (!PhRectEmpty(&Context->GutterRect))
            {
                themeDrawSucceeded &= PhDrawThemeBackground(Context->ThemeHandle, hdc, trackLeadPart, SCRBS_DISABLED, &Context->GutterRect, NULL);
            }
        }

        if (themeDrawSucceeded)
            return;
    }

    {
        // Windows 10 flat scrollbar — no 3D borders, muted grays, DC_BRUSH/DC_PEN for zero allocation.
        HBRUSH dcBrush = PhGetStockBrush(DC_BRUSH);
        BOOLEAN upHot   = (Context->HotPart    == PhScrollNewPartUpButton);
        BOOLEAN upPress = (Context->PressedPart == PhScrollNewPartUpButton);
        BOOLEAN dnHot   = (Context->HotPart    == PhScrollNewPartDownButton);
        BOOLEAN dnPress = (Context->PressedPart == PhScrollNewPartDownButton);
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
        FillRect(hdc, &Context->GutterRect, dcBrush);

        // Up / Left arrow button background
        SetDCBrushColor(hdc, upPress ? arrowButtonPressColor : upHot ? arrowButtonHotColor : trackColor);
        FillRect(hdc, &Context->UpButtonRect, dcBrush);

        // Down / Right arrow button background
        SetDCBrushColor(hdc, dnPress ? arrowButtonPressColor : dnHot ? arrowButtonHotColor : trackColor);
        FillRect(hdc, &Context->DownButtonRect, dcBrush);

        // Pre-select DC_BRUSH and DC_PEN once for both Polygon calls; avoids 6 redundant
        // SelectObject round-trips (4 select + 4 restore -> 2 select + 2 restore per frame).
        {
            HGDIOBJ prevBrush = SelectBrush(hdc, dcBrush);
            HGDIOBJ prevPen   = SelectPen(hdc, PhGetStockPen(DC_PEN));

            PhScrollNewDrawArrow(hdc, &Context->UpButtonRect, Context->Horizontal, FALSE, (upHot || upPress) ? arrowHotColor : arrowNormalColor);
            PhScrollNewDrawArrow(hdc, &Context->DownButtonRect, Context->Horizontal, TRUE, (dnHot || dnPress) ? arrowHotColor : arrowNormalColor);

            SelectBrush(hdc, prevBrush);
            SelectPen(hdc, prevPen);
        }

        // Thumb
        if (hasThumb)
        {
            COLORREF thumbColor =
                (Context->PressedPart == PhScrollNewPartThumb) ? thumbPressedColor :
                (Context->HotPart     == PhScrollNewPartThumb) ? thumbHotColor     :
                                                               thumbNormalColor;
            SetDCBrushColor(hdc, thumbColor);
            FillRect(hdc, &Context->ThumbRect, dcBrush);
        }
    }
}

/**
 * Handles the WM_PAINT message for the PhScrollNew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_SCROLLNEW_STATE structure.
 */
static VOID PhpScrollNewOnPaint(
    _In_ HWND WindowHandle,
    _In_ PPH_SCROLLNEW_STATE Context
    )
{
    PAINTSTRUCT paintStruct;
    HDC hdc = BeginPaint(WindowHandle, &paintStruct);

    if (hdc)
    {
        // Paint the current geometry even while the resize timer is pending.
        // BeginPaint validates the update region, so skipping drawing would
        // leave stale pixels; every successful BeginPaint also needs EndPaint.
        PhpScrollNewPaintNow(Context, hdc);
        EndPaint(WindowHandle, &paintStruct);
    }
}

BOOLEAN PhScrollNewHandleMessage(
    _Inout_ PPH_SCROLLNEW_STATE Context,
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
            PH_SCROLLNEW_PART hit = PhScrollNewHitTest(Context, pt);

            if (Context->HotPart != hit)
            {
                Context->HotPart = hit;
                InvalidateRect(WindowHandle, &Context->Rect, FALSE);
            }

            if (Context->PressedPart == PhScrollNewPartThumb)
            {
                LONG gutterLength, thumbLength, thumbScrollArea, range, scrollRange;
                LONG deltaPixels;

                range = Context->Maximum - Context->Minimum + 1;
                scrollRange = range - Context->Page;

                if (Context->Horizontal)
                {
                    gutterLength = Context->GutterRect.right - Context->GutterRect.left;
                    thumbLength = Context->ThumbRect.right - Context->ThumbRect.left;
                    deltaPixels = pt.x - Context->DragStartPoint.x;
                }
                else
                {
                    gutterLength = Context->GutterRect.bottom - Context->GutterRect.top;
                    thumbLength = Context->ThumbRect.bottom - Context->ThumbRect.top;
                    deltaPixels = pt.y - Context->DragStartPoint.y;
                }

                thumbScrollArea = gutterLength - thumbLength;

                if (thumbScrollArea > 0)
                {
                    LONG maxPos = PH_SCROLLNEW_MAX_POS(Context);
                    LONG deltaPos = PhMultiplyDivideSigned(deltaPixels, scrollRange, thumbScrollArea);
                    LONG pos = Context->DragStartPosition + deltaPos;

                    if (pos < Context->Minimum) pos = Context->Minimum;
                    if (pos > maxPos) pos = maxPos;

                    if (Context->TrackPosition != pos)
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
            Context->PressedPart = PhScrollNewHitTest(Context, pt);

            if (Context->PressedPart != PhScrollNewPartNone)
            {
                SetCapture(WindowHandle);

                Context->DragStartPoint = pt;
                Context->DragStartPosition = Context->Position;
                Context->TrackPosition = Context->Position;

                if (Context->PressedPart == PhScrollNewPartUpButton)
                {
                    *NewPosition = Context->Position - 1;
                    changed = TRUE;
                }
                else if (Context->PressedPart == PhScrollNewPartDownButton)
                {
                    *NewPosition = Context->Position + 1;
                    changed = TRUE;
                }
                else if (Context->PressedPart == PhScrollNewPartPageUp)
                {
                    *NewPosition = Context->Position - Context->Page;
                    changed = TRUE;
                }
                else if (Context->PressedPart == PhScrollNewPartPageDown)
                {
                    *NewPosition = Context->Position + Context->Page;
                    changed = TRUE;
                }

                InvalidateRect(WindowHandle, &Context->Rect, FALSE);
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if (Context->PressedPart != PhScrollNewPartNone)
            {
                Context->PressedPart = PhScrollNewPartNone;
                ReleaseCapture();
                InvalidateRect(WindowHandle, &Context->Rect, FALSE);
            }
        }
        break;
    case WM_MOUSELEAVE:
        {
            if (Context->HotPart != PhScrollNewPartNone)
            {
                Context->HotPart = PhScrollNewPartNone;
                InvalidateRect(WindowHandle, &Context->Rect, FALSE);
            }
        }
        break;
    }

    if (changed)
    {
        LONG maxPos = PH_SCROLLNEW_MAX_POS(Context);

        if (*NewPosition < Context->Minimum) *NewPosition = Context->Minimum;
        if (*NewPosition > maxPos) *NewPosition = maxPos;
        if (*NewPosition < Context->Minimum) *NewPosition = Context->Minimum;

        // Update visual track position. During thumb drag this is the ahead-of-commit
        // position; committed Position is updated by the parent after SB_THUMBTRACK.
        Context->TrackPosition = *NewPosition;

        PhScrollNewLayout(Context, &Context->Rect);
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
    switch (WindowMessage)
    {
    case SBM_SETPOS:
    case SBM_GETPOS:
    case SBM_SETRANGE:
    case SBM_SETRANGEREDRAW:
    case SBM_GETRANGE:
    case SBM_SETSCROLLINFO:
    case SBM_GETSCROLLINFO:
        {
            PPH_SCROLLNEW_STATE context;

            if ((context = (PPH_SCROLLNEW_STATE)PhGetWindowContextEx(WindowHandle)))
            {
#if defined(DEBUG)
                assert(GetWindowThreadProcessId(WindowHandle, NULL) == HandleToUlong(NtCurrentThreadId()));
#endif
                return PhpScrollNewOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);
            }
        }
        break;
    }

#if defined(DEBUG)
    assert(FALSE);
#endif
    return SendMessage(WindowHandle, WindowMessage, wParam, lParam);
}
