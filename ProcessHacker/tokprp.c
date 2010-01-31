#include <phgui.h>

typedef struct _TOKEN_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PVOID Context;
    DLGPROC HookProc;
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
    __in PVOID Context,
    __in_opt DLGPROC HookProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PTOKEN_PAGE_CONTEXT tokenPageContext;

    if (!NT_SUCCESS(PhCreateAlloc(&tokenPageContext, sizeof(TOKEN_PAGE_CONTEXT))))
        return NULL;

    tokenPageContext->OpenObject = OpenObject;
    tokenPageContext->Context = Context;
    tokenPageContext->HookProc = HookProc;

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
    PTOKEN_PAGE_CONTEXT tokenPageContext;

    if (uMsg != WM_INITDIALOG)
    {
        tokenPageContext = (PTOKEN_PAGE_CONTEXT)GetProp(hwndDlg, L"TokenPageContext");
    }
    else
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;

        tokenPageContext = (PTOKEN_PAGE_CONTEXT)propSheetPage->lParam;
        SetProp(hwndDlg, L"TokenPageContext", (HANDLE)tokenPageContext);
    }

    if (!tokenPageContext)
        return FALSE;

    if (tokenPageContext->HookProc)
    {
        if (tokenPageContext->HookProc(hwndDlg, uMsg, wParam, lParam))
            return TRUE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND groupsLv;
            HWND privilegesLv;
            HANDLE tokenHandle;

            groupsLv = GetDlgItem(hwndDlg, IDC_GROUPS);
            privilegesLv = GetDlgItem(hwndDlg, IDC_PRIVILEGES);
            PhSetListViewStyle(groupsLv, FALSE, TRUE);
            PhSetListViewStyle(privilegesLv, FALSE, TRUE);
            PhSetControlTheme(groupsLv, L"explorer");
            PhSetControlTheme(privilegesLv, L"explorer");

            PhAddListViewColumn(groupsLv, 0, 0, 0, LVCFMT_LEFT, 160, L"Name");
            PhAddListViewColumn(groupsLv, 1, 1, 1, LVCFMT_LEFT, 200, L"Flags");

            PhAddListViewColumn(privilegesLv, 0, 0, 0, LVCFMT_LEFT, 100, L"Name");
            PhAddListViewColumn(privilegesLv, 1, 1, 1, LVCFMT_LEFT, 100, L"Status");
            PhAddListViewColumn(privilegesLv, 2, 2, 2, LVCFMT_LEFT, 170, L"Description");

            PhSetExtendedListView(groupsLv);
            PhSetExtendedListView(privilegesLv);

            SetDlgItemText(hwndDlg, IDC_USER, L"Unknown");
            SetDlgItemText(hwndDlg, IDC_USERSID, L"Unknown");

            if (NT_SUCCESS(tokenPageContext->OpenObject(
                &tokenHandle,
                TOKEN_QUERY,
                tokenPageContext->Context
                )))
            {
                PTOKEN_USER tokenUser;
                PPH_STRING fullUserName;
                PPH_STRING stringUserSid;
                ULONG sessionId;
                BOOLEAN isElevated;
                BOOLEAN isVirtualizationAllowed;
                BOOLEAN isVirtualizationEnabled;

                if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
                {
                    if (fullUserName = PhGetSidFullName(tokenUser->User.Sid))
                    {
                        SetDlgItemText(hwndDlg, IDC_USER, fullUserName->Buffer);
                        PhDereferenceObject(fullUserName);
                    }

                    if (stringUserSid = PhConvertSidToStringSid(tokenUser->User.Sid))
                    {
                        SetDlgItemText(hwndDlg, IDC_USERSID, stringUserSid->Buffer);
                        PhDereferenceObject(stringUserSid);
                    }

                    PhFree(tokenUser);
                }

                if (NT_SUCCESS(PhGetTokenSessionId(tokenHandle, &sessionId)))
                    SetDlgItemInt(hwndDlg, IDC_SESSIONID, sessionId, FALSE);

                if (WINDOWS_HAS_UAC)
                {
                    if (NT_SUCCESS(PhGetTokenIsElevated(tokenHandle, &isElevated)))
                        SetDlgItemText(hwndDlg, IDC_ELEVATED, isElevated ? L"Yes" : L"No");

                    if (NT_SUCCESS(PhGetTokenIsVirtualizationAllowed(tokenHandle, &isVirtualizationAllowed)))
                    {
                        if (isVirtualizationAllowed)
                        {
                            if (NT_SUCCESS(PhGetTokenIsVirtualizationEnabled(tokenHandle, &isVirtualizationEnabled)))
                            {
                                SetDlgItemText(
                                    hwndDlg,
                                    IDC_VIRTUALIZED,
                                    isVirtualizationEnabled ? L"Yes" : L"No"
                                    );
                            }
                        }
                        else
                        {
                            SetDlgItemText(hwndDlg, IDC_VIRTUALIZED, L"Not Allowed");
                        }
                    }
                }
                else
                {
                    SetDlgItemText(hwndDlg, IDC_ELEVATED, L"N/A");
                    SetDlgItemText(hwndDlg, IDC_VIRTUALIZED, L"N/A");
                }

                CloseHandle(tokenHandle);
            }
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"TokenPageContext");
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_ADVANCED:
                {
                    // TODO
                    PhShowInformation(hwndDlg, L"Advanced");
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
