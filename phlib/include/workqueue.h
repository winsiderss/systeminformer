/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *
 */

#ifndef _PH_WORKQUEUE_H
#define _PH_WORKQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DEBUG)
extern PPH_LIST PhDbgWorkQueueList;
extern PH_QUEUED_LOCK PhDbgWorkQueueListLock;
#endif

typedef struct _PH_WORK_QUEUE
{
    PH_RUNDOWN_PROTECT RundownProtect;
    BOOLEAN Terminating;

    LIST_ENTRY QueueListHead;
    PH_QUEUED_LOCK QueueLock;
    PH_CONDITION QueueEmptyCondition;

    ULONG MaximumThreads;
    ULONG MinimumThreads;
    ULONG NoWorkTimeout;

    PH_QUEUED_LOCK StateLock;
    HANDLE SemaphoreHandle;
    ULONG CurrentThreads;
    ULONG BusyCount;
} PH_WORK_QUEUE, *PPH_WORK_QUEUE;

typedef VOID (NTAPI *PPH_WORK_QUEUE_ITEM_DELETE_FUNCTION)(
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_ PVOID Context
    );

typedef struct _PH_WORK_QUEUE_ENVIRONMENT
{
    LONG BasePriority : 6; // Base priority increment
    ULONG IoPriority : 3; // I/O priority hint
    ULONG PagePriority : 3; // Page/memory priority
    ULONG ForceUpdate : 1; // Always set priorities regardless of cached values
    ULONG SpareBits : 19;
} PH_WORK_QUEUE_ENVIRONMENT, *PPH_WORK_QUEUE_ENVIRONMENT;

PHLIBAPI
VOID
NTAPI
PhInitializeWorkQueue(
    _Out_ PPH_WORK_QUEUE WorkQueue,
    _In_ ULONG MinimumThreads,
    _In_ ULONG MaximumThreads,
    _In_ ULONG NoWorkTimeout
    );

PHLIBAPI
VOID
NTAPI
PhDeleteWorkQueue(
    _Inout_ PPH_WORK_QUEUE WorkQueue
    );

PHLIBAPI
VOID
NTAPI
PhWaitForWorkQueue(
    _Inout_ PPH_WORK_QUEUE WorkQueue
    );

PHLIBAPI
VOID
NTAPI
PhQueueItemWorkQueue(
    _Inout_ PPH_WORK_QUEUE WorkQueue,
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhQueueItemWorkQueueEx(
    _Inout_ PPH_WORK_QUEUE WorkQueue,
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_WORK_QUEUE_ITEM_DELETE_FUNCTION DeleteFunction,
    _In_opt_ PPH_WORK_QUEUE_ENVIRONMENT Environment
    );

PHLIBAPI
VOID
NTAPI
PhInitializeWorkQueueEnvironment(
    _Out_ PPH_WORK_QUEUE_ENVIRONMENT Environment
    );

PHLIBAPI
PPH_WORK_QUEUE
NTAPI
PhGetGlobalWorkQueue(
    VOID
    );

#ifdef __cplusplus
}
#endif

#endif
