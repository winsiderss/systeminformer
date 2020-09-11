/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2020 dmex
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

#include <peview.h>

INT_PTR CALLBACK PvOptionsWndProc(
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
            HWND comboBoxHandle;

            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PvImageSmallIcon);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PvImageLargeIcon);

            comboBoxHandle = GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetDialogItemText(hwndDlg, IDC_DBGHELPSEARCHPATH, PhaGetStringSetting(L"DbgHelpSearchPath")->Buffer);

            for (ULONG i = 0; i < RTL_NUMBER_OF(PhSizeUnitNames); i++)
                ComboBox_AddString(comboBoxHandle, PhSizeUnitNames[i]);

            if (PhMaxSizeUnit != ULONG_MAX)
                ComboBox_SetCurSel(comboBoxHandle, PhMaxSizeUnit);
            else
                ComboBox_SetCurSel(comboBoxHandle, RTL_NUMBER_OF(PhSizeUnitNames) - 1);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
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
                    BOOLEAN restartRequired = FALSE;

                    if (!PhEqualString(PhaGetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH), PhaGetStringSetting(L"DbgHelpSearchPath"), TRUE))
                    {
                        PhSetStringSetting2(L"DbgHelpSearchPath", &(PhaGetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH)->sr));
                        restartRequired = TRUE;
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
