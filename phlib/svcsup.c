/*
 * Process Hacker -
 *   service support functions
 *
 * Copyright (C) 2010-2011 wj32
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
#include <winmisc.h>

#define SIP(String, Integer) { (String), (PVOID)(Integer) }

static PH_KEY_VALUE_PAIR PhpServiceStatePairs[] =
{
    SIP(L"Stopped", SERVICE_STOPPED),
    SIP(L"Start Pending", SERVICE_START_PENDING),
    SIP(L"Stop Pending", SERVICE_STOP_PENDING),
    SIP(L"Running", SERVICE_RUNNING),
    SIP(L"Continue Pending", SERVICE_CONTINUE_PENDING),
    SIP(L"Pause Pending", SERVICE_PAUSE_PENDING),
    SIP(L"Paused", SERVICE_PAUSED)
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

WCHAR *PhServiceTypeStrings[6] = { L"Driver", L"FS Driver", L"Own Process", L"Share Process",
    L"Own Interactive Process", L"Share Interactive Process" };
WCHAR *PhServiceStartTypeStrings[5] = { L"Disabled", L"Boot Start", L"System Start",
    L"Auto Start", L"Demand Start" };
WCHAR *PhServiceErrorControlStrings[4] = { L"Ignore", L"Normal", L"Severe", L"Critical" };

PVOID PhEnumServices(
    __in SC_HANDLE ScManagerHandle,
    __in_opt ULONG Type,
    __in_opt ULONG State,
    __out PULONG Count
    )
{
    static ULONG initialBufferSize = 0x8000;
    LOGICAL result;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;
    ULONG servicesReturned;

    if (!Type)
        Type = SERVICE_DRIVER | SERVICE_WIN32;
    if (!State)
        State = SERVICE_STATE_ALL;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    if (!(result = EnumServicesStatusEx(
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
                buffer,
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

    if (bufferSize <= 0x10000) initialBufferSize = bufferSize;
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

PVOID PhQueryServiceVariableSize(
    __in SC_HANDLE ServiceHandle,
    __in ULONG InfoLevel
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
    __in SC_HANDLE ServiceHandle
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

BOOLEAN PhGetServiceDelayedAutoStart(
    __in SC_HANDLE ServiceHandle,
    __out PBOOLEAN DelayedAutoStart
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
    __in SC_HANDLE ServiceHandle,
    __in BOOLEAN DelayedAutoStart
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
    static PQUERY_TAG_INFORMATION I_QueryTagInformation = NULL;
    PPH_STRING serviceName = NULL;
    TAG_INFO_NAME_FROM_TAG nameFromTag;

    if (!I_QueryTagInformation)
    {
        I_QueryTagInformation = PhGetProcAddress(L"advapi32.dll", "I_QueryTagInformation");

        if (!I_QueryTagInformation)
            return NULL;
    }

    memset(&nameFromTag, 0, sizeof(TAG_INFO_NAME_FROM_TAG));
    nameFromTag.InParams.dwPid = (ULONG)ProcessId;
    nameFromTag.InParams.dwTag = (ULONG)ServiceTag;

    I_QueryTagInformation(NULL, eTagInfoLevelNameFromTag, &nameFromTag);

    if (nameFromTag.OutParams.pszName)
    {
        serviceName = PhCreateString(nameFromTag.OutParams.pszName);
        LocalFree(nameFromTag.OutParams.pszName);
    }

    return serviceName;
}

NTSTATUS PhGetThreadServiceTag(
    __in HANDLE ThreadHandle,
    __in_opt HANDLE ProcessHandle,
    __out PVOID *ServiceTag
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
