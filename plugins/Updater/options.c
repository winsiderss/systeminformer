/*
 * Process Hacker Update Checker -
 *   options dialog
 *
 * Copyright (C) 2011 dmex
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

#include "updater.h"

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND hashCboxHandle = GetDlgItem(hwndDlg, IDC_HASHCOMBOBOX);

            ComboBox_AddString(hashCboxHandle, L"SHA1");
            ComboBox_AddString(hashCboxHandle, L"MD5");
            ComboBox_SetCurSel(hashCboxHandle, PhGetIntegerSetting(L"ProcessHacker.Updater.HashAlgorithm"));

            if (PhGetIntegerSetting(L"ProcessHacker.Updater.EnableCache"))
                Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLECACHE), BST_CHECKED);

            if (PhGetIntegerSetting(L"ProcessHacker.Updater.PromptStart"))
                Button_SetCheck(GetDlgItem(hwndDlg, IDC_AUTOCHECKBOX), BST_CHECKED);
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
                    PhSetIntegerSetting(L"ProcessHacker.Updater.EnableCache",
                        Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLECACHE)) == BST_CHECKED);

                    PhSetIntegerSetting(L"ProcessHacker.Updater.PromptStart",
                        Button_GetCheck(GetDlgItem(hwndDlg, IDC_AUTOCHECKBOX)) == BST_CHECKED);

                    PhSetIntegerSetting(L"ProcessHacker.Updater.HashAlgorithm",
                        ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_HASHCOMBOBOX)));

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
