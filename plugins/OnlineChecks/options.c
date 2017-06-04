/*
 * Process Hacker Online Checks -
 *   options dialog
 *
 * Copyright (C) 2016 dmex
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

#include "onlnchk.h"

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
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_VIRUSTOTAL),
                PhGetIntegerSetting(SETTING_NAME_VIRUSTOTAL_SCAN_ENABLED) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_IDC_ENABLE_VIRUSTOTAL_HIGHLIGHT),
                PhGetIntegerSetting(SETTING_NAME_VIRUSTOTAL_HIGHLIGHT_DETECTIONS) ? BST_CHECKED : BST_UNCHECKED);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                {
                    PhSetIntegerSetting(SETTING_NAME_VIRUSTOTAL_SCAN_ENABLED, 
                        Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_VIRUSTOTAL)) == BST_CHECKED ? 1 : 0);

                    EndDialog(hwndDlg, IDCANCEL);
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
        (HWND)Parent,
        OptionsDlgProc
        );
}