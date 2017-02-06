/*
 * Process Hacker Plugins -
 *   Update Checker Plugin
 *
 * Copyright (C) 2016 dmex
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

#include "updater.h"

static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
{
    { IDOK, L"Download" }
};

HRESULT CALLBACK ShowAvailableCallbackProc(
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
                if (UpdaterInstalledUsingSetup())
                {
                    ShowProgressDialog(context);
                    return S_FALSE;
                }
                else
                {
                    if (PhGetIntegerSetting(SETTING_NAME_NIGHTLY_BUILD))
                    {
                        PhShellExecute(hwndDlg, L"https://wj32.org/processhacker/nightly.php", NULL);
                    }
                    else
                    {
                        PhShellExecute(hwndDlg, L"https://wj32.org/processhacker/downloads.php", NULL);
                    }
                }
            }
        }
        break;
    case TDN_HYPERLINK_CLICKED:
        {
            TaskDialogLinkClicked(context);
        }
        break;
    }

    return S_OK;
}

VOID ShowAvailableDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.cxWidth = 200;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = ARRAYSIZE(TaskDialogButtonArray);
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = ShowAvailableCallbackProc;
    config.pszWindowTitle = L"Process Hacker - Updater";

    if (PhGetIntegerSetting(SETTING_NAME_NIGHTLY_BUILD))
    {
        config.pszMainInstruction = L"A new Process Hacker nightly build is available";
        config.pszContent = PhaFormatString(L"Build: %lu.%lu.%lu\r\nDownload size: %s",
            Context->MajorVersion,
            Context->MinorVersion,
            Context->RevisionVersion,
            PhGetStringOrEmpty(Context->Size)
            )->Buffer;

        if (PhIsNullOrEmptyString(Context->BuildMessage))
            config.pszExpandedInformation = L"<A HREF=\"executablestring\">View Changelog</A>";
        else
            config.pszExpandedInformation = PhGetStringOrEmpty(Context->BuildMessage);
    }
    else
    {
        config.pszMainInstruction = L"A new Process Hacker release is available";
        config.pszContent = PhaFormatString(L"Version: %lu.%lu.%lu\r\nDownload size: %s",
            Context->MajorVersion,
            Context->MinorVersion,
            Context->RevisionVersion,
            PhGetStringOrEmpty(Context->Size)
            )->Buffer;
        config.pszExpandedInformation = L"<A HREF=\"executablestring\">View Changelog</A>";
    }

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}