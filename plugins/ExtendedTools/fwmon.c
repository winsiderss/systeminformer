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
#include <fwpmu.h>
#include <fwpsu.h>

PH_CALLBACK_REGISTRATION EtFwProcessesUpdatedCallbackRegistration;
PPH_OBJECT_TYPE FwObjectType = NULL;
HANDLE FwEngineHandle = NULL;
HANDLE FwEventHandle = NULL;
HANDLE FwEnumHandle = NULL;
ULONG FwRunCount = 0;
ULONG FwMaxEventAge = 60;
SLIST_HEADER FwPacketListHead;
LIST_ENTRY FwAgeListHead = { &FwAgeListHead, &FwAgeListHead };
_FwpmNetEventSubscribe FwpmNetEventSubscribe_I = NULL;
PPH_HASHTABLE EtFwResolveCacheHashtable = NULL;

PH_CALLBACK_DECLARE(FwItemAddedEvent);
PH_CALLBACK_DECLARE(FwItemModifiedEvent);
PH_CALLBACK_DECLARE(FwItemRemovedEvent);
PH_CALLBACK_DECLARE(FwItemsUpdatedEvent);

typedef struct _FW_EVENT
{
    FWPM_NET_EVENT_TYPE Type;
    BOOL IsLoopback;
    ULONG Direction;
    ULONG ProcessId;
    ULONG IpProtocol;

    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;

    PPH_STRING LocalAddressString;
    PPH_STRING RemoteAddressString;
    PPH_STRING LocalHostnameString;
    PPH_STRING RemoteHostnameString;

    PPH_STRING RuleName;
    PPH_STRING RuleDescription;
    PPH_STRING RuleLayerName;
    PPH_STRING RuleLayerDescription;

    PPH_STRING ProcessFileName;
    PPH_STRING ProcessFileNameWin32;
    PPH_STRING ProcessBaseString;

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

    if (event->TimeString)
        PhDereferenceObject(event->TimeString);
    if (event->ProcessFileName)
        PhDereferenceObject(event->ProcessFileName);
    if (event->ProcessFileNameWin32)
        PhDereferenceObject(event->ProcessFileNameWin32);
    if (event->ProcessBaseString)
        PhDereferenceObject(event->ProcessBaseString);
    if (event->LocalAddressString)
        PhDereferenceObject(event->LocalAddressString);
    if (event->RemoteAddressString)
        PhDereferenceObject(event->RemoteAddressString);
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
    SYSTEMTIME systemTime;

    entry = FwCreateEventItem();
    PhQuerySystemTime(&entry->AddedTime);
    PhLargeIntegerToLocalSystemTime(&systemTime, &entry->AddedTime);
    entry->TimeString = PhFormatDateTime(&systemTime);
    entry->Direction = diskEvent->Direction;
    entry->Type = diskEvent->Type;
    entry->IpProtocol = diskEvent->IpProtocol;

    entry->LocalEndpoint = diskEvent->LocalEndpoint;
    entry->LocalAddressString = diskEvent->LocalAddressString;
    entry->LocalHostnameString = diskEvent->LocalHostnameString;
    entry->RemoteEndpoint = diskEvent->RemoteEndpoint;
    entry->RemoteAddressString = diskEvent->RemoteAddressString;
    entry->RemoteHostnameString = diskEvent->RemoteHostnameString;

    entry->ProcessFileName = diskEvent->ProcessFileName;
    entry->ProcessFileNameWin32 = diskEvent->ProcessFileNameWin32;
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

    // Add the item to the age list.
    entry->AddTime = RunId;
    entry->FreshTime = RunId;
    InsertHeadList(&FwAgeListHead, &entry->AgeListEntry);

    PhReferenceObject(entry);

    // Raise the item added event.
    PhInvokeCallback(&FwItemAddedEvent, entry);
}

_Success_(return)
BOOLEAN FwProcessEventType(
    _In_ const FWPM_NET_EVENT* FwEvent,
    _Out_ PBOOLEAN IsLoopback,
    _Out_ PULONG Direction,
    _Out_opt_ PPH_STRING* RuleName,
    _Out_opt_ PPH_STRING* RuleDescription,
    _Out_opt_ PPH_STRING* RuleLayerName,
    _Out_opt_ PPH_STRING* RuleLayerDescription
    )
{
    switch (FwEvent->type)
    {
    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP:
        {
            FWPM_FILTER* fwFilterItem = NULL;
            FWPM_LAYER* fwLayerItem = NULL;
            FWPM_NET_EVENT_CLASSIFY_DROP* fwDropEvent = FwEvent->classifyDrop;

            if (IsLoopback)
                *IsLoopback = fwDropEvent->isLoopback;

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
                return FALSE;
            }

            if (fwDropEvent->layerId && FwpmLayerGetById(FwEngineHandle, fwDropEvent->layerId, &fwLayerItem) == ERROR_SUCCESS)
            {
                if (
                    IsEqualGUID(&fwLayerItem->layerKey, &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4) ||
                    IsEqualGUID(&fwLayerItem->layerKey, &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6) ||
                    IsEqualGUID(&fwLayerItem->layerKey, &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4) ||
                    IsEqualGUID(&fwLayerItem->layerKey, &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6)
                    )
                {
                    FwpmFreeMemory(&fwLayerItem);
                    return FALSE;
                }

                if (RuleLayerName && fwLayerItem->displayData.name)
                    *RuleLayerName = PhCreateString(fwLayerItem->displayData.name);
                if (RuleLayerDescription && fwLayerItem->displayData.description)
                    *RuleLayerDescription = PhCreateString(fwLayerItem->displayData.description);

                FwpmFreeMemory(&fwLayerItem);
            }

            if (fwDropEvent->filterId && FwpmFilterGetById(FwEngineHandle, fwDropEvent->filterId, &fwFilterItem) == ERROR_SUCCESS)
            {
                if (RuleName && fwFilterItem->displayData.name)
                    *RuleName = PhCreateString(fwFilterItem->displayData.name);
                if (RuleDescription && fwFilterItem->displayData.description)
                    *RuleDescription = PhCreateString(fwFilterItem->displayData.description);

                FwpmFreeMemory(&fwFilterItem);
            }
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_CLASSIFY_ALLOW:
        {
            FWPM_FILTER* fwFilterItem = NULL;
            FWPM_LAYER* fwLayerItem = NULL;
            FWPM_NET_EVENT_CLASSIFY_ALLOW* fwAllowEvent = FwEvent->classifyAllow;

            if (IsLoopback)
                *IsLoopback = fwAllowEvent->isLoopback;

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
                return FALSE;
            }

            if (fwAllowEvent->layerId && FwpmLayerGetById(FwEngineHandle, fwAllowEvent->layerId, &fwLayerItem) == ERROR_SUCCESS)
            {
                if (
                    IsEqualGUID(&fwLayerItem->layerKey, &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4) ||
                    IsEqualGUID(&fwLayerItem->layerKey, &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6) ||
                    IsEqualGUID(&fwLayerItem->layerKey, &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4) ||
                    IsEqualGUID(&fwLayerItem->layerKey, &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6)
                    )
                {
                    FwpmFreeMemory(&fwLayerItem);
                    return FALSE;
                }

                if (RuleLayerName && fwLayerItem->displayData.name)
                    *RuleLayerName = PhCreateString(fwLayerItem->displayData.name);
                if (RuleLayerDescription && fwLayerItem->displayData.description)
                    *RuleLayerDescription = PhCreateString(fwLayerItem->displayData.description);

                FwpmFreeMemory(&fwLayerItem);
            }

            if (fwAllowEvent->filterId && FwpmFilterGetById(FwEngineHandle, fwAllowEvent->filterId, &fwFilterItem) == ERROR_SUCCESS)
            {
                if (RuleName && fwFilterItem->displayData.name)
                    *RuleName = PhCreateString(fwFilterItem->displayData.name);
                if (RuleDescription && fwFilterItem->displayData.description)
                    *RuleDescription = PhCreateString(fwFilterItem->displayData.description);

                FwpmFreeMemory(&fwFilterItem);
            }
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
    PDNS_RECORD dnsRecordList;

    if (dnsEndpointReverseString = EtFwGetDnsReverseNameFromAddress(Address))
    {
        if (dnsRecordList = PhDnsQuery2(NULL, dnsEndpointReverseString->Buffer, DNS_TYPE_PTR, DNS_QUERY_NO_WIRE_QUERY))
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

    return addressEndpointString;
}

VOID PhpQueryHostnameForEntry(
    _In_ PFW_EVENT Entry
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
        if (cacheItem = EtFwLookupResolveCacheItem(&Entry->LocalEndpoint.Address))
        {
            PhReferenceObject(cacheItem->HostString);
            Entry->LocalHostnameString = cacheItem->HostString;
        }
        else
        {
            cacheItem = PhAllocateZero(sizeof(FW_RESOLVE_CACHE_ITEM));
            cacheItem->Address = Entry->LocalEndpoint.Address;

            switch (Entry->LocalEndpoint.Address.Type)
            {
            case PH_IPV4_NETWORK_TYPE:
                {
                    if (IN4_IS_ADDR_UNSPECIFIED(&Entry->LocalEndpoint.Address.InAddr))
                    {
                        cacheItem->HostString = PhCreateString(L"UNSPECIFIED");
                    }
                    else if (IN4_IS_ADDR_LOOPBACK(&Entry->LocalEndpoint.Address.InAddr))
                    {
                        cacheItem->HostString = PhCreateString(L"LOOPBACK");
                    }
                    else if (IN4_IS_ADDR_BROADCAST(&Entry->LocalEndpoint.Address.InAddr))
                    {
                        cacheItem->HostString = PhCreateString(L"BROADCAST");
                    }
                }
                break;
            case PH_IPV6_NETWORK_TYPE:
                {
                    if (IN6_IS_ADDR_UNSPECIFIED(&Entry->LocalEndpoint.Address.In6Addr))
                    {
                        cacheItem->HostString = PhCreateString(L"UNSPECIFIED");
                    }
                    else if (IN6_IS_ADDR_LOOPBACK(&Entry->LocalEndpoint.Address.In6Addr))
                    {
                        cacheItem->HostString = PhCreateString(L"LOOPBACK");
                    }
                }
                break;
            }

            if (!cacheItem->HostString)
            {
                cacheItem->HostString = EtFwGetNameFromAddress(&Entry->LocalEndpoint.Address);
            }

            if (!cacheItem->HostString)
            {
                cacheItem->HostString = PhReferenceEmptyString();
                PhReferenceObject(cacheItem->HostString);
            }

            PhAddEntryHashtable(EtFwResolveCacheHashtable, &cacheItem);
            Entry->LocalHostnameString = cacheItem->HostString;
        }
    }

    // Remote
    if (!PhIsNullIpAddress(&Entry->RemoteEndpoint.Address))
    {
        if (cacheItem = EtFwLookupResolveCacheItem(&Entry->RemoteEndpoint.Address))
        {
            PhReferenceObject(cacheItem->HostString);
            Entry->RemoteHostnameString = cacheItem->HostString;
        }
        else
        {
            cacheItem = PhAllocateZero(sizeof(FW_RESOLVE_CACHE_ITEM));
            cacheItem->Address = Entry->RemoteEndpoint.Address;

            switch (Entry->RemoteEndpoint.Address.Type)
            {
            case PH_IPV4_NETWORK_TYPE:
                {
                    if (IN4_IS_ADDR_UNSPECIFIED(&Entry->RemoteEndpoint.Address.InAddr))
                    {
                        cacheItem->HostString = PhCreateString(L"UNSPECIFIED");
                    }
                    else if (IN4_IS_ADDR_LOOPBACK(&Entry->RemoteEndpoint.Address.InAddr))
                    {
                        cacheItem->HostString = PhCreateString(L"LOOPBACK");
                    }
                    else if (IN4_IS_ADDR_BROADCAST(&Entry->RemoteEndpoint.Address.InAddr))
                    {
                        cacheItem->HostString = PhCreateString(L"BROADCAST");
                    }
                }
                break;
            case PH_IPV6_NETWORK_TYPE:
                {
                    if (IN6_IS_ADDR_UNSPECIFIED(&Entry->RemoteEndpoint.Address.In6Addr))
                    {
                        cacheItem->HostString = PhCreateString(L"UNSPECIFIED");
                    }
                    else if (IN6_IS_ADDR_LOOPBACK(&Entry->RemoteEndpoint.Address.In6Addr))
                    {
                        cacheItem->HostString = PhCreateString(L"LOOPBACK");
                    }
                }
                break;
            }

            if (!cacheItem->HostString)
            {
                cacheItem->HostString = EtFwGetNameFromAddress(&Entry->RemoteEndpoint.Address);
            }

            if (!cacheItem->HostString)
            {
                cacheItem->HostString = PhReferenceEmptyString();
                PhReferenceObject(cacheItem->HostString);
            }

            PhAddEntryHashtable(EtFwResolveCacheHashtable, &cacheItem);
            Entry->RemoteHostnameString = cacheItem->HostString;
        }
    }
}

VOID CALLBACK FwEventCallback(
    _Inout_ PVOID FwContext,
    _In_ const FWPM_NET_EVENT* FwEvent
    )
{
    FW_EVENT entry;
    BOOLEAN isLoopback = FALSE;
    ULONG direction = ULONG_MAX;
    PPH_STRING ruleName = NULL;
    PPH_STRING ruleDescription = NULL;
    PPH_STRING ruleLayerName = NULL;
    PPH_STRING ruleLayerDescription = NULL;

    if (!EtFwEnabled)
        return;

    if (!FwProcessEventType(
        FwEvent,
        &isLoopback,
        &direction,
        &ruleName,
        &ruleDescription,
        &ruleLayerName,
        &ruleLayerDescription
        ))
    {
        return;
    }

    memset(&entry, 0, sizeof(FW_EVENT));
    entry.Type = FwEvent->type;
    entry.IsLoopback = isLoopback;
    entry.Direction = direction;
    entry.RuleName = ruleName;
    entry.RuleDescription = ruleDescription;
    entry.RuleLayerName = ruleLayerName;
    entry.RuleLayerDescription = ruleLayerDescription;
    entry.IpProtocol = FwEvent->header.ipProtocol;

    if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_IP_VERSION_SET)
    {
        if (FwEvent->header.ipVersion == FWP_IP_VERSION_V4)
        {
            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_LOCAL_ADDR_SET)
            {
                ULONG ipvAddressStringLength = INET6_ADDRSTRLEN;
                WCHAR ipvAddressString[INET6_ADDRSTRLEN] = L"";
                ULONG localAddrV4 = _byteswap_ulong(FwEvent->header.localAddrV4);
                entry.LocalEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                entry.LocalEndpoint.Address.Ipv4 = localAddrV4;
                entry.LocalEndpoint.Port = FwEvent->header.localPort;

                if (NT_SUCCESS(RtlIpv4AddressToStringEx((PIN_ADDR)&localAddrV4, 0, ipvAddressString, &ipvAddressStringLength)))
                {
                    entry.LocalAddressString = PhCreateStringEx(ipvAddressString, ipvAddressStringLength * sizeof(WCHAR));
                }
            }

            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_REMOTE_ADDR_SET)
            {
                ULONG ipvAddressStringLength = INET6_ADDRSTRLEN;
                WCHAR ipvAddressString[INET6_ADDRSTRLEN] = L"";
                ULONG remoteAddrV4 = _byteswap_ulong(FwEvent->header.remoteAddrV4);
                entry.RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                entry.RemoteEndpoint.Address.Ipv4 = remoteAddrV4;
                entry.RemoteEndpoint.Port = FwEvent->header.remotePort;

                if (NT_SUCCESS(RtlIpv4AddressToStringEx((PIN_ADDR)&remoteAddrV4, 0, ipvAddressString, &ipvAddressStringLength)))
                {
                    entry.RemoteAddressString = PhCreateStringEx(ipvAddressString, ipvAddressStringLength * sizeof(WCHAR));
                }
            }

            PhpQueryHostnameForEntry(&entry);
        }
        else if (FwEvent->header.ipVersion == FWP_IP_VERSION_V6)
        {
            entry.LocalEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
            memcpy(entry.LocalEndpoint.Address.Ipv6, FwEvent->header.localAddrV6.byteArray16, 16);
            entry.LocalEndpoint.Port = FwEvent->header.localPort;
            entry.RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
            memcpy(entry.RemoteEndpoint.Address.Ipv6, FwEvent->header.remoteAddrV6.byteArray16, 16);
            entry.RemoteEndpoint.Port = FwEvent->header.remotePort;

            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_LOCAL_ADDR_SET)
            {
                ULONG ipvAddressStringLength = INET6_ADDRSTRLEN;
                WCHAR ipvAddressString[INET6_ADDRSTRLEN] = L"";

                if (NT_SUCCESS(RtlIpv6AddressToStringEx((PIN6_ADDR)&FwEvent->header.localAddrV6, 0, 0, ipvAddressString, &ipvAddressStringLength)))
                {
                    entry.LocalAddressString = PhCreateStringEx(ipvAddressString, ipvAddressStringLength * sizeof(WCHAR));
                }
            }

            if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_REMOTE_ADDR_SET)
            {
                ULONG ipvAddressStringLength = INET6_ADDRSTRLEN;
                WCHAR ipvAddressString[INET6_ADDRSTRLEN] = L"";

                if (NT_SUCCESS(RtlIpv6AddressToStringEx((PIN6_ADDR)&FwEvent->header.remoteAddrV6, 0, 0, ipvAddressString, &ipvAddressStringLength)))
                {
                    entry.RemoteAddressString = PhCreateStringEx(ipvAddressString, ipvAddressStringLength * sizeof(WCHAR));
                }
            }

            PhpQueryHostnameForEntry(&entry);
        }
    }

    if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_APP_ID_SET)
    {
        if (FwEvent->header.appId.data && FwEvent->header.appId.size)
        {
            PPH_STRING fileName = PhCreateStringEx((PWSTR)FwEvent->header.appId.data, (SIZE_T)FwEvent->header.appId.size);
            entry.ProcessFileName = PhGetFileName(fileName);
            PhDereferenceObject(fileName);
        }

        if (entry.ProcessFileName)
        {
            PVOID processes;
            PSYSTEM_PROCESS_INFORMATION processInfo;

            entry.ProcessFileNameWin32 = entry.ProcessFileName;
            entry.ProcessBaseString = PhGetBaseName(entry.ProcessFileName);

            if (entry.ProcessBaseString && NT_SUCCESS(PhEnumProcesses(&processes)))
            {
                if (processInfo = PhFindProcessInformationByImageName(processes, &entry.ProcessBaseString->sr))
                {
                    entry.ProcessItem = PhReferenceProcessItem(processInfo->UniqueProcessId);
                }

                PhFree(processes);
            }

            //if (node->ProcessIconValid && node->ProcessIcon)
            //{
            //    getNodeIcon->Icon = node->ProcessIcon;
            //}
            //else
            //{
            //    if (!node->ProcessIconValid && node->ProcessFileName)
            //    {
            //        PPH_PROCESS_ITEM* processes;
            //        ULONG numberOfProcesses;
            //        ULONG i;
            //
            //        PhEnumProcessItems(&processes, &numberOfProcesses);
            //
            //        for (i = 0; i < numberOfProcesses; i++)
            //        {
            //            PPH_PROCESS_ITEM process = processes[i];
            //
            //            if (process->FileName && PhEqualString(process->FileName, node->ProcessFileName, TRUE))
            //            {
            //                if (PhTestEvent(&process->Stage1Event))
            //                {
            //                    PhSetReference(&node->ProcessItem, process);
            //                    node->ProcessIcon = process->SmallIcon;
            //                    node->ProcessIconValid = TRUE;
            //                    break;
            //                }
            //            }
            //        }
            //
            //        PhDereferenceObjects(processes, numberOfProcesses);
            //    }
            //}

            //FWP_BYTE_BLOB* fwpApplicationByteBlob = NULL;
            //if (FwpmGetAppIdFromFileName(fileNameString->Buffer, &fwpApplicationByteBlob) == ERROR_SUCCESS)
            //fwEventItem->ProcessBaseString = PhCreateStringEx(fwpApplicationByteBlob->data, fwpApplicationByteBlob->size);        
            //fwEventItem->Icon = PhGetFileShellIcon(PhGetString(fwEventItem->ProcessFileName), L".exe", FALSE);
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
        FwEventCallback,
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
        FwpmNetEventUnsubscribe(FwEngineHandle, FwEventHandle);
        FwEventHandle = NULL;
    }

    if (FwEngineHandle)
    {
        FWP_VALUE value = { FWP_EMPTY };

        // Disable collection of NetEvents

        value.type = FWP_UINT32;
        value.uint32 = 0;

        FwpmEngineSetOption(FwEngineHandle, FWPM_ENGINE_COLLECT_NET_EVENTS, &value);

        FwpmEngineClose(FwEngineHandle);
        FwEngineHandle = NULL;
    }
}
