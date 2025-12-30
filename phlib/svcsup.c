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

/**
 * Gets a cached service manager handle.
 *
 * \return Service manager handle, or NULL if failed.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-openscmanagerw
 */
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

/**
 * Enumerates all services.
 *
 * \param Services Receives a pointer to service status array. Caller must free with PhFree.
 * \param NumberOfServices Receives the number of services enumerated.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-enumservicesstatusexw
 */
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

/**
 * Enumerates services that depend on the specified service.
 *
 * \param ServiceHandle Handle to the service.
 * \param DependentServices Receives a pointer to dependent services array. Caller must free with PhFree.
 * \param NumberOfDependentServices Receives the number of dependent services.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-enumdependentservicesw
 */
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

/**
 * Opens a handle to the service control manager.
 *
 * \param ServiceManagerHandle Receives the service manager handle.
 * \param DatabaseName Name of the service control manager database, or NULL for the default.
 * \param DesiredAccess Access rights for the handle.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-openscmanagerw
 */
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
        *ServiceManagerHandle = NULL;
        return PhGetLastWin32ErrorAsNtStatus();
    }
}

/**
 * Opens a handle to a service.
 *
 * \param ServiceHandle Receives the service handle.
 * \param DesiredAccess Access rights for the handle.
 * \param ServiceName Name of the service.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-openservicew
 */
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

/**
 * Opens a registry key for a service.
 *
 * \param KeyHandle Receives the key handle.
 * \param DesiredAccess Access rights for the handle.
 * \param ServiceName Name of the service.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Closes a service handle.
 *
 * \param ServiceHandle Handle to close.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-closeservicehandle
 */
NTSTATUS PhCloseServiceHandle(
    _In_ SC_HANDLE ServiceHandle
    )
{
    if (CloseServiceHandle(ServiceHandle))
        return STATUS_SUCCESS;

    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Creates a service.
 *
 * \param ServiceHandle Receives the service handle.
 * \param ServiceName Name of the service to create.
 * \param DisplayName Display name of the service, or NULL.
 * \param DesiredAccess Access rights for the handle.
 * \param ServiceType Service type.
 * \param StartType Service start type.
 * \param ErrorControl Error control level.
 * \param BinaryPathName Path to the service binary, or NULL.
 * \param UserName User account for the service, or NULL.
 * \param Password Password for the user account, or NULL.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-createservicew
 */
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

/**
 * Changes the configuration of a service.
 *
 * \param ServiceHandle Handle to the service.
 * \param ServiceType Service type, or SERVICE_NO_CHANGE.
 * \param StartType Service start type, or SERVICE_NO_CHANGE.
 * \param ErrorControl Error control level, or SERVICE_NO_CHANGE.
 * \param BinaryPathName Path to the service binary, or NULL.
 * \param LoadOrderGroup Load order group, or NULL.
 * \param TagId Receives the tag identifier, or NULL.
 * \param Dependencies List of dependencies, or NULL.
 * \param ServiceStartName User account for the service, or NULL.
 * \param Password Password for the user account, or NULL.
 * \param DisplayName Display name of the service, or NULL.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-changeserviceconfigw
 */
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

/**
 * Changes optional configuration parameters of a service.
 *
 * \param ServiceHandle Handle to the service.
 * \param ServiceConfigLevel Configuration information level.
 * \param Buffer Configuration data, or NULL.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-changeserviceconfig2w
 */
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

/**
 * Enumerates services that depend on the specified service (variant).
 *
 * \param ServiceHandle Handle to the service.
 * \param Buffer Buffer to receive dependent services, or NULL.
 * \param BufferLength Size of the buffer in bytes.
 * \param ReturnLength Receives the required buffer size.
 * \param NumberOfServices Receives the number of dependent services.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-enumdependentservicesw
 */
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

/**
 * Queries service configuration.
 *
 * \param ServiceHandle Handle to the service.
 * \param Buffer Buffer to receive configuration, or NULL.
 * \param BufferLength Size of the buffer in bytes.
 * \param ReturnLength Receives the required buffer size, or NULL.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryserviceconfigw
 */
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

/**
 * Queries optional service configuration parameters.
 *
 * \param ServiceHandle Handle to the service.
 * \param ServiceConfigLevel Configuration information level.
 * \param Buffer Buffer to receive configuration, or NULL.
 * \param BufferLength Size of the buffer in bytes.
 * \param ReturnLength Receives the required buffer size, or NULL.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryserviceconfig2w
 */
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

/**
 * Retrieves security descriptor for a service.
 *
 * \param ServiceHandle Handle to the service.
 * \param SecurityInformation Security information to retrieve.
 * \param SecurityDescriptor Receives a pointer to the security descriptor. Caller must free with PhFree.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryserviceobjectsecurity
 */
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
            return STATUS_SUCCESS;
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

    *SecurityDescriptor = NULL;

    return status;
}

/**
 * Sets the security descriptor for a service object.
 *
 * \param ServiceHandle Handle to the service.
 * \param SecurityInformation Specifies the type of security information to set.
 * \param SecurityDescriptor Pointer to a SECURITY_DESCRIPTOR structure containing the new security descriptor.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-setserviceobjectsecurity
 */
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

/**
 * Queries the current status of a specified service.
 *
 * \param ServiceHandle A handle to the service. This handle must have the SERVICE_QUERY_STATUS access right.
 * \param ServiceStatus A pointer to a SERVICE_STATUS_PROCESS structure that receives the status information for the service.
 * \return Returns an NTSTATUS code indicating success or failure of the operation.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryservicestatusex
 */
NTSTATUS PhQueryServiceStatus(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ LPSERVICE_STATUS_PROCESS ServiceStatus
    )
{
    NTSTATUS status;
    ULONG returnLength = 0;

    memset(ServiceStatus, 0, sizeof(SERVICE_STATUS_PROCESS));

    if (QueryServiceStatusEx(
        ServiceHandle,
        SC_STATUS_PROCESS_INFO,
        (PBYTE)ServiceStatus,
        sizeof(SERVICE_STATUS_PROCESS),
        &returnLength
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

/**
 * Queries variable-sized information about a service.
 *
 * \param ServiceHandle Handle to the service to query.
 * \param InfoLevel The information level specifying the type of service information to retrieve.
 * \param ServiceConfig Pointer to a variable containing the requested service information.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryserviceconfig2w
 */
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

/**
 * Sends a continue control code to a service.
 *
 * \param ServiceHandle Handle to the service.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Sends a pause control code to a service.
 *
 * \param ServiceHandle Handle to the service.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Deletes a service from the service control manager database.
 *
 * \param ServiceHandle Handle to the service.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-deleteservice
 */
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

/**
 * Starts a service.
 *
 * \param ServiceHandle Handle to the service.
 * \param NumberOfServiceArgs Number of arguments in the ServiceArgVectors array.
 * \param ServiceArgVectors Array of arguments to pass to the service, or NULL.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-startservicew
 */
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

/**
 * Sends a stop control code to a service.
 *
 * \param ServiceHandle Handle to the service.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Retrieves the configuration of a service.
 *
 * \param ServiceHandle Handle to the service.
 * \param ServiceConfig Receives a pointer to the service configuration. Caller must free with PhFree.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Retrieves the description string for a service.
 *
 * \param ServiceHandle Handle to the service.
 * \return A PPH_STRING containing the service description, or NULL if not found.
 */
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

PPH_STRING PhGetServiceDescriptionKey(
    _In_ PPH_STRINGREF ServiceName
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    PPH_STRING description = NULL;

    status = PhOpenServiceKey(
        &keyHandle,
        KEY_QUERY_VALUE,
        ServiceName
        );

    if (NT_SUCCESS(status))
    {
        PPH_STRING descriptionString;
        PPH_STRING serviceDescriptionString;

        if (descriptionString = PhQueryRegistryStringZ(keyHandle, L"Description"))
        {
            if (serviceDescriptionString = PhLoadIndirectString(&descriptionString->sr))
                PhMoveReference(&description, serviceDescriptionString);
            else
                PhSwapReference(&description, descriptionString);

            PhDereferenceObject(descriptionString);
        }

        NtClose(keyHandle);
    }
    else
    {
        PhMoveReference(&description, PhGetStatusMessage(status, 0));
    }

    return description;
}

/**
 * Retrieves the delayed auto-start setting for a service.
 *
 * \param ServiceHandle Handle to the service.
 * \param DelayedAutoStart Receives TRUE if delayed auto-start is enabled.
 * \return TRUE if successful, FALSE otherwise.
 */
NTSTATUS PhGetServiceDelayedAutoStart(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ PBOOLEAN DelayedAutoStart
    )
{
    NTSTATUS status;
    SERVICE_DELAYED_AUTO_START_INFO delayedAutoStartInfo;

    status = PhQueryServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
        &delayedAutoStartInfo,
        sizeof(SERVICE_DELAYED_AUTO_START_INFO),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *DelayedAutoStart = !!delayedAutoStartInfo.fDelayedAutostart;
    }

    return status;
}

/**
 * Sets the delayed auto-start setting for a service.
 *
 * \param ServiceHandle Handle to the service.
 * \param DelayedAutoStart TRUE to enable delayed auto-start, FALSE to disable.
 * \return TRUE if successful, FALSE otherwise.
 */
NTSTATUS PhSetServiceDelayedAutoStart(
    _In_ SC_HANDLE ServiceHandle,
    _In_ BOOLEAN DelayedAutoStart
    )
{
    SERVICE_DELAYED_AUTO_START_INFO delayedAutoStartInfo;

    delayedAutoStartInfo.fDelayedAutostart = DelayedAutoStart;

    return PhChangeServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
        &delayedAutoStartInfo
        );
}

/**
 * Retrieves the trigger information for a service.
 *
 * \param ServiceHandle Handle to the service.
 * \param ServiceTriggerInfo Receives a pointer to the trigger information. Caller must free with PhFree.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetServiceTriggerInfo(
    _In_ SC_HANDLE ServiceHandle,
    _Out_opt_ PSERVICE_TRIGGER_INFO* ServiceTriggerInfo
    )
{
    NTSTATUS status;
    PVOID buffer = NULL;
    ULONG bufferSize = 0;
    SERVICE_TRIGGER_INFO triggerInfo;

    if (ServiceTriggerInfo)
        *ServiceTriggerInfo = NULL;

    status = PhQueryServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_TRIGGER_INFO,
        &triggerInfo,
        sizeof(SERVICE_TRIGGER_INFO),
        &bufferSize
        );

    if (NT_SUCCESS(status))
    {
        // The fixed-size struct was sufficient (e.g., no triggers or no variable list).
        if (ServiceTriggerInfo)
        {
            buffer = PhAllocate(sizeof(SERVICE_TRIGGER_INFO));
            if (!buffer)
                return STATUS_NO_MEMORY;

            memcpy(buffer, &triggerInfo, sizeof(SERVICE_TRIGGER_INFO));
            *ServiceTriggerInfo = buffer;
        }

        return STATUS_SUCCESS;
    }

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        if (bufferSize == 0)
            return STATUS_INVALID_BUFFER_SIZE;

        // If the caller does not want it, just report success.
        if (!ServiceTriggerInfo)
            return STATUS_SUCCESS;

        buffer = PhAllocate(bufferSize);
        if (!buffer)
            return STATUS_NO_MEMORY;

        status = PhQueryServiceConfig2(
            ServiceHandle,
            SERVICE_CONFIG_TRIGGER_INFO,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
        {
            *ServiceTriggerInfo = buffer;
            return STATUS_SUCCESS;
        }
    }

    if (buffer)
    {
        PhFree(buffer);
    }

    return status;
}

/**
 * Retrieves the service state.
 *
 * \param ServiceState The service state value (e.g., SERVICE_RUNNING, SERVICE_STOPPED).
 * \return A pointer to a string reference describing the service state, or "Unknown" if not found.
 */
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

/**
 * Retrieves the display type.
 *
 * \param ServiceType The service type value (e.g., SERVICE_WIN32_OWN_PROCESS).
 * \return A pointer to a string reference describing the service type, or "Unknown" if not found.
 */
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

/**
 * Converts a service type string to its corresponding integer value.
 *
 * \param ServiceType A pointer to a string reference containing the service type name.
 * \return The integer value of the service type, or ULONG_MAX if not found.
 */
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

/**
 * Retrieves the display string for a service start type value.
 *
 * \param ServiceStartType The service start type value (e.g., SERVICE_AUTO_START).
 * \return A pointer to a string reference describing the start type, or "Unknown" if not found.
 */
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

/**
 * Converts a service start type string to its corresponding integer value.
 *
 * \param ServiceStartType A pointer to a string reference containing the start type name.
 * \return The integer value of the start type, or ULONG_MAX if not found.
 */
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

/**
 * Retrieves the display string for a service error control value.
 *
 * \param ServiceErrorControl The service error control value (e.g., SERVICE_ERROR_NORMAL).
 * \return A pointer to a string reference describing the error control, or "Unknown" if not found.
 */
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

/**
 * Converts a service error control string to its corresponding integer value.
 *
 * \param ServiceErrorControl A pointer to a string reference containing the error control name.
 * \return The integer value of the error control, or ULONG_MAX if not found.
 */
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

/**
 * Retrieves the service name associated with a service tag in a process.
 *
 * \param ProcessId The process ID to query.
 * \param ServiceTag The service tag value.
 * \return A newly allocated PPH_STRING containing the service name, or NULL if not found.
 */
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

/**
 * Retrieves a comma-separated list of service names referencing a module in a process.
 *
 * \param ProcessId The process ID to query.
 * \param ModuleName The name of the module to check for service references.
 * \return A newly allocated PPH_STRING containing the service names, or NULL if not found.
 */
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

/**
 * Retrieves the service tag value for a thread in a process.
 *
 * \param ThreadHandle Handle to the thread.
 * \param ProcessHandle Handle to the process containing the thread.
 * \param ServiceTag Receives the service tag value.
 * \return NTSTATUS Successful or errant status.
 */
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
        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, FIELD_OFFSET(TEB, SubProcessTag)),
            ServiceTag,
            sizeof(PVOID),
            NULL
            );
    }

    return status;
}

/**
 * Builds the full registry key name for a service.
 *
 * \param ServiceName The service name as a string reference.
 * \return A newly allocated PPH_STRING containing the full registry key name.
 */
PPH_STRING PhGetServiceKeyName(
    _In_ PPH_STRINGREF ServiceName
    )
{
    static CONST PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services");

    return PhConcatStringRef3(&servicesKeyName, &PhNtPathSeparatorString, ServiceName);
}

/**
 * Builds the full registry key name for a service's Parameters subkey.
 *
 * \param ServiceName The service name as a string reference.
 * \return A newly allocated PPH_STRING containing the full Parameters subkey name.
 */
PPH_STRING PhGetServiceParametersKeyName(
    _In_ PPH_STRINGREF ServiceName
    )
{
    static CONST PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services");
    static CONST PH_STRINGREF parametersKeyName = PH_STRINGREF_INIT(L"\\Parameters");

    return PhConcatStringRef4(&servicesKeyName, &PhNtPathSeparatorString, ServiceName, &parametersKeyName);
}

/**
 * Attempts to determine the file name of a service's binary or DLL.
 *
 * \param ServiceType The service type flags.
 * \param ServicePathName The service's binary path name.
 * \param ServiceName The service name as a string reference.
 * \return A newly allocated PPH_STRING containing the file name, or NULL if not found.
 */
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
                PPH_STRING fileFullName = NULL;
                PH_STRINGREF dummyFileName = { 0 };
                PH_STRINGREF dummyArguments = { 0 };

                if (PhParseCommandLineFuzzy(&commandLine->sr, &dummyFileName, &dummyArguments, &fileFullName))
                {
                    PhSwapReference(&fileName, fileFullName);
                }
                else
                {
                    PhSwapReference(&fileName, PhGetFileName(commandLine));
                }

                PhClearReference(&fileFullName);
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

/**
 * Attempts to determine the file name of a service's binary or DLL (variant).
 *
 * \param ServiceType The service type flags.
 * \param ServicePathName The service's binary path name.
 * \param ServiceName The service name as a string reference.
 * \param ServiceFileName Receives a pointer to the file name string.
 * \return NTSTATUS Successful or errant status.
 */
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
                PPH_STRING fileFullName = NULL;
                PH_STRINGREF dummyFileName = { 0 };
                PH_STRINGREF dummyArguments = { 0 };

                if (PhParseCommandLineFuzzy(&commandLine->sr, &dummyFileName, &dummyArguments, &fileFullName))
                {
                    PhSwapReference(&fileName, fileFullName);
                }
                else
                {
                    PhSwapReference(&fileName, PhGetFileName(commandLine));
                }

                PhClearReference(&fileFullName);
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

/**
 * Retrieves the file name of a service's binary or DLL using a service handle.
 *
 * \param ServiceHandle Handle to the service.
 * \param ServiceName The service name as a string reference.
 * \param ServiceFileName Receives a pointer to the file name string.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Retrieves the file name of a service's binary or DLL from the registry.
 *
 * \param ServiceName The service name as a string reference.
 * \param ServiceFileName Receives a pointer to the file name string.
 * \return NTSTATUS Successful or errant status.
 */
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
                    PhSwapReference(&serviceDllString, expandedString);
                }

                PhDereferenceObject(expandedString);
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

/**
 * Retrieves the ServiceDll value for a service from the registry.
 *
 * \param ServiceKeyName The full registry key name for the service.
 * \param ServiceDll Receives a pointer to the ServiceDll string.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Retrieves the ServiceDll parameter for a service, handling user service instances.
 *
 * \param ServiceType The service type flags.
 * \param ServiceName The service name as a string reference.
 * \param ServiceDll Receives a pointer to the ServiceDll string.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Retrieves the AppUserModelId value for a service from the registry.
 *
 * \param ServiceName The service name as a string reference.
 * \return A PPH_STRING containing the AppUserModelId, or NULL if not found.
 */
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

/**
 * Retrieves the BootFlags value for a service from the registry.
 *
 * \param ServiceName The service name as a string reference.
 * \return The BootFlags value, or 0 if not found.
 */
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

/**
 * Retrieves the PackageFullName value for a service.
 *
 * \param ServiceName Name of the service.
 * \return A PPH_STRING containing the PackageFullName, or NULL if not found.
 */
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
    ULONG64 startTick;
    ULONG64 lastProgressTick;
    ULONG serviceCheck;

    status = PhQueryServiceStatus(ServiceHandle, &serviceStatus);
    if (!NT_SUCCESS(status))
        return status;
    if (serviceStatus.dwCurrentState == WaitForState)
        return STATUS_SUCCESS;

    startTick = NtGetTickCount64();
    lastProgressTick = startTick;
    serviceCheck = serviceStatus.dwCheckPoint;

    while (
        serviceStatus.dwCurrentState == SERVICE_START_PENDING ||
        serviceStatus.dwCurrentState == SERVICE_STOP_PENDING ||
        serviceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING ||
        serviceStatus.dwCurrentState == SERVICE_PAUSE_PENDING
        )
    {
        ULONG statusWaitHint = serviceStatus.dwWaitHint / 10;
        ULONG64 nowTick;

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

        nowTick = NtGetTickCount64();

        if (serviceStatus.dwCheckPoint > serviceCheck)
        {
            serviceCheck = serviceStatus.dwCheckPoint;
            lastProgressTick = nowTick;
        }
        else if ((nowTick - lastProgressTick) > serviceStatus.dwWaitHint)
        {
            // Service doesn't report progress.
        }

        if (Timeout && (nowTick - startTick) > Timeout)
        {
            return STATUS_TIMEOUT; // STATUS_IO_TIMEOUT
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Queries whether a service is configured for SafeBoot (Safe Mode or Safe Mode with Networking).
 *
 * \param ServiceName Name of the service.
 * \param DriverName Name of the service.
 * \param SafeModeNormal Receives TRUE if the service is configured to start in Safe Mode.
 * \param SafeModeNetwork Receives TRUE if the service is configured to start in Safe Mode with Network.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetServiceSafeBootStart(
    _In_opt_ PPH_STRINGREF ServiceName,
    _In_opt_ PPH_STRINGREF DriverName,
    _Out_ PBOOLEAN SafeModeNormal,
    _Out_ PBOOLEAN SafeModeNetwork
    )
{
    static CONST PH_STRINGREF keyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Control\\SafeBoot");
    static CONST PH_STRINGREF keyNameSafeBoot[] = { PH_STRINGREF_INIT(L"Minimal"), PH_STRINGREF_INIT(L"Network") };
    NTSTATUS status;
    HANDLE safeBootKeyHandle = NULL;
    HANDLE groupKeyHandle = NULL;
    HANDLE serviceKeyHandle = NULL;
    BOOLEAN safeModeNormalFound = FALSE;
    BOOLEAN safeModeNetworkFound = FALSE;

    *SafeModeNormal = FALSE;
    *SafeModeNetwork = FALSE;

    status = PhOpenKey(
        &safeBootKeyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    for (ULONG i = 0; i < RTL_NUMBER_OF(keyNameSafeBoot); i++)
    {
        if (NT_SUCCESS(PhOpenKey(
            &groupKeyHandle,
            KEY_READ,
            safeBootKeyHandle,
            &keyNameSafeBoot[i],
            0
            )))
        {
            if (ServiceName && PhOpenKey(
                &serviceKeyHandle,
                KEY_READ,
                groupKeyHandle,
                ServiceName,
                0
                ))
            {
                if (i == 0)
                    safeModeNormalFound = TRUE;
                else if (i == 1)
                    safeModeNetworkFound = TRUE;

                NtClose(serviceKeyHandle);
            }

            if (DriverName && PhOpenKey(
                &serviceKeyHandle,
                KEY_READ,
                groupKeyHandle,
                DriverName,
                0
                ))
            {
                if (i == 0)
                    safeModeNormalFound = TRUE;
                else if (i == 1)
                    safeModeNetworkFound = TRUE;

                NtClose(serviceKeyHandle);
            }

            NtClose(groupKeyHandle);
        }
    }

    NtClose(safeBootKeyHandle);

    *SafeModeNormal = safeModeNormalFound;
    *SafeModeNetwork = safeModeNetworkFound;
    return STATUS_SUCCESS;
}

/**
 * Retrieves the security descriptor for a service directly from the registry.
 *
 * \param ServiceName Name of the service.
 * \param SecurityDescriptor Receives a pointer to the security descriptor. Caller must free with PhFree.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetServiceRegistrySecurityDescriptor(
    _In_ PPH_STRINGREF ServiceName,
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    static const PH_STRINGREF securityKeyName = PH_STRINGREF_INIT(L"Security");
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE keyHandle = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION keyValueInfo;
    PPH_STRING serviceKeyName = NULL;
    PVOID buffer = NULL;
    ULONG bufferSize = 0;
    ULONG resultLength = 0;

    *SecurityDescriptor = NULL;

    // Build the full registry path: System\CurrentControlSet\Services\<ServiceName>\Security
    serviceKeyName = PhGetServiceKeyName(ServiceName);

    if (!serviceKeyName)
    {
        return STATUS_NO_MEMORY;
    }

    // Append "\Security" to the service key name
    PhSwapReference(&serviceKeyName, PhConcatStringRef3(
        &serviceKeyName->sr, 
        &PhNtPathSeparatorString,
        &securityKeyName
        ));

    status = PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &serviceKeyName->sr,
        0
        );

    PhDereferenceObject(serviceKeyName);

    if (!NT_SUCCESS(status))
        return status;

    status = PhQueryValueKey(
        keyHandle,
        &securityKeyName,
        KeyValuePartialInformation,
        &keyValueInfo
        );

    if (NT_SUCCESS(status))
    {
        if (keyValueInfo->Type == REG_BINARY)
        {
            RtlMoveMemory(SecurityDescriptor, keyValueInfo->Data, keyValueInfo->DataLength);
            return STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_INVALID_SECURITY_DESCR;
        }

        NtClose(keyHandle);
    }
    else if (NT_SUCCESS(status) && bufferSize == 0)
    {
        status = STATUS_INVALID_SECURITY_DESCR;
    }

    PhFree(buffer);

    return status;
}

/**
 * Builds the full registry key name for a service into a caller-supplied buffer.
 *
 * \param ServiceName Name of the service.
 * \param Buffer Buffer to receive the key name.
 * \param BufferLength Length of the buffer in characters.
 * \param RequiredLength Receives the required length in characters (including null terminator).
 * \return TRUE if successful, FALSE if the buffer is too small.
 */
_Success_(return)
BOOLEAN PhGetServiceKeyNameToBuffer(
    _In_ PPH_STRINGREF ServiceName,
    _Out_writes_to_opt_(BufferLength, *RequiredLength) PWSTR Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T RequiredLength
    )
{
    static CONST PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services");
    SIZE_T totalLength = servicesKeyName.Length / sizeof(WCHAR) + ServiceName->Length / sizeof(WCHAR);
    SIZE_T requiredLength = totalLength + 1;

    if (RequiredLength)
    {
        *RequiredLength = requiredLength;
    }

    if (BufferLength < requiredLength)
        return FALSE;

    memcpy(Buffer, servicesKeyName.Buffer, servicesKeyName.Length);
    memcpy(Buffer + (servicesKeyName.Length / sizeof(WCHAR)), ServiceName->Buffer, ServiceName->Length);
    Buffer[totalLength] = 0;

    return TRUE;
}

/**
 * Builds the full registry key name for a service's Parameters subkey into a caller-supplied buffer.
 *
 * \param ServiceName Name of the service.
 * \param Buffer Buffer to receive the key name.
 * \param BufferLength Length of the buffer in characters.
 * \param RequiredLength Receives the required length in characters (including null terminator).
 * \return TRUE if successful, FALSE if the buffer is too small.
 */
_Success_(return)
BOOLEAN PhGetServiceParametersKeyNameToBuffer(
    _In_ PPH_STRINGREF ServiceName,
    _Out_writes_to_opt_(BufferLength, *RequiredLength) PWSTR Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T RequiredLength
    )
{
    static CONST PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services");
    static CONST PH_STRINGREF parametersKeyName = PH_STRINGREF_INIT(L"\\Parameters");
    SIZE_T totalLength;
    SIZE_T requiredLength;

    totalLength =
        servicesKeyName.Length / sizeof(WCHAR) +
        ServiceName->Length / sizeof(WCHAR) +
        parametersKeyName.Length / sizeof(WCHAR);

    requiredLength = totalLength + 1;

    if (RequiredLength)
    {
        *RequiredLength = requiredLength;
    }

    if (BufferLength < requiredLength)
    {
        return FALSE;
    }

    memcpy(Buffer, servicesKeyName.Buffer, servicesKeyName.Length);
    memcpy(Buffer + (servicesKeyName.Length / sizeof(WCHAR)), ServiceName->Buffer, ServiceName->Length);
    memcpy(Buffer + (servicesKeyName.Length / sizeof(WCHAR)) + (ServiceName->Length / sizeof(WCHAR)), parametersKeyName.Buffer, parametersKeyName.Length);
    Buffer[totalLength] = 0;

    return TRUE;
}

