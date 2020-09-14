/*
 * Process Hacker Plugins -
 *   Update Checker Plugin
 *
 * Copyright (C) 2016-2019 dmex
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
                ShowProgressDialog(context);
                return S_FALSE;
            }
        }
        break;
    case TDN_HYPERLINK_CLICKED:
        {
            TaskDialogLinkClicked(context);
            return S_FALSE;
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
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.cxWidth = 200;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = RTL_NUMBER_OF(TaskDialogButtonArray);
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = ShowAvailableCallbackProc;

    config.pszWindowTitle = L"Process Hacker - Updater";
    config.pszMainInstruction = L"A newer build of Process Hacker is available.";
    config.pszContent = PhaFormatString(L"Version: %s\r\nDownload size: %s\r\n\r\n<A HREF=\"changelog.txt\">View changelog</A>",
        PhGetStringOrEmpty(Context->Version),
        PhGetStringOrEmpty(Context->SetupFileLength)
        )->Buffer;

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}
