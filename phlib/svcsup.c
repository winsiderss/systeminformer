/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2012
 *     dmex    2019-2023
 *
 */

#include <ph.h>
#include <subprocesstag.h>
#include <svcsup.h>
#include <mapldr.h>

#define SIP(String, Integer) \
    { (String), (PVOID)(Integer) }

#define SREF(String) (PVOID)&(PH_STRINGREF)PH_STRINGREF_INIT((String))
static PH_STRINGREF PhpServiceUnknownString = PH_STRINGREF_INIT(L"Unknown");

static PH_KEY_VALUE_PAIR PhpServiceStatePairs[] =
{
    SIP(SREF(L"Stopped"), SERVICE_STOPPED),
    SIP(SREF(L"Start pending"), SERVICE_START_PENDING),
    SIP(SREF(L"Stop pending"), SERVICE_STOP_PENDING),
    SIP(SREF(L"Running"), SERVICE_RUNNING),
    SIP(SREF(L"Continue pending"), SERVICE_CONTINUE_PENDING),
    SIP(SREF(L"Pause pending"), SERVICE_PAUSE_PENDING),
    SIP(SREF(L"Paused"), SERVICE_PAUSED)
};

static PH_KEY_VALUE_PAIR PhpServiceTypePairs[] =
{
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

static PH_KEY_VALUE_PAIR PhpServiceStartTypePairs[] =
{
    SIP(SREF(L"Disabled"), SERVICE_DISABLED),
    SIP(SREF(L"Boot start"), SERVICE_BOOT_START),
    SIP(SREF(L"System start"), SERVICE_SYSTEM_START),
    SIP(SREF(L"Auto start"), SERVICE_AUTO_START),
    SIP(SREF(L"Demand start"), SERVICE_DEMAND_START)
};

static PH_KEY_VALUE_PAIR PhpServiceErrorControlPairs[] =
{
    SIP(SREF(L"Ignore"), SERVICE_ERROR_IGNORE),
    SIP(SREF(L"Normal"), SERVICE_ERROR_NORMAL),
    SIP(SREF(L"Severe"), SERVICE_ERROR_SEVERE),
    SIP(SREF(L"Critical"), SERVICE_ERROR_CRITICAL)
};

PWSTR PhServiceTypeStrings[] =
{
    L"Driver",
    L"FS driver",
    L"Own process",
    L"Share process",
    L"Own interactive process",
    L"Share interactive process",
    L"User own process",
    L"User own process (instance)",
    L"User share process",
    L"User share process (instance)",
    L"Package own process",
    L"Package share process",
};

PWSTR PhServiceStartTypeStrings[5] =
{
    L"Disabled",
    L"Boot start",
    L"System start",
    L"Auto start",
    L"Demand start"
};

PWSTR PhServiceErrorControlStrings[4] =
{
    L"Ignore",
    L"Normal",
    L"Severe",
    L"Critical"
};

_Success_(return != NULL)
PVOID PhEnumServices(
    _In_ SC_HANDLE ScManagerHandle,
    _In_opt_ ULONG Type,
    _In_opt_ ULONG State,
    _Out_ PULONG Count
    )
{
    static ULONG initialBufferSize = 0x8000;
    BOOL result;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;
    ULONG servicesReturned;

    if (!Type)
    {
        if (WindowsVersion >= WINDOWS_10_RS1)
        {
            Type = SERVICE_TYPE_ALL;
        }
        else if (WindowsVersion >= WINDOWS_10)
        {
            Type = SERVICE_WIN32 |
                SERVICE_ADAPTER |
                SERVICE_DRIVER |
                SERVICE_INTERACTIVE_PROCESS |
                SERVICE_USER_SERVICE |
                SERVICE_USERSERVICE_INSTANCE;
        }
        else
        {
            Type = SERVICE_DRIVER | SERVICE_WIN32;
        }
    }

    if (!State)
        State = SERVICE_STATE_ALL;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    if (!(result = EnumServicesStatusEx(
        ScManagerHandle,
        SC_ENUM_PROCESS_INFO,
        Type,
        State,
        (PBYTE)buffer,
        bufferSize,
        &returnLength,
        &servicesReturned,
        NULL,
        NULL
        )))
    {
        if (GetLastError() == ERROR_MORE_DATA)
        {
            PhFree(buffer);
            bufferSize += returnLength;
            buffer = PhAllocate(bufferSize);

            result = EnumServicesStatusEx(
                ScManagerHandle,
                SC_ENUM_PROCESS_INFO,
                Type,
                State,
                (PBYTE)buffer,
                bufferSize,
                &returnLength,
                &servicesReturned,
                NULL,
                NULL
                );
        }

        if (!result)
        {
            PhFree(buffer);
            return NULL;
        }
    }

    if (bufferSize <= 0x20000) initialBufferSize = bufferSize;
    *Count = servicesReturned;

    return buffer;
}

SC_HANDLE PhGetServiceManagerHandle(
    VOID
    )
{
    static SC_HANDLE cachedServiceManagerHandle = NULL;
    SC_HANDLE serviceManagerHandle;
    SC_HANDLE newServiceManagerHandle;

    // Use the cached value if possible.

    serviceManagerHandle = InterlockedCompareExchangePointer(&cachedServiceManagerHandle, NULL, NULL);

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
                CloseServiceHandle(newServiceManagerHandle);
            }
        }
    }

    return serviceManagerHandle;
}

SC_HANDLE PhOpenService(
    _In_ PWSTR ServiceName,
    _In_ ACCESS_MASK DesiredAccess
    )
{
    SC_HANDLE serviceHandle;

    serviceHandle = OpenService(PhGetServiceManagerHandle(), ServiceName, DesiredAccess);

    return serviceHandle;
}

VOID PhCloseServiceHandle(
    _In_ SC_HANDLE ServiceHandle
    )
{
    CloseServiceHandle(ServiceHandle);
}

PVOID PhGetServiceConfig(
    _In_ SC_HANDLE ServiceHandle
    )
{
    PVOID buffer;
    ULONG bufferSize = 0x200;

    buffer = PhAllocate(bufferSize);

    if (!QueryServiceConfig(ServiceHandle, buffer, bufferSize, &bufferSize))
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        if (!QueryServiceConfig(ServiceHandle, buffer, bufferSize, &bufferSize))
        {
            PhFree(buffer);
            return NULL;
        }
    }

    return buffer;
}

PVOID PhQueryServiceVariableSize(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG InfoLevel
    )
{
    PVOID buffer;
    ULONG bufferSize = 0x100;

    buffer = PhAllocate(bufferSize);

    if (!QueryServiceConfig2(
        ServiceHandle,
        InfoLevel,
        (BYTE *)buffer,
        bufferSize,
        &bufferSize
        ))
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        if (!QueryServiceConfig2(
            ServiceHandle,
            InfoLevel,
            (BYTE *)buffer,
            bufferSize,
            &bufferSize
            ))
        {
            PhFree(buffer);
            return NULL;
        }
    }

    return buffer;
}

PPH_STRING PhGetServiceDescription(
    _In_ SC_HANDLE ServiceHandle
    )
{
    PPH_STRING description = NULL;
    LPSERVICE_DESCRIPTION serviceDescription;

    serviceDescription = PhQueryServiceVariableSize(ServiceHandle, SERVICE_CONFIG_DESCRIPTION);

    if (serviceDescription)
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
    ULONG returnLength;

    if (QueryServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
        (BYTE *)&delayedAutoStartInfo,
        sizeof(SERVICE_DELAYED_AUTO_START_INFO),
        &returnLength
        ))
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

    return !!ChangeServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
        &delayedAutoStartInfo
        );
}

PPH_STRINGREF PhGetServiceStateString(
    _In_ ULONG ServiceState
    )
{
    PPH_STRINGREF string;

    if (PhFindStringSiKeyValuePairs(
        PhpServiceStatePairs,
        sizeof(PhpServiceStatePairs),
        ServiceState,
        (PWSTR*)&string
        ))
        return string;
    else
        return &PhpServiceUnknownString;
}

PPH_STRINGREF PhGetServiceTypeString(
    _In_ ULONG ServiceType
    )
{
    PPH_STRINGREF string;

    if (PhFindStringSiKeyValuePairs(
        PhpServiceTypePairs,
        sizeof(PhpServiceTypePairs),
        ServiceType,
        (PWSTR*)&string
        ))
        return string;
    else
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

PPH_STRINGREF PhGetServiceStartTypeString(
    _In_ ULONG ServiceStartType
    )
{
    PPH_STRINGREF string;

    if (PhFindStringSiKeyValuePairs(
        PhpServiceStartTypePairs,
        sizeof(PhpServiceStartTypePairs),
        ServiceStartType,
        (PWSTR*)&string
        ))
        return string;
    else
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

PPH_STRINGREF PhGetServiceErrorControlString(
    _In_ ULONG ServiceErrorControl
    )
{
    PPH_STRINGREF string;

    if (PhFindStringSiKeyValuePairs(
        PhpServiceErrorControlPairs,
        sizeof(PhpServiceErrorControlPairs),
        ServiceErrorControl,
        (PWSTR*)&string
        ))
        return string;
    else
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
    static PQUERY_TAG_INFORMATION I_QueryTagInformation = NULL;
    PPH_STRING serviceName = NULL;
    TAG_INFO_NAME_FROM_TAG nameFromTag;

    if (!I_QueryTagInformation)
    {
        I_QueryTagInformation = PhGetDllProcedureAddress(L"advapi32.dll", "I_QueryTagInformation", 0);

        if (!I_QueryTagInformation)
            return NULL;
    }

    memset(&nameFromTag, 0, sizeof(TAG_INFO_NAME_FROM_TAG));
    nameFromTag.InParams.dwPid = HandleToUlong(ProcessId);
    nameFromTag.InParams.dwTag = PtrToUlong(ServiceTag);

    I_QueryTagInformation(NULL, eTagInfoLevelNameFromTag, &nameFromTag);

    if (nameFromTag.OutParams.pszName)
    {
        serviceName = PhCreateString(nameFromTag.OutParams.pszName);
        LocalFree(nameFromTag.OutParams.pszName);
    }

    return serviceName;
}

PPH_STRING PhGetServiceNameForModuleReference(
    _In_ HANDLE ProcessId,
    _In_ PWSTR ModuleName
    )
{
    static PQUERY_TAG_INFORMATION I_QueryTagInformation = NULL;
    PPH_STRING serviceNames = NULL;
    TAG_INFO_NAMES_REFERENCING_MODULE moduleNameRef;

    if (!I_QueryTagInformation)
    {
        I_QueryTagInformation = PhGetDllProcedureAddress(L"advapi32.dll", "I_QueryTagInformation", 0);

        if (!I_QueryTagInformation)
            return NULL;
    }

    memset(&moduleNameRef, 0, sizeof(TAG_INFO_NAMES_REFERENCING_MODULE));
    moduleNameRef.InParams.dwPid = HandleToUlong(ProcessId);
    moduleNameRef.InParams.pszModule = ModuleName;

    I_QueryTagInformation(NULL, eTagInfoLevelNamesReferencingModule, &moduleNameRef);

    if (moduleNameRef.OutParams.pmszNames)
    {
        PH_STRING_BUILDER sb;
        PWSTR serviceName;

        PhInitializeStringBuilder(&sb, 0x40);

        for (serviceName = moduleNameRef.OutParams.pmszNames; *serviceName; serviceName += PhCountStringZ(serviceName) + 1)
            PhAppendFormatStringBuilder(&sb, L"%s, ", serviceName);

        if (sb.String->Length != 0)
            PhRemoveEndStringBuilder(&sb, 2);

        serviceNames = PhFinalStringBuilderString(&sb);
        LocalFree(moduleNameRef.OutParams.pmszNames);
    }

    return serviceNames;
}

NTSTATUS PhGetThreadServiceTag(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle,
    _Out_ PVOID *ServiceTag
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    BOOLEAN openedProcessHandle = FALSE;

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

    if (!ProcessHandle)
    {
        if (!NT_SUCCESS(status = PhOpenProcess(
            &ProcessHandle,
            PROCESS_VM_READ,
            basicInfo.ClientId.UniqueProcess
            )))
            return status;

        openedProcessHandle = TRUE;
    }

    status = NtReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(basicInfo.TebBaseAddress, FIELD_OFFSET(TEB, SubProcessTag)),
        ServiceTag,
        sizeof(PVOID),
        NULL
        );

    if (openedProcessHandle)
        NtClose(ProcessHandle);

    return status;
}

PPH_STRING PhGetServiceKeyName(
    _In_ PPH_STRINGREF ServiceName
    )
{
    static PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");

    return PhConcatStringRef2(&servicesKeyName, ServiceName);
}

PPH_STRING PhGetServiceHandleFileName(
    _In_ SC_HANDLE ServiceHandle,
    _In_ PPH_STRINGREF ServiceName
    )
{
    PPH_STRING fileName = NULL;
    LPQUERY_SERVICE_CONFIG config;

    if (config = PhGetServiceConfig(ServiceHandle))
    {
        PhGetServiceDllParameter(config->dwServiceType, ServiceName, &fileName);

        if (!fileName)
        {
            PPH_STRING commandLine;

            if (config->lpBinaryPathName[0])
            {
                commandLine = PhCreateString(config->lpBinaryPathName);

                if (config->dwServiceType & SERVICE_WIN32)
                {
                    PH_STRINGREF dummyFileName;
                    PH_STRINGREF dummyArguments;

                    PhParseCommandLineFuzzy(&commandLine->sr, &dummyFileName, &dummyArguments, &fileName);

                    if (!fileName)
                        PhSwapReference(&fileName, commandLine);
                }
                else
                {
                    fileName = PhGetFileName(commandLine);
                }

                PhDereferenceObject(commandLine);
            }
        }

        PhFree(config);
    }

    return fileName;
}

PPH_STRING PhGetServiceFileName(
    _In_ PPH_STRINGREF ServiceName
    )
{
    PPH_STRING serviceDllString = NULL;
    NTSTATUS status;
    HANDLE keyHandle;
    PPH_STRING keyName;

    keyName = PhGetServiceKeyName(ServiceName);

    status = PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName->sr,
        0
        );

    if (NT_SUCCESS(status))
    {
        if (serviceDllString = PhQueryRegistryStringZ(keyHandle, L"ImagePath"))
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

    PhDereferenceObject(keyName);

    if (NT_SUCCESS(status))
    {
        return serviceDllString;
    }

    return NULL;
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

    return status;
}

NTSTATUS PhGetServiceDllParameter(
    _In_ ULONG ServiceType,
    _In_ PPH_STRINGREF ServiceName,
    _Out_ PPH_STRING *ServiceDll
    )
{
    static PH_STRINGREF parameters = PH_STRINGREF_INIT(L"\\Parameters");
    NTSTATUS status;
    PPH_STRING serviceDllString;
    PPH_STRING serviceKeyName;
    PPH_STRING keyName;

    if (ServiceType & SERVICE_USERSERVICE_INSTANCE)
    {
        PH_STRINGREF hostServiceName;
        PH_STRINGREF userSessionLuid;

        // The SCM creates multiple "user service instance" processes for each user session with the following template:
        // [Host Service Instance Name]_[LUID for Session]
        // The SCM internally uses the ServiceDll of the "host service instance" for all "user service instance" processes/services
        // and we need to parse the user service template and query the "host service instance" configuration. (hsebs)

        if (PhSplitStringRefAtLastChar(ServiceName, L'_', &hostServiceName, &userSessionLuid))
        {
            serviceKeyName = PhGetServiceKeyName(&hostServiceName);
            keyName = PhConcatStringRef2(&serviceKeyName->sr, &parameters);
        }
        else
        {
            serviceKeyName = PhGetServiceKeyName(ServiceName);
            keyName = PhConcatStringRef2(&serviceKeyName->sr, &parameters);
        }
    }
    else
    {
        serviceKeyName = PhGetServiceKeyName(ServiceName);
        keyName = PhConcatStringRef2(&serviceKeyName->sr, &parameters);
    }

    status = PhpGetServiceDllName(
        keyName,
        &serviceDllString
        );

    PhDereferenceObject(keyName);
    PhDereferenceObject(serviceKeyName);

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
    PPH_STRING serviceKeyName;
    HANDLE keyHandle;

    serviceKeyName = PhGetServiceKeyName(ServiceName);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &serviceKeyName->sr,
        0
        )))
    {
        serviceAppUserModelId = PhQueryRegistryStringZ(keyHandle, L"AppUserModelId");
        NtClose(keyHandle);
    }

    PhDereferenceObject(serviceKeyName);

    return serviceAppUserModelId;
}

PPH_STRING PhGetServicePackageFullName(
    _In_ PPH_STRINGREF ServiceName
    )
{
    PPH_STRING servicePackageName = NULL;
    PPH_STRING serviceKeyName;
    HANDLE keyHandle;

    serviceKeyName = PhGetServiceKeyName(ServiceName);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &serviceKeyName->sr,
        0
        )))
    {
        servicePackageName = PhQueryRegistryStringZ(keyHandle, L"PackageFullName");
        NtClose(keyHandle);
    }

    PhDereferenceObject(serviceKeyName);

    return servicePackageName;
}
