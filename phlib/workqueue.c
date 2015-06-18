/*
 * Process Hacker -
 *   thread pool
 *
 * Copyright (C) 2009-2015 wj32
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

#define _PH_WORKQUEUE_PRIVATE
#include <phbase.h>
#include <phintrnl.h>

HANDLE PhpGetSemaphoreWorkQueue(
    _Inout_ PPH_WORK_QUEUE WorkQueue
    );

BOOLEAN PhpCreateWorkQueueThread(
    _Inout_ PPH_WORK_QUEUE WorkQueue
    );

NTSTATUS PhpWorkQueueThreadStart(
    _In_ PVOID Parameter
    );

static PH_FREE_LIST PhWorkQueueItemFreeList;
static PH_WORK_QUEUE PhGlobalWorkQueue;
static PH_INITONCE PhGlobalWorkQueueInitOnce = PH_INITONCE_INIT;
#ifdef DEBUG
PPH_LIST PhDbgWorkQueueList;
PH_QUEUED_LOCK PhDbgWorkQueueListLock = PH_QUEUED_LOCK_INIT;
#endif

VOID PhWorkQueueInitialization(
    VOID
    )
{
    PhInitializeFreeList(&PhWorkQueueItemFreeList, sizeof(PH_WORK_QUEUE_ITEM), 32);

#ifdef DEBUG
    PhDbgWorkQueueList = PhCreateList(4);
#endif
}

FORCEINLINE PPH_WORK_QUEUE_ITEM PhpCreateWorkQueueItem(
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_WORK_QUEUE_ITEM_DELETE_FUNCTION DeleteFunction
    )
{
    PPH_WORK_QUEUE_ITEM workQueueItem;

    workQueueItem = PhAllocateFromFreeList(&PhWorkQueueItemFreeList);
    workQueueItem->Function = Function;
    workQueueItem->Context = Context;
    workQueueItem->DeleteFunction = DeleteFunction;

    return workQueueItem;
}

FORCEINLINE VOID PhpDestroyWorkQueueItem(
    _In_ PPH_WORK_QUEUE_ITEM WorkQueueItem
    )
{
    if (WorkQueueItem->DeleteFunction)
        WorkQueueItem->DeleteFunction(WorkQueueItem->Function, WorkQueueItem->Context);

    PhFreeToFreeList(&PhWorkQueueItemFreeList, WorkQueueItem);
}

FORCEINLINE VOID PhpExecuteWorkQueueItem(
    _Inout_ PPH_WORK_QUEUE_ITEM WorkQueueItem
    )
{
    WorkQueueItem->Function(WorkQueueItem->Context);
}

/**
 * Initializes a work queue.
 *
 * \param WorkQueue A work queue object.
 * \param MinimumThreads The suggested minimum number of threads to keep alive, even
 * when there is no work to be performed.
 * \param MaximumThreads The suggested maximum number of threads to create.
 * \param NoWorkTimeout The number of milliseconds after which threads without work
 * will terminate.
 */
VOID PhInitializeWorkQueue(
    _Out_ PPH_WORK_QUEUE WorkQueue,
    _In_ ULONG MinimumThreads,
    _In_ ULONG MaximumThreads,
    _In_ ULONG NoWorkTimeout
    )
{
    PhInitializeRundownProtection(&WorkQueue->RundownProtect);
    WorkQueue->Terminating = FALSE;

    InitializeListHead(&WorkQueue->QueueListHead);
    PhInitializeQueuedLock(&WorkQueue->QueueLock);
    PhInitializeQueuedLock(&WorkQueue->QueueEmptyCondition);

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
    if ((index = PhFindItemList(PhDbgWorkQueueList, WorkQueue)) != -1)
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

    threadHandle = PhCreateThread(0, PhpWorkQueueThreadStart, WorkQueue);

    if (threadHandle)
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
    PPH_WORK_QUEUE workQueue = (PPH_WORK_QUEUE)Parameter;

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

                PhpExecuteWorkQueueItem(workQueueItem);
                _InterlockedDecrement(&workQueue->BusyCount);

                PhpDestroyWorkQueueItem(workQueueItem);
            }
        }
        else
        {
            BOOLEAN terminate = FALSE;

            // No work arrived before the timeout passed, or we are terminating, or some error occurred.
            // Terminate the thread.

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
    }

    PhReleaseRundownProtection(&workQueue->RundownProtect);

    return STATUS_SUCCESS;
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
    PhQueueItemWorkQueueEx(WorkQueue, Function, Context, NULL);
}

/**
 * Queues a work item to a work queue.
 *
 * \param WorkQueue A work queue object.
 * \param Function A function to execute.
 * \param Context A user-defined value to pass to the function.
 * \param DeleteFunction A callback function that is executed when the work queue item is about to be freed.
 */
VOID PhQueueItemWorkQueueEx(
    _Inout_ PPH_WORK_QUEUE WorkQueue,
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_WORK_QUEUE_ITEM_DELETE_FUNCTION DeleteFunction
    )
{
    PPH_WORK_QUEUE_ITEM workQueueItem;

    workQueueItem = PhpCreateWorkQueueItem(Function, Context, DeleteFunction);

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

/**
 * Queues a work item to the global work queue.
 *
 * \param Function A function to execute.
 * \param Context A user-defined value to pass to the function.
 */
VOID PhQueueItemGlobalWorkQueue(
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context
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

    PhQueueItemWorkQueue(
        &PhGlobalWorkQueue,
        Function,
        Context
        );
}
