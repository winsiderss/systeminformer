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

NTSTATUS CALLBACK SetupUpdateBuild(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;

    // Create the folder.
    if (!NT_SUCCESS(status = PhCreateDirectoryWin32(&Context->SetupInstallPath->sr)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

    // Stop the application.
    if (!NT_SUCCESS(status = SetupShutdownApplication(Context)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

    // Stop the kernel driver.
    if (!NT_SUCCESS(status = SetupUninstallDriver(Context)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

    // Upgrade the settings file.
    SetupUpgradeSettingsFile();
    // Convert the settings file.
    SetupConvertSettingsFile();

    // Remove the previous installation.
    //if (Context->SetupResetSettings)
    //    PhDeleteDirectory(Context->SetupInstallPath);

    // Create the uninstaller.
    if (!NT_SUCCESS(status = SetupCreateUninstallFile(Context)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

    // Create the ARP uninstall entries.
    SetupCreateUninstallKey(Context);

    // Create autorun.
    SetupCreateWindowsOptions(Context);

    // Create shortcuts.
    //SetupCreateShortcuts(Context);

    // Set the default image execution options.
    //SetupCreateImageFileExecutionOptions();

    // Extract the updated files.
    if (!NT_SUCCESS(status = SetupExtractBuild(Context)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

    PostMessage(Context->DialogHandle, SETUP_SHOWUPDATEFINAL, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:

    PostMessage(Context->DialogHandle, SETUP_SHOWUPDATEERROR, 0, 0);
    return STATUS_UNSUCCESSFUL;
}

HRESULT CALLBACK SetupUpdatingTaskDialogCallbackProc(
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

            PhCreateThread2(SetupUpdateBuild, context);
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK SetupCompletedTaskDialogCallbackProc(
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
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCLOSE)
            {
                SetupExecuteApplication(context);
            }
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK SetupErrorTaskDialogCallbackProc(
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
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDYES)
            {
                ShowUpdatePageDialog(context);
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID ShowUpdatePageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_MARQUEE_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupUpdatingTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = PhaFormatString(
        L"Updating to version %lu.%lu.%lu.%lu...",
        PHAPP_VERSION_MAJOR,
        PHAPP_VERSION_MINOR,
        PHAPP_VERSION_BUILD,
        PHAPP_VERSION_REVISION
        )->Buffer;
    config.pszContent = L" ";

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

VOID ShowUpdateCompletedPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupCompletedTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Update complete.";
    config.pszContent = L"Select Close to exit.";

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

VOID ShowUpdateErrorPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
    {
        { IDYES, L"Retry" },
        { IDCLOSE, L"Close" },
    };
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    //config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupErrorTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = ARRAYSIZE(TaskDialogButtonArray);
    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Error updating to the latest version.";

    if (Context->LastStatus)
    {
        PPH_STRING errorString;

        if (errorString = PhGetStatusMessage(Context->LastStatus, 0))
            config.pszContent = PhGetString(errorString);
    }

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}
