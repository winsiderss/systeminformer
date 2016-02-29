/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2015-2016 dmex
 * Copyright (C) 2016 wj32
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

VOID NetAdapterUpdateGraphs(
    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    Context->GraphState.Valid = FALSE;
    Context->GraphState.TooltipIndex = -1;
    Graph_MoveGrid(Context->GraphHandle, 1);
    Graph_Draw(Context->GraphHandle);
    Graph_UpdateTooltip(Context->GraphHandle);
    InvalidateRect(Context->GraphHandle, NULL, FALSE);
}

VOID NetAdapterUpdatePanel(
    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    ULONG64 inOctets = 0;
    ULONG64 outOctets = 0;
    ULONG64 linkSpeed = 0;
    NDIS_MEDIA_CONNECT_STATE mediaState = MediaConnectStateUnknown;
    HANDLE deviceHandle = NULL;

    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_NDIS))
    {
        // Create the handle to the network device
        if (NT_SUCCESS(NetworkAdapterCreateHandle(&deviceHandle, Context->AdapterEntry->Id.InterfaceGuid)))
        {
            if (!Context->AdapterEntry->CheckedDeviceSupport)
            {
                // Check the network adapter supports the OIDs we're going to be using.
                if (NetworkAdapterQuerySupported(deviceHandle))
                {
                    Context->AdapterEntry->DeviceSupported = TRUE;
                }

                Context->AdapterEntry->CheckedDeviceSupport = TRUE;
            }

            if (!Context->AdapterEntry->DeviceSupported)
            {
                // Device is faulty. Close the handle so we can fallback to GetIfEntry.
                NtClose(deviceHandle);
                deviceHandle = NULL;
            }
        }
    }

    if (deviceHandle)
    {
        NDIS_STATISTICS_INFO interfaceStats;
        NDIS_LINK_STATE interfaceState;

        if (NT_SUCCESS(NetworkAdapterQueryStatistics(deviceHandle, &interfaceStats)))
        {
            if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV))
                inOctets = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_RCV);
            else
                inOctets = interfaceStats.ifHCInOctets;

            if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT))
                outOctets = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_XMIT);
            else
                outOctets = interfaceStats.ifHCOutOctets;
        }
        else
        {
            // Note: The above code fails for some drivers that don't implement statistics (even though statistics are mandatory).
            // NDIS handles these two OIDs for all miniport drivers and we can use these for those special cases.

            // https://msdn.microsoft.com/en-us/library/ff569443.aspx
            inOctets = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_RCV);

            // https://msdn.microsoft.com/en-us/library/ff569445.aspx
            outOctets = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_XMIT);
        }

        if (NT_SUCCESS(NetworkAdapterQueryLinkState(deviceHandle, &interfaceState)))
        {
            mediaState = interfaceState.MediaConnectState;
            linkSpeed = interfaceState.XmitLinkSpeed;
        }
        else
        {
            NetworkAdapterQueryLinkSpeed(deviceHandle, &linkSpeed);
        }

        NtClose(deviceHandle);
    }
    else if (WindowsVersion >= WINDOWS_VISTA && GetIfEntry2)
    {
        MIB_IF_ROW2 interfaceRow;

        if (QueryInterfaceRowVista(&Context->AdapterEntry->Id, &interfaceRow))
        {
            inOctets = interfaceRow.InOctets;
            outOctets = interfaceRow.OutOctets;
            mediaState = interfaceRow.MediaConnectState;
            linkSpeed = interfaceRow.TransmitLinkSpeed;
        }
    }
    else
    {
        MIB_IFROW interfaceRow;

        if (QueryInterfaceRowXP(&Context->AdapterEntry->Id, &interfaceRow))
        {
            inOctets = interfaceRow.dwInOctets;
            outOctets = interfaceRow.dwOutOctets;
            mediaState = interfaceRow.dwOperStatus == IF_OPER_STATUS_OPERATIONAL ? MediaConnectStateConnected : MediaConnectStateUnknown;
            linkSpeed = interfaceRow.dwSpeed;
        }
    }

    if (mediaState == MediaConnectStateConnected)
        SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Connected");
    else
        SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Disconnected");

    SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_SPEED, PhaFormatString(L"%s/s", PhaFormatSize(linkSpeed / BITS_IN_ONE_BYTE, -1)->Buffer)->Buffer);
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BSENT, PhaFormatSize(outOctets, -1)->Buffer);
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BRECEIVED, PhaFormatSize(inOctets, -1)->Buffer);
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BTOTAL, PhaFormatSize(inOctets + outOctets, -1)->Buffer);
}

VOID UpdateNetAdapterDialog(
    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    if (Context->AdapterEntry->AdapterName)
        SetDlgItemText(Context->WindowHandle, IDC_ADAPTERNAME, Context->AdapterEntry->AdapterName->Buffer);
    else
        SetDlgItemText(Context->WindowHandle, IDC_ADAPTERNAME, L"Unknown network adapter");

    NetAdapterUpdateGraphs(Context);
    NetAdapterUpdatePanel(Context);
}

INT_PTR CALLBACK NetAdapterPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_NETADAPTER_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_NETADAPTER_SYSINFO_CONTEXT)lParam;

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PDV_NETADAPTER_SYSINFO_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_NCDESTROY)
        {
            RemoveProp(hwndDlg, L"Context");
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_DETAILS:
                ShowNetAdapterDetailsDialog(context);
                break;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK NetAdapterDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_NETADAPTER_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_NETADAPTER_SYSINFO_CONTEXT)lParam;

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PDV_NETADAPTER_SYSINFO_CONTEXT)GetProp(hwndDlg, L"Context");

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

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ADAPTERNAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_ADAPTERNAME), WM_SETFONT, (WPARAM)context->SysinfoSection->Parameters->LargeFont, FALSE);

            if (context->AdapterEntry->AdapterName)
                SetDlgItemText(hwndDlg, IDC_ADAPTERNAME, context->AdapterEntry->AdapterName->Buffer);
            else
                SetDlgItemText(hwndDlg, IDC_ADAPTERNAME, L"Unknown network adapter");

            context->PanelWindowHandle = CreateDialogParam(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_NETADAPTER_PANEL), hwndDlg, NetAdapterPanelDialogProc, (LPARAM)context);
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

            UpdateNetAdapterDialog(context);
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
                            context->AdapterEntry->InboundBuffer.Count
                            );

                        if (!context->GraphState.Valid)
                        {
                            ULONG i;
                            FLOAT max = 4 * 1024; // Minimum scaling 4KB

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;
                                FLOAT data2;

                                context->GraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->AdapterEntry->InboundBuffer, i);
                                context->GraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->AdapterEntry->OutboundBuffer, i);

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
                                ULONG64 adapterInboundValue = PhGetItemCircularBuffer_ULONG64(
                                    &context->AdapterEntry->InboundBuffer,
                                    getTooltipText->Index
                                    );

                                ULONG64 adapterOutboundValue = PhGetItemCircularBuffer_ULONG64(
                                    &context->AdapterEntry->OutboundBuffer,
                                    getTooltipText->Index
                                    );

                                PhMoveReference(&context->GraphState.TooltipText, PhFormatString(
                                    L"R: %s\nS: %s\n%s",
                                    PhaFormatSize(adapterInboundValue, -1)->Buffer,
                                    PhaFormatSize(adapterOutboundValue, -1)->Buffer,
                                    ((PPH_STRING)PH_AUTO(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
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
            UpdateNetAdapterDialog(context);
        }
        break;
    }

    return FALSE;
}

BOOLEAN NetAdapterSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    PDV_NETADAPTER_SYSINFO_CONTEXT context = (PDV_NETADAPTER_SYSINFO_CONTEXT)Section->Context;

    switch (Message)
    {
    case SysInfoCreate:
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
            if (context->WindowHandle)
                PostMessage(context->WindowHandle, UPDATE_MSG, 0, 0);
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = (PPH_SYSINFO_CREATE_DIALOG)Parameter1;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_NETADAPTER_DIALOG);
            createDialog->DialogProc = NetAdapterDialogProc;
            createDialog->Parameter = context;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, context->AdapterEntry->InboundBuffer.Count);

            if (!Section->GraphState.Valid)
            {
                FLOAT max = 4 * 1024; // Minimum scaling 4KB

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    Section->GraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->AdapterEntry->InboundBuffer, i);
                    Section->GraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->AdapterEntry->OutboundBuffer, i);

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

            ULONG64 adapterInboundValue = PhGetItemCircularBuffer_ULONG64(
                &context->AdapterEntry->InboundBuffer,
                getTooltipText->Index
                );

            ULONG64 adapterOutboundValue = PhGetItemCircularBuffer_ULONG64(
                &context->AdapterEntry->OutboundBuffer,
                getTooltipText->Index
                );

            PhMoveReference(&Section->GraphState.TooltipText, PhFormatString(
                L"R: %s\nS: %s\n%s",
                PhaFormatSize(adapterInboundValue, -1)->Buffer,
                PhaFormatSize(adapterOutboundValue, -1)->Buffer,
                ((PPH_STRING)PH_AUTO(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));

            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = (PPH_SYSINFO_DRAW_PANEL)Parameter1;

            PhSetReference(&drawPanel->Title, context->AdapterEntry->AdapterName);
            drawPanel->SubTitle = PhFormatString(
                L"R: %s\nS: %s",
                PhaFormatSize(context->AdapterEntry->InboundValue, -1)->Buffer,
                PhaFormatSize(context->AdapterEntry->OutboundValue, -1)->Buffer
                );

            if (!drawPanel->Title)
                drawPanel->Title = PhCreateString(L"Unknown network adapter");
        }
        return TRUE;
    }

    return FALSE;
}

VOID NetAdapterSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ _Assume_refs_(1) PDV_NETADAPTER_ENTRY AdapterEntry
    )
{
    PH_SYSINFO_SECTION section;
    PDV_NETADAPTER_SYSINFO_CONTEXT context;

    context = (PDV_NETADAPTER_SYSINFO_CONTEXT)PhAllocate(sizeof(DV_NETADAPTER_SYSINFO_CONTEXT));
    memset(context, 0, sizeof(DV_NETADAPTER_SYSINFO_CONTEXT));
    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));

    context->AdapterEntry = AdapterEntry;
    context->SectionName = PhConcatStrings2(L"NetAdapter ", AdapterEntry->Id.InterfaceGuid->Buffer);

    section.Context = context;
    section.Callback = NetAdapterSectionCallback;
    section.Name = context->SectionName->sr;

    context->SysinfoSection = Pointers->CreateSection(&section);
}