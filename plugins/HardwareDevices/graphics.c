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
 * \return NTSTATUS result of the query.
 */
NTSTATUS GraphicsQueryAdapterNodeRunningTime(
    _In_ LUID AdapterLuid,
    _In_ ULONG NodeId,
    _Out_ PULONG64 RunningTime
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
        //queryStatistics.QueryResult.NodeInformation.SystemInformation.RunningTime.QuadPart;
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
 * Queries a display adapter description from a device instance.
 *
 * \param DeviceHandle Device instance handle.
 * \return Adapter description string.
 */
PPH_STRING GraphicsQueryDeviceDescription(
    _In_ DEVINST DeviceHandle
    )
{
    static const PH_STRINGREF defaultName = PH_STRINGREF_INIT(L"Unknown Adapter");
    PPH_STRING string;

    string = GraphicsQueryDevicePropertyString(
        DeviceHandle,
        &DEVPKEY_Device_DeviceDesc
        );

    if (PhIsNullOrEmptyString(string))
        return PhCreateString2((PPH_STRINGREF)&defaultName);
    else
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
        DEVPROPTYPE devicePropertyType;
        DEVINST deviceInstanceHandle;
        ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
        WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN + 1] = L"";

        if (CM_Get_Device_Interface_Property(
            DeviceInterface,
            &DEVPKEY_Device_InstanceId,
            &devicePropertyType,
            (PBYTE)deviceInstanceId,
            &deviceInstanceIdLength,
            0
            ) == CR_SUCCESS)
        {
            if (CM_Locate_DevNode(&deviceInstanceHandle, deviceInstanceId, CM_LOCATE_DEVNODE_NORMAL) == CR_SUCCESS)
            {
                return GraphicsQueryDeviceDescription(deviceInstanceHandle);
            }
        }
    }

    {
        static const PH_STRINGREF defaultName = PH_STRINGREF_INIT(L"Unknown Adapter");
        return PhCreateString2((PPH_STRINGREF)&defaultName);
    }
}

/**
 * Queries an arbitrary device property from a device instance.
 *
 * \param DeviceHandle Device instance handle.
 * \param DeviceProperty Property key to query.
 * \param PropertyType Receives the property type.
 * \param BufferLength Receives the buffer size.
 * \param Buffer Receives the allocated property buffer.
 * \return TRUE if the property query succeeded.
 */
_Success_(return)
BOOLEAN GraphicsQueryDevicePropertyKey(
    _In_ DEVINST DeviceHandle,
    _In_ CONST DEVPROPKEY* DeviceProperty,
    _Out_opt_ DEVPROPTYPE* PropertyType,
    _Out_opt_ PULONG BufferLength,
    _Out_opt_ PVOID* Buffer
    )
{
    CONFIGRET result;
    PBYTE buffer;
    ULONG bufferSize;
    DEVPROPTYPE propertyType;

    bufferSize = 0x80;
    buffer = PhAllocate(bufferSize);
    propertyType = DEVPROP_TYPE_EMPTY;

    if ((result = CM_Get_DevNode_Property(
        DeviceHandle,
        DeviceProperty,
        &propertyType,
        buffer,
        &bufferSize,
        0
        )) == CR_BUFFER_SMALL)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        result = CM_Get_DevNode_Property(
            DeviceHandle,
            DeviceProperty,
            &propertyType,
            buffer,
            &bufferSize,
            0
            );
    }

    if (result == CR_SUCCESS)
    {
        if (PropertyType)
            *PropertyType = propertyType;
        if (BufferLength)
            *BufferLength = bufferSize;
        if (Buffer)
            *Buffer = buffer;
        else
            PhFree(buffer);
        return TRUE;
    }

    PhFree(buffer);
    return FALSE;
}

/**
 * Queries a device property and converts it to a display string.
 *
 * \param DeviceHandle Device instance handle.
 * \param DeviceProperty Property key to query.
 * \return Formatted property string, or NULL on failure.
 */
PPH_STRING GraphicsQueryDevicePropertyString(
    _In_ DEVINST DeviceHandle,
    _In_ CONST DEVPROPKEY *DeviceProperty
    )
{
    DEVPROPTYPE propertyType = DEVPROP_TYPE_EMPTY;
    ULONG bufferSize;
    PBYTE buffer;

    if (!GraphicsQueryDevicePropertyKey(DeviceHandle, DeviceProperty, &propertyType, &bufferSize, &buffer))
        return NULL;

    switch (propertyType)
    {
    case DEVPROP_TYPE_STRING:
        {
            PPH_STRING string;

            string = PhCreateStringEx((PWCHAR)buffer, bufferSize);
            PhTrimToNullTerminatorString(string);

            PhFree(buffer);
            return string;
        }
        break;
    case DEVPROP_TYPE_FILETIME:
        {
            PPH_STRING string;
            PFILETIME fileTime;
            LARGE_INTEGER time;
            SYSTEMTIME systemTime;

            fileTime = (PFILETIME)buffer;
            time.HighPart = fileTime->dwHighDateTime;
            time.LowPart = fileTime->dwLowDateTime;

            PhLargeIntegerToLocalSystemTime(&systemTime, &time);

            string = PhFormatDate(&systemTime, NULL);

            //FILETIME newFileTime;
            //SYSTEMTIME systemTime;
            //
            //FileTimeToLocalFileTime((PFILETIME)buffer, &newFileTime);
            //FileTimeToSystemTime(&newFileTime, &systemTime);
            //
            //string = PhFormatDate(&systemTime, NULL);

            PhFree(buffer);
            return string;
        }
        break;
    case DEVPROP_TYPE_UINT32:
        {
            PPH_STRING string;

            string = PhFormatUInt64(*(PULONG)buffer, FALSE);

            PhFree(buffer);
            return string;
        }
        break;
    case DEVPROP_TYPE_UINT64:
        {
            PPH_STRING string;

            string = PhFormatUInt64(*(PULONG64)buffer, FALSE);

            PhFree(buffer);
            return string;
        }
        break;
    }

    PhFree(buffer);
    return NULL;
}

/**
 * Queries the installed memory value for a graphics adapter.
 *
 * \param DeviceHandle Device instance handle.
 * \return Installed memory size, or ULLONG_MAX when unavailable.
 */
ULONG64 GraphicsQueryInstalledMemory(
    _In_ DEVINST DeviceHandle
    )
{
    ULONG64 installedMemory = ULLONG_MAX;
    HKEY keyHandle;

    if (CM_Open_DevInst_Key(
        DeviceHandle,
        KEY_READ,
        0,
        RegDisposition_OpenExisting,
        &keyHandle,
        CM_REGISTRY_SOFTWARE
        ) == CR_SUCCESS)
    {
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
                if (buffer->Type == REG_BINARY)
                {
                    if (buffer->DataLength == sizeof(ULONG))
                        installedMemory = *(PULONG)buffer->Data;
                }

                PhFree(buffer);
            }
        }

        NtClose(keyHandle);
    }

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
        return FALSE;
    }

    if (CM_Locate_DevNode(
        &deviceInstanceHandle,
        deviceInstanceId,
        CM_LOCATE_DEVNODE_NORMAL
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    if (Description)
        *Description = GraphicsQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_DeviceDesc);
    if (DriverDate)
        *DriverDate = GraphicsQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_DriverDate);
    if (DriverVersion)
        *DriverVersion = GraphicsQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_DriverVersion);
    if (LocationInfo)
        *LocationInfo = GraphicsQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_LocationInfo);
    if (InstalledMemory)
        *InstalledMemory = GraphicsQueryInstalledMemory(deviceInstanceHandle);

    if (AdapterLuid)
    {
        DEVPROPTYPE propertyType = DEVPROP_TYPE_EMPTY;
        ULONG bufferSize;
        PBYTE buffer;

        if (GraphicsQueryDevicePropertyKey(deviceInstanceHandle, &DEVPKEY_Gpu_Luid, &propertyType, &bufferSize, &buffer))
        {
            if (propertyType == DEVPROP_TYPE_UINT64 && bufferSize >= sizeof(LUID))
                memcpy(AdapterLuid, buffer, sizeof(LUID));
            else
                memset(AdapterLuid, 0, sizeof(LUID));

            PhFree(buffer);
        }
        else
        {
            memset(AdapterLuid, 0, sizeof(LUID));
        }
    }

    //if (PhysicalAdapterIndex)
    //{
    //    DEVPROPTYPE propertyType = DEVPROP_TYPE_EMPTY;
    //    ULONG bufferSize;
    //    PBYTE buffer;
    //
    //    if (GraphicsQueryDevicePropertyKey(deviceInstanceHandle, &DEVPKEY_Gpu_PhysicalAdapterIndex, &propertyType, &bufferSize, &buffer))
    //    {
    //        PhysicalAdapterIndex = (PULONG)buffer;
    //    }
    //}

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
    PBYTE buffer;
    ULONG bufferSize;
    DEVPROPTYPE propertyType = DEVPROP_TYPE_EMPTY;
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
        return FALSE;
    }

    if (CM_Locate_DevNode(
        &deviceInstanceHandle,
        deviceInstanceId,
        CM_LOCATE_DEVNODE_NORMAL
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    if (!GraphicsQueryDevicePropertyKey(
        deviceInstanceHandle,
        &DEVPKEY_Gpu_Luid,
        &propertyType,
        &bufferSize,
        &buffer
        ))
    {
        return FALSE;
    }

    if (propertyType != DEVPROP_TYPE_UINT64 || bufferSize < sizeof(LUID))
    {
        PhFree(buffer);
        memset(AdapterLuid, 0, sizeof(LUID));
        return FALSE;
    }

    memcpy(AdapterLuid, buffer, sizeof(LUID));
    PhFree(buffer);
    return TRUE;
}

/**
 * Computes a stable unique adapter index for display.
 *
 * \param DeviceInstanceHandle Device instance handle.
 * \param DeviceInterface Device interface path.
 * \param PhysicalAdapterIndex Receives the unique adapter index.
 * \return TRUE if an index was produced successfully.
 */
_Success_(return)
BOOLEAN GraphicsQueryDeviceInterfaceAdapterIndexUnique(
    _In_ DEVINST DeviceInstanceHandle,
    _In_ PCWSTR DeviceInterface,
    _Out_ PULONG PhysicalAdapterIndex
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_LIST gpuLuids = NULL;
    static PPH_LIST npuLuids = NULL;
    LUID luid;
    ULONG luidSize;
    DEVPROPTYPE propertyType;
    D3DKMT_HANDLE adapterHandle;

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

    luidSize = sizeof(LUID);
    propertyType = DEVPROP_TYPE_EMPTY;

    if (CM_Get_DevNode_Property(
        DeviceInstanceHandle,
        &DEVPKEY_Gpu_Luid,
        &propertyType,
        (PBYTE)&luid,
        &luidSize,
        0
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    if (propertyType != DEVPROP_TYPE_UINT64 || luidSize < sizeof(LUID))
        return FALSE;

    BOOLEAN npuDevice = FALSE;

    if (NT_SUCCESS(GraphicsOpenAdapterFromDeviceName(&adapterHandle, NULL, (PWSTR)DeviceInterface)))
    {
        GX_ADAPTER_ATTRIBUTES adapterSttributes;

        if (NT_SUCCESS(GraphicsQueryAdapterAttributes(
            adapterHandle,
            &adapterSttributes
            )))
        {
            npuDevice = !!adapterSttributes.TypeNpu;
        }

        GraphicsCloseAdapterHandle(adapterHandle);
    }

    if (npuDevice)
    {
        ULONG index;

        PhAcquireQueuedLockExclusive(&GraphicsDeviceInterfaceAdapterIndexLock);

        if (!GraphicsFindCachedAdapterLuid(npuLuids, &luid, &index))
        {
            PLUID luidEntry = PhAllocate(sizeof(LUID));

            *luidEntry = luid;
            PhAddItemList(npuLuids, luidEntry);
            index = npuLuids->Count - 1;
        }

        *PhysicalAdapterIndex = index;
        PhReleaseQueuedLockExclusive(&GraphicsDeviceInterfaceAdapterIndexLock);
    }
    else
    {
        ULONG index;

        PhAcquireQueuedLockExclusive(&GraphicsDeviceInterfaceAdapterIndexLock);

        if (!GraphicsFindCachedAdapterLuid(gpuLuids, &luid, &index))
        {
            PLUID luidEntry = PhAllocate(sizeof(LUID));

            *luidEntry = luid;
            PhAddItemList(gpuLuids, luidEntry);
            index = gpuLuids->Count - 1;
        }

        *PhysicalAdapterIndex = index;
        PhReleaseQueuedLockExclusive(&GraphicsDeviceInterfaceAdapterIndexLock);
    }

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
        return FALSE;
    }

    if (CM_Locate_DevNode(
        &deviceInstanceHandle,
        deviceInstanceId,
        CM_LOCATE_DEVNODE_NORMAL
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    if (PhGetIntegerSetting(SETTING_NAME_GRAPHICS_UNIQUE_INDICES))
    {
        if (GraphicsQueryDeviceInterfaceAdapterIndexUnique(
            deviceInstanceHandle,
            DeviceInterface,
            PhysicalAdapterIndex
            ))
        {
            return TRUE;
        }
    }

    if (NetWindowsVersion >= WINDOWS_10)
    {
        ULONG adapterIndex;
        ULONG adapterIndexSize;
        DEVPROPTYPE propertyType;

        adapterIndexSize = sizeof(ULONG);
        propertyType = DEVPROP_TYPE_EMPTY;

        if (CM_Get_DevNode_Property(
            deviceInstanceHandle,
            &DEVPKEY_Gpu_PhysicalAdapterIndex,
            &propertyType,
            (PBYTE)&adapterIndex,
            &adapterIndexSize,
            0
            ) == CR_SUCCESS)
        {
            if (
                propertyType == DEVPROP_TYPE_UINT32 &&
                adapterIndexSize == sizeof(ULONG)
                )
            {
                *PhysicalAdapterIndex = adapterIndex;
                return TRUE;
            }
        }
    }

    {
        PPH_STRING deviceString = NULL;
        PPH_STRING locationString;
        ULONG_PTR deviceIndex;
        ULONG_PTR deviceIndexLength;
        ULONG64 index;

        locationString = GraphicsQueryDevicePropertyString(
            deviceInstanceHandle,
            &DEVPKEY_Device_LocationInfo
            );

        if (!locationString)
            return FALSE;

        if ((deviceIndex = PhFindStringInString(locationString, 0, L"device ")) == SIZE_MAX)
            goto CleanupExit;
        if ((deviceIndexLength = PhFindStringInString(locationString, deviceIndex, L",")) == SIZE_MAX)
            goto CleanupExit;
        if ((deviceIndexLength = deviceIndexLength - deviceIndex) == 0)
            goto CleanupExit;

        deviceString = PhSubstring(
            locationString,
            deviceIndex + (RTL_NUMBER_OF(L"device ") - 1),
            deviceIndexLength - (RTL_NUMBER_OF(L"device ") - 1)
            );

        if (PhStringToInteger64(&deviceString->sr, 10, &index))
        {
            PhDereferenceObject(deviceString);
            PhDereferenceObject(locationString);

            *PhysicalAdapterIndex = (ULONG)index;
            return TRUE;
        }

    CleanupExit:
        PhClearReference(&deviceString);
        PhClearReference(&locationString);
    }

    return FALSE;
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
