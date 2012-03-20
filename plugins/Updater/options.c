/*
 * Process Hacker Update Checker -
 *   options dialog
 *
 * Copyright (C) 2011-2012 dmex
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
            HWND sfComboBoxHandle = GetDlgItem(hwndDlg, IDC_DLCOMBOBOX);
            HWND hashComboBoxHandle = GetDlgItem(hwndDlg, IDC_HASHCOMBOBOX);

            WCHAR *hashTypeStrings[] = { L"SHA1", L"MD5" };
            WCHAR *sfDownloadLocationStrings[] = 
            { 
                L"Auto-detect", 
                L"AARNet (Melbourne, Australia, AU)", 
                L"Waix (Perth, Australia, AU)"
                L"Internode (Adelaide, Australia, AU)",

                L"Japan AIST (Nomi, Japan, JP)",
                L"CDNetworks (Seoul, Korea, Republic of, KR)",
                L"CityLan (Moscow, Russian Federation, RU)",
                L"Ignum (Prague, Czech Republic, CZ)",
                L"SWITCH (Zurich, Switzerland, CH)",
                L"NetCologne (K&ouml;ln, Germany, DE)",
                L"University of Kent (Bristol, United Kingdom, GB)",
                
                L"National Center for HPC (Taipei, Taiwan, TW)",
                L"CDNetworks (Seoul, Korea, Republic of, KR)",
                L"TENET (Wynberg, South Africa, ZA)",
                L"garr.it (Ancona, Italy, IT)",
                L"Centro de Computacao (Curitiba, Brazil, BR)",
                L"German Research Network (Germany, DE)",
                L"Free France (Paris, France, FR)",
                L"HEAnet (Ireland, IE)",

                L"Softlayer (Dallas, TX, US)",
                L"Superb Internet (United States, US)",
                L"Voxel Hosting (New York, NY, US)",

                L"iWeb (Montreal, QC, CA)",
            };

            PhAddComboBoxStrings(hashComboBoxHandle, hashTypeStrings, ARRAYSIZE(hashTypeStrings));
            ComboBox_SetCurSel(hashComboBoxHandle, PhGetIntegerSetting(SETTING_HASH_ALGORITHM));

            PhAddComboBoxStrings(sfComboBoxHandle, sfDownloadLocationStrings, ARRAYSIZE(sfDownloadLocationStrings));
            ComboBox_SetCurSel(sfComboBoxHandle, ComboBox_FindString(sfComboBoxHandle, 0, L"Auto-detect"));          

            if (PhGetIntegerSetting(SETTING_ENABLE_CACHE))
                Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLECACHE), BST_CHECKED);

            if (PhGetIntegerSetting(SETTING_AUTO_CHECK))
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
                    PhSetIntegerSetting(SETTING_ENABLE_CACHE,
                        Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLECACHE)) == BST_CHECKED);

                    PhSetIntegerSetting(SETTING_AUTO_CHECK,
                        Button_GetCheck(GetDlgItem(hwndDlg, IDC_AUTOCHECKBOX)) == BST_CHECKED);

                    PhSetIntegerSetting(SETTING_HASH_ALGORITHM,
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