/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2020
 *
 */

#include "exttools.h"

DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D11_GRAPHICS, 0x8c47866b, 0x7583, 0x450d, 0xf0, 0xf0, 0x6b, 0xad, 0xa8, 0x95, 0xaf, 0x4b);
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, 0x0c9ece4d, 0x2f6e, 0x4f01, 0x8c, 0x96, 0xe8, 0x9e, 0x33, 0x1b, 0x47, 0xb1);
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE, 0x248e2800, 0xa793, 0x4724, 0xab, 0xaa, 0x23, 0xa6, 0xde, 0x1b, 0xe0, 0x90);
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GENERIC_ML, 0xb71b0d41, 0x1088, 0x422f, 0xa2, 0x7c, 0x2, 0x50, 0xb7, 0xd3, 0xa9, 0x88);
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GENERIC_MEDIA, 0x8eb2c848, 0x82f6, 0x4b49, 0xaa, 0x87, 0xae, 0xcf, 0xcf, 0x1, 0x74, 0xc6);
// rev
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_WSL, 0x42696D9D, 0xD678, 0x4006, 0xA9, 0x3D, 0x30, 0x0C, 0x35, 0x65, 0xBB, 0xE5);

DEFINE_GUID(DXCORE_HARDWARE_TYPE_ATTRIBUTE_GPU, 0xb69eb219, 0x3ded, 0x4464, 0x97, 0x9f, 0xa0, 0xb, 0xd4, 0x68, 0x70, 0x6);
DEFINE_GUID(DXCORE_HARDWARE_TYPE_ATTRIBUTE_COMPUTE_ACCELERATOR, 0xe0b195da, 0x58ef, 0x4a22, 0x90, 0xf1, 0x1f, 0x28, 0x16, 0x9c, 0xab, 0x8d);
DEFINE_GUID(DXCORE_HARDWARE_TYPE_ATTRIBUTE_NPU, 0xd46140c4, 0xadd7, 0x451b, 0x9e, 0x56, 0x6, 0xfe, 0x8c, 0x3b, 0x58, 0xed);
DEFINE_GUID(DXCORE_HARDWARE_TYPE_ATTRIBUTE_MEDIA_ACCELERATOR, 0x66bdb96a, 0x50b, 0x44c7, 0xa4, 0xfd, 0xd1, 0x44, 0xce, 0xa, 0xb4, 0x43);

VOID EtFormatToBuffer(
    _In_reads_(Count) PPH_FORMAT Format,
    _In_ ULONG Count,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    SIZE_T returnLength;

    if (PhFormatToBuffer(Format, Count, Block->TextCache[Message->SubId], sizeof(Block->TextCache[Message->SubId]), &returnLength))
        Block->TextCacheLength[Message->SubId] = returnLength - sizeof(UNICODE_NULL);
    else
        Block->TextCacheLength[Message->SubId] = 0;
}

VOID EtFormatToBufferNetwork(
    _In_reads_(Count) PPH_FORMAT Format,
    _In_ ULONG Count,
    _In_ PET_NETWORK_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    SIZE_T returnLength;

    if (PhFormatToBuffer(Format, Count, Block->TextCache[Message->SubId], sizeof(Block->TextCache[Message->SubId]), &returnLength))
        Block->TextCacheLength[Message->SubId] = returnLength - sizeof(UNICODE_NULL);
    else
        Block->TextCacheLength[Message->SubId] = 0;
}

VOID EtFormatInt64(
    _In_ ULONG64 Value,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    if (Value != 0)
    {
        PH_FORMAT format[1];

        PhInitFormatI64U(&format[0], Value);

        EtFormatToBuffer(format, 1, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatNetworkInt64(
    _In_ ULONG64 Value,
    _In_ PET_NETWORK_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    if (Value != 0)
    {
        PH_FORMAT format[1];

        PhInitFormatI64U(&format[0], Value);

        EtFormatToBufferNetwork(format, 1, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatSize(
    _In_ ULONG64 Value,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    if (Value != 0)
    {
        PH_FORMAT format[1];

        PhInitFormatSize(&format[0], Value);

        EtFormatToBuffer(format, 1, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatNetworkSize(
    _In_ ULONG64 Value,
    _In_ PET_NETWORK_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    if (Value != 0)
    {
        PH_FORMAT format[1];

        PhInitFormatSize(&format[0], Value);

        EtFormatToBufferNetwork(format, 1, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatDouble(
    _In_ DOUBLE Value,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    if (Value >= 0.01)
    {
        PH_FORMAT format[1];

        PhInitFormatF(&format[0], Value, 2);

        EtFormatToBuffer(format, 1, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatRate(
    _In_ ULONG64 ValuePerPeriod,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    ULONG64 number;

    number = ValuePerPeriod;
    number *= 1000;
    number /= EtUpdateInterval;

    if (number != 0)
    {
        PH_FORMAT format[2];

        PhInitFormatSize(&format[0], number);
        PhInitFormatS(&format[1], L"/s");

        EtFormatToBuffer(format, 2, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatNetworkRate(
    _In_ ULONG64 ValuePerPeriod,
    _In_ PET_NETWORK_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    ULONG64 number;

    number = ValuePerPeriod;
    number *= 1000;
    number /= EtUpdateInterval;

    if (number != 0)
    {
        PH_FORMAT format[2];

        PhInitFormatSize(&format[0], number);
        PhInitFormatS(&format[1], L"/s");

        EtFormatToBufferNetwork(format, 2, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

_Success_(return)
BOOLEAN EtOpenAdapterFromDeviceName(
    _Out_ PD3DKMT_HANDLE AdapterHandle,
    _In_ PWSTR DeviceName
    )
{
    D3DKMT_OPENADAPTERFROMDEVICENAME openAdapterFromDeviceName;

    memset(&openAdapterFromDeviceName, 0, sizeof(D3DKMT_OPENADAPTERFROMDEVICENAME));
    openAdapterFromDeviceName.pDeviceName = DeviceName;

    if (NT_SUCCESS(D3DKMTOpenAdapterFromDeviceName(&openAdapterFromDeviceName)))
    {
        *AdapterHandle = openAdapterFromDeviceName.hAdapter;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN EtCloseAdapterHandle(
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_CLOSEADAPTER closeAdapter;

    memset(&closeAdapter, 0, sizeof(D3DKMT_CLOSEADAPTER));
    closeAdapter.hAdapter = AdapterHandle;

    return NT_SUCCESS(D3DKMTCloseAdapter(&closeAdapter));
}

NTSTATUS EtQueryAdapterInformation(
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

// HardwareDevices!GraphicsQueryAdapterPropertyString
NTSTATUS EtpQueryAdapterPropertyString(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ PPH_STRINGREF PropertyName,
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

    if (!NT_SUCCESS(status = EtQueryAdapterInformation(
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

        if (!NT_SUCCESS(status = EtQueryAdapterInformation(
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

// HardwareDevices!GraphicsQueryAdapterAttributes
NTSTATUS EtQueryAdapterAttributes(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _Out_ PET_ADAPTER_ATTRIBUTES Attributes
    )
{
    static PH_STRINGREF dxCoreAttributes = PH_STRINGREF_INIT(L"DXCoreAttributes");
    static PH_STRINGREF dxAttributes = PH_STRINGREF_INIT(L"DXAttributes");
    NTSTATUS status;
    PPH_STRING adapterAttributes;

    Attributes->Flags = 0;

    // DXCoreAdapter::QueryAndFillAdapterExtendedProperiesOnPlatform
    if (!NT_SUCCESS(status = EtpQueryAdapterPropertyString(AdapterHandle, &dxCoreAttributes, &adapterAttributes)))
        if (!NT_SUCCESS(status = EtpQueryAdapterPropertyString(AdapterHandle, &dxAttributes, &adapterAttributes)))
            return status;

    for (PWCHAR attr = adapterAttributes->Buffer;;)
    {
        PH_STRINGREF attribute;
        GUID guid;

        PhInitializeStringRef(&attribute, attr);

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

PPH_STRING EtpQueryDevicePropertyString(
    _In_ DEVINST DeviceHandle,
    _In_ CONST DEVPROPKEY *DeviceProperty
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

    if (result != CR_SUCCESS)
    {
        PhFree(buffer);
        return NULL;
    }

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

ULONG64 EtpQueryInstalledMemory(
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

_Success_(return)
BOOLEAN EtQueryDeviceProperties(
    _In_ PPH_STRING DeviceInterface,
    _Out_opt_ PPH_STRING *Description,
    _Out_opt_ PPH_STRING *DriverDate,
    _Out_opt_ PPH_STRING *DriverVersion,
    _Out_opt_ PPH_STRING *LocationInfo,
    _Out_opt_ ULONG64 *InstalledMemory
    )
{
    DEVPROPTYPE devicePropertyType;
    DEVINST deviceInstanceHandle;
    ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
    WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];

    if (CM_Get_Device_Interface_Property(
        PhGetString(DeviceInterface),
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
        *Description = EtpQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_DeviceDesc);
    if (DriverDate)
        *DriverDate = EtpQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_DriverDate);
    if (DriverVersion)
        *DriverVersion = EtpQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_DriverVersion);
    if (LocationInfo)
        *LocationInfo = EtpQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_LocationInfo);
    if (InstalledMemory)
        *InstalledMemory = EtpQueryInstalledMemory(deviceInstanceHandle);
    // EtpQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_Manufacturer);

    return TRUE;
}

//D3D_FEATURE_LEVEL EtQueryAdapterFeatureLevel(
//    _In_ LUID AdapterLuid
//    )
//{
//    static PH_INITONCE initOnce = PH_INITONCE_INIT;
//    static PFN_D3D11_CREATE_DEVICE D3D11CreateDevice_I = NULL;
//    static HRESULT (WINAPI *CreateDXGIFactory1_I)(_In_ REFIID riid, _Out_ PVOID *ppFactory) = NULL;
//    D3D_FEATURE_LEVEL d3dFeatureLevelResult = 0;
//    IDXGIFactory1 *dxgiFactory;
//    IDXGIAdapter* dxgiAdapter;
//
//    if (PhBeginInitOnce(&initOnce))
//    {
//        PVOID dxgi;
//        PVOID d3d11;
//
//        if (dxgi = PhLoadLibrary(L"dxgi.dll"))
//        {
//            CreateDXGIFactory1_I = PhGetProcedureAddress(dxgi, "CreateDXGIFactory1", 0);
//        }
//
//        if (d3d11 = PhLoadLibrary(L"d3d11.dll"))
//        {
//            D3D11CreateDevice_I = PhGetProcedureAddress(d3d11, "D3D11CreateDevice", 0);
//        }
//
//        PhEndInitOnce(&initOnce);
//    }
//
//    if (!CreateDXGIFactory1_I || !SUCCEEDED(CreateDXGIFactory1_I(&IID_IDXGIFactory1, &dxgiFactory)))
//        return 0;
//
//    for (UINT i = 0; i < 25; i++)
//    {
//        DXGI_ADAPTER_DESC dxgiAdapterDescription;
//
//        if (!SUCCEEDED(IDXGIFactory1_EnumAdapters(dxgiFactory, i, &dxgiAdapter)))
//            break;
//
//        if (SUCCEEDED(IDXGIAdapter_GetDesc(dxgiAdapter, &dxgiAdapterDescription)))
//        {
//            if (RtlIsEqualLuid(&dxgiAdapterDescription.AdapterLuid, &AdapterLuid))
//            {
//                D3D_FEATURE_LEVEL d3dFeatureLevel[] =
//                {
//                    D3D_FEATURE_LEVEL_12_1,
//                    D3D_FEATURE_LEVEL_12_0,
//                    D3D_FEATURE_LEVEL_11_1,
//                    D3D_FEATURE_LEVEL_11_0,
//                    D3D_FEATURE_LEVEL_10_1,
//                    D3D_FEATURE_LEVEL_10_0,
//                    D3D_FEATURE_LEVEL_9_3,
//                    D3D_FEATURE_LEVEL_9_2,
//                    D3D_FEATURE_LEVEL_9_1
//                };
//                D3D_FEATURE_LEVEL d3ddeviceFeatureLevel;
//                ID3D11Device* d3d11device;
//
//                if (D3D11CreateDevice_I && SUCCEEDED(D3D11CreateDevice_I(
//                    dxgiAdapter,
//                    D3D_DRIVER_TYPE_UNKNOWN,
//                    NULL,
//                    0,
//                    d3dFeatureLevel,
//                    RTL_NUMBER_OF(d3dFeatureLevel),
//                    D3D11_SDK_VERSION,
//                    &d3d11device,
//                    &d3ddeviceFeatureLevel,
//                    NULL
//                    )))
//                {
//                    d3dFeatureLevelResult = d3ddeviceFeatureLevel;
//                    ID3D11Device_Release(d3d11device);
//                    IDXGIAdapter_Release(dxgiAdapter);
//                    break;
//                }
//            }
//        }
//
//        IDXGIAdapter_Release(dxgiAdapter);
//    }
//
//    IDXGIFactory1_Release(dxgiFactory);
//    return d3dFeatureLevelResult;
//}

BOOLEAN EtIsSoftwareDevice(
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_ADAPTERTYPE adapterType;

    memset(&adapterType, 0, sizeof(D3DKMT_ADAPTERTYPE));

    if (NT_SUCCESS(EtQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_ADAPTERTYPE,
        &adapterType,
        sizeof(D3DKMT_ADAPTERTYPE)
        )))
    {
        if (adapterType.SoftwareDevice) // adapterType.HybridIntegrated
        {
            return TRUE;
        }
    }

    return FALSE;
}

PPH_STRING EtGetNodeEngineTypeString(
    _In_ D3DKMT_NODEMETADATA NodeMetaData
    )
{
    switch (NodeMetaData.NodeData.EngineType)
    {
    case DXGK_ENGINE_TYPE_OTHER:
        return PhCreateString(NodeMetaData.NodeData.FriendlyName);
    case DXGK_ENGINE_TYPE_3D:
        return PhCreateString(L"3D");
    case DXGK_ENGINE_TYPE_VIDEO_DECODE:
        return PhCreateString(L"Video Decode");
    case DXGK_ENGINE_TYPE_VIDEO_ENCODE:
        return PhCreateString(L"Video Encode");
    case DXGK_ENGINE_TYPE_VIDEO_PROCESSING:
        return PhCreateString(L"Video Processing");
    case DXGK_ENGINE_TYPE_SCENE_ASSEMBLY:
        return PhCreateString(L"Scene Assembly");
    case DXGK_ENGINE_TYPE_COPY:
        return PhCreateString(L"Copy");
    case DXGK_ENGINE_TYPE_OVERLAY:
        return PhCreateString(L"Overlay");
    case DXGK_ENGINE_TYPE_CRYPTO:
        return PhCreateString(L"Crypto");
    }

    return PhFormatString(L"ERROR (%lu)", NodeMetaData.NodeData.EngineType);
}

PVOID EtQueryDeviceProperty(
    _In_ DEVINST DeviceHandle,
    _In_ CONST DEVPROPKEY* DeviceProperty
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
        return buffer;

    PhFree(buffer);
    return NULL;
}
