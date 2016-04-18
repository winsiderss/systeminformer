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

HRESULT CALLBACK ShowProgressCallbackProc(
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
        {
            HANDLE downloadThreadHandle = NULL;

            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            // Start file download thread
            if (downloadThreadHandle = PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)UpdateDownloadThread, context))
                NtClose(downloadThreadHandle);
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

VOID ShowProgressDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context
    )
{
    PPH_UPDATER_CONTEXT context;
    TASKDIALOGCONFIG config;

    context = (PPH_UPDATER_CONTEXT)Context;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_EXPAND_FOOTER_AREA | TDF_ENABLE_HYPERLINKS | TDF_SHOW_MARQUEE_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = context->IconLargeHandle;

    config.pszWindowTitle = L"Process Hacker - Updater";
    config.pszMainInstruction = PhaFormatString(L"Downloading update %lu.%lu.%lu...",
        context->MajorVersion,
        context->MinorVersion,
        context->RevisionVersion
        )->Buffer;
    config.pszContent = L"Downloaded: ~ of ~ (0%)\r\nSpeed: ~ KB/s";
    config.pszExpandedInformation = L"<A HREF=\"executablestring\">View Changelog</A>";   
    
    config.cxWidth = 200;
    config.lpCallbackData = Context;
    config.pfCallback = ShowProgressCallbackProc;

    SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}