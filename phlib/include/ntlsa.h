#ifndef _NTLSA_H
#define _NTLSA_H

// This header file provides access to the complete LSA API.
// (The rest is provided by ntsecapi.h)

#define _NTDEF_ // ntbasic already defines these things
#include <ntsecapi.h>

// Basic

NTSTATUS
NTAPI
LsaDelete(
    _In_ LSA_HANDLE ObjectHandle
    );

NTSTATUS
NTAPI
LsaQuerySecurityObject(
    _In_ LSA_HANDLE ObjectHandle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

NTSTATUS
NTAPI
LsaSetSecurityObject(
    _In_ LSA_HANDLE ObjectHandle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

// System access

#define SECURITY_ACCESS_INTERACTIVE_LOGON ((ULONG)0x00000001L)
#define SECURITY_ACCESS_NETWORK_LOGON ((ULONG)0x00000002L)
#define SECURITY_ACCESS_BATCH_LOGON ((ULONG)0x00000004L)
#define SECURITY_ACCESS_SERVICE_LOGON ((ULONG)0x00000010L)
#define SECURITY_ACCESS_PROXY_LOGON ((ULONG)0x00000020L)
#define SECURITY_ACCESS_DENY_INTERACTIVE_LOGON ((ULONG)0x00000040L)
#define SECURITY_ACCESS_DENY_NETWORK_LOGON ((ULONG)0x00000080L)
#define SECURITY_ACCESS_DENY_BATCH_LOGON ((ULONG)0x00000100L)
#define SECURITY_ACCESS_DENY_SERVICE_LOGON ((ULONG)0x00000200L)
#define SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON ((ULONG)0x00000400L)
#define SECURITY_ACCESS_DENY_REMOTE_INTERACTIVE_LOGON ((ULONG)0x00000800L)

// Policy

typedef ULONG POLICY_SYSTEM_ACCESS_MODE, *PPOLICY_SYSTEM_ACCESS_MODE;

#define POLICY_MODE_INTERACTIVE SECURITY_ACCESS_INTERACTIVE_LOGON
#define POLICY_MODE_NETWORK SECURITY_ACCESS_NETWORK_LOGON
#define POLICY_MODE_BATCH SECURITY_ACCESS_BATCH_LOGON
#define POLICY_MODE_SERVICE SECURITY_ACCESS_SERVICE_LOGON
#define POLICY_MODE_PROXY SECURITY_ACCESS_PROXY_LOGON
#define POLICY_MODE_DENY_INTERACTIVE SECURITY_ACCESS_DENY_INTERACTIVE_LOGON
#define POLICY_MODE_DENY_NETWORK SECURITY_ACCESS_DENY_NETWORK_LOGON
#define POLICY_MODE_DENY_BATCH SECURITY_ACCESS_DENY_BATCH_LOGON
#define POLICY_MODE_DENY_SERVICE SECURITY_ACCESS_DENY_SERVICE_LOGON
#define POLICY_MODE_REMOTE_INTERACTIVE SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON
#define POLICY_MODE_DENY_REMOTE_INTERACTIVE SECURITY_ACCESS_DENY_REMOTE_INTERACTIVE_LOGON

#define POLICY_MODE_ALL ( \
    POLICY_MODE_INTERACTIVE | \
    POLICY_MODE_NETWORK | \
    POLICY_MODE_BATCH | \
    POLICY_MODE_SERVICE | \
    POLICY_MODE_PROXY | \
    POLICY_MODE_DENY_INTERACTIVE | \
    POLICY_MODE_DENY_NETWORK | \
    SECURITY_ACCESS_DENY_BATCH_LOGON | \
    SECURITY_ACCESS_DENY_SERVICE_LOGON | \
    POLICY_MODE_REMOTE_INTERACTIVE | \
    POLICY_MODE_DENY_REMOTE_INTERACTIVE \
    )

typedef struct _POLICY_PRIVILEGE_DEFINITION
{
    LSA_UNICODE_STRING Name;
    LUID LocalValue;
} POLICY_PRIVILEGE_DEFINITION, *PPOLICY_PRIVILEGE_DEFINITION;

NTSTATUS
NTAPI
LsaOpenPolicySce(
    _In_opt_ PLSA_UNICODE_STRING SystemName,
    _In_ PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PLSA_HANDLE PolicyHandle
    );

NTSTATUS
NTAPI
LsaClearAuditLog(
    _In_ LSA_HANDLE PolicyHandle
    );

NTSTATUS
NTAPI
LsaEnumeratePrivileges(
    _In_ LSA_HANDLE PolicyHandle,
    _Inout_ PLSA_ENUMERATION_HANDLE EnumerationContext,
    _Out_ PVOID *Buffer, // PPOLICY_PRIVILEGE_DEFINITION *Buffer
    _In_ ULONG PreferedMaximumLength,
    _Out_ PULONG CountReturned
    );

#define LSA_LOOKUP_ISOLATED_AS_LOCAL 0x80000000

// Account

#define ACCOUNT_VIEW 0x00000001
#define ACCOUNT_ADJUST_PRIVILEGES 0x00000002
#define ACCOUNT_ADJUST_QUOTAS 0x00000004
#define ACCOUNT_ADJUST_SYSTEM_ACCESS 0x00000008
#define ACCOUNT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | \
    ACCOUNT_VIEW | \
    ACCOUNT_ADJUST_PRIVILEGES | \
    ACCOUNT_ADJUST_QUOTAS | \
    ACCOUNT_ADJUST_SYSTEM_ACCESS)
#define ACCOUNT_READ (STANDARD_RIGHTS_READ | ACCOUNT_VIEW)
#define ACCOUNT_WRITE (STANDARD_RIGHTS_WRITE | \
    ACCOUNT_ADJUST_PRIVILEGES | \
    ACCOUNT_ADJUST_QUOTAS | \
    ACCOUNT_ADJUST_SYSTEM_ACCESS)
#define ACCOUNT_EXECUTE (STANDARD_RIGHTS_EXECUTE)

NTSTATUS
NTAPI
LsaCreateAccount(
    _In_ LSA_HANDLE PolicyHandle,
    _In_ PSID AccountSid,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PLSA_HANDLE AccountHandle
    );

NTSTATUS
NTAPI
LsaOpenAccount(
    _In_ LSA_HANDLE PolicyHandle,
    _In_ PSID AccountSid,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PLSA_HANDLE AccountHandle
    );

NTSTATUS
NTAPI
LsaEnumerateAccounts(
    _In_ LSA_HANDLE PolicyHandle,
    _Inout_ PLSA_ENUMERATION_HANDLE EnumerationContext,
    _Out_ PVOID *Buffer, // PSID **Buffer
    _In_ ULONG PreferedMaximumLength,
    _Out_ PULONG CountReturned
    );

NTSTATUS
NTAPI
LsaAddPrivilegesToAccount(
    _In_ LSA_HANDLE AccountHandle,
    _In_ PPRIVILEGE_SET Privileges
    );

NTSTATUS
NTAPI
LsaRemovePrivilegesFromAccount(
    _In_ LSA_HANDLE AccountHandle,
    _In_ BOOLEAN AllPrivileges,
    _In_opt_ PPRIVILEGE_SET Privileges
    );

NTSTATUS
NTAPI
LsaEnumeratePrivilegesOfAccount(
    _In_ LSA_HANDLE AccountHandle,
    _Out_ PPRIVILEGE_SET *Privileges
    );

NTSTATUS
NTAPI
LsaGetQuotasForAccount(
    _In_ LSA_HANDLE AccountHandle,
    _Out_ PQUOTA_LIMITS QuotaLimits
    );

NTSTATUS
NTAPI
LsaSetQuotasForAccount(
    _In_ LSA_HANDLE AccountHandle,
    _In_ PQUOTA_LIMITS QuotaLimits
    );

NTSTATUS
NTAPI
LsaGetSystemAccessAccount(
    _In_ LSA_HANDLE AccountHandle,
    _Out_ PULONG SystemAccess
    );

NTSTATUS
NTAPI
LsaSetSystemAccessAccount(
    _In_ LSA_HANDLE AccountHandle,
    _In_ ULONG SystemAccess
    );

// Trusted Domain

#define TRUSTED_QUERY_DOMAIN_NAME 0x00000001
#define TRUSTED_QUERY_CONTROLLERS 0x00000002
#define TRUSTED_SET_CONTROLLERS 0x00000004
#define TRUSTED_QUERY_POSIX 0x00000008
#define TRUSTED_SET_POSIX 0x00000010
#define TRUSTED_SET_AUTH 0x00000020
#define TRUSTED_QUERY_AUTH 0x00000040
#define TRUSTED_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | \
    TRUSTED_QUERY_DOMAIN_NAME | \
    TRUSTED_QUERY_CONTROLLERS | \
    TRUSTED_SET_CONTROLLERS | \
    TRUSTED_QUERY_POSIX | \
    TRUSTED_SET_POSIX | \
    TRUSTED_SET_AUTH | \
    TRUSTED_QUERY_AUTH)
#define TRUSTED_READ    (STANDARD_RIGHTS_READ | \
    TRUSTED_QUERY_DOMAIN_NAME)
#define TRUSTED_WRITE   (STANDARD_RIGHTS_WRITE | \
    TRUSTED_SET_CONTROLLERS | \
    TRUSTED_SET_POSIX | \
    TRUSTED_SET_AUTH)
#define TRUSTED_EXECUTE (STANDARD_RIGHTS_EXECUTE | \
    TRUSTED_QUERY_CONTROLLERS | \
    TRUSTED_QUERY_POSIX)

NTSTATUS
NTAPI
LsaCreateTrustedDomain(
    _In_ LSA_HANDLE PolicyHandle,
    _In_ PLSA_TRUST_INFORMATION TrustedDomainInformation,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PLSA_HANDLE TrustedDomainHandle
    );

NTSTATUS
NTAPI
LsaOpenTrustedDomain(
    _In_ LSA_HANDLE PolicyHandle,
    _In_ PSID TrustedDomainSid,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PLSA_HANDLE TrustedDomainHandle
    );

NTSTATUS
NTAPI
LsaQueryInfoTrustedDomain(
    _In_ LSA_HANDLE TrustedDomainHandle,
    _In_ TRUSTED_INFORMATION_CLASS InformationClass,
    _Out_ PVOID *Buffer
    );

NTSTATUS
NTAPI
LsaSetInformationTrustedDomain(
    _In_ LSA_HANDLE TrustedDomainHandle,
    _In_ TRUSTED_INFORMATION_CLASS InformationClass,
    _In_ PVOID Buffer
    );

// Secret

#define SECRET_SET_VALUE 0x00000001
#define SECRET_QUERY_VALUE 0x00000002
#define SECRET_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | \
    SECRET_SET_VALUE | \
    SECRET_QUERY_VALUE)
#define SECRET_READ (STANDARD_RIGHTS_READ | \
    SECRET_QUERY_VALUE)
#define SECRET_WRITE (STANDARD_RIGHTS_WRITE | \
    SECRET_SET_VALUE)
#define SECRET_EXECUTE (STANDARD_RIGHTS_EXECUTE)

#define LSA_GLOBAL_SECRET_PREFIX L"G$"
#define LSA_GLOBAL_SECRET_PREFIX_LENGTH 2

#define LSA_LOCAL_SECRET_PREFIX L"L$"
#define LSA_LOCAL_SECRET_PREFIX_LENGTH 2

#define LSA_MACHINE_SECRET_PREFIX L"M$"
#define LSA_MACHINE_SECRET_PREFIX_LENGTH \
    ((sizeof(LSA_MACHINE_SECRET_PREFIX) - sizeof(WCHAR)) / sizeof(WCHAR))

#define LSA_SECRET_MAXIMUM_COUNT 0x00001000L
#define LSA_SECRET_MAXIMUM_LENGTH 0x00000200L

NTSTATUS
NTAPI
LsaCreateSecret(
    _In_ LSA_HANDLE PolicyHandle,
    _In_ PLSA_UNICODE_STRING SecretName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PLSA_HANDLE SecretHandle
    );

NTSTATUS
NTAPI
LsaOpenSecret(
    _In_ LSA_HANDLE PolicyHandle,
    _In_ PLSA_UNICODE_STRING SecretName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PLSA_HANDLE SecretHandle
    );

NTSTATUS
NTAPI
LsaSetSecret(
    _In_ LSA_HANDLE SecretHandle,
    _In_opt_ PLSA_UNICODE_STRING CurrentValue,
    _In_opt_ PLSA_UNICODE_STRING OldValue
    );

NTSTATUS
NTAPI
LsaQuerySecret(
    _In_ LSA_HANDLE SecretHandle,
    _Out_opt_ PLSA_UNICODE_STRING *CurrentValue,
    _Out_opt_ PLARGE_INTEGER CurrentValueSetTime,
    _Out_opt_ PLSA_UNICODE_STRING *OldValue,
    _Out_opt_ PLARGE_INTEGER OldValueSetTime
    );

// Privilege

NTSTATUS
NTAPI
LsaLookupPrivilegeValue(
    _In_ LSA_HANDLE PolicyHandle,
    _In_ PLSA_UNICODE_STRING Name,
    _Out_ PLUID Value
    );

NTSTATUS
NTAPI
LsaLookupPrivilegeName(
    _In_ LSA_HANDLE PolicyHandle,
    _In_ PLUID Value,
    _Out_ PLSA_UNICODE_STRING *Name
    );

NTSTATUS
NTAPI
LsaLookupPrivilegeDisplayName(
    _In_ LSA_HANDLE PolicyHandle,
    _In_ PLSA_UNICODE_STRING Name,
    _Out_ PLSA_UNICODE_STRING *DisplayName,
    _Out_ PSHORT LanguageReturned
    );

// Misc.

NTSTATUS
NTAPI
LsaChangePassword(
    _In_ PLSA_UNICODE_STRING ServerName,
    _In_ PLSA_UNICODE_STRING DomainName,
    _In_ PLSA_UNICODE_STRING AccountName,
    _In_ PLSA_UNICODE_STRING OldPassword,
    _In_ PLSA_UNICODE_STRING NewPassword
    );

NTSTATUS
NTAPI
LsaGetUserName(
    _Outptr_ PLSA_UNICODE_STRING *UserName,
    _Outptr_opt_ PLSA_UNICODE_STRING *DomainName
    );

NTSTATUS
NTAPI
LsaGetRemoteUserName(
    _In_opt_ PLSA_UNICODE_STRING SystemName,
    _Outptr_ PLSA_UNICODE_STRING *UserName,
    _Outptr_opt_ PLSA_UNICODE_STRING *DomainName
    );

#endif
