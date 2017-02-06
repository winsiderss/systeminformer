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

#include "..\main.h"

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
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDOK)
            {
                //if (UpdaterInstalledUsingSetup())
                {
                    ShowProgressDialog(context);
                    return S_FALSE;
                }
                //else
                {
                    //PhShellExecute(hwndDlg, L"https://wj32.org/processhacker/downloads.php", NULL);
                }
            }
        }
        break;
    case TDN_HYPERLINK_CLICKED:
        {
            //TaskDialogLinkClicked(context);
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
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS | TDF_SHOW_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;

    config.pszWindowTitle = L"Process Hacker - Plugin Manager";
    config.pszMainInstruction = PhIsNullOrEmptyString(Context->Node->Name) ? PhGetStringOrEmpty(Context->Node->InternalName) : PhGetStringOrEmpty(Context->Node->Name);
    config.pszContent = PhaFormatString(L"%s\r\n\r\nAuthor: %s\r\nVersion: %s\r\nUpdated: %s",
        PhGetStringOrEmpty(Context->Node->Description),
        PhGetStringOrEmpty(Context->Node->Author),
        PhGetStringOrEmpty(Context->Node->Version),
        PhGetStringOrEmpty(Context->Node->UpdatedTime)
        )->Buffer;
    //config.pszExpandedInformation = L"<A HREF=\"executablestring\">View Changelog</A>";

    config.cxWidth = 200;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = ARRAYSIZE(TaskDialogButtonArray);

    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = ShowAvailableCallbackProc;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}