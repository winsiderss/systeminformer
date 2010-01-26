#include <phgui.h>
#include <windowsx.h>

INT_PTR CALLBACK PhpFindObjectsDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HWND PhFindObjectsWindowHandle = NULL;
static PH_LAYOUT_MANAGER WindowLayoutManager;

VOID PhShowFindObjectsDialog()
{
    if (!PhFindObjectsWindowHandle)
    {
        PhFindObjectsWindowHandle = CreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_FINDOBJECTS),
            PhMainWndHandle,
            PhpFindObjectsDlgProc
            );
    }

    ShowWindow(PhFindObjectsWindowHandle, SW_SHOW);
}

static INT_PTR CALLBACK PhpFindObjectsDlgProc(      
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

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_FILTER),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK),
                NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_RESULTS),
                NULL, PH_ANCHOR_ALL);

            PhRegisterDialog(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            Edit_SetSel(GetDlgItem(hwndDlg, IDC_FILTER), 0, -1);
        }
        break;
    case WM_CLOSE:
        {
            ShowWindow(hwndDlg, SW_HIDE);
            SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, 0);
        }
        return TRUE;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
                {
                    PhShowInformation(hwndDlg, L"Find");
                }
                break;
            case IDCANCEL:
                {
                    SendMessage(hwndDlg, WM_CLOSE, 0, 0);
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&WindowLayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 320, 280);
        }
        break;
    }

    return FALSE;
}
