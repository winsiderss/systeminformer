/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2016-2023
 *
 */

#include "exttools.h"
#include <devguid.h>
#include <ntddvdeo.h>
#include "gpumon.h"

static PH_CALLBACK_REGISTRATION EtGpuProcessesUpdatedCallbackRegistration;

BOOLEAN EtGpuEnabled = FALSE;
BOOLEAN EtGpuSupported = FALSE;
BOOLEAN EtGpuD3DEnabled = FALSE;
BOOLEAN EtGpuD3DEnumProcesses = FALSE;
PPH_LIST EtpGpuAdapterList;

ULONG EtGpuTotalNodeCount = 0;
ULONG EtGpuTotalSegmentCount = 0;
ULONG EtGpuNextNodeIndex = 0;

ULONG64 EtGpuSystemRunningTimeDelta = 0;

FLOAT EtGpuNodeUsage = 0;
PH_CIRCULAR_BUFFER_FLOAT EtGpuNodeHistory;
PH_CIRCULAR_BUFFER_ULONG EtMaxGpuNodeHistory; // ID of max. GPU usage process
PH_CIRCULAR_BUFFER_FLOAT EtMaxGpuNodeUsageHistory;

PPH_UINT64_DELTA EtGpuNodesTotalRunningTimeDelta;
PPH_UINT64_DELTA EtGpuNodesSystemRunningTimeDelta;
PPH_CIRCULAR_BUFFER_FLOAT EtGpuNodesHistory;

ULONG64 EtGpuDedicatedLimit = 0;
ULONG64 EtGpuDedicatedUsage = 0;
ULONG64 EtGpuSharedLimit = 0;
ULONG64 EtGpuSharedUsage = 0;
FLOAT EtGpuPowerUsageLimit = 100.0f;
FLOAT EtGpuPowerUsage = 0.0f;
FLOAT EtGpuTemperatureLimit = 0.0f;
FLOAT EtGpuTemperature = 0.0f;
ULONG64 EtGpuFanRpmLimit = 0;
ULONG64 EtGpuFanRpm = 0;
PH_CIRCULAR_BUFFER_ULONG64 EtGpuDedicatedHistory;
PH_CIRCULAR_BUFFER_ULONG64 EtGpuSharedHistory;
PH_CIRCULAR_BUFFER_FLOAT EtGpuPowerUsageHistory;
PH_CIRCULAR_BUFFER_FLOAT EtGpuTemperatureHistory;
PH_CIRCULAR_BUFFER_ULONG64 EtGpuFanRpmHistory;

#define ET_UPDATE_PROCESS_STATISTICS_MINMAX(Minimum, Maximum, Difference, Value) \
    if ((Value) != 0 && ((Minimum) == 0 || (Value) < (Minimum))) \
        (Minimum) = (Value); \
    if ((Value) != 0 && ((Maximum) == 0 || (Value) > (Maximum))) \
        (Maximum) = (Value); \
    (Difference) = (Maximum) - (Minimum);

VOID EtGpuMonitorInitialization(
    VOID
    )
{
    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_GPU_MONITOR))
    {
        EtGpuSupported = EtWindowsVersion >= WINDOWS_10_RS4; // Note: Changed to RS4 due to reports of BSODs on LTSB versions of RS3 (dmex)
        EtGpuD3DEnabled = EtGpuSupported && !!PhGetIntegerSetting(SETTING_NAME_ENABLE_GPUPERFCOUNTERS);
        EtGpuD3DEnumProcesses = EtWindowsVersion >= WINDOWS_11;

        EtpGpuAdapterList = PhCreateList(4);

        if (EtpGpuInitializeD3DStatistics())
            EtGpuEnabled = TRUE;
    }

    if (EtGpuD3DEnabled)
    {
        EtPerfCounterInitialization();
    }

    if (EtGpuEnabled)
    {
        ULONG i;

        PhInitializeCircularBuffer_FLOAT(&EtGpuNodeHistory, EtSampleCount);
        PhInitializeCircularBuffer_ULONG(&EtMaxGpuNodeHistory, EtSampleCount);
        PhInitializeCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, EtSampleCount);
        PhInitializeCircularBuffer_ULONG64(&EtGpuDedicatedHistory, EtSampleCount);
        PhInitializeCircularBuffer_ULONG64(&EtGpuSharedHistory, EtSampleCount);

        if (EtGpuSupported)
        {
            PhInitializeCircularBuffer_FLOAT(&EtGpuPowerUsageHistory, EtSampleCount);
            PhInitializeCircularBuffer_FLOAT(&EtGpuTemperatureHistory, EtSampleCount);
            PhInitializeCircularBuffer_ULONG64(&EtGpuFanRpmHistory, EtSampleCount);
        }

        if (!EtGpuD3DEnabled)
        {
            EtGpuNodesTotalRunningTimeDelta = PhAllocateZero(sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
            EtGpuNodesSystemRunningTimeDelta = PhAllocateZero(sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
        }

        EtGpuNodesHistory = PhAllocateZero(sizeof(PH_CIRCULAR_BUFFER_FLOAT) * EtGpuTotalNodeCount);

        for (i = 0; i < EtGpuTotalNodeCount; i++)
        {
            PhInitializeCircularBuffer_FLOAT(&EtGpuNodesHistory[i], EtSampleCount);
        }

        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
            EtGpuProcessesUpdatedCallback,
            NULL,
            &EtGpuProcessesUpdatedCallbackRegistration
            );
    }
}

PETP_GPU_ADAPTER EtpAddGpuAdapter(
    _In_ PPH_STRING DeviceInterface,
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ LUID AdapterLuid,
    _In_ ULONG NumberOfSegments,
    _In_ ULONG NumberOfNodes
    )
{
    PETP_GPU_ADAPTER adapter;
    D3DKMT_HANDLE adapterHandleToUse = AdapterHandle;

    adapter = EtpAllocateGpuAdapter(NumberOfSegments);
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

    // Cache adapter handle for reuse in update callbacks
    if (!adapterHandleToUse && DeviceInterface)
    {
        // If no handle provided, open one from device interface
        EtOpenAdapterFromDeviceName(&adapterHandleToUse, NULL, PhGetString(DeviceInterface));
    }
    adapter->CachedAdapterHandle = adapterHandleToUse;

    if (EtGpuSupported)
    {
        adapter->NodeNameList = PhCreateList(adapter->NodeCount);

        for (ULONG i = 0; i < adapter->NodeCount; i++)
        {
            D3DKMT_NODEMETADATA metaDataInfo;

            memset(&metaDataInfo, 0, sizeof(D3DKMT_NODEMETADATA));
            metaDataInfo.NodeOrdinalAndAdapterIndex = MAKEWORD(i, 0);

            if (adapterHandleToUse && NT_SUCCESS(EtQueryAdapterInformation(
                adapterHandleToUse,
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

    PhAddItemList(EtpGpuAdapterList, adapter);

    return adapter;
}

BOOLEAN EtpGpuInitializeD3DStatistics(
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

        if (!entry->Attributes.TypeGpu && !entry->Attributes.TypeComputeAccelerator)
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

        if (EtGpuSupported && processedCount > 0)
        {
            if (EtIsSoftwareDevice(adapterHandle))
            {
                if (entry->AdapterHandle == 0)
                    EtCloseAdapterHandle(adapterHandle);
                continue;
            }
        }

        if (EtGpuSupported)
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
                EtGpuDedicatedLimit += segmentInfo.DedicatedVideoMemorySize;
                EtGpuSharedLimit += segmentInfo.SharedSystemMemorySize;
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
                EtGpuTemperatureLimit += max(perfCaps.TemperatureWarning, perfCaps.TemperatureMax);
                EtGpuFanRpmLimit += perfCaps.MaxFanRPM;
            }
        }

        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
        queryStatistics.AdapterLuid = adapterLuid;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            PETP_GPU_ADAPTER gpuAdapter;

            gpuAdapter = EtpAddGpuAdapter(
                entry->DeviceInterface,
                adapterHandle,
                adapterLuid,
                queryStatistics.QueryResult.AdapterInformation.NbSegments,
                queryStatistics.QueryResult.AdapterInformation.NodeCount
                );

            gpuAdapter->FirstNodeIndex = EtGpuNextNodeIndex;
            EtGpuTotalNodeCount += gpuAdapter->NodeCount;
            EtGpuTotalSegmentCount += gpuAdapter->SegmentCount;
            EtGpuNextNodeIndex += gpuAdapter->NodeCount;

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

                    if (!EtGpuSupported || !EtGpuD3DEnabled)
                    {
                        if (aperture)
                            EtGpuSharedLimit += commitLimit;
                        else
                            EtGpuDedicatedLimit += commitLimit;
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

    if (EtGpuSupported && processedCount > 0)
    {
        //
        // Use the average as the limit since we show one graph for all.
        //
        EtGpuTemperatureLimit /= processedCount;
        EtGpuFanRpmLimit /= processedCount;

        // Set limit at 100C (dmex)
        if (EtGpuTemperatureLimit == 0)
            EtGpuTemperatureLimit = 100;
    }

    EtUninitializeGraphicsAdapters(discoveredAdapterList);

    if (EtGpuTotalNodeCount == 0)
        return FALSE;

    return TRUE;
}

PETP_GPU_ADAPTER EtpAllocateGpuAdapter(
    _In_ ULONG NumberOfSegments
    )
{
    PETP_GPU_ADAPTER adapter;
    SIZE_T sizeNeeded;

    sizeNeeded = FIELD_OFFSET(ETP_GPU_ADAPTER, ApertureBitMapBuffer);
    sizeNeeded += BYTES_NEEDED_FOR_BITS(NumberOfSegments);

    adapter = PhAllocate(sizeNeeded);
    memset(adapter, 0, sizeNeeded);

    return adapter;
}

VOID EtpGpuUpdateProcessSegmentInformation(
    _In_ PET_PROCESS_BLOCK Block
    )
{
    ULONG i;
    ULONG j;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 dedicatedUsage;
    ULONG64 sharedUsage;
    ULONG64 commitUsage;

    if (!Block->ProcessItem->QueryHandle)
        return;

    dedicatedUsage = 0;
    sharedUsage = 0;
    commitUsage = 0;

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

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

    Block->GpuDedicatedUsage = dedicatedUsage;
    Block->GpuSharedUsage = sharedUsage;
    Block->GpuCommitUsage = commitUsage;
}

VOID EtpGpuUpdateSystemSegmentInformation(
    VOID
    )
{
    ULONG i;
    ULONG j;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 dedicatedUsage;
    ULONG64 sharedUsage;

    dedicatedUsage = 0;
    sharedUsage = 0;

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

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

    EtGpuDedicatedUsage = dedicatedUsage;
    EtGpuSharedUsage = sharedUsage;
}

VOID EtpGpuUpdateProcessNodeInformation(
    _In_ PET_PROCESS_BLOCK Block
    )
{
    ULONG i;
    ULONG j;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 totalRunningTime;

    if (!Block->ProcessItem->QueryHandle)
        return;

    totalRunningTime = 0;

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

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
                //PhUpdateDelta(&Block->GpuTotalRunningTimeDelta[j], runningTime);

                totalRunningTime += queryStatistics.QueryResult.ProcessNodeInformation.RunningTime.QuadPart;
                //totalContextSwitches += queryStatistics.QueryResult.ProcessNodeInformation.ContextSwitch;
            }
        }
    }

    PhUpdateDelta(&Block->GpuRunningTimeDelta, totalRunningTime);
}

VOID EtpGpuUpdateSystemNodeInformation(
    VOID
    )
{
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 totalSystemDelta = 0;

    for (ULONG i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

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

                PhUpdateDelta(&EtGpuNodesTotalRunningTimeDelta[nodeIndex], runningTime);
                PhUpdateDelta(&EtGpuNodesSystemRunningTimeDelta[nodeIndex], systemRunningTime);

                totalSystemDelta += EtGpuNodesSystemRunningTimeDelta[nodeIndex].Delta;
            }
        }
    }

    EtGpuSystemRunningTimeDelta = totalSystemDelta;
}

PPH_LIST EtpBuildGpuProcessList(
    _Out_ PBOOLEAN FilterAvailable
    )
{
    PPH_LIST gpuProcessList;
    ULONG i;
    BOOLEAN querySucceeded;

    *FilterAvailable = FALSE;

    if (!EtpGpuAdapterList->Count)
        return NULL;

    gpuProcessList = PhCreateList(128);
    querySucceeded = FALSE;

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        PETP_GPU_ADAPTER gpuAdapter = EtpGpuAdapterList->Items[i];
        PULONG processIds = NULL;
        PHANDLE processHandles = NULL;
        SIZE_T processCount = 0;
        NTSTATUS status;

        if (!gpuAdapter->CachedAdapterHandle)
            continue;

        if (EtGpuD3DEnumProcesses)
        {
            status = EtAdapterEnumProcessList(
                gpuAdapter->AdapterLuid,
                gpuAdapter->ProcessIdCount,
                &processIds,
                &processCount
                );

            if (!NT_SUCCESS(status))
            {
                if (status == STATUS_PROCEDURE_NOT_FOUND)
                {
                    EtGpuD3DEnumProcesses = FALSE;
                }
            }
            else
            {
                querySucceeded = TRUE;
                gpuAdapter->ProcessIdCount = processCount;
            }
        }

        if (!EtGpuD3DEnumProcesses)
        {
            status = EtAdapterGetProcessList(
                gpuAdapter->AdapterLuid,
                gpuAdapter->ProcessHandleCount,
                &processHandles,
                &processCount
                );

            if (!NT_SUCCESS(status))
                continue;

            querySucceeded = TRUE;
            gpuAdapter->ProcessHandleCount = processCount;
        }

        for (SIZE_T j = 0; j < processCount; j++)
        {
            HANDLE pid;
            BOOLEAN found = FALSE;

            if (processIds)
            {
                pid = UlongToHandle(processIds[j]);
            }
            else
            {
                PROCESS_BASIC_INFORMATION basicInfo;

                if (!NT_SUCCESS(PhGetProcessBasicInformation(processHandles[j], &basicInfo)))
                    continue;

                pid = basicInfo.UniqueProcessId;
            }

            for (SIZE_T k = 0; k < gpuProcessList->Count; k++)
            {
                if (gpuProcessList->Items[k] == pid)
                {
                    found = TRUE;
                    break;
                }
            }

            if (!found)
                PhAddItemList(gpuProcessList, pid);
        }

        if (processIds)
            PhFree(processIds);

        if (processHandles)
        {
            for (SIZE_T j = 0; j < processCount; j++)
                NtClose(processHandles[j]);

            PhFree(processHandles);
        }
    }

    if (!querySucceeded)
    {
        PhDereferenceObject(gpuProcessList);
        return NULL;
    }

    *FilterAvailable = TRUE;

    return gpuProcessList;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI EtGpuProcessesUpdatedCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PROVIDER_UPDATED_EVENT updateEvent = Parameter;
    ULONG runCount;
    FLOAT elapsedTime = 0; // total GPU node elapsed time in micro-seconds
    FLOAT tempGpuUsage = 0;
    ULONG i;
    PLIST_ENTRY listEntry;
    FLOAT maxNodeValue = 0;
    PET_PROCESS_BLOCK maxNodeBlock = NULL;

    if (!updateEvent)
        return;

    runCount = updateEvent->RunCount;

    if (EtGpuD3DEnabled)
    {
        FLOAT gpuTotal;
        ULONG64 dedicatedTotal;
        ULONG64 sharedTotal;

        gpuTotal = EtLookupTotalGpuUtilization();
        dedicatedTotal = EtLookupTotalGpuDedicated();
        sharedTotal = EtLookupTotalGpuShared();

        if (gpuTotal > 1)
            gpuTotal = 1;
        if (gpuTotal > tempGpuUsage)
            tempGpuUsage = gpuTotal;

        EtGpuNodeUsage = tempGpuUsage;
        EtGpuDedicatedUsage = dedicatedTotal;
        EtGpuSharedUsage = sharedTotal;
    }
    else
    {
        EtpGpuUpdateSystemSegmentInformation();
        EtpGpuUpdateSystemNodeInformation();

        elapsedTime = (FLOAT)EtGpuSystemRunningTimeDelta;

        if (elapsedTime != 0)
        {
            for (i = 0; i < EtGpuTotalNodeCount; i++)
            {
                ULONG64 nodeElapsedTime;
                FLOAT usage;

                nodeElapsedTime = EtGpuNodesSystemRunningTimeDelta[i].Delta;

                if (nodeElapsedTime == 0)
                    continue;

                usage = (FLOAT)EtGpuNodesTotalRunningTimeDelta[i].Delta / (FLOAT)nodeElapsedTime;

                if (usage > 1)
                    usage = 1;
                if (usage > tempGpuUsage)
                    tempGpuUsage = usage;
            }
        }

        EtGpuNodeUsage = tempGpuUsage;
    }

    if (EtGpuSupported && EtpGpuAdapterList->Count)
    {
        FLOAT powerUsage;
        FLOAT temperature;
        ULONG64 fanRpm;

        powerUsage = 0.0f;
        temperature = 0.0f;
        fanRpm = 0;

        for (i = 0; i < EtpGpuAdapterList->Count; i++)
        {
            PETP_GPU_ADAPTER gpuAdapter;
            D3DKMT_ADAPTER_PERFDATA adapterPerfData;

            gpuAdapter = EtpGpuAdapterList->Items[i];

            // Use cached adapter handle instead of opening/closing on every update
            D3DKMT_HANDLE adapterHandle = gpuAdapter->CachedAdapterHandle;

            // If cached handle is invalid (0), try to re-open it
            if (!adapterHandle && gpuAdapter->DeviceInterface)
            {
                if (NT_SUCCESS(EtOpenAdapterFromDeviceName(&adapterHandle, NULL, PhGetString(gpuAdapter->DeviceInterface))))
                {
                    gpuAdapter->CachedAdapterHandle = adapterHandle;
                }
                else
                {
                    continue;
                }
            }

            if (!adapterHandle)
                continue;

            memset(&adapterPerfData, 0, sizeof(D3DKMT_ADAPTER_PERFDATA));

            if (NT_SUCCESS(EtQueryAdapterInformation(
                adapterHandle,
                KMTQAITYPE_ADAPTERPERFDATA,
                &adapterPerfData,
                sizeof(D3DKMT_ADAPTER_PERFDATA)
                )))
            {
                //powerUsage += (((FLOAT)adapterPerfData.Power / 1000) * 100);
                //temperature += (((FLOAT)adapterPerfData.Temperature / 1000) * 100);

                // Value * 100 / 1000 is equivalent to dividing by 10. (dmex)
                powerUsage += (FLOAT)adapterPerfData.Power / 10;
                temperature += (FLOAT)adapterPerfData.Temperature / 10;

                fanRpm += adapterPerfData.FanRPM;
            }
            else
            {
                // If query fails, invalidate the cached handle so we try to reopen next time
                gpuAdapter->CachedAdapterHandle = 0;
            }
        }

        EtGpuPowerUsage = powerUsage / EtpGpuAdapterList->Count;
        EtGpuTemperature = temperature / EtpGpuAdapterList->Count;
        EtGpuFanRpm = fanRpm / EtpGpuAdapterList->Count;

        //
        // Update the limits if we see higher values
        //

        if (EtGpuPowerUsage > EtGpuPowerUsageLimit)
        {
            //
            // Possibly over-clocked power limit
            //
            EtGpuPowerUsageLimit = EtGpuPowerUsage;
        }

        if (EtGpuTemperature > EtGpuTemperatureLimit)
        {
            //
            // Damn that card is hawt
            //
            EtGpuTemperatureLimit = EtGpuTemperature;
        }

        if (EtGpuFanRpm > EtGpuFanRpmLimit)
        {
            //
            // Fan go brrrrrr
            //
            EtGpuFanRpmLimit = EtGpuFanRpm;
        }
    }

    // Update per-process statistics.
    // Note: no lock is needed because we only ever modify the list on this same thread.

    PPH_LIST gpuProcessList = NULL;
    BOOLEAN gpuProcessFilterAvailable = FALSE;

    if (EtGpuEnabled)
    {
        gpuProcessList = EtpBuildGpuProcessList(&gpuProcessFilterAvailable);
    }

    listEntry = EtProcessBlockListHead.Flink;

    while (listEntry != &EtProcessBlockListHead)
    {
        PET_PROCESS_BLOCK block;
        BOOLEAN found = TRUE;

        block = CONTAINING_RECORD(listEntry, ET_PROCESS_BLOCK, ListEntry);

        if (FlagOn(block->ProcessItem->State, PH_PROCESS_ITEM_REMOVED))
        {
            listEntry = listEntry->Flink;
            continue;
        }

        if (gpuProcessFilterAvailable)
        {
            found = FALSE;

            if (gpuProcessList)
            {
                for (SIZE_T j = 0; j < gpuProcessList->Count; j++)
                {
                    if (gpuProcessList->Items[j] == block->ProcessItem->ProcessId)
                    {
                        found = TRUE;
                        break;
                    }
                }
            }
        }

        if (EtGpuD3DEnabled)
        {
            ULONG64 sharedUsage;
            ULONG64 dedicatedUsage;
            ULONG64 commitUsage;

            if (found)
            {
                block->GpuNodeUtilization = EtLookupProcessGpuUtilization(block->ProcessItem->ProcessId);

                if (EtLookupProcessGpuMemoryCounters(
                    block->ProcessItem->ProcessId,
                    &sharedUsage,
                    &dedicatedUsage,
                    &commitUsage
                    ))
                {
                    block->GpuSharedUsage = sharedUsage;
                    block->GpuDedicatedUsage = dedicatedUsage;
                    block->GpuCommitUsage = commitUsage;
                }
                else
                {
                    block->GpuSharedUsage = 0;
                    block->GpuDedicatedUsage = 0;
                    block->GpuCommitUsage = 0;
                }
            }
            else
            {
                // Process doesn't use GPU, zero out stats
                block->GpuNodeUtilization = 0;
                block->GpuSharedUsage = 0;
                block->GpuDedicatedUsage = 0;
                block->GpuCommitUsage = 0;
            }

            if (runCount != 0)
            {
                block->GpuCurrentUsage = block->GpuNodeUtilization;
                block->GpuCurrentMemUsage = (ULONG)(block->GpuDedicatedUsage / PAGE_SIZE);
                block->GpuCurrentMemSharedUsage = (ULONG)(block->GpuSharedUsage / PAGE_SIZE);
                block->GpuCurrentCommitUsage = (ULONG)(block->GpuCommitUsage / PAGE_SIZE);

                ET_CIRCULAR_BUFFER_ADD_FLOAT(&block->GpuHistory, block->GpuCurrentUsage);
                ET_CIRCULAR_BUFFER_ADD_ULONG(&block->GpuMemoryHistory, block->GpuCurrentMemUsage);
                ET_CIRCULAR_BUFFER_ADD_ULONG(&block->GpuMemorySharedHistory, block->GpuCurrentMemSharedUsage);
                ET_CIRCULAR_BUFFER_ADD_ULONG(&block->GpuCommittedHistory, block->GpuCurrentCommitUsage);
            }
        }
        else
        {
            // Only query GPU stats if process uses GPU or if we don't have a GPU process list
            if (found)
            {
                EtpGpuUpdateProcessSegmentInformation(block);
                EtpGpuUpdateProcessNodeInformation(block);
            }
            else
            {
                // Process doesn't use GPU, zero out stats
                block->GpuDedicatedUsage = 0;
                block->GpuSharedUsage = 0;
                block->GpuCommitUsage = 0;
                block->GpuNodeUtilization = 0;
            }

            if (elapsedTime != 0)
            {
                if (found)
                {
                    block->GpuNodeUtilization = (FLOAT)block->GpuRunningTimeDelta.Delta / elapsedTime;

                    //for (i = 0; i < EtGpuTotalNodeCount; i++)
                    //{
                    //    FLOAT usage = (FLOAT)(block->GpuTotalRunningTimeDelta[i].Delta / elapsedTime);
                    //
                    //    if (usage > block->GpuNodeUtilization)
                    //    {
                    //        block->GpuNodeUtilization = usage;
                    //    }
                    //}

                    if (block->GpuNodeUtilization > 1)
                        block->GpuNodeUtilization = 1;
                }

                if (runCount != 0)
                {
                    block->GpuCurrentUsage = block->GpuNodeUtilization;
                    block->GpuCurrentMemUsage = (ULONG)(block->GpuDedicatedUsage / PAGE_SIZE);
                    block->GpuCurrentMemSharedUsage = (ULONG)(block->GpuSharedUsage / PAGE_SIZE);
                    block->GpuCurrentCommitUsage = (ULONG)(block->GpuCommitUsage / PAGE_SIZE);

                    ET_CIRCULAR_BUFFER_ADD_FLOAT(&block->GpuHistory, block->GpuCurrentUsage);
                    ET_CIRCULAR_BUFFER_ADD_ULONG(&block->GpuMemoryHistory, block->GpuCurrentMemUsage);
                    ET_CIRCULAR_BUFFER_ADD_ULONG(&block->GpuMemorySharedHistory, block->GpuCurrentMemSharedUsage);
                    ET_CIRCULAR_BUFFER_ADD_ULONG(&block->GpuCommittedHistory, block->GpuCurrentCommitUsage);
                }
            }
        }

        if (runCount != 0)
        {
            ET_UPDATE_PROCESS_STATISTICS_MINMAX(block->GpuDedicatedUsageMin, block->GpuDedicatedUsageMax, block->GpuDedicatedUsageDiff, block->GpuDedicatedUsage);
            ET_UPDATE_PROCESS_STATISTICS_MINMAX(block->GpuSharedUsageMin, block->GpuSharedUsageMax, block->GpuSharedUsageDiff, block->GpuSharedUsage);
            ET_UPDATE_PROCESS_STATISTICS_MINMAX(block->GpuCommitUsageMin, block->GpuCommitUsageMax, block->GpuCommitUsageDiff, block->GpuCommitUsage);
            ET_UPDATE_PROCESS_STATISTICS_MINMAX(block->GpuTotalUsageMin, block->GpuTotalUsageMax, block->GpuTotalUsageDiff, block->GpuDedicatedUsage + block->GpuSharedUsage + block->GpuCommitUsage);
        }

        if (maxNodeValue < block->GpuNodeUtilization)
        {
            maxNodeValue = block->GpuNodeUtilization;
            maxNodeBlock = block;
        }

        listEntry = listEntry->Flink;
    }

    // Clean up GPU process list if we built one
    if (gpuProcessList)
    {
        PhDereferenceObject(gpuProcessList);
    }

    // Update history buffers.

    if (runCount != 0)
    {
        ET_CIRCULAR_BUFFER_ADD_FLOAT(&EtGpuNodeHistory, EtGpuNodeUsage);
        ET_CIRCULAR_BUFFER_ADD_ULONG64(&EtGpuDedicatedHistory, EtGpuDedicatedUsage);
        ET_CIRCULAR_BUFFER_ADD_ULONG64(&EtGpuSharedHistory, EtGpuSharedUsage);

        if (EtGpuSupported)
        {
            ET_CIRCULAR_BUFFER_ADD_FLOAT(&EtGpuPowerUsageHistory, EtGpuPowerUsage);
            ET_CIRCULAR_BUFFER_ADD_FLOAT(&EtGpuTemperatureHistory, EtGpuTemperature);
            ET_CIRCULAR_BUFFER_ADD_ULONG64(&EtGpuFanRpmHistory, EtGpuFanRpm);
        }

        if (EtGpuD3DEnabled)
        {
            for (i = 0; i < EtGpuTotalNodeCount; i++)
            {
                FLOAT usage;

                usage = EtLookupTotalGpuEngineUtilization(i);

                if (usage > 1)
                    usage = 1;

                ET_CIRCULAR_BUFFER_ADD_FLOAT(&EtGpuNodesHistory[i], usage);
            }
        }
        else
        {
            if (elapsedTime != 0)
            {
                for (i = 0; i < EtGpuTotalNodeCount; i++)
                {
                    FLOAT usage;

                    if (EtGpuNodesSystemRunningTimeDelta[i].Delta != 0)
                        usage = (FLOAT)EtGpuNodesTotalRunningTimeDelta[i].Delta / (FLOAT)EtGpuNodesSystemRunningTimeDelta[i].Delta;
                    else
                        usage = 0;

                    if (usage > 1)
                        usage = 1;

                    ET_CIRCULAR_BUFFER_ADD_FLOAT(&EtGpuNodesHistory[i], usage);
                }
            }
            else
            {
                for (i = 0; i < EtGpuTotalNodeCount; i++)
                {
                    ET_CIRCULAR_BUFFER_ADD_FLOAT(&EtGpuNodesHistory[i], 0);
                }
            }
        }

        if (maxNodeBlock)
        {
            ET_CIRCULAR_BUFFER_ADD_ULONG(&EtMaxGpuNodeHistory, HandleToUlong(maxNodeBlock->ProcessItem->ProcessId));
            ET_CIRCULAR_BUFFER_ADD_FLOAT(&EtMaxGpuNodeUsageHistory, maxNodeBlock->GpuNodeUtilization);
            PhReferenceProcessRecordForStatistics(maxNodeBlock->ProcessItem->Record);
        }
        else
        {
            ET_CIRCULAR_BUFFER_ADD_ULONG(&EtMaxGpuNodeHistory, 0);
            ET_CIRCULAR_BUFFER_ADD_FLOAT(&EtMaxGpuNodeUsageHistory, 0);
        }
    }
}

ULONG EtGetGpuAdapterCount(
    VOID
    )
{
    return EtpGpuAdapterList->Count;
}

ULONG EtGetGpuAdapterIndexFromNodeIndex(
    _In_ ULONG NodeIndex
    )
{
    ULONG i;
    PETP_GPU_ADAPTER gpuAdapter;

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

        if (NodeIndex >= gpuAdapter->FirstNodeIndex && NodeIndex < gpuAdapter->FirstNodeIndex + gpuAdapter->NodeCount)
            return i;
    }

    return ULONG_MAX;
}

PPH_STRING EtGetGpuAdapterNodeDescription(
    _In_ ULONG Index,
    _In_ ULONG NodeIndex
    )
{
    PETP_GPU_ADAPTER gpuAdapter;

    if (Index >= EtpGpuAdapterList->Count)
        return NULL;

    gpuAdapter = EtpGpuAdapterList->Items[Index];

    if (!gpuAdapter->NodeNameList)
        return NULL;

    return gpuAdapter->NodeNameList->Items[NodeIndex - gpuAdapter->FirstNodeIndex];
}

PPH_STRING EtGetGpuAdapterDescription(
    _In_ ULONG Index
    )
{
    PPH_STRING description;

    if (Index >= EtpGpuAdapterList->Count)
        return NULL;

    description = ((PETP_GPU_ADAPTER)EtpGpuAdapterList->Items[Index])->Description;

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

VOID EtQueryProcessGpuStatistics(
    _In_ HANDLE ProcessHandle,
    _Out_ PET_PROCESS_GPU_STATISTICS Statistics
    )
{
    ULONG i;
    ULONG j;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;

    memset(Statistics, 0, sizeof(ET_PROCESS_GPU_STATISTICS));

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

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

D3DKMT_CLIENTHINT EtQueryProcessGpuClientHint(
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
