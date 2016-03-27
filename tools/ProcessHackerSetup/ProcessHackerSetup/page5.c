#include <setup.h>
#include <appsup.h>

static VOID LoadSetupImage(
    _In_ HWND hwndDlg
    )
{
    HBITMAP imageBitmap = LoadPngImageFromResources(MAKEINTRESOURCE(IDB_PNG1));

    // The image control uses a large square frame so that we can use the VS dialog designer more easily.
    // Remove the frame style and apply the bitmap style.
    PhSetWindowStyle(
        GetDlgItem(hwndDlg, IDC_PROJECT_ICON),
        SS_BITMAP | SS_BLACKFRAME,
        SS_BITMAP
        );

    SendMessage(
        GetDlgItem(hwndDlg, IDC_PROJECT_ICON),
        STM_SETIMAGE,
        IMAGE_BITMAP,
        (LPARAM)imageBitmap
        );

    DeleteObject(imageBitmap);
}

BOOL PropSheetPage5_OnInitDialog(
    _In_ HWND hwndDlg,
    _In_ HWND hwndFocus,
    _Inout_ LPARAM lParam
    )
{
    // Set the fonts.
    InitializeFont(GetDlgItem(hwndDlg, IDC_MAINHEADER), -17, FW_SEMIBOLD);

    LoadSetupImage(hwndDlg);

    // Enable the themed dialog background texture.
    EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

    return TRUE;
}

BOOL PropSheetPage5_OnNotify(
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
            Button_SetText(GetDlgItem(pageNotify->hdr.hwndFrom, IDCANCEL), L"Close");
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PropSheetPage5_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    )
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, PropSheetPage5_OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, PropSheetPage5_OnNotify);
    }

    return FALSE;
}