/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2023
 *
 */
#ifndef _PH_LSAMSUP_H
#define _PH_LSAMSUP_H

EXTERN_C_START

PHLIBAPI
NTSTATUS
NTAPI
PhOpenSamHandle(
    _Out_ PSAM_HANDLE SamHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PUNICODE_STRING SystemName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSamServerHandle(
    _Out_ PSAM_HANDLE SamHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenSamDomainHandle(
    _Out_ PSAM_HANDLE DomainHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PSID DomainSid
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSamDomainHandle(
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PSAM_HANDLE DomainHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLookupSamName(
    _In_ SAM_HANDLE DomainHandle,
    _In_ PPH_STRINGREF Name,
    _Out_ PULONG RelativeId,
    _Out_ PSID_NAME_USE Use
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLookupSamNames(
    _In_ SAM_HANDLE DomainHandle,
    _In_ ULONG Count,
    _In_ PPH_STRINGREF Names,
    _Out_ PULONG* RelativeIds,
    _Out_ PSID_NAME_USE* Uses
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSamAliasDescription(
    _In_ PSID Sid,
    _Out_ PPH_STRING *Description
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSamGroupDescription(
    _In_ PSID Sid,
    _Out_ PPH_STRING *Description
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSamUserDescription(
    _In_ PSID Sid,
    _Out_ PPH_STRING *Description
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetSamAccountDescription(
    _In_ PSID Sid
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryLogonInformation(
    _In_ PCPH_STRINGREF DomainName,
    _In_ PCPH_STRINGREF UserName,
    _Out_ PUSER_LOGON_INFORMATION* LogonInfo
    );

EXTERN_C_END

#endif // _PH_LSAMSUP_H
