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
#include <appsup.h>
#include <workqueue.h>

typedef struct _PH_SETUP_UNINSTALL_CONTEXT
{
    HWND DialogHandle;
    HICON IconSmallHandle;
    HICON IconLargeHandle;
} PH_SETUP_UNINSTALL_CONTEXT, *PPH_SETUP_UNINSTALL_CONTEXT;

#define WM_TASKDIALOGINIT (WM_APP + 550)
HWND UninstallDialogHandle = NULL;
HANDLE UninstallDialogThreadHandle = NULL;
PH_EVENT UninstallInitializedEvent = PH_EVENT_INIT;

VOID ShowUninstallConfirmDialog(
    _In_ PPH_SETUP_UNINSTALL_CONTEXT Context
    );
VOID ShowUninstallDialog(
    _In_ PPH_SETUP_UNINSTALL_CONTEXT Context
    );
VOID ShowUninstallCompleteDialog(
    _In_ PPH_SETUP_UNINSTALL_CONTEXT Context
    );
VOID ShowUninstallErrorDialog(
    _In_ PPH_SETUP_UNINSTALL_CONTEXT Context
    );

NTSTATUS SetupUninstallBuild(
    _In_ PPH_SETUP_UNINSTALL_CONTEXT Context
    )
{
    SetupFindInstallDirectory();

    // Stop Process Hacker.
    if (!ShutdownProcessHacker())
        goto CleanupExit;

    // Stop the kernel driver(s).
    if (!SetupUninstallKph())
        goto CleanupExit;

    // Remove autorun and shortcuts.
    SetupDeleteWindowsOptions();

    // Remove the uninstaller.
    SetupDeleteUninstallFile();

    // Remove the previous installation.
    if (!RemoveDirectoryPath(PhGetString(SetupInstallPath)))
        goto CleanupExit;

    // Remove the ARP uninstall entry.
    SetupDeleteUninstallKey();

    ShowUninstallCompleteDialog(Context);
    return STATUS_SUCCESS;

CleanupExit:
    ShowUninstallErrorDialog(Context);
    return STATUS_SUCCESS;
}

static VOID TaskDialogCreateIcons(
    _In_ PPH_SETUP_UNINSTALL_CONTEXT Context
    )
{
    HICON largeIcon;
    HICON smallIcon;

    largeIcon = PhLoadIcon(
        NtCurrentPeb()->ImageBaseAddress,
        MAKEINTRESOURCE(IDI_ICON1),
        PH_LOAD_ICON_SIZE_LARGE,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON)
        );
    smallIcon = PhLoadIcon(
        NtCurrentPeb()->ImageBaseAddress,
        MAKEINTRESOURCE(IDI_ICON1),
        PH_LOAD_ICON_SIZE_LARGE,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON)
        );

    Context->IconLargeHandle = largeIcon;
    Context->IconSmallHandle = smallIcon;

    SendMessage(Context->DialogHandle, WM_SETICON, ICON_SMALL, (LPARAM)largeIcon);
    SendMessage(Context->DialogHandle, WM_SETICON, ICON_BIG, (LPARAM)smallIcon);
}

HRESULT CALLBACK TaskDialogUninstallConfirmCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_SETUP_UNINSTALL_CONTEXT context = (PPH_SETUP_UNINSTALL_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDYES)
            {
                ShowUninstallDialog(context);
                return S_FALSE;
            } 
            else if ((INT)wParam == IDRETRY)
            {
                ShowUninstallCompleteDialog(context);
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK TaskDialogUninstallCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_SETUP_UNINSTALL_CONTEXT context = (PPH_SETUP_UNINSTALL_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        {
            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), SetupUninstallBuild, context);
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK TaskDialogUninstallCompleteCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_SETUP_UNINSTALL_CONTEXT context = (PPH_SETUP_UNINSTALL_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_POS, 100, 0);
        break;
    }

    return S_OK;
}

VOID ShowUninstallCompleteDialog(
    _In_ PPH_SETUP_UNINSTALL_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = TaskDialogUninstallCompleteCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Process Hacker has been uninstalled.";
    config.pszContent = L"Click close to exit setup.";
    config.cxWidth = 200;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

VOID ShowUninstallConfirmDialog(
    _In_ PPH_SETUP_UNINSTALL_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
    config.nDefaultButton = IDNO;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = TaskDialogUninstallConfirmCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Process Hacker - Setup";
    config.pszContent = L"Are you sure you want to uninstall Process Hacker?";
    config.cxWidth = 200;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

VOID ShowUninstallDialog(
    _In_ PPH_SETUP_UNINSTALL_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_MARQUEE_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = TaskDialogUninstallCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Uninstalling Process Hacker...";
    config.cxWidth = 200;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

VOID ShowUninstallErrorDialog(
    _In_ PPH_SETUP_UNINSTALL_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = TaskDialogUninstallConfirmCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Process Hacker could not be uninstalled.";
    config.pszContent = L"Click retry to try again or close to exit setup.";
    config.cxWidth = 200;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}
HRESULT CALLBACK TaskDialogUninstallBootstrapCallback(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_SETUP_UNINSTALL_CONTEXT context = (PPH_SETUP_UNINSTALL_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            context->DialogHandle = hwndDlg;

            // Center the window on the desktop
            PhCenterWindow(hwndDlg, NULL);
            // Create the Taskdialog icons
            TaskDialogCreateIcons(context);
            // Navigate to the first page
            ShowUninstallConfirmDialog(context);
        }
        break;
    }

    return S_OK;
}

VOID SetupShowUninstallDialog(
    VOID
    )
{
    PVOID context;
    TASKDIALOGCONFIG config;
    PH_AUTO_POOL autoPool;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));

    PhInitializeAutoPool(&autoPool);
    context = (PPH_SETUP_UNINSTALL_CONTEXT)PhCreateAlloc(sizeof(PH_SETUP_UNINSTALL_CONTEXT));
    memset(context, 0, sizeof(PH_SETUP_UNINSTALL_CONTEXT));

    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pszWindowTitle = PhApplicationName;
    config.pfCallback = TaskDialogUninstallBootstrapCallback;
    config.lpCallbackData = (LONG_PTR)context;

    TaskDialogIndirect(&config, NULL, NULL, NULL);

    PhDereferenceObject(context);
    PhDeleteAutoPool(&autoPool);
}