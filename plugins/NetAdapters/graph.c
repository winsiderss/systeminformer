/*
 * Process Hacker Extra Plugins -
 *   Network Adapters Plugin
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

static VOID NTAPI ProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_NETADAPTER_SYSINFO_CONTEXT context = Context;

    if (context->WindowHandle)
    {
        PostMessage(context->WindowHandle, MSG_UPDATE, 0, 0);
    }
}

static VOID NetAdapterUpdateGraphs(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    Context->GraphState.Valid = FALSE;
    Context->GraphState.TooltipIndex = -1;
    Graph_MoveGrid(Context->GraphHandle, 1);
    Graph_Draw(Context->GraphHandle);
    Graph_UpdateTooltip(Context->GraphHandle);
    InvalidateRect(Context->GraphHandle, NULL, FALSE);
}

static VOID NetAdapterUpdatePanel(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    ULONG64 inOctets = 0;
    ULONG64 outOctets = 0;
    ULONG64 linkSpeed = 0;
    NDIS_MEDIA_CONNECT_STATE mediaState = MediaConnectStateUnknown;

    if (Context->DeviceHandle)
    {
        NDIS_STATISTICS_INFO interfaceStats;
        NDIS_LINK_STATE interfaceState;

        if (NT_SUCCESS(NetworkAdapterQueryStatistics(Context->DeviceHandle, &interfaceStats)))
        {
            if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV))
                inOctets = NetworkAdapterQueryValue(Context->DeviceHandle, OID_GEN_BYTES_RCV);
            else
                inOctets = interfaceStats.ifHCInOctets;

            if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT))
                outOctets = NetworkAdapterQueryValue(Context->DeviceHandle, OID_GEN_BYTES_XMIT);
            else
                outOctets = interfaceStats.ifHCOutOctets;
        }
        else
        {
            // Note: The above code fails for some drivers that don't implement statistics (even though statistics are mandatory).
            // NDIS handles these two OIDs for all miniport drivers and we can use these for those special cases.

            // https://msdn.microsoft.com/en-us/library/ff569443.aspx
            inOctets = NetworkAdapterQueryValue(Context->DeviceHandle, OID_GEN_BYTES_RCV);

            // https://msdn.microsoft.com/en-us/library/ff569445.aspx
            outOctets = NetworkAdapterQueryValue(Context->DeviceHandle, OID_GEN_BYTES_XMIT);
        }

        if (NT_SUCCESS(NetworkAdapterQueryLinkState(Context->DeviceHandle, &interfaceState)))
        {
            mediaState = interfaceState.MediaConnectState;
            linkSpeed = interfaceState.XmitLinkSpeed;
        }
        else
        {
            NetworkAdapterQueryLinkSpeed(Context->DeviceHandle, &linkSpeed);
        }
    }
    else if (GetIfEntry2_I)
    {
        MIB_IF_ROW2 interfaceRow;

        interfaceRow = QueryInterfaceRowVista(Context->AdapterEntry);

        inOctets = interfaceRow.InOctets;
        outOctets = interfaceRow.OutOctets;
        mediaState = interfaceRow.MediaConnectState;
        linkSpeed = interfaceRow.TransmitLinkSpeed;
    }
    else
    {
        MIB_IFROW interfaceRow;

        interfaceRow = QueryInterfaceRowXP(Context->AdapterEntry);

        inOctets = interfaceRow.dwInOctets;
        outOctets = interfaceRow.dwOutOctets;
        linkSpeed = interfaceRow.dwSpeed;

        if (interfaceRow.dwOperStatus == IF_OPER_STATUS_OPERATIONAL)
            mediaState = MediaConnectStateConnected;
        else
            mediaState = MediaConnectStateDisconnected;
    }

    if (mediaState == MediaConnectStateConnected)
        SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Connected");
    else
        SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Disconnected");

    SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_SPEED, PhaFormatString(L"%s/s", PhaFormatSize(linkSpeed / BITS_IN_ONE_BYTE, -1)->Buffer)->Buffer);
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BSENT, PhaFormatSize(outOctets, -1)->Buffer);
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BRECIEVED, PhaFormatSize(inOctets, -1)->Buffer);
    SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BTOTAL, PhaFormatSize(inOctets + outOctets, -1)->Buffer);
}

static INT_PTR CALLBACK NetAdapterPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_NETADAPTER_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_NETADAPTER_SYSINFO_CONTEXT)lParam;

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPH_NETADAPTER_SYSINFO_CONTEXT)GetProp(hwndDlg, L"Context");

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
            switch (LOWORD(wParam))
            {
            case IDC_DETAILS:
                ShowDetailsDialog(context);
                break;
            }
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK NetAdapterDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_NETADAPTER_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_NETADAPTER_SYSINFO_CONTEXT)lParam;

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPH_NETADAPTER_SYSINFO_CONTEXT)GetProp(hwndDlg, L"Context");

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

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ADAPTERNAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_ADAPTERNAME), WM_SETFONT, (WPARAM)context->SysinfoSection->Parameters->LargeFont, FALSE);
            SetDlgItemText(hwndDlg, IDC_ADAPTERNAME, context->SysinfoSection->Name.Buffer);

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

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                ProcessesUpdatedHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            NetAdapterUpdateGraphs(context);
            NetAdapterUpdatePanel(context);
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
                        context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));

                        PhGraphStateGetDrawInfo(
                            &context->GraphState,
                            getDrawInfo,
                            context->InboundBuffer.Count
                            );

                        if (!context->GraphState.Valid)
                        {
                            ULONG i;
                            FLOAT max = 0;

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;
                                FLOAT data2;

                                context->GraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->InboundBuffer, i);
                                context->GraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->OutboundBuffer, i);

                                if (max < data1 + data2)
                                    max = data1 + data2;
                            }

                            // Minimum scaling of 1 MB.
                            //if (max < 1024 * 1024)
                            //    max = 1024 * 1024;

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
                                    &context->InboundBuffer,
                                    getTooltipText->Index
                                    );

                                ULONG64 adapterOutboundValue = PhGetItemCircularBuffer_ULONG64(
                                    &context->OutboundBuffer,
                                    getTooltipText->Index
                                    );

                                PhMoveReference(&context->GraphState.TooltipText, PhFormatString(
                                    L"R: %s\nS: %s\n%s",
                                    PhaFormatSize(adapterInboundValue, -1)->Buffer,
                                    PhaFormatSize(adapterOutboundValue, -1)->Buffer,
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
            NetAdapterUpdateGraphs(context);
            NetAdapterUpdatePanel(context);
        }
        break;
    }

    return FALSE;
}

static BOOLEAN NetAdapterSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    PPH_NETADAPTER_SYSINFO_CONTEXT context = (PPH_NETADAPTER_SYSINFO_CONTEXT)Section->Context;

    switch (Message)
    {
    case SysInfoCreate:
        {
            if (PhGetIntegerSetting(SETTING_NAME_ENABLE_NDIS))
            {
                // Create the handle to the network device
                PhCreateFileWin32(
                    &context->DeviceHandle,
                    PhaFormatString(L"\\\\.\\%s", context->AdapterEntry->InterfaceGuid->Buffer)->Buffer,
                    FILE_GENERIC_READ,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                    );

                if (context->DeviceHandle)
                {
                    // Check the network adapter supports the OIDs we're going to be using.
                    if (!NetworkAdapterQuerySupported(context->DeviceHandle))
                    {
                        // Device is faulty. Close the handle so we can fallback to GetIfEntry.
                        NtClose(context->DeviceHandle);
                        context->DeviceHandle = NULL;
                    }
                }
            }

            PhInitializeCircularBuffer_ULONG64(&context->InboundBuffer, PhGetIntegerSetting(L"SampleCount"));
            PhInitializeCircularBuffer_ULONG64(&context->OutboundBuffer, PhGetIntegerSetting(L"SampleCount"));
        }
        return TRUE;
    case SysInfoDestroy:
        {
            PhDeleteCircularBuffer_ULONG64(&context->InboundBuffer);
            PhDeleteCircularBuffer_ULONG64(&context->OutboundBuffer);

            if (context->AdapterName)
                PhDereferenceObject(context->AdapterName);

            if (context->DeviceHandle)
                NtClose(context->DeviceHandle);

            PhFree(context);
        }
        return TRUE;
    case SysInfoTick:
        {
            ULONG64 networkInOctets = 0;
            ULONG64 networkOutOctets = 0;
            ULONG64 networkRcvSpeed = 0;
            ULONG64 networkXmitSpeed = 0;
            //ULONG64 networkLinkSpeed = 0;

            if (context->DeviceHandle)
            {
                NDIS_STATISTICS_INFO interfaceStats;
                //NDIS_LINK_STATE interfaceState;

                if (NT_SUCCESS(NetworkAdapterQueryStatistics(context->DeviceHandle, &interfaceStats)))
                {
                    if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV))
                        networkInOctets = NetworkAdapterQueryValue(context->DeviceHandle, OID_GEN_BYTES_RCV);
                    else
                        networkInOctets = interfaceStats.ifHCInOctets;

                    if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT))
                        networkOutOctets = NetworkAdapterQueryValue(context->DeviceHandle, OID_GEN_BYTES_XMIT);
                    else
                        networkOutOctets = interfaceStats.ifHCOutOctets;

                    networkRcvSpeed = networkInOctets - context->LastInboundValue;
                    networkXmitSpeed = networkOutOctets - context->LastOutboundValue;
                }
                else
                {
                    networkInOctets = NetworkAdapterQueryValue(context->DeviceHandle, OID_GEN_BYTES_RCV);
                    networkOutOctets = NetworkAdapterQueryValue(context->DeviceHandle, OID_GEN_BYTES_XMIT);

                    networkRcvSpeed = networkInOctets - context->LastInboundValue;
                    networkXmitSpeed = networkOutOctets - context->LastOutboundValue;
                }

                //if (NT_SUCCESS(NetworkAdapterQueryLinkState(context->DeviceHandle, &interfaceState)))
                //{
                //    networkLinkSpeed = interfaceState.XmitLinkSpeed;
                //}
                //else
                //{
                //    NetworkAdapterQueryLinkSpeed(context->DeviceHandle, &networkLinkSpeed);
                //}

                // HACK: Pull the Adapter name from the current query.
                if (context->SysinfoSection->Name.Length == 0)
                {
                    if (context->AdapterName = NetworkAdapterQueryName(context))
                    {
                        context->SysinfoSection->Name = context->AdapterName->sr;
                    }
                }
            }
            else if (GetIfEntry2_I)
            {
                MIB_IF_ROW2 interfaceRow;

                interfaceRow = QueryInterfaceRowVista(context->AdapterEntry);

                networkInOctets = interfaceRow.InOctets;
                networkOutOctets = interfaceRow.OutOctets;
                networkRcvSpeed = networkInOctets - context->LastInboundValue;
                networkXmitSpeed = networkOutOctets - context->LastOutboundValue;
                //networkLinkSpeed = interfaceRow.TransmitLinkSpeed; // interfaceRow.ReceiveLinkSpeed

                // HACK: Pull the Adapter name from the current query.
                if (context->SysinfoSection->Name.Length == 0)
                {
                    if (context->AdapterName = PhCreateString(interfaceRow.Description))
                    {
                        context->SysinfoSection->Name = context->AdapterName->sr;
                    }
                }
            }
            else
            {
                MIB_IFROW interfaceRow;

                interfaceRow = QueryInterfaceRowXP(context->AdapterEntry);

                networkInOctets = interfaceRow.dwInOctets;
                networkOutOctets = interfaceRow.dwOutOctets;
                networkRcvSpeed = networkInOctets - context->LastInboundValue;
                networkXmitSpeed = networkOutOctets - context->LastOutboundValue;
                //networkLinkSpeed = interfaceRow.dwSpeed;

                // HACK: Pull the Adapter name from the current query.
                if (context->SysinfoSection->Name.Length == 0)
                {
                    if (context->AdapterName = PhConvertMultiByteToUtf16(interfaceRow.bDescr))
                    {
                        context->SysinfoSection->Name = context->AdapterName->sr;
                    }
                }
            }

            if (!context->HaveFirstSample)
            {
                networkRcvSpeed = 0;
                networkXmitSpeed = 0;
                context->HaveFirstSample = TRUE;
            }

            PhAddItemCircularBuffer_ULONG64(&context->InboundBuffer, networkRcvSpeed);
            PhAddItemCircularBuffer_ULONG64(&context->OutboundBuffer, networkXmitSpeed);

            //context->LinkSpeed = networkLinkSpeed;
            context->InboundValue = networkRcvSpeed;
            context->OutboundValue = networkXmitSpeed;
            context->LastInboundValue = networkInOctets;
            context->LastOutboundValue = networkOutOctets;
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

            drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, context->InboundBuffer.Count);

            if (!Section->GraphState.Valid)
            {
                FLOAT max = 0;

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    Section->GraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->InboundBuffer, i);
                    Section->GraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->OutboundBuffer, i);

                    if (max < data1 + data2)
                        max = data1 + data2;
                }

                // Minimum scaling of 1 MB.
                //if (max < 1024 * 1024)
                //    max = 1024 * 1024;

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
                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = (PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT)Parameter1;

            ULONG64 adapterInboundValue = PhGetItemCircularBuffer_ULONG64(
                &context->InboundBuffer,
                getTooltipText->Index
                );

            ULONG64 adapterOutboundValue = PhGetItemCircularBuffer_ULONG64(
                &context->OutboundBuffer,
                getTooltipText->Index
                );

            PhMoveReference(&Section->GraphState.TooltipText, PhFormatString(
                L"R: %s\nS: %s\n%s",
                PhaFormatSize(adapterInboundValue, -1)->Buffer,
                PhaFormatSize(adapterOutboundValue, -1)->Buffer,
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
                L"R: %s\nS: %s",
                PhaFormatSize(context->InboundValue, -1)->Buffer,
                PhaFormatSize(context->OutboundValue, -1)->Buffer
                );
        }
        return TRUE;
    }

    return FALSE;
}

VOID NetAdapterSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ PPH_NETADAPTER_ENTRY AdapterEntry
    )
{
    PH_SYSINFO_SECTION section;
    PPH_NETADAPTER_SYSINFO_CONTEXT context;

    context = (PPH_NETADAPTER_SYSINFO_CONTEXT)PhAllocate(sizeof(PH_NETADAPTER_SYSINFO_CONTEXT));
    memset(context, 0, sizeof(PH_NETADAPTER_SYSINFO_CONTEXT));
    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));

    context->AdapterEntry = AdapterEntry;

    section.Context = context;
    section.Callback = NetAdapterSectionCallback;

    PhInitializeStringRef(&section.Name, L"");

    context->SysinfoSection = Pointers->CreateSection(&section);
}