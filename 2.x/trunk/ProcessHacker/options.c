/*
 * Process Hacker - 
 *   options window
 * 
 * Copyright (C) 2010 wj32
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

#include <phapp.h>
#include <settings.h>
#include <windowsx.h>

INT_PTR CALLBACK PhpOptionsGeneralDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowOptionsDialog(
    __in HWND ParentWindowHandle
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[1];

    propSheetHeader.dwFlags =
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = L"Options";
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // General page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OPTGENERAL);
    propSheetPage.pfnDlgProc = PhpOptionsGeneralDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    PropertySheet(&propSheetHeader);
}

#define SetDlgItemCheckForSetting(hwndDlg, Id, Name) \
    Button_SetCheck(GetDlgItem(hwndDlg, Id), PhGetIntegerSetting(Name) ? BST_CHECKED : BST_UNCHECKED)

INT_PTR CALLBACK PhpOptionsGeneralDlgProc(
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
            HWND comboBoxHandle;
            ULONG i;

            // HACK
            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg)));

            comboBoxHandle = GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT);

            for (i = 0; i < sizeof(PhSizeUnitNames) / sizeof(PWSTR); i++)
                ComboBox_AddString(comboBoxHandle, PhSizeUnitNames[i]);

            if (PhMaxSizeUnit == -1) PhMaxSizeUnit = sizeof(PhSizeUnitNames) / sizeof(PWSTR) - 1;
            ComboBox_SelectString(comboBoxHandle, -1, PhSizeUnitNames[PhMaxSizeUnit]);

            SetDlgItemText(hwndDlg, IDC_SEARCHENGINE, PHA_GET_STRING_SETTING(L"SearchEngine")->Buffer);

            SetDlgItemCheckForSetting(hwndDlg, IDC_ALLOWONLYONEINSTANCE, L"AllowOnlyOneInstance");
            SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEWARNINGS, L"EnableWarnings");
            SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEKERNELMODEDRIVER, L"EnableKph");
            SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEUNNAMEDHANDLES, L"HideUnnamedHandles");
        }
        break;
    }

    return FALSE;
}
