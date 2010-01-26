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
            PPH_LAYOUT_MANAGER layoutManager;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            SetDlgItemText(hwndDlg, IDC_TEXT, string);

            layoutManager = PhAllocate(sizeof(PH_LAYOUT_MANAGER));
            PhInitializeLayoutManager(layoutManager, hwndDlg);
            PhAddLayoutItem(layoutManager, GetDlgItem(hwndDlg, IDC_TEXT), NULL,
                PH_ANCHOR_ALL);
            PhAddLayoutItem(layoutManager, GetDlgItem(hwndDlg, IDOK), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SetProp(hwndDlg, L"LayoutManager", (HANDLE)layoutManager);
        }
        break;
    case WM_DESTROY:
        {
            PPH_LAYOUT_MANAGER layoutManager;

            layoutManager = (PPH_LAYOUT_MANAGER)GetProp(hwndDlg, L"LayoutManager");
            PhDeleteLayoutManager(layoutManager);
            PhFree(layoutManager);
            RemoveProp(hwndDlg, L"LayoutManager");
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
        break;
    case WM_SIZE:
        {
            PPH_LAYOUT_MANAGER layoutManager;

            layoutManager = (PPH_LAYOUT_MANAGER)GetProp(hwndDlg, L"LayoutManager");
            PhLayoutManagerLayout(layoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 180, 160);
        }
        break;
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
