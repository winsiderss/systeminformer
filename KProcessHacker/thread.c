/*
 * KProcessHacker
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

#include <kph.h>
#include <dyndata.h>

typedef struct _EXIT_THREAD_CONTEXT
{
    KAPC Apc;
    KEVENT CompletedEvent;
    NTSTATUS ExitStatus;
} EXIT_THREAD_CONTEXT, *PEXIT_THREAD_CONTEXT;

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
} CAPTURE_BACKTRACE_THREAD_CONTEXT, *PCAPTURE_BACKTRACE_THREAD_CONTEXT;

KKERNEL_ROUTINE KphpCaptureStackBackTraceThreadSpecialApc;
KKERNEL_ROUTINE KphpExitThreadSpecialApc;

VOID KphpCaptureStackBackTraceThreadSpecialApc(
    __in PRKAPC Apc,
    __inout PKNORMAL_ROUTINE *NormalRoutine,
    __inout PVOID *NormalContext,
    __inout PVOID *SystemArgument1,
    __inout PVOID *SystemArgument2
    );

VOID KphpExitThreadSpecialApc(
    __in PRKAPC Apc,
    __inout PKNORMAL_ROUTINE *NormalRoutine,
    __inout PVOID *NormalContext,
    __inout PVOID *SystemArgument1,
    __inout PVOID *SystemArgument2
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KpiOpenThread)
#pragma alloc_text(PAGE, KpiOpenThreadProcess)
#pragma alloc_text(PAGE, KphTerminateThreadByPointerInternal)
#pragma alloc_text(PAGE, KpiTerminateThread)
#pragma alloc_text(PAGE, KpiTerminateThreadUnsafe)
#pragma alloc_text(PAGE, KphpExitThreadSpecialApc)
#pragma alloc_text(PAGE, KpiGetContextThread)
#pragma alloc_text(PAGE, KpiSetContextThread)
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
 * \param ClientId The identifier of a thread. \a UniqueThread must be 
 * present. If \a UniqueProcess is present, the process of the referenced 
 * thread will be checked against this identifier.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PCLIENT_ID ClientId,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    CLIENT_ID clientId;
    PETHREAD thread;
    HANDLE threadHandle;

    PAGED_CODE();

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
        return status;

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
    ObDereferenceObject(thread);

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

    return status;
}

/**
 * Opens the process of a thread.
 *
 * \param ThreadHandle A handle to a thread.
 * \param DesiredAccess The desired access to the process.
 * \param ProcessHandle A variable which receives the process handle.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiOpenThreadProcess(
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PETHREAD thread;
    PEPROCESS process;
    HANDLE processHandle;

    PAGED_CODE();

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

    process = IoThreadToProcess(thread);

    status = ObOpenObjectByPointer(
        process,
        0,
        NULL,
        DesiredAccess,
        *PsProcessType,
        KernelMode,
        &processHandle
        );

    ObDereferenceObject(thread);

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

    return status;
}

/**
 * Terminates a thread using PspTerminateThreadByPointer.
 *
 * \param Thread A thread object.
 * \param ExitStatus A status value which indicates why the thread 
 * is being terminated.
 */
NTSTATUS KphTerminateThreadByPointerInternal(
    __in PETHREAD Thread,
    __in NTSTATUS ExitStatus
    )
{
    PVOID PspTerminateThreadByPointer_I;

    PAGED_CODE();

    PspTerminateThreadByPointer_I = KphGetDynamicProcedureScan(&KphDynPspTerminateThreadByPointerScan);

    if (!PspTerminateThreadByPointer_I)
        return STATUS_NOT_SUPPORTED;

    if (KphDynNtVersion == PHNT_WINXP)
    {
        return ((_PspTerminateThreadByPointer51)PspTerminateThreadByPointer_I)(
            Thread,
            ExitStatus
            );
    }
    else if (
        KphDynNtVersion == PHNT_WS03 ||
        KphDynNtVersion == PHNT_VISTA ||
        KphDynNtVersion == PHNT_WIN7
        )
    {
        return ((_PspTerminateThreadByPointer52)PspTerminateThreadByPointer_I)(
            Thread,
            ExitStatus,
            Thread == PsGetCurrentThread()
            );
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }
}

/**
 * Terminates a thread.
 *
 * \param ThreadHandle A handle to a thread.
 * \param ExitStatus A status value which indicates why the thread 
 * is being terminated.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiTerminateThread(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
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

    if (thread != PsGetCurrentThread())
    {
        status = KphTerminateThreadByPointerInternal(thread, ExitStatus);
    }
    else
    {
        status = STATUS_CANT_TERMINATE_SELF;
    }

    ObDereferenceObject(thread);

    return status;
}

/**
 * Terminates a thread using an unsafe method.
 *
 * \param ThreadHandle A handle to a thread.
 * \param ExitStatus A status value which indicates why the thread 
 * is being terminated.
 * \param AccessMode The mode in which to perform access checks.
 *
 * \remarks The thread will be terminated even if it is currently 
 * running kernel-mode code. Therefore, resources may be leaked 
 * or remain locked indefinitely.
 */
NTSTATUS KpiTerminateThreadUnsafe(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
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

    if (thread != PsGetCurrentThread())
    {
        EXIT_THREAD_CONTEXT context;

        // Initialize the context structure.
        context.ExitStatus = ExitStatus;
        KeInitializeEvent(&context.CompletedEvent, NotificationEvent, FALSE);

        KeInitializeApc(
            &context.Apc,
            (PKTHREAD)thread,
            OriginalApcEnvironment,
            KphpExitThreadSpecialApc,
            NULL,
            NULL,
            KernelMode,
            NULL
            );

        // Queue the APC.
        if (KeInsertQueueApc(&context.Apc, &context, NULL, 2))
        {
            // Wait for the APC procedure to finish retrieving information.
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
    else
    {
        status = STATUS_CANT_TERMINATE_SELF;
    }

    ObDereferenceObject(thread);

    return status;
}

VOID KphpExitThreadSpecialApc(
    __in PRKAPC Apc,
    __inout PKNORMAL_ROUTINE *NormalRoutine,
    __inout PVOID *NormalContext,
    __inout PVOID *SystemArgument1,
    __inout PVOID *SystemArgument2
    )
{
    PEXIT_THREAD_CONTEXT context = *SystemArgument1;
    NTSTATUS exitStatus;

    PAGED_CODE();

    exitStatus = context->ExitStatus;
    // That's the best we can do. Once we exit the current thread we can't
    // signal the event, so just signal it now.
    KeSetEvent(&context->CompletedEvent, 0, FALSE);
    // Exit the thread.
    KphTerminateThreadByPointerInternal(PsGetCurrentThread(), exitStatus);
}

/**
 * Gets the context of a thread.
 *
 * \param ThreadHandle A handle to a thread.
 * \param ThreadContext A pointer to a context structure. \a ContextFlags must be 
 * set.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiGetContextThread(
    __in HANDLE ThreadHandle,
    __inout PCONTEXT ThreadContext,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
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

    status = PsGetContextThread(thread, ThreadContext, AccessMode);
    ObDereferenceObject(thread);

    return status;
}

/**
 * Sets the context of a thread.
 *
 * \param ThreadHandle A handle to a thread.
 * \param ThreadContext The new context of the thread.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiSetContextThread(
    __in HANDLE ThreadHandle,
    __in PCONTEXT ThreadContext,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
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

    status = PsSetContextThread(thread, ThreadContext, AccessMode);
    ObDereferenceObject(thread);

    return status;
}

/**
 * Captures a stack trace of the current thread.
 *
 * \param FramesToSkip The number of frames to skip from the 
 * bottom of the stack.
 * \param FramesToCapture The number of frames to capture.
 * \param Flags A combination of the following:
 * \li \c RTL_WALK_USER_MODE_STACK The user-mode stack will 
 * be retrieved instead of the kernel-mode stack.
 * \param BackTrace An array in which the stack trace will be 
 * stored.
 * \param BackTraceHash A variable which receives a hash of 
 * the stack trace.
 *
 * \return The number of frames captured.
 */
ULONG KphCaptureStackBackTrace(
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __in_opt ULONG Flags,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PULONG BackTraceHash
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
    // Validate the flags.
    if ((Flags & RTL_WALK_VALID_FLAGS) != Flags)
        return 0;

    // Walk the stack.
    framesFound = RtlWalkFrameChain(
        backTrace,
        FramesToCapture + FramesToSkip,
        Flags
        );
    // Return nothing if we found fewer frames than we wanted to skip.
    if (framesFound <= FramesToSkip)
        return 0;

    // Copy over the stack trace.
    // At the same time we calculate the stack trace hash by 
    // summing the addresses.
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
 * \param FramesToSkip The number of frames to skip from the 
 * bottom of the stack.
 * \param FramesToCapture The number of frames to capture.
 * \param Flags A combination of the following:
 * \li \c RTL_WALK_USER_MODE_STACK The user-mode stack will 
 * be retrieved instead of the kernel-mode stack.
 * \param BackTrace An array in which the stack trace will be 
 * stored.
 * \param BackTraceHash A variable which receives a hash of 
 * the stack trace.
 * \param AccessMode The mode in which to perform access checks.
 *
 * \return The number of frames captured.
 */
NTSTATUS KphCaptureStackBackTraceThread(
    __in PETHREAD Thread,
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PULONG CapturedFrames,
    __out_opt PULONG BackTraceHash,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CAPTURE_BACKTRACE_THREAD_CONTEXT context;
    ULONG backTraceSize;
    PVOID *backTrace;

    PAGED_CODE();

    // Make sure the caller didn't request too many frames.
    // This also restricts the amount of memory we will try to 
    // allocate later.
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
    backTrace = ExAllocatePoolWithTag(NonPagedPool, backTraceSize, 'bhpK');

    if (!backTrace)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Initialize the context structure.
    context.FramesToSkip = FramesToSkip;
    context.FramesToCapture = FramesToCapture;
    context.BackTrace = backTrace;

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
    __in PRKAPC Apc,
    __inout PKNORMAL_ROUTINE *NormalRoutine,
    __inout PVOID *NormalContext,
    __inout PVOID *SystemArgument1,
    __inout PVOID *SystemArgument2
    )
{
    PCAPTURE_BACKTRACE_THREAD_CONTEXT context = *SystemArgument1;

    PAGED_CODE();

    context->CapturedFrames = KphCaptureStackBackTrace(
        context->FramesToSkip,
        context->FramesToCapture,
        0,
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
 * \param ThreadHandle A handle to the thread to capture the 
 * stack trace of.
 * \param FramesToSkip The number of frames to skip from the 
 * bottom of the stack.
 * \param FramesToCapture The number of frames to capture.
 * \param Flags A combination of the following:
 * \li \c RTL_WALK_USER_MODE_STACK The user-mode stack will 
 * be retrieved instead of the kernel-mode stack.
 * \param BackTrace An array in which the stack trace will be 
 * stored.
 * \param BackTraceHash A variable which receives a hash of 
 * the stack trace.
 * \param AccessMode The mode in which to perform access checks.
 *
 * \return The number of frames captured.
 */
NTSTATUS KpiCaptureStackBackTraceThread(
    __in HANDLE ThreadHandle,
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PULONG CapturedFrames,
    __out_opt PULONG BackTraceHash,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD thread;

    PAGED_CODE();

    status = ObReferenceObjectByHandle(
        ThreadHandle,
        0,
        *PsThreadType,
        KernelMode,
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
        AccessMode
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
 * \param ThreadInformationLength The number of bytes available in 
 * \a ThreadInformation.
 * \param ReturnLength A variable which receives the number of bytes 
 * required to be available in \a ThreadInformation.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiQueryInformationThread(
    __in HANDLE ThreadHandle,
    __in KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
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
        0,
        *PsThreadType,
        AccessMode,
        &thread,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    switch (ThreadInformationClass)
    {
    case KphThreadWin32Thread:
        {
            PVOID win32Thread;

            win32Thread = PsGetThreadWin32Thread(thread);

            if (ThreadInformationLength == sizeof(PVOID))
            {
                __try
                {
                    *(PVOID *)ThreadInformation = win32Thread;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }
            else
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            returnLength = sizeof(PVOID);
        }
        break;
    case KphThreadIoPriority:
        {
            HANDLE newThreadHandle;
            ULONG ioPriority;

            if (NT_SUCCESS(status = ObOpenObjectByPointer(
                thread,
                OBJ_KERNEL_HANDLE,
                NULL,
                THREAD_QUERY_INFORMATION,
                *PsThreadType,
                KernelMode,
                &newThreadHandle
                )))
            {
                if (NT_SUCCESS(status = ZwQueryInformationThread(
                    newThreadHandle,
                    ThreadIoPriority,
                    &ioPriority,
                    sizeof(ULONG),
                    NULL
                    )))
                {
                    if (ThreadInformationLength == sizeof(ULONG))
                    {
                        __try
                        {
                            *(PULONG)ThreadInformation = ioPriority;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            status = GetExceptionCode();
                        }
                    }
                    else
                    {
                        status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                }

                ZwClose(newThreadHandle);
            }

            returnLength = sizeof(ULONG);
        }
        break;
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
 * \param ThreadInformationLength The number of bytes present in 
 * \a ThreadInformation.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiSetInformationThread(
    __in HANDLE ThreadHandle,
    __in KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    __in_bcount(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __in KPROCESSOR_MODE AccessMode
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
        0,
        *PsThreadType,
        AccessMode,
        &thread,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    switch (ThreadInformationClass)
    {
    case KphThreadImpersonationToken:
        {
            HANDLE tokenHandle;
            PACCESS_TOKEN token;
            HANDLE newTokenHandle;

            if (ThreadInformationLength == sizeof(HANDLE))
            {
                __try
                {
                    tokenHandle = *(PHANDLE)ThreadInformation;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }
            else
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            if (NT_SUCCESS(status))
            {
                if (NT_SUCCESS(status = ObReferenceObjectByHandle(
                    tokenHandle,
                    TOKEN_IMPERSONATE,
                    *SeTokenObjectType,
                    AccessMode,
                    &token,
                    NULL
                    )))
                {
                    if (NT_SUCCESS(status = ObOpenObjectByPointer(
                        token,
                        OBJ_KERNEL_HANDLE,
                        NULL,
                        TOKEN_IMPERSONATE,
                        *SeTokenObjectType,
                        KernelMode,
                        &newTokenHandle
                        )))
                    {
                        status = PsAssignImpersonationToken(thread, newTokenHandle);
                        ZwClose(newTokenHandle);
                    }

                    ObDereferenceObject(token);
                }
            }
        }
        break;
    case KphThreadIoPriority:
        {
            ULONG ioPriority;
            HANDLE newThreadHandle;

            if (ThreadInformationLength == sizeof(ULONG))
            {
                __try
                {
                    ioPriority = *(PULONG)ThreadInformation;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }
            else
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            if (NT_SUCCESS(status))
            {
                if (NT_SUCCESS(status = ObOpenObjectByPointer(
                    thread,
                    OBJ_KERNEL_HANDLE,
                    NULL,
                    THREAD_SET_INFORMATION,
                    *PsThreadType,
                    KernelMode,
                    &newThreadHandle
                    )))
                {
                    status = ZwSetInformationThread(
                        newThreadHandle,
                        ThreadIoPriority,
                        &ioPriority,
                        sizeof(ULONG)
                        );
                    ZwClose(newThreadHandle);
                }
            }
        }
        break;
    default:
        status = STATUS_INVALID_INFO_CLASS;
        break;
    }

    ObDereferenceObject(thread);

    return status;
}
