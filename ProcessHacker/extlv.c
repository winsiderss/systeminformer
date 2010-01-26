#include <phgui.h>
#include <windowsx.h>

#define PH_MAX_COMPARE_FUNCTIONS 16

// We have nowhere else to store state highlighting 
// information except for the state image index 
// value. This is conveniently 4 bits wide.

typedef enum _PH_ITEM_STATE
{
    // The item is normal. Use the ItemColorFunction 
    // to determine the color of the item.
    NormalItemState = 0,
    // The item is new. On the next tick, change the 
    // state to NewPostItemState. When an item is 
    // in this state, highlight it in NewColor.
    NewItemState,
    // The item is new. On the next tick, 
    // change the state to NormalItemState. When an 
    // item is in this state, highlight it in NewColor.
    NewPostItemState,
    // The item is being removed. On the next tick, 
    // change the state to RemovingPostItemState. When 
    // an item is in this state, highlight it in 
    // RemovingColor.
    RemovingItemState,
    // The item is being removed. On the next tick,
    // delete the item. When an item is in this state, 
    // highlight it in RemovingColor.
    RemovingPostItemState
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
    ULONG NumberOfTickItems;
} PH_EXTLV_CONTEXT, *PPH_EXTLV_CONTEXT;

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
    context->NumberOfTickItems = 0;

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

            PhFree(context);
            RemoveProp(hwnd, L"ExtLvContext");

            if (context->EnableStateHighlighting > 0)
                KillTimer(hwnd, PH_STATE_TIMER);
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

                                item.mask = LVIF_STATE;
                                item.iItem = customDraw->nmcd.dwItemSpec;
                                item.iSubItem = 0;
                                item.stateMask = LVIS_STATEIMAGEMASK;
                                ListView_GetItem(hwnd, &item);
                                itemState = PH_GET_ITEM_STATE(item.state);

                                if (itemState == NormalItemState)
                                {
                                    if (context->ItemColorFunction)
                                    {
                                        customDraw->clrTextBk = context->ItemColorFunction(
                                            customDraw->nmcd.dwItemSpec,
                                            (PVOID)customDraw->nmcd.lItemlParam,
                                            context->Context
                                            );
                                    }
                                }
                                else if (
                                    itemState == NewItemState ||
                                    itemState == NewPostItemState
                                    )
                                {
                                    customDraw->clrTextBk = context->NewColor;
                                }
                                else if (
                                    itemState == RemovingItemState ||
                                    itemState == RemovingPostItemState
                                    )
                                {
                                    customDraw->clrTextBk = context->RemovingColor;
                                }
                            }
                            return CDRF_DODEFAULT;
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_TIMER:
        {
            if (wParam == PH_STATE_TIMER)
            {
                INT index = -1;

                // First pass

                while (
                    context->NumberOfTickItems != 0 &&
                    (index = ListView_GetNextItem(hwnd, index, LVNI_ALL)) != -1
                    )
                {
                    LVITEM item;
                    PH_ITEM_STATE itemState;

                    item.mask = LVIF_STATE;
                    item.iItem = index;
                    item.iSubItem = 0;
                    item.stateMask = LVIS_STATEIMAGEMASK;
                    ListView_GetItem(hwnd, &item);
                    itemState = PH_GET_ITEM_STATE(item.state);

                    if (itemState == NewItemState)
                    {
                        item.state = INDEXTOSTATEIMAGEMASK(NewPostItemState);
                        ListView_SetItem(hwnd, &item);
                    }
                    else if (itemState == NewPostItemState)
                    {
                        item.state = INDEXTOSTATEIMAGEMASK(NormalItemState);
                        ListView_SetItem(hwnd, &item);

                        context->NumberOfTickItems--;
                    }
                    else if (itemState == RemovingItemState)
                    {
                        item.state = INDEXTOSTATEIMAGEMASK(RemovingPostItemState);
                        ListView_SetItem(hwnd, &item);
                    }
                }

                // Second pass
                // This pass is specifically for deleting items.

                while (
                    context->NumberOfTickItems != 0 &&
                    (index = ListView_GetNextItem(hwnd, index, LVNI_ALL)) != -1
                    )
                {
                    LVITEM item;
                    PH_ITEM_STATE itemState;

                    item.mask = LVIF_STATE;
                    item.iItem = index;
                    item.iSubItem = 0;
                    item.stateMask = LVIS_STATEIMAGEMASK;
                    ListView_GetItem(hwnd, &item);
                    itemState = PH_GET_ITEM_STATE(item.state);

                    if (itemState == RemovingPostItemState)
                    {
                        CallWindowProc(oldWndProc, hwnd, LVM_DELETEITEM, index, 0);
                        context->NumberOfTickItems--;

                        // Start the search again. There doesn't seem to be any 
                        // other way of doing this.
                        index = -1;
                    }
                }

                return 0;
            }
        }
        break;
    case LVM_INSERTITEM:
        {
            LPLVITEM item = (LPLVITEM)lParam;

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
                context->NumberOfTickItems++;
            }
            else
            {
                item->state = NormalItemState;
            }
        }
        break;
    case LVM_DELETEITEM:
        {
            if (context->EnableStateHighlighting > 0)
            {
                LVITEM item;

                item.mask = LVIF_STATE;
                item.iItem = (INT)wParam;
                item.iSubItem = 0;
                item.stateMask = LVIS_STATEIMAGEMASK;
                ListView_GetItem(hwnd, &item);

                // We only increment the tick items counter if we're 
                // transitioning from NormalItemState to RemovingItemState.
                if (PH_GET_ITEM_STATE(item.state) == NormalItemState)
                    context->NumberOfTickItems++;

                item.state = INDEXTOSTATEIMAGEMASK(RemovingItemState);
                ListView_SetItem(hwnd, &item);

                return TRUE;
            }
        }
        break;
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
    case ELVM_SETITEMCOLORFUNCTION:
        {
            context->ItemColorFunction = (PPH_GET_ITEM_COLOR)lParam;
        }
        return TRUE;
    case ELVM_SETNEWCOLOR:
        {
            context->NewColor = (COLORREF)wParam;
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

            if (context->EnableStateHighlighting == 1)
            {
                SetTimer(hwnd, PH_STATE_TIMER, context->HighlightingDuration, NULL);
            }
            else if (context->EnableStateHighlighting == 0)
            {
                KillTimer(hwnd, PH_STATE_TIMER);
            }
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
    ULONG xItemState;
    ULONG yItemState;

    xItem.mask = LVIF_PARAM | LVIF_STATE;
    xItem.iItem = X;
    xItem.iSubItem = 0;
    xItem.stateMask = LVIS_STATEIMAGEMASK;

    yItem.mask = LVIF_PARAM | LVIF_STATE;
    yItem.iItem = Y;
    yItem.iSubItem = 0;
    yItem.stateMask = LVIS_STATEIMAGEMASK;

    if (!ListView_GetItem(Context->Handle, &xItem))
        return 0;
    if (!ListView_GetItem(Context->Handle, &yItem))
        return 0;

    // Check if one of the items is being deleted. 
    // We can't pass them to the compare function 
    // because it could mean that the pointer 
    // stored in the param field is invalid.
    xItemState = PH_GET_ITEM_STATE(xItem.state);
    yItemState = PH_GET_ITEM_STATE(yItem.state);

    if (xItemState == RemovingItemState || xItemState == RemovingPostItemState)
        return 0;
    if (yItemState == RemovingItemState || yItemState == RemovingPostItemState)
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
