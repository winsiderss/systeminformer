/*
 * Process Hacker Extra Plugins -
 *   Network Adapters Plugin
 *
 * Copyright (C) 2015 dmex
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

PVOID IphlpHandle = NULL;
_GetIfEntry2 GetIfEntry2_I = NULL;
_GetInterfaceDescriptionFromGuid GetInterfaceDescriptionFromGuid_I = NULL;

BOOLEAN NetworkAdapterQuerySupported(
    _In_ HANDLE DeviceHandle
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    BOOLEAN ndisQuerySupported = FALSE;
    BOOLEAN adapterNameSupported = FALSE;
    BOOLEAN adapterStatsSupported = FALSE;
    BOOLEAN adapterLinkStateSupported = FALSE;
    BOOLEAN adapterLinkSpeedSupported = FALSE;
    PNDIS_OID ndisObjectIdentifiers = NULL;

    // https://msdn.microsoft.com/en-us/library/ff569642.aspx
    opcode = OID_GEN_SUPPORTED_LIST;

    // TODO: 4096 objects might be too small...
    ndisObjectIdentifiers = PhAllocate(PAGE_SIZE * sizeof(NDIS_OID));
    memset(ndisObjectIdentifiers, 0, PAGE_SIZE * sizeof(NDIS_OID));

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS, // https://msdn.microsoft.com/en-us/library/windows/hardware/ff548975.aspx
        &opcode,
        sizeof(NDIS_OID),
        ndisObjectIdentifiers,
        PAGE_SIZE * sizeof(NDIS_OID)
        )))
    {
        ndisQuerySupported = TRUE;

        for (ULONG i = 0; i < (ULONG)isb.Information; i++)
        {
            NDIS_OID opcode = ndisObjectIdentifiers[i];

            switch (opcode)
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

    PhFree(ndisObjectIdentifiers);

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

BOOLEAN NetworkAdapterQueryNdisVersion(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PUINT MajorVersion,
    _Out_opt_ PUINT MinorVersion
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    ULONG versionResult = 0;

    // https://msdn.microsoft.com/en-us/library/ff569582.aspx
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
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    WCHAR adapterNameBuffer[MAX_PATH] = L"";

    // https://msdn.microsoft.com/en-us/library/ff569584.aspx
    opcode = OID_GEN_FRIENDLY_NAME;

    if (NT_SUCCESS(NtDeviceIoControlFile(
        Context->DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &opcode,
        sizeof(NDIS_OID),
        adapterNameBuffer,
        sizeof(adapterNameBuffer)
        )))
    {
        return PhCreateString(adapterNameBuffer);
    }

    // HACK: Query adapter description using undocumented function.
    if (GetInterfaceDescriptionFromGuid_I)
    {
        GUID deviceGuid = GUID_NULL;
        UNICODE_STRING guidStringUs;

        PhStringRefToUnicodeString(&Context->AdapterEntry->InterfaceGuid->sr, &guidStringUs);

        if (NT_SUCCESS(RtlGUIDFromString(&guidStringUs, &deviceGuid)))
        {
            SIZE_T adapterDescriptionLength = 0;
            WCHAR adapterDescription[NDIS_IF_MAX_STRING_SIZE + 1] = L"";

            adapterDescriptionLength = sizeof(adapterDescription);

            if (SUCCEEDED(GetInterfaceDescriptionFromGuid_I(&deviceGuid, adapterDescription, &adapterDescriptionLength, NULL, NULL)))
            {
                return PhCreateString(adapterDescription);
            }
        }
    }

    return PhCreateString(L"Unknown");
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

    // https://msdn.microsoft.com/en-us/library/ff569640.aspx
    opcode = OID_GEN_STATISTICS;

    memset(&result, 0, sizeof(NDIS_STATISTICS_INFO));
    result.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    result.Header.Revision = NDIS_STATISTICS_INFO_REVISION_1;
    result.Header.Size = NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;

    status = NtDeviceIoControlFile(
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
        );

    *Info = result;

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

    // https://msdn.microsoft.com/en-us/library/ff569595.aspx
    opcode = OID_GEN_LINK_STATE; // OID_GEN_MEDIA_CONNECT_STATUS;

    memset(&result, 0, sizeof(NDIS_LINK_STATE));
    result.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    result.Header.Revision = NDIS_LINK_STATE_REVISION_1;
    result.Header.Size = NDIS_SIZEOF_LINK_STATE_REVISION_1;

    status = NtDeviceIoControlFile(
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
        );

    *State = result;

    return status;
}

BOOLEAN NetworkAdapterQueryMediaType(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_PHYSICAL_MEDIUM Medium
    )
{
    NDIS_OID opcode;
    IO_STATUS_BLOCK isb;
    NDIS_PHYSICAL_MEDIUM adapterMediaType = NdisPhysicalMediumUnspecified;

    // https://msdn.microsoft.com/en-us/library/ff569622.aspx
    opcode = OID_GEN_PHYSICAL_MEDIUM_EX;

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
    }

    if (adapterMediaType != NdisPhysicalMediumUnspecified)
        return TRUE;

    // https://msdn.microsoft.com/en-us/library/ff569621.aspx
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
    }

    if (adapterMediaType != NdisPhysicalMediumUnspecified)
        return TRUE;

    //NDIS_MEDIUM adapterMediaType = NdisMediumMax;
    //opcode = OID_GEN_MEDIA_IN_USE;

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

    // https://msdn.microsoft.com/en-us/library/ff569593.aspx
    opcode = OID_GEN_LINK_SPEED;

    memset(&result, 0, sizeof(NDIS_LINK_SPEED));

    status = NtDeviceIoControlFile(
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
        );

    *LinkSpeed = UInt32x32To64(result.XmitLinkSpeed, NDIS_UNIT_OF_MEASUREMENT);

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

MIB_IF_ROW2 QueryInterfaceRowVista(
    _In_ PPH_NETADAPTER_ENTRY AdapterEntry
    )
{
    MIB_IF_ROW2 interfaceRow;

    memset(&interfaceRow, 0, sizeof(MIB_IF_ROW2));

    interfaceRow.InterfaceLuid = AdapterEntry->InterfaceLuid;
    interfaceRow.InterfaceIndex = AdapterEntry->InterfaceIndex;

    if (GetIfEntry2_I)
    {
        GetIfEntry2_I(&interfaceRow);
    }

    //MIB_IPINTERFACE_ROW interfaceTable;
    //memset(&interfaceTable, 0, sizeof(MIB_IPINTERFACE_ROW));
    //interfaceTable.Family = AF_INET;
    //interfaceTable.InterfaceLuid.Value = Context->AdapterEntry->InterfaceLuidValue;
    //interfaceTable.InterfaceIndex = Context->AdapterEntry->InterfaceIndex;
    //GetIpInterfaceEntry(&interfaceTable);

    return interfaceRow;
}

MIB_IFROW QueryInterfaceRowXP(
    _In_ PPH_NETADAPTER_ENTRY AdapterEntry
    )
{
    MIB_IFROW interfaceRow;

    memset(&interfaceRow, 0, sizeof(MIB_IFROW));

    interfaceRow.dwIndex = AdapterEntry->InterfaceIndex;

    GetIfEntry(&interfaceRow);

    //MIB_IPINTERFACE_ROW interfaceTable;
    //memset(&interfaceTable, 0, sizeof(MIB_IPINTERFACE_ROW));
    //interfaceTable.Family = AF_INET;
    //interfaceTable.InterfaceIndex = Context->AdapterEntry->InterfaceIndex;
    //GetIpInterfaceEntry(&interfaceTable);

    return interfaceRow;
}


//BOOLEAN NetworkAdapterQueryInternet(
//    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context,
//    _In_ PPH_STRING IpAddress
//    )
//{
//    // https://technet.microsoft.com/en-us/library/cc766017.aspx
//    BOOLEAN socketResult = FALSE;
//    WSADATA wsadata;
//    DNS_STATUS dnsQueryStatus = DNS_ERROR_RCODE_NO_ERROR;
//    PDNS_RECORD dnsQueryRecords = NULL;
//
//    WSAStartup(WINSOCK_VERSION, &wsadata);
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

//BOOLEAN NetworkAdapterQueryConfig(
//    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context, 
//    _Out_ PPH_NETADAPTER_CONFIG Config
//    )
//{
//    HANDLE keyHandle = NULL;
//    PPH_STRING keyPath = PhaFormatString(
//        L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s", 
//        Context->AdapterEntry->InterfaceGuid->Buffer
//        );
//
//    if (NT_SUCCESS(PhOpenKey(
//        &keyHandle,
//        KEY_READ,
//        PH_KEY_LOCAL_MACHINE,
//        &keyPath->sr,
//        0
//        )))
//    {
//        PH_NETADAPTER_CONFIG config;
//
//        memset(&config, 0, sizeof(PH_NETADAPTER_CONFIG));
//
//        config.AddressType = PhQueryRegistryUlong(keyHandle, L"AddressType");
//        config.EnableDHCP = PhQueryRegistryUlong(keyHandle, L"EnableDHCP");
//        config.IsServerNapAware = PhQueryRegistryUlong(keyHandle, L"IsServerNapAware");
//        config.interfaceLease.QuadPart = PhQueryRegistryUlong(keyHandle, L"Lease");
//        config.interfaceLeaseObtainedTime.QuadPart = PhQueryRegistryUlong(keyHandle, L"LeaseObtainedTime");
//        config.interfaceLeaseTerminatesTime.QuadPart = PhQueryRegistryUlong(keyHandle, L"LeaseTerminatesTime");
//      
//        if (config.EnableDHCP)
//        {
//            config.Domain = PhAutoDereferenceObject(PhQueryRegistryString(keyHandle, L"DhcpDomain"));
//            config.IPAddress = PhAutoDereferenceObject(PhQueryRegistryString(keyHandle, L"DhcpIPAddress"));
//            config.SubnetMask = PhQueryRegistryString(keyHandle, L"DhcpSubnetMask");
//            config.DefaultGateway = PhQueryRegistryString(keyHandle, L"DhcpDefaultGateway");
//            config.NameServer = PhQueryRegistryString(keyHandle, L"DhcpNameServer");
//        }
//        else
//        {
//            config.Domain = PhAutoDereferenceObject(PhQueryRegistryString(keyHandle, L"Domain"));
//            config.IPAddress = PhAutoDereferenceObject(PhQueryRegistryString(keyHandle, L"IPAddress"));
//            config.SubnetMask = PhQueryRegistryString(keyHandle, L"SubnetMask");
//            config.DefaultGateway = PhQueryRegistryString(keyHandle, L"DefaultGateway");
//            config.NameServer = PhQueryRegistryString(keyHandle, L"NameServer");
//            //PPH_STRING DhcpServer = PhQueryRegistryString(keyHandle, L"DhcpServer");
//            //PPH_STRING DhcpSubnetMask = PhQueryRegistryString(keyHandle, L"DhcpSubnetMask");
//            //PPH_STRING DhcpSubnetMaskOpt = PhQueryRegistryString(keyHandle, L"DhcpSubnetMaskOpt");
//            //ULONG dhcpServerMacAddressCount = PhQueryRegistryUlong(keyHandle, L"DhcpGatewayHardwareCount");
//            //ULONG dhcpServerMacAddress = PhQueryRegistryUlong(keyHandle, L"DhcpGatewayHardware");  // PhQueryRegistryValue  
//            //ULONG publisherName20 = PhQueryRegistryUlong(keyHandle, L"DhcpConnForceBroadcastFlag");
//        }
//
//        *Config = config;
//
//        NtClose(keyHandle);
//        return TRUE;
//    }
//
//    return FALSE;
//}