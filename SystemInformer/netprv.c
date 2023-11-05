/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     evilpie 2010
 *     dmex    2016-2023
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <phsettings.h>
#include <extmgri.h>
#include <mapldr.h>
#include <netprv.h>
#include <procprv.h>
#include <svcsup.h>
#include <workqueue.h>
#include <hvsocketcontrol.h>

typedef struct _PH_NETWORK_CONNECTION
{
    ULONG ProtocolType;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;
    MIB_TCP_STATE State;
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
BOOLEAN PhEnableNetworkBoundConnections = TRUE;
ULONG PhNetworkProviderFlagsMask = 0;
static PPH_HASHTABLE PhpResolveCacheHashtable = NULL;
static PH_QUEUED_LOCK PhpResolveCacheHashtableLock = PH_QUEUED_LOCK_INIT;

static BOOLEAN NetworkImportDone = FALSE;
static _GetExtendedTcpTable GetExtendedTcpTable_I = NULL;
static _GetExtendedUdpTable GetExtendedUdpTable_I = NULL;
static _InternalGetBoundTcpEndpointTable GetBoundTcpEndpointTable_I = NULL;
static _InternalGetBoundTcp6EndpointTable GetBoundTcp6EndpointTable_I = NULL;

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

    PhInitializeSListHead(&PhNetworkItemQueryListHead);

    PhpResolveCacheHashtable = PhCreateHashtable(
        sizeof(PPHP_RESOLVE_CACHE_ITEM),
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

    if (networkItem->LocalAddressString)
        PhDereferenceObject(networkItem->LocalAddressString);
    if (networkItem->RemoteAddressString)
        PhDereferenceObject(networkItem->RemoteAddressString);
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
        (HandleToUlong(networkItem->ProcessId) / 4);
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

/**
 * Enumerates the network items.
 *
 * \param NetworkItems A variable which receives an array of pointers to network items. You must
 * free the buffer with PhFree() when you no longer need it.
 * \param NumberOfNetworkItems A variable which receives the number of network items returned in
 * \a ProcessItems.
 */
VOID PhEnumNetworkItems(
    _Out_opt_ PPH_NETWORK_ITEM **NetworkItems,
    _Out_ PULONG NumberOfNetworkItems
    )
{
    PPH_NETWORK_ITEM* networkItems;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_NETWORK_ITEM* networkItem;
    ULONG numberOfNetworkItems;
    ULONG count = 0;

    PhAcquireQueuedLockShared(&PhNetworkHashtableLock);

    PhBeginEnumHashtable(PhNetworkHashtable, &enumContext);

    while (networkItem = PhNextEnumHashtable(&enumContext))
    {
        count++;
    }

    if (count == 0)
    {
        PhReleaseQueuedLockShared(&PhNetworkHashtableLock);

        if (NetworkItems) *NetworkItems = NULL;
        *NumberOfNetworkItems = count;
        return;
    }

    numberOfNetworkItems = count;
    networkItems = PhAllocate(sizeof(PPH_NETWORK_ITEM) * numberOfNetworkItems);
    count = 0;

    PhBeginEnumHashtable(PhNetworkHashtable, &enumContext);

    while (networkItem = PhNextEnumHashtable(&enumContext))
    {
        PhReferenceObject((*networkItem));
        networkItems[count++] = (*networkItem);
    }

    PhReleaseQueuedLockShared(&PhNetworkHashtableLock);

    *NetworkItems = networkItems;
    *NumberOfNetworkItems = numberOfNetworkItems;
}

VOID PhEnumNetworkItemsByProcessId(
    _In_opt_ HANDLE ProcessId,
    _Out_opt_ PPH_NETWORK_ITEM** NetworkItems,
    _Out_ PULONG NumberOfNetworkItems
    )
{
    PPH_NETWORK_ITEM* networkItems;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_NETWORK_ITEM* networkItem;
    ULONG numberOfNetworkItems;
    ULONG count = 0;

    PhAcquireQueuedLockShared(&PhNetworkHashtableLock);

    PhBeginEnumHashtable(PhNetworkHashtable, &enumContext);

    while (networkItem = PhNextEnumHashtable(&enumContext))
    {
        if ((*networkItem)->ProcessId == ProcessId)
        {
            count++;
        }
    }

    if (count == 0)
    {
        PhReleaseQueuedLockShared(&PhNetworkHashtableLock);

        if (NetworkItems) *NetworkItems = NULL;
        *NumberOfNetworkItems = count;
        return;
    }

    numberOfNetworkItems = count;
    networkItems = PhAllocate(sizeof(PPH_NETWORK_ITEM) * numberOfNetworkItems);
    count = 0;

    PhBeginEnumHashtable(PhNetworkHashtable, &enumContext);

    while (networkItem = PhNextEnumHashtable(&enumContext))
    {
        if ((*networkItem)->ProcessId == ProcessId)
        {
            PhReferenceObject((*networkItem));
            networkItems[count++] = (*networkItem);
        }
    }

    PhReleaseQueuedLockShared(&PhNetworkHashtableLock);

    *NetworkItems = networkItems;
    *NumberOfNetworkItems = numberOfNetworkItems;
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

//PPH_STRING PhGetHostNameFromAddress(
//    _In_ PPH_IP_ADDRESS Address
//    )
//{
//    SOCKADDR_IN ipv4Address;
//    SOCKADDR_IN6 ipv6Address;
//    PSOCKADDR address;
//    socklen_t length;
//    PPH_STRING hostName;
//
//    if (Address->Type == PH_IPV4_NETWORK_TYPE)
//    {
//        ipv4Address.sin_family = AF_INET;
//        ipv4Address.sin_port = 0;
//        ipv4Address.sin_addr = Address->InAddr;
//        address = (PSOCKADDR)&ipv4Address;
//        length = sizeof(ipv4Address);
//    }
//    else if (Address->Type == PH_IPV6_NETWORK_TYPE)
//    {
//        ipv6Address.sin6_family = AF_INET6;
//        ipv6Address.sin6_port = 0;
//        ipv6Address.sin6_flowinfo = 0;
//        ipv6Address.sin6_addr = Address->In6Addr;
//        ipv6Address.sin6_scope_id = 0;
//        address = (PSOCKADDR)&ipv6Address;
//        length = sizeof(ipv6Address);
//    }
//    else
//    {
//        return NULL;
//    }
//
//    hostName = PhCreateStringEx(NULL, 128);
//
//    if (GetNameInfo(
//        address,
//        length,
//        hostName->Buffer,
//        (ULONG)hostName->Length / sizeof(WCHAR) + 1,
//        NULL,
//        0,
//        NI_NAMEREQD
//        ) != 0)
//    {
//        // Try with the maximum host name size.
//        PhDereferenceObject(hostName);
//        hostName = PhCreateStringEx(NULL, NI_MAXHOST * sizeof(WCHAR));
//
//        if (GetNameInfo(
//            address,
//            length,
//            hostName->Buffer,
//            (ULONG)hostName->Length / sizeof(WCHAR) + 1,
//            NULL,
//            0,
//            NI_NAMEREQD
//            ) != 0)
//        {
//            PhDereferenceObject(hostName);
//
//            return NULL;
//        }
//    }
//
//    PhTrimToNullTerminatorString(hostName);
//
//    return hostName;
//}

PPH_STRING PhpGetDnsReverseNameFromAddress(
    _In_ PPH_IP_ADDRESS Address
    )
{
#define IP4_REVERSE_DOMAIN_STRING_LENGTH (IP4_ADDRESS_STRING_LENGTH + sizeof(DNS_IP4_REVERSE_DOMAIN_STRING_W) + 1)
#define IP6_REVERSE_DOMAIN_STRING_LENGTH (IP6_ADDRESS_STRING_LENGTH + sizeof(DNS_IP6_REVERSE_DOMAIN_STRING_W) + 1)

    switch (Address->Type)
    {
    case PH_IPV4_NETWORK_TYPE:
        {
            static PH_STRINGREF reverseLookupDomainNameSr = PH_STRINGREF_INIT(DNS_IP4_REVERSE_DOMAIN_STRING);
            PH_FORMAT format[9];
            SIZE_T returnLength;
            WCHAR reverseNameBuffer[IP4_REVERSE_DOMAIN_STRING_LENGTH];

            PhInitFormatU(&format[0], Address->InAddr.s_impno);
            PhInitFormatC(&format[1], L'.');
            PhInitFormatU(&format[2], Address->InAddr.s_lh);
            PhInitFormatC(&format[3], L'.');
            PhInitFormatU(&format[4], Address->InAddr.s_host);
            PhInitFormatC(&format[5], L'.');
            PhInitFormatU(&format[6], Address->InAddr.s_net);
            PhInitFormatC(&format[7], L'.');
            PhInitFormatSR(&format[8], reverseLookupDomainNameSr);

            if (PhFormatToBuffer(
                format,
                RTL_NUMBER_OF(format),
                reverseNameBuffer,
                sizeof(reverseNameBuffer),
                &returnLength
                ))
            {
                PH_STRINGREF reverseNameString;

                reverseNameString.Buffer = reverseNameBuffer;
                reverseNameString.Length = returnLength - sizeof(UNICODE_NULL);

                return PhCreateString2(&reverseNameString);
            }
            else
            {
                return PhFormat(format, RTL_NUMBER_OF(format), IP4_REVERSE_DOMAIN_STRING_LENGTH);
            }
        }
        break;
    case PH_IPV6_NETWORK_TYPE:
        {
            static PH_STRINGREF reverseLookupDomainNameSr = PH_STRINGREF_INIT(DNS_IP6_REVERSE_DOMAIN_STRING);
            PH_STRING_BUILDER stringBuilder;

            // DNS_MAX_IP6_REVERSE_NAME_LENGTH
            PhInitializeStringBuilder(&stringBuilder, IP6_REVERSE_DOMAIN_STRING_LENGTH);

            for (INT i = sizeof(IN6_ADDR) - 1; i >= 0; i--)
            {
                PH_FORMAT format[4];
                SIZE_T returnLength;
                WCHAR reverseNameBuffer[PH_INT32_STR_LEN_1];

                PhInitFormatX(&format[0], Address->In6Addr.s6_addr[i] & 0xF);
                PhInitFormatC(&format[1], L'.');
                PhInitFormatX(&format[2], (Address->In6Addr.s6_addr[i] >> 4) & 0xF);
                PhInitFormatC(&format[3], L'.');

                if (PhFormatToBuffer(
                    format,
                    RTL_NUMBER_OF(format),
                    reverseNameBuffer,
                    sizeof(reverseNameBuffer),
                    &returnLength
                    ))
                {
                    PH_STRINGREF reverseNameString;

                    reverseNameString.Buffer = reverseNameBuffer;
                    reverseNameString.Length = returnLength - sizeof(UNICODE_NULL);

                    PhAppendStringBuilder(&stringBuilder, &reverseNameString);
                }
                else
                {
                    PhAppendFormatStringBuilder(
                        &stringBuilder,
                        L"%hhx.%hhx.",
                        Address->In6Addr.s6_addr[i] & 0xF,
                        (Address->In6Addr.s6_addr[i] >> 4) & 0xF
                        );
                }
            }

            PhAppendStringBuilder(&stringBuilder, &reverseLookupDomainNameSr);

            return PhFinalStringBuilderString(&stringBuilder);
        }
        break;
    }

    return NULL;
}

PPH_STRING PhGetHostNameFromAddressEx(
    _In_ PPH_IP_ADDRESS Address
    )
{
    BOOLEAN dnsLocalQuery = FALSE;
    PPH_STRING dnsHostNameString = NULL;
    PPH_STRING dnsReverseNameString;
    PDNS_RECORD dnsRecordList;

    switch (Address->Type)
    {
    case PH_IPV4_NETWORK_TYPE:
        {
            if (IN4_IS_ADDR_UNSPECIFIED(&Address->InAddr))
                return NULL;

            if (IN4_IS_ADDR_LOOPBACK(&Address->InAddr) ||
                IN4_IS_ADDR_BROADCAST(&Address->InAddr) ||
                IN4_IS_ADDR_MULTICAST(&Address->InAddr) ||
                IN4_IS_ADDR_LINKLOCAL(&Address->InAddr) ||
                IN4_IS_ADDR_MC_LINKLOCAL(&Address->InAddr) ||
                IN4_IS_ADDR_RFC1918(&Address->InAddr))
            {
                dnsLocalQuery = TRUE;
            }
        }
        break;
    case PH_IPV6_NETWORK_TYPE:
        {
            if (IN6_IS_ADDR_UNSPECIFIED(&Address->In6Addr))
                return NULL;

            if (IN6_IS_ADDR_LOOPBACK(&Address->In6Addr) ||
                IN6_IS_ADDR_MULTICAST(&Address->In6Addr) ||
                IN6_IS_ADDR_LINKLOCAL(&Address->In6Addr) ||
                IN6_IS_ADDR_MC_LINKLOCAL(&Address->In6Addr))
            {
                dnsLocalQuery = TRUE;
            }
        }
        break;
    }

    if (!(dnsReverseNameString = PhpGetDnsReverseNameFromAddress(Address)))
        return NULL;

    if (PhEnableNetworkResolveDoHSupport && !dnsLocalQuery)
    {
        dnsRecordList = PhDnsQuery(
            NULL,
            dnsReverseNameString->Buffer,
            DNS_TYPE_PTR
            );
    }
    else
    {
        dnsRecordList = PhDnsQuery2(
            NULL,
            dnsReverseNameString->Buffer,
            DNS_TYPE_PTR,
            DNS_QUERY_NO_HOSTS_FILE // DNS_QUERY_BYPASS_CACHE
            );
    }

    if (dnsRecordList)
    {
        for (PDNS_RECORD dnsRecord = dnsRecordList; dnsRecord; dnsRecord = dnsRecord->pNext)
        {
            if (dnsRecord->wType == DNS_TYPE_PTR)
            {
                dnsHostNameString = PhCreateString(dnsRecord->Data.PTR.pNameHost); // Return the first result (dmex)
                break;
            }
        }

        PhDnsFree(dnsRecordList);
    }

    PhDereferenceObject(dnsReverseNameString);

    return dnsHostNameString;
}

VOID PhFlushNetworkItemResolveCache(
    VOID
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPHP_RESOLVE_CACHE_ITEM* entry;

    if (!PhpResolveCacheHashtable)
        return;

    PhAcquireQueuedLockExclusive(&PhpResolveCacheHashtableLock);

    PhBeginEnumHashtable(PhpResolveCacheHashtable, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
    {
        if ((*entry)->HostString)
        {
            PhDereferenceObject((*entry)->HostString);
        }

        PhFree((*entry));
    }

    PhDereferenceObject(PhpResolveCacheHashtable);
    PhpResolveCacheHashtable = PhCreateHashtable(
        sizeof(PPHP_RESOLVE_CACHE_ITEM),
        PhpResolveCacheHashtableEqualFunction,
        PhpResolveCacheHashtableHashFunction,
        20
        );

    PhReleaseQueuedLockExclusive(&PhpResolveCacheHashtableLock);
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
        hostString = PhGetHostNameFromAddressEx(&data->Address);

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
    {
        if (Remote)
            NetworkItem->RemoteHostnameResolved = TRUE;
        else
            NetworkItem->LocalHostnameResolved = TRUE;
        return;
    }

    data = PhAllocateZero(sizeof(PH_NETWORK_ITEM_QUERY_DATA));
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

VOID PhNetworkItemResolveHostname(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PPHP_RESOLVE_CACHE_ITEM cacheItem;

    if (!FlagOn(PhNetworkProviderFlagsMask, PH_NETWORK_PROVIDER_FLAG_HOSTNAME))
        return;

    // Local

    if (!PhIsNullIpAddress(&NetworkItem->LocalEndpoint.Address))
    {
        PhAcquireQueuedLockShared(&PhpResolveCacheHashtableLock);
        cacheItem = PhpLookupResolveCacheItem(&NetworkItem->LocalEndpoint.Address);
        PhReleaseQueuedLockShared(&PhpResolveCacheHashtableLock);

        if (cacheItem)
        {
            PhReferenceObject(cacheItem->HostString);
            NetworkItem->LocalHostString = cacheItem->HostString;
            NetworkItem->LocalHostnameResolved = TRUE;
        }
        else
        {
            PhpQueueNetworkItemQuery(NetworkItem, FALSE);
        }
    }
    else
    {
        NetworkItem->LocalHostnameResolved = TRUE;
    }

    // Remote

    if (!PhIsNullIpAddress(&NetworkItem->RemoteEndpoint.Address))
    {
        PhAcquireQueuedLockShared(&PhpResolveCacheHashtableLock);
        cacheItem = PhpLookupResolveCacheItem(&NetworkItem->RemoteEndpoint.Address);
        PhReleaseQueuedLockShared(&PhpResolveCacheHashtableLock);

        if (cacheItem)
        {
            PhReferenceObject(cacheItem->HostString);
            NetworkItem->RemoteHostString = cacheItem->HostString;
            NetworkItem->RemoteHostnameResolved = TRUE;
        }
        else
        {
            PhpQueueNetworkItemQuery(NetworkItem, TRUE);
        }
    }
    else
    {
        NetworkItem->RemoteHostnameResolved = TRUE;
    }
}

VOID PhNetworkItemInvalidateHostname(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    if (!FlagOn(PhNetworkProviderFlagsMask, PH_NETWORK_PROVIDER_FLAG_HOSTNAME))
        return;

    if (NetworkItem->LocalHostString)
    {
        PhDereferenceObject(NetworkItem->LocalHostString);
        NetworkItem->LocalHostString = NULL;
    }

    if (NetworkItem->RemoteHostString)
    {
        PhDereferenceObject(NetworkItem->RemoteHostString);
        NetworkItem->RemoteHostString = NULL;
    }

    NetworkItem->LocalHostnameResolved = FALSE;
    NetworkItem->RemoteHostnameResolved = FALSE;

    PhNetworkItemResolveHostname(NetworkItem);
}

VOID PhpUpdateNetworkItemOwner(
    _In_ PPH_NETWORK_ITEM NetworkItem,
    _In_ ULONGLONG ServiceTag
    )
{
    if (ServiceTag)
    {
        PPH_STRING serviceName;

        serviceName = PhGetServiceNameFromTag(NetworkItem->ProcessId, (PVOID)ServiceTag);

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

    if (!FlagOn(PhNetworkProviderFlagsMask, PH_NETWORK_PROVIDER_FLAG_HOSTNAME))
        return;
    //if (!RtlFirstEntrySList(&PhNetworkItemQueryListHead))
    //    return;

    entry = RtlInterlockedFlushSList(&PhNetworkItemQueryListHead);

    while (entry)
    {
        data = CONTAINING_RECORD(entry, PH_NETWORK_ITEM_QUERY_DATA, ListEntry);
        entry = entry->Next;

        if (data->Remote)
        {
            PhMoveReference(&data->NetworkItem->RemoteHostString, data->HostString);
            data->NetworkItem->RemoteHostnameResolved = TRUE;
        }
        else
        {
            PhMoveReference(&data->NetworkItem->LocalHostString, data->HostString);
            data->NetworkItem->LocalHostnameResolved = TRUE;
        }

        data->NetworkItem->JustResolved = TRUE;

        PhDereferenceObject(data->NetworkItem);
        PhFree(data);
    }
}

VOID PhNetworkProviderUpdate(
    _In_ PVOID Object
    )
{
    static ULONG runCount = 0;
    PPH_NETWORK_CONNECTION connections;
    ULONG numberOfConnections;
    ULONG i;

    if (!NetworkImportDone)
    {
        PVOID iphlpapi;

        if (iphlpapi = PhLoadLibrary(L"iphlpapi.dll"))
        {
            GetExtendedTcpTable_I = PhGetDllBaseProcedureAddress(iphlpapi, "GetExtendedTcpTable", 0);
            GetExtendedUdpTable_I = PhGetDllBaseProcedureAddress(iphlpapi, "GetExtendedUdpTable", 0);
            GetBoundTcpEndpointTable_I = PhGetDllBaseProcedureAddress(iphlpapi, "InternalGetBoundTcpEndpointTable", 0);
            GetBoundTcp6EndpointTable_I = PhGetDllBaseProcedureAddress(iphlpapi, "InternalGetBoundTcp6EndpointTable", 0);
        }

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
                    (*networkItem)->ProcessId == connections[i].ProcessId &&
                    (*networkItem)->CreateTime.QuadPart == connections[i].CreateTime.QuadPart
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
            networkItem->LocalScopeId = connections[i].LocalScopeId;
            networkItem->RemoteScopeId = connections[i].RemoteScopeId;
            PhpUpdateNetworkItemOwner(networkItem, connections[i].OwnerInfo[0]);

            // Format various strings.

            switch (networkItem->LocalEndpoint.Address.Type)
            {
            case PH_IPV4_NETWORK_TYPE:
                {
                    WCHAR localAddressString[IP4_ADDRESS_STRING_LENGTH];
                    ULONG localAddressStringLength = RTL_NUMBER_OF(localAddressString);

                    if (NT_SUCCESS(RtlIpv4AddressToStringEx(
                        &networkItem->LocalEndpoint.Address.InAddr,
                        0,
                        localAddressString,
                        &localAddressStringLength
                        )))
                    {
                        networkItem->LocalAddressString = PhCreateStringEx(
                            localAddressString,
                            (localAddressStringLength - 1) * sizeof(WCHAR)
                            );
                    }
                }
                break;
            case PH_IPV6_NETWORK_TYPE:
                {
                    WCHAR localAddressString[IP6_ADDRESS_STRING_LENGTH];
                    ULONG localAddressStringLength = RTL_NUMBER_OF(localAddressString);

                    if (NT_SUCCESS(RtlIpv6AddressToStringEx(
                        &networkItem->LocalEndpoint.Address.In6Addr,
                        networkItem->LocalScopeId,
                        0,
                        localAddressString,
                        &localAddressStringLength
                        )))
                    {
                        networkItem->LocalAddressString = PhCreateStringEx(
                            localAddressString,
                            (localAddressStringLength - 1) * sizeof(WCHAR)
                            );
                    }
                }
                break;
            case PH_HV_NETWORK_TYPE:
                {
                    networkItem->LocalAddressString = PhFormatGuid(
                        &networkItem->LocalEndpoint.Address.HvAddr
                        );
                }
                break;
            }

            switch (networkItem->RemoteEndpoint.Address.Type)
            {
            case PH_IPV4_NETWORK_TYPE:
                {
                    if (!PhIsNullIpAddress(&networkItem->RemoteEndpoint.Address))
                    {
                        WCHAR remoteAddressString[IP4_ADDRESS_STRING_LENGTH];
                        ULONG remoteAddressStringLength = RTL_NUMBER_OF(remoteAddressString);

                        if (NT_SUCCESS(RtlIpv4AddressToStringEx(
                            &networkItem->RemoteEndpoint.Address.InAddr,
                            0,
                            remoteAddressString,
                            &remoteAddressStringLength
                            )))
                        {
                            networkItem->RemoteAddressString = PhCreateStringEx(
                                remoteAddressString,
                                (remoteAddressStringLength - 1) * sizeof(WCHAR)
                                );
                        }
                    }
                }
                break;
            case PH_IPV6_NETWORK_TYPE:
                {
                    if (!PhIsNullIpAddress(&networkItem->RemoteEndpoint.Address))
                    {
                        WCHAR remoteAddressString[IP6_ADDRESS_STRING_LENGTH];
                        ULONG remoteAddressStringLength = RTL_NUMBER_OF(remoteAddressString);

                        if (NT_SUCCESS(RtlIpv6AddressToStringEx(
                            &networkItem->RemoteEndpoint.Address.In6Addr,
                            networkItem->RemoteScopeId,
                            0,
                            remoteAddressString,
                            &remoteAddressStringLength
                            )))
                        {
                            networkItem->RemoteAddressString = PhCreateStringEx(
                                remoteAddressString,
                                (remoteAddressStringLength - 1) * sizeof(WCHAR)
                                );
                        }
                    }
                }
                break;
            case PH_HV_NETWORK_TYPE:
                {
                    networkItem->RemoteAddressString = PhFormatGuid(
                        &networkItem->RemoteEndpoint.Address.HvAddr
                        );
                }
                break;
            }

            if (networkItem->LocalEndpoint.Port != 0)
                PhPrintUInt32(networkItem->LocalPortString, networkItem->LocalEndpoint.Port);
            if (networkItem->RemoteEndpoint.Port != 0)
                PhPrintUInt32(networkItem->RemotePortString, networkItem->RemoteEndpoint.Port);

            // Get host names.
            PhNetworkItemResolveHostname(networkItem);

            // Get process information.
            if (processItem = PhReferenceProcessItem(networkItem->ProcessId))
            {
                networkItem->ProcessItem = processItem;
                PhSetReference(&networkItem->ProcessName, processItem->ProcessName);
                networkItem->SubsystemProcess = !!processItem->IsSubsystemProcess;

                if (PhTestEvent(&processItem->Stage1Event))
                {
                    networkItem->ProcessIconIndex = processItem->SmallIconIndex;
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
                // WSL subsystem processes (e.g. apache/nginx) create sockets, clone/fork themselves, duplicate the socket into the child process and then terminate.
                // The socket handle remains valid and in-use by the child process BUT the socket continues returning the PID of the exited process???
                // Fixing this causes a major performance problem; If we have 100,000 sockets then on previous versions of Windows we would only need 2 system calls maximum
                // (for the process list) to identify the owner of every socket but now we need to make 4 system calls for every_last_socket totaling 400,000 system calls... great. (dmex)
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

            if (!networkItem->ProcessItem)
            {
                networkItem->ProcessItem = PhReferenceProcessItem(networkItem->ProcessId);
                // NOTE: We dereference processItem in PhpNetworkItemDeleteProcedure. (dmex)
            }

            if (networkItem->ProcessItem)
            {
                if (PhIsNullOrEmptyString(networkItem->ProcessName))
                {
                    PhSetReference(&networkItem->ProcessName, &networkItem->ProcessItem->ProcessName);
                    modified = TRUE;
                }

                if (!networkItem->ProcessIconValid && PhTestEvent(&networkItem->ProcessItem->Stage1Event))
                {
                    networkItem->ProcessIconIndex = networkItem->ProcessItem->SmallIconIndex;
                    networkItem->ProcessIconValid = TRUE;
                    modified = TRUE;
                }
            }

            if (networkItem->InvalidateHostname)
            {
                networkItem->InvalidateHostname = FALSE;

                PhNetworkItemInvalidateHostname(networkItem);

                modified = TRUE;
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
    runCount++;
}

#ifdef _WIN64
PHVSOCKET_LISTENERS PhpGetHvSocketListeners(
    _In_ HANDLE SystemHandle,
    _In_ const GUID* VmId
    )
{
    NTSTATUS status;
    ULONG length;
    PHVSOCKET_LISTENERS listeners;

    length = PAGE_SIZE;
    listeners = PhAllocate(length);

    for (;;)
    {
        status = HvSocketGetListeners(SystemHandle, VmId, listeners, length, &length);
        if (status != STATUS_BUFFER_TOO_SMALL)
        {
            break;
        }

        length *= 2;
        listeners = PhReAllocate(listeners, length);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(listeners);
        listeners = NULL;
    }

    return listeners;
}

PHVSOCKET_CONNECTIONS PhpGetHvSocketConnections(
    _In_ HANDLE SystemHandle,
    _In_ const GUID* VmId
    )
{
    NTSTATUS status;
    ULONG length;
    PHVSOCKET_CONNECTIONS connections;

    length = PAGE_SIZE;
    connections = PhAllocate(length);

    for (;;)
    {
        status = HvSocketGetConnections(SystemHandle, VmId, connections, length, &length);
        if (status != STATUS_BUFFER_TOO_SMALL)
        {
            break;
        }

        length *= 2;
        connections = PhReAllocate(connections, length);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(connections);
        connections = NULL;
    }

    return connections;
}

VOID PhpGetHvSocket(
    _Out_ PHVSOCKET_LISTENERS* Listeners,
    _Out_ PHVSOCKET_CONNECTIONS* Connections
    )
{
    HANDLE systemHandle;
    PHVSOCKET_LISTENERS listeners;
    PHVSOCKET_CONNECTIONS connections;
    ULONG listenersCount;
    ULONG connectionsCount;
    PPH_LIST listenersList;
    PPH_LIST connectionsList;

    if (!NT_SUCCESS(HvSocketOpenSystemControl(&systemHandle, NULL)))
    {
        *Listeners = NULL;
        *Connections = NULL;
        return;
    }

    listenersCount = 0;
    connectionsCount = 0;
    listenersList = PhCreateList(1);
    connectionsList = PhCreateList(1);

    listeners = PhpGetHvSocketListeners(systemHandle, &HV_GUID_ZERO);
    connections = PhpGetHvSocketConnections(systemHandle, &HV_GUID_ZERO);

    if (listeners)
    {
        for (ULONG i = 0; i < listeners->Count; i++)
        {
            PHVSOCKET_LISTENERS l;
            PHVSOCKET_CONNECTIONS c;

            if (IsEqualGUID(&listeners->Listener[i].VmId, &HV_GUID_ZERO))
                continue;

            l = PhpGetHvSocketListeners(systemHandle, &listeners->Listener[i].VmId);
            if (l)
            {
                listenersCount += l->Count;
                PhAddItemList(listenersList, l);
            }

            c = PhpGetHvSocketConnections(systemHandle, &listeners->Listener[i].VmId);
            if (c)
            {
                connectionsCount += c->Count;
                PhAddItemList(connectionsList, c);
            }
        }

        listenersCount += listeners->Count;
        PhAddItemList(listenersList, listeners);
        listeners = NULL;
    }

    if (connections)
    {
        for (ULONG i = 0; i < connections->Count; i++)
        {
            PHVSOCKET_LISTENERS l;
            PHVSOCKET_CONNECTIONS c;

            if (IsEqualGUID(&connections->Connection[i].VmId, &HV_GUID_ZERO))
                continue;

            l = PhpGetHvSocketListeners(systemHandle, &connections->Connection[i].VmId);
            if (l)
            {
                listenersCount += l->Count;
                PhAddItemList(listenersList, l);
            }

            c = PhpGetHvSocketConnections(systemHandle, &connections->Connection[i].VmId);
            if (c)
            {
                connectionsCount += c->Count;
                PhAddItemList(connectionsList, c);
            }
        }

        connectionsCount += connections->Count;
        PhAddItemList(connectionsList, connections);
        connections = NULL;
    }

    if (listenersCount)
    {
        listeners = PhAllocate(
            RTL_SIZEOF_THROUGH_FIELD(HVSOCKET_LISTENERS, Listener) +
            (sizeof(HVSOCKET_LISTENER) * listenersCount)
            );
        listeners->Count = 0;
    }

    for (ULONG i = 0; i < listenersList->Count; i++)
    {
        PHVSOCKET_LISTENERS l;

        l = listenersList->Items[i];

        if (listeners)
        {
            RtlCopyMemory(
                &listeners->Listener[listeners->Count],
                l->Listener,
                sizeof(HVSOCKET_LISTENER) * l->Count
                );

            listeners->Count += l->Count;
        }

        PhFree(l);
    }

    if (connectionsCount)
    {
        connections = PhAllocate(
            RTL_SIZEOF_THROUGH_FIELD(HVSOCKET_CONNECTIONS, Connection) +
            (sizeof(HVSOCKET_CONNECTION) * connectionsCount)
            );
        connections->Count = 0;
    }

    for (ULONG i = 0; i < connectionsList->Count; i++)
    {
        PHVSOCKET_CONNECTIONS c;

        c = connectionsList->Items[i];

        if (connections)
        {
            RtlCopyMemory(
                &connections->Connection[connections->Count],
                c->Connection,
                sizeof(HVSOCKET_CONNECTION) * c->Count
                );

            connections->Count += c->Count;
        }

        PhFree(c);
    }

    PhDereferenceObject(listenersList);
    PhDereferenceObject(connectionsList);

    *Listeners = listeners;
    *Connections = connections;

    NtClose(systemHandle);
}
#endif // _WIN64

BOOLEAN PhGetNetworkConnections(
    _Out_ PPH_NETWORK_CONNECTION *Connections,
    _Out_ PULONG NumberOfConnections
    )
{
    PVOID table;
    ULONG tableSize;
    PMIB_TCPTABLE_OWNER_MODULE tcp4Table;
    PMIB_TCP6TABLE_OWNER_MODULE tcp6Table;
    PMIB_UDPTABLE_OWNER_MODULE udp4Table;
    PMIB_UDP6TABLE_OWNER_MODULE udp6Table;
    PMIB_TCPTABLE2 boundTcpTable;
    PMIB_TCP6TABLE2 boundTcp6Table;
#ifdef _WIN64
    PHVSOCKET_LISTENERS hvListeners;
    PHVSOCKET_CONNECTIONS hvConnections;
#endif
    ULONG i;
    ULONG count = 0;
    ULONG index = 0;
    PPH_NETWORK_CONNECTION connections;

    // TCP IPv4

    tableSize = 0;
    GetExtendedTcpTable_I(NULL, &tableSize, FALSE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0);
    table = PhAllocate(tableSize);

    if (GetExtendedTcpTable_I(table, &tableSize, FALSE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0) == NO_ERROR)
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

    if (GetExtendedTcpTable_I(table, &tableSize, FALSE, AF_INET6, TCP_TABLE_OWNER_MODULE_ALL, 0) == NO_ERROR)
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

    if (GetExtendedUdpTable_I(table, &tableSize, FALSE, AF_INET, UDP_TABLE_OWNER_MODULE, 0) == NO_ERROR)
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

    if (GetExtendedUdpTable_I(table, &tableSize, FALSE, AF_INET6, UDP_TABLE_OWNER_MODULE, 0) == NO_ERROR)
    {
        udp6Table = table;
        count += udp6Table->dwNumEntries;
    }
    else
    {
        PhFree(table);
        udp6Table = NULL;
    }

#ifdef _WIN64
    // Hyper-V
    PhpGetHvSocket(&hvListeners, &hvConnections);
    if (hvListeners)
    {
        count += hvListeners->Count;
    }
    if (hvConnections)
    {
        count += hvConnections->Count;
    }
#endif

    if (PhEnableNetworkBoundConnections && WindowsVersion >= WINDOWS_10_RS5 && GetBoundTcpEndpointTable_I && GetBoundTcp6EndpointTable_I)
    {
        // Bound TCP IPv4

        if (GetBoundTcpEndpointTable_I(&table, PhHeapHandle, 0) == NO_ERROR)
        {
            boundTcpTable = table;
            count += boundTcpTable->dwNumEntries;
        }
        else
        {
            boundTcpTable = NULL;
        }

        // Bound TCP IPv6

        if (GetBoundTcp6EndpointTable_I(&table, PhHeapHandle, 0) == NO_ERROR)
        {
            boundTcp6Table = table;
            count += boundTcp6Table->dwNumEntries;
        }
        else
        {
            boundTcp6Table = NULL;
        }
    }
    else
    {
        boundTcpTable = NULL;
        boundTcp6Table = NULL;
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

            connections[index].LocalScopeId = udp6Table->table[i].dwLocalScopeId;
            connections[index].RemoteScopeId = 0;

            index++;
        }

        PhFree(udp6Table);
    }

    if (PhEnableNetworkBoundConnections && WindowsVersion >= WINDOWS_10_RS5)
    {
        if (boundTcpTable)
        {
            for (i = 0; i < boundTcpTable->dwNumEntries; i++)
            {
                connections[index].ProtocolType = PH_TCP4_NETWORK_PROTOCOL;

                connections[index].LocalEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                connections[index].LocalEndpoint.Address.Ipv4 = boundTcpTable->table[i].dwLocalAddr;
                connections[index].LocalEndpoint.Port = _byteswap_ushort((USHORT)boundTcpTable->table[i].dwLocalPort);

                connections[index].RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                connections[index].RemoteEndpoint.Address.Ipv4 = boundTcpTable->table[i].dwRemoteAddr;
                connections[index].RemoteEndpoint.Port = _byteswap_ushort((USHORT)boundTcpTable->table[i].dwRemotePort);

                connections[index].State = boundTcpTable->table[i].dwState;
                connections[index].ProcessId = UlongToHandle(boundTcpTable->table[i].dwOwningPid);

                index++;
            }

            RtlFreeHeap(PhHeapHandle, 0, boundTcpTable);
        }

        if (boundTcp6Table)
        {
            for (i = 0; i < boundTcp6Table->dwNumEntries; i++)
            {
                connections[index].ProtocolType = PH_TCP6_NETWORK_PROTOCOL;

                connections[index].LocalEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                memcpy(connections[index].LocalEndpoint.Address.Ipv6, boundTcp6Table->table[i].LocalAddr.s6_addr, 16);
                connections[index].LocalEndpoint.Port = _byteswap_ushort((USHORT)boundTcp6Table->table[i].dwLocalPort);

                connections[index].RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                memcpy(connections[index].RemoteEndpoint.Address.Ipv6, boundTcp6Table->table[i].RemoteAddr.s6_addr, 16);
                connections[index].RemoteEndpoint.Port = _byteswap_ushort((USHORT)boundTcp6Table->table[i].dwRemotePort);

                connections[index].State = boundTcp6Table->table[i].State;
                connections[index].ProcessId = UlongToHandle(boundTcp6Table->table[i].dwOwningPid);

                connections[index].LocalScopeId = boundTcp6Table->table[i].dwLocalScopeId;
                connections[index].RemoteScopeId = boundTcp6Table->table[i].dwRemoteScopeId;

                index++;
            }

            RtlFreeHeap(PhHeapHandle, 0, boundTcp6Table);
        }
    }

#ifdef _WIN64
    if (hvListeners)
    {
        for (i = 0; i < hvListeners->Count; i++)
        {
            connections[index].ProtocolType = PH_HV_NETWORK_PROTOCOL;

            connections[index].LocalEndpoint.Address.Type = PH_HV_NETWORK_TYPE;
            connections[index].LocalEndpoint.Address.HvAddr = hvListeners->Listener[i].ServiceId;

            if (hvListeners->Listener[i].Port <= 0x7FFFFFFF) // valid port range
                connections[index].LocalEndpoint.Port = hvListeners->Listener[i].Port;

            connections[index].RemoteEndpoint.Address.Type = PH_HV_NETWORK_TYPE;
            connections[index].RemoteEndpoint.Address.HvAddr = hvListeners->Listener[i].VmId;

            connections[index].ProcessId = UlongToHandle(hvListeners->Listener[i].ProcessId);

            connections[index].State = 0; // HACK

            index++;
        }

        PhFree(hvListeners);
    }

    if (hvConnections)
    {
        for (i = 0; i < hvConnections->Count; i++)
        {
            connections[index].ProtocolType = PH_HV_NETWORK_PROTOCOL;

            connections[index].LocalEndpoint.Address.Type = PH_HV_NETWORK_TYPE;
            connections[index].LocalEndpoint.Address.HvAddr = hvConnections->Connection[i].ServiceId;

            if (hvConnections->Connection[i].Port <= 0x7FFFFFFF) // valid port range
                connections[index].LocalEndpoint.Port = hvConnections->Connection[i].Port;

            connections[index].RemoteEndpoint.Address.Type = PH_HV_NETWORK_TYPE;
            connections[index].RemoteEndpoint.Address.HvAddr = hvConnections->Connection[i].VmId;

            connections[index].ProcessId = UlongToHandle(hvConnections->Connection[i].ProcessId);

            connections[index].State = 1; // HACK

            index++;
        }

        PhFree(hvConnections);
    }
#endif

    *NumberOfConnections = count;
    *Connections = connections;

    return TRUE;
}

static PH_KEY_VALUE_PAIR PhProtocolTypeStrings[] =
{
    SIP(SREF(L"TCP"), PH_TCP4_NETWORK_PROTOCOL),
    SIP(SREF(L"TCP6"), PH_TCP6_NETWORK_PROTOCOL),
    SIP(SREF(L"UDP"), PH_UDP4_NETWORK_PROTOCOL),
    SIP(SREF(L"UDP6"), PH_UDP6_NETWORK_PROTOCOL),
    SIP(SREF(L"HYPERV"), PH_HV_NETWORK_PROTOCOL),
};

static PH_KEY_VALUE_PAIR PhTcpStateStrings[] =
{
    SIP(SREF(L"Closed"), MIB_TCP_STATE_CLOSED),
    SIP(SREF(L"Listen"), MIB_TCP_STATE_LISTEN),
    SIP(SREF(L"SYN sent"), MIB_TCP_STATE_SYN_SENT),
    SIP(SREF(L"SYN received"), MIB_TCP_STATE_SYN_RCVD),
    SIP(SREF(L"Established"), MIB_TCP_STATE_ESTAB),
    SIP(SREF(L"FIN wait 1"), MIB_TCP_STATE_FIN_WAIT1),
    SIP(SREF(L"FIN wait 2"), MIB_TCP_STATE_FIN_WAIT2),
    SIP(SREF(L"Close wait"), MIB_TCP_STATE_CLOSE_WAIT),
    SIP(SREF(L"Closing"), MIB_TCP_STATE_CLOSING),
    SIP(SREF(L"Last ACK"), MIB_TCP_STATE_LAST_ACK),
    SIP(SREF(L"Time wait"), MIB_TCP_STATE_TIME_WAIT),
    SIP(SREF(L"Delete TCB"), MIB_TCP_STATE_DELETE_TCB),
    SIP(SREF(L"Bound"), MIB_TCP_STATE_RESERVED),
};

PPH_STRINGREF PhGetProtocolTypeName(
    _In_ ULONG ProtocolType
    )
{
    static PH_STRINGREF unknown = PH_STRINGREF_INIT(L"Unknown");
    PPH_STRINGREF string;

    if (PhFindStringSiKeyValuePairs(
        PhProtocolTypeStrings,
        sizeof(PhProtocolTypeStrings),
        ProtocolType,
        (PWSTR*)&string
        ))
    {
        return string;
    }

    return &unknown;
}

PPH_STRINGREF PhGetTcpStateName(
    _In_ ULONG State
    )
{
    static PH_STRINGREF unknown = PH_STRINGREF_INIT(L"Unknown");
    PPH_STRINGREF string;

    if (PhFindStringSiKeyValuePairs(
        PhTcpStateStrings,
        sizeof(PhTcpStateStrings),
        State,
        (PWSTR*)&string
        ))
    {
        return string;
    }

    return &unknown;
}
