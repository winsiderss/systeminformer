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

#define PH_NETPRV_PRIVATE
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

typedef struct _PH_NETWORK_ITEM_QUERY_DATA
{
    SLIST_ENTRY ListEntry;
    PPH_NETWORK_ITEM NetworkItem;

    PH_IP_ADDRESS Address;
    BOOLEAN Remote;
    PPH_STRING HostString;
} PH_NETWORK_ITEM_QUERY_DATA, *PPH_NETWORK_ITEM_QUERY_DATA;

typedef struct _PHP_RESOLVE_CACHE_ITEM
{
    PH_IP_ADDRESS Address;
    PPH_STRING HostString;
} PHP_RESOLVE_CACHE_ITEM, *PPHP_RESOLVE_CACHE_ITEM;

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

typedef int (WSAAPI *_WSAStartup)(
    __in WORD wVersionRequested,
    __out LPWSADATA lpWSAData
    );

typedef int (WSAAPI *_WSAGetLastError)();

typedef INT (WSAAPI *_GetNameInfoW)(
    __in_bcount(SockaddrLength) const SOCKADDR *pSockaddr,
    __in socklen_t SockaddrLength,
    __out_ecount_opt(NodeBufferSize) PWCHAR pNodeBuffer,
    __in DWORD NodeBufferSize,
    __out_ecount_opt(ServiceBufferSize) PWCHAR pServiceBuffer,
    __in DWORD ServiceBufferSize,
    __in INT Flags
    );

typedef struct hostent *(WSAAPI *_gethostbyaddr)(
    __in_bcount(len) const char *addr,
    __in int len,
    __in int type
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

BOOLEAN PhpResolveCacheHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpResolveCacheHashtableHashFunction(
    __in PVOID Entry
    );

BOOLEAN PhGetNetworkConnections(
    __out PPH_NETWORK_CONNECTION *Connections,
    __out PULONG NumberOfConnections
    );

PPH_OBJECT_TYPE PhNetworkItemType;

PPH_HASHTABLE PhNetworkHashtable;
PH_QUEUED_LOCK PhNetworkHashtableLock = PH_QUEUED_LOCK_INIT;

PH_CALLBACK_DECLARE(PhNetworkItemAddedEvent);
PH_CALLBACK_DECLARE(PhNetworkItemModifiedEvent);
PH_CALLBACK_DECLARE(PhNetworkItemRemovedEvent);
PH_CALLBACK_DECLARE(PhNetworkItemsUpdatedEvent);

PH_INITONCE PhNetworkProviderWorkQueueInitOnce = PH_INITONCE_INIT;
PH_WORK_QUEUE PhNetworkProviderWorkQueue;
SLIST_HEADER PhNetworkItemQueryListHead;

static PPH_HASHTABLE PhpResolveCacheHashtable;
static PH_QUEUED_LOCK PhpResolveCacheHashtableLock = PH_QUEUED_LOCK_INIT;

static BOOLEAN NetworkImportDone = FALSE;
static _GetExtendedTcpTable GetExtendedTcpTable_I;
static _GetExtendedUdpTable GetExtendedUdpTable_I;
static _WSAStartup WSAStartup_I;
static _WSAGetLastError WSAGetLastError_I;
static _GetNameInfoW GetNameInfoW_I;
static _gethostbyaddr gethostbyaddr_I;

BOOLEAN PhNetworkProviderInitialization()
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

    RtlInitializeSListHead(&PhNetworkItemQueryListHead);

    PhpResolveCacheHashtable = PhCreateHashtable(
        sizeof(PHP_RESOLVE_CACHE_ITEM),
        PhpResolveCacheHashtableCompareFunction,
        PhpResolveCacheHashtableHashFunction,
        20
        );

    return TRUE;
}

PPH_NETWORK_ITEM PhCreateNetworkItem()
{
    PPH_NETWORK_ITEM networkItem;

    if (!NT_SUCCESS(PhCreateObject(
        &networkItem,
        sizeof(PH_NETWORK_ITEM),
        0,
        PhNetworkItemType
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

    if (networkItem->ProcessName)
        PhDereferenceObject(networkItem->ProcessName);
    if (networkItem->OwnerName)
        PhDereferenceObject(networkItem->OwnerName);
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
        PhEqualIpEndpoint(&networkItem1->LocalEndpoint, &networkItem2->LocalEndpoint) &&
        PhEqualIpEndpoint(&networkItem1->RemoteEndpoint, &networkItem2->RemoteEndpoint) &&
        networkItem1->ProcessId == networkItem2->ProcessId;
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

    PhAcquireQueuedLockShared(&PhNetworkHashtableLock);

    networkItemPtr = (PPH_NETWORK_ITEM *)PhFindEntryHashtable(
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

    PhReleaseQueuedLockShared(&PhNetworkHashtableLock);

    return networkItem;
}

__assumeLocked VOID PhpRemoveNetworkItem(
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    PhRemoveEntryHashtable(PhNetworkHashtable, &NetworkItem);
    PhDereferenceObject(NetworkItem);
}

BOOLEAN PhpResolveCacheHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PPHP_RESOLVE_CACHE_ITEM cacheItem1 = *(PPHP_RESOLVE_CACHE_ITEM *)Entry1;
    PPHP_RESOLVE_CACHE_ITEM cacheItem2 = *(PPHP_RESOLVE_CACHE_ITEM *)Entry2;

    return PhEqualIpAddress(&cacheItem1->Address, &cacheItem2->Address);
}

ULONG NTAPI PhpResolveCacheHashtableHashFunction(
    __in PVOID Entry
    )
{
    PPHP_RESOLVE_CACHE_ITEM cacheItem = *(PPHP_RESOLVE_CACHE_ITEM *)Entry;

    return PhHashIpAddress(&cacheItem->Address);
}

__assumeLocked PPHP_RESOLVE_CACHE_ITEM PhpLookupResolveCacheItem(
    __in PPH_IP_ADDRESS Address
    )
{
    PHP_RESOLVE_CACHE_ITEM lookupCacheItem;
    PPHP_RESOLVE_CACHE_ITEM lookupCacheItemPtr = &lookupCacheItem;
    PPHP_RESOLVE_CACHE_ITEM *cacheItemPtr;

    // Construct a temporary cache item for the lookup.
    lookupCacheItem.Address = *Address;

    cacheItemPtr = (PPHP_RESOLVE_CACHE_ITEM *)PhFindEntryHashtable(
        PhpResolveCacheHashtable,
        &lookupCacheItemPtr
        );

    if (cacheItemPtr)
        return *cacheItemPtr;
    else
        return NULL;
}

PPH_STRING PhGetHostNameFromAddress(
    __in PPH_IP_ADDRESS Address
    )
{
    struct sockaddr_in ipv4Address;
    struct sockaddr_in6 ipv6Address;
    struct sockaddr *address;
    socklen_t length;
    PPH_STRING hostName;

    if (!GetNameInfoW_I)
        return NULL;

    if (Address->Type == PH_IPV4_NETWORK_TYPE)
    {
        ipv4Address.sin_family = AF_INET;
        ipv4Address.sin_port = 0;
        ipv4Address.sin_addr = Address->InAddr;
        address = (struct sockaddr *)&ipv4Address;
        length = sizeof(ipv4Address);
    }
    else if (Address->Type == PH_IPV6_NETWORK_TYPE)
    {
        ipv6Address.sin6_family = AF_INET6;
        ipv6Address.sin6_port = 0;
        ipv6Address.sin6_flowinfo = 0;
        ipv6Address.sin6_addr = Address->In6Addr;
        ipv6Address.sin6_scope_id = 0;
        address = (struct sockaddr *)&ipv6Address;
        length = sizeof(ipv6Address);
    }
    else
    {
        return NULL;
    }

    hostName = PhCreateStringEx(NULL, 128);

    if (GetNameInfoW_I(
        address,
        length,
        hostName->Buffer,
        hostName->Length + 2,
        NULL,
        0,
        NI_NAMEREQD
        ) != 0)
    {
        // Try with the maximum host name size.
        PhDereferenceObject(hostName);
        hostName = PhCreateStringEx(NULL, NI_MAXHOST * 2);

        if (GetNameInfoW_I(
            address,
            length,
            hostName->Buffer,
            hostName->Length + 2,
            NULL,
            0,
            NI_NAMEREQD
            ) != 0)
        {
            PhDereferenceObject(hostName);

            return NULL;
        }
    }

    PhTrimToNullTerminatorString(hostName);

    return hostName;
}

NTSTATUS PhpNetworkItemQueryWorker(
    __in PVOID Parameter
    )
{
    PPH_NETWORK_ITEM_QUERY_DATA data = (PPH_NETWORK_ITEM_QUERY_DATA)Parameter;
    PPH_STRING hostString;
    PPHP_RESOLVE_CACHE_ITEM cacheItem;

    // Last minute check of the cache.

    PhAcquireQueuedLockShared(&PhpResolveCacheHashtableLock);
    cacheItem = PhpLookupResolveCacheItem(&data->Address);
    PhReleaseQueuedLockShared(&PhpResolveCacheHashtableLock);

    if (!cacheItem)
    {
        hostString = PhGetHostNameFromAddress(&data->Address);

        if (hostString)
        {
            data->HostString = hostString;

            // Update the cache.

            PhAcquireQueuedLockExclusive(&PhpResolveCacheHashtableLock);

            cacheItem = PhpLookupResolveCacheItem(&data->Address);

            if (!cacheItem)
            {
                cacheItem = PhAllocate(sizeof(PHP_RESOLVE_CACHE_ITEM));
                cacheItem->Address = data->Address;
                cacheItem->HostString = hostString;
                PhReferenceObject(hostString);

                PhAddEntryHashtable(PhpResolveCacheHashtable, &cacheItem);
            }

            PhReleaseQueuedLockExclusive(&PhpResolveCacheHashtableLock);
        }
        else
        {
            dwlprintf(L"resolve failed, error %u", WSAGetLastError_I());
        }
    }
    else
    {
        PhReferenceObject(cacheItem->HostString);
        data->HostString = cacheItem->HostString;
    }

    RtlInterlockedPushEntrySList(&PhNetworkItemQueryListHead, &data->ListEntry);

    return STATUS_SUCCESS;
}

VOID PhpQueueNetworkItemQuery(
    __in PPH_NETWORK_ITEM NetworkItem,
    __in BOOLEAN Remote
    )
{
    PPH_NETWORK_ITEM_QUERY_DATA data;

    data = PhAllocate(sizeof(PH_NETWORK_ITEM_QUERY_DATA));
    memset(data, 0, sizeof(PH_NETWORK_ITEM_QUERY_DATA));
    data->NetworkItem = NetworkItem;
    data->Remote = Remote;

    if (Remote)
        data->Address = NetworkItem->RemoteEndpoint.Address;
    else
        data->Address = NetworkItem->LocalEndpoint.Address;

    PhReferenceObject(NetworkItem);

    if (PhBeginInitOnce(&PhNetworkProviderWorkQueueInitOnce))
    {
        PhInitializeWorkQueue(&PhNetworkProviderWorkQueue, 0, 3, 500);
        PhEndInitOnce(&PhNetworkProviderWorkQueueInitOnce);
    }

    PhQueueItemWorkQueue(&PhNetworkProviderWorkQueue, PhpNetworkItemQueryWorker, data);
}

VOID PhpUpdateNetworkItemOwner(
    __in PPH_NETWORK_ITEM NetworkItem,
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    if (*(PULONG64)NetworkItem->OwnerInfo)
    {
        PVOID serviceTag;
        PPH_STRING serviceName;

        // May change in the future...
        serviceTag = (PVOID)*(PULONG)NetworkItem->OwnerInfo;
        serviceName = PhGetServiceNameFromTag(NetworkItem->ProcessId, serviceTag);

        if (serviceName)
            PhSwapReference2(&NetworkItem->OwnerName, serviceName);
    }
}

VOID PhNetworkProviderUpdate(
    __in PVOID Object
    )
{
    PPH_NETWORK_CONNECTION connections;
    ULONG numberOfConnections;
    ULONG i;

    if (!NetworkImportDone)
    {
        WSADATA wsaData;

        LoadLibrary(L"iphlpapi.dll");
        GetExtendedTcpTable_I = PhGetProcAddress(L"iphlpapi.dll", "GetExtendedTcpTable");
        GetExtendedUdpTable_I = PhGetProcAddress(L"iphlpapi.dll", "GetExtendedUdpTable");
        LoadLibrary(L"ws2_32.dll");
        WSAStartup_I = PhGetProcAddress(L"ws2_32.dll", "WSAStartup");
        WSAGetLastError_I = PhGetProcAddress(L"ws2_32.dll", "WSAGetLastError");
        GetNameInfoW_I = PhGetProcAddress(L"ws2_32.dll", "GetNameInfoW");
        gethostbyaddr_I = PhGetProcAddress(L"ws2_32.dll", "gethostbyaddr");

        if (WSAStartup_I)
        {
            WSAStartup_I(MAKEWORD(2, 2), &wsaData);
        }

        NetworkImportDone = TRUE;
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
                    PhEqualIpEndpoint(&(*networkItem)->LocalEndpoint, &connections[i].LocalEndpoint) &&
                    PhEqualIpEndpoint(&(*networkItem)->RemoteEndpoint, &connections[i].RemoteEndpoint) &&
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

                PhAddItemList(connectionsToRemove, *networkItem);
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

    // Go through the queued network item query data.
    {
        PSLIST_ENTRY entry;
        PPH_NETWORK_ITEM_QUERY_DATA data;

        entry = RtlInterlockedFlushSList(&PhNetworkItemQueryListHead);

        while (entry)
        {
            data = CONTAINING_RECORD(entry, PH_NETWORK_ITEM_QUERY_DATA, ListEntry);
            entry = entry->Next;

            if (data->Remote)
                PhSwapReference2(&data->NetworkItem->RemoteHostString, data->HostString);
            else
                PhSwapReference2(&data->NetworkItem->LocalHostString, data->HostString);

            data->NetworkItem->JustResolved = TRUE;

            PhDereferenceObject(data->NetworkItem);
            PhFree(data);
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
            PPHP_RESOLVE_CACHE_ITEM cacheItem;
            PPH_PROCESS_ITEM processItem;

            // Network item not found, create it.

            networkItem = PhCreateNetworkItem();

            // Fill in basic information.
            networkItem->ProtocolType = connections[i].ProtocolType;
            networkItem->LocalEndpoint = connections[i].LocalEndpoint;
            networkItem->RemoteEndpoint = connections[i].RemoteEndpoint;
            networkItem->State = connections[i].State;
            networkItem->ProcessId = connections[i].ProcessId;
            memcpy(networkItem->OwnerInfo, connections[i].OwnerInfo, sizeof(ULONGLONG) * PH_NETWORK_OWNER_INFO_SIZE);

            // Format various strings.

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
                !PhIsNullIpAddress(&networkItem->RemoteEndpoint.Address)
                )
            {
                RtlIpv6AddressToString(&networkItem->RemoteEndpoint.Address.In6Addr, networkItem->RemoteAddressString);
            }

            if (networkItem->RemoteEndpoint.Address.Type != 0 && networkItem->RemoteEndpoint.Port != 0)
                PhPrintUInt32(networkItem->RemotePortString, networkItem->RemoteEndpoint.Port);

            // Get host names.

            // Local
            {
                PhAcquireQueuedLockShared(&PhpResolveCacheHashtableLock);
                cacheItem = PhpLookupResolveCacheItem(&networkItem->LocalEndpoint.Address);
                PhReleaseQueuedLockShared(&PhpResolveCacheHashtableLock);

                if (cacheItem)
                {
                    PhReferenceObject(cacheItem->HostString);
                    networkItem->LocalHostString = cacheItem->HostString;
                }
                else
                {
                    PhpQueueNetworkItemQuery(networkItem, FALSE);
                }
            }

            // Remote
            if (!PhIsNullIpAddress(&networkItem->RemoteEndpoint.Address))
            {
                PhAcquireQueuedLockShared(&PhpResolveCacheHashtableLock);
                cacheItem = PhpLookupResolveCacheItem(&networkItem->RemoteEndpoint.Address);
                PhReleaseQueuedLockShared(&PhpResolveCacheHashtableLock);

                if (cacheItem)
                {
                    PhReferenceObject(cacheItem->HostString);
                    networkItem->RemoteHostString = cacheItem->HostString;
                }
                else
                {
                    PhpQueueNetworkItemQuery(networkItem, TRUE);
                }
            }

            // Get process information.
            if (processItem = PhReferenceProcessItem(networkItem->ProcessId))
            {
                networkItem->ProcessName = processItem->ProcessName;
                PhReferenceObject(processItem->ProcessName);
                PhpUpdateNetworkItemOwner(networkItem, processItem);

                if (PhTestEvent(&processItem->Stage1Event))
                {
                    networkItem->ProcessIcon = processItem->SmallIcon;
                    networkItem->ProcessIconValid = TRUE;
                }

                PhDereferenceObject(processItem);
            }

            // Add the network item to the hashtable.
            PhAcquireQueuedLockExclusive(&PhNetworkHashtableLock);
            PhAddEntryHashtable(PhNetworkHashtable, &networkItem);
            PhReleaseQueuedLockExclusive(&PhNetworkHashtableLock);

            // Raise the network item added event.
            PhInvokeCallback(&PhNetworkItemAddedEvent, networkItem);
        }
        else
        {
            BOOLEAN modified = FALSE;
            PPH_PROCESS_ITEM processItem;

            if (networkItem->JustResolved)
                modified = TRUE;

            if (networkItem->State != connections[i].State)
            {
                networkItem->State = connections[i].State;
                modified = TRUE;
            }

            if (!networkItem->ProcessName || !networkItem->ProcessIconValid)
            {
                if (processItem = PhReferenceProcessItem(networkItem->ProcessId))
                {
                    if (!networkItem->ProcessName)
                    {
                        networkItem->ProcessName = processItem->ProcessName;
                        PhReferenceObject(processItem->ProcessName);
                        PhpUpdateNetworkItemOwner(networkItem, processItem);
                        modified = TRUE;
                    }

                    if (!networkItem->ProcessIconValid && PhTestEvent(&processItem->Stage1Event))
                    {
                        networkItem->ProcessIcon = processItem->SmallIcon;
                        networkItem->ProcessIconValid = TRUE;
                        modified = TRUE;
                    }

                    PhDereferenceObject(processItem);
                }
            }

            networkItem->JustResolved = FALSE;

            if (modified)
            {
                // Raise the network item modified event.
                PhInvokeCallback(&PhNetworkItemModifiedEvent, networkItem);
            }

            PhDereferenceObject(networkItem);
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

PWSTR PhGetTcpStateName(
    __in ULONG State
    )
{
    switch (State)
    {
    case MIB_TCP_STATE_CLOSED:
        return L"Closed";
    case MIB_TCP_STATE_LISTEN:
        return L"Listen";
    case MIB_TCP_STATE_SYN_SENT:
        return L"SYN Sent";
    case MIB_TCP_STATE_SYN_RCVD:
        return L"SYN Received";
    case MIB_TCP_STATE_ESTAB:
        return L"Established";
    case MIB_TCP_STATE_FIN_WAIT1:
        return L"FIN Wait 1";
    case MIB_TCP_STATE_FIN_WAIT2:
        return L"FIN Wait 2";
    case MIB_TCP_STATE_CLOSE_WAIT:
        return L"Close Wait";
    case MIB_TCP_STATE_CLOSING:
        return L"Closing";
    case MIB_TCP_STATE_LAST_ACK:
        return L"Last ACK";
    case MIB_TCP_STATE_TIME_WAIT:
        return L"Time Wait";
    case MIB_TCP_STATE_DELETE_TCB:
        return L"Delete TCB";
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
