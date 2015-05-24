/*
 * Process Hacker Extra Plugins -
 *   Nvidia GPU Plugin
 *
 * Copyright (C) 2015 dmex
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

#include "main.h"
#include <windowsx.h>

#define SetDlgItemCheckForSetting(hwndDlg, Id, Name) \
    Button_SetCheck(GetDlgItem(hwndDlg, Id), PhGetIntegerSetting(Name) ? BST_CHECKED : BST_UNCHECKED)
#define SetSettingForDlgItemCheckRestartRequired(hwndDlg, Id, Name) \
    do { \
        BOOLEAN __oldValue = !!PhGetIntegerSetting(Name); \
        BOOLEAN __newValue = Button_GetCheck(GetDlgItem(hwndDlg, Id)) == BST_CHECKED; \
        if (__newValue != __oldValue) \
            RestartRequired = TRUE; \
        PhSetIntegerSetting(Name, __newValue); \
    } while (0)

static INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static BOOLEAN RestartRequired;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            RestartRequired = FALSE;

            SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLENVIDIASUPPORT, SETTING_NAME_ENABLE_MONITORING);
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
                    SetSettingForDlgItemCheckRestartRequired(hwndDlg, IDC_ENABLENVIDIASUPPORT, SETTING_NAME_ENABLE_MONITORING);

                    if (RestartRequired)
                    {
                        if (PhShowMessage(
                            PhMainWndHandle,
                            MB_ICONQUESTION | MB_YESNO,
                            L"One or more options you have changed requires a restart of Process Hacker. "
                            L"Do you want to restart Process Hacker now?"
                            ) == IDYES)
                        {
                            ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);
                            PhShellProcessHacker(
                                PhMainWndHandle,
                                L"-v",
                                SW_SHOW,
                                0,
                                PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                                0,
                                NULL
                                );
                            ProcessHacker_Destroy(PhMainWndHandle);
                        }
                    }

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
    _In_ HWND ParentHandle
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        ParentHandle,
        OptionsDlgProc
        );
}