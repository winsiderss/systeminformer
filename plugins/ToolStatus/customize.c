/*
* Process Hacker ToolStatus -
*   Toolbar Customize Dialog
*
* Copyright (C) 2015 dmex
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

typedef struct _BUTTON_CONTEXT
{
    INT IdCommand;
    INT IdBitmap;
    BOOLEAN IsVirtual;
    BOOLEAN IsRemovable;
    BOOLEAN IsSeperator;
} BUTTON_CONTEXT, *PBUTTON_CONTEXT;

typedef struct _TBCUSTOMIZE_CONTEXT
{
    HWND DialogHandle;
    HWND AvailableListHandle;
    HWND CurrentListHandle;
    HWND MoveUpButtonHandle;
    HWND MoveDownButtonHandle;
    HWND RemoveButtonHandle;

    INT BitmapWidth;
    HFONT Font;
} TBCUSTOMIZE_CONTEXT, *PTBCUSTOMIZE_CONTEXT;


static BOOLEAN CustomizeItemExists(
    _In_ PTBCUSTOMIZE_CONTEXT Context, 
    _In_ INT idCommand
    )
{
    INT buttonIndex = 0;
    INT buttonCount = 0;

    buttonCount = ListBox_GetCount(Context->CurrentListHandle);

    for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
    {
        PBUTTON_CONTEXT buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, buttonIndex);

        if (buttonContext->IdCommand == idCommand)
            return TRUE;
    }

    return FALSE;
}

static VOID CustomizeInsertToolbarButton(
    _In_ INT Index,
    _In_ PBUTTON_CONTEXT ButtonContext
    )
{
    TBBUTTON button;

    memset(&button, 0, sizeof(TBBUTTON));

    button.iBitmap = ButtonContext->IdBitmap;
    button.idCommand = ButtonContext->IdCommand;
    button.fsState = TBSTATE_ENABLED;
    button.fsStyle = ButtonContext->IsSeperator ? BTNS_SEP : BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT; // Initial state (gets reset by ToolbarLoadSettings)
    button.iString = (INT_PTR)ToolbarGetText(ButtonContext->IdCommand);

    SendMessage(ToolBarHandle, TB_INSERTBUTTON, Index, (LPARAM)&button);
}

static VOID CustomizeAddButton(
    _In_ PTBCUSTOMIZE_CONTEXT Context, 
    _In_ INT IndexAvail,
    _In_ INT IndexTo
    )
{
    INT count;
    PBUTTON_CONTEXT buttonContext;

    count = ListBox_GetCount(Context->AvailableListHandle);
    buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, IndexAvail);

    if (IndexAvail != 0) // index 0 is separator
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
    }
    else
    {
        buttonContext = PhAllocate(sizeof(BUTTON_CONTEXT));
        memset(buttonContext, 0, sizeof(BUTTON_CONTEXT));

        buttonContext->IsSeperator = TRUE;
        buttonContext->IsRemovable = TRUE;
    }

    // insert into 'current' list 
    ListBox_InsertItemData(Context->CurrentListHandle, IndexTo, buttonContext);

    CustomizeInsertToolbarButton(IndexTo, buttonContext);
}

static VOID CustomizeRemoveButton(
    _In_ PTBCUSTOMIZE_CONTEXT Context, 
    _In_ INT IndexFrom
    )
{
    PBUTTON_CONTEXT buttonContext;

    buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, IndexFrom);

    ListBox_DeleteString(Context->CurrentListHandle, IndexFrom);
    ListBox_SetCurSel(Context->CurrentListHandle, IndexFrom);

    SendMessage(ToolBarHandle, TB_DELETEBUTTON, IndexFrom, 0);

    if (buttonContext->IsSeperator)
    {
        PhFree(buttonContext); 
    }
    else
    {
        // insert into 'available' list
        ListBox_AddItemData(Context->AvailableListHandle, buttonContext);
    }

    SendMessage(Context->DialogHandle, WM_COMMAND, MAKEWPARAM(IDC_CURRENT, LBN_SELCHANGE), 0);
}

static VOID CustomizeMoveButton(
    _In_ PTBCUSTOMIZE_CONTEXT Context,
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

    SendMessage(ToolBarHandle, TB_DELETEBUTTON, IndexFrom, 0);

    CustomizeInsertToolbarButton(IndexTo, buttonContext);
}

static VOID CustomizeFreeButtons(
    _In_ PTBCUSTOMIZE_CONTEXT Context
    )
{
    INT buttonIndex = 0;
    INT buttonCount = 0;

    buttonCount = ListBox_GetCount(Context->CurrentListHandle);

    for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
    {
        PBUTTON_CONTEXT buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, buttonIndex);

        if (buttonContext)
        {
            PhFree(buttonContext);
        }
    }

    buttonCount = ListBox_GetCount(Context->AvailableListHandle);

    for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
    {
        PBUTTON_CONTEXT buttonContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, buttonIndex);

        if (buttonContext)
        {
            PhFree(buttonContext);
        }
    }
}


static VOID CustomizeLoadItems(
    _In_ PTBCUSTOMIZE_CONTEXT Context
    )
{
    INT buttonIndex = 0;
    INT buttonCount = 0;

    CustomizeFreeButtons(Context);

    ListBox_ResetContent(Context->AvailableListHandle);
    ListBox_ResetContent(Context->CurrentListHandle);

    buttonCount = (INT)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);

    for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
    {
        TBBUTTON button;

        if (SendMessage(ToolBarHandle, TB_GETBUTTON, buttonIndex, (LPARAM)&button))
        {
            PBUTTON_CONTEXT buttonContext = PhAllocate(sizeof(BUTTON_CONTEXT));
            memset(buttonContext, 0, sizeof(BUTTON_CONTEXT));

            buttonContext->IsVirtual = FALSE;
            buttonContext->IsRemovable = TRUE;
            buttonContext->IdCommand = button.idCommand;
            buttonContext->IdBitmap = button.iBitmap;

            if (button.fsStyle & BTNS_SEP)
            {
                buttonContext->IsSeperator = TRUE;
            }

            ListBox_AddItemData(Context->CurrentListHandle, buttonContext);
        }
    }

    for (buttonIndex = 0; buttonIndex < ARRAYSIZE(ToolbarButtons); buttonIndex++)
    {
        TBBUTTON button = ToolbarButtons[buttonIndex];

        if (button.idCommand == 0)
            continue;

        if (CustomizeItemExists(Context, button.idCommand))
            continue;

        PBUTTON_CONTEXT buttonContext = PhAllocate(sizeof(BUTTON_CONTEXT));
        memset(buttonContext, 0, sizeof(BUTTON_CONTEXT));

        buttonContext->IsRemovable = TRUE;
        buttonContext->IdCommand = button.idCommand;
        buttonContext->IdBitmap = button.iBitmap;

        ListBox_AddItemData(Context->AvailableListHandle, buttonContext);
    }

    {
        // append separator to the last 'current list'  position
        PBUTTON_CONTEXT buttonContext = PhAllocate(sizeof(BUTTON_CONTEXT));
        memset(buttonContext, 0, sizeof(BUTTON_CONTEXT));
        buttonContext->IsSeperator = TRUE;
        buttonContext->IsVirtual = TRUE;
        buttonContext->IsRemovable = FALSE;

        INT index = ListBox_AddItemData(Context->CurrentListHandle, buttonContext);
        ListBox_SetCurSel(Context->CurrentListHandle, index);
        ListBox_SetTopIndex(Context->CurrentListHandle, index);
    }

    {
        // insert separator into first 'available list' position
        PBUTTON_CONTEXT buttonContext = PhAllocate(sizeof(BUTTON_CONTEXT));
        memset(buttonContext, 0, sizeof(BUTTON_CONTEXT));
        buttonContext->IsSeperator = TRUE;
        buttonContext->IsVirtual = TRUE;
        buttonContext->IsRemovable = FALSE;

        ListBox_InsertItemData(Context->AvailableListHandle, 0, buttonContext);
        ListBox_SetCurSel(Context->AvailableListHandle, 0);
    }

    // set focus and disable buttons
    Button_Enable(Context->MoveUpButtonHandle, FALSE);
    Button_Enable(Context->MoveDownButtonHandle, FALSE);
    Button_Enable(Context->RemoveButtonHandle, FALSE);
}

static VOID CustomizeLoadSettings(
    _In_ PTBCUSTOMIZE_CONTEXT Context
    )
{
    HWND toolbarCombo = GetDlgItem(Context->DialogHandle, IDC_TEXTOPTIONS);
    HWND searchboxCombo = GetDlgItem(Context->DialogHandle, IDC_SEARCHOPTIONS);

    ComboBox_AddString(toolbarCombo, L"No Text Labels"); // Displays no text label for the toolbar buttons.
    ComboBox_AddString(toolbarCombo, L"Selective Text"); // (Selective Text On Right) Displays text for just the Refresh, Options, Find Handles and Sysinfo toolbar buttons.
    ComboBox_AddString(toolbarCombo, L"Show Text Labels"); // Displays text labels for the toolbar buttons.
    ComboBox_SetCurSel(toolbarCombo, PhGetIntegerSetting(SETTING_NAME_TOOLBARDISPLAYSTYLE));

    ComboBox_AddString(searchboxCombo, L"Always show");
    ComboBox_AddString(searchboxCombo, L"Hide when inactive (Ctrl+K)");
    //ComboBox_AddString(searchboxCombo, L"Auto-hide");
    ComboBox_SetCurSel(searchboxCombo, PhGetIntegerSetting(SETTING_NAME_SEARCHBOXDISPLAYMODE));

    Button_SetCheck(GetDlgItem(Context->DialogHandle, IDC_ENABLE_MODERN),
        PhGetIntegerSetting(SETTING_NAME_ENABLE_MODERNICONS) ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(Context->DialogHandle, IDC_ENABLE_AUTOHIDE_MENU),
        PhGetIntegerSetting(SETTING_NAME_ENABLE_AUTOHIDE_MENU) ? BST_CHECKED : BST_UNCHECKED);
    
    if (!EnableSearchBox)
    {
        ComboBox_Enable(searchboxCombo, FALSE);
    }
}


INT_PTR CALLBACK ToolbarCustomizeDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PTBCUSTOMIZE_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PTBCUSTOMIZE_CONTEXT)PhAllocate(sizeof(TBCUSTOMIZE_CONTEXT));
        memset(context, 0, sizeof(TBCUSTOMIZE_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PTBCUSTOMIZE_CONTEXT)GetProp(hwndDlg, L"Context");

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
            context->RemoveButtonHandle = GetDlgItem(hwndDlg, IDC_REMOVE);
            context->BitmapWidth = GetSystemMetrics(SM_CYSMICON) + 4;
            context->Font = (HFONT)SendMessage(ToolBarHandle, WM_GETFONT, 0, 0);

            ListBox_SetItemHeight(context->AvailableListHandle, 0, context->BitmapWidth); // BitmapHeight
            ListBox_SetItemHeight(context->CurrentListHandle, 0, context->BitmapWidth); // BitmapHeight 

            CustomizeLoadItems(context);
            CustomizeLoadSettings(context);

            SendMessage(context->DialogHandle, WM_NEXTDLGCTL, (WPARAM)context->CurrentListHandle, TRUE);
        }
        return TRUE;
    case WM_DESTROY:
        {
            ToolbarSaveButtonSettings();
            ToolbarLoadSettings();

            CustomizeFreeButtons(context);
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
                    case LBN_DBLCLK:
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

                            Button_Enable(context->RemoveButtonHandle, buttonContext->IsRemovable);
                        }
                        break;
                    case LBN_DBLCLK:
                        {
                            INT count;
                            INT index;

                            count = ListBox_GetCount(context->CurrentListHandle);
                            index = ListBox_GetCurSel(context->CurrentListHandle);

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
                    // Reset the Toolbar buttons to default settings.
                    ToolbarResetSettings();
                    // Re-load the settings.
                    ToolbarLoadSettings();
                    // Save as the new defaults.
                    ToolbarSaveButtonSettings();

                    CustomizeLoadItems(context);
                }
                break;
            case IDC_TEXTOPTIONS:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        PhSetIntegerSetting(SETTING_NAME_TOOLBARDISPLAYSTYLE,
                            (DisplayStyle = (TOOLBAR_DISPLAY_STYLE)ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_TEXTOPTIONS))));

                        ToolbarLoadSettings();
                    }
                }
                break;
            case IDC_SEARCHOPTIONS:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        PhSetIntegerSetting(SETTING_NAME_SEARCHBOXDISPLAYMODE,
                            (SearchBoxDisplayMode = (SEARCHBOX_DISPLAY_MODE)ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_SEARCHOPTIONS))));

                        ToolbarLoadSettings();
                    }
                }
                break;
            case IDC_ENABLE_MODERN:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    {
                        PhSetIntegerSetting(SETTING_NAME_ENABLE_MODERNICONS,
                            Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_MODERN)) == BST_CHECKED);

                        ToolbarLoadSettings();

                        //CustomizeLoadItems(context);
                        InvalidateRect(context->AvailableListHandle, NULL, TRUE);
                        InvalidateRect(context->CurrentListHandle, NULL, TRUE);
                    }
                }
                break;
            case IDC_ENABLE_AUTOHIDE_MENU:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    {
                        AutoHideMenu = !AutoHideMenu;

                        PhSetIntegerSetting(SETTING_NAME_ENABLE_AUTOHIDE_MENU, AutoHideMenu);

                        if (AutoHideMenu)
                        {
                            MainMenu = GetMenu(PhMainWndHandle);
                            SetMenu(PhMainWndHandle, NULL);
                        }
                        else
                        {
                            SetMenu(PhMainWndHandle, MainMenu);
                            DrawMenuBar(PhMainWndHandle);
                        }
                    }
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
                SelectFont(bufferDc, context->Font);

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

                if (buttonContext->IsVirtual)
                {
                    SetTextColor(bufferDc, GetSysColor(COLOR_GRAYTEXT));
                }
                else
                {
                    SetTextColor(bufferDc, GetSysColor(isFocused ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
                }

                if (!buttonContext->IsSeperator)
                {
                    ImageList_Draw(
                        ToolBarImageList,
                        buttonContext->IdBitmap,
                        bufferDc,
                        bufferRect.left + 2,
                        bufferRect.top + 2,
                        ILD_NORMAL
                        );
                }

                bufferRect.left += context->BitmapWidth + 2;

                if (buttonContext->IdCommand != 0)
                {
                    DrawText(
                        bufferDc, 
                        ToolbarGetText(buttonContext->IdCommand), 
                        -1,
                        &bufferRect, 
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE
                        );
                }
                else
                {
                    DrawText(
                        bufferDc, 
                        L"Seperator", 
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

VOID ShowCustomizeDialog(
    VOID
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_CUSTOMIZE),
        PhMainWndHandle,
        ToolbarCustomizeDialogProc
        );
}