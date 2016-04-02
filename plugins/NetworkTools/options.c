/*
 * Process Hacker Network Tools -
 *   options dialog
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2012-2013 dmex
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

#include "nettools.h"

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

            SetDlgItemInt(hwndDlg, IDC_PINGPACKETLENGTH, PhGetIntegerSetting(SETTING_NAME_PING_SIZE), FALSE);
            SetDlgItemInt(hwndDlg, IDC_MINSCALING, PhGetIntegerSetting(SETTING_NAME_PING_MINIMUM_SCALING), FALSE);
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
                    ULONG pingPacketLength = GetDlgItemInt(hwndDlg, IDC_PINGPACKETLENGTH, NULL, FALSE);
                    ULONG minGraphScaling = GetDlgItemInt(hwndDlg, IDC_MINSCALING, NULL, FALSE);

                    PhSetIntegerSetting(SETTING_NAME_PING_SIZE, pingPacketLength);
                    PhSetIntegerSetting(SETTING_NAME_PING_MINIMUM_SCALING, minGraphScaling);

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
        (HWND)Parent,
        OptionsDlgProc
        );
}