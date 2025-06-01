/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2025
 *
 */

#include <phapp.h>
#include <colmgr.h>
#include <cpysave.h>
#include <emenu.h>
#include <lsasup.h>
#include <mapldr.h>
#include <secedit.h>
#include <settings.h>

static NTSTATUS (NTAPI* LsaFreeReturnBuffer_I)(
    _In_ PVOID Buffer
    ) = NULL;

static NTSTATUS (NTAPI* LsaEnumerateLogonSessions_I)(
    _Out_ PULONG LogonSessionCount,
    _Out_ PLUID* LogonSessionList
    ) = NULL;

static NTSTATUS (NTAPI* LsaGetLogonSessionData_I)(
    _In_ PLUID LogonId,
    _Out_ PSECURITY_LOGON_SESSION_DATA* ppLogonSessionData
    );

//
// Values for UserFlags. (ntscapi.h)
//
#define LOGON_GUEST                 0x01
#define LOGON_NOENCRYPTION          0x02
#define LOGON_CACHED_ACCOUNT        0x04
#define LOGON_USED_LM_PASSWORD      0x08
#define LOGON_EXTRA_SIDS            0x20
#define LOGON_SUBAUTH_SESSION_KEY   0x40
#define LOGON_SERVER_TRUST_ACCOUNT  0x80
#define LOGON_NTLMV2_ENABLED        0x100       // says DC understands NTLMv2
#define LOGON_RESOURCE_GROUPS       0x200
#define LOGON_PROFILE_PATH_RETURNED 0x400
#define LOGON_NT_V2                 0x800   // NT response was used for validation
#define LOGON_LM_V2                 0x1000  // LM response was used for validation
#define LOGON_NTLM_V2               0x2000  // LM response was used to authenticate but NT response was used to derive the session key
#define LOGON_OPTIMIZED             0x4000  // this is an optimized logon
#define LOGON_WINLOGON              0x8000  // the logon session was created for winlogon
#define LOGON_PKINIT               0x10000  // Kerberos PKINIT extension was used to authenticate the user
#define LOGON_NO_OPTIMIZED         0x20000  // optimized logon has been disabled for this account
#define LOGON_NO_ELEVATION         0x40000  // Do not allow elevation for this logon
#define LOGON_MANAGED_SERVICE      0x80000  // Managed service account

PPH_STRING PhpFormatUserFlags(
    _In_ ULONG UserFlags
    )
{
#define PH_LSA_USER_FLAG(x, n)            { TEXT(#x), x, FALSE, FALSE, n }
    static const PH_ACCESS_ENTRY userFlags[] =
    {
        PH_LSA_USER_FLAG(LOGON_GUEST, L"Guest"),
        PH_LSA_USER_FLAG(LOGON_NOENCRYPTION, L"No encryption"),
        PH_LSA_USER_FLAG(LOGON_CACHED_ACCOUNT, L"Cached account"),
        PH_LSA_USER_FLAG(LOGON_USED_LM_PASSWORD, L"Used LM password"),
        PH_LSA_USER_FLAG(LOGON_EXTRA_SIDS, L"Extra SIDs"),
        PH_LSA_USER_FLAG(LOGON_SUBAUTH_SESSION_KEY, L"Subauth session key"),
        PH_LSA_USER_FLAG(LOGON_SERVER_TRUST_ACCOUNT, L"Server trust account"),
        PH_LSA_USER_FLAG(LOGON_NTLMV2_ENABLED, L"NTLMv2 enabled"),
        PH_LSA_USER_FLAG(LOGON_RESOURCE_GROUPS, L"Resource groups"),
        PH_LSA_USER_FLAG(LOGON_PROFILE_PATH_RETURNED, L"Profile path returned"),
        PH_LSA_USER_FLAG(LOGON_NT_V2, L"NTv2"),
        PH_LSA_USER_FLAG(LOGON_LM_V2, L"LMv2"),
        PH_LSA_USER_FLAG(LOGON_NTLM_V2, L"NTLMv2"),
        PH_LSA_USER_FLAG(LOGON_OPTIMIZED, L"Optimized"),
        PH_LSA_USER_FLAG(LOGON_WINLOGON, L"WinLogon created"),
        PH_LSA_USER_FLAG(LOGON_PKINIT, L"Keberos"),
        PH_LSA_USER_FLAG(LOGON_NO_OPTIMIZED, L"Not optimized"),
        PH_LSA_USER_FLAG(LOGON_NO_ELEVATION, L"No elevation"),
        PH_LSA_USER_FLAG(LOGON_MANAGED_SERVICE, L"Managed service"),
    };

    PH_FORMAT format[4];
    PPH_STRING string;

    string = PhGetAccessString(UserFlags, (PPH_ACCESS_ENTRY)userFlags, RTL_NUMBER_OF(userFlags));
    PhInitFormatSR(&format[0], string->sr);
    PhInitFormatS(&format[1], L" (0x");
    PhInitFormatX(&format[2], UserFlags);
    PhInitFormatS(&format[3], L")");

    PhMoveReference(&string, PhFormat(format, 4, 10));

    return string;
}

typedef enum _PH_USER_LIST_COLUMN
{
    PH_USER_LIST_COLUMN_LOGON_ID,
    PH_USER_LIST_COLUMN_USER_NAME,
    PH_USER_LIST_COLUMN_LOGON_DOMAIN,
    PH_USER_LIST_COLUMN_AUTHENTICATION_PACKAGE,
    PH_USER_LIST_COLUMN_LOGON_TYPE,
    PH_USER_LIST_COLUMN_SESSION_ID,
    PH_USER_LIST_COLUMN_SID,
    PH_USER_LIST_COLUMN_LOGON_TIME,
    PH_USER_LIST_COLUMN_LOGON_SERVER,
    PH_USER_LIST_COLUMN_DNS_DOMAIN_NAME,
    PH_USER_LIST_COLUMN_USER_PRINCIPAL_NAME,
    PH_USER_LIST_COLUMN_USER_FLAGS,
    PH_USER_LIST_COLUMN_FAILED_ATTEMPTS_SINCE_LAST_SUCCESSFUL_LOGON,
    PH_USER_LIST_COLUMN_LAST_SUCCESSFUL_LOGON,
    PH_USER_LIST_COLUMN_LAST_FAILED_LOGON,
    PH_USER_LIST_COLUMN_LOGON_SCRIPT,
    PH_USER_LIST_COLUMN_PROFILE_PATH,
    PH_USER_LIST_COLUMN_HOME_DIRECTORY,
    PH_USER_LIST_COLUMN_HOME_DIRECTORY_DRIVE,
    PH_USER_LIST_COLUMN_LOGOFF_TIME,
    PH_USER_LIST_COLUMN_KICKOFF_TIME,
    PH_USER_LIST_COLUMN_PASSWORD_LAST_SET,
    PH_USER_LIST_COLUMN_PASSWORD_CAN_CHANGE,
    PH_USER_LIST_COLUMN_PASSWORD_MUST_CHANGE,
    PH_USER_LIST_COLUMN_MAXIMUM,
} PH_USER_LIST_COLUMN;

typedef struct _PH_USER_NODE
{
    PH_TREENEW_NODE Node;

    LUID LogonId;
    PPH_STRING UserName;
    PPH_STRING LogonDomain;
    PPH_STRING AuthenticationPackage;
    SECURITY_LOGON_TYPE LogonType;
    ULONG SessionId;
    PPH_STRING Sid;
    LARGE_INTEGER LogonTime;
    PPH_STRING LogonServer;
    PPH_STRING DnsDomainName;
    PPH_STRING UserPrincipalName;
    ULONG UserFlags;
    ULONG FailedAttemptsSinceLastSuccessfulLogon;
    LARGE_INTEGER LastSuccessfulLogon;
    LARGE_INTEGER LastFailedLogon;
    PPH_STRING LogonScript;
    PPH_STRING ProfilePath;
    PPH_STRING HomeDirectory;
    PPH_STRING HomeDirectoryDrive;
    LARGE_INTEGER LogoffTime;
    LARGE_INTEGER KickOffTime;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER PasswordCanChange;
    LARGE_INTEGER PasswordMustChange;

    WCHAR LogonIdText[PH_PTR_STR_LEN_1];
    PPH_STRING SessionIdText;
    PPH_STRING LogonTimeText;
    PPH_STRING UserFlagsText;
    PPH_STRING FailedAttemptsSinceLastSuccessfulLogonText;
    PPH_STRING LastSuccessfulLogonText;
    PPH_STRING LastFailedLogonText;
    PPH_STRING LogoffTimeText;
    PPH_STRING KickOffTimeText;
    PPH_STRING PasswordLastSetText;
    PPH_STRING PasswordCanChangeText;
    PPH_STRING PasswordMustChangeText;

    PH_STRINGREF TextCache[PH_USER_LIST_COLUMN_MAXIMUM];
} PH_USER_NODE, *PPH_USER_NODE;

typedef struct _PH_USER_LIST_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    HWND MessageHandle;
    HWND SearchWindowHandle;
    ULONG_PTR SearchMatchHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_TN_FILTER_SUPPORT FilterSupport;
    RECT MinimumSize;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_LIST NodeList;
} PH_USER_LIST_CONTEXT, *PPH_USER_LIST_CONTEXT;

VOID PhpDeleteUserNode(
    _In_ PPH_USER_NODE User
    )
{
    PhClearReference(&User->UserName);
    PhClearReference(&User->LogonDomain);
    PhClearReference(&User->AuthenticationPackage);
    PhClearReference(&User->Sid);
    PhClearReference(&User->LogonServer);
    PhClearReference(&User->DnsDomainName);
    PhClearReference(&User->UserPrincipalName);
    PhClearReference(&User->LogonScript);
    PhClearReference(&User->ProfilePath);
    PhClearReference(&User->HomeDirectory);
    PhClearReference(&User->HomeDirectoryDrive);

    PhClearReference(&User->SessionIdText);
    PhClearReference(&User->LogonTimeText);
    PhClearReference(&User->UserFlagsText);
    PhClearReference(&User->FailedAttemptsSinceLastSuccessfulLogonText);
    PhClearReference(&User->LastSuccessfulLogonText);
    PhClearReference(&User->LastFailedLogonText);
    PhClearReference(&User->LogoffTimeText);
    PhClearReference(&User->KickOffTimeText);
    PhClearReference(&User->PasswordLastSetText);
    PhClearReference(&User->PasswordCanChangeText);
    PhClearReference(&User->PasswordMustChangeText);
}

PPH_USER_NODE PhpCreateUserNode(
    _In_ PPH_USER_LIST_CONTEXT Context,
    _In_ PSECURITY_LOGON_SESSION_DATA LogonSessionData
    )
{
    PPH_USER_NODE user;

    user = PhAllocateZero(sizeof(PH_USER_NODE));

    PhInitializeTreeNewNode(&user->Node);
    memset(user->TextCache, 0, sizeof(PH_STRINGREF) * PH_USER_LIST_COLUMN_MAXIMUM);
    user->Node.TextCache = user->TextCache;
    user->Node.TextCacheSize = PH_USER_LIST_COLUMN_MAXIMUM;

    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, LogonId))
        user->LogonId = LogonSessionData->LogonId;
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, UserName))
        user->UserName = PhCreateStringFromUnicodeString(&LogonSessionData->UserName);
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, LogonDomain))
        user->LogonDomain = PhCreateStringFromUnicodeString(&LogonSessionData->LogonDomain);
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, AuthenticationPackage))
        user->AuthenticationPackage = PhCreateStringFromUnicodeString(&LogonSessionData->AuthenticationPackage);
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, LogonType))
        user->LogonType = LogonSessionData->LogonType;
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, Session))
        user->SessionId = LogonSessionData->Session;
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, Sid))
        user->Sid = PhSidToStringSid(LogonSessionData->Sid);
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, LogonTime))
        user->LogonTime = LogonSessionData->LogonTime;
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, LogonServer))
        user->LogonServer = PhCreateStringFromUnicodeString(&LogonSessionData->LogonServer);
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, DnsDomainName))
        user->DnsDomainName = PhCreateStringFromUnicodeString(&LogonSessionData->DnsDomainName);
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, Upn))
        user->UserPrincipalName = PhCreateStringFromUnicodeString(&LogonSessionData->Upn);
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, UserFlags))
        user->UserFlags = LogonSessionData->UserFlags;
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, LastLogonInfo))
    {
        user->FailedAttemptsSinceLastSuccessfulLogon = LogonSessionData->LastLogonInfo.FailedAttemptCountSinceLastSuccessfulLogon;
        user->LastSuccessfulLogon = LogonSessionData->LastLogonInfo.LastSuccessfulLogon;
        user->LastFailedLogon = LogonSessionData->LastLogonInfo.LastFailedLogon;
    }
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, LogonScript))
        user->LogonScript = PhCreateStringFromUnicodeString(&LogonSessionData->LogonScript);
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, ProfilePath))
        user->ProfilePath = PhCreateStringFromUnicodeString(&LogonSessionData->ProfilePath);
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, HomeDirectory))
        user->HomeDirectory = PhCreateStringFromUnicodeString(&LogonSessionData->HomeDirectory);
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, HomeDirectoryDrive))
        user->HomeDirectoryDrive = PhCreateStringFromUnicodeString(&LogonSessionData->HomeDirectoryDrive);
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, LogoffTime))
        user->LogoffTime = LogonSessionData->LogoffTime;
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, KickOffTime))
        user->KickOffTime = LogonSessionData->KickOffTime;
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, PasswordLastSet))
        user->PasswordLastSet = LogonSessionData->PasswordLastSet;
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, PasswordCanChange))
        user->PasswordCanChange = LogonSessionData->PasswordCanChange;
    if (RTL_CONTAINS_FIELD(LogonSessionData, LogonSessionData->Size, PasswordMustChange))
        user->PasswordMustChange = LogonSessionData->PasswordMustChange;

    if (Context->FilterSupport.NodeList)
        user->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &user->Node);

    return user;
}

BOOLEAN PhpUserListTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_USER_LIST_CONTEXT context = Context;
    PPH_USER_NODE user = (PPH_USER_NODE)Node;

    assert(Context);

    if (!context->SearchMatchHandle)
        return TRUE;

    if (PhSearchControlMatchLongHintZ(context->SearchMatchHandle, user->LogonIdText))
        return TRUE;
    if (user->UserName && PhSearchControlMatch(context->SearchMatchHandle, &user->UserName->sr))
        return TRUE;
    if (user->LogonDomain && PhSearchControlMatch(context->SearchMatchHandle, &user->LogonDomain->sr))
        return TRUE;
    if (user->AuthenticationPackage && PhSearchControlMatch(context->SearchMatchHandle, &user->AuthenticationPackage->sr))
        return TRUE;
    if (user->LogonTimeText && PhSearchControlMatch(context->SearchMatchHandle, &user->LogonTimeText->sr))
        return TRUE;
    if (user->SessionIdText && PhSearchControlMatch(context->SearchMatchHandle, &user->SessionIdText->sr))
        return TRUE;
    if (user->Sid && PhSearchControlMatch(context->SearchMatchHandle, &user->Sid->sr))
        return TRUE;
    if (user->LogoffTimeText && PhSearchControlMatch(context->SearchMatchHandle, &user->LogoffTimeText->sr))
        return TRUE;
    if (user->LogonServer && PhSearchControlMatch(context->SearchMatchHandle, &user->LogonServer->sr))
        return TRUE;
    if (user->DnsDomainName && PhSearchControlMatch(context->SearchMatchHandle, &user->DnsDomainName->sr))
        return TRUE;
    if (user->UserPrincipalName && PhSearchControlMatch(context->SearchMatchHandle, &user->UserPrincipalName->sr))
        return TRUE;
    if (user->UserFlagsText && PhSearchControlMatch(context->SearchMatchHandle, &user->UserFlagsText->sr))
        return TRUE;
    if (user->FailedAttemptsSinceLastSuccessfulLogonText && PhSearchControlMatch(context->SearchMatchHandle, &user->FailedAttemptsSinceLastSuccessfulLogonText->sr))
        return TRUE;
    if (user->LastSuccessfulLogonText && PhSearchControlMatch(context->SearchMatchHandle, &user->LastSuccessfulLogonText->sr))
        return TRUE;
    if (user->LastFailedLogonText && PhSearchControlMatch(context->SearchMatchHandle, &user->LastFailedLogonText->sr))
        return TRUE;
    if (user->LogonScript && PhSearchControlMatch(context->SearchMatchHandle, &user->LogonScript->sr))
        return TRUE;
    if (user->ProfilePath && PhSearchControlMatch(context->SearchMatchHandle, &user->ProfilePath->sr))
        return TRUE;
    if (user->HomeDirectory && PhSearchControlMatch(context->SearchMatchHandle, &user->HomeDirectory->sr))
        return TRUE;
    if (user->HomeDirectoryDrive && PhSearchControlMatch(context->SearchMatchHandle, &user->HomeDirectoryDrive->sr))
        return TRUE;
    if (user->LogoffTimeText && PhSearchControlMatch(context->SearchMatchHandle, &user->LogoffTimeText->sr))
        return TRUE;
    if (user->KickOffTimeText && PhSearchControlMatch(context->SearchMatchHandle, &user->KickOffTimeText->sr))
        return TRUE;
    if (user->PasswordLastSetText && PhSearchControlMatch(context->SearchMatchHandle, &user->PasswordLastSetText->sr))
        return TRUE;
    if (user->PasswordCanChangeText && PhSearchControlMatch(context->SearchMatchHandle, &user->PasswordCanChangeText->sr))
        return TRUE;
    if (user->PasswordMustChangeText && PhSearchControlMatch(context->SearchMatchHandle, &user->PasswordMustChangeText->sr))
        return TRUE;

    return FALSE;
}

VOID PhpDeleteUserListTree(
    _In_ PPH_USER_LIST_CONTEXT Context
    )
{
    if (Context->NodeList)
    {
        for (ULONG i = 0; i < Context->NodeList->Count; i++)
        {
            PhpDeleteUserNode(Context->NodeList->Items[i]);
        }

        PhDereferenceObject(Context->NodeList);
        Context->NodeList = NULL;
    }

    PhDeleteTreeNewFilterSupport(&Context->FilterSupport);
}

VOID PhpUserListRefresh(
    _In_ PPH_USER_LIST_CONTEXT Context
    )
{
    NTSTATUS status;
    ULONG logonSessionCount;
    PLUID logonSessionList;
    PH_FORMAT format[2];
    PPH_STRING message;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhpDeleteUserListTree(Context);

    Context->NodeList = PhCreateList(1);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);
    PhAddTreeNewFilter(&Context->FilterSupport, PhpUserListTreeFilterCallback, Context);

    if (NT_SUCCESS(status = LsaEnumerateLogonSessions_I(&logonSessionCount, &logonSessionList)))
    {
        for (ULONG i = 0; i < logonSessionCount; i++)
        {
            PSECURITY_LOGON_SESSION_DATA logonSessionData;

            if (NT_SUCCESS(status = LsaGetLogonSessionData_I(&logonSessionList[i], &logonSessionData)))
            {
                PhAddItemList(Context->NodeList, PhpCreateUserNode(Context, logonSessionData));
                LsaFreeReturnBuffer_I(logonSessionData);
            }
        }

        LsaFreeReturnBuffer_I(logonSessionList);
    }

    PhInitFormatS(&format[0], L"Number of users: ");
    PhInitFormatU(&format[1], Context->NodeList->Count);
    message = PhFormat(format, 2, 10);

    SetWindowText(Context->MessageHandle, message->Buffer);

    PhDereferenceObject(message);

    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

BOOLEAN PhpGetSelectedUserListNodes(
    _In_ PPH_USER_LIST_CONTEXT Context,
    _Out_ PPH_USER_NODE** Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_USER_NODE node = (PPH_USER_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
            PhAddItemList(list, node);
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    *Nodes = NULL;
    *NumberOfNodes = 0;

    PhDereferenceObject(list);
    return FALSE;
}

#define SORT_FUNCTION(Column) PvpUserListTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvpUserListTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_USER_NODE node1 = *(PPH_USER_NODE *)_elem1; \
    PPH_USER_NODE node2 = *(PPH_USER_NODE *)_elem2; \
    PPH_USER_LIST_CONTEXT context = _context;
    int sortResult = 0;

#define END_SORT_FUNCTION \
    return PhModifySort(sortResult, context->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(LogonId)
{
    sortResult = uintcmp(node1->LogonId.LowPart, node2->LogonId.LowPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(UserName)
{
    sortResult = PhCompareStringWithNullSortOrder(
        node1->UserName,
        node2->UserName,
        context->TreeNewSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LogonDomain)
{
    sortResult = PhCompareStringWithNullSortOrder(
        node1->LogonDomain,
        node2->LogonDomain,
        context->TreeNewSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(AuthenticationPackage)
{
    sortResult = PhCompareStringWithNullSortOrder(
        node1->AuthenticationPackage,
        node2->AuthenticationPackage,
        context->TreeNewSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LogonType)
{
    sortResult = uintcmp(node1->LogonType, node2->LogonType);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Session)
{
    sortResult = uintcmp(node1->SessionId, node2->SessionId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Sid)
{
    sortResult = PhCompareStringWithNullSortOrder(
        node1->Sid,
        node2->Sid,
        context->TreeNewSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LogonTime)
{
    sortResult = int64cmp(node1->LogonTime.QuadPart, node2->LogonTime.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LogonServer)
{
    sortResult = PhCompareStringWithNullSortOrder(
        node1->LogonServer,
        node2->LogonServer,
        context->TreeNewSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(DnsDomainName)
{
    sortResult = PhCompareStringWithNullSortOrder(
        node1->DnsDomainName,
        node2->DnsDomainName,
        context->TreeNewSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(UserPrincipalName)
{
    sortResult = PhCompareStringWithNullSortOrder(
        node1->UserPrincipalName,
        node2->UserPrincipalName,
        context->TreeNewSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(UserFlags)
{
    sortResult = uintcmp(node1->UserFlags, node2->UserFlags);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(FailedAttemptsSinceLastSuccessfulLogon)
{
    sortResult = uintcmp(node1->FailedAttemptsSinceLastSuccessfulLogon, node2->FailedAttemptsSinceLastSuccessfulLogon);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LastSuccessfulLogon)
{
    sortResult = int64cmp(node1->LastSuccessfulLogon.QuadPart, node2->LastSuccessfulLogon.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LastFailedLogon)
{
    sortResult = int64cmp(node1->LastFailedLogon.QuadPart, node2->LastFailedLogon.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LogonScript)
{
    sortResult = PhCompareStringWithNullSortOrder(
        node1->LogonScript,
        node2->LogonScript,
        context->TreeNewSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ProfilePath)
{
    sortResult = PhCompareStringWithNullSortOrder(
        node1->ProfilePath,
        node2->ProfilePath,
        context->TreeNewSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(HomeDirectory)
{
    sortResult = PhCompareStringWithNullSortOrder(
        node1->HomeDirectory,
        node2->HomeDirectory,
        context->TreeNewSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(HomeDirectoryDrive)
{
    sortResult = PhCompareStringWithNullSortOrder(
        node1->HomeDirectoryDrive,
        node2->HomeDirectoryDrive,
        context->TreeNewSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LogoffTime)
{
    sortResult = int64cmp(node1->LogoffTime.QuadPart, node2->LogoffTime.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(KickOffTime)
{
    sortResult = int64cmp(node1->KickOffTime.QuadPart, node2->KickOffTime.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PasswordLastSet)
{
    sortResult = int64cmp(node1->PasswordLastSet.QuadPart, node2->PasswordLastSet.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PasswordCanChange)
{
    sortResult = int64cmp(node1->PasswordCanChange.QuadPart, node2->PasswordCanChange.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PasswordMustChange)
{
    sortResult = int64cmp(node1->PasswordMustChange.QuadPart, node2->PasswordMustChange.QuadPart);
}
END_SORT_FUNCTION


BOOLEAN NTAPI PhpUserListTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_USER_LIST_CONTEXT context = Context;
    PPH_USER_NODE user;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            user = (PPH_USER_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(LogonId),
                    SORT_FUNCTION(UserName),
                    SORT_FUNCTION(LogonDomain),
                    SORT_FUNCTION(AuthenticationPackage),
                    SORT_FUNCTION(LogonType),
                    SORT_FUNCTION(Session),
                    SORT_FUNCTION(Sid),
                    SORT_FUNCTION(LogonTime),
                    SORT_FUNCTION(LogonServer),
                    SORT_FUNCTION(DnsDomainName),
                    SORT_FUNCTION(UserPrincipalName),
                    SORT_FUNCTION(UserFlags),
                    SORT_FUNCTION(FailedAttemptsSinceLastSuccessfulLogon),
                    SORT_FUNCTION(LastSuccessfulLogon),
                    SORT_FUNCTION(LastFailedLogon),
                    SORT_FUNCTION(LogonScript),
                    SORT_FUNCTION(ProfilePath),
                    SORT_FUNCTION(HomeDirectory),
                    SORT_FUNCTION(HomeDirectoryDrive),
                    SORT_FUNCTION(LogoffTime),
                    SORT_FUNCTION(KickOffTime),
                    SORT_FUNCTION(PasswordLastSet),
                    SORT_FUNCTION(PasswordCanChange),
                    SORT_FUNCTION(PasswordMustChange),

                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                static_assert(RTL_NUMBER_OF(sortFunctions) == PH_USER_LIST_COLUMN_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < PH_USER_LIST_COLUMN_MAXIMUM)
                    sortFunction = sortFunctions[context->TreeNewSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                }

                getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;
            user = (PPH_USER_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            user = (PPH_USER_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PH_USER_LIST_COLUMN_LOGON_ID:
                PhPrintPointer(user->LogonIdText, UlongToPtr(user->LogonId.LowPart));
                PhInitializeStringRefLongHint(&getCellText->Text, user->LogonIdText);
                break;
            case PH_USER_LIST_COLUMN_USER_NAME:
                getCellText->Text = PhGetStringRef(user->UserName);
                break;
            case PH_USER_LIST_COLUMN_LOGON_DOMAIN:
                getCellText->Text = PhGetStringRef(user->LogonDomain);
                break;
            case PH_USER_LIST_COLUMN_AUTHENTICATION_PACKAGE:
                getCellText->Text = PhGetStringRef(user->AuthenticationPackage);
                break;
            case PH_USER_LIST_COLUMN_LOGON_TYPE:
                switch (user->LogonType)
                {
                case UndefinedLogonType:
                    PhInitializeStringRef(&getCellText->Text, L"Undefined");
                    break;
                case Interactive:
                    PhInitializeStringRef(&getCellText->Text, L"Interactive");
                    break;
                case Network:
                    PhInitializeStringRef(&getCellText->Text, L"Network");
                    break;
                case Batch:
                    PhInitializeStringRef(&getCellText->Text, L"Batch");
                    break;
                case Service:
                    PhInitializeStringRef(&getCellText->Text, L"Service");
                    break;
                case Proxy:
                    PhInitializeStringRef(&getCellText->Text, L"Proxy");
                    break;
                case Unlock:
                    PhInitializeStringRef(&getCellText->Text, L"Unlock");
                    break;
                case NetworkCleartext:
                    PhInitializeStringRef(&getCellText->Text, L"Network cleartext");
                    break;
                case NewCredentials:
                    PhInitializeStringRef(&getCellText->Text, L"New credentials");
                    break;
                case RemoteInteractive:
                    PhInitializeStringRef(&getCellText->Text, L"Remote interactive");
                    break;
                case CachedInteractive:
                    PhInitializeStringRef(&getCellText->Text, L"Cached interactive");
                    break;
                case CachedRemoteInteractive:
                    PhInitializeStringRef(&getCellText->Text, L"Cached remote interactive");
                    break;
                case CachedUnlock:
                    PhInitializeStringRef(&getCellText->Text, L"Cached unlock");
                    break;
                default:
                    break;
                }
                break;
            case PH_USER_LIST_COLUMN_SESSION_ID:
                PhMoveReference(&user->SessionIdText, PhFormatUInt64(user->SessionId, TRUE));
                getCellText->Text = user->SessionIdText->sr;
                break;
            case PH_USER_LIST_COLUMN_SID:
                getCellText->Text = PhGetStringRef(user->Sid);
                break;
            case PH_USER_LIST_COLUMN_LOGON_TIME:
                {
                    if (user->LogonTime.QuadPart != 0)
                    {
                        SYSTEMTIME systemTime;

                        PhLargeIntegerToLocalSystemTime(&systemTime, &user->LogonTime);
                        PhMoveReference(&user->LogonTimeText, PhFormatDateTime(&systemTime));
                        getCellText->Text = PhGetStringRef(user->LogonTimeText);
                    }
                }
                break;
            case PH_USER_LIST_COLUMN_LOGON_SERVER:
                getCellText->Text = PhGetStringRef(user->LogonServer);
                break;
            case PH_USER_LIST_COLUMN_DNS_DOMAIN_NAME:
                getCellText->Text = PhGetStringRef(user->DnsDomainName);
                break;
            case PH_USER_LIST_COLUMN_USER_PRINCIPAL_NAME:
                getCellText->Text = PhGetStringRef(user->UserPrincipalName);
                break;
            case PH_USER_LIST_COLUMN_USER_FLAGS:
                {
                    if (user->UserFlags)
                    {
                        user->UserFlagsText = PhpFormatUserFlags(user->UserFlags);
                        getCellText->Text = user->UserFlagsText->sr;
                    }
                }
                break;
            case PH_USER_LIST_COLUMN_FAILED_ATTEMPTS_SINCE_LAST_SUCCESSFUL_LOGON:
                PhMoveReference(&user->FailedAttemptsSinceLastSuccessfulLogonText, PhFormatUInt64(user->FailedAttemptsSinceLastSuccessfulLogon, TRUE));
                getCellText->Text = user->FailedAttemptsSinceLastSuccessfulLogonText->sr;
                break;
            case PH_USER_LIST_COLUMN_LAST_SUCCESSFUL_LOGON:
                {
                    if (user->LastSuccessfulLogon.QuadPart != 0)
                    {
                        SYSTEMTIME systemTime;

                        PhLargeIntegerToLocalSystemTime(&systemTime, &user->LastSuccessfulLogon);
                        PhMoveReference(&user->LastSuccessfulLogonText, PhFormatDateTime(&systemTime));
                        getCellText->Text = PhGetStringRef(user->LastSuccessfulLogonText);
                    }
                }
                break;
            case PH_USER_LIST_COLUMN_LAST_FAILED_LOGON:
                {
                    if (user->LastFailedLogon.QuadPart != 0)
                    {
                        SYSTEMTIME systemTime;

                        PhLargeIntegerToLocalSystemTime(&systemTime, &user->LastFailedLogon);
                        PhMoveReference(&user->LastFailedLogonText, PhFormatDateTime(&systemTime));
                        getCellText->Text = PhGetStringRef(user->LastFailedLogonText);
                    }
                }
                break;
            case PH_USER_LIST_COLUMN_LOGON_SCRIPT:
                getCellText->Text = PhGetStringRef(user->LogonScript);
                break;
            case PH_USER_LIST_COLUMN_PROFILE_PATH:
                getCellText->Text = PhGetStringRef(user->ProfilePath);
                break;
            case PH_USER_LIST_COLUMN_HOME_DIRECTORY:
                getCellText->Text = PhGetStringRef(user->HomeDirectory);
                break;
            case PH_USER_LIST_COLUMN_HOME_DIRECTORY_DRIVE:
                getCellText->Text = PhGetStringRef(user->HomeDirectoryDrive);
                break;
            case PH_USER_LIST_COLUMN_LOGOFF_TIME:
                {
                    if (user->LogoffTime.QuadPart != 0)
                    {
                        SYSTEMTIME systemTime;

                        PhLargeIntegerToLocalSystemTime(&systemTime, &user->LogoffTime);
                        PhMoveReference(&user->LogoffTimeText, PhFormatDateTime(&systemTime));
                        getCellText->Text = PhGetStringRef(user->LogoffTimeText);
                    }
                }
                break;
            case PH_USER_LIST_COLUMN_KICKOFF_TIME:
                {
                    if (user->KickOffTime.QuadPart != 0)
                    {
                        SYSTEMTIME systemTime;

                        PhLargeIntegerToLocalSystemTime(&systemTime, &user->KickOffTime);
                        PhMoveReference(&user->KickOffTimeText, PhFormatDateTime(&systemTime));
                        getCellText->Text = PhGetStringRef(user->KickOffTimeText);
                    }
                }
                break;
            case PH_USER_LIST_COLUMN_PASSWORD_LAST_SET:
                {
                    if (user->PasswordLastSet.QuadPart != 0)
                    {
                        SYSTEMTIME systemTime;

                        PhLargeIntegerToLocalSystemTime(&systemTime, &user->PasswordLastSet);
                        PhMoveReference(&user->PasswordLastSetText, PhFormatDateTime(&systemTime));
                        getCellText->Text = PhGetStringRef(user->PasswordLastSetText);
                    }
                }
                break;
            case PH_USER_LIST_COLUMN_PASSWORD_CAN_CHANGE:
                {
                    if (user->PasswordCanChange.QuadPart != 0)
                    {
                        SYSTEMTIME systemTime;

                        PhLargeIntegerToLocalSystemTime(&systemTime, &user->PasswordCanChange);
                        PhMoveReference(&user->PasswordCanChangeText, PhFormatDateTime(&systemTime));
                        getCellText->Text = PhGetStringRef(user->PasswordCanChangeText);
                    }
                }
                break;
            case PH_USER_LIST_COLUMN_PASSWORD_MUST_CHANGE:
                {
                    if (user->PasswordMustChange.QuadPart != 0)
                    {
                        SYSTEMTIME systemTime;

                        PhLargeIntegerToLocalSystemTime(&systemTime, &user->PasswordMustChange);
                        PhMoveReference(&user->PasswordMustChangeText, PhFormatDateTime(&systemTime));
                        getCellText->Text = PhGetStringRef(user->PasswordMustChangeText);
                    }
                }
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = (PPH_TREENEW_GET_NODE_COLOR)Parameter1;
            user = (PPH_USER_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->TreeNewSortColumn = sorting->SortColumn;
            context->TreeNewSortOrder = sorting->SortOrder;

            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                {
                    if (GetKeyState(VK_CONTROL) < 0)
                    {
                        PPH_STRING text;

                        text = PhGetTreeNewText(hwnd, 0);
                        PhSetClipboardString(hwnd, &text->sr);
                        PhDereferenceObject(text);
                    }
                }
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(context->TreeNewHandle, 0, -1);
                break;
            }
        }
        return TRUE;
    case TreeNewNodeExpanding:
        return TRUE;
    case TreeNewLeftDoubleClick:
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenu = Parameter1;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPH_USER_NODE* userNodes = NULL;
            ULONG numberOfNodes = 0;

            if (!PhpGetSelectedUserListNodes(context, &userNodes, &numberOfNodes))
                break;

            if (numberOfNodes != 0)
            {
                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"Copy", NULL, NULL), ULONG_MAX);
                PhInsertCopyCellEMenuItem(menu, IDC_COPY, context->TreeNewHandle, contextMenu->Column);

                selectedItem = PhShowEMenu(
                    menu,
                    hwnd,
                    PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    contextMenu->Location.x,
                    contextMenu->Location.y
                    );

                if (selectedItem && selectedItem->Id != ULONG_MAX && !PhHandleCopyCellEMenuItem(selectedItem))
                {
                    if (selectedItem->Id == IDC_COPY)
                    {
                        PPH_STRING text;

                        text = PhGetTreeNewText(context->TreeNewHandle, 0);
                        PhSetClipboardString(context->TreeNewHandle, &text->sr);
                        PhDereferenceObject(text);
                    }
                }

                PhDestroyEMenu(menu);
                PhFree(userNodes);
            }
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

VOID PhpUserListLoadSettingsTreeList(
    _Inout_ PPH_USER_LIST_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhGetStringSetting(L"UserListTreeListColumns");
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID PhpUserListSaveSettingsTreeList(
    _Inout_ PPH_USER_LIST_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(L"UserListTreeListColumns", &settings->sr);
    PhDereferenceObject(settings);
}

VOID PhpInitializeUserListTree(
    _In_ PPH_USER_LIST_CONTEXT Context
    )
{
    ULONG index = 0;

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");

    TreeNew_SetCallback(Context->TreeNewHandle, PhpUserListTreeNewCallback, Context);
    TreeNew_SetExtendedFlags(Context->TreeNewHandle, TN_FLAG_ITEM_DRAG_SELECT, TN_FLAG_ITEM_DRAG_SELECT);

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_USER_NAME, TRUE, L"User name", 250, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_LOGON_DOMAIN, TRUE, L"Logon domain", 180, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_SESSION_ID, TRUE, L"Session ID", 80, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_LOGON_TYPE, TRUE, L"Logon type", 100, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_AUTHENTICATION_PACKAGE, TRUE, L"Authentication package", 140, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_DNS_DOMAIN_NAME, TRUE, L"DNS domain name", 140, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_LOGON_TIME, TRUE, L"Logon time", 140, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_PASSWORD_LAST_SET, TRUE, L"Password last set", 140, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_SID, TRUE, L"SID", 180, PH_ALIGN_LEFT, index++, 0);

    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_LOGON_ID, FALSE, L"Logon ID", 80, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_LOGON_SERVER, FALSE, L"Logon server", 180, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_USER_PRINCIPAL_NAME, FALSE, L"User principal name", 180, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_USER_FLAGS, FALSE, L"User flags", 180, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_FAILED_ATTEMPTS_SINCE_LAST_SUCCESSFUL_LOGON, FALSE, L"Failed logon attempts since", 80, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_LAST_SUCCESSFUL_LOGON, FALSE, L"Last successful logon", 140, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_LAST_FAILED_LOGON, FALSE, L"Last failed logon", 140, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_LOGON_SCRIPT, FALSE, L"Longon script", 180, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_PROFILE_PATH, FALSE, L"Profile path", 180, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_HOME_DIRECTORY, FALSE, L"Home directory", 180, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_HOME_DIRECTORY_DRIVE, FALSE, L"Home directory drive", 80, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_LOGOFF_TIME, FALSE, L"Logoff time", 140, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_KICKOFF_TIME, FALSE, L"Kickoff time", 140, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_PASSWORD_CAN_CHANGE, FALSE, L"Password can set", 140, PH_ALIGN_LEFT, index++, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_USER_LIST_COLUMN_PASSWORD_MUST_CHANGE, FALSE, L"Password must set", 140, PH_ALIGN_LEFT, index++, 0);

    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
    //TreeNew_SetTriState(Context->TreeNewHandle, FALSE);

    PhpUserListLoadSettingsTreeList(Context);
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PhpUserListSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPH_USER_LIST_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    PhApplyTreeNewFilters(&context->FilterSupport);
}

INT_PTR CALLBACK PhpUserListDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_USER_LIST_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_USER_LIST_CONTEXT)lParam;
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
            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);
            context->MessageHandle = GetDlgItem(hwndDlg, IDC_MESSAGE);
            context->SearchWindowHandle = GetDlgItem(hwndDlg, IDC_FILTER);

            PhSetApplicationWindowIcon(hwndDlg);
            PhRegisterDialog(hwndDlg);
            PhCreateSearchControl(
                hwndDlg,
                context->SearchWindowHandle,
                L"Search Users",
                PhpUserListSearchControlCallback,
                context
                );

            PhpInitializeUserListTree(context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->MessageHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 300;
            context->MinimumSize.bottom = 100;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            if (PhGetIntegerPairSetting(L"UserListWindowPosition").X)
                PhLoadWindowPlacementFromSetting(L"UserListWindowPosition", L"UserListWindowSize", hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            PhRegisterWindowCallback(hwndDlg, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);

            PhpUserListRefresh(context);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhSaveWindowPlacementToSetting(L"UserListWindowPosition", L"UserListWindowSize", hwndDlg);

            PhUnregisterWindowCallback(hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhpUserListSaveSettingsTreeList(context);

            PhpDeleteUserListTree(context);
            PhFree(context);

            PostQuitMessage(0);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                case IDC_REFRESH:
                    PhpUserListRefresh(context);
                    break;
                case IDCANCEL:
                    DestroyWindow(hwndDlg);
                    break;
                }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpUserListDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    HWND windowHandle;
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    windowHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_USRLIST),
        NULL,
        PhpUserListDlgProc,
        Parameter
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

        if (!IsDialogMessage(windowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID PhShowUserListDialog(
    _In_ HWND ParentWindowHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_USER_LIST_CONTEXT context;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"secur32.dll"))
        {
            LsaFreeReturnBuffer_I = PhGetDllBaseProcedureAddress(baseAddress, "LsaFreeReturnBuffer", 0);
            LsaEnumerateLogonSessions_I = PhGetDllBaseProcedureAddress(baseAddress, "LsaEnumerateLogonSessions", 0);
            LsaGetLogonSessionData_I = PhGetDllBaseProcedureAddress(baseAddress, "LsaGetLogonSessionData", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!LsaFreeReturnBuffer_I || !LsaEnumerateLogonSessions_I || !LsaGetLogonSessionData_I)
    {
        PhShowStatus(ParentWindowHandle, L"Unable to locate routines.", STATUS_NOINTERFACE, 0);
        return;
    }

    context = PhAllocateZero(sizeof(PH_USER_LIST_CONTEXT));
    context->ParentWindowHandle = ParentWindowHandle;

    if (!NT_SUCCESS(PhCreateThread2(PhpUserListDialogThreadStart, context)))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to create the window.", 0, ERROR_OUTOFMEMORY);
        PhFree(context);
    }
}
