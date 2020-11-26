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

NTSTATUS SetupProgressThread(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    // Stop Process Hacker.
    if (!ShutdownProcessHacker())
        goto CleanupExit;

    // Stop the kernel driver(s).
    if (!SetupUninstallKph(Context))
        goto CleanupExit;

    // Upgrade the 2.x settings file.
    SetupUpgradeSettingsFile();

    // Remove the previous installation.
    //if (Context->SetupResetSettings)
    //    PhDeleteDirectory(Context->SetupInstallPath);

    // Create the install folder path.
    if (!NT_SUCCESS(PhCreateDirectory(Context->SetupInstallPath)))
        goto CleanupExit;

    // Create the uninstaller.
    if (!SetupCreateUninstallFile(Context))
        goto CleanupExit;

    // Create the ARP uninstall entries.
    SetupCreateUninstallKey(Context);

    // Create autorun and shortcuts.
    SetupSetWindowsOptions(Context);

    // Set the default image execution options.
    //SetupCreateImageFileExecutionOptions();

    // Setup new installation.
    if (!SetupExtractBuild(Context))
        goto CleanupExit;

    // Setup kernel driver.
    //if (Context->SetupInstallKphService)
    //    SetupStartKph(Context, TRUE);

    PostMessage(Context->DialogHandle, SETUP_SHOWFINAL, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:
    PostMessage(Context->DialogHandle, SETUP_SHOWERROR, 0, 0);
    return STATUS_UNSUCCESSFUL;
}

HRESULT CALLBACK SetupInstallPageCallbackProc(
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

            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), SetupProgressThread, context);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            return S_FALSE;
        }
        break;
    }

    return S_OK;
}

VOID ShowInstallPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_MARQUEE_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupInstallPageCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.cxWidth = 200;

    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Preparing to install...";
    config.pszContent = L" ";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}
