/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2015-2023
 *
 */

#include "devices.h"

VOID NTAPI NetAdapterProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PDV_NETADAPTER_DETAILS_CONTEXT context = Context;

    if (context && context->WindowHandle)
    {
        PostMessage(context->WindowHandle, WM_PH_UPDATE_DIALOG, 0, 0);
    }
}

VOID NetAdapterAddListViewItemGroups(
    _In_ HWND ListViewHandle
    )
{
    ListView_EnableGroupView(ListViewHandle, TRUE);
    PhAddListViewGroup(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, L"Adapter");
    PhAddListViewGroup(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, L"Unicast");
    PhAddListViewGroup(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, L"Broadcast");
    PhAddListViewGroup(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, L"Multicast");
    PhAddListViewGroup(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, L"Errors");

    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_STATE, L"State", NULL);
    //PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_CONNECTIVITY, L"Connectivity");
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_IPV4ADDRESS, L"IPv4 address", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_IPV4SUBNET, L"IPv4 subnet mask", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_IPV4GATEWAY, L"IPv4 default gateway", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_IPV4DNS, L"IPv4 DNS address", NULL);

    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_IPV6ADDRESS, L"IPv6 address", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_IPV6TEMPADDRESS, L"IPv6 address (temporary)", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_IPV6GATEWAY, L"IPv6 default gateway", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_IPV6DNS, L"IPv6 DNS address", NULL);

    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_DOMAIN, L"Domain", NULL);

    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_LINKSPEED, L"Link speed", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_SENT, L"Sent", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_RECEIVED, L"Received", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_TOTAL, L"Total", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_SENDING, L"Sending", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_RECEIVING, L"Receiving", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ADAPTER, NETADAPTER_DETAILS_INDEX_UTILIZATION, L"Utilization", NULL);

    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_SENTPKTS, L"Sent packets", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_RECVPKTS, L"Received packets", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_TOTALPKTS, L"Total packets", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_SENT, L"Sent", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_RECEIVED, L"Received", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_TOTAL, L"Total", NULL);
    //PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_SENDING, L"Sending");
    //PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_RECEIVING, L"Receiving");
    //PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_UNICAST, NETADAPTER_DETAILS_INDEX_UNICAST_UTILIZATION, L"Utilization");

    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_SENTPKTS, L"Sent packets", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_RECVPKTS, L"Received packets", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_TOTALPKTS, L"Total packets", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_SENT, L"Sent", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_RECEIVED, L"Received", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_BROADCAST, NETADAPTER_DETAILS_INDEX_BROADCAST_TOTAL, L"Total", NULL);

    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_SENTPKTS, L"Sent packets", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_RECVPKTS, L"Received packets", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_TOTALPKTS, L"Total packets", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_SENT, L"Sent", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_RECEIVED, L"Received", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_MULTICAST, NETADAPTER_DETAILS_INDEX_MULTICAST_TOTAL, L"Total", NULL);

    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_SENTPKTS, L"Send errors", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_RECVPKTS, L"Receive errors", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_TOTALPKTS, L"Total errors", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_SENT, L"Send discards", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_RECEIVED, L"Receive discards", NULL);
    PhAddListViewGroupItem(ListViewHandle, NETADAPTER_DETAILS_CATEGORY_ERRORS, NETADAPTER_DETAILS_INDEX_ERRORS_TOTAL, L"Total discards", NULL);
}

PVOID NetAdapterGetAddresses(
    _In_ ULONG Family
    )
{
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_INCLUDE_ALL_INTERFACES;
    ULONG bufferLength = 0;
    PIP_ADAPTER_ADDRESSES buffer;

    if (GetAdaptersAddresses(Family, flags, NULL, NULL, &bufferLength) != ERROR_BUFFER_OVERFLOW)
        return NULL;

    buffer = PhAllocate(bufferLength);
    memset(buffer, 0, bufferLength);

    if (GetAdaptersAddresses(Family, flags, NULL, buffer, &bufferLength) != ERROR_SUCCESS)
    {
        PhFree(buffer);
        return NULL;
    }

    return buffer;
}

VOID NetAdapterEnumerateAddresses(
    _In_ PVOID AddressesBuffer,
    _In_ ULONG64 InterfaceLuid,
    _In_ PPH_STRING_BUILDER DomainBuffer,
    _In_ PPH_STRING_BUILDER Ipv4AddressBuffer,
    _In_ PPH_STRING_BUILDER Ipv6AddressBuffer,
    _In_ PPH_STRING_BUILDER Ipv6TempAddressBuffer,
    _In_ PPH_STRING_BUILDER SubnetAddressBuffer,
    _In_ PPH_STRING_BUILDER Ipv4GatewayAddressBuffer,
    _In_ PPH_STRING_BUILDER Ipv6GatewayAddressBuffer,
    _In_ PPH_STRING_BUILDER Ipv4DnsAddressBuffer,
    _In_ PPH_STRING_BUILDER Ipv6DnsAddressBuffer
    )
{
    PIP_ADAPTER_ADDRESSES addressesBuffer;
    PIP_ADAPTER_UNICAST_ADDRESS unicastAddress;
    PIP_ADAPTER_GATEWAY_ADDRESS gatewayAddress;
    PIP_ADAPTER_DNS_SERVER_ADDRESS dnsAddress;

    for (addressesBuffer = AddressesBuffer; addressesBuffer; addressesBuffer = addressesBuffer->Next)
    {
        if (addressesBuffer->Luid.Value != InterfaceLuid)
            continue;

        if (addressesBuffer->DnsSuffix && PhCountStringZ(addressesBuffer->DnsSuffix) > 0)
        {
            PhAppendFormatStringBuilder(DomainBuffer, L"%s, ", addressesBuffer->DnsSuffix);
        }

        for (unicastAddress = addressesBuffer->FirstUnicastAddress; unicastAddress; unicastAddress = unicastAddress->Next)
        {
            if (unicastAddress->Address.lpSockaddr->sa_family == AF_INET)
            {
                PSOCKADDR_IN ipv4SockAddr;
                IN_ADDR subnetMask = { 0 };
                WCHAR ipv4AddressString[INET_ADDRSTRLEN] = L"";
                WCHAR subnetAddressString[INET_ADDRSTRLEN] = L"";
                ULONG ipv4AddressStringLength = RTL_NUMBER_OF(ipv4AddressString);
                ULONG subnetAddressStringLength = RTL_NUMBER_OF(subnetAddressString);

                ipv4SockAddr = (PSOCKADDR_IN)unicastAddress->Address.lpSockaddr;

                if (NT_SUCCESS(RtlIpv4AddressToStringEx(
                    &ipv4SockAddr->sin_addr,
                    0,
                    ipv4AddressString,
                    &ipv4AddressStringLength
                    )))
                {
                    PhAppendFormatStringBuilder(Ipv4AddressBuffer, L"%s, ", ipv4AddressString);
                }

                ConvertLengthToIpv4Mask(unicastAddress->OnLinkPrefixLength, &subnetMask.s_addr);

                if (NT_SUCCESS(RtlIpv4AddressToStringEx(
                    &subnetMask,
                    0,
                    subnetAddressString,
                    &subnetAddressStringLength
                    )))
                {
                    PhAppendFormatStringBuilder(SubnetAddressBuffer, L"%s, ", subnetAddressString);
                }
            }
            else if (unicastAddress->Address.lpSockaddr->sa_family == AF_INET6)
            {
                PSOCKADDR_IN6 ipv6SockAddr;
                WCHAR ipv6AddressString[INET6_ADDRSTRLEN] = L"";
                ULONG ipv6AddressStringLength = RTL_NUMBER_OF(ipv6AddressString);

                ipv6SockAddr = (PSOCKADDR_IN6)unicastAddress->Address.lpSockaddr;

                if (NT_SUCCESS(RtlIpv6AddressToStringEx(
                    &ipv6SockAddr->sin6_addr,
                    ipv6SockAddr->sin6_scope_id,
                    0,
                    ipv6AddressString,
                    &ipv6AddressStringLength
                    )))
                {
                    if (unicastAddress->SuffixOrigin == IpSuffixOriginRandom)
                    {
                        PhAppendFormatStringBuilder(Ipv6TempAddressBuffer, L"%s, ", ipv6AddressString);
                    }
                    else
                    {
                        PhAppendFormatStringBuilder(Ipv6AddressBuffer, L"%s, ", ipv6AddressString);
                    }
                }
            }
        }

        for (gatewayAddress = addressesBuffer->FirstGatewayAddress; gatewayAddress; gatewayAddress = gatewayAddress->Next)
        {
            if (gatewayAddress->Address.lpSockaddr->sa_family == AF_INET)
            {
                PSOCKADDR_IN ipv4SockAddr;
                WCHAR ipv4AddressString[INET_ADDRSTRLEN] = L"";
                ULONG ipv4AddressStringLength = RTL_NUMBER_OF(ipv4AddressString);

                ipv4SockAddr = (PSOCKADDR_IN)gatewayAddress->Address.lpSockaddr;

                if (NT_SUCCESS(RtlIpv4AddressToStringEx(
                    &ipv4SockAddr->sin_addr,
                    0,
                    ipv4AddressString,
                    &ipv4AddressStringLength
                    )))
                {
                    PhAppendFormatStringBuilder(Ipv4GatewayAddressBuffer, L"%s, ", ipv4AddressString);
                }
            }
            else if (gatewayAddress->Address.lpSockaddr->sa_family == AF_INET6)
            {
                PSOCKADDR_IN6 ipv6SockAddr;
                WCHAR ipv6AddressString[INET6_ADDRSTRLEN] = L"";
                ULONG ipv6AddressStringLength = RTL_NUMBER_OF(ipv6AddressString);

                ipv6SockAddr = (PSOCKADDR_IN6)gatewayAddress->Address.lpSockaddr;

                if (NT_SUCCESS(RtlIpv6AddressToStringEx(
                    &ipv6SockAddr->sin6_addr,
                    ipv6SockAddr->sin6_scope_id,
                    0,
                    ipv6AddressString,
                    &ipv6AddressStringLength
                    )))
                {
                    PhAppendFormatStringBuilder(Ipv6GatewayAddressBuffer, L"%s, ", ipv6AddressString);
                }
            }
        }

        for (dnsAddress = addressesBuffer->FirstDnsServerAddress; dnsAddress; dnsAddress = dnsAddress->Next)
        {
            if (dnsAddress->Address.lpSockaddr->sa_family == AF_INET)
            {
                PSOCKADDR_IN ipv4SockAddr;
                WCHAR ipv4AddressString[INET_ADDRSTRLEN] = L"";
                ULONG ipv4AddressStringLength = RTL_NUMBER_OF(ipv4AddressString);

                ipv4SockAddr = (PSOCKADDR_IN)dnsAddress->Address.lpSockaddr;

                if (NT_SUCCESS(RtlIpv4AddressToStringEx(
                    &ipv4SockAddr->sin_addr,
                    0,
                    ipv4AddressString,
                    &ipv4AddressStringLength
                    )))
                {
                    PhAppendFormatStringBuilder(Ipv4DnsAddressBuffer, L"%s, ", ipv4AddressString);
                }
            }
            else if (dnsAddress->Address.lpSockaddr->sa_family == AF_INET6)
            {
                PSOCKADDR_IN6 ipv6SockAddr;
                WCHAR ipv6AddressString[INET6_ADDRSTRLEN] = L"";
                ULONG ipv6AddressStringLength = RTL_NUMBER_OF(ipv6AddressString);

                ipv6SockAddr = (PSOCKADDR_IN6)dnsAddress->Address.lpSockaddr;

                if (NT_SUCCESS(RtlIpv6AddressToStringEx(
                    &ipv6SockAddr->sin6_addr,
                    ipv6SockAddr->sin6_scope_id,
                    0,
                    ipv6AddressString,
                    &ipv6AddressStringLength
                    )))
                {
                    PhAppendFormatStringBuilder(Ipv6DnsAddressBuffer, L"%s, ", ipv6AddressString);
                }
            }
        }
    }
}

VOID NetAdapterLookupConfig(
    _Inout_ PDV_NETADAPTER_DETAILS_CONTEXT Context
    )
{
    PVOID addressesBuffer = NULL;
    PPH_STRING domainString = NULL;
    PPH_STRING ipv4AddressString = NULL;
    PPH_STRING ipv6AddressString = NULL;
    PPH_STRING ipv6TempAddressString = NULL;
    PPH_STRING subnetAddressString = NULL;
    PPH_STRING ipv4gatewayAddressString = NULL;
    PPH_STRING ipv6gatewayAddressString = NULL;
    PPH_STRING ipv4dnsAddressString = NULL;
    PPH_STRING ipv6dnsAddressString = NULL;
    PH_STRING_BUILDER domainBuffer;
    PH_STRING_BUILDER ipv4AddressBuffer;
    PH_STRING_BUILDER ipv6AddressBuffer;
    PH_STRING_BUILDER ipv6TempAddressBuffer;
    PH_STRING_BUILDER subnetAddressBuffer;
    PH_STRING_BUILDER ipv4gatewayAddressBuffer;
    PH_STRING_BUILDER ipv6gatewayAddressBuffer;
    PH_STRING_BUILDER ipv4dnsAddressBuffer;
    PH_STRING_BUILDER ipv6dnsAddressBuffer;

    PhInitializeStringBuilder(&domainBuffer, 0x100);
    PhInitializeStringBuilder(&ipv4AddressBuffer, 0x100);
    PhInitializeStringBuilder(&ipv6AddressBuffer, 0x100);
    PhInitializeStringBuilder(&ipv6TempAddressBuffer, 0x100);
    PhInitializeStringBuilder(&subnetAddressBuffer, 0x100);
    PhInitializeStringBuilder(&ipv4gatewayAddressBuffer, 0x100);
    PhInitializeStringBuilder(&ipv6gatewayAddressBuffer, 0x100);
    PhInitializeStringBuilder(&ipv4dnsAddressBuffer, 0x100);
    PhInitializeStringBuilder(&ipv6dnsAddressBuffer, 0x100);

    if (addressesBuffer = NetAdapterGetAddresses(AF_INET))
    {
        NetAdapterEnumerateAddresses(
            addressesBuffer,
            Context->AdapterId.InterfaceLuid.Value,
            &domainBuffer,
            &ipv4AddressBuffer,
            &ipv6AddressBuffer,
            &ipv6TempAddressBuffer,
            &subnetAddressBuffer,
            &ipv4gatewayAddressBuffer,
            &ipv6gatewayAddressBuffer,
            &ipv4dnsAddressBuffer,
            &ipv6dnsAddressBuffer
            );

        PhFree(addressesBuffer);
    }

    if (addressesBuffer = NetAdapterGetAddresses(AF_INET6))
    {
        NetAdapterEnumerateAddresses(
            addressesBuffer,
            Context->AdapterId.InterfaceLuid.Value,
            &domainBuffer,
            &ipv4AddressBuffer,
            &ipv6AddressBuffer,
            &ipv6TempAddressBuffer,
            &subnetAddressBuffer,
            &ipv4gatewayAddressBuffer,
            &ipv6gatewayAddressBuffer,
            &ipv4dnsAddressBuffer,
            &ipv6dnsAddressBuffer
            );

        PhFree(addressesBuffer);
    }

    if (domainBuffer.String->Length > 2)
        PhRemoveEndStringBuilder(&domainBuffer, 2);
    if (ipv4AddressBuffer.String->Length > 2)
        PhRemoveEndStringBuilder(&ipv4AddressBuffer, 2);
    if (ipv6AddressBuffer.String->Length > 2)
        PhRemoveEndStringBuilder(&ipv6AddressBuffer, 2);
    if (ipv6TempAddressBuffer.String->Length > 2)
        PhRemoveEndStringBuilder(&ipv6TempAddressBuffer, 2);
    if (subnetAddressBuffer.String->Length > 2)
        PhRemoveEndStringBuilder(&subnetAddressBuffer, 2);
    if (ipv4gatewayAddressBuffer.String->Length > 2)
        PhRemoveEndStringBuilder(&ipv4gatewayAddressBuffer, 2);
    if (ipv6gatewayAddressBuffer.String->Length > 2)
        PhRemoveEndStringBuilder(&ipv6gatewayAddressBuffer, 2);
    if (ipv4dnsAddressBuffer.String->Length > 2)
        PhRemoveEndStringBuilder(&ipv4dnsAddressBuffer, 2);
    if (ipv6dnsAddressBuffer.String->Length > 2)
        PhRemoveEndStringBuilder(&ipv6dnsAddressBuffer, 2);

    domainString = PhFinalStringBuilderString(&domainBuffer);
    ipv4AddressString = PhFinalStringBuilderString(&ipv4AddressBuffer);
    ipv6AddressString = PhFinalStringBuilderString(&ipv6AddressBuffer);
    ipv6TempAddressString = PhFinalStringBuilderString(&ipv6TempAddressBuffer);
    subnetAddressString = PhFinalStringBuilderString(&subnetAddressBuffer);
    ipv4gatewayAddressString = PhFinalStringBuilderString(&ipv4gatewayAddressBuffer);
    ipv6gatewayAddressString = PhFinalStringBuilderString(&ipv6gatewayAddressBuffer);
    ipv4dnsAddressString = PhFinalStringBuilderString(&ipv4dnsAddressBuffer);
    ipv6dnsAddressString = PhFinalStringBuilderString(&ipv6dnsAddressBuffer);

    //PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_CONNECTIVITY, 1, internet ? L"Internet" : L"Local");
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_IPV4ADDRESS, 1, ipv4AddressString->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_IPV4SUBNET, 1, subnetAddressString->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_IPV4GATEWAY, 1, ipv4gatewayAddressString->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_IPV4DNS, 1, ipv4dnsAddressString->Buffer);

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_IPV6ADDRESS, 1, ipv6AddressString->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_IPV6TEMPADDRESS, 1, ipv6TempAddressString->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_IPV6GATEWAY, 1, ipv6gatewayAddressString->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_IPV6DNS, 1, ipv6dnsAddressString->Buffer);

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_DOMAIN, 1, domainString->Buffer);

    PhDeleteStringBuilder(&domainBuffer);
    PhDeleteStringBuilder(&ipv4AddressBuffer);
    PhDeleteStringBuilder(&ipv6AddressBuffer);
    PhDeleteStringBuilder(&ipv6TempAddressBuffer);
    PhDeleteStringBuilder(&subnetAddressBuffer);
    PhDeleteStringBuilder(&ipv4gatewayAddressBuffer);
    PhDeleteStringBuilder(&ipv6gatewayAddressBuffer);
    PhDeleteStringBuilder(&ipv4dnsAddressBuffer);
    PhDeleteStringBuilder(&ipv6dnsAddressBuffer);
}

VOID NETIOAPI_API_ NetAdapterChangeCallback(
    _In_ PVOID CallerContext,
    _In_opt_ PMIB_IPINTERFACE_ROW Row,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    PDV_NETADAPTER_DETAILS_CONTEXT context = CallerContext;

    if (NotificationType == MibInitialNotification)
    {
        NetAdapterLookupConfig(context);
    }
    else if (NotificationType == MibParameterNotification)
    {
        if (Row && (Row->InterfaceLuid.Value = context->AdapterId.InterfaceLuid.Value))
        {
            NetAdapterLookupConfig(context);
        }
    }
}

VOID NetAdapterUpdateDetails(
    _Inout_ PDV_NETADAPTER_DETAILS_CONTEXT Context
    )
{
    ULONG64 interfaceLinkSpeed = 0;
    ULONG64 interfaceRcvSpeed = 0;
    ULONG64 interfaceXmitSpeed = 0;
    NDIS_STATISTICS_INFO interfaceStats = { 0 };
    NDIS_MEDIA_CONNECT_STATE mediaState = MediaConnectStateUnknown;
    HANDLE deviceHandle = NULL;

    if (NetAdapterEnableNdis)
    {
        if (NT_SUCCESS(PhCreateFile(
            &deviceHandle,
            &Context->AdapterId.InterfacePath->sr,
            FILE_GENERIC_READ,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            if (!Context->CheckedDeviceSupport)
            {
                // Check the network adapter supports the OIDs we're going to be using.
                if (NetworkAdapterQuerySupported(deviceHandle))
                {
                    Context->DeviceSupported = TRUE;
                }

                Context->CheckedDeviceSupport = TRUE;
            }

            if (!Context->DeviceSupported)
            {
                // Device is faulty. Close the handle so we can fallback to GetIfEntry.
                NtClose(deviceHandle);
                deviceHandle = NULL;
            }
        }
    }

    if (deviceHandle)
    {
        NDIS_LINK_STATE interfaceState;

        NetworkAdapterQueryStatistics(deviceHandle, &interfaceStats);

        if (NT_SUCCESS(NetworkAdapterQueryLinkState(deviceHandle, &interfaceState)))
        {
            mediaState = interfaceState.MediaConnectState;
            interfaceLinkSpeed = interfaceState.XmitLinkSpeed;
        }
        else
        {
            NetworkAdapterQueryLinkSpeed(deviceHandle, &interfaceLinkSpeed);
        }

        NtClose(deviceHandle);
    }
    else
    {
        MIB_IF_ROW2 interfaceRow;

        if (NetworkAdapterQueryInterfaceRow(&Context->AdapterId, MibIfEntryNormal, &interfaceRow))
        {
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
    }

    interfaceRcvSpeed = interfaceStats.ifHCInOctets - Context->LastDetailsInboundValue;
    interfaceXmitSpeed = interfaceStats.ifHCOutOctets - Context->LastDetailsIOutboundValue;
    //interfaceRcvUnicastSpeed = interfaceStats.ifHCInUcastOctets - Context->LastDetailsInboundUnicastValue;
    //interfaceXmitUnicastSpeed = interfaceStats.ifHCOutUcastOctets - Context->LastDetailsIOutboundUnicastValue;

    if (!Context->HaveFirstSample)
    {
        interfaceRcvSpeed = 0;
        interfaceXmitSpeed = 0;
        Context->HaveFirstSample = TRUE;
    }

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_STATE, 1, mediaState == MediaConnectStateConnected ? L"Connected" : L"Disconnected");
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_LINKSPEED, 1, PhaFormatString(
        L"%s/s (%s)",
        PhaFormatSize(interfaceLinkSpeed / BITS_IN_ONE_BYTE, ULONG_MAX)->Buffer,
        PH_AUTO_T(PH_STRING, NetAdapterFormatBitratePrefix(interfaceLinkSpeed))->Buffer
        )->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_SENT, 1, PhaFormatSize(interfaceStats.ifHCOutOctets, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_RECEIVED, 1, PhaFormatSize(interfaceStats.ifHCInOctets, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_TOTAL, 1, PhaFormatSize(interfaceStats.ifHCInOctets + interfaceStats.ifHCOutOctets, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_SENDING, 1, interfaceXmitSpeed != 0 ? PhaFormatString(
        L"%s/s (%s)", 
        PhaFormatSize(interfaceXmitSpeed, ULONG_MAX)->Buffer,
        PH_AUTO_T(PH_STRING, NetAdapterFormatBitratePrefix(interfaceXmitSpeed * BITS_IN_ONE_BYTE))->Buffer
        )->Buffer : L"");
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_RECEIVING, 1, interfaceRcvSpeed != 0 ? PhaFormatString(
        L"%s/s (%s)", 
        PhaFormatSize(interfaceRcvSpeed, ULONG_MAX)->Buffer,
        PH_AUTO_T(PH_STRING, NetAdapterFormatBitratePrefix(interfaceRcvSpeed * BITS_IN_ONE_BYTE))->Buffer
        )->Buffer : L"");

    if (interfaceLinkSpeed > 0)
    {
        FLOAT utilization = (FLOAT)(interfaceRcvSpeed + interfaceXmitSpeed) / (interfaceLinkSpeed / BITS_IN_ONE_BYTE);

        PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UTILIZATION, 1, PhaFormatString(L"%.2f%%", 100.0 * utilization)->Buffer);
    }
    else
    {
        PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UTILIZATION, 1, L"N/A");
    }

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_SENTPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCOutUcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_RECVPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInUcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_TOTALPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInUcastPkts + interfaceStats.ifHCOutUcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_SENT, 1, PhaFormatSize(interfaceStats.ifHCOutUcastOctets, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_RECEIVED, 1, PhaFormatSize(interfaceStats.ifHCInUcastOctets, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_TOTAL, 1, PhaFormatSize(interfaceStats.ifHCInUcastOctets + interfaceStats.ifHCOutUcastOctets, ULONG_MAX)->Buffer);
    //PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_SENDING, 1, interfaceXmitUnicastSpeed != 0 ? PhaFormatString(L"%s/s", PhaFormatSize(interfaceXmitUnicastSpeed, ULONG_MAX)->Buffer)->Buffer : L"");
    //PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_RECEIVING, 1, interfaceRcvUnicastSpeed != 0 ? PhaFormatString(L"%s/s", PhaFormatSize(interfaceRcvUnicastSpeed, ULONG_MAX)->Buffer)->Buffer : L"");
    //PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_UNICAST_UTILIZATION, 1, PhaFormatString(L"%.2f%%", utilization2)->Buffer);

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_SENTPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCOutBroadcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_RECVPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInBroadcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_TOTALPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInBroadcastPkts + interfaceStats.ifHCOutBroadcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_SENT, 1, PhaFormatSize(interfaceStats.ifHCOutBroadcastOctets, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_RECEIVED, 1, PhaFormatSize(interfaceStats.ifHCInBroadcastOctets, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_BROADCAST_TOTAL, 1, PhaFormatSize(interfaceStats.ifHCInBroadcastOctets + interfaceStats.ifHCOutBroadcastOctets, ULONG_MAX)->Buffer);

    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_SENTPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCOutMulticastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_RECVPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInMulticastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_TOTALPKTS, 1, PhaFormatUInt64(interfaceStats.ifHCInMulticastPkts + interfaceStats.ifHCOutMulticastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_SENT, 1, PhaFormatSize(interfaceStats.ifHCOutMulticastOctets, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_RECEIVED, 1, PhaFormatSize(interfaceStats.ifHCInMulticastOctets, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, NETADAPTER_DETAILS_INDEX_MULTICAST_TOTAL, 1, PhaFormatSize(interfaceStats.ifHCInMulticastOctets + interfaceStats.ifHCOutMulticastOctets, ULONG_MAX)->Buffer);

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

INT_PTR CALLBACK NetAdapterDetailsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_NETADAPTER_DETAILS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_NETADAPTER_DETAILS_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DETAILS_LIST);

            PhSetApplicationWindowIcon(hwndDlg);

            PhSetWindowText(hwndDlg, PhGetStringOrDefault(context->AdapterName, L"Unknown network adapter"));

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 200, L"Property");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 240, L"Value");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            if (PhGetIntegerPairSetting(SETTING_NAME_NETWORK_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_NETWORK_POSITION, SETTING_NAME_NETWORK_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentHandle);

            NetAdapterAddListViewItemGroups(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_NETWORK_COLUMNS, context->ListViewHandle);
            PhLoadListViewSortColumnsFromSetting(SETTING_NAME_NETWORK_SORTCOLUMN, context->ListViewHandle);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                NetAdapterProcessesUpdatedHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            NetAdapterLookupConfig(context);
            NetAdapterUpdateDetails(context);

            NotifyIpInterfaceChange(
                AF_UNSPEC,
                NetAdapterChangeCallback,
                context,
                FALSE,
                &context->NotifyHandle
                );

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport")); // HACK
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &context->ProcessesUpdatedRegistration);

            if (context->NotifyHandle)
                CancelMibChangeNotify2(context->NotifyHandle);

            PhSaveWindowPlacementToSetting(SETTING_NAME_NETWORK_POSITION, SETTING_NAME_NETWORK_SIZE, hwndDlg);
            PhSaveListViewSortColumnsToSetting(SETTING_NAME_NETWORK_SORTCOLUMN, context->ListViewHandle);
            PhSaveListViewColumnsToSetting(SETTING_NAME_NETWORK_COLUMNS, context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);
            PostQuitMessage(0);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                DestroyWindow(hwndDlg);
                break;
            }
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_PH_SHOW_DIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        NetAdapterUpdateDetails(context);
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();

                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case PHAPP_IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    }

    return FALSE;
}

VOID FreeNetAdapterDetailsContext(
    _In_ PDV_NETADAPTER_DETAILS_CONTEXT Context
    )
{
    DeleteNetAdapterId(&Context->AdapterId);
    PhClearReference(&Context->AdapterName);
    PhFree(Context);
}

NTSTATUS ShowNetAdapterDetailsDialogThread(
    _In_ PVOID Parameter
    )
{
    PDV_NETADAPTER_DETAILS_CONTEXT context = (PDV_NETADAPTER_DETAILS_CONTEXT)Parameter;
    BOOL result;
    MSG message;
    HWND dialogHandle;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    dialogHandle = context->SysInfoContext->DetailsWindowDialogHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_NETADAPTER_DETAILS),
        NULL,
        NetAdapterDetailsDlgProc,
        context
        );

    PhSetEvent(&context->SysInfoContext->DetailsWindowInitializedEvent);

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

    PhResetEvent(&context->SysInfoContext->DetailsWindowInitializedEvent);

    if (context->SysInfoContext->DetailsWindowThreadHandle)
    {
        NtClose(context->SysInfoContext->DetailsWindowThreadHandle);
        context->SysInfoContext->DetailsWindowThreadHandle = NULL;
    }

    FreeNetAdapterDetailsContext(context);

    return STATUS_SUCCESS;
}

VOID ShowNetAdapterDetailsDialog(
    _In_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    if (!Context->DetailsWindowThreadHandle)
    {
        HANDLE threadHandle;
        PDV_NETADAPTER_DETAILS_CONTEXT context;

        context = PhAllocateZero(sizeof(DV_NETADAPTER_DETAILS_CONTEXT));
        context->ParentHandle = Context->WindowHandle;
        context->SysInfoContext = Context;

        PhSetReference(&context->AdapterName, Context->AdapterEntry->AdapterName);
        CopyNetAdapterId(&context->AdapterId, &Context->AdapterEntry->AdapterId);

        if (!NT_SUCCESS(PhCreateThreadEx(&threadHandle, ShowNetAdapterDetailsDialogThread, context)))
        {
            PhShowError(Context->WindowHandle, L"%s", L"Unable to create the window.");
            return;
        }

        PhWaitForEvent(&Context->DetailsWindowInitializedEvent, NULL);

        Context->DetailsWindowThreadHandle = threadHandle;
    }

    PostMessage(Context->DetailsWindowDialogHandle, WM_PH_SHOW_DIALOG, 0, 0);
}
