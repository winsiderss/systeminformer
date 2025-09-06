/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2018-2022
 *
 */

#ifndef _PH_LSASUP_H
#define _PH_LSASUP_H

#ifdef __cplusplus
extern "C" {
#endif

PHLIBAPI
NTSTATUS
NTAPI
PhOpenLsaPolicy(
    _Out_ PLSA_HANDLE PolicyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PUNICODE_STRING SystemName
    );

PHLIBAPI
LSA_HANDLE
NTAPI
PhGetLookupPolicyHandle(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLookupPrivilegeName(
    _In_ PLUID PrivilegeValue,
    _Out_ PPH_STRING *PrivilegeName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLookupPrivilegeDisplayName(
    _In_ PPH_STRINGREF PrivilegeName,
    _Out_ PPH_STRING *PrivilegeDisplayName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLookupPrivilegeValue(
    _In_ PPH_STRINGREF PrivilegeName,
    _Out_ PLUID PrivilegeValue
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLookupSid(
    _In_ PSID Sid,
    _Out_opt_ PPH_STRING *Name,
    _Out_opt_ PPH_STRING *DomainName,
    _Out_opt_ PSID_NAME_USE NameUse
    );

PHLIBAPI
VOID
NTAPI
PhLookupSids(
    _In_ ULONG Count,
    _In_ PSID *Sids,
    _Out_ PPH_STRING **FullNames
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLookupName(
    _In_ PPH_STRINGREF Name,
    _Out_opt_ PSID *Sid,
    _Out_opt_ PPH_STRING *DomainName,
    _Out_opt_ PSID_NAME_USE NameUse
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetSidFullName(
    _In_ PSID Sid,
    _In_ BOOLEAN IncludeDomain,
    _Out_opt_ PSID_NAME_USE NameUse
    );

PHLIBAPI
PPH_STRING
NTAPI
PhSidToStringSid(
    _In_ PSID Sid
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSidToBuffer(
    _In_ PSID Sid,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_ USHORT BufferLength,
    _Out_opt_ PUSHORT ReturnLength
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetTokenUserString(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN IncludeDomain
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetAccountPrivileges(
    _In_ PSID AccountSid,
    _Out_ PTOKEN_PRIVILEGES* Privileges
    );

typedef NTSTATUS (NTAPI* PPH_ENUM_PRIVILEGES)(
    _In_ PPOLICY_PRIVILEGE_DEFINITION Privileges,
    _In_ ULONG NumberOfPrivileges,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumeratePrivileges(
    _In_ PPH_ENUM_PRIVILEGES Callback,
    _In_opt_ PVOID Context
    );

typedef enum _LSA_USER_ACCOUNT_TYPE
{
    UnknownUserAccountType,
    LocalUserAccountType,
    PrimaryDomainUserAccountType,
    ExternalDomainUserAccountType,
    LocalConnectedUserAccountType,
    AADUserAccountType,
    InternetUserAccountType,
    MSAUserAccountType
} LSA_USER_ACCOUNT_TYPE, *PLSA_USER_ACCOUNT_TYPE;

PHLIBAPI
NTSTATUS
NTAPI
PhGetSidAccountType(
    _In_ PSID Sid,
    _Out_ PLSA_USER_ACCOUNT_TYPE AccountType
    );

PHLIBAPI
PCWSTR
NTAPI
PhGetSidAccountTypeString(
    _In_ PSID Sid
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetCapabilitySidName(
    _In_ PSID CapabilitySid
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetCapabilityGuidName(
    _In_ PPH_STRING GuidString
    );

PHLIBAPI
BOOLEAN
NTAPI
PhBuildTrusteeWithSid(
    _Out_ PVOID Trustee,
    _In_opt_ PSID Sid
    );

PHLIBAPI
VOID
NTAPI
PhMapGenericMask(
    _Inout_ PACCESS_MASK AccessMask,
    _In_ PGENERIC_MAPPING GenericMapping
    );

typedef _Function_class_(PH_ENUM_ACCOUNT_CALLBACK)
NTSTATUS PH_ENUM_ACCOUNT_CALLBACK(
    _In_ PPH_STRINGREF AccountName,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_ACCOUNT_CALLBACK* PPH_ENUM_ACCOUNT_CALLBACK;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumerateAccounts(
    _In_ PPH_ENUM_ACCOUNT_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateServiceSidToBuffer(
    _In_ PPH_STRINGREF ServiceName,
    _Out_writes_bytes_opt_(*ServiceSidLength) PSID ServiceSid,
    _Inout_ PULONG ServiceSidLength
    );

PHLIBAPI
PPH_STRING
NTAPI
PhCreateServiceSidToStringSid(
    _In_ PPH_STRINGREF ServiceName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetAzureDirectoryObjectSid(
    _In_ PSID ActiveDirectorySid
    );

#ifdef __cplusplus
}
#endif

#endif
