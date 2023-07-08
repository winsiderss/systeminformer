/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2022
 *
 */

#include <kph.h>
#include <dyndata.h>

#include <trace.h>

static UNICODE_STRING KphpStackBackTraceTypeName = RTL_CONSTANT_STRING(L"KphStackBackTrace");
static PKPH_OBJECT_TYPE KphpStackBackTraceType = NULL;

PAGED_FILE();

typedef struct _KPH_STACK_BACKTRACE_OBJECT
{
    KSI_KAPC Apc;
    KEVENT CompletedEvent;
    ULONG FramesToSkip;
    ULONG FramesToCapture;
    ULONG Flags;
    ULONG BackTraceHash;
    ULONG CapturedFrames;
    PVOID BackTrace[ANYSIZE_ARRAY];
} KPH_STACK_BACKTRACE_OBJECT, *PKPH_STACK_BACKTRACE_OBJECT;

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

    PAGED_PASSIVE();

    thread = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(ThreadHandle, HANDLE);
            ProbeInputType(ClientId, CLIENT_ID);
            clientId = *ClientId;
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
            KphTracePrint(TRACE_LEVEL_ERROR,
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
            KphTracePrint(TRACE_LEVEL_ERROR,
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
            KphTracePrint(TRACE_LEVEL_ERROR,
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
        KphTracePrint(TRACE_LEVEL_ERROR,
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

    PAGED_PASSIVE();

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
        KphTracePrint(TRACE_LEVEL_ERROR,
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
            KphTracePrint(TRACE_LEVEL_ERROR,
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
        KphTracePrint(TRACE_LEVEL_ERROR,
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
 * \brief Captures a stack trace of the current thread.
 *
 * \param[in] FramesToSkip The number of frames to skip from the bottom of the stack.
 * \param[in] FramesToCapture The number of frames to capture.
 * \param[in] Flags A combination of the following:
 * \li \c KPH_STACK_TRACE_CAPTURE_USER_STACK The user-mode stack will be
 * included in the back trace.
 * \param[out] BackTrace An array in which the stack trace will be stored.
 * \param[out] BackTraceHash A variable which receives a hash of the stack trace.
 *
 * \return The number of frames captured.
 */
_IRQL_requires_max_(APC_LEVEL)
_Success_(return != 0)
ULONG KphCaptureStackBackTrace(
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _In_opt_ ULONG Flags,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Out_opt_ PULONG BackTraceHash
    )
{
    PVOID backTrace[MAX_STACK_DEPTH];
    ULONG framesFound;
    ULONG hash;
    ULONG i;

    PAGED_CODE();

    //
    // Skip the current frame (for this function).
    //
    FramesToSkip++;

    if ((FramesToCapture + FramesToSkip) > MAX_STACK_DEPTH)
    {
        return 0;
    }

    if (FlagOn(Flags, KPH_STACK_TRACE_CAPTURE_USER_STACK))
    {
        framesFound = KphCaptureStack(backTrace,
                                      (FramesToCapture + FramesToSkip));
    }
    else
    {
        framesFound = RtlWalkFrameChain(backTrace,
                                        (FramesToCapture + FramesToSkip),
                                        0);
    }

    //
    // Return nothing if we found fewer frames than we wanted to skip.
    //
    if (framesFound <= FramesToSkip)
    {
        return 0;
    }

    //
    // Copy over the stack trace. At the same time we calculate the stack
    // trace hash by summing the addresses.
    //
    for (i = 0, hash = 0; i < FramesToCapture; i++)
    {
        if ((FramesToSkip + i) >= framesFound)
        {
            break;
        }

        BackTrace[i] = backTrace[FramesToSkip + i];
        hash += PtrToUlong(BackTrace[i]);
    }

    if (BackTraceHash)
    {
        *BackTraceHash = hash;
    }

    return i;
}

/**
 * \brief Captures the current stack back trace into a back trace object.
 *
 * \param[in,out] BackTrace The back trace object to populate.
 */
_IRQL_requires_(APC_LEVEL)
_IRQL_requires_same_
VOID KphpCaptureStackBackTraceIntoObject(
    _Inout_ PKPH_STACK_BACKTRACE_OBJECT BackTrace
    )
{
    PAGED_CODE();

    BackTrace->CapturedFrames =
        KphCaptureStackBackTrace(BackTrace->FramesToSkip,
                                 BackTrace->FramesToCapture,
                                 BackTrace->Flags,
                                 BackTrace->BackTrace,
                                 &BackTrace->BackTraceHash);

    KeSetEvent(&BackTrace->CompletedEvent, EVENT_INCREMENT, FALSE);
}

/**
 * \brief APC routine for capturing the stack back trace of a thread.
 *
 * \param[in] Apc The ACP executed, contained within the back trace object.
 * \param[in] NormalRoutine Unused.
 * \param[in] NormalContext Unused.
 * \param[in] SystemArgument1 Unused.
 * \param[in] SystemArgument2 Unused.
 */
_Function_class_(KSI_KKERNEL_ROUTINE)
_IRQL_requires_(APC_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpCaptureStackBackTraceThreadSpecialApc(
    _In_ PKSI_KAPC Apc,
    _Inout_ _Deref_pre_maybenull_ PKNORMAL_ROUTINE *NormalRoutine,
    _Inout_ _Deref_pre_maybenull_ PVOID *NormalContext,
    _Inout_ _Deref_pre_maybenull_ PVOID *SystemArgument1,
    _Inout_ _Deref_pre_maybenull_ PVOID *SystemArgument2
    )
{
    PKPH_STACK_BACKTRACE_OBJECT backTrace;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    backTrace = CONTAINING_RECORD(Apc, KPH_STACK_BACKTRACE_OBJECT, Apc);

    KphpCaptureStackBackTraceIntoObject(backTrace);
}

/**
 * \brief APC cleanup routine for stack back trace capture.
 *
 * \param[in] Apc The ACP to clean up.
 * \param[in] Reason Unused.
 */
_Function_class_(KSI_KCLEANUP_ROUTINE)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpCaptureStackBackTraceThreadSpecialApcCleanup(
    _In_ PKSI_KAPC Apc,
    _In_ KSI_KAPC_CLEANUP_REASON Reason
    )
{
    PKPH_STACK_BACKTRACE_OBJECT backTrace;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Apc);
    DBG_UNREFERENCED_PARAMETER(Reason);

    backTrace = CONTAINING_RECORD(Apc, KPH_STACK_BACKTRACE_OBJECT, Apc);

    KphDereferenceObject(backTrace);
}

/**
 * \brief Allocates a stack back trace object.
 *
 * \param[in] Size The size to allocate.
 *
 * \return Allocated object, null on allocation failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpStackBackTraceAllocate(
    _In_ SIZE_T Size
    )
{
    PAGED_CODE();

    return KphAllocateNPaged(Size, KPH_TAG_BACKTRACE);
}

/**
 * \brief Frees a stack back trace object.
 *
 * \param[in] Object The stack back trace object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
VOID KSIAPI KphpStackBackTraceFree(
    _In_freesMem_ PVOID Object
    )
{
    PAGED_CODE();

    KphFree(Object, KPH_TAG_BACKTRACE);
}

/**
 * \brief Initializes a stack back trace object.
 *
 * \param[in,out] Object The stack back trace object to initialize.
 * \param[in] Parameter The thread to initialize the back trace object for.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_Must_inspect_result_
NTSTATUS KSIAPI KphpStackBackTraceInitialize(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_STACK_BACKTRACE_OBJECT backTrace;
    PETHREAD thread;

    PAGED_CODE();

    NT_ASSERT(Parameter);

    backTrace = Object;
    thread = Parameter;

    KsiInitializeApc(&backTrace->Apc,
                     KphDriverObject,
                     thread,
                     OriginalApcEnvironment,
                     KphpCaptureStackBackTraceThreadSpecialApc,
                     KphpCaptureStackBackTraceThreadSpecialApcCleanup,
                     NULL,
                     KernelMode,
                     NULL);

    KeInitializeEvent(&backTrace->CompletedEvent, NotificationEvent, FALSE);

    return STATUS_SUCCESS;
}

/**
 * \brief Initialized stack back trace infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeStackBackTrace(
    VOID
    )
{
    KPH_OBJECT_TYPE_INFO typeInfo;

    PAGED_PASSIVE();

    typeInfo.Allocate = KphpStackBackTraceAllocate;
    typeInfo.Initialize = KphpStackBackTraceInitialize;
    typeInfo.Delete = NULL;
    typeInfo.Free = KphpStackBackTraceFree;

    KphCreateObjectType(&KphpStackBackTraceTypeName,
                        &typeInfo,
                        &KphpStackBackTraceType);
}

/**
 * \brief Captures the stack trace of a thread.
 *
 * \param[in] Thread The thread to capture the stack trace of.
 * \param[in] FramesToSkip The number of frames to skip from the bottom of the stack.
 * \param[in] FramesToCapture The number of frames to capture.
 * \param[out] BackTrace An array in which the stack trace will be stored.
 * \param[out] CapturedFrames A variable which receives the number of frames captured.
 * \param[out] BackTraceHash A variable which receives a hash of the stack trace.
 * \param[in] AccessMode The mode in which to perform access checks.
 * \param[in] Flags A combination of the following:
 * \li \c KPH_STACK_TRACE_CAPTURE_USER_STACK The user-mode stack will be
 * included in the back trace.
 * \param[in] Optional timeout to wait for the back trace to be captured.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCaptureStackBackTraceThread(
    _In_ PETHREAD Thread,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Out_opt_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;
    LARGE_INTEGER timeout;
    PKPH_STACK_BACKTRACE_OBJECT backTrace;
    ULONG backTraceSize;

    PAGED_PASSIVE();

    backTrace = NULL;

    if (!BackTrace)
    {
        status = STATUS_INVALID_PARAMETER_4;
        goto Exit;
    }

    //
    // Make sure the caller didn't request too many frames. This also restricts
    // the amount of memory we will try to allocate later.
    //
    if (FramesToCapture > MAX_STACK_DEPTH)
    {
        status = STATUS_INVALID_PARAMETER_3;
        goto Exit;
    }

    backTraceSize = (FramesToCapture * sizeof(PVOID));

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(BackTrace, backTraceSize, 1);

            if (CapturedFrames)
            {
                ProbeOutputType(CapturedFrames, ULONG);

                *CapturedFrames = 0;
            }
            if (BackTraceHash)
            {
                ProbeOutputType(BackTraceHash, ULONG);

                *BackTraceHash = 0;
            }

            if (Timeout)
            {
                ProbeInputType(Timeout, LARGE_INTEGER);
                timeout.QuadPart = Timeout->QuadPart;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        if (CapturedFrames)
        {
            *CapturedFrames = 0;
        }

        if (BackTraceHash)
        {
            *BackTraceHash = 0;
        }

        if (Timeout)
        {
            timeout.QuadPart = Timeout->QuadPart;
        }
    }

    if (backTraceSize == 0)
    {
        status = STATUS_SUCCESS;
        goto Exit;
    }

    status = RtlULongAdd(backTraceSize,
                         sizeof(KPH_STACK_BACKTRACE_OBJECT),
                         &backTraceSize);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphCreateObject(KphpStackBackTraceType,
                             backTraceSize,
                             &backTrace,
                             Thread);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        backTrace = NULL;
        goto Exit;
    }

    backTrace->FramesToCapture = FramesToSkip;
    backTrace->FramesToCapture = FramesToCapture;
    backTrace->Flags = Flags;

    if (Thread == PsGetCurrentThread())
    {
        KIRQL oldIrql;
        KeRaiseIrql(APC_LEVEL, &oldIrql);
        KphpCaptureStackBackTraceIntoObject(backTrace);
        KeLowerIrql(oldIrql);

        status = STATUS_SUCCESS;
        goto Exit;
    }

    KphReferenceObject(backTrace);
    if (!KsiInsertQueueApc(&backTrace->Apc, NULL, NULL, IO_NO_INCREMENT))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KsiInsertQueueApc failed");

        KphReferenceObject(backTrace);
        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    status = KeWaitForSingleObject(&backTrace->CompletedEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (Timeout ? &timeout : NULL));
    if (status != STATUS_SUCCESS)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KeWaitForSingleObject failed: %!STATUS!",
                      status);

        goto Exit;
    }

    ASSERT(backTrace->CapturedFrames <= FramesToCapture);

    if (AccessMode != KernelMode)
    {
        __try
        {
            RtlCopyMemory(BackTrace,
                          backTrace->BackTrace,
                          (backTrace->CapturedFrames * sizeof(PVOID)));

            if (CapturedFrames)
            {
                *CapturedFrames = backTrace->CapturedFrames;
            }
            if (BackTraceHash)
            {
                *BackTraceHash = backTrace->BackTraceHash;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }
    }
    else
    {
        RtlCopyMemory(BackTrace,
                      backTrace->BackTrace,
                      (backTrace->CapturedFrames * sizeof(PVOID)));

        if (CapturedFrames)
        {
            *CapturedFrames = backTrace->CapturedFrames;
        }
        if (BackTraceHash)
        {
            *BackTraceHash = backTrace->BackTraceHash;
        }
    }

Exit:

    if (backTrace)
    {
        KphDereferenceObject(backTrace);
    }

    return status;
}

/**
 * \brief Captures the stack trace of a thread.
 *
 * \param[in] ThreadHandle A handle to the thread to capture the stack trace of.
 * \param[in] FramesToSkip The number of frames to skip from the bottom of the stack.
 * \param[in] FramesToCapture The number of frames to capture.
 * \param[out] BackTrace An array in which the stack trace will be stored.
 * \param[out] CapturedFrames A variable which receives the number of frames captured.
 * \param[out] BackTraceHash A variable which receives a hash of the stack trace.
 * \param[in] AccessMode The mode in which to perform access checks.
 * \param[in] Flags A combination of the following:
 * \li \c KPH_STACK_TRACE_CAPTURE_USER_STACK The user-mode stack will be
 * included in the back trace.
 * \param[in] Optional timeout to wait for the back trace to be captured.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCaptureStackBackTraceThreadByHandle(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Out_opt_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD thread;

    PAGED_PASSIVE();

    if (ThreadHandle == NtCurrentThread())
    {
        thread = PsGetCurrentThread();
        ObReferenceObject(thread);
    }
    else
    {
        status = ObReferenceObjectByHandle(ThreadHandle,
                                           0,
                                           *PsThreadType,
                                           AccessMode,
                                           &thread,
                                           NULL);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "ObReferenceObjectByHandle failed: %!STATUS!",
                          status);

            thread = NULL;
            goto Exit;
        }
    }

    status = KphCaptureStackBackTraceThread(thread,
                                            FramesToSkip,
                                            FramesToCapture,
                                            BackTrace,
                                            CapturedFrames,
                                            BackTraceHash,
                                            AccessMode,
                                            Flags,
                                            Timeout);

Exit:

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

    PAGED_PASSIVE();

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
        if (ThreadInformationLength <= ARRAYSIZE(stackBuffer))
        {
            RtlZeroMemory(stackBuffer, ARRAYSIZE(stackBuffer));
            threadInformation = stackBuffer;
        }
        else
        {
            threadInformation = KphAllocatePaged(ThreadInformationLength,
                                                 KPH_TAG_THREAD_INFO);
            if (!threadInformation)
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              GENERAL,
                              "Failed to allocate thread info buffer.");

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }
        }

        __try
        {
            ProbeForRead(ThreadInformation, ThreadInformationLength, 1);
            RtlCopyMemory(threadInformation,
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
        KphTracePrint(TRACE_LEVEL_ERROR,
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
        KphTracePrint(TRACE_LEVEL_ERROR,
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
        KphTracePrint(TRACE_LEVEL_ERROR,
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

    if (threadInformation &&
        (threadInformation != ThreadInformation) &&
        (threadInformation != stackBuffer))
    {
        KphFree(threadInformation, KPH_TAG_THREAD_INFO);
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
    PETHREAD threadObject;
    PKPH_THREAD_CONTEXT thread;
    ULONG returnLength;

    PAGED_PASSIVE();

    threadObject = NULL;
    thread = NULL;
    returnLength = 0;

    if (AccessMode != KernelMode)
    {
        __try
        {
            if (ThreadInformation)
            {
                ProbeForWrite(ThreadInformation, ThreadInformationLength, 1);
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
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        threadObject = NULL;
        goto Exit;
    }

    thread = KphGetThreadContext(PsGetThreadId(threadObject));
    if (!thread)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
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
            
            if ((KphDynKtReadOperationCount == ULONG_MAX) ||
                (KphDynKtWriteOperationCount == ULONG_MAX) ||
                (KphDynKtOtherOperationCount == ULONG_MAX) ||
                (KphDynKtReadTransferCount == ULONG_MAX) ||
                (KphDynKtWriteTransferCount == ULONG_MAX) ||
                (KphDynKtOtherTransferCount == ULONG_MAX))
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
                value = Add2Ptr(threadObject, KphDynKtReadOperationCount);
                counters->ReadOperationCount = *value;

                value = Add2Ptr(threadObject, KphDynKtWriteOperationCount);
                counters->WriteOperationCount = *value;

                value = Add2Ptr(threadObject, KphDynKtOtherOperationCount);
                counters->OtherOperationCount = *value;

                value = Add2Ptr(threadObject, KphDynKtReadTransferCount);
                counters->ReadTransferCount = *value;

                value = Add2Ptr(threadObject, KphDynKtWriteTransferCount);
                counters->WriteTransferCount = *value;

                value = Add2Ptr(threadObject, KphDynKtOtherTransferCount);
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
            PULONG threadId;

            if (thread->SubsystemType != SubsystemInformationTypeWSL)
            {
                KphTracePrint(TRACE_LEVEL_WARNING,
                              GENERAL,
                              "Invalid subsystem for WSL thread ID query.");

                status = STATUS_INVALID_HANDLE;
                goto Exit;
            }

            if (!thread->WSL.ValidThreadId)
            {
                KphTracePrint(TRACE_LEVEL_WARNING,
                              GENERAL,
                              "WSL thread ID is not valid.");

                status = STATUS_OBJECTID_NOT_FOUND;
                goto Exit;
            }

            if (!ThreadInformation ||
                (ThreadInformationLength < sizeof(ULONG)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(ULONG);
                goto Exit;
            }

            threadId = ThreadInformation;

            __try
            {
                *threadId = thread->WSL.ThreadId;
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

    return status;
}
