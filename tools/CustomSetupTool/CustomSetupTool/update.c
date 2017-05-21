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

typedef struct _PH_SETUP_UPDATE_CONTEXT
{
    HWND DialogHandle;
    HICON IconSmallHandle;
    HICON IconLargeHandle;

    PPH_STRING FileDownloadUrl;
    PPH_STRING RevVersion;
    PPH_STRING Size;
    PPH_STRING SetupFilePath;
} PH_SETUP_UPDATE_CONTEXT, *PPH_SETUP_UPDATE_CONTEXT;

#define WM_TASKDIALOGINIT (WM_APP + 550)
HWND UpdateDialogHandle = NULL;
HANDLE UpdateDialogThreadHandle = NULL;
PH_EVENT InitializedEvent = PH_EVENT_INIT;

NTSTATUS SetupUpdateBuild(
    _In_ PPH_SETUP_UPDATE_CONTEXT Context
    )
{
    SetupFindInstallDirectory();

    if (!ShutdownProcessHacker())
        goto CleanupExit;

    if (!SetupUninstallKph())
        goto CleanupExit;

    SetupCreateUninstallKey();
    SetupCreateUninstallFile();
    //SetupSetWindowsOptions();

    if (!SetupExtractBuild(Context->DialogHandle))
        goto CleanupExit;

    if (SetupInstallKphService)
        SetupInstallKph();

    if (!SetupExecuteProcessHacker(Context->DialogHandle))
        goto CleanupExit;

    PostMessage(Context->DialogHandle, WM_QUIT, 0, 0);
    PhDereferenceObject(Context);  
    return STATUS_SUCCESS;

CleanupExit:

    PostMessage(Context->DialogHandle, IDD_ERROR, 0, 0);
    PhDereferenceObject(Context);
    return STATUS_FAIL_CHECK;
}


PPH_SETUP_UPDATE_CONTEXT CreateUpdateContext(
    VOID
    )
{
    PPH_SETUP_UPDATE_CONTEXT context;

    context = (PPH_SETUP_UPDATE_CONTEXT)PhCreateAlloc(sizeof(PH_SETUP_UPDATE_CONTEXT));
    memset(context, 0, sizeof(PH_SETUP_UPDATE_CONTEXT));

    return context;
}

VOID FreeUpdateContext(
    _In_ _Post_invalid_ PPH_SETUP_UPDATE_CONTEXT Context
    )
{
    //PhClearReference(&Context->Version);
    //PhClearReference(&Context->RevVersion);
    //PhClearReference(&Context->RelDate);
    //PhClearReference(&Context->Size);
    //PhClearReference(&Context->Hash);
    //PhClearReference(&Context->Signature);
    //PhClearReference(&Context->ReleaseNotesUrl);
    //PhClearReference(&Context->SetupFilePath);
    //PhClearReference(&Context->SetupFileDownloadUrl);

    if (Context->IconLargeHandle)
        DestroyIcon(Context->IconLargeHandle);

    if (Context->IconSmallHandle)
        DestroyIcon(Context->IconSmallHandle);

    //PhClearReference(&Context);
}

VOID TaskDialogCreateIcons(
    _In_ PPH_SETUP_UPDATE_CONTEXT Context
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

VOID TaskDialogLinkClicked(
    _In_ PPH_SETUP_UPDATE_CONTEXT Context
    )
{
    
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
    PPH_SETUP_UPDATE_CONTEXT context = (PPH_SETUP_UPDATE_CONTEXT)dwRefData;

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
    }

    return DefSubclassProc(hwndDlg, uMsg, wParam, lParam);
}

HRESULT CALLBACK CheckingForUpdatesCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_SETUP_UPDATE_CONTEXT context = (PPH_SETUP_UPDATE_CONTEXT)dwRefData;

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

VOID ShowCheckForUpdatesDialog(
    _In_ PPH_SETUP_UPDATE_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = CheckingForUpdatesCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = PhaFormatString(L"Updating to version %lu.%lu.%lu...", PHAPP_VERSION_MAJOR, PHAPP_VERSION_MINOR, PHAPP_VERSION_REVISION)->Buffer;
    config.cxWidth = 200;

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
    PPH_SETUP_UPDATE_CONTEXT context = (PPH_SETUP_UPDATE_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            context->DialogHandle = hwndDlg;

            // Center the window on the desktop
            PhCenterWindow(hwndDlg, NULL);
            // Create the Taskdialog icons
            TaskDialogCreateIcons(context);
            // Subclass the Taskdialog
            SetWindowSubclass(hwndDlg, TaskDialogSubclassProc, 0, (ULONG_PTR)context);
            // Navigate to the first page
            ShowCheckForUpdatesDialog(context);

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

    FreeUpdateContext(context);
    PhDeleteAutoPool(&autoPool);
}