/*
 * Process Hacker Extended Tools -
 *   GPU details window
 *
 * Copyright (C) 2018 dmex
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

#include "exttools.h"
#include <uxtheme.h>

INT_PTR CALLBACK EtpGpuDetailsDlgProc(
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
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER)));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PH_LOAD_SHARED_ICON_LARGE(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER)));

            //PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            //PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            //LayoutMargin = PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_ALL)->Margin;

            // Note: This dialog must be centered after all other graphs and controls have been added.
            //if (PhGetIntegerPairSetting(SETTING_NAME_GPU_NODES_WINDOW_POSITION).X != 0)
            //    PhLoadWindowPlacementFromSetting(SETTING_NAME_GPU_NODES_WINDOW_POSITION, SETTING_NAME_GPU_NODES_WINDOW_SIZE, hwndDlg);
            //else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {

        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID EtShowGpuDetailsDialog(
    _In_ HWND ParentWindowHandle
    )
{
    DialogBox(
        PluginInstance->DllBase, 
        MAKEINTRESOURCE(IDD_SYSINFO_GPUDETAILS),
        ParentWindowHandle,
        EtpGpuDetailsDlgProc
        );
}
