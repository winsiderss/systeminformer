#include <phgui.h>

typedef struct _PROCESS_HEAPS_CONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
} PROCESS_HEAPS_CONTEXT, *PPROCESS_HEAPS_CONTEXT;

INT_PTR CALLBACK PhpProcessHeapsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowProcessHeapsDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PROCESS_HEAPS_CONTEXT context;

    context.ProcessItem = ProcessItem;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_HEAPS),
        ParentWindowHandle,
        PhpProcessHeapsDlgProc,
        (LPARAM)&context
        );
}

INT_PTR CALLBACK PhpProcessHeapsDlgProc(
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
            SetProp(hwndDlg, L"Context", (HANDLE)lParam);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"Context");
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
            }
        }
        break;
    }

    return FALSE;
}
