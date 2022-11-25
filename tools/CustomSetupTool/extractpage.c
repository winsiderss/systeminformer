/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

#include "setup.h"

NTSTATUS SetupProgressThread(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    // Stop System Informer.
    if (!ShutdownApplication())
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    // Stop the kernel driver(s).
    if (!SetupUninstallDriver(Context))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    // Upgrade the legacy settings file.
    SetupUpgradeSettingsFile();

    // Remove the previous installation.
    //if (Context->SetupResetSettings)
    //    PhDeleteDirectory(Context->SetupInstallPath);

    // Create the install folder path.
    if (!NT_SUCCESS(PhCreateDirectoryWin32(Context->SetupInstallPath)))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    // Create the uninstaller.
    if (!SetupCreateUninstallFile(Context))
        goto CleanupExit;

    // Create the ARP uninstall entries.
    SetupCreateUninstallKey(Context);

    // Create autorun.
    SetupSetWindowsOptions(Context);

    // Create shortcuts.
    SetupChangeNotifyShortcuts(Context);

    // Set the default image execution options.
    //SetupCreateImageFileExecutionOptions();

    // Setup new installation.
    if (!SetupExtractBuild(Context))
        goto CleanupExit;

    PostMessage(Context->DialogHandle, SETUP_SHOWFINAL, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:
    PostMessage(Context->DialogHandle, SETUP_SHOWERROR, 0, 0);
    return STATUS_UNSUCCESSFUL;
}

HRESULT CALLBACK SetupInstallPageCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
)
{
    PPH_SETUP_CONTEXT context = (PPH_SETUP_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        {
            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), SetupProgressThread, context);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            return S_FALSE;
        }
        break;
    }

    return S_OK;
}

VOID ShowInstallPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_MARQUEE_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupInstallPageCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.cxWidth = 200;

    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Preparing to install...";
    config.pszContent = L" ";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}
