/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2023
 *
 */

#include "exttools.h"
#include <workqueue.h>
#include <fwpmu.h>
#include <fwpsu.h>

PH_CALLBACK_REGISTRATION EtFwProcessesUpdatedCallbackRegistration;
PPH_OBJECT_TYPE EtFwObjectType = NULL;
HANDLE EtFwEngineHandle = NULL;
HANDLE EtFwEventHandle = NULL;
ULONG FwRunCount = 0;
ULONG EtFwMaxEventAge = 60;
SLIST_HEADER EtFwPacketListHead;
PH_FREE_LIST EtFwPacketFreeList;
LIST_ENTRY EtFwAgeListHead = { &EtFwAgeListHead, &EtFwAgeListHead };
BOOLEAN EtFwEnableResolveCache = TRUE;
BOOLEAN EtFwEnableResolveDoH = FALSE;
BOOLEAN EtFwIgnoreOnError = TRUE;
BOOLEAN EtFwIgnorePortScan = FALSE;
BOOLEAN EtFwIgnoreLoopback = TRUE;
BOOLEAN EtFwIgnoreAllow = FALSE;
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
_FwpmLayerGetById0 FwpmLayerGetById_I = NULL;
_FwpmNetEventSubscribe4 FwpmNetEventSubscribe_I = NULL;
_FwpmNetEventUnsubscribe0 FwpmNetEventUnsubscribe_I = NULL;
_FwpmNetEventCreateEnumHandle0 FwpmNetEventCreateEnumHandle_I = NULL;
_FwpmNetEventDestroyEnumHandle0 FwpmNetEventDestroyEnumHandle_I = NULL;
_FwpmNetEventEnum5 FwpmNetEventEnum_I = NULL;

#undef FwpmEngineOpen
#undef FwpmEngineClose
#undef FwpmFreeMemory
#undef FwpmFilterGetById0
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
#define FwpmLayerGetById0 FwpmLayerGetById_I
#define FwpmEngineSetOption FwpmEngineSetOption_I
#define FwpmNetEventSubscribe FwpmNetEventSubscribe_I
#define FwpmNetEventUnsubscribe FwpmNetEventUnsubscribe_I
#define FwpmNetEventCreateEnumHandle FwpmNetEventCreateEnumHandle_I
#define FwpmNetEventDestroyEnumHandle FwpmNetEventDestroyEnumHandle_I
#define FwpmNetEventEnum FwpmNetEventEnum_I

typedef struct _FW_EVENT
{
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
    UINT CountryIconIndex;

    PSID UserSid;
    //PSID PackageSid;
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
    PFW_RESOLVE_CACHE_ITEM cacheItem1 = (PFW_RESOLVE_CACHE_ITEM)Entry1;
    PFW_RESOLVE_CACHE_ITEM cacheItem2 = (PFW_RESOLVE_CACHE_ITEM)Entry2;

    return PhEqualIpAddress(&cacheItem1->Address, &cacheItem2->Address);
}

ULONG NTAPI EtFwResolveCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PFW_RESOLVE_CACHE_ITEM cacheItem = (PFW_RESOLVE_CACHE_ITEM)Entry;

    return PhHashIpAddress(&cacheItem->Address);
}

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
}

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

VOID FwPushFirewallEvent(
    _In_ PFW_EVENT Event
    )
{
    PFW_EVENT_PACKET packet;

    packet = PhAllocateFromFreeList(&EtFwPacketFreeList);
    memcpy(&packet->Event, Event, sizeof(FW_EVENT));
    RtlInterlockedPushEntrySList(&EtFwPacketListHead, &packet->ListEntry);
}

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

    // Add the item to the age list.
    entry->RunId = RunId;
    InsertHeadList(&EtFwAgeListHead, &entry->AgeListEntry);

    // Queue hostname lookup.
    PhpQueryHostnameForEntry(entry);

    // Raise the item added event.
    PhInvokeCallback(&FwItemAddedEvent, entry);
}

_Success_(return)
BOOLEAN FwProcessEventType(
    _In_ const FWPM_NET_EVENT* FwEvent,
    _Out_ PBOOLEAN IsLoopback,
    _Out_ PULONG Direction,
    _Out_ PULONG64 FilterId,
    _Out_ PUSHORT LayerId
    )
{
    switch (FwEvent->type)
    {
    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP:
        {
            FWPM_NET_EVENT_CLASSIFY_DROP* fwDropEvent = FwEvent->classifyDrop;

            if (EtWindowsVersion >= WINDOWS_10 && fwDropEvent->isLoopback && EtFwIgnoreLoopback)
                return FALSE;

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
            //FWPM_NET_EVENT_IKEEXT_MM_FAILURE* fwIkeMmFailureEvent = FwEvent->ikeMmFailure;
            //UINT32 failureErrorCode;
            //IPSEC_FAILURE_POINT failurePoint;
            //UINT32 flags;
            //IKEEXT_KEY_MODULE_TYPE keyingModuleType;
            //IKEEXT_MM_SA_STATE mmState;
            //IKEEXT_SA_ROLE saRole;
            //IKEEXT_AUTHENTICATION_METHOD_TYPE mmAuthMethod;
            //UINT8 endCertHash[20];
            //UINT64 mmId;
            //UINT64 mmFilterId;
            //wchar_t* localPrincipalNameForAuth;
            //wchar_t* remotePrincipalNameForAuth;
            //UINT32 numLocalPrincipalGroupSids;
            //LPWSTR* localPrincipalGroupSids;
            //UINT32 numRemotePrincipalGroupSids;
            //LPWSTR* remotePrincipalGroupSids;
            //GUID* providerContextKey;
        }
        return FALSE;
    case FWPM_NET_EVENT_TYPE_IKEEXT_QM_FAILURE:
        {
            //FWPM_NET_EVENT_IKEEXT_QM_FAILURE* fwIkeQmFailureEvent = FwEvent->ikeQmFailure;
            //UINT32 failureErrorCode;
            //IPSEC_FAILURE_POINT failurePoint;
            //IKEEXT_KEY_MODULE_TYPE keyingModuleType;
            //IKEEXT_QM_SA_STATE qmState;
            //IKEEXT_SA_ROLE saRole;
            //IPSEC_TRAFFIC_TYPE saTrafficType;
            //union
            //{
            //    FWP_CONDITION_VALUE0 localSubNet;
            //};
            //union
            //{
            //    FWP_CONDITION_VALUE0 remoteSubNet;
            //};
            //UINT64 qmFilterId;
            //UINT64 mmSaLuid;
            //GUID mmProviderContextKey;
        }
        return FALSE;
    case FWPM_NET_EVENT_TYPE_IKEEXT_EM_FAILURE:
        {
            //FWPM_NET_EVENT_IKEEXT_EM_FAILURE* fwIkeEmFailureEvent = FwEvent->ikeEmFailure;
            //UINT32 failureErrorCode;
            //IPSEC_FAILURE_POINT failurePoint;
            //UINT32 flags;
            //IKEEXT_EM_SA_STATE emState;
            //IKEEXT_SA_ROLE saRole;
            //IKEEXT_AUTHENTICATION_METHOD_TYPE emAuthMethod;
            //UINT8 endCertHash[20];
            //UINT64 mmId;
            //UINT64 qmFilterId;
            //wchar_t* localPrincipalNameForAuth;
            //wchar_t* remotePrincipalNameForAuth;
            //UINT32 numLocalPrincipalGroupSids;
            //LPWSTR* localPrincipalGroupSids;
            //UINT32 numRemotePrincipalGroupSids;
            //LPWSTR* remotePrincipalGroupSids;
            //IPSEC_TRAFFIC_TYPE saTrafficType;
        }
        return FALSE;
    case FWPM_NET_EVENT_TYPE_IPSEC_KERNEL_DROP:
        {
            //FWPM_NET_EVENT_IPSEC_KERNEL_DROP* fwIpsecDropEvent = FwEvent->ipsecDrop;
            //INT32 failureStatus;
            //FWP_DIRECTION direction;
            //IPSEC_SA_SPI spi;
            //UINT64 filterId;
            //UINT16 layerId;
        }
        return FALSE;
    case FWPM_NET_EVENT_TYPE_IPSEC_DOSP_DROP:
        {
            //FWPM_NET_EVENT_IPSEC_DOSP_DROP* fwIdpDropEvent = FwEvent->idpDrop;
            //FWP_IP_VERSION ipVersion;
            //union
            //{
            //    UINT32 publicHostV4Addr;
            //    UINT8 publicHostV6Addr[16];
            //};
            //union
            //{
            //    UINT32 internalHostV4Addr;
            //    UINT8 internalHostV6Addr[16];
            //};
            //INT32 failureStatus;
            //FWP_DIRECTION direction;
        }
        return FALSE;
    case FWPM_NET_EVENT_TYPE_CAPABILITY_DROP:
        {
            FWPM_NET_EVENT_CAPABILITY_DROP* fwCapabilityDropEvent = FwEvent->capabilityDrop;
            //FWPM_APPC_NETWORK_CAPABILITY_TYPE networkCapabilityId = fwCapabilityDropEvent->networkCapabilityId;

            if (EtWindowsVersion >= WINDOWS_10 && fwCapabilityDropEvent->isLoopback && EtFwIgnoreLoopback)
                return FALSE;

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
            //FWPM_APPC_NETWORK_CAPABILITY_TYPE networkCapabilityId = fwCapabilityAllowEvent->networkCapabilityId;

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
            //FWPM_NET_EVENT_CLASSIFY_DROP_MAC* fwClassifyDropMacEvent = FwEvent->classifyDropMac;
            //FWP_BYTE_ARRAY6 localMacAddr;
            //FWP_BYTE_ARRAY6 remoteMacAddr;
            //UINT32 mediaType;
            //UINT32 ifType;
            //UINT16 etherType;
            //UINT32 ndisPortNumber;
            //UINT32 reserved;
            //UINT16 vlanTag;
            //UINT64 ifLuid;
            //UINT64 filterId;
            //UINT16 layerId;
            //UINT32 reauthReason;
            //UINT32 originalProfile;
            //UINT32 currentProfile;
            //UINT32 msFwpDirection;
            //BOOL isLoopback;
            //FWP_BYTE_BLOB vSwitchId;
            //UINT32 vSwitchSourcePort;
            //UINT32 vSwitchDestinationPort;
        }
        return FALSE;
    case FWPM_NET_EVENT_TYPE_LPM_PACKET_ARRIVAL:
        {
            //FWPM_NET_EVENT_LPM_PACKET_ARRIVAL* fwLpmPacketArrivalEvent = FwEvent->lpmPacketArrival;
            //IPSEC_SA_SPI spi;
        }
        return FALSE;
    }

    return FALSE;
}

PPH_STRING EtFwGetDnsReverseNameFromAddress(
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
                WCHAR reverseNameBuffer[IP6_REVERSE_DOMAIN_STRING_LENGTH];

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
    default:
        return NULL;
    }
}

PPH_STRING EtFwGetNameFromAddress(
    _In_ PPH_IP_ADDRESS Address
    )
{
    BOOLEAN dnsLocalQuery = FALSE;
    PPH_STRING addressEndpointString = NULL;
    PPH_STRING addressReverseString;
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

    if (!(addressReverseString = EtFwGetDnsReverseNameFromAddress(Address)))
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

VOID PhpQueryHostnameForEntry(
    _In_ PFW_EVENT_ITEM Entry
    )
{
    PFW_RESOLVE_CACHE_ITEM cacheItem;

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

#define NETWORKTOOLS_PLUGIN_NAME L"ProcessHacker.NetworkTools"
#define NETWORKTOOLS_INTERFACE_VERSION 1

typedef BOOLEAN (NTAPI* PNETWORKTOOLS_GET_COUNTRYCODE)(
    _In_ PH_IP_ADDRESS RemoteAddress,
    _Out_ ULONG* CountryCode,
    _Out_ PPH_STRING* CountryName
    );
typedef INT (NTAPI* PNETWORKTOOLS_GET_COUNTRYICON)(
    _In_ ULONG Name
    );
typedef BOOLEAN (NTAPI* PNETWORKTOOLS_GET_SERVICENAME)(
    _In_ ULONG Port,
    _Out_ PPH_STRINGREF ServiceName
    );
typedef VOID (NTAPI* PNETWORKTOOLS_DRAW_COUNTRYICON)(
    _In_ HDC hdc,
    _In_ RECT rect,
    _In_ INT Index
    );
typedef VOID (NTAPI* PNETWORKTOOLS_SHOWWINDOW_PING)(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    );
typedef VOID (NTAPI* PNETWORKTOOLS_SHOWWINDOW_TRACERT)(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    );
typedef VOID (NTAPI* PNETWORKTOOLS_SHOWWINDOW_WHOIS)(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    );

typedef struct _NETWORKTOOLS_INTERFACE
{
    ULONG Version;
    PNETWORKTOOLS_GET_COUNTRYCODE LookupCountryCode;
    PNETWORKTOOLS_GET_COUNTRYICON LookupCountryIcon;
    PNETWORKTOOLS_GET_SERVICENAME LookupPortServiceName;
    PNETWORKTOOLS_DRAW_COUNTRYICON DrawCountryIcon;
    PNETWORKTOOLS_SHOWWINDOW_PING ShowPingWindow;
    PNETWORKTOOLS_SHOWWINDOW_TRACERT ShowTracertWindow;
    PNETWORKTOOLS_SHOWWINDOW_WHOIS ShowWhoisWindow;
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

VOID EtFwShowPingWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    )
{
    if (EtFwGetPluginInterface())
        EtFwGetPluginInterface()->ShowPingWindow(ParentWindowHandle, Endpoint);
}

VOID EtFwShowTracerWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    )
{
    if (EtFwGetPluginInterface())
        EtFwGetPluginInterface()->ShowTracertWindow(ParentWindowHandle, Endpoint);
}

VOID EtFwShowWhoisWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    )
{
    if (EtFwGetPluginInterface())
        EtFwGetPluginInterface()->ShowWhoisWindow(ParentWindowHandle, Endpoint);
}

_Success_(return)
BOOLEAN EtFwLookupPortServiceName(
    _In_ ULONG Port,
    _Out_ PPH_STRINGREF ServiceName
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

static BOOLEAN NTAPI EtFwFilterDisplayDataEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PETFW_FILTER_DISPLAY_CONTEXT entry1 = Entry1;
    PETFW_FILTER_DISPLAY_CONTEXT entry2 = Entry2;

    return entry1->FilterId == entry2->FilterId;
}

static ULONG NTAPI EtFwFilterDisplayDataHashFunction(
    _In_ PVOID Entry
    )
{
    PETFW_FILTER_DISPLAY_CONTEXT entry = Entry;

    return PhHashInt64(entry->FilterId);
}

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

VOID EtFwRemoveFilterDisplayData(
    _In_ ULONG64 FilterId
    )
{
    ETFW_FILTER_DISPLAY_CONTEXT lookupEntry;

    lookupEntry.FilterId = FilterId;

    PhRemoveEntryHashtable(EtFwFilterDisplayDataHashTable, &lookupEntry);
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

_Success_(return)
BOOLEAN EtFwLookupAddressClass(
    _In_ PPH_IP_ADDRESS Address,
    _Out_ PPH_STRINGREF ClassString
    )
{
    switch (Address->Type)
    {
    case PH_IPV4_NETWORK_TYPE:
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
    case PH_IPV6_NETWORK_TYPE:
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

_Success_(return)
BOOLEAN EtFwLookupAddressScope(
    _In_ PPH_IP_ADDRESS Address,
    _Out_ PPH_STRINGREF ScopeString
    )
{
    switch (Address->Type)
    {
    case PH_IPV4_NETWORK_TYPE:
        {
            switch (Ipv4AddressScope((PUCHAR)&Address->Ipv4))
            {
            case ScopeLevelInterface:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Interface");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelLink:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Link");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelSubnet:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Subnet");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelAdmin:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Admin");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelSite:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Site");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelOrganization:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Organization");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelGlobal:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Global");
                    *ScopeString = string;
                }
                return TRUE;
            }
        }
        break;
    case PH_IPV6_NETWORK_TYPE:
        {
            switch (Ipv6AddressScope((PUCHAR)&Address->Ipv6))
            {
            case ScopeLevelInterface:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Interface");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelLink:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Link");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelSubnet:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Subnet");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelAdmin:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Admin");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelSite:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Site");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelOrganization:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Organization");
                    *ScopeString = string;
                }
                return TRUE;
            case ScopeLevelGlobal:
                {
                    static const PH_STRINGREF string = PH_STRINGREF_INIT(L"Global");
                    *ScopeString = string;
                }
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

BOOLEAN EtFwSidFullNameCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PETFW_SID_FULL_NAME_CACHE_ENTRY entry1 = Entry1;
    PETFW_SID_FULL_NAME_CACHE_ENTRY entry2 = Entry2;

    return PhEqualSid(entry1->Sid, entry2->Sid);
}

ULONG EtFwSidFullNameCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PETFW_SID_FULL_NAME_CACHE_ENTRY entry = Entry;

    return PhHashBytes(entry->Sid, PhLengthSid(entry->Sid));
}

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

BOOLEAN EtFwFileNameProcessCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PETFW_NAME_PROCESS_CACHE_ENTRY entry1 = Entry1;
    PETFW_NAME_PROCESS_CACHE_ENTRY entry2 = Entry2;

    return entry1->FileNameHash == entry2->FileNameHash;
}

ULONG EtFwFileNameProcessCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PETFW_NAME_PROCESS_CACHE_ENTRY entry = Entry;

    return entry->FileNameHash;
}

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

VOID CALLBACK EtFwEventCallback(
    _In_opt_ PVOID FwContext,
    _In_ const FWPM_NET_EVENT* FwEvent
    )
{
    FW_EVENT entry;
    BOOLEAN isLoopback = FALSE;
    ULONG direction = ULONG_MAX;
    ULONG64 filterId = 0;
    USHORT layerId = 0;
    PPH_STRING ruleName = NULL;
    PPH_STRING ruleDescription = NULL;

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

    if (EtWindowsVersion >= WINDOWS_10 && EtFwIgnoreOnError)
    {
        if (
            layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 || // IsEqualGUID(layerKey, FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4)
            layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6 || // IsEqualGUID(layerKey, FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6)
            layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 || // IsEqualGUID(layerKey, FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4)
            layerId == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 // IsEqualGUID(layerKey, FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6)
            )
        {
            return;
        }
    }

    EtFwGetFilterDisplayData(filterId, &ruleName, &ruleDescription);

    memset(&entry, 0, sizeof(FW_EVENT));
    entry.TimeStamp.HighPart = FwEvent->header.timeStamp.dwHighDateTime;
    entry.TimeStamp.LowPart = FwEvent->header.timeStamp.dwLowDateTime;
    entry.Type = FwEvent->type;
    entry.IsLoopback = isLoopback;
    entry.Direction = direction;
    entry.RuleName = ruleName;
    entry.RuleDescription = ruleDescription;

    if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_IP_PROTOCOL_SET))
    {
        entry.IpProtocol = FwEvent->header.ipProtocol;
    }

    if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_SCOPE_ID_SET))
    {
        entry.ScopeId = FwEvent->header.scopeId;
    }

    if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_IP_VERSION_SET))
    {
        if (FwEvent->header.ipVersion == FWP_IP_VERSION_V4)
        {
            if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_LOCAL_ADDR_SET))
            {
                entry.LocalEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                entry.LocalEndpoint.Address.Ipv4 = _byteswap_ulong(FwEvent->header.localAddrV4);
            }

            if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_LOCAL_PORT_SET))
            {
                entry.LocalEndpoint.Port = FwEvent->header.localPort;
            }

            if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_REMOTE_ADDR_SET))
            {
                entry.RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                entry.RemoteEndpoint.Address.Ipv4 = _byteswap_ulong(FwEvent->header.remoteAddrV4);
            }

            if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_REMOTE_PORT_SET))
            {
                entry.RemoteEndpoint.Port = FwEvent->header.remotePort;
            }
        }
        else if (FwEvent->header.ipVersion == FWP_IP_VERSION_V6)
        {
            if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_LOCAL_ADDR_SET))
            {
                entry.LocalEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                memcpy(entry.LocalEndpoint.Address.Ipv6, FwEvent->header.localAddrV6.byteArray16, 16);
            }

            if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_LOCAL_PORT_SET))
            {
                entry.LocalEndpoint.Port = FwEvent->header.localPort;
            }

            if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_REMOTE_ADDR_SET))
            {
                entry.RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                memcpy(entry.RemoteEndpoint.Address.Ipv6, FwEvent->header.remoteAddrV6.byteArray16, 16);
            }

            if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_REMOTE_PORT_SET))
            {
                entry.RemoteEndpoint.Port = FwEvent->header.remotePort;
            }
        }
    }

    if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_APP_ID_SET))
    {
        if (FwEvent->header.appId.data && FwEvent->header.appId.size > sizeof(UNICODE_NULL))
        {
            PPH_STRING fileName;

            fileName = PhCreateStringEx(
                (PWSTR)FwEvent->header.appId.data,
                (SIZE_T)FwEvent->header.appId.size - sizeof(UNICODE_NULL)
                );

            entry.ProcessFileName = PhGetFileName(fileName);
            entry.ProcessBaseString = PhGetBaseName(entry.ProcessFileName);

            if (entry.ProcessItem = EtFwGetFileNameProcessCachedSlow(fileName))
            {
                PhSwapReference(&entry.ProcessFileNameWin32, entry.ProcessItem->FileNameWin32);
            }

            PhDereferenceObject(fileName);
        }
    }

    if (FlagOn(FwEvent->header.flags, FWPM_NET_EVENT_FLAG_USER_ID_SET))
    {
        //if (entry.ProcessItem && PhEqualSid(FwEvent->header.userId, entry.ProcessItem->Sid))
        if (FwEvent->header.userId)
        {
            entry.UserSid = PhAllocateCopy(FwEvent->header.userId, PhLengthSid(FwEvent->header.userId));
        }
    }

    //if (FwEvent->header.flags & FWPM_NET_EVENT_FLAG_PACKAGE_ID_SET)
    //{
    //    SID PhSeNobodySid = { SID_REVISION, 1, SECURITY_NULL_SID_AUTHORITY, { SECURITY_NULL_RID } };
    //    if (FwEvent->header.packageSid && !PhEqualSid(FwEvent->header.packageSid, &PhSeNobodySid))
    //    {
    //        entry.PackageSid = PhAllocateCopy(FwEvent->header.packageSid, PhLengthSid(FwEvent->header.packageSid));
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

    if (PhIsNullOrEmptyString(entry.ProcessFileName) || PhIsNullOrEmptyString(entry.ProcessBaseString))
    {
        PhMoveReference(&entry.ProcessItem, PhReferenceProcessItem(SYSTEM_PROCESS_ID));
        PhSwapReference(&entry.ProcessFileName, entry.ProcessItem->FileName);
        PhSwapReference(&entry.ProcessFileNameWin32, entry.ProcessItem->FileNameWin32);
        PhSwapReference(&entry.ProcessBaseString, entry.ProcessItem->ProcessName);
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

ULONG EtFwMonitorEnumEvents(
    VOID
    )
{
    ULONG status;
    HANDLE enumHandle;
    FWPM_NET_EVENT_ENUM_TEMPLATE enumTemplate;

    memset(&enumTemplate, 0, sizeof(FWPM_NET_EVENT_ENUM_TEMPLATE));

    status = FwpmNetEventCreateEnumHandle(
        EtFwEngineHandle,
        &enumTemplate,
        &enumHandle
        );

    if (status == ERROR_SUCCESS)
    {
        while (TRUE)
        {
            #define FWPM_NET_EVENTS_COUNT 100
            UINT32 count = 0;
            FWPM_NET_EVENT** events;

            status = FwpmNetEventEnum(EtFwEngineHandle, enumHandle, FWPM_NET_EVENTS_COUNT, &events, &count);

            if (status != ERROR_SUCCESS || count == 0)
                break;

            for (UINT32 i = 0; i < count; i++)
            {
                if (EtFwIgnoreAllow)
                {
                    if (events[i]->type == FWPM_NET_EVENT_TYPE_CLASSIFY_ALLOW || events[i]->type == FWPM_NET_EVENT_TYPE_CAPABILITY_ALLOW)
                        continue;
                }

                EtFwEventCallback(NULL, events[i]);
            }

            FwpmFreeMemory((PVOID*)&events); // Note: Fixes crash on Win10-17763 (dmex)

            if (count < FWPM_NET_EVENTS_COUNT)
                break;
        }

        FwpmNetEventDestroyEnumHandle(EtFwEngineHandle, enumHandle);
    }

    return status;
}

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

    EtFwEnableResolveCache = !!PhGetIntegerSetting(L"EnableNetworkResolve");
    EtFwEnableResolveDoH = !!PhGetIntegerSetting(L"EnableNetworkResolveDoH");
    EtFwIgnorePortScan = !!PhGetIntegerSetting(SETTING_NAME_FW_IGNORE_PORTSCAN);
    EtFwIgnoreLoopback = !!PhGetIntegerSetting(SETTING_NAME_FW_IGNORE_LOOPBACK);
    EtFwIgnoreAllow = !!PhGetIntegerSetting(SETTING_NAME_FW_IGNORE_ALLOW);

    PhInitializeFreeList(&EtFwPacketFreeList, sizeof(FW_EVENT_PACKET), 64);
    RtlInitializeSListHead(&EtFwPacketListHead);
    RtlInitializeSListHead(&EtFwQueryListHead);

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
        return GetLastError();

    FwpmEngineOpen = PhGetProcedureAddress(baseAddress, "FwpmEngineOpen0", 0);
    FwpmEngineClose = PhGetProcedureAddress(baseAddress, "FwpmEngineClose0", 0);
    FwpmFreeMemory = PhGetProcedureAddress(baseAddress, "FwpmFreeMemory0", 0);
    FwpmFilterGetById = PhGetProcedureAddress(baseAddress, "FwpmFilterGetById0", 0);
    FwpmEngineSetOption = PhGetProcedureAddress(baseAddress, "FwpmEngineSetOption0", 0);
    FwpmNetEventUnsubscribe = PhGetProcedureAddress(baseAddress, "FwpmNetEventUnsubscribe0", 0);
    FwpmNetEventCreateEnumHandle = PhGetProcedureAddress(baseAddress, "FwpmNetEventCreateEnumHandle0", 0);
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
    session.displayData.name = L"SiNetEventSession";
    session.displayData.description = L"";

    status = FwpmEngineOpen(
        NULL,
        RPC_C_AUTHN_WINNT,
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

    //if (EtWindowsVersion >= WINDOWS_8)
    //{
    //    memset(&value, 0, sizeof(FWP_VALUE));
    //    value.type = FWP_UINT32;
    //    value.uint32 = TRUE;
    //    FwpmEngineSetOption(EtFwEngineHandle, FWPM_ENGINE_MONITOR_IPSEC_CONNECTIONS, &value);
    //
    //    memset(&value, 0, sizeof(FWP_VALUE));
    //    value.type = FWP_UINT32;
    //    value.uint32 = FWPM_ENGINE_OPTION_PACKET_QUEUE_INBOUND | FWPM_ENGINE_OPTION_PACKET_QUEUE_FORWARD | FWPM_ENGINE_OPTION_PACKET_BATCH_INBOUND;
    //    FwpmEngineSetOption(EtFwEngineHandle, FWPM_ENGINE_PACKET_QUEUING, &value);
    //}

    // Subscribe to all event conditions

    memset(&enumTemplate, 0, sizeof(FWPM_NET_EVENT_ENUM_TEMPLATE));
    memset(&subscription, 0, sizeof(FWPM_NET_EVENT_SUBSCRIPTION));

    subscription.sessionKey = session.sessionKey;
    subscription.enumTemplate = &enumTemplate;

    status = FwpmNetEventSubscribe(
        EtFwEngineHandle,
        &subscription,
        EtFwEventCallback,
        NULL,
        &EtFwEventHandle
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

VOID EtFwMonitorUninitialize(
    VOID
    )
{
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
