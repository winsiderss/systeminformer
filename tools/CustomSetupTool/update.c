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

/**
 * Updates an existing System Informer installation.
 *
 * \param Context The setup context.
 * \return Successful or errant status.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS CALLBACK SetupUpdateBuild(
    _In_ PVOID Context
    )
{
    PPH_SETUP_CONTEXT context = (PPH_SETUP_CONTEXT)Context;
    NTSTATUS status;

    context->SetupProgressActive = TRUE;

    //
    // Create the folder.
    //

    SetupSetProgressMarquee(context, TRUE);
    SetupSetProgressText(context, L"Preparing the update directory...", NULL);

    if (!NT_SUCCESS(status = PhCreateDirectoryWin32(&context->SetupInstallPath->sr)))
    {
        context->LastStatus = status;
        goto CleanupExit;
    }

    //
    // Stop the application.
    //

    SetupSetProgressText(context, L"Stopping System Informer...", NULL);

    if (!NT_SUCCESS(status = SetupShutdownApplication(context)))
    {
        context->LastStatus = status;
        goto CleanupExit;
    }

    //
    // Stop the kernel driver.
    //

    SetupSetProgressText(context, L"Stopping the kernel driver...", NULL);

    if (!NT_SUCCESS(status = SetupUninstallDriver(context)))
    {
        context->LastStatus = status;
        goto CleanupExit;
    }

    //
    // Create the uninstaller.
    //

    SetupSetProgressText(context, L"Updating the uninstaller...", NULL);

    if (!NT_SUCCESS(status = SetupCreateUninstallFile(context)))
    {
        context->LastStatus = status;
        goto CleanupExit;
    }

    //
    // Extract the updated files.
    //

    SetupSetProgressText(context, L"Extracting updated files...", NULL);

    if (!NT_SUCCESS(status = SetupExtractBuild(context)))
    {
        context->LastStatus = status;
        goto CleanupExit;
    }

    //
    // Upgrade the settings file.
    //
    SetupSetProgressText(context, L"Updating settings...", NULL);
    SetupUpgradeSettingsFile();

    //
    // Convert the settings file.
    //
    SetupSetProgressText(context, L"Converting settings...", NULL);
    SetupConvertSettingsFile();

    //
    // Create the ARP uninstall config.
    //
    SetupSetProgressText(context, L"Updating uninstall registration...", NULL);
    SetupCreateUninstallKey(Context);

    //
    // Create Windows Error Reporting config.
    //
    SetupSetProgressText(context, L"Updating LocalDumps configuration...", NULL);
    SetupCreateLocalDumpsKey();

    //
    // Create the application path config.
    //
    SetupSetProgressText(context, L"Updating Windows integration...", NULL);
    SetupCreateWindowsOptions(Context);

    SetupSetProgressText(context, L"Update complete.", NULL);
    SetupSetProgressValue(context, 100);
    context->SetupProgressActive = FALSE;
    PostMessage(context->DialogHandle, SETUP_SHOWUPDATEFINAL, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:

    SetupSetProgressText(context, L"Update failed.", NULL);
    context->SetupProgressActive = FALSE;
    PostMessage(context->DialogHandle, SETUP_SHOWUPDATEERROR, 0, 0);
    return STATUS_UNSUCCESSFUL;
}

/**
 * Callback for the update progress task dialog.
 *
 * \param hwndDlg The task dialog window handle.
 * \param uMsg The notification message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \param dwRefData The setup context.
 * \return S_OK to continue, otherwise an HRESULT value.
 */
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
            SetupApplyDarkModeToPage(hwndDlg);
            SetupSetProgressMarquee(context, TRUE);

            PhCreateThread2(SetupUpdateBuild, context);
        }
        break;
    }

    return S_OK;
}

/**
 * Callback for the update completed task dialog.
 *
 * \param hwndDlg The task dialog window handle.
 * \param uMsg The notification message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \param dwRefData The setup context.
 * \return S_OK to continue, otherwise an HRESULT value.
 */
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

/**
 * Callback for the update error task dialog.
 *
 * \param hwndDlg The task dialog window handle.
 * \param uMsg The notification message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \param dwRefData The setup context.
 * \return S_OK to continue, otherwise an HRESULT value.
 */
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
    case TDN_NAVIGATED:
        SetupApplyDarkModeToPage(hwndDlg);
        break;
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

/**
 * Shows the update progress page.
 *
 * \param Context The setup context.
 */
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

/**
 * Shows the update completed page.
 *
 * \param Context The setup context.
 */
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

/**
 * Shows the update error page.
 *
 * \param Context The setup context.
 */
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
