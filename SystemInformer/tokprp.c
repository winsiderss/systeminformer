/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2012
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <apiimport.h>
#include <appresolver.h>
#include <cpysave.h>
#include <emenu.h>
#include <hndlinfo.h>
#include <lsasup.h>
#include <mapldr.h>
#include <secedit.h>
#include <settings.h>
#include <symprv.h>
#include <workqueue.h>
#include <phsettings.h>

#include <appmodel.h>

typedef enum _PH_PROCESS_TOKEN_CATEGORY
{
    PH_PROCESS_TOKEN_CATEGORY_DANGEROUS_FLAGS,
    PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES,
    PH_PROCESS_TOKEN_CATEGORY_GROUPS,
    PH_PROCESS_TOKEN_CATEGORY_RESTRICTED
} PH_PROCESS_TOKEN_CATEGORY;

typedef enum _PH_PROCESS_TOKEN_FLAG
{
    PH_PROCESS_TOKEN_FLAG_NO_WRITE_UP,
    PH_PROCESS_TOKEN_FLAG_SANDBOX_INERT,
    PH_PROCESS_TOKEN_FLAG_UIACCESS
} PH_PROCESS_TOKEN_FLAG;

typedef enum _PH_PROCESS_TOKEN_INDEX
{
    PH_PROCESS_TOKEN_INDEX_NAME,
    PH_PROCESS_TOKEN_INDEX_STATUS,
    PH_PROCESS_TOKEN_INDEX_DESCRIPTION,
    PH_PROCESS_TOKEN_INDEX_SID,
    PH_PROCESS_TOKEN_INDEX_TYPE,
    PH_PROCESS_TOKEN_INDEX_USE,
} PH_PROCESS_TOKEN_INDEX;

typedef struct _PHP_TOKEN_PAGE_LISTVIEW_ITEM
{
    PH_PROCESS_TOKEN_CATEGORY ItemCategory;
    PSID_AND_ATTRIBUTES TokenGroup;
    PLUID_AND_ATTRIBUTES TokenPrivilege;
    PH_PROCESS_TOKEN_FLAG ItemFlag;
    BOOLEAN ItemFlagState;
} PHP_TOKEN_PAGE_LISTVIEW_ITEM, *PPHP_TOKEN_PAGE_LISTVIEW_ITEM;

typedef struct _PHP_TOKEN_USER_RESOLVE_CONTEXT
{
    HWND WindowHandle;
    PSID TokenUserSid;
} PHP_TOKEN_USER_RESOLVE_CONTEXT, *PPHP_TOKEN_USER_RESOLVE_CONTEXT;

typedef struct _PHP_TOKEN_GROUP_RESOLVE_CONTEXT
{
    HWND ListViewHandle;
    IListView* ListView;
    PPHP_TOKEN_PAGE_LISTVIEW_ITEM LvItem;
    PSID TokenGroupSid;
} PHP_TOKEN_GROUP_RESOLVE_CONTEXT, *PPHP_TOKEN_GROUP_RESOLVE_CONTEXT;

typedef struct _PH_TOKEN_ATTRIBUTE_NODE
{
    PH_TREENEW_NODE Node;
    PPH_LIST Children;
    PPH_STRING Text;
    PPH_STRING Value;
} PH_TOKEN_ATTRIBUTE_NODE, *PPH_TOKEN_ATTRIBUTE_NODE;

typedef struct _PH_TOKEN_ATTRIBUTE_TREE_CONTEXT
{
    HWND WindowHandle;
    PPH_LIST RootList;
    PPH_LIST NodeList;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
} PH_TOKEN_ATTRIBUTE_TREE_CONTEXT, *PPH_TOKEN_ATTRIBUTE_TREE_CONTEXT;

typedef struct _TOKEN_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PPH_CLOSE_OBJECT CloseObject;
    PVOID Context;
    DLGPROC HookProc;
    HANDLE ProcessId;

    HWND ListViewHandle;
    IListView* ListViewClass;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    BOOLEAN SinglePageContext;

    PTOKEN_GROUPS Groups;
    PTOKEN_GROUPS RestrictedSids;
    PTOKEN_PRIVILEGES Privileges;
    PTOKEN_GROUPS Capabilities;

    PH_TOKEN_ATTRIBUTE_TREE_CONTEXT CapsTreeContext;
    PH_TOKEN_ATTRIBUTE_TREE_CONTEXT ClaimsTreeContext;
    PH_TOKEN_ATTRIBUTE_TREE_CONTEXT AuthzTreeContext;
    PH_TOKEN_ATTRIBUTE_TREE_CONTEXT AppPolicyTreeContext;
} TOKEN_PAGE_CONTEXT, *PTOKEN_PAGE_CONTEXT;

static CONST PH_KEY_VALUE_PAIR PhElevationTypePairs[] =
{
    SIP(SREF(L"Unknown"), 0),
    SIP(SREF(L"No (Default)"), TokenElevationTypeDefault),
    SIP(SREF(L"No (Full)"), TokenElevationTypeFull),
    SIP(SREF(L"No (Limited)"), TokenElevationTypeLimited),
    SIP(SREF(L"Yes"), 4),
    SIP(SREF(L"Yes (Default)"), 4 + TokenElevationTypeDefault),
    SIP(SREF(L"Yes (Full)"), 4 + TokenElevationTypeFull),
    SIP(SREF(L"Yes (Limited)"), 4 + TokenElevationTypeLimited),
};

static CONST PH_KEY_VALUE_PAIR PhImpersonationLevelPairs[] =
{
    SIP(L"Anonymous", SecurityAnonymous),
    SIP(L"Identification", SecurityIdentification),
    SIP(L"Impersonation", SecurityImpersonation),
    SIP(L"Delegation", SecurityDelegation),
};

static CONST PH_KEY_VALUE_PAIR PhTokenTypePairs[] =
{
    SIP(L"Unknown", 0),
    SIP(L"Primary", TokenPrimary),
    SIP(L"Impersonation", TokenImpersonation),
};

static CONST PH_KEY_VALUE_PAIR PhSidTypePairs[] =
{
    SIP(L"Unknown", 0),
    SIP(L"User", SidTypeUser),
    SIP(L"Group", SidTypeGroup),
    SIP(L"Domain", SidTypeDomain),
    SIP(L"Alias", SidTypeAlias),
    SIP(L"WellKnownGroup", SidTypeWellKnownGroup),
    SIP(L"DeletedAccount", SidTypeDeletedAccount),
    SIP(L"Yes (Limited)", SidTypeInvalid),
    SIP(L"Unknown", SidTypeUnknown),
    SIP(L"Computer", SidTypeComputer),
    SIP(L"Label", SidTypeLabel),
    SIP(L"Logon session", SidTypeLogonSession),
};

PH_ACCESS_ENTRY CONST PhpGroupDescriptionEntries[6] =
{
    { NULL, SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED, FALSE, FALSE, L"Integrity" },
    { NULL, SE_GROUP_LOGON_ID, FALSE, FALSE, L"Logon Id" },
    { NULL, SE_GROUP_OWNER, FALSE, FALSE, L"Owner" },
    { NULL, SE_GROUP_MANDATORY, FALSE, FALSE, L"Mandatory" },
    { NULL, SE_GROUP_USE_FOR_DENY_ONLY, FALSE, FALSE, L"Use for deny only" },
    { NULL, SE_GROUP_RESOURCE, FALSE, FALSE, L"Resource" }
};

static PH_STRINGREF PhpEmptyTokenAttributesText = PH_STRINGREF_INIT(L"There are no attributes to display.");
static PH_STRINGREF PhpEmptyTokenClaimsText = PH_STRINGREF_INIT(L"There are no claims to display.");
static PH_STRINGREF PhpEmptyTokenCapabilitiesText = PH_STRINGREF_INIT(L"There are no capabilities to display.");

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
    _In_ PTOKEN_PAGE_CONTEXT Context,
    _In_ BOOLEAN ShowAppContainerPage
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
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
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

INT_PTR CALLBACK PhpTokenContainerPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpTokenAppPolicyPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

PPH_STRING PhpGetTokenFolderPath(
    _In_ HANDLE TokenHandle
    );
PPH_STRING PhpGetTokenRegistryPath(
    _In_ HANDLE TokenHandle
    );

NTSTATUS PhpTokenDialogThread(
    _In_ PVOID Context
    )
{
    PH_AUTO_POOL autoPool;
    LPPROPSHEETPAGE propSheetPage;

    PhInitializeAutoPool(&autoPool);

    propSheetPage = PhAllocateZero(sizeof(PROPSHEETPAGE));
    propSheetPage->lParam = (LPARAM)Context;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_OBJTOKEN),
        NULL,
        PhpTokenPageProc,
        propSheetPage
        );

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID PhCreateTokenDialog(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_ HANDLE ProcessId,
    _In_ PVOID Context,
    _In_opt_ DLGPROC HookProc
    )
{
    NTSTATUS status;
    PTOKEN_PAGE_CONTEXT tokenPageContext;
    HANDLE tokenHandle = NULL;

    if (!NT_SUCCESS(status = NtDuplicateObject(
        NtCurrentProcess(),
        Context,
        NtCurrentProcess(),
        &tokenHandle,
        0,
        0,
        DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES
        )))
    {
        PhShowStatus(NULL, L"Unable to duplicate the token.", status, 0);
        return;
    }

    tokenPageContext = PhCreateAlloc(sizeof(TOKEN_PAGE_CONTEXT));
    memset(tokenPageContext, 0, sizeof(TOKEN_PAGE_CONTEXT));
    tokenPageContext->OpenObject = OpenObject;
    tokenPageContext->CloseObject = CloseObject;
    tokenPageContext->Context = tokenHandle;
    //tokenPageContext->HookProc = HookProc;
    tokenPageContext->ProcessId = ProcessId;
    tokenPageContext->SinglePageContext = TRUE;

    PhCreateThread2(PhpTokenDialogThread, tokenPageContext);
}

VOID PhShowTokenProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_ HANDLE ProcessId,
    _In_ PVOID Context,
    _In_opt_ PCWSTR Title
    )
{
    PhCreateTokenDialog(OpenObject, CloseObject, ProcessId, Context, NULL);
}

INT CALLBACK PhpTokenSheetProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam
)
{
    switch (uMsg)
    {
    case PSCB_INITIALIZED:
        {
            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    }

    return 0;
}

HPROPSHEETPAGE PhCreateTokenPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_ PPH_CLOSE_OBJECT CloseObject,
    _In_ HANDLE ProcessId,
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
    tokenPageContext->CloseObject = CloseObject;
    tokenPageContext->Context = Context;
    tokenPageContext->HookProc = HookProc;
    tokenPageContext->ProcessId = ProcessId;

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

    if (FlagOn(Attributes, SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED))
    {
        if (FlagOn(Attributes, SE_GROUP_ENABLED))
            string = PhCreateString(L"Enabled (as a group)");
        else
            string = PhReferenceEmptyString();
    }
    else
    {
        if (FlagOn(Attributes, SE_GROUP_ENABLED))
        {
            if (FlagOn(Attributes, SE_GROUP_ENABLED_BY_DEFAULT))
                string = PhCreateString(L"Enabled");
            else
                string = PhCreateString(L"Enabled (modified)");
        }
        else
        {
            if (FlagOn(Attributes, SE_GROUP_ENABLED_BY_DEFAULT))
                string = PhCreateString(L"Disabled (modified)");
            else
                string = PhCreateString(L"Disabled");
        }
    }

    if (Restricted)
    {
        PhMoveReference(&string, PhConcatStringRefZ(&string->sr, L" (restricted)"));
    }

    return string;
}

COLORREF PhGetGroupAttributesColorDark(
    _In_ ULONG Attributes
    )
{
    if (FlagOn(Attributes, SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED))
    {
        if (!FlagOn(Attributes, SE_GROUP_ENABLED))
            return RGB(0, 26, 0);
    }

    if (FlagOn(Attributes, SE_GROUP_ENABLED))
    {
        if (FlagOn(Attributes, SE_GROUP_ENABLED_BY_DEFAULT))
            return RGB(0, 26, 0);
        else
            return RGB(0, 102, 0);
    }
    else
    {
        if (FlagOn(Attributes, SE_GROUP_ENABLED_BY_DEFAULT))
            return RGB(122, 77, 84);
        else
            return RGB(43, 12, 15);
    }
}

COLORREF PhGetPrivilegeAttributesColorDark(
    _In_ ULONG Attributes
    )
{
    if (FlagOn(Attributes, SE_PRIVILEGE_REMOVED))
    {
        return RGB(0, 0, 0);
    }

    if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED))
    {
        if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED_BY_DEFAULT))
            return RGB(0, 26, 0);
        else
            return RGB(0, 102, 0);
    }
    else
    {
        if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED_BY_DEFAULT))
            return RGB(122, 77, 84);
        else
            return RGB(43, 12, 15);
    }
}

COLORREF PhGetDangerousFlagColorDark(
    _In_ BOOLEAN FlagState
    )
{
    if (FlagState)
        return RGB(0xc0, 0xf0, 0xc0);
    else
        return RGB(0xf0, 0xc0, 0xc0);
}

COLORREF PhGetGroupAttributesColor(
    _In_ ULONG Attributes
    )
{
    if (FlagOn(Attributes, SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED))
    {
        if (!FlagOn(Attributes, SE_GROUP_ENABLED))
            return RGB(0xe0, 0xf0, 0xe0);
    }

    if (FlagOn(Attributes, SE_GROUP_ENABLED))
    {
        if (FlagOn(Attributes, SE_GROUP_ENABLED_BY_DEFAULT))
            return RGB(0xe0, 0xf0, 0xe0);
        else
            return RGB(0xc0, 0xf0, 0xc0);
    }
    else
    {
        if (FlagOn(Attributes, SE_GROUP_ENABLED_BY_DEFAULT))
            return RGB(0xf0, 0xc0, 0xc0);
        else
            return RGB(0xf0, 0xe0, 0xe0);
    }
}

COLORREF PhGetPrivilegeAttributesColor(
    _In_ ULONG Attributes
    )
{
    if (FlagOn(Attributes, SE_PRIVILEGE_REMOVED))
    {
        return RGB(0xc0, 0xc0, 0xc0);
    }

    if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED))
    {
        if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED_BY_DEFAULT))
            return RGB(0xe0, 0xf0, 0xe0);
        else
            return RGB(0xc0, 0xf0, 0xc0);
    }
    else
    {
        if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED_BY_DEFAULT))
            return RGB(0xf0, 0xc0, 0xc0);
        else
            return RGB(0xf0, 0xe0, 0xe0);
    }
}

COLORREF PhGetDangerousFlagColor(
    _In_ BOOLEAN FlagState
    )
{
    if (FlagState)
        return RGB(0xc0, 0xf0, 0xc0);
    else
        return RGB(0xf0, 0xc0, 0xc0);
}

static COLORREF NTAPI PhpTokenGroupColorFunction(
    _In_ LONG Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    )
{
    PPHP_TOKEN_PAGE_LISTVIEW_ITEM entry = Param;

    if (PhEnableThemeSupport)
    {
        if (entry->ItemCategory == PH_PROCESS_TOKEN_CATEGORY_DANGEROUS_FLAGS)
            return PhGetDangerousFlagColorDark(entry->ItemFlagState);
        else if (entry->ItemCategory == PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES)
            return PhGetPrivilegeAttributesColorDark(entry->TokenPrivilege->Attributes);
        else
            return PhGetGroupAttributesColorDark(entry->TokenGroup->Attributes);
    }
    else
    {
        if (entry->ItemCategory == PH_PROCESS_TOKEN_CATEGORY_DANGEROUS_FLAGS)
            return PhGetDangerousFlagColor(entry->ItemFlagState);
        else if (entry->ItemCategory == PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES)
            return PhGetPrivilegeAttributesColor(entry->TokenPrivilege->Attributes);
        else
            return PhGetGroupAttributesColor(entry->TokenGroup->Attributes);
    }
}

PCWSTR PhGetPrivilegeAttributesString(
    _In_ ULONG Attributes
    )
{
    if (FlagOn(Attributes, SE_PRIVILEGE_REMOVED))
    {
        return L"Removed";
    }

    if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED))
    {
        if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED_BY_DEFAULT))
            return L"Enabled";
        else
            return L"Enabled (modified)";
    }
    else
    {
        if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED_BY_DEFAULT))
            return L"Disabled (modified)";
        else
            return L"Disabled";
    }
}

_Success_(return)
BOOLEAN PhGetElevationTypeString(
    _In_ BOOLEAN IsElevated,
    _In_ TOKEN_ELEVATION_TYPE ElevationType,
    _Out_ PPH_STRINGREF* ElevationTypeString
    )
{
    PPH_STRINGREF string;

    if (PhFindStringRefSiKeyValuePairs(
        PhElevationTypePairs,
        sizeof(PhElevationTypePairs),
        (IsElevated ? 4 : 0) + (ULONG)ElevationType,
        &string
        ))
    {
        *ElevationTypeString = string;
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN PhGetImpersonationLevelString(
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _Out_ PWSTR* ImpersonationLevelString
    )
{
    if (PhIndexStringSiKeyValuePairs(
        PhImpersonationLevelPairs,
        sizeof(PhImpersonationLevelPairs),
        ImpersonationLevel,
        ImpersonationLevelString
        ))
    {
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN PhGetTokenTypeString(
    _In_ TOKEN_TYPE TokenType,
    _Out_ PWSTR* TokenTypeString
    )
{
    if (PhIndexStringSiKeyValuePairs(
        PhTokenTypePairs,
        sizeof(PhTokenTypePairs),
        TokenType,
        TokenTypeString
        ))
    {
        return TRUE;
    }

    return FALSE;
}

VOID PhpTokenPageFreeListViewEntries(
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext
    )
{
    LONG index = INT_ERROR;

    while ((index = PhFindIListViewItemByFlags(
        TokenPageContext->ListViewClass,
        index,
        LVNI_ALL
        )) != INT_ERROR)
    {
        PPHP_TOKEN_PAGE_LISTVIEW_ITEM entry;

        if (PhGetIListViewItemParam(TokenPageContext->ListViewClass, index, &entry))
        {
            PhFree(entry);
        }
    }
}

_Success_(return)
BOOLEAN PhGetTokenSidTypeString(
    _In_ SID_NAME_USE TokenNameUse,
    _Out_ PWSTR* TokenNameUseString
    )
{
    if (PhIndexStringSiKeyValuePairs(
        PhSidTypePairs,
        sizeof(PhSidTypePairs),
        TokenNameUse,
        TokenNameUseString
        ))
    {
        return TRUE;
    }

    return FALSE;
}

static NTSTATUS NTAPI PhpTokenGroupResolveWorker(
    _In_ PVOID ThreadParameter
    )
{
    PPHP_TOKEN_GROUP_RESOLVE_CONTEXT context = ThreadParameter;
    PPH_STRING sidString;
    SID_NAME_USE sidUse;
    LONG lvItemIndex;

    lvItemIndex = PhFindIListViewItemByParam(
        context->ListView,
        INT_ERROR,
        context->LvItem
        );

    if (lvItemIndex != INT_ERROR)
    {
        if (sidString = PhGetSidFullName(context->TokenGroupSid, TRUE, NULL))
        {
            PhMoveReference(&sidString, PhReferenceObject(sidString));
        }
        else if (sidString = PhGetAppContainerPackageName(context->TokenGroupSid))
        {
            PhMoveReference(&sidString, PhConcatStringRefZ(&sidString->sr, L" (APP_PACKAGE)"));
        }
        else if (sidString = PhGetAppContainerName(context->TokenGroupSid))
        {
            PhMoveReference(&sidString, PhConcatStringRefZ(&sidString->sr, L" (APP_CONTAINER)"));
        }
        else if (sidString = PhGetCapabilitySidName(context->TokenGroupSid))
        {
            PhMoveReference(&sidString, PhConcatStringRefZ(&sidString->sr, L" (APP_CAPABILITY)"));
        }
        else
        {
            if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(context->TokenGroupSid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_NT_AUTHORITY))
            {
                ULONG subAuthority = *PhSubAuthoritySid(context->TokenGroupSid, 0);

                switch (subAuthority)
                {
                case SECURITY_UMFD_BASE_RID:
                    PhMoveReference(&sidString, PhCreateString(L"Font Driver Host\\UMFD"));
                    break;
                }
            }
            else if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(context->TokenGroupSid), PhIdentifierAuthoritySid(PhSeCloudActiveDirectorySid())))
            {
                ULONG subAuthority = *PhSubAuthoritySid(context->TokenGroupSid, 0);

                if (subAuthority == 1)
                {
                    PhMoveReference(&sidString, PhGetAzureDirectoryObjectSid(context->TokenGroupSid));
                }
            }
        }

        if (sidString)
        {
            PhSetIListViewSubItem(context->ListView, lvItemIndex, PH_PROCESS_TOKEN_INDEX_NAME, PhGetString(sidString));
            PhDereferenceObject(sidString);
        }
        else
        {
            PhSetIListViewSubItem(context->ListView, lvItemIndex, PH_PROCESS_TOKEN_INDEX_NAME, L"[Unknown SID]");
        }

        PhSetIListViewSubItem(context->ListView, lvItemIndex, PH_PROCESS_TOKEN_INDEX_TYPE, PhGetSidAccountTypeString(context->TokenGroupSid));
    }

    if (NT_SUCCESS(PhLookupSid(context->TokenGroupSid, NULL, NULL, &sidUse)))
    {
        PWSTR tokenSidType;

        if (PhGetTokenSidTypeString(sidUse, &tokenSidType))
            PhSetIListViewSubItem(context->ListView, lvItemIndex, PH_PROCESS_TOKEN_INDEX_USE, tokenSidType);
        else
            PhSetIListViewSubItem(context->ListView, lvItemIndex, PH_PROCESS_TOKEN_INDEX_USE, L"N/A");
    }
    else
    {
        PhSetIListViewSubItem(context->ListView, lvItemIndex, PH_PROCESS_TOKEN_INDEX_USE, L"N/A");
    }

    PhFree(context->TokenGroupSid);
    PhFree(context);

    return STATUS_SUCCESS;
}

VOID PhpUpdateSidsFromTokenGroups(
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext,
    _In_ PTOKEN_GROUPS Groups,
    _In_ BOOLEAN Restricted
    )
{
    for (ULONG i = 0; i < Groups->GroupCount; i++)
    {
        LONG lvItemIndex;
        PPH_STRING stringUserSid;
        PPH_STRING attributesString;
        PPH_STRING descriptionString;
        PPHP_TOKEN_PAGE_LISTVIEW_ITEM lvitem;

        lvitem = PhAllocateZero(sizeof(PHP_TOKEN_PAGE_LISTVIEW_ITEM));
        lvitem->ItemCategory = Restricted ? PH_PROCESS_TOKEN_CATEGORY_RESTRICTED : PH_PROCESS_TOKEN_CATEGORY_GROUPS;
        lvitem->TokenGroup = &Groups->Groups[i];

        lvItemIndex = PhAddIListViewGroupItem(
            TokenPageContext->ListViewClass,
            lvitem->ItemCategory,
            MAXINT,
            L"Resolving...",
            lvitem
            );

        if (attributesString = PhGetGroupAttributesString(Groups->Groups[i].Attributes, Restricted))
        {
            PhSetIListViewSubItem(TokenPageContext->ListViewClass, lvItemIndex, PH_PROCESS_TOKEN_INDEX_STATUS, PhGetString(attributesString));
            PhDereferenceObject(attributesString);
        }

        descriptionString = PhGetAccessString(
            Groups->Groups[i].Attributes,
            (PPH_ACCESS_ENTRY)PhpGroupDescriptionEntries,
            RTL_NUMBER_OF(PhpGroupDescriptionEntries)
            );

        if (descriptionString)
        {
            PhSetIListViewSubItem(TokenPageContext->ListViewClass, lvItemIndex, PH_PROCESS_TOKEN_INDEX_DESCRIPTION, PhGetString(descriptionString));
            PhDereferenceObject(descriptionString);
        }

        if (stringUserSid = PhSidToStringSid(Groups->Groups[i].Sid))
        {
            PhSetIListViewSubItem(TokenPageContext->ListViewClass, lvItemIndex, PH_PROCESS_TOKEN_INDEX_SID, PhGetString(stringUserSid));
            PhDereferenceObject(stringUserSid);
        }

        {
            PPHP_TOKEN_GROUP_RESOLVE_CONTEXT tokenGroupResolve;

            tokenGroupResolve = PhAllocateZero(sizeof(PHP_TOKEN_GROUP_RESOLVE_CONTEXT));
            tokenGroupResolve->ListViewHandle = TokenPageContext->ListViewHandle;
            tokenGroupResolve->ListView = TokenPageContext->ListViewClass;
            tokenGroupResolve->LvItem = lvitem;
            tokenGroupResolve->TokenGroupSid = PhAllocateCopy(Groups->Groups[i].Sid, PhLengthSid(Groups->Groups[i].Sid));

            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), PhpTokenGroupResolveWorker, tokenGroupResolve);
        }
    }
}

BOOLEAN PhpUpdateTokenGroups(
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext,
    _In_ HANDLE TokenHandle
    )
{
    PTOKEN_GROUPS groups = NULL;
    PTOKEN_GROUPS restrictedSIDs = NULL;

    if (NT_SUCCESS(PhGetTokenGroups(TokenHandle, &groups)))
    {
        PhpUpdateSidsFromTokenGroups(TokenPageContext, groups, FALSE);
    }

    if (NT_SUCCESS(PhGetTokenRestrictedSids(TokenHandle, &restrictedSIDs)))
    {
        PhpUpdateSidsFromTokenGroups(TokenPageContext, restrictedSIDs, TRUE);
    }

    if (TokenPageContext->RestrictedSids)
        PhFree(TokenPageContext->RestrictedSids);
    if (TokenPageContext->Groups)
        PhFree(TokenPageContext->Groups);

    TokenPageContext->RestrictedSids = restrictedSIDs;
    TokenPageContext->Groups = groups;

    return TRUE;
}

NTSTATUS NTAPI PhpEnumeratePrivilegesCallback(
    _In_ PPOLICY_PRIVILEGE_DEFINITION Privileges,
    _In_ ULONG NumberOfPrivileges,
    _In_ PVOID Context
    )
{
    PTOKEN_PAGE_CONTEXT tokenPageContext = Context;
    LONG lvItemIndex;

    if (!tokenPageContext->Privileges)
        return STATUS_UNSUCCESSFUL;

    for (ULONG i = 0; i < NumberOfPrivileges; i++)
    {
        PPH_STRING privilegeName;
        PPH_STRING privilegeDisplayName;
        PPHP_TOKEN_PAGE_LISTVIEW_ITEM lvitem;

        for (ULONG j = 0; j < tokenPageContext->Privileges->PrivilegeCount; j++)
        {
            if (RtlIsEqualLuid(&tokenPageContext->Privileges->Privileges[j].Luid, &Privileges[i].LocalValue))
            {
                continue;
            }
        }

        privilegeName = PhCreateStringFromUnicodeString(&Privileges[i].Name);
        privilegeDisplayName = NULL;
        PhLookupPrivilegeDisplayName(&privilegeName->sr, &privilegeDisplayName);

        lvitem = PhAllocateZero(sizeof(PHP_TOKEN_PAGE_LISTVIEW_ITEM));
        lvitem->ItemCategory = PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES;
        lvitem->TokenPrivilege = PhAllocateZero(sizeof(LUID_AND_ATTRIBUTES));
        lvitem->TokenPrivilege->Luid = Privileges[i].LocalValue;
        lvitem->TokenPrivilege->Attributes = SE_PRIVILEGE_REMOVED;

        // Name
        lvItemIndex = PhAddIListViewGroupItem(tokenPageContext->ListViewClass, PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES, MAXINT, PhGetString(privilegeName), lvitem);
        // Status
        PhSetIListViewSubItem(tokenPageContext->ListViewClass, lvItemIndex, PH_PROCESS_TOKEN_INDEX_STATUS, PhGetPrivilegeAttributesString(lvitem->TokenPrivilege->Attributes));
        // Description
        PhSetIListViewSubItem(tokenPageContext->ListViewClass, lvItemIndex, PH_PROCESS_TOKEN_INDEX_DESCRIPTION, PhGetStringOrEmpty(privilegeDisplayName));
        // Privilege value
        PhSetIListViewSubItem(tokenPageContext->ListViewClass, lvItemIndex, PH_PROCESS_TOKEN_INDEX_SID, PhaFormatUInt64(Privileges[i].LocalValue.LowPart, FALSE)->Buffer);

        PhClearReference(&privilegeDisplayName);
        PhClearReference(&privilegeName);
    }

    return STATUS_SUCCESS;
}

BOOLEAN PhpUpdateTokenPrivileges(
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext,
    _In_ HANDLE TokenHandle
    )
{
    PTOKEN_PRIVILEGES privileges;
    ULONG i;

    if (!NT_SUCCESS(PhGetTokenPrivileges(TokenHandle, &privileges)))
        return FALSE;

    for (i = 0; i < privileges->PrivilegeCount; i++)
    {
        LONG lvItemIndex;
        PPH_STRING privilegeName;
        PPH_STRING privilegeDisplayName;

        if (PhLookupPrivilegeName(
            &privileges->Privileges[i].Luid,
            &privilegeName
            ))
        {
            PPHP_TOKEN_PAGE_LISTVIEW_ITEM lvitem;

            lvitem = PhAllocateZero(sizeof(PHP_TOKEN_PAGE_LISTVIEW_ITEM));
            lvitem->ItemCategory = PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES;
            lvitem->TokenPrivilege = &privileges->Privileges[i];

            privilegeDisplayName = NULL;
            PhLookupPrivilegeDisplayName(&privilegeName->sr, &privilegeDisplayName);

            // Name
            lvItemIndex = PhAddIListViewGroupItem(TokenPageContext->ListViewClass, PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES, MAXINT, privilegeName->Buffer, lvitem);
            // Status
            PhSetIListViewSubItem(TokenPageContext->ListViewClass, lvItemIndex, PH_PROCESS_TOKEN_INDEX_STATUS, PhGetPrivilegeAttributesString(privileges->Privileges[i].Attributes));
            // Description
            PhSetIListViewSubItem(TokenPageContext->ListViewClass, lvItemIndex, PH_PROCESS_TOKEN_INDEX_DESCRIPTION, PhGetStringOrEmpty(privilegeDisplayName));
            // Value
            PhSetIListViewSubItem(TokenPageContext->ListViewClass, lvItemIndex, PH_PROCESS_TOKEN_INDEX_SID, PhaFormatUInt64(privileges->Privileges[i].Luid.LowPart, FALSE)->Buffer);

            PhClearReference(&privilegeDisplayName);
            PhClearReference(&privilegeName);
        }
    }

    if (TokenPageContext->Privileges)
        PhFree(TokenPageContext->Privileges);

    TokenPageContext->Privileges = privileges;

    if (PhGetIntegerSetting(L"EnableTokenRemovedPrivileges"))
    {
        PhEnumeratePrivileges(PhpEnumeratePrivilegesCallback, TokenPageContext);
    }

    return TRUE;
}

VOID PhpUpdateTokenDangerousFlagItem(
    _In_ IListView* ListView,
    _In_ PH_PROCESS_TOKEN_FLAG Flag,
    _In_ BOOLEAN State,
    _In_ PWSTR Name,
    _In_ PWSTR Description
    )
{
    INT lvItemIndex;
    PPHP_TOKEN_PAGE_LISTVIEW_ITEM lvitem;

    lvitem = PhAllocateZero(sizeof(PHP_TOKEN_PAGE_LISTVIEW_ITEM));
    lvitem->ItemCategory = PH_PROCESS_TOKEN_CATEGORY_DANGEROUS_FLAGS;
    lvitem->ItemFlag = Flag;
    lvitem->ItemFlagState = State;

    // Name
    lvItemIndex = PhAddIListViewGroupItem(ListView, lvitem->ItemCategory, MAXINT, Name, lvitem);
    // Status
    PhSetIListViewSubItem(ListView, lvItemIndex, PH_PROCESS_TOKEN_INDEX_STATUS, State ? L"Enabled (modified)" : L"Disabled (modified)");
    // Description
    PhSetIListViewSubItem(ListView, lvItemIndex, PH_PROCESS_TOKEN_INDEX_DESCRIPTION, Description);
    // Value
    PhSetIListViewSubItem(ListView, lvItemIndex, PH_PROCESS_TOKEN_INDEX_SID, PhaFormatUInt64(Flag, FALSE)->Buffer);
}

BOOLEAN PhpUpdateTokenDangerousFlags(
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext,
    _In_ HANDLE TokenHandle
    )
{
    TOKEN_MANDATORY_POLICY mandatoryPolicy;
    BOOLEAN isSandboxInert;
    BOOLEAN isUIAccess;

    if (NT_SUCCESS(PhGetTokenMandatoryPolicy(TokenHandle, &mandatoryPolicy)))
    {
        // The disabled no-write-up policy is considered to be dangerous (diversenok)
        if ((mandatoryPolicy.Policy & TOKEN_MANDATORY_POLICY_NO_WRITE_UP) == 0)
        {
            PhpUpdateTokenDangerousFlagItem(
                TokenPageContext->ListViewClass,
                PH_PROCESS_TOKEN_FLAG_NO_WRITE_UP,
                FALSE,
                L"No-Write-Up Policy",
                L"Prevents the process from modifying objects with a higher integrity"
                );
        }
    }

    if (NT_SUCCESS(PhGetTokenIsSandBoxInert(TokenHandle, &isSandboxInert)))
    {
        // The presence of SandboxInert flag is considered dangerous (diversenok)
        if (isSandboxInert)
        {
            PhpUpdateTokenDangerousFlagItem(
                TokenPageContext->ListViewClass,
                PH_PROCESS_TOKEN_FLAG_SANDBOX_INERT,
                TRUE,
                L"Sandbox Inert",
                L"Ignore AppLocker rules and Software Restriction Policies"
                );
        }
    }

    if (NT_SUCCESS(PhGetTokenUIAccess(TokenHandle, &isUIAccess)))
    {
        // The presence of UIAccess flag is considered dangerous (diversenok)
        if (isUIAccess)
        {
            PhpUpdateTokenDangerousFlagItem(
                TokenPageContext->ListViewClass,
                PH_PROCESS_TOKEN_FLAG_UIACCESS,
                TRUE,
                L"UIAccess",
                L"Ignore User Interface Privilege Isolation"
                );
        }
    }

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

static NTSTATUS NTAPI PhpTokenUserResolveWorker(
    _In_ PVOID ThreadParameter
    )
{
    PPHP_TOKEN_USER_RESOLVE_CONTEXT context = ThreadParameter;
    PPH_STRING fullUserName;

    if (fullUserName = PhGetSidFullName(context->TokenUserSid, TRUE, NULL))
    {
        PhSetWindowText(context->WindowHandle, PhGetString(fullUserName));
        PhDereferenceObject(fullUserName);
    }
    else
    {
        if (fullUserName = PhGetAzureDirectoryObjectSid(context->TokenUserSid))
        {
            PhSetWindowText(context->WindowHandle, PhGetString(fullUserName));
            PhDereferenceObject(fullUserName);
        }
    }

    PhFree(context->TokenUserSid);
    PhFree(context);

    return STATUS_SUCCESS;
}

static VOID PhpTokenSetImageList(
    _In_ HWND WindowHandle,
    _Inout_ PTOKEN_PAGE_CONTEXT TokenPageContext
    )
{
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(WindowHandle);

    if (TokenPageContext->ListViewImageList)
    {
        PhImageListSetIconSize(
            TokenPageContext->ListViewImageList,
            2,
            PhGetDpi(20, dpiValue)
            );
    }
    else
    {
        TokenPageContext->ListViewImageList = PhImageListCreate(
            2,
            PhGetDpi(20, dpiValue),
            ILC_MASK | ILC_COLOR32,
            1, 1
            );
    }

    ListView_SetImageList(TokenPageContext->ListViewHandle, TokenPageContext->ListViewImageList, LVSIL_SMALL);
}

LONG PhpGetTokenPrivilegeSortingIndex(
    _In_ ULONG Attributes
    )
{
    if (FlagOn(Attributes, SE_PRIVILEGE_REMOVED))
        return 4;
    else
    {
        if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED))
        {
            if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED_BY_DEFAULT))
                return 0;
            else
                return 1;
        }
        else
        {
            if (FlagOn(Attributes, SE_PRIVILEGE_ENABLED_BY_DEFAULT))
                return 2;
            else
                return 3;
        }
    }
}

LONG PhpGetTokenGroupSortingIndex(
    _In_ ULONG Attributes
    )
{
    if (FlagOn(Attributes, SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED))
    {
        if (!FlagOn(Attributes, SE_GROUP_ENABLED))
            return 0;
    }

    if (FlagOn(Attributes, SE_GROUP_ENABLED))
    {
        if (FlagOn(Attributes, SE_GROUP_ENABLED_BY_DEFAULT))
            return 1;
        else
            return 2;
    }
    else
    {
        if (FlagOn(Attributes, SE_GROUP_ENABLED_BY_DEFAULT))
            return 3;
        else
            return 4;
    }
}

LONG NTAPI PhpTokenStatusColumnCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PPHP_TOKEN_PAGE_LISTVIEW_ITEM item1 = Item1;
    PPHP_TOKEN_PAGE_LISTVIEW_ITEM item2 = Item2;
    ULONG value1;
    ULONG value2;

    if (item1->ItemCategory == PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES)
        value1 = PhpGetTokenPrivilegeSortingIndex(item1->TokenPrivilege->Attributes);
    else if (item1->ItemCategory == PH_PROCESS_TOKEN_CATEGORY_GROUPS)
        value1 = PhpGetTokenGroupSortingIndex(item1->TokenGroup->Attributes);
    else
        value1 = 0;

    if (item2->ItemCategory == PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES)
        value2 = PhpGetTokenPrivilegeSortingIndex(item2->TokenPrivilege->Attributes);
    else if (item2->ItemCategory == PH_PROCESS_TOKEN_CATEGORY_GROUPS)
        value2 = PhpGetTokenGroupSortingIndex(item2->TokenGroup->Attributes);
    else
        value2 = 0;

    return uintcmp(value1, value2);
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
            tokenPageContext->ListViewClass = PhGetListViewInterface(tokenPageContext->ListViewHandle);

            PhSetListViewStyle(tokenPageContext->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(tokenPageContext->ListViewHandle, L"explorer");
            PhAddListViewColumn(tokenPageContext->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Name");
            PhAddListViewColumn(tokenPageContext->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Status");
            PhAddListViewColumn(tokenPageContext->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 170, L"Description");
            PhAddListViewColumn(tokenPageContext->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"SID");
            PhAddListViewColumn(tokenPageContext->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(tokenPageContext->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 100, L"Use");

            PhSetExtendedListView(tokenPageContext->ListViewHandle);
            ExtendedListView_SetCompareFunction(tokenPageContext->ListViewHandle, 1, PhpTokenStatusColumnCompareFunction);
            ExtendedListView_SetItemColorFunction(tokenPageContext->ListViewHandle, PhpTokenGroupColorFunction);
            ListView_EnableGroupView(tokenPageContext->ListViewHandle, TRUE);
            PhAddListViewGroup(tokenPageContext->ListViewHandle, PH_PROCESS_TOKEN_CATEGORY_DANGEROUS_FLAGS, L"Dangerous Flags");
            PhAddListViewGroup(tokenPageContext->ListViewHandle, PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES, L"Privileges");
            PhAddListViewGroup(tokenPageContext->ListViewHandle, PH_PROCESS_TOKEN_CATEGORY_GROUPS, L"Groups");
            PhAddListViewGroup(tokenPageContext->ListViewHandle, PH_PROCESS_TOKEN_CATEGORY_RESTRICTED, L"Restricting SIDs");
            PhLoadListViewColumnsFromSetting(L"TokenGroupsListViewColumns", tokenPageContext->ListViewHandle);
            PhLoadListViewGroupStatesFromSetting(L"TokenGroupsListViewStates", tokenPageContext->ListViewHandle);
            PhLoadListViewSortColumnsFromSetting(L"TokenGroupsListViewSort", tokenPageContext->ListViewHandle);
            PhpTokenSetImageList(hwndDlg, tokenPageContext);

            PhSetDialogItemText(hwndDlg, IDC_USER, L"Unknown");
            PhSetDialogItemText(hwndDlg, IDC_USERSID, L"Unknown");

            if (NT_SUCCESS(tokenPageContext->OpenObject(
                &tokenHandle,
                TOKEN_QUERY,
                tokenPageContext->Context // ProcessId
                )))
            {
                PH_TOKEN_USER tokenUser;
                PPH_STRING stringUserSid;
                ULONG tokenSessionId = ULONG_MAX;
                BOOLEAN tokenElevation = FALSE;
                TOKEN_ELEVATION_TYPE tokenElevationType = 0;
                PPH_STRINGREF tokenElevationTypeString;
                BOOLEAN isVirtualizationAllowed;
                BOOLEAN isVirtualizationEnabled;
                PH_TOKEN_APPCONTAINER tokenAppContainer;
                PPH_STRING appContainerName;
                PPH_STRING appContainerSidString;

                if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
                {
                    BOOLEAN tokenIsAppContainer = FALSE;

                    PhGetTokenIsAppContainer(tokenHandle, &tokenIsAppContainer);

                    if (!tokenIsAppContainer) // HACK (dmex)
                    {
                        PPHP_TOKEN_USER_RESOLVE_CONTEXT tokenUserResolve;

                        PhSetDialogItemText(hwndDlg, IDC_USER, L"Resolving...");

                        tokenUserResolve = PhAllocateZero(sizeof(PHP_TOKEN_USER_RESOLVE_CONTEXT));
                        tokenUserResolve->WindowHandle = GetDlgItem(hwndDlg, IDC_USER);
                        tokenUserResolve->TokenUserSid = PhAllocateCopy(tokenUser.User.Sid, PhLengthSid(tokenUser.User.Sid));

                        PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), PhpTokenUserResolveWorker, tokenUserResolve);
                    }

                    if (stringUserSid = PhSidToStringSid(tokenUser.User.Sid))
                    {
                        PhSetDialogItemText(hwndDlg, IDC_USERSID, stringUserSid->Buffer);
                        PhDereferenceObject(stringUserSid);
                    }
                }

                PhGetTokenSessionId(tokenHandle, &tokenSessionId);
                PhGetTokenElevation(tokenHandle, &tokenElevation);
                PhGetTokenElevationType(tokenHandle, &tokenElevationType);

                if (tokenSessionId != ULONG_MAX)
                    PhSetDialogItemValue(hwndDlg, IDC_SESSIONID, tokenSessionId, FALSE);
                else
                    PhSetDialogItemText(hwndDlg, IDC_SESSIONID, L"Unknown");

                if (PhGetElevationTypeString(tokenElevation, tokenElevationType, &tokenElevationTypeString))
                    PhSetDialogItemText(hwndDlg, IDC_ELEVATED, PhGetStringRefZ(tokenElevationTypeString));
                else
                    PhSetDialogItemText(hwndDlg, IDC_ELEVATED, L"Unknown");

                if (NT_SUCCESS(PhGetTokenIsVirtualizationAllowed(tokenHandle, &isVirtualizationAllowed)))
                {
                    if (isVirtualizationAllowed)
                    {
                        if (NT_SUCCESS(PhGetTokenIsVirtualizationEnabled(tokenHandle, &isVirtualizationEnabled)))
                        {
                            PhSetDialogItemText(hwndDlg, IDC_VIRTUALIZED, isVirtualizationEnabled ? L"Yes" : L"No");
                        }
                    }
                    else
                    {
                        PhSetDialogItemText(hwndDlg, IDC_VIRTUALIZED, L"Not allowed");
                    }
                }

                if (WindowsVersion >= WINDOWS_8)
                {
                    appContainerName = NULL;
                    appContainerSidString = NULL;

                    if (NT_SUCCESS(PhGetTokenAppContainerSid(tokenHandle, &tokenAppContainer)))
                    {
                        appContainerName = PhGetAppContainerName(tokenAppContainer.AppContainer.Sid);
                        appContainerSidString = PhSidToStringSid(tokenAppContainer.AppContainer.Sid);
                    }

                    if (appContainerName)
                    {
                        PPH_STRING packageFamilyName;

                        packageFamilyName = PhConcatStrings2(appContainerName->Buffer, L" (APP_CONTAINER)");
                        PhSetDialogItemText(hwndDlg, IDC_USER, PhGetString(packageFamilyName));

                        PhDereferenceObject(packageFamilyName);
                        PhDereferenceObject(appContainerName);
                    }

                    if (appContainerSidString)
                    {
                        PhSetDialogItemText(hwndDlg, IDC_USERSID, PhGetString(appContainerSidString));
                        PhDereferenceObject(appContainerSidString);
                    }
                }

                {
                    ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, FALSE);
                    PhpTokenPageFreeListViewEntries(tokenPageContext);
                    ListView_DeleteAllItems(tokenPageContext->ListViewHandle);

                    PhpUpdateTokenDangerousFlags(tokenPageContext, tokenHandle);
                    PhpUpdateTokenGroups(tokenPageContext, tokenHandle);
                    PhpUpdateTokenPrivileges(tokenPageContext, tokenHandle);

                    ExtendedListView_SortItems(tokenPageContext->ListViewHandle);
                    ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, TRUE);
                }

                tokenPageContext->CloseObject(tokenHandle, FALSE, tokenPageContext->Context);
            }

            if (tokenPageContext->SinglePageContext)
            {
                LPPROPSHEETPAGE page = (LPPROPSHEETPAGE)lParam;
                if (page) PhFree(page);

                PhSetApplicationWindowIcon(hwndDlg);
                PhSetWindowText(hwndDlg, L"Linked Token");

                PhInitializeLayoutManager(&tokenPageContext->LayoutManager, hwndDlg);
                PhAddLayoutItem(&tokenPageContext->LayoutManager, tokenPageContext->ListViewHandle, NULL, PH_ANCHOR_ALL);
                PhAddLayoutItem(&tokenPageContext->LayoutManager, GetDlgItem(hwndDlg, IDC_DEFAULTPERM), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhAddLayoutItem(&tokenPageContext->LayoutManager, GetDlgItem(hwndDlg, IDC_PERMISSIONS), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhAddLayoutItem(&tokenPageContext->LayoutManager, GetDlgItem(hwndDlg, IDC_INTEGRITY), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhAddLayoutItem(&tokenPageContext->LayoutManager, GetDlgItem(hwndDlg, IDC_ADVANCED), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

                if (PhGetIntegerPairSetting(L"TokenWindowPosition").X)
                    PhLoadWindowPlacementFromSetting(L"TokenWindowPosition", L"TokenWindowSize", hwndDlg);
                else
                    PhCenterWindow(hwndDlg, NULL);

                PhSetDialogFocus(hwndDlg, tokenPageContext->ListViewHandle);
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewSortColumnsToSetting(L"TokenGroupsListViewSort", tokenPageContext->ListViewHandle);
            PhSaveListViewGroupStatesToSetting(L"TokenGroupsListViewStates", tokenPageContext->ListViewHandle);
            PhSaveListViewColumnsToSetting(L"TokenGroupsListViewColumns", tokenPageContext->ListViewHandle);

            if (tokenPageContext->SinglePageContext)
            {
                PhSaveWindowPlacementToSetting(L"TokenWindowPosition", L"TokenWindowSize", hwndDlg);
                PhDeleteLayoutManager(&tokenPageContext->LayoutManager);

                if (tokenPageContext->Context)
                {
                    NtClose(tokenPageContext->Context);
                    tokenPageContext->Context = NULL;
                }
            }

            PhpTokenPageFreeListViewEntries(tokenPageContext);

            if (tokenPageContext->ListViewImageList) PhImageListDestroy(tokenPageContext->ListViewImageList);
            if (tokenPageContext->Groups) PhFree(tokenPageContext->Groups);
            if (tokenPageContext->RestrictedSids) PhFree(tokenPageContext->RestrictedSids);
            if (tokenPageContext->Privileges) PhFree(tokenPageContext->Privileges);
            if (tokenPageContext->ListViewClass) IListView_Release(tokenPageContext->ListViewClass);
        }
        break;
    case WM_SIZE:
        {
            if (tokenPageContext->SinglePageContext)
            {
                PhLayoutManagerLayout(&tokenPageContext->LayoutManager);
            }
        }
        break;
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:
        {
            PhpTokenSetImageList(hwndDlg, tokenPageContext);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    if (tokenPageContext->SinglePageContext)
                    {
                        EndDialog(hwndDlg, IDOK);
                    }
                }
                break;
            case ID_PRIVILEGE_ENABLE:
            case ID_PRIVILEGE_DISABLE:
            case ID_PRIVILEGE_RESET:
            case ID_PRIVILEGE_REMOVE:
                {
                    NTSTATUS status;
                    BOOLEAN listViewGroupItemsValid = TRUE;
                    PPHP_TOKEN_PAGE_LISTVIEW_ITEM *listViewItems;
                    ULONG numberOfItems;
                    HANDLE tokenHandle;
                    ULONG i;

                    PhGetSelectedIListViewItemParams(
                        tokenPageContext->ListViewClass,
                        &listViewItems,
                        &numberOfItems
                        );

                    for (i = 0; i < numberOfItems; i++)
                    {
                        if (listViewItems[i]->ItemCategory != PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES)
                        {
                            listViewGroupItemsValid = FALSE;
                            break;
                        }
                    }

                    if (!listViewGroupItemsValid)
                    {
                        PhFree(listViewItems);
                        break;
                    }

                    if (GET_WM_COMMAND_ID(wParam, lParam) == ID_PRIVILEGE_REMOVE)
                    {
                        if (!PhShowConfirmMessage(
                            hwndDlg,
                            L"remove",
                            L"the selected privilege(s)",
                            L"Removing privileges may reduce the functionality of the process, "
                            L"and is permanent for the lifetime of the process.",
                            FALSE
                            ))
                        {
                            PhFree(listViewItems);
                            break;
                        }
                    }

                    status = tokenPageContext->OpenObject(
                        &tokenHandle,
                        TOKEN_ADJUST_PRIVILEGES,
                        tokenPageContext->Context // ProcessId
                        );

                    if (NT_SUCCESS(status))
                    {
                        ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, FALSE);

                        for (i = 0; i < numberOfItems; i++)
                        {
                            PPH_STRING privilegeName = NULL;
                            ULONG newAttributes = listViewItems[i]->TokenPrivilege->Attributes;

                            if (PhLookupPrivilegeName(&listViewItems[i]->TokenPrivilege->Luid, &privilegeName))
                            {
                                PH_AUTO(privilegeName);
                            }

                            switch (GET_WM_COMMAND_ID(wParam, lParam))
                            {
                            case ID_PRIVILEGE_ENABLE:
                                {
                                    SetFlag(newAttributes, SE_PRIVILEGE_ENABLED);
                                }
                                break;
                            case ID_PRIVILEGE_DISABLE:
                                {
                                    ClearFlag(newAttributes, SE_PRIVILEGE_ENABLED);
                                }
                                break;
                            case ID_PRIVILEGE_RESET:
                                {
                                    if (FlagOn(newAttributes, SE_PRIVILEGE_ENABLED_BY_DEFAULT))
                                        SetFlag(newAttributes, SE_PRIVILEGE_ENABLED);
                                    else
                                        ClearFlag(newAttributes, SE_PRIVILEGE_ENABLED);
                                }
                                break;
                            case ID_PRIVILEGE_REMOVE:
                                {
                                    newAttributes = SE_PRIVILEGE_REMOVED;
                                }
                                break;
                            }

                            if (PhSetTokenPrivilege(
                                tokenHandle,
                                NULL,
                                &listViewItems[i]->TokenPrivilege->Luid,
                                newAttributes
                                ))
                            {
                                LONG lvItemIndex = PhFindIListViewItemByParam(
                                    tokenPageContext->ListViewClass,
                                    INT_ERROR,
                                    listViewItems[i]
                                    );

                                if (lvItemIndex != INT_ERROR)
                                {
                                    if (GET_WM_COMMAND_ID(wParam, lParam) != ID_PRIVILEGE_REMOVE)
                                    {
                                        // Refresh the status text (and background color).
                                        listViewItems[i]->TokenPrivilege->Attributes = newAttributes;
                                        PhSetIListViewSubItem(
                                            tokenPageContext->ListViewClass,
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
                            }
                            else
                            {
                                PWSTR action = L"set";

                                switch (GET_WM_COMMAND_ID(wParam, lParam))
                                {
                                case ID_PRIVILEGE_ENABLE:
                                    action = L"enable";
                                    break;
                                case ID_PRIVILEGE_DISABLE:
                                    action = L"disable";
                                    break;
                                case ID_PRIVILEGE_RESET:
                                    action = L"reset";
                                    break;
                                case ID_PRIVILEGE_REMOVE:
                                    action = L"remove";
                                    break;
                                }

                                if (!PhShowContinueStatus(
                                    hwndDlg,
                                    PhaFormatString(L"Unable to %s %s.", action, PhGetStringOrDefault(privilegeName, L"privilege"))->Buffer,
                                    STATUS_UNSUCCESSFUL,
                                    0
                                    ))
                                    break;
                            }
                        }

                        ExtendedListView_SortItems(tokenPageContext->ListViewHandle);
                        ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, TRUE);

                        tokenPageContext->CloseObject(tokenHandle, FALSE, tokenPageContext->Context);
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to open the token.", status, 0);
                    }

                    PhFree(listViewItems);
                }
                break;
            case ID_GROUP_ENABLE:
            case ID_GROUP_DISABLE:
            case ID_GROUP_RESET:
                {
                    NTSTATUS status;
                    BOOLEAN listViewGroupItemsValid = TRUE;
                    PPHP_TOKEN_PAGE_LISTVIEW_ITEM *listViewItems;
                    ULONG numberOfItems;
                    HANDLE tokenHandle;
                    ULONG i;

                    PhGetSelectedIListViewItemParams(
                        tokenPageContext->ListViewClass,
                        &listViewItems,
                        &numberOfItems
                        );

                    for (i = 0; i < numberOfItems; i++)
                    {
                        if (listViewItems[i]->ItemCategory != PH_PROCESS_TOKEN_CATEGORY_GROUPS)
                        {
                            listViewGroupItemsValid = FALSE;
                            break;
                        }
                    }

                    if (!listViewGroupItemsValid)
                    {
                        PhFree(listViewItems);
                        break;
                    }

                    status = tokenPageContext->OpenObject(
                        &tokenHandle,
                        TOKEN_ADJUST_GROUPS,
                        tokenPageContext->Context // ProcessId
                        );

                    if (NT_SUCCESS(status))
                    {
                        ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, FALSE);

                        for (i = 0; i < numberOfItems; i++)
                        {
                            ULONG newAttributes = listViewItems[i]->TokenGroup->Attributes;

                            switch (GET_WM_COMMAND_ID(wParam, lParam))
                            {
                            case ID_GROUP_ENABLE:
                                newAttributes |= SE_GROUP_ENABLED;
                                break;
                            case ID_GROUP_DISABLE:
                                newAttributes &= ~SE_GROUP_ENABLED;
                                break;
                            case ID_GROUP_RESET:
                                {
                                    if (newAttributes & SE_GROUP_ENABLED_BY_DEFAULT)
                                        newAttributes |= SE_GROUP_ENABLED;
                                    else
                                        newAttributes &= ~SE_GROUP_ENABLED;
                                }
                                break;
                            }

                            if (NT_SUCCESS(status = PhSetTokenGroups(
                                tokenHandle,
                                NULL,
                                listViewItems[i]->TokenGroup->Sid,
                                newAttributes
                                )))
                            {
                                PPH_STRING attributesString;
                                LONG lvItemIndex;

                                attributesString = PhGetGroupAttributesString(newAttributes, FALSE);
                                lvItemIndex = PhFindIListViewItemByParam(
                                    tokenPageContext->ListViewClass,
                                    INT_ERROR,
                                    listViewItems[i]
                                    );

                                if (lvItemIndex != INT_ERROR)
                                {
                                    // Refresh the status text (and background color).
                                    listViewItems[i]->TokenGroup->Attributes = newAttributes;
                                    PhSetIListViewSubItem(
                                        tokenPageContext->ListViewClass,
                                        lvItemIndex,
                                        PH_PROCESS_TOKEN_INDEX_STATUS,
                                        attributesString->Buffer
                                        );
                                }

                                PhDereferenceObject(attributesString);
                            }
                            else
                            {
                                PWSTR action = L"set";

                                switch (GET_WM_COMMAND_ID(wParam, lParam))
                                {
                                case ID_GROUP_ENABLE:
                                    action = L"enable";
                                    break;
                                case ID_GROUP_DISABLE:
                                    action = L"disable";
                                    break;
                                case ID_GROUP_RESET:
                                    action = L"reset";
                                    break;
                                }

                                if (!PhShowContinueStatus(
                                    hwndDlg,
                                    PhaFormatString(L"Unable to %s %s.", action, L"group")->Buffer,
                                    status,
                                    0
                                    ))
                                    break;
                            }
                        }

                        ExtendedListView_SortItems(tokenPageContext->ListViewHandle);
                        ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, TRUE);

                        tokenPageContext->CloseObject(
                            tokenHandle,
                            FALSE,
                            tokenPageContext->Context
                            );
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to open the token.", status, 0);
                    }

                    PhFree(listViewItems);
                }
                break;
            case ID_UIACCESS_REMOVE:
                {
                    NTSTATUS status;
                    PPHP_TOKEN_PAGE_LISTVIEW_ITEM *listViewItems;
                    ULONG numberOfItems;
                    HANDLE tokenHandle;

                    PhGetSelectedListViewItemParams(
                        tokenPageContext->ListViewHandle,
                        &listViewItems,
                        &numberOfItems
                        );

                    if (numberOfItems != 1)
                    {
                        PhFree(listViewItems);
                        break;
                    }

                    if ((listViewItems[0]->ItemCategory != PH_PROCESS_TOKEN_CATEGORY_DANGEROUS_FLAGS) ||
                        (listViewItems[0]->ItemFlag != PH_PROCESS_TOKEN_FLAG_UIACCESS))
                    {
                        PhFree(listViewItems);
                        break;
                    }

                    if (!PhShowConfirmMessage(
                        hwndDlg,
                        L"remove",
                        L"the UIAccess flag",
                        L"Removing this flag may reduce the functionality of the process "
                        L"provided it is an accessibility application.",
                        FALSE
                        ))
                    {
                        PhFree(listViewItems);
                        break;
                    }

                    status = tokenPageContext->OpenObject(
                        &tokenHandle,
                        TOKEN_ADJUST_DEFAULT,
                        tokenPageContext->Context // ProcessId
                        );

                    if (NT_SUCCESS(status))
                    {
                        ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, FALSE);

                        status = PhSetTokenUIAccess(tokenHandle, FALSE);

                        if (NT_SUCCESS(status))
                        {
                            LONG lvItemIndex = PhFindIListViewItemByParam(
                                tokenPageContext->ListViewClass,
                                INT_ERROR,
                                listViewItems[0]
                                );

                            if (lvItemIndex != INT_ERROR)
                            {
                                ListView_DeleteItem(tokenPageContext->ListViewHandle, lvItemIndex);
                            }
                        }
                        else
                        {
                            PhShowStatus(hwndDlg, L"Unable to disable UIAccess flag.", status, 0);
                        }

                        ExtendedListView_SortItems(tokenPageContext->ListViewHandle);
                        ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, TRUE);

                        tokenPageContext->CloseObject(
                            tokenHandle,
                            FALSE,
                            tokenPageContext->Context
                            );
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to open the token.", status, 0);
                    }

                    PhFree(listViewItems);
                }
                break;
            case IDC_DEFAULTPERM:
                {
                    PhEditSecurity(
                        PhCsForceNoParent ? NULL : hwndDlg,
                        L"Default Token",
                        L"TokenDefault",
                        tokenPageContext->OpenObject,
                        tokenPageContext->CloseObject,
                        tokenPageContext->Context // ProcessId
                        );
                }
                break;
            case IDC_PERMISSIONS:
                {
                    PhEditSecurity(
                        PhCsForceNoParent ? NULL : hwndDlg,
                        L"Token",
                        L"Token",
                        tokenPageContext->OpenObject,
                        tokenPageContext->CloseObject,
                        tokenPageContext->Context // ProcessId
                        );
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
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatorySecureProcessRID, L"Protected", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatorySystemRID, L"System", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatoryHighRID, L"High", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatoryMediumPlusRID, L"Medium +", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatoryMediumRID, L"Medium", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatoryLowRID, L"Low", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MandatoryUntrustedRID, L"Untrusted", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"Custom...", NULL, NULL), ULONG_MAX);

                    integrityLevelRID = ULONG_MAX;

                    // Put a radio check on the menu item that corresponds with the current integrity level.
                    // Also disable menu items which correspond to higher integrity levels since
                    // NtSetInformationToken doesn't allow integrity levels to be raised.
                    if (NT_SUCCESS(status = tokenPageContext->OpenObject(
                        &tokenHandle,
                        TOKEN_QUERY,
                        tokenPageContext->Context // ProcessId
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
                                else if (menuItem->Id > (ULONG)integrityLevelRID && menuItem->Id != USHRT_MAX)
                                {
                                    PhSetDisabledEMenuItem(menuItem);
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

                        tokenPageContext->CloseObject(
                            tokenHandle,
                            FALSE,
                            tokenPageContext->Context
                            );
                    }

                    if (!NT_SUCCESS(status))
                    {
                        for (ULONG i = 0; i < menu->Items->Count; i++)
                        {
                            PhSetDisabledEMenuItem(menu->Items->Items[i]);
                        }
                    }

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_BOTTOM,
                        rect.left,
                        rect.top
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
                            ULONG integrityLevel = ULONG_MAX;

                            if (selectedItem->Id == USHRT_MAX)
                            {
                                PPH_STRING selectedChoice = NULL;
                                ULONG64 integer = 0;

                                while (PhaChoiceDialog(
                                    hwndDlg,
                                    L"Integrity Level",
                                    L"Enter a custom integrity level:",
                                    NULL,
                                    0,
                                    NULL,
                                    PH_CHOICE_DIALOG_USER_CHOICE,
                                    &selectedChoice,
                                    NULL,
                                    NULL
                                    ))
                                {
                                    if (PhStringToInteger64(&selectedChoice->sr, 0, &integer))
                                    {
                                        if ((ULONG)integer < (ULONG)integrityLevelRID)
                                        {
                                            integrityLevel = (ULONG)integer;
                                            break;
                                        }
                                    }
                                }

                                if (integrityLevel == ULONG_MAX)
                                    goto CleanupExit;
                            }
                            else
                            {
                                integrityLevel = selectedItem->Id;
                            }

                            if (NT_SUCCESS(status = tokenPageContext->OpenObject(
                                &tokenHandle,
                                TOKEN_QUERY | TOKEN_ADJUST_DEFAULT,
                                tokenPageContext->Context // ProcessId
                                )))
                            {
                                static SID_IDENTIFIER_AUTHORITY mandatoryLabelAuthority = SECURITY_MANDATORY_LABEL_AUTHORITY;
                                UCHAR newSidBuffer[FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG)];
                                PSID newSid;
                                TOKEN_MANDATORY_LABEL mandatoryLabel;

                                newSid = (PSID)newSidBuffer;
                                PhInitializeSid(newSid, &mandatoryLabelAuthority, 1);
                                *PhSubAuthoritySid(newSid, 0) = integrityLevel;
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
                                    PhpUpdateTokenGroups(tokenPageContext, tokenHandle);
                                    PhpUpdateTokenPrivileges(tokenPageContext, tokenHandle);
                                    ExtendedListView_SortItems(tokenPageContext->ListViewHandle);
                                    ExtendedListView_SetRedraw(tokenPageContext->ListViewHandle, TRUE);
                                }

                                tokenPageContext->CloseObject(
                                    tokenHandle,
                                    FALSE,
                                    tokenPageContext->Context
                                    );
                            }

                            if (!NT_SUCCESS(status))
                                PhShowStatus(hwndDlg, L"Unable to set the integrity level", status, 0);
                        }
                    }

                CleanupExit:
                    PhDestroyEMenu(menu);
                }
                break;
            case IDC_ADVANCED:
                {
                    HANDLE tokenHandle;
                    BOOLEAN tokenIsAppContainer = FALSE;

                    if (NT_SUCCESS(tokenPageContext->OpenObject(
                        &tokenHandle,
                        TOKEN_QUERY,
                        tokenPageContext->Context // ProcessId
                        )))
                    {
                        PhGetTokenIsAppContainer(tokenHandle, &tokenIsAppContainer);

                        if (!tokenIsAppContainer)
                        {
                            PPH_STRING packageName = PhGetTokenPackageFullName(tokenHandle);

                            tokenIsAppContainer = !PhIsNullOrEmptyString(packageName);

                            PhClearReference(&packageName);
                        }

                        tokenPageContext->CloseObject(
                            tokenHandle,
                            FALSE,
                            tokenPageContext->Context
                            );
                    }

                    PhpShowTokenAdvancedProperties(hwndDlg, tokenPageContext, tokenIsAppContainer);
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

            REFLECT_MESSAGE_DLG(hwndDlg, tokenPageContext->ListViewHandle, uMsg, wParam, lParam);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == tokenPageContext->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PPHP_TOKEN_PAGE_LISTVIEW_ITEM *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(tokenPageContext->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(tokenPageContext->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    BOOLEAN hasMixedCategories = FALSE;
                    BOOLEAN hasRemovedItems = FALSE;
                    PH_PROCESS_TOKEN_CATEGORY currentCategory = listviewItems[0]->ItemCategory;
                    ULONG i;

                    for (i = 0; i < numberOfItems; i++)
                    {
                        if (listviewItems[i]->ItemCategory != currentCategory)
                        {
                            hasMixedCategories = TRUE;
                            break;
                        }
                    }

                    if (currentCategory == PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES)
                    {
                        for (i = 0; i < numberOfItems; i++)
                        {
                            if (listviewItems[i]->TokenPrivilege &&
                                FlagOn(listviewItems[i]->TokenPrivilege->Attributes, SE_PRIVILEGE_REMOVED))
                            {
                                hasRemovedItems = TRUE;
                                break;
                            }
                        }
                    }

                    menu = PhCreateEMenu();

                    if (!hasMixedCategories)
                    {
                        switch (currentCategory)
                        {
                        case PH_PROCESS_TOKEN_CATEGORY_PRIVILEGES:
                            {
                                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PRIVILEGE_ENABLE, L"&Enable", NULL, NULL), ULONG_MAX);
                                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PRIVILEGE_DISABLE, L"&Disable", NULL, NULL), ULONG_MAX);
                                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PRIVILEGE_RESET, L"Re&set", NULL, NULL), ULONG_MAX);
                                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PRIVILEGE_REMOVE, L"&Remove", NULL, NULL), ULONG_MAX);
                                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);

                                if (hasRemovedItems)
                                {
                                    PhSetFlagsAllEMenuItems(menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                                }
                            }
                            break;
                        case PH_PROCESS_TOKEN_CATEGORY_GROUPS:
                            {
                                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_GROUP_ENABLE, L"&Enable", NULL, NULL), ULONG_MAX);
                                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_GROUP_DISABLE, L"&Disable", NULL, NULL), ULONG_MAX);
                                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_GROUP_RESET, L"Re&set", NULL, NULL), ULONG_MAX);
                                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                            }
                            break;
                        case PH_PROCESS_TOKEN_CATEGORY_DANGEROUS_FLAGS:
                            {
                                if ((numberOfItems == 1) && (listviewItems[0]->ItemFlag == PH_PROCESS_TOKEN_FLAG_UIACCESS))
                                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_UIACCESS_REMOVE, L"&Remove", NULL, NULL), ULONG_MAX);
                            }
                            break;
                        }
                    }

                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, tokenPageContext->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                {
                                    PhCopyListView(tokenPageContext->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    case WM_CTLCOLORBTN:
        {
            if (tokenPageContext->SinglePageContext)
                return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
        }
        break;
    case WM_CTLCOLORDLG:
        {
            if (tokenPageContext->SinglePageContext)
                return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            if (tokenPageContext->SinglePageContext)
                return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
        }
        break;
    }

    return FALSE;
}

VOID PhpShowTokenAdvancedProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PTOKEN_PAGE_CONTEXT Context,
    _In_ BOOLEAN ShowAppContainerPage
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    HPROPSHEETPAGE pages[7];
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
        if (ShowAppContainerPage)
        {
            memset(&page, 0, sizeof(PROPSHEETPAGE));
            page.dwSize = sizeof(PROPSHEETPAGE);
            page.dwFlags = PSP_USETITLE;
            page.pszTemplate = MAKEINTRESOURCE(IDD_TOKADVANCED);
            page.hInstance = PhInstanceHandle;
            page.pszTitle = L"Container";
            page.pfnDlgProc = PhpTokenContainerPageProc;
            page.lParam = (LPARAM)Context;
            pages[numberOfPages++] = CreatePropertySheetPage(&page);
        }

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

        // AppModel Policy

        memset(&page, 0, sizeof(PROPSHEETPAGE));
        page.dwSize = sizeof(PROPSHEETPAGE);
        page.dwFlags = PSP_USETITLE;
        page.pszTemplate = MAKEINTRESOURCE(IDD_TOKAPPPOLICY);
        page.hInstance = PhInstanceHandle;
        page.pszTitle = L"Policy";
        page.pfnDlgProc = PhpTokenAppPolicyPageProc;
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
    if (Context)
        return PhGetTokenLinkedToken((HANDLE)Context, Handle);

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS PhpCloseLinkedToken(
    _In_opt_ HANDLE Handle,
    _In_opt_ BOOLEAN Release,
    _In_opt_ PVOID Context
    )
{
    if (Handle) NtClose(Handle);
    return STATUS_SUCCESS;
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
            ULONG tokenSessionId = ULONG_MAX;
            BOOLEAN tokenElevation = FALSE;
            TOKEN_ELEVATION_TYPE tokenElevationType = 0;
            PPH_STRINGREF tokenElevationTypeString;
            BOOLEAN hasLinkedToken = FALSE;
            PWSTR tokenVirtualization = L"N/A";
            PWSTR tokenUIAccess = L"Unknown";
            WCHAR tokenSourceName[TOKEN_SOURCE_LENGTH + 1] = { L"Unknown" };
            WCHAR tokenSourceLuid[PH_PTR_STR_LEN_1] = { L"Unknown" };

            // HACK
            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg)));

            if (NT_SUCCESS(tokenPageContext->OpenObject(
                &tokenHandle,
                TOKEN_QUERY,
                tokenPageContext->Context // ProcessId
                )))
            {
                PH_TOKEN_USER tokenUser;
                PH_TOKEN_OWNER tokenOwner;
                PTOKEN_PRIMARY_GROUP tokenPrimaryGroup;
                BOOLEAN isVirtualizationAllowed;
                BOOLEAN isVirtualizationEnabled;
                BOOLEAN isUIAccessEnabled;

                if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
                {
                    tokenUserName = PH_AUTO(PhGetSidFullName(tokenUser.User.Sid, TRUE, NULL));
                    tokenUserSid = PH_AUTO(PhSidToStringSid(tokenUser.User.Sid));
                }

                if (NT_SUCCESS(PhGetTokenOwner(tokenHandle, &tokenOwner)))
                {
                    tokenOwnerName = PH_AUTO(PhGetSidFullName(tokenOwner.Owner.Sid, TRUE, NULL));
                }

                if (NT_SUCCESS(PhGetTokenPrimaryGroup(tokenHandle, &tokenPrimaryGroup)))
                {
                    tokenPrimaryGroupName = PH_AUTO(PhGetSidFullName(tokenPrimaryGroup->PrimaryGroup, TRUE, NULL));
                    PhFree(tokenPrimaryGroup);
                }

                PhGetTokenSessionId(tokenHandle, &tokenSessionId);
                PhGetTokenElevation(tokenHandle, &tokenElevation);

                if (NT_SUCCESS(PhGetTokenElevationType(tokenHandle, &tokenElevationType)))
                {
                    hasLinkedToken = tokenElevationType != TokenElevationTypeDefault;
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

                if (NT_SUCCESS(PhGetTokenUIAccess(tokenHandle, &isUIAccessEnabled)))
                {
                    tokenUIAccess = isUIAccessEnabled ? L"Enabled": L"Disabled";
                }

                tokenPageContext->CloseObject(tokenHandle, FALSE, tokenPageContext->Context);
            }

            if (NT_SUCCESS(tokenPageContext->OpenObject(
                &tokenHandle,
                TOKEN_QUERY_SOURCE,
                tokenPageContext->Context // ProcessId
                )))
            {
                TOKEN_SOURCE tokenSource;

                if (NT_SUCCESS(PhGetTokenSource(tokenHandle, &tokenSource)))
                {
                    PhCopyStringZFromBytes(
                        tokenSource.SourceName,
                        TOKEN_SOURCE_LENGTH,
                        tokenSourceName,
                        RTL_NUMBER_OF(tokenSourceName),
                        NULL
                        );

                    PhPrintPointer(tokenSourceLuid, UlongToPtr(tokenSource.SourceIdentifier.LowPart));
                }

                tokenPageContext->CloseObject(tokenHandle, FALSE, tokenPageContext->Context);
            }

            PhSetDialogItemText(hwndDlg, IDC_USER, PhGetStringOrDefault(tokenUserName, L"Unknown"));
            PhSetDialogItemText(hwndDlg, IDC_USERSID, PhGetStringOrDefault(tokenUserSid, L"Unknown"));
            PhSetDialogItemText(hwndDlg, IDC_OWNER, PhGetStringOrDefault(tokenOwnerName, L"Unknown"));
            PhSetDialogItemText(hwndDlg, IDC_PRIMARYGROUP, PhGetStringOrDefault(tokenPrimaryGroupName, L"Unknown"));

            if (tokenSessionId != ULONG_MAX)
                PhSetDialogItemValue(hwndDlg, IDC_SESSIONID, tokenSessionId, FALSE);
            else
                PhSetDialogItemText(hwndDlg, IDC_SESSIONID, L"Unknown");

            if (PhGetElevationTypeString(tokenElevation, tokenElevationType, &tokenElevationTypeString))
                PhSetDialogItemText(hwndDlg, IDC_ELEVATED, PhGetStringRefZ(tokenElevationTypeString));
            else
                PhSetDialogItemText(hwndDlg, IDC_ELEVATED, L"Unknown");

            PhSetDialogItemText(hwndDlg, IDC_VIRTUALIZATION, tokenVirtualization);
            PhSetDialogItemText(hwndDlg, IDC_UIACCESS, tokenUIAccess);
            PhSetDialogItemText(hwndDlg, IDC_SOURCENAME, tokenSourceName);
            PhSetDialogItemText(hwndDlg, IDC_SOURCELUID, tokenSourceLuid);

            EnableWindow(GetDlgItem(hwndDlg, IDC_LINKEDTOKEN), !!hasLinkedToken);

            if (PhEnableThemeSupport) // TODO: Required for compat (dmex)
                PhInitializeWindowTheme(GetParent(hwndDlg), PhEnableThemeSupport);  // HACK (GetParent)
            else
                PhInitializeWindowTheme(hwndDlg, FALSE);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_LINKEDTOKEN:
                {
                    NTSTATUS status;
                    HANDLE tokenHandle;

                    if (NT_SUCCESS(status = tokenPageContext->OpenObject(
                        &tokenHandle,
                        TOKEN_QUERY,
                        tokenPageContext->Context // ProcessId
                        )))
                    {
                        PhShowTokenProperties(
                            hwndDlg,
                            PhpOpenLinkedToken,
                            PhpCloseLinkedToken,
                            tokenPageContext->ProcessId,
                            (PVOID)tokenHandle,
                            L"Linked Token"
                            );

                        tokenPageContext->CloseObject(tokenHandle, FALSE, tokenPageContext->Context);
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

typedef struct _PHP_TOKEN_ADVANCED_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    IListView* ListView;
    PH_LAYOUT_MANAGER LayoutManager;
} PHP_TOKEN_ADVANCED_CONTEXT, *PPHP_TOKEN_ADVANCED_CONTEXT;

INT_PTR CALLBACK PhpTokenAdvancedPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PTOKEN_PAGE_CONTEXT tokenPageContext;
    PPHP_TOKEN_ADVANCED_CONTEXT context;

    tokenPageContext = PhpTokenPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!tokenPageContext)
        return FALSE;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PHP_TOKEN_ADVANCED_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }
    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE tokenHandle;
            LONG listViewGroupIndex = 0;
            PWSTR tokenType = L"Unknown";
            PWSTR tokenImpersonationLevel = L"Unknown";
            WCHAR tokenLuid[PH_PTR_STR_LEN_1] = { L"Unknown" };
            WCHAR authenticationLuid[PH_PTR_STR_LEN_1] = { L"Unknown" };
            WCHAR tokenModifiedLuid[PH_PTR_STR_LEN_1] = { L"Unknown" };
            WCHAR tokenOriginLogonSession[PH_PTR_STR_LEN_1] = { L"Unknown" };
            PPH_STRING memoryUsed = NULL;
            PPH_STRING memoryAvailable = NULL;
            PPH_STRING tokenNamedObjectPathString = NULL;
            PPH_STRING tokenSecurityDescriptorString = NULL;
            PPH_STRING tokenTrustLevelSidString;
            PPH_STRING tokenTrustLevelNameString;
            PPH_STRING tokenProfilePathString;
            PPH_STRING tokenProfileRegistryString;
            PPH_STRING tokenSystemIdForPublisher = NULL;
            PPH_STRING tokenSystemIdForUser = NULL;

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->ListView = PhGetListViewInterface(context->ListViewHandle);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 280, L"Value");
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddIListViewGroup(context->ListView, listViewGroupIndex++, L"General");
            PhAddIListViewGroup(context->ListView, listViewGroupIndex++, L"LUIDs");
            PhAddIListViewGroup(context->ListView, listViewGroupIndex++, L"Memory");
            PhAddIListViewGroup(context->ListView, listViewGroupIndex++, L"Properties");
            PhAddIListViewGroupItem(context->ListView, 0, MAXINT, L"Type", NULL);
            PhAddIListViewGroupItem(context->ListView, 0, MAXINT, L"Impersonation level", NULL);
            PhAddIListViewGroupItem(context->ListView, 1, MAXINT, L"Token LUID", NULL);
            PhAddIListViewGroupItem(context->ListView, 1, MAXINT, L"Authentication LUID", NULL);
            PhAddIListViewGroupItem(context->ListView, 1, MAXINT, L"ModifiedId LUID", NULL);
            PhAddIListViewGroupItem(context->ListView, 1, MAXINT, L"Origin LUID", NULL);
            PhAddIListViewGroupItem(context->ListView, 2, MAXINT, L"Memory used", NULL);
            PhAddIListViewGroupItem(context->ListView, 2, MAXINT, L"Memory available", NULL);
            PhAddIListViewGroupItem(context->ListView, 3, MAXINT, L"Token object path", NULL);
            PhAddIListViewGroupItem(context->ListView, 3, MAXINT, L"Token SDDL", NULL);

            if (NT_SUCCESS(tokenPageContext->OpenObject(
                &tokenHandle,
                TOKEN_QUERY,
                tokenPageContext->Context // ProcessId
                )))
            {
                TOKEN_STATISTICS statistics;
                TOKEN_ORIGIN origin;

                if (NT_SUCCESS(PhGetTokenStatistics(tokenHandle, &statistics)))
                {
                    PhGetTokenTypeString(statistics.TokenType, &tokenType);
                    PhGetImpersonationLevelString(statistics.ImpersonationLevel, &tokenImpersonationLevel);

                    PhPrintPointer(tokenLuid, UlongToPtr(statistics.TokenId.LowPart));
                    PhPrintPointer(authenticationLuid, UlongToPtr(statistics.AuthenticationId.LowPart));
                    PhPrintPointer(tokenModifiedLuid, UlongToPtr(statistics.ModifiedId.LowPart));

                    memoryUsed = PhFormatSize(statistics.DynamicCharged - statistics.DynamicAvailable, ULONG_MAX); // DynamicAvailable contains the number of bytes free.
                    memoryAvailable = PhFormatSize(statistics.DynamicCharged, ULONG_MAX); // DynamicCharged contains the number of bytes allocated.
                }

                if (NT_SUCCESS(PhGetTokenOrigin(tokenHandle, &origin)))
                {
                    PhPrintPointer(tokenOriginLogonSession, UlongToPtr(origin.OriginatingLogonSession.LowPart));
                }

                PhGetTokenNamedObjectPath(tokenHandle, NULL, &tokenNamedObjectPathString);
                PhGetObjectSecurityDescriptorAsString(tokenHandle, &tokenSecurityDescriptorString);

                if (NT_SUCCESS(PhGetTokenProcessTrustLevelRID(
                    tokenHandle,
                    NULL,
                    NULL,
                    &tokenTrustLevelNameString,
                    &tokenTrustLevelSidString
                    )))
                {
                    LONG trustLevelGroupIndex;
                    LONG trustLevelSidIndex;
                    LONG trustLevelNameIndex;

                    trustLevelGroupIndex = PhAddIListViewGroup(context->ListView, listViewGroupIndex++, L"TrustLevel");
                    trustLevelSidIndex = PhAddIListViewGroupItem(context->ListView, trustLevelGroupIndex, MAXINT, L"TrustLevel Sid", NULL);
                    trustLevelNameIndex = PhAddIListViewGroupItem(context->ListView, trustLevelGroupIndex, MAXINT, L"TrustLevel Name", NULL);
                    PhSetIListViewSubItem(context->ListView, trustLevelSidIndex, 1, PhGetStringOrDefault(tokenTrustLevelSidString, L"N/A"));
                    PhSetIListViewSubItem(context->ListView, trustLevelNameIndex, 1, PhGetStringOrDefault(tokenTrustLevelNameString, L"N/A"));

                    PhClearReference(&tokenTrustLevelNameString);
                    PhClearReference(&tokenTrustLevelSidString);
                }

                //PTOKEN_GROUPS tokenLogonGroups;
                //if (NT_SUCCESS(PhQueryTokenVariableSize(tokenHandle, TokenLogonSid, &tokenLogonGroups)))
                //{
                //    PPH_STRING tokenLogonName = PhGetSidFullName(tokenLogonGroups->Groups[0].Sid, TRUE, NULL);
                //    PPH_STRING tokenLogonSid = PhSidToStringSid(tokenLogonGroups->Groups[0].Sid);
                //    LONG tokenLogonGroupIndex = PhAddListViewGroup(context->ListViewHandle, listViewGroupIndex++, L"Logon");
                //    LONG tokenLogonNameIndex = PhAddListViewGroupItem(context->ListViewHandle, tokenLogonGroupIndex, MAXINT, L"Token logon SID", NULL);
                //    LONG tokenLogonSidIndex = PhAddListViewGroupItem(context->ListViewHandle, tokenLogonGroupIndex, MAXINT, L"Token logon Name", NULL);
                //    PhSetListViewSubItem(context->ListViewHandle, tokenLogonNameIndex, 1, PhGetStringOrDefault(tokenLogonName, L"Unknown"));
                //    PhSetListViewSubItem(context->ListViewHandle, tokenLogonSidIndex, 1, PhGetStringOrDefault(tokenLogonSid, L"Unknown"));
                //    PhFree(tokenLogonGroups);
                //}

                if (tokenProfilePathString = PhpGetTokenFolderPath(tokenHandle))
                {
                    LONG profileGroupIndex;
                    LONG profileFolderIndex;
                    LONG profileRegistryIndex;

                    profileGroupIndex = PhAddIListViewGroup(context->ListView, listViewGroupIndex++, L"Profile");
                    profileFolderIndex = PhAddIListViewGroupItem(context->ListView, profileGroupIndex, MAXINT, L"Folder path", NULL);
                    profileRegistryIndex = PhAddIListViewGroupItem(context->ListView, profileGroupIndex, MAXINT, L"Registry path", NULL);

                    PhSetIListViewSubItem(context->ListView, profileFolderIndex, 1, PhGetStringOrDefault(tokenProfilePathString, L"N/A"));

                    if (tokenProfileRegistryString = PhpGetTokenRegistryPath(tokenHandle))
                    {
                        PhSetIListViewSubItem(context->ListView, profileRegistryIndex, 1, PhGetStringOrDefault(tokenProfileRegistryString, L"N/A"));
                        PhDereferenceObject(tokenProfileRegistryString);
                    }

                    PhDereferenceObject(tokenProfilePathString);
                }

                tokenPageContext->CloseObject(tokenHandle, FALSE, tokenPageContext->Context);
            }

            if (SUCCEEDED(PhGetProcessSystemIdentification(
                tokenPageContext->Context, // ProcessId,
                &tokenSystemIdForPublisher,
                &tokenSystemIdForUser
                )))
            {
                LONG systemIdGroupIndex;
                LONG systemIdPublisherIndex;
                LONG systemIdUserIndex;

                systemIdGroupIndex = PhAddIListViewGroup(context->ListView, listViewGroupIndex++, L"System ID");
                systemIdPublisherIndex = PhAddIListViewGroupItem(context->ListView, systemIdGroupIndex, MAXINT, L"HWID (Publisher)", NULL);
                systemIdUserIndex = PhAddIListViewGroupItem(context->ListView, systemIdGroupIndex, MAXINT, L"HWID (User)", NULL);

                PhSetIListViewSubItem(context->ListView, systemIdPublisherIndex, 1, PhGetStringOrDefault(tokenSystemIdForPublisher, L"N/A"));
                PhSetIListViewSubItem(context->ListView, systemIdUserIndex, 1, PhGetStringOrDefault(tokenSystemIdForUser, L"N/A"));

                PhClearReference(&tokenSystemIdForPublisher);
                PhClearReference(&tokenSystemIdForUser);
            }

            PhSetIListViewSubItem(context->ListView, 0, 1, tokenType);
            PhSetIListViewSubItem(context->ListView, 1, 1, tokenImpersonationLevel);
            PhSetIListViewSubItem(context->ListView, 2, 1, tokenLuid);
            PhSetIListViewSubItem(context->ListView, 3, 1, authenticationLuid);
            PhSetIListViewSubItem(context->ListView, 4, 1, tokenModifiedLuid);
            PhSetIListViewSubItem(context->ListView, 5, 1, tokenOriginLogonSession);
            PhSetIListViewSubItem(context->ListView, 6, 1, PhGetStringOrDefault(memoryUsed, L"Unknown"));
            PhSetIListViewSubItem(context->ListView, 7, 1, PhGetStringOrDefault(memoryAvailable, L"Unknown"));
            PhSetIListViewSubItem(context->ListView, 8, 1, PhGetStringOrDefault(tokenNamedObjectPathString, L"Unknown"));
            PhSetIListViewSubItem(context->ListView, 9, 1, PhGetStringOrDefault(tokenSecurityDescriptorString, L"Unknown"));

            PhClearReference(&memoryUsed);
            PhClearReference(&memoryAvailable);
            PhClearReference(&tokenNamedObjectPathString);
            PhClearReference(&tokenSecurityDescriptorString);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    }

    return FALSE;
}

BOOLEAN NTAPI PhpAttributeTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_TOKEN_ATTRIBUTE_TREE_CONTEXT context = Context;
    PPH_TOKEN_ATTRIBUTE_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPH_TOKEN_ATTRIBUTE_NODE)getChildren->Node;

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
            node = (PPH_TOKEN_ATTRIBUTE_NODE)isLeaf->Node;

            isLeaf->IsLeaf = node->Children->Count == 0;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            node = (PPH_TOKEN_ATTRIBUTE_NODE)getCellText->Node;

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
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(
                context->WindowHandle,
                WM_CONTEXTMENU,
                0,
                (LPARAM)contextMenuEvent
                );
        }
        return TRUE;
    }

    return FALSE;
}

PPH_TOKEN_ATTRIBUTE_NODE PhpAddAttributeNode(
    _In_ PPH_TOKEN_ATTRIBUTE_TREE_CONTEXT Context,
    _In_opt_ PPH_TOKEN_ATTRIBUTE_NODE Parent,
    _In_opt_ _Assume_refs_(1) PPH_STRING Text
    )
{
    PPH_TOKEN_ATTRIBUTE_NODE node;

    node = PhAllocate(sizeof(PH_TOKEN_ATTRIBUTE_NODE));
    memset(node, 0, sizeof(PH_TOKEN_ATTRIBUTE_NODE));
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
    _In_ PPH_TOKEN_ATTRIBUTE_NODE Node
    )
{
    PhDereferenceObject(Node->Children);
    PhClearReference(&Node->Text);
    PhClearReference(&Node->Value);
    PhFree(Node);
}

VOID PhpRemoveAttributeNode(
    _In_ PPH_TOKEN_ATTRIBUTE_TREE_CONTEXT Context,
    _In_ PPH_TOKEN_ATTRIBUTE_NODE Node
    )
{
    ULONG index;

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);
    if ((index = PhFindItemList(Context->RootList, Node)) != ULONG_MAX)
        PhRemoveItemList(Context->RootList, index);

    PhpDestroyAttributeNode(Node);
}

_Success_(return)
BOOLEAN PhpGetSelectedAttributeTreeNodes(
    _Inout_ PPH_TOKEN_ATTRIBUTE_TREE_CONTEXT Context,
    _Out_ PPH_TOKEN_ATTRIBUTE_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = (PPH_TOKEN_ATTRIBUTE_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    PhDereferenceObject(list);
    return FALSE;
}

VOID PhpInitializeAttributeTreeContext(
    _Out_ PPH_TOKEN_ATTRIBUTE_TREE_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    Context->WindowHandle = WindowHandle;
    Context->NodeList = PhCreateList(10);
    Context->RootList = PhCreateList(10);

    PhSetControlTheme(TreeNewHandle, L"explorer");
    TreeNew_SetRedraw(TreeNewHandle, FALSE);
    TreeNew_SetCallback(TreeNewHandle, PhpAttributeTreeNewCallback, Context);
    //TreeNew_GetViewParts(TreeNewHandle, &parts); // column width = (parts.ClientRect.right - parts.VScrollWidth) // TODO: VScrollWidth not set during INITDIALOG. (dmex)
    PhAddTreeNewColumnEx2(TreeNewHandle, 0, TRUE, L"Attributes", 200, PH_ALIGN_LEFT, 0, 0, TN_COLUMN_FLAG_NODPISCALEONADD);
    TreeNew_SetRedraw(TreeNewHandle, TRUE);
}

VOID PhpDeleteAttributeTreeContext(
    _Inout_ PPH_TOKEN_ATTRIBUTE_TREE_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
        PhpDestroyAttributeNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeList);
    PhDereferenceObject(Context->RootList);
}

//static COLORREF NTAPI PhpTokenCapabilitiesColorFunction(
//    _In_ LONG Index,
//    _In_ PVOID Param,
//    _In_opt_ PVOID Context
//    )
//{
//    PSID_AND_ATTRIBUTES sidAndAttributes = Param;
//
//    return PhGetGroupAttributesColor(sidAndAttributes->Attributes);
//}

BOOLEAN PhpAddTokenCapabilities(
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext,
    _In_ HWND tnHandle
    )
{
    HANDLE tokenHandle;
    ULONG i;

    if (!NT_SUCCESS(TokenPageContext->OpenObject(
        &tokenHandle,
        TOKEN_QUERY,
        TokenPageContext->Context
        )))
        return FALSE;

    if (NT_SUCCESS(PhQueryTokenVariableSize(tokenHandle, TokenCapabilities, &TokenPageContext->Capabilities)))
    {
        for (i = 0; i < TokenPageContext->Capabilities->GroupCount; i++)
        {
            PPH_TOKEN_ATTRIBUTE_NODE node;
            PPH_STRING name;
            ULONG subAuthoritiesCount;
            ULONG subAuthority;

            name = PhSidToStringSid(TokenPageContext->Capabilities->Groups[i].Sid);
            node = PhpAddAttributeNode(&TokenPageContext->CapsTreeContext, NULL, name);

            if (name = PhGetSidFullName(TokenPageContext->Capabilities->Groups[i].Sid, TRUE, NULL))
            {
                PhpAddAttributeNode(&TokenPageContext->CapsTreeContext, node, PhFormatString(L"FullName: %s", PhGetString(name)));
                PhDereferenceObject(name);
            }

            if (name = PhGetCapabilitySidName(TokenPageContext->Capabilities->Groups[i].Sid))
            {
                PhpAddAttributeNode(&TokenPageContext->CapsTreeContext, node, PhFormatString(L"Capability: %s", PhGetString(name)));
                PhDereferenceObject(name);
            }

            subAuthoritiesCount = *PhSubAuthorityCountSid(TokenPageContext->Capabilities->Groups[i].Sid);
            subAuthority = *PhSubAuthoritySid(TokenPageContext->Capabilities->Groups[i].Sid, 0);

            if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(TokenPageContext->Capabilities->Groups[i].Sid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_APP_PACKAGE_AUTHORITY))
            {
                if (subAuthority == SECURITY_CAPABILITY_BASE_RID)
                {
                    if (subAuthoritiesCount == SECURITY_APP_PACKAGE_RID_COUNT)
                    {
                        PH_TOKEN_APPCONTAINER tokenAppContainer;

                        //if (*PhSubAuthoritySid(TokenPageContext->Capabilities->Groups[i].Sid, 1) == SECURITY_CAPABILITY_APP_RID)
                        //    continue;

                        if (NT_SUCCESS(PhGetTokenAppContainerSid(tokenHandle, &tokenAppContainer)))
                        {
                            if (PhIsPackageCapabilitySid(tokenAppContainer.AppContainer.Sid, TokenPageContext->Capabilities->Groups[i].Sid))
                            {
                                static PH_STRINGREF packageNameStringRef = PH_STRINGREF_INIT(L"Package: ");

                                if (name = PhGetTokenPackageFullName(tokenHandle))
                                {
                                    PhpAddAttributeNode(&TokenPageContext->CapsTreeContext, node, PhConcatStringRef2(&packageNameStringRef, &name->sr));
                                    PhDereferenceObject(name);
                                }
                                else
                                {
                                    PhpAddAttributeNode(&TokenPageContext->CapsTreeContext, node, PhCreateString2(&packageNameStringRef));
                                }
                            }
                        }
                    }
                    else if (subAuthoritiesCount == SECURITY_CAPABILITY_RID_COUNT)
                    {
                        PPH_STRING capabilityName;
                        union
                        {
                            GUID Guid;
                            struct
                            {
                                ULONG Data1;
                                ULONG Data2;
                                ULONG Data3;
                                ULONG Data4;
                            };
                        } capabilityGuid;

                        capabilityGuid.Data1 = *PhSubAuthoritySid(TokenPageContext->Capabilities->Groups[i].Sid, 1);
                        capabilityGuid.Data2 = *PhSubAuthoritySid(TokenPageContext->Capabilities->Groups[i].Sid, 2);
                        capabilityGuid.Data3 = *PhSubAuthoritySid(TokenPageContext->Capabilities->Groups[i].Sid, 3);
                        capabilityGuid.Data4 = *PhSubAuthoritySid(TokenPageContext->Capabilities->Groups[i].Sid, 4);

                        if (name = PhFormatGuid(&capabilityGuid.Guid))
                        {
                            static PH_STRINGREF guidNameStringRef = PH_STRINGREF_INIT(L"Guid: ");
                            static PH_STRINGREF capabilityNameStringRef = PH_STRINGREF_INIT(L"Capability: ");

                            PhpAddAttributeNode(&TokenPageContext->CapsTreeContext, node, PhConcatStringRef2(&guidNameStringRef, &name->sr));

                            if (capabilityName = PhGetCapabilityGuidName(name))
                            {
                                PhpAddAttributeNode(&TokenPageContext->CapsTreeContext, node, PhConcatStringRef2(&capabilityNameStringRef, &capabilityName->sr));
                                PhDereferenceObject(capabilityName);
                            }

                            PhDereferenceObject(name);
                        }
                    }
                }
            }
        }
    }

    TokenPageContext->CloseObject(tokenHandle, FALSE, TokenPageContext->Context);

    return TRUE;
}

INT_PTR CALLBACK PhpTokenCapabilitiesPageProc(
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
            tokenPageContext->CapsTreeContext.WindowHandle = hwndDlg;
            tokenPageContext->CapsTreeContext.NodeList = PhCreateList(10);
            tokenPageContext->CapsTreeContext.RootList = PhCreateList(10);

            PhSetControlTheme(tnHandle, L"explorer");
            TreeNew_SetRedraw(tnHandle, FALSE);
            TreeNew_SetEmptyText(tnHandle, &PhpEmptyTokenCapabilitiesText, 0);
            TreeNew_SetCallback(tnHandle, PhpAttributeTreeNewCallback, &tokenPageContext->CapsTreeContext);
            PhAddTreeNewColumnEx2(tnHandle, 0, TRUE, L"Capabilities", 200, PH_ALIGN_LEFT, 0, 0, TN_COLUMN_FLAG_NODPISCALEONADD);
            PhpAddTokenCapabilities(tokenPageContext, tnHandle);
            TreeNew_NodesStructured(tnHandle);
            TreeNew_SetRedraw(tnHandle, TRUE);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhpDeleteAttributeTreeContext(&tokenPageContext->CapsTreeContext);

            if (tokenPageContext->Capabilities)
            {
                PhFree(tokenPageContext->Capabilities);
                tokenPageContext->Capabilities = NULL;
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            TreeNew_AutoSizeColumn(tnHandle, 0, TN_AUTOSIZE_REMAINING_SPACE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPH_TOKEN_ATTRIBUTE_NODE *attributeObjectNodes = NULL;
            ULONG numberOfAttributeObjectNodes = 0;

            if (!PhpGetSelectedAttributeTreeNodes(&tokenPageContext->CapsTreeContext, &attributeObjectNodes, &numberOfAttributeObjectNodes))
                break;

            if (numberOfAttributeObjectNodes != 0)
            {
                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"Copy", NULL, NULL), ULONG_MAX);
                PhInsertCopyCellEMenuItem(menu, IDC_COPY, tnHandle, contextMenuEvent->Column);

                selectedItem = PhShowEMenu(
                    menu,
                    hwndDlg,
                    PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    contextMenuEvent->Location.x,
                    contextMenuEvent->Location.y
                    );

                if (selectedItem && selectedItem->Id != ULONG_MAX)
                {
                    if (!PhHandleCopyCellEMenuItem(selectedItem))
                    {
                        switch (selectedItem->Id)
                        {
                        case IDC_COPY:
                            {
                                PPH_STRING text;

                                text = PhGetTreeNewText(tnHandle, 0);
                                PhSetClipboardString(tnHandle, &text->sr);
                                PhDereferenceObject(text);
                            }
                            break;
                        }
                    }
                }

                PhDestroyEMenu(menu);
            }

            PhFree(attributeObjectNodes);
        }
        break;
    }

    return FALSE;
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
    if (Flags & TOKEN_SECURITY_ATTRIBUTE_COMPARE_IGNORE)
        PhAppendStringBuilder2(&sb, L"Compare-ignore, ");

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
        return PhFormatString(L"Version %llu: %s",
            Attribute->Values.pFqbn[ValueIndex].Version,
            Attribute->Values.pFqbn[ValueIndex].Name);
    case CLAIM_SECURITY_ATTRIBUTE_TYPE_SID:
        {
            if (PhValidSid(Attribute->Values.pOctetString[ValueIndex].pValue))
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
    _In_ PPH_STRINGREF Name,
    _In_ PTOKEN_SECURITY_ATTRIBUTE_V1 Attribute,
    _In_ ULONG ValueIndex
    )
{
    static PH_STRINGREF winPkg = PH_STRINGREF_INIT(L"WIN://PKG");
    PH_FORMAT format[6];
    ULONG count = 0;

    // Special cases for known attributes
    if (ValueIndex == 0 &&
        Attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_UINT64 &&
        PhEqualStringRef(Name, &winPkg, TRUE))
    {
        ULONG upper = (ULONG)(Attribute->Values.pUint64[0] >> 32);
        ULONG lower = (ULONG)Attribute->Values.pUint64[0];
        PH_STRING_BUILDER sb;
        PPH_STRING string;

        switch (upper)
        {
        case PackageOrigin_Unknown:
            PhInitFormatS(&format[count++], L"Unknown");
            break;
        case PackageOrigin_Unsigned:
            PhInitFormatS(&format[count++], L"Unsigned");
            break;
        case PackageOrigin_Inbox:
            PhInitFormatS(&format[count++], L"Inbox");
            break;
        case PackageOrigin_Store:
            PhInitFormatS(&format[count++], L"Store");
            break;
        case PackageOrigin_DeveloperUnsigned:
            PhInitFormatS(&format[count++], L"Developer unsigned");
            break;
        case PackageOrigin_DeveloperSigned:
            PhInitFormatS(&format[count++], L"Developer signed");
            break;
        case PackageOrigin_LineOfBusiness:
            PhInitFormatS(&format[count++], L"Line of business");
            break;
        default:
            PhInitFormatS(&format[count++], L"Undefined");
            break;
        }

        PhInitializeStringBuilder(&sb, 10);
        if (FlagOn(lower, PSM_ACTIVATION_TOKEN_PACKAGED_APPLICATION))
            PhAppendStringBuilder2(&sb, L"AppX, ");
        if (FlagOn(lower, PSM_ACTIVATION_TOKEN_SHARED_ENTITY))
            PhAppendStringBuilder2(&sb, L"Shared token, ");
        if (FlagOn(lower, PSM_ACTIVATION_TOKEN_FULL_TRUST))
            PhAppendStringBuilder2(&sb, L"Trusted, ");
        if (FlagOn(lower, PSM_ACTIVATION_TOKEN_NATIVE_SERVICE))
            PhAppendStringBuilder2(&sb, L"Service, ");
        if (FlagOn(lower, PSM_ACTIVATION_TOKEN_MULTIPLE_INSTANCES_ALLOWED))
            PhAppendStringBuilder2(&sb, L"Multiple instances allowed, ");
        if (FlagOn(lower, PSM_ACTIVATION_TOKEN_BREAKAWAY_INHIBITED))
            PhAppendStringBuilder2(&sb, L"Breakaway inhibited, ");
        if (FlagOn(lower, PSM_ACTIVATION_TOKEN_RUNTIME_BROKER))
            PhAppendStringBuilder2(&sb, L"Runtime broker, ");
        if (FlagOn(lower, PSM_ACTIVATION_TOKEN_UNIVERSAL_CONSOLE))
            PhAppendStringBuilder2(&sb, L"Universal console, ");
        if (FlagOn(lower, PSM_ACTIVATION_TOKEN_WIN32ALACARTE_PROCESS))
            PhAppendStringBuilder2(&sb, L"Win32 process, ");

        if (sb.String->Length != 0)
            PhRemoveEndStringBuilder(&sb, 2);

        string = PhFinalStringBuilderString(&sb);

        if (string->Length != 0)
        {
            PhInitFormatS(&format[count++], L" : ");
            PhInitFormatSR(&format[count++], string->sr);
        }

        PhInitFormatS(&format[count++], L" (0x");
        PhInitFormatI64X(&format[count++], Attribute->Values.pUint64[0]);
        PhInitFormatS(&format[count++], L")");

        PhMoveReference(&string, PhFormat(format, count, 0));

        return string;
    }

    // Default handling
    switch (Attribute->ValueType)
    {
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_INT64:
        PhInitFormatI64D(&format[0], Attribute->Values.pInt64[ValueIndex]);
        PhInitFormatS(&format[1], L" (0x");
        PhInitFormatI64X(&format[2], Attribute->Values.pInt64[ValueIndex]);
        PhInitFormatS(&format[3], L")");
        return PhFormat(format, 4, 0);
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_UINT64:
        PhInitFormatI64U(&format[0], Attribute->Values.pUint64[ValueIndex]);
        PhInitFormatS(&format[1], L" (0x");
        PhInitFormatI64X(&format[2], Attribute->Values.pUint64[ValueIndex]);
        PhInitFormatS(&format[3], L")");
        return PhFormat(format, 4, 0);
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING:
        return PhCreateStringFromUnicodeString(&Attribute->Values.pString[ValueIndex]);
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_FQBN:
        return PhFormatString(L"Version %llu: %.*s",
            Attribute->Values.pFqbn[ValueIndex].Version,
            Attribute->Values.pFqbn[ValueIndex].Name.Length / (USHORT)sizeof(WCHAR),
            Attribute->Values.pFqbn[ValueIndex].Name.Buffer);
    case TOKEN_SECURITY_ATTRIBUTE_TYPE_SID:
        {
            if (PhValidSid(Attribute->Values.pOctetString[ValueIndex].Value))
            {
                PPH_STRING name;

                name = PhGetSidFullName(Attribute->Values.pOctetString[ValueIndex].Value, TRUE, NULL);

                if (name)
                    return name;

                name = PhSidToStringSid(Attribute->Values.pOctetString[ValueIndex].Value);

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
    _In_ PPH_TOKEN_ATTRIBUTE_NODE Parent
    )
{
    HANDLE tokenHandle;
    PCLAIM_SECURITY_ATTRIBUTES_INFORMATION info;
    ULONG i;
    ULONG j;

    if (!NT_SUCCESS(TokenPageContext->OpenObject(
        &tokenHandle,
        TOKEN_QUERY,
        TokenPageContext->Context // ProcessId
        )))
        return FALSE;

    if (NT_SUCCESS(PhQueryTokenVariableSize(tokenHandle, DeviceClaims ? TokenDeviceClaimAttributes : TokenUserClaimAttributes, &info)))
    {
        for (i = 0; i < info->AttributeCount; i++)
        {
            PCLAIM_SECURITY_ATTRIBUTE_V1 attribute = &info->Attribute.pAttributeV1[i];
            PPH_TOKEN_ATTRIBUTE_NODE node;
            PPH_STRING temp;

            // Attribute
            node = PhpAddAttributeNode(&TokenPageContext->ClaimsTreeContext, Parent, PhCreateString(attribute->Name));
            // Type
            PhpAddAttributeNode(&TokenPageContext->ClaimsTreeContext, node,
                PhFormatString(L"Type: %s", PhGetSecurityAttributeTypeString(attribute->ValueType)));
            // Flags
            temp = PhGetSecurityAttributeFlagsString(attribute->Flags);
            PhpAddAttributeNode(&TokenPageContext->ClaimsTreeContext, node,
                PhFormatString(L"Flags: %s (0x%lx)", temp->Buffer, attribute->Flags));
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

    TokenPageContext->CloseObject(tokenHandle, FALSE, TokenPageContext->Context);

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
            PPH_TOKEN_ATTRIBUTE_NODE userNode;
            PPH_TOKEN_ATTRIBUTE_NODE deviceNode;

            PhpInitializeAttributeTreeContext(&tokenPageContext->ClaimsTreeContext, hwndDlg, tnHandle);

            TreeNew_SetEmptyText(tnHandle, &PhpEmptyTokenClaimsText, 0);
            TreeNew_SetRedraw(tnHandle, FALSE);

            userNode = PhpAddAttributeNode(&tokenPageContext->ClaimsTreeContext, NULL, PhCreateString(L"User claims"));
            deviceNode = PhpAddAttributeNode(&tokenPageContext->ClaimsTreeContext, NULL, PhCreateString(L"Device claims"));

            PhpAddTokenClaimAttributes(tokenPageContext, tnHandle, FALSE, userNode);
            PhpAddTokenClaimAttributes(tokenPageContext, tnHandle, TRUE, deviceNode);

            if (userNode->Children->Count == 0 && deviceNode->Children->Count == 0)
            {
                PhpRemoveAttributeNode(&tokenPageContext->ClaimsTreeContext, userNode);
                PhpRemoveAttributeNode(&tokenPageContext->ClaimsTreeContext, deviceNode);
            }
            else
            {
                if (userNode->Children->Count == 0)
                    PhpAddAttributeNode(&tokenPageContext->ClaimsTreeContext, userNode, PhCreateString(L"(None)"));
                if (deviceNode->Children->Count == 0)
                    PhpAddAttributeNode(&tokenPageContext->ClaimsTreeContext, deviceNode, PhCreateString(L"(None)"));
            }

            TreeNew_NodesStructured(tnHandle);
            TreeNew_SetRedraw(tnHandle, TRUE);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhpDeleteAttributeTreeContext(&tokenPageContext->ClaimsTreeContext);
        }
        break;
    case WM_SHOWWINDOW:
        {
            TreeNew_AutoSizeColumn(tnHandle, 0, TN_AUTOSIZE_REMAINING_SPACE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPH_TOKEN_ATTRIBUTE_NODE *attributeObjectNodes = NULL;
            ULONG numberOfAttributeObjectNodes = 0;

            if (!PhpGetSelectedAttributeTreeNodes(&tokenPageContext->ClaimsTreeContext, &attributeObjectNodes, &numberOfAttributeObjectNodes))
                break;

            if (numberOfAttributeObjectNodes != 0)
            {
                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"Copy", NULL, NULL), ULONG_MAX);
                PhInsertCopyCellEMenuItem(menu, IDC_COPY, tnHandle, contextMenuEvent->Column);

                selectedItem = PhShowEMenu(
                    menu,
                    hwndDlg,
                    PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    contextMenuEvent->Location.x,
                    contextMenuEvent->Location.y
                    );

                if (selectedItem && selectedItem->Id != ULONG_MAX)
                {
                    if (!PhHandleCopyCellEMenuItem(selectedItem))
                    {
                        switch (selectedItem->Id)
                        {
                        case IDC_COPY:
                            {
                                PPH_STRING text;

                                text = PhGetTreeNewText(tnHandle, 0);
                                PhSetClipboardString(tnHandle, &text->sr);
                                PhDereferenceObject(text);
                            }
                            break;
                        }
                    }
                }

                PhDestroyEMenu(menu);
            }

            PhFree(attributeObjectNodes);
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
        TokenPageContext->Context // ProcessId
        )))
        return FALSE;

    if (NT_SUCCESS(PhGetTokenSecurityAttributes(tokenHandle, &info)))
    {
        for (i = 0; i < info->AttributeCount; i++)
        {
            PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = &info->Attribute.pAttributeV1[i];
            PPH_STRING name;
            PPH_TOKEN_ATTRIBUTE_NODE node;
            PPH_STRING temp;

            name = PhCreateStringFromUnicodeString(&attribute->Name);

            // Attribute
            node = PhpAddAttributeNode(&TokenPageContext->AuthzTreeContext, NULL, PhReferenceObject(name));
            // Type
            PhpAddAttributeNode(&TokenPageContext->AuthzTreeContext, node,
                PhFormatString(L"Type: %s", PhGetSecurityAttributeTypeString(attribute->ValueType)));
            // Flags
            temp = PhGetSecurityAttributeFlagsString(attribute->Flags);
            PhpAddAttributeNode(&TokenPageContext->AuthzTreeContext, node,
                PhFormatString(L"Flags: %s (0x%lx)", temp->Buffer, attribute->Flags));
            PhDereferenceObject(temp);

            // Values
            for (j = 0; j < attribute->ValueCount; j++)
            {
                temp = PhFormatTokenSecurityAttributeValue(&name->sr, attribute, j);
                PhpAddAttributeNode(&TokenPageContext->AuthzTreeContext, node,
                    PhFormatString(L"Value %u: %s", j, temp->Buffer));
                PhDereferenceObject(temp);
            }
        }

        PhFree(info);
    }

    TokenPageContext->CloseObject(tokenHandle, FALSE, TokenPageContext->Context);

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
            PhpInitializeAttributeTreeContext(&tokenPageContext->AuthzTreeContext, hwndDlg, tnHandle);

            TreeNew_SetEmptyText(tnHandle, &PhpEmptyTokenAttributesText, 0);
            TreeNew_SetRedraw(tnHandle, FALSE);

            PhpAddTokenAttributes(tokenPageContext, tnHandle);

            //if (tokenPageContext->AuthzTreeContext.RootList->Count == 0)
            //    PhpAddAttributeNode(&tokenPageContext->AuthzTreeContext, NULL, PhCreateString(L"(None)"));

            TreeNew_NodesStructured(tnHandle);
            TreeNew_SetRedraw(tnHandle, TRUE);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhpDeleteAttributeTreeContext(&tokenPageContext->AuthzTreeContext);
        }
        break;
    case WM_SHOWWINDOW:
        {
            TreeNew_AutoSizeColumn(tnHandle, 0, TN_AUTOSIZE_REMAINING_SPACE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPH_TOKEN_ATTRIBUTE_NODE *attributeObjectNodes = NULL;
            ULONG numberOfAttributeObjectNodes = 0;

            if (!PhpGetSelectedAttributeTreeNodes(&tokenPageContext->AuthzTreeContext, &attributeObjectNodes, &numberOfAttributeObjectNodes))
                break;

            if (numberOfAttributeObjectNodes != 0)
            {
                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"Copy", NULL, NULL), ULONG_MAX);
                PhInsertCopyCellEMenuItem(menu, IDC_COPY, tnHandle, contextMenuEvent->Column);

                selectedItem = PhShowEMenu(
                    menu,
                    hwndDlg,
                    PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    contextMenuEvent->Location.x,
                    contextMenuEvent->Location.y
                    );

                if (selectedItem && selectedItem->Id != ULONG_MAX)
                {
                    if (!PhHandleCopyCellEMenuItem(selectedItem))
                    {
                        switch (selectedItem->Id)
                        {
                        case IDC_COPY:
                            {
                                PPH_STRING text;

                                text = PhGetTreeNewText(tnHandle, 0);
                                PhSetClipboardString(tnHandle, &text->sr);
                                PhDereferenceObject(text);
                            }
                            break;
                        }
                    }
                }

                PhDestroyEMenu(menu);
            }

            PhFree(attributeObjectNodes);
        }
        break;
    }

    return FALSE;
}

// rev from GetUserProfileDirectory (dmex)
PPH_STRING PhpGetTokenFolderPath(
    _In_ HANDLE TokenHandle
    )
{
    static PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
    PPH_STRING profileFolderPath = NULL;
    PPH_STRING profileKeyPath = NULL;
    PPH_STRING tokenUserSid;
    PH_TOKEN_USER tokenUser;

    if (NT_SUCCESS(PhGetTokenUser(TokenHandle, &tokenUser)))
    {
        if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(tokenUser.User.Sid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_NT_AUTHORITY))
        {
            ULONG subAuthority = *PhSubAuthoritySid(tokenUser.User.Sid, 0);

            if (subAuthority == SECURITY_UMFD_BASE_RID)
            {
                if (tokenUserSid = PhSidToStringSid((PSID)&PhSeLocalSystemSid))
                {
                    profileKeyPath = PhConcatStringRef2(&servicesKeyName, &tokenUserSid->sr);
                    PhDereferenceObject(tokenUserSid);
                }
            }
            else
            {
                if (tokenUserSid = PhSidToStringSid(tokenUser.User.Sid))
                {
                    profileKeyPath = PhConcatStringRef2(&servicesKeyName, &tokenUserSid->sr);
                    PhDereferenceObject(tokenUserSid);
                }
            }
        }
        else
        {
            if (tokenUserSid = PhSidToStringSid(tokenUser.User.Sid))
            {
                profileKeyPath = PhConcatStringRef2(&servicesKeyName, &tokenUserSid->sr);
                PhDereferenceObject(tokenUserSid);
            }
        }
    }

    if (profileKeyPath)
    {
        HANDLE keyHandle;

        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_READ,
            PH_KEY_LOCAL_MACHINE,
            &profileKeyPath->sr,
            0
            )))
        {
            PPH_STRING profileImagePath;

            if (profileFolderPath = PhQueryRegistryStringZ(keyHandle, L"ProfileImagePath"))
            {
                if (profileImagePath = PhExpandEnvironmentStrings(&profileFolderPath->sr))
                {
                    PhMoveReference(&profileFolderPath, profileImagePath);
                }
            }

            NtClose(keyHandle);
        }

        PhDereferenceObject(profileKeyPath);
    }

    //ULONG profileFolderLength;
    //if (GetUserProfileDirectory)
    //{
    //    GetUserProfileDirectory(TokenHandle, NULL, &profileFolderLength);
    //    profileFolderPath = PhCreateStringEx(NULL, profileFolderLength * sizeof(WCHAR));
    //    GetUserProfileDirectory(TokenHandle, profileFolderPath->Buffer, &profileFolderLength);
    //}

    return profileFolderPath;
}

PPH_STRING PhpGetTokenRegistryPath(
    _In_ HANDLE TokenHandle
    )
{
    PPH_STRING profileRegistryPath = NULL;
    PPH_STRING tokenUserSid = NULL;
    PH_TOKEN_USER tokenUser;

    if (NT_SUCCESS(PhGetTokenUser(TokenHandle, &tokenUser)))
    {
        tokenUserSid = PhSidToStringSid(tokenUser.User.Sid);
    }

    if (tokenUserSid)
    {
        NTSTATUS status;
        HANDLE keyHandle = NULL;

        status = PhOpenKey(
            &keyHandle,
            KEY_READ,
            PH_KEY_USERS,
            &tokenUserSid->sr,
            0
            );

        if (NT_SUCCESS(status) || status == STATUS_ACCESS_DENIED)
        {
            profileRegistryPath = PhConcatStrings2(L"HKU\\", tokenUserSid->Buffer);
        }

        if (keyHandle)
            NtClose(keyHandle);

        PhDereferenceObject(tokenUserSid);
    }

    return profileRegistryPath;
}

PPH_STRING PhpGetTokenAppContainerFolderPath(
    _In_ HANDLE TokenHandle,
    _In_opt_ PSID TokenAppContainerSid
    )
{
    if (PhGetTokenIsFullTrustPackage(TokenHandle))
    {
        PPH_STRING packageLocalAppData = PhGetKnownFolderPathEx(
            &FOLDERID_LocalAppData,
            PH_KF_FLAG_FORCE_PACKAGE_REDIRECTION,
            TokenHandle,
            NULL
            );
        PPH_STRING localAppData = PhGetKnownFolderPathEx(
            &FOLDERID_LocalAppData,
            0,
            NULL,
            NULL
            );

        if (PhEqualString(packageLocalAppData, localAppData, TRUE))
        {
            PhDereferenceObject(localAppData);
            PhDereferenceObject(packageLocalAppData);
            return NULL;
        }

        PhDereferenceObject(localAppData);

        return packageLocalAppData;
    }
    else if (TokenAppContainerSid)
    {
        PPH_STRING appContainerFolderPath;
        //PWSTR folderPath = NULL;

        appContainerFolderPath = PhGetKnownFolderPathEx(
            &FOLDERID_LocalAppData,
            PH_KF_FLAG_FORCE_APPCONTAINER_REDIRECTION,
            TokenHandle,
            NULL
            );

        //if (PhIsNullOrEmptyString(appContainerFolderPath))
        //{
        //    if (NT_SUCCESS(PhImpersonateToken(NtCurrentThread(), TokenHandle)))
        //    {
        //        if (GetAppContainerFolderPath_Import())
        //        {
        //            PPH_STRING appContainerSid = PhSidToStringSid(TokenAppContainerSid);
        //
        //            if (SUCCEEDED(GetAppContainerFolderPath_Import()(appContainerSid->Buffer, &folderPath)) && folderPath)
        //            {
        //                assert(PhEqualString2(appContainerFolderPath, folderPath, TRUE));
        //                CoTaskMemFree(folderPath);
        //            }
        //
        //            PhDereferenceObject(appContainerSid);
        //        }
        //
        //        PhRevertImpersonationToken(NtCurrentThread());
        //    }
        //}

        // Workaround for pseudo Appcontainers created by System processes that default to the \systemprofile path. (dmex)
        if (PhIsNullOrEmptyString(appContainerFolderPath))
        {
            PH_TOKEN_USER tokenUser;

            if (NT_SUCCESS(PhGetTokenUser(TokenHandle, &tokenUser)))
            {
                ULONG subAuthority;
                PPH_STRING tokenProfilePathString;
                PPH_STRING appContainerName;

                subAuthority = *PhSubAuthoritySid(tokenUser.User.Sid, 0);
                //PhIdentifierAuthoritySid(tokenUser.User.Sid) == (BYTE[])SECURITY_NT_AUTHORITY

                if (subAuthority == SECURITY_UMFD_BASE_RID)
                {
                    if (tokenProfilePathString = PhpGetTokenFolderPath(TokenHandle))
                    {
                        if (appContainerName = PhGetAppContainerName(TokenAppContainerSid))
                        {
                            static PH_STRINGREF appDataPackagePath = PH_STRINGREF_INIT(L"\\AppData\\Local\\Packages\\");

                            PhMoveReference(&appContainerFolderPath, PhConcatStringRef3(
                                &tokenProfilePathString->sr,
                                &appDataPackagePath,
                                &appContainerName->sr
                                ));

                            PhDereferenceObject(appContainerName);
                        }

                        PhDereferenceObject(tokenProfilePathString);
                    }
                }
            }
        }

        return appContainerFolderPath;
    }
    else
    {
        return NULL;
    }
}

PPH_STRING PhpGetTokenAppContainerRegistryPath(
    _In_ HANDLE TokenHandle
    )
{
    PPH_STRING appContainerRegistryPath = NULL;
    HKEY registryHandle = NULL;

    if (NT_SUCCESS(PhImpersonateToken(NtCurrentThread(), TokenHandle)))
    {
        if (GetAppContainerRegistryLocation_Import())
            GetAppContainerRegistryLocation_Import()(KEY_READ, &registryHandle);

        PhRevertImpersonationToken(NtCurrentThread());
    }

    if (registryHandle)
    {
        PhQueryObjectName(registryHandle, &appContainerRegistryPath);

        NtClose(registryHandle);
    }

    return appContainerRegistryPath;
}

INT_PTR CALLBACK PhpTokenContainerPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PTOKEN_PAGE_CONTEXT tokenPageContext;
    PPHP_TOKEN_ADVANCED_CONTEXT context;

    tokenPageContext = PhpTokenPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!tokenPageContext)
        return FALSE;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PHP_TOKEN_ADVANCED_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }
    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE tokenHandle;
            BOOLEAN isLessPrivilegedAppContainer = FALSE;
            PPH_STRING tokenNamedObjectPathString = NULL;

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->ListView = PhGetListViewInterface(context->ListViewHandle);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 280, L"Value");
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddIListViewGroup(context->ListView, 0, L"General");
            PhAddIListViewGroup(context->ListView, 1, L"Properties");
            PhAddIListViewGroup(context->ListView, 2, L"Parent");
            PhAddIListViewGroup(context->ListView, 3, L"Package");
            PhAddIListViewGroup(context->ListView, 4, L"Profile");

            PhAddIListViewGroupItem(context->ListView, 0, MAXINT, L"Name", NULL);
            PhAddIListViewGroupItem(context->ListView, 0, MAXINT, L"Type", NULL);
            PhAddIListViewGroupItem(context->ListView, 0, MAXINT, L"SID", NULL);
            PhAddIListViewGroupItem(context->ListView, 1, MAXINT, L"Number", NULL);
            PhAddIListViewGroupItem(context->ListView, 1, MAXINT, L"LPAC", NULL);
            PhAddIListViewGroupItem(context->ListView, 1, MAXINT, L"Token object path", NULL);
            PhAddIListViewGroupItem(context->ListView, 2, MAXINT, L"Name", NULL);
            PhAddIListViewGroupItem(context->ListView, 2, MAXINT, L"SID", NULL);
            PhAddIListViewGroupItem(context->ListView, 3, MAXINT, L"Name", NULL);
            PhAddIListViewGroupItem(context->ListView, 3, MAXINT, L"Path", NULL);
            PhAddIListViewGroupItem(context->ListView, 4, MAXINT, L"Folder path", NULL);
            PhAddIListViewGroupItem(context->ListView, 4, MAXINT, L"Registry path", NULL);

            if (NT_SUCCESS(tokenPageContext->OpenObject(
                &tokenHandle,
                TOKEN_QUERY,
                tokenPageContext->Context // ProcessId
                )))
            {
                APPCONTAINER_SID_TYPE appContainerSidType = InvalidAppContainerSidType;
                PH_TOKEN_APPCONTAINER tokenAppContainer;
                PSID appContainerSidParent = NULL;
                PPH_STRING appContainerName = NULL;
                PPH_STRING appContainerSidString = NULL;
                ULONG appContainerNumber;
                PPH_STRING packageFullName;
                PPH_STRING packagePath;

                if (NT_SUCCESS(PhGetTokenAppContainerSid(tokenHandle, &tokenAppContainer)))
                {
                    if (RtlGetAppContainerSidType_Import())
                        RtlGetAppContainerSidType_Import()(tokenAppContainer.AppContainer.Sid, &appContainerSidType);
                    if (RtlGetAppContainerParent_Import())
                        RtlGetAppContainerParent_Import()(tokenAppContainer.AppContainer.Sid, &appContainerSidParent);

                    appContainerName = PhGetAppContainerName(tokenAppContainer.AppContainer.Sid);
                    appContainerSidString = PhSidToStringSid(tokenAppContainer.AppContainer.Sid);
                }

                if (appContainerName)
                {
                    PhSetIListViewSubItem(context->ListView, 0, 1, appContainerName->Buffer);
                    PhDereferenceObject(appContainerName);
                }

                switch (appContainerSidType)
                {
                case ChildAppContainerSidType:
                    PhSetIListViewSubItem(context->ListView, 1, 1, L"Child");
                    break;
                case ParentAppContainerSidType:
                    PhSetIListViewSubItem(context->ListView, 1, 1, L"Parent");
                    break;
                }

                if (appContainerSidString)
                {
                    PhSetIListViewSubItem(context->ListView, 2, 1, appContainerSidString->Buffer);
                    PhDereferenceObject(appContainerSidString);
                }

                if (!PhGetTokenIsFullTrustPackage(tokenHandle))
                {
                    if (NT_SUCCESS(PhGetTokenAppContainerNumber(tokenHandle, &appContainerNumber)))
                    {
                        WCHAR string[PH_INT64_STR_LEN_1] = L"Unknown";

                        PhPrintUInt32(string, appContainerNumber);
                        PhSetIListViewSubItem(context->ListView, 3, 1, string);
                    }

                    PhGetTokenIsLessPrivilegedAppContainer(tokenHandle, &isLessPrivilegedAppContainer);
                    PhSetIListViewSubItem(context->ListView, 4, 1, isLessPrivilegedAppContainer ? L"True" : L"False");
                }

                if (NT_SUCCESS(PhGetAppContainerNamedObjectPath(tokenHandle, NULL, FALSE, &tokenNamedObjectPathString)))
                {
                    PhSetIListViewSubItem(context->ListView, 5, 1, PhGetStringOrDefault(tokenNamedObjectPathString, L"Unknown"));
                    PhDereferenceObject(tokenNamedObjectPathString);
                }

                if (appContainerSidParent)
                {
                    if (appContainerName = PhGetAppContainerName(appContainerSidParent))
                    {
                        PhSetIListViewSubItem(context->ListView, 6, 1, appContainerName->Buffer);
                        PhDereferenceObject(appContainerName);
                    }

                    if (appContainerSidString = PhSidToStringSid(appContainerSidParent))
                    {
                        PhSetIListViewSubItem(context->ListView, 7, 1, appContainerSidString->Buffer);
                        PhDereferenceObject(appContainerSidString);
                    }

                    PhFreeSid(appContainerSidParent);
                }

                if (packageFullName = PhGetTokenPackageFullName(tokenHandle))
                {
                    PhSetIListViewSubItem(context->ListView, 8, 1, packageFullName->Buffer);

                    if (packagePath = PhGetPackagePath(packageFullName))
                    {
                        PhSetIListViewSubItem(context->ListView, 9, 1, packagePath->Buffer);
                        PhDereferenceObject(packagePath);
                    }

                    PhDereferenceObject(packageFullName);
                }

                tokenPageContext->CloseObject(tokenHandle, FALSE, tokenPageContext->Context);
            }

            tokenHandle = NULL;

            if (!NT_SUCCESS(tokenPageContext->OpenObject(
                &tokenHandle,
                TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                tokenPageContext->Context // ProcessId
                )))
            {
                if (!NT_SUCCESS(tokenPageContext->OpenObject(
                    &tokenHandle,
                    TOKEN_QUERY | TOKEN_IMPERSONATE,
                    tokenPageContext->Context // ProcessId
                    )))
                {
                    tokenPageContext->OpenObject(
                        &tokenHandle,
                        TOKEN_QUERY,
                        tokenPageContext->Context // ProcessId
                        );
                }
            }

            if (tokenHandle)
            {
                PH_TOKEN_APPCONTAINER tokenAppContainer;
                PPH_STRING appContainerFolderPath;
                PPH_STRING appContainerRegistryPath;

                if (NT_SUCCESS(PhGetTokenAppContainerSid(tokenHandle, &tokenAppContainer)))
                    appContainerFolderPath = PhpGetTokenAppContainerFolderPath(tokenHandle, tokenAppContainer.AppContainer.Sid);
                else
                    appContainerFolderPath = PhpGetTokenAppContainerFolderPath(tokenHandle, NULL);

                if (appContainerFolderPath)
                {
                    PhSetListViewSubItem(context->ListViewHandle, 10, 1, appContainerFolderPath->Buffer);
                    PhDereferenceObject(appContainerFolderPath);
                }

                if (appContainerRegistryPath = PhpGetTokenAppContainerRegistryPath(tokenHandle))
                {
                    PhSetListViewSubItem(context->ListViewHandle, 11, 1, appContainerRegistryPath->Buffer);
                    PhDereferenceObject(appContainerRegistryPath);
                }

                tokenPageContext->CloseObject(tokenHandle, FALSE, tokenPageContext->Context);
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    }

    return FALSE;
}

typedef enum _AppModelPolicy_Type
{
    AppModelPolicy_Type_LifecycleManager = 1,
    AppModelPolicy_Type_AppDataAccess = 2,
    AppModelPolicy_Type_WindowingModel = 3,
    AppModelPolicy_Type_DllSearchOrder = 4,
    AppModelPolicy_Type_Fusion = 5,
    AppModelPolicy_Type_NonWindowsCodecLoading = 6,
    AppModelPolicy_Type_ProcessEnd = 7,
    AppModelPolicy_Type_BeginThreadInit = 8,
    AppModelPolicy_Type_DeveloperInformation = 9,
    AppModelPolicy_Type_CreateFileAccess = 10,
    AppModelPolicy_Type_ImplicitPackageBreakaway_Internal = 11,
    AppModelPolicy_Type_ProcessActivationShim = 12,
    AppModelPolicy_Type_AppKnownToStateRepository = 13,
    AppModelPolicy_Type_AudioManagement = 14,
    AppModelPolicy_Type_PackageMayContainPublicComRegistrations = 15,
    AppModelPolicy_Type_PackageMayContainPrivateComRegistrations = 16,
    AppModelPolicy_Type_LaunchCreateProcessExtensions = 17,
    AppModelPolicy_Type_ClrCompat = 18,
    AppModelPolicy_Type_LoaderIgnoreAlteredSearchForRelativePath = 19,
    AppModelPolicy_Type_ImplicitlyActivateClassicAAAServersAsIU = 20,
    AppModelPolicy_Type_ComClassicCatalog = 21,
    AppModelPolicy_Type_ComUnmarshaling = 22,
    AppModelPolicy_Type_ComAppLaunchPerfEnhancements = 23,
    AppModelPolicy_Type_ComSecurityInitialization = 24,
    AppModelPolicy_Type_RoInitializeSingleThreadedBehavior = 25,
    AppModelPolicy_Type_ComDefaultExceptionHandling = 26,
    AppModelPolicy_Type_ComOopProxyAgility = 27,
    AppModelPolicy_Type_AppServiceLifetime = 28,
    AppModelPolicy_Type_WebPlatform = 29,
    AppModelPolicy_Type_WinInetStoragePartitioning = 30,
    AppModelPolicy_Type_IndexerProtocolHandlerHost = 31, // since RS2
    AppModelPolicy_Type_LoaderIncludeUserDirectories = 32,
    AppModelPolicy_Type_ConvertAppContainerToRestrictedAppContainer = 33,
    AppModelPolicy_Type_PackageMayContainPrivateMapiProvider = 34,
    AppModelPolicy_Type_AdminProcessPackageClaims = 35, // since RS3
    AppModelPolicy_Type_RegistryRedirectionBehavior = 36,
    AppModelPolicy_Type_BypassCreateProcessAppxExtension = 37,
    AppModelPolicy_Type_KnownFolderRedirection = 38,
    AppModelPolicy_Type_PrivateActivateAsPackageWinrtClasses = 39,
    AppModelPolicy_Type_AppPrivateFolderRedirection = 40,
    AppModelPolicy_Type_GlobalSystemAppDataAccess = 41,
    AppModelPolicy_Type_ConsoleHandleInheritance = 42, // since RS4
    AppModelPolicy_Type_ConsoleBufferAccess = 43,
    AppModelPolicy_Type_ConvertCallerTokenToUserTokenForDeployment = 44,
    AppModelPolicy_Type_ShellExecuteRetrieveIdentityFromCurrentProcess = 45, // since RS5
    AppModelPolicy_Type_CodeIntegritySigning = 46, // since 19H1
    AppModelPolicy_Type_PTCActivation = 47,
    AppModelPolicy_Type_ComIntraPackageRpcCall = 48, // since 20H1
    AppModelPolicy_Type_LoadUser32ShimOnWindowsCoreOS = 49,
    AppModelPolicy_Type_SecurityCapabilitiesOverride = 50,
    AppModelPolicy_Type_CurrentDirectoryOverride = 51,
    AppModelPolicy_Type_ComTokenMatchingForAAAServers = 52,
    AppModelPolicy_Type_UseOriginalFileNameInTokenFQBNAttribute = 53,
    AppModelPolicy_Type_LoaderIncludeAlternateForwarders = 54,
    AppModelPolicy_Type_PullPackageDependencyData = 55,
    AppModelPolicy_Type_AppInstancingErrorBehavior = 56, // since WIN11
    AppModelPolicy_Type_BackgroundTaskRegistrationType = 57,
    AppModelPolicy_Type_ModsPowerNotification = 58,
    AppModelPolicy_Type_DamRegistration = 59, // since 24H2
    AppModelPolicy_Type_Count = 59,
} AppModelPolicy_Type;

typedef enum _AppModelPolicy_PolicyValue
{
    AppModelPolicy_LifecycleManager_Unmanaged = 0x10000,
    AppModelPolicy_LifecycleManager_ManagedByPLM = 0x10001,
    AppModelPolicy_LifecycleManager_ManagedByEM = 0x10002,
    AppModelPolicy_AppDataAccess_Allowed = 0x20000,
    AppModelPolicy_AppDataAccess_Denied = 0x20001,
    AppModelPolicy_WindowingModel_Hwnd = 0x30000,
    AppModelPolicy_WindowingModel_CoreWindow = 0x30001,
    AppModelPolicy_WindowingModel_LegacyPhone = 0x30002,
    AppModelPolicy_WindowingModel_None = 0x30003,
    AppModelPolicy_DllSearchOrder_Traditional = 0x40000,
    AppModelPolicy_DllSearchOrder_PackageGraphBased = 0x40001,
    AppModelPolicy_Fusion_Full = 0x50000,
    AppModelPolicy_Fusion_Limited = 0x50001,
    AppModelPolicy_NonWindowsCodecLoading_Allowed = 0x60000,
    AppModelPolicy_NonWindowsCodecLoading_Denied = 0x60001,
    AppModelPolicy_ProcessEnd_TerminateProcess = 0x70000,
    AppModelPolicy_ProcessEnd_ExitProcess = 0x70001,
    AppModelPolicy_BeginThreadInit_RoInitialize = 0x80000,
    AppModelPolicy_BeginThreadInit_None = 0x80001,
    AppModelPolicy_DeveloperInformation_UI = 0x90000,
    AppModelPolicy_DeveloperInformation_None = 0x90001,
    AppModelPolicy_CreateFileAccess_Full = 0xa0000,
    AppModelPolicy_CreateFileAccess_Limited = 0xa0001,
    AppModelPolicy_ImplicitPackageBreakaway_Allowed = 0xb0000,
    AppModelPolicy_ImplicitPackageBreakaway_Denied = 0xb0001,
    AppModelPolicy_ImplicitPackageBreakaway_DeniedByApp = 0xb0002,
    AppModelPolicy_ProcessActivationShim_None = 0xc0000,
    AppModelPolicy_ProcessActivationShim_PackagedCWALauncher = 0xc0001,
    AppModelPolicy_AppKnownToStateRepository_Known = 0xd0000,
    AppModelPolicy_AppKnownToStateRepository_Unknown = 0xd0001,
    AppModelPolicy_AudioManagement_Unmanaged = 0xe0000,
    AppModelPolicy_AudioManagement_ManagedByPBM = 0xe0001,
    AppModelPolicy_PackageMayContainPublicComRegistrations_Yes = 0xf0000,
    AppModelPolicy_PackageMayContainPublicComRegistrations_No = 0xf0001,
    AppModelPolicy_PackageMayContainPrivateComRegistrations_None = 0x100000,
    AppModelPolicy_PackageMayContainPrivateComRegistrations_PrivateHive = 0x100001,
    AppModelPolicy_LaunchCreateProcessExtensions_None = 0x110000,
    AppModelPolicy_LaunchCreateProcessExtensions_RegisterWithPsm = 0x110001,
    AppModelPolicy_LaunchCreateProcessExtensions_RegisterWithDesktopAppX = 0x110002,
    AppModelPolicy_LaunchCreateProcessExtensions_RegisterWithDesktopAppXNoHeliumContainer = 0x110003,
    AppModelPolicy_ClrCompat_Others = 0x120000,
    AppModelPolicy_ClrCompat_ClassicDesktop = 0x120001,
    AppModelPolicy_ClrCompat_Universal = 0x120002,
    AppModelPolicy_ClrCompat_PackagedDesktop = 0x120003,
    AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_False = 0x130000,
    AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_True = 0x130001,
    AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_Yes = 0x140000,
    AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_No = 0x140001,
    AppModelPolicy_ComClassicCatalog_MachineHiveAndUserHive = 0x150000,
    AppModelPolicy_ComClassicCatalog_MachineHiveOnly = 0x150001,
    AppModelPolicy_ComUnmarshaling_ForceStrongUnmarshaling = 0x160000,
    AppModelPolicy_ComUnmarshaling_ApplicationManaged = 0x160001,
    AppModelPolicy_ComAppLaunchPerfEnhancements_Enabled = 0x170000,
    AppModelPolicy_ComAppLaunchPerfEnhancements_Disabled = 0x170001,
    AppModelPolicy_ComSecurityInitialization_ApplicationManaged = 0x180000,
    AppModelPolicy_ComSecurityInitialization_SystemManaged = 0x180001,
    AppModelPolicy_RoInitializeSingleThreadedBehavior_ASTA = 0x190000,
    AppModelPolicy_RoInitializeSingleThreadedBehavior_STA = 0x190001,
    AppModelPolicy_ComDefaultExceptionHandling_HandleAll = 0x1a0000,
    AppModelPolicy_ComDefaultExceptionHandling_HandleNone = 0x1a0001,
    AppModelPolicy_ComOopProxyAgility_Agile = 0x1b0000,
    AppModelPolicy_ComOopProxyAgility_NonAgile = 0x1b0001,
    AppModelPolicy_AppServiceLifetime_StandardTimeout = 0x1c0000,
    AppModelPolicy_AppServiceLifetime_ExtensibleTimeout = 0x1c0001,
    AppModelPolicy_AppServiceLifetime_ExtendedForSamePackage = 0x1c0002,
    AppModelPolicy_WebPlatform_Edge = 0x1d0000,
    AppModelPolicy_WebPlatform_Legacy = 0x1d0001,
    AppModelPolicy_WinInetStoragePartitioning_Isolated = 0x1e0000,
    AppModelPolicy_WinInetStoragePartitioning_SharedWithAppContainer = 0x1e0001,
    AppModelPolicy_IndexerProtocolHandlerHost_PerUser = 0x1f0000,
    AppModelPolicy_IndexerProtocolHandlerHost_PerApp = 0x1f0001,
    AppModelPolicy_LoaderIncludeUserDirectories_False = 0x200000,
    AppModelPolicy_LoaderIncludeUserDirectories_True = 0x200001,
    AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_False = 0x210000,
    AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_True = 0x210001,
    AppModelPolicy_PackageMayContainPrivateMapiProvider_None = 0x220000,
    AppModelPolicy_PackageMayContainPrivateMapiProvider_PrivateHive = 0x220001,
    AppModelPolicy_AdminProcessPackageClaims_None = 0x230000,
    AppModelPolicy_AdminProcessPackageClaims_Caller = 0x230001,
    AppModelPolicy_RegistryRedirectionBehavior_None = 0x240000,
    AppModelPolicy_RegistryRedirectionBehavior_CopyOnWrite = 0x240001,
    AppModelPolicy_BypassCreateProcessAppxExtension_False = 0x250000,
    AppModelPolicy_BypassCreateProcessAppxExtension_True = 0x250001,
    AppModelPolicy_KnownFolderRedirection_Isolated = 0x260000,
    AppModelPolicy_KnownFolderRedirection_RedirectToPackage = 0x260001,
    AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNone = 0x270000,
    AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowFullTrust = 0x270001,
    AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNonFullTrust = 0x270002,
    AppModelPolicy_AppPrivateFolderRedirection_None = 0x280000,
    AppModelPolicy_AppPrivateFolderRedirection_AppPrivate = 0x280001,
    AppModelPolicy_GlobalSystemAppDataAccess_Normal = 0x290000,
    AppModelPolicy_GlobalSystemAppDataAccess_Virtualized = 0x290001,
    AppModelPolicy_ConsoleHandleInheritance_ConsoleOnly = 0x2a0000,
    AppModelPolicy_ConsoleHandleInheritance_All = 0x2a0001,
    AppModelPolicy_ConsoleBufferAccess_RestrictedUnidirectional = 0x2b0000,
    AppModelPolicy_ConsoleBufferAccess_Unrestricted = 0x2b0001,
    AppModelPolicy_ConvertCallerTokenToUserTokenForDeployment_UserCallerToken = 0x2c0000,
    AppModelPolicy_ConvertCallerTokenToUserTokenForDeployment_ConvertTokenToUserToken = 0x2c0001,
    AppModelPolicy_ShellExecuteRetrieveIdentityFromCurrentProcess_False = 0x2d0000,
    AppModelPolicy_ShellExecuteRetrieveIdentityFromCurrentProcess_True = 0x2d0001,
    AppModelPolicy_CodeIntegritySigning_Default = 0x2e0000,
    AppModelPolicy_CodeIntegritySigning_OriginBased = 0x2e0001,
    AppModelPolicy_CodeIntegritySigning_OriginBasedForDev = 0x2e0002,
    AppModelPolicy_PTCActivation_Default = 0x2f0000,
    AppModelPolicy_PTCActivation_AllowActivationInBrokerForMediumILContainer = 0x2f0001,
    AppModelPolicy_Type_ComIntraPackageRpcCall_NoWake = 0x300000,
    AppModelPolicy_Type_ComIntraPackageRpcCall_Wake = 0x300001,
    AppModelPolicy_LoadUser32ShimOnWindowsCoreOS_True = 0x310000,
    AppModelPolicy_LoadUser32ShimOnWindowsCoreOS_False = 0x310001,
    AppModelPolicy_SecurityCapabilitiesOverride_None = 0x320000,
    AppModelPolicy_SecurityCapabilitiesOverride_PackageCapabilities = 0x320001,
    AppModelPolicy_CurrentDirectoryOverride_None = 0x330000,
    AppModelPolicy_CurrentDirectoryOverride_PackageInstallDirectory = 0x330001,
    AppModelPolicy_ComTokenMatchingForAAAServers_DontUseNtCompareTokens = 0x340000,
    AppModelPolicy_ComTokenMatchingForAAAServers_UseNtCompareTokens = 0x340001,
    AppModelPolicy_UseOriginalFileNameInTokenFQBNAttribute_False = 0x350000,
    AppModelPolicy_UseOriginalFileNameInTokenFQBNAttribute_True = 0x350001,
    AppModelPolicy_LoaderIncludeAlternateForwarders_False = 0x360000,
    AppModelPolicy_LoaderIncludeAlternateForwarders_True = 0x360001,
    AppModelPolicy_PullPackageDependencyData_False = 0x370000,
    AppModelPolicy_PullPackageDependencyData_True = 0x370001,
    AppModelPolicy_AppInstancingErrorBehavior_SuppressErrors = 0x380000,
    AppModelPolicy_AppInstancingErrorBehavior_RaiseErrors = 0x380001,
    AppModelPolicy_BackgroundTaskRegistrationType_Unsupported = 0x390000,
    AppModelPolicy_BackgroundTaskRegistrationType_Manifested = 0x390001,
    AppModelPolicy_BackgroundTaskRegistrationType_Win32Clsid = 0x390002,
    AppModelPolicy_Type_ModsPowerNotification_Disabled = 0x3a0000,
    AppModelPolicy_Type_ModsPowerNotification_Enabled = 0x3a0001,
    AppModelPolicy_Type_ModsPowerNotification_QueryDam = 0x3a0002,
    AppModelPolicy_Type_DamRegistration_DoNotRegister = 0x3B0000,
    AppModelPolicy_Type_DamRegistration_RegisterAtLaunch = 0x3B0001,
} AppModelPolicy_PolicyValue;

#define WM_PH_APPMODEL_SYMBOL_RESULT (WM_APP + 101)

NTSTATUS PhGetAppModelPolicy(
    _In_ HANDLE TokenHandle,
    _In_ AppModelPolicy_Type PolicyType,
    _Out_ AppModelPolicy_PolicyValue* PolicyValue
    )
{
    static NTSTATUS (NTAPI* GetAppModelPolicy_I)(
        _In_ HANDLE TokenHandle,
        _In_ AppModelPolicy_Type PolicyType,
        _Out_ AppModelPolicy_PolicyValue* PolicyValue
        ) = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_SYMBOL_PROVIDER symbolProvider;
        PH_SYMBOL_INFORMATION symbolInfo;
        PVOID baseAddress;
        ULONG sizeOfImage;
        PPH_STRING fileName;

        symbolProvider = PhCreateSymbolProvider(NULL);
        PhLoadSymbolProviderOptions(symbolProvider);

        if (PhGetLoaderEntryDataZ(L"kernelbase.dll", &baseAddress, &sizeOfImage, &fileName))
        {
            PhLoadModuleSymbolProvider(symbolProvider, fileName, baseAddress, sizeOfImage);
            PhDereferenceObject(fileName);
        }

        if (PhGetSymbolFromName(symbolProvider, L"GetAppModelPolicy", &symbolInfo))
        {
            if (NT_SUCCESS(PhGuardGrantSuppressedCallAccess(NtCurrentProcess(), symbolInfo.Address)))
            {
                GetAppModelPolicy_I = symbolInfo.Address;
            }
        }

        PhDereferenceObject(symbolProvider);

        PhEndInitOnce(&initOnce);
    }

    if (GetAppModelPolicy_I)
    {
        // GetAppModelPolicy doesn't perform range checks and will read out-of-bound
        // and return garbage if we ask it about a not-yet-supported policy type (diversenok)
        if ((PolicyType >= AppModelPolicy_Type_AppInstancingErrorBehavior && WindowsVersion < WINDOWS_11) ||
            (PolicyType >= AppModelPolicy_Type_ComIntraPackageRpcCall && WindowsVersion < WINDOWS_10_20H1) ||
            (PolicyType >= AppModelPolicy_Type_CodeIntegritySigning && WindowsVersion < WINDOWS_10_19H1) ||
            (PolicyType >= AppModelPolicy_Type_ShellExecuteRetrieveIdentityFromCurrentProcess && WindowsVersion < WINDOWS_10_RS5) ||
            (PolicyType >= AppModelPolicy_Type_ConsoleHandleInheritance && WindowsVersion < WINDOWS_10_RS4) ||
            (PolicyType >= AppModelPolicy_Type_AdminProcessPackageClaims && WindowsVersion < WINDOWS_10_RS3) ||
            (PolicyType >= AppModelPolicy_Type_IndexerProtocolHandlerHost && WindowsVersion < WINDOWS_10_RS2))
        {
            return STATUS_INVALID_INFO_CLASS;
        }

        return GetAppModelPolicy_I(TokenHandle, PolicyType, PolicyValue);
    }

    return STATUS_PROCEDURE_NOT_FOUND;
}

static NTSTATUS PhGetAppModelPolicySymbolDownloadThread(
    _In_ PVOID Context
    )
{
    AppModelPolicy_PolicyValue result;

    if (PhGetAppModelPolicy(
        PhGetOwnTokenAttributes().TokenHandle,
        AppModelPolicy_Type_DllSearchOrder,
        &result
        ) != STATUS_PROCEDURE_NOT_FOUND)
    {
        PostMessage(Context, WM_PH_APPMODEL_SYMBOL_RESULT, 0, TRUE);
    }
    else
    {
        PostMessage(Context, WM_PH_APPMODEL_SYMBOL_RESULT, 0, FALSE);
    }

    return STATUS_SUCCESS;
}

VOID PhEnumTokenAppModelPolicy(
    _In_ PTOKEN_PAGE_CONTEXT TokenPageContext
    )
{
    HANDLE TokenHandle;
    AppModelPolicy_PolicyValue result;

    if (!NT_SUCCESS(TokenPageContext->OpenObject(
        &TokenHandle,
        TOKEN_QUERY,
        TokenPageContext->Context // ProcessId
        )))
    {
        return;
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_LifecycleManager, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"LifecycleManager"));

        switch (result)
        {
        case AppModelPolicy_LifecycleManager_Unmanaged:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Unmanaged"));
            break;
        case AppModelPolicy_LifecycleManager_ManagedByPLM:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ManagedByPLM"));
            break;
        case AppModelPolicy_LifecycleManager_ManagedByEM:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ManagedByEM"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_AppDataAccess, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"AppDataAccess"));

        switch (result)
        {
        case AppModelPolicy_AppDataAccess_Allowed:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Allowed"));
            break;
        case AppModelPolicy_AppDataAccess_Denied:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Denied"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_WindowingModel, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"WindowingModel"));

        switch (result)
        {
        case AppModelPolicy_WindowingModel_Hwnd:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Hwnd"));
            break;
        case AppModelPolicy_WindowingModel_CoreWindow:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"CoreWindow"));
            break;
        case AppModelPolicy_WindowingModel_LegacyPhone:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"LegacyPhone"));
            break;
        case AppModelPolicy_WindowingModel_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_DllSearchOrder, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"DllSearchOrder"));

        switch (result)
        {
        case AppModelPolicy_DllSearchOrder_Traditional:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Traditional"));
            break;
        case AppModelPolicy_DllSearchOrder_PackageGraphBased:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"PackageGraphBased"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_Fusion, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"Fusion"));

        switch (result)
        {
        case AppModelPolicy_Fusion_Full:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Full"));
            break;
        case AppModelPolicy_Fusion_Limited:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Limited"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_NonWindowsCodecLoading, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"NonWindowsCodecLoading"));

        switch (result)
        {
        case AppModelPolicy_NonWindowsCodecLoading_Allowed:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Allowed"));
            break;
        case AppModelPolicy_NonWindowsCodecLoading_Denied:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Denied"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ProcessEnd, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ProcessEnd"));

        switch (result)
        {
        case AppModelPolicy_ProcessEnd_TerminateProcess:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"TerminateProcess"));
            break;
        case AppModelPolicy_ProcessEnd_ExitProcess:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ExitProcess"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_BeginThreadInit, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"BeginThreadInit"));

        switch (result)
        {
        case AppModelPolicy_BeginThreadInit_RoInitialize:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"RoInitialize"));
            break;
        case AppModelPolicy_BeginThreadInit_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_DeveloperInformation, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"DeveloperInformation"));

        switch (result)
        {
        case AppModelPolicy_DeveloperInformation_UI:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"UI"));
            break;
        case AppModelPolicy_DeveloperInformation_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_CreateFileAccess, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"CreateFileAccess"));

        switch (result)
        {
        case AppModelPolicy_CreateFileAccess_Full:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Full"));
            break;
        case AppModelPolicy_CreateFileAccess_Limited:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Limited"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ImplicitPackageBreakaway_Internal, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ImplicitPackageBreakaway"));

        switch (result)
        {
        case AppModelPolicy_ImplicitPackageBreakaway_Allowed:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Allowed"));
            break;
        case AppModelPolicy_ImplicitPackageBreakaway_Denied:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Denied"));
            break;
        case AppModelPolicy_ImplicitPackageBreakaway_DeniedByApp:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"DeniedByApp"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ProcessActivationShim, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ProcessActivationShim"));

        switch (result)
        {
        case AppModelPolicy_ProcessActivationShim_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        case AppModelPolicy_ProcessActivationShim_PackagedCWALauncher:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"PackagedCWALauncher"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_AppKnownToStateRepository, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"AppKnownToStateRepository"));

        switch (result)
        {
        case AppModelPolicy_AppKnownToStateRepository_Known:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Known"));
            break;
        case AppModelPolicy_AppKnownToStateRepository_Unknown:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Unknown"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_AudioManagement, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"AudioManagement"));

        switch (result)
        {
        case AppModelPolicy_AudioManagement_Unmanaged:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Unmanaged"));
            break;
        case AppModelPolicy_AudioManagement_ManagedByPBM:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ManagedByPBM"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_PackageMayContainPublicComRegistrations, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"PackageMayContainPublicComRegistrations"));

        switch (result)
        {
        case AppModelPolicy_PackageMayContainPublicComRegistrations_Yes:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Yes"));
            break;
        case AppModelPolicy_PackageMayContainPublicComRegistrations_No:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"No"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_PackageMayContainPrivateComRegistrations, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"PackageMayContainPrivateComRegistrations"));

        switch (result)
        {
        case AppModelPolicy_PackageMayContainPrivateComRegistrations_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        case AppModelPolicy_PackageMayContainPrivateComRegistrations_PrivateHive:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"PrivateHive"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_LaunchCreateProcessExtensions, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"LaunchCreateProcessExtensions"));

        switch (result)
        {
        case AppModelPolicy_LaunchCreateProcessExtensions_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        case AppModelPolicy_LaunchCreateProcessExtensions_RegisterWithPsm:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"RegisterWithPsm"));
            break;
        case AppModelPolicy_LaunchCreateProcessExtensions_RegisterWithDesktopAppX:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"RegisterWithDesktopAppX"));
            break;
        case AppModelPolicy_LaunchCreateProcessExtensions_RegisterWithDesktopAppXNoHeliumContainer:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"RegisterWithDesktopAppXNoHeliumContainer"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ClrCompat, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ClrCompat"));

        switch (result)
        {
        case AppModelPolicy_ClrCompat_Others:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Others"));
            break;
        case AppModelPolicy_ClrCompat_ClassicDesktop:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ClassicDesktop"));
            break;
        case AppModelPolicy_ClrCompat_Universal:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Universal"));
            break;
        case AppModelPolicy_ClrCompat_PackagedDesktop:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"PackagedDesktop"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_LoaderIgnoreAlteredSearchForRelativePath, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"IgnoreAlteredSearchForRelativePath"));

        switch (result)
        {
        case AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_False:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"False"));
            break;
        case AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_True:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"True"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ImplicitlyActivateClassicAAAServersAsIU, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ImplicitlyActivateClassicAAAServersAsIU"));

        switch (result)
        {
        case AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_Yes:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Yes"));
            break;
        case AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_No:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"No"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ComClassicCatalog, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ComClassicCatalog"));

        switch (result)
        {
        case AppModelPolicy_ComClassicCatalog_MachineHiveAndUserHive:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"MachineHiveAndUserHive"));
            break;
        case AppModelPolicy_ComClassicCatalog_MachineHiveOnly:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"MachineHiveOnly"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ComUnmarshaling, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ComUnmarshaling"));

        switch (result)
        {
        case AppModelPolicy_ComUnmarshaling_ForceStrongUnmarshaling:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ForceStrongUnmarshaling"));
            break;
        case AppModelPolicy_ComUnmarshaling_ApplicationManaged:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ApplicationManaged"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ComAppLaunchPerfEnhancements, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ComAppLaunchPerfEnhancements"));

        switch (result)
        {
        case AppModelPolicy_ComAppLaunchPerfEnhancements_Enabled:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Enabled"));
            break;
        case AppModelPolicy_ComAppLaunchPerfEnhancements_Disabled:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Disabled"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ComSecurityInitialization, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ComSecurityInitialization"));

        switch (result)
        {
        case AppModelPolicy_ComSecurityInitialization_ApplicationManaged:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ApplicationManaged"));
            break;
        case AppModelPolicy_ComSecurityInitialization_SystemManaged:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"SystemManaged"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_RoInitializeSingleThreadedBehavior, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"RoInitializeSingleThreadedBehavior"));

        switch (result)
        {
        case AppModelPolicy_RoInitializeSingleThreadedBehavior_ASTA:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ASTA"));
            break;
        case AppModelPolicy_RoInitializeSingleThreadedBehavior_STA:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"STA"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ComDefaultExceptionHandling, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ComDefaultExceptionHandling"));

        switch (result)
        {
        case AppModelPolicy_ComDefaultExceptionHandling_HandleAll:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"All"));
            break;
        case AppModelPolicy_ComDefaultExceptionHandling_HandleNone:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ComOopProxyAgility, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ComOopProxyAgility"));

        switch (result)
        {
        case AppModelPolicy_ComOopProxyAgility_Agile:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Agile"));
            break;
        case AppModelPolicy_ComOopProxyAgility_NonAgile:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"NonAgile"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_AppServiceLifetime, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"AppServiceLifetime"));

        switch (result)
        {
        case AppModelPolicy_AppServiceLifetime_StandardTimeout:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"StandardTimeout"));
            break;
        case AppModelPolicy_AppServiceLifetime_ExtensibleTimeout:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ExtensibleTimeout"));
            break;
        case AppModelPolicy_AppServiceLifetime_ExtendedForSamePackage:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ExtendedForSamePackage"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_WebPlatform, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"WebPlatform"));

        switch (result)
        {
        case AppModelPolicy_WebPlatform_Edge:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Edge"));
            break;
        case AppModelPolicy_WebPlatform_Legacy:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Legacy"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_WinInetStoragePartitioning, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"WinInetStoragePartitioning"));

        switch (result)
        {
        case AppModelPolicy_WinInetStoragePartitioning_Isolated:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Isolated"));
            break;
        case AppModelPolicy_WinInetStoragePartitioning_SharedWithAppContainer:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"SharedWithAppContainer"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_IndexerProtocolHandlerHost, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"IndexerProtocolHandlerHost"));

        switch (result)
        {
        case AppModelPolicy_IndexerProtocolHandlerHost_PerUser:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"PerUser"));
            break;
        case AppModelPolicy_IndexerProtocolHandlerHost_PerApp:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"PerApp"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_LoaderIncludeUserDirectories, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"LoaderIncludeUserDirectories"));

        switch (result)
        {
        case AppModelPolicy_LoaderIncludeUserDirectories_False:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"False"));
            break;
        case AppModelPolicy_LoaderIncludeUserDirectories_True:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"True"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ConvertAppContainerToRestrictedAppContainer, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ConvertAppContainerToRestrictedAppContainer"));

        switch (result)
        {
        case AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_False:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"False"));
            break;
        case AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_True:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"True"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_PackageMayContainPrivateMapiProvider, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"PackageMayContainPrivateMapiProvider"));

        switch (result)
        {
        case AppModelPolicy_PackageMayContainPrivateMapiProvider_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        case AppModelPolicy_PackageMayContainPrivateMapiProvider_PrivateHive:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"PrivateHive"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_AdminProcessPackageClaims, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"AdminProcessPackageClaims"));

        switch (result)
        {
        case AppModelPolicy_AdminProcessPackageClaims_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        case AppModelPolicy_AdminProcessPackageClaims_Caller:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Caller"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_RegistryRedirectionBehavior, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"RegistryRedirectionBehavior"));

        switch (result)
        {
        case AppModelPolicy_RegistryRedirectionBehavior_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        case AppModelPolicy_RegistryRedirectionBehavior_CopyOnWrite:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"CopyOnWrite"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_BypassCreateProcessAppxExtension, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"BypassCreateProcessAppxExtension"));

        switch (result)
        {
        case AppModelPolicy_BypassCreateProcessAppxExtension_False:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"False"));
            break;
        case AppModelPolicy_BypassCreateProcessAppxExtension_True:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"True"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_KnownFolderRedirection, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"KnownFolderRedirection"));

        switch (result)
        {
        case AppModelPolicy_KnownFolderRedirection_Isolated:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Isolated"));
            break;
        case AppModelPolicy_KnownFolderRedirection_RedirectToPackage:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"RedirectToPackage"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_PrivateActivateAsPackageWinrtClasses, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"PrivateActivateAsPackageWinrtClasses"));

        switch (result)
        {
        case AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNone:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"AllowNone"));
            break;
        case AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowFullTrust:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"AllowFullTrust"));
            break;
        case AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNonFullTrust:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"AllowNonFullTrust"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_AppPrivateFolderRedirection, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"AppPrivateFolderRedirection"));

        switch (result)
        {
        case AppModelPolicy_AppPrivateFolderRedirection_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        case AppModelPolicy_AppPrivateFolderRedirection_AppPrivate:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"AppPrivate"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_GlobalSystemAppDataAccess, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"GlobalSystemAppDataAccess"));

        switch (result)
        {
        case AppModelPolicy_GlobalSystemAppDataAccess_Normal:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Normal"));
            break;
        case AppModelPolicy_GlobalSystemAppDataAccess_Virtualized:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Virtualized"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ConsoleHandleInheritance, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ConsoleHandleInheritance"));

        switch (result)
        {
        case AppModelPolicy_ConsoleHandleInheritance_ConsoleOnly:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ConsoleOnly"));
            break;
        case AppModelPolicy_ConsoleHandleInheritance_All:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"All"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ConsoleBufferAccess, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ConsoleBufferAccess"));

        switch (result)
        {
        case AppModelPolicy_ConsoleBufferAccess_RestrictedUnidirectional:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"RestrictedUnidirectional"));
            break;
        case AppModelPolicy_ConsoleBufferAccess_Unrestricted:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Unrestricted"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ConvertCallerTokenToUserTokenForDeployment, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ConvertCallerTokenToUserTokenForDeployment"));

        switch (result)
        {
        case AppModelPolicy_ConvertCallerTokenToUserTokenForDeployment_UserCallerToken:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"UserCallerToken"));
            break;
        case AppModelPolicy_ConvertCallerTokenToUserTokenForDeployment_ConvertTokenToUserToken:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"ConvertTokenToUserToken"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ShellExecuteRetrieveIdentityFromCurrentProcess, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ShellExecuteRetrieveIdentityFromCurrentProcess"));

        switch (result)
        {
        case AppModelPolicy_ShellExecuteRetrieveIdentityFromCurrentProcess_False:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"False"));
            break;
        case AppModelPolicy_ShellExecuteRetrieveIdentityFromCurrentProcess_True:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"True"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_CodeIntegritySigning, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"CodeIntegritySigning"));

        switch (result)
        {
        case AppModelPolicy_CodeIntegritySigning_Default:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Default"));
            break;
        case AppModelPolicy_CodeIntegritySigning_OriginBased:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"OriginBased"));
            break;
        case AppModelPolicy_CodeIntegritySigning_OriginBasedForDev:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"OriginBasedForDev"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_PTCActivation, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"PTCActivation"));

        switch (result)
        {
        case AppModelPolicy_PTCActivation_Default:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Default"));
            break;
        case AppModelPolicy_PTCActivation_AllowActivationInBrokerForMediumILContainer:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"AllowActivationInBrokerForMediumIL"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ComIntraPackageRpcCall, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ComIntraPackageRpcCall"));

        switch (result)
        {
        case AppModelPolicy_Type_ComIntraPackageRpcCall_NoWake:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"NoWake"));
            break;
        case AppModelPolicy_Type_ComIntraPackageRpcCall_Wake:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Wake"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_LoadUser32ShimOnWindowsCoreOS, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"LoadUser32ShimOnWindowsCoreOS"));

        switch (result)
        {
        case AppModelPolicy_LoadUser32ShimOnWindowsCoreOS_True:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"True"));
            break;
        case AppModelPolicy_LoadUser32ShimOnWindowsCoreOS_False:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"False"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_SecurityCapabilitiesOverride, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"SecurityCapabilitiesOverride"));

        switch (result)
        {
        case AppModelPolicy_SecurityCapabilitiesOverride_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        case AppModelPolicy_SecurityCapabilitiesOverride_PackageCapabilities:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"PackageCapabilities"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_CurrentDirectoryOverride, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"CurrentDirectoryOverride"));

        switch (result)
        {
        case AppModelPolicy_CurrentDirectoryOverride_None:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"None"));
            break;
        case AppModelPolicy_CurrentDirectoryOverride_PackageInstallDirectory:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"PackageInstallDirectory"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ComTokenMatchingForAAAServers, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ComTokenMatching"));

        switch (result)
        {
        case AppModelPolicy_ComTokenMatchingForAAAServers_DontUseNtCompareTokens:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"DontUseNtCompareTokens"));
            break;
        case AppModelPolicy_ComTokenMatchingForAAAServers_UseNtCompareTokens:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"UseNtCompareTokens"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_UseOriginalFileNameInTokenFQBNAttribute, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"UseOriginalFileNameInTokenFQBN"));

        switch (result)
        {
        case AppModelPolicy_UseOriginalFileNameInTokenFQBNAttribute_False:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"False"));
            break;
        case AppModelPolicy_UseOriginalFileNameInTokenFQBNAttribute_True:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"True"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_LoaderIncludeAlternateForwarders, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"LoaderIncludeAlternateForwarders"));

        switch (result)
        {
        case AppModelPolicy_LoaderIncludeAlternateForwarders_False:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"False"));
            break;
        case AppModelPolicy_LoaderIncludeAlternateForwarders_True:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"True"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_PullPackageDependencyData, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"PullPackageDependencyData"));

        switch (result)
        {
        case AppModelPolicy_PullPackageDependencyData_False:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"False"));
            break;
        case AppModelPolicy_PullPackageDependencyData_True:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"True"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_AppInstancingErrorBehavior, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"AppInstancingErrorBehavior"));

        switch (result)
        {
        case AppModelPolicy_AppInstancingErrorBehavior_SuppressErrors:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"SuppressErrors"));
            break;
        case AppModelPolicy_AppInstancingErrorBehavior_RaiseErrors:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"RaiseErrors"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_BackgroundTaskRegistrationType, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"BackgroundTaskRegistrationType"));

        switch (result)
        {
        case AppModelPolicy_BackgroundTaskRegistrationType_Unsupported:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Unsupported"));
            break;
        case AppModelPolicy_BackgroundTaskRegistrationType_Manifested:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Manifested"));
            break;
        case AppModelPolicy_BackgroundTaskRegistrationType_Win32Clsid:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Win32Clsid"));
            break;
        }
    }

    if (NT_SUCCESS(PhGetAppModelPolicy(TokenHandle, AppModelPolicy_Type_ModsPowerNotification, &result)))
    {
        PPH_TOKEN_ATTRIBUTE_NODE node = PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, NULL, PhCreateString(L"ModsPowerNotification"));

        switch (result)
        {
        case AppModelPolicy_Type_ModsPowerNotification_Disabled:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Disabled"));
            break;
        case AppModelPolicy_Type_ModsPowerNotification_Enabled:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"Enabled"));
            break;
        case AppModelPolicy_Type_ModsPowerNotification_QueryDam:
            PhpAddAttributeNode(&TokenPageContext->AppPolicyTreeContext, node, PhCreateString(L"QueryDam"));
            break;
        }
    }

    TokenPageContext->CloseObject(TokenHandle, FALSE, TokenPageContext->Context);
}

#define SORT_FUNCTION(Column) PhpAppPolicyTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpAppPolicyTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_TOKEN_ATTRIBUTE_NODE node1 = *(PPH_TOKEN_ATTRIBUTE_NODE*)_elem1; \
    PPH_TOKEN_ATTRIBUTE_NODE node2 = *(PPH_TOKEN_ATTRIBUTE_NODE*)_elem2; \
    LONG sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, ((PPH_TOKEN_ATTRIBUTE_TREE_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNull(node1->Text, node2->Text, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Value)
{
    sortResult = PhCompareStringWithNull(node1->Text, node2->Text, TRUE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpAppPolicyTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_TOKEN_ATTRIBUTE_TREE_CONTEXT context = Context;
    PPH_TOKEN_ATTRIBUTE_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPH_TOKEN_ATTRIBUTE_NODE)getChildren->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
            {
                if (!node)
                {
                    getChildren->Children = (PPH_TREENEW_NODE*)context->RootList->Items;
                    getChildren->NumberOfChildren = context->RootList->Count;
                }
                else
                {
                    getChildren->Children = (PPH_TREENEW_NODE*)node->Children->Items;
                    getChildren->NumberOfChildren = node->Children->Count;
                }
            }
            else
            {
                if (!node)
                {
                    static PVOID sortFunctions[] =
                    {
                        SORT_FUNCTION(Name),
                        SORT_FUNCTION(Value)
                    };
                    int (__cdecl* sortFunction)(void*, const void*, const void*);

                    static_assert(RTL_NUMBER_OF(sortFunctions) == 2, "SortFunctions must equal maximum.");

                    if (context->TreeNewSortColumn < 2)
                        sortFunction = sortFunctions[context->TreeNewSortColumn];
                    else
                        sortFunction = NULL;

                    if (sortFunction)
                    {
                        qsort_s(context->RootList->Items, context->RootList->Count, sizeof(PVOID), sortFunction, context);
                    }

                    getChildren->Children = (PPH_TREENEW_NODE*)context->RootList->Items;
                    getChildren->NumberOfChildren = context->RootList->Count;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            node = (PPH_TOKEN_ATTRIBUTE_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            node = (PPH_TOKEN_ATTRIBUTE_NODE)getCellText->Node;

            if (getCellText->Id == 0)
                getCellText->Text = PhGetStringRef(node->Text);
            else if (getCellText->Id == 1)
            {
                if (PhIsNullOrEmptyString(node->Value))
                {
                    PH_STRING_BUILDER stringBuilder;

                    PhInitializeStringBuilder(&stringBuilder, 64);

                    for (ULONG i = 0; i < node->Children->Count; i++)
                    {
                        PhAppendStringBuilder(&stringBuilder, &((PPH_TOKEN_ATTRIBUTE_NODE)node->Children->Items[i])->Text->sr);
                        PhAppendStringBuilder2(&stringBuilder, L", ");
                    }

                    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
                        PhRemoveEndStringBuilder(&stringBuilder, 2);

                    node->Value = PhFinalStringBuilderString(&stringBuilder);
                }

                getCellText->Text = PhGetStringRef(node->Value);
            }
            else
                return FALSE;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->TreeNewSortColumn = sorting->SortColumn;
            context->TreeNewSortOrder = sorting->SortOrder;

            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
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
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(context->WindowHandle, WM_CONTEXTMENU, 0, (LPARAM)contextMenuEvent);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

static PH_STRINGREF PhAppPolicyLoadingText = PH_STRINGREF_INIT(L"Initializing kernelbase symbols...");
static PH_STRINGREF PhAppPolicyEmptyText = PH_STRINGREF_INIT(L"There are no policies to display.");

INT_PTR CALLBACK PhpTokenAppPolicyPageProc(
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
            tokenPageContext->AppPolicyTreeContext.WindowHandle = hwndDlg;
            tokenPageContext->AppPolicyTreeContext.NodeList = PhCreateList(10);
            tokenPageContext->AppPolicyTreeContext.RootList = PhCreateList(10);

            PhSetControlTheme(tnHandle, L"explorer");
            TreeNew_SetRedraw(tnHandle, FALSE);
            TreeNew_SetEmptyText(tnHandle, &PhAppPolicyLoadingText, 0);
            TreeNew_SetCallback(tnHandle, PhpAppPolicyTreeNewCallback, &tokenPageContext->AppPolicyTreeContext);
            PhAddTreeNewColumnEx2(tnHandle, 0, TRUE, L"Policy", 220, PH_ALIGN_LEFT, 0, 0, TN_COLUMN_FLAG_NODPISCALEONADD);
            PhAddTreeNewColumnEx2(tnHandle, 1, TRUE, L"Value", 150, PH_ALIGN_LEFT, 1, 0, TN_COLUMN_FLAG_NODPISCALEONADD);
            TreeNew_SetTriState(tnHandle, TRUE);
            TreeNew_SetRedraw(tnHandle, TRUE);

            PhCreateThread2(PhGetAppModelPolicySymbolDownloadThread, hwndDlg);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhpDeleteAttributeTreeContext(&tokenPageContext->AppPolicyTreeContext);
        }
        break;
    case WM_PH_APPMODEL_SYMBOL_RESULT:
        {
            BOOLEAN result = (BOOLEAN)lParam;

            if (result)
            {
                TreeNew_SetEmptyText(tnHandle, &PhAppPolicyEmptyText, 0);

                TreeNew_SetRedraw(tnHandle, FALSE);
                PhEnumTokenAppModelPolicy(tokenPageContext);
                TreeNew_NodesStructured(tnHandle);
                TreeNew_SetRedraw(tnHandle, TRUE);
            }
            else
            {
                TreeNew_SetEmptyText(tnHandle, &PhAppPolicyEmptyText, 0);
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPH_TOKEN_ATTRIBUTE_NODE* attributeObjectNodes = NULL;
            ULONG numberOfAttributeObjectNodes = 0;

            if (!PhpGetSelectedAttributeTreeNodes(&tokenPageContext->AppPolicyTreeContext, &attributeObjectNodes, &numberOfAttributeObjectNodes))
                break;

            if (numberOfAttributeObjectNodes != 0)
            {
                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"Copy", NULL, NULL), ULONG_MAX);
                PhInsertCopyCellEMenuItem(menu, IDC_COPY, tnHandle, contextMenuEvent->Column);

                selectedItem = PhShowEMenu(
                    menu,
                    hwndDlg,
                    PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    contextMenuEvent->Location.x,
                    contextMenuEvent->Location.y
                    );

                if (selectedItem && selectedItem->Id != ULONG_MAX)
                {
                    if (!PhHandleCopyCellEMenuItem(selectedItem))
                    {
                        switch (selectedItem->Id)
                        {
                        case IDC_COPY:
                            {
                                PPH_STRING text;

                                text = PhGetTreeNewText(tnHandle, 0);
                                PhSetClipboardString(tnHandle, &text->sr);
                                PhDereferenceObject(text);
                            }
                            break;
                        }
                    }
                }

                PhDestroyEMenu(menu);
            }

            PhFree(attributeObjectNodes);
        }
        break;
    }

    return FALSE;
}
