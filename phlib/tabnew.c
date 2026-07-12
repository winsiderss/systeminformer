/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#include <ph.h>
#include <guisup.h>
#include <guisupp.h>
#include <tabnew.h>
#include <tabnewp.h>
#include <vsstyle.h>
#include <uxtheme.h>
#include <vssym32.h>

/**
 * Initializes the tab control class.
 *
 * \return The registered window class atom.
 */
RTL_ATOM PhTabNewInitialization(
    VOID
    )
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_DBLCLKS | CS_GLOBALCLASS;
    wcex.lpfnWndProc = PhTabNewWndProc;
    wcex.cbWndExtra = sizeof(PVOID);
    wcex.hInstance = NtCurrentImageBase();
    wcex.hCursor = PhLoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = PhTabNewGetBackgroundBrush();
    wcex.lpszClassName = PH_TABNEW_CLASSNAME;

    return RegisterClassEx(&wcex);
}

/**
 * Creates a new tab control context.
 *
 * \return A pointer to the created context.
 */
PPH_TABNEW_CONTEXT PhCreateTabNewContext(
    VOID
    )
{
    PPH_TABNEW_CONTEXT context;

    context = PhAllocateZero(sizeof(PH_TABNEW_CONTEXT));
    context->Items = PhCreateList(PH_TABNEW_INITIAL_LIST_CAPACITY);
    context->Pages = PhCreateList(PH_TABNEW_INITIAL_LIST_CAPACITY);
    context->SelectedIndex = LONG_ERROR;
    context->HotIndex = LONG_ERROR;
    context->DragSourceIndex = LONG_ERROR;
    context->DragTargetIndex = LONG_ERROR;
    context->DragOriginIndex = LONG_ERROR;
    context->Skin = PhTabNewSkinUxTheme;
    context->Side = TNS_TOP;
    context->BaseMinTabWidth = PH_TABNEW_DEFAULT_MIN_WIDTH;
    context->BasePaddingX = PH_TABNEW_DEFAULT_PADDING_X;
    context->BasePaddingY = PH_TABNEW_DEFAULT_PADDING_Y;
    context->MinTabWidth = PH_TABNEW_DEFAULT_MIN_WIDTH;
    context->PaddingX = PH_TABNEW_DEFAULT_PADDING_X;
    context->PaddingY = PH_TABNEW_DEFAULT_PADDING_Y;
    context->LayoutDirty = TRUE;

    return context;
}

/**
 * Frees a tab control item.
 *
 * \param Item A pointer to the tab control item.
 */
VOID PhFreeTabNewItem(
    _In_ PPH_TABNEW_INTERNAL_ITEM Item
    )
{
    PhClearReference(&Item->Text);
    PhFree(Item);
}

VOID PhTabNewSynchronizePages(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    ULONG i;

    if (!Context->Pages)
        return;

    PhClearList(Context->Pages);

    for (i = 0; i < Context->Items->Count; i++)
    {
        PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];

        if (item->Page)
        {
            item->Page->Index = (LONG)i;
            PhAddItemList(Context->Pages, item->Page);
        }
    }
}

VOID PhTabNewDeleteAllItems(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    PPH_LIST items;
    PPH_LIST pages;
    ULONG i;

    items = Context->Items;
    pages = Context->Pages;

    // Detach the collections before notifying the owner. This ensures that a
    // notification handler can safely query or modify the control.
    Context->Items = PhCreateList(1);
    Context->Pages = pages ? PhCreateList(1) : NULL;
    Context->SelectedIndex = LONG_ERROR;
    Context->HotIndex = LONG_ERROR;

    for (i = 0; i < items->Count; i++)
    {
        PPH_TABNEW_INTERNAL_ITEM item = items->Items[i];
        PPH_TABNEW_PAGE page = item->Page;

        PhTabNewSendDeleteNotify(Context, (LONG)i, item->Param);

        if (page)
        {
            ULONG pageIndex = pages ? PhFindItemList(pages, page) : ULONG_MAX;

            if (pageIndex != ULONG_MAX)
                PhRemoveItemList(pages, pageIndex);

            if (page->Callback)
                page->Callback(page, PhTabNewPageDestroy, NULL, NULL);

            PhFree(page);
        }

        PhFreeTabNewItem(item);
    }

    PhDereferenceObject(items);

    if (pages)
    {
        // Destroy any legacy page-list entries that were not associated with
        // an item. New items use the explicit item-to-page association above.
        for (i = 0; i < pages->Count; i++)
        {
            PPH_TABNEW_PAGE page = pages->Items[i];

            if (page->Callback)
                page->Callback(page, PhTabNewPageDestroy, NULL, NULL);

            PhFree(page);
        }

        PhDereferenceObject(pages);
    }
}

VOID PhTabNewFlushLayout(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ BOOLEAN NotifyLayout,
    _In_ BOOLEAN Erase
    )
{
    if (Context->LayoutSuspended)
        return;

    PhTabNewLayout(Context, NULL);

    if (NotifyLayout)
        PhTabNewSendLayoutNotify(Context);

    InvalidateRect(Context->WindowHandle, NULL, Erase);
}

/**
 * Destroys a tab control context.
 *
 * \param Context A pointer to the tab control context.
 */
VOID PhDestroyTabNewContext(
    _In_ _Post_invalid_ PPH_TABNEW_CONTEXT Context
    )
{
    if (Context->Items)
    {
        PhTabNewDeleteAllItems(Context);
        PhDereferenceObject(Context->Items);
    }

    if (Context->Pages)
        PhDereferenceObject(Context->Pages);

    if (Context->DragImageList)
    {
        PhImageListDestroy(Context->DragImageList);
        Context->DragImageList = NULL;
    }

    if (Context->Font && Context->OwnFont)
        DeleteFont(Context->Font);

    if (Context->ThemeHandle)
        PhCloseThemeData(Context->ThemeHandle);

    PhTabNewDeleteCachedResources(Context);

    PhFree(Context);
}

_Success_(return)
BOOLEAN PhTabNewGetItemLayoutIdentifier(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG Index,
    _In_ PPH_TABNEW_LAYOUT_CALLBACK Callback,
    _In_opt_ PVOID CallbackContext,
    _Out_ PPH_STRINGREF Identifier
    )
{
    PPH_TABNEW_INTERNAL_ITEM item;

    if (!Callback)
        return FALSE;
    if (Index < 0 || (ULONG)Index >= Context->Items->Count)
        return FALSE;

    item = Context->Items->Items[Index];

    return Callback(Context->WindowHandle, item->Param, Identifier, CallbackContext);
}

_Success_(return)
BOOLEAN PhTabNewReadLayoutToken(
    _Inout_ PPH_STRINGREF Remaining,
    _Out_ PPH_STRINGREF Token
    )
{
    PWSTR cursor;
    ULONG length = 0;
    SIZE_T index = 0;
    SIZE_T count;

    if (!Remaining->Buffer || Remaining->Length == 0)
        return FALSE;

    cursor = Remaining->Buffer;
    count = Remaining->Length / sizeof(WCHAR);

    while (index < count && cursor[index] >= L'0' && cursor[index] <= L'9')
    {
        ULONG digit = (ULONG)(cursor[index] - L'0');

        if (length > (ULONG_MAX - digit) / PH_TABNEW_LAYOUT_DECIMAL_RADIX)
            return FALSE;

        length = (length * PH_TABNEW_LAYOUT_DECIMAL_RADIX) + digit;
        index++;
    }

    if (index == 0 || index >= count || cursor[index] != L':')
        return FALSE;

    index++;

    if ((SIZE_T)length > count - index)
        return FALSE;

    Token->Buffer = cursor + index;
    Token->Length = (SIZE_T)length * sizeof(WCHAR);

    index += length;

    if (index < count && cursor[index] == L';')
        index++;

    Remaining->Buffer = cursor + index;
    Remaining->Length = (count - index) * sizeof(WCHAR);

    return TRUE;
}

VOID PhTabNewUpdateUxThemeMetrics(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    HDC hdc;
    HFONT oldFont = NULL;
    TEXTMETRIC textMetric;
    SIZE partSize = { 0 };
    THEMEMARGINS margins;
    LONG states[] = { TIS_NORMAL, TIS_HOT, TIS_SELECTED };
    ULONG i;

    Context->UxThemeTabHeight = 0;

    if (!(Context->TabFlags & PHTNF_THIN_TABS))
        return;

    if (!Context->ThemeHandle)
        return;

    hdc = GetDC(Context->WindowHandle);

    if (!hdc)
        return;

    if (Context->Font)
        oldFont = SelectFont(hdc, Context->Font);

    for (i = 0; i < RTL_NUMBER_OF(states); i++)
    {
        if (PhGetThemeMargins(
            Context->ThemeHandle,
            hdc,
            TABP_TABITEM,
            states[i],
            TMT_CONTENTMARGINS,
            NULL,
            &margins
            ))
        {
            LONG paddingX;
            LONG paddingY;

            paddingX = max(margins.cxLeftWidth, margins.cxRightWidth);
            paddingY = max(margins.cyTopHeight, margins.cyBottomHeight);

            if (paddingX > Context->PaddingX)
                Context->PaddingX = paddingX;

            if (paddingY > Context->PaddingY)
                Context->PaddingY = paddingY;
        }

        if (PhGetThemePartSize(
            Context->ThemeHandle,
            hdc,
            TABP_TABITEM,
            states[i],
            NULL,
            THEMEPARTSIZE_TRUE,
            &partSize
            ))
        {
            if (partSize.cx > Context->MinTabWidth)
                Context->MinTabWidth = partSize.cx;

            if (partSize.cy > Context->UxThemeTabHeight)
                Context->UxThemeTabHeight = partSize.cy;
        }
    }

    if (GetTextMetrics(hdc, &textMetric))
    {
        LONG height = textMetric.tmHeight + Context->PaddingY * 2;

        if (height > Context->UxThemeTabHeight)
            Context->UxThemeTabHeight = height;
    }

    if (oldFont)
        SelectFont(hdc, oldFont);

    ReleaseDC(Context->WindowHandle, hdc);
}

/**
 * Updates the metrics of the tab control based on the current DPI.
 *
 * \param Context A pointer to the tab control context.
 */
VOID PhTabNewUpdateMetrics(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    Context->MinTabWidth = PhScaleToDisplay(Context->BaseMinTabWidth, Context->WindowDpi);
    Context->PaddingX = PhScaleToDisplay(Context->BasePaddingX, Context->WindowDpi);
    Context->PaddingY = PhScaleToDisplay(Context->BasePaddingY, Context->WindowDpi);
    Context->InsertMarkerWidth = PhScaleToDisplay(PH_TABNEW_INSERT_MARKER_WIDTH, Context->WindowDpi);

    PhTabNewUpdateUxThemeMetrics(Context);
}

/**
 * Updates the font of the tab control based on the current DPI.
 *
 * \param Context A pointer to the tab control context.
 */
VOID PhTabNewUpdateFont(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    HFONT newFont;

    if (Context->Font && !Context->OwnFont)
        return;

    newFont = PhCreateCommonFont(
        PH_TABNEW_DEFAULT_FONT_HEIGHT,
        FW_NORMAL,
        Context->WindowHandle,
        Context->WindowDpi
        );

    if (newFont)
    {
        if (Context->Font && Context->OwnFont)
            DeleteFont(Context->Font);

        Context->Font = newFont;
        Context->OwnFont = TRUE;
    }
}

BOOLEAN PhTabNewUseDarkTheme(
    VOID
    )
{
    if (!PhEnableThemeSupport)
        return FALSE;

    return PhGetColorBrightness(PhThemeWindowBackgroundColor) < PH_TABNEW_DARK_THEME_BRIGHTNESS;
}

HBRUSH PhTabNewGetBackgroundBrush(
    VOID
    )
{
    if (PhTabNewUseDarkTheme())
    {
        if (!PhThemeWindowBackgroundBrush)
        {
            PhThemeWindowBackgroundBrush = CreateSolidBrush(PhThemeWindowBackgroundColor);
        }

        return PhThemeWindowBackgroundBrush;
    }

    return CreateSolidBrush(RGB(0, 0, 0));
}

/**
 * Updates the theme handle and dark mode status.
 *
 * \param Context A pointer to the tab control context.
 */
VOID PhTabNewUpdateTheme(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    Context->ThemeDark = PhTabNewUseDarkTheme();

    if (Context->ThemeHandle)
    {
        PhCloseThemeData(Context->ThemeHandle);
        Context->ThemeHandle = NULL;
    }

    if (Context->Skin == PhTabNewSkinUxTheme)
    {
        Context->ThemeHandle = PhOpenThemeData(Context->WindowHandle, L"Tab", Context->WindowDpi);
    }
}

/**
 * Measures the height of a tab row.
 *
 * \param Context A pointer to the tab control context.
 * \param Hdc A handle to a device context.
 * \return The measured height.
 */
LONG PhMeasureTabHeight(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc
    )
{
    TEXTMETRIC tm = { 0 };
    LONG height;

    if ((Context->TabFlags & PHTNF_THIN_TABS) && Context->UxThemeTabHeight > 0)
        return Context->UxThemeTabHeight;

    if (!GetTextMetrics(Hdc, &tm))
    {
        tm.tmHeight = PhScaleToDisplay(-PH_TABNEW_DEFAULT_FONT_HEIGHT, Context->WindowDpi);
    }

    height = tm.tmHeight + Context->PaddingY * 2;

    return height;
}

/**
 * Measures the width of a specific tab item.
 *
 * \param Context A pointer to the tab control context.
 * \param Hdc A handle to a device context.
 * \param Item A pointer to the tab item.
 * \return The measured width.
 */
LONG PhMeasureTabWidth(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PPH_TABNEW_INTERNAL_ITEM Item
    )
{
    RECT rc = {0,0,0,0};
    HDC localHdc = Hdc;
    HFONT oldFont = NULL;
    LONG width = 0;

    if (!Item || !Item->Text)
    {
        width = Context->MinTabWidth;
        return width;
    }

    if (!localHdc)
    {
        localHdc = GetDC(Context->WindowHandle);
        if (!localHdc)
            return Context->MinTabWidth;
    }

    if (Context->Font)
        oldFont = SelectFont(localHdc, Context->Font);

    // Measure full caption width (no ellipsis) for auto-sized tabs.
    rc.left = 0; rc.top = 0; rc.right = 0; rc.bottom = 0;
    DrawText(
        localHdc,
        Item->Text->Buffer,
        (INT)(Item->Text->Length / sizeof(WCHAR)),
        &rc,
        DT_CALCRECT | DT_SINGLELINE | DT_VCENTER
        );

    width = (rc.right - rc.left) + Context->PaddingX * 2;

    if (Item->ImageIndex >= 0 && Context->ImageList)
    {
        LONG iconCx, iconCy;
        PhImageListGetIconSize(Context->ImageList, &iconCx, &iconCy);
        width += iconCx + Context->PaddingX;
    }

    if (oldFont)
        SelectFont(localHdc, oldFont);

    if (localHdc != Hdc)
        ReleaseDC(Context->WindowHandle, localHdc);

    if (width < Context->MinTabWidth)
        width = Context->MinTabWidth;

    return width;
}

/**
 * Recomputes the layout of all tabs.
 *
 * \param Context A pointer to the tab control context.
 * \param Hdc Optional device context to measure with. If NULL, a device
 * context is acquired and released internally.
 */
// Refactored layout helpers and orchestrator
VOID PhTabNewMeasureItems(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _Out_ PLONG fixedTabWidth,
    _Out_ BOOLEAN *isVertical,
    _Out_ LONG *rowHeight
    )
{
    ULONG i;
    BOOLEAN vertical = (Context->Side == TNS_LEFT || Context->Side == TNS_RIGHT);
    BOOLEAN fixedWidth = !!(Context->StyleFlags & TNS_FIXEDWIDTH);
    LONG maxFixed = 0;

    *isVertical = vertical;

    *rowHeight = PhMeasureTabHeight(Context, Hdc);
    Context->RowHeight = *rowHeight;

    if (fixedWidth && !vertical)
    {
        for (i = 0; i < Context->Items->Count; i++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
            LONG w = PhMeasureTabWidth(Context, Hdc, item);
            if (w > maxFixed) maxFixed = w;
        }
    }

    *fixedTabWidth = maxFixed;
}

VOID PhTabNewComputeRows(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ RECT const *ClientRect,
    _In_ BOOLEAN vertical,
    _In_ LONG fixedTabWidth,
    _Out_ PLONG RowCounts,
    _Out_ PLONG RowWidths
    )
{
    ULONG i;
    LONG cursor = 0;
    LONG row = 0;
    LONG stripExtent = vertical ? (LONG)(ClientRect->bottom - ClientRect->top)
                                : (LONG)(ClientRect->right - ClientRect->left);

    for (i = 0; i < Context->Items->Count; i++)
    {
        PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
        LONG itemExtent;

        if (vertical)
        {
            item->Row = 0;
            itemExtent = Context->RowHeight;
            item->Rect.left = 0;
            item->Rect.top = cursor;
            item->Rect.right = 0; // set later once strip known
            item->Rect.bottom = cursor + itemExtent;
            cursor += itemExtent;
        }
        else
        {
            LONG itemWidth = fixedTabWidth ? fixedTabWidth : PhMeasureTabWidth(Context, NULL, item);
            itemExtent = itemWidth;

            if (cursor > 0 && cursor + itemExtent > stripExtent)
            {
                row++;
                cursor = 0;
            }

            item->Row = row;
            item->Rect.left = cursor;
            item->Rect.right = cursor + itemExtent;
            item->Rect.top = row * Context->RowHeight;
            item->Rect.bottom = (row + 1) * Context->RowHeight;
            cursor += itemExtent;

            if (RowCounts && RowWidths)
            {
                RowCounts[row]++;
                RowWidths[row] += itemExtent;
            }
        }
    }

    if (vertical || Context->Items->Count == 0)
    {
        Context->RowCount = 1;
    }
    else
    {
        PPH_TABNEW_INTERNAL_ITEM lastItem = Context->Items->Items[Context->Items->Count - 1];
        Context->RowCount = lastItem->Row + 1;
    }
}

VOID PhTabNewApplyRowReordering(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    ULONG i;
    LONG selectedRow = LONG_ERROR;

    if (Context->RowCount <= 1)
        return;

    if (Context->SelectedIndex >= 0 && Context->SelectedIndex < (LONG)Context->Items->Count)
        selectedRow = ((PPH_TABNEW_INTERNAL_ITEM)Context->Items->Items[Context->SelectedIndex])->Row;

    for (i = 0; i < Context->Items->Count; i++)
    {
        PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
        LONG visualRow;

        if (Context->Side == TNS_TOP)
        {
            if (selectedRow < 0)
                visualRow = Context->RowCount - 1 - item->Row;
            else if (item->Row == selectedRow)
                visualRow = Context->RowCount - 1;
            else
                visualRow = Context->RowCount - 1 - item->Row - (selectedRow > item->Row ? 1 : 0);
        }
        else
        {
            if (selectedRow < 0)
                visualRow = item->Row;
            else if (item->Row == selectedRow)
                visualRow = 0;
            else
                visualRow = item->Row + 1 - (selectedRow < item->Row ? 1 : 0);
        }

        item->Rect.top = visualRow * Context->RowHeight;
        item->Rect.bottom = (visualRow + 1) * Context->RowHeight;
    }
}

VOID PhTabNewStretchRows(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ RECT const *ClientRect,
    _In_ PLONG RowCounts,
    _In_ PLONG RowWidths
    )
{
    if (Context->RowCount <= 1 || Context->Items->Count == 0)
        return;

    if (Context->TabFlags & PHTNF_THIN_TABS)
        return;

    LONG stripExtent = (LONG)(ClientRect->right - ClientRect->left);
    LONG r;

    for (r = 0; r < Context->RowCount; r++)
    {
        LONG rowCount = RowCounts[r];
        LONG rowTotalWidth = RowWidths[r];
        LONG slack;
        LONG extraEach;
        LONG extraRemainder;
        LONG cursor = 0;
        LONG seen = 0;

        if (rowCount == 0) continue;
        slack = stripExtent - rowTotalWidth;
        if (slack <= 0) continue;

        extraEach = slack / rowCount;
        extraRemainder = slack % rowCount;

        for (ULONG i = 0; i < Context->Items->Count; i++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
            if (item->Row != r) continue;

            LONG width = (item->Rect.right - item->Rect.left) + extraEach;
            if (seen < extraRemainder) width += 1;
            item->Rect.left = cursor;
            item->Rect.right = cursor + width;
            cursor += width;
            seen++;
        }
    }
}

VOID PhTabNewComputeStripThickness(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ RECT const *ClientRect
    )
{
    ULONG i;
    BOOLEAN vertical = (Context->Side == TNS_LEFT || Context->Side == TNS_RIGHT);

    if (vertical)
    {
        LONG maxWidth = Context->MinTabWidth;
        for (i = 0; i < Context->Items->Count; i++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
            LONG w = PhMeasureTabWidth(Context, Hdc, item);
            if (w > maxWidth) maxWidth = w;
        }
        Context->StripThickness = maxWidth;
        for (i = 0; i < Context->Items->Count; i++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
            item->Rect.right = Context->StripThickness;
        }
    }
    else
    {
        Context->StripThickness = Context->RowCount * Context->RowHeight;
    }
}

VOID PhTabNewComputePageRect(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ RECT const *ClientRect
    )
{
    RECT pageRect = *ClientRect;

    switch (Context->Side)
    {
    case TNS_TOP:
        pageRect.top += Context->StripThickness;
        break;
    case TNS_BOTTOM:
        pageRect.bottom -= Context->StripThickness;
        break;
    case TNS_LEFT:
        pageRect.left += Context->StripThickness;
        break;
    case TNS_RIGHT:
        pageRect.right -= Context->StripThickness;
        break;
    }

    Context->CachedPageRect = pageRect;
}

VOID PhTabNewLayout(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_opt_ HDC Hdc
    )
{
    RECT clientRect;
    HDC hdc = NULL;
    HFONT oldFont = NULL;
    LONG rowHeight = 0;
    LONG fixedTabWidth = 0;
    LONG *rowCounts = NULL;
    LONG *rowWidths = NULL;
    BOOLEAN vertical = FALSE;
    ULONG i;

    if (!PhGetClientRect(Context->WindowHandle, &clientRect))
        return;

    hdc = Hdc ? Hdc : GetDC(Context->WindowHandle);
    oldFont = SelectFont(hdc, Context->Font ? Context->Font : GetStockFont(DEFAULT_GUI_FONT));

    // Measure items and derive rowHeight / fixed width candidate
    PhTabNewMeasureItems(Context, hdc, &fixedTabWidth, &vertical, &rowHeight);

    // Allocate row arrays when horizontal and multiple items exist
    if (!vertical && Context->Items->Count > 0)
    {
        if (Context->Items->Count <= RTL_NUMBER_OF((LONG[PH_TABNEW_ROW_STACK_COUNT]){0}))
        {
            // use stack buffers via compound literal addresses; allocate small heap buffers instead for clarity
        }

        rowCounts = PhAllocateZero(sizeof(LONG) * Context->Items->Count);
        rowWidths = PhAllocateZero(sizeof(LONG) * Context->Items->Count);
    }

    // Compute logical rows and initial item rects
    PhTabNewComputeRows(Context, &clientRect, vertical, fixedTabWidth, rowCounts, rowWidths);

    // Reorder rows so selected row appears adjacent to the page edge
    PhTabNewApplyRowReordering(Context);

    // Stretch rows proportionally when required
    PhTabNewStretchRows(Context, &clientRect, rowCounts, rowWidths);

    // Right-align row 0 if requested (keep existing behavior)
    if (!vertical && (Context->StyleFlags & TNS_RIGHTALIGN) && Context->Items->Count > 0)
    {
        LONG stripExtent = (LONG)(clientRect.right - clientRect.left);
        LONG rowMaxRight = MINLONG;
        LONG shift;

        for (i = 0; i < Context->Items->Count; i++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
            if (item->Row != 0) continue;
            if (item->Rect.right > rowMaxRight) rowMaxRight = item->Rect.right;
        }

        shift = stripExtent - rowMaxRight;
        if (shift > 0)
        {
            for (i = 0; i < Context->Items->Count; i++)
            {
                PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
                if (item->Row != 0) continue;
                item->Rect.left += shift;
                item->Rect.right += shift;
            }
        }
    }

    // Compute strip thickness and page rect
    PhTabNewComputeStripThickness(Context, hdc, &clientRect);
    PhTabNewComputePageRect(Context, &clientRect);

    SelectFont(hdc, oldFont);
    if (!Hdc)
        ReleaseDC(Context->WindowHandle, hdc);

    if (rowCounts) PhFree(rowCounts);
    if (rowWidths) PhFree(rowWidths);

    Context->LayoutDirty = FALSE;
}

/**
 * Translates a tab's layout rectangle to client coordinates.
 *
 * \param Context A pointer to the tab control context.
 * \param Item A pointer to the tab item.
 * \param Rect A pointer to a RECT structure that receives the client coordinates.
 */
VOID PhTabNewGetItemRectInClient(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ PPH_TABNEW_INTERNAL_ITEM Item,
    _Out_ PRECT Rect
    )
{
    RECT clientRect;

    GetClientRect(Context->WindowHandle, &clientRect);

    *Rect = Item->Rect;

    switch (Context->Side)
    {
    case TNS_BOTTOM:
        {
            LONG h = clientRect.bottom - clientRect.top;
            LONG topOffset = h - (LONG)Context->StripThickness;
            Rect->top += topOffset;
            Rect->bottom += topOffset;
        }
        break;
    case TNS_RIGHT:
        {
            LONG w = clientRect.right - clientRect.left;
            LONG leftOffset = w - (LONG)Context->StripThickness;
            Rect->left += leftOffset;
            Rect->right += leftOffset;
        }
        break;
    case TNS_TOP:
    case TNS_LEFT:
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Hit testing
// ---------------------------------------------------------------------------

/**
 * Identifies which tab (if any) is at a given point.
 *
 * \param Context A pointer to the tab control context.
 * \param Point The point to test, in client coordinates.
 * \param Flags A pointer to a variable that receives hit-test result flags.
 * \return The index of the tab at the given point, or LONG_ERROR if no tab is present.
 */
LONG PhTabNewHitTest(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ POINT Point,
    _Out_opt_ PULONG Flags
    )
{
    ULONG i;
    ULONG flags = TNHT_NOWHERE;
    LONG result = LONG_ERROR;

    for (i = 0; i < Context->Items->Count; i++)
    {
        PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
        RECT itemRect;

        PhTabNewGetItemRectInClient(Context, item, &itemRect);

        if (PhPtInRect(&itemRect, &Point))
        {
            flags = TNHT_ONITEM | TNHT_ONLABEL;
            result = (LONG)i;
            break;
        }
    }

    if (Flags)
        *Flags = flags;

    return result;
}

// ---------------------------------------------------------------------------
// Notifications
// ---------------------------------------------------------------------------

/**
 * Dispatches a notification to the parent or callback.
 *
 * \param Context A pointer to the tab control context.
 * \param Code The notification code.
 * \param Header A pointer to the NMHDR structure.
 * \return The result of the notification.
 */
LRESULT PhTabNewDispatchNotify(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ ULONG Code,
    _Inout_ NMHDR *Header
    )
{
    Header->hwndFrom = Context->WindowHandle;
    Header->idFrom = Context->Id;
    Header->code = Code;

    if (Context->Callback)
    {
        return Context->Callback(Context->WindowHandle, Code, Header, NULL, Context->Context);
    }

    if (!Context->ParentHandle)
        return 0;

    return SendMessage(Context->ParentHandle, WM_NOTIFY, Header->idFrom, (LPARAM)Header);
}

/**
 * Sends a generic tab notification.
 *
 * \param Context A pointer to the tab control context.
 * \param Code The notification code.
 * \param ItemIndex The index of the item, or LONG_ERROR.
 * \return The result of the notification.
 */
LRESULT PhTabNewSendNotify(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ UINT Code,
    _In_ LONG ItemIndex
    )
{
    NMTABNEW nm;

    nm.ItemIndex = ItemIndex;
    nm.ItemParam = 0;

    if (ItemIndex >= 0 && (ULONG)ItemIndex < Context->Items->Count)
    {
        PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[ItemIndex];
        nm.ItemParam = item->Param;
    }

    return PhTabNewDispatchNotify(Context, Code, &nm.Header);
}

/**
 * Sends a PHTNN_DELETEITEM notification.
 *
 * \param Context A pointer to the tab control context.
 * \param ItemIndex The index occupied by the item before deletion.
 * \param ItemParam The application-defined value associated with the item.
 */
VOID PhTabNewSendDeleteNotify(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ LPARAM ItemParam
    )
{
    NMTABNEW nm;

    nm.ItemIndex = ItemIndex;
    nm.ItemParam = ItemParam;

    PhTabNewDispatchNotify(Context, PHTNN_DELETEITEM, &nm.Header);
}

/**
 * Sends a PHTNN_LAYOUT notification.
 *
 * \param Context A pointer to the tab control context.
 */
VOID PhTabNewSendLayoutNotify(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    NMTABNEWLAYOUT nm;

    if (Context->LayoutDirty)
    {
        PhTabNewLayout(Context, NULL);
    }

    nm.PageRect = Context->CachedPageRect;

    // Translate to parent client coords
    if (Context->ParentHandle)
    {
        MapWindowRect(Context->WindowHandle, Context->ParentHandle, &nm.PageRect);
    }

    PhTabNewDispatchNotify(Context, PHTNN_LAYOUT, &nm.Header);
}

/**
 * Changes the currently selected tab.
 *
 * \param Context A pointer to the tab control context.
 * \param NewIndex The index of the new selection.
 * \param Notify Whether to send selection change notifications.
 */
VOID PhTabNewSetSelection(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG NewIndex,
    _In_ BOOLEAN Notify
    )
{
    LONG oldIndex = Context->SelectedIndex;

    if (NewIndex < LONG_ERROR || NewIndex >= (LONG)Context->Items->Count)
        return;

    if (NewIndex == oldIndex)
        return;

    if (Notify)
    {
        LRESULT result = PhTabNewSendNotify(Context, PHTNN_SELCHANGING, NewIndex);

        if (result)
            return; // Cancel
    }

    Context->SelectedIndex = NewIndex;
    // Re-run layout so the multi-row reorder picks up the new selection
    // (the row containing the selected tab moves adjacent to the page).
    Context->LayoutDirty = TRUE;
    PhTabNewFlushLayout(Context, FALSE, FALSE);

    if (Notify)
    {
        PhTabNewSendNotify(Context, PHTNN_SELCHANGED, NewIndex);
    }
}

// ---------------------------------------------------------------------------
// Item management
// ---------------------------------------------------------------------------

/**
 * Inserts a new tab item.
 *
 * \param Context A pointer to the tab control context.
 * \param Index The index at which to insert the item.
 * \param InsertItem A pointer to a structure containing item information.
 * \return The index of the inserted item.
 */
LONG PhTabNewInsertItem(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG Index,
    _In_ PPH_TABNEW_INSERTITEM InsertItem
    )
{
    PPH_TABNEW_INTERNAL_ITEM item;
    LONG count = (LONG)Context->Items->Count;

    item = PhAllocateZero(sizeof(PH_TABNEW_INTERNAL_ITEM));
    item->ImageIndex = InsertItem->ImageIndex;
    item->Param = InsertItem->Param;

    if (InsertItem->Text)
        item->Text = PhCreateString(InsertItem->Text);

    if (Index < 0 || Index > count)
        Index = count;

    PhInsertItemList(Context->Items, Index, item);

    if (Context->SelectedIndex < 0)
        Context->SelectedIndex = Index;
    else if (Index <= Context->SelectedIndex)
        Context->SelectedIndex++;

    Context->LayoutDirty = TRUE;
    PhTabNewFlushLayout(Context, TRUE, TRUE);

    return Index;
}

/**
 * Deletes a tab item.
 *
 * \param Context A pointer to the tab control context.
 * \param Index The index of the item to delete.
 * \return TRUE if the item was deleted, FALSE otherwise.
 */
BOOL PhTabNewDeleteItem(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG Index
    )
{
    PPH_TABNEW_INTERNAL_ITEM item;
    PPH_TABNEW_PAGE page;

    if (Index < 0 || (ULONG)Index >= Context->Items->Count)
        return FALSE;

    item = Context->Items->Items[Index];
    page = item->Page;
    PhRemoveItemList(Context->Items, Index);

    PhTabNewSynchronizePages(Context);

    if (Context->SelectedIndex == Index)
    {
        LONG newSel = Index;

        if (newSel >= (LONG)Context->Items->Count)
            newSel = (LONG)Context->Items->Count - 1;

        Context->SelectedIndex = LONG_ERROR;
        PhTabNewSetSelection(Context, newSel, TRUE);
    }
    else if (Context->SelectedIndex > Index)
    {
        Context->SelectedIndex--;
    }

    if (Context->HotIndex >= (LONG)Context->Items->Count)
        Context->HotIndex = LONG_ERROR;

    Context->LayoutDirty = TRUE;
    PhTabNewFlushLayout(Context, TRUE, TRUE);

    PhTabNewSendDeleteNotify(Context, Index, item->Param);

    if (page)
    {
        if (page->Callback)
            page->Callback(page, PhTabNewPageDestroy, NULL, NULL);

        PhFree(page);
    }

    PhFreeTabNewItem(item);

    return TRUE;
}

/**
 * Moves a tab item from one index to another.
 *
 * \param Context A pointer to the tab control context.
 * \param FromIndex The current index of the item.
 * \param ToIndex The new index for the item.
 * \return TRUE if the item was moved, FALSE otherwise.
 */
BOOL PhTabNewMoveItem(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG FromIndex,
    _In_ LONG ToIndex,
    _In_ BOOLEAN NotifyLayout
    )
{
    PPH_TABNEW_INTERNAL_ITEM item;
    LONG count = (LONG)Context->Items->Count;

    if (FromIndex < 0 || FromIndex >= count)
        return FALSE;
    if (ToIndex < 0 || ToIndex >= count)
        return FALSE;
    if (FromIndex == ToIndex)
        return TRUE;

    item = Context->Items->Items[FromIndex];
    PhRemoveItemList(Context->Items, FromIndex);
    PhInsertItemList(Context->Items, ToIndex, item);
    PhTabNewSynchronizePages(Context);

    // Update selection index
    if (Context->SelectedIndex == FromIndex)
    {
        Context->SelectedIndex = ToIndex;
    }
    else
    {
        if (FromIndex < Context->SelectedIndex && ToIndex >= Context->SelectedIndex)
            Context->SelectedIndex--;
        else if (FromIndex > Context->SelectedIndex && ToIndex <= Context->SelectedIndex)
            Context->SelectedIndex++;
    }

    Context->LayoutDirty = TRUE;

    if (NotifyLayout)
        PhTabNewFlushLayout(Context, TRUE, TRUE);

    return TRUE;
}

// ---------------------------------------------------------------------------
// Drag and drop reorder (Ctrl+drag)
//
// Uses a ListView-style deferred reorder: while dragging, the underlying item
// order is left untouched. A floating drag image (built with the ImageList
// drag API) follows the cursor and an insertion marker shows the drop gap.
// The reorder is committed once, on drop.
// ---------------------------------------------------------------------------

/**
 * Builds a drag image (an ImageList) from the snapshot of a tab.
 *
 * \param Context A pointer to the tab control context.
 * \param ItemIndex The index of the item to snapshot.
 * \param Hotspot Receives the cursor offset within the tab (drag hotspot).
 * \param Point The current cursor position, in client coordinates.
 * \return The created ImageList, or NULL on failure. Caller destroys it.
 */
HIMAGELIST PhTabNewCreateDragImage(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _Out_ PPOINT Hotspot,
    _In_ POINT Point
    )
{
    PPH_TABNEW_INTERNAL_ITEM item;
    HIMAGELIST imageList = NULL;
    RECT itemRect;
    LONG width;
    LONG height;
    HDC screenDc;
    HDC memoryDc;
    HBITMAP bitmap;
    HBITMAP oldBitmap;
    HFONT oldFont;

    Hotspot->x = 0;
    Hotspot->y = 0;

    if (ItemIndex < 0 || (ULONG)ItemIndex >= Context->Items->Count)
        return NULL;

    item = Context->Items->Items[ItemIndex];
    PhTabNewGetItemRectInClient(Context, item, &itemRect);

    width = itemRect.right - itemRect.left;
    height = itemRect.bottom - itemRect.top;

    if (width <= 0 || height <= 0)
        return NULL;

    Hotspot->x = Point.x - itemRect.left;
    Hotspot->y = Point.y - itemRect.top;

    screenDc = GetDC(Context->WindowHandle);
    memoryDc = CreateCompatibleDC(screenDc);
    bitmap = CreateCompatibleBitmap(screenDc, width, height);
    oldBitmap = SelectBitmap(memoryDc, bitmap);

    // Render the tab into the bitmap with its client rect mapped to (0, 0).
    {
        RECT localRect = { 0, 0, width, height };

        FillRect(memoryDc, &localRect, Context->ActiveBrush ? Context->ActiveBrush : Context->BackgroundBrush);

        if (Context->OutlinePen)
        {
            HPEN oldPen = SelectPen(memoryDc, Context->OutlinePen);
            HBRUSH oldBrush = SelectBrush(memoryDc, GetStockBrush(NULL_BRUSH));
            Rectangle(memoryDc, 0, 0, width, height);
            SelectBrush(memoryDc, oldBrush);
            SelectPen(memoryDc, oldPen);
        }

        oldFont = SelectFont(memoryDc, Context->Font ? Context->Font : GetStockFont(DEFAULT_GUI_FONT));
        PhTabNewDrawItemContent(Context, memoryDc, item, &localRect, PhTabNewTextColor(Context));
        SelectFont(memoryDc, oldFont);
    }

    SelectBitmap(memoryDc, oldBitmap);
    DeleteDC(memoryDc);
    ReleaseDC(Context->WindowHandle, screenDc);

    imageList = PhImageListCreate(width, height, ILC_COLOR32, 1, 0);

    if (imageList)
        PhImageListAddBitmap(imageList, bitmap, NULL);

    DeleteBitmap(bitmap);

    return imageList;
}

/**
 * Computes the drop gap and insertion-marker rectangle for a cursor position.
 *
 * \param Context A pointer to the tab control context.
 * \param Point The cursor position, in client coordinates.
 * \param InsertIndex Receives the insertion gap in [0, Items->Count].
 * \param Marker Receives the insertion-marker rectangle, in client coordinates.
 */
VOID PhTabNewComputeDropTarget(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ POINT Point,
    _Out_ PLONG InsertIndex,
    _Out_ PRECT Marker
    )
{
    BOOLEAN vertical = (Context->Side == TNS_LEFT || Context->Side == TNS_RIGHT);
    LONG count = (LONG)Context->Items->Count;
    LONG gap = count;
    LONG hot;
    ULONG flags;
    PPH_TABNEW_INTERNAL_ITEM refItem;
    RECT refRect;
    LONG boundary;
    LONG half = Context->InsertMarkerWidth > 0 ? Context->InsertMarkerWidth / 2 : PH_TABNEW_PIXEL_OVERLAP;

    PhSetRectEmpty(Marker);

    if (count == 0)
    {
        *InsertIndex = 0;
        return;
    }

    hot = PhTabNewHitTest(Context, Point, &flags);

    if (hot >= 0)
    {
        PPH_TABNEW_INTERNAL_ITEM hotItem = Context->Items->Items[hot];
        RECT hotRect;
        LONG mid;

        PhTabNewGetItemRectInClient(Context, hotItem, &hotRect);

        if (vertical)
        {
            mid = (hotRect.top + hotRect.bottom) / 2;
            gap = (Point.y < mid) ? hot : hot + 1;
        }
        else
        {
            mid = (hotRect.left + hotRect.right) / 2;
            gap = (Point.x < mid) ? hot : hot + 1;
        }
    }
    else
    {
        // Not over a tab: snap to the nearest end on the cursor's row.
        gap = count;
    }

    if (gap < 0)
        gap = 0;
    if (gap > count)
        gap = count;

    *InsertIndex = gap;

    // Build the marker rectangle from the adjacent tab.
    if (gap < count)
    {
        refItem = Context->Items->Items[gap];
        PhTabNewGetItemRectInClient(Context, refItem, &refRect);
        boundary = vertical ? refRect.top : refRect.left;
    }
    else
    {
        refItem = Context->Items->Items[count - 1];
        PhTabNewGetItemRectInClient(Context, refItem, &refRect);
        boundary = vertical ? refRect.bottom : refRect.right;
    }

    if (vertical)
    {
        Marker->left = refRect.left;
        Marker->right = refRect.right;
        Marker->top = boundary - half;
        Marker->bottom = boundary + half + PH_TABNEW_PIXEL_OVERLAP;
    }
    else
    {
        Marker->top = refRect.top;
        Marker->bottom = refRect.bottom;
        Marker->left = boundary - half;
        Marker->right = boundary + half + PH_TABNEW_PIXEL_OVERLAP;
    }
}

/**
 * Starts a tab reorder drag operation.
 *
 * \param Context A pointer to the tab control context.
 * \param ItemIndex The index of the item being dragged.
 * \param Point The starting point of the drag, in client coordinates.
 */
VOID PhTabNewBeginDrag(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ POINT Point
    )
{
    if (ItemIndex < 0 || (ULONG)ItemIndex >= Context->Items->Count)
        return;

    Context->DragSourceIndex = ItemIndex;
    Context->DragTargetIndex = LONG_ERROR;
    Context->DragOriginIndex = ItemIndex;
    Context->DragStartPoint = Point;
    Context->DragArmed = TRUE;
    Context->DragActive = FALSE;
    PhSetRectEmpty(&Context->DragInsertMarker);

    SetCapture(Context->WindowHandle);
}

/**
 * Updates an active drag operation.
 *
 * \param Context A pointer to the tab control context.
 * \param Point The current point of the drag, in client coordinates.
 */
VOID PhTabNewUpdateDrag(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ POINT Point
    )
{
    LONG insertIndex;
    RECT marker;

    if (!Context->DragArmed)
        return;

    if (!Context->DragActive)
    {
        LONG dx = abs(Point.x - Context->DragStartPoint.x);
        LONG dy = abs(Point.y - Context->DragStartPoint.y);
        POINT hotspot;

        if (dx < GetSystemMetrics(SM_CXDRAG) && dy < GetSystemMetrics(SM_CYDRAG))
            return;

        Context->DragActive = TRUE;
        PhSetCursor(PhLoadCursor(NULL, IDC_SIZEALL));

        // Build and begin showing the floating drag image.
        Context->DragImageList = PhTabNewCreateDragImage(Context, Context->DragOriginIndex, &hotspot, Point);

        if (Context->DragImageList)
        {
            PhImageListBeginDrag(Context->DragImageList, 0, hotspot.x, hotspot.y);
            PhImageListDragEnter(Context->WindowHandle, Point.x, Point.y);
        }
    }

    PhTabNewComputeDropTarget(Context, Point, &insertIndex, &marker);

    // Repaint the insertion marker if the gap changed, hiding the drag image
    // around our own drawing so the ImageList compositor stays consistent.
    if (insertIndex != Context->DragTargetIndex)
    {
        if (Context->DragImageList)
            PhImageListDragShowNolock(FALSE);

        Context->DragTargetIndex = insertIndex;
        Context->DragInsertMarker = marker;
        InvalidateRect(Context->WindowHandle, NULL, FALSE);
        UpdateWindow(Context->WindowHandle);

        if (Context->DragImageList)
            PhImageListDragShowNolock(TRUE);
    }

    if (Context->DragImageList)
        PhImageListDragMove(Point.x, Point.y);
}

/**
 * Completes or cancels a drag operation.
 *
 * \param Context A pointer to the tab control context.
 * \param Cancel TRUE to cancel the operation and revert changes, FALSE to complete it.
 */
VOID PhTabNewEndDrag(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ BOOLEAN Cancel
    )
{
    LONG originalIndex = Context->DragOriginIndex;
    LONG insertIndex = Context->DragTargetIndex;
    BOOLEAN wasActive = !!Context->DragActive;
    LONG finalIndex = originalIndex;

    // Clear the drag state before releasing capture. ReleaseCapture delivers
    // WM_CAPTURECHANGED synchronously, which re-enters this function; clearing
    // the flags first makes that reentrant call a no-op instead of a spurious
    // second teardown.
    Context->DragArmed = FALSE;
    Context->DragActive = FALSE;

    // Tear down the floating drag image.
    if (Context->DragImageList)
    {
        PhImageListDragLeave(Context->WindowHandle);
        PhImageListEndDrag();
        PhImageListDestroy(Context->DragImageList);
        Context->DragImageList = NULL;
    }

    if (GetCapture() == Context->WindowHandle)
    {
        ReleaseCapture();
    }

    // Commit the reorder once, translating the insertion gap into a final index.
    if (wasActive && !Cancel && originalIndex >= 0 && insertIndex >= 0)
    {
        LONG target = (insertIndex > originalIndex) ? insertIndex - 1 : insertIndex;

        if (target != originalIndex)
        {
            PhTabNewMoveItem(Context, originalIndex, target, FALSE);
            finalIndex = target;
        }
    }

    PhSetRectEmpty(&Context->DragInsertMarker);

    if (wasActive && !Cancel && originalIndex >= 0 && originalIndex != finalIndex)
    {
        NMTABNEWREORDER nm;

        nm.FromIndex = originalIndex;
        nm.ToIndex = finalIndex;

        PhTabNewDispatchNotify(Context, PHTNN_REORDERED, &nm.Header);
    }

    if (wasActive)
    {
        PhTabNewFlushLayout(Context, TRUE, FALSE);
    }
    else if (!Context->LayoutSuspended)
    {
        InvalidateRect(Context->WindowHandle, NULL, FALSE);
    }

    Context->DragSourceIndex = LONG_ERROR;
    Context->DragTargetIndex = LONG_ERROR;
    Context->DragOriginIndex = LONG_ERROR;
}

/**
 * Gets the current background color.
 *
 * \param Context A pointer to the tab control context.
 * \return The background color.
 */
COLORREF PhTabNewBackgroundColor(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    if (Context->ThemeDark)
        return PhThemeWindowBackground2Color;
    return GetSysColor(COLOR_BTNFACE);
}

/**
 * Gets the current active page color.
 *
 * \param Context A pointer to the tab control context.
 * \return The active page color.
 */
COLORREF PhTabNewActiveColor(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    if (Context->ThemeDark)
        return PhThemeWindowBackgroundColor;
    return PhTabNewBackgroundColor(Context);
}

/**
 * Gets the current text color.
 *
 * \param Context A pointer to the tab control context.
 * \return The text color.
 */
COLORREF PhTabNewTextColor(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    if (Context->ThemeDark)
        return PhThemeWindowTextColor;
    return GetSysColor(COLOR_BTNTEXT);
}

/**
 * Frees GDI brushes and pens.
 *
 * \param Context A pointer to the tab control context.
 */
VOID PhTabNewDeleteCachedResources(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    if (Context->BackgroundBrush)
    {
        DeleteBrush(Context->BackgroundBrush);
        Context->BackgroundBrush = NULL;
    }

    if (Context->ActiveBrush)
    {
        DeleteBrush(Context->ActiveBrush);
        Context->ActiveBrush = NULL;
    }

    // Non-owned: clear the reference without freeing the shared/global brush.
    Context->WindowBackgroundBrush = NULL;

    if (Context->AccentBrush)
    {
        DeleteBrush(Context->AccentBrush);
        Context->AccentBrush = NULL;
    }

    if (Context->HotBrush)
    {
        DeleteBrush(Context->HotBrush);
        Context->HotBrush = NULL;
    }

    if (Context->OutlinePen)
    {
        DeletePen(Context->OutlinePen);
        Context->OutlinePen = NULL;
    }

    if (Context->BackgroundPen)
    {
        DeletePen(Context->BackgroundPen);
        Context->BackgroundPen = NULL;
    }

    if (Context->ActivePen)
    {
        DeletePen(Context->ActivePen);
        Context->ActivePen = NULL;
    }
}

/**
 * Recreates GDI brushes and pens based on current theme/colors.
 *
 * \param Context A pointer to the tab control context.
 */
VOID PhTabNewUpdateCachedResources(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    COLORREF backgroundColor;
    COLORREF activeColor;
    COLORREF accentColor;
    COLORREF hotColor;
    COLORREF outlineColor;

    PhTabNewDeleteCachedResources(Context);

    backgroundColor = PhTabNewBackgroundColor(Context);
    activeColor = PhTabNewActiveColor(Context);
    accentColor = Context->ThemeDark ? PhThemeWindowHighlightColor : GetSysColor(COLOR_HIGHLIGHT);
    hotColor = Context->ThemeDark ? PhThemeWindowHighlightColor : PH_TABNEW_LIGHT_HOT_COLOR;
    outlineColor = Context->ThemeDark ? PH_TABNEW_DARK_OUTLINE_COLOR : PH_TABNEW_LIGHT_OUTLINE_COLOR;

    Context->BackgroundBrush = CreateSolidBrush(backgroundColor);
    Context->ActiveBrush = CreateSolidBrush(activeColor);
    // Non-owned brush for the selected tab so it takes the window (white) color
    // and blends into the page area. Uses the shared theme window background when
    // theming is active, otherwise the system window brush (both non-owned).
    Context->WindowBackgroundBrush = (PhEnableThemeSupport && PhThemeWindowBackgroundBrush) ?
        PhThemeWindowBackgroundBrush : GetSysColorBrush(COLOR_WINDOW);
    Context->AccentBrush = CreateSolidBrush(accentColor);
    Context->HotBrush = CreateSolidBrush(hotColor);
    Context->OutlinePen = CreatePen(PS_SOLID, PH_TABNEW_PEN_WIDTH, outlineColor);
    Context->BackgroundPen = CreatePen(PS_SOLID, PH_TABNEW_PEN_WIDTH, backgroundColor);
    Context->ActivePen = CreatePen(PS_SOLID, PH_TABNEW_PEN_WIDTH, activeColor);
}

/**
 * Paints the text and icon of a tab.
 *
 * \param Context A pointer to the tab control context.
 * \param Hdc A handle to a device context.
 * \param Item A pointer to the tab item.
 * \param ItemRect A pointer to the rectangle where the item is painted.
 * \param TextColor The color of the text.
 */
VOID PhTabNewDrawItemContent(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PPH_TABNEW_INTERNAL_ITEM Item,
    _In_ PRECT ItemRect,
    _In_ COLORREF TextColor
    )
{
    RECT contentRect = *ItemRect;
    LONG iconCx = 0, iconCy = 0;

    // Skip the icon/text draw for tabs outside the DC's update region.
    if (!RectVisible(Hdc, ItemRect))
        return;

    contentRect.left += (LONG)Context->PaddingX;
    contentRect.right -= (LONG)Context->PaddingX;

    if (Item->ImageIndex >= 0 && Context->ImageList)
    {
        PhImageListGetIconSize(Context->ImageList, &iconCx, &iconCy);
        PhImageListDrawIcon(
            Context->ImageList,
            Item->ImageIndex,
            Hdc,
            contentRect.left,
            contentRect.top + ((contentRect.bottom - contentRect.top) - iconCy) / 2,
            ILD_NORMAL,
            FALSE
            );
        contentRect.left += iconCx + (LONG)Context->PaddingX;
    }

    if (Item->Text && Item->Text->Length > 0)
    {
        SetBkMode(Hdc, TRANSPARENT);
        SetTextColor(Hdc, TextColor);
        DrawText(
            Hdc,
            Item->Text->Buffer,
            (LONG)(Item->Text->Length / sizeof(WCHAR)),
            &contentRect,
            DT_SINGLELINE | DT_VCENTER | DT_CENTER
            );
    }
}

VOID PhTabNewDrawPageDivider(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect,
    _In_opt_ PRECT SelectedRect
    )
{
    HPEN oldPen;
    LONG dividerPos = 0;
    BOOLEAN vertical = (Context->Side == TNS_LEFT || Context->Side == TNS_RIGHT);

    oldPen = SelectPen(Hdc, Context->OutlinePen);

    switch (Context->Side)
    {
    case TNS_TOP:
        dividerPos = (LONG)Context->StripThickness - 1;
        break;
    case TNS_BOTTOM:
        dividerPos = (ClientRect->bottom - ClientRect->top) - (LONG)Context->StripThickness;
        break;
    case TNS_LEFT:
        dividerPos = (LONG)Context->StripThickness - 1;
        break;
    case TNS_RIGHT:
        dividerPos = (ClientRect->right - ClientRect->left) - (LONG)Context->StripThickness;
        break;
    }

    if (vertical)
    {
        MoveToEx(Hdc, dividerPos, ClientRect->top, NULL);
        LineTo(Hdc, dividerPos, ClientRect->bottom);
    }
    else
    {
        MoveToEx(Hdc, ClientRect->left, dividerPos, NULL);
        LineTo(Hdc, ClientRect->right, dividerPos);
    }

    if (SelectedRect)
    {
        SelectPen(Hdc, Context->ActivePen);

        switch (Context->Side)
        {
        case TNS_TOP:
            MoveToEx(Hdc, SelectedRect->left + 1, SelectedRect->bottom - 1, NULL);
            LineTo(Hdc, SelectedRect->right - 1, SelectedRect->bottom - 1);
            break;
        case TNS_BOTTOM:
            MoveToEx(Hdc, SelectedRect->left + 1, SelectedRect->top, NULL);
            LineTo(Hdc, SelectedRect->right - 1, SelectedRect->top);
            break;
        case TNS_LEFT:
            MoveToEx(Hdc, SelectedRect->right - 1, SelectedRect->top + 1, NULL);
            LineTo(Hdc, SelectedRect->right - 1, SelectedRect->bottom - 1);
            break;
        case TNS_RIGHT:
            MoveToEx(Hdc, SelectedRect->left, SelectedRect->top + 1, NULL);
            LineTo(Hdc, SelectedRect->left, SelectedRect->bottom - 1);
            break;
        }
    }

    SelectPen(Hdc, oldPen);
}

/**
 * Paints the control using the Windows 10 flat skin.
 *
 * \param Context A pointer to the tab control context.
 * \param Hdc A handle to a device context.
 * \param ClientRect A pointer to the client rectangle.
 */
VOID PhTabNewPaintWin10(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    )
{
    COLORREF text = PhTabNewTextColor(Context);
    ULONG i;

    FillRect(Hdc, ClientRect, Context->BackgroundBrush);

    for (i = 0; i < Context->Items->Count; i++)
    {
        PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
        RECT itemRect;
        BOOLEAN selected = ((LONG)i == Context->SelectedIndex);
        BOOLEAN isHot = ((LONG)i == Context->HotIndex);

        PhTabNewGetItemRectInClient(Context, item, &itemRect);

        if (selected)
        {
            FillRect(Hdc, &itemRect, Context->ActiveBrush);

            // Accent indicator
            {
                RECT accentRect = itemRect;
                LONG t = PH_TABNEW_ACCENT_THICKNESS;

                switch (Context->Side)
                {
                case TNS_TOP:    accentRect.top = accentRect.bottom - t; break;
                case TNS_BOTTOM: accentRect.bottom = accentRect.top + t; break;
                case TNS_LEFT:   accentRect.left = accentRect.right - t; break;
                case TNS_RIGHT:  accentRect.right = accentRect.left + t; break;
                }

                FillRect(Hdc, &accentRect, Context->AccentBrush);
            }
        }
        else if (isHot)
        {
            FillRect(Hdc, &itemRect, Context->HotBrush);
        }

        PhTabNewDrawItemContent(Context, Hdc, item, &itemRect, text);
    }
}

// Win7 Aero replica — classic Windows tab control look: a single horizontal
// divider line runs along the inner edge of the strip; inactive tabs share
// that line as their bottom edge; the selected tab erases the segment of
// the line beneath it and extends 1 px past so it visually merges with the
// page region.
/**
 * Paints the control using the Windows 7 Aero skin.
 *
 * \param Context A pointer to the tab control context.
 * \param Hdc A handle to a device context.
 * \param ClientRect A pointer to the client rectangle.
 */
VOID PhTabNewPaintWin7(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    )
{
    COLORREF text = PhTabNewTextColor(Context);
    COLORREF inacTop = Context->ThemeDark ? PH_TABNEW_DARK_INACTIVE_TOP : PH_TABNEW_LIGHT_INACTIVE_TOP;
    COLORREF inacBot = Context->ThemeDark ? PH_TABNEW_DARK_INACTIVE_BOTTOM : PH_TABNEW_LIGHT_INACTIVE_BOTTOM;
    COLORREF hotTop  = Context->ThemeDark ? PH_TABNEW_DARK_HOT_TOP : PH_TABNEW_LIGHT_HOT_TOP;
    COLORREF hotBot  = Context->ThemeDark ? PH_TABNEW_DARK_HOT_BOTTOM : PH_TABNEW_LIGHT_HOT_BOTTOM;
    HPEN oldPen;
    ULONG i;
    LONG selected = Context->SelectedIndex;
    BOOLEAN vertical = (Context->Side == TNS_LEFT || Context->Side == TNS_RIGHT);

    FillRect(Hdc, ClientRect, Context->BackgroundBrush);

    oldPen = SelectPen(Hdc, Context->OutlinePen);

    // Draw inactive + hot tabs first; selected last so it overlays the divider
    for (i = 0; i < Context->Items->Count; i++)
    {
        PPH_TABNEW_INTERNAL_ITEM item;
        RECT itemRect;
        TRIVERTEX vert[PH_TABNEW_TRIVERTEX_COUNT];
        GRADIENT_RECT gRect = { PH_TABNEW_GRADIENT_RECT_LOWER, PH_TABNEW_GRADIENT_RECT_UPPER };
        BOOLEAN isHot;
        COLORREF top, bot;
        RECT fillRect;

        if ((LONG)i == selected)
            continue;

        item = Context->Items->Items[i];
        isHot = ((LONG)i == Context->HotIndex);

        PhTabNewGetItemRectInClient(Context, item, &itemRect);

        // Inactive tabs sit shorter than the strip so the selected tab
        // visually rises above them (classic Aero "raised selected" effect).
        // The far edge is also inset so the divider shows through.
        fillRect = itemRect;

        switch (Context->Side)
        {
        case TNS_TOP:    fillRect.top += PH_TABNEW_AERO_TAB_INSET; fillRect.bottom -= PH_TABNEW_PIXEL_OVERLAP; break;
        case TNS_BOTTOM: fillRect.bottom -= PH_TABNEW_AERO_TAB_INSET; fillRect.top += PH_TABNEW_PIXEL_OVERLAP; break;
        case TNS_LEFT:   fillRect.left += PH_TABNEW_AERO_TAB_INSET; fillRect.right -= PH_TABNEW_PIXEL_OVERLAP; break;
        case TNS_RIGHT:  fillRect.right -= PH_TABNEW_AERO_TAB_INSET; fillRect.left += PH_TABNEW_PIXEL_OVERLAP; break;
        }

        if (isHot)
        {
            top = hotTop;
            bot = hotBot; 
        }
        else
        {
            top = inacTop;
            bot = inacBot;
        }

        vert[0].x = fillRect.left;  vert[0].y = fillRect.top;
        vert[0].Red   = (USHORT)(GetRValue(top) << PH_TABNEW_TRIVERTEX_COLOR_SHIFT);
        vert[0].Green = (USHORT)(GetGValue(top) << PH_TABNEW_TRIVERTEX_COLOR_SHIFT);
        vert[0].Blue  = (USHORT)(GetBValue(top) << PH_TABNEW_TRIVERTEX_COLOR_SHIFT);
        vert[0].Alpha = 0;
        vert[1].x = fillRect.right; vert[1].y = fillRect.bottom;
        vert[1].Red   = (USHORT)(GetRValue(bot) << PH_TABNEW_TRIVERTEX_COLOR_SHIFT);
        vert[1].Green = (USHORT)(GetGValue(bot) << PH_TABNEW_TRIVERTEX_COLOR_SHIFT);
        vert[1].Blue  = (USHORT)(GetBValue(bot) << PH_TABNEW_TRIVERTEX_COLOR_SHIFT);
        vert[1].Alpha = 0;

        //GradientFill(
        //    Hdc,
        //    vert,
        //    PH_TABNEW_TRIVERTEX_COUNT,
        //    &gRect,
        //    PH_TABNEW_PIXEL_OVERLAP,
        //    vertical ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V);

        // Outline — three edges only (skip the edge adjacent to the divider)
        SelectPen(Hdc, Context->OutlinePen);

        switch (Context->Side)
        {
        case TNS_TOP:
            MoveToEx(Hdc, fillRect.left, fillRect.bottom, NULL);
            LineTo(Hdc, fillRect.left, fillRect.top);
            LineTo(Hdc, fillRect.right - 1, fillRect.top);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom);
            break;
        case TNS_BOTTOM:
            MoveToEx(Hdc, fillRect.left, fillRect.top - 1, NULL);
            LineTo(Hdc, fillRect.left, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.right - 1, fillRect.top - 1);
            break;
        case TNS_LEFT:
            MoveToEx(Hdc, fillRect.right, fillRect.top, NULL);
            LineTo(Hdc, fillRect.left, fillRect.top);
            LineTo(Hdc, fillRect.left, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.right, fillRect.bottom - 1);
            break;
        case TNS_RIGHT:
            MoveToEx(Hdc, fillRect.left - 1, fillRect.top, NULL);
            LineTo(Hdc, fillRect.right - 1, fillRect.top);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.left - 1, fillRect.bottom - 1);
            break;
        }

        PhTabNewDrawItemContent(Context, Hdc, item, &fillRect, text);
    }

    // Now draw the divider line across the strip (after inactive fills so
    // it shows under them as their bottom edge).
    SelectPen(Hdc, Context->OutlinePen);

    {
        LONG dividerPos = 0;

        switch (Context->Side)
        {
        case TNS_TOP:    dividerPos = (LONG)Context->StripThickness - 1; break;
        case TNS_BOTTOM: dividerPos = (ClientRect->bottom - ClientRect->top) - (LONG)Context->StripThickness; break;
        case TNS_LEFT:   dividerPos = (LONG)Context->StripThickness - 1; break;
        case TNS_RIGHT:  dividerPos = (ClientRect->right - ClientRect->left) - (LONG)Context->StripThickness; break;
        }

        if (vertical)
        {
            MoveToEx(Hdc, dividerPos, ClientRect->top, NULL);
            LineTo(Hdc, dividerPos, ClientRect->bottom);
        }
        else
        {
            MoveToEx(Hdc, ClientRect->left, dividerPos, NULL);
            LineTo(Hdc, ClientRect->right, dividerPos);
        }
    }

    // Selected tab last — extends across the divider line
    if (selected >= 0 && (ULONG)selected < Context->Items->Count)
    {
        PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[selected];
        RECT itemRect;
        RECT fillRect;

        PhTabNewGetItemRectInClient(Context, item, &itemRect);

        // Selected fill extends 1 px past the divider so it merges into the page
        fillRect = itemRect;

        FillRect(Hdc, &fillRect, Context->ActiveBrush);

        // Outline — three edges; the edge facing the page is drawn in the
        // background color to erase the divider segment beneath the tab.
        SelectPen(Hdc, Context->OutlinePen);

        switch (Context->Side)
        {
        case TNS_TOP:
            MoveToEx(Hdc, fillRect.left, fillRect.bottom, NULL);
            LineTo(Hdc, fillRect.left, fillRect.top);
            LineTo(Hdc, fillRect.right - 1, fillRect.top);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom);
            // Erase the divider segment underneath
            SelectPen(Hdc, Context->ActivePen);
            MoveToEx(Hdc, fillRect.left + 1, fillRect.bottom - 1, NULL);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom - 1);
            break;
        case TNS_BOTTOM:
            MoveToEx(Hdc, fillRect.left, fillRect.top - 1, NULL);
            LineTo(Hdc, fillRect.left, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.right - 1, fillRect.top - 1);
            SelectPen(Hdc, Context->ActivePen);
            MoveToEx(Hdc, fillRect.left + 1, fillRect.top, NULL);
            LineTo(Hdc, fillRect.right - 1, fillRect.top);
            break;
        case TNS_LEFT:
            MoveToEx(Hdc, fillRect.right, fillRect.top, NULL);
            LineTo(Hdc, fillRect.left, fillRect.top);
            LineTo(Hdc, fillRect.left, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.right, fillRect.bottom - 1);
            SelectPen(Hdc, Context->ActivePen);
            MoveToEx(Hdc, fillRect.right - 1, fillRect.top + 1, NULL);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom - 1);
            break;
        case TNS_RIGHT:
            MoveToEx(Hdc, fillRect.left - 1, fillRect.top, NULL);
            LineTo(Hdc, fillRect.right - 1, fillRect.top);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.left - 1, fillRect.bottom - 1);
            SelectPen(Hdc, Context->ActivePen);
            MoveToEx(Hdc, fillRect.left, fillRect.top + 1, NULL);
            LineTo(Hdc, fillRect.left, fillRect.bottom - 1);
            break;
        }

        PhTabNewDrawItemContent(Context, Hdc, item, &itemRect, text);
    }

    SelectPen(Hdc, oldPen);
}

/**
 * Paints the control using the native UxTheme skin.
 *
 * \param Context A pointer to the tab control context.
 * \param Hdc A handle to a device context.
 * \param ClientRect A pointer to the client rectangle.
 */
VOID PhTabNewPaintUxTheme(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    )
{
    COLORREF text = PhTabNewTextColor(Context);
    ULONG i;

    if (!Context->ThemeHandle)
    {
        PhTabNewPaintWin10(Context, Hdc, ClientRect);
        return;
    }

    // Fill the strip first — the buffered DC is uninitialized memory
    // (renders as black). DrawThemeParentBackground only succeeds when the
    // parent paints its own client area; many of our parents don't.
    FillRect(Hdc, ClientRect, Context->BackgroundBrush);

    if (Context->ThemeDark)
    {
        RECT selectedRect;
        BOOLEAN hasSelectedRect = FALSE;

        for (i = 0; i < Context->Items->Count; i++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
            RECT itemRect;
            BOOLEAN selected = ((LONG)i == Context->SelectedIndex);
            BOOLEAN isHot = ((LONG)i == Context->HotIndex);

            PhTabNewGetItemRectInClient(Context, item, &itemRect);

            if (selected)
            {
                selectedRect = itemRect;
                hasSelectedRect = TRUE;
                continue;
            }

            if (isHot)
                FillRect(Hdc, &itemRect, Context->HotBrush);
            else
                FillRect(Hdc, &itemRect, Context->BackgroundBrush);

            PhTabNewDrawItemContent(Context, Hdc, item, &itemRect, text);
        }

        if (hasSelectedRect)
        {
            FillRect(Hdc, &selectedRect, Context->ActiveBrush);
            PhTabNewDrawPageDivider(Context, Hdc, ClientRect, &selectedRect);

            if (Context->SelectedIndex >= 0 && (ULONG)Context->SelectedIndex < Context->Items->Count)
            {
                PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[Context->SelectedIndex];

                PhTabNewDrawItemContent(Context, Hdc, item, &selectedRect, text);
            }
        }
        else
        {
            PhTabNewDrawPageDivider(Context, Hdc, ClientRect, NULL);
        }

        return;
    }

    PhDrawThemeParentBackground(Context->WindowHandle, Hdc, ClientRect);

    for (i = 0; i < Context->Items->Count; i++)
    {
        PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
        RECT itemRect;
        LONG stateId;
        BOOLEAN selected = ((LONG)i == Context->SelectedIndex);
        BOOLEAN isHot = ((LONG)i == Context->HotIndex);

        PhTabNewGetItemRectInClient(Context, item, &itemRect);

        if (selected)
        {
            stateId = TIS_SELECTED;
            FillRect(Hdc, &itemRect, Context->WindowBackgroundBrush);
        }
        else if (isHot)
        {
            stateId = TIS_HOT;
            PhDrawThemeBackground(Context->ThemeHandle, Hdc, TABP_TABITEM, stateId, &itemRect, NULL);
        }
        else
        {
            stateId = TIS_NORMAL;
            PhDrawThemeBackground(Context->ThemeHandle, Hdc, TABP_TABITEM, stateId, &itemRect, NULL);
        }


        PhTabNewDrawItemContent(Context, Hdc, item, &itemRect, text);
    }
}

/**
 * Dispatches painting to the appropriate skin handler.
 *
 * \param Context A pointer to the tab control context.
 * \param Hdc A handle to a device context.
 * \param ClientRect A pointer to the client rectangle.
 */
VOID PhTabNewPaint(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    )
{
    HFONT oldFont;

    if (Context->LayoutDirty)
    {
        PhTabNewLayout(Context, Hdc);
    }

    oldFont = SelectFont(Hdc, Context->Font ? Context->Font : GetStockFont(DEFAULT_GUI_FONT));

    switch (Context->Skin)
    {
    default:
    case PhTabNewSkinUxTheme:
        PhTabNewPaintUxTheme(Context, Hdc, ClientRect);
        break;
    case PhTabNewSkinWin10:
        PhTabNewPaintWin10(Context, Hdc, ClientRect);
        break;
    case PhTabNewSkinWin7:
        PhTabNewPaintWin7(Context, Hdc, ClientRect);
        break;
    }

    if (
        Context->HasFocus &&
        !(SendMessage(Context->WindowHandle, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS) &&
        Context->SelectedIndex >= 0 &&
        (ULONG)Context->SelectedIndex < Context->Items->Count
        )
    {
        PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[Context->SelectedIndex];
        RECT focusRect;

        PhTabNewGetItemRectInClient(Context, item, &focusRect);
        InflateRect(&focusRect, -3, -3);

        if (!PhRectEmpty(&focusRect))
            DrawFocusRect(Hdc, &focusRect);
    }

    // Draw the drop insertion marker on top of the strip during a drag.
    if (Context->DragActive && !PhRectEmpty(&Context->DragInsertMarker))
    {
        FillRect(Hdc, &Context->DragInsertMarker, Context->AccentBrush ? Context->AccentBrush : GetSysColorBrush(COLOR_HIGHLIGHT));
    }

    //if (Context->StyleFlags & TNS_DRAW_PANEL)
    //{
    //    NMTABNEWDRAWPANEL drawPanel;
    //
    //    memset(&drawPanel, 0, sizeof(NMTABNEWDRAWPANEL));
    //    drawPanel.hdc = Hdc;
    //    drawPanel.Rect = *ClientRect;
    //
    //    PhTabNewDispatchNotify(Context, PHTNN_DRAWPANEL, &drawPanel.Header);
    //}

    SelectFont(Hdc, oldFont);
}

static PPH_STRING PhpTabNewSaveLayout(
    _In_ PPH_TABNEW_CONTEXT TabContext,
    _In_ PPH_TABNEW_LAYOUT_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i;

    PhInitializeStringBuilder(&stringBuilder, PH_TABNEW_LAYOUT_BUILDER_SIZE);

    for (i = 0; i < TabContext->Items->Count; i++)
    {
        PH_STRINGREF identifier;

        if (!PhTabNewGetItemLayoutIdentifier(TabContext, (LONG)i, Callback, Context, &identifier))
            continue;
        if (!identifier.Buffer || identifier.Length == 0)
            continue;

        PhAppendFormatStringBuilder(&stringBuilder, L"%lu:", (ULONG)(identifier.Length / sizeof(WCHAR)));
        PhAppendStringBuilder(&stringBuilder, &identifier);
        PhAppendCharStringBuilder(&stringBuilder, L';');
    }

    if (stringBuilder.String && stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

static BOOLEAN PhpTabNewRestoreLayout(
    _In_ PPH_TABNEW_CONTEXT TabContext,
    _In_ PPH_STRINGREF Layout,
    _In_ PPH_TABNEW_LAYOUT_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PPH_LIST orderedItems;
    PPH_LIST remainingItems;
    PH_STRINGREF remaining;
    ULONG i;
    BOOLEAN changed = FALSE;

    if (!Layout || !Layout->Buffer || Layout->Length == 0 || !Callback)
        return FALSE;
    if (TabContext->Items->Count == 0)
        return FALSE;

    orderedItems = PhCreateList(TabContext->Items->Count);
    remainingItems = PhCreateList(TabContext->Items->Count);
    remaining = *Layout;

    for (i = 0; i < TabContext->Items->Count; i++)
    {
        PhAddItemList(remainingItems, TabContext->Items->Items[i]);
    }

    while (remaining.Length != 0)
    {
        PH_STRINGREF token;
        ULONG j;

        if (!PhTabNewReadLayoutToken(&remaining, &token))
            break;

        for (j = 0; j < remainingItems->Count; j++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = remainingItems->Items[j];
            PH_STRINGREF identifier;

            if (!Callback(TabContext->WindowHandle, item->Param, &identifier, Context))
                continue;
            if (identifier.Length != token.Length)
                continue;
            if (identifier.Length != 0 && memcmp(identifier.Buffer, token.Buffer, token.Length) != 0)
                continue;

            PhAddItemList(orderedItems, item);
            PhRemoveItemList(remainingItems, j);
            changed = TRUE;
            break;
        }
    }

    for (i = 0; i < remainingItems->Count; i++)
    {
        PhAddItemList(orderedItems, remainingItems->Items[i]);
    }

    if (orderedItems->Count == TabContext->Items->Count)
    {
        for (i = 0; i < orderedItems->Count; i++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = orderedItems->Items[i];
            ULONG currentIndex;

            currentIndex = PhFindItemList(TabContext->Items, item);
            if (currentIndex == ULONG_MAX)
                continue;

            if (currentIndex != i)
            {
                PhTabNewMoveItem(TabContext, (LONG)currentIndex, (LONG)i, FALSE);
                changed = TRUE;
            }
        }
    }

    PhDereferenceObject(orderedItems);
    PhDereferenceObject(remainingItems);

    if (changed)
    {
        PhTabNewLayout(TabContext, NULL);
        PhTabNewSendLayoutNotify(TabContext);
        InvalidateRect(TabContext->WindowHandle, NULL, TRUE);
    }

    return changed;
}

// ---------------------------------------------------------------------------
// Page helper layer
// ---------------------------------------------------------------------------

/**
 * Adds a new page to the tab control.
 *
 * \param TabControl A handle to the tab control window.
 * \param Name The name of the page to add.
 * \param Flags Flags associated with the page.
 * \param Callback A callback function for the page.
 * \param Context An optional context parameter passed to the callback.
 * \return A pointer to the created page structure, or NULL on failure.
 */
static PPH_TABNEW_PAGE PhpTabNewAddPage(
    _In_ PPH_TABNEW_CONTEXT TabContext,
    _In_ PPH_STRINGREF Name,
    _In_ ULONG Flags,
    _In_ PPH_TABNEW_PAGE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PPH_TABNEW_PAGE page;
    PH_TABNEW_INSERTITEM item;
    WCHAR nameBuffer[PH_TABNEW_PAGE_NAME_LENGTH];
    LONG index;

    page = PhAllocateZero(sizeof(PH_TABNEW_PAGE));
    page->Name = *Name;
    page->Flags = Flags;
    page->Callback = Callback;
    page->Context = Context;
    page->TabControl = TabContext->WindowHandle;
    page->Index = (LONG)TabContext->Pages->Count;

    // Insert into tab strip
    if (Name->Length / sizeof(WCHAR) >= RTL_NUMBER_OF(nameBuffer))
        nameBuffer[RTL_NUMBER_OF(nameBuffer) - 1] = 0;
    else
        nameBuffer[Name->Length / sizeof(WCHAR)] = 0;
    memcpy(nameBuffer, Name->Buffer, min(Name->Length, sizeof(nameBuffer) - sizeof(WCHAR)));
    nameBuffer[min(Name->Length / sizeof(WCHAR), RTL_NUMBER_OF(nameBuffer) - 1)] = 0;

    item.Text = nameBuffer;
    item.ImageIndex = LONG_ERROR;
    item.Param = (LPARAM)page;
    index = PhTabNewInsertItem(TabContext, LONG_ERROR, &item);
    ((PPH_TABNEW_INTERNAL_ITEM)TabContext->Items->Items[index])->Page = page;
    PhTabNewSynchronizePages(TabContext);

    if (Callback)
        Callback(page, PhTabNewPageCreate, NULL, NULL);

    return page;
}

/**
 * Finds a page in the tab control by name.
 *
 * \param TabControl A handle to the tab control window.
 * \param Name The name of the page to find.
 * \return A pointer to the page structure if found, otherwise NULL.
 */
static PPH_TABNEW_PAGE PhpTabNewFindPage(
    _In_ PPH_TABNEW_CONTEXT TabContext,
    _In_ PPH_STRINGREF Name
    )
{
    for (ULONG i = 0; i < TabContext->Pages->Count; i++)
    {
        PPH_TABNEW_PAGE page = TabContext->Pages->Items[i];

        if (PhEqualStringRef(&page->Name, Name, TRUE))
            return page;
    }

    return NULL;
}

/**
 * Retrieves a page in the tab control by index.
 *
 * \param TabControl A handle to the tab control window.
 * \param Index The index of the page to retrieve.
 * \return A pointer to the page structure if found, otherwise NULL.
 */
PPH_TABNEW_PAGE PhpTabNewGetPageByIndex(
    _In_ PPH_TABNEW_CONTEXT TabContext,
    _In_ LONG Index
    )
{
    if (Index < 0 || (ULONG)Index >= TabContext->Pages->Count)
        return NULL;

    return TabContext->Pages->Items[Index];
}

/**
 * Selects a specific page in the tab control.
 *
 * \param TabControl A handle to the tab control window.
 * \param Page A pointer to the page to select.
 */
VOID PhpTabNewSelectPage(
    _In_ PPH_TABNEW_CONTEXT TabContext,
    _In_ PPH_TABNEW_PAGE Page
    )
{
    //PhTabNew_SetCurSel(TabControl, Page->Index);
}

/**
 * Sends a message to all pages in the tab control.
 *
 * \param TabContext A handle to the tab control window.
 * \param Message The message to send.
 * \param Parameter1 The first message parameter.
 * \param Parameter2 The second message parameter.
 */
VOID PhpTabNewNotifyAllPages(
    _In_ PPH_TABNEW_CONTEXT TabContext,
    _In_ PH_TABNEW_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    ULONG i;

    for (i = 0; i < TabContext->Pages->Count; i++)
    {
        PPH_TABNEW_PAGE page = TabContext->Pages->Items[i];

        if (page->Callback)
        {
            page->Callback(page, Message, Parameter1, Parameter2);
        }
    }
}

/**
 * Retrieves the currently selected page in the tab control.
 *
 * \param TabContext A handle to the tab control window.
 * \return A pointer to the currently selected page, or NULL if none is selected.
 */
PPH_TABNEW_PAGE PhpTabNewGetCurrentPage(
    _In_ PPH_TABNEW_CONTEXT TabContext
    )
{
    LONG index;

    index = TabContext->SelectedIndex;
    if (index < 0 || (ULONG)index >= TabContext->Items->Count)
        return NULL;

    return ((PPH_TABNEW_INTERNAL_ITEM)TabContext->Items->Items[index])->Page;
}

// ---------------------------------------------------------------------------
// WndProc
// ---------------------------------------------------------------------------

LRESULT PhTabNewOnUserMessage(
    _In_ HWND WindowHandle,
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ ULONG Message,
    _In_ WPARAM WParam,
    _In_ LPARAM LParam
    )
{
    switch (Message)
    {
    case PHTNM_INSERTITEM:
        return (LRESULT)PhTabNewInsertItem(Context, (LONG)WParam, (PPH_TABNEW_INSERTITEM)LParam);
    case PHTNM_DELETEITEM:
        return (LRESULT)PhTabNewDeleteItem(Context, (LONG)WParam);
    case PHTNM_DELETEALLITEMS:
        {
            PhTabNewDeleteAllItems(Context);

            Context->LayoutDirty = TRUE;

            PhTabNewFlushLayout(Context, TRUE, TRUE);
        }
        return TRUE;
    case PHTNM_GETITEMCOUNT:
        return (LRESULT)Context->Items->Count;
    case PHTNM_SETCURSEL:
        {
            LONG oldSel = Context->SelectedIndex;
            PhTabNewSetSelection(Context, (LONG)WParam, TRUE);
            return (LRESULT)oldSel;
        }
    case PHTNM_GETCURSEL:
        return (LRESULT)Context->SelectedIndex;
    case PHTNM_SETITEMTEXT:
        {
            LONG idx = (LONG)WParam;
            PWSTR text = (PWSTR)LParam;
            PPH_TABNEW_INTERNAL_ITEM item;

            if (idx < 0 || (ULONG)idx >= Context->Items->Count)
                return FALSE;

            item = Context->Items->Items[idx];
            PhClearReference(&item->Text);
            if (text) item->Text = PhCreateString(text);
            Context->LayoutDirty = TRUE;

            PhTabNewFlushLayout(Context, FALSE, TRUE);
        }
        return TRUE;
    case PHTNM_GETITEMPARAM:
        {
            LONG idx = (LONG)WParam;
            PPH_TABNEW_INTERNAL_ITEM item;

            if (idx < 0 || (ULONG)idx >= Context->Items->Count)
                return 0;

            item = Context->Items->Items[idx];
            return (LRESULT)item->Param;
        }
    case PHTNM_SETITEMPARAM:
        {
            LONG idx = (LONG)WParam;
            PPH_TABNEW_INTERNAL_ITEM item;

            if (idx < 0 || (ULONG)idx >= Context->Items->Count)
                return FALSE;

            item = Context->Items->Items[idx];
            item->Param = (LPARAM)LParam;
        }
        return TRUE;
    case PHTNM_GETPAGERECT:
        {
            PRECT outRect = (PRECT)LParam;

            if (!outRect) return FALSE;

            if (Context->LayoutDirty)
            {
                PhTabNewLayout(Context, NULL);
            }

            *outRect = Context->CachedPageRect;

            if (Context->ParentHandle)
            {
                MapWindowPoints(WindowHandle, Context->ParentHandle, (POINT*)outRect, 2);
            }
        }
        return TRUE;
    case PHTNM_SETSKIN:
        {
            Context->Skin = (PH_TABNEW_SKIN)WParam;
            
            PhTabNewUpdateTheme(Context);
            PhTabNewUpdateCachedResources(Context);
            PhTabNewUpdateMetrics(Context);
            
            Context->LayoutDirty = TRUE;
            
            PhTabNewFlushLayout(Context, TRUE, TRUE);
        }
        return TRUE;

    case PHTNM_GETSKIN:
        return (LRESULT)Context->Skin;
    case PHTNM_SETSIDE:
        {
            Context->Side = (ULONG)WParam & TNS_SIDE_MASK;

            PhTabNewUpdateTheme(Context);
            PhTabNewUpdateCachedResources(Context);
            PhTabNewUpdateMetrics(Context);

            Context->LayoutDirty = TRUE;

            PhTabNewFlushLayout(Context, TRUE, TRUE);
        }
        return TRUE;
    case PHTNM_GETSIDE:
        return (LRESULT)Context->Side;
    case PHTNM_HITTEST:
        {
            PPH_TABNEW_HITTESTINFO info = (PPH_TABNEW_HITTESTINFO)LParam;
            if (!info) return LONG_ERROR;
            info->ItemIndex = PhTabNewHitTest(Context, info->Point, &info->Flags);
            return (LRESULT)info->ItemIndex;
        }
    case PHTNM_MOVEITEM:
        return (LRESULT)PhTabNewMoveItem(Context, (LONG)WParam, (LONG)LParam, TRUE);
    case PHTNM_SETIMAGELIST:
        {
            HIMAGELIST old = Context->ImageList;
            Context->ImageList = (HIMAGELIST)WParam;
            Context->LayoutDirty = TRUE;

            PhTabNewFlushLayout(Context, FALSE, TRUE);

            return (LRESULT)old;
        }
    case PHTNM_GETIMAGELIST:
        return (LRESULT)Context->ImageList;
    case PHTNM_SETPADDING:
        {
            Context->BasePaddingX = (LONG)WParam;
            Context->BasePaddingY = (LONG)LParam;

            PhTabNewUpdateMetrics(Context);
            Context->LayoutDirty = TRUE;

            PhTabNewFlushLayout(Context, FALSE, TRUE);
        }
        return TRUE;
    case PHTNM_SETMINTABWIDTH:
        {
            Context->BaseMinTabWidth = (LONG)WParam;

            PhTabNewUpdateMetrics(Context);

            Context->LayoutDirty = TRUE;

            PhTabNewFlushLayout(Context, FALSE, TRUE);
        }
        return TRUE;
    case PHTNM_GETROWCOUNT:
        return (LRESULT)Context->RowCount;
    case PHTNM_INVALIDATEITEM:
        {
            LONG idx = (LONG)WParam;
            PPH_TABNEW_INTERNAL_ITEM item;
            RECT itemRect;

            if (idx < 0 || (ULONG)idx >= Context->Items->Count)
                return 0;

            item = Context->Items->Items[idx];

            PhTabNewGetItemRectInClient(Context, item, &itemRect);
            InvalidateRect(WindowHandle, &itemRect, FALSE);
        }
        return 0;
    case PHTNM_SETTHEMEDARK:
        {
            Context->ThemeDark = !!WParam;

            PhTabNewUpdateCachedResources(Context);
            InvalidateRect(WindowHandle, NULL, TRUE);
        }
        return 0;
    case PHTNM_SETCALLBACK:
        {
            Context->Callback = (PPH_TABNEW_MESSAGE_CALLBACK)WParam;
            Context->Context = (PVOID)LParam;
        }
        return TRUE;
    case PHTNM_SAVELAYOUT:
        {
            PPH_TABNEW_LAYOUT_MESSAGE message = (PPH_TABNEW_LAYOUT_MESSAGE)LParam;

            if (!message)
                return 0;

            return (LRESULT)PhpTabNewSaveLayout(Context, message->Callback, message->Context);
        }
        break;
    case PHTNM_RESTORELAYOUT:
        {
            PPH_TABNEW_LAYOUT_MESSAGE message = (PPH_TABNEW_LAYOUT_MESSAGE)LParam;

            if (!message)
                return FALSE;

            return (LRESULT)PhpTabNewRestoreLayout(Context, message->Layout, message->Callback, message->Context);
        }
        break;
    case PHTNM_ADDPAGE:
        {
            PPH_TABNEW_ADD_PAGE_MESSAGE message = (PPH_TABNEW_ADD_PAGE_MESSAGE)LParam;

            if (!message || !message->Name)
                return 0;

            return (LRESULT)PhpTabNewAddPage(Context, message->Name, message->Flags, message->Callback, message->Context);
        }
        break;
    case PHTNM_FINDPAGE:
        {
            PPH_STRINGREF name = (PPH_STRINGREF)LParam;

            if (!name)
                return 0;

            return (LRESULT)PhpTabNewFindPage(Context, name);
        }
        break;
    case PHTNM_GETPAGEBYINDEX:
        return (LRESULT)PhpTabNewGetPageByIndex(Context, (LONG)WParam);
    case PHTNM_NOTIFYPAGES:
        {
            PPH_TABNEW_NOTIFY_PAGES_MESSAGE message = (PPH_TABNEW_NOTIFY_PAGES_MESSAGE)LParam;

            if (!message)
                return 0;

            PhpTabNewNotifyAllPages(Context, message->Message, message->Parameter1, message->Parameter2);
        }
        return 0;
    case PHTNM_GETCURRENTPAGE:
        return (LRESULT)PhpTabNewGetCurrentPage(Context);
    case PHTNM_SELECTPAGE:
        {
            PPH_TABNEW_PAGE page = (PPH_TABNEW_PAGE)WParam;

            if (!page)
                return 0;

            PhpTabNewSelectPage(Context, page);
        }
        return 0;
    case PHTNM_SETFLAGS:
        {
            ULONG oldFlags = Context->TabFlags;

            Context->TabFlags = (ULONG)WParam;

            PhTabNewUpdateMetrics(Context);

            Context->LayoutDirty = TRUE;

            PhTabNewFlushLayout(Context, TRUE, TRUE);

            return (LRESULT)oldFlags;
        }
    case TCM_GETCURSEL:
        return (LRESULT)Context->SelectedIndex;
    case TCM_SETCURSEL:
        {
            LONG old = Context->SelectedIndex;
            PhTabNewSetSelection(Context, (LONG)WParam, FALSE);
            return (LRESULT)old;
        }
    case TCM_GETITEMCOUNT:
        return (LRESULT)Context->Items->Count;
    case TCM_DELETEITEM:
        return (LRESULT)PhTabNewDeleteItem(Context, (LONG)WParam);
    case TCM_DELETEALLITEMS:
        {
            PhTabNewDeleteAllItems(Context);

            Context->LayoutDirty = TRUE;

            PhTabNewFlushLayout(Context, TRUE, TRUE);
        }
        return TRUE;
    case TCM_INSERTITEMW:
        {
            TCITEMW *tci = (TCITEMW *)LParam;

            PH_TABNEW_INSERTITEM ins;
            ins.Text = (tci && (tci->mask & TCIF_TEXT)) ? tci->pszText : L"";
            ins.ImageIndex = (tci && (tci->mask & TCIF_IMAGE)) ? tci->iImage : LONG_ERROR;
            ins.Param = (tci && (tci->mask & TCIF_PARAM)) ? tci->lParam : 0;
            return (LRESULT)PhTabNewInsertItem(Context, (LONG)WParam, &ins);
        }
        break;
    case TCM_ADJUSTRECT:
        {
            PRECT rc = (PRECT)LParam;

            if (!rc)
            {
                return 0;
            }

            if (Context->LayoutDirty)
            {
                PhTabNewLayout(Context, NULL);
            }

            // wParam: TRUE=add tab padding to display->window; FALSE=subtract
            if ((BOOL)WParam)
            {
                switch (Context->Side)
                {
                case TNS_TOP:    
                    rc->top -= (LONG)Context->StripThickness; 
                    break;
                case TNS_BOTTOM: 
                    rc->bottom += (LONG)Context->StripThickness; 
                    break;
                case TNS_LEFT:   
                    rc->left -= (LONG)Context->StripThickness; 
                    break;
                case TNS_RIGHT:  
                    rc->right += (LONG)Context->StripThickness; 
                    break;
                }
            }
            else
            {
                switch (Context->Side)
                {
                case TNS_TOP:    
                    rc->top += (LONG)Context->StripThickness; 
                    break;
                case TNS_BOTTOM: 
                    rc->bottom -= (LONG)Context->StripThickness; 
                    break;
                case TNS_LEFT:   
                    rc->left += (LONG)Context->StripThickness; 
                    break;
                case TNS_RIGHT:  
                    rc->right -= (LONG)Context->StripThickness; 
                    break;
                }
            }
        }
        return 0;
    case TCM_GETITEMRECT:
        {
            LONG idx = (LONG)WParam;
            PRECT rc = (PRECT)LParam;
            PPH_TABNEW_INTERNAL_ITEM item;

            if (!rc || idx < 0 || (ULONG)idx >= Context->Items->Count)
                return FALSE;

            item = Context->Items->Items[idx];
            PhTabNewGetItemRectInClient(Context, item, rc);
        }
        return TRUE;
    case TCM_GETROWCOUNT:
        return (LRESULT)Context->RowCount;
    }

    return DefWindowProc(WindowHandle, Message, WParam, LParam);
}

/**
 * Sends a message to a TabNew window, handling custom messages if possible.
 *
 * \param WindowHandle A handle to the window.
 * \param WindowMessage The message to send.
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \return The result of the message processing.
 */
LRESULT PhTabNewSendMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG WindowMessage,
    _Pre_maybenull_ _Post_valid_ WPARAM wParam,
    _Pre_maybenull_ _Post_valid_ LPARAM lParam
    )
{
    if (WindowMessage >= PHTNM_FIRST && WindowMessage <= PHTNM_LAST)
    {
        PPH_TABNEW_CONTEXT context;

        if (context = PhGetWindowContextEx(WindowHandle))
        {
            return PhTabNewOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);
        }
    }

#if defined(DEBUG)
    assert(FALSE);
#endif

    return SendMessage(WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Main window procedure for the tab control.
 *
 * \param WindowHandle A handle to the window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \return The result of the message processing.
 */
LRESULT CALLBACK PhTabNewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_TABNEW_CONTEXT context = PhGetWindowContextEx(WindowHandle);

    switch (WindowMessage)
    {
    case WM_NCCREATE:
        {
            CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
            ULONG style = (ULONG)cs->style;

            context = PhCreateTabNewContext();
            context->WindowHandle = WindowHandle;
            context->ParentHandle = GetParent(WindowHandle);
            context->Id = (ULONG_PTR)cs->hMenu;
            context->Side = style & TNS_SIDE_MASK;
            context->StyleFlags = style & ~TNS_SIDE_MASK;
            context->WindowDpi = PhGetWindowDpi(WindowHandle);
            context->StyleFlags |= TNS_REORDER;

            if (cs->lpCreateParams)
            {
                PPH_TABNEW_CREATEPARAMS params = cs->lpCreateParams;

                if (RTL_CONTAINS_FIELD(params, params->Size, Callback))
                {
                    if (params->Callback)
                        context->Callback = params->Callback;
                }

                if (RTL_CONTAINS_FIELD(params, params->Size, Context))
                {
                    if (params->Context)
                        context->Context = params->Context;
                }
            }

            PhSetWindowContextEx(WindowHandle, context);
            PhTabNewUpdateFont(context);
            PhTabNewUpdateTheme(context);
            PhTabNewUpdateMetrics(context);
            PhTabNewUpdateCachedResources(context);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContextEx(WindowHandle);
            PhDestroyTabNewContext(context);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT clientRect;
            HDC hdc;
            PH_BUFFERED_PAINT paintBuffer;
            HDC bufferDc;

            hdc = BeginPaint(WindowHandle, &ps);
            if (!hdc) break;

            if (PhRectEmpty(&ps.rcPaint))
            {
                EndPaint(WindowHandle, &ps);
                break;
            }

            GetClientRect(WindowHandle, &clientRect);

            // Buffer only the invalidated region; PhTabNewPaint still lays out using the
            // full client rect (window coordinates) and is clipped to the rcPaint buffer.
            if (PhBeginBufferedPaint(hdc, &ps.rcPaint, &paintBuffer, &bufferDc))
            {
                PhTabNewPaint(context, bufferDc, &clientRect);
                PhEndBufferedPaint(&paintBuffer, TRUE);
            }
            else
            {
                PhTabNewPaint(context, hdc, &clientRect);
            }

            EndPaint(WindowHandle, &ps);
        }
        return 0;
    case WM_SIZE:
        {
            context->LayoutDirty = TRUE;

            if (wParam != SIZE_MINIMIZED)
                PhTabNewFlushLayout(context, TRUE, FALSE);
        }
        break;
    case WM_SETFOCUS:
        context->HasFocus = TRUE;
        InvalidateRect(WindowHandle, NULL, FALSE);
        break;
    case WM_KILLFOCUS:
        context->HasFocus = FALSE;
        InvalidateRect(WindowHandle, NULL, FALSE);
        break;
    case WM_UPDATEUISTATE:
        {
            LRESULT result = DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);

            InvalidateRect(WindowHandle, NULL, FALSE);
            return result;
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            context->WindowDpi = PhGetWindowDpi(WindowHandle);

            PhTabNewUpdateFont(context);
            PhTabNewUpdateTheme(context);
            PhTabNewUpdateMetrics(context);
            PhTabNewUpdateCachedResources(context);

            context->LayoutDirty = TRUE;

            PhTabNewFlushLayout(context, TRUE, TRUE);
        }
        break;
    case WM_THEMECHANGED:
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
        {
            PhTabNewUpdateTheme(context);
            PhTabNewUpdateCachedResources(context);

            PhTabNewUpdateMetrics(context);
            context->LayoutDirty = TRUE;

            PhTabNewFlushLayout(context, TRUE, TRUE);
        }
        break;
    case WM_SETREDRAW:
        {
            LRESULT result = DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);

            if (wParam)
            {
                BOOLEAN wasSuspended = !!context->LayoutSuspended;

                context->LayoutSuspended = FALSE;

                if (wasSuspended && context->LayoutDirty)
                {
                    PhTabNewFlushLayout(context, TRUE, TRUE);
                }
            }
            else
            {
                context->LayoutSuspended = TRUE;
            }

            return result;
        }
        break;
    case WM_MOUSEMOVE:
        {
            POINT pt;
            LONG hit;
            LONG oldHot = context->HotIndex;

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            if (context->DragArmed || context->DragActive)
            {
                PhTabNewUpdateDrag(context, pt);
                break;
            }

            hit = PhTabNewHitTest(context, pt, NULL);
            context->HotIndex = hit;

            if (oldHot != hit)
            {
                InvalidateRect(WindowHandle, NULL, FALSE);
            }

            if (!context->TrackingMouse)
            {
                TRACKMOUSEEVENT tme = { sizeof(tme) };
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = WindowHandle;
                TrackMouseEvent(&tme);

                context->TrackingMouse = TRUE;
            }
        }
        break;
    case WM_MOUSELEAVE:
        {
            context->TrackingMouse = FALSE;

            if (context->HotIndex != LONG_ERROR)
            {
                context->HotIndex = LONG_ERROR;
                InvalidateRect(WindowHandle, NULL, FALSE);
            }
        }
        break;
    case WM_LBUTTONDOWN:
        {
            POINT pt;
            LONG hit;

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            hit = PhTabNewHitTest(context, pt, NULL);

            if (hit < 0)
                break;

            if ((wParam & MK_CONTROL) && (context->StyleFlags & TNS_REORDER))
            {
                PhTabNewBeginDrag(context, hit, pt);
            }
            else
            {
                PhTabNewSetSelection(context, hit, TRUE);
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            POINT pt;
            LONG hit;

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            if (context->DragArmed || context->DragActive)
            {
                BOOLEAN wasActive = !!context->DragActive;
                LONG startIdx = context->DragSourceIndex;

                PhTabNewEndDrag(context, FALSE);

                if (!wasActive && startIdx >= 0)
                {
                    // Treat as a normal click
                    PhTabNewSetSelection(context, startIdx, TRUE);
                }
                break;
            }

            if (GetCapture() == WindowHandle)
            {
                ReleaseCapture();
            }

            hit = PhTabNewHitTest(context, pt, NULL);

            if (hit >= 0 && (LONG)hit == context->HotIndex)
            {
                InvalidateRect(WindowHandle, NULL, FALSE);
            }
        }
        break;
    case WM_RBUTTONUP:
        {
            POINT pt;
            LONG hit;

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            hit = PhTabNewHitTest(context, pt, NULL);

            PhTabNewSendNotify(context, PHTNN_RCLICK, hit);
        }
        break;
    case WM_GETDLGCODE:
        {
            PMSG message = (PMSG)lParam;

            if (message && message->message == WM_KEYDOWN)
            {
                switch (message->wParam)
                {
                case VK_TAB:
                    if (GetKeyState(VK_CONTROL) < 0)
                        return DLGC_WANTMESSAGE;
                    break;
                case VK_HOME:
                case VK_END:
                    return DLGC_WANTARROWS | DLGC_WANTMESSAGE;
                }
            }

            return DLGC_WANTARROWS;
        }
        break;
    case WM_KEYDOWN:
        {
            BOOLEAN handled = FALSE;
            LONG count;
            LONG newIndex;

            if (wParam == VK_ESCAPE && (context->DragArmed || context->DragActive))
            {
                PhTabNewEndDrag(context, TRUE);
                return 0;
            }

            count = (LONG)context->Items->Count;

            if (count <= 0)
                break;

            newIndex = context->SelectedIndex;

            switch (wParam)
            {
            case VK_TAB:
                if (GetKeyState(VK_CONTROL) < 0)
                {
                    handled = TRUE;

                    if (GetKeyState(VK_SHIFT) < 0)
                    {
                        if (newIndex > 0)
                            newIndex--;
                        else
                            newIndex = count - 1;
                    }
                    else
                    {
                        if (newIndex >= 0 && newIndex < count - 1)
                            newIndex++;
                        else
                            newIndex = 0;
                    }
                }
                break;
            case VK_LEFT:
            case VK_UP:
                handled = TRUE;

                if (newIndex > 0)
                    newIndex--;
                else
                    newIndex = count - 1;
                break;
            case VK_RIGHT:
            case VK_DOWN:
                handled = TRUE;

                if (newIndex >= 0 && newIndex < count - 1)
                    newIndex++;
                else
                    newIndex = 0;
                break;
            case VK_HOME:
                handled = TRUE;
                newIndex = 0;
                break;
            case VK_END:
                handled = TRUE;
                newIndex = count - 1;
                break;
            default:
                break;
            }

            if (handled)
            {
                if (newIndex != context->SelectedIndex)
                {
                    PhTabNewSetSelection(context, newIndex, TRUE);

                    if (IsWindow(WindowHandle))
                        SetFocus(WindowHandle);
                }

                return 0;
            }
        }
        break;
    case WM_CAPTURECHANGED:
        {
            if (context->DragArmed || context->DragActive)
            {
                PhTabNewEndDrag(context, TRUE);
            }
        }
        break;
    case WM_GETFONT:
        return (LRESULT)context->Font;
    case WM_SETFONT:
        {
            if (context->Font && context->OwnFont)
                DeleteFont(context->Font);

            context->Font = (HFONT)wParam;
            context->OwnFont = FALSE;

            if (!context->Font)
                PhTabNewUpdateFont(context);

            PhTabNewUpdateMetrics(context);

            context->LayoutDirty = TRUE;

            PhTabNewFlushLayout(context, TRUE, !!lParam);
        }
        return 0;
    }

    if (WindowMessage >= PHTNM_FIRST && WindowMessage <= PHTNM_LAST)
    {
        return PhTabNewOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);
    }

    switch (WindowMessage)
    {
    case TCM_GETCURSEL:
    case TCM_SETCURSEL:
    case TCM_GETITEMCOUNT:
    case TCM_DELETEITEM:
    case TCM_DELETEALLITEMS:
    case TCM_INSERTITEMW:
    case TCM_ADJUSTRECT:
    case TCM_GETITEMRECT:
    case TCM_GETROWCOUNT:
        {
            return PhTabNewOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);
        }
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}
