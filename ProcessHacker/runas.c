/*
 * Process Hacker - 
 *   run as dialog
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

#include <phgui.h>
#include <windowsx.h>

INT_PTR CALLBACK PhpRunAsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#define SIP(String, Integer) { (String), (PVOID)(Integer) }

static PH_KEY_VALUE_PAIR PhpLogonTypePairs[] =
{
    SIP(L"Batch", LOGON32_LOGON_BATCH),
    SIP(L"Interactive", LOGON32_LOGON_INTERACTIVE),
    SIP(L"Network", LOGON32_LOGON_NETWORK),
    SIP(L"New credentials", LOGON32_LOGON_NEW_CREDENTIALS),
    SIP(L"Service", LOGON32_LOGON_SERVICE)
};

VOID PhShowRunAsDialog(
    __in HWND ParentWindowHandle,
    __in_opt HANDLE ProcessId
    )
{
    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_RUNAS),
        ParentWindowHandle,
        PhpRunAsDlgProc,
        (LPARAM)ProcessId
        );
}

static BOOLEAN NTAPI PhpRunAsEnumAccountsCallback(
    __in PSID Sid,
    __in PVOID Context
    )
{
    PPH_STRING name;
    SID_NAME_USE nameUse;

    name = PhGetSidFullName(Sid, TRUE, &nameUse);

    if (name)
    {
        if (nameUse == SidTypeUser)
            ComboBox_AddString((HWND)Context, name->Buffer);

        PhDereferenceObject(name);
    }

    return TRUE;
}

INT_PTR CALLBACK PhpRunAsDlgProc(
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
            HWND typeComboBoxHandle = GetDlgItem(hwndDlg, IDC_TYPE);
            HWND userNameComboBoxHandle = GetDlgItem(hwndDlg, IDC_USERNAME);
            LSA_HANDLE policyHandle;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            ComboBox_AddString(typeComboBoxHandle, L"Batch");
            ComboBox_AddString(typeComboBoxHandle, L"Interactive");
            ComboBox_AddString(typeComboBoxHandle, L"Network");
            ComboBox_AddString(typeComboBoxHandle, L"New credentials");
            ComboBox_AddString(typeComboBoxHandle, L"Service");
            ComboBox_SelectString(typeComboBoxHandle, -1, L"Interactive");

            ComboBox_AddString(userNameComboBoxHandle, L"NT AUTHORITY\\SYSTEM");
            ComboBox_AddString(userNameComboBoxHandle, L"NT AUTHORITY\\LOCAL SERVICE");
            ComboBox_AddString(userNameComboBoxHandle, L"NT AUTHORITY\\NETWORK SERVICE");

            if (NT_SUCCESS(PhOpenLsaPolicy(&policyHandle, POLICY_VIEW_LOCAL_INFORMATION, NULL)))
            {
                PhEnumAccounts(policyHandle, PhpRunAsEnumAccountsCallback, (PVOID)GetDlgItem(hwndDlg, IDC_USERNAME));
                LsaClose(policyHandle);
            }

            SetFocus(GetDlgItem(hwndDlg, IDC_PROGRAM));
            Edit_SetSel(GetDlgItem(hwndDlg, IDC_PROGRAM), 0, -1);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDOK:
                {
                    EndDialog(hwndDlg, IDOK);
                }
                break;
            case IDC_BROWSE:
                {
                    PVOID fileDialog;
                    PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Programs (*.exe;*.pif;*.com;*.bat)", L"*.exe;*.pif;*.com;*.bat" },
                        { L"All files (*.*)", L"*.*" }
                    };

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        PPH_STRING fileName;

                        fileName = PhGetFileDialogFileName(fileDialog);
                        SetDlgItemText(hwndDlg, IDC_PROGRAM, fileName->Buffer);
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
