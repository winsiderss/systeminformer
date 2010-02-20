#ifndef NTLSA_H
#define NTLSA_H

#define _NTDEF_
#include <ntsecapi.h>

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

NTSTATUS NTAPI LsaDelete(
    __in LSA_HANDLE ObjectHandle
    );

NTSTATUS NTAPI LsaQuerySecurityObject(
    __in LSA_HANDLE ObjectHandle,
    __in SECURITY_INFORMATION SecurityInformation,
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

NTSTATUS NTAPI LsaSetSecurityObject(
    __in LSA_HANDLE ObjectHandle,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS NTAPI LsaChangePassword(
    __in PLSA_UNICODE_STRING ServerName,
    __in PLSA_UNICODE_STRING DomainName,
    __in PLSA_UNICODE_STRING AccountName,
    __in PLSA_UNICODE_STRING OldPassword,
    __in PLSA_UNICODE_STRING NewPassword
    );

NTSTATUS NTAPI LsaCreateAccount(
    __in LSA_HANDLE PolicyHandle,
    __in PSID AccountSid,
    __in ACCESS_MASK DesiredAccess,
    __out PLSA_HANDLE AccountHandle
    );

NTSTATUS NTAPI LsaEnumerateAccounts(
    __in LSA_HANDLE PolicyHandle,
    __inout PLSA_ENUMERATION_HANDLE EnumerationContext,
    __out PVOID *Buffer,
    __in ULONG PreferedMaximumLength,
    __out PULONG CountReturned
    );

NTSTATUS NTAPI LsaCreateTrustedDomain(
    __in LSA_HANDLE PolicyHandle,
    __in PLSA_TRUST_INFORMATION TrustedDomainInformation,
    __in ACCESS_MASK DesiredAccess,
    __out PLSA_HANDLE TrustedDomainHandle
    );

NTSTATUS NTAPI LsaEnumeratePrivileges(
    __in LSA_HANDLE PolicyHandle,
    __inout PLSA_ENUMERATION_HANDLE EnumerationContext,
    __out PVOID *Buffer,
    __in ULONG PreferedMaximumLength,
    __out PULONG CountReturned
    );

NTSTATUS NTAPI LsaCreateSecret(
    __in LSA_HANDLE PolicyHandle,
    __in PLSA_UNICODE_STRING SecretName,
    __in ACCESS_MASK DesiredAccess,
    __out PLSA_HANDLE SecretHandle
    );

NTSTATUS NTAPI LsaOpenAccount(
    __in LSA_HANDLE PolicyHandle,
    __in PSID AccountSid,
    __in ACCESS_MASK DesiredAccess,
    __out PLSA_HANDLE AccountHandle
    );

NTSTATUS NTAPI LsaEnumeratePrivilegesOfAccount(
    __in LSA_HANDLE AccountHandle,
    __out PPRIVILEGE_SET *Privileges
    );

NTSTATUS NTAPI LsaAddPrivilegesToAccount(
    __in LSA_HANDLE AccountHandle,
    __in PPRIVILEGE_SET Privileges
    );

NTSTATUS NTAPI LsaRemovePrivilegesFromAccount(
    __in LSA_HANDLE AccountHandle,
    __in BOOLEAN AllPrivileges,
    __in_opt PPRIVILEGE_SET Privileges
    );

NTSTATUS NTAPI LsaGetQuotasForAccount(
    __in LSA_HANDLE AccountHandle,
    __out PQUOTA_LIMITS QuotaLimits
    );

NTSTATUS NTAPI LsaSetQuotasForAccount(
    __in LSA_HANDLE AccountHandle,
    __in PQUOTA_LIMITS QuotaLimits
    );

NTSTATUS NTAPI LsaGetSystemAccessAccount(
    __in LSA_HANDLE AccountHandle,
    __out PULONG SystemAccess
    );

NTSTATUS NTAPI LsaSetSystemAccessAccount(
    __in LSA_HANDLE AccountHandle,
    __in ULONG SystemAccess
    );

NTSTATUS NTAPI LsaOpenTrustedDomain(
    __in LSA_HANDLE PolicyHandle,
    __in PSID TrustedDomainSid,
    __in ACCESS_MASK DesiredAccess,
    __out PLSA_HANDLE TrustedDomainHandle
    );

NTSTATUS NTAPI LsaQueryInfoTrustedDomain(
    __in LSA_HANDLE TrustedDomainHandle,
    __in TRUSTED_INFORMATION_CLASS InformationClass,
    __out PVOID *Buffer
    );

NTSTATUS NTAPI LsaSetInformationTrustedDomain(
    __in LSA_HANDLE TrustedDomainHandle,
    __in TRUSTED_INFORMATION_CLASS InformationClass,
    __in PVOID Buffer
    );

NTSTATUS NTAPI LsaOpenSecret(
    __in LSA_HANDLE PolicyHandle,
    __in PLSA_UNICODE_STRING SecretName,
    __in ACCESS_MASK DesiredAccess,
    __out PLSA_HANDLE SecretHandle
    );

NTSTATUS NTAPI LsaSetSecret(
    __in LSA_HANDLE SecretHandle,
    __in_opt PLSA_UNICODE_STRING CurrentValue,
    __in_opt PLSA_UNICODE_STRING OldValue
    );

NTSTATUS NTAPI LsaQuerySecret(
    __in LSA_HANDLE SecretHandle,
    __out_opt OPTIONAL PLSA_UNICODE_STRING *CurrentValue,
    __out_opt PLARGE_INTEGER CurrentValueSetTime,
    __out_opt PLSA_UNICODE_STRING *OldValue,
    __out_opt PLARGE_INTEGER OldValueSetTime
    );

NTSTATUS NTAPI LsaLookupPrivilegeValue(
    __in LSA_HANDLE PolicyHandle,
    __in PLSA_UNICODE_STRING Name,
    __out PLUID Value
    );

NTSTATUS NTAPI LsaLookupPrivilegeName(
    __in LSA_HANDLE PolicyHandle,
    __in PLUID Value,
    __out PLSA_UNICODE_STRING *Name
    );

NTSTATUS NTAPI LsaLookupPrivilegeDisplayName(
    __in LSA_HANDLE PolicyHandle,
    __in PLSA_UNICODE_STRING Name,
    __out PLSA_UNICODE_STRING *DisplayName,
    __out PSHORT LanguageReturned
    );

NTSTATUS NTAPI LsaGetUserName(
    __deref_out PLSA_UNICODE_STRING *UserName,
    __deref_opt_out PLSA_UNICODE_STRING *DomainName
    );

NTSTATUS NTAPI LsaGetRemoteUserName(
    __in_opt PLSA_UNICODE_STRING SystemName,
    __deref_out PLSA_UNICODE_STRING *UserName,
    __deref_opt_out PLSA_UNICODE_STRING *DomainName
    );

#endif
