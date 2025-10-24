/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2022-2024
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PAGED_FILE();

/**
 * \brief Opens a thread.
 *
 * \param[out] ThreadHandle A variable which receives the thread handle.
 * \param[in] DesiredAccess The desired access to the thread.
 * \param[in] ClientId The identifier of a thread. \a UniqueThread must be
 * present. If \a UniqueProcess is present, the process of the referenced
 * thread will be checked against this identifier.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    CLIENT_ID clientId;
    PETHREAD thread;
    HANDLE threadHandle = NULL;

    KPH_PAGED_CODE_PASSIVE();

    thread = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(ThreadHandle, HANDLE);
            ProbeInputType(ClientId, CLIENT_ID);
            RtlCopyVolatileMemory(&clientId, ClientId, sizeof(CLIENT_ID));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    else
    {
        clientId = *ClientId;
    }

    //
    // Use the process ID if it was specified.
    //
    if (clientId.UniqueProcess)
    {
        status = PsLookupProcessThreadByCid(&clientId, NULL, &thread);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "PsLookupProcessThreadByCid failed: %!STATUS!",
                          status);

            thread = NULL;
            goto Exit;
        }
    }
    else
    {
        status = PsLookupThreadByThreadId(clientId.UniqueThread, &thread);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "PsLookupThreadByThreadId failed: %!STATUS!",
                          status);

            thread = NULL;
            goto Exit;
        }
    }

    if ((DesiredAccess & KPH_THREAD_READ_ACCESS) != DesiredAccess)
    {
        status = KphDominationCheck(PsGetCurrentProcess(),
                                    PsGetThreadProcess(thread),
                                    AccessMode);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "KphDominationCheck failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    //
    // Always open in KernelMode to skip access checks.
    //
    status = ObOpenObjectByPointer(thread,
                                   (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                   NULL,
                                   DesiredAccess,
                                   *PsThreadType,
                                   KernelMode,
                                   &threadHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        threadHandle = NULL;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            *ThreadHandle = threadHandle;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ObCloseHandle(threadHandle, UserMode);
            status = GetExceptionCode();
        }
    }
    else
    {
        *ThreadHandle = threadHandle;
    }

Exit:
    if (thread)
    {
        ObDereferenceObject(thread);
    }

    return status;
}

/**
 * \brief Opens the process of a thread.
 *
 * \param[in] ThreadHandle A handle to a thread.
 * \param[in] DesiredAccess The desired access to the process.
 * \param[out] ProcessHandle A variable which receives the process handle.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PETHREAD thread;
    HANDLE processHandle;

    KPH_PAGED_CODE_PASSIVE();

    thread = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(ProcessHandle, HANDLE);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(ThreadHandle,
                                       0,
                                       *PsThreadType,
                                       AccessMode,
                                       &thread,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        thread = NULL;
        goto Exit;
    }

    if ((DesiredAccess & KPH_PROCESS_READ_ACCESS) != DesiredAccess)
    {
        status = KphDominationCheck(PsGetCurrentProcess(),
                                    PsGetThreadProcess(thread),
                                    AccessMode);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "KphDominationCheck failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    //
    // Always open in KernelMode to skip access checks.
    //
    status = ObOpenObjectByPointer(PsGetThreadProcess(thread),
                                   (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                   NULL,
                                   DesiredAccess,
                                   *PsProcessType,
                                   KernelMode,
                                   &processHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        processHandle = NULL;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            *ProcessHandle = processHandle;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ObCloseHandle(processHandle, UserMode);
            status = GetExceptionCode();
        }
    }
    else
    {
        *ProcessHandle = processHandle;
    }

Exit:
    if (thread)
    {
        ObDereferenceObject(thread);
    }

    return status;
}

/**
 * \brief Captures the stack trace of a thread.
 *
 * \param[in] ThreadHandle A handle to the thread to capture the stack trace of.
 * \param[in] FramesToSkip The number of kernel frames to skip.
 * \param[in] FramesToCapture The number of frames to capture.
 * \param[out] BackTrace Buffer to store the back trace in.
 * \param[out] CapturedFrames Receives the number of captured frames.
 * \param[out] BackTraceHash Optionally receives a hash of the back trace.
 * \param[in] Flags A combination of KPH_STACK_BACK_TRACE_* flags.
 * \param[in] Timeout Optionally specifies a timeout for the capture operation.
 *
 * \return STATUS_SUCCES, STATUS_TIMEOUT, or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCaptureStackBackTraceThreadByHandle(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID* BackTrace,
    _Out_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD thread;
    PVOID backTrace;
    ULONG capturedFrames;
    ULONG backTraceHash;
    LARGE_INTEGER timeout;

    KPH_PAGED_CODE_PASSIVE();

    backTrace = NULL;
    thread = NULL;
    backTraceHash = 0;

    if (!CapturedFrames)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputBytes(BackTrace, FramesToCapture * sizeof(PVOID));

            ProbeOutputType(CapturedFrames, ULONG);
            *CapturedFrames = 0;

            if (BackTraceHash)
            {
                ProbeOutputType(BackTraceHash, ULONG);
                *BackTraceHash = 0;
            }

            if (Timeout)
            {
                ProbeInputType(Timeout, LARGE_INTEGER);
                RtlCopyVolatileMemory(&timeout, Timeout, sizeof(LARGE_INTEGER));
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }

        backTrace = KphAllocatePaged(FramesToCapture * sizeof(PVOID),
                                     KPH_TAG_THREAD_BACK_TRACE);
        if (!backTrace)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "Failed to allocate back trace buffer.");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
    }
    else
    {
        *CapturedFrames = 0;

        if (BackTraceHash)
        {
            *BackTraceHash = 0;
        }

        if (Timeout)
        {
            timeout.QuadPart = Timeout->QuadPart;
        }

        backTrace = BackTrace;
    }

    status = ObReferenceObjectByHandle(ThreadHandle,
                                       0,
                                       *PsThreadType,
                                       AccessMode,
                                       &thread,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        thread = NULL;
        goto Exit;
    }

    status = KphCaptureStackBackTraceThread(thread,
                                            FramesToSkip,
                                            FramesToCapture,
                                            backTrace,
                                            &capturedFrames,
                                            (BackTraceHash ? &backTraceHash : NULL),
                                            Flags,
                                            (Timeout ? &timeout : NULL));
    if (!NT_SUCCESS(status) || (status == STATUS_TIMEOUT))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphCaptureStackBackTraceThread failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            RtlCopyMemory(BackTrace, backTrace, capturedFrames * sizeof(PVOID));

            *CapturedFrames = capturedFrames;

            if (BackTraceHash)
            {
                *BackTraceHash = backTraceHash;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }
    }
    else
    {
        *CapturedFrames = capturedFrames;

        if (BackTraceHash)
        {
            *BackTraceHash = backTraceHash;
        }
    }

Exit:

    if (backTrace && (backTrace != BackTrace))
    {
        KphFree(backTrace, KPH_TAG_THREAD_BACK_TRACE);
    }

    if (thread)
    {
        ObDereferenceObject(thread);
    }

    return status;
}

/**
 * \brief Sets information about a thread.
 *
 * \param[in] ThreadHandle Handle to thread to set information for.
 * \param[in] ThreadInformationClass Information class to set.
 * \param[in] ThreadInformation Information to set.
 * \param[in] ThreadInformationLength Length of the thread information buffer.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PVOID threadInformation;
    BYTE stackBuffer[64];
    PETHREAD thread;
    HANDLE threadHandle;
    THREADINFOCLASS threadInformationClass;

    KPH_PAGED_CODE_PASSIVE();

    threadInformation = NULL;
    thread = NULL;
    threadHandle = NULL;

    if (!ThreadInformation)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        threadInformation = KphAllocatePagedA(ThreadInformationLength,
                                              KPH_TAG_THREAD_INFO,
                                              stackBuffer);
        if (!threadInformation)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "Failed to allocate thread info buffer.");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        __try
        {
            ProbeInputBytes(ThreadInformation, ThreadInformationLength);
            RtlCopyVolatileMemory(threadInformation,
                                  ThreadInformation,
                                  ThreadInformationLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        threadInformation = ThreadInformation;
    }

    status = ObReferenceObjectByHandle(ThreadHandle,
                                       0,
                                       *PsThreadType,
                                       AccessMode,
                                       &thread,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        thread = NULL;
        goto Exit;
    }

    status = KphDominationCheck(PsGetCurrentProcess(),
                                PsGetThreadProcess(thread),
                                AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphDominationCheck failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = ObOpenObjectByPointer(thread,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   THREAD_SET_INFORMATION,
                                   *PsThreadType,
                                   KernelMode,
                                   &threadHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        threadHandle = NULL;
        goto Exit;
    }

    switch (ThreadInformationClass)
    {
        case KphThreadPriority:
        {
            threadInformationClass = ThreadPriority;
            break;
        }
        case KphThreadBasePriority:
        {
            threadInformationClass = ThreadBasePriority;
            break;
        }
        case KphThreadAffinityMask:
        {
            threadInformationClass = ThreadAffinityMask;
            break;
        }
        case KphThreadIdealProcessor:
        {
            threadInformationClass = ThreadIdealProcessor;
            break;
        }
        case KphThreadPriorityBoost:
        {
            threadInformationClass = ThreadPriorityBoost;
            break;
        }
        case KphThreadIoPriority:
        {
            threadInformationClass = ThreadIoPriority;
            break;
        }
        case KphThreadPagePriority:
        {
            threadInformationClass = ThreadPagePriority;
            break;
        }
        case KphThreadActualBasePriority:
        {
            threadInformationClass = ThreadActualBasePriority;
            break;
        }
        case KphThreadGroupInformation:
        {
            threadInformationClass = ThreadGroupInformation;
            break;
        }
        case KphThreadIdealProcessorEx:
        {
            threadInformationClass = ThreadIdealProcessorEx;
            break;
        }
        case KphThreadActualGroupAffinity:
        {
            threadInformationClass = ThreadActualGroupAffinity;
            break;
        }
        case KphThreadPowerThrottlingState:
        {
            threadInformationClass = ThreadPowerThrottlingState;
            break;
        }
        case KphThreadExplicitCaseSensitivity:
        {
            threadInformationClass = ThreadExplicitCaseSensitivity;
            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            goto Exit;
        }
    }

    status = ZwSetInformationThread(threadHandle,
                                    threadInformationClass,
                                    threadInformation,
                                    ThreadInformationLength);

Exit:

    if (threadHandle)
    {
        ObCloseHandle(threadHandle, KernelMode);
    }

    if (thread)
    {
        ObDereferenceObject(thread);
    }

    if (threadInformation && (threadInformation != ThreadInformation))
    {
        KphFreeA(threadInformation, KPH_TAG_THREAD_INFO, stackBuffer);
    }

    return status;
}

/**
 * \brief Queries thread information.
 *
 * \param[in] ThreadHandle Handle to thread to query information of.
 * \param[in] ThreadInformationClass Information class to query.
 * \param[out] ThreadInformation Populated with thread information by class.
 * \param[in] ThreadInformationLength Length of the thread information buffer.
 * \param[out] ReturnLength Number of bytes written or necessary for the
 * information.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _Out_writes_bytes_opt_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;
    PETHREAD threadObject;
    PKPH_THREAD_CONTEXT thread;
    ULONG returnLength;

    KPH_PAGED_CODE_PASSIVE();

    dyn = NULL;
    threadObject = NULL;
    thread = NULL;
    returnLength = 0;

    if (AccessMode != KernelMode)
    {
        __try
        {
            if (ThreadInformation)
            {
                ProbeOutputBytes(ThreadInformation, ThreadInformationLength);
            }

            if (ReturnLength)
            {
                ProbeOutputType(ReturnLength, ULONG);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    status = ObReferenceObjectByHandle(ThreadHandle,
                                       0,
                                       *PsThreadType,
                                       AccessMode,
                                       &threadObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        threadObject = NULL;
        goto Exit;
    }

    thread = KphGetEThreadContext(threadObject);
    if (!thread)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphGetThreadContext returned null.");

        status = STATUS_OBJECTID_NOT_FOUND;
        goto Exit;
    }

    switch (ThreadInformationClass)
    {
        case KphThreadIoCounters:
        {
            PIO_COUNTERS counters;
            PULONGLONG value;

            dyn = KphReferenceDynData();

            if (!dyn ||
                (dyn->KtReadOperationCount == ULONG_MAX) ||
                (dyn->KtWriteOperationCount == ULONG_MAX) ||
                (dyn->KtOtherOperationCount == ULONG_MAX) ||
                (dyn->KtReadTransferCount == ULONG_MAX) ||
                (dyn->KtWriteTransferCount == ULONG_MAX) ||
                (dyn->KtOtherTransferCount == ULONG_MAX))
            {
                status = STATUS_NOINTERFACE;
                goto Exit;
            }

            if (!ThreadInformation ||
                (ThreadInformationLength < sizeof(IO_COUNTERS)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(IO_COUNTERS);
                goto Exit;
            }

            counters = ThreadInformation;

            __try
            {
                value = Add2Ptr(threadObject, dyn->KtReadOperationCount);
                counters->ReadOperationCount = *value;

                value = Add2Ptr(threadObject, dyn->KtWriteOperationCount);
                counters->WriteOperationCount = *value;

                value = Add2Ptr(threadObject, dyn->KtOtherOperationCount);
                counters->OtherOperationCount = *value;

                value = Add2Ptr(threadObject, dyn->KtReadTransferCount);
                counters->ReadTransferCount = *value;

                value = Add2Ptr(threadObject, dyn->KtWriteTransferCount);
                counters->WriteTransferCount = *value;

                value = Add2Ptr(threadObject, dyn->KtOtherTransferCount);
                counters->OtherTransferCount = *value;

                returnLength = sizeof(IO_COUNTERS);
                status = STATUS_SUCCESS;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }

            break;
        }
        case KphThreadWSLThreadId:
        {
            ULONG threadId;

            if (thread->SubsystemType != SubsystemInformationTypeWSL)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "Invalid subsystem for WSL thread ID query.");

                status = STATUS_INVALID_HANDLE;
                goto Exit;
            }

            if (!ThreadInformation ||
                (ThreadInformationLength < sizeof(ULONG)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(ULONG);
                goto Exit;
            }

            status = KphQueryInformationThreadContext(thread,
                                                      KphThreadContextWSLThreadId,
                                                      &threadId,
                                                      sizeof(threadId),
                                                      NULL);
            if (!NT_SUCCESS(status))
            {
                goto Exit;
            }

            __try
            {
                *(PULONG)ThreadInformation = threadId;
                returnLength = sizeof(ULONG);
                status = STATUS_SUCCESS;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }

            break;
        }
        case KphThreadKernelStackInformation:
        {
            PKPH_KERNEL_STACK_INFORMATION info;

            dyn = KphReferenceDynData();

            if (!dyn ||
                (dyn->KtInitialStack == ULONG_MAX) ||
                (dyn->KtStackLimit == ULONG_MAX) ||
                (dyn->KtStackBase == ULONG_MAX) ||
                (dyn->KtKernelStack == ULONG_MAX))
            {
                status = STATUS_NOINTERFACE;
                goto Exit;
            }

            if (!ThreadInformation ||
                (ThreadInformationLength < sizeof(KPH_KERNEL_STACK_INFORMATION)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KPH_KERNEL_STACK_INFORMATION);
                goto Exit;
            }

            info = ThreadInformation;

            __try
            {
                info->InitialStack = *(PVOID*)Add2Ptr(threadObject, dyn->KtInitialStack);
                info->StackLimit = *(PVOID*)Add2Ptr(threadObject, dyn->KtStackLimit);
                info->StackBase = *(PVOID*)Add2Ptr(threadObject, dyn->KtStackBase);
                info->KernelStack = *(PVOID*)Add2Ptr(threadObject, dyn->KtKernelStack);

                returnLength = sizeof(KPH_KERNEL_STACK_INFORMATION);
                status = STATUS_SUCCESS;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }

            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            break;
        }
    }

Exit:

    if (ReturnLength)
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *ReturnLength = returnLength;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                NOTHING;
            }
        }
        else
        {
            *ReturnLength = returnLength;
        }
    }

    if (thread)
    {
        KphDereferenceObject(thread);
    }

    if (threadObject)
    {
        ObDereferenceObject(threadObject);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    return status;
}
