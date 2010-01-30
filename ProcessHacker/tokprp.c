#include <phgui.h>

typedef struct _TOKEN_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PVOID Context;
} TOKEN_PAGE_CONTEXT, *PTOKEN_PAGE_CONTEXT;

INT CALLBACK PhpTokenPropPageProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
    );

INT_PTR CALLBACK PhpTokenPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HPROPSHEETPAGE PhCreateTokenPage(
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PTOKEN_PAGE_CONTEXT tokenPageContext;

    if (!NT_SUCCESS(PhCreateAlloc(&tokenPageContext, sizeof(TOKEN_PAGE_CONTEXT))))
        return NULL;

    tokenPageContext->OpenObject = OpenObject;
    tokenPageContext->Context = Context;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USECALLBACK;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OBJTOKEN);
    propSheetPage.pfnDlgProc = PhpTokenPageProc;
    propSheetPage.lParam = (LPARAM)tokenPageContext;
    propSheetPage.pfnCallback = PhpTokenPropPageProc;

    propSheetPageHandle = CreatePropertySheetPage(&propSheetPage);
    // CreatePropertySheetPage would have sent PSPCB_ADDREF (below), 
    // which would have added a reference.
    PhDereferenceObject(tokenPageContext);

    return propSheetPageHandle;
}

INT CALLBACK PhpTokenPropPageProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
    )
{
    PTOKEN_PAGE_CONTEXT tokenPageContext;

    tokenPageContext = (PTOKEN_PAGE_CONTEXT)ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
    {
        PhReferenceObject(tokenPageContext);
    }
    else if (uMsg == PSPCB_RELEASE)
    {
        PhDereferenceObject(tokenPageContext);
    }

    return 1;
}

INT_PTR CALLBACK PhpTokenPageProc(
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
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PTOKEN_PAGE_CONTEXT tokenPageContext = (PTOKEN_PAGE_CONTEXT)propSheetPage->lParam;

            SetProp(hwndDlg, L"TokenPageContext", (HANDLE)tokenPageContext);
        }
        break;
    case WM_DESTROY:
        {
            PTOKEN_PAGE_CONTEXT tokenPageContext;

            tokenPageContext = (PTOKEN_PAGE_CONTEXT)GetProp(hwndDlg, L"TokenPageContext");

            RemoveProp(hwndDlg, L"TokenPageContext");
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_ADVANCED:
                {
                    PhShowInformation(hwndDlg, L"Advanced");
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
