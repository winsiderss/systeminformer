/*
 * Process Hacker ToolStatus -
 *   Toolbar Graph Bands
 *
 * Copyright (C) 2015-2020 dmex
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

#include "toolstatus.h"

PPH_LIST PhpToolbarGraphList = NULL;
PPH_HASHTABLE PhpToolbarGraphHashtable = NULL;

TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(CpuHistoryGraphMessageCallback);
TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(PhysicalHistoryGraphMessageCallback);
TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(CommitHistoryGraphMessageCallback);
TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(IoHistoryGraphMessageCallback);

VOID ToolbarGraphLoadSettings(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRINGREF remaining;

    settingsString = PhGetStringSetting(SETTING_NAME_TOOLBAR_GRAPH_CONFIG);
    remaining = settingsString->sr;

    if (remaining.Length == 0)
        return;

    while (remaining.Length != 0)
    {
        PH_STRINGREF idPart;
        PH_STRINGREF flagsPart;
        PH_STRINGREF pluginNamePart;
        ULONG64 idInteger;
        ULONG64 flagsInteger;

        PhSplitStringRefAtChar(&remaining, '|', &idPart, &remaining);
        PhSplitStringRefAtChar(&remaining, '|', &flagsPart, &remaining);
        PhSplitStringRefAtChar(&remaining, '|', &pluginNamePart, &remaining);

        if (!PhStringToInteger64(&idPart, 10, &idInteger))
            break;
        if (!PhStringToInteger64(&flagsPart, 10, &flagsInteger))
            break;

        if (flagsInteger)
        {
            PPH_TOOLBAR_GRAPH graph;

            if (pluginNamePart.Length)
            {
                if (graph = ToolbarGraphFindByName(&pluginNamePart, (ULONG)idInteger))
                    graph->Flags |= TOOLSTATUS_GRAPH_ENABLED;
            }
            else
            {
                if (graph = ToolbarGraphFindById((ULONG)idInteger))
                    graph->Flags |= TOOLSTATUS_GRAPH_ENABLED;
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

        if (!(graph->Flags & TOOLSTATUS_GRAPH_ENABLED))
            continue;

        pluginName = graph->Plugin ? PhGetPluginName(graph->Plugin) : NULL;

        PhAppendFormatStringBuilder(
            &graphListBuilder,
            L"%lu|%lu|%s|",
            graph->GraphId,
            graph->Flags & TOOLSTATUS_GRAPH_ENABLED ? 1 : 0,
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

VOID ToolbarRegisterGraph(
    _In_ struct _PH_PLUGIN *Plugin,
    _In_ ULONG Id,
    _In_ PWSTR Text,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context,
    _In_opt_ PTOOLSTATUS_GRAPH_MESSAGE_CALLBACK MessageCallback
    )
{
    PPH_TOOLBAR_GRAPH graph;
    //PPH_STRING pluginName;

    graph = PhAllocateZero(sizeof(PH_TOOLBAR_GRAPH));
    graph->Plugin = Plugin;
    graph->Text = Text;
    graph->Flags = Flags;
    graph->Context = Context;
    graph->MessageCallback = MessageCallback;
    graph->GraphId = 1000 + Id;

    //pluginName = PhGetPluginName(graph->Plugin);
    //graph->GraphId = (PhHashStringRef(&pluginName->sr, TRUE) & 0xFFFF) + Id;
    //PhDereferenceObject(pluginName);

    PhAddItemList(PhpToolbarGraphList, graph);
}

BOOLEAN ToolbarAddGraph(
    _In_ PPH_TOOLBAR_GRAPH Graph
    )
{
    if (!Graph->GraphHandle && RebarHandle && ToolStatusConfig.ToolBarEnabled)
    {
        UINT rebarHeight = (UINT)SendMessage(RebarHandle, RB_GETROWHEIGHT, 0, 0);

        if (Graph->GraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            0,
            0,
            RebarHandle,
            NULL,
            NULL,
            NULL
            ))
        {
            Graph_SetTooltip(Graph->GraphHandle, TRUE);
            PhInitializeGraphState(&Graph->GraphState);

            PhAddItemSimpleHashtable(PhpToolbarGraphHashtable, Graph->GraphHandle, Graph);

            if (!RebarBandExists(Graph->GraphId))
                RebarBandInsert(Graph->GraphId, Graph->GraphHandle, 145, rebarHeight); // height: 85

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

    for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
    {
        PPH_TOOLBAR_GRAPH graph = PhpToolbarGraphList->Items[i];

        if (!(graph->Flags & TOOLSTATUS_GRAPH_ENABLED))
            continue;

        ToolbarAddGraph(graph);
    }
}

VOID ToolbarUpdateGraphs(
    VOID
    )
{
    if (!ToolStatusConfig.ToolBarEnabled)
        return;

    for (ULONG i = 0; i < PhpToolbarGraphList->Count; i++)
    {
        PPH_TOOLBAR_GRAPH graph = PhpToolbarGraphList->Items[i];

        if (!(graph->Flags & TOOLSTATUS_GRAPH_ENABLED))
            continue;

        if (!graph->GraphHandle)
            continue;

        graph->GraphState.Valid = FALSE;
        graph->GraphState.TooltipIndex = ULONG_MAX;
        Graph_MoveGrid(graph->GraphHandle, 1);
        Graph_Draw(graph->GraphHandle);
        //Graph_UpdateTooltip(graph->GraphHandle);
        InvalidateRect(graph->GraphHandle, NULL, FALSE);
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

        graph->MessageCallback(graph, graph->GraphHandle, &graph->GraphState, Header, NULL);
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
        Graph->Flags |= TOOLSTATUS_GRAPH_ENABLED;
        ToolbarAddGraph(Graph);
    }
    else
    {
        Graph->Flags &= ~TOOLSTATUS_GRAPH_ENABLED;
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

        if (graph->Flags & TOOLSTATUS_GRAPH_ENABLED)
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
    _In_ PPH_STRINGREF PluginName,
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

        if (graph->Flags & TOOLSTATUS_GRAPH_ENABLED)
        {
            menuItem->Flags |= PH_EMENU_CHECKED;
        }

        if (graph->Flags & TOOLSTATUS_GRAPH_UNAVAILABLE)
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

        if (graph->Flags & TOOLSTATUS_GRAPH_ENABLED)
        {
            menuItem->Flags |= PH_EMENU_CHECKED;
        }

        if (graph->Flags & TOOLSTATUS_GRAPH_UNAVAILABLE)
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
    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = PhSipReferenceMaxCpuRecord(Index))
    {
        // We found the process record, so now we construct the max. usage string.
        maxCpuUsage = PhGetItemCircularBuffer_FLOAT(SystemStatistics.MaxCpuUsageHistory, Index);

        // Make sure we don't try to display the PID of DPCs or Interrupts.
        if (!PH_IS_FAKE_PROCESS_ID(maxProcessRecord->ProcessId))
        {
            maxUsageString = PhaFormatString(
                L"\n%s (%u): %.2f%%",
                maxProcessRecord->ProcessName->Buffer,
                HandleToUlong(maxProcessRecord->ProcessId),
                maxCpuUsage * 100
                );
        }
        else
        {
            maxUsageString = PhaFormatString(
                L"\n%s: %.2f%%",
                maxProcessRecord->ProcessName->Buffer,
                maxCpuUsage * 100
                );
        }

        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}

static PPH_PROCESS_RECORD PhSipReferenceMaxIoRecord(
    _In_ LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

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

    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = PhSipReferenceMaxIoRecord(Index))
    {
        // We found the process record, so now we construct the max. usage string.
        maxIoReadOther = PhGetItemCircularBuffer_ULONG64(SystemStatistics.MaxIoReadOtherHistory, Index);
        maxIoWrite = PhGetItemCircularBuffer_ULONG64(SystemStatistics.MaxIoWriteHistory, Index);

        if (!PH_IS_FAKE_PROCESS_ID(maxProcessRecord->ProcessId))
        {
            maxUsageString = PhaFormatString(
                L"\n%s (%u): R+O: %s, W: %s",
                maxProcessRecord->ProcessName->Buffer,
                HandleToUlong(maxProcessRecord->ProcessId),
                PhaFormatSize(maxIoReadOther, ULONG_MAX)->Buffer,
                PhaFormatSize(maxIoWrite, ULONG_MAX)->Buffer
                );
        }
        else
        {
            maxUsageString = PhaFormatString(
                L"\n%s: R+O: %s, W: %s",
                maxProcessRecord->ProcessName->Buffer,
                PhaFormatSize(maxIoReadOther, ULONG_MAX)->Buffer,
                PhaFormatSize(maxIoWrite, ULONG_MAX)->Buffer
                );
        }

        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}

//
// END copied from ProcessHacker/sysinfo.c
//

TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(CpuHistoryGraphMessageCallback)
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_LINE_2;
            PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), PhGetIntegerSetting(L"ColorCpuUser"));

            if (ProcessesUpdatedCount != 3)
                return;

            PhGraphStateGetDrawInfo(GraphState, getDrawInfo, SystemStatistics.CpuUserHistory->Count);

            if (!GraphState->Valid)
            {
                PhCopyCircularBuffer_FLOAT(SystemStatistics.CpuKernelHistory, GraphState->Data1, drawInfo->LineDataCount);
                PhCopyCircularBuffer_FLOAT(SystemStatistics.CpuUserHistory, GraphState->Data2, drawInfo->LineDataCount);

                GraphState->Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (GraphState->TooltipIndex != getTooltipText->Index)
                {
                    FLOAT cpuKernel;
                    FLOAT cpuUser;

                    cpuKernel = PhGetItemCircularBuffer_FLOAT(SystemStatistics.CpuKernelHistory, getTooltipText->Index);
                    cpuUser = PhGetItemCircularBuffer_FLOAT(SystemStatistics.CpuUserHistory, getTooltipText->Index);

                    PhMoveReference(&GraphState->TooltipText, PhFormatString(
                        L"%.2f%%%s\n%s",
                        (cpuKernel + cpuUser) * 100,
                        PhGetStringOrEmpty(PhSipGetMaxCpuString(getTooltipText->Index)),
                        PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                        ));
                }

                getTooltipText->Text = PhGetStringRef(GraphState->TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Header;
            PPH_PROCESS_RECORD record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK && mouseEvent->Index < mouseEvent->TotalCount)
            {
                record = PhSipReferenceMaxCpuRecord(mouseEvent->Index);
            }

            if (record)
            {
                PhShowProcessRecordDialog(PhMainWndHandle, record);
                PhDereferenceProcessRecord(record);
            }
        }
        break;
    }
}

TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(PhysicalHistoryGraphMessageCallback)
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X;
            PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorPhysical"), 0);

            if (ProcessesUpdatedCount != 3)
                break;

            PhGraphStateGetDrawInfo(GraphState, getDrawInfo, SystemStatistics.PhysicalHistory->Count);

            if (!GraphState->Valid)
            {
                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    GraphState->Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(SystemStatistics.PhysicalHistory, i);
                }

                PhDivideSinglesBySingle(
                    GraphState->Data1,
                    (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages,
                    drawInfo->LineDataCount
                    );

                GraphState->Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (GraphState->TooltipIndex != getTooltipText->Index)
                {
                    ULONG physicalUsage;

                    if (ProcessesUpdatedCount != 3)
                        break;

                    physicalUsage = PhGetItemCircularBuffer_ULONG(SystemStatistics.PhysicalHistory, getTooltipText->Index);

                    PhMoveReference(&GraphState->TooltipText, PhFormatString(
                        L"Physical memory: %s\n%s",
                        PhaFormatSize(UInt32x32To64(physicalUsage, PAGE_SIZE), ULONG_MAX)->Buffer,
                        PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                        ));
                }

                getTooltipText->Text = PhGetStringRef(GraphState->TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            NOTHING;
        }
        break;
    }
}

TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(CommitHistoryGraphMessageCallback)
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X;
            PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorPrivate"), 0);

            if (ProcessesUpdatedCount != 3)
                break;

            PhGraphStateGetDrawInfo(GraphState, getDrawInfo, SystemStatistics.CommitHistory->Count);

            if (!GraphState->Valid)
            {
                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    GraphState->Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(SystemStatistics.CommitHistory, i);
                }

                PhDivideSinglesBySingle(
                    GraphState->Data1,
                    (FLOAT)SystemStatistics.Performance->CommitLimit,
                    drawInfo->LineDataCount
                    );

                GraphState->Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (GraphState->TooltipIndex != getTooltipText->Index)
                {
                    ULONG commitUsage;

                    if (ProcessesUpdatedCount != 3)
                        break;

                    commitUsage = PhGetItemCircularBuffer_ULONG(SystemStatistics.CommitHistory, getTooltipText->Index);

                    PhMoveReference(&GraphState->TooltipText, PhFormatString(
                        L"Commit charge: %s\n%s",
                        PhaFormatSize(UInt32x32To64(commitUsage, PAGE_SIZE), ULONG_MAX)->Buffer,
                        PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                        ));
                }

                getTooltipText->Text = PhGetStringRef(GraphState->TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            NOTHING;
        }
        break;
    }
}

TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(IoHistoryGraphMessageCallback)
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_LINE_2;
            PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));

            if (ProcessesUpdatedCount != 3)
                break;

            PhGraphStateGetDrawInfo(GraphState, getDrawInfo, SystemStatistics.IoReadHistory->Count);

            if (!GraphState->Valid)
            {
                FLOAT max = 1024 * 1024; // minimum scaling of 1 MB.

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    GraphState->Data1[i] =
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoReadHistory, i) +
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoOtherHistory, i);
                    GraphState->Data2[i] =
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoWriteHistory, i);

                    if (max < GraphState->Data1[i] + GraphState->Data2[i])
                        max = GraphState->Data1[i] + GraphState->Data2[i];
                }

                PhDivideSinglesBySingle(GraphState->Data1, max, drawInfo->LineDataCount);
                PhDivideSinglesBySingle(GraphState->Data2, max, drawInfo->LineDataCount);

                GraphState->Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (GraphState->TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 ioRead;
                    ULONG64 ioWrite;
                    ULONG64 ioOther;

                    if (ProcessesUpdatedCount != 3)
                        break;

                    ioRead = PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoReadHistory, getTooltipText->Index);
                    ioWrite = PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoWriteHistory, getTooltipText->Index);
                    ioOther = PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoOtherHistory, getTooltipText->Index);

                    PhMoveReference(&GraphState->TooltipText, PhFormatString(
                        L"R: %s\nW: %s\nO: %s%s\n%s",
                        PhaFormatSize(ioRead, ULONG_MAX)->Buffer,
                        PhaFormatSize(ioWrite, ULONG_MAX)->Buffer,
                        PhaFormatSize(ioOther, ULONG_MAX)->Buffer,
                        PhGetStringOrEmpty(PhSipGetMaxIoString(getTooltipText->Index)),
                        PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                        ));
                }

                getTooltipText->Text = PhGetStringRef(GraphState->TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Header;
            PPH_PROCESS_RECORD record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK && mouseEvent->Index < mouseEvent->TotalCount)
            {
                record = PhSipReferenceMaxIoRecord(mouseEvent->Index);
            }

            if (record)
            {
                PhShowProcessRecordDialog(PhMainWndHandle, record);
                PhDereferenceProcessRecord(record);
            }
        }
        break;
    }
}
