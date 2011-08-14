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
    );

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
			PhCenterWindow(hwndDlg, GetParent(hwndDlg));

			EnableCache = PhGetIntegerSetting(L"ProcessHacker.Updater.EnableCache");

			if (EnableCache)
			{
				Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLECACHE), BST_CHECKED);
			}
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
					PhSetIntegerSetting(L"ProcessHacker.Updater.EnableCache", Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLECACHE)) == BST_CHECKED);

					EnableCache = PhGetIntegerSetting(L"ProcessHacker.Updater.EnableCache");

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
