/*
 * Process Hacker Plugins -
 *   Network Adapters Plugin
 *
 * Copyright (C) 2015-2016 dmex
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

#include "main.h"

static VOID AdapterEntryDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_NETADAPTER_ENTRY Entry = Object;

    PhAcquireQueuedLockExclusive(&NetworkAdaptersListLock);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        PPH_NETADAPTER_ENTRY currentEntry;

        currentEntry = (PPH_NETADAPTER_ENTRY)NetworkAdaptersList->Items[i];

        if (WindowsVersion >= WINDOWS_VISTA)
        {
            if (Entry->InterfaceLuid.Value == currentEntry->InterfaceLuid.Value)
            {
                PhRemoveItemList(NetworkAdaptersList, i);
                break;
            }
        }
        else
        {
            if (Entry->InterfaceIndex == currentEntry->InterfaceIndex)
            {
                PhRemoveItemList(NetworkAdaptersList, i);
                break;
            }
        }
    }

    PhReleaseQueuedLockExclusive(&NetworkAdaptersListLock);

    PhClearReference(&Entry->InterfaceGuid);
    PhClearReference(&Entry->AdapterName);

    PhDeleteCircularBuffer_ULONG64(&Entry->InboundBuffer);
    PhDeleteCircularBuffer_ULONG64(&Entry->OutboundBuffer);
}

VOID NetAdaptersInitialize(
    VOID
    )
{
    NetworkAdaptersList = PhCreateList(1);
    NetAdapterEntryType = PhCreateObjectType(L"NetAdapterEntry", 0, AdapterEntryDeleteProcedure);
}

VOID NetAdaptersUpdate(
    VOID
    )
{
    PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        HANDLE adapterHandle = NULL;
        PPH_NETADAPTER_ENTRY entry;
        ULONG64 networkInOctets = 0;
        ULONG64 networkOutOctets = 0;
        ULONG64 networkRcvSpeed = 0;
        ULONG64 networkXmitSpeed = 0;
        NDIS_MEDIA_CONNECT_STATE mediaState = MediaConnectStateUnknown;

        entry = (PPH_NETADAPTER_ENTRY)NetworkAdaptersList->Items[i];

        if (PhGetIntegerSetting(SETTING_NAME_ENABLE_NDIS))
        {
            // Create the handle to the network device
            PhCreateFileWin32(
                &adapterHandle,
                PhaConcatStrings(2, L"\\\\.\\", entry->InterfaceGuid->Buffer)->Buffer,
                FILE_GENERIC_READ,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );
        }

        if (adapterHandle)
        {
            NDIS_STATISTICS_INFO interfaceStats;
            NDIS_LINK_STATE interfaceState;

            memset(&interfaceStats, 0, sizeof(NDIS_STATISTICS_INFO));

            NetworkAdapterQueryStatistics(adapterHandle, &interfaceStats);

            if (NT_SUCCESS(NetworkAdapterQueryLinkState(adapterHandle, &interfaceState)))
            {
                mediaState = interfaceState.MediaConnectState;
            }

            if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV))
                networkInOctets = NetworkAdapterQueryValue(adapterHandle, OID_GEN_BYTES_RCV);
            else
                networkInOctets = interfaceStats.ifHCInOctets;

            if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT))
                networkOutOctets = NetworkAdapterQueryValue(adapterHandle, OID_GEN_BYTES_XMIT);
            else
                networkOutOctets = interfaceStats.ifHCOutOctets;

            networkRcvSpeed = networkInOctets - entry->LastInboundValue;
            networkXmitSpeed = networkOutOctets - entry->LastOutboundValue;

            // HACK: Pull the Adapter name from the current query.
            if (!entry->AdapterName)
            {
                entry->AdapterName = NetworkAdapterQueryName(adapterHandle, entry->InterfaceGuid);
            }

            NtClose(adapterHandle);
        }
        else if (GetIfEntry2_I)
        {
            MIB_IF_ROW2 interfaceRow;

            interfaceRow = QueryInterfaceRowVista(entry);

            networkInOctets = interfaceRow.InOctets;
            networkOutOctets = interfaceRow.OutOctets;
            mediaState = interfaceRow.MediaConnectState;
            networkRcvSpeed = networkInOctets - entry->LastInboundValue;
            networkXmitSpeed = networkOutOctets - entry->LastOutboundValue;

            // HACK: Pull the Adapter name from the current query.
            if (!entry->AdapterName)
            {
                entry->AdapterName = PhCreateString(interfaceRow.Description);
            }
        }
        else
        {
            MIB_IFROW interfaceRow;

            interfaceRow = QueryInterfaceRowXP(entry);

            networkInOctets = interfaceRow.dwInOctets;
            networkOutOctets = interfaceRow.dwOutOctets;
            networkRcvSpeed = networkInOctets - entry->LastInboundValue;
            networkXmitSpeed = networkOutOctets - entry->LastOutboundValue;

            if (interfaceRow.dwOperStatus == IF_OPER_STATUS_OPERATIONAL)
                mediaState = MediaConnectStateConnected;
            else
                mediaState = MediaConnectStateUnknown;

            // HACK: Pull the Adapter name from the current query.
            if (!entry->AdapterName)
            {
                entry->AdapterName = PhConvertMultiByteToUtf16(interfaceRow.bDescr);
            }
        }

        if (!entry->HaveFirstSample)
        {
            networkRcvSpeed = 0;
            networkXmitSpeed = 0;
            entry->HaveFirstSample = TRUE;
        }

        // We don't want incorrect data when the adapter is disabled.
        if (mediaState == MediaConnectStateUnknown)
        {
            networkRcvSpeed = 0;
            networkXmitSpeed = 0;
        }

        PhAddItemCircularBuffer_ULONG64(&entry->InboundBuffer, networkRcvSpeed);
        PhAddItemCircularBuffer_ULONG64(&entry->OutboundBuffer, networkXmitSpeed);

        //context->LinkSpeed = networkLinkSpeed;
        entry->InboundValue = networkRcvSpeed;
        entry->OutboundValue = networkXmitSpeed;
        entry->LastInboundValue = networkInOctets;
        entry->LastOutboundValue = networkOutOctets;
    }

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);
}