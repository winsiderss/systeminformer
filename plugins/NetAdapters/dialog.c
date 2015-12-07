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
    PPH_NETADAPTER_DETAILS_CONTEXT context = Context;

    if (context->WindowHandle)
    {
        PostMessage(context->WindowHandle, MSG_UPDATE, 0, 0);
    }
}

static VOID PhAddListViewItemGroupId(
    _In_ HWND ListViewHandle,
    _In_ INT GroupId,
    _In_ INT Index,
    _In_ PWSTR Text
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT | LVIF_GROUPID;
    item.iGroupId = GroupId;
    item.iItem = Index;
    item.pszText = Text;
    item.iSubItem = 0;

    ListView_InsertItem(ListViewHandle, &item);
}

static VOID PhAddListViewGroup(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ PWSTR Text
    )
{
    LVGROUP group;

    group.cbSize = sizeof(LVGROUP);
    group.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_ALIGN | LVGF_STATE;
    group.uAlign = LVGA_HEADER_LEFT;
    group.state = LVGS_COLLAPSIBLE;
    group.iGroupId = Index;
    group.pszHeader = Text;

    ListView_InsertGroup(ListViewHandle, INT_MAX, &group);
}


static VOID AddListViewItemGroups(
    _In_ HWND ListViewHandle
    )
{
    ListView_EnableGroupView(ListViewHandle, TRUE);
    PhAddListViewGroup(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, L"Adapter");
    PhAddListViewGroup(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, L"Unicast");
    PhAddListViewGroup(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, L"Broadcast");
    PhAddListViewGroup(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, L"Multicast");
    PhAddListViewGroup(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, L"Errors");
    
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_STATE, L"State");
    //PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_CONNECTIVITY, L"Connectivity");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_DOMAIN, L"Domain");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_IPADDRESS, L"IP Address");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_SUBNET, L"SubnetMask");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_GATEWAY, L"DefaultGateway");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_LINKSPEED,  L"Link Speed");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_SENT, L"Sent");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_RECEIVED, L"Received");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_TOTAL, L"Total");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_SENDING, L"Sending");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_RECEIVING, L"Receiving");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_UTILIZATION, L"Utilization");

    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_SENTPKTS, L"Sent Packets");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_RECVPKTS, L"Received Packets");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_TOTALPKTS, L"Total Packets");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_SENT, L"Sent");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_RECEIVED, L"Received");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_TOTAL, L"Total");
    //PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_SENDING, L"Sending");
    //PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_RECEIVING, L"Receiving");
    //PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_UTILIZATION, L"Utilization");

    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_SENTPKTS, L"Sent Packets");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_RECVPKTS, L"Received Packets");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_TOTALPKTS, L"Total Packets");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_SENT, L"Sent");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_RECEIVED, L"Received");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_TOTAL, L"Total");

    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_SENTPKTS, L"Sent Packets");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_RECVPKTS, L"Received Packets");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_TOTALPKTS, L"Total Packets");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_SENT, L"Sent");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_RECEIVED, L"Received");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_TOTAL, L"Total");

    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_SENTPKTS, L"Send Errors");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_RECVPKTS, L"Receive Errors");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_TOTALPKTS, L"Total Errors");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_SENT, L"Send Discards");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_RECEIVED, L"Receive Discards");
    PhAddListViewItemGroupId(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_TOTAL, L"Total Discards");
}



static VOID NetAdapterLookupConfig(
    _Inout_ PPH_NETADAPTER_DETAILS_CONTEXT Context
    )
{
    HANDLE keyHandle;
    PPH_STRING keyNameIpV4;
    //PPH_STRING keyNameIpV6;
    
    static PH_STRINGREF tcpIpv4KeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\");
    //static PH_STRINGREF tcpIpv6KeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\Tcpip6\\Parameters\\Interfaces\\");

    keyNameIpV4 = PhConcatStringRef2(&tcpIpv4KeyName, &Context->AdapterEntry->InterfaceGuid->sr);
    //keyNameIpV6 = PhConcatStringRef2(&tcpIpv6KeyName, &Context->AdapterEntry->InterfaceGuid->sr);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyNameIpV4->sr,
        0
        )))
    {

        PPH_STRING domainString = NULL;
        PPH_STRING ipAddressString = NULL;
        PPH_STRING subnetAddressString = NULL;
        PPH_STRING gatewayAddressString = NULL;

        // SeeAlso: https://support.microsoft.com/en-us/kb/314053
        // The IP Helper API sets and queries the IP configuration for the adapter from this registry key.
        // The NDIS API queries the IP configuration for the adapter from this registry key when initializing the netweork device.       

        domainString = PhQueryRegistryString(keyHandle, L"DhcpDomain");
        ipAddressString = PhQueryRegistryString(keyHandle, L"DhcpIPAddress");
        subnetAddressString = PhQueryRegistryString(keyHandle, L"DhcpSubnetMask");
        gatewayAddressString = PhQueryRegistryString(keyHandle, L"DhcpDefaultGateway");

        if (PhIsNullOrEmptyString(domainString))
        {
            if (domainString)
                PhDereferenceObject(domainString);

            domainString = PhQueryRegistryString(keyHandle, L"Domain");
        }

        if (PhIsNullOrEmptyString(ipAddressString))
        {
            if (ipAddressString)
                PhDereferenceObject(ipAddressString);

            ipAddressString = PhQueryRegistryString(keyHandle, L"IPAddress");
        }

        if (PhIsNullOrEmptyString(subnetAddressString))
        {
            if (subnetAddressString)
                PhDereferenceObject(subnetAddressString);

            subnetAddressString = PhQueryRegistryString(keyHandle, L"SubnetMask");
        }

        if (PhIsNullOrEmptyString(gatewayAddressString))
        {
            if (gatewayAddressString)
                PhDereferenceObject(gatewayAddressString);

            gatewayAddressString = PhQueryRegistryString(keyHandle, L"DefaultGateway");
        }

        //PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_CONNECTIVITY, 1, internet ? L"Internet" : L"Local");
        PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_DOMAIN, 1, domainString ? domainString->Buffer : L"");
        PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_IPADDRESS, 1, ipAddressString ? ipAddressString->Buffer : L"");
        PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_SUBNET, 1, subnetAddressString ? subnetAddressString->Buffer : L"");
        PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_GATEWAY, 1, gatewayAddressString ? gatewayAddressString->Buffer : L"");

        if (domainString)
            PhDereferenceObject(domainString);

        if (ipAddressString)
            PhDereferenceObject(ipAddressString);

        if (subnetAddressString)
            PhDereferenceObject(subnetAddressString);

        if (gatewayAddressString)
            PhDereferenceObject(gatewayAddressString);

        NtClose(keyHandle);
    }

    PhDereferenceObject(keyNameIpV4);
    //PhDereferenceObject(keyNameIpV6);
}


static VOID NetAdapterUpdateDetails(
    _Inout_ PPH_NETADAPTER_DETAILS_CONTEXT Context
    )
{
    ULONG64 interfaceLinkSpeed = 0;
    ULONG64 interfaceRcvSpeed = 0;
    ULONG64 interfaceXmitSpeed = 0;
    FLOAT utilization = 0.0f;
    NDIS_STATISTICS_INFO interfaceStats = { 0 };
    NDIS_MEDIA_CONNECT_STATE mediaState = MediaConnectStateUnknown;

    if (Context->DeviceHandle)
    {
        NDIS_LINK_STATE interfaceState;

        NetworkAdapterQueryStatistics(Context->DeviceHandle, &interfaceStats);

        if (!NT_SUCCESS(NetworkAdapterQueryLinkState(Context->DeviceHandle, &interfaceState)))
        {
            NetworkAdapterQueryLinkSpeed(Context->DeviceHandle, &interfaceLinkSpeed);
        }
        else
        {
            mediaState = interfaceState.MediaConnectState;
            interfaceLinkSpeed = interfaceState.XmitLinkSpeed;
        }
    }
    else
    {
        if (GetIfEntry2_I)
        {
            MIB_IF_ROW2 interfaceRow;

            interfaceRow = QueryInterfaceRowVista(Context->AdapterEntry);

            interfaceStats.ifInDiscards = interfaceRow.InDiscards;
            interfaceStats.ifInErrors = interfaceRow.InErrors;
            interfaceStats.ifHCInOctets = interfaceRow.InOctets;
            interfaceStats.ifHCInUcastPkts = interfaceRow.InUcastPkts;
            //interfaceStats.ifHCInMulticastPkts;
            //interfaceStats.ifHCInBroadcastPkts;
            interfaceStats.ifHCOutOctets = interfaceRow.OutOctets;
            interfaceStats.ifHCOutUcastPkts = interfaceRow.OutUcastPkts;
            //interfaceStats.ifHCOutMulticastPkts;
            //interfaceStats.ifHCOutBroadcastPkts;
            interfaceStats.ifOutErrors = interfaceRow.OutErrors;
            interfaceStats.ifOutDiscards = interfaceRow.OutDiscards;
            interfaceStats.ifHCInUcastOctets = interfaceRow.InUcastOctets;
            interfaceStats.ifHCInMulticastOctets = interfaceRow.InMulticastOctets;
            interfaceStats.ifHCInBroadcastOctets = interfaceRow.InBroadcastOctets;
            interfaceStats.ifHCOutUcastOctets = interfaceRow.OutUcastOctets;
            interfaceStats.ifHCOutMulticastOctets = interfaceRow.OutMulticastOctets;
            interfaceStats.ifHCOutBroadcastOctets = interfaceRow.OutBroadcastOctets;
            //interfaceRow.InNUcastPkts;
            //interfaceRow.InUnknownProtos;
            //interfaceRow.OutNUcastPkts;
            //interfaceRow.OutQLen;

            mediaState = interfaceRow.MediaConnectState;
            interfaceLinkSpeed = interfaceRow.TransmitLinkSpeed;
        }
        else
        {
            MIB_IFROW interfaceRow;

            interfaceRow = QueryInterfaceRowXP(Context->AdapterEntry);

            interfaceStats.ifInDiscards = interfaceRow.dwInDiscards;
            interfaceStats.ifInErrors = interfaceRow.dwInErrors;
            interfaceStats.ifHCInOctets = interfaceRow.dwInOctets;
            interfaceStats.ifHCInUcastPkts = interfaceRow.dwInUcastPkts;
            //interfaceStats.ifHCInMulticastPkts;
            //interfaceStats.ifHCInBroadcastPkts;
            interfaceStats.ifHCOutOctets = interfaceRow.dwOutOctets;
            interfaceStats.ifHCOutUcastPkts = interfaceRow.dwOutUcastPkts;
            //interfaceStats.ifHCOutMulticastPkts;
            //interfaceStats.ifHCOutBroadcastPkts;
            interfaceStats.ifOutErrors = interfaceRow.dwOutErrors;
            interfaceStats.ifOutDiscards = interfaceRow.dwOutDiscards;
            //interfaceStats.ifHCInUcastOctets;
            //interfaceStats.ifHCInMulticastOctets;
            //interfaceStats.ifHCInBroadcastOctets;
            //interfaceStats.ifHCOutUcastOctets;
            //interfaceStats.ifHCOutMulticastOctets;
            //interfaceStats.ifHCOutBroadcastOctets;
            //interfaceRow.InNUcastPkts;
            //interfaceRow.InUnknownProtos;
            //interfaceRow.OutNUcastPkts;
            //interfaceRow.OutQLen;

            mediaState = interfaceRow.dwOperStatus == IF_OPER_STATUS_OPERATIONAL ? MediaConnectStateConnected : MediaConnectStateDisconnected;
            interfaceLinkSpeed = interfaceRow.dwSpeed;
        }
    }

    interfaceRcvSpeed = interfaceStats.ifHCInOctets - Context->LastDetailsInboundValue;
    interfaceXmitSpeed = interfaceStats.ifHCOutOctets - Context->LastDetailsIOutboundValue;
    //interfaceRcvUnicastSpeed = interfaceStats.ifHCInUcastOctets - Context->LastDetailsInboundUnicastValue;
    //interfaceXmitUnicastSpeed = interfaceStats.ifHCOutUcastOctets - Context->LastDetailsIOutboundUnicastValue;

    if (!Context->HaveFirstDetailsSample)
    {
        interfaceRcvSpeed = 0;
        interfaceXmitSpeed = 0;
        Context->HaveFirstDetailsSample = TRUE;
    }

    utilization = (FLOAT)(interfaceRcvSpeed + interfaceXmitSpeed) / (interfaceLinkSpeed / BITS_IN_ONE_BYTE) * 100;

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_STATE, 1, mediaState == MediaConnectStateConnected ? L"Connected" : L"Disconnected");

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_LINKSPEED, 1, PhaFormatString(L"%s/s", PhaFormatSize(interfaceLinkSpeed / BITS_IN_ONE_BYTE, -1)->Buffer)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_SENT, 1, PhaFormatSize(interfaceStats.ifHCOutOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_RECEIVED, 1, PhaFormatSize(interfaceStats.ifHCInOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_TOTAL, 1, PhaFormatSize(interfaceStats.ifHCInOctets + interfaceStats.ifHCOutOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_SENDING, 1, interfaceXmitSpeed != 0 ? PhaFormatString(L"%s/s", PhaFormatSize(interfaceXmitSpeed, -1)->Buffer)->Buffer : L"");
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_RECEIVING, 1, interfaceRcvSpeed != 0 ? PhaFormatString(L"%s/s", PhaFormatSize(interfaceRcvSpeed, -1)->Buffer)->Buffer : L"");
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UTILIZATION, 1, PhaFormatString(L"%.2f%%", utilization)->Buffer);

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_SENTPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCOutUcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_RECVPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInUcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_TOTALPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInUcastPkts + interfaceStats.ifHCOutUcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_SENT, 1, PhaFormatSize(interfaceStats.ifHCOutUcastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_RECEIVED, 1, PhaFormatSize(interfaceStats.ifHCInUcastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_TOTAL, 1, PhaFormatSize(interfaceStats.ifHCInUcastOctets + interfaceStats.ifHCOutUcastOctets, -1)->Buffer);
    //PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_SENDING, 1, interfaceXmitUnicastSpeed != 0 ? PhaFormatString(L"%s/s", PhaFormatSize(interfaceXmitUnicastSpeed, -1)->Buffer)->Buffer : L"");
    //PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_RECEIVING, 1, interfaceRcvUnicastSpeed != 0 ? PhaFormatString(L"%s/s", PhaFormatSize(interfaceRcvUnicastSpeed, -1)->Buffer)->Buffer : L"");
    //PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_UTILIZATION, 1, PhaFormatString(L"%.2f%%", utilization2)->Buffer);

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_SENTPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCOutBroadcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_RECVPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInBroadcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_TOTALPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInBroadcastPkts + interfaceStats.ifHCOutBroadcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_SENT, 1, PhaFormatSize(interfaceStats.ifHCOutBroadcastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_RECEIVED, 1, PhaFormatSize(interfaceStats.ifHCInBroadcastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_TOTAL, 1, PhaFormatSize(interfaceStats.ifHCInBroadcastOctets + interfaceStats.ifHCOutBroadcastOctets, -1)->Buffer);

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_SENTPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCOutMulticastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_RECVPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInMulticastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_TOTALPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInMulticastPkts + interfaceStats.ifHCOutMulticastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_SENT, 1, PhaFormatSize(interfaceStats.ifHCOutMulticastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_RECEIVED, 1, PhaFormatSize(interfaceStats.ifHCInMulticastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_TOTAL, 1, PhaFormatSize(interfaceStats.ifHCInMulticastOctets + interfaceStats.ifHCOutMulticastOctets, -1)->Buffer);

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_ERRORS_SENTPKTS, 1, PhaFormatUInt64(interfaceStats.ifOutErrors, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_ERRORS_RECVPKTS, 1, PhaFormatUInt64(interfaceStats.ifInErrors, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_ERRORS_TOTALPKTS, 1, PhaFormatUInt64(interfaceStats.ifInErrors + interfaceStats.ifOutErrors, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_ERRORS_SENT, 1, PhaFormatUInt64(interfaceStats.ifOutDiscards, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_ERRORS_RECEIVED, 1, PhaFormatUInt64(interfaceStats.ifInDiscards, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_ERRORS_TOTAL, 1, PhaFormatUInt64(interfaceStats.ifInDiscards + interfaceStats.ifOutDiscards, TRUE)->Buffer);

    Context->LastDetailsInboundValue = interfaceStats.ifHCInOctets;
    Context->LastDetailsIOutboundValue = interfaceStats.ifHCOutOctets;
    //Context->LastDetailsInboundUnicastValue = interfaceStats.ifHCInUcastOctets;
    //Context->LastDetailsIOutboundUnicastValue = interfaceStats.ifHCOutUcastOctets;
}


static INT_PTR CALLBACK AdapterDetailsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_NETADAPTER_DETAILS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_NETADAPTER_DETAILS_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPH_NETADAPTER_DETAILS_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_NCDESTROY)
        {
            RemoveProp(hwndDlg, L"Context");

            PhDereferenceObject(context->AdapterEntry->InterfaceGuid);
            PhFree(context->AdapterEntry);

            PhDereferenceObject(context->AdapterName);
            PhFree(context);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DETAILS_LIST);

            PhCenterWindow(hwndDlg, context->ParentHandle);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 325, L"Property");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 90, L"Value");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (context->AdapterName)
                SetWindowText(hwndDlg, context->AdapterName->Buffer);

            AddListViewItemGroups(context->ListViewHandle);

            NetAdapterLookupConfig(context);

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                ProcessesUpdatedHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            NetAdapterUpdateDetails(context);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(&PhProcessesUpdatedEvent, &context->ProcessesUpdatedRegistration);

            PhDeleteLayoutManager(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case MSG_UPDATE:
        {
            NetAdapterUpdateDetails(context);
        }
        break;
    }

    return FALSE;
}

static NTSTATUS ShowDetailsDialogThread(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    HWND dialogHandle;
    PH_AUTO_POOL autoPool;
    
    PhInitializeAutoPool(&autoPool);

    dialogHandle = CreateDialogParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_NETADAPTER_DETAILS),
        NULL,
        AdapterDetailsDlgProc,
        (LPARAM)Parameter
        );

    ShowWindow(dialogHandle, SW_SHOW);
    SetForegroundWindow(dialogHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(dialogHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);     
    DestroyWindow(dialogHandle);

    return STATUS_SUCCESS;
}

VOID ShowDetailsDialog(
    _In_opt_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    HANDLE dialogThread = NULL;
    PPH_NETADAPTER_DETAILS_CONTEXT context;

    context = PhAllocate(sizeof(PH_NETADAPTER_DETAILS_CONTEXT));
    memset(context, 0, sizeof(PH_NETADAPTER_DETAILS_CONTEXT));

    context->ParentHandle = Context->WindowHandle;
    context->AdapterName = PhDuplicateString(Context->AdapterName);

    context->AdapterEntry = PhAllocate(sizeof(PH_NETADAPTER_ENTRY));
    context->AdapterEntry->InterfaceIndex = Context->AdapterEntry->InterfaceIndex;
    context->AdapterEntry->InterfaceLuid = Context->AdapterEntry->InterfaceLuid;
    context->AdapterEntry->InterfaceGuid = PhDuplicateString(Context->AdapterEntry->InterfaceGuid);

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

    if (dialogThread = PhCreateThread(0, ShowDetailsDialogThread, context))
        NtClose(dialogThread);
}