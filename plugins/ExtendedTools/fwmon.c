/*
 * Process Hacker Extended Tools -
 *   Firewall monitoring
 *
 * Copyright (C) 2020 dmex
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

#include "exttools.h"
#include <workqueue.h>
#include <fwpmu.h>
#include <fwpsu.h>

PH_CALLBACK_REGISTRATION EtFwProcessesUpdatedCallbackRegistration;
PPH_OBJECT_TYPE FwObjectType = NULL;
HANDLE EtFwEngineHandle = NULL;
HANDLE FwEventHandle = NULL;
ULONG FwRunCount = 0;
ULONG FwMaxEventAge = 60;
SLIST_HEADER FwPacketListHead;
LIST_ENTRY FwAgeListHead = { &FwAgeListHead, &FwAgeListHead };
_FwpmNetEventSubscribe FwpmNetEventSubscribe_I = NULL;
PPH_HASHTABLE EtFwResolveCacheHashtable = NULL;

BOOLEAN EtFwEnableResolveCache = TRUE;
PH_INITONCE EtFwWorkQueueInitOnce = PH_INITONCE_INIT;
PH_WORK_QUEUE EtFwWorkQueue;
SLIST_HEADER EtFwQueryListHead;
static PPH_HASHTABLE EtFwCacheHashtable = NULL;
static PH_QUEUED_LOCK EtFwCacheHashtableLock = PH_QUEUED_LOCK_INIT;

PH_CALLBACK_DECLARE(FwItemAddedEvent);
PH_CALLBACK_DECLARE(FwItemModifiedEvent);
PH_CALLBACK_DECLARE(FwItemRemovedEvent);
PH_CALLBACK_DECLARE(FwItemsUpdatedEvent);

typedef struct _FW_EVENT
{
    FWPM_NET_EVENT_TYPE Type;
    ULONG IsLoopback;
    ULONG Direction;
    ULONG ProcessId;
    ULONG IpProtocol;

    ULONG64 FilterId;
    ULONG LayerId;
    ULONG OriginalProfile;
    ULONG CurrentProfile;

    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;

    PPH_STRING LocalHostnameString;
    PPH_STRING RemoteHostnameString;

    PPH_STRING RuleName;
    PPH_STRING RuleDescription;
    PPH_STRING RuleLayerName;
    PPH_STRING RuleLayerDescription;

    PPH_STRING ProcessFileName;
    PPH_STRING ProcessBaseString;

    INT CountryIconIndex;
    PPH_STRING RemoteCountryName;

    PPH_PROCESS_ITEM ProcessItem;
} FW_EVENT, *PFW_EVENT;

typedef struct _FW_EVENT_PACKET
{
    SLIST_ENTRY ListEntry;
    FW_EVENT Event;
} FW_EVENT_PACKET, *PFW_EVENT_PACKET;

typedef struct _FW_RESOLVE_CACHE_ITEM
{
    PH_IP_ADDRESS Address;
    PPH_STRING HostString;
} FW_RESOLVE_CACHE_ITEM, *PFW_RESOLVE_CACHE_ITEM;

typedef struct _FW_ITEM_QUERY_DATA
{
    SLIST_ENTRY ListEntry;
    PFW_EVENT_ITEM EventItem;

    PH_IP_ADDRESS Address;
    BOOLEAN Remote;
    PPH_STRING HostString;
} FW_ITEM_QUERY_DATA, *PFW_ITEM_QUERY_DATA;

VOID PhpQueryHostnameForEntry(
    _In_ PFW_EVENT_ITEM Entry
    );

BOOLEAN EtFwResolveCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PFW_RESOLVE_CACHE_ITEM cacheItem1 = *(PFW_RESOLVE_CACHE_ITEM*)Entry1;
    PFW_RESOLVE_CACHE_ITEM cacheItem2 = *(PFW_RESOLVE_CACHE_ITEM*)Entry2;

    return PhEqualIpAddress(&cacheItem1->Address, &cacheItem2->Address);
}

ULONG NTAPI EtFwResolveCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PFW_RESOLVE_CACHE_ITEM cacheItem = *(PFW_RESOLVE_CACHE_ITEM*)Entry;

    return PhHashIpAddress(&cacheItem->Address);
}

PFW_RESOLVE_CACHE_ITEM EtFwLookupResolveCacheItem(
    _In_ PPH_IP_ADDRESS Address
    )
{
    FW_RESOLVE_CACHE_ITEM lookupCacheItem;
    PFW_RESOLVE_CACHE_ITEM lookupCacheItemPtr = &lookupCacheItem;
    PFW_RESOLVE_CACHE_ITEM* cacheItemPtr;

    // Construct a temporary cache item for the lookup.
    lookupCacheItem.Address = *Address;

    cacheItemPtr = (PFW_RESOLVE_CACHE_ITEM*)PhFindEntryHashtable(
        EtFwResolveCacheHashtable,
        &lookupCacheItemPtr
        );

    if (cacheItemPtr)
        return *cacheItemPtr;
    else
        return NULL;
}

VOID NTAPI FwObjectTypeDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PFW_EVENT_ITEM event = Object;

    if (event->ProcessFileName)
        PhDereferenceObject(event->ProcessFileName);
    if (event->ProcessBaseString)
        PhDereferenceObject(event->ProcessBaseString);
    if (event->LocalHostnameString)
        PhDereferenceObject(event->LocalHostnameString);
    if (event->RemoteHostnameString)
        PhDereferenceObject(event->RemoteHostnameString);
    if (event->RuleName)
        PhDereferenceObject(event->RuleName);
    if (event->RuleDescription)
        PhDereferenceObject(event->RuleDescription);
    if (event->FwRuleLayerNameString)
        PhDereferenceObject(event->FwRuleLayerNameString);
    if (event->FwRuleLayerDescriptionString)
        PhDereferenceObject(event->FwRuleLayerDescriptionString);
    if (event->RemoteCountryName)
        PhDereferenceObject(event->RemoteCountryName);

    if (event->TimeString)
        PhDereferenceObject(event->TimeString);
    if (event->TooltipText)
        PhDereferenceObject(event->TooltipText);

    if (event->ProcessItem)
        PhDereferenceObject(event->ProcessItem);
}

PFW_EVENT_ITEM FwCreateEventItem(
    VOID
    )
{
    static ULONG64 Index = 0;
    PFW_EVENT_ITEM entry;

    entry = PhCreateObjectZero(sizeof(FW_EVENT_ITEM), FwObjectType);
    entry->Index = ++Index;

    return entry;
}

VOID FwPushFirewallEvent(
    _In_ PFW_EVENT Event
    )
{
    PFW_EVENT_PACKET packet;

    packet = PhAllocateZero(sizeof(FW_EVENT_PACKET));
    memcpy(&packet->Event, Event, sizeof(FW_EVENT));
    RtlInterlockedPushEntrySList(&FwPacketListHead, &packet->ListEntry);
}

VOID FwProcessFirewallEvent(
    _In_ PFW_EVENT_PACKET Packet,
    _In_ ULONG RunId
    )
{
    PFW_EVENT diskEvent = &Packet->Event;
    PFW_EVENT_ITEM entry;

    entry = FwCreateEventItem();
    PhQuerySystemTime(&entry->AddedTime);
    entry->Direction = diskEvent->Direction;
    entry->Type = diskEvent->Type;
    entry->IpProtocol = diskEvent->IpProtocol;
    entry->LocalEndpoint = diskEvent->LocalEndpoint;
    entry->LocalHostnameString = diskEvent->LocalHostnameString;
    entry->RemoteEndpoint = diskEvent->RemoteEndpoint;
    entry->RemoteHostnameString = diskEvent->RemoteHostnameString;
    entry->ProcessFileName = diskEvent->ProcessFileName;
    entry->ProcessBaseString = diskEvent->ProcessBaseString;
    entry->ProcessItem = diskEvent->ProcessItem;

    if (entry->ProcessItem && !entry->ProcessIconValid && PhTestEvent(&entry->ProcessItem->Stage1Event))
    {
        entry->ProcessIcon = entry->ProcessItem->SmallIcon;
        entry->ProcessIconValid = TRUE;
    }

    entry->RuleName = diskEvent->RuleName;
    entry->RuleDescription = diskEvent->RuleDescription;
    entry->FwRuleLayerNameString = diskEvent->RuleLayerName;
    entry->FwRuleLayerDescriptionString = diskEvent->RuleLayerDescription;
    entry->CountryIconIndex = diskEvent->CountryIconIndex;
    entry->RemoteCountryName = diskEvent->RemoteCountryName;

    // Add the item to the age list.
    entry->AddTime = RunId;
    entry->FreshTime = RunId;
    InsertHeadList(&FwAgeListHead, &entry->AgeListEntry);

    PhpQueryHostnameForEntry(entry);

    // Raise the item added event.
    PhInvokeCallback(&FwItemAddedEvent, entry);
}

_Success_(return)
BOOLEAN FwProcessEventType(
    _In_ const FWPM_NET_EVENT* FwEvent,
    _Out_ PBOOLEAN IsLoopback,
    _Out_ PULONG Direction,
    _Out_ PULONG64 filterId,
    _Out_ PUSHORT layerId,
    _Out_ PULONG reauthReason,
    _Out_ PULONG originalProfile,
    _Out_ PULONG currentProfile
    )
{
    switch (FwEvent->type)
    {
    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP:
        {
            FWPM_NET_EVENT_CLASSIFY_DROP* fwDropEvent = FwEvent->classifyDrop;

            switch (fwDropEvent->msFwpDirection)
            {
            case FWP_DIRECTION_IN:
            case FWP_DIRECTION_INBOUND:
                *Direction = FWP_DIRECTION_INBOUND;
                break;
            case FWP_DIRECTION_OUT:
            case FWP_DIRECTION_OUTBOUND:
                *Direction = FWP_DIRECTION_OUTBOUND;
                break;
            default:
                *Direction = FWP_DIRECTION_MAX;
                break;
            }

            if (filterId)
                *filterId = fwDropEvent->filterId;
            if (layerId)
                *layerId = fwDropEvent->layerId;
            if (reauthReason)
                *reauthReason = fwDropEvent->reauthReason;
            if (originalProfile)
                *originalProfile = fwDropEvent->originalProfile;
            if (currentProfile)
                *currentProfile = fwDropEvent->currentProfile;
            if (IsLoopback)
                *IsLoopback = !!fwDropEvent->isLoopback;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_CLASSIFY_ALLOW:
        {
            FWPM_NET_EVENT_CLASSIFY_ALLOW* fwAllowEvent = FwEvent->classifyAllow;

            switch (fwAllowEvent->msFwpDirection)
            {
            case FWP_DIRECTION_IN:
            case FWP_DIRECTION_INBOUND:
                *Direction = FWP_DIRECTION_INBOUND;
                break;
            case FWP_DIRECTION_OUT:
            case FWP_DIRECTION_OUTBOUND:
                *Direction = FWP_DIRECTION_OUTBOUND;
                break;
            default:
                *Direction = FWP_DIRECTION_MAX;
                break;
            }

            if (filterId)
                *filterId = fwAllowEvent->filterId;
            if (layerId)
                *layerId = fwAllowEvent->layerId;
            if (reauthReason)
                *reauthReason = fwAllowEvent->reauthReason;
            if (originalProfile)
                *originalProfile = fwAllowEvent->originalProfile;
            if (currentProfile)
                *currentProfile = fwAllowEvent->currentProfile;
            if (IsLoopback)
                *IsLoopback = !!fwAllowEvent->isLoopback;
        }
        return TRUE;
    }

    return FALSE;
}

PPH_STRING EtFwGetDnsReverseNameFromAddress(
    _In_ PPH_IP_ADDRESS Address
    )
{
    switch (Address->Type)
    {
    case PH_IPV4_NETWORK_TYPE:
        {
            PH_STRING_BUILDER stringBuilder;

            PhInitializeStringBuilder(&stringBuilder, DNS_MAX_IP4_REVERSE_NAME_LENGTH);

            PhAppendFormatStringBuilder(
                &stringBuilder,
                L"%hhu.%hhu.%hhu.%hhu.",
                Address->InAddr.s_impno,
                Address->InAddr.s_lh,
                Address->InAddr.s_host,
                Address->InAddr.s_net
                );

            PhAppendStringBuilder2(&stringBuilder, DNS_IP4_REVERSE_DOMAIN_STRING);

            return PhFinalStringBuilderString(&stringBuilder);
        }
    case PH_IPV6_NETWORK_TYPE:
        {
            PH_STRING_BUILDER stringBuilder;

            PhInitializeStringBuilder(&stringBuilder, DNS_MAX_IP6_REVERSE_NAME_LENGTH);

            for (INT i = sizeof(IN6_ADDR) - 1; i >= 0; i--)
            {
                PhAppendFormatStringBuilder(
                    &stringBuilder,
                    L"%hhx.%hhx.",
                    Address->In6Addr.s6_addr[i] & 0xF,
                    (Address->In6Addr.s6_addr[i] >> 4) & 0xF
                    );
            }

            PhAppendStringBuilder2(&stringBuilder, DNS_IP6_REVERSE_DOMAIN_STRING);

            return PhFinalStringBuilderString(&stringBuilder);
        }
    default:
        return NULL;
    }
}

PPH_STRING EtFwGetNameFromAddress(
    _In_ PPH_IP_ADDRESS Address
    )
{
    PPH_STRING addressEndpointString = NULL;
    PPH_STRING dnsEndpointReverseString;

    switch (Address->Type)
    {
    case PH_IPV4_NETWORK_TYPE:
        {
            if (IN4_IS_ADDR_UNSPECIFIED(&Address->InAddr))
            {
                static PPH_STRING string = NULL;
                if (!string) string = PhCreateString(L"[Unspecified]");

                return PhReferenceObject(string);
            }
            else if (IN4_IS_ADDR_LOOPBACK(&Address->InAddr))
            {
                static PPH_STRING string = NULL;
                if (!string) string = PhCreateString(L"[Loopback]");

                return PhReferenceObject(string);
            }
            else if (IN4_IS_ADDR_BROADCAST(&Address->InAddr))
            {
                static PPH_STRING string = NULL;
                if (!string) string = PhCreateString(L"[Broadcast]");

                return PhReferenceObject(string);
            }
            else if (IN4_IS_ADDR_MULTICAST(&Address->InAddr))
            {
                static PPH_STRING string = NULL;
                if (!string) string = PhCreateString(L"[Multicast]");

                return PhReferenceObject(string);
            }
            else if (IN4_IS_ADDR_LINKLOCAL(&Address->InAddr))
            {
                static PPH_STRING string = NULL;
                if (!string) string = PhCreateString(L"[Linklocal]");

                return PhReferenceObject(string);
            }
            //else if (IN4_IS_ADDR_RFC1918(&Address->InAddr))
            //{
            //    static PPH_STRING string = NULL;
            //    if (!string) string = PhCreateString(L"RFC1918");
            //    return PhReferenceObject(string);
            //}
        }
        break;
    case PH_IPV6_NETWORK_TYPE:
        {
            if (IN6_IS_ADDR_UNSPECIFIED(&Address->In6Addr))
            {
                static PPH_STRING string = NULL;
                if (!string) string = PhCreateString(L"[Unspecified]");

                return PhReferenceObject(string);
            }
            else if (IN6_IS_ADDR_LOOPBACK(&Address->In6Addr))
            {
                static PPH_STRING string = NULL;
                if (!string) string = PhCreateString(L"[Loopback]");

                return PhReferenceObject(string);
            }
            else if (IN6_IS_ADDR_MULTICAST(&Address->In6Addr))
            {
                static PPH_STRING string = NULL;
                if (!string) string = PhCreateString(L"[Multicast]");

                return PhReferenceObject(string);
            }
            else if (IN6_IS_ADDR_LINKLOCAL(&Address->In6Addr))
            {
                static PPH_STRING string = NULL;
                if (!string) string = PhCreateString(L"[Linklocal]");

                return PhReferenceObject(string);
            }
        }
        break;
    }

    if (dnsEndpointReverseString = EtFwGetDnsReverseNameFromAddress(Address))
    {
        PDNS_RECORD dnsRecordList;

        if (dnsRecordList = PhDnsQuery(NULL, dnsEndpointReverseString->Buffer, DNS_TYPE_PTR)) // DNS_QUERY_NO_WIRE_QUERY
        {
            for (PDNS_RECORD dnsRecord = dnsRecordList; dnsRecord; dnsRecord = dnsRecord->pNext)
            {
                if (dnsRecord->wType == DNS_TYPE_PTR)
                {
                    addressEndpointString = PhCreateString(dnsRecord->Data.PTR.pNameHost);

                    PhReferenceObject(addressEndpointString);
                    break;
                }
            }

            PhDnsFree(dnsRecordList);
        }

        PhDereferenceObject(dnsEndpointReverseString);
    }

    if (!addressEndpointString) // DNS_QUERY_NO_WIRE_QUERY
        addressEndpointString = PhReferenceEmptyString();

    return addressEndpointString;
}

NTSTATUS EtFwNetworkItemQueryWorker(
    _In_ PVOID Parameter
    )
{
    PFW_ITEM_QUERY_DATA data = (PFW_ITEM_QUERY_DATA)Parameter;
    PPH_STRING hostString;
    PFW_RESOLVE_CACHE_ITEM cacheItem;

    // Last minute check of the cache.

    PhAcquireQueuedLockShared(&EtFwCacheHashtableLock);
    cacheItem = EtFwLookupResolveCacheItem(&data->Address);
    PhReleaseQueuedLockShared(&EtFwCacheHashtableLock);

    if (!cacheItem)
    {
        hostString = EtFwGetNameFromAddress(&data->Address);

        if (hostString)
        {
            data->HostString = hostString;

            // Update the cache.

            PhAcquireQueuedLockExclusive(&EtFwCacheHashtableLock);

            cacheItem = EtFwLookupResolveCacheItem(&data->Address);

            if (!cacheItem)
            {
                cacheItem = PhAllocateZero(sizeof(FW_RESOLVE_CACHE_ITEM));
                cacheItem->Address = data->Address;
                cacheItem->HostString = hostString;
                PhReferenceObject(hostString);

                PhAddEntryHashtable(EtFwResolveCacheHashtable, &cacheItem);
            }

            PhReleaseQueuedLockExclusive(&EtFwCacheHashtableLock);
        }
    }
    else
    {
        PhReferenceObject(cacheItem->HostString);
        data->HostString = cacheItem->HostString;
    }

    RtlInterlockedPushEntrySList(&EtFwQueryListHead, &data->ListEntry);

    return STATUS_SUCCESS;
}

VOID EtFwQueueNetworkItemQuery(
    _In_ PFW_EVENT_ITEM EventItem,
    _In_ BOOLEAN Remote
    )
{
    PFW_ITEM_QUERY_DATA data;

    if (!EtFwEnableResolveCache)
        return;

    data = PhAllocateZero(sizeof(FW_ITEM_QUERY_DATA));
    data->EventItem = PhReferenceObject(EventItem);
    data->Remote = Remote;

    if (Remote)
        data->Address = EventItem->RemoteEndpoint.Address;
    else
        data->Address = EventItem->LocalEndpoint.Address;

    if (PhBeginInitOnce(&EtFwWorkQueueInitOnce))
    {
        PhInitializeWorkQueue(&EtFwWorkQueue, 0, 3, 500);
        PhEndInitOnce(&EtFwWorkQueueInitOnce);
    }

    PhQueueItemWorkQueue(&EtFwWorkQueue, EtFwNetworkItemQueryWorker, data);
}

VOID EtFwFlushHostNameData(
    VOID
    )
{
    PSLIST_ENTRY entry;
    PFW_ITEM_QUERY_DATA data;

    if (!RtlFirstEntrySList(&EtFwQueryListHead))
        return;

    entry = RtlInterlockedFlushSList(&EtFwQueryListHead);

    while (entry)
    {
        data = CONTAINING_RECORD(entry, FW_ITEM_QUERY_DATA, ListEntry);
        entry = entry->Next;

        if (data->Remote)
            PhMoveReference(&data->EventItem->RemoteHostnameString, data->HostString);
        else
            PhMoveReference(&data->EventItem->LocalHostnameString, data->HostString);

        data->EventItem->JustResolved = TRUE;

        PhDereferenceObject(data->EventItem);
        PhFree(data);
    }
}

VOID PhpQueryHostnameForEntry(
    _In_ PFW_EVENT_ITEM Entry
    )
{
    PFW_RESOLVE_CACHE_ITEM cacheItem;

    // Reset hashtable once in a while.
    {
        static ULONG64 lastTickCount = 0;
        ULONG64 tickCount = NtGetTickCount64();

        if (tickCount - lastTickCount >= 120 * CLOCKS_PER_SEC)
        {
            PFW_RESOLVE_CACHE_ITEM* entry;
            ULONG i = 0;

            while (PhEnumHashtable(EtFwResolveCacheHashtable, (PVOID*)&entry, &i))
            {
                if ((*entry)->HostString)
                    PhReferenceObject((*entry)->HostString);
                PhFree(*entry);
            }

            PhClearHashtable(EtFwResolveCacheHashtable);
            lastTickCount = tickCount;
        }
    }

    // Local
    if (!PhIsNullIpAddress(&Entry->LocalEndpoint.Address))
    {
        PhAcquireQueuedLockShared(&EtFwCacheHashtableLock);
        cacheItem = EtFwLookupResolveCacheItem(&Entry->LocalEndpoint.Address);
        PhReleaseQueuedLockShared(&EtFwCacheHashtableLock);

        if (cacheItem)
        {
            PhReferenceObject(cacheItem->HostString);
            Entry->LocalHostnameString = cacheItem->HostString;
        }
        else
        {
            EtFwQueueNetworkItemQuery(Entry, FALSE);
        }
    }

    // Remote
    if (!PhIsNullIpAddress(&Entry->RemoteEndpoint.Address))
    {
        PhAcquireQueuedLockShared(&EtFwCacheHashtableLock);
        cacheItem = EtFwLookupResolveCacheItem(&Entry->RemoteEndpoint.Address);
        PhReleaseQueuedLockShared(&EtFwCacheHashtableLock);

        if (cacheItem)
        {
            PhReferenceObject(cacheItem->HostString);
            Entry->RemoteHostnameString = cacheItem->HostString;
        }
        else
        {
            EtFwQueueNetworkItemQuery(Entry, TRUE);
        }
    }
}

PPH_PROCESS_ITEM EtFwFileNameToProcess(
    _In_ PPH_STRING ProcessBaseString
)
{
    static PVOID processInfo = NULL;
    static ULONG64 lastTickTotal = 0;
    PSYSTEM_PROCESS_INFORMATION process;
    ULONG64 tickCount = NtGetTickCount64();

    if (tickCount - lastTickTotal >= 2 * CLOCKS_PER_SEC)
    {
        lastTickTotal = tickCount;

        if (processInfo)
        {
            PhFree(processInfo);
            processInfo = NULL;
        }

        PhEnumProcesses(&processInfo);
    }

    if (process = PhFindProcessInformationByImageName(processInfo, &ProcessBaseString->sr))
    {
        return PhReferenceProcessItem(process->UniqueProcessId);
    }

    lastTickTotal = tickCount;

    if (processInfo)
    {
        PhFree(processInfo);
        processInfo = NULL;
    }

    PhEnumProcesses(&processInfo);

    if (process = PhFindProcessInformationByImageName(processInfo, &ProcessBaseString->sr))
    {
        return PhReferenceProcessItem(process->UniqueProcessId);
    }

    return NULL;
}

#define NETWORKTOOLS_PLUGIN_NAME L"ProcessHacker.NetworkTools"
#define NETWORKTOOLS_INTERFACE_VERSION 1

typedef BOOLEAN (NTAPI* PNETWORKTOOLS_GET_COUNTRYCODE)(
    _In_ PH_IP_ADDRESS RemoteAddress,
    _Out_ PPH_STRING* CountryCode,
    _Out_ PPH_STRING* CountryName
    );

typedef INT (NTAPI* PNETWORKTOOLS_GET_COUNTRYICON)(
    _In_ PPH_STRING Name
    );

typedef VOID (NTAPI* PNETWORKTOOLS_DRAW_COUNTRYICON)(
    _In_ HDC hdc,
    _In_ RECT rect,
    _In_ INT Index
    );

typedef struct _NETWORKTOOLS_INTERFACE
{
    ULONG Version;
    PNETWORKTOOLS_GET_COUNTRYCODE LookupCountryCode;
    PNETWORKTOOLS_GET_COUNTRYICON LookupCountryIcon;
    PNETWORKTOOLS_DRAW_COUNTRYICON DrawCountryIcon;
} NETWORKTOOLS_INTERFACE, *PNETWORKTOOLS_INTERFACE;

PNETWORKTOOLS_INTERFACE EtFwGetPluginInterface(
    VOID
    )
{
    static PNETWORKTOOLS_INTERFACE pluginInterface = NULL;
    PPH_PLUGIN toolStatusPlugin;

    if (!pluginInterface && (toolStatusPlugin = PhFindPlugin(NETWORKTOOLS_PLUGIN_NAME)))
    {
        pluginInterface = PhGetPluginInformation(toolStatusPlugin)->Interface;

        if (pluginInterface->Version < NETWORKTOOLS_INTERFACE_VERSION)
            pluginInterface = NULL;
    }

    return pluginInterface;
}

VOID EtFwDrawCountryIcon(
    _In_ HDC hdc,
    _In_ RECT rect,
    _In_ INT Index
    )
{
    if (EtFwGetPluginInterface())
        EtFwGetPluginInterface()->DrawCountryIcon(hdc, rect, Index);
}

VOID CALLBACK EtFwEventCallback(
    _Inout_ PVOID FwContext,
    _In_ const FWPM_NET_EVENT* FwEvent
    )
{
    FW_EVENT entry;
    BOOLEAN isLoopback = FALSE;
    ULONG direction = ULONG_MAX;
    ULONG64 filterId;
    USHORT layerId;
    ULONG reauthReason;
    ULONG originalProfile;
    ULONG currentProfile;
    PPH_STRING ruleName = NULL;
    PPH_STRING ruleDescription = NULL;
    PPH_STRING ruleLayerName = NULL;
    PPH_STRING ruleLayerDescription = NULL;
    FWPM_LAYER* layer = NULL;
    FWPM_FILTER* filter = NULL;

    if (!EtFwEnabled)
        return;

    if (!FwProcessEventType(
        FwEvent,
        &isLoopback,
        &direction,
        &filterId,
        &layerId,
        &reauthReason,
        &originalProfile,
        &currentProfile
        ))
    {
        return;
    }

    if (FwpmLayerGetById(EtFwEngineHandle, layerId, &layer) == ERROR_SUCCESS)
    {
        if (
            IsEqualGUID(&layer->layerKey, &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4) ||
            IsEqualGUID(&layer->layerKey, &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6) ||
            IsEqualGUID(&layer->layerKey, &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4) ||
            IsEqualGUID(&layer->layerKey, &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6)
            )
        {
            FwpmFreeMemory(&layer);
            return;
        }

        if (layer->displayData.name)
            ruleLayerName = PhCreateString(layer->displayData.name);
        if (layer->displayData.description)
            ruleLayerDescription = PhCreateString(layer->displayData.description);

        FwpmFreeMemory(&layer);
    }

    if (FwpmFilterGetById(EtFwEngineHandle, filterId, &filter) == ERROR_SUCCESS)
    {
        if (filter->displayData.name)
            ruleName = PhCreateString(filter->displayData.name);
        if (filter->displayData.description)
            ruleDescription = PhCreateString(filter->displayData.description);

        FwpmFreeMemory(&filter);
    }

    memset(&entry, 0, sizeof(FW_EVENT));
    entry.Type = FwEvent->type;
    entry.IsLoopback = isLoopback;
    entry.Direction = direction;
    entry.RuleName = ruleName;
    entry.RuleDescription = ruleDescription;
    entry.RuleLayerName = ruleLayerName;
    entry.RuleLayerDescription = ruleLayerDescription;

    if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_IP_PROTOCOL_SET)
    {
        entry.IpProtocol = FwEvent->header.ipProtocol;
    }

    if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_IP_VERSION_SET)
    {
        if (FwEvent->header.ipVersion == FWP_IP_VERSION_V4)
        {
            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_LOCAL_ADDR_SET)
            {
                entry.LocalEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                entry.LocalEndpoint.Address.Ipv4 = _byteswap_ulong(FwEvent->header.localAddrV4);
            }

            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_LOCAL_PORT_SET)
            {
                entry.LocalEndpoint.Port = FwEvent->header.localPort;
            }

            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_REMOTE_ADDR_SET)
            {
                entry.RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                entry.RemoteEndpoint.Address.Ipv4 = _byteswap_ulong(FwEvent->header.remoteAddrV4);
            }

            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_REMOTE_PORT_SET)
            {
                entry.RemoteEndpoint.Port = FwEvent->header.remotePort;
            }
        }
        else if (FwEvent->header.ipVersion == FWP_IP_VERSION_V6)
        {
            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_LOCAL_ADDR_SET)
            {
                entry.LocalEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                memcpy(entry.LocalEndpoint.Address.Ipv6, FwEvent->header.localAddrV6.byteArray16, 16);
            }

            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_LOCAL_PORT_SET)
            {
                entry.LocalEndpoint.Port = FwEvent->header.localPort;
            }

            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_REMOTE_ADDR_SET)
            {
                entry.RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                memcpy(entry.RemoteEndpoint.Address.Ipv6, FwEvent->header.remoteAddrV6.byteArray16, 16);
            }

            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_REMOTE_PORT_SET)
            {
                entry.RemoteEndpoint.Port = FwEvent->header.remotePort;
            }
        }
    }

    if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_APP_ID_SET)
    {
        if (FwEvent->header.appId.data && FwEvent->header.appId.size)
        {
            PPH_STRING fileName;

            fileName = PhCreateStringEx((PWSTR)FwEvent->header.appId.data, (SIZE_T)FwEvent->header.appId.size);
            entry.ProcessFileName = PhGetFileName(fileName);

            PhDereferenceObject(fileName);
        }

        if (entry.ProcessFileName)
        {
            if (entry.ProcessBaseString = PhGetBaseName(entry.ProcessFileName))
            {
                entry.ProcessItem = EtFwFileNameToProcess(entry.ProcessBaseString);
            }
        }
    }

    if (EtFwGetPluginInterface())
    {
        PPH_STRING remoteCountryCode;
        PPH_STRING remoteCountryName;

        if (EtFwGetPluginInterface()->LookupCountryCode(
            entry.RemoteEndpoint.Address,
            &remoteCountryCode,
            &remoteCountryName
            ))
        {
            PhMoveReference(&entry.RemoteCountryName, remoteCountryName);
            entry.CountryIconIndex = EtFwGetPluginInterface()->LookupCountryIcon(remoteCountryCode);

            PhDereferenceObject(remoteCountryCode);
        }
    }

    FwPushFirewallEvent(&entry);
}

VOID NTAPI EtFwProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PSLIST_ENTRY listEntry;
    PLIST_ENTRY ageListEntry;

    // Process incoming event packets.

    listEntry = RtlInterlockedFlushSList(&FwPacketListHead);

    while (listEntry)
    {
        PFW_EVENT_PACKET packet;

        packet = CONTAINING_RECORD(listEntry, FW_EVENT_PACKET, ListEntry);
        listEntry = listEntry->Next;

        FwProcessFirewallEvent(packet, FwRunCount);

        PhFree(packet);
    }

    // merge into below loop

    EtFwFlushHostNameData();

    ageListEntry = FwAgeListHead.Blink;

    while (ageListEntry != &FwAgeListHead)
    {
        PFW_EVENT_ITEM item;
        BOOLEAN modified = FALSE;

        item = CONTAINING_RECORD(ageListEntry, FW_EVENT_ITEM, AgeListEntry);
        ageListEntry = ageListEntry->Blink;

        if (InterlockedExchange(&item->JustResolved, 0) != 0)
            modified = TRUE;

        if (modified)
        {
            PhInvokeCallback(&FwItemModifiedEvent, item);
        }
    }

    // Remove old entries.

    ageListEntry = FwAgeListHead.Blink;

    while (ageListEntry != &FwAgeListHead)
    {
        PFW_EVENT_ITEM item;

        item = CONTAINING_RECORD(ageListEntry, FW_EVENT_ITEM, AgeListEntry);
        ageListEntry = ageListEntry->Blink;

        if (FwRunCount - item->FreshTime < FwMaxEventAge)
            break;

        PhInvokeCallback(&FwItemRemovedEvent, item);
        RemoveEntryList(&item->AgeListEntry);
    }

    PhInvokeCallback(&FwItemsUpdatedEvent, NULL);
    FwRunCount++;
}

ULONG EtFwStartMonitor(
    VOID
    )
{
    PVOID moduleHandle;
    ULONG status;
    FWP_VALUE value = { FWP_EMPTY };
    FWPM_SESSION session = { 0 };
    FWPM_NET_EVENT_SUBSCRIPTION subscription = { 0 };
    FWPM_NET_EVENT_ENUM_TEMPLATE eventTemplate = { 0 };

    RtlInitializeSListHead(&FwPacketListHead);
    RtlInitializeSListHead(&EtFwQueryListHead);
    FwObjectType = PhCreateObjectType(L"FwObject", 0, FwObjectTypeDeleteProcedure);
    EtFwResolveCacheHashtable = PhCreateHashtable(
        sizeof(FW_RESOLVE_CACHE_ITEM),
        EtFwResolveCacheHashtableEqualFunction,
        EtFwResolveCacheHashtableHashFunction,
        20
        );

    if (!(moduleHandle = LoadLibrary(L"fwpuclnt.dll")))
        return GetLastError();
    if (!FwpmNetEventSubscribe_I)
        FwpmNetEventSubscribe_I = PhGetProcedureAddress(moduleHandle, "FwpmNetEventSubscribe4", 0);
    if (!FwpmNetEventSubscribe_I)
        FwpmNetEventSubscribe_I = PhGetProcedureAddress(moduleHandle, "FwpmNetEventSubscribe3", 0);
    if (!FwpmNetEventSubscribe_I)
        FwpmNetEventSubscribe_I = PhGetProcedureAddress(moduleHandle, "FwpmNetEventSubscribe2", 0);
    if (!FwpmNetEventSubscribe_I)
        FwpmNetEventSubscribe_I = PhGetProcedureAddress(moduleHandle, "FwpmNetEventSubscribe1", 0);
    if (!FwpmNetEventSubscribe_I)
        return ERROR_PROC_NOT_FOUND;

    session.flags = 0;// FWPM_SESSION_FLAG_DYNAMIC;
    session.displayData.name  = L"PhFwSession";
    session.displayData.description = L"";

    // Create a non-dynamic BFE session
    status = FwpmEngineOpen(
        NULL,
        RPC_C_AUTHN_WINNT,
        NULL,
        &session,
        &EtFwEngineHandle
        );

    if (status != ERROR_SUCCESS)
        return status;

    // Enable collection of NetEvents

    value.type = FWP_UINT32;
    value.uint32 = 1;

    status = FwpmEngineSetOption(EtFwEngineHandle, FWPM_ENGINE_COLLECT_NET_EVENTS, &value);

    if (status != ERROR_SUCCESS)
        return status;

    if (WindowsVersion >= WINDOWS_8)
    {
        value.type = FWP_UINT32;
        value.uint32 =
            FWPM_NET_EVENT_KEYWORD_CAPABILITY_DROP | FWPM_NET_EVENT_KEYWORD_CAPABILITY_ALLOW |
            FWPM_NET_EVENT_KEYWORD_CLASSIFY_ALLOW | FWPM_NET_EVENT_KEYWORD_INBOUND_MCAST |
            FWPM_NET_EVENT_KEYWORD_INBOUND_BCAST;

        if (WindowsVersion >= WINDOWS_10_19H1)
            value.uint32 |= FWPM_NET_EVENT_KEYWORD_PORT_SCANNING_DROP;

        FwpmEngineSetOption(EtFwEngineHandle, FWPM_ENGINE_NET_EVENT_MATCH_ANY_KEYWORDS, &value);

        value.type = FWP_UINT32;
        value.uint32 = 1;

        FwpmEngineSetOption(EtFwEngineHandle, FWPM_ENGINE_MONITOR_IPSEC_CONNECTIONS, &value);
    }

    eventTemplate.numFilterConditions = 0; // get events for all conditions
    subscription.sessionKey = session.sessionKey;
    subscription.enumTemplate = &eventTemplate;

    // Subscribe to the events

    status = FwpmNetEventSubscribe_I(
        EtFwEngineHandle,
        &subscription,
        EtFwEventCallback,
        NULL,
        &FwEventHandle
        );

    if (status != ERROR_SUCCESS)
        return status;

    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
        EtFwProcessesUpdatedCallback,
        NULL,
        &EtFwProcessesUpdatedCallbackRegistration
        );

    return ERROR_SUCCESS;
}

VOID EtFwStopMonitor(
    VOID
    )
{
    if (FwEventHandle)
    {
        FwpmNetEventUnsubscribe(EtFwEngineHandle, FwEventHandle);
        FwEventHandle = NULL;
    }

    if (EtFwEngineHandle)
    {
        FWP_VALUE value = { FWP_EMPTY };

        // Disable collection of NetEvents

        value.type = FWP_UINT32;
        value.uint32 = 0;

        FwpmEngineSetOption(EtFwEngineHandle, FWPM_ENGINE_COLLECT_NET_EVENTS, &value);

        FwpmEngineClose(EtFwEngineHandle);
        EtFwEngineHandle = NULL;
    }
}
