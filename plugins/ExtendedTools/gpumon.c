/*
 * Process Hacker Extended Tools -
 *   GPU monitoring
 *
 * Copyright (C) 2011 wj32
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
#include "setupapi.h"
#include "d3dkmt.h"
#include "gpumon.h"

static GUID GUID_DISPLAY_DEVICE_ARRIVAL_I = { 0x1ca05180, 0xa699, 0x450a, { 0x9a, 0x0c, 0xde, 0x4f, 0xbe, 0x3d, 0xdd, 0x89 } };

static PFND3DKMT_OPENADAPTERFROMDEVICENAME D3DKMTOpenAdapterFromDeviceName_I;
static PFND3DKMT_CLOSEADAPTER D3DKMTCloseAdapter_I;
static PFND3DKMT_QUERYSTATISTICS D3DKMTQueryStatistics_I;
static _SetupDiGetClassDevsW SetupDiGetClassDevsW_I;
static _SetupDiDestroyDeviceInfoList SetupDiDestroyDeviceInfoList_I;
static _SetupDiEnumDeviceInterfaces SetupDiEnumDeviceInterfaces_I;
static _SetupDiGetDeviceInterfaceDetailW SetupDiGetDeviceInterfaceDetailW_I;
static _SetupDiGetDeviceRegistryPropertyW SetupDiGetDeviceRegistryPropertyW_I;

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
    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_GPU_MONITOR) && WindowsVersion >= WINDOWS_7)
    {
        HMODULE gdi32Handle;
        HMODULE setupapiHandle;

        if (gdi32Handle = GetModuleHandle(L"gdi32.dll"))
        {
            D3DKMTOpenAdapterFromDeviceName_I = (PVOID)GetProcAddress(gdi32Handle, "D3DKMTOpenAdapterFromDeviceName");
            D3DKMTCloseAdapter_I = (PVOID)GetProcAddress(gdi32Handle, "D3DKMTCloseAdapter");
            D3DKMTQueryStatistics_I = (PVOID)GetProcAddress(gdi32Handle, "D3DKMTQueryStatistics");
        }

        if (setupapiHandle = LoadLibrary(L"setupapi.dll"))
        {
            SetupDiGetClassDevsW_I = (PVOID)GetProcAddress(setupapiHandle, "SetupDiGetClassDevsW");
            SetupDiDestroyDeviceInfoList_I = (PVOID)GetProcAddress(setupapiHandle, "SetupDiDestroyDeviceInfoList");
            SetupDiEnumDeviceInterfaces_I = (PVOID)GetProcAddress(setupapiHandle, "SetupDiEnumDeviceInterfaces");
            SetupDiGetDeviceInterfaceDetailW_I = (PVOID)GetProcAddress(setupapiHandle, "SetupDiGetDeviceInterfaceDetailW");
            SetupDiGetDeviceRegistryPropertyW_I = (PVOID)GetProcAddress(setupapiHandle, "SetupDiGetDeviceRegistryPropertyW");
        }

        if (
            D3DKMTOpenAdapterFromDeviceName_I &&
            D3DKMTCloseAdapter_I &&
            D3DKMTQueryStatistics_I &&
            SetupDiGetClassDevsW_I &&
            SetupDiDestroyDeviceInfoList_I &&
            SetupDiEnumDeviceInterfaces_I &&
            SetupDiGetDeviceInterfaceDetailW_I
            )
        {
            EtpGpuAdapterList = PhCreateList(4);

            if (EtpInitializeD3DStatistics() && EtpGpuAdapterList->Count != 0)
                EtGpuEnabled = TRUE;
        }
    }

    if (EtGpuEnabled)
    {
        ULONG sampleCount;
        ULONG i;
        PPH_STRING bitmapString;

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
            ProcessesUpdatedCallback,
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
    }
}

BOOLEAN EtpInitializeD3DStatistics(
    VOID
    )
{
    LOGICAL result;
    HDEVINFO deviceInfoSet;
    ULONG memberIndex;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA detailData;
    SP_DEVINFO_DATA deviceInfoData;
    ULONG detailDataSize;
    D3DKMT_OPENADAPTERFROMDEVICENAME openAdapterFromDeviceName;
    D3DKMT_QUERYSTATISTICS queryStatistics;

    deviceInfoSet = SetupDiGetClassDevsW_I(&GUID_DISPLAY_DEVICE_ARRIVAL_I, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (!deviceInfoSet)
        return FALSE;

    memberIndex = 0;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    while (SetupDiEnumDeviceInterfaces_I(deviceInfoSet, NULL, &GUID_DISPLAY_DEVICE_ARRIVAL_I, memberIndex, &deviceInterfaceData))
    {
        detailDataSize = 0x100;
        detailData = PhAllocate(detailDataSize);
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (!(result = SetupDiGetDeviceInterfaceDetailW_I(deviceInfoSet, &deviceInterfaceData, detailData, detailDataSize, &detailDataSize, &deviceInfoData)) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            PhFree(detailData);
            detailData = PhAllocate(detailDataSize);

            if (detailDataSize >= sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA))
                detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            result = SetupDiGetDeviceInterfaceDetailW_I(deviceInfoSet, &deviceInterfaceData, detailData, detailDataSize, &detailDataSize, &deviceInfoData);
        }

        if (result)
        {
            openAdapterFromDeviceName.pDeviceName = detailData->DevicePath;

            if (NT_SUCCESS(D3DKMTOpenAdapterFromDeviceName_I(&openAdapterFromDeviceName)))
            {
                memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
                queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
                queryStatistics.AdapterLuid = openAdapterFromDeviceName.AdapterLuid;

                if (NT_SUCCESS(D3DKMTQueryStatistics_I(&queryStatistics)))
                {
                    PETP_GPU_ADAPTER gpuAdapter;
                    ULONG i;

                    gpuAdapter = EtpAllocateGpuAdapter(queryStatistics.QueryResult.AdapterInformation.NbSegments);
                    gpuAdapter->AdapterLuid = openAdapterFromDeviceName.AdapterLuid;
                    gpuAdapter->Description = EtpQueryDeviceDescription(deviceInfoSet, &deviceInfoData);
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

                        if (NT_SUCCESS(D3DKMTQueryStatistics_I(&queryStatistics)))
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
            }
        }

        PhFree(detailData);

        memberIndex++;
    }

    SetupDiDestroyDeviceInfoList_I(deviceInfoSet);

    EtGpuNodeBitMapBuffer = PhAllocate(BYTES_NEEDED_FOR_BITS(EtGpuTotalNodeCount));
    RtlInitializeBitMap(&EtGpuNodeBitMap, EtGpuNodeBitMapBuffer, EtGpuTotalNodeCount);
    RtlSetBits(&EtGpuNodeBitMap, 0, 1);
    EtGpuNodeBitMapBitsSet = 1;

    return TRUE;
}

PETP_GPU_ADAPTER EtpAllocateGpuAdapter(
    __in ULONG NumberOfSegments
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
    __in HDEVINFO DeviceInfoSet,
    __in PSP_DEVINFO_DATA DeviceInfoData
    )
{
    LOGICAL result;
    PPH_STRING string;
    ULONG bufferSize;

    if (!SetupDiGetDeviceRegistryPropertyW_I)
        return NULL;

    bufferSize = 0x40;
    string = PhCreateStringEx(NULL, bufferSize);

    if (!(result = SetupDiGetDeviceRegistryPropertyW_I(
        DeviceInfoSet,
        DeviceInfoData,
        SPDRP_DEVICEDESC,
        NULL,
        (PBYTE)string->Buffer,
        bufferSize,
        &bufferSize
        )) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        PhDereferenceObject(string);
        string = PhCreateStringEx(NULL, bufferSize);

        result = SetupDiGetDeviceRegistryPropertyW_I(
            DeviceInfoSet,
            DeviceInfoData,
            SPDRP_DEVICEDESC,
            NULL,
            (PBYTE)string->Buffer,
            bufferSize,
            NULL
            );
    }

    if (!result)
    {
        PhDereferenceObject(string);
        return NULL;
    }

    PhTrimToNullTerminatorString(string);

    return string;
}

static VOID EtpUpdateSegmentInformation(
    __in_opt PET_PROCESS_BLOCK Block
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

            if (NT_SUCCESS(D3DKMTQueryStatistics_I(&queryStatistics)))
            {
                if (Block)
                {
                    if (RtlCheckBit(&gpuAdapter->ApertureBitMap, j))
                        sharedUsage += queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                    else
                        dedicatedUsage += queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                }
                else
                {
                    ULONG64 bytesCommitted;

                    if (WindowsVersion >= WINDOWS_8)
                    {
                        bytesCommitted = queryStatistics.QueryResult.SegmentInformation.BytesCommitted;
                    }
                    else
                    {
                        bytesCommitted = queryStatistics.QueryResult.SegmentInformationV1.BytesCommitted;
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

static VOID EtpUpdateNodeInformation(
    __in_opt PET_PROCESS_BLOCK Block
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

            if (NT_SUCCESS(D3DKMTQueryStatistics_I(&queryStatistics)))
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

static VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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
            PhAddItemCircularBuffer_ULONG(&EtMaxGpuNodeHistory, (ULONG)maxNodeBlock->ProcessItem->ProcessId);
            PhAddItemCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, maxNodeBlock->GpuNodeUsage);
            PhReferenceProcessRecordForStatistics(maxNodeBlock->ProcessItem->Record);
        }
        else
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxGpuNodeHistory, (ULONG)NULL);
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

PPH_STRING EtGetGpuAdapterDescription(
    __in ULONG Index
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
    __out PRTL_BITMAP BitMap
    )
{
    SIZE_T numberOfBytes;

    numberOfBytes = BYTES_NEEDED_FOR_BITS(EtGpuTotalNodeCount);

    BitMap->Buffer = PhAllocate(numberOfBytes);
    BitMap->SizeOfBitMap = EtGpuTotalNodeCount;
    memset(BitMap->Buffer, 0, numberOfBytes);
}

VOID EtUpdateGpuNodeBitMap(
    __in PRTL_BITMAP NewBitMap
    )
{
    PULONG buffer;

    buffer = _InterlockedExchangePointer(&EtGpuNewNodeBitMapBuffer, NewBitMap->Buffer);

    if (buffer)
        PhFree(buffer);
}

VOID EtQueryProcessGpuStatistics(
    __in HANDLE ProcessHandle,
    __out PET_PROCESS_GPU_STATISTICS Statistics
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

            if (NT_SUCCESS(D3DKMTQueryStatistics_I(&queryStatistics)))
            {
                if (RtlCheckBit(&gpuAdapter->ApertureBitMap, j))
                {
                    Statistics->SharedCommitted += queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                }
                else
                {
                    Statistics->DedicatedCommitted += queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                }
            }
        }

        for (j = 0; j < gpuAdapter->NodeCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_NODE;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.hProcess = ProcessHandle;
            queryStatistics.QueryProcessNode.NodeId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics_I(&queryStatistics)))
            {
                Statistics->RunningTime += queryStatistics.QueryResult.ProcessNodeInformation.RunningTime.QuadPart;
                Statistics->ContextSwitches += queryStatistics.QueryResult.ProcessNodeInformation.ContextSwitch;
            }
        }

        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS;
        queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
        queryStatistics.hProcess = ProcessHandle;

        if (NT_SUCCESS(D3DKMTQueryStatistics_I(&queryStatistics)))
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
