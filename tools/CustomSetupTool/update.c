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

NTSTATUS SetupUpdateBuild(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    if (!ShutdownProcessHacker())
    {
        Context->ErrorCode = ERROR_INVALID_FUNCTION;
        goto CleanupExit;
    }

    if (!SetupCreateUninstallFile(Context))
        goto CleanupExit;

    if (!SetupUninstallKph(Context))
        goto CleanupExit;

    if (!SetupExtractBuild(Context))
        goto CleanupExit;

    // Set the default image execution options.
    //SetupCreateImageFileExecutionOptions();

    // Set the default KPH configuration.
    SetupStartKph(Context, FALSE);

    PostMessage(Context->DialogHandle, SETUP_SHOWUPDATEFINAL, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:

    PostMessage(Context->DialogHandle, SETUP_SHOWUPDATEERROR, 0, 0);
    return STATUS_UNSUCCESSFUL;
}

HRESULT CALLBACK SetupUpdatingTaskDialogCallbackProc(
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
            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), SetupUpdateBuild, context);
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK SetupCompletedTaskDialogCallbackProc(
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
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCLOSE)
            {
                SetupExecuteProcessHacker(context);
            }
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK SetupErrorTaskDialogCallbackProc(
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
        break;
    }

    return S_OK;
}

VOID ShowUpdatePageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_MARQUEE_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupUpdatingTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = PhaFormatString(
        L"Updating to version %lu.%lu.%lu...",
        PHAPP_VERSION_MAJOR,
        PHAPP_VERSION_MINOR,
        PHAPP_VERSION_REVISION
        )->Buffer;
    config.pszContent = L" ";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

VOID ShowUpdateCompletedPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupCompletedTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Update complete.";
    config.pszContent = L"Select Close to exit.";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

VOID ShowUpdateErrorPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupErrorTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Error updating to the latest version.";

    if (Context->ErrorCode)
    {
        PPH_STRING errorString;
        
        if (errorString = PhGetStatusMessage(0, Context->ErrorCode))
            config.pszContent = PhGetString(errorString);
    }

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}
