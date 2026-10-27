/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"
#include <mapldr.h>
#include <iphlpapi.h>

static PVOID AtpIphlpapiBaseAddress = NULL;
static typeof(&GetAdaptersAddresses) AtpGetAdaptersAddresses = NULL;
static typeof(&GetIfEntry2) AtpGetIfEntry2 = NULL;

BOOLEAN AtpInitializeIphlpapi(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        if (AtpIphlpapiBaseAddress = PhLoadLibrary(L"iphlpapi.dll"))
        {
            AtpGetAdaptersAddresses = PhGetProcedureAddress(AtpIphlpapiBaseAddress, "GetAdaptersAddresses", 0);
            AtpGetIfEntry2 = PhGetProcedureAddress(AtpIphlpapiBaseAddress, "GetIfEntry2", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    return !!AtpGetAdaptersAddresses;
}

PCSTR AtpInterfaceTypeString(
    _In_ ULONG InterfaceType
    )
{
    switch (InterfaceType)
    {
    case IF_TYPE_ETHERNET_CSMACD:
        return "ethernet";
    case IF_TYPE_IEEE80211:
        return "wireless";
    case IF_TYPE_SOFTWARE_LOOPBACK:
        return "loopback";
    case IF_TYPE_PPP:
        return "ppp";
    case IF_TYPE_TUNNEL:
        return "tunnel";
    case IF_TYPE_IEEE1394:
        return "firewire";
    case IF_TYPE_ISO88025_TOKENRING:
        return "token_ring";
    case IF_TYPE_ATM:
        return "atm";
    case IF_TYPE_WWANPP:
    case IF_TYPE_WWANPP2:
        return "mobile_broadband";
    case IF_TYPE_OTHER:
        return "other";
    }

    return "unknown";
}

PCSTR AtpOperStatusString(
    _In_ IF_OPER_STATUS Status
    )
{
    switch (Status)
    {
    case IfOperStatusUp:
        return "up";
    case IfOperStatusDown:
        return "down";
    case IfOperStatusTesting:
        return "testing";
    case IfOperStatusUnknown:
        return "unknown";
    case IfOperStatusDormant:
        return "dormant";
    case IfOperStatusNotPresent:
        return "not_present";
    case IfOperStatusLowerLayerDown:
        return "lower_layer_down";
    }

    return "unknown";
}

PPH_STRING AtpFormatSocketAddress(
    _In_ PSOCKET_ADDRESS Address
    )
{
    WCHAR buffer[65];
    ULONG length = RTL_NUMBER_OF(buffer);

    if (!Address->lpSockaddr)
        return NULL;

    if (Address->lpSockaddr->sa_family == AF_INET)
    {
        PSOCKADDR_IN in = (PSOCKADDR_IN)Address->lpSockaddr;

        if (NT_SUCCESS(PhIpv4AddressToString(&in->sin_addr, 0, buffer, &length)))
            return PhCreateStringEx(buffer, (length - 1) * sizeof(WCHAR));
    }
    else if (Address->lpSockaddr->sa_family == AF_INET6)
    {
        PSOCKADDR_IN6 in6 = (PSOCKADDR_IN6)Address->lpSockaddr;

        if (NT_SUCCESS(PhIpv6AddressToString(&in6->sin6_addr, in6->sin6_scope_id, 0, buffer, &length)))
            return PhCreateStringEx(buffer, (length - 1) * sizeof(WCHAR));
    }

    return NULL;
}

VOID AtpAddAddressString(
    _In_ PVOID Array,
    _In_opt_ PPH_STRING String
    )
{
    PPH_BYTES utf8;

    if (!String)
        return;

    if (utf8 = PhConvertUtf16ToUtf8Ex(String->Buffer, String->Length))
    {
        PhAddJsonArrayObject(Array, PhCreateJsonStringObject(utf8->Buffer));
        PhDereferenceObject(utf8);
    }
}

VOID AtpAddAdapterAddresses(
    _In_ PVOID Row,
    _In_ PIP_ADAPTER_ADDRESSES Adapter
    )
{
    PVOID unicast;
    PVOID gateways;
    PVOID dnsServers;
    PIP_ADAPTER_UNICAST_ADDRESS unicastAddress;
    PIP_ADAPTER_GATEWAY_ADDRESS gatewayAddress;
    PIP_ADAPTER_DNS_SERVER_ADDRESS dnsAddress;

    unicast = PhCreateJsonArray();
    gateways = PhCreateJsonArray();
    dnsServers = PhCreateJsonArray();

    for (unicastAddress = Adapter->FirstUnicastAddress; unicastAddress; unicastAddress = unicastAddress->Next)
    {
        PPH_STRING address = AtpFormatSocketAddress(&unicastAddress->Address);

        // The prefix length belongs with the address; an address without it does not say what
        // the machine considers local.
        if (address)
        {
            PPH_STRING withPrefix = PhFormatString(L"%s/%lu", address->Buffer, unicastAddress->OnLinkPrefixLength);

            AtpAddAddressString(unicast, withPrefix);
            PhDereferenceObject(withPrefix);
            PhDereferenceObject(address);
        }
    }

    for (gatewayAddress = Adapter->FirstGatewayAddress; gatewayAddress; gatewayAddress = gatewayAddress->Next)
    {
        PPH_STRING address = AtpFormatSocketAddress(&gatewayAddress->Address);

        AtpAddAddressString(gateways, address);
        PhClearReference(&address);
    }

    for (dnsAddress = Adapter->FirstDnsServerAddress; dnsAddress; dnsAddress = dnsAddress->Next)
    {
        PPH_STRING address = AtpFormatSocketAddress(&dnsAddress->Address);

        AtpAddAddressString(dnsServers, address);
        PhClearReference(&address);
    }

    PhAddJsonObjectValue(Row, "addresses", unicast);
    PhAddJsonObjectValue(Row, "gateways", gateways);
    PhAddJsonObjectValue(Row, "dns_servers", dnsServers);
}

VOID AtpAddAdapterCounters(
    _In_ PVOID Row,
    _In_ PIP_ADAPTER_ADDRESSES Adapter
    )
{
    MIB_IF_ROW2 interfaceRow;
    PVOID counters;

    if (!AtpGetIfEntry2)
    {
        AtJsonAddNull(Row, "counters");
        AtJsonAddNull(Row, "connected");
        return;
    }

    memset(&interfaceRow, 0, sizeof(MIB_IF_ROW2));
    interfaceRow.InterfaceLuid = Adapter->Luid;

    if (AtpGetIfEntry2(&interfaceRow) != NO_ERROR)
    {
        AtJsonAddNull(Row, "counters");
        AtJsonAddNull(Row, "connected");
        return;
    }

    counters = PhCreateJsonObject();
    PhAddJsonObjectUInt64(counters, "in_octets", interfaceRow.InOctets);
    PhAddJsonObjectUInt64(counters, "out_octets", interfaceRow.OutOctets);
    PhAddJsonObjectUInt64(counters, "in_unicast_packets", interfaceRow.InUcastPkts);
    PhAddJsonObjectUInt64(counters, "out_unicast_packets", interfaceRow.OutUcastPkts);
    PhAddJsonObjectUInt64(counters, "in_errors", interfaceRow.InErrors);
    PhAddJsonObjectUInt64(counters, "out_errors", interfaceRow.OutErrors);
    PhAddJsonObjectUInt64(counters, "in_discards", interfaceRow.InDiscards);
    PhAddJsonObjectUInt64(counters, "out_discards", interfaceRow.OutDiscards);
    PhAddJsonObjectValue(Row, "counters", counters);

    PhAddJsonObjectBoolean(Row, "connected", interfaceRow.MediaConnectState == MediaConnectStateConnected);
}

PPH_STRING AtpFormatPhysicalAddress(
    _In_ PIP_ADAPTER_ADDRESSES Adapter
    )
{
    PH_STRING_BUILDER builder;
    ULONG i;

    if (!Adapter->PhysicalAddressLength)
        return NULL;

    PhInitializeStringBuilder(&builder, Adapter->PhysicalAddressLength * 3);

    for (i = 0; i < Adapter->PhysicalAddressLength; i++)
    {
        if (i != 0)
            PhAppendCharStringBuilder(&builder, L'-');

        PhAppendFormatStringBuilder(&builder, L"%02X", Adapter->PhysicalAddress[i]);
    }

    return PhFinalStringBuilderString(&builder);
}

VOID AtpListNetworkAdapters(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PIP_ADAPTER_ADDRESSES buffer = NULL;
    PIP_ADAPTER_ADDRESSES adapter;
    PPH_STRING nameContains;
    BOOLEAN connectedOnly;
    BOOLEAN includeAllInterfaces;
    ULONG bufferLength = 0;
    ULONG flags;
    ULONG error;
    AT_ROWS rows;
    PVOID structured;

    if (!AtpInitializeIphlpapi())
    {
        AtSetToolError(Result, "failed", STATUS_NOT_SUPPORTED, L"The IP helper library is not available.");
        return;
    }

    nameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    connectedOnly = AtJsonGetObjectBoolean(Call->Arguments, "connected_only");
    includeAllInterfaces = AtJsonGetObjectBoolean(Call->Arguments, "include_all_interfaces");

    // By default only the interfaces bound to TCP/IP. The full enumeration adds one
    // pseudo-interface per NDIS filter module bound to each adapter, which turns four adapters into
    // thirty-five.
    flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;

    if (includeAllInterfaces)
        flags |= GAA_FLAG_INCLUDE_ALL_INTERFACES;

    // Asking with no buffer is expected to overflow; anything else, including success, is a
    // failure to size the answer.
    if ((error = AtpGetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &bufferLength)) != ERROR_BUFFER_OVERFLOW)
    {
        AtSetToolStatusError(Result, PhDosErrorToNtStatus(error), L"Sizing the network adapter list");
        PhClearReference(&nameContains);
        return;
    }

    buffer = PhAllocateZero(bufferLength);

    if ((error = AtpGetAdaptersAddresses(AF_UNSPEC, flags, NULL, buffer, &bufferLength)) != ERROR_SUCCESS)
    {
        AtSetToolStatusError(Result, PhDosErrorToNtStatus(error), L"Enumerating the network adapters");
        PhFree(buffer);
        PhClearReference(&nameContains);
        return;
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (adapter = buffer; adapter; adapter = adapter->Next)
    {
        PVOID row;
        PPH_STRING friendlyName;
        PPH_STRING description;
        PPH_STRING physicalAddress;

        if (connectedOnly && adapter->OperStatus != IfOperStatusUp)
            continue;

        friendlyName = adapter->FriendlyName ? PhCreateString(adapter->FriendlyName) : NULL;
        description = adapter->Description ? PhCreateString(adapter->Description) : NULL;

        if (nameContains &&
            !AtContainsString(friendlyName, nameContains) &&
            !AtContainsString(description, nameContains))
        {
            PhClearReference(&friendlyName);
            PhClearReference(&description);
            continue;
        }

        row = PhCreateJsonObject();
        AtJsonAddString(row, "name", friendlyName);
        AtJsonAddString(row, "description", description);
        if (adapter->AdapterName)
            PhAddJsonObject(row, "guid", adapter->AdapterName);
        else
            AtJsonAddNull(row, "guid");
        PhAddJsonObjectUInt64(row, "interface_index", adapter->IfIndex);
        AtJsonAddHex(row, "interface_luid", adapter->Luid.Value);
        PhAddJsonObject(row, "type", AtpInterfaceTypeString(adapter->IfType));
        PhAddJsonObject(row, "operational_status", AtpOperStatusString(adapter->OperStatus));

        physicalAddress = AtpFormatPhysicalAddress(adapter);
        AtJsonAddString(row, "mac_address", physicalAddress);
        PhClearReference(&physicalAddress);

        PhAddJsonObjectUInt64(row, "mtu", adapter->Mtu);
        PhAddJsonObjectUInt64(row, "transmit_link_speed_bps", adapter->TransmitLinkSpeed);
        PhAddJsonObjectUInt64(row, "receive_link_speed_bps", adapter->ReceiveLinkSpeed);
        AtJsonAddStringZ(row, "dns_suffix", adapter->DnsSuffix);
        PhAddJsonObjectBoolean(row, "dhcp_enabled", !!(adapter->Flags & IP_ADAPTER_DHCP_ENABLED));
        PhAddJsonObjectBoolean(row, "dynamic_dns_enabled", !!(adapter->Flags & IP_ADAPTER_DDNS_ENABLED));

        AtpAddAdapterAddresses(row, adapter);
        AtpAddAdapterCounters(row, adapter);

        AtAddRow(&rows, row);

        PhClearReference(&friendlyName);
        PhClearReference(&description);
    }

    AtAddRows(structured, "adapters", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhFree(buffer);
    AtDeleteRows(&rows);
    PhClearReference(&nameContains);
}

VOID AtAdapterInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionListNetworkAdapters:
        AtpListNetworkAdapters(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
