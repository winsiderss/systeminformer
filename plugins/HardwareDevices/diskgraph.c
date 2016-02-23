/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
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

#include "devices.h"

VOID DiskDriveUpdateGraphs(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    Context->GraphState.Valid = FALSE;
    Context->GraphState.TooltipIndex = -1;
    Graph_MoveGrid(Context->GraphHandle, 1);
    Graph_Draw(Context->GraphHandle);
    Graph_UpdateTooltip(Context->GraphHandle);
    InvalidateRect(Context->GraphHandle, NULL, FALSE);
}

VOID DiskDriveUpdatePanel(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BREAD, PhaFormatSize(Context->AdapterEntry->BytesReadDelta.Value, -1)->Buffer);
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BWRITE, PhaFormatSize(Context->AdapterEntry->BytesWrittenDelta.Value, -1)->Buffer);
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BTOTAL, PhaFormatSize(Context->AdapterEntry->BytesReadDelta.Value + Context->AdapterEntry->BytesWrittenDelta.Value, -1)->Buffer);

    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_ACTIVE,
        PhaFormatString(L"%.0f%%", Context->AdapterEntry->ActiveTime)->Buffer
        );
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_RESPONSETIME,
        PhaFormatString(L"%.1f ms", Context->AdapterEntry->ResponseTime / PH_TICKS_PER_MS)->Buffer
        );
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_QUEUELENGTH,
        PhaFormatString(L"%lu", Context->AdapterEntry->QueueDepth)->Buffer
        );
}

VOID UpdateDiskDriveDialog(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    if (Context->AdapterEntry->DiskName)
        SetDlgItemText(Context->WindowHandle, IDC_DISKNAME, Context->AdapterEntry->DiskName->Buffer);
    else
        SetDlgItemText(Context->WindowHandle, IDC_DISKNAME, L"Unknown disk");

    if (Context->AdapterEntry->DiskIndexName)
        SetDlgItemText(Context->WindowHandle, IDC_ADAPTERNAME, Context->AdapterEntry->DiskIndexName->Buffer);
    else
        SetDlgItemText(Context->WindowHandle, IDC_ADAPTERNAME, L"Unknown disk");

    DiskDriveUpdateGraphs(Context);
    DiskDriveUpdatePanel(Context);
}

VOID UpdateDiskIndexText(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    if (Context->AdapterEntry->DiskIndex != ULONG_MAX && !Context->AdapterEntry->DiskIndexName)
    {
        PPH_STRING diskMountPoints = PH_AUTO_T(PH_STRING, DiskDriveQueryDosMountPoints(Context->AdapterEntry->DiskIndex));

        if (!PhIsNullOrEmptyString(diskMountPoints))
        {
            PhMoveReference(&Context->AdapterEntry->DiskIndexName, PhFormatString(
                L"Disk %lu (%s)",
                Context->AdapterEntry->DiskIndex,
                diskMountPoints->Buffer
                ));
        }
        else
        {
            PhMoveReference(&Context->AdapterEntry->DiskIndexName, PhFormatString(
                L"Disk %lu",
                Context->AdapterEntry->DiskIndex
                ));
        }
    }
}

INT_PTR CALLBACK DiskDrivePanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return FALSE;
}

INT_PTR CALLBACK DiskDriveDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_DISK_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_DISK_SYSINFO_CONTEXT)lParam;

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PDV_DISK_SYSINFO_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            PhDeleteLayoutManager(&context->LayoutManager);
            PhDeleteGraphState(&context->GraphState);

            if (context->GraphHandle)
                DestroyWindow(context->GraphHandle);

            if (context->PanelWindowHandle)
                DestroyWindow(context->PanelWindowHandle);

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

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ADAPTERNAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DISKNAME), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_ADAPTERNAME), WM_SETFONT, (WPARAM)context->SysinfoSection->Parameters->LargeFont, FALSE);
            SendMessage(GetDlgItem(hwndDlg, IDC_DISKNAME), WM_SETFONT, (WPARAM)context->SysinfoSection->Parameters->MediumFont, FALSE);

            if (context->AdapterEntry->DiskIndexName)
                SetDlgItemText(hwndDlg, IDC_ADAPTERNAME, context->AdapterEntry->DiskIndexName->Buffer);
            else
                SetDlgItemText(hwndDlg, IDC_ADAPTERNAME, L"Unknown disk");

            if (context->AdapterEntry->DiskName)
                SetDlgItemText(hwndDlg, IDC_DISKNAME, context->AdapterEntry->DiskName->Buffer);
            else
                SetDlgItemText(hwndDlg, IDC_DISKNAME, L"Unknown disk");

            context->PanelWindowHandle = CreateDialogParam(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_DISKDRIVE_PANEL), hwndDlg, DiskDrivePanelDialogProc, (LPARAM)context);
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

            UpdateDiskDriveDialog(context);
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

                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
                        context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));

                        PhGraphStateGetDrawInfo(
                            &context->GraphState,
                            getDrawInfo,
                            context->AdapterEntry->ReadBuffer.Count
                            );

                        if (!context->GraphState.Valid)
                        {
                            FLOAT max = 1024 * 1024; // minimum scaling of 1 MB.

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;
                                FLOAT data2;

                                context->GraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->AdapterEntry->ReadBuffer, i);
                                context->GraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->AdapterEntry->WriteBuffer, i);

                                if (max < data1 + data2)
                                    max = data1 + data2;
                            }

                            if (max != 0)
                            {
                                // Scale the data.
                                PhDivideSinglesBySingle(
                                    context->GraphState.Data1,
                                    max,
                                    drawInfo->LineDataCount
                                    );

                                // Scale the data.
                                PhDivideSinglesBySingle(
                                    context->GraphState.Data2,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                            }

                            drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;

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
                                    &context->AdapterEntry->ReadBuffer,
                                    getTooltipText->Index
                                    );

                                ULONG64 diskWriteValue = PhGetItemCircularBuffer_ULONG64(
                                    &context->AdapterEntry->WriteBuffer,
                                    getTooltipText->Index
                                    );

                                PhMoveReference(&context->GraphState.TooltipText, PhFormatString(
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
    case UPDATE_MSG:
        {
            UpdateDiskDriveDialog(context);
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
    PDV_DISK_SYSINFO_CONTEXT context = (PDV_DISK_SYSINFO_CONTEXT)Section->Context;

    switch (Message)
    {
    case SysInfoCreate:
        {
            UpdateDiskIndexText(context);
        }
        return TRUE;
    case SysInfoDestroy:
        {
            PhDereferenceObject(context->AdapterEntry);
            PhDereferenceObject(context->SectionName);
            PhFree(context);
        }
        return TRUE;
    case SysInfoTick:
        {
            UpdateDiskIndexText(context);

            if (context->WindowHandle)
                PostMessage(context->WindowHandle, UPDATE_MSG, 0, 0);
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, context->AdapterEntry->ReadBuffer.Count);

            if (!Section->GraphState.Valid)
            {
                FLOAT max = 1024 * 1024; // minimum scaling of 1 MB.

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    Section->GraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->AdapterEntry->ReadBuffer, i);
                    Section->GraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->AdapterEntry->WriteBuffer, i);

                    if (max < data1 + data2)
                        max = data1 + data2;
                }

                if (max != 0)
                {
                    // Scale the data.
                    PhDivideSinglesBySingle(
                        Section->GraphState.Data1,
                        max,
                        drawInfo->LineDataCount
                        );

                    // Scale the data.
                    PhDivideSinglesBySingle(
                        Section->GraphState.Data2,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = (PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT)Parameter1;

            ULONG64 diskReadValue = PhGetItemCircularBuffer_ULONG64(
                &context->AdapterEntry->ReadBuffer,
                getTooltipText->Index
                );

            ULONG64 diskWriteValue = PhGetItemCircularBuffer_ULONG64(
                &context->AdapterEntry->WriteBuffer,
                getTooltipText->Index
                );

            PhMoveReference(&Section->GraphState.TooltipText, PhFormatString(
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

            PhSetReference(&drawPanel->Title, context->AdapterEntry->DiskIndexName);
            drawPanel->SubTitle = PhFormatString(
                L"R: %s\nW: %s",
                PhaFormatSize(context->AdapterEntry->BytesReadDelta.Delta, -1)->Buffer,
                PhaFormatSize(context->AdapterEntry->BytesWrittenDelta.Delta, -1)->Buffer
                );

            if (!drawPanel->Title)
                drawPanel->Title = PhCreateString(L"Unknown disk");
        }
        return TRUE;
    }

    return FALSE;
}

VOID DiskDriveSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ PDV_DISK_ENTRY DiskEntry
    )
{
    PH_SYSINFO_SECTION section;
    PDV_DISK_SYSINFO_CONTEXT context;

    context = (PDV_DISK_SYSINFO_CONTEXT)PhAllocate(sizeof(DV_DISK_SYSINFO_CONTEXT));
    memset(context, 0, sizeof(DV_DISK_SYSINFO_CONTEXT));
    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));

    context->AdapterEntry = DiskEntry;
    context->SectionName = PhFormatString(L"Disk %s", DiskEntry->Id.DevicePath);

    section.Context = context;
    section.Callback = DiskDriveSectionCallback;
    section.Name = context->SectionName->sr;

    context->SysinfoSection = Pointers->CreateSection(&section);
}