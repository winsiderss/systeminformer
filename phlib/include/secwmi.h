/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2023
 *
 */

#ifndef _PH_SECWMI_H
#define _PH_SECWMI_H

EXTERN_C_START

PHLIBAPI
HRESULT
NTAPI
PhGetWbemLocatorClass(
    _Out_ struct IWbemLocator** WbemLocatorClass
    );

PHLIBAPI
HRESULT
NTAPI
PhCoSetProxyBlanket(
    _In_ IUnknown* InterfacePtr
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetWbemClassObjectString(
    _In_ PVOID WbemClassObject,
    _In_ PCWSTR Name
    );

#define PhStringRefToBSTR(String) \
    SysAllocStringLen((String)->Buffer, (UINT)(String)->Length / sizeof(WCHAR))
#define PhStringZToBSTR(String) \
    SysAllocStringLen((String), (UINT)sizeof(String) - sizeof(UNICODE_NULL) / sizeof(WCHAR))

// powrprof.h
typedef enum _POWER_DATA_ACCESSOR POWER_DATA_ACCESSOR;

// rev
typedef ULONG (WINAPI* _PowerReadSecurityDescriptor)(
    _In_ POWER_DATA_ACCESSOR AccessFlags,
    _In_ PGUID PowerGuid,
    _Out_ PWSTR* StringSecurityDescriptor
    );

// rev
typedef ULONG (WINAPI* _PowerWriteSecurityDescriptor)(
    _In_ POWER_DATA_ACCESSOR AccessFlags,
    _In_ PGUID PowerGuid,
    _In_ PCWSTR StringSecurityDescriptor
    );

// Power policy

NTSTATUS PhpGetPowerPolicySecurityDescriptor(
    _Out_ PPH_STRING* StringSecurityDescriptor
    );

NTSTATUS PhpSetPowerPolicySecurityDescriptor(
    _In_ PPH_STRING StringSecurityDescriptor
    );

// Terminal server policy

NTSTATUS PhpGetRemoteDesktopSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    );

NTSTATUS PhpSetRemoteDesktopSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

// Wbem namespace policy

NTSTATUS PhGetWmiNamespaceSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    );

NTSTATUS PhSetWmiNamespaceSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

// Defender offline scan

HRESULT PhRestartDefenderOfflineScan(
    VOID
    );

EXTERN_C_END

#endif
