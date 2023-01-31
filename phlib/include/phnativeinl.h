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
 * \param IsWow64 A variable which receives a boolean indicating whether the process is 32-bit.
 */
FORCEINLINE
NTSTATUS
PhGetProcessIsWow64(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsWow64
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
        *IsWow64 = !!wow64;
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
 */
FORCEINLINE
NTSTATUS
PhGetProcessPeb32(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID *Peb32
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
        // No PEB for System and minimal/pico processes. (dmex)
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

    status = PhGetProcessBasicInformation(
        ProcessHandle,
        &basicInfo
        );

    if (NT_SUCCESS(status))
    {
        // No PEB for System and minimal/pico processes. (dmex)
        if (!basicInfo.PebBaseAddress)
            return STATUS_UNSUCCESSFUL;

        *PebBaseAddress = (PVOID)basicInfo.PebBaseAddress;
    }

    return status;
}

/**
 * Gets a handle to a process' debug object.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION
 * access.
 * \param DebugObjectHandle A variable which receives a handle to the debug object associated with
 * the process. You must close the handle when you no longer need it.
 *
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
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION
 * access.
 * \param ExecuteFlags A variable which receives the no-execute flags.
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
PhGetProcessPowerThrottlingState(
    _In_ HANDLE ProcessHandle,
    _Out_ PPOWER_THROTTLING_PROCESS_STATE PowerThrottlingState
    )
{
    NTSTATUS status;
    POWER_THROTTLING_PROCESS_STATE powerThrottlingState;

    memset(&powerThrottlingState, 0, sizeof(POWER_THROTTLING_PROCESS_STATE));
    powerThrottlingState.Version = POWER_THROTTLING_PROCESS_CURRENT_VERSION;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessPowerThrottlingState,
        &powerThrottlingState,
        sizeof(POWER_THROTTLING_PROCESS_STATE),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *PowerThrottlingState = powerThrottlingState;
    }

    return status;
}

/**
 * Gets basic information for a thread.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION
 * access.
 * \param BasicInformation A variable which receives the information.
 */
FORCEINLINE
NTSTATUS
PhGetThreadBasicInformation(
    _In_ HANDLE ThreadHandle,
    _Out_ PTHREAD_BASIC_INFORMATION BasicInformation
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadBasicInformation,
        BasicInformation,
        sizeof(THREAD_BASIC_INFORMATION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetThreadBasePriority(
    _In_ HANDLE ThreadHandle,
    _Out_ PKPRIORITY Increment
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;

    status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo);

    if (NT_SUCCESS(status))
    {
        *Increment = basicInfo.BasePriority;
    }

    return status;

    //return NtQueryInformationThread(
    //    ThreadHandle,
    //    ThreadBasePriority,
    //    Increment,
    //    sizeof(LONG),
    //    NULL
    //    );
}

FORCEINLINE
NTSTATUS
PhGetThreadStartAddress(
    _In_ HANDLE ThreadHandle,
    _Out_ PVOID *StartAddress
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadQuerySetWin32StartAddress,
        StartAddress,
        sizeof(PVOID),
        NULL
        );
}

/**
 * Gets a thread's I/O priority.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION
 * access.
 * \param IoPriority A variable which receives the I/O priority of the thread.
 */
FORCEINLINE
NTSTATUS
PhGetThreadIoPriority(
    _In_ HANDLE ThreadHandle,
    _Out_ IO_PRIORITY_HINT *IoPriority
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadIoPriority,
        IoPriority,
        sizeof(IO_PRIORITY_HINT),
        NULL
        );
}

/**
 * Gets a thread's page priority.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION
 * access.
 * \param PagePriority A variable which receives the page priority of the thread.
 */
FORCEINLINE
NTSTATUS
PhGetThreadPagePriority(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG PagePriority
    )
{
    NTSTATUS status;
    PAGE_PRIORITY_INFORMATION pagePriorityInfo;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadPagePriority,
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
PhGetThreadPriorityBoost(
    _In_ HANDLE ThreadHandle,
    _Out_ PBOOLEAN PriorityBoost
    )
{
    NTSTATUS status;
    ULONG priorityBoost;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadPriorityBoost,
        &priorityBoost,
        sizeof(ULONG),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *PriorityBoost = !!priorityBoost;
    }

    return status;
}

/**
 * Gets a thread's cycle count.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION
 * access.
 * \param CycleTime A variable which receives the 64-bit cycle time.
 */
FORCEINLINE
NTSTATUS
PhGetThreadCycleTime(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG64 CycleTime
    )
{
    NTSTATUS status;
    THREAD_CYCLE_TIME_INFORMATION cycleTimeInfo;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadCycleTime,
        &cycleTimeInfo,
        sizeof(THREAD_CYCLE_TIME_INFORMATION),
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
PhGetThreadIdealProcessor(
    _In_ HANDLE ThreadHandle,
    _Out_ PPROCESSOR_NUMBER ProcessorNumber
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadIdealProcessorEx,
        ProcessorNumber,
        sizeof(PROCESSOR_NUMBER),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetThreadSuspendCount(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG SuspendCount
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadSuspendCount,
        SuspendCount,
        sizeof(ULONG),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetThreadWow64Context(
    _In_ HANDLE ThreadHandle,
    _Out_ PWOW64_CONTEXT Context
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadWow64Context,
        Context,
        sizeof(WOW64_CONTEXT),
        NULL
        );
}

#if defined(_ARM64_)
FORCEINLINE
NTSTATUS
PhGetThreadArm32Context(
    _In_ HANDLE ThreadHandle,
    _Out_ PARM_NT_CONTEXT Context
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadWow64Context,
        Context,
        sizeof(ARM_NT_CONTEXT),
        NULL
        );
}
#endif

FORCEINLINE
NTSTATUS
PhGetThreadBreakOnTermination(
    _In_ HANDLE ThreadHandle,
    _Out_ PBOOLEAN BreakOnTermination
    )
{
    NTSTATUS status;
    ULONG breakOnTermination;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadBreakOnTermination,
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
PhSetThreadBreakOnTermination(
    _In_ HANDLE ThreadHandle,
    _In_ BOOLEAN BreakOnTermination
    )
{
    ULONG breakOnTermination;

    breakOnTermination = BreakOnTermination ? 1 : 0;

    return NtSetInformationThread(
        ThreadHandle,
        ThreadBreakOnTermination,
        &breakOnTermination,
        sizeof(ULONG)
        );
}

FORCEINLINE
NTSTATUS
PhGetThreadIsIoPending(
    _In_ HANDLE ThreadHandle,
    _Out_ PBOOLEAN IsIoPending
    )
{
    NTSTATUS status;
    ULONG isIoPending;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadIsIoPending,
        &isIoPending,
        sizeof(ULONG),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *IsIoPending = !!isIoPending;
    }

    return status;
}

/**
 * Gets time information for a thread.
 *
 * \param ThreadHandle A handle to a thread. The handle must have
 * THREAD_QUERY_LIMITED_INFORMATION access.
 * \param Times A variable which receives the information.
 */
FORCEINLINE
NTSTATUS
PhGetThreadTimes(
    _In_ HANDLE ThreadHandle,
    _Out_ PKERNEL_USER_TIMES Times
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadTimes,
        Times,
        sizeof(KERNEL_USER_TIMES),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetThreadIsTerminated(
    _In_ HANDLE ThreadHandle,
    _Out_ PBOOLEAN IsTerminated
    )
{
    NTSTATUS status;
    ULONG threadIsTerminated;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadIsTerminated,
        &threadIsTerminated,
        sizeof(ULONG),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *IsTerminated = !!threadIsTerminated;
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetThreadAffinityMask(
    _In_ HANDLE ThreadHandle,
    _Out_ PKAFFINITY AffinityMask
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;

    status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo);

    if (NT_SUCCESS(status))
    {
        *AffinityMask = basicInfo.AffinityMask;
    }

    return status;

    //return NtQueryInformationThread(
    //    ThreadHandle,
    //    ThreadAffinityMask,
    //    &AffinityMask,
    //    sizeof(KAFFINITY),
    //    NULL
    //    );
}

FORCEINLINE
NTSTATUS
PhGetThreadGroupAffinity(
    _In_ HANDLE ThreadHandle,
    _Out_ PGROUP_AFFINITY GroupAffinity
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadGroupInformation,
        GroupAffinity,
        sizeof(GROUP_AFFINITY),
        NULL
        );
}

/**
 * Gets a token's session ID.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param SessionId A variable which receives the session ID.
 */
FORCEINLINE
NTSTATUS
PhGetTokenSessionId(
    _In_ HANDLE TokenHandle,
    _Out_ PULONG SessionId
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenSessionId,
        SessionId,
        sizeof(ULONG),
        &returnLength
        );
}

/**
 * Gets a token's elevation type.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param ElevationType A variable which receives the elevation type.
 */
FORCEINLINE
NTSTATUS
PhGetTokenElevationType(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_ELEVATION_TYPE ElevationType
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenElevationType,
        ElevationType,
        sizeof(TOKEN_ELEVATION_TYPE),
        &returnLength
        );
}

/**
 * Gets whether a token is elevated.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Elevated A variable which receives a boolean indicating whether the token is elevated.
 */
FORCEINLINE
NTSTATUS
PhGetTokenIsElevated(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN Elevated
    )
{
    NTSTATUS status;
    TOKEN_ELEVATION elevation;
    ULONG returnLength;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenElevation,
        &elevation,
        sizeof(TOKEN_ELEVATION),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *Elevated = !!elevation.TokenIsElevated;
    }

    return status;
}

/**
 * Gets a token's statistics.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Statistics A variable which receives the token's statistics.
 */
FORCEINLINE
NTSTATUS
PhGetTokenStatistics(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_STATISTICS Statistics
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenStatistics,
        Statistics,
        sizeof(TOKEN_STATISTICS),
        &returnLength
        );
}

/**
 * Gets a token's source.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY_SOURCE access.
 * \param Source A variable which receives the token's source.
 */
FORCEINLINE
NTSTATUS
PhGetTokenSource(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_SOURCE Source
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenSource,
        Source,
        sizeof(TOKEN_SOURCE),
        &returnLength
        );
}

/**
 * Gets a handle to a token's linked token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param LinkedTokenHandle A variable which receives a handle to the linked token. You must close
 * the handle using NtClose() when you no longer need it.
 */
FORCEINLINE
NTSTATUS
PhGetTokenLinkedToken(
    _In_ HANDLE TokenHandle,
    _Out_ PHANDLE LinkedTokenHandle
    )
{
    NTSTATUS status;
    ULONG returnLength;
    TOKEN_LINKED_TOKEN linkedToken;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenLinkedToken,
        &linkedToken,
        sizeof(TOKEN_LINKED_TOKEN),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *LinkedTokenHandle = linkedToken.LinkedToken;
    }

    return status;
}

FORCEINLINE
NTSTATUS
PhGetTokenIsRestricted(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsRestricted
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG restricted;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenIsRestricted,
        &restricted,
        sizeof(ULONG),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *IsRestricted = !!restricted;
    }

    return status;
}

/**
 * Gets whether virtualization is allowed for a token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param IsVirtualizationAllowed A variable which receives a boolean indicating whether
 * virtualization is allowed for the token.
 */
FORCEINLINE
NTSTATUS
PhGetTokenIsVirtualizationAllowed(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsVirtualizationAllowed
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG virtualizationAllowed;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenVirtualizationAllowed,
        &virtualizationAllowed,
        sizeof(ULONG),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *IsVirtualizationAllowed = !!virtualizationAllowed;
    }

    return status;
}

/**
 * Gets whether virtualization is enabled for a token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param IsVirtualizationEnabled A variable which receives a boolean indicating whether
 * virtualization is enabled for the token.
 */
FORCEINLINE
NTSTATUS
PhGetTokenIsVirtualizationEnabled(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsVirtualizationEnabled
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG virtualizationEnabled;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenVirtualizationEnabled,
        &virtualizationEnabled,
        sizeof(ULONG),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    *IsVirtualizationEnabled = !!virtualizationEnabled;

    return status;
}

/**
* Gets UIAccess flag for a token.
*
* \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
* \param IsUIAccessEnabled A variable which receives a boolean indicating whether
* UIAccess is enabled for the token.
*/
FORCEINLINE
NTSTATUS
PhGetTokenIsUIAccessEnabled(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsUIAccessEnabled
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG uiAccess;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenUIAccess,
        &uiAccess,
        sizeof(ULONG),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    *IsUIAccessEnabled = !!uiAccess;

    return status;
}

/**
* Sets UIAccess flag for a token.
*
* \param TokenHandle A handle to a token. The handle must have TOKEN_ADJUST_DEFAULT access.
* \param IsUIAccessEnabled The new flag state.
* \remarks Enabling UIAccess requires SeTcbPrivilege.
*/
FORCEINLINE
NTSTATUS
PhSetTokenUIAccessEnabled(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN IsUIAccessEnabled
    )
{
    ULONG uiAccess;

    uiAccess = IsUIAccessEnabled ? 1 : 0;

    return NtSetInformationToken(
        TokenHandle,
        TokenUIAccess,
        &uiAccess,
        sizeof(ULONG)
        );
}

/**
* Gets SandBoxInert flag for a token.
*
* \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
* \param IsSandBoxInert A variable which receives a boolean indicating whether
* AppLocker rules or Software Restriction Policies are enabled for the token.
*/
FORCEINLINE
NTSTATUS
PhGetTokenIsSandBoxInert(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsSandBoxInert
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG sandBoxInert;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenSandBoxInert,
        &sandBoxInert,
        sizeof(ULONG),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    *IsSandBoxInert = !!sandBoxInert;

    return status;
}

/**
* Gets Mandatory Policy for a token.
*
* \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
* \param MandatoryPolicy A variable which receives a set of mandatory integrity
* policies enforced for the token.
*/
FORCEINLINE
NTSTATUS
PhGetTokenMandatoryPolicy(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_MANDATORY_POLICY MandatoryPolicy
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenMandatoryPolicy,
        MandatoryPolicy,
        sizeof(TOKEN_MANDATORY_POLICY),
        &returnLength
        );
}

FORCEINLINE
NTSTATUS
PhGetTokenOrigin(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_ORIGIN Origin
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenOrigin,
        Origin,
        sizeof(TOKEN_ORIGIN),
        &returnLength
        );
}

FORCEINLINE
NTSTATUS
PhGetTokenIsAppContainer(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsAppContainer
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG isAppContainer;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenIsAppContainer,
        &isAppContainer,
        sizeof(ULONG),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    *IsAppContainer = !!isAppContainer;

    return status;
}

FORCEINLINE
NTSTATUS
PhGetTokenAppContainerNumber(
    _In_ HANDLE TokenHandle,
    _Out_ PULONG AppContainerNumber
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenAppContainerNumber,
        AppContainerNumber,
        sizeof(ULONG),
        &returnLength
        );
}

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
        *IsCetEnabled = !!policyInfo.UserShadowStackPolicy.EnableUserShadowStack;
        *IsCetStrictModeEnabled = !!policyInfo.UserShadowStackPolicy.EnableUserShadowStackStrictMode;
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

#endif
