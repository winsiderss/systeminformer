/*
 * Process Hacker Toolchain -
 *   project setup
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

#include <setup.h>

NTSTATUS SetupDownloadProgressThread(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    if (!SetupQueryUpdateData(Context))
        goto CleanupExit;

    if (!UpdateDownloadUpdateData(Context))
        goto CleanupExit;

    PostMessage(Context->DialogHandle, SETUP_SHOWINSTALL, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:

    PostMessage(Context->DialogHandle, SETUP_SHOWERROR, 0, 0);
    return STATUS_FAIL_CHECK;
}

HRESULT CALLBACK SetupDownloadPageCallbackProc(
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
            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), SetupDownloadProgressThread, context);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCANCEL)
            {
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID ShowDownloadPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.cxWidth = 200;
    config.pfCallback = SetupDownloadPageCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Preparing download...";
    config.pszContent = L"Downloaded: ~ of ~ (0%)\r\nSpeed: ~ KB/s";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

