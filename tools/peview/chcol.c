/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2017-2021
 *
 */

// NOTE: Copied from processhacker\ProcessHacker\chcol.c

#include <peview.h>
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
    PPH_LIST InactiveListArray;
    PPH_LIST ActiveListArray;
    PPH_STRING InactiveSearchboxText;
    PPH_STRING ActiveSearchboxText;
} COLUMNS_DIALOG_CONTEXT, *PCOLUMNS_DIALOG_CONTEXT;

INT_PTR CALLBACK PvColumnsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PvShowChooseColumnsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HWND ControlHandle,
    _In_ ULONG Type
    )
{
    COLUMNS_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(COLUMNS_DIALOG_CONTEXT));
    context.ControlHandle = ControlHandle;
    context.Type = Type;

    if (Type == PV_CONTROL_TYPE_TREE_NEW)
        context.Columns = PhCreateList(TreeNew_GetColumnCount(ControlHandle));
    else
        return;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CHOOSECOLUMNS),
        ParentWindowHandle,
        PvColumnsDlgProc,
        &context
        );

    PhDereferenceObject(context.Columns);
}

static int __cdecl PvColumnsCompareDisplayIndexTn(
    _In_ const void* Context,
    _In_ const void* elem1,
    _In_ const void* elem2
    )
{
    PPH_TREENEW_COLUMN column1 = *(PPH_TREENEW_COLUMN*)elem1;
    PPH_TREENEW_COLUMN column2 = *(PPH_TREENEW_COLUMN*)elem2;

    return uintcmp(column1->DisplayIndex, column2->DisplayIndex);
}

static int __cdecl PvInactiveColumnsCompareNameTn(
    _In_ const void* Context,
    _In_ const void* elem1,
    _In_ const void* elem2
    )
{
    PWSTR column1 = *(PWSTR*)elem1;
    PWSTR column2 = *(PWSTR*)elem2;

    return PhCompareStringZ(column1, column2, FALSE);
}

_Success_(return != ULONG_MAX)
static ULONG IndexOfStringInList(
    _In_ PPH_LIST List,
    _In_ PWSTR String
    )
{
    for (ULONG i = 0; i < List->Count; i++)
    {
        if (PhEqualStringZ(List->Items[i], String, FALSE))
            return i;
    }

    return ULONG_MAX;
}

static HFONT PvColumnsGetCurrentFont(
    _In_ HWND hwnd
    )
{
    NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
    HFONT font;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(hwnd);

    if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, dpiValue))
        font = CreateFontIndirect(&metrics.lfMessageFont);
    else
        font = NULL;

    return font;
}

BOOLEAN PvColumnsWordMatchStringRef(
    _In_ PPH_STRING SearchboxText,
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = SearchboxText->sr;

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, L'|', &part, &remainingPart);

        if (part.Length)
        {
            if (PhFindStringInStringRef(Text, &part, TRUE) != SIZE_MAX)
                return TRUE;
        }
    }

    return FALSE;
}

VOID PvColumnsResetListBox(
    _In_ HWND ListBoxHandle,
    _In_ PPH_STRING SearchboxText,
    _In_ PPH_LIST Array,
    _In_ PVOID CompareFunction
    )
{
    SendMessage(ListBoxHandle, WM_SETREDRAW, FALSE, 0);

    ListBox_ResetContent(ListBoxHandle);

    if (CompareFunction)
        qsort_s(Array->Items, Array->Count, sizeof(ULONG_PTR), CompareFunction, NULL);

    if (PhIsNullOrEmptyString(SearchboxText))
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

            if (PvColumnsWordMatchStringRef(SearchboxText, &text))
            {
                ListBox_InsertString(ListBoxHandle, index, Array->Items[i]);
                index++;
            }
        }
    }

    SendMessage(ListBoxHandle, WM_SETREDRAW, TRUE, 0);
}

VOID PvSetListHeight(
    _In_ PCOLUMNS_DIALOG_CONTEXT context,
    _In_ HWND hwndDlg
)
{
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(hwndDlg);

    ListBox_SetItemHeight(context->InactiveWindowHandle, 0, PhGetDpi(16, dpiValue));
    ListBox_SetItemHeight(context->ActiveWindowHandle, 0, PhGetDpi(16, dpiValue));
}

INT_PTR CALLBACK PvColumnsDlgProc(
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

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PvImageSmallIcon);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PvImageLargeIcon);

            context->InactiveWindowHandle = GetDlgItem(hwndDlg, IDC_INACTIVE);
            context->ActiveWindowHandle = GetDlgItem(hwndDlg, IDC_ACTIVE);
            context->SearchInactiveHandle = GetDlgItem(hwndDlg, IDC_SEARCH);
            context->SearchActiveHandle = GetDlgItem(hwndDlg, IDC_FILTER);
            context->InactiveListArray = PhCreateList(1);
            context->ActiveListArray = PhCreateList(1);
            context->ControlFont = PvColumnsGetCurrentFont(hwndDlg);
            context->InactiveSearchboxText = PhReferenceEmptyString();
            context->ActiveSearchboxText = PhReferenceEmptyString();

            PvCreateSearchControl(context->SearchInactiveHandle, L"Inactive columns...");
            PvCreateSearchControl(context->SearchActiveHandle, L"Active columns...");

            PvSetListHeight(context, hwndDlg);

            Button_Enable(GetDlgItem(hwndDlg, IDC_HIDE), FALSE);
            Button_Enable(GetDlgItem(hwndDlg, IDC_SHOW), FALSE);
            Button_Enable(GetDlgItem(hwndDlg, IDC_MOVEUP), FALSE);
            Button_Enable(GetDlgItem(hwndDlg, IDC_MOVEDOWN), FALSE);

            if (PhGetIntegerSetting(L"EnableThemeSupport"))
            {
                context->BrushNormal = CreateSolidBrush(RGB(43, 43, 43));
                context->BrushHot = CreateSolidBrush(RGB(128, 128, 128));
                context->BrushPushed = CreateSolidBrush(RGB(153, 209, 255));
                context->TextColor = RGB(0xff, 0xff, 0xff);
            }
            else
            {
                context->BrushNormal = GetSysColorBrush(COLOR_WINDOW);
                context->BrushHot = CreateSolidBrush(RGB(145, 201, 247));
                context->BrushPushed = CreateSolidBrush(RGB(153, 209, 255));
                context->TextColor = GetSysColor(COLOR_WINDOWTEXT);
            }

            if (context->Type == PV_CONTROL_TYPE_TREE_NEW)
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
                            PhAddItemList(context->InactiveListArray, column.Text);
                        }
                    }

                    i++;
                }

                qsort_s(displayOrderList->Items, displayOrderList->Count, sizeof(PVOID), PvColumnsCompareDisplayIndexTn, NULL);
            }

            PvColumnsResetListBox(
                context->InactiveWindowHandle,
                NULL,
                context->InactiveListArray,
                PvInactiveColumnsCompareNameTn
                );

            if (displayOrderList)
            {
                for (i = 0; i < displayOrderList->Count; i++)
                {
                    if (context->Type == PV_CONTROL_TYPE_TREE_NEW)
                    {
                        PPH_TREENEW_COLUMN copy = displayOrderList->Items[i];

                        PhAddItemList(context->ActiveListArray, copy->Text);
                        ListBox_InsertItemData(context->ActiveWindowHandle, i, copy->Text);
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
            if (context->InactiveSearchboxText)
                PhDereferenceObject(context->InactiveSearchboxText);
            if (context->ActiveSearchboxText)
                PhDereferenceObject(context->ActiveSearchboxText);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_DPICHANGED:
        {
            PvSetListHeight (context, hwndDlg);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    if (GET_WM_COMMAND_HWND(wParam, lParam) == context->SearchInactiveHandle)
                    {
                        PPH_STRING newSearchboxText = PH_AUTO(PhGetWindowText(context->SearchInactiveHandle));

                        if (!PhEqualString(context->InactiveSearchboxText, newSearchboxText, FALSE))
                        {
                            PhSwapReference(&context->InactiveSearchboxText, newSearchboxText);

                            PvColumnsResetListBox(
                                context->InactiveWindowHandle, context->InactiveSearchboxText,
                                context->InactiveListArray, PvInactiveColumnsCompareNameTn);
                        }
                    }
                    else if (GET_WM_COMMAND_HWND(wParam, lParam) == context->SearchActiveHandle)
                    {
                        PPH_STRING newSearchboxText = PH_AUTO(PhGetWindowText(context->SearchActiveHandle));

                        if (!PhEqualString(context->ActiveSearchboxText, newSearchboxText, FALSE))
                        {
                            PhSwapReference(&context->ActiveSearchboxText, newSearchboxText);

                            PvColumnsResetListBox(
                                context->ActiveWindowHandle, context->ActiveSearchboxText,
                                context->ActiveListArray, NULL);
                        }
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
#define ORDER_LIMIT 200
                    ULONG i;
                    INT orderArray[ORDER_LIMIT];
                    INT maxOrder;

                    memset(orderArray, 0, sizeof(orderArray));
                    maxOrder = 0;

                    if (context->Type == PV_CONTROL_TYPE_TREE_NEW)
                    {
                        // Apply visibility settings and build the order array.

                        TreeNew_SetRedraw(context->ControlHandle, FALSE);

                        for (i = 0; i < context->Columns->Count; i++)
                        {
                            PPH_TREENEW_COLUMN column = context->Columns->Items[i];
                            ULONG index;

                            index = IndexOfStringInList(context->ActiveListArray, column->Text);
                            column->Visible = index != ULONG_MAX;

                            TreeNew_SetColumn(context->ControlHandle, TN_COLUMN_FLAG_VISIBLE, column);

                            if (column->Visible && index < ORDER_LIMIT)
                            {
                                orderArray[index] = column->Id;

                                if ((ULONG)maxOrder < index + 1)
                                    maxOrder = index + 1;
                            }
                        }

                        // Apply display order.
                        TreeNew_SetColumnOrderArray(context->ControlHandle, maxOrder, orderArray);

                        TreeNew_SetRedraw(context->ControlHandle, TRUE);

                        InvalidateRect(context->ControlHandle, NULL, FALSE);
                    }

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
                            INT sel = ListBox_GetCurSel(context->InactiveWindowHandle);

                            EnableWindow(GetDlgItem(hwndDlg, IDC_SHOW), sel != LB_ERR);
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
                            INT sel = ListBox_GetCurSel(context->ActiveWindowHandle);
                            INT count = ListBox_GetCount(context->ActiveWindowHandle);

                            if (sel != LB_ERR)
                            {
                                EnableWindow(GetDlgItem(hwndDlg, IDC_HIDE), sel != 0);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEUP), sel != 0);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEDOWN), sel != count - 1);
                            }
                        }
                        break;
                    }
                }
                break;
            case IDC_SHOW:
                {
                    INT sel;
                    INT count;
                    PWSTR string;

                    sel = ListBox_GetCurSel(context->InactiveWindowHandle);
                    count = ListBox_GetCount(context->InactiveWindowHandle);

                    if (sel != LB_ERR)
                    {
                        if (string = (PWSTR)ListBox_GetItemData(context->InactiveWindowHandle, sel))
                        {
                            ULONG index = IndexOfStringInList(context->InactiveListArray, string);

                            if (index != ULONG_MAX)
                            {
                                PVOID item = context->InactiveListArray->Items[index];

                                PhRemoveItemsList(context->InactiveListArray, index, 1);
                                PhAddItemList(context->ActiveListArray, item);

                                ListBox_DeleteString(context->InactiveWindowHandle, sel);
                                ListBox_AddItemData(context->ActiveWindowHandle, item);
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
                    INT sel;
                    INT count;
                    PWSTR string;

                    sel = ListBox_GetCurSel(context->ActiveWindowHandle);
                    count = ListBox_GetCount(context->ActiveWindowHandle);

                    if (sel != LB_ERR)
                    {
                        if (string = (PWSTR)ListBox_GetItemData(context->ActiveWindowHandle, sel))
                        {
                            ULONG index = IndexOfStringInList(context->ActiveListArray, string);

                            if (index != ULONG_MAX)
                            {
                                PVOID item = context->ActiveListArray->Items[index];

                                // Remove from active array, insert into inactive
                                PhRemoveItemsList(context->ActiveListArray, index, 1);
                                PhAddItemList(context->InactiveListArray, item);

                                // Sort inactive list with new entry
                                qsort_s(context->InactiveListArray->Items, context->InactiveListArray->Count, sizeof(ULONG_PTR), PvInactiveColumnsCompareNameTn, NULL);
                                // Find index of new entry in inactive list
                                ULONG lb_index = IndexOfStringInList(context->InactiveListArray, item);

                                // Delete from active list
                                ListBox_DeleteString(context->ActiveWindowHandle, sel);
                                // Add to list in the same position as the inactive list
                                ListBox_InsertString(context->InactiveWindowHandle, lb_index, item);

                                PvColumnsResetListBox(context->InactiveWindowHandle, context->InactiveSearchboxText, context->InactiveListArray, PvInactiveColumnsCompareNameTn);
                            }

                            count--;

                            if (sel >= count - 1)
                                sel = count - 1;

                            if (sel != LB_ERR)
                            {
                                ListBox_SetCurSel(context->ActiveWindowHandle, sel);
                            }
                        }
                    }

                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INACTIVE, LBN_SELCHANGE), (LPARAM)context->InactiveWindowHandle);
                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ACTIVE, LBN_SELCHANGE), (LPARAM)context->ActiveWindowHandle);
                }
                break;
            case IDC_MOVEUP:
                {
                    INT sel;
                    INT count;
                    PWSTR string;

                    sel = ListBox_GetCurSel(context->ActiveWindowHandle);
                    count = ListBox_GetCount(context->ActiveWindowHandle);

                    if (sel != LB_ERR)
                    {
                        if (string = (PWSTR)ListBox_GetItemData(context->ActiveWindowHandle, sel))
                        {
                            ULONG index = IndexOfStringInList(context->ActiveListArray, string);

                            if (index != ULONG_MAX)
                            {
                                PVOID item = context->ActiveListArray->Items[index];
                                PhRemoveItemsList(context->ActiveListArray, index, 1);
                                PhInsertItemList(context->ActiveListArray, index - 1, item);

                                ListBox_DeleteString(context->ActiveWindowHandle, sel);
                                sel -= 1;
                                ListBox_InsertItemData(context->ActiveWindowHandle, sel, item);
                                ListBox_SetCurSel(context->ActiveWindowHandle, sel);

                                EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEUP), sel != 0);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEDOWN), sel != count - 1);
                            }
                        }
                    }
                }
                break;
            case IDC_MOVEDOWN:
                {
                    INT sel;
                    INT count;
                    PWSTR string;

                    sel = ListBox_GetCurSel(context->ActiveWindowHandle);
                    count = ListBox_GetCount(context->ActiveWindowHandle);

                    if (sel != LB_ERR && sel != count - 1)
                    {
                        if (string = (PWSTR)ListBox_GetItemData(context->ActiveWindowHandle, sel))
                        {
                            ULONG index = IndexOfStringInList(context->ActiveListArray, string);

                            if (index != ULONG_MAX)
                            {
                                PVOID item = context->ActiveListArray->Items[index];
                                PhRemoveItemsList(context->ActiveListArray, index, 1);
                                PhInsertItemList(context->ActiveListArray, index + 1, item);

                                ListBox_DeleteString(context->ActiveWindowHandle, sel);
                                sel += 1;
                                ListBox_InsertItemData(context->ActiveWindowHandle, sel, item);
                                ListBox_SetCurSel(context->ActiveWindowHandle, sel);

                                EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEUP), sel != 0);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEDOWN), sel != count - 1);
                            }
                        }
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
                PWSTR string;
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

                if (!(string = (PWSTR)ListBox_GetItemData(drawInfo->hwndItem, drawInfo->itemID)))
                    break;

                bufferDc = CreateCompatibleDC(drawInfo->hDC);
                bufferBitmap = CreateCompatibleBitmap(drawInfo->hDC, bufferRect.right, bufferRect.bottom);

                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);
                SelectFont(bufferDc, context->ControlFont);
                SetBkMode(bufferDc, TRANSPARENT);

                if (isSelected || isFocused)
                {
                    FillRect(bufferDc, &bufferRect, context->BrushHot);
                    //FrameRect(bufferDc, &bufferRect, GetStockBrush(BLACK_BRUSH));
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
                    string,
                    -1,
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

                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
