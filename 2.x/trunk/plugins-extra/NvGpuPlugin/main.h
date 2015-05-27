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

#ifndef _NVGPU_H_
#define _NVGPU_H_

#define PLUGIN_NAME L"dmex.NvGpuPlugin"
#define SETTING_NAME_ENABLE_MONITORING (PLUGIN_NAME L".Enabled")
#define SETTING_NAME_ENABLE_FAHRENHEIT (PLUGIN_NAME L".EnableFahrenheit")

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>

#include "resource.h"

extern BOOLEAN NvApiInitialized;
extern PPH_PLUGIN PluginInstance;

extern ULONG GpuMemoryLimit;
extern FLOAT GpuCurrentGpuUsage;
extern FLOAT GpuCurrentCoreUsage;
extern FLOAT GpuCurrentBusUsage;
extern ULONG GpuCurrentMemUsage;
extern ULONG GpuCurrentMemSharedUsage;
extern ULONG GpuCurrentCoreTemp;
extern ULONG GpuCurrentBoardTemp;
extern ULONG GpuCurrentCoreClock;
extern ULONG GpuCurrentMemoryClock;
extern ULONG GpuCurrentShaderClock;
extern ULONG GpuCurrentVoltage;

BOOLEAN InitializeNvApi(VOID);
BOOLEAN DestroyNvApi(VOID);

PPH_STRING NvGpuQueryDriverVersion(VOID);
PPH_STRING NvGpuQueryVbiosVersionString(VOID);
PPH_STRING NvGpuQueryName(VOID);
PPH_STRING NvGpuQueryFanSpeed(VOID);

VOID NvGpuUpdateValues(VOID);

typedef struct _PH_NVGPU_SYSINFO_CONTEXT
{
    PPH_STRING GpuName;
    HWND WindowHandle;
    PPH_SYSINFO_SECTION Section;
    PH_LAYOUT_MANAGER LayoutManager;

    RECT GpuGraphMargin;
    HWND GpuPanel;

    HWND GpuLabelHandle;
    HWND MemLabelHandle;
    HWND SharedLabelHandle;
    HWND BusLabelHandle;

    HWND GpuGraphHandle;
    HWND MemGraphHandle;
    HWND SharedGraphHandle;
    HWND BusGraphHandle;

    PH_GRAPH_STATE GpuGraphState;
    PH_GRAPH_STATE MemGraphState;
    PH_GRAPH_STATE SharedGraphState;
    PH_GRAPH_STATE BusGraphState;

    PH_CIRCULAR_BUFFER_FLOAT GpuUtilizationHistory;
    PH_CIRCULAR_BUFFER_ULONG GpuMemoryHistory;
    PH_CIRCULAR_BUFFER_FLOAT GpuBoardHistory;
    PH_CIRCULAR_BUFFER_FLOAT GpuBusHistory;

    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
} PH_NVGPU_SYSINFO_CONTEXT, *PPH_NVGPU_SYSINFO_CONTEXT;

VOID NvGpuSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers
    );

VOID ShowOptionsDialog(
    _In_ HWND ParentHandle
    );

#endif _NVGPU_H_