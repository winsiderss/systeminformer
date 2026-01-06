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
