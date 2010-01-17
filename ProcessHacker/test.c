#include <phgui.h>

PH_LAYOUT_MANAGER TestLayoutManager;

INT_PTR CALLBACK PhpTestResizingDlgProc(      
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
            PPH_LAYOUT_ITEM windowItem;

            PhInitializeLayoutManager(&TestLayoutManager);
            windowItem = PhAddLayoutItem(&TestLayoutManager, hwndDlg, NULL, 0); 
            PhAddLayoutItem(&TestLayoutManager, GetDlgItem(hwndDlg, IDABORT),
                windowItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP);
            PhAddLayoutItem(&TestLayoutManager, GetDlgItem(hwndDlg, IDHELP),
                windowItem, PH_ANCHOR_ALL);
            PhAddLayoutItem(&TestLayoutManager, GetDlgItem(hwndDlg, IDOK),
                windowItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&TestLayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDABORT:
                PhShowMessage(hwndDlg, MB_ICONINFORMATION, L"You clicked the button.");
                break;
            case IDOK:
                EndDialog(hwndDlg, 0);
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&TestLayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhRectMinimumSize((PRECT)lParam, 400, 250);
        }
        break;
    }

    return FALSE;
}

VOID TestResizing()
{
    DialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_TEST),
        NULL,
        PhpTestResizingDlgProc
        );
}
