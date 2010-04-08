/*
 * Process Hacker Driver - 
 *   processes and threads
 * 
 * Copyright (C) 2009 wj32
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

#include "include/kph.h"
#include "include/ke.h"
#include "include/ps.h"

VOID NTAPI KphpCaptureStackBackTraceThreadSpecialApc(
    PKAPC Apc,
    PKNORMAL_ROUTINE *NormalRoutine,
    PVOID *NormalContext,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    );

VOID NTAPI KphpExitSpecialApc(
    PKAPC Apc,
    PKNORMAL_ROUTINE *NormalRoutine,
    PVOID *NormalContext,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphAssignImpersonationToken)
#pragma alloc_text(PAGE, KphCaptureStackBackTraceThread)
#pragma alloc_text(PAGE, KphpCaptureStackBackTraceThread)
#pragma alloc_text(PAGE, KphpCaptureStackBackTraceThreadSpecialApc)
#pragma alloc_text(PAGE, KphDangerousTerminateThread)
#pragma alloc_text(PAGE, KphpExitSpecialApc)
#pragma alloc_text(PAGE, KphGetContextThread)
#pragma alloc_text(PAGE, KphGetProcessId)
#pragma alloc_text(PAGE, KphGetThreadId)
#pragma alloc_text(PAGE, KphGetThreadWin32Thread)
#pragma alloc_text(PAGE, KphOpenProcess)
#pragma alloc_text(PAGE, KphOpenProcessJob)
#pragma alloc_text(PAGE, KphOpenThread)
#pragma alloc_text(PAGE, KphOpenThreadProcess)
#pragma alloc_text(PAGE, KphResumeProcess)
#pragma alloc_text(PAGE, KphSetContextThread)
#pragma alloc_text(PAGE, KphSuspendProcess)
#pragma alloc_text(PAGE, KphResumeProcess)
#pragma alloc_text(PAGE, KphTerminateProcess)
#pragma alloc_text(PAGE, KphTerminateThread)
#pragma alloc_text(PAGE, PsTerminateProcess)
#pragma alloc_text(PAGE, PspTerminateThreadByPointer)
#endif

/* KphAcquireProcessRundownProtection
 * 
 * Prevents the process from terminating.
 */
BOOLEAN KphAcquireProcessRundownProtection(
    __in PEPROCESS Process
    )
{
    return ExAcquireRundownProtection((PEX_RUNDOWN_REF)KVOFF(Process, OffEpRundownProtect));
}

/* KphAssignImpersonationToken
 * 
 * Assigns an impersonation token to the specified thread.
 */
NTSTATUS KphAssignImpersonationToken(
    __in HANDLE ThreadHandle,
    __in HANDLE TokenHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD threadObject;
    
    status = ObReferenceObjectByHandle(
        ThreadHandle,
        0,
        *PsThreadType,
        KernelMode,
        &threadObject,
        NULL
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    status = PsAssignImpersonationToken(threadObject, TokenHandle);
    ObDereferenceObject(threadObject);
    
    return status;
}

/* KphCaptureStackBackTraceThread
 * 
 * Captures a kernel-mode stack backtrace for the specified thread.
 */
NTSTATUS KphCaptureStackBackTraceThread(
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
    PETHREAD threadObject;
    
    /* Reference the thread. */
    status = ObReferenceObjectByHandle(
        ThreadHandle,
        THREAD_QUERY_INFORMATION,
        *PsThreadType,
        KernelMode,
        &threadObject,
        NULL
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Get the stack trace. */
    status = KphpCaptureStackBackTraceThread(
        threadObject,
        FramesToSkip,
        FramesToCapture,
        BackTrace,
        CapturedFrames,
        BackTraceHash,
        AccessMode
        );
    /* Dereference the thread. */
    ObDereferenceObject(threadObject);
    
    return status;
}

/* KphpCaptureStackBackTraceThread
 * 
 * Captures a kernel-mode stack backtrace for the specified thread.
 * 
 * IRQL: <= APC_LEVEL
 */
NTSTATUS KphpCaptureStackBackTraceThread(
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
    
    backTraceSize = FramesToCapture * sizeof(PVOID);
    
    /* Probe user input. */
    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(BackTrace, backTraceSize, 1);
            
            if (CapturedFrames)
                ProbeForWrite(CapturedFrames, sizeof(ULONG), 1);
            if (BackTraceHash)
                ProbeForWrite(BackTraceHash, sizeof(ULONG), 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    
    /* Allocate storage for the stack trace. */
    backTrace = (PVOID *)ExAllocatePoolWithTag(NonPagedPool, backTraceSize, TAG_CAPTURE_STACK_BACKTRACE);
    
    if (!backTrace)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    /* Initialize the context structure. */
    context.FramesToSkip = FramesToSkip;
    context.FramesToCapture = FramesToCapture;
    context.BackTrace = backTrace;
    
    /* Check if we're trying to get a stack trace of the current thread. */
    if (Thread == PsGetCurrentThread())
    {
        PCAPTURE_BACKTRACE_THREAD_CONTEXT contextPtr = &context;
        PVOID dummy = NULL;
        KIRQL oldIrql;
        
        context.Local = TRUE;
        /* Raise the IRQL to APC_LEVEL to simulate an APC environment. */
        KeRaiseIrql(APC_LEVEL, &oldIrql);
        /* Call the APC routine directly. */
        KphpCaptureStackBackTraceThreadSpecialApc(
            &context.Apc,
            NULL,
            NULL,
            &contextPtr,
            &dummy
            );
        /* Lower the IRQL back. */
        KeLowerIrql(oldIrql);
    }
    else
    {
        context.Local = FALSE;
        /* Initialize the stack trace completed event. */
        KeInitializeEvent(&context.CompletedEvent, NotificationEvent, FALSE);
        /* Initialize the APC. */
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
        /* Queue the APC. */
        if (KeInsertQueueApc(&context.Apc, &context, NULL, 2))
        {
            /* Wait for the APC to complete. */
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
        
        /* Write the information. */
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
    
    /* Free the allocated stack trace storage. */
    ExFreePoolWithTag(backTrace, TAG_CAPTURE_STACK_BACKTRACE);
    
    return status;
}

/* KphpCaptureStackBackTraceThreadSpecialApc
 * 
 * The special APC routine which captures a thread stack trace.
 */
VOID NTAPI KphpCaptureStackBackTraceThreadSpecialApc(
    PKAPC Apc,
    PKNORMAL_ROUTINE *NormalRoutine,
    PVOID *NormalContext,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    )
{
    PCAPTURE_BACKTRACE_THREAD_CONTEXT context = 
        (PCAPTURE_BACKTRACE_THREAD_CONTEXT)*SystemArgument1;
    
    /* Capture a stack trace. */
    context->CapturedFrames = KphCaptureStackBackTrace(
        context->FramesToSkip,
        context->FramesToCapture,
        0,
        context->BackTrace,
        &context->BackTraceHash
        );
    
    if (!context->Local)
    {
        /* Signal the completed event. */
        KeSetEvent(&context->CompletedEvent, 0, FALSE);
    }
}

/* KphDangerousTerminateThread
 * 
 * Terminates the specified thread by queueing an APC.
 */
NTSTATUS KphDangerousTerminateThread(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD threadObject;
    
    if (!__PspTerminateThreadByPointer)
        return STATUS_NOT_SUPPORTED;
    
    status = ObReferenceObjectByHandle(
        ThreadHandle,
        THREAD_TERMINATE,
        *PsThreadType,
        KernelMode,
        &threadObject,
        NULL
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    if (threadObject != PsGetCurrentThread())
    {
        EXIT_THREAD_CONTEXT context;
        
        /* Initialize the context structure. */
        context.ExitStatus = ExitStatus;
        /* Initialize the completion event. */
        KeInitializeEvent(&context.CompletedEvent, NotificationEvent, FALSE);
        /* Initialize the APC. */
        KeInitializeApc(
            &context.Apc,
            (PKTHREAD)threadObject,
            OriginalApcEnvironment,
            KphpExitSpecialApc,
            NULL,
            NULL,
            KernelMode,
            NULL
            );
        
        /* Queue the APC. */
        if (KeInsertQueueApc(&context.Apc, &context, NULL, 2))
        {
            /* Wait for the APC to initialize. */
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
        
        ObDereferenceObject(threadObject);
    }
    else
    {
        /* Can't terminate self. */
        ObDereferenceObject(threadObject);
        return STATUS_CANT_TERMINATE_SELF;
    }
    
    return status;
}

VOID NTAPI KphpExitSpecialApc(
    PKAPC Apc,
    PKNORMAL_ROUTINE *NormalRoutine,
    PVOID *NormalContext,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    )
{
    PEXIT_THREAD_CONTEXT context = 
        (PEXIT_THREAD_CONTEXT)*SystemArgument1;
    NTSTATUS exitStatus;
    
    /* Get the exit status. */
    exitStatus = context->ExitStatus;
    /* That's the best we can do. Once we exit the current thread we can't 
     * signal the event, so just signal it now. */
    KeSetEvent(&context->CompletedEvent, 0, FALSE);
    /* Exit the thread by calling PspTerminateThreadByPointer. */
    PspTerminateThreadByPointer(PsGetCurrentThread(), exitStatus);
    /* Should never happen. */
    dfprintf(
        "WARNING: Thread was not terminated by PspTerminateThreadByPointer: %d, %#x\n",
        PsGetCurrentThreadId(),
        PsGetCurrentThread()
        );
}

/* KphGetContextThread
 * 
 * Gets the context of the specified thread.
 */
NTSTATUS KphGetContextThread(
    __in HANDLE ThreadHandle,
    __inout PCONTEXT ThreadContext,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD threadObject;
    
    status = ObReferenceObjectByHandle(
        ThreadHandle,
        THREAD_GET_CONTEXT,
        *PsThreadType,
        KernelMode,
        &threadObject,
        NULL
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    status = PsGetContextThread(threadObject, ThreadContext, AccessMode);
    ObDereferenceObject(threadObject);
    
    return status;
}

/* KphGetProcessId
 * 
 * Gets the ID of the process referenced by the specified handle.
 */
HANDLE KphGetProcessId(
    __in HANDLE ProcessHandle
    )
{
    PEPROCESS processObject;
    HANDLE processId;
    
    if (!NT_SUCCESS(ObReferenceObjectByHandle(ProcessHandle, 0,
        *PsProcessType, KernelMode, &processObject, NULL)))
        return 0;
    
    processId = PsGetProcessId(processObject);
    ObDereferenceObject(processObject);
    
    return processId;
}

/* KphGetThreadId
 * 
 * Gets the ID of the thread referenced by the specified handle, 
 * and optionally the ID of the thread's process.
 */
HANDLE KphGetThreadId(
    __in HANDLE ThreadHandle,
    __out_opt PHANDLE ProcessId
    )
{
    PETHREAD threadObject;
    CLIENT_ID clientId;
    
    if (!NT_SUCCESS(ObReferenceObjectByHandle(ThreadHandle, 0, 
        *PsThreadType, KernelMode, &threadObject, NULL)))
        return 0;
    
    clientId = *(PCLIENT_ID)KVOFF(threadObject, OffEtClientId);
    
    ObDereferenceObject(threadObject);
    
    if (ProcessId)
    {
        *ProcessId = clientId.UniqueProcess;
    }
    
    return clientId.UniqueThread;
}

/* KphGetThreadWin32Thread
 * 
 * Gets a pointer to the WIN32THREAD structure of the specified thread.
 */
NTSTATUS KphGetThreadWin32Thread(
    __in HANDLE ThreadHandle,
    __out PVOID *Win32Thread,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD threadObject;
    PVOID win32Thread;
    
    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(Win32Thread, sizeof(PVOID), 1);
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
        KernelMode,
        &threadObject,
        NULL
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    win32Thread = PsGetThreadWin32Thread(threadObject);
    ObDereferenceObject(threadObject);
    
    __try
    {
        *Win32Thread = win32Thread;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
    
    return status;
}

/* KphOpenProcess
 * 
 * Opens a process.
 */
NTSTATUS KphOpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PCLIENT_ID ClientId,
    __in KPROCESSOR_MODE AccessMode
    )
{
    BOOLEAN hasObjectName = ObjectAttributes->ObjectName != NULL;
    ULONG attributes = ObjectAttributes->Attributes;
    NTSTATUS status = STATUS_SUCCESS;
    ACCESS_STATE accessState;
    CHAR auxData[AUX_ACCESS_DATA_SIZE];
    PEPROCESS processObject = NULL;
    PETHREAD threadObject = NULL;
    HANDLE processHandle = NULL;
    
    if (hasObjectName && ClientId)
        return STATUS_INVALID_PARAMETER_MIX;
    
    /* ReactOS code cleared this bit up for me :) */
    status = SeCreateAccessState(
        &accessState,
        (PAUX_ACCESS_DATA)auxData,
        DesiredAccess,
        (PGENERIC_MAPPING)KVOFF(*PsProcessType, OffOtiGenericMapping)
        );
    
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    /* Let's hope our client isn't a virus... */
    if (accessState.RemainingDesiredAccess & MAXIMUM_ALLOWED)
        accessState.PreviouslyGrantedAccess |= ProcessAllAccess;
    else
        accessState.PreviouslyGrantedAccess |= accessState.RemainingDesiredAccess;
    
    accessState.RemainingDesiredAccess = 0;
    
    if (hasObjectName)
    {
        status = ObOpenObjectByName(
            ObjectAttributes,
            *PsProcessType,
            AccessMode,
            &accessState,
            0,
            NULL,
            &processHandle
            );
        SeDeleteAccessState(&accessState);
    }
    else if (ClientId)
    {
        if (ClientId->UniqueThread)
        {
            status = PsLookupProcessThreadByCid(ClientId, &processObject, &threadObject);
        }
        else
        {
            status = PsLookupProcessByProcessId(ClientId->UniqueProcess, &processObject);
        }
        
        if (!NT_SUCCESS(status))
        {
            SeDeleteAccessState(&accessState);
            return status;
        }
        
        status = ObOpenObjectByPointer(
            processObject,
            attributes,
            &accessState,
            0,
            *PsProcessType,
            AccessMode,
            &processHandle
            );
        
        SeDeleteAccessState(&accessState);
        ObDereferenceObject(processObject);
        
        if (threadObject)
            ObDereferenceObject(threadObject);
    }
    else
    {
        SeDeleteAccessState(&accessState);
        return STATUS_INVALID_PARAMETER_MIX;
    }
    
    if (NT_SUCCESS(status))
    {
        *ProcessHandle = processHandle;
    }
    
    return status;
}

/* KphOpenProcessJob
 * 
 * Opens the specified process' job object. If the process has 
 * not been assigned to a job object, the function returns 
 * STATUS_PROCESS_NOT_IN_JOB.
 */
NTSTATUS KphOpenProcessJob(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE JobHandle,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS processObject;
    PVOID jobObject;
    HANDLE jobHandle;
    ACCESS_STATE accessState;
    CHAR auxData[AUX_ACCESS_DATA_SIZE];
    
    status = SeCreateAccessState(
        &accessState,
        (PAUX_ACCESS_DATA)auxData,
        DesiredAccess,
        (PGENERIC_MAPPING)KVOFF(*PsJobType, OffOtiGenericMapping)
        );
    
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    if (accessState.RemainingDesiredAccess & MAXIMUM_ALLOWED)
        accessState.PreviouslyGrantedAccess |= JOB_OBJECT_ALL_ACCESS;
    else
        accessState.PreviouslyGrantedAccess |= accessState.RemainingDesiredAccess;
    
    accessState.RemainingDesiredAccess = 0;
    
    status = ObReferenceObjectByHandle(ProcessHandle, 0, *PsProcessType, KernelMode, &processObject, 0);
    
    if (!NT_SUCCESS(status))
    {
        SeDeleteAccessState(&accessState);
        return status;
    }
    
    /* If we have PsGetProcessJob, use it. Otherwise, read the EPROCESS structure. */
    if (PsGetProcessJob)
    {
        jobObject = PsGetProcessJob(processObject);
    }
    else
    {
        jobObject = *(PVOID *)((PCHAR)processObject + OffEpJob);
    }
    
    ObDereferenceObject(processObject);
    
    if (jobObject == NULL)
    {
        /* No such job. Output a NULL handle and exit. */
        SeDeleteAccessState(&accessState);
        *JobHandle = NULL;
        return STATUS_PROCESS_NOT_IN_JOB;
    }
    
    ObReferenceObject(jobObject);
    status = ObOpenObjectByPointer(
        jobObject,
        0,
        &accessState,
        0,
        *PsJobType,
        AccessMode,
        &jobHandle
        );
    SeDeleteAccessState(&accessState);
    ObDereferenceObject(jobObject);
    
    if (NT_SUCCESS(status))
        *JobHandle = jobHandle;
    
    return status;
}

/* KphOpenThread
 * 
 * Opens a thread.
 */
NTSTATUS KphOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PCLIENT_ID ClientId,
    __in KPROCESSOR_MODE AccessMode
    )
{
    BOOLEAN hasObjectName = ObjectAttributes->ObjectName != NULL;
    ULONG attributes = ObjectAttributes->Attributes;
    NTSTATUS status = STATUS_SUCCESS;
    ACCESS_STATE accessState;
    CHAR auxData[AUX_ACCESS_DATA_SIZE];
    PETHREAD threadObject = NULL;
    HANDLE threadHandle = NULL;
    
    if (hasObjectName && ClientId)
        return STATUS_INVALID_PARAMETER_MIX;
    
    status = SeCreateAccessState(
        &accessState,
        (PAUX_ACCESS_DATA)auxData,
        DesiredAccess,
        (PGENERIC_MAPPING)KVOFF(*PsThreadType, OffOtiGenericMapping)
        );
    
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    if (accessState.RemainingDesiredAccess & MAXIMUM_ALLOWED)
        accessState.PreviouslyGrantedAccess |= ThreadAllAccess;
    else
        accessState.PreviouslyGrantedAccess |= accessState.RemainingDesiredAccess;
    
    accessState.RemainingDesiredAccess = 0;
    
    if (hasObjectName)
    {
        status = ObOpenObjectByName(
            ObjectAttributes,
            *PsThreadType,
            AccessMode,
            &accessState,
            0,
            NULL,
            &threadHandle
            );
        SeDeleteAccessState(&accessState);
    }
    else if (ClientId)
    {
        if (ClientId->UniqueProcess)
        {
            status = PsLookupProcessThreadByCid(ClientId, NULL, &threadObject);
        }
        else
        {
            status = PsLookupThreadByThreadId(ClientId->UniqueThread, &threadObject);
        }
        
        if (!NT_SUCCESS(status))
        {
            SeDeleteAccessState(&accessState);
            return status;
        }
        
        status = ObOpenObjectByPointer(
            threadObject,
            attributes,
            &accessState,
            0,
            *PsThreadType,
            AccessMode,
            &threadHandle
            );
        
        SeDeleteAccessState(&accessState);
        ObDereferenceObject(threadObject);
    }
    else
    {
        SeDeleteAccessState(&accessState);
        return STATUS_INVALID_PARAMETER_MIX;
    }
    
    if (NT_SUCCESS(status))
    {
        *ThreadHandle = threadHandle;
    }
    
    return status;
}

/* KphOpenThreadProcess
 * 
 * Opens a thread's process.
 */
NTSTATUS KphOpenThreadProcess(
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD threadObject;
    PEPROCESS processObject;
    HANDLE processHandle;
    ACCESS_STATE accessState;
    CHAR auxData[AUX_ACCESS_DATA_SIZE];
    
    status = SeCreateAccessState(
        &accessState,
        (PAUX_ACCESS_DATA)auxData,
        DesiredAccess,
        (PGENERIC_MAPPING)KVOFF(*PsProcessType, OffOtiGenericMapping)
        );
    
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    if (accessState.RemainingDesiredAccess & MAXIMUM_ALLOWED)
        accessState.PreviouslyGrantedAccess |= ProcessAllAccess;
    else
        accessState.PreviouslyGrantedAccess |= accessState.RemainingDesiredAccess;
    
    accessState.RemainingDesiredAccess = 0;
    
    status = ObReferenceObjectByHandle(ThreadHandle, 0, *PsThreadType, KernelMode, &threadObject, 0);
    
    if (!NT_SUCCESS(status))
    {
        SeDeleteAccessState(&accessState);
        return status;
    }
    
    /* Get the process object. */
    processObject = IoThreadToProcess(threadObject);
    ObDereferenceObject(threadObject);
    
    if (processObject == NULL)
    {
        /* Thread does not have a process (?). */
        SeDeleteAccessState(&accessState);
        *ProcessHandle = NULL;
        return STATUS_UNSUCCESSFUL;
    }
    
    ObReferenceObject(processObject);
    status = ObOpenObjectByPointer(
        processObject,
        0,
        &accessState,
        0,
        *PsProcessType,
        AccessMode,
        &processHandle
        );
    SeDeleteAccessState(&accessState);
    ObDereferenceObject(processObject);
    
    if (NT_SUCCESS(status))
        *ProcessHandle = processHandle;
    
    return status;
}

/* KphReleaseProcessRundownProtection
 * 
 * Allows the process to terminate.
 */
VOID KphReleaseProcessRundownProtection(
    __in PEPROCESS Process
    )
{
    ExReleaseRundownProtection((PEX_RUNDOWN_REF)KVOFF(Process, OffEpRundownProtect));
}

/* KphResumeProcess
 * 
 * Resumes the specified process.
 */
NTSTATUS KphResumeProcess(
    __in HANDLE ProcessHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS processObject;
    
    if (!PsResumeProcess)
        return STATUS_NOT_SUPPORTED;
    
    status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_SUSPEND_RESUME,
        *PsProcessType,
        KernelMode,
        &processObject,
        NULL);
    
    if (!NT_SUCCESS(status))
        return status;
    
    status = PsResumeProcess(processObject);
    ObDereferenceObject(processObject);
    
    return status;
}

/* KphSetContextThread
 * 
 * Sets the context of the specified thread.
 */
NTSTATUS KphSetContextThread(
    __in HANDLE ThreadHandle,
    __in PCONTEXT ThreadContext,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD threadObject;
    
    status = ObReferenceObjectByHandle(
        ThreadHandle,
        THREAD_SET_CONTEXT,
        *PsThreadType,
        KernelMode,
        &threadObject,
        NULL);
    
    if (!NT_SUCCESS(status))
        return status;
    
    status = PsSetContextThread(threadObject, ThreadContext, AccessMode);
    ObDereferenceObject(threadObject);
    
    return status;
}

/* KphSuspendProcess
 * 
 * Suspends the specified process.
 */
NTSTATUS KphSuspendProcess(
    __in HANDLE ProcessHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS processObject;
    
    if (!PsSuspendProcess)
        return STATUS_NOT_SUPPORTED;
    
    status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_SUSPEND_RESUME,
        *PsProcessType,
        KernelMode,
        &processObject,
        NULL);
    
    if (!NT_SUCCESS(status))
        return status;
    
    status = PsSuspendProcess(processObject);
    ObDereferenceObject(processObject);
    
    return status;
}

/* KphTerminateProcess
 * 
 * Terminates the specified process.
 */
NTSTATUS KphTerminateProcess(
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS processObject;
    
    status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_TERMINATE,
        *PsProcessType,
        KernelMode,
        &processObject,
        NULL);
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Can't terminate ourself. Get user-mode to do it. */
    if (processObject == PsGetCurrentProcess())
    {
        ObDereferenceObject(processObject);
        return STATUS_CANT_TERMINATE_SELF;
    }
    
    /* If we have located PsTerminateProcess/PspTerminateProcess, 
       call it. */
    if (__PsTerminateProcess)
    {
        status = PsTerminateProcess(processObject, ExitStatus);
        ObDereferenceObject(processObject);
    }
    else
    {
        /* Otherwise, we'll have to call ZwTerminateProcess - most hooks on this function 
           allow kernel-mode callers through. */
        OBJECT_ATTRIBUTES objectAttributes = { 0 };
        CLIENT_ID clientId;
        HANDLE newProcessHandle;
        
        /* We have to open it again because ZwTerminateProcess only accepts kernel handles. */
        clientId.UniqueThread = 0;
        clientId.UniqueProcess = PsGetProcessId(processObject);
        status = KphOpenProcess(&newProcessHandle, 0x1, &objectAttributes, &clientId, KernelMode);
        ObDereferenceObject(processObject);
        
        if (NT_SUCCESS(status))
        {
            status = ZwTerminateProcess(newProcessHandle, ExitStatus);
            ZwClose(newProcessHandle);
        }
    }
    
    return status;
}

/* KphTerminateThread
 * 
 * Terminates the specified thread.
 */
NTSTATUS KphTerminateThread(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PETHREAD threadObject;
    
    status = ObReferenceObjectByHandle(
        ThreadHandle,
        THREAD_TERMINATE,
        *PsThreadType,
        KernelMode,
        &threadObject,
        NULL);
    
    if (!NT_SUCCESS(status))
        return status;
    
    if (threadObject != PsGetCurrentThread())
    {
        status = PspTerminateThreadByPointer(threadObject, ExitStatus);
        ObDereferenceObject(threadObject);
    }
    else
    {/*
        ObDereferenceObject(threadObject);
        status = PspTerminateThreadByPointer(PsGetCurrentThread(), ExitStatus); */
        /* Leads to bugs, so don't terminate self. */
        ObDereferenceObject(threadObject);
        return STATUS_CANT_TERMINATE_SELF;
    }
    
    return status;
}

/* PsTerminateProcess
 * 
 * Terminates the specified process. If PsTerminateProcess or 
 * PspTerminateProcess could not be located, the call will fail 
 * with STATUS_NOT_SUPPORTED.
 */
NTSTATUS PsTerminateProcess(
    __in PEPROCESS Process,
    __in NTSTATUS ExitStatus
    )
{
    PVOID psTerminateProcess = __PsTerminateProcess;
    NTSTATUS status;
    
    if (!psTerminateProcess)
        return STATUS_NOT_SUPPORTED;
    
#ifdef _X86_
    if (
        WindowsVersion == WINDOWS_XP || 
        WindowsVersion == WINDOWS_SERVER_2003
        )
    {
        /* PspTerminateProcess on XP and Server 2003 is stdcall. */
        __asm
        {
            push    [ExitStatus]
            push    [Process]
            call    [psTerminateProcess]
            mov     [status], eax
        }
    }
    else if (
        WindowsVersion == WINDOWS_VISTA || 
        WindowsVersion == WINDOWS_7
        )
    {
        /* PsTerminateProcess on Vista and above is thiscall. */
        __asm
        {
            push    [ExitStatus]
            mov     ecx, [Process]
            call    [psTerminateProcess]
            mov     [status], eax
        }
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }
#else
    status = __PsTerminateProcess(Process, ExitStatus);
#endif
    
    return status;
}

/* PspTerminateThreadByPointer
 * 
 * Terminates the specified thread. If PspTerminateThreadByPointer 
 * could not be located, the call will fail with STATUS_NOT_SUPPORTED.
 */
NTSTATUS PspTerminateThreadByPointer(
    __in PETHREAD Thread,
    __in NTSTATUS ExitStatus
    )
{
    PVOID pspTerminateThreadByPointer = __PspTerminateThreadByPointer;
    
    if (!pspTerminateThreadByPointer)
        return STATUS_NOT_SUPPORTED;
    
    if (WindowsVersion == WINDOWS_XP)
    {
        return ((_PspTerminateThreadByPointer51)pspTerminateThreadByPointer)(
            Thread,
            ExitStatus
            );
    }
    else if (
        WindowsVersion == WINDOWS_SERVER_2003 || 
        WindowsVersion == WINDOWS_VISTA || 
        WindowsVersion == WINDOWS_7
        )
    {
        return ((_PspTerminateThreadByPointer52)pspTerminateThreadByPointer)(
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
