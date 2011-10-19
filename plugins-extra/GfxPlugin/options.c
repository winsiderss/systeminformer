#include "gfxinfo.h"

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID EtShowOptionsDialog(
    __in HWND ParentWindowHandle
    )
{
    DialogBox(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        ParentWindowHandle,
        OptionsDlgProc
        );
}

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {

        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                }
                break;
            case IDOK:
                {

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}