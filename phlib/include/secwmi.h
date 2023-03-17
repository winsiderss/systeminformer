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

// Import typedefs

typedef ULONG (WINAPI* _PowerGetActiveScheme)(
    _In_opt_ HKEY UserRootPowerKey,
    _Out_ PGUID* ActivePolicyGuid
    );

typedef ULONG (WINAPI* _PowerSetActiveScheme)(
    _In_opt_ HKEY UserRootPowerKey,
    _In_ PGUID SchemeGuid
    );

typedef ULONG (WINAPI* _PowerRestoreDefaultPowerSchemes)(
    VOID
    );

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
    _In_ PWSTR StringSecurityDescriptor
    );

typedef BOOL (WINAPI* _WTSGetListenerSecurity)(
    _In_opt_ HANDLE ServerHandle,
    _In_opt_ PVOID Reserved1,
    _In_opt_ ULONG Reserved2,
    _In_ PWSTR ListenerName,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Length,
    _Out_ PULONG LengthNeeded
    );

typedef BOOL (WINAPI* _WTSSetListenerSecurity)(
    _In_opt_ HANDLE ServerHandle,
    _In_opt_ PVOID Reserved1,
    _In_opt_ ULONG Reserved2,
    _In_ PWSTR ListenerName,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

PVOID PhGetWbemProxImageBaseAddress(
    VOID
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
