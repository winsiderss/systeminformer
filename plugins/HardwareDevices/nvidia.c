/*
 * Process Hacker Extra Plugins -
 *   Nvidia GPU Plugin
 *
 * Copyright (C) 2015 dmex
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
#include "devices.h"
#include "nvapi\nvapi.h"
#include "nvidia.h"

#ifdef _NV_GPU_BUILD

BOOLEAN NvApiInitialized = FALSE;
static PVOID NvApiLibrary = NULL;
static PPH_LIST NvGpuPhysicalHandleList = NULL;
static PPH_LIST NvGpuDisplayHandleList = NULL;
static NvU32 GpuArchType = 0;

ULONG GpuMemoryLimit = 0;
FLOAT GpuCurrentGpuUsage = 0.0f;
FLOAT GpuCurrentCoreUsage = 0.0f;
FLOAT GpuCurrentBusUsage = 0.0f;
ULONG GpuCurrentMemUsage = 0;
ULONG GpuCurrentMemSharedUsage = 0;
ULONG GpuCurrentCoreTemp = 0;
ULONG GpuCurrentBoardTemp = 0;
ULONG GpuCurrentCoreClock = 0;
ULONG GpuCurrentMemoryClock = 0;
ULONG GpuCurrentShaderClock = 0;
ULONG GpuCurrentVoltage = 0;
//NVAPI_GPU_PERF_DECREASE GpuPerfDecreaseReason = NV_GPU_PERF_DECREASE_NONE;

static VOID NvGpuEnumPhysicalHandles(VOID)
{
    NvU32 gpuCount = 0;
    NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS];

    memset(gpuHandles, 0, sizeof(gpuHandles));

    if (NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount) == NVAPI_OK)
    {
        PhAddItemsList(NvGpuPhysicalHandleList, gpuHandles, gpuCount);
    }
}

static VOID NvGpuEnumDisplayHandles(VOID)
{
    for (NvU32 i = 0; i < NVAPI_MAX_DISPLAYS; i++)
    {
        NvDisplayHandle displayHandle;

        if (NvAPI_EnumNvidiaDisplayHandle(i, &displayHandle) == NVAPI_END_ENUMERATION)
        {
            break;
        }

        PhAddItemList(NvGpuDisplayHandleList, displayHandle);
    }
}

VOID NvApiInitialize(VOID)
{
    ULONG sampleCount;

    if (!PhGetIntegerSetting(SETTING_NAME_ENABLE_GPU))
        return;

    sampleCount = PhGetIntegerSetting(L"SampleCount");
    PhInitializeCircularBuffer_FLOAT(&GpuUtilizationHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG(&GpuMemoryHistory, sampleCount);
    PhInitializeCircularBuffer_FLOAT(&GpuBoardHistory, sampleCount);
    PhInitializeCircularBuffer_FLOAT(&GpuBusHistory, sampleCount);

    NvGpuPhysicalHandleList = PhCreateList(1);
    NvGpuDisplayHandleList = PhCreateList(1);

#ifdef _M_IX86
    if (!(NvApiLibrary = LoadLibrary(L"nvapi.dll")))
        return;
#else
    if (!(NvApiLibrary = LoadLibrary(L"nvapi64.dll")))
        return;
#endif

    // Retrieve the NvAPI_QueryInterface function address
    if (!(NvAPI_QueryInterface = PhGetProcedureAddress(NvApiLibrary, "nvapi_QueryInterface", 0)))
        return;

    // Initialization functions
    if (!(NvAPI_Initialize = NvAPI_QueryInterface(0x150E828UL)))
        return;
    if (!(NvAPI_Unload = NvAPI_QueryInterface(0xD22BDD7EUL)))
        return;

    // Error functions
    NvAPI_GetErrorMessage = NvAPI_QueryInterface(0x6C2D048CUL);

    // Handle functions
    NvAPI_EnumPhysicalGPUs = NvAPI_QueryInterface(0xE5AC921FUL);
    NvAPI_EnumNvidiaDisplayHandle = NvAPI_QueryInterface(0x9ABDD40DUL);

    // Information functions
    NvAPI_SYS_GetDriverAndBranchVersion = NvAPI_QueryInterface(0x2926AAADUL);
    NvAPI_GPU_GetFullName = NvAPI_QueryInterface(0xCEEE8E9FUL);

    // Query functions
    NvAPI_GPU_GetMemoryInfo = NvAPI_QueryInterface(0x774AA982UL);
    NvAPI_GPU_GetThermalSettings = NvAPI_QueryInterface(0xE3640A56UL);
    NvAPI_GPU_GetCoolerSettings = NvAPI_QueryInterface(0xDA141340UL);
    NvAPI_GPU_GetPerfDecreaseInfo = NvAPI_QueryInterface(0x7F7F4600UL);
    NvAPI_GPU_GetTachReading = NvAPI_QueryInterface(0x5F608315UL);
    NvAPI_GPU_GetAllClockFrequencies = NvAPI_QueryInterface(0xDCB616C3UL);

    // Undocumented functions below

    NvAPI_GPU_GetUsages = NvAPI_QueryInterface(0x189A1FDFUL);
    NvAPI_GPU_GetAllClocks = NvAPI_QueryInterface(0x1BD69F49UL);
    NvAPI_GPU_GetVoltageDomainsStatus = NvAPI_QueryInterface(0xC16C7E2CUL);

    NvAPI_GPU_GetPerfClocks = NvAPI_QueryInterface(0x1EA54A3B);
    NvAPI_GPU_GetVoltages = NvAPI_QueryInterface(0x7D656244);
    NvAPI_GPU_QueryActiveApps = NvAPI_QueryInterface(0x65B1C5F5);
    NvAPI_GPU_GetShaderPipeCount = NvAPI_QueryInterface(0x63E2F56F);
    NvAPI_GPU_GetShaderSubPipeCount = NvAPI_QueryInterface(0x0BE17923);
    NvAPI_GPU_GetRamBusWidth = NvAPI_QueryInterface(0x7975C581);
    NvAPI_GPU_GetRamBankCount = NvAPI_QueryInterface(0x17073A3CUL);
    NvAPI_GPU_GetRamType = NvAPI_QueryInterface(0x57F7CAACUL);
    NvAPI_GPU_GetRamMaker = NvAPI_QueryInterface(0x42AEA16AUL);
    NvAPI_GPU_GetFoundry = NvAPI_QueryInterface(0x5D857A00UL);
    //NvAPI_GetDisplayDriverMemoryInfo = NvAPI_QueryInterface(0x774AA982);
    //NvAPI_GetPhysicalGPUsFromDisplay = NvAPI_QueryInterface(0x34EF9506);
    NvAPI_GetDisplayDriverVersion = NvAPI_QueryInterface(0xF951A4D1UL);
    NvAPI_GetDisplayDriverRegistryPath = NvAPI_QueryInterface(0x0E24CEEEUL);
    //NvAPI_RestartDisplayDriver = NvAPI_QueryInterface(0xB4B26B65UL);
    //NvAPI_GPU_GetBoardInfo = NvAPI_QueryInterface(0x22D54523);
    //NvAPI_GPU_GetBusType = NvAPI_QueryInterface(0x1BB18724);
    //NvAPI_GPU_GetIRQ = NvAPI_QueryInterface(0xE4715417);
    NvAPI_GPU_GetVbiosVersionString = NvAPI_QueryInterface(0xA561FD7DUL);
    NvAPI_GPU_GetShortName = NvAPI_QueryInterface(0xD988F0F3UL);
    NvAPI_GPU_GetArchInfo = NvAPI_QueryInterface(0xD8265D24UL);
    NvAPI_GPU_GetPCIIdentifiers = NvAPI_QueryInterface(0x2DDFB66EUL);
    NvAPI_GPU_GetPartitionCount = NvAPI_QueryInterface(0x86F05D7AUL);
    NvAPI_GPU_GetGpuCoreCount = NvAPI_QueryInterface(0xC7026A87UL);
    NvAPI_GPU_GetPCIEInfo = NvAPI_QueryInterface(0xE3795199UL);
    NvAPI_GPU_GetFBWidthAndLocation = NvAPI_QueryInterface(0x11104158UL);
    NvAPI_GPU_ClientPowerTopologyGetStatus = NvAPI_QueryInterface(0x0EDCF624EUL);
    NvAPI_GetDisplayDriverBuildTitle = NvAPI_QueryInterface(0x7562E947);
    NvAPI_GetDisplayDriverCompileType = NvAPI_QueryInterface(0x988AEA78);
    NvAPI_GetDisplayDriverSecurityLevel = NvAPI_QueryInterface(0x9D772BBA);
    NvAPI_GPU_GetVPECount = NvAPI_QueryInterface(0xD8CBF37B);
    NvAPI_GPU_GetTargetID = NvAPI_QueryInterface(0x35B5FD2F);
    //NvAPI_GPU_GetExtendedMinorRevision = NvAPI_QueryInterface(0x025F17421);
    //NvAPI_GPU_GetSerialNumber = NvAPI_QueryInterface(0x14B83A5F);

    if (NvAPI_Initialize() == NVAPI_OK)
    {
        NvGpuEnumPhysicalHandles();
        NvGpuEnumDisplayHandles();

        NvApiInitialized = TRUE;
    }
}

BOOLEAN DestroyNvApi(VOID)
{
    NvApiInitialized = FALSE;

    if (NvGpuDisplayHandleList)
        PhDereferenceObject(NvGpuDisplayHandleList);

    if (NvGpuPhysicalHandleList)
        PhDereferenceObject(NvGpuPhysicalHandleList);

    if (NvAPI_Unload)
        NvAPI_Unload();

    if (NvApiLibrary)
        FreeLibrary(NvApiLibrary);

    return TRUE;
}

PPH_STRING NvGpuQueryDriverVersion(VOID)
{
    if (NvApiInitialized && NvAPI_SYS_GetDriverAndBranchVersion)
    {
        NvU32 driverVersion = 0;
        NvAPI_ShortString driverAndBranchString = "";

        if (NvAPI_SYS_GetDriverAndBranchVersion(&driverVersion, driverAndBranchString) == NVAPI_OK)
        {
            return PhFormatString(
                L"%lu.%lu [%hs]", 
                driverVersion / 100, 
                driverVersion % 100, 
                driverAndBranchString
                );
        }
    }

    if (NvApiInitialized && NvAPI_GetDisplayDriverVersion)
    {
        NV_DISPLAY_DRIVER_VERSION nvDisplayDriverVersion = { NV_DISPLAY_DRIVER_VERSION_VER };

        if (NvAPI_GetDisplayDriverVersion(NvGpuDisplayHandleList->Items[0], &nvDisplayDriverVersion) == NVAPI_OK)
        {
            return PhFormatString(
                L"%lu.%lu [%hs]", 
                nvDisplayDriverVersion.drvVersion / 100, 
                nvDisplayDriverVersion.drvVersion % 100,
                nvDisplayDriverVersion.szBuildBranchString
                );
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryVbiosVersionString(VOID)
{
    if (NvApiInitialized && NvAPI_GPU_GetVbiosVersionString)
    {
        NvAPI_ShortString biosRevision = "";

        if (NvAPI_GPU_GetVbiosVersionString(NvGpuPhysicalHandleList->Items[0], biosRevision) == NVAPI_OK)
        {
            return PhConvertMultiByteToUtf16(biosRevision);
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryName(VOID)
{
    if (NvApiInitialized && NvAPI_GPU_GetFullName)
    {
        NvAPI_ShortString nvNameAnsiString = "";

        if (NvAPI_GPU_GetFullName(NvGpuPhysicalHandleList->Items[0], nvNameAnsiString) == NVAPI_OK)
        {
            return PhConvertMultiByteToUtf16(nvNameAnsiString);
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryShortName(VOID)
{
    if (NvApiInitialized && NvAPI_GPU_GetShortName)
    {
        NvAPI_ShortString nvShortNameAnsiString = "";

        if (NvAPI_GPU_GetShortName(NvGpuPhysicalHandleList->Items[0], nvShortNameAnsiString) == NVAPI_OK)
        {
            return PhConvertMultiByteToUtf16(nvShortNameAnsiString);
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryRevision(VOID)
{
    if (NvApiInitialized && NvAPI_GPU_GetArchInfo)
    {
        NV_ARCH_INFO nvArchInfo = { NV_ARCH_INFO_VER };

        if (NvAPI_GPU_GetArchInfo(
            NvGpuPhysicalHandleList->Items[0], 
            &nvArchInfo
            ) == NVAPI_OK)
        {
            GpuArchType = nvArchInfo.unknown[0];

            return PhFormatString(L"%02X", nvArchInfo.unknown[2]);
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryRamType(VOID)
{
    if (NvApiInitialized)
    {
        PPH_STRING ramTypeString = NULL;
        PPH_STRING ramMakerString = NULL;
        NV_RAM_TYPE nvRamType = NV_RAM_TYPE_NONE;
        NV_RAM_MAKER nvRamMaker = NV_RAM_MAKER_NONE;

        if (NvAPI_GPU_GetRamType)
        {
            NvAPI_GPU_GetRamType(NvGpuPhysicalHandleList->Items[0], &nvRamType);
        }

        if (NvAPI_GPU_GetRamMaker)
        {
            NvAPI_GPU_GetRamMaker(NvGpuPhysicalHandleList->Items[0], &nvRamMaker);
        }

        switch (nvRamType)
        {
        case NV_RAM_TYPE_SDRAM:
            ramTypeString = PhaCreateString(L"SDRAM");
            break;
        case NV_RAM_TYPE_DDR1:
            ramTypeString = PhaCreateString(L"DDR1");
            break;
        case NV_RAM_TYPE_DDR2:
            ramTypeString = PhaCreateString(L"DDR2");
            break;
        case NV_RAM_TYPE_GDDR2:
            ramTypeString = PhaCreateString(L"GDDR2");
            break;
        case NV_RAM_TYPE_GDDR3:
            ramTypeString = PhaCreateString(L"GDDR3");
            break;
        case NV_RAM_TYPE_GDDR4:
            ramTypeString = PhaCreateString(L"GDDR4");
            break;
        case NV_RAM_TYPE_DDR3:
            ramTypeString = PhaCreateString(L"DDR3");
            break;
        case NV_RAM_TYPE_GDDR5:
            ramTypeString = PhaCreateString(L"GDDR5");
            break;
        case NV_RAM_TYPE_LPDDR2:
            ramTypeString = PhaCreateString(L"LPDDR2");
            break;
        default:
            ramTypeString = PhaFormatString(L"Unknown: %lu", nvRamType);
            break;
        }

        switch (nvRamMaker)
        {
        case NV_RAM_MAKER_SAMSUNG:
            ramMakerString = PhaCreateString(L"Samsung");
            break;
        case NV_RAM_MAKER_QIMONDA:
            ramMakerString = PhaCreateString(L"Qimonda");
            break;
        case NV_RAM_MAKER_ELPIDA:
            ramMakerString = PhaCreateString(L"Elpida");
            break;
        case NV_RAM_MAKER_ETRON:
            ramMakerString = PhaCreateString(L"Etron");
            break;
        case NV_RAM_MAKER_NANYA:
            ramMakerString = PhaCreateString(L"Nanya");
            break;
        case NV_RAM_MAKER_HYNIX:
            ramMakerString = PhaCreateString(L"Hynix");
            break;
        case NV_RAM_MAKER_MOSEL:
            ramMakerString = PhaCreateString(L"Mosel");
            break;
        case NV_RAM_MAKER_WINBOND:
            ramMakerString = PhaCreateString(L"Winbond");
            break;
        case NV_RAM_MAKER_ELITE:
            ramMakerString = PhaCreateString(L"Elite");
            break;
        case NV_RAM_MAKER_MICRON:
            ramMakerString = PhaCreateString(L"Micron");
            break;
        default:
            ramMakerString = PhaFormatString(L"Unknown: %lu", nvRamMaker);
            break;
        }

        return PhFormatString(L"%s (%s)", ramTypeString->Buffer, ramMakerString->Buffer);
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryFoundry(VOID)
{
    if (NvApiInitialized && NvAPI_GPU_GetFoundry)
    {
        NV_FOUNDRY nvFoundryType = NV_FOUNDRY_NONE;

        if (NvAPI_GPU_GetFoundry(
            NvGpuPhysicalHandleList->Items[0], 
            &nvFoundryType
            ) == NVAPI_OK)
        {
            switch (nvFoundryType)
            {
            case NV_FOUNDRY_TSMC:
                return PhCreateString(L"Taiwan Semiconductor Manufacturing Company (TSMC)");
            case NV_FOUNDRY_UMC:
                return PhCreateString(L"United Microelectronics Corporation (UMC)");
            case NV_FOUNDRY_IBM:
                return PhCreateString(L"IBM Microelectronics");
            case NV_FOUNDRY_SMIC:
                return PhCreateString(L"Semiconductor Manufacturing International Corporation (SMIC)");
            case NV_FOUNDRY_CSM:
                return PhCreateString(L"Chartered Semiconductor Manufacturing (CSM)");
            case NV_FOUNDRY_TOSHIBA:
                return PhCreateString(L"Toshiba Corporation");
            default:
                return PhFormatString(L"Unknown: %lu", nvFoundryType);
            }
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryDeviceId(VOID)
{
    if (NvApiInitialized && NvAPI_GPU_GetPCIIdentifiers)
    {
        NvU32 pDeviceId = 0;
        NvU32 pSubSystemId = 0;
        NvU32 pRevisionId = 0;
        NvU32 pExtDeviceId = 0;

        if (NvAPI_GPU_GetPCIIdentifiers(
            NvGpuPhysicalHandleList->Items[0], 
            &pDeviceId, 
            &pSubSystemId, 
            &pRevisionId, 
            &pExtDeviceId
            ) == NVAPI_OK)
        {
            return PhFormatString(L"%04X - %04X", pDeviceId & 65535, pDeviceId >> 16);
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryRopsCount(VOID)
{
    if (NvApiInitialized && NvAPI_GPU_GetPartitionCount)
    {
        NvU32 pCount = 0;

        if (NvAPI_GPU_GetPartitionCount(NvGpuPhysicalHandleList->Items[0], &pCount) == NVAPI_OK)
        {
            if (GpuArchType >= 0x120)
            {
                return PhFormatString(L"%lu", pCount * 16);
            }
            else if (GpuArchType >= 0x0c0)
            {
                return PhFormatString(L"%lu", pCount * 8);
            }
            else
            {
                return PhFormatString(L"%lu", pCount * 4);
            }
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryShaderCount(VOID)
{
    if (NvApiInitialized && NvAPI_GPU_GetGpuCoreCount)
    {
        NvU32 pCount = 0;

        if (NvAPI_GPU_GetGpuCoreCount(NvGpuPhysicalHandleList->Items[0], &pCount) == NVAPI_OK)
        {
            return PhFormatString(L"%lu Unified", pCount);
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryPciInfo(VOID)
{
    if (NvApiInitialized && NvAPI_GPU_GetPCIEInfo)
    {
        NV_PCIE_INFO pciInfo = { NV_PCIE_INFO_VER };

        if (NvAPI_GPU_GetPCIEInfo(NvGpuPhysicalHandleList->Items[0], &pciInfo) == NVAPI_OK)
        {
            return PhFormatString(L"%lu @ %lu %lu", 
                pciInfo.info[1].unknown1,
                pciInfo.info[0].unknown5,
                pciInfo.info[0].unknown6
                );
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryBusWidth(VOID)
{
    if (NvApiInitialized && NvAPI_GPU_GetFBWidthAndLocation)
    {
        NvU32 pWidth = 0;
        NvU32 pLocation = 0;

        if (NvAPI_GPU_GetFBWidthAndLocation(NvGpuPhysicalHandleList->Items[0], &pWidth, &pLocation) == NVAPI_OK)
        {
            return PhFormatString(L"%lu Bit", pWidth);
        }
    }

    return PhCreateString(L"N/A");
}


PPH_STRING NvGpuQueryPcbValue(VOID)
{
    if (NvApiInitialized && NvAPI_GPU_ClientPowerTopologyGetStatus)
    {
        NV_POWER_TOPOLOGY_STATUS nvPowerTopologyStatus = { NV_POWER_TOPOLOGY_STATUS_VER };

        if (NvAPI_GPU_ClientPowerTopologyGetStatus(NvGpuPhysicalHandleList->Items[0], &nvPowerTopologyStatus) == NVAPI_OK)
        {
            for (NvU32 i = 0; i < nvPowerTopologyStatus.count; i++)
            {
                NV_POWER_TOPOLOGY_2 powerTopology = nvPowerTopologyStatus.unknown[i];

                if (powerTopology.flags == NV_POWER_TOPOLOGY_FLAG_ENABLED)
                {
                    return PhFormatString(L"PMU: %.2f%%", (FLOAT)powerTopology.unknown.unknown2 / 1000);
                }
            }
        }
    }

    return PhCreateString(L"N/A");
}


PPH_STRING NvGpuQueryDriverSettings(VOID)
{
    if (NvApiInitialized && NvAPI_GetDisplayDriverRegistryPath)
    {
        NvAPI_LongString nvKeyPathAnsiString = "";

        if (NvAPI_GetDisplayDriverRegistryPath(NvGpuDisplayHandleList->Items[0], nvKeyPathAnsiString) == NVAPI_OK)
        {
            HANDLE keyHandle;
            PPH_STRING keyPath;

            keyPath = PhConvertMultiByteToUtf16(nvKeyPathAnsiString);

            if (NT_SUCCESS(PhOpenKey(
                &keyHandle,
                KEY_READ,
                PH_KEY_LOCAL_MACHINE,
                &keyPath->sr,
                0
                )))
            {
                PPH_STRING driverDateString = NULL;// PhQueryRegistryString(keyHandle, L"DriverDate");
                PPH_STRING driverVersionString = PhQueryRegistryString(keyHandle, L"DriverVersion");

                UNICODE_STRING valueName;
                PKEY_VALUE_PARTIAL_INFORMATION buffer = NULL;
                ULONG bufferSize;

                RtlInitUnicodeString(&valueName, L"DriverDateData");

                if (NtQueryValueKey(
                    keyHandle,
                    &valueName,
                    KeyValuePartialInformation,
                    NULL,
                    0,
                    &bufferSize
                    ) == STATUS_BUFFER_TOO_SMALL)
                {
                    buffer = PhAllocate(bufferSize);

                    if (NT_SUCCESS(NtQueryValueKey(
                        keyHandle,
                        &valueName,
                        KeyValuePartialInformation,
                        buffer,
                        bufferSize,
                        &bufferSize
                        )))
                    {
                        if (buffer->Type == REG_BINARY && buffer->DataLength == sizeof(FILETIME))
                        {
                            SYSTEMTIME systemTime;
                            SYSTEMTIME localTime;

                            FileTimeToSystemTime((const FILETIME*)buffer->Data, &systemTime);
                            SystemTimeToTzSpecificLocalTime(NULL, &systemTime, &localTime);

                            driverDateString = PhFormatDate(&localTime, NULL);
                        }
                    }

                    PhFree(buffer);
                }
                
                NtClose(keyHandle);
                PhDereferenceObject(keyPath);
                PhAutoDereferenceObject(driverVersionString);

                if (driverDateString)
                {
                    PhAutoDereferenceObject(driverDateString);
                    return PhFormatString(L"%s [%s]", driverVersionString->Buffer, driverDateString->Buffer);
                }
                else
                {
                    return PhFormatString(L"%s", driverVersionString->Buffer);
                }
            }

            PhDereferenceObject(keyPath);
        }
    }

    return PhCreateString(L"N/A");
}



BOOLEAN NvGpuDriverIsWHQL(VOID)
{
    BOOLEAN nvGpuDriverIsWHQL = FALSE;
    //NvAPI_LongString nvNameAnsiString = "";

    //HANDLE keyHandle;
    ////HANDLE keySettingsHandle;

    //PPH_STRING keyPath;
    //PPH_STRING matchingDeviceIdString;
    ////PPH_STRING keySettingsPath;
    //PPH_STRING keyServicePath;

    //WCHAR displayInstancePath[MAX_PATH] = L"";

    //HDEVINFO deviceInfoHandle = INVALID_HANDLE_VALUE;
    //SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
    //SP_DEVINFO_DATA deviceInfoData = { sizeof(SP_DEVINFO_DATA) };
    //PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetail = NULL;
    //ULONG deviceInfoLength = 0;

    //__try
    //{
    //    if (!NvApiInitialized)
    //        __leave;

    //    if (!NvAPI_GetDisplayDriverRegistryPath)
    //        __leave;

    //    if (NvAPI_GetDisplayDriverRegistryPath(NvGpuDisplayHandleList->Items[0], nvNameAnsiString) != NVAPI_OK)
    //        __leave;

    //    keyPath = PhConvertMultiByteToUtf16(nvNameAnsiString);

    //    if (!NT_SUCCESS(PhOpenKey(
    //        &keyHandle,
    //        KEY_READ,
    //        PH_KEY_LOCAL_MACHINE,
    //        &keyPath->sr,
    //        0
    //        )))
    //    {
    //        __leave;
    //    }

    //    matchingDeviceIdString = PhQueryRegistryString(keyHandle, L"MatchingDeviceId");

        //keySettingsPath = PhConcatStrings2(keyPath->Buffer, L"\\VolatileSettings");

        //if (NT_SUCCESS(PhOpenKey(
        //    &keySettingsHandle,
        //    KEY_READ,
        //    PH_KEY_LOCAL_MACHINE,
        //    &keySettingsPath->sr,
        //    0
        //    )))
        //{
        //    GUID settingsKey = GUID_DEVINTERFACE_DISPLAY_ADAPTER;
        //    PPH_STRING guidString = PhFormatGuid(&settingsKey);
        //
        //    ULONG dwType = REG_BINARY;
        //    LONG length = MAX_PATH;
        //
        //    if (RegQueryValueEx(
        //        keySettingsHandle,
        //        guidString->Buffer,
        //        0,
        //        &dwType,
        //        (PBYTE)displayInstancePath,
        //        &length
        //        ) != ERROR_SUCCESS)
        //    {
        //        //__leave;
        //    }
        //
        //    NtClose(keySettingsHandle);
        //    PhDereferenceObject(guidString);
        //}

    //    if ((deviceInfoHandle = SetupDiGetClassDevs(
    //        &GUID_DEVINTERFACE_DISPLAY_ADAPTER, 
    //        NULL, 
    //        NULL, 
    //        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    //        )) == INVALID_HANDLE_VALUE)
    //    {
    //        __leave;
    //    }

    //    for (ULONG i = 0; i < 1000; i++)
    //    {
    //        ULONG devicePropertyLength = 0;
    //        DEVPROPTYPE devicePropertyType = 0;
    //        HANDLE keyServiceHandle;
    //        WCHAR matchingDeviceId[MAX_PATH] = L"";
    //        WCHAR deviceServiceName[MAX_PATH] = L"";

    //        if (!SetupDiEnumDeviceInterfaces(deviceInfoHandle, 0, &GUID_DEVINTERFACE_DISPLAY_ADAPTER, i, &deviceInterfaceData))
    //            break;

    //        if (SetupDiGetDeviceInterfaceDetail(
    //            deviceInfoHandle,
    //            &deviceInterfaceData, 
    //            0, 
    //            0, 
    //            &deviceInfoLength, 
    //            &deviceInfoData
    //            ) || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    //        {
    //            continue;
    //        }

    //        deviceInterfaceDetail = PhAllocate(deviceInfoLength);
    //        deviceInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    //        if (!SetupDiGetDeviceInterfaceDetail(
    //            deviceInfoHandle, 
    //            &deviceInterfaceData, 
    //            deviceInterfaceDetail, 
    //            deviceInfoLength, 
    //            &deviceInfoLength,
    //            &deviceInfoData
    //            ))
    //        {
    //            continue;
    //        }

    //        if (!SetupDiGetDeviceProperty(
    //            deviceInfoHandle,
    //            &deviceInfoData,
    //            &DEVPKEY_Device_MatchingDeviceId,
    //            &devicePropertyType,
    //            (PBYTE)&matchingDeviceId,
    //            sizeof(matchingDeviceId),
    //            &devicePropertyLength,
    //            0
    //            ))
    //        {
    //            continue;
    //        }


    //        //if (PhEqualStringZ(deviceInterfaceDetail->DevicePath, displayInstancePath, TRUE))
    //        if (!PhEqualStringZ(matchingDeviceId, matchingDeviceIdString->Buffer, TRUE))
    //            continue;

    //        if (!SetupDiGetDeviceProperty(
    //            deviceInfoHandle,
    //            &deviceInfoData,
    //            &DEVPKEY_Device_Service,
    //            &devicePropertyType,
    //            (PBYTE)&deviceServiceName,
    //            sizeof(deviceServiceName),
    //            &devicePropertyLength,
    //            0
    //            ))
    //        {
    //            continue;
    //        }

    //        keyServicePath = PhConcatStrings2(L"System\\CurrentControlSet\\Services\\", deviceServiceName);

    //        if (NT_SUCCESS(PhOpenKey(
    //            &keyServiceHandle,
    //            KEY_READ,
    //            PH_KEY_LOCAL_MACHINE,
    //            &keyServicePath->sr,
    //            0
    //            )))
    //        {
    //            PPH_STRING driverNtPathString = NULL;
    //            PPH_STRING driverDosPathString = NULL;

    //            if (driverNtPathString = PhQueryRegistryString(keyServiceHandle, L"ImagePath"))
    //            {
    //                driverDosPathString = PhGetFileName(driverNtPathString);
    //                PhDereferenceObject(driverNtPathString);
    //            }

    //            if (driverDosPathString)
    //            {
    //                PPH_STRING fileSignerName = NULL;
    //                //PH_MAPPED_IMAGE fileMappedImage;
    //                //
    //                //if (NT_SUCCESS(PhLoadMappedImage(driverDosPathString->Buffer, NULL, TRUE, &fileMappedImage)))
    //                //{
    //                //    LARGE_INTEGER time;
    //                //    SYSTEMTIME systemTime;
    //                //    PPH_STRING string;
    //                //
    //                //    RtlSecondsSince1970ToTime(fileMappedImage.NtHeaders->FileHeader.TimeDateStamp, &time);
    //                //    PhLargeIntegerToLocalSystemTime(&systemTime, &time);
    //                //
    //                //    string = PhFormatDateTime(&systemTime);
    //                //    //SetDlgItemText(hwndDlg, IDC_TIMESTAMP, string->Buffer);
    //                //    PhDereferenceObject(string);
    //                //
    //                //    PhUnloadMappedImage(&fileMappedImage);
    //                //}

    //                if (PhVerifyFile(driverDosPathString->Buffer, &fileSignerName) == VrTrusted)
    //                {
    //                    if (PhEqualString2(fileSignerName, L"Microsoft Windows Hardware Compatibility Publisher", TRUE))
    //                    {
    //                        nvGpuDriverIsWHQL = TRUE;
    //                    }
    //                }

    //                if (fileSignerName)
    //                    PhDereferenceObject(fileSignerName);

    //                PhDereferenceObject(driverDosPathString);
    //            }

    //            NtClose(keyServiceHandle);
    //        }
    //    }
    //}
    //__finally
    //{
    //    if (deviceInfoHandle != INVALID_HANDLE_VALUE)
    //    {
    //        SetupDiDestroyDeviceInfoList(deviceInfoHandle);
    //    }

    //    if (keyHandle)
    //    {
    //        NtClose(keyHandle);
    //    }

    //    if (deviceInterfaceDetail)
    //    {
    //        PhFree(deviceInterfaceDetail);
    //    }

    //    if (keyPath)
    //    {
    //        PhDereferenceObject(keyPath);
    //    }
    //}

    return nvGpuDriverIsWHQL;
}

PPH_STRING NvGpuQueryFanSpeed(VOID)
{
    NvU32 tachValue = 0;
    NV_GPU_COOLER_SETTINGS coolerInfo = { NV_GPU_COOLER_SETTINGS_VER };

    if (NvApiInitialized)
    {
        if (NvAPI_GPU_GetTachReading(NvGpuPhysicalHandleList->Items[0], &tachValue) == NVAPI_OK)
        {
            if (NvAPI_GPU_GetCoolerSettings(NvGpuPhysicalHandleList->Items[0], NVAPI_COOLER_TARGET_ALL, &coolerInfo) == NVAPI_OK)
            {
                return PhFormatString(L"%lu RPM (%lu%%)", tachValue, coolerInfo.cooler[0].currentLevel);
            }

            return PhFormatString(L"%lu RPM", tachValue);
        }
        else
        {
            if (NvAPI_GPU_GetCoolerSettings(NvGpuPhysicalHandleList->Items[0], NVAPI_COOLER_TARGET_ALL, &coolerInfo) == NVAPI_OK)
            {
                return PhFormatString(L"%lu%%", coolerInfo.cooler[0].currentLevel);
            }
        }
    }

    return PhCreateString(L"N/A");
}




VOID NvGpuUpdate(VOID)
{
    NV_USAGES_INFO usagesInfo = { NV_USAGES_INFO_VER };
    NV_GPU_THERMAL_SETTINGS thermalSettings = { NV_GPU_THERMAL_SETTINGS_VER };
    NV_GPU_CLOCK_FREQUENCIES clkFreqs  = { NV_GPU_CLOCK_FREQUENCIES_VER };
    NV_CLOCKS_INFO clocksInfo = { NV_CLOCKS_INFO_VER };
    NV_VOLTAGE_DOMAINS voltageDomains = { NV_VOLTAGE_DOMAIN_INFO_VER };
    ULONG totalMemory = 0;
    ULONG sharedMemory = 0;
    ULONG freeMemory = 0;
    ULONG usedMemory = 0;

    if (!NvApiInitialized)
        return;

    for (ULONG i = 0; i < NvGpuDisplayHandleList->Count; i++)
    {
        NvDisplayHandle nvDisplayHandle;
        NV_DISPLAY_DRIVER_MEMORY_INFO memoryInfo = { NV_DISPLAY_DRIVER_MEMORY_INFO_VER };

        nvDisplayHandle = NvGpuDisplayHandleList->Items[i];

        if (NvAPI_GPU_GetMemoryInfo(nvDisplayHandle, &memoryInfo) == NVAPI_OK)
        {
            totalMemory += memoryInfo.dedicatedVideoMemory;
            sharedMemory += memoryInfo.sharedSystemMemory;
            freeMemory += memoryInfo.curAvailableDedicatedVideoMemory;
            usedMemory += totalMemory - freeMemory;
        }
    }

    GpuMemoryLimit = totalMemory;
    GpuCurrentMemUsage = usedMemory;
    GpuCurrentMemSharedUsage = sharedMemory;

    if (NvAPI_GPU_GetUsages(NvGpuPhysicalHandleList->Items[0], &usagesInfo) == NVAPI_OK)
    {
        GpuCurrentGpuUsage = (FLOAT)usagesInfo.usages[2] / 100;
        GpuCurrentCoreUsage = (FLOAT)usagesInfo.usages[6] / 100;
        GpuCurrentBusUsage = (FLOAT)usagesInfo.usages[14] / 100;
    }

    if (NvAPI_GPU_GetThermalSettings(NvGpuPhysicalHandleList->Items[0], NVAPI_THERMAL_TARGET_ALL, &thermalSettings) == NVAPI_OK)
    {
        GpuCurrentCoreTemp = thermalSettings.sensor[0].currentTemp;
        GpuCurrentBoardTemp = thermalSettings.sensor[1].currentTemp;
    }

    if (NvAPI_GPU_GetAllClockFrequencies(NvGpuPhysicalHandleList->Items[0], &clkFreqs) == NVAPI_OK)
    {
        //if (clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].bIsPresent)
        GpuCurrentCoreClock = clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].frequency / 1000;

        //if (clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY].bIsPresent)
        GpuCurrentMemoryClock = clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY].frequency / 1000;

        //if (clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR].bIsPresent)
        GpuCurrentShaderClock = clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR].frequency / 1000;
    }

    if (NvAPI_GPU_GetAllClocks(NvGpuPhysicalHandleList->Items[0], &clocksInfo) == NVAPI_OK)
    {
        if (GpuCurrentCoreClock == 0)
            GpuCurrentCoreClock = clocksInfo.clocks[0] / 1000;

        if (GpuCurrentMemoryClock == 0)
            GpuCurrentMemoryClock = clocksInfo.clocks[1] / 1000;

        if (GpuCurrentShaderClock == 0)
            GpuCurrentShaderClock = clocksInfo.clocks[2] / 1000;

        if (clocksInfo.clocks[30] != 0)
        {
            if (GpuCurrentCoreClock == 0)
                GpuCurrentCoreClock = (ULONG)(clocksInfo.clocks[30] * 0.0005f);

            if (GpuCurrentShaderClock == 0)
                GpuCurrentShaderClock = (ULONG)(clocksInfo.clocks[30] * 0.001f);
        }
    }

    if (NvAPI_GPU_GetVoltageDomainsStatus(NvGpuPhysicalHandleList->Items[0], &voltageDomains) == NVAPI_OK)
    {
        GpuCurrentVoltage = voltageDomains.domain[0].mvolt / 1000;

        //for (NvU32 i = 0; i < voltageDomains.max; i++)
        //{
        //    if (voltageDomains.domain[i].domainId == NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_CORE)
        //    {
        //        OutputDebugString(PhaFormatString(L"Voltage: [%lu] %lu\r\n", i, voltageDomains.domain[0].mvolt / 1000))->Buffer);
        //    }
        //}
    }

    //if (NvAPI_GPU_GetPerfDecreaseInfo(NvGpuPhysicalHandleList->Items[0], &GpuPerfDecreaseReason) != NVAPI_OK)
    //{
    //    GpuPerfDecreaseReason = NV_GPU_PERF_DECREASE_REASON_UNKNOWN;
    //}

    //NvU32 totalApps = 0;
    //NV_ACTIVE_APPS activeApps[NVAPI_MAX_PROCESSES] = { NV_ACTIVE_APPS_INFO_VER };
    //NvAPI_GPU_QueryActiveApps(NvGpuPhysicalHandleList->Items[0], activeApps, &totalApps);

    //NV_VOLTAGES voltages = { NV_VOLTAGES_INFO_VER };
    //NvAPI_GPU_GetVoltages(NvGpuPhysicalHandleList->Items[0], &voltages);

    //Nv120 clockInfo = { NV_PERF_CLOCKS_INFO_VER };
    //NvAPI_GPU_GetPerfClocks(NvGpuPhysicalHandleList->Items[0], 0, &clockInfo);

    PhAddItemCircularBuffer_FLOAT(&GpuUtilizationHistory, GpuCurrentGpuUsage);
    PhAddItemCircularBuffer_ULONG(&GpuMemoryHistory, GpuCurrentMemUsage);
    PhAddItemCircularBuffer_FLOAT(&GpuBoardHistory, GpuCurrentCoreUsage);
    PhAddItemCircularBuffer_FLOAT(&GpuBusHistory, GpuCurrentBusUsage);
}

#endif