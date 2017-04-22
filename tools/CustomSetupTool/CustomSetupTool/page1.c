/*
 * Process Hacker Toolchain -
 *   project setup
 *
 * Copyright (C) 2017 dmex
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

#include <setup.h>

VOID SetupPropSheetParent(
    _In_ HWND hwndDlg
    )
{
    HWND parent = GetParent(hwndDlg);

    // Center the property sheet on the desktop
    PhCenterWindow(parent, NULL);
    SetForegroundWindow(parent);

    // Set the fonts for the Property Sheet buttons
    SetupInitializeFont(GetDlgItem(parent, IDC_PROPSHEET_BACK), -12, FW_NORMAL);
    SetupInitializeFont(GetDlgItem(parent, IDC_PROPSHEET_NEXT), -12, FW_NORMAL);
    SetupInitializeFont(GetDlgItem(parent, IDC_PROPSHEET_CANCEL), -12, FW_NORMAL);
}

static VOID LoadSetupIcons(
    _In_ HWND hwndDlg
    )
{
    HBITMAP smallIconHandle = (HBITMAP)LoadImage(
        PhLibImageBase,
        MAKEINTRESOURCE(IDI_ICON1),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
        );
    HBITMAP largeIconHandle = (HBITMAP)LoadImage(
        PhLibImageBase,
        MAKEINTRESOURCE(IDI_ICON1),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        LR_DEFAULTCOLOR
        );

    SendMessage(GetParent(hwndDlg), WM_SETICON, ICON_SMALL, (LPARAM)smallIconHandle);
    SendMessage(GetParent(hwndDlg), WM_SETICON, ICON_BIG, (LPARAM)largeIconHandle);

    DeleteObject(largeIconHandle);
    DeleteObject(smallIconHandle);
}

INT_PTR CALLBACK SetupPropPage1_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SetupPropSheetParent(hwndDlg);

            LoadSetupIcons(hwndDlg);
            SetupLoadImage(GetDlgItem(hwndDlg, IDC_PROJECT_ICON), MAKEINTRESOURCE(IDB_PNG1));
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_MAINHEADER), -18, FW_SEMIBOLD);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_SUBHEADER), 0, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_SYSLINK1), 0, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_SYSLINK2), 0, FW_NORMAL);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            LPPSHNOTIFY pageNotify = (LPPSHNOTIFY)header;

            switch (pageNotify->hdr.code)
            {
            case PSN_SETACTIVE:
                {
                    HWND hwPropSheet = pageNotify->hdr.hwndFrom;

                    // Reset the button state.
                    PropSheet_SetWizButtons(hwPropSheet, PSWIZB_NEXT);

                    // Hide the back button.
                    //ShowWindow(GetDlgItem(hwPropSheet, IDC_PROPSHEET_BACK), SW_HIDE);

                    // HACK: Focus the next button (reset after changing button state).
                    PostMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwPropSheet, IDC_PROPSHEET_NEXT), TRUE);
                }
                break;
            case PSN_KILLACTIVE:
                {
                    HWND hwPropSheet = pageNotify->hdr.hwndFrom;

                    // Enable the back button.
                    PropSheet_SetWizButtons(hwPropSheet, PSWIZB_NEXT | PSWIZB_BACK);

                    // Show the back button.
                    //ShowWindow(GetDlgItem(hwPropSheet, IDC_PROPSHEET_BACK), SW_SHOW);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}