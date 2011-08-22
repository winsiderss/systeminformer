/*
 * Process Hacker Extended Tools - 
 *   ETW statistics collection
 * 
 * Copyright (C) 2010-2011 wj32
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

VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI NetworkItemsUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID EtpUpdateDiskInformation(
    VOID
    );

static PH_CALLBACK_REGISTRATION EtpProcessesUpdatedCallbackRegistration;
static PH_CALLBACK_REGISTRATION EtpNetworkItemsUpdatedCallbackRegistration;

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

VOID EtEtwStatisticsInitialization(
    VOID
    )
{
    EtEtwMonitorInitialization();

    if (EtEtwEnabled)
    {
        ULONG sampleCount;

        sampleCount = PhGetIntegerSetting(L"SampleCount");
        PhInitializeCircularBuffer_ULONG(&EtDiskReadHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtDiskWriteHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtNetworkReceiveHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtNetworkSendHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtMaxDiskHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtMaxNetworkHistory, sampleCount);

        PhRegisterCallback(
            &PhProcessesUpdatedEvent,
            ProcessesUpdatedCallback,
            NULL,
            &EtpProcessesUpdatedCallbackRegistration
            );
        PhRegisterCallback(
            &PhNetworkItemsUpdatedEvent,
            NetworkItemsUpdatedCallback,
            NULL,
            &EtpNetworkItemsUpdatedCallbackRegistration
            );
    }
}

VOID EtEtwStatisticsUninitialization(
    VOID
    )
{
    EtEtwMonitorUninitialization();
}

VOID EtProcessDiskEvent(
    __in PET_ETW_DISK_EVENT Event
    )
{
    PPH_PROCESS_ITEM processItem;
    PET_PROCESS_BLOCK block;

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
        block = EtGetProcessBlock(processItem);

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

        PhDereferenceObject(processItem);
    }
}

VOID EtProcessNetworkEvent(
    __in PET_ETW_NETWORK_EVENT Event
    )
{
    PPH_PROCESS_ITEM processItem;
    PET_PROCESS_BLOCK block;
    PPH_NETWORK_ITEM networkItem;
    PET_NETWORK_BLOCK networkBlock;

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

    // Note: there is always the possibility of us receiving the event too early, 
    // before the process item or network item is created. So events may be lost.

    if (processItem = PhReferenceProcessItem(Event->ClientId.UniqueProcess))
    {
        block = EtGetProcessBlock(processItem);

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

        PhDereferenceObject(processItem);
    }

    if (networkItem = PhReferenceNetworkItem(
        Event->ProtocolType,
        &Event->LocalEndpoint,
        &Event->RemoteEndpoint,
        Event->ClientId.UniqueProcess
        ))
    {
        networkBlock = EtGetNetworkBlock(networkItem);

        if (Event->Type == EtEtwNetworkReceiveType)
        {
            networkBlock->ReceiveRaw += Event->TransferSize;
            networkBlock->ReceiveCount++;
        }
        else
        {
            networkBlock->SendRaw += Event->TransferSize;
            networkBlock->SendCount++;
        }

        PhDereferenceObject(networkItem);
    }
}

static VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    static ULONG runCount = 0; // MUST keep in sync with runCount in process provider

    PLIST_ENTRY listEntry;
    ULONG maxDiskValue = 0;
    PET_PROCESS_BLOCK maxDiskBlock = NULL;
    ULONG maxNetworkValue = 0;
    PET_PROCESS_BLOCK maxNetworkBlock = NULL;

    // ETW is extremely lazy when it comes to flushing buffers, so we must do it 
    // manually.
    EtFlushEtwSession();

    // Update global statistics.

    PhUpdateDelta(&EtDiskReadDelta, EtpDiskReadRaw);
    PhUpdateDelta(&EtDiskWriteDelta, EtpDiskWriteRaw);
    PhUpdateDelta(&EtNetworkReceiveDelta, EtpNetworkReceiveRaw);
    PhUpdateDelta(&EtNetworkSendDelta, EtpNetworkSendRaw);

    // Update per-process statistics.
    // Note: no lock is needed because we only ever modify the list on this same thread.

    listEntry = EtProcessBlockListHead.Flink;

    while (listEntry != &EtProcessBlockListHead)
    {
        PET_PROCESS_BLOCK block;

        block = CONTAINING_RECORD(listEntry, ET_PROCESS_BLOCK, ListEntry);

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

        listEntry = listEntry->Flink;
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

static VOID NTAPI EtpInvalidateNetworkNode(
    __in PVOID Parameter
    )
{
    PPH_NETWORK_ITEM networkItem = Parameter;
    PPH_NETWORK_NODE networkNode;

    if (networkNode = PhFindNetworkNode(networkItem))
        TreeNew_InvalidateNode(NetworkTreeNewHandle, &networkNode->Node);

    PhDereferenceObject(networkItem);
}

static VOID NTAPI NetworkItemsUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PLIST_ENTRY listEntry;

    // ETW is flushed in the processes-updated callback above. This may cause us the network 
    // blocks to all fall one update interval behind, however.

    // Update per-connection statistics.
    // Note: no lock is needed because we only ever modify the list on this same thread.

    listEntry = EtNetworkBlockListHead.Flink;

    while (listEntry != &EtNetworkBlockListHead)
    {
        PET_NETWORK_BLOCK block;
        PH_UINT32_DELTA oldDeltas[4];

        block = CONTAINING_RECORD(listEntry, ET_NETWORK_BLOCK, ListEntry);

        memcpy(oldDeltas, block->Deltas, sizeof(block->Deltas));

        PhUpdateDelta(&block->ReceiveDelta, block->ReceiveCount);
        PhUpdateDelta(&block->ReceiveRawDelta, block->ReceiveRaw);
        PhUpdateDelta(&block->SendDelta, block->SendCount);
        PhUpdateDelta(&block->SendRawDelta, block->SendRaw);

        if (memcmp(oldDeltas, block->Deltas, sizeof(block->Deltas)))
        {
            // Values have changed. Invalidate the network node.
            PhReferenceObject(block->NetworkItem);
            ProcessHacker_Invoke(PhMainWndHandle, EtpInvalidateNetworkNode, block->NetworkItem);
        }

        listEntry = listEntry->Flink;
    }
}
