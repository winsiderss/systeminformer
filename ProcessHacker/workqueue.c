/*
 * Process Hacker - 
 *   thread pool
 * 
 * Copyright (C) 2009-2010 wj32
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

#include <phbase.h>

typedef struct _PH_WORK_QUEUE_ITEM
{
    PTHREAD_START_ROUTINE Function;
    PVOID Context;
} PH_WORK_QUEUE_ITEM, *PPH_WORK_QUEUE_ITEM;

NTSTATUS PhpWorkQueueThreadStart(
    __in PVOID Parameter
    );

PH_WORK_QUEUE PhGlobalWorkQueue;

VOID PhWorkQueueInitialization()
{
    PhInitializeWorkQueue(
        &PhGlobalWorkQueue,
        0,
        3,
        1000
        );
}

VOID PhInitializeWorkQueue(
    __out PPH_WORK_QUEUE WorkQueue,
    __in ULONG MinimumThreads,
    __in ULONG MaximumThreads,
    __in ULONG NoWorkTimeout
    )
{
    WorkQueue->Queue = PhCreateQueue(1);
    PhInitializeMutex(&WorkQueue->QueueLock);
    WorkQueue->MinimumThreads = MinimumThreads;
    WorkQueue->MaximumThreads = MaximumThreads;
    WorkQueue->NoWorkTimeout = NoWorkTimeout;
    PhInitializeMutex(&WorkQueue->StateLock);
    WorkQueue->SemaphoreHandle = CreateSemaphore(NULL, 0, MAXLONG, NULL);
    WorkQueue->CurrentThreads = 0;
    WorkQueue->BusyThreads = 0;
}

VOID PhDeleteWorkQueue(
    __inout PPH_WORK_QUEUE WorkQueue
    )
{
    PhDereferenceObject(WorkQueue->Queue);
    PhDeleteMutex(&WorkQueue->QueueLock);
    PhDeleteMutex(&WorkQueue->StateLock);
    CloseHandle(WorkQueue->SemaphoreHandle);
}

FORCEINLINE VOID PhpInitializeWorkQueueItem(
    __out PPH_WORK_QUEUE_ITEM WorkQueueItem,
    __in PTHREAD_START_ROUTINE Function,
    __in PVOID Context
    )
{
    WorkQueueItem->Function = Function;
    WorkQueueItem->Context = Context;
}

FORCEINLINE VOID PhpExecuteWorkQueueItem(
    __inout PPH_WORK_QUEUE_ITEM WorkQueueItem
    )
{
    WorkQueueItem->Function(WorkQueueItem->Context);
}

BOOLEAN PhpCreateWorkQueueThread(
    __inout PPH_WORK_QUEUE WorkQueue
    )
{
    HANDLE threadHandle;
    
    threadHandle = PhCreateThread(0, PhpWorkQueueThreadStart, WorkQueue);

    if (threadHandle)
    {
        WorkQueue->CurrentThreads++;
        CloseHandle(threadHandle);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

NTSTATUS PhpWorkQueueThreadStart(
    __in PVOID Parameter
    )
{
    PPH_WORK_QUEUE workQueue = (PPH_WORK_QUEUE)Parameter;

    while (TRUE)
    {
        PPH_WORK_QUEUE_ITEM workQueueItem = NULL;
        ULONG result;

        // Check if we have more threads than the limit.
        if (workQueue->CurrentThreads > workQueue->MaximumThreads)
        {
            BOOLEAN terminate = FALSE;

            // Lock and re-check.
            PhAcquireMutex(&workQueue->StateLock);

            // Check the minimum as well.
            if (
                workQueue->CurrentThreads > workQueue->MaximumThreads &&
                workQueue->CurrentThreads > workQueue->MinimumThreads
                )
            {
                workQueue->CurrentThreads--;
                terminate = TRUE;
            }

            PhReleaseMutex(&workQueue->StateLock);

            if (terminate)
                break;
        }

        // Wait for work.
        result = WaitForSingleObject(workQueue->SemaphoreHandle, workQueue->NoWorkTimeout);

        if (result == WAIT_OBJECT_0)
        {
            // Dequeue the work item.
            PhAcquireMutex(&workQueue->QueueLock);
            PhDequeueQueueItem(workQueue->Queue, &workQueueItem);
            PhReleaseMutex(&workQueue->QueueLock);

            // Make sure we got work.
            if (workQueueItem)
            {
                _InterlockedIncrement(&workQueue->BusyThreads);
                PhpExecuteWorkQueueItem(workQueueItem);
                _InterlockedDecrement(&workQueue->BusyThreads);

                PhFree(workQueueItem);
            }
        }
        else
        {
            BOOLEAN terminate = FALSE;

            // No work arrived before the timeout passed (or some error occurred).
            // Terminate the thread.

            PhAcquireMutex(&workQueue->StateLock);

            // Check the minimum.
            if (workQueue->CurrentThreads > workQueue->MinimumThreads)
            {
                workQueue->CurrentThreads--;
                terminate = TRUE;
            }

            PhReleaseMutex(&workQueue->StateLock);

            if (terminate)
                break;
        }
    }

    return STATUS_SUCCESS;
}

VOID PhQueueWorkQueueItem(
    __inout PPH_WORK_QUEUE WorkQueue,
    __in PTHREAD_START_ROUTINE Function,
    __in PVOID Context
    )
{
    PPH_WORK_QUEUE_ITEM workQueueItem;

    workQueueItem = PhAllocate(sizeof(PH_WORK_QUEUE_ITEM));
    PhpInitializeWorkQueueItem(workQueueItem, Function, Context);

    // Enqueue the work item.
    PhAcquireMutex(&WorkQueue->QueueLock);
    PhEnqueueQueueItem(WorkQueue->Queue, workQueueItem);
    PhReleaseMutex(&WorkQueue->QueueLock);
    // Signal the semaphore once to let a worker thread continue.
    ReleaseSemaphore(WorkQueue->SemaphoreHandle, 1, NULL);

    // Check if all worker threads are currently busy, 
    // and if we can create more threads.
    if (
        WorkQueue->BusyThreads == WorkQueue->CurrentThreads &&
        WorkQueue->CurrentThreads < WorkQueue->MaximumThreads
        )
    {
        // Lock and re-check.
        PhAcquireMutex(&WorkQueue->StateLock);

        if (WorkQueue->CurrentThreads < WorkQueue->MaximumThreads)
        {
            PhpCreateWorkQueueThread(WorkQueue);
        }

        PhReleaseMutex(&WorkQueue->StateLock);
    }
}

VOID PhQueueGlobalWorkQueueItem(
    __in PTHREAD_START_ROUTINE Function,
    __in PVOID Context
    )
{
    PhQueueWorkQueueItem(
        &PhGlobalWorkQueue,
        Function,
        Context
        );
}
