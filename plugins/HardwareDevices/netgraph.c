/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2015-2019 dmex
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
    Context->GraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->GraphHandle, 1);
    Graph_Draw(Context->GraphHandle);
    Graph_UpdateTooltip(Context->GraphHandle);
    InvalidateRect(Context->GraphHandle, NULL, FALSE);
}

VOID NetAdapterUpdatePanel(
    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    ULONG64 inOctetsValue = 0;
    ULONG64 outOctetsValue = 0;
    ULONG64 linkSpeedValue = 0;
    NDIS_MEDIA_CONNECT_STATE mediaState = MediaConnectStateUnknown;
    HANDLE deviceHandle = NULL;

    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_NDIS))
    {
        if (NT_SUCCESS(PhCreateFileWin32(
            &deviceHandle,
            PhGetString(Context->AdapterEntry->AdapterId.InterfaceDevice),
            FILE_GENERIC_READ,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
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
                // Close the handle and fallback to GetIfEntry.
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
                inOctetsValue = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_RCV);
            else
                inOctetsValue = interfaceStats.ifHCInOctets;

            if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT))
                outOctetsValue = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_XMIT);
            else
                outOctetsValue = interfaceStats.ifHCOutOctets;
        }
        else
        {
            // https://msdn.microsoft.com/en-us/library/ff569443.aspx
            // https://msdn.microsoft.com/en-us/library/ff569445.aspx
            inOctetsValue = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_RCV);
            outOctetsValue = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_XMIT);
        }

        if (NT_SUCCESS(NetworkAdapterQueryLinkState(deviceHandle, &interfaceState)))
        {
            mediaState = interfaceState.MediaConnectState;
            linkSpeedValue = interfaceState.XmitLinkSpeed;
        }
        else
        {
            NetworkAdapterQueryLinkSpeed(deviceHandle, &linkSpeedValue);
        }

        NtClose(deviceHandle);
    }
    else
    {
        MIB_IF_ROW2 interfaceRow;

        if (QueryInterfaceRow(&Context->AdapterEntry->AdapterId, &interfaceRow))
        {
            inOctetsValue = interfaceRow.InOctets;
            outOctetsValue = interfaceRow.OutOctets;
            mediaState = interfaceRow.MediaConnectState;
            linkSpeedValue = interfaceRow.TransmitLinkSpeed;
        }
    }

    if (Context->NetworkReceiveRaw < inOctetsValue)
        Context->NetworkReceiveRaw = inOctetsValue;
    if (Context->NetworkSendRaw < outOctetsValue)
        Context->NetworkSendRaw = outOctetsValue;

    PhUpdateDelta(&Context->NetworkSendDelta, Context->NetworkSendRaw);
    PhUpdateDelta(&Context->NetworkReceiveDelta, Context->NetworkReceiveRaw);

    if (!Context->HaveFirstSample)
    {
        Context->NetworkSendDelta.Delta = 0;
        Context->NetworkReceiveDelta.Delta = 0;
        Context->HaveFirstSample = TRUE;
    }

    Context->CurrentNetworkSend = Context->NetworkSendDelta.Delta;
    Context->CurrentNetworkReceive = Context->NetworkReceiveDelta.Delta;

    PhSetDialogItemText(Context->PanelWindowHandle, IDC_STAT_BSENT, PhaFormatSize(outOctetsValue, ULONG_MAX)->Buffer);
    PhSetDialogItemText(Context->PanelWindowHandle, IDC_STAT_BRECEIVED, PhaFormatSize(inOctetsValue, ULONG_MAX)->Buffer);
    PhSetDialogItemText(Context->PanelWindowHandle, IDC_STAT_BTOTAL, PhaFormatSize(inOctetsValue + outOctetsValue, ULONG_MAX)->Buffer);

    if (mediaState == MediaConnectStateConnected)
    {
        PhSetDialogItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Connected");
        PhSetDialogItemText(Context->PanelWindowHandle, IDC_LINK_SPEED, PhaConcatStrings2(
            PhaFormatSize(linkSpeedValue / BITS_IN_ONE_BYTE, ULONG_MAX)->Buffer,
            L"/s"
            //linkSpeedValue / 1000000.0   L"%.1f Mbps",
            )->Buffer);
    }
    else
    {
        PhSetDialogItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Disconnected");
        PhSetDialogItemText(Context->PanelWindowHandle, IDC_LINK_SPEED, L"N/A");
    }

    PhSetDialogItemText(Context->PanelWindowHandle, IDC_STAT_QUEUELENGTH, PhaFormatString(
        L"%s/s",
        PhaFormatSize(Context->CurrentNetworkReceive + Context->CurrentNetworkSend, ULONG_MAX)->Buffer)->Buffer
        );
}

VOID UpdateNetAdapterDialog(
    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    MIB_IF_ROW2 interfaceRow;

    if (Context->AdapterEntry->AdapterName)
        PhSetDialogItemText(Context->WindowHandle, IDC_ADAPTERNAME, Context->AdapterEntry->AdapterName->Buffer);
    else
        PhSetDialogItemText(Context->WindowHandle, IDC_ADAPTERNAME, L"Unknown network adapter");

    if (QueryInterfaceRow(&Context->AdapterEntry->AdapterId, &interfaceRow))
    {
        PhSetDialogItemText(Context->WindowHandle, IDC_ADAPTERTEXT, interfaceRow.Alias);
    }
    else
    {
        PhSetDialogItemText(Context->WindowHandle, IDC_ADAPTERTEXT, L"");
    }

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

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_NCDESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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
            MIB_IF_ROW2 interfaceRow;

            context->WindowHandle = hwndDlg;

            PhInitializeGraphState(&context->GraphState);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ADAPTERNAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SetWindowFont(GetDlgItem(hwndDlg, IDC_ADAPTERTEXT), context->SysinfoSection->Parameters->LargeFont, FALSE);
            SetWindowFont(GetDlgItem(hwndDlg, IDC_ADAPTERNAME), context->SysinfoSection->Parameters->MediumFont, FALSE);

            if (QueryInterfaceRow(&context->AdapterEntry->AdapterId, &interfaceRow))
            {
                PhSetDialogItemText(hwndDlg, IDC_ADAPTERTEXT, interfaceRow.Alias);
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_ADAPTERTEXT, L"");
            }

            //SetWindowFont(GetDlgItem(hwndDlg, IDC_ADAPTERNAME), context->SysinfoSection->Parameters->LargeFont, FALSE);
            PhSetDialogItemText(hwndDlg, IDC_ADAPTERNAME, PhGetStringOrDefault(context->AdapterEntry->AdapterName, L"Unknown network adapter"));

            context->PanelWindowHandle = CreateDialogParam(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_NETADAPTER_PANEL), hwndDlg, NetAdapterPanelDialogProc, (LPARAM)context);
            ShowWindow(context->PanelWindowHandle, SW_SHOW);
            PhAddLayoutItemEx(&context->LayoutManager, context->PanelWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

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
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
            PhDeleteGraphState(&context->GraphState);

            if (context->GraphHandle)
                DestroyWindow(context->GraphHandle);

            if (context->PanelWindowHandle)
                DestroyWindow(context->PanelWindowHandle);
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
                                    PhaFormatSize(adapterInboundValue, ULONG_MAX)->Buffer,
                                    PhaFormatSize(adapterOutboundValue, ULONG_MAX)->Buffer,
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

            if (!createDialog)
                break;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_NETADAPTER_DIALOG);
            createDialog->DialogProc = NetAdapterDialogProc;
            createDialog->Parameter = context;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)Parameter1;

            if (!drawInfo)
                break;

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
            ULONG64 adapterInboundValue;
            ULONG64 adapterOutboundValue;

            if (!getTooltipText)
                break;

            adapterInboundValue = PhGetItemCircularBuffer_ULONG64(
                &context->AdapterEntry->InboundBuffer,
                getTooltipText->Index
                );

            adapterOutboundValue = PhGetItemCircularBuffer_ULONG64(
                &context->AdapterEntry->OutboundBuffer,
                getTooltipText->Index
                );

            PhMoveReference(&Section->GraphState.TooltipText, PhFormatString(
                L"R: %s\nS: %s\n%s",
                PhaFormatSize(adapterInboundValue, ULONG_MAX)->Buffer,
                PhaFormatSize(adapterOutboundValue, ULONG_MAX)->Buffer,
                ((PPH_STRING)PH_AUTO(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));

            getTooltipText->Text = PhGetStringRef(Section->GraphState.TooltipText);
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = (PPH_SYSINFO_DRAW_PANEL)Parameter1;

            if (!drawPanel)
                break;

            PhSetReference(&drawPanel->Title, context->AdapterEntry->AdapterName);
            drawPanel->SubTitle = PhFormatString(
                L"R: %s\nS: %s",
                PhaFormatSize(context->AdapterEntry->CurrentNetworkReceive, ULONG_MAX)->Buffer,
                PhaFormatSize(context->AdapterEntry->CurrentNetworkSend, ULONG_MAX)->Buffer
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

    context = PhAllocateZero(sizeof(DV_NETADAPTER_SYSINFO_CONTEXT));
    context->AdapterEntry = AdapterEntry;
    context->SectionName = PhConcatStrings2(L"NetAdapter ", PhGetStringOrEmpty(AdapterEntry->AdapterId.InterfaceGuid));
    
    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    section.Context = context;
    section.Callback = NetAdapterSectionCallback;
    section.Name = PhGetStringRef(context->SectionName);

    context->SysinfoSection = Pointers->CreateSection(&section);
}
