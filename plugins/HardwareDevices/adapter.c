/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2015-2022
 *
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

    DeleteNetAdapterId(&entry->AdapterId);
    PhClearReference(&entry->AdapterName);
    PhClearReference(&entry->AdapterAlias);

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

        entry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!entry)
            continue;

        if (NetAdapterEnableNdis)
        {
            if (NT_SUCCESS(PhCreateFile(
                &deviceHandle,
                entry->AdapterId.InterfacePath,
                FILE_GENERIC_READ,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                )))
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

            memset(&interfaceStats, 0, sizeof(NDIS_STATISTICS_INFO));

            if (NT_SUCCESS(NetworkAdapterQueryStatistics(deviceHandle, &interfaceStats)))
            {
                if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV))
                    networkInOctets = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_RCV);
                else
                    networkInOctets = interfaceStats.ifHCInOctets;

                if (!(interfaceStats.SupportedStatistics & NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT))
                    networkOutOctets = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_XMIT);
                else
                    networkOutOctets = interfaceStats.ifHCOutOctets;
            }
            else
            {
                networkInOctets = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_RCV);
                networkOutOctets = NetworkAdapterQueryValue(deviceHandle, OID_GEN_BYTES_XMIT);
            }

            NDIS_LINK_STATE interfaceState;

            if (NT_SUCCESS(NetworkAdapterQueryLinkState(deviceHandle, &interfaceState)))
            {
                entry->DevicePresent = interfaceState.MediaConnectState == MediaConnectStateConnected;
            }
            else
            {
                entry->DevicePresent = FALSE;
            }

            NtClose(deviceHandle);
        }
        else
        {
            MIB_IF_ROW2 interfaceRow;

            if (NetworkAdapterQueryInterfaceRow(&entry->AdapterId, MibIfEntryNormal, &interfaceRow))
            {
                networkInOctets = interfaceRow.InOctets;
                networkOutOctets = interfaceRow.OutOctets;

                // HACK: Pull the Adapter name from the current query. (dmex)
                if (!entry->AdapterName && PhCountStringZ(interfaceRow.Description) > 0)
                    entry->AdapterName = PhCreateString(interfaceRow.Description);

                // HACK: Hide the device when it's not operational. (dmex)
                if (interfaceRow.OperStatus == IfOperStatusUp)
                    entry->DevicePresent = TRUE;
                else
                    entry->DevicePresent = FALSE;
            }
            else
            {
                entry->DevicePresent = FALSE;
            }
        }

        if (!entry->DevicePresent)
        {
            entry->NetworkReceiveRaw = 0;
            entry->NetworkSendRaw = 0;
            PhInitializeDelta(&entry->NetworkSendDelta);
            PhInitializeDelta(&entry->NetworkReceiveDelta);
        }
        else
        {
            //if (entry->NetworkReceiveRaw < networkInOctets)
            entry->NetworkReceiveRaw = networkInOctets;
            //if (entry->NetworkSendRaw < networkOutOctets)
            entry->NetworkSendRaw = networkOutOctets;

            PhUpdateDelta(&entry->NetworkSendDelta, entry->NetworkSendRaw);
            PhUpdateDelta(&entry->NetworkReceiveDelta, entry->NetworkReceiveRaw);
        }

        if (!entry->HaveFirstSample)
        {
            entry->NetworkSendDelta.Delta = 0;
            entry->NetworkReceiveDelta.Delta = 0;
            entry->HaveFirstSample = TRUE;
        }

        if (runCount != 0)
        {
            entry->CurrentNetworkSend = entry->NetworkSendDelta.Delta;
            entry->CurrentNetworkReceive = entry->NetworkReceiveDelta.Delta;

            PhAddItemCircularBuffer_ULONG64(&entry->OutboundBuffer, entry->CurrentNetworkSend);
            PhAddItemCircularBuffer_ULONG64(&entry->InboundBuffer, entry->CurrentNetworkReceive);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);

    runCount++;
}

VOID NetAdapterUpdateDeviceInfo(
    _In_opt_ HANDLE DeviceHandle,
    _In_ PDV_NETADAPTER_ENTRY AdapterEntry
    )
{
    if (PhIsNullOrEmptyString(AdapterEntry->AdapterAlias))
    {
        AdapterEntry->AdapterAlias = NetworkAdapterGetInterfaceAliasFromLuid(&AdapterEntry->AdapterId);
    }

    if (PhIsNullOrEmptyString(AdapterEntry->AdapterName))
    {
        HANDLE deviceHandle = NULL;

        if (DeviceHandle)
        {
            deviceHandle = DeviceHandle;
        }
        else
        {
            if (NetAdapterEnableNdis)
            {
                if (NT_SUCCESS(PhCreateFile(
                    &deviceHandle,
                    AdapterEntry->AdapterId.InterfacePath,
                    FILE_GENERIC_READ,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                    )))
                {
                    if (!AdapterEntry->CheckedDeviceSupport)
                    {
                        // Check the network adapter supports the OIDs we're going to be using.
                        if (NetworkAdapterQuerySupported(deviceHandle))
                        {
                            AdapterEntry->DeviceSupported = TRUE;
                        }

                        AdapterEntry->CheckedDeviceSupport = TRUE;
                    }

                    if (!AdapterEntry->DeviceSupported)
                    {
                        // Device is faulty. Close the handle so we can fallback to GetIfEntry.
                        NtClose(deviceHandle);
                        deviceHandle = NULL;
                    }
                }
            }
        }

        if (deviceHandle)
        {
            if (PhIsNullOrEmptyString(AdapterEntry->AdapterName))
            {
                AdapterEntry->AdapterName = NetworkAdapterQueryName(deviceHandle);
            }

            if (PhIsNullOrEmptyString(AdapterEntry->AdapterName))
            {
                AdapterEntry->AdapterName = NetworkAdapterQueryNameFromInterfaceGuid(AdapterEntry->AdapterId.InterfaceGuid);
            }

            if (!DeviceHandle && deviceHandle)
            {
                NtClose(deviceHandle);
            }
        }
        else
        {
            MIB_IF_ROW2 interfaceRow;

            if (PhIsNullOrEmptyString(AdapterEntry->AdapterName))
            {
                AdapterEntry->AdapterName = NetworkAdapterQueryNameFromInterfaceGuid(AdapterEntry->AdapterId.InterfaceGuid);
            }

            if (PhIsNullOrEmptyString(AdapterEntry->AdapterName))
            {
                if (NetworkAdapterQueryInterfaceRow(&AdapterEntry->AdapterId, MibIfEntryNormalWithoutStatistics, &interfaceRow))
                {
                    if (PhIsNullOrEmptyString(AdapterEntry->AdapterName) && PhCountStringZ(interfaceRow.Description) > 0)
                    {
                        AdapterEntry->AdapterName = PhCreateString(interfaceRow.Description);
                    }
                }
            }
        }
    }
}

VOID InitializeNetAdapterId(
    _Out_ PDV_NETADAPTER_ID Id,
    _In_ NET_IFINDEX InterfaceIndex,
    _In_ IF_LUID InterfaceLuid,
    _In_ PPH_STRING InterfaceGuid
    )
{
    static PH_STRINGREF nativeNamespacePath = PH_STRINGREF_INIT(L"\\??\\");

    Id->InterfaceIndex = InterfaceIndex;
    Id->InterfaceLuid = InterfaceLuid;
    PhSetReference(&Id->InterfaceGuid, InterfaceGuid);
    Id->InterfacePath = PhConcatStringRef2(&nativeNamespacePath, &InterfaceGuid->sr);
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
    PhClearReference(&Id->InterfacePath);
}

BOOLEAN EquivalentNetAdapterId(
    _In_ PDV_NETADAPTER_ID Id1,
    _In_ PDV_NETADAPTER_ID Id2
    )
{
    if (Id1->InterfaceLuid.Value == Id2->InterfaceLuid.Value)
        return TRUE;

    return FALSE;
}

PDV_NETADAPTER_ENTRY CreateNetAdapterEntry(
    _In_ PDV_NETADAPTER_ID Id
    )
{
    PDV_NETADAPTER_ENTRY entry;

    entry = PhCreateObject(sizeof(DV_NETADAPTER_ENTRY), NetAdapterEntryType);
    memset(entry, 0, sizeof(DV_NETADAPTER_ENTRY));

    CopyNetAdapterId(&entry->AdapterId, Id);

    PhInitializeCircularBuffer_ULONG64(&entry->InboundBuffer, PhGetIntegerSetting(L"SampleCount"));
    PhInitializeCircularBuffer_ULONG64(&entry->OutboundBuffer, PhGetIntegerSetting(L"SampleCount"));

    PhAcquireQueuedLockExclusive(&NetworkAdaptersListLock);
    PhAddItemList(NetworkAdaptersList, entry);
    PhReleaseQueuedLockExclusive(&NetworkAdaptersListLock);

    return entry;
}
