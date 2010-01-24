#include <phgui.h>

static INT_PTR CALLBACK PhpInformationDlgProc(      
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
            PWSTR string = (PWSTR)lParam;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            SetDlgItemText(hwndDlg, IDC_TEXT, string);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
    }

    return FALSE;
}

VOID PhShowInformationDialog(
    __in HWND ParentWindowHandle,
    __in PWSTR String
    )
{
    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_INFORMATION),
        ParentWindowHandle,
        PhpInformationDlgProc,
        (LPARAM)String
        );
}
