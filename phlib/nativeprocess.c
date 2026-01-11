/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <kphuser.h>

/**
 * Opens a process.
 *
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param DesiredAccess The desired access to the process.
 * \param ProcessId The ID of the process.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    CLIENT_ID clientId;
    KPH_LEVEL level;

    clientId.UniqueProcess = ProcessId;
    clientId.UniqueThread = NULL;

    level = KsiLevel();

    if ((level >= KphLevelMed) && (DesiredAccess & KPH_PROCESS_READ_ACCESS) == DesiredAccess)
    {
        status = KphOpenProcess(
            ProcessHandle,
            DesiredAccess,
            &clientId
            );
    }
    else
    {
        InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
        status = NtOpenProcess(
            ProcessHandle,
            DesiredAccess,
            &objectAttributes,
            &clientId
            );

        if (status == STATUS_ACCESS_DENIED && (level == KphLevelMax))
        {
            status = KphOpenProcess(
                ProcessHandle,
                DesiredAccess,
                &clientId
                );
        }
    }

    return status;
}

/**
 * Opens a process handle using a CLIENT_ID structure.
 *
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param DesiredAccess The desired access rights for the process handle.
 * \param ClientId A pointer to a CLIENT_ID structure that specifies the process and (optionally) thread.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenProcessClientId(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    KPH_LEVEL level;

    level = KsiLevel();

    if ((level >= KphLevelMed) && (DesiredAccess & KPH_PROCESS_READ_ACCESS) == DesiredAccess)
    {
        status = KphOpenProcess(
            ProcessHandle,
            DesiredAccess,
            ClientId
            );
    }
    else
    {
        InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
        status = NtOpenProcess(
            ProcessHandle,
            DesiredAccess,
            &objectAttributes,
            ClientId
            );

        if (status == STATUS_ACCESS_DENIED && (level == KphLevelMax))
        {
            status = KphOpenProcess(
                ProcessHandle,
                DesiredAccess,
                ClientId
                );
        }
    }

    return status;
}

/** Limited API for untrusted/external code. */
/**
 * Opens a process with limited access for untrusted or external code.
 *
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param DesiredAccess The desired access rights for the process handle.
 * \param ProcessId The unique identifier (PID) of the process to open.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenProcessPublic(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId
    )
{
    OBJECT_ATTRIBUTES objectAttributes;
    CLIENT_ID clientId;

    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
    clientId.UniqueProcess = ProcessId;
    clientId.UniqueThread = NULL;

    return NtOpenProcess(
        ProcessHandle,
        DesiredAccess,
        &objectAttributes,
        &clientId
        );
}

/**
 * Terminates a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_TERMINATE access.
 * \param ExitStatus A status value that indicates why the process is being terminated.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    )
{
    NTSTATUS status;

    if (KsiLevel() == KphLevelMax)
    {
        status = KphTerminateProcess(
            ProcessHandle,
            ExitStatus
            );

        if (NT_SUCCESS(status))
            return status;
    }

    status = NtTerminateProcess(
        ProcessHandle,
        ExitStatus
        );

    return status;
}

/**
 * Suspends the specified process.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_SUSPEND_RESUME access.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSuspendProcess(
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status;

    status = NtSuspendProcess(
        ProcessHandle
        );

    return status;
}

/**
 * Resumes the specified process.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_SUSPEND_RESUME access.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhResumeProcess(
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status;

    status = NtResumeProcess(
        ProcessHandle
        );

    return status;
}

/**
 * Gets basic information for a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param BasicInformation A variable which receives the information.
 * \return Successful or errant status.
 */
NTSTATUS PhGetProcessBasicInformation(
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
NTSTATUS PhGetProcessExtendedBasicInformation(
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
NTSTATUS PhGetProcessTimes(
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
NTSTATUS PhGetProcessSessionId(
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
NTSTATUS PhGetProcessIsWow64(
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
NTSTATUS PhGetProcessPeb32(
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

NTSTATUS PhGetProcessPeb(
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
NTSTATUS PhGetProcessDebugObject(
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

NTSTATUS PhGetProcessEnergyValues(
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

NTSTATUS PhGetProcessErrorMode(
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
NTSTATUS PhSetProcessErrorMode(
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
NTSTATUS PhGetProcessExecuteFlags(
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
NTSTATUS PhGetProcessIoPriority(
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
NTSTATUS PhGetProcessPagePriority(
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

NTSTATUS PhGetProcessPriorityBoost(
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
NTSTATUS PhGetProcessCycleTime(
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

NTSTATUS PhGetProcessUptime(
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

NTSTATUS PhGetProcessConsoleHostProcessId(
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

NTSTATUS PhGetProcessConsoleHostProcess(
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

NTSTATUS PhGetProcessProtection(
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

NTSTATUS PhGetProcessAffinityMask(
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

NTSTATUS PhGetProcessGroupInformation(
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

NTSTATUS PhGetProcessGroupAffinity(
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

NTSTATUS PhGetProcessIsCFGuardEnabled(
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

NTSTATUS PhGetProcessIsXFGuardEnabled(
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

NTSTATUS PhGetProcessHandleCount(
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

NTSTATUS PhGetProcessBreakOnTermination(
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

NTSTATUS PhSetProcessBreakOnTermination(
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

NTSTATUS PhGetProcessAppMemoryInformation(
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

NTSTATUS PhGetProcessMitigationPolicy(
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

NTSTATUS PhGetProcessNetworkIoCounters(
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
 * Queries variable-sized information for a process. The function allocates a buffer to contain the
 * information.
 *
 * \param ProcessHandle A handle to a process. The access required depends on the information class
 * specified.
 * \param ProcessInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer containing the information. You
 * must free the buffer using PhFree() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhpQueryProcessVariableSize(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG returnLength = 0;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessInformationClass,
        NULL,
        0,
        &returnLength
        );

    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL && status != STATUS_INFO_LENGTH_MISMATCH)
    {
        *Buffer = NULL;
        return status;
    }

    buffer = PhAllocate(returnLength);
    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessInformationClass,
        buffer,
        returnLength,
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
        *Buffer = NULL;
    }

    return status;
}

/**
 * Gets the cookie of the process.
 * A ProcessCookie is a per-process random value generated by the Windows kernel.
 * Its main purpose is to help protect against certain types of security attacks,
 * such as handle prediction and spoofing. By associating a unique, unpredictable
 * cookie with each process, the system can use it as part of internal validation
 * checks-making it harder for malicious code to guess or forge process information.
 * \param ProcessHandle Handle to the process for which the cookie is retrieved.
 * \param ProcessModifiedCookie Pointer to a ULONG that receives the modified process cookie.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessModifiedCookie(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ProcessModifiedCookie
    )
{
    NTSTATUS status;
    ULONG processCookie;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessCookie,
        &processCookie,
        sizeof(ULONG),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ProcessModifiedCookie = (0x7FFFFFED * processCookie + 0x7FFFFFC3) % 0x7FFFFFFF;
    }

    return status;
}

/**
 * Gets the file name of the process' image.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param FileName A variable which receives a pointer to a string containing the file name. You
 * must free the string using PhDereferenceObject() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessImageFileName(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    PUNICODE_STRING fileName;
    ULONG bufferLength;
    ULONG returnLength = 0;

    bufferLength = sizeof(UNICODE_STRING) + DOS_MAX_PATH_LENGTH * sizeof(WCHAR);
    fileName = PhAllocateStack(bufferLength);
    if (!fileName) return STATUS_NO_MEMORY;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessImageFileName,
        fileName,
        bufferLength,
        &returnLength
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFreeStack(fileName);
        bufferLength = returnLength;
        fileName = PhAllocateStack(bufferLength);
        if (!fileName) return STATUS_NO_MEMORY;

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessImageFileName,
            fileName,
            bufferLength,
            &returnLength
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFreeStack(fileName);
        return status;
    }

    // Note: Some minimal/pico processes have UNICODE_NULL as their filename. (dmex)
    if (RtlIsNullOrEmptyUnicodeString(fileName))
    {
        PhFreeStack(fileName);
        return STATUS_UNSUCCESSFUL;
    }

    *FileName = PhCreateStringFromUnicodeString(fileName);
    PhFreeStack(fileName);

    return status;
}

/**
 * Gets the Win32 file name of the process' image.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param FileName A variable which receives a pointer to a string containing the file name. You
 * must free the string using PhDereferenceObject() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function is only available on Windows Vista and above.
 */
NTSTATUS PhGetProcessImageFileNameWin32(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    PUNICODE_STRING fileName;
    ULONG bufferLength;
    ULONG returnLength = 0;

    bufferLength = sizeof(UNICODE_STRING) + DOS_MAX_PATH_LENGTH * sizeof(WCHAR);
    fileName = PhAllocateStack(bufferLength);
    if (!fileName) return STATUS_NO_MEMORY;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessImageFileNameWin32,
        fileName,
        bufferLength,
        &returnLength
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFreeStack(fileName);
        bufferLength = returnLength;
        fileName = PhAllocateStack(bufferLength);
        if (!fileName) return STATUS_NO_MEMORY;

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessImageFileNameWin32,
            fileName,
            bufferLength,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        PPH_STRING fileNameWin32;

        // Note: Some minimal/pico processes have UNICODE_NULL as their filename. (dmex)
        if (RtlIsNullOrEmptyUnicodeString(fileName))
        {
            PhFreeStack(fileName);
            return STATUS_UNSUCCESSFUL;
        }

        fileNameWin32 = PhCreateStringFromUnicodeString(fileName);

        // Note: ProcessImageFileNameWin32 returns the NT device path
        // instead of the Win32 path in some cases were drivers haven't
        // registered with the volume manager or have ignored the mount
        // manager (e.g. ImDisk). We workaround these issues by calling
        // PhGetFileName and resolving the NT device prefix. (dmex)

        if (fileNameWin32->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
        {
            PhMoveReference(&fileNameWin32, PhGetFileName(fileNameWin32));
        }

        *FileName = fileNameWin32;
    }

    PhFreeStack(fileName);

    return status;
}

/**
 * Gets the file name of the process' image by a PID.
 *
 * \param ProcessId A unique identifier of the process.
 * \param FullFileName A variable which receives a pointer to a string containing the full name
 * of the file. You must free the string using PhDereferenceObject() when you no longer need it.
 * \param FileName A variable which receives a pointer to a string containing the file name without
 * the path. You must free the string using PhDereferenceObject() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessImageFileNameById(
    _In_ HANDLE ProcessId,
    _Out_opt_ PPH_STRING *FullFileName,
    _Out_opt_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    PVOID buffer;
    USHORT bufferSize;
    SYSTEM_PROCESS_ID_INFORMATION processIdInfo;

    if (!FullFileName && !FileName)
        return STATUS_INVALID_PARAMETER_MIX;

    bufferSize = 0x200;
    buffer = PhAllocateStack(bufferSize);
    if (!buffer) return STATUS_NO_MEMORY;

    processIdInfo.ProcessId = ProcessId;
    processIdInfo.ImageName.Length = 0;
    processIdInfo.ImageName.MaximumLength = bufferSize;
    processIdInfo.ImageName.Buffer = buffer;

    status = NtQuerySystemInformation(
        SystemProcessIdInformation,
        &processIdInfo,
        sizeof(SYSTEM_PROCESS_ID_INFORMATION),
        NULL
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        // Required length is stored in MaximumLength.

        PhFreeStack(buffer);
        buffer = PhAllocateStack(processIdInfo.ImageName.MaximumLength);
        if (!buffer) return STATUS_NO_MEMORY;
        processIdInfo.ImageName.Buffer = buffer;

        status = NtQuerySystemInformation(
            SystemProcessIdInformation,
            &processIdInfo,
            sizeof(SYSTEM_PROCESS_ID_INFORMATION),
            NULL
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFreeStack(buffer);
        return status;
    }

    if (FullFileName)
    {
        *FullFileName = PhCreateStringFromUnicodeString(&processIdInfo.ImageName);
    }

    if (FileName)
    {
        PH_STRINGREF stringRef;
        ULONG_PTR index;

        stringRef.Length = processIdInfo.ImageName.Length;
        stringRef.Buffer = processIdInfo.ImageName.Buffer;

        // Find where the name starts
        index = PhFindLastCharInStringRef(&stringRef, L'\\', FALSE);

        if (index == SIZE_MAX)
            *FileName = PhCreateStringFromUnicodeString(&processIdInfo.ImageName);
        else
        {
            // Reference the tail only
            stringRef.Buffer = PTR_ADD_OFFSET(stringRef.Buffer, index * sizeof(WCHAR) + sizeof(UNICODE_NULL));
            stringRef.Length = stringRef.Length - index * sizeof(WCHAR) - sizeof(UNICODE_NULL);

            *FileName = PhCreateString2(&stringRef);
        }
    }

    PhFreeStack(buffer);

    return status;
}

/**
 * Gets the file name of a process' image.
 *
 * \param ProcessId The ID of the process.
 * \param FileName A variable which receives a pointer to a string containing the file name. You
 * must free the string using PhDereferenceObject() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function only works on Windows Vista and above. There does not appear to be any
 * access checking performed by the kernel for this.
 */
NTSTATUS PhGetProcessImageFileNameByProcessId(
    _In_opt_ HANDLE ProcessId,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    PVOID buffer;
    USHORT bufferSize;
    SYSTEM_PROCESS_ID_INFORMATION processIdInfo;

    bufferSize = 0x200;
    buffer = PhAllocateStack(bufferSize);
    if (!buffer) return STATUS_NO_MEMORY;

    processIdInfo.ProcessId = ProcessId;
    processIdInfo.ImageName.Length = 0;
    processIdInfo.ImageName.MaximumLength = bufferSize;
    processIdInfo.ImageName.Buffer = buffer;

    status = NtQuerySystemInformation(
        SystemProcessIdInformation,
        &processIdInfo,
        sizeof(SYSTEM_PROCESS_ID_INFORMATION),
        NULL
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        // Required length is stored in MaximumLength.

        PhFreeStack(buffer);
        buffer = PhAllocateStack(processIdInfo.ImageName.MaximumLength);
        if (!buffer) return STATUS_NO_MEMORY;
        processIdInfo.ImageName.Buffer = buffer;

        status = NtQuerySystemInformation(
            SystemProcessIdInformation,
            &processIdInfo,
            sizeof(SYSTEM_PROCESS_ID_INFORMATION),
            NULL
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFreeStack(buffer);
        return status;
    }

    // Note: Some minimal/pico processes have UNICODE_NULL as their filename. (dmex)
    if (RtlIsNullOrEmptyUnicodeString(&processIdInfo.ImageName))
    {
        PhFreeStack(buffer);
        return STATUS_UNSUCCESSFUL;
    }

    *FileName = PhCreateStringFromUnicodeString(&processIdInfo.ImageName);
    PhFreeStack(buffer);

    return status;
}

/**
 * Gets whether a process is being debugged.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION access.
 * \param IsBeingDebugged A variable which receives a boolean indicating whether the process is being debugged.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessIsBeingDebugged(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsBeingDebugged
    )
{
    NTSTATUS status;
    PVOID debugHandle;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessDebugPort,
        &debugHandle,
        sizeof(PVOID),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *IsBeingDebugged = !!debugHandle;
        return status;
    }

    if (KsiLevel() >= KphLevelLow)
    {
        KPH_PROCESS_STATE processState;

        status = KphQueryInformationProcess(
            ProcessHandle,
            KphProcessStateInformation,
            &processState,
            sizeof(processState),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            *IsBeingDebugged = !BooleanFlagOn(processState, KPH_PROCESS_NOT_BEING_DEBUGGED);
        }
    }

    return status;
}

/**
 * Determines if a process has started termination.
 *
 * \param[in] ProcessHandle A handle to the process whose termination state is to be retrieved.
 * \param[out] IsTerminated A pointer to a variable that receives the termination state (TRUE if terminated).
 * \return Successful or errant status.
 * \remarks The handle must have PROCESS_QUERY_LIMITED_INFORMATION access.
 */
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsTerminated
    )
{
    NTSTATUS status;
    PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

    status = PhGetProcessExtendedBasicInformation(ProcessHandle, &basicInfo);

    if (NT_SUCCESS(status))
    {
        *IsTerminated = !!basicInfo.IsProcessDeleting;
    }

    return status;
}

/**
 * Determines if a process has terminated by waiting with zero timeout.
 *
 * \param[in] ProcessHandle A handle to the process.
 * \return TRUE if the process is terminated, FALSE otherwise.
 * \remarks The handle must have SYNCRONIZE access.
 */
NTSTATUS PhGetProcessIsTerminated(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsTerminated
    )
{
    NTSTATUS status;
    LARGE_INTEGER timeout;

    memset(&timeout, 0, sizeof(LARGE_INTEGER));

    status = NtWaitForSingleObject(
        ProcessHandle,
        FALSE,
        &timeout
        );

    *IsTerminated = status == STATUS_WAIT_0;

    return status;
}

/**
 * Retrieves the device map for a specified process.
 *
 * \param ProcessHandle Handle to the process whose device map is to be queried.
 * \param DeviceMap Pointer to a ULONG that receives the device map value.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessDeviceMap(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG DeviceMap
    )
{
    NTSTATUS status;
#ifndef _WIN64
    PROCESS_DEVICEMAP_INFORMATION deviceMapInfo;
#else
    PROCESS_DEVICEMAP_INFORMATION_EX deviceMapInfo;
#endif
    memset(&deviceMapInfo, 0, sizeof(deviceMapInfo));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessDeviceMap,
        &deviceMapInfo,
        sizeof(deviceMapInfo),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (DeviceMap)
        {
            *DeviceMap = deviceMapInfo.Query.DriveMap;
        }
    }

    return status;
}

/**
 * Gets a string stored in a process' parameters structure.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param Offset The string to retrieve.
 * \param String A variable which receives a pointer to the requested string. You must free the
 * string using PhDereferenceObject() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_INVALID_PARAMETER_2 An invalid value was specified in the Offset parameter.
 */
NTSTATUS PhGetProcessPebString(
    _In_ HANDLE ProcessHandle,
    _In_ PH_PEB_OFFSET Offset,
    _Out_ PPH_STRING *String
    )
{
    NTSTATUS status;
    PPH_STRING string;
    ULONG offset;

#define PEB_OFFSET_CASE(Enum, Field) \
    case Enum: offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Field); break; \
    case Enum | PhpoWow64: offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS32, Field); break

    switch (Offset)
    {
        PEB_OFFSET_CASE(PhpoCurrentDirectory, CurrentDirectory);
        PEB_OFFSET_CASE(PhpoDllPath, DllPath);
        PEB_OFFSET_CASE(PhpoImagePathName, ImagePathName);
        PEB_OFFSET_CASE(PhpoCommandLine, CommandLine);
        PEB_OFFSET_CASE(PhpoWindowTitle, WindowTitle);
        PEB_OFFSET_CASE(PhpoDesktopInfo, DesktopInfo);
        PEB_OFFSET_CASE(PhpoShellInfo, ShellInfo);
        PEB_OFFSET_CASE(PhpoRuntimeData, RuntimeData);
    default:
        return STATUS_INVALID_PARAMETER_2;
    }

    if (!(Offset & PhpoWow64))
    {
        PVOID pebBaseAddress;
        PVOID processParameters;
        UNICODE_STRING unicodeString;

        // Get the PEB address.
        if (!NT_SUCCESS(status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress)))
            return status;

        // Read the address of the process parameters.
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
            &processParameters,
            sizeof(PVOID),
            NULL
            )))
            return status;

        // Read the string structure.
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters, offset),
            &unicodeString,
            sizeof(UNICODE_STRING),
            NULL
            )))
            return status;

        if (RtlIsNullOrEmptyUnicodeString(&unicodeString))
        {
            *String = PhReferenceEmptyString();
            return status;
        }

        string = PhCreateStringEx(NULL, unicodeString.Length);

        // Read the string contents.
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            unicodeString.Buffer,
            string->Buffer,
            string->Length,
            NULL
            )))
        {
            PhDereferenceObject(string);
            return status;
        }
    }
    else
    {
        PVOID pebBaseAddress32;
        ULONG processParameters32;
        UNICODE_STRING32 unicodeString32;

        if (!NT_SUCCESS(status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress32)))
            return status;

        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress32, FIELD_OFFSET(PEB32, ProcessParameters)),
            &processParameters32,
            sizeof(ULONG),
            NULL
            )))
            return status;

        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters32, offset),
            &unicodeString32,
            sizeof(UNICODE_STRING32),
            NULL
            )))
            return status;

        if (unicodeString32.Length == 0)
        {
            *String = PhReferenceEmptyString();
            return status;
        }

        string = PhCreateStringEx(NULL, unicodeString32.Length);

        // Read the string contents.
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            UlongToPtr(unicodeString32.Buffer),
            string->Buffer,
            string->Length,
            NULL
            )))
        {
            PhDereferenceObject(string);
            return status;
        }
    }

    *String = string;

    return status;
}

/**
 * Gets a process' command line.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION. Before Windows 8.1, the handle must also have PROCESS_VM_READ
 * access.
 * \param CommandLine A variable which receives a pointer to a string containing the command line. You
 * must free the string using PhDereferenceObject() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *CommandLine
    )
{
    if (WindowsVersion >= WINDOWS_8_1)
    {
        NTSTATUS status;
        PUNICODE_STRING buffer;
        ULONG bufferLength;
        ULONG returnLength = 0;

        bufferLength = sizeof(UNICODE_STRING) + DOS_MAX_PATH_LENGTH * sizeof(WCHAR);
        buffer = PhAllocateStack(bufferLength);
        if (!buffer) return STATUS_NO_MEMORY;

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessCommandLineInformation,
            buffer,
            bufferLength,
            &returnLength
            );

        if (status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFreeStack(buffer);
            bufferLength = returnLength;
            buffer = PhAllocateStack(bufferLength);
            if (!buffer) return STATUS_NO_MEMORY;

            status = NtQueryInformationProcess(
                ProcessHandle,
                ProcessCommandLineInformation,
                buffer,
                bufferLength,
                &returnLength
                );
        }

        if (NT_SUCCESS(status))
        {
            *CommandLine = PhCreateStringFromUnicodeString(buffer);
        }

        PhFreeStack(buffer);

        return status;
    }

    return PhGetProcessPebString(ProcessHandle, PhpoCommandLine, CommandLine);
}

/**
 * Retrieves the command line string reference for the current process.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessCommandLineStringRef(
    _Out_ PPH_STRINGREF CommandLine
    )
{
    PhUnicodeStringToStringRef(&NtCurrentPeb()->ProcessParameters->CommandLine, CommandLine);
    return STATUS_SUCCESS;
}

/**
 * Retrieves the current directory of a specified process.
 *
 * \param ProcessHandle A handle to the process. The handle must have PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param IsWow64 Specifies whether to retrieve the current directory from the WOW64 PEB (TRUE) or the native PEB (FALSE).
 * \param CurrentDirectory A variable which receives a pointer to a string containing the current directory.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessCurrentDirectory(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN IsWow64,
    _Out_ PPH_STRING* CurrentDirectory
    )
{
    return PhGetProcessPebString(ProcessHandle, PhpoCurrentDirectory | (IsWow64 ? PhpoWow64 : PhpoNone), CurrentDirectory);
}

/**
 * Retrieves the desktop information string for a specified process.
 *
 * \param ProcessHandle A handle to the process. The handle must have PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param DesktopInfo A variable which receives a pointer to a string containing the desktop information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessDesktopInfo(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING* DesktopInfo
    )
{
    return PhGetProcessPebString(ProcessHandle, PhpoDesktopInfo, DesktopInfo);
}

/**
 * Gets the window flags and window title of a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION. Before Windows 7 SP1, the handle must also have
 * PROCESS_VM_READ access.
 * \param WindowFlags A variable which receives the window flags.
 * \param WindowTitle A variable which receives a pointer to the window title. You must free the
 * string using PhDereferenceObject() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessWindowTitle(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG WindowFlags,
    _Out_ PPH_STRING *WindowTitle
    )
{
    NTSTATUS status;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif
    ULONG windowFlags;
    PPROCESS_WINDOW_INFORMATION windowInfo;
    ULONG bufferLength;
    ULONG returnLength = 0;

    bufferLength = UFIELD_OFFSET(PROCESS_WINDOW_INFORMATION, WindowTitle[DOS_MAX_PATH_LENGTH]) + sizeof(UNICODE_NULL);
    windowInfo = PhAllocate(bufferLength);

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessWindowInformation,
        windowInfo,
        bufferLength,
        &returnLength
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(windowInfo);
        bufferLength = returnLength;
        windowInfo = PhAllocate(bufferLength);

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessWindowInformation,
            windowInfo,
            bufferLength,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        *WindowFlags = windowInfo->WindowFlags;
        *WindowTitle = PhCreateStringEx(windowInfo->WindowTitle, windowInfo->WindowTitleLength);
        PhFree(windowInfo);

        return status;
    }

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (!isWow64)
#endif
    {
        PVOID pebBaseAddress;
        PVOID processParameters;

        // Get the PEB address.
        if (!NT_SUCCESS(status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress)))
            return status;

        // Read the address of the process parameters.
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
            &processParameters,
            sizeof(PVOID),
            NULL
            )))
            return status;

        // Read the window flags.
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters, FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, WindowFlags)),
            &windowFlags,
            sizeof(ULONG),
            NULL
            )))
            return status;
    }
#ifdef _WIN64
    else
    {
        PVOID pebBaseAddress32;
        ULONG processParameters32;

        if (!NT_SUCCESS(status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress32)))
            return status;

        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress32, FIELD_OFFSET(PEB32, ProcessParameters)),
            &processParameters32,
            sizeof(ULONG),
            NULL
            )))
            return status;

        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters32, FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS32, WindowFlags)),
            &windowFlags,
            sizeof(ULONG),
            NULL
            )))
            return status;
    }
#endif

#ifdef _WIN64
    status = PhGetProcessPebString(ProcessHandle, PhpoWindowTitle | (isWow64 ? PhpoWow64 : PhpoNone), WindowTitle);
#else
    status = PhGetProcessPebString(ProcessHandle, PhpoWindowTitle, WindowTitle);
#endif

    if (NT_SUCCESS(status))
        *WindowFlags = windowFlags;

    return status;
}

/**
 * Retrieves the Data Execution Prevention (DEP) status for a specified process.
 *
 * \param ProcessHandle Handle to the process whose DEP status is to be queried.
 * \param DepStatus Pointer to a variable that receives the DEP status flags.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessDepStatus(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG DepStatus
    )
{
    NTSTATUS status;
    ULONG executeFlags;
    ULONG depStatus;

    if (!NT_SUCCESS(status = PhGetProcessExecuteFlags(
        ProcessHandle,
        &executeFlags
        )))
        return status;

    // Check if execution of data pages is enabled.
    // TODO: if we want real, effective DEP status here as computed by nt!MiCanGrantExecute (x64 oskernels):
    // canGrantExecEvenForNxPages = (EPROCESS.WoW64Process && EPROCESS.Machine == IMAGE_FILE_MACHINE_I386)
    //     && (KF_GLOBAL_32BIT_EXECUTE || MEM_EXECUTE_OPTION_ENABLE
    //         || (!KF_GLOBAL_32BIT_NOEXECUTE && !MEM_EXECUTE_OPTION_DISABLE));
    if (executeFlags & MEM_EXECUTE_OPTION_DISABLE)
        depStatus = PH_PROCESS_DEP_ENABLED;
    else
        depStatus = 0;

    if (executeFlags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
        depStatus |= PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED;
    if (executeFlags & MEM_EXECUTE_OPTION_PERMANENT)
        depStatus |= PH_PROCESS_DEP_PERMANENT;

    *DepStatus = depStatus;

    return status;
}

/**
 * Gets a process' environment block.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION and
 * PROCESS_VM_READ access.
 * \param IsWow64Process A variable which receives a boolean indicating whether the process is 32-bit.
 * \li \c PH_GET_PROCESS_ENVIRONMENT_WOW64 Retrieve the environment block from the WOW64 PEB.
 * \param Environment A variable which will receive a pointer to the environment block copied from
 * the process. You must free the block using PhFreePage() when you no longer need it.
 * \param EnvironmentLength A variable which will receive the length of the environment block, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessEnvironment(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN IsWow64Process,
    _Out_ PVOID *Environment,
    _Out_ PULONG EnvironmentLength
    )
{
    NTSTATUS status;
    PVOID environmentRemote;
    MEMORY_BASIC_INFORMATION mbi;
    PVOID environment;
    SIZE_T environmentLength;

    if (IsWow64Process)
    {
        PVOID pebBaseAddress32;
        ULONG processParameters32;
        ULONG environmentRemote32;

        status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress32);

        if (!NT_SUCCESS(status))
            return status;

        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress32, UFIELD_OFFSET(PEB32, ProcessParameters)),
            &processParameters32,
            sizeof(ULONG),
            NULL
            )))
            return status;

        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters32, UFIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS32, Environment)),
            &environmentRemote32,
            sizeof(ULONG),
            NULL
            )))
            return status;

        environmentRemote = UlongToPtr(environmentRemote32);
    }
    else
    {
        PVOID pebBaseAddress;
        PVOID processParameters;

        status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            return status;

        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, ProcessParameters)),
            &processParameters,
            sizeof(PVOID),
            NULL
            )))
            return status;

        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters, UFIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Environment)),
            &environmentRemote,
            sizeof(PVOID),
            NULL
            )))
            return status;
    }

    if (!NT_SUCCESS(status = NtQueryVirtualMemory(
        ProcessHandle,
        environmentRemote,
        MemoryBasicInformation,
        &mbi,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
        return status;

    // Read in the entire region of memory.

    environmentLength = (SIZE_T)PTR_SUB_OFFSET(mbi.RegionSize,
        PTR_SUB_OFFSET(environmentRemote, mbi.BaseAddress));

    environment = PhAllocatePage(environmentLength, NULL);

    if (!environment)
        return STATUS_NO_MEMORY;

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        environmentRemote,
        environment,
        environmentLength,
        NULL
        )))
    {
        PhFreePage(environment);
        return status;
    }

    *Environment = environment;

    if (EnvironmentLength)
        *EnvironmentLength = (ULONG)environmentLength;

    return status;
}

/**
 * Gets the file name of a mapped section.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION access.
 * \param BaseAddress The base address of the section view.
 * \param FileName A variable which receives a pointer to a string containing the file name of the
 * section. You must free the string using PhDereferenceObject() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessMappedFileName(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    SIZE_T bufferSize;
    SIZE_T returnLength;
    PUNICODE_STRING buffer;

    returnLength = 0;
    bufferSize = 0x200;
    buffer = PhAllocateStack(bufferSize);
    if (!buffer) return STATUS_NO_MEMORY;

    status = NtQueryVirtualMemory(
        ProcessHandle,
        BaseAddress,
        MemoryMappedFilenameInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    if (status == STATUS_BUFFER_OVERFLOW && returnLength > 0) // returnLength > 0 required for MemoryMappedFilename on Windows 7 SP1 (dmex)
    {
        PhFreeStack(buffer);
        bufferSize = returnLength;
        buffer = PhAllocateStack(bufferSize);
        if (!buffer) return STATUS_NO_MEMORY;

        status = NtQueryVirtualMemory(
            ProcessHandle,
            BaseAddress,
            MemoryMappedFilenameInformation,
            buffer,
            bufferSize,
            &returnLength
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFreeStack(buffer);
        return status;
    }

    *FileName = PhCreateStringFromUnicodeString(buffer);
    PhFreeStack(buffer);

    return status;
}

/**
 * Retrieves information about a mapped image in the virtual memory of a process.
 *
 * \param ProcessHandle Handle to the process whose memory is being queried.
 * \param BaseAddress Base address of the mapped image in the process's virtual memory.
 * \param ImageInformation Pointer to a MEMORY_IMAGE_INFORMATION structure that receives the image information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessMappedImageInformation(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_ PMEMORY_IMAGE_INFORMATION ImageInformation
    )
{
    NTSTATUS status;
    MEMORY_IMAGE_INFORMATION imageInformation;

    status = NtQueryVirtualMemory(
        ProcessHandle,
        BaseAddress,
        MemoryImageInformation,
        &imageInformation,
        sizeof(MEMORY_IMAGE_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ImageInformation = imageInformation;
    }

    return status;
}

/**
 * Retrieves the base address and optional size of a mapped image in a process, given an address within the image.
 *
 * This function queries the process for image mapping information at the specified address.
 * If the address is valid and corresponds to an executable image, the base address and size
 * (if requested) are returned. Otherwise, an unsuccessful status is returned.
 *
 * \param ProcessHandle Handle to the target process.
 * \param Address Address within the mapped image in the target process.
 * \param ImageBaseAddress Receives the base address of the mapped image.
 * \param SizeOfImage Optional pointer to receive the size of the mapped image.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessMappedImageBaseFromAddress(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID Address,
    _Out_ PVOID* ImageBaseAddress,
    _Out_opt_ PSIZE_T SizeOfImage
    )
{
    NTSTATUS status;
    MEMORY_IMAGE_INFORMATION imageInfo;

    status = PhGetProcessMappedImageInformation(
        ProcessHandle,
        Address,
        &imageInfo
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!imageInfo.ImageBase || imageInfo.ImageNotExecutable || imageInfo.ImagePartialMap)
        return STATUS_UNSUCCESSFUL;
    if (Address < imageInfo.ImageBase || (imageInfo.SizeOfImage && ((ULONG_PTR)Address >= (ULONG_PTR)imageInfo.ImageBase + imageInfo.SizeOfImage)))
        return STATUS_UNSUCCESSFUL;

    *ImageBaseAddress = imageInfo.ImageBase;

    if (SizeOfImage)
    {
        *SizeOfImage = imageInfo.SizeOfImage;
    }

    return STATUS_SUCCESS;
}

/**
 * Gets working set information for a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION access.
 * \param WorkingSetInformation A variable which receives a pointer to the information. You must
 * free the buffer using PhFree() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessWorkingSetInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PMEMORY_WORKING_SET_INFORMATION *WorkingSetInformation
    )
{
    NTSTATUS status;
    PMEMORY_WORKING_SET_INFORMATION buffer;
    SIZE_T bufferSize;
    ULONG attempts = 0;

    bufferSize = 0x8000;
    buffer = PhAllocate(bufferSize);

    status = NtQueryVirtualMemory(
        ProcessHandle,
        NULL,
        MemoryWorkingSetInformation,
        buffer,
        bufferSize,
        NULL
        );

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        bufferSize = UFIELD_OFFSET(MEMORY_WORKING_SET_INFORMATION, WorkingSetInfo[buffer->NumberOfEntries]);
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQueryVirtualMemory(
            ProcessHandle,
            NULL,
            MemoryWorkingSetInformation,
            buffer,
            bufferSize,
            NULL
            );

        attempts++;
    }

    if (!NT_SUCCESS(status))
    {
        // Fall back to using the previous code that we've used since Windows 7 (dmex)
        bufferSize = 0x8000;
        buffer = PhAllocate(bufferSize);

        while ((status = NtQueryVirtualMemory(
            ProcessHandle,
            NULL,
            MemoryWorkingSetInformation,
            buffer,
            bufferSize,
            NULL
            )) == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            bufferSize *= 2;

            // Fail if we're resizing the buffer to something very large.
            if (bufferSize > PH_LARGE_BUFFER_SIZE)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferSize);
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *WorkingSetInformation = (PMEMORY_WORKING_SET_INFORMATION)buffer;

    return status;
}

/**
 * Gets working set counters for a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION access.
 * \param WsCounters A variable which receives the counters.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessWsCounters(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_WS_COUNTERS WsCounters
    )
{
    NTSTATUS status;
    PMEMORY_WORKING_SET_INFORMATION wsInfo;
    PH_PROCESS_WS_COUNTERS wsCounters;
    ULONG_PTR i;

    if (!NT_SUCCESS(status = PhGetProcessWorkingSetInformation(
        ProcessHandle,
        &wsInfo
        )))
        return status;

    memset(&wsCounters, 0, sizeof(PH_PROCESS_WS_COUNTERS));

    for (i = 0; i < wsInfo->NumberOfEntries; i++)
    {
        wsCounters.NumberOfPages++;

        if (wsInfo->WorkingSetInfo[i].ShareCount > 1)
            wsCounters.NumberOfSharedPages++;
        if (wsInfo->WorkingSetInfo[i].ShareCount == 0)
            wsCounters.NumberOfPrivatePages++;
        if (wsInfo->WorkingSetInfo[i].Shared)
            wsCounters.NumberOfShareablePages++;
    }

    PhFree(wsInfo);

    *WsCounters = wsCounters;

    return status;
}

/**
 * Retrieves the mandatory integrity policy for a specified process.
 *
 * \param ProcessHandle A handle to the process whose mandatory policy is to be queried.
 * \param Mask A pointer to an ACCESS_MASK variable that receives the mandatory policy mask.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessMandatoryPolicy(
    _In_ HANDLE ProcessHandle,
    _Out_ PACCESS_MASK Mask
    )
{
    NTSTATUS status;
    BOOLEAN found = FALSE;
    ACCESS_MASK currentMask = 0;
    PSYSTEM_MANDATORY_LABEL_ACE currentAce;
    PSECURITY_DESCRIPTOR currentSecurityDescriptor;
    BOOLEAN currentSaclPresent;
    BOOLEAN currentSaclDefaulted;
    PACL currentSacl;

    status = PhGetObjectSecurity(
        ProcessHandle,
        LABEL_SECURITY_INFORMATION,
        &currentSecurityDescriptor
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetSaclSecurityDescriptor(
        currentSecurityDescriptor,
        &currentSaclPresent,
        &currentSacl,
        &currentSaclDefaulted
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!(currentSaclPresent && currentSacl))
        goto CleanupExit;

    for (USHORT i = 0; i < currentSacl->AceCount; i++)
    {
        status = PhGetAce(currentSacl, i, &currentAce);

        if (!NT_SUCCESS(status))
            break;

        if (currentAce->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
        {
            currentMask = currentAce->Mask;
            found = TRUE;
            break;
        }
    }

CleanupExit:
    PhFree(currentSecurityDescriptor);

    if (NT_SUCCESS(status))
    {
        if (found)
        {
            *Mask = currentMask;
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}

/**
 * Sets the mandatory label policy for a specified process.
 *
 * \param ProcessHandle Handle to the target process.
 * \param Mask Mandatory label policy mask to apply.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetProcessMandatoryPolicy(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK Mask
    )
{
    NTSTATUS status;
    PSYSTEM_MANDATORY_LABEL_ACE currentAce;
    PSECURITY_DESCRIPTOR currentSecurityDescriptor;
    BOOLEAN currentSaclPresent;
    BOOLEAN currentSaclDefaulted;
    PACL currentSacl;

    status = PhGetObjectSecurity(
        ProcessHandle,
        LABEL_SECURITY_INFORMATION,
        &currentSecurityDescriptor
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetSaclSecurityDescriptor(
        currentSecurityDescriptor,
        &currentSaclPresent,
        &currentSacl,
        &currentSaclDefaulted
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = STATUS_UNSUCCESSFUL;

    if (!(currentSaclPresent && currentSacl))
        goto CleanupExit;

    for (USHORT i = 0; i < currentSacl->AceCount; i++)
    {
        status = PhGetAce(currentSacl, i, &currentAce);

        if (!NT_SUCCESS(status))
            break;

        if (currentAce->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
        {
            currentAce->Mask = Mask;

            status = PhSetObjectSecurity(
                ProcessHandle,
                LABEL_SECURITY_INFORMATION,
                currentSecurityDescriptor
                );
            break;
        }
    }

CleanupExit:
    PhFree(currentSecurityDescriptor);

    return status;
}

/**
 * Retrieves the quota limits for a specified process.
 *
 * \param ProcessHandle Handle to the process whose quota limits are to be queried.
 * \param QuotaLimits Pointer to a QUOTA_LIMITS structure that receives the quota limits.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessQuotaLimits(
    _In_ HANDLE ProcessHandle,
    _Out_ PQUOTA_LIMITS QuotaLimits
    )
{
    NTSTATUS status;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessQuotaLimits,
        QuotaLimits,
        sizeof(QUOTA_LIMITS),
        NULL
        );

    // Not implemented (dmex)
    //if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    //{
    //    status = KphQueryInformationProcess(
    //        ProcessHandle,
    //        KphProcessQuotaLimits,
    //        QuotaLimits,
    //        sizeof(QUOTA_LIMITS),
    //        NULL
    //        );
    //}

    return status;
}

/**
 * Sets the quota limits for a specified process.
 *
 * \param ProcessHandle A handle to the process whose quota limits are to be set.
 * \param QuotaLimits The quota limits to apply to the process.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetProcessQuotaLimits(
    _In_ HANDLE ProcessHandle,
    _In_ QUOTA_LIMITS QuotaLimits
    )
{
    NTSTATUS status;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessQuotaLimits,
        &QuotaLimits,
        sizeof(QUOTA_LIMITS)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessQuotaLimits,
            &QuotaLimits,
            sizeof(QUOTA_LIMITS)
            );
    }

    return status;
}

/**
 * Attempts to empty the working set of the specified process.
 *
 * \param ProcessHandle A handle to the process whose working set should be emptied.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetProcessEmptyWorkingSet(
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status;
    QUOTA_LIMITS_EX quotaLimits;

    memset(&quotaLimits, 0, sizeof(QUOTA_LIMITS_EX));
    quotaLimits.MinimumWorkingSetSize = SIZE_MAX;
    quotaLimits.MaximumWorkingSetSize = SIZE_MAX;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessQuotaLimits,
        &quotaLimits,
        sizeof(QUOTA_LIMITS_EX)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessEmptyWorkingSet,
            &quotaLimits,
            sizeof(QUOTA_LIMITS_EX)
            );
    }

    return status;
}

/**
 * Empties the working set of the specified process.
 *
 * \param ProcessHandle A handle to the process whose working set should be emptied.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetProcessWorkingSetEmpty(
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status;
    PROCESS_WORKING_SET_CONTROL controlInfo;

    memset(&controlInfo, 0, sizeof(PROCESS_WORKING_SET_CONTROL));
    controlInfo.Version = PROCESS_WORKING_SET_CONTROL_VERSION;
    controlInfo.Operation = ProcessWorkingSetEmpty;
    controlInfo.Flags = PROCESS_WORKING_SET_FLAG_EMPTY_PAGES;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessWorkingSetControl,
        &controlInfo,
        sizeof(PROCESS_WORKING_SET_CONTROL)
        );

    return status;
}

/**
 * Sets the specified region of a process's working set to empty pages.
 *
 * \param ProcessHandle Handle to the target process.
 * \param BaseAddress Pointer to the base address of the region to modify.
 * \param Size Size, in bytes, of the region to modify.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetProcessEmptyPageWorkingSet(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size
    )
{
    NTSTATUS status;
    PVOID baseAddress;
    SIZE_T regionSize;

    baseAddress = BaseAddress;
    regionSize = Size;

    // Calling VirtualUnlock on a range of memory that is not locked
    // releases the pages from the process's working set. (MSDN)

    status = NtUnlockVirtualMemory(
        ProcessHandle,
        &baseAddress,
        &regionSize,
        MAP_PROCESS
        );

    // Note: STATUS_SUCCESS is a bad status in this case. (dmex)
    assert(status == STATUS_NOT_LOCKED);

    if (status == STATUS_NOT_LOCKED)
        status = STATUS_SUCCESS;

    return status;
}

/**
 * Retrieves the priority class of a specified process.
 *
 * \param ProcessHandle Handle to the process whose priority class is to be retrieved.
 * \param PriorityClass Pointer to a variable that receives the priority class value.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessPriorityClass(
    _In_ HANDLE ProcessHandle,
    _Out_ PUCHAR PriorityClass
    )
{
    NTSTATUS status;
    PROCESS_PRIORITY_CLASS processPriorityClass;

    memset(&processPriorityClass, 0, sizeof(PROCESS_PRIORITY_CLASS));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessPriorityClass,
        &processPriorityClass,
        sizeof(PROCESS_PRIORITY_CLASS),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *PriorityClass = processPriorityClass.PriorityClass;
    }

    return status;
}

/**
 * Sets the priority class of a process.
 *
 * \param ProcessHandle A handle to the process whose priority class is to be set.
 * \param PriorityClass The priority class value to assign to the process.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetProcessPriorityClass(
    _In_ HANDLE ProcessHandle,
    _In_ UCHAR PriorityClass
    )
{
    NTSTATUS status;

    if (WindowsVersion >= WINDOWS_11_22H2)
    {
        PROCESS_PRIORITY_CLASS_EX processPriorityClassEx;

        memset(&processPriorityClassEx, 0, sizeof(PROCESS_PRIORITY_CLASS_EX));
        processPriorityClassEx.PriorityClassValid = TRUE;
        processPriorityClassEx.PriorityClass = PriorityClass;

        status = NtSetInformationProcess(
            ProcessHandle,
            ProcessPriorityClassEx,
            &processPriorityClassEx,
            sizeof(PROCESS_PRIORITY_CLASS_EX)
            );

        if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
        {
            status = KphSetInformationProcess(
                ProcessHandle,
                KphProcessPriorityClassEx,
                &processPriorityClassEx,
                sizeof(PROCESS_PRIORITY_CLASS_EX)
                );
        }
    }
    else
    {
        PROCESS_PRIORITY_CLASS processPriorityClass;

        memset(&processPriorityClass, 0, sizeof(PROCESS_PRIORITY_CLASS));
        processPriorityClass.Foreground = FALSE;
        processPriorityClass.PriorityClass = PriorityClass;

        status = NtSetInformationProcess(
            ProcessHandle,
            ProcessPriorityClass,
            &processPriorityClass,
            sizeof(PROCESS_PRIORITY_CLASS)
            );

        if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
        {
            status = KphSetInformationProcess(
                ProcessHandle,
                KphProcessPriorityClass,
                &processPriorityClass,
                sizeof(PROCESS_PRIORITY_CLASS)
                );
        }
    }

    return status;
}

/**
 * Sets a process' I/O priority.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_SET_INFORMATION access.
 * \param IoPriority The new I/O priority.
 */
NTSTATUS PhSetProcessIoPriority(
    _In_ HANDLE ProcessHandle,
    _In_ IO_PRIORITY_HINT IoPriority
    )
{
    NTSTATUS status;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessIoPriority,
        &IoPriority,
        sizeof(IO_PRIORITY_HINT)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessIoPriority,
            &IoPriority,
            sizeof(IO_PRIORITY_HINT)
            );
    }

    return status;
}

/**
 * Sets the page priority for the specified process.
 *
 * \param ProcessHandle Handle to the process whose page priority is to be set.
 * \param PagePriority The desired page priority value.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetProcessPagePriority(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG PagePriority
    )
{
    // See also PhSetVirtualMemoryPagePriority (dmex)
    NTSTATUS status;
    PAGE_PRIORITY_INFORMATION pagePriorityInfo;

    pagePriorityInfo.PagePriority = PagePriority;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessPagePriority,
        &pagePriorityInfo,
        sizeof(PAGE_PRIORITY_INFORMATION)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessPagePriority,
            &pagePriorityInfo,
            sizeof(PAGE_PRIORITY_INFORMATION)
            );
    }

    return status;
}

/**
 * Sets the priority boost for a specified process.
 *
 * \param ProcessHandle Handle to the process whose priority boost setting is to be changed.
 * \param DisablePriorityBoost If TRUE, disables priority boost for the process; if FALSE, enables it.
 * \return NTSTATUS code indicating success or failure of the operation.
 */
NTSTATUS PhSetProcessPriorityBoost(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN DisablePriorityBoost
    )
{
    NTSTATUS status;
    ULONG priorityBoost;

    priorityBoost = DisablePriorityBoost ? 1 : 0;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessPriorityBoost,
        &priorityBoost,
        sizeof(ULONG)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessPriorityBoost,
            &priorityBoost,
            sizeof(ULONG)
            );
    }

    return status;
}

/**
 * Retrieves the activity moderation state for a process based on the specified moderation identifier.
 *
 * \param processId The identifier of the process to query.
 * \param moderationId The moderation identifier to check.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessActivityModerationState(
    _In_ PCPH_STRINGREF ModerationIdentifier,
    _Out_ PSYSTEM_ACTIVITY_MODERATION_APP_SETTINGS ModerationSettings
    )
{
    NTSTATUS status;
    SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS activityModeration;
    PKEY_VALUE_PARTIAL_INFORMATION keyValueInfo = NULL;

    RtlZeroMemory(&activityModeration, sizeof(SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS));

    status = NtQuerySystemInformation(
        SystemActivityModerationUserSettings,
        &activityModeration,
        sizeof(SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhQueryValueKey(
        activityModeration.UserKeyHandle,
        ModerationIdentifier,
        KeyValuePartialInformation,
        &keyValueInfo
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (
        keyValueInfo->Type != REG_BINARY ||
        keyValueInfo->DataLength != sizeof(SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS)
        )
    {
        status = STATUS_INVALID_KERNEL_INFO_VERSION;
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    RtlCopyMemory(
        ModerationSettings,
        keyValueInfo->Data,
        sizeof(SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS)
        );

CleanupExit:
    if (activityModeration.UserKeyHandle)
    {
        NtClose(activityModeration.UserKeyHandle);
    }
    if (keyValueInfo)
    {
        PhFree(keyValueInfo);
    }

    return status;
}

/**
 * Sets the activity moderation state for a process.
 *
 * \param ModerationIdentifier A pointer to a PH_STRINGREF structure that identifies the moderation context.
 * \param ModerationType The type of activity moderation to apply (SYSTEM_ACTIVITY_MODERATION_APP_TYPE).
 * \param ModerationState The desired moderation state (SYSTEM_ACTIVITY_MODERATION_STATE).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetProcessActivityModerationState(
    _In_ PCPH_STRINGREF ModerationIdentifier,
    _In_ SYSTEM_ACTIVITY_MODERATION_APP_TYPE ModerationType,
    _In_ SYSTEM_ACTIVITY_MODERATION_STATE ModerationState
    )
{
    NTSTATUS status;
    SYSTEM_ACTIVITY_MODERATION_INFO moderationInfo;

    RtlZeroMemory(&moderationInfo, sizeof(SYSTEM_ACTIVITY_MODERATION_INFO));
    moderationInfo.AppType = ModerationType;
    moderationInfo.ModerationState = ModerationState;

    if (!PhStringRefToUnicodeString(ModerationIdentifier, &moderationInfo.Identifier))
        return STATUS_NAME_TOO_LONG;

    status = NtSetSystemInformation(
        SystemActivityModerationExeState,
        &moderationInfo,
        sizeof(SYSTEM_ACTIVITY_MODERATION_INFO)
        );

    if (NT_SUCCESS(status))
    {
        SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS activityModeration;

        RtlZeroMemory(&activityModeration, sizeof(SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS));

        status = NtQuerySystemInformation(
            SystemActivityModerationUserSettings,
            &activityModeration,
            sizeof(SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS moderationSettings;

            RtlZeroMemory(&moderationSettings, sizeof(SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS));
            PhQuerySystemTime(&moderationSettings.LastUpdatedTime);
            moderationSettings.AppType = ModerationType;
            moderationSettings.ModerationState = ModerationState;

            status = PhSetValueKey(
                activityModeration.UserKeyHandle,
                ModerationIdentifier,
                REG_BINARY,
                &moderationSettings,
                sizeof(SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS)
                );

            NtClose(activityModeration.UserKeyHandle);
        }
    }

    return status;
}

/**
 * Sets a process' affinity mask.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_SET_INFORMATION access.
 * \param AffinityMask The new affinity mask.
 */
NTSTATUS PhSetProcessAffinityMask(
    _In_ HANDLE ProcessHandle,
    _In_ KAFFINITY AffinityMask
    )
{
    NTSTATUS status;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessAffinityMask,
        &AffinityMask,
        sizeof(KAFFINITY)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessAffinityMask,
            &AffinityMask,
            sizeof(KAFFINITY)
            );
    }

    return status;
}

/**
 * Sets the processor group affinity for a specified process.
 *
 * \param ProcessHandle Handle to the process whose group affinity is to be set.
 * \param GroupAffinity The GROUP_AFFINITY structure specifying the desired processor group and affinity mask.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetProcessGroupAffinity(
    _In_ HANDLE ProcessHandle,
    _In_ GROUP_AFFINITY GroupAffinity
    )
{
    NTSTATUS status;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessAffinityMask,
        &GroupAffinity,
        sizeof(GROUP_AFFINITY)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessAffinityMask,
            &GroupAffinity,
            sizeof(GROUP_AFFINITY)
            );
    }

    return status;
}

/**
 * Query the power throttling state for a specified process.
 *
 * \param ProcessHandle The handle to the target process.
 * \param PowerThrottlingState The control and state masks indicating the current power throttling of the process.
 * \return The NTSTATUS code indicating the success or failure of the operation.
 */
NTSTATUS PhGetProcessPowerThrottlingState(
    _In_ HANDLE ProcessHandle,
    _Out_ PPOWER_THROTTLING_PROCESS_STATE PowerThrottlingState
    )
{
    NTSTATUS status;

    memset(PowerThrottlingState, 0, sizeof(POWER_THROTTLING_PROCESS_STATE));
    PowerThrottlingState->Version = POWER_THROTTLING_PROCESS_CURRENT_VERSION;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessPowerThrottlingState,
        PowerThrottlingState,
        sizeof(POWER_THROTTLING_PROCESS_STATE),
        NULL
        );

    return status;
}

/**
 * Sets the power throttling state for a specified process.
 *
 * \param ProcessHandle The handle to the target process.
 * \param ControlMask The control mask specifying the power throttling control actions to perform.
 * \param StateMask The state mask specifying the power throttling states to set.
 * \return The NTSTATUS code indicating the success or failure of the operation.
 */
NTSTATUS PhSetProcessPowerThrottlingState(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG ControlMask,
    _In_ ULONG StateMask
    )
{
    NTSTATUS status;
    POWER_THROTTLING_PROCESS_STATE powerThrottlingState;

    memset(&powerThrottlingState, 0, sizeof(POWER_THROTTLING_PROCESS_STATE));
    powerThrottlingState.Version = POWER_THROTTLING_PROCESS_CURRENT_VERSION;
    powerThrottlingState.ControlMask = ControlMask;
    powerThrottlingState.StateMask = StateMask;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessPowerThrottlingState,
        &powerThrottlingState,
        sizeof(POWER_THROTTLING_PROCESS_STATE)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessPowerThrottlingState,
            &powerThrottlingState,
            sizeof(POWER_THROTTLING_PROCESS_STATE)
            );
    }

    return status;
}

// rev from KernelBase!QueryProcessMachine (jxy-s)
/**
 * Retrieves the architecture of the specified process.
 *
 * \param ProcessHandle A handle to the process whose architecture is to be determined.
 * \param ProcessArchitecture A pointer to a variable that receives the process architecture.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessArchitecture(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT ProcessArchitecture
    )
{
    NTSTATUS status;
    HANDLE input[1] = { ProcessHandle };
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION output[6];
    ULONG returnLength;

    status = NtQuerySystemInformationEx(
        SystemSupportedProcessorArchitectures2,
        input,
        sizeof(input),
        output,
        sizeof(output),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        for (ULONG i = 0; i < returnLength / sizeof(SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION); i++)
        {
            if (output[i].Process)
            {
                *ProcessArchitecture = (USHORT)output[i].Machine;
                return STATUS_SUCCESS;
            }
        }

        status = STATUS_NOT_FOUND;
    }

    return status;
}

/**
 * Retrieves the base address of the image for a specified process.
 *
 * \param ProcessHandle A handle to the process whose image base address is to be retrieved.
 * \param ImageBaseAddress A pointer that receives the image base address of the process.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessImageBaseAddress(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* ImageBaseAddress
    )
{
    NTSTATUS status;
    PVOID pebAddress;
    PVOID baseAddress;
#ifdef _WIN64
    BOOLEAN isWow64;

    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        ULONG imageBaseAddress32;

        status = PhGetProcessPeb32(ProcessHandle, &pebAddress);

        if (!NT_SUCCESS(status))
            return status;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebAddress, UFIELD_OFFSET(PEB32, ImageBaseAddress)),
            &imageBaseAddress32,
            sizeof(ULONG),
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;

        baseAddress = UlongToPtr(imageBaseAddress32);
    }
    else
#endif
    {
        PVOID imageBaseAddress;

        status = PhGetProcessPeb(ProcessHandle, &pebAddress);

        if (!NT_SUCCESS(status))
            return status;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebAddress, UFIELD_OFFSET(PEB, ImageBaseAddress)),
            &imageBaseAddress,
            sizeof(PVOID),
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;

        baseAddress = imageBaseAddress;
    }

    if (NT_SUCCESS(status))
    {
        *ImageBaseAddress = baseAddress;
    }

    return status;
}

/**
 * Retrieves the security domain identifier for a specified process.
 *
 * \param ProcessHandle A handle to the process whose security domain is to be retrieved.
 * \param SecurityDomain A pointer to a variable that receives the security domain identifier.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessSecurityDomain(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONGLONG SecurityDomain
    )
{
    NTSTATUS status;
    PROCESS_SECURITY_DOMAIN_INFORMATION processSecurityDomainInfo;

    memset(&processSecurityDomainInfo, 0, sizeof(PROCESS_SECURITY_DOMAIN_INFORMATION));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessSecurityDomainInformation,
        &processSecurityDomainInfo,
        sizeof(PROCESS_SECURITY_DOMAIN_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *SecurityDomain = processSecurityDomainInfo.SecurityDomain;
    }

    return status;
}

/**
 * Retrieves the Server Silo identifier for a specified process.
 *
 * \param ProcessHandle A handle to the process for which the Server Silo information is to be retrieved.
 * \param ServerSilo A pointer to a variable that receives the Server Silo identifier.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessServerSilo(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ServerSilo
    )
{
    NTSTATUS status;
    PROCESS_MEMBERSHIP_INFORMATION processMembershipInfo;

    memset(&processMembershipInfo, 0, sizeof(PROCESS_MEMBERSHIP_INFORMATION));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessMembershipInformation,
        &processMembershipInfo,
        sizeof(PROCESS_MEMBERSHIP_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ServerSilo = processMembershipInfo.ServerSiloId;
    }

    return status;
}

/**
 * Retrieves the sequence number of a process.
 *
 * \param ProcessHandle A handle to the process whose sequence number is to be retrieved.
 * \param SequenceNumber A pointer to a variable that receives the process sequence number.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessSequenceNumber(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONGLONG SequenceNumber
    )
{
    NTSTATUS status;

    if (KsiLevel() >= KphLevelLow)
    {
        // The driver exposes this information earlier than ProcessSequenceNumber was introduced.
        // Where not available it synthesizes it for informer messages, for consistency use it if
        // it's enabled.
        status = KphQueryInformationProcess(
            ProcessHandle,
            KphProcessSequenceNumber,
            SequenceNumber,
            sizeof(ULONGLONG),
            NULL
            );
    }
    else
    {
        ULONGLONG sequenceNumber = 0;

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessSequenceNumber,
            &sequenceNumber,
            sizeof(ULONGLONG),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            *SequenceNumber = sequenceNumber;
        }
        else if (status == STATUS_INVALID_INFO_CLASS && WindowsVersion >= WINDOWS_10)
        {
            PROCESS_TELEMETRY_ID_INFORMATION telemetryInfo;

            memset(&telemetryInfo, 0, sizeof(PROCESS_TELEMETRY_ID_INFORMATION));

            // ProcessTelemetryIdInformation exposes the process sequence number (and process start
            // key) earlier than ProcessSequenceNumber was introduced.
            status = NtQueryInformationProcess(
                ProcessHandle,
                ProcessTelemetryIdInformation,
                &telemetryInfo,
                sizeof(PROCESS_TELEMETRY_ID_INFORMATION),
                NULL
                );

            if (
                status == STATUS_BUFFER_OVERFLOW &&
                RTL_CONTAINS_FIELD(&telemetryInfo, telemetryInfo.HeaderSize, ProcessSequenceNumber)
                )
            {
                status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(status))
            {
                *SequenceNumber = telemetryInfo.ProcessSequenceNumber;
            }
        }
    }

    return status;
}

/**
 * Retrieves the unique start key for a specified process.
 *
 * \param ProcessHandle Handle to the process whose start key is to be retrieved.
 * \param ProcessStartKey Pointer to a variable that receives the process start key.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessStartKey(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONGLONG ProcessStartKey
    )
{
    NTSTATUS status;

    if (KsiLevel() >= KphLevelLow)
    {
        // The driver exposes this information earlier than ProcessSequenceNumber was introduced.
        // Where not available it synthesizes it for informer messages, for consistency use it if
        // it's enabled.
        status = KphQueryInformationProcess(
            ProcessHandle,
            KphProcessStartKey,
            ProcessStartKey,
            sizeof(ULONGLONG),
            NULL
            );
    }
    else
    {
        ULONGLONG processSequenceNumber;

        status = PhGetProcessSequenceNumber(
            ProcessHandle,
            &processSequenceNumber
            );

        if (NT_SUCCESS(status))
        {
            *ProcessStartKey = PH_PROCESS_EXTENSION_STARTKEY(processSequenceNumber);
        }
    }

    return status;
}

/**
 * Retrieves the telemetry application session GUID for a specified process.
 *
 * \param ProcessHandle Handle to the process whose telemetry session GUID is to be retrieved.
 * \param TelemetrySessionGuid Pointer to a GUID structure that receives the telemetry session GUID.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessTelemetryAppSessionGuid(
    _In_ HANDLE ProcessHandle,
    _Out_ PGUID TelemetrySessionGuid
    )
{
    NTSTATUS status;
    PROCESS_TELEMETRY_ID_INFORMATION telemetryInfo;
    ULONG returnLength;

    memset(&telemetryInfo, 0, sizeof(PROCESS_TELEMETRY_ID_INFORMATION));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessTelemetryIdInformation,
        &telemetryInfo,
        sizeof(PROCESS_TELEMETRY_ID_INFORMATION),
        &returnLength
        );

    if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW &&
        RTL_CONTAINS_FIELD(&telemetryInfo, returnLength, BootId) &&
        RTL_CONTAINS_FIELD(&telemetryInfo, telemetryInfo.HeaderSize, BootId)))
    {
        GUID telemetryAppGuid;

        memset(&telemetryAppGuid, 0, sizeof(GUID));
        telemetryAppGuid.Data1 = telemetryInfo.ProcessId;
        telemetryAppGuid.Data2 = (USHORT)telemetryInfo.SessionId;
        telemetryAppGuid.Data3 = (USHORT)telemetryInfo.BootId;

        if (memcpy_s(telemetryAppGuid.Data4, sizeof(telemetryAppGuid.Data4), &telemetryInfo.CreateTime, sizeof(telemetryInfo.CreateTime)))
            return STATUS_FAIL_CHECK;
        if (memcpy_s(TelemetrySessionGuid, sizeof(*TelemetrySessionGuid), &telemetryAppGuid, sizeof(telemetryAppGuid)))
            return STATUS_FAIL_CHECK;
    }

    return status;
}

/**
 * Retrieves telemetry ID information for a specified process.
 *
 * \param ProcessHandle Handle to the process for which telemetry information is to be retrieved.
 * \param TelemetryInformation Pointer to a variable that receives a pointer to the telemetry ID information structure.
 * \param TelemetryInformationLength Optional pointer to a variable that receives the length, in bytes, of the telemetry information structure.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessTelemetryIdInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_TELEMETRY_ID_INFORMATION* TelemetryInformation,
    _Out_opt_ PULONG TelemetryInformationLength
    )
{
    NTSTATUS status;
    PPROCESS_TELEMETRY_ID_INFORMATION telemetryBuffer;
    ULONG telemetryLength;
    ULONG returnLength;
    ULONG attempts;

    telemetryLength = sizeof(PROCESS_TELEMETRY_ID_INFORMATION) + SECURITY_MAX_SID_SIZE + (DOS_MAX_PATH_LENGTH * sizeof(WCHAR)) + (DOS_MAX_PATH_LENGTH * sizeof(WCHAR));
    telemetryBuffer = PhAllocateZero(telemetryLength);

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessTelemetryIdInformation,
        telemetryBuffer,
        telemetryLength,
        &returnLength
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        telemetryLength = returnLength;
        telemetryBuffer = PhReAllocate(telemetryBuffer, telemetryLength);

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessTelemetryIdInformation,
            telemetryBuffer,
            telemetryLength,
            &returnLength
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
    {
        *TelemetryInformation = telemetryBuffer;

        if (TelemetryInformationLength)
        {
            *TelemetryInformationLength = telemetryLength;
        }
    }

    return status;
}

/**
 * Retrieves the count of TLS (Thread Local Storage) bitmap entries and TLS expansion bitmap entries for a specified process.
 *
 * \param ProcessHandle A handle to the process whose TLS bitmap counters are to be retrieved.
 * \param TlsBitMapCount Pointer to a variable that receives the count of TLS bitmap entries.
 * \param TlsExpansionBitMapCount Pointer to a variable that receives the count of TLS expansion bitmap entries.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessTlsBitMapCounters(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG TlsBitMapCount,
    _Out_ PULONG TlsExpansionBitMapCount
    )
{
    NTSTATUS status;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif
    PVOID pebBaseAddress;
    RTL_BITMAP tlsBitMap;
    RTL_BITMAP tlsExpansionBitMap;
    ULONG bitmapBits[2] = { 0 };
    ULONG bitmapExpansionBits[32] = { 0 };

    static_assert(sizeof(bitmapBits) == RTL_FIELD_SIZE(PEB, TlsBitmapBits), "Buffer must equal TlsBitmapBits");
    static_assert(sizeof(bitmapExpansionBits) == RTL_FIELD_SIZE(PEB, TlsExpansionBitmapBits), "Buffer must equal TlsExpansionBitmapBits");

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB32, TlsBitmapBits)),
            bitmapBits,
            sizeof(bitmapBits),
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB32, TlsExpansionBitmapBits)),
            bitmapExpansionBits,
            sizeof(bitmapExpansionBits),
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }
    else
#endif
    {
        status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, TlsBitmapBits)),
            bitmapBits,
            sizeof(bitmapBits),
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, TlsExpansionBitmapBits)),
            bitmapExpansionBits,
            sizeof(bitmapExpansionBits),
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    RtlInitializeBitMap(&tlsBitMap, bitmapBits, TLS_MINIMUM_AVAILABLE);
    RtlInitializeBitMap(&tlsExpansionBitMap, bitmapExpansionBits, TLS_EXPANSION_SLOTS);

    *TlsBitMapCount = RtlNumberOfSetBits(&tlsBitMap);
    *TlsExpansionBitMapCount = RtlNumberOfSetBits(&tlsExpansionBitMap);

CleanupExit:
    return status;
}

/**
 * Gets whether the process is running under the POSIX subsystem.
 *
 * \param ProcessHandle A handle to a process.
 * \param IsPosix A variable which receives a boolean
 * indicating whether the process is running under the POSIX subsystem.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessIsPosix(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsPosix
    )
{
    NTSTATUS status;
    PVOID pebBaseAddress;
    ULONG imageSubsystem;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    if (WindowsVersion >= WINDOWS_10)
    {
        *IsPosix = FALSE; // Not supported (dmex)
        return STATUS_SUCCESS;
    }

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            return status;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB32, ImageSubsystem)),
            &imageSubsystem,
            sizeof(ULONG),
            NULL
            );
    }
    else
#endif
    {
        status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            return status;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, ImageSubsystem)),
            &imageSubsystem,
            sizeof(ULONG),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        *IsPosix = imageSubsystem == IMAGE_SUBSYSTEM_POSIX_CUI;
    }

    return status;
}
