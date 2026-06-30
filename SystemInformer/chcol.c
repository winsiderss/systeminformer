/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2017-2026
 *
 */

#include <phapp.h>
#include <settings.h>

typedef struct _COLUMNS_DIALOG_CONTEXT
{
    HWND ControlHandle;
    HFONT ControlFont;
    ULONG Type;
    PPH_LIST Columns;

    HBRUSH BrushNormal;
    HBRUSH BrushPushed;
    HBRUSH BrushHot;
    HBRUSH BrushHover;
    COLORREF TextColor;

    WNDPROC InactiveListWindowProc;
    WNDPROC ActiveListWindowProc;
    HWND HotListHandle;
    LONG HotListIndex;
    BOOLEAN MouseTracking;

    HWND InactiveWindowHandle;
    HWND ActiveWindowHandle;
    HWND SearchInactiveHandle;
    HWND SearchActiveHandle;
    HWND HideWindowHandle;
    HWND ShowWindowHandle;
    HWND MoveUpHandle;
    HWND MoveDownHandle;
    PPH_LIST InactiveListArray;
    PPH_LIST ActiveListArray;

    UINT DragListMessage;
    HWND DragSourceHandle;
    LONG DragItemIndex;
} COLUMNS_DIALOG_CONTEXT, *PCOLUMNS_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpColumnsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

/**
 * Shows the column chooser dialog for a control.
 *
 * \param ParentWindowHandle Parent window for the dialog.
 * \param ControlHandle The handle to the control.
 * \param Type The type of the control.
 */
VOID PhShowChooseColumnsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HWND ControlHandle,
    _In_ ULONG Type
    )
{
    COLUMNS_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(COLUMNS_DIALOG_CONTEXT));
    context.ControlHandle = ControlHandle;
    context.Type = Type;

    if (Type == PH_CONTROL_TYPE_TREE_NEW)
        context.Columns = PhCreateList(TreeNew_GetColumnCount(ControlHandle));
    else
        return;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CHOOSECOLUMNS),
        ParentWindowHandle,
        PhpColumnsDlgProc,
        &context
        );

    PhDereferenceObject(context.Columns);
}

/**
 * Compares the display index of two columns for sorting.
 *
 * \param Context Optional callback context.
 * \param elem1 First column pointer.
 * \param elem2 Second column pointer.
 * \return Comparison result.
 */
static int __cdecl PhpColumnsCompareDisplayIndexTn(
    _In_ void* Context,
    _In_ void const* elem1,
    _In_ void const* elem2
    )
{
    PPH_TREENEW_COLUMN column1 = *(PPH_TREENEW_COLUMN *)elem1;
    PPH_TREENEW_COLUMN column2 = *(PPH_TREENEW_COLUMN *)elem2;

    return uintcmp(column1->DisplayIndex, column2->DisplayIndex);
}

/**
 * Compares the names of two inactive columns alphabetically.
 *
 * \param Context Optional callback context.
 * \param elem1 First string pointer.
 * \param elem2 Second string pointer.
 * \return Comparison result.
 */
static long __cdecl PhpInactiveColumnsCompareNameTn(
    _In_ const void* Context,
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PCWSTR column1 = *(PCWSTR *)elem1;
    PCWSTR column2 = *(PCWSTR *)elem2;

    return PhCompareStringZ(column1, column2, FALSE);
}

/**
 * Finds the index of a string in a list.
 *
 * \param List The list to search.
 * \param String The string to find.
 * \return The index of the string, or ULONG_MAX if not found.
 */
_Success_(return != ULONG_MAX)
static ULONG IndexOfStringInList(
    _In_ PPH_LIST List,
    _In_ PCWSTR String
    )
{
    for (ULONG i = 0; i < List->Count; i++)
    {
        if (PhEqualStringZ(List->Items[i], String, FALSE))
            return i;
    }

    return ULONG_MAX;
}

/**
 * Updates the hot (mouse-over) item, invalidating the previous and new items.
 *
 * \param Context The columns dialog context.
 * \param ListBoxHandle The list box under the mouse, or NULL for none.
 * \param ItemIndex The hot item index, or LB_ERR for none.
 */
static VOID PhpColumnsSetHotItem(
    _In_ PCOLUMNS_DIALOG_CONTEXT Context,
    _In_opt_ HWND ListBoxHandle,
    _In_ LONG ItemIndex
    )
{
    RECT itemRect;

    if (Context->HotListHandle == ListBoxHandle && Context->HotListIndex == ItemIndex)
        return;

    if (Context->HotListHandle && Context->HotListIndex != LB_ERR)
    {
        if (ListBox_GetItemRect(Context->HotListHandle, Context->HotListIndex, &itemRect) != LB_ERR)
            InvalidateRect(Context->HotListHandle, &itemRect, FALSE);
    }

    Context->HotListHandle = ListBoxHandle;
    Context->HotListIndex = ItemIndex;

    if (ListBoxHandle && ItemIndex != LB_ERR)
    {
        if (ListBox_GetItemRect(ListBoxHandle, ItemIndex, &itemRect) != LB_ERR)
            InvalidateRect(ListBoxHandle, &itemRect, FALSE);
    }
}

/**
 * Resets a list box and populates it with items, optionally sorting and filtering them.
 *
 * \param ListBoxHandle The handle to the list box control.
 * \param MatchHandle The search control match handle for filtering.
 * \param Array The list containing the items to display.
 * \param CompareFunction Optional comparison function for sorting.
 */
VOID PhpColumnsResetListBox(
    _In_ HWND ListBoxHandle,
    _In_ ULONG_PTR MatchHandle,
    _In_ PPH_LIST Array,
    _In_opt_ PVOID CompareFunction
    )
{
    SendMessage(ListBoxHandle, WM_SETREDRAW, FALSE, 0);

    ListBox_ResetContent(ListBoxHandle);

    if (CompareFunction)
        qsort_s(Array->Items, Array->Count, sizeof(ULONG_PTR), CompareFunction, NULL);

    if (!MatchHandle)
    {
        for (ULONG i = 0; i < Array->Count; i++)
        {
            ListBox_InsertString(ListBoxHandle, i, Array->Items[i]);
        }
    }
    else
    {
        ULONG index = 0;

        for (ULONG i = 0; i < Array->Count; i++)
        {
            PH_STRINGREF text;

            PhInitializeStringRefLongHint(&text, Array->Items[i]);

            if (PhSearchControlMatch(MatchHandle, &text))
            {
                ListBox_InsertString(ListBoxHandle, index, Array->Items[i]);
                index++;
            }
        }
    }

    SendMessage(ListBoxHandle, WM_SETREDRAW, TRUE, 0);
}

// Moves the item at listbox row FromIndex to insert position ToIndex (the item is
// inserted before the item currently at ToIndex; ToIndex == count appends).
/**
 * Moves an item in the active columns list from one position to another.
 *
 * \param Context The columns dialog context.
 * \param FromIndex The index of the item to move.
 * \param ToIndex The destination index.
 */
static VOID PhpColumnsMoveActiveItem(
    _In_ PCOLUMNS_DIALOG_CONTEXT Context,
    _In_ LONG FromIndex,
    _In_ LONG ToIndex
    )
{
    LONG count;
    LONG target;
    PPH_STRING string;
    ULONG arrayFrom;
    ULONG arrayTo;
    PVOID item;

    count = ListBox_GetCount(Context->ActiveWindowHandle);

    if (FromIndex == LB_ERR || FromIndex >= count || ToIndex < 0 || ToIndex > count)
        return;

    PhpColumnsSetHotItem(Context, NULL, LB_ERR);
    if (ToIndex == FromIndex || ToIndex == FromIndex + 1) // no-op move
        return;

    string = PhGetListBoxString(Context->ActiveWindowHandle, FromIndex);

    if (PhIsNullOrEmptyString(string))
    {
        PhClearReference(&string);
        return;
    }

    arrayFrom = IndexOfStringInList(Context->ActiveListArray, string->Buffer);

    if (arrayFrom != ULONG_MAX)
    {
        // The listbox may be filtered, so translate the insert row to an array
        // position through the string of the item currently at that row.
        if (ToIndex >= count)
        {
            arrayTo = Context->ActiveListArray->Count;
        }
        else
        {
            PPH_STRING targetString = PhGetListBoxString(Context->ActiveWindowHandle, ToIndex);

            arrayTo = targetString ? IndexOfStringInList(Context->ActiveListArray, targetString->Buffer) : ULONG_MAX;
            PhClearReference(&targetString);

            if (arrayTo == ULONG_MAX)
                arrayTo = Context->ActiveListArray->Count;
        }

        item = Context->ActiveListArray->Items[arrayFrom];
        PhRemoveItemsList(Context->ActiveListArray, arrayFrom, 1);

        if (arrayTo > arrayFrom)
            arrayTo--;

        PhInsertItemList(Context->ActiveListArray, arrayTo, item);

        target = ToIndex;

        if (target > FromIndex)
            target--;

        ListBox_DeleteString(Context->ActiveWindowHandle, FromIndex);
        ListBox_InsertString(Context->ActiveWindowHandle, target, item);
        ListBox_SetCurSel(Context->ActiveWindowHandle, target);

        EnableWindow(Context->MoveUpHandle, target != 0);
        EnableWindow(Context->MoveDownHandle, target != count - 1);
    }

    PhClearReference(&string);
}

// Moves the item at inactive listbox row InactiveIndex into the active list at
// insert position ActiveInsertIndex (INT_ERROR or count appends).
/**
 * Moves a column from the inactive list to the active list.
 *
 * \param Context The columns dialog context.
 * \param InactiveIndex The index of the item in the inactive list.
 * \param ActiveInsertIndex The insert index in the active list.
 */
static VOID PhpColumnsShowItem(
    _In_ PCOLUMNS_DIALOG_CONTEXT Context,
    _In_ LONG InactiveIndex,
    _In_ LONG ActiveInsertIndex
    )
{
    LONG count;
    PPH_STRING string;

    count = ListBox_GetCount(Context->InactiveWindowHandle);

    if (InactiveIndex == LB_ERR || InactiveIndex >= count)
        return;

    PhpColumnsSetHotItem(Context, NULL, LB_ERR);

    string = PhGetListBoxString(Context->InactiveWindowHandle, InactiveIndex);

    if (!PhIsNullOrEmptyString(string))
    {
        ULONG index = IndexOfStringInList(Context->InactiveListArray, string->Buffer);

        if (index != ULONG_MAX)
        {
            PVOID item = Context->InactiveListArray->Items[index];
            LONG activeCount = ListBox_GetCount(Context->ActiveWindowHandle);
            ULONG arrayTo;

            PhRemoveItemsList(Context->InactiveListArray, index, 1);

            if (ActiveInsertIndex < 0 || ActiveInsertIndex >= activeCount)
            {
                arrayTo = Context->ActiveListArray->Count;
                ActiveInsertIndex = activeCount;
            }
            else
            {
                PPH_STRING targetString = PhGetListBoxString(Context->ActiveWindowHandle, ActiveInsertIndex);

                arrayTo = targetString ? IndexOfStringInList(Context->ActiveListArray, targetString->Buffer) : ULONG_MAX;
                PhClearReference(&targetString);

                if (arrayTo == ULONG_MAX)
                    arrayTo = Context->ActiveListArray->Count;
            }

            PhInsertItemList(Context->ActiveListArray, arrayTo, item);

            ListBox_DeleteString(Context->InactiveWindowHandle, InactiveIndex);
            ListBox_InsertString(Context->ActiveWindowHandle, ActiveInsertIndex, item);
        }

        count--;

        if (InactiveIndex >= count - 1)
            InactiveIndex = count - 1;

        if (InactiveIndex != LB_ERR)
        {
            ListBox_SetCurSel(Context->InactiveWindowHandle, InactiveIndex);
        }
    }

    PhClearReference(&string);
}

// Moves the item at active listbox row ActiveIndex back into the (sorted) inactive list.
/**
 * Moves a column from the active list to the inactive list.
 *
 * \param Context The columns dialog context.
 * \param ActiveIndex The index of the item in the active list.
 */
static VOID PhpColumnsHideItem(
    _In_ PCOLUMNS_DIALOG_CONTEXT Context,
    _In_ LONG ActiveIndex
    )
{
    LONG count;
    PPH_STRING string;

    count = ListBox_GetCount(Context->ActiveWindowHandle);

    if (ActiveIndex == LB_ERR || ActiveIndex >= count)
        return;

    PhpColumnsSetHotItem(Context, NULL, LB_ERR);

    string = PhGetListBoxString(Context->ActiveWindowHandle, ActiveIndex);

    if (!PhIsNullOrEmptyString(string))
    {
        ULONG index = IndexOfStringInList(Context->ActiveListArray, string->Buffer);

        if (index != ULONG_MAX)
        {
            PVOID item = Context->ActiveListArray->Items[index];

            // Remove from active array, insert into inactive
            PhRemoveItemsList(Context->ActiveListArray, index, 1);
            PhAddItemList(Context->InactiveListArray, item);

            // Delete from active list
            ListBox_DeleteString(Context->ActiveWindowHandle, ActiveIndex);
            // Sort the inactive list with the new entry and refresh
            PhpColumnsResetListBox(Context->InactiveWindowHandle, 0, Context->InactiveListArray, PhpInactiveColumnsCompareNameTn);
        }

        count--;

        if (ActiveIndex >= count - 1)
            ActiveIndex = count - 1;

        if (ActiveIndex != LB_ERR)
        {
            ListBox_SetCurSel(Context->ActiveWindowHandle, ActiveIndex);
        }
    }

    PhClearReference(&string);
}

// Returns the insert row under the cursor for a drag list target, count when the
// cursor is inside the listbox but below the last item, or INT_ERROR when outside.
/**
 * Finds the drag insert index under the cursor for a list box.
 *
 * \param ListBoxHandle The handle to the list box.
 * \param Point The current cursor coordinates.
 * \param AutoScroll Whether to auto-scroll the list box.
 * \return The insert index, or INT_ERROR if the cursor is outside.
 */
static LONG PhpColumnsDragTargetIndex(
    _In_ HWND ListBoxHandle,
    _In_ POINT Point,
    _In_ BOOLEAN AutoScroll
    )
{
    LONG index;
    RECT rect;

    index = LBItemFromPt(ListBoxHandle, Point, AutoScroll);

    if (index == INT_ERROR)
    {
        GetWindowRect(ListBoxHandle, &rect);

        if (PtInRect(&rect, Point))
            index = ListBox_GetCount(ListBoxHandle);
    }

    return index;
}

/**
 * Callback for the inactive columns search control.
 *
 * \param MatchHandle The search control match handle.
 * \param Context The columns dialog context.
 */
_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PhpInactiveColumnsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PCOLUMNS_DIALOG_CONTEXT context = Context;

    PhpColumnsSetHotItem(context, NULL, LB_ERR);

    PhpColumnsResetListBox(
        context->InactiveWindowHandle,
        MatchHandle,
        context->InactiveListArray,
        PhpInactiveColumnsCompareNameTn
        );
}

/**
 * Callback for the active columns search control.
 *
 * \param MatchHandle The search control match handle.
 * \param Context The columns dialog context.
 */
_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PhpActiveColumnsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PCOLUMNS_DIALOG_CONTEXT context = Context;

    PhpColumnsSetHotItem(context, NULL, LB_ERR);

    PhpColumnsResetListBox(
        context->ActiveWindowHandle,
        MatchHandle,
        context->ActiveListArray,
        NULL
        );
}

/**
 * Window procedure for the column list boxes providing hot (mouse-over) tracking.
 *
 * \param WindowHandle The handle to the list box window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \return LRESULT The message result.
 */
static LRESULT CALLBACK PhpColumnsListBoxWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCOLUMNS_DIALOG_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT)))
        return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);

    if (WindowHandle == context->ActiveWindowHandle)
        oldWndProc = context->ActiveListWindowProc;
    else
        oldWndProc = context->InactiveListWindowProc;

    switch (WindowMessage)
    {
    case WM_NCDESTROY:
        {
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_MOUSEMOVE:
        {
            LRESULT result = SendMessage(WindowHandle, LB_ITEMFROMPOINT, 0, lParam);
            LONG index = HIWORD(result) == 0 ? (LONG)(SHORT)LOWORD(result) : LB_ERR;

            PhpColumnsSetHotItem(context, WindowHandle, index);

            if (!context->MouseTracking)
            {
                TRACKMOUSEEVENT trackMouseEvent = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, WindowHandle, 0 };

                context->MouseTracking = !!TrackMouseEvent(&trackMouseEvent);
            }
        }
        break;
    case WM_MOUSELEAVE:
        {
            context->MouseTracking = FALSE;
            PhpColumnsSetHotItem(context, NULL, LB_ERR);
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * The dialog procedure for the column chooser dialog.
 *
 * \param hwndDlg The handle to the dialog window.
 * \param uMsg The window message.
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \return INT_PTR Dialog return value.
 */
INT_PTR CALLBACK PhpColumnsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCOLUMNS_DIALOG_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PCOLUMNS_DIALOG_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ULONG count;
            ULONG total;
            ULONG i;
            PPH_LIST displayOrderList = NULL;
            LONG dpiValue;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            PhSetApplicationWindowIcon(hwndDlg);

            dpiValue = PhGetWindowDpi(hwndDlg);

            context->InactiveWindowHandle = GetDlgItem(hwndDlg, IDC_INACTIVE);
            context->ActiveWindowHandle = GetDlgItem(hwndDlg, IDC_ACTIVE);
            context->SearchInactiveHandle = GetDlgItem(hwndDlg, IDC_SEARCH);
            context->SearchActiveHandle = GetDlgItem(hwndDlg, IDC_FILTER);
            context->HideWindowHandle = GetDlgItem(hwndDlg, IDC_HIDE);
            context->ShowWindowHandle = GetDlgItem(hwndDlg, IDC_SHOW);
            context->MoveUpHandle = GetDlgItem(hwndDlg, IDC_MOVEUP);
            context->MoveDownHandle = GetDlgItem(hwndDlg, IDC_MOVEDOWN);
            context->InactiveListArray = PhCreateList(1);
            context->ActiveListArray = PhCreateList(1);
            context->ControlFont = PhCreateMessageFont(dpiValue); // PhDuplicateFontUpdateDpi(PhTreeWindowFont, PhGetWindowDpi(hwndDlg))

            PhCreateSearchControl(
                hwndDlg,
                context->SearchInactiveHandle,
                L"Inactive columns...",
                PhpInactiveColumnsSearchControlCallback,
                context
                );

            PhCreateSearchControl(
                hwndDlg,
                context->SearchActiveHandle,
                L"Active columns...",
                PhpActiveColumnsSearchControlCallback,
                context
                );

            ListBox_SetItemHeight(context->InactiveWindowHandle, 0, PhScaleToDisplay(16, dpiValue));
            ListBox_SetItemHeight(context->ActiveWindowHandle, 0, PhScaleToDisplay(16, dpiValue));

            context->DragListMessage = RegisterWindowMessage(DRAGLISTMSGSTRING);
            context->DragItemIndex = LB_ERR;
            MakeDragList(context->InactiveWindowHandle);
            MakeDragList(context->ActiveWindowHandle);

            // Subclass the list boxes (after MakeDragList) for hot tracking.
            context->HotListIndex = LB_ERR;
            PhSetWindowContext(context->InactiveWindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
            PhSetWindowContext(context->ActiveWindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
            context->InactiveListWindowProc = (WNDPROC)SetWindowLongPtr(context->InactiveWindowHandle, GWLP_WNDPROC, (LONG_PTR)PhpColumnsListBoxWndProc);
            context->ActiveListWindowProc = (WNDPROC)SetWindowLongPtr(context->ActiveWindowHandle, GWLP_WNDPROC, (LONG_PTR)PhpColumnsListBoxWndProc);

            Button_Enable(context->HideWindowHandle, FALSE);
            Button_Enable(context->ShowWindowHandle, FALSE);
            Button_Enable(context->MoveUpHandle, FALSE);
            Button_Enable(context->MoveDownHandle, FALSE);

            if (PhEnableThemeSupport)
            {
                context->BrushNormal = CreateSolidBrush(PhThemeWindowBackgroundColor);
                context->BrushHot = CreateSolidBrush(PhThemeWindowHighlightColor);
                context->BrushPushed = CreateSolidBrush(PhThemeWindowHighlight2Color);
                context->BrushHover = CreateSolidBrush(PhThemeWindowBackground2Color);
                context->TextColor = PhThemeWindowTextColor;
            }
            else
            {
                context->BrushNormal = GetSysColorBrush(COLOR_WINDOW);
                context->BrushHot = CreateSolidBrush(RGB(145, 201, 247));
                context->BrushPushed = CreateSolidBrush(RGB(153, 209, 255));
                context->BrushHover = CreateSolidBrush(RGB(229, 243, 255));
                context->TextColor = GetSysColor(COLOR_WINDOWTEXT);
            }

            if (context->Type == PH_CONTROL_TYPE_TREE_NEW)
            {
                PH_TREENEW_COLUMN column;

                count = 0;
                total = TreeNew_GetColumnCount(context->ControlHandle);
                i = 0;

                displayOrderList = PhCreateList(total);

                while (count < total)
                {
                    if (TreeNew_GetColumn(context->ControlHandle, i, &column))
                    {
                        PPH_TREENEW_COLUMN copy;

                        if (column.Fixed)
                        {
                            i++;
                            total--;
                            continue;
                        }

                        copy = PhAllocateCopy(&column, sizeof(PH_TREENEW_COLUMN));
                        PhAddItemList(context->Columns, copy);
                        count++;

                        if (column.Visible)
                        {
                            PhAddItemList(displayOrderList, copy);
                        }
                        else
                        {
                            PhAddItemList(context->InactiveListArray, (PWSTR)column.Text);
                        }
                    }

                    i++;
                }

                qsort_s(displayOrderList->Items, displayOrderList->Count, sizeof(PVOID), PhpColumnsCompareDisplayIndexTn, NULL);
            }

            PhpColumnsResetListBox(
                context->InactiveWindowHandle,
                0,
                context->InactiveListArray,
                PhpInactiveColumnsCompareNameTn
                );

            if (displayOrderList)
            {
                for (i = 0; i < displayOrderList->Count; i++)
                {
                    if (context->Type == PH_CONTROL_TYPE_TREE_NEW)
                    {
                        PPH_TREENEW_COLUMN copy = displayOrderList->Items[i];

                        PhAddItemList(context->ActiveListArray, (PWSTR)copy->Text);
                        ListBox_InsertString(context->ActiveWindowHandle, i, copy->Text);
                    }
                }

                PhDereferenceObject(displayOrderList);
            }

            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INACTIVE, LBN_SELCHANGE), (LPARAM)context->InactiveWindowHandle);
            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ACTIVE, LBN_SELCHANGE), (LPARAM)context->ActiveWindowHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDCANCEL));
        }
        break;
    case WM_DESTROY:
        {
            for (ULONG i = 0; i < context->Columns->Count; i++)
                PhFree(context->Columns->Items[i]);

            if (context->BrushNormal)
                DeleteBrush(context->BrushNormal);
            if (context->BrushHot)
                DeleteBrush(context->BrushHot);
            if (context->BrushPushed)
                DeleteBrush(context->BrushPushed);
            if (context->BrushHover)
                DeleteBrush(context->BrushHover);
            if (context->ControlFont)
                DeleteFont(context->ControlFont);
            if (context->InactiveListArray)
                PhDereferenceObject(context->InactiveListArray);
            if (context->ActiveListArray)
                PhDereferenceObject(context->ActiveListArray);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_DPICHANGED:
        {
            if (context->ControlFont) DeleteFont(context->ControlFont);
            context->ControlFont = PhCreateMessageFont(LOWORD(wParam));

            ListBox_SetItemHeight(context->InactiveWindowHandle, 0, PhScaleToDisplay(16, LOWORD(wParam)));
            ListBox_SetItemHeight(context->ActiveWindowHandle, 0, PhScaleToDisplay(16, LOWORD(wParam)));
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    ULONG i;
                    ULONG orderArraySize;
                    PULONG orderArray;
                    ULONG maxOrder;

                    if (context->Type == PH_CONTROL_TYPE_TREE_NEW)
                    {
                        orderArraySize = (TreeNew_GetColumnCount(context->ControlHandle) + 1) * sizeof(ULONG);
                        orderArray = PhAllocateStack(orderArraySize);

                        memset(orderArray, 0, orderArraySize);
                        maxOrder = 0;

                        // Apply visibility settings and build the order array.

                        TreeNew_SetRedraw(context->ControlHandle, FALSE);

                        for (i = 0; i < context->Columns->Count; i++)
                        {
                            PPH_TREENEW_COLUMN column = context->Columns->Items[i];
                            ULONG index;

                            index = IndexOfStringInList(context->ActiveListArray, column->Text);
                            column->Visible = index != ULONG_MAX;

                            TreeNew_SetColumn(context->ControlHandle, TN_COLUMN_FLAG_VISIBLE, column);

                            if (column->Visible)
                            {
                                orderArray[index] = column->Id;

                                if (maxOrder < index + 1)
                                    maxOrder = index + 1;
                            }
                        }

                        // Apply display order.
                        TreeNew_SetColumnOrderArray(context->ControlHandle, maxOrder, orderArray);

                        TreeNew_SetRedraw(context->ControlHandle, TRUE);

                        InvalidateRect(context->ControlHandle, NULL, FALSE);

                        PhFreeStack(orderArray);
                    }

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            case IDC_INACTIVE:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                    case LBN_DBLCLK:
                        {
                            SendMessage(hwndDlg, WM_COMMAND, IDC_SHOW, 0);
                        }
                        break;
                    case LBN_SELCHANGE:
                        {
                            LONG sel = ListBox_GetCurSel(context->InactiveWindowHandle);

                            EnableWindow(context->ShowWindowHandle, sel != LB_ERR);
                        }
                        break;
                    }
                }
                break;
            case IDC_ACTIVE:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                    case LBN_DBLCLK:
                        {
                            SendMessage(hwndDlg, WM_COMMAND, IDC_HIDE, 0);
                        }
                        break;
                    case LBN_SELCHANGE:
                        {
                            LONG sel = ListBox_GetCurSel(context->ActiveWindowHandle);
                            LONG count = ListBox_GetCount(context->ActiveWindowHandle);

                            if (sel != LB_ERR)
                            {
                                EnableWindow(context->HideWindowHandle, sel != 0);
                                EnableWindow(context->MoveUpHandle, sel != 0);
                                EnableWindow(context->MoveDownHandle, sel != count - 1);
                            }
                        }
                        break;
                    }
                }
                break;
            case IDC_SHOW:
                {
                    PhpColumnsShowItem(context, ListBox_GetCurSel(context->InactiveWindowHandle), INT_ERROR);

                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INACTIVE, LBN_SELCHANGE), (LPARAM)context->InactiveWindowHandle);
                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ACTIVE, LBN_SELCHANGE), (LPARAM)context->ActiveWindowHandle);
                }
                break;
            case IDC_HIDE:
                {
                    PhpColumnsHideItem(context, ListBox_GetCurSel(context->ActiveWindowHandle));

                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INACTIVE, LBN_SELCHANGE), (LPARAM)context->InactiveWindowHandle);
                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ACTIVE, LBN_SELCHANGE), (LPARAM)context->ActiveWindowHandle);
                }
                break;
            case IDC_MOVEUP:
                {
                    LONG sel = ListBox_GetCurSel(context->ActiveWindowHandle);

                    if (sel != LB_ERR)
                        PhpColumnsMoveActiveItem(context, sel, sel - 1);
                }
                break;
            case IDC_MOVEDOWN:
                {
                    LONG sel = ListBox_GetCurSel(context->ActiveWindowHandle);

                    if (sel != LB_ERR)
                        PhpColumnsMoveActiveItem(context, sel, sel + 2);
                }
                break;
            }
        }
        break;
     case WM_DRAWITEM:
         {
             LPDRAWITEMSTRUCT drawInfo = (LPDRAWITEMSTRUCT)lParam;

             if (
                 drawInfo->hwndItem == context->InactiveWindowHandle ||
                 drawInfo->hwndItem == context->ActiveWindowHandle
                 )
             {
                 HDC bufferDc;
                 HBITMAP bufferBitmap;
                 HBITMAP oldBufferBitmap;
                 PPH_STRING string;
                 RECT bufferRect =
                 {
                     0, 0,
                     drawInfo->rcItem.right - drawInfo->rcItem.left,
                     drawInfo->rcItem.bottom - drawInfo->rcItem.top
                 };
                 BOOLEAN isSelected = (drawInfo->itemState & ODS_SELECTED) == ODS_SELECTED;
                 BOOLEAN isFocused = (drawInfo->itemState & ODS_FOCUS) == ODS_FOCUS;
                 BOOLEAN isHot =
                     drawInfo->hwndItem == context->HotListHandle &&
                     (LONG)drawInfo->itemID == context->HotListIndex;

                 if (drawInfo->itemID == LB_ERR)
                     break;

                 string = PhGetListBoxString(drawInfo->hwndItem, drawInfo->itemID);

                 if (PhIsNullOrEmptyString(string))
                 {
                     PhClearReference(&string);
                     break;
                 }

                 bufferDc = CreateCompatibleDC(drawInfo->hDC);
                 bufferBitmap = CreateCompatibleBitmap(drawInfo->hDC, bufferRect.right, bufferRect.bottom);
                 oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

                 SelectFont(bufferDc, context->ControlFont);
                 SetBkMode(bufferDc, TRANSPARENT);

                 if (isSelected || isFocused)
                 {
                     FillRect(bufferDc, &bufferRect, context->BrushHot);
                     //FrameRect(bufferDc, &bufferRect, PhGetStockBrush(BLACK_BRUSH));
                     SetTextColor(bufferDc, context->TextColor);
                 }
                 else if (isHot)
                 {
                     FillRect(bufferDc, &bufferRect, context->BrushHover);
                     SetTextColor(bufferDc, context->TextColor);
                 }
                 else
                 {
                     FillRect(bufferDc, &bufferRect, context->BrushNormal);
                     //FrameRect(bufferDc, &bufferRect, GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
                     SetTextColor(bufferDc, context->TextColor);
                 }

                 bufferRect.left += 5;
                 DrawText(
                     bufferDc,
                     string->Buffer,
                     (UINT)string->Length / sizeof(WCHAR),
                     &bufferRect,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP
                     );
                 bufferRect.left -= 5;

                 BitBlt(
                     drawInfo->hDC,
                     drawInfo->rcItem.left,
                     drawInfo->rcItem.top,
                     drawInfo->rcItem.right,
                     drawInfo->rcItem.bottom,
                     bufferDc,
                     0,
                     0,
                     SRCCOPY
                     );

                 SelectBitmap(bufferDc, oldBufferBitmap);
                 DeleteBitmap(bufferBitmap);
                 DeleteDC(bufferDc);
                 PhClearReference(&string);

                 return TRUE;
             }
         }
         break;
     case WM_CTLCOLORBTN:
         return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
     case WM_CTLCOLORDLG:
         return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
     case WM_CTLCOLORSTATIC:
         return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    if (context->DragListMessage && uMsg == context->DragListMessage)
    {
        LPDRAGLISTINFO dragInfo = (LPDRAGLISTINFO)lParam;
        LRESULT result = 0;

        switch (dragInfo->uNotification)
        {
        case DL_BEGINDRAG:
            {
                LONG index = LBItemFromPt(dragInfo->hWnd, dragInfo->ptCursor, FALSE);

                if (index != INT_ERROR)
                {
                    context->DragSourceHandle = dragInfo->hWnd;
                    context->DragItemIndex = index;
                    ListBox_SetCurSel(dragInfo->hWnd, index);
                    result = TRUE;
                }
            }
            break;
        case DL_DRAGGING:
            {
                LONG activeIndex = PhpColumnsDragTargetIndex(context->ActiveWindowHandle, dragInfo->ptCursor, TRUE);
                LONG inactiveIndex = PhpColumnsDragTargetIndex(context->InactiveWindowHandle, dragInfo->ptCursor, FALSE);

                if (activeIndex != INT_ERROR)
                {
                    DrawInsert(hwndDlg, context->ActiveWindowHandle, activeIndex);
                    result = DL_MOVECURSOR;
                }
                else if (inactiveIndex != INT_ERROR && context->DragSourceHandle == context->ActiveWindowHandle)
                {
                    // The inactive list is sorted, so there's no insert position to show.
                    DrawInsert(hwndDlg, context->ActiveWindowHandle, INT_ERROR);
                    result = DL_MOVECURSOR;
                }
                else
                {
                    DrawInsert(hwndDlg, context->ActiveWindowHandle, INT_ERROR);
                    result = DL_STOPCURSOR;
                }
            }
            break;
        case DL_DROPPED:
            {
                LONG activeIndex = PhpColumnsDragTargetIndex(context->ActiveWindowHandle, dragInfo->ptCursor, FALSE);
                LONG inactiveIndex = PhpColumnsDragTargetIndex(context->InactiveWindowHandle, dragInfo->ptCursor, FALSE);

                DrawInsert(hwndDlg, context->ActiveWindowHandle, INT_ERROR);

                if (context->DragItemIndex != LB_ERR)
                {
                    if (context->DragSourceHandle == context->ActiveWindowHandle)
                    {
                        if (activeIndex != INT_ERROR)
                            PhpColumnsMoveActiveItem(context, context->DragItemIndex, activeIndex);
                        else if (inactiveIndex != INT_ERROR)
                            PhpColumnsHideItem(context, context->DragItemIndex);
                    }
                    else if (context->DragSourceHandle == context->InactiveWindowHandle)
                    {
                        if (activeIndex != INT_ERROR)
                            PhpColumnsShowItem(context, context->DragItemIndex, activeIndex);
                    }

                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INACTIVE, LBN_SELCHANGE), (LPARAM)context->InactiveWindowHandle);
                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ACTIVE, LBN_SELCHANGE), (LPARAM)context->ActiveWindowHandle);
                }

                context->DragSourceHandle = NULL;
                context->DragItemIndex = LB_ERR;
            }
            break;
        case DL_CANCELDRAG:
            {
                DrawInsert(hwndDlg, context->ActiveWindowHandle, INT_ERROR);
                context->DragSourceHandle = NULL;
                context->DragItemIndex = LB_ERR;
            }
            break;
        }

        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, result);
        return TRUE;
    }

    return FALSE;
}
