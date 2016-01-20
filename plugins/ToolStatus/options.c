/*
 * Process Hacker ToolStatus -
 *   Plugin Options
 *
 * Copyright (C) 2011-2016 dmex
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

static INT_PTR CALLBACK OptionsDlgProc(
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
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_TOOLBAR),
                ToolStatusConfig.ToolBarEnabled ? BST_CHECKED : BST_UNCHECKED);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_STATUSBAR),
                ToolStatusConfig.StatusBarEnabled ? BST_CHECKED : BST_UNCHECKED);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_RESOLVEGHOSTWINDOWS),
                ToolStatusConfig.ResolveGhostWindows ? BST_CHECKED : BST_UNCHECKED);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_AUTOHIDE_MENU),
                ToolStatusConfig.AutoHideMenu ? BST_CHECKED : BST_UNCHECKED);
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
                    ToolStatusConfig.ToolBarEnabled = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_TOOLBAR)) == BST_CHECKED;
                    ToolStatusConfig.StatusBarEnabled = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_STATUSBAR)) == BST_CHECKED;
                    ToolStatusConfig.ResolveGhostWindows = Button_GetCheck(GetDlgItem(hwndDlg, IDC_RESOLVEGHOSTWINDOWS)) == BST_CHECKED;
                    ToolStatusConfig.AutoHideMenu = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_AUTOHIDE_MENU)) == BST_CHECKED;

                    PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                    ToolbarLoadSettings();

                    if (ToolStatusConfig.AutoHideMenu)
                    {
                        SetMenu(PhMainWndHandle, NULL);
                    }
                    else
                    {
                        SetMenu(PhMainWndHandle, MainMenu);
                        DrawMenuBar(PhMainWndHandle);
                    }

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID ShowOptionsDialog(
    _In_opt_ HWND Parent
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        Parent,
        OptionsDlgProc
        );
}