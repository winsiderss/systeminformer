/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kph.h>
#include <comms.h>
#include <dyndata.h>

#include <trace.h>

PAGED_FILE();

typedef enum _KPH_THREAD_NOTIFY_TYPE
{
    KphThreadNotifyCreate,
    KphThreadNotifyExecute,
    KphThreadNotifyExit

} KPH_THREAD_NOTIFY_TYPE;

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

    PAGED_PASSIVE();

    if (!Create)
    {
        thread = KphUntrackThreadContext(ThreadId);
        if (!thread)
        {
            return NULL;
        }

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Stopped tracking thread %lu in process %lu",
                      HandleToULong(thread->ClientId.UniqueThread),
                      HandleToULong(thread->ClientId.UniqueProcess));

        thread->ExitNotification = TRUE;

        if (thread->ProcessContext)
        {
            KphAcquireRWLockExclusive(&thread->ProcessContext->ThreadListLock);

            if (thread->InThreadList)
            {
                RemoveEntryList(&thread->ThreadListEntry);
                thread->ProcessContext->NumberOfThreads--;
                thread->InThreadList = FALSE;
                KphDereferenceObject(thread);
            }

            KphReleaseRWLock(&thread->ProcessContext->ThreadListLock);
        }

        return thread;
    }

    NT_ASSERT(Create);

    thread = KphTrackThreadContext(Thread);
    if (!thread)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "Failed to track thread %lu in process %lu",
                      HandleToULong(ThreadId),
                      HandleToULong(ProcessId));

        return NULL;
    }

    thread->CreateNotification = TRUE;
    thread->CreatorClientId.UniqueProcess = PsGetCurrentProcessId();
    thread->CreatorClientId.UniqueThread = PsGetCurrentThreadId();

    if (!thread->ProcessContext)
    {
        thread->ProcessContext = KphGetProcessContext(ProcessId);
    }

    if (thread->ProcessContext)
    {
        KphAcquireRWLockExclusive(&thread->ProcessContext->ThreadListLock);

        if (!thread->InThreadList)
        {
            InsertTailList(&thread->ProcessContext->ThreadListHead,
                           &thread->ThreadListEntry);
            thread->ProcessContext->NumberOfThreads++;
            thread->InThreadList = TRUE;
            KphReferenceObject(thread);
        }

        KphReleaseRWLock(&thread->ProcessContext->ThreadListLock);
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  TRACKING,
                  "Tracking thread %lu in process %lu",
                  HandleToULong(thread->ClientId.UniqueThread),
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

    PAGED_PASSIVE();

    msg = NULL;

    if (Type == KphThreadNotifyCreate)
    {
        if (!KphInformerSettings.ThreadCreate)
        {
            goto Exit;
        }

        msg = KphAllocateMessage();
        if (!msg)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          INFORMER,
                          "Failed to allocate message");
            goto Exit;
        }

        KphMsgInit(msg, KphMsgThreadCreate);
        msg->Kernel.ThreadCreate.CreatingClientId.UniqueProcess = PsGetCurrentProcessId();
        msg->Kernel.ThreadCreate.CreatingClientId.UniqueThread = PsGetCurrentThreadId();
        msg->Kernel.ThreadCreate.TargetClientId.UniqueProcess = ProcessId;
        msg->Kernel.ThreadCreate.TargetClientId.UniqueThread = ThreadId;
    }
    else if (Type == KphThreadNotifyExecute)
    {
        if (!KphInformerSettings.ThreadExecute)
        {
            goto Exit;
        }

        msg = KphAllocateMessage();
        if (!msg)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          INFORMER,
                          "Failed to allocate message");
            goto Exit;
        }

        KphMsgInit(msg, KphMsgThreadExecute);
        msg->Kernel.ThreadExecute.ExecutingClientId.UniqueProcess = ProcessId;
        msg->Kernel.ThreadExecute.ExecutingClientId.UniqueThread = ThreadId;
    }
    else
    {
        NT_ASSERT(Type == KphThreadNotifyExit);

        if (!KphInformerSettings.ThreadExit)
        {
            goto Exit;
        }

        msg = KphAllocateMessage();
        if (!msg)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          INFORMER,
                          "Failed to allocate message");
            goto Exit;
        }

        KphMsgInit(msg, KphMsgThreadExit);
        msg->Kernel.ThreadExit.ExitingClientId.UniqueProcess = ProcessId;
        msg->Kernel.ThreadExit.ExitingClientId.UniqueThread = ThreadId;
        msg->Kernel.ThreadExit.ExitStatus = KphGetThreadExitStatus(Thread->EThread);
    }

    if (KphInformerSettings.EnableStackTraces)
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

    PAGED_PASSIVE();

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

    PAGED_PASSIVE();

    NT_VERIFY(NT_SUCCESS(PsLookupThreadByThreadId(ThreadId, &threadObject)));
    NT_ASSERT(threadObject);

    thread = KphpPerformThreadTracking(ProcessId,
                                       ThreadId,
                                       Create,
                                       threadObject);

    if (!thread)
    {
        goto Exit;
    }

    if (Create)
    {
        KphpCreateThreadNotifyInformer(thread,
                                       ProcessId,
                                       ThreadId,
                                       KphThreadNotifyCreate);
    }
    else
    {
        KphpCreateThreadNotifyInformer(thread,
                                       ProcessId,
                                       ThreadId,
                                       KphThreadNotifyExit);
    }

Exit:

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

    PAGED_PASSIVE();

    if (KphDynPsSetCreateThreadNotifyRoutineEx)
    {
        status = KphDynPsSetCreateThreadNotifyRoutineEx(PsCreateThreadNotifySubsystems,
                                                        (PVOID)KphpCreateThreadNotifyRoutine);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          INFORMER,
                          "Failed to register thread notify routine: %!STATUS!",
                          status);
            return status;
        }

        status = KphDynPsSetCreateThreadNotifyRoutineEx(PsCreateThreadNotifyNonSystem,
                                                        (PVOID)KphpExecuteThreadNotifyRoutine);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          INFORMER,
                          "Failed to register thread notify routine: %!STATUS!",
                          status);
            return status;
        }

        return STATUS_SUCCESS;
    }

    status = PsSetCreateThreadNotifyRoutine(KphpCreateThreadNotifyRoutine);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
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
    PAGED_PASSIVE();

    if (KphDynPsSetCreateThreadNotifyRoutineEx)
    {
        PsRemoveCreateThreadNotifyRoutine(KphpExecuteThreadNotifyRoutine);
        // fall through
    }

    PsRemoveCreateThreadNotifyRoutine(KphpCreateThreadNotifyRoutine);
}
