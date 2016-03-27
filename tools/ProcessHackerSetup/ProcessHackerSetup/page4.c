#include <setup.h>
#include <appsup.h>

NTSTATUS DownloadThread(
    _In_ PVOID Arguments
    )
{
    BOOLEAN setupSuccess = FALSE;

    // Download the latest build
    if (setupSuccess = SetupDownloadBuild(Arguments))
    {
        // Reset the current installation
        if (setupSuccess = SetupResetCurrentInstall(Arguments))
        {
            // Extract and install the latest build
            if (setupSuccess = SetupExtractBuild(Arguments))
            {
                PostMessage(Arguments, PSM_SETCURSELID, 0, IDD_DIALOG5);
            }
        }
    }

    if (!setupSuccess)
    {
        // Retry download...
        PostMessage(Arguments, PSM_SETCURSELID, 0, IDD_DIALOG4);
    }

    return STATUS_SUCCESS;
}

BOOL PropSheetPage4_OnInitDialog(
    _In_ HWND hwndDlg,
    _In_ HWND hwndFocus,
    _Inout_ LPARAM lParam
    )
{
    InitializeFont(GetDlgItem(hwndDlg, IDC_MAINHEADER), -17, FW_SEMIBOLD);
    InitializeFont(GetDlgItem(hwndDlg, IDC_MAINHEADER1), -12, FW_SEMIBOLD);

    SetWindowSubclass(
        GetDlgItem(hwndDlg, IDC_PROGRESS1),
        SubclassWindowProc,
        IDC_PROGRESS1,
        0
        );

    // Enable the themed dialog background texture.
    EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

    return TRUE;
}

BOOL PropSheetPage4_OnNotify(
    _In_ HWND hwndDlg,
    _In_ INT idCtrl,
    _Inout_ LPNMHDR lpNmh
    )
{
    LPPSHNOTIFY pageNotify = (LPPSHNOTIFY)lpNmh;

    switch (pageNotify->hdr.code)
    {
    case PSN_SETACTIVE:
        {
            HWND hwPropSheet = pageNotify->hdr.hwndFrom;

            // Disable Next/Back buttons
            PropSheet_SetWizButtons(hwPropSheet, 0);

            _hwndProgress = hwndDlg;

            SetTimer(hwndDlg, 1, 100, NULL);

            PhCreateThread(0, DownloadThread, hwPropSheet);
        }
        break;
    case PSN_QUERYCANCEL:
        {
            //if (UpdateResetState == InstallStateResetting || UpdateResetState == InstallStateInstalling)

            //PropSheet_CancelToClose(GetParent(hwndDlg));
            //EnableMenuItem(GetSystemMenu(GetParent(hwndDlg), FALSE), SC_CLOSE, MF_GRAYED);
            //EnableMenuItem(GetSystemMenu(GetParent(hwndDlg), FALSE), SC_CLOSE, MF_ENABLED);

            //SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)TRUE);
            //return TRUE;
        }
        break;
    case PSN_KILLACTIVE:
        {
            KillTimer(hwndDlg, 1);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PropSheetPage4_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    )
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, PropSheetPage4_OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, PropSheetPage4_OnNotify);
    case WM_TIMER:
        {
            _SetProgressTime();
        }
        break;
    }

    return FALSE;
}