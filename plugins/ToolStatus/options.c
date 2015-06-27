/*
 * Process Hacker ToolStatus -
 *   rebar code
 *
 * Copyright (C) 2011-2015 dmex
 * Copyright (C) 2010-2013 wj32
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

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND toolbarCombo = GetDlgItem(hwndDlg, IDC_DISPLAYSTYLECOMBO);
            HWND searchboxCombo = GetDlgItem(hwndDlg, IDC_SEARCHBOX_DISPLAYSTYLECOMBO);

            ComboBox_AddString(toolbarCombo, L"No Text Labels"); // Displays no text label for the toolbar buttons.
            ComboBox_AddString(toolbarCombo, L"Selective Text"); // (Selective Text On Right) Displays text for just the Refresh, Options, Find Handles and Sysinfo toolbar buttons.
            ComboBox_AddString(toolbarCombo, L"Show Text Labels"); // Displays text labels for the toolbar buttons.
            ComboBox_SetCurSel(toolbarCombo, PhGetIntegerSetting(SETTING_NAME_TOOLBARDISPLAYSTYLE));
            
            ComboBox_AddString(searchboxCombo, L"Always show");
            ComboBox_AddString(searchboxCombo, L"Hide when inactive");
            //ComboBox_AddString(searchboxCombo, L"Auto-hide");
            ComboBox_SetCurSel(searchboxCombo, PhGetIntegerSetting(SETTING_NAME_SEARCHBOXDISPLAYMODE));

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_TOOLBAR),
                PhGetIntegerSetting(SETTING_NAME_ENABLE_TOOLBAR) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_SEARCHBOX),
                PhGetIntegerSetting(SETTING_NAME_ENABLE_SEARCHBOX) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_STATUSBAR),
                PhGetIntegerSetting(SETTING_NAME_ENABLE_STATUSBAR) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_RESOLVEGHOSTWINDOWS),
                PhGetIntegerSetting(SETTING_NAME_ENABLE_RESOLVEGHOSTWINDOWS) ? BST_CHECKED : BST_UNCHECKED);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDC_CUSTOMIZETOOLBAR:
                PostMessage(ToolBarHandle, TB_CUSTOMIZE, 0, 0);
                break;
            case IDOK:
                {
                    PhSetIntegerSetting(SETTING_NAME_TOOLBARDISPLAYSTYLE,
                        (DisplayStyle = (TOOLBAR_DISPLAY_STYLE)ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_DISPLAYSTYLECOMBO))));
                    PhSetIntegerSetting(SETTING_NAME_SEARCHBOXDISPLAYMODE,
                        (SearchBoxDisplayMode = (SEARCHBOX_DISPLAY_MODE)ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_SEARCHBOX_DISPLAYSTYLECOMBO))));
                    PhSetIntegerSetting(SETTING_NAME_ENABLE_TOOLBAR,
                        (EnableToolBar = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_TOOLBAR)) == BST_CHECKED));
                    PhSetIntegerSetting(SETTING_NAME_ENABLE_SEARCHBOX,
                        (EnableSearchBox = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_SEARCHBOX)) == BST_CHECKED));
                    PhSetIntegerSetting(SETTING_NAME_ENABLE_STATUSBAR,
                        (EnableStatusBar = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_STATUSBAR)) == BST_CHECKED));
                    PhSetIntegerSetting(SETTING_NAME_ENABLE_RESOLVEGHOSTWINDOWS,
                        Button_GetCheck(GetDlgItem(hwndDlg, IDC_RESOLVEGHOSTWINDOWS)) == BST_CHECKED);

                    LoadToolbarSettings();
                    InvalidateRect(ToolBarHandle, NULL, TRUE);

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}