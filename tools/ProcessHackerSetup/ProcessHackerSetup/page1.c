#include <setup.h>
#include <appsup.h>

VOID LoadSetupIcons(
    _In_ HWND hwndDlg
    )
{
    HBITMAP smallIconHandle = (HBITMAP)LoadImage(
        PhLibImageBase,
        MAKEINTRESOURCE(IDI_ICON1),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
        );
    HBITMAP largeIconHandle = (HBITMAP)LoadImage(
        PhLibImageBase,
        MAKEINTRESOURCE(IDI_ICON1),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        LR_DEFAULTCOLOR
        );
   
    SendMessage(GetParent(hwndDlg), WM_SETICON, ICON_SMALL, (LPARAM)smallIconHandle);
    SendMessage(GetParent(hwndDlg), WM_SETICON, ICON_BIG, (LPARAM)largeIconHandle);

    DeleteObject(largeIconHandle);
    DeleteObject(smallIconHandle);
}

VOID LoadSetupImage(
    _In_ HWND hwndDlg
    )
{
    HBITMAP imageBitmap = LoadPngImageFromResources(MAKEINTRESOURCE(IDB_PNG1));

    // The image control uses a large square frame so that we can use the VS designer easily.
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

BOOL PropSheetPage1_OnInitDialog(
    _In_ HWND hwndDlg,
    _In_ HWND hwndFocus,
    _Inout_ LPARAM lParam
    )
{
    LoadSetupIcons(hwndDlg);
    LoadSetupImage(hwndDlg);

    // Center the dialog window on the desktop
    PhCenterWindow(GetParent(hwndDlg), NULL);
    SetForegroundWindow(GetParent(hwndDlg));

    // Set the fonts
    InitializeFont(GetDlgItem(hwndDlg, IDC_MAINHEADER), 24, FW_SEMIBOLD);
    InitializeFont(GetDlgItem(hwndDlg, IDC_SUBHEADER), 0, FW_NORMAL);

    // Enable the themed dialog background texture.
    EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

    return TRUE;
}

BOOL PropSheetPage1_OnNotify(
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

            // Disable the back button on Welcome page.
            //PropSheet_SetWizButtons(hwPropSheet, PSWIZB_NEXT);

            // Hide the back button on the Welcome page.
            ShowWindow(GetDlgItem(hwPropSheet, IDC_PROPSHEET_BACK), SW_HIDE);
        }
        break;
    case PSN_KILLACTIVE:
        {
            HWND hwPropSheet = pageNotify->hdr.hwndFrom;

            // Enable the back button for other pages.
            //PropSheet_SetWizButtons(hwPropSheet, PSWIZB_NEXT | PSWIZB_BACK);
            
            // Show the back button for other pages.
            ShowWindow(GetDlgItem(hwPropSheet, IDC_PROPSHEET_BACK), SW_SHOW);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PropSheetPage1_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    )
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, PropSheetPage1_OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, PropSheetPage1_OnNotify);
    }

    return FALSE;
}