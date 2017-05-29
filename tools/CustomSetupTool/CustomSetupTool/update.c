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

#define WM_TASKDIALOGINIT (WM_APP + 550)
HWND UpdateDialogHandle = NULL;
HANDLE UpdateDialogThreadHandle = NULL;
PH_EVENT InitializedEvent = PH_EVENT_INIT;

NTSTATUS SetupUpdateBuild(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    Context->SetupInstallPath = SetupFindInstallDirectory();

    if (!ShutdownProcessHacker())
        goto CleanupExit;

    if (!SetupUninstallKph(Context))
        goto CleanupExit;

    if (!SetupCreateUninstallFile(Context))
        goto CleanupExit;

    SetupCreateUninstallKey(Context);

    if (!SetupExtractBuild(Context))
        goto CleanupExit;

    SetupStartKph(Context);

    if (!SetupExecuteProcessHacker(Context))
        goto CleanupExit;

    PostMessage(Context->PropSheetHandle, WM_QUIT, 0, 0);
    PhDereferenceObject(Context);  
    return STATUS_SUCCESS;

CleanupExit:

    PostMessage(Context->PropSheetHandle, WM_APP + IDD_ERROR, 0, 0);
    PhDereferenceObject(Context);
    return STATUS_FAIL_CHECK;
}

PPH_SETUP_CONTEXT CreateUpdateContext(
    VOID
    )
{
    PPH_SETUP_CONTEXT context;

    context = (PPH_SETUP_CONTEXT)PhCreateAlloc(sizeof(PH_SETUP_CONTEXT));
    memset(context, 0, sizeof(PH_SETUP_CONTEXT));

    return context;
}

VOID TaskDialogCreateIcons(
    _In_ PPH_SETUP_CONTEXT Context
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

    SendMessage(Context->PropSheetHandle, WM_SETICON, ICON_SMALL, (LPARAM)largeIcon);
    SendMessage(Context->PropSheetHandle, WM_SETICON, ICON_BIG, (LPARAM)smallIcon);
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
        {

        }
        break;
    }

    return S_OK;
}

VOID SetupShowUpdatingErrorDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.cxWidth = 200;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupErrorTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = PhaFormatString(
        L"Error updating to the latest version...",
        PHAPP_VERSION_MAJOR,
        PHAPP_VERSION_MINOR,
        PHAPP_VERSION_REVISION
        )->Buffer;

    if (Context->ErrorCode)
    {
        PPH_STRING errorString;
        
        if (errorString = PhGetStatusMessage(0, Context->ErrorCode))
            config.pszContent = PhGetString(errorString);
    }

    SendMessage(Context->PropSheetHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

LRESULT CALLBACK TaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
    )
{
    PPH_SETUP_CONTEXT context = (PPH_SETUP_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case WM_TASKDIALOGINIT:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hwndDlg, TaskDialogSubclassProc, uIdSubclass);
        }
        break;
    case WM_APP + IDD_ERROR:
        {
            SetupShowUpdatingErrorDialog(context);
        }
        break;
    }

    return DefSubclassProc(hwndDlg, uMsg, wParam, lParam);
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

VOID SetupShowUpdatingDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.cxWidth = 200;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupUpdatingTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = PhaFormatString(
        L"Updating to version %lu.%lu.%lu...", 
        PHAPP_VERSION_MAJOR, 
        PHAPP_VERSION_MINOR, 
        PHAPP_VERSION_REVISION
        )->Buffer;

    SendMessage(Context->PropSheetHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

HRESULT CALLBACK TaskDialogBootstrapCallback(
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
    case TDN_CREATED:
        {
            context->PropSheetHandle = hwndDlg;

            // Center the window on the desktop.
            PhCenterWindow(hwndDlg, NULL);
            // Create the Taskdialog icons.
            TaskDialogCreateIcons(context);
            // Subclass the Taskdialog.
            SetWindowSubclass(hwndDlg, TaskDialogSubclassProc, 0, (ULONG_PTR)context);
            // Navigate to the first page.
            SetupShowUpdatingDialog(context);

            SendMessage(hwndDlg, WM_TASKDIALOGINIT, 0, 0);
        }
        break;
    }

    return S_OK;
}

VOID SetupShowUpdateDialog(
    VOID
    )
{
    PVOID context;
    TASKDIALOGCONFIG config;
    PH_AUTO_POOL autoPool;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));

    PhInitializeAutoPool(&autoPool);

    context = CreateUpdateContext();
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pszWindowTitle = PhApplicationName;
    config.pfCallback = TaskDialogBootstrapCallback;
    config.lpCallbackData = (LONG_PTR)context;

    TaskDialogIndirect(&config, NULL, NULL, NULL);

    PhDeleteAutoPool(&autoPool);
}