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
    ULONG numberOfConnections;
    PPH_STRING protocol;
    ULONG64 pid;
    ULONG64 port;
    PVOID structured;
    PVOID rows;
    ULONG count = 0;
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
    rows = PhCreateJsonArray();

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

            PhAddJsonArrayObject(rows, row);
            count++;
        }

        PhDereferenceObject(local);
        PhDereferenceObject(remote);
        PhClearReference(&cachedItem);
        PhClearReference(&processItem);
    }

    PhFree(connections);

    PhAddJsonObjectValue(structured, "connections", rows);
    PhAddJsonObjectUInt64(structured, "count", count);
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
