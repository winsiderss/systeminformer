/*
 * Process Hacker Firewall Monitor -
 *   firewall monitor
 *
 * Copyright (C) 2012 dmex
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

#include "fwmon.h"
#include <Winsock2.h>
#ifndef INCLUDED_FWPMU
#define INCLUDED_FWPMU
#include <fwpmu.h>
#endif

PH_CALLBACK_DECLARE(FwItemAddedEvent);
PH_CALLBACK_DECLARE(FwItemModifiedEvent);
PH_CALLBACK_DECLARE(FwItemRemovedEvent);
PH_CALLBACK_DECLARE(FwItemsUpdatedEvent);

HANDLE EngineHandle;
HANDLE EventHandle;

VOID CALLBACK DropEventCallback(__inout VOID* pContext, __in const FWPM_NET_EVENT* pEvent)
{
    SYSTEMTIME st;
    FILETIME ft;
    USHORT localPort;
    USHORT remotePort;

    //SID_NAME_USE name;
    //DWORD length;
    //WCHAR szSidName[MAX_PATH];

    WCHAR szLocalDate[255], szLocalTime[255], szProtoType[255], szPacketType[255], szPacketSrcDst[255], szPacketinOut[255];

    FileTimeToLocalFileTime(&pEvent->header.timeStamp, &ft);
    FileTimeToSystemTime(&ft, &st);

    GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, szLocalDate, _countof(szLocalDate));
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szLocalTime, _countof(szLocalTime));

    localPort = _byteswap_ushort(pEvent->header.localPort);
    remotePort = _byteswap_ushort(pEvent->header.remotePort);

    switch (pEvent->header.ipProtocol)
    {
    case IPPROTO_ICMP:
        {
            swprintf_s(
                szProtoType, 
                _countof(szProtoType), 
                L"ICMP"
                );
        }
        break;
    case IPPROTO_IGMP:
        {
            swprintf_s(
                szProtoType, 
                _countof(szProtoType), 
                L"ICMP"
                );
        }
        break;
    case IPPROTO_TCP:
        {
            swprintf_s(
                szProtoType, 
                _countof(szProtoType), 
                L"TCP"
                );
        }
        break;
    case IPPROTO_UDP:
        {
            swprintf_s(
                szProtoType, 
                _countof(szProtoType), 
                L"UDP"
                );
        }
        break;
    default:
        {
            swprintf_s(
                szProtoType, 
                _countof(szProtoType), 
                L"<notImplemented ipProtocol: %d>",
                pEvent->header.ipProtocol
                );
        }
        break;
    /*case IPPROTO_ICMPV6:
        {
            swprintf_s(
                szProtoType, 
                _countof(szProtoType), 
                L"ICMPV6"
                );
        }
        break;*/
    }
    
    switch(pEvent->ipsecDrop->direction)
    {
    case FWP_DIRECTION_INBOUND:
        {
            swprintf_s(
                szPacketinOut, 
                _countof(szPacketinOut), 
                L"Inbound"
                );
        }
        break;
    case FWP_DIRECTION_OUTBOUND:
        {
            swprintf_s(
                szPacketinOut, 
                _countof(szPacketinOut), 
                L"Outbound"
                );
        }
        break;
    }

    switch (pEvent->header.ipVersion)
    {
    case FWP_IP_VERSION_V4:
        {
            swprintf_s(
                szPacketType, 
                _countof(szPacketType), 
                L"IPv4"
                );

            swprintf_s(
                szPacketSrcDst, 
                _countof(szPacketSrcDst), 
                L"SRC: %ld.%ld.%ld.%ld:%ld \nDST: %ld.%ld.%ld.%ld:%ld",
                ((byte*)&pEvent->header.localAddrV4)[3],
                ((byte*)&pEvent->header.localAddrV4)[2],
                ((byte*)&pEvent->header.localAddrV4)[1],
                ((byte*)&pEvent->header.localAddrV4)[0],
                localPort,
                ((byte*)&pEvent->header.remoteAddrV4)[3],
                ((byte*)&pEvent->header.remoteAddrV4)[2],
                ((byte*)&pEvent->header.remoteAddrV4)[1],
                ((byte*)&pEvent->header.remoteAddrV4)[0],
                remotePort
                );
        }
        break;
    case FWP_IP_VERSION_V6:
        {
            swprintf_s(
                szPacketType, 
                _countof(szPacketType), 
                L"IPv6"
                );

            swprintf_s(
                szPacketSrcDst, 
                _countof(szPacketSrcDst), 
                L"SRC: [%x:%x:%x:%x%x:%x:%x:%x]:%ld \nDST: [%x:%x:%x:%x%x:%x:%x:%x]:%ld",
                ((WORD*)&pEvent->header.localAddrV6)[7],
                ((WORD*)&pEvent->header.localAddrV6)[6],
                ((WORD*)&pEvent->header.localAddrV6)[5],
                ((WORD*)&pEvent->header.localAddrV6)[4],
                ((WORD*)&pEvent->header.localAddrV6)[3],
                ((WORD*)&pEvent->header.localAddrV6)[2],
                ((WORD*)&pEvent->header.localAddrV6)[1],
                ((WORD*)&pEvent->header.localAddrV6)[0],
                localPort,
                ((WORD*)&pEvent->header.remoteAddrV6)[7],
                ((WORD*)&pEvent->header.remoteAddrV6)[6],
                ((WORD*)&pEvent->header.remoteAddrV6)[5],
                ((WORD*)&pEvent->header.remoteAddrV6)[4],
                ((WORD*)&pEvent->header.remoteAddrV6)[3],
                ((WORD*)&pEvent->header.remoteAddrV6)[2],
                ((WORD*)&pEvent->header.remoteAddrV6)[1],
                ((WORD*)&pEvent->header.remoteAddrV6)[0],
                remotePort);
        }
        break;
    }

    
    if (IsValidSid(pEvent->header.userId))
    {
        SID_NAME_USE eUse = SidTypeUnknown;
        DWORD dwAcctName = 256, dwDomainName = 256;
        WCHAR AcctName[256];
        WCHAR DomainName[256];

        LookupAccountSidW(NULL, pEvent->header.userId, AcctName, (LPDWORD)&dwAcctName, DomainName, (LPDWORD)&dwDomainName, &eUse);

        wprintf(
            L"%s %s %s Packet dropped: \nUser: %s \nAppID: %s \n%s \n%s %s\n\n", 
            szPacketType, 
            szProtoType,
            szPacketinOut,
            AcctName, 
            pEvent->header.appId.data, 
            szPacketSrcDst, 
            szLocalDate, 
            szLocalTime
            );
    }
    else
    {
        wprintf(
            L"%s %s %s Packet dropped: \nAppID: %s \n%s \n%s %s\n\n", 
            szPacketType, 
            szProtoType,
            szPacketinOut,
            pEvent->header.appId.data, 
            szPacketSrcDst, 
            szLocalDate, 
            szLocalTime
            );
    }

    // Do whatever you need for the event
}

ULONG StartFwMonitor(
    VOID
    )
{
    HANDLE engineHandle = 0, eventHandle = 0;
    FWPM_SESSION session = { 0 };
    FWP_VALUE0 value = { 0 };
    FWPM_NET_EVENT_ENUM_TEMPLATE enumTemplate = { 0 };
    FWPM_NET_EVENT_SUBSCRIPTION subscription = { 0 };
    DWORD result = 0;

    session.flags = 0;
    session.displayData.name  = L"PhFirewallMonitoringSession";
    session.displayData.description = L"Non-Dynamic session for Process Hacker";

    // Create a non-dynamic BFE session
    result = FwpmEngineOpen0(
        NULL,
        RPC_C_AUTHN_WINNT,
        NULL,
        &session,
        &engineHandle
        );

    if (result != ERROR_SUCCESS)
    {
        StopFwMonitor();
        return result;
    }

    value.type = FWP_UINT32;
    value.uint32 = 1;

    // Enable collection of NetEvents
    result = FwpmEngineSetOption(
        engineHandle,
        FWPM_ENGINE_COLLECT_NET_EVENTS,
        &value
        );

    if (result != ERROR_SUCCESS)
    {
        StopFwMonitor();
        return result;
    }

    enumTemplate.numFilterConditions = 0; // get events for all conditions

    subscription.sessionKey = session.sessionKey;
    subscription.enumTemplate = &enumTemplate;

    // Subscribe to the events
    result = FwpmNetEventSubscribe(
        engineHandle,
        &subscription,
        DropEventCallback,
        0,
        &eventHandle
        );

    if (result != ERROR_SUCCESS)
    {
        StopFwMonitor();
        return result;
    }

    return ERROR_SUCCESS;
}

VOID StopFwMonitor(
    VOID
    )
{
    FWP_VALUE0 value = { 0 };

    if (EventHandle)
    {
        FwpmNetEventUnsubscribe(
            EngineHandle,
            EventHandle
            );

        EventHandle = 0;
    }

    if (EngineHandle)
    {
        value.type = FWP_UINT32;
        value.uint32 = 0;

        // Disable collection of NetEvents
        FwpmEngineSetOption(EngineHandle,
            FWPM_ENGINE_COLLECT_NET_EVENTS,
            &value);

        FwpmEngineClose(EngineHandle);

        EngineHandle = 0;
    }
}
