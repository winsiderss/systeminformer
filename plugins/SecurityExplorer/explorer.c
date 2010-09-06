#include "explorer.h"

INT_PTR CALLBACK SxLsaDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HWND AccountsLv;
PPH_LIST AccountsList = NULL;
HWND PrivilegesLv;

PSID SelectedAccount;

VOID SxShowExplorer()
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[1];

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = PhMainWndHandle;
    propSheetHeader.pszCaption = L"Security";
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // LSA page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_LSA);
    propSheetPage.pfnDlgProc = SxLsaDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    PropertySheet(&propSheetHeader);
}

NTSTATUS SxpOpenLsaPolicy(
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt PVOID Context
    )
{
    return PhOpenLsaPolicy(Handle, DesiredAccess, NULL);
}

VOID SxpFreeAccounts()
{
    ULONG i;

    if (AccountsList)
    {
        for (i = 0; i < AccountsList->Count; i++)
            PhFree(AccountsList->Items[i]);

        PhClearList(AccountsList);
    }
}

VOID SxpRefreshAccounts()
{
    LSA_HANDLE policyHandle;
    LSA_ENUMERATION_HANDLE enumerationHandle = 0;
    PLSA_ENUMERATION_INFORMATION accounts;
    ULONG numberOfAccounts;
    ULONG i;

    if (AccountsList)
    {
        SxpFreeAccounts();
    }
    else
    {
        AccountsList = PhCreateList(40);
    }

    ListView_DeleteAllItems(AccountsLv);

    if (NT_SUCCESS(PhOpenLsaPolicy(&policyHandle, POLICY_VIEW_LOCAL_INFORMATION, NULL)))
    {
        while (NT_SUCCESS(LsaEnumerateAccounts(
            policyHandle,
            &enumerationHandle,
            &accounts,
            0x100,
            &numberOfAccounts
            )))
        {
            for (i = 0; i < numberOfAccounts; i++)
            {
                INT lvItemIndex;
                PSID sid;
                PPH_STRING name;
                PPH_STRING sidString;

                sid = PhAllocateCopy(accounts[i].Sid, RtlLengthSid(accounts[i].Sid));
                PhAddItemList(AccountsList, sid);

                name = PhGetSidFullName(accounts[i].Sid, TRUE, NULL);

                lvItemIndex = PhAddListViewItem(AccountsLv, MAXINT, PhGetStringOrDefault(name, L"(unknown)"), sid);

                if (name)
                    PhDereferenceObject(name);

                sidString = PhSidToStringSid(accounts[i].Sid);
                PhSetListViewSubItem(AccountsLv, lvItemIndex, 1, sidString->Buffer);
                PhDereferenceObject(sidString);
            }

            LsaFreeMemory(accounts);
        }

        LsaClose(policyHandle);
    }

    ExtendedListView_SortItems(AccountsLv);
}

VOID SxpRefreshPrivileges()
{
    LSA_HANDLE policyHandle;
    LSA_ENUMERATION_HANDLE enumerationHandle = 0;
    PPOLICY_PRIVILEGE_DEFINITION privileges;
    ULONG numberOfPrivileges;
    ULONG i;

    ListView_DeleteAllItems(PrivilegesLv);

    if (NT_SUCCESS(PhOpenLsaPolicy(&policyHandle, POLICY_VIEW_LOCAL_INFORMATION, NULL)))
    {
        while (NT_SUCCESS(LsaEnumeratePrivileges(
            policyHandle,
            &enumerationHandle,
            &privileges,
            0x100,
            &numberOfPrivileges
            )))
        {
            for (i = 0; i < numberOfPrivileges; i++)
            {
                INT lvItemIndex;
                PPH_STRING name;
                PPH_STRING displayName;

                name = PhCreateStringEx(privileges[i].Name.Buffer, privileges[i].Name.Length);
                lvItemIndex = PhAddListViewItem(PrivilegesLv, MAXINT, name->Buffer, NULL);

                if (PhLookupPrivilegeDisplayName(&name->sr, &displayName))
                {
                    PhSetListViewSubItem(PrivilegesLv, lvItemIndex, 1, displayName->Buffer);
                    PhDereferenceObject(displayName);
                }

                PhDereferenceObject(name);
            }

            LsaFreeMemory(privileges);
        }

        LsaClose(policyHandle);
    }

    ExtendedListView_SortItems(PrivilegesLv);
}

NTSTATUS SxpOpenSelectedLsaAccount(
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt PVOID Context
    )
{
    NTSTATUS status;
    LSA_HANDLE policyHandle;

    if (NT_SUCCESS(status = PhOpenLsaPolicy(&policyHandle, POLICY_LOOKUP_NAMES, NULL)))
    {
        status = LsaOpenAccount(policyHandle, SelectedAccount, DesiredAccess, Handle);
        LsaClose(policyHandle);
    }

    return status;
}

INT_PTR CALLBACK SxLsaDlgProc(
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
            AccountsLv = GetDlgItem(hwndDlg, IDC_ACCOUNTS);
            PrivilegesLv = GetDlgItem(hwndDlg, IDC_PRIVILEGES);

            PhSetListViewStyle(AccountsLv, FALSE, TRUE);
            PhSetControlTheme(AccountsLv, L"explorer");
            PhAddListViewColumn(AccountsLv, 0, 0, 0, LVCFMT_LEFT, 220, L"Name");
            PhAddListViewColumn(AccountsLv, 1, 1, 1, LVCFMT_LEFT, 300, L"SID");
            PhSetExtendedListView(AccountsLv);

            PhSetListViewStyle(PrivilegesLv, FALSE, TRUE);
            PhSetControlTheme(PrivilegesLv, L"explorer");
            PhAddListViewColumn(PrivilegesLv, 0, 0, 0, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(PrivilegesLv, 1, 1, 1, LVCFMT_LEFT, 360, L"Description");
            PhSetExtendedListView(PrivilegesLv);

            SxpRefreshAccounts();
            SxpRefreshPrivileges();
        }
        break;
    case WM_DESTROY:
        {
            SxpFreeAccounts();
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_EDITPOLICYSECURITY:
                {
                    PH_STD_OBJECT_SECURITY stdObjectSecurity;
                    PPH_ACCESS_ENTRY accessEntries;
                    ULONG numberOfAccessEntries;

                    stdObjectSecurity.OpenObject = SxpOpenLsaPolicy;
                    stdObjectSecurity.ObjectType = L"LsaPolicy";
                    stdObjectSecurity.Context = NULL;

                    if (PhGetAccessEntries(L"LsaPolicy", &accessEntries, &numberOfAccessEntries))
                    {
                        PhEditSecurity(
                            hwndDlg,
                            L"Local LSA Policy",
                            SxStdGetObjectSecurity,
                            SxStdSetObjectSecurity,
                            &stdObjectSecurity,
                            accessEntries,
                            numberOfAccessEntries
                            );
                        PhFree(accessEntries);
                    }
                }
                break;
            case IDC_ACCOUNT_DELETE:
                {
                    if (!SelectedAccount)
                        return FALSE;

                    if (PhShowConfirmMessage(
                        hwndDlg,
                        L"delete",
                        L"the selected account",
                        NULL,
                        TRUE
                        ))
                    {
                        NTSTATUS status;
                        LSA_HANDLE policyHandle;
                        LSA_HANDLE accountHandle;

                        if (NT_SUCCESS(status = PhOpenLsaPolicy(&policyHandle, POLICY_LOOKUP_NAMES, NULL)))
                        {
                            if (NT_SUCCESS(status = LsaOpenAccount(
                                policyHandle,
                                SelectedAccount,
                                ACCOUNT_VIEW | DELETE, // ACCOUNT_VIEW is needed as well for some reason
                                &accountHandle
                                )))
                            {
                                status = LsaDelete(accountHandle);
                                LsaClose(accountHandle);
                            }

                            LsaClose(policyHandle);
                        }

                        if (NT_SUCCESS(status))
                            SxpRefreshAccounts();
                        else
                            PhShowStatus(hwndDlg, L"Unable to delete the account", status, 0);
                    }
                }
                break;
            case IDC_ACCOUNT_SECURITY:
                {
                    PH_STD_OBJECT_SECURITY stdObjectSecurity;
                    PPH_ACCESS_ENTRY accessEntries;
                    ULONG numberOfAccessEntries;

                    if (!SelectedAccount)
                        return FALSE;

                    stdObjectSecurity.OpenObject = SxpOpenSelectedLsaAccount;
                    stdObjectSecurity.ObjectType = L"LsaAccount";
                    stdObjectSecurity.Context = NULL;

                    if (PhGetAccessEntries(L"LsaAccount", &accessEntries, &numberOfAccessEntries))
                    {
                        PPH_STRING name;

                        name = PhGetSidFullName(SelectedAccount, TRUE, NULL);

                        PhEditSecurity(
                            hwndDlg,
                            name->Buffer,
                            SxStdGetObjectSecurity,
                            SxStdSetObjectSecurity,
                            &stdObjectSecurity,
                            accessEntries,
                            numberOfAccessEntries
                            );
                        PhFree(accessEntries);

                        PhDereferenceObject(name);
                    }
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == AccountsLv)
                    {
                        if (ListView_GetSelectedCount(AccountsLv) == 1)
                        {
                            SelectedAccount = PhGetSelectedListViewItemParam(AccountsLv);
                        }
                        else
                        {
                            SelectedAccount = NULL;
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
