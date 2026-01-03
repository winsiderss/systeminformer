/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2018-2024
 *
 */

#ifndef _PH_PHNATINL_H
#define _PH_PHNATINL_H

// This file contains inlined native API wrapper functions. These functions were previously
// exported, but are now inlined because they are extremely simple wrappers around equivalent native
// API functions.

/**
 * Gets basic information for a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param BasicInformation A variable which receives the information.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhGetProcessBasicInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_BASIC_INFORMATION BasicInformation
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessBasicInformation,
        BasicInformation,
        sizeof(PROCESS_BASIC_INFORMATION),
        NULL
        );
}

/**
 * Gets extended basic information for a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param ExtendedBasicInformation A variable which receives the information.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhGetProcessExtendedBasicInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_EXTENDED_BASIC_INFORMATION ExtendedBasicInformation
    )
{
    ExtendedBasicInformation->Size = sizeof(PROCESS_EXTENDED_BASIC_INFORMATION);

    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessBasicInformation,
        ExtendedBasicInformation,
        sizeof(PROCESS_EXTENDED_BASIC_INFORMATION),
        NULL
        );
}

/**
 * Gets time information for a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param Times A variable which receives the information.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhGetProcessTimes(
    _In_ HANDLE ProcessHandle,
    _Out_ PKERNEL_USER_TIMES Times
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessTimes,
        Times,
        sizeof(KERNEL_USER_TIMES),
        NULL
        );
}

/**
 * Gets a process' session ID.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param SessionId A variable which receives the process' session ID.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhGetProcessSessionId(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG SessionId
    )
{
    NTSTATUS status;
    PROCESS_SESSION_INFORMATION sessionInfo;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessSessionInformation,
        &sessionInfo,
        sizeof(PROCESS_SESSION_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *SessionId = sessionInfo.SessionId;
    }

    return status;
}

/**
 * Gets whether a process is running under 32-bit emulation.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param IsWow64Process A variable which receives a boolean indicating whether the process is 32-bit.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhGetProcessIsWow64(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsWow64Process
    )
{
    NTSTATUS status;
    ULONG_PTR wow64;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessWow64Information,
        &wow64,
        sizeof(ULONG_PTR),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *IsWow64Process = !!wow64;
    }

    return status;
}

/**
 * Gets a process' WOW64 PEB address.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param Peb32 A variable which receives the base address of the process' WOW64 PEB. If the process
 * is 64-bit, the variable receives NULL.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhGetProcessPeb32(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* Peb32
    )
{
    NTSTATUS status;
    ULONG_PTR wow64;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessWow64Information,
        &wow64,
        sizeof(ULONG_PTR),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        // No PEB for System, Minimal or Pico processes. (dmex)
        if (!wow64)
            return STATUS_UNSUCCESSFUL;

        *Peb32 = (PVOID)wow64;
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessPeb(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* PebBaseAddress
    )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION basicInfo;

    status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo);

    if (NT_SUCCESS(status))
    {
        // No PEB for System, Minimal or Pico processes. (dmex)
        if (!basicInfo.PebBaseAddress)
            return STATUS_UNSUCCESSFUL;

        *PebBaseAddress = (PVOID)basicInfo.PebBaseAddress;
    }

    return status;
}

/**
 * Gets a handle to a process' debug object.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION access.
 * \param DebugObjectHandle A variable which receives a handle to the debug object associated with
 * the process. You must close the handle when you no longer need it.
 * \return Successful or errant status.
 * \retval STATUS_PORT_NOT_SET The process is not being debugged and has no associated debug object.
 */
FORCEINLINE
NTSTATUS
PhGetProcessDebugObject(
    _In_ HANDLE ProcessHandle,
    _Out_ PHANDLE DebugObjectHandle
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessDebugObjectHandle,
        DebugObjectHandle,
        sizeof(HANDLE),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetProcessEnergyValues(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_EXTENDED_ENERGY_VALUES EnergyValues
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessEnergyValues,
        EnergyValues,
        sizeof(PROCESS_EXTENDED_ENERGY_VALUES),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetProcessErrorMode(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ErrorMode
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessDefaultHardErrorMode,
        ErrorMode,
        sizeof(ULONG),
        NULL
        );
}

/**
 * Sets the error mode for a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_SET_INFORMATION access.
 * \param ErrorMode The error mode to set for the process.
 * \return STATUS_SUCCESS if the error mode was successfully set, otherwise an appropriate NTSTATUS error code.
 */
FORCEINLINE
NTSTATUS
PhSetProcessErrorMode(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG ErrorMode
    )
{
    return NtSetInformationProcess(
        ProcessHandle,
        ProcessDefaultHardErrorMode,
        &ErrorMode,
        sizeof(ULONG)
        );
}

/**
 * Gets a process' no-execute status.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION access.
 * \param ExecuteFlags A variable which receives the no-execute flags.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhGetProcessExecuteFlags(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ExecuteFlags
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessExecuteFlags,
        ExecuteFlags,
        sizeof(ULONG),
        NULL
        );
}

/**
 * Gets a process' I/O priority.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param IoPriority A variable which receives the I/O priority of the process.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhGetProcessIoPriority(
    _In_ HANDLE ProcessHandle,
    _Out_ IO_PRIORITY_HINT *IoPriority
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessIoPriority,
        IoPriority,
        sizeof(IO_PRIORITY_HINT),
        NULL
        );
}

/**
 * Gets a process' page priority.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param PagePriority A variable which receives the page priority of the process.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhGetProcessPagePriority(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG PagePriority
    )
{
    NTSTATUS status;
    PAGE_PRIORITY_INFORMATION pagePriorityInfo;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessPagePriority,
        &pagePriorityInfo,
        sizeof(PAGE_PRIORITY_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *PagePriority = pagePriorityInfo.PagePriority;
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessPriorityBoost(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN PriorityBoostDisabled
    )
{
    NTSTATUS status;
    ULONG priorityBoostDisabled;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessPriorityBoost,
        &priorityBoostDisabled,
        sizeof(ULONG),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *PriorityBoostDisabled = !!priorityBoostDisabled;
    }

    return status;
}

/**
 * Gets a process' cycle count.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param CycleTime A variable which receives the 64-bit cycle time.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhGetProcessCycleTime(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG64 CycleTime
    )
{
    NTSTATUS status;
    PROCESS_CYCLE_TIME_INFORMATION cycleTimeInfo;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessCycleTime,
        &cycleTimeInfo,
        sizeof(PROCESS_CYCLE_TIME_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *CycleTime = cycleTimeInfo.AccumulatedCycles;
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessUptime(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_UPTIME_INFORMATION Uptime
    )
{
    NTSTATUS status;
    PROCESS_UPTIME_INFORMATION uptimeInfo;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessUptimeInformation,
        &uptimeInfo,
        sizeof(PROCESS_UPTIME_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *Uptime = uptimeInfo;
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessConsoleHostProcessId(
    _In_ HANDLE ProcessHandle,
    _Out_ PHANDLE ConsoleHostProcessId
    )
{
    NTSTATUS status;
    ULONG_PTR consoleHostProcess;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessConsoleHostProcess,
        &consoleHostProcess,
        sizeof(ULONG_PTR),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ConsoleHostProcessId = (HANDLE)consoleHostProcess;
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessConsoleHostProcess(
    _In_ HANDLE ProcessHandle,
    _Out_ PHANDLE ConsoleHostProcessId,
    _Out_opt_ PBOOLEAN ConsoleApplication
    )
{
    NTSTATUS status;
    ULONG_PTR consoleHostProcess;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessConsoleHostProcess,
        &consoleHostProcess,
        sizeof(ULONG_PTR),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ConsoleHostProcessId = (HANDLE)(consoleHostProcess & ~3);
    }

    if (ConsoleApplication)
    {
        *ConsoleApplication = !!(ULONG_PTR)(consoleHostProcess & 2);
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessProtection(
    _In_ HANDLE ProcessHandle,
    _Out_ PPS_PROTECTION Protection
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessProtectionInformation,
        Protection,
        sizeof(PS_PROTECTION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetProcessAffinityMask(
    _In_ HANDLE ProcessHandle,
    _Out_ PKAFFINITY AffinityMask
    )
{
    NTSTATUS status;
    KAFFINITY affinityMask;

    memset(&affinityMask, 0, sizeof(KAFFINITY));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessAffinityMask,
        &affinityMask,
        sizeof(KAFFINITY),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *AffinityMask = affinityMask;
    }
    else // Windows 7 (dmex)
    {
        PROCESS_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhGetProcessBasicInformation(ProcessHandle, &basicInfo)))
        {
            *AffinityMask = basicInfo.AffinityMask;
            return STATUS_SUCCESS;
        }
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessGroupInformation(
    _In_ HANDLE ProcessHandle,
    _Inout_ PUSHORT GroupCount,
    _Out_ PUSHORT GroupArray
    )
{
    NTSTATUS status;
    ULONG returnLength;

    // rev from GetProcessGroupAffinity (dmex)
    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessGroupInformation,
        GroupArray,
        sizeof(USHORT) * *GroupCount,
        &returnLength
        );

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL)
    {
        *GroupCount = (USHORT)returnLength / sizeof(USHORT); // (USHORT)returnLength >> 1
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessGroupAffinity(
    _In_ HANDLE ProcessHandle,
    _Out_ PGROUP_AFFINITY GroupAffinity
    )
{
    NTSTATUS status;
    GROUP_AFFINITY groupAffinity;

    memset(&groupAffinity, 0, sizeof(GROUP_AFFINITY));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessAffinityMask,
        &groupAffinity,
        sizeof(GROUP_AFFINITY),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        memcpy(GroupAffinity, &groupAffinity, sizeof(GROUP_AFFINITY));
    }
    else // Windows 7 (dmex)
    {
        KAFFINITY affinityMask;

        if (NT_SUCCESS(PhGetProcessAffinityMask(ProcessHandle, &affinityMask)))
        {
            groupAffinity.Mask = affinityMask;
            memcpy(GroupAffinity, &groupAffinity, sizeof(GROUP_AFFINITY));
            return STATUS_SUCCESS;
        }
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessIsCFGuardEnabled(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsControlFlowGuardEnabled
    )
{
    NTSTATUS status;
    PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

    policyInfo.Policy = ProcessControlFlowGuardPolicy;
    policyInfo.ControlFlowGuardPolicy.Flags = 0;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessMitigationPolicy,
        &policyInfo,
        sizeof(PROCESS_MITIGATION_POLICY_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *IsControlFlowGuardEnabled = !!policyInfo.ControlFlowGuardPolicy.EnableControlFlowGuard;
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessIsXFGuardEnabled(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsXFGuardEnabled,
    _Out_ PBOOLEAN IsXFGuardAuditEnabled
    )
{
    NTSTATUS status;
    PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

    policyInfo.Policy = ProcessControlFlowGuardPolicy;
    policyInfo.ControlFlowGuardPolicy.Flags = 0;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessMitigationPolicy,
        &policyInfo,
        sizeof(PROCESS_MITIGATION_POLICY_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
#if !defined(NTDDI_WIN10_CO) || (NTDDI_VERSION < NTDDI_WIN10_CO)
        *IsXFGuardEnabled = _bittest((const PLONG)&policyInfo.ControlFlowGuardPolicy.Flags, 3);
        *IsXFGuardAuditEnabled = _bittest((const PLONG)&policyInfo.ControlFlowGuardPolicy.Flags, 4);
#else
        *IsXFGuardEnabled = !!policyInfo.ControlFlowGuardPolicy.EnableXfg;
        *IsXFGuardAuditEnabled = !!policyInfo.ControlFlowGuardPolicy.EnableXfgAuditMode;
#endif
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessHandleCount(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_HANDLE_INFORMATION HandleInfo
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessHandleCount,
        HandleInfo,
        sizeof(PROCESS_HANDLE_INFORMATION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetProcessBreakOnTermination(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN BreakOnTermination
    )
{
    NTSTATUS status;
    ULONG breakOnTermination;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessBreakOnTermination,
        &breakOnTermination,
        sizeof(ULONG),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *BreakOnTermination = !!breakOnTermination;
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhSetProcessBreakOnTermination(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN BreakOnTermination
    )
{
    ULONG breakOnTermination;

    breakOnTermination = BreakOnTermination ? 1 : 0;

    return NtSetInformationProcess(
        ProcessHandle,
        ProcessBreakOnTermination,
        &breakOnTermination,
        sizeof(ULONG)
        );
}

FORCEINLINE
NTSTATUS
PhGetProcessAppMemoryInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_JOB_MEMORY_INFO JobMemoryInfo
    )
{
    NTSTATUS status;
    PROCESS_JOB_MEMORY_INFO jobMemoryInfo;

    // Win32 called this ProcessAppMemoryInfo with APP_MEMORY_INFORMATION struct (dmex)
    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessJobMemoryInformation,
        &jobMemoryInfo,
        sizeof(PROCESS_JOB_MEMORY_INFO),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *JobMemoryInfo = jobMemoryInfo;
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetProcessMitigationPolicy(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESS_MITIGATION_POLICY Policy,
    _Out_ PPROCESS_MITIGATION_POLICY_INFORMATION MitigationPolicy
    )
{
    memset(MitigationPolicy, 0, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));
    MitigationPolicy->Policy = Policy;

    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessMitigationPolicy,
        MitigationPolicy,
        sizeof(PROCESS_MITIGATION_POLICY_INFORMATION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetProcessNetworkIoCounters(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_NETWORK_COUNTERS NetworkIoCounters
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessNetworkIoCounters,
        NetworkIoCounters,
        sizeof(PROCESS_NETWORK_COUNTERS),
        NULL
        );
}

/**
 * Gets basic information for a event.
 *
 * \param EventHandle A handle to a event. The handle must have EVENT_QUERY_STATE access.
 * \param BasicInformation A variable which receives the information.
 */
FORCEINLINE
NTSTATUS
PhGetEventBasicInformation(
    _In_ HANDLE EventHandle,
    _Out_ PEVENT_BASIC_INFORMATION BasicInformation
    )
{
    return NtQueryEvent(
        EventHandle,
        EventBasicInformation,
        BasicInformation,
        sizeof(EVENT_BASIC_INFORMATION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhOpenMutant(
    _Out_ PHANDLE MutantHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_opt_ PCPH_STRINGREF ObjectName
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (ObjectName)
    {
        if (!PhStringRefToUnicodeString(ObjectName, &objectName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&objectName, NULL, 0);
    }

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenMutant(
        MutantHandle,
        DesiredAccess,
        &objectAttributes
        );

    return status;
}

FORCEINLINE
NTSTATUS
PhGetMutantBasicInformation(
    _In_ HANDLE MutantHandle,
    _Out_ PMUTANT_BASIC_INFORMATION BasicInformation
    )
{
    return NtQueryMutant(
        MutantHandle,
        MutantBasicInformation,
        BasicInformation,
        sizeof(MUTANT_BASIC_INFORMATION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetMutantOwnerInformation(
    _In_ HANDLE MutantHandle,
    _Out_ PMUTANT_OWNER_INFORMATION OwnerInformation
    )
{
    return NtQueryMutant(
        MutantHandle,
        MutantOwnerInformation,
        OwnerInformation,
        sizeof(MUTANT_OWNER_INFORMATION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetSectionBasicInformation(
    _In_ HANDLE SectionHandle,
    _Out_ PSECTION_BASIC_INFORMATION BasicInformation
    )
{
    return NtQuerySection(
        SectionHandle,
        SectionBasicInformation,
        BasicInformation,
        sizeof(SECTION_BASIC_INFORMATION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetSemaphoreBasicInformation(
    _In_ HANDLE SemaphoreHandle,
    _Out_ PSEMAPHORE_BASIC_INFORMATION BasicInformation
    )
{
    return NtQuerySemaphore(
        SemaphoreHandle,
        SemaphoreBasicInformation,
        BasicInformation,
        sizeof(SEMAPHORE_BASIC_INFORMATION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetTimerBasicInformation(
    _In_ HANDLE TimerHandle,
    _Out_ PTIMER_BASIC_INFORMATION BasicInformation
    )
{
    return NtQueryTimer(
        TimerHandle,
        TimerBasicInformation,
        BasicInformation,
        sizeof(TIMER_BASIC_INFORMATION),
        NULL
        );
}

/**
 * The action to be performed when the calling thread exits.
 *
 * \param DebugObjectHandle A handle to a process' debug object.
 * \param KillProcessOnExit If this parameter is TRUE, the thread terminates all attached processes on exit
 * (note that this is the default). Otherwise, the thread detaches from all processes being debugged on exit.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhSetDebugKillProcessOnExit(
    _In_ HANDLE DebugObjectHandle,
    _In_ BOOLEAN KillProcessOnExit
    )
{
    ULONG killProcessOnExit;

    killProcessOnExit = KillProcessOnExit ? 1 : 0;

    return NtSetInformationDebugObject(
        DebugObjectHandle,
        DebugObjectKillProcessOnExitInformation,
        &killProcessOnExit,
        sizeof(ULONG),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetProcessIsCetEnabled(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsCetEnabled,
    _Out_ PBOOLEAN IsCetStrictModeEnabled
    )
{
    NTSTATUS status;
    PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

    policyInfo.Policy = ProcessUserShadowStackPolicy;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessMitigationPolicy,
        &policyInfo,
        sizeof(PROCESS_MITIGATION_POLICY_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
#if !defined(NTDDI_WIN10_CO) || (NTDDI_VERSION < NTDDI_WIN10_CO)
        *IsCetEnabled = _bittest((const PLONG)&policyInfo.ControlFlowGuardPolicy.Flags, 0);
        *IsCetStrictModeEnabled = _bittest((const PLONG)&policyInfo.ControlFlowGuardPolicy.Flags, 4);
#else
        *IsCetEnabled = !!policyInfo.UserShadowStackPolicy.EnableUserShadowStack;
        *IsCetStrictModeEnabled = !!policyInfo.UserShadowStackPolicy.EnableUserShadowStackStrictMode;
#endif
    }

    return status;
}

FORCEINLINE
NTSTATUS
NTAPI
PhGetSystemHypervisorSharedPageInformation(
    _Out_ PSYSTEM_HYPERVISOR_USER_SHARED_DATA* HypervisorSharedUserVa
    )
{
    NTSTATUS status;
    SYSTEM_HYPERVISOR_SHARED_PAGE_INFORMATION HypervisorSharedPageInfo;

    status = NtQuerySystemInformation(
        SystemHypervisorSharedPageInformation,
        &HypervisorSharedPageInfo,
        sizeof(SYSTEM_HYPERVISOR_SHARED_PAGE_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *HypervisorSharedUserVa = HypervisorSharedPageInfo.HypervisorSharedUserVa;
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetSystemShadowStackInformation(
    _Out_ PSYSTEM_SHADOW_STACK_INFORMATION ShadowStackInformation
    )
{
    return NtQuerySystemInformation(
        SystemShadowStackInformation,
        ShadowStackInformation,
        sizeof(SYSTEM_SHADOW_STACK_INFORMATION),
        NULL
        );
}

/**
* The system boot time in Coordinated Universal Time (UTC) format.
*
* \param BootTime A pointer to a LARGE_INTEGER structure to receive the current system boot date and time.
* \return Successful or errant status.
*/
FORCEINLINE
NTSTATUS
PhGetSystemBootTime(
    _Out_ PLARGE_INTEGER BootTime
    )
{
    NTSTATUS status;
    SYSTEM_TIMEOFDAY_INFORMATION timeOfDayInfo;

    status = NtQuerySystemInformation(
        SystemTimeOfDayInformation,
        &timeOfDayInfo,
        sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *BootTime = timeOfDayInfo.BootTime;
    }

    return status;
}

/**
 * The system uptime in Coordinated Universal Time (UTC) format.
 *
 * \param Uptime A pointer to a LARGE_INTEGER structure to receive the current system uptime.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhGetSystemUptime(
    _Out_ PLARGE_INTEGER Uptime
    )
{
    NTSTATUS status;
    SYSTEM_TIMEOFDAY_INFORMATION timeOfDayInfo;

    status = NtQuerySystemInformation(
        SystemTimeOfDayInformation,
        &timeOfDayInfo,
        sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        Uptime->QuadPart = timeOfDayInfo.CurrentTime.QuadPart - timeOfDayInfo.BootTime.QuadPart;
    }

    return status;
}

/**
 * Waits until the specified object is in the signaled state or the time-out interval elapses.
 *
 * \param Handle A handle to the object.
 * \param Timeout The time-out interval.
 * If a nonzero value is specified, the function waits until the object is signaled or the interval elapses.
 * If Timeout is zero, the function does not enter a wait state if the object is not signaled; it always returns immediately.
 * If Timeout is INFINITE, the function will return only when the object is signaled.
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhWaitForSingleObject(
    _In_ HANDLE Handle,
    _In_opt_ ULONG Timeout
    )
{
    if (Timeout)
    {
        LARGE_INTEGER timeout;

        timeout.QuadPart = -(LONGLONG)UInt32x32To64(Timeout, PH_TIMEOUT_MS);

        return NtWaitForSingleObject(Handle, FALSE, &timeout);
    }
    else
    {
        return NtWaitForSingleObject(Handle, FALSE, NULL);
    }
}

/**
 * Provides a process-wide memory barrier that ensures that reads and writes from any CPU cannot move across the barrier.
 *
 * The process-wide memory barrier method differs from the "normal" MemoryBarrier method as follows:
 * The process-wide memory barrier ensures that any read or write from any CPU being used in the process can't move across the barrier.
 * The process-wide memory barrier forces other CPUs to synchronize with process memory (for example, to flush write buffers and synchronize read buffers).
 * The process-wide memory barrier is very expensive. It has to force every CPU in the process do to something, at a probable cost of thousands of cycles.
 * The process-wide memory barrier suffers from all the subtleties of lock-free programming.
 * \sa https://learn.microsoft.com/en-us/dotnet/api/system.threading.interlocked.memorybarrierprocesswid
 * \return Successful or errant status.
 */
FORCEINLINE
NTSTATUS
PhMemoryBarrierProcessWide(
    VOID
    )
{
    return NtFlushProcessWriteBuffers();
}

#endif
