/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2019
 *
 */

#include "updater.h"

static TASKDIALOG_BUTTON UpdateTaskDialogButtonArray[] =
{
    { IDOK, L"Check" }
};

static TASKDIALOG_BUTTON SwitchTaskDialogButtonArray[] =
{
    { IDOK, L"Yes" }
};

HRESULT CALLBACK CheckForUpdatesCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        PhSetEvent(&InitializedEvent);
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDOK)
            {
                ShowCheckingForUpdatesDialog(context);
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID ShowCheckForUpdatesDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.cxWidth = 200;
    config.pfCallback = CheckForUpdatesCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.pszWindowTitle = L"System Informer - Updater";
    if (Context->SwitchingChannel)
    {
        config.pButtons = SwitchTaskDialogButtonArray;
        config.cButtons = RTL_NUMBER_OF(SwitchTaskDialogButtonArray);

        switch (Context->Channel)
        {
        case PhReleaseChannel:
            config.pszMainInstruction = L"Switch to the System Informer release channel?";
            break;
        //case PhPreviewChannel:
        //    config.pszMainInstruction = L"Switch to the System Informer preview channel?";
        //    break;
        case PhCanaryChannel:
            config.pszMainInstruction = L"Switch to the System Informer canary channel?";
            break;
        //case PhDeveloperChannel:
        //    config.pszMainInstruction = L"Switch to the System Informer developer channel?";
        //    break;
        default:
            config.pszMainInstruction = L"Switch the System Informer update channel?";
            break;
        }

        //if (Context->Channel < (PH_RELEASE_CHANNEL)PhGetIntegerSetting(L"ReleaseChannel"))
        //{
        //    config.pszContent = L"Downgrading the channel might cause instability.\r\n\r\nClick Yes to continue.\r\n";
        //}
        //else
        {
            config.pszContent = L"Click Yes to continue.\r\n";
        }
    }
    else
    {
        config.pButtons = UpdateTaskDialogButtonArray;
        config.cButtons = RTL_NUMBER_OF(UpdateTaskDialogButtonArray);
        config.pszMainInstruction = L"Check for an updated System Informer release?";
        config.pszContent = L"Click Check to continue.\r\n";
    }


    TaskDialogNavigatePage(Context->DialogHandle, &config);
}
