/*
 * Process Hacker - 
 *   column chooser
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

#include <phapp.h>
#include <settings.h>
#include <windowsx.h>

typedef struct _COLUMNS_DIALOG_CONTEXT
{
    HWND TreeListHandle;
    PPH_LIST Columns;

    HWND InactiveList;
    HWND ActiveList;
} COLUMNS_DIALOG_CONTEXT, *PCOLUMNS_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpColumnsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowChooseColumnsDialog(
    __in HWND ParentWindowHandle,
    __in HWND TreeListHandle
    )
{
    COLUMNS_DIALOG_CONTEXT context;

    context.TreeListHandle = TreeListHandle;
    context.Columns = PhCreateList(TreeList_GetColumnCount(TreeListHandle));

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CHOOSECOLUMNS),
        ParentWindowHandle,
        PhpColumnsDlgProc,
        (LPARAM)&context
        );

    PhDereferenceObject(context.Columns);
}

static int __cdecl PhpColumnsCompareDisplayIndex(
    __in const void *elem1,
    __in const void *elem2
    )
{
    PPH_TREELIST_COLUMN column1 = *(PPH_TREELIST_COLUMN *)elem1;
    PPH_TREELIST_COLUMN column2 = *(PPH_TREELIST_COLUMN *)elem2;

    return uintcmp(column1->DisplayIndex, column2->DisplayIndex);
}

__success(return != -1)
static ULONG IndexOfStringInList(
    __in PPH_LIST List,
    __in PWSTR String
    )
{
    ULONG i;

    for (i = 0; i < List->Count; i++)
    {
        if (PhEqualString2(List->Items[i], String, FALSE))
            return i;
    }

    return -1;
}

INT_PTR CALLBACK PhpColumnsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PCOLUMNS_DIALOG_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PCOLUMNS_DIALOG_CONTEXT)lParam;
        SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);
    }
    else
    {
        context = (PCOLUMNS_DIALOG_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());
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
            PH_TREELIST_COLUMN column;
            PPH_LIST displayOrderList;

            context->InactiveList = GetDlgItem(hwndDlg, IDC_INACTIVE);
            context->ActiveList = GetDlgItem(hwndDlg, IDC_ACTIVE);

            count = 0;
            total = TreeList_GetColumnCount(context->TreeListHandle);
            i = 0;

            displayOrderList = PhCreateList(total);

            while (count < total)
            {
                column.Id = i;

                if (TreeList_GetColumn(context->TreeListHandle, &column))
                {
                    PPH_TREELIST_COLUMN copy;

                    copy = PhAllocateCopy(&column, sizeof(PH_TREELIST_COLUMN));
                    PhAddItemList(context->Columns, copy);
                    count++;

                    if (column.Visible)
                    {
                        PhAddItemList(displayOrderList, copy);
                    }
                    else
                    {
                        ListBox_AddString(context->InactiveList, column.Text);
                    }
                }

                i++;
            }

            qsort(displayOrderList->Items, displayOrderList->Count, sizeof(PVOID), PhpColumnsCompareDisplayIndex);

            for (i = 0; i < displayOrderList->Count; i++)
            {
                PPH_TREELIST_COLUMN copy = displayOrderList->Items[i];

                ListBox_AddString(context->ActiveList, copy->Text);
            }

            PhDereferenceObject(displayOrderList);

            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INACTIVE, LBN_SELCHANGE), (LPARAM)context->InactiveList);
            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ACTIVE, LBN_SELCHANGE), (LPARAM)context->ActiveList);
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;

            for (i = 0; i < context->Columns->Count; i++)
                PhFree(context->Columns->Items[i]);

            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
#define ORDER_LIMIT 100
                    PPH_LIST activeList;
                    ULONG activeCount;
                    ULONG i;
                    INT orderArray[ORDER_LIMIT];
                    INT maxOrder;

                    memset(orderArray, 0, sizeof(orderArray));
                    maxOrder = 0;

                    activeCount = ListBox_GetCount(context->ActiveList);
                    activeList = PhCreateList(activeCount);

                    for (i = 0; i < activeCount; i++)
                        PhAddItemList(activeList, PhGetListBoxString(context->ActiveList, i));

                    // Apply visiblity settings.

                    TreeList_SetRedraw(context->TreeListHandle, FALSE);

                    for (i = 0; i < context->Columns->Count; i++)
                    {
                        PPH_TREELIST_COLUMN column = context->Columns->Items[i];
                        PH_TREELIST_COLUMN tempColumn;
                        ULONG index;

                        index = IndexOfStringInList(activeList, column->Text);
                        column->Visible = index != -1;
                        column->DisplayIndex = index; // the active list box order is the actual display order

                        TreeList_SetColumn(context->TreeListHandle, column, TLCM_FLAGS);

                        // Get the ViewIndex for use in the second pass.
                        tempColumn.Id = column->Id;
                        TreeList_GetColumn(context->TreeListHandle, &tempColumn);
                        column->s.ViewIndex = tempColumn.s.ViewIndex;
                    }

                    // Do a second pass to create the order array. This is because the ViewIndex of each column 
                    // were unstable in the previous pass since we were both adding and removing columns.
                    for (i = 0; i < context->Columns->Count; i++)
                    {
                        PPH_TREELIST_COLUMN column = context->Columns->Items[i];

                        if (column->Visible)
                        {
                            if (column->DisplayIndex < ORDER_LIMIT)
                            {
                                orderArray[column->DisplayIndex] = column->s.ViewIndex;

                                if ((ULONG)maxOrder < column->DisplayIndex + 1)
                                    maxOrder = column->DisplayIndex + 1;
                            }
                        }
                    }

                    // Apply display order.
                    TreeList_SetColumnOrderArray(context->TreeListHandle, maxOrder, orderArray);

                    TreeList_SetRedraw(context->TreeListHandle, TRUE);

                    PhDereferenceObject(activeList);

                    // Force a refresh of the scrollbars.
                    TreeList_Scroll(context->TreeListHandle, 0, 0);
                    InvalidateRect(context->TreeListHandle, NULL, TRUE);

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            case IDC_INACTIVE:
                {
                    switch (HIWORD(wParam))
                    {
                    case LBN_DBLCLK:
                        {
                            SendMessage(hwndDlg, WM_COMMAND, IDC_SHOW, 0);
                        }
                        break;
                    case LBN_SELCHANGE:
                        {
                            INT sel = ListBox_GetCurSel(context->InactiveList);

                            EnableWindow(GetDlgItem(hwndDlg, IDC_SHOW), sel != -1);
                        }
                        break;
                    }
                }
                break;
            case IDC_ACTIVE:
                {
                    switch (HIWORD(wParam))
                    {
                    case LBN_DBLCLK:
                        {
                            SendMessage(hwndDlg, WM_COMMAND, IDC_HIDE, 0);
                        }
                        break;
                    case LBN_SELCHANGE:
                        {
                            INT sel = ListBox_GetCurSel(context->ActiveList);
                            INT count = ListBox_GetCount(context->ActiveList);

                            EnableWindow(GetDlgItem(hwndDlg, IDC_HIDE), sel != -1 && count != 1);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEUP), sel != 0 && sel != -1);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEDOWN), sel != count - 1 && sel != -1);
                        }
                        break;
                    }
                }
                break;
            case IDC_SHOW:
                {
                    INT sel;
                    INT count;
                    PPH_STRING string;

                    sel = ListBox_GetCurSel(context->InactiveList);
                    count = ListBox_GetCount(context->InactiveList);

                    if (string = PhGetListBoxString(context->InactiveList, sel))
                    {
                        ListBox_DeleteString(context->InactiveList, sel);
                        ListBox_AddString(context->ActiveList, string->Buffer);
                        PhDereferenceObject(string);

                        count--;

                        if (sel >= count - 1)
                            sel = count - 1;

                        ListBox_SetCurSel(context->InactiveList, sel);

                        SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INACTIVE, LBN_SELCHANGE), (LPARAM)context->InactiveList);
                        SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ACTIVE, LBN_SELCHANGE), (LPARAM)context->ActiveList);
                    }
                }
                break;
            case IDC_HIDE:
                {
                    INT sel;
                    INT count;
                    PPH_STRING string;

                    sel = ListBox_GetCurSel(context->ActiveList);
                    count = ListBox_GetCount(context->ActiveList);

                    if (count != 1)
                    {
                        if (string = PhGetListBoxString(context->ActiveList, sel))
                        {
                            ListBox_DeleteString(context->ActiveList, sel);
                            ListBox_AddString(context->InactiveList, string->Buffer);
                            PhDereferenceObject(string);

                            count--;

                            if (sel >= count - 1)
                                sel = count - 1;

                            ListBox_SetCurSel(context->ActiveList, sel);

                            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INACTIVE, LBN_SELCHANGE), (LPARAM)context->InactiveList);
                            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ACTIVE, LBN_SELCHANGE), (LPARAM)context->ActiveList);
                        }
                    }
                }
                break;
            case IDC_MOVEUP:
                {
                    INT sel;
                    INT count;
                    PPH_STRING string;

                    sel = ListBox_GetCurSel(context->ActiveList);
                    count = ListBox_GetCount(context->ActiveList);

                    if (sel != 0)
                    {
                        if (string = PhGetListBoxString(context->ActiveList, sel))
                        {
                            ListBox_DeleteString(context->ActiveList, sel);
                            ListBox_InsertString(context->ActiveList, sel - 1, string->Buffer);
                            PhDereferenceObject(string);

                            sel -= 1;
                            ListBox_SetCurSel(context->ActiveList, sel);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEUP), sel != 0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEDOWN), sel != count - 1);
                        }
                    }
                }
                break;
            case IDC_MOVEDOWN:
                {
                    INT sel;
                    INT count;
                    PPH_STRING string;

                    sel = ListBox_GetCurSel(context->ActiveList);
                    count = ListBox_GetCount(context->ActiveList);

                    if (sel != count - 1)
                    {
                        if (string = PhGetListBoxString(context->ActiveList, sel))
                        {
                            ListBox_DeleteString(context->ActiveList, sel);
                            ListBox_InsertString(context->ActiveList, sel + 1, string->Buffer);
                            PhDereferenceObject(string);

                            sel += 1;
                            ListBox_SetCurSel(context->ActiveList, sel);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEUP), sel != 0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEDOWN), sel != count - 1);
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
