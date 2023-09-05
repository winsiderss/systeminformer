/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2018-2023
 *
 */

#ifndef _PH_SVCSUP_H
#define _PH_SVCSUP_H

#ifdef __cplusplus
extern "C" {
#endif

extern CONST PPH_STRINGREF PhServiceTypeStrings[12];
extern CONST PPH_STRINGREF PhServiceStartTypeStrings[5];
extern CONST PPH_STRINGREF PhServiceErrorControlStrings[4];

typedef SC_HANDLE* PSC_HANDLE;

PHLIBAPI
SC_HANDLE
NTAPI
PhGetServiceManagerHandle(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumServices(
    _Out_ LPENUM_SERVICE_STATUS_PROCESS* Services,
    _Out_ PULONG NumberOfServices
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumDependentServices(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ LPENUM_SERVICE_STATUS* DependentServices,
    _Out_ PULONG NumberOfDependentServices
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenService(
    _Out_ PSC_HANDLE ServiceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PWSTR ServiceName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenServiceKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_STRINGREF ServiceName
    );

PHLIBAPI
VOID
NTAPI
PhCloseServiceHandle(
    _In_ SC_HANDLE ServiceHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateService(
    _Out_ PSC_HANDLE ServiceHandle,
    _In_ PCWSTR ServiceName,
    _In_opt_ PCWSTR DisplayName,
    _In_ ULONG DesiredAccess,
    _In_ ULONG ServiceType,
    _In_ ULONG StartType,
    _In_ ULONG ErrorControl,
    _In_opt_ PCWSTR BinaryPathName,
    _In_opt_ PCWSTR UserName,
    _In_opt_ PCWSTR Password
    );


PHLIBAPI
NTSTATUS
NTAPI
PhChangeServiceConfig(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG ServiceType,
    _In_ ULONG StartType,
    _In_ ULONG ErrorControl,
    _In_opt_ PCWSTR BinaryPathName,
    _In_opt_ PCWSTR LoadOrderGroup,
    _Out_opt_ PULONG TagId,
    _In_opt_ PCWSTR Dependencies,
    _In_opt_ PCWSTR ServiceStartName,
    _In_opt_ PCWSTR Password,
    _In_opt_ PCWSTR DisplayName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhChangeServiceConfig2(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG ServiceConfigLevel,
    _In_opt_ PVOID Buffer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryServiceConfig(
    _In_ SC_HANDLE ServiceHandle,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryServiceConfig2(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG ServiceConfigLevel,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryServiceObjectSecurity(
    _In_ SC_HANDLE ServiceHandle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_writes_bytes_opt_(SecurityDescriptorLength) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG SecurityDescriptorLength,
    _Out_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetServiceObjectSecurity(
    _In_ SC_HANDLE ServiceHandle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetServiceObjectSecurity(
    _In_ SC_HANDLE ServiceHandle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryServiceStatus(
    _In_ SC_HANDLE ServiceHandle,
    _Inout_ LPSERVICE_STATUS_PROCESS ServiceStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
PhContinueService(
    _In_ SC_HANDLE ServiceHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhPauseService(
    _In_ SC_HANDLE ServiceHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteService(
    _In_ SC_HANDLE ServiceHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhStartService(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG NumberOfServiceArgs,
    _In_reads_opt_(NumberOfServiceArgs) PCWSTR* ServiceArgVectors
    );

PHLIBAPI
NTSTATUS
NTAPI
PhStopService(
    _In_ SC_HANDLE ServiceHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetServiceConfig(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ LPQUERY_SERVICE_CONFIG* ServiceConfig
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryServiceVariableSize(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG InfoLevel,
    _Out_ PVOID* ServiceConfig
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServiceDescription(
    _In_ SC_HANDLE ServiceHandle
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetServiceDelayedAutoStart(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ PBOOLEAN DelayedAutoStart
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSetServiceDelayedAutoStart(
    _In_ SC_HANDLE ServiceHandle,
    _In_ BOOLEAN DelayedAutoStart
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetServiceTriggerInfo(
    _In_ SC_HANDLE ServiceHandle,
    _Out_opt_ PSERVICE_TRIGGER_INFO* ServiceTriggerInfo
    );

PHLIBAPI
PPH_STRINGREF
NTAPI
PhGetServiceStateString(
    _In_ ULONG ServiceState
    );

PHLIBAPI
PPH_STRINGREF
NTAPI
PhGetServiceTypeString(
    _In_ ULONG ServiceType
    );

PHLIBAPI
ULONG
NTAPI
PhGetServiceTypeInteger(
    _In_ PPH_STRINGREF ServiceType
    );

PHLIBAPI
PPH_STRINGREF
NTAPI
PhGetServiceStartTypeString(
    _In_ ULONG ServiceStartType
    );

PHLIBAPI
ULONG
NTAPI
PhGetServiceStartTypeInteger(
    _In_ PPH_STRINGREF ServiceStartType
    );

PHLIBAPI
PPH_STRINGREF
NTAPI
PhGetServiceErrorControlString(
    _In_ ULONG ServiceErrorControl
    );

PHLIBAPI
ULONG
NTAPI
PhGetServiceErrorControlInteger(
    _In_ PPH_STRINGREF ServiceErrorControl
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServiceNameFromTag(
    _In_ HANDLE ProcessId,
    _In_ PVOID ServiceTag
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServiceNameForModuleReference(
    _In_ HANDLE ProcessId,
    _In_ PWSTR ModuleName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadServiceTag(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle,
    _Out_ PVOID *ServiceTag
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServiceKeyName(
    _In_ PPH_STRINGREF ServiceName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServiceParametersKeyName(
    _In_ PPH_STRINGREF ServiceName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServiceConfigFileName(
    _In_ ULONG ServiceType,
    _In_ PWSTR ServicePathName,
    _In_ PPH_STRINGREF ServiceName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServiceHandleFileName(
    _In_ SC_HANDLE ServiceHandle,
    _In_ PPH_STRINGREF ServiceName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServiceFileName(
    _In_ PPH_STRINGREF ServiceName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetServiceDllParameter(
    _In_ ULONG ServiceType,
    _In_ PPH_STRINGREF ServiceName,
    _Out_ PPH_STRING *ServiceDll
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServiceAppUserModelId(
    _In_ PPH_STRINGREF ServiceName
    );

PHLIBAPI
ULONG
NTAPI
PhGetServiceBootFlags(
    _In_ PPH_STRINGREF ServiceName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServicePackageFullName(
    _In_ PPH_STRINGREF ServiceName
    );

FORCEINLINE
VOID
NTAPI
PhServiceWorkaroundWindowsServiceTypeBug(
    _Inout_ LPENUM_SERVICE_STATUS_PROCESS ServiceEntry
    )
{
    // SERVICE_WIN32 is not a valid ServiceType: https://github.com/winsiderss/systeminformer/issues/120 (dmex)
    if (ServiceEntry->ServiceStatusProcess.dwServiceType == SERVICE_WIN32)
        ServiceEntry->ServiceStatusProcess.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    if (ServiceEntry->ServiceStatusProcess.dwServiceType == (SERVICE_WIN32 | SERVICE_USER_SHARE_PROCESS | SERVICE_USERSERVICE_INSTANCE))
        ServiceEntry->ServiceStatusProcess.dwServiceType = SERVICE_USER_SHARE_PROCESS | SERVICE_USERSERVICE_INSTANCE;
}

#ifdef __cplusplus
}
#endif

#endif
