#include <phgui.h>

INT_PTR CALLBACK PhpServicePropertiesDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowServicePropertiesDialog(
    __in HWND ParentWindowHandle,
    __in PPH_SERVICE_ITEM Service
    )
{
    PhReferenceObject(Service);
    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SERVICE),
        ParentWindowHandle,
        PhpServicePropertiesDlgProc,
        (LPARAM)Service
        );
}

INT_PTR CALLBACK PhpServicePropertiesDlgProc(      
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
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}
