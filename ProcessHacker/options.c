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

INT CALLBACK PhpOptionsPropSheetProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpOptionsGeneralDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpOptionsAdvancedDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

static BOOLEAN PressedOk;

VOID PhShowOptionsDialog(
    __in HWND ParentWindowHandle
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[2];

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE |
        PSH_USECALLBACK;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = L"Options";
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;
    propSheetHeader.pfnCallback = PhpOptionsPropSheetProc;

    // General page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OPTGENERAL);
    propSheetPage.pfnDlgProc = PhpOptionsGeneralDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);
    // Advanced page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OPTADVANCED);
    propSheetPage.pfnDlgProc = PhpOptionsAdvancedDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    PressedOk = FALSE;
    PropertySheet(&propSheetHeader);

    if (PressedOk)
    {
        ProcessHacker_SaveAllSettings(PhMainWndHandle);
        PhInvalidateAllProcessNodes();
    }
}

INT CALLBACK PhpOptionsPropSheetProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case PSCB_BUTTONPRESSED:
        {
            if (lParam == PSBTN_OK)
            {
                PressedOk = TRUE;
            }
        }
        break;
    }

    return 0;
}

#define SetDlgItemCheckForSetting(hwndDlg, Id, Name) \
    Button_SetCheck(GetDlgItem(hwndDlg, Id), PhGetIntegerSetting(Name) ? BST_CHECKED : BST_UNCHECKED)
#define SetSettingForDlgItemCheck(hwndDlg, Id, Name) \
    PhSetIntegerSetting(Name, Button_GetCheck(GetDlgItem(hwndDlg, Id)) == BST_CHECKED)
#define DialogChanged PropSheet_Changed(GetParent(hwndDlg), hwndDlg)

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
            // HACK
            SetWindowText(GetParent(hwndDlg), L"Options"); // so the title isn't "Options Properties"

            comboBoxHandle = GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT);

            for (i = 0; i < sizeof(PhSizeUnitNames) / sizeof(PWSTR); i++)
                ComboBox_AddString(comboBoxHandle, PhSizeUnitNames[i]);

            SetDlgItemText(hwndDlg, IDC_SEARCHENGINE, PHA_GET_STRING_SETTING(L"SearchEngine")->Buffer);
            SetDlgItemText(hwndDlg, IDC_PEVIEWER, PHA_GET_STRING_SETTING(L"ProgramInspectExecutables")->Buffer);

            if (PhMaxSizeUnit != -1)
                ComboBox_SetCurSel(comboBoxHandle, PhMaxSizeUnit);
            else
                ComboBox_SetCurSel(comboBoxHandle, sizeof(PhSizeUnitNames) / sizeof(PWSTR) - 1);

            SetDlgItemCheckForSetting(hwndDlg, IDC_ALLOWONLYONEINSTANCE, L"AllowOnlyOneInstance");
            SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEONCLOSE, L"HideOnClose");
            SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEONMINIMIZE, L"HideOnMinimize");
            SetDlgItemCheckForSetting(hwndDlg, IDC_STARTHIDDEN, L"StartHidden");
        }
        break;
    /*case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_SEARCHENGINE:
            case IDC_PEVIEWER:
                if (HIWORD(wParam) == EN_CHANGE)
                    DialogChanged;
                break;
            case IDC_MAXSIZEUNIT:
            case IDC_ALLOWONLYONEINSTANCE:
            case IDC_ENABLEWARNINGS:
            case IDC_ENABLEKERNELMODEDRIVER:
            case IDC_HIDEUNNAMEDHANDLES:
                DialogChanged;
                break;
            }
        }
        break;*/
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_APPLY:
                {
                    PhSetStringSetting2(L"SearchEngine", &(PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_SEARCHENGINE)->sr));
                    PhSetStringSetting2(L"ProgramInspectExecutables", &(PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_PEVIEWER)->sr));
                    PhSetIntegerSetting(L"MaxSizeUnit", PhMaxSizeUnit = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT)));
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ALLOWONLYONEINSTANCE, L"AllowOnlyOneInstance");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_HIDEONCLOSE, L"HideOnClose");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_HIDEONMINIMIZE, L"HideOnMinimize");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_STARTHIDDEN, L"StartHidden");

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpOptionsAdvancedDlgProc(
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
            SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEWARNINGS, L"EnableWarnings");
            SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEKERNELMODEDRIVER, L"EnableKph");
            SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEUNNAMEDHANDLES, L"HideUnnamedHandles");
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_APPLY:
                {
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ENABLEWARNINGS, L"EnableWarnings");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ENABLEKERNELMODEDRIVER, L"EnableKph");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_HIDEUNNAMEDHANDLES, L"HideUnnamedHandles");

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
