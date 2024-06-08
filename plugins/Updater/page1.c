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
    case TDN_RADIO_BUTTON_CLICKED:
        {
            PH_RELEASE_CHANNEL channel;

            switch ((INT)wParam)
            {
            default:
            case IDOK:
                channel = PhReleaseChannel;
                break;
            case IDRETRY:
                channel = PhCanaryChannel;
                break;
            }

            if (PhGetPhReleaseChannel() != channel)
            {
                context->Channel = channel;
                context->SwitchingChannel = TRUE;
                PhSetIntegerSetting(L"ReleaseChannel", channel);
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
    static TASKDIALOG_BUTTON UpdateTaskDialogButtonArray[] =
    {
        { IDOK, L"Check" }
    };
    //static TASKDIALOG_BUTTON SwitchTaskDialogButtonArray[] =
    //{
    //    { IDOK, L"Yes" }
    //};
    static TASKDIALOG_BUTTON checkForUpdatesRadioButtons[] =
    {
        { IDOK, L"Stable\n - Recommended" },
        { IDRETRY, L"Canary\n - Preview" },
        //{ IDIGNORE, L"Stable\n - Recommended" },
        //{ IDCONTINUE, L"Canary\n - Preview" },
    };
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS | TDF_EXPAND_FOOTER_AREA;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.pRadioButtons = checkForUpdatesRadioButtons;
    config.cRadioButtons = RTL_NUMBER_OF(checkForUpdatesRadioButtons);
    config.pfCallback = CheckForUpdatesCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.cxWidth = 200;

    config.pszWindowTitle = L"System Informer - Updater";

    switch (Context->Channel)
    {
    default:
    case PhReleaseChannel:
        config.nDefaultRadioButton = IDOK;
        break;
    case PhCanaryChannel:
        config.nDefaultRadioButton = IDRETRY;
        break;
    }

    //if (Context->SwitchingChannel)
    //{
    //    config.pButtons = SwitchTaskDialogButtonArray;
    //    config.cButtons = RTL_NUMBER_OF(SwitchTaskDialogButtonArray);
    //
    //    switch (Context->Channel)
    //    {
    //    case PhReleaseChannel:
    //        config.pszMainInstruction = L"Switch to the System Informer release channel?";
    //        break;
    //    //case PhPreviewChannel:
    //    //    config.pszMainInstruction = L"Switch to the System Informer preview channel?";
    //    //    break;
    //    case PhCanaryChannel:
    //        config.pszMainInstruction = L"Switch to the System Informer canary channel?";
    //        break;
    //    //case PhDeveloperChannel:
    //    //    config.pszMainInstruction = L"Switch to the System Informer developer channel?";
    //    //    break;
    //    default:
    //        config.pszMainInstruction = L"Switch the System Informer update channel?";
    //        break;
    //    }
    //
    //    //if (Context->Channel < PhGetPhReleaseChannel())
    //    //{
    //    //    config.pszContent = L"Downgrading the channel might cause instability.\r\n\r\nClick Yes to continue.\r\n";
    //    //}
    //    //else
    //    {
    //        config.pszContent = L"Click Yes to continue.";
    //    }
    //}
    //else
    {
        config.pButtons = UpdateTaskDialogButtonArray;
        config.cButtons = RTL_NUMBER_OF(UpdateTaskDialogButtonArray);
        config.pszMainInstruction = L"Check for an updated System Informer release?";
        config.pszContent = L"Click Check to continue.";
    }


    TaskDialogNavigatePage(Context->DialogHandle, &config);
}
