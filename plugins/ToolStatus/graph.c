/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2023
 *
 */

#include "toolstatus.h"

static PPH_LIST PhpToolbarGraphList = NULL;
static PPH_HASHTABLE PhpToolbarGraphHashtable = NULL;

VOID ToolbarGraphLoadSettings(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRINGREF remaining;

    settingsString = PhGetStringSetting(SETTING_NAME_TOOLBAR_GRAPH_CONFIG);

    if (PhIsNullOrEmptyString(settingsString))
        return;

    remaining = PhGetStringRef(settingsString);

    while (remaining.Length != 0)
    {
        PH_STRINGREF idPart;
        PH_STRINGREF flagsPart;
        PH_STRINGREF pluginNamePart;
        ULONG64 idInteger;
        ULONG64 flagsInteger;

        PhSplitStringRefAtChar(&remaining, L'|', &idPart, &remaining);
        PhSplitStringRefAtChar(&remaining, L'|', &flagsPart, &remaining);
        PhSplitStringRefAtChar(&remaining, L'|', &pluginNamePart, &remaining);

        if (!PhStringToUInt64(&idPart, 10, &idInteger))
            break;
        if (!PhStringToUInt64(&flagsPart, 10, &flagsInteger))
            break;

        if (flagsInteger)
        {
            PPH_TOOLBAR_GRAPH graph;

            if (pluginNamePart.Length)
            {
                if (graph = ToolbarGraphFindByName(&pluginNamePart, (ULONG)idInteger))
                {
                    SetFlag(graph->Flags, TOOLSTATUS_GRAPH_ENABLED);
                }
            }
            else
            {
                if (graph = ToolbarGraphFindById((ULONG)idInteger))
                {
                    SetFlag(graph->Flags, TOOLSTATUS_GRAPH_ENABLED);
                }
            }
        }
    }

    PhDereferenceObject(settingsString);
}

VOID ToolbarGraphSaveSettings(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRING_BUILDER graphListBuilder;

    PhInitializeStringBuilder(&graphListBuilder, 100);

    for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
    {
        PPH_TOOLBAR_GRAPH graph = PhpToolbarGraphList->Items[i];
        PPH_STRING pluginName;

        if (!FlagOn(graph->Flags, TOOLSTATUS_GRAPH_ENABLED))
            continue;

        pluginName = graph->Plugin ? PhGetPluginName(graph->Plugin) : NULL;

        PhAppendFormatStringBuilder(
            &graphListBuilder,
            L"%lu|%lu|%s|",
            graph->GraphId,
            FlagOn(graph->Flags, TOOLSTATUS_GRAPH_ENABLED) ? 1 : 0,
            PhGetStringOrEmpty(pluginName)
            );

        if (pluginName)
            PhDereferenceObject(pluginName);
    }

    if (graphListBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&graphListBuilder, 1);

    settingsString = PhFinalStringBuilderString(&graphListBuilder);
    PhSetStringSetting2(SETTING_NAME_TOOLBAR_GRAPH_CONFIG, &settingsString->sr);
    PhDereferenceObject(settingsString);
}

VOID ToolbarGraphsInitialize(
    VOID
    )
{
    if (!PhpToolbarGraphList)
    {
        PhpToolbarGraphList = PhCreateList(10);
        PhpToolbarGraphHashtable = PhCreateSimpleHashtable(10);
    }

    ToolbarRegisterGraph(
        PluginInstance,
        1,
        L"CPU history",
        0,
        NULL,
        CpuHistoryGraphMessageCallback
        );

    ToolbarRegisterGraph(
        PluginInstance,
        2,
        L"Physical memory history",
        0,
        NULL,
        PhysicalHistoryGraphMessageCallback
        );

    ToolbarRegisterGraph(
        PluginInstance,
        3,
        L"Commit charge history",
        0,
        NULL,
        CommitHistoryGraphMessageCallback
        );

    ToolbarRegisterGraph(
        PluginInstance,
        4,
        L"I/O history",
        0,
        NULL,
        IoHistoryGraphMessageCallback
        );
}

VOID ToolbarGraphsInitializeDpi(
    VOID
    )
{
    for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
    {
        PPH_TOOLBAR_GRAPH graph = PhpToolbarGraphList->Items[i];

        graph->GraphDpi = SystemInformer_GetWindowDpi();
    }
}

VOID ToolbarRegisterGraph(
    _In_ PPH_PLUGIN Plugin,
    _In_ ULONG Id,
    _In_ PWSTR Text,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context,
    _In_ PTOOLSTATUS_GRAPH_CALLBACK GraphCallback
    )
{
    PPH_TOOLBAR_GRAPH graph;
    //PPH_STRING pluginName;

    graph = PhAllocateZero(sizeof(PH_TOOLBAR_GRAPH));
    graph->Plugin = Plugin;
    graph->Text = Text;
    graph->Flags = Flags;
    graph->Context = Context;
    graph->GraphCallback = GraphCallback;
    graph->GraphId = 1000 + Id;

    //pluginName = PhGetPluginName(graph->Plugin);
    //graph->GraphId = (PhHashStringRef(&pluginName->sr, TRUE) & 0xFFFF) + Id;
    //PhDereferenceObject(pluginName);

    PhAddItemList(PhpToolbarGraphList, graph);
}

BOOLEAN ToolbarAddGraph(
    _In_ PPH_TOOLBAR_GRAPH Graph,
    _In_ ULONG Width,
    _In_ ULONG Height
    )
{
    if (!Graph->GraphHandle && RebarHandle && ToolStatusConfig.ToolBarEnabled)
    {
        PH_GRAPH_CREATEPARAMS graphCreateParams;

        memset(&graphCreateParams, 0, sizeof(PH_GRAPH_CREATEPARAMS));
        graphCreateParams.Size = sizeof(PH_GRAPH_CREATEPARAMS);
        graphCreateParams.Callback = Graph->GraphCallback;
        graphCreateParams.Context = Graph;

        if (Graph->GraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0, 0, 0, 0,
            MainWindowHandle,
            NULL,
            NULL,
            &graphCreateParams
            ))
        {
            PhInitializeGraphState(&Graph->GraphState);
            Graph_SetTooltip(Graph->GraphHandle, TRUE);

            PhAddItemSimpleHashtable(PhpToolbarGraphHashtable, Graph->GraphHandle, Graph);

            if (!RebarBandExists(Graph->GraphId))
                RebarBandInsert(Graph->GraphId, Graph->GraphHandle, Width, Height); // height: 85

            if (!IsWindowVisible(Graph->GraphHandle))
                ShowWindow(Graph->GraphHandle, SW_SHOW);
        }
    }

    return TRUE;
}

BOOLEAN ToolbarRemoveGraph(
    _In_ PPH_TOOLBAR_GRAPH Graph
    )
{
    if (RebarBandExists(Graph->GraphId))
        RebarBandRemove(Graph->GraphId);

    if (Graph->GraphHandle)
    {
        PhRemoveItemSimpleHashtable(PhpToolbarGraphHashtable, Graph->GraphHandle);

        PhDeleteGraphState(&Graph->GraphState);

        DestroyWindow(Graph->GraphHandle);
        Graph->GraphHandle = NULL;
    }

    return TRUE;
}

VOID ToolbarCreateGraphs(
    VOID
    )
{
    ToolbarGraphLoadSettings();

    {
        ULONG height;
        ULONG width;

        height = (ULONG)SendMessage(RebarHandle, RB_GETROWHEIGHT, REBAR_BAND_ID_TOOLBAR, 0);
        width = PhGetDpi(145, SystemInformer_GetWindowDpi());

        for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
        {
            PPH_TOOLBAR_GRAPH graph = PhpToolbarGraphList->Items[i];

            if (!FlagOn(graph->Flags, TOOLSTATUS_GRAPH_ENABLED))
                continue;

            ToolbarAddGraph(graph, width, height);
        }
    }
}

VOID ToolbarUpdateGraphs(
    VOID
    )
{
    for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
    {
        PPH_TOOLBAR_GRAPH graph = PhpToolbarGraphList->Items[i];

        if (!FlagOn(graph->Flags, TOOLSTATUS_GRAPH_ENABLED))
            continue;

        if (!graph->GraphHandle)
            continue;

        graph->GraphState.Valid = FALSE;
        graph->GraphState.TooltipIndex = ULONG_MAX;
        Graph_Update(graph->GraphHandle);
    }
}

VOID ToolbarUpdateGraphVisualStates(
    VOID
    )
{
    if (!ToolStatusConfig.ToolBarEnabled)
        return;

    for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
    {
        PPH_TOOLBAR_GRAPH graph = PhpToolbarGraphList->Items[i];

        if (!FlagOn(graph->Flags, TOOLSTATUS_GRAPH_ENABLED))
            continue;

        if (!graph->GraphHandle)
            continue;

        graph->GraphState.Valid = FALSE;
        graph->GraphState.TooltipIndex = ULONG_MAX;
        Graph_Draw(graph->GraphHandle);
    }
}

BOOLEAN ToolbarUpdateGraphsInfo(
    _In_ HWND WindowHandle,
    _In_ LPNMHDR Header
    )
{
    PPH_TOOLBAR_GRAPH graph;

    if (graph = PhFindItemSimpleHashtable2(PhpToolbarGraphHashtable, Header->hwndFrom))
    {
        if (Header->code == GCN_MOUSEEVENT) // HACK
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Header;

            if (mouseEvent->Message == WM_RBUTTONUP)
            {
                ShowCustomizeMenu(WindowHandle);
            }
        }

        if (graph->GraphCallback)
        {
            graph->GraphCallback(graph->GraphHandle, Header->code, Header, NULL, graph);
        }

        return TRUE;
    }

    return FALSE;
}

VOID ToolbarSetVisibleGraph(
    _In_ PPH_TOOLBAR_GRAPH Graph,
    _In_ BOOLEAN Visible
    )
{
    if (Visible)
    {
        ULONG height = (ULONG)SendMessage(RebarHandle, RB_GETROWHEIGHT, REBAR_BAND_ID_TOOLBAR, 0);
        ULONG width = PhGetDpi(145, SystemInformer_GetWindowDpi());

        SetFlag(Graph->Flags, TOOLSTATUS_GRAPH_ENABLED);
        ToolbarAddGraph(Graph, width, height);
    }
    else
    {
        ClearFlag(Graph->Flags, TOOLSTATUS_GRAPH_ENABLED);
        ToolbarRemoveGraph(Graph);
    }
}

BOOLEAN ToolbarGraphsEnabled(
    VOID
    )
{
    BOOLEAN enabled = FALSE;

    for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
    {
        PPH_TOOLBAR_GRAPH graph = PhpToolbarGraphList->Items[i];

        if (FlagOn(graph->Flags, TOOLSTATUS_GRAPH_ENABLED))
        {
            enabled = TRUE;
            break;
        }
    }

    return enabled;
}

PPH_TOOLBAR_GRAPH ToolbarGraphFindById(
    _In_ ULONG GraphId
    )
{
    for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
    {
        PPH_TOOLBAR_GRAPH graph = PhpToolbarGraphList->Items[i];

        if (graph->GraphId == GraphId)
            return graph;
    }

    return NULL;
}

PPH_TOOLBAR_GRAPH ToolbarGraphFindByName(
    _In_ PCPH_STRINGREF PluginName,
    _In_ ULONG GraphId
    )
{
    for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
    {
        PPH_TOOLBAR_GRAPH graph = PhpToolbarGraphList->Items[i];

        if (graph->Plugin)
        {
            PPH_STRING pluginName;

            pluginName = PhGetPluginName(graph->Plugin);

            if (graph->GraphId == GraphId &&
                PhEqualStringRef(PluginName, &pluginName->sr, TRUE))
            {
                PhDereferenceObject(pluginName);
                return graph;
            }

            PhDereferenceObject(pluginName);
        }
    }

    return NULL;
}

VOID ToolbarGraphCreateMenu(
    _In_ PPH_EMENU ParentMenu,
    _In_ ULONG MenuId
    )
{
    for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
    {
        PPH_TOOLBAR_GRAPH graph;
        PPH_EMENU menuItem;

        graph = PhpToolbarGraphList->Items[i];
        menuItem = PhCreateEMenuItem(0, MenuId, graph->Text, NULL, graph);

        if (FlagOn(graph->Flags, TOOLSTATUS_GRAPH_ENABLED))
        {
            SetFlag(menuItem->Flags, PH_EMENU_CHECKED);
        }

        if (FlagOn(graph->Flags, TOOLSTATUS_GRAPH_UNAVAILABLE))
        {
            PPH_STRING newText;

            newText = PhaConcatStrings2(graph->Text, L" (Unavailable)");
            PhModifyEMenuItem(menuItem, PH_EMENU_MODIFY_TEXT, PH_EMENU_TEXT_OWNED,
                PhAllocateCopy(newText->Buffer, newText->Length + sizeof(UNICODE_NULL)), NULL);
        }

        PhInsertEMenuItem(ParentMenu, menuItem, ULONG_MAX);
    }
}

VOID ToolbarGraphCreatePluginMenu(
    _In_ PPH_EMENU ParentMenu,
    _In_ ULONG MenuId
    )
{
    for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
    {
        PPH_TOOLBAR_GRAPH graph;
        PPH_EMENU menuItem;

        graph = PhpToolbarGraphList->Items[i];
        menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, MenuId, graph->Text, graph);

        if (FlagOn(graph->Flags, TOOLSTATUS_GRAPH_ENABLED))
        {
            SetFlag(menuItem->Flags, PH_EMENU_CHECKED);
        }

        if (FlagOn(graph->Flags, TOOLSTATUS_GRAPH_UNAVAILABLE))
        {
            PPH_STRING newText;

            newText = PhaConcatStrings2(graph->Text, L" (Unavailable)");
            PhModifyEMenuItem(menuItem, PH_EMENU_MODIFY_TEXT, PH_EMENU_TEXT_OWNED,
                PhAllocateCopy(newText->Buffer, newText->Length + sizeof(UNICODE_NULL)), NULL);
        }

        PhInsertEMenuItem(ParentMenu, menuItem, ULONG_MAX);
    }
}

//
// BEGIN copied from ProcessHacker/sysinfo.c
//

static PPH_PROCESS_RECORD PhSipReferenceMaxCpuRecord(
    _In_ LONG Index
    )
{
    LARGE_INTEGER time;
    LONG maxProcessIdLong;
    HANDLE maxProcessId;

    if (!SystemStatistics.MaxCpuHistory)
        return NULL;

    // Find the process record for the max. CPU process for the particular time.

    maxProcessIdLong = PhGetItemCircularBuffer_ULONG(SystemStatistics.MaxCpuHistory, Index);

    if (!maxProcessIdLong)
        return NULL;

    // This must be treated as a signed integer to handle Interrupts correctly.
    maxProcessId = LongToHandle(maxProcessIdLong);

    // Note that the time we get has its components beyond seconds cleared.
    // For example:
    // * At 2.5 seconds a process is started.
    // * At 2.75 seconds our process provider is fired, and the process is determined
    //   to have 75% CPU usage, which happens to be the maximum CPU usage.
    // * However the 2.75 seconds is recorded as 2 seconds due to
    //   RtlTimeToSecondsSince1980.
    // * If we call PhFindProcessRecord, it cannot find the process because it was
    //   started at 2.5 seconds, not 2 seconds or older.
    //
    // This means we must add one second minus one tick (100ns) to the time, giving us
    // 2.9999999 seconds. This will then make sure we find the process.
    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord(maxProcessId, &time);
}

static PPH_STRING PhSipGetMaxCpuString(
    _In_ LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
    FLOAT maxCpuUsage;

    if (!SystemStatistics.MaxCpuUsageHistory)
        return PhReferenceEmptyString();

    if (maxProcessRecord = PhSipReferenceMaxCpuRecord(Index))
    {
        PPH_STRING maxUsageString;

        // We found the process record, so now we construct the max. usage string.
        maxCpuUsage = PhGetItemCircularBuffer_FLOAT(SystemStatistics.MaxCpuUsageHistory, Index);

        // Make sure we don't try to display the PID of DPCs or Interrupts.
        if (!PH_IS_FAKE_PROCESS_ID(maxProcessRecord->ProcessId))
        {
            PH_FORMAT format[7];

            // \n%s (%u): %.2f%%
            PhInitFormatC(&format[0], L'\n');
            PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
            PhInitFormatS(&format[2], L" (");
            PhInitFormatU(&format[3], HandleToUlong(maxProcessRecord->ProcessId));
            PhInitFormatS(&format[4], L"): ");
            PhInitFormatF(&format[5], maxCpuUsage * 100, 2);
            PhInitFormatC(&format[6], L'%');

            maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);
        }
        else
        {
            PH_FORMAT format[5];

            // \n%s: %.2f%%
            PhInitFormatC(&format[0], L'\n');
            PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
            PhInitFormatS(&format[2], L": ");
            PhInitFormatF(&format[3], maxCpuUsage * 100, 2);
            PhInitFormatC(&format[4], L'%');

            maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);
        }

        PhDereferenceProcessRecord(maxProcessRecord);

        return maxUsageString;
    }

    return PhReferenceEmptyString();
}

static PPH_PROCESS_RECORD PhSipReferenceMaxIoRecord(
    _In_ LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    if (!SystemStatistics.MaxIoHistory)
        return NULL;

    // Find the process record for the max. I/O process for the particular time.

    maxProcessId = PhGetItemCircularBuffer_ULONG(SystemStatistics.MaxIoHistory, Index);

    if (!maxProcessId)
        return NULL;

    // See above for the explanation.
    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord(UlongToHandle(maxProcessId), &time);
}

static PPH_STRING PhSipGetMaxIoString(
    _In_ LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
    ULONG64 maxIoReadOther;
    ULONG64 maxIoWrite;

    if (!(SystemStatistics.MaxIoReadOtherHistory && SystemStatistics.MaxIoWriteHistory))
        return PhReferenceEmptyString();

    if (maxProcessRecord = PhSipReferenceMaxIoRecord(Index))
    {
        PPH_STRING maxUsageString;

        // We found the process record, so now we construct the max. usage string.
        maxIoReadOther = PhGetItemCircularBuffer_ULONG64(SystemStatistics.MaxIoReadOtherHistory, Index);
        maxIoWrite = PhGetItemCircularBuffer_ULONG64(SystemStatistics.MaxIoWriteHistory, Index);

        if (!PH_IS_FAKE_PROCESS_ID(maxProcessRecord->ProcessId))
        {
            PH_FORMAT format[8];

            // \n%s (%u): R+O: %s, W: %s
            PhInitFormatC(&format[0], L'\n');
            PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
            PhInitFormatS(&format[2], L" (");
            PhInitFormatU(&format[3], HandleToUlong(maxProcessRecord->ProcessId));
            PhInitFormatS(&format[4], L"): R+O: ");
            PhInitFormatSize(&format[5], maxIoReadOther);
            PhInitFormatS(&format[6], L", W: ");
            PhInitFormatSize(&format[7], maxIoWrite);

            maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);
        }
        else
        {
            PH_FORMAT format[6];

            // \n%s: R+O: %s, W: %s
            PhInitFormatC(&format[0], L'\n');
            PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
            PhInitFormatS(&format[2], L": R+O: ");
            PhInitFormatSize(&format[3], maxIoReadOther);
            PhInitFormatS(&format[4], L", W: ");
            PhInitFormatSize(&format[5], maxIoWrite);

            maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);
        }

        PhDereferenceProcessRecord(maxProcessRecord);

        return maxUsageString;
    }

    return PhReferenceEmptyString();
}

//
// END copied from ProcessHacker/sysinfo.c
//

BOOLEAN CpuHistoryGraphMessageCallback(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    switch (Message)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_TOOLBAR_GRAPH graph = (PPH_TOOLBAR_GRAPH)Context;
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Parameter1;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            if (graph->GraphDpi == 0)
            {
                graph->GraphDpi = SystemInformer_GetWindowDpi();
                graph->GraphColor1 = PhGetIntegerSetting(L"ColorCpuKernel");
                graph->GraphColor2 = PhGetIntegerSetting(L"ColorCpuUser");
            }

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_LINE_2;
            PhSiSetColorsGraphDrawInfo(drawInfo, graph->GraphColor1, graph->GraphColor2, graph->GraphDpi);

            if (!(SystemStatistics.CpuKernelHistory && SystemStatistics.CpuUserHistory))
                break;

            PhGraphStateGetDrawInfo(&graph->GraphState, getDrawInfo, SystemStatistics.CpuUserHistory->Count);

            if (!graph->GraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(SystemStatistics.CpuKernelHistory, graph->GraphState.Data1, drawInfo->LineDataCount);
                PhCopyCircularBuffer_FLOAT(SystemStatistics.CpuUserHistory, graph->GraphState.Data2, drawInfo->LineDataCount);

                graph->GraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Parameter1;
            PPH_TOOLBAR_GRAPH graph = (PPH_TOOLBAR_GRAPH)Context;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (graph->GraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT cpuKernel;
                    FLOAT cpuUser;
                    PH_FORMAT format[5];

                    cpuKernel = PhGetItemCircularBuffer_FLOAT(SystemStatistics.CpuKernelHistory, getTooltipText->Index);
                    cpuUser = PhGetItemCircularBuffer_FLOAT(SystemStatistics.CpuUserHistory, getTooltipText->Index);

                    // %.2f%%%s\n%s
                    PhInitFormatF(&format[0], (cpuKernel + cpuUser) * 100, 2);
                    PhInitFormatC(&format[1], L'%');
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhSipGetMaxCpuString(getTooltipText->Index))->sr);
                    PhInitFormatC(&format[3], L'\n');
                    PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&graph->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                }

                getTooltipText->Text = PhGetStringRef(graph->GraphState.TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_TOOLBAR_GRAPH graph = (PPH_TOOLBAR_GRAPH)Context;
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Parameter1;
            PPH_PROCESS_RECORD record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK)
            {
                if (PhGetIntegerSetting(SETTING_NAME_SHOWSYSINFOGRAPH))
                {
                    PhShowSystemInformationDialog(L"CPU");
                }
                else
                {
                    if (mouseEvent->Index < mouseEvent->TotalCount)
                    {
                        record = PhSipReferenceMaxCpuRecord(mouseEvent->Index);
                    }

                    if (record)
                    {
                        PhShowProcessRecordDialog(MainWindowHandle, record);
                        PhDereferenceProcessRecord(record);
                    }
                }
            }
        }
        break;
    }

    return TRUE;
}

BOOLEAN PhysicalHistoryGraphMessageCallback(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    switch (Message)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_TOOLBAR_GRAPH graph = (PPH_TOOLBAR_GRAPH)Context;
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Parameter1;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            if (graph->GraphDpi == 0)
            {
                graph->GraphDpi = SystemInformer_GetWindowDpi();
                graph->GraphColor1 = PhGetIntegerSetting(L"ColorPhysical");
            }

            drawInfo->Flags = PH_GRAPH_USE_GRID_X;
            PhSiSetColorsGraphDrawInfo(drawInfo, graph->GraphColor1, 0, graph->GraphDpi);

            if (!SystemStatistics.PhysicalHistory)
                break;

            PhGraphStateGetDrawInfo(&graph->GraphState, getDrawInfo, SystemStatistics.PhysicalHistory->Count);

            if (!graph->GraphState.Valid)
            {
                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    graph->GraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(SystemStatistics.PhysicalHistory, i);
                }

                PhDivideSinglesBySingle(
                    graph->GraphState.Data1,
                    (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages,
                    drawInfo->LineDataCount
                    );

                graph->GraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Parameter1;
            PPH_TOOLBAR_GRAPH graph = (PPH_TOOLBAR_GRAPH)Context;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (graph->GraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG physicalUsage;
                    PH_FORMAT format[4];

                    if (!SystemStatistics.PhysicalHistory)
                        break;

                    physicalUsage = PhGetItemCircularBuffer_ULONG(SystemStatistics.PhysicalHistory, getTooltipText->Index);

                    // Physical memory: %s\n%s
                    PhInitFormatS(&format[0], L"Physical memory: ");
                    PhInitFormatSize(&format[1], UInt32x32To64(physicalUsage, PAGE_SIZE));
                    PhInitFormatC(&format[2], L'\n');
                    PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&graph->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                }

                getTooltipText->Text = PhGetStringRef(graph->GraphState.TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Parameter1;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK)
            {
                if (PhGetIntegerSetting(SETTING_NAME_SHOWSYSINFOGRAPH))
                {
                    PhShowSystemInformationDialog(L"Memory");
                }
            }
        }
        break;
    }

    return TRUE;
}

BOOLEAN CommitHistoryGraphMessageCallback(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    switch (Message)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_TOOLBAR_GRAPH graph = (PPH_TOOLBAR_GRAPH)Context;
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Parameter1;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            if (graph->GraphDpi == 0)
            {
                graph->GraphDpi = SystemInformer_GetWindowDpi();
                graph->GraphColor1 = PhGetIntegerSetting(L"ColorPrivate");
            }

            drawInfo->Flags = PH_GRAPH_USE_GRID_X;
            PhSiSetColorsGraphDrawInfo(drawInfo, graph->GraphColor1, 0, graph->GraphDpi);

            if (!(SystemStatistics.CommitHistory && SystemStatistics.Performance))
                break;

            PhGraphStateGetDrawInfo(&graph->GraphState, getDrawInfo, SystemStatistics.CommitHistory->Count);

            if (!graph->GraphState.Valid)
            {
                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    graph->GraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(SystemStatistics.CommitHistory, i);
                }

                PhDivideSinglesBySingle(
                    graph->GraphState.Data1,
                    (FLOAT)SystemStatistics.Performance->CommitLimit,
                    drawInfo->LineDataCount
                    );

                graph->GraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Parameter1;
            PPH_TOOLBAR_GRAPH graph = (PPH_TOOLBAR_GRAPH)Context;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (graph->GraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG commitUsage;
                    PH_FORMAT format[4];

                    if (!SystemStatistics.CommitHistory)
                        break;

                    commitUsage = PhGetItemCircularBuffer_ULONG(SystemStatistics.CommitHistory, getTooltipText->Index);

                    // Commit charge: %s\n%s
                    PhInitFormatS(&format[0], L"Commit charge: ");
                    PhInitFormatSize(&format[1], UInt32x32To64(commitUsage, PAGE_SIZE));
                    PhInitFormatC(&format[2], L'\n');
                    PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&graph->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                }

                getTooltipText->Text = PhGetStringRef(graph->GraphState.TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Parameter1;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK)
            {
                if (PhGetIntegerSetting(SETTING_NAME_SHOWSYSINFOGRAPH))
                {
                    PhShowSystemInformationDialog(L"Memory");
                }
            }
        }
        break;
    }

    return TRUE;
}

BOOLEAN IoHistoryGraphMessageCallback(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    switch (Message)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_TOOLBAR_GRAPH graph = (PPH_TOOLBAR_GRAPH)Context;
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Parameter1;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            if (graph->GraphDpi == 0)
            {
                graph->GraphDpi = SystemInformer_GetWindowDpi();
                graph->GraphColor1 = PhGetIntegerSetting(L"ColorIoReadOther");
                graph->GraphColor2 = PhGetIntegerSetting(L"ColorIoWrite");
            }

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_LINE_2;
            PhSiSetColorsGraphDrawInfo(drawInfo, graph->GraphColor1, graph->GraphColor2, graph->GraphDpi);

            if (!(SystemStatistics.IoReadHistory && SystemStatistics.IoOtherHistory && SystemStatistics.IoWriteHistory))
                break;

            PhGraphStateGetDrawInfo(&graph->GraphState, getDrawInfo, SystemStatistics.IoReadHistory->Count);

            if (!graph->GraphState.Valid)
            {
                FLOAT max = 1024 * 1024; // minimum scaling of 1 MB.

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    graph->GraphState.Data1[i] =
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoReadHistory, i) +
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoOtherHistory, i);
                    graph->GraphState.Data2[i] =
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoWriteHistory, i);

                    if (max < graph->GraphState.Data1[i] + graph->GraphState.Data2[i])
                        max = graph->GraphState.Data1[i] + graph->GraphState.Data2[i];
                }

                PhDivideSinglesBySingle(graph->GraphState.Data1, max, drawInfo->LineDataCount);
                PhDivideSinglesBySingle(graph->GraphState.Data2, max, drawInfo->LineDataCount);

                graph->GraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Parameter1;
            PPH_TOOLBAR_GRAPH graph = (PPH_TOOLBAR_GRAPH)Context;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (graph->GraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 ioRead;
                    ULONG64 ioWrite;
                    ULONG64 ioOther;
                    PH_FORMAT format[9];

                    if (!(SystemStatistics.IoReadHistory && SystemStatistics.IoOtherHistory && SystemStatistics.IoWriteHistory))
                        break;

                    ioRead = PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoReadHistory, getTooltipText->Index);
                    ioWrite = PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoWriteHistory, getTooltipText->Index);
                    ioOther = PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoOtherHistory, getTooltipText->Index);

                    // R: %s\nW: %s\nO: %s%s\n%s
                    PhInitFormatS(&format[0], L"R: ");
                    PhInitFormatSize(&format[1], ioRead);
                    PhInitFormatS(&format[2], L"\nW: ");
                    PhInitFormatSize(&format[3], ioWrite);
                    PhInitFormatS(&format[4], L"\nO: ");
                    PhInitFormatSize(&format[5], ioOther);
                    PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhSipGetMaxIoString(getTooltipText->Index))->sr);
                    PhInitFormatC(&format[7], L'\n');
                    PhInitFormatSR(&format[8], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&graph->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                }

                getTooltipText->Text = PhGetStringRef(graph->GraphState.TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Parameter1;
            PPH_PROCESS_RECORD record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK)
            {
                if (PhGetIntegerSetting(SETTING_NAME_SHOWSYSINFOGRAPH))
                {
                    PhShowSystemInformationDialog(L"I/O");
                }
                else
                {
                    if (mouseEvent->Index < mouseEvent->TotalCount)
                    {
                        record = PhSipReferenceMaxIoRecord(mouseEvent->Index);
                    }

                    if (record)
                    {
                        PhShowProcessRecordDialog(MainWindowHandle, record);
                        PhDereferenceProcessRecord(record);
                    }
                }
            }
        }
        break;
    }

    return TRUE;
}
