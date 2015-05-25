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

#include "perfmon.h"

#define MSG_UPDATE (WM_APP + 1)

static VOID NTAPI ProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PERFMON_SYSINFO_CONTEXT context = Context;

    if (context->WindowHandle)
    {
        PostMessage(context->WindowHandle, MSG_UPDATE, 0, 0);
    }
}

static VOID PerfCounterUpdateGraphs(
    _Inout_ PPH_PERFMON_SYSINFO_CONTEXT Context
    )
{
    Context->GraphState.Valid = FALSE;
    Context->GraphState.TooltipIndex = -1;
    Graph_MoveGrid(Context->GraphHandle, 1);
    Graph_Draw(Context->GraphHandle);
    Graph_UpdateTooltip(Context->GraphHandle);
    InvalidateRect(Context->GraphHandle, NULL, FALSE);
}

static INT_PTR CALLBACK PerfCounterDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PERFMON_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_PERFMON_SYSINFO_CONTEXT)lParam;

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPH_PERFMON_SYSINFO_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            PhDeleteGraphState(&context->GraphState);

            if (context->GraphHandle)
                DestroyWindow(context->GraphHandle);

            PhUnregisterCallback(&PhProcessesUpdatedEvent, &context->ProcessesUpdatedRegistration);

            RemoveProp(hwndDlg, L"Context");
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_LAYOUT_ITEM panelItem;

            context->WindowHandle = hwndDlg;

            // Create the graph control.
            context->GraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                0,
                0,
                3,
                3,
                hwndDlg,
                NULL,
                NULL,
                NULL
                );
            Graph_SetTooltip(context->GraphHandle, TRUE);

            PhInitializeGraphState(&context->GraphState);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_COUNTERNAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItemEx(&context->LayoutManager, context->GraphHandle, NULL, PH_ANCHOR_ALL, panelItem->Margin);

            SendMessage(GetDlgItem(hwndDlg, IDC_COUNTERNAME), WM_SETFONT, (WPARAM)context->SysinfoSection->Parameters->LargeFont, FALSE);
            SetDlgItemText(hwndDlg, IDC_COUNTERNAME, context->SysinfoSection->Name.Buffer);

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                ProcessesUpdatedHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            PerfCounterUpdateGraphs(context);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_NOTIFY:
        {
            NMHDR* header = (NMHDR*)lParam;

            if (header->hwndFrom == context->GraphHandle)
            {
                switch (header->code)
                {
                case GCN_GETDRAWINFO:
                    {
                        PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                        PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                        drawInfo->Flags = PH_GRAPH_USE_GRID;
                        context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);

                        PhGraphStateGetDrawInfo(
                            &context->GraphState,
                            getDrawInfo,
                            context->HistoryBuffer.Count
                            );

                        if (!context->GraphState.Valid)
                        {
                            FLOAT maxGraphHeight = 0;

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                context->GraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&context->HistoryBuffer, i);

                                if (context->GraphState.Data1[i] > maxGraphHeight)
                                    maxGraphHeight = context->GraphState.Data1[i];
                            }

                            // Scale the data.
                            PhxfDivideSingle2U(
                                context->GraphState.Data1,
                                maxGraphHeight,
                                drawInfo->LineDataCount
                                );

                            context->GraphState.Valid = TRUE;
                        }
                    }
                    break;
                case GCN_GETTOOLTIPTEXT:
                    {
                        PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)header;

                        if (getTooltipText->Index < getTooltipText->TotalCount)
                        {
                            if (context->GraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG itemUsage = PhGetItemCircularBuffer_ULONG(
                                    &context->HistoryBuffer,
                                    getTooltipText->Index
                                    );

                                PhSwapReference2(&context->GraphState.TooltipText, PhFormatString(
                                    L"%u\n%s",
                                    itemUsage,
                                    ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                                    ));
                            }

                            getTooltipText->Text = context->GraphState.TooltipText->sr;
                        }
                    }
                    break;
                }
            }
        }
        break;
    case MSG_UPDATE:
        PerfCounterUpdateGraphs(context);
        break;
    }

    return FALSE;
}

static BOOLEAN PerfCounterSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    PPH_PERFMON_SYSINFO_CONTEXT context = (PPH_PERFMON_SYSINFO_CONTEXT)Section->Context;

    switch (Message)
    {
    case SysInfoCreate:
        {
            ULONG counterLength = 0;
            PDH_STATUS counterStatus = 0;
            //PPDH_COUNTER_INFO counterInfo;

            PhInitializeCircularBuffer_ULONG(&context->HistoryBuffer, PhGetIntegerSetting(L"SampleCount"));

            // Create the query handle.
            if ((counterStatus = PdhOpenQuery(NULL, (ULONG_PTR)NULL, &context->PerfQueryHandle)) != ERROR_SUCCESS)
            {
                PhShowError(NULL, L"PdhOpenQuery failed with status 0x%x.", counterStatus);
            }

            // Add the selected counter to the query handle.
            if ((counterStatus = PdhAddCounter(context->PerfQueryHandle, Section->Name.Buffer, 0, &context->PerfCounterHandle)))
            {
                PhShowError(NULL, L"PdhAddCounter failed with status 0x%x.", counterStatus);
            }

            //if ((counterStatus = PdhGetCounterInfo(context->PerfCounterHandle, TRUE, &counterLength, NULL)) == PDH_MORE_DATA)
            //{
            //    counterInfo = PhAllocate(counterLength);
            //    memset(counterInfo, 0, counterLength);
            //}

            //if ((counterStatus = PdhGetCounterInfo(context->PerfCounterHandle, TRUE, &counterLength, counterInfo)))
            //{
            //    PhShowError(NULL, L"PdhGetCounterInfo failed with status 0x%x.", counterStatus);
            //}
        }
        return TRUE;
    case SysInfoDestroy:
        {
            PhDeleteCircularBuffer_ULONG(&context->HistoryBuffer);

            // Close the query handle.
            if (context->PerfQueryHandle)
            {
                PdhCloseQuery(context->PerfQueryHandle);
            }

            PhFree(context);
        }
        return TRUE;
    case SysInfoTick:
        {
            ULONG counterType = 0;
            PDH_FMT_COUNTERVALUE displayValue = { 0 };

            // TODO: Handle this on a different thread.
            PdhCollectQueryData(context->PerfQueryHandle);

            //PdhSetCounterScaleFactor(context->PerfCounterHandle, PDH_MAX_SCALE);

            PdhGetFormattedCounterValue(
                context->PerfCounterHandle,
                PDH_FMT_LONG | PDH_FMT_NOSCALE | PDH_FMT_NOCAP100,
                &counterType,
                &displayValue
                );

            //if (counterType == PERF_COUNTER_COUNTER) {  }

            context->GraphValue = displayValue.longValue;

            PhAddItemCircularBuffer_ULONG(&context->HistoryBuffer, displayValue.longValue);
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = (PPH_SYSINFO_CREATE_DIALOG)Parameter1;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_PERFMON_DIALOG);
            createDialog->DialogProc = PerfCounterDialogProc;
            createDialog->Parameter = context;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);

            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, context->HistoryBuffer.Count);

            if (!Section->GraphState.Valid)
            {
                FLOAT maxGraphHeight = 0;

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    Section->GraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&context->HistoryBuffer, i);

                    if (Section->GraphState.Data1[i] > maxGraphHeight)
                        maxGraphHeight = Section->GraphState.Data1[i];
                }

                // Scale the data.
                PhxfDivideSingle2U(
                    Section->GraphState.Data1,
                    maxGraphHeight,
                    drawInfo->LineDataCount
                    );

                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = (PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT)Parameter1;

            ULONG counterValue = PhGetItemCircularBuffer_ULONG(
                &context->HistoryBuffer,
                getTooltipText->Index
                );

            PhSwapReference2(&Section->GraphState.TooltipText, PhFormatString(
                L"%u\n%s",
                counterValue,
                ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));

            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = (PPH_SYSINFO_DRAW_PANEL)Parameter1;

            drawPanel->Title = PhCreateString(Section->Name.Buffer);
            drawPanel->SubTitle = PhFormatString(
                L"%u",
                context->GraphValue
                );
        }
        return TRUE;
    }

    return FALSE;
}

VOID PerfCounterSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ PPH_STRING CounterName
    )
{
    PH_SYSINFO_SECTION section;
    PPH_PERFMON_SYSINFO_CONTEXT context;

    context = (PPH_PERFMON_SYSINFO_CONTEXT)PhAllocate(sizeof(PH_PERFMON_SYSINFO_CONTEXT));
    memset(context, 0, sizeof(PH_PERFMON_SYSINFO_CONTEXT));
    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));

    section.Context = context;
    section.Callback = PerfCounterSectionCallback;

    PhInitializeStringRef(&section.Name, CounterName->Buffer);

    context->SysinfoSection = Pointers->CreateSection(&section);
}