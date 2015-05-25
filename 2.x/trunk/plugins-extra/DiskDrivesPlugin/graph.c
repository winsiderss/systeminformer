/*
 * Process Hacker Extra Plugins -
 *   Disk Drives Plugin
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

#define MSG_UPDATE (WM_APP + 1)
//#define MSG_UPDATE_PANEL (WM_APP + 2)

static VOID NTAPI ProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_DISKDRIVE_SYSINFO_CONTEXT context = Context;

    if (context->WindowHandle)
    {
        PostMessage(context->WindowHandle, MSG_UPDATE, 0, 0);
    }
}

static VOID DiskDriveUpdateGraphs(
    _Inout_ PPH_DISKDRIVE_SYSINFO_CONTEXT Context
    )
{
    Context->GraphState.Valid = FALSE;
    Context->GraphState.TooltipIndex = -1;
    Graph_MoveGrid(Context->GraphHandle, 1);
    Graph_Draw(Context->GraphHandle);
    Graph_UpdateTooltip(Context->GraphHandle);
    InvalidateRect(Context->GraphHandle, NULL, FALSE);
}

static VOID DiskDriveUpdatePanel(
    _Inout_ PPH_DISKDRIVE_SYSINFO_CONTEXT Context
    )
{
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BREAD, PhaFormatSize(Context->LastBytesReadValue, -1)->Buffer);
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BWRITE, PhaFormatSize(Context->LastBytesWriteValue, -1)->Buffer);
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BTOTAL, PhaFormatSize(Context->LastBytesReadValue + Context->LastBytesWriteValue, -1)->Buffer);

    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_ACTIVE,
        PhaFormatString(L"Active Time: %.0f%%", Context->ActiveTime)->Buffer
        );
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_RESPONSETIME,
        PhaFormatString(L"Average Response Time: %s ms", PhaFormatUInt64(Context->ResponseTime, TRUE)->Buffer)->Buffer
        );
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_QUEUELENGTH,
        PhaFormatString(L"Queue Length: %u", Context->QueueDepth)->Buffer
        );
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_CAPACITY,
        PhaFormatString(L"Capacity: %s", Context->DiskLength->Buffer)->Buffer
        );
}

static INT_PTR CALLBACK DiskDrivePanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return FALSE;
}

static INT_PTR CALLBACK DiskDriveDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_DISKDRIVE_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_DISKDRIVE_SYSINFO_CONTEXT)lParam;

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPH_DISKDRIVE_SYSINFO_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_NCDESTROY)
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            PhDeleteGraphState(&context->GraphState);

            if (context->GraphHandle)
                DestroyWindow(context->GraphHandle);

            if (context->PanelWindowHandle)
                DestroyWindow(context->PanelWindowHandle);

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
            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;

            context->WindowHandle = hwndDlg;

            PhInitializeGraphState(&context->GraphState);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DISKDRIVE_NAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_DISKDRIVE_NAME), WM_SETFONT, (WPARAM)context->SysinfoSection->Parameters->LargeFont, FALSE);
            SetDlgItemText(hwndDlg, IDC_DISKDRIVE_NAME, context->SysinfoSection->Name.Buffer);

            context->PanelWindowHandle = CreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_DISKDRIVE_PANEL), hwndDlg, DiskDrivePanelDialogProc);
            ShowWindow(context->PanelWindowHandle, SW_SHOW);
            PhAddLayoutItemEx(&context->LayoutManager, context->PanelWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

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

            PhAddLayoutItemEx(&context->LayoutManager, context->GraphHandle, NULL, PH_ANCHOR_ALL, graphItem->Margin);

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                ProcessesUpdatedHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            DiskDriveUpdateGraphs(context);
            DiskDriveUpdatePanel(context);
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

                        drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                        context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), PhGetIntegerSetting(L"ColorCpuUser"));

                        PhGraphStateGetDrawInfo(
                            &context->GraphState,
                            getDrawInfo,
                            context->ReadBuffer.Count
                            );

                        if (!context->GraphState.Valid)
                        {
                            FLOAT max = 0;

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;
                                FLOAT data2;

                                context->GraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->ReadBuffer, i);
                                context->GraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->WriteBuffer, i);

                                if (max < data1 + data2)
                                    max = data1 + data2;
                            }

                            // Scale the data.
                            PhxfDivideSingle2U(
                                context->GraphState.Data1,
                                max,
                                drawInfo->LineDataCount
                                );

                            // Scale the data.
                            PhxfDivideSingle2U(
                                context->GraphState.Data2,
                                max,
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
                                ULONG64 diskReadValue = PhGetItemCircularBuffer_ULONG64(
                                    &context->ReadBuffer,
                                    getTooltipText->Index
                                    );

                                ULONG64 diskWriteValue = PhGetItemCircularBuffer_ULONG64(
                                    &context->WriteBuffer,
                                    getTooltipText->Index
                                    );

                                PhSwapReference2(&context->GraphState.TooltipText, PhFormatString(
                                    L"R: %s\nW: %s\n%s",
                                    PhaFormatSize(diskReadValue, -1)->Buffer,
                                    PhaFormatSize(diskWriteValue, -1)->Buffer,
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
        {
            DiskDriveUpdateGraphs(context);
            DiskDriveUpdatePanel(context);
        }
        break;
    }

    return FALSE;
}

static BOOLEAN DiskDriveSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    PPH_DISKDRIVE_SYSINFO_CONTEXT context = (PPH_DISKDRIVE_SYSINFO_CONTEXT)Section->Context;

    switch (Message)
    {
    case SysInfoCreate:
        {
            PhCreateFileWin32(
                &context->DeviceHandle,
                context->DiskEntry->DiskDevicePath->Buffer,
                PhElevated ? FILE_GENERIC_READ | FILE_GENERIC_WRITE : FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );

            context->DiskLength = DiskDriveQueryGeometry(context->DeviceHandle);

            PhInitializeCircularBuffer_ULONG64(&context->ReadBuffer, PhGetIntegerSetting(L"SampleCount"));
            PhInitializeCircularBuffer_ULONG64(&context->WriteBuffer, PhGetIntegerSetting(L"SampleCount"));
        }
        return TRUE;
    case SysInfoDestroy:
        {
            PhSwapReference2(&context->DiskLength, NULL);

            PhDeleteCircularBuffer_ULONG64(&context->ReadBuffer);
            PhDeleteCircularBuffer_ULONG64(&context->WriteBuffer);

            if (context->DeviceHandle)
                NtClose(context->DeviceHandle);

            PhFree(context);
        }
        return TRUE;
    case SysInfoTick:
        {
            ULONG64 diskBytesReadOctets = 0;
            ULONG64 diskBytesWrittenOctets = 0;
            ULONG64 diskBytesRead = 0;
            ULONG64 diskBytesWritten = 0;

            if (context->DeviceHandle)
            {
                DISK_PERFORMANCE diskPerformance;

                if (NT_SUCCESS(DiskDriveQueryStatistics(context->DeviceHandle, &diskPerformance)))
                {
                    ULONG64 ReadTime = diskPerformance.ReadTime.QuadPart - context->LastReadTime;
                    ULONG64 WriteTime = diskPerformance.WriteTime.QuadPart - context->LastWriteTime;
                    ULONG64 IdleTime = diskPerformance.IdleTime.QuadPart - context->LastIdletime;
                    ULONG64 QueryTime = diskPerformance.QueryTime.QuadPart - context->LastQueryTime;

                    diskBytesReadOctets = diskPerformance.BytesRead.QuadPart;
                    diskBytesWrittenOctets = diskPerformance.BytesWritten.QuadPart;
                    diskBytesRead = diskBytesReadOctets - context->LastBytesReadValue;
                    diskBytesWritten = diskBytesWrittenOctets - context->LastBytesWriteValue;

                    if (QueryTime != 0)
                    {
                        // TODO: check math... Task Manager has better precision.
                        context->ResponseTime = (ReadTime + WriteTime / QueryTime) / PH_TICKS_PER_MS; // ReadTime + WriteTime + IdleTime / PhGetIntegerSetting(L"SampleCount")
                        context->ActiveTime = (FLOAT)(QueryTime - IdleTime) / QueryTime * 100;
                    }
                    else
                    {
                        // If querytime is 0 then enough time hasn't passed to calculate results.
                        context->ResponseTime = 0;
                        context->ActiveTime = 0.0f;
                    }

                    if (context->ActiveTime > 100.f)
                        context->ActiveTime = 0.f;
                    if (context->ActiveTime < 0.f)
                        context->ActiveTime = 0.f;

                    context->LastReadTime = diskPerformance.ReadTime.QuadPart;
                    context->LastWriteTime = diskPerformance.WriteTime.QuadPart;
                    context->LastIdletime = diskPerformance.IdleTime.QuadPart;
                    context->LastQueryTime = diskPerformance.QueryTime.QuadPart;
                    context->QueueDepth = diskPerformance.QueueDepth;
                    context->SplitCount = diskPerformance.SplitCount;
                }

                // HACK: Pull the Disk name from the current query.
                //if (context->SysinfoSection->Name.Length == 0)
                //{
                //    if (DiskDriveQueryDeviceInformation(context->DeviceHandle, NULL, &context->DiskName, NULL, NULL))
                //    {
                //        context->SysinfoSection->Name = context->DiskName->sr;
                //    }
                //}
            }

            if (!context->HaveFirstSample)
            {
                diskBytesRead = 0;
                diskBytesWritten = 0;
                context->HaveFirstSample = TRUE;
            }

            PhAddItemCircularBuffer_ULONG64(&context->ReadBuffer, diskBytesRead);
            PhAddItemCircularBuffer_ULONG64(&context->WriteBuffer, diskBytesWritten);

            context->BytesReadValue = diskBytesRead;
            context->BytesWriteValue = diskBytesWritten;
            context->LastBytesReadValue = diskBytesReadOctets;
            context->LastBytesWriteValue = diskBytesWrittenOctets;
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = (PPH_SYSINFO_CREATE_DIALOG)Parameter1;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_DISKDRIVE_DIALOG);
            createDialog->DialogProc = DiskDriveDialogProc;
            createDialog->Parameter = context;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), PhGetIntegerSetting(L"ColorCpuUser"));
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, context->ReadBuffer.Count);

            if (!Section->GraphState.Valid)
            {
                FLOAT max = 0;

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    Section->GraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->ReadBuffer, i);
                    Section->GraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->WriteBuffer, i);

                    if (max < data1 + data2)
                        max = data1 + data2;
                }

                // Scale the data.
                PhxfDivideSingle2U(
                    Section->GraphState.Data1,
                    max,
                    drawInfo->LineDataCount
                    );

                // Scale the data.
                PhxfDivideSingle2U(
                    Section->GraphState.Data2,
                    max,
                    drawInfo->LineDataCount
                    );
                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = (PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT)Parameter1;

            ULONG64 diskReadValue = PhGetItemCircularBuffer_ULONG64(
                &context->ReadBuffer,
                getTooltipText->Index
                );

            ULONG64 diskWriteValue = PhGetItemCircularBuffer_ULONG64(
                &context->WriteBuffer,
                getTooltipText->Index
                );

            PhSwapReference2(&Section->GraphState.TooltipText, PhFormatString(
                L"R: %s\nW: %s\n%s",
                PhaFormatSize(diskReadValue, -1)->Buffer,
                PhaFormatSize(diskWriteValue, -1)->Buffer,
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
                L"R: %s\nW: %s",
                PhaFormatSize(context->BytesReadValue, -1)->Buffer,
                PhaFormatSize(context->BytesWriteValue, -1)->Buffer
                );
        }
        return TRUE;
    }

    return FALSE;
}

VOID DiskDriveSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ PPH_DISK_ENTRY DiskEntry
    )
{
    PH_SYSINFO_SECTION section;
    PPH_DISKDRIVE_SYSINFO_CONTEXT context;

    context = (PPH_DISKDRIVE_SYSINFO_CONTEXT)PhAllocate(sizeof(PH_DISKDRIVE_SYSINFO_CONTEXT));
    memset(context, 0, sizeof(PH_DISKDRIVE_SYSINFO_CONTEXT));
    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));

    context->DiskEntry = DiskEntry;

    section.Context = context;
    section.Callback = DiskDriveSectionCallback;

    PhInitializeStringRef(&section.Name, DiskEntry->DiskFriendlyName->Buffer);

    context->SysinfoSection = Pointers->CreateSection(&section);
}