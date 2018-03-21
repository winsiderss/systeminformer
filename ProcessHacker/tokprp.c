/*
 * Process Hacker -
 *   token properties
 *
 * Copyright (C) 2010-2012 wj32
 * Copyright (C) 2017-2018 dmex
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
#include <appresolver.h>
#include <cpysave.h>
#include <emenu.h>
#include <lsasup.h>
#include <secedit.h>
#include <settings.h>
#include <splitter.h>

#include <windowsx.h>
#include <uxtheme.h>

typedef enum _PH_PROCESS_TOKEN_CATEGORY
{
    PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES,
    PH_PROCESS_TOKEN_CATEGORY_GROUPS
} PH_PROCESS_TOKEN_CATEGORY;

typedef enum _PH_PROCESS_TOKEN_INDEX
{
    PH_PROCESS_TOKEN_INDEX_NAME,
    PH_PROCESS_TOKEN_INDEX_STATUS,
    PH_PROCESS_TOKEN_INDEX_DESCRIPTION,
} PH_PROCESS_TOKEN_INDEX;

typedef struct _PHP_TOKEN_PAGE_LISTVIEW_ITEM
{
    BOOLEAN IsTokenGroupEntry;
    union
    {
        PSID_AND_ATTRIBUTES TokenGroup;
        PLUID_AND_ATTRIBUTES TokenPrivilege;
    };
} PHP_TOKEN_PAGE_LISTVIEW_ITEM, *PPHP_TOKEN_PAGE_LISTVIEW_ITEM;

typedef struct _ATTRIBUTE_NODE
{
    PH_TREENEW_NODE Node;
    PPH_LIST Children;
    PPH_STRING Text;
} ATTRIBUTE_NODE, *PATTRIBUTE_NODE;

typedef struct _ATTRIBUTE_TREE_CONTEXT
{
    PPH_LIST RootList;
    PPH_LIST NodeList;
} ATTRIBUTE_TREE_CONTEXT, *PATTRIBUTE_TREE_CONTEXT;

typedef struct _TOKEN_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PVOID Context;
    DLGPROC HookProc;

    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;

    PTOKEN_GROUPS Groups;
    PTOKEN_GROUPS RestrictedSids;
    PTOKEN_PRIVILEGES Privileges;
    PTOKEN_GROUPS Capabilities;

    ATTRIBUTE_TREE_CONTEXT ClaimsTreeContext;
    ATTRIBUTE_TREE_CONTEXT AuthzTreeContext;
} TOKEN_PAGE_CONTEXT, *PTOKEN_PAGE_CONTEXT;

INT CALLBACK PhpTokenPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

INT_PTR CALLBACK PhpTokenPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhpShowTokenAdvancedProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PTOKEN_PAGE_CONTEXT Context
    );

INT_PTR CALLBACK PhpTokenGeneralPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpTokenAdvancedPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpTokenCapabilitiesPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

BOOLEAN NTAPI PhpAttributeTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

INT_PTR CALLBACK PhpTokenClaimsPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpTokenAttributesPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowTokenProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context,
    _In_opt_ PWSTR Title
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    HPROPSHEETPAGE pages[1];

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hInstance = PhInstanceHandle;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = Title ? Title : L"Token";
    propSheetHeader.nPages = 1;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    pages[0] = PhCreateTokenPage(OpenObject, Context, NULL);

    PhModalPropertySheet(&propSheetHeader);
}

HPROPSHEETPAGE PhCreateTokenPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context,
    _In_opt_ DLGPROC HookProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PTOKEN_PAGE_CONTEXT tokenPageContext;

    tokenPageContext = PhCreateAlloc(sizeof(TOKEN_PAGE_CONTEXT));
    memset(tokenPageContext, 0, sizeof(TOKEN_PAGE_CONTEXT));
    tokenPageContext->OpenObject = OpenObject;
    tokenPageContext->Context = Context;
    tokenPageContext->HookProc = HookProc;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USECALLBACK;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OBJTOKEN);
    propSheetPage.hInstance = PhInstanceHandle;
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
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
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
    _In_ ULONG Attributes,
    _In_ BOOLEAN Restricted
    )
{
    PPH_STRING string;

    if (Attributes & SE_GROUP_INTEGRITY)
    {
        if (Attributes & SE_GROUP_INTEGRITY_ENABLED)
            string = PhReferenceEmptyString();
        else
            string = PhCreateString(L"Disabled");
    }
    else
    {
        if (Attributes & SE_GROUP_ENABLED_BY_DEFAULT)
            string = PhCreateString(L"Default Enabled");
        else if (Attributes & SE_GROUP_ENABLED)
            string = PhReferenceEmptyString();
        else
            string = PhCreateString(L"Disabled");
    }

    if (Restricted)
    {
        PPH_STRING prefixString = string;
        string = PhConcatStrings2(prefixString->Buffer, L" (restricted)");
        PhDereferenceObject(prefixString);
    }

    return string;
}

PWSTR PhGetGroupDescriptionString(
    _In_ ULONG Attributes
    )
{
    PWSTR baseString;

    if (Attributes & SE_GROUP_INTEGRITY)
    {
        if (Attributes & SE_GROUP_INTEGRITY_ENABLED)
            baseString = L"Integrity";
        else
            baseString = NULL;
    }
    else
    {
        if (Attributes & SE_GROUP_LOGON_ID)
            baseString = L"Logon ID";
        else if (Attributes & SE_GROUP_MANDATORY)
            baseString = L"Mandatory";
        else if (Attributes & SE_GROUP_OWNER)
            baseString = L"Owner";
        else if (Attributes & SE_GROUP_RESOURCE)
            baseString = L"Resource";
        else if (Attributes & SE_GROUP_USE_FOR_DENY_ONLY)
            baseString = L"Use for deny only";
        else
            baseString = NULL;
    }

    return baseString;
}


COLORREF PhGetGroupAttributesColor(
    _In_ ULONG Attributes
    )
{
    if (Attributes & SE_GROUP_INTEGRITY)
    {
        if (Attributes & SE_GROUP_INTEGRITY_ENABLED)
            return RGB(0xe0, 0xf0, 0xe0);
        else
            return GetSysColor(COLOR_WINDOW);
    }

    if (Attributes & SE_GROUP_ENABLED_BY_DEFAULT)
        return RGB(0xc0, 0xf0, 0xc0);
    else if (Attributes & SE_GROUP_ENABLED)
        return GetSysColor(COLOR_WINDOW);
    else
        return RGB(0xf0, 0xe0, 0xe0);
}

COLORREF PhGetPrivilegeAttributesColor(
    _In_ ULONG Attributes
    )
{
    if (Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
        return RGB(0xc0, 0xf0, 0xc0);
    else if (Attributes & SE_PRIVILEGE_ENABLED)
        return RGB(0xe0, 0xf0, 0xe0);
    else
        return RGB(0xf0, 0xe0, 0xe0);
}

static COLORREF NTAPI PhpTokenGroupColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    )
{
    PPHP_TOKEN_PAGE_LISTVIEW_ITEM entry = Param;

    if (entry->IsTokenGroupEntry)
        return PhGetGroupAttributesColor(entry->TokenGroup->Attributes);
    else 
        return PhGetPrivilegeAttributesColor(entry->TokenPrivilege->Attributes);
}

PWSTR PhGetPrivilegeAttributesString(
    _In_ ULONG Attributes
    )
{
    if (Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
        return L"Default Enabled";
    else if (Attributes & SE_PRIVILEGE_ENABLED)
        return L"Enabled";
    else
        return L"Disabled";
}

PWSTR PhGetElevationTypeString(
    _In_ TOKEN_ELEVATION_TYPE ElevationType
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

VOID PhpTokenPageFreeListViewEntries(
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext
    )
{
    ULONG index = -1;

    while ((index = PhFindListViewItemByFlags(
        TokenPageContext->ListViewHandle,
        index,
        LVNI_ALL
        )) != -1)
    {
        PPHP_TOKEN_PAGE_LISTVIEW_ITEM entry;

        if (PhGetListViewItemParam(TokenPageContext->ListViewHandle, index, &entry))
        {
            PhFree(entry);
        }
    }
}

VOID PhpUpdateSidsFromTokenGroups(
    _In_ HWND ListViewHandle,
    _In_ PTOKEN_GROUPS Groups,
    _In_ BOOLEAN Restricted
    )
{
    ULONG i;

    for (i = 0; i < Groups->GroupCount; i++)
    {
        INT lvItemIndex;
        PPH_STRING fullName;
        PPH_STRING attributesString;
        PWSTR descriptionString;

        if (!(fullName = PhGetSidFullName(Groups->Groups[i].Sid, TRUE, NULL)))
            fullName = PhSidToStringSid(Groups->Groups[i].Sid);

        if (fullName)
        {
            PPHP_TOKEN_PAGE_LISTVIEW_ITEM lvitem;

            lvitem = PhAllocate(sizeof(PHP_TOKEN_PAGE_LISTVIEW_ITEM));
            memset(lvitem, 0, sizeof(PHP_TOKEN_PAGE_LISTVIEW_ITEM));

            lvitem->IsTokenGroupEntry = TRUE;
            lvitem->TokenGroup = &Groups->Groups[i];

            lvItemIndex = PhAddListViewGroupItem(
                ListViewHandle, 
                PH_PROCESS_TOKEN_CATEGORY_GROUPS, 
                PH_PROCESS_TOKEN_INDEX_NAME, 
                fullName->Buffer, 
                lvitem
                );

            if (attributesString = PhGetGroupAttributesString(Groups->Groups[i].Attributes, Restricted))
            {
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, PH_PROCESS_TOKEN_INDEX_STATUS, PhGetString(attributesString));
                PhDereferenceObject(attributesString);
            }

            if (descriptionString = PhGetGroupDescriptionString(Groups->Groups[i].Attributes))
            {
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, PH_PROCESS_TOKEN_INDEX_DESCRIPTION, descriptionString);
            }
      
            PhDereferenceObject(fullName);
        }
    }
}

BOOLEAN PhpUpdateTokenGroups(
    _In_ HWND hwndDlg,
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext,
    _In_ HWND ListViewHandle,
    _In_ HANDLE TokenHandle
    )
{
    PTOKEN_GROUPS groups;
    PTOKEN_GROUPS restrictedSIDs = NULL;

    if (!NT_SUCCESS(PhGetTokenGroups(TokenHandle, &groups)))
        return FALSE;

    PhpUpdateSidsFromTokenGroups(ListViewHandle, groups, FALSE);

    if (NT_SUCCESS(PhGetTokenRestrictedSids(TokenHandle, &restrictedSIDs)))
    {
        PhpUpdateSidsFromTokenGroups(ListViewHandle, restrictedSIDs, TRUE);
    }

    if (TokenPageContext->RestrictedSids)
        PhFree(TokenPageContext->RestrictedSids);

    TokenPageContext->RestrictedSids = restrictedSIDs;

    if (TokenPageContext->Groups)
        PhFree(TokenPageContext->Groups);

    TokenPageContext->Groups = groups;

    return TRUE;
}

BOOLEAN PhpUpdateTokenPrivileges(
    _In_ HWND hwndDlg,
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext,
    _In_ HWND ListViewHandle,
    _In_ HANDLE TokenHandle
    )
{
    PTOKEN_PRIVILEGES privileges;
    ULONG i;

    if (!NT_SUCCESS(PhGetTokenPrivileges(TokenHandle, &privileges)))
        return FALSE;

    for (i = 0; i < privileges->PrivilegeCount; i++)
    {
        INT lvItemIndex;
        PPH_STRING privilegeName;
        PPH_STRING privilegeDisplayName;

        if (PhLookupPrivilegeName(
            &privileges->Privileges[i].Luid,
            &privilegeName
            ))
        {
            PPHP_TOKEN_PAGE_LISTVIEW_ITEM lvitem;

            lvitem = PhAllocate(sizeof(PHP_TOKEN_PAGE_LISTVIEW_ITEM));
            memset(lvitem, 0, sizeof(PHP_TOKEN_PAGE_LISTVIEW_ITEM));

            lvitem->TokenPrivilege = &privileges->Privileges[i];

            privilegeDisplayName = NULL;
            PhLookupPrivilegeDisplayName(&privilegeName->sr, &privilegeDisplayName);

            // Name
            lvItemIndex = PhAddListViewGroupItem(
                ListViewHandle,
                PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES,
                PH_PROCESS_TOKEN_INDEX_NAME,
                privilegeName->Buffer,
                lvitem
                );
            // Status
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, PH_PROCESS_TOKEN_INDEX_STATUS, PhGetPrivilegeAttributesString(privileges->Privileges[i].Attributes));
            // Description
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, PH_PROCESS_TOKEN_INDEX_DESCRIPTION, PhGetString(privilegeDisplayName));

            if (privilegeDisplayName) PhDereferenceObject(privilegeDisplayName);
            PhDereferenceObject(privilegeName);
        }
    }

    if (TokenPageContext->Privileges)
        PhFree(TokenPageContext->Privileges);

    TokenPageContext->Privileges = privileges;

    return TRUE;
}

FORCEINLINE PTOKEN_PAGE_CONTEXT PhpTokenPageHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return PhpGenericPropertyPageHeader(hwndDlg, uMsg, wParam, lParam, 3);
}

INT_PTR CALLBACK PhpTokenPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
            HANDLE tokenHandle;

            tokenPageContext->ListViewHandle = GetDlgItem(hwndDlg, IDC_GROUPS);
            tokenPageContext->ListViewImageList = ImageList_Create(2, 20, ILC_COLOR, 1, 1);

            PhSetListViewStyle(tokenPageContext->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(tokenPageContext->ListViewHandle, L"explorer");
            PhAddListViewColumn(tokenPageContext->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Name");
            PhAddListViewColumn(tokenPageContext->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Status");
            PhAddListViewColumn(tokenPageContext->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 170, L"Description");

            PhSetExtendedListView(tokenPageContext->ListViewHandle);
            ExtendedListView_SetItemColorFunction(tokenPageContext->ListViewHandle, PhpTokenGroupColorFunction);
            PhLoadListViewColumnsFromSetting(L"TokenGroupsListViewColumns", tokenPageContext->ListViewHandle); 

            ListView_EnableGroupView(tokenPageContext->ListViewHandle, TRUE);
            PhAddListViewGroup(tokenPageContext->ListViewHandle, PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES, L"Privileges");
            PhAddListViewGroup(tokenPageContext->ListViewHandle, PH_PROCESS_TOKEN_CATEGORY_GROUPS, L"Groups");
            ListView_SetImageList(tokenPageContext->ListViewHandle, tokenPageContext->ListViewImageList, LVSIL_SMALL);

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
                PTOKEN_APPCONTAINER_INFORMATION appContainerInfo;
                PPH_STRING appContainerName;
                PPH_STRING appContainerSid;

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
                        SetDlgItemText(hwndDlg, IDC_VIRTUALIZED, L"Not allowed");
                    }
                }

                if (WINDOWS_HAS_IMMERSIVE)
                {
                    appContainerName = NULL;
                    appContainerSid = NULL;

                    if (NT_SUCCESS(PhQueryTokenVariableSize(tokenHandle, TokenAppContainerSid, &appContainerInfo)))
                    {
                        if (appContainerInfo->TokenAppContainer)
                        {
                            appContainerName = PhGetAppContainerName(appContainerInfo->TokenAppContainer);
                            appContainerSid = PhSidToStringSid(appContainerInfo->TokenAppContainer);    
                        }

                        PhFree(appContainerInfo);
                    }

                    if (appContainerName)
                    {
                        PPH_STRING packageFamilyName;

                        packageFamilyName = PhConcatStrings2(appContainerName->Buffer, L" (APP_CONTAINER)");
                        SetDlgItemText(hwndDlg, IDC_USER, packageFamilyName->Buffer);

                        PhDereferenceObject(packageFamilyName);
                        PhDereferenceObject(appContainerName);
                    }

                    if (appContainerSid)
                    {
                        SetDlgItemText(hwndDlg, IDC_USERSID, appContainerSid->Buffer);
                        PhDereferenceObject(appContainerSid);
                    }
                }

                ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, FALSE);
                PhpTokenPageFreeListViewEntries(tokenPageContext);
                ListView_DeleteAllItems(tokenPageContext->ListViewHandle);
                PhpUpdateTokenGroups(hwndDlg, tokenPageContext, tokenPageContext->ListViewHandle, tokenHandle);
                PhpUpdateTokenPrivileges(hwndDlg, tokenPageContext, tokenPageContext->ListViewHandle, tokenHandle);
                ExtendedListView_SortItems(tokenPageContext->ListViewHandle);
                ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, TRUE);

                NtClose(tokenHandle);
            }
        }
        break;
    case WM_DESTROY:
        {
            if (tokenPageContext->ListViewImageList)
                ImageList_Destroy(tokenPageContext->ListViewImageList);

            PhpTokenPageFreeListViewEntries(tokenPageContext);

            PhSaveListViewColumnsToSetting(L"TokenGroupsListViewColumns", tokenPageContext->ListViewHandle);

            if (tokenPageContext->Groups) PhFree(tokenPageContext->Groups);
            if (tokenPageContext->RestrictedSids) PhFree(tokenPageContext->RestrictedSids);
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
                    BOOLEAN listViewGroupItemsValid = FALSE;
                    PPHP_TOKEN_PAGE_LISTVIEW_ITEM *listViewItems;
                    ULONG numberOfItems;
                    HANDLE tokenHandle;
                    ULONG i;

                    PhGetSelectedListViewItemParams(
                        tokenPageContext->ListViewHandle,
                        &listViewItems,
                        &numberOfItems
                        );

                    for (i = 0; i < numberOfItems; i++)
                    {
                        if (listViewItems[i]->IsTokenGroupEntry)
                        {
                            listViewGroupItemsValid = TRUE;
                            break;
                        }
                    }

                    if (listViewGroupItemsValid)
                    {
                        PhFree(listViewItems);
                        break;
                    }

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

                    status = tokenPageContext->OpenObject(
                        &tokenHandle,
                        TOKEN_ADJUST_PRIVILEGES,
                        tokenPageContext->Context
                        );

                    if (NT_SUCCESS(status))
                    {
                        ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, FALSE);

                        for (i = 0; i < numberOfItems; i++)
                        {
                            PPH_STRING privilegeName = NULL;
                            ULONG newAttributes;

                            PhLookupPrivilegeName(&listViewItems[i]->TokenPrivilege->Luid, &privilegeName);
                            PH_AUTO(privilegeName);

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
                                listViewItems[i]->TokenPrivilege->Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT &&
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
                                &listViewItems[i]->TokenPrivilege->Luid,
                                newAttributes
                                ))
                            {
                                INT lvItemIndex = PhFindListViewItemByParam(
                                    tokenPageContext->ListViewHandle,
                                    -1,
                                    listViewItems[i]
                                    );

                                if (LOWORD(wParam) != ID_PRIVILEGE_REMOVE)
                                {
                                    // Refresh the status text (and background color).
                                    listViewItems[i]->TokenPrivilege->Attributes = newAttributes;
                                    PhSetListViewSubItem(
                                        tokenPageContext->ListViewHandle,
                                        lvItemIndex,
                                        PH_PROCESS_TOKEN_INDEX_STATUS,
                                        PhGetPrivilegeAttributesString(newAttributes)
                                        );
                                }
                                else
                                {
                                    ListView_DeleteItem(
                                        tokenPageContext->ListViewHandle,
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
                                    STATUS_UNSUCCESSFUL,
                                    0
                                    ))
                                    break;
                            }
                        }

                        ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, TRUE);

                        NtClose(tokenHandle);
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to open the token", status, 0);
                    }

                    PhFree(listViewItems);

                    ExtendedListView_SortItems(tokenPageContext->ListViewHandle);
                }
                break;
            case ID_PRIVILEGE_COPY:
                {
                    PhCopyListView(tokenPageContext->ListViewHandle);
                }
                break;
            case IDC_PERMISSIONS:
                {
                    PH_STD_OBJECT_SECURITY stdObjectSecurity;
                    PPH_ACCESS_ENTRY accessEntries;
                    ULONG numberOfAccessEntries;

                    stdObjectSecurity.OpenObject = tokenPageContext->OpenObject;
                    stdObjectSecurity.ObjectType = L"Token";
                    stdObjectSecurity.Context = tokenPageContext->Context;

                    if (PhGetAccessEntries(L"Token", &accessEntries, &numberOfAccessEntries))
                    {
                        PhEditSecurity(
                            hwndDlg,
                            L"Token",
                            PhStdGetObjectSecurity,
                            PhStdSetObjectSecurity,
                            &stdObjectSecurity,
                            accessEntries,
                            numberOfAccessEntries
                            );
                        PhFree(accessEntries);
                    }
                }
                break;
            case IDC_INTEGRITY:
                {
                    NTSTATUS status;
                    RECT rect;
                    PPH_EMENU menu;
                    HANDLE tokenHandle;
                    MANDATORY_LEVEL_RID integrityLevelRID;
                    PPH_EMENU_ITEM selectedItem;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_INTEGRITY), &rect);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatorySecureProcessRID, L"Protected", NULL, NULL), -1);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatorySystemRID, L"System", NULL, NULL), -1);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatoryHighRID, L"High", NULL, NULL), -1);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatoryMediumRID, L"Medium", NULL, NULL), -1);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatoryLowRID, L"Low", NULL, NULL), -1);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatoryUntrustedRID, L"Untrusted", NULL, NULL), -1);

                    integrityLevelRID = -1;

                    // Put a radio check on the menu item that corresponds with the current integrity level.
                    // Also disable menu items which correspond to higher integrity levels since
                    // NtSetInformationToken doesn't allow integrity levels to be raised.
                    if (NT_SUCCESS(status = tokenPageContext->OpenObject(
                        &tokenHandle,
                        TOKEN_QUERY,
                        tokenPageContext->Context
                        )))
                    {
                        if (NT_SUCCESS(status = PhGetTokenIntegrityLevelRID(
                            tokenHandle,
                            &integrityLevelRID,
                            NULL
                            )))
                        {
                            ULONG customLevelPosition = 0; // Processing atypical integrity levels

                            for (ULONG i = 0; i < menu->Items->Count; i++)
                            {
                                PPH_EMENU_ITEM menuItem = menu->Items->Items[i];

                                if (menuItem->Id == (ULONG)integrityLevelRID)
                                {
                                    menuItem->Flags |= PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK;
                                    customLevelPosition = 0; // The integrity level is a well-known one. No need to add a new menu item.
                                }
                                else if (menuItem->Id > (ULONG)integrityLevelRID)
                                {
                                    menuItem->Flags |= PH_EMENU_DISABLED;
                                    customLevelPosition = i + 1;
                                }
                            }

                            if (customLevelPosition)
                            {
                                PPH_EMENU_ITEM unknownIntegrityItem;
                                
                                unknownIntegrityItem = PhCreateEMenuItem(0, (ULONG)integrityLevelRID, L"Intermediate level", NULL, NULL);
                                unknownIntegrityItem->Flags |= PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK;
                                PhInsertEMenuItem(menu, unknownIntegrityItem, customLevelPosition);
                            }
                        }

                        NtClose(tokenHandle);
                    }

                    if (!NT_SUCCESS(status))
                    {
                        for (ULONG i = 0; i < menu->Items->Count; i++)
                        {
                            PPH_EMENU_ITEM menuItem = menu->Items->Items[i];
                            menuItem->Flags |= PH_EMENU_DISABLED;
                        }
                    }

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id != integrityLevelRID)
                    {
                        if (PhShowConfirmMessage(
                            hwndDlg,
                            L"set",
                            L"the integrity level",
                            L"Once lowered, the integrity level of the token cannot be raised again.",
                            FALSE
                            ))
                        {
                            if (NT_SUCCESS(status = tokenPageContext->OpenObject(
                                &tokenHandle,
                                TOKEN_QUERY | TOKEN_ADJUST_DEFAULT,
                                tokenPageContext->Context
                                )))
                            {
                                static SID_IDENTIFIER_AUTHORITY mandatoryLabelAuthority = SECURITY_MANDATORY_LABEL_AUTHORITY;

                                UCHAR newSidBuffer[FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG)];
                                PSID newSid;
                                TOKEN_MANDATORY_LABEL mandatoryLabel;

                                newSid = (PSID)newSidBuffer;
                                RtlInitializeSid(newSid, &mandatoryLabelAuthority, 1);
                                *RtlSubAuthoritySid(newSid, 0) = selectedItem->Id;
                                mandatoryLabel.Label.Sid = newSid;
                                mandatoryLabel.Label.Attributes = SE_GROUP_INTEGRITY;

                                status = NtSetInformationToken(
                                    tokenHandle,
                                    TokenIntegrityLevel,
                                    &mandatoryLabel,
                                    sizeof(TOKEN_MANDATORY_LABEL)
                                    );

                                if (NT_SUCCESS(status))
                                {
                                    ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, FALSE);
                                    PhpTokenPageFreeListViewEntries(tokenPageContext);
                                    ListView_DeleteAllItems(tokenPageContext->ListViewHandle);
                                    PhpUpdateTokenGroups(hwndDlg, tokenPageContext, tokenPageContext->ListViewHandle, tokenHandle);
                                    PhpUpdateTokenPrivileges(hwndDlg, tokenPageContext, tokenPageContext->ListViewHandle, tokenHandle);
                                    ExtendedListView_SortItems(tokenPageContext->ListViewHandle);
                                    ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, TRUE);
                                }

                                NtClose(tokenHandle);
                            }

                            if (!NT_SUCCESS(status))
                                PhShowStatus(hwndDlg, L"Unable to set the integrity level", status, 0);
                        }
                    }

                    PhDestroyEMenu(menu);
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
            case PSN_QUERYINITIALFOCUS:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_SESSIONID));
                    return TRUE;
                }
                break;
            }

            PhHandleListViewNotifyBehaviors(lParam, tokenPageContext->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == tokenPageContext->ListViewHandle)
            {
                POINT point;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                if (ListView_GetSelectedCount(tokenPageContext->ListViewHandle) != 0)
                {
                    PPH_EMENU menu;
                    BOOLEAN listViewGroupItemsValid = TRUE;
                    PPHP_TOKEN_PAGE_LISTVIEW_ITEM *listviewItems;
                    ULONG numberOfItems;
                    ULONG i;

                    PhGetSelectedListViewItemParams(
                        tokenPageContext->ListViewHandle,
                        &listviewItems,
                        &numberOfItems
                        );

                    for (i = 0; i < numberOfItems; i++)
                    {
                        if (listviewItems[i]->IsTokenGroupEntry)
                        {
                            listViewGroupItemsValid = FALSE;
                            break;
                        }
                    }

                    menu = PhCreateEMenu();
                    if (listViewGroupItemsValid)
                    {
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PRIVILEGE_ENABLE, L"&Enable", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PRIVILEGE_DISABLE, L"&Disable", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PRIVILEGE_REMOVE, L"&Remove", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), -1);
                    }
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PRIVILEGE_COPY, L"&Copy", NULL, NULL), -1);
                    PhShowEMenu(
                        menu, 
                        hwndDlg, 
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP, 
                        point.x, 
                        point.y
                        );
                    PhDestroyEMenu(menu);

                    PhFree(listviewItems);
                }
            }
        }
        break;
    }

    REFLECT_MESSAGE_DLG(hwndDlg, tokenPageContext->ListViewHandle, uMsg, wParam, lParam);

    return FALSE;
}

VOID PhpShowTokenAdvancedProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PTOKEN_PAGE_CONTEXT Context
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    HPROPSHEETPAGE pages[5];
    PROPSHEETPAGE page;
    ULONG numberOfPages;

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hInstance = PhInstanceHandle;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = L"Token";
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    numberOfPages = 0;

    // General

    memset(&page, 0, sizeof(PROPSHEETPAGE));
    page.dwSize = sizeof(PROPSHEETPAGE);
    page.pszTemplate = MAKEINTRESOURCE(IDD_TOKGENERAL);
    page.hInstance = PhInstanceHandle;
    page.pfnDlgProc = PhpTokenGeneralPageProc;
    page.lParam = (LPARAM)Context;
    pages[numberOfPages++] = CreatePropertySheetPage(&page);

    // Advanced

    memset(&page, 0, sizeof(PROPSHEETPAGE));
    page.dwSize = sizeof(PROPSHEETPAGE);
    page.pszTemplate = MAKEINTRESOURCE(IDD_TOKADVANCED);
    page.hInstance = PhInstanceHandle;
    page.pfnDlgProc = PhpTokenAdvancedPageProc;
    page.lParam = (LPARAM)Context;
    pages[numberOfPages++] = CreatePropertySheetPage(&page);

    if (WindowsVersion >= WINDOWS_8)
    {
        // Capabilities

        memset(&page, 0, sizeof(PROPSHEETPAGE));
        page.dwSize = sizeof(PROPSHEETPAGE);
        page.pszTemplate = MAKEINTRESOURCE(IDD_TOKCAPABILITIES);
        page.hInstance = PhInstanceHandle;
        page.pfnDlgProc = PhpTokenCapabilitiesPageProc;
        page.lParam = (LPARAM)Context;
        pages[numberOfPages++] = CreatePropertySheetPage(&page);

        // Claims

        memset(&page, 0, sizeof(PROPSHEETPAGE));
        page.dwSize = sizeof(PROPSHEETPAGE);
        page.dwFlags = PSP_USETITLE;
        page.pszTemplate = MAKEINTRESOURCE(IDD_TOKATTRIBUTES);
        page.hInstance = PhInstanceHandle;
        page.pszTitle = L"Claims";
        page.pfnDlgProc = PhpTokenClaimsPageProc;
        page.lParam = (LPARAM)Context;
        pages[numberOfPages++] = CreatePropertySheetPage(&page);

        // (Token) Attributes

        memset(&page, 0, sizeof(PROPSHEETPAGE));
        page.dwSize = sizeof(PROPSHEETPAGE);
        page.dwFlags = PSP_USETITLE;
        page.pszTemplate = MAKEINTRESOURCE(IDD_TOKATTRIBUTES);
        page.hInstance = PhInstanceHandle;
        page.pszTitle = L"Attributes";
        page.pfnDlgProc = PhpTokenAttributesPageProc;
        page.lParam = (LPARAM)Context;
        pages[numberOfPages++] = CreatePropertySheetPage(&page);
    }

    propSheetHeader.nPages = numberOfPages;
    PhModalPropertySheet(&propSheetHeader);
}

static NTSTATUS PhpOpenLinkedToken(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    return PhGetTokenLinkedToken((HANDLE)Context, Handle);
}

INT_PTR CALLBACK PhpTokenGeneralPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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

            // HACK
            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg)));

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
                    tokenUserName = PH_AUTO(PhGetSidFullName(tokenUser->User.Sid, TRUE, NULL));
                    tokenUserSid = PH_AUTO(PhSidToStringSid(tokenUser->User.Sid));

                    PhFree(tokenUser);
                }

                if (NT_SUCCESS(PhGetTokenOwner(tokenHandle, &tokenOwner)))
                {
                    tokenOwnerName = PH_AUTO(PhGetSidFullName(tokenOwner->Owner, TRUE, NULL));
                    PhFree(tokenOwner);
                }

                if (NT_SUCCESS(PhGetTokenPrimaryGroup(tokenHandle, &tokenPrimaryGroup)))
                {
                    tokenPrimaryGroupName = PH_AUTO(PhGetSidFullName(
                        tokenPrimaryGroup->PrimaryGroup, TRUE, NULL));
                    PhFree(tokenPrimaryGroup);
                }

                PhGetTokenSessionId(tokenHandle, &tokenSessionId);

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
                            tokenVirtualization = isVirtualizationEnabled ? L"Enabled" : L"Disabled";
                        }
                    }
                    else
                    {
                        tokenVirtualization = L"Not Allowed";
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
                    PhCopyStringZFromBytes(
                        tokenSource.SourceName,
                        TOKEN_SOURCE_LENGTH,
                        tokenSourceName,
                        sizeof(tokenSourceName) / 2,
                        NULL
                        );

                    PhPrintPointer(tokenSourceLuid, UlongToPtr(tokenSource.SourceIdentifier.LowPart));
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
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_LINKEDTOKEN));
                    return TRUE;
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpTokenAdvancedPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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

                    if (statistics.TokenType == TokenImpersonation)
                    {
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
                    }
                    else
                    {
                        tokenImpersonationLevel = L"N/A";
                    }

                    PhPrintPointer(tokenLuid, UlongToPtr(statistics.TokenId.LowPart));
                    PhPrintPointer(authenticationLuid, UlongToPtr(statistics.AuthenticationId.LowPart));

                    // DynamicCharged contains the number of bytes allocated.
                    // DynamicAvailable contains the number of bytes free.
                    memoryUsed = PhaFormatSize(statistics.DynamicCharged - statistics.DynamicAvailable, -1);
                    memoryAvailable = PhaFormatSize(statistics.DynamicCharged, -1);
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

static COLORREF NTAPI PhpTokenCapabilitiesColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    )
{
    PSID_AND_ATTRIBUTES sidAndAttributes = Param;

    return PhGetGroupAttributesColor(sidAndAttributes->Attributes);
}

INT_PTR CALLBACK PhpTokenCapabilitiesPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PTOKEN_PAGE_CONTEXT tokenPageContext;
    HWND lvHandle;

    tokenPageContext = PhpTokenPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!tokenPageContext)
        return FALSE;

    lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE tokenHandle;
            ULONG i;

            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 160, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Flags");
            PhSetExtendedListView(lvHandle);
            ExtendedListView_SetItemColorFunction(lvHandle, PhpTokenCapabilitiesColorFunction);

            if (NT_SUCCESS(tokenPageContext->OpenObject(
                &tokenHandle,
                TOKEN_QUERY,
                tokenPageContext->Context
                )))
            {
                if (NT_SUCCESS(PhQueryTokenVariableSize(tokenHandle, TokenCapabilities, &tokenPageContext->Capabilities)))
                {
                    for (i = 0; i < tokenPageContext->Capabilities->GroupCount; i++)
                    {
                        INT lvItemIndex;
                        PPH_STRING name;
                        PPH_STRING attributesString;

                        name = PhGetSidFullName(tokenPageContext->Capabilities->Groups[i].Sid, TRUE, NULL);

                        if (!name)
                            name = PhSidToStringSid(tokenPageContext->Capabilities->Groups[i].Sid);

                        if (name)
                        {
                            lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, name->Buffer,
                                &tokenPageContext->Capabilities->Groups[i]);
                            attributesString = PhGetGroupAttributesString(
                                tokenPageContext->Capabilities->Groups[i].Attributes, FALSE);
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, attributesString->Buffer);

                            PhDereferenceObject(attributesString);
                            PhDereferenceObject(name);
                        }
                    }

                    if (ListView_GetItemCount(lvHandle) != 0)
                    {
                        ListView_SetColumnWidth(lvHandle, 0, LVSCW_AUTOSIZE);
                        ExtendedListView_SetColumnWidth(lvHandle, 1, ELVSCW_AUTOSIZE_REMAININGSPACE);
                    }

                    ExtendedListView_SortItems(lvHandle);
                }

                NtClose(tokenHandle);
            }

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhFree(tokenPageContext->Capabilities);
            tokenPageContext->Capabilities = NULL;
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, lvHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    }

    REFLECT_MESSAGE_DLG(hwndDlg, lvHandle, uMsg, wParam, lParam);

    return FALSE;
}

BOOLEAN NTAPI PhpAttributeTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PATTRIBUTE_TREE_CONTEXT context;
    PATTRIBUTE_NODE node;

    context = Context;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            node = (PATTRIBUTE_NODE)getChildren->Node;

            if (!node)
            {
                getChildren->Children = (PPH_TREENEW_NODE *)context->RootList->Items;
                getChildren->NumberOfChildren = context->RootList->Count;
            }
            else
            {
                getChildren->Children = (PPH_TREENEW_NODE *)node->Children->Items;
                getChildren->NumberOfChildren = node->Children->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            node = (PATTRIBUTE_NODE)isLeaf->Node;

            isLeaf->IsLeaf = node->Children->Count == 0;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;

            node = (PATTRIBUTE_NODE)getCellText->Node;

            if (getCellText->Id == 0)
                getCellText->Text = PhGetStringRef(node->Text);
            else
                return FALSE;
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(hwnd, 0);
                    PhSetClipboardString(hwnd, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            }
        }
        return TRUE;
    }

    return FALSE;
}

PATTRIBUTE_NODE PhpAddAttributeNode(
    _In_ PATTRIBUTE_TREE_CONTEXT Context,
    _In_opt_ PATTRIBUTE_NODE Parent,
    _In_opt_ _Assume_refs_(1) PPH_STRING Text
    )
{
    PATTRIBUTE_NODE node;

    node = PhAllocate(sizeof(ATTRIBUTE_NODE));
    memset(node, 0, sizeof(ATTRIBUTE_NODE));
    PhInitializeTreeNewNode(&node->Node);

    node->Children = PhCreateList(2);

    PhAddItemList(Context->NodeList, node);

    if (Parent)
        PhAddItemList(Parent->Children, node);
    else
        PhAddItemList(Context->RootList, node);

    PhMoveReference(&node->Text, Text);

    return node;
}

VOID PhpDestroyAttributeNode(
    _In_ PATTRIBUTE_NODE Node
    )
{
    PhDereferenceObject(Node->Children);
    PhClearReference(&Node->Text);
    PhFree(Node);
}

VOID PhpInitializeAttributeTreeContext(
    _Out_ PATTRIBUTE_TREE_CONTEXT Context,
    _In_ HWND TreeNewHandle
    )
{
    PH_TREENEW_VIEW_PARTS parts;

    Context->NodeList = PhCreateList(10);
    Context->RootList = PhCreateList(10);

    PhSetControlTheme(TreeNewHandle, L"explorer");
    TreeNew_SetCallback(TreeNewHandle, PhpAttributeTreeNewCallback, Context);
    TreeNew_GetViewParts(TreeNewHandle, &parts);
    PhAddTreeNewColumnEx2(TreeNewHandle, 0, TRUE, L"Attributes", parts.ClientRect.right - parts.VScrollWidth, PH_ALIGN_LEFT, 0, 0, TN_COLUMN_FLAG_NODPISCALEONADD);
}

VOID PhpDeleteAttributeTreeContext(
    _Inout_ PATTRIBUTE_TREE_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
        PhpDestroyAttributeNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeList);
    PhDereferenceObject(Context->RootList);
}

PWSTR PhGetSecurityAttributeTypeString(
    _In_ USHORT Type
    )
{
    // These types are shared between CLAIM_* and TOKEN_* security attributes.

    switch (Type)
    {
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_INVALID:
        return L"Invalid";
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_INT64:
        return L"Int64";
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_UINT64:
        return L"UInt64";
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING:
        return L"String";
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_FQBN:
        return L"FQBN";
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_SID:
        return L"SID";
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_BOOLEAN:
        return L"Boolean";
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_OCTET_STRING:
        return L"Octet string";
    default:
        return L"(Unknown)";
    }
}

PPH_STRING PhGetSecurityAttributeFlagsString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER sb;

    // These flags are shared between CLAIM_* and TOKEN_* security attributes.

    PhInitializeStringBuilder(&sb, 100);

    if (Flags & TOKEN_SECURITY_ATTRIBUTE_MANDATORY)
        PhAppendStringBuilder2(&sb, L"Mandatory, ");
    if (Flags & TOKEN_SECURITY_ATTRIBUTE_DISABLED)
        PhAppendStringBuilder2(&sb, L"Disabled, ");
    if (Flags & TOKEN_SECURITY_ATTRIBUTE_DISABLED_BY_DEFAULT)
        PhAppendStringBuilder2(&sb, L"Default disabled, ");
    if (Flags & TOKEN_SECURITY_ATTRIBUTE_USE_FOR_DENY_ONLY)
        PhAppendStringBuilder2(&sb, L"Use for deny only, ");
    if (Flags & TOKEN_SECURITY_ATTRIBUTE_VALUE_CASE_SENSITIVE)
        PhAppendStringBuilder2(&sb, L"Case-sensitive, ");
    if (Flags & TOKEN_SECURITY_ATTRIBUTE_NON_INHERITABLE)
        PhAppendStringBuilder2(&sb, L"Non-inheritable, ");

    if (sb.String->Length != 0)
        PhRemoveEndStringBuilder(&sb, 2);
    else
        PhAppendStringBuilder2(&sb, L"(None)");

    return PhFinalStringBuilderString(&sb);
}

PPH_STRING PhFormatClaimSecurityAttributeValue(
    _In_ PCLAIM_SECURITY_ATTRIBUTE_V1 Attribute,
    _In_ ULONG ValueIndex
    )
{
    PH_FORMAT format;

    switch (Attribute->ValueType)
    {
    case CLAIM_SECURITY_ATTRIBUTE_TYPE_INT64:
        PhInitFormatI64D(&format, Attribute->Values.pInt64[ValueIndex]);
        return PhFormat(&format, 1, 0);
    case CLAIM_SECURITY_ATTRIBUTE_TYPE_UINT64:
        PhInitFormatI64U(&format, Attribute->Values.pUint64[ValueIndex]);
        return PhFormat(&format, 1, 0);
    case CLAIM_SECURITY_ATTRIBUTE_TYPE_STRING:
        return PhCreateString(Attribute->Values.ppString[ValueIndex]);
    case CLAIM_SECURITY_ATTRIBUTE_TYPE_FQBN:
        return PhFormatString(L"Version %I64u: %s",
            Attribute->Values.pFqbn[ValueIndex].Version,
            Attribute->Values.pFqbn[ValueIndex].Name);
    case CLAIM_SECURITY_ATTRIBUTE_TYPE_SID:
        {
            if (RtlValidSid(Attribute->Values.pOctetString[ValueIndex].pValue))
            {
                PPH_STRING name;

                name = PhGetSidFullName(Attribute->Values.pOctetString[ValueIndex].pValue, TRUE, NULL);

                if (name)
                    return name;

                name = PhSidToStringSid(Attribute->Values.pOctetString[ValueIndex].pValue);

                if (name)
                    return name;
            }
        }
        return PhCreateString(L"(Invalid SID)");
    case CLAIM_SECURITY_ATTRIBUTE_TYPE_BOOLEAN:
        return PhCreateString(Attribute->Values.pInt64[ValueIndex] != 0 ? L"True" : L"False");
    case CLAIM_SECURITY_ATTRIBUTE_TYPE_OCTET_STRING:
        return PhCreateString(L"(Octet string)");
    default:
        return PhCreateString(L"(Unknown)");
    }
}

PPH_STRING PhFormatTokenSecurityAttributeValue(
    _In_ PTOKEN_SECURITY_ATTRIBUTE_V1 Attribute,
    _In_ ULONG ValueIndex
    )
{
    PH_FORMAT format;

    switch (Attribute->ValueType)
    {
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_INT64:
        PhInitFormatI64D(&format, Attribute->Values.pInt64[ValueIndex]);
        return PhFormat(&format, 1, 0);
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_UINT64:
        PhInitFormatI64U(&format, Attribute->Values.pUint64[ValueIndex]);
        return PhFormat(&format, 1, 0);
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING:
        return PhCreateStringFromUnicodeString(&Attribute->Values.pString[ValueIndex]);
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_FQBN:
        return PhFormatString(L"Version %I64u: %.*s",
            Attribute->Values.pFqbn[ValueIndex].Version,
            Attribute->Values.pFqbn[ValueIndex].Name.Length / sizeof(WCHAR),
            Attribute->Values.pFqbn[ValueIndex].Name.Buffer);
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_SID:
        {
            if (RtlValidSid(Attribute->Values.pOctetString[ValueIndex].pValue))
            {
                PPH_STRING name;

                name = PhGetSidFullName(Attribute->Values.pOctetString[ValueIndex].pValue, TRUE, NULL);

                if (name)
                    return name;

                name = PhSidToStringSid(Attribute->Values.pOctetString[ValueIndex].pValue);

                if (name)
                    return name;
            }
        }
        return PhCreateString(L"(Invalid SID)");
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_BOOLEAN:
        return PhCreateString(Attribute->Values.pInt64[ValueIndex] != 0 ? L"True" : L"False");
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_OCTET_STRING:
        return PhCreateString(L"(Octet string)");
    default:
        return PhCreateString(L"(Unknown)");
    }
}

BOOLEAN PhpAddTokenClaimAttributes(
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext,
    _In_ HWND tnHandle,
    _In_ BOOLEAN DeviceClaims,
    _In_ PATTRIBUTE_NODE Parent
    )
{
    HANDLE tokenHandle;
    PCLAIM_SECURITY_ATTRIBUTES_INFORMATION info;
    ULONG i;
    ULONG j;

    if (!NT_SUCCESS(TokenPageContext->OpenObject(
        &tokenHandle,
        TOKEN_QUERY,
        TokenPageContext->Context
        )))
        return FALSE;

    if (NT_SUCCESS(PhQueryTokenVariableSize(tokenHandle, DeviceClaims ? TokenDeviceClaimAttributes : TokenUserClaimAttributes, &info)))
    {
        for (i = 0; i < info->AttributeCount; i++)
        {
            PCLAIM_SECURITY_ATTRIBUTE_V1 attribute = &info->Attribute.pAttributeV1[i];
            PATTRIBUTE_NODE node;
            PPH_STRING temp;

            // Attribute
            node = PhpAddAttributeNode(&TokenPageContext->ClaimsTreeContext, Parent, PhCreateString(attribute->Name));
            // Type
            PhpAddAttributeNode(&TokenPageContext->ClaimsTreeContext, node,
                PhFormatString(L"Type: %s", PhGetSecurityAttributeTypeString(attribute->ValueType)));
            // Flags
            temp = PhGetSecurityAttributeFlagsString(attribute->Flags);
            PhpAddAttributeNode(&TokenPageContext->ClaimsTreeContext, node,
                PhFormatString(L"Flags: %s", temp->Buffer));
            PhDereferenceObject(temp);

            // Values
            for (j = 0; j < attribute->ValueCount; j++)
            {
                temp = PhFormatClaimSecurityAttributeValue(attribute, j);
                PhpAddAttributeNode(&TokenPageContext->ClaimsTreeContext, node,
                    PhFormatString(L"Value %u: %s", j, temp->Buffer));
                PhDereferenceObject(temp);
            }
        }

        PhFree(info);
    }

    NtClose(tokenHandle);

    TreeNew_NodesStructured(tnHandle);

    return TRUE;
}

INT_PTR CALLBACK PhpTokenClaimsPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PTOKEN_PAGE_CONTEXT tokenPageContext;
    HWND tnHandle;

    tokenPageContext = PhpTokenPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!tokenPageContext)
        return FALSE;

    tnHandle = GetDlgItem(hwndDlg, IDC_LIST);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PATTRIBUTE_NODE userNode;
            PATTRIBUTE_NODE deviceNode;

            PhpInitializeAttributeTreeContext(&tokenPageContext->ClaimsTreeContext, tnHandle);

            TreeNew_SetRedraw(tnHandle, FALSE);

            userNode = PhpAddAttributeNode(&tokenPageContext->ClaimsTreeContext, NULL, PhCreateString(L"User claims"));
            PhpAddTokenClaimAttributes(tokenPageContext, tnHandle, FALSE, userNode);
            deviceNode = PhpAddAttributeNode(&tokenPageContext->ClaimsTreeContext, NULL, PhCreateString(L"Device claims"));
            PhpAddTokenClaimAttributes(tokenPageContext, tnHandle, TRUE, deviceNode);

            if (userNode->Children->Count == 0)
                PhpAddAttributeNode(&tokenPageContext->ClaimsTreeContext, userNode, PhCreateString(L"(None)"));
            if (deviceNode->Children->Count == 0)
                PhpAddAttributeNode(&tokenPageContext->ClaimsTreeContext, deviceNode, PhCreateString(L"(None)"));

            TreeNew_NodesStructured(tnHandle);
            TreeNew_SetRedraw(tnHandle, TRUE);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhpDeleteAttributeTreeContext(&tokenPageContext->ClaimsTreeContext);
        }
        break;
    }

    return FALSE;
}

BOOLEAN PhpAddTokenAttributes(
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext,
    _In_ HWND tnHandle
    )
{
    HANDLE tokenHandle;
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;
    ULONG i;
    ULONG j;

    if (!NT_SUCCESS(TokenPageContext->OpenObject(
        &tokenHandle,
        TOKEN_QUERY,
        TokenPageContext->Context
        )))
        return FALSE;

    if (NT_SUCCESS(PhQueryTokenVariableSize(tokenHandle, TokenSecurityAttributes, &info)))
    {
        for (i = 0; i < info->AttributeCount; i++)
        {
            PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = &info->Attribute.pAttributeV1[i];
            PATTRIBUTE_NODE node;
            PPH_STRING temp;

            // Attribute
            node = PhpAddAttributeNode(&TokenPageContext->AuthzTreeContext, NULL,
                PhCreateStringFromUnicodeString(&attribute->Name));
            // Type
            PhpAddAttributeNode(&TokenPageContext->AuthzTreeContext, node,
                PhFormatString(L"Type: %s", PhGetSecurityAttributeTypeString(attribute->ValueType)));
            // Flags
            temp = PhGetSecurityAttributeFlagsString(attribute->Flags);
            PhpAddAttributeNode(&TokenPageContext->AuthzTreeContext, node,
                PhFormatString(L"Flags: %s", temp->Buffer));
            PhDereferenceObject(temp);

            // Values
            for (j = 0; j < attribute->ValueCount; j++)
            {
                temp = PhFormatTokenSecurityAttributeValue(attribute, j);
                PhpAddAttributeNode(&TokenPageContext->AuthzTreeContext, node,
                    PhFormatString(L"Value %u: %s", j, temp->Buffer));
                PhDereferenceObject(temp);
            }
        }

        PhFree(info);
    }

    NtClose(tokenHandle);

    TreeNew_NodesStructured(tnHandle);

    return TRUE;
}

INT_PTR CALLBACK PhpTokenAttributesPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PTOKEN_PAGE_CONTEXT tokenPageContext;
    HWND tnHandle;

    tokenPageContext = PhpTokenPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!tokenPageContext)
        return FALSE;

    tnHandle = GetDlgItem(hwndDlg, IDC_LIST);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhpInitializeAttributeTreeContext(&tokenPageContext->AuthzTreeContext, tnHandle);

            TreeNew_SetRedraw(tnHandle, FALSE);

            PhpAddTokenAttributes(tokenPageContext, tnHandle);

            if (tokenPageContext->AuthzTreeContext.RootList->Count == 0)
                PhpAddAttributeNode(&tokenPageContext->AuthzTreeContext, NULL, PhCreateString(L"(None)"));

            TreeNew_NodesStructured(tnHandle);
            TreeNew_SetRedraw(tnHandle, TRUE);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhpDeleteAttributeTreeContext(&tokenPageContext->AuthzTreeContext);
        }
        break;
    }

    return FALSE;
}
