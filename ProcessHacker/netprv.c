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

#include <phapp.h>
#include <phplug.h>
#include <netprv.h>
#include <svcsup.h>
#include <workqueue.h>

#include <extmgri.h>
#include <procprv.h>

typedef struct _PH_NETWORK_CONNECTION
{
    ULONG ProtocolType;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;
    ULONG State;
    HANDLE ProcessId;
    LARGE_INTEGER CreateTime;
    ULONGLONG OwnerInfo[PH_NETWORK_OWNER_INFO_SIZE];
    ULONG LocalScopeId; // Ipv6
    ULONG RemoteScopeId; // Ipv6
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

VOID NTAPI PhpNetworkItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

BOOLEAN PhpNetworkHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG NTAPI PhpNetworkHashtableHashFunction(
    _In_ PVOID Entry
    );

BOOLEAN PhpResolveCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG NTAPI PhpResolveCacheHashtableHashFunction(
    _In_ PVOID Entry
    );

BOOLEAN PhGetNetworkConnections(
    _Out_ PPH_NETWORK_CONNECTION *Connections,
    _Out_ PULONG NumberOfConnections
    );

PPH_OBJECT_TYPE PhNetworkItemType = NULL;
PPH_HASHTABLE PhNetworkHashtable = NULL;
PH_QUEUED_LOCK PhNetworkHashtableLock = PH_QUEUED_LOCK_INIT;

PH_INITONCE PhNetworkProviderWorkQueueInitOnce = PH_INITONCE_INIT;
PH_WORK_QUEUE PhNetworkProviderWorkQueue;
SLIST_HEADER PhNetworkItemQueryListHead;

BOOLEAN PhEnableNetworkProviderResolve = TRUE;
static BOOLEAN NetworkImportDone = FALSE;
static PPH_HASHTABLE PhpResolveCacheHashtable = NULL;
static PH_QUEUED_LOCK PhpResolveCacheHashtableLock = PH_QUEUED_LOCK_INIT;

BOOLEAN PhNetworkProviderInitialization(
    VOID
    )
{
    PhNetworkItemType = PhCreateObjectType(L"NetworkItem", 0, PhpNetworkItemDeleteProcedure);
    PhNetworkHashtable = PhCreateHashtable(
        sizeof(PPH_NETWORK_ITEM),
        PhpNetworkHashtableEqualFunction,
        PhpNetworkHashtableHashFunction,
        40
        );

    RtlInitializeSListHead(&PhNetworkItemQueryListHead);

    PhpResolveCacheHashtable = PhCreateHashtable(
        sizeof(PHP_RESOLVE_CACHE_ITEM),
        PhpResolveCacheHashtableEqualFunction,
        PhpResolveCacheHashtableHashFunction,
        20
        );

    return TRUE;
}

PPH_NETWORK_ITEM PhCreateNetworkItem(
    VOID
    )
{
    PPH_NETWORK_ITEM networkItem;

    networkItem = PhCreateObject(
        PhEmGetObjectSize(EmNetworkItemType, sizeof(PH_NETWORK_ITEM)),
        PhNetworkItemType
        );
    memset(networkItem, 0, sizeof(PH_NETWORK_ITEM));
    PhEmCallObjectOperation(EmNetworkItemType, networkItem, EmObjectCreate);

    return networkItem;
}

VOID NTAPI PhpNetworkItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)Object;

    PhEmCallObjectOperation(EmNetworkItemType, networkItem, EmObjectDelete);

    if (networkItem->ProcessName)
        PhDereferenceObject(networkItem->ProcessName);
    if (networkItem->OwnerName)
        PhDereferenceObject(networkItem->OwnerName);
    if (networkItem->LocalHostString)
        PhDereferenceObject(networkItem->LocalHostString);
    if (networkItem->RemoteHostString)
        PhDereferenceObject(networkItem->RemoteHostString);

    // NOTE: Dereferencing the ProcessItem will destroy the NetworkItem->ProcessIcon handle.
    if (networkItem->ProcessItem)
        PhDereferenceObject(networkItem->ProcessItem);
}

BOOLEAN PhpNetworkHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
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
    _In_ PVOID Entry
    )
{
    PPH_NETWORK_ITEM networkItem = *(PPH_NETWORK_ITEM *)Entry;

    return
        networkItem->ProtocolType ^
        PhHashIpEndpoint(&networkItem->LocalEndpoint) ^
        PhHashIpEndpoint(&networkItem->RemoteEndpoint) ^
        HandleToUlong(networkItem->ProcessId);
}

PPH_NETWORK_ITEM PhReferenceNetworkItem(
    _In_ ULONG ProtocolType,
    _In_ PPH_IP_ENDPOINT LocalEndpoint,
    _In_ PPH_IP_ENDPOINT RemoteEndpoint,
    _In_ HANDLE ProcessId
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

VOID PhpRemoveNetworkItem(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PhRemoveEntryHashtable(PhNetworkHashtable, &NetworkItem);
    PhDereferenceObject(NetworkItem);
}

BOOLEAN PhpResolveCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPHP_RESOLVE_CACHE_ITEM cacheItem1 = *(PPHP_RESOLVE_CACHE_ITEM *)Entry1;
    PPHP_RESOLVE_CACHE_ITEM cacheItem2 = *(PPHP_RESOLVE_CACHE_ITEM *)Entry2;

    return PhEqualIpAddress(&cacheItem1->Address, &cacheItem2->Address);
}

ULONG NTAPI PhpResolveCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPHP_RESOLVE_CACHE_ITEM cacheItem = *(PPHP_RESOLVE_CACHE_ITEM *)Entry;

    return PhHashIpAddress(&cacheItem->Address);
}

PPHP_RESOLVE_CACHE_ITEM PhpLookupResolveCacheItem(
    _In_ PPH_IP_ADDRESS Address
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
    _In_ PPH_IP_ADDRESS Address
    )
{
    struct sockaddr_in ipv4Address;
    struct sockaddr_in6 ipv6Address;
    struct sockaddr *address;
    socklen_t length;
    PPH_STRING hostName;

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

    if (GetNameInfo(
        address,
        length,
        hostName->Buffer,
        (ULONG)hostName->Length / sizeof(WCHAR) + 1,
        NULL,
        0,
        NI_NAMEREQD
        ) != 0)
    {
        // Try with the maximum host name size.
        PhDereferenceObject(hostName);
        hostName = PhCreateStringEx(NULL, NI_MAXHOST * sizeof(WCHAR));

        if (GetNameInfo(
            address,
            length,
            hostName->Buffer,
            (ULONG)hostName->Length / sizeof(WCHAR) + 1,
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
    _In_ PVOID Parameter
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
    _In_ PPH_NETWORK_ITEM NetworkItem,
    _In_ BOOLEAN Remote
    )
{
    PPH_NETWORK_ITEM_QUERY_DATA data;

    if (!PhEnableNetworkProviderResolve)
        return;

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
    _In_ PPH_NETWORK_ITEM NetworkItem,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    if (*(PULONG64)NetworkItem->OwnerInfo)
    {
        PVOID serviceTag;
        PPH_STRING serviceName;

        // May change in the future...
        serviceTag = UlongToPtr(*(PULONG)NetworkItem->OwnerInfo);
        serviceName = PhGetServiceNameFromTag(NetworkItem->ProcessId, serviceTag);

        if (serviceName)
            PhMoveReference(&NetworkItem->OwnerName, serviceName);
    }
}

VOID PhFlushNetworkQueryData(
    VOID
    )
{
    PSLIST_ENTRY entry;
    PPH_NETWORK_ITEM_QUERY_DATA data;

    if (!RtlFirstEntrySList(&PhNetworkItemQueryListHead))
        return;

    entry = RtlInterlockedFlushSList(&PhNetworkItemQueryListHead);

    while (entry)
    {
        data = CONTAINING_RECORD(entry, PH_NETWORK_ITEM_QUERY_DATA, ListEntry);
        entry = entry->Next;

        if (data->Remote)
            PhMoveReference(&data->NetworkItem->RemoteHostString, data->HostString);
        else
            PhMoveReference(&data->NetworkItem->LocalHostString, data->HostString);

        data->NetworkItem->JustResolved = TRUE;

        PhDereferenceObject(data->NetworkItem);
        PhFree(data);
    }
}

VOID PhNetworkProviderUpdate(
    _In_ PVOID Object
    )
{
    PPH_NETWORK_CONNECTION connections;
    ULONG numberOfConnections;
    ULONG i;

    if (!NetworkImportDone)
    {
        WSADATA wsaData;

        // Make sure WSA is initialized.
        WSAStartup(WINSOCK_VERSION, &wsaData);
        NetworkImportDone = TRUE;
    }

    if (!PhGetNetworkConnections(&connections, &numberOfConnections))
        return;

    // Look for closed connections.
    {
        PPH_LIST connectionsToRemove = NULL;
        PH_HASHTABLE_ENUM_CONTEXT enumContext;
        PPH_NETWORK_ITEM *networkItem;

        PhBeginEnumHashtable(PhNetworkHashtable, &enumContext);

        while (networkItem = PhNextEnumHashtable(&enumContext))
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
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackNetworkProviderRemovedEvent), *networkItem);

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
    PhFlushNetworkQueryData();

    // Look for new network connections and update existing ones.
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
            networkItem->CreateTime = connections[i].CreateTime;
            memcpy(networkItem->OwnerInfo, connections[i].OwnerInfo, sizeof(ULONGLONG) * PH_NETWORK_OWNER_INFO_SIZE);
            networkItem->LocalScopeId = connections[i].LocalScopeId;
            networkItem->RemoteScopeId = connections[i].RemoteScopeId;

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
                networkItem->ProcessItem = processItem;
                networkItem->ProcessName = PhReferenceObject(processItem->ProcessName);
                networkItem->SubsystemProcess = !!processItem->IsSubsystemProcess;
                PhpUpdateNetworkItemOwner(networkItem, processItem);

                if (PhTestEvent(&processItem->Stage1Event))
                {
                    networkItem->ProcessIcon = processItem->SmallIcon;
                    networkItem->ProcessIconValid = TRUE;
                }

                // NOTE: We dereference processItem in PhpNetworkItemDeleteProcedure. (dmex)
            }
            else
            {
                HANDLE processHandle;
                PPH_STRING fileName;
                PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

                // HACK HACK HACK
                // WSL subsystem processes (e.g. nginx) create sockets, clone/fork themselves, duplicate the socket into the child process and then terminate.
                // The socket handle remains valid and in-use by the child process BUT the socket continues returning the PID of the exited process???
                // Fixing this causes a major performance problem; If we have 100,000 sockets then on previous versions of Windows we would only need 2 system calls maximum
                // (for the process list) to identify the owner of every socket but now we need to make 4 system calls for every_last_socket totaling 400,000 system calls... great.
                if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, networkItem->ProcessId)))
                {
                    if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(processHandle, &basicInfo)))
                    {
                        networkItem->SubsystemProcess = !!basicInfo.IsSubsystemProcess;
                    }

                    if (NT_SUCCESS(PhGetProcessImageFileName(processHandle, &fileName)))
                    {
                        PhMoveReference(&networkItem->ProcessName, PhGetBaseName(fileName));
                    }
                
                    NtClose(processHandle);
                }

                networkItem->UnknownProcess = TRUE;
            }

            // Add the network item to the hashtable.
            PhAcquireQueuedLockExclusive(&PhNetworkHashtableLock);
            PhAddEntryHashtable(PhNetworkHashtable, &networkItem);
            PhReleaseQueuedLockExclusive(&PhNetworkHashtableLock);

            // Raise the network item added event.
            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackNetworkProviderAddedEvent), networkItem);
        }
        else
        {
            BOOLEAN modified = FALSE;

            if (InterlockedExchange(&networkItem->JustResolved, 0) != 0)
                modified = TRUE;

            if (networkItem->State != connections[i].State)
            {
                networkItem->State = connections[i].State;
                modified = TRUE;
            }

            if (!networkItem->ProcessIconValid)
            {
                if (!networkItem->ProcessItem)
                {
                    networkItem->ProcessItem = PhReferenceProcessItem(networkItem->ProcessId);
                    // NOTE: We dereference processItem in PhpNetworkItemDeleteProcedure. (dmex)
                }

                if (networkItem->ProcessItem)
                {
                    if (!networkItem->ProcessName)
                    {
                        networkItem->ProcessName = PhReferenceObject(networkItem->ProcessItem->ProcessName);
                        PhpUpdateNetworkItemOwner(networkItem, networkItem->ProcessItem);
                        modified = TRUE;
                    }

                    if (!networkItem->ProcessIconValid && PhTestEvent(&networkItem->ProcessItem->Stage1Event))
                    {
                        networkItem->ProcessIcon = networkItem->ProcessItem->SmallIcon;
                        networkItem->ProcessIconValid = TRUE;
                        modified = TRUE;
                    }
                }
            }

            if (modified)
            {
                // Raise the network item modified event.
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackNetworkProviderModifiedEvent), networkItem);
            }

            PhDereferenceObject(networkItem);
        }
    }

    PhFree(connections);

    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackNetworkProviderUpdatedEvent), NULL);
}

PWSTR PhGetProtocolTypeName(
    _In_ ULONG ProtocolType
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
    _In_ ULONG State
    )
{
    switch (State)
    {
    case MIB_TCP_STATE_CLOSED:
        return L"Closed";
    case MIB_TCP_STATE_LISTEN:
        return L"Listen";
    case MIB_TCP_STATE_SYN_SENT:
        return L"SYN sent";
    case MIB_TCP_STATE_SYN_RCVD:
        return L"SYN received";
    case MIB_TCP_STATE_ESTAB:
        return L"Established";
    case MIB_TCP_STATE_FIN_WAIT1:
        return L"FIN wait 1";
    case MIB_TCP_STATE_FIN_WAIT2:
        return L"FIN wait 2";
    case MIB_TCP_STATE_CLOSE_WAIT:
        return L"Close wait";
    case MIB_TCP_STATE_CLOSING:
        return L"Closing";
    case MIB_TCP_STATE_LAST_ACK:
        return L"Last ACK";
    case MIB_TCP_STATE_TIME_WAIT:
        return L"Time wait";
    case MIB_TCP_STATE_DELETE_TCB:
        return L"Delete TCB";
    default:
        return L"Unknown";
    }
}

BOOLEAN PhGetNetworkConnections(
    _Out_ PPH_NETWORK_CONNECTION *Connections,
    _Out_ PULONG NumberOfConnections
    )
{
    PVOID table;
    ULONG tableSize;
    PMIB_TCPTABLE_OWNER_MODULE tcp4Table;
    PMIB_UDPTABLE_OWNER_MODULE udp4Table;
    PMIB_TCP6TABLE_OWNER_MODULE tcp6Table;
    PMIB_UDP6TABLE_OWNER_MODULE udp6Table;
    ULONG count = 0;
    ULONG i;
    ULONG index = 0;
    PPH_NETWORK_CONNECTION connections;

    // TCP IPv4

    tableSize = 0;
    GetExtendedTcpTable(NULL, &tableSize, FALSE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0);
    table = PhAllocate(tableSize);

    if (GetExtendedTcpTable(table, &tableSize, FALSE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0) == NO_ERROR)
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
    GetExtendedTcpTable(NULL, &tableSize, FALSE, AF_INET6, TCP_TABLE_OWNER_MODULE_ALL, 0);

    table = PhAllocate(tableSize);

    if (GetExtendedTcpTable(table, &tableSize, FALSE, AF_INET6, TCP_TABLE_OWNER_MODULE_ALL, 0) == NO_ERROR)
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
    GetExtendedUdpTable(NULL, &tableSize, FALSE, AF_INET, UDP_TABLE_OWNER_MODULE, 0);
    table = PhAllocate(tableSize);

    if (GetExtendedUdpTable(table, &tableSize, FALSE, AF_INET, UDP_TABLE_OWNER_MODULE, 0) == NO_ERROR)
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
    GetExtendedUdpTable(NULL, &tableSize, FALSE, AF_INET6, UDP_TABLE_OWNER_MODULE, 0);
    table = PhAllocate(tableSize);

    if (GetExtendedUdpTable(table, &tableSize, FALSE, AF_INET6, UDP_TABLE_OWNER_MODULE, 0) == NO_ERROR)
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
            connections[index].ProcessId = UlongToHandle(tcp4Table->table[i].dwOwningPid);
            connections[index].CreateTime = tcp4Table->table[i].liCreateTimestamp;
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
            connections[index].ProcessId = UlongToHandle(tcp6Table->table[i].dwOwningPid);
            connections[index].CreateTime = tcp6Table->table[i].liCreateTimestamp;
            memcpy(
                connections[index].OwnerInfo,
                tcp6Table->table[i].OwningModuleInfo,
                sizeof(ULONGLONG) * min(PH_NETWORK_OWNER_INFO_SIZE, TCPIP_OWNING_MODULE_SIZE)
                );

            connections[index].LocalScopeId = tcp6Table->table[i].dwLocalScopeId;
            connections[index].RemoteScopeId = tcp6Table->table[i].dwRemoteScopeId;

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
            connections[index].ProcessId = UlongToHandle(udp4Table->table[i].dwOwningPid);
            connections[index].CreateTime = udp4Table->table[i].liCreateTimestamp;
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
            connections[index].ProcessId = UlongToHandle(udp6Table->table[i].dwOwningPid);
            connections[index].CreateTime = udp6Table->table[i].liCreateTimestamp;
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
