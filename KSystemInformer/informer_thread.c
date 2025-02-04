/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *
 */

#include <kph.h>
#include <informer.h>
#include <comms.h>

#include <trace.h>

typedef enum _KPH_THREAD_NOTIFY_TYPE
{
    KphThreadNotifyCreate,
    KphThreadNotifyExecute,
    KphThreadNotifyExit
} KPH_THREAD_NOTIFY_TYPE;

KPH_PAGED_FILE();

/**
 * \brief Performing thread tracking.
 *
 * \param[in] ProcessId Process ID from the notification callback.
 * \param[in] ThreadId Thread ID from the notification callback.
 * \param[in] Create The create parameter from the notification callback.
 * \param[in] Thread The thread object of the thread ID.
 *
 * \return Pointer to the thread context, may be null, the caller should
 * dereference this object if it is non-null.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PKPH_THREAD_CONTEXT KphpPerformThreadTracking(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create,
    _In_ PETHREAD Thread
    )
{
    PKPH_THREAD_CONTEXT thread;

    KPH_PAGED_CODE_PASSIVE();

    if (!Create)
    {
        thread = KphUntrackThreadContext(ThreadId);
        if (!thread)
        {
            return NULL;
        }

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Stopped tracking thread %lu in process %wZ (%lu)",
                      HandleToULong(thread->ClientId.UniqueThread),
                      KphGetThreadImageName(thread),
                      HandleToULong(thread->ClientId.UniqueProcess));

        thread->ExitNotification = TRUE;

        return thread;
    }

    NT_ASSERT(Create);

    thread = KphTrackThreadContext(Thread);
    if (!thread)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Failed to track thread %lu in process %lu",
                      HandleToULong(ThreadId),
                      HandleToULong(ProcessId));

        return NULL;
    }

    thread->CreateNotification = TRUE;
    thread->CreatorClientId.UniqueProcess = PsGetCurrentProcessId();
    thread->CreatorClientId.UniqueThread = PsGetCurrentThreadId();

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  TRACKING,
                  "Tracking thread %lu in process %wZ (%lu)",
                  HandleToULong(thread->ClientId.UniqueThread),
                  KphGetThreadImageName(thread),
                  HandleToULong(thread->ClientId.UniqueProcess));

    return thread;
}

/**
 * \brief Informs any clients of thread notify routine invocations.
 *
 * \param[in] Thread The thread being created.
 * \param[in] ProcessId Process ID of the process where the thread is being
 * created.
 * \param[in] ThreadID Thread ID of the thread being created.
 * \param[in] Create If true the thread is being created, if false the thread
 * is being destroyed.
 */
_Function_class_(PCREATE_THREAD_NOTIFY_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpCreateThreadNotifyInformer(
    _In_ PKPH_THREAD_CONTEXT Thread,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ KPH_THREAD_NOTIFY_TYPE Type
    )
{
    PKPH_MESSAGE msg;
    PKPH_PROCESS_CONTEXT actorProcess;

    KPH_PAGED_CODE_PASSIVE();

    msg = NULL;
    actorProcess = KphGetCurrentProcessContext();

    if (Type == KphThreadNotifyCreate)
    {
        if (!KphInformerEnabled2(ThreadCreate, actorProcess, Thread->ProcessContext))
        {
            goto Exit;
        }

        msg = KphAllocateMessage();
        if (!msg)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "Failed to allocate message");
            goto Exit;
        }

        KphMsgInit(msg, KphMsgThreadCreate);
        msg->Kernel.ThreadCreate.CreatingClientId.UniqueProcess = PsGetCurrentProcessId();
        msg->Kernel.ThreadCreate.CreatingClientId.UniqueThread = PsGetCurrentThreadId();
        msg->Kernel.ThreadCreate.CreatingProcessStartKey = KphGetCurrentProcessStartKey();
        msg->Kernel.ThreadCreate.CreatingThreadSubProcessTag = KphGetCurrentThreadSubProcessTag();
        msg->Kernel.ThreadCreate.TargetClientId.UniqueProcess = ProcessId;
        msg->Kernel.ThreadCreate.TargetClientId.UniqueThread = ThreadId;
        msg->Kernel.ThreadCreate.TargetProcessStartKey = KphGetThreadProcessStartKey(Thread->EThread);
    }
    else if (Type == KphThreadNotifyExecute)
    {
        PVOID subProcessTag;

        //
        // Call this even if the informer is disabled since this is the best
        // place to update the cached sub-process tag in the thread context.
        //
        subProcessTag = KphGetCurrentThreadSubProcessTag();

        if (!KphInformerEnabled2(ThreadExecute, actorProcess, Thread->ProcessContext))
        {
            goto Exit;
        }

        msg = KphAllocateMessage();
        if (!msg)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "Failed to allocate message");
            goto Exit;
        }

        KphMsgInit(msg, KphMsgThreadExecute);
        msg->Kernel.ThreadExecute.ClientId.UniqueProcess = ProcessId;
        msg->Kernel.ThreadExecute.ClientId.UniqueThread = ThreadId;
        NT_ASSERT(ProcessId == PsGetCurrentProcessId());
        msg->Kernel.ThreadExecute.ProcessStartKey = KphGetCurrentProcessStartKey();
        msg->Kernel.ThreadExecute.ThreadSubProcessTag = subProcessTag;
    }
    else
    {
        NT_ASSERT(Type == KphThreadNotifyExit);

        if (!KphInformerEnabled2(ThreadExit, actorProcess, Thread->ProcessContext))
        {
            goto Exit;
        }

        msg = KphAllocateMessage();
        if (!msg)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "Failed to allocate message");
            goto Exit;
        }

        KphMsgInit(msg, KphMsgThreadExit);
        msg->Kernel.ThreadExit.ClientId.UniqueProcess = ProcessId;
        msg->Kernel.ThreadExit.ClientId.UniqueThread = ThreadId;
        msg->Kernel.ThreadExit.ExitStatus = PsGetThreadExitStatus(Thread->EThread);
        NT_ASSERT(ProcessId == PsGetCurrentProcessId());
        msg->Kernel.ThreadExit.ProcessStartKey = KphGetCurrentProcessStartKey();
        msg->Kernel.ThreadExit.ThreadSubProcessTag = KphGetCurrentThreadSubProcessTag();
    }

    if (KphInformerEnabled2(EnableStackTraces, actorProcess, Thread->ProcessContext))
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
    msg = NULL;

Exit:

    if (msg)
    {
        KphFreeMessage(msg);
    }

    if (actorProcess)
    {
        KphDereferenceObject(actorProcess);
    }
}

/**
 * \brief Thread execution notify routine.
 *
 * \param[in] ProcessId Process ID of the process where the thread is beginning
 * execution.
 * \param[in] ThreadId Thread ID of the thread beginning execution.
 * \param[in] Create Unused
 */
_Function_class_(PCREATE_THREAD_NOTIFY_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpExecuteThreadNotifyRoutine(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
    )
{
    PKPH_THREAD_CONTEXT thread;

    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Create);

    thread = KphGetThreadContext(ThreadId);
    if (thread)
    {
        thread->ExecuteNotification = TRUE;

        KphpCreateThreadNotifyInformer(thread,
                                       ProcessId,
                                       ThreadId,
                                       KphThreadNotifyExecute);

        KphDereferenceObject(thread);
    }
}

/**
 * \brief Thread create notify routine.
 *
 * \param[in] ProcessId Process ID of the process where the thread is being
 * created.
 * \param[in] ThreadID Thread ID of the thread being created.
 * \param[in] Create If true the thread is being created, if false the thread
 * is being destroyed.
 */
_Function_class_(PCREATE_THREAD_NOTIFY_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpCreateThreadNotifyRoutine(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
    )
{
    PKPH_THREAD_CONTEXT thread;
    PETHREAD threadObject;

    KPH_PAGED_CODE_PASSIVE();

    NT_VERIFY(NT_SUCCESS(PsLookupThreadByThreadId(ThreadId, &threadObject)));
    NT_ASSERT(threadObject);

    if (Create)
    {
        thread = KphpPerformThreadTracking(ProcessId,
                                           ThreadId,
                                           TRUE,
                                           threadObject);
        if (thread)
        {
            KphpCreateThreadNotifyInformer(thread,
                                           ProcessId,
                                           ThreadId,
                                           KphThreadNotifyCreate);
        }
    }
    else
    {
        thread = KphGetThreadContext(ThreadId);
        if (thread)
        {
            KphpCreateThreadNotifyInformer(thread,
                                           ProcessId,
                                           ThreadId,
                                           KphThreadNotifyExit);

            KphDereferenceObject(thread);
        }

        thread = KphpPerformThreadTracking(ProcessId,
                                           ThreadId,
                                           FALSE,
                                           threadObject);
    }

    if (thread)
    {
        KphDereferenceObject(thread);
    }

    ObDereferenceObject(threadObject);
}

/**
 * \brief Starts the thread informer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphThreadInformerStart(
    VOID
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    status = PsSetCreateThreadNotifyRoutineEx(PsCreateThreadNotifySubsystems,
                                              (PVOID)KphpCreateThreadNotifyRoutine);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to register thread notify routine: %!STATUS!",
                      status);
        return status;
    }

    status = PsSetCreateThreadNotifyRoutineEx(PsCreateThreadNotifyNonSystem,
                                              (PVOID)KphpExecuteThreadNotifyRoutine);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to register thread notify routine: %!STATUS!",
                      status);
        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Stops the thread informer.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphThreadInformerStop(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    PsRemoveCreateThreadNotifyRoutine(KphpExecuteThreadNotifyRoutine);
    PsRemoveCreateThreadNotifyRoutine(KphpCreateThreadNotifyRoutine);
}
