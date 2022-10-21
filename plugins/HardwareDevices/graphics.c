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

NTSTATUS GraphicsOpenAdapterFromDeviceName(
    _Out_ D3DKMT_HANDLE* AdapterHandle,
    _Out_opt_ PLUID AdapterLuid,
    _In_ PWSTR DeviceName
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

BOOLEAN GraphicsCloseAdapterHandle(
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_CLOSEADAPTER closeAdapter;

    memset(&closeAdapter, 0, sizeof(D3DKMT_CLOSEADAPTER));
    closeAdapter.hAdapter = AdapterHandle;

    return NT_SUCCESS(D3DKMTCloseAdapter(&closeAdapter));
}

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

        if (PhWindowsVersion >= WINDOWS_8)
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
        *PowerUsage = (((FLOAT)adapterPerfData.Power / 1000) * 100);
        *Temperature = (((FLOAT)adapterPerfData.Temperature / 1000) * 100);
        *FanRPM = adapterPerfData.FanRPM;
    }

    return status;
}

PPH_STRING GraphicsQueryDeviceDescription(
    _In_ DEVINST DeviceHandle
    )
{
    static PH_STRINGREF defaultName = PH_STRINGREF_INIT(L"Unknown Adapter");
    PPH_STRING string;

    string = GraphicsQueryDevicePropertyString(
        DeviceHandle,
        &DEVPKEY_Device_DeviceDesc
        );

    if (PhIsNullOrEmptyString(string))
        return PhCreateString2(&defaultName);
    else
        return string;
}

PPH_STRING GraphicsQueryDeviceInterfaceDescription(
    _In_opt_ PWSTR DeviceInterface
    )
{
    static PH_STRINGREF defaultName = PH_STRINGREF_INIT(L"Unknown Adapter");

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

    return PhCreateString2(&defaultName);
}

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
        return TRUE;
    }

    PhFree(buffer);
    return FALSE;
}

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

    return NULL;
}

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
        installedMemory = PhQueryRegistryUlong64(keyHandle, L"HardwareInformation.qwMemorySize");

        if (installedMemory == ULLONG_MAX)
            installedMemory = PhQueryRegistryUlong(keyHandle, L"HardwareInformation.MemorySize");

        if (installedMemory == ULONG_MAX) // HACK
            installedMemory = ULLONG_MAX;

        // Intel GPU devices incorrectly create the key with type REG_BINARY.
        if (installedMemory == ULLONG_MAX)
        {
            PH_STRINGREF valueName;
            PKEY_VALUE_PARTIAL_INFORMATION buffer;

            PhInitializeStringRef(&valueName, L"HardwareInformation.MemorySize");

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
            AdapterLuid = (PLUID)buffer;
        }
        else
        {
            memset(AdapterLuid, 0, sizeof(AdapterLuid));
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

    AdapterLuid = (PLUID)buffer;
    return TRUE;
}

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

    if (PhWindowsVersion >= WINDOWS_10)
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
            deviceIndex + wcslen(L"device "),
            deviceIndexLength - wcslen(L"device ")
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
        PH_STRINGREF name3DString = PH_STRINGREF_INIT(L"3D");
        PH_STRINGREF nameDecodeString = PH_STRINGREF_INIT(L"Video Decode");
        PH_STRINGREF nameEncodeString = PH_STRINGREF_INIT(L"Video Encode");
        PH_STRINGREF nameProcessingString = PH_STRINGREF_INIT(L"Video Processing");
        PH_STRINGREF nameAssemblyString = PH_STRINGREF_INIT(L"Scene Assembly");
        PH_STRINGREF nameCopyString = PH_STRINGREF_INIT(L"Copy");
        PH_STRINGREF nameOverlayString = PH_STRINGREF_INIT(L"Overlay");
        PH_STRINGREF nameCryptoString = PH_STRINGREF_INIT(L"Crypto");

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
