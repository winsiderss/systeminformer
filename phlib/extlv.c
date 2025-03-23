/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2012
 *     dmex    2017-2024
 *
 */

/*
 * The extended list view adds some functionality to the default list view control, such as sorting,
 * item colors and fonts, better redraw disabling, and the ability to change the cursor. This is
 * currently implemented by hooking the window procedure.
 */

#include <ph.h>
#include <guisup.h>

#define PH_MAX_COMPARE_FUNCTIONS 16

typedef struct _PH_EXTLV_CONTEXT
{
    HWND Handle;
    WNDPROC OldWndProc;
    PVOID Context;

    // Sorting

    BOOLEAN TriState;
    LONG SortColumn;
    PH_SORT_ORDER SortOrder;
    BOOLEAN SortFast;

    PPH_COMPARE_FUNCTION TriStateCompareFunction;
    PPH_COMPARE_FUNCTION CompareFunctions[PH_MAX_COMPARE_FUNCTIONS];
    ULONG FallbackColumns[PH_MAX_COMPARE_FUNCTIONS];
    ULONG NumberOfFallbackColumns;

    // Color and Font
    PPH_EXTLV_GET_ITEM_COLOR ItemColorFunction;
    PPH_EXTLV_GET_ITEM_FONT ItemFontFunction;

    // Misc.

    LONG EnableRedraw;
    LONG WindowDpi;
    HCURSOR Cursor;
    IListView* ListViewClass;
} PH_EXTLV_CONTEXT, *PPH_EXTLV_CONTEXT;

LRESULT CALLBACK PhpExtendedListViewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT PhpExtendedListViewCompareFunc(
    _In_ LPARAM lParam1,
    _In_ LPARAM lParam2,
    _In_ LPARAM lParamSort
    );

INT PhpExtendedListViewCompareFastFunc(
    _In_ LPARAM lParam1,
    _In_ LPARAM lParam2,
    _In_ LPARAM lParamSort
    );

INT PhpCompareListViewItems(
    _In_ PPH_EXTLV_CONTEXT Context,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ PVOID XParam,
    _In_ PVOID YParam,
    _In_ ULONG Column,
    _In_ BOOLEAN EnableDefault
    );

INT PhpDefaultCompareListViewItems(
    _In_ PPH_EXTLV_CONTEXT Context,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ ULONG Column
    );

HWND PhGetExtendedListViewHeader(
    _In_ PPH_EXTLV_CONTEXT Context
    );

HWND PhGetExtendedListViewTooltips(
    _In_ PPH_EXTLV_CONTEXT Context
    );

/**
 * Enables extended list view support for a list view control.
 *
 * \param WindowHandle A handle to the list view control.
 */
VOID PhSetExtendedListView(
    _In_ HWND WindowHandle
    )
{
    PhSetExtendedListViewEx(WindowHandle, 0, AscendingSortOrder);
}

VOID PhSetExtendedListViewEx(
    _In_ HWND WindowHandle,
    _In_ ULONG SortColumn,
    _In_ ULONG SortOrder
    )
{
    PPH_EXTLV_CONTEXT context;

    context = PhAllocateZero(sizeof(PH_EXTLV_CONTEXT));
    context->Handle = WindowHandle;
    context->Context = NULL;
    context->TriState = FALSE;
    context->SortColumn = SortColumn;
    context->SortOrder = SortOrder;
    context->SortFast = FALSE;
    context->TriStateCompareFunction = NULL;
    memset(context->CompareFunctions, 0, sizeof(context->CompareFunctions));
    context->NumberOfFallbackColumns = 0;
    context->ItemColorFunction = NULL;
    context->ItemFontFunction = NULL;
    context->EnableRedraw = 1;
    context->Cursor = NULL;

    context->ListViewClass = PhGetListViewInterface(WindowHandle);

    context->OldWndProc = PhGetWindowProcedure(WindowHandle);
    PhSetWindowContext(WindowHandle, MAXCHAR, context);
    PhSetWindowProcedure(WindowHandle, PhpExtendedListViewWndProc);

    ExtendedListView_Init(WindowHandle);
}

LRESULT CALLBACK PhpExtendedListViewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_EXTLV_CONTEXT context;
    WNDPROC oldWndProc;

    context = PhGetWindowContext(WindowHandle, MAXCHAR);

    if (!context)
        return 0;

    oldWndProc = context->OldWndProc;

    switch (WindowMessage)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, MAXCHAR);
            PhSetWindowProcedure(WindowHandle, oldWndProc);

            if (context->ListViewClass)
            {
                IListView_Release(context->ListViewClass);
                context->ListViewClass = NULL;
            }

            PhFree(context);
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

                    headerHandle = PhGetExtendedListViewHeader(context);

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
                        ExtendedListView_SortItems(WindowHandle);
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
                    if (header->hwndFrom == WindowHandle)
                    {
                        LPNMLVCUSTOMDRAW customDraw = (LPNMLVCUSTOMDRAW)header;

                        switch (customDraw->nmcd.dwDrawStage)
                        {
                        case CDDS_PREPAINT:
                            return CDRF_NOTIFYITEMDRAW;
                        case CDDS_ITEMPREPAINT:
                            {
                                BOOLEAN colorChanged = FALSE;
                                BOOLEAN selected = FALSE;
                                HFONT newFont = NULL;

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
                                    SelectFont(customDraw->nmcd.hdc, newFont);

                                // Fix text readability for hot and selected colored items (Dart Vanya)

                                if (PhEnableThemeSupport)
                                {
                                    ULONG state;

                                    if (context->ListViewClass && SUCCEEDED(IListView_GetItemState(
                                        context->ListViewClass,
                                        (INT)customDraw->nmcd.dwItemSpec,
                                        0,
                                        LVIS_SELECTED,
                                        &state
                                        )))
                                    {
                                        if (FlagOn(customDraw->nmcd.uItemState, CDIS_HOT) || FlagOn(state, LVIS_SELECTED))
                                        {
                                            selected = TRUE;
                                        }
                                    }
                                    else
                                    {
                                        if (FlagOn(customDraw->nmcd.uItemState, CDIS_HOT) || FlagOn(CallWindowProc(
                                            oldWndProc,
                                            WindowHandle,
                                            LVM_GETITEMSTATE,
                                            customDraw->nmcd.dwItemSpec,
                                            LVIS_SELECTED
                                            ), LVIS_SELECTED))
                                        {
                                            selected = TRUE;
                                        }
                                    }
                                }

                                if (selected)
                                {
                                    customDraw->clrText = PhThemeWindowTextColor;
                                }
                                else if (colorChanged)
                                {
                                    if (PhGetColorBrightness(customDraw->clrTextBk) > 100) // slightly less than half
                                        customDraw->clrText = RGB(0x00, 0x00, 0x00);
                                    else
                                        customDraw->clrText = RGB(0xff, 0xff, 0xff);
                                }

                                if (newFont)
                                    return CDRF_NEWFONT;
                                else
                                    return CDRF_DODEFAULT;
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
                PhSetCursor(context->Cursor);
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
    case ELVM_INIT:
        {
            context->WindowDpi = PhGetWindowDpi(WindowHandle);

            PhSetHeaderSortIcon(PhGetExtendedListViewHeader(context), context->SortColumn, context->SortOrder);

            // Fix tooltips showing behind Always On Top windows.
            SetWindowPos(PhGetExtendedListViewTooltips(context), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

            // Make sure focus rectangles are disabled.
            SendMessage(WindowHandle, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);
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

                GetClientRect(WindowHandle, &clientRect);
                availableWidth = clientRect.right;
                i = 0;
                lvColumn.mask = LVCF_WIDTH;

                while (TRUE)
                {
                    if (i != column)
                    {
                        if (context->ListViewClass)
                        {
                            if (SUCCEEDED(IListView_GetColumn(context->ListViewClass, i, &lvColumn)))
                            {
                                availableWidth -= lvColumn.cx;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            if (CallWindowProc(oldWndProc, WindowHandle, LVM_GETCOLUMN, i, (LPARAM)&lvColumn))
                            {
                                availableWidth -= lvColumn.cx;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }

                    i++;
                }

                if (availableWidth >= 40)
                {
                    if (context->ListViewClass && SUCCEEDED(IListView_SetColumnWidth(context->ListViewClass, column, availableWidth)))
                        return TRUE;

                    return CallWindowProc(oldWndProc, WindowHandle, LVM_SETCOLUMNWIDTH, column, availableWidth);
                }
            }

            if (context->ListViewClass && SUCCEEDED(IListView_SetColumnWidth(context->ListViewClass, column, width)))
                return TRUE;

            return CallWindowProc(oldWndProc, WindowHandle, LVM_SETCOLUMNWIDTH, column, width);
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
    case ELVM_SETREDRAW:
        {
            if (wParam)
                context->EnableRedraw++;
            else
                context->EnableRedraw--;

            if (context->EnableRedraw == 1)
            {
                SendMessage(WindowHandle, WM_SETREDRAW, TRUE, 0);
                InvalidateRect(WindowHandle, NULL, FALSE);
            }
            else if (context->EnableRedraw == 0)
            {
                SendMessage(WindowHandle, WM_SETREDRAW, FALSE, 0);
            }
        }
        return TRUE;
    case ELVM_GETSORT:
        {
            PULONG sortColumn = (PULONG)wParam;
            PPH_SORT_ORDER sortOrder = (PPH_SORT_ORDER)lParam;

            if (sortColumn)
                *sortColumn = context->SortColumn;
            if (sortOrder)
                *sortOrder = context->SortOrder;
        }
        return TRUE;
    case ELVM_SETSORT:
        {
            context->SortColumn = (ULONG)wParam;
            context->SortOrder = (PH_SORT_ORDER)lParam;

            PhSetHeaderSortIcon(PhGetExtendedListViewHeader(context), context->SortColumn, context->SortOrder);
        }
        return TRUE;
    case ELVM_SETSORTFAST:
        {
            context->SortFast = !!wParam;
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
            BOOL result;

            if (context->SortFast)
            {
                // This sort method is faster than the normal sort because our comparison function
                // doesn't have to call the list view window procedure to get the item lParam
                // values. The disadvantage of this method is that default sorting is not available
                // - if a column doesn't have a comparison function, it doesn't get sorted at all.

                if (context->ListViewClass)
                {
                    result = SUCCEEDED(IListView_SortItems(
                        context->ListViewClass,
                        FALSE,
                        (WPARAM)context,
                        (PFNLVCOMPARE)PhpExtendedListViewCompareFastFunc
                        ));
                }
                else
                {
                    result = (BOOL)CallWindowProc( // ListView_SortItems
                        oldWndProc,
                        WindowHandle,
                        LVM_SORTITEMS,
                        (WPARAM)context,
                        (LPARAM)(PFNLVCOMPARE)PhpExtendedListViewCompareFastFunc
                        );
                }
            }
            else
            {
                if (context->ListViewClass)
                {
                    result = SUCCEEDED(IListView_SortItems(
                        context->ListViewClass,
                        TRUE,
                        (WPARAM)context,
                        (PFNLVCOMPARE)PhpExtendedListViewCompareFunc
                        ));
                }
                else
                {
                    result = (BOOL)CallWindowProc( // ListView_SortItemsEx
                        oldWndProc,
                        WindowHandle,
                        LVM_SORTITEMSEX,
                        (WPARAM)context,
                        (LPARAM)(PFNLVCOMPARE)PhpExtendedListViewCompareFunc
                        );
                }
            }

            return result;
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            LONG listviewDpi;
            LVCOLUMN lvColumn;
            ULONG i;

            i = 0;
            lvColumn.mask = LVCF_WIDTH;
            listviewDpi = PhGetWindowDpi(WindowHandle);

            if (context->WindowDpi == 0)
                break;

            // Workaround the ListView using incorrect widths for header columns by re-setting the width. (dmex)
            // Note: This resets the width a second time after the header control has already set the width.

            ExtendedListView_SetRedraw(WindowHandle, FALSE);

            while (TRUE)
            {
                if (context->ListViewClass)
                {
                    if (SUCCEEDED(IListView_GetColumn(context->ListViewClass, i, &lvColumn)))
                    {
                        lvColumn.cx = PhMultiplyDivideSigned(lvColumn.cx, listviewDpi, context->WindowDpi);

                        IListView_SetColumn(context->ListViewClass, i, &lvColumn);
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    if (CallWindowProc(oldWndProc, WindowHandle, LVM_GETCOLUMN, i, (LPARAM)&lvColumn))
                    {
                        lvColumn.cx = PhMultiplyDivideSigned(lvColumn.cx, listviewDpi, context->WindowDpi);

                        CallWindowProc(oldWndProc, WindowHandle, LVM_SETCOLUMN, i, (WPARAM)&lvColumn);
                    }
                    else
                    {
                        break;
                    }
                }

                i++;
            }

            ExtendedListView_SetRedraw(WindowHandle, TRUE);

            context->WindowDpi = listviewDpi;
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Visually indicates the sort order of a header control item.
 *
 * \param WindowHandle A handle to the header control.
 * \param Index The index of the item.
 * \param Order The sort order of the item.
 */
VOID PhSetHeaderSortIcon(
    _In_ HWND WindowHandle,
    _In_ LONG Index,
    _In_ PH_SORT_ORDER Order
    )
{
    LONG count;
    LONG i;

    count = Header_GetItemCount(WindowHandle);

    if (count == INT_ERROR)
        return;

    for (i = 0; i < count; i++)
    {
        HDITEM item;

        item.mask = HDI_FORMAT;
        Header_GetItem(WindowHandle, i, &item);

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

        Header_SetItem(WindowHandle, i, &item);
    }
}

INT PhpExtendedListViewCompareFunc(
    _In_ LPARAM lParam1,
    _In_ LPARAM lParam2,
    _In_ LPARAM lParamSort
    )
{
    PPH_EXTLV_CONTEXT context = (PPH_EXTLV_CONTEXT)lParamSort;
    LONG result;
    LONG x = (LONG)lParam1;
    LONG y = (LONG)lParam2;
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

    if (context->ListViewClass)
    {
        if (FAILED(IListView_GetItem(context->ListViewClass, &xItem)))
            return 0;
        if (FAILED(IListView_GetItem(context->ListViewClass, &yItem)))
            return 0;
    }
    else
    {
        if (!CallWindowProc(context->OldWndProc, context->Handle, LVM_GETITEM, 0, (LPARAM)&xItem))
            return 0;
        if (!CallWindowProc(context->OldWndProc, context->Handle, LVM_GETITEM, 0, (LPARAM)&yItem))
            return 0;
    }

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

INT PhpExtendedListViewCompareFastFunc(
    _In_ LPARAM lParam1,
    _In_ LPARAM lParam2,
    _In_ LPARAM lParamSort
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

FORCEINLINE INT PhpCompareListViewItems(
    _In_ PPH_EXTLV_CONTEXT Context,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ PVOID XParam,
    _In_ PVOID YParam,
    _In_ ULONG Column,
    _In_ BOOLEAN EnableDefault
    )
{
    if (
        Column < PH_MAX_COMPARE_FUNCTIONS &&
        Context->CompareFunctions[Column]
        )
    {
        INT result;

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

INT PhpDefaultCompareListViewItems(
    _In_ PPH_EXTLV_CONTEXT Context,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ ULONG Column
    )
{
    WCHAR xText[MAX_PATH + 1];
    WCHAR yText[MAX_PATH + 1];
    LVITEM item;

    // Get the X item text.

    item.mask = LVIF_TEXT;
    item.iItem = X;
    item.iSubItem = Column;
    item.pszText = xText;
    item.cchTextMax = MAX_PATH;

    xText[0] = UNICODE_NULL;

    if (Context->ListViewClass)
        IListView_GetItem(Context->ListViewClass, &item);
    else
        CallWindowProc(Context->OldWndProc, Context->Handle, LVM_GETITEM, 0, (LPARAM)&item);

    // Get the Y item text.

    item.iItem = Y;
    item.pszText = yText;
    item.cchTextMax = MAX_PATH;

    yText[0] = UNICODE_NULL;

    if (Context->ListViewClass)
        IListView_GetItem(Context->ListViewClass, &item);
    else
        CallWindowProc(Context->OldWndProc, Context->Handle, LVM_GETITEM, 0, (LPARAM)&item);

    // Compare them.

#if 1
    return PhCompareStringZNatural(xText, yText, TRUE);
#else
    return _wcsicmp(xText, yText);
#endif
}

HWND PhGetExtendedListViewHeader(
    _In_ PPH_EXTLV_CONTEXT Context
    )
{
    if (Context->ListViewClass)
    {
        HWND headerHandle;

        if (HR_SUCCESS(IListView_GetHeaderControl(Context->ListViewClass, &headerHandle)))
        {
            return headerHandle;
        }
    }

    return ListView_GetHeader(Context->Handle);
}

HWND PhGetExtendedListViewTooltips(
    _In_ PPH_EXTLV_CONTEXT Context
    )
{
    if (Context->ListViewClass)
    {
        HWND tooltipHandle;

        if (HR_SUCCESS(IListView_GetToolTip(Context->ListViewClass, &tooltipHandle)))
        {
            return tooltipHandle;
        }
    }

    return ListView_GetToolTips(Context->Handle);
}
