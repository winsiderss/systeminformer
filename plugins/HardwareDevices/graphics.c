/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022-2023
 *
 */

#include "devices.h"

static PH_QUEUED_LOCK GraphicsDeviceInterfaceAdapterIndexLock = PH_QUEUED_LOCK_INIT;

/**
 * Finds a full adapter LUID in a cached list and returns its index.
 *
 * \param LuidList Cached adapter LUID list.
 * \param AdapterLuid Adapter LUID to search for.
 * \param Index Receives the matching list index.
 * \return TRUE if the LUID was found in the list.
 */
_Success_(return)
static BOOLEAN GraphicsFindCachedAdapterLuid(
    _In_ PPH_LIST LuidList,
    _In_ CONST LUID* AdapterLuid,
    _Out_ PULONG Index
    )
{
    for (ULONG i = 0; i < LuidList->Count; i++)
    {
        PLUID luidEntry = LuidList->Items[i];

        if (luidEntry->LowPart == AdapterLuid->LowPart &&
            luidEntry->HighPart == AdapterLuid->HighPart)
        {
            *Index = i;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Opens a graphics adapter handle from a device interface name.
 *
 * \param AdapterHandle Receives the adapter handle.
 * \param AdapterLuid Receives the adapter LUID, if requested.
 * \param DeviceName Device interface name.
 * \return NTSTATUS result of the open request.
 */
NTSTATUS GraphicsOpenAdapterFromDeviceName(
    _Out_ D3DKMT_HANDLE* AdapterHandle,
    _Out_opt_ PLUID AdapterLuid,
    _In_ PCWSTR DeviceName
    )
{
    NTSTATUS status;
    D3DKMT_OPENADAPTERFROMDEVICENAME openAdapterFromDeviceName;

    memset(&openAdapterFromDeviceName, 0, sizeof(D3DKMT_OPENADAPTERFROMDEVICENAME));
    openAdapterFromDeviceName.pDeviceName = DeviceName;

    status = D3DKMTOpenAdapterFromDeviceName(&openAdapterFromDeviceName);

    if (NT_SUCCESS(status))
    {
        *AdapterHandle = openAdapterFromDeviceName.hAdapter;

        if (AdapterLuid)
            *AdapterLuid = openAdapterFromDeviceName.AdapterLuid;
    }

    return status;
}

/**
 * Closes a graphics adapter handle.
 *
 * \param AdapterHandle Adapter handle to close.
 * \return TRUE if the adapter was closed successfully.
 */
BOOLEAN GraphicsCloseAdapterHandle(
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_CLOSEADAPTER closeAdapter;

    memset(&closeAdapter, 0, sizeof(D3DKMT_CLOSEADAPTER));
    closeAdapter.hAdapter = AdapterHandle;

    return NT_SUCCESS(D3DKMTCloseAdapter(&closeAdapter));
}

/**
 * Queries adapter information through D3DKMT.
 *
 * \param AdapterHandle Adapter handle.
 * \param InformationClass Information class to query.
 * \param Information Output buffer.
 * \param InformationLength Output buffer length.
 * \return NTSTATUS result of the query.
 */
NTSTATUS GraphicsQueryAdapterInformation(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ KMTQUERYADAPTERINFOTYPE InformationClass,
    _Out_writes_bytes_opt_(InformationLength) PVOID Information,
    _In_ UINT32 InformationLength
    )
{
    D3DKMT_QUERYADAPTERINFO queryAdapterInfo;

    memset(&queryAdapterInfo, 0, sizeof(D3DKMT_QUERYADAPTERINFO));
    queryAdapterInfo.hAdapter = AdapterHandle;
    queryAdapterInfo.Type = InformationClass;
    queryAdapterInfo.pPrivateDriverData = Information;
    queryAdapterInfo.PrivateDriverDataSize = InformationLength;

    return D3DKMTQueryAdapterInfo(&queryAdapterInfo);
}

/**
 * Queries the adapter's segment and node counts.
 *
 * \param AdapterLuid Adapter LUID.
 * \param NumberOfSegments Receives the segment count.
 * \param NumberOfNodes Receives the node count.
 * \return NTSTATUS result of the query.
 */
NTSTATUS GraphicsQueryAdapterNodeInformation(
    _In_ LUID AdapterLuid,
    _Out_ PULONG NumberOfSegments,
    _Out_ PULONG NumberOfNodes
    )
{
    NTSTATUS status;
    D3DKMT_QUERYSTATISTICS queryStatistics;

    memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
    queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
    queryStatistics.AdapterLuid = AdapterLuid;

    status = D3DKMTQueryStatistics(&queryStatistics);

    if (NT_SUCCESS(status))
    {
        *NumberOfSegments = queryStatistics.QueryResult.AdapterInformation.NbSegments;
        *NumberOfNodes = queryStatistics.QueryResult.AdapterInformation.NodeCount;
    }

    return status;
}

/**
 * Queries memory segment usage and limits for an adapter.
 *
 * \param AdapterLuid Adapter LUID.
 * \param NumberOfSegments Number of segments to inspect.
 * \param SharedUsage Receives total shared resident usage.
 * \param SharedCommit Receives shared committed bytes.
 * \param SharedLimit Receives shared commit limit.
 * \param DedicatedUsage Receives dedicated resident usage.
 * \param DedicatedCommit Receives dedicated committed bytes.
 * \param DedicatedLimit Receives dedicated commit limit.
 * \return NTSTATUS result of the query.
 */
NTSTATUS GraphicsQueryAdapterSegmentLimits(
    _In_ LUID AdapterLuid,
    _In_ ULONG NumberOfSegments,
    _Out_opt_ PULONG64 SharedUsage,
    _Out_opt_ PULONG64 SharedCommit,
    _Out_opt_ PULONG64 SharedLimit,
    _Out_opt_ PULONG64 DedicatedUsage,
    _Out_opt_ PULONG64 DedicatedCommit,
    _Out_opt_ PULONG64 DedicatedLimit
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 sharedUsage = 0;
    ULONG64 sharedCommit = 0;
    ULONG64 sharedLimit = 0;
    ULONG64 dedicatedUsage = 0;
    ULONG64 dedicatedCommit = 0;
    ULONG64 dedicatedLimit = 0;

    for (ULONG i = 0; i < NumberOfSegments; i++)
    {
        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
        queryStatistics.AdapterLuid = AdapterLuid;
        queryStatistics.QuerySegment.SegmentId = i;

        status = D3DKMTQueryStatistics(&queryStatistics);

        if (!NT_SUCCESS(status))
            return status;

        if (NetWindowsVersion >= WINDOWS_8)
        {
            if (queryStatistics.QueryResult.SegmentInformation.Aperture)
            {
                sharedLimit += queryStatistics.QueryResult.SegmentInformation.CommitLimit;
                sharedCommit += queryStatistics.QueryResult.SegmentInformation.BytesCommitted;
                sharedUsage += queryStatistics.QueryResult.SegmentInformation.BytesResident;
            }
            else
            {
                dedicatedLimit += queryStatistics.QueryResult.SegmentInformation.CommitLimit;
                dedicatedCommit += queryStatistics.QueryResult.SegmentInformation.BytesCommitted;
                dedicatedUsage += queryStatistics.QueryResult.SegmentInformation.BytesResident;
            }
        }
        else
        {
            PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1 segmentInfo = (PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1)&queryStatistics.QueryResult;

            if (segmentInfo->Aperture)
            {
                sharedLimit += segmentInfo->CommitLimit;
                sharedCommit += segmentInfo->BytesCommitted;
                sharedUsage += segmentInfo->BytesResident;
            }
            else
            {
                dedicatedLimit += segmentInfo->CommitLimit;
                dedicatedCommit += segmentInfo->BytesCommitted;
                dedicatedUsage += segmentInfo->BytesResident;
            }
        }
    }

    if (SharedUsage)
        *SharedUsage = sharedUsage;
    if (SharedCommit)
        *SharedCommit = sharedCommit;
    if (SharedLimit)
        *SharedLimit = sharedLimit;
    if (DedicatedUsage)
        *DedicatedUsage = dedicatedUsage;
    if (DedicatedCommit)
        *DedicatedCommit = dedicatedCommit;
    if (DedicatedLimit)
        *DedicatedLimit = dedicatedLimit;

    return status;
}

/**
 * Queries node running time for an adapter node.
 *
 * \param AdapterLuid Adapter LUID.
 * \param NodeId Node ordinal.
 * \param RunningTime Receives the running time value.
 * \param SystemRunningTime Receives the total running time value.
 * \return NTSTATUS result of the query.
 */
NTSTATUS GraphicsQueryAdapterNodeRunningTime(
    _In_ LUID AdapterLuid,
    _In_ ULONG NodeId,
    _Out_ PULONG64 RunningTime,
    _Out_ PULONG64 SystemRunningTime
    )
{
    NTSTATUS status;
    D3DKMT_QUERYSTATISTICS queryStatistics;

    memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
    queryStatistics.Type = D3DKMT_QUERYSTATISTICS_NODE;
    queryStatistics.AdapterLuid = AdapterLuid;
    queryStatistics.QueryNode.NodeId = NodeId;

    status = D3DKMTQueryStatistics(&queryStatistics);

    if (NT_SUCCESS(status))
    {
        *RunningTime = queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
        *SystemRunningTime = queryStatistics.QueryResult.NodeInformation.SystemInformation.RunningTime.QuadPart;
    }

    return status;
}

/**
 * Queries adapter-wide performance data.
 *
 * \param AdapterHandle Adapter handle.
 * \param PowerUsage Receives power usage.
 * \param Temperature Receives temperature.
 * \param FanRPM Receives fan speed.
 * \return NTSTATUS result of the query.
 */
NTSTATUS GraphicsQueryAdapterDevicePerfData(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _Out_ PFLOAT PowerUsage,
    _Out_ PFLOAT Temperature,
    _Out_ PULONG FanRPM
    )
{
    NTSTATUS status;
    D3DKMT_ADAPTER_PERFDATA adapterPerfData;

    memset(&adapterPerfData, 0, sizeof(D3DKMT_ADAPTER_PERFDATA));

    status = GraphicsQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_ADAPTERPERFDATA,
        &adapterPerfData,
        sizeof(D3DKMT_ADAPTER_PERFDATA)
        );

    if (NT_SUCCESS(status))
    {
        //*PowerUsage = (((FLOAT)adapterPerfData.Power / 1000) * 100);
        //*Temperature = (((FLOAT)adapterPerfData.Temperature / 1000) * 100);
        *PowerUsage = (FLOAT)adapterPerfData.Power / 10;
        *Temperature = (FLOAT)adapterPerfData.Temperature / 10;
        *FanRPM = adapterPerfData.FanRPM;
    }

    return status;
}

/**
 * Queries performance data for a specific adapter node.
 *
 * \param AdapterHandle Adapter handle.
 * \param NodeOrdinal Node ordinal.
 * \param Frequency Receives the current frequency.
 * \param MaxFrequency Receives the maximum frequency.
 * \param MaxFrequencyOC Receives the overclocked maximum frequency.
 * \param Voltage Receives the current voltage.
 * \param VoltageMax Receives the maximum voltage.
 * \param VoltageMaxOC Receives the overclocked maximum voltage.
 * \param MaxTransitionLatency Receives the transition latency.
 * \return NTSTATUS result of the query.
 */
NTSTATUS GraphicsQueryAdapterDeviceNodePerfData(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ ULONG NodeOrdinal,
    _Out_opt_ PULONG64 Frequency,
    _Out_opt_ PULONG64 MaxFrequency,
    _Out_opt_ PULONG64 MaxFrequencyOC,
    _Out_opt_ PULONG Voltage,
    _Out_opt_ PULONG VoltageMax,
    _Out_opt_ PULONG VoltageMaxOC,
    _Out_opt_ PULONG64 MaxTransitionLatency
    )
{
    NTSTATUS status;
    D3DKMT_NODE_PERFDATA nodePerfData;

    memset(&nodePerfData, 0, sizeof(D3DKMT_NODE_PERFDATA));
    nodePerfData.NodeOrdinal = NodeOrdinal;
    nodePerfData.PhysicalAdapterIndex = 0;

    status = GraphicsQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_NODEPERFDATA,
        &nodePerfData,
        sizeof(D3DKMT_NODE_PERFDATA)
        );

    if (NT_SUCCESS(status))
    {
        if (Frequency) *Frequency = nodePerfData.Frequency;
        if (MaxFrequency) *MaxFrequency = nodePerfData.MaxFrequency;
        if (MaxFrequencyOC) *MaxFrequencyOC = nodePerfData.MaxFrequencyOC;
        if (Voltage) *Voltage = nodePerfData.Voltage;
        if (VoltageMax) *VoltageMax = nodePerfData.VoltageMax;
        if (VoltageMaxOC) *VoltageMaxOC = nodePerfData.VoltageMaxOC;
        if (MaxTransitionLatency) *MaxTransitionLatency = nodePerfData.MaxTransitionLatency;
    }

    return status;
}

/**
 * Probes VidPN source IDs starting from zero until the query fails,
 * yielding the number of active VidPN sources on the adapter.
 *
 * \param AdapterLuid Adapter LUID.
 * \param NumberOfVidPnSources Receives the source count (may be zero
 *        for headless or compute-only adapters).
 * \return STATUS_SUCCESS if at least one source exists,
 *         STATUS_NOT_FOUND if none do.
 */
NTSTATUS GraphicsQueryAdapterVidPnSourceCount(
    _In_ LUID AdapterLuid,
    _Out_ PULONG NumberOfVidPnSources
    )
{
    ULONG count = 0;
    D3DKMT_QUERYSTATISTICS queryStatistics;

    for (ULONG i = 0; i < 16; i++)
    {
        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_VIDPNSOURCE;
        queryStatistics.AdapterLuid = AdapterLuid;
        queryStatistics.QueryVidPnSource.VidPnSourceId = i;

        if (!NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            break;

        count++;
    }

    *NumberOfVidPnSources = count;
    return count > 0 ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

/**
 * Queries present frame statistics for a single VidPN source.
 *
 * The Frame counter is a 32-bit monotonically increasing value that
 * wraps at 2^32. Callers that track deltas should use ULONG arithmetic
 * so that wraparound is handled correctly.
 *
 * \param AdapterLuid Adapter LUID.
 * \param VidPnSourceId VidPN source ordinal (0-based).
 * \param FrameCount Receives the total frame present count (Blt + Flip).
 * \param CancelledFrameCount Receives the cancelled frame count (Flip only, optional).
 * \param QueuedPresent Receives the queued present count (optional).
 * \return NTSTATUS result of the query.
 */
NTSTATUS GraphicsQueryAdapterVidPnSourceFrames(
    _In_ LUID AdapterLuid,
    _In_ ULONG VidPnSourceId,
    _Out_ PULONG FrameCount,
    _Out_opt_ PULONG CancelledFrameCount,
    _Out_opt_ PULONG QueuedPresent
    )
{
    NTSTATUS status;
    D3DKMT_QUERYSTATISTICS queryStatistics;

    memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
    queryStatistics.Type = D3DKMT_QUERYSTATISTICS_VIDPNSOURCE;
    queryStatistics.AdapterLuid = AdapterLuid;
    queryStatistics.QueryVidPnSource.VidPnSourceId = VidPnSourceId;

    status = D3DKMTQueryStatistics(&queryStatistics);

    if (NT_SUCCESS(status))
    {
        *FrameCount = queryStatistics.QueryResult.VidPnSourceInformation.GlobalInformation.Frame;

        if (CancelledFrameCount)
            *CancelledFrameCount = queryStatistics.QueryResult.VidPnSourceInformation.GlobalInformation.CancelledFrame;

        if (QueuedPresent)
            *QueuedPresent = queryStatistics.QueryResult.VidPnSourceInformation.GlobalInformation.QueuedPresent;
    }

    return status;
}

/**
 * Drains the adapter's present history ring buffer in batches and
 * accumulates per-VidPN-source and adapter-level token statistics.
 *
 * The ring buffer is shared and destructive: each token read here is
 * consumed and will not be returned to any other reader. Callers should
 * drain on every update tick to avoid ring overflow at high frame rates.
 *
 * \param AdapterHandle Open adapter handle (hAdapter from D3DKMTOpenAdapter*).
 * \param Buffer  Pre-allocated scratch buffer; must be at least BufferSize bytes.
 * \param BufferSize  Byte capacity of Buffer. Should be a multiple of
 *        sizeof(D3DKMT_PRESENTHISTORYTOKEN). Determines how many tokens are
 *        fetched per kernel round-trip.
 * \param NumberOfVidPnSources  Length of the VidPnSourceStats array.
 * \param VidPnSourceStats  Per-source accumulator array (indexed by VidPnSourceId).
 *        Flip tokens whose VidPnSourceId is in range are accumulated here.
 *        Pass NULL to skip per-source accounting.
 * \param AdapterStats  Adapter-level accumulator for tokens not attributed to a
 *        specific source (BLT/GDI/FlipManager) and out-of-range source IDs.
 *        Pass NULL to skip adapter-level accounting.
 */
VOID GraphicsQueryAdapterPresentHistory(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _In_ ULONG NumberOfVidPnSources,
    _Inout_opt_ PGX_PRESENT_STATS VidPnSourceStats,
    _Inout_opt_ PGX_PRESENT_STATS AdapterStats
    )
{
    D3DKMT_GETPRESENTHISTORY getPresentHistory;

    for (;;)
    {
        memset(Buffer, 0, BufferSize);
        memset(&getPresentHistory, 0, sizeof(D3DKMT_GETPRESENTHISTORY));
        getPresentHistory.hAdapter = AdapterHandle;
        getPresentHistory.ProvidedSize = BufferSize;
        getPresentHistory.pTokens = (D3DKMT_PRESENTHISTORYTOKEN*)Buffer;

        if (!NT_SUCCESS(D3DKMTGetPresentHistory(&getPresentHistory)) ||
            getPresentHistory.NumTokens == 0)
        {
            break;
        }

        // Walk the variable-length token buffer using TokenSize for each step.
        D3DKMT_PRESENTHISTORYTOKEN* token = (D3DKMT_PRESENTHISTORYTOKEN*)Buffer;

        for (ULONG i = 0; i < getPresentHistory.NumTokens; i++)
        {
            // Safety: a zero TokenSize would produce an infinite loop.
            if (token->TokenSize == 0)
                break;

            // Route flip-model tokens to per-source stats; everything else
            // goes to the adapter-level bucket.
            PGX_PRESENT_STATS target = AdapterStats;

            if (VidPnSourceStats && token->Model == D3DKMT_PM_REDIRECTED_FLIP)
            {
                ULONG sourceId = token->Token.Flip.VidPnSourceId;
                if (sourceId < NumberOfVidPnSources)
                    target = &VidPnSourceStats[sourceId];
            }

            if (target)
            {
                switch (token->Model)
                {
                case D3DKMT_PM_REDIRECTED_FLIP:
                    {
                        D3DKMT_FLIPMODEL_PRESENTHISTORYTOKENFLAGS flags = token->Token.Flip.Flags;
                        D3DDDI_FLIPINTERVAL_TYPE interval = token->Token.Flip.FlipInterval;

                        target->RedirectedFlipCount++;

                        if (flags.IndependentFlip)
                            target->IndependentFlipCount++;
                        if (flags.FlipRestart)
                            target->FlipRestartCount++;
                        if (flags.VariableRefreshOverrideEligible)
                            target->VrrEligibleCount++;

                        if (interval == D3DDDI_FLIPINTERVAL_IMMEDIATE ||
                            interval == D3DDDI_FLIPINTERVAL_IMMEDIATE_ALLOW_TEARING)
                            target->Interval0Count++;
                        else if (interval == D3DDDI_FLIPINTERVAL_ONE)
                            target->Interval1Count++;
                        else
                            target->Interval2PlusCount++;
                    }
                    break;

                case D3DKMT_PM_REDIRECTED_BLT:
                case D3DKMT_PM_REDIRECTED_GDI:
                case D3DKMT_PM_REDIRECTED_GDI_SYSMEM:
                case D3DKMT_PM_REDIRECTED_VISTABLT:
                    target->RedirectedBltCount++;
                    break;

                case D3DKMT_PM_REDIRECTED_COMPOSITION:
                case D3DKMT_PM_SURFACECOMPLETE:
                    target->CompositionCount++;
                    break;

                case D3DKMT_PM_FLIPMANAGER:
                    // FlipManager tokens (DX12 / new flip manager) carry
                    // opaque hPrivateData — no VidPnSourceId is available
                    // in the token itself, so these always go to AdapterStats.
                    target->FlipManagerCount++;
                    break;
                }
            }

            token = (D3DKMT_PRESENTHISTORYTOKEN*)((PUCHAR)token + token->TokenSize);
        }
    }
}

/**
 * Formats a DEVPROPERTY value as a display string.
 *
 * \param Property Property record (may be NULL).
 * \return Formatted string, or NULL if the property is missing or unsupported.
 */
static PPH_STRING GraphicsFormatDeviceProperty(
    _In_opt_ const DEVPROPERTY* Property
    )
{
    if (!Property || !Property->Buffer)
        return NULL;

    switch (Property->Type)
    {
    case DEVPROP_TYPE_STRING:
        {
            SIZE_T length;

            if (Property->BufferSize < sizeof(UNICODE_NULL))
                return NULL;

            length = Property->BufferSize;
            if (((PWSTR)Property->Buffer)[(Property->BufferSize / sizeof(WCHAR)) - 1] == UNICODE_NULL)
                length -= sizeof(UNICODE_NULL);

            return PhCreateStringEx((PWCHAR)Property->Buffer, length);
        }
    case DEVPROP_TYPE_FILETIME:
        {
            PFILETIME fileTime;
            LARGE_INTEGER time;
            SYSTEMTIME systemTime;

            if (Property->BufferSize < sizeof(FILETIME))
                return NULL;

            fileTime = (PFILETIME)Property->Buffer;
            time.HighPart = fileTime->dwHighDateTime;
            time.LowPart = fileTime->dwLowDateTime;
            PhLargeIntegerToLocalSystemTime(&systemTime, &time);

            return PhFormatDate(&systemTime, NULL);
        }
    case DEVPROP_TYPE_UINT32:
        if (Property->BufferSize >= sizeof(ULONG))
            return PhFormatUInt64(*(PULONG)Property->Buffer, FALSE);
        return NULL;
    case DEVPROP_TYPE_UINT64:
        if (Property->BufferSize >= sizeof(ULONG64))
            return PhFormatUInt64(*(PULONG64)Property->Buffer, FALSE);
        return NULL;
    }

    return NULL;
}

/**
 * Resolves a device interface to its parent device instance ID string.
 *
 * \param DeviceInterface Device interface path.
 * \return Allocated PPH_STRING containing the instance ID, or NULL on failure.
 *         Caller must PhDereferenceObject the result.
 */
_Success_(return != NULL)
static PPH_STRING GraphicsQueryDeviceInterfaceInstanceId(
    _In_ PCWSTR DeviceInterface
    )
{
    PPH_STRING instanceId = NULL;
    DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
    };
    ULONG propertyCount = 0;
    const DEVPROPERTY* properties = NULL;

    if (HR_SUCCESS(PhDevGetObjectProperties(
        DevObjectTypeDeviceInterface,
        DeviceInterface,
        DevQueryFlagNone,
        RTL_NUMBER_OF(requestedProperties),
        requestedProperties,
        &propertyCount,
        &properties
        )))
    {
        const DEVPROPERTY* instanceProperty = PhDevFindProperty(
            &DEVPKEY_Device_InstanceId,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties
            );

        if (
            instanceProperty &&
            instanceProperty->Type == DEVPROP_TYPE_STRING &&
            instanceProperty->Buffer &&
            instanceProperty->BufferSize >= sizeof(UNICODE_NULL)
            )
        {
            SIZE_T length = instanceProperty->BufferSize;

            if (((PWSTR)instanceProperty->Buffer)[(instanceProperty->BufferSize / sizeof(WCHAR)) - 1] == UNICODE_NULL)
            {
                length -= sizeof(UNICODE_NULL);
            }

            instanceId = PhCreateStringEx(instanceProperty->Buffer, length);
        }

        PhDevFreeObjectProperties(propertyCount, properties);
    }

    return instanceId;
}

/**
 * Queries a display adapter description from a device instance ID.
 *
 * \param DeviceInstanceId Device instance ID string.
 * \return Adapter description string.
 */
PPH_STRING GraphicsQueryDeviceDescription(
    _In_ PCWSTR DeviceInstanceId
    )
{
    static const PH_STRINGREF defaultName = PH_STRINGREF_INIT(L"Unknown Adapter");
    PPH_STRING string = NULL;
    const DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Device_DeviceDesc, DEVPROP_STORE_SYSTEM, NULL },
    };
    ULONG propertyCount = 0;
    const DEVPROPERTY* properties = NULL;

    if (HR_SUCCESS(PhDevGetObjectProperties(
        DevObjectTypeDevice,
        DeviceInstanceId,
        DevQueryFlagNone,
        RTL_NUMBER_OF(requestedProperties),
        requestedProperties,
        &propertyCount,
        &properties
        )))
    {
        string = GraphicsFormatDeviceProperty(PhDevFindProperty(
            &DEVPKEY_Device_DeviceDesc,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties
            ));

        PhDevFreeObjectProperties(propertyCount, properties);
    }

    if (PhIsNullOrEmptyString(string))
    {
        PhClearReference(&string);
        string = PhCreateString2(&defaultName);
    }

    return string;
}

/**
 * Queries a display adapter description from a device interface path.
 *
 * \param DeviceInterface Device interface path.
 * \return Adapter description string.
 */
PPH_STRING GraphicsQueryDeviceInterfaceDescription(
    _In_opt_ PWSTR DeviceInterface
    )
{
    if (DeviceInterface)
    {
        PPH_STRING instanceId = GraphicsQueryDeviceInterfaceInstanceId(DeviceInterface);

        if (instanceId)
        {
            PPH_STRING description = GraphicsQueryDeviceDescription(PhGetString(instanceId));
            PhDereferenceObject(instanceId);
            return description;
        }
    }

    {
        static const PH_STRINGREF defaultName = PH_STRINGREF_INIT(L"Unknown Adapter");
        return PhCreateString2(&defaultName);
    }
}

/**
 * Queries the installed memory value for a graphics adapter.
 *
 * \param DeviceInstanceId Device instance ID string.
 * \return Installed memory size, or ULLONG_MAX when unavailable.
 */
static ULONG64 GraphicsQueryInstalledMemory(
    _In_ PPH_STRING DeviceInstanceId
    )
{
    ULONG64 installedMemory;
    HANDLE keyHandle;

    if (!NT_SUCCESS(PhDevOpenObjectKey(
        DeviceInstanceId,
        KEY_READ,
        PH_DEVKEY_SOFTWARE,
        &keyHandle
        )))
    {
        return ULLONG_MAX;
    }

    installedMemory = PhQueryRegistryUlong64Z(keyHandle, L"HardwareInformation.qwMemorySize");

    if (installedMemory == ULLONG_MAX)
        installedMemory = PhQueryRegistryUlongZ(keyHandle, L"HardwareInformation.MemorySize");

    if (installedMemory == ULONG_MAX) // HACK
        installedMemory = ULLONG_MAX;

    // Intel GPU devices incorrectly create the key with type REG_BINARY.
    if (installedMemory == ULLONG_MAX)
    {
        static PH_STRINGREF valueName = PH_STRINGREF_INIT(L"HardwareInformation.MemorySize");
        PKEY_VALUE_PARTIAL_INFORMATION buffer;

        if (NT_SUCCESS(PhQueryValueKey(keyHandle, &valueName, KeyValuePartialInformation, &buffer)))
        {
            if (buffer->Type == REG_BINARY && buffer->DataLength == sizeof(ULONG))
                installedMemory = *(PULONG)buffer->Data;

            PhFree(buffer);
        }
    }

    NtClose(keyHandle);
    return installedMemory;
}

/**
 * Queries a set of common device properties for a graphics adapter.
 *
 * \param DeviceInterface Device interface path.
 * \param Description Receives the device description.
 * \param DriverDate Receives the driver date.
 * \param DriverVersion Receives the driver version.
 * \param LocationInfo Receives the location string.
 * \param InstalledMemory Receives installed memory.
 * \param AdapterLuid Receives the adapter LUID.
 * \return TRUE if the device was resolved successfully.
 */
_Success_(return)
BOOLEAN GraphicsQueryDeviceProperties(
    _In_ PCWSTR DeviceInterface,
    _Out_opt_ PPH_STRING* Description,
    _Out_opt_ PPH_STRING* DriverDate,
    _Out_opt_ PPH_STRING* DriverVersion,
    _Out_opt_ PPH_STRING* LocationInfo,
    _Out_opt_ PULONG64 InstalledMemory,
    _Out_opt_ LUID* AdapterLuid
    )
{
    PPH_STRING instanceId;
    DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Device_DeviceDesc,    DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_DriverDate,    DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_DriverVersion, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_LocationInfo,  DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Gpu_Luid,             DEVPROP_STORE_SYSTEM, NULL },
    };
    ULONG propertyCount = 0;
    const DEVPROPERTY* properties = NULL;

    instanceId = GraphicsQueryDeviceInterfaceInstanceId(DeviceInterface);
    if (!instanceId)
        return FALSE;

    if (HR_FAILED(PhDevGetObjectProperties(
        DevObjectTypeDevice,
        PhGetString(instanceId),
        DevQueryFlagNone,
        RTL_NUMBER_OF(requestedProperties),
        requestedProperties,
        &propertyCount,
        &properties
        )))
    {
        PhDereferenceObject(instanceId);
        return FALSE;
    }

    if (Description)
    {
        *Description = GraphicsFormatDeviceProperty(PhDevFindProperty(
            &DEVPKEY_Device_DeviceDesc,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties));
    }

    if (DriverDate)
    {
        *DriverDate = GraphicsFormatDeviceProperty(PhDevFindProperty(
            &DEVPKEY_Device_DriverDate,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties));
    }

    if (DriverVersion)
    {
        *DriverVersion = GraphicsFormatDeviceProperty(PhDevFindProperty(
            &DEVPKEY_Device_DriverVersion,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties));
    }

    if (LocationInfo)
    {
        *LocationInfo = GraphicsFormatDeviceProperty(PhDevFindProperty(
            &DEVPKEY_Device_LocationInfo,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties));
    }

    if (InstalledMemory)
    {
        *InstalledMemory = GraphicsQueryInstalledMemory(instanceId);
    }

    if (AdapterLuid)
    {
        const DEVPROPERTY* gpuAdapterProperty = PhDevFindProperty(
            &DEVPKEY_Gpu_Luid,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties);

        if (
            gpuAdapterProperty &&
            gpuAdapterProperty->Type == DEVPROP_TYPE_UINT64 &&
            gpuAdapterProperty->Buffer &&
            gpuAdapterProperty->BufferSize >= sizeof(LUID)
            )
        {
            memcpy(AdapterLuid, gpuAdapterProperty->Buffer, sizeof(LUID));
        }
        else
        {
            memset(AdapterLuid, 0, sizeof(LUID));
        }
    }

    PhDevFreeObjectProperties(propertyCount, properties);
    PhDereferenceObject(instanceId);
    return TRUE;
}

/**
 * Queries the LUID associated with a graphics device interface.
 *
 * \param DeviceInterface Device interface path.
 * \param AdapterLuid Receives the adapter LUID.
 * \return TRUE if the LUID was queried successfully.
 */
_Success_(return)
BOOLEAN GraphicsQueryDeviceInterfaceLuid(
    _In_ PCWSTR DeviceInterface,
    _Out_ LUID* AdapterLuid
    )
{
    const DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Gpu_Luid, DEVPROP_STORE_SYSTEM, NULL },
    };
    PPH_STRING instanceId;
    ULONG propertyCount = 0;
    BOOLEAN success = FALSE;
    const DEVPROPERTY* properties = NULL;

    memset(AdapterLuid, 0, sizeof(LUID));

    instanceId = GraphicsQueryDeviceInterfaceInstanceId(DeviceInterface);
    if (!instanceId)
        return FALSE;

    if (HR_SUCCESS(PhDevGetObjectProperties(
        DevObjectTypeDevice,
        PhGetString(instanceId),
        DevQueryFlagNone,
        RTL_NUMBER_OF(requestedProperties),
        requestedProperties,
        &propertyCount,
        &properties
        )))
    {
        const DEVPROPERTY* gpuAdapterProperty = PhDevFindProperty(
            &DEVPKEY_Gpu_Luid,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties);

        if (
            gpuAdapterProperty &&
            gpuAdapterProperty->Type == DEVPROP_TYPE_UINT64 &&
            gpuAdapterProperty->Buffer &&
            gpuAdapterProperty->BufferSize >= sizeof(LUID)
            )
        {
            memcpy(AdapterLuid, gpuAdapterProperty->Buffer, sizeof(LUID));
            success = TRUE;
        }

        PhDevFreeObjectProperties(propertyCount, properties);
    }

    PhDereferenceObject(instanceId);
    return success;
}

/**
 * Computes a stable unique adapter index for display.
 *
 * \param DeviceInterface Device interface path.
 * \param DeviceInstanceId Device instance ID string.
 * \param PhysicalAdapterIndex Receives the unique adapter index.
 * \return TRUE if an index was produced successfully.
 */
_Success_(return)
static BOOLEAN GraphicsQueryDeviceInterfaceAdapterIndexUnique(
    _In_ PCWSTR DeviceInterface,
    _In_ PCWSTR DeviceInstanceId,
    _Out_ PULONG PhysicalAdapterIndex
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_LIST gpuLuids = NULL;
    static PPH_LIST npuLuids = NULL;
    const DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Gpu_Luid, DEVPROP_STORE_SYSTEM, NULL },
    };
    ULONG propertyCount = 0;
    const DEVPROPERTY* properties = NULL;
    LUID luid;
    D3DKMT_HANDLE adapterHandle;
    BOOLEAN npuDevice = FALSE;
    PPH_LIST list;
    ULONG index;

    *PhysicalAdapterIndex = 0;

    // DEVPKEY_Gpu_PhysicalAdapterIndex can be the same between different graphics devices.
    // This option makes them unique for each type we display, this mimics the behavior of similar
    // tools that display information about graphics devices. Note this has nothing to do with
    // NPU vs GPU, for example on laptops with an integrated and dedicated GPU they might show
    // the same index in the UI, this resolves that.

    if (PhBeginInitOnce(&initOnce))
    {
        gpuLuids = PhCreateList(1);
        npuLuids = PhCreateList(1);

        PhEndInitOnce(&initOnce);
    }
    
    if (HR_FAILED(PhDevGetObjectProperties(
        DevObjectTypeDevice,
        DeviceInstanceId,
        DevQueryFlagNone,
        RTL_NUMBER_OF(requestedProperties),
        requestedProperties,
        &propertyCount,
        &properties
        )))
    {
        return FALSE;
    }

    {
        const DEVPROPERTY* instanceProperty = PhDevFindProperty(
            &DEVPKEY_Gpu_Luid,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties);

        if (
            !instanceProperty ||
            instanceProperty->Type != DEVPROP_TYPE_UINT64 ||
            !instanceProperty->Buffer ||
            instanceProperty->BufferSize < sizeof(LUID)
            )
        {
            PhDevFreeObjectProperties(propertyCount, properties);
            return FALSE;
        }

        memcpy(&luid, instanceProperty->Buffer, sizeof(LUID));
    }

    PhDevFreeObjectProperties(propertyCount, properties);

    if (NT_SUCCESS(GraphicsOpenAdapterFromDeviceName(&adapterHandle, NULL, (PWSTR)DeviceInterface)))
    {
        GX_ADAPTER_ATTRIBUTES adapterAttributes;

        if (NT_SUCCESS(GraphicsQueryAdapterAttributes(
            adapterHandle,
            &adapterAttributes
            )))
        {
            npuDevice = !!adapterAttributes.TypeNpu;
        }

        GraphicsCloseAdapterHandle(adapterHandle);
    }

    list = npuDevice ? npuLuids : gpuLuids;

    PhAcquireQueuedLockExclusive(&GraphicsDeviceInterfaceAdapterIndexLock);

    if (!GraphicsFindCachedAdapterLuid(list, &luid, &index))
    {
        PLUID luidEntry = PhAllocate(sizeof(LUID));

        *luidEntry = luid;
        PhAddItemList(list, luidEntry);
        index = list->Count - 1;
    }

    *PhysicalAdapterIndex = index;
    PhReleaseQueuedLockExclusive(&GraphicsDeviceInterfaceAdapterIndexLock);

    return TRUE;
}

/**
 * Queries the adapter index associated with a graphics device interface.
 *
 * \param DeviceInterface Device interface path.
 * \param PhysicalAdapterIndex Receives the adapter index.
 * \return TRUE if an adapter index was found.
 */
_Success_(return)
BOOLEAN GraphicsQueryDeviceInterfaceAdapterIndex(
    _In_ PCWSTR DeviceInterface,
    _Out_ PULONG PhysicalAdapterIndex
    )
{
    PPH_STRING instanceId;
    DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Gpu_PhysicalAdapterIndex, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_LocationInfo,      DEVPROP_STORE_SYSTEM, NULL },
    };
    ULONG propertyCount = 0;
    const DEVPROPERTY* properties = NULL;
    BOOLEAN success = FALSE;

    instanceId = GraphicsQueryDeviceInterfaceInstanceId(DeviceInterface);
    if (!instanceId)
        return FALSE;

    if (PhGetIntegerSetting(SETTING_NAME_GRAPHICS_UNIQUE_INDICES))
    {
        if (GraphicsQueryDeviceInterfaceAdapterIndexUnique(
            DeviceInterface,
            PhGetString(instanceId),
            PhysicalAdapterIndex
            ))
        {
            PhDereferenceObject(instanceId);
            return TRUE;
        }
    }

    if (HR_FAILED(PhDevGetObjectProperties(
        DevObjectTypeDevice,
        PhGetString(instanceId),
        DevQueryFlagNone,
        RTL_NUMBER_OF(requestedProperties),
        requestedProperties,
        &propertyCount,
        &properties
        )))
    {
        PhDereferenceObject(instanceId);
        return FALSE;
    }

    if (NetWindowsVersion >= WINDOWS_10)
    {
        const DEVPROPERTY* instanceProperty = PhDevFindProperty(
            &DEVPKEY_Gpu_PhysicalAdapterIndex,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties
            );

        if (
            instanceProperty &&
            instanceProperty->Type == DEVPROP_TYPE_UINT32 &&
            instanceProperty->Buffer &&
            instanceProperty->BufferSize >= sizeof(ULONG)
            )
        {
            *PhysicalAdapterIndex = *(PULONG)instanceProperty->Buffer;
            success = TRUE;
        }
    }

    if (!success)
    {
        PPH_STRING locationString;

        locationString = GraphicsFormatDeviceProperty(PhDevFindProperty(
            &DEVPKEY_Device_LocationInfo,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties));

        if (locationString)
        {
            PPH_STRING deviceString = NULL;
            ULONG_PTR deviceIndex;
            ULONG_PTR deviceIndexLength;
            ULONG64 index;

            if ((deviceIndex = PhFindStringInString(locationString, 0, L"device ")) != SIZE_MAX &&
                (deviceIndexLength = PhFindStringInString(locationString, deviceIndex, L",")) != SIZE_MAX &&
                (deviceIndexLength = deviceIndexLength - deviceIndex) != 0)
            {
                deviceString = PhSubstring(
                    locationString,
                    deviceIndex + (RTL_NUMBER_OF(L"device ") - 1),
                    deviceIndexLength - (RTL_NUMBER_OF(L"device ") - 1)
                    );

                if (PhStringToInteger64(&deviceString->sr, 10, &index))
                {
                    *PhysicalAdapterIndex = (ULONG)index;
                    success = TRUE;
                }
            }

            PhClearReference(&deviceString);
            PhDereferenceObject(locationString);
        }
    }

    PhDevFreeObjectProperties(propertyCount, properties);
    PhDereferenceObject(instanceId);
    return success;
}

/**
 * Converts a GPU node engine type to a display string.
 *
 * \param NodeMetaData Node metadata record.
 * \return Engine type display string.
 */
PPH_STRING GraphicsGetNodeEngineTypeString(
    _In_ D3DKMT_NODEMETADATA NodeMetaData
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_STRING name3D = NULL;
    static PPH_STRING decode = NULL;
    static PPH_STRING encode = NULL;
    static PPH_STRING processing = NULL;
    static PPH_STRING assembly = NULL;
    static PPH_STRING copy = NULL;
    static PPH_STRING overlay = NULL;
    static PPH_STRING crypto = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        static CONST PH_STRINGREF name3DString = PH_STRINGREF_INIT(L"3D");
        static CONST PH_STRINGREF nameDecodeString = PH_STRINGREF_INIT(L"Video Decode");
        static CONST PH_STRINGREF nameEncodeString = PH_STRINGREF_INIT(L"Video Encode");
        static CONST PH_STRINGREF nameProcessingString = PH_STRINGREF_INIT(L"Video Processing");
        static CONST PH_STRINGREF nameAssemblyString = PH_STRINGREF_INIT(L"Scene Assembly");
        static CONST PH_STRINGREF nameCopyString = PH_STRINGREF_INIT(L"Copy");
        static CONST PH_STRINGREF nameOverlayString = PH_STRINGREF_INIT(L"Overlay");
        static CONST PH_STRINGREF nameCryptoString = PH_STRINGREF_INIT(L"Crypto");

        name3D = PhCreateString2(&name3DString);
        decode = PhCreateString2(&nameDecodeString);
        encode = PhCreateString2(&nameEncodeString);
        processing = PhCreateString2(&nameProcessingString);
        assembly = PhCreateString2(&nameAssemblyString);
        copy = PhCreateString2(&nameCopyString);
        overlay = PhCreateString2(&nameOverlayString);
        crypto = PhCreateString2(&nameCryptoString);

        PhEndInitOnce(&initOnce);
    }

    switch (NodeMetaData.NodeData.EngineType)
    {
    case DXGK_ENGINE_TYPE_OTHER:
        return PhCreateString(NodeMetaData.NodeData.FriendlyName);
    case DXGK_ENGINE_TYPE_3D:
        return PhReferenceObject(name3D);
    case DXGK_ENGINE_TYPE_VIDEO_DECODE:
        return PhReferenceObject(decode);
    case DXGK_ENGINE_TYPE_VIDEO_ENCODE:
        return PhReferenceObject(encode);
    case DXGK_ENGINE_TYPE_VIDEO_PROCESSING:
        return PhReferenceObject(processing);
    case DXGK_ENGINE_TYPE_SCENE_ASSEMBLY:
        return PhReferenceObject(assembly);
    case DXGK_ENGINE_TYPE_COPY:
        return PhReferenceObject(copy);
    case DXGK_ENGINE_TYPE_OVERLAY:
        return PhReferenceObject(overlay);
    case DXGK_ENGINE_TYPE_CRYPTO:
        return PhReferenceObject(crypto);
    default:
        return PhCreateString(L"ERROR");
    }
}

/**
 * Queries display names for all adapter nodes.
 *
 * \param AdapterHandle Adapter handle.
 * \param NodeCount Number of nodes to inspect.
 * \return List of node name strings.
 */
PPH_LIST GraphicsQueryDeviceNodeList(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ ULONG NodeCount
    )
{
    PPH_LIST NodeNameList = PhCreateList(NodeCount);

    for (ULONG i = 0; i < NodeCount; i++)
    {
        D3DKMT_NODEMETADATA metaDataInfo;

        memset(&metaDataInfo, 0, sizeof(D3DKMT_NODEMETADATA));
        metaDataInfo.NodeOrdinalAndAdapterIndex = MAKEWORD(i, 0);

        if (NT_SUCCESS(GraphicsQueryAdapterInformation(
            AdapterHandle,
            KMTQAITYPE_NODEMETADATA,
            &metaDataInfo,
            sizeof(D3DKMT_NODEMETADATA)
            )))
        {
            PhAddItemList(NodeNameList, GraphicsGetNodeEngineTypeString(metaDataInfo));
        }
        else
        {
            PhAddItemList(NodeNameList, PhReferenceEmptyString());
        }
    }

    return NodeNameList;
}

/**
 * Queries a string property from the adapter registry.
 *
 * \param AdapterHandle Adapter handle.
 * \param PropertyName Registry property name.
 * \param String Receives the property string.
 * \return NTSTATUS result of the query.
 */
NTSTATUS GraphicsQueryAdapterPropertyString(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ PCPH_STRINGREF PropertyName,
    _Out_ PPH_STRING* String
    )
{
    NTSTATUS status;
    ULONG regInfoSize;
    D3DDDI_QUERYREGISTRY_INFO* regInfo;

    *String = NULL;

    regInfoSize = sizeof(D3DDDI_QUERYREGISTRY_INFO) + 512;
    regInfo = PhAllocateZero(regInfoSize);
    regInfo->QueryType = D3DDDI_QUERYREGISTRY_ADAPTERKEY;
    regInfo->QueryFlags.TranslatePath = 1;
    regInfo->ValueType = REG_MULTI_SZ;

    memcpy(regInfo->ValueName, PropertyName->Buffer, PropertyName->Length);

    if (!NT_SUCCESS(status = GraphicsQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_QUERYREGISTRY,
        regInfo,
        regInfoSize
        )))
        goto CleanupExit;

    if (regInfo->Status == D3DDDI_QUERYREGISTRY_STATUS_BUFFER_OVERFLOW)
    {
        regInfoSize = regInfo->OutputValueSize;
        regInfo = PhReAllocate(regInfo, regInfoSize);

        if (!NT_SUCCESS(status = GraphicsQueryAdapterInformation(
            AdapterHandle,
            KMTQAITYPE_QUERYREGISTRY,
            regInfo,
            regInfoSize
            )))
            goto CleanupExit;
        }

    if (regInfo->Status != D3DDDI_QUERYREGISTRY_STATUS_SUCCESS)
    {
        status = STATUS_REGISTRY_IO_FAILED;
        goto CleanupExit;
    }

    *String = PhCreateStringEx(regInfo->OutputString, regInfo->OutputValueSize);

CleanupExit:

    PhFree(regInfo);

    return status;
}

/**
 * Queries DXCore/DX adapter attribute flags.
 *
 * \param AdapterHandle Adapter handle.
 * \param Attributes Receives the decoded attribute flags.
 * \return NTSTATUS result of the query.
 */
NTSTATUS GraphicsQueryAdapterAttributes(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _Out_ PGX_ADAPTER_ATTRIBUTES Attributes
    )
{
    static CONST PH_STRINGREF dxCoreAttributes = PH_STRINGREF_INIT(L"DXCoreAttributes");
    static CONST PH_STRINGREF dxAttributes = PH_STRINGREF_INIT(L"DXAttributes");
    NTSTATUS status;
    PPH_STRING adapterAttributes;

    Attributes->Flags = 0;

    // DXCoreAdapter::QueryAndFillAdapterExtendedProperiesOnPlatform
    if (!NT_SUCCESS(status = GraphicsQueryAdapterPropertyString(AdapterHandle, &dxCoreAttributes, &adapterAttributes)))
    {
        if (!NT_SUCCESS(status = GraphicsQueryAdapterPropertyString(AdapterHandle, &dxAttributes, &adapterAttributes)))
            return status;
    }

    for (PWCHAR attr = adapterAttributes->Buffer;;)
    {
        PH_STRINGREF attribute;
        GUID guid;

        PhInitializeStringRefLongHint(&attribute, attr);

        if (!attribute.Length)
            break;

        attr = PTR_ADD_OFFSET(attr, attribute.Length + sizeof(UNICODE_NULL));

        if (!NT_SUCCESS(status = PhStringToGuid(&attribute, &guid)))
            break;

        if (IsEqualGUID(&guid, &DXCORE_HARDWARE_TYPE_ATTRIBUTE_GPU))
        {
            Attributes->TypeGpu = TRUE;
            continue;
        }

        if (IsEqualGUID(&guid, &DXCORE_HARDWARE_TYPE_ATTRIBUTE_COMPUTE_ACCELERATOR))
        {
            Attributes->TypeComputeAccelerator = TRUE;
            continue;
        }

        if (IsEqualGUID(&guid, &DXCORE_HARDWARE_TYPE_ATTRIBUTE_NPU))
        {
            Attributes->TypeNpu = TRUE;
            continue;
        }

        if (IsEqualGUID(&guid, &DXCORE_HARDWARE_TYPE_ATTRIBUTE_MEDIA_ACCELERATOR))
        {
            Attributes->TypeMediaAccelerator = TRUE;
            continue;
        }

        if (IsEqualGUID(&guid, &DXCORE_ADAPTER_ATTRIBUTE_D3D11_GRAPHICS))
        {
            Attributes->D3D11Graphics = TRUE;
            continue;
        }

        if (IsEqualGUID(&guid, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS))
        {
            Attributes->D3D12Graphics = TRUE;
            continue;
        }

        if (IsEqualGUID(&guid, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE))
        {
            Attributes->D3D12CoreCompute = TRUE;
            continue;
        }

        if (IsEqualGUID(&guid, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GENERIC_ML))
        {
            Attributes->D3D12GenericML = TRUE;
            continue;
        }

        if (IsEqualGUID(&guid, &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GENERIC_MEDIA))
        {
            Attributes->D3D12GenericMedia = TRUE;
            continue;
        }

        if (IsEqualGUID(&guid, &DXCORE_ADAPTER_ATTRIBUTE_WSL))
        {
            Attributes->WSL = TRUE;
            continue;
        }
    }

    PhDereferenceObject(adapterAttributes);

    return status;
}
