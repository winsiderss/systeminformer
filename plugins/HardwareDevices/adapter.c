/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2015-2016 dmex
 * Copyright (C) 2016 wj32
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

#include "devices.h"

VOID AdapterEntryDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PDV_NETADAPTER_ENTRY entry = Object;

    PhAcquireQueuedLockExclusive(&NetworkAdaptersListLock);
    PhRemoveItemList(NetworkAdaptersList, PhFindItemList(NetworkAdaptersList, entry));
    PhReleaseQueuedLockExclusive(&NetworkAdaptersListLock);

    DeleteNetAdapterId(&entry->Id);
    PhClearReference(&entry->AdapterName);

    PhDeleteCircularBuffer_ULONG64(&entry->InboundBuffer);
    PhDeleteCircularBuffer_ULONG64(&entry->OutboundBuffer);
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
    static ULONG runCount = 0; // MUST keep in sync with runCount in process provider

    PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        HANDLE deviceHandle = NULL;
        PDV_NETADAPTER_ENTRY entry;
        ULONG64 networkInOctets = 0;
        ULONG64 networkOutOctets = 0;
        ULONG64 networkRcvSpeed = 0;
        ULONG64 networkXmitSpeed = 0;
        NDIS_MEDIA_CONNECT_STATE mediaState = MediaConnectStateUnknown;

        entry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!entry)
            continue;

        if (PhGetIntegerSetting(SETTING_NAME_ENABLE_NDIS))
        {
            if (NT_SUCCESS(NetworkAdapterCreateHandle(&deviceHandle, entry->Id.InterfaceGuid)))
            {
                if (!entry->CheckedDeviceSupport)
                {
                    // Check the network adapter supports the OIDs we're going to be using.
                    if (NetworkAdapterQuerySupported(deviceHandle))
                    {
                        entry->DeviceSupported = TRUE;
                    }

                    entry->CheckedDeviceSupport = TRUE;
                }

                if (!entry->DeviceSupported)
                {
                    // Device is faulty. Close the handle so we can fallback to GetIfEntry.
                    NtClose(deviceHandle);
                    deviceHandle = NULL;
                }
            }
        }

        if (deviceHandle)
        {
            NDIS_STATISTICS_INFO interfaceStats;
            NDIS_LINK_STATE interfaceState;

            memset(&interfaceStats, 0, sizeof(NDIS_STATISTICS_INFO));

            NetworkAdapterQueryStatistics(deviceHandle, &interfaceStats);

            if (NT_SUCCESS(NetworkAdapterQueryLinkState(deviceHandle, &interfaceState)))
            {
                mediaState = interfaceState.MediaConnectState;
            }

            if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV))
                networkInOctets = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_RCV);
            else
                networkInOctets = interfaceStats.ifHCInOctets;

            if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT))
                networkOutOctets = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_XMIT);
            else
                networkOutOctets = interfaceStats.ifHCOutOctets;

            networkRcvSpeed = networkInOctets - entry->LastInboundValue;
            networkXmitSpeed = networkOutOctets - entry->LastOutboundValue;

            // HACK: Pull the Adapter name from the current query.
            if (!entry->AdapterName)
            {
                entry->AdapterName = NetworkAdapterQueryName(deviceHandle, entry->Id.InterfaceGuid);
            }

            entry->DevicePresent = TRUE;

            NtClose(deviceHandle);
        }
        else if (WindowsVersion >= WINDOWS_VISTA && GetIfEntry2)
        {
            MIB_IF_ROW2 interfaceRow;

            if (QueryInterfaceRowVista(&entry->Id, &interfaceRow))
            {
                networkInOctets = interfaceRow.InOctets;
                networkOutOctets = interfaceRow.OutOctets;
                mediaState = interfaceRow.MediaConnectState;
                networkRcvSpeed = networkInOctets - entry->LastInboundValue;
                networkXmitSpeed = networkOutOctets - entry->LastOutboundValue;

                // HACK: Pull the Adapter name from the current query.
                if (!entry->AdapterName && PhCountStringZ(interfaceRow.Description) > 0)
                {
                    entry->AdapterName = PhCreateString(interfaceRow.Description);
                }

                entry->DevicePresent = TRUE;
            }
            else
            {
                entry->DevicePresent = FALSE;
            }
        }
        else
        {
            MIB_IFROW interfaceRow;

            if (QueryInterfaceRowXP(&entry->Id, &interfaceRow))
            {
                networkInOctets = interfaceRow.dwInOctets;
                networkOutOctets = interfaceRow.dwOutOctets;
                mediaState = interfaceRow.dwOperStatus == IF_OPER_STATUS_OPERATIONAL ? MediaConnectStateConnected : MediaConnectStateDisconnected;
                networkRcvSpeed = networkInOctets - entry->LastInboundValue;
                networkXmitSpeed = networkOutOctets - entry->LastOutboundValue;

                // HACK: Pull the Adapter name from the current query.
                if (!entry->AdapterName && strlen(interfaceRow.bDescr) > 0)
                {
                    entry->AdapterName = PhConvertMultiByteToUtf16(interfaceRow.bDescr);
                }

                entry->DevicePresent = TRUE;
            }
            else
            {
                entry->DevicePresent = FALSE;
            }
        }

        if (mediaState == MediaConnectStateUnknown)
        {
            // We don't want incorrect data when the adapter is disabled.
            networkRcvSpeed = 0;
            networkXmitSpeed = 0;
        }

        if (!entry->HaveFirstSample)
        {
            // The first sample must be zero.
            networkRcvSpeed = 0;
            networkXmitSpeed = 0;
            entry->HaveFirstSample = TRUE;
        }

        if (runCount != 0)
        {
            PhAddItemCircularBuffer_ULONG64(&entry->InboundBuffer, networkRcvSpeed);
            PhAddItemCircularBuffer_ULONG64(&entry->OutboundBuffer, networkXmitSpeed);
        }

        //context->LinkSpeed = networkLinkSpeed;
        entry->InboundValue = networkRcvSpeed;
        entry->OutboundValue = networkXmitSpeed;
        entry->LastInboundValue = networkInOctets;
        entry->LastOutboundValue = networkOutOctets;

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);

    runCount++;
}

VOID InitializeNetAdapterId(
    _Out_ PDV_NETADAPTER_ID Id,
    _In_ NET_IFINDEX InterfaceIndex,
    _In_ IF_LUID InterfaceLuid,
    _In_ PPH_STRING InterfaceGuid
    )
{
    Id->InterfaceIndex = InterfaceIndex;
    Id->InterfaceLuid = InterfaceLuid;
    PhSetReference(&Id->InterfaceGuid, InterfaceGuid);
}

VOID CopyNetAdapterId(
    _Out_ PDV_NETADAPTER_ID Destination,
    _In_ PDV_NETADAPTER_ID Source
    )
{
    InitializeNetAdapterId(
        Destination,
        Source->InterfaceIndex,
        Source->InterfaceLuid,
        Source->InterfaceGuid
        );
}

VOID DeleteNetAdapterId(
    _Inout_ PDV_NETADAPTER_ID Id
    )
{
    PhClearReference(&Id->InterfaceGuid);
}

BOOLEAN EquivalentNetAdapterId(
    _In_ PDV_NETADAPTER_ID Id1,
    _In_ PDV_NETADAPTER_ID Id2
    )
{
    if (WindowsVersion >= WINDOWS_VISTA)
    {
        if (Id1->InterfaceLuid.Value == Id2->InterfaceLuid.Value)
            return TRUE;
    }
    else
    {
        if (Id1->InterfaceIndex == Id2->InterfaceIndex)
            return TRUE;
    }

    return FALSE;
}

PDV_NETADAPTER_ENTRY CreateNetAdapterEntry(
    _In_ PDV_NETADAPTER_ID Id
    )
{
    PDV_NETADAPTER_ENTRY entry;

    entry = PhCreateObject(sizeof(DV_NETADAPTER_ENTRY), NetAdapterEntryType);
    memset(entry, 0, sizeof(DV_NETADAPTER_ENTRY));

    CopyNetAdapterId(&entry->Id, Id);

    PhInitializeCircularBuffer_ULONG64(&entry->InboundBuffer, PhGetIntegerSetting(L"SampleCount"));
    PhInitializeCircularBuffer_ULONG64(&entry->OutboundBuffer, PhGetIntegerSetting(L"SampleCount"));

    PhAcquireQueuedLockExclusive(&NetworkAdaptersListLock);
    PhAddItemList(NetworkAdaptersList, entry);
    PhReleaseQueuedLockExclusive(&NetworkAdaptersListLock);

    return entry;
}
