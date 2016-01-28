/*
 * Process Hacker .NET Tools
 *
 * Copyright (C) 2011-2015 wj32
 * Copyright (C) 2015-2016 dmex
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

#ifndef DN_H
#define DN_H

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <windowsx.h>

#include "resource.h"

#define PLUGIN_NAME L"ProcessHacker.DotNetTools"
#define SETTING_NAME_ASM_TREE_LIST_COLUMNS (PLUGIN_NAME L".AsmTreeListColumns")
#define SETTING_NAME_DOT_NET_CATEGORY_INDEX (PLUGIN_NAME L".DotNetCategoryIndex")
#define SETTING_NAME_DOT_NET_COUNTERS_COLUMNS (PLUGIN_NAME L".DotNetListColumns")

#define MSG_UPDATE (WM_APP + 1)

extern PPH_PLUGIN PluginInstance;

typedef struct _DN_THREAD_ITEM
{
    PPH_THREAD_ITEM ThreadItem;

    BOOLEAN ClrDataValid;
    PPH_STRING AppDomainText;
} DN_THREAD_ITEM, *PDN_THREAD_ITEM;

// counters

PVOID GetPerfIpcBlock_V2(
    _In_ BOOLEAN Wow64,
    _In_ PVOID BlockTableAddress
    );

PVOID GetPerfIpcBlock_V4(
    _In_ BOOLEAN Wow64,
    _In_ PVOID BlockTableAddress
    );

BOOLEAN OpenDotNetPublicControlBlock_V2(
    _In_ HANDLE ProcessId,
    _Out_ HANDLE* BlockTableHandle,
    _Out_ PVOID* BlockTableAddress
    );

BOOLEAN OpenDotNetPublicControlBlock_V4(
    _In_ BOOLEAN IsImmersive,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId,
    _Out_ HANDLE* BlockTableHandle,
    _Out_ PVOID* BlockTableAddress
    );

PPH_LIST QueryDotNetAppDomainsForPid_V2(
    _In_ BOOLEAN Wow64,
    _In_ HANDLE ProcessHandle, 
    _In_ HANDLE ProcessId
    );

PPH_LIST QueryDotNetAppDomainsForPid_V4(
    _In_ BOOLEAN Wow64,
    _In_ HANDLE ProcessHandle, 
    _In_ HANDLE ProcessId
    );

// asmpage

VOID AddAsmPageToPropContext(
    _In_ PPH_PLUGIN_PROCESS_PROPCONTEXT PropContext
    );

// perfpage

VOID AddPerfPageToPropContext(
    _In_ PPH_PLUGIN_PROCESS_PROPCONTEXT PropContext
    );

// stackext

VOID ProcessThreadStackControl(
    _In_ PPH_PLUGIN_THREAD_STACK_CONTROL Control
    );

VOID PredictAddressesFromClrData(
    _In_ struct _CLR_PROCESS_SUPPORT *Support,
    _In_ HANDLE ThreadId,
    _In_ PVOID PcAddress,
    _In_ PVOID FrameAddress,
    _In_ PVOID StackAddress,
    _Out_ PVOID *PredictedEip,
    _Out_ PVOID *PredictedEbp,
    _Out_ PVOID *PredictedEsp
    );

// svcext

VOID DispatchPhSvcRequest(
    _In_ PVOID Parameter
    );

// treeext

VOID InitializeTreeNewObjectExtensions(
    VOID
    );

VOID DispatchTreeNewMessage(
    __in PVOID Parameter
    );

#define DNTHTNC_APPDOMAIN 1

VOID ThreadTreeNewInitializing(
    __in PVOID Parameter
    );

VOID ThreadTreeNewUninitializing(
    __in PVOID Parameter
    );

#endif
