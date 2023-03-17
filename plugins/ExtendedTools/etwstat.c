/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2019-2022
 *
 */

#include "exttools.h"
#include "etwmon.h"

VOID NTAPI EtEtwProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI EtEtwNetworkItemsUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID EtpUpdateProcessInformation(
    VOID
    );

BOOLEAN EtDiskCountersEnabled = FALSE;
static PH_CALLBACK_REGISTRATION EtpProcessesUpdatedCallbackRegistration;
static PH_CALLBACK_REGISTRATION EtpNetworkItemsUpdatedCallbackRegistration;

static PVOID EtpProcessInformation = NULL;
static PH_QUEUED_LOCK EtpProcessInformationLock = PH_QUEUED_LOCK_INIT;

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

PH_UINT32_DELTA EtDiskReadCountDelta;
PH_UINT32_DELTA EtDiskWriteCountDelta;
PH_UINT32_DELTA EtNetworkReceiveCountDelta;
PH_UINT32_DELTA EtNetworkSendCountDelta;

PH_CIRCULAR_BUFFER_ULONG EtDiskReadHistory;
PH_CIRCULAR_BUFFER_ULONG EtDiskWriteHistory;
PH_CIRCULAR_BUFFER_ULONG EtNetworkReceiveHistory;
PH_CIRCULAR_BUFFER_ULONG EtNetworkSendHistory;
PH_CIRCULAR_BUFFER_ULONG EtMaxDiskHistory; // ID of max. disk usage process
PH_CIRCULAR_BUFFER_ULONG EtMaxNetworkHistory; // ID of max. network usage process
#ifdef PH_RECORD_MAX_USAGE
PH_CIRCULAR_BUFFER_ULONG64 PhMaxDiskUsageHistory;
PH_CIRCULAR_BUFFER_ULONG64 PhMaxNetworkUsageHistory;
#endif

VOID EtEtwStatisticsInitialization(
    VOID
    )
{
    ULONG sampleCount;

    sampleCount = PhGetIntegerSetting(L"SampleCount");
    PhInitializeCircularBuffer_ULONG(&EtDiskReadHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG(&EtDiskWriteHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG(&EtNetworkReceiveHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG(&EtNetworkSendHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG(&EtMaxDiskHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG(&EtMaxNetworkHistory, sampleCount);
#ifdef PH_RECORD_MAX_USAGE
    PhInitializeCircularBuffer_ULONG64(&PhMaxDiskUsageHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG64(&PhMaxNetworkUsageHistory, sampleCount);
#endif

    if (EtWindowsVersion >= WINDOWS_10_RS3 && !PhIsExecutingInWow64)
    {
        EtDiskCountersEnabled = !!PhGetIntegerSetting(SETTING_NAME_ENABLE_DISKPERFCOUNTERS);
    }

    EtEtwMonitorInitialization();

    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
        EtEtwProcessesUpdatedCallback,
        NULL,
        &EtpProcessesUpdatedCallbackRegistration
        );

    if (EtEtwEnabled)
    {
        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackNetworkProviderUpdatedEvent),
            EtEtwNetworkItemsUpdatedCallback,
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
    _In_ PET_ETW_DISK_EVENT Event
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
    _In_ PET_ETW_NETWORK_EVENT Event
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

    networkItem = PhReferenceNetworkItem(
        Event->ProtocolType,
        &Event->LocalEndpoint,
        &Event->RemoteEndpoint,
        Event->ClientId.UniqueProcess
        );

    if (!networkItem && Event->ProtocolType & PH_UDP_PROTOCOL_TYPE)
    {
        // Note: ETW generates UDP events with the LocalEndpoint set to the LAN endpoint address
        // of the local adapter the packet was sent or received but GetExtendedUdpTable
        // returns some UDP connections with endpoints set to in4addr_any/in6addr_any (zero). (dmex)

        if (Event->ProtocolType & PH_IPV4_NETWORK_TYPE)
            memset(&Event->LocalEndpoint.Address.InAddr, 0, sizeof(IN_ADDR)); // same as in4addr_any
        else
            memset(&Event->LocalEndpoint.Address.In6Addr, 0, sizeof(IN6_ADDR)); // same as in6addr_any

        networkItem = PhReferenceNetworkItem(
            Event->ProtocolType,
            &Event->LocalEndpoint,
            &Event->RemoteEndpoint,
            Event->ClientId.UniqueProcess
            );
    }

    if (networkItem)
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

VOID NTAPI EtEtwProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    static ULONG runCount = 0; // MUST keep in sync with runCount in process provider
    ULONG64 maxDiskValue = 0;
    ULONG64 maxNetworkValue = 0;
    PET_PROCESS_BLOCK maxDiskBlock = NULL;
    PET_PROCESS_BLOCK maxNetworkBlock = NULL;
    PLIST_ENTRY listEntry;

    // Since Windows 8, we no longer get the correct process/thread IDs in the
    // event headers for disk events. We need to update our process information since
    // etwmon uses our EtThreadIdToProcessId function. (wj32)
    if (EtWindowsVersion >= WINDOWS_8)
        EtpUpdateProcessInformation();

    // ETW is extremely lazy when it comes to flushing buffers, so we must do it manually. (wj32)
    //EtFlushEtwSession();

    // Update global statistics.
    PhUpdateDelta(&EtDiskReadDelta, EtpDiskReadRaw);
    PhUpdateDelta(&EtDiskWriteDelta, EtpDiskWriteRaw);
    PhUpdateDelta(&EtNetworkReceiveDelta, EtpNetworkReceiveRaw);
    PhUpdateDelta(&EtNetworkSendDelta, EtpNetworkSendRaw);

    PhUpdateDelta(&EtDiskReadCountDelta, EtDiskReadCount);
    PhUpdateDelta(&EtDiskWriteCountDelta, EtDiskWriteCount);
    PhUpdateDelta(&EtNetworkReceiveCountDelta, EtNetworkReceiveCount);
    PhUpdateDelta(&EtNetworkSendCountDelta, EtNetworkSendCount);

    // Update per-process statistics.
    // Note: no lock is needed because we only ever modify the list on this same thread. (wj32)

    listEntry = EtProcessBlockListHead.Flink;

    while (listEntry != &EtProcessBlockListHead)
    {
        PET_PROCESS_BLOCK block;

        block = CONTAINING_RECORD(listEntry, ET_PROCESS_BLOCK, ListEntry);

        if (EtDiskCountersEnabled)
        {
            ULONG64 diskReads = block->ProcessItem->DiskCounters.ReadOperationCount;
            ULONG64 diskWrites = block->ProcessItem->DiskCounters.WriteOperationCount;
            ULONG64 diskReadRaw = block->ProcessItem->DiskCounters.BytesRead;
            ULONG64 diskWriteRaw = block->ProcessItem->DiskCounters.BytesWritten;

            if (block->DiskReadCount < diskReads)
                block->DiskReadCount = diskReads;
            if (block->DiskWriteCount < diskWrites)
                block->DiskWriteCount = diskWrites;
            if (block->DiskReadRaw < diskReadRaw)
                block->DiskReadRaw = diskReadRaw;
            if (block->DiskWriteRaw < diskWriteRaw)
                block->DiskWriteRaw = diskWriteRaw;
        }

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

        if (runCount != 0)
        {
            block->CurrentDiskRead = block->DiskReadRawDelta.Delta;
            block->CurrentDiskWrite = block->DiskWriteRawDelta.Delta;
            block->CurrentNetworkSend = block->NetworkSendRawDelta.Delta;
            block->CurrentNetworkReceive = block->NetworkReceiveRawDelta.Delta;

            PhAddItemCircularBuffer_ULONG64(&block->DiskReadHistory, block->CurrentDiskRead);
            PhAddItemCircularBuffer_ULONG64(&block->DiskWriteHistory, block->CurrentDiskWrite);
            PhAddItemCircularBuffer_ULONG64(&block->NetworkSendHistory, block->CurrentNetworkSend);
            PhAddItemCircularBuffer_ULONG64(&block->NetworkReceiveHistory, block->CurrentNetworkReceive);
        }

        listEntry = listEntry->Flink;
    }

    // Update history buffers.

    if (runCount != 0)
    {
        PhAddItemCircularBuffer_ULONG(&EtDiskReadHistory, EtDiskReadDelta.Delta);
        PhAddItemCircularBuffer_ULONG(&EtDiskWriteHistory, EtDiskWriteDelta.Delta);
        PhAddItemCircularBuffer_ULONG(&EtNetworkReceiveHistory, EtNetworkReceiveDelta.Delta);
        PhAddItemCircularBuffer_ULONG(&EtNetworkSendHistory, EtNetworkSendDelta.Delta);

        if (maxDiskBlock)
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxDiskHistory, HandleToUlong(maxDiskBlock->ProcessItem->ProcessId));
#ifdef PH_RECORD_MAX_USAGE
            PhAddItemCircularBuffer_ULONG64(&PhMaxDiskUsageHistory, maxDiskValue);
#endif
            PhReferenceProcessRecordForStatistics(maxDiskBlock->ProcessItem->Record);
        }
        else
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxDiskHistory, 0);
#ifdef PH_RECORD_MAX_USAGE
            PhAddItemCircularBuffer_ULONG64(&PhMaxDiskUsageHistory, 0);
#endif
        }

        if (maxNetworkBlock)
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxNetworkHistory, HandleToUlong(maxNetworkBlock->ProcessItem->ProcessId));
#ifdef PH_RECORD_MAX_USAGE
            PhAddItemCircularBuffer_ULONG64(&PhMaxNetworkUsageHistory, maxNetworkValue);
#endif
            PhReferenceProcessRecordForStatistics(maxNetworkBlock->ProcessItem->Record);
        }
        else
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxNetworkHistory, 0);
#ifdef PH_RECORD_MAX_USAGE
            PhAddItemCircularBuffer_ULONG64(&PhMaxNetworkUsageHistory, 0);
#endif
        }
    }

    runCount++;
}

VOID NTAPI EtEtwNetworkItemsUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
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
        PH_UINT64_DELTA oldDeltas[4];

        block = CONTAINING_RECORD(listEntry, ET_NETWORK_BLOCK, ListEntry);

        memcpy(oldDeltas, block->Deltas, sizeof(block->Deltas));

        PhUpdateDelta(&block->ReceiveDelta, block->ReceiveCount);
        PhUpdateDelta(&block->ReceiveRawDelta, block->ReceiveRaw);
        PhUpdateDelta(&block->SendDelta, block->SendCount);
        PhUpdateDelta(&block->SendRawDelta, block->SendRaw);

        if (memcmp(oldDeltas, block->Deltas, sizeof(block->Deltas)))
        {
            // Values have changed. Invalidate the network node.
            PhAcquireQueuedLockExclusive(&block->TextCacheLock);
            memset(block->TextCacheValid, 0, sizeof(block->TextCacheValid));
            PhReleaseQueuedLockExclusive(&block->TextCacheLock);
        }

        listEntry = listEntry->Flink;
    }
}

VOID EtpUpdateProcessInformation(
    VOID
    )
{
    PhAcquireQueuedLockExclusive(&EtpProcessInformationLock);

    if (EtpProcessInformation)
    {
        PhFree(EtpProcessInformation);
        EtpProcessInformation = NULL;
    }

    PhEnumProcesses(&EtpProcessInformation);

    PhReleaseQueuedLockExclusive(&EtpProcessInformationLock);
}

HANDLE EtThreadIdToProcessId(
    _In_ HANDLE ThreadId
    )
{
    PSYSTEM_PROCESS_INFORMATION process;
    ULONG i;
    HANDLE processId;

    PhAcquireQueuedLockShared(&EtpProcessInformationLock);

    if (!EtpProcessInformation)
    {
        PhReleaseQueuedLockShared(&EtpProcessInformationLock);
        return SYSTEM_PROCESS_ID;
    }

    process = PH_FIRST_PROCESS(EtpProcessInformation);

    do
    {
        for (i = 0; i < process->NumberOfThreads; i++)
        {
            if (process->Threads[i].ClientId.UniqueThread == ThreadId)
            {
                processId = process->UniqueProcessId;
                PhReleaseQueuedLockShared(&EtpProcessInformationLock);

                return processId;
            }
        }
    } while (process = PH_NEXT_PROCESS(process));

    PhReleaseQueuedLockShared(&EtpProcessInformationLock);

    return SYSTEM_PROCESS_ID;
}
