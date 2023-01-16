/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2023
 *
 */

#include "toolstatus.h"
#include "commonutil.h"

static PWSTR CustomizeTextOptionsStrings[] =
{
    L"No text labels",
    L"Selective text",
    L"Show text labels"
};

static PWSTR CustomizeSearchDisplayStrings[] =
{
    L"Always show",
    L"Hide when inactive (Ctrl+K)",
    // L"Auto-hide"
};

BOOLEAN CustomizeToolbarItemExists(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IdCommand
    )
{
    INT index = 0;
    INT count = 0;
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

VOID CustomizeInsertToolbarButton(
    _In_ INT Index,
    _In_ PBUTTON_CONTEXT ItemContext
    )
{
    TBBUTTON button;

    memset(&button, 0, sizeof(TBBUTTON));

    button.idCommand = ItemContext->IdCommand;
    button.iBitmap = I_IMAGECALLBACK;
    button.fsState = TBSTATE_ENABLED;
    button.iString = (INT_PTR)ToolbarGetText(ItemContext->IdCommand);

    if (ItemContext->IsSeparator)
    {
        button.fsStyle = BTNS_SEP;
    }
    else
    {
        switch (DisplayStyle)
        {
        case TOOLBAR_DISPLAY_STYLE_IMAGEONLY:
            button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
            break;
        case TOOLBAR_DISPLAY_STYLE_SELECTIVETEXT:
            {
                switch (button.idCommand)
                {
                case PHAPP_ID_VIEW_REFRESH:
                case PHAPP_ID_HACKER_OPTIONS:
                case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
                case PHAPP_ID_VIEW_SYSTEMINFORMATION:
                    button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
                    break;
                default:
                    button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
                    break;
                }
            }
            break;
        case TOOLBAR_DISPLAY_STYLE_ALLTEXT:
            button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
            break;
        }
    }

    SendMessage(ToolBarHandle, TB_INSERTBUTTON, Index, (LPARAM)&button);
}

VOID CustomizeAddToolbarItem(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IndexAvail,
    _In_ INT IndexTo
    )
{
    INT count;
    PBUTTON_CONTEXT button;

    if ((count = ListBox_GetCount(Context->AvailableListHandle)) == LB_ERR)
        return;

    if (!(button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, IndexAvail)))
        return;

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
        button = PhAllocateZero(sizeof(BUTTON_CONTEXT));
        button->IsSeparator = TRUE;
        button->IsRemovable = TRUE;
    }

    // insert into 'current' list
    ListBox_InsertItemData(Context->CurrentListHandle, IndexTo, button);

    CustomizeInsertToolbarButton(IndexTo, button);
}

VOID CustomizeRemoveToolbarItem(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IndexFrom
    )
{
    PBUTTON_CONTEXT button;

    if (!(button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, IndexFrom)))
        return;

    ListBox_DeleteString(Context->CurrentListHandle, IndexFrom);
    ListBox_SetCurSel(Context->CurrentListHandle, IndexFrom);

    SendMessage(ToolBarHandle, TB_DELETEBUTTON, IndexFrom, 0);

    if (button->IsSeparator)
    {
        PhFree(button);
    }
    else
    {
        // insert into 'available' list
        ListBox_AddItemData(Context->AvailableListHandle, button);
    }

    SendMessage(Context->WindowHandle, WM_COMMAND, MAKEWPARAM(IDC_CURRENT, LBN_SELCHANGE), 0);
}

VOID CustomizeMoveToolbarItem(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IndexFrom,
    _In_ INT IndexTo
    )
{
    INT count;
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

    SendMessage(ToolBarHandle, TB_DELETEBUTTON, IndexFrom, 0);

    CustomizeInsertToolbarButton(IndexTo, button);
}

VOID CustomizeFreeToolbarItems(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    INT i = 0;
    INT count = 0;
    PBUTTON_CONTEXT button;

    if ((count = ListBox_GetCount(Context->CurrentListHandle)) != LB_ERR)
    {
        for (i = 0; i < count; i++)
        {
            if (button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, i))
            {
                if (button->IconHandle)
                {
                    DestroyIcon(button->IconHandle);
                }

                PhFree(button);
            }
        }
    }

    if ((count = ListBox_GetCount(Context->AvailableListHandle)) != LB_ERR)
    {
        for (i = 0; i < count; i++)
        {
            if (button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, i))
            {
                if (button->IconHandle)
                {
                    DestroyIcon(button->IconHandle);
                }

                PhFree(button);
            }
        }
    }
}

VOID CustomizeLoadToolbarItems(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    INT index = 0;
    INT count = 0;
    PBUTTON_CONTEXT context;

    CustomizeFreeToolbarItems(Context);

    ListBox_ResetContent(Context->AvailableListHandle);
    ListBox_ResetContent(Context->CurrentListHandle);

    count = (INT)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);

    for (index = 0; index < count; index++)
    {
        TBBUTTON button;

        memset(&button, 0, sizeof(TBBUTTON));

        if (SendMessage(ToolBarHandle, TB_GETBUTTON, index, (LPARAM)&button))
        {
            context = PhAllocateZero(sizeof(BUTTON_CONTEXT));
            context->IsVirtual = FALSE;
            context->IsRemovable = TRUE;
            context->IdCommand = button.idCommand;

            if (button.fsStyle & BTNS_SEP)
            {
                context->IsSeparator = TRUE;
            }
            else
            {
                context->IconHandle = CustomizeGetToolbarIcon(Context, button.idCommand, Context->WindowDpi);
            }

            ListBox_AddItemData(Context->CurrentListHandle, context);
        }
    }

    for (index = 0; index < MAX_TOOLBAR_ITEMS; index++)
    {
        TBBUTTON button = ToolbarButtons[index];

        if (button.idCommand == 0)
            continue;

        if (CustomizeToolbarItemExists(Context, button.idCommand))
            continue;

        context = PhAllocateZero(sizeof(BUTTON_CONTEXT));
        context->IsRemovable = TRUE;
        context->IdCommand = button.idCommand;
        context->IconHandle = CustomizeGetToolbarIcon(Context, button.idCommand, Context->WindowDpi);

        ListBox_AddItemData(Context->AvailableListHandle, context);
    }

    // Append separator to the last 'current list'  position
    context = PhAllocateZero(sizeof(BUTTON_CONTEXT));
    context->IsSeparator = TRUE;
    context->IsVirtual = TRUE;
    context->IsRemovable = FALSE;

    index = ListBox_AddItemData(Context->CurrentListHandle, context);
    ListBox_SetCurSel(Context->CurrentListHandle, index);
    ListBox_SetTopIndex(Context->CurrentListHandle, index);

    // Insert separator into first 'available list' position
    context = PhAllocateZero(sizeof(BUTTON_CONTEXT));
    context->IsSeparator = TRUE;
    context->IsVirtual = FALSE;
    context->IsRemovable = FALSE;

    index = ListBox_InsertItemData(Context->AvailableListHandle, 0, context);
    ListBox_SetCurSel(Context->AvailableListHandle, index);
    ListBox_SetTopIndex(Context->AvailableListHandle, index);

    // Disable buttons
    Button_Enable(Context->MoveUpButtonHandle, FALSE);
    Button_Enable(Context->MoveDownButtonHandle, FALSE);
    Button_Enable(Context->AddButtonHandle, FALSE);
    Button_Enable(Context->RemoveButtonHandle, FALSE);
}

VOID CustomizeLoadToolbarSettings(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    HWND toolbarCombo = GetDlgItem(Context->WindowHandle, IDC_TEXTOPTIONS);
    HWND searchboxCombo = GetDlgItem(Context->WindowHandle, IDC_SEARCHOPTIONS);

    PhAddComboBoxStrings(
        toolbarCombo,
        CustomizeTextOptionsStrings,
        ARRAYSIZE(CustomizeTextOptionsStrings)
        );
    PhAddComboBoxStrings(
        searchboxCombo,
        CustomizeSearchDisplayStrings,
        ARRAYSIZE(CustomizeSearchDisplayStrings)
        );
    ComboBox_SetCurSel(toolbarCombo, PhGetIntegerSetting(SETTING_NAME_TOOLBARDISPLAYSTYLE));
    ComboBox_SetCurSel(searchboxCombo, PhGetIntegerSetting(SETTING_NAME_SEARCHBOXDISPLAYMODE));

    Button_SetCheck(GetDlgItem(Context->WindowHandle, IDC_ENABLE_MODERN),
        ToolStatusConfig.ModernIcons ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(Context->WindowHandle, IDC_ENABLE_AUTOHIDE_MENU),
        ToolStatusConfig.AutoHideMenu ? BST_CHECKED : BST_UNCHECKED);

    if (!ToolStatusConfig.SearchBoxEnabled)
    {
        ComboBox_Enable(searchboxCombo, FALSE);
    }
}

VOID CustomizeResetImages(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    INT index = 0;
    INT count = 0;
    PBUTTON_CONTEXT button;

    if ((count = ListBox_GetCount(Context->CurrentListHandle)) != LB_ERR)
    {
        for (index = 0; index < count; index++)
        {
            if (button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, index))
            {
                if (button->IdCommand != 0)
                {
                    if (button->IconHandle)
                    {
                        DestroyIcon(button->IconHandle);
                        button->IconHandle = NULL;
                    }

                    button->IconHandle = CustomizeGetToolbarIcon(Context, button->IdCommand, Context->WindowDpi);
                }
            }
        }
    }

    if ((count = ListBox_GetCount(Context->AvailableListHandle)) != LB_ERR)
    {
        for (index = 0; index < count; index++)
        {
            if (button = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, index))
            {
                if (button->IdCommand != 0)
                {
                    if (button->IconHandle)
                    {
                        DestroyIcon(button->IconHandle);
                        button->IconHandle = NULL;
                    }

                    button->IconHandle = CustomizeGetToolbarIcon(Context, button->IdCommand, Context->WindowDpi);
                }
            }
        }
    }

    InvalidateRect(Context->AvailableListHandle, NULL, TRUE);
    InvalidateRect(Context->CurrentListHandle, NULL, TRUE);
}

HICON CustomizeGetToolbarIcon(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT CommandID,
    _In_ LONG DpiValue
    )
{
    HICON iconHandle = NULL;
    HBITMAP bitmapHandle;

    if (bitmapHandle = ToolbarGetImage(CommandID, DpiValue))
    {
        iconHandle = CommonBitmapToIcon(bitmapHandle, ToolBarImageSize.cx, ToolBarImageSize.cy);
        DeleteBitmap(bitmapHandle);
    }

    return iconHandle;
}

VOID CustomizeResetToolbarImages(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    // Reset the image cache with the new icons.
    // TODO: Move function to Toolbar.c
    for (UINT index = 0; index < RTL_NUMBER_OF(ToolbarButtons); index++)
    {
        if (ToolbarButtons[index].iBitmap != I_IMAGECALLBACK && !FlagOn(ToolbarButtons[index].fsStyle, BTNS_SEP))
        {
            HBITMAP bitmap;

            if (bitmap = ToolbarGetImage(ToolbarButtons[index].idCommand, Context->WindowDpi))
            {
                PhImageListReplace(
                    ToolBarImageList,
                    ToolbarButtons[index].iBitmap,
                    bitmap,
                    NULL
                    );

                DeleteBitmap(bitmap);
            }
        }
    }
}

INT_PTR CALLBACK CustomizeToolbarDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCUSTOMIZE_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(CUSTOMIZE_CONTEXT));
        memset(context, 0, sizeof(CUSTOMIZE_CONTEXT));

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhSetApplicationWindowIcon(hwndDlg);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            context->WindowHandle = hwndDlg;
            context->AvailableListHandle = GetDlgItem(hwndDlg, IDC_AVAILABLE);
            context->CurrentListHandle = GetDlgItem(hwndDlg, IDC_CURRENT);
            context->MoveUpButtonHandle = GetDlgItem(hwndDlg, IDC_MOVEUP);
            context->MoveDownButtonHandle = GetDlgItem(hwndDlg, IDC_MOVEDOWN);
            context->AddButtonHandle = GetDlgItem(hwndDlg, IDC_ADD);
            context->RemoveButtonHandle = GetDlgItem(hwndDlg, IDC_REMOVE);
            context->FontHandle = PhDuplicateFont(GetWindowFont(ToolBarHandle));

            context->WindowDpi = PhGetWindowDpi(hwndDlg);
            context->CXWidth = PhGetDpi(16, context->WindowDpi);

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

            ListBox_SetItemHeight(context->AvailableListHandle, 0, context->CXWidth + 6); // BitmapHeight
            ListBox_SetItemHeight(context->CurrentListHandle, 0, context->CXWidth + 6); // BitmapHeight

            CustomizeLoadToolbarItems(context);
            CustomizeLoadToolbarSettings(context);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            PhSetDialogFocus(context->WindowHandle, context->CurrentListHandle);
        }
        break;
    case WM_DESTROY:
        {
            ToolbarSaveButtonSettings();
            ToolbarLoadSettings(FALSE);
            CustomizeFreeToolbarItems(context);

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
    case WM_DPICHANGED:
        {
            context->WindowDpi = LOWORD(wParam); //PhGetWindowDpi(hwndDlg);
            context->CXWidth = PhGetDpi(16, context->WindowDpi);

            ListBox_SetItemHeight(context->AvailableListHandle, 0, context->CXWidth + 6); // BitmapHeight
            ListBox_SetItemHeight(context->CurrentListHandle, 0, context->CXWidth + 6); // BitmapHeight

            {
                LONG dpi = PhGetWindowDpi(PhMainWndHandle);
                ToolBarImageSize.cx = PhGetSystemMetrics(SM_CXSMICON, dpi);
                ToolBarImageSize.cy = PhGetSystemMetrics(SM_CYSMICON, dpi);
            }

            CustomizeResetImages(context);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
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
                            Button_Enable(context->AddButtonHandle, TRUE);
                        }
                        break;
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

                            CustomizeAddToolbarItem(context, index, indexto);
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
                            INT count;
                            INT index;
                            PBUTTON_CONTEXT itemContext;

                            if ((count = ListBox_GetCount(context->CurrentListHandle)) == LB_ERR)
                                break;

                            if ((index = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                                break;

                            if (!(itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(context->CurrentListHandle, index)))
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

                            Button_Enable(context->RemoveButtonHandle, itemContext->IsRemovable);
                        }
                        break;
                    case LBN_DBLCLK:
                        {
                            INT count;
                            INT index;

                            if ((count = ListBox_GetCount(context->CurrentListHandle)) == LB_ERR)
                                break;

                            if ((index = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                                break;

                            if (index == (count - 1))
                            {
                                // virtual separator
                                break;
                            }

                            CustomizeRemoveToolbarItem(context, index);
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
                    INT index;
                    INT indexto;

                    if ((index = ListBox_GetCurSel(context->AvailableListHandle)) == LB_ERR)
                        break;

                    if ((indexto = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                        break;

                    CustomizeAddToolbarItem(context, index, indexto);
                }
                break;
            case IDC_REMOVE:
                {
                    INT index;

                    if ((index = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                        break;

                    CustomizeRemoveToolbarItem(context, index);
                }
                break;
            case IDC_MOVEUP:
                {
                    INT index;

                    if ((index = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                        break;

                    CustomizeMoveToolbarItem(context, index, index - 1);
                }
                break;
            case IDC_MOVEDOWN:
                {
                    INT index;

                    if ((index = ListBox_GetCurSel(context->CurrentListHandle)) == LB_ERR)
                        break;

                    CustomizeMoveToolbarItem(context, index, index + 1);
                }
                break;
            case IDC_RESET:
                {
                    // Reset the Toolbar buttons to default settings.
                    ToolbarResetSettings();
                    // Re-load the settings.
                    ToolbarLoadSettings(FALSE);
                    // Save as the new defaults.
                    ToolbarSaveButtonSettings();

                    CustomizeLoadToolbarItems(context);
                }
                break;
            case IDC_TEXTOPTIONS:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        PhSetIntegerSetting(SETTING_NAME_TOOLBARDISPLAYSTYLE,
                            (DisplayStyle = (TOOLBAR_DISPLAY_STYLE)ComboBox_GetCurSel(GET_WM_COMMAND_HWND(wParam, lParam))));

                        ToolbarLoadSettings(FALSE);
                    }
                }
                break;
            case IDC_SEARCHOPTIONS:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        PhSetIntegerSetting(SETTING_NAME_SEARCHBOXDISPLAYMODE,
                            (SearchBoxDisplayMode = (SEARCHBOX_DISPLAY_MODE)ComboBox_GetCurSel(GET_WM_COMMAND_HWND(wParam, lParam))));

                        ToolbarLoadSettings(FALSE);
                    }
                }
                break;
            //case IDC_THEMEOPTIONS:
            //    {
            //        if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
            //        {
            //            PhSetIntegerSetting(SETTING_NAME_TOOLBAR_THEME,
            //                (ToolBarTheme = (TOOLBAR_THEME)ComboBox_GetCurSel(GET_WM_COMMAND_HWND(wParam, lParam))));
            //
            //            switch (ToolBarTheme)
            //            {
            //            case TOOLBAR_THEME_NONE:
            //                {
            //                    SendMessage(RebarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"");
            //                    SendMessage(ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"");
            //                }
            //                break;
            //            case TOOLBAR_THEME_BLACK:
            //                {
            //                    SendMessage(RebarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"Media");
            //                    SendMessage(ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"Media");
            //                }
            //                break;
            //            case TOOLBAR_THEME_BLUE:
            //                {
            //                    SendMessage(RebarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"Communications");
            //                    SendMessage(ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"Communications");
            //                }
            //                break;
            //            }
            //        }
            //    }
            //    break;
            case IDC_ENABLE_MODERN:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    {
                        ToolStatusConfig.ModernIcons = Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED;

                        PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                        ToolbarLoadSettings(FALSE);

                        CustomizeResetImages(context);
                        CustomizeResetToolbarImages(context);
                        //CustomizeLoadItems(context);
                    }
                }
                break;
            case IDC_ENABLE_AUTOHIDE_MENU:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    {
                        ToolStatusConfig.AutoHideMenu = !ToolStatusConfig.AutoHideMenu;

                        PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                        if (ToolStatusConfig.AutoHideMenu)
                        {
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
                PBUTTON_CONTEXT itemContext;
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

                if (!(itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(drawInfo->hwndItem, drawInfo->itemID)))
                    break;

                bufferDc = CreateCompatibleDC(drawInfo->hDC);
                bufferBitmap = CreateCompatibleBitmap(drawInfo->hDC, bufferRect.right, bufferRect.bottom);

                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);
                SelectFont(bufferDc, context->FontHandle);
                SetBkMode(bufferDc, TRANSPARENT);

                if (isFocused || isSelected)
                    FillRect(bufferDc, &bufferRect, context->BrushHot);
                else
                    FillRect(bufferDc, &bufferRect, context->BrushNormal);

                if (!itemContext->IsVirtual)
                    SetTextColor(bufferDc, context->TextColor);
                else
                    SetTextColor(bufferDc, GetSysColor(COLOR_GRAYTEXT));

                if (itemContext->IconHandle && !itemContext->IsSeparator)
                {
                    DrawIconEx(
                        bufferDc,
                        bufferRect.left + 2,
                        bufferRect.top + ((bufferRect.bottom - bufferRect.top) - context->CXWidth) / 2,
                        itemContext->IconHandle,
                        context->CXWidth,
                        context->CXWidth,
                        0,
                        NULL,
                        DI_NORMAL
                        );
                }

                bufferRect.left += context->CXWidth + 4;

                if (itemContext->IdCommand != 0)
                {
                    DrawText(
                        bufferDc,
                        ToolbarGetText(itemContext->IdCommand),
                        -1,
                        &bufferRect,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP
                        );
                }
                else
                {
                    DrawText(
                        bufferDc,
                        L"Separator",
                        -1,
                        &bufferRect,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP
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
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID ToolBarShowCustomizeDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_CUSTOMIZE_TB),
        ParentWindowHandle,
        CustomizeToolbarDialogProc,
        NULL
        );
}
