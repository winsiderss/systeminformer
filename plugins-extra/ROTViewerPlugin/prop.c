#include "main.h"

INT_PTR CALLBACK PropDialogProc(
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
            IMoniker* pmkObjectName = (IMoniker*)lParam;
            IBindCtx* ctx;
            IMalloc* iMalloc;

            ULARGE_INTEGER objSize;
            ULONG count = 0;
            ULONG index = 0;

            OLECHAR* name = NULL;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            CoGetMalloc(1, &iMalloc);

            if (SUCCEEDED(CreateBindCtx(0, &ctx)))
            {
                if (SUCCEEDED(IMoniker_GetDisplayName(pmkObjectName, ctx, NULL, &name)))
                {
                    //PhSetListViewSubItem(lvRot, itemIndex, 1, name);
                    SetDlgItemText(
                        hwndDlg, 
                        IDC_NAME, 
                        name
                        );

                    IMalloc_Free(iMalloc, name);
                }
            }
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
    }

    return FALSE;
}