#include <phgui.h>
#include <windowsx.h>

#define PH_MAX_COMPARE_FUNCTIONS 16

typedef struct _PH_EXTLV_CONTEXT
{
    HWND Handle;
    WNDPROC OldWndProc;

    PVOID Context;
    BOOLEAN TriState;
    INT SortColumn;
    PH_SORT_ORDER SortOrder;

    PPH_COMPARE_FUNCTION TriStateCompareFunction;
    PPH_COMPARE_FUNCTION CompareFunctions[PH_MAX_COMPARE_FUNCTIONS];
    PPH_LIST FallbackColumns;
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
    case ELVM_INIT:
        {
            PhpSetSortIcon(ListView_GetHeader(hwnd), context->SortColumn, context->SortOrder);

            return TRUE;
        }
        break;
    case ELVM_SORTITEMS:
        {
            ListView_SortItemsEx(
                hwnd,
                PhpExtendedListViewCompareFunc,
                (LPARAM)context
                );

            return TRUE;
        }
        break;
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

    if (
        Context->TriState &&
        Context->SortOrder == NoSortOrder &&
        Context->TriStateCompareFunction
        )
    {
        result = Context->TriStateCompareFunction((PVOID)X, (PVOID)Y, Context->Context);
    }

    if (result != 0)
        return result;

    if (
        Column < PH_MAX_COMPARE_FUNCTIONS &&
        Context->CompareFunctions[Column] != NULL
        )
    {
        result = PhModifySort(
            Context->CompareFunctions[Column]((PVOID)X, (PVOID)Y, Context->Context),
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
    WCHAR buffer[261]; 
    LVITEM item;

    // Get the X item text.

    item.mask = LVIF_TEXT;
    item.iItem = X;
    item.iSubItem = Column;
    item.pszText = buffer;
    item.cchTextMax = 260;

    buffer[0] = 0;
    ListView_GetItem(Context->Handle, &item);
    memcpy(xText, buffer, 261 * 2);

    // Get the Y item text.

    item.iItem = Y;
    item.pszText = buffer;
    item.cchTextMax = 260;

    buffer[0] = 0;
    ListView_GetItem(Context->Handle, &item);
    memcpy(yText, buffer, 261 * 2);

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
