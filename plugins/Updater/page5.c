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
#include <shellapi.h>
#include <shlobj.h>

static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
{
    { IDYES, L"Install" }
};

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
        {
            INT buttonId = (INT)wParam;

            if (buttonId == IDRETRY)
            {
                ShowCheckForUpdatesDialog(context);
                return S_FALSE;
            }
            else if (buttonId == IDYES)
            {
                SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };

                if (PhIsNullOrEmptyString(context->SetupFilePath))
                    break;

                info.lpFile = PhGetStringOrEmpty(context->SetupFilePath);
                info.lpParameters = L"-update";
                info.lpVerb = PhGetOwnTokenAttributes().Elevated ? NULL : L"runas";
                info.nShow = SW_SHOW;
                info.hwnd = hwndDlg;

                ProcessHacker_PrepareForEarlyShutdown(PhMainWindowHandle);

                if (ShellExecuteEx(&info))
                {
                    ProcessHacker_Destroy(PhMainWindowHandle);
                }
                else
                {
                    // Install failed, cancel the shutdown.
                    ProcessHacker_CancelEarlyShutdown(PhMainWindowHandle);

                    // Set button text for next action
                    //Button_SetText(GetDlgItem(hwndDlg, IDOK), L"Retry");
                    return S_FALSE;
                }
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

VOID ShowUpdateInstallDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = ARRAYSIZE(TaskDialogButtonArray);

    config.pszWindowTitle = L"Process Hacker - Updater";
    config.pszMainInstruction = L"Ready to install update?";
    config.pszContent = L"The update has been successfully downloaded and verified.\r\n\r\nClick Install to continue.";

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

VOID ShowLatestVersionDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;
    LARGE_INTEGER time;
    SYSTEMTIME systemTime = { 0 };
    PIMAGE_DOS_HEADER imageDosHeader;
    PIMAGE_NT_HEADERS imageNtHeader;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS | TDF_EXPAND_FOOTER_AREA;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    
    // HACK
    imageDosHeader = (PIMAGE_DOS_HEADER)PhImageBaseAddress;
    imageNtHeader = (PIMAGE_NT_HEADERS)PTR_ADD_OFFSET(imageDosHeader, (ULONG)imageDosHeader->e_lfanew);
    RtlSecondsSince1970ToTime(imageNtHeader->FileHeader.TimeDateStamp, &time);
    PhLargeIntegerToLocalSystemTime(&systemTime, &time);

    config.pszWindowTitle = L"Process Hacker - Updater";
    config.pszMainInstruction = L"You're running the latest version.";
    config.pszContent = PhaFormatString(
        L"Version: v%lu.%lu.%lu\r\nCompiled: %s\r\n\r\n<A HREF=\"changelog.txt\">View Changelog</A>",
        Context->CurrentMajorVersion,
        Context->CurrentMinorVersion,
        Context->CurrentRevisionVersion,
        PhaFormatDateTime(&systemTime)->Buffer
        )->Buffer;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

VOID ShowNewerVersionDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_EXPAND_FOOTER_AREA;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.pszWindowTitle = L"Process Hacker - Updater";
    config.pszMainInstruction = L"You're running a pre-release build.";
    config.pszContent = PhaFormatString(
        L"Pre-release build: v%lu.%lu.%lu\r\n",
        Context->CurrentMajorVersion,
        Context->CurrentMinorVersion,
        Context->CurrentRevisionVersion
        )->Buffer;

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

    config.pszWindowTitle = L"Process Hacker - Updater";
    config.pszMainInstruction = L"Error downloading the update";

    if (SignatureFailed)
    {
        config.pszContent = L"Signature check failed. Click Retry to download the update again.";
    }
    else if (HashFailed)
    {
        config.pszContent = L"Hash check failed. Click Retry to download the update again.";
    }
    else
    {
        if (Context->ErrorCode)
        {
            PPH_STRING message;
            
            message = PhGetMessage(
                GetModuleHandle(L"winhttp.dll"), 
                0xb, 
                GetUserDefaultLangID(), 
                Context->ErrorCode
                );

            //if (PhIsNullOrEmptyString(message))
            //    PhMoveReference(&message, PhGetNtMessage(Context->ErrorCode));

            if (message)
            {          
                config.pszContent = PhaFormatString(L"[%lu] %s", Context->ErrorCode, message->Buffer)->Buffer;
                PhDereferenceObject(message);
            }
            else
            {
                config.pszContent = L"Click Retry to download the update again.";
            }
        }
        else
        {
            config.pszContent = L"Click Retry to download the update again.";
        }
    }

    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}