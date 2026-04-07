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

/**
 * Deletes a graphics device entry and releases its resources.
 *
 * \param Object Graphics device entry object.
 * \param Flags Object deletion flags.
 */
_Function_class_(PH_TYPE_DELETE_PROCEDURE)
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

/**
 * Initializes the graphics device list and related settings.
 */
VOID GraphicsDeviceInitialize(
    VOID
    )
{
    GraphicsDevicesList = PhCreateList(1);
    GraphicsDeviceEntryType = PhCreateObjectType(L"GraphicsDeviceEntry", 0, GraphicsDeviceEntryDeleteProcedure);
    GraphicsGraphShowText = !!PhGetIntegerSetting(SETTING_GRAPH_SHOW_TEXT);
    GraphicsEnableAvxSupport = !!PhGetIntegerSetting(SETTING_ENABLE_AVX_SUPPORT);
    GraphicsEnableScaleGraph = !!PhGetIntegerSetting(SETTING_ENABLE_GRAPH_MAX_SCALE);
    GraphicsEnableScaleText = !!PhGetIntegerSetting(SETTING_ENABLE_GRAPH_MAX_TEXT);
    GraphicsPropagateCpuUsage = !!PhGetIntegerSetting(SETTING_PROPAGATE_CPU_USAGE);
    PhQueryPerformanceFrequency(&GraphicsTotalRunningTimeFrequency);
}

/**
 * Refreshes all tracked graphics devices and samples current counters.
 *
 * \param RunCount Current provider update count.
 */
VOID GraphicsDevicesUpdate(
    _In_ ULONG RunCount
    )
{
    PhAcquireQueuedLockShared(&GraphicsDevicesListLock);

    for (ULONG i = 0; i < GraphicsDevicesList->Count; i++)
    {
        PDV_GPU_ENTRY entry;

        entry = PhReferenceObjectUnsafe(GraphicsDevicesList->Items[i]);

        if (!entry)
            continue;

        if (!entry->DeviceSupported)
        {
            BOOLEAN initialized = FALSE;
            D3DKMT_HANDLE adapterHandle;
            LUID adapterLuid = { 0 };
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
                            ULONG sampleCount = PhGetIntegerSetting(SETTING_SAMPLE_COUNT);

                            for (ULONG node = 0; node < entry->NumberOfNodes; node++)
                            {
                                PhDeleteCircularBuffer_FLOAT(&entry->GpuNodesHistory[node]);
                                PhInitializeCircularBuffer_FLOAT(&entry->GpuNodesHistory[node], sampleCount);
                            }
                        }

                        initialized = TRUE;
                    }
                }

                GraphicsCloseAdapterHandle(adapterHandle);
            }

            entry->DeviceSupported = initialized;
        }

        if (entry->DeviceSupported)
        {
            D3DKMT_HANDLE adapterHandle;
            LUID adapterLuid = { 0 };
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
                entry->DevicePresent = FALSE;
            }

            if (entry->DeviceSupported && entry->TotalRunningTimeNodesDelta)
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

                    PhQueryPerformanceCounter(&performanceCounter);
                    PhUpdateDelta(&entry->TotalRunningTimeDelta, performanceCounter.QuadPart);

                    FLOAT elapsedTime = (FLOAT)entry->TotalRunningTimeDelta.Delta * 10000000 / GraphicsTotalRunningTimeFrequency.QuadPart;
                    FLOAT tempValue = 0.0f;

                    if (elapsedTime != 0)
                    {
                        for (ULONG node = 0; node < entry->NumberOfNodes; node++)
                        {
                            FLOAT usage = (FLOAT)(entry->TotalRunningTimeNodesDelta[node].Delta / elapsedTime);

                            if (usage > 1)
                                usage = 1;
                            if (usage > tempValue)
                                tempValue = usage;

                            if (RunCount != 0)
                            {
                                PhAddItemCircularBuffer_FLOAT(&entry->GpuNodesHistory[node], usage);
                            }
                        }

                        entry->CurrentGpuUsage = tempValue;
                    }
                    else
                    {
                        if (RunCount != 0)
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

            entry->DevicePresent = entry->DeviceSupported;
        }
        else
        {
            if (RunCount != 0)
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

        if (RunCount != 0)
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
}

/**
 * Initializes a graphics device identifier.
 *
 * \param Id Destination identifier.
 * \param DevicePath Device interface path.
 */
VOID InitializeGraphicsDeviceId(
    _Out_ PDV_GPU_ID Id,
    _In_ PPH_STRING DevicePath
    )
{
    PhSetReference(&Id->DevicePath, DevicePath);
}

/**
 * Copies a graphics device identifier.
 *
 * \param Destination Destination identifier.
 * \param Source Source identifier.
 */
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

/**
 * Releases references held by a graphics device identifier.
 *
 * \param Id Identifier to release.
 */
VOID DeleteGraphicsDeviceId(
    _Inout_ PDV_GPU_ID Id
    )
{
    PhClearReference(&Id->DevicePath);
}

/**
 * Compares two graphics device identifiers.
 *
 * \param Id1 First identifier.
 * \param Id2 Second identifier.
 * \return TRUE if both identifiers refer to the same device.
 */
BOOLEAN EquivalentGraphicsDeviceId(
    _In_ PDV_GPU_ID Id1,
    _In_ PDV_GPU_ID Id2
    )
{
    return PhEqualString(Id1->DevicePath, Id2->DevicePath, TRUE);
}

/**
 * Creates and registers a graphics device entry.
 *
 * \param Id Graphics device identifier.
 * \return Newly created graphics device entry.
 */
PDV_GPU_ENTRY CreateGraphicsDeviceEntry(
    _In_ PDV_GPU_ID Id
    )
{
    PDV_GPU_ENTRY entry;
    ULONG sampleCount;

    entry = PhCreateObjectZero(sizeof(DV_GPU_ENTRY), GraphicsDeviceEntryType);

    CopyGraphicsDeviceId(&entry->Id, Id);

    sampleCount = PhGetIntegerSetting(SETTING_SAMPLE_COUNT);
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

/**
 * Gets the ExtendedTools plugin interface used for GPU data.
 *
 * \return Cached ExtendedTools interface, or NULL if unavailable.
 */
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

/**
 * Queries GPU utilization from the ExtendedTools plugin interface.
 *
 * \param AdapterLuid Adapter LUID.
 * \return GPU utilization value.
 */
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

/**
 * Queries dedicated GPU memory usage from the ExtendedTools plugin interface.
 *
 * \param AdapterLuid Adapter LUID.
 * \return Dedicated memory usage.
 */
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

/**
 * Queries shared GPU memory usage from the ExtendedTools plugin interface.
 *
 * \param AdapterLuid Adapter LUID.
 * \return Shared memory usage.
 */
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

/**
 * Queries per-engine GPU utilization from the ExtendedTools plugin interface.
 *
 * \param AdapterLuid Adapter LUID.
 * \param EngineId Engine ordinal.
 * \return Engine utilization value.
 */
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
