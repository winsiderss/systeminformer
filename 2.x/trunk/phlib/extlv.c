/*
 * Process Hacker -
 *   extended list view
 *
 * Copyright (C) 2010-2011 wj32
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

/*
 * The extended list view adds some functionality to the default list view control, such
 * as sorting, (state) highlighting, better redraw disabling, and the ability to change
 * the cursor. This is currently implemented by hooking the window procedure.
 */

#include <phgui.h>
#include <windowsx.h>

#define PH_DURATION_MULT 100
#define PH_MAX_COMPARE_FUNCTIONS 16

// We have nowhere else to store state highlighting
// information except for the state image index
// value. This is conveniently 4 bits wide.

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
    BOOLEAN SortFast;

    PPH_COMPARE_FUNCTION TriStateCompareFunction;
    PPH_COMPARE_FUNCTION CompareFunctions[PH_MAX_COMPARE_FUNCTIONS];
    ULONG FallbackColumns[PH_MAX_COMPARE_FUNCTIONS];
    ULONG NumberOfFallbackColumns;

    // State Highlighting

    BOOLEAN EnableState;
    LONG EnableStateHighlighting;
    ULONG HighlightingDuration;
    COLORREF NewColor;
    COLORREF RemovingColor;
    PPH_EXTLV_GET_ITEM_COLOR ItemColorFunction;
    PPH_EXTLV_GET_ITEM_FONT ItemFontFunction;
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

INT PhpExtendedListViewCompareFunc(
    __in LPARAM lParam1,
    __in LPARAM lParam2,
    __in LPARAM lParamSort
    );

INT PhpExtendedListViewCompareFastFunc(
    __in LPARAM lParam1,
    __in LPARAM lParam2,
    __in LPARAM lParamSort
    );

INT PhpCompareListViewItems(
    __in PPH_EXTLV_CONTEXT Context,
    __in_opt INT X,
    __in_opt INT Y,
    __in PVOID XParam,
    __in PVOID YParam,
    __in ULONG Column,
    __in BOOLEAN EnableDefault
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

static PWSTR PhpMakeExtLvContextAtom(
    VOID
    )
{
    PH_DEFINE_MAKE_ATOM(L"PhLib_ExtLvContext");
}

/**
 * Enables extended list view support for a list view control.
 *
 * \param hWnd A handle to the list view control.
 */
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
    context->SortFast = FALSE;
    context->TriStateCompareFunction = NULL;
    memset(context->CompareFunctions, 0, sizeof(context->CompareFunctions));
    context->NumberOfFallbackColumns = 0;

    context->EnableState = FALSE;
    context->EnableStateHighlighting = 0;
    context->HighlightingDuration = 1000;
    context->NewColor = RGB(0x00, 0xff, 0x00);
    context->RemovingColor = RGB(0xff, 0x00, 0x00);
    context->ItemColorFunction = NULL;
    context->ItemFontFunction = NULL;
    context->TickHashtable = NULL;

    context->EnableRedraw = 1;
    context->Cursor = NULL;

    SetProp(hWnd, PhpMakeExtLvContextAtom(), (HANDLE)context);

    ExtendedListView_Init(hWnd);
}

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

FORCEINLINE VOID PhpEnsureTickHashtableCreated(
    __in PPH_EXTLV_CONTEXT Context
    )
{
    if (!Context->TickHashtable)
    {
        Context->TickHashtable = PhCreateHashtable(
            sizeof(PH_TICK_ENTRY),
            PhpTickHashtableCompareFunction,
            PhpTickHashtableHashFunction,
            20
            );
    }
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

    context = (PPH_EXTLV_CONTEXT)GetProp(hwnd, PhpMakeExtLvContextAtom());
    oldWndProc = context->OldWndProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);

            if (context->TickHashtable)
                PhDereferenceObject(context->TickHashtable);

            PhFree(context);
            RemoveProp(hwnd, PhpMakeExtLvContextAtom());
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case HDN_ITEMCLICK:
                {
                    HWND headerHandle;

                    headerHandle = (HWND)CallWindowProc(context->OldWndProc, hwnd, LVM_GETHEADER, 0, 0);

                    if (header->hwndFrom == headerHandle)
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

                        PhSetHeaderSortIcon(headerHandle, context->SortColumn, context->SortOrder);
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

                                if (!context->EnableState || itemState == NormalItemState)
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
    case WM_UPDATEUISTATE:
        {
            // Disable focus rectangles by setting or masking out the flag where appropriate.
            switch (LOWORD(wParam))
            {
            case UIS_SET:
                wParam |= UISF_HIDEFOCUS << 16;
                break;
            case UIS_CLEAR:
            case UIS_INITIALIZE:
                wParam &= ~(UISF_HIDEFOCUS << 16);
                break;
            }
        }
        break;
    case LVM_INSERTITEM:
        {
            LPLVITEM item = (LPLVITEM)lParam;
            INT index;

            if (!context->EnableState)
                break; // pass through

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

                PhpEnsureTickHashtableCreated(context);

                entry.Id = ListView_MapIndexToID(hwnd, index);
                entry.TickCount = GetTickCount();

                PhAddEntryHashtable(context->TickHashtable, &entry);
            }

            return index;
        }
    case LVM_DELETEITEM:
        {
            if (!context->EnableState)
                break; // pass through

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

                CallWindowProc(context->OldWndProc, hwnd, LVM_SETITEM, 0, (LPARAM)&item);

                {
                    PH_TICK_ENTRY localEntry;
                    PPH_TICK_ENTRY entry;

                    PhpEnsureTickHashtableCreated(context);

                    localEntry.Id = ListView_MapIndexToID(hwnd, (INT)wParam);
                    entry = PhAddEntryHashtableEx(context->TickHashtable, &localEntry, NULL);

                    entry->TickCount = GetTickCount();
                }

                return TRUE;
            }
            else
            {
                // The item may still be under state highlighting.
                if (context->TickHashtable)
                {
                    PH_TICK_ENTRY entry;

                    entry.Id = ListView_MapIndexToID(hwnd, (INT)wParam);
                    PhRemoveEntryHashtable(context->TickHashtable, &entry);
                }
            }
        }
        break;
    case LVM_GETITEM:
        {
            LVITEM item;
            ULONG itemState;
            ULONG oldMask;
            ULONG oldStateMask;

            if (!context->EnableState)
                break; // pass through

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

            if (itemState == RemovingItemState)
                return FALSE;

            item.mask = oldMask;
            item.stateMask = oldStateMask;
            item.state &= item.stateMask;

            memcpy((LPLVITEM)lParam, &item, sizeof(LVITEM));
        }
        return TRUE;
    case ELVM_ADDFALLBACKCOLUMN:
        {
            if (context->NumberOfFallbackColumns < PH_MAX_COMPARE_FUNCTIONS)
                context->FallbackColumns[context->NumberOfFallbackColumns++] = (ULONG)wParam;
            else
                return FALSE;
        }
        return TRUE;
    case ELVM_ADDFALLBACKCOLUMNS:
        {
            ULONG numberOfColumns = (ULONG)wParam;
            PULONG columns = (PULONG)lParam;

            if (context->NumberOfFallbackColumns + numberOfColumns <= PH_MAX_COMPARE_FUNCTIONS)
            {
                memcpy(
                    &context->FallbackColumns[context->NumberOfFallbackColumns],
                    columns,
                    numberOfColumns * sizeof(ULONG)
                    );
                context->NumberOfFallbackColumns += numberOfColumns;
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case ELVM_ENABLESTATE:
        {
            context->EnableState = !!wParam;
        }
        return TRUE;
    case ELVM_INIT:
        {
            PhSetHeaderSortIcon(ListView_GetHeader(hwnd), context->SortColumn, context->SortOrder);

            // HACK to fix tooltips showing behind Always On Top windows.
            SetWindowPos(ListView_GetToolTips(hwnd), HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

            // Make sure focus rectangles are disabled.
            SendMessage(hwnd, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);
        }
        return TRUE;
    case ELVM_SETCOLUMNWIDTH:
        {
            ULONG column = (ULONG)wParam;
            LONG width = (LONG)lParam;

            if (width == ELVSCW_AUTOSIZE_REMAININGSPACE)
            {
                RECT clientRect;
                LONG availableWidth;
                ULONG i;
                LVCOLUMN lvColumn;

                GetClientRect(hwnd, &clientRect);
                availableWidth = clientRect.right;
                i = 0;
                lvColumn.mask = LVCF_WIDTH;

                while (TRUE)
                {
                    if (i != column)
                    {
                        if (CallWindowProc(oldWndProc, hwnd, LVM_GETCOLUMN, i, (LPARAM)&lvColumn))
                        {
                            availableWidth -= lvColumn.cx;
                        }
                        else
                        {
                            break;
                        }
                    }

                    i++;
                }

                if (availableWidth >= 40)
                    return CallWindowProc(oldWndProc, hwnd, LVM_SETCOLUMNWIDTH, column, availableWidth);
            }

            return CallWindowProc(oldWndProc, hwnd, LVM_SETCOLUMNWIDTH, column, width);
        }
        break;
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
    case ELVM_SETHIGHLIGHTINGDURATION:
        {
            context->HighlightingDuration = (ULONG)wParam;
        }
        return TRUE;
    case ELVM_SETITEMCOLORFUNCTION:
        {
            context->ItemColorFunction = (PPH_EXTLV_GET_ITEM_COLOR)lParam;
        }
        return TRUE;
    case ELVM_SETITEMFONTFUNCTION:
        {
            context->ItemFontFunction = (PPH_EXTLV_GET_ITEM_FONT)lParam;
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

            PhSetHeaderSortIcon(ListView_GetHeader(hwnd), context->SortColumn, context->SortOrder);
        }
        return TRUE;
    case ELVM_SETSORTFAST:
        {
            context->SortFast = !!wParam;
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
            if (context->SortFast)
            {
                // This sort method is faster than the normal sort because our comparison function
                // doesn't have to call the list view window procedure to get the item lParam values.
                // The disadvantage of this method is that default sorting is not available - if a
                // column doesn't have a comparison function, it doesn't get sorted at all.

                ListView_SortItems(
                    hwnd,
                    PhpExtendedListViewCompareFastFunc,
                    (LPARAM)context
                    );
            }
            else
            {
                ListView_SortItemsEx(
                    hwnd,
                    PhpExtendedListViewCompareFunc,
                    (LPARAM)context
                    );
            }
        }
        return TRUE;
    case ELVM_TICK:
        {
            if (
                context->EnableStateHighlighting > 0 &&
                context->TickHashtable &&
                context->TickHashtable->Count != 0
                )
            {
                PhListTick(context);
            }
        }
        return TRUE;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

/**
 * Visually indicates the sort order of a header control item.
 *
 * \param hwnd A handle to the header control.
 * \param Index The index of the item.
 * \param Order The sort order of the item.
 */
VOID PhSetHeaderSortIcon(
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
    PULONG fallbackColumns;
    LVITEM xItem;
    LVITEM yItem;

    // Get the param values.

    xItem.mask = LVIF_PARAM | LVIF_STATE;
    xItem.iItem = x;
    xItem.iSubItem = 0;

    yItem.mask = LVIF_PARAM | LVIF_STATE;
    yItem.iItem = y;
    yItem.iSubItem = 0;

    // Don't use SendMessage/ListView_* because it will call our new window procedure,
    // which will use GetProp. This calls NtUserGetProp, and obviously having a system call
    // in a comparison function is very, very bad for performance.

    if (!CallWindowProc(context->OldWndProc, context->Handle, LVM_GETITEM, 0, (LPARAM)&xItem))
        return 0;
    if (!CallWindowProc(context->OldWndProc, context->Handle, LVM_GETITEM, 0, (LPARAM)&yItem))
        return 0;
    if (PH_GET_ITEM_STATE(xItem.state) == RemovingItemState)
        return 0;
    if (PH_GET_ITEM_STATE(yItem.state) == RemovingItemState)
        return 0;

    // First, do tri-state sorting.

    if (
        context->TriState &&
        context->SortOrder == NoSortOrder &&
        context->TriStateCompareFunction
        )
    {
        result = context->TriStateCompareFunction(
            (PVOID)xItem.lParam,
            (PVOID)yItem.lParam,
            context->Context
            );

        if (result != 0)
            return result;
    }

    // Compare using the user-selected column and move on to the fallback columns if necessary.

    result = PhpCompareListViewItems(context, x, y, (PVOID)xItem.lParam, (PVOID)yItem.lParam, context->SortColumn, TRUE);

    if (result != 0)
        return result;

    fallbackColumns = context->FallbackColumns;

    for (i = context->NumberOfFallbackColumns; i != 0; i--)
    {
        ULONG fallbackColumn = *fallbackColumns++;

        if (fallbackColumn == context->SortColumn)
            continue;

        result = PhpCompareListViewItems(context, x, y, (PVOID)xItem.lParam, (PVOID)yItem.lParam, fallbackColumn, TRUE);

        if (result != 0)
            return result;
    }

    return 0;
}

static INT PhpExtendedListViewCompareFastFunc(
    __in LPARAM lParam1,
    __in LPARAM lParam2,
    __in LPARAM lParamSort
    )
{
    PPH_EXTLV_CONTEXT context = (PPH_EXTLV_CONTEXT)lParamSort;
    INT result;
    ULONG i;
    PULONG fallbackColumns;

    if (!lParam1 || !lParam2)
        return 0;

    // First, do tri-state sorting.

    if (
        context->TriState &&
        context->SortOrder == NoSortOrder &&
        context->TriStateCompareFunction
        )
    {
        result = context->TriStateCompareFunction(
            (PVOID)lParam1,
            (PVOID)lParam2,
            context->Context
            );

        if (result != 0)
            return result;
    }

    // Compare using the user-selected column and move on to the fallback columns if necessary.

    result = PhpCompareListViewItems(context, 0, 0, (PVOID)lParam1, (PVOID)lParam2, context->SortColumn, FALSE);

    if (result != 0)
        return result;

    fallbackColumns = context->FallbackColumns;

    for (i = context->NumberOfFallbackColumns; i != 0; i--)
    {
        ULONG fallbackColumn = *fallbackColumns++;

        if (fallbackColumn == context->SortColumn)
            continue;

        result = PhpCompareListViewItems(context, 0, 0, (PVOID)lParam1, (PVOID)lParam2, fallbackColumn, FALSE);

        if (result != 0)
            return result;
    }

    return 0;
}

static FORCEINLINE INT PhpCompareListViewItems(
    __in PPH_EXTLV_CONTEXT Context,
    __in INT X,
    __in INT Y,
    __in PVOID XParam,
    __in PVOID YParam,
    __in ULONG Column,
    __in BOOLEAN EnableDefault
    )
{
    INT result = 0;

    if (
        Column < PH_MAX_COMPARE_FUNCTIONS &&
        Context->CompareFunctions[Column]
        )
    {
        result = PhModifySort(
            Context->CompareFunctions[Column](XParam, YParam, Context->Context),
            Context->SortOrder
            );

        if (result != 0)
            return result;
    }

    if (EnableDefault)
    {
        return PhModifySort(
            PhpDefaultCompareListViewItems(Context, X, Y, Column),
            Context->SortOrder
            );
    }
    else
    {
        return 0;
    }
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
    item.pszText = xText;
    item.cchTextMax = 260;

    xText[0] = 0;
    CallWindowProc(Context->OldWndProc, Context->Handle, LVM_GETITEM, 0, (LPARAM)&item);

    // Get the Y item text.

    item.iItem = Y;
    item.pszText = yText;
    item.cchTextMax = 260;

    yText[0] = 0;
    CallWindowProc(Context->OldWndProc, Context->Handle, LVM_GETITEM, 0, (LPARAM)&item);

    // Compare them.

#if 1
    return PhCompareUnicodeStringZNatural(xText, yText, TRUE);
#else
    return wcsicmp(xText, yText);
#endif
}

static VOID PhListTick(
    __in PPH_EXTLV_CONTEXT Context
    )
{
    HWND hwnd = Context->Handle;
    ULONG tickCount;
    BOOLEAN redrawDisabled = FALSE;
    PPH_LIST itemsToRemove = NULL;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_TICK_ENTRY entry;

    if (!Context->TickHashtable)
        return;

    tickCount = GetTickCount();

    // First pass

    PhBeginEnumHashtable(Context->TickHashtable, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
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
            CallWindowProc(Context->OldWndProc, hwnd, LVM_SETITEM, 0, (LPARAM)&item);

            if (!itemsToRemove)
                itemsToRemove = PhCreateList(2);

            PhAddItemList(itemsToRemove, (PVOID)entry->Id);

            entry->TickCount = tickCount;
        }
    }

    // Second pass
    // This pass is specifically for deleting items.

    PhBeginEnumHashtable(Context->TickHashtable, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
    {
        LVITEM item;
        PH_ITEM_STATE itemState;

        if (itemsToRemove)
        {
            if (PhFindItemList(itemsToRemove, (PVOID)entry->Id) != -1)
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

            PhAddItemList(itemsToRemove, (PVOID)entry->Id);

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
            PH_TICK_ENTRY removeEntry;

            removeEntry.Id = (ULONG)itemsToRemove->Items[i];

            PhRemoveEntryHashtable(Context->TickHashtable, &removeEntry);
        }

        PhDereferenceObject(itemsToRemove);
    }
}
