#include <setup.h>

PPH_STRING SetupInstallPath = NULL;

static VOID PvpInitializeDpi(
    VOID
    )
{
    HDC hdc;

    if (hdc = GetDC(NULL))
    {
        PhGlobalDpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
    }
}

INT CALLBACK MainPropSheet_Callback(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case PSCB_INITIALIZED:
        {

        }
        break;
    case PSCB_PRECREATE:
        {
            if (lParam)
            {
                ((LPDLGTEMPLATE)lParam)->style |= WS_MINIMIZEBOX;
            }
        }
        break;
    }

    return FALSE;
}

INT WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR lpCmdLine,
    _In_ INT nCmdShow
    )
{
    PROPSHEETPAGE propSheetPage = { sizeof(PROPSHEETPAGE) };
    PROPSHEETHEADER propSheetHeader = { sizeof(PROPSHEETHEADER) };
    HPROPSHEETPAGE pages[5];

    if (!NT_SUCCESS(PhInitializePhLibEx(0, 0, 0)))
        return 1;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    PhApplicationName = L"Process Hacker - Setup";
    PhGuiSupportInitialization();
    PvpInitializeDpi();

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_USECALLBACK |
        PSH_WIZARD_LITE;
    propSheetHeader.hInstance = PhLibImageBase;
    propSheetHeader.pszIcon = MAKEINTRESOURCE(IDI_ICON1);
    propSheetHeader.pfnCallback = MainPropSheet_Callback;
    propSheetHeader.phpage = pages;

    // page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USETITLE;
    propSheetPage.pszTitle = PhApplicationName;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG1);
    propSheetPage.pfnDlgProc = PropSheetPage1_WndProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USETITLE;
    propSheetPage.pszTitle = PhApplicationName;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG2);
    propSheetPage.pfnDlgProc = PropSheetPage2_WndProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG3);
    propSheetPage.pfnDlgProc = PropSheetPage3_WndProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USETITLE;
    propSheetPage.pszTitle = PhApplicationName;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG4);
    propSheetPage.pfnDlgProc = PropSheetPage4_WndProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USETITLE;
    propSheetPage.pszTitle = PhApplicationName;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG5);
    propSheetPage.pfnDlgProc = PropSheetPage5_WndProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    PhModalPropertySheet(&propSheetHeader);

    return EXIT_SUCCESS;
}