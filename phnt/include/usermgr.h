/*
 * User Manager service API definitions.
 *
 * This file is part of System Informer.
 */

#ifndef _USERMGR_H
#define _USERMGR_H

// private
typedef struct _SESSION_USER_CONTEXT
{
    ULONGLONG ContextToken;
    ULONG SessionId;
    ULONG Reserved;
} SESSION_USER_CONTEXT, *PSESSION_USER_CONTEXT;

// private
typedef struct _CRED_PROV_CREDENTIAL
{
    ULONG Flags;
    ULONG AuthenticationPackage;
    ULONG Size;
    PVOID Information;
} CRED_PROV_CREDENTIAL, *PCRED_PROV_CREDENTIAL;

#define USERMGRAPI DECLSPEC_IMPORT

// Contexts

#if (PHNT_VERSION >= PHNT_THRESHOLD)

// rev
USERMGRAPI
VOID
WINAPI
UMgrFreeSessionUsers(
    _In_ _Post_invalid_ PSESSION_USER_CONTEXT SessionUsers
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrEnumerateSessionUsers(
    _Out_ PULONG Count,
    _Outptr_ PSESSION_USER_CONTEXT *SessionUsers
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrQueryUserContext(
    _In_ HANDLE TokenHandle,
    _Out_ PULONGLONG ContextToken
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrQueryUserContextFromSid(
    _In_ PWSTR SidString,
    _Out_ PULONGLONG ContextToken
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrQueryUserContextFromName(
    _In_ PWSTR UserName,
    _Out_ PULONGLONG ContextToken
    );

#endif

// Tokens

#if (PHNT_VERSION >= PHNT_THRESHOLD)

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrQueryDefaultAccountToken(
    _Out_ PHANDLE TokenHandle
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrQuerySessionUserToken(
    _In_ ULONG SessionId,
    _Out_ PHANDLE TokenHandle
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrQueryUserToken(
    _In_ ULONGLONG Context,
    _Out_ PHANDLE TokenHandle
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrQueryUserTokenFromSid(
    _In_ PWSTR SidString,
    _Out_ PHANDLE TokenHandle
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrQueryUserTokenFromName(
    _In_ PWSTR UserName,
    _Out_ PHANDLE TokenHandle
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrGetConstrainedUserToken(
    _In_opt_ HANDLE InputTokenHandle,
    _In_ ULONGLONG Context,
    _In_opt_ PSECURITY_CAPABILITIES Capabilities,
    _Out_ _Ret_maybenull_ PHANDLE OutputTokenHandle
    );

#endif

#if (PHNT_VERSION >= PHNT_THRESHOLD2)

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrChangeSessionUserToken(
    _In_ HANDLE TokenHandle
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrGetImpersonationTokenForContext(
    _In_ HANDLE InputTokenHandle,
    _In_ ULONGLONG Context,
    _Out_ PHANDLE OutputTokenHandle
    );

#endif

#if (PHNT_VERSION >= PHNT_REDSTONE)

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrGetSessionActiveShellUserToken(
    _In_ ULONG SessionId,
    _Out_ PHANDLE TokenHandle
    );

#endif

// Single-session SKU

#if (PHNT_VERSION >= PHNT_THRESHOLD)

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrOpenProcessTokenForQuery(
    _In_ ULONG ProcessId,
    _Out_ PHANDLE TokenHandle
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrOpenProcessHandleForAccess(
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ProcessId,
    _Out_ PHANDLE ProcessHandle
    );

#endif

// Credentials

#if (PHNT_VERSION >= PHNT_THRESHOLD)

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrFreeUserCredentials(
    _In_ PCRED_PROV_CREDENTIAL Credentials
    );

// rev
USERMGRAPI
HRESULT
WINAPI
UMgrGetCachedCredentials(
    _In_ PSID Sid,
    _Outptr_ PCRED_PROV_CREDENTIAL *Credentials
    );

#endif

#endif
