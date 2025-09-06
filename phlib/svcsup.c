/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2012
 *     dmex    2019-2024
 *
 */

#include <ph.h>
#include <subprocesstag.h>
#include <svcsup.h>
#include <mapldr.h>

static CONST PH_STRINGREF PhpServiceUnknownString = PH_STRINGREF_INIT(L"Unknown");

static CONST PH_KEY_VALUE_PAIR PhpServiceStatePairs[] =
{
    SIP(SREF(L"Unknown"), 0),
    SIP(SREF(L"Stopped"), SERVICE_STOPPED),
    SIP(SREF(L"Start pending"), SERVICE_START_PENDING),
    SIP(SREF(L"Stop pending"), SERVICE_STOP_PENDING),
    SIP(SREF(L"Running"), SERVICE_RUNNING),
    SIP(SREF(L"Continue pending"), SERVICE_CONTINUE_PENDING),
    SIP(SREF(L"Pause pending"), SERVICE_PAUSE_PENDING),
    SIP(SREF(L"Paused"), SERVICE_PAUSED)
};

static CONST PH_KEY_VALUE_PAIR PhpServiceTypePairs[] =
{
    SIP(SREF(L"Unknown"), 0),
    SIP(SREF(L"Driver"), SERVICE_KERNEL_DRIVER),
    SIP(SREF(L"FS driver"), SERVICE_FILE_SYSTEM_DRIVER),
    SIP(SREF(L"Own process"), SERVICE_WIN32_OWN_PROCESS),
    SIP(SREF(L"Share process"), SERVICE_WIN32_SHARE_PROCESS),
    SIP(SREF(L"Own interactive process"), SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS),
    SIP(SREF(L"Share interactive process"), SERVICE_WIN32_SHARE_PROCESS | SERVICE_INTERACTIVE_PROCESS),
    SIP(SREF(L"User own process"), SERVICE_USER_OWN_PROCESS),
    SIP(SREF(L"User own process (instance)"), SERVICE_USER_OWN_PROCESS | SERVICE_USERSERVICE_INSTANCE),
    SIP(SREF(L"User share process"), SERVICE_USER_SHARE_PROCESS),
    SIP(SREF(L"User share process (instance)"), SERVICE_USER_SHARE_PROCESS | SERVICE_USERSERVICE_INSTANCE),
    SIP(SREF(L"Package own process"), SERVICE_PKG_SERVICE | SERVICE_WIN32_OWN_PROCESS),
    SIP(SREF(L"Package share process"), SERVICE_PKG_SERVICE | SERVICE_WIN32_SHARE_PROCESS),
};

static CONST PH_KEY_VALUE_PAIR PhpServiceStartTypePairs[] =
{
    SIP(SREF(L"Boot start"), SERVICE_BOOT_START),
    SIP(SREF(L"System start"), SERVICE_SYSTEM_START),
    SIP(SREF(L"Auto start"), SERVICE_AUTO_START),
    SIP(SREF(L"Demand start"), SERVICE_DEMAND_START),
    SIP(SREF(L"Disabled"), SERVICE_DISABLED),
};

static CONST PH_KEY_VALUE_PAIR PhpServiceErrorControlPairs[] =
{
    SIP(SREF(L"Ignore"), SERVICE_ERROR_IGNORE),
    SIP(SREF(L"Normal"), SERVICE_ERROR_NORMAL),
    SIP(SREF(L"Severe"), SERVICE_ERROR_SEVERE),
    SIP(SREF(L"Critical"), SERVICE_ERROR_CRITICAL)
};

CONST PPH_STRINGREF PhServiceTypeStrings[] =
{
    SREF(L"Driver"),
    SREF(L"FS driver"),
    SREF(L"Own process"),
    SREF(L"Share process"),
    SREF(L"Own interactive process"),
    SREF(L"Share interactive process"),
    SREF(L"User own process"),
    SREF(L"User own process (instance)"),
    SREF(L"User share process"),
    SREF(L"User share process (instance)"),
    SREF(L"Package own process"),
    SREF(L"Package share process"),
};

CONST PPH_STRINGREF PhServiceStartTypeStrings[5] =
{
    SREF(L"Disabled"),
    SREF(L"Boot start"),
    SREF(L"System start"),
    SREF(L"Auto start"),
    SREF(L"Demand start"),
};

CONST PPH_STRINGREF PhServiceErrorControlStrings[4] =
{
    SREF(L"Ignore"),
    SREF(L"Normal"),
    SREF(L"Severe"),
    SREF(L"Critical"),
};

SC_HANDLE PhGetServiceManagerHandle(
    VOID
    )
{
    static SC_HANDLE cachedServiceManagerHandle = NULL;
    SC_HANDLE serviceManagerHandle;
    SC_HANDLE newServiceManagerHandle;

    // Use the cached value if possible.

    serviceManagerHandle = ReadPointerAcquire(&cachedServiceManagerHandle);

    // If there is no cached handle, open one.

    if (!serviceManagerHandle)
    {
        if (newServiceManagerHandle = OpenSCManager(
            NULL,
            NULL,
            SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE
            ))
        {
            // We succeeded in opening a policy handle, and since we did not have a cached handle
            // before, we will now store it.
            serviceManagerHandle = InterlockedCompareExchangePointer(
                &cachedServiceManagerHandle,
                newServiceManagerHandle,
                NULL
                );

            if (!serviceManagerHandle)
            {
                // Success. Use our handle.
                serviceManagerHandle = newServiceManagerHandle;
            }
            else
            {
                // Someone already placed a handle in the cache. Close our handle and use their handle.
                PhCloseServiceHandle(newServiceManagerHandle);
            }
        }
    }

    return serviceManagerHandle;
}

NTSTATUS PhEnumServices(
    _Out_ LPENUM_SERVICE_STATUS_PROCESS* Services,
    _Out_ PULONG NumberOfServices
    )
{
    static ULONG initialBufferSize = 0x8000;
    NTSTATUS status;
    SC_HANDLE scManagerHandle;
    ULONG type;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;
    ULONG servicesReturned;

    if (WindowsVersion >= WINDOWS_10_RS1)
    {
        type = SERVICE_TYPE_ALL;
    }
    else if (WindowsVersion >= WINDOWS_10)
    {
        type = SERVICE_WIN32 |
            SERVICE_ADAPTER |
            SERVICE_DRIVER |
            SERVICE_INTERACTIVE_PROCESS |
            SERVICE_USER_SERVICE |
            SERVICE_USERSERVICE_INSTANCE;
    }
    else
    {
        type = SERVICE_DRIVER | SERVICE_WIN32;
    }

    scManagerHandle = PhGetServiceManagerHandle();
    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    if (EnumServicesStatusEx(
        scManagerHandle,
        SC_ENUM_PROCESS_INFO,
        type,
        SERVICE_STATE_ALL,
        (PBYTE)buffer,
        bufferSize,
        &returnLength,
        &servicesReturned,
        NULL,
        NULL
        ))
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = PhGetLastWin32ErrorAsNtStatus();
    }

    if (status == STATUS_MORE_ENTRIES)
    {
        PhFree(buffer);
        bufferSize += returnLength;
        buffer = PhAllocate(bufferSize);

        if (EnumServicesStatusEx(
            scManagerHandle,
            SC_ENUM_PROCESS_INFO,
            type,
            SERVICE_STATE_ALL,
            (PBYTE)buffer,
            bufferSize,
            &returnLength,
            &servicesReturned,
            NULL,
            NULL
            ))
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = PhGetLastWin32ErrorAsNtStatus();
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x20000) initialBufferSize = bufferSize;
    *Services = buffer;
    *NumberOfServices = servicesReturned;

    return status;
}

NTSTATUS PhEnumDependentServices(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ LPENUM_SERVICE_STATUS* DependentServices,
    _Out_ PULONG NumberOfDependentServices
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;
    ULONG servicesReturned;

    bufferSize = 0x800;
    buffer = PhAllocate(bufferSize);

    if (EnumDependentServices(
        ServiceHandle,
        SERVICE_STATE_ALL,
        buffer,
        bufferSize,
        &returnLength,
        &servicesReturned
        ))
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = PhGetLastWin32ErrorAsNtStatus();
    }

    if (status == STATUS_MORE_ENTRIES)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        if (EnumDependentServices(
            ServiceHandle,
            SERVICE_STATE_ALL,
            buffer,
            bufferSize,
            &returnLength,
            &servicesReturned
            ))
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = PhGetLastWin32ErrorAsNtStatus();
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *DependentServices = buffer;
    *NumberOfDependentServices = servicesReturned;

    return status;
}

NTSTATUS PhOpenServiceManager(
    _Out_ PSC_HANDLE ServiceManagerHandle,
    _In_opt_ PCWSTR DatabaseName,
    _In_ ACCESS_MASK DesiredAccess
    )
{
    SC_HANDLE serviceManagerHandle;

    if (serviceManagerHandle = OpenSCManager(NULL, DatabaseName, DesiredAccess))
    {
        *ServiceManagerHandle = serviceManagerHandle;
        return STATUS_SUCCESS;
    }
    else
    {
        return PhGetLastWin32ErrorAsNtStatus();
    }
}

NTSTATUS PhOpenService(
    _Out_ PSC_HANDLE ServiceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCWSTR ServiceName
    )
{
    SC_HANDLE serviceHandle;

    if (serviceHandle = OpenService(PhGetServiceManagerHandle(), ServiceName, DesiredAccess))
    {
        *ServiceHandle = serviceHandle;
        return STATUS_SUCCESS;
    }

    *ServiceHandle = NULL;
    return PhGetLastWin32ErrorAsNtStatus();
}

NTSTATUS PhOpenServiceKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_STRINGREF ServiceName
    )
{
    static CONST PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services");
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HANDLE servicesKeyHandle = NULL;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PPH_STRING serviceName;

    if (PhBeginInitOnce(&initOnce))
    {
        status = PhOpenKey(&servicesKeyHandle, KEY_READ, PH_KEY_LOCAL_MACHINE, &servicesKeyName, 0);
        PhEndInitOnce(&initOnce);
    }

    if (servicesKeyHandle)
    {
        status = PhOpenKey(KeyHandle, DesiredAccess, servicesKeyHandle, ServiceName, 0);
    }

    if (NT_SUCCESS(status))
    {
        if (serviceName = PhGetServiceKeyName(ServiceName))
        {
            status = PhOpenKey(
                KeyHandle,
                DesiredAccess,
                PH_KEY_LOCAL_MACHINE,
                &serviceName->sr,
                0
                );

            PhDereferenceObject(serviceName);
        }
        else
        {
            status = STATUS_NO_MEMORY;
        }
    }

    return status;
}

VOID PhCloseServiceHandle(
    _In_ SC_HANDLE ServiceHandle
    )
{
    CloseServiceHandle(ServiceHandle);
}

NTSTATUS PhCreateService(
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
    )
{
    NTSTATUS status;
    SC_HANDLE scManagerHandle;
    SC_HANDLE serviceHandle;

    status = PhOpenServiceManager(&scManagerHandle, NULL, SC_MANAGER_CREATE_SERVICE);

    if (NT_SUCCESS(status))
    {
        if (serviceHandle = CreateService(
            scManagerHandle,
            ServiceName,
            DisplayName,
            DesiredAccess,
            ServiceType,
            StartType,
            ErrorControl,
            BinaryPathName,
            NULL,
            NULL,
            NULL,
            UserName,
            Password
            ))
        {
            *ServiceHandle = serviceHandle;
            status = STATUS_SUCCESS;
        }
        else
        {
            *ServiceHandle = NULL;
            status = PhGetLastWin32ErrorAsNtStatus();
        }

        PhCloseServiceHandle(scManagerHandle);
    }

    return status;
}

NTSTATUS PhChangeServiceConfig(
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
    )
{
    NTSTATUS status;

    if (ChangeServiceConfig(
        ServiceHandle,
        ServiceType,
        StartType,
        ErrorControl,
        BinaryPathName,
        LoadOrderGroup,
        TagId,
        Dependencies,
        ServiceStartName,
        Password,
        DisplayName
        ))
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = PhGetLastWin32ErrorAsNtStatus();
    }

    return status;
}

NTSTATUS PhChangeServiceConfig2(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG ServiceConfigLevel,
    _In_opt_ PVOID Buffer
    )
{
    NTSTATUS status;

    if (ChangeServiceConfig2(ServiceHandle, ServiceConfigLevel, Buffer))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    return status;
}

NTSTATUS PhEnumDependentServices2(
    _In_ SC_HANDLE ServiceHandle,
    _Out_writes_bytes_opt_(BufferLength) LPENUM_SERVICE_STATUSW Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength,
    _Out_ PULONG NumberOfServices
    )
{
    NTSTATUS status;

    if (EnumDependentServices(ServiceHandle, SERVICE_STATE_ALL, Buffer, BufferLength, ReturnLength, NumberOfServices))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    return status;
}

NTSTATUS PhQueryServiceConfig(
    _In_ SC_HANDLE ServiceHandle,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    ULONG returnLength = 0;

    if (QueryServiceConfig(ServiceHandle, Buffer, BufferLength, &returnLength))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    if (ReturnLength)
        *ReturnLength = returnLength;

    return status;
}

NTSTATUS PhQueryServiceConfig2(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG ServiceConfigLevel,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    ULONG returnLength = 0;

    if (QueryServiceConfig2(ServiceHandle, ServiceConfigLevel, (PBYTE)Buffer, BufferLength, &returnLength))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    if (ReturnLength)
        *ReturnLength = returnLength;

    return status;
}

NTSTATUS PhGetServiceObjectSecurity(
    _In_ SC_HANDLE ServiceHandle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;

    if (QueryServiceObjectSecurity(ServiceHandle, SecurityInformation, NULL, 0, &bufferSize))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    if (status == STATUS_BUFFER_TOO_SMALL && bufferSize)
    {
        buffer = PhAllocate(bufferSize);
        memset(buffer, 0, bufferSize);

        if (QueryServiceObjectSecurity(ServiceHandle, SecurityInformation, buffer, bufferSize, &bufferSize))
            status = STATUS_SUCCESS;
        else
            status = PhGetLastWin32ErrorAsNtStatus();

        if (NT_SUCCESS(status))
        {
            *SecurityDescriptor = buffer;
        }
        else
        {
            PhFree(buffer);
        }
    }

    if (NT_SUCCESS(status) && !bufferSize)
    {
        status = STATUS_INVALID_SECURITY_DESCR;
    }

    return status;
}

NTSTATUS PhSetServiceObjectSecurity(
    _In_ SC_HANDLE ServiceHandle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    NTSTATUS status;

    if (SetServiceObjectSecurity(ServiceHandle, SecurityInformation, SecurityDescriptor))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    return status;
}

NTSTATUS PhQueryServiceStatus(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ LPSERVICE_STATUS_PROCESS ServiceStatus
    )
{
    NTSTATUS status;
    ULONG returnLength = 0;

    memset(ServiceStatus, 0, sizeof(SERVICE_STATUS_PROCESS));

    if (QueryServiceStatusEx(ServiceHandle, SC_STATUS_PROCESS_INFO, (PBYTE)ServiceStatus, sizeof(SERVICE_STATUS_PROCESS), &returnLength))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    return status;
}

NTSTATUS PhQueryServiceVariableSize(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG InfoLevel,
    _Out_ PVOID* ServiceConfig
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = PhQueryServiceConfig2(
        ServiceHandle,
        InfoLevel,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = PhQueryServiceConfig2(
            ServiceHandle,
            InfoLevel,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (!NT_SUCCESS(status))
        {
            PhFree(buffer);
            return status;
        }
    }

    *ServiceConfig = buffer;

    return status;
}

NTSTATUS PhContinueService(
    _In_ SC_HANDLE ServiceHandle
    )
{
    NTSTATUS status;
    SERVICE_STATUS serviceStatus;

    if (ControlService(ServiceHandle, SERVICE_CONTROL_CONTINUE, &serviceStatus))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    return status;
}

NTSTATUS PhPauseService(
    _In_ SC_HANDLE ServiceHandle
    )
{
    NTSTATUS status;
    SERVICE_STATUS serviceStatus;

    if (ControlService(ServiceHandle, SERVICE_CONTROL_PAUSE, &serviceStatus))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    return status;
}

NTSTATUS PhDeleteService(
    _In_ SC_HANDLE ServiceHandle
    )
{
    NTSTATUS status;

    if (DeleteService(ServiceHandle))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    return status;
}

NTSTATUS PhStartService(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG NumberOfServiceArgs,
    _In_reads_opt_(NumberOfServiceArgs) PCWSTR* ServiceArgVectors
    )
{
    NTSTATUS status;

    if (StartService(ServiceHandle, NumberOfServiceArgs, ServiceArgVectors))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    return status;
}

NTSTATUS PhStopService(
    _In_ SC_HANDLE ServiceHandle
    )
{
    NTSTATUS status;
    SERVICE_STATUS serviceStatus;

    if (ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &serviceStatus))
        status = STATUS_SUCCESS;
    else
        status = PhGetLastWin32ErrorAsNtStatus();

    return status;
}

NTSTATUS PhGetServiceConfig(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ LPQUERY_SERVICE_CONFIG* ServiceConfig
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = 0x200;
    buffer = PhAllocate(bufferSize);

    status = PhQueryServiceConfig(
        ServiceHandle,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = PhQueryServiceConfig(
            ServiceHandle,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (!NT_SUCCESS(status))
        {
            PhFree(buffer);
            return status;
        }
    }

    *ServiceConfig = buffer;

    return status;
}

PPH_STRING PhGetServiceDescription(
    _In_ SC_HANDLE ServiceHandle
    )
{
    PPH_STRING description = NULL;
    LPSERVICE_DESCRIPTION serviceDescription;

    if (NT_SUCCESS(PhQueryServiceVariableSize(ServiceHandle, SERVICE_CONFIG_DESCRIPTION, &serviceDescription)))
    {
        if (serviceDescription->lpDescription)
            description = PhCreateString(serviceDescription->lpDescription);

        PhFree(serviceDescription);

        return description;
    }
    else
    {
        return NULL;
    }
}

_Success_(return)
BOOLEAN PhGetServiceDelayedAutoStart(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ PBOOLEAN DelayedAutoStart
    )
{
    SERVICE_DELAYED_AUTO_START_INFO delayedAutoStartInfo;

    if (NT_SUCCESS(PhQueryServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
        &delayedAutoStartInfo,
        sizeof(SERVICE_DELAYED_AUTO_START_INFO),
        NULL
        )))
    {
        *DelayedAutoStart = !!delayedAutoStartInfo.fDelayedAutostart;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOLEAN PhSetServiceDelayedAutoStart(
    _In_ SC_HANDLE ServiceHandle,
    _In_ BOOLEAN DelayedAutoStart
    )
{
    SERVICE_DELAYED_AUTO_START_INFO delayedAutoStartInfo;

    delayedAutoStartInfo.fDelayedAutostart = DelayedAutoStart;

    return NT_SUCCESS(PhChangeServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
        &delayedAutoStartInfo
        ));
}

_Success_(return)
BOOLEAN PhGetServiceTriggerInfo(
    _In_ SC_HANDLE ServiceHandle,
    _Out_opt_ PSERVICE_TRIGGER_INFO* ServiceTriggerInfo
    )
{
    PVOID buffer;
    ULONG bufferSize;
    SERVICE_TRIGGER_INFO triggerInfo;

    if (PhQueryServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_TRIGGER_INFO,
        &triggerInfo,
        sizeof(SERVICE_TRIGGER_INFO),
        &bufferSize
        ) == STATUS_BUFFER_TOO_SMALL)
    {
        if (!ServiceTriggerInfo)
            return TRUE;

        buffer = PhAllocate(bufferSize);

        if (NT_SUCCESS(PhQueryServiceConfig2(
            ServiceHandle,
            SERVICE_CONFIG_TRIGGER_INFO,
            buffer,
            bufferSize,
            &bufferSize
            )))
        {
            *ServiceTriggerInfo = buffer;
            return TRUE;
        }

        PhFree(buffer);
    }

    return FALSE;
}

PCPH_STRINGREF PhGetServiceStateString(
    _In_ ULONG ServiceState
    )
{
    PCPH_STRINGREF string;

    if (PhIndexStringRefSiKeyValuePairs(
        PhpServiceStatePairs,
        sizeof(PhpServiceStatePairs),
        ServiceState,
        &string
        ))
    {
        return string;
    }

    return &PhpServiceUnknownString;
}

PCPH_STRINGREF PhGetServiceTypeString(
    _In_ ULONG ServiceType
    )
{
    PCPH_STRINGREF string;

    if (PhFindStringRefSiKeyValuePairs(
        PhpServiceTypePairs,
        sizeof(PhpServiceTypePairs),
        ServiceType,
        &string
        ))
    {
        return string;
    }

    return &PhpServiceUnknownString;
}

ULONG PhGetServiceTypeInteger(
    _In_ PPH_STRINGREF ServiceType
    )
{
    ULONG integer;

    if (PhFindIntegerSiKeyValuePairsStringRef(
        PhpServiceTypePairs,
        sizeof(PhpServiceTypePairs),
        ServiceType,
        &integer
        ))
        return integer;
    else
        return ULONG_MAX;
}

PCPH_STRINGREF PhGetServiceStartTypeString(
    _In_ ULONG ServiceStartType
    )
{
    PCPH_STRINGREF string;

    if (PhIndexStringRefSiKeyValuePairs(
        PhpServiceStartTypePairs,
        sizeof(PhpServiceStartTypePairs),
        ServiceStartType,
        &string
        ))
    {
        return string;
    }

    return &PhpServiceUnknownString;
}

ULONG PhGetServiceStartTypeInteger(
    _In_ PPH_STRINGREF ServiceStartType
    )
{
    ULONG integer;

    if (PhFindIntegerSiKeyValuePairsStringRef(
        PhpServiceStartTypePairs,
        sizeof(PhpServiceStartTypePairs),
        ServiceStartType,
        &integer
        ))
        return integer;
    else
        return ULONG_MAX;
}

PCPH_STRINGREF PhGetServiceErrorControlString(
    _In_ ULONG ServiceErrorControl
    )
{
    PCPH_STRINGREF string;

    if (PhIndexStringRefSiKeyValuePairs(
        PhpServiceErrorControlPairs,
        sizeof(PhpServiceErrorControlPairs),
        ServiceErrorControl,
        &string
        ))
    {
        return string;
    }

    return &PhpServiceUnknownString;
}

ULONG PhGetServiceErrorControlInteger(
    _In_ PPH_STRINGREF ServiceErrorControl
    )
{
    ULONG integer;

    if (PhFindIntegerSiKeyValuePairsStringRef(
        PhpServiceErrorControlPairs,
        sizeof(PhpServiceErrorControlPairs),
        ServiceErrorControl,
        &integer
        ))
        return integer;
    else
        return ULONG_MAX;
}

PPH_STRING PhGetServiceNameFromTag(
    _In_ HANDLE ProcessId,
    _In_ PVOID ServiceTag
    )
{
    static typeof(&I_QueryTagInformation) QueryTagInformation_I = NULL;
    PPH_STRING serviceName = NULL;
    TAG_INFO_NAME_FROM_TAG nameFromTag;

    if (!QueryTagInformation_I)
    {
        QueryTagInformation_I = PhGetDllProcedureAddress(L"sechost.dll", "I_QueryTagInformation", 0);
    }

    if (!QueryTagInformation_I)
        return NULL;

    memset(&nameFromTag, 0, sizeof(TAG_INFO_NAME_FROM_TAG));
    nameFromTag.InParams.ProcessId = HandleToUlong(ProcessId);
    nameFromTag.InParams.ServiceTag = PtrToUlong(ServiceTag);

    QueryTagInformation_I(NULL, eTagInfoLevelNameFromTag, &nameFromTag);

    if (nameFromTag.OutParams.Name)
    {
        serviceName = PhCreateString(nameFromTag.OutParams.Name);
        LocalFree((HLOCAL)nameFromTag.OutParams.Name);
    }

    return serviceName;
}

PPH_STRING PhGetServiceNameForModuleReference(
    _In_ HANDLE ProcessId,
    _In_ PCWSTR ModuleName
    )
{
    static typeof(&I_QueryTagInformation) QueryTagInformation_I = NULL;
    PPH_STRING serviceNames = NULL;
    TAG_INFO_NAMES_REFERENCING_MODULE moduleNameRef;

    if (!QueryTagInformation_I)
    {
        if (WindowsVersion >= WINDOWS_8_1)
        {
            QueryTagInformation_I = PhGetDllProcedureAddress(L"sechost.dll", "I_QueryTagInformation", 0);
        }

        if (!QueryTagInformation_I)
            QueryTagInformation_I = PhGetDllProcedureAddress(L"advapi32.dll", "I_QueryTagInformation", 0);
    }

    if (!QueryTagInformation_I)
        return NULL;

    memset(&moduleNameRef, 0, sizeof(TAG_INFO_NAMES_REFERENCING_MODULE));
    moduleNameRef.InParams.ProcessId = HandleToUlong(ProcessId);
    moduleNameRef.InParams.ModuleName = ModuleName;

    QueryTagInformation_I(NULL, eTagInfoLevelNamesReferencingModule, &moduleNameRef);

    if (moduleNameRef.OutParams.Names)
    {
        PH_STRING_BUILDER sb;
        PCWSTR serviceName;

        PhInitializeStringBuilder(&sb, 0x40);

        for (serviceName = moduleNameRef.OutParams.Names; *serviceName; serviceName += PhCountStringZ(serviceName) + 1)
            PhAppendFormatStringBuilder(&sb, L"%s, ", serviceName);

        if (sb.String->Length != 0)
            PhRemoveEndStringBuilder(&sb, 2);

        serviceNames = PhFinalStringBuilderString(&sb);
        LocalFree((HLOCAL)moduleNameRef.OutParams.Names);
    }

    return serviceNames;
}

NTSTATUS PhGetThreadServiceTag(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID *ServiceTag
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;

    status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo);

    if (NT_SUCCESS(status))
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, FIELD_OFFSET(TEB, SubProcessTag)),
            ServiceTag,
            sizeof(PVOID),
            NULL
            );
    }

    return status;
}

PPH_STRING PhGetServiceKeyName(
    _In_ PPH_STRINGREF ServiceName
    )
{
    static CONST PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");

    return PhConcatStringRef2(&servicesKeyName, ServiceName);
}

PPH_STRING PhGetServiceParametersKeyName(
    _In_ PPH_STRINGREF ServiceName
    )
{
    static CONST PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");
    static CONST PH_STRINGREF parametersKeyName = PH_STRINGREF_INIT(L"\\Parameters");

    return PhConcatStringRef3(&servicesKeyName, ServiceName, &parametersKeyName);
}

PPH_STRING PhGetServiceConfigFileName(
    _In_ ULONG ServiceType,
    _In_ PCWSTR ServicePathName,
    _In_ PPH_STRINGREF ServiceName
    )
{
    NTSTATUS status;
    PPH_STRING fileName = NULL;

    status = PhGetServiceDllParameter(ServiceType, ServiceName, &fileName);

    if (!NT_SUCCESS(status))
    {
        if (ServicePathName[0])
        {
            PPH_STRING commandLine = PhCreateString(ServicePathName);

            if (FlagOn(ServiceType, SERVICE_WIN32))
            {
                PH_STRINGREF dummyFileName;
                PH_STRINGREF dummyArguments;

                PhParseCommandLineFuzzy(&commandLine->sr, &dummyFileName, &dummyArguments, &fileName);

                if (!PhIsNullOrEmptyString(fileName))
                {
                    PhSwapReference(&fileName, commandLine);
                }
            }
            else
            {
                PhMoveReference(&fileName, PhGetFileName(commandLine));
            }

            PhDereferenceObject(commandLine);
        }
    }

    return fileName;
}

NTSTATUS PhGetServiceConfigFileName2(
    _In_ ULONG ServiceType,
    _In_ PCWSTR ServicePathName,
    _In_ PPH_STRINGREF ServiceName,
    _Out_ PPH_STRING* ServiceFileName
    )
{
    NTSTATUS status;
    PPH_STRING fileName = NULL;

    status = PhGetServiceDllParameter(ServiceType, ServiceName, &fileName);

    if (!NT_SUCCESS(status))
    {
        if (ServicePathName[0])
        {
            PPH_STRING commandLine = PhCreateString(ServicePathName);

            if (FlagOn(ServiceType, SERVICE_WIN32))
            {
                PH_STRINGREF dummyFileName;
                PH_STRINGREF dummyArguments;
                PPH_STRING fileFullName = NULL;

                PhParseCommandLineFuzzy(&commandLine->sr, &dummyFileName, &dummyArguments, &fileFullName);

                if (!PhIsNullOrEmptyString(fileFullName))
                {
                    PhMoveReference(&fileName, fileFullName);
                }
            }
            else
            {
                PhMoveReference(&fileName, PhGetFileName(commandLine));
            }

            if (!PhIsNullOrEmptyString(fileName))
            {
                *ServiceFileName = fileName;
                status = STATUS_SUCCESS;
            }

            PhDereferenceObject(commandLine);
        }
    }
    else
    {
        *ServiceFileName = fileName;
    }

    return status;
}

NTSTATUS PhGetServiceHandleFileName(
    _In_ SC_HANDLE ServiceHandle,
    _In_ PPH_STRINGREF ServiceName,
    _Out_ PPH_STRING* ServiceFileName
    )
{
    NTSTATUS status;
    PPH_STRING fileName;
    LPQUERY_SERVICE_CONFIG config;

    status = PhGetServiceConfig(ServiceHandle, &config);

    if (NT_SUCCESS(status))
    {
        status = PhGetServiceConfigFileName2(
            config->dwServiceType,
            config->lpBinaryPathName,
            ServiceName,
            &fileName
            );

        if (NT_SUCCESS(status))
        {
            *ServiceFileName = fileName;
        }

        PhFree(config);
    }

    return status;
}

NTSTATUS PhGetServiceFileName(
    _In_ PPH_STRINGREF ServiceName,
    _Out_ PPH_STRING* ServiceFileName
    )
{
    PPH_STRING serviceDllString = NULL;
    NTSTATUS status;
    HANDLE keyHandle;

    status = PhOpenServiceKey(
        &keyHandle,
        KEY_READ,
        ServiceName
        );

    if (NT_SUCCESS(status))
    {
        if (serviceDllString = PhQueryRegistryStringZ(keyHandle, L"ImagePath"))
        {
            PPH_STRING expandedString;

            if (expandedString = PhExpandEnvironmentStrings(&serviceDllString->sr))
            {
                PH_STRINGREF fileName;
                PH_STRINGREF arguments;

                if (PhParseCommandLineFuzzy(&expandedString->sr, &fileName, &arguments, NULL))
                {
                    PhMoveReference(&serviceDllString, PhCreateString2(&fileName));
                }
                else
                {
                    PhMoveReference(&serviceDllString, expandedString);
                }
            }
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }

        NtClose(keyHandle);
    }

    if (NT_SUCCESS(status))
    {
        *ServiceFileName = serviceDllString;
    }
    else
    {
        PhClearReference(&serviceDllString);
    }

    return status;
}

NTSTATUS PhpGetServiceDllName(
    _In_ PPH_STRING ServiceKeyName,
    _Out_ PPH_STRING* ServiceDll
    )
{
    PPH_STRING serviceDllString = NULL;
    NTSTATUS status;
    HANDLE keyHandle;

    status = PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &ServiceKeyName->sr,
        0
        );

    if (NT_SUCCESS(status))
    {
        if (serviceDllString = PhQueryRegistryStringZ(keyHandle, L"ServiceDll"))
        {
            PPH_STRING expandedString;

            if (expandedString = PhExpandEnvironmentStrings(&serviceDllString->sr))
            {
                PhMoveReference(&serviceDllString, expandedString);
            }
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }

        NtClose(keyHandle);
    }

    if (NT_SUCCESS(status))
    {
        *ServiceDll = serviceDllString;
    }
    else
    {
        PhClearReference(&serviceDllString);
    }

    return status;
}

NTSTATUS PhGetServiceDllParameter(
    _In_ ULONG ServiceType,
    _In_ PPH_STRINGREF ServiceName,
    _Out_ PPH_STRING *ServiceDll
    )
{
    NTSTATUS status;
    PPH_STRING serviceDllString;
    PPH_STRING keyName;

    if (FlagOn(ServiceType, SERVICE_USERSERVICE_INSTANCE))
    {
        PH_STRINGREF hostServiceName;
        PH_STRINGREF userSessionLuid;

        // The SCM creates multiple "user service instance" processes for each user session with the following template:
        // [Host Service Instance Name]_[LUID for Session]
        // The SCM internally uses the ServiceDll of the "host service instance" for all "user service instance" processes/services
        // and we need to parse the user service template and query the "host service instance" configuration. (hsebs)

        if (PhSplitStringRefAtLastChar(ServiceName, L'_', &hostServiceName, &userSessionLuid))
        {
            keyName = PhGetServiceParametersKeyName(&hostServiceName);
        }
        else
        {
            keyName = PhGetServiceParametersKeyName(ServiceName);
        }
    }
    else
    {
        keyName = PhGetServiceParametersKeyName(ServiceName);
    }

    status = PhpGetServiceDllName(
        keyName,
        &serviceDllString
        );

    PhDereferenceObject(keyName);

    if (NT_SUCCESS(status))
    {
        *ServiceDll = serviceDllString;
        return STATUS_SUCCESS;
    }

    if (
        (WindowsVersion == WINDOWS_8 || WindowsVersion == WINDOWS_8_1) &&
        (status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_NOT_FOUND)
        )
    {
        keyName = PhGetServiceKeyName(ServiceName);

        // Windows 8 places the ServiceDll for some services in the root key. (dmex)
        status = PhpGetServiceDllName(
            keyName,
            &serviceDllString
            );

        PhDereferenceObject(keyName);

        if (NT_SUCCESS(status))
        {
            *ServiceDll = serviceDllString;
            return STATUS_SUCCESS;
        }
    }

    return status;
}

PPH_STRING PhGetServiceAppUserModelId(
    _In_ PPH_STRINGREF ServiceName
    )
{
    PPH_STRING serviceAppUserModelId = NULL;
    HANDLE keyHandle;

    if (NT_SUCCESS(PhOpenServiceKey(
        &keyHandle,
        KEY_READ,
        ServiceName
        )))
    {
        serviceAppUserModelId = PhQueryRegistryStringZ(keyHandle, L"AppUserModelId");

        NtClose(keyHandle);
    }

    return serviceAppUserModelId;
}

ULONG PhGetServiceBootFlags(
    _In_ PPH_STRINGREF ServiceName
    )
{
    ULONG serviceBootFlags = 0;
    HANDLE keyHandle;

    if (NT_SUCCESS(PhOpenServiceKey(
        &keyHandle,
        KEY_READ,
        ServiceName
        )))
    {
        serviceBootFlags = PhQueryRegistryUlongZ(keyHandle, L"BootFlags");

        NtClose(keyHandle);
    }

    if (serviceBootFlags == ULONG_MAX)
    {
        PPH_STRING serviceKeyName;

        serviceKeyName = PhGetServiceParametersKeyName(ServiceName);

        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_READ,
            PH_KEY_LOCAL_MACHINE,
            &serviceKeyName->sr,
            0
            )))
        {
            serviceBootFlags = PhQueryRegistryUlongZ(keyHandle, L"BootFlags");

            NtClose(keyHandle);
        }

        PhDereferenceObject(serviceKeyName);
    }

    return serviceBootFlags;
}

PPH_STRING PhGetServicePackageFullName(
    _In_ PPH_STRINGREF ServiceName
    )
{
    PPH_STRING servicePackageName = NULL;
    HANDLE keyHandle;

    if (NT_SUCCESS(PhOpenServiceKey(
        &keyHandle,
        KEY_READ,
        ServiceName
        )))
    {
        servicePackageName = PhQueryRegistryStringZ(keyHandle, L"PackageFullName");

        NtClose(keyHandle);
    }

    return servicePackageName;
}

NTSTATUS PhWaitForServiceStatus(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG WaitForState,
    _In_ ULONG Timeout
    )
{
    NTSTATUS status;
    SERVICE_STATUS_PROCESS serviceStatus;
    ULONG64 serviceTicks;
    ULONG serviceCheck;

    status = PhQueryServiceStatus(ServiceHandle, &serviceStatus);

    if (!NT_SUCCESS(status))
        return status;
    if (serviceStatus.dwCurrentState == WaitForState)
        return STATUS_SUCCESS;

    serviceTicks = NtGetTickCount64();
    serviceCheck = serviceStatus.dwCheckPoint;

    while (
        serviceStatus.dwCurrentState == SERVICE_START_PENDING ||
        serviceStatus.dwCurrentState == SERVICE_STOP_PENDING ||
        serviceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING ||
        serviceStatus.dwCurrentState == SERVICE_PAUSE_PENDING
        )
    {
        ULONG statusWaitHint = serviceStatus.dwWaitHint / 10;

        if (statusWaitHint < 1000)
            statusWaitHint = 1000;
        if (statusWaitHint > 10000)
            statusWaitHint = 10000;

        PhDelayExecution(statusWaitHint);

        status = PhQueryServiceStatus(ServiceHandle, &serviceStatus);

        if (!NT_SUCCESS(status))
            return status;

        if (serviceStatus.dwCurrentState == WaitForState)
            return STATUS_SUCCESS;

        if (!(
            serviceStatus.dwCurrentState == SERVICE_START_PENDING ||
            serviceStatus.dwCurrentState == SERVICE_STOP_PENDING ||
            serviceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING ||
            serviceStatus.dwCurrentState == SERVICE_PAUSE_PENDING
            ))
        {
            return STATUS_SUCCESS;
        }

        serviceTicks = NtGetTickCount64();

        if (serviceStatus.dwCheckPoint > serviceCheck)
        {
            serviceCheck = serviceStatus.dwCheckPoint;
        }
        else if ((NtGetTickCount64() - serviceTicks) > serviceStatus.dwWaitHint)
        {
            // Service doesn't report progress.
        }

        if ((NtGetTickCount64() - serviceTicks) > Timeout)
        {
            return STATUS_TIMEOUT; // STATUS_IO_TIMEOUT
        }
    }

    return status;
}
