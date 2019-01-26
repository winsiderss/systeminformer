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
#include <setupsup.h>

#define WM_TASKDIALOGINIT (WM_APP + 550)
HWND UpdateDialogHandle = NULL;
HANDLE UpdateDialogThreadHandle = NULL;
PH_EVENT InitializedEvent = PH_EVENT_INIT;

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
    SetupCreateImageFileExecutionOptions();

    // Set the default KPH configuration.
    SetupStartKph(Context, FALSE);

    if (!SetupExecuteProcessHacker(Context))
        goto CleanupExit;

    PostMessage(Context->DialogHandle, WM_QUIT, 0, 0);
    PhDereferenceObject(Context);  
    return STATUS_SUCCESS;

CleanupExit:

    PostMessage(Context->DialogHandle, WM_APP + IDD_ERROR, 0, 0);
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
        PhInstanceHandle,
        MAKEINTRESOURCE(IDI_ICON1),
        PH_LOAD_ICON_SIZE_LARGE,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON)
        );
    smallIcon = PhLoadIcon(
        PhInstanceHandle,
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

VOID SetupShowUpdatingErrorDialog(
    _In_ HWND hwndDlg,
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.cxWidth = 200;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = SetupErrorTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Error updating to the latest version.";

    if (Context->ErrorCode)
    {
        PPH_STRING errorString;
        
        if (errorString = PhGetStatusMessage(0, Context->ErrorCode))
            config.pszContent = PhGetString(errorString);
    }

    SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

LRESULT CALLBACK TaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SETUP_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hwndDlg, UCHAR_MAX)))
        return 0;

    oldWndProc = context->TaskDialogWndProc;

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
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwndDlg, UCHAR_MAX);
        }
        break;
    case WM_APP + IDD_ERROR:
        {
            SetupShowUpdatingErrorDialog(hwndDlg, context);
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwndDlg, uMsg, wParam, lParam);
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

    if (SetupMode == SETUP_COMMAND_SILENTINSTALL)
    {
        config.pszMainInstruction = PhaFormatString(
            L"Installing Process Hacker %lu.%lu.%lu...",
            PHAPP_VERSION_MAJOR,
            PHAPP_VERSION_MINOR,
            PHAPP_VERSION_REVISION
            )->Buffer;
    }
    else
    {
        config.pszMainInstruction = PhaFormatString(
            L"Updating to version %lu.%lu.%lu...",
            PHAPP_VERSION_MAJOR,
            PHAPP_VERSION_MINOR,
            PHAPP_VERSION_REVISION
            )->Buffer;
    }

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
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
            context->DialogHandle = hwndDlg;

            // Center the window on the desktop.
            PhCenterWindow(hwndDlg, NULL);

            // Create the Taskdialog icons.
            TaskDialogCreateIcons(context);

            // Subclass the Taskdialog.
            context->TaskDialogWndProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            PhSetWindowContext(hwndDlg, UCHAR_MAX, context);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)TaskDialogSubclassProc);

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
    TASKDIALOGCONFIG config;
    PH_AUTO_POOL autoPool;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));

    PhInitializeAutoPool(&autoPool);

    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS | TDF_POSITION_RELATIVE_TO_WINDOW;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pszWindowTitle = PhApplicationName;
    config.pfCallback = TaskDialogBootstrapCallback;
    config.lpCallbackData = (LONG_PTR)CreateUpdateContext();

    TaskDialogIndirect(&config, NULL, NULL, NULL);

    PhDeleteAutoPool(&autoPool);
}
