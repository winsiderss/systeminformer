/*
 * Process Hacker Extended Tools - 
 *   ETW statistics collection
 * 
 * Copyright (C) 2010 wj32
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

#include "exttools.h"
#include "etwmon.h"

VOID NTAPI ProcessAddedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ProcessRemovedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PPH_HASHTABLE EtEtwBlockHashtable;
PH_QUEUED_LOCK EtEtwBlockHashtableLock = PH_QUEUED_LOCK_INIT;

static PH_CALLBACK_REGISTRATION EtpProcessAddedCallbackRegistration;
static PH_CALLBACK_REGISTRATION EtpProcessRemovedCallbackRegistration;
static PH_CALLBACK_REGISTRATION EtpProcessesUpdatedCallbackRegistration;

ULONG EtpDiskReadRaw;
ULONG EtpDiskWriteRaw;
ULONG EtpNetworkReceiveRaw;
ULONG EtpNetworkSendRaw;

ULONG EtDiskReadCount;
ULONG EtDiskWriteCount;
ULONG EtNetworkReceiveCount;
ULONG EtNetworkSendCount;

PH_UINT32_DELTA EtDiskReadDelta;
PH_UINT32_DELTA EtDiskWriteDelta;
PH_UINT32_DELTA EtNetworkReceiveDelta;
PH_UINT32_DELTA EtNetworkSendDelta;

PH_CIRCULAR_BUFFER_ULONG EtDiskReadHistory;
PH_CIRCULAR_BUFFER_ULONG EtDiskWriteHistory;
PH_CIRCULAR_BUFFER_ULONG EtNetworkReceiveHistory;
PH_CIRCULAR_BUFFER_ULONG EtNetworkSendHistory;
PH_CIRCULAR_BUFFER_ULONG EtMaxDiskHistory; // ID of max. disk usage process
PH_CIRCULAR_BUFFER_ULONG EtMaxNetworkHistory; // ID of max. network usage process

VOID EtEtwStatisticsInitialization()
{
    EtEtwMonitorInitialization();

    if (EtEtwEnabled)
    {
        ULONG sampleCount;

        EtEtwBlockHashtable = PhCreateSimpleHashtable(64);

        sampleCount = PhGetIntegerSetting(L"SampleCount");
        PhInitializeCircularBuffer_ULONG(&EtDiskReadHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtDiskWriteHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtNetworkReceiveHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtNetworkSendHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtMaxDiskHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtMaxNetworkHistory, sampleCount);

        PhRegisterCallback(
            &PhProcessAddedEvent,
            ProcessAddedCallback,
            NULL,
            &EtpProcessAddedCallbackRegistration
            );
        PhRegisterCallback(
            &PhProcessRemovedEvent,
            ProcessRemovedCallback,
            NULL,
            &EtpProcessRemovedCallbackRegistration
            );
        PhRegisterCallback(
            &PhProcessesUpdatedEvent,
            ProcessesUpdatedCallback,
            NULL,
            &EtpProcessesUpdatedCallbackRegistration
            );
    }
}

VOID EtEtwStatisticsUninitialization()
{
    EtEtwMonitorUninitialization();
}

PET_PROCESS_ETW_BLOCK EtCreateProcessEtwBlock(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PET_PROCESS_ETW_BLOCK block;

    block = PhAllocate(sizeof(ET_PROCESS_ETW_BLOCK));
    memset(block, 0, sizeof(ET_PROCESS_ETW_BLOCK));
    block->RefCount = 1;
    block->ProcessItem = ProcessItem;
    PhInitializeQueuedLock(&block->TextCacheLock);

    return block;
}

VOID EtReferenceProcessEtwBlock(
    __inout PET_PROCESS_ETW_BLOCK Block
    )
{
    _InterlockedIncrement(&Block->RefCount);
}

VOID EtDereferenceProcessEtwBlock(
    __inout PET_PROCESS_ETW_BLOCK Block
    )
{
    if (_InterlockedDecrement(&Block->RefCount) == 0)
    {
        ULONG i;

        for (i = 1; i <= ETETWTNC_MAXIMUM; i++)
        {
            PhSwapReference(&Block->TextCache[i], NULL);
        }

        PhFree(Block);
    }
}

VOID EtpAddProcessEtwBlock(
    __in PET_PROCESS_ETW_BLOCK Block
    )
{
    PhAcquireQueuedLockExclusive(&EtEtwBlockHashtableLock);
    PhAddItemSimpleHashtable(EtEtwBlockHashtable, Block->ProcessItem, Block);
    PhReleaseQueuedLockExclusive(&EtEtwBlockHashtableLock);

    EtReferenceProcessEtwBlock(Block);
}

VOID EtpRemoveProcessEtwBlock(
    __in PET_PROCESS_ETW_BLOCK Block
    )
{
    PhAcquireQueuedLockExclusive(&EtEtwBlockHashtableLock);
    PhRemoveItemSimpleHashtable(EtEtwBlockHashtable, Block->ProcessItem);
    PhReleaseQueuedLockExclusive(&EtEtwBlockHashtableLock);

    EtDereferenceProcessEtwBlock(Block);
}

PET_PROCESS_ETW_BLOCK EtFindProcessEtwBlock(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PET_PROCESS_ETW_BLOCK block;
    PPVOID entry;

    PhAcquireQueuedLockShared(&EtEtwBlockHashtableLock);

    entry = PhFindItemSimpleHashtable(EtEtwBlockHashtable, ProcessItem);

    if (entry)
    {
        block = *entry;
        EtReferenceProcessEtwBlock(block);
    }
    else
    {
        block = NULL;
    }

    PhReleaseQueuedLockShared(&EtEtwBlockHashtableLock);

    return block;
}

VOID EtProcessDiskEvent(
    __in PET_ETW_DISK_EVENT Event
    )
{
    PPH_PROCESS_ITEM processItem;
    PET_PROCESS_ETW_BLOCK block;

    if (Event->Type == EtEtwDiskReadType)
    {
        EtpDiskReadRaw += Event->TransferSize;
        EtDiskReadCount++;
    }
    else
    {
        EtpDiskWriteRaw += Event->TransferSize;
        EtDiskWriteCount++;
    }

    if (processItem = PhReferenceProcessItem(Event->ClientId.UniqueProcess))
    {
        if (block = EtFindProcessEtwBlock(processItem))
        {
            if (Event->Type == EtEtwDiskReadType)
            {
                block->DiskReadRaw += Event->TransferSize;
                block->DiskReadCount++;
            }
            else
            {
                block->DiskWriteRaw += Event->TransferSize;
                block->DiskWriteCount++;
            }

            EtDereferenceProcessEtwBlock(block);
        }

        PhDereferenceObject(processItem);
    }
}

VOID EtProcessNetworkEvent(
    __in PET_ETW_NETWORK_EVENT Event
    )
{
    PPH_PROCESS_ITEM processItem;
    PET_PROCESS_ETW_BLOCK block;

    if (Event->Type == EtEtwNetworkReceiveType)
    {
        EtpNetworkReceiveRaw += Event->TransferSize;
        EtNetworkReceiveCount++;
    }
    else
    {
        EtpNetworkSendRaw += Event->TransferSize;
        EtNetworkSendCount++;
    }

    if (processItem = PhReferenceProcessItem(Event->ClientId.UniqueProcess))
    {
        if (block = EtFindProcessEtwBlock(processItem))
        {
            if (Event->Type == EtEtwNetworkReceiveType)
            {
                block->NetworkReceiveRaw += Event->TransferSize;
                block->NetworkReceiveCount++;
            }
            else
            {
                block->NetworkSendRaw += Event->TransferSize;
                block->NetworkSendCount++;
            }

            EtDereferenceProcessEtwBlock(block);
        }

        PhDereferenceObject(processItem);
    }
}

static VOID NTAPI ProcessAddedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PET_PROCESS_ETW_BLOCK block;
    PPH_PROCESS_ITEM processItem = Parameter;

    block = EtCreateProcessEtwBlock(processItem);
    EtpAddProcessEtwBlock(block);
    EtDereferenceProcessEtwBlock(block);
}

static VOID NTAPI ProcessRemovedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PET_PROCESS_ETW_BLOCK block;
    PPH_PROCESS_ITEM processItem = Parameter;

    if (block = EtFindProcessEtwBlock(processItem))
    {
        EtpRemoveProcessEtwBlock(block);
        EtDereferenceProcessEtwBlock(block);
    }
}

static VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    static ULONG runCount = 0; // MUST keep in sync with runCount in process provider

    PPH_KEY_VALUE_PAIR pair;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    ULONG maxDiskValue = 0;
    PET_PROCESS_ETW_BLOCK maxDiskBlock = NULL;
    ULONG maxNetworkValue = 0;
    PET_PROCESS_ETW_BLOCK maxNetworkBlock = NULL;

    // ETW is extremely lazy when it comes to flushing buffers, so we must do it 
    // manually.
    EtFlushEtwSession();

    // Update global statistics.

    PhUpdateDelta(&EtDiskReadDelta, EtpDiskReadRaw);
    PhUpdateDelta(&EtDiskWriteDelta, EtpDiskWriteRaw);
    PhUpdateDelta(&EtNetworkReceiveDelta, EtpNetworkReceiveRaw);
    PhUpdateDelta(&EtNetworkSendDelta, EtpNetworkSendRaw);

    // Update per-process statistics.
    // Note: no lock is needed because we only ever modify the hashtable on this same thread.

    PhBeginEnumHashtable(EtEtwBlockHashtable, &enumContext);

    while (pair = PhNextEnumHashtable(&enumContext))
    {
        PET_PROCESS_ETW_BLOCK block;

        block = pair->Value;

        PhUpdateDelta(&block->DiskReadDelta, block->DiskReadCount);
        PhUpdateDelta(&block->DiskReadRawDelta, block->DiskReadRaw);
        PhUpdateDelta(&block->DiskWriteDelta, block->DiskWriteCount);
        PhUpdateDelta(&block->DiskWriteRawDelta, block->DiskWriteRaw);
        PhUpdateDelta(&block->NetworkReceiveDelta, block->NetworkReceiveCount);
        PhUpdateDelta(&block->NetworkReceiveRawDelta, block->NetworkReceiveRaw);
        PhUpdateDelta(&block->NetworkSendDelta, block->NetworkSendCount);
        PhUpdateDelta(&block->NetworkSendRawDelta, block->NetworkSendRaw);

        if (maxDiskValue < block->DiskReadRawDelta.Delta + block->DiskWriteRawDelta.Delta)
        {
            maxDiskValue = block->DiskReadRawDelta.Delta + block->DiskWriteRawDelta.Delta;
            maxDiskBlock = block;
        }

        if (maxNetworkValue < block->NetworkReceiveRawDelta.Delta + block->NetworkSendRawDelta.Delta)
        {
            maxNetworkValue = block->NetworkReceiveRawDelta.Delta + block->NetworkSendRawDelta.Delta;
            maxNetworkBlock = block;
        }

        // Invalidate all text.

        PhAcquireQueuedLockExclusive(&block->TextCacheLock);
        memset(block->TextCacheValid, 0, sizeof(block->TextCacheValid));
        PhReleaseQueuedLockExclusive(&block->TextCacheLock);
    }

    // Update history buffers.

    if (runCount != 0)
    {
        PhAddItemCircularBuffer2_ULONG(&EtDiskReadHistory, EtDiskReadDelta.Delta);
        PhAddItemCircularBuffer2_ULONG(&EtDiskWriteHistory, EtDiskWriteDelta.Delta);
        PhAddItemCircularBuffer2_ULONG(&EtNetworkReceiveHistory, EtNetworkReceiveDelta.Delta);
        PhAddItemCircularBuffer2_ULONG(&EtNetworkSendHistory, EtNetworkSendDelta.Delta);

        if (maxDiskBlock)
        {
            PhAddItemCircularBuffer2_ULONG(&EtMaxDiskHistory, (ULONG)maxDiskBlock->ProcessItem->ProcessId);
            PhReferenceProcessRecordForStatistics(maxDiskBlock->ProcessItem->Record);
        }
        else
        {
            PhAddItemCircularBuffer2_ULONG(&EtMaxDiskHistory, (ULONG)NULL);
        }

        if (maxNetworkBlock)
        {
            PhAddItemCircularBuffer2_ULONG(&EtMaxNetworkHistory, (ULONG)maxNetworkBlock->ProcessItem->ProcessId);
            PhReferenceProcessRecordForStatistics(maxNetworkBlock->ProcessItem->Record);
        }
        else
        {
            PhAddItemCircularBuffer2_ULONG(&EtMaxNetworkHistory, (ULONG)NULL);
        }
    }

    runCount++;
}
