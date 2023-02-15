/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2022
 *
 */

#include "devices.h"
#include <objbase.h>

BOOLEAN NetworkAdapterQuerySupported(
    _In_ HANDLE DeviceHandle
    )
{
    NTSTATUS status;
    BOOLEAN ndisQuerySupported = FALSE;
    BOOLEAN adapterNameSupported = FALSE;
    BOOLEAN adapterStatsSupported = FALSE;
    BOOLEAN adapterLinkStateSupported = FALSE;
    BOOLEAN adapterLinkSpeedSupported = FALSE;
    PNDIS_OID objectIdBuffer;
    ULONG objectIdBufferLength;
    ULONG objectIdBufferReturnLength;
    ULONG attempts = 0;

    objectIdBufferLength = 2048 * sizeof(NDIS_OID);
    objectIdBuffer = PhAllocateZero(objectIdBufferLength);

    status = PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &(NDIS_OID) { OID_GEN_SUPPORTED_LIST },
        sizeof(NDIS_OID),
        objectIdBuffer,
        objectIdBufferLength,
        &objectIdBufferReturnLength
        );

    while (status == STATUS_BUFFER_OVERFLOW && attempts < 8)
    {
        PhFree(objectIdBuffer);
        objectIdBufferLength *= 2;
        objectIdBuffer = PhAllocateZero(objectIdBufferLength);

        status = PhDeviceIoControlFile(
            DeviceHandle,
            IOCTL_NDIS_QUERY_GLOBAL_STATS,
            &(NDIS_OID) { OID_GEN_SUPPORTED_LIST },
            sizeof(NDIS_OID),
            objectIdBuffer,
            objectIdBufferLength,
            &objectIdBufferReturnLength
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
    {
        ndisQuerySupported = TRUE;

        for (ULONG i = 0; i < objectIdBufferReturnLength / sizeof(NDIS_OID); i++)
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
    ULONG versionResult = 0;

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &(NDIS_OID) { OID_GEN_DRIVER_VERSION }, // OID_GEN_VENDOR_DRIVER_VERSION
        sizeof(NDIS_OID),
        &versionResult,
        sizeof(versionResult),
        NULL
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

PPH_STRING NetworkAdapterQueryNameFromInterfaceGuid(
    _In_ PGUID InterfaceGuid
    )
{
    // Query adapter description using undocumented function. (dmex)
    static ULONG (WINAPI* NhGetInterfaceDescriptionFromGuid_I)(
        _In_ PGUID InterfaceGuid,
        _Out_opt_ PWSTR InterfaceDescription,
        _Inout_ PSIZE_T InterfaceDescriptionLength,
        _In_ BOOL Cache,
        _In_ BOOL Refresh
        ) = NULL;
    GUID interfaceGuid = { 0 };
    WCHAR adapterDescription[NDIS_IF_MAX_STRING_SIZE + 1] = L"";
    SIZE_T adapterDescriptionLength = sizeof(adapterDescription);

    if (!NhGetInterfaceDescriptionFromGuid_I)
    {
        PVOID iphlpHandle;

        if (!(iphlpHandle = PhGetDllHandle(L"iphlpapi.dll")))
            iphlpHandle = PhLoadLibrary(L"iphlpapi.dll");

        if (iphlpHandle)
        {
            NhGetInterfaceDescriptionFromGuid_I = PhGetProcedureAddress(iphlpHandle, "NhGetInterfaceDescriptionFromGuid", 0);
        }
    }

    if (!NhGetInterfaceDescriptionFromGuid_I)
        return NULL;

    if (SUCCEEDED(NhGetInterfaceDescriptionFromGuid_I(
        InterfaceGuid,
        adapterDescription,
        &adapterDescriptionLength,
        FALSE,
        TRUE
        )))
    {
        return PhCreateString(adapterDescription);
    }

    return NULL;
}

PPH_STRING NetworkAdapterQueryNameFromDeviceGuid(
    _In_ PGUID InterfaceGuid
    )
{
    // Query adapter description using undocumented function. (dmex)
    static ULONG (WINAPI* NhGetInterfaceNameFromDeviceGuid_I)(
        _In_ PGUID DeviceGuid,
        _Out_writes_(InterfaceDescriptionLength) PWSTR InterfaceDescription,
        _Inout_ PSIZE_T InterfaceDescriptionLength,
        _In_ BOOL Cache,
        _In_ BOOL Refresh
        ) = NULL;
    GUID interfaceGuid = { 0 };
    WCHAR adapterAlias[NDIS_IF_MAX_STRING_SIZE + 1] = L"";
    SIZE_T adapterAliasLength = sizeof(adapterAlias);

    if (!NhGetInterfaceNameFromDeviceGuid_I)
    {
        PVOID iphlpHandle;

        if (!(iphlpHandle = PhGetDllHandle(L"iphlpapi.dll")))
            iphlpHandle = PhLoadLibrary(L"iphlpapi.dll");

        if (iphlpHandle)
        {
            NhGetInterfaceNameFromDeviceGuid_I = PhGetProcedureAddress(iphlpHandle, "NhGetInterfaceNameFromDeviceGuid", 0);
        }
    }

    if (!NhGetInterfaceNameFromDeviceGuid_I)
        return NULL;

    if (SUCCEEDED(NhGetInterfaceNameFromDeviceGuid_I(
        InterfaceGuid,
        adapterAlias,
        &adapterAliasLength,
        FALSE,
        TRUE
        )))
    {
        return PhCreateString(adapterAlias);
    }

    return NULL;
}

PPH_STRING NetworkAdapterGetInterfaceAliasFromLuid(
    _In_ PDV_NETADAPTER_ID Id
    )
{
    WCHAR aliasBuffer[IF_MAX_STRING_SIZE + 1];

    if (NETIO_SUCCESS(ConvertInterfaceLuidToAlias(&Id->InterfaceLuid, aliasBuffer, IF_MAX_STRING_SIZE)))
    {
        return PhCreateString(aliasBuffer);
    }

    return NULL;
}

PPH_STRING NetworkAdapterGetInterfaceNameFromLuid(
    _In_ PDV_NETADAPTER_ID Id
    )
{
    WCHAR interfaceName[IF_MAX_STRING_SIZE + 1];

    if (NETIO_SUCCESS(ConvertInterfaceLuidToNameW(&Id->InterfaceLuid, interfaceName, IF_MAX_STRING_SIZE)))
    {
        return PhCreateString(interfaceName);
    }

    return NULL;
}

PPH_STRING NetworkAdapterGetInterfaceAliasNameFromGuid(
    _In_ PGUID InterfaceGuid
    )
{
    static ULONG (WINAPI *NhGetInterfaceNameFromGuid_I)(
        _In_ PGUID InterfaceGuid,
        _Out_writes_(InterfaceAliasLength) PWSTR InterfaceAlias,
        _Inout_ PSIZE_T InterfaceAliasLength,
        _In_ BOOL Cache,
        _In_ BOOL Refresh
        ) = NULL;
    GUID interfaceGuid = { 0 };
    WCHAR adapterAlias[NDIS_IF_MAX_STRING_SIZE + 1] = L"";
    SIZE_T adapterAliasLength = sizeof(adapterAlias);

    if (!NhGetInterfaceNameFromGuid_I)
    {
        PVOID iphlpHandle;

        if (!(iphlpHandle = PhGetDllHandle(L"iphlpapi.dll")))
            iphlpHandle = PhLoadLibrary(L"iphlpapi.dll");

        if (iphlpHandle)
        {
            NhGetInterfaceNameFromGuid_I = PhGetProcedureAddress(iphlpHandle, "NhGetInterfaceNameFromGuid", 0);
        }
    }

    if (!NhGetInterfaceNameFromGuid_I)
        return NULL;

    if (SUCCEEDED(NhGetInterfaceNameFromGuid_I(
        InterfaceGuid,
        adapterAlias,
        &adapterAliasLength,
        FALSE,
        TRUE
        )))
    {
        return PhCreateString(adapterAlias);
    }

    return NULL;
}

PPH_STRING NetworkAdapterQueryName(
    _In_ HANDLE DeviceHandle
    )
{
    ULONG adapterNameReturnLength;
    WCHAR adapterName[NDIS_IF_MAX_STRING_SIZE + 1] = L"";

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &(NDIS_OID) { OID_GEN_FRIENDLY_NAME },
        sizeof(NDIS_OID),
        adapterName,
        sizeof(adapterName),
        &adapterNameReturnLength
        )))
    {
        PPH_STRING string;

        if (adapterNameReturnLength > sizeof(UNICODE_NULL))
        {
            string = PhCreateStringEx(adapterName, adapterNameReturnLength - sizeof(UNICODE_NULL));
            //PhTrimToNullTerminatorString(string);
        }
        else
        {
            string = PhCreateString(adapterName);
        }

        return string;
    }

    return NULL;
}

NTSTATUS NetworkAdapterQueryStatistics(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_STATISTICS_INFO Info
    )
{
    NDIS_STATISTICS_INFO result;

    memset(&result, 0, sizeof(NDIS_STATISTICS_INFO));
    result.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    result.Header.Revision = NDIS_STATISTICS_INFO_REVISION_1;
    result.Header.Size = NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;

    return PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &(NDIS_OID) { OID_GEN_STATISTICS },
        sizeof(NDIS_OID),
        Info,
        sizeof(NDIS_STATISTICS_INFO),
        NULL
        );
}

NTSTATUS NetworkAdapterQueryLinkState(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_LINK_STATE State
    )
{
    NDIS_LINK_STATE result;

    memset(&result, 0, sizeof(NDIS_LINK_STATE));
    result.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    result.Header.Revision = NDIS_LINK_STATE_REVISION_1;
    result.Header.Size = NDIS_SIZEOF_LINK_STATE_REVISION_1;

    return PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &(NDIS_OID) { OID_GEN_LINK_STATE }, // OID_GEN_MEDIA_CONNECT_STATUS
        sizeof(NDIS_OID),
        State,
        sizeof(NDIS_LINK_STATE),
        NULL
        );
}

_Success_(return)
BOOLEAN NetworkAdapterQueryMediaType(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_PHYSICAL_MEDIUM Medium
    )
{
    NDIS_MEDIUM adapterType;
    NDIS_PHYSICAL_MEDIUM adapterMediaType;

    adapterMediaType = NdisPhysicalMediumUnspecified;

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &(NDIS_OID) { OID_GEN_PHYSICAL_MEDIUM_EX },
        sizeof(NDIS_OID),
        &adapterMediaType,
        sizeof(adapterMediaType),
        NULL
        )))
    {
        *Medium = adapterMediaType;
        return TRUE;
    }

    adapterMediaType = NdisPhysicalMediumUnspecified;

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &(NDIS_OID) { OID_GEN_PHYSICAL_MEDIUM },
        sizeof(NDIS_OID),
        &adapterMediaType,
        sizeof(adapterMediaType),
        NULL
        )))
    {
        *Medium = adapterMediaType;
        return TRUE;
    }

    adapterType = NdisMediumMax;

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &(NDIS_OID) { OID_GEN_MEDIA_SUPPORTED }, // OID_GEN_MEDIA_IN_USE
        sizeof(NDIS_OID),
        &adapterType,
        sizeof(adapterType),
        NULL
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
    NDIS_LINK_SPEED result;

    memset(&result, 0, sizeof(NDIS_LINK_SPEED));

    status = PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &(NDIS_OID) { OID_GEN_LINK_SPEED },
        sizeof(NDIS_OID),
        &result,
        sizeof(NDIS_LINK_SPEED),
        NULL
        );

    if (NT_SUCCESS(status))
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
    ULONG64 result = 0;

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_NDIS_QUERY_GLOBAL_STATS,
        &OpCode,
        sizeof(NDIS_OID),
        &result,
        sizeof(result),
        NULL
        )))
    {
        return result;
    }

    return 0;
}

_Success_(return)
BOOLEAN NetworkAdapterQueryInterfaceRow(
    _In_ PDV_NETADAPTER_ID Id,
    _In_ MIB_IF_ENTRY_LEVEL Level,
    _Out_ PMIB_IF_ROW2 InterfaceRow
    )
{
    MIB_IF_ROW2 interfaceRow;

    memset(&interfaceRow, 0, sizeof(MIB_IF_ROW2));
    interfaceRow.InterfaceLuid = Id->InterfaceLuid;
    interfaceRow.InterfaceIndex = Id->InterfaceIndex;

    if (PhWindowsVersion >= WINDOWS_10_RS2 && GetIfEntry2Ex)
    {
        if (NETIO_SUCCESS(GetIfEntry2Ex(Level, &interfaceRow)))
        {
            *InterfaceRow = interfaceRow;
            return TRUE;
        }
    }

    if (NETIO_SUCCESS(GetIfEntry2(&interfaceRow)))
    {
        *InterfaceRow = interfaceRow;
        return TRUE;
    }

    //MIB_IPINTERFACE_ROW interfaceTable;
    //memset(&interfaceTable, 0, sizeof(MIB_IPINTERFACE_ROW));
    //interfaceTable.Family = AF_INET;
    //interfaceTable.InterfaceLuid.Value = Context->AdapterEntry->InterfaceLuidValue;
    //interfaceTable.InterfaceIndex = Context->AdapterEntry->InterfaceIndex;
    //GetIpInterfaceEntry(&interfaceTable);

    return FALSE;
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

PPH_STRING NetAdapterFormatBitratePrefix(
    _In_ ULONG64 Value
    )
{
    static PH_STRINGREF SiPrefixUnitNamesCounted[7] =
    {
        PH_STRINGREF_INIT(L" Bps"),
        PH_STRINGREF_INIT(L" Kbps"),
        PH_STRINGREF_INIT(L" Mbps"),
        PH_STRINGREF_INIT(L" Gbps"),
        PH_STRINGREF_INIT(L" Tbps"),
        PH_STRINGREF_INIT(L" Pbps"),
        PH_STRINGREF_INIT(L" Ebps")
    };
    DOUBLE number = (DOUBLE)Value;
    ULONG i = 0;
    PH_FORMAT format[2];

    while (
        number >= 1000 &&
        i < RTL_NUMBER_OF(SiPrefixUnitNamesCounted) &&
        i < ULONG_MAX // PhMaxSizeUnit
        )
    {
        number /= 1000;
        i++;
    }

    format[0].Type = DoubleFormatType | FormatUsePrecision;
    format[0].Precision = 1;
    format[0].u.Double = number;
    PhInitFormatSR(&format[1], SiPrefixUnitNamesCounted[i]);

    return PhFormat(format, 2, 0);
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
//                if ((socketHandle = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_NO_HANDLE_INHERIT)) == INVALID_SOCKET)
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
//                if ((socketHandle = WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_NO_HANDLE_INHERIT)) == INVALID_SOCKET)
//                    continue;
//
//                IN6_ADDR sockAddr;
//                InetPton(AF_INET6, IpAddress->Buffer, &sockAddr);
//
//                SOCKADDR_IN6 remoteAddr = { 0 };
//                remoteAddr.sin6_family = AF_INET6;
//                remoteAddr.sin6_port = htons(80);
//                memcpy(&remoteAddr.sin6_addr.s6_addr, i->Data.AAAA.Ip6Address.IP6Byte, sizeof(i->Data.AAAA.Ip6Address.IP6Byte));
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
