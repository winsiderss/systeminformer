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

#define BITS_IN_ONE_BYTE 8
#define NDIS_UNIT_OF_MEASUREMENT 100
#define MSG_UPDATE (WM_APP + 1)
#define MSG_UPDATE_PANEL (WM_APP + 2)

static BOOLEAN NetworkAdapterQuerySupported(
    _In_ HANDLE DeviceHandle
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    BOOLEAN ndisQuerySupported = FALSE;
    BOOLEAN adapterNameSupported = FALSE;
    BOOLEAN adapterStatsSupported = FALSE;
    BOOLEAN adapterLinkStateSupported = FALSE;
    BOOLEAN adapterLinkSpeedSupported = FALSE;
    PNDIS_OID ndisObjectIdentifiers = NULL;

    // https://msdn.microsoft.com/en-us/library/ff569642.aspx
    opcode = OID_GEN_SUPPORTED_LIST;
    //opcode = OID_GEN_CO_SUPPORTED_LIST;

    // TODO: 4096 objects might be too small...
    ndisObjectIdentifiers = PhAllocate(sizeof(NDIS_OID) * PAGE_SIZE);
    memset(ndisObjectIdentifiers, 0, sizeof(NDIS_OID) * PAGE_SIZE);

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        ndisObjectIdentifiers,
        sizeof(NDIS_OID) * PAGE_SIZE
        )))
    {
        ndisQuerySupported = TRUE;

        for (ULONG i = 0; i < (ULONG)isb.Information; i++)
        {
            NDIS_OID opcode = ndisObjectIdentifiers[i];

            switch (opcode)
            {
            case OID_GEN_FRIENDLY_NAME:
                adapterNameSupported = TRUE;
                break;
            case OID_GEN_STATISTICS:
                adapterStatsSupported = TRUE;
                break;
            case OID_GEN_LINK_STATE:
                adapterLinkStateSupported = TRUE;
                break;
            case OID_GEN_LINK_SPEED:
                adapterLinkSpeedSupported = TRUE;
                break;
            }
        }
    }

    PhFree(ndisObjectIdentifiers);

    if (!adapterNameSupported)
        ndisQuerySupported = FALSE;
    if (!adapterStatsSupported)
        ndisQuerySupported = FALSE;
    if (!adapterLinkStateSupported)
        ndisQuerySupported = FALSE;
    if (!adapterLinkSpeedSupported)
        ndisQuerySupported = FALSE;

    return ndisQuerySupported;
}

static BOOLEAN NetworkAdapterQueryNdisVersion(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    ULONG versionResult = 0;

    // https://msdn.microsoft.com/en-us/library/ff569582.aspx
    opcode = OID_GEN_DRIVER_VERSION;

    if (NT_SUCCESS(NtDeviceIoControlFile(
        Context->DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &versionResult,
        sizeof(versionResult)
        )))
    {
        Context->NdisMajorVersion = HIBYTE(versionResult);
        Context->NdisMinorVersion = LOBYTE(versionResult);

        return TRUE;
    }

    return FALSE;
}

static PPH_STRING NetworkAdapterQueryName(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    WCHAR adapterNameBuffer[MAX_PATH] = L"";

    // https://msdn.microsoft.com/en-us/library/ff569584.aspx
    opcode = OID_GEN_FRIENDLY_NAME;

    if (NT_SUCCESS(NtDeviceIoControlFile(
        Context->DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        adapterNameBuffer,
        sizeof(adapterNameBuffer)
        )))
    {
        return PhCreateString(adapterNameBuffer);
    }

    // HACK: Query adapter description using undocumented function.
    if (Context->GetInterfaceDescriptionFromGuid_I)
    {
        GUID deviceGuid = GUID_NULL;
        UNICODE_STRING guidStringUs;

        PhStringRefToUnicodeString(&Context->AdapterEntry->InterfaceGuid->sr, &guidStringUs);

        if (NT_SUCCESS(RtlGUIDFromString(&guidStringUs, &deviceGuid)))
        {
            SIZE_T adapterDescriptionLength = 0;
            WCHAR adapterDescription[NDIS_IF_MAX_STRING_SIZE + 1] = L"";

            adapterDescriptionLength = sizeof(adapterDescription);

            if (SUCCEEDED(Context->GetInterfaceDescriptionFromGuid_I(&deviceGuid, adapterDescription, &adapterDescriptionLength, NULL, NULL)))
            {
                return PhCreateString(adapterDescription);
            }
        }
    }

    return PhCreateString(L"Unknown");
}

static NTSTATUS NetworkAdapterQueryStatistics(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_STATISTICS_INFO Info
    )
{
    NTSTATUS status;
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    NDIS_STATISTICS_INFO result;

    // https://msdn.microsoft.com/en-us/library/ff569640.aspx
    opcode = OID_GEN_STATISTICS;

    memset(&result, 0, sizeof(NDIS_STATISTICS_INFO));
    result.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    result.Header.Revision = NDIS_STATISTICS_INFO_REVISION_1;
    result.Header.Size = NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;

    status = NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &result,
        sizeof(result)
        );

    *Info = result;

    //if (isb.Information != NDIS_SIZEOF_STATISTICS_INFO_REVISION_1)
    //    return STATUS_INFO_LENGTH_MISMATCH;
    
    return status;
}

static NTSTATUS NetworkAdapterQueryLinkState(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_LINK_STATE State
    )
{
    NTSTATUS status;
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    NDIS_LINK_STATE result;

    // https://msdn.microsoft.com/en-us/library/ff569595.aspx
    opcode = OID_GEN_LINK_STATE;
    //opcode = OID_GEN_MEDIA_CONNECT_STATUS;

    memset(&result, 0, sizeof(NDIS_LINK_STATE));
    result.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    result.Header.Revision = NDIS_LINK_STATE_REVISION_1;
    result.Header.Size = NDIS_SIZEOF_LINK_STATE_REVISION_1;

    status = NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &result,
        sizeof(result)
        );

    *State = result;

    //if (isb.Information != NDIS_SIZEOF_LINK_STATE_REVISION_1)
    //    return STATUS_INFO_LENGTH_MISMATCH;

    return status;
}

static BOOLEAN NetworkAdapterQueryMediaType(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    NDIS_PHYSICAL_MEDIUM adapterMediaType = NdisPhysicalMediumUnspecified;

    opcode = OID_GEN_PHYSICAL_MEDIUM_EX;

    if (NT_SUCCESS(NtDeviceIoControlFile(
        Context->DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &adapterMediaType,
        sizeof(adapterMediaType)
        )))
    {
        Context->NdisAdapterType = adapterMediaType;
    }

    if (Context->NdisAdapterType != NdisPhysicalMediumUnspecified)
        return TRUE;

    opcode = OID_GEN_PHYSICAL_MEDIUM;
    memset(&isb, 0, sizeof(IO_STATUS_BLOCK));

    if (NT_SUCCESS(NtDeviceIoControlFile(
        Context->DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &adapterMediaType,
        sizeof(adapterMediaType)
        )))
    {
        Context->NdisAdapterType = adapterMediaType;
    }
       
    if (Context->NdisAdapterType != NdisPhysicalMediumUnspecified)
        return TRUE;

    //NDIS_MEDIUM adapterMediaType = NdisMediumMax;
    //opcode = OID_GEN_MEDIA_IN_USE;

    return FALSE;
}

static PPH_STRING NetworkAdapterQueryLinkSpeed(
    _In_ HANDLE DeviceHandle
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    NDIS_CO_LINK_SPEED result;

    // https://msdn.microsoft.com/en-us/library/ff569593.aspx
    opcode = OID_GEN_LINK_SPEED; // OID_GEN_CO_LINK_SPEED

    memset(&result, 0, sizeof(NDIS_CO_LINK_SPEED));

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &result,
        sizeof(result)
        )))
    {
        return PhFormatSize(UInt32x32To64(result.Outbound, NDIS_UNIT_OF_MEASUREMENT) / BITS_IN_ONE_BYTE, -1);
    }

    return PhReferenceEmptyString();
}

static ULONG64 NetworkAdapterQueryValue(
    _In_ HANDLE DeviceHandle,
    _In_ NDIS_OID OpCode
    )
{
    IO_STATUS_BLOCK isb;
    ULONG64 result = 0;

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &OpCode,
        sizeof(NDIS_OID),
        &result,
        sizeof(result)
        )))
    {
        return result;
    }

    return 0;
}



static MIB_IF_ROW2 QueryInterfaceRowVista(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{    
    MIB_IF_ROW2 interfaceRow;

    memset(&interfaceRow, 0, sizeof(MIB_IF_ROW2));

    interfaceRow.InterfaceLuid = Context->AdapterEntry->InterfaceLuid;
    //interfaceRow.InterfaceIndex = Context->AdapterEntry->InterfaceIndex;

    if (Context->GetIfEntry2_I)
    {
        Context->GetIfEntry2_I(&interfaceRow);
    }

    //MIB_IPINTERFACE_ROW table;
    //memset(&table, 0, sizeof(MIB_IPINTERFACE_ROW));
    //table.Family = AF_INET;
    //table.InterfaceLuid.Value = Context->AdapterEntry->InterfaceLuidValue;
    //table.InterfaceIndex = Context->AdapterEntry->InterfaceIndex;
    //GetIpInterfaceEntry(&table);

    return interfaceRow;
}

static MIB_IFROW QueryInterfaceRowXP(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{    
    MIB_IFROW interfaceRow;

    memset(&interfaceRow, 0, sizeof(MIB_IFROW));

    interfaceRow.dwIndex = Context->AdapterEntry->InterfaceIndex;

    GetIfEntry(&interfaceRow);

    //MIB_IPINTERFACE_ROW table;
    //memset(&table, 0, sizeof(MIB_IPINTERFACE_ROW));
    //table.Family = AF_INET;
    //table.InterfaceIndex = Context->AdapterEntry->InterfaceIndex;
    //GetIpInterfaceEntry(&table);

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
    ULONG64 inOctets = 0;
    ULONG64 outOctets = 0;
    ULONG64 xmitLinkSpeed = 0;
    NDIS_MEDIA_CONNECT_STATE mediaState = MediaConnectStateUnknown;
    PPH_STRING linkSpeed = NULL;

    if (Context->DeviceHandle)
    {
        NDIS_STATISTICS_INFO interfaceStats;
        NDIS_LINK_STATE interfaceState;

        if (NT_SUCCESS(NetworkAdapterQueryStatistics(Context->DeviceHandle, &interfaceStats)))
        {
            //if (interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV)
            //if (interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT)

            inOctets = interfaceStats.ifHCInOctets;
            outOctets = interfaceStats.ifHCOutOctets;
        }
        else
        {
            // The above code should return statistics however some drivers bypassed Microsoft driver testing requirements...
            //  NDIS handles these two OIDs for all miniport drivers reguardless.
            inOctets = NetworkAdapterQueryValue(Context->DeviceHandle, OID_GEN_BYTES_RCV); // OID_GEN_RCV_OK
            outOctets = NetworkAdapterQueryValue(Context->DeviceHandle, OID_GEN_BYTES_XMIT); // OID_GEN_XMIT_OK

            //NetworkAdapterQueryValue(Context->DeviceHandle, OID_GEN_RCV_ERROR);
            //NetworkAdapterQueryValue(Context->DeviceHandle, OID_GEN_XMIT_ERROR);
        }

        if (NT_SUCCESS(NetworkAdapterQueryLinkState(Context->DeviceHandle, &interfaceState)))
        {
            mediaState = interfaceState.MediaConnectState;
            xmitLinkSpeed = interfaceState.XmitLinkSpeed;
        }

        linkSpeed = NetworkAdapterQueryLinkSpeed(Context->DeviceHandle);
    }
    else if (Context->GetIfEntry2_I)
    {
        MIB_IF_ROW2 interfaceRow;

        interfaceRow = QueryInterfaceRowVista(Context);

        inOctets = interfaceRow.InOctets;
        outOctets = interfaceRow.OutOctets;
        mediaState = interfaceRow.MediaConnectState;
        xmitLinkSpeed = interfaceRow.TransmitLinkSpeed;
    }
    else
    {
        MIB_IFROW interfaceRow;

        interfaceRow = QueryInterfaceRowXP(Context);

        inOctets = interfaceRow.dwInOctets;
        outOctets = interfaceRow.dwOutOctets;
        xmitLinkSpeed = interfaceRow.dwSpeed;

        if (interfaceRow.dwOperStatus == IF_OPER_STATUS_OPERATIONAL)
            mediaState = MediaConnectStateConnected;
        else
            mediaState = MediaConnectStateDisconnected;
    }

    
    if (linkSpeed)
    {
        SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_SPEED, linkSpeed->Buffer);
        PhDereferenceObject(linkSpeed);
    }
    else
    {
        SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_SPEED, PhaFormatSize(xmitLinkSpeed / BITS_IN_ONE_BYTE, -1)->Buffer);
    }

    if (mediaState == MediaConnectStateConnected)
        SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Connected");
    else
        SetDlgItemText(Context->PanelWindowHandle, IDC_LINK_STATE, L"Disconnected");

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
                                    L"R: %s\nS: %s\n%s",
                                    PhaFormatSize(adapterInboundValue, -1)->Buffer,
                                    PhaFormatSize(adapterOutboundValue, -1)->Buffer,
                                    ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
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
                    if (!NetworkAdapterQuerySupported(context->DeviceHandle))
                    {       
                        NtClose(context->DeviceHandle);
                        context->DeviceHandle = NULL;
                    }
                }
            }

            if (WindowsVersion > WINDOWS_VISTA)
            {
                if ((context->IphlpHandle = LoadLibrary(L"iphlpapi.dll")))
                {
                    context->GetIfEntry2_I = (_GetIfEntry2)GetProcAddress(context->IphlpHandle, "GetIfEntry2");
                    context->GetInterfaceDescriptionFromGuid_I = (_GetInterfaceDescriptionFromGuid)GetProcAddress(context->IphlpHandle, "NhGetInterfaceDescriptionFromGuid");
                }
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

            if (context->DeviceHandle)
                NtClose(context->DeviceHandle);

            PhFree(context);
        }
        return TRUE;
    case SysInfoTick:
        {
            ULONG64 networkInboundSpeed = 0;
            ULONG64 networkOutboundSpeed = 0;
            ULONG64 networkInOctets = 0;
            ULONG64 networkOutOctets = 0;
            ULONG64 xmitLinkSpeed = 0;
            ULONG64 rcvLinkSpeed = 0;

            if (context->DeviceHandle)
            {
                NDIS_STATISTICS_INFO interfaceStats;
                NDIS_LINK_STATE interfaceState;

                if (NT_SUCCESS(NetworkAdapterQueryStatistics(context->DeviceHandle, &interfaceStats)))
                {
                    networkInboundSpeed = interfaceStats.ifHCInOctets - context->LastInboundValue;
                    networkOutboundSpeed = interfaceStats.ifHCOutOctets - context->LastOutboundValue;
                    networkInOctets = interfaceStats.ifHCInOctets;
                    networkOutOctets = interfaceStats.ifHCOutOctets;
                }
                else
                {
                    ULONG64 inOctets = NetworkAdapterQueryValue(context->DeviceHandle, OID_GEN_BYTES_RCV); // OID_GEN_RCV_OK
                    ULONG64 outOctets = NetworkAdapterQueryValue(context->DeviceHandle, OID_GEN_BYTES_XMIT); // OID_GEN_XMIT_OK

                    networkInboundSpeed = inOctets - context->LastInboundValue;
                    networkOutboundSpeed = outOctets - context->LastOutboundValue;
                    networkInOctets = inOctets;
                    networkOutOctets = outOctets;
                }

                if (NT_SUCCESS(NetworkAdapterQueryLinkState(context, &interfaceState)))
                {
                    xmitLinkSpeed = interfaceState.XmitLinkSpeed;
                    rcvLinkSpeed = interfaceState.RcvLinkSpeed;
                }

                // HACK: Pull the Adapter name from the current query.
                if (context->SysinfoSection->Name.Length == 0)
                {
                    if (context->AdapterName = NetworkAdapterQueryName(context))
                    {
                        context->SysinfoSection->Name = context->AdapterName->sr;
                    }
                }
            }
            else if (context->GetIfEntry2_I)
            {
                MIB_IF_ROW2 interfaceRow;

                interfaceRow = QueryInterfaceRowVista(context);

                networkInboundSpeed = interfaceRow.InOctets - context->LastInboundValue;
                networkOutboundSpeed = interfaceRow.OutOctets - context->LastOutboundValue;
                networkInOctets = interfaceRow.InOctets;
                networkOutOctets = interfaceRow.OutOctets;
                xmitLinkSpeed = interfaceRow.TransmitLinkSpeed;
                rcvLinkSpeed = interfaceRow.ReceiveLinkSpeed;

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

                interfaceRow = QueryInterfaceRowXP(context);

                networkInboundSpeed = interfaceRow.dwInOctets - context->LastInboundValue;
                networkOutboundSpeed = interfaceRow.dwOutOctets - context->LastOutboundValue;
                networkInOctets = interfaceRow.dwInOctets;
                networkOutOctets = interfaceRow.dwOutOctets;
                xmitLinkSpeed = interfaceRow.dwSpeed;
                rcvLinkSpeed = interfaceRow.dwSpeed;

                // HACK: Pull the Adapter name from the current query.
                if (context->SysinfoSection->Name.Length == 0)
                {
                    if (context->AdapterName = PhCreateStringFromAnsi(interfaceRow.bDescr))
                    {
                        context->SysinfoSection->Name = context->AdapterName->sr;
                    }
                }
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
            context->LastInboundValue = networkInOctets;
            context->LastOutboundValue = networkOutOctets;

            context->MaxSendSpeed = xmitLinkSpeed;
            context->MaxReceiveSpeed = rcvLinkSpeed;
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
                L"R: %s\nS: %s\n%s",
                PhaFormatSize(adapterInboundValue, -1)->Buffer,
                PhaFormatSize(adapterOutboundValue, -1)->Buffer,
                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
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