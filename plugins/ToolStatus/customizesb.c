/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2026
 *
 */

#include "toolstatus.h"

BOOLEAN CustomizeStatusBarItemExists(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ LONG IdCommand
    )
{
    LONG index = 0;
    LONG count = 0;
    PBUTTON_CONTEXT button;

    if ((count = ListBox_GetCount(Context->CurrentListHandle)) == LB_ERR)
        return FALSE;

    for (index = 0; index < count; index++)
    {
        if (!(button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, index)))
            continue;

        if (button->IdCommand == IdCommand)
            return TRUE;
    }

    return FALSE;
}

VOID CustomizeInsertStatusBarItem(
    _In_ LONG Index,
    _In_ PBUTTON_CONTEXT Button
    )
{
    PhInsertItemList(StatusBarItemList, Index, UlongToPtr(Button->IdCommand));

    StatusBarUpdate(TRUE);
}

VOID CustomizeAddStatusBarItem(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ LONG IndexAvail,
    _In_ LONG IndexTo
    )
{
    LONG count;
    PBUTTON_CONTEXT button;

    if ((count = ListBox_GetCount(Context->AvailableListHandle)) == LB_ERR)
        return;

    if (!(button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, IndexAvail)))
        return;

    if (!button->IsVirtual)
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
        ListBox_InsertItemData(Context->CurrentListHandle, IndexTo, button);

        CustomizeInsertStatusBarItem(IndexTo, button);
    }

    SendMessage(Context->WindowHandle, WM_COMMAND, MAKEWPARAM(IDC_AVAILABLE, LBN_SELCHANGE), 0);
}

VOID CustomizeRemoveStatusBarItem(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ LONG IndexFrom
    )
{
    PBUTTON_CONTEXT button;

    if (!(button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, IndexFrom)))
        return;

    ListBox_DeleteString(Context->CurrentListHandle, IndexFrom);
    ListBox_SetCurSel(Context->CurrentListHandle, IndexFrom);

    PhRemoveItemList(StatusBarItemList, IndexFrom);

    if (!button->IsVirtual)
    {
        LONG count = ListBox_GetCount(Context->AvailableListHandle);

        if (count == LB_ERR)
            count = 1;

        // insert into 'available' list
        ListBox_InsertItemData(Context->AvailableListHandle, count - 1, button);
    }

    SendMessage(Context->WindowHandle, WM_COMMAND, MAKEWPARAM(IDC_CURRENT, LBN_SELCHANGE), 0);

    StatusBarUpdate(TRUE);
}

VOID CustomizeMoveStatusBarItem(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ LONG IndexFrom,
    _In_ LONG IndexTo
    )
{
    LONG count;
    PBUTTON_CONTEXT button;

    if (IndexFrom == IndexTo)
        return;

    if ((count = ListBox_GetCount(Context->CurrentListHandle)) == LB_ERR)
        return;

    if (!(button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, IndexFrom)))
        return;

    ListBox_DeleteString(Context->CurrentListHandle, IndexFrom);
    ListBox_InsertItemData(Context->CurrentListHandle, IndexTo, button);
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

    CustomizeInsertStatusBarItem(IndexTo, button);
}

VOID CustomizeFreeStatusBarItems(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    LONG index = 0;
    LONG count = 0;
    PBUTTON_CONTEXT button;

    if ((count = ListBox_GetCount(Context->CurrentListHandle)) != LB_ERR)
    {
        for (index = 0; index < count; index++)
        {
            if (button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, index))
            {
                PhFree(button);
            }
        }
    }

    if ((count = ListBox_GetCount(Context->AvailableListHandle)) != LB_ERR)
    {
        for (index = 0; index < count; index++)
        {
            if (button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, index))
            {
                PhFree(button);
            }
        }
    }
}

VOID CustomizeLoadStatusBarItems(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    ULONG index;
    PBUTTON_CONTEXT button;

    CustomizeFreeStatusBarItems(Context);

    ListBox_ResetContent(Context->AvailableListHandle);
    ListBox_ResetContent(Context->CurrentListHandle);

    for (index = 0; index < StatusBarItemList->Count; index++)
    {
        button = PhAllocateZero(sizeof(BUTTON_CONTEXT));
        button->IdCommand = PtrToUlong(StatusBarItemList->Items[index]);

        ListBox_AddItemData(Context->CurrentListHandle, button);
    }

    for (index = 0; index < MAX_STATUSBAR_ITEMS; index++)
    {
        ULONG buttonId = StatusBarItems[index];

        if (CustomizeStatusBarItemExists(Context, buttonId))
            continue;

        button = PhAllocateZero(sizeof(BUTTON_CONTEXT));
        button->IdCommand = buttonId;

        ListBox_AddItemData(Context->AvailableListHandle, button);
    }

    // Append separator to the last 'current list' position
    button = PhAllocateZero(sizeof(BUTTON_CONTEXT));
    button->IsVirtual = TRUE;

    index = ListBox_AddItemData(Context->CurrentListHandle, button);
    ListBox_SetCurSel(Context->CurrentListHandle, index);
    ListBox_SetTopIndex(Context->CurrentListHandle, index);

    // Append separator to the last 'available list' position
    button = PhAllocateZero(sizeof(BUTTON_CONTEXT));
    button->IsVirtual = TRUE;

    index = ListBox_AddItemData(Context->AvailableListHandle, button);
    ListBox_SetCurSel(Context->AvailableListHandle, index);
    ListBox_SetTopIndex(Context->AvailableListHandle, 0); // NOTE: This is intentional.

    // Disable buttons
    Button_Enable(Context->MoveUpButtonHandle, FALSE);
    Button_Enable(Context->MoveDownButtonHandle, FALSE);
    Button_Enable(Context->AddButtonHandle, FALSE);
    Button_Enable(Context->RemoveButtonHandle, FALSE);
}

INT_PTR CALLBACK CustomizeStatusBarDialogProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCUSTOMIZE_CONTEXT context = NULL;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(CUSTOMIZE_CONTEXT));
        memset(context, 0, sizeof(CUSTOMIZE_CONTEXT));

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->AvailableListHandle = GetDlgItem(WindowHandle, IDC_AVAILABLE);
            context->CurrentListHandle = GetDlgItem(WindowHandle, IDC_CURRENT);
            context->MoveUpButtonHandle = GetDlgItem(WindowHandle, IDC_MOVEUP);
            context->MoveDownButtonHandle = GetDlgItem(WindowHandle, IDC_MOVEDOWN);
            context->AddButtonHandle = GetDlgItem(WindowHandle, IDC_ADD);
            context->RemoveButtonHandle = GetDlgItem(WindowHandle, IDC_REMOVE);

            context->WindowDpi = PhGetWindowDpi(WindowHandle);
            context->FontHandle = PhCreateIconTitleFont(context->WindowDpi);

            if (PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT))
            {
                context->BrushNormal = CreateSolidBrush(PhGetIntegerSetting(SETTING_THEME_WINDOW_BACKGROUND_COLOR));
                context->BrushHot = CreateSolidBrush(PhGetIntegerSetting(SETTING_THEME_WINDOW_HIGHLIGHT_COLOR));
                context->BrushPushed = CreateSolidBrush(PhGetIntegerSetting(SETTING_THEME_WINDOW_HIGHLIGHT2_COLOR));
                context->TextColor = PhGetIntegerSetting(SETTING_THEME_WINDOW_TEXT_COLOR);
            }
            else
            {
                context->BrushNormal = GetSysColorBrush(COLOR_WINDOW);
                context->BrushHot = CreateSolidBrush(RGB(145, 201, 247));
                context->BrushPushed = CreateSolidBrush(RGB(153, 209, 255));
                context->TextColor = GetSysColor(COLOR_WINDOWTEXT);
            }

            PhSetApplicationWindowIcon(WindowHandle);

            PhCenterWindow(WindowHandle, GetParent(WindowHandle));

            ListBox_SetItemHeight(context->AvailableListHandle, 0, PhScaleToDisplay(22, context->WindowDpi)); // BitmapHeight
            ListBox_SetItemHeight(context->CurrentListHandle, 0, PhScaleToDisplay(22, context->WindowDpi)); // BitmapHeight

            CustomizeLoadStatusBarItems(context);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));

            PhSetDialogFocus(context->WindowHandle, context->CurrentListHandle);
        }
        break;
    case WM_DESTROY:
        {
            StatusBarSaveSettings();

            CustomizeFreeStatusBarItems(context);

            if (context->BrushNormal)
                DeleteBrush(context->BrushNormal);

            if (context->BrushHot)
                DeleteBrush(context->BrushHot);

            if (context->BrushPushed)
                DeleteBrush(context->BrushPushed);

            if (context->FontHandle)
                DeleteFont(context->FontHandle);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            context->WindowDpi = LOWORD(wParam); // PhGetWindowDpi(WindowHandle);
            if (context->FontHandle) DeleteFont(context->FontHandle);
            context->FontHandle = PhCreateIconTitleFont(context->WindowDpi);
            ListBox_SetItemHeight(context->AvailableListHandle, 0, PhScaleToDisplay(22, context->WindowDpi)); // BitmapHeight
            ListBox_SetItemHeight(context->CurrentListHandle, 0, PhScaleToDisplay(22, context->WindowDpi)); // BitmapHeight

            InvalidateRect(context->AvailableListHandle, NULL, TRUE);
            InvalidateRect(context->CurrentListHandle, NULL, TRUE);
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
                            LONG count;
                            LONG index;

                            if ((count = ListBox_GetCount(context->AvailableListHandle)) == LB_ERR)
                                break;

                            if ((index = ListBox_GetCurSel(context->AvailableListHandle)) == LB_ERR)
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
                            LONG count;
                            LONG index;
                            LONG indexto;

                            if ((count = ListBox_GetCount(context->AvailableListHandle)) == LB_ERR)
                                break;

                            if ((index = ListBox_GetCurSel(context->AvailableListHandle)) == LB_ERR)
                                break;

                            if ((indexto = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                                break;

                            if (index == (count - 1))
                            {
                                // virtual separator
                                break;
                            }

                            CustomizeAddStatusBarItem(context, index, indexto);
                        }
                        break;
                    //case LBN_KILLFOCUS:
                    //    {
                    //        Button_Enable(context->AddButtonHandle, FALSE);
                    //    }
                    //    break;
                    }
                }
                break;
            case IDC_CURRENT:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                    case LBN_SELCHANGE:
                        {
                            LONG count;
                            LONG index;
                            PBUTTON_CONTEXT button;

                            if ((count = ListBox_GetCount(context->CurrentListHandle)) == LB_ERR)
                                break;

                            if ((index = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                                break;

                            button = (PBUTTON_CONTEXT)ListBox_GetItemData(context->CurrentListHandle, index);
                            if (button == NULL)
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

                            Button_Enable(context->RemoveButtonHandle, !button->IsVirtual);
                        }
                        break;
                    case LBN_DBLCLK:
                        {
                            LONG count;
                            LONG index;

                            if ((count = ListBox_GetCount(context->CurrentListHandle)) == LB_ERR)
                                break;

                            if ((index = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                                break;

                            if (index == (count - 1))
                            {
                                // virtual separator
                                break;
                            }

                            CustomizeRemoveStatusBarItem(context, index);
                        }
                        break;
                    //case LBN_KILLFOCUS:
                    //    {
                    //        Button_Enable(context->MoveUpButtonHandle, FALSE);
                    //        Button_Enable(context->MoveDownButtonHandle, FALSE);
                    //        Button_Enable(context->RemoveButtonHandle, FALSE);
                    //    }
                    //    break;
                    }
                }
                break;
            case IDC_ADD:
                {
                    LONG index;
                    LONG indexto;

                    if ((index = ListBox_GetCurSel(context->AvailableListHandle)) == LB_ERR)
                        break;

                    if ((indexto = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                        break;

                    CustomizeAddStatusBarItem(context, index, indexto);
                }
                break;
            case IDC_REMOVE:
                {
                    LONG index;

                    if ((index = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                        break;

                    CustomizeRemoveStatusBarItem(context, index);
                }
                break;
            case IDC_MOVEUP:
                {
                    LONG index;

                    if ((index = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                        break;

                    CustomizeMoveStatusBarItem(context, index, index - 1);
                }
                break;
            case IDC_MOVEDOWN:
                {
                    LONG index;

                    if ((index = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                        break;

                    CustomizeMoveStatusBarItem(context, index, index + 1);
                }
                break;
            case IDC_RESET:
                {
                    // Reset to default settings.
                    StatusBarResetSettings();

                    // Save as the new defaults.
                    StatusBarSaveSettings();

                    StatusBarUpdate(TRUE);

                    CustomizeLoadStatusBarItems(context);
                }
                break;
            case IDCANCEL:
                {
                    EndDialog(WindowHandle, IDCANCEL);
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
                PBUTTON_CONTEXT button;
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

                if (!(button = (PBUTTON_CONTEXT)ListBox_GetItemData(drawInfo->hwndItem, drawInfo->itemID)))
                    break;

                bufferDc = CreateCompatibleDC(drawInfo->hDC);
                bufferBitmap = CreateCompatibleBitmap(drawInfo->hDC, bufferRect.right, bufferRect.bottom);

                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);
                SelectFont(bufferDc, context->FontHandle);
                SetBkMode(bufferDc, TRANSPARENT);

                if (isSelected || isFocused)
                    FillRect(bufferDc, &bufferRect, context->BrushHot);
                else
                    FillRect(bufferDc, &bufferRect, context->BrushNormal);

                if (!button->IsVirtual)
                    SetTextColor(bufferDc, context->TextColor);
                else
                    SetTextColor(bufferDc, GetSysColor(COLOR_GRAYTEXT));

                if (!button->IsVirtual)
                {
                    bufferRect.left += 5;
                    DrawText(
                        bufferDc,
                        StatusBarGetText(button->IdCommand),
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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID StatusBarShowCustomizeDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_CUSTOMIZE_SB),
        ParentWindowHandle,
        CustomizeStatusBarDialogProc,
        NULL
        );
}
