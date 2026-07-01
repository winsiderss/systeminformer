/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2020-2026
 *
 */

#include "exttools.h"

 // Undocumented device properties (Win10 only)
DEFINE_GUID(GUID_DISPLAY_DEVICE_ARRIVAL, 0x1ca05180, 0xa699, 0x450a, 0x9a, 0x0c, 0xde, 0x4f, 0xbe, 0x3d, 0xdd, 0x89);
//DEFINE_GUID(GUID_COMPUTE_DEVICE_ARRIVAL, 0x1024e4c9, 0x47c9, 0x48d3, 0xb4, 0xa8, 0xf9, 0xdf, 0x78, 0x52, 0x3b, 0x53);

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

static typeof(&D3DKMTEnumProcesses) D3DKMTEnumProcesses_I = NULL;
static typeof(&D3DKMTGetProcessList) D3DKMTGetProcessList_I = NULL;

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
    _In_ FLOAT Value,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    if (Value >= 0.01f)
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

NTSTATUS EtOpenAdapterFromDeviceName(
    _Out_ PD3DKMT_HANDLE AdapterHandle,
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
        {
            *AdapterLuid = openAdapterFromDeviceName.AdapterLuid;
        }
    }

    return status;
}

NTSTATUS EtCloseAdapterHandle(
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_CLOSEADAPTER closeAdapter;

    memset(&closeAdapter, 0, sizeof(D3DKMT_CLOSEADAPTER));
    closeAdapter.hAdapter = AdapterHandle;

    return D3DKMTCloseAdapter(&closeAdapter);
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

NTSTATUS EtpQueryAdapterProperty(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ PCPH_STRINGREF PropertyName,
    _Out_ D3DDDI_QUERYREGISTRY_INFO** PropertyInfo
    )
{
    NTSTATUS status;
    ULONG queryBufferLength;
    D3DDDI_QUERYREGISTRY_INFO* queryBuffer;

    queryBufferLength = FIELD_OFFSET(D3DDDI_QUERYREGISTRY_INFO, OutputString[MAX_PATH]);
    queryBuffer = PhAllocateZero(queryBufferLength);
    queryBuffer->QueryType = D3DDDI_QUERYREGISTRY_ADAPTERKEY;
    queryBuffer->QueryFlags.TranslatePath = 1;
    queryBuffer->ValueType = REG_MULTI_SZ;
    memcpy(queryBuffer->ValueName, PropertyName->Buffer, PropertyName->Length);

    status = EtQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_QUERYREGISTRY,
        queryBuffer,
        queryBufferLength
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (queryBuffer->Status == D3DDDI_QUERYREGISTRY_STATUS_BUFFER_OVERFLOW)
    {
        queryBufferLength = queryBuffer->OutputValueSize;
        queryBuffer = PhReAllocate(queryBuffer, queryBufferLength);

        status = EtQueryAdapterInformation(
            AdapterHandle,
            KMTQAITYPE_QUERYREGISTRY,
            queryBuffer,
            queryBufferLength
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (queryBuffer->Status != D3DDDI_QUERYREGISTRY_STATUS_SUCCESS)
    {
        status = STATUS_REGISTRY_IO_FAILED;
        goto CleanupExit;
    }

    if (NT_SUCCESS(status))
    {
        *PropertyInfo = queryBuffer;
        return status;
    }

CleanupExit:
    PhFree(queryBuffer);
    return status;
}

// HardwareDevices!GraphicsQueryAdapterAttributes
NTSTATUS EtQueryAdapterAttributes(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _Out_ PET_ADAPTER_ATTRIBUTES Attributes
    )
{
    static const PH_STRINGREF dxCoreAttributes = PH_STRINGREF_INIT(L"DXCoreAttributes");
    static const PH_STRINGREF dxAttributes = PH_STRINGREF_INIT(L"DXAttributes");
    NTSTATUS status;
    D3DDDI_QUERYREGISTRY_INFO* adapterAttributes;
    PWSTR attributes;

    Attributes->Flags = 0;

    status = EtpQueryAdapterProperty(AdapterHandle, &dxCoreAttributes, &adapterAttributes);

    if (!NT_SUCCESS(status))
    {
        status = EtpQueryAdapterProperty(AdapterHandle, &dxAttributes, &adapterAttributes);

        if (!NT_SUCCESS(status))
            return status;
    }

    attributes = adapterAttributes->OutputString;

    // DXCoreAdapter::QueryAndFillAdapterExtendedProperiesOnPlatform
    //if (!NT_SUCCESS(status = EtpQueryAdapterPropertyString(AdapterHandle, &dxCoreAttributes, &adapterAttributes)))
    //    if (!NT_SUCCESS(status = EtpQueryAdapterPropertyString(AdapterHandle, &dxAttributes, &adapterAttributes)))
    //        return status;

    for (;;)
    {
        PH_STRINGREF attribute;
        GUID guid;

        PhInitializeStringRefLongHint(&attribute, attributes);

        if (!attribute.Length)
            break;

        attributes = PTR_ADD_OFFSET(attributes, attribute.Length + sizeof(UNICODE_NULL));

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

    PhFree(adapterAttributes);

    return status;
}

/**
 * Formats a DEVPROPERTY value as a display string.
 */
static PPH_STRING EtpFormatDeviceProperty(
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
 * Queries the installed memory value for a graphics adapter from its device instance ID.
 */
static ULONG64 EtpQueryInstalledMemory(
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
        static CONST PH_STRINGREF valueName = PH_STRINGREF_INIT(L"HardwareInformation.MemorySize");
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
    DEVPROPCOMPKEY interfaceProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
    };
    DEVPROPCOMPKEY deviceProperties[] =
    {
        { DEVPKEY_Device_DeviceDesc,    DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_DriverDate,    DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_DriverVersion, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_LocationInfo,  DEVPROP_STORE_SYSTEM, NULL },
    };
    ULONG interfacePropertyCount = 0;
    const DEVPROPERTY* interfaceProps = NULL;
    ULONG devicePropertyCount = 0;
    const DEVPROPERTY* deviceProps = NULL;
    const DEVPROPERTY* instanceIdProp;
    PPH_STRING instanceIdString;
    SIZE_T instanceIdLength;

    if (HR_FAILED(PhDevGetObjectProperties(
        DevObjectTypeDeviceInterface,
        PhGetString(DeviceInterface),
        DevQueryFlagNone,
        RTL_NUMBER_OF(interfaceProperties),
        interfaceProperties,
        &interfacePropertyCount,
        &interfaceProps
        )))
    {
        return FALSE;
    }

    instanceIdProp = PhDevFindProperty(
        &DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, interfacePropertyCount, interfaceProps);

    if (!instanceIdProp || instanceIdProp->Type != DEVPROP_TYPE_STRING || !instanceIdProp->Buffer || instanceIdProp->BufferSize < sizeof(UNICODE_NULL))
    {
        PhDevFreeObjectProperties(interfacePropertyCount, interfaceProps);
        return FALSE;
    }

    instanceIdLength = instanceIdProp->BufferSize;
    if (((PWSTR)instanceIdProp->Buffer)[(instanceIdProp->BufferSize / sizeof(WCHAR)) - 1] == UNICODE_NULL)
        instanceIdLength -= sizeof(UNICODE_NULL);
    instanceIdString = PhCreateStringEx((PWCHAR)instanceIdProp->Buffer, instanceIdLength);

    if (HR_FAILED(PhDevGetObjectProperties(
        DevObjectTypeDevice,
        PhGetString(instanceIdString),
        DevQueryFlagNone,
        RTL_NUMBER_OF(deviceProperties),
        deviceProperties,
        &devicePropertyCount,
        &deviceProps
        )))
    {
        PhDereferenceObject(instanceIdString);
        PhDevFreeObjectProperties(interfacePropertyCount, interfaceProps);
        return FALSE;
    }

    if (Description)
    {
        *Description = EtpFormatDeviceProperty(PhDevFindProperty(
            &DEVPKEY_Device_DeviceDesc, DEVPROP_STORE_SYSTEM, devicePropertyCount, deviceProps));
    }
    if (DriverDate)
    {
        *DriverDate = EtpFormatDeviceProperty(PhDevFindProperty(
            &DEVPKEY_Device_DriverDate, DEVPROP_STORE_SYSTEM, devicePropertyCount, deviceProps));
    }
    if (DriverVersion)
    {
        *DriverVersion = EtpFormatDeviceProperty(PhDevFindProperty(
            &DEVPKEY_Device_DriverVersion, DEVPROP_STORE_SYSTEM, devicePropertyCount, deviceProps));
    }
    if (LocationInfo)
    {
        *LocationInfo = EtpFormatDeviceProperty(PhDevFindProperty(
            &DEVPKEY_Device_LocationInfo, DEVPROP_STORE_SYSTEM, devicePropertyCount, deviceProps));
    }
    if (InstalledMemory)
    {
        *InstalledMemory = EtpQueryInstalledMemory(instanceIdString);
    }

    PhDevFreeObjectProperties(devicePropertyCount, deviceProps);
    PhDereferenceObject(instanceIdString);
    PhDevFreeObjectProperties(interfacePropertyCount, interfaceProps);
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
    _In_ D3DKMT_NODEMETADATA* NodeMetaData
    )
{
    switch (NodeMetaData->NodeData.EngineType)
    {
    case DXGK_ENGINE_TYPE_OTHER:
        return PhCreateString(NodeMetaData->NodeData.FriendlyName);
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
    case DXGK_ENGINE_TYPE_VIDEO_CODEC:
        return PhCreateString(L"Video Codec");
    }

    return PhFormatString(L"ERROR (%lu)", NodeMetaData->NodeData.EngineType);
}

// Initialize dynamic D3DKMT function pointers
VOID EtpInitializeD3DKMTFunctions(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        HMODULE gdi32Module = PhGetDllHandle(L"gdi32.dll");
        if (gdi32Module)
        {
            D3DKMTEnumProcesses_I = PhGetProcedureAddress(gdi32Module, "D3DKMTEnumProcesses", 0);
            D3DKMTGetProcessList_I = PhGetProcedureAddress(gdi32Module, "D3DKMTGetProcessList", 0);
        }

        PhEndInitOnce(&initOnce);
    }
}

NTSTATUS EtAdapterGetProcessListToBuffer(
    _In_ LUID AdapterLuid,
    _Out_writes_bytes_(BufferLength) PHANDLE Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    NTSTATUS status;
    D3DKMT_GET_PROCESS_LIST request;

    EtpInitializeD3DKMTFunctions();

    if (!D3DKMTGetProcessList_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&request, 0, sizeof(D3DKMT_GET_PROCESS_LIST));
    request.AdapterLuid = AdapterLuid;
    request.DesiredAccess = PROCESS_QUERY_INFORMATION;
    request.ProcessHandleCount = (ULONG)(BufferLength / sizeof(HANDLE));
    request.ProcessHandle = Buffer;

    status = D3DKMTGetProcessList_I(&request);

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL)
    {
        if (ReturnLength)
        {
            *ReturnLength = request.ProcessHandleCount * sizeof(HANDLE);
        }
    }

    return status;
}

NTSTATUS EtAdapterEnumProcessListToBuffer(
    _In_ LUID AdapterLuid,
    _Out_writes_bytes_(BufferLength) PULONG Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    NTSTATUS status;
    D3DKMT_ENUM_PROCESS_LIST request;

    EtpInitializeD3DKMTFunctions();

    if (!D3DKMTEnumProcesses_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&request, 0, sizeof(D3DKMT_ENUM_PROCESS_LIST));
    request.AdapterLuid = AdapterLuid;
    request.ProcessIdCount = BufferLength / sizeof(ULONG);
    request.ProcessIdBuffer = Buffer;

    status = D3DKMTEnumProcesses_I(&request);

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL)
    {
        if (ReturnLength)
        {
            *ReturnLength = request.ProcessIdCount * sizeof(ULONG);
        }
    }

    return status;
}

NTSTATUS EtAdapterGetProcessList(
    _In_ LUID AdapterLuid,
    _In_ SIZE_T PreviousProcessCount,
    _Outptr_result_buffer_(*ProcessCount) PHANDLE* ProcessHandles,
    _Out_ PSIZE_T ProcessCount
    )
{
    NTSTATUS status;
    PHANDLE buffer;
    SIZE_T bufferLength;
    SIZE_T returnLength;
    ULONG attempts;

    *ProcessHandles = NULL;
    *ProcessCount = 0;

    returnLength = PreviousProcessCount * sizeof(HANDLE);

    if (!returnLength)
    {
        status = EtAdapterGetProcessListToBuffer(AdapterLuid, NULL, 0, &returnLength);

        if (status != STATUS_BUFFER_TOO_SMALL)
            return status;
    }

    bufferLength = returnLength;
    buffer = NULL;

    for (attempts = 0; attempts < 8; attempts++)
    {
        if (buffer) PhFree(buffer);
        buffer = (PHANDLE)PhAllocateSafe(bufferLength);
        if (!buffer) return STATUS_NO_MEMORY;

        status = EtAdapterGetProcessListToBuffer(AdapterLuid, buffer, bufferLength, &returnLength);

        if (status != STATUS_BUFFER_TOO_SMALL)
            break;

        bufferLength = returnLength;
    }

    if (!NT_SUCCESS(status))
    {
        if (buffer)
            PhFree(buffer);

        return status;
    }

    *ProcessHandles = buffer;
    *ProcessCount = returnLength / sizeof(HANDLE);

    return status;
}

NTSTATUS EtAdapterEnumProcessList(
    _In_ LUID AdapterLuid,
    _In_ SIZE_T PreviousProcessCount,
    _Outptr_result_buffer_(*ProcessCount) PULONG* ProcessIds,
    _Out_ PSIZE_T ProcessCount
    )
{
    NTSTATUS status;
    PULONG buffer;
    SIZE_T bufferLength;
    SIZE_T returnLength;
    ULONG attempts;

    *ProcessIds = NULL;
    *ProcessCount = 0;

    returnLength = PreviousProcessCount * sizeof(ULONG);

    if (!returnLength)
    {
        status = EtAdapterEnumProcessListToBuffer(AdapterLuid, NULL, 0, &returnLength);

        if (status != STATUS_BUFFER_TOO_SMALL)
            return status;
    }

    bufferLength = returnLength;
    buffer = NULL;

    for (attempts = 0; attempts < 8; attempts++)
    {
        if (buffer) PhFree(buffer);
        buffer = (PULONG)PhAllocateSafe(bufferLength);
        if (!buffer) return STATUS_NO_MEMORY;

        status = EtAdapterEnumProcessListToBuffer(AdapterLuid, buffer, bufferLength, &returnLength);

        if (status != STATUS_BUFFER_TOO_SMALL)
            break;

        bufferLength = returnLength;
    }

    if (!NT_SUCCESS(status))
    {
        if (buffer)
            PhFree(buffer);

        return status;
    }

    *ProcessIds = buffer;
    *ProcessCount = returnLength / sizeof(ULONG);

    return status;
}

FORCEINLINE CONST PPH_STRING EtGetDeviceInstanceIdFromDeviceObject(
    _In_ const DEV_OBJECT* Object
    )
{
    const DEVPROPERTY* instanceIdProperty;

    instanceIdProperty = PhDevFindProperty(
        &DEVPKEY_Device_InstanceId,
        DEVPROP_STORE_SYSTEM,
        Object->cPropertyCount,
        Object->pProperties
        );

    if (
        instanceIdProperty && instanceIdProperty->Type == DEVPROP_TYPE_STRING &&
        instanceIdProperty->Buffer && instanceIdProperty->BufferSize >= sizeof(UNICODE_NULL)
        )
    {
        PPH_STRING normalizedPath;

        normalizedPath = PhCreateString(instanceIdProperty->Buffer);

        if (normalizedPath->Length >= 2 * sizeof(WCHAR) && normalizedPath->Buffer[1] == L'?')
            normalizedPath->Buffer[1] = OBJ_NAME_PATH_SEPARATOR;

        return normalizedPath;
    }

    return NULL;
}

PPH_STRING EtGetDeviceInterfaceFromLuid(
    _In_ PLUID AdapterLuid
    )
{
    PPH_STRING deviceInterface = NULL;
    ULONG deviceCount = 0;
    PDEV_OBJECT deviceObjects = NULL;
    const DEVPROPCOMPKEY deviceProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Gpu_Luid, DEVPROP_STORE_SYSTEM, NULL },
    };
    const DEVPROP_FILTER_EXPRESSION deviceFilter[] =
    {
        { DEVPROP_OPERATOR_EQUALS, { { DEVPKEY_Gpu_Luid, DEVPROP_STORE_SYSTEM, NULL }, DEVPROP_TYPE_UINT64, sizeof(LUID), AdapterLuid } }
    };

    if (HR_SUCCESS(PhDevGetObjects(
        DevObjectTypeDeviceInterface,
        DevQueryFlagNone,
        RTL_NUMBER_OF(deviceProperties),
        deviceProperties,
        RTL_NUMBER_OF(deviceFilter),
        deviceFilter,
        &deviceCount,
        &deviceObjects
        )))
    {
        for (ULONG i = 0; i < deviceCount; i++)
        {
            PDEV_OBJECT device = &deviceObjects[i];
            const DEVPROPERTY* classGuidProperty;
            const DEVPROPERTY* luidProperty;

            classGuidProperty = PhDevFindProperty(
                &DEVPKEY_DeviceInterface_ClassGuid,
                DEVPROP_STORE_SYSTEM,
                device->cPropertyCount,
                device->pProperties
                );

            luidProperty = PhDevFindProperty(
                &DEVPKEY_Gpu_Luid,
                DEVPROP_STORE_SYSTEM,
                device->cPropertyCount,
                device->pProperties
                );

            if (!luidProperty || luidProperty->Type != DEVPROP_TYPE_UINT64 || !luidProperty->Buffer || luidProperty->BufferSize != sizeof(LUID))
                continue;

            if (!RtlIsEqualLuid((PLUID)luidProperty->Buffer, AdapterLuid))
                continue;

            if (!classGuidProperty || classGuidProperty->Type != DEVPROP_TYPE_GUID || !classGuidProperty->Buffer || classGuidProperty->BufferSize != sizeof(GUID))
                continue;

            if (
                !RtlEqualMemory(classGuidProperty->Buffer, &GUID_DISPLAY_DEVICE_ARRIVAL, sizeof(GUID)) &&
                !RtlEqualMemory(classGuidProperty->Buffer, &GUID_COMPUTE_DEVICE_ARRIVAL, sizeof(GUID))
                )
                continue;

            deviceInterface = PhCreateString(device->pszObjectId);
            break;
        }

        PhDevFreeObjects(deviceCount, deviceObjects);
    }

    return deviceInterface;
}

static VOID EtpAddDiscoveredAdapter(
    _In_ PPH_LIST AdapterList,
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ LUID AdapterLuid,
    _In_opt_ PPH_STRING DeviceInterface
    )
{
    PET_DISCOVERED_ADAPTER entry;
    PPH_STRING deviceInterface = DeviceInterface;

    for (ULONG i = 0; i < AdapterList->Count; i++)
    {
        entry = AdapterList->Items[i];

        if (RtlIsEqualLuid(&entry->AdapterLuid, &AdapterLuid))
        {
            if (!entry->DeviceInterface && deviceInterface)
                entry->DeviceInterface = PhReferenceObject(deviceInterface);

            if (!entry->AdapterHandle && AdapterHandle)
            {
                entry->AdapterHandle = AdapterHandle;

                if (!entry->AttributesValid)
                    entry->AttributesValid = NT_SUCCESS(EtQueryAdapterAttributes(entry->AdapterHandle, &entry->Attributes));
            }
            else if (AdapterHandle && AdapterHandle != entry->AdapterHandle)
                EtCloseAdapterHandle(AdapterHandle);

            return;
        }
    }

    if (!deviceInterface)
    {
        deviceInterface = EtGetDeviceInterfaceFromLuid(&AdapterLuid);
    }

    entry = PhAllocateZero(sizeof(ET_DISCOVERED_ADAPTER));
    entry->AdapterHandle = AdapterHandle;
    entry->AdapterLuid = AdapterLuid;
    PhSetReference(&entry->DeviceInterface, deviceInterface);

    entry->AttributesValid = NT_SUCCESS(EtQueryAdapterAttributes(entry->AdapterHandle, &entry->Attributes));

    PhAddItemList(AdapterList, entry);

    // {
    //     if (entry->AdapterHandle)
    //         EtCloseAdapterHandle(entry->AdapterHandle);
    //     PhClearReference(&entry->DeviceInterface);
    //     PhFree(entry);
    // }
}

PPH_LIST EtInitializeGraphicsAdapters(
    VOID
    )
{
    PPH_LIST graphicsAdapterList;

    graphicsAdapterList = PhCreateList(10);

    // 1. Primary Enumeration: D3DKMTEnumAdapters3 (Win10 2004+)
    if (EtWindowsVersion >= WINDOWS_10_20H1)
    {
        static PFND3DKMT_ENUMADAPTERS3 EtpD3DKMTEnumAdapters3 = NULL;

        if (!EtpD3DKMTEnumAdapters3)
        {
            PVOID gdi32 = PhGetDllHandle(L"gdi32.dll");
            if (gdi32) EtpD3DKMTEnumAdapters3 = PhGetProcedureAddressT(gdi32, D3DKMTEnumAdapters3);
        }

        if (EtpD3DKMTEnumAdapters3)
        {
            D3DKMT_ENUMADAPTERS3 enumAdapters;

            memset(&enumAdapters, 0, sizeof(D3DKMT_ENUMADAPTERS3));
            enumAdapters.Filter.IncludeComputeOnly = 1;
            enumAdapters.Filter.IncludeDisplayOnly = 1;

            if (NT_SUCCESS(EtpD3DKMTEnumAdapters3(&enumAdapters)) && enumAdapters.NumAdapters > 0)
            {
                D3DKMT_ADAPTERINFO* adapters;

                adapters = PhAllocateZero(sizeof(D3DKMT_ADAPTERINFO) * enumAdapters.NumAdapters);
                enumAdapters.pAdapters = adapters;

                if (NT_SUCCESS(EtpD3DKMTEnumAdapters3(&enumAdapters)))
                {
                    for (ULONG i = 0; i < enumAdapters.NumAdapters; i++)
                    {
                        EtpAddDiscoveredAdapter(graphicsAdapterList, adapters[i].hAdapter, adapters[i].AdapterLuid, NULL);
                    }
                }

                PhFree(adapters);
            }
        }
    }

    // 2. Enhanced Fallback: cfgmgr32 (PhDevGetObjects)
    {
        ULONG deviceCount = 0;
        PDEV_OBJECT deviceObjects = NULL;
        DEVPROPCOMPKEY deviceProperties[1];
        DEVPROP_FILTER_EXPRESSION deviceFilter[4];
        DEVPROPERTY deviceFilterProperty[2];
        DEVPROPCOMPKEY deviceFilterCompoundProp;

        memset(deviceProperties, 0, sizeof(deviceProperties));
        deviceProperties[0].Key = DEVPKEY_Device_InstanceId;
        deviceProperties[0].Store = DEVPROP_STORE_SYSTEM;

        memset(&deviceFilterCompoundProp, 0, sizeof(deviceFilterCompoundProp));
        deviceFilterCompoundProp.Key = DEVPKEY_DeviceInterface_ClassGuid;
        deviceFilterCompoundProp.Store = DEVPROP_STORE_SYSTEM;

        memset(deviceFilterProperty, 0, sizeof(deviceFilterProperty));
        deviceFilterProperty[0].CompKey = deviceFilterCompoundProp;
        deviceFilterProperty[0].Type = DEVPROP_TYPE_GUID;
        deviceFilterProperty[0].BufferSize = sizeof(GUID);
        deviceFilterProperty[0].Buffer = (PGUID)&GUID_DISPLAY_DEVICE_ARRIVAL;

        deviceFilterProperty[1].CompKey = deviceFilterCompoundProp;
        deviceFilterProperty[1].Type = DEVPROP_TYPE_GUID;
        deviceFilterProperty[1].BufferSize = sizeof(GUID);
        deviceFilterProperty[1].Buffer = (PGUID)&GUID_COMPUTE_DEVICE_ARRIVAL;

        memset(deviceFilter, 0, sizeof(deviceFilter));
        deviceFilter[0].Operator = DEVPROP_OPERATOR_OR_OPEN;
        deviceFilter[1].Operator = DEVPROP_OPERATOR_EQUALS;
        deviceFilter[1].Property = deviceFilterProperty[0];
        deviceFilter[2].Operator = DEVPROP_OPERATOR_EQUALS;
        deviceFilter[2].Property = deviceFilterProperty[1];
        deviceFilter[3].Operator = DEVPROP_OPERATOR_OR_CLOSE;

        if (HR_SUCCESS(PhDevGetObjects(
            DevObjectTypeDeviceInterface,
            DevQueryFlagNone,
            RTL_NUMBER_OF(deviceProperties),
            deviceProperties,
            RTL_NUMBER_OF(deviceFilter),
            deviceFilter,
            &deviceCount,
            &deviceObjects
            )))
        {
            for (ULONG i = 0; i < deviceCount; i++)
            {
                PDEV_OBJECT device = &deviceObjects[i];
                D3DKMT_HANDLE adapterHandle;
                LUID adapterLuid;

                if (NT_SUCCESS(EtOpenAdapterFromDeviceName(&adapterHandle, &adapterLuid, device->pszObjectId)))
                {
                    EtpAddDiscoveredAdapter(graphicsAdapterList, adapterHandle, adapterLuid, PhCreateString(device->pszObjectId));
                }
            }

            PhDevFreeObjects(deviceCount, deviceObjects);
        }
        else
        {
            // Legacy CM_Get_Device_Interface_List fallback
            PGUID GUIDs[] = { (PGUID)&GUID_DISPLAY_DEVICE_ARRIVAL, (PGUID)&GUID_COMPUTE_DEVICE_ARRIVAL };

            for (ULONG i = 0; i < 2; i++)
            {
                PWSTR deviceInterfaceList;
                ULONG deviceInterfaceListLength = 0;
                PWSTR deviceInterface;

                if (CM_Get_Device_Interface_List_Size(
                    &deviceInterfaceListLength,
                    GUIDs[i],
                    NULL,
                    CM_GET_DEVICE_INTERFACE_LIST_PRESENT
                    ) == CR_SUCCESS)
                {
                    deviceInterfaceList = PhAllocate(deviceInterfaceListLength * sizeof(WCHAR));

                    if (CM_Get_Device_Interface_List(
                        GUIDs[i],
                        NULL,
                        deviceInterfaceList,
                        deviceInterfaceListLength,
                        CM_GET_DEVICE_INTERFACE_LIST_PRESENT
                        ) == CR_SUCCESS)
                    {
                        deviceInterface = deviceInterfaceList;

                        while (*deviceInterface)
                        {
                            PH_STRINGREF string;

                            PhInitializeStringRefLongHint(&string, deviceInterface);

                            if (string.Length > 0)
                            {
                                D3DKMT_HANDLE adapterHandle;
                                LUID adapterLuid;

                                if (NT_SUCCESS(EtOpenAdapterFromDeviceName(&adapterHandle, &adapterLuid, deviceInterface)))
                                {
                                    EtpAddDiscoveredAdapter(graphicsAdapterList, adapterHandle, adapterLuid, PhCreateString2(&string));
                                }
                            }

                            deviceInterface = PTR_ADD_OFFSET(deviceInterface, string.Length + sizeof(UNICODE_NULL));
                        }
                    }

                    PhFree(deviceInterfaceList);
                }
            }
        }
    }

    return graphicsAdapterList;
}

VOID EtUninitializeGraphicsAdapters(
    _In_ PPH_LIST AdapterList
    )
{
    if (AdapterList)
    {
        for (ULONG i = 0; i < AdapterList->Count; i++)
        {
            PET_DISCOVERED_ADAPTER entry = AdapterList->Items[i];

            if (entry->AdapterHandle)
            {
                EtCloseAdapterHandle(entry->AdapterHandle);
                entry->AdapterHandle = 0;
            }

            if (entry->DeviceInterface)
            {
                PhDereferenceObject(entry->DeviceInterface);
                entry->DeviceInterface = NULL;
            }

            PhFree(entry);
        }

        PhDereferenceObject(AdapterList);
    }
}
