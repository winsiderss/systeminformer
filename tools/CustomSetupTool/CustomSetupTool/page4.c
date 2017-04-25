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

BOOLEAN SetupRunning = FALSE;
ULONG64 ExtractCurrentLength = 0;
ULONG64 ExtractTotalLength = 0;

NTSTATUS SetupProgressThread(
    _In_ PSETUP_PROGRESS_THREAD Context
    )
{
    //if (SetupInstallDebuggingTools)
    //    SetupDownloadThread(Arguments)

    // Stop Process Hacker.
    if (!ShutdownProcessHacker())
        goto CleanupExit;

    // Stop the kernel driver(s).
    if (!SetupUninstallKph())
        goto CleanupExit;

    // Create the install folder path.
    if (!CreateDirectoryPath(PhGetString(SetupInstallPath)))
        goto CleanupExit;

    SetupUpgradeSettingsFile();

    // Remove the previous installation.
    if (SetupResetSettings)
        RemoveDirectoryPath(PhGetString(SetupInstallPath));

    // Create the ARP uninstall entries.
    SetupCreateUninstallKey();
    // Create the uninstaller.
    SetupCreateUninstallFile();
    // Create autorun and shortcuts.
    SetupSetWindowsOptions();

    // Setup new installation.
    if (!SetupExtractBuild(Context->DialogHandle))
        goto CleanupExit;

    // Install updated kernel driver
    if (SetupInstallKphService)
        SetupInstallKph();

    PostMessage(Context->DialogHandle, WM_END_SETUP, 0, 0);
    PhDereferenceObject(Context);
    return STATUS_SUCCESS;

CleanupExit:

    PostMessage(Context->PropSheetHandle, PSM_SETCURSELID, 0, IDD_ERROR);
    PhDereferenceObject(Context);
    return STATUS_FAIL_CHECK;
}

INT_PTR CALLBACK SetupPropPage4_WndProc(
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
            SetupLoadImage(GetDlgItem(hwndDlg, IDC_PROJECT_ICON), MAKEINTRESOURCE(IDB_PNG1));
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_MAINHEADER), -17, FW_SEMIBOLD);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_SUBHEADER), -12, FW_NORMAL);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_INSTALL_STATUS), -12, FW_SEMIBOLD);
            SetupInitializeFont(GetDlgItem(hwndDlg, IDC_INSTALL_SUBSTATUS), -12, FW_NORMAL);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            LPPSHNOTIFY pageNotify = (LPPSHNOTIFY)header;

            switch (pageNotify->hdr.code)
            {
            case PSN_QUERYCANCEL:
                {
                    if (SetupRunning && DialogPromptExit(hwndDlg))
                    {
                        PropSheet_CancelToClose(GetParent(hwndDlg));
                        EnableMenuItem(GetSystemMenu(GetParent(hwndDlg), FALSE), SC_CLOSE, MF_GRAYED);
                        EnableMenuItem(GetSystemMenu(GetParent(hwndDlg), FALSE), SC_CLOSE, MF_ENABLED);
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)TRUE);
                        return TRUE;
                    }
                }
                break;
            case PSN_SETACTIVE:
                {
                    HWND hwPropSheet;
                    HANDLE threadHandle;
                    PSETUP_PROGRESS_THREAD progress;

                    hwPropSheet = pageNotify->hdr.hwndFrom;

                    // Disable Next/Back buttons
                    PropSheet_SetWizButtons(hwPropSheet, 0);

                    if (!SetupRunning)
                    {
                        SetupRunning = TRUE;

                        // Setup the progress thread
                        progress = PhCreateAlloc(sizeof(SETUP_PROGRESS_THREAD));
                        progress->DialogHandle = hwndDlg;
                        progress->PropSheetHandle = hwPropSheet;

                        if (threadHandle = PhCreateThread(0, SetupProgressThread, progress))
                            NtClose(threadHandle);
                    }
                }
                break;
            }
        }
        break;
    case WM_START_SETUP:
        {
            SetWindowText(GetDlgItem(hwndDlg, IDC_MAINHEADER), 
                PhaFormatString(L"Installing Process Hacker %lu.%lu.%lu", PHAPP_VERSION_MAJOR, PHAPP_VERSION_MINOR, PHAPP_VERSION_REVISION)->Buffer);
            SetWindowText(GetDlgItem(hwndDlg, IDC_INSTALL_SUBSTATUS), L"Progress: ~ of ~ (0.0%)");
            SendMessage(GetDlgItem(hwndDlg, IDC_INSTALL_PROGRESS), PBM_SETRANGE32, 0, (LPARAM)ExtractTotalLength);
        }
        break;
    case WM_UPDATE_SETUP:
        {
            PPH_STRING currentFile = (PPH_STRING)lParam;

            SetWindowText(GetDlgItem(hwndDlg, IDC_INSTALL_STATUS),
                PhaConcatStrings2(L"Extracting: ", currentFile->Buffer)->Buffer
                );
            SetWindowText(GetDlgItem(hwndDlg, IDC_INSTALL_SUBSTATUS), PhaFormatString(
                L"Progress: %s of %s (%.2f%%)",
                PhaFormatSize(ExtractCurrentLength, -1)->Buffer,
                PhaFormatSize(ExtractTotalLength, -1)->Buffer,
                (FLOAT)((double)ExtractCurrentLength / (double)ExtractTotalLength) * 100
                )->Buffer);
            SendMessage(GetDlgItem(hwndDlg, IDC_INSTALL_PROGRESS), PBM_SETPOS, (WPARAM)ExtractCurrentLength, 0);

            PhDereferenceObject(currentFile);
        }
        break;
    case WM_END_SETUP:
        {
            SetupRunning = FALSE;

            SetWindowText(GetDlgItem(hwndDlg, IDC_MAINHEADER), L"Setup Complete");
            SetWindowText(GetDlgItem(hwndDlg, IDC_INSTALL_STATUS), L"Extract complete");
            Button_SetText(GetDlgItem(GetParent(hwndDlg), IDC_PROPSHEET_CANCEL), L"Close");

            if (SetupStartAppAfterExit)
            {
                SetupExecuteProcessHacker(GetParent(hwndDlg));
                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_FINISH);
            }
        }
        break;
    }

    return FALSE;
}