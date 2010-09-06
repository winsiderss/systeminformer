#include <phdk.h>
#include "procdb.h"
#include "actions.h"
#include "config.h"
#include "resource.h"

INT_PTR CALLBACK ConfigDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PaShowConfigurationDialog(
    __in HWND ParentWindowHandle,
    __in PVOID DllBase
    )
{
    DialogBox(
        DllBase,
        MAKEINTRESOURCE(IDD_CONFIG),
        ParentWindowHandle,
        ConfigDlgProc
        );
}

INT_PTR CALLBACK ConfigDlgProc(
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
    case WM_DESTROY:
        {

        }
        break;
    }

    return FALSE;
}
