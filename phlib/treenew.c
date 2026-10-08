/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2017-2026
 *
 */

/*
 * The tree new is a tree view with columns. Unlike the old tree list control, which was a wrapper
 * around the list view control, this control was written from scratch.
 *
 * Current issues not included in any comments:
 * * It is not possible to change a column to make it fixed. The current fixed column must be
 *   removed and the new fixed column must then be added.
 * * When there are no visible normal columns, the space usually occupied by the normal column
 *   headers is filled with a solid background color. We should catch this and paint the usual
 *   themed background there instead.
 * * Runtime style updates support TN_STYLE_RUNTIME_MASK; other flags remain creation-only.
 *
 * Possible additions:
 * * More flexible mouse input callbacks to allow custom controls inside columns.
 * * Allow custom drawn columns to customize their behaviour when TN_FLAG_ITEM_DRAG_SELECT is set
 *   (e.g. disable drag selection over certain areas).
 * * Virtual mode
 */

#include <ph.h>
#include <commctrl.h>
#include <graphscroll.h>
#include <guisup.h>
#include <treenew.h>
#include <treenewp.h>
#include <uxtheme.h>
#include <vssym32.h>

#pragma comment(lib, "uxtheme.lib")

static BOOLEAN PhTnpGetCellPartsWithDc(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Index,
    _In_opt_ PPH_TREENEW_COLUMN Column,
    _In_ ULONG Flags,
    _Out_ PPH_TREENEW_CELL_PARTS Parts,
    _In_opt_ HDC MeasureDc
    );

/**
 * Saves deferred paint damage with a bounded region-operation budget.
 * Region failure or excessive fragmentation falls back to a full repaint.
 */
VOID PhpTnpAccumulateDamage(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ HRGN Region
    )
{
    if (Context->PendingFullInvalidate)
        return;

    if (!Region || ++Context->DeferredDamageCount > 64)
        goto FullDamage;

    if (!Context->SuspendUpdateRegion)
        Context->SuspendUpdateRegion = CreateRectRgn(0, 0, 0, 0);

    if (!Context->SuspendUpdateRegion || CombineRgn(Context->SuspendUpdateRegion, Context->SuspendUpdateRegion, Region, RGN_OR) == RGN_ERROR)
        goto FullDamage;

    return;

FullDamage:
    Context->PendingFullInvalidate = TRUE;
    if (Context->SuspendUpdateRegion)
    {
        DeleteRgn(Context->SuspendUpdateRegion);
        Context->SuspendUpdateRegion = NULL;
    }
}

VOID PhpTnpInvalidateRect(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ PRECT Rect
    )
{
    RECT clientRect;
    RECT invalidRect;

    clientRect = Context->ClientRect;

    if (PhRectEmpty(&clientRect))
    {
        if (!GetClientRect(Context->Handle, &clientRect))
            return;
    }

    if (Rect)
    {
        if (!PhIntersectRect(&invalidRect, Rect, &clientRect))
            return;
    }
    else
    {
        invalidRect = clientRect;
    }

    if (Context->EnableRedraw <= 0)
    {
        HRGN updateRegion;

        updateRegion = PhGetScratchRegion(&Context->UpdateScratchRegion);

        if (updateRegion && !SetRectRgn(updateRegion, invalidRect.left, invalidRect.top, invalidRect.right, invalidRect.bottom))
            updateRegion = NULL;

        PhpTnpAccumulateDamage(Context, updateRegion);
        return;
    }

    InvalidateRect(Context->Handle, &invalidRect, FALSE);
}

BOOLEAN PhpTnpGetContentRect(
    _In_ PPH_TREENEW_CONTEXT Context,
    _Out_ PRECT Rect
    )
{
    RECT contentRect;

    contentRect = Context->ClientRect;
    contentRect.top = Context->HeaderHeight;

    if (Context->VScrollVisible)
        contentRect.right -= Context->VScrollWidth;

    if (Context->HScrollVisible)
        contentRect.bottom -= Context->HScrollHeight;

    if (PhRectEmpty(&contentRect))
        return FALSE;

    *Rect = contentRect;
    return TRUE;
}

VOID PhpTnpInvalidateContent(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    RECT rect;

    if (PhpTnpGetContentRect(Context, &rect))
        PhpTnpInvalidateRect(Context, &rect);
}

BOOLEAN PhpTnpInvalidateRows(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Start,
    _In_ ULONG End
    )
{
    RECT contentRect;
    RECT rowRect;

    if (!PhTnpGetRowRects(Context, Start, End, TRUE, &rowRect))
        return FALSE;

    if (PhpTnpGetContentRect(Context, &contentRect) && PhIntersectRect(&rowRect, &rowRect, &contentRect))
    {
        PhpTnpInvalidateRect(Context, &rowRect);
    }

    return TRUE;
}

VOID PhpTnpInvalidateSelectedRows(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    ULONG i;
    ULONG start;
#ifndef PH_TREENEW_OPTIMIZE_SPARSE_SELECTION
    ULONG end;
#endif

    if (Context->SuspendUpdateStructure)
    {
        PhpTnpInvalidateContent(Context);
        return;
    }

#if defined(PH_TREENEW_OPTIMIZE_SPARSE_SELECTION)

    // Invalidate only contiguous ranges of selected rows.
    // This reduces unnecessary redraws when selections are sparse (e.g., rows 10, 50, 100).
    // Each contiguous range is invalidated separately as we encounter transitions from
    // selected to unselected rows. (dmex)

    start = ULONG_MAX;

    for (i = 0; i < Context->FlatList->Count; i++)
    {
        PPH_TREENEW_NODE node = Context->FlatList->Items[i];

        if (node->Selected)
        {
            if (start == ULONG_MAX)
                start = i;
        }
        else if (start != ULONG_MAX)
        {
            // End of a contiguous selection range - invalidate it now.

            PhpTnpInvalidateRows(Context, start, i - 1);
            start = ULONG_MAX;
        }
    }

    // Handle case where selection extends to the last row.

    if (start != ULONG_MAX)
    {
        PhpTnpInvalidateRows(Context, start, i - 1);
    }

#else

    // Find first and last selected rows, then invalidate the entire range between them.
    // This is simpler but may invalidate many unselected rows when selections are sparse
    // (e.g., selecting rows 10 and 1000 invalidates all 990 rows in between).

    start = ULONG_MAX;
    end = 0;

    for (i = 0; i < Context->FlatList->Count; i++)
    {
        PPH_TREENEW_NODE node = Context->FlatList->Items[i];

        if (node->Selected)
        {
            if (start == ULONG_MAX)
                start = i;

            end = i;
        }
    }

    if (start != ULONG_MAX)
    {
        PhpTnpInvalidateRows(Context, start, end);
    }
#endif
}

/**
 * Adds the area occupied by a child control to a redraw rectangle.
 *
 * The child rectangles used by the layout code are in MoveWindow form (left, top, width, height),
 * so they have to be converted before they can be unioned. (dmex)
 *
 * \param RedrawRect The rectangle to extend. May be empty.
 * \param ChildRect The child rectangle in MoveWindow form.
 */
VOID PhpTnpUnionChildRect(
    _Inout_ PRECT RedrawRect,
    _In_ const RECT* ChildRect
    )
{
    RECT rect;

    rect.left = ChildRect->left;
    rect.top = ChildRect->top;
    rect.right = ChildRect->left + ChildRect->right;
    rect.bottom = ChildRect->top + ChildRect->bottom;

    UnionRect(RedrawRect, RedrawRect, &rect);
}

/**
 * Blends a solid source color over a solid destination color using a constant alpha.
 *
 * The row backgrounds are solid on both sides of the blend, so the result is a solid color and can
 * be computed directly instead of going through GdiAlphaBlend for every row. (dmex)
 *
 * \param BackColor The destination color.
 * \param OverColor The source color drawn over the destination.
 * \param Alpha The constant source alpha (0-255).
 * \return The blended color.
 */
FORCEINLINE COLORREF PhpTnpBlendColor(
    _In_ COLORREF BackColor,
    _In_ COLORREF OverColor,
    _In_ ULONG Alpha
    )
{
    ULONG inverseAlpha = 255 - Alpha;

    return RGB(
        (GetRValue(OverColor) * Alpha + GetRValue(BackColor) * inverseAlpha) / 255,
        (GetGValue(OverColor) * Alpha + GetGValue(BackColor) * inverseAlpha) / 255,
        (GetBValue(OverColor) * Alpha + GetBValue(BackColor) * inverseAlpha) / 255
        );
}

/**
 * Invalidates the header item belonging to a single column.
 *
 * Hot tracking only changes the appearance of the column the mouse entered or left, so there is no
 * reason to repaint every column in the header. (dmex)
 *
 * \param Context Pointer to the treenew context structure.
 * \param HeaderHandle The header control containing the column.
 * \param ColumnId The column identifier, or ULONG_MAX for none.
 */
VOID PhpTnpInvalidateHeaderColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HWND HeaderHandle,
    _In_ ULONG ColumnId
    )
{
    LONG index;
    RECT rect;

    if (ColumnId == ULONG_MAX)
        return;

    if (HeaderHandle == Context->FixedHeaderHandle)
    {
        index = 0;
    }
    else
    {
        PPH_TREENEW_COLUMN column;

        if (!(column = PhTnpLookupColumnById(Context, ColumnId)))
            return;

        index = column->s.ViewIndex;
    }

    if (Header_GetItemRect(HeaderHandle, index, &rect))
        InvalidateRect(HeaderHandle, &rect, FALSE);
    else
        InvalidateRect(HeaderHandle, NULL, FALSE);
}

/**
 * Initializes the treenew window class.
 * \return RTL_ATOM Returns an atom representing the initialized window class.
 */
RTL_ATOM PhTreeNewInitialization(
    VOID
    )
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_DBLCLKS | CS_GLOBALCLASS;
    wcex.lpfnWndProc = PhTnpWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(PVOID);
    wcex.hInstance = NtCurrentImageBase();
    wcex.hCursor = PhLoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = PH_TREENEW_CLASSNAME;
    //wcex.hbrBackground = PhThemeWindowBackgroundBrush;

    return RegisterClassEx(&wcex);
}

/**
 * Window procedure for the treenew control.
 *
 * \param WindowHandle Handle to the window receiving the message.
 * \param WindowMessage The message identifier.
 * \param wParam Additional message-specific information (depends on the message).
 * \param lParam Additional message-specific information (depends on the message).
 * \return The result of the message processing (depends on the message).
 */
LRESULT CALLBACK PhTnpWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_TREENEW_CONTEXT context;

    if (WindowMessage == WM_NCCREATE)
    {
        context = PhTnpCreateTreeNewContext();
        PhSetWindowContextEx(WindowHandle, context);
    }
    else
    {
        context = PhGetWindowContextEx(WindowHandle);
    }

    if (!context)
        return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);

    if (context->Tracking && (GetAsyncKeyState(VK_ESCAPE) & 0x1))
    {
        PhTnpCancelTrack(context);
    }

    // Note: if we have suspended restructuring, we *cannot* access any nodes, because all node
    // pointers are now invalid. Below, we disable all input.

    switch (WindowMessage)
    {
    case WM_CREATE:
        {
            if (!PhTnpOnCreate(WindowHandle, context, (CREATESTRUCT *)lParam))
                return -1;
        }
        return 0;
    case WM_DESTROY:
        {
            context->Callback(WindowHandle, TreeNewDestroying, NULL, NULL, context->CallbackContext);
        }
        return 0;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContextEx(WindowHandle);

            PhTnpDestroyTreeNewContext(context);
        }
        return 0;
    case WM_SIZE:
        {
            PhTnpOnSize(WindowHandle, context, (ULONG)wParam);
        }
        break;
    case WM_SHOWWINDOW:
        {
            // Release the back buffer while the control is hidden. Hidden tabs and property pages
            // would otherwise each keep a full client-sized bitmap alive. (dmex)
            if (!wParam && context->BufferedContext)
            {
                PhTnpDestroyBufferedContext(context);
            }
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PhTnpOnPaint(WindowHandle, context);
        }
        return 0;
    case WM_PRINTCLIENT:
        {
            if (!context->SuspendUpdateStructure)
                PhTnpOnPrintClient(WindowHandle, context, (HDC)wParam, (ULONG)lParam);
        }
        return 0;
    case WM_NCPAINT:
        {
            if (PhTnpOnNcPaint(WindowHandle, context, (HRGN)wParam))
                return 0;
        }
        break;
    case WM_GETFONT:
        return (LRESULT)context->Font;
    case WM_SETFONT:
        {
            PhTnpOnSetFont(WindowHandle, context, (HFONT)wParam, LOWORD(lParam));
        }
        break;
    case WM_STYLECHANGED:
        {
            PhTnpOnStyleChanged(WindowHandle, context, (LONG)wParam, (STYLESTRUCT *)lParam);
        }
        break;
    case WM_SETTINGCHANGE:
        {
            PhTnpOnSettingChange(WindowHandle, context);
        }
        break;
    case WM_THEMECHANGED:
        {
            PhTnpOnThemeChanged(WindowHandle, context);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            PhTnpOnDpiChanged(WindowHandle, context);
        }
        break;
    case WM_GETDLGCODE:
        {
            return PhTnpOnGetDlgCode(WindowHandle, context, (ULONG)wParam, (PMSG)lParam);
        }
        break;
    case WM_ACTIVATE:
        {
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                // Hide tooltip when window is deactivated
                PhTnpPopTooltip(context);
            }
        }
        break;
    case WM_SETFOCUS:
        {
            context->HasFocus = TRUE;

            // Only selected rows change appearance with the focus state. (dmex)
            PhpTnpInvalidateSelectedRows(context);
        }
        return 0;
    case WM_KILLFOCUS:
        {
            //if (!context->ContextMenuActive && !(context->Style & TN_STYLE_ALWAYS_SHOW_SELECTION))
            //    PhTnpSelectRange(context, -1, -1, TN_SELECT_RESET, NULL, NULL);

            context->HasFocus = FALSE;

            // Immediately hide tooltip on focus loss.
            PhTnpPopTooltip(context);

            // Only selected rows change appearance with the focus state. (dmex)
            PhpTnpInvalidateSelectedRows(context);
        }
        return 0;
    case WM_SETCURSOR:
        {
            if (PhTnpOnSetCursor(WindowHandle, context, (HWND)wParam, LOWORD(lParam), HIWORD(lParam)))
                return TRUE;
        }
        break;
    case WM_TIMER:
        {
            PhTnpOnTimer(WindowHandle, context, (ULONG)wParam);
        }
        return 0;
    case WM_MOUSEMOVE:
        {
            context->MouseLocation.x = GET_X_LPARAM(lParam);
            context->MouseLocation.y = GET_Y_LPARAM(lParam);

            if (context->SuspendUpdateStructure)
                context->SuspendUpdateMoveMouse = TRUE;
            else
            {
                PhTnpOnMouseMove(WindowHandle, context, (ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
        }
        break;
    case WM_MOUSELEAVE:
        {
            //if (!context->ContextMenuActive && !(context->Style & TN_STYLE_ALWAYS_SHOW_SELECTION))
            //{
            //    ULONG changedStart;
            //    ULONG changedEnd;
            //    RECT rect;
            //
            //    PhTnpSelectRange(context, -1, -1, TN_SELECT_RESET, &changedStart, &changedEnd);
            //
            //    if (PhTnpGetRowRects(context, changedStart, changedEnd, TRUE, &rect))
            //    {
            //        InvalidateRect(context->Handle, &rect, FALSE);
            //    }
            //}

            if (!context->SuspendUpdateStructure)
                PhTnpOnMouseLeave(WindowHandle, context);
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
        {
            if (!context->SuspendUpdateStructure)
                PhTnpOnXxxButtonXxx(WindowHandle, context, WindowMessage, (ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_CAPTURECHANGED:
        {
            PhTnpOnCaptureChanged(WindowHandle, context);
        }
        break;
    case WM_KEYDOWN:
        {
            if (!context->SuspendUpdateStructure)
                PhTnpOnKeyDown(WindowHandle, context, (ULONG)wParam, (ULONG)lParam);
        }
        break;
    case WM_CHAR:
        {
            if (!context->SuspendUpdateStructure)
                PhTnpOnChar(WindowHandle, context, (ULONG)wParam, (ULONG)lParam);
        }
        return 0;
    case WM_MOUSEWHEEL:
        {
            PhTnpOnMouseWheel(WindowHandle, context, (SHORT)HIWORD(wParam), LOWORD(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_MOUSEHWHEEL:
        {
            PhTnpOnMouseHWheel(WindowHandle, context, (SHORT)HIWORD(wParam), LOWORD(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_CONTEXTMENU:
        {
            if (!context->SuspendUpdateStructure)
                PhTnpOnContextMenu(WindowHandle, context, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        return 0;
    case WM_VSCROLL:
        {
            PhTnpOnVScroll(WindowHandle, context, LOWORD(wParam), HIWORD(wParam));
        }
        return 0;
    case WM_HSCROLL:
        {
            PhTnpOnHScroll(WindowHandle, context, LOWORD(wParam), HIWORD(wParam));
        }
        return 0;
    case WM_NOTIFY:
        {
            LRESULT result;

            if (PhTnpOnNotify(WindowHandle, context, (NMHDR *)lParam, &result))
                return result;
        }
        break;
    case WM_MEASUREITEM:
        if (context->ThemeSupport && PhThemeWindowMeasureItem(WindowHandle, (LPMEASUREITEMSTRUCT)lParam))
            return TRUE;
        break;
    case WM_DRAWITEM:
        if (PhThemeWindowDrawItem(WindowHandle, (LPDRAWITEMSTRUCT)lParam))
            return TRUE;
        break;
    case WM_CTLCOLORSCROLLBAR:
        if (context->ThemeSupport)
            return HANDLE_WM_CTLCOLORSCROLLBAR(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
        break;
    case WM_CTLCOLORSTATIC:
        if (context->ThemeSupport)
            return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
        break;
    }

    if (WindowMessage >= TNM_FIRST && WindowMessage <= TNM_LAST)
    {
        return PhTnpOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);
    }

    switch (WindowMessage)
    {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        {
            if (context->TooltipsHandle)
            {
                MSG message;

                message.hwnd = WindowHandle;
                message.message = WindowMessage;
                message.wParam = wParam;
                message.lParam = lParam;
                SendMessage(context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }
        }
        break;
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Default null callback function for the treenew control.
 *
 * \param WindowHandle Handle to the treenew window.
 * \param Message The treenew message identifier.
 * \param Parameter1 First message-specific parameter.
 * \param Parameter2 Second message-specific parameter.
 * \param Context User-defined context pointer.
 * \return Always returns FALSE.
 */
_Function_class_(PH_TREENEW_CALLBACK)
BOOLEAN NTAPI PhTnpNullCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    return FALSE;
}

/**
 * Creates a new PPH_TREENEW_CONTEXT structure for use with the treenew control.
 * \return A pointer to the newly created PPH_TREENEW_CONTEXT structure.
 */
PPH_TREENEW_CONTEXT PhTnpCreateTreeNewContext(
    VOID
    )
{
    PPH_TREENEW_CONTEXT context;

    context = PhAllocateZero(sizeof(PH_TREENEW_CONTEXT));
    context->FixedWidthMinimum = 20;
    context->RowHeight = 1; // must never be 0
    context->HotNodeIndex = ULONG_MAX;
    context->Callback = PhTnpNullCallback;
    context->FlatList = PhCreateList(32);
    context->TooltipIndex = ULONG_MAX;
    context->TooltipId = ULONG_MAX;
    context->TooltipColumnId = ULONG_MAX;
    context->EnableRedraw = 1;
    // The scroll bars start with the class defaults, so the first update must always be applied.
    context->VScrollLastMax = -1;
    context->HScrollLastMax = -1;
    context->DefaultBackColor = GetSysColor(COLOR_WINDOW); // RGB(0xff, 0xff, 0xff)
    context->DefaultForeColor = GetSysColor(COLOR_WINDOWTEXT); // RGB(0x00, 0x00, 0x00)

   return context;
}

/**
 * Destroys a treenew context.
 * \param Context A pointer to the PPH_TREENEW_CONTEXT structure to be destroyed.
 */
VOID PhTnpDestroyTreeNewContext(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    if (Context->Columns)
    {
        for (ULONG i = 0; i < Context->NextId; i++)
        {
            if (Context->Columns[i])
                PhFree(Context->Columns[i]);
        }

        PhFree(Context->Columns);
    }

    if (Context->ColumnsByDisplay)
        PhFree(Context->ColumnsByDisplay);

    PhDereferenceObject(Context->FlatList);

    if (Context->FontOwned)
        DeleteFont(Context->Font);

    if (Context->ThemeData)
        PhCloseThemeData(Context->ThemeData);

    if (Context->SearchString)
        PhFree(Context->SearchString);

    if (Context->TooltipText)
        PhDereferenceObject(Context->TooltipText);

    if (Context->TooltipsHandle)
        DestroyWindow(Context->TooltipsHandle);

    if (Context->BufferedContext)
        PhTnpDestroyBufferedContext(Context);

    if (Context->SuspendUpdateRegion)
        DeleteRgn(Context->SuspendUpdateRegion);

    PhDeleteScratchRegion(&Context->UpdateScratchRegion);
    PhDeleteScratchRegion(&Context->ClipScratchRegion);
    PhDeleteScratchRegion(&Context->PaintScratchRegion);

    if (Context->HeaderThemeHandle)
        PhCloseThemeData(Context->HeaderThemeHandle);

    if (Context->HeaderBoldFontHandle)
        DeleteFont(Context->HeaderBoldFontHandle);

    PhTnpSelectionDestroyBufferedContext(Context);

    PhFree(Context);
}

/**
 * Handles the creation message of a treenew control.
 *
 * \param WindowHandle The handle to the window being created for the tree new control.
 * \param Context A pointer to the PPH_TREENEW_CONTEXT structure that holds the context for the tree new control.
 * \param CreateStruct A pointer to a constant CREATESTRUCT structure containing the creation parameters.
 * \return TRUE if the creation and initialization were successful; otherwise, FALSE.
 */
BOOLEAN PhTnpOnCreate(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ CONST CREATESTRUCT *CreateStruct
    )
{
    ULONG headerStyle;
    PPH_TREENEW_CREATEPARAMS createParamaters;

    Context->Handle = WindowHandle;
    Context->InstanceHandle = CreateStruct->hInstance;
    Context->Style = CreateStruct->style;
    Context->ExtendedStyle = CreateStruct->dwExStyle;

    // Prevent the TreeNew parent from erasing/painting over its child headers
    // and scrollbars during invalidation and resize.
    SetWindowLongPtr(
        WindowHandle,
        GWL_STYLE,
        GetWindowLongPtr(WindowHandle, GWL_STYLE) | WS_CLIPCHILDREN
        );

    createParamaters = CreateStruct->lpCreateParams;

    if (Context->Style & TN_STYLE_DOUBLE_BUFFERED)
        Context->DoubleBuffered = TRUE;
    if ((Context->Style & TN_STYLE_ANIMATE_DIVIDER) && Context->DoubleBuffered)
        Context->AnimateDivider = TRUE;

    headerStyle = HDS_HORZ | HDS_FULLDRAG;

    if (!(Context->Style & TN_STYLE_NO_COLUMN_SORT))
        headerStyle |= HDS_BUTTONS;
    if (!(Context->Style & TN_STYLE_NO_COLUMN_HEADER))
        headerStyle |= WS_VISIBLE;

    if (createParamaters)
    {
        if (Context->Style & TN_STYLE_CUSTOM_COLORS && RTL_CONTAINS_FIELD(createParamaters, createParamaters->Size, SelectionColor))
        {
            Context->CustomTextColor = createParamaters->TextColor ? createParamaters->TextColor : RGB(0xff, 0xff, 0xff);
            Context->CustomFocusColor = createParamaters->FocusColor ? createParamaters->FocusColor : RGB(0x0, 0x0, 0xff);
            Context->CustomSelectedColor = createParamaters->SelectionColor ? createParamaters->SelectionColor : RGB(0x0, 0x0, 0x80);
            Context->CustomColors = TRUE;
        }
        else
        {
            Context->CustomTextColor = GetSysColor(COLOR_WINDOWTEXT);
            Context->CustomFocusColor = GetSysColor(COLOR_HOTLIGHT);
            Context->CustomSelectedColor = GetSysColor(COLOR_HIGHLIGHT);
        }

        if (RTL_CONTAINS_FIELD(createParamaters, createParamaters->Size, RowHeight) && createParamaters->RowHeight)
        {
            Context->CustomRowHeight = TRUE;
            Context->RowHeight = max(createParamaters->RowHeight, 1);
        }
    }
    else
    {
        Context->CustomTextColor = GetSysColor(COLOR_WINDOWTEXT);
        Context->CustomFocusColor = GetSysColor(COLOR_HOTLIGHT);
        Context->CustomSelectedColor = GetSysColor(COLOR_HIGHLIGHT);
    }

    if (Context->Style & TN_STYLE_CUSTOM_HEADERDRAW)
    {
        Context->HeaderCustomDraw = TRUE;
        Context->HeaderInvalidatePending = TRUE;
    }

    if (!(Context->FixedHeaderHandle = PhCreateWindow(
        WC_HEADER,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | headerStyle,
        0,
        0,
        0,
        0,
        WindowHandle,
        NULL,
        CreateStruct->hInstance,
        NULL
        )))
    {
        return FALSE;
    }

    if (!(Context->Style & TN_STYLE_NO_COLUMN_REORDER))
        headerStyle |= HDS_DRAGDROP;

    if (!(Context->HeaderHandle = PhCreateWindow(
        WC_HEADER,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | headerStyle,
        0,
        0,
        0,
        0,
        WindowHandle,
        NULL,
        CreateStruct->hInstance,
        NULL
        )))
    {
        return FALSE;
    }

    // The normal header slides underneath the fixed header when the view is scrolled horizontally,
    // so it has to sit below the fixed header in the z-order for WS_CLIPSIBLINGS to clip the
    // overlapping part instead of painting over it. (dmex)
    SetWindowPos(
        Context->HeaderHandle,
        Context->FixedHeaderHandle,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOREDRAW
        );

    if (!(Context->VScrollHandle = PhCreateWindow(
        PH_TREENEW_SCROLLBAR_CLASSNAME,
        NULL,
        WS_CHILD | SBS_VERT,
        0,
        0,
        0,
        0,
        WindowHandle,
        NULL,
        CreateStruct->hInstance,
        NULL
        )))
    {
        return FALSE;
    }

    if (!(Context->HScrollHandle = PhCreateWindow(
        PH_TREENEW_SCROLLBAR_CLASSNAME,
        NULL,
        WS_CHILD | SBS_HORZ,
        0,
        0,
        0,
        0,
        WindowHandle,
        NULL,
        CreateStruct->hInstance,
        NULL
        )))
    {
        return FALSE;
    }

    if (!(Context->FillerBoxHandle = PhCreateWindow(
        WC_STATIC,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS,
        0,
        0,
        0,
        0,
        WindowHandle,
        NULL,
        CreateStruct->hInstance,
        NULL
        )))
    {
        return FALSE;
    }

#if defined(DEBUG)
    CLIENT_ID clientId;
    assert(NT_SUCCESS(PhGetWindowClientId(WindowHandle, &clientId)));
    Context->UniqueThread = clientId.UniqueThread;
#endif

    PhTnpUpdateSystemMetrics(Context);
    PhTnpSetFont(Context, NULL, FALSE); // use default font
    PhTnpInitializeHeaders(Context);
    PhTnpInitializeTooltips(Context);

    return TRUE;
}

/**
 * Handles the WM_SIZE message for the treenew control.
 *
 * \param WindowHandle Handle to the window being resized.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param Request The resize request type (SIZE_MINIMIZED, SIZE_RESTORED, etc).
 */
VOID PhTnpOnSize(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Request
    )
{
    if (!PhGetClientRect(WindowHandle, &Context->ClientRect))
        return;

    // There is nothing to lay out against an empty client area, and the scroll bar page size would
    // be computed from a negative height. A real size always follows. (dmex)
    if (Request == SIZE_MINIMIZED || PhRectEmpty(&Context->ClientRect))
    {
        if (Context->BufferedContext)
            PhTnpDestroyBufferedContext(Context);

        return;
    }

    if (Context->BufferedContext && (
        Context->BufferedContextRect.right < Context->ClientRect.right ||
        Context->BufferedContextRect.bottom < Context->ClientRect.bottom))
    {
        // Invalidate the buffered context because the client size has increased.
        PhTnpDestroyBufferedContext(Context);
    }

    PhTnpLayout(Context);

    if (Context->TooltipsHandle)
    {
        TOOLINFO toolInfo;

        memset(&toolInfo, 0, sizeof(TOOLINFO));
        toolInfo.cbSize = sizeof(TOOLINFO);
        toolInfo.hwnd = WindowHandle;
        toolInfo.uId = TNP_TOOLTIPS_ITEM;
        toolInfo.rect = Context->ClientRect;
        SendMessage(Context->TooltipsHandle, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);
    }
}

/**
 * Handles the WM_SETFONT message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param Font Handle to the font to set, or NULL for default.
 * \param Redraw Whether to redraw the control after setting the font.
 */
VOID PhTnpOnSetFont(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ HFONT Font,
    _In_ LOGICAL Redraw
    )
{
    PhTnpSetFont(Context, Font, !!Redraw);
    PhTnpInvalidateLayoutCache(Context);
    PhTnpLayout(Context);

    // The row height follows the font, so every row moved. (dmex)
    PhpTnpInvalidateContent(Context);
}

/**
 * Handles the WM_STYLECHANGED message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param Type Specifies whether the style or extended style changed.
 * \param StyleStruct Pointer to a STYLESTRUCT structure with style information.
 */
VOID PhTnpOnStyleChanged(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG Type,
    _In_ STYLESTRUCT *StyleStruct
    )
{
    if (Type == GWL_EXSTYLE)
        Context->ExtendedStyle = StyleStruct->styleNew;

    // Note: GWL_STYLE is deliberately not tracked. The TN_STYLE_* flags live in the low word of the
    // window style and are latched at creation; callers that rewrite the style don't necessarily
    // carry them, so copying styleNew here would silently drop control behavior. (dmex)
}

/**
 * Handles the WM_SETTINGCHANGE message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 */
VOID PhTnpOnSettingChange(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    PhTnpUpdateSystemMetrics(Context);
    PhTnpUpdateTextMetrics(Context);
    PhTnpInvalidateLayoutCache(Context);
    PhTnpLayout(Context);
}

/**
 * Handles the WM_THEMECHANGED message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 */
VOID PhTnpOnThemeChanged(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    PhTnpUpdateThemeData(Context);
    PhTnpInvalidateLayoutCache(Context);
}

/**
 * Handles the WM_DPICHANGED_AFTERPARENT message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 */
VOID PhTnpOnDpiChanged(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    LONG oldWindowDpi = Context->WindowDpi;

    PhTnpSetRedraw(Context, FALSE);

    PhTnpUpdateSystemMetrics(Context);
    PhTnpUpdateTextMetrics(Context);
    PhTnpUpdateThemeData(Context);
    PhTnpUpdateColumnHeadersDpiChanged(Context, oldWindowDpi, Context->WindowDpi);

    {
        PH_TREENEW_DPICHANGED_EVENT dpiChangedEvent;

        memset(&dpiChangedEvent, 0, sizeof(PH_TREENEW_DPICHANGED_EVENT));
        dpiChangedEvent.OldWindowDpi = oldWindowDpi;
        dpiChangedEvent.NewWindowDpi = Context->WindowDpi;

        Context->Callback(Context->Handle, TreeNewDpiChanged, &dpiChangedEvent, NULL, Context->CallbackContext);
    }

    PhTnpSetRedraw(Context, TRUE);

    PhTnpInvalidateLayoutCache(Context);
    PhTnpLayout(Context);
}

/**
 * Handles the WM_GETDLGCODE message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param VirtualKey The virtual key code.
 * \param Message Optional pointer to a MSG structure.
 * \return The dialog code flags.
 */
ULONG PhTnpOnGetDlgCode(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKey,
    _In_opt_ PMSG Message
    )
{
    ULONG code = 0;

    if (Context->Callback(WindowHandle, TreeNewGetDialogCode, UlongToPtr(VirtualKey), &code, Context->CallbackContext))
    {
        return code;
    }

    return DLGC_WANTARROWS | DLGC_WANTCHARS;
}

/**
 * Handles the WM_PAINT message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 */
VOID PhTnpOnPaint(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    RECT updateRect;
    HDC hdc;
    HRGN paintRegion;
    PAINTSTRUCT paintStruct;
    BOOLEAN deferPaint;

    // Painting is unsafe while the node structure is suspended: every node pointer in the flat list
    // is invalid until the next restructure. (dmex)
    deferPaint = Context->EnableRedraw <= 0 || Context->SuspendUpdateStructure;

    if (!deferPaint && Context->PendingFullInvalidate)
    {
        Context->PendingFullInvalidate = FALSE;
        InvalidateRect(WindowHandle, NULL, FALSE);
    }

    // The update region has to be captured before BeginPaint validates the window. It is used
    // either to accumulate the deferred damage, or to clip the buffered context so the paint code
    // can skip rows lying between disjoint dirty bands. (dmex)

    paintRegion = PhGetScratchRegion(&Context->PaintScratchRegion);

    if (paintRegion && GetUpdateRgn(WindowHandle, paintRegion, FALSE) == RGN_ERROR)
        paintRegion = NULL;

    if (deferPaint)
        PhpTnpAccumulateDamage(Context, paintRegion);

    // BeginPaint/EndPaint must run even when there is nothing to draw, otherwise the update region
    // is never validated and the window is sent WM_PAINT again. (dmex)

    if (!(hdc = BeginPaint(WindowHandle, &paintStruct)))
        return;

    updateRect = paintStruct.rcPaint;

    if (!deferPaint && !PhRectEmpty(&updateRect))
    {
        // The retained bitmap covers the client area. Clip rendering and the
        // final blit to the dirty region; hidden controls release this bitmap.

        if (Context->DoubleBuffered)
        {
            if (!Context->BufferedContext)
            {
                PhTnpCreateBufferedContext(Context, hdc);
            }
        }

        if (Context->BufferedContext)
        {
            // The memory DC has no clip of its own, so give it the window's update region. Pixels
            // outside the region stay stale in the buffer but are discarded by the window DC's own
            // clip when the block below blits them back. (dmex)
            if (paintRegion)
                SelectClipRgn(Context->BufferedContext, paintRegion);

            PhTnpPaint(WindowHandle, Context, Context->BufferedContext, &updateRect);

            if (paintRegion)
                SelectClipRgn(Context->BufferedContext, NULL);

            BitBlt(
                hdc,
                updateRect.left,
                updateRect.top,
                updateRect.right - updateRect.left,
                updateRect.bottom - updateRect.top,
                Context->BufferedContext,
                updateRect.left,
                updateRect.top,
                SRCCOPY
                );
        }
        else
        {
            // The window DC returned by BeginPaint is already clipped to the update region.
            PhTnpPaint(WindowHandle, Context, hdc, &updateRect);
        }
    }

    EndPaint(WindowHandle, &paintStruct);
}

/**
 * Handles the WM_PRINTCLIENT message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param hdc Device context to print to.
 * \param Flags Print flags.
 */
VOID PhTnpOnPrintClient(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc,
    _In_ ULONG Flags
    )
{
    if (!(Flags & PRF_CLIENT))
        return;

    PhTnpPaint(WindowHandle, Context, hdc, &Context->ClientRect);
}

/**
 * Handles the WM_NCPAINT message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param UpdateRegion Optional region to update.
 * \return TRUE if the non-client area was painted, FALSE otherwise.
 */
BOOLEAN PhTnpOnNcPaint(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ HRGN UpdateRegion
    )
{
    PhTnpInitializeThemeData(Context);

    // Themed border. The visual style border follows the system light/dark setting, so when the
    // control carries our own theme it has to be drawn from the palette instead. (dmex)
    if ((Context->ExtendedStyle & WS_EX_CLIENTEDGE) && (Context->ThemeSupport || Context->ThemeData))
    {
        HDC hdc;
        ULONG flags;

        if (UpdateRegion == HRGN_FULL)
            UpdateRegion = NULL;

        // Note the use of undocumented flags below. GetDCEx doesn't work without these.

        flags = DCX_WINDOW | DCX_CACHE | DCX_USESTYLE;

        if (UpdateRegion)
            flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

        if (hdc = GetDCEx(WindowHandle, UpdateRegion, flags))
        {
            PhTnpDrawThemedBorder(Context, hdc);
            ReleaseDC(WindowHandle, hdc);
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Handles the WM_SETCURSOR message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param CursorWindowHandle Handle to the window receiving the cursor.
 * \param HitTest Hit test value.
 * \param Source Source of the cursor event.
 * \return TRUE if the cursor was set, FALSE otherwise.
 */
BOOLEAN PhTnpOnSetCursor(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HWND CursorWindowHandle,
    _In_ ULONG HitTest,
    _In_ ULONG Source
    )
{
    POINT point;

    if (
        PhGetClientPos(WindowHandle, &point) &&
        TNP_HIT_TEST_FIXED_DIVIDER(point.x, Context)
        )
    {
        PhSetCursor(PhLoadDividerCursor());
        return TRUE;
    }

    if (Context->Cursor)
    {
        PhSetCursor(Context->Cursor);
        return TRUE;
    }

    return FALSE;
}

/**
 * Handles the WM_TIMER message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param Id Timer identifier.
 */
VOID PhTnpOnTimer(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Id
    )
{
    if (Id == TNP_TIMER_ANIMATE_DIVIDER)
    {
        RECT dividerRect;

        dividerRect.left = Context->FixedWidth;
        dividerRect.top = Context->HeaderHeight;
        dividerRect.right = Context->FixedWidth + 1;
        dividerRect.bottom = Context->ClientRect.bottom;

        if (Context->AnimateDividerFadingIn)
        {
            Context->DividerHot += TNP_ANIMATE_DIVIDER_INCREMENT;

            if (Context->DividerHot >= 100)
            {
                Context->DividerHot = 100;
                Context->AnimateDividerFadingIn = FALSE;
                PhKillTimer(WindowHandle, TNP_TIMER_ANIMATE_DIVIDER);
            }

            PhpTnpInvalidateRect(Context, &dividerRect);
        }
        else if (Context->AnimateDividerFadingOut)
        {
            if (Context->DividerHot <= TNP_ANIMATE_DIVIDER_DECREMENT)
            {
                Context->DividerHot = 0;
                Context->AnimateDividerFadingOut = FALSE;
                PhKillTimer(WindowHandle, TNP_TIMER_ANIMATE_DIVIDER);
            }
            else
            {
                Context->DividerHot -= TNP_ANIMATE_DIVIDER_DECREMENT;
            }

            PhpTnpInvalidateRect(Context, &dividerRect);
        }
    }
}

/**
 * Handles the WM_MOUSEMOVE message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param VirtualKeys Virtual key flags.
 * \param CursorX X coordinate of the cursor.
 * \param CursorY Y coordinate of the cursor.
 */
VOID PhTnpOnMouseMove(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKeys,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    )
{
    if (FlagOn(Context->Style, TN_STYLE_DRAG_REORDER_ROWS))
    {
        // Reorder drag in progress: update target and caret, swallow normal processing (dmex)

        if (Context->ReorderDragActive)
        {
            if (Context->ReorderJustStarted)
            {
                Context->ReorderJustStarted = FALSE;
            }

            if (Context->ReorderCursor)
            {
                PhSetCursor(Context->ReorderCursor);
            }

            PhTnpReorderUpdate(Context, CursorX, CursorY);
            return;
        }
    }

    TRACKMOUSEEVENT trackMouseEvent;

    trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
    trackMouseEvent.dwFlags = TME_LEAVE;
    trackMouseEvent.hwndTrack = WindowHandle;
    trackMouseEvent.dwHoverTime = 0;
    TrackMouseEvent(&trackMouseEvent);

    if (Context->Tracking)
    {
        ULONG newFixedWidth;

        newFixedWidth = Context->TrackOldFixedWidth + (CursorX - Context->TrackStartX);
        PhTnpSetFixedWidth(Context, newFixedWidth);
    }

    PhTnpProcessMoveMouse(Context, CursorX, CursorY);
}

/**
 * Handles the WM_MOUSELEAVE message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 */
VOID PhTnpOnMouseLeave(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    RECT rect;

    if (Context->HotNodeIndex != ULONG_MAX && Context->ThemeData)
    {
        // Update the old hot node because it may have a different non-hot background and plus minus part.
        if (PhTnpGetRowRects(Context, Context->HotNodeIndex, Context->HotNodeIndex, TRUE, &rect))
        {
            PhpTnpInvalidateRect(Context, &rect);
        }
    }

    Context->HotNodeIndex = ULONG_MAX;

    if (Context->AnimateDivider && Context->FixedDividerVisible)
    {
        if ((Context->DividerHot != 0 || Context->AnimateDividerFadingIn) && !Context->AnimateDividerFadingOut)
        {
            // Fade out the divider.
            Context->AnimateDividerFadingOut = TRUE;
            Context->AnimateDividerFadingIn = FALSE;
            PhSetTimer(Context->Handle, TNP_TIMER_ANIMATE_DIVIDER, TNP_ANIMATE_DIVIDER_INTERVAL, NULL);
        }
    }

    if (Context->TooltipIndex != ULONG_MAX || Context->TooltipId != ULONG_MAX)
    {
        // Hide the tooltip when the mouse leaves the window or we lose focus. This fixes a certain tooltip bug
        // when hovering over an item to show the tooltip while alt-tabbing causes the tooltip to remain stuck
        // on screen. There's also a similar issue when a window steals focus just as the tooltip becomes visible
        // and also causes the tooltip to remain stuck on screen. Popping here fixes both issues. (dmex)
        PhTnpPopTooltip(Context);
    }
}

/**
 * Handles mouse button messages (down, up, double-click) for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param Message Mouse message identifier.
 * \param VirtualKeys Virtual key flags.
 * \param CursorX X coordinate of the cursor.
 * \param CursorY Y coordinate of the cursor.
 */
VOID PhTnpOnXxxButtonXxx(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Message,
    _In_ ULONG VirtualKeys,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    )
{
    BOOLEAN startingTracking;
    PH_TREENEW_HIT_TEST hitTest;
    LOGICAL controlKey;
    LOGICAL shiftKey;
    RECT rect;
    ULONG changedStart;
    ULONG changedEnd;
    ULONG clickMessage;

    // Focus

    if (Message == WM_LBUTTONDOWN || Message == WM_RBUTTONDOWN)
        SetFocus(WindowHandle);

    // Divider tracking

    startingTracking = FALSE;

    switch (Message)
    {
    case WM_LBUTTONDOWN:
        {
            if (TNP_HIT_TEST_FIXED_DIVIDER(CursorX, Context))
            {
                startingTracking = TRUE;
                Context->Tracking = TRUE;
                Context->TrackStartX = CursorX;
                Context->TrackOldFixedWidth = Context->FixedWidth;
                SetCapture(WindowHandle);

                PhSetTimer(WindowHandle, TNP_TIMER_NULL, 100, NULL); // make sure we get messages once in a while so we can detect the escape key
                GetAsyncKeyState(VK_ESCAPE);
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if (Context->Tracking)
            {
                ReleaseCapture();
            }
        }
        break;
    case WM_RBUTTONDOWN:
        {
            if (Context->Tracking)
            {
                PhTnpCancelTrack(Context);
            }
        }
        break;
    }

    if (!startingTracking && Context->Tracking) // still OK to process further if the user is only starting to drag the divider
        return;

    hitTest.Point.x = CursorX;
    hitTest.Point.y = CursorY;
    hitTest.InFlags = TN_TEST_COLUMN | TN_TEST_SUBITEM;
    PhTnpHitTest(Context, &hitTest);

    if (FlagOn(Context->Style, TN_STYLE_DRAG_REORDER_ROWS))
    {
        // Begin reorder drag if:
        // - TN_STYLE_DRAG_REORDER_ROWS flag enabled
        // - Left button down on item content (not plus/minus, not divider)
        // - Ctrl is held (to avoid conflict with drag selection)
        if (Message == WM_LBUTTONDOWN &&
            (VirtualKeys & MK_CONTROL) &&
            (hitTest.Flags & TN_HIT_ITEM) &&
            !(hitTest.Flags & (TN_HIT_ITEM_PLUSMINUS | TN_HIT_DIVIDER)))
        {
            PH_TREENEW_REORDER_EVENT reorderEvent = { 0 };

            memset(&reorderEvent, 0, sizeof(PH_TREENEW_REORDER_EVENT));
            reorderEvent.Source = hitTest.Node;
            reorderEvent.Target = hitTest.Node;
            reorderEvent.DropAfter = FALSE;
            reorderEvent.Allow = TRUE;

            Context->Callback(Context->Handle, TreeNewReorderBegin, &reorderEvent, NULL, Context->CallbackContext);

            if (reorderEvent.Allow)
            {
                ULONG saveIndex = hitTest.Node ? hitTest.Node->Index : ULONG_MAX;
                ULONG cancelledByMessage = 0;

                if (PhTnpDetectDrag(Context, CursorX, CursorY, TRUE, &cancelledByMessage))
                {
                    // Begin drag-reorder
                    if (saveIndex != ULONG_MAX && saveIndex < Context->FlatList->Count)
                    {
                        PhTnpReorderBegin(Context, saveIndex);
                        PhTnpReorderUpdateCaretRect(Context);
                        PhpTnpInvalidateRect(Context, &Context->ReorderInsertRect);
                        return; // swallow normal selection behavior
                    }
                }
                else
                {
                    // If detection consumed up-event, don't duplicate
                    if (cancelledByMessage == WM_LBUTTONUP)
                        return;
                }
            }
        }

        // Commit/cancel on mouse up if in reorder mode
        if (Message == WM_LBUTTONUP && Context->ReorderDragActive)
        {
            PhTnpReorderCommit(Context);
            return;
        }
        if (Message == WM_RBUTTONDOWN && Context->ReorderDragActive)
        {
            PhTnpReorderCancel(Context);
            return;
        }
    }

    controlKey = VirtualKeys & MK_CONTROL;
    shiftKey = VirtualKeys & MK_SHIFT;

    // Plus minus glyph

    if ((hitTest.Flags & TN_HIT_ITEM_PLUSMINUS) && Message == WM_LBUTTONDOWN)
    {
        PhTnpSetExpandedNode(Context, hitTest.Node, !hitTest.Node->Expanded);
    }

    // Selection

    if (!(hitTest.Flags & TN_HIT_ITEM_PLUSMINUS) && (Message == WM_LBUTTONDOWN || Message == WM_RBUTTONDOWN))
    {
        LOGICAL allowDragSelect;
        PH_TREENEW_CELL_PARTS parts;

        PhTnpPopTooltip(Context);
        allowDragSelect = TRUE;

        if (hitTest.Flags & TN_HIT_ITEM)
        {
            allowDragSelect = FALSE;
            Context->FocusNode = hitTest.Node;

            if (Context->ExtendedFlags & TN_FLAG_ITEM_DRAG_SELECT)
            {
                // To allow drag selection to begin even if the cursor is on an item, we check if
                // the cursor is on the item icon or text. Exceptions are:
                // * When the item is already selected
                // * When user is beginning to drag the divider

                if (!hitTest.Node->Selected && !startingTracking)
                {
                    if (PhTnpGetCellParts(Context, hitTest.Node->Index, hitTest.Column, TN_MEASURE_TEXT, &parts))
                    {
                        allowDragSelect = TRUE;

                        if ((parts.Flags & TN_PART_ICON) && CursorX >= parts.IconRect.left && CursorX < parts.IconRect.right)
                            allowDragSelect = FALSE;

                        if ((parts.Flags & TN_PART_CONTENT) && (parts.Flags & TN_PART_TEXT))
                        {
                            if (CursorX >= parts.TextRect.left && CursorX < parts.TextRect.right)
                                allowDragSelect = FALSE;
                        }
                    }
                }
            }

            PhTnpProcessSelectNode(Context, hitTest.Node, controlKey, shiftKey, Message == WM_RBUTTONDOWN);
        }

        if (allowDragSelect)
        {
            BOOLEAN dragSelect;
            ULONG indexToSelect;
            BOOLEAN selectionProcessed;
            BOOLEAN showContextMenu;

            dragSelect = FALSE;
            indexToSelect = ULONG_MAX;
            selectionProcessed = FALSE;
            showContextMenu = FALSE;

            if (!(hitTest.Flags & (TN_HIT_LEFT | TN_HIT_RIGHT | TN_HIT_ABOVE | TN_HIT_BELOW)) && !startingTracking) // don't interfere with divider
            {
                BOOLEAN result;
                ULONG saveIndex;
                ULONG saveId;
                ULONG cancelledByMessage;

                // Check for drag selection. PhTnpDetectDrag has its own message loop, so we need to
                // clear our pointers before we continue or we will have some access violations when
                // items get deleted.

                if (hitTest.Node)
                    saveIndex = hitTest.Node->Index;
                else
                    saveIndex = ULONG_MAX;

                if (hitTest.Column)
                    saveId = hitTest.Column->Id;
                else
                    saveId = ULONG_MAX;

                result = PhTnpDetectDrag(Context, CursorX, CursorY, TRUE, &cancelledByMessage);

                // Restore the pointers.

                if (saveIndex == ULONG_MAX)
                    hitTest.Node = NULL;
                else if (saveIndex < Context->FlatList->Count)
                    hitTest.Node = Context->FlatList->Items[saveIndex];
                else
                    return;

                if (saveId != ULONG_MAX && !(hitTest.Column = PhTnpLookupColumnById(Context, saveId)))
                    return;

                if (result)
                {
                    dragSelect = TRUE;

                    if ((hitTest.Flags & TN_HIT_ITEM) && (Context->ExtendedFlags & TN_FLAG_ITEM_DRAG_SELECT))
                    {
                        // Include the current node before starting the drag selection, otherwise
                        // the user will never be able to select the current node.
                        if (hitTest.Node)
                        {
                            indexToSelect = hitTest.Node->Index;
                        }
                    }
                }
                else
                {
                    if ((Message == WM_LBUTTONDOWN && cancelledByMessage == WM_LBUTTONUP) ||
                        (Message == WM_RBUTTONDOWN && cancelledByMessage == WM_RBUTTONUP))
                    {
                        POINT point;

                        if ((hitTest.Flags & TN_HIT_ITEM) && (Context->ExtendedFlags & TN_FLAG_ITEM_DRAG_SELECT))
                        {
                            // The user isn't performing a drag selection, so prevent deselection.
                            selectionProcessed = TRUE;
                        }

                        // The button up message gets consumed by PhTnpDetectDrag, so send the mouse
                        // event here.
                        // Check if the cursor stayed in the same place.

                        if (
                            PhGetClientPos(Context->Handle, &point) && 
                            point.x == CursorX && point.y == CursorY
                            )
                        {
                            PhTnpSendMouseEvent(
                                Context,
                                Message == WM_LBUTTONDOWN ? TreeNewLeftClick : TreeNewRightClick,
                                CursorX,
                                CursorY,
                                hitTest.Node,
                                hitTest.Column,
                                VirtualKeys
                                );
                        }

                        if (Message == WM_RBUTTONDOWN)
                            showContextMenu = TRUE;
                    }
                }
            }

            if (!selectionProcessed && !controlKey && !shiftKey)
            {
                // Nothing: deselect everything.

                PhTnpSelectRange(Context, indexToSelect, indexToSelect, TN_SELECT_RESET, &changedStart, &changedEnd);

                if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                {
                    PhpTnpInvalidateRect(Context, &rect);
                }
            }

            if (dragSelect)
            {
                PhTnpDragSelect(Context, CursorX, CursorY);
            }

            if (showContextMenu)
            {
                SendMessage(Context->Handle, WM_CONTEXTMENU, (WPARAM)Context->Handle, GetMessagePos());
            }

            return;
        }
    }

    // Click, double-click
    // Note: If TN_FLAG_ITEM_DRAG_SELECT is enabled, the code below that processes WM_xBUTTONDOWN
    // and WM_xBUTTONUP messages only takes effect when the user clicks directly on an item's icon
    // or text.

    clickMessage = ULONG_MAX;

    if (Message == WM_LBUTTONDOWN || Message == WM_RBUTTONDOWN || Message == WM_MBUTTONDOWN)
    {
        if (Context->MouseDownLast != 0 && Context->MouseDownLast != Message)
        {
            // User pressed one button and pressed the other without letting go of the first one.
            // This counts as a click.

            if (Context->MouseDownLast == WM_LBUTTONDOWN)
                clickMessage = TreeNewLeftClick;
            else if (Context->MouseDownLast == WM_RBUTTONDOWN)
                clickMessage = TreeNewRightClick;
            else
                clickMessage = TreeNewMiddleClick;
        }

        Context->MouseDownLast = Message;
        Context->MouseDownLocation.x = CursorX;
        Context->MouseDownLocation.y = CursorY;
    }
    else if (Message == WM_LBUTTONUP || Message == WM_RBUTTONUP || Message == WM_MBUTTONUP)
    {
        if (Context->MouseDownLast != 0 &&
            Context->MouseDownLocation.x == CursorX && Context->MouseDownLocation.y == CursorY)
        {
            if (Context->MouseDownLast == WM_LBUTTONDOWN)
                clickMessage = TreeNewLeftClick;
            else if (Context->MouseDownLast == WM_RBUTTONDOWN)
                clickMessage = TreeNewRightClick;
            else
                clickMessage = TreeNewMiddleClick;
        }

        Context->MouseDownLast = 0;
    }
    else if (Message == WM_LBUTTONDBLCLK)
    {
        clickMessage = TreeNewLeftDoubleClick;
    }
    else if (Message == WM_RBUTTONDBLCLK)
    {
        clickMessage = TreeNewRightDoubleClick;
    }

    if (!(hitTest.Flags & TN_HIT_ITEM_PLUSMINUS) && clickMessage != ULONG_MAX)
    {
        PhTnpSendMouseEvent(Context, clickMessage, CursorX, CursorY, hitTest.Node, hitTest.Column, VirtualKeys);
    }
}

/**
 * Handles the WM_CAPTURECHANGED message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 */
VOID PhTnpOnCaptureChanged(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    Context->Tracking = FALSE;
    PhKillTimer(WindowHandle, TNP_TIMER_NULL);

    if (FlagOn(Context->Style, TN_STYLE_DRAG_REORDER_ROWS))
    {
        if (Context->ReorderDragActive && !Context->ReorderJustStarted)
            PhTnpReorderCancel(Context);
    }
}

/**
 * Handles the WM_KEYDOWN message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param VirtualKey Virtual key code.
 * \param Data Additional key data.
 */
VOID PhTnpOnKeyDown(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKey,
    _In_ ULONG Data
    )
{
    PH_TREENEW_KEY_EVENT keyEvent;

    if (FlagOn(Context->Style, TN_STYLE_DRAG_REORDER_ROWS))
    {
        // Cancel reorder drag with ESC
        if (Context->ReorderDragActive && VirtualKey == VK_ESCAPE)
        {
            PhTnpReorderCancel(Context);
            return;
        }
    }

    keyEvent.Handled = FALSE;
    keyEvent.VirtualKey = VirtualKey;
    keyEvent.Data = Data;
    Context->Callback(Context->Handle, TreeNewKeyDown, &keyEvent, NULL, Context->CallbackContext);

    if (keyEvent.Handled)
        return;

    if (PhTnpProcessFocusKey(Context, VirtualKey))
        return;
    if (PhTnpProcessNodeKey(Context, VirtualKey))
        return;

    // handle standard key presses
    switch (VirtualKey)
    {
    case 'A':
        if (GetKeyState(VK_CONTROL) < 0)
            TreeNew_SelectRange(WindowHandle, 0, -1);
        return;
    }

    // pass unhandled key presses to parent
    SendMessage(GetParent(WindowHandle), WM_KEYDOWN, VirtualKey, Data);
}

/**
 * Handles the WM_CHAR message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param Character The character code.
 * \param Data Additional data.
 */
VOID PhTnpOnChar(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Character,
    _In_ ULONG Data
    )
{
    // Make sure the character is printable.
    if (Character >= ' ' && Character <= '~')
    {
        PhTnpProcessSearchKey(Context, Character);
    }
}

/**
 * Handles the WM_MOUSEWHEEL message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param Distance Wheel delta.
 * \param VirtualKeys Virtual key flags.
 * \param CursorX X coordinate of the cursor.
 * \param CursorY Y coordinate of the cursor.
 */
VOID PhTnpOnMouseWheel(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG Distance,
    _In_ ULONG VirtualKeys,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    )
{
    // The normal mouse wheel can affect both the vertical scrollbar and the horizontal scrollbar,
    // but the vertical scrollbar takes precedence.
    if (Context->VScrollVisible)
    {
        PhTnpProcessMouseVWheel(Context, -Distance);
    }
    else if (Context->HScrollVisible)
    {
        PhTnpProcessMouseHWheel(Context, -Distance);
    }
}

/**
 * Handles the WM_MOUSEHWHEEL message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param Distance Wheel delta.
 * \param VirtualKeys Virtual key flags.
 * \param CursorX X coordinate of the cursor.
 * \param CursorY Y coordinate of the cursor.
 */
VOID PhTnpOnMouseHWheel(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG Distance,
    _In_ ULONG VirtualKeys,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    )
{
    PhTnpProcessMouseHWheel(Context, Distance);
}

/**
 * Handles the WM_CONTEXTMENU message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param CursorScreenX X coordinate of the context menu (screen coordinates).
 * \param CursorScreenY Y coordinate of the context menu (screen coordinates).
 */
VOID PhTnpOnContextMenu(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG CursorScreenX,
    _In_ LONG CursorScreenY
    )
{
    POINT clientPoint;
    BOOLEAN keyboardInvoked;
    PH_TREENEW_HIT_TEST hitTest;
    PH_TREENEW_CONTEXT_MENU contextMenu;

    if (CursorScreenX == INT_ERROR && CursorScreenY == INT_ERROR)
    {
        ULONG i;
        BOOLEAN found;
        RECT windowRect;
        RECT rect;

        keyboardInvoked = TRUE;

        // Context menu was invoked via keyboard. Display the context menu at the selected item.

        found = FALSE;

        for (i = 0; i < Context->FlatList->Count; i++)
        {
            if (((PPH_TREENEW_NODE)Context->FlatList->Items[i])->Selected)
            {
                found = TRUE;
                break;
            }
        }

        if (found && PhTnpGetRowRects(Context, i, i, FALSE, &rect) &&
            rect.top >= Context->ClientRect.top && rect.top < Context->ClientRect.bottom)
        {
            clientPoint.x = rect.left + Context->SmallIconWidth / 2;
            clientPoint.y = rect.top + Context->RowHeight / 2;
        }
        else
        {
            clientPoint.x = 0;
            clientPoint.y = 0;
        }

        PhGetWindowRect(WindowHandle, &windowRect);
        CursorScreenX = windowRect.left + clientPoint.x;
        CursorScreenY = windowRect.top + clientPoint.y;
    }
    else
    {
        keyboardInvoked = FALSE;

        clientPoint.x = CursorScreenX;
        clientPoint.y = CursorScreenY;
        ScreenToClient(WindowHandle, &clientPoint);

        if (clientPoint.y < Context->HeaderHeight)
        {
            // Already handled by TreeNewHeaderRightClick.
            return;
        }
    }

    hitTest.Point = clientPoint;
    hitTest.InFlags = TN_TEST_COLUMN;
    PhTnpHitTest(Context, &hitTest);

    contextMenu.Location.x = CursorScreenX;
    contextMenu.Location.y = CursorScreenY;
    contextMenu.ClientLocation = clientPoint;
    contextMenu.Node = hitTest.Node;
    contextMenu.Column = hitTest.Column;
    contextMenu.KeyboardInvoked = keyboardInvoked;
    Context->ContextMenuActive = TRUE;
    Context->Callback(WindowHandle, TreeNewContextMenu, &contextMenu, NULL, Context->CallbackContext);
    Context->ContextMenuActive = FALSE;
}

/**
 * Handles the WM_VSCROLL message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param Request Scroll request type.
 * \param Position Scroll position.
 */
VOID PhTnpOnVScroll(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Request,
    _In_ USHORT Position
    )
{
    SCROLLINFO scrollInfo;
    LONG oldPosition;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_ALL;
    GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);
    oldPosition = scrollInfo.nPos;

    switch (Request)
    {
    case SB_LINEUP:
        scrollInfo.nPos--;
        break;
    case SB_LINEDOWN:
        scrollInfo.nPos++;
        break;
    case SB_PAGEUP:
        scrollInfo.nPos -= scrollInfo.nPage;
        break;
    case SB_PAGEDOWN:
        scrollInfo.nPos += scrollInfo.nPage;
        break;
    case SB_THUMBPOSITION:
        // Touch scrolling seems to give us Position but not nTrackPos. The problem is that Position
        // is a 16-bit value, so don't use it if we have too many rows.
        if (Context->FlatList->Count <= 0xffff)
            scrollInfo.nPos = Position;
#if defined(TREENEW_VSCROLL_ANCHOR)
        Context->VScrollThumbTracking = TRUE;
#endif
        break;
    case SB_THUMBTRACK:
        scrollInfo.nPos = scrollInfo.nTrackPos;
#if defined(TREENEW_VSCROLL_ANCHOR)
        Context->VScrollThumbTracking = TRUE;
#endif
        break;
    case SB_TOP:
        scrollInfo.nPos = 0;
        break;
    case SB_BOTTOM:
        scrollInfo.nPos = MAXINT;
        break;
#if defined(TREENEW_VSCROLL_ANCHOR)
    case SB_ENDSCROLL:
        // The drag/scroll interaction ended; resume anchoring on subsequent structural updates.
        Context->VScrollThumbTracking = FALSE;
        break;
#endif
    }

    scrollInfo.fMask = SIF_POS;
    SetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo, TRUE);
    GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);

    if (scrollInfo.nPos != oldPosition)
    {
        Context->VScrollPosition = scrollInfo.nPos;
        PhTnpProcessScroll(Context, (LONG)scrollInfo.nPos - oldPosition, 0);
    }
}

/**
 * Handles the WM_HSCROLL message for the treenew control.
 *
 * \param WindowHandle Handle to the window.
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param Request Scroll request type.
 * \param Position Scroll position.
 */
VOID PhTnpOnHScroll(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Request,
    _In_ USHORT Position
    )
{
    SCROLLINFO scrollInfo;
    LONG deltaX;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_ALL;
    GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);

    switch (Request)
    {
    case SB_LINELEFT:
        scrollInfo.nPos -= Context->TextMetrics.tmAveCharWidth;
        break;
    case SB_LINERIGHT:
        scrollInfo.nPos += Context->TextMetrics.tmAveCharWidth;
        break;
    case SB_PAGELEFT:
        scrollInfo.nPos -= scrollInfo.nPage;
        break;
    case SB_PAGERIGHT:
        scrollInfo.nPos += scrollInfo.nPage;
        break;
    case SB_THUMBPOSITION:
        // Touch scrolling seems to give us Position but not nTrackPos. The problem is that Position
        // is a 16-bit value, so don't use it if we have too many rows.
        if (Context->FlatList->Count <= 0xffff)
            scrollInfo.nPos = Position;
        break;
    case SB_THUMBTRACK:
        scrollInfo.nPos = scrollInfo.nTrackPos;
        break;
    case SB_LEFT:
        scrollInfo.nPos = 0;
        break;
    case SB_RIGHT:
        scrollInfo.nPos = MAXINT;
        break;
    }

    deltaX = PhTnpApplyHScrollPosition(Context, scrollInfo.nPos);

    if (deltaX != 0)
        PhTnpProcessScroll(Context, 0, deltaX);
}

/**
 * Handles WM_NOTIFY messages from child controls.
 *
 * \param WindowHandle Handle to the treenew window.
 * \param Context Pointer to the treenew context structure.
 * \param Header Pointer to the NMHDR notification structure.
 * \param Result Pointer to receive the message result.
 * \return TRUE if the message was handled, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhTnpOnNotify(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    )
{
    switch (Header->code)
    {
    case HDN_ITEMCHANGING:
    case HDN_ITEMCHANGED:
        {
            NMHEADER *nmHeader = (NMHEADER *)Header;

            if (!nmHeader->pitem)
                break;

            if (Header->code == HDN_ITEMCHANGING && Header->hwndFrom == Context->FixedHeaderHandle)
            {
                if (nmHeader->pitem->mask & HDI_WIDTH)
                {
                    if (Context->FixedColumnVisible)
                    {
                        Context->FixedWidth = nmHeader->pitem->cxy - 1;

                        if (Context->FixedWidth < Context->FixedWidthMinimum)
                            Context->FixedWidth = Context->FixedWidthMinimum;

                        Context->NormalLeft = Context->FixedWidth + 1;
                        nmHeader->pitem->cxy = Context->FixedWidth + 1;
                    }
                    else
                    {
                        Context->FixedWidth = 0;
                        Context->NormalLeft = 0;
                    }
                }
            }

            if (Header->hwndFrom == Context->FixedHeaderHandle || Header->hwndFrom == Context->HeaderHandle)
            {
                if (nmHeader->pitem->mask & HDI_WIDTH)
                {
                    // A column is being resized. Note that this arrives for every mouse movement
                    // while the divider is dragged (HDS_FULLDRAG), so the whole header must not be
                    // enumerated here - only the column that actually changed is touched. (dmex)

                    if (Header->code == HDN_ITEMCHANGING)
                    {
                        HDITEM item;

                        item.mask = HDI_WIDTH | HDI_LPARAM;

                        if (Header_GetItem(Header->hwndFrom, nmHeader->iItem, &item) && item.lParam)
                        {
                            Context->ResizingColumn = (PPH_TREENEW_COLUMN)item.lParam;
                            Context->OldColumnWidth = item.cxy;
                        }
                        else
                        {
                            Context->ResizingColumn = NULL;
                            Context->OldColumnWidth = -1;
                        }
                    }
                    else if (Header->code == HDN_ITEMCHANGED)
                    {
                        if (Context->ResizingColumn)
                        {
                            LONG delta;

                            delta = nmHeader->pitem->cxy - Context->OldColumnWidth;

                            if (delta != 0)
                            {
                                // The scroll below depends on the new width, so apply it first.
                                PhTnpUpdateColumnWidth(Context, Context->ResizingColumn, nmHeader->pitem->cxy);

                                PhTnpProcessResizeColumn(Context, Context->ResizingColumn, delta);
                                Context->Callback(Context->Handle, TreeNewColumnResized, Context->ResizingColumn, NULL, Context->CallbackContext);
                            }

                            Context->ResizingColumn = NULL;

                            // Redraw the entire window if we are displaying empty text.
                            if (Context->FlatList->Count == 0 && Context->EmptyText.Length != 0)
                                PhpTnpInvalidateRect(Context, NULL);
                        }
                        else
                        {
                            // We don't know which column changed, so resynchronize everything and
                            // redraw the entire window.
                            PhTnpUpdateColumnHeaders(Context);
                            PhTnpUpdateColumnMaps(Context);
                            PhpTnpInvalidateRect(Context, NULL);
                        }
                    }
                }
            }
        }
        break;
    case HDN_ITEMCLICK:
        {
            if ((Header->hwndFrom == Context->FixedHeaderHandle || Header->hwndFrom == Context->HeaderHandle) &&
                !(Context->Style & TN_STYLE_NO_COLUMN_SORT))
            {
                NMHEADER *nmHeader = (NMHEADER *)Header;
                HDITEM item;
                PPH_TREENEW_COLUMN column;

                // A column has been clicked, so update the sort state.

                item.mask = HDI_LPARAM;

                if (Header_GetItem(Header->hwndFrom, nmHeader->iItem, &item))
                {
                    column = (PPH_TREENEW_COLUMN)item.lParam;
                    PhTnpProcessSortColumn(Context, column);
                }
            }
        }
        break;
    case HDN_ENDDRAG:
    case NM_RELEASEDCAPTURE:
        {
            if (Header->hwndFrom == Context->HeaderHandle)
            {
                // Columns have been re-ordered, so refresh our information.
                // Note: The fixed column cannot be re-ordered.
                PhTnpUpdateColumnHeaders(Context);
                PhTnpUpdateColumnMaps(Context);
                Context->HeaderInvalidatePending = TRUE;
                Context->Callback(Context->Handle, TreeNewColumnReordered, NULL, NULL, Context->CallbackContext);
                PhpTnpInvalidateContent(Context);
            }

            // We need to invalidate the header but hwndFrom doesn't match HeaderHandle,
            // instead we'll always invalidate? (dmex)
            if (Context->ThemeSupport)
            {
                InvalidateRect(Context->HeaderHandle, NULL, FALSE);
            }
        }
        break;
    case HDN_DIVIDERDBLCLICK:
        {
            if (Header->hwndFrom == Context->FixedHeaderHandle || Header->hwndFrom == Context->HeaderHandle)
            {
                NMHEADER *nmHeader = (NMHEADER *)Header;
                HDITEM item;

                if (Context->SuspendUpdateStructure)
                    break;

                item.mask = HDI_LPARAM;

                if (Header_GetItem(Header->hwndFrom, nmHeader->iItem, &item))
                {
                    PhTnpAutoSizeColumnHeader(
                        Context,
                        Header->hwndFrom,
                        (PPH_TREENEW_COLUMN)item.lParam,
                        0
                        );
                }
            }
        }
        break;
    case NM_RCLICK:
        {
            if (Header->hwndFrom == Context->FixedHeaderHandle || Header->hwndFrom == Context->HeaderHandle)
            {
                PH_TREENEW_HEADER_MOUSE_EVENT mouseEvent;
                ULONG position;

                position = GetMessagePos();
                mouseEvent.ScreenLocation.x = GET_X_LPARAM(position);
                mouseEvent.ScreenLocation.y = GET_Y_LPARAM(position);

                mouseEvent.Location = mouseEvent.ScreenLocation;
                ScreenToClient(WindowHandle, &mouseEvent.Location);

                mouseEvent.HeaderLocation = mouseEvent.ScreenLocation;
                ScreenToClient(Header->hwndFrom, &mouseEvent.HeaderLocation);

                mouseEvent.Column = PhTnpHitTestHeader(
                    Context,
                    Header->hwndFrom == Context->FixedHeaderHandle,
                    &mouseEvent.HeaderLocation,
                    NULL
                    );

                Context->Callback(
                    WindowHandle,
                    TreeNewHeaderRightClick,
                    &mouseEvent,
                    NULL,
                    Context->CallbackContext
                    );
            }
        }
        break;
    case TTN_GETDISPINFO:
        {
            if (Header->hwndFrom == Context->TooltipsHandle)
            {
                NMTTDISPINFO *info = (NMTTDISPINFO *)Header;
                POINT point;
                PPH_STRING string;

                if (PhGetClientPos(WindowHandle, &point))
                {
                    if (PhTnpGetTooltipText(Context, &point, &string))
                    {
                        info->lpszText = string->Buffer;
                        break;
                    }
                }
            }
        }
        break;
    case TTN_SHOW:
        {
            if (Header->hwndFrom == Context->TooltipsHandle)
            {
                *Result = PhTnpPrepareTooltipShow(Context);
                return TRUE;
            }
        }
        break;
    case TTN_POP:
        {
            if (Header->hwndFrom == Context->TooltipsHandle)
            {
                PhTnpPrepareTooltipPop(Context);
            }
        }
        break;
    case NM_CUSTOMDRAW:
        {
            if (Header->hwndFrom == Context->FixedHeaderHandle || Header->hwndFrom == Context->HeaderHandle)
            {
                LPNMCUSTOMDRAW customDraw = (LPNMCUSTOMDRAW)Header;

                switch (customDraw->dwDrawStage)
                {
                case CDDS_PREPAINT:
                    {
                        *Result = CDRF_NOTIFYITEMDRAW;
                    }
                    return TRUE;
                case CDDS_ITEMPREPAINT:
                    {
                        if (TnHeaderCustomPaint(Context, customDraw))
                        {
                            *Result = CDRF_SKIPDEFAULT;
                        }
                        else
                        {
                            *Result = CDRF_DODEFAULT;
                        }
                    }
                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

/**
 * Processes treenew-specific user messages (TNM_* messages).
 *
 * \param WindowHandle Handle to the treenew window.
 * \param Context Pointer to the treenew context structure.
 * \param Message The treenew message identifier.
 * \param WParam First message-specific parameter.
 * \param LParam Second message-specific parameter.
 * \return The result of the message processing.
 */
LRESULT PhTnpOnUserMessage(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    )
{
    switch (Message)
    {
    case TNM_GETSELECTEDNODES:
        {
            PPH_TREENEW_SELECTED_NODES snapshot = (PPH_TREENEW_SELECTED_NODES)LParam;
            ULONG i;

            if (!snapshot) return FALSE;
            snapshot->Count = 0;

            if (Context->SuspendUpdateStructure || (snapshot->Capacity && !snapshot->Nodes))
                return FALSE;

            for (i = 0; i < Context->FlatList->Count; i++)
            {
                PPH_TREENEW_NODE node = Context->FlatList->Items[i];
                
                if (!node->Selected) 
                    continue;
                
                if (snapshot->Count < snapshot->Capacity)
                    snapshot->Nodes[snapshot->Count] = node;
                    
                snapshot->Count++;
            }
            return snapshot->Count <= snapshot->Capacity;
        }
    case TNM_SETSTYLEFLAGS:
        {
            ULONG mask = (ULONG)WParam;
            ULONG style;
            
            if (mask & ~TN_STYLE_RUNTIME_MASK) 
                return FALSE;
            if (Context->Tracking || Context->DragSelectionActive || Context->ReorderDragActive)
                return FALSE;
            
            style = (Context->Style & ~mask) | ((ULONG)LParam & mask);
            if (style == Context->Style) 
                return TRUE;
            
            Context->Style = style;
            Context->DoubleBuffered = !!(style & TN_STYLE_DOUBLE_BUFFERED);
            Context->AnimateDivider = Context->DoubleBuffered && !!(style & TN_STYLE_ANIMATE_DIVIDER);
            
            if (!Context->DoubleBuffered && Context->BufferedContext)
                PhTnpDestroyBufferedContext(Context);
            
            if (!Context->AnimateDivider)
            {
                Context->AnimateDividerFadingIn = FALSE;
                Context->AnimateDividerFadingOut = FALSE;
                Context->DividerHot = 0;
                KillTimer(Context->Handle, TNP_TIMER_ANIMATE_DIVIDER);
            }

            PhTnpUpdateTextMetrics(Context);
            PhTnpInvalidateLayoutCache(Context);
            PhTnpLayout(Context);
            PhpTnpInvalidateContent(Context);
        }
        return TRUE;
    case TNM_SETCALLBACK:
        {
            Context->Callback = (PPH_TREENEW_CALLBACK)LParam;
            Context->CallbackContext = (PVOID)WParam;

            if (!Context->Callback)
                Context->Callback = PhTnpNullCallback;
        }
        return TRUE;
    case TNM_NODESSTRUCTURED:
        {
            if (Context->EnableRedraw <= 0)
            {
#if defined(TREENEW_VSCROLL_ANCHOR)
                PhTnpPrepareVScrollAnchor(Context);
#endif
                // Coalesce repeated structure requests while redraw is suspended.
                Context->SuspendUpdateStructure = TRUE;
                Context->SuspendUpdateLayout = TRUE;
                PhpTnpInvalidateContent(Context);
                return TRUE;
            }

#if defined(TREENEW_VSCROLL_ANCHOR)
            PhTnpPrepareVScrollAnchor(Context);
#endif
            PhTnpRestructureNodes(Context);
            PhTnpLayout(Context);
#if !defined(TREENEW_VSCROLL_ANCHOR)
            // In the VSCROLL_ANCHOR path, PhTnpUpdateScrollBars (called from PhTnpLayout)
            // owns the invalidation decision for structural changes. Without the anchor
            // path, it cannot guarantee a full repaint when content shifts, so we must
            // explicitly invalidate here.
            PhpTnpInvalidateContent(Context);
#endif
        }
        return TRUE;
    case TNM_ADDCOLUMN:
        return PhTnpAddColumn(Context, (PPH_TREENEW_COLUMN)LParam);
    case TNM_REMOVECOLUMN:
        return PhTnpRemoveColumn(Context, (ULONG)WParam);
    case TNM_GETCOLUMN:
        return PhTnpCopyColumn(Context, (ULONG)WParam, (PPH_TREENEW_COLUMN)LParam);
    case TNM_SETCOLUMN:
        {
            PPH_TREENEW_COLUMN column = (PPH_TREENEW_COLUMN)LParam;

            return PhTnpChangeColumn(Context, (ULONG)WParam, column->Id, column);
        }
        break;
    case TNM_GETCOLUMNORDERARRAY:
        {
            ULONG count = (ULONG)WParam;
            PULONG order = (PULONG)LParam;
            ULONG i;

            if (count != Context->NumberOfColumnsByDisplay)
                return FALSE;

            for (i = 0; i < count; i++)
            {
                order[i] = Context->ColumnsByDisplay[i]->Id;
            }
        }
        return TRUE;
    case TNM_SETCOLUMNORDERARRAY:
        {
            ULONG count = (ULONG)WParam;
            PULONG order = (PULONG)LParam;
            ULONG i;
            ULONG newOrderStack[32];
            PULONG newOrder;
            PPH_TREENEW_COLUMN column;

            if (count)
            {
                if (count <= RTL_NUMBER_OF(newOrderStack))
                    newOrder = newOrderStack;
                else
                    newOrder = PhAllocate(count * sizeof(ULONG));

                for (i = 0; i < count; i++)
                {
                    if (!(column = PhTnpLookupColumnById(Context, order[i])))
                    {
                        if (newOrder != newOrderStack)
                            PhFree(newOrder);

                        return FALSE;
                    }

                    newOrder[i] = column->s.ViewIndex;
                }

                if (!Header_SetOrderArray(Context->HeaderHandle, count, newOrder))
                {
                    if (newOrder != newOrderStack)
                        PhFree(newOrder);

                    return FALSE;
                }

                if (newOrder != newOrderStack)
                    PhFree(newOrder);
            }

            PhTnpUpdateColumnHeaders(Context);
            PhTnpUpdateColumnMaps(Context);
        }
        return TRUE;
    case TNM_SETCURSOR:
        {
            Context->Cursor = (HCURSOR)LParam;
        }
        return TRUE;
    case TNM_GETSORT:
        {
            PULONG sortColumn = (PULONG)WParam;
            PPH_SORT_ORDER sortOrder = (PPH_SORT_ORDER)LParam;

            if (sortColumn)
                *sortColumn = Context->SortColumn;
            if (sortOrder)
                *sortOrder = Context->SortOrder;
        }
        return TRUE;
    case TNM_SETSORT:
        {
            ULONG sortColumn = (ULONG)WParam;
            PH_SORT_ORDER sortOrder = (PH_SORT_ORDER)LParam;
            PH_TREENEW_SORT_CHANGED_EVENT sortOrderEvent;
            PPH_TREENEW_COLUMN column;

            if (sortColumn == Context->SortColumn &&
                sortOrder == Context->SortOrder)
            {
                return TRUE; // Nothing to change (dmex)
            }

            if (sortOrder != NoSortOrder)
            {
                if (!(column = PhTnpLookupColumnById(Context, sortColumn)))
                    return FALSE;
            }
            else
            {
                sortColumn = 0;
                column = NULL;
            }

            Context->SortColumn = sortColumn;
            Context->SortOrder = sortOrder;

            PhTnpSetColumnHeaderSortIcon(Context, column);

            memset(&sortOrderEvent, 0, sizeof(PH_TREENEW_SORT_CHANGED_EVENT));
            sortOrderEvent.SortColumn = sortColumn;
            sortOrderEvent.SortOrder = sortOrder;

            Context->Callback(Context->Handle, TreeNewSortChanged, &sortOrderEvent, NULL, Context->CallbackContext);
        }
        return TRUE;
    case TNM_SETTRISTATE:
        Context->TriState = !!WParam;
        return TRUE;
    case TNM_ENSUREVISIBLE:
        {
            if (Context->SuspendUpdateStructure)
                return FALSE;

            return PhTnpEnsureVisibleNode(Context, ((PPH_TREENEW_NODE)LParam)->Index);
        }
        break;
    case TNM_SCROLL:
        PhTnpScroll(Context, (LONG)WParam, (LONG)LParam);
        return TRUE;
    case TNM_GETFLATNODECOUNT:
        if (!Context->SuspendUpdateStructure)
            return (LRESULT)Context->FlatList->Count;
        else
            return 0;
    case TNM_GETFLATNODE:
        {
            ULONG index = (ULONG)WParam;

            if (Context->SuspendUpdateStructure)
                return (LRESULT)NULL;

            if (index >= Context->FlatList->Count)
                return (LRESULT)NULL;

            return (LRESULT)Context->FlatList->Items[index];
        }
        break;
    case TNM_GETCELLTEXT:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)LParam;

            return PhTnpGetCellText(
                Context,
                getCellText->Node,
                getCellText->Id,
                &getCellText->Text
                );
        }
        break;
    case TNM_SETNODEEXPANDED:
        PhTnpSetExpandedNode(Context, (PPH_TREENEW_NODE)LParam, !!WParam);
        return TRUE;
    case TNM_GETMAXID:
        return (LRESULT)(Context->NextId - 1);
    case TNM_SETMAXID:
        {
            ULONG maxId = (ULONG)WParam;

            if (Context->NextId < maxId + 1)
            {
                Context->NextId = maxId + 1;

                if (Context->AllocatedColumns < Context->NextId)
                {
                    PhTnpExpandAllocatedColumns(Context);
                }
            }
        }
        return TRUE;
    case TNM_INVALIDATENODE:
        {
            PPH_TREENEW_NODE node = (PPH_TREENEW_NODE)LParam;
            RECT rect;

            if (Context->SuspendUpdateStructure)
                return FALSE;

            if (!node->Visible)
                return FALSE;

            if (!PhTnpGetRowRects(Context, node->Index, node->Index, TRUE, &rect))
                return FALSE;

            PhpTnpInvalidateRect(Context, &rect);
        }
        return TRUE;
    case TNM_INVALIDATENODES:
        {
            RECT rect;

            if (Context->SuspendUpdateStructure)
                return FALSE;

            if (!PhTnpGetRowRects(Context, (ULONG)WParam, (ULONG)LParam, TRUE, &rect))
                return FALSE;

            PhpTnpInvalidateRect(Context, &rect);
        }
        return TRUE;
    case TNM_GETFIXEDHEADER:
        return (LRESULT)Context->FixedHeaderHandle;
    case TNM_GETHEADER:
        return (LRESULT)Context->HeaderHandle;
    case TNM_GETTOOLTIPS:
        return (LRESULT)Context->TooltipsHandle;
    case TNM_SELECTRANGE:
    case TNM_DESELECTRANGE:
        {
            ULONG flags;
            ULONG changedStart;
            ULONG changedEnd;
            RECT rect;

            flags = 0;

            if (Message == TNM_DESELECTRANGE)
                flags |= TN_SELECT_DESELECT;

            PhTnpSelectRange(Context, (ULONG)WParam, (ULONG)LParam, flags, &changedStart, &changedEnd);

            if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
            {
                PhpTnpInvalidateRect(Context, &rect);
            }
        }
        return TRUE;
    case TNM_GETCOLUMNCOUNT:
        return (LRESULT)Context->NumberOfColumns;
    case TNM_SETREDRAW:
        PhTnpSetRedraw(Context, !!WParam);
        return (LRESULT)Context->EnableRedraw;
    case TNM_GETVIEWPARTS:
        {
            PPH_TREENEW_VIEW_PARTS parts = (PPH_TREENEW_VIEW_PARTS)LParam;

            parts->ClientRect = Context->ClientRect;
            parts->HeaderHeight = Context->HeaderHeight;
            parts->RowHeight = Context->RowHeight;
            parts->VScrollWidth = Context->VScrollVisible ? Context->VScrollWidth : 0;
            parts->HScrollHeight = Context->HScrollVisible ? Context->HScrollHeight : 0;
            parts->VScrollPosition = Context->VScrollPosition;
            parts->HScrollPosition = Context->HScrollPosition;
            parts->FixedWidth = Context->FixedWidth;
            parts->NormalLeft = Context->NormalLeft;
            parts->NormalWidth = Context->TotalViewX;
            parts->ScrollTickCount = (NtGetTickCount64() - Context->ScrollTickCount);
        }
        return TRUE;
    case TNM_GETFIXEDCOLUMN:
        return (LRESULT)Context->FixedColumn;
    case TNM_GETFIRSTCOLUMN:
        return (LRESULT)Context->FirstColumn;
    case TNM_SETFOCUSNODE:
        Context->FocusNode = (PPH_TREENEW_NODE)LParam;
        return TRUE;
    case TNM_SETMARKNODE:
        Context->MarkNodeIndex = ((PPH_TREENEW_NODE)LParam)->Index;
        return TRUE;
    case TNM_SETHOTNODE:
        PhTnpSetHotNode(Context, (PPH_TREENEW_NODE)LParam, FALSE);
        return TRUE;
    case TNM_SETEXTENDEDFLAGS:
        Context->ExtendedFlags = (Context->ExtendedFlags & ~(ULONG)WParam) | ((ULONG)LParam & (ULONG)WParam);
        return TRUE;
    case TNM_GETCALLBACK:
        {
            PPH_TREENEW_CALLBACK *callback = (PPH_TREENEW_CALLBACK *)LParam;
            PVOID *callbackContext = (PVOID *)WParam;

            if (callback)
            {
                if (Context->Callback != PhTnpNullCallback)
                    *callback = Context->Callback;
                else
                    *callback = NULL;
            }

            if (callbackContext)
            {
                *callbackContext = Context->CallbackContext;
            }
        }
        return TRUE;
    case TNM_HITTEST:
        PhTnpHitTest(Context, (PPH_TREENEW_HIT_TEST)LParam);
        return TRUE;
    case TNM_GETVISIBLECOLUMNCOUNT:
        return Context->NumberOfColumnsByDisplay + (Context->FixedColumnVisible ? 1 : 0);
    case TNM_AUTOSIZECOLUMN:
        {
            ULONG id = (ULONG)WParam;
            ULONG flags = (ULONG)LParam;
            PPH_TREENEW_COLUMN column;

            if (!(column = PhTnpLookupColumnById(Context, id)))
                return FALSE;

            if (!column->Visible)
                return FALSE;

            PhTnpAutoSizeColumnHeader(
                Context,
                column->Fixed ? Context->FixedHeaderHandle : Context->HeaderHandle,
                column,
                flags
                );
        }
        return TRUE;
    case TNM_SETEMPTYTEXT:
        {
            PPH_STRINGREF text = (PPH_STRINGREF)LParam;
            ULONG flags = (ULONG)WParam;

            Context->EmptyText = *text;

            if (Context->FlatList->Count == 0)
                PhpTnpInvalidateContent(Context);
        }
        return TRUE;
    case TNM_SETROWHEIGHT:
        {
            LONG rowHeight = (LONG)WParam;

            if (rowHeight != 0)
            {
                Context->CustomRowHeight = TRUE;
                Context->RowHeight = max(rowHeight, 1);
            }
            else
            {
                Context->CustomRowHeight = FALSE;
                PhTnpUpdateTextMetrics(Context);
            }

            // Every row moved, and the scroll bar page size is derived from the row height. (dmex)
            PhTnpLayout(Context);
            PhpTnpInvalidateContent(Context);
        }
        return TRUE;
    case TNM_ISFLATNODEVALID:
        return !Context->SuspendUpdateStructure;
    case TNM_THEMESUPPORT:
        {
            if (Context->ThemeSupport == (ULONG)!!WParam)
                return TRUE; // nothing to change

            Context->ThemeSupport = !!WParam;

            // The rows, the header and the border are all drawn differently now. (dmex)

            if (Context->HeaderThemeHandle)
            {
                PhCloseThemeData(Context->HeaderThemeHandle);
                Context->HeaderThemeHandle = NULL;
            }

            Context->HeaderInvalidatePending = TRUE;

            InvalidateRect(Context->FixedHeaderHandle, NULL, FALSE);
            InvalidateRect(Context->HeaderHandle, NULL, FALSE);
            PhpTnpInvalidateRect(Context, NULL);
        }
        return TRUE;
    case TNM_SETIMAGELIST:
        {
            Context->ImageListSupport = !!WParam;
            Context->ImageListHandle = (HIMAGELIST)WParam;

            PhpTnpInvalidateContent(Context);
        }
        return TRUE;
    case TNM_SETCOLUMNTEXTCACHE:
        {
            PPH_TREENEW_SET_HEADER_CACHE headerCache = (PPH_TREENEW_SET_HEADER_CACHE)WParam;

            Context->HeaderColumnCacheMax = headerCache->HeaderTreeColumnMax;
            Context->HeaderStringCache = headerCache->HeaderTreeColumnStringCache;
            Context->HeaderTextCache = headerCache->HeaderTreeColumnTextCache;
        }
        return TRUE;
    case TNM_ENSUREVISIBLEINDEX:
        {
            if (Context->SuspendUpdateStructure)
                return FALSE;

            return PhTnpEnsureVisibleNode(Context, (ULONG)LParam);
        }
        break;
    case TNM_GETVISIBLECOLUMN:
        {
            ULONG index = (ULONG)WParam;

            if (index >= Context->NumberOfColumnsByDisplay + (Context->FixedColumnVisible ? 1 : 0))
                return FALSE;

            if (Context->FixedColumnVisible)
            {
                if (index == 0)
                    return PhTnpCopyColumn(Context, Context->FixedColumn->Id, (PPH_TREENEW_COLUMN)LParam);
                index = Context->ColumnsByDisplay[index - 1]->Id;
            }
            else
            {
                index = Context->ColumnsByDisplay[index]->Id;
            }

            return PhTnpCopyColumn(Context, index, (PPH_TREENEW_COLUMN)LParam);
        }
        break;
    case TNM_GETVISIBLECOLUMNARRAY:
        {
            ULONG count = (ULONG)WParam;
            PULONG visible = (PULONG)LParam;
            PPH_TREENEW_COLUMN column;

            for (ULONG i = 0; i < count; i++)
            {
                if (!(column = PhTnpLookupColumnById(Context, visible[i])))
                    return FALSE;

                visible[i] = column->Visible;
            }
        }
        return TRUE;
    case TNM_GETSELECTEDCOUNT:
       {
            ULONG i;
            ULONG visibleCount;
            ULONG selectedCount;
            PPH_TREENEW_NODE node;

            if (Context->SuspendUpdateStructure)
                visibleCount = 0;
            else
                visibleCount = Context->FlatList->Count;
            selectedCount = 0;

            for (i = 0; i < visibleCount; i++)
            {
                node = Context->FlatList->Items[i];

                if (node->Selected)
                {
                    selectedCount++;
                }
            }

            return (LRESULT)selectedCount;
       }
       break;
    case TNM_GETSELECTEDNODE:
        {
            ULONG i;
            ULONG visibleCount;
            PPH_TREENEW_NODE node;

            if (Context->SuspendUpdateStructure)
                visibleCount = 0;
            else
                visibleCount = Context->FlatList->Count;

            for (i = 0; i < visibleCount; i++)
            {
                node = Context->FlatList->Items[i];

                if (node->Selected)
                {
                    return (LRESULT)node;
                }
            }
        }
        break;
    case TNM_FOCUSMARKSELECT:
       {
            PPH_TREENEW_NODE node = (PPH_TREENEW_NODE)LParam;

            SetFocus(WindowHandle);

            Context->FocusNode = node; // TNM_SETFOCUSNODE
            Context->MarkNodeIndex = node->Index; // TNM_SETMARKNODE
            PhTnpOnUserMessage(WindowHandle, Context, TNM_DESELECTRANGE, 0, -1);
            PhTnpOnUserMessage(WindowHandle, Context, TNM_SELECTRANGE, node->Index, node->Index);
            PhTnpEnsureVisibleNode(Context, node->Index); // TNM_ENSUREVISIBLE
            PhTnpOnUserMessage(WindowHandle, Context, TNM_INVALIDATENODES, node->Index, node->Index);
       }
       return TRUE;
    case TNM_FOCUSVISIBLENODE:
        {
            ULONG i;
            ULONG visibleCount;
            PPH_TREENEW_NODE node;

            if (Context->SuspendUpdateStructure)
                visibleCount = 0;
            else
                visibleCount = Context->FlatList->Count;

            for (i = 0; i < visibleCount; i++)
            {
                node = Context->FlatList->Items[i];

                // Select the first visible node.
                if (node->Visible)
                {
                    SetFocus(WindowHandle);

                    Context->FocusNode = node; // TNM_SETFOCUSNODE
                    Context->MarkNodeIndex = node->Index; // TNM_SETMARKNODE
                    PhTnpOnUserMessage(WindowHandle, Context, TNM_DESELECTRANGE, 0, -1);
                    PhTnpOnUserMessage(WindowHandle, Context, TNM_SELECTRANGE, node->Index, node->Index);
                    PhTnpEnsureVisibleNode(Context, node->Index); // TNM_ENSUREVISIBLE
                    PhTnpOnUserMessage(WindowHandle, Context, TNM_INVALIDATENODES, node->Index, node->Index);

                    return TRUE;
                }
            }
        }
        break;
    case TNM_GETCELLPARTS:
        {
            PPH_TREENEW_GET_CELL_PARTS getCellParts = (PPH_TREENEW_GET_CELL_PARTS)LParam;

            if (getCellParts->Node && getCellParts->Column)
            {
                ULONG measureFlags = 0;
                PH_TREENEW_CELL_PARTS cellparts;

                if (FlagOn(getCellParts->Flags, TN_MEASURE_TEXT))
                {
                    measureFlags |= TN_MEASURE_TEXT;
                }

                RtlZeroMemory(&cellparts, sizeof(PH_TREENEW_CELL_PARTS));

                if (PhTnpGetCellParts(
                    Context,
                    getCellParts->Node->Index,
                    getCellParts->Column,
                    measureFlags,
                    &cellparts
                    ))
                {
                    RtlCopyMemory(&getCellParts->Parts, &cellparts, sizeof(PH_TREENEW_CELL_PARTS));
                    return TRUE;
                }
            }
        }
        break;
    }

    return 0;
}

/**
 * Sets the font for the treenew control and updates related metrics.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Font Handle to the font to set, or NULL to use the default font.
 * \param Redraw TRUE to redraw the control after setting the font, FALSE otherwise.
 */
VOID PhTnpSetFont(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ HFONT Font,
    _In_ BOOLEAN Redraw
    )
{
    LOGFONT logFont;

    if (Context->FontOwned)
    {
        DeleteFont(Context->Font);
        Context->FontOwned = FALSE;
    }

    Context->Font = Font;

    if (!Context->Font)
    {
        if (PhGetSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, Context->WindowDpi))
        {
            Context->Font = CreateFontIndirect(&logFont);
            Context->FontOwned = TRUE;
        }
    }

    SetWindowFont(Context->FixedHeaderHandle, Context->Font, Redraw);
    SetWindowFont(Context->HeaderHandle, Context->Font, Redraw);

    if (Context->TooltipsHandle)
    {
        SetWindowFont(Context->TooltipsHandle, Context->Font, FALSE);
        Context->TooltipFont = Context->Font;
    }

    PhTnpUpdateTextMetrics(Context);
}

/**
 * Updates the cached system metrics used by the treenew control.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpUpdateSystemMetrics(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    Context->WindowDpi = PhGetWindowDpi(Context->Handle);

    Context->VScrollWidth = PhGetSystemMetrics(SM_CXVSCROLL, Context->WindowDpi);
    Context->HScrollHeight = PhGetSystemMetrics(SM_CYHSCROLL, Context->WindowDpi);
    Context->SystemBorderX = PhGetSystemMetrics(SM_CXBORDER, Context->WindowDpi);
    Context->SystemBorderY = PhGetSystemMetrics(SM_CYBORDER, Context->WindowDpi);
    Context->SystemEdgeX = PhGetSystemMetrics(SM_CXEDGE, Context->WindowDpi);
    Context->SystemEdgeY = PhGetSystemMetrics(SM_CYEDGE, Context->WindowDpi);
    Context->SystemDragX = PhGetSystemMetrics(SM_CXDRAG, Context->WindowDpi);
    Context->SystemDragY = PhGetSystemMetrics(SM_CYDRAG, Context->WindowDpi);
    Context->SmallIconWidth = PhGetSystemMetrics(SM_CXSMICON, Context->WindowDpi);
    Context->SmallIconHeight = PhGetSystemMetrics(SM_CYSMICON, Context->WindowDpi);

    Context->CellMarginLeft = PhScaleToDisplay(TNP_CELL_LEFT_MARGIN, Context->WindowDpi);
    Context->CellMarginRight = PhScaleToDisplay(TNP_CELL_RIGHT_MARGIN, Context->WindowDpi);
    Context->IconRightPadding = PhScaleToDisplay(TNP_ICON_RIGHT_PADDING, Context->WindowDpi);
    Context->TextMarginPadding = PhScaleToDisplay(6 + 6, Context->WindowDpi);
    Context->HeaderTextPadding = PhScaleToDisplay(5, Context->WindowDpi);
    Context->HeaderTextMargin = PhScaleToDisplay(2, Context->WindowDpi);
    Context->HeaderRowMargin = PhScaleToDisplay(1, Context->WindowDpi);

    if (Context->SystemDragX < 2)
        Context->SystemDragX = 2;
    if (Context->SystemDragY < 2)
        Context->SystemDragY = 2;
}

/**
 * Updates the text metrics and row height for the treenew control.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpUpdateTextMetrics(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    HDC hdc;

    if (hdc = GetDC(Context->Handle))
    {
        SelectFont(hdc, Context->Font);
        GetTextMetrics(hdc, &Context->TextMetrics);

        if (!Context->CustomRowHeight)
        {
            // Below we try to match the row height as calculated by the list view, even if it
            // involves magic numbers. On Vista and above there seems to be extra padding.

            Context->RowHeight = Context->TextMetrics.tmHeight;

            if (Context->Style & TN_STYLE_ICONS)
            {
                if (Context->RowHeight < Context->SmallIconHeight)
                    Context->RowHeight = Context->SmallIconHeight;
            }
            else
            {
                if (!(Context->Style & TN_STYLE_THIN_ROWS))
                    Context->RowHeight += 1; // HACK
            }

            Context->RowHeight += Context->HeaderRowMargin; // HACK

            if (!(Context->Style & TN_STYLE_THIN_ROWS))
                Context->RowHeight += Context->HeaderTextMargin; // HACK
        }

        ReleaseDC(Context->Handle, hdc);
    }
}

/**
 * Updates the theme data and colors for the treenew control.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpUpdateThemeData(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    Context->DefaultBackColor = GetSysColor(COLOR_WINDOW);
    Context->DefaultForeColor = GetSysColor(COLOR_WINDOWTEXT);
    Context->ThemeActive = !!PhIsThemeActive();

    if (Context->ThemeData)
    {
        PhCloseThemeData(Context->ThemeData);
        Context->ThemeData = NULL;
    }

    Context->ThemeData = PhOpenThemeData(Context->Handle, VSCLASS_TREEVIEW, Context->WindowDpi);

    if (Context->ThemeData)
    {
        Context->ThemeHasItemBackground = !!PhIsThemePartDefined(Context->ThemeData, TVP_TREEITEM, 0);
        Context->ThemeHasGlyph = !!PhIsThemePartDefined(Context->ThemeData, TVP_GLYPH, 0);
        Context->ThemeHasHotGlyph = !!PhIsThemePartDefined(Context->ThemeData, TVP_HOTGLYPH, 0);
    }
    else
    {
        Context->ThemeHasItemBackground = FALSE;
        Context->ThemeHasGlyph = FALSE;
        Context->ThemeHasHotGlyph = FALSE;
    }
}

/**
 * Initializes the theme data for the treenew control on first use.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpInitializeThemeData(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    if (!Context->ThemeInitialized)
    {
        PhTnpUpdateThemeData(Context);
        Context->ThemeInitialized = TRUE;
    }
}

/**
 * Cancels any active tracking operation in the treenew control.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpCancelTrack(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    PhTnpSetFixedWidth(Context, Context->TrackOldFixedWidth);
    ReleaseCapture();
}

/**
 * Resets the layout geometry cache used by PhTnpLayout / PhTnpLayoutHeader.
 * Call when child windows may have been moved/sized outside this code path
 * (theme change, DPI change, child recreate) so the next layout pass re-issues
 * real MoveWindow / SetWindowPos / Header_Layout / TTM_NEWTOOLRECT calls.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpInvalidateLayoutCache(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    PhSetRectEmpty(&Context->VScrollLastRect);
    PhSetRectEmpty(&Context->HScrollLastRect);
    PhSetRectEmpty(&Context->FillerBoxLastRect);
    PhSetRectEmpty(&Context->FixedHeaderLastOutRect);
    PhSetRectEmpty(&Context->NormalHeaderLastOutRect);
    PhSetRectEmpty(&Context->TooltipFixedHeaderLastRect);
    PhSetRectEmpty(&Context->TooltipNormalHeaderLastRect);
    Context->VScrollLastVisible = 0;
    Context->HScrollLastVisible = 0;
    Context->FillerBoxLastVisible = 0;
    Context->VScrollLastMax = -1;
    Context->HScrollLastMax = -1;
}

/**
 * Recalculates the layout of all treenew control elements.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpLayout(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    RECT clientRect;
    RECT redrawRect;
    BOOLEAN layoutRedraw = FALSE;

    if (Context->EnableRedraw <= 0)
    {
        Context->SuspendUpdateLayout = TRUE;
        return;
    }

    clientRect = Context->ClientRect;
    PhSetRectEmpty(&redrawRect);

    PhTnpUpdateScrollBars(Context);

    // Vertical scroll bar
    if (Context->VScrollVisible)
    {
        RECT rect;

        rect.left = clientRect.right - Context->VScrollWidth;
        rect.top = 0;
        rect.right = Context->VScrollWidth;
        rect.bottom = clientRect.bottom - (Context->HScrollVisible ? Context->HScrollHeight : 0);

        if (!Context->VScrollLastVisible || !PhEqualRect(&rect, &Context->VScrollLastRect))
        {
            //SetWindowPos(
            //    Context->VScrollHandle,
            //    NULL,
            //    rect.left,
            //    rect.top,
            //    rect.right,
            //    rect.bottom,
            //    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER //| SWP_NOREDRAW
            //    );

            // The child is moved without repainting, so both the area it vacates and the area it
            // occupies have to be redrawn. (dmex)
            if (Context->VScrollLastVisible)
                PhpTnpUnionChildRect(&redrawRect, &Context->VScrollLastRect);
            PhpTnpUnionChildRect(&redrawRect, &rect);

            MoveWindow(
                Context->VScrollHandle,
                rect.left,
                rect.top,
                rect.right,
                rect.bottom,
                FALSE
                );
            Context->VScrollLastRect = rect;
            Context->VScrollLastVisible = TRUE;
            layoutRedraw = TRUE;
        }
    }
    else
    {
        Context->VScrollLastVisible = FALSE;
    }

    // Horizontal scroll bar
    if (Context->HScrollVisible)
    {
        RECT rect;

        rect.left = Context->NormalLeft;
        rect.top = clientRect.bottom - Context->HScrollHeight;
        rect.right = clientRect.right - Context->NormalLeft - (Context->VScrollVisible ? Context->VScrollWidth : 0);
        rect.bottom = Context->HScrollHeight;

        if (!Context->HScrollLastVisible || !PhEqualRect(&rect, &Context->HScrollLastRect))
        {
            //SetWindowPos(
            //    Context->HScrollHandle,
            //    NULL,
            //    rect.left,
            //    rect.top,
            //    rect.right,
            //    rect.bottom,
            //    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER ///| SWP_NOREDRAW
            //    );

            if (Context->HScrollLastVisible)
                PhpTnpUnionChildRect(&redrawRect, &Context->HScrollLastRect);
            PhpTnpUnionChildRect(&redrawRect, &rect);

            MoveWindow(
                Context->HScrollHandle,
                rect.left,
                rect.top,
                rect.right,
                rect.bottom,
                FALSE
                );
            Context->HScrollLastRect = rect;
            Context->HScrollLastVisible = TRUE;
            layoutRedraw = TRUE;
        }
    }
    else
    {
        Context->HScrollLastVisible = FALSE;
    }

    // Filler box
    if (Context->VScrollVisible && Context->HScrollVisible)
    {
        RECT rect;

        rect.left = clientRect.right - Context->VScrollWidth;
        rect.top = clientRect.bottom - Context->HScrollHeight;
        rect.right = Context->VScrollWidth;
        rect.bottom = Context->HScrollHeight;

        if (!Context->FillerBoxLastVisible || !PhEqualRect(&rect, &Context->FillerBoxLastRect))
        {
            //SetWindowPos(
            //    Context->FillerBoxHandle,
            //    NULL,
            //    rect.left,
            //    rect.top,
            //    rect.right,
            //    rect.bottom,
            //    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER //| SWP_NOREDRAW
            //    );

            if (Context->FillerBoxLastVisible)
                PhpTnpUnionChildRect(&redrawRect, &Context->FillerBoxLastRect);
            PhpTnpUnionChildRect(&redrawRect, &rect);

            MoveWindow(
                Context->FillerBoxHandle,
                rect.left,
                rect.top,
                rect.right,
                rect.bottom,
                FALSE
                );
            Context->FillerBoxLastRect = rect;
            Context->FillerBoxLastVisible = TRUE;
            layoutRedraw = TRUE;
        }
    }
    else
    {
        Context->FillerBoxLastVisible = FALSE;
    }

    PhTnpLayoutHeader(Context);

    if (layoutRedraw && !PhRectEmpty(&redrawRect))
    {
        // Only the strips the scroll bars vacated or now occupy are dirty. (dmex)
        RedrawWindow(
            Context->Handle,
            &redrawRect,
            NULL,
            RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_NOERASE
            );
    }

    // Redraw the entire window if we are displaying empty text.
    if (Context->FlatList->Count == 0 && Context->EmptyText.Length != 0)
        PhpTnpInvalidateRect(Context, NULL);
}

/**
 * Recalculates the layout of the header controls.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpLayoutHeader(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    RECT rect;
    HDLAYOUT hdl;
    WINDOWPOS windowPos;

    hdl.prc = &rect;
    hdl.pwpos = &windowPos;

    if (!(Context->Style & TN_STYLE_NO_COLUMN_HEADER))
    {
        LONG headerHeight = 0;

        if (Context->HeaderCustomDraw)
        {
            // HACK: Use the row height instead of querying the font. (dmex)
            if (Context->RowHeight)
            {
                headerHeight = Context->RowHeight + 1;
            }
            else
            {
                TEXTMETRIC textMetrics;
                HDC hdc;

                if (hdc = GetDC(Context->FixedHeaderHandle))
                {
                    SelectFont(hdc, GetWindowFont(Context->FixedHeaderHandle));
                    GetTextMetrics(hdc, &textMetrics);

                    // Below we try to match the height as calculated by the header, even if it
                    // involves magic numbers. On Vista and above there seems to be extra padding.

                    headerHeight = textMetrics.tmHeight;
                    ReleaseDC(Context->FixedHeaderHandle, hdc);
                }

                headerHeight += 5; // Add magic padding
            }
        }

        RECT outRect;

        // Fixed portion header control
        rect.left = 0;
        rect.top = 0;
        rect.right = Context->NormalLeft;
        rect.bottom = Context->ClientRect.bottom;
        Header_Layout(Context->FixedHeaderHandle, &hdl);
        outRect.left = windowPos.x;
        outRect.top = windowPos.y;
        outRect.right = windowPos.cx;
        outRect.bottom = windowPos.cy + headerHeight;

        if (!PhEqualRect(&outRect, &Context->FixedHeaderLastOutRect))
        {
            SetWindowPos(
                Context->FixedHeaderHandle,
                NULL,
                outRect.left,
                outRect.top,
                outRect.right,
                outRect.bottom,
                windowPos.flags | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOREDRAW
                );
            Context->FixedHeaderLastOutRect = outRect;

            // SWP_NOREDRAW suppresses the repaint of the moved window, so repaint it here. (dmex)
            InvalidateRect(Context->FixedHeaderHandle, NULL, FALSE);
        }
        Context->HeaderHeight = outRect.bottom;

        // Normal portion header control
        rect.left = Context->NormalLeft - Context->HScrollPosition;
        rect.top = 0;
        rect.right = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);
        rect.bottom = Context->ClientRect.bottom;
        Header_Layout(Context->HeaderHandle, &hdl);
        outRect.left = windowPos.x;
        outRect.top = windowPos.y;
        outRect.right = windowPos.cx;
        outRect.bottom = windowPos.cy + headerHeight;

        if (!PhEqualRect(&outRect, &Context->NormalHeaderLastOutRect))
        {
            SetWindowPos(
                Context->HeaderHandle,
                NULL,
                outRect.left,
                outRect.top,
                outRect.right,
                outRect.bottom,
                windowPos.flags | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOREDRAW
                );
            Context->NormalHeaderLastOutRect = outRect;

            // The normal header moves horizontally when the view is scrolled. SWP_NOREDRAW leaves
            // the old pixels in place, so invalidate the new position. Only the part right of the
            // fixed header is visible; invalidating the whole window leaves artifacts over the
            // fixed header on systems where the sibling clipping is not honored. (dmex)
            {
                RECT invalidateRect;

                invalidateRect.left = Context->HScrollPosition;
                invalidateRect.top = 0;
                invalidateRect.right = outRect.right;
                invalidateRect.bottom = outRect.bottom;
                InvalidateRect(Context->HeaderHandle, &invalidateRect, FALSE);
            }
        }
    }
    else
    {
        Context->HeaderHeight = 0;
    }

    if (Context->TooltipsHandle)
    {
        TOOLINFO toolInfo;

        memset(&toolInfo, 0, sizeof(TOOLINFO));
        toolInfo.cbSize = sizeof(TOOLINFO);
        toolInfo.hwnd = Context->FixedHeaderHandle;
        toolInfo.uId = TNP_TOOLTIPS_FIXED_HEADER;

        if (PhGetClientRect(Context->FixedHeaderHandle, &toolInfo.rect))
        {
            if (!PhEqualRect(&toolInfo.rect, &Context->TooltipFixedHeaderLastRect))
            {
                SendMessage(Context->TooltipsHandle, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);
                Context->TooltipFixedHeaderLastRect = toolInfo.rect;
            }
        }

        memset(&toolInfo, 0, sizeof(TOOLINFO));
        toolInfo.cbSize = sizeof(TOOLINFO);
        toolInfo.hwnd = Context->HeaderHandle;
        toolInfo.uId = TNP_TOOLTIPS_HEADER;

        if (PhGetClientRect(Context->HeaderHandle, &toolInfo.rect))
        {
            if (!PhEqualRect(&toolInfo.rect, &Context->TooltipNormalHeaderLastRect))
            {
                SendMessage(Context->TooltipsHandle, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);
                Context->TooltipNormalHeaderLastRect = toolInfo.rect;
            }
        }
    }
}

/**
 * Sets the width of the fixed column.
 *
 * \param Context Pointer to the treenew context structure.
 * \param FixedWidth The new width for the fixed column.
 */
VOID PhTnpSetFixedWidth(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG FixedWidth
    )
{
    HDITEM item;

    if (Context->FixedColumnVisible)
    {
        Context->FixedWidth = FixedWidth;

        if (Context->FixedWidth < Context->FixedWidthMinimum)
            Context->FixedWidth = Context->FixedWidthMinimum;

        Context->NormalLeft = Context->FixedWidth + 1;

        item.mask = HDI_WIDTH;
        item.cxy = Context->FixedWidth + 1;
        Header_SetItem(Context->FixedHeaderHandle, 0, &item);
    }
    else
    {
        Context->FixedWidth = 0;
        Context->NormalLeft = 0;
    }
}

#if defined(TREENEW_VSCROLL_ANCHOR)
/**
 * Computes the maximum valid vertical scroll position for the specified row count and page size.
 *
 * \param Count Number of rows in the flat list.
 * \param RowsPerPage Number of visible rows in the viewport.
 * \return The maximum valid vertical scroll position.
 */
LONG PhTnpGetMaxVScrollPosition(
    _In_ ULONG Count,
    _In_ LONG RowsPerPage
    )
{
    LONG maxPosition;

    maxPosition = Count != 0 ? (LONG)Count - 1 : 0;

    if (RowsPerPage > 0)
        maxPosition -= RowsPerPage - 1;

    return __max(maxPosition, 0);
}

/**
 * Finds a node in the current flat list using pointer identity.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Node Pointer to the node to locate.
 * \param Index Receives the node index when found.
 * \return TRUE if the node is present in the flat list; otherwise, FALSE.
 */
_Success_(return)
BOOLEAN PhTnpFindFlatListNode(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_NODE Node,
    _Out_ PULONG Index
    )
{
    // Fast path: check if the node's cached index is still valid
    if (
        Node->Index < Context->FlatList->Count &&
        Context->FlatList->Items[Node->Index] == Node
        )
    {
        *Index = Node->Index;
        return TRUE;
    }

    // Fallback to linear search if node moved
    for (ULONG i = 0; i < Context->FlatList->Count; i++)
    {
        if (Context->FlatList->Items[i] == Node)
        {
            *Index = i;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Captures the current viewport anchor so a structural update can restore it afterwards.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpPrepareVScrollAnchor(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    SCROLLINFO scrollInfo;
    LONG maxPosition;

    if (FlagOn(Context->VScrollAnchorFlags, PH_TREENEW_VSCROLL_ANCHOR_PENDING))
        return;

    if (Context->VScrollThumbTracking)
        return; // user is driving the position by hand; don't fight the drag

    Context->VScrollAnchorFlags = PH_TREENEW_VSCROLL_ANCHOR_PENDING;
    Context->VScrollAnchorNode = NULL;
    Context->FlatListAnchorEnd = FALSE;

    if (Context->VScrollPosition <= 0 || Context->FlatList->Count == 0)
    {
        SetFlag(Context->VScrollAnchorFlags, PH_TREENEW_VSCROLL_ANCHOR_START);
        return;
    }

    ZeroMemory(&scrollInfo, sizeof(SCROLLINFO));
    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
    GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);

    maxPosition = PhTnpGetMaxVScrollPosition(__max((LONG)scrollInfo.nMax + 1, 0), scrollInfo.nPage);

    if (Context->VScrollPosition >= maxPosition)
    {
        SetFlag(Context->VScrollAnchorFlags, PH_TREENEW_VSCROLL_ANCHOR_END);
        Context->FlatListAnchorEnd = TRUE;
        return;
    }

    if ((ULONG)Context->VScrollPosition < Context->FlatList->Count)
    {
        Context->VScrollAnchorNode = Context->FlatList->Items[Context->VScrollPosition];
        SetFlag(Context->VScrollAnchorFlags, PH_TREENEW_VSCROLL_ANCHOR_NODE);
    }
}

/**
 * Computes the anchored vertical scroll position after a structural update.
 *
 * \param Context Pointer to the treenew context structure.
 * \param RowsPerPage Number of visible rows in the viewport.
 * \param Position Receives the anchored scroll position.
 * \return TRUE if an anchored position was resolved; otherwise, FALSE.
 */
_Success_(return)
BOOLEAN PhTnpGetAnchoredVScrollPosition(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG RowsPerPage,
    _Out_ PLONG Position
    )
{
    BOOLEAN anchored;
    ULONG index;

    anchored = FALSE;

    if (!FlagOn(Context->VScrollAnchorFlags, PH_TREENEW_VSCROLL_ANCHOR_PENDING))
        return FALSE;

    if (FlagOn(Context->VScrollAnchorFlags, PH_TREENEW_VSCROLL_ANCHOR_START))
    {
        *Position = 0;
        anchored = TRUE;
    }
    else if (FlagOn(Context->VScrollAnchorFlags, PH_TREENEW_VSCROLL_ANCHOR_END))
    {
        *Position = PhTnpGetMaxVScrollPosition(Context->FlatList->Count, RowsPerPage);
        anchored = TRUE;
    }
    else if (
        FlagOn(Context->VScrollAnchorFlags, PH_TREENEW_VSCROLL_ANCHOR_NODE) &&
        Context->VScrollAnchorNode &&
        PhTnpFindFlatListNode(Context, Context->VScrollAnchorNode, &index)
        )
    {
        *Position = index;
        anchored = TRUE;
    }

    Context->VScrollAnchorFlags = 0;
    Context->VScrollAnchorNode = NULL;

    return anchored;
}
#endif // #if defined(TREENEW_VSCROLL_ANCHOR)

/**
 * Enables or disables redrawing of the treenew control.
 * When the outermost redraw suspension ends, any deferred structure/layout
 * updates requested via TNM_NODESSTRUCTURED are committed once here.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Redraw TRUE to enable redrawing, FALSE to disable.
 */
VOID PhTnpSetRedraw(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ BOOLEAN Redraw
    )
{
    if (Redraw)
    {
        // Clamp instead of trusting every caller to balance its calls: if the counter climbs above
        // one the suspended region, structure and layout would never be applied again. (dmex)
        if (Context->EnableRedraw < 1)
            Context->EnableRedraw++;
        else
            return; // already enabled: do not replay deferred work
    }
    else
    {
        if (Context->EnableRedraw == MINLONG)
        {
            assert(FALSE); // unbalanced redraw suspension would underflow
            return;
        }
        Context->EnableRedraw--;
    }

    if (Context->EnableRedraw == 1)
    {
        if (Context->SuspendUpdateStructure)
        {
            PhTnpRestructureNodes(Context);
        }

        if (Context->SuspendUpdateLayout)
        {
            PhTnpLayout(Context);
        }

        if (Context->SuspendUpdateMoveMouse)
        {
            POINT point;

            if (PhGetClientPos(Context->Handle, &point))
            {
                PhTnpProcessMoveMouse(Context, point.x, point.y);
            }
        }

        Context->SuspendUpdateStructure = FALSE;
        Context->SuspendUpdateLayout = FALSE;
        Context->SuspendUpdateMoveMouse = FALSE;

        if (Context->PendingFullInvalidate)
        {
            Context->PendingFullInvalidate = FALSE;
            InvalidateRect(Context->Handle, NULL, FALSE);
        }
        Context->DeferredDamageCount = 0;

        if (Context->SuspendUpdateRegion)
        {
            InvalidateRgn(Context->Handle, Context->SuspendUpdateRegion, FALSE);
            DeleteRgn(Context->SuspendUpdateRegion);
            Context->SuspendUpdateRegion = NULL;
        }
    }
}

/**
 * Sends a mouse event notification to the callback function.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Message The mouse event message type.
 * \param CursorX The X coordinate of the cursor.
 * \param CursorY The Y coordinate of the cursor.
 * \param Node Pointer to the node under the cursor, if any.
 * \param Column Pointer to the column under the cursor, if any.
 * \param VirtualKeys The state of virtual keys.
 */
VOID PhTnpSendMouseEvent(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ LONG CursorX,
    _In_ LONG CursorY,
    _In_opt_ PPH_TREENEW_NODE Node,
    _In_opt_ PPH_TREENEW_COLUMN Column,
    _In_ ULONG VirtualKeys
    )
{
    PH_TREENEW_MOUSE_EVENT mouseEvent;

    mouseEvent.Location.x = CursorX;
    mouseEvent.Location.y = CursorY;
    mouseEvent.Node = Node;
    mouseEvent.Column = Column;
    mouseEvent.KeyFlags = VirtualKeys;
    Context->Callback(Context->Handle, Message, &mouseEvent, NULL, Context->CallbackContext);
}

/**
 * Looks up a column by its unique identifier.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Id The column identifier to search for.
 * \return Pointer to the column structure if found, NULL otherwise.
 */
PPH_TREENEW_COLUMN PhTnpLookupColumnById(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Id
    )
{
    if (Id >= Context->AllocatedColumns)
        return NULL;

    return Context->Columns[Id];
}

/**
 * Adds a new column to the treenew control.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Column Pointer to the column structure to add.
 * \return TRUE if the column was added successfully, FALSE otherwise.
 */
BOOLEAN PhTnpAddColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_COLUMN Column
    )
{
    PPH_TREENEW_COLUMN realColumn;

    // Reject ULONG_MAX, otherwise Id + 1 wraps to zero below and the column
    // array is indexed without being expanded.
    if (Column->Id == ULONG_MAX || Column->Width < 0)
        return FALSE;

    // Check if a column with the same ID already exists.
    if (Column->Id < Context->AllocatedColumns && Context->Columns[Column->Id])
        return FALSE;

    if (Context->NextId < Column->Id + 1)
        Context->NextId = Column->Id + 1;

    realColumn = PhAllocateCopy(Column, sizeof(PH_TREENEW_COLUMN));

    if (realColumn->DpiScaleOnAdd)
    {
        if (WindowsVersion >= WINDOWS_10)
        {
            realColumn->Width = PhScaleToDisplay(realColumn->Width, Context->WindowDpi);
        }
        realColumn->DpiScaleOnAdd = FALSE;
    }

    if (Context->AllocatedColumns < Context->NextId)
    {
        PhTnpExpandAllocatedColumns(Context);
    }

    Context->Columns[Column->Id] = realColumn;
    Context->NumberOfColumns++;

    if (realColumn->Fixed)
    {
        if (Context->FixedColumn)
        {
            // We already have a fixed column, and we can't have two. Make this new column un-fixed.
            realColumn->Fixed = FALSE;
        }
        else
        {
            Context->FixedColumn = realColumn;
        }

        realColumn->DisplayIndex = 0;
        realColumn->s.ViewX = 0;
    }

    if (realColumn->Visible)
    {
        BOOLEAN updateHeaders;

        updateHeaders = FALSE;

        assert(Context->NumberOfColumnsByDisplay == (ULONG)Header_GetItemCount(Context->HeaderHandle));
        //if (!realColumn->Fixed && realColumn->DisplayIndex != Header_GetItemCount(Context->HeaderHandle))
        if (!realColumn->Fixed && realColumn->DisplayIndex != Context->NumberOfColumnsByDisplay)
            updateHeaders = TRUE;

        realColumn->s.ViewIndex = PhTnpInsertColumnHeader(Context, realColumn);

        if (realColumn->s.ViewIndex == INT_ERROR)
        {
            // The header control rejected the column. Roll back the addition.
            if (Context->FixedColumn == realColumn)
            {
                Context->FixedColumn = NULL;
                Context->FixedColumnVisible = FALSE;
                Context->FixedDividerVisible = FALSE;
                Context->FixedWidth = 0;
                Context->NormalLeft = 0;
            }

            Context->Columns[Column->Id] = NULL;
            Context->NumberOfColumns--;
            PhFree(realColumn);
            return FALSE;
        }

        if (updateHeaders)
            PhTnpUpdateColumnHeaders(Context);
    }
    else
    {
        realColumn->s.ViewIndex = -1;
    }

    PhTnpUpdateColumnMaps(Context);

    if (realColumn->Visible)
        PhTnpLayout(Context);

    // Column mutation is self-invalidating; callers need not issue a second
    // repaint. The shared helper coalesces this while redraw is suspended.
    PhpTnpInvalidateContent(Context);

    return TRUE;
}

/**
 * Removes a column from the tree view control by its ID.
 *
 * This function deletes the specified column, updates the layout if necessary,
 * and frees associated resources. The column is removed from the internal columns array,
 * and the column maps are updated accordingly.
 *
 * \param Context Pointer to the tree-new context.
 * \param Id The ID of the column to remove.
 * \return TRUE if the column was removed; FALSE if the column was not found.
 */
BOOLEAN PhTnpRemoveColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Id
    )
{
    PPH_TREENEW_COLUMN realColumn;
    BOOLEAN updateLayout;

    if (!(realColumn = PhTnpLookupColumnById(Context, Id)))
        return FALSE;

    updateLayout = FALSE;

    if (realColumn->Visible)
        updateLayout = TRUE;

    PhTnpDeleteColumnHeader(Context, realColumn);
    Context->Columns[realColumn->Id] = NULL;
    PhFree(realColumn);
    PhTnpUpdateColumnMaps(Context);

    if (updateLayout)
        PhTnpLayout(Context);

    Context->NumberOfColumns--;

    // Column mutation is self-invalidating; callers need not issue a second
    // repaint. The shared helper coalesces this while redraw is suspended.
    PhpTnpInvalidateContent(Context);

    return TRUE;
}

/**
 * Copies the data of a column by its ID into a provided column structure.
 *
 * This function looks up the column by ID and copies its data into the output structure.
 *
 * \param Context Pointer to the tree-new context.
 * \param Id The ID of the column to copy.
 * \param Column Pointer to a PH_TREENEW_COLUMN structure to receive the data.
 * \return TRUE if the column was found and copied; FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhTnpCopyColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Id,
    _Out_ PPH_TREENEW_COLUMN Column
    )
{
    PPH_TREENEW_COLUMN realColumn;

    if (!(realColumn = PhTnpLookupColumnById(Context, Id)))
        return FALSE;

    memcpy(Column, realColumn, sizeof(PH_TREENEW_COLUMN));

    return TRUE;
}

/**
 * Changes the properties of a column by its ID and a mask.
 *
 * This function updates the specified properties of a column, such as visibility, width,
 * alignment, display index, and other attributes, according to the provided mask and column data.
 * It handles layout and header updates as needed.
 *
 * \param Context Pointer to the tree-new context.
 * \param Mask Bitmask specifying which properties to update.
 * \param Id The ID of the column to change.
 * \param Column Pointer to a PH_TREENEW_COLUMN structure containing new values.
 * \return TRUE if the column was found and updated; FALSE otherwise.
 */
BOOLEAN PhTnpChangeColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Mask,
    _In_ ULONG Id,
    _In_ PPH_TREENEW_COLUMN Column
    )
{
    PPH_TREENEW_COLUMN realColumn;
    BOOLEAN addingOrRemoving;

    if (!(realColumn = PhTnpLookupColumnById(Context, Id)))
        return FALSE;

    addingOrRemoving = FALSE;

    if (Mask & TN_COLUMN_FLAG_VISIBLE)
    {
        if (realColumn->Visible != Column->Visible)
        {
            addingOrRemoving = TRUE;
        }
    }

    if (Mask & TN_COLUMN_FLAG_CUSTOMDRAW)
    {
        realColumn->CustomDraw = Column->CustomDraw;
    }

    if (Mask & TN_COLUMN_FLAG_SORTDESCENDING)
    {
        realColumn->SortDescending = Column->SortDescending;
    }

    if (Mask & (TN_COLUMN_TEXT | TN_COLUMN_WIDTH | TN_COLUMN_ALIGNMENT | TN_COLUMN_DISPLAYINDEX))
    {
        BOOLEAN updateHeaders;
        BOOLEAN updateMaps;
        BOOLEAN updateLayout;

        updateHeaders = FALSE;
        updateMaps = FALSE;
        updateLayout = FALSE;

        if (Mask & TN_COLUMN_TEXT)
        {
            realColumn->Text = Column->Text;
        }

        if (Mask & TN_COLUMN_WIDTH)
        {
            realColumn->Width = Column->Width;
            updateMaps = TRUE;
        }

        if (Mask & TN_COLUMN_ALIGNMENT)
        {
            realColumn->Alignment = Column->Alignment;
        }

        if (Mask & TN_COLUMN_DISPLAYINDEX)
        {
            realColumn->DisplayIndex = Column->DisplayIndex;
            updateHeaders = TRUE;
            updateMaps = TRUE;
            updateLayout = TRUE;
        }

        if (!addingOrRemoving && realColumn->Visible)
        {
            PhTnpChangeColumnHeader(Context, Mask, realColumn);

            if (updateHeaders)
                PhTnpUpdateColumnHeaders(Context);
            if (updateMaps)
                PhTnpUpdateColumnMaps(Context);
            if (updateLayout)
                PhTnpLayout(Context);
        }
    }

    if (Mask & TN_COLUMN_CONTEXT)
    {
        realColumn->Context = Column->Context;
    }

    if (Mask & TN_COLUMN_TEXTFLAGS)
    {
        realColumn->TextFlags = Column->TextFlags;
    }

    if (addingOrRemoving)
    {
        if (Column->Visible)
        {
            BOOLEAN updateHeaders;

            updateHeaders = FALSE;

            if (realColumn->Fixed)
            {
                realColumn->DisplayIndex = 0;
            }
            else
            {
                if (Mask & TN_COLUMN_DISPLAYINDEX)
                    updateHeaders = TRUE;
                else
                    realColumn->DisplayIndex = Header_GetItemCount(Context->HeaderHandle);
            }

            realColumn->s.ViewIndex = PhTnpInsertColumnHeader(Context, realColumn);

            if (updateHeaders)
                PhTnpUpdateColumnHeaders(Context);
        }
        else
        {
            PhTnpDeleteColumnHeader(Context, realColumn);
        }

        PhTnpUpdateColumnMaps(Context);
        PhTnpLayout(Context);
    }

    // Column mutation is self-invalidating; callers need not issue a second
    // repaint. The shared helper coalesces this while redraw is suspended.
    PhpTnpInvalidateContent(Context);

    return TRUE;
}

/**
 * Expands the allocated columns array for the tree view control.
 *
 * This function doubles the size of the columns array when more space is needed,
 * or initializes it if not already allocated. It ensures that the array is large enough
 * to accommodate all columns, and zeroes the newly allocated memory.
 *
 * \param Context Pointer to the tree-new context.
 */
VOID PhTnpExpandAllocatedColumns(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    if (Context->Columns)
    {
        ULONG oldAllocatedColumns;

        oldAllocatedColumns = Context->AllocatedColumns;
        Context->AllocatedColumns *= 2;

        if (Context->AllocatedColumns < Context->NextId)
            Context->AllocatedColumns = Context->NextId;

        Context->Columns = PhReAllocate(
            Context->Columns,
            Context->AllocatedColumns * sizeof(PPH_TREENEW_COLUMN)
            );

        // Zero the newly allocated portion.
        memset(
            &Context->Columns[oldAllocatedColumns],
            0,
            (Context->AllocatedColumns - oldAllocatedColumns) * sizeof(PPH_TREENEW_COLUMN)
            );
    }
    else
    {
        Context->AllocatedColumns = 16;

        if (Context->AllocatedColumns < Context->NextId)
            Context->AllocatedColumns = Context->NextId;

        Context->Columns = PhAllocate(
            Context->AllocatedColumns * sizeof(PPH_TREENEW_COLUMN)
            );
        memset(Context->Columns, 0, Context->AllocatedColumns * sizeof(PPH_TREENEW_COLUMN));
    }
}

/**
 * Updates the internal column mapping arrays.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpUpdateColumnMaps(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    ULONG i;
    LONG x;

    if (Context->AllocatedColumnsByDisplay < Context->NumberOfColumns)
    {
        if (Context->ColumnsByDisplay)
            PhFree(Context->ColumnsByDisplay);

        Context->ColumnsByDisplay = PhAllocate(sizeof(PPH_TREENEW_COLUMN) * Context->NumberOfColumns);
        Context->AllocatedColumnsByDisplay = Context->NumberOfColumns;
    }

    memset(Context->ColumnsByDisplay, 0, sizeof(PPH_TREENEW_COLUMN) * Context->AllocatedColumnsByDisplay);

    for (i = 0; i < Context->NextId; i++)
    {
        if (!Context->Columns[i])
            continue;

        if (Context->Columns[i]->Visible && !Context->Columns[i]->Fixed && Context->Columns[i]->DisplayIndex != ULONG_MAX)
        {
            if (Context->Columns[i]->DisplayIndex >= Context->NumberOfColumns)
            {
                PhRaiseStatus(STATUS_INTERNAL_ERROR);
                return;
            }

            Context->ColumnsByDisplay[Context->Columns[i]->DisplayIndex] = Context->Columns[i];
        }
    }

    x = 0;

    for (i = 0; i < Context->AllocatedColumnsByDisplay; i++)
    {
        if (!Context->ColumnsByDisplay[i])
            break;

        Context->ColumnsByDisplay[i]->s.ViewX = x;
        x += Context->ColumnsByDisplay[i]->Width;
    }

    Context->NumberOfColumnsByDisplay = i;
    Context->TotalViewX = x;

    if (Context->FixedColumnVisible)
        Context->FirstColumn = Context->FixedColumn;
    else if (Context->NumberOfColumnsByDisplay != 0)
        Context->FirstColumn = Context->ColumnsByDisplay[0];
    else
        Context->FirstColumn = NULL;

    if (Context->NumberOfColumnsByDisplay != 0)
        Context->LastColumn = Context->ColumnsByDisplay[Context->NumberOfColumnsByDisplay - 1];
    else if (Context->FixedColumnVisible)
        Context->LastColumn = Context->FixedColumn;
    else
        Context->LastColumn = NULL;
}

/**
 * Inserts a column header into the appropriate header control.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Column Pointer to the column structure.
 * \return The index of the inserted header item, or -1 on failure.
 */
LONG PhTnpInsertColumnHeader(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_COLUMN Column
    )
{
    HDITEM item;

    if (Column->Fixed)
    {
        if (Column->Width < Context->FixedWidthMinimum)
            Column->Width = Context->FixedWidthMinimum;

        Context->FixedWidth = Column->Width;
        Context->NormalLeft = Context->FixedWidth + 1;
        Context->FixedColumnVisible = TRUE;

        if (!(Context->Style & TN_STYLE_NO_DIVIDER))
            Context->FixedDividerVisible = TRUE;
    }

    memset(&item, 0, sizeof(HDITEM));
    item.mask = HDI_WIDTH | HDI_TEXT | HDI_FORMAT | HDI_LPARAM | HDI_ORDER;
    item.cxy = Column->Width;
    item.pszText = (PWSTR)Column->Text;
    item.fmt = 0;
    item.lParam = (LPARAM)Column;

    if (Column->Fixed)
        item.cxy++;

    if (Column->Fixed)
        item.iOrder = 0;
    else
        item.iOrder = Column->DisplayIndex;

    if (Column->Alignment & PH_ALIGN_LEFT)
        item.fmt |= HDF_LEFT;
    else if (Column->Alignment & PH_ALIGN_RIGHT)
        item.fmt |= HDF_RIGHT;
    else
        item.fmt |= HDF_CENTER;

    if (Column->Id == Context->SortColumn)
    {
        if (Context->SortOrder == AscendingSortOrder)
            item.fmt |= HDF_SORTUP;
        else if (Context->SortOrder == DescendingSortOrder)
            item.fmt |= HDF_SORTDOWN;
    }

    Column->Visible = TRUE;

    if (Column->Fixed)
        return Header_InsertItem(Context->FixedHeaderHandle, 0, &item);
    else
        return Header_InsertItem(Context->HeaderHandle, MAXINT, &item);
}

/**
 * Changes the properties of a column header.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Mask Bitmask specifying which properties to update.
 * \param Column Pointer to the column structure.
 */
VOID PhTnpChangeColumnHeader(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Mask,
    _In_ PPH_TREENEW_COLUMN Column
    )
{
    HDITEM item;

    memset(&item, 0, sizeof(HDITEM));
    item.mask = 0;

    if (Mask & TN_COLUMN_TEXT)
    {
        item.mask |= HDI_TEXT;
        item.pszText = (PWSTR)Column->Text;
    }

    if (Mask & TN_COLUMN_WIDTH)
    {
        item.mask |= HDI_WIDTH;
        item.cxy = Column->Width;

        if (Column->Fixed)
            item.cxy++;
    }

    if (Mask & TN_COLUMN_ALIGNMENT)
    {
        item.mask |= HDI_FORMAT;
        item.fmt = 0;

        if (Column->Alignment & PH_ALIGN_LEFT)
            item.fmt |= HDF_LEFT;
        else if (Column->Alignment & PH_ALIGN_RIGHT)
            item.fmt |= HDF_RIGHT;
        else
            item.fmt |= HDF_CENTER;

        if (Column->Id == Context->SortColumn)
        {
            if (Context->SortOrder == AscendingSortOrder)
                item.fmt |= HDF_SORTUP;
            else if (Context->SortOrder == DescendingSortOrder)
                item.fmt |= HDF_SORTDOWN;
        }
    }

    if (Mask & TN_COLUMN_DISPLAYINDEX)
    {
        item.mask |= HDI_ORDER;

        if (Column->Fixed)
            item.iOrder = 0;
        else
            item.iOrder = Column->DisplayIndex;
    }

    if (Column->Fixed)
        Header_SetItem(Context->FixedHeaderHandle, 0, &item);
    else
        Header_SetItem(Context->HeaderHandle, Column->s.ViewIndex, &item);
}

/**
 * Deletes a column header from the header control.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Column Pointer to the column structure.
 */
VOID PhTnpDeleteColumnHeader(
    _In_ PPH_TREENEW_CONTEXT Context,
    _Inout_ PPH_TREENEW_COLUMN Column
    )
{
    if (Column->Fixed)
    {
        Context->FixedColumn = NULL;
        Context->FixedWidth = 0;
        Context->NormalLeft = 0;
        Context->FixedColumnVisible = FALSE;
        Context->FixedDividerVisible = FALSE;
    }

    if (Column->Fixed)
        Header_DeleteItem(Context->FixedHeaderHandle, Column->s.ViewIndex);
    else
        Header_DeleteItem(Context->HeaderHandle, Column->s.ViewIndex);

    Column->Visible = FALSE;
    Column->s.ViewIndex = -1;
    PhTnpUpdateColumnHeaders(Context);
}

/**
 * Updates all column headers to reflect current column states.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpUpdateColumnHeaders(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    LONG count;
    LONG i;
    HDITEM item;
    PPH_TREENEW_COLUMN column;

    item.mask = HDI_WIDTH | HDI_LPARAM | HDI_ORDER;

    // Fixed column

    if (Context->FixedColumnVisible && Header_GetItem(Context->FixedHeaderHandle, 0, &item))
    {
        column = Context->FixedColumn;
        column->Width = item.cxy - 1;
    }

    // Normal columns

    count = Header_GetItemCount(Context->HeaderHandle);

    if (count != INT_ERROR)
    {
        for (i = 0; i < count; i++)
        {
            if (Header_GetItem(Context->HeaderHandle, i, &item))
            {
                if (!(column = (PPH_TREENEW_COLUMN)item.lParam))
                    continue; // the item doesn't belong to us

                column->s.ViewIndex = i;
                column->Width = item.cxy;
                column->DisplayIndex = item.iOrder;
            }
        }
    }

    if (Context->HeaderCustomDraw)
        Context->HeaderInvalidatePending = TRUE;
}

/**
 * Updates the stored width of a single column and the view offsets that depend on it.
 *
 * This is the incremental form of PhTnpUpdateColumnHeaders + PhTnpUpdateColumnMaps for the common
 * case of a column resize: only the widths to the right of the column move, so the header does not
 * have to be enumerated and the display map does not have to be rebuilt. (dmex)
 *
 * \param Context Pointer to the treenew context structure.
 * \param Column Pointer to the column that changed.
 * \param Width The new width of the header item.
 */
VOID PhTnpUpdateColumnWidth(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ LONG Width
    )
{
    ULONG i;
    LONG x;

    if (Column->Fixed)
    {
        // The fixed header item is one pixel wider than the column itself.
        Column->Width = Width - 1;
    }
    else if (
        Column->DisplayIndex < Context->NumberOfColumnsByDisplay &&
        Context->ColumnsByDisplay[Column->DisplayIndex] == Column
        )
    {
        Column->Width = Width;

        // Columns to the left keep their offsets; recompute from this one rightwards.

        x = Column->s.ViewX;

        for (i = Column->DisplayIndex; i < Context->NumberOfColumnsByDisplay; i++)
        {
            Context->ColumnsByDisplay[i]->s.ViewX = x;
            x += Context->ColumnsByDisplay[i]->Width;
        }

        Context->TotalViewX = x;
    }
    else
    {
        // The display map doesn't agree with the column; fall back to a full rebuild.
        Column->Width = Width;
        PhTnpUpdateColumnMaps(Context);
    }

    if (Context->HeaderCustomDraw)
        Context->HeaderInvalidatePending = TRUE;
}

/**
 * Updates column headers after a DPI change.
 *
 * \param Context Pointer to the treenew context structure.
 * \param OldWindowDpi The previous DPI value.
 * \param NewWindowDpi The new DPI value.
 */
VOID PhTnpUpdateColumnHeadersDpiChanged(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG OldWindowDpi,
    _In_ LONG NewWindowDpi
    )
{
    LONG count;
    LONG i;
    HDITEM item;
    PPH_TREENEW_COLUMN column;

    item.mask = HDI_WIDTH | HDI_LPARAM;

    // Fixed column

    if (Context->FixedColumnVisible && Header_GetItem(Context->FixedHeaderHandle, 0, &item))
    {
        column = Context->FixedColumn;
        column->Width = PhMultiplyDivideSigned(item.cxy, NewWindowDpi, OldWindowDpi);

        PhTnpChangeColumn(Context, TN_COLUMN_WIDTH, column->Id, column);
    }

    // Normal columns

    count = Header_GetItemCount(Context->HeaderHandle);

    if (count != INT_ERROR)
    {
        for (i = 0; i < count; i++)
        {
            if (Header_GetItem(Context->HeaderHandle, i, &item))
            {
                if (!(column = (PPH_TREENEW_COLUMN)item.lParam))
                    continue; // the item doesn't belong to us

                column->Width = PhMultiplyDivideSigned(item.cxy, NewWindowDpi, OldWindowDpi);

                PhTnpChangeColumn(Context, TN_COLUMN_WIDTH, column->Id, column);
            }
        }
    }
}

/**
 * Processes a column resize operation.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Column Pointer to the column being resized.
 * \param Delta The change in width.
 */
VOID PhTnpProcessResizeColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ LONG Delta
    )
{
    RECT contentRect;
    RECT rect;
    LONG columnLeft;
    LONG oldColumnWidth;

    if (Column->Fixed)
    {
        columnLeft = 0;
    }
    else
    {
        columnLeft = Context->NormalLeft + Column->s.ViewX - Context->HScrollPosition;
    }

    // Scroll the content to the right of the column.
    //
    // Clip the scroll area to the new width, or the old width if that is further to the left. We
    // may have the WS_CLIPCHILDREN style set, so we need to remove the horizontal scrollbar from
    // the rectangle, otherwise ScrollWindowEx will want to invalidate the entire region! (The
    // horizontal scrollbar is an overlapping child control.)
    rect.left = columnLeft + Column->Width;
    rect.top = Context->HeaderHeight;
    rect.right = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);
    rect.bottom = Context->ClientRect.bottom - (Context->HScrollVisible ? Context->HScrollHeight : 0);

    if (Delta > 0)
        rect.left -= Delta; // old width

    // Scroll the window.
    ScrollWindowEx(
        Context->Handle,
        Delta,
        0,
        &rect,
        &rect,
        NULL,
        NULL,
        SW_INVALIDATE
        );

    PhTnpLayout(Context);

    // Redraw the whole column because the content may depend on the width (e.g. text ellipsis).
    oldColumnWidth = Column->Width - Delta;
    rect.left = columnLeft;
    rect.right = columnLeft + max(Column->Width, oldColumnWidth);

    if (PhpTnpGetContentRect(Context, &contentRect) &&
        PhIntersectRect(&rect, &rect, &contentRect))
    {
        PhpTnpInvalidateRect(Context, &rect);
    }
}

/**
 * Processes a column sort operation.
 *
 * \param Context Pointer to the treenew context structure.
 * \param NewColumn Pointer to the column being sorted.
 */
VOID PhTnpProcessSortColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_COLUMN NewColumn
    )
{
    PH_TREENEW_SORT_CHANGED_EVENT sortOrderEvent;

    if (NewColumn->Id == Context->SortColumn)
    {
        if (Context->TriState)
        {
            if (!NewColumn->SortDescending)
            {
                // Ascending -> Descending -> None

                if (Context->SortOrder == AscendingSortOrder)
                    Context->SortOrder = DescendingSortOrder;
                else if (Context->SortOrder == DescendingSortOrder)
                    Context->SortOrder = NoSortOrder;
                else
                    Context->SortOrder = AscendingSortOrder;
            }
            else
            {
                // Descending -> Ascending -> None

                if (Context->SortOrder == DescendingSortOrder)
                    Context->SortOrder = AscendingSortOrder;
                else if (Context->SortOrder == AscendingSortOrder)
                    Context->SortOrder = NoSortOrder;
                else
                    Context->SortOrder = DescendingSortOrder;
            }
        }
        else
        {
            if (Context->SortOrder == AscendingSortOrder)
                Context->SortOrder = DescendingSortOrder;
            else
                Context->SortOrder = AscendingSortOrder;
        }
    }
    else
    {
        Context->SortColumn = NewColumn->Id;

        if (!NewColumn->SortDescending)
            Context->SortOrder = AscendingSortOrder;
        else
            Context->SortOrder = DescendingSortOrder;
    }

    PhTnpSetColumnHeaderSortIcon(Context, NewColumn);

    memset(&sortOrderEvent, 0, sizeof(PH_TREENEW_SORT_CHANGED_EVENT));
    sortOrderEvent.SortColumn = Context->SortColumn;
    sortOrderEvent.SortOrder = Context->SortOrder;

    Context->Callback(Context->Handle, TreeNewSortChanged, &sortOrderEvent, NULL, Context->CallbackContext);
}

/**
 * Sets the sort icon on a column header.
 *
 * \param Context Pointer to the treenew context structure.
 * \param SortColumnPointer Pointer to the column being sorted, or NULL to look it up.
 * \return TRUE if the icon was set successfully, FALSE otherwise.
 */
BOOLEAN PhTnpSetColumnHeaderSortIcon(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ PPH_TREENEW_COLUMN SortColumnPointer
    )
{
    if (Context->SortOrder == NoSortOrder)
    {
        PhSetHeaderSortIcon(
            Context->FixedHeaderHandle,
            -1,
            NoSortOrder
            );
        PhSetHeaderSortIcon(
            Context->HeaderHandle,
            -1,
            NoSortOrder
            );

        return TRUE;
    }

    if (!SortColumnPointer)
    {
        if (!(SortColumnPointer = PhTnpLookupColumnById(Context, Context->SortColumn)))
            return FALSE;
    }

    if (SortColumnPointer->Fixed)
    {
        PhSetHeaderSortIcon(
            Context->FixedHeaderHandle,
            0,
            Context->SortOrder
            );
        PhSetHeaderSortIcon(
            Context->HeaderHandle,
            -1,
            NoSortOrder
            );
    }
    else
    {
        PhSetHeaderSortIcon(
            Context->FixedHeaderHandle,
            -1,
            NoSortOrder
            );
        PhSetHeaderSortIcon(
            Context->HeaderHandle,
            SortColumnPointer->s.ViewIndex,
            Context->SortOrder
            );
    }

    return TRUE;
}

/**
 * Automatically sizes a column header to fit its content.
 *
 * \param Context Pointer to the treenew context structure.
 * \param HeaderHandle Handle to the header control.
 * \param Column Pointer to the column to auto-size.
 * \param Flags Flags controlling the auto-size behavior.
 */
VOID PhTnpAutoSizeColumnHeader(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HWND HeaderHandle,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ ULONG Flags
    )
{
    LONG newWidth;
    HDITEM item;

    if (Flags & TN_AUTOSIZE_REMAINING_SPACE)
    {
        newWidth = Context->ClientRect.right - (Context->TotalViewX - Column->Width);

        if (Context->FixedColumn)
            newWidth -= Context->FixedColumn->Width;
        if (Context->VScrollVisible)
            newWidth -= Context->VScrollWidth;

        if (newWidth <= 0)
            return;
    }
    else
    {
        ULONG i;
        LONG maximumWidth;
        PH_TREENEW_CELL_PARTS parts;
        LONG width;
        HDC measureDc;
        ULONG first = 0;
        ULONG count;
        ULONG samples;

        if (Context->FlatList->Count == 0)
            return;
        if (Column->CustomDraw)
            return;

        if (!(measureDc = GetDC(Context->Handle)))
            return;

        count = Context->FlatList->Count;
        if (Flags & TN_AUTOSIZE_VISIBLE_ROWS)
        {
            RECT contentRect;
            first = min((ULONG)max(Context->VScrollPosition, 0), count);
            count = PhpTnpGetContentRect(Context, &contentRect) ?
                min(count - first, (ULONG)((contentRect.bottom - contentRect.top + Context->RowHeight - 1) / Context->RowHeight)) : 0;
        }
        samples = Flags & TN_AUTOSIZE_SAMPLED_ROWS ? min(count, 256u) : count;
        maximumWidth = 0;

        for (i = 0; i < samples; i++)
        {
            ULONG index = first + (ULONG)((ULONG64)i * count / samples);
            if (PhTnpGetCellPartsWithDc(Context, index, Column, TN_MEASURE_TEXT, &parts, measureDc) &&
                (parts.Flags & TN_PART_CELL) && (parts.Flags & TN_PART_CONTENT) && (parts.Flags & TN_PART_TEXT))
            {
                width = parts.TextRect.right - parts.TextRect.left; // text width
                width += parts.ContentRect.left - parts.CellRect.left; // left padding

                if (maximumWidth < width)
                    maximumWidth = width;
            }
        }

        ReleaseDC(Context->Handle, measureDc);

        newWidth = maximumWidth + Context->CellMarginRight; // right padding

        if (Column->Fixed)
            newWidth++;

        // Check the column header text width.
        if (Column->Text)
        {
            PCWSTR text;
            SIZE_T textCount;
            HDC hdc;
            SIZE textSize;

            text = Column->Text;
            textCount = PhCountStringZ(text);

            if (hdc = GetDC(Context->Handle))
            {
                SelectFont(hdc, Context->Font);

                if (GetTextExtentPoint32(hdc, text, (ULONG)textCount, &textSize))
                {
                    if (newWidth < textSize.cx + Context->TextMarginPadding) // HACK: Magic values (same as our cell margins?)
                        newWidth = textSize.cx + Context->TextMarginPadding;
                }

                ReleaseDC(Context->Handle, hdc);
            }
        }

        // Check the custom header text width. (dmex)
        if (Context->HeaderCustomDraw)
        {
            PH_STRINGREF headerString;

            if (PhTnpGetColumnHeaderText(
                Context,
                Column,
                &headerString
                ))
            {
                HDC hdc;
                SIZE textSize;

                if (hdc = GetDC(Context->Handle))
                {
                    SelectFont(hdc, Context->HeaderBoldFontHandle);

                    if (GetTextExtentPoint32(hdc, headerString.Buffer, (ULONG)headerString.Length / sizeof(WCHAR), &textSize))
                    {
                        if (newWidth < textSize.cx + Context->TextMarginPadding) // HACK: Magic values (same as our cell margins?)
                            newWidth = textSize.cx + Context->TextMarginPadding;
                    }

                    ReleaseDC(Context->Handle, hdc);
                }
            }
        }
    }

    item.mask = HDI_WIDTH;
    item.cxy = newWidth;

    Header_SetItem(HeaderHandle, Column->s.ViewIndex, &item);
}

/**
 * Gets the list of child nodes for a given node.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Node Pointer to the parent node, or NULL for root nodes.
 * \param Children Pointer to receive the array of child nodes.
 * \param NumberOfChildren Pointer to receive the number of children.
 * \return TRUE if the operation succeeded, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhTnpGetNodeChildren(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ PPH_TREENEW_NODE Node,
    _Out_ PPH_TREENEW_NODE **Children,
    _Out_ PULONG NumberOfChildren
    )
{
    PH_TREENEW_GET_CHILDREN getChildren;

    *Children = NULL;
    *NumberOfChildren = 0;

    getChildren.Flags = 0;
    getChildren.Node = Node;
    getChildren.Children = NULL;
    getChildren.NumberOfChildren = 0;

    if (Context->Callback(
        Context->Handle,
        TreeNewGetChildren,
        &getChildren,
        NULL,
        Context->CallbackContext
        ))
    {
        if (getChildren.NumberOfChildren && !getChildren.Children)
            return FALSE;
        *Children = getChildren.Children;
        *NumberOfChildren = getChildren.NumberOfChildren;

        return TRUE;
    }

    return FALSE;
}

/**
 * Determines whether a node is a leaf node (has no children).
 *
 * \param Context Pointer to the treenew context structure.
 * \param Node Pointer to the node to check.
 * \return TRUE if the node is a leaf, FALSE otherwise.
 */
BOOLEAN PhTnpIsNodeLeaf(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_NODE Node
    )
{
    PH_TREENEW_IS_LEAF isLeaf;

    isLeaf.Flags = 0;
    isLeaf.Node = Node;
    isLeaf.IsLeaf = TRUE;

    if (Context->Callback(
        Context->Handle,
        TreeNewIsLeaf,
        &isLeaf,
        NULL,
        Context->CallbackContext
        ))
    {
        return isLeaf.IsLeaf;
    }

    // Doesn't matter, decide when we do the get-children callback.
    return FALSE;
}

/**
 * Retrieves the text for a specific cell.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Node Pointer to the node.
 * \param Id The column ID.
 * \param Text Pointer to receive the cell text as a string reference.
 * \return TRUE if text was retrieved, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhTnpGetCellText(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_NODE Node,
    _In_ ULONG Id,
    _Out_ PPH_STRINGREF Text
    )
{
    PH_TREENEW_GET_CELL_TEXT getCellText;

    if (Id < Node->TextCacheSize && Node->TextCache[Id].Buffer)
    {
        *Text = Node->TextCache[Id];
        return TRUE;
    }

    getCellText.Flags = 0;
    getCellText.Node = Node;
    getCellText.Id = Id;
    PhInitializeEmptyStringRef(&getCellText.Text);

    if (Context->Callback(
        Context->Handle,
        TreeNewGetCellText,
        &getCellText,
        NULL,
        Context->CallbackContext
        ) && getCellText.Text.Buffer)
    {
        *Text = getCellText.Text;

        if ((getCellText.Flags & TN_CACHE) && Id < Node->TextCacheSize)
            Node->TextCache[Id] = getCellText.Text;

        return TRUE;
    }

    return FALSE;
}

/**
 * Restructures the flat list of visible nodes based on the tree hierarchy.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpRestructureNodes(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    PPH_TREENEW_NODE *children;
    ULONG numberOfChildren;
    ULONG i;

    if (!PhTnpGetNodeChildren(Context, NULL, &children, &numberOfChildren))
        return;

    // We try to preserve the hot node, the focused node and the selection mark node. At this point
    // all node pointers must be regarded as invalid, so we must not follow any pointers.

    Context->FocusNodeFound = FALSE;

#if defined(TREENEW_VSCROLL_ANCHOR)
    Context->FlatListPreCount = Context->FlatList->Count;
#endif

    PhClearList(Context->FlatList);
    Context->CanAnyExpand = FALSE;

#if defined(TREENEW_VSCROLL_ANCHOR)
    Context->FlatListStructureChanged = TRUE;
#endif

    for (i = 0; i < numberOfChildren; i++)
    {
        PhTnpInsertNodeChildren(Context, children[i], 0);
    }

    if (!Context->FocusNodeFound)
        Context->FocusNode = NULL; // focused node is no longer present

    if (Context->HotNodeIndex >= Context->FlatList->Count) // covers -1 case as well
        Context->HotNodeIndex = ULONG_MAX;

    if (Context->MarkNodeIndex >= Context->FlatList->Count)
        Context->MarkNodeIndex = ULONG_MAX;

}

/**
 * Inserts child nodes into the flat list recursively.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Node Pointer to the parent node.
 * \param Level The nesting level of the children.
 */
VOID PhTnpInsertNodeChildren(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_NODE Node,
    _In_ ULONG Level
    )
{
    typedef struct _TN_WALK_FRAME
    {
        PPH_TREENEW_NODE Node;
        PPH_TREENEW_NODE *Children;
        ULONG Count;
        ULONG Next;
        ULONG Level;
        ULONG ChildLevel;
        BOOLEAN Entered;
    } TN_WALK_FRAME;
    TN_WALK_FRAME *stack;
    ULONG depth = 1;
    ULONG capacity = 32;

    if (!Node || !(stack = PhAllocateSafe(capacity * sizeof(TN_WALK_FRAME))))
    {
        Context->PendingFullInvalidate = TRUE;
        return;
    }
    memset(&stack[0], 0, sizeof(TN_WALK_FRAME));
    stack[0].Node = Node;
    stack[0].Level = Level;

    while (depth)
    {
        TN_WALK_FRAME *frame = &stack[depth - 1];

        if (!frame->Entered)
        {
            ULONG i;
            BOOLEAN cycle = FALSE;

            for (i = 0; i + 1 < depth; i++)
            {
                if (stack[i].Node == frame->Node) 
                { 
                    cycle = TRUE; 
                    break; 
                }
            }

            if (cycle) 
            { 
                assert(FALSE); 
                depth--; 
                continue; 
            }

            frame->Entered = TRUE;

            if (frame->Node->Visible)
            {
                frame->Node->Level = frame->Level;
                frame->Node->Index = Context->FlatList->Count;
                PhAddItemList(Context->FlatList, frame->Node);
                if (Context->FocusNode == frame->Node)
                    Context->FocusNodeFound = TRUE;
                frame->ChildLevel = frame->Level + 1;
            }

            frame->Node->s.IsLeaf = PhTnpIsNodeLeaf(Context, frame->Node);

            if (!frame->Node->s.IsLeaf)
            {
                Context->CanAnyExpand = TRUE;

                if (frame->Node->Expanded)
                {
                    PPH_TREENEW_NODE *children;
                    ULONG count;
                    SIZE_T bytes;

                    if (PhTnpGetNodeChildren(Context, frame->Node, &children, &count))
                    {
                        if (!count)
                            frame->Node->s.IsLeaf = TRUE;
                        else if (NT_SUCCESS(RtlSizeTMult(count, sizeof(PPH_TREENEW_NODE), &bytes)) &&
                            (frame->Children = PhAllocateSafe(bytes)))
                        {
                            memcpy(frame->Children, children, bytes);
                            frame->Count = count;
                        }
                        else
                        {
                            Context->PendingFullInvalidate = TRUE;
                            break;
                        }
                    }
                }
            }
        }
        if (frame->Next < frame->Count)
        {
            PPH_TREENEW_NODE child = frame->Children[frame->Next++];
            ULONG childLevel = frame->ChildLevel;

            if (!child) 
            { 
                assert(FALSE); 
                continue; 
            }

            if (depth >= 4096) 
            { 
                assert(FALSE); 
                continue; 
            }

            if (depth == capacity)
            {
                TN_WALK_FRAME *newStack;
                ULONG newCapacity = capacity * 2;

                newStack = PhAllocateSafe(newCapacity * sizeof(TN_WALK_FRAME));

                if (!newStack) 
                {
                    Context->PendingFullInvalidate = TRUE; 
                    break; 
                }

                memcpy(newStack, stack, capacity * sizeof(TN_WALK_FRAME));
                PhFree(stack);

                stack = newStack;
                capacity = newCapacity;
            }

            memset(&stack[depth], 0, sizeof(TN_WALK_FRAME));
            stack[depth].Node = child;
            stack[depth++].Level = childLevel;
        }
        else
        {
            if (frame->Children) PhFree(frame->Children);
            depth--;
        }
    }
    while (depth)
    {
        if (stack[depth - 1].Children) 
        {
            PhFree(stack[depth - 1].Children);
        }

        depth--;
    }
    
    PhFree(stack);
}

/**
 * Sets the expanded state of a node.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Node Pointer to the node.
 * \param Expanded TRUE to expand the node, FALSE to collapse it.
 */
VOID PhTnpSetExpandedNode(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_NODE Node,
    _In_ BOOLEAN Expanded
    )
{
    if (Node->Expanded != Expanded)
    {
        PH_TREENEW_NODE_EVENT nodeEvent;

        memset(&nodeEvent, 0, sizeof(PH_TREENEW_NODE_EVENT));
        Context->Callback(Context->Handle, TreeNewNodeExpanding, Node, &nodeEvent, Context->CallbackContext);

        if (!nodeEvent.Handled)
        {
            if (!Expanded)
            {
                ULONG i;
                PPH_TREENEW_NODE node;
                BOOLEAN changed;

                // Make sure no children are selected - we don't want invisible selected nodes. Note
                // that this does not cause any UI changes by itself, since we are hiding the nodes.

                changed = FALSE;

                for (i = Node->Index + 1; i < Context->FlatList->Count; i++)
                {
                    node = Context->FlatList->Items[i];

                    if (node->Level <= Node->Level)
                        break; // no more children

                    if (node->Selected)
                    {
                        node->Selected = FALSE;
                        changed = TRUE;
                    }
                }

                if (changed)
                {
                    Context->Callback(Context->Handle, TreeNewSelectionChanged, NULL, NULL, Context->CallbackContext);
                }
            }

#if defined(TREENEW_VSCROLL_ANCHOR)
            PhTnpPrepareVScrollAnchor(Context);
            Node->Expanded = Expanded;
            PhTnpRestructureNodes(Context);
            PhTnpLayout(Context);

            if (Node->Visible)
                PhpTnpInvalidateRows(Context, Node->Index, Context->FlatList->Count - 1);

            UpdateWindow(Context->Handle);
#else
            Node->Expanded = Expanded;
            PhTnpRestructureNodes(Context);
            // We need to update the window before the scrollbars get updated in order for the
            // scroll processing to work properly.
            if (Node->Visible)
                PhpTnpInvalidateRows(Context, Node->Index, Context->FlatList->Count - 1);

            UpdateWindow(Context->Handle);
            PhTnpLayout(Context);
#endif
        }
    }
}

/**
 * Calculates the positions and sizes of all parts of a cell.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Index The index of the row.
 * \param Column Pointer to the column, or NULL for the fixed column.
 * \param Flags Flags controlling which parts to calculate.
 * \param Parts Pointer to receive the cell parts information.
 * \return TRUE if successful, FALSE otherwise.
 */
static BOOLEAN PhTnpGetCellPartsWithDc(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Index,
    _In_opt_ PPH_TREENEW_COLUMN Column,
    _In_ ULONG Flags,
    _Out_ PPH_TREENEW_CELL_PARTS Parts,
    _In_opt_ HDC MeasureDc
    )
{
    PPH_TREENEW_NODE node;
    LONG viewWidth;
    LONG nodeY;
    LONG iconVerticalMargin;
    LONG currentX;

    if (Index >= Context->FlatList->Count)
        return FALSE;

    node = Context->FlatList->Items[Index];
    nodeY = Context->HeaderHeight + ((LONG)Index - Context->VScrollPosition) * Context->RowHeight;

    Parts->Flags = 0;
    Parts->RowRect.left = 0;
    Parts->RowRect.right = Context->NormalLeft + Context->TotalViewX - Context->HScrollPosition;
    Parts->RowRect.top = nodeY;
    Parts->RowRect.bottom = nodeY + Context->RowHeight;

    viewWidth = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);

    if (Parts->RowRect.right > viewWidth)
        Parts->RowRect.right = viewWidth;

    if (!Column)
        return TRUE;
    if (!Column->Visible)
        return FALSE;

    iconVerticalMargin = (Context->RowHeight - Context->SmallIconHeight) / 2;

    if (Column->Fixed)
    {
        currentX = 0;
    }
    else
    {
        currentX = Context->NormalLeft + Column->s.ViewX - Context->HScrollPosition;
    }

    Parts->Flags |= TN_PART_CELL;
    Parts->CellRect.left = currentX;
    Parts->CellRect.right = currentX + Column->Width;
    Parts->CellRect.top = Parts->RowRect.top;
    Parts->CellRect.bottom = Parts->RowRect.bottom;

    currentX += Context->CellMarginLeft;

    if (Column == Context->FirstColumn)
    {
        currentX += (LONG)node->Level * Context->SmallIconWidth;

        if (Context->CanAnyExpand)
        {
            if (!node->s.IsLeaf)
            {
                Parts->Flags |= TN_PART_PLUSMINUS;
                Parts->PlusMinusRect.left = currentX;
                Parts->PlusMinusRect.right = currentX + Context->SmallIconWidth;
                Parts->PlusMinusRect.top = Parts->RowRect.top + iconVerticalMargin;
                Parts->PlusMinusRect.bottom = Parts->RowRect.bottom - iconVerticalMargin;
            }

            currentX += Context->SmallIconWidth;
        }

        if (node->Icon)
        {
            Parts->Flags |= TN_PART_ICON;
            Parts->IconRect.left = currentX;
            Parts->IconRect.right = currentX + Context->SmallIconWidth;
            Parts->IconRect.top = Parts->RowRect.top + iconVerticalMargin;
            Parts->IconRect.bottom = Parts->RowRect.bottom - iconVerticalMargin;

            currentX += Context->SmallIconWidth + Context->IconRightPadding;
        }
    }

    Parts->Flags |= TN_PART_CONTENT;
    Parts->ContentRect.left = currentX;
    Parts->ContentRect.right = Parts->CellRect.right - Context->CellMarginRight;
    Parts->ContentRect.top = Parts->RowRect.top;
    Parts->ContentRect.bottom = Parts->RowRect.bottom;

    if (Flags & TN_MEASURE_TEXT)
    {
        HDC hdc;
        HFONT oldFont;
        PH_STRINGREF text;
        HFONT font;
        SIZE textSize;

        if (hdc = MeasureDc ? MeasureDc : GetDC(Context->Handle))
        {
            oldFont = GetCurrentObject(hdc, OBJ_FONT);
            PhTnpPrepareRowForDraw(Context, hdc, node);

            if (PhTnpGetCellText(Context, node, Column->Id, &text))
            {
                if (node->Font)
                    font = node->Font;
                else
                    font = Context->Font;

                SelectFont(hdc, font);

                if (GetTextExtentPoint32(hdc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &textSize))
                {
                    Parts->Flags |= TN_PART_TEXT;
                    Parts->TextRect.left = currentX;
                    Parts->TextRect.right = currentX + textSize.cx;
                    Parts->TextRect.top = Parts->RowRect.top + (Context->RowHeight - textSize.cy) / 2;
                    Parts->TextRect.bottom = Parts->RowRect.bottom - (Context->RowHeight - textSize.cy) / 2;

                    if (Column->TextFlags & DT_CENTER)
                    {
                        Parts->TextRect.left = (Parts->ContentRect.left + Parts->ContentRect.right - textSize.cx) / 2;
                        Parts->TextRect.right = Parts->TextRect.left + textSize.cx;
                    }
                    else if (Column->TextFlags & DT_RIGHT)
                    {
                        Parts->TextRect.right = Parts->ContentRect.right;
                        Parts->TextRect.left = Parts->TextRect.right - textSize.cx;
                    }

                    Parts->Text = text;
                    Parts->Font = font;
                }
            }

            if (oldFont) SelectFont(hdc, oldFont);
            if (!MeasureDc) ReleaseDC(Context->Handle, hdc);
        }
    }

    return TRUE;
}

BOOLEAN PhTnpGetCellParts(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Index,
    _In_opt_ PPH_TREENEW_COLUMN Column,
    _In_ ULONG Flags,
    _Out_ PPH_TREENEW_CELL_PARTS Parts
    )
{
    return PhTnpGetCellPartsWithDc(Context, Index, Column, Flags, Parts, NULL);
}

_Success_(return)
BOOLEAN PhTnpGetRowRects(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Start,
    _In_ ULONG End,
    _In_ BOOLEAN Clip,
    _Out_ PRECT Rect
    )
{
    LONG startY;
    LONG endY;
    LONG viewWidth;

    if (End >= Context->FlatList->Count)
        return FALSE;
    if (Start > End)
        return FALSE;

    startY = Context->HeaderHeight + ((LONG)Start - Context->VScrollPosition) * Context->RowHeight;
    endY = Context->HeaderHeight + ((LONG)End - Context->VScrollPosition) * Context->RowHeight;

    Rect->left = 0;
    Rect->right = Context->NormalLeft + Context->TotalViewX - Context->HScrollPosition;
    Rect->top = startY;
    Rect->bottom = endY + Context->RowHeight;

    viewWidth = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);

    if (Rect->right > viewWidth)
        Rect->right = viewWidth;

    if (Clip)
    {
        if (Rect->top < Context->HeaderHeight)
            Rect->top = Context->HeaderHeight;
        if (Rect->bottom > Context->ClientRect.bottom)
            Rect->bottom = Context->ClientRect.bottom;
    }

    return TRUE;
}

/**
 * Performs hit testing to determine what element is at a given point.
 *
 * \param Context Pointer to the treenew context structure.
 * \param HitTest Pointer to the hit test structure (Point input, other fields output).
 */
VOID PhTnpHitTest(
    _In_ PPH_TREENEW_CONTEXT Context,
    _Inout_ PPH_TREENEW_HIT_TEST HitTest
    )
{
    RECT clientRect;
    LONG x;
    LONG y;
    ULONG index;
    PPH_TREENEW_NODE node;

    HitTest->Flags = 0;
    HitTest->Node = NULL;
    HitTest->Column = NULL;

    clientRect = Context->ClientRect;
    x = HitTest->Point.x;
    y = HitTest->Point.y;

    if (x < 0)
        HitTest->Flags |= TN_HIT_LEFT;
    if (x >= clientRect.right)
        HitTest->Flags |= TN_HIT_RIGHT;
    if (y < 0)
        HitTest->Flags |= TN_HIT_ABOVE;
    if (y >= clientRect.bottom)
        HitTest->Flags |= TN_HIT_BELOW;

    if (HitTest->Flags == 0)
    {
        if (TNP_HIT_TEST_FIXED_DIVIDER(x, Context))
        {
            HitTest->Flags |= TN_HIT_DIVIDER;
        }

        if (y >= Context->HeaderHeight && x < Context->FixedWidth + Context->TotalViewX)
        {
            index = (y - Context->HeaderHeight) / Context->RowHeight + Context->VScrollPosition;

            if (index < Context->FlatList->Count)
            {
                HitTest->Flags |= TN_HIT_ITEM;
                node = Context->FlatList->Items[index];
                HitTest->Node = node;

                if (HitTest->InFlags & TN_TEST_COLUMN)
                {
                    PPH_TREENEW_COLUMN column;
                    LONG columnX;

                    column = NULL;

                    if (x < Context->FixedWidth && Context->FixedColumnVisible)
                    {
                        column = Context->FixedColumn;
                        columnX = 0;
                    }
                    else
                    {
                        LONG currentX;
                        ULONG i;
                        PPH_TREENEW_COLUMN currentColumn;

                        currentX = Context->NormalLeft - Context->HScrollPosition;

                        for (i = 0; i < Context->NumberOfColumnsByDisplay; i++)
                        {
                            currentColumn = Context->ColumnsByDisplay[i];

                            if (x >= currentX && x < currentX + currentColumn->Width)
                            {
                                column = currentColumn;
                                columnX = currentX;
                                break;
                            }

                            currentX += currentColumn->Width;
                        }
                    }

                    HitTest->Column = column;

                    if (column && (HitTest->InFlags & TN_TEST_SUBITEM))
                    {
                        BOOLEAN isFirstColumn;
                        LONG currentX;
                        LONG width;

                        isFirstColumn = HitTest->Column == Context->FirstColumn;

                        currentX = columnX;
                        currentX += Context->CellMarginLeft;

                        if (isFirstColumn)
                        {
                            width = Context->SmallIconWidth;

                            currentX += (LONG)node->Level * width;

                            if (!node->s.IsLeaf)
                            {
                                if (x >= currentX && x < currentX + width)
                                    HitTest->Flags |= TN_HIT_ITEM_PLUSMINUS;

                                currentX += width;
                            }

                            if (node->Icon)
                            {
                                if (x >= currentX && x < currentX + width)
                                    HitTest->Flags |= TN_HIT_ITEM_ICON;

                                currentX += width + Context->IconRightPadding;
                            }
                        }

                        if (x >= currentX)
                        {
                            HitTest->Flags |= TN_HIT_ITEM_CONTENT;
                        }
                    }
                }
            }
        }
    }
}

/**
 * Selects a range of nodes.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Start The starting node index.
 * \param End The ending node index.
 * \param Flags Flags controlling the selection behavior.
 * \param ChangedStart Pointer to receive the start of the changed range, or NULL.
 * \param ChangedEnd Pointer to receive the end of the changed range, or NULL.
 */
VOID PhTnpSelectRange(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Start,
    _In_ ULONG End,
    _In_ ULONG Flags,
    _Out_opt_ PULONG ChangedStart,
    _Out_opt_ PULONG ChangedEnd
    )
{
    ULONG maximum;
    ULONG i;
    PPH_TREENEW_NODE node;
    BOOLEAN targetValue;
    ULONG changedStart;
    ULONG changedEnd;

    if (Context->FlatList->Count == 0)
    {
        if (ChangedStart)
            *ChangedStart = 0;
        if (ChangedEnd)
            *ChangedEnd = 0;

        return;
    }

    maximum = Context->FlatList->Count - 1;

    if (End > maximum)
    {
        End = maximum;
    }

    if (Start > End)
    {
        // Start is too big, so the selection range becomes empty.
        // Set it to max + 1 so that Reset still works.
        Start = maximum + 1;
        End = 0;
    }

    targetValue = !(Flags & TN_SELECT_DESELECT);
    changedStart = maximum;
    changedEnd = 0;

    if (Flags & TN_SELECT_RESET)
    {
        for (i = 0; i < Start; i++)
        {
            node = Context->FlatList->Items[i];

            if (node->Selected)
            {
                node->Selected = FALSE;

                if (changedStart > i)
                    changedStart = i;
                if (changedEnd < i)
                    changedEnd = i;
            }
        }
    }

    for (i = Start; i <= End; i++)
    {
        node = Context->FlatList->Items[i];

        if (!node->Unselectable && ((Flags & TN_SELECT_TOGGLE) || node->Selected != targetValue))
        {
            node->Selected = !node->Selected;

            if (changedStart > i)
                changedStart = i;
            if (changedEnd < i)
                changedEnd = i;
        }
    }

    if (Flags & TN_SELECT_RESET)
    {
        for (i = End + 1; i <= maximum; i++)
        {
            node = Context->FlatList->Items[i];

            if (node->Selected)
            {
                node->Selected = FALSE;

                if (changedStart > i)
                    changedStart = i;
                if (changedEnd < i)
                    changedEnd = i;
            }
        }
    }

    if (changedStart <= changedEnd)
    {
        Context->Callback(Context->Handle, TreeNewSelectionChanged, NULL, NULL, Context->CallbackContext);
    }

    if (ChangedStart)
        *ChangedStart = changedStart;
    if (ChangedEnd)
        *ChangedEnd = changedEnd;
}

/**
 * Sets the hot node (the node under the mouse cursor).
 *
 * \param Context Pointer to the treenew context structure.
 * \param NewHotNode Pointer to the new hot node, or NULL to clear.
 * \param NewPlusMinusHot TRUE if the plus/minus glyph is hot, FALSE otherwise.
 */
VOID PhTnpSetHotNode(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ PPH_TREENEW_NODE NewHotNode,
    _In_ BOOLEAN NewPlusMinusHot
    )
{
    ULONG newHotNodeIndex;
    RECT rowRect;
    BOOLEAN needsInvalidate;

    if (NewHotNode)
        newHotNodeIndex = NewHotNode->Index;
    else
        newHotNodeIndex = ULONG_MAX;

    needsInvalidate = FALSE;

    if (Context->HotNodeIndex != newHotNodeIndex)
    {
        if (Context->HotNodeIndex != ULONG_MAX)
        {
            if (Context->ThemeData && PhTnpGetRowRects(Context, Context->HotNodeIndex, Context->HotNodeIndex, TRUE, &rowRect))
            {
                // Update the old hot node because it may have a different non-hot background and
                // plus minus part.
                PhpTnpInvalidateRect(Context, &rowRect);
            }
        }

        Context->HotNodeIndex = newHotNodeIndex;

        if (NewHotNode)
        {
            needsInvalidate = TRUE;
        }
    }

    if (NewHotNode)
    {
        if (NewHotNode->s.PlusMinusHot != NewPlusMinusHot)
        {
            NewHotNode->s.PlusMinusHot = NewPlusMinusHot;
            needsInvalidate = TRUE;
        }

        if (needsInvalidate && Context->ThemeData && PhTnpGetRowRects(Context, newHotNodeIndex, newHotNodeIndex, TRUE, &rowRect))
        {
            PhpTnpInvalidateRect(Context, &rowRect);
        }
    }
}

/**
 * Processes node selection in response to mouse or keyboard input.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Node Pointer to the node to select.
 * \param ControlKey TRUE if the Control key is pressed, FALSE otherwise.
 * \param ShiftKey TRUE if the Shift key is pressed, FALSE otherwise.
 * \param RightButton TRUE if the right mouse button was used, FALSE otherwise.
 */
VOID PhTnpProcessSelectNode(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_NODE Node,
    _In_ LOGICAL ControlKey,
    _In_ LOGICAL ShiftKey,
    _In_ LOGICAL RightButton
    )
{
    ULONG changedStart;
    ULONG changedEnd;
    RECT rect;

    if (RightButton)
    {
        // Right button:
        // If the current node is selected, then do nothing. This is to allow context menus to
        // operate on multiple items.
        // If the current node is not selected, select only that node.

        if (!ControlKey && !ShiftKey && !Node->Selected)
        {
            PhTnpSelectRange(Context, Node->Index, Node->Index, TN_SELECT_RESET, &changedStart, &changedEnd);
            Context->MarkNodeIndex = Node->Index;

            if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
            {
                PhpTnpInvalidateRect(Context, &rect);
            }
        }
    }
    else if (ShiftKey && Context->MarkNodeIndex != ULONG_MAX)
    {
        ULONG start;
        ULONG end;

        // Shift key: select a range from the selection mark node to the current node.

        if (Node->Index > Context->MarkNodeIndex)
        {
            start = Context->MarkNodeIndex;
            end = Node->Index;
        }
        else
        {
            start = Node->Index;
            end = Context->MarkNodeIndex;
        }

        PhTnpSelectRange(Context, start, end, TN_SELECT_RESET, &changedStart, &changedEnd);

        if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
        {
            PhpTnpInvalidateRect(Context, &rect);
        }
    }
    else if (ControlKey)
    {
        // Control key: toggle the selection on the current node, and also make it the selection
        // mark.

        PhTnpSelectRange(Context, Node->Index, Node->Index, TN_SELECT_TOGGLE, NULL, NULL);
        Context->MarkNodeIndex = Node->Index;

        if (PhTnpGetRowRects(Context, Node->Index, Node->Index, TRUE, &rect))
        {
            PhpTnpInvalidateRect(Context, &rect);
        }
    }
    else
    {
        // Normal: select the current node, and also make it the selection mark.

        PhTnpSelectRange(Context, Node->Index, Node->Index, TN_SELECT_RESET, &changedStart, &changedEnd);
        Context->MarkNodeIndex = Node->Index;

        if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
        {
            PhpTnpInvalidateRect(Context, &rect);
        }
    }
}

/**
 * Ensures that the specified node is visible within the tree new control.
 *
 * This function scrolls the view if necessary to bring the node at the given index
 * into the visible area of the control. If the node is already fully visible, no action is taken.
 *
 * \param Context Pointer to the tree new context structure.
 * \param Index The index of the node to make visible.
 * \return TRUE if the node is (or was made) visible; FALSE if the index is out of range.
 */
BOOLEAN PhTnpEnsureVisibleNode(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Index
    )
{
    LONG viewTop;
    LONG viewBottom;
    LONG rowTop;
    LONG rowBottom;
    LONG deltaY;
    LONG deltaRows;

    if (Index >= Context->FlatList->Count)
        return FALSE;

    viewTop = Context->HeaderHeight;
    viewBottom = Context->ClientRect.bottom - (Context->HScrollVisible ? Context->HScrollHeight : 0);
    rowTop = Context->HeaderHeight + ((LONG)Index - Context->VScrollPosition) * Context->RowHeight;
    rowBottom = rowTop + Context->RowHeight;

    // Check if the row is fully visible.
    if (rowTop >= viewTop && rowBottom <= viewBottom)
        return TRUE;

    deltaY = rowTop - viewTop;

    if (deltaY > 0)
    {
        // The row is below the view area. We want to scroll the row into view at the bottom of the
        // screen. We need to round up when dividing to make sure the node becomes fully visible.
        deltaY = rowBottom - viewBottom;
        deltaRows = (deltaY + Context->RowHeight - 1) / Context->RowHeight; // divide, round up
    }
    else
    {
        deltaRows = deltaY / Context->RowHeight;
    }

    PhTnpScroll(Context, deltaRows, 0);

    return TRUE;
}

/**
 * Processes mouse movement within the tree new control.
 *
 * This function handles mouse movement events by performing hit testing to determine
 * the node and column under the cursor, updating the hot node state, managing divider
 * animation, and handling tooltip display logic.
 *
 * \param Context Pointer to the tree new context structure.
 * \param CursorX The X coordinate of the mouse cursor.
 * \param CursorY The Y coordinate of the mouse cursor.
 */
VOID PhTnpProcessMoveMouse(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    )
{
    PH_TREENEW_HIT_TEST hitTest;
    PPH_TREENEW_NODE hotNode;

    hitTest.Point.x = CursorX;
    hitTest.Point.y = CursorY;
    hitTest.InFlags = TN_TEST_COLUMN | TN_TEST_SUBITEM;
    PhTnpHitTest(Context, &hitTest);

    if (hitTest.Flags & TN_HIT_ITEM)
        hotNode = hitTest.Node;
    else
        hotNode = NULL;

    PhTnpSetHotNode(Context, hotNode, !!(hitTest.Flags & TN_HIT_ITEM_PLUSMINUS));

    if (Context->AnimateDivider && Context->FixedDividerVisible)
    {
        if (hitTest.Flags & TN_HIT_DIVIDER)
        {
            if ((Context->DividerHot < 100 || Context->AnimateDividerFadingOut) && !Context->AnimateDividerFadingIn)
            {
                // Begin fading in the divider.
                Context->AnimateDividerFadingIn = TRUE;
                Context->AnimateDividerFadingOut = FALSE;
                PhSetTimer(Context->Handle, TNP_TIMER_ANIMATE_DIVIDER, TNP_ANIMATE_DIVIDER_INTERVAL, NULL);
            }
        }
        else
        {
            if ((Context->DividerHot != 0 || Context->AnimateDividerFadingIn) && !Context->AnimateDividerFadingOut)
            {
                Context->AnimateDividerFadingOut = TRUE;
                Context->AnimateDividerFadingIn = FALSE;
                PhSetTimer(Context->Handle, TNP_TIMER_ANIMATE_DIVIDER, TNP_ANIMATE_DIVIDER_INTERVAL, NULL);
            }
        }
    }

    if (Context->TooltipsHandle)
    {
        ULONG index;
        ULONG id;

        if (!(hitTest.Flags & TN_HIT_DIVIDER))
        {
            index = hitTest.Node ? hitTest.Node->Index : ULONG_MAX;
            id = hitTest.Column ? hitTest.Column->Id : ULONG_MAX;
        }
        else
        {
            index = ULONG_MAX;
            id = ULONG_MAX;
        }

        // This pops unnecessarily - when the cell has no tooltip text, and the user is moving the
        // mouse over it. However these unnecessary calls seem to fix a certain tooltip bug (move
        // the mouse around very quickly over the last column and the blank space to the right, and
        // no more tooltips will appear).
        if (Context->TooltipIndex != index || Context->TooltipId != id)
        {
            PhTnpPopTooltip(Context);
        }
    }
}

/**
 * Processes vertical mouse wheel events for the tree new control.
 *
 * This function handles vertical scrolling when the user rotates the mouse wheel.
 * It calculates the number of lines to scroll based on system settings and updates the vertical
 * scroll position accordingly, including handling partial scrolls and direction changes.
 * It also manages tooltip updates after scrolling.
 *
 * \param Context Pointer to the tree new context structure.
 * \param Distance The wheel delta value indicating the amount and direction of scrolling.
 */
VOID PhTnpProcessMouseVWheel(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG Distance
    )
{
    ULONG wheelScrollLines;
    FLOAT linesToScroll;
    LONG wholeLinesToScroll;
    SCROLLINFO scrollInfo;
    LONG oldPosition;

    if (!PhGetSystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &wheelScrollLines, 0))
    {
        wheelScrollLines = PhScaleToDisplay(3, Context->WindowDpi);
    }

    // If page scrolling is enabled, use the number of visible rows.
    if (wheelScrollLines == ULONG_MAX)
        wheelScrollLines = (Context->ClientRect.bottom - Context->HeaderHeight - (Context->HScrollVisible ? Context->HScrollHeight : 0)) / Context->RowHeight;

    // Zero the remainder if the direction changed.
    if ((Context->VScrollRemainder > 0) != (Distance > 0))
        Context->VScrollRemainder = 0;

    linesToScroll = (FLOAT)wheelScrollLines * Distance / WHEEL_DELTA + Context->VScrollRemainder;
    wholeLinesToScroll = (LONG)linesToScroll;
    Context->VScrollRemainder = linesToScroll - wholeLinesToScroll;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_ALL;
    GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);
    oldPosition = scrollInfo.nPos;

    scrollInfo.nPos += wholeLinesToScroll;

    scrollInfo.fMask = SIF_POS;
    SetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo, TRUE);
    GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);

    if (scrollInfo.nPos != oldPosition)
    {
        Context->VScrollPosition = scrollInfo.nPos;
        PhTnpProcessScroll(Context, scrollInfo.nPos - oldPosition, 0);

        if (Context->TooltipsHandle)
        {
            MSG message;
            POINT point;

            PhTnpPopTooltip(Context);

            if (
                PhGetClientPos(Context->Handle, &point) && 
                point.x >= 0 && point.y >= 0 && 
                point.x < Context->ClientRect.right && 
                point.y < Context->ClientRect.bottom
                )
            {
                // Send a fake mouse move message for the new node that the mouse may be hovering over.
                message.hwnd = Context->Handle;
                message.message = WM_MOUSEMOVE;
                message.wParam = 0;
                message.lParam = MAKELPARAM(point.x, point.y);
                SendMessage(Context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }
        }
    }
}

/**
 * Processes horizontal mouse wheel events for the tree new control.
 *
 * This function handles horizontal scrolling when the user rotates the mouse wheel horizontally.
 * It calculates the number of characters to scroll based on system settings and updates the horizontal
 * scroll position accordingly, including handling partial scrolls and direction changes.
 *
 * \param Context Pointer to the tree new context structure.
 * \param Distance The wheel delta value indicating the amount and direction of scrolling.
 */
VOID PhTnpProcessMouseHWheel(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG Distance
    )
{
    ULONG wheelScrollChars;
    FLOAT pixelsToScroll;
    LONG wholePixelsToScroll;
    SCROLLINFO scrollInfo;
    LONG deltaX;

    if (!PhGetSystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &wheelScrollChars, 0))
    {
        wheelScrollChars = PhScaleToDisplay(3, Context->WindowDpi);
    }

    // Zero the remainder if the direction changed.
    if ((Context->HScrollRemainder > 0) != (Distance > 0))
        Context->HScrollRemainder = 0;

    pixelsToScroll = (FLOAT)wheelScrollChars * Context->TextMetrics.tmAveCharWidth * Distance / WHEEL_DELTA + Context->HScrollRemainder;
    wholePixelsToScroll = (LONG)pixelsToScroll;
    Context->HScrollRemainder = pixelsToScroll - wholePixelsToScroll;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;
    GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);

    deltaX = PhTnpApplyHScrollPosition(Context, scrollInfo.nPos + wholePixelsToScroll);

    if (deltaX != 0)
        PhTnpProcessScroll(Context, 0, deltaX);
}

/**
 * Processes focus navigation keys for the tree new control.
 *
 * This function handles keyboard navigation for moving the focus between nodes in the tree new control,
 * such as Up, Down, Home, End, Page Up, and Page Down keys. It updates the focused node, selection mark,
 * and selection range as appropriate, and ensures the focused node is visible.
 *
 * \param Context Pointer to the tree new context structure.
 * \param VirtualKey The virtual key code to process (e.g., VK_UP, VK_DOWN, VK_HOME, VK_END, VK_PRIOR, VK_NEXT).
 * \return TRUE if the key was handled; otherwise, FALSE.
 */
BOOLEAN PhTnpProcessFocusKey(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKey
    )
{
    ULONG count;
    ULONG index;
    BOOLEAN controlKey;
    BOOLEAN shiftKey;
    ULONG start;
    ULONG end;
    ULONG changedStart;
    ULONG changedEnd;
    RECT rect;

    if (VirtualKey != VK_UP && VirtualKey != VK_DOWN &&
        VirtualKey != VK_HOME && VirtualKey != VK_END &&
        VirtualKey != VK_PRIOR && VirtualKey != VK_NEXT)
    {
        return FALSE;
    }

    count = Context->FlatList->Count;

    if (count == 0)
        return TRUE;

    // Find the new node to focus.

    switch (VirtualKey)
    {
    case VK_UP:
        {
            if (Context->FocusNode && Context->FocusNode->Index > 0)
            {
                index = Context->FocusNode->Index - 1;
            }
            else
            {
                index = 0;
            }
        }
        break;
    case VK_DOWN:
        {
            if (Context->FocusNode)
            {
                index = Context->FocusNode->Index + 1;

                if (index >= count)
                    index = count - 1;
            }
            else
            {
                index = 0;
            }
        }
        break;
    case VK_HOME:
        index = 0;
        break;
    case VK_END:
        index = count - 1;
        break;
    case VK_PRIOR:
    case VK_NEXT:
        {
            LONG rowsPerPage;

            if (Context->FocusNode)
                index = Context->FocusNode->Index;
            else
                index = 0;

            rowsPerPage = Context->ClientRect.bottom - Context->HeaderHeight - (Context->HScrollVisible ? Context->HScrollHeight : 0);

            if (rowsPerPage < 0)
                return TRUE;

            rowsPerPage = rowsPerPage / Context->RowHeight - 1;

            if (rowsPerPage < 0)
                return TRUE;

            if (VirtualKey == VK_PRIOR)
            {
                ULONG startOfPageIndex;

                startOfPageIndex = Context->VScrollPosition;

                if (index > startOfPageIndex)
                {
                    index = startOfPageIndex;
                }
                else
                {
                    // Already at or before the start of the page. Go back a page.
                    if (index >= (ULONG)rowsPerPage)
                        index -= rowsPerPage;
                    else
                        index = 0;
                }
            }
            else
            {
                ULONG endOfPageIndex;

                endOfPageIndex = Context->VScrollPosition + rowsPerPage;

                if (endOfPageIndex >= count)
                    endOfPageIndex = count - 1;

                if (index < endOfPageIndex)
                {
                    index = endOfPageIndex;
                }
                else
                {
                    // Already at or after the end of the page. Go forward a page.
                    index += rowsPerPage;

                    if (index >= count)
                        index = count - 1;
                }
            }
        }
        break;
    default:
        {
            index = 0;
        }
        break;
    }

    // Select the relevant nodes.

    controlKey = GetKeyState(VK_CONTROL) < 0;
    shiftKey = GetKeyState(VK_SHIFT) < 0;

    Context->FocusNode = Context->FlatList->Items[index];
    PhTnpSetHotNode(Context, Context->FocusNode, FALSE);

    if (shiftKey && Context->MarkNodeIndex != ULONG_MAX)
    {
        if (index > Context->MarkNodeIndex)
        {
            start = Context->MarkNodeIndex;
            end = index;
        }
        else
        {
            start = index;
            end = Context->MarkNodeIndex;
        }

        PhTnpSelectRange(Context, start, end, TN_SELECT_RESET, &changedStart, &changedEnd);

        if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
        {
            PhpTnpInvalidateRect(Context, &rect);
        }
    }
    else if (!controlKey)
    {
        Context->MarkNodeIndex = Context->FocusNode->Index;
        PhTnpSelectRange(Context, index, index, TN_SELECT_RESET, &changedStart, &changedEnd);

        if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
        {
            PhpTnpInvalidateRect(Context, &rect);
        }
    }

    PhTnpEnsureVisibleNode(Context, index);
    PhTnpPopTooltip(Context);

    return TRUE;
}

/**
 * Processes a key event for the focused node in the tree new control.
 *
 * This function handles keyboard input such as space, left, right, and plus keys when a node is focused.
 * It manages node selection, expansion/collapse, and navigation based on the key pressed and modifier keys.
 *
 * \param Context Pointer to the tree new context structure.
 * \param VirtualKey The virtual key code of the key event to process.
 * \return TRUE if the key was handled; otherwise, FALSE.
 */
BOOLEAN PhTnpProcessNodeKey(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKey
    )
{
    BOOLEAN controlKey;
    BOOLEAN shiftKey;
    ULONG changedStart;
    ULONG changedEnd;
    RECT rect;

    if (VirtualKey != VK_SPACE && VirtualKey != VK_LEFT && VirtualKey != VK_RIGHT && VirtualKey != VK_ADD && VirtualKey != VK_OEM_PLUS)
    {
        return FALSE;
    }

    if (!Context->FocusNode)
        return TRUE;

    controlKey = GetKeyState(VK_CONTROL) < 0;
    shiftKey = GetKeyState(VK_SHIFT) < 0;

    switch (VirtualKey)
    {
    case VK_SPACE:
        {
            if (controlKey)
            {
                // Control key: toggle the selection on the focused node.

                Context->MarkNodeIndex = Context->FocusNode->Index;
                PhTnpSelectRange(Context, Context->FocusNode->Index, Context->FocusNode->Index, TN_SELECT_TOGGLE, &changedStart, &changedEnd);

                if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                {
                    PhpTnpInvalidateRect(Context, &rect);
                }
            }
            else if (shiftKey)
            {
                ULONG start;
                ULONG end;

                // Shift key: select a range from the selection mark node to the focused node.

                if (Context->MarkNodeIndex == ULONG_MAX)
                {
                    Context->MarkNodeIndex = Context->FocusNode->Index;
                    return TRUE;
                }

                if (Context->FocusNode->Index > Context->MarkNodeIndex)
                {
                    start = Context->MarkNodeIndex;
                    end = Context->FocusNode->Index;
                }
                else
                {
                    start = Context->FocusNode->Index;
                    end = Context->MarkNodeIndex;
                }

                PhTnpSelectRange(Context, start, end, TN_SELECT_RESET, &changedStart, &changedEnd);

                if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                {
                    PhpTnpInvalidateRect(Context, &rect);
                }
            }
        }
        break;
    case VK_LEFT:
        {
            ULONG i;
            ULONG targetLevel;
            PPH_TREENEW_NODE newNode;

            // If the node is expanded, collapse it. Otherwise, select the node's parent.
            if (!Context->FocusNode->s.IsLeaf && Context->FocusNode->Expanded)
            {
                PhTnpSetExpandedNode(Context, Context->FocusNode, FALSE);
            }
            else if (Context->FocusNode->Level != 0)
            {
                i = Context->FocusNode->Index;
                targetLevel = Context->FocusNode->Level - 1;

                while (i != 0)
                {
                    i--;
                    newNode = Context->FlatList->Items[i];

                    if (newNode->Level == targetLevel)
                    {
                        Context->FocusNode = newNode;
                        Context->MarkNodeIndex = newNode->Index;
                        PhTnpEnsureVisibleNode(Context, i);
                        PhTnpSetHotNode(Context, newNode, FALSE);
                        PhTnpSelectRange(Context, i, i, TN_SELECT_RESET, &changedStart, &changedEnd);

                        if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                        {
                            PhpTnpInvalidateRect(Context, &rect);
                        }

                        PhTnpPopTooltip(Context);

                        break;
                    }
                }
            }
        }
        break;
    case VK_RIGHT:
        {
            PPH_TREENEW_NODE newNode;

            if (!Context->FocusNode->s.IsLeaf)
            {
                // If the node is collapsed, expand it. Otherwise, select the node's first child.
                if (!Context->FocusNode->Expanded)
                {
                    PhTnpSetExpandedNode(Context, Context->FocusNode, TRUE);
                }
                else
                {
                    if (Context->FocusNode->Index + 1 < Context->FlatList->Count)
                    {
                        newNode = Context->FlatList->Items[Context->FocusNode->Index + 1];

                        if (newNode->Level == Context->FocusNode->Level + 1)
                        {
                            Context->FocusNode = newNode;
                            Context->MarkNodeIndex = newNode->Index;
                            PhTnpEnsureVisibleNode(Context, Context->FocusNode->Index);
                            PhTnpSetHotNode(Context, newNode, FALSE);
                            PhTnpSelectRange(Context, Context->FocusNode->Index, Context->FocusNode->Index, TN_SELECT_RESET, &changedStart, &changedEnd);

                            if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                            {
                                PhpTnpInvalidateRect(Context, &rect);
                            }

                            PhTnpPopTooltip(Context);
                        }
                    }
                }
            }
        }
        break;
    case VK_ADD:
    case VK_OEM_PLUS:
        {
            if ((VirtualKey == VK_ADD && controlKey) ||
                (VirtualKey == VK_OEM_PLUS && controlKey && shiftKey))
            {
                ULONG i;

                if (Context->FixedColumnVisible)
                    PhTnpAutoSizeColumnHeader(Context, Context->FixedHeaderHandle, Context->FixedColumn, 0);

                for (i = 0; i < Context->NumberOfColumnsByDisplay; i++)
                    PhTnpAutoSizeColumnHeader(Context, Context->HeaderHandle, Context->ColumnsByDisplay[i], 0);
            }
        }
        break;
    }

    return TRUE;
}

/**
 * Processes a character input for incremental search in the tree view control.
 *
 * This function handles character input (typically from WM_CHAR) to perform incremental
 * search within the tree view. It manages the search buffer, handles timeouts, and
 * updates the focused node and selection based on the search result.
 *
 * \param Context Pointer to the tree view context.
 * \param Character The character code to process for search.
 */
VOID PhTnpProcessSearchKey(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Character
    )
{
    LONG messageTime;
    BOOLEAN newSearch;
    PH_TREENEW_SEARCH_EVENT searchEvent;
    PPH_TREENEW_NODE foundNode;
    ULONG changedStart;
    ULONG changedEnd;
    RECT rect;

    if (Context->FlatList->Count == 0)
        return;

    messageTime = GetMessageTime();
    newSearch = FALSE;

    // Check if the search timed out.
    if (messageTime - Context->SearchMessageTime > PH_TREENEW_SEARCH_TIMEOUT)
    {
        Context->SearchStringCount = 0;
        Context->SearchFailed = FALSE;
        newSearch = TRUE;
        Context->SearchSingleCharMode = TRUE;
    }

    Context->SearchMessageTime = messageTime;

    // Append the character to the search buffer.

    if (!Context->SearchString)
    {
        Context->AllocatedSearchString = 32;
        Context->SearchString = PhAllocateSafe(Context->AllocatedSearchString * sizeof(WCHAR));
        newSearch = TRUE;
        Context->SearchSingleCharMode = TRUE;
    }

    if (!Context->SearchString)
    {
        Context->SearchFailed = TRUE;
        return;
    }

    if (Context->SearchStringCount >= PH_TREENEW_SEARCH_MAXIMUM_LENGTH)
    {
        // The search string has become too long. Fail the search.
        if (!Context->SearchFailed)
            MessageBeep(MB_OK);

        Context->SearchFailed = TRUE;
        return;
    }
    else if (Context->SearchStringCount == Context->AllocatedSearchString)
    {
        ULONG allocatedSearchString;
        PWSTR searchString;

        allocatedSearchString = Context->AllocatedSearchString * 2;
        searchString = PhReAllocateSafe(Context->SearchString, allocatedSearchString * sizeof(WCHAR));

        if (!searchString)
        {
            Context->SearchFailed = TRUE;
            return;
        }

        Context->AllocatedSearchString = allocatedSearchString;
        Context->SearchString = searchString;
    }

    Context->SearchString[Context->SearchStringCount++] = (WCHAR)Character;

    if (Context->SearchString[Context->SearchStringCount - 1] != Context->SearchString[0])
    {
        // The user has stopped typing the same character (or never started). Turn single-character
        // search off.
        Context->SearchSingleCharMode = FALSE;
    }

    searchEvent.FoundIndex = INT_ERROR;

    if (Context->FocusNode)
    {
        searchEvent.StartIndex = Context->FocusNode->Index;

        if (newSearch || Context->SearchSingleCharMode)
        {
            // If it's a new search, start at the next item so the user doesn't find the same item
            // again.
            searchEvent.StartIndex++;

            if (searchEvent.StartIndex == Context->FlatList->Count)
                searchEvent.StartIndex = 0;
        }
    }
    else
    {
        searchEvent.StartIndex = 0;
    }

    searchEvent.String.Buffer = Context->SearchString;

    if (!Context->SearchSingleCharMode)
        searchEvent.String.Length = Context->SearchStringCount * sizeof(WCHAR);
    else
        searchEvent.String.Length = sizeof(WCHAR);

    // Give the user a chance to modify how the search is performed.
    if (!Context->Callback(Context->Handle, TreeNewIncrementalSearch, &searchEvent, NULL, Context->CallbackContext))
    {
        // Use the default search function.
        if (!PhTnpDefaultIncrementalSearch(Context, &searchEvent, TRUE, TRUE))
        {
            return;
        }
    }

    if (searchEvent.FoundIndex == INT_ERROR && !Context->SearchFailed)
    {
        // No search result. Beep to indicate an error, and set the flag so we don't beep again. But
        // don't beep if the first character was a space, because that's used for other purposes
        // elsewhere (see PhTnpProcessNodeKey).
        if (searchEvent.String.Buffer[0] != L' ')
        {
            MessageBeep(MB_OK);
        }

        Context->SearchFailed = TRUE;
        return;
    }

    if (searchEvent.FoundIndex < 0 || searchEvent.FoundIndex >= (LONG)Context->FlatList->Count)
        return;

    foundNode = Context->FlatList->Items[searchEvent.FoundIndex];
    Context->FocusNode = foundNode;
    PhTnpEnsureVisibleNode(Context, searchEvent.FoundIndex);
    PhTnpSetHotNode(Context, foundNode, FALSE);
    PhTnpSelectRange(Context, searchEvent.FoundIndex, searchEvent.FoundIndex, TN_SELECT_RESET, &changedStart, &changedEnd);

    if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
    {
        PhpTnpInvalidateRect(Context, &rect);
    }

    PhTnpPopTooltip(Context);
}

/**
 * Performs the default incremental search in the tree view.
 *
 * This function searches for a node whose text matches the search string, starting from a given index.
 * It supports partial and wrapped searches.
 *
 * \param Context Pointer to the tree view context.
 * \param SearchEvent Pointer to the search event structure, updated with the found index.
 * \param Partial TRUE to allow partial matches; FALSE for exact matches.
 * \param Wrap TRUE to wrap around the list; FALSE to stop at the end.
 * \return TRUE if the search was performed; FALSE otherwise.
 */
BOOLEAN PhTnpDefaultIncrementalSearch(
    _In_ PPH_TREENEW_CONTEXT Context,
    _Inout_ PPH_TREENEW_SEARCH_EVENT SearchEvent,
    _In_ BOOLEAN Partial,
    _In_ BOOLEAN Wrap
    )
{
    LONG startIndex;
    LONG currentIndex;
    LONG foundIndex;
    BOOLEAN firstTime;

    if (Context->FlatList->Count == 0)
        return FALSE;
    if (!Context->FirstColumn)
        return FALSE;

    startIndex = SearchEvent->StartIndex;
    currentIndex = startIndex;
    foundIndex = INT_ERROR;
    firstTime = TRUE;

    while (TRUE)
    {
        PH_STRINGREF text;

        if (currentIndex >= (LONG)Context->FlatList->Count)
        {
            if (Wrap)
                currentIndex = 0;
            else
                break;
        }

        // We use the firstTime variable instead of a simpler check because we want to include the
        // current item in the search. E.g. the current item is the only item beginning with "Z". If
        // the user searches for "Z", we want to return the current item as being found.
        if (!firstTime && currentIndex == startIndex)
            break;

        if (PhTnpGetCellText(Context, Context->FlatList->Items[currentIndex], Context->FirstColumn->Id, &text))
        {
            if (Partial)
            {
                if (PhStartsWithStringRef(&text, &SearchEvent->String, TRUE))
                {
                    foundIndex = currentIndex;
                    break;
                }
            }
            else
            {
                if (PhEqualStringRef(&text, &SearchEvent->String, TRUE))
                {
                    foundIndex = currentIndex;
                    break;
                }
            }
        }

        currentIndex++;
        firstTime = FALSE;
    }

    SearchEvent->FoundIndex = foundIndex;

    return TRUE;
}

/**
 * Updates the scroll bars for the tree view control.
 *
 * This function recalculates and updates the visibility, range, and position of the vertical and horizontal scroll bars
 * based on the current content and client area size. It also manages the filler box visibility and triggers scrolling if needed.
 *
 * \param Context Pointer to the tree view context.
 */
VOID PhTnpUpdateScrollBars(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    RECT clientRect;
    LONG width;
    LONG height;
    LONG contentWidth;
    LONG contentHeight;
    SCROLLINFO scrollInfo;
    LONG oldPosition;
    LONG deltaRows;
    LONG deltaX;
    LOGICAL oldHScrollVisible;
    RECT rect;
#if defined(TREENEW_VSCROLL_ANCHOR)
    LONG anchoredPosition;
#endif

    clientRect = Context->ClientRect;
    // The normal columns start at NormalLeft, not FixedWidth; using the latter left the scroll
    // range one pixel wider than the area actually painted. (dmex)
    width = clientRect.right - Context->NormalLeft;
    height = clientRect.bottom - Context->HeaderHeight;

    contentWidth = Context->TotalViewX;
    contentHeight = (LONG)Context->FlatList->Count * Context->RowHeight;

    if (contentHeight > height)
    {
        // We need a vertical scrollbar, so we can't use that area of the screen for content.
        width -= Context->VScrollWidth;
    }

    if (contentWidth > width)
    {
        height -= Context->HScrollHeight;
    }

    // The client area can be smaller than the header while the control is being sized. Page sizes
    // are unsigned, so a negative value would wrap into a huge range. (dmex)

    if (width < 0)
        width = 0;
    if (height < 0)
        height = 0;

    // Vertical scroll bar

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;
    GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);
    oldPosition = scrollInfo.nPos;

    scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
    scrollInfo.nMin = 0;
    scrollInfo.nMax = Context->FlatList->Count != 0 ? Context->FlatList->Count - 1 : 0;
    scrollInfo.nPage = height / Context->RowHeight;

    // This runs on every layout, which includes every structure update. Skip the cross-window
    // update when the range and page are unchanged. (dmex)
    if (Context->VScrollLastMax != scrollInfo.nMax || Context->VScrollLastPage != scrollInfo.nPage)
    {
        Context->VScrollLastMax = scrollInfo.nMax;
        Context->VScrollLastPage = scrollInfo.nPage;

        SetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo, TRUE);
    }

#if defined(TREENEW_VSCROLL_ANCHOR)
    if (PhTnpGetAnchoredVScrollPosition(Context, scrollInfo.nPage, &anchoredPosition))
    {
        scrollInfo.fMask = SIF_POS;
        scrollInfo.nPos = anchoredPosition;
        SetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo, TRUE);
    }
#endif

    // The scroll position may have changed due to the modified scroll range.
    scrollInfo.fMask = SIF_POS;
    GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);
    deltaRows = scrollInfo.nPos - oldPosition;
    Context->VScrollPosition = scrollInfo.nPos;

    if (contentHeight > height && contentHeight != 0)
    {
        if (!Context->VScrollVisible)
        {
            ShowWindow(Context->VScrollHandle, SW_SHOW);
            Context->VScrollVisible = TRUE;
        }
    }
    else
    {
        if (Context->VScrollVisible)
        {
            ShowWindow(Context->VScrollHandle, SW_HIDE);
            Context->VScrollVisible = FALSE;
        }
    }

    // Horizontal scroll bar

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;
    GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);
    oldPosition = scrollInfo.nPos;

    scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
    scrollInfo.nMin = 0;
    scrollInfo.nMax = contentWidth != 0 ? contentWidth - 1 : 0;
    scrollInfo.nPage = width;

    if (Context->HScrollLastMax != scrollInfo.nMax || Context->HScrollLastPage != scrollInfo.nPage)
    {
        Context->HScrollLastMax = scrollInfo.nMax;
        Context->HScrollLastPage = scrollInfo.nPage;

        SetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo, TRUE);
    }

    scrollInfo.fMask = SIF_POS;
    GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);
    deltaX = scrollInfo.nPos - oldPosition;
    Context->HScrollPosition = scrollInfo.nPos;

    oldHScrollVisible = Context->HScrollVisible;

    if (contentWidth > width && contentWidth != 0)
    {
        if (!Context->HScrollVisible)
        {
            ShowWindow(Context->HScrollHandle, SW_SHOW);
            Context->HScrollVisible = TRUE;
        }
    }
    else
    {
        if (Context->HScrollVisible)
        {
            ShowWindow(Context->HScrollHandle, SW_HIDE);
            Context->HScrollVisible = FALSE;
        }
    }

    if ((Context->HScrollVisible != oldHScrollVisible) && Context->FixedDividerVisible && Context->AnimateDivider)
    {
        rect.left = Context->FixedWidth;
        rect.top = Context->HeaderHeight;
        rect.right = Context->FixedWidth + 1;
        rect.bottom = Context->ClientRect.bottom;
        PhpTnpInvalidateRect(Context, &rect);
    }

#if defined(TREENEW_VSCROLL_ANCHOR)
    if (Context->FlatListStructureChanged)
    {
        // Optimization: when the viewport is anchored to the end of the list and rows were
        // only appended (pure append), the already-visible rows haven't changed content.
        // Blit them up by deltaRows and repaint only the newly exposed bottom strip, instead
        // of invalidating the entire client area.
        if (
            Context->FlatListAnchorEnd &&
            deltaX == 0 &&
            deltaRows > 0 &&
            Context->FlatList->Count > Context->FlatListPreCount &&
            deltaRows == (LONG)(Context->FlatList->Count - Context->FlatListPreCount)
            )
        {
            PhTnpProcessScroll(Context, deltaRows, 0);
        }
        else
        {
            PhpTnpInvalidateRect(Context, NULL);
        }

        Context->FlatListStructureChanged = FALSE;
        Context->FlatListAnchorEnd = FALSE;
    }
    else if (deltaRows != 0 || deltaX != 0)
    {
        PhTnpProcessScroll(Context, deltaRows, deltaX);
    }
#else
    if (deltaRows != 0 || deltaX != 0)
    {
        PhTnpProcessScroll(Context, deltaRows, deltaX);
    }
#endif

    if (Context->VScrollVisible && Context->HScrollVisible)
    {
        if (!Context->FillerBoxVisible)
        {
            ShowWindow(Context->FillerBoxHandle, SW_SHOW);
            Context->FillerBoxVisible = TRUE;
        }
    }
    else
    {
        if (Context->FillerBoxVisible)
        {
            ShowWindow(Context->FillerBoxHandle, SW_HIDE);
            Context->FillerBoxVisible = FALSE;
        }
    }
}

/**
 * Scrolls the tree view by the specified number of rows and columns.
 *
 * This function adjusts the scroll bar positions and triggers a redraw if the scroll position changes.
 *
 * \param Context Pointer to the tree view context.
 * \param DeltaRows Number of rows to scroll vertically (can be MINLONG or MAXLONG for extremes).
 * \param DeltaX Number of pixels to scroll horizontally (can be MINLONG or MAXLONG for extremes).
 */
VOID PhTnpScroll(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG DeltaRows,
    _In_ LONG DeltaX
    )
{
    SCROLLINFO scrollInfo = { 0 };
    LONG oldPosition;
    LONG deltaRows;
    LONG deltaX;

    deltaRows = 0;
    deltaX = 0;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;

    if (DeltaRows != 0 && Context->VScrollVisible)
    {
        if (!GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo))
            return;
        oldPosition = scrollInfo.nPos;

        if (DeltaRows == MINLONG)
            scrollInfo.nPos = 0;
        else if (DeltaRows == MAXLONG)
            scrollInfo.nPos = Context->FlatList->Count - 1;
        else
            scrollInfo.nPos += DeltaRows;

        SetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo, TRUE);
        if (!GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo))
            return;
        Context->VScrollPosition = scrollInfo.nPos;
        deltaRows = scrollInfo.nPos - oldPosition;
    }

    if (DeltaX != 0 && Context->HScrollVisible)
    {
        if (!GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo))
            return;

        if (DeltaX == MINLONG)
            scrollInfo.nPos = 0;
        else if (DeltaX == MAXLONG)
            scrollInfo.nPos = Context->TotalViewX;
        else
            scrollInfo.nPos += DeltaX;

        deltaX = PhTnpApplyHScrollPosition(Context, scrollInfo.nPos);
    }

    if (deltaRows != 0 || deltaX != 0)
        PhTnpProcessScroll(Context, deltaRows, deltaX);
}

/**
 * Moves the horizontal scroll bar to an absolute position.
 *
 * The scroll bar clamps the requested position to its range, so the caller has to be told what was
 * actually applied. This is the tail shared by the scroll bar, mouse wheel and programmatic scroll
 * paths. (dmex)
 *
 * \param Context Pointer to the tree view context.
 * \param Position The requested scroll position.
 * \return The number of pixels actually scrolled; zero if the position did not change.
 */
LONG PhTnpApplyHScrollPosition(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG Position
    )
{
    SCROLLINFO scrollInfo = { 0 };
    LONG oldPosition;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;
    if (!GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo))
        return 0;
    oldPosition = scrollInfo.nPos;

    scrollInfo.nPos = Position;
    SetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo, TRUE);

    if (!GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo))

        return 0;
    Context->HScrollPosition = scrollInfo.nPos;

    return scrollInfo.nPos - oldPosition;
}

/**
 * Processes the actual scrolling of the tree view window.
 *
 * This function performs the window scrolling operation for both vertical and horizontal directions,
 * updates the header layout, and records the scroll tick count.
 *
 * \param Context Pointer to the tree view context.
 * \param DeltaRows Number of rows to scroll vertically.
 * \param DeltaX Number of pixels to scroll horizontally.
 */
VOID PhTnpProcessScroll(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG DeltaRows,
    _In_ LONG DeltaX
    )
{
    RECT rect;
    LONG deltaY;

    rect.top = Context->HeaderHeight;
    rect.bottom = Context->ClientRect.bottom;

    if (DeltaX == 0)
    {
        rect.left = 0;
        rect.right = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);
        ScrollWindowEx(
            Context->Handle,
            0,
            -DeltaRows * Context->RowHeight,
            &rect,
            NULL,
            NULL,
            NULL,
            SW_INVALIDATE
            );
    }
    else
    {
        // Don't scroll if there are no rows. This is especially important if the user wants us to
        // display empty text.
        if (Context->FlatList->Count != 0)
        {
            deltaY = DeltaRows * Context->RowHeight;

            // If we're scrolling vertically as well, we need to scroll the fixed part and the
            // normal part separately.

            if (DeltaRows != 0)
            {
                rect.left = 0;
                rect.right = Context->NormalLeft;
                ScrollWindowEx(
                    Context->Handle,
                    0,
                    -deltaY,
                    &rect,
                    &rect,
                    NULL,
                    NULL,
                    SW_INVALIDATE
                    );
            }

            rect.left = Context->NormalLeft;
            rect.right = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);
            ScrollWindowEx(
                Context->Handle,
                -DeltaX,
                -deltaY,
                &rect,
                &rect,
                NULL,
                NULL,
                SW_INVALIDATE
                );
        }

        PhTnpLayoutHeader(Context);
    }

    Context->ScrollTickCount = NtGetTickCount64();
}

/**
 * Determines if the tree view can scroll in the specified direction.
 *
 * This function checks the scroll bar positions and ranges to determine if scrolling is possible
 * in the given direction (horizontal/vertical, positive/negative).
 *
 * \param Context Pointer to the tree view context.
 * \param Horizontal TRUE to check horizontal scrolling; FALSE for vertical.
 * \param Positive TRUE to check forward (right/down); FALSE for backward (left/up).
 * \return TRUE if scrolling is possible; FALSE otherwise.
 */
BOOLEAN PhTnpCanScroll(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ BOOLEAN Horizontal,
    _In_ BOOLEAN Positive
    )
{
    SCROLLINFO scrollInfo = { 0 };

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

    if (!Horizontal)
    {
        if (!GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo))
            return FALSE;
    }
    else
    {
        if (!GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo))
            return FALSE;
    }

    if (Positive)
    {
        if (scrollInfo.nPage != 0)
            scrollInfo.nMax -= scrollInfo.nPage - 1;

        return scrollInfo.nPos < scrollInfo.nMax;
    }
    else
    {
        return scrollInfo.nPos > scrollInfo.nMin;
    }
}

/**
 * Paints the tree view control.
 *
 * This function handles the drawing of all visible rows, columns, and cells in the tree view,
 * including themed and non-themed backgrounds, selection, and custom colors. It also manages
 * the drawing of fixed and normal columns and handles the painting of empty space.
 *
 * \param WindowHandle Handle to the tree view window.
 * \param Context Pointer to the tree view context.
 * \param hdc Handle to the device context for painting.
 * \param PaintRect Pointer to the rectangle to be painted.
 */
VOID PhTnpPaint(
    _In_ HWND WindowHandle,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc,
    _In_ PRECT PaintRect
    )
{
    RECT viewRect;
    LONG vScrollPosition;
    LONG hScrollPosition;
    LONG firstRowToUpdate;
    LONG lastRowToUpdate;
    LONG i;
    LONG j;
    PPH_TREENEW_NODE node;
    PPH_TREENEW_COLUMN column;
    RECT rowRect;
    LONG x;
    BOOLEAN fixedUpdate;
    LONG normalUpdateLeftX;
    LONG normalUpdateRightX;
    LONG normalUpdateLeftIndex;
    LONG normalUpdateRightIndex;
    LONG normalTotalX;
    RECT cellRect;
    LONG selectionBarLeft;
    LONG selectionBarWidth;
    BOOLEAN selectionBarVisible;
    COLORREF selectionBarColor;
    HRGN oldClipRegion;
    BOOLEAN hasOldClipRegion;

    PhTnpInitializeThemeData(Context);

    viewRect = Context->ClientRect;

    if (Context->VScrollVisible)
        viewRect.right -= Context->VScrollWidth;

    vScrollPosition = Context->VScrollPosition;
    hScrollPosition = Context->HScrollPosition;

    // Calculate the indices of the first and last rows that need painting. These indices are
    // relative to the top of the view area.

    firstRowToUpdate = (PaintRect->top - Context->HeaderHeight) / Context->RowHeight;
    lastRowToUpdate = (PaintRect->bottom - 1 - Context->HeaderHeight) / Context->RowHeight; // minus one since bottom is exclusive

    if (firstRowToUpdate < 0)
        firstRowToUpdate = 0;

    rowRect.left = 0;
    rowRect.top = Context->HeaderHeight + firstRowToUpdate * Context->RowHeight;
    rowRect.right = Context->NormalLeft + Context->TotalViewX - Context->HScrollPosition;
    rowRect.bottom = rowRect.top + Context->RowHeight;

    // Calculate the accent bar drawn along the left edge of selected rows. This must be done here
    // because the themed background below moves rowRect.left when there's no fixed column. (dmex)

    selectionBarLeft = Context->FixedColumnVisible ? 0 : Context->NormalLeft - hScrollPosition;

    if (selectionBarLeft < 0)
        selectionBarLeft = 0;

    selectionBarWidth = PhScaleToDisplay(PH_TREENEW_SELECTION_BAR_WIDTH, Context->WindowDpi);
    selectionBarVisible = selectionBarLeft < PaintRect->right && selectionBarLeft + selectionBarWidth > PaintRect->left;

    // The bar is a theme accent rather than a row color, so the palette accent wins over the
    // custom row colors. PhThemeWindowHighlightColor is a grey and doesn't show up here. (dmex)

    if (Context->ThemeSupport)
        selectionBarColor = PhThemeWindowFocusBorderColor;
    else if (Context->CustomColors)
        selectionBarColor = Context->CustomFocusColor;
    else
        selectionBarColor = GetSysColor(COLOR_HOTLIGHT);

    // Change the indices to absolute row indices.

    firstRowToUpdate += vScrollPosition;
    lastRowToUpdate += vScrollPosition;

    if (lastRowToUpdate >= (LONG)Context->FlatList->Count)
        lastRowToUpdate = Context->FlatList->Count - 1; // becomes -1 when there are no items, handled correctly by loop below

    // Determine whether the fixed column needs painting, and which normal columns need painting.

    fixedUpdate = FALSE;

    if (Context->FixedColumnVisible && PaintRect->left < Context->FixedWidth)
        fixedUpdate = TRUE;

    x = Context->NormalLeft - hScrollPosition;
    normalUpdateLeftX = viewRect.right;
    normalUpdateLeftIndex = 0;
    normalUpdateRightX = 0;
    normalUpdateRightIndex = -1;

    for (j = 0; j < (LONG)Context->NumberOfColumnsByDisplay; j++)
    {
        column = Context->ColumnsByDisplay[j];

        if (x + column->Width >= Context->NormalLeft && x + column->Width > PaintRect->left && x < PaintRect->right)
        {
            if (normalUpdateLeftX > x)
            {
                normalUpdateLeftX = x;
                normalUpdateLeftIndex = j;
            }

            if (normalUpdateRightX < x + column->Width)
            {
                normalUpdateRightX = x + column->Width;
                normalUpdateRightIndex = j;
            }
        }

        x += column->Width;
    }

    normalTotalX = x;

    if (normalUpdateRightIndex >= (LONG)Context->NumberOfColumnsByDisplay)
        normalUpdateRightIndex = Context->NumberOfColumnsByDisplay - 1;

    // Paint the rows.

    SelectFont(hdc, Context->Font);
    SetBkMode(hdc, TRANSPARENT);

    for (i = firstRowToUpdate; i <= lastRowToUpdate; i++)
    {
        node = Context->FlatList->Items[i];

        // PaintRect is only the bounding box of the update region. When rows are invalidated in
        // disjoint bands (sparse selection changes) most of the rows in between are still clean, so
        // skip the ones the device context would clip away anyway. (dmex)

        if (!RectVisible(hdc, &rowRect))
        {
            rowRect.top += Context->RowHeight;
            rowRect.bottom += Context->RowHeight;
            continue;
        }

        // Prepare the row for drawing.

        PhTnpPrepareRowForDraw(Context, hdc, node);

        if (Context->ThemeSupport)
        {
            SetTextColor(hdc, PhThemeWindowTextColor);

            // The custom row color is blended over the window background at a constant alpha. Both
            // sides of that blend are solid, so the result is a solid color and there is no reason
            // to run GdiAlphaBlend for every row. (dmex)

            if (node->s.DrawBackColor != Context->DefaultBackColor)
            {
                SetDCBrushColor(hdc, PhpTnpBlendColor(
                    PhThemeWindowBackgroundColor,
                    node->s.DrawBackColor,
                    TNP_THEME_ROW_BLEND_ALPHA
                    ));
                FillRect(hdc, &rowRect, PhGetStockBrush(DC_BRUSH));
            }
            else
            {
                FillRect(hdc, &rowRect, PhThemeWindowBackgroundBrush);
            }

            // Draw the outline of the selection rectangle (Dart Vanya)
            if (Context->HasFocus && node->Selected)
            {
                FrameRect(hdc, &rowRect, PhThemeWindowBackgroundBrush);
            }
        }
        else
        {
            if (node->Selected && (Context->CustomColors || !Context->ThemeHasItemBackground))
            {
                // Non-themed background

                if (Context->HasFocus)
                {
                    if (Context->CustomColors)
                    {
                        SetTextColor(hdc, Context->CustomTextColor);
                        SetDCBrushColor(hdc, Context->CustomFocusColor);
                        FillRect(hdc, &rowRect, PhGetStockBrush(DC_BRUSH));
                    }
                    else
                    {
                        SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                        FillRect(hdc, &rowRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                    }
                }
                else
                {
                    if (Context->CustomColors)
                    {
                        SetTextColor(hdc, Context->CustomTextColor);
                        SetDCBrushColor(hdc, Context->CustomSelectedColor);
                        FillRect(hdc, &rowRect, PhGetStockBrush(DC_BRUSH));
                    }
                    else
                    {
                        SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
                        SetDCBrushColor(hdc, GetSysColor(COLOR_BTNFACE));
                        FillRect(hdc, &rowRect, GetSysColorBrush(COLOR_BTNFACE));
                    }
                }
            }
            else
            {
                SetTextColor(hdc, node->s.DrawForeColor);
                SetDCBrushColor(hdc, node->s.DrawBackColor);
                FillRect(hdc, &rowRect, PhGetStockBrush(DC_BRUSH));
            }
        }

        if (!Context->CustomColors && Context->ThemeHasItemBackground)
        {
            INT stateId;

            // Themed background

            if (node->Selected)
            {
                if (i == Context->HotNodeIndex)
                    stateId = TREIS_HOTSELECTED;
                else if (!Context->HasFocus)
                    stateId = TREIS_SELECTEDNOTFOCUS;
                else
                    stateId = TREIS_SELECTED;
            }
            else
            {
                if (i == Context->HotNodeIndex)
                    stateId = TREIS_HOT;
                else
                    stateId = INT_MAX;
            }

            // Themed background

            if (stateId != INT_MAX)
            {
                if (!Context->FixedColumnVisible)
                {
                    rowRect.left = Context->NormalLeft - hScrollPosition;
                }

                PhDrawThemeBackground(
                    Context->ThemeData,
                    hdc,
                    TVP_TREEITEM,
                    stateId,
                    &rowRect,
                    PaintRect
                    );
            }
        }

        // Paint the fixed column.

        cellRect.top = rowRect.top;
        cellRect.bottom = rowRect.bottom;

        if (fixedUpdate)
        {
            cellRect.left = 0;
            cellRect.right = Context->FixedWidth;
            PhTnpDrawCell(Context, hdc, &cellRect, node, Context->FixedColumn, i, -1);
        }

        // Paint the normal columns.

        if (normalUpdateLeftX < normalUpdateRightX)
        {
            cellRect.left = normalUpdateLeftX;
            cellRect.right = cellRect.left;

            // GetClipRgn overwrites the region contents, so reuse a cached one instead of
            // allocating a new region for every row. (dmex)
            oldClipRegion = PhGetScratchRegion(&Context->ClipScratchRegion);
            hasOldClipRegion = oldClipRegion && GetClipRgn(hdc, oldClipRegion) == 1;

            IntersectClipRect(hdc, Context->NormalLeft, cellRect.top, viewRect.right, cellRect.bottom);

            for (j = normalUpdateLeftIndex; j <= normalUpdateRightIndex; j++)
            {
                column = Context->ColumnsByDisplay[j];

                cellRect.left = cellRect.right;
                cellRect.right = cellRect.left + column->Width;
                PhTnpDrawCell(Context, hdc, &cellRect, node, column, i, j);
            }

            // The cached region stays alive for the next row; only the clip selection is restored.
            SelectClipRgn(hdc, hasOldClipRegion ? oldClipRegion : NULL);
        }

        // Paint the accent bar for selected rows. This is done last so the cells above don't
        // paint over it. (dmex)

        if (selectionBarVisible && node->Selected)
        {
            RECT selectionBarRect;

            selectionBarRect.left = selectionBarLeft;
            selectionBarRect.top = rowRect.top;
            selectionBarRect.right = selectionBarLeft + selectionBarWidth;
            selectionBarRect.bottom = rowRect.bottom;

            SetDCBrushColor(hdc, selectionBarColor);
            FillRect(hdc, &selectionBarRect, PhGetStockBrush(DC_BRUSH));
        }

        rowRect.top += Context->RowHeight;
        rowRect.bottom += Context->RowHeight;
    }

    if (lastRowToUpdate == Context->FlatList->Count - 1) // works even if there are no items
    {
        // Fill the rest of the space on the bottom with the window color.
        rowRect.bottom = viewRect.bottom;

        if (Context->ThemeSupport)
        {
            SetTextColor(hdc, PhThemeWindowTextColor);
            FillRect(hdc, &rowRect, PhThemeWindowBackgroundBrush);
        }
        else
        {
            FillRect(hdc, &rowRect, (HBRUSH)(COLOR_WINDOW + 1));
        }
    }

    if (normalTotalX < viewRect.right && viewRect.right > PaintRect->left && normalTotalX < PaintRect->right)
    {
        // Fill the rest of the space on the right with the window color.
        rowRect.left = normalTotalX;
        rowRect.top = Context->HeaderHeight;
        rowRect.right = viewRect.right;
        rowRect.bottom = viewRect.bottom;

        if (Context->ThemeSupport)
        {
            SetTextColor(hdc, PhThemeWindowTextColor);
            FillRect(hdc, &rowRect, PhThemeWindowBackgroundBrush);
        }
        else
        {
            FillRect(hdc, &rowRect, (HBRUSH)(COLOR_WINDOW + 1));
        }
    }

    if (Context->FlatList->Count == 0 && Context->EmptyText.Length != 0)
    {
        RECT textRect;

        textRect.left = PhScaleToDisplay(20, Context->WindowDpi);
        textRect.top = Context->HeaderHeight + PhScaleToDisplay(10, Context->WindowDpi);
        textRect.right = viewRect.right - PhScaleToDisplay(20, Context->WindowDpi);
        textRect.bottom = viewRect.bottom - Context->HeaderTextPadding;

        if (Context->ThemeSupport)
            SetTextColor(hdc, PhThemeWindowTextColor);
        else
            SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));

        DrawText(
            hdc,
            Context->EmptyText.Buffer,
            (ULONG)Context->EmptyText.Length / 2,
            &textRect,
            DT_NOPREFIX | DT_CENTER | DT_END_ELLIPSIS
            );
    }

    if (Context->FixedDividerVisible && Context->FixedWidth >= PaintRect->left && Context->FixedWidth < PaintRect->right)
    {
        PhTnpDrawDivider(Context, hdc);
    }

    if (Context->DragSelectionActive)
    {
        PhTnpDrawSelectionRectangle(Context, hdc, &Context->DragRect);
    }

    if (FlagOn(Context->Style, TN_STYLE_DRAG_REORDER_ROWS))
    {
        if (Context->ReorderDragActive)
        {
            PhTnpDrawInsertionCaret(Context, hdc);
        }
    }

    if (Context->HeaderCustomDraw && Context->HeaderInvalidatePending)
    {
        Context->HeaderInvalidatePending = FALSE;
        if (Context->HeaderHandle && !Context->Tracking) // GetCapture() != Context->HeaderHandle)
        {
            InvalidateRect(Context->HeaderHandle, NULL, FALSE);
        }
    }
}

/**
 * Prepares a tree new node for drawing.
 *
 * This function prepares the specified tree new node for drawing by performing
 * necessary calculations or setups using the provided device context and context.
 *
 * \param Context A pointer to the tree new context.
 * \param hdc The handle to the device context used for drawing.
 * \param Node A pointer to the tree new node to prepare for drawing.
 */
VOID PhTnpPrepareRowForDraw(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc,
    _Inout_ PPH_TREENEW_NODE Node
    )
{
    if (!Node->s.CachedColorValid)
    {
        PH_TREENEW_GET_NODE_COLOR getNodeColor;

        getNodeColor.Flags = 0;
        getNodeColor.Node = Node;
        getNodeColor.BackColor = Context->DefaultBackColor;
        getNodeColor.ForeColor = Context->DefaultForeColor;

        if (Context->Callback(
            Context->Handle,
            TreeNewGetNodeColor,
            &getNodeColor,
            NULL,
            Context->CallbackContext
            ))
        {
            Node->BackColor = getNodeColor.BackColor;
            Node->ForeColor = getNodeColor.ForeColor;
            Node->UseAutoForeColor = !!(getNodeColor.Flags & TN_AUTO_FORECOLOR);

            if (getNodeColor.Flags & TN_CACHE)
                Node->s.CachedColorValid = TRUE;
        }
        else
        {
            Node->BackColor = getNodeColor.BackColor;
            Node->ForeColor = getNodeColor.ForeColor;
        }
    }

    Node->s.DrawForeColor = Node->ForeColor;

    if (Node->UseTempBackColor)
        Node->s.DrawBackColor = Node->TempBackColor;
    else
        Node->s.DrawBackColor = Node->BackColor;

    if (!Node->s.CachedFontValid)
    {
        PH_TREENEW_GET_NODE_FONT getNodeFont;

        getNodeFont.Flags = 0;
        getNodeFont.Node = Node;
        getNodeFont.Font = NULL;

        if (Context->Callback(
            Context->Handle,
            TreeNewGetNodeFont,
            &getNodeFont,
            NULL,
            Context->CallbackContext
            ))
        {
            Node->Font = getNodeFont.Font;

            if (getNodeFont.Flags & TN_CACHE)
                Node->s.CachedFontValid = TRUE;
        }
        else
        {
            Node->Font = NULL;
        }
    }

    if (!Node->s.CachedIconValid)
    {
        PH_TREENEW_GET_NODE_ICON getNodeIcon;

        getNodeIcon.Flags = 0;
        getNodeIcon.Node = Node;
        getNodeIcon.Icon = NULL;

        if (Context->Callback(
            Context->Handle,
            TreeNewGetNodeIcon,
            &getNodeIcon,
            NULL,
            Context->CallbackContext
            ))
        {
            Node->Icon = getNodeIcon.Icon;

            if (getNodeIcon.Flags & TN_CACHE)
                Node->s.CachedIconValid = TRUE;
        }
        else
        {
            Node->Icon = NULL;
        }
    }

    if (Node->UseAutoForeColor || Node->UseTempBackColor)
    {
        if (PhGetColorBrightness(Node->s.DrawBackColor) > 100) // slightly less than half
            Node->s.DrawForeColor = RGB(0x00, 0x00, 0x00);
        else
            Node->s.DrawForeColor = RGB(0xff, 0xff, 0xff);
    }
}

/**
 * Draws a cell in the tree new control.
 *
 * This function renders the content of a specific cell within the tree new control,
 * including text, icons, and other visual elements based on the provided node and column.
 *
 * \param Context A pointer to the PPH_TREENEW_CONTEXT structure containing the tree new context.
 * \param hdc A handle to the device context used for drawing operations.
 * \param CellRect A pointer to a RECT structure defining the boundaries of the cell to draw.
 * \param Node A pointer to the PPH_TREENEW_NODE structure representing the node associated with the cell.
 * \param Column A pointer to the PPH_TREENEW_COLUMN structure representing the column.
 * \param RowIndex The index of the row containing the cell.
 * \param ColumnIndex The index of the column containing the cell.
 */
VOID PhTnpDrawCell(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc,
    _In_ PRECT CellRect,
    _In_ PPH_TREENEW_NODE Node,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ LONG RowIndex,
    _In_ LONG ColumnIndex
    )
{
    HFONT font; // font to use
    HFONT oldFont = NULL;
    PH_STRINGREF text; // text to draw
    RECT textRect; // working rectangle, modified as needed
    ULONG textFlags; // DT_* flags
    LONG iconVerticalMargin; // top/bottom margin for icons (determined using height of small icon)
    LONG width;
    LONG height;

    font = Node->Font;
    textFlags = Column->TextFlags;

    if (Column->Alignment & PH_ALIGN_MONOSPACE_FONT)
    {
        font = PhMonospaceFont;
    }

    textRect = *CellRect;

    width = Context->SmallIconWidth;
    height = Context->SmallIconHeight;

    // Initial margins used by default list view
    textRect.left += Context->CellMarginLeft;
    textRect.right -= Context->CellMarginRight;

    // icon margin = (height of row - height of small icon) / 2
    iconVerticalMargin = ((textRect.bottom - textRect.top) - height) / 2;

    textRect.top += iconVerticalMargin;
    textRect.bottom -= iconVerticalMargin;

    if (Column == Context->FirstColumn)
    {
        BOOLEAN needsClip = FALSE;
        INT savedDcState = 0;

        textRect.left += Node->Level * width;

        // The icon may need to be clipped if the column is too small.
        needsClip = Column->Width < textRect.left + (Context->CanAnyExpand ? width : 0) + (Node->Icon ? width : 0);

        if (needsClip)
        {
            // Snapshot the clip state with SaveDC instead of allocating a scratch
            // region per cell. This nests correctly inside the row loop. (dmex)
            savedDcState = SaveDC(hdc);

            // Clip contents to the column.
            IntersectClipRect(hdc, CellRect->left, textRect.top, CellRect->right, textRect.bottom);
        }

        if (Context->CanAnyExpand) // flag is used so we can avoid indenting when it's a flat list
        {
            BOOLEAN drewUsingTheme = FALSE;
            RECT themeRect;

            if (!Node->s.IsLeaf)
            {
                // Draw the plus/minus glyph.

                themeRect.left = textRect.left;
                themeRect.right = themeRect.left + width;
                //themeRect.left = textRect.right;
                //themeRect.right = textRect.right - SmallIconWidth;
                themeRect.top = textRect.top;
                themeRect.bottom = themeRect.top + height;

                if (Context->ThemeHasGlyph)
                {
                    INT partId;
                    INT stateId;

                    partId = (RowIndex == Context->HotNodeIndex && Node->s.PlusMinusHot && Context->ThemeHasHotGlyph) ? TVP_HOTGLYPH : TVP_GLYPH;
                    stateId = Node->Expanded ? GLPS_OPENED : GLPS_CLOSED;

                    if (PhDrawThemeBackground(
                        Context->ThemeData,
                        hdc,
                        partId,
                        stateId,
                        &themeRect,
                        NULL
                        ))
                    {
                        drewUsingTheme = TRUE;
                    }
                }

                if (!drewUsingTheme)
                {
                    ULONG glyphWidth;
                    ULONG glyphHeight;
                    RECT glyphRect;

                    glyphWidth = width / 2;
                    glyphHeight = height / 2;

                    glyphRect.left = textRect.left + (width - glyphWidth) / 2;
                    glyphRect.right = glyphRect.left + glyphWidth;
                    //glyphRect.left = textRect.right + (SmallIconWidth - glyphWidth) / 2;
                    //glyphRect.right = glyphRect.left - glyphWidth;
                    glyphRect.top = textRect.top + (height - glyphHeight) / 2;
                    glyphRect.bottom = glyphRect.top + glyphHeight;

                    PhTnpDrawPlusMinusGlyph(Context, hdc, &glyphRect, !Node->Expanded);
                }
            }

            textRect.left += width;
        }

        // Draw the icon.

        // Note: in image list mode the icon column is reserved for every row, including rows
        // without an icon (they draw image zero), so the text stays aligned. (dmex)
        if (Context->ImageListSupport)
        {
            //LONG right = textRect.right;
            //textRect.right = textRect.left + SmallIconWidth;
            //FillRect(hdc, &textRect, GetSysColorBrush(COLOR_HOTLIGHT));
            //textRect.right = right;

            PhImageListDrawEx(
                Context->ImageListHandle,
                (ULONG)(ULONG_PTR)Node->Icon, // HACK (dmex)
                hdc,
                textRect.left,
                textRect.top,
                width,
                height,
                CLR_DEFAULT,
                CLR_NONE,
                ILD_NORMAL | ILD_TRANSPARENT,
                ILS_NORMAL
                );

            textRect.left += width + Context->IconRightPadding;
        }
        else if (Node->Icon)
        {
            DrawIconEx(
                hdc,
                textRect.left,
                textRect.top,
                Node->Icon,
                width,
                height,
                0,
                NULL,
                DI_NORMAL
                );

            textRect.left += width + Context->IconRightPadding;
        }

        if (needsClip)
        {
            if (savedDcState)
                RestoreDC(hdc, savedDcState);
            else
                SelectClipRgn(hdc, NULL);
        }

        if (textRect.left > textRect.right)
            textRect.left = textRect.right;
    }

    if (Column->CustomDraw)
    {
        BOOLEAN result;
        PH_TREENEW_CUSTOM_DRAW customDraw = { 0 };
        INT savedDc;

        customDraw.Node = Node;
        customDraw.Column = Column;
        customDraw.Dc = hdc;
        customDraw.CellRect = *CellRect;
        customDraw.TextRect = textRect;
        customDraw.WindowDpi = Context->WindowDpi;

        // Fix up the rectangles before giving them to the user.
        if (customDraw.CellRect.left > customDraw.CellRect.right)
            customDraw.CellRect.left = customDraw.CellRect.right;
        if (customDraw.TextRect.left > customDraw.TextRect.right)
            customDraw.TextRect.left = customDraw.TextRect.right;

        savedDc = SaveDC(hdc);
        if (savedDc)
        {
            result = Context->Callback(Context->Handle, TreeNewCustomDraw, &customDraw, NULL, Context->CallbackContext);
            RestoreDC(hdc, savedDc);
            if (result)
                return;
        }
        // If DC state cannot be isolated, use the normal text fallback instead
        // of exposing the remaining cells to a callback's modified DC state.
    }

    if (PhTnpGetCellText(Context, Node, Column->Id, &text))
    {
        if (!(textFlags & (DT_PATH_ELLIPSIS | DT_WORD_ELLIPSIS)))
            textFlags |= DT_END_ELLIPSIS;

        textFlags |= DT_NOPREFIX | DT_VCENTER | DT_SINGLELINE;

        textRect.top = CellRect->top;
        textRect.bottom = CellRect->bottom;

        if (font)
            oldFont = SelectFont(hdc, font);

        DrawText(
            hdc,
            text.Buffer,
            (ULONG)text.Length / 2,
            &textRect,
            textFlags
            );

        if (oldFont)
            SelectFont(hdc, oldFont);
    }
}

/**
 * Draws the divider line between the fixed and normal columns.
 *
 * \param Context Pointer to the treenew context structure.
 * \param hdc Device context handle.
 */
VOID PhTnpDrawDivider(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc
    )
{
    POINT points[2];
    HPEN oldPen;
    COLORREF dividerColor;

    // The palette colors only apply when the control carries our theme. (dmex)
    dividerColor = Context->ThemeSupport ? PhThemeWindowHighlight2Color : GetSysColor(COLOR_3DSHADOW);

    if (Context->AnimateDivider)
    {
        if (Context->DividerHot == 0 && !Context->HScrollVisible)
            return; // divider is invisible

        if (Context->DividerHot < 100)
        {
            // We need to draw and alpha blend the divider. The scratch bitmap is a single pixel
            // of the divider color, stretched over the divider rectangle by the blend. (dmex)

            if (PhTnpSelectionCreateBufferedContext(Context))
            {
                HBITMAP oldBitmap;
                RECT tempRect;
                BLENDFUNCTION blendFunction;

                oldBitmap = SelectBitmap(Context->SelectionScratchDc, Context->SelectionScratchBitmap);
                tempRect.left = 0;
                tempRect.top = 0;
                tempRect.right = 1;
                tempRect.bottom = 1;
                SetDCBrushColor(Context->SelectionScratchDc, dividerColor);
                FillRect(Context->SelectionScratchDc, &tempRect, PhGetStockBrush(DC_BRUSH));

                blendFunction.BlendOp = AC_SRC_OVER;
                blendFunction.BlendFlags = 0;
                blendFunction.AlphaFormat = 0;

                // If the horizontal scroll bar is visible, we need to display a line even if the
                // divider is not hot. In this case we increase the base alpha value.
                if (!Context->HScrollVisible)
                    blendFunction.SourceConstantAlpha = (UCHAR)(Context->DividerHot * 255 / 100);
                else
                    blendFunction.SourceConstantAlpha = 55 + (UCHAR)(Context->DividerHot * 2);

                GdiAlphaBlend(
                    hdc,
                    Context->FixedWidth,
                    Context->HeaderHeight,
                    1,
                    Context->ClientRect.bottom - Context->HeaderHeight,
                    Context->SelectionScratchDc,
                    0,
                    0,
                    1,
                    1,
                    blendFunction
                    );

                SelectBitmap(Context->SelectionScratchDc, oldBitmap);

                return;
            }
        }
    }

    points[0].x = Context->FixedWidth;
    points[0].y = Context->HeaderHeight;
    points[1].x = Context->FixedWidth;
    points[1].y = Context->ClientRect.bottom;
    SetDCPenColor(hdc, dividerColor);
    oldPen = SelectPen(hdc, PhGetStockPen(DC_PEN));
    Polyline(hdc, points, 2);

    if (oldPen)
        SelectPen(hdc, oldPen);
}

/**
 * Draws the plus/minus glyph for expanding/collapsing tree nodes.
 *
 * \param hdc Device context handle.
 * \param Rect Pointer to the bounding rectangle for the glyph.
 * \param Plus TRUE to draw a plus sign, FALSE to draw a minus sign.
 */
VOID PhTnpDrawPlusMinusGlyph(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc,
    _In_ PRECT Rect,
    _In_ BOOLEAN Plus
    )
{
    INT savedDc;
    ULONG width;
    ULONG height;
    POINT points[2];
    COLORREF borderColor;
    COLORREF backColor;
    COLORREF textColor;

    // The palette colors only apply when the control carries our theme. (dmex)
    if (Context->ThemeSupport)
    {
        borderColor = PhThemeWindowBorderColor;
        backColor = PhThemeWindowBackgroundColor;
        textColor = PhThemeWindowTextColor;
    }
    else
    {
        borderColor = GetSysColor(COLOR_3DSHADOW);
        backColor = GetSysColor(COLOR_WINDOW);
        textColor = GetSysColor(COLOR_WINDOWTEXT);
    }

    savedDc = SaveDC(hdc);

    SelectPen(hdc, PhGetStockPen(DC_PEN));
    SetDCPenColor(hdc, borderColor);
    SelectBrush(hdc, PhGetStockBrush(DC_BRUSH));
    SetDCBrushColor(hdc, backColor);

    width = Rect->right - Rect->left;
    height = Rect->bottom - Rect->top;

    // Draw the rectangle.
    Rectangle(hdc, Rect->left, Rect->top, Rect->right + 1, Rect->bottom + 1);

    SetDCPenColor(hdc, textColor);

    // Draw the horizontal line.
    points[0].x = Rect->left + 2;
    points[0].y = Rect->top + height / 2;
    points[1].x = Rect->right - 2 + 1;
    points[1].y = points[0].y;
    Polyline(hdc, points, 2);

    if (Plus)
    {
        // Draw the vertical line.
        points[0].x = Rect->left + width / 2;
        points[0].y = Rect->top + 2;
        points[1].x = points[0].x;
        points[1].y = Rect->bottom - 2 + 1;
        Polyline(hdc, points, 2);
    }

    RestoreDC(hdc, savedDc);
}

/**
 * Draws the selection rectangle around selected items.
 *
 * \param Context Pointer to the treenew context structure.
 * \param hdc Device context handle.
 * \param Rect Pointer to the rectangle to draw.
 */
VOID PhTnpDrawSelectionRectangle(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc,
    _In_ PRECT Rect
    )
{
    RECT rect;
    BOOLEAN drewWithAlpha;

    rect = *Rect;

    // MSDN says FrameRect/DrawFocusRect doesn't draw anything if bottom <= top or right <= left.
    // That's complete rubbish.
    if (rect.right - rect.left == 0 || rect.bottom - rect.top == 0)
        return;

    drewWithAlpha = FALSE;

    if (Context->SelectionRectangleAlpha)
    {
        if (PhTnpSelectionCreateBufferedContext(Context))
        {
            HBITMAP oldBitmap;
            RECT tempRect;
            BLENDFUNCTION blendFunction;

            // Draw the outline of the selection rectangle.
            FrameRect(hdc, &rect, GetSysColorBrush(COLOR_HIGHLIGHT));

            // Fill in the selection rectangle.
            oldBitmap = SelectBitmap(Context->SelectionScratchDc, Context->SelectionScratchBitmap);
            tempRect.left = 0;
            tempRect.top = 0;
            tempRect.right = 1;
            tempRect.bottom = 1;
            FillRect(Context->SelectionScratchDc, &tempRect, (HBRUSH)(COLOR_HOTLIGHT + 1));

            blendFunction.BlendOp = AC_SRC_OVER;
            blendFunction.BlendFlags = 0;
            blendFunction.SourceConstantAlpha = 70;
            blendFunction.AlphaFormat = 0;

            GdiAlphaBlend(
                hdc,
                rect.left,
                rect.top,
                rect.right - rect.left,
                rect.bottom - rect.top,
                Context->SelectionScratchDc,
                0,
                0,
                1,
                1,
                blendFunction
                );

            drewWithAlpha = TRUE;

            SelectBitmap(Context->SelectionScratchDc, oldBitmap);
        }
    }

    if (!drewWithAlpha)
    {
        DrawFocusRect(hdc, &rect);
    }
}

/**
 * Draws the themed border around the control.
 *
 * \param Context Pointer to the treenew context structure.
 * \param hdc Device context handle.
 */
VOID PhTnpDrawThemedBorder(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc
    )
{
    RECT windowRect;
    RECT clientRect;
    LONG sizingBorderWidth;
    LONG borderX;
    LONG borderY;

    if (!PhGetWindowRect(Context->Handle, &windowRect))
        return;

    windowRect.right -= windowRect.left;
    windowRect.bottom -= windowRect.top;
    windowRect.left = 0;
    windowRect.top = 0;

    clientRect.left = windowRect.left + Context->SystemEdgeX;
    clientRect.top = windowRect.top + Context->SystemEdgeY;
    clientRect.right = windowRect.right - Context->SystemEdgeX;
    clientRect.bottom = windowRect.bottom - Context->SystemEdgeY;

    // Make sure we don't paint in the client area.
    ExcludeClipRect(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

    if (Context->ThemeSupport)
    {
        // The visual style border is drawn from the system palette and stays light while the
        // control is dark, so use the theme border color instead. (dmex)
        SetDCBrushColor(hdc, PhThemeWindowBorderColor);
        FillRect(hdc, &windowRect, PhGetStockBrush(DC_BRUSH));
        return;
    }

    if (!Context->ThemeData)
        return;

    // Draw the themed border.
    PhDrawThemeBackground(Context->ThemeData, hdc, 0, 0, &windowRect, NULL);

    // Calculate the size of the border we just drew, and fill in the rest of the space if we didn't
    // fully paint the region.

    if (PhGetThemeInt(Context->ThemeData, 0, 0, TMT_SIZINGBORDERWIDTH, &sizingBorderWidth))
    {
        borderX = sizingBorderWidth;
        borderY = sizingBorderWidth;
    }
    else
    {
        borderX = Context->SystemBorderX;
        borderY = Context->SystemBorderY;
    }

    if (borderX < Context->SystemEdgeX || borderY < Context->SystemEdgeY)
    {
        windowRect.left += Context->SystemEdgeX - borderX;
        windowRect.top += Context->SystemEdgeY - borderY;
        windowRect.right -= Context->SystemEdgeX - borderX;
        windowRect.bottom -= Context->SystemEdgeY - borderY;
        FillRect(hdc, &windowRect, (HBRUSH)(COLOR_WINDOW + 1));
    }
}

/**
 * Hooks the header control window procedures.
 *
 * The hook owns the custom header painting, hot tracking and the mouse messages forwarded to the
 * tooltip control, so it must be installed even when there are no tooltips. (dmex)
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpInitializeHeaders(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    // TnHeaderCustomPaint also runs for themed trees without TN_STYLE_CUSTOM_HEADERDRAW,
    // so the hot column must always start out as "none": zero is a valid column id and
    // would leave the first column stuck in the hot state.
    Context->HeaderHotColumn = ULONG_MAX;

    if (Context->HeaderCustomDraw)
    {
        Context->HeaderThemeHandle = PhOpenThemeData(Context->HeaderHandle, VSCLASS_HEADER, Context->WindowDpi);
    }

    Context->HeaderWindowProc = PhGetWindowProcedure(Context->HeaderHandle);
    PhSetWindowContext(Context->HeaderHandle, MAXCHAR, Context);
    PhSetWindowProcedure(Context->HeaderHandle, PhTnpHeaderHookWndProc);

    Context->FixedHeaderWindowProc = PhGetWindowProcedure(Context->FixedHeaderHandle);
    PhSetWindowContext(Context->FixedHeaderHandle, MAXCHAR, Context);
    PhSetWindowProcedure(Context->FixedHeaderHandle, PhTnpHeaderHookWndProc);
}

/**
 * Initializes the tooltip control for the treenew control.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpInitializeTooltips(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    TOOLINFO toolInfo;

    Context->TooltipsHandle = PhCreateWindowEx(
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_NOANIMATE | TTS_NOFADE | TTS_NOPREFIX | TTS_ALWAYSTIP,
        WS_EX_TOPMOST | WS_EX_TRANSPARENT, // solves double-click problem (wj32)
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        NULL,
        NULL
        );

    if (!Context->TooltipsHandle)
        return;

    // Item tooltips
    memset(&toolInfo, 0, sizeof(TOOLINFO));
    toolInfo.cbSize = sizeof(TOOLINFO);
    toolInfo.uFlags = TTF_TRANSPARENT;
    toolInfo.hwnd = Context->Handle;
    toolInfo.uId = TNP_TOOLTIPS_ITEM;
    toolInfo.lpszText = LPSTR_TEXTCALLBACK;
    toolInfo.lParam = TNP_TOOLTIPS_ITEM;
    SendMessage(Context->TooltipsHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

    // Fixed column tooltips
    toolInfo.uFlags = 0;
    toolInfo.hwnd = Context->FixedHeaderHandle;
    toolInfo.uId = TNP_TOOLTIPS_FIXED_HEADER;
    toolInfo.lpszText = LPSTR_TEXTCALLBACK;
    toolInfo.lParam = TNP_TOOLTIPS_FIXED_HEADER;
    SendMessage(Context->TooltipsHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

    // Normal column tooltips
    toolInfo.uFlags = 0;
    toolInfo.hwnd = Context->HeaderHandle;
    toolInfo.uId = TNP_TOOLTIPS_HEADER;
    toolInfo.lpszText = LPSTR_TEXTCALLBACK;
    toolInfo.lParam = TNP_TOOLTIPS_HEADER;
    SendMessage(Context->TooltipsHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

    SetWindowPos(
        Context->TooltipsHandle,
        HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
        );

    SendMessage(Context->TooltipsHandle, TTM_SETMAXTIPWIDTH, 0, MAXSHORT); // no limit
    SetWindowFont(Context->TooltipsHandle, Context->Font, FALSE);
    Context->TooltipFont = Context->Font;
}

/**
 * Retrieves the tooltip text for a cell at a given point.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Point Pointer to the point to test.
 * \param Text Pointer to receive the tooltip text.
 * \return TRUE if tooltip text was retrieved, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhTnpGetTooltipText(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPOINT Point,
    _Out_ PPH_STRING *Text
    )
{
    PH_TREENEW_HIT_TEST hitTest;
    BOOLEAN unfoldingTooltip;
    BOOLEAN unfoldingTooltipFromViewCancelled;
    PH_TREENEW_CELL_PARTS parts;
    LONG viewRight;
    PH_TREENEW_GET_CELL_TOOLTIP getCellTooltip;

    hitTest.Point = *Point;
    hitTest.InFlags = TN_TEST_COLUMN | TN_TEST_SUBITEM;
    PhTnpHitTest(Context, &hitTest);

    if (Context->DragSelectionActive)
        return FALSE;
    if (!(hitTest.Flags & TN_HIT_ITEM))
        return FALSE;
    if (hitTest.Flags & (TN_HIT_ITEM_PLUSMINUS | TN_HIT_DIVIDER))
        return FALSE;
    if (!hitTest.Column)
        return FALSE;

    if (Context->TooltipIndex != hitTest.Node->Index || Context->TooltipId != hitTest.Column->Id)
    {
        Context->TooltipIndex = hitTest.Node->Index;
        Context->TooltipId = hitTest.Column->Id;

        getCellTooltip.Flags = 0;
        getCellTooltip.Node = hitTest.Node;
        getCellTooltip.Column = hitTest.Column;
        getCellTooltip.Unfolding = FALSE;
        PhInitializeEmptyStringRef(&getCellTooltip.Text);
        getCellTooltip.Font = Context->Font;
        getCellTooltip.MaximumWidth = ULONG_MAX;

        unfoldingTooltip = FALSE;
        unfoldingTooltipFromViewCancelled = FALSE;

        if (!(Context->ExtendedFlags & TN_FLAG_NO_UNFOLDING_TOOLTIPS) &&
            PhTnpGetCellParts(Context, hitTest.Node->Index, hitTest.Column, TN_MEASURE_TEXT, &parts) &&
            (parts.Flags & TN_PART_CONTENT) && (parts.Flags & TN_PART_TEXT))
        {
            viewRight = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);

            // Use an unfolding tooltip if the text was truncated within the column, or the text
            // extends beyond the view area in either direction.

            if (parts.TextRect.left < parts.ContentRect.left || parts.TextRect.right > parts.ContentRect.right)
            {
                unfoldingTooltip = TRUE;
            }
            else if ((!hitTest.Column->Fixed && parts.TextRect.left < Context->NormalLeft) || parts.TextRect.right > viewRight)
            {
                // Only show view-based unfolding tooltips if the mouse is over the text itself.
                if (Point->x >= parts.TextRect.left && Point->x < parts.TextRect.right)
                    unfoldingTooltip = TRUE;
                else
                    unfoldingTooltipFromViewCancelled = TRUE;
            }

            if (unfoldingTooltip)
            {
                getCellTooltip.Unfolding = TRUE;
                getCellTooltip.Text = parts.Text;
                getCellTooltip.Font = parts.Font; // try to use the same font as the cell

                Context->TooltipRect = parts.TextRect;
            }
        }

        Context->Callback(Context->Handle, TreeNewGetCellTooltip, &getCellTooltip, NULL, Context->CallbackContext);

        Context->TooltipUnfolding = getCellTooltip.Unfolding;

        if (getCellTooltip.Text.Buffer && getCellTooltip.Text.Length != 0)
        {
            PhMoveReference(&Context->TooltipText, PhCreateString2(&getCellTooltip.Text));
        }
        else
        {
            PhClearReference(&Context->TooltipText);

            if (unfoldingTooltipFromViewCancelled)
            {
                // We may need to show the view-based unfolding tooltip if the mouse moves over the
                // text in the future. Reset the index and ID to make sure we keep checking.
                Context->TooltipIndex = ULONG_MAX;
                Context->TooltipId = ULONG_MAX;
            }
        }

        Context->NewTooltipFont = getCellTooltip.Font;

        if (!Context->NewTooltipFont)
            Context->NewTooltipFont = Context->Font;

        if (getCellTooltip.MaximumWidth <= MAXSHORT) // seems to be the maximum value that the tooltip control supports
            SendMessage(Context->TooltipsHandle, TTM_SETMAXTIPWIDTH, 0, getCellTooltip.MaximumWidth);
        else
            SendMessage(Context->TooltipsHandle, TTM_SETMAXTIPWIDTH, 0, MAXSHORT);
    }

    if (Context->TooltipText)
    {
        *Text = Context->TooltipText;
        return TRUE;
    }

    return FALSE;
}

/**
 * Prepares to show the tooltip.
 *
 * \param Context Pointer to the treenew context structure.
 * \return TRUE if the tooltip should be shown, FALSE otherwise.
 */
BOOLEAN PhTnpPrepareTooltipShow(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    RECT rect;

    if (Context->TooltipFont != Context->NewTooltipFont)
    {
        Context->TooltipFont = Context->NewTooltipFont;
        SetWindowFont(Context->TooltipsHandle, Context->TooltipFont, FALSE);
    }

    if (!Context->TooltipUnfolding)
    {
        SetWindowPos(
            Context->TooltipsHandle,
            NULL,
            0,
            0,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE
            );

        return FALSE;
    }

    rect = Context->TooltipRect;
    SendMessage(Context->TooltipsHandle, TTM_ADJUSTRECT, TRUE, (LPARAM)&rect);
    MapWindowRect(Context->Handle, NULL, &rect);
    SetWindowPos(
        Context->TooltipsHandle,
        NULL,
        rect.left,
        rect.top,
        0,
        0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER
        );

    return TRUE;
}

VOID PhTnpPrepareTooltipPop(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    Context->TooltipIndex = ULONG_MAX;
    Context->TooltipId = ULONG_MAX;
    Context->TooltipColumnId = ULONG_MAX;
}

/**
 * Hides the tooltip.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpPopTooltip(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    if (Context->TooltipsHandle)
    {
        SendMessage(Context->TooltipsHandle, TTM_POP, 0, 0);
        PhTnpPrepareTooltipPop(Context);
    }
}

/**
 * Performs hit testing on the header control.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Fixed TRUE to test the fixed header, FALSE for the normal header.
 * \param Point Pointer to the point to test.
 * \param ItemRect Pointer to receive the item rectangle, or NULL.
 * \return Pointer to the column under the point, or NULL.
 */
PPH_TREENEW_COLUMN PhTnpHitTestHeader(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ BOOLEAN Fixed,
    _In_ PPOINT Point,
    _Out_opt_ PRECT ItemRect
    )
{
    PPH_TREENEW_COLUMN column;
    RECT itemRect;

    if (ItemRect)
    {
        memset(ItemRect, 0, sizeof(RECT));
    }

    if (Fixed)
    {
        if (!Context->FixedColumnVisible)
            return NULL;

        column = Context->FixedColumn;

        if (!Header_GetItemRect(Context->FixedHeaderHandle, 0, &itemRect))
            return NULL;
    }
    else
    {
        HDHITTESTINFO hitTestInfo;

        hitTestInfo.pt = *Point;
        hitTestInfo.flags = 0;
        hitTestInfo.iItem = INT_ERROR;

        if (SendMessage(Context->HeaderHandle, HDM_HITTEST, 0, (LPARAM)&hitTestInfo) != INT_ERROR && hitTestInfo.iItem != INT_ERROR)
        {
            HDITEM item;

            item.mask = HDI_LPARAM;

            if (!Header_GetItem(Context->HeaderHandle, hitTestInfo.iItem, &item))
                return NULL;

            column = (PPH_TREENEW_COLUMN)item.lParam;

            if (!Header_GetItemRect(Context->HeaderHandle, hitTestInfo.iItem, &itemRect))
                return NULL;
        }
        else
        {
            return NULL;
        }
    }

    if (ItemRect)
        *ItemRect = itemRect;

    return column;
}

/**
 * Retrieves the tooltip text for a header column.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Fixed TRUE if this is the fixed header, FALSE for normal header.
 * \param Point Pointer to the point under the cursor.
 * \param Text Pointer to receive the tooltip text.
 * \return TRUE if tooltip text was retrieved, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhTnpGetHeaderTooltipText(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ BOOLEAN Fixed,
    _In_ PPOINT Point,
    _Out_ PPH_STRING *Text
    )
{
    LOGICAL result;
    PPH_TREENEW_COLUMN column;
    RECT itemRect;
    PCWSTR text;
    SIZE_T textCount;
    HFONT oldFont;
    HDC hdc;
    SIZE textSize;

    column = PhTnpHitTestHeader(Context, Fixed, Point, &itemRect);

    if (!column)
        return FALSE;

    if (Context->TooltipColumnId != column->Id)
    {
        // Determine if the tooltip needs to be shown.

        text = column->Text;
        textCount = PhCountStringZ(text);

        if (!(hdc = GetDC(Context->Handle)))
            return FALSE;

        oldFont = SelectFont(hdc, Context->Font);
        result = GetTextExtentPoint32(hdc, text, (ULONG)textCount, &textSize);
        if (oldFont) SelectFont(hdc, oldFont);
        ReleaseDC(Context->Handle, hdc);

        if (!result)
            return FALSE;

        if (textSize.cx + Context->TextMarginPadding <= itemRect.right - itemRect.left) // HACK: Magic values (same as our cell margins?)
            return FALSE;

        Context->TooltipColumnId = column->Id;
        PhMoveReference(&Context->TooltipText, PhCreateStringEx(text, textCount * sizeof(WCHAR)));
    }

    *Text = Context->TooltipText;

    // Always use the default parameters for column header tooltips.
    Context->NewTooltipFont = Context->Font;
    Context->TooltipUnfolding = FALSE;
    SendMessage(Context->TooltipsHandle, TTM_SETMAXTIPWIDTH, 0, TNP_TOOLTIPS_DEFAULT_MAXIMUM_WIDTH);

    return TRUE;
}

/**
 * Retrieves the text for a column header.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Column Pointer to the column.
 * \param Text Pointer to receive the header text as a string reference.
 * \return TRUE if text was retrieved, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhTnpGetColumnHeaderText(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_COLUMN Column,
    _Out_ PPH_STRINGREF Text
    )
{
    if (Column->Id > Context->HeaderColumnCacheMax)
        return FALSE;

    if (Context->HeaderStringCache && Context->HeaderStringCache[Column->Id].Length)
    {
        *Text = Context->HeaderStringCache[Column->Id];
        return TRUE;
    }

    if (Context->HeaderTextCache)
    {
        PH_TREENEW_GET_HEADER_TEXT getHeaderText;

        PhInitializeEmptyStringRef(&getHeaderText.Text);
        getHeaderText.Column = Column;
        getHeaderText.TextCache = (PWSTR)&((WCHAR(*)[PH_TREENEW_HEADER_TEXT_SIZE_MAX])Context->HeaderTextCache)[Column->Id]; // HACK (dmex)
        getHeaderText.TextCacheSize = PH_TREENEW_HEADER_TEXT_SIZE_MAX * sizeof(WCHAR);

        if (Context->Callback(
            Context->Handle,
            TreeNewGetHeaderText,
            &getHeaderText,
            NULL,
            Context->CallbackContext
            ) && getHeaderText.Text.Buffer)
        {
            *Text = getHeaderText.Text;

            if (Context->HeaderStringCache) // (getHeaderText.Flags & TN_CACHE)
            {
                Context->HeaderStringCache[Column->Id] = getHeaderText.Text;
            }

            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN TnHeaderCustomPaint(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LPNMCUSTOMDRAW CustomDraw
    )
{
    PPH_TREENEW_COLUMN column;

    // The native header paints itself with the light common-control class even when
    // the window is dark, so every themed tree is drawn here. HeaderCustomDraw only
    // adds the per-column totals text (TN_STYLE_CUSTOM_HEADERDRAW).
    if (!Context->HeaderCustomDraw && !Context->ThemeSupport)
        return FALSE;

    if (!(column = (PPH_TREENEW_COLUMN)CustomDraw->lItemlParam))
        return FALSE;

    // The header is rendered in one pass over every item, so cull the columns that fall outside the
    // update region instead of formatting text that is clipped away. (dmex)
    if (!RectVisible(CustomDraw->hdc, &CustomDraw->rc))
        return TRUE;

    if (!Context->HeaderThemeHandle)
        Context->HeaderThemeHandle = PhOpenThemeData(Context->HeaderHandle, VSCLASS_HEADER, Context->WindowDpi);

    SetBkMode(CustomDraw->hdc, TRANSPARENT);

    if (Context->HeaderHotColumn != ULONG_MAX && Context->HeaderHotColumn == column->Id)
    {
        if (Context->ThemeSupport)
        {
            HBRUSH oldBrush;

            SetDCBrushColor(CustomDraw->hdc, PhThemeWindowHighlight2Color);
            FillRect(CustomDraw->hdc, &CustomDraw->rc, PhGetStockBrush(DC_BRUSH));

            // PatBlt uses the brush selected into the device context, unlike FillRect. (dmex)
            oldBrush = SelectBrush(CustomDraw->hdc, PhGetStockBrush(DC_BRUSH));

            if (Context->HeaderDragging)
            {
                SetDCBrushColor(CustomDraw->hdc, RGB(0, 0, 229));
                PatBlt(CustomDraw->hdc, CustomDraw->rc.right - 2, CustomDraw->rc.top, 2, CustomDraw->rc.bottom - CustomDraw->rc.top, PATCOPY);
            }
            else
            {
                SetDCBrushColor(CustomDraw->hdc, PhThemeWindowBorderColor);
                PatBlt(CustomDraw->hdc, CustomDraw->rc.right - 1, CustomDraw->rc.top, 1, CustomDraw->rc.bottom - CustomDraw->rc.top, PATCOPY);
                //PatBlt(CustomDraw->hdc, CustomDraw->rc.left, CustomDraw->rc.bottom - 1, CustomDraw->rc.right - CustomDraw->rc.left, 1, PATCOPY);
            }

            if (oldBrush)
                SelectBrush(CustomDraw->hdc, oldBrush);
        }
        else
        {
            if (Context->HeaderThemeHandle)
            {
                INT state;

                if (CustomDraw->uItemState == (CDIS_SHOWKEYBOARDCUES | CDIS_SELECTED))
                    state = HIS_PRESSED;
                else
                    state = HIS_HOT;

                PhDrawThemeBackground(
                    Context->HeaderThemeHandle,
                    CustomDraw->hdc,
                    HP_HEADERITEM,
                    state,
                    &CustomDraw->rc,
                    NULL
                    );
            }
            else
            {
                FillRect(CustomDraw->hdc, &CustomDraw->rc, (HBRUSH)(COLOR_HIGHLIGHT + 1));
            }
        }
    }
    else
    {
        if (Context->ThemeSupport)
        {
            HBRUSH oldBrush;

            SetDCBrushColor(CustomDraw->hdc, PhThemeWindowBackgroundColor);
            FillRect(CustomDraw->hdc, &CustomDraw->rc, PhGetStockBrush(DC_BRUSH));

            oldBrush = SelectBrush(CustomDraw->hdc, PhGetStockBrush(DC_BRUSH));

            if (Context->HeaderDragging && Context->HeaderHotColumn == column->Id)
            {
                SetDCBrushColor(CustomDraw->hdc, RGB(0, 0, 229));
                PatBlt(CustomDraw->hdc, CustomDraw->rc.right - 2, CustomDraw->rc.top, 2, CustomDraw->rc.bottom - CustomDraw->rc.top, PATCOPY);
            }
            else
            {
                SetDCBrushColor(CustomDraw->hdc, PhThemeWindowBorderColor);
                PatBlt(CustomDraw->hdc, CustomDraw->rc.right - 1, CustomDraw->rc.top, 1, CustomDraw->rc.bottom - CustomDraw->rc.top, PATCOPY);
                //PatBlt(Hdc, CustomDraw->rc.left, CustomDraw->rc.bottom - 1, CustomDraw->rc.right - CustomDraw->rc.left, 1, PATCOPY);
            }

            if (oldBrush)
                SelectBrush(CustomDraw->hdc, oldBrush);
        }
        else if (Context->HeaderThemeHandle)
        {
            PhDrawThemeBackground(
                Context->HeaderThemeHandle,
                CustomDraw->hdc,
                HP_HEADERITEM,
                HIS_NORMAL,
                &CustomDraw->rc,
                NULL
                );
        }
        else
        {
            FillRect(CustomDraw->hdc, &CustomDraw->rc, (HBRUSH)(COLOR_WINDOW + 1));
        }
    }

    if (column->Text)
    {
        PCWSTR textBuffer;
        LONG textLength;
        RECT textRect;
        HFONT oldFont;
        PH_STRINGREF headerString;
        ULONG fmt = 0;
        LONG sdf = 0;

        if (FlagOn(column->Alignment, PH_ALIGN_LEFT))
            SetFlag(fmt, HDF_LEFT);
        else if (FlagOn(column->Alignment, PH_ALIGN_RIGHT))
            SetFlag(fmt, HDF_RIGHT);
        else
            SetFlag(fmt, HDF_CENTER);

        if (column->Id == Context->SortColumn)
        {
            if (Context->SortOrder == AscendingSortOrder)
                SetFlag(fmt, HDF_SORTUP);
            else if (Context->SortOrder == DescendingSortOrder)
                SetFlag(fmt, HDF_SORTDOWN);

            if (FlagOn(fmt, HDF_SORTDOWN))
                sdf = HSAS_SORTEDDOWN;
            else if (FlagOn(fmt, HDF_SORTUP))
                sdf = HSAS_SORTEDUP;
        }

        textBuffer = column->Text;
        textLength = (LONG)PhCountStringZ(column->Text);

        textRect = CustomDraw->rc;
        textRect.left += Context->HeaderTextPadding;
        textRect.right -= Context->HeaderTextPadding;
        textRect.bottom -= Context->HeaderTextPadding;
        textRect.top += Context->HeaderTextMargin;

        SetTextColor(CustomDraw->hdc, Context->ThemeSupport ? PhThemeWindowDisabledTextColor : RGB(97, 116, 139)); // RGB(178, 178, 178)

        oldFont = SelectFont(CustomDraw->hdc, Context->Font);
        if (FlagOn(fmt, HDF_RIGHT))
        {
            DrawText(
                CustomDraw->hdc, textBuffer, textLength, &textRect,
                DT_SINGLELINE | DT_HIDEPREFIX | DT_WORD_ELLIPSIS | DT_BOTTOM | DT_RIGHT);
        }
        else
        {
            DrawText(CustomDraw->hdc, textBuffer, textLength, &textRect,
                DT_SINGLELINE | DT_HIDEPREFIX | DT_WORD_ELLIPSIS | DT_BOTTOM | DT_LEFT);
        }
        SelectFont(CustomDraw->hdc, oldFont);

        if (Context->HeaderCustomDraw && PhTnpGetColumnHeaderText(Context, column, &headerString))
        {
            SetTextColor(CustomDraw->hdc, Context->ThemeSupport ? PhThemeWindowTextColor : RGB(0, 0, 0));
            oldFont = SelectFont(CustomDraw->hdc, Context->HeaderBoldFontHandle);
            DrawText(
                CustomDraw->hdc,
                headerString.Buffer,
                (LONG)headerString.Length / (LONG)sizeof(WCHAR),
                &textRect,
                DT_SINGLELINE | DT_HIDEPREFIX | DT_WORD_ELLIPSIS | DT_TOP | DT_RIGHT);
            if (oldFont) SelectFont(CustomDraw->hdc, oldFont);
        }

        //DrawEdge(CustomDraw->hdc, &CustomDraw->rc, EDGE_SUNKEN, BF_SOFT | BF_RIGHT);
        SetDCBrushColor(CustomDraw->hdc, Context->ThemeSupport ? PhThemeWindowBorderColor : RGB(229, 229, 229));
        HBRUSH oldBrush = SelectBrush(CustomDraw->hdc, PhGetStockBrush(DC_BRUSH));
        PatBlt(CustomDraw->hdc, CustomDraw->rc.right - 1, CustomDraw->rc.top, 1, CustomDraw->rc.bottom - CustomDraw->rc.top, PATCOPY);
        SelectBrush(CustomDraw->hdc, oldBrush);

        if (FlagOn(fmt, HDF_SORTDOWN | HDF_SORTUP))
        {
            if (Context->HeaderThemeHandle)
            {
                SIZE sortArrowSize = { 0 };

                PhGetThemePartSize(
                    Context->HeaderThemeHandle,
                    CustomDraw->hdc,
                    HP_HEADERSORTARROW,
                    sdf,
                    NULL,
                    THEMEPARTSIZE_TRUE,
                    &sortArrowSize
                    );

                CustomDraw->rc.bottom = sortArrowSize.cy;

                PhDrawThemeBackground(
                    Context->HeaderThemeHandle,
                    CustomDraw->hdc,
                    HP_HEADERSORTARROW,
                    sdf,
                    &CustomDraw->rc,
                    NULL
                    );
            }
        }
    }

    return TRUE;
}

/**
 * Creates a buffered paint context for the header.
 *
 * \param Context Pointer to the treenew context structure.
 * \param Hdc The device context handle.
 * \param BufferRect Pointer to the buffer rectangle.
 */
VOID PhTnpHeaderCreateBufferedContext(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT BufferRect
    )
{
    Context->HeaderBufferedDc = CreateCompatibleDC(Hdc);

    if (!Context->HeaderBufferedDc)
        return;

    Context->HeaderBufferedContextRect = *BufferRect;
    Context->HeaderBufferedBitmap = PhCreateDIBSection(
        Hdc,
        PHBF_TOPDOWNDIB,
        Context->HeaderBufferedContextRect.right,
        Context->HeaderBufferedContextRect.bottom,
        NULL
        );

    Context->HeaderBufferedOldBitmap = SelectBitmap(Context->HeaderBufferedDc, Context->HeaderBufferedBitmap);
}

/**
 * Destroys the buffered paint context for the header.
 *
 * \param Context Pointer to the treenew context structure.
 */
VOID PhTnpHeaderDestroyBufferedContext(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    if (Context->HeaderBufferedDc && Context->HeaderBufferedOldBitmap)
    {
        SelectBitmap(Context->HeaderBufferedDc, Context->HeaderBufferedOldBitmap);
        Context->HeaderBufferedOldBitmap = NULL;
    }

    if (Context->HeaderBufferedBitmap)
    {
        DeleteBitmap(Context->HeaderBufferedBitmap);
        Context->HeaderBufferedBitmap = NULL;
    }

    if (Context->HeaderBufferedDc)
    {
        DeleteDC(Context->HeaderBufferedDc);
        Context->HeaderBufferedDc = NULL;
    }
}

BOOLEAN PhTnpSelectionCreateBufferedContext(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    BITMAPINFO bmi;

    if (Context->SelectionScratchDc)
        return TRUE;

    Context->SelectionScratchDc = CreateCompatibleDC(NULL);
    if (!Context->SelectionScratchDc)
        return FALSE;

    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 1;
    bmi.bmiHeader.biHeight = -1;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    Context->SelectionScratchBitmap = CreateDIBSection(
        Context->SelectionScratchDc,
        &bmi,
        DIB_RGB_COLORS,
        NULL,
        NULL,
        0
        );

    if (!Context->SelectionScratchBitmap)
    {
        DeleteDC(Context->SelectionScratchDc);
        Context->SelectionScratchDc = NULL;
        return FALSE;
    }

    return TRUE;
}

VOID PhTnpSelectionDestroyBufferedContext(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    // Note: the scratch bitmap is selected and restored by the callers that use it, so there is no
    // old bitmap to put back here. (dmex)

    if (Context->SelectionScratchBitmap)
    {
        DeleteBitmap(Context->SelectionScratchBitmap);
        Context->SelectionScratchBitmap = NULL;
    }

    if (Context->SelectionScratchDc)
    {
        DeleteDC(Context->SelectionScratchDc);
        Context->SelectionScratchDc = NULL;
    }
}

/**
 * Window procedure hook for the header control.
 *
 * \param WindowHandle Handle to the header window.
 * \param WindowMessage The message identifier.
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \return The result of the message processing.
 */
LRESULT CALLBACK PhTnpHeaderHookWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_TREENEW_CONTEXT context;
    WNDPROC oldWndProc;

    if (context = PhGetWindowContext(WindowHandle, MAXCHAR))
    {
        if (WindowHandle == context->FixedHeaderHandle)
            oldWndProc = context->FixedHeaderWindowProc;
        else
            oldWndProc = context->HeaderWindowProc;
    }
    else
    {
        return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
    }

    switch (WindowMessage)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            RECT clientRect;
            HDC hdc;

            // BeginPaint must run before anything else can bail out: without it the update region
            // is never validated and the window is sent WM_PAINT again, forever. (dmex)

            if (!(hdc = BeginPaint(WindowHandle, &paintStruct)))
                return 0;

            if (!PhGetClientRect(WindowHandle, &clientRect) ||
                clientRect.right <= 0 || clientRect.bottom <= 0)
            {
                EndPaint(WindowHandle, &paintStruct);
                return 0;
            }

            if (!context->HeaderBufferedDc ||
                context->HeaderBufferedContextRect.right != clientRect.right ||
                context->HeaderBufferedContextRect.bottom != clientRect.bottom)
            {
                PhTnpHeaderDestroyBufferedContext(context);
                PhTnpHeaderCreateBufferedContext(context, hdc, &clientRect);
            }

            if (context->HeaderBufferedDc)
            {
                INT savedDcState;

                // The header renders every item in one pass. Clipping the buffer to the dirty
                // rectangle lets the custom draw handler cull the columns outside it. (dmex)
                savedDcState = SaveDC(context->HeaderBufferedDc);

                IntersectClipRect(
                    context->HeaderBufferedDc,
                    paintStruct.rcPaint.left,
                    paintStruct.rcPaint.top,
                    paintStruct.rcPaint.right,
                    paintStruct.rcPaint.bottom
                    );

                CallWindowProc(
                    oldWndProc,
                    WindowHandle,
                    WM_PRINTCLIENT,
                    (WPARAM)context->HeaderBufferedDc,
                    PRF_CLIENT
                    );

                if (savedDcState)
                    RestoreDC(context->HeaderBufferedDc, savedDcState);
                else
                    SelectClipRgn(context->HeaderBufferedDc, NULL);

                BitBlt(
                    hdc,
                    paintStruct.rcPaint.left,
                    paintStruct.rcPaint.top,
                    paintStruct.rcPaint.right - paintStruct.rcPaint.left,
                    paintStruct.rcPaint.bottom - paintStruct.rcPaint.top,
                    context->HeaderBufferedDc,
                    paintStruct.rcPaint.left,
                    paintStruct.rcPaint.top,
                    SRCCOPY
                    );
            }
            else
            {
                CallWindowProc(oldWndProc, WindowHandle, WM_PRINTCLIENT,  (WPARAM)hdc, PRF_CLIENT);
            }

            EndPaint(WindowHandle, &paintStruct);
            return 0;
        }
    case WM_DESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWndProc);

            PhRemoveWindowContext(WindowHandle, MAXCHAR);

            PhTnpHeaderDestroyBufferedContext(context);
        }
        break;
    case WM_MOUSEMOVE:
        {
            POINT point;
            PPH_TREENEW_COLUMN column;
            ULONG id;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);
            column = PhTnpHitTestHeader(context, WindowHandle == context->FixedHeaderHandle, &point, NULL);

            if (column)
                id = column->Id;
            else
                id = -1;

            if (context->TooltipColumnId != id)
            {
                PhTnpPopTooltip(context);
            }
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;

            switch (header->code)
            {
            case TTN_GETDISPINFO:
                {
                    if (header->hwndFrom == context->TooltipsHandle)
                    {
                        NMTTDISPINFO *info = (NMTTDISPINFO *)header;
                        POINT point;
                        PPH_STRING string;

                        if (PhGetClientPos(WindowHandle, &point))
                        {
                            if (PhTnpGetHeaderTooltipText(context, info->lParam == TNP_TOOLTIPS_FIXED_HEADER, &point, &string))
                            {
                                info->lpszText = string->Buffer;
                                break;
                            }
                        }
                    }
                }
                break;
            case TTN_SHOW:
                {
                    if (header->hwndFrom == context->TooltipsHandle)
                    {
                        return PhTnpPrepareTooltipShow(context);
                    }
                }
                break;
            case TTN_POP:
                {
                    if (header->hwndFrom == context->TooltipsHandle)
                    {
                        PhTnpPrepareTooltipPop(context);
                    }
                }
                break;
            }
        }
        break;
    }

    switch (WindowMessage)
    {
    //case WM_MOUSEMOVE:
    //case WM_LBUTTONDOWN:
    //case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        {
            if (context->TooltipsHandle)
            {
                MSG message;

                message.hwnd = WindowHandle;
                message.message = WindowMessage;
                message.wParam = wParam;
                message.lParam = lParam;
                SendMessage(context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }
        }
        break;
    case WM_SETFONT:
        {
            HFONT fontHandle = (HFONT)wParam;
            LOGFONT logFont;

            if (!context->HeaderCustomDraw && !context->ThemeSupport)
                break;

            if (context->HeaderBoldFontHandle)
            {
                DeleteFont(context->HeaderBoldFontHandle);
                context->HeaderBoldFontHandle = NULL;
            }

            if (GetObject(fontHandle, sizeof(LOGFONT), &logFont))
            {
                logFont.lfHeight -= context->HeaderTextMargin;
                context->HeaderBoldFontHandle = CreateFontIndirect(&logFont);
                //context->HeaderBoldFontHandle = PhDuplicateFontWithNewHeight(fontHandle, -14);
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            BOOLEAN redraw = FALSE;

            if (context->TooltipsHandle)
            {
                MSG message;

                message.hwnd = WindowHandle;
                message.message = WindowMessage;
                message.wParam = wParam;
                message.lParam = lParam;
                SendMessage(context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }

            if (!context->HeaderCustomDraw && !context->ThemeSupport)
                break;
            //if (GetCapture() == WindowHandle)
            //    break;

            if (!context->HeaderDragging)
            {
                ULONG hitcolumn;
                POINT point;
                PPH_TREENEW_COLUMN column;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);
                column = PhTnpHitTestHeader(context, WindowHandle == context->FixedHeaderHandle, &point, NULL);

                hitcolumn = column ? column->Id : ULONG_MAX;

                if (context->HeaderHotColumn != hitcolumn)
                {
                    // Only the two columns involved change appearance. Repainting the whole header
                    // on every mouse movement redraws every column. (dmex)
                    PhpTnpInvalidateHeaderColumn(context, WindowHandle, context->HeaderHotColumn);
                    context->HeaderHotColumn = hitcolumn;
                    PhpTnpInvalidateHeaderColumn(context, WindowHandle, hitcolumn);
                }
            }

            if (!context->HeaderMouseActive)
            {
                TRACKMOUSEEVENT trackEvent =
                {
                    sizeof(TRACKMOUSEEVENT),
                    TME_LEAVE,
                    WindowHandle,
                    0
                };

                TrackMouseEvent(&trackEvent);
                context->HeaderMouseActive = TRUE;

                redraw = TRUE;
            }

            if (redraw)
            {
                InvalidateRect(WindowHandle, NULL, FALSE);
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            LRESULT result;

            if (!context->HeaderCustomDraw)
                break;

            result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
            InvalidateRect(WindowHandle, NULL, FALSE);
            return result;
        }
        break;
    case WM_LBUTTONDOWN:
        {
            LRESULT result;
            ULONG hitcolumn;
            POINT point;
            PPH_TREENEW_COLUMN column;

            if (context->TooltipsHandle)
            {
                MSG message;

                message.hwnd = WindowHandle;
                message.message = WindowMessage;
                message.wParam = wParam;
                message.lParam = lParam;
                SendMessage(context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }

            if (!context->HeaderCustomDraw)
                break;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);
            column = PhTnpHitTestHeader(context, WindowHandle == context->FixedHeaderHandle, &point, NULL);

            hitcolumn = column ? column->Id : ULONG_MAX;

            if (context->HeaderHotColumn != hitcolumn)
            {
                context->HeaderHotColumn = hitcolumn;
                //redraw = TRUE;
            }

            InvalidateRect(WindowHandle, NULL, FALSE);

            result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
            context->HeaderDragging = TRUE;
            return result;
        }
        break;
    case WM_LBUTTONUP:
        {
            LRESULT result;

            if (context->TooltipsHandle)
            {
                MSG message;

                message.hwnd = WindowHandle;
                message.message = WindowMessage;
                message.wParam = wParam;
                message.lParam = lParam;
                SendMessage(context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }

            if (!context->HeaderCustomDraw)
                break;

            result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
            context->HeaderDragging = FALSE;
            return result;
        }
        break;
    case WM_MOUSELEAVE:
        {
            LRESULT result;

            if (context->TooltipsHandle)
            {
                MSG message;

                message.hwnd = WindowHandle;
                message.message = WindowMessage;
                message.wParam = wParam;
                message.lParam = lParam;
                SendMessage(context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }

            if (!context->HeaderCustomDraw)
                break;

            result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
            context->HeaderMouseActive = FALSE;

            //if (GetCapture() != WindowHandle)
            //{
            //    InvalidateRect(WindowHandle, NULL, FALSE);
            //}

            // Only the column losing the hot state needs repainting. (dmex)
            PhpTnpInvalidateHeaderColumn(context, WindowHandle, context->HeaderHotColumn);
            context->HeaderHotColumn = ULONG_MAX;

            return result;
        }
        break;
    case WM_THEMECHANGED:
        {
            if (context->HeaderThemeHandle)
            {
                PhCloseThemeData(context->HeaderThemeHandle);
                context->HeaderThemeHandle = NULL;
            }

            context->HeaderThemeHandle = PhOpenThemeData(WindowHandle, VSCLASS_HEADER, context->WindowDpi);
            context->HeaderInvalidatePending = TRUE;
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Detects the start of a drag operation.
 *
 * \param Context Pointer to the treenew context structure.
 * \param CursorX The X coordinate of the cursor.
 * \param CursorY The Y coordinate of the cursor.
 * \param DispatchMessages TRUE to dispatch messages during drag detection, FALSE otherwise.
 * \param CancelledByMessage Pointer to receive the message that cancelled the drag, or NULL.
 * \return TRUE if a drag was detected, FALSE otherwise.
 */
BOOLEAN PhTnpDetectDrag(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG CursorX,
    _In_ LONG CursorY,
    _In_ BOOLEAN DispatchMessages,
    _Out_opt_ PULONG CancelledByMessage
    )
{
    RECT dragRect;
    MSG msg;

    // Capture mouse input and see if the user moves the mouse beyond the drag rectangle.

    dragRect.left = CursorX - Context->SystemDragX;
    dragRect.top = CursorY - Context->SystemDragY;
    dragRect.right = CursorX + Context->SystemDragX;
    dragRect.bottom = CursorY + Context->SystemDragY;
    MapWindowRect(Context->Handle, NULL, &dragRect);

    SetCapture(Context->Handle);

    if (CancelledByMessage)
        *CancelledByMessage = 0;

    do
    {
        // It seems that GetMessage dispatches nonqueued messages directly from kernel-mode, so we
        // have to use PeekMessage and WaitMessage in order to process WM_CAPTURECHANGED messages.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.hwnd != Context->Handle)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                continue;
            }

            switch (msg.message)
            {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
                ReleaseCapture();

                if (CancelledByMessage)
                    *CancelledByMessage = msg.message;

                break;
            case WM_MOUSEMOVE:
                if (msg.pt.x < dragRect.left || msg.pt.x >= dragRect.right ||
                    msg.pt.y < dragRect.top || msg.pt.y >= dragRect.bottom)
                {
                    if (IsWindow(Context->Handle))
                        return TRUE;
                    else
                        return FALSE;
                }
                break;
            default:
                if (DispatchMessages)
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                break;
            }
        }
        else
        {
            WaitMessage();
        }
    } while (IsWindow(Context->Handle) && GetCapture() == Context->Handle);

    return FALSE;
}

/**
 * Performs drag selection of nodes.
 *
 * \param Context Pointer to the treenew context structure.
 * \param CursorX The starting X coordinate of the cursor.
 * \param CursorY The starting Y coordinate of the cursor.
 */
VOID PhTnpDragSelect(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    )
{
    MSG msg;
    LONG cursorX;
    LONG cursorY;
    BOOLEAN originFixed;
    RECT dragRect;
    RECT oldDragRect;
    RECT windowRect;
    POINT cursorPoint;
    BOOLEAN showContextMenu;

    cursorX = CursorX;
    cursorY = CursorY;
    originFixed = cursorX < Context->FixedWidth;

    dragRect.left = cursorX;
    dragRect.top = cursorY;
    dragRect.right = cursorX;
    dragRect.bottom = cursorY;
    oldDragRect = dragRect;
    Context->DragRect = dragRect;
    Context->DragSelectionActive = TRUE;

    if (Context->DoubleBuffered)
        Context->SelectionRectangleAlpha = TRUE;
    // TODO: Make sure the monitor's color depth is sufficient for alpha-blended selection
    // rectangles.

    if (!PhGetWindowRect(Context->Handle, &windowRect))
        return;

    cursorPoint.x = windowRect.left + cursorX;
    cursorPoint.y = windowRect.top + cursorY;

    showContextMenu = FALSE;

    SetCapture(Context->Handle);

    while (TRUE)
    {
        if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            BOOLEAN leftOrRight;
            BOOLEAN aboveOrBelow;

            // If the cursor is outside of the window, generate some messages so the window keeps
            // scrolling.

            leftOrRight = cursorPoint.x < windowRect.left || cursorPoint.x > windowRect.right;
            aboveOrBelow = cursorPoint.y < windowRect.top || cursorPoint.y > windowRect.bottom;

            if ((Context->VScrollVisible && aboveOrBelow && PhTnpCanScroll(Context, FALSE, cursorPoint.y > windowRect.bottom)) ||
                (Context->HScrollVisible && leftOrRight && PhTnpCanScroll(Context, TRUE, cursorPoint.x > windowRect.right)))
            {
                SetCursorPos(cursorPoint.x, cursorPoint.y);
            }
            else
            {
                WaitMessage();
            }

            goto EndOfLoop;
        }

        cursorPoint = msg.pt;

        switch (msg.message)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            ReleaseCapture();
            goto EndOfLoop;
        case WM_RBUTTONUP:
            ReleaseCapture();
            showContextMenu = TRUE;
            goto EndOfLoop;
        case WM_MOUSEMOVE:
            {
                LONG newCursorX;
                LONG newCursorY;
                LONG deltaRows;
                LONG deltaX;
                LONG oldVScrollPosition;
                LONG oldHScrollPosition;
                LONG newDeltaX;
                LONG newDeltaY;
                LONG viewLeft;
                LONG viewTop;
                LONG viewRight;
                LONG viewBottom;
                LONG temp;
                RECT totalRect;

                newCursorX = GET_X_LPARAM(msg.lParam);
                newCursorY = GET_Y_LPARAM(msg.lParam);

                // Scroll the window if the cursor is outside of it.

                deltaRows = 0;
                deltaX = 0;

                if (Context->VScrollVisible)
                {
                    if (cursorPoint.y < windowRect.top)
                        deltaRows = -(windowRect.top - cursorPoint.y + Context->RowHeight - 1) / Context->RowHeight; // scroll up
                    else if (cursorPoint.y >= windowRect.bottom)
                        deltaRows = (cursorPoint.y - windowRect.bottom + Context->RowHeight - 1) / Context->RowHeight; // scroll down
                }

                if (Context->HScrollVisible)
                {
                    if (cursorPoint.x < windowRect.left)
                        deltaX = -(windowRect.left - cursorPoint.x); // scroll left
                    else if (cursorPoint.x >= windowRect.right)
                        deltaX = cursorPoint.x - windowRect.right; // scroll right
                }

                oldVScrollPosition = Context->VScrollPosition;
                oldHScrollPosition = Context->HScrollPosition;

                if (deltaRows != 0 || deltaX != 0)
                    PhTnpScroll(Context, deltaRows, deltaX);

                newDeltaX = oldHScrollPosition - Context->HScrollPosition;
                newDeltaY = (oldVScrollPosition - Context->VScrollPosition) * Context->RowHeight;

                // Adjust our original drag point for the scrolling.
                if (!originFixed)
                    cursorX += newDeltaX;
                cursorY += newDeltaY;

                // Adjust the old drag rectangle for the scrolling.
                if (!originFixed)
                    oldDragRect.left += newDeltaX;
                oldDragRect.top += newDeltaY;
                if (!originFixed)
                    oldDragRect.right += newDeltaX;
                oldDragRect.bottom += newDeltaY;

                // Ensure that the new cursor position is within the content area.

                viewLeft = Context->FixedColumnVisible ? 0 : -Context->HScrollPosition;
                viewTop = Context->HeaderHeight - Context->VScrollPosition * Context->RowHeight;
                viewRight = Context->NormalLeft + Context->TotalViewX - Context->HScrollPosition;
                viewBottom = Context->HeaderHeight + ((LONG)Context->FlatList->Count - Context->VScrollPosition) * Context->RowHeight;

                temp = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);
                viewRight = max(viewRight, temp);
                temp = Context->ClientRect.bottom - ((!Context->FixedColumnVisible && Context->HScrollVisible) ? Context->HScrollHeight : 0);
                viewBottom = max(viewBottom, temp);

                if (newCursorX < viewLeft)
                    newCursorX = viewLeft;
                if (newCursorX > viewRight)
                    newCursorX = viewRight;
                if (newCursorY < viewTop)
                    newCursorY = viewTop;
                if (newCursorY > viewBottom)
                    newCursorY = viewBottom;

                // Create the new drag rectangle.

                if (cursorX < newCursorX)
                {
                    dragRect.left = cursorX;
                    dragRect.right = newCursorX;
                }
                else
                {
                    dragRect.left = newCursorX;
                    dragRect.right = cursorX;
                }

                if (cursorY < newCursorY)
                {
                    dragRect.top = cursorY;
                    dragRect.bottom = newCursorY;
                }
                else
                {
                    dragRect.top = newCursorY;
                    dragRect.bottom = cursorY;
                }

                // Has anything changed from before?
                if (dragRect.left == oldDragRect.left && dragRect.top == oldDragRect.top &&
                    dragRect.right == oldDragRect.right && dragRect.bottom == oldDragRect.bottom)
                {
                    break;
                }

                Context->DragRect = dragRect;

                // Process the selection.
                totalRect.left = min(dragRect.left, oldDragRect.left);
                totalRect.top = min(dragRect.top, oldDragRect.top);
                totalRect.right = max(dragRect.right, oldDragRect.right);
                totalRect.bottom = max(dragRect.bottom, oldDragRect.bottom);
                PhTnpProcessDragSelect(Context, (ULONG)msg.wParam, &oldDragRect, &dragRect, &totalRect);

                // Redraw the drag rectangle.
                RedrawWindow(Context->Handle, &totalRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

                oldDragRect = dragRect;
            }
            break;
        case WM_MOUSELEAVE:
            break; // don't process
        case WM_MOUSEWHEEL:
            break; // don't process
        case WM_KEYDOWN:
            if (msg.wParam == VK_ESCAPE)
            {
                ULONG changedStart;
                ULONG changedEnd;
                RECT rect;

                PhTnpSelectRange(Context, ULONG_MAX, ULONG_MAX, TN_SELECT_RESET, &changedStart, &changedEnd);

                if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
                {
                    PhpTnpInvalidateRect(Context, &rect);
                }

                ReleaseCapture();
            }
            break; // don't process
        case WM_CHAR:
            break; // don't process
        default:
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }

EndOfLoop:
        if (GetCapture() != Context->Handle)
            break;
    }

    Context->DragSelectionActive = FALSE;
    RedrawWindow(Context->Handle, &dragRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

    if (showContextMenu)
    {
        // Display a context menu at the original drag point.
        SendMessage(Context->Handle, WM_CONTEXTMENU, (WPARAM)Context->Handle, MAKELPARAM(windowRect.left + CursorX, windowRect.top + CursorY));
    }
}

/**
 * Processes drag selection based on mouse movement.
 *
 * \param Context Pointer to the treenew context structure.
 * \param VirtualKeys The state of virtual keys.
 * \param OldRect Pointer to the previous drag rectangle.
 * \param NewRect Pointer to the new drag rectangle.
 * \param TotalRect Pointer to the total drag rectangle.
 */
VOID PhTnpProcessDragSelect(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKeys,
    _In_ PRECT OldRect,
    _In_ PRECT NewRect,
    _In_ PRECT TotalRect
    )
{
    LONG firstRow;
    LONG lastRow;
    RECT rowRect;
    LONG i;
    PPH_TREENEW_NODE node;
    LONG changedStart;
    LONG changedEnd;
    RECT rect;

    // Determine which rows we need to test. The divisions below must be done on positive integers
    // to ensure correct rounding.

    firstRow = (TotalRect->top - Context->HeaderHeight + Context->VScrollPosition * Context->RowHeight) / Context->RowHeight;
    lastRow = (TotalRect->bottom - 1 - Context->HeaderHeight + Context->VScrollPosition * Context->RowHeight) / Context->RowHeight;

    if (firstRow < 0)
        firstRow = 0;
    if (lastRow >= (LONG)Context->FlatList->Count)
        lastRow = Context->FlatList->Count - 1;

    if (lastRow < firstRow)
        return;

    rowRect.left = 0;
    rowRect.top = Context->HeaderHeight + (firstRow - Context->VScrollPosition) * Context->RowHeight;
    rowRect.right = Context->NormalLeft + Context->TotalViewX - Context->HScrollPosition;
    rowRect.bottom = rowRect.top + Context->RowHeight;

    changedStart = lastRow;
    changedEnd = firstRow;

    // Process the rows.
    for (i = firstRow; i <= lastRow; i++)
    {
        BOOLEAN inOldRect;
        BOOLEAN inNewRect;

        node = Context->FlatList->Items[i];

        inOldRect = rowRect.top < OldRect->bottom && rowRect.bottom > OldRect->top &&
            rowRect.left < OldRect->right && rowRect.right > OldRect->left;
        inNewRect = rowRect.top < NewRect->bottom && rowRect.bottom > NewRect->top &&
            rowRect.left < NewRect->right && rowRect.right > NewRect->left;

        if (VirtualKeys & MK_CONTROL)
        {
            if (!node->Unselectable && inOldRect != inNewRect)
            {
                node->Selected = !node->Selected;

                if (changedStart > i)
                    changedStart = i;
                if (changedEnd < i)
                    changedEnd = i;
            }
        }
        else
        {
            if (!node->Unselectable && inOldRect != inNewRect)
            {
                node->Selected = inNewRect;

                if (changedStart > i)
                    changedStart = i;
                if (changedEnd < i)
                    changedEnd = i;
            }
        }

        rowRect.top = rowRect.bottom;
        rowRect.bottom += Context->RowHeight;
    }

    if (changedStart <= changedEnd)
    {
        Context->Callback(Context->Handle, TreeNewSelectionChanged, NULL, NULL, Context->CallbackContext);
    }

    if (PhTnpGetRowRects(Context, changedStart, changedEnd, TRUE, &rect))
    {
        PhpTnpInvalidateRect(Context, &rect);
    }
}

/**
 * Sends a message to a TreeNew window, handling custom messages if needed.
 *
 * \param WindowHandle Handle to the window.
 * \param WindowMessage Message to send.
 * \param wParam WPARAM for the message.
 * \param lParam LPARAM for the message.
 * \return The result of the message processing.
 */
LRESULT PhTnSendMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG WindowMessage,
    _Pre_maybenull_ _Post_valid_ WPARAM wParam,
    _Pre_maybenull_ _Post_valid_ LPARAM lParam
    )
{
    if (!WindowHandle)
        return 0;

    if (WindowMessage >= TNM_FIRST && WindowMessage <= TNM_LAST)
    {
        PPH_TREENEW_CONTEXT context;
        if (context = PhGetWindowContextEx(WindowHandle))
        {
#if defined(DEBUG)
            assert(context->UniqueThread == NtCurrentThreadId());
#endif
            return PhTnpOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);
        }
    }
#if defined(DEBUG)
    assert(FALSE);
#endif
    return SendMessage(WindowHandle, WindowMessage, wParam, lParam);
}

//
// Drag-reorder
//

/**
 * Draws the insertion caret for drag-reorder operations.
 *
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param Hdc Handle to the device context to draw on.
 */
VOID PhTnpDrawInsertionCaret(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC Hdc
    )
{
    RECT r;
    HPEN old;
    COLORREF prev;

    r = Context->ReorderInsertRect;

    if (r.right <= r.left || r.bottom <= r.top)
        return;

    old = SelectPen(Hdc, PhGetStockPen(DC_PEN));
    // The accent color in themes that resolve it; the system highlight otherwise. (dmex)
    prev = SetDCPenColor(Hdc, Context->ThemeSupport ? PhThemeWindowFocusBorderColor : GetSysColor(COLOR_HOTLIGHT));

    POINT pts[2];
    pts[0].x = r.left;
    pts[0].y = (r.top + r.bottom) / 2;
    pts[1].x = r.right;
    pts[1].y = pts[0].y;
    Polyline(Hdc, pts, 2);

    SetDCPenColor(Hdc, prev);
    if (old) SelectPen(Hdc, old);
}

/**
 * Invalidates the region occupied by the drag-reorder insertion caret.
 *
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 */
VOID PhTnpReorderInvalidateCaret(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    if (Context->ReorderInsertRect.right > Context->ReorderInsertRect.left &&
        Context->ReorderInsertRect.bottom > Context->ReorderInsertRect.top)
    {
        PhpTnpInvalidateRect(Context, &Context->ReorderInsertRect);
    }
}

/**
 * Updates the rectangle for the drag-reorder insertion caret based on the current target index.
 *
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 */
VOID PhTnpReorderUpdateCaretRect(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    // Compute y position for insertion caret
    LONG viewLeft = 0;
    LONG viewRight = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);
    LONG rowYTop = Context->HeaderHeight + ((LONG)Context->ReorderTargetIndex - Context->VScrollPosition) * Context->RowHeight;

    if (Context->ReorderDropAfter)
    {
        rowYTop += Context->RowHeight;
    }

    RECT rect;
    rect.left   = viewLeft;
    rect.right  = viewRight;
    rect.top    = rowYTop - 1;
    rect.bottom = rowYTop + 1;

    memcpy(&Context->ReorderInsertRect, &rect, sizeof(RECT));
}

/**
 * Begins a drag-reorder operation.
 *
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param SourceIndex Index of the node being dragged.
 */
VOID PhTnpReorderBegin(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG SourceIndex
    )
{
    Context->ReorderDragActive  = TRUE;
    Context->ReorderSourceIndex = SourceIndex;
    Context->ReorderTargetIndex = SourceIndex;
    Context->ReorderDropAfter   = FALSE;
    Context->ReorderJustStarted = TRUE;

    SetCapture(Context->Handle);

    if (!Context->ReorderCursor)
        Context->ReorderCursor = PhLoadCursor(NULL, IDC_SIZENS);
}

/**
 * Cancels an active drag-reorder operation and notifies the callback.
 *
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 */
VOID PhTnpReorderCancel(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    if (!Context->ReorderDragActive)
        return;

    {
        PH_TREENEW_REORDER_EVENT reorderEvent;

        memset(&reorderEvent, 0, sizeof(PH_TREENEW_REORDER_EVENT));
        reorderEvent.Source = (Context->ReorderSourceIndex < Context->FlatList->Count) ? (PPH_TREENEW_NODE)Context->FlatList->Items[Context->ReorderSourceIndex] : NULL;
        reorderEvent.Target = (Context->ReorderTargetIndex < Context->FlatList->Count) ? (PPH_TREENEW_NODE)Context->FlatList->Items[Context->ReorderTargetIndex] : NULL;
        reorderEvent.DropAfter = Context->ReorderDropAfter;

        Context->Callback(Context->Handle, TreeNewReorderCancel, &reorderEvent, NULL, Context->CallbackContext);
    }

    // Nothing changed except the caret, which was just invalidated. (dmex)
    PhTnpReorderInvalidateCaret(Context);

    memset(&Context->ReorderInsertRect, 0, sizeof(Context->ReorderInsertRect));

    Context->ReorderDragActive = FALSE;

    ReleaseCapture();
}

/**
 * Commits a drag-reorder operation, notifies the callback, and updates the UI.
 *
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 */
VOID PhTnpReorderCommit(
    _In_ PPH_TREENEW_CONTEXT Context
    )
{
    PH_TREENEW_REORDER_EVENT reorderEvent;

    if (!Context->ReorderDragActive)
        return;

    // No-op if same place
    //if (Context->ReorderSourceIndex == Context->ReorderTargetIndex &&
    //    !Context->ReorderDropAfter)
    //{
    //    PhTnpReorderCancel(Context);
    //    return;
    //}

    memset(&reorderEvent, 0, sizeof(PH_TREENEW_REORDER_EVENT));
    reorderEvent.Source = (Context->ReorderSourceIndex < Context->FlatList->Count) ? (PPH_TREENEW_NODE)Context->FlatList->Items[Context->ReorderSourceIndex] : NULL;
    reorderEvent.Target = (Context->ReorderTargetIndex < Context->FlatList->Count) ? (PPH_TREENEW_NODE)Context->FlatList->Items[Context->ReorderTargetIndex] : NULL;
    reorderEvent.DropAfter = Context->ReorderDropAfter;
    reorderEvent.Allow = TRUE;

    // The parent callback must reorder its data and trigger TNM_NODESSTRUCTURED.
    Context->Callback(Context->Handle, TreeNewReorderCommit, &reorderEvent, NULL, Context->CallbackContext);

    PhTnpReorderInvalidateCaret(Context);

    Context->ReorderInsertRect = (RECT){ 0 };
    Context->ReorderDragActive = FALSE;

    ReleaseCapture();

    // Parent should reorder underlying data and then trigger TNM_NODESSTRUCTURED
    PhpTnpInvalidateRect(Context, NULL);
}

/**
 * Updates the drag-reorder target index and caret based on the current cursor position.
 *
 * \param Context Pointer to the PPH_TREENEW_CONTEXT structure.
 * \param CursorX X coordinate of the cursor.
 * \param CursorY Y coordinate of the cursor.
 */
VOID PhTnpReorderUpdate(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    )
{
    LONG y;
    ULONG idx;
    LONG localYTop;
    LONG mid;
    BOOLEAN dropAfter = FALSE;

    if (Context->FlatList->Count == 0)
        return;

    // Update target index and caret based on cursor position

    y = CursorY;
    if (y < Context->HeaderHeight)
        y = Context->HeaderHeight;

    idx = (y - Context->HeaderHeight) / Context->RowHeight + Context->VScrollPosition;
    if (idx >= Context->FlatList->Count)
        idx = Context->FlatList->Count - 1;

    localYTop = Context->HeaderHeight + ((LONG)idx - Context->VScrollPosition) * Context->RowHeight;
    mid = localYTop + (Context->RowHeight / 2);

    if (CursorY >= mid)
    {
        dropAfter = TRUE;
    }

    if (idx != Context->ReorderTargetIndex || dropAfter != Context->ReorderDropAfter)
    {
        PH_TREENEW_REORDER_EVENT reorderEvent;

        memset(&reorderEvent, 0, sizeof(PH_TREENEW_REORDER_EVENT));
        reorderEvent.Source = (Context->ReorderSourceIndex < Context->FlatList->Count) ? (PPH_TREENEW_NODE)Context->FlatList->Items[Context->ReorderSourceIndex] : NULL;
        reorderEvent.Target = (idx < Context->FlatList->Count) ? (PPH_TREENEW_NODE)Context->FlatList->Items[idx] : NULL;
        reorderEvent.DropAfter = dropAfter;
        reorderEvent.Allow = TRUE;

        Context->Callback(Context->Handle, TreeNewReorderOver, &reorderEvent, NULL, Context->CallbackContext);

        if (reorderEvent.Allow)
        {
            PhTnpReorderInvalidateCaret(Context);
            Context->ReorderTargetIndex = idx;
            Context->ReorderDropAfter   = dropAfter;
            PhTnpReorderUpdateCaretRect(Context);
            PhpTnpInvalidateRect(Context, &Context->ReorderInsertRect);
        }
    }
}
