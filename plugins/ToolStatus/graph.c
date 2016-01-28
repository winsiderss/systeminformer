/*
 * Process Hacker ToolStatus -
 *   Toolbar Graph Bands
 *
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

#include "toolstatus.h"

HWND CpuGraphHandle = NULL;
HWND MemGraphHandle = NULL;
HWND CommitGraphHandle = NULL;
HWND IoGraphHandle = NULL;
static PH_GRAPH_STATE CpuGraphState;
static PH_GRAPH_STATE MemGraphState;
static PH_GRAPH_STATE CommitGraphState;
static PH_GRAPH_STATE IoGraphState;

VOID ToolbarCreateGraphs(VOID)
{
    UINT height = (UINT)SendMessage(RebarHandle, RB_GETROWHEIGHT, 0, 0);

    if (ToolStatusConfig.CpuGraphEnabled && !CpuGraphHandle)
    {
        CpuGraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            0,
            0,
            PhMainWndHandle,
            NULL,
            NULL,
            NULL
            );
        Graph_SetTooltip(CpuGraphHandle, TRUE);

        PhInitializeGraphState(&CpuGraphState);
    }

    if (ToolStatusConfig.MemGraphEnabled && !MemGraphHandle)
    {
        MemGraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            0,
            0,
            PhMainWndHandle,
            NULL,
            NULL,
            NULL
            );
        Graph_SetTooltip(MemGraphHandle, TRUE);

        PhInitializeGraphState(&MemGraphState);
    }

    if (ToolStatusConfig.CommitGraphEnabled && !CommitGraphHandle)
    {
        CommitGraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            0,
            0,
            PhMainWndHandle,
            NULL,
            NULL,
            NULL
            );
        Graph_SetTooltip(CommitGraphHandle, TRUE);

        PhInitializeGraphState(&CommitGraphState);
    }

    if (ToolStatusConfig.IoGraphEnabled && !IoGraphHandle)
    {
        IoGraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            0,
            0,
            PhMainWndHandle,
            NULL,
            NULL,
            NULL
            );
        Graph_SetTooltip(IoGraphHandle, TRUE);

        PhInitializeGraphState(&IoGraphState);
    }

    if (ToolStatusConfig.CpuGraphEnabled)
    {
        if (!RebarBandExists(REBAR_BAND_ID_CPUGRAPH))
            RebarBandInsert(REBAR_BAND_ID_CPUGRAPH, CpuGraphHandle, 145, height); // 85

        if (CpuGraphHandle && !IsWindowVisible(CpuGraphHandle))
            ShowWindow(CpuGraphHandle, SW_SHOW);
    }
    else
    {
        if (RebarBandExists(REBAR_BAND_ID_CPUGRAPH))
            RebarBandRemove(REBAR_BAND_ID_CPUGRAPH);

        if (CpuGraphHandle)
        {
            PhDeleteGraphState(&CpuGraphState);

            DestroyWindow(CpuGraphHandle);
            CpuGraphHandle = NULL;
        }
    }

    if (ToolStatusConfig.MemGraphEnabled)
    {
        if (!RebarBandExists(REBAR_BAND_ID_MEMGRAPH))
            RebarBandInsert(REBAR_BAND_ID_MEMGRAPH, MemGraphHandle, 145, height); // 85

        if (MemGraphHandle && !IsWindowVisible(MemGraphHandle))
        {
            ShowWindow(MemGraphHandle, SW_SHOW);
        }
    }
    else
    {
        if (RebarBandExists(REBAR_BAND_ID_MEMGRAPH))
            RebarBandRemove(REBAR_BAND_ID_MEMGRAPH);

        if (MemGraphHandle)
        {
            PhDeleteGraphState(&MemGraphState);

            DestroyWindow(MemGraphHandle);
            MemGraphHandle = NULL;
        }
    }

    if (ToolStatusConfig.CommitGraphEnabled)
    {
        if (!RebarBandExists(REBAR_BAND_ID_COMMITGRAPH))
            RebarBandInsert(REBAR_BAND_ID_COMMITGRAPH, CommitGraphHandle, 145, height); // 85

        if (CommitGraphHandle && !IsWindowVisible(CommitGraphHandle))
        {
            ShowWindow(CommitGraphHandle, SW_SHOW);
        }
    }
    else
    {
        if (RebarBandExists(REBAR_BAND_ID_COMMITGRAPH))
            RebarBandRemove(REBAR_BAND_ID_COMMITGRAPH);

        if (CommitGraphHandle)
        {
            PhDeleteGraphState(&CommitGraphState);

            DestroyWindow(CommitGraphHandle);
            CommitGraphHandle = NULL;
        }
    }

    if (ToolStatusConfig.IoGraphEnabled)
    {
        if (!RebarBandExists(REBAR_BAND_ID_IOGRAPH))
            RebarBandInsert(REBAR_BAND_ID_IOGRAPH, IoGraphHandle, 145, height); // 85

        if (IoGraphHandle && !IsWindowVisible(IoGraphHandle))
        {
            ShowWindow(IoGraphHandle, SW_SHOW);
        }
    }
    else
    {
        if (RebarBandExists(REBAR_BAND_ID_IOGRAPH))
            RebarBandRemove(REBAR_BAND_ID_IOGRAPH);

        if (IoGraphHandle)
        {
            PhDeleteGraphState(&IoGraphState);

            DestroyWindow(IoGraphHandle);
            IoGraphHandle = NULL;
        }
    }
}

VOID ToolbarUpdateGraphs(VOID)
{
    if (ToolStatusConfig.CpuGraphEnabled && CpuGraphHandle)
    {
        CpuGraphState.Valid = FALSE;
        CpuGraphState.TooltipIndex = -1;
        Graph_MoveGrid(CpuGraphHandle, 1);
        Graph_Draw(CpuGraphHandle);
        Graph_UpdateTooltip(CpuGraphHandle);
        InvalidateRect(CpuGraphHandle, NULL, FALSE);
    }

    if (ToolStatusConfig.MemGraphEnabled && MemGraphHandle)
    {
        MemGraphState.Valid = FALSE;
        MemGraphState.TooltipIndex = -1;
        Graph_MoveGrid(MemGraphHandle, 1);
        Graph_Draw(MemGraphHandle);
        Graph_UpdateTooltip(MemGraphHandle);
        InvalidateRect(MemGraphHandle, NULL, FALSE);
    }

    if (ToolStatusConfig.CommitGraphEnabled && CommitGraphHandle)
    {
        CommitGraphState.Valid = FALSE;
        CommitGraphState.TooltipIndex = -1;
        Graph_MoveGrid(CommitGraphHandle, 1);
        Graph_Draw(CommitGraphHandle);
        Graph_UpdateTooltip(CommitGraphHandle);
        InvalidateRect(CommitGraphHandle, NULL, FALSE);
    }

    if (ToolStatusConfig.IoGraphEnabled && IoGraphHandle)
    {
        IoGraphState.Valid = FALSE;
        IoGraphState.TooltipIndex = -1;
        Graph_MoveGrid(IoGraphHandle, 1);
        Graph_Draw(IoGraphHandle);
        Graph_UpdateTooltip(IoGraphHandle);
        InvalidateRect(IoGraphHandle, NULL, FALSE);
    }
}

static PPH_PROCESS_RECORD PhSipReferenceMaxCpuRecord(
    _In_ LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    // Find the process record for the max. CPU process for the particular time.
    maxProcessId = PhGetItemCircularBuffer_ULONG(SystemStatistics.MaxCpuHistory, Index);

    if (!maxProcessId)
        return NULL;

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
    // This mean we must add one second minus one tick (100ns) to the time, giving us
    // 2.9999999 seconds. This will then make sure we find the process.
    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord(UlongToHandle(maxProcessId), &time);
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
                PhaFormatSize(maxIoReadOther, -1)->Buffer,
                PhaFormatSize(maxIoWrite, -1)->Buffer
                );
        }
        else
        {
            maxUsageString = PhaFormatString(
                L"\n%s: R+O: %s, W: %s",
                maxProcessRecord->ProcessName->Buffer,
                PhaFormatSize(maxIoReadOther, -1)->Buffer,
                PhaFormatSize(maxIoWrite, -1)->Buffer
                );
        }

        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}

VOID ToolbarUpdateGraphsInfo(LPNMHDR Header)
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            if (ToolStatusConfig.CpuGraphEnabled && Header->hwndFrom == CpuGraphHandle)
            {
                PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
                PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), PhGetIntegerSetting(L"ColorCpuUser"));

                if (ProcessesUpdatedCount < 2)
                    return;

                PhGraphStateGetDrawInfo(&CpuGraphState, getDrawInfo, SystemStatistics.CpuUserHistory->Count);

                if (!CpuGraphState.Valid)
                {
                    PhCopyCircularBuffer_FLOAT(SystemStatistics.CpuKernelHistory,
                        CpuGraphState.Data1, drawInfo->LineDataCount);
                    PhCopyCircularBuffer_FLOAT(SystemStatistics.CpuUserHistory,
                        CpuGraphState.Data2, drawInfo->LineDataCount);

                    CpuGraphState.Valid = TRUE;
                }
            }
            else if (ToolStatusConfig.MemGraphEnabled && Header->hwndFrom == MemGraphHandle)
            {
                PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
                PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                drawInfo->Flags = PH_GRAPH_USE_GRID;
                PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorPhysical"), 0);

                if (ProcessesUpdatedCount < 2)
                    return;

                PhGraphStateGetDrawInfo(&MemGraphState, getDrawInfo, SystemStatistics.PhysicalHistory->Count);

                if (!MemGraphState.Valid)
                {
                    for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        MemGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(SystemStatistics.PhysicalHistory, i);
                    }

                    PhDivideSinglesBySingle(
                        MemGraphState.Data1,
                        (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages,
                        drawInfo->LineDataCount
                        );

                    MemGraphState.Valid = TRUE;
                }
            }
            else if (ToolStatusConfig.CommitGraphEnabled && Header->hwndFrom == CommitGraphHandle)
            {
                PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
                PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                drawInfo->Flags = PH_GRAPH_USE_GRID;
                PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorPrivate"), 0);

                if (ProcessesUpdatedCount < 2)
                    return;

                PhGraphStateGetDrawInfo(&CommitGraphState, getDrawInfo, SystemStatistics.CommitHistory->Count);

                if (!CommitGraphState.Valid)
                {
                    for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        CommitGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(SystemStatistics.CommitHistory, i);
                    }

                    PhDivideSinglesBySingle(
                        CommitGraphState.Data1,
                        (FLOAT)SystemStatistics.Performance->CommitLimit,
                        drawInfo->LineDataCount
                        );

                    CommitGraphState.Valid = TRUE;
                }
            }
            else if (ToolStatusConfig.IoGraphEnabled && Header->hwndFrom == IoGraphHandle)
            {
                PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
                PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));

                if (ProcessesUpdatedCount < 2)
                    return;

                PhGraphStateGetDrawInfo(&IoGraphState, getDrawInfo, SystemStatistics.IoReadHistory->Count);

                if (!IoGraphState.Valid)
                {
                    FLOAT max = 1024 * 1024; // minimum scaling of 1 MB.

                    for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        IoGraphState.Data1[i] =
                            (FLOAT)PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoReadHistory, i) +
                            (FLOAT)PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoOtherHistory, i);
                        IoGraphState.Data2[i] =
                            (FLOAT)PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoWriteHistory, i);

                        if (max < IoGraphState.Data1[i] + IoGraphState.Data2[i])
                            max = IoGraphState.Data1[i] + IoGraphState.Data2[i];
                    }

                    PhDivideSinglesBySingle(IoGraphState.Data1, max, drawInfo->LineDataCount);
                    PhDivideSinglesBySingle(IoGraphState.Data2, max, drawInfo->LineDataCount);

                    IoGraphState.Valid = TRUE;
                }
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (ToolStatusConfig.CpuGraphEnabled && Header->hwndFrom == CpuGraphHandle)
                {
                    if (CpuGraphState.TooltipIndex != getTooltipText->Index)
                    {
                        FLOAT cpuKernel;
                        FLOAT cpuUser;

                        cpuKernel = PhGetItemCircularBuffer_FLOAT(SystemStatistics.CpuKernelHistory, getTooltipText->Index);
                        cpuUser = PhGetItemCircularBuffer_FLOAT(SystemStatistics.CpuUserHistory, getTooltipText->Index);

                        PhMoveReference(&CpuGraphState.TooltipText, PhFormatString(
                            L"%.2f%%%s\n%s",
                            (cpuKernel + cpuUser) * 100,
                            PhGetStringOrEmpty(PhSipGetMaxCpuString(getTooltipText->Index)),
                            ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                            ));
                    }

                    getTooltipText->Text = CpuGraphState.TooltipText->sr;
                }
                else if (ToolStatusConfig.MemGraphEnabled && Header->hwndFrom == MemGraphHandle)
                {
                    ULONG physicalUsage;

                    physicalUsage = PhGetItemCircularBuffer_ULONG(SystemStatistics.PhysicalHistory, getTooltipText->Index);

                    PhMoveReference(&MemGraphState.TooltipText, PhFormatString(
                        L"Physical Memory: %s\n%s",
                        PhaFormatSize(UInt32x32To64(physicalUsage, PAGE_SIZE), -1)->Buffer,
                        ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                    getTooltipText->Text = MemGraphState.TooltipText->sr;
                }
                else if (ToolStatusConfig.CommitGraphEnabled && Header->hwndFrom == CommitGraphHandle)
                {
                    ULONG commitUsage;

                    commitUsage = PhGetItemCircularBuffer_ULONG(SystemStatistics.CommitHistory, getTooltipText->Index);

                    PhMoveReference(&CommitGraphState.TooltipText, PhFormatString(
                        L"Commit: %s\n%s",
                        PhaFormatSize(UInt32x32To64(commitUsage, PAGE_SIZE), -1)->Buffer,
                        ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                    getTooltipText->Text = CommitGraphState.TooltipText->sr;
                }
                else if (ToolStatusConfig.IoGraphEnabled && Header->hwndFrom == IoGraphHandle)
                {
                    ULONG64 ioRead;
                    ULONG64 ioWrite;
                    ULONG64 ioOther;

                    ioRead = PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoReadHistory, getTooltipText->Index);
                    ioWrite = PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoWriteHistory, getTooltipText->Index);
                    ioOther = PhGetItemCircularBuffer_ULONG64(SystemStatistics.IoOtherHistory, getTooltipText->Index);

                    PhMoveReference(&IoGraphState.TooltipText, PhFormatString(
                        L"R: %s\nW: %s\nO: %s%s\n%s",
                        PhaFormatSize(ioRead, -1)->Buffer,
                        PhaFormatSize(ioWrite, -1)->Buffer,
                        PhaFormatSize(ioOther, -1)->Buffer,
                        PhGetStringOrEmpty(PhSipGetMaxIoString(getTooltipText->Index)),
                        ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                    getTooltipText->Text = IoGraphState.TooltipText->sr;
                }
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Header;
            PPH_PROCESS_RECORD record = NULL;

            if (ToolStatusConfig.CpuGraphEnabled && Header->hwndFrom == CpuGraphHandle)
            {
                if (mouseEvent->Message == WM_RBUTTONUP)
                {
                    ShowCustomizeMenu();
                }
                else
                {
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
            }
            else if (ToolStatusConfig.MemGraphEnabled && Header->hwndFrom == MemGraphHandle)
            {
                if (mouseEvent->Message == WM_RBUTTONUP)
                {
                    ShowCustomizeMenu();
                }
            }
            else if (ToolStatusConfig.CommitGraphEnabled && Header->hwndFrom == CommitGraphHandle)
            {
                if (mouseEvent->Message == WM_RBUTTONUP)
                {
                    ShowCustomizeMenu();
                }
            }
            else if (ToolStatusConfig.IoGraphEnabled && Header->hwndFrom == IoGraphHandle)
            {
                if (mouseEvent->Message == WM_RBUTTONUP)
                {
                    ShowCustomizeMenu();
                }
                else
                {
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
            }
        }
        break;
    }
}