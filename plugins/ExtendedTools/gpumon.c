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

static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;

BOOLEAN EtGpuEnabled = FALSE;
BOOLEAN EtGpuSupported = FALSE;
BOOLEAN EtGpuD3DEnabled = FALSE;
PPH_LIST EtpGpuAdapterList;

ULONG EtGpuTotalNodeCount = 0;
ULONG EtGpuTotalSegmentCount = 0;
ULONG EtGpuNextNodeIndex = 0;

PH_UINT64_DELTA EtGpuClockTotalRunningTimeDelta = { 0, 0 };
LARGE_INTEGER EtGpuClockTotalRunningTimeFrequency = { 0 };

FLOAT EtGpuNodeUsage = 0;
PH_CIRCULAR_BUFFER_FLOAT EtGpuNodeHistory;
PH_CIRCULAR_BUFFER_ULONG EtMaxGpuNodeHistory; // ID of max. GPU usage process
PH_CIRCULAR_BUFFER_FLOAT EtMaxGpuNodeUsageHistory;

PPH_UINT64_DELTA EtGpuNodesTotalRunningTimeDelta;
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

VOID EtGpuMonitorInitialization(
    VOID
    )
{
    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_GPU_MONITOR))
    {
        EtGpuSupported = EtWindowsVersion >= WINDOWS_10_RS4; // Note: Changed to RS4 due to reports of BSODs on LTSB versions of RS3 (dmex)
        EtGpuD3DEnabled = EtGpuSupported && !!PhGetIntegerSetting(SETTING_NAME_ENABLE_GPUPERFCOUNTERS);

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
        ULONG sampleCount;
        ULONG i;

        sampleCount = PhGetIntegerSetting(L"SampleCount");
        PhInitializeCircularBuffer_FLOAT(&EtGpuNodeHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtMaxGpuNodeHistory, sampleCount);
        PhInitializeCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG64(&EtGpuDedicatedHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG64(&EtGpuSharedHistory, sampleCount);
        if (EtGpuSupported)
        {
            PhInitializeCircularBuffer_FLOAT(&EtGpuPowerUsageHistory, sampleCount);
            PhInitializeCircularBuffer_FLOAT(&EtGpuTemperatureHistory, sampleCount);
            PhInitializeCircularBuffer_ULONG64(&EtGpuFanRpmHistory, sampleCount);
        }

        if (!EtGpuD3DEnabled)
            EtGpuNodesTotalRunningTimeDelta = PhAllocateZero(sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
        EtGpuNodesHistory = PhAllocateZero(sizeof(PH_CIRCULAR_BUFFER_FLOAT) * EtGpuTotalNodeCount);

        for (i = 0; i < EtGpuTotalNodeCount; i++)
        {
            PhInitializeCircularBuffer_FLOAT(&EtGpuNodesHistory[i], sampleCount);
        }

        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
            EtGpuProcessesUpdatedCallback,
            NULL,
            &ProcessesUpdatedCallbackRegistration
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

    adapter = EtpAllocateGpuAdapter(NumberOfSegments);
    adapter->DeviceInterface = PhReferenceObject(DeviceInterface);
    adapter->AdapterLuid = AdapterLuid;
    adapter->NodeCount = NumberOfNodes;
    adapter->SegmentCount = NumberOfSegments;
    RtlInitializeBitMap(&adapter->ApertureBitMap, adapter->ApertureBitMapBuffer, NumberOfSegments);

    {
        PPH_STRING description;

        if (EtQueryDeviceProperties(DeviceInterface, &description, NULL, NULL, NULL, NULL))
        {
            adapter->Description = description;
        }
    }

    if (EtGpuSupported)
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
                PhAddItemList(adapter->NodeNameList, EtGetNodeEngineTypeString(metaDataInfo));
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
    PPH_LIST deviceAdapterList;
    PWSTR deviceInterfaceList;
    ULONG deviceInterfaceListLength = 0;
    PWSTR deviceInterface;
    D3DKMT_OPENADAPTERFROMDEVICENAME openAdapterFromDeviceName;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    D3DKMT_ADAPTER_PERFDATACAPS perfCaps;

    if (CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        (PGUID)&GUID_DISPLAY_DEVICE_ARRIVAL,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    deviceInterfaceList = PhAllocate(deviceInterfaceListLength * sizeof(WCHAR));
    memset(deviceInterfaceList, 0, deviceInterfaceListLength * sizeof(WCHAR));

    if (CM_Get_Device_Interface_List(
        (PGUID)&GUID_DISPLAY_DEVICE_ARRIVAL,
        NULL,
        deviceInterfaceList,
        deviceInterfaceListLength,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT
        ) != CR_SUCCESS)
    {
        PhFree(deviceInterfaceList);
        return FALSE;
    }

    deviceAdapterList = PhCreateList(10);
    deviceInterface = deviceInterfaceList;

    while (TRUE)
    {
        PH_STRINGREF string;

        PhInitializeStringRefLongHint(&string, deviceInterface);

        if (string.Length == 0)
            break;

        PhAddItemList(deviceAdapterList, PhCreateString2(&string));

        deviceInterface = PTR_ADD_OFFSET(deviceInterface, string.Length + sizeof(UNICODE_NULL));
    }

    for (ULONG i = 0; i < deviceAdapterList->Count; i++)
    {
        ET_ADAPTER_ATTRIBUTES adapterAttributes;

        memset(&openAdapterFromDeviceName, 0, sizeof(D3DKMT_OPENADAPTERFROMDEVICENAME));
        openAdapterFromDeviceName.pDeviceName = PhGetString(deviceAdapterList->Items[i]);

        if (!NT_SUCCESS(D3DKMTOpenAdapterFromDeviceName(&openAdapterFromDeviceName)))
            continue;

        if (NT_SUCCESS(EtQueryAdapterAttributes(
            openAdapterFromDeviceName.hAdapter,
            &adapterAttributes
            )))
        {
            if (!adapterAttributes.TypeGpu)
            {
                EtCloseAdapterHandle(openAdapterFromDeviceName.hAdapter);
                continue;
            }
        }

        if (EtGpuSupported && deviceAdapterList->Count > 1)
        {
            if (EtIsSoftwareDevice(openAdapterFromDeviceName.hAdapter))
            {
                EtCloseAdapterHandle(openAdapterFromDeviceName.hAdapter);
                continue;
            }
        }

        if (EtGpuSupported)
        {
            D3DKMT_SEGMENTSIZEINFO segmentInfo;

            memset(&segmentInfo, 0, sizeof(D3DKMT_SEGMENTSIZEINFO));

            if (NT_SUCCESS(EtQueryAdapterInformation(
                openAdapterFromDeviceName.hAdapter,
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
                openAdapterFromDeviceName.hAdapter,
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
        queryStatistics.AdapterLuid = openAdapterFromDeviceName.AdapterLuid;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            PETP_GPU_ADAPTER gpuAdapter;

            gpuAdapter = EtpAddGpuAdapter(
                deviceAdapterList->Items[i],
                openAdapterFromDeviceName.hAdapter,
                openAdapterFromDeviceName.AdapterLuid,
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
                        PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1 segmentInfo;

                        segmentInfo = (PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1)&queryStatistics.QueryResult;
                        commitLimit = segmentInfo->CommitLimit;
                        aperture = segmentInfo->Aperture;
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

        EtCloseAdapterHandle(openAdapterFromDeviceName.hAdapter);
    }

    if (EtGpuSupported && deviceAdapterList->Count > 0)
    {
        //
        // Use the average as the limit since we show one graph for all.
        //
        EtGpuTemperatureLimit /= deviceAdapterList->Count;
        EtGpuFanRpmLimit /= deviceAdapterList->Count;

        // Set limit at 100C (dmex)
        if (EtGpuTemperatureLimit == 0)
            EtGpuTemperatureLimit = 100;
    }

    PhDereferenceObjects(deviceAdapterList->Items, deviceAdapterList->Count);
    PhDereferenceObject(deviceAdapterList);
    PhFree(deviceInterfaceList);

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
    LARGE_INTEGER performanceCounter;

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
                ULONG64 runningTime;
                //ULONG64 systemRunningTime;

                runningTime = queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
                //systemRunningTime = queryStatistics.QueryResult.NodeInformation.SystemInformation.RunningTime.QuadPart;

                PhUpdateDelta(&EtGpuNodesTotalRunningTimeDelta[gpuAdapter->FirstNodeIndex + j], runningTime);
            }
        }
    }

    PhQueryPerformanceCounter(&performanceCounter);
    PhUpdateDelta(&EtGpuClockTotalRunningTimeDelta, performanceCounter.QuadPart);
}

VOID NTAPI EtGpuProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    static ULONG runCount = 0; // MUST keep in sync with runCount in process provider
    DOUBLE elapsedTime = 0; // total GPU node elapsed time in micro-seconds
    FLOAT tempGpuUsage = 0;
    ULONG i;
    PLIST_ENTRY listEntry;
    FLOAT maxNodeValue = 0;
    PET_PROCESS_BLOCK maxNodeBlock = NULL;

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

        elapsedTime = (DOUBLE)EtGpuClockTotalRunningTimeDelta.Delta * 10000000 / EtGpuClockTotalRunningTimeFrequency.QuadPart;

        if (elapsedTime != 0)
        {
            for (i = 0; i < EtGpuTotalNodeCount; i++)
            {
                FLOAT usage = (FLOAT)(EtGpuNodesTotalRunningTimeDelta[i].Delta / elapsedTime);

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
            D3DKMT_HANDLE adapterHandle;
            D3DKMT_ADAPTER_PERFDATA adapterPerfData;

            gpuAdapter = EtpGpuAdapterList->Items[i];

            //
            // jxy-s: we open this frequently, consider opening this once in the list
            //
            if (!NT_SUCCESS(EtOpenAdapterFromDeviceName(&adapterHandle, PhGetString(gpuAdapter->DeviceInterface))))
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

        if (EtGpuD3DEnabled)
        {
            ULONG64 sharedUsage;
            ULONG64 dedicatedUsage;
            ULONG64 commitUsage;

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

            if (runCount != 0)
            {
                block->GpuCurrentUsage = block->GpuNodeUtilization;
                block->GpuCurrentMemUsage = (ULONG)(block->GpuDedicatedUsage / PAGE_SIZE);
                block->GpuCurrentMemSharedUsage = (ULONG)(block->GpuSharedUsage / PAGE_SIZE);
                block->GpuCurrentCommitUsage = (ULONG)(block->GpuCommitUsage / PAGE_SIZE);

                PhAddItemCircularBuffer_FLOAT(&block->GpuHistory, block->GpuCurrentUsage);
                PhAddItemCircularBuffer_ULONG(&block->GpuMemoryHistory, block->GpuCurrentMemUsage);
                PhAddItemCircularBuffer_ULONG(&block->GpuMemorySharedHistory, block->GpuCurrentMemSharedUsage);
                PhAddItemCircularBuffer_ULONG(&block->GpuCommittedHistory, block->GpuCurrentCommitUsage);
            }
        }
        else
        {
            EtpGpuUpdateProcessSegmentInformation(block);
            EtpGpuUpdateProcessNodeInformation(block);

            if (elapsedTime != 0)
            {
                block->GpuNodeUtilization = (FLOAT)(block->GpuRunningTimeDelta.Delta / elapsedTime);

                // HACK
                if (block->GpuNodeUtilization > EtGpuNodeUsage)
                    block->GpuNodeUtilization = EtGpuNodeUsage;

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

                if (runCount != 0)
                {
                    block->GpuCurrentUsage = block->GpuNodeUtilization;
                    block->GpuCurrentMemUsage = (ULONG)(block->GpuDedicatedUsage / PAGE_SIZE);
                    block->GpuCurrentMemSharedUsage = (ULONG)(block->GpuSharedUsage / PAGE_SIZE);
                    block->GpuCurrentCommitUsage = (ULONG)(block->GpuCommitUsage / PAGE_SIZE);

                    PhAddItemCircularBuffer_FLOAT(&block->GpuHistory, block->GpuCurrentUsage);
                    PhAddItemCircularBuffer_ULONG(&block->GpuMemoryHistory, block->GpuCurrentMemUsage);
                    PhAddItemCircularBuffer_ULONG(&block->GpuMemorySharedHistory, block->GpuCurrentMemSharedUsage);
                    PhAddItemCircularBuffer_ULONG(&block->GpuCommittedHistory, block->GpuCurrentCommitUsage);
                }
            }
        }

        if (maxNodeValue < block->GpuNodeUtilization)
        {
            maxNodeValue = block->GpuNodeUtilization;
            maxNodeBlock = block;
        }

        listEntry = listEntry->Flink;
    }

    // Update history buffers.

    if (runCount != 0)
    {
        PhAddItemCircularBuffer_FLOAT(&EtGpuNodeHistory, EtGpuNodeUsage);
        PhAddItemCircularBuffer_ULONG64(&EtGpuDedicatedHistory, EtGpuDedicatedUsage);
        PhAddItemCircularBuffer_ULONG64(&EtGpuSharedHistory, EtGpuSharedUsage);
        if (EtGpuSupported)
        {
            PhAddItemCircularBuffer_FLOAT(&EtGpuPowerUsageHistory, EtGpuPowerUsage);
            PhAddItemCircularBuffer_FLOAT(&EtGpuTemperatureHistory, EtGpuTemperature);
            PhAddItemCircularBuffer_ULONG64(&EtGpuFanRpmHistory, EtGpuFanRpm);
        }

        if (EtGpuD3DEnabled)
        {
            for (i = 0; i < EtGpuTotalNodeCount; i++)
            {
                FLOAT usage;

                usage = EtLookupTotalGpuEngineUtilization(i);

                if (usage > 1)
                    usage = 1;

                PhAddItemCircularBuffer_FLOAT(&EtGpuNodesHistory[i], usage);
            }
        }
        else
        {
            if (elapsedTime != 0)
            {
                for (i = 0; i < EtGpuTotalNodeCount; i++)
                {
                    FLOAT usage;

                    usage = (FLOAT)(EtGpuNodesTotalRunningTimeDelta[i].Delta / elapsedTime);

                    if (usage > 1)
                        usage = 1;

                    PhAddItemCircularBuffer_FLOAT(&EtGpuNodesHistory[i], usage);
                }
            }
            else
            {
                for (i = 0; i < EtGpuTotalNodeCount; i++)
                    PhAddItemCircularBuffer_FLOAT(&EtGpuNodesHistory[i], 0);
            }
        }

        if (maxNodeBlock)
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxGpuNodeHistory, HandleToUlong(maxNodeBlock->ProcessItem->ProcessId));
            PhAddItemCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, maxNodeBlock->GpuNodeUtilization);
            PhReferenceProcessRecordForStatistics(maxNodeBlock->ProcessItem->Record);
        }
        else
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxGpuNodeHistory, 0);
            PhAddItemCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, 0);
        }
    }

    runCount++;
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
