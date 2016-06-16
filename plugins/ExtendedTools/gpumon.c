/*
 * Process Hacker Extended Tools -
 *   GPU monitoring
 *
 * Copyright (C) 2011-2015 wj32
 * Copyright (C) 2016 dmex
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

#define INITGUID
#include "exttools.h"
#include <cfgmgr32.h>
#include <devpkey.h>
#include <ntddvdeo.h>
#include "d3dkmt.h"
#include "gpumon.h"

BOOLEAN EtGpuEnabled;
static PPH_LIST EtpGpuAdapterList;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;

ULONG EtGpuTotalNodeCount;
ULONG EtGpuTotalSegmentCount;
ULONG64 EtGpuDedicatedLimit;
ULONG64 EtGpuSharedLimit;
ULONG EtGpuNextNodeIndex = 0;
RTL_BITMAP EtGpuNodeBitMap;
PULONG EtGpuNodeBitMapBuffer;
ULONG EtGpuNodeBitMapBitsSet;
PULONG EtGpuNewNodeBitMapBuffer;

PH_UINT64_DELTA EtClockTotalRunningTimeDelta;
LARGE_INTEGER EtClockTotalRunningTimeFrequency;
PH_UINT64_DELTA EtGpuTotalRunningTimeDelta;
PH_UINT64_DELTA EtGpuSystemRunningTimeDelta;
FLOAT EtGpuNodeUsage;
PH_CIRCULAR_BUFFER_FLOAT EtGpuNodeHistory;
PH_CIRCULAR_BUFFER_ULONG EtMaxGpuNodeHistory; // ID of max. GPU usage process
PH_CIRCULAR_BUFFER_FLOAT EtMaxGpuNodeUsageHistory;

PPH_UINT64_DELTA EtGpuNodesTotalRunningTimeDelta;
PPH_CIRCULAR_BUFFER_FLOAT EtGpuNodesHistory;

ULONG64 EtGpuDedicatedUsage;
ULONG64 EtGpuSharedUsage;
PH_CIRCULAR_BUFFER_ULONG EtGpuDedicatedHistory;
PH_CIRCULAR_BUFFER_ULONG EtGpuSharedHistory;

VOID EtGpuMonitorInitialization(
    VOID
    )
{
    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_GPU_MONITOR))
    {
        EtpGpuAdapterList = PhCreateList(4);

        if (EtpInitializeD3DStatistics())
            EtGpuEnabled = TRUE;
    }

    if (EtGpuEnabled)
    {
        ULONG sampleCount;
        ULONG i;
        ULONG j;
        PPH_STRING bitmapString;
        D3DKMT_QUERYSTATISTICS queryStatistics;

        sampleCount = PhGetIntegerSetting(L"SampleCount");
        PhInitializeCircularBuffer_FLOAT(&EtGpuNodeHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtMaxGpuNodeHistory, sampleCount);
        PhInitializeCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtGpuDedicatedHistory, sampleCount);
        PhInitializeCircularBuffer_ULONG(&EtGpuSharedHistory, sampleCount);

        EtGpuNodesTotalRunningTimeDelta = PhAllocate(sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
        memset(EtGpuNodesTotalRunningTimeDelta, 0, sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
        EtGpuNodesHistory = PhAllocate(sizeof(PH_CIRCULAR_BUFFER_FLOAT) * EtGpuTotalNodeCount);

        for (i = 0; i < EtGpuTotalNodeCount; i++)
        {
            PhInitializeCircularBuffer_FLOAT(&EtGpuNodesHistory[i], sampleCount);
        }

        PhRegisterCallback(
            &PhProcessesUpdatedEvent,
            EtGpuProcessesUpdatedCallback,
            NULL,
            &ProcessesUpdatedCallbackRegistration
            );

        // Load the node bitmap.

        bitmapString = PhGetStringSetting(SETTING_NAME_GPU_NODE_BITMAP);

        if (!(bitmapString->Length & 3) && bitmapString->Length / 4 <= BYTES_NEEDED_FOR_BITS(EtGpuTotalNodeCount))
        {
            PhHexStringToBuffer(&bitmapString->sr, (PUCHAR)EtGpuNodeBitMapBuffer);
            EtGpuNodeBitMapBitsSet = RtlNumberOfSetBits(&EtGpuNodeBitMap);
        }

        PhDereferenceObject(bitmapString);

        // Fix up the node bitmap if the current node count differs from what we've seen.
        if (EtGpuTotalNodeCount != PhGetIntegerSetting(SETTING_NAME_GPU_LAST_NODE_COUNT))
        {
            ULONG maxContextSwitch = 0;
            ULONG maxContextSwitchNodeIndex = 0;

            RtlClearAllBits(&EtGpuNodeBitMap);
            EtGpuNodeBitMapBitsSet = 0;

            for (i = 0; i < EtpGpuAdapterList->Count; i++)
            {
                PETP_GPU_ADAPTER gpuAdapter = EtpGpuAdapterList->Items[i];

                for (j = 0; j < gpuAdapter->NodeCount; j++)
                {
                    memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
                    queryStatistics.Type = D3DKMT_QUERYSTATISTICS_NODE;
                    queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
                    queryStatistics.QueryNode.NodeId = j;

                    if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
                    {
                        // The numbers below are quite arbitrary.
                        if (queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart != 0 &&
                            queryStatistics.QueryResult.NodeInformation.GlobalInformation.ContextSwitch > 10000)
                        {
                            RtlSetBits(&EtGpuNodeBitMap, gpuAdapter->FirstNodeIndex + j, 1);
                            EtGpuNodeBitMapBitsSet++;
                        }

                        if (maxContextSwitch < queryStatistics.QueryResult.NodeInformation.GlobalInformation.ContextSwitch)
                        {
                            maxContextSwitch = queryStatistics.QueryResult.NodeInformation.GlobalInformation.ContextSwitch;
                            maxContextSwitchNodeIndex = gpuAdapter->FirstNodeIndex + j;
                        }
                    }
                }
            }

            // Just in case
            if (EtGpuNodeBitMapBitsSet == 0)
            {
                RtlSetBits(&EtGpuNodeBitMap, maxContextSwitchNodeIndex, 1);
                EtGpuNodeBitMapBitsSet = 1;
            }

            PhSetIntegerSetting(SETTING_NAME_GPU_LAST_NODE_COUNT, EtGpuTotalNodeCount);
        }
    }
}

BOOLEAN EtpInitializeD3DStatistics(
    VOID
    )
{
    PWSTR deviceInterfaceList;
    ULONG deviceInterfaceListLength = 0;
    PWSTR deviceInterface;
    D3DKMT_OPENADAPTERFROMDEVICENAME openAdapterFromDeviceName;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    D3DKMT_CLOSEADAPTER closeAdapter;

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

    for (deviceInterface = deviceInterfaceList; *deviceInterface; deviceInterface += PhCountStringZ(deviceInterface) + 1)
    {
        memset(&openAdapterFromDeviceName, 0, sizeof(D3DKMT_OPENADAPTERFROMDEVICENAME));
        openAdapterFromDeviceName.pDeviceName = deviceInterface;

        if (NT_SUCCESS(D3DKMTOpenAdapterFromDeviceName(&openAdapterFromDeviceName)))
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
            queryStatistics.AdapterLuid = openAdapterFromDeviceName.AdapterLuid;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                PETP_GPU_ADAPTER gpuAdapter;
                ULONG i;

                gpuAdapter = EtpAllocateGpuAdapter(queryStatistics.QueryResult.AdapterInformation.NbSegments);
                gpuAdapter->AdapterLuid = openAdapterFromDeviceName.AdapterLuid;
                gpuAdapter->Description = EtpQueryDeviceDescription(deviceInterface);
                gpuAdapter->NodeCount = queryStatistics.QueryResult.AdapterInformation.NodeCount;
                gpuAdapter->SegmentCount = queryStatistics.QueryResult.AdapterInformation.NbSegments;
                RtlInitializeBitMap(&gpuAdapter->ApertureBitMap, gpuAdapter->ApertureBitMapBuffer, queryStatistics.QueryResult.AdapterInformation.NbSegments);

                PhAddItemList(EtpGpuAdapterList, gpuAdapter);
                EtGpuTotalNodeCount += gpuAdapter->NodeCount;
                EtGpuTotalSegmentCount += gpuAdapter->SegmentCount;

                gpuAdapter->FirstNodeIndex = EtGpuNextNodeIndex;
                EtGpuNextNodeIndex += gpuAdapter->NodeCount;

                for (i = 0; i < gpuAdapter->SegmentCount; i++)
                {
                    memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
                    queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
                    queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
                    queryStatistics.QuerySegment.SegmentId = i;

                    if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
                    {
                        ULONG64 commitLimit;
                        ULONG aperture;

                        if (WindowsVersion >= WINDOWS_8)
                        {
                            commitLimit = queryStatistics.QueryResult.SegmentInformation.CommitLimit;
                            aperture = queryStatistics.QueryResult.SegmentInformation.Aperture;
                        }
                        else
                        {
                            commitLimit = queryStatistics.QueryResult.SegmentInformationV1.CommitLimit;
                            aperture = queryStatistics.QueryResult.SegmentInformationV1.Aperture;
                        }

                        if (aperture)
                            EtGpuSharedLimit += commitLimit;
                        else
                            EtGpuDedicatedLimit += commitLimit;

                        if (aperture)
                            RtlSetBits(&gpuAdapter->ApertureBitMap, i, 1);
                    }
                }
            }

            memset(&closeAdapter, 0, sizeof(D3DKMT_CLOSEADAPTER));
            closeAdapter.hAdapter = openAdapterFromDeviceName.hAdapter;
            D3DKMTCloseAdapter(&closeAdapter);
        }
    }

    PhFree(deviceInterfaceList);

    EtGpuNodeBitMapBuffer = PhAllocate(BYTES_NEEDED_FOR_BITS(EtGpuTotalNodeCount));
    RtlInitializeBitMap(&EtGpuNodeBitMap, EtGpuNodeBitMapBuffer, EtGpuTotalNodeCount);
    RtlSetBits(&EtGpuNodeBitMap, 0, 1);
    EtGpuNodeBitMapBitsSet = 1;

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

PPH_STRING EtpQueryDeviceDescription(
    _In_ PWSTR DeviceInterface
    )
{
    CONFIGRET result;
    PPH_STRING string;
    ULONG bufferSize;
    DEVPROPTYPE devicePropertyType;
    DEVINST deviceInstanceHandle;
    ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
    WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];

    if (CM_Get_Device_Interface_Property(
        DeviceInterface,
        &DEVPKEY_Device_InstanceId,
        &devicePropertyType,
        (PBYTE)deviceInstanceId,
        &deviceInstanceIdLength,
        0
        ) != CR_SUCCESS)
    {
        return NULL;
    }

    if (CM_Locate_DevNode(
        &deviceInstanceHandle,
        deviceInstanceId,
        CM_LOCATE_DEVNODE_NORMAL
        ) != CR_SUCCESS)
    {
        return NULL;
    }

    bufferSize = 0x40;
    string = PhCreateStringEx(NULL, bufferSize);

    if ((result = CM_Get_DevNode_Property( // CM_Get_DevNode_Registry_Property with CM_DRP_DEVICEDESC??
        deviceInstanceHandle,
        &DEVPKEY_Device_DeviceDesc,
        &devicePropertyType,
        (PBYTE)string->Buffer,
        &bufferSize,
        0
        )) != CR_SUCCESS)
    {
        PhDereferenceObject(string);
        string = PhCreateStringEx(NULL, bufferSize);

        result = CM_Get_DevNode_Property(
            deviceInstanceHandle,
            &DEVPKEY_Device_DeviceDesc,
            &devicePropertyType,
            (PBYTE)string->Buffer,
            &bufferSize,
            0
            );
    }

    if (result != CR_SUCCESS)
    {
        PhDereferenceObject(string);
        return NULL;
    }

    PhTrimToNullTerminatorString(string);

    return string;
}

VOID EtpUpdateSegmentInformation(
    _In_opt_ PET_PROCESS_BLOCK Block
    )
{
    ULONG i;
    ULONG j;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 dedicatedUsage;
    ULONG64 sharedUsage;

    if (Block && !Block->ProcessItem->QueryHandle)
        return;

    dedicatedUsage = 0;
    sharedUsage = 0;

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

        for (j = 0; j < gpuAdapter->SegmentCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));

            if (Block)
                queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT;
            else
                queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;

            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;

            if (Block)
            {
                queryStatistics.hProcess = Block->ProcessItem->QueryHandle;
                queryStatistics.QueryProcessSegment.SegmentId = j;
            }
            else
            {
                queryStatistics.QuerySegment.SegmentId = j;
            }

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                if (Block)
                {
                    ULONG64 bytesCommitted;

                    if (WindowsVersion >= WINDOWS_8)
                    {
                        bytesCommitted = queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                    }
                    else
                    {
                        bytesCommitted = (ULONG)queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                    }

                    if (RtlCheckBit(&gpuAdapter->ApertureBitMap, j))
                        sharedUsage += bytesCommitted;
                    else
                        dedicatedUsage += bytesCommitted;
                }
                else
                {
                    ULONG64 bytesCommitted;

                    if (WindowsVersion >= WINDOWS_8)
                    {
                        bytesCommitted = queryStatistics.QueryResult.SegmentInformation.BytesResident;
                    }
                    else
                    {
                        bytesCommitted = queryStatistics.QueryResult.SegmentInformationV1.BytesResident;
                    }

                    if (RtlCheckBit(&gpuAdapter->ApertureBitMap, j))
                        sharedUsage += bytesCommitted;
                    else
                        dedicatedUsage += bytesCommitted;
                }
            }
        }
    }

    if (Block)
    {
        Block->GpuDedicatedUsage = dedicatedUsage;
        Block->GpuSharedUsage = sharedUsage;
    }
    else
    {
        EtGpuDedicatedUsage = dedicatedUsage;
        EtGpuSharedUsage = sharedUsage;
    }
}

VOID EtpUpdateNodeInformation(
    _In_opt_ PET_PROCESS_BLOCK Block
    )
{
    ULONG i;
    ULONG j;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 totalRunningTime;
    ULONG64 systemRunningTime;

    if (Block && !Block->ProcessItem->QueryHandle)
        return;

    totalRunningTime = 0;
    systemRunningTime = 0;

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

        for (j = 0; j < gpuAdapter->NodeCount; j++)
        {
            if (Block && !RtlCheckBit(&EtGpuNodeBitMap, gpuAdapter->FirstNodeIndex + j))
                continue;

            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));

            if (Block)
                queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_NODE;
            else
                queryStatistics.Type = D3DKMT_QUERYSTATISTICS_NODE;

            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;

            if (Block)
            {
                queryStatistics.hProcess = Block->ProcessItem->QueryHandle;
                queryStatistics.QueryProcessNode.NodeId = j;
            }
            else
            {
                queryStatistics.QueryNode.NodeId = j;
            }

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                if (Block)
                {
                    totalRunningTime += queryStatistics.QueryResult.ProcessNodeInformation.RunningTime.QuadPart;
                }
                else
                {
                    ULONG nodeIndex;

                    nodeIndex = gpuAdapter->FirstNodeIndex + j;

                    PhUpdateDelta(&EtGpuNodesTotalRunningTimeDelta[nodeIndex], queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart);

                    if (RtlCheckBit(&EtGpuNodeBitMap, gpuAdapter->FirstNodeIndex + j))
                    {
                        totalRunningTime += queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
                        systemRunningTime += queryStatistics.QueryResult.NodeInformation.SystemInformation.RunningTime.QuadPart;
                    }
                }
            }
        }
    }

    if (Block)
    {
        PhUpdateDelta(&Block->GpuRunningTimeDelta, totalRunningTime);
    }
    else
    {
        LARGE_INTEGER performanceCounter;

        NtQueryPerformanceCounter(&performanceCounter, &EtClockTotalRunningTimeFrequency);
        PhUpdateDelta(&EtClockTotalRunningTimeDelta, performanceCounter.QuadPart);
        PhUpdateDelta(&EtGpuTotalRunningTimeDelta, totalRunningTime);
        PhUpdateDelta(&EtGpuSystemRunningTimeDelta, systemRunningTime);
    }
}

VOID NTAPI EtGpuProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    static ULONG runCount = 0; // MUST keep in sync with runCount in process provider

    DOUBLE elapsedTime; // total GPU node elapsed time in micro-seconds
    ULONG i;
    PLIST_ENTRY listEntry;
    FLOAT maxNodeValue = 0;
    PET_PROCESS_BLOCK maxNodeBlock = NULL;

    // Update global statistics.

    EtpUpdateSegmentInformation(NULL);
    EtpUpdateNodeInformation(NULL);

    elapsedTime = (DOUBLE)EtClockTotalRunningTimeDelta.Delta * 10000000 / EtClockTotalRunningTimeFrequency.QuadPart;

    if (elapsedTime != 0)
        EtGpuNodeUsage = (FLOAT)(EtGpuTotalRunningTimeDelta.Delta / (elapsedTime * EtGpuNodeBitMapBitsSet));
    else
        EtGpuNodeUsage = 0;

    if (EtGpuNodeUsage > 1)
        EtGpuNodeUsage = 1;

    // Do the update of the node bitmap if needed.
    if (EtGpuNewNodeBitMapBuffer)
    {
        PULONG newBuffer;

        newBuffer = _InterlockedExchangePointer(&EtGpuNewNodeBitMapBuffer, NULL);

        if (newBuffer)
        {
            PhFree(EtGpuNodeBitMap.Buffer);
            EtGpuNodeBitMap.Buffer = newBuffer;
            EtGpuNodeBitMapBuffer = newBuffer;
            EtGpuNodeBitMapBitsSet = RtlNumberOfSetBits(&EtGpuNodeBitMap);
            EtSaveGpuMonitorSettings();
        }
    }

    // Update per-process statistics.
    // Note: no lock is needed because we only ever modify the list on this same thread.

    listEntry = EtProcessBlockListHead.Flink;

    while (listEntry != &EtProcessBlockListHead)
    {
        PET_PROCESS_BLOCK block;

        block = CONTAINING_RECORD(listEntry, ET_PROCESS_BLOCK, ListEntry);

        EtpUpdateSegmentInformation(block);
        EtpUpdateNodeInformation(block);

        if (elapsedTime != 0)
        {
            block->GpuNodeUsage = (FLOAT)(block->GpuRunningTimeDelta.Delta / (elapsedTime * EtGpuNodeBitMapBitsSet));

            if (block->GpuNodeUsage > 1)
                block->GpuNodeUsage = 1;
        }

        if (maxNodeValue < block->GpuNodeUsage)
        {
            maxNodeValue = block->GpuNodeUsage;
            maxNodeBlock = block;
        }

        listEntry = listEntry->Flink;
    }

    // Update history buffers.

    if (runCount != 0)
    {
        PhAddItemCircularBuffer_FLOAT(&EtGpuNodeHistory, EtGpuNodeUsage);
        PhAddItemCircularBuffer_ULONG(&EtGpuDedicatedHistory, (ULONG)(EtGpuDedicatedUsage / PAGE_SIZE));
        PhAddItemCircularBuffer_ULONG(&EtGpuSharedHistory, (ULONG)(EtGpuSharedUsage / PAGE_SIZE));

        for (i = 0; i < EtGpuTotalNodeCount; i++)
        {
            FLOAT usage;

            usage = (FLOAT)(EtGpuNodesTotalRunningTimeDelta[i].Delta / elapsedTime);

            if (usage > 1)
                usage = 1;

            PhAddItemCircularBuffer_FLOAT(&EtGpuNodesHistory[i], usage);
        }

        if (maxNodeBlock)
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxGpuNodeHistory, HandleToUlong(maxNodeBlock->ProcessItem->ProcessId));
            PhAddItemCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, maxNodeBlock->GpuNodeUsage);
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

VOID EtSaveGpuMonitorSettings(
    VOID
    )
{
    PPH_STRING string;

    string = PhBufferToHexString((PUCHAR)EtGpuNodeBitMapBuffer, BYTES_NEEDED_FOR_BITS(EtGpuTotalNodeCount));
    PhSetStringSetting2(SETTING_NAME_GPU_NODE_BITMAP, &string->sr);
    PhDereferenceObject(string);
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

    return -1;
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

VOID EtAllocateGpuNodeBitMap(
    _Out_ PRTL_BITMAP BitMap
    )
{
    SIZE_T numberOfBytes;

    numberOfBytes = BYTES_NEEDED_FOR_BITS(EtGpuTotalNodeCount);

    BitMap->Buffer = PhAllocate(numberOfBytes);
    BitMap->SizeOfBitMap = EtGpuTotalNodeCount;
    memset(BitMap->Buffer, 0, numberOfBytes);
}

VOID EtUpdateGpuNodeBitMap(
    _In_ PRTL_BITMAP NewBitMap
    )
{
    PULONG buffer;

    buffer = _InterlockedExchangePointer(&EtGpuNewNodeBitMapBuffer, NewBitMap->Buffer);

    if (buffer)
        PhFree(buffer);
}

VOID EtQueryProcessGpuStatistics(
    _In_ HANDLE ProcessHandle,
    _Out_ PET_PROCESS_GPU_STATISTICS Statistics
    )
{
    NTSTATUS status;
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

            if (NT_SUCCESS(status = D3DKMTQueryStatistics(&queryStatistics)))
            {
                ULONG64 bytesCommitted;

                if (WindowsVersion >= WINDOWS_8)
                {
                    bytesCommitted = queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                }
                else
                {
                    bytesCommitted = (ULONG)queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                }

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
