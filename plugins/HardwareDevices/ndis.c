/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
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

#include "devices.h"
#include <cguid.h>
#include <objbase.h>

PVOID IphlpHandle = NULL;
_GetInterfaceDescriptionFromGuid GetInterfaceDescriptionFromGuid_I = NULL;

BOOLEAN NetworkAdapterQuerySupported(
    _In_ HANDLE DeviceHandle
    )
{
    NTSTATUS status;
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    BOOLEAN ndisQuerySupported = FALSE;
    BOOLEAN adapterNameSupported = FALSE;
    BOOLEAN adapterStatsSupported = FALSE;
    BOOLEAN adapterLinkStateSupported = FALSE;
    BOOLEAN adapterLinkSpeedSupported = FALSE;
    PNDIS_OID objectIdBuffer;
    ULONG objectIdBufferLength;
    ULONG attempts = 0;

    opcode = OID_GEN_SUPPORTED_LIST;

    objectIdBufferLength = 2048 * sizeof(NDIS_OID);
    objectIdBuffer = PhAllocateZero(objectIdBufferLength);

    status = NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        objectIdBuffer,
        objectIdBufferLength
        );

    while (status == STATUS_BUFFER_OVERFLOW && attempts < 8)
    {
        PhFree(objectIdBuffer);
        objectIdBufferLength *= 2;
        objectIdBuffer = PhAllocateZero(objectIdBufferLength);

        status = NtDeviceIoControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            IOCTL_NDIS_QUERY_GLOBAL_STATS,
            &opcode,
            sizeof(NDIS_OID),
            objectIdBuffer,
            objectIdBufferLength
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
    {
        ndisQuerySupported = TRUE;

        for (ULONG i = 0; i < (ULONG)isb.Information / sizeof(NDIS_OID); i++)
        {
            NDIS_OID objectId = objectIdBuffer[i];

            switch (objectId)
            {
            case OID_GEN_FRIENDLY_NAME:
                adapterNameSupported = TRUE;
                break;
            case OID_GEN_STATISTICS:
                adapterStatsSupported = TRUE;
                break;
            case OID_GEN_LINK_STATE:
                adapterLinkStateSupported = TRUE;
                break;
            case OID_GEN_LINK_SPEED:
                adapterLinkSpeedSupported = TRUE;
                break;
            }
        }
    }

    PhFree(objectIdBuffer);

    if (!adapterNameSupported)
        ndisQuerySupported = FALSE;
    if (!adapterStatsSupported)
        ndisQuerySupported = FALSE;
    if (!adapterLinkStateSupported)
        ndisQuerySupported = FALSE;
    if (!adapterLinkSpeedSupported)
        ndisQuerySupported = FALSE;

    return ndisQuerySupported;
}

_Success_(return)
BOOLEAN NetworkAdapterQueryNdisVersion(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PUINT MajorVersion,
    _Out_opt_ PUINT MinorVersion
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    ULONG versionResult = 0;

    opcode = OID_GEN_DRIVER_VERSION; // OID_GEN_VENDOR_DRIVER_VERSION

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &versionResult,
        sizeof(versionResult)
        )))
    {
        if (MajorVersion)
        {
            *MajorVersion = HIBYTE(versionResult);
        }

        if (MinorVersion)
        {
            *MinorVersion = LOBYTE(versionResult);
        }

        return TRUE;
    }

    return FALSE;
}

PPH_STRING NetworkAdapterQueryName(
    _In_ HANDLE DeviceHandle,
    _In_ PPH_STRING InterfaceGuid
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    WCHAR adapterName[NDIS_IF_MAX_STRING_SIZE + 1] = L"";

    opcode = OID_GEN_FRIENDLY_NAME;

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        adapterName,
        sizeof(adapterName)
        )))
    {
        return PhCreateString(adapterName);
    }

    if (!GetInterfaceDescriptionFromGuid_I)
    {
        if (IphlpHandle = LoadLibrary(L"iphlpapi.dll"))
        {
            GetInterfaceDescriptionFromGuid_I = PhGetProcedureAddress(IphlpHandle, "NhGetInterfaceDescriptionFromGuid", 0);
        }
    }

    // HACK: Query adapter description using undocumented function.
    if (GetInterfaceDescriptionFromGuid_I)
    {
        GUID deviceGuid = GUID_NULL;
        UNICODE_STRING guidStringUs;

        PhStringRefToUnicodeString(&InterfaceGuid->sr, &guidStringUs);

        if (NT_SUCCESS(RtlGUIDFromString(&guidStringUs, &deviceGuid)))
        {
            WCHAR adapterDescription[NDIS_IF_MAX_STRING_SIZE + 1] = L"";
            SIZE_T adapterDescriptionLength = sizeof(adapterDescription);

            if (SUCCEEDED(GetInterfaceDescriptionFromGuid_I(&deviceGuid, adapterDescription, &adapterDescriptionLength, NULL, NULL)))
            {
                return PhCreateString(adapterDescription);
            }
        }
    }

    return PhCreateString(L"Unknown Network Adapter");
}

NTSTATUS NetworkAdapterQueryStatistics(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_STATISTICS_INFO Info
    )
{
    NTSTATUS status;
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    NDIS_STATISTICS_INFO result;

    opcode = OID_GEN_STATISTICS;

    memset(&result, 0, sizeof(NDIS_STATISTICS_INFO));
    result.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    result.Header.Revision = NDIS_STATISTICS_INFO_REVISION_1;
    result.Header.Size = NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;

    if (NT_SUCCESS(status = NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &result,
        sizeof(result)
        )))
    {
        *Info = result;
    }

    return status;
}

NTSTATUS NetworkAdapterQueryLinkState(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_LINK_STATE State
    )
{
    NTSTATUS status;
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    NDIS_LINK_STATE result;

    opcode = OID_GEN_LINK_STATE; // OID_GEN_MEDIA_CONNECT_STATUS;

    memset(&result, 0, sizeof(NDIS_LINK_STATE));
    result.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    result.Header.Revision = NDIS_LINK_STATE_REVISION_1;
    result.Header.Size = NDIS_SIZEOF_LINK_STATE_REVISION_1;

    if (NT_SUCCESS(status = NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &result,
        sizeof(result)
        )))
    {
        *State = result;
    }

    return status;
}

_Success_(return)
BOOLEAN NetworkAdapterQueryMediaType(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_PHYSICAL_MEDIUM Medium
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    NDIS_MEDIUM adapterType;
    NDIS_PHYSICAL_MEDIUM adapterMediaType;

    opcode = OID_GEN_PHYSICAL_MEDIUM_EX;
    adapterMediaType = NdisPhysicalMediumUnspecified;
    memset(&isb, 0, sizeof(IO_STATUS_BLOCK));

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &adapterMediaType,
        sizeof(adapterMediaType)
        )))
    {
        *Medium = adapterMediaType;
        return TRUE;
    }

    opcode = OID_GEN_PHYSICAL_MEDIUM;
    adapterMediaType = NdisPhysicalMediumUnspecified;
    memset(&isb, 0, sizeof(IO_STATUS_BLOCK));

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &adapterMediaType,
        sizeof(adapterMediaType)
        )))
    {
        *Medium = adapterMediaType;
        return TRUE;
    }

    opcode = OID_GEN_MEDIA_SUPPORTED; // OID_GEN_MEDIA_IN_USE
    adapterType = NdisMediumMax;
    memset(&isb, 0, sizeof(IO_STATUS_BLOCK));

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &adapterType,
        sizeof(adapterType)
        )))
    {
        switch (adapterType)
        {
        case NdisMedium802_3:
            *Medium = NdisPhysicalMedium802_3;
            break;
        case NdisMedium802_5:
            *Medium = NdisPhysicalMedium802_5;
            break;
        case NdisMediumWirelessWan:
            *Medium = NdisPhysicalMediumWirelessLan;
            break;
        case NdisMediumWiMAX:
            *Medium = NdisPhysicalMediumWiMax;
            break;
        default:
            *Medium = NdisPhysicalMediumOther;
            break;
        }

        return TRUE;
    }

    return FALSE;
}

NTSTATUS NetworkAdapterQueryLinkSpeed(
    _In_ HANDLE DeviceHandle,
    _Out_ PULONG64 LinkSpeed
    )
{
    NTSTATUS status;
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    NDIS_LINK_SPEED result;

    opcode = OID_GEN_LINK_SPEED;

    memset(&result, 0, sizeof(NDIS_LINK_SPEED));

    if (NT_SUCCESS(status = NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        &result,
        sizeof(result)
        )))
    {
        *LinkSpeed = UInt32x32To64(result.XmitLinkSpeed, NDIS_UNIT_OF_MEASUREMENT);
    }

    return status;
}

ULONG64 NetworkAdapterQueryValue(
    _In_ HANDLE DeviceHandle,
    _In_ NDIS_OID OpCode
    )
{
    IO_STATUS_BLOCK isb;
    ULONG64 result = 0;

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &OpCode,
        sizeof(NDIS_OID),
        &result,
        sizeof(result)
        )))
    {
        return result;
    }

    return 0;
}

_Success_(return)
BOOLEAN QueryInterfaceRow(
    _In_ PDV_NETADAPTER_ID Id,
    _Out_ PMIB_IF_ROW2 InterfaceRow
    )
{
    BOOLEAN result = FALSE;
    MIB_IF_ROW2 interfaceRow;

    memset(&interfaceRow, 0, sizeof(MIB_IF_ROW2));

    interfaceRow.InterfaceLuid = Id->InterfaceLuid;
    interfaceRow.InterfaceIndex = Id->InterfaceIndex;

    if (GetIfEntry2(&interfaceRow) == NO_ERROR)
    {
        result = TRUE;
        *InterfaceRow = interfaceRow;
    }

    //MIB_IPINTERFACE_ROW interfaceTable;
    //memset(&interfaceTable, 0, sizeof(MIB_IPINTERFACE_ROW));
    //interfaceTable.Family = AF_INET;
    //interfaceTable.InterfaceLuid.Value = Context->AdapterEntry->InterfaceLuidValue;
    //interfaceTable.InterfaceIndex = Context->AdapterEntry->InterfaceIndex;
    //GetIpInterfaceEntry(&interfaceTable);

    return result;
}

PWSTR MediumTypeToString(
    _In_ NDIS_PHYSICAL_MEDIUM MediumType
    )
{
    switch (MediumType)
    {
    case NdisPhysicalMediumWirelessLan:
        return L"Wireless LAN";
    case NdisPhysicalMediumCableModem:
        return L"Cable Modem";
    case NdisPhysicalMediumPhoneLine:
        return L"Phone Line";
    case NdisPhysicalMediumPowerLine:
        return L"Power Line";
    case NdisPhysicalMediumDSL:      // includes ADSL and UADSL (G.Lite)
        return L"DSL";
    case NdisPhysicalMediumFibreChannel:
        return L"Fibre";
    case NdisPhysicalMedium1394:
        return L"1394";
    case NdisPhysicalMediumWirelessWan:
        return L"Wireless WAN";
    case NdisPhysicalMediumNative802_11:
        return L"Native802_11";
    case NdisPhysicalMediumBluetooth:
        return L"Bluetooth";
    case NdisPhysicalMediumInfiniband:
        return L"Infiniband";
    case NdisPhysicalMediumWiMax:
        return L"WiMax";
    case NdisPhysicalMediumUWB:
        return L"UWB";
    case NdisPhysicalMedium802_3:
        return L"Ethernet";
    case NdisPhysicalMedium802_5:
        return L"802_5";
    case NdisPhysicalMediumIrda:
        return L"Infrared";
    case NdisPhysicalMediumWiredWAN:
        return L"Wired WAN";
    case NdisPhysicalMediumWiredCoWan:
        return L"Wired CoWan";
    case NdisPhysicalMediumOther:
        return L"Other";
    case NdisPhysicalMediumNative802_15_4:
        return L"Native802_15_";
    }

    return L"N/A";
}

//BOOLEAN NetworkAdapterQueryInternet(
//    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context,
//    _In_ PPH_STRING IpAddress
//    )
//{
//    // https://technet.microsoft.com/en-us/library/cc766017.aspx
//    BOOLEAN socketResult = FALSE;
//    DNS_STATUS dnsQueryStatus = DNS_ERROR_RCODE_NO_ERROR;
//    PDNS_RECORD dnsQueryRecords = NULL;
//
//    __try
//    {
//        if ((dnsQueryStatus = DnsQuery(
//            L"www.msftncsi.com",
//            DNS_TYPE_A,
//            DNS_QUERY_NO_HOSTS_FILE | DNS_QUERY_BYPASS_CACHE,
//            NULL,
//            &dnsQueryRecords,
//            NULL
//            )) != DNS_ERROR_RCODE_NO_ERROR)
//        {
//            __leave;
//        }
//
//        for (PDNS_RECORD i = dnsQueryRecords; i != NULL; i = i->pNext)
//        {
//            if (i->wType == DNS_TYPE_A)
//            {
//                SOCKET socketHandle = INVALID_SOCKET;
//
//                if ((socketHandle = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0)) == INVALID_SOCKET)
//                    continue;
//
//                IN_ADDR sockAddr;
//                InetPton(AF_INET, IpAddress->Buffer, &sockAddr);
//
//                SOCKADDR_IN localaddr = { 0 };
//                localaddr.sin_family = AF_INET;
//                localaddr.sin_addr.s_addr = sockAddr.s_addr;
//
//                if (bind(socketHandle, (PSOCKADDR)&localaddr, sizeof(localaddr)) == SOCKET_ERROR)
//                {
//                    closesocket(socketHandle);
//                    continue;
//                }
//
//                SOCKADDR_IN remoteAddr;
//                remoteAddr.sin_family = AF_INET;
//                remoteAddr.sin_port = htons(80);
//                remoteAddr.sin_addr.s_addr = i->Data.A.IpAddress;
//
//                if (WSAConnect(socketHandle, (PSOCKADDR)&remoteAddr, sizeof(remoteAddr), NULL, NULL, NULL, NULL) != SOCKET_ERROR)
//                {
//                    socketResult = TRUE;
//                    closesocket(socketHandle);
//                    break;
//                }
//
//                closesocket(socketHandle);
//            }
//        }
//    }
//    __finally
//    {
//        if (dnsQueryRecords)
//        {
//            DnsFree(dnsQueryRecords, DnsFreeRecordList);
//        }
//    }
//
//    __try
//    {
//        if ((dnsQueryStatus = DnsQuery(
//            L"ipv6.msftncsi.com",
//            DNS_TYPE_AAAA,
//            DNS_QUERY_NO_HOSTS_FILE | DNS_QUERY_BYPASS_CACHE, //  | DNS_QUERY_DUAL_ADDR
//            NULL,
//            &dnsQueryRecords,
//            NULL
//            )) != DNS_ERROR_RCODE_NO_ERROR)
//        {
//            __leave;
//        }
//
//        for (PDNS_RECORD i = dnsQueryRecords; i != NULL; i = i->pNext)
//        {
//            if (i->wType == DNS_TYPE_AAAA)
//            {
//                SOCKET socketHandle = INVALID_SOCKET;
//
//                if ((socketHandle = WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0)) == INVALID_SOCKET)
//                    continue;
//
//                IN6_ADDR sockAddr;
//                InetPton(AF_INET6, IpAddress->Buffer, &sockAddr);
//
//                SOCKADDR_IN6 remoteAddr = { 0 };
//                remoteAddr.sin6_family = AF_INET6;
//                remoteAddr.sin6_port = htons(80);
//                memcpy(&remoteAddr.sin6_addr.u.Byte, i->Data.AAAA.Ip6Address.IP6Byte, sizeof(i->Data.AAAA.Ip6Address.IP6Byte));
//
//                if (WSAConnect(socketHandle, (PSOCKADDR)&remoteAddr, sizeof(SOCKADDR_IN6), NULL, NULL, NULL, NULL) != SOCKET_ERROR)
//                {
//                    socketResult = TRUE;
//                    closesocket(socketHandle);
//                    break;
//                }
//
//                closesocket(socketHandle);
//            }
//        }
//    }
//    __finally
//    {
//        if (dnsQueryRecords)
//        {
//            DnsFree(dnsQueryRecords, DnsFreeRecordList);
//        }
//    }
//
//    WSACleanup();
//
//    return socketResult;
//}
