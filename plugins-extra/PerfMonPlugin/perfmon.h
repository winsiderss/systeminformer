/*
 * Process Hacker Extra Plugins -
 *   Performance Monitor Plugin
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

#ifndef _PERFMON_H_
#define _PERFMON_H_

#pragma comment(lib, "pdh.lib")

#define SETTING_PREFIX L"dmex.PerfMonPlugin"
#define SETTING_NAME_PERFMON_LIST (SETTING_PREFIX L".CountersList")

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>
#include <pdh.h>
#include <pdhmsg.h>

#include "resource.h"

extern PPH_PLUGIN PluginInstance;
extern PPH_LIST CountersList;

typedef struct _PH_PERFMON_ENTRY
{
    PPH_STRING Name;
} PH_PERFMON_ENTRY, *PPH_PERFMON_ENTRY;

typedef struct _PH_PERFMON_CONTEXT
{
    HWND ListViewHandle;
    PPH_LIST CountersListEdited;
} PH_PERFMON_CONTEXT, *PPH_PERFMON_CONTEXT;

typedef struct _PH_PERFMON_SYSINFO_CONTEXT
{       
    HQUERY PerfQueryHandle;       
    HCOUNTER PerfCounterHandle;

    ULONG GraphValue;
    HWND WindowHandle;
    HWND GraphHandle;

    PPH_SYSINFO_SECTION SysinfoSection;
    PH_GRAPH_STATE GraphState;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    PH_CIRCULAR_BUFFER_ULONG HistoryBuffer;
} PH_PERFMON_SYSINFO_CONTEXT, *PPH_PERFMON_SYSINFO_CONTEXT;

VOID LoadCounterList(
    _Inout_ PPH_LIST FilterList,
    _In_ PPH_STRING String
    );

VOID ShowOptionsDialog(
    _In_ HWND ParentHandle
    );

VOID PerfCounterSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ PPH_STRING CounterName
    );

#endif _ROTHEADER_