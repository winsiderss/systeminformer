/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2017-2023
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
    COLORREF TextColor;

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
} COLUMNS_DIALOG_CONTEXT, *PCOLUMNS_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpColumnsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

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

VOID NTAPI PhpInactiveColumnsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PCOLUMNS_DIALOG_CONTEXT context = Context;

    PhpColumnsResetListBox(
        context->InactiveWindowHandle,
        MatchHandle,
        context->InactiveListArray,
        PhpInactiveColumnsCompareNameTn
        );
}

VOID NTAPI PhpActiveColumnsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PCOLUMNS_DIALOG_CONTEXT context = Context;

    PhpColumnsResetListBox(
        context->ActiveWindowHandle,
        MatchHandle,
        context->ActiveListArray,
        NULL
        );
}

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
            context->ControlFont = PhCreateMessageFont(dpiValue); // PhDuplicateFont(PhTreeWindowFont)

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

            ListBox_SetItemHeight(context->InactiveWindowHandle, 0, PhGetDpi(16, dpiValue));
            ListBox_SetItemHeight(context->ActiveWindowHandle, 0, PhGetDpi(16, dpiValue));

            Button_Enable(context->HideWindowHandle, FALSE);
            Button_Enable(context->ShowWindowHandle, FALSE);
            Button_Enable(context->MoveUpHandle, FALSE);
            Button_Enable(context->MoveDownHandle, FALSE);

            if (PhEnableThemeSupport)
            {
                context->BrushNormal = CreateSolidBrush(PhThemeWindowBackgroundColor);
                context->BrushHot = CreateSolidBrush(PhThemeWindowHighlightColor);
                context->BrushPushed = CreateSolidBrush(PhThemeWindowHighlight2Color);
                context->TextColor = PhThemeWindowTextColor;
            }
            else
            {
                context->BrushNormal = GetSysColorBrush(COLOR_WINDOW);
                context->BrushHot = CreateSolidBrush(RGB(145, 201, 247));
                context->BrushPushed = CreateSolidBrush(RGB(153, 209, 255));
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

            ListBox_SetItemHeight(context->InactiveWindowHandle, 0, PhGetDpi(16, LOWORD(wParam)));
            ListBox_SetItemHeight(context->ActiveWindowHandle, 0, PhGetDpi(16, LOWORD(wParam)));
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
                        orderArray = _malloca(orderArraySize);

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

                        _freea(orderArray);
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
                    LONG sel;
                    LONG count;
                    PPH_STRING string;

                    sel = ListBox_GetCurSel(context->InactiveWindowHandle);
                    count = ListBox_GetCount(context->InactiveWindowHandle);

                    if (sel != LB_ERR)
                    {
                        string = PhGetListBoxString(context->InactiveWindowHandle, sel);

                        if (!PhIsNullOrEmptyString(string))
                        {
                            ULONG index = IndexOfStringInList(context->InactiveListArray, string->Buffer);

                            if (index != ULONG_MAX)
                            {
                                PVOID item = context->InactiveListArray->Items[index];

                                PhRemoveItemsList(context->InactiveListArray, index, 1);
                                PhAddItemList(context->ActiveListArray, item);

                                ListBox_DeleteString(context->InactiveWindowHandle, sel);
                                ListBox_AddString(context->ActiveWindowHandle, item);
                            }

                            count--;

                            if (sel >= count - 1)
                                sel = count - 1;

                            if (sel != LB_ERR)
                            {
                                ListBox_SetCurSel(context->InactiveWindowHandle, sel);
                            }
                        }
                    }

                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INACTIVE, LBN_SELCHANGE), (LPARAM)context->InactiveWindowHandle);
                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ACTIVE, LBN_SELCHANGE), (LPARAM)context->ActiveWindowHandle);
                }
                break;
            case IDC_HIDE:
                {
                    LONG sel;
                    LONG count;
                    PPH_STRING string;

                    sel = ListBox_GetCurSel(context->ActiveWindowHandle);
                    count = ListBox_GetCount(context->ActiveWindowHandle);

                    if (sel != LB_ERR)
                    {
                        string = PhGetListBoxString(context->ActiveWindowHandle, sel);

                        if (!PhIsNullOrEmptyString(string))
                        {
                            ULONG index = IndexOfStringInList(context->ActiveListArray, string->Buffer);

                            if (index != ULONG_MAX)
                            {
                                PVOID item = context->ActiveListArray->Items[index];

                                // Remove from active array, insert into inactive
                                PhRemoveItemsList(context->ActiveListArray, index, 1);
                                PhAddItemList(context->InactiveListArray, item);

                                // Sort inactive list with new entry
                                qsort_s(context->InactiveListArray->Items, context->InactiveListArray->Count, sizeof(ULONG_PTR), PhpInactiveColumnsCompareNameTn, NULL);
                                // Find index of new entry in inactive list
                                ULONG lb_index = IndexOfStringInList(context->InactiveListArray, item);

                                // Delete from active list
                                ListBox_DeleteString(context->ActiveWindowHandle, sel);
                                // Add to list in the same position as the inactive list
                                ListBox_InsertString(context->InactiveWindowHandle, lb_index, item);

                                PhpColumnsResetListBox(context->InactiveWindowHandle, 0, context->InactiveListArray, PhpInactiveColumnsCompareNameTn);
                            }

                            count--;

                            if (sel >= count - 1)
                                sel = count - 1;

                            if (sel != LB_ERR)
                            {
                                ListBox_SetCurSel(context->ActiveWindowHandle, sel);
                            }
                        }

                        PhClearReference(&string);
                    }

                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INACTIVE, LBN_SELCHANGE), (LPARAM)context->InactiveWindowHandle);
                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ACTIVE, LBN_SELCHANGE), (LPARAM)context->ActiveWindowHandle);
                }
                break;
            case IDC_MOVEUP:
                {
                    LONG sel;
                    LONG count;
                    PPH_STRING string;

                    sel = ListBox_GetCurSel(context->ActiveWindowHandle);
                    count = ListBox_GetCount(context->ActiveWindowHandle);

                    if (sel != LB_ERR)
                    {
                        string = PhGetListBoxString(context->ActiveWindowHandle, sel);

                        if (!PhIsNullOrEmptyString(string))
                        {
                            ULONG index = IndexOfStringInList(context->ActiveListArray, string->Buffer);

                            if (index != ULONG_MAX)
                            {
                                PVOID item = context->ActiveListArray->Items[index];
                                PhRemoveItemsList(context->ActiveListArray, index, 1);
                                PhInsertItemList(context->ActiveListArray, index - 1, item);

                                ListBox_DeleteString(context->ActiveWindowHandle, sel);
                                sel -= 1;
                                ListBox_InsertString(context->ActiveWindowHandle, sel, item);
                                ListBox_SetCurSel(context->ActiveWindowHandle, sel);

                                EnableWindow(context->MoveUpHandle, sel != 0);
                                EnableWindow(context->MoveDownHandle, sel != count - 1);
                            }
                        }

                        PhClearReference(&string);
                    }
                }
                break;
            case IDC_MOVEDOWN:
                {
                    LONG sel;
                    LONG count;
                    PPH_STRING string;

                    sel = ListBox_GetCurSel(context->ActiveWindowHandle);
                    count = ListBox_GetCount(context->ActiveWindowHandle);

                    if (sel != LB_ERR && sel != count - 1)
                    {
                        string = PhGetListBoxString(context->ActiveWindowHandle, sel);

                        if (!PhIsNullOrEmptyString(string))
                        {
                            ULONG index = IndexOfStringInList(context->ActiveListArray, string->Buffer);

                            if (index != ULONG_MAX)
                            {
                                PVOID item = context->ActiveListArray->Items[index];
                                PhRemoveItemsList(context->ActiveListArray, index, 1);
                                PhInsertItemList(context->ActiveListArray, index + 1, item);

                                ListBox_DeleteString(context->ActiveWindowHandle, sel);
                                sel += 1;
                                ListBox_InsertString(context->ActiveWindowHandle, sel, item);
                                ListBox_SetCurSel(context->ActiveWindowHandle, sel);

                                EnableWindow(context->MoveUpHandle, sel != 0);
                                EnableWindow(context->MoveDownHandle, sel != count - 1);
                            }
                        }

                        PhClearReference(&string);
                    }
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

    return FALSE;
}
