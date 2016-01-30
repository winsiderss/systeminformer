/*
 * Process Hacker ToolStatus -
 *   Statusbar Customize Dialog
 *
 * Copyright (C) 2015-2016 dmex
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

#include "toolstatus.h"

static BOOLEAN CustomizeItemExists(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IdCommand
    )
{
    INT buttonIndex = 0;
    INT buttonCount = 0;

    buttonCount = ListBox_GetCount(Context->CurrentListHandle);

    if (buttonCount == LB_ERR)
        return FALSE;

    for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
    {
        PBUTTON_CONTEXT buttonContext;

        buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, buttonIndex);

        if (buttonContext == NULL)
            continue;

        if (buttonContext->IdCommand == IdCommand)
            return TRUE;
    }

    return FALSE;
}

static VOID CustomizeInsertStatusBarItem(
    _In_ INT Index,
    _In_ PBUTTON_CONTEXT ButtonContext
    )
{
    PSTATUSBAR_ITEM statusItem;

    statusItem = PhAllocate(sizeof(STATUSBAR_ITEM));
    memset(statusItem, 0, sizeof(STATUSBAR_ITEM));

    statusItem->Id = ButtonContext->IdCommand;

    PhInsertItemList(StatusBarItemList, Index, statusItem);

    StatusBarUpdate(TRUE);
}

static VOID CustomizeAddButton(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IndexAvail,
    _In_ INT IndexTo
    )
{
    INT count;
    PBUTTON_CONTEXT buttonContext;

    count = ListBox_GetCount(Context->AvailableListHandle);
    buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, IndexAvail);

    if (count == LB_ERR)
        return;

    if (buttonContext == NULL)
        return;

    if (!buttonContext->IsVirtual)
    {
        // remove from 'available' list
        ListBox_DeleteString(Context->AvailableListHandle, IndexAvail);

        if (IndexAvail == count - 1)
        {
            ListBox_SetCurSel(Context->AvailableListHandle, IndexAvail - 1);
        }
        else
        {
            ListBox_SetCurSel(Context->AvailableListHandle, IndexAvail);
        }

        // insert into 'current' list 
        ListBox_InsertItemData(Context->CurrentListHandle, IndexTo, buttonContext);

        CustomizeInsertStatusBarItem(IndexTo, buttonContext);
    }

    SendMessage(Context->DialogHandle, WM_COMMAND, MAKEWPARAM(IDC_AVAILABLE, LBN_SELCHANGE), 0);
}

static VOID CustomizeRemoveButton(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IndexFrom
    )
{
    PBUTTON_CONTEXT buttonContext;

    buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, IndexFrom);

    if (buttonContext == NULL)
        return;

    ListBox_DeleteString(Context->CurrentListHandle, IndexFrom);
    ListBox_SetCurSel(Context->CurrentListHandle, IndexFrom);

    PhRemoveItemList(StatusBarItemList, IndexFrom);

    if (!buttonContext->IsVirtual)
    {
        INT count;

        count = ListBox_GetCount(Context->AvailableListHandle);

        if (count == LB_ERR)
            count = 1;

        // insert into 'available' list
        ListBox_InsertItemData(Context->AvailableListHandle, count - 1, buttonContext);
    }

    SendMessage(Context->DialogHandle, WM_COMMAND, MAKEWPARAM(IDC_CURRENT, LBN_SELCHANGE), 0);

    StatusBarUpdate(TRUE);
}

static VOID CustomizeMoveButton(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IndexFrom,
    _In_ INT IndexTo
    )
{
    INT count;
    PBUTTON_CONTEXT buttonContext;

    if (IndexFrom == IndexTo)
        return;

    count = ListBox_GetCount(Context->CurrentListHandle);
    buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, IndexFrom);

    if (count == LB_ERR)
        return;

    if (buttonContext == NULL)
        return;

    ListBox_DeleteString(Context->CurrentListHandle, IndexFrom);
    ListBox_InsertItemData(Context->CurrentListHandle, IndexTo, buttonContext);
    ListBox_SetCurSel(Context->CurrentListHandle, IndexTo);

    if (IndexTo <= 0)
    {
        Button_Enable(Context->MoveUpButtonHandle, FALSE);
    }
    else
    {
        Button_Enable(Context->MoveUpButtonHandle, TRUE);
    }

    // last item is always separator
    if (IndexTo >= (count - 2))
    {
        Button_Enable(Context->MoveDownButtonHandle, FALSE);
    }
    else
    {
        Button_Enable(Context->MoveDownButtonHandle, TRUE);
    }

    PhRemoveItemList(StatusBarItemList, IndexFrom);

    CustomizeInsertStatusBarItem(IndexTo, buttonContext);
}

static VOID CustomizeFreeButtons(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    INT buttonIndex = 0;
    INT buttonCount = 0;

    buttonCount = ListBox_GetCount(Context->CurrentListHandle);

    if (buttonCount != LB_ERR)
    {
        for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
        {
            PBUTTON_CONTEXT buttonContext;

            if (buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, buttonIndex))
            {
                PhFree(buttonContext);
            }
        }
    }

    buttonCount = ListBox_GetCount(Context->AvailableListHandle);

    if (buttonCount != LB_ERR)
    {
        for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
        {
            PBUTTON_CONTEXT buttonContext;

            if (buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, buttonIndex))
            {
                PhFree(buttonContext);
            }
        }
    }
}

static VOID CustomizeLoadItems(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    ULONG buttonIndex;
    PBUTTON_CONTEXT buttonContext;

    CustomizeFreeButtons(Context);

    ListBox_ResetContent(Context->AvailableListHandle);
    ListBox_ResetContent(Context->CurrentListHandle);

    for (buttonIndex = 0; buttonIndex < StatusBarItemList->Count; buttonIndex++)
    {
        PSTATUSBAR_ITEM statusItem;

        statusItem = StatusBarItemList->Items[buttonIndex];

        buttonContext = PhAllocate(sizeof(BUTTON_CONTEXT));
        memset(buttonContext, 0, sizeof(BUTTON_CONTEXT));

        buttonContext->IdCommand = statusItem->Id;

        ListBox_AddItemData(Context->CurrentListHandle, buttonContext);
    }

    for (buttonIndex = 0; buttonIndex < MAX_STATUSBAR_ITEMS; buttonIndex++)
    {
        ULONG buttonId = StatusBarItems[buttonIndex];

        if (CustomizeItemExists(Context, buttonId))
            continue;

        buttonContext = PhAllocate(sizeof(BUTTON_CONTEXT));
        memset(buttonContext, 0, sizeof(BUTTON_CONTEXT));

        buttonContext->IdCommand = buttonId;

        ListBox_AddItemData(Context->AvailableListHandle, buttonContext);
    }

    // Append separator to the last 'current list' position
    buttonContext = PhAllocate(sizeof(BUTTON_CONTEXT));
    memset(buttonContext, 0, sizeof(BUTTON_CONTEXT));
    buttonContext->IsVirtual = TRUE;

    buttonIndex = ListBox_AddItemData(Context->CurrentListHandle, buttonContext);
    ListBox_SetCurSel(Context->CurrentListHandle, buttonIndex);
    ListBox_SetTopIndex(Context->CurrentListHandle, buttonIndex);

    // Append separator to the last 'available list' position
    buttonContext = PhAllocate(sizeof(BUTTON_CONTEXT));
    memset(buttonContext, 0, sizeof(BUTTON_CONTEXT));
    buttonContext->IsVirtual = TRUE;

    buttonIndex = ListBox_AddItemData(Context->AvailableListHandle, buttonContext);
    ListBox_SetCurSel(Context->AvailableListHandle, buttonIndex);
    ListBox_SetTopIndex(Context->AvailableListHandle, 0); // NOTE: This is intentional.

    // Disable buttons
    Button_Enable(Context->MoveUpButtonHandle, FALSE);
    Button_Enable(Context->MoveDownButtonHandle, FALSE);
    Button_Enable(Context->AddButtonHandle, FALSE);
    Button_Enable(Context->RemoveButtonHandle, FALSE);
}

static INT_PTR CALLBACK CustomizeDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCUSTOMIZE_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PCUSTOMIZE_CONTEXT)PhAllocate(sizeof(CUSTOMIZE_CONTEXT));
        memset(context, 0, sizeof(CUSTOMIZE_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PCUSTOMIZE_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_NCDESTROY)
        {
            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhCenterWindow(hwndDlg, PhMainWndHandle);

            context->DialogHandle = hwndDlg;
            context->AvailableListHandle = GetDlgItem(hwndDlg, IDC_AVAILABLE);
            context->CurrentListHandle = GetDlgItem(hwndDlg, IDC_CURRENT);
            context->MoveUpButtonHandle = GetDlgItem(hwndDlg, IDC_MOVEUP);
            context->MoveDownButtonHandle = GetDlgItem(hwndDlg, IDC_MOVEDOWN);
            context->AddButtonHandle = GetDlgItem(hwndDlg, IDC_ADD);
            context->RemoveButtonHandle = GetDlgItem(hwndDlg, IDC_REMOVE);
            context->FontHandle = PhDuplicateFont((HFONT)SendMessage(StatusBarHandle, WM_GETFONT, 0, 0));

            ListBox_SetItemHeight(context->AvailableListHandle, 0, 22); // BitmapHeight
            ListBox_SetItemHeight(context->CurrentListHandle, 0, 22); // BitmapHeight 

            CustomizeLoadItems(context);

            SendMessage(context->DialogHandle, WM_NEXTDLGCTL, (WPARAM)context->CurrentListHandle, TRUE);
        }
        return TRUE;
    case WM_DESTROY:
        {
            StatusBarSaveSettings();

            CustomizeFreeButtons(context);

            if (context->FontHandle)
            {
                DeleteObject(context->FontHandle);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_AVAILABLE:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                    case LBN_SELCHANGE:
                        {
                            INT count;
                            INT index;

                            count = ListBox_GetCount(context->AvailableListHandle);
                            index = ListBox_GetCurSel(context->AvailableListHandle);

                            if (count == LB_ERR)
                                break;

                            if (index == LB_ERR)
                                break;

                            if (index == (count - 1))
                            {
                                Button_Enable(context->AddButtonHandle, FALSE);
                            }
                            else
                            {
                                Button_Enable(context->AddButtonHandle, TRUE);
                            }
                        }
                        break;
                    case LBN_DBLCLK:
                        {
                            INT count;
                            INT index;
                            INT indexto;

                            count = ListBox_GetCount(context->AvailableListHandle);
                            index = ListBox_GetCurSel(context->AvailableListHandle);
                            indexto = ListBox_GetCurSel(context->CurrentListHandle);

                            if (count == LB_ERR)
                                break;

                            if (index == LB_ERR)
                                break;

                            if (indexto == LB_ERR)
                                break;

                            if (index == (count - 1))
                            {
                                // virtual separator
                                break;
                            }

                            CustomizeAddButton(context, index, indexto);
                        }
                        break;
                    }
                }
                break;
            case IDC_CURRENT:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                    case LBN_SELCHANGE:
                        {
                            INT count;
                            INT index;
                            PBUTTON_CONTEXT buttonContext;

                            count = ListBox_GetCount(context->CurrentListHandle);
                            index = ListBox_GetCurSel(context->CurrentListHandle);

                            if (count == LB_ERR)
                                break;

                            if (index == LB_ERR)
                                break;

                            buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(context->CurrentListHandle, index);
                            if (buttonContext == NULL)
                                break;

                            if (index == 0 && count == 2)
                            {
                                // first and last item
                                Button_Enable(context->MoveUpButtonHandle, FALSE);
                                Button_Enable(context->MoveDownButtonHandle, FALSE);
                            }
                            else if (index == (count - 1))
                            {
                                // last item (virtual separator)
                                Button_Enable(context->MoveUpButtonHandle, FALSE);
                                Button_Enable(context->MoveDownButtonHandle, FALSE);
                            }
                            else if (index == (count - 2))
                            {
                                // second last item (last non-virtual item)
                                Button_Enable(context->MoveUpButtonHandle, TRUE);
                                Button_Enable(context->MoveDownButtonHandle, FALSE);
                            }
                            else if (index == 0)
                            {
                                // first item
                                Button_Enable(context->MoveUpButtonHandle, FALSE);
                                Button_Enable(context->MoveDownButtonHandle, TRUE);
                            }
                            else
                            {
                                Button_Enable(context->MoveUpButtonHandle, TRUE);
                                Button_Enable(context->MoveDownButtonHandle, TRUE);
                            }

                            Button_Enable(context->RemoveButtonHandle, !buttonContext->IsVirtual);
                        }
                        break;
                    case LBN_DBLCLK:
                        {
                            INT count;
                            INT index;

                            count = ListBox_GetCount(context->CurrentListHandle);
                            index = ListBox_GetCurSel(context->CurrentListHandle);

                            if (count == LB_ERR)
                                break;

                            if (index == LB_ERR)
                                break;

                            if (index == (count - 1))
                            {
                                // virtual separator
                                break;
                            }

                            CustomizeRemoveButton(context, index);
                        }
                        break;
                    }
                }
                break;
            case IDC_ADD:
                {
                    INT index;
                    INT indexto;

                    index = ListBox_GetCurSel(context->AvailableListHandle);
                    indexto = ListBox_GetCurSel(context->CurrentListHandle);

                    if (index == LB_ERR)
                        break;

                    if (indexto == LB_ERR)
                        break;

                    CustomizeAddButton(context, index, indexto);
                }
                break;
            case IDC_REMOVE:
                {
                    INT index;

                    index = ListBox_GetCurSel(context->CurrentListHandle);

                    if (index == LB_ERR)
                        break;

                    CustomizeRemoveButton(context, index);
                }
                break;
            case IDC_MOVEUP:
                {
                    INT index;

                    index = ListBox_GetCurSel(context->CurrentListHandle);

                    if (index == LB_ERR)
                        break;

                    CustomizeMoveButton(context, index, index - 1);
                }
                break;
            case IDC_MOVEDOWN:
                {
                    INT index;

                    index = ListBox_GetCurSel(context->CurrentListHandle);

                    if (index == LB_ERR)
                        break;

                    CustomizeMoveButton(context, index, index + 1);
                }
                break;
            case IDC_RESET:
                {
                    // Reset to default settings.
                    StatusBarResetSettings();

                    // Save as the new defaults.
                    StatusBarSaveSettings();

                    StatusBarUpdate(TRUE);

                    CustomizeLoadItems(context);
                }
                break;
            case IDCANCEL:
                {
                    EndDialog(hwndDlg, FALSE);
                }
                break;
            }
        }
        break;
    case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawInfo = (LPDRAWITEMSTRUCT)lParam;

            if (drawInfo->CtlID == IDC_AVAILABLE || drawInfo->CtlID == IDC_CURRENT)
            {
                HDC bufferDc;
                HBITMAP bufferBitmap;
                HBITMAP oldBufferBitmap;
                PBUTTON_CONTEXT buttonContext;
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

                buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(drawInfo->hwndItem, drawInfo->itemID);
                if (buttonContext == NULL)
                    break;

                bufferDc = CreateCompatibleDC(drawInfo->hDC);
                bufferBitmap = CreateCompatibleBitmap(drawInfo->hDC, bufferRect.right, bufferRect.bottom);
                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);
                SelectFont(bufferDc, context->FontHandle);

                SetBkMode(bufferDc, TRANSPARENT);
                FillRect(bufferDc, &bufferRect, GetSysColorBrush(isFocused ? COLOR_HIGHLIGHT : COLOR_WINDOW));

                if (isSelected)
                {
                    FrameRect(bufferDc, &bufferRect, isFocused ? GetStockBrush(BLACK_BRUSH) : GetSysColorBrush(COLOR_HIGHLIGHT));
                }
                else
                {
                    FrameRect(bufferDc, &bufferRect, isFocused ? GetStockBrush(BLACK_BRUSH) : GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
                }

                if (!buttonContext->IsVirtual)
                {
                    SetTextColor(bufferDc, GetSysColor(isFocused ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

                    bufferRect.left += 5;

                    DrawText(
                        bufferDc,
                        StatusBarGetText(buttonContext->IdCommand),
                        -1,
                        &bufferRect,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE
                        );
                }

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

VOID StatusBarShowCustomizeDialog(
    VOID
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_CUSTOMIZE_SB),
        PhMainWndHandle,
        CustomizeDialogProc
        );
}