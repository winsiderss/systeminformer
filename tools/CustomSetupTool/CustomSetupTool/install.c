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
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    // Stop Process Hacker.
    if (!ShutdownProcessHacker())
        goto CleanupExit;

    // Stop the kernel driver(s).
    if (!SetupUninstallKph(Context))
        goto CleanupExit;

    // Upgrade the 2.x settings file.
    SetupUpgradeSettingsFile();

    // Remove the previous installation.
    if (Context->SetupResetSettings)
        PhDeleteDirectory(SetupInstallPath);

    // Create the install folder path.
    if (!NT_SUCCESS(PhCreateDirectory(SetupInstallPath)))
        goto CleanupExit;

    // Create the uninstaller.
    if (!SetupCreateUninstallFile(Context))
        goto CleanupExit;

    // Create the ARP uninstall entries.
    SetupCreateUninstallKey(Context);

    // Create autorun and shortcuts.
    SetupSetWindowsOptions(Context);

    // Set the default image execution options.
    SetupCreateImageFileExecutionOptions();

    // Setup new installation.
    if (!SetupExtractBuild(Context))
        goto CleanupExit;

    // Setup kernel driver.
    if (Context->SetupInstallKphService)
        SetupStartKph(Context, TRUE);

    PostMessage(Context->ExtractPageHandle, WM_END_SETUP, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:
    PostMessage(Context->DialogHandle, PSM_SETCURSELID, 0, IDD_ERROR);
    return STATUS_FAIL_CHECK;
}

INT_PTR CALLBACK SetupInstallPropPage_WndProc(
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
            context->ExtractPageHandle = hwndDlg;

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
                    context->MainHeaderHandle = GetDlgItem(hwndDlg, IDC_MAINHEADER);
                    context->StatusHandle = GetDlgItem(hwndDlg, IDC_INSTALL_STATUS);
                    context->SubStatusHandle = GetDlgItem(hwndDlg, IDC_INSTALL_SUBSTATUS);
                    context->ProgressHandle = GetDlgItem(hwndDlg, IDC_INSTALL_PROGRESS);

                    SetWindowText(context->MainHeaderHandle, L"Installing...");
                    SetWindowText(context->StatusHandle, L"");
                    SetWindowText(context->SubStatusHandle, L"Progress: ~ of ~ (0.0%)");

                    // Disable Next/Back buttons
                    PropSheet_SetWizButtons(context->DialogHandle, 0);

                    if (!SetupRunning)
                    {
                        SetupRunning = TRUE;

                        PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), SetupProgressThread, context);
                    }
                }
                break;
            }
        }
        break;
    case WM_START_SETUP:
        {
#ifdef PH_BUILD_API
            SetWindowText(context->MainHeaderHandle, PhaFormatString(
                L"Installing Process Hacker %lu.%lu.%lu", 
                PHAPP_VERSION_MAJOR, 
                PHAPP_VERSION_MINOR, 
                PHAPP_VERSION_REVISION
                )->Buffer);
#else
            SetWindowText(context->MainHeaderHandle, PhaFormatString(
                L"Installing Process Hacker %s", 
                PhGetString(context->RelVersion)
                )->Buffer);
#endif
            SendMessage(context->ProgressHandle, PBM_SETRANGE32, 0, (LPARAM)ExtractTotalLength);
        }
        break;
    case WM_UPDATE_SETUP:
        {
            PPH_STRING currentFile = (PPH_STRING)lParam;

            SetWindowText(context->StatusHandle,
                PhaConcatStrings2(L"Extracting: ", currentFile->Buffer)->Buffer
                );
            SetWindowText(context->SubStatusHandle, PhaFormatString(
                L"Progress: %s of %s (%.2f%%)",
                PhaFormatSize(ExtractCurrentLength, -1)->Buffer,
                PhaFormatSize(ExtractTotalLength, -1)->Buffer,
                (FLOAT)((double)ExtractCurrentLength / (double)ExtractTotalLength) * 100
                )->Buffer);
            SendMessage(context->ProgressHandle, PBM_SETPOS, (WPARAM)ExtractCurrentLength, 0);

            PhDereferenceObject(currentFile);
        }
        break;
    case WM_END_SETUP:
        {
            SetupRunning = FALSE;

            SetWindowText(context->MainHeaderHandle, L"Setup Complete");
            SetWindowText(context->StatusHandle, L"Extract complete");
            Button_SetText(context->PropSheetCancelHandle, L"Close");

            if (context->SetupStartAppAfterExit)
            {
                SetupExecuteProcessHacker(context);

                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_FINISH);
            }
        }
        break;
    }

    return FALSE;
}
