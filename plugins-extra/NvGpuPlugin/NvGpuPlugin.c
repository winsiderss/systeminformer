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
FLOAT GpuCurrentCoreClock = 0.0f;
FLOAT GpuCurrentMemoryClock = 0.0f;
FLOAT GpuCurrentShaderClock = 0.0f;
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
    if (!(NvAPI_QueryInterface = (_NvAPI_QueryInterface)GetProcAddress(NvApiLibrary, "nvapi_QueryInterface")))
        return FALSE;

    // Initialization functions
    NvAPI_Initialize = (_NvAPI_Initialize)NvAPI_QueryInterface(0x150E828);
    NvAPI_Unload = (_NvAPI_Unload)NvAPI_QueryInterface(0xD22BDD7E);

    // Error functions
    NvAPI_GetErrorMessage = (_NvAPI_GetErrorMessage)NvAPI_QueryInterface(0x6C2D048Cu);

    // Handle functions
    NvAPI_EnumPhysicalGPUs = (_NvAPI_EnumPhysicalGPUs)NvAPI_QueryInterface(0xE5AC921F);
    NvAPI_EnumNvidiaDisplayHandle = (_NvAPI_EnumNvidiaDisplayHandle)NvAPI_QueryInterface(0x9ABDD40D);

    // Information functions
    NvAPI_SYS_GetDriverAndBranchVersion = (_NvAPI_SYS_GetDriverAndBranchVersion)NvAPI_QueryInterface(0x2926AAAD);
    NvAPI_GPU_GetFullName = (_NvAPI_GPU_GetFullName)NvAPI_QueryInterface(0xCEEE8E9F);

    // Undocumented Query functions
    NvAPI_GPU_GetUsages = (_NvAPI_GPU_GetUsages)NvAPI_QueryInterface(0x189A1FDF);
    NvAPI_GPU_GetAllClocks = (_NvAPI_GPU_GetAllClocks)NvAPI_QueryInterface(0x1BD69F49);
    //NvAPI_GPU_GetVoltages
    //NvAPI_GPU_QueryActiveApps

    // Query functions
    NvAPI_GPU_GetMemoryInfo = (_NvAPI_GPU_GetMemoryInfo)NvAPI_QueryInterface(0x774AA982);
    NvAPI_GetPhysicalGPUsFromDisplay = (_NvAPI_GetPhysicalGPUsFromDisplay)NvAPI_QueryInterface(0x34EF9506);
    NvAPI_GPU_GetThermalSettings = (_NvAPI_GPU_GetThermalSettings)NvAPI_QueryInterface(0xE3640A56);
    NvAPI_GPU_GetCoolerSettings = (_NvAPI_GPU_GetCoolerSettings)NvAPI_QueryInterface(0xDA141340);
    NvAPI_GPU_GetPerfDecreaseInfo = (_NvAPI_GPU_GetPerfDecreaseInfo)NvAPI_QueryInterface(0x7F7F4600);
    NvAPI_GPU_GetTachReading = (_NvAPI_GPU_GetTachReading)NvAPI_QueryInterface(0x5F608315);
    NvAPI_GPU_GetAllClockFrequencies = (_NvAPI_GPU_GetAllClockFrequencies)NvAPI_QueryInterface(0xDCB616C3);

    //NvAPI_GPU_GetBoardInfo = (_NvAPI_GPU_GetBoardInfo)NvAPI_QueryInterface(0x22D54523);
    //NvAPI_SYS_GetChipSetInfo = (_NvAPI_SYS_GetChipSetInfo)NvAPI_QueryInterface(0x53DABBCA);
    //NvAPI_GPU_GetBusType = (_NvAPI_GPU_GetBusType)NvAPI_QueryInterface(0x1BB18724);
    NvAPI_GPU_GetVbiosVersionString = (_NvAPI_GPU_GetVbiosVersionString)NvAPI_QueryInterface(0xA561FD7D);
    NvAPI_GPU_GetIRQ = (_NvAPI_GPU_GetIRQ)NvAPI_QueryInterface(0xE4715417);

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

    if (NvAPI_Unload)
        NvAPI_Unload();

    PhClearList(NvGpuDisplayHandleList);
    PhClearList(NvGpuPhysicalHandleList);

    PhDereferenceObject(NvGpuDisplayHandleList);
    PhDereferenceObject(NvGpuPhysicalHandleList);

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
            return PhFormatString(L"%u.%u", driverVersion / 100, driverVersion % 100);
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
            return PhCreateStringFromAnsi(nvNameAnsiString);
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING NvGpuQueryIRQ(VOID)
{
    NvU32 irq = 0;

    if (NvApiInitialized)
    {
        if (NvAPI_GPU_GetIRQ(NvGpuPhysicalHandleList->Items[0], &irq) == NVAPI_OK)
        {
            return PhFormatString(L"IRQ: %u", irq);
        }
    }

    return PhCreateString(L"IRQ: N/A");
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
                return PhFormatString(L"%u RPM (%u%%)", tachValue, coolerInfo.cooler[0].currentLevel);
            }

            return PhFormatString(L"%u RPM", tachValue);
        }
        else
        { 
            if (NvAPI_GPU_GetCoolerSettings(NvGpuPhysicalHandleList->Items[0], NVAPI_COOLER_TARGET_ALL, &coolerInfo) == NVAPI_OK)
            {
                return PhFormatString(L"%u%%", coolerInfo.cooler[0].currentLevel);
            }
        }
    }

    return PhCreateString(L"N/A");
}

VOID NvGpuUpdateValues(VOID)
{
    NV_USAGES_INFO usagesInfo = { NV_USAGES_INFO_VER };
    NV_DISPLAY_DRIVER_MEMORY_INFO memoryInfo = { NV_DISPLAY_DRIVER_MEMORY_INFO_VER };
    NV_GPU_THERMAL_SETTINGS thermalSettings = { NV_GPU_THERMAL_SETTINGS_VER };
    NV_GPU_CLOCK_FREQUENCIES clkFreqs  = { NV_GPU_CLOCK_FREQUENCIES_VER };
    NV_CLOCKS_INFO_V2 clocksInfo = { NV_CLOCKS_INFO_VER };

    if (!NvApiInitialized)
        return;

    if (NvAPI_GPU_GetMemoryInfo(NvGpuDisplayHandleList->Items[0], &memoryInfo) == NVAPI_OK)
    {
        ULONG totalMemory = memoryInfo.dedicatedVideoMemory;
        ULONG sharedMemory = memoryInfo.sharedSystemMemory;
        ULONG freeMemory = memoryInfo.curAvailableDedicatedVideoMemory;
        ULONG usedMemory = totalMemory - freeMemory;

        GpuMemoryLimit = totalMemory;
        GpuCurrentMemUsage = usedMemory;
        GpuCurrentMemSharedUsage = sharedMemory;
    }

    if (NvAPI_GPU_GetUsages(NvGpuPhysicalHandleList->Items[0], &usagesInfo) == NVAPI_OK)
    {
        GpuCurrentGpuUsage = (FLOAT)usagesInfo.Values[2] / 100;
        GpuCurrentCoreUsage = (FLOAT)usagesInfo.Values[6] / 100;
        GpuCurrentBusUsage = (FLOAT)usagesInfo.Values[14] / 100;
    }

    if (NvAPI_GPU_GetThermalSettings(NvGpuPhysicalHandleList->Items[0], NVAPI_THERMAL_TARGET_ALL, &thermalSettings) == NVAPI_OK)
    {
        GpuCurrentCoreTemp = thermalSettings.sensor[0].currentTemp;
        GpuCurrentBoardTemp = thermalSettings.sensor[1].currentTemp;
    }

    if (NvAPI_GPU_GetAllClockFrequencies(NvGpuPhysicalHandleList->Items[0], &clkFreqs) == NVAPI_OK)
    {
        GpuCurrentCoreClock = (FLOAT)clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].frequency * 0.001f;
        GpuCurrentMemoryClock = (FLOAT)clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY].frequency * 0.001f;
        GpuCurrentShaderClock = (FLOAT)clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR].frequency * 0.001f;
    }

    if (NvAPI_GPU_GetAllClocks(NvGpuPhysicalHandleList->Items[0], &clocksInfo) == NVAPI_OK)
    {
        if (GpuCurrentCoreClock == 0)
            GpuCurrentCoreClock = (FLOAT)clocksInfo.Values[0] * 0.001f;

        if (GpuCurrentMemoryClock == 0)       
            GpuCurrentMemoryClock = (FLOAT)clocksInfo.Values[1] * 0.001f;

        if (GpuCurrentShaderClock == 0)
            GpuCurrentShaderClock = (FLOAT)clocksInfo.Values[2] * 0.001f;

        if (clocksInfo.Values[30] != 0)
        {
            if (GpuCurrentCoreClock == 0)
                GpuCurrentCoreClock = clocksInfo.Values[30] * 0.0005f;

            if (GpuCurrentShaderClock == 0)
                GpuCurrentShaderClock = clocksInfo.Values[30] * 0.001f;
        }
    }

    //if (NvAPI_GPU_GetPerfDecreaseInfo(NvGpuPhysicalHandleList->Items[0], &GpuPerfDecreaseReason) != NVAPI_OK)
    //{
    //    GpuPerfDecreaseReason = NV_GPU_PERF_DECREASE_REASON_UNKNOWN;
    //}
}