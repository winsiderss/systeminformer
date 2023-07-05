/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022
 *
 */

#include "devices.h"
#include "ndiswlan.h"

typedef struct _DV_NETADAPTER_WLAN_CONFIG
{
    ULONG BeaconInterval;
    ULONGLONG Age;
    ULONGLONG Timestamp;
    ULONGLONG HostTimestamp;
    ULONG ConnectedStations;
    ULONG ChannelUtilization;
    ULONG ChannelUtilizationPercent;
} DV_NETADAPTER_WLAN_CONFIG, *PDV_NETADAPTER_WLAN_CONFIG;

_Success_(return)
BOOLEAN NetworkAdapterQueryWlanConfig(
    _In_ PGUID InterfaceGuid
    )
{
    HANDLE clientHandle;
    ULONG version = 0;
    ULONG status = 0;
    ULONG connectionAttributesLength = 0;
    PWLAN_CONNECTION_ATTRIBUTES connectionAttributes;
    PWLAN_BSS_LIST connectionList;

    status = WlanOpenHandle(
        WLAN_API_VERSION_2_0,
        NULL,
        &version,
        &clientHandle
        );

    if (status != ERROR_SUCCESS)
        return FALSE;

    status = WlanQueryInterface(
        clientHandle,
        InterfaceGuid,
        wlan_intf_opcode_current_connection,
        NULL,
        &connectionAttributesLength,
        &connectionAttributes,
        NULL
        );

    if (status != ERROR_SUCCESS)
        goto CleanupExit;

    status = WlanGetNetworkBssList(
        clientHandle,
        InterfaceGuid,
        &connectionAttributes->AssociationAttributes.Ssid,
        connectionAttributes->AssociationAttributes.BssType,
        connectionAttributes->SecurityAttributes.SecurityEnabled,
        NULL,
        &connectionList
        );

    WlanFreeMemory(connectionAttributes);

    if (status != ERROR_SUCCESS)
        goto CleanupExit;

    for (ULONG i = 0; i < connectionList->NumberOfItems; i++)
    {
        WLAN_BSS_ENTRY entry = connectionList->BssEntries[i];
        PWLAN_EXTENDED_HEADER extended = PTR_ADD_OFFSET(connectionList->BssEntries, entry.IeOffset);
        PVOID extendedLength = PTR_ADD_OFFSET(extended, entry.IeSize);
        LARGE_INTEGER systemTime;

        PhQuerySystemTime(&systemTime);

        while ((ULONG_PTR)extended < (ULONG_PTR)extendedLength)
        {
            if (extended->Id == WLAN_EXTENDED_ID_BSS_LOAD)
            {
                PWLAN_EXTENDED_BSS_LOAD load = PTR_ADD_OFFSET(extended, sizeof(WLAN_EXTENDED_HEADER));
                ULONG connectedStations = load->ConnectedStationsHigh << 8 | load->ConnectedStationsLow;
                ULONG channelUtilization = load->ChannelUtilization;
                //ULONG connectedStations = ((PBYTE)extended)[3] << 8 | ((PBYTE)extended)[2];
                //ULONG channelUtilization = ((PBYTE)extended)[4];
                //ULONG mediumCapacity = ((PBYTE)extended)[5];
                PDV_NETADAPTER_WLAN_CONFIG config;

                config = PhAllocateZero(sizeof(DV_NETADAPTER_WLAN_CONFIG));
                config->Age = systemTime.QuadPart - entry.HostTimestamp;
                config->BeaconInterval = entry.BeaconPeriod * WLAN_TIMESTAMP_TIME_UNIT;
                config->Timestamp = entry.Timestamp;
                config->HostTimestamp = entry.HostTimestamp;
                config->ChannelUtilization = channelUtilization;
                config->ChannelUtilizationPercent = channelUtilization * 100 / 255;

                //ULONGLONG value;
                //ULONGLONG days = entry.Timestamp / WLAN_TIMESTAMP_DAY;
                //value = entry.Timestamp % WLAN_TIMESTAMP_DAY;
                //ULONGLONG hours = value / WLAN_TIMESTAMP_HOUR;
                //value %= WLAN_TIMESTAMP_HOUR;
                //ULONGLONG mins = value / WLAN_TIMESTAMP_MIN;
                //value %= WLAN_TIMESTAMP_MIN;
                //ULONGLONG secs = value / WLAN_TIMESTAMP_SEC;
                //dprintf("Uptime: %u days, %u hours, %u mins, %u seconds\n", days, hours, mins, secs);
            }

            extended = PTR_ADD_OFFSET(extended, (extended->Length & USHRT_MAX) + sizeof(WLAN_EXTENDED_HEADER));
        }
    }

    WlanFreeMemory(connectionList);

CleanupExit:
    WlanCloseHandle(clientHandle, NULL);

    return TRUE;
}
