/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2024
 *
 */

#include "exttools.h"
#include <networktoolsintf.h>
#include <workqueue.h>
#include <fwpmu.h>
#include <fwpsu.h>

#include "fwmon.h"

ULONG EtFwFlagsMask = 0;
PH_CALLBACK_REGISTRATION EtFwProcessesUpdatedCallbackRegistration;
PPH_OBJECT_TYPE EtFwObjectType = NULL;
HANDLE EtFwEngineHandle = NULL;
HANDLE EtFwEventHandle = NULL;
HANDLE EtFwFilterEventHandle = NULL;
ULONG FwRunCount = 0;
ULONG EtFwMaxEventAge = 60;
SLIST_HEADER EtFwPacketListHead;
PH_FREE_LIST EtFwPacketFreeList;
RTL_STATIC_LIST_HEAD(EtFwAgeListHead);
BOOLEAN EtFwEnableResolveCache = TRUE;
BOOLEAN EtFwEnableResolveDoH = FALSE;
BOOLEAN EtFwIgnoreOnError = TRUE;
BOOLEAN EtFwIgnorePortScan = FALSE;
BOOLEAN EtFwIgnoreLoopback = TRUE;
BOOLEAN EtFwIgnoreAllow = FALSE;
PPH_STRING EtFwSessionName = NULL;
PH_INITONCE EtFwWorkQueueInitOnce = PH_INITONCE_INIT;
SLIST_HEADER EtFwQueryListHead;
PH_WORK_QUEUE EtFwWorkQueue;
PPH_HASHTABLE EtFwResolveCacheHashtable = NULL;
PH_QUEUED_LOCK EtFwCacheHashtableLock = PH_QUEUED_LOCK_INIT;
PPH_HASHTABLE EtFwSidFullNameCacheHashtable = NULL;
PH_QUEUED_LOCK EtFwSidFullNameCacheHashtableLock = PH_QUEUED_LOCK_INIT;
PPH_HASHTABLE EtFwFilterDisplayDataHashTable = NULL;
PH_QUEUED_LOCK EtFwFilterDisplayDataHashTableLock = PH_QUEUED_LOCK_INIT;
PPH_HASHTABLE EtFwFileNameProcessCacheHashTable = NULL;
PH_QUEUED_LOCK EtFwFileNameProcessCacheHashTableLock = PH_QUEUED_LOCK_INIT;

PH_CALLBACK_DECLARE(FwItemAddedEvent);
PH_CALLBACK_DECLARE(FwItemModifiedEvent);
PH_CALLBACK_DECLARE(FwItemRemovedEvent);
PH_CALLBACK_DECLARE(FwItemsUpdatedEvent);

_FwpmEngineOpen0 FwpmEngineOpen_I = NULL;
_FwpmEngineClose0 FwpmEngineClose_I = NULL;
_FwpmFreeMemory0 FwpmFreeMemory_I = NULL;
_FwpmEngineSetOption0 FwpmEngineSetOption_I = NULL;
_FwpmFilterGetById0 FwpmFilterGetById_I = NULL;
typeof(&FwpmFilterSubscribeChanges0) FwpmFilterSubscribeChanges_I = NULL;
typeof(&FwpmFilterUnsubscribeChanges0) FwpmFilterUnsubscribeChanges_I = NULL;
typeof(&FwpmLayerGetById0) FwpmLayerGetById_I = NULL;
typeof(&FwpmNetEventSubscribe4) FwpmNetEventSubscribe_I = NULL;
typeof(&FwpmNetEventUnsubscribe0) FwpmNetEventUnsubscribe_I = NULL;
typeof(&FwpmNetEventCreateEnumHandle0) FwpmNetEventCreateEnumHandle_I = NULL;
typeof(&FwpmNetEventCreateEnumHandleEx) FwpmNetEventCreateEnumHandleEx_I = NULL;
typeof(&FwpmNetEventDestroyEnumHandle0) FwpmNetEventDestroyEnumHandle_I = NULL;
typeof(&FwpmNetEventEnum5) FwpmNetEventEnum_I = NULL;

#undef FwpmEngineOpen
#undef FwpmEngineClose
#undef FwpmFreeMemory
#undef FwpmFilterGetById0
#undef FwpmFilterSubscribeChanges0
#undef FwpmFilterUnsubscribeChanges0
#undef FwpmEngineSetOption
#undef FwpmNetEventSubscribe
#undef FwpmNetEventUnsubscribe
#undef FwpmNetEventCreateEnumHandle
#undef FwpmNetEventDestroyEnumHandle
#undef FwpmNetEventEnum

#define FwpmEngineOpen FwpmEngineOpen_I
#define FwpmEngineClose FwpmEngineClose_I
#define FwpmFreeMemory FwpmFreeMemory_I
#define FwpmFilterGetById0 FwpmFilterGetById_I
#define FwpmFilterSubscribeChanges0 FwpmFilterSubscribeChanges_I
#define FwpmFilterUnsubscribeChanges0 FwpmFilterUnsubscribeChanges_I
#define FwpmLayerGetById0 FwpmLayerGetById_I
#define FwpmEngineSetOption FwpmEngineSetOption_I
#define FwpmNetEventSubscribe FwpmNetEventSubscribe_I
#define FwpmNetEventUnsubscribe FwpmNetEventUnsubscribe_I
#define FwpmNetEventCreateEnumHandle FwpmNetEventCreateEnumHandle_I
#define FwpmNetEventCreateEnumHandleEx FwpmNetEventCreateEnumHandleEx_I
#define FwpmNetEventDestroyEnumHandle FwpmNetEventDestroyEnumHandle_I
#define FwpmNetEventEnum FwpmNetEventEnum_I

typedef struct _FW_EVENT
{
    PPH_STRING ProcessName;
    PPH_PROCESS_ITEM ProcessItem;
    ULONG_PTR ProcessIconIndex;
    BOOLEAN ProcessIconValid;

    LARGE_INTEGER TimeStamp;
    FWPM_NET_EVENT_TYPE Type;
    ULONG IsLoopback;
    ULONG Direction;
    ULONG ProcessId;
    ULONG IpProtocol;

    ULONG64 FilterId;
    ULONG LayerId;
    ULONG OriginalProfile;
    ULONG CurrentProfile;

    ULONG ScopeId;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;

    PPH_STRING LocalHostnameString;
    PPH_STRING RemoteHostnameString;

    PPH_STRING RuleName;
    PPH_STRING RuleDescription;
    PPH_STRING ProcessFileName;
    PPH_STRING ProcessFileNameWin32;
    PPH_STRING ProcessBaseString;
    PPH_STRING RemoteCountryName;
    LONG CountryIconIndex;

    PSID UserSid;
    //PSID PackageSid;

    ULONG SubsystemProcess ;

    PPH_STRING FilterOrigin;
    PPH_STRING PolicyAppId;
    PPH_STRING ServiceSids;
    PPH_STRING FqbnName;

    PPH_STRING InterfaceLuidString;
    PPH_STRING CompartmentIdString;
    PPH_STRING ProcessIdString;

    NET_LUID InterfaceLuid;
    NET_IF_COMPARTMENT_ID CompartmentId;
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

VOID EtFwQueryHostnameForEntry(
    _In_ PFW_EVENT_ITEM Entry
    );

/**
 * Compares two hostname resolve cache entries.
 *
 * \param Entry1 The first resolve cache entry.
 * \param Entry2 The second resolve cache entry.
 * \return TRUE if the cached IP addresses are equal, otherwise FALSE.
 */
_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN EtFwResolveCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PFW_RESOLVE_CACHE_ITEM cacheItem1 = (PFW_RESOLVE_CACHE_ITEM)Entry1;
    PFW_RESOLVE_CACHE_ITEM cacheItem2 = (PFW_RESOLVE_CACHE_ITEM)Entry2;

    return PhEqualIpAddress(&cacheItem1->Address, &cacheItem2->Address);
}

/**
 * Computes the hash value for a hostname resolve cache entry.
 *
 * \param Entry The resolve cache entry.
 * \return The IP address hash value.
 */
_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG NTAPI EtFwResolveCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PFW_RESOLVE_CACHE_ITEM cacheItem = (PFW_RESOLVE_CACHE_ITEM)Entry;

    return PhHashIpAddress(&cacheItem->Address);
}

/**
 * Flushes the hostname resolve cache.
 *
 * \remarks The existing hashtable is released after dereferencing cached host
 * strings and a new empty hashtable is created for subsequent lookups.
 */
VOID EtFwFlushResolveCache(
    VOID
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PFW_RESOLVE_CACHE_ITEM entry;

    if (!EtFwResolveCacheHashtable)
        return;

    PhBeginEnumHashtable(EtFwResolveCacheHashtable, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
    {
        if (entry->HostString)
            PhDereferenceObject(entry->HostString);
    }

    PhDereferenceObject(EtFwResolveCacheHashtable);
    EtFwResolveCacheHashtable = PhCreateHashtable(
        sizeof(FW_RESOLVE_CACHE_ITEM),
        EtFwResolveCacheHashtableEqualFunction,
        EtFwResolveCacheHashtableHashFunction,
        20
    );
}

/**
 * Looks up a hostname resolve cache entry by IP address.
 *
 * \param Address The IP address to find.
 * \return The matching cache entry, or NULL if no entry exists.
 */
PFW_RESOLVE_CACHE_ITEM EtFwLookupResolveCacheItem(
    _In_ PPH_IP_ADDRESS Address
    )
{
    FW_RESOLVE_CACHE_ITEM lookupEntry;
    PFW_RESOLVE_CACHE_ITEM entry;

    if (!EtFwResolveCacheHashtable)
        return NULL;

    lookupEntry.Address = *Address;

    entry = (PFW_RESOLVE_CACHE_ITEM)PhFindEntryHashtable(
        EtFwResolveCacheHashtable,
        &lookupEntry
        );

    if (entry)
        return entry;
    else
        return NULL;
}

/**
 * Deletes a firewall event item object.
 *
 * \param Object The firewall event item object.
 * \param Flags Object deletion flags.
 * \remarks This function releases all referenced strings, process items and
 * allocated SIDs owned by the firewall event item.
 */
_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI FwObjectTypeDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PFW_EVENT_ITEM event = Object;

    if (event->ProcessFileName)
        PhDereferenceObject(event->ProcessFileName);
    if (event->ProcessFileNameWin32)
        PhDereferenceObject(event->ProcessFileNameWin32);
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
    if (event->RemoteCountryName)
        PhDereferenceObject(event->RemoteCountryName);
    if (event->TimeString)
        PhDereferenceObject(event->TimeString);
    if (event->UserName)
        PhDereferenceObject(event->UserName);
    if (event->UserSid)
        PhFree(event->UserSid);
    if (event->TooltipText)
        PhDereferenceObject(event->TooltipText);

    if (event->ProcessItem)
        PhDereferenceObject(event->ProcessItem);

    if (event->FilterOrigin)
        PhDereferenceObject(event->FilterOrigin);
    if (event->PolicyAppId)
        PhDereferenceObject(event->PolicyAppId);
    if (event->ServiceSids)
        PhDereferenceObject(event->ServiceSids);
    if (event->FqbnName)
        PhDereferenceObject(event->FqbnName);

    if (event->InterfaceLuidString)
        PhDereferenceObject(event->InterfaceLuidString);
    if (event->CompartmentIdString)
        PhDereferenceObject(event->CompartmentIdString);
    if (event->ProcessIdString)
        PhDereferenceObject(event->ProcessIdString);
}

/**
 * Creates a firewall event item.
 *
 * \return A new firewall event item with a unique index.
 */
PFW_EVENT_ITEM FwCreateEventItem(
    VOID
    )
{
    static ULONG64 index = 0;
    PFW_EVENT_ITEM entry;

    entry = PhCreateObjectZero(sizeof(FW_EVENT_ITEM), EtFwObjectType);
    entry->Index = ++index;

    return entry;
}

/**
 * Queues a firewall event packet for processing on the process provider update callback.
 *
 * \param Event The firewall event data to copy into the packet queue.
 */
VOID FwPushFirewallEvent(
    _In_ PFW_EVENT Event
    )
{
    PFW_EVENT_PACKET packet;

    packet = PhAllocateFromFreeList(&EtFwPacketFreeList);
    memcpy(&packet->Event, Event, sizeof(FW_EVENT));
    RtlInterlockedPushEntrySList(&EtFwPacketListHead, &packet->ListEntry);
}

/**
 * Converts a queued firewall event packet into a firewall event item.
 *
 * \param Packet The queued firewall event packet.
 * \param RunId The current firewall monitor run identifier.
 * \remarks The created item is inserted into the age list, queued for hostname
 * resolution and published through the item added callback.
 */
VOID FwProcessFirewallEvent(
    _In_ PFW_EVENT_PACKET Packet,
    _In_ ULONG RunId
    )
{
    PFW_EVENT firewallEvent = &Packet->Event;
    PFW_EVENT_ITEM entry;

    entry = FwCreateEventItem();
    entry->TimeStamp = firewallEvent->TimeStamp;
    entry->Direction = firewallEvent->Direction;
    entry->Type = firewallEvent->Type;
    entry->ScopeId = firewallEvent->ScopeId;
    entry->IpProtocol = firewallEvent->IpProtocol;
    entry->FilterId = firewallEvent->FilterId;
    entry->LayerId = firewallEvent->LayerId;
    entry->LocalEndpoint = firewallEvent->LocalEndpoint;
    entry->LocalHostnameString = firewallEvent->LocalHostnameString;
    entry->RemoteEndpoint = firewallEvent->RemoteEndpoint;
    entry->RemoteHostnameString = firewallEvent->RemoteHostnameString;
    entry->ProcessFileName = firewallEvent->ProcessFileName;
    entry->ProcessFileNameWin32 = firewallEvent->ProcessFileNameWin32;
    entry->ProcessBaseString = firewallEvent->ProcessBaseString;
    entry->ProcessItem = firewallEvent->ProcessItem;

    if (entry->ProcessItem && !entry->ProcessIconValid && PhTestEvent(&entry->ProcessItem->Stage1Event))
    {
        entry->ProcessIconIndex = entry->ProcessItem->SmallIconIndex;
        entry->ProcessIconValid = TRUE;
    }

    entry->UserSid = firewallEvent->UserSid;
    entry->RuleName = firewallEvent->RuleName;
    entry->RuleDescription = firewallEvent->RuleDescription;
    entry->CountryIconIndex = firewallEvent->CountryIconIndex;
    entry->RemoteCountryName = firewallEvent->RemoteCountryName;

    if (firewallEvent->FilterOrigin)
        entry->FilterOrigin = PhReferenceObject(firewallEvent->FilterOrigin);
    entry->InterfaceLuid = firewallEvent->InterfaceLuid;
    entry->CompartmentId = firewallEvent->CompartmentId;
    if (firewallEvent->PolicyAppId)
        entry->PolicyAppId = PhReferenceObject(firewallEvent->PolicyAppId);
    if (firewallEvent->ServiceSids)
        entry->ServiceSids = PhReferenceObject(firewallEvent->ServiceSids);
    if (firewallEvent->FqbnName)
        entry->FqbnName = PhReferenceObject(firewallEvent->FqbnName);

    if (firewallEvent->InterfaceLuidString)
        entry->InterfaceLuidString = PhReferenceObject(firewallEvent->InterfaceLuidString);
    if (firewallEvent->CompartmentIdString)
        entry->CompartmentIdString = PhReferenceObject(firewallEvent->CompartmentIdString);
    if (firewallEvent->ProcessIdString)
        entry->ProcessIdString = PhReferenceObject(firewallEvent->ProcessIdString);

    entry->ProcessId = firewallEvent->ProcessId;
    entry->InterfaceLuid = firewallEvent->InterfaceLuid;
    entry->CompartmentId = firewallEvent->CompartmentId;

    // Add the item to the age list.
    entry->RunId = RunId;
    InsertHeadList(&EtFwAgeListHead, &entry->AgeListEntry);

    // Queue hostname lookup.
    EtFwQueryHostnameForEntry(entry);

    // Raise the item added event.
    PhInvokeCallback(&FwItemAddedEvent, entry);
}

/**
 * Converts a WFP net event type into firewall monitor event metadata.
 *
 * \param FwEvent The WFP net event.
 * \param IsLoopback A variable which receives whether the event is loopback.
 * \param Direction A variable which receives the firewall event direction.
 * \param FilterId A variable which receives the WFP filter identifier.
 * \param LayerId A variable which receives the WFP layer identifier.
 * \return TRUE if the event type was recognized and should be processed,
 * otherwise FALSE.
 */
_Success_(return)
BOOLEAN FwProcessEventType(
    _In_ const FWPM_NET_EVENT* FwEvent,
    _Out_ PBOOLEAN IsLoopback,
    _Out_ PULONG Direction,
    _Out_ PULONG64 FilterId,
    _Out_ PUSHORT LayerId
    )
{
    switch ((ULONG)FwEvent->type)
    {
    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP:
        {
            FWPM_NET_EVENT_CLASSIFY_DROP* fwDropEvent = FwEvent->classifyDrop;

            //if (EtWindowsVersion >= WINDOWS_10 && fwDropEvent->isLoopback && EtFwIgnoreLoopback)
            //    return FALSE;

            switch (fwDropEvent->msFwpDirection)
            {
            case FWP_DIRECTION_INBOUND:
            case FWP_DIRECTION_MAP_INBOUND:
                *Direction = FW_EVENT_DIRECTION_INBOUND;
                break;
            case FWP_DIRECTION_OUTBOUND:
            case FWP_DIRECTION_MAP_OUTBOUND:
                *Direction = FW_EVENT_DIRECTION_OUTBOUND;
                break;
            case FWP_DIRECTION_MAP_FORWARD:
                *Direction = FW_EVENT_DIRECTION_FORWARD;
                break;
            case FWP_DIRECTION_MAP_BIDIRECTIONAL:
                *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;
                break;
            default:
                *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL; assert(FALSE);
                break;
            }

            if (IsLoopback)
                *IsLoopback = !!fwDropEvent->isLoopback;
            if (FilterId)
                *FilterId = fwDropEvent->filterId;
            if (LayerId)
                *LayerId = fwDropEvent->layerId;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_CLASSIFY_ALLOW:
        {
            FWPM_NET_EVENT_CLASSIFY_ALLOW* fwAllowEvent = FwEvent->classifyAllow;

            if (EtWindowsVersion >= WINDOWS_10 && fwAllowEvent->isLoopback && EtFwIgnoreLoopback)
                return FALSE;

            switch (fwAllowEvent->msFwpDirection)
            {
            case FWP_DIRECTION_INBOUND:
            case FWP_DIRECTION_MAP_INBOUND:
                *Direction = FW_EVENT_DIRECTION_INBOUND;
                break;
            case FWP_DIRECTION_OUTBOUND:
            case FWP_DIRECTION_MAP_OUTBOUND:
                *Direction = FW_EVENT_DIRECTION_OUTBOUND;
                break;
            case FWP_DIRECTION_MAP_FORWARD:
                *Direction = FW_EVENT_DIRECTION_FORWARD;
                break;
            case FWP_DIRECTION_MAP_BIDIRECTIONAL:
                *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;
                break;
            default:
                *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL; assert(FALSE);
                break;
            }

            if (IsLoopback)
                *IsLoopback = !!fwAllowEvent->isLoopback;
            if (FilterId)
                *FilterId = fwAllowEvent->filterId;
            if (LayerId)
                *LayerId = fwAllowEvent->layerId;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_IKEEXT_MM_FAILURE:
        {
            FWPM_NET_EVENT_IKEEXT_MM_FAILURE* fwIkeMmFailureEvent = FwEvent->ikeMmFailure;

            *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;

            if (FilterId)
                *FilterId = fwIkeMmFailureEvent->mmFilterId;
            if (LayerId)
                *LayerId = FWPS_BUILTIN_LAYER_MAX;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_IKEEXT_QM_FAILURE:
        {
            FWPM_NET_EVENT_IKEEXT_QM_FAILURE* fwIkeQmFailureEvent = FwEvent->ikeQmFailure;

            *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;

            if (FilterId)
                *FilterId = fwIkeQmFailureEvent->qmFilterId;
            if (LayerId)
                *LayerId = FWPS_BUILTIN_LAYER_MAX;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_IKEEXT_EM_FAILURE:
        {
            FWPM_NET_EVENT_IKEEXT_EM_FAILURE* fwIkeEmFailureEvent = FwEvent->ikeEmFailure;

            *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;

            if (FilterId)
                *FilterId = fwIkeEmFailureEvent->qmFilterId;
            if (LayerId)
                *LayerId = FWPS_BUILTIN_LAYER_MAX;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_IPSEC_KERNEL_DROP:
        {
            FWPM_NET_EVENT_IPSEC_KERNEL_DROP* fwIpsecDropEvent = FwEvent->ipsecDrop;

            switch (fwIpsecDropEvent->direction)
            {
            case FWP_DIRECTION_INBOUND:
            case FWP_DIRECTION_MAP_INBOUND:
                *Direction = FW_EVENT_DIRECTION_INBOUND;
                break;
            case FWP_DIRECTION_OUTBOUND:
            case FWP_DIRECTION_MAP_OUTBOUND:
                *Direction = FW_EVENT_DIRECTION_OUTBOUND;
                break;
            case FWP_DIRECTION_MAP_FORWARD:
                *Direction = FW_EVENT_DIRECTION_FORWARD;
                break;
            case FWP_DIRECTION_MAP_BIDIRECTIONAL:
                *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;
                break;
            default:
                *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;
                break;
            }

            if (FilterId)
                *FilterId = fwIpsecDropEvent->filterId;
            if (LayerId)
                *LayerId = fwIpsecDropEvent->layerId;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_IPSEC_DOSP_DROP:
        {
            FWPM_NET_EVENT_IPSEC_DOSP_DROP* fwIdpDropEvent = FwEvent->idpDrop;

            switch (fwIdpDropEvent->direction)
            {
            case FWP_DIRECTION_INBOUND:
            case FWP_DIRECTION_MAP_INBOUND:
                *Direction = FW_EVENT_DIRECTION_INBOUND;
                break;
            case FWP_DIRECTION_OUTBOUND:
            case FWP_DIRECTION_MAP_OUTBOUND:
                *Direction = FW_EVENT_DIRECTION_OUTBOUND;
                break;
            case FWP_DIRECTION_MAP_FORWARD:
                *Direction = FW_EVENT_DIRECTION_FORWARD;
                break;
            case FWP_DIRECTION_MAP_BIDIRECTIONAL:
                *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;
                break;
            default:
                *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;
                break;
            }

            if (LayerId)
                *LayerId = FWPS_BUILTIN_LAYER_MAX;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_CAPABILITY_DROP:
        {
            FWPM_NET_EVENT_CAPABILITY_DROP* fwCapabilityDropEvent = FwEvent->capabilityDrop;

            //if (EtWindowsVersion >= WINDOWS_10 && fwCapabilityDropEvent->isLoopback && EtFwIgnoreLoopback)
            //    return FALSE;

            *Direction = FW_EVENT_DIRECTION_OUTBOUND;

            if (IsLoopback)
                *IsLoopback = !!fwCapabilityDropEvent->isLoopback;
            if (FilterId)
                *FilterId = fwCapabilityDropEvent->filterId;
            if (LayerId)
                *LayerId = FWPS_BUILTIN_LAYER_MAX;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_CAPABILITY_ALLOW:
        {
            FWPM_NET_EVENT_CAPABILITY_ALLOW* fwCapabilityAllowEvent = FwEvent->capabilityAllow;

            if (EtWindowsVersion >= WINDOWS_10 && fwCapabilityAllowEvent->isLoopback && EtFwIgnoreLoopback)
                return FALSE;

            *Direction = FW_EVENT_DIRECTION_OUTBOUND;

            if (IsLoopback)
                *IsLoopback = !!fwCapabilityAllowEvent->isLoopback;
            if (FilterId)
                *FilterId = fwCapabilityAllowEvent->filterId;
            if (LayerId)
                *LayerId = FWPS_BUILTIN_LAYER_MAX;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP_MAC:
        {
            FWPM_NET_EVENT_CLASSIFY_DROP_MAC* fwClassifyDropMacEvent = FwEvent->classifyDropMac;

            //if (EtWindowsVersion >= WINDOWS_10 && fwClassifyDropMacEvent->isLoopback && EtFwIgnoreLoopback)
            //    return FALSE;

            switch (fwClassifyDropMacEvent->msFwpDirection)
            {
            case FWP_DIRECTION_INBOUND:
            case FWP_DIRECTION_MAP_INBOUND:
                *Direction = FW_EVENT_DIRECTION_INBOUND;
                break;
            case FWP_DIRECTION_OUTBOUND:
            case FWP_DIRECTION_MAP_OUTBOUND:
                *Direction = FW_EVENT_DIRECTION_OUTBOUND;
                break;
            case FWP_DIRECTION_MAP_FORWARD:
                *Direction = FW_EVENT_DIRECTION_FORWARD;
                break;
            case FWP_DIRECTION_MAP_BIDIRECTIONAL:
                *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;
                break;
            default:
                *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;
                break;
            }

            if (IsLoopback)
                *IsLoopback = !!fwClassifyDropMacEvent->isLoopback;
            if (FilterId)
                *FilterId = fwClassifyDropMacEvent->filterId;
            if (LayerId)
                *LayerId = fwClassifyDropMacEvent->layerId;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_LPM_PACKET_ARRIVAL:
        {
            *Direction = FW_EVENT_DIRECTION_INBOUND;

            if (LayerId)
                *LayerId = FWPS_BUILTIN_LAYER_MAX;
        }
        return TRUE;
    case FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_MM_ESTABLISHED:
    case FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_MM_DELETED:
    case FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_MM_FAILURE:
    case FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_QM_ESTABLISHED:
    case FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_QM_DELETED:
    case FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_QM_FAILURE:
    case FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_NO_QM_FOR_MMSA:
    case FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_POLICY_CHANGE:
        {
            *Direction = FW_EVENT_DIRECTION_BIDIRECTIONAL;

            if (LayerId)
                *LayerId = FWPS_BUILTIN_LAYER_MAX;
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * Resolves a hostname for an IP address.
 *
 * \param Address The IP address to resolve.
 * \return A string containing the resolved hostname, an empty string for local
 * addresses that should be cached as unresolved, or NULL if resolution fails.
 * \remarks Local, multicast and private addresses are queried without DoH and
 * may be cached as an empty string to prevent recursive firewall events.
 */
PPH_STRING EtFwGetNameFromAddress(
    _In_ PPH_IP_ADDRESS Address
    )
{
    BOOLEAN dnsLocalQuery = FALSE;
    PPH_STRING addressEndpointString = NULL;
    PPH_STRING addressReverseString = NULL;
    PDNS_RECORD dnsRecordList;

    switch (Address->Type)
    {
    case PH_NETWORK_TYPE_IPV4:
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

            addressReverseString = PhDnsReverseLookupNameFromAddress(PH_NETWORK_TYPE_IPV4, &Address->InAddr);
        }
        break;
    case PH_NETWORK_TYPE_IPV6:
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

            addressReverseString = PhDnsReverseLookupNameFromAddress(PH_NETWORK_TYPE_IPV6, &Address->In6Addr);
        }
        break;
    }

    if (PhIsNullOrEmptyString(addressReverseString))
    {
        if (dnsLocalQuery) // See note below (dmex)
            return PhReferenceEmptyString();
        return NULL;
    }

    if (EtFwEnableResolveDoH && !dnsLocalQuery)
    {
        dnsRecordList = PhDnsQuery(
            NULL,
            addressReverseString->Buffer,
            DNS_TYPE_PTR
            );
    }
    else
    {
        dnsRecordList = PhDnsQuery2(
            NULL,
            addressReverseString->Buffer,
            DNS_TYPE_PTR,
            DNS_QUERY_NO_HOSTS_FILE // DNS_QUERY_BYPASS_CACHE
            );
    }

    PhDereferenceObject(addressReverseString);

    if (dnsRecordList)
    {
        for (PDNS_RECORD dnsRecord = dnsRecordList; dnsRecord; dnsRecord = dnsRecord->pNext)
        {
            if (dnsRecord->wType == DNS_TYPE_PTR)
            {
                addressEndpointString = PhCreateString(dnsRecord->Data.PTR.pNameHost); // Return the first result (dmex)
                break;
            }
        }

        PhDnsFree(dnsRecordList);
    }

    if (dnsLocalQuery && PhIsNullOrEmptyString(addressEndpointString))
    {
        // If the local hostname query failed then we'll cache an empty string.
        // The hostname lookup generates a firewall event, caching the null string for
        // these requests prevents hostname lookups generating infinite firewall events. (dmex)
        PhMoveReference(&addressEndpointString, PhReferenceEmptyString());
    }

    return addressEndpointString;
}

/**
 * Resolves a queued firewall item hostname.
 *
 * \param Parameter A FW_ITEM_QUERY_DATA structure.
 * \remarks The worker performs a final cache lookup, resolves the address when
 * required, updates the resolve cache and queues the completed query for the UI
 * update path.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
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
                FW_RESOLVE_CACHE_ITEM newEntry;

                memset(&newEntry, 0, sizeof(FW_RESOLVE_CACHE_ITEM));
                memcpy(&newEntry.Address, &data->Address, sizeof(data->Address));
                newEntry.HostString = PhReferenceObject(hostString);
                PhAddEntryHashtable(EtFwResolveCacheHashtable, &newEntry);
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

/**
 * Queues hostname resolution for a firewall event item endpoint.
 *
 * \param EventItem The firewall event item.
 * \param Remote TRUE to resolve the remote endpoint, FALSE to resolve the local endpoint.
 */
VOID EtFwQueueNetworkItemQuery(
    _In_ PFW_EVENT_ITEM EventItem,
    _In_ BOOLEAN Remote
    )
{
    PFW_ITEM_QUERY_DATA data;

    if (!EtFwEnableResolveCache)
    {
        if (Remote)
            EventItem->RemoteHostnameResolved = TRUE;
        else
            EventItem->LocalHostnameResolved = TRUE;
        return;
    }

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

/**
 * Applies completed hostname resolution data to firewall event items.
 *
 * \remarks Completed worker results are flushed from the query list, copied to
 * their event items and marked as modified for the process provider update pass.
 */
VOID EtFwFlushHostNameData(
    VOID
    )
{
    PSLIST_ENTRY entry;
    PFW_ITEM_QUERY_DATA data;

    if (!FlagOn(EtFwFlagsMask, FW_PROVIDER_FLAG_HOSTNAME))
        return;
    //if (!RtlFirstEntrySList(&EtFwQueryListHead))
    //    return;

    entry = RtlInterlockedFlushSList(&EtFwQueryListHead);

    while (entry)
    {
        data = CONTAINING_RECORD(entry, FW_ITEM_QUERY_DATA, ListEntry);
        entry = entry->Next;

        if (data->Remote)
        {
            PhMoveReference(&data->EventItem->RemoteHostnameString, data->HostString);
            data->EventItem->RemoteHostnameResolved = TRUE;
        }
        else
        {
            PhMoveReference(&data->EventItem->LocalHostnameString, data->HostString);
            data->EventItem->LocalHostnameResolved = TRUE;
        }

        InterlockedExchange(&data->EventItem->JustResolved, 1);

        PhDereferenceObject(data->EventItem);
        PhFree(data);
    }
}

/**
 * Queues hostname resolution for both endpoints in a firewall event item.
 *
 * \param Entry The firewall event item.
 * \remarks Cached hostnames are applied immediately. Missing hostnames are
 * queued to the firewall hostname work queue.
 */
VOID EtFwQueryHostnameForEntry(
    _In_ PFW_EVENT_ITEM Entry
    )
{
    PFW_RESOLVE_CACHE_ITEM cacheItem;

    if (!FlagOn(EtFwFlagsMask, FW_PROVIDER_FLAG_HOSTNAME))
        return;

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
            Entry->LocalHostnameResolved = TRUE;
        }
        else
        {
            EtFwQueueNetworkItemQuery(Entry, FALSE);
        }
    }
    else
    {
        Entry->LocalHostnameResolved = TRUE;
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
            Entry->RemoteHostnameResolved = TRUE;
        }
        else
        {
            EtFwQueueNetworkItemQuery(Entry, TRUE);
        }
    }
    else
    {
        Entry->RemoteHostnameResolved = TRUE;
    }
}

/**
 * References a process item by executable file name.
 *
 * \param ProcessFileName The native process file name to match.
 * \return A referenced process item, or NULL if no matching process exists.
 */
PPH_PROCESS_ITEM EtFwFileNameToProcess(
    _In_ PPH_STRING ProcessFileName
    )
{
    PPH_PROCESS_ITEM* processItems;
    ULONG numberOfProcessItems;

    if (
        ProcessFileName->Length == 12 &&
        PhEqualString2(ProcessFileName, L"System", FALSE)
        )
    {
        return PhReferenceProcessItem(SYSTEM_PROCESS_ID);
    }

    PhEnumProcessItems(&processItems, &numberOfProcessItems);

    for (ULONG i = 0; i < numberOfProcessItems; i++)
    {
        if (
            processItems[i]->FileName &&
            PhEqualString(processItems[i]->FileName, ProcessFileName, TRUE)
            )
        {
            PVOID object = PhReferenceObject(processItems[i]);
            PhDereferenceObjects(processItems, numberOfProcessItems);
            PhFree(processItems);
            return object;
        }
    }

    PhDereferenceObjects(processItems, numberOfProcessItems);
    PhFree(processItems);

    return NULL;
}

/**
 * Gets the NetworkTools plugin interface.
 *
 * \return The NetworkTools interface, or NULL if the plugin is not loaded or
 * the interface version is unsupported.
 */
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

/**
 * Draws a country icon using the NetworkTools plugin.
 *
 * \param hdc The destination device context.
 * \param rect The destination rectangle.
 * \param Index The country icon index.
 */
VOID EtFwDrawCountryIcon(
    _In_ HDC hdc,
    _In_ RECT rect,
    _In_ LONG Index
    )
{
    if (EtFwGetPluginInterface())
        EtFwGetPluginInterface()->DrawCountryIcon(hdc, rect, Index);
}

/**
 * Shows the NetworkTools ping window for an endpoint.
 *
 * \param ParentWindowHandle The parent window handle.
 * \param Endpoint The endpoint to ping.
 */
VOID EtFwShowPingWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    )
{
    if (EtFwGetPluginInterface())
        EtFwGetPluginInterface()->ShowPingWindow(ParentWindowHandle, Endpoint);
}

/**
 * Shows the NetworkTools traceroute window for an endpoint.
 *
 * \param ParentWindowHandle The parent window handle.
 * \param Endpoint The endpoint to trace.
 */
VOID EtFwShowTracerWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    )
{
    if (EtFwGetPluginInterface())
        EtFwGetPluginInterface()->ShowTracertWindow(ParentWindowHandle, Endpoint);
}

/**
 * Shows the NetworkTools WHOIS window for an endpoint.
 *
 * \param ParentWindowHandle The parent window handle.
 * \param Endpoint The endpoint to query.
 */
VOID EtFwShowWhoisWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    )
{
    if (EtFwGetPluginInterface())
        EtFwGetPluginInterface()->ShowWhoisWindow(ParentWindowHandle, Endpoint);
}

/**
 * Looks up the service name for a TCP or UDP port.
 *
 * \param Port The port number.
 * \param ServiceName A variable which receives the service name string reference.
 * \return TRUE if a service name was found, otherwise FALSE.
 */
_Success_(return)
BOOLEAN EtFwLookupPortServiceName(
    _In_ ULONG Port,
    _Out_ PPH_STRINGREF* ServiceName
    )
{
    if (EtFwGetPluginInterface())
    {
        return EtFwGetPluginInterface()->LookupPortServiceName(Port, ServiceName);
    }

    return FALSE;
}

typedef struct _ETFW_FILTER_DISPLAY_CONTEXT
{
    ULONG64 FilterId;
    PPH_STRING Name;
    PPH_STRING Description;
} ETFW_FILTER_DISPLAY_CONTEXT, *PETFW_FILTER_DISPLAY_CONTEXT;

/**
 * Compares two filter display cache entries.
 *
 * \param Entry1 The first filter display cache entry.
 * \param Entry2 The second filter display cache entry.
 * \return TRUE if both entries refer to the same filter identifier, otherwise FALSE.
 */
_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
static BOOLEAN NTAPI EtFwFilterDisplayDataEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PETFW_FILTER_DISPLAY_CONTEXT entry1 = Entry1;
    PETFW_FILTER_DISPLAY_CONTEXT entry2 = Entry2;

    return entry1->FilterId == entry2->FilterId;
}

/**
 * Computes the hash value for a filter display cache entry.
 *
 * \param Entry The filter display cache entry.
 * \return The filter identifier hash value.
 */
_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
static ULONG NTAPI EtFwFilterDisplayDataHashFunction(
    _In_ PVOID Entry
    )
{
    PETFW_FILTER_DISPLAY_CONTEXT entry = Entry;

    return PhHashInt64(entry->FilterId);
}

/**
 * Flushes cached WFP filter display data.
 *
 * \remarks The existing cache strings are dereferenced and the cache hashtable
 * is recreated empty.
 */
VOID EtFwFlushFilterDisplayCache(
    VOID
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PETFW_FILTER_DISPLAY_CONTEXT entry;

    if (!EtFwFilterDisplayDataHashTable)
        return;

    PhBeginEnumHashtable(EtFwFilterDisplayDataHashTable, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
    {
        if (entry->Name)
            PhDereferenceObject(entry->Name);
        if (entry->Description)
            PhDereferenceObject(entry->Description);
    }

    PhDereferenceObject(EtFwFilterDisplayDataHashTable);
    EtFwFilterDisplayDataHashTable = PhCreateHashtable(
        sizeof(ETFW_FILTER_DISPLAY_CONTEXT),
        EtFwFilterDisplayDataEqualFunction,
        EtFwFilterDisplayDataHashFunction,
        20
    );
}

/**
 * Looks up cached WFP filter display data.
 *
 * \param FilterId The WFP filter identifier.
 * \return The cached display data entry, or NULL if no entry exists.
 */
PETFW_FILTER_DISPLAY_CONTEXT EtFwFilterLookupCacheItem(
    _In_ ULONG64 FilterId
    )
{
    ETFW_FILTER_DISPLAY_CONTEXT lookupEntry;
    PETFW_FILTER_DISPLAY_CONTEXT entry;

    if (!EtFwFilterDisplayDataHashTable)
        return NULL;

    lookupEntry.FilterId = FilterId;

    entry = (PETFW_FILTER_DISPLAY_CONTEXT)PhFindEntryHashtable(
        EtFwFilterDisplayDataHashTable,
        &lookupEntry
        );

    if (entry)
        return entry;
    else
        return NULL;
}

/**
 * Adds cached display data for a WFP filter.
 *
 * \param FilterId The WFP filter identifier.
 * \return TRUE if display data was added, otherwise FALSE.
 */
_Success_(return)
BOOLEAN EtFwAddFilterDisplayData(
    _In_ ULONG64 FilterId
    )
{
    PETFW_FILTER_DISPLAY_CONTEXT entry;
    PPH_STRING filterName = NULL;
    PPH_STRING filterDescription = NULL;
    PPH_STRING oldFilterName = NULL;
    PPH_STRING oldFilterDescription = NULL;
    FWPM_FILTER* filter;

    if (!EtFwFilterDisplayDataHashTable)
        return FALSE;

    if (FwpmFilterGetById(EtFwEngineHandle, FilterId, &filter) == ERROR_SUCCESS)
    {
        if (filter->displayData.name)
            filterName = PhCreateString(filter->displayData.name);
        if (filter->displayData.description)
            filterDescription = PhCreateString(filter->displayData.description);

        FwpmFreeMemory(&filter);
    }

    if (!filterName || !filterDescription)
    {
        if (filterName)
            PhDereferenceObject(filterName);
        if (filterDescription)
            PhDereferenceObject(filterDescription);

        return FALSE;
    }

    PhAcquireQueuedLockExclusive(&EtFwFilterDisplayDataHashTableLock);

    if (entry = EtFwFilterLookupCacheItem(FilterId))
    {
        oldFilterName = entry->Name;
        oldFilterDescription = entry->Description;
        PhRemoveEntryHashtable(EtFwFilterDisplayDataHashTable, entry);
    }

    {
        ETFW_FILTER_DISPLAY_CONTEXT newEntry;

        memset(&newEntry, 0, sizeof(ETFW_FILTER_DISPLAY_CONTEXT));
        newEntry.FilterId = FilterId;
        newEntry.Name = filterName;
        newEntry.Description = filterDescription;
        PhAddEntryHashtable(EtFwFilterDisplayDataHashTable, &newEntry);
    }

    PhReleaseQueuedLockExclusive(&EtFwFilterDisplayDataHashTableLock);

    if (oldFilterName)
        PhDereferenceObject(oldFilterName);
    if (oldFilterDescription)
        PhDereferenceObject(oldFilterDescription);

    return TRUE;
}

/**
 * Gets the display name and description for a WFP filter.
 *
 * \param FilterId The WFP filter identifier.
 * \param Name A variable which receives a referenced display name string.
 * \param Description A variable which receives a referenced description string.
 * \return TRUE if display data was found, otherwise FALSE.
 */
_Success_(return)
BOOLEAN EtFwGetFilterDisplayData(
    _In_ ULONG64 FilterId,
    _Out_ PPH_STRING* Name,
    _Out_ PPH_STRING* Description
    )
{
    PETFW_FILTER_DISPLAY_CONTEXT entry;
    PPH_STRING filterName = NULL;
    PPH_STRING filterDescription = NULL;
    FWPM_FILTER* filter;

    if (!EtFwFilterDisplayDataHashTable)
        return FALSE;

    PhAcquireQueuedLockShared(&EtFwFilterDisplayDataHashTableLock);

    entry = EtFwFilterLookupCacheItem(FilterId);

    if (entry)
    {
        if (Name)
            *Name = PhReferenceObject(entry->Name);
        if (Description)
            *Description = PhReferenceObject(entry->Description);

        PhReleaseQueuedLockShared(&EtFwFilterDisplayDataHashTableLock);

        return TRUE;
    }

    PhReleaseQueuedLockShared(&EtFwFilterDisplayDataHashTableLock);

    if (FwpmFilterGetById(EtFwEngineHandle, FilterId, &filter) == ERROR_SUCCESS)
    {
        if (filter->displayData.name)
            filterName = PhCreateString(filter->displayData.name);
        if (filter->displayData.description)
            filterDescription = PhCreateString(filter->displayData.description);

        FwpmFreeMemory(&filter);
    }

    if (filterName && filterDescription)
    {
        PhAcquireQueuedLockExclusive(&EtFwFilterDisplayDataHashTableLock);

        ETFW_FILTER_DISPLAY_CONTEXT newentry;
        memset(&newentry, 0, sizeof(ETFW_FILTER_DISPLAY_CONTEXT));
        newentry.FilterId = FilterId;
        newentry.Name = filterName;
        newentry.Description = filterDescription;
        PhAddEntryHashtable(EtFwFilterDisplayDataHashTable, &newentry);

        *Name = PhReferenceObject(filterName);
        *Description = PhReferenceObject(filterDescription);

        PhReleaseQueuedLockExclusive(&EtFwFilterDisplayDataHashTableLock);

        return TRUE;
    }

    if (filterName)
        PhDereferenceObject(filterName);
    if (filterDescription)
        PhDereferenceObject(filterDescription);

    return FALSE;
}

/**
 * Removes cached display data for a WFP filter.
 *
 * \param FilterId The WFP filter identifier.
 */
VOID EtFwRemoveFilterDisplayData(
    _In_ ULONG64 FilterId
    )
{
    PETFW_FILTER_DISPLAY_CONTEXT entry;
    PPH_STRING filterName = NULL;
    PPH_STRING filterDescription = NULL;

    if (!EtFwFilterDisplayDataHashTable)
        return;

    PhAcquireQueuedLockExclusive(&EtFwFilterDisplayDataHashTableLock);

    if (entry = EtFwFilterLookupCacheItem(FilterId))
    {
        filterName = entry->Name;
        filterDescription = entry->Description;
        PhRemoveEntryHashtable(EtFwFilterDisplayDataHashTable, entry);
    }

    PhReleaseQueuedLockExclusive(&EtFwFilterDisplayDataHashTableLock);

    if (filterName)
        PhDereferenceObject(filterName);
    if (filterDescription)
        PhDereferenceObject(filterDescription);
}

//_Success_(return)
//BOOLEAN EtFwGetLayerDisplayData(
//    _In_ UINT16 LayerId,
//    _Out_ PPH_STRING* Name,
//    _Out_ PPH_STRING* Description
//    )
//{
//    PPH_STRING layerName = NULL;
//    PPH_STRING layerDescription = NULL;
//    FWPM_LAYER* layer;
//
//    if (FwpmLayerGetById(EtFwEngineHandle, LayerId, &layer) == ERROR_SUCCESS)
//    {
//        if (layer->displayData.name)
//            layerName = PhCreateString(layer->displayData.name);
//        if (layer->displayData.description)
//            layerDescription = PhCreateString(layer->displayData.description);
//
//        FwpmFreeMemory(&layer);
//    }
//
//    if (layerName && layerDescription)
//    {
//        *Name = layerName;
//        *Description = layerDescription;
//        return TRUE;
//    }
//
//    if (layerName)
//        PhDereferenceObject(layerName);
//    if (layerDescription)
//        PhDereferenceObject(layerDescription);
//
//    return FALSE;
//}

/**
 * Looks up the class string for an IP address.
 *
 * \param Address The IP address to classify.
 * \param ClassString A variable which receives the address class string reference.
 * \return TRUE if the address type was recognized, otherwise FALSE.
 */
_Success_(return)
BOOLEAN EtFwLookupAddressClass(
    _In_ PPH_IP_ADDRESS Address,
    _Out_ PPH_STRINGREF ClassString
    )
{
    switch (Address->Type)
    {
    case PH_NETWORK_TYPE_IPV4:
        {
            PIN_ADDR inAddr = (PIN_ADDR)&Address->Ipv4;

            if (IN4_IS_ADDR_UNSPECIFIED(inAddr))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Unspecified");
                *ClassString = string;
                return TRUE;
            }
            else if (IN4_IS_ADDR_LOOPBACK(inAddr))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Loopback");
                *ClassString = string;
                return TRUE;
            }
            else if (IN4_IS_ADDR_BROADCAST(inAddr))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Broadcast");
                *ClassString = string;
                return TRUE;
            }
            else if (IN4_IS_ADDR_MULTICAST(inAddr))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Multicast");
                *ClassString = string;
                return TRUE;
            }
            else if (IN4_IS_ADDR_LINKLOCAL(inAddr))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Linklocal");
                *ClassString = string;
                return TRUE;
            }
            else if (IN4_IS_ADDR_MC_LINKLOCAL(inAddr))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Multicast-Linklocal");
                *ClassString = string;
                return TRUE;
            }
            else if (IN4_IS_ADDR_RFC1918(inAddr))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"RFC1918");
                *ClassString = string;
                return TRUE;
            }
            else
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Unicast");
                *ClassString = string;
                return TRUE;
            }
        }
        break;
    case PH_NETWORK_TYPE_IPV6:
        {
            PIN6_ADDR inAddr6 = (PIN6_ADDR)&Address->Ipv6;

            if (IN6_IS_ADDR_UNSPECIFIED(inAddr6))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Unspecified");
                *ClassString = string;
                return TRUE;
            }
            else if (IN6_IS_ADDR_LOOPBACK(inAddr6))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Loopback");
                *ClassString = string;
                return TRUE;
            }
            else if (IN6_IS_ADDR_MULTICAST(inAddr6))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Multicast");
                *ClassString = string;
                return TRUE;
            }
            else if (IN6_IS_ADDR_LINKLOCAL(inAddr6))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Linklocal");
                *ClassString = string;
                return TRUE;
            }
            else if (IN6_IS_ADDR_MC_LINKLOCAL(inAddr6))
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Multicast-Linklocal");
                *ClassString = string;
                return TRUE;
            }
            else
            {
                static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Unicast");
                *ClassString = string;
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

CONST PH_KEY_VALUE_PAIR FwAddressScopePairs[] =
{
    SIP(SREF(L"Unknoen"), 0),
    SIP(SREF(L"Interface"), ScopeLevelInterface),
    SIP(SREF(L"Link"), ScopeLevelLink),
    SIP(SREF(L"Subnet"), ScopeLevelSubnet),
    SIP(SREF(L"Admin"), ScopeLevelAdmin),
    SIP(SREF(L"Site"), ScopeLevelSite),
    SIP(SREF(L"Region‑local (abandoned)"), 6),
    SIP(SREF(L"Campus‑local (abandoned)"), 7),
    SIP(SREF(L"Organization"), ScopeLevelOrganization),
    SIP(SREF(L"Cross‑site (transitional)"), 9),
    SIP(SREF(L"Cross‑site (transitional)"), 10),
    SIP(SREF(L"Reserved"), 11),
    SIP(SREF(L"Reserved"), 12),
    SIP(SREF(L"Reserved"), 13),
    SIP(SREF(L"Global"), ScopeLevelGlobal),
    SIP(SREF(L"Reserved"), 15),
    SIP(SREF(L"Maximum"), ScopeLevelCount),
};

/**
 * Looks up the scope string for an IP address.
 *
 * \param Address The IP address to classify.
 * \param ScopeString A variable which receives the address scope string reference.
 * \return TRUE if the address type was recognized and the scope was mapped,
 * otherwise FALSE.
 */
_Success_(return)
BOOLEAN EtFwLookupAddressScope(
    _In_ PPH_IP_ADDRESS Address,
    _Out_ PPH_STRINGREF ScopeString
    )
{
    PCPH_STRINGREF scopeString;

    switch (Address->Type)
    {
    case PH_NETWORK_TYPE_IPV4:
        {
            if (PhIndexStringRefSiKeyValuePairs(
                FwAddressScopePairs,
                sizeof(FwAddressScopePairs),
                Ipv4AddressScope((PUCHAR)&Address->Ipv4),
                &scopeString
                ))
            {
                *ScopeString = *scopeString;
                return TRUE;
            }
        }
        break;
    case PH_NETWORK_TYPE_IPV6:
        {
            if (PhIndexStringRefSiKeyValuePairs(
                FwAddressScopePairs,
                sizeof(FwAddressScopePairs),
                Ipv6AddressScope((PUCHAR)&Address->Ipv6),
                &scopeString
                ))
            {
                *ScopeString = *scopeString;
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

typedef struct _ETFW_SID_FULL_NAME_CACHE_ENTRY
{
    PSID Sid;
    PPH_STRING FullName;
} ETFW_SID_FULL_NAME_CACHE_ENTRY, *PETFW_SID_FULL_NAME_CACHE_ENTRY;

/**
 * Compares two SID full name cache entries.
 *
 * \param Entry1 The first SID cache entry.
 * \param Entry2 The second SID cache entry.
 * \return TRUE if both entries contain the same SID, otherwise FALSE.
 */
_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN EtFwSidFullNameCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PETFW_SID_FULL_NAME_CACHE_ENTRY entry1 = Entry1;
    PETFW_SID_FULL_NAME_CACHE_ENTRY entry2 = Entry2;

    return PhEqualSid(entry1->Sid, entry2->Sid);
}

/**
 * Computes the hash value for a SID full name cache entry.
 *
 * \param Entry The SID cache entry.
 * \return The SID hash value.
 */
_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG EtFwSidFullNameCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PETFW_SID_FULL_NAME_CACHE_ENTRY entry = Entry;

    return PhHashBytes(entry->Sid, PhLengthSid(entry->Sid));
}

/**
 * Flushes the SID full name cache.
 *
 * \remarks Cached SIDs and full name strings are released before the hashtable
 * is recreated empty.
 */
VOID EtFwFlushSidFullNameCache(
    VOID
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PETFW_SID_FULL_NAME_CACHE_ENTRY entry;

    if (!EtFwSidFullNameCacheHashtable)
        return;

    PhBeginEnumHashtable(EtFwSidFullNameCacheHashtable, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
    {
        if (entry->Sid)
            PhFree(entry->Sid);
        if (entry->FullName)
            PhDereferenceObject(entry->FullName);
    }

    PhDereferenceObject(EtFwSidFullNameCacheHashtable);
    EtFwSidFullNameCacheHashtable = PhCreateHashtable(
        sizeof(ETFW_SID_FULL_NAME_CACHE_ENTRY),
        EtFwSidFullNameCacheHashtableEqualFunction,
        EtFwSidFullNameCacheHashtableHashFunction,
        16
        );
}

/**
 * Looks up a cached SID full name entry.
 *
 * \param Sid The SID to find.
 * \return The cached SID entry, or NULL if no entry exists.
 */
PETFW_SID_FULL_NAME_CACHE_ENTRY EtFwSidLookupCacheItem(
    _In_ PSID Sid
    )
{
    ETFW_SID_FULL_NAME_CACHE_ENTRY lookupEntry;
    PETFW_SID_FULL_NAME_CACHE_ENTRY entry;

    if (!EtFwSidFullNameCacheHashtable)
        return NULL;

    lookupEntry.Sid = Sid;

    entry = (PETFW_SID_FULL_NAME_CACHE_ENTRY)PhFindEntryHashtable(
        EtFwSidFullNameCacheHashtable,
        &lookupEntry
        );

    if (entry)
        return entry;
    else
        return NULL;
}

/**
 * Gets and caches the full account name for a SID.
 *
 * \param Sid The SID to resolve.
 * \return A referenced full name string, or NULL if the SID cannot be resolved.
 * \remarks This function performs the slow SID lookup path and inserts the
 * resolved full name into the cache.
 */
PPH_STRING EtFwGetSidFullNameCachedSlow(
    _In_ PSID Sid
    )
{
    PETFW_SID_FULL_NAME_CACHE_ENTRY entry;
    PPH_STRING fullName;

    if (!EtFwSidFullNameCacheHashtable)
    {
        EtFwSidFullNameCacheHashtable = PhCreateHashtable(
            sizeof(ETFW_SID_FULL_NAME_CACHE_ENTRY),
            EtFwSidFullNameCacheHashtableEqualFunction,
            EtFwSidFullNameCacheHashtableHashFunction,
            16
            );
    }

    PhAcquireQueuedLockShared(&EtFwSidFullNameCacheHashtableLock);

    if (entry = EtFwSidLookupCacheItem(Sid))
    {
        PPH_STRING name = PhReferenceObject(entry->FullName);
        PhReleaseQueuedLockShared(&EtFwSidFullNameCacheHashtableLock);
        return name;
    }

    PhReleaseQueuedLockShared(&EtFwSidFullNameCacheHashtableLock);

    fullName = PhGetSidFullName(Sid, TRUE, NULL);

    if (!fullName)
        return NULL;

    if (EtFwSidFullNameCacheHashtable)
    {
        PhAcquireQueuedLockExclusive(&EtFwSidFullNameCacheHashtableLock);

        ETFW_SID_FULL_NAME_CACHE_ENTRY newEntry;
        memset(&newEntry, 0, sizeof(ETFW_SID_FULL_NAME_CACHE_ENTRY));
        newEntry.Sid = PhAllocateCopy(Sid, PhLengthSid(Sid));
        newEntry.FullName = PhReferenceObject(fullName);
        PhAddEntryHashtable(EtFwSidFullNameCacheHashtable, &newEntry);

        PhReleaseQueuedLockExclusive(&EtFwSidFullNameCacheHashtableLock);
    }

    return fullName;
}

/**
 * Gets a cached full account name for a SID.
 *
 * \param Sid The SID to find.
 * \return A referenced full name string, or NULL if the SID is not cached.
 */
PPH_STRING EtFwGetSidFullNameCached(
    _In_ PSID Sid
    )
{
    if (EtFwSidFullNameCacheHashtable)
    {
        PhAcquireQueuedLockShared(&EtFwSidFullNameCacheHashtableLock);

        PETFW_SID_FULL_NAME_CACHE_ENTRY entry;

        if (entry = EtFwSidLookupCacheItem(Sid))
        {
            PPH_STRING fullName = PhReferenceObject(entry->FullName);
            PhReleaseQueuedLockShared(&EtFwSidFullNameCacheHashtableLock);
            return fullName;
        }

        PhReleaseQueuedLockShared(&EtFwSidFullNameCacheHashtableLock);
    }

    return NULL;
}

typedef struct _ETFW_NAME_PROCESS_CACHE_ENTRY
{
    PPH_PROCESS_ITEM ProcessItem;
    ULONG FileNameHash;
} ETFW_NAME_PROCESS_CACHE_ENTRY, *PETFW_NAME_PROCESS_CACHE_ENTRY;

/**
 * Compares two file name process cache entries.
 *
 * \param Entry1 The first file name process cache entry.
 * \param Entry2 The second file name process cache entry.
 * \return TRUE if both entries have the same file name hash, otherwise FALSE.
 */
_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN EtFwFileNameProcessCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PETFW_NAME_PROCESS_CACHE_ENTRY entry1 = Entry1;
    PETFW_NAME_PROCESS_CACHE_ENTRY entry2 = Entry2;

    return entry1->FileNameHash == entry2->FileNameHash;
}

/**
 * Computes the hash value for a file name process cache entry.
 *
 * \param Entry The file name process cache entry.
 * \return The cached file name hash value.
 */
_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG EtFwFileNameProcessCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PETFW_NAME_PROCESS_CACHE_ENTRY entry = Entry;

    return entry->FileNameHash;
}

/**
 * Flushes the file name process cache.
 *
 * \remarks Cached process item references are released before the hashtable is
 * recreated empty.
 */
VOID EtFwFlushFileNameProcessCache(
    VOID
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PETFW_NAME_PROCESS_CACHE_ENTRY entry;

    if (!EtFwFileNameProcessCacheHashTable)
        return;

    PhBeginEnumHashtable(EtFwFileNameProcessCacheHashTable, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
    {
        if (entry->ProcessItem)
            PhDereferenceObject(entry->ProcessItem);
    }

    PhDereferenceObject(EtFwFileNameProcessCacheHashTable);
    EtFwFileNameProcessCacheHashTable = PhCreateHashtable(
        sizeof(ETFW_NAME_PROCESS_CACHE_ENTRY),
        EtFwFileNameProcessCacheHashtableEqualFunction,
        EtFwFileNameProcessCacheHashtableHashFunction,
        16
        );
}

/**
 * Looks up a cached process item by file name.
 *
 * \param FileName The process file name.
 * \return The cached file name process entry, or NULL if no entry exists.
 */
PETFW_NAME_PROCESS_CACHE_ENTRY EtFwFileNameProcessLookupCacheItem(
    _In_ PPH_STRING FileName
    )
{
    ETFW_NAME_PROCESS_CACHE_ENTRY lookupEntry;
    PETFW_NAME_PROCESS_CACHE_ENTRY entry;

    if (!EtFwFileNameProcessCacheHashTable)
        return NULL;

    lookupEntry.FileNameHash = PhHashStringRefEx(&FileName->sr, FALSE, PH_STRING_HASH_X65599);

    entry = (PETFW_NAME_PROCESS_CACHE_ENTRY)PhFindEntryHashtable(
        EtFwFileNameProcessCacheHashTable,
        &lookupEntry
        );

    if (entry)
        return entry;
    else
        return NULL;
}

/**
 * Gets and caches a process item by file name.
 *
 * \param FileName The process file name.
 * \return A referenced process item, or NULL if no matching process exists.
 * \remarks This function performs the slow process enumeration path and inserts
 * the result into the file name process cache.
 */
PPH_PROCESS_ITEM EtFwGetFileNameProcessCachedSlow(
    _In_ PPH_STRING FileName
    )
{
    PETFW_NAME_PROCESS_CACHE_ENTRY entry;
    PPH_PROCESS_ITEM process;

    if (PhIsNullOrEmptyString(FileName))
        return NULL;

    if (!EtFwFileNameProcessCacheHashTable)
    {
        EtFwFileNameProcessCacheHashTable = PhCreateHashtable(
            sizeof(ETFW_NAME_PROCESS_CACHE_ENTRY),
            EtFwFileNameProcessCacheHashtableEqualFunction,
            EtFwFileNameProcessCacheHashtableHashFunction,
            16
            );
    }

    PhAcquireQueuedLockShared(&EtFwFileNameProcessCacheHashTableLock);

    if (entry = EtFwFileNameProcessLookupCacheItem(FileName))
    {
        PPH_PROCESS_ITEM processItem = PhReferenceObject(entry->ProcessItem);
        PhReleaseQueuedLockShared(&EtFwFileNameProcessCacheHashTableLock);
        return processItem;
    }

    PhReleaseQueuedLockShared(&EtFwFileNameProcessCacheHashTableLock);

    process = EtFwFileNameToProcess(FileName);

    if (!process)
        return NULL;

    if (EtFwFileNameProcessCacheHashTable)
    {
        PhAcquireQueuedLockExclusive(&EtFwFileNameProcessCacheHashTableLock);

        ETFW_NAME_PROCESS_CACHE_ENTRY newEntry;
        memset(&newEntry, 0, sizeof(ETFW_NAME_PROCESS_CACHE_ENTRY));
        newEntry.FileNameHash = PhHashStringRefEx(&FileName->sr, FALSE, PH_STRING_HASH_X65599);
        newEntry.ProcessItem = PhReferenceObject(process);
        PhAddEntryHashtable(EtFwFileNameProcessCacheHashTable, &newEntry);

        PhReleaseQueuedLockExclusive(&EtFwFileNameProcessCacheHashTableLock);
    }

    return process;
}

/**
 * Gets a cached process item by file name.
 *
 * \param FileName The process file name.
 * \return A referenced process item, or NULL if the file name is not cached.
 */
PPH_PROCESS_ITEM EtFwGetFileNameProcessCached(
    _In_ PPH_STRING FileName
    )
{
    if (EtFwFileNameProcessCacheHashTable)
    {
        PhAcquireQueuedLockShared(&EtFwFileNameProcessCacheHashTableLock);

        PETFW_NAME_PROCESS_CACHE_ENTRY entry;

        if (entry = EtFwFileNameProcessLookupCacheItem(FileName))
        {
            PPH_PROCESS_ITEM processItem = PhReferenceObject(entry->ProcessItem);
            PhReleaseQueuedLockShared(&EtFwFileNameProcessCacheHashTableLock);
            return processItem;
        }

        PhReleaseQueuedLockShared(&EtFwFileNameProcessCacheHashTableLock);
    }

    return NULL;
}

/**
 * Periodically flushes firewall monitor caches.
 *
 * \remarks Hostname, filter and SID caches are flushed less frequently than the
 * file name process cache.
 */
VOID EtFwFlushCache(
    VOID
    )
{
    static ULONG64 lastTickCount = 0;
    static ULONG64 lastFilenameTickCount = 0;
    ULONG64 tickCount = NtGetTickCount64();

    if (tickCount - lastTickCount >= UInt32x32To64(120, CLOCKS_PER_SEC))
    {
        // Hostname cache
        PhAcquireQueuedLockExclusive(&EtFwCacheHashtableLock);
        EtFwFlushResolveCache();
        PhReleaseQueuedLockExclusive(&EtFwCacheHashtableLock);

        // Filter cache
        PhAcquireQueuedLockExclusive(&EtFwFilterDisplayDataHashTableLock);
        EtFwFlushFilterDisplayCache();
        PhReleaseQueuedLockExclusive(&EtFwFilterDisplayDataHashTableLock);

        // SID cache
        PhAcquireQueuedLockExclusive(&EtFwSidFullNameCacheHashtableLock);
        EtFwFlushSidFullNameCache();
        PhReleaseQueuedLockExclusive(&EtFwSidFullNameCacheHashtableLock);

        lastTickCount = tickCount;
    }

    if (tickCount - lastFilenameTickCount >= UInt32x32To64(30, CLOCKS_PER_SEC))
    {
        // Filename cache
        PhAcquireQueuedLockExclusive(&EtFwFileNameProcessCacheHashTableLock);
        EtFwFlushFileNameProcessCache();
        PhReleaseQueuedLockExclusive(&EtFwFileNameProcessCacheHashTableLock);

        lastFilenameTickCount = tickCount;
    }
}

/**
 * Handles WFP filter change notifications.
 *
 * \param Context The callback context.
 * \param Change The WFP filter change record.
 * \remarks Added filters are cached immediately and deleted filters are removed
 * from the cache.
 */
VOID CALLBACK EtFwFilterChangeCallback(
    _In_opt_ PVOID Context,
    _In_ const FWPM_FILTER_CHANGE0* Change
    )
{
    switch (Change->changeType)
    {
    case FWPM_CHANGE_ADD:
        EtFwAddFilterDisplayData(Change->filterId);
        break;
    case FWPM_CHANGE_DELETE:
        EtFwRemoveFilterDisplayData(Change->filterId);
        break;
    }
}

/**
 * References the process item associated with a firewall event.
 *
 * \param ProcessId The process identifier from the event.
 * \param EventTime The timestamp when the event occurred.
 * \return A referenced process item. If the process is not found or the PID was reused
 * after the event, a reference to the System process item is returned.
 * \remarks This function validates the process creation time against the event timestamp
 * to ensure events are not incorrectly attributed to a new process instance that has
 * reused the same PID.
 */
PPH_PROCESS_ITEM EtFwReferenceProcessItemForEvent(
    _In_ HANDLE ProcessId,
    _In_ PLARGE_INTEGER EventTime
    )
{
    PPH_PROCESS_ITEM processItem;

    if (ProcessId != 0)
    {
        if (processItem = PhReferenceProcessItem(ProcessId))
        {
            if (processItem->CreateTime.QuadPart <= EventTime->QuadPart)
                return processItem;

            PhDereferenceObject(processItem);
        }
    }

    return PhReferenceProcessItem(SYSTEM_PROCESS_ID);
}

/**
 * Handles WFP net event notifications.
 *
 * \param FwContext The callback context.
 * \param FwEvent The WFP net event.
 * \remarks The callback extracts public and internal WFP event fields, resolves
 * filter and process metadata, updates per-process firewall counters and queues
 * the resulting firewall event for the provider update path.
 */
VOID CALLBACK EtFwEventCallback(
    _In_opt_ PVOID FwContext,
    _In_ const FWPM_NET_EVENT* FwEvent
    )
{
    const FWPM_NET_EVENT_HEADER3* FwEventHeader = &FwEvent->header;
    FW_EVENT entry;
    BOOLEAN isLoopback = FALSE;
    ULONG direction = ULONG_MAX;
    ULONG64 filterId = 0;
    USHORT layerId = 0;
    PPH_STRING ruleName;
    PPH_STRING ruleDescription;

    if (!EtFwEnabled)
        return;

    if (!FwProcessEventType(
        FwEvent,
        &isLoopback,
        &direction,
        &filterId,
        &layerId
        ))
    {
        if (EtWindowsVersion >= WINDOWS_10 && EtFwIgnoreOnError)
            return;
    }

    //if (EtWindowsVersion >= WINDOWS_10 && EtFwIgnoreOnError)
    //{
    //    if (
    //        layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 || // IsEqualGUID(layerKey, FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4)
    //        layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6 || // IsEqualGUID(layerKey, FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6)
    //        layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 || // IsEqualGUID(layerKey, FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4)
    //        layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 // IsEqualGUID(layerKey, FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6)
    //        )
    //    {
    //        return;
    //    }
    //}

    RtlZeroMemory(&entry, sizeof(FW_EVENT));
    entry.TimeStamp.HighPart = FwEventHeader->timeStamp.dwHighDateTime;
    entry.TimeStamp.LowPart = FwEventHeader->timeStamp.dwLowDateTime;
    entry.Type = FwEvent->type;
    entry.IsLoopback = isLoopback;
    entry.Direction = direction;

    if (EtFwGetFilterDisplayData(filterId, &ruleName, &ruleDescription))
    {
        entry.RuleName = ruleName;
        entry.RuleDescription = ruleDescription;
    }

    if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_IP_PROTOCOL_SET))
    {
        entry.IpProtocol = FwEventHeader->ipProtocol;
    }

    if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_SCOPE_ID_SET))
    {
        entry.ScopeId = FwEventHeader->scopeId;
    }

    if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_IP_VERSION_SET))
    {
        if (FwEventHeader->ipVersion == FWP_IP_VERSION_V4)
        {
            if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_LOCAL_ADDR_SET))
            {
                entry.LocalEndpoint.Address.Type = PH_NETWORK_TYPE_IPV4;
                entry.LocalEndpoint.Address.InAddr.s_addr = _byteswap_ulong(FwEventHeader->localAddrV4);
            }

            if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_LOCAL_PORT_SET))
            {
                entry.LocalEndpoint.Port = FwEventHeader->localPort;
            }

            if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_REMOTE_ADDR_SET))
            {
                entry.RemoteEndpoint.Address.Type = PH_NETWORK_TYPE_IPV4;
                entry.RemoteEndpoint.Address.InAddr.s_addr = _byteswap_ulong(FwEventHeader->remoteAddrV4);
            }

            if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_REMOTE_PORT_SET))
            {
                entry.RemoteEndpoint.Port = FwEventHeader->remotePort;
            }
        }
        else if (FwEventHeader->ipVersion == FWP_IP_VERSION_V6)
        {
            if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_LOCAL_ADDR_SET))
            {
                entry.LocalEndpoint.Address.Type = PH_NETWORK_TYPE_IPV6;
                memcpy(entry.LocalEndpoint.Address.Ipv6, FwEventHeader->localAddrV6.byteArray16, 16);
            }

            if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_LOCAL_PORT_SET))
            {
                entry.LocalEndpoint.Port = FwEventHeader->localPort;
            }

            if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_REMOTE_ADDR_SET))
            {
                entry.RemoteEndpoint.Address.Type = PH_NETWORK_TYPE_IPV6;
                memcpy(entry.RemoteEndpoint.Address.Ipv6, FwEventHeader->remoteAddrV6.byteArray16, 16);
            }

            if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_REMOTE_PORT_SET))
            {
                entry.RemoteEndpoint.Port = FwEventHeader->remotePort;
            }
        }
    }

    if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_APP_ID_SET))
    {
        if (FwEventHeader->appId.data && FwEventHeader->appId.size > sizeof(UNICODE_NULL))
        {
            //PPH_STRING fileName;
            //
            //fileName = PhCreateStringEx(
            //    (PCWSTR)FwEventHeader->appId.data,
            //    (SIZE_T)FwEventHeader->appId.size - sizeof(UNICODE_NULL)
            //    );
            //
            //entry.ProcessFileName = PhGetFileName(fileName);
            //entry.ProcessBaseString = PhGetBaseName(entry.ProcessFileName);
            //
            //if (entry.ProcessItem = EtFwGetFileNameProcessCachedSlow(fileName))
            //{
            //    PhSwapReference(&entry.ProcessFileNameWin32, PhGetFileName(entry.ProcessItem->FileName));
            //}
            //
            //PhDereferenceObject(fileName);
        }
    }

    if (FlagOn(FwEventHeader->flags, FWPM_NET_EVENT_FLAG_USER_ID_SET))
    {
        //if (entry.ProcessItem && PhEqualSid(header->userId, entry.ProcessItem->Sid))
        if (FwEventHeader->userId)
        {
            entry.UserSid = PhAllocateCopy(FwEventHeader->userId, PhLengthSid(FwEventHeader->userId));
        }
    }

    //if (header->flags & FWPM_NET_EVENT_FLAG_PACKAGE_ID_SET)
    //{
    //    SID PhSeNobodySid = { SID_REVISION, 1, SECURITY_NULL_SID_AUTHORITY, { SECURITY_NULL_RID } };
    //    if (header->packageSid && !PhEqualSid(header->packageSid, &PhSeNobodySid))
    //    {
    //        entry.PackageSid = PhAllocateCopy(header->packageSid, PhLengthSid(header->packageSid));
    //    }
    //}

    if (EtFwGetPluginInterface())
    {
        ULONG remoteCountryCode;
        PPH_STRING remoteCountryName;

        if (EtFwGetPluginInterface()->LookupCountryCode(
            entry.RemoteEndpoint.Address,
            &remoteCountryCode,
            &remoteCountryName
            ))
        {
            PhMoveReference(&entry.RemoteCountryName, remoteCountryName);
            entry.CountryIconIndex = EtFwGetPluginInterface()->LookupCountryIcon(remoteCountryCode);
        }
    }

    if (FwpmNetEventCreateEnumHandleEx_I)
    {
        const FWPM_NET_EVENT_INTERNAL* FwEventInternal = (const FWPM_NET_EVENT_INTERNAL*)FwEvent;
        const FWPM_NET_EVENT_INTERNAL_FIELDS0* FwInternal = &FwEventInternal->InternalFields;
        PPH_PROCESS_ITEM processItem;

        processItem = EtFwReferenceProcessItemForEvent(
            UlongToHandle(FwInternal->ProcessId),
            &entry.TimeStamp
            );

        if (processItem)
        {
            entry.ProcessItem = processItem;
            entry.ProcessId = HandleToUlong(processItem->ProcessId);

            PhSetReference(&entry.ProcessName, processItem->ProcessName);
            PhSetReference(&entry.ProcessFileName, entry.ProcessItem->FileName);
            PhSetReference(&entry.ProcessFileNameWin32, PhGetFileName(entry.ProcessItem->FileName));
            PhSetReference(&entry.ProcessBaseString, entry.ProcessItem->ProcessName);
            entry.SubsystemProcess = !!processItem->IsSubsystemProcess;

            if (PhTestEvent(&processItem->Stage1Event))
            {
                entry.ProcessIconIndex = processItem->SmallIconIndex;
                entry.ProcessIconValid = TRUE;
            }

            {
                WCHAR buffer[PH_INT32_STR_LEN_1];

                PhPrintUInt32(buffer, entry.ProcessId);
                entry.ProcessIdString = PhCreateString(buffer);
            }

            if (processItem)
            {
                PET_PROCESS_BLOCK block = EtGetProcessBlock(processItem);

                if (block)
                {
                    if (entry.Type == FWPM_NET_EVENT_TYPE_CLASSIFY_ALLOW ||
                        entry.Type == FWPM_NET_EVENT_TYPE_CAPABILITY_ALLOW)
                    {
                        block->FirewallAllowCount++;
                    }
                    else
                    {
                        block->FirewallBlockCount++;
                    }
                }
            }

            // NOTE: We dereference processItem in PhpNetworkItemDeleteProcedure. (dmex)
        }

        entry.InterfaceLuid = FwInternal->InterfaceLuid;
        entry.CompartmentId = FwInternal->CompartmentId;

        if (FwInternal->ServiceSids)
        {
            entry.ServiceSids = PhCreateString(FwInternal->ServiceSids);
        }

        if (FlagOn(FwInternal->InternalFlags, FWPM_NET_EVENT_INTERNAL_FLAG_FILTER_ORIGIN_SET) && FwInternal->FilterOrigin)
        {
            entry.FilterOrigin = PhCreateString(FwInternal->FilterOrigin);
        }

        if (FlagOn(FwInternal->InternalFlags, FWPM_NET_EVENT_INTERNAL_FLAG_FQBN_SET) && FwInternal->FqbnName)
        {
            entry.FqbnName = PhCreateString(FwInternal->FqbnName);
        }

        if (FlagOn(FwInternal->InternalFlags, FWPM_NET_EVENT_INTERNAL_FLAG_POLICY_APP_ID_SET) && FwInternal->PolicyAppId)
        {
            entry.PolicyAppId = PhCreateString(FwInternal->PolicyAppId);
        }

        {
            WCHAR buffer[PH_INT64_STR_LEN_1];

            PhPrintUInt64(buffer, entry.InterfaceLuid.Value);
            entry.InterfaceLuidString = PhCreateString(buffer);

            PhPrintUInt32(buffer, (ULONG)entry.CompartmentId);
            entry.CompartmentIdString = PhCreateString(buffer);
        }
    }

    if (PhIsNullOrEmptyString(entry.ProcessFileName) || PhIsNullOrEmptyString(entry.ProcessBaseString))
    {
        PhMoveReference(&entry.ProcessItem, PhReferenceProcessItem(SYSTEM_PROCESS_ID));
        PhSwapReference(&entry.ProcessFileName, entry.ProcessItem->FileName);
        PhSwapReference(&entry.ProcessFileNameWin32, PhGetFileName(entry.ProcessItem->FileName));
        PhSwapReference(&entry.ProcessBaseString, entry.ProcessItem->ProcessName);
    }

    FwPushFirewallEvent(&entry);
}

/**
 * Processes queued firewall monitor updates.
 *
 * \param Parameter The callback parameter.
 * \param Context The callback context.
 * \remarks This callback drains queued firewall event packets, applies completed
 * hostname lookups, removes expired items and raises the firewall item update
 * callbacks.
 */
_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI EtFwProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PSLIST_ENTRY listEntry;
    PLIST_ENTRY ageListEntry;

    // Process incoming event packets.

    listEntry = RtlInterlockedFlushSList(&EtFwPacketListHead);

    while (listEntry)
    {
        PFW_EVENT_PACKET packet;

        packet = CONTAINING_RECORD(listEntry, FW_EVENT_PACKET, ListEntry);
        listEntry = listEntry->Next;

        FwProcessFirewallEvent(packet, FwRunCount);

        PhFreeToFreeList(&EtFwPacketFreeList, packet);
    }

    EtFwFlushHostNameData();

    // Remove old entries and update existing.

    ageListEntry = EtFwAgeListHead.Blink;

    while (ageListEntry != &EtFwAgeListHead)
    {
        PFW_EVENT_ITEM item;

        item = CONTAINING_RECORD(ageListEntry, FW_EVENT_ITEM, AgeListEntry);
        ageListEntry = ageListEntry->Blink;

        if (FwRunCount - item->RunId < EtFwMaxEventAge)
        {
            BOOLEAN modified = FALSE;

            if (InterlockedExchange(&item->JustResolved, 0) != 0)
            {
                modified = TRUE;
            }

            if (modified)
            {
                PhInvokeCallback(&FwItemModifiedEvent, item);
            }

            continue;
        }

        PhInvokeCallback(&FwItemRemovedEvent, item);
        RemoveEntryList(&item->AgeListEntry);
    }

    PhInvokeCallback(&FwItemsUpdatedEvent, NULL);
    FwRunCount++;

    EtFwFlushCache();
}

/**
 * Enumerates existing WFP net events.
 *
 * \return ERROR_SUCCESS or a Win32 error code returned by the WFP event
 * enumeration APIs.
 * \remarks Enumerated events are passed through the same event callback used by
 * live WFP notifications.
 */
ULONG EtFwMonitorEnumEvents(
    VOID
    )
{
    ULONG status;
    HANDLE enumHandle;
    FWPM_NET_EVENT_ENUM_TEMPLATE enumTemplate;

    memset(&enumTemplate, 0, sizeof(FWPM_NET_EVENT_ENUM_TEMPLATE));

    if (FwpmNetEventCreateEnumHandleEx)
    {
        status = FwpmNetEventCreateEnumHandleEx(
            EtFwEngineHandle,
            FWPM_NET_EVENT_DATA_WFP,
            &enumTemplate,
            &enumHandle
            );
    }
    else
    {
        status = FwpmNetEventCreateEnumHandle(
            EtFwEngineHandle,
            &enumTemplate,
            &enumHandle
            );
    }

    if (status != ERROR_SUCCESS)
        return status;

    while (TRUE)
    {
        FWPM_NET_EVENT** entries;
        ULONG count;

        status = FwpmNetEventEnum(EtFwEngineHandle, enumHandle, ULONG_MAX, &entries, &count);

        if (status != ERROR_SUCCESS)
            break;

        if (count == 0)
        {
            FwpmFreeMemory((PVOID*)&entries);
            break;
        }

        for (ULONG i = 0; i < count; i++)
        {
            EtFwEventCallback(NULL, entries[i]);
        }

        FwpmFreeMemory((PVOID*)&entries);
    }

    FwpmNetEventDestroyEnumHandle(EtFwEngineHandle, enumHandle);

    return status;
}

/**
 * Initializes the persistent firewall monitor WFP session name.
 *
 * \return A referenced session name string.
 * \remarks A GUID-based session name is persisted in settings when one does not
 * already exist.
 */
PPH_STRING EtFwSessionInitialize(
    VOID
    )
{
    PPH_STRING settingsString;
    GUID settingsGuid;

    settingsString = PhGetStringSetting(SETTING_NAME_FW_SESSION_GUID);

    if (PhIsNullOrEmptyString(settingsString))
    {
        PPH_STRING string;

        PhGenerateGuid(&settingsGuid);

        if (string = PhFormatGuid(&settingsGuid))
        {
            PhSetStringSetting2(SETTING_NAME_FW_SESSION_GUID, &string->sr);
            PhMoveReference(&settingsString, string);
        }
        else
        {
            PhMoveReference(&settingsString, PhCreateString(L"SiNetEventSession"));
        }
    }

    return settingsString;
}

/**
 * Initializes the firewall monitor.
 *
 * \return ERROR_SUCCESS or a Win32 error code describing the initialization failure.
 * \remarks This function loads fwpuclnt.dll, resolves WFP entry points, opens a
 * BFE session, enables net event collection, subscribes to net event and filter
 * change notifications and registers the process provider callback.
 */
ULONG EtFwMonitorInitialize(
    VOID
    )
{
    ULONG status;
    PVOID baseAddress;
    FWP_VALUE value;
    FWPM_SESSION session;
    FWPM_NET_EVENT_ENUM_TEMPLATE enumTemplate;
    FWPM_NET_EVENT_SUBSCRIPTION subscription;

    EtFwEnableResolveCache = !!PhGetIntegerSetting(SETTING_ENABLE_NETWORK_RESOLVE);
    EtFwEnableResolveDoH = !!PhGetIntegerSetting(SETTING_ENABLE_NETWORK_RESOLVE_DOH);
    EtFwIgnorePortScan = !!PhGetIntegerSetting(SETTING_NAME_FW_IGNORE_PORTSCAN);
    EtFwIgnoreLoopback = !!PhGetIntegerSetting(SETTING_NAME_FW_IGNORE_LOOPBACK);
    EtFwIgnoreAllow = !!PhGetIntegerSetting(SETTING_NAME_FW_IGNORE_ALLOW);
    EtFwSessionName = EtFwSessionInitialize();

    PhInitializeFreeList(&EtFwPacketFreeList, sizeof(FW_EVENT_PACKET), 64);
    PhInitializeSListHead(&EtFwPacketListHead);
    PhInitializeSListHead(&EtFwQueryListHead);

    EtFwObjectType = PhCreateObjectType(L"FwObject", 0, FwObjectTypeDeleteProcedure);
    EtFwResolveCacheHashtable = PhCreateHashtable(
        sizeof(FW_RESOLVE_CACHE_ITEM),
        EtFwResolveCacheHashtableEqualFunction,
        EtFwResolveCacheHashtableHashFunction,
        20
        );
    EtFwFilterDisplayDataHashTable = PhCreateHashtable(
        sizeof(ETFW_FILTER_DISPLAY_CONTEXT),
        EtFwFilterDisplayDataEqualFunction,
        EtFwFilterDisplayDataHashFunction,
        20
        );

    if (!(baseAddress = PhLoadLibrary(L"fwpuclnt.dll")))
        return ERROR_MOD_NOT_FOUND;

    FwpmEngineOpen = PhGetProcedureAddress(baseAddress, "FwpmEngineOpen0", 0);
    FwpmEngineClose = PhGetProcedureAddress(baseAddress, "FwpmEngineClose0", 0);
    FwpmFreeMemory = PhGetProcedureAddress(baseAddress, "FwpmFreeMemory0", 0);
    FwpmFilterGetById = PhGetProcedureAddress(baseAddress, "FwpmFilterGetById0", 0);
    FwpmLayerGetById_I = PhGetProcedureAddress(baseAddress, "FwpmLayerGetById0", 0);
    FwpmFilterSubscribeChanges_I = PhGetProcedureAddress(baseAddress, "FwpmFilterSubscribeChanges0", 0);
    FwpmFilterUnsubscribeChanges_I = PhGetProcedureAddress(baseAddress, "FwpmFilterUnsubscribeChanges0", 0);
    FwpmEngineSetOption = PhGetProcedureAddress(baseAddress, "FwpmEngineSetOption0", 0);
    FwpmNetEventUnsubscribe = PhGetProcedureAddress(baseAddress, "FwpmNetEventUnsubscribe0", 0);
    FwpmNetEventCreateEnumHandle = PhGetProcedureAddress(baseAddress, "FwpmNetEventCreateEnumHandle0", 0);
    FwpmNetEventCreateEnumHandleEx = PhGetProcedureAddress(baseAddress, "FwpmNetEventCreateEnumHandleEx", 0);
    FwpmNetEventDestroyEnumHandle = PhGetProcedureAddress(baseAddress, "FwpmNetEventDestroyEnumHandle0", 0);

    FwpmNetEventSubscribe = PhGetProcedureAddress(baseAddress, "FwpmNetEventSubscribe4", 0);
    if (!FwpmNetEventSubscribe)
        FwpmNetEventSubscribe = PhGetProcedureAddress(baseAddress, "FwpmNetEventSubscribe3", 0);
    if (!FwpmNetEventSubscribe)
        FwpmNetEventSubscribe = PhGetProcedureAddress(baseAddress, "FwpmNetEventSubscribe2", 0);
    if (!FwpmNetEventSubscribe)
        FwpmNetEventSubscribe = PhGetProcedureAddress(baseAddress, "FwpmNetEventSubscribe1", 0);
    if (!FwpmNetEventSubscribe)
        FwpmNetEventSubscribe = PhGetProcedureAddress(baseAddress, "FwpmNetEventSubscribe0", 0);

    FwpmNetEventEnum = PhGetProcedureAddress(baseAddress, "FwpmNetEventEnum5", 0);
    if (!FwpmNetEventEnum)
        FwpmNetEventEnum = PhGetProcedureAddress(baseAddress, "FwpmNetEventEnum4", 0);
    if (!FwpmNetEventEnum)
        FwpmNetEventEnum = PhGetProcedureAddress(baseAddress, "FwpmNetEventEnum3", 0);
    if (!FwpmNetEventEnum)
        FwpmNetEventEnum = PhGetProcedureAddress(baseAddress, "FwpmNetEventEnum2", 0);
    if (!FwpmNetEventEnum)
        FwpmNetEventEnum = PhGetProcedureAddress(baseAddress, "FwpmNetEventEnum1", 0);
    if (!FwpmNetEventEnum)
        FwpmNetEventEnum = PhGetProcedureAddress(baseAddress, "FwpmNetEventEnum0", 0);

    if (!(
        FwpmEngineOpen &&
        FwpmEngineClose &&
        FwpmFreeMemory &&
        FwpmFilterGetById &&
        FwpmEngineSetOption &&
        FwpmNetEventUnsubscribe &&
        FwpmNetEventCreateEnumHandle &&
        FwpmNetEventDestroyEnumHandle &&
        FwpmNetEventSubscribe &&
        FwpmNetEventEnum
        ))
    {
        return ERROR_PROC_NOT_FOUND;
    }

    // Create a non-dynamic BFE session

    memset(&session, 0, sizeof(FWPM_SESSION));
    session.displayData.name = PhGetString(EtFwSessionName);
    session.displayData.description = L"";

    status = FwpmEngineOpen(
        NULL,
        RPC_C_AUTHN_DEFAULT,
        NULL,
        &session,
        &EtFwEngineHandle
        );

    if (status != ERROR_SUCCESS)
        return status;

    // Enable collection of events

    memset(&value, 0, sizeof(FWP_VALUE));
    value.type = FWP_UINT32;
    value.uint32 = TRUE;

    status = FwpmEngineSetOption(
        EtFwEngineHandle,
        FWPM_ENGINE_COLLECT_NET_EVENTS,
        &value
        );

    if (status != ERROR_SUCCESS)
        return status;

    memset(&value, 0, sizeof(FWP_VALUE));
    value.type = FWP_UINT32;
    value.uint32 = FWPM_NET_EVENT_KEYWORD_INBOUND_MCAST | FWPM_NET_EVENT_KEYWORD_INBOUND_BCAST;
    if (EtWindowsVersion >= WINDOWS_8)
        value.uint32 |= FWPM_NET_EVENT_KEYWORD_CAPABILITY_DROP;
    if (EtWindowsVersion >= WINDOWS_8 && !EtFwIgnoreAllow)
        value.uint32 |= FWPM_NET_EVENT_KEYWORD_CAPABILITY_ALLOW | FWPM_NET_EVENT_KEYWORD_CLASSIFY_ALLOW;
    if (EtWindowsVersion >= WINDOWS_10_19H1 && !EtFwIgnorePortScan)
        value.uint32 |= FWPM_NET_EVENT_KEYWORD_PORT_SCANNING_DROP;

    status = FwpmEngineSetOption(
        EtFwEngineHandle,
        FWPM_ENGINE_NET_EVENT_MATCH_ANY_KEYWORDS,
        &value
        );

    if (status != ERROR_SUCCESS)
        return status;

    //// Enable the name cache
    //
    //memset(&value, 0, sizeof(FWP_VALUE));
    //value.type = FWP_UINT32;
    //value.uint32 = TRUE;
    //
    //status = FwpmEngineSetOption(
    //    EtFwEngineHandle,
    //    FWPM_ENGINE_NAME_CACHE,
    //    &value
    //    );
    //
    //if (status != ERROR_SUCCESS)
    //    return status;

    if (EtWindowsVersion >= WINDOWS_8)
    {
        memset(&value, 0, sizeof(FWP_VALUE));
        value.type = FWP_UINT32;
        value.uint32 = TRUE;
        FwpmEngineSetOption(EtFwEngineHandle, FWPM_ENGINE_MONITOR_IPSEC_CONNECTIONS, &value);

        memset(&value, 0, sizeof(FWP_VALUE));
        value.type = FWP_UINT32;
        value.uint32 = FWPM_ENGINE_OPTION_PACKET_QUEUE_INBOUND | FWPM_ENGINE_OPTION_PACKET_QUEUE_FORWARD | FWPM_ENGINE_OPTION_PACKET_BATCH_INBOUND;
        FwpmEngineSetOption(EtFwEngineHandle, FWPM_ENGINE_PACKET_QUEUING, &value);
    }

    // Subscribe to all event conditions

    memset(&enumTemplate, 0, sizeof(FWPM_NET_EVENT_ENUM_TEMPLATE));
    memset(&subscription, 0, sizeof(FWPM_NET_EVENT_SUBSCRIPTION));

    subscription.sessionKey = session.sessionKey;
    subscription.enumTemplate = &enumTemplate;

    status = FwpmNetEventSubscribe(
        EtFwEngineHandle,
        &subscription,
        (FWPM_NET_EVENT_CALLBACK4)EtFwEventCallback,
        NULL,
        &EtFwEventHandle
        );

    if (status != ERROR_SUCCESS)
        return status;

    if (FwpmFilterSubscribeChanges)
    {
        FWPM_FILTER_SUBSCRIPTION0 filterSubscription;

        memset(&filterSubscription, 0, sizeof(FWPM_FILTER_SUBSCRIPTION0));
        filterSubscription.sessionKey = session.sessionKey;

        FwpmFilterSubscribeChanges(
            EtFwEngineHandle,
            &filterSubscription,
            EtFwFilterChangeCallback,
            NULL,
            &EtFwFilterEventHandle
            );
    }

    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
        EtFwProcessesUpdatedCallback,
        NULL,
        &EtFwProcessesUpdatedCallbackRegistration
        );

    return ERROR_SUCCESS;
}

/**
 * Uninitializes the firewall monitor.
 *
 * \remarks WFP filter and net event subscriptions are removed, net event
 * collection is disabled and the BFE engine handle is closed.
 */
VOID EtFwMonitorUninitialize(
    VOID
    )
{
    if (EtFwFilterEventHandle && FwpmFilterUnsubscribeChanges)
    {
        FwpmFilterUnsubscribeChanges(EtFwEngineHandle, EtFwFilterEventHandle);
        EtFwFilterEventHandle = NULL;
    }

    if (EtFwEventHandle && FwpmNetEventUnsubscribe)
    {
        FwpmNetEventUnsubscribe(EtFwEngineHandle, EtFwEventHandle);
        EtFwEventHandle = NULL;
    }

    if (EtFwEngineHandle)
    {
        if (FwpmEngineSetOption)
        {
            FWP_VALUE value;

            // Disable event collection
            value.type = FWP_UINT32;
            value.uint32 = 0;
            FwpmEngineSetOption(EtFwEngineHandle, FWPM_ENGINE_COLLECT_NET_EVENTS, &value);
        }

        if (FwpmEngineClose)
        {
            FwpmEngineClose(EtFwEngineHandle);
            EtFwEngineHandle = NULL;
        }
    }
}
