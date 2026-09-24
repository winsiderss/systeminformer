/*
 * User Manager service API definitions.
 *
 * This file is part of System Informer.
 */

#ifndef _USERMGR_H
#define _USERMGR_H

/** Describes a user context associated with a session. */
typedef struct _SESSION_USER_CONTEXT
{
    ULONGLONG ContextToken;
    ULONG SessionId;
    ULONG Reserved;
} SESSION_USER_CONTEXT, *PSESSION_USER_CONTEXT;

/** Describes cached credential-provider authentication data. */
typedef struct _CRED_PROV_CREDENTIAL
{
    ULONG Flags;
    ULONG AuthenticationPackage;
    ULONG Size;
    PVOID Information;
} CRED_PROV_CREDENTIAL, *PCRED_PROV_CREDENTIAL;

#if (PHNT_VERSION >= PHNT_WINDOWS_10)

//
// Compatibility session and token helpers
//

/**
 * Tests whether a session is the interactive user session.
 *
 * @param SessionId The session identifier to test.
 * @return TRUE when the session is interactive; otherwise FALSE.
 */
NTSYSAPI
BOOL
NTAPI
IsInteractiveUserSession(
    _In_ ULONG SessionId
    );

/**
 * Retrieves the active session on a single-session system.
 *
 * @param SessionId Receives the active session identifier.
 * @return TRUE on success; otherwise FALSE. Extended error information is available through GetLastError.
 */
NTSYSAPI
BOOL
NTAPI
QueryActiveSession(
    _Out_ PULONG SessionId
    );

/**
 * Opens the primary user token for a session.
 *
 * @param SessionId The session identifier.
 * @param TokenHandle Receives a handle to the token. The caller must close the handle.
 * @return TRUE on success; otherwise FALSE. Extended error information is available through GetLastError.
 */
NTSYSAPI
BOOL
NTAPI
QueryUserToken(
    _In_ ULONG SessionId,
    _Out_ PHANDLE TokenHandle
    );

/**
 * Reports that registration of a user token without Winlogon is unsupported.
 *
 * @return FALSE. GetLastError returns ERROR_NOT_SUPPORTED.
 */
NTSYSAPI
BOOL
NTAPI
RegisterUsertokenForNoWinlogon(
    VOID
    );

//
// Contexts
//

/**
 * Frees an array returned by UMgrEnumerateSessionUsers.
 *
 * @param SessionUsers The array to free. This parameter may be NULL.
 */
NTSYSAPI
VOID
NTAPI
UMgrFreeSessionUsers(
    _In_opt_ _Post_invalid_ PSESSION_USER_CONTEXT SessionUsers
    );

/**
 * Enumerates user contexts associated with sessions.
 *
 * @param Count Receives the number of entries.
 * @param SessionUsers Receives an array that must be released with UMgrFreeSessionUsers.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrEnumerateSessionUsers(
    _Out_ PULONG Count,
    _Outptr_result_buffer_(*Count) PSESSION_USER_CONTEXT *SessionUsers
    );

/**
 * Retrieves the user-manager context token associated with a token handle.
 *
 * @param TokenHandle A token handle.
 * @param ContextToken Receives the context token.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrQueryUserContext(
    _In_ HANDLE TokenHandle,
    _Out_ PULONGLONG ContextToken
    );

/**
 * Retrieves a user-manager context token from a string SID.
 *
 * @param SidString The string-form security identifier.
 * @param ContextToken Receives the context token.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrQueryUserContextFromSid(
    _In_ PCWSTR SidString,
    _Out_ PULONGLONG ContextToken
    );

/**
 * Retrieves a user-manager context token from an account name.
 *
 * @param UserName The account name.
 * @param ContextToken Receives the context token.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrQueryUserContextFromName(
    _In_ PCWSTR UserName,
    _Out_ PULONGLONG ContextToken
    );

/**
 * Changes the active-shell user associated with a session.
 *
 * @param SessionId The session identifier.
 * @param ContextToken The user-manager context token.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrChangeSessionActiveShellUser(
    _In_ ULONG SessionId,
    _In_ ULONGLONG ContextToken
    );

//
// Tokens
//

/**
 * Retrieves the token for the default account.
 *
 * @param TokenHandle Receives the token handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrQueryDefaultAccountToken(
    _Out_ PHANDLE TokenHandle
    );

/**
 * Retrieves the user token for a session.
 *
 * @param SessionId The session identifier.
 * @param TokenHandle Receives the token handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrQuerySessionUserToken(
    _In_ ULONG SessionId,
    _Out_ PHANDLE TokenHandle
    );

/**
 * Retrieves the virtual-account token for a session.
 *
 * @param SessionId The session identifier.
 * @param TokenHandle Receives the token handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrQuerySessionVirtualAccountToken(
    _In_ ULONG SessionId,
    _Out_ PHANDLE TokenHandle
    );

/**
 * Retrieves the token associated with a User Manager context.
 *
 * @param ContextToken The User Manager context token.
 * @param TokenHandle Receives the token handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrQueryUserToken(
    _In_ ULONGLONG ContextToken,
    _Out_ PHANDLE TokenHandle
    );

/**
 * Retrieves a user token by string-form security identifier.
 *
 * @param SidString The string-form security identifier.
 * @param TokenHandle Receives the token handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrQueryUserTokenFromSid(
    _In_ PCWSTR SidString,
    _Out_ PHANDLE TokenHandle
    );

/**
 * Retrieves a user token by account name.
 *
 * @param UserName The account name.
 * @param TokenHandle Receives the token handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrQueryUserTokenFromName(
    _In_ PCWSTR UserName,
    _Out_ PHANDLE TokenHandle
    );

/**
 * Creates a constrained token for a User Manager context.
 *
 * @param InputTokenHandle An optional token on which to base the constrained token.
 * @param ContextToken The User Manager context token.
 * @param SecurityCapabilities Optional security capabilities for the constrained token.
 * @param OutputTokenHandle Receives the constrained token handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrGetConstrainedUserToken(
    _In_opt_ HANDLE InputTokenHandle,
    _In_ ULONGLONG ContextToken,
    _In_opt_ PSECURITY_CAPABILITIES SecurityCapabilities,
    _Out_ PHANDLE OutputTokenHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_TH2)

/**
 * Changes the user token associated with the caller's session.
 *
 * @param TokenHandle The replacement user token handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrChangeSessionUserToken(
    _In_ HANDLE TokenHandle
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10_TH2

#if (PHNT_VERSION >= PHNT_WINDOWS_10_TH2)

/**
 * Creates an impersonation token for a User Manager context.
 *
 * @param InputTokenHandle The token on which to base the impersonation token.
 * @param ContextToken The User Manager context token.
 * @param OutputTokenHandle Receives the impersonation token handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrGetImpersonationTokenForContext(
    _In_ HANDLE InputTokenHandle,
    _In_ ULONGLONG ContextToken,
    _Out_ PHANDLE OutputTokenHandle
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10_TH2

/**
 * Creates a standard-access impersonation token for a User Manager context.
 *
 * @param InputTokenHandle The token on which to base the impersonation token.
 * @param ContextToken The User Manager context token.
 * @param OutputTokenHandle Receives the impersonation token handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrGetStandardAccessImpersonationTokenForContext(
    _In_ HANDLE InputTokenHandle,
    _In_ ULONGLONG ContextToken,
    _Out_ PHANDLE OutputTokenHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)

/**
 * Retrieves the active-shell user token for a session.
 *
 * @param SessionId The session identifier.
 * @param TokenHandle Receives the token handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrGetSessionActiveShellUserToken(
    _In_ ULONG SessionId,
    _Out_ PHANDLE TokenHandle
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

/**
 * Determines whether a token may activate as a user context.
 *
 * @param TokenHandle The token to test.
 * @param ContextToken The User Manager context token.
 * @param Allowed Receives TRUE if activation is allowed; otherwise FALSE.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrIsAllowedToActivateAsUser(
    _In_ HANDLE TokenHandle,
    _In_ ULONGLONG ContextToken,
    _Out_ PBOOLEAN Allowed
    );

//
// Logon and shell lifecycle
//

/**
 * Logs on a user through the User Manager service.
 *
 * @param OriginName Identifies the source of the logon request.
 * @param LogonType The requested logon type.
 * @param AuthenticationPackage The authentication package identifier.
 * @param AuthenticationInformation Authentication-package-specific logon information.
 * @param AuthenticationInformationLength The size, in bytes, of AuthenticationInformation.
 * @param LocalGroups Optional additional groups for the token.
 * @param SourceContext The token source for the new token.
 * @param CallFlags Call-context flags supplied to the security subsystem.
 * @param NtlmThreadOptions NTLM thread options applied while processing the logon.
 * @param IpAddress Optional network-address data for the logon request.
 * @param IpAddressLength The size, in bytes, of IpAddress.
 * @param ProfileBuffer Receives the authentication-package profile buffer.
 * @param ProfileBufferLength Receives the size, in bytes, of the profile buffer.
 * @param LogonId Receives the logon-session identifier.
 * @param TokenHandle Receives the logged-on token handle. Close the handle with NtClose.
 * @param Quotas Receives the token quota limits.
 * @param Status Receives the status of the logon operation.
 * @param SubStatus Receives authentication-package-specific failure information.
 * @return An HRESULT indicating whether the request was transported and processed.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrLogonUser(
    _In_ PSTRING OriginName,
    _In_ SECURITY_LOGON_TYPE LogonType,
    _In_ ULONG AuthenticationPackage,
    _In_reads_bytes_(AuthenticationInformationLength) PVOID AuthenticationInformation,
    _In_ ULONG AuthenticationInformationLength,
    _In_opt_ PTOKEN_GROUPS LocalGroups,
    _In_ PTOKEN_SOURCE SourceContext,
    _In_ ULONG CallFlags,
    _In_ ULONG NtlmThreadOptions,
    _In_reads_bytes_opt_(IpAddressLength) PBYTE IpAddress,
    _In_ ULONG IpAddressLength,
    _Outptr_result_bytebuffer_(*ProfileBufferLength) PVOID *ProfileBuffer,
    _Out_ PULONG ProfileBufferLength,
    _Out_ PLUID LogonId,
    _Out_ PHANDLE TokenHandle,
    _Out_ PQUOTA_LIMITS Quotas,
    _Out_ PNTSTATUS Status,
    _Out_ PNTSTATUS SubStatus
    );

/**
 * Supplies caller state flags to User Manager.
 *
 * @param Flags The state flags to report.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrInformFlags(
    _In_ ULONG Flags
    );

/**
 * Notifies User Manager that the primary user has logged on.
 *
 * @param TokenHandle A token handle for the logged-on user. The service duplicates the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrInformUserLogon(
    _In_ HANDLE TokenHandle
    );

/**
 * Notifies User Manager that the primary user has logged off.
 *
 * @param TokenHandle A token handle identifying the user that logged off.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrInformUserLogoff(
    _In_ HANDLE TokenHandle
    );

/**
 * Notifies User Manager that a secondary user has logged on.
 *
 * @param Token The token handle for the secondary user.
 * @param UserId The identifier of the secondary user.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrInformSecondaryUserLogon(
    _In_ HANDLE Token,
    _In_ ULONG UserId
    );

/**
 * Notifies User Manager that a secondary user has logged off.
 *
 * @param Token The token handle for the secondary user.
 * @param UserId The identifier of the secondary user.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrInformSecondaryUserLogoff(
    _In_ HANDLE Token,
    _In_ ULONG UserId
    );

/**
 * Launches the user shell.
 *
 * @param ProcessHandle Receives a handle to the launched shell process. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrLaunchShell(
    _Out_ PHANDLE ProcessHandle
    );

/**
 * Launches the Shell Infrastructure Host for a user.
 *
 * @param Sid The security identifier of the user for whom to launch the host.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrLaunchShellInfrastructureHost(
    _In_ PSID Sid
    );

/**
 * Associates a shell process handle with the caller's session.
 *
 * @param ShellProcessHandle Optional handle to the shell process. The service duplicates the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrSetShellInformation(
    _In_opt_ HANDLE ShellProcessHandle
    );

//
// Local users and default sign-in
//

/**
 * Connects a local user.
 *
 * @param UserName The local user account name.
 * @param UserId The local user identifier.
 * @param ConnectionInformation Connection-specific data.
 * @param ConnectionInformationLength The size, in bytes, of ConnectionInformation.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrConnectLocalUser(
    _In_ PCWSTR UserName,
    _In_ GUID UserId,
    _In_reads_bytes_(ConnectionInformationLength) PBYTE ConnectionInformation,
    _In_ ULONG ConnectionInformationLength
    );

/**
 * Disconnects a local user.
 *
 * @param UserId The local user identifier.
 * @param ConnectionInformation Connection-specific data.
 * @param ConnectionInformationLength The size, in bytes, of ConnectionInformation.
 * @param UserName The local user account name.
 * @param DomainName The account domain name.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrDisconnectLocalUser(
    _In_ GUID UserId,
    _In_reads_bytes_(ConnectionInformationLength) PBYTE ConnectionInformation,
    _In_ ULONG ConnectionInformationLength,
    _In_ PCWSTR UserName,
    _In_ PCWSTR DomainName
    );

/**
 * The current service implementation does not support clearing the default sign-in account.
 *
 * @return E_UNEXPECTED in the current implementation.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrClearDefaultSignInAccount(
    VOID
    );

/**
 * The current service implementation does not support retrieving the default sign-in account.
 *
 * @return E_UNEXPECTED in the current implementation.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrGetDefaultSignInAccount(
    VOID
    );

//
// Single-session process access
//

/**
 * Opens the token of a process for query access.
 *
 * @param ProcessId The process identifier.
 * @param TokenHandle Receives the token handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrOpenProcessTokenForQuery(
    _In_ ULONG ProcessId,
    _Out_ PHANDLE TokenHandle
    );

/**
 * Opens a process with the requested access rights.
 *
 * @param DesiredAccess The requested process access mask.
 * @param ProcessId The process identifier.
 * @param ProcessHandle Receives the process handle. The caller must close the handle.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrOpenProcessHandleForAccess(
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ProcessId,
    _Out_ PHANDLE ProcessHandle
    );

//
// Credentials
//

/**
 * Securely erases and frees credentials returned by UMgrGetCachedCredentials.
 *
 * @param Credentials The credential record to erase and free.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrFreeUserCredentials(
    _In_ _Post_invalid_ PCRED_PROV_CREDENTIAL Credentials
    );

/**
 * Retrieves cached credential-provider data for a security identifier.
 *
 * @param Sid The user security identifier.
 * @param Credentials Receives an allocated credential record. Release it with UMgrFreeUserCredentials.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrGetCachedCredentials(
    _In_ PSID Sid,
    _Outptr_ PCRED_PROV_CREDENTIAL *Credentials
    );

/**
 * Stores credential-provider data for a user.
 *
 * @param UserId The user identifier.
 * @param Sid The user security identifier.
 * @param Credentials The credential-provider data to store.
 * @return An HRESULT indicating success or failure.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrSetCachedCredentials(
    _In_ ULONG UserId,
    _In_ PSID Sid,
    _In_ PCRED_PROV_CREDENTIAL Credentials
    );

/**
 * Returns E_NOTIMPL; the current implementation accepts no arguments.
 *
 * @return E_NOTIMPL.
 */
NTSYSAPI
HRESULT
NTAPI
UMgrpGetRegistryLocation(
    VOID
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#endif // _USERMGR_H
