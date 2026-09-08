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

#include <networktoolsintf.h>

PCWSTR AtProtocolTypeString(
    _In_ ULONG ProtocolType
    )
{
    switch (ProtocolType)
    {
    case PH_NETWORK_PROTOCOL_TCP4:
        return L"tcp";
    case PH_NETWORK_PROTOCOL_TCP6:
        return L"tcp6";
    case PH_NETWORK_PROTOCOL_UDP4:
        return L"udp";
    case PH_NETWORK_PROTOCOL_UDP6:
        return L"udp6";
    case PH_NETWORK_PROTOCOL_HYPERV:
        return L"hyperv";
    }

    return L"unknown";
}

BOOLEAN AtParseProtocolType(
    _In_opt_ PPH_STRING String,
    _Out_ PULONG ProtocolType
    )
{
    if (!String)
        return FALSE;

    if (PhEqualStringZ(String->Buffer, L"tcp", TRUE))
        *ProtocolType = PH_NETWORK_PROTOCOL_TCP4;
    else if (PhEqualStringZ(String->Buffer, L"tcp6", TRUE))
        *ProtocolType = PH_NETWORK_PROTOCOL_TCP6;
    else if (PhEqualStringZ(String->Buffer, L"udp", TRUE))
        *ProtocolType = PH_NETWORK_PROTOCOL_UDP4;
    else if (PhEqualStringZ(String->Buffer, L"udp6", TRUE))
        *ProtocolType = PH_NETWORK_PROTOCOL_UDP6;
    else if (PhEqualStringZ(String->Buffer, L"hyperv", TRUE))
        *ProtocolType = PH_NETWORK_PROTOCOL_HYPERV;
    else
        return FALSE;

    return TRUE;
}

// The NetworkTools plugin knows two things about an endpoint that nothing else here does: which
// country an address is registered to, from the GeoLite database it ships, and what a well-known
// port is usually for. Both are local lookups - no traffic leaves the machine for either.

PNETWORKTOOLS_INTERFACE AtGetNetworkToolsInterface(
    VOID
    )
{
    static PNETWORKTOOLS_INTERFACE pluginInterface = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_PLUGIN plugin;

        if (plugin = PhFindPlugin(NETWORKTOOLS_PLUGIN_NAME))
        {
            pluginInterface = PhGetPluginInformation(plugin)->Interface;

            if (pluginInterface && pluginInterface->Version < NETWORKTOOLS_INTERFACE_VERSION)
                pluginInterface = NULL;
        }

        PhEndInitOnce(&initOnce);
    }

    return pluginInterface;
}

// An address the database will never answer for. The lookup refuses these itself, so without this
// a private address and an address the database does not cover look identical from outside.
BOOLEAN AtpIsPrivateAddress(
    _In_ PPH_IP_ADDRESS Address
    )
{
    if (Address->Type == PH_NETWORK_TYPE_IPV4)
    {
        return !!(IN4_IS_ADDR_UNSPECIFIED(&Address->InAddr) ||
            IN4_IS_ADDR_LOOPBACK(&Address->InAddr) ||
            IN4_IS_ADDR_BROADCAST(&Address->InAddr) ||
            IN4_IS_ADDR_MULTICAST(&Address->InAddr) ||
            IN4_IS_ADDR_LINKLOCAL(&Address->InAddr) ||
            IN4_IS_ADDR_MC_LINKLOCAL(&Address->InAddr) ||
            IN4_IS_ADDR_RFC1918(&Address->InAddr));
    }

    if (Address->Type == PH_NETWORK_TYPE_IPV6)
    {
        return !!(IN6_IS_ADDR_UNSPECIFIED(&Address->In6Addr) ||
            IN6_IS_ADDR_LOOPBACK(&Address->In6Addr) ||
            IN6_IS_ADDR_MULTICAST(&Address->In6Addr) ||
            IN6_IS_ADDR_LINKLOCAL(&Address->In6Addr) ||
            IN6_IS_ADDR_MC_LINKLOCAL(&Address->In6Addr));
    }

    return TRUE;
}

VOID AtpAddCountry(
    _In_ PVOID Object,
    _In_opt_ PNETWORKTOOLS_INTERFACE Interface,
    _In_ PPH_IP_ADDRESS Address
    )
{
    ULONG geoNameId = 0;
    PPH_STRING countryName = NULL;

    if (Interface && !AtpIsPrivateAddress(Address) &&
        Interface->LookupCountryCode(*Address, &geoNameId, &countryName))
    {
        // A lookup can succeed with only one of the two: an address the database knows but has
        // no country name for comes back with an identifier of zero, which is not an identifier.
        AtJsonAddString(Object, "country", countryName);

        if (geoNameId)
            PhAddJsonObjectUInt64(Object, "country_geoname_id", geoNameId);
        else
            AtJsonAddNull(Object, "country_geoname_id");

        // Not an ISO code: the database returns a GeoNames identifier, and calling it a country
        // code would have a reader expecting two letters.
        PhClearReference(&countryName);
    }
    else
    {
        AtJsonAddNull(Object, "country");
        AtJsonAddNull(Object, "country_geoname_id");
    }
}

VOID AtpAddServiceName(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_opt_ PNETWORKTOOLS_INTERFACE Interface,
    _In_ ULONG Port,
    _In_ ULONG ProtocolType
    )
{
    PPH_STRINGREF serviceName;

    if (Interface && Port && Interface->LookupPortServiceName(
        Port,
        FlagOn(ProtocolType, PH_PROTOCOL_TYPE_TCP) ? IPPROTO_TCP : IPPROTO_UDP,
        &serviceName
        ))
    {
        AtJsonAddStringRef(Object, Key, serviceName);
    }
    else
    {
        AtJsonAddNull(Object, Key);
    }
}

VOID AtpLookupIpCountry(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PNETWORKTOOLS_INTERFACE pluginInterface;
    PPH_STRING address;
    PH_IP_ADDRESS ipAddress;
    PVOID structured;

    if (!(pluginInterface = AtGetNetworkToolsInterface()))
    {
        AtSetToolHint(Result, AT_HINT_PLUGIN_MISSING);
        AtSetToolError(
            Result,
            "plugin_missing",
            STATUS_NOT_FOUND,
            L"The NetworkTools plugin is not loaded, so there is no geolocation database to read."
            );
        return;
    }

    if (!(address = AtGetArgumentString(Call->Arguments, "address")))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"address is required.");
        return;
    }

    memset(&ipAddress, 0, sizeof(PH_IP_ADDRESS));

    {
        USHORT port = 0;
        ULONG scopeId = 0;

        if (NT_SUCCESS(PhIpv4StringToAddress(address->Buffer, TRUE, &ipAddress.InAddr, &port)))
        {
            ipAddress.Type = PH_NETWORK_TYPE_IPV4;
        }
        else if (NT_SUCCESS(PhIpv6StringToAddress(address->Buffer, &ipAddress.In6Addr, &scopeId, &port)))
        {
            ipAddress.Type = PH_NETWORK_TYPE_IPV6;
        }
        else
        {
            AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"That is not an IPv4 or IPv6 address.");
            PhDereferenceObject(address);
            return;
        }
    }

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "address", address);
    PhAddJsonObject(structured, "family", ipAddress.Type == PH_NETWORK_TYPE_IPV6 ? "ipv6" : "ipv4");
    // Reported separately so a null country can be read: a private address was never going to have
    // one, a public address without one means the database did not cover it or is not installed.
    PhAddJsonObjectBoolean(structured, "is_private", AtpIsPrivateAddress(&ipAddress));
    AtpAddCountry(structured, pluginInterface, &ipAddress);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhDereferenceObject(address);
}

PPH_STRING AtFormatNetworkEndpoint(
    _In_ PPH_IP_ENDPOINT Endpoint,
    _In_ ULONG ProtocolType,
    _In_ ULONG ScopeId,
    _In_ BOOLEAN IncludePort
    )
{
    WCHAR buffer[65];
    ULONG length = RTL_NUMBER_OF(buffer);
    PPH_STRING address = NULL;

    if (ProtocolType & PH_NETWORK_TYPE_IPV4)
    {
        if (NT_SUCCESS(PhIpv4AddressToString(&Endpoint->Address.InAddr, 0, buffer, &length)))
            address = PhCreateStringEx(buffer, (length - 1) * sizeof(WCHAR));
    }
    else if (ProtocolType & PH_NETWORK_TYPE_IPV6)
    {
        if (NT_SUCCESS(PhIpv6AddressToString(&Endpoint->Address.In6Addr, ScopeId, 0, buffer, &length)))
            address = PhCreateStringEx(buffer, (length - 1) * sizeof(WCHAR));
    }

    if (!address)
        address = PhCreateString(L"?");

    if (IncludePort)
    {
        PPH_STRING result = PhFormatString(L"%s:%lu", address->Buffer, Endpoint->Port);

        PhDereferenceObject(address);
        return result;
    }

    return address;
}

NTSTATUS AtFindNetworkConnection(
    _In_opt_ PVOID Arguments,
    _In_opt_ PPH_PROCESS_ITEM ProcessItem,
    _Out_ PPH_NETWORK_ITEM* NetworkItem,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING protocolString;
    PPH_STRING localString;
    PPH_STRING remoteString;
    ULONG protocolType;
    ULONG64 localPort;
    ULONG64 remotePort;
    PPH_NETWORK_CONNECTION connections;
    ULONG numberOfConnections;
    PPH_NETWORK_ITEM found = NULL;
    ULONG i;

    protocolString = AtGetArgumentString(Arguments, "protocol");
    localString = AtGetArgumentString(Arguments, "local_address");
    remoteString = AtGetArgumentString(Arguments, "remote_address");

    if (!AtParseProtocolType(protocolString, &protocolType) ||
        !AtGetArgumentUInt64(Arguments, "local_port", &localPort) ||
        !AtGetArgumentUInt64(Arguments, "remote_port", &remotePort) ||
        !localString || !remoteString)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"protocol, local_address, local_port, remote_address and remote_port are all required.");
        PhClearReference(&protocolString);
        PhClearReference(&localString);
        PhClearReference(&remoteString);
        return STATUS_INVALID_PARAMETER;
    }

    // The live table, not the provider cache: the cache is only maintained while the Network tab
    // is showing.
    if (!PhGetNetworkConnections(&connections, &numberOfConnections))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"Enumerating the network connections failed.");
        PhClearReference(&protocolString);
        PhClearReference(&localString);
        PhClearReference(&remoteString);
        return STATUS_UNSUCCESSFUL;
    }

    for (i = 0; i < numberOfConnections && !found; i++)
    {
        PPH_NETWORK_CONNECTION item = &connections[i];
        PPH_STRING itemLocal;
        PPH_STRING itemRemote;

        if (item->ProtocolType != protocolType)
            continue;

        if (ProcessItem && item->ProcessId != ProcessItem->ProcessId)
            continue;

        if (item->LocalEndpoint.Port != (ULONG)localPort || item->RemoteEndpoint.Port != (ULONG)remotePort)
            continue;

        itemLocal = AtFormatNetworkEndpoint(&item->LocalEndpoint, item->ProtocolType, item->LocalScopeId, FALSE);
        itemRemote = AtFormatNetworkEndpoint(&item->RemoteEndpoint, item->ProtocolType, item->RemoteScopeId, FALSE);

        if (PhEqualString(itemLocal, localString, TRUE) && PhEqualString(itemRemote, remoteString, TRUE))
        {
            found = PhAllocateZero(sizeof(PH_NETWORK_ITEM));
            found->ProtocolType = item->ProtocolType;
            found->LocalEndpoint = item->LocalEndpoint;
            found->RemoteEndpoint = item->RemoteEndpoint;
            found->State = item->State;
            found->ProcessId = item->ProcessId;
            found->LocalScopeId = item->LocalScopeId;
            found->RemoteScopeId = item->RemoteScopeId;
            found->CreateTime = item->CreateTime;
        }

        PhDereferenceObject(itemLocal);
        PhDereferenceObject(itemRemote);
    }

    PhFree(connections);
    PhClearReference(&protocolString);
    PhClearReference(&localString);
    PhClearReference(&remoteString);

    if (!found)
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"No matching connection is in the live table. Re-list the connections and try again.");
        return STATUS_NOT_FOUND;
    }

    *NetworkItem = found;

    return STATUS_SUCCESS;
}

typedef struct _AT_NETWORK_FILTER
{
    BOOLEAN HavePid;
    HANDLE Pid;
    BOOLEAN HaveProtocol;
    ULONG Protocol;
    PPH_STRING State;
    PPH_STRING AddressContains;
    BOOLEAN HavePort;
    ULONG Port;
    BOOLEAN ExcludeListeners;
} AT_NETWORK_FILTER, *PAT_NETWORK_FILTER;

BOOLEAN AtpNetworkMatchesFilter(
    _In_ PAT_NETWORK_FILTER Filter,
    _In_ PPH_NETWORK_ITEM Item,
    _In_ PPH_STRING Local,
    _In_ PPH_STRING Remote,
    _In_ PCPH_STRINGREF StateName
    )
{
    if (Filter->HavePid && Item->ProcessId != Filter->Pid)
        return FALSE;

    if (Filter->HaveProtocol && Item->ProtocolType != Filter->Protocol)
        return FALSE;

    if (Filter->HavePort && Item->LocalEndpoint.Port != Filter->Port && Item->RemoteEndpoint.Port != Filter->Port)
        return FALSE;

    if (Filter->State)
    {
        if (!StateName || PhFindStringInStringRef(StateName, &Filter->State->sr, TRUE) == SIZE_MAX)
            return FALSE;
    }

    if (Filter->ExcludeListeners)
    {
        BOOLEAN isTcp = !!(Item->ProtocolType & PH_PROTOCOL_TYPE_TCP);

        if (isTcp && Item->State == MIB_TCP_STATE_LISTEN)
            return FALSE;
        if (!isTcp && Item->RemoteEndpoint.Port == 0)
            return FALSE;
    }

    if (Filter->AddressContains)
    {
        if (!AtContainsString(Local, Filter->AddressContains) &&
            !AtContainsString(Remote, Filter->AddressContains) &&
            !AtContainsString(Item->RemoteHostString, Filter->AddressContains) &&
            !AtContainsString(Item->LocalHostString, Filter->AddressContains))
        {
            return FALSE;
        }
    }

    return TRUE;
}

VOID AtpListNetworkConnections(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_NETWORK_FILTER filter;
    PPH_NETWORK_CONNECTION connections;
    PNETWORKTOOLS_INTERFACE networkTools;
    ULONG numberOfConnections;
    PPH_STRING protocol;
    ULONG64 pid;
    ULONG64 port;
    AT_ROWS rows;
    PVOID structured;
    ULONG i;

    memset(&filter, 0, sizeof(AT_NETWORK_FILTER));

    if (Call->Arguments)
    {
        if (AtGetArgumentUInt64(Call->Arguments, "pid", &pid) && pid <= MAXULONG)
        {
            filter.HavePid = TRUE;
            filter.Pid = UlongToHandle((ULONG)pid);
        }

        if (protocol = AtGetArgumentString(Call->Arguments, "protocol"))
        {
            filter.HaveProtocol = AtParseProtocolType(protocol, &filter.Protocol);
            PhDereferenceObject(protocol);
        }

        filter.State = AtGetArgumentString(Call->Arguments, "state");
        filter.AddressContains = AtGetArgumentString(Call->Arguments, "address_contains");
        filter.ExcludeListeners = AtJsonGetObjectBoolean(Call->Arguments, "exclude_listeners");

        if (AtGetArgumentUInt64(Call->Arguments, "port", &port) && port <= 0xffff)
        {
            filter.HavePort = TRUE;
            filter.Port = (ULONG)port;
        }
    }

    // The live table, not the provider cache: the cache is only maintained while the Network tab
    // is showing. A cached item, when there is one, still supplies the names the provider resolved.
    if (!PhGetNetworkConnections(&connections, &numberOfConnections))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"Enumerating the network connections failed.");
        PhClearReference(&filter.State);
        PhClearReference(&filter.AddressContains);
        return;
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    networkTools = AtGetNetworkToolsInterface();

    for (i = 0; i < numberOfConnections; i++)
    {
        PPH_NETWORK_CONNECTION connection = &connections[i];
        PH_NETWORK_ITEM view;
        PPH_NETWORK_ITEM item = &view;
        PPH_NETWORK_ITEM cachedItem;
        PPH_PROCESS_ITEM processItem = NULL;
        PPH_STRING local;
        PPH_STRING remote;
        PCPH_STRINGREF stateName = NULL;
        BOOLEAN isTcp = !!(connection->ProtocolType & PH_PROTOCOL_TYPE_TCP);
        PVOID row;

        // A cache-shaped view of the live entry, so the filter and the row see one thing.
        memset(&view, 0, sizeof(PH_NETWORK_ITEM));
        view.ProtocolType = connection->ProtocolType;
        view.LocalEndpoint = connection->LocalEndpoint;
        view.RemoteEndpoint = connection->RemoteEndpoint;
        view.State = connection->State;
        view.ProcessId = connection->ProcessId;
        view.CreateTime = connection->CreateTime;
        view.LocalScopeId = connection->LocalScopeId;
        view.RemoteScopeId = connection->RemoteScopeId;

        if (connection->ProcessId)
            processItem = PhReferenceProcessItem(connection->ProcessId);

        cachedItem = PhReferenceNetworkItem(
            connection->ProtocolType,
            &connection->LocalEndpoint,
            &connection->RemoteEndpoint,
            connection->ProcessId
            );

        if (cachedItem)
        {
            view.ProcessName = cachedItem->ProcessName;
            view.OwnerName = cachedItem->OwnerName;
            view.LocalHostString = cachedItem->LocalHostString;
            view.RemoteHostString = cachedItem->RemoteHostString;
        }

        if (!view.ProcessName && processItem)
            view.ProcessName = processItem->ProcessName;

        local = AtFormatNetworkEndpoint(&item->LocalEndpoint, item->ProtocolType, item->LocalScopeId, FALSE);
        remote = AtFormatNetworkEndpoint(&item->RemoteEndpoint, item->ProtocolType, item->RemoteScopeId, FALSE);

        if (isTcp)
            stateName = PhGetTcpStateName(item->State);

        if (AtpNetworkMatchesFilter(&filter, item, local, remote, stateName))
        {
            row = PhCreateJsonObject();
            AtJsonAddStringZ(row, "protocol", AtProtocolTypeString(item->ProtocolType));
            AtJsonAddString(row, "local_address", local);
            PhAddJsonObjectUInt64(row, "local_port", item->LocalEndpoint.Port);
            AtJsonAddString(row, "remote_address", remote);
            PhAddJsonObjectUInt64(row, "remote_port", item->RemoteEndpoint.Port);
            AtJsonAddStringRef(row, "state", stateName);

            if (item->ProcessId)
            {
                PhAddJsonObjectUInt64(row, "pid", HandleToUlong(item->ProcessId));

                if (processItem)
                    PhAddJsonObjectUInt64(row, "process_sequence_number", processItem->ProcessSequenceNumber);
                else
                    AtJsonAddNull(row, "process_sequence_number");
            }
            else
            {
                AtJsonAddNull(row, "pid");
                AtJsonAddNull(row, "process_sequence_number");
            }

            AtJsonAddString(row, "process_name", item->ProcessName);
            AtJsonAddString(row, "owner_name", item->OwnerName);
            AtJsonAddString(row, "remote_host", item->RemoteHostString);
            AtJsonAddTime(row, "create_time", &item->CreateTime);

            AtpAddServiceName(row, "local_service", networkTools, item->LocalEndpoint.Port, item->ProtocolType);
            AtpAddServiceName(row, "remote_service", networkTools, item->RemoteEndpoint.Port, item->ProtocolType);
            AtpAddCountry(row, networkTools, &item->RemoteEndpoint.Address);

            AtAddRow(&rows, row);
        }

        PhDereferenceObject(local);
        PhDereferenceObject(remote);
        PhClearReference(&cachedItem);
        PhClearReference(&processItem);
    }

    PhFree(connections);

    AtAddRows(structured, "connections", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&filter.State);
    PhClearReference(&filter.AddressContains);
}

VOID AtpCloseNetworkConnection(
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PVOID structured;

    if (Target->NetworkItem->State != MIB_TCP_STATE_ESTAB)
    {
        AtSetToolError(Result, "invalid_state", STATUS_INVALID_PARAMETER, L"Only an established TCP connection can be closed.");
        return;
    }

    status = PhSetTcpEntry(Target->NetworkItem);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Closing the connection");
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    PhAddJsonObject(structured, "action", "close_network_connection");
    AtJsonAddString(structured, "connection", Target->ConnectionText);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtNetworkInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    switch (Tool->Action)
    {
    case AtActionLookupIpCountry:
        AtpLookupIpCountry(Call, Result);
        break;
    case AtActionListNetworkConnections:
        AtpListNetworkConnections(Call, Result);
        break;
    case AtActionCloseNetworkConnection:
        AtpCloseNetworkConnection(Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
