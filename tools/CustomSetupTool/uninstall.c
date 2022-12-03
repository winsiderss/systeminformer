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

NTSTATUS SetupUninstallBuild(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    // Stop the application.
    if (!ShutdownApplication())
        goto CleanupExit;

    // Stop the kernel driver(s).
    if (!SetupUninstallDriver(Context))
        goto CleanupExit;

    // Remove autorun and shortcuts.
    SetupDeleteWindowsOptions(Context);

    // Remove the uninstaller.
    SetupDeleteUninstallFile(Context);

    // Remove the ARP uninstall entry.
    SetupDeleteUninstallKey();

    // Remove the previous installation.
    if (!NT_SUCCESS(PhDeleteDirectoryWin32(Context->SetupInstallPath)))
    {
        static PH_STRINGREF ksiFileName = PH_STRINGREF_INIT(L"ksi.dll");
        static PH_STRINGREF ksiOldFileName = PH_STRINGREF_INIT(L"ksi.dll-old");
        PPH_STRING ksiFile;
        PPH_STRING ksiOldFile;

        ksiFile = PhConcatStringRef3(
            &Context->SetupInstallPath->sr,
            &PhNtPathSeperatorString,
            &ksiFileName
            );

        ksiOldFile = PhConcatStringRef3(
            &Context->SetupInstallPath->sr,
            &PhNtPathSeperatorString,
            &ksiOldFileName
            );

        PhMoveFileWin32(PhGetString(ksiFile), PhGetString(ksiOldFile), FALSE);

        MoveFileExW(PhGetString(ksiOldFile), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        MoveFileExW(PhGetString(Context->SetupInstallPath), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

        PhDereferenceObject(ksiOldFile);
        PhDereferenceObject(ksiFile);

        Context->NeedsReboot = TRUE;
    }

    // Remove the application data.
    if (Context->SetupRemoveAppData)
        SetupDeleteAppdataDirectory(Context);

    PostMessage(Context->DialogHandle, SETUP_SHOWUNINSTALLFINAL, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:
    PostMessage(Context->DialogHandle, SETUP_SHOWUNINSTALLERROR, 0, 0);
    return STATUS_UNSUCCESSFUL;
}

HRESULT CALLBACK TaskDialogUninstallConfirmCallbackProc(
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
                ShowUninstallingPageDialog(context);
                return S_FALSE;
            }
        }
        break;
    case TDN_VERIFICATION_CLICKED:
        {
            context->SetupRemoveAppData = !!(BOOL)wParam;
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK TaskDialogUninstallErrorCallbackProc(
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
            if ((INT)wParam == IDRETRY)
            {
                ShowUninstallingPageDialog(context);
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK TaskDialogUninstallCallbackProc(
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

            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), SetupUninstallBuild, context);
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK TaskDialogUninstallCompleteCallbackProc(
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
        SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_POS, 100, 0);
        break;
    }

    return S_OK;
}

VOID ShowUninstallCompletedPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = TaskDialogUninstallCompleteCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"System Informer has been uninstalled.";
    if (Context->NeedsReboot)
        config.pszContent = L"A reboot is required to complete the uninstall.";
    else
        config.pszContent = L"Click close to exit setup.";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

VOID ShowUninstallingPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_MARQUEE_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = TaskDialogUninstallCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Uninstalling System Informer...";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

VOID ShowUninstallErrorPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = TaskDialogUninstallErrorCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"System Informer could not be uninstalled.";
    config.pszContent = L"Click retry to try again or close to exit setup.";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

VOID ShowUninstallPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOG_BUTTON buttonArray[] =
    {
        { IDYES, L"Uninstall" }
    };
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.hMainIcon = Context->IconLargeHandle;
    config.pButtons = buttonArray;
    config.cButtons = RTL_NUMBER_OF(buttonArray);
    config.pfCallback = TaskDialogUninstallConfirmCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = PhApplicationName;
    config.pszContent = L"Are you sure you want to uninstall System Informer?";
    config.pszVerificationText = L"Remove application settings";
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.nDefaultButton = IDCANCEL;

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}
