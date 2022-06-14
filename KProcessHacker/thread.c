/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#include <kph.h>
#include <dyndata.h>

typedef struct _CAPTURE_BACKTRACE_THREAD_CONTEXT
{
    BOOLEAN Local;
    KAPC Apc;
    KEVENT CompletedEvent;
    ULONG FramesToSkip;
    ULONG FramesToCapture;
    PVOID *BackTrace;
    ULONG CapturedFrames;
    ULONG BackTraceHash;
    ULONG Flags;
} CAPTURE_BACKTRACE_THREAD_CONTEXT, *PCAPTURE_BACKTRACE_THREAD_CONTEXT;

KKERNEL_ROUTINE KphpCaptureStackBackTraceThreadSpecialApc;

VOID KphpCaptureStackBackTraceThreadSpecialApc(
    _In_ PRKAPC Apc,
    _Inout_opt_ PKNORMAL_ROUTINE *NormalRoutine,
    _Inout_opt_ PVOID *NormalContext,
    _Inout_ PVOID *SystemArgument1,
    _Inout_ PVOID *SystemArgument2
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KpiOpenThread)
#pragma alloc_text(PAGE, KpiOpenThreadProcess)
#pragma alloc_text(PAGE, KphCaptureStackBackTraceThread)
#pragma alloc_text(PAGE, KphpCaptureStackBackTraceThreadSpecialApc)
#pragma alloc_text(PAGE, KpiCaptureStackBackTraceThread)
#pragma alloc_text(PAGE, KpiQueryInformationThread)
#pragma alloc_text(PAGE, KpiSetInformationThread)
#endif

/**
 * Opens a thread.
 *
 * \param ThreadHandle A variable which receives the thread handle.
 * \param DesiredAccess The desired access to the thread.
 * \param ClientId The identifier of a thread. \a UniqueThread must be present. If \a UniqueProcess
 * is present, the process of the referenced thread will be checked against this identifier.
 * \param Key An access key.
 * \li If a L2 key is provided, no access checks are performed.
 * \li If a L1 key is provided, only read access is permitted but no additional access checks are
 * performed.
 * \li If no valid key is provided, the function fails.
 * \param Client The client that initiated the request.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    CLIENT_ID clientId;
    PETHREAD thread;
    KPH_KEY_LEVEL requiredKeyLevel;
    HANDLE threadHandle = NULL;

    PAGED_CODE();

    thread = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(ThreadHandle, sizeof(HANDLE), sizeof(HANDLE));
            ProbeForRead(ClientId, sizeof(CLIENT_ID), sizeof(ULONG));
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

    // Use the process ID if it was specified.
    if (clientId.UniqueProcess)
    {
        status = PsLookupProcessThreadByCid(&clientId, NULL, &thread);
    }
    else
    {
        status = PsLookupThreadByThreadId(clientId.UniqueThread, &thread);
    }

    if (!NT_SUCCESS(status))
    {
        thread = NULL;
        goto CleanupExit;
    }

    requiredKeyLevel = KphKeyLevel1;

    if ((DesiredAccess & KPH_THREAD_READ_ACCESS) != DesiredAccess)
    {
        requiredKeyLevel = KphKeyLevel2;

        status = KphDominationCheck(
            Client,
            PsGetCurrentProcess(),
            PsGetThreadProcess(thread),
            AccessMode
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = KphVerifyCaller(
            Client,
            PsGetCurrentThread(),
            AccessMode
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = KphValidateKey(requiredKeyLevel, Key, Client, AccessMode);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Always open in KernelMode to skip access checks.
    status = ObOpenObjectByPointer(
        thread,
        0,
        NULL,
        DesiredAccess,
        *PsThreadType,
        KernelMode,
        &threadHandle
        );

    if (NT_SUCCESS(status))
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *ThreadHandle = threadHandle;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        else
        {
            *ThreadHandle = threadHandle;
        }
    }

CleanupExit:
    if (thread)
    {
        ObDereferenceObject(thread);
    }

    return status;
}

/**
 * Opens the process of a thread.
 *
 * \param ThreadHandle A handle to a thread.
 * \param DesiredAccess The desired access to the process.
 * \param ProcessHandle A variable which receives the process handle.
 * \param Key An access key.
 * \li If a L2 key is provided, no access checks are performed.
 * \li If a L1 key is provided, only read access is permitted but no additional access checks are
 * performed.
 * \li If no valid key is provided, the function fails.
 * \param Client The client that initiated the request.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PETHREAD thread;
    KPH_KEY_LEVEL requiredKeyLevel;
    HANDLE processHandle;

    PAGED_CODE();

    thread = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(ProcessHandle, sizeof(HANDLE), sizeof(HANDLE));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    requiredKeyLevel = KphKeyLevel1;

    if ((DesiredAccess & KPH_PROCESS_READ_ACCESS) != DesiredAccess)
    {
        requiredKeyLevel = KphKeyLevel2;

        status = KphDominationCheck(
            Client,
            PsGetCurrentProcess(),
            PsGetThreadProcess(thread),
            AccessMode
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = KphVerifyCaller(
            Client,
            PsGetCurrentThread(),
            AccessMode
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = KphValidateKey(requiredKeyLevel, Key, Client, AccessMode);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = ObReferenceObjectByHandle(
        ThreadHandle,
        0,
        *PsThreadType,
        AccessMode,
        &thread,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        thread = NULL;
        goto CleanupExit;
    }
    
    // Note: Windows 7 and Windows 8 require KernelMode (dmex)
    status = ObOpenObjectByPointer(
        PsGetThreadProcess(thread),
        0,
        NULL,
        DesiredAccess,
        *PsProcessType,
        KernelMode,
        &processHandle
        );

    if (NT_SUCCESS(status))
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *ProcessHandle = processHandle;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        else
        {
            *ProcessHandle = processHandle;
        }
    }

CleanupExit:
    if (thread)
    {
        ObDereferenceObject(thread);
    }

    return status;
}

/**
 * Captures a stack trace of the current thread.
 *
 * \param FramesToSkip The number of frames to skip from the bottom of the stack.
 * \param FramesToCapture The number of frames to capture.
 * \param Flags A combination of the following:
 * \li \c KPH_STACK_TRACE_CAPTURE_USER_STACK The user-mode stack will be
 * included in the back trace.
 * \param BackTrace An array in which the stack trace will be stored.
 * \param BackTraceHash A variable which receives a hash of the stack trace.
 *
 * \return The number of frames captured.
 */
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

    // Skip the current frame (for this function).
    FramesToSkip++;

    // Ensure that we won't overrun the buffer.
    if (FramesToCapture + FramesToSkip > MAX_STACK_DEPTH)
        return 0;

    // Walk the stack.
    framesFound = RtlWalkFrameChain(
        backTrace,
        FramesToCapture + FramesToSkip,
        0 
        );

    if (FlagOn(Flags, KPH_STACK_TRACE_CAPTURE_USER_STACK))
    {
        framesFound += RtlWalkFrameChain(
            &backTrace[framesFound],
            (FramesToCapture + FramesToSkip) - framesFound,
            RTL_WALK_USER_MODE_STACK
            );
    }

    // Return nothing if we found fewer frames than we wanted to skip.
    if (framesFound <= FramesToSkip)
        return 0;

    // Copy over the stack trace. At the same time we calculate the stack trace hash by summing the
    // addresses.
    for (i = 0, hash = 0; i < FramesToCapture; i++)
    {
        if (FramesToSkip + i >= framesFound)
            break;

        BackTrace[i] = backTrace[FramesToSkip + i];
        hash += PtrToUlong(BackTrace[i]);
    }

    if (BackTraceHash)
        *BackTraceHash = hash;

    return i;
}

/**
 * Captures the stack trace of a thread.
 *
 * \param Thread The thread to capture the stack trace of.
 * \param FramesToSkip The number of frames to skip from the bottom of the stack.
 * \param FramesToCapture The number of frames to capture.
 * \param BackTrace An array in which the stack trace will be stored.
 * \param CapturedFrames A variable which receives the number of frames captured.
 * \param BackTraceHash A variable which receives a hash of the stack trace.
 * \param AccessMode The mode in which to perform access checks.
 * \param Flags A combination of the following:
 * \li \c KPH_STACK_TRACE_CAPTURE_USER_STACK The user-mode stack will be
 * included in the back trace.
 *
 * \return The number of frames captured.
 */
NTSTATUS KphCaptureStackBackTraceThread(
    _In_ PETHREAD Thread,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Out_opt_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ ULONG Flags
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CAPTURE_BACKTRACE_THREAD_CONTEXT context;
    ULONG backTraceSize;
    PVOID *backTrace;

    PAGED_CODE();

    // Make sure the caller didn't request too many frames. This also restricts the amount of memory
    // we will try to allocate later.
    if (FramesToCapture > MAX_STACK_DEPTH)
        return STATUS_INVALID_PARAMETER_3;

    backTraceSize = FramesToCapture * sizeof(PVOID);

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(BackTrace, backTraceSize, sizeof(PVOID));

            if (CapturedFrames)
                ProbeForWrite(CapturedFrames, sizeof(ULONG), sizeof(ULONG));
            if (BackTraceHash)
                ProbeForWrite(BackTraceHash, sizeof(ULONG), sizeof(ULONG));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    // If the caller doesn't want to capture anything, return immediately.
    if (backTraceSize == 0)
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                if (CapturedFrames)
                    *CapturedFrames = 0;
                if (BackTraceHash)
                    *BackTraceHash = 0;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        else
        {
            if (CapturedFrames)
                *CapturedFrames = 0;
            if (BackTraceHash)
                *BackTraceHash = 0;
        }

        return status;
    }

    // Allocate storage for the stack trace.
    backTrace = ExAllocatePoolZero(NonPagedPool, backTraceSize, 'bhpK');

    if (!backTrace)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Initialize the context structure.
    context.FramesToSkip = FramesToSkip;
    context.FramesToCapture = FramesToCapture;
    context.BackTrace = backTrace;
    context.Flags = Flags;

    // Check if we're trying to get a stack trace of the current thread.
    // If so, we don't need to insert an APC.
    if (Thread == PsGetCurrentThread())
    {
        PCAPTURE_BACKTRACE_THREAD_CONTEXT contextPtr = &context;
        PVOID dummy = NULL;
        KIRQL oldIrql;

        // Raise the IRQL to APC_LEVEL to simulate an APC environment,
        // and call the APC routine directly.

        context.Local = TRUE;
        KeRaiseIrql(APC_LEVEL, &oldIrql);
        KphpCaptureStackBackTraceThreadSpecialApc(
            &context.Apc,
            NULL,
            NULL,
            &contextPtr,
            &dummy
            );
        KeLowerIrql(oldIrql);
    }
    else
    {
        context.Local = FALSE;
        KeInitializeEvent(&context.CompletedEvent, NotificationEvent, FALSE);
        KeInitializeApc(
            &context.Apc,
            (PKTHREAD)Thread,
            OriginalApcEnvironment,
            KphpCaptureStackBackTraceThreadSpecialApc,
            NULL,
            NULL,
            KernelMode,
            NULL
            );

        if (KeInsertQueueApc(&context.Apc, &context, NULL, 2))
        {
            // Wait for the APC to complete.
            status = KeWaitForSingleObject(
                &context.CompletedEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status))
    {
        ASSERT(context.CapturedFrames <= FramesToCapture);

        if (AccessMode != KernelMode)
        {
            __try
            {
                memcpy(BackTrace, backTrace, context.CapturedFrames * sizeof(PVOID));

                if (CapturedFrames)
                    *CapturedFrames = context.CapturedFrames;
                if (BackTraceHash)
                    *BackTraceHash = context.BackTraceHash;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        else
        {
            memcpy(BackTrace, backTrace, context.CapturedFrames * sizeof(PVOID));

            if (CapturedFrames)
                *CapturedFrames = context.CapturedFrames;
            if (BackTraceHash)
                *BackTraceHash = context.BackTraceHash;
        }
    }

    ExFreePoolWithTag(backTrace, 'bhpK');

    return status;
}

VOID KphpCaptureStackBackTraceThreadSpecialApc(
    _In_ PRKAPC Apc,
    _Inout_opt_ PKNORMAL_ROUTINE *NormalRoutine,
    _Inout_opt_ PVOID *NormalContext,
    _Inout_ PVOID *SystemArgument1,
    _Inout_ PVOID *SystemArgument2
    )
{
    PCAPTURE_BACKTRACE_THREAD_CONTEXT context = *SystemArgument1;

    PAGED_CODE();

    context->CapturedFrames = KphCaptureStackBackTrace(
        context->FramesToSkip,
        context->FramesToCapture,
        context->Flags,
        context->BackTrace,
        &context->BackTraceHash
        );

    if (!context->Local)
    {
        // Notify the originating thread that we have completed.
        KeSetEvent(&context->CompletedEvent, 0, FALSE);
    }
}

/**
 * Captures the stack trace of a thread.
 *
 * \param ThreadHandle A handle to the thread to capture the stack trace of.
 * \param FramesToSkip The number of frames to skip from the bottom of the stack.
 * \param FramesToCapture The number of frames to capture.
 * \param BackTrace An array in which the stack trace will be stored.
 * \param CapturedFrames A variable which receives the number of frames captured.
 * \param BackTraceHash A variable which receives a hash of the stack trace.
 * \param AccessMode The mode in which to perform access checks.
 * \param Flags A combination of the following:
 * \li \c KPH_STACK_TRACE_CAPTURE_USER_STACK The user-mode stack will be
 * included in the back trace.
 *
 * \return The number of frames captured.
 */
NTSTATUS KpiCaptureStackBackTraceThread(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Out_opt_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ ULONG Flags
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD thread;

    PAGED_CODE();

    status = ObReferenceObjectByHandle(
        ThreadHandle,
        0,
        *PsThreadType,
        AccessMode,
        &thread,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = KphCaptureStackBackTraceThread(
        thread,
        FramesToSkip,
        FramesToCapture,
        BackTrace,
        CapturedFrames,
        BackTraceHash,
        AccessMode,
        Flags
        );
    ObDereferenceObject(thread);

    return status;
}

/**
 * Queries thread information.
 *
 * \param ThreadHandle A handle to a thread.
 * \param ThreadInformationClass The type of information to query.
 * \param ThreadInformation The buffer in which the information will be stored.
 * \param ThreadInformationLength The number of bytes available in \a ThreadInformation.
 * \param ReturnLength A variable which receives the number of bytes required to be available in
 * \a ThreadInformation.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PETHREAD thread;
    ULONG returnLength;

    PAGED_CODE();

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(ThreadInformation, ThreadInformationLength, sizeof(ULONG));

            if (ReturnLength)
                ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(
        ThreadHandle,
        THREAD_QUERY_INFORMATION,
        *PsThreadType,
        AccessMode,
        &thread,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    switch (ThreadInformationClass)
    {
    default:
        status = STATUS_INVALID_INFO_CLASS;
        returnLength = 0;
        break;
    }

    ObDereferenceObject(thread);

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

    return status;
}

/**
 * Sets thread information.
 *
 * \param ThreadHandle A handle to a thread.
 * \param ThreadInformationClass The type of information to set.
 * \param ThreadInformation A buffer which contains the information to set.
 * \param ThreadInformationLength The number of bytes present in \a ThreadInformation.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PETHREAD thread;

    PAGED_CODE();

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForRead(ThreadInformation, ThreadInformationLength, sizeof(ULONG));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(
        ThreadHandle,
        THREAD_SET_INFORMATION,
        *PsThreadType,
        AccessMode,
        &thread,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    switch (ThreadInformationClass)
    {
    default:
        status = STATUS_INVALID_INFO_CLASS;
        break;
    }

    ObDereferenceObject(thread);

    return status;
}
