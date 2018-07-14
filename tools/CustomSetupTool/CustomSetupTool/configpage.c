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

INT_PTR CALLBACK SetupPropPage3_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    )
{
    PPH_SETUP_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhGetWindowContext(GetParent(hwndDlg), ULONG_MAX);
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_MAINHEADER), -17, FW_SEMIBOLD);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_SUBHEADER), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_STATIC1), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_INSTALL_DIRECTORY), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_FOLDER_BROWSE), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_STATIC2), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_SHORTCUT_CHECK), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_SHORTCUT_ALL_CHECK), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_TASKMANAGER_CHECK), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_STARTUP_CHECK), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_STARTMIN_CHECK), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_STATIC3), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_PEVIEW_CHECK), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_KPH_CHECK), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_DBGTOOLS_CHECK), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_RESET_CHECK), -12, FW_NORMAL);

            if (PhIsNullOrEmptyString(SetupInstallPath))
                SetupInstallPath = SetupFindInstallDirectory();

            SetDlgItemText(hwndDlg, IDC_INSTALL_DIRECTORY, PhGetString(SetupInstallPath));
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_SHORTCUT_CHECK), TRUE);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_SHORTCUT_ALL_CHECK), TRUE);
            //Button_SetCheck(GetDlgItem(hwndDlg, IDC_KPH_CHECK), TRUE);
            //Button_SetCheck(GetDlgItem(hwndDlg, IDC_DBGTOOLS_CHECK), TRUE);     
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_PHSTART_CHECK), TRUE);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_FOLDER_BROWSE:
                {
                    PVOID fileDialog;

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_PICKFOLDERS);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        PPH_STRING fileDialogFolderPath;

                        fileDialogFolderPath = PH_AUTO(PhGetFileDialogFileName(fileDialog));
                        PhTrimToNullTerminatorString(fileDialogFolderPath);
                        PhSwapReference(&SetupInstallPath, fileDialogFolderPath);

                        PhFreeFileDialog(fileDialog);
                    }

                    if (PhIsNullOrEmptyString(SetupInstallPath))
                        SetupInstallPath = SetupFindInstallDirectory();

                    SetDlgItemText(hwndDlg, IDC_INSTALL_DIRECTORY, PhGetStringOrEmpty(SetupInstallPath));
                }
                break;
            case IDC_STARTUP_CHECK:
                {
                    BOOL enabled = Button_GetCheck(GetDlgItem(hwndDlg, IDC_STARTUP_CHECK)) == BST_CHECKED;

                    // Enable the 'Start minimized on system tray' checkbox if 'Start minimized' has been enabled.
                    EnableWindow(GetDlgItem(hwndDlg, IDC_STARTMIN_CHECK), enabled);

                    if (!enabled)
                    {
                        // Clear the 'Start minimized on system tray' checkbox if 'Start minimized' has been disabled.
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_STARTMIN_CHECK), FALSE);
                    }
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            LPPSHNOTIFY pageNotify = (LPPSHNOTIFY)header;

            switch (pageNotify->hdr.code)
            {
            case PSN_WIZNEXT:
                {
                    context->SetupCreateDesktopShortcut = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SHORTCUT_CHECK)) == BST_CHECKED;
                    context->SetupCreateDesktopShortcutAllUsers = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SHORTCUT_ALL_CHECK)) == BST_CHECKED;
                    context->SetupCreateDefaultTaskManager = Button_GetCheck(GetDlgItem(hwndDlg, IDC_TASKMANAGER_CHECK)) == BST_CHECKED;
                    context->SetupCreateSystemStartup = Button_GetCheck(GetDlgItem(hwndDlg, IDC_STARTUP_CHECK)) == BST_CHECKED;
                    context->SetupCreateMinimizedSystemStartup = Button_GetCheck(GetDlgItem(hwndDlg, IDC_STARTMIN_CHECK)) == BST_CHECKED;
                    context->SetupInstallDebuggingTools = Button_GetCheck(GetDlgItem(hwndDlg, IDC_DBGTOOLS_CHECK)) == BST_CHECKED;
                    context->SetupInstallPeViewAssociations = Button_GetCheck(GetDlgItem(hwndDlg, IDC_PEVIEW_CHECK)) == BST_CHECKED;
                    context->SetupInstallKphService = Button_GetCheck(GetDlgItem(hwndDlg, IDC_KPH_CHECK)) == BST_CHECKED;
                    context->SetupResetSettings = Button_GetCheck(GetDlgItem(hwndDlg, IDC_RESET_CHECK)) == BST_CHECKED;
                    context->SetupStartAppAfterExit = Button_GetCheck(GetDlgItem(hwndDlg, IDC_PHSTART_CHECK)) == BST_CHECKED;

                    SetupInstallPath = PhGetWindowText(GetDlgItem(hwndDlg, IDC_INSTALL_DIRECTORY));

                    if (PhIsNullOrEmptyString(SetupInstallPath))
                        SetupInstallPath = SetupFindInstallDirectory();
#ifdef PH_BUILD_API
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)IDD_DIALOG4);
                    return TRUE;
#endif
                }
                break;
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hwndDlg, IDC_FOLDER_BROWSE));
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}