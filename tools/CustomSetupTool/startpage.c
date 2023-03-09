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

VOID SetupShowBrowseDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PVOID fileDialog;

    if (fileDialog = PhCreateOpenFileDialog())
    {
        PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_PICKFOLDERS);

        if (PhShowFileDialog(Context->DialogHandle, fileDialog))
        {
            PPH_STRING fileDialogFolderPath;

            fileDialogFolderPath = PhGetFileDialogFileName(fileDialog);
            PhTrimToNullTerminatorString(fileDialogFolderPath);
            PhSwapReference(&Context->SetupInstallPath, fileDialogFolderPath);
        }

        PhFreeFileDialog(fileDialog);
    }

    if (PhIsNullOrEmptyString(Context->SetupInstallPath))
    {
        Context->SetupInstallPath = SetupFindInstallDirectory();
    }

    if (!PhIsNullOrEmptyString(Context->SetupInstallPath))
    {
        if (!PhEndsWithStringRef(&Context->SetupInstallPath->sr, &PhNtPathSeperatorString, TRUE))
        {
            PhSwapReference(&Context->SetupInstallPath, PhConcatStringRef2(&Context->SetupInstallPath->sr, &PhNtPathSeperatorString));
        }
    }
}

NTSTATUS SetupDownloadProgressThread(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    if (!SetupQueryUpdateData(Context))
        goto CleanupExit;

    if (!UpdateDownloadUpdateData(Context))
        goto CleanupExit;

    PostMessage(Context->DialogHandle, SETUP_SHOWINSTALL, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:
    PostMessage(Context->DialogHandle, SETUP_SHOWERROR, 0, 0);
    return STATUS_FAIL_CHECK;
}

static BOOLEAN SetupCheckDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PH_STRINGREF baseName;

    baseName.Buffer = Information->FileName;
    baseName.Length = Information->FileNameLength;

    if (PhEqualStringRef2(&baseName, L".", TRUE) || PhEqualStringRef2(&baseName, L"..", TRUE))
        return TRUE;

    (*(PULONG)Context) += 1;
    return FALSE;
}

BOOLEAN SetupShowDirectoryWarningPrompt(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    if (PhDoesFileExistWin32(PhGetString(Context->SetupInstallPath)))
    {
        HANDLE directoryHandle;
        ULONG count = 0;

        if (NT_SUCCESS(PhCreateFileWin32(
            &directoryHandle,
            PhGetString(Context->SetupInstallPath),
            FILE_LIST_DIRECTORY | SYNCHRONIZE,
            FILE_ATTRIBUTE_DIRECTORY,
            FILE_SHARE_READ,
            FILE_OPEN,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            PhEnumDirectoryFile(directoryHandle, NULL, SetupCheckDirectoryCallback, &count);
            NtClose(directoryHandle);
        }

        if (count != 0)
        {
            //PhShowMessage2(
            //    Context->DialogHandle,
            //    TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
            //    TD_WARNING_ICON,
            //    L"WARNING",
            //    L"The installation directory already contains files and data. Please select a different directory"
            //    L" or click Yes to delete the files and data and continue. Are you sure you want to continue?"
            //    );

            return TRUE;
        }
    }

    return FALSE;
}

VOID ShowErrorPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Setup failed with an error.";
    config.pszContent = L"Select Close to exit setup.";
    config.cxWidth = 200;

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

HRESULT CALLBACK SetupWelcomePageCallbackProc(
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
            PhCenterWindow(hwndDlg, NULL);

            if (!PhGetOwnTokenAttributes().Elevated)
            {
                SendMessage(hwndDlg, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDCONTINUE, TRUE);
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCONTINUE)
            {
#ifndef FORCE_TEST_UPDATE_LOCAL_INSTALL
                if (PhGetOwnTokenAttributes().Elevated)
                {
                    ShowConfigPageDialog(context);
                    return S_FALSE;
                }
                else
                {
                    SHELLEXECUTEINFO info;

                    memset(&info, 0, sizeof(SHELLEXECUTEINFO));
                    info.cbSize = sizeof(SHELLEXECUTEINFO);
                    info.lpFile = NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer;
                    info.lpParameters = NtCurrentPeb()->ProcessParameters->CommandLine.Buffer;
                    info.lpVerb = L"runas";
                    info.nShow = SW_SHOW;
                    info.hwnd = hwndDlg;
                    info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;

                    if (ShellExecuteEx(&info))
                    {
                        NtTerminateProcess(NtCurrentProcess(), STATUS_SUCCESS);
                    }
                    else
                    {
                        return S_FALSE;
                    }
                }
#else
                ShowConfigPageDialog(context);
                return S_FALSE;
#endif
            }
        }
        break;
    }

    return S_OK;
}

VOID ShowWelcomePageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOG_BUTTON buttonArray[] =
    {
        { IDCONTINUE, L"Install" }
    };
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pButtons = buttonArray;
    config.cButtons = ARRAYSIZE(buttonArray);
    config.pfCallback = SetupWelcomePageCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = PhApplicationName;
    config.pszContent = L"A free, powerful, multi-purpose tool that helps you monitor system resources, debug software and detect malware.";
    config.cxWidth = 200;

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

HRESULT CALLBACK SetupCompletePageCallbackProc(
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
        break;
    }

    return S_OK;
}

VOID ShowCompletedPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_VERIFICATION_FLAG_CHECKED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupCompletePageCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = PhaConcatStrings2(PhApplicationName, L" complete.")->Buffer;
    config.pszContent = L"Select Close to exit setup.";
    config.pszVerificationText = L"Start program when setup exits";
    config.cxWidth = 200;

#ifdef FORCE_TEST_UPDATE_LOCAL_INSTALL
    ClearFlag(config.dwFlags, TDF_VERIFICATION_FLAG_CHECKED);
#endif

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

HRESULT CALLBACK SetupConfigPageCallbackProc(
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
            PPH_STRING status;

            status = PhFormatString(
                L"Installation Folder:\r\n\r\n%s",
                PhGetStringOrEmpty(context->SetupInstallPath)
                );
            SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)status->Buffer);
            PhDereferenceObject(status);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDYES)
            {
                PPH_STRING status;

                SetupShowBrowseDialog(context);

                status = PhFormatString(
                    L"Installation Folder:\r\n\r\n%s",
                    PhGetStringOrEmpty(context->SetupInstallPath)
                    );
                SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)status->Buffer);
                PhDereferenceObject(status);

                return S_FALSE;
            }

            if ((INT)wParam == IDOK)
            {
                if (PhIsNullOrEmptyString(context->SetupInstallPath))
                    return S_FALSE;

                //if (SetupShowDirectoryWarningPrompt(context))
                //    return S_FALSE;

                if (SetupShowDirectoryWarningPrompt(context))
                {
                    ShowConfigDirectoryNonEmptyDialog(context);
                    return S_FALSE;
                }

#ifdef PH_BUILD_API
                ShowInstallPageDialog(context);
#else
                ShowDownloadPageDialog(context);
#endif
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID ShowConfigPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOG_BUTTON buttonConfig[] =
    {
        { IDYES, L"Browse" },
        { IDOK, L"Next" },
    };
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pButtons = buttonConfig;
    config.cButtons = ARRAYSIZE(buttonConfig);
    config.pfCallback = SetupConfigPageCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Setup Options";
    config.pszContent = L"Installation Folder:\r\n\r\nSelect \"Browse\" to continue.";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

HRESULT CALLBACK SetupDirectoryNonEmptyTaskDialogCallbackProc(
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
            if ((INT)wParam == IDNO)
            {
#ifdef PH_BUILD_API
                ShowInstallPageDialog(context);
#else
                ShowDownloadPageDialog(context);
#endif
                return S_FALSE;
            }

            if ((INT)wParam == IDYES)
            {
                ShowConfigPageDialog(context);
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID ShowConfigDirectoryNonEmptyDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOG_BUTTON buttonConfig[] =
    {
        { IDYES, L"Change directory" },
        { IDNO, L"Continue" },
    };
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.pszMainIcon = TD_WARNING_ICON;
    config.pButtons = buttonConfig;
    config.cButtons = ARRAYSIZE(buttonConfig);
    config.pfCallback = SetupDirectoryNonEmptyTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"WARNING";
    config.pszContent = L"The selected installation directory already contains files and data. "
        L"If you continue this directory and files will be deleted.\r\n\r\nDo you want to change the directory?";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

HRESULT CALLBACK SetupDownloadPageCallbackProc(
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

            PhCreateThread2(SetupDownloadProgressThread, context);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCANCEL)
            {
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID ShowDownloadPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.cxWidth = 200;
    config.pfCallback = SetupDownloadPageCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Downloading System Informer...";
    config.pszContent = L"Downloading System Informer...";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
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

            PhCreateThread2(SetupProgressThread, context);
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
