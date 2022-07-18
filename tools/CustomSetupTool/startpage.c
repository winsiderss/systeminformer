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

#include <setup.h>

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
            }
        }
        break;
    }

    return S_OK;
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
    config.cButtons = RTL_NUMBER_OF(buttonArray);
    config.pfCallback = SetupWelcomePageCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = PhApplicationName;
    config.pszContent = L"A free, powerful, multi-purpose tool that helps you monitor system resources, debug software and detect malware.";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
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

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = PhaConcatStrings2(PhApplicationName, L" complete.")->Buffer;
    config.pszContent = L"Select Close to exit setup.";
    config.pszVerificationText = L"Start program when setup exits";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
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

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Setup failed with an error.";
    config.pszContent = L"Select Close to exit setup.";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}
