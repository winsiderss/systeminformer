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
            ULONG maxPingCount = PhGetIntegerSetting(L"ProcessHacker.NetTools.MaxPingCount");
            ULONG maxPingTimeout = PhGetIntegerSetting(L"ProcessHacker.NetTools.MaxPingTimeout");

            if (maxPingCount)
            {
                //Static_SetText(GetDlgItem(hwndDlg, IDC_MAXPINGTEXT), maxPingCountText->Buffer); 
                //PhDereferenceObject(maxPingCountText);
            }

            if (maxPingTimeout)
            {
                //Static_SetText(GetDlgItem(hwndDlg, IDC_MAXTIMEOUTTEXT), maxPingTimeoutText->Buffer); 
                //PhDereferenceObject(maxPingTimeoutText);
            }

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_NETRESOLVECHECK),
                PhGetIntegerSetting(L"ProcessHacker.NetTools.EnableHostnameLookup") ? BST_CHECKED : BST_UNCHECKED
                );
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
                    PPH_STRING maxPingCountText = NULL;
                    PPH_STRING maxPingTimeoutText = NULL;

                    if (maxPingCountText = PhGetWindowText(GetDlgItem(hwndDlg, IDC_MAXPINGTEXT)))
                    {
                        PhSetStringSetting(L"ProcessHacker.NetTools.MaxPingCount", maxPingCountText->Buffer);
                        PhDereferenceObject(maxPingCountText);
                    }

                    if (maxPingTimeoutText = PhGetWindowText(GetDlgItem(hwndDlg, IDC_MAXTIMEOUTTEXT)))
                    {
                        PhSetStringSetting(L"ProcessHacker.NetTools.MaxPingTimeout", maxPingTimeoutText->Buffer); 
                        PhDereferenceObject(maxPingTimeoutText);
                    }

                    PhSetIntegerSetting(L"ProcessHacker.NetTools.EnableHostnameLookup",
                        Button_GetCheck(GetDlgItem(hwndDlg, IDC_NETRESOLVECHECK)) == BST_CHECKED
                        );

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}