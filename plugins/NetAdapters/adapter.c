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

    if (Entry->InterfaceGuid)
        PhDereferenceObject(Entry->InterfaceGuid);
}

VOID NetAdaptersInitialize(
    VOID
    )
{
    NetworkAdaptersList = PhCreateList(1);
    PhAdapterItemType = PhCreateObjectType(L"NetAdapterItem", 0, AdapterEntryDeleteProcedure);
}

VOID NetAdaptersUpdate(
    VOID
    )
{
    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        PPH_NETADAPTER_ENTRY entry = (PPH_NETADAPTER_ENTRY)PhReferenceObject(NetworkAdaptersList->Items[i]);

        ULONG64 networkInOctets = 0;
        ULONG64 networkOutOctets = 0;
        ULONG64 networkRcvSpeed = 0;
        ULONG64 networkXmitSpeed = 0;
        //ULONG64 networkLinkSpeed = 0;
        HANDLE adapterHandle = NULL;

        if (PhGetIntegerSetting(SETTING_NAME_ENABLE_NDIS))
        {
            // Create the handle to the network device
            PhCreateFileWin32(
                &adapterHandle,
                PhaFormatString(L"\\\\.\\%s", entry->InterfaceGuid->Buffer)->Buffer,
                FILE_GENERIC_READ,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );

            if (adapterHandle)
            {
                // Check the network adapter supports the OIDs we're going to be using.
                if (!NetworkAdapterQuerySupported(adapterHandle))
                {
                    // Device is faulty. Close the handle so we can fallback to GetIfEntry.
                    NtClose(adapterHandle);
                    adapterHandle = NULL;
                }
            }
        }


        if (adapterHandle)
        {
            NDIS_STATISTICS_INFO interfaceStats;
            //NDIS_LINK_STATE interfaceState;

            if (NT_SUCCESS(NetworkAdapterQueryStatistics(adapterHandle, &interfaceStats)))
            {
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
            }
            else
            {
                networkInOctets = NetworkAdapterQueryValue(adapterHandle, OID_GEN_BYTES_RCV);
                networkOutOctets = NetworkAdapterQueryValue(adapterHandle, OID_GEN_BYTES_XMIT);

                networkRcvSpeed = networkInOctets - entry->LastInboundValue;
                networkXmitSpeed = networkOutOctets - entry->LastOutboundValue;
            }

            //if (NT_SUCCESS(NetworkAdapterQueryLinkState(context->DeviceHandle, &interfaceState)))
            //{
            //    networkLinkSpeed = interfaceState.XmitLinkSpeed;
            //}
            //else
            //{
            //    NetworkAdapterQueryLinkSpeed(context->DeviceHandle, &networkLinkSpeed);
            //}

            // HACK: Pull the Adapter name from the current query.
            if (!entry->AdapterName)
            {
                entry->AdapterName = NetworkAdapterQueryName(adapterHandle, entry->InterfaceGuid);
            }
        }
        else if (GetIfEntry2_I)
        {
            MIB_IF_ROW2 interfaceRow;

            interfaceRow = QueryInterfaceRowVista(entry);

            networkInOctets = interfaceRow.InOctets;
            networkOutOctets = interfaceRow.OutOctets;
            networkRcvSpeed = networkInOctets - entry->LastInboundValue;
            networkXmitSpeed = networkOutOctets - entry->LastOutboundValue;
            //networkLinkSpeed = interfaceRow.TransmitLinkSpeed; // interfaceRow.ReceiveLinkSpeed

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
            //networkLinkSpeed = interfaceRow.dwSpeed;

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

        PhAddItemCircularBuffer_ULONG64(&entry->InboundBuffer, networkRcvSpeed);
        PhAddItemCircularBuffer_ULONG64(&entry->OutboundBuffer, networkXmitSpeed);

        //context->LinkSpeed = networkLinkSpeed;
        entry->InboundValue = networkRcvSpeed;
        entry->OutboundValue = networkXmitSpeed;
        entry->LastInboundValue = networkInOctets;
        entry->LastOutboundValue = networkOutOctets;

        NtClose(adapterHandle);
        PhDereferenceObject(entry);
    }
}