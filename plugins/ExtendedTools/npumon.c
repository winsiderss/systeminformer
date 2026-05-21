/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2016-2023
 *     jxy-s   2024
 *
 */

#include "exttools.h"
#include <devguid.h>
#include <ntddvdeo.h>
#include "npumon.h"

static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;

BOOLEAN EtNpuEnabled = FALSE;
BOOLEAN EtNpuSupported = FALSE;
BOOLEAN EtNpuD3DEnabled = FALSE;
PPH_LIST EtpNpuAdapterList;

ULONG EtNpuTotalNodeCount = 0;
ULONG EtNpuTotalSegmentCount = 0;
ULONG EtNpuNextNodeIndex = 0;

ULONG64 EtNpuSystemRunningTimeDelta = 0;

FLOAT EtNpuNodeUsage = 0;
PH_CIRCULAR_BUFFER_FLOAT EtNpuNodeHistory;
PH_CIRCULAR_BUFFER_ULONG EtMaxNpuNodeHistory; // ID of max. NPU usage process
PH_CIRCULAR_BUFFER_FLOAT EtMaxNpuNodeUsageHistory;

PPH_UINT64_DELTA EtNpuNodesTotalRunningTimeDelta;
PPH_UINT64_DELTA EtNpuNodesSystemRunningTimeDelta;
PPH_CIRCULAR_BUFFER_FLOAT EtNpuNodesHistory;

ULONG64 EtNpuDedicatedLimit = 0;
ULONG64 EtNpuDedicatedUsage = 0;
ULONG64 EtNpuSharedLimit = 0;
ULONG64 EtNpuSharedUsage = 0;
FLOAT EtNpuPowerUsageLimit = 100.0f;
FLOAT EtNpuPowerUsage = 0.0f;
FLOAT EtNpuTemperatureLimit = 0.0f;
FLOAT EtNpuTemperature = 0.0f;
ULONG64 EtNpuFanRpmLimit = 0;
ULONG64 EtNpuFanRpm = 0;
PH_CIRCULAR_BUFFER_ULONG64 EtNpuDedicatedHistory;
PH_CIRCULAR_BUFFER_ULONG64 EtNpuSharedHistory;
PH_CIRCULAR_BUFFER_FLOAT EtNpuPowerUsageHistory;
PH_CIRCULAR_BUFFER_FLOAT EtNpuTemperatureHistory;
PH_CIRCULAR_BUFFER_ULONG64 EtNpuFanRpmHistory;

VOID EtNpuMonitorInitialization(
    VOID
    )
{
    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_NPU_MONITOR))
    {
        EtNpuSupported = EtWindowsVersion >= WINDOWS_10_22H2;
        EtNpuD3DEnabled = EtNpuSupported && !!PhGetIntegerSetting(SETTING_NAME_ENABLE_NPUPERFCOUNTERS);

        EtpNpuAdapterList = PhCreateList(4);

        if (EtpNpuInitializeD3DStatistics())
            EtNpuEnabled = TRUE;
    }

    if (EtNpuD3DEnabled)
    {
        EtPerfCounterInitialization();
    }

    if (EtNpuEnabled)
    {
        ULONG i;

        PhInitializeCircularBuffer_FLOAT(&EtNpuNodeHistory, EtSampleCount);
        PhInitializeCircularBuffer_ULONG(&EtMaxNpuNodeHistory, EtSampleCount);
        PhInitializeCircularBuffer_FLOAT(&EtMaxNpuNodeUsageHistory, EtSampleCount);
        PhInitializeCircularBuffer_ULONG64(&EtNpuDedicatedHistory, EtSampleCount);
        PhInitializeCircularBuffer_ULONG64(&EtNpuSharedHistory, EtSampleCount);
        if (EtNpuSupported)
        {
            PhInitializeCircularBuffer_FLOAT(&EtNpuPowerUsageHistory, EtSampleCount);
            PhInitializeCircularBuffer_FLOAT(&EtNpuTemperatureHistory, EtSampleCount);
            PhInitializeCircularBuffer_ULONG64(&EtNpuFanRpmHistory, EtSampleCount);
        }

        if (!EtNpuD3DEnabled)
        {
            EtNpuNodesTotalRunningTimeDelta = PhAllocateZero(sizeof(PH_UINT64_DELTA) * EtNpuTotalNodeCount);
            EtNpuNodesSystemRunningTimeDelta = PhAllocateZero(sizeof(PH_UINT64_DELTA) * EtNpuTotalNodeCount);
        }
        EtNpuNodesHistory = PhAllocateZero(sizeof(PH_CIRCULAR_BUFFER_FLOAT) * EtNpuTotalNodeCount);

        for (i = 0; i < EtNpuTotalNodeCount; i++)
        {
            PhInitializeCircularBuffer_FLOAT(&EtNpuNodesHistory[i], EtSampleCount);
        }

        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
            EtNpuProcessesUpdatedCallback,
            NULL,
            &ProcessesUpdatedCallbackRegistration
            );
    }
}

PETP_NPU_ADAPTER EtpAddNpuAdapter(
    _In_ PPH_STRING DeviceInterface,
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ LUID AdapterLuid,
    _In_ ULONG NumberOfSegments,
    _In_ ULONG NumberOfNodes
    )
{
    PETP_NPU_ADAPTER adapter;

    adapter = EtpAllocateNpuAdapter(NumberOfSegments);
    adapter->DeviceInterface = PhReferenceObject(DeviceInterface);
    adapter->AdapterLuid = AdapterLuid;
    adapter->NodeCount = NumberOfNodes;
    adapter->SegmentCount = NumberOfSegments;
    RtlInitializeBitMap(&adapter->ApertureBitMap, adapter->ApertureBitMapBuffer, NumberOfSegments);

    if (DeviceInterface)
    {
        PPH_STRING description;

        if (EtQueryDeviceProperties(DeviceInterface, &description, NULL, NULL, NULL, NULL))
        {
            adapter->Description = description;
        }
    }

    if (EtNpuSupported)
    {
        adapter->NodeNameList = PhCreateList(adapter->NodeCount);

        for (ULONG i = 0; i < adapter->NodeCount; i++)
        {
            D3DKMT_NODEMETADATA metaDataInfo;

            memset(&metaDataInfo, 0, sizeof(D3DKMT_NODEMETADATA));
            metaDataInfo.NodeOrdinalAndAdapterIndex = MAKEWORD(i, 0);

            if (NT_SUCCESS(EtQueryAdapterInformation(
                AdapterHandle,
                KMTQAITYPE_NODEMETADATA,
                &metaDataInfo,
                sizeof(D3DKMT_NODEMETADATA)
                )))
            {
                PhAddItemList(adapter->NodeNameList, EtGetNodeEngineTypeString(&metaDataInfo));
            }
            else
            {
                PhAddItemList(adapter->NodeNameList, PhReferenceEmptyString());
            }
        }
    }

    PhAddItemList(EtpNpuAdapterList, adapter);

    return adapter;
}

BOOLEAN EtpNpuInitializeD3DStatistics(
    VOID
    )
{
    PPH_LIST discoveredAdapterList;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    D3DKMT_ADAPTER_PERFDATACAPS perfCaps;
    ULONG processedCount = 0;

    discoveredAdapterList = EtInitializeGraphicsAdapters();
    if (!discoveredAdapterList)
        return FALSE;

    for (ULONG i = 0; i < discoveredAdapterList->Count; i++)
    {
        PET_DISCOVERED_ADAPTER entry = discoveredAdapterList->Items[i];
        D3DKMT_HANDLE adapterHandle = 0;
        LUID adapterLuid;

        if (!entry->Attributes.TypeNpu)
            continue;

        if (entry->Attributes.TypeGpu || entry->Attributes.TypeComputeAccelerator)
            continue;

        adapterLuid = entry->AdapterLuid;

        if (entry->AdapterHandle)
        {
            adapterHandle = entry->AdapterHandle;
        }
        else if (entry->DeviceInterface)
        {
            if (!NT_SUCCESS(EtOpenAdapterFromDeviceName(
                &adapterHandle,
                &adapterLuid,
                PhGetString(entry->DeviceInterface)
                )))
            {
                continue;
            }
        }

        if (!adapterHandle)
            continue;

        if (EtNpuSupported && processedCount > 0)
        {
            if (EtIsSoftwareDevice(adapterHandle))
            {
                if (entry->AdapterHandle == 0)
                    EtCloseAdapterHandle(adapterHandle);
                continue;
            }
        }

        if (EtNpuSupported)
        {
            D3DKMT_SEGMENTSIZEINFO segmentInfo;

            memset(&segmentInfo, 0, sizeof(D3DKMT_SEGMENTSIZEINFO));

            if (NT_SUCCESS(EtQueryAdapterInformation(
                adapterHandle,
                KMTQAITYPE_GETSEGMENTSIZE,
                &segmentInfo,
                sizeof(D3DKMT_SEGMENTSIZEINFO)
                )))
            {
                EtNpuDedicatedLimit += segmentInfo.DedicatedVideoMemorySize;
                EtNpuSharedLimit += segmentInfo.SharedSystemMemorySize;
            }

            memset(&perfCaps, 0, sizeof(D3DKMT_ADAPTER_PERFDATACAPS));

            if (NT_SUCCESS(EtQueryAdapterInformation(
                adapterHandle,
                KMTQAITYPE_ADAPTERPERFDATA_CAPS,
                &perfCaps,
                sizeof(D3DKMT_ADAPTER_PERFDATACAPS)
                )))
            {
                //
                // This will be averaged below.
                //
                EtNpuTemperatureLimit += max(perfCaps.TemperatureWarning, perfCaps.TemperatureMax);
                EtNpuFanRpmLimit += perfCaps.MaxFanRPM;
            }
        }

        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
        queryStatistics.AdapterLuid = adapterLuid;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            PETP_NPU_ADAPTER gpuAdapter;

            gpuAdapter = EtpAddNpuAdapter(
                entry->DeviceInterface,
                adapterHandle,
                adapterLuid,
                queryStatistics.QueryResult.AdapterInformation.NbSegments,
                queryStatistics.QueryResult.AdapterInformation.NodeCount
                );

            gpuAdapter->FirstNodeIndex = EtNpuNextNodeIndex;
            EtNpuTotalNodeCount += gpuAdapter->NodeCount;
            EtNpuTotalSegmentCount += gpuAdapter->SegmentCount;
            EtNpuNextNodeIndex += gpuAdapter->NodeCount;

            for (ULONG ii = 0; ii < gpuAdapter->SegmentCount; ii++)
            {
                memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
                queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
                queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
                queryStatistics.QuerySegment.SegmentId = ii;

                if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
                {
                    ULONG64 commitLimit;
                    ULONG aperture;

                    if (EtWindowsVersion >= WINDOWS_8)
                    {
                        commitLimit = queryStatistics.QueryResult.SegmentInformation.CommitLimit;
                        aperture = queryStatistics.QueryResult.SegmentInformation.Aperture;
                    }
                    else
                    {
                        PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1 segmentInfoV1;

                        segmentInfoV1 = (PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1)&queryStatistics.QueryResult;
                        commitLimit = segmentInfoV1->CommitLimit;
                        aperture = segmentInfoV1->Aperture;
                    }

                    if (!EtNpuSupported || !EtNpuD3DEnabled)
                    {
                        if (aperture)
                            EtNpuSharedLimit += commitLimit;
                        else
                            EtNpuDedicatedLimit += commitLimit;
                    }

                    if (aperture)
                        RtlSetBits(&gpuAdapter->ApertureBitMap, ii, 1);
                }
            }
        }

        if (entry->AdapterHandle == 0)
            EtCloseAdapterHandle(adapterHandle);

        processedCount++;
    }

    if (EtNpuSupported && processedCount > 0)
    {
        //
        // Use the average as the limit since we show one graph for all.
        //
        EtNpuTemperatureLimit /= processedCount;
        EtNpuFanRpmLimit /= processedCount;

        // Set limit at 100C (dmex)
        if (EtNpuTemperatureLimit == 0)
            EtNpuTemperatureLimit = 100;
    }

    EtUninitializeGraphicsAdapters(discoveredAdapterList);

    if (EtNpuTotalNodeCount == 0)
        return FALSE;

    return TRUE;
}

PETP_NPU_ADAPTER EtpAllocateNpuAdapter(
    _In_ ULONG NumberOfSegments
    )
{
    PETP_NPU_ADAPTER adapter;
    SIZE_T sizeNeeded;

    sizeNeeded = FIELD_OFFSET(ETP_NPU_ADAPTER, ApertureBitMapBuffer);
    sizeNeeded += BYTES_NEEDED_FOR_BITS(NumberOfSegments);

    adapter = PhAllocate(sizeNeeded);
    memset(adapter, 0, sizeNeeded);

    return adapter;
}

VOID EtpNpuUpdateProcessSegmentInformation(
    _In_ PET_PROCESS_BLOCK Block
    )
{
    ULONG i;
    ULONG j;
    PETP_NPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 dedicatedUsage;
    ULONG64 sharedUsage;
    ULONG64 commitUsage;

    if (!Block->ProcessItem->QueryHandle)
        return;

    dedicatedUsage = 0;
    sharedUsage = 0;
    commitUsage = 0;

    for (i = 0; i < EtpNpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpNpuAdapterList->Items[i];

        for (j = 0; j < gpuAdapter->SegmentCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.hProcess = Block->ProcessItem->QueryHandle;
            queryStatistics.QueryProcessSegment.SegmentId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                ULONG64 bytesCommitted;

                if (EtWindowsVersion >= WINDOWS_8)
                    bytesCommitted = queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                else
                    bytesCommitted = (ULONG)queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;

                if (RtlCheckBit(&gpuAdapter->ApertureBitMap, j))
                    sharedUsage += bytesCommitted;
                else
                    dedicatedUsage += bytesCommitted;
            }
        }

        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS;
        queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
        queryStatistics.hProcess = Block->ProcessItem->QueryHandle;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            commitUsage += queryStatistics.QueryResult.ProcessInformation.SystemMemory.BytesAllocated;
        }
    }

    Block->NpuDedicatedUsage = dedicatedUsage;
    Block->NpuSharedUsage = sharedUsage;
    Block->NpuCommitUsage = commitUsage;
}

VOID EtpNpuUpdateSystemSegmentInformation(
    VOID
    )
{
    ULONG i;
    ULONG j;
    PETP_NPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 dedicatedUsage;
    ULONG64 sharedUsage;

    dedicatedUsage = 0;
    sharedUsage = 0;

    for (i = 0; i < EtpNpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpNpuAdapterList->Items[i];

        for (j = 0; j < gpuAdapter->SegmentCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.QuerySegment.SegmentId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                ULONG64 bytesResident;
                ULONG aperture;

                if (EtWindowsVersion >= WINDOWS_8)
                {
                    bytesResident = queryStatistics.QueryResult.SegmentInformation.BytesResident;
                    aperture = queryStatistics.QueryResult.SegmentInformation.Aperture;
                }
                else
                {
                    PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1 segmentInfo;

                    segmentInfo = (PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1)&queryStatistics.QueryResult;
                    bytesResident = segmentInfo->BytesResident;
                    aperture = segmentInfo->Aperture;
                }

                if (aperture) // RtlCheckBit(&gpuAdapter->ApertureBitMap, j)
                    sharedUsage += bytesResident;
                else
                    dedicatedUsage += bytesResident;
            }
        }
    }

    EtNpuDedicatedUsage = dedicatedUsage;
    EtNpuSharedUsage = sharedUsage;
}

VOID EtpNpuUpdateProcessNodeInformation(
    _In_ PET_PROCESS_BLOCK Block
    )
{
    ULONG i;
    ULONG j;
    PETP_NPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 totalRunningTime;

    if (!Block->ProcessItem->QueryHandle)
        return;

    totalRunningTime = 0;

    for (i = 0; i < EtpNpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpNpuAdapterList->Items[i];

        for (j = 0; j < gpuAdapter->NodeCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_NODE;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.hProcess = Block->ProcessItem->QueryHandle;
            queryStatistics.QueryProcessNode.NodeId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                //ULONG64 runningTime;
                //runningTime = queryStatistics.QueryResult.ProcessNodeInformation.RunningTime.QuadPart;
                //PhUpdateDelta(&Block->NpuTotalRunningTimeDelta[j], runningTime);

                totalRunningTime += queryStatistics.QueryResult.ProcessNodeInformation.RunningTime.QuadPart;
                //totalContextSwitches += queryStatistics.QueryResult.ProcessNodeInformation.ContextSwitch;
            }
        }
    }

    PhUpdateDelta(&Block->NpuRunningTimeDelta, totalRunningTime);
}

VOID EtpNpuUpdateSystemNodeInformation(
    VOID
    )
{
    PETP_NPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 maxSystemDelta = 0;

    for (ULONG i = 0; i < EtpNpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpNpuAdapterList->Items[i];

        for (ULONG j = 0; j < gpuAdapter->NodeCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_NODE;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.QueryNode.NodeId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                ULONG nodeIndex = gpuAdapter->FirstNodeIndex + j;
                ULONG64 runningTime;
                ULONG64 systemRunningTime;

                runningTime = queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
                systemRunningTime = queryStatistics.QueryResult.NodeInformation.SystemInformation.RunningTime.QuadPart;

                PhUpdateDelta(&EtNpuNodesTotalRunningTimeDelta[nodeIndex], runningTime);
                PhUpdateDelta(&EtNpuNodesSystemRunningTimeDelta[nodeIndex], systemRunningTime);

                if (EtNpuNodesSystemRunningTimeDelta[nodeIndex].Delta > maxSystemDelta)
                    maxSystemDelta = EtNpuNodesSystemRunningTimeDelta[nodeIndex].Delta;
            }
        }
    }

    EtNpuSystemRunningTimeDelta = maxSystemDelta;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI EtNpuProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROVIDER_UPDATED_EVENT updateEvent = Parameter;
    ULONG runCount;
    DOUBLE elapsedTime = 0; // total NPU node elapsed time in micro-seconds
    FLOAT tempNpuUsage = 0;
    ULONG i;
    PLIST_ENTRY listEntry;
    FLOAT maxNodeValue = 0;
    PET_PROCESS_BLOCK maxNodeBlock = NULL;

    if (!updateEvent)
        return;

    runCount = updateEvent->RunCount;

    if (runCount < 2)
        return;

    if (EtNpuD3DEnabled)
    {
        FLOAT gpuTotal;
        ULONG64 dedicatedTotal;
        ULONG64 sharedTotal;

        gpuTotal = EtLookupTotalNpuUtilization();
        dedicatedTotal = EtLookupTotalNpuDedicated();
        sharedTotal = EtLookupTotalNpuShared();

        if (gpuTotal > 1)
            gpuTotal = 1;

        if (gpuTotal > tempNpuUsage)
            tempNpuUsage = gpuTotal;

        EtNpuNodeUsage = tempNpuUsage;
        EtNpuDedicatedUsage = dedicatedTotal;
        EtNpuSharedUsage = sharedTotal;
    }
    else
    {
        EtpNpuUpdateSystemSegmentInformation();
        EtpNpuUpdateSystemNodeInformation();

        elapsedTime = (DOUBLE)EtNpuSystemRunningTimeDelta;

        if (elapsedTime != 0)
        {
            for (i = 0; i < EtNpuTotalNodeCount; i++)
            {
                FLOAT usage = (FLOAT)(EtNpuNodesTotalRunningTimeDelta[i].Delta / elapsedTime);

                if (usage > 1)
                    usage = 1;

                if (usage > tempNpuUsage)
                    tempNpuUsage = usage;
            }
        }

        EtNpuNodeUsage = tempNpuUsage;
    }

    if (EtNpuSupported && EtpNpuAdapterList->Count)
    {
        FLOAT powerUsage;
        FLOAT temperature;
        ULONG64 fanRpm;

        powerUsage = 0.0f;
        temperature = 0.0f;
        fanRpm = 0;

        for (i = 0; i < EtpNpuAdapterList->Count; i++)
        {
            PETP_NPU_ADAPTER gpuAdapter;
            D3DKMT_HANDLE adapterHandle;
            D3DKMT_ADAPTER_PERFDATA adapterPerfData;

            gpuAdapter = EtpNpuAdapterList->Items[i];

            //
            // jxy-s: we open this frequently, consider opening this once in the list
            //
            if (!NT_SUCCESS(EtOpenAdapterFromDeviceName(&adapterHandle, NULL, PhGetString(gpuAdapter->DeviceInterface))))
                continue;

            memset(&adapterPerfData, 0, sizeof(D3DKMT_ADAPTER_PERFDATA));

            if (NT_SUCCESS(EtQueryAdapterInformation(
                adapterHandle,
                KMTQAITYPE_ADAPTERPERFDATA,
                &adapterPerfData,
                sizeof(D3DKMT_ADAPTER_PERFDATA)
                )))
            {
                powerUsage += (((FLOAT)adapterPerfData.Power / 1000) * 100);
                temperature += (((FLOAT)adapterPerfData.Temperature / 1000) * 100);
                fanRpm += adapterPerfData.FanRPM;
            }

            EtCloseAdapterHandle(adapterHandle);
        }

        EtNpuPowerUsage = powerUsage / EtpNpuAdapterList->Count;
        EtNpuTemperature = temperature / EtpNpuAdapterList->Count;
        EtNpuFanRpm = fanRpm / EtpNpuAdapterList->Count;

        //
        // Update the limits if we see higher values
        //

        if (EtNpuPowerUsage > EtNpuPowerUsageLimit)
        {
            //
            // Possibly over-clocked power limit
            //
            EtNpuPowerUsageLimit = EtNpuPowerUsage;
        }

        if (EtNpuTemperature > EtNpuTemperatureLimit)
        {
            //
            // Damn that card is hawt
            //
            EtNpuTemperatureLimit = EtNpuTemperature;
        }

        if (EtNpuFanRpm > EtNpuFanRpmLimit)
        {
            //
            // Fan go brrrrrr
            //
            EtNpuFanRpmLimit = EtNpuFanRpm;
        }
    }

    // Update per-process statistics.
    // Note: no lock is needed because we only ever modify the list on this same thread.

    listEntry = EtProcessBlockListHead.Flink;

    while (listEntry != &EtProcessBlockListHead)
    {
        PET_PROCESS_BLOCK block;

        block = CONTAINING_RECORD(listEntry, ET_PROCESS_BLOCK, ListEntry);

        if (block->ProcessItem->State & PH_PROCESS_ITEM_REMOVED)
        {
            listEntry = listEntry->Flink;
            continue;
        }

        if (EtNpuD3DEnabled)
        {
            ULONG64 sharedUsage;
            ULONG64 dedicatedUsage;
            ULONG64 commitUsage;

            block->NpuNodeUtilization = EtLookupProcessNpuUtilization(block->ProcessItem->ProcessId);

            if (EtLookupProcessNpuMemoryCounters(
                block->ProcessItem->ProcessId,
                &sharedUsage,
                &dedicatedUsage,
                &commitUsage
                ))
            {
                block->NpuSharedUsage = sharedUsage;
                block->NpuDedicatedUsage = dedicatedUsage;
                block->NpuCommitUsage = commitUsage;
            }
            else
            {
                block->NpuSharedUsage = 0;
                block->NpuDedicatedUsage = 0;
                block->NpuCommitUsage = 0;
            }

            if (runCount != 0)
            {
                block->NpuCurrentUsage = block->NpuNodeUtilization;
                block->NpuCurrentMemUsage = (ULONG)(block->NpuDedicatedUsage / PAGE_SIZE);
                block->NpuCurrentMemSharedUsage = (ULONG)(block->NpuSharedUsage / PAGE_SIZE);
                block->NpuCurrentCommitUsage = (ULONG)(block->NpuCommitUsage / PAGE_SIZE);

                ET_CIRCULAR_BUFFER_ADD_FLOAT(&block->NpuHistory, block->NpuCurrentUsage);
                ET_CIRCULAR_BUFFER_ADD_ULONG(&block->NpuMemoryHistory, block->NpuCurrentMemUsage);
                ET_CIRCULAR_BUFFER_ADD_ULONG(&block->NpuMemorySharedHistory, block->NpuCurrentMemSharedUsage);
                ET_CIRCULAR_BUFFER_ADD_ULONG(&block->NpuCommittedHistory, block->NpuCurrentCommitUsage);
            }
        }
        else
        {
            EtpNpuUpdateProcessSegmentInformation(block);
            EtpNpuUpdateProcessNodeInformation(block);

            if (elapsedTime != 0)
            {
                block->NpuNodeUtilization = (FLOAT)(block->NpuRunningTimeDelta.Delta / elapsedTime);

                //for (i = 0; i < EtNpuTotalNodeCount; i++)
                //{
                //    FLOAT usage = (FLOAT)(block->NpuTotalRunningTimeDelta[i].Delta / elapsedTime);
                //
                //    if (usage > block->NpuNodeUtilization)
                //    {
                //        block->NpuNodeUtilization = usage;
                //    }
                //}

                if (block->NpuNodeUtilization > 1)
                    block->NpuNodeUtilization = 1;

                if (runCount != 0)
                {
                    block->NpuCurrentUsage = block->NpuNodeUtilization;
                    block->NpuCurrentMemUsage = (ULONG)(block->NpuDedicatedUsage / PAGE_SIZE);
                    block->NpuCurrentMemSharedUsage = (ULONG)(block->NpuSharedUsage / PAGE_SIZE);
                    block->NpuCurrentCommitUsage = (ULONG)(block->NpuCommitUsage / PAGE_SIZE);

                    PhAddItemCircularBuffer_FLOAT(&block->NpuHistory, block->NpuCurrentUsage);
                    PhAddItemCircularBuffer_ULONG(&block->NpuMemoryHistory, block->NpuCurrentMemUsage);
                    PhAddItemCircularBuffer_ULONG(&block->NpuMemorySharedHistory, block->NpuCurrentMemSharedUsage);
                    PhAddItemCircularBuffer_ULONG(&block->NpuCommittedHistory, block->NpuCurrentCommitUsage);
                }
            }
        }

        if (maxNodeValue < block->NpuNodeUtilization)
        {
            maxNodeValue = block->NpuNodeUtilization;
            maxNodeBlock = block;
        }

        listEntry = listEntry->Flink;
    }

    // Update history buffers.

    if (runCount != 0)
    {
        PhAddItemCircularBuffer_FLOAT(&EtNpuNodeHistory, EtNpuNodeUsage);
        PhAddItemCircularBuffer_ULONG64(&EtNpuDedicatedHistory, EtNpuDedicatedUsage);
        PhAddItemCircularBuffer_ULONG64(&EtNpuSharedHistory, EtNpuSharedUsage);
        if (EtNpuSupported)
        {
            PhAddItemCircularBuffer_FLOAT(&EtNpuPowerUsageHistory, EtNpuPowerUsage);
            PhAddItemCircularBuffer_FLOAT(&EtNpuTemperatureHistory, EtNpuTemperature);
            PhAddItemCircularBuffer_ULONG64(&EtNpuFanRpmHistory, EtNpuFanRpm);
        }

        if (EtNpuD3DEnabled)
        {
            for (i = 0; i < EtNpuTotalNodeCount; i++)
            {
                FLOAT usage;

                usage = EtLookupTotalNpuEngineUtilization(i);

                if (usage > 1)
                    usage = 1;

                PhAddItemCircularBuffer_FLOAT(&EtNpuNodesHistory[i], usage);
            }
        }
        else
        {
            if (elapsedTime != 0)
            {
                for (i = 0; i < EtNpuTotalNodeCount; i++)
                {
                    FLOAT usage;

                    usage = (FLOAT)(EtNpuNodesTotalRunningTimeDelta[i].Delta / elapsedTime);

                    if (usage > 1)
                        usage = 1;

                    PhAddItemCircularBuffer_FLOAT(&EtNpuNodesHistory[i], usage);
                }
            }
            else
            {
                for (i = 0; i < EtNpuTotalNodeCount; i++)
                    PhAddItemCircularBuffer_FLOAT(&EtNpuNodesHistory[i], 0);
            }
        }

        if (maxNodeBlock)
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxNpuNodeHistory, HandleToUlong(maxNodeBlock->ProcessItem->ProcessId));
            PhAddItemCircularBuffer_FLOAT(&EtMaxNpuNodeUsageHistory, maxNodeBlock->NpuNodeUtilization);
            PhReferenceProcessRecordForStatistics(maxNodeBlock->ProcessItem->Record);
        }
        else
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxNpuNodeHistory, 0);
            PhAddItemCircularBuffer_FLOAT(&EtMaxNpuNodeUsageHistory, 0);
        }
    }
}

ULONG EtGetNpuAdapterCount(
    VOID
    )
{
    return EtpNpuAdapterList->Count;
}

ULONG EtGetNpuAdapterIndexFromNodeIndex(
    _In_ ULONG NodeIndex
    )
{
    ULONG i;
    PETP_NPU_ADAPTER gpuAdapter;

    for (i = 0; i < EtpNpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpNpuAdapterList->Items[i];

        if (NodeIndex >= gpuAdapter->FirstNodeIndex && NodeIndex < gpuAdapter->FirstNodeIndex + gpuAdapter->NodeCount)
            return i;
    }

    return ULONG_MAX;
}

PPH_STRING EtGetNpuAdapterNodeDescription(
    _In_ ULONG Index,
    _In_ ULONG NodeIndex
    )
{
    PETP_NPU_ADAPTER gpuAdapter;

    if (Index >= EtpNpuAdapterList->Count)
        return NULL;

    gpuAdapter = EtpNpuAdapterList->Items[Index];

    if (!gpuAdapter->NodeNameList)
        return NULL;

    return gpuAdapter->NodeNameList->Items[NodeIndex - gpuAdapter->FirstNodeIndex];
}

PPH_STRING EtGetNpuAdapterDescription(
    _In_ ULONG Index
    )
{
    PPH_STRING description;

    if (Index >= EtpNpuAdapterList->Count)
        return NULL;

    description = ((PETP_NPU_ADAPTER)EtpNpuAdapterList->Items[Index])->Description;

    if (description)
    {
        PhReferenceObject(description);
        return description;
    }
    else
    {
        return NULL;
    }
}

VOID EtQueryProcessNpuStatistics(
    _In_ HANDLE ProcessHandle,
    _Out_ PET_PROCESS_NPU_STATISTICS Statistics
    )
{
    ULONG i;
    ULONG j;
    PETP_NPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;

    memset(Statistics, 0, sizeof(ET_PROCESS_NPU_STATISTICS));

    for (i = 0; i < EtpNpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpNpuAdapterList->Items[i];

        Statistics->SegmentCount += gpuAdapter->SegmentCount;
        Statistics->NodeCount += gpuAdapter->NodeCount;

        for (j = 0; j < gpuAdapter->SegmentCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.hProcess = ProcessHandle;
            queryStatistics.QueryProcessSegment.SegmentId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                ULONG64 bytesCommitted;

                if (EtWindowsVersion >= WINDOWS_8)
                    bytesCommitted = queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                else
                    bytesCommitted = (ULONG)queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;

                if (RtlCheckBit(&gpuAdapter->ApertureBitMap, j))
                    Statistics->SharedCommitted += bytesCommitted;
                else
                    Statistics->DedicatedCommitted += bytesCommitted;
            }
        }

        for (j = 0; j < gpuAdapter->NodeCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_NODE;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.hProcess = ProcessHandle;
            queryStatistics.QueryProcessNode.NodeId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                Statistics->RunningTime += queryStatistics.QueryResult.ProcessNodeInformation.RunningTime.QuadPart;
                Statistics->ContextSwitches += queryStatistics.QueryResult.ProcessNodeInformation.ContextSwitch;
            }
        }

        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS;
        queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
        queryStatistics.hProcess = ProcessHandle;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            Statistics->BytesAllocated += queryStatistics.QueryResult.ProcessInformation.SystemMemory.BytesAllocated;
            Statistics->BytesReserved += queryStatistics.QueryResult.ProcessInformation.SystemMemory.BytesReserved;
            Statistics->WriteCombinedBytesAllocated += queryStatistics.QueryResult.ProcessInformation.SystemMemory.WriteCombinedBytesAllocated;
            Statistics->WriteCombinedBytesReserved += queryStatistics.QueryResult.ProcessInformation.SystemMemory.WriteCombinedBytesReserved;
            Statistics->CachedBytesAllocated += queryStatistics.QueryResult.ProcessInformation.SystemMemory.CachedBytesAllocated;
            Statistics->CachedBytesReserved += queryStatistics.QueryResult.ProcessInformation.SystemMemory.CachedBytesReserved;
            Statistics->SectionBytesAllocated += queryStatistics.QueryResult.ProcessInformation.SystemMemory.SectionBytesAllocated;
            Statistics->SectionBytesReserved += queryStatistics.QueryResult.ProcessInformation.SystemMemory.SectionBytesReserved;
        }
    }
}

D3DKMT_CLIENTHINT EtQueryProcessNpuClientHint(
    _In_ LUID AdapterLuid,
    _In_ HANDLE ProcessHandle
    )
{
    D3DKMT_QUERYSTATISTICS queryStatistics;

    memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
    queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER;
    queryStatistics.AdapterLuid = AdapterLuid;
    queryStatistics.hProcess = ProcessHandle;

    if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
    {
        return queryStatistics.QueryResult.ProcessAdapterInformation.ClientHint;
    }

    return D3DKMT_CLIENTHINT_UNKNOWN;
}
