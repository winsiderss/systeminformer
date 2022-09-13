/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022
 *
 */

#include "devices.h"
#include "../ExtendedTools/extension/plugin.h"

static LARGE_INTEGER GraphicsTotalRunningTimeFrequency = { 0 };
BOOLEAN GraphicsGraphShowText = FALSE;
BOOLEAN GraphicsEnableScaleGraph = FALSE;
BOOLEAN GraphicsEnableScaleText = FALSE;
BOOLEAN GraphicsPropagateCpuUsage = FALSE;

VOID GraphicsDeviceEntryDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PDV_GPU_ENTRY entry = Object;

    PhAcquireQueuedLockExclusive(&GraphicsDevicesListLock);
    PhRemoveItemList(GraphicsDevicesList, PhFindItemList(GraphicsDevicesList, entry));
    PhReleaseQueuedLockExclusive(&GraphicsDevicesListLock);

    DeleteGraphicsDeviceId(&entry->Id);

    PhDeleteCircularBuffer_FLOAT(&entry->GpuUsageHistory);
    PhDeleteCircularBuffer_ULONG64(&entry->DedicatedHistory);
    PhDeleteCircularBuffer_ULONG64(&entry->SharedHistory);
    PhDeleteCircularBuffer_ULONG64(&entry->CommitHistory);
    PhDeleteCircularBuffer_FLOAT(&entry->PowerHistory);
    PhDeleteCircularBuffer_FLOAT(&entry->TemperatureHistory);
    PhDeleteCircularBuffer_ULONG(&entry->FanHistory);
}

VOID GraphicsDeviceInitialize(
    VOID
    )
{
    GraphicsDevicesList = PhCreateList(1);
    GraphicsDeviceEntryType = PhCreateObjectType(L"GraphicsDeviceEntry", 0, GraphicsDeviceEntryDeleteProcedure);
    GraphicsGraphShowText = !!PhGetIntegerSetting(L"GraphShowText");
    GraphicsEnableScaleGraph = !!PhGetIntegerSetting(L"EnableGraphMaxScale");
    GraphicsEnableScaleText = !!PhGetIntegerSetting(L"EnableGraphMaxText");
    GraphicsPropagateCpuUsage = !!PhGetIntegerSetting(L"PropagateCpuUsage");
    PhQueryPerformanceFrequency(&GraphicsTotalRunningTimeFrequency);
}

VOID GraphicsDevicesUpdate(
    VOID
    )
{
    static ULONG runCount = 0; // MUST keep in sync with runCount in process provider

    PhAcquireQueuedLockShared(&GraphicsDevicesListLock);

    for (ULONG i = 0; i < GraphicsDevicesList->Count; i++)
    {
        PDV_GPU_ENTRY entry;

        entry = PhReferenceObjectSafe(GraphicsDevicesList->Items[i]);

        if (!entry)
            continue;

        if (!entry->DeviceSupported)
        {
            D3DKMT_HANDLE adapterHandle;
            LUID adapterLuid;
            ULONG numberOfSegments;
            ULONG numberOfNodes;
            ULONG64 sharedTotal;
            ULONG64 sharedCommitTotal;
            ULONG64 dedicatedTotal;
            ULONG64 dedicatedCommitTotal;

            if (NT_SUCCESS(GraphicsOpenAdapterFromDeviceName(&adapterHandle, &adapterLuid, PhGetString(entry->Id.DevicePath))))
            {
                if (NT_SUCCESS(GraphicsQueryAdapterNodeInformation(adapterLuid, &numberOfSegments, &numberOfNodes)))
                {
                    if (NT_SUCCESS(GraphicsQueryAdapterSegmentLimits(
                        adapterLuid,
                        numberOfSegments,
                        NULL,
                        &sharedCommitTotal,
                        &sharedTotal,
                        NULL,
                        &dedicatedCommitTotal,
                        &dedicatedTotal
                        )))
                    {
                        entry->NumberOfSegments = numberOfSegments;
                        entry->NumberOfNodes = numberOfNodes;
                        entry->SharedLimit = sharedTotal;
                        entry->DedicatedLimit = dedicatedTotal;
                        entry->CommitLimit = sharedCommitTotal + dedicatedCommitTotal;

                        if (entry->TotalRunningTimeNodesDelta)
                        {
                            PhFree(entry->TotalRunningTimeNodesDelta);
                            entry->TotalRunningTimeNodesDelta = NULL;
                        }
                        if (entry->GpuNodesHistory)
                        {
                            PhFree(entry->GpuNodesHistory);
                            entry->GpuNodesHistory = NULL;
                        }

                        entry->TotalRunningTimeNodesDelta = PhAllocateZero(sizeof(PH_UINT64_DELTA) * numberOfNodes);
                        entry->GpuNodesHistory = PhAllocateZero(sizeof(PH_CIRCULAR_BUFFER_FLOAT) * numberOfNodes);

                        {
                            ULONG sampleCount = PhGetIntegerSetting(L"SampleCount");

                            for (ULONG node = 0; node < entry->NumberOfNodes; node++)
                            {
                                PhDeleteCircularBuffer_FLOAT(&entry->GpuNodesHistory[node]);
                                PhInitializeCircularBuffer_FLOAT(&entry->GpuNodesHistory[node], sampleCount);
                            }
                        }
                    }
                }

                GraphicsCloseAdapterHandle(adapterHandle);
            }

            entry->DeviceSupported = TRUE;
        }

        if (entry->DeviceSupported)
        {
            D3DKMT_HANDLE adapterHandle;
            LUID adapterLuid;
            LARGE_INTEGER performanceCounter = { 0 };
            ULONG64 sharedUsage = 0;
            ULONG64 sharedCommit = 0;
            ULONG64 sharedLimit = 0;
            ULONG64 dedicatedUsage = 0;
            ULONG64 dedicatedCommit = 0;
            ULONG64 dedicatedLimit = 0;
            FLOAT powerUsage;
            FLOAT temperature;
            ULONG fanRPM;

            if (NT_SUCCESS(GraphicsOpenAdapterFromDeviceName(&adapterHandle, &adapterLuid, PhGetString(entry->Id.DevicePath))))
            {
                //if (dedicatedUsage = GraphicsDevicePluginInterfaceGetGpuAdapterDedicated(adapterLuid))
                //{
                //    entry->CurrentDedicatedUsage = dedicatedUsage;
                //}
                //else
                //{
                //    entry->CurrentDedicatedUsage = 0;
                //}
                //
                //if (sharedUsage = GraphicsDevicePluginInterfaceGetGpuAdapterShared(adapterLuid))
                //{
                //    entry->CurrentSharedUsage = sharedUsage;
                //}
                //else
                //{
                //    entry->CurrentSharedUsage = 0;
                //}

                if (!(dedicatedUsage && sharedUsage))
                {
                    if (NT_SUCCESS(GraphicsQueryAdapterSegmentLimits(
                        adapterLuid,
                        entry->NumberOfSegments,
                        &sharedUsage,
                        &sharedCommit,
                        NULL,
                        &dedicatedUsage,
                        &dedicatedCommit,
                        NULL
                        )))
                    {
                        entry->CurrentSharedUsage = sharedUsage;
                        entry->CurrentDedicatedUsage = dedicatedUsage;
                        entry->CurrentCommitUsage = sharedCommit + dedicatedCommit;
                    }
                    else
                    {
                        entry->CurrentSharedUsage = 0;
                        entry->CurrentDedicatedUsage = 0;
                        entry->CurrentCommitUsage = 0;
                    }
                }

                if (NT_SUCCESS(GraphicsQueryAdapterDevicePerfData(adapterHandle, &powerUsage, &temperature, &fanRPM)))
                {
                    entry->CurrentPowerUsage = powerUsage;
                    entry->CurrentTemperature = temperature;
                    entry->CurrentFanRPM = fanRPM;
                }
                else
                {
                    entry->CurrentPowerUsage = 0;
                    entry->CurrentTemperature = 0;
                    entry->CurrentFanRPM = 0;
                }

                GraphicsCloseAdapterHandle(adapterHandle);
            }
            else
            {
                entry->CurrentSharedUsage = 0;
                entry->CurrentDedicatedUsage = 0;
                entry->CurrentCommitUsage = 0;
                entry->CurrentPowerUsage = 0;
                entry->CurrentTemperature = 0;
                entry->CurrentFanRPM = 0;
                entry->DeviceSupported = FALSE;
            }

            if (entry->TotalRunningTimeNodesDelta)
            {
                FLOAT value;

                if (value = GraphicsDevicePluginInterfaceGetGpuAdapterUtilization(adapterLuid))
                {
                    entry->CurrentGpuUsage = value;

                    for (ULONG n = 0; n < entry->NumberOfNodes; n++)
                    {
                        value = GraphicsDevicePluginInterfaceGetGpuAdapterEngineUtilization(adapterLuid, n);
                        PhAddItemCircularBuffer_FLOAT(&entry->GpuNodesHistory[n], value);
                    }
                }
                else
                {
                    for (ULONG n = 0; n < entry->NumberOfNodes; n++)
                    {
                        ULONG64 runningTime = 0;
                        GraphicsQueryAdapterNodeRunningTime(adapterLuid, n, &runningTime);
                        PhUpdateDelta(&entry->TotalRunningTimeNodesDelta[n], runningTime);
                    }

                    PhQueryPerformanceCounter(&performanceCounter, NULL);
                    PhUpdateDelta(&entry->TotalRunningTimeDelta, performanceCounter.QuadPart);

                    DOUBLE elapsedTime = (DOUBLE)entry->TotalRunningTimeDelta.Delta * 10000000 / GraphicsTotalRunningTimeFrequency.QuadPart;
                    DOUBLE tempValue = 0.0f;

                    if (elapsedTime != 0)
                    {
                        for (ULONG node = 0; node < entry->NumberOfNodes; node++)
                        {
                            DOUBLE usage = (DOUBLE)(entry->TotalRunningTimeNodesDelta[node].Delta / elapsedTime);

                            if (usage > 1)
                                usage = 1;
                            if (usage > tempValue)
                                tempValue = usage;

                            if (runCount != 0)
                            {
                                PhAddItemCircularBuffer_FLOAT(&entry->GpuNodesHistory[node], (FLOAT)usage);
                            }
                        }

                        entry->CurrentGpuUsage = (FLOAT)tempValue;
                    }
                    else
                    {
                        if (runCount != 0)
                        {
                            for (ULONG node = 0; node < entry->NumberOfNodes; node++)
                            {
                                PhAddItemCircularBuffer_FLOAT(&entry->GpuNodesHistory[node], 0);
                            }
                        }

                        entry->CurrentGpuUsage = 0.0f;
                    }
                }
            }

            entry->DevicePresent = TRUE;
        }
        else
        {
            if (runCount != 0)
            {
                for (ULONG node = 0; node < entry->NumberOfNodes; node++)
                {
                    PhAddItemCircularBuffer_FLOAT(&entry->GpuNodesHistory[node], 0);
                }
            }

            entry->CurrentGpuUsage = 0.0f;
            entry->CurrentDedicatedUsage = 0;
            entry->CurrentSharedUsage = 0;
            entry->CurrentCommitUsage = 0;
            entry->CurrentPowerUsage = 0.f;
            entry->CurrentTemperature = 0.f;
            entry->CurrentFanRPM = 0;
            entry->DevicePresent = FALSE;
        }

        if (runCount != 0)
        {
            PhAddItemCircularBuffer_FLOAT(&entry->GpuUsageHistory, entry->CurrentGpuUsage);
            PhAddItemCircularBuffer_ULONG64(&entry->DedicatedHistory, entry->CurrentDedicatedUsage);
            PhAddItemCircularBuffer_ULONG64(&entry->SharedHistory, entry->CurrentSharedUsage);
            PhAddItemCircularBuffer_ULONG64(&entry->CommitHistory, entry->CurrentCommitUsage);
            PhAddItemCircularBuffer_FLOAT(&entry->PowerHistory, entry->CurrentPowerUsage);
            PhAddItemCircularBuffer_FLOAT(&entry->TemperatureHistory, entry->CurrentTemperature);
            PhAddItemCircularBuffer_ULONG(&entry->FanHistory, entry->CurrentFanRPM);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&GraphicsDevicesListLock);

    runCount++;
}

VOID InitializeGraphicsDeviceId(
    _Out_ PDV_GPU_ID Id,
    _In_ PPH_STRING DevicePath
    )
{
    PhSetReference(&Id->DevicePath, DevicePath);
}

VOID CopyGraphicsDeviceId(
    _Out_ PDV_GPU_ID Destination,
    _In_ PDV_GPU_ID Source
    )
{
    InitializeGraphicsDeviceId(
        Destination,
        Source->DevicePath
        );
}

VOID DeleteGraphicsDeviceId(
    _Inout_ PDV_GPU_ID Id
    )
{
    PhClearReference(&Id->DevicePath);
}

BOOLEAN EquivalentGraphicsDeviceId(
    _In_ PDV_GPU_ID Id1,
    _In_ PDV_GPU_ID Id2
    )
{
    return PhEqualString(Id1->DevicePath, Id2->DevicePath, TRUE);
}

PDV_GPU_ENTRY CreateGraphicsDeviceEntry(
    _In_ PDV_GPU_ID Id
    )
{
    PDV_GPU_ENTRY entry;
    ULONG sampleCount;

    entry = PhCreateObjectZero(sizeof(DV_GPU_ENTRY), GraphicsDeviceEntryType);

    CopyGraphicsDeviceId(&entry->Id, Id);

    sampleCount = PhGetIntegerSetting(L"SampleCount");
    PhInitializeCircularBuffer_FLOAT(&entry->GpuUsageHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG64(&entry->DedicatedHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG64(&entry->SharedHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG64(&entry->CommitHistory, sampleCount);
    PhInitializeCircularBuffer_FLOAT(&entry->PowerHistory, sampleCount);
    PhInitializeCircularBuffer_FLOAT(&entry->TemperatureHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG(&entry->FanHistory, sampleCount);

    PhAcquireQueuedLockExclusive(&GraphicsDevicesListLock);
    PhAddItemList(GraphicsDevicesList, entry);
    PhReleaseQueuedLockExclusive(&GraphicsDevicesListLock);

    return entry;
}

PEXTENDEDTOOLS_INTERFACE GraphicsDeviceGetPluginInterface(
    VOID
    )
{
    static PEXTENDEDTOOLS_INTERFACE pluginInterface = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_PLUGIN toolStatusPlugin;

        if (toolStatusPlugin = PhFindPlugin(EXTENDEDTOOLS_PLUGIN_NAME))
        {
            pluginInterface = PhGetPluginInformation(toolStatusPlugin)->Interface;

            if (pluginInterface->Version < EXTENDEDTOOLS_INTERFACE_VERSION)
                pluginInterface = NULL;
        }

        PhEndInitOnce(&initOnce);
    }

    return pluginInterface;
}

FLOAT GraphicsDevicePluginInterfaceGetGpuAdapterUtilization(
    _In_ LUID AdapterLuid
    )
{
    if (GraphicsDeviceGetPluginInterface())
    {
        return GraphicsDeviceGetPluginInterface()->GetGpuAdapterUtilization(AdapterLuid);
    }

    return 0.0f;
}

ULONG64 GraphicsDevicePluginInterfaceGetGpuAdapterDedicated(
    _In_ LUID AdapterLuid
    )
{
    if (GraphicsDeviceGetPluginInterface())
    {
        return GraphicsDeviceGetPluginInterface()->GetGpuAdapterDedicated(AdapterLuid);
    }

    return 0;
}

ULONG64 GraphicsDevicePluginInterfaceGetGpuAdapterShared(
    _In_ LUID AdapterLuid
    )
{
    if (GraphicsDeviceGetPluginInterface())
    {
        return GraphicsDeviceGetPluginInterface()->GetGpuAdapterShared(AdapterLuid);
    }

    return 0;
}

FLOAT GraphicsDevicePluginInterfaceGetGpuAdapterEngineUtilization(
    _In_ LUID AdapterLuid,
    _In_ ULONG EngineId
    )
{
    if (GraphicsDeviceGetPluginInterface())
    {
        return GraphicsDeviceGetPluginInterface()->GetGpuAdapterEngineUtilization(AdapterLuid, EngineId);
    }

    return 0.0f;
}
