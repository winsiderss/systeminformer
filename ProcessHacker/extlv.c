/*
 * Process Hacker - 
 *   extended list view
 * 
 * Copyright (C) 2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phgui.h>
#include <windowsx.h>

#define PH_DURATION_MULT 100
#define PH_MAX_COMPARE_FUNCTIONS 16

// We have nowhere else to store state highlighting 
// information except for the state image index 
// value. This is conveniently 4 bits wide.

typedef enum _PH_ITEM_STATE
{
    // The item is normal. Use the ItemColorFunction 
    // to determine the color of the item.
    NormalItemState = 0,
    // The item is new. On the next tick, 
    // change the state to NormalItemState. When an 
    // item is in this state, highlight it in NewColor.
    NewItemState,
    // The item is being removed. On the next tick,
    // delete the item. When an item is in this state, 
    // highlight it in RemovingColor.
    RemovingItemState
} PH_ITEM_STATE;

#define PH_GET_ITEM_STATE(State) (((State) & LVIS_STATEIMAGEMASK) >> 12)

#define PH_STATE_TIMER 3

typedef struct _PH_EXTLV_CONTEXT
{
    HWND Handle;
    WNDPROC OldWndProc;
    PVOID Context;

    // Sorting

    BOOLEAN TriState;
    ULONG SortColumn;
    PH_SORT_ORDER SortOrder;

    PPH_COMPARE_FUNCTION TriStateCompareFunction;
    PPH_COMPARE_FUNCTION CompareFunctions[PH_MAX_COMPARE_FUNCTIONS];
    PPH_LIST FallbackColumns;

    // State Highlighting

    LONG EnableStateHighlighting;
    ULONG HighlightingDuration;
    COLORREF NewColor;
    COLORREF RemovingColor;
    PPH_GET_ITEM_COLOR ItemColorFunction;
    PPH_GET_ITEM_FONT ItemFontFunction;
    PPH_HASHTABLE TickHashtable;

    // Misc.

    LONG EnableRedraw;
    HCURSOR Cursor;
} PH_EXTLV_CONTEXT, *PPH_EXTLV_CONTEXT;

typedef struct _PH_TICK_ENTRY
{
    ULONG Id;
    ULONG TickCount;
} PH_TICK_ENTRY, *PPH_TICK_ENTRY;

LRESULT CALLBACK PhpExtendedListViewWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhpSetSortIcon(
    __in HWND hwnd,
    __in INT Index,
    __in PH_SORT_ORDER Order
    );

INT PhpExtendedListViewCompareFunc(
    __in LPARAM lParam1,
    __in LPARAM lParam2,
    __in LPARAM lParamSort
    );

INT PhpDefaultCompareListViewItems(
    __in PPH_EXTLV_CONTEXT Context,
    __in INT X,
    __in INT Y,
    __in ULONG Column
    );

VOID PhListTick(
    __in PPH_EXTLV_CONTEXT Context
    );

static BOOLEAN NTAPI PhpTickHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    return ((PPH_TICK_ENTRY)Entry1)->Id == ((PPH_TICK_ENTRY)Entry2)->Id;
}

static ULONG NTAPI PhpTickHashtableHashFunction(
    __in PVOID Entry
    )
{
    return ((PPH_TICK_ENTRY)Entry)->Id;
}

VOID PhSetExtendedListView(
    __in HWND hWnd
    )
{
    WNDPROC oldWndProc;
    PPH_EXTLV_CONTEXT context;

    oldWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)PhpExtendedListViewWndProc);

    context = PhAllocate(sizeof(PH_EXTLV_CONTEXT));

    context->Handle = hWnd;
    context->OldWndProc = oldWndProc;
    context->Context = NULL;
    context->TriState = FALSE;
    context->SortColumn = 0;
    context->SortOrder = AscendingSortOrder;
    context->TriStateCompareFunction = NULL;
    memset(context->CompareFunctions, 0, sizeof(context->CompareFunctions));
    context->FallbackColumns = PhCreateList(1);

    context->EnableStateHighlighting = 0;
    context->HighlightingDuration = 1000;
    context->NewColor = RGB(0x00, 0xff, 0x00);
    context->RemovingColor = RGB(0xff, 0x00, 0x00);
    context->ItemColorFunction = NULL;
    context->ItemFontFunction = NULL;
    context->TickHashtable = PhCreateHashtable(
        sizeof(PH_TICK_ENTRY),
        PhpTickHashtableCompareFunction,
        PhpTickHashtableHashFunction,
        20
        );

    context->EnableRedraw = 1;
    context->Cursor = NULL;

    SetProp(hWnd, L"ExtLvContext", (HANDLE)context);

    ExtendedListView_Init(hWnd);
}

LRESULT CALLBACK PhpExtendedListViewWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PPH_EXTLV_CONTEXT context;
    WNDPROC oldWndProc;

    context = (PPH_EXTLV_CONTEXT)GetProp(hwnd, L"ExtLvContext");
    oldWndProc = context->OldWndProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

            PhDereferenceObject(context->FallbackColumns);
            PhDereferenceObject(context->TickHashtable);

            PhFree(context);
            RemoveProp(hwnd, L"ExtLvContext");
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case HDN_ITEMCLICK:
                {
                    if (header->hwndFrom == ListView_GetHeader(hwnd))
                    {
                        LPNMHEADER header2 = (LPNMHEADER)header;

                        if (header2->iItem == context->SortColumn)
                        {
                            if (context->TriState)
                            {
                                if (context->SortOrder == AscendingSortOrder)
                                    context->SortOrder = DescendingSortOrder;
                                else if (context->SortOrder == DescendingSortOrder)
                                    context->SortOrder = NoSortOrder;
                                else
                                    context->SortOrder = AscendingSortOrder;
                            }
                            else
                            {
                                if (context->SortOrder == AscendingSortOrder)
                                    context->SortOrder = DescendingSortOrder;
                                else
                                    context->SortOrder = AscendingSortOrder;
                            }
                        }
                        else
                        {
                            context->SortColumn = header2->iItem;
                            context->SortOrder = AscendingSortOrder;
                        }

                        PhpSetSortIcon(ListView_GetHeader(hwnd), context->SortColumn, context->SortOrder);
                        ExtendedListView_SortItems(hwnd);
                    }
                }
                break;
            }
        }
        break;
    case WM_REFLECT + WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_CUSTOMDRAW:
                {
                    if (header->hwndFrom == hwnd)
                    {
                        LPNMLVCUSTOMDRAW customDraw = (LPNMLVCUSTOMDRAW)header;

                        switch (customDraw->nmcd.dwDrawStage)
                        {
                        case CDDS_PREPAINT:
                            return CDRF_NOTIFYITEMDRAW;
                        case CDDS_ITEMPREPAINT:
                            {
                                LVITEM item;
                                PH_ITEM_STATE itemState;
                                BOOLEAN colorChanged = FALSE;
                                HFONT newFont = NULL;

                                item.mask = LVIF_STATE;
                                item.iItem = (INT)customDraw->nmcd.dwItemSpec;
                                item.iSubItem = 0;
                                item.stateMask = LVIS_STATEIMAGEMASK;
                                CallWindowProc(oldWndProc, hwnd, LVM_GETITEM, 0, (LPARAM)&item);
                                itemState = PH_GET_ITEM_STATE(item.state);

                                if (itemState == NormalItemState)
                                {
                                    if (context->ItemColorFunction)
                                    {
                                        customDraw->clrTextBk = context->ItemColorFunction(
                                            (INT)customDraw->nmcd.dwItemSpec,
                                            (PVOID)customDraw->nmcd.lItemlParam,
                                            context->Context
                                            );
                                        colorChanged = TRUE;
                                    }

                                    if (context->ItemFontFunction)
                                    {
                                        newFont = context->ItemFontFunction(
                                            (INT)customDraw->nmcd.dwItemSpec,
                                            (PVOID)customDraw->nmcd.lItemlParam,
                                            context->Context
                                            );
                                    }

                                    if (newFont)
                                        SelectObject(customDraw->nmcd.hdc, newFont);
                                }
                                else if (itemState == NewItemState)
                                {
                                    customDraw->clrTextBk = context->NewColor;
                                    colorChanged = TRUE;
                                }
                                else if (itemState == RemovingItemState)
                                {
                                    customDraw->clrTextBk = context->RemovingColor;
                                    colorChanged = TRUE;
                                }

                                if (colorChanged)
                                {
                                    if (PhGetColorBrightness(customDraw->clrTextBk) > 100) // slightly less than half
                                        customDraw->clrText = RGB(0x00, 0x00, 0x00);
                                    else
                                        customDraw->clrText = RGB(0xff, 0xff, 0xff);
                                }

                                if (!newFont)
                                    return CDRF_DODEFAULT;
                                else
                                    return CDRF_NEWFONT;
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_SETCURSOR:
        {
            if (context->Cursor)
            {
                SetCursor(context->Cursor);
                return TRUE;
            }
        }
        break;
    case LVM_INSERTITEM:
        {
            LPLVITEM item = (LPLVITEM)lParam;
            INT index;

            if (!(item->mask & LVIF_STATE))
            {
                item->mask |= LVIF_STATE;
                item->stateMask = LVIS_STATEIMAGEMASK;
                item->state = 0;
            }
            else
            {
                item->stateMask |= LVIS_STATEIMAGEMASK;
                item->state &= ~LVIS_STATEIMAGEMASK;
            }

            if (context->EnableStateHighlighting > 0)
            {
                item->state |= INDEXTOSTATEIMAGEMASK(NewItemState);
            }
            else
            {
                item->state = NormalItemState;
            }

            if ((index = (INT)CallWindowProc(oldWndProc, hwnd, LVM_INSERTITEM, 0, (LPARAM)item)) == -1)
                return -1;

            if (context->EnableStateHighlighting > 0)
            {
                PH_TICK_ENTRY entry;

                entry.Id = ListView_MapIndexToID(hwnd, index);
                entry.TickCount = GetTickCount();

                PhAddHashtableEntry(context->TickHashtable, &entry);
            }

            return index;
        }
    case LVM_DELETEITEM:
        {
            if (context->EnableStateHighlighting > 0)
            {
                LVITEM item;

                item.mask = LVIF_STATE | LVIF_PARAM;
                item.iItem = (INT)wParam;
                item.iSubItem = 0;
                item.stateMask = LVIS_STATEIMAGEMASK;
                item.state = INDEXTOSTATEIMAGEMASK(RemovingItemState);

                // IMPORTANT:
                // We need to null the param. This is important because the user 
                // will most likely be storing pointers to heap allocations in 
                // here, and free the allocation after it has deleted the 
                // item. The user may allocate sometime in the future and receive 
                // the same pointer as is stored here. The user may call 
                // LVM_FINDITEM or LVM_GETNEXTITEM and find this item, which 
                // is supposed to be deleted. It may then attempt to delete 
                // this item *twice*, which leads to bad things happening, 
                // including *not* deleting the item that the user wanted to delete.
                item.lParam = (LPARAM)NULL;

                ListView_SetItem(hwnd, &item);

                {
                    PH_TICK_ENTRY entry;

                    entry.Id = ListView_MapIndexToID(hwnd, (INT)wParam);
                    entry.TickCount = GetTickCount();

                    if (!PhAddHashtableEntry(context->TickHashtable, &entry))
                    {
                        PPH_TICK_ENTRY existingEntry;

                        existingEntry = PhGetHashtableEntry(context->TickHashtable, &entry);

                        if (existingEntry)
                            existingEntry->TickCount = GetTickCount();
                    }
                }

                return TRUE;
            }
            else
            {
                PH_TICK_ENTRY entry;

                entry.Id = ListView_MapIndexToID(hwnd, (INT)wParam);

                PhRemoveHashtableEntry(context->TickHashtable, &entry);
            }
        }
        break;
    case LVM_GETITEM:
        {
            LVITEM item;
            ULONG itemState;
            ULONG oldMask;
            ULONG oldStateMask;

            memcpy(&item, (LPLVITEM)lParam, sizeof(LVITEM));

            oldMask = item.mask;
            oldStateMask = item.stateMask;

            if (!(item.mask & LVIF_STATE))
            {
                item.mask |= LVIF_STATE;
                item.stateMask = LVIS_STATEIMAGEMASK;
            }
            else
            {
                item.stateMask |= LVIS_STATEIMAGEMASK;
            }

            if (!CallWindowProc(oldWndProc, hwnd, LVM_GETITEM, 0, (LPARAM)&item))
                return FALSE;

            // Check if the item is being deleted. If so, pretend it doesn't 
            // exist.

            itemState = PH_GET_ITEM_STATE(item.state);

            if (itemState == RemovingItemState || itemState == RemovingItemState)
                return FALSE;

            item.mask = oldMask;
            item.stateMask = oldStateMask;
            item.state &= item.stateMask;

            memcpy((LPLVITEM)lParam, &item, sizeof(LVITEM));
        }
        return TRUE;
    case ELVM_ADDFALLBACKCOLUMN:
        {
            PhAddListItem(context->FallbackColumns, (PVOID)wParam); 
        }
        return TRUE;
    case ELVM_ADDFALLBACKCOLUMNS:
        {
            ULONG numberOfColumns = (ULONG)wParam;
            PULONG columns = (PULONG)lParam;
            ULONG i;

            for (i = 0; i < numberOfColumns; i++)
                PhAddListItem(context->FallbackColumns, (PVOID)columns[i]);
        }
        return TRUE;
    case ELVM_INIT:
        {
            PhpSetSortIcon(ListView_GetHeader(hwnd), context->SortColumn, context->SortOrder);
        }
        return TRUE;
    case ELVM_SETCOMPAREFUNCTION:
        {
            ULONG column = (ULONG)wParam;
            PPH_COMPARE_FUNCTION compareFunction = (PPH_COMPARE_FUNCTION)lParam;

            if (column >= PH_MAX_COMPARE_FUNCTIONS)
                return FALSE;

            context->CompareFunctions[column] = compareFunction;
        }
        return TRUE;
    case ELVM_SETCONTEXT:
        {
            context->Context = (PVOID)lParam;
        }
        return TRUE;
    case ELVM_SETCURSOR:
        {
            context->Cursor = (HCURSOR)lParam;
        }
        return TRUE;
    case ELVM_SETITEMCOLORFUNCTION:
        {
            context->ItemColorFunction = (PPH_GET_ITEM_COLOR)lParam;
        }
        return TRUE;
    case ELVM_SETITEMFONTFUNCTION:
        {
            context->ItemFontFunction = (PPH_GET_ITEM_FONT)lParam;
        }
        return TRUE;
    case ELVM_SETNEWCOLOR:
        {
            context->NewColor = (COLORREF)wParam;
        }
        return TRUE;
    case ELVM_SETREDRAW:
        {
            if (wParam)
                context->EnableRedraw++;
            else
                context->EnableRedraw--;

            if (context->EnableRedraw == 1)
            {
                SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            else if (context->EnableRedraw == 0)
            {
                SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
            }
        }
        return TRUE;
    case ELVM_SETREMOVINGCOLOR:
        {
            context->RemovingColor = (COLORREF)wParam;
        }
        return TRUE;
    case ELVM_SETSORT:
        {
            context->SortColumn = (ULONG)wParam;
            context->SortOrder = (PH_SORT_ORDER)lParam;

            PhpSetSortIcon(ListView_GetHeader(hwnd), context->SortColumn, context->SortOrder);
        }
        return TRUE;
    case ELVM_SETSTATEHIGHLIGHTING:
        {
            if (wParam)
                context->EnableStateHighlighting++;
            else
                context->EnableStateHighlighting--;
        }
        return TRUE;
    case ELVM_SETTRISTATE:
        {
            context->TriState = !!wParam;
        }
        return TRUE;
    case ELVM_SETTRISTATECOMPAREFUNCTION:
        {
            context->TriStateCompareFunction = (PPH_COMPARE_FUNCTION)lParam;
        }
        return TRUE;
    case ELVM_SORTITEMS:
        {
            ListView_SortItemsEx(
                hwnd,
                PhpExtendedListViewCompareFunc,
                (LPARAM)context
                );
        }
        return TRUE;
    case ELVM_TICK:
        {
            if (context->EnableStateHighlighting > 0)
            {
                PhListTick(context);
            }
        }
        return TRUE;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

static VOID PhpSetSortIcon(
    __in HWND hwnd,
    __in INT Index,
    __in PH_SORT_ORDER Order
    )
{
    ULONG count;
    ULONG i;

    count = Header_GetItemCount(hwnd);

    if (count == -1)
        return;

    for (i = 0; i < count; i++)
    {
        HDITEM item;

        item.mask = HDI_FORMAT;
        Header_GetItem(hwnd, i, &item);

        if (Order != NoSortOrder && i == Index)
        {
            if (Order == AscendingSortOrder)
            {
                item.fmt &= ~HDF_SORTDOWN;
                item.fmt |= HDF_SORTUP;
            }
            else if (Order == DescendingSortOrder)
            {
                item.fmt &= ~HDF_SORTUP;
                item.fmt |= HDF_SORTDOWN;
            }
        }
        else
        {
            item.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        }

        Header_SetItem(hwnd, i, &item);
    }
}

static INT PhpCompareListViewItems(
    __in PPH_EXTLV_CONTEXT Context,
    __in INT X,
    __in INT Y,
    __in ULONG Column
    )
{
    INT result = 0;
    LVITEM xItem;
    LVITEM yItem;

    xItem.mask = LVIF_PARAM;
    xItem.iItem = X;
    xItem.iSubItem = 0;

    yItem.mask = LVIF_PARAM;
    yItem.iItem = Y;
    yItem.iSubItem = 0;

    if (!ListView_GetItem(Context->Handle, &xItem))
        return 0;
    if (!ListView_GetItem(Context->Handle, &yItem))
        return 0;

    if (
        Context->TriState &&
        Context->SortOrder == NoSortOrder &&
        Context->TriStateCompareFunction
        )
    {
        result = Context->TriStateCompareFunction(
            (PVOID)xItem.lParam,
            (PVOID)yItem.lParam,
            Context->Context
            );
    }

    if (result != 0)
        return result;

    if (
        Column < PH_MAX_COMPARE_FUNCTIONS &&
        Context->CompareFunctions[Column] != NULL
        )
    {
        result = PhModifySort(
            Context->CompareFunctions[Column]((PVOID)xItem.lParam, (PVOID)yItem.lParam, Context->Context),
            Context->SortOrder
            );
    }

    if (result != 0)
        return result;

    return PhModifySort(
        PhpDefaultCompareListViewItems(Context, X, Y, Column),
        Context->SortOrder
        );
}

static INT PhpExtendedListViewCompareFunc(
    __in LPARAM lParam1,
    __in LPARAM lParam2,
    __in LPARAM lParamSort
    )
{
    PPH_EXTLV_CONTEXT context = (PPH_EXTLV_CONTEXT)lParamSort;
    INT result;
    INT x = (INT)lParam1;
    INT y = (INT)lParam2;
    ULONG i;

    result = PhpCompareListViewItems(context, x, y, context->SortColumn);

    if (result != 0)
        return result;

    for (i = 0; i < context->FallbackColumns->Count; i++)
    {
        ULONG fallbackColumn = (ULONG)context->FallbackColumns->Items[i];

        if (fallbackColumn == context->SortColumn)
            continue;

        result = PhpCompareListViewItems(context, x, y, fallbackColumn);

        if (result != 0)
            return result;
    }

    return 0;
}

static INT PhpDefaultCompareListViewItems(
    __in PPH_EXTLV_CONTEXT Context,
    __in INT X,
    __in INT Y,
    __in ULONG Column
    )
{
    WCHAR xText[261];
    WCHAR yText[261]; 
    LVITEM item;

    // Get the X item text.

    item.mask = LVIF_TEXT;
    item.iItem = X;
    item.iSubItem = Column;
    item.pszText = yText;
    item.cchTextMax = 260;

    yText[0] = 0;
    ListView_GetItem(Context->Handle, &item);
    memcpy(xText, yText, 261 * 2);

    // Get the Y item text.

    item.iItem = Y;
    item.pszText = yText;
    item.cchTextMax = 260;

    yText[0] = 0;
    ListView_GetItem(Context->Handle, &item);

    // Compare them.

    if (StrCmpLogicalW_I)
    {
        return StrCmpLogicalW_I(xText, yText);
    }
    else
    {
        return wcsicmp(xText, yText);
    }
}

static VOID PhListTick(
    __in PPH_EXTLV_CONTEXT Context
    )
{
    HWND hwnd = Context->Handle;
    ULONG tickCount;
    BOOLEAN redrawDisabled = FALSE;
    PPH_LIST itemsToRemove = NULL;
    ULONG enumerationKey;
    PPH_TICK_ENTRY entry;

    tickCount = GetTickCount();

    // First pass

    enumerationKey = 0;

    while (PhEnumHashtable(Context->TickHashtable, &entry, &enumerationKey))
    {
        LVITEM item;
        PH_ITEM_STATE itemState;

        if (PhRoundNumber(tickCount - entry->TickCount, PH_DURATION_MULT) < Context->HighlightingDuration)
            continue;

        item.mask = LVIF_STATE;
        item.iItem = ListView_MapIDToIndex(hwnd, entry->Id);
        item.iSubItem = 0;
        item.stateMask = LVIS_STATEIMAGEMASK;
        CallWindowProc(Context->OldWndProc, hwnd, LVM_GETITEM, 0, (LPARAM)&item);
        itemState = PH_GET_ITEM_STATE(item.state);

        if (itemState == NewItemState)
        {
            item.state = INDEXTOSTATEIMAGEMASK(NormalItemState);
            ListView_SetItem(hwnd, &item);

            if (!itemsToRemove)
                itemsToRemove = PhCreateList(2);

            PhAddListItem(itemsToRemove, (PVOID)entry->Id);

            entry->TickCount = tickCount;
        }
    }

    // Second pass
    // This pass is specifically for deleting items.

    enumerationKey = 0;

    while (PhEnumHashtable(Context->TickHashtable, &entry, &enumerationKey))
    {
        LVITEM item;
        PH_ITEM_STATE itemState;

        if (itemsToRemove)
        {
            if (PhIndexOfListItem(itemsToRemove, (PVOID)entry->Id) != -1)
                continue;
        }

        if (PhRoundNumber(tickCount - entry->TickCount, PH_DURATION_MULT) < Context->HighlightingDuration)
            continue;

        item.mask = LVIF_STATE;
        item.iItem = ListView_MapIDToIndex(hwnd, entry->Id);
        item.iSubItem = 0;
        item.stateMask = LVIS_STATEIMAGEMASK;
        CallWindowProc(Context->OldWndProc, hwnd, LVM_GETITEM, 0, (LPARAM)&item);
        itemState = PH_GET_ITEM_STATE(item.state);

        if (itemState == RemovingItemState)
        {
            if (!redrawDisabled)
            {
                ExtendedListView_SetRedraw(hwnd, FALSE);
                redrawDisabled = TRUE;
            }

            CallWindowProc(Context->OldWndProc, hwnd, LVM_DELETEITEM, item.iItem, 0);

            if (!itemsToRemove)
                itemsToRemove = PhCreateList(2);

            PhAddListItem(itemsToRemove, (PVOID)entry->Id);

            entry->TickCount = tickCount;
        }
    }

    if (redrawDisabled)
    {
        ExtendedListView_SetRedraw(hwnd, TRUE);
    }

    if (itemsToRemove)
    {
        ULONG i;

        for (i = 0; i < itemsToRemove->Count; i++)
        {
            PH_TICK_ENTRY entry;

            entry.Id = (ULONG)itemsToRemove->Items[i];

            PhRemoveHashtableEntry(Context->TickHashtable, &entry);
        }

        PhDereferenceObject(itemsToRemove);
    }
}
