/*
 * Process Hacker -
 *   send message window
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2019 dmex
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
#include <phsettings.h>
#include <lsasup.h>

#include <winsta.h>

#define SIP(String, Integer) { (String), (PVOID)(Integer) }

static PH_KEY_VALUE_PAIR PhpMessageBoxIconPairs[] =
{
    SIP(L"None", 0),
    SIP(L"Information", MB_ICONINFORMATION),
    SIP(L"Warning", MB_ICONWARNING),
    SIP(L"Error", MB_ICONERROR),
    SIP(L"Question", MB_ICONQUESTION)
};

INT_PTR CALLBACK PhpSessionSendMessageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowSessionSendMessageDialog(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG SessionId
    )
{
    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_EDITMESSAGE),
        ParentWindowHandle,
        PhpSessionSendMessageDlgProc,
        (LPARAM)SessionId
        );
}

INT_PTR CALLBACK PhpSessionSendMessageDlgProc(
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
            HWND iconComboBox;
            PPH_STRING currentUserName;

            PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, UlongToPtr((ULONG)lParam));

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            iconComboBox = GetDlgItem(hwndDlg, IDC_TYPE);

            ComboBox_AddString(iconComboBox, L"None");
            ComboBox_AddString(iconComboBox, L"Information");
            ComboBox_AddString(iconComboBox, L"Warning");
            ComboBox_AddString(iconComboBox, L"Error");
            ComboBox_AddString(iconComboBox, L"Question");
            PhSelectComboBoxString(iconComboBox, L"None", FALSE);

            if (currentUserName = PhGetTokenUserString(PhGetOwnTokenAttributes().TokenHandle, TRUE))
            {
                PhSetDialogItemText(
                    hwndDlg,
                    IDC_TITLE,
                    PhaFormatString(L"Message from %s", currentUserName->Buffer)->Buffer
                    );
                PhDereferenceObject(currentUserName);
            }

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDC_TEXT));

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport); // HACK (dmex)
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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
                    ULONG sessionId = PtrToUlong(PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT));
                    PPH_STRING title;
                    PPH_STRING text;
                    ULONG icon = 0;
                    ULONG64 timeout = 0;
                    ULONG response;

                    title = PhaGetDlgItemText(hwndDlg, IDC_TITLE);
                    text = PhaGetDlgItemText(hwndDlg, IDC_TEXT);

                    PhFindIntegerSiKeyValuePairs(
                        PhpMessageBoxIconPairs,
                        sizeof(PhpMessageBoxIconPairs),
                        PhaGetDlgItemText(hwndDlg, IDC_TYPE)->Buffer,
                        &icon
                        );
                    PhStringToInteger64(
                        &PhaGetDlgItemText(hwndDlg, IDC_TIMEOUT)->sr,
                        10,
                        &timeout
                        );

                    if (WinStationSendMessageW(
                        NULL,
                        sessionId,
                        title->Buffer,
                        (ULONG)title->Length,
                        text->Buffer,
                        (ULONG)text->Length,
                        icon,
                        (ULONG)timeout,
                        &response,
                        TRUE
                        ))
                    {
                        EndDialog(hwndDlg, IDOK);
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to send the message", 0, GetLastError());
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
