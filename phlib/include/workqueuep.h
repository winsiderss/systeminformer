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

#ifndef _PH_WORKQUEUEP_H
#define _PH_WORKQUEUEP_H

typedef struct _PH_WORK_QUEUE_ITEM
{
    LIST_ENTRY ListEntry;
    PUSER_THREAD_START_ROUTINE Function;
    PVOID Context;
    PPH_WORK_QUEUE_ITEM_DELETE_FUNCTION DeleteFunction;
    PH_WORK_QUEUE_ENVIRONMENT Environment;
} PH_WORK_QUEUE_ITEM, *PPH_WORK_QUEUE_ITEM;

VOID PhpGetDefaultWorkQueueEnvironment(
    _Out_ PPH_WORK_QUEUE_ENVIRONMENT Environment
    );

VOID PhpUpdateWorkQueueEnvironment(
    _Inout_ PPH_WORK_QUEUE_ENVIRONMENT CurrentEnvironment,
    _In_ PPH_WORK_QUEUE_ENVIRONMENT NewEnvironment
    );

PPH_WORK_QUEUE_ITEM PhpCreateWorkQueueItem(
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_WORK_QUEUE_ITEM_DELETE_FUNCTION DeleteFunction,
    _In_opt_ PPH_WORK_QUEUE_ENVIRONMENT Environment
    );

VOID PhpDestroyWorkQueueItem(
    _In_ PPH_WORK_QUEUE_ITEM WorkQueueItem
    );

VOID PhpExecuteWorkQueueItem(
    _Inout_ PPH_WORK_QUEUE_ITEM WorkQueueItem
    );

HANDLE PhpGetSemaphoreWorkQueue(
    _Inout_ PPH_WORK_QUEUE WorkQueue
    );

BOOLEAN PhpCreateWorkQueueThread(
    _Inout_ PPH_WORK_QUEUE WorkQueue
    );

NTSTATUS PhpWorkQueueThreadStart(
    _In_ PVOID Parameter
    );

#endif
