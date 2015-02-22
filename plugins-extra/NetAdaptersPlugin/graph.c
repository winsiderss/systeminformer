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

#define MSG_UPDATE (WM_APP + 1)
#define MSG_UPDATE_PANEL (WM_APP + 2)

static _GetIfEntry2 GetIfEntry2_I = NULL;

static MIB_IF_ROW2 QueryInterfaceRowVista(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{    
    MIB_IF_ROW2 interfaceRow;

    memset(&interfaceRow, 0, sizeof(MIB_IF_ROW2));

    interfaceRow.InterfaceLuid.Value = Context->AdapterEntry->Luid64;     

    if (GetIfEntry2_I)
    {
        GetIfEntry2_I(&interfaceRow);
    }

    return interfaceRow;
}

static MIB_IFROW QueryInterfaceRowXP(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{    
    MIB_IFROW interfaceRow;

    memset(&interfaceRow, 0, sizeof(MIB_IFROW));

    interfaceRow.dwIndex = Context->AdapterEntry->IfIndex;

    GetIfEntry(&interfaceRow);

    return interfaceRow;
}

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
    //if (context->PanelWindowHandle)
    //    PostMessage(context->PanelWindowHandle, MSG_UPDATE_PANEL, 0, 0);
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
    if (WindowsVersion >= WINDOWS_VISTA)
    {
        MIB_IF_ROW2 interfaceRow;

        interfaceRow = QueryInterfaceRowVista(Context);

        SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_SPEED, PhaFormatSize(interfaceRow.ReceiveLinkSpeed, -1)->Buffer);

        if (interfaceRow.MediaConnectState == MediaConnectStateConnected)
        {
            SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Connected");
        }
        else
        {
            SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Disconnected");
        }

        SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BSENT, PhaFormatSize(interfaceRow.OutOctets, -1)->Buffer);
        SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BRECIEVED, PhaFormatSize(interfaceRow.InOctets, -1)->Buffer);
        SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BTOTAL, PhaFormatSize(interfaceRow.OutOctets + interfaceRow.InOctets, -1)->Buffer);
    }
    else
    {
        MIB_IFROW interfaceRow;

        interfaceRow = QueryInterfaceRowXP(Context);

        SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_SPEED, PhaFormatSize(interfaceRow.dwSpeed, -1)->Buffer);

        if (interfaceRow.dwOperStatus == IF_OPER_STATUS_OPERATIONAL)
        {
            SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Connected");
        }
        else
        {
            SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Disconnected");
        }

        SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BSENT, PhaFormatSize(interfaceRow.dwOutOctets, -1)->Buffer);
        SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BRECIEVED, PhaFormatSize(interfaceRow.dwInOctets, -1)->Buffer);
        SetDlgItemText(Context->PanelWindowHandle, IDC_STAT_BTOTAL, PhaFormatSize(interfaceRow.dwOutOctets + interfaceRow.dwInOctets, -1)->Buffer);
    }
}

static INT_PTR CALLBACK NetAdapterPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
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

            context->PanelWindowHandle = CreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_NETADAPTER_PANEL), hwndDlg, NetAdapterPanelDialogProc);
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
                PluginInstance->DllBase,
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
                        context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), PhGetIntegerSetting(L"ColorCpuUser"));

                        PhGraphStateGetDrawInfo(
                            &context->GraphState,
                            getDrawInfo,
                            context->InboundBuffer.Count
                            );

                        if (!context->GraphState.Valid)
                        {
                            FLOAT maxGraphHeight1 = 0;
                            FLOAT maxGraphHeight2 = 0;

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                context->GraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->InboundBuffer, i);
                                context->GraphState.Data2[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->OutboundBuffer, i);

                                if (context->GraphState.Data1[i] > maxGraphHeight1)
                                    maxGraphHeight1 = context->GraphState.Data1[i];

                                if (context->GraphState.Data2[i] > maxGraphHeight2)
                                    maxGraphHeight2 = context->GraphState.Data2[i];
                            }

                            // Scale the data.
                            PhxfDivideSingle2U(
                                context->GraphState.Data1,
                                maxGraphHeight1,
                                drawInfo->LineDataCount
                                );

                            // Scale the data.
                            PhxfDivideSingle2U(
                                context->GraphState.Data2,
                                maxGraphHeight2,
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

                                PhSwapReference2(&context->GraphState.TooltipText, PhFormatString(
                                    L"R: %s\nS: %s",
                                    PhaFormatSize(adapterInboundValue, -1)->Buffer,
                                    PhaFormatSize(adapterOutboundValue, -1)->Buffer
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
            if (WindowsVersion > WINDOWS_VISTA)
            {
                if ((context->IphlpHandle = LoadLibrary(L"Iphlpapi.dll")))
                    GetIfEntry2_I = (_GetIfEntry2)GetProcAddress(context->IphlpHandle, "GetIfEntry2");
            }

            PhInitializeCircularBuffer_ULONG64(&context->InboundBuffer, PhGetIntegerSetting(L"SampleCount"));
            PhInitializeCircularBuffer_ULONG64(&context->OutboundBuffer, PhGetIntegerSetting(L"SampleCount"));
        }
        return TRUE;
    case SysInfoDestroy:
        {
            if (context->AdapterName)
                PhDereferenceObject(context->AdapterName);

            PhDeleteCircularBuffer_ULONG64(&context->InboundBuffer);
            PhDeleteCircularBuffer_ULONG64(&context->OutboundBuffer);

            if (context->IphlpHandle)
                FreeLibrary(context->IphlpHandle);

            PhFree(context);
        }
        return TRUE;
    case SysInfoTick:
        {
            if (WindowsVersion >= WINDOWS_VISTA)
            {
                MIB_IF_ROW2 interfaceRow;
                ULONG64 networkInboundSpeed;
                ULONG64 networkOutboundSpeed;

                interfaceRow = QueryInterfaceRowVista(context);

                networkInboundSpeed = interfaceRow.InOctets - context->LastInboundValue;
                networkOutboundSpeed = interfaceRow.OutOctets - context->LastOutboundValue;

                // HACK: Pull the Adapter name from the current query persisted adapter name might change.
                if (context->SysinfoSection->Name.Length == 0)
                {
                    context->AdapterName = PhCreateString(interfaceRow.Description);
                    context->SysinfoSection->Name = context->AdapterName->sr;
                }

                if (!context->HaveFirstSample)
                {
                    networkInboundSpeed = 0;
                    networkOutboundSpeed = 0;
                    context->HaveFirstSample = TRUE;
                }

                PhAddItemCircularBuffer_ULONG64(&context->InboundBuffer, networkInboundSpeed);
                PhAddItemCircularBuffer_ULONG64(&context->OutboundBuffer, networkOutboundSpeed);

                context->InboundValue = networkInboundSpeed;
                context->OutboundValue = networkOutboundSpeed;
                context->LastInboundValue = interfaceRow.InOctets;
                context->LastOutboundValue = interfaceRow.OutOctets;
                context->MaxSendSpeed = interfaceRow.TransmitLinkSpeed;
                context->MaxReceiveSpeed = interfaceRow.ReceiveLinkSpeed;
            }
            else
            {
                MIB_IFROW interfaceRow;
                ULONG64 networkInboundSpeed;
                ULONG64 networkOutboundSpeed;

                interfaceRow = QueryInterfaceRowXP(context);

                networkInboundSpeed = interfaceRow.dwInOctets - context->LastInboundValue;
                networkOutboundSpeed = interfaceRow.dwOutOctets - context->LastOutboundValue;

                // HACK: Pull the Adapter name from the current query since persisted adapter name might change.
                if (context->SysinfoSection->Name.Length == 0)
                {
                    context->AdapterName = PhCreateStringFromAnsi(interfaceRow.bDescr);
                    context->SysinfoSection->Name = context->AdapterName->sr;
                }

                if (!context->HaveFirstSample)
                {
                    networkInboundSpeed = 0;
                    networkOutboundSpeed = 0;
                    context->HaveFirstSample = TRUE;
                }

                PhAddItemCircularBuffer_ULONG64(&context->InboundBuffer, networkInboundSpeed);
                PhAddItemCircularBuffer_ULONG64(&context->OutboundBuffer, networkOutboundSpeed);

                context->InboundValue = networkInboundSpeed;
                context->OutboundValue = networkOutboundSpeed;
                context->LastInboundValue = interfaceRow.dwInOctets;
                context->LastOutboundValue = interfaceRow.dwOutOctets;
                context->MaxSendSpeed = interfaceRow.dwSpeed; // TODO: Use the value we get from GetAdaptersAddresses 
                context->MaxReceiveSpeed = interfaceRow.dwSpeed; // TODO: Use the value we get from GetAdaptersAddresses 

            }
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
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), PhGetIntegerSetting(L"ColorCpuUser"));
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, context->InboundBuffer.Count);

            if (!Section->GraphState.Valid)
            {
                FLOAT maxGraphHeight1 = 0;
                FLOAT maxGraphHeight2 = 0;

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    Section->GraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->InboundBuffer, i);
                    Section->GraphState.Data2[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->OutboundBuffer, i);

                    if (Section->GraphState.Data1[i] > maxGraphHeight1)
                        maxGraphHeight1 = Section->GraphState.Data1[i];
                    if (Section->GraphState.Data2[i] > maxGraphHeight2)
                        maxGraphHeight2 = Section->GraphState.Data2[i];
                }

                // Scale the data.
                PhxfDivideSingle2U(
                    Section->GraphState.Data1,
                    maxGraphHeight1, // (FLOAT)context->MaxReceiveSpeed,
                    drawInfo->LineDataCount
                    );

                // Scale the data.
                PhxfDivideSingle2U(
                    Section->GraphState.Data2,
                    maxGraphHeight2, // (FLOAT)context->MaxSendSpeed,
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

            PhSwapReference2(&Section->GraphState.TooltipText, PhFormatString(
                L"R: %s\nS: %s",
                PhaFormatSize(adapterInboundValue, -1)->Buffer,
                PhaFormatSize(adapterOutboundValue, -1)->Buffer
                ));

            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = (PPH_SYSINFO_DRAW_PANEL)Parameter1;

            PhSwapReference2(&drawPanel->Title, PhCreateString(Section->Name.Buffer));
            PhSwapReference2(&drawPanel->SubTitle, PhFormatString(
                L"R: %s\nS: %s",
                PhaFormatSize(context->InboundValue, -1)->Buffer,
                PhaFormatSize(context->OutboundValue, -1)->Buffer
                ));
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