/*
 * Process Hacker - 
 *   network provider
 * 
 * Copyright (C) 2010 wj32
 * Copyright (C) 2010 evilpie
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

#define NETPRV_PRIVATE
#include <phapp.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>

typedef struct _PH_NETWORK_CONNECTION
{
    ULONG ProtocolType;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;
    ULONG State;
    HANDLE ProcessId;
    ULONGLONG OwnerInfo[PH_NETWORK_OWNER_INFO_SIZE];
} PH_NETWORK_CONNECTION, *PPH_NETWORK_CONNECTION;

typedef DWORD (WINAPI *_GetExtendedTcpTable)(
    __out_bcount_opt(*pdwSize) PVOID pTcpTable,
    __inout PDWORD pdwSize,
    __in BOOL bOrder,
    __in ULONG ulAf,
    __in TCP_TABLE_CLASS TableClass,
    __in ULONG Reserved
    );

typedef DWORD (WINAPI *_GetExtendedUdpTable)(
    __out_bcount_opt(*pdwSize) PVOID pUdpTable,
    __inout PDWORD pdwSize,
    __in BOOL bOrder,
    __in ULONG ulAf,
    __in UDP_TABLE_CLASS TableClass,
    __in ULONG Reserved
    );

VOID NTAPI PhpNetworkItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

BOOLEAN PhpNetworkHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpNetworkHashtableHashFunction(
    __in PVOID Entry
    );

BOOLEAN PhGetNetworkConnections(
    __out PPH_NETWORK_CONNECTION *Connections,
    __out PULONG NumberOfConnections
    );

PPH_OBJECT_TYPE PhNetworkItemType;

PPH_HASHTABLE PhNetworkHashtable;
PH_QUEUED_LOCK PhNetworkHashtableLock;

PH_CALLBACK PhNetworkItemAddedEvent;
PH_CALLBACK PhNetworkItemModifiedEvent;
PH_CALLBACK PhNetworkItemRemovedEvent;
PH_CALLBACK PhNetworkItemsUpdatedEvent;

static _GetExtendedTcpTable GetExtendedTcpTable_I;
static _GetExtendedUdpTable GetExtendedUdpTable_I;

BOOLEAN PhInitializeNetworkProvider()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhNetworkItemType,
        L"NetworkItem",
        0,
        PhpNetworkItemDeleteProcedure
        )))
        return FALSE;
    
    PhNetworkHashtable = PhCreateHashtable(
        sizeof(PPH_NETWORK_ITEM),
        PhpNetworkHashtableCompareFunction,
        PhpNetworkHashtableHashFunction,
        40
        );

    PhInitializeQueuedLock(&PhNetworkHashtableLock);

    PhInitializeCallback(&PhNetworkItemAddedEvent);
    PhInitializeCallback(&PhNetworkItemModifiedEvent);
    PhInitializeCallback(&PhNetworkItemRemovedEvent);
    PhInitializeCallback(&PhNetworkItemsUpdatedEvent);

    return TRUE;
}

PPH_NETWORK_ITEM PhCreateNetworkItem()
{
    PPH_NETWORK_ITEM networkItem;

    if (!NT_SUCCESS(PhCreateObject(
        &networkItem,
        sizeof(PH_NETWORK_ITEM),
        0,
        PhNetworkItemType,
        0
        )))
        return NULL;
    
    memset(networkItem, 0, sizeof(PH_NETWORK_ITEM));

    return networkItem;
}

VOID NTAPI PhpNetworkItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)Object;

    if (networkItem->LocalHostString)
        PhDereferenceObject(networkItem->LocalHostString);
    if (networkItem->RemoteHostString)
        PhDereferenceObject(networkItem->RemoteHostString);
}

BOOLEAN PhpNetworkHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PPH_NETWORK_ITEM networkItem1 = *(PPH_NETWORK_ITEM *)Entry1;
    PPH_NETWORK_ITEM networkItem2 = *(PPH_NETWORK_ITEM *)Entry2;

    return
        networkItem1->ProtocolType == networkItem2->ProtocolType &&
        PhIpEndpointEquals(&networkItem1->LocalEndpoint, &networkItem2->LocalEndpoint) &&
        PhIpEndpointEquals(&networkItem1->RemoteEndpoint, &networkItem2->RemoteEndpoint) &&
        networkItem1->ProcessId == networkItem2->ProcessId;

    return FALSE;
}

ULONG NTAPI PhpNetworkHashtableHashFunction(
    __in PVOID Entry
    )
{
    PPH_NETWORK_ITEM networkItem = *(PPH_NETWORK_ITEM *)Entry;

    return
        networkItem->ProtocolType ^
        PhHashIpEndpoint(&networkItem->LocalEndpoint) ^
        PhHashIpEndpoint(&networkItem->RemoteEndpoint) ^
        (ULONG)networkItem->ProcessId;
}

PPH_NETWORK_ITEM PhReferenceNetworkItem(
    __in ULONG ProtocolType,
    __in PPH_IP_ENDPOINT LocalEndpoint,
    __in PPH_IP_ENDPOINT RemoteEndpoint,
    __in HANDLE ProcessId
    )
{
    PH_NETWORK_ITEM lookupNetworkItem;
    PPH_NETWORK_ITEM lookupNetworkItemPtr = &lookupNetworkItem;
    PPH_NETWORK_ITEM *networkItemPtr;
    PPH_NETWORK_ITEM networkItem;

    lookupNetworkItem.ProtocolType = ProtocolType;
    lookupNetworkItem.LocalEndpoint = *LocalEndpoint;
    lookupNetworkItem.RemoteEndpoint = *RemoteEndpoint;
    lookupNetworkItem.ProcessId = ProcessId;

    PhAcquireQueuedLockSharedFast(&PhNetworkHashtableLock);

    networkItemPtr = (PPH_NETWORK_ITEM *)PhGetHashtableEntry(
        PhNetworkHashtable,
        &lookupNetworkItemPtr
        );

    if (networkItemPtr)
    {
        networkItem = *networkItemPtr;
        PhReferenceObject(networkItem);
    }
    else
    {
        networkItem = NULL;
    }

    PhReleaseQueuedLockSharedFast(&PhNetworkHashtableLock);

    return networkItem;
}

__assumeLocked VOID PhpRemoveNetworkItem(
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    PhRemoveHashtableEntry(PhNetworkHashtable, &NetworkItem);
    PhDereferenceObject(NetworkItem);
}

VOID PhNetworkProviderUpdate(
    __in PVOID Object
    )
{
    PPH_NETWORK_CONNECTION connections;
    ULONG numberOfConnections;
    ULONG i;

    if (!GetExtendedTcpTable_I || !GetExtendedUdpTable_I)
    {
        LoadLibrary(L"iphlpapi.dll");
        GetExtendedTcpTable_I = PhGetProcAddress(L"iphlpapi.dll", "GetExtendedTcpTable");
        GetExtendedUdpTable_I = PhGetProcAddress(L"iphlpapi.dll", "GetExtendedUdpTable");
    }

    if (!PhGetNetworkConnections(&connections, &numberOfConnections))
        return;

    {
        PPH_LIST connectionsToRemove = NULL;
        ULONG enumerationKey = 0;
        PPH_NETWORK_ITEM *networkItem;

        while (PhEnumHashtable(PhNetworkHashtable, (PPVOID)&networkItem, &enumerationKey))
        {
            BOOLEAN found = FALSE;

            for (i = 0; i < numberOfConnections; i++)
            {
                if (
                    (*networkItem)->ProtocolType == connections[i].ProtocolType &&
                    PhIpEndpointEquals(&(*networkItem)->LocalEndpoint, &connections[i].LocalEndpoint) &&
                    PhIpEndpointEquals(&(*networkItem)->RemoteEndpoint, &connections[i].RemoteEndpoint) &&
                    (*networkItem)->ProcessId == connections[i].ProcessId
                    )
                {
                    found = TRUE;
                    break;
                }
            }

            if (!found)
            {
                PhInvokeCallback(&PhNetworkItemRemovedEvent, *networkItem);

                if (!connectionsToRemove)
                    connectionsToRemove = PhCreateList(2);

                PhAddListItem(connectionsToRemove, *networkItem);
            }
        }

        if (connectionsToRemove)
        {
            PhAcquireQueuedLockExclusive(&PhNetworkHashtableLock);

            for (i = 0; i < connectionsToRemove->Count; i++)
            {
                PhpRemoveNetworkItem(connectionsToRemove->Items[i]);
            }

            PhReleaseQueuedLockExclusive(&PhNetworkHashtableLock);
            PhDereferenceObject(connectionsToRemove);
        }
    }

    for (i = 0; i < numberOfConnections; i++)
    {
        PPH_NETWORK_ITEM networkItem;

        // Try to find the connection in our hashtable.
        networkItem = PhReferenceNetworkItem(
            connections[i].ProtocolType,
            &connections[i].LocalEndpoint,
            &connections[i].RemoteEndpoint,
            connections[i].ProcessId
            );

        if (!networkItem)
        {
            // Network item not found, create it.

            networkItem = PhCreateNetworkItem();
            networkItem->ProtocolType = connections[i].ProtocolType;
            networkItem->LocalEndpoint = connections[i].LocalEndpoint;
            networkItem->RemoteEndpoint = connections[i].RemoteEndpoint;
            networkItem->State = connections[i].State;
            networkItem->ProcessId = connections[i].ProcessId;
            memcpy(networkItem->OwnerInfo, connections[i].OwnerInfo, sizeof(ULONGLONG) * PH_NETWORK_OWNER_INFO_SIZE);

            networkItem->ProtocolTypeString = PhGetProtocolTypeName(networkItem->ProtocolType);

            if (networkItem->LocalEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
                RtlIpv4AddressToString(&networkItem->LocalEndpoint.Address.InAddr, networkItem->LocalAddressString);
            else
                RtlIpv6AddressToString(&networkItem->LocalEndpoint.Address.In6Addr, networkItem->LocalAddressString);

            PhPrintUInt32(networkItem->LocalPortString, networkItem->LocalEndpoint.Port);

            if (
                networkItem->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE &&
                networkItem->RemoteEndpoint.Address.Ipv4 != 0
                )
            {
                RtlIpv4AddressToString(&networkItem->RemoteEndpoint.Address.InAddr, networkItem->RemoteAddressString);
            }
            else if (
                networkItem->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE &&
                !PhIsIpAddressNull(&networkItem->RemoteEndpoint.Address)
                )
            {
                RtlIpv6AddressToString(&networkItem->RemoteEndpoint.Address.In6Addr, networkItem->RemoteAddressString);
            }

            if (networkItem->RemoteEndpoint.Address.Type != 0 && networkItem->RemoteEndpoint.Port != 0)
                PhPrintUInt32(networkItem->RemotePortString, networkItem->RemoteEndpoint.Port);

            // Add the network item to the hashtable.
            PhAcquireQueuedLockExclusive(&PhNetworkHashtableLock);
            PhAddHashtableEntry(PhNetworkHashtable, &networkItem);
            PhReleaseQueuedLockExclusive(&PhNetworkHashtableLock);

            // Raise the network item added event.
            PhInvokeCallback(&PhNetworkItemAddedEvent, networkItem);
        }
        else
        {
            // TODO
        }
    }

    PhFree(connections);

    PhInvokeCallback(&PhNetworkItemsUpdatedEvent, NULL);
}

PWSTR PhGetProtocolTypeName(
    __in ULONG ProtocolType
    )
{
    switch (ProtocolType)
    {
    case PH_TCP4_NETWORK_PROTOCOL:
        return L"TCP";
    case PH_TCP6_NETWORK_PROTOCOL:
        return L"TCP6";
    case PH_UDP4_NETWORK_PROTOCOL:
        return L"UDP";
    case PH_UDP6_NETWORK_PROTOCOL:
        return L"UDP6";
    default:
        return L"Unknown";
    }
}

BOOLEAN PhGetNetworkConnections(
    __out PPH_NETWORK_CONNECTION *Connections,
    __out PULONG NumberOfConnections
    )
{
    PVOID table;
    DWORD tableSize;
    PMIB_TCPTABLE_OWNER_MODULE tcp4Table;
    PMIB_UDPTABLE_OWNER_MODULE udp4Table;
    PMIB_TCP6TABLE_OWNER_MODULE tcp6Table;
    PMIB_UDP6TABLE_OWNER_MODULE udp6Table;
    ULONG count = 0;
    ULONG i;
    ULONG index = 0;
    PPH_NETWORK_CONNECTION connections;

    if (!GetExtendedTcpTable_I || !GetExtendedUdpTable_I)
        return FALSE;

    // TCP IPv4
    tableSize = 0;
    GetExtendedTcpTable_I(NULL, &tableSize, FALSE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0);

    table = PhAllocate(tableSize);
    if (GetExtendedTcpTable_I(table, &tableSize, FALSE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0) == 0)
    {
        tcp4Table = table;
        count += tcp4Table->dwNumEntries;
    }
    else
    {
        PhFree(table);
        tcp4Table = NULL;
    }

    // TCP IPv6
    tableSize = 0;
    GetExtendedTcpTable_I(NULL, &tableSize, FALSE, AF_INET6, TCP_TABLE_OWNER_MODULE_ALL, 0);

    table = PhAllocate(tableSize);
    if (GetExtendedTcpTable_I(table, &tableSize, FALSE, AF_INET6, TCP_TABLE_OWNER_MODULE_ALL, 0) == 0)
    {
        tcp6Table = table;
        count += tcp6Table->dwNumEntries;
    }
    else
    {
        PhFree(table);
        tcp6Table = NULL;
    }

    // UDP IPv4
    tableSize = 0;
    GetExtendedUdpTable_I(NULL, &tableSize, FALSE, AF_INET, UDP_TABLE_OWNER_MODULE, 0);

    table = PhAllocate(tableSize);
    if (GetExtendedUdpTable_I(table, &tableSize, FALSE, AF_INET, UDP_TABLE_OWNER_MODULE, 0) == 0)
    {
        udp4Table = table;
        count += udp4Table->dwNumEntries;
    }
    else
    {
        PhFree(table);
        udp4Table = NULL;
    }

    // UDP IPv6
    tableSize = 0;
    GetExtendedUdpTable_I(NULL, &tableSize, FALSE, AF_INET6, UDP_TABLE_OWNER_MODULE, 0);

    table = PhAllocate(tableSize);
    if (GetExtendedUdpTable_I(table, &tableSize, FALSE, AF_INET6, UDP_TABLE_OWNER_MODULE, 0) == 0)
    {
        udp6Table = table;
        count += udp6Table->dwNumEntries;
    }
    else
    {
        PhFree(table);
        udp6Table = NULL;
    }

    connections = PhAllocate(sizeof(PH_NETWORK_CONNECTION) * count);
    memset(connections, 0, sizeof(PH_NETWORK_CONNECTION) * count);

    if (tcp4Table)
    {
        for (i = 0; i < tcp4Table->dwNumEntries; i++)
        {
            connections[index].ProtocolType = PH_TCP4_NETWORK_PROTOCOL;

            connections[index].LocalEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
            connections[index].LocalEndpoint.Address.Ipv4 = tcp4Table->table[i].dwLocalAddr;
            connections[index].LocalEndpoint.Port = _byteswap_ushort((USHORT)tcp4Table->table[i].dwLocalPort);

            connections[index].RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
            connections[index].RemoteEndpoint.Address.Ipv4 = tcp4Table->table[i].dwRemoteAddr;
            connections[index].RemoteEndpoint.Port = _byteswap_ushort((USHORT)tcp4Table->table[i].dwRemotePort);

            connections[index].State = tcp4Table->table[i].dwState;
            connections[index].ProcessId = (HANDLE)tcp4Table->table[i].dwOwningPid;
            memcpy(
                connections[index].OwnerInfo,
                tcp4Table->table[i].OwningModuleInfo,
                sizeof(ULONGLONG) * min(PH_NETWORK_OWNER_INFO_SIZE, TCPIP_OWNING_MODULE_SIZE)
                );

            index++;
        }
        PhFree(tcp4Table);
    }

    if (tcp6Table)
    {
        for (i = 0; i < tcp6Table->dwNumEntries; i++)
        {
            connections[index].ProtocolType = PH_TCP6_NETWORK_PROTOCOL;

            connections[index].LocalEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
            memcpy(connections[index].LocalEndpoint.Address.Ipv6, tcp6Table->table[i].ucLocalAddr, 16);
            connections[index].LocalEndpoint.Port = _byteswap_ushort((USHORT)tcp6Table->table[i].dwLocalPort);

            connections[index].RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
            memcpy(connections[index].RemoteEndpoint.Address.Ipv6, tcp6Table->table[i].ucRemoteAddr, 16);
            connections[index].RemoteEndpoint.Port = _byteswap_ushort((USHORT)tcp6Table->table[i].dwRemotePort);

            connections[index].State = tcp6Table->table[i].dwState;
            connections[index].ProcessId = (HANDLE)tcp6Table->table[i].dwOwningPid;
            memcpy(
                connections[index].OwnerInfo,
                tcp6Table->table[i].OwningModuleInfo,
                sizeof(ULONGLONG) * min(PH_NETWORK_OWNER_INFO_SIZE, TCPIP_OWNING_MODULE_SIZE)
                );

            index++;
        }
        PhFree(tcp6Table);
    }

    if (udp4Table)
    {
        for (i = 0; i < udp4Table->dwNumEntries; i++)
        {
            connections[index].ProtocolType = PH_UDP4_NETWORK_PROTOCOL;

            connections[index].LocalEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
            connections[index].LocalEndpoint.Address.Ipv4 = udp4Table->table[i].dwLocalAddr;
            connections[index].LocalEndpoint.Port = _byteswap_ushort((USHORT)udp4Table->table[i].dwLocalPort);

            connections[index].RemoteEndpoint.Address.Type = 0;

            connections[index].State = 0;
            connections[index].ProcessId = (HANDLE)udp4Table->table[i].dwOwningPid;
            memcpy(
                connections[index].OwnerInfo,
                udp4Table->table[i].OwningModuleInfo,
                sizeof(ULONGLONG) * min(PH_NETWORK_OWNER_INFO_SIZE, TCPIP_OWNING_MODULE_SIZE)
                );

            index++;
        }
        PhFree(udp4Table);
    }

    if (udp6Table)
    {
        for (i = 0; i < udp6Table->dwNumEntries; i++)
        {
            connections[index].ProtocolType = PH_UDP6_NETWORK_PROTOCOL;

            connections[index].LocalEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
            memcpy(connections[index].LocalEndpoint.Address.Ipv6, udp6Table->table[i].ucLocalAddr, 16);
            connections[index].LocalEndpoint.Port = _byteswap_ushort((USHORT)udp6Table->table[i].dwLocalPort);

            connections[index].RemoteEndpoint.Address.Type = 0;

            connections[index].State = 0;
            connections[index].ProcessId = (HANDLE)udp6Table->table[i].dwOwningPid;
            memcpy(
                connections[index].OwnerInfo,
                udp6Table->table[i].OwningModuleInfo,
                sizeof(ULONGLONG) * min(PH_NETWORK_OWNER_INFO_SIZE, TCPIP_OWNING_MODULE_SIZE)
                );

            index++;
        }
        PhFree(udp6Table);
    }

    *NumberOfConnections = count;
    *Connections = connections;

    return TRUE;
}
