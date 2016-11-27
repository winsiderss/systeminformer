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
#include <shellapi.h>
#include <shlobj.h>

static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
{
    { IDYES, L"Restart" }
};

HRESULT CALLBACK RestartTaskDialogCallbackProc(
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
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDYES)
            {
                SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };

               /* if (PhIsNullOrEmptyString(context->SetupFilePath))
                    break;*/

                info.lpFile = L"ProcessHacker.exe";
                info.nShow = SW_SHOW;
                info.hwnd = hwndDlg;
                //info.lpParameters = L"-plugin dmex.ExtraPlugins:INSTALL -plugin dmex.ExtraPlugins:hex64value";

                ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);

                if (ShellExecuteEx(&info))
                {
                    NtTerminateProcess(NtCurrentProcess(), STATUS_SUCCESS);
                }
                else
                {
                    // Install failed, cancel the shutdown
                    ProcessHacker_CancelEarlyShutdown(PhMainWndHandle);
                    // Set button text for next action
                    //Button_SetText(GetDlgItem(hwndDlg, IDOK), L"Retry");
                    return S_FALSE;
                }
            }
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK FinalTaskDialogCallbackProc(
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
            if (!PhGetOwnTokenAttributes().Elevated)
            {
                SendMessage(hwndDlg, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDYES, TRUE);
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        break;
    }

    return S_OK;
}

VOID ShowInstallRestartDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;

    config.pszWindowTitle = L"Process Hacker - Plugin Manager";
    config.pszMainInstruction = L"The GeoIP database has been installed";
    config.pszContent = L"You need to restart Process Hacker for the changes to take effect...";

    config.pButtons = TaskDialogButtonArray;
    config.cButtons = ARRAYSIZE(TaskDialogButtonArray);

    config.cxWidth = 205;
    config.pfCallback = RestartTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

VOID ShowLatestVersionDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PPH_UPDATER_CONTEXT context;
    TASKDIALOGCONFIG config;

    context = (PPH_UPDATER_CONTEXT)Context;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = context->IconLargeHandle;

    config.pszWindowTitle = L"Process Hacker - Plugin Manager";
    config.pszMainInstruction = L"You're running the latest version.";
    //config.pszContent = PhaFormatString(
    //    L"Stable release build: v%lu.%lu.%lu\r\n\r\n<A HREF=\"executablestring\">View Changelog</A>",
    //    context->CurrentMajorVersion,
    //    context->CurrentMinorVersion,
    //    context->CurrentRevisionVersion
    //    )->Buffer;

    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}


VOID ShowUpdateFailedDialog(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ BOOLEAN HashFailed,
    _In_ BOOLEAN SignatureFailed
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    //config.pszMainIcon = MAKEINTRESOURCE(65529);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON | TDCBF_RETRY_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;

    config.pszWindowTitle = L"Process Hacker - Plugin Manager";
    config.pszMainInstruction = L"Error downloading plugin files.";

    if (SignatureFailed)
    {
        config.pszContent = L"Signature check failed. Click Retry to download the plugin again.";
    }
    else if (HashFailed)
    {
        config.pszContent = L"Hash check failed. Click Retry to download the plugin again.";
    }
    else
    {
        config.pszContent = L"Click Retry to download the plugin again.";
    }

    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}