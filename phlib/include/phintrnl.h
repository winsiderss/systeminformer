/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2023
 *
 */

#ifndef _PH_PHINTRNL_H
#define _PH_PHINTRNL_H

typedef struct _PHLIB_STATISTICS_BLOCK
{
    // basesup
    ULONG BaseThreadsCreated;
    ULONG BaseThreadsCreateFailed;
    ULONG BaseStringBuildersCreated;
    ULONG BaseStringBuildersResized;

    // ref
    ULONG RefObjectsCreated;
    ULONG RefObjectsDestroyed;
    ULONG RefObjectsAllocated;
    ULONG RefObjectsFreed;
    ULONG RefObjectsAllocatedFromSmallFreeList;
    ULONG RefObjectsFreedToSmallFreeList;
    ULONG RefObjectsAllocatedFromTypeFreeList;
    ULONG RefObjectsFreedToTypeFreeList;
    ULONG RefObjectsDeleteDeferred;
    ULONG RefAutoPoolsCreated;
    ULONG RefAutoPoolsDestroyed;
    ULONG RefAutoPoolsDynamicAllocated;
    ULONG RefAutoPoolsDynamicResized;

    // queuedlock
    ULONG QlBlockSpins;
    ULONG QlBlockWaits;
    ULONG QlAcquireExclusiveBlocks;
    ULONG QlAcquireSharedBlocks;

    // workqueue
    ULONG WqWorkQueueThreadsCreated;
    ULONG WqWorkQueueThreadsCreateFailed;
    ULONG WqWorkItemsQueued;
} PHLIB_STATISTICS_BLOCK;

#ifdef DEBUG
extern PHLIB_STATISTICS_BLOCK PhLibStatisticsBlock;
#endif

#ifdef DEBUG
#define PHLIB_INC_STATISTIC(Name) (_InterlockedIncrement(&PhLibStatisticsBlock.Name))
#else
#define PHLIB_INC_STATISTIC(Name)
#endif

#endif
