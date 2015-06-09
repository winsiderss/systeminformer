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

#include "main.h"
#include "nvapi\nvapi.h"
#include "NvGpuPlugin.h"

static HMODULE NvApiLibrary = NULL;
static PPH_LIST NvGpuPhysicalHandleList = NULL;
static PPH_LIST NvGpuDisplayHandleList = NULL;

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

BOOLEAN InitializeNvApi(VOID)
{
    if (NvApiInitialized)
        return TRUE;

    NvGpuPhysicalHandleList = PhCreateList(1);
    NvGpuDisplayHandleList = PhCreateList(1);

#ifdef _M_IX86
    NvApiLibrary = LoadLibrary(L"nvapi.dll");
#else
    NvApiLibrary = LoadLibrary(L"nvapi64.dll");
#endif

    if (!NvApiLibrary)
        return FALSE;

    // Retrieve the NvAPI_QueryInterface function address
    if (!(NvAPI_QueryInterface = PhGetProcedureAddress(NvApiLibrary, "nvapi_QueryInterface", 0)))
        return FALSE;

    // Initialization functions
    if (!(NvAPI_Initialize = (_NvAPI_Initialize)NvAPI_QueryInterface(0x150E828)))
        return FALSE;
    if (!(NvAPI_Unload = (_NvAPI_Unload)NvAPI_QueryInterface(0xD22BDD7E)))
        return FALSE;

    // Error functions
    if (!(NvAPI_GetErrorMessage = (_NvAPI_GetErrorMessage)NvAPI_QueryInterface(0x6C2D048Cu)))
        return FALSE;

    // Handle functions
    if (!(NvAPI_EnumPhysicalGPUs = (_NvAPI_EnumPhysicalGPUs)NvAPI_QueryInterface(0xE5AC921F)))
        return FALSE;
    if (!(NvAPI_EnumNvidiaDisplayHandle = (_NvAPI_EnumNvidiaDisplayHandle)NvAPI_QueryInterface(0x9ABDD40D)))
        return FALSE;

    // Information functions
    if (!(NvAPI_SYS_GetDriverAndBranchVersion = (_NvAPI_SYS_GetDriverAndBranchVersion)NvAPI_QueryInterface(0x2926AAAD)))
        return FALSE;
    if (!(NvAPI_GPU_GetFullName = (_NvAPI_GPU_GetFullName)NvAPI_QueryInterface(0xCEEE8E9F)))
        return FALSE;

    // Query functions
    if (!(NvAPI_GPU_GetMemoryInfo = (_NvAPI_GPU_GetMemoryInfo)NvAPI_QueryInterface(0x774AA982)))
        return FALSE;
    if (!(NvAPI_GPU_GetThermalSettings = (_NvAPI_GPU_GetThermalSettings)NvAPI_QueryInterface(0xE3640A56)))
        return FALSE;
    if (!(NvAPI_GPU_GetCoolerSettings = (_NvAPI_GPU_GetCoolerSettings)NvAPI_QueryInterface(0xDA141340)))
        return FALSE;
    if (!( NvAPI_GPU_GetPerfDecreaseInfo = (_NvAPI_GPU_GetPerfDecreaseInfo)NvAPI_QueryInterface(0x7F7F4600)))
        return FALSE;
    if (!(NvAPI_GPU_GetTachReading = (_NvAPI_GPU_GetTachReading)NvAPI_QueryInterface(0x5F608315)))
        return FALSE;
    if (!(NvAPI_GPU_GetAllClockFrequencies = (_NvAPI_GPU_GetAllClockFrequencies)NvAPI_QueryInterface(0xDCB616C3)))
        return FALSE;

    // Undocumented Query functions
    if (!(NvAPI_GPU_GetUsages = (_NvAPI_GPU_GetUsages)NvAPI_QueryInterface(0x189A1FDF)))
        return FALSE;
    if (!(NvAPI_GPU_GetAllClocks = (_NvAPI_GPU_GetAllClocks)NvAPI_QueryInterface(0x1BD69F49)))
        return FALSE;
    if (!(NvAPI_GPU_GetVoltageDomainsStatus = (_NvAPI_GPU_GetVoltageDomainsStatus)NvAPI_QueryInterface(0xC16C7E2C)))
        return FALSE;

    //NvAPI_GPU_GetPerfClocks = (_NvAPI_GPU_GetPerfClocks)NvAPI_QueryInterface(0x1EA54A3B);
    //NvAPI_GPU_GetVoltages = (_NvAPI_GPU_GetVoltages)NvAPI_QueryInterface(0x7D656244);
    //NvAPI_GPU_QueryActiveApps = (_NvAPI_GPU_QueryActiveApps)NvAPI_QueryInterface(0x65B1C5F5);

    //NvAPI_GPU_GetShaderPipeCount = (_NvAPI_GPU_GetShaderPipeCount)NvAPI_QueryInterface(0x63E2F56F);
    //NvAPI_GPU_GetShaderSubPipeCount = (_NvAPI_GPU_GetShaderSubPipeCount)NvAPI_QueryInterface(0x0BE17923);
    //NvAPI_GPU_GetRamType = (_NvAPI_GPU_GetRamType)NvAPI_QueryInterface(0x57F7CAAC);

    //NvAPI_GetDisplayDriverMemoryInfo = (_NvAPI_GetDisplayDriverMemoryInfo)NvAPI_QueryInterface(0x774AA982);

    //if (!(NvAPI_GetPhysicalGPUsFromDisplay = (_NvAPI_GetPhysicalGPUsFromDisplay)NvAPI_QueryInterface(0x34EF9506)))
    //    return FALSE;
    //if (!(NvAPI_GPU_GetBoardInfo = (_NvAPI_GPU_GetBoardInfo)NvAPI_QueryInterface(0x22D54523)))
    //    return FALSE;
    //if (!(NvAPI_SYS_GetChipSetInfo = (_NvAPI_SYS_GetChipSetInfo)NvAPI_QueryInterface(0x53DABBCA)))
    //    return FALSE;
    //if (!(NvAPI_GPU_GetBusType = (_NvAPI_GPU_GetBusType)NvAPI_QueryInterface(0x1BB18724)))
    //    return FALSE;
    //if (!(NvAPI_GPU_GetVbiosVersionString = (_NvAPI_GPU_GetVbiosVersionString)NvAPI_QueryInterface(0xA561FD7D)))
    //    return FALSE;
    //if (!(NvAPI_GPU_GetIRQ = (_NvAPI_GPU_GetIRQ)NvAPI_QueryInterface(0xE4715417)))
    //    return FALSE;

    if (NvAPI_Initialize() == NVAPI_OK)
    {
        NvGpuEnumPhysicalHandles();
        NvGpuEnumDisplayHandles();

        return TRUE;
    }

    return FALSE;
}

BOOLEAN DestroyNvApi(VOID)
{
    NvApiInitialized = FALSE;

    if (NvGpuDisplayHandleList)
    {
        PhClearList(NvGpuDisplayHandleList);
        PhDereferenceObject(NvGpuDisplayHandleList);
    }

    if (NvGpuPhysicalHandleList)
    {
        PhClearList(NvGpuPhysicalHandleList);
        PhDereferenceObject(NvGpuPhysicalHandleList);
    }

    if (NvAPI_Unload)
        NvAPI_Unload();

    if (NvApiLibrary)
        FreeLibrary(NvApiLibrary);

    return TRUE;
}

PPH_STRING NvGpuQueryDriverVersion(VOID)
{
    NvU32 driverVersion = 0;
    NvAPI_ShortString driverAndBranchString = "";

    if (NvApiInitialized)
    {
        if (NvAPI_SYS_GetDriverAndBranchVersion(&driverVersion, driverAndBranchString) == NVAPI_OK)
        {
            return PhFormatString(L"%lu.%lu", driverVersion / 100, driverVersion % 100);
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryVbiosVersionString(VOID)
{
    NvAPI_ShortString biosRevision = "";

    if (NvApiInitialized)
    {
        if (NvAPI_GPU_GetVbiosVersionString(NvGpuPhysicalHandleList->Items[0], biosRevision) == NVAPI_OK)
        {
            return PhFormatString(L"Video BIOS version: %hs", biosRevision);
        }
    }

    return PhCreateString(L"Video BIOS version: N/A");
}

PPH_STRING NvGpuQueryName(VOID)
{
    NvAPI_ShortString nvNameAnsiString = "";

    if (NvApiInitialized)
    {
        if (NvAPI_GPU_GetFullName(NvGpuPhysicalHandleList->Items[0], nvNameAnsiString) == NVAPI_OK)
        {
            return PhConvertMultiByteToUtf16(nvNameAnsiString);
        }
    }

    return PhCreateString(L"N/A");
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

VOID NvGpuUpdateValues(VOID)
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
}