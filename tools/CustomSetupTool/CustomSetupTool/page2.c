/*
 * Process Hacker Toolchain -
 *   project setup
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

INT_PTR CALLBACK SetupPropPage2_WndProc(
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
            PSTR resourceBuffer;
            PPH_STRING eulaTextString;

            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_MAINHEADER), -17, FW_SEMIBOLD);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_SUBHEADER), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_EDIT1), -12, FW_NORMAL);

            if (resourceBuffer = ExtractResourceToBuffer(MAKEINTRESOURCE(IDR_LICENCE_DATA)))
            {
                if (eulaTextString = PhConvertUtf8ToUtf16(resourceBuffer))
                {
                    SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT1), eulaTextString->Buffer);

                    PhDereferenceObject(eulaTextString);
                }

                PhFree(resourceBuffer);
            }

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

                    PostMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwPropSheet, IDC_PROPSHEET_NEXT), TRUE);

                    // Disable the Next button
                    //PropSheet_SetWizButtons(hwPropSheet, PSWIZB_BACK);
                }
                break;
            case PSN_QUERYINITIALFOCUS:
                {
                    HWND hwPropSheet = pageNotify->hdr.hwndFrom;
                    HWND hwnd = GetDlgItem(hwPropSheet, IDC_PROPSHEET_CANCEL);

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)hwnd);
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}