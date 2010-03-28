/*
 * Process Hacker - 
 *   service support functions
 * 
 * Copyright (C) 2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ph.h>

#define SIP(String, Integer) { (String), (PVOID)(Integer) }

static PH_KEY_VALUE_PAIR PhpServiceStatePairs[] =
{
    SIP(L"Running", SERVICE_RUNNING),
    SIP(L"Paused", SERVICE_PAUSED),
    SIP(L"Start Pending", SERVICE_START_PENDING),
    SIP(L"Continue Pending", SERVICE_CONTINUE_PENDING),
    SIP(L"Pause Pending", SERVICE_PAUSE_PENDING),
    SIP(L"Stop Pending", SERVICE_STOP_PENDING)
};

static PH_KEY_VALUE_PAIR PhpServiceTypePairs[] =
{
    SIP(L"Driver", SERVICE_KERNEL_DRIVER),
    SIP(L"FS Driver", SERVICE_FILE_SYSTEM_DRIVER),
    SIP(L"Own Process", SERVICE_WIN32_OWN_PROCESS),
    SIP(L"Share Process", SERVICE_WIN32_SHARE_PROCESS),
    SIP(L"Own Interactive Process", SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS),
    SIP(L"Share Interactive Process", SERVICE_WIN32_SHARE_PROCESS | SERVICE_INTERACTIVE_PROCESS)
};

static PH_KEY_VALUE_PAIR PhpServiceStartTypePairs[] =
{
    SIP(L"Disabled", SERVICE_DISABLED),
    SIP(L"Boot Start", SERVICE_BOOT_START),
    SIP(L"System Start", SERVICE_SYSTEM_START),
    SIP(L"Auto Start", SERVICE_AUTO_START),
    SIP(L"Demand Start", SERVICE_DEMAND_START)
};

static PH_KEY_VALUE_PAIR PhpServiceErrorControlPairs[] =
{
    SIP(L"Ignore", SERVICE_ERROR_IGNORE),
    SIP(L"Normal", SERVICE_ERROR_NORMAL),
    SIP(L"Severe", SERVICE_ERROR_SEVERE),
    SIP(L"Critical", SERVICE_ERROR_CRITICAL)
};

PVOID PhEnumServices(
    __in SC_HANDLE ScManagerHandle,
    __in_opt ULONG Type,
    __in_opt ULONG State,
    __out PULONG Count
    )
{
    PVOID buffer;
    static ULONG bufferSize = 0x8000;
    ULONG returnLength;
    ULONG servicesReturned;

    if (!Type)
        Type = SERVICE_DRIVER | SERVICE_WIN32;
    if (!State)
        State = SERVICE_STATE_ALL;

    buffer = PhAllocate(bufferSize);

    if (!EnumServicesStatusEx(
        ScManagerHandle,
        SC_ENUM_PROCESS_INFO,
        Type,
        State,
        buffer,
        bufferSize,
        &returnLength,
        &servicesReturned,
        NULL,
        NULL
        ))
    {
        if (GetLastError() == ERROR_MORE_DATA)
        {
            PhFree(buffer);
            bufferSize += returnLength;
            buffer = PhAllocate(bufferSize);

            if (!EnumServicesStatusEx(
                ScManagerHandle,
                SC_ENUM_PROCESS_INFO,
                Type,
                State,
                buffer,
                bufferSize,
                &returnLength,
                &servicesReturned,
                NULL,
                NULL
                ))
            {
                PhFree(buffer);
                return NULL;
            }
        }
    }

    *Count = servicesReturned;

    return buffer;
}

SC_HANDLE PhOpenService(
    __in PWSTR ServiceName,
    __in ACCESS_MASK DesiredAccess
    )
{
    SC_HANDLE scManagerHandle;
    SC_HANDLE serviceHandle;

    scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (!scManagerHandle)
        return NULL;

    serviceHandle = OpenService(scManagerHandle, ServiceName, DesiredAccess);
    CloseServiceHandle(scManagerHandle);

    return serviceHandle;
}

PVOID PhGetServiceConfig(
    __in SC_HANDLE ServiceHandle
    )
{
    PVOID buffer;
    ULONG bufferSize = 0x100;

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

PPH_STRING PhGetServiceDescription(
    __in SC_HANDLE ServiceHandle
    )
{
    PVOID buffer;
    ULONG returnLength = 0x100;
    LPSERVICE_DESCRIPTION serviceDescription;
    PPH_STRING description = NULL;

    QueryServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_DESCRIPTION,
        NULL,
        0,
        &returnLength
        );
    buffer = PhAllocate(returnLength);

    if (QueryServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_DESCRIPTION,
        buffer,
        returnLength,
        &returnLength
        ))
    {
        serviceDescription = (LPSERVICE_DESCRIPTION)buffer;

        if (serviceDescription->lpDescription)
            description = PhCreateString(serviceDescription->lpDescription);
    }

    PhFree(buffer);

    return description;
}

PWSTR PhGetServiceStateString(
    __in ULONG ServiceState
    )
{
    PWSTR string;

    if (PhFindStringSiKeyValuePairs(
        PhpServiceStatePairs,
        sizeof(PhpServiceStatePairs),
        ServiceState,
        &string
        ))
        return string;
    else
        return L"Unknown";
}

PWSTR PhGetServiceTypeString(
    __in ULONG ServiceType
    )
{
    PWSTR string;

    if (PhFindStringSiKeyValuePairs(
        PhpServiceTypePairs,
        sizeof(PhpServiceTypePairs),
        ServiceType,
        &string
        ))
        return string;
    else
        return L"Unknown";
}

ULONG PhGetServiceTypeInteger(
    __in PWSTR ServiceType
    )
{
    ULONG integer;

    if (PhFindIntegerSiKeyValuePairs(
        PhpServiceTypePairs,
        sizeof(PhpServiceTypePairs),
        ServiceType,
        &integer
        ))
        return integer;
    else
        return -1;
}

PWSTR PhGetServiceStartTypeString(
    __in ULONG ServiceStartType
    )
{
    PWSTR string;

    if (PhFindStringSiKeyValuePairs(
        PhpServiceStartTypePairs,
        sizeof(PhpServiceStartTypePairs),
        ServiceStartType,
        &string
        ))
        return string;
    else
        return L"Unknown";
}

ULONG PhGetServiceStartTypeInteger(
    __in PWSTR ServiceStartType
    )
{
    ULONG integer;

    if (PhFindIntegerSiKeyValuePairs(
        PhpServiceStartTypePairs,
        sizeof(PhpServiceStartTypePairs),
        ServiceStartType,
        &integer
        ))
        return integer;
    else
        return -1;
}

PWSTR PhGetServiceErrorControlString(
    __in ULONG ServiceErrorControl
    )
{
    PWSTR string;

    if (PhFindStringSiKeyValuePairs(
        PhpServiceErrorControlPairs,
        sizeof(PhpServiceErrorControlPairs),
        ServiceErrorControl,
        &string
        ))
        return string;
    else
        return L"Unknown";
}

ULONG PhGetServiceErrorControlInteger(
    __in PWSTR ServiceErrorControl
    )
{
    ULONG integer;

    if (PhFindIntegerSiKeyValuePairs(
        PhpServiceErrorControlPairs,
        sizeof(PhpServiceErrorControlPairs),
        ServiceErrorControl,
        &integer
        ))
        return integer;
    else
        return -1;
}

PPH_STRING PhGetServiceNameFromTag(
    __in HANDLE ProcessId,
    __in PVOID ServiceTag
    )
{
    PPH_STRING serviceName = NULL;
    _I_QueryTagInformation I_QueryTagInformation;
    SC_SERVICE_TAG_QUERY query;

    I_QueryTagInformation = PhGetProcAddress(L"advapi32.dll", "I_QueryTagInformation");

    if (!I_QueryTagInformation)
        return NULL;

    query.ProcessId = (ULONG)ProcessId;
    query.ServiceTag = (ULONG)ServiceTag;
    query.Unknown = 0;
    query.Buffer = NULL;

    I_QueryTagInformation(NULL, ServiceNameFromTagInformation, &query);

    if (query.Buffer)
    {
        serviceName = PhCreateString((PWSTR)query.Buffer);
        LocalFree(query.Buffer);
    }

    return serviceName;
}

NTSTATUS PhGetThreadServiceTag(
    __in HANDLE ThreadHandle,
    __in_opt HANDLE ProcessHandle,
    __out PPVOID ServiceTag
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    BOOLEAN openedProcessHandle = FALSE;

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

    if (!ProcessHandle)
    {
        if (!NT_SUCCESS(status = PhOpenThreadProcess(
            &ProcessHandle,
            PROCESS_VM_READ,
            ThreadHandle
            )))
            return status;

        openedProcessHandle = TRUE;
    }

    status = PhReadVirtualMemory(
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
