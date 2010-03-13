/*
 * Process Hacker - 
 *   token properties
 * 
 * Copyright (C) 2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phapp.h>

typedef struct _TOKEN_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PVOID Context;
    DLGPROC HookProc;

    HWND GroupsListViewHandle;
    HWND PrivilegesListViewHandle;

    PTOKEN_GROUPS Groups;
    PTOKEN_PRIVILEGES Privileges;
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

VOID PhpShowTokenAdvancedProperties(
    __in HWND ParentWindowHandle,
    __in PTOKEN_PAGE_CONTEXT Context
    );

INT_PTR CALLBACK PhpTokenGeneralPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpTokenAdvancedPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowTokenProperties(
    __in HWND ParentWindowHandle,
    __in PPH_OPEN_OBJECT OpenObject,
    __in PVOID Context,
    __in_opt PWSTR Title
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    HPROPSHEETPAGE pages[1];

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = Title ? Title : L"Token";
    propSheetHeader.nPages = 1;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    pages[0] = PhCreateTokenPage(OpenObject, Context, NULL);

    PropertySheet(&propSheetHeader);
}

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

    memset(tokenPageContext, 0, sizeof(TOKEN_PAGE_CONTEXT));
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

PPH_STRING PhGetGroupAttributesString(
    __in ULONG Attributes
    )
{
    PWSTR baseString;
    PPH_STRING string;

    if (Attributes & SE_GROUP_INTEGRITY)
    {
        if (Attributes & SE_GROUP_INTEGRITY_ENABLED)
            return PhCreateString(L"Integrity");
        else
            return PhCreateString(L"Integrity (Disabled)");
    }

    if (Attributes & SE_GROUP_LOGON_ID)
        baseString = L"Logon ID";
    else if (Attributes & SE_GROUP_MANDATORY)
        baseString = L"Mandatory";
    else if (Attributes & SE_GROUP_OWNER)
        baseString = L"Owner";
    else if (Attributes & SE_GROUP_RESOURCE)
        baseString = L"Resource";
    else if (Attributes & SE_GROUP_USE_FOR_DENY_ONLY)
        baseString = L"Use for Deny Only";
    else
        baseString = L"Unknown";

    if (Attributes & SE_GROUP_ENABLED_BY_DEFAULT)
        string = PhConcatStrings2(baseString, L" (Default Enabled)");
    else if (Attributes & SE_GROUP_ENABLED)
        string = PhCreateString(baseString);
    else
        string = PhConcatStrings2(baseString, L" (Disabled)");

    return string;
}

COLORREF PhGetGroupAttributesColor(
    __in ULONG Attributes
    )
{
    if (Attributes & SE_GROUP_INTEGRITY)
    {
        if (Attributes & SE_GROUP_INTEGRITY_ENABLED)
            return RGB(0xe0, 0xf0, 0xe0);
        else
            return PhSysWindowColor;
    }

    if (Attributes & SE_GROUP_ENABLED_BY_DEFAULT)
        return RGB(0xe0, 0xf0, 0xe0);
    else if (Attributes & SE_GROUP_ENABLED)
        return PhSysWindowColor;
    else
        return RGB(0xf0, 0xe0, 0xe0);
}

static COLORREF NTAPI PhpTokenGroupColorFunction(
    __in INT Index,
    __in PVOID Param,
    __in PVOID Context
    )
{
    PSID_AND_ATTRIBUTES sidAndAttributes = Param;

    return PhGetGroupAttributesColor(sidAndAttributes->Attributes);
}

PWSTR PhGetPrivilegeAttributesString(
    __in ULONG Attributes
    )
{
    if (Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
        return L"Default Enabled";
    else if (Attributes & SE_PRIVILEGE_ENABLED)
        return L"Enabled";
    else
        return L"Disabled";
}

COLORREF PhGetPrivilegeAttributesColor(
    __in ULONG Attributes
    )
{
    if (Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
        return RGB(0xc0, 0xf0, 0xc0);
    else if (Attributes & SE_PRIVILEGE_ENABLED)
        return RGB(0xe0, 0xf0, 0xe0);
    else
        return RGB(0xf0, 0xe0, 0xe0);
}

static COLORREF NTAPI PhpTokenPrivilegeColorFunction(
    __in INT Index,
    __in PVOID Param,
    __in PVOID Context
    )
{
    PLUID_AND_ATTRIBUTES luidAndAttributes = Param;

    return PhGetPrivilegeAttributesColor(luidAndAttributes->Attributes);
}

PWSTR PhGetElevationTypeString(
    __in TOKEN_ELEVATION_TYPE ElevationType
    )
{
    switch (ElevationType)
    {
    case TokenElevationTypeFull:
        return L"Yes";
    case TokenElevationTypeLimited:
        return L"No";
    default:
        return L"N/A";
    }
}

FORCEINLINE PTOKEN_PAGE_CONTEXT PhpTokenPageHeader(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    return (PTOKEN_PAGE_CONTEXT)PhpGenericPropertyPageHeader(
        hwndDlg, uMsg, wParam, lParam, L"TokenPageContext");
}

INT_PTR CALLBACK PhpTokenPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PTOKEN_PAGE_CONTEXT tokenPageContext;

    tokenPageContext = PhpTokenPageHeader(hwndDlg, uMsg, wParam, lParam);

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

            tokenPageContext->GroupsListViewHandle = groupsLv = GetDlgItem(hwndDlg, IDC_GROUPS);
            tokenPageContext->PrivilegesListViewHandle = privilegesLv = GetDlgItem(hwndDlg, IDC_PRIVILEGES);
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
            ExtendedListView_SetItemColorFunction(groupsLv, PhpTokenGroupColorFunction);

            PhSetExtendedListView(privilegesLv);
            ExtendedListView_SetItemColorFunction(privilegesLv, PhpTokenPrivilegeColorFunction);

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
                TOKEN_ELEVATION_TYPE elevationType;
                BOOLEAN isVirtualizationAllowed;
                BOOLEAN isVirtualizationEnabled;
                ULONG i;

                if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
                {
                    if (fullUserName = PhGetSidFullName(tokenUser->User.Sid, TRUE, NULL))
                    {
                        SetDlgItemText(hwndDlg, IDC_USER, fullUserName->Buffer);
                        PhDereferenceObject(fullUserName);
                    }

                    if (stringUserSid = PhSidToStringSid(tokenUser->User.Sid))
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
                    if (NT_SUCCESS(PhGetTokenElevationType(tokenHandle, &elevationType)))
                        SetDlgItemText(hwndDlg, IDC_ELEVATED, PhGetElevationTypeString(elevationType));

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

                // Groups
                if (NT_SUCCESS(PhGetTokenGroups(tokenHandle, &tokenPageContext->Groups)))
                {
                    for (i = 0; i < tokenPageContext->Groups->GroupCount; i++)
                    {
                        INT lvItemIndex;
                        PPH_STRING fullName;
                        PPH_STRING attributesString;

                        if (!(fullName = PhGetSidFullName(tokenPageContext->Groups->Groups[i].Sid, TRUE, NULL)))
                            fullName = PhSidToStringSid(tokenPageContext->Groups->Groups[i].Sid);

                        if (fullName)
                        {
                            lvItemIndex = PhAddListViewItem(groupsLv, MAXINT, fullName->Buffer,
                                &tokenPageContext->Groups->Groups[i]);
                            attributesString = PhGetGroupAttributesString(
                                tokenPageContext->Groups->Groups[i].Attributes);
                            PhSetListViewSubItem(groupsLv, lvItemIndex, 1, attributesString->Buffer);

                            PhDereferenceObject(attributesString);
                            PhDereferenceObject(fullName);
                        }
                    }

                    ExtendedListView_SortItems(groupsLv);
                }

                // Privileges
                if (NT_SUCCESS(PhGetTokenPrivileges(tokenHandle, &tokenPageContext->Privileges)))
                {
                    for (i = 0; i < tokenPageContext->Privileges->PrivilegeCount; i++)
                    {
                        INT lvItemIndex;
                        PPH_STRING privilegeName;
                        PPH_STRING privilegeDisplayName;

                        if (PhLookupPrivilegeName(
                            &tokenPageContext->Privileges->Privileges[i].Luid,
                            &privilegeName
                            ))
                        {
                            privilegeDisplayName = NULL;
                            PhLookupPrivilegeDisplayName(privilegeName->Buffer, &privilegeDisplayName);

                            // Name
                            lvItemIndex = PhAddListViewItem(privilegesLv, MAXINT, privilegeName->Buffer,
                                &tokenPageContext->Privileges->Privileges[i]);
                            // Status
                            PhSetListViewSubItem(privilegesLv, lvItemIndex, 1,
                                PhGetPrivilegeAttributesString(
                                tokenPageContext->Privileges->Privileges[i].Attributes));
                            // Description
                            PhSetListViewSubItem(privilegesLv, lvItemIndex, 2,
                                PhGetString(privilegeDisplayName));

                            if (privilegeDisplayName) PhDereferenceObject(privilegeDisplayName);
                            PhDereferenceObject(privilegeName);
                        }
                    }

                    ExtendedListView_SortItems(privilegesLv);
                }

                NtClose(tokenHandle);
            }
        }
        break;
    case WM_DESTROY:
        {
            if (tokenPageContext->Groups) PhFree(tokenPageContext->Groups);
            if (tokenPageContext->Privileges) PhFree(tokenPageContext->Privileges);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case ID_PRIVILEGE_ENABLE:
            case ID_PRIVILEGE_DISABLE:
            case ID_PRIVILEGE_REMOVE:
                {
                    NTSTATUS status;
                    PLUID_AND_ATTRIBUTES *privileges;
                    ULONG numberOfPrivileges;
                    HANDLE tokenHandle;
                    ULONG i;

                    if (LOWORD(wParam) == ID_PRIVILEGE_REMOVE)
                    {
                        if (!PhShowConfirmMessage(
                            hwndDlg,
                            L"remove",
                            L"the selected privilege(s)",
                            L"Removing privileges may reduce the functionality of the process, "
                            L"and is permanent for the lifetime of the process.",
                            FALSE
                            ))
                            break;
                    }

                    PhGetSelectedListViewItemParams(
                        tokenPageContext->PrivilegesListViewHandle,
                        &privileges,
                        &numberOfPrivileges
                        );

                    status = tokenPageContext->OpenObject(
                        &tokenHandle,
                        TOKEN_ADJUST_PRIVILEGES,
                        tokenPageContext->Context
                        );

                    if (NT_SUCCESS(status))
                    {
                        ExtendedListView_SetRedraw(tokenPageContext->PrivilegesListViewHandle, FALSE);

                        for (i = 0; i < numberOfPrivileges; i++)
                        {
                            PPH_STRING privilegeName = NULL;
                            ULONG newAttributes;

                            PhLookupPrivilegeName(&privileges[i]->Luid, &privilegeName);
                            PHA_DEREFERENCE(privilegeName);

                            switch (LOWORD(wParam))
                            {
                            case ID_PRIVILEGE_ENABLE:
                                newAttributes = SE_PRIVILEGE_ENABLED;
                                break;
                            case ID_PRIVILEGE_DISABLE:
                                newAttributes = 0;
                                break;
                            case ID_PRIVILEGE_REMOVE:
                                newAttributes = SE_PRIVILEGE_REMOVED;
                                break;
                            }

                            // Privileges which are enabled by default cannot be 
                            // modified except to remove them.

                            if (
                                privileges[i]->Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT &&
                                LOWORD(wParam) != ID_PRIVILEGE_REMOVE
                                )
                            {
                                if (LOWORD(wParam) == ID_PRIVILEGE_DISABLE)
                                {
                                    if (!PhShowContinueStatus(
                                        hwndDlg,
                                        PhaConcatStrings2(L"Unable to disable ", privilegeName->Buffer)->Buffer,
                                        STATUS_UNSUCCESSFUL,
                                        0
                                        ))
                                        break;
                                }

                                continue;
                            }

                            if (PhSetTokenPrivilege(
                                tokenHandle,
                                NULL,
                                &privileges[i]->Luid,
                                newAttributes
                                ))
                            {
                                INT lvItemIndex = PhFindListViewItemByParam(
                                    tokenPageContext->PrivilegesListViewHandle,
                                    -1,
                                    privileges[i]
                                    );

                                if (LOWORD(wParam) != ID_PRIVILEGE_REMOVE)
                                {
                                    // Refresh the status text (and background 
                                    // color).
                                    privileges[i]->Attributes = newAttributes;
                                    PhSetListViewSubItem(
                                        tokenPageContext->PrivilegesListViewHandle,
                                        lvItemIndex,
                                        1,
                                        PhGetPrivilegeAttributesString(newAttributes)
                                        );
                                }
                                else
                                {
                                    ListView_DeleteItem(
                                        tokenPageContext->PrivilegesListViewHandle,
                                        lvItemIndex
                                        );
                                }
                            }
                            else
                            {
                                PWSTR action = L"set";

                                switch (LOWORD(wParam))
                                {
                                case ID_PRIVILEGE_ENABLE:
                                    action = L"enable";
                                    break;
                                case ID_PRIVILEGE_DISABLE:
                                    action = L"disable";
                                    break;
                                case ID_PRIVILEGE_REMOVE:
                                    action = L"remove";
                                    break;
                                }

                                if (!PhShowContinueStatus(
                                    hwndDlg,
                                    PhaFormatString(L"Unable to %s %s", action, privilegeName->Buffer)->Buffer,
                                    0,
                                    GetLastError()
                                    ))
                                    break;
                            }
                        }

                        ExtendedListView_SetRedraw(tokenPageContext->PrivilegesListViewHandle, TRUE);

                        NtClose(tokenHandle);
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to open the token", status, 0);
                    }

                    PhFree(privileges);

                    ExtendedListView_SortItems(tokenPageContext->PrivilegesListViewHandle);
                }
                break;
            case IDC_ADVANCED:
                {
                    PhpShowTokenAdvancedProperties(hwndDlg, tokenPageContext);
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
            case NM_RCLICK:
                {
                    if (header->hwndFrom == tokenPageContext->PrivilegesListViewHandle)
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;

                        if (ListView_GetSelectedCount(tokenPageContext->PrivilegesListViewHandle) != 0)
                        {
                            HMENU menu;
                            HMENU subMenu;

                            menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_PRIVILEGE));
                            subMenu = GetSubMenu(menu, 0);

                            PhShowContextMenu(
                                hwndDlg,
                                tokenPageContext->PrivilegesListViewHandle,
                                subMenu,
                                itemActivate->ptAction
                                );
                            DestroyMenu(menu);
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    REFLECT_MESSAGE_DLG(hwndDlg, tokenPageContext->GroupsListViewHandle, uMsg, wParam, lParam);
    REFLECT_MESSAGE_DLG(hwndDlg, tokenPageContext->PrivilegesListViewHandle, uMsg, wParam, lParam);

    return FALSE;
}

VOID PhpShowTokenAdvancedProperties(
    __in HWND ParentWindowHandle,
    __in PTOKEN_PAGE_CONTEXT Context
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    HPROPSHEETPAGE pages[3];
    PROPSHEETPAGE generalPage;
    PROPSHEETPAGE advancedPage;
    PH_STD_OBJECT_SECURITY stdObjectSecurity;
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = L"Token";
    propSheetHeader.nPages = 3;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // General

    memset(&generalPage, 0, sizeof(PROPSHEETPAGE));
    generalPage.dwSize = sizeof(PROPSHEETPAGE);
    generalPage.pszTemplate = MAKEINTRESOURCE(IDD_TOKGENERAL);
    generalPage.pfnDlgProc = PhpTokenGeneralPageProc;
    generalPage.lParam = (LPARAM)Context;
    pages[0] = CreatePropertySheetPage(&generalPage);

    // Advanced

    memset(&advancedPage, 0, sizeof(PROPSHEETPAGE));
    advancedPage.dwSize = sizeof(PROPSHEETPAGE);
    advancedPage.pszTemplate = MAKEINTRESOURCE(IDD_TOKADVANCED);
    advancedPage.pfnDlgProc = PhpTokenAdvancedPageProc;
    advancedPage.lParam = (LPARAM)Context;
    pages[1] = CreatePropertySheetPage(&advancedPage);

    // Security

    stdObjectSecurity.OpenObject = Context->OpenObject;
    stdObjectSecurity.ObjectType = L"Token";
    stdObjectSecurity.Context = Context->Context;

    if (PhGetAccessEntries(L"Token", &accessEntries, &numberOfAccessEntries))
    {
        pages[2] = PhCreateSecurityPage(
            L"Token",
            PhStdGetObjectSecurity,
            PhStdSetObjectSecurity,
            &stdObjectSecurity,
            accessEntries,
            numberOfAccessEntries
            );
        PhFree(accessEntries);
    }

    PropertySheet(&propSheetHeader);
}

static NTSTATUS PhpOpenLinkedToken(
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in PVOID Context
    )
{
    return PhGetTokenLinkedToken((HANDLE)Context, Handle);
}

INT_PTR CALLBACK PhpTokenGeneralPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PTOKEN_PAGE_CONTEXT tokenPageContext;

    tokenPageContext = PhpTokenPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!tokenPageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE tokenHandle;
            PPH_STRING tokenUserName = NULL;
            PPH_STRING tokenUserSid = NULL;
            PPH_STRING tokenOwnerName = NULL;
            PPH_STRING tokenPrimaryGroupName = NULL;
            ULONG tokenSessionId = -1;
            PWSTR tokenElevated = L"N/A";
            BOOLEAN hasLinkedToken = FALSE;
            PWSTR tokenVirtualization = L"N/A";
            WCHAR tokenSourceName[TOKEN_SOURCE_LENGTH + 1] = L"Unknown";
            WCHAR tokenSourceLuid[PH_PTR_STR_LEN_1] = L"Unknown";

            if (NT_SUCCESS(tokenPageContext->OpenObject(
                &tokenHandle,
                TOKEN_QUERY,
                tokenPageContext->Context
                )))
            {
                PTOKEN_USER tokenUser;
                PTOKEN_OWNER tokenOwner;
                PTOKEN_PRIMARY_GROUP tokenPrimaryGroup;
                TOKEN_ELEVATION_TYPE elevationType;
                BOOLEAN isVirtualizationAllowed;
                BOOLEAN isVirtualizationEnabled;

                if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
                {
                    tokenUserName = PHA_DEREFERENCE(PhGetSidFullName(tokenUser->User.Sid, TRUE, NULL));
                    tokenUserSid = PHA_DEREFERENCE(PhSidToStringSid(tokenUser->User.Sid));

                    PhFree(tokenUser);
                }

                if (NT_SUCCESS(PhGetTokenOwner(tokenHandle, &tokenOwner)))
                {
                    tokenOwnerName = PHA_DEREFERENCE(PhGetSidFullName(tokenOwner->Owner, TRUE, NULL));
                    PhFree(tokenOwner);
                }

                if (NT_SUCCESS(PhGetTokenPrimaryGroup(tokenHandle, &tokenPrimaryGroup)))
                {
                    tokenPrimaryGroupName = PHA_DEREFERENCE(PhGetSidFullName(
                        tokenPrimaryGroup->PrimaryGroup, TRUE, NULL));
                    PhFree(tokenPrimaryGroup);
                }

                PhGetTokenSessionId(tokenHandle, &tokenSessionId);

                if (WINDOWS_HAS_UAC)
                {
                    if (NT_SUCCESS(PhGetTokenElevationType(tokenHandle, &elevationType)))
                    {
                        tokenElevated = PhGetElevationTypeString(elevationType);
                        hasLinkedToken = elevationType != TokenElevationTypeDefault;
                    }

                    if (NT_SUCCESS(PhGetTokenIsVirtualizationAllowed(tokenHandle, &isVirtualizationAllowed)))
                    {
                        if (isVirtualizationAllowed)
                        {
                            if (NT_SUCCESS(PhGetTokenIsVirtualizationEnabled(tokenHandle, &isVirtualizationEnabled)))
                            {
                                tokenVirtualization = isVirtualizationEnabled ? L"Yes" : L"No";
                            }
                        }
                        else
                        {
                            tokenVirtualization = L"Not Allowed";
                        }
                    }
                }

                NtClose(tokenHandle);
            }

            if (NT_SUCCESS(tokenPageContext->OpenObject(
                &tokenHandle,
                TOKEN_QUERY_SOURCE,
                tokenPageContext->Context
                )))
            {
                TOKEN_SOURCE tokenSource;

                if (NT_SUCCESS(PhGetTokenSource(tokenHandle, &tokenSource)))
                {
                    PhCopyUnicodeStringZFromAnsi(
                        tokenSource.SourceName,
                        TOKEN_SOURCE_LENGTH,
                        tokenSourceName,
                        sizeof(tokenSourceName) / 2,
                        NULL
                        );

                    PhPrintPointer(tokenSourceLuid, (PVOID)tokenSource.SourceIdentifier.LowPart);
                }

                NtClose(tokenHandle);
            }

            SetDlgItemText(hwndDlg, IDC_USER, PhGetStringOrDefault(tokenUserName, L"Unknown"));
            SetDlgItemText(hwndDlg, IDC_USERSID, PhGetStringOrDefault(tokenUserSid, L"Unknown"));
            SetDlgItemText(hwndDlg, IDC_OWNER, PhGetStringOrDefault(tokenOwnerName, L"Unknown"));
            SetDlgItemText(hwndDlg, IDC_PRIMARYGROUP, PhGetStringOrDefault(tokenPrimaryGroupName, L"Unknown"));

            if (tokenSessionId != -1)
                SetDlgItemInt(hwndDlg, IDC_SESSIONID, tokenSessionId, FALSE);
            else
                SetDlgItemText(hwndDlg, IDC_SESSIONID, L"Unknown");

            SetDlgItemText(hwndDlg, IDC_ELEVATED, tokenElevated);
            SetDlgItemText(hwndDlg, IDC_VIRTUALIZATION, tokenVirtualization);
            SetDlgItemText(hwndDlg, IDC_SOURCENAME, tokenSourceName);
            SetDlgItemText(hwndDlg, IDC_SOURCELUID, tokenSourceLuid);

            if (!hasLinkedToken)
                ShowWindow(GetDlgItem(hwndDlg, IDC_LINKEDTOKEN), SW_HIDE);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_LINKEDTOKEN:
                {
                    NTSTATUS status;
                    HANDLE tokenHandle;

                    if (NT_SUCCESS(status = tokenPageContext->OpenObject(
                        &tokenHandle,
                        TOKEN_QUERY,
                        tokenPageContext->Context
                        )))
                    {
                        PhShowTokenProperties(hwndDlg, PhpOpenLinkedToken, (PVOID)tokenHandle, L"Linked Token");
                        NtClose(tokenHandle);
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to open the token", status, 0);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpTokenAdvancedPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PTOKEN_PAGE_CONTEXT tokenPageContext;

    tokenPageContext = PhpTokenPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!tokenPageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE tokenHandle;
            PWSTR tokenType = L"Unknown";
            PWSTR tokenImpersonationLevel = L"Unknown";
            WCHAR tokenLuid[PH_PTR_STR_LEN_1] = L"Unknown";
            WCHAR authenticationLuid[PH_PTR_STR_LEN_1] = L"Unknown";
            PPH_STRING memoryUsed = NULL;
            PPH_STRING memoryAvailable = NULL;

            if (NT_SUCCESS(tokenPageContext->OpenObject(
                &tokenHandle,
                TOKEN_QUERY,
                tokenPageContext->Context
                )))
            {
                TOKEN_STATISTICS statistics;

                if (NT_SUCCESS(PhGetTokenStatistics(tokenHandle, &statistics)))
                {
                    switch (statistics.TokenType)
                    {
                    case TokenPrimary:
                        tokenType = L"Primary";
                        break;
                    case TokenImpersonation:
                        tokenType = L"Impersonation";
                        break;
                    }

                    switch (statistics.ImpersonationLevel)
                    {
                    case SecurityAnonymous:
                        tokenImpersonationLevel = L"Anonymous";
                        break;
                    case SecurityIdentification:
                        tokenImpersonationLevel = L"Identification";
                        break;
                    case SecurityImpersonation:
                        tokenImpersonationLevel = L"Impersonation";
                        break;
                    case SecurityDelegation:
                        tokenImpersonationLevel = L"Delegation";
                        break;
                    }

                    PhPrintPointer(tokenLuid, (PVOID)statistics.TokenId.LowPart);
                    PhPrintPointer(authenticationLuid, (PVOID)statistics.AuthenticationId.LowPart);

                    // DynamicCharged contains the number of bytes allocated.
                    // DynamicAvailable contains the number of bytes free.
                    memoryUsed = PHA_DEREFERENCE(PhFormatSize(
                        statistics.DynamicCharged - statistics.DynamicAvailable, -1));
                    memoryAvailable = PHA_DEREFERENCE(PhFormatSize(statistics.DynamicCharged, -1));
                }

                NtClose(tokenHandle);
            }

            SetDlgItemText(hwndDlg, IDC_TYPE, tokenType);
            SetDlgItemText(hwndDlg, IDC_IMPERSONATIONLEVEL, tokenImpersonationLevel);
            SetDlgItemText(hwndDlg, IDC_TOKENLUID, tokenLuid);
            SetDlgItemText(hwndDlg, IDC_AUTHENTICATIONLUID, authenticationLuid);
            SetDlgItemText(hwndDlg, IDC_MEMORYUSED, PhGetStringOrDefault(memoryUsed, L"Unknown"));
            SetDlgItemText(hwndDlg, IDC_MEMORYAVAILABLE, PhGetStringOrDefault(memoryAvailable, L"Unknown"));
        }
        break;
    }

    return FALSE;
}
