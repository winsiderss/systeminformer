#ifndef _PH_PHNATINL_H
#define _PH_PHNATINL_H

#pragma once

// This file contains inlined native API wrapper functions.
// These functions were previously exported, but are now inlined
// because they are extremely simple wrappers around equivalent
// native API functions.

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
 * \param ProcessHandle A handle to a process. The handle
 * must have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param SessionId A variable which receives the
 * process' session ID.
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
 * Gets whether a process is running under 32-bit
 * emulation.
 *
 * \param ProcessHandle A handle to a process. The handle
 * must have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param IsWow64 A variable which receives a boolean
 * indicating whether the process is 32-bit.
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
 * \param ProcessHandle A handle to a process. The handle
 * must have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param Peb32 A variable which receives the base address
 * of the process' WOW64 PEB. If the process is 64-bit,
 * the variable receives NULL.
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
        *Peb32 = (PVOID)wow64;
    }

    return status;
}

/**
 * Gets whether a process is being debugged.
 *
 * \param ProcessHandle A handle to a process. The handle
 * must have PROCESS_QUERY_INFORMATION access.
 * \param IsBeingDebugged A variable which receives a boolean
 * indicating whether the process is being debugged.
 */
FORCEINLINE
NTSTATUS
PhGetProcessIsBeingDebugged(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsBeingDebugged
    )
{
    NTSTATUS status;
    PVOID debugPort;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessDebugPort,
        &debugPort,
        sizeof(PVOID),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *IsBeingDebugged = !!debugPort;
    }

    return status;
}

/**
 * Gets a handle to a process' debug object.
 *
 * \param ProcessHandle A handle to a process. The handle
 * must have PROCESS_QUERY_INFORMATION access.
 * \param DebugObjectHandle A variable which receives a
 * handle to the debug object associated with the process.
 * You must close the handle when you no longer need it.
 *
 * \retval STATUS_PORT_NOT_SET The process is not being
 * debugged and has no associated debug object.
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

/**
 * Gets a process' I/O priority.
 *
 * \param ProcessHandle A handle to a process. The handle
 * must have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param IoPriority A variable which receives the I/O
 * priority of the process.
 */
FORCEINLINE
NTSTATUS
PhGetProcessIoPriority(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG IoPriority
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessIoPriority,
        IoPriority,
        sizeof(ULONG),
        NULL
        );
}

/**
 * Gets a process' page priority.
 *
 * \param ProcessHandle A handle to a process. The handle
 * must have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param PagePriority A variable which receives the page
 * priority of the process.
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

/**
 * Gets a process' cycle count.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param CycleTime A variable which receives the 64-bit cycle
 * time.
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

    if (!NT_SUCCESS(status))
        return status;

    *CycleTime = cycleTimeInfo.AccumulatedCycles;

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

    if (!NT_SUCCESS(status))
        return status;

    *ConsoleHostProcessId = (HANDLE)consoleHostProcess;

    return status;
}

/**
 * Sets a process' affinity mask.
 *
 * \param ProcessHandle A handle to a process. The handle
 * must have PROCESS_SET_INFORMATION access.
 * \param AffinityMask The new affinity mask.
 */
FORCEINLINE
NTSTATUS
PhSetProcessAffinityMask(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG_PTR AffinityMask
    )
{
    return NtSetInformationProcess(
        ProcessHandle,
        ProcessAffinityMask,
        &AffinityMask,
        sizeof(ULONG_PTR)
        );
}

/**
 * Gets basic information for a thread.
 *
 * \param ThreadHandle A handle to a thread. The handle must have
 * THREAD_QUERY_LIMITED_INFORMATION access.
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

/**
 * Gets a thread's I/O priority.
 *
 * \param ThreadHandle A handle to a thread. The handle
 * must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param IoPriority A variable which receives the I/O
 * priority of the thread.
 */
FORCEINLINE
NTSTATUS
PhGetThreadIoPriority(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG IoPriority
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadIoPriority,
        IoPriority,
        sizeof(ULONG),
        NULL
        );
}

/**
 * Gets a thread's page priority.
 *
 * \param ThreadHandle A handle to a thread. The handle
 * must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param PagePriority A variable which receives the page
 * priority of the thread.
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

/**
 * Gets a thread's cycle count.
 *
 * \param ThreadHandle A handle to a thread. The handle must have
 * THREAD_QUERY_LIMITED_INFORMATION access.
 * \param CycleTime A variable which receives the 64-bit cycle
 * time.
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

    if (!NT_SUCCESS(status))
        return status;

    *CycleTime = cycleTimeInfo.AccumulatedCycles;

    return status;
}

/**
 * Sets a thread's affinity mask.
 *
 * \param ThreadHandle A handle to a thread. The handle
 * must have THREAD_SET_LIMITED_INFORMATION access.
 * \param AffinityMask The new affinity mask.
 */
FORCEINLINE
NTSTATUS
PhSetThreadAffinityMask(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG_PTR AffinityMask
    )
{
    return NtSetInformationThread(
        ThreadHandle,
        ThreadAffinityMask,
        &AffinityMask,
        sizeof(ULONG_PTR)
        );
}

FORCEINLINE
NTSTATUS
PhGetJobBasicAndIoAccounting(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION BasicAndIoAccounting
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectBasicAndIoAccountingInformation,
        BasicAndIoAccounting,
        sizeof(JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetJobBasicLimits(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimits
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectBasicLimitInformation,
        BasicLimits,
        sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetJobExtendedLimits(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimits
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectExtendedLimitInformation,
        ExtendedLimits,
        sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION),
        NULL
        );
}

FORCEINLINE
NTSTATUS
PhGetJobBasicUiRestrictions(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_UI_RESTRICTIONS BasicUiRestrictions
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectBasicUIRestrictions,
        BasicUiRestrictions,
        sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS),
        NULL
        );
}

/**
 * Gets a token's session ID.
 *
 * \param TokenHandle A handle to a token. The handle
 * must have TOKEN_QUERY access.
 * \param SessionId A variable which receives the
 * session ID.
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
 * \param TokenHandle A handle to a token. The handle
 * must have TOKEN_QUERY access.
 * \param ElevationType A variable which receives the
 * elevation type.
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
 * \param TokenHandle A handle to a token. The handle
 * must have TOKEN_QUERY access.
 * \param Elevated A variable which receives a
 * boolean indicating whether the token is elevated.
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
 * \param TokenHandle A handle to a token. The handle
 * must have TOKEN_QUERY access.
 * \param Statistics A variable which receives the
 * token's statistics.
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
 * \param TokenHandle A handle to a token. The handle
 * must have TOKEN_QUERY_SOURCE access.
 * \param Source A variable which receives the
 * token's source.
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
 * \param TokenHandle A handle to a token. The handle
 * must have TOKEN_QUERY access.
 * \param LinkedTokenHandle A variable which receives a
 * handle to the linked token. You must close the handle
 * using NtClose() when you no longer need it.
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

    if (!NT_SUCCESS(status))
        return status;

    *LinkedTokenHandle = linkedToken.LinkedToken;

    return status;
}

/**
 * Gets whether virtualization is allowed for a token.
 *
 * \param TokenHandle A handle to a token. The handle
 * must have TOKEN_QUERY access.
 * \param IsVirtualizationAllowed A variable which receives
 * a boolean indicating whether virtualization is allowed
 * for the token.
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

    if (!NT_SUCCESS(status))
        return status;

    *IsVirtualizationAllowed = !!virtualizationAllowed;

    return status;
}

/**
 * Gets whether virtualization is enabled for a token.
 *
 * \param TokenHandle A handle to a token. The handle
 * must have TOKEN_QUERY access.
 * \param IsVirtualizationEnabled A variable which receives
 * a boolean indicating whether virtualization is enabled
 * for the token.
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

#endif
