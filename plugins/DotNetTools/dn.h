/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2015-2021
 *
 */

#ifndef DN_H
#define DN_H

#include <phdk.h>
#include <workqueue.h>
#include <settings.h>

#include "resource.h"

#define PLUGIN_NAME L"ProcessHacker.DotNetTools"
#define SETTING_NAME_ASM_TREE_LIST_COLUMNS (PLUGIN_NAME L".AsmTreeListColumns")
#define SETTING_NAME_ASM_TREE_LIST_FLAGS (PLUGIN_NAME L".AsmTreeListFlags")
#define SETTING_NAME_ASM_TREE_LIST_SORT (PLUGIN_NAME L".AsmTreeListSort")
#define SETTING_NAME_DOT_NET_CATEGORY_INDEX (PLUGIN_NAME L".DotNetCategoryIndex")
#define SETTING_NAME_DOT_NET_COUNTERS_COLUMNS (PLUGIN_NAME L".DotNetListColumns")
#define SETTING_NAME_DOT_NET_COUNTERS_SORTCOLUMN (PLUGIN_NAME L".DotNetListSort")
#define SETTING_NAME_DOT_NET_COUNTERS_GROUPSTATES (PLUGIN_NAME L".DotNetListGroupStates")

extern PPH_PLUGIN PluginInstance;

#define DN_ASM_MENU_HIDE_DYNAMIC_OPTION 1
#define DN_ASM_MENU_HIGHLIGHT_DYNAMIC_OPTION 2
#define DN_ASM_MENU_HIDE_NATIVE_OPTION 3
#define DN_ASM_MENU_HIGHLIGHT_NATIVE_OPTION 4

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

_Success_(return)
BOOLEAN OpenDotNetPublicControlBlock_V2(
    _In_ HANDLE ProcessId,
    _Out_ PVOID* BlockTableAddress
    );

_Success_(return)
BOOLEAN OpenDotNetPublicControlBlock_V4(
    _In_ BOOLEAN IsImmersive,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId,
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
