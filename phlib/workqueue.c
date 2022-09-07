/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *
 */

#include <phbase.h>
#include <workqueue.h>

#include <phintrnl.h>
#include <phnativeinl.h>
#include <phnative.h>

#include <workqueuep.h>

static PH_INITONCE PhWorkQueueInitOnce = PH_INITONCE_INIT;
static PH_FREE_LIST PhWorkQueueItemFreeList;
static PH_INITONCE PhGlobalWorkQueueInitOnce = PH_INITONCE_INIT;
static PH_WORK_QUEUE PhGlobalWorkQueue;
#ifdef DEBUG
PPH_LIST PhDbgWorkQueueList;
PH_QUEUED_LOCK PhDbgWorkQueueListLock = PH_QUEUED_LOCK_INIT;
#endif

/**
 * Initializes a work queue.
 *
 * \param WorkQueue A work queue object.
 * \param MinimumThreads The suggested minimum number of threads to keep alive, even when there is
 * no work to be performed.
 * \param MaximumThreads The suggested maximum number of threads to create.
 * \param NoWorkTimeout The number of milliseconds after which threads without work will terminate.
 */
VOID PhInitializeWorkQueue(
    _Out_ PPH_WORK_QUEUE WorkQueue,
    _In_ ULONG MinimumThreads,
    _In_ ULONG MaximumThreads,
    _In_ ULONG NoWorkTimeout
    )
{
    if (PhBeginInitOnce(&PhWorkQueueInitOnce))
    {
        PhInitializeFreeList(&PhWorkQueueItemFreeList, sizeof(PH_WORK_QUEUE_ITEM), 32);
#ifdef DEBUG
        PhDbgWorkQueueList = PhCreateList(4);
#endif

        PhEndInitOnce(&PhWorkQueueInitOnce);
    }

    PhInitializeRundownProtection(&WorkQueue->RundownProtect);
    WorkQueue->Terminating = FALSE;

    InitializeListHead(&WorkQueue->QueueListHead);
    PhInitializeQueuedLock(&WorkQueue->QueueLock);
    PhInitializeCondition(&WorkQueue->QueueEmptyCondition);

    WorkQueue->MinimumThreads = MinimumThreads;
    WorkQueue->MaximumThreads = MaximumThreads;
    WorkQueue->NoWorkTimeout = NoWorkTimeout;

    PhInitializeQueuedLock(&WorkQueue->StateLock);

    WorkQueue->SemaphoreHandle = NULL;
    WorkQueue->CurrentThreads = 0;
    WorkQueue->BusyCount = 0;

#ifdef DEBUG
    PhAcquireQueuedLockExclusive(&PhDbgWorkQueueListLock);
    PhAddItemList(PhDbgWorkQueueList, WorkQueue);
    PhReleaseQueuedLockExclusive(&PhDbgWorkQueueListLock);
#endif
}

/**
 * Frees resources used by a work queue.
 *
 * \param WorkQueue A work queue object.
 */
VOID PhDeleteWorkQueue(
    _Inout_ PPH_WORK_QUEUE WorkQueue
    )
{
    PLIST_ENTRY listEntry;
    PPH_WORK_QUEUE_ITEM workQueueItem;
#ifdef DEBUG
    ULONG index;
#endif

#ifdef DEBUG
    PhAcquireQueuedLockExclusive(&PhDbgWorkQueueListLock);
    if ((index = PhFindItemList(PhDbgWorkQueueList, WorkQueue)) != ULONG_MAX)
        PhRemoveItemList(PhDbgWorkQueueList, index);
    PhReleaseQueuedLockExclusive(&PhDbgWorkQueueListLock);
#endif

    // Wait for all worker threads to exit.

    WorkQueue->Terminating = TRUE;
    MemoryBarrier();

    if (WorkQueue->SemaphoreHandle)
        NtReleaseSemaphore(WorkQueue->SemaphoreHandle, WorkQueue->CurrentThreads, NULL);

    PhWaitForRundownProtection(&WorkQueue->RundownProtect);

    // Free all un-executed work items.

    listEntry = WorkQueue->QueueListHead.Flink;

    while (listEntry != &WorkQueue->QueueListHead)
    {
        workQueueItem = CONTAINING_RECORD(listEntry, PH_WORK_QUEUE_ITEM, ListEntry);
        listEntry = listEntry->Flink;
        PhpDestroyWorkQueueItem(workQueueItem);
    }

    if (WorkQueue->SemaphoreHandle)
        NtClose(WorkQueue->SemaphoreHandle);
}

/**
 * Waits for all queued work items to be executed.
 *
 * \param WorkQueue A work queue object.
 */
VOID PhWaitForWorkQueue(
    _Inout_ PPH_WORK_QUEUE WorkQueue
    )
{
    PhAcquireQueuedLockExclusive(&WorkQueue->QueueLock);

    while (!IsListEmpty(&WorkQueue->QueueListHead))
        PhWaitForCondition(&WorkQueue->QueueEmptyCondition, &WorkQueue->QueueLock, NULL);

    PhReleaseQueuedLockExclusive(&WorkQueue->QueueLock);
}

/**
 * Queues a work item to a work queue.
 *
 * \param WorkQueue A work queue object.
 * \param Function A function to execute.
 * \param Context A user-defined value to pass to the function.
 */
VOID PhQueueItemWorkQueue(
    _Inout_ PPH_WORK_QUEUE WorkQueue,
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context
    )
{
    PhQueueItemWorkQueueEx(WorkQueue, Function, Context, NULL, NULL);
}

/**
 * Queues a work item to a work queue.
 *
 * \param WorkQueue A work queue object.
 * \param Function A function to execute.
 * \param Context A user-defined value to pass to the function.
 * \param DeleteFunction A callback function that is executed when the work queue item is about to
 * be freed.
 * \param Environment Execution environment parameters (e.g. priority).
 */
VOID PhQueueItemWorkQueueEx(
    _Inout_ PPH_WORK_QUEUE WorkQueue,
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_WORK_QUEUE_ITEM_DELETE_FUNCTION DeleteFunction,
    _In_opt_ PPH_WORK_QUEUE_ENVIRONMENT Environment
    )
{
    PPH_WORK_QUEUE_ITEM workQueueItem;

    workQueueItem = PhpCreateWorkQueueItem(Function, Context, DeleteFunction, Environment);

    // Enqueue the work item.
    PhAcquireQueuedLockExclusive(&WorkQueue->QueueLock);
    InsertTailList(&WorkQueue->QueueListHead, &workQueueItem->ListEntry);
    _InterlockedIncrement(&WorkQueue->BusyCount);
    PhReleaseQueuedLockExclusive(&WorkQueue->QueueLock);
    // Signal the semaphore once to let a worker thread continue.
    NtReleaseSemaphore(PhpGetSemaphoreWorkQueue(WorkQueue), 1, NULL);

    PHLIB_INC_STATISTIC(WqWorkItemsQueued);

    // Check if all worker threads are currently busy, and if we can create more threads.
    if (WorkQueue->BusyCount >= WorkQueue->CurrentThreads &&
        WorkQueue->CurrentThreads < WorkQueue->MaximumThreads)
    {
        // Lock and re-check.
        PhAcquireQueuedLockExclusive(&WorkQueue->StateLock);

        if (WorkQueue->CurrentThreads < WorkQueue->MaximumThreads)
            PhpCreateWorkQueueThread(WorkQueue);

        PhReleaseQueuedLockExclusive(&WorkQueue->StateLock);
    }
}

VOID PhInitializeWorkQueueEnvironment(
    _Out_ PPH_WORK_QUEUE_ENVIRONMENT Environment
    )
{
    PhpGetDefaultWorkQueueEnvironment(Environment);
}

/** Returns a pointer to the default shared work queue. */
PPH_WORK_QUEUE PhGetGlobalWorkQueue(
    VOID
    )
{
    if (PhBeginInitOnce(&PhGlobalWorkQueueInitOnce))
    {
        PhInitializeWorkQueue(
            &PhGlobalWorkQueue,
            0,
            3,
            1000
            );
        PhEndInitOnce(&PhGlobalWorkQueueInitOnce);
    }

    return &PhGlobalWorkQueue;
}

VOID PhpGetDefaultWorkQueueEnvironment(
    _Out_ PPH_WORK_QUEUE_ENVIRONMENT Environment
    )
{
    memset(Environment, 0, sizeof(PH_WORK_QUEUE_ENVIRONMENT));
    Environment->BasePriority = THREAD_PRIORITY_NORMAL;
    Environment->IoPriority = IoPriorityNormal;
    Environment->PagePriority = MEMORY_PRIORITY_NORMAL;
    Environment->ForceUpdate = FALSE;
}

VOID PhpUpdateWorkQueueEnvironment(
    _Inout_ PPH_WORK_QUEUE_ENVIRONMENT CurrentEnvironment,
    _In_ PPH_WORK_QUEUE_ENVIRONMENT NewEnvironment
    )
{
    if (CurrentEnvironment->BasePriority != NewEnvironment->BasePriority || NewEnvironment->ForceUpdate)
    {
        LONG increment;

        increment = NewEnvironment->BasePriority;

        if (NT_SUCCESS(PhSetThreadBasePriority(NtCurrentThread(), increment)))
        {
            CurrentEnvironment->BasePriority = NewEnvironment->BasePriority;
        }
    }

    if (CurrentEnvironment->IoPriority != NewEnvironment->IoPriority || NewEnvironment->ForceUpdate)
    {
        IO_PRIORITY_HINT ioPriority;

        ioPriority = NewEnvironment->IoPriority;

        if (NT_SUCCESS(PhSetThreadIoPriority(NtCurrentThread(), ioPriority)))
        {
            CurrentEnvironment->IoPriority = NewEnvironment->IoPriority;
        }
    }

    if (CurrentEnvironment->PagePriority != NewEnvironment->PagePriority || NewEnvironment->ForceUpdate)
    {
        ULONG pagePriority;

        pagePriority = NewEnvironment->PagePriority;

        if (NT_SUCCESS(PhSetThreadPagePriority(NtCurrentThread(), pagePriority)))
        {
            CurrentEnvironment->PagePriority = NewEnvironment->PagePriority;
        }
    }
}

PPH_WORK_QUEUE_ITEM PhpCreateWorkQueueItem(
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_WORK_QUEUE_ITEM_DELETE_FUNCTION DeleteFunction,
    _In_opt_ PPH_WORK_QUEUE_ENVIRONMENT Environment
    )
{
    PPH_WORK_QUEUE_ITEM workQueueItem;

    workQueueItem = PhAllocateFromFreeList(&PhWorkQueueItemFreeList);
    workQueueItem->Function = Function;
    workQueueItem->Context = Context;
    workQueueItem->DeleteFunction = DeleteFunction;

    if (Environment)
        workQueueItem->Environment = *Environment;
    else
        PhpGetDefaultWorkQueueEnvironment(&workQueueItem->Environment);

    return workQueueItem;
}

VOID PhpDestroyWorkQueueItem(
    _In_ PPH_WORK_QUEUE_ITEM WorkQueueItem
    )
{
    if (WorkQueueItem->DeleteFunction)
        WorkQueueItem->DeleteFunction(WorkQueueItem->Function, WorkQueueItem->Context);

    PhFreeToFreeList(&PhWorkQueueItemFreeList, WorkQueueItem);
}

VOID PhpExecuteWorkQueueItem(
    _Inout_ PPH_WORK_QUEUE_ITEM WorkQueueItem
    )
{
    WorkQueueItem->Function(WorkQueueItem->Context);
}

HANDLE PhpGetSemaphoreWorkQueue(
    _Inout_ PPH_WORK_QUEUE WorkQueue
    )
{
    HANDLE semaphoreHandle;

    semaphoreHandle = WorkQueue->SemaphoreHandle;

    if (!semaphoreHandle)
    {
        NtCreateSemaphore(&semaphoreHandle, SEMAPHORE_ALL_ACCESS, NULL, 0, MAXLONG);
        assert(semaphoreHandle);

        if (_InterlockedCompareExchangePointer(
            &WorkQueue->SemaphoreHandle,
            semaphoreHandle,
            NULL
            ) != NULL)
        {
            // Someone else created the semaphore before we did.
            NtClose(semaphoreHandle);
            semaphoreHandle = WorkQueue->SemaphoreHandle;
        }
    }

    return semaphoreHandle;
}

BOOLEAN PhpCreateWorkQueueThread(
    _Inout_ PPH_WORK_QUEUE WorkQueue
    )
{
    HANDLE threadHandle;

    // Make sure the structure doesn't get deleted while the thread is running.
    if (!PhAcquireRundownProtection(&WorkQueue->RundownProtect))
        return FALSE;

    if (NT_SUCCESS(PhCreateThreadEx(
        &threadHandle,
        PhpWorkQueueThreadStart,
        WorkQueue
        )))
    {
        PHLIB_INC_STATISTIC(WqWorkQueueThreadsCreated);
        WorkQueue->CurrentThreads++;

        NtClose(threadHandle);
        return TRUE;
    }
    else
    {
        PHLIB_INC_STATISTIC(WqWorkQueueThreadsCreateFailed);
        PhReleaseRundownProtection(&WorkQueue->RundownProtect);
        return FALSE;
    }
}

NTSTATUS PhpWorkQueueThreadStart(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    PPH_WORK_QUEUE workQueue = (PPH_WORK_QUEUE)Parameter;
    PH_WORK_QUEUE_ENVIRONMENT currentEnvironment;

    PhInitializeAutoPool(&autoPool);
    PhpGetDefaultWorkQueueEnvironment(&currentEnvironment);

    while (TRUE)
    {
        NTSTATUS status;
        HANDLE semaphoreHandle;
        LARGE_INTEGER timeout;
        PPH_WORK_QUEUE_ITEM workQueueItem = NULL;

        // Check if we have more threads than the limit.
        if (workQueue->CurrentThreads > workQueue->MaximumThreads)
        {
            BOOLEAN terminate = FALSE;

            // Lock and re-check.
            PhAcquireQueuedLockExclusive(&workQueue->StateLock);

            // Check the minimum as well.
            if (workQueue->CurrentThreads > workQueue->MaximumThreads &&
                workQueue->CurrentThreads > workQueue->MinimumThreads)
            {
                workQueue->CurrentThreads--;
                terminate = TRUE;
            }

            PhReleaseQueuedLockExclusive(&workQueue->StateLock);

            if (terminate)
                break;
        }

        semaphoreHandle = PhpGetSemaphoreWorkQueue(workQueue);

        if (!workQueue->Terminating)
        {
            // Wait for work.
            status = NtWaitForSingleObject(
                semaphoreHandle,
                FALSE,
                PhTimeoutFromMilliseconds(&timeout, workQueue->NoWorkTimeout)
                );
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }

        if (status == STATUS_WAIT_0 && !workQueue->Terminating)
        {
            PLIST_ENTRY listEntry;

            // Dequeue the work item.

            PhAcquireQueuedLockExclusive(&workQueue->QueueLock);

            listEntry = RemoveHeadList(&workQueue->QueueListHead);

            if (IsListEmpty(&workQueue->QueueListHead))
                PhPulseCondition(&workQueue->QueueEmptyCondition);

            PhReleaseQueuedLockExclusive(&workQueue->QueueLock);

            // Make sure we got work.
            if (listEntry != &workQueue->QueueListHead)
            {
                workQueueItem = CONTAINING_RECORD(listEntry, PH_WORK_QUEUE_ITEM, ListEntry);

                PhpUpdateWorkQueueEnvironment(&currentEnvironment, &workQueueItem->Environment);
                PhpExecuteWorkQueueItem(workQueueItem);
                _InterlockedDecrement(&workQueue->BusyCount);

                PhpDestroyWorkQueueItem(workQueueItem);
            }
        }
        else
        {
            BOOLEAN terminate = FALSE;

            // No work arrived before the timeout passed, or we are terminating, or some error
            // occurred. Terminate the thread.

            PhAcquireQueuedLockExclusive(&workQueue->StateLock);

            if (workQueue->Terminating || workQueue->CurrentThreads > workQueue->MinimumThreads)
            {
                workQueue->CurrentThreads--;
                terminate = TRUE;
            }

            PhReleaseQueuedLockExclusive(&workQueue->StateLock);

            if (terminate)
                break;
        }

        PhDrainAutoPool(&autoPool);
    }

    PhReleaseRundownProtection(&workQueue->RundownProtect);
    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}
