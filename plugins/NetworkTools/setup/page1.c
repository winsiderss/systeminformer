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

#include "..\nettools.h"

static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
{
    { IDOK, L"Download" }
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
    config.hMainIcon = Context->IconLargeHandle;

    config.pszWindowTitle = L"Network Tools - GeoIP Updater";
    config.pszMainInstruction = L"Download the latest GeoIP database?";
    config.pszContent = L"This product includes GeoLite2 data created by MaxMind, available from <a href=\"http://www.maxmind.com\">http://www.maxmind.com</a>\r\n\r\nSelect download to continue.";

    config.cxWidth = 200;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = ARRAYSIZE(TaskDialogButtonArray);
    config.pfCallback = CheckForUpdatesCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}