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
#include <apiimport.h>
#include <hndlinfo.h>
#include <kphuser.h>
#include <lsasup.h>
#include <mapldr.h>
#include <phafd.h>

/**
 * Opens a thread.
 *
 * \param[out] ThreadHandle A variable which receives a handle to the thread.
 * \param[in] DesiredAccess The desired access to the thread.
 * \param[in] ThreadId The ID of the thread.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    CLIENT_ID clientId;
    KPH_LEVEL level;

    clientId.UniqueProcess = NULL;
    clientId.UniqueThread = ThreadId;

    level = KsiLevel();

    if ((level >= KphLevelMed) && (DesiredAccess & KPH_THREAD_READ_ACCESS) == DesiredAccess)
    {
        status = KphOpenThread(
            ThreadHandle,
            DesiredAccess,
            &clientId
            );
    }
    else
    {
        InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
        status = NtOpenThread(
            ThreadHandle,
            DesiredAccess,
            &objectAttributes,
            &clientId
            );

        if (status == STATUS_ACCESS_DENIED && (level == KphLevelMax))
        {
            status = KphOpenThread(
                ThreadHandle,
                DesiredAccess,
                &clientId
                );
        }
    }

    return status;
}

/**
 * Opens a thread using a CLIENT_ID structure.
 *
 * \param[out] ThreadHandle Receives a handle to the opened thread.
 * \param[in] DesiredAccess The access rights requested for the thread.
 * \param[in] ClientId Pointer to a CLIENT_ID structure identifying the thread.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenThreadClientId(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    KPH_LEVEL level;

    level = KsiLevel();

    if ((level >= KphLevelMed) && (DesiredAccess & KPH_THREAD_READ_ACCESS) == DesiredAccess)
    {
        status = KphOpenThread(
            ThreadHandle,
            DesiredAccess,
            ClientId
            );
    }
    else
    {
        InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
        status = NtOpenThread(
            ThreadHandle,
            DesiredAccess,
            &objectAttributes,
            ClientId
            );

        if (status == STATUS_ACCESS_DENIED && (level == KphLevelMax))
        {
            status = KphOpenThread(
                ThreadHandle,
                DesiredAccess,
                ClientId
                );
        }
    }

    return status;
}

/**
 * Limited API for untrusted/external code.
 *
 * \param[out] ThreadHandle A variable which receives a handle to the thread.
 * \param[in] DesiredAccess The desired access to the thread.
 * \param[in] ThreadId The ID of the thread.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenThreadPublic(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadId
    )
{
    OBJECT_ATTRIBUTES objectAttributes;
    CLIENT_ID clientId;

    clientId.UniqueProcess = NULL;
    clientId.UniqueThread = ThreadId;

    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);

    return NtOpenThread(
        ThreadHandle,
        DesiredAccess,
        &objectAttributes,
        &clientId
        );
}

/**
 * Opens the process that owns the specified thread.
 *
 * \param[in] ThreadHandle Handle to the thread whose owning process is to be opened.
 * \param[in] DesiredAccess Access rights requested for the process handle.
 * \param[out] ProcessHandle Receives a handle to the opened process.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    KPH_LEVEL level;

    level = KsiLevel();

    if (level == KphLevelMax || (level >= KphLevelMed && FlagOn(DesiredAccess, KPH_PROCESS_READ_ACCESS) == DesiredAccess))
    {
        status = KphOpenThreadProcess(
            ThreadHandle,
            DesiredAccess,
            ProcessHandle
            );

        if (NT_SUCCESS(status))
            return status;
    }

    status = PhGetThreadBasicInformation(
        ThreadHandle,
        &basicInfo
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhOpenProcessClientId(
        ProcessHandle,
        DesiredAccess,
        &basicInfo.ClientId
        );

    return status;
}

/**
 * Gets basic information for a thread.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param BasicInformation A variable which receives the information.
 * \return Successful or errant status.
 */
NTSTATUS PhGetThreadBasicInformation(
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
 * Retrieves the base priority of a thread.
 *
 * \param[in] ThreadHandle A handle to the thread whose base priority is to be retrieved.
 * \param[out] Increment A pointer to a variable that receives the base priority value (KPRIORITY).
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadBasePriority(
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

/**
 * Retrieves the Thread Environment Block (TEB) base address for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread whose TEB base address is to be retrieved.
 * \param[out] TebBaseAddress A pointer to a variable that receives the TEB base address as an ULONG_PTR.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadTeb(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG_PTR TebBaseAddress
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;

    status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo);

    if (NT_SUCCESS(status))
    {
        if (!basicInfo.TebBaseAddress)
            return STATUS_UNSUCCESSFUL;

        *TebBaseAddress = (ULONG_PTR)basicInfo.TebBaseAddress;
    }

    return status;
}

/**
 * Retrieves the Thread Environment Block (TEB32) base address for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread whose TEB32 base address is to be retrieved.
 * \param[out] TebBaseAddress A pointer to a variable that receives the TEB base address as an ULONG_PTR.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadTeb32(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG_PTR TebBaseAddress
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;

    status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo);

    if (NT_SUCCESS(status))
    {
        if (!basicInfo.TebBaseAddress)
            return STATUS_UNSUCCESSFUL;

        *TebBaseAddress = (ULONG_PTR)WOW64_GET_TEB32(basicInfo.TebBaseAddress);
    }

    return status;
}

/**
 * Gets a thread's Win32 start address.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param StartAddress A variable which receives the start address of the thread.
 * \return Successful or errant status.
 */
NTSTATUS PhGetThreadStartAddress(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG_PTR StartAddress
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadQuerySetWin32StartAddress,
        StartAddress,
        sizeof(ULONG_PTR),
        NULL
        );
}

/**
 * Gets a thread's I/O priority.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param IoPriority A variable which receives the I/O priority of the thread.
 * \return Successful or errant status.
 */
NTSTATUS PhGetThreadIoPriority(
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
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param PagePriority A variable which receives the page priority of the thread.
 * \return Successful or errant status.
 */
NTSTATUS PhGetThreadPagePriority(
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
 * Gets a thread's dynamic boosting.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param PriorityBoostDisabled A variable which receives the dynamic boosting state.
 * \return Successful or errant status.
 */
NTSTATUS PhGetThreadPriorityBoost(
    _In_ HANDLE ThreadHandle,
    _Out_ PBOOLEAN PriorityBoostDisabled
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
        *PriorityBoostDisabled = !!priorityBoost;
    }

    return status;
}

/**
 * Gets a thread's performance counter.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param PerformanceCounter A variable which receives the 64-bit performance counter.
 * \return Successful or errant status.
 */
NTSTATUS PhGetThreadPerformanceCounter(
    _In_ HANDLE ThreadHandle,
    _Out_ PLARGE_INTEGER PerformanceCounter
    )
{
    NTSTATUS status;
    LARGE_INTEGER performanceCounter;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadPerformanceCount,
        &performanceCounter,
        sizeof(LARGE_INTEGER),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *PerformanceCounter = performanceCounter;
    }

    return status;
}

/**
 * Gets a thread's cycle count.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param CycleTime A variable which receives the 64-bit cycle time.
 * \return Successful or errant status.
 */
NTSTATUS PhGetThreadCycleTime(
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

/**
 * Retrieves the ideal processor for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread whose ideal processor is to be retrieved.
 * \param[out] ProcessorNumber A pointer to a PROCESSOR_NUMBER structure that receives the ideal processor information.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadIdealProcessor(
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

/**
 * Retrieves the suspend count for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread whose suspend count is to be retrieved.
 * \param[out] SuspendCount A pointer to a variable that receives the suspend count.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadSuspendCount(
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

/**
 * Retrieves the WOW64 context for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread whose WOW64 context is to be retrieved.
 * \param[out] Context A pointer to a WOW64_CONTEXT structure that receives the context.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadWow64Context(
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
/**
 * Retrieves the ARM32 context for a thread on ARM64 systems.
 *
 * \param[in] ThreadHandle A handle to the thread whose ARM32 context is to be retrieved.
 * \param[out] Context A pointer to an ARM_NT_CONTEXT structure that receives the context.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadArm32Context(
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

/**
 * Retrieves the break on termination state for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \param[out] BreakOnTermination A pointer to a variable that receives the break on termination state.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadBreakOnTermination(
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

/**
 * Sets the break on termination state for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \param[in] BreakOnTermination TRUE to enable break on termination, FALSE to disable.
 * \return Successful or errant status.
 */
NTSTATUS PhSetThreadBreakOnTermination(
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

/**
 * Retrieves the container ID for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \param[out] ContainerId A pointer to a GUID structure that receives the container ID.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadContainerId(
    _In_ HANDLE ThreadHandle,
    _In_ PGUID ContainerId
    )
{
    NTSTATUS status;
    GUID threadContainerId;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadContainerId,
        &threadContainerId,
        sizeof(ULONG),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        memcpy(ContainerId, &threadContainerId, sizeof(GUID));
    }

    return status;
}

/**
 * Retrieves the I/O pending state for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread whose I/O pending state is to be retrieved.
 * \param[out] IsIoPending A pointer to a variable that receives the I/O pending state.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadIsIoPending(
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
 * \param ThreadHandle A handle to a thread.
 * \param Times A variable which receives the information.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadTimes(
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

/**
 * Retrieves the termination state for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread whose termination state is to be retrieved.
 * \param[out] IsTerminated A pointer to a variable that receives the termination state (TRUE if terminated).
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadIsTerminated(
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

/**
 * Determines if a thread is terminated by waiting with zero timeout.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \return TRUE if the thread is terminated, FALSE otherwise.
 */
BOOLEAN PhGetThreadIsTerminated2(
    _In_ HANDLE ThreadHandle
    )
{
    NTSTATUS status;
    LARGE_INTEGER timeout;

    memset(&timeout, 0, sizeof(LARGE_INTEGER));

    status = NtWaitForSingleObject(
        ThreadHandle,
        FALSE,
        &timeout
        );

    return status == STATUS_WAIT_0;
}

/**
 * Retrieves the group affinity for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread whose group affinity is to be retrieved.
 * \param[out] GroupAffinity A pointer to a GROUP_AFFINITY structure that receives the group affinity.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadGroupAffinity(
    _In_ HANDLE ThreadHandle,
    _Out_ PGROUP_AFFINITY GroupAffinity
    )
{
    ULONG returnLength;

    return NtQueryInformationThread(
        ThreadHandle,
        ThreadGroupInformation,
        GroupAffinity,
        sizeof(GROUP_AFFINITY),
        &returnLength
        );
}

/**
 * Retrieves the index information for a thread.
 *
 * \param[in] ThreadHandle A handle to the thread whose index information is to be retrieved.
 * \param[out] ThreadIndex A pointer to a THREAD_INDEX_INFORMATION structure that receives the index information.
 * \return Successful or errant status.
 * \remarks The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadIndexInformation(
    _In_ HANDLE ThreadHandle,
    _Out_ PTHREAD_INDEX_INFORMATION ThreadIndex
    )
{
    ULONG returnLength;

    return NtQueryInformationThread(
        ThreadHandle,
        ThreadIndexInformation,
        ThreadIndex,
        sizeof(THREAD_INDEX_INFORMATION),
        &returnLength
        );
}

/**
 * Terminates a thread.
 *
 * \param[in] ThreadHandle A handle to the thread to be terminated.
 * \param[in] ExitStatus The exit status for the thread.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhTerminateThread(
    _In_ HANDLE ThreadHandle,
    _In_ NTSTATUS ExitStatus
    )
{
    NTSTATUS status;

    status = NtTerminateThread(
        ThreadHandle,
        ExitStatus
        );

    return status;
}

/**
 * Retrieves the context of a thread.
 *
 * \param[in] ThreadHandle The handle to the thread.
 * \param[in,out] ThreadContext A pointer to the CONTEXT structure that receives the thread context.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetContextThread(
    _In_ HANDLE ThreadHandle,
    _Inout_ PCONTEXT ThreadContext
    )
{
    NTSTATUS status;

    status = NtGetContextThread(
        ThreadHandle,
        ThreadContext
        );

    return status;
}

/**
 * Atomically queries raw bytes from a thread's TEB at a specified offset.
 *
 * \param[in] ThreadHandle Handle to target thread.
 * \param[in,out] TebInformation Buffer to receive the bytes.
 * \param[in] TebOffset Offset (in bytes) from TEB base.
 * \param[in] BytesToRead Number of bytes to read.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadTebInformationAtomic(
    _In_ HANDLE ThreadHandle,
    _Inout_bytecount_(BytesToRead) PVOID TebInformation,
    _In_ ULONG TebOffset,
    _In_ ULONG BytesToRead
    )
{
    NTSTATUS status;
    THREAD_TEB_INFORMATION threadInfo;
    ULONG returnLength;

    if (WindowsVersion < WINDOWS_11_24H2)
        return STATUS_NOT_SUPPORTED;

    threadInfo.TebInformation = TebInformation;
    threadInfo.TebOffset = TebOffset; // FIELD_OFFSET(TEB, Value);
    threadInfo.BytesToRead = BytesToRead; // RTL_FIELD_SIZE(TEB, Value);

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadTebInformationAtomic,
        &threadInfo,
        sizeof(THREAD_TEB_INFORMATION),
        &returnLength
        );

    return status;
}

/**
 * Retrieves a thread's name (Set by SetThreadDescription API).
 *
 * \param[in] ThreadHandle Handle to thread (THREAD_QUERY_LIMITED_INFORMATION).
 * \param[out] ThreadName Receives allocated PPH_STRING containing the name.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadName(
    _In_ HANDLE ThreadHandle,
    _Out_ PPH_STRING *ThreadName
    )
{
    NTSTATUS status;
    PTHREAD_NAME_INFORMATION buffer;
    ULONG bufferSize;
    ULONG returnLength;

    if (WindowsVersion < WINDOWS_10)
        return STATUS_NOT_SUPPORTED;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadNameInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = NtQueryInformationThread(
            ThreadHandle,
            ThreadNameInformation,
            buffer,
            bufferSize,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        // Note: Some threads have UNICODE_NULL as their name. (dmex)
        if (RtlIsNullOrEmptyUnicodeString(&buffer->ThreadName))
        {
            PhFree(buffer);
            return STATUS_UNSUCCESSFUL;
        }

        *ThreadName = PhCreateStringFromUnicodeString(&buffer->ThreadName);
    }

    PhFree(buffer);

    return status;
}

/**
 * Sets the description (thread name) for a thread.
 *
 * \param[in] ThreadHandle Handle to thread (THREAD_SET_LIMITED_INFORMATION).
 * \param[in] ThreadName Wide string for new name.
 * \returns NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetThreadName(
    _In_ HANDLE ThreadHandle,
    _In_ PCWSTR ThreadName
    )
{
    NTSTATUS status;
    THREAD_NAME_INFORMATION threadNameInfo;

    if (WindowsVersion < WINDOWS_10)
        return STATUS_NOT_SUPPORTED;

    memset(&threadNameInfo, 0, sizeof(THREAD_NAME_INFORMATION));

    status = RtlInitUnicodeStringEx(
        &threadNameInfo.ThreadName,
        ThreadName
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadNameInformation,
        &threadNameInfo,
        sizeof(THREAD_NAME_INFORMATION)
        );

    return status;
}

/**
 * Gets a thread's affinity mask.
 *
 * \param[in] ThreadHandle A handle to a thread.
 * \param[out] AffinityMask The affinity mask.
 * \return NTSTATUS Successful or errant status.
 * \remarks The handle must have THREAD_SET_LIMITED_INFORMATION access.
 */
NTSTATUS PhGetThreadAffinityMask(
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

/**
 * Sets a thread's affinity mask.
 *
 * \param[in] ThreadHandle A handle to a thread.
 * \param[in] AffinityMask The new affinity mask.
 * \return NTSTATUS Successful or errant status.
 * \remarks The handle must have THREAD_SET_LIMITED_INFORMATION access.
 */
NTSTATUS PhSetThreadAffinityMask(
    _In_ HANDLE ThreadHandle,
    _In_ KAFFINITY AffinityMask
    )
{
    NTSTATUS status;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadAffinityMask,
        &AffinityMask,
        sizeof(KAFFINITY)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadAffinityMask,
            &AffinityMask,
            sizeof(KAFFINITY)
            );
    }

    return status;
}

/**
 * Sets thread base priority using a CLIENT_ID (system-wide).
 *
 * \param[in] ClientId Target thread identifier.
 * \param[in] Increment New base priority value.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetThreadBasePriorityClientId(
    _In_ CLIENT_ID ClientId,
    _In_ KPRIORITY Increment
    )
{
    NTSTATUS status;
    SYSTEM_THREAD_CID_PRIORITY_INFORMATION threadInfo;

    threadInfo.ClientId = ClientId;
    threadInfo.Priority = Increment;

    status = NtSetSystemInformation(
        SystemThreadPriorityClientIdInformation,
        &threadInfo,
        sizeof(SYSTEM_THREAD_CID_PRIORITY_INFORMATION)
        );

    if (status == STATUS_PENDING)
        status = STATUS_SUCCESS;

    return status;
}

/**
 * Sets a thread's base priority.
 *
 * \param[in] ThreadHandle Handle to thread.
 * \param[in] Increment New priority value (KPRIORITY).
 * \returns NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetThreadBasePriority(
    _In_ HANDLE ThreadHandle,
    _In_ KPRIORITY Increment
    )
{
    NTSTATUS status;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadBasePriority,
        &Increment,
        sizeof(KPRIORITY)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadBasePriority,
            &Increment,
            sizeof(KPRIORITY)
            );
    }

    return status;
}

/**
 * Sets a thread's I/O priority.
 *
 * \param[in] ThreadHandle A handle to a thread.
 * \param[in] IoPriority The new I/O priority.
 * \return NTSTATUS Successful or errant status.
 * \remarks The handle must have THREAD_SET_LIMITED_INFORMATION access.
 */
NTSTATUS PhSetThreadIoPriority(
    _In_ HANDLE ThreadHandle,
    _In_ IO_PRIORITY_HINT IoPriority
    )
{
    NTSTATUS status;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadIoPriority,
        &IoPriority,
        sizeof(IO_PRIORITY_HINT)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadIoPriority,
            &IoPriority,
            sizeof(IO_PRIORITY_HINT)
            );
    }

    return status;
}

/**
 * Sets a thread's page priority.
 *
 * \param[in] ThreadHandle Thread handle.
 * \param[in] PagePriority New page priority value (1..5 typical).
 * \returns NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetThreadPagePriority(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG PagePriority
    )
{
    NTSTATUS status;
    PAGE_PRIORITY_INFORMATION pagePriorityInfo;

    pagePriorityInfo.PagePriority = PagePriority;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadPagePriority,
        &pagePriorityInfo,
        sizeof(PAGE_PRIORITY_INFORMATION)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadPagePriority,
            &pagePriorityInfo,
            sizeof(PAGE_PRIORITY_INFORMATION)
            );
    }

    return status;
}

/**
 * Enables or disables priority boost for a thread.
 *
 * \param[in] ThreadHandle Thread handle.
 * \param[in] DisablePriorityBoost TRUE to disable boost, FALSE to enable.
 * \returns NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetThreadPriorityBoost(
    _In_ HANDLE ThreadHandle,
    _In_ BOOLEAN DisablePriorityBoost
    )
{
    NTSTATUS status;
    ULONG priorityBoost;

    priorityBoost = DisablePriorityBoost ? 1 : 0;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadPriorityBoost,
        &priorityBoost,
        sizeof(ULONG)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadPriorityBoost,
            &priorityBoost,
            sizeof(ULONG)
            );
    }

    return status;
}

/**
 * Retrieves the current power throttling state applied to the specified thread.
 *
 * \param[in] ThreadHandle Handle to the thread whose power throttling state is to be queried.
 * \param[out] PowerThrottlingState Pointer to a POWER_THROTTLING_THREAD_STATE structure that receives the throttling state.
 * \returns NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadPowerThrottlingState(
    _In_ HANDLE ThreadHandle,
    _Out_ PPOWER_THROTTLING_THREAD_STATE PowerThrottlingState
    )
{
    NTSTATUS status;
    POWER_THROTTLING_THREAD_STATE threadPowerThrottlingState;

    memset(&threadPowerThrottlingState, 0, sizeof(POWER_THROTTLING_THREAD_STATE));
    threadPowerThrottlingState.Version = POWER_THROTTLING_THREAD_CURRENT_VERSION;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadPowerThrottlingState,
        &threadPowerThrottlingState,
        sizeof(POWER_THROTTLING_THREAD_STATE),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *PowerThrottlingState = threadPowerThrottlingState;
    }

    return status;
}

/**
 * Sets a thread's ideal processor (ThreadIdealProcessorEx).
 *
 * \param[in] ULONG Thread handle.
 * \param[in] ProcessorNumber Desired processor number/group.
 * \param[out] PreviousIdealProcessor Receives previous value if provided.
 * \returns NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetThreadIdealProcessor(
    _In_ HANDLE ThreadHandle,
    _In_ PPROCESSOR_NUMBER ProcessorNumber,
    _Out_opt_ PPROCESSOR_NUMBER PreviousIdealProcessor
    )
{
    NTSTATUS status;
    PROCESSOR_NUMBER processorNumber;

    processorNumber = *ProcessorNumber;
    status = NtSetInformationThread(
        ThreadHandle,
        ThreadIdealProcessorEx,
        &processorNumber,
        sizeof(PROCESSOR_NUMBER)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadIdealProcessorEx,
            &processorNumber,
            sizeof(PROCESSOR_NUMBER)
            );
    }

    if (PreviousIdealProcessor)
        *PreviousIdealProcessor = processorNumber;

    return status;
}

/**
 * Sets a thread's group affinity.
 *
 * \param[in] ThreadHandle Thread handle.
 * \param[in] GroupAffinity New affinity data.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetThreadGroupAffinity(
    _In_ HANDLE ThreadHandle,
    _In_ GROUP_AFFINITY GroupAffinity
    )
{
    NTSTATUS status;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadGroupInformation,
        &GroupAffinity,
        sizeof(GROUP_AFFINITY)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadGroupInformation,
            &GroupAffinity,
            sizeof(GROUP_AFFINITY)
            );
    }

    return status;
}

/**
 * The PhGetThreadLastSystemCall function returns the last system call of a thread.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \param[out] LastSystemCall The last system call of the thread.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadLastSystemCall(
    _In_ HANDLE ThreadHandle,
    _Out_ PTHREAD_LAST_SYSCALL_INFORMATION LastSystemCall
    )
{
    if (WindowsVersion < WINDOWS_8)
    {
        return NtQueryInformationThread(
            ThreadHandle,
            ThreadLastSystemCall,
            LastSystemCall,
            FIELD_OFFSET(THREAD_LAST_SYSCALL_INFORMATION, WaitTime),
            NULL
            );
    }

    return NtQueryInformationThread(
        ThreadHandle,
        ThreadLastSystemCall,
        LastSystemCall,
        sizeof(THREAD_LAST_SYSCALL_INFORMATION),
        NULL
        );
}

// rev from Advapi32!ImpersonateAnonymousToken (dmex)
/**
 * The PhCreateImpersonationToken function creates an anonymous logon token.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \param[out] TokenHandle A handle to the token.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateImpersonationToken(
    _In_ HANDLE ThreadHandle,
    _Out_ PHANDLE TokenHandle
    )
{
    NTSTATUS status;
    HANDLE tokenHandle;
    SECURITY_QUALITY_OF_SERVICE securityService;

    status = PhRevertImpersonationToken(ThreadHandle);

    if (!NT_SUCCESS(status))
        return status;

    securityService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    securityService.ImpersonationLevel = SecurityImpersonation;
    securityService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    securityService.EffectiveOnly = FALSE;

    status = NtImpersonateThread(
        ThreadHandle,
        ThreadHandle,
        &securityService
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhOpenThreadToken(
        ThreadHandle,
        TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
        FALSE,
        &tokenHandle
        );

    if (NT_SUCCESS(status))
    {
        *TokenHandle = tokenHandle;
    }

    return status;
}

// rev from Advapi32!ImpersonateLoggedOnUser (dmex)
/**
 * The PhImpersonateToken function enables the specified thread to impersonate the security context of a token.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \param[in] TokenHandle A handle to the token.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhImpersonateToken(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE TokenHandle
    )
{
    NTSTATUS status;
    TOKEN_TYPE tokenType;
    ULONG returnLength;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenType,
        &tokenType,
        sizeof(TOKEN_TYPE),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (tokenType == TokenPrimary)
    {
        SECURITY_QUALITY_OF_SERVICE securityService;
        OBJECT_ATTRIBUTES objectAttributes;
        HANDLE tokenHandle;

        memset(&securityService, 0, sizeof(SECURITY_QUALITY_OF_SERVICE));
        securityService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        securityService.ImpersonationLevel = SecurityImpersonation;
        securityService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        securityService.EffectiveOnly = FALSE;

        InitializeObjectAttributesEx(
            &objectAttributes,
            NULL,
            OBJ_EXCLUSIVE,
            NULL,
            NULL,
            &securityService
            );

        status = NtDuplicateToken(
            TokenHandle,
            TOKEN_IMPERSONATE | TOKEN_QUERY,
            &objectAttributes,
            FALSE,
            TokenImpersonation,
            &tokenHandle
            );

        if (!NT_SUCCESS(status))
            return status;

        status = NtSetInformationThread(
            ThreadHandle,
            ThreadImpersonationToken,
            &tokenHandle,
            sizeof(HANDLE)
            );

        NtClose(tokenHandle);
    }
    else
    {
        status = NtSetInformationThread(
            ThreadHandle,
            ThreadImpersonationToken,
            &TokenHandle,
            sizeof(HANDLE)
            );
    }

    return status;
}

// rev from Advapi32!RevertToSelf (dmex)
/**
 * The PhRevertImpersonationToken function terminates the impersonation of a security context.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhRevertImpersonationToken(
    _In_ HANDLE ThreadHandle
    )
{
    HANDLE tokenHandle = NULL;

    return NtSetInformationThread(
        ThreadHandle,
        ThreadImpersonationToken,
        &tokenHandle,
        sizeof(HANDLE)
        );
}

/**
 * Retrieves the last error status of a thread.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \param[in] ProcessHandle A handle to the process.
 * \param[out] LastStatusValue The last status of the thread.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadLastStatusValue(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PNTSTATUS LastStatusValue
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, LastStatusValue)),
            LastStatusValue,
            sizeof(NTSTATUS),
            NULL
            );
    }
    else
#endif
    {
        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, LastStatusValue)), // LastErrorValue/ExceptionCode
            LastStatusValue,
            sizeof(NTSTATUS),
            NULL
            );
    }

    return status;
}

/**
 * Retrieves statistics about COM multi-threaded apartment (MTA) usage in a process.
 *
 * \param[in] ProcessHandle A handle to the process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param[out] MTAInits The total number of MTA references in the process.
 * \param[out] MTAIncInits The number of MTA references from CoIncrementMTAUsage.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessMTAUsage(
    _In_ HANDLE ProcessHandle,
    _Out_opt_ PULONG MTAInits,
    _Out_opt_ PULONG MTAIncInits
    )
{
    NTSTATUS status;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PMTA_USAGE_GLOBALS mtaUsageGlobals = NULL;

    if (!MTAInits && !MTAIncInits)
        return STATUS_INVALID_PARAMETER;

#ifdef _WIN64
    if (!NT_SUCCESS(status = PhGetProcessIsWow64(ProcessHandle, &isWow64)))
        return status;

    if (isWow64)
        return STATUS_NOT_SUPPORTED;
#endif

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion >= WINDOWS_8)
        {
            PVOID combase;
            PMTA_USAGE_GLOBALS (WINAPI* CoGetMTAUsageInfo_I)(VOID);

            if (combase = PhGetLoaderEntryDllBaseZ(L"combase.dll"))
            {
                // combase exports CoGetMTAUsageInfo as ordinal 70
                CoGetMTAUsageInfo_I = PhGetDllBaseProcedureAddress(combase, NULL, 70);

                if (CoGetMTAUsageInfo_I)
                {
                    // CoGetMTAUsageInfo returns addresses of several global variables we can read
                    mtaUsageGlobals = CoGetMTAUsageInfo_I();
                }
            }
        }

        PhEndInitOnce(&initOnce);
    }

    if (!mtaUsageGlobals)
        return STATUS_UNSUCCESSFUL;

    if (MTAInits)
    {
        status = PhReadVirtualMemory(
            ProcessHandle,
            mtaUsageGlobals->MTAInits,
            MTAInits,
            sizeof(ULONG),
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;
    }

    if (MTAIncInits)
    {
        status = PhReadVirtualMemory(
            ProcessHandle,
            mtaUsageGlobals->MTAIncInits,
            MTAIncInits,
            sizeof(ULONG),
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;
    }

    return STATUS_SUCCESS;
}

/**
 * Retrieves COM apartment flags and init count of a thread.
 *
 * \param[in] ThreadHandle A handle to the thread. The handle must have
 * THREAD_QUERY_LIMITED_INFORMATION access.
 * \param[in] ProcessHandle A handle to the process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param[out] ApartmentFlags The COM apartment flags of the thread.
 * \param[out] ComInits The number of times the thread initialized COM.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadApartmentFlags(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ApartmentFlags,
    _Out_opt_ PULONG ComInits
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    PVOID apartmentStateOffset;
    PVOID oletlsBaseAddress;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    if (!NT_SUCCESS(status = PhGetProcessIsWow64(ProcessHandle, &isWow64)))
        return status;

    if (isWow64)
    {
        ULONG oletlsDataAddress32 = 0;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, ReservedForOle)),
            &oletlsDataAddress32,
            sizeof(ULONG),
            NULL
            );

        oletlsBaseAddress = UlongToPtr(oletlsDataAddress32);
    }
    else
#endif
    {
        ULONG_PTR oletlsDataAddress = 0;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, ReservedForOle)),
            &oletlsDataAddress,
            sizeof(ULONG_PTR),
            NULL
            );

        oletlsBaseAddress = (PVOID)oletlsDataAddress;
    }

    if (!NT_SUCCESS(status))
        return status;

    if (!oletlsBaseAddress)
    {
        // Return a special error to indicate that we successfully determined
        // that the thread has no associated COM state. (diversenok)
        return NTSTATUS_FROM_WIN32(CO_E_NOTINITIALIZED);
    }

#ifdef _WIN64
    if (isWow64)
        apartmentStateOffset = PTR_ADD_OFFSET(oletlsBaseAddress, UFIELD_OFFSET(SOleTlsData32, Flags));
    else
        apartmentStateOffset = PTR_ADD_OFFSET(oletlsBaseAddress, UFIELD_OFFSET(SOleTlsData, Flags));
#else
    apartmentStateOffset = PTR_ADD_OFFSET(oletlsBaseAddress, UFIELD_OFFSET(SOleTlsData, Flags));
#endif

    status = PhReadVirtualMemory(
        ProcessHandle,
        apartmentStateOffset,
        ApartmentFlags,
        sizeof(ULONG),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (ComInits)
    {
        PVOID comInitsOffset;

#ifdef _WIN64
        if (isWow64)
            comInitsOffset = PTR_ADD_OFFSET(oletlsBaseAddress, UFIELD_OFFSET(SOleTlsData32, ComInits));
        else
            comInitsOffset = PTR_ADD_OFFSET(oletlsBaseAddress, UFIELD_OFFSET(SOleTlsData, ComInits));
#else
        comInitsOffset = PTR_ADD_OFFSET(oletlsBaseAddress, UFIELD_OFFSET(SOleTlsData, ComInits));
#endif

        status = PhReadVirtualMemory(
            ProcessHandle,
            comInitsOffset,
            ComInits,
            sizeof(ULONG),
            NULL
            );
    }

    return status;
}

/**
 * Determines COM apartment type of a thread, similar to CoGetApartmentType.
 *
 * \param[in] ThreadHandle A handle to the thread. The handle must have
 * THREAD_QUERY_LIMITED_INFORMATION access.
 * \param[in] ProcessHandle A handle to the process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param[out] ApartmentInfo The COM apartment information of the thread.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadApartment(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_APARTMENT_INFO ApartmentInfo
    )
{
    NTSTATUS status;
    PH_APARTMENT_INFO info = { 0 };

    //
    // N.B. Most information about the thread's apartment comes from OLE TLS data in TEB.
    // Without it, threads can still implicitly belong to the multi-threaded apartment (MTA)
    // as long as one exists in the process. (diversenok)
    //

    // Read OLE TLS flags
    status = PhGetThreadApartmentFlags(ThreadHandle, ProcessHandle, &info.Flags, &info.ComInits);

    if (status == NTSTATUS_FROM_WIN32(CO_E_NOTINITIALIZED))
    {
        // For our purposes, no OLE TLS data is equivalent to empty flags
        info.Flags = 0;
        info.ComInits = 0;
        status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status))
        return status;

    if (info.Flags & OLETLS_APARTMENTTHREADED)
    {
        //
        // N.B. Single-threaded apartments (STAs) belong to one of the three sub-types:
        //  - Main STA: the first (classic) STA created in the process. It has the responsibility of hosting
        //    all components with no ThreadingModel and ThreadingModel=Single (which are equivalent).
        //  - Classic STA: a reentrant single-threaded apartment, usually referred to as just STA.
        //  - Application STA (ASTA): a non-reentrant single-threaded apartment used primarily by WinRT.
        //

        if (info.Flags & OLETLS_APPLICATION_STA)
        {
            // The non-reentrancy requirement of ASTA means it cannot serve as the main STA
            info.Type = PH_APARTMENT_TYPE_APPLICATION_STA;
        }
        else
        {
            THREAD_BASIC_INFORMATION basicInfo;
            BOOLEAN isMainSta = FALSE;

            //
            // N.B. There is no flag to distinguish between main and non-main classic STAs.
            // Internally, CoGetApartmentType compares the caller's thread ID to the thread ID
            // stored in a private global variable (which we cannot access). Instead, we can
            // check if the specified thread owns the main STA window - a message-only window
            // with a known class and name. (diversenok)
            //

            if (NT_SUCCESS(PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
            {

                CLIENT_ID clientId;
                HWND hwnd = NULL;

                do
                {
                    // Find the next main STA window
                    hwnd = FindWindowExW(
                        HWND_MESSAGE,
                        hwnd,
                        L"OleMainThreadWndClass",
                        L"OleMainThreadWndName"
                        );

                    // Check if it belongs to the specified thread ID
                } while (hwnd && NT_SUCCESS(PhGetWindowClientId(hwnd, &clientId)) &&
                    (clientId.UniqueProcess != basicInfo.ClientId.UniqueProcess ||
                    clientId.UniqueThread != basicInfo.ClientId.UniqueThread));

                isMainSta = !!hwnd;
            }

            info.Type = isMainSta ? PH_APARTMENT_TYPE_MAIN_STA : PH_APARTMENT_TYPE_STA;
        }
    }
    else if (info.Flags & (OLETLS_MULTITHREADED | OLETLS_DISPATCHTHREAD))
    {
        // CoGetApartmentType treats explicit MTA threads and dispatch threads equally
        info.Type = PH_APARTMENT_TYPE_MTA;
    }
    else
    {
        //
        // N.B. The thread lacks an explicit apartment. A single MTA init, however, is
        // enough to put all apartmentless threads into implicit MTA. The existence of MTA
        // can be checked by reading the process-wide MTA usage counter. (diversenok)
        //

        if (!NT_SUCCESS(status = PhGetProcessMTAUsage(ProcessHandle, &info.ComInits, NULL)))
            return status;

        if (info.ComInits > 0)
            info.Type = PH_APARTMENT_TYPE_IMPLICIT_MTA;
        else
            return NTSTATUS_FROM_WIN32(CO_E_NOTINITIALIZED);
    }

    //
    // N.B. Threads can temporarily enter the neutral apartment on top of their existing apartment.
    // Neutral apartment is often abbreviated to NA, NTA, or TNA. (diversenok)
    //

    info.InNeutral = !!(info.Flags & OLETLS_INNEUTRALAPT);

    *ApartmentInfo = info;
    return STATUS_SUCCESS;
}

// rev from advapi32!WctGetCOMInfo (dmex)
/**
 * If a thread is blocked on a COM call, we can retrieve COM ownership information using these functions.
 * Retrieves COM information when a thread is blocked on a COM call.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \param[in] ProcessHandle A handle to a process.
 * \param[out] ApartmentCallState The COM call information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadApartmentCallState(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_COM_CALLSTATE ApartmentCallState
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif
    PVOID oletlsBaseAddress = NULL;

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        ULONG oletlsDataAddress32 = 0;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, ReservedForOle)),
            &oletlsDataAddress32,
            sizeof(ULONG),
            NULL
            );

        oletlsBaseAddress = UlongToPtr(oletlsDataAddress32);
    }
    else
#endif
    {
        ULONG_PTR oletlsDataAddress = 0;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, ReservedForOle)),
            &oletlsDataAddress,
            sizeof(ULONG_PTR),
            NULL
            );

        oletlsBaseAddress = (PVOID)oletlsDataAddress;
    }

    if (NT_SUCCESS(status) && oletlsBaseAddress)
    {
        typedef enum _CALL_STATE_TYPE
        {
            CALL_STATE_TYPE_OUTGOING, // tagOutgoingCallData
            CALL_STATE_TYPE_INCOMING, // tagIncomingCallData
            CALL_STATE_TYPE_ACTIVATION // tagOutgoingActivationData
        } CALL_STATE_TYPE;
        typedef struct tagOutgoingCallData // private
        {
            ULONG dwServerPID;
            ULONG dwServerTID;
        } tagOutgoingCallData, *PtagOutgoingCallData;
        typedef struct tagIncomingCallData // private
        {
            ULONG dwClientPID;
        } tagIncomingCallData, *PtagIncomingCallData;
        typedef struct tagOutgoingActivationData // private
        {
            GUID guidServer;
        } tagOutgoingActivationData, *PtagOutgoingActivationData;
        static HRESULT (WINAPI* CoGetCallState_I)( // rev
            _In_ CALL_STATE_TYPE Type,
            _Out_ PULONG OffSet
            ) = NULL;
        //static HRESULT (WINAPI* CoGetActivationState_I)( // rev
        //    _In_ LPCLSID Clsid,
        //    _In_ ULONG ClientTid,
        //    _Out_ PULONG ServerPid
        //    ) = NULL;
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        ULONG outgoingCallDataOffset = 0;
        ULONG incomingCallDataOffset = 0;
        ULONG outgoingActivationDataOffset = 0;
        tagOutgoingCallData outgoingCallData = { 0 };
        tagIncomingCallData incomingCallData = { 0 };
        tagOutgoingActivationData outgoingActivationData = { 0 };

        if (PhBeginInitOnce(&initOnce))
        {
            PVOID baseAddress;

            if (baseAddress = PhGetLoaderEntryDllBaseZ(L"combase.dll"))
            {
                CoGetCallState_I = PhGetDllBaseProcedureAddress(baseAddress, "CoGetCallState", 0);
                //CoGetActivationState_I = PhGetDllBaseProcedureAddress(baseAddress, "CoGetActivationState", 0);
            }

            PhEndInitOnce(&initOnce);
        }

        if (HR_SUCCESS(CoGetCallState_I(CALL_STATE_TYPE_OUTGOING, &outgoingCallDataOffset)) && outgoingCallDataOffset)
        {
            PhReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(oletlsBaseAddress, outgoingCallDataOffset),
                &outgoingCallData,
                sizeof(tagOutgoingCallData),
                NULL
                );
        }

        if (HR_SUCCESS(CoGetCallState_I(CALL_STATE_TYPE_INCOMING, &incomingCallDataOffset)) && incomingCallDataOffset)
        {
            PhReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(oletlsBaseAddress, incomingCallDataOffset),
                &incomingCallData,
                sizeof(tagIncomingCallData),
                NULL
                );
        }

        if (HR_SUCCESS(CoGetCallState_I(CALL_STATE_TYPE_ACTIVATION, &outgoingActivationDataOffset)) && outgoingActivationDataOffset)
        {
            PhReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(oletlsBaseAddress, outgoingActivationDataOffset),
                &outgoingActivationData,
                sizeof(tagOutgoingActivationData),
                NULL
                );
        }

        memset(ApartmentCallState, 0, sizeof(PH_COM_CALLSTATE));
        ApartmentCallState->ServerPID = outgoingCallData.dwServerPID != 0 ? outgoingCallData.dwServerPID : ULONG_MAX;
        ApartmentCallState->ServerTID = outgoingCallData.dwServerTID != 0 ? outgoingCallData.dwServerTID : ULONG_MAX;
        ApartmentCallState->ClientPID = incomingCallData.dwClientPID != 0 ? incomingCallData.dwClientPID : ULONG_MAX;
        memcpy(&ApartmentCallState->ServerGuid, &outgoingActivationData.guidServer, sizeof(GUID));
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

/**
 * Determines if a thread has an associated RPC state.
 *
 * \param[in] ThreadHandle A handle to the thread. The handle must have
 * THREAD_QUERY_LIMITED_INFORMATION access.
 * \param[in] ProcessHandle A handle to the process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param[out] HasRpcState Whether the thread has allocated RPC state.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadRpcState(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN HasRpcState
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    if (!NT_SUCCESS(status = PhGetProcessIsWow64(ProcessHandle, &isWow64)))
        return status;

    if (isWow64)
    {
        typeof(RTL_FIELD_TYPE(TEB32, ReservedForNtRpc)) reservedForNtRpc32 = 0;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, ReservedForNtRpc)),
            &reservedForNtRpc32,
            sizeof(RTL_FIELD_TYPE(TEB32, ReservedForNtRpc)),
            NULL
            );

        *HasRpcState = !!reservedForNtRpc32;
    }
    else
#endif
    {
        typeof(RTL_FIELD_TYPE(TEB, ReservedForNtRpc)) reservedForNtRpc = NULL;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, ReservedForNtRpc)),
            &reservedForNtRpc,
            sizeof(RTL_FIELD_TYPE(TEB, ReservedForNtRpc)),
            NULL
            );

        *HasRpcState = !!reservedForNtRpc;
    }

    return status;
}

// rev from advapi32!WctGetCritSecInfo (dmex)
/**
 * Retrieves the thread identifier when a thread is blocked on a critical section.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \param[in] ProcessId The ID of a process.
 * \param[out] ThreadId The ID of the thread owning the critical section.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadCriticalSectionOwnerThread(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessId,
    _Out_ PULONG ThreadId
    )
{
    NTSTATUS status;
    PRTL_DEBUG_INFORMATION debugBuffer;

    if (WindowsVersion < WINDOWS_11)
        return STATUS_UNSUCCESSFUL;

    if (!(debugBuffer = RtlCreateQueryDebugBuffer(0, FALSE)))
        return STATUS_UNSUCCESSFUL;

    debugBuffer->CriticalSectionOwnerThread = ThreadHandle;

    status = RtlQueryProcessDebugInformation(
        ProcessId,
        RTL_QUERY_PROCESS_NONINVASIVE_CS_OWNER, // TODO: RTL_QUERY_PROCESS_CS_OWNER (dmex)
        debugBuffer
        );

    if (!NT_SUCCESS(status))
    {
        RtlDestroyQueryDebugBuffer(debugBuffer);
        return status;
    }

    if (!debugBuffer->Reserved[0])
    {
        RtlDestroyQueryDebugBuffer(debugBuffer);
        return STATUS_UNSUCCESSFUL;
    }

    *ThreadId = PtrToUlong(debugBuffer->Reserved[0]);

    RtlDestroyQueryDebugBuffer(debugBuffer);

    return STATUS_SUCCESS;
}

// rev from advapi32!WctGetSocketInfo (dmex)
/**
 * Retrieves the connection state when a thread is blocked on a socket.
 *
 * \param[in] ThreadHandle A handle to the thread.
 * \param[in] ProcessHandle A handle to a process.
 * \param[out] ThreadSocketState The state of the socket.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadSocketState(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_THREAD_SOCKET_STATE ThreadSocketState
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif
    typeof(RTL_FIELD_TYPE(TEB, WinSockData)) winsockHandleAddress = NULL;

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        typeof(RTL_FIELD_TYPE(TEB32, WinSockData)) winsockDataAddress = 0;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, WinSockData)),
            &winsockDataAddress,
            sizeof(RTL_FIELD_TYPE(TEB32, WinSockData)),
            NULL
            );

        winsockHandleAddress = UlongToHandle(winsockDataAddress);
    }
    else
#endif
    {
        typeof(RTL_FIELD_TYPE(TEB, WinSockData)) winsockDataAddress = NULL;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, WinSockData)),
            &winsockDataAddress,
            sizeof(RTL_FIELD_TYPE(TEB, WinSockData)),
            NULL
            );

        winsockHandleAddress = winsockDataAddress;
    }

    if (NT_SUCCESS(status) && winsockHandleAddress)
    {
#if defined(PHLIB_SOCKET_STATE_WINSOCK)
        static LONG (WINAPI* LPFN_WSASTARTUP)(
            _In_ WORD wVersionRequested,
            _Out_ PVOID* lpWSAData
            );
        static LONG (WINAPI* LPFN_GETSOCKOPT)(
            _In_ UINT_PTR s,
            _In_ LONG level,
            _In_ LONG optname,
            _Out_writes_bytes_(*optlen) char FAR* optval,
            _Inout_ LONG FAR* optlen
            );
        static LONG (WINAPI* LPFN_CLOSESOCKET)(
            _In_ ULONG_PTR s
            );
        static LONG (WINAPI* LPFN_WSACLEANUP)(
            void
            );
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        #ifndef WINSOCK_VERSION
        #define WINSOCK_VERSION MAKEWORD(2,2)
        #endif
        #ifndef SOCKET_ERROR
        #define SOCKET_ERROR (-1)
        #endif
        #ifndef SOL_SOCKET
        #define SOL_SOCKET 0xffff
        #endif
        #ifndef SO_BSP_STATE
        #define SO_BSP_STATE 0x1009
        #endif
        typedef struct _SOCKET_ADDRESS
        {
            _Field_size_bytes_(iSockaddrLength) PVOID lpSockaddr;
            // _When_(lpSockaddr->sa_family == AF_INET, _Field_range_(>=, sizeof(SOCKADDR_IN)))
            // _When_(lpSockaddr->sa_family == AF_INET6, _Field_range_(>=, sizeof(SOCKADDR_IN6)))
            LONG iSockaddrLength;
        } SOCKET_ADDRESS, *PSOCKET_ADDRESS, *LPSOCKET_ADDRESS;
        typedef struct _CSADDR_INFO
        {
            SOCKET_ADDRESS LocalAddr;
            SOCKET_ADDRESS RemoteAddr;
            LONG iSocketType;
            LONG iProtocol;
        } CSADDR_INFO, *PCSADDR_INFO, FAR* LPCSADDR_INFO;
        PVOID wsaStartupData;
#endif
        HANDLE winsockTargetHandle;

#if defined(PHLIB_SOCKET_STATE_WINSOCK)
        if (PhBeginInitOnce(&initOnce))
        {
            PVOID baseAddress;

            if (baseAddress = PhLoadLibrary(L"ws2_32.dll"))
            {
                LPFN_WSASTARTUP = PhGetDllBaseProcedureAddress(baseAddress, "WSAStartup", 0);
                LPFN_GETSOCKOPT = PhGetDllBaseProcedureAddress(baseAddress, "getsockopt", 0);
                //LPFN_GETSOCKNAME = PhGetDllBaseProcedureAddress(baseAddress, "getsockname", 0);
                //LPFN_GETPEERNAME = PhGetDllBaseProcedureAddress(baseAddress, "getpeername", 0);
                LPFN_CLOSESOCKET = PhGetDllBaseProcedureAddress(baseAddress, "closesocket", 0);
                LPFN_WSACLEANUP = PhGetDllBaseProcedureAddress(baseAddress, "WSACleanup", 0);
            }

            PhEndInitOnce(&initOnce);
        }

        if (LPFN_WSASTARTUP(WINSOCK_VERSION, &wsaStartupData) != 0)
        {
            status = STATUS_UNSUCCESSFUL;
            goto CleanupExit;
        }
#endif
        status = NtDuplicateObject(
            ProcessHandle,
            winsockHandleAddress,
            NtCurrentProcess(),
            &winsockTargetHandle,
            0,
            0,
            DUPLICATE_SAME_ACCESS
            );

        if (NT_SUCCESS(status))
        {
            OBJECT_BASIC_INFORMATION winsockTargetBasicInfo;

            status = PhGetObjectBasicInformation(
                ProcessHandle,
                winsockTargetHandle,
                &winsockTargetBasicInfo
                );

            if (NT_SUCCESS(status))
            {
                ULONG winsockAddressInfoLength = sizeof(CSADDR_INFO);
                CSADDR_INFO winsockAddressInfo;

                status = PhAfdQuerySocketOption(
                    winsockTargetHandle,
                    SOL_SOCKET,
                    SO_BSP_STATE,
                    &winsockAddressInfo,
                    winsockAddressInfoLength,
                    &winsockAddressInfoLength
                    );

                if (NT_SUCCESS(status))
                {
                    if (winsockAddressInfo.iProtocol == 6)
                    {
                        if (winsockAddressInfo.LocalAddr.lpSockaddr && winsockAddressInfo.RemoteAddr.lpSockaddr)
                            *ThreadSocketState = PH_THREAD_SOCKET_STATE_SHARED;
                        else
                            *ThreadSocketState = PH_THREAD_SOCKET_STATE_DISCONNECTED;
                    }
                    else
                    {
                        *ThreadSocketState = PH_THREAD_SOCKET_STATE_NOT_TCPIP;
                    }
                }

#if defined(PHLIB_SOCKET_STATE_WINSOCK)
                if (LPFN_GETSOCKOPT((UINT_PTR)winsockTargetHandle, SOL_SOCKET, SO_BSP_STATE, (PCHAR)&winsockAddressInfo, &winsockAddressInfoLength) != SOCKET_ERROR)
                {
                    if (winsockAddressInfo.iProtocol == 6)
                    {
                        if (winsockAddressInfo.LocalAddr.lpSockaddr && winsockAddressInfo.RemoteAddr.lpSockaddr)
                            *ThreadSocketState = PH_THREAD_SOCKET_STATE_SHARED;
                        else
                            *ThreadSocketState = PH_THREAD_SOCKET_STATE_DISCONNECTED;
                    }
                    else
                        *ThreadSocketState = PH_THREAD_SOCKET_STATE_NOT_TCPIP;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL; // WSAGetLastError();
                }
#endif
            }
#if defined(PHLIB_SOCKET_STATE_WINSOCK)
            LPFN_CLOSESOCKET((UINT_PTR)winsockTargetHandle);
#endif
            NtClose(winsockTargetHandle);
        }
#if defined(PHLIB_SOCKET_STATE_WINSOCK)
        LPFN_WSACLEANUP();
#endif
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

/**
 * Retrieves stack base/limit pointers for a thread.
 *
 * \param[in] ThreadHandle Thread handle.
 * \param[in] ProcessHandle Process handle.
 * \param[out] LowPart Receives stack base address.
 * \param[out] HighPart Receives stack limit address.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadStackLimits(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG_PTR LowPart,
    _Out_ PULONG_PTR HighPart
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    PVOID stackBaseAddress;
    PVOID stackLimitAddress;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        typeof(RTL_FIELD_TYPE(TEB32, NtTib)) ntTib32 = { 0 };

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, NtTib)),
            &ntTib32,
            sizeof(RTL_FIELD_TYPE(TEB32, NtTib)),
            NULL
            );

        stackBaseAddress = UlongToPtr(ntTib32.StackBase);
        stackLimitAddress = UlongToPtr(ntTib32.StackLimit);
    }
    else
#endif
    {
        typeof(RTL_FIELD_TYPE(TEB, NtTib)) ntTib = { 0 };

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, NtTib)),
            &ntTib,
            sizeof(RTL_FIELD_TYPE(TEB, NtTib)),
            NULL
            );

        stackBaseAddress = ntTib.StackBase;
        stackLimitAddress = ntTib.StackLimit;
    }

    if (NT_SUCCESS(status))
    {
        *LowPart = (ULONG_PTR)stackBaseAddress;
        *HighPart = (ULONG_PTR)stackLimitAddress;
    }

    return status;
}

/**
 * Computes stack usage and total reserved size for a thread.
 *
 * \param[in] ThreadHandle Thread handle.
 * \param[in] ProcessHandle Process handle.
 * \param[out] StackUsage Bytes currently committed (base - limit).
 * \param[out] StackLimit Total reserved size (base - allocation base).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadStackSize(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG_PTR StackUsage,
    _Out_ PULONG_PTR StackLimit
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    PVOID stackBaseAddress;
    PVOID stackLimitAddress;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        typeof(RTL_FIELD_TYPE(TEB32, NtTib)) ntTib32 = { 0 };

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, NtTib)),
            &ntTib32,
            sizeof(RTL_FIELD_TYPE(TEB32, NtTib)),
            NULL
            );

        stackBaseAddress = UlongToPtr(ntTib32.StackBase);
        stackLimitAddress = UlongToPtr(ntTib32.StackLimit);
    }
    else
#endif
    {
        typeof(RTL_FIELD_TYPE(TEB, NtTib)) ntTib = { 0 };

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, NtTib)),
            &ntTib,
            sizeof(RTL_FIELD_TYPE(TEB, NtTib)),
            NULL
            );

        stackBaseAddress = ntTib.StackBase;
        stackLimitAddress = ntTib.StackLimit;
    }

    if (NT_SUCCESS(status))
    {
        MEMORY_BASIC_INFORMATION memoryBasicInformation;

        memset(&memoryBasicInformation, 0, sizeof(MEMORY_BASIC_INFORMATION));

        status = NtQueryVirtualMemory(
            ProcessHandle,
            stackLimitAddress,
            MemoryBasicInformation,
            &memoryBasicInformation,
            sizeof(MEMORY_BASIC_INFORMATION),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            // TEB->DeallocationStack == memoryBasicInfo.AllocationBase
            *StackUsage = (ULONG_PTR)PTR_SUB_OFFSET(stackBaseAddress, stackLimitAddress);
            *StackLimit = (ULONG_PTR)PTR_SUB_OFFSET(stackBaseAddress, memoryBasicInformation.AllocationBase);
        }
    }

    return status;
}

/**
 * Determines whether a thread has fiber data.
 *
 * \param[in] ThreadHandle Thread handle.
 * \param[in] ProcessHandle Process handle.
 * \param[out] ThreadIsFiber TRUE if fiber data present.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetThreadIsFiber(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN ThreadIsFiber
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    BOOLEAN threadIsFiber;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        typeof(RTL_FIELD_TYPE(TEB32, SameTebFlags)) flags = 0;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, SameTebFlags)),
            &flags,
            sizeof(RTL_FIELD_TYPE(TEB32, SameTebFlags)),
            NULL
            );

        threadIsFiber = _bittest((LONG CONST*)&flags, 2); // HasFiberData offset (dmex)
    }
    else
#endif
    {
        typeof(RTL_FIELD_TYPE(TEB, SameTebFlags)) flags = 0;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, SameTebFlags)),
            &flags,
            sizeof(RTL_FIELD_TYPE(TEB, SameTebFlags)),
            NULL
            );

        threadIsFiber = _bittest((LONG CONST*)&flags, 2); // HasFiberData offset (dmex)
    }

    if (NT_SUCCESS(status))
    {
        *ThreadIsFiber = threadIsFiber;
    }

    return status;
}

// rev from SwitchToThread (dmex)
/**
 * Causes the calling thread to yield execution to another thread that is ready to run on the
 * current processor. The operating system selects the next thread to be executed.
 *
 * \remarks The operating system will not switch execution to another processor, even if that processor is idle or is running a thread of lower priority.
 * \return If calling the SwitchToThread function caused the operating system to switch execution to another thread, the return value is nonzero.
 * \rthere are no other threads ready to execute, the operating system does not switch execution to another thread, and the return value is zero.
 */
BOOLEAN PhSwitchToThread(
    VOID
    )
{
    LARGE_INTEGER interval = { 0 };

    return PhDelayExecutionEx(FALSE, &interval) != STATUS_NO_YIELD_PERFORMED;
}

/**
 * Determines runtime library path set (ntdll/kernel32/user32) for a process,
 * including WOW64/ARM32 variants.
 *
 * \param[in] ProcessHandle Process handle.
 * \param[out] RuntimeLibrary Receives pointer to predefined library set.
 * \param[out] IsWow64Process TRUE if target is WOW64/alternate architecture.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessRuntimeLibrary(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_RUNTIME_LIBRARY* RuntimeLibrary,
    _Out_opt_ PBOOLEAN IsWow64Process
    )
{
    static PH_PROCESS_RUNTIME_LIBRARY NativeRuntime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\user32.dll"),
    };
#ifdef _WIN64
    static PH_PROCESS_RUNTIME_LIBRARY Wow64Runtime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\user32.dll"),
    };
#ifdef _M_ARM64
    static PH_PROCESS_RUNTIME_LIBRARY Arm32Runtime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\user32.dll"),
    };
    static PH_PROCESS_RUNTIME_LIBRARY Chpe32Runtime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\user32.dll"),
    };
#endif
#endif

    *RuntimeLibrary = &NativeRuntime;

    if (IsWow64Process)
        *IsWow64Process = FALSE;

#ifdef _WIN64
    NTSTATUS status;
#ifdef _M_ARM64
    USHORT machine;

    status = PhGetProcessArchitecture(ProcessHandle, &machine);

    if (!NT_SUCCESS(status))
        return status;

    if (machine != IMAGE_FILE_MACHINE_TARGET_HOST)
    {
        switch (machine)
        {
        case IMAGE_FILE_MACHINE_I386:
        case IMAGE_FILE_MACHINE_CHPE_X86:
            {
                *RuntimeLibrary = &Chpe32Runtime;

                if (IsWow64Process)
                    *IsWow64Process = TRUE;
            }
            break;
        case IMAGE_FILE_MACHINE_ARMNT:
            {
                *RuntimeLibrary = &Arm32Runtime;

                if (IsWow64Process)
                    *IsWow64Process = TRUE;
            }
            break;
        case IMAGE_FILE_MACHINE_AMD64:
        case IMAGE_FILE_MACHINE_ARM64:
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }
    }
#else
    BOOLEAN isWow64 = FALSE;

    status = PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (!NT_SUCCESS(status))
        return status;

    if (isWow64)
    {
        *RuntimeLibrary = &Wow64Runtime;

        if (IsWow64Process)
            *IsWow64Process = TRUE;
    }
#endif
#endif

    return STATUS_SUCCESS;
}

/**
 * Loads the specified module into the process's address space using standard LoadLibraryW provided
 * by the operating system.
 *
 * \param[in] ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_CREATE_THREAD, PROCESS_VM_OPERATION, PROCESS_VM_READ
 * and PROCESS_VM_WRITE access.
 * \param[in] FileName The file name of the DLL to inject.
 * \param[in] Timeout The timeout, in milliseconds, for the process to load the DLL.
 * \return NTSTATUS Successful or errant status.
 * \remarks If the process does not load the DLL before the timeout expires it may crash. Choose the
 * timeout value carefully.
 */
NTSTATUS PhLoadDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF FileName,
    _In_opt_ ULONG Timeout
    )
{
    NTSTATUS status;
    PVOID fileNameBaseAddress = NULL;
    PVOID loadLibraryW = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    if (KphProcessLevel(ProcessHandle) > KphLevelMed)
    {
        return STATUS_ACCESS_DENIED;
    }

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "LoadLibraryW",
        &loadLibraryW,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhAllocateVirtualMemory(
        ProcessHandle,
        &fileNameBaseAddress,
        FileName->Length + sizeof(UNICODE_NULL),
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWriteVirtualMemory(
        ProcessHandle,
        fileNameBaseAddress,
        FileName->Buffer,
        FileName->Length + sizeof(UNICODE_NULL),
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        0,
        0,
        0,
        0,
        loadLibraryW,
        fileNameBaseAddress,
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, Timeout);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

CleanupExit:
    if (threadHandle)
        NtClose(threadHandle);

    if (powerRequestHandle)
        PhDestroyExecutionRequiredRequest(powerRequestHandle);

    if (fileNameBaseAddress)
        PhFreeVirtualMemory(ProcessHandle, fileNameBaseAddress, MEM_RELEASE);

    return status;
}

/**
 * Loads the specified module into the process's address space using standard Asynchronous Procedure Call (APC)
 * routines provided by the operating system.
 *
 * \param[in] ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_CREATE_THREAD, PROCESS_VM_OPERATION, PROCESS_VM_READ
 * and PROCESS_VM_WRITE access.
 * \param[in] FileName The file name of the DLL to inject.
 * \param[in]Timeout The timeout, in milliseconds, for the process to load the DLL.
 * \return NTSTATUS Successful or errant status.
 * \remarks If the process does not load the DLL before the timeout expires it may crash. Choose the
 * timeout value carefully.
 */
NTSTATUS PhLoadDllProcessApcThread(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF FileName,
    _In_opt_ ULONG Timeout
    )
{
    NTSTATUS status;
    PVOID fileNameBaseAddress = NULL;
    PVOID loadLibraryW = NULL;
    PVOID rtlExitUserThread = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    if (KphProcessLevel(ProcessHandle) > KphLevelMed)
    {
        return STATUS_ACCESS_DENIED;
    }

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "LoadLibraryW",
        &loadLibraryW,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhAllocateVirtualMemory(
        ProcessHandle,
        &fileNameBaseAddress,
        FileName->Length + sizeof(UNICODE_NULL),
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWriteVirtualMemory(
        ProcessHandle,
        fileNameBaseAddress,
        FileName->Buffer,
        FileName->Length + sizeof(UNICODE_NULL),
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtQueueApcThread(
        threadHandle,
        loadLibraryW,
        fileNameBaseAddress,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, Timeout);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

CleanupExit:
    if (threadHandle)
        NtClose(threadHandle);

    if (powerRequestHandle)
        PhDestroyExecutionRequiredRequest(powerRequestHandle);

    if (fileNameBaseAddress)
        PhFreeVirtualMemory(ProcessHandle, fileNameBaseAddress, MEM_RELEASE);

    return status;
}

/**
 * Causes a process to unload a DLL.
 *
 * \param[in] ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_CREATE_THREAD, PROCESS_VM_OPERATION, PROCESS_VM_READ
 * and PROCESS_VM_WRITE access.
 * \param[in]BaseAddress The base address of the DLL to unload.
 * \param[in] Timeout The timeout, in milliseconds, for the process to unload the DLL.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhUnloadDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_opt_ ULONG Timeout
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    HANDLE powerRequestHandle = NULL;
    THREAD_BASIC_INFORMATION basicInfo;
    PVOID freeLibrary = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // No point trying to set the load count on Windows 8 and higher, because NT now uses a DAG of
    // loader nodes. (wj32)
    if (WindowsVersion < WINDOWS_8)
    {
#ifdef _WIN64
        BOOLEAN isWow64 = FALSE;
#endif
        status = PhSetProcessModuleLoadCount(
            ProcessHandle,
            BaseAddress,
            1
            );

#ifdef _WIN64
        PhGetProcessIsWow64(ProcessHandle, &isWow64);

        if (isWow64 && status == STATUS_DLL_NOT_FOUND)
        {
            // The DLL might be 32-bit.
            status = PhSetProcessModuleLoadCount32(
                ProcessHandle,
                BaseAddress,
                1
                );
        }
#endif
        if (!NT_SUCCESS(status))
            return status;
    }

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "FreeLibrary",
        &freeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            return status;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        0,
        0,
        0,
        0,
        freeLibrary,
        BaseAddress,
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhWaitForSingleObject(threadHandle, Timeout);

    if (status == STATUS_WAIT_0)
    {
        status = PhGetThreadBasicInformation(threadHandle, &basicInfo);

        if (NT_SUCCESS(status))
            status = basicInfo.ExitStatus;
    }

    NtClose(threadHandle);

    if (powerRequestHandle)
        PhDestroyExecutionRequiredRequest(powerRequestHandle);

    return status;
}

/**
 * Sets an environment variable in a process.
 *
 * \param[in] ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_CREATE_THREAD, PROCESS_VM_OPERATION, PROCESS_VM_READ
 * and PROCESS_VM_WRITE access.
 * \param[in] Name The name of the environment variable to set.
 * \param[in] Value The new value of the environment variable. If this parameter is NULL, the
 * environment variable is deleted.
 * \param[in] Timeout The timeout, in milliseconds, for the process to set the environment variable.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetEnvironmentVariableRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF Name,
    _In_opt_ PPH_STRINGREF Value,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInformation;
    PVOID nameBaseAddress = NULL;
    PVOID valueBaseAddress = NULL;
    SIZE_T nameAllocationSize = 0;
    SIZE_T valueAllocationSize = 0;
    PVOID rtlExitUserThread = NULL;
    PVOID setEnvironmentVariableW = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    nameAllocationSize = Name->Length + sizeof(UNICODE_NULL);

    if (Value)
        valueAllocationSize = Value->Length + sizeof(UNICODE_NULL);

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        &isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "SetEnvironmentVariableW",
        &setEnvironmentVariableW,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhAllocateVirtualMemory(
        ProcessHandle,
        &nameBaseAddress,
        nameAllocationSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWriteVirtualMemory(
        ProcessHandle,
        nameBaseAddress,
        Name->Buffer,
        Name->Length,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (Value)
    {
        status = PhAllocateVirtualMemory(
            ProcessHandle,
            &valueBaseAddress,
            valueAllocationSize,
            MEM_COMMIT,
            PAGE_READWRITE
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhWriteVirtualMemory(
            ProcessHandle,
            valueBaseAddress,
            Value->Buffer,
            Value->Length,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            setEnvironmentVariableW,
            nameBaseAddress,
            valueBaseAddress,
            NULL
            );
    }
    else
    {
#endif
        status = NtQueueApcThread(
            threadHandle,
            setEnvironmentVariableW,
            nameBaseAddress,
            valueBaseAddress,
            NULL
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL); // Execute the pending APC (dmex)

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWaitForSingleObject(threadHandle, FALSE, Timeout);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetThreadBasicInformation(threadHandle, &basicInformation);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = basicInformation.ExitStatus;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    if (nameBaseAddress)
    {
        nameAllocationSize = 0;
        NtFreeVirtualMemory(
            ProcessHandle,
            &nameBaseAddress,
            &nameAllocationSize,
            MEM_RELEASE
            );
    }

    if (valueBaseAddress)
    {
        valueAllocationSize = 0;
        NtFreeVirtualMemory(
            ProcessHandle,
            &valueBaseAddress,
            &valueAllocationSize,
            MEM_RELEASE
            );
    }

    return status;
}

// based on https://www.drdobbs.com/a-safer-alternative-to-terminateprocess/184416547 (dmex)
/**
 * Safe process termination via remote RtlExitUserProcess in the context of the remote process.
 *
 * \param[in] ProcessHandle A handle to the process to be terminated.
 * \param[in] ExitStatus The exit status code to be used when terminating the process.
 * \param[in] Timeout Optional. The timeout, in milliseconds, to wait for the termination thread to complete.
 * If NULL, the function will wait indefinitely.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function attempts to terminate the specified process by creating a remote thread
 * that calls RtlExitUserProcess. On Windows 8 and later, it creates an execution required power request
 * to prevent deadlocks during the termination operation. The function cleans up any handles it creates before returning.
 */
NTSTATUS PhTerminateProcessAlternative(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus,
    _In_opt_ ULONG Timeout
    )
{
    NTSTATUS status;
    PVOID rtlExitUserProcess = NULL;
    HANDLE powerRequestHandle = NULL;
    HANDLE threadHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserProcess",
        &rtlExitUserProcess,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        0,
        0,
        0,
        0,
        rtlExitUserProcess,
        LongToPtr(ExitStatus),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, Timeout);

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

/**
 * Retrieves a copy of the system DLL init block for the process.
 *
 * \param[in] ProcessHandle A handle to the process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param[out] SystemDllInitBlock A buffer for a version-independent copy of LdrSystemDllInitBlock.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessSystemDllInitBlock(
    _In_ HANDLE ProcessHandle,
    _Out_ PPS_SYSTEM_DLL_INIT_BLOCK SystemDllInitBlock
    )
{
    NTSTATUS status;
    PS_SYSTEM_DLL_INIT_BLOCK systemDllInitBlock = { 0 };
    PVOID ldrSystemDllInitBlockAddress;
    ULONG expectedSize;

    // N.B. Aside from having three revisions, PS_SYSTEM_DLL_INIT_BLOCK
    // has different fields available on different OS versions. Determine
    // the maximum number of bytes we can read. (diversenok)

    if (WindowsVersion >= WINDOWS_11_24H2)
        expectedSize = sizeof(PS_SYSTEM_DLL_INIT_BLOCK_V3);
    else if (WindowsVersion >= PHNT_WINDOWS_10_20H1)
        expectedSize = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK_V3, MitigationAuditOptionsMap);
    else if (WindowsVersion >= PHNT_WINDOWS_10_RS3)
        expectedSize = sizeof(PS_SYSTEM_DLL_INIT_BLOCK_V2);
    else if (WindowsVersion >= PHNT_WINDOWS_10_RS2)
        expectedSize = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK_V2, Wow64CfgBitMapSize);
    else if (WindowsVersion >= PHNT_WINDOWS_10)
        expectedSize = sizeof(PS_SYSTEM_DLL_INIT_BLOCK_V1);
    else if (WindowsVersion >= PHNT_WINDOWS_8_1)
        expectedSize = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK_V1, CfgBitMapSize);
    else if (WindowsVersion >= PHNT_WINDOWS_8)
        expectedSize = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK_V1, MitigationOptions);
    else
        return STATUS_NOT_SUPPORTED;

    status = PhGetProcedureAddressRemoteZ(
        ProcessHandle,
        L"\\SystemRoot\\System32\\ntdll.dll",
        "LdrSystemDllInitBlock",
        &ldrSystemDllInitBlockAddress,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhReadVirtualMemory(
        ProcessHandle,
        ldrSystemDllInitBlockAddress,
        &systemDllInitBlock,
        expectedSize,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (systemDllInitBlock.Size > expectedSize)
        systemDllInitBlock.Size = expectedSize;

    status = PhCaptureSystemDllInitBlock(
        &systemDllInitBlock,
        SystemDllInitBlock
        );

    return status;
}

/**
 * Retrieves the CrossProcessFlags from the target process's PEB (Process Environment Block).
 *
 * \param[in] ProcessHandle Handle to the process whose cross-process flags are to be retrieved.
 * \param[in] ProcessFlags Pointer to a ULONG variable that receives the cross-process flags.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessCrossProcessFlags(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ProcessFlags
    )
{
    NTSTATUS status;
    ULONG crossProcessFlags = 0;
    PVOID pebBaseAddress;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;

    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB32, CrossProcessFlags)),
            &crossProcessFlags,
            sizeof(ULONG),
            NULL
            );
    }
    else
#endif
    {
        status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, CrossProcessFlags)),
            &crossProcessFlags,
            sizeof(ULONG),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        *ProcessFlags = crossProcessFlags;
    }

CleanupExit:
    return status;
}

/**
 * Retrieves the active ANSI code page of a process (PEB ActiveCodePage or ntdll variable).
 *
 * \param[in] ProcessHandle Process handle.
 * \param[out] ProcessCodePage Receives code page ID.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessCodePage(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT ProcessCodePage
    )
{
    NTSTATUS status;
    USHORT codePage = 0;

    if (WindowsVersion >= WINDOWS_11)
    {
        PVOID pebBaseAddress;
#ifdef _WIN64
        BOOLEAN isWow64 = FALSE;

        PhGetProcessIsWow64(ProcessHandle, &isWow64);

        if (isWow64)
        {
            status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB32, ActiveCodePage)),
                &codePage,
                sizeof(USHORT),
                NULL
                );
        }
        else
#endif
        {
            status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, ActiveCodePage)),
                &codePage,
                sizeof(USHORT),
                NULL
                );
        }
    }
    else
    {
        PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
        PVOID nlsAnsiCodePage;

        status = PhGetProcessRuntimeLibrary(
            ProcessHandle,
            &runtimeLibrary,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetProcedureAddressRemote(
            ProcessHandle,
            &runtimeLibrary->NtdllFileName,
            "NlsAnsiCodePage",
            &nlsAnsiCodePage,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhReadVirtualMemory(
            ProcessHandle,
            nlsAnsiCodePage,
            &codePage,
            sizeof(USHORT),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        *ProcessCodePage = codePage;
    }

CleanupExit:
    return status;
}

/**
 * Executes GetConsoleCP or GetConsoleOutputCP inside remote process and returns its code page.
 *
 * \param[in] ProcessHandle Target process.
 * \param[in] ConsoleOutputCP TRUE for output, FALSE for input code page.
 * \param[out] ConsoleCodePage Receives code page.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessConsoleCodePage(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN ConsoleOutputCP,
    _Out_ PUSHORT ConsoleCodePage
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInformation;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PVOID getConsoleCP = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        ConsoleOutputCP ? "GetConsoleOutputCP" : "GetConsoleCP",
        &getConsoleCP,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            return status;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        0,
        0,
        0,
        0,
        getConsoleCP,
        NULL,
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, 5000);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetThreadBasicInformation(threadHandle, &basicInformation);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    *ConsoleCodePage = (USHORT)basicInformation.ExitStatus;

CleanupExit:
    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

/**
 * Flushes remote process heaps by invoking RtlFlushHeaps through a queued APC.
 *
 * \param[in] ProcessHandle Target process.
 * \param[in] Timeout Optional timeout.
 * \return NTSTATUS Successful or errant status.
 * \remarks Uses suspended thread with queued APC; waits for completion (5s).
 */
NTSTATUS PhFlushProcessHeapsRemote(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInformation;
    PVOID rtlExitUserThread = NULL;
    PVOID rtlFlushHeaps = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        &isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlFlushHeaps",
        &rtlFlushHeaps,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            rtlFlushHeaps,
            NULL,
            NULL,
            NULL
            );
    }
    else
    {
#endif
        status = NtQueueApcThreadEx(
            threadHandle,
            QUEUE_USER_APC_SPECIAL_USER_APC,
            rtlFlushHeaps,
            NULL,
            NULL,
            NULL
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL); // Execute the pending APC (dmex)

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWaitForSingleObject(threadHandle, FALSE, Timeout);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetThreadBasicInformation(threadHandle, &basicInformation);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = basicInformation.ExitStatus;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

/**
 * Invokes a procedure in the context of the owning thread for the window.
 *
 * \param[in] WindowHandle The handle of the window.
 * \param[in] ApcRoutine The procedure to be invoked.
 * \param[in] ApcArgument1 The first argument to be passed to the procedure.
 * \param[in] ApcArgument2 The second argument to be passed to the procedure.
 * \param[in] ApcArgument3 The third argument to be passed to the procedure.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhInvokeWindowProcedureRemote(
    _In_ HWND WindowHandle,
    _In_ PPS_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    )
{
    NTSTATUS status;
    HANDLE processHande = NULL;
    HANDLE threadHande = NULL;
    HANDLE powerHandle = NULL;
    CLIENT_ID clientId;

    // Get the client ID of the window.

    status = PhGetWindowClientId(WindowHandle, &clientId);

    if (!NT_SUCCESS(status))
        return status;

    // Open the process associated with the window.

    status = PhOpenProcessClientId(
        &processHande,
        PROCESS_ALL_ACCESS,
        &clientId
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Open the thread associated with the window.

    status = PhOpenThreadClientId(
        &threadHande,
        THREAD_ALL_ACCESS, // THREAD_ALERT
        &clientId
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Create an execution required request for the process (Windows 8 and above)

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(processHande, &powerHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    // Queue a Special user-mode APC to execute within the context of the message loop.

    status = NtQueueApcThreadEx(
        threadHande,
        QUEUE_USER_APC_SPECIAL_USER_APC,
        ApcRoutine,
        ApcArgument1,
        ApcArgument2,
        ApcArgument3
        );

CleanupExit:
    if (threadHande)
        NtClose(threadHande);
    if (processHande)
        NtClose(processHande);
    if (powerHandle)
        PhDestroyExecutionRequiredRequest(powerHandle);

    return status;
}

/**
 * Destroys the specified window in a process.
 *
 * \param[in] ProcessHandle A handle to a process. The handle must have PROCESS_SET_LIMITED_INFORMATION access.
 * \param[in] WindowHandle A handle to the window to be destroyed.
 * \return NTSTATUS Successful or errant status.
 * \remarks A thread cannot call DestroyWindow for a window created by a different thread,
 * unless we queue a special APC to the owner thread.
 */
NTSTATUS PhDestroyWindowRemote(
    _In_ HANDLE ProcessHandle,
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;
    PVOID destroyWindow = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->User32FileName,
        "DestroyWindow",
        &destroyWindow,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhInvokeWindowProcedureRemote(
        WindowHandle,
        destroyWindow,
        (PVOID)WindowHandle,
        NULL,
        NULL
        );

CleanupExit:
    return status;
}

/**
 * Posts WM_QUIT (exit code) to a window's message loop via PostQuitMessage in remote context.
 *
 * \param[in] ProcessHandle Process handle.
 * \param[in] WindowHandle Window handle.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhPostWindowQuitMessageRemote(
    _In_ HANDLE ProcessHandle,
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;
    PVOID postQuitMessage = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->User32FileName,
        "PostQuitMessage",
        &postQuitMessage,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhInvokeWindowProcedureRemote(
        WindowHandle,
        postQuitMessage,
        UlongToPtr(EXIT_SUCCESS),
        NULL,
        NULL
        );

CleanupExit:
    return status;
}

// https://learn.microsoft.com/en-us/windows/win32/multimedia/obtaining-and-setting-timer-resolution
/**
 * Sets process timer resolution (TimeBeginPeriod) via remote APC invocation.
 *
 * \param[in] ProcessHandle Process handle.
 * \param[in] Period Requested timer resolution in ms.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetProcessTimerResolutionRemote(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Period
    )
{
    NTSTATUS status;
    PVOID rtlExitUserThread = NULL;
    PVOID timeBeginPeriod = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
    LARGE_INTEGER timeout;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        & isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "TimeBeginPeriod",
        &timeBeginPeriod,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            timeBeginPeriod,
            UlongToPtr(Period),
            NULL,
            NULL
            );
    }
    else
    {
#endif
        status = NtQueueApcThread(
            threadHandle,
            timeBeginPeriod,
            UlongToPtr(Period),
            NULL,
            NULL
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWaitForSingleObject(threadHandle, FALSE, PhTimeoutFromMilliseconds(&timeout, 5000));

    if (!NT_SUCCESS(status))
        goto CleanupExit;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

/**
 * Alternate variant for setting timer resolution (same semantics as PhSetProcessTimerResolutionRemote).
 *
 * \param[in] ProcessHandle Process handle.
 * \param[in] Period Requested period.
 * \returns NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetProcessTimerResolutionRemote2(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Period
    )
{
    NTSTATUS status;
    PVOID rtlExitUserThread = NULL;
    PVOID timeBeginPeriod = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        & isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "TimeBeginPeriod",
        &timeBeginPeriod,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            timeBeginPeriod,
            UlongToPtr(Period),
            NULL,
            NULL
            );
    }
    else
    {
#endif
        status = NtQueueApcThread(
            threadHandle,
            timeBeginPeriod,
            UlongToPtr(Period),
            NULL,
            NULL
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, 5000);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

/**
 * Sets handle flags (e.g. HANDLE_FLAG_INHERIT, HANDLE_FLAG_PROTECT_FROM_CLOSE) in a remote process.
 *
 * \param[in] ProcessHandle Target process.
 * \param[in] RemoteHandle Handle value in remote process.
 * \param[in] Mask Bitmask of flags to modify.
 * \param[in] Flags New flag values.
 * \return NTSTATUS Successful or errant status.
 * \remarks Invokes SetHandleInformation via APC onto suspended thread.
 */
NTSTATUS PhSetHandleInformationRemote(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE RemoteHandle,
    _In_ ULONG Mask,
    _In_ ULONG Flags
    )
{
    NTSTATUS status;
    PVOID rtlExitUserThread = NULL;
    PVOID setHandleInformation = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
    THREAD_BASIC_INFORMATION basicInformation;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        & isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "SetHandleInformation",
        &setHandleInformation,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            setHandleInformation,
            RemoteHandle,
            UlongToPtr(Mask),
            UlongToPtr(Flags)
            );
    }
    else
    {
#endif
        status = NtQueueApcThread(
            threadHandle,
            setHandleInformation,
            RemoteHandle,
            UlongToPtr(Mask),
            UlongToPtr(Flags)
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, 5000);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetThreadBasicInformation(threadHandle, &basicInformation);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = basicInformation.ExitStatus;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}
