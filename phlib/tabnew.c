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
    wcex.hbrBackground = NULL;
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
    context->Items = PhCreateList(8);
    context->Pages = PhCreateList(8);
    context->SelectedIndex = LONG_ERROR;
    context->HotIndex = -1;
    context->DragSourceIndex = -1;
    context->DragTargetIndex = -1;
    context->DragOriginIndex = -1;
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

/**
 * Destroys a tab control context.
 *
 * \param Context A pointer to the tab control context.
 */
VOID PhDestroyTabNewContext(
    _In_ _Post_invalid_ PPH_TABNEW_CONTEXT Context
    )
{
    ULONG i;

    if (Context->Items)
    {
        for (i = 0; i < Context->Items->Count; i++)
            PhFreeTabNewItem(Context->Items->Items[i]);

        PhDereferenceObject(Context->Items);
    }

    if (Context->Pages)
    {
        for (i = 0; i < Context->Pages->Count; i++)
        {
            PPH_TABNEW_PAGE page = Context->Pages->Items[i];

            if (page->Callback)
                page->Callback(page, PhTabNewPageDestroy, NULL, NULL);

            PhFree(page);
        }

        PhDereferenceObject(Context->Pages);
    }

    if (Context->Font)
        DeleteFont(Context->Font);

    if (Context->ThemeHandle)
        PhCloseThemeData(Context->ThemeHandle);

    PhTabNewDeleteCachedResources(Context);

    PhFree(Context);
}

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
        length = (length * 10) + (ULONG)(cursor[index] - L'0');
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

    newFont = PhCreateCommonFont(
        -11,
        FW_NORMAL,
        Context->WindowHandle,
        Context->WindowDpi
        );

    if (newFont)
    {
        if (Context->Font)
            DeleteFont(Context->Font);
        Context->Font = newFont;
    }
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
    if (Context->ThemeHandle)
    {
        PhCloseThemeData(Context->ThemeHandle);
        Context->ThemeHandle = NULL;
    }

    if (Context->Skin == PhTabNewSkinUxTheme)
    {
        PCWSTR themeClass;

        switch (Context->Side)
        {
        case TNS_BOTTOM:
        case TNS_LEFT:
        case TNS_RIGHT:
        default:
            themeClass = L"Tab";
            break;
        }

        Context->ThemeHandle = PhOpenThemeData(Context->WindowHandle, themeClass, Context->WindowDpi);
    }

    Context->ThemeDark = !!PhEnableThemeSupport;
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
    TEXTMETRIC tm;
    LONG height;

    GetTextMetrics(Hdc, &tm);

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
    SIZE textSize = { 0, 0 };
    LONG width;

    if (Item->Text)
    {
        GetTextExtentPoint32(Hdc, Item->Text->Buffer, (LONG)Item->Text->Length / sizeof(WCHAR), &textSize);
    }

    width = textSize.cx + Context->PaddingX * 2;

    if (Item->ImageIndex >= 0 && Context->ImageList)
    {
        LONG iconCx, iconCy;
        ImageList_GetIconSize(Context->ImageList, &iconCx, &iconCy);
        width += iconCx + Context->PaddingX;
    }

    if (width < Context->MinTabWidth)
        width = Context->MinTabWidth;

    return width;
}

/**
 * Recomputes the layout of all tabs.
 *
 * \param Context A pointer to the tab control context.
 */
VOID PhTabNewLayout(
    _In_ PPH_TABNEW_CONTEXT Context
    )
{
    RECT clientRect;
    HDC hdc;
    HFONT oldFont;
    LONG rowHeight;
    LONG rowCountsStack[32];
    LONG rowWidthsStack[32];
    PLONG rowCounts = NULL;
    PLONG rowWidths = NULL;
    ULONG i;
    BOOLEAN vertical;

    if (!PhGetClientRect(Context->WindowHandle, &clientRect))
        return;

    vertical = (Context->Side == TNS_LEFT || Context->Side == TNS_RIGHT);

    hdc = GetDC(Context->WindowHandle);
    oldFont = SelectFont(hdc, Context->Font ? Context->Font : GetStockFont(DEFAULT_GUI_FONT));

    rowHeight = PhMeasureTabHeight(Context, hdc);
    Context->RowHeight = rowHeight;

    if (!vertical && Context->Items->Count > 0)
    {
        if (Context->Items->Count <= RTL_NUMBER_OF(rowCountsStack))
        {
            rowCounts = rowCountsStack;
            rowWidths = rowWidthsStack;
            memset(rowCounts, 0, sizeof(rowCountsStack));
            memset(rowWidths, 0, sizeof(rowWidthsStack));
        }
        else
        {
            rowCounts = PhAllocateZero(sizeof(LONG) * Context->Items->Count);
            rowWidths = PhAllocateZero(sizeof(LONG) * Context->Items->Count);
        }
    }

    // Measure widths
    {
        LONG cursor = 0;
        LONG row = 0;
        LONG stripExtent;

        stripExtent = vertical ? (LONG)(clientRect.bottom - clientRect.top)
                               : (LONG)(clientRect.right - clientRect.left);

        for (i = 0; i < Context->Items->Count; i++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
            LONG itemExtent;

            if (vertical)
            {
                // For left/right placement: tabs flow vertically with fixed strip width.
                // No multi-column wrap; treat overflow by clipping.
                item->Row = 0;
                itemExtent = rowHeight; // re-use rowHeight as item height
                item->Rect.left = 0;
                item->Rect.top = (LONG)cursor;
                item->Rect.right = 0; // filled below after strip width known
                item->Rect.bottom = (LONG)(cursor + itemExtent);
                cursor += itemExtent;
            }
            else
            {
                LONG itemWidth = PhMeasureTabWidth(Context, hdc, item);
                itemExtent = itemWidth;

                // Multi-line wrap is the default for top/bottom placements
                // (matches classic tab control behavior). Opt out by clearing
                // TNS_MULTILINE — but the bit is also implicit when at the
                // top/bottom so wrapping happens automatically.
                if (cursor > 0 && cursor + itemExtent > stripExtent)
                {
                    row++;
                    cursor = 0;
                }

                item->Row = row;
                item->Rect.left = (LONG)cursor;
                item->Rect.right = (LONG)(cursor + itemExtent);
                item->Rect.top = (LONG)(row * rowHeight);
                item->Rect.bottom = (LONG)((row + 1) * rowHeight);
                cursor += itemExtent;

                if (rowCounts && rowWidths)
                {
                    rowCounts[row]++;
                    rowWidths[row] += itemExtent;
                }
            }
        }

        Context->RowCount = vertical ? 1 : (Context->Items->Count > 0 ? (((PPH_TABNEW_INTERNAL_ITEM)Context->Items->Items[Context->Items->Count - 1])->Row + 1) : 1);
    }

    // Move the selected row next to the page and keep the remaining rows in
    // logical order when read from the page edge outward.
    if (!vertical && Context->RowCount > 1)
    {
        LONG selectedRow = -1;

        if (Context->SelectedIndex >= 0 && Context->SelectedIndex < (LONG)Context->Items->Count)
        {
            selectedRow = ((PPH_TABNEW_INTERNAL_ITEM)Context->Items->Items[Context->SelectedIndex])->Row;
        }

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

            item->Rect.top = (LONG)(visualRow * rowHeight);
            item->Rect.bottom = (LONG)((visualRow + 1) * rowHeight);
        }
    }

    // When there's overflow (more than one row), stretch every row's tabs
    // proportionally to fill the strip width — matches Windows TCS_MULTILINE
    // justified layout. Single-row strips keep their natural widths and
    // honor the alignment style below.
    if (!vertical && Context->RowCount > 1 && Context->Items->Count > 0)
    {
        LONG stripExtent = (LONG)(clientRect.right - clientRect.left);
        LONG r;

        for (r = 0; r < Context->RowCount; r++)
        {
            LONG rowCount;
            LONG rowTotalWidth;
            LONG slack;
            LONG extraEach;
            LONG extraRemainder;
            LONG cursor;
            LONG seen = 0;

            rowCount = rowCounts[r];
            rowTotalWidth = rowWidths[r];

            if (rowCount == 0) continue;
            slack = (LONG)stripExtent - rowTotalWidth;
            if (slack <= 0) continue;

            extraEach = slack / (LONG)rowCount;
            extraRemainder = slack % (LONG)rowCount;
            cursor = 0;

            for (i = 0; i < Context->Items->Count; i++)
            {
                PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
                LONG width;

                if (item->Row != r) 
                    continue;

                width = (item->Rect.right - item->Rect.left) + extraEach;
                if (seen < (LONG)extraRemainder) width += 1; // distribute leftover px to first tabs
                item->Rect.left = cursor;
                item->Rect.right = cursor + width;
                cursor += width;
                seen++;
            }
        }
    }

    // Optional right-align for row 0 (TNS_RIGHTALIGN style).
    if (!vertical && (Context->StyleFlags & TNS_RIGHTALIGN) && Context->Items->Count > 0)
    {
        LONG stripExtent = (LONG)(clientRect.right - clientRect.left);
        LONG rowMaxRight = MINLONG;
        LONG shift;

        for (i = 0; i < Context->Items->Count; i++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];

            if (item->Row != 0) 
                continue;

            if (item->Rect.right > rowMaxRight)
            {
                 rowMaxRight = item->Rect.right;
            }
        }

        shift = (LONG)stripExtent - rowMaxRight;
        if (shift > 0)
        {
            for (i = 0; i < Context->Items->Count; i++)
            {
                PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];

                if (item->Row != 0) 
                    continue;

                item->Rect.left += shift;
                item->Rect.right += shift;
            }
        }
    }

    // Strip thickness
    if (vertical)
    {
        // Strip width = max measured tab width
        LONG maxWidth = Context->MinTabWidth;

        for (i = 0; i < Context->Items->Count; i++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];

            LONG w = PhMeasureTabWidth(Context, hdc, item);

            if (w > maxWidth)
                maxWidth = w;
        }

        Context->StripThickness = maxWidth;

        for (i = 0; i < Context->Items->Count; i++)
        {
            PPH_TABNEW_INTERNAL_ITEM item = Context->Items->Items[i];
            item->Rect.right = (LONG)Context->StripThickness;
        }
    }
    else
    {
        Context->StripThickness = Context->RowCount * rowHeight;
    }

    SelectFont(hdc, oldFont);
    ReleaseDC(Context->WindowHandle, hdc);

    if (rowCounts && rowCounts != rowCountsStack)
        PhFree(rowCounts);
    if (rowWidths && rowWidths != rowWidthsStack)
        PhFree(rowWidths);

    // Compute page rect in client coords
    {
        RECT pageRect = clientRect;

        switch (Context->Side)
        {
        case TNS_TOP:
            pageRect.top += (LONG)Context->StripThickness;
            break;
        case TNS_BOTTOM:
            pageRect.bottom -= (LONG)Context->StripThickness;
            break;
        case TNS_LEFT:
            pageRect.left += (LONG)Context->StripThickness;
            break;
        case TNS_RIGHT:
            pageRect.right -= (LONG)Context->StripThickness;
            break;
        }

        Context->CachedPageRect = pageRect;
    }

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
 * \return The index of the tab at the given point, or -1 if no tab is present.
 */
LONG PhTabNewHitTest(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ POINT Point,
    _Out_opt_ PULONG Flags
    )
{
    ULONG i;
    ULONG flags = TNHT_NOWHERE;
    LONG result = -1;

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
 * \param ItemIndex The index of the item, or -1.
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
        PhTabNewLayout(Context);

    nm.PageRect = Context->CachedPageRect;

    // Translate to parent client coords
    if (Context->ParentHandle)
    {
        MapWindowPoints(Context->WindowHandle, Context->ParentHandle, (POINT *)&nm.PageRect, 2);
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

    if (NewIndex < -1 || NewIndex >= (LONG)Context->Items->Count)
        return;

    if (NewIndex == oldIndex)
        return;

    if (Notify)
    {
        LRESULT result = PhTabNewSendNotify(Context, PHTNN_SELCHANGING, NewIndex);

        if (result)
            return; // Cancel

        // Stock TCN_* alias for legacy consumers
        PhTabNewSendNotify(Context, TCN_SELCHANGING, NewIndex);
    }

    Context->SelectedIndex = NewIndex;
    // Re-run layout so the multi-row reorder picks up the new selection
    // (the row containing the selected tab moves adjacent to the page).
    Context->LayoutDirty = TRUE;
    PhTabNewLayout(Context);
    InvalidateRect(Context->WindowHandle, NULL, FALSE);

    if (Notify)
    {
        PhTabNewSendNotify(Context, PHTNN_SELCHANGED, NewIndex);
        PhTabNewSendNotify(Context, TCN_SELCHANGE, NewIndex);
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

    if (!Context->LayoutSuspended)
    {
        PhTabNewLayout(Context);
        PhTabNewSendLayoutNotify(Context);
        InvalidateRect(Context->WindowHandle, NULL, TRUE);
    }

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
    ULONG pageIndex;

    if (Index < 0 || (ULONG)Index >= Context->Items->Count)
        return FALSE;

    item = Context->Items->Items[Index];
    PhRemoveItemList(Context->Items, Index);

    if (Context->Pages && Context->Pages->Count == Context->Items->Count + 1)
    {
        pageIndex = PhFindItemList(Context->Pages, (PVOID)item->Param);

        if (pageIndex != ULONG_MAX)
        {
            PhRemoveItemList(Context->Pages, pageIndex);

            for (pageIndex = 0; pageIndex < Context->Pages->Count; pageIndex++)
            {
                ((PPH_TABNEW_PAGE)Context->Pages->Items[pageIndex])->Index = pageIndex;
            }
        }
    }

    PhFreeTabNewItem(item);

    if (Context->SelectedIndex == Index)
    {
        LONG newSel = Index;

        if (newSel >= (LONG)Context->Items->Count)
            newSel = (LONG)Context->Items->Count - 1;

        Context->SelectedIndex = -1;
        PhTabNewSetSelection(Context, newSel, TRUE);
    }
    else if (Context->SelectedIndex > Index)
    {
        Context->SelectedIndex--;
    }

    if (Context->HotIndex >= (LONG)Context->Items->Count)
        Context->HotIndex = -1;

    Context->LayoutDirty = TRUE;
    PhTabNewLayout(Context);
    PhTabNewSendLayoutNotify(Context);
    InvalidateRect(Context->WindowHandle, NULL, TRUE);

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
    ULONG pageIndex;
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

    if (Context->Pages && Context->Pages->Count == Context->Items->Count)
    {
        pageIndex = PhFindItemList(Context->Pages, (PVOID)item->Param);

        if (pageIndex != ULONG_MAX)
        {
            PhRemoveItemList(Context->Pages, pageIndex);
            PhInsertItemList(Context->Pages, ToIndex, (PVOID)item->Param);

            for (pageIndex = 0; pageIndex < Context->Pages->Count; pageIndex++)
            {
                ((PPH_TABNEW_PAGE)Context->Pages->Items[pageIndex])->Index = pageIndex;
            }
        }
    }

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
    {
        PhTabNewLayout(Context);
        PhTabNewSendLayoutNotify(Context);
        InvalidateRect(Context->WindowHandle, NULL, TRUE);
    }

    return TRUE;
}

// ---------------------------------------------------------------------------
// Drag and drop reorder (Ctrl+drag)
// ---------------------------------------------------------------------------

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
    Context->DragTargetIndex = ItemIndex;
    Context->DragOriginIndex = ItemIndex;
    Context->DragStartPoint = Point;
    Context->DragArmed = TRUE;
    Context->DragActive = FALSE;

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
    LONG target;

    if (!Context->DragArmed)
        return;

    if (!Context->DragActive)
    {
        LONG dx = abs(Point.x - Context->DragStartPoint.x);
        LONG dy = abs(Point.y - Context->DragStartPoint.y);

        if (dx < GetSystemMetrics(SM_CXDRAG) && dy < GetSystemMetrics(SM_CYDRAG))
            return;

        Context->DragActive = TRUE;

        PhSetCursor(PhLoadCursor(NULL, IDC_SIZEALL));
    }

    target = PhTabNewHitTest(Context, Point, NULL);
    if (target < 0 || target == Context->DragSourceIndex)
        return;

    // Move source toward target
    PhTabNewMoveItem(Context, Context->DragSourceIndex, target, FALSE);
    Context->DragSourceIndex = target;
    Context->DragTargetIndex = target;
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
    LONG finalIndex = Context->DragSourceIndex;
    LONG originalIndex = Context->DragOriginIndex;
    BOOLEAN wasActive = !!Context->DragActive;

    if (GetCapture() == Context->WindowHandle)
    {
        ReleaseCapture();
    }

    if (Cancel && originalIndex >= 0 && finalIndex >= 0 && finalIndex != originalIndex)
    {
        PhTabNewMoveItem(Context, finalIndex, originalIndex, FALSE);
        finalIndex = originalIndex;
    }

    Context->DragArmed = FALSE;
    Context->DragActive = FALSE;
    PhSetRectEmpty(&Context->DragInsertMarker);

    if (wasActive && !Cancel && finalIndex >= 0 && originalIndex >= 0 && originalIndex != finalIndex)
    {
        NMTABNEWREORDER nm;

        nm.FromIndex = originalIndex;
        nm.ToIndex = finalIndex;

        PhTabNewDispatchNotify(Context, PHTNN_REORDERED, &nm.Header);
    }

    if (wasActive)
    {
        PhTabNewLayout(Context);
        PhTabNewSendLayoutNotify(Context);
    }

    InvalidateRect(Context->WindowHandle, NULL, FALSE);
    Context->DragSourceIndex = -1;
    Context->DragTargetIndex = -1;
    Context->DragOriginIndex = -1;
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
        return PhThemeWindowBackgroundColor;
    return GetSysColor(COLOR_BTNFACE);
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
    COLORREF accentColor;
    COLORREF hotColor;
    COLORREF outlineColor;

    PhTabNewDeleteCachedResources(Context);

    backgroundColor = PhTabNewBackgroundColor(Context);
    accentColor = Context->ThemeDark ? PhThemeWindowHighlightColor : GetSysColor(COLOR_HIGHLIGHT);
    hotColor = Context->ThemeDark ? PhThemeWindowBackground2Color : RGB(0xE5, 0xF1, 0xFB);
    outlineColor = Context->ThemeDark ? RGB(0x55, 0x55, 0x55) : RGB(0xAC, 0xAC, 0xAC);

    Context->BackgroundBrush = CreateSolidBrush(backgroundColor);
    Context->AccentBrush = CreateSolidBrush(accentColor);
    Context->HotBrush = CreateSolidBrush(hotColor);
    Context->OutlinePen = CreatePen(PS_SOLID, 1, outlineColor);
    Context->BackgroundPen = CreatePen(PS_SOLID, 1, backgroundColor);
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
        ImageList_GetIconSize(Context->ImageList, &iconCx, &iconCy);
        ImageList_Draw(
            Context->ImageList,
            Item->ImageIndex,
            Hdc,
            contentRect.left,
            contentRect.top + ((contentRect.bottom - contentRect.top) - iconCy) / 2,
            ILD_NORMAL
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
            DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS
            );
    }
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
            FillRect(Hdc, &itemRect, Context->BackgroundBrush);

            // Accent indicator
            {
                RECT accentRect = itemRect;
                LONG t = 2;

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
    COLORREF inacTop = Context->ThemeDark ? RGB(0x33, 0x33, 0x33) : RGB(0xF5, 0xF5, 0xF5);
    COLORREF inacBot = Context->ThemeDark ? RGB(0x28, 0x28, 0x28) : RGB(0xE2, 0xE2, 0xE2);
    COLORREF hotTop  = Context->ThemeDark ? RGB(0x3D, 0x4A, 0x5C) : RGB(0xEA, 0xF6, 0xFD);
    COLORREF hotBot  = Context->ThemeDark ? RGB(0x2A, 0x36, 0x48) : RGB(0xD9, 0xEE, 0xF7);
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
        TRIVERTEX vert[2];
        GRADIENT_RECT gRect = { 0, 1 };
        BOOLEAN isHot;
        COLORREF top, bot;
        RECT fillRect;

        if ((LONG)i == selected)
            continue;

        item = Context->Items->Items[i];
        isHot = ((LONG)i == Context->HotIndex);

        PhTabNewGetItemRectInClient(Context, item, &itemRect);

        // Inactive tabs sit 2 px shorter than the strip so the selected tab
        // visually rises above them (classic Aero "raised selected" effect).
        // The far edge is also inset 1 px so the divider shows through.
        fillRect = itemRect;

        switch (Context->Side)
        {
        case TNS_TOP:    fillRect.top += 2; fillRect.bottom -= 1; break;
        case TNS_BOTTOM: fillRect.bottom -= 2; fillRect.top += 1; break;
        case TNS_LEFT:   fillRect.left += 2; fillRect.right -= 1; break;
        case TNS_RIGHT:  fillRect.right -= 2; fillRect.left += 1; break;
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
        vert[0].Red   = (USHORT)(GetRValue(top) << 8);
        vert[0].Green = (USHORT)(GetGValue(top) << 8);
        vert[0].Blue  = (USHORT)(GetBValue(top) << 8);
        vert[0].Alpha = 0;
        vert[1].x = fillRect.right; vert[1].y = fillRect.bottom;
        vert[1].Red   = (USHORT)(GetRValue(bot) << 8);
        vert[1].Green = (USHORT)(GetGValue(bot) << 8);
        vert[1].Blue  = (USHORT)(GetBValue(bot) << 8);
        vert[1].Alpha = 0;

        //GradientFill(
        //    Hdc,
        //    vert,
        //    2,
        //    &gRect,
        //    1,
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

        FillRect(Hdc, &fillRect, Context->BackgroundBrush);

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
            SelectPen(Hdc, Context->BackgroundPen);
            MoveToEx(Hdc, fillRect.left + 1, fillRect.bottom - 1, NULL);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom - 1);
            break;
        case TNS_BOTTOM:
            MoveToEx(Hdc, fillRect.left, fillRect.top - 1, NULL);
            LineTo(Hdc, fillRect.left, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.right - 1, fillRect.top - 1);
            SelectPen(Hdc, Context->BackgroundPen);
            MoveToEx(Hdc, fillRect.left + 1, fillRect.top, NULL);
            LineTo(Hdc, fillRect.right - 1, fillRect.top);
            break;
        case TNS_LEFT:
            MoveToEx(Hdc, fillRect.right, fillRect.top, NULL);
            LineTo(Hdc, fillRect.left, fillRect.top);
            LineTo(Hdc, fillRect.left, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.right, fillRect.bottom - 1);
            SelectPen(Hdc, Context->BackgroundPen);
            MoveToEx(Hdc, fillRect.right - 1, fillRect.top + 1, NULL);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom - 1);
            break;
        case TNS_RIGHT:
            MoveToEx(Hdc, fillRect.left - 1, fillRect.top, NULL);
            LineTo(Hdc, fillRect.right - 1, fillRect.top);
            LineTo(Hdc, fillRect.right - 1, fillRect.bottom - 1);
            LineTo(Hdc, fillRect.left - 1, fillRect.bottom - 1);
            SelectPen(Hdc, Context->BackgroundPen);
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
        }
        else if (isHot)
        {
            stateId = TIS_HOT;
        }
        else
        {
            stateId = TIS_NORMAL;
        }

        if (selected)
        {
            //DTBGOPTS options;
            //FillRect(Hdc, &itemRect, Context->BackgroundBrush);
            //memset(&options, 0, sizeof(DTBGOPTS));
            //options.dwSize = sizeof(DTBGOPTS);
            //options.dwFlags = DTBG_OMITCONTENT;
            //PhDrawThemeBackgroundEx(Context->ThemeHandle, Hdc, TABP_TABITEM, stateId, &itemRect, &options);

            FillRect(Hdc, &itemRect, PhThemeWindowBackgroundBrush);
        }
        else
        {
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
        PhTabNewLayout(Context);
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

    if (Context->StyleFlags & TNS_DRAW_PANEL)
    {
        NMTABNEWDRAWPANEL drawPanel;

        memset(&drawPanel, 0, sizeof(NMTABNEWDRAWPANEL));
        drawPanel.hdc = Hdc;
        drawPanel.Rect = *ClientRect;

        PhTabNewDispatchNotify(Context, PHTNN_DRAWPANEL, &drawPanel.Header);
    }

    SelectFont(Hdc, oldFont);
}

typedef struct _PH_TABNEW_LAYOUT_MESSAGE
{
    PPH_STRINGREF Layout;
    PPH_TABNEW_LAYOUT_CALLBACK Callback;
    PVOID Context;
} PH_TABNEW_LAYOUT_MESSAGE, *PPH_TABNEW_LAYOUT_MESSAGE;

typedef struct _PH_TABNEW_ADD_PAGE_MESSAGE
{
    PPH_STRINGREF Name;
    ULONG Flags;
    PPH_TABNEW_PAGE_CALLBACK Callback;
    PVOID Context;
} PH_TABNEW_ADD_PAGE_MESSAGE, *PPH_TABNEW_ADD_PAGE_MESSAGE;

typedef struct _PH_TABNEW_NOTIFY_PAGES_MESSAGE
{
    PH_TABNEW_PAGE_MESSAGE Message;
    PVOID Parameter1;
    PVOID Parameter2;
} PH_TABNEW_NOTIFY_PAGES_MESSAGE, *PPH_TABNEW_NOTIFY_PAGES_MESSAGE;

static PPH_STRING PhpTabNewSaveLayout(
    _In_ PPH_TABNEW_CONTEXT TabContext,
    _In_ PPH_TABNEW_LAYOUT_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i;

    PhInitializeStringBuilder(&stringBuilder, 0x80);

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

PPH_STRING PhTabNewSaveLayout(
    _In_ HWND TabControl,
    _In_ PPH_TABNEW_LAYOUT_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PH_TABNEW_LAYOUT_MESSAGE message;

    message.Layout = NULL;
    message.Callback = Callback;
    message.Context = Context;

    return (PPH_STRING)PhTabNewSendMessage(TabControl, PHTNM_SAVELAYOUT, 0, (LPARAM)&message);
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
        PhTabNewLayout(TabContext);
        PhTabNewSendLayoutNotify(TabContext);
        InvalidateRect(TabContext->WindowHandle, NULL, TRUE);
    }

    return changed;
}

BOOLEAN PhTabNewRestoreLayout(
    _In_ HWND TabControl,
    _In_ PPH_STRINGREF Layout,
    _In_ PPH_TABNEW_LAYOUT_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PH_TABNEW_LAYOUT_MESSAGE message;

    message.Layout = Layout;
    message.Callback = Callback;
    message.Context = Context;

    return !!PhTabNewSendMessage(TabControl, PHTNM_RESTORELAYOUT, 0, (LPARAM)&message);
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
    WCHAR nameBuffer[256];
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
    item.ImageIndex = -1;
    item.Param = (LPARAM)page;
    index = PhTabNewInsertItem(TabContext, -1, &item);
    page->Index = index;

    PhAddItemList(TabContext->Pages, page);

    if (Callback)
        Callback(page, PhTabNewPageCreate, NULL, NULL);

    return page;
}

PPH_TABNEW_PAGE PhTabNewAddPage(
    _In_ HWND TabControl,
    _In_ PPH_STRINGREF Name,
    _In_ ULONG Flags,
    _In_ PPH_TABNEW_PAGE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PH_TABNEW_ADD_PAGE_MESSAGE message;

    message.Name = Name;
    message.Flags = Flags;
    message.Callback = Callback;
    message.Context = Context;

    return (PPH_TABNEW_PAGE)PhTabNewSendMessage(TabControl, PHTNM_ADDPAGE, 0, (LPARAM)&message);
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
    ULONG i;

    for (i = 0; i < TabContext->Pages->Count; i++)
    {
        PPH_TABNEW_PAGE page = TabContext->Pages->Items[i];

        if (PhEqualStringRef(&page->Name, Name, TRUE))
            return page;
    }

    return NULL;
}

PPH_TABNEW_PAGE PhTabNewFindPage(
    _In_ HWND TabControl,
    _In_ PPH_STRINGREF Name
    )
{
    return (PPH_TABNEW_PAGE)PhTabNewSendMessage(TabControl, PHTNM_FINDPAGE, 0, (LPARAM)Name);
}

/**
 * Retrieves a page in the tab control by index.
 *
 * \param TabControl A handle to the tab control window.
 * \param Index The index of the page to retrieve.
 * \return A pointer to the page structure if found, otherwise NULL.
 */
static PPH_TABNEW_PAGE PhpTabNewGetPageByIndex(
    _In_ PPH_TABNEW_CONTEXT TabContext,
    _In_ LONG Index
    )
{
    if (Index < 0 || (ULONG)Index >= TabContext->Pages->Count)
        return NULL;

    return TabContext->Pages->Items[Index];
}

PPH_TABNEW_PAGE PhTabNewGetPageByIndex(
    _In_ HWND TabControl,
    _In_ LONG Index
    )
{
    return (PPH_TABNEW_PAGE)PhTabNewSendMessage(TabControl, PHTNM_GETPAGEBYINDEX, (WPARAM)Index, 0);
}

/**
 * Selects a specific page in the tab control.
 *
 * \param TabControl A handle to the tab control window.
 * \param Page A pointer to the page to select.
 */
VOID PhTabNewSelectPage(
    _In_ HWND TabControl,
    _In_ PPH_TABNEW_PAGE Page
    )
{
    PhTabNew_SetCurSel(TabControl, Page->Index);
}

/**
 * Sends a message to all pages in the tab control.
 *
 * \param TabControl A handle to the tab control window.
 * \param Message The message to send.
 * \param Parameter1 The first message parameter.
 * \param Parameter2 The second message parameter.
 */
static VOID PhpTabNewNotifyAllPages(
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
            page->Callback(page, Message, Parameter1, Parameter2);
    }
}

VOID PhTabNewNotifyAllPages(
    _In_ HWND TabControl,
    _In_ PH_TABNEW_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    PH_TABNEW_NOTIFY_PAGES_MESSAGE message;

    message.Message = Message;
    message.Parameter1 = Parameter1;
    message.Parameter2 = Parameter2;

    PhTabNewSendMessage(TabControl, PHTNM_NOTIFYPAGES, 0, (LPARAM)&message);
}

/**
 * Retrieves the currently selected page in the tab control.
 *
 * \param TabControl A handle to the tab control window.
 * \return A pointer to the currently selected page, or NULL if none is selected.
 */
static PPH_TABNEW_PAGE PhpTabNewGetCurrentPage(
    _In_ PPH_TABNEW_CONTEXT TabContext
    )
{
    LONG sel;

    sel = TabContext->SelectedIndex;
    if (sel < 0 || (ULONG)sel >= TabContext->Pages->Count)
        return NULL;

    return TabContext->Pages->Items[sel];
}

PPH_TABNEW_PAGE PhTabNewGetCurrentPage(
    _In_ HWND TabControl
    )
{
    return (PPH_TABNEW_PAGE)PhTabNewSendMessage(TabControl, PHTNM_GETCURRENTPAGE, 0, 0);
}

// ---------------------------------------------------------------------------
// WndProc
// ---------------------------------------------------------------------------

static LRESULT PhpTabNewOnUserMessage(
    _In_ HWND WindowHandle,
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
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
            ULONG i;

            for (i = 0; i < Context->Items->Count; i++)
                PhFreeTabNewItem(Context->Items->Items[i]);

            PhClearList(Context->Items);

            Context->SelectedIndex = -1;
            Context->HotIndex = -1;
            Context->LayoutDirty = TRUE;

            PhTabNewLayout(Context);
            PhTabNewSendLayoutNotify(Context);

            InvalidateRect(WindowHandle, NULL, TRUE);
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

            PhTabNewLayout(Context);
            InvalidateRect(WindowHandle, NULL, TRUE);
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
                PhTabNewLayout(Context);
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
            
            Context->LayoutDirty = TRUE;
            
            PhTabNewLayout(Context);
            PhTabNewSendLayoutNotify(Context);
         
            InvalidateRect(WindowHandle, NULL, TRUE);
        }
        return TRUE;

    case PHTNM_GETSKIN:
        return (LRESULT)Context->Skin;
    case PHTNM_SETSIDE:
        {
            Context->Side = (ULONG)WParam & TNS_SIDE_MASK;

            PhTabNewUpdateTheme(Context);
            PhTabNewUpdateCachedResources(Context);

            Context->LayoutDirty = TRUE;

            PhTabNewLayout(Context);
            PhTabNewSendLayoutNotify(Context);

            InvalidateRect(WindowHandle, NULL, TRUE);
        }
        return TRUE;
    case PHTNM_GETSIDE:
        return (LRESULT)Context->Side;
    case PHTNM_HITTEST:
        {
            PPH_TABNEW_HITTESTINFO info = (PPH_TABNEW_HITTESTINFO)LParam;
            if (!info) return -1;
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

            PhTabNewLayout(Context);
            InvalidateRect(WindowHandle, NULL, TRUE);

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

            PhTabNewLayout(Context);
            InvalidateRect(WindowHandle, NULL, TRUE);
        }
        return TRUE;
    case PHTNM_SETMINTABWIDTH:
        {
            Context->BaseMinTabWidth = (LONG)WParam;

            PhTabNewUpdateMetrics(Context);

            Context->LayoutDirty = TRUE;

            PhTabNewLayout(Context);
            InvalidateRect(WindowHandle, NULL, TRUE);
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
    case PHTNM_RESTORELAYOUT:
        {
            PPH_TABNEW_LAYOUT_MESSAGE message = (PPH_TABNEW_LAYOUT_MESSAGE)LParam;

            if (!message)
                return FALSE;

            return (LRESULT)PhpTabNewRestoreLayout(Context, message->Layout, message->Callback, message->Context);
        }
    case PHTNM_ADDPAGE:
        {
            PPH_TABNEW_ADD_PAGE_MESSAGE message = (PPH_TABNEW_ADD_PAGE_MESSAGE)LParam;

            if (!message || !message->Name)
                return 0;

            return (LRESULT)PhpTabNewAddPage(Context, message->Name, message->Flags, message->Callback, message->Context);
        }
    case PHTNM_FINDPAGE:
        {
            PPH_STRINGREF name = (PPH_STRINGREF)LParam;

            if (!name)
                return 0;

            return (LRESULT)PhpTabNewFindPage(Context, name);
        }
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
            ULONG i;

            for (i = 0; i < Context->Items->Count; i++)
                PhFreeTabNewItem(Context->Items->Items[i]);

            PhClearList(Context->Items);

            Context->SelectedIndex = -1;
            Context->HotIndex = -1;
            Context->LayoutDirty = TRUE;

            PhTabNewLayout(Context);
            PhTabNewSendLayoutNotify(Context);
            InvalidateRect(WindowHandle, NULL, TRUE);
        }
        return TRUE;
    case TCM_INSERTITEMW:
        {
            TCITEMW *tci = (TCITEMW *)LParam;

            PH_TABNEW_INSERTITEM ins;
            ins.Text = (tci && (tci->mask & TCIF_TEXT)) ? tci->pszText : L"";
            ins.ImageIndex = (tci && (tci->mask & TCIF_IMAGE)) ? tci->iImage : -1;
            ins.Param = (tci && (tci->mask & TCIF_PARAM)) ? tci->lParam : 0;
            return (LRESULT)PhTabNewInsertItem(Context, (LONG)WParam, &ins);
        }
    case TCM_ADJUSTRECT:
        {
            PRECT rc = (PRECT)LParam;

            if (!rc)
            {
                return 0;
            }

            if (Context->LayoutDirty)
            {
                PhTabNewLayout(Context);
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
            return PhpTabNewOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);
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
            // TNS_REORDER is on by default
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
            PhTabNewUpdateMetrics(context);
            PhTabNewUpdateFont(context);
            PhTabNewUpdateTheme(context);
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
        // Suppress background erasure; WM_PAINT renders the entire control using a buffer.
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

            PhTabNewLayout(context);
            PhTabNewSendLayoutNotify(context);

            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            context->WindowDpi = PhGetWindowDpi(WindowHandle);

            PhTabNewUpdateMetrics(context);
            PhTabNewUpdateFont(context);
            PhTabNewUpdateTheme(context);
            PhTabNewUpdateCachedResources(context);

            context->LayoutDirty = TRUE;

            PhTabNewLayout(context);
            PhTabNewSendLayoutNotify(context);
            InvalidateRect(WindowHandle, NULL, TRUE);
        }
        break;
    case WM_THEMECHANGED:
    case WM_SETTINGCHANGE:
        {
            PhTabNewUpdateTheme(context);
            PhTabNewUpdateCachedResources(context);

            InvalidateRect(WindowHandle, NULL, TRUE);
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
                    PhTabNewLayout(context);
                    PhTabNewSendLayoutNotify(context);
                    InvalidateRect(WindowHandle, NULL, TRUE);
                }
            }
            else
            {
                context->LayoutSuspended = TRUE;
            }

            return result;
        }
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

            if (context->HotIndex != -1)
            {
                context->HotIndex = -1;
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
    case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE && (context->DragArmed || context->DragActive))
            {
                PhTabNewEndDrag(context, TRUE);
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
    case WM_SETFONT:
        {
            if (context->Font && wParam) { /* allow override */ }
        }
        break;
    }

    if (WindowMessage >= PHTNM_FIRST && WindowMessage <= PHTNM_LAST)
        return PhpTabNewOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);

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
        return PhpTabNewOnUserMessage(WindowHandle, context, WindowMessage, wParam, lParam);
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}
