/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2025
 *
 */

#include <phapp.h>
#include <mapldr.h>
#include <netprv.h>

CONST NPI_MODULEID NPI_MS_IPV4_MODULEID = { 2, MIT_GUID, .Guid = { 0xeb004a00, 0x9b1a, 0x11d4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xbc } } };
CONST NPI_MODULEID NPI_MS_IPV6_MODULEID = { 2, MIT_GUID, .Guid = { 0xeb004a01, 0x9b1a, 0x11d4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xbc } } };
CONST NPI_MODULEID NPI_MS_UDP_MODULEID = { 2, MIT_GUID, .Guid = { 0xeb004a02, 0x9b1a, 0x11d4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xbc } } };
CONST NPI_MODULEID NPI_MS_TCP_MODULEID = { 2, MIT_GUID, .Guid = { 0xeb004a03, 0x9b1a, 0x11d4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xbc } } };
CONST NPI_MODULEID NPI_MS_NDIS_MODULEID = { 2, MIT_GUID, .Guid = { 0xeb004a11, 0x9b1a, 0x11d4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xbc } } };
CONST NPI_MODULEID NPI_MS_FL6T_MODULEID = { 2, MIT_GUID, .Guid = { 0xeb004a1C, 0x9b1a, 0x11d4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xbc } } }; // Teredo

typedef enum _NSI_STORE
{
    NsiPersistent,
    NsiActive,
    NsiBoth,
    NsiCurrent,
    NsiBootFirmwareTable
} NSI_STORE;

typedef enum _NSI_SET_ACTION
{
    NsiSetDefault = 0,
    NsiSetCreateOnly = 1,
    NsiSetCreateOrSet = 2,
    NsiSetDelete = 3,
    NsiSetReset = 4,
    NsiSetClear = 5,
    NsiSetCreateOrSetWithReference = 6,
    NsiSetDeleteWithReference = 7,
} NSI_SET_ACTION;

typedef enum _NSI_TCP_INFORMATION_CLASS
{
    NsiTcpTableOwnerPidAll = 3, // NSI_TCP_ALL_TABLE // TCP_TABLE_OWNER_PID_ALL and TCP_TABLE_OWNER_MODULE_ALL
    NsiTcpTableOwnerPidAllConnected = 4, // NSI_TCP_ESTAB_TABLE // TCP_TABLE_OWNER_PID_CONNECTIONS and TCP_TABLE_OWNER_MODULE_CONNECTIONS
    NsiTcpTableOwnerPidAllListener = 5, // NSI_TCP_LISTEN_TABLE // TCP_TABLE_OWNER_PID_LISTENER and TCP_TABLE_OWNER_MODULE_LISTENER

    //NsiTcpConnectionTable, // TCP_TABLE_OWNER_PID_ALL and TCP_TABLE_OWNER_MODULE_ALL
    //NsiTcpListenerTable, // TCP_TABLE_OWNER_PID_LISTENER and TCP_TABLE_OWNER_MODULE_LISTENER
    //NsiTcpStats, // MIB_TCPSTATS
    //NsiTcp6ConnectionTable, // TCP_TABLE_OWNER_PID_ALL and TCP_TABLE_OWNER_MODULE_ALL (IPv6)
    //NsiTcp6ListenerTable, // TCP_TABLE_OWNER_PID_LISTENER and TCP_TABLE_OWNER_MODULE_LISTENER (IPv6)
    //NsiTcp6Stats, // MIB_TCP6STATS
    //NsiTcpGlobalStats, // MIB_TCPSTATS_EX
    //NsiTcp6GlobalStats, // MIB_TCP6STATS_EX
    //NsiTcpConnectionTableEx, // TCP_TABLE_OWNER_PID_CONNECTIONS and TCP_TABLE_OWNER_MODULE_CONNECTIONS
    //NsiTcp6ConnectionTableEx, // TCP_TABLE_OWNER_PID_CONNECTIONS and TCP_TABLE_OWNER_MODULE_CONNECTIONS (IPv6)
    //NsiTcpFineGrainStats, // MIB_TCPSTATS_FINE
    //NsiTcp6FineGrainStats, // MIB_TCP6STATS_FINE
    //NsiTcpConnectionOffloadState, // TCP_OFFLOAD_STATE
    //NsiTcp6ConnectionOffloadState, // TCP_OFFLOAD_STATE (IPv6)
    //NsiTcpConnectionOffloadParameters, // TCP_OFFLOAD_PARAMETERS
    //NsiTcp6ConnectionOffloadParameters, // TCP_OFFLOAD_PARAMETERS (IPv6)

    NsiTcpSetIfEntry = 16, // input: NSI_SET_TCP_ENTRY

    //NsiTcpCongestionProvider, // TCP_CONGESTION_PROVIDER
    //NsiTcpCongestionProviderEx, // TCP_CONGESTION_PROVIDER_EX
    //NsiTcpMaxConnections, // ULONG
    //NsiTcpMaxHalfOpenConnections, // ULONG
    //NsiTcpConnectionOffloadStateEx, // TCP_OFFLOAD_STATE_EX
    //NsiTcp6ConnectionOffloadStateEx, // TCP_OFFLOAD_STATE_EX (IPv6)
    //NsiTcpMaxInfoClass
} NSI_TCP_INFORMATION_CLASS;

//typedef enum _NSI_UDP_INFORMATION_CLASS
//{
//    NsiUdpEndpointTable, // UDP_TABLE_OWNER_PID
//    NsiUdp6EndpointTable, // UDP6_TABLE_OWNER_PID
//    NsiUdpStats, // MIB_UDPSTATS
//    NsiUdp6Stats, // MIB_UDP6STATS
//    NsiUdpGlobalStats, // MIB_UDPSTATS_EX
//    NsiUdp6GlobalStats, // MIB_UDP6STATS_EX
//    NsiUdpMaxInfoClass
//} NSI_UDP_INFORMATION_CLASS;

typedef enum _NSI_IF_INFORMATION_CLASS
{
    NsiInterfaceLuidToGuid = 0,
    NsiInterfaceGuidToLuid = 1,
    NsiInterfaceIndexToLuid = 2,
    //NsiInterfaceMaxInfoClass
} NSI_IF_INFORMATION_CLASS;

typedef struct _NSI_SET_TCP_ENTRY
{
    union
    {
        union
        {
            SOCKADDR_IN LocalTcpEntryIpV4;
            SOCKADDR_IN6 LocalTcpEntryIpV6;
        };
        struct
        {
            ADDRESS_FAMILY LocalFamily;
            USHORT LocalPort;
            IN_ADDR LocalAddressIPv4;
            IN6_ADDR LocalAddressIPv6;
            ULONG LocalScopeID;
        };
    };
    union
    {
        union
        {
            SOCKADDR_IN RemoteTcpEntryIpV4;
            SOCKADDR_IN6 RemoteTcpEntryIpV6;
        };
        struct
        {
            ADDRESS_FAMILY RemoteFamily;
            USHORT RemotePort;
            IN_ADDR RemoteAddressIPv4;
            IN6_ADDR RemoteAddressIPv6;
            ULONG RemoteScopeID;
        };
    };
} NSI_SET_TCP_ENTRY, *PNSI_SET_TCP_ENTRY;

#if defined(PH_INLINE_NSI)
NTSYSAPI
NTSTATUS
NTAPI
NsiGetParameter(
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID Module,
    _In_ ULONG Class,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _In_ ULONG InputControlCode,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_ ULONG OutputControlCode
    );

NTSYSAPI
NTSTATUS
NTAPI
NsiSetAllParameters(
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID Module,
    _In_ ULONG Class,
    _In_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _In_ PVOID OptionalBuffer,
    _In_ ULONG OptionalBufferLength
    );
#else

#define NSI_DEVICE_NAME L"\\Device\\Nsi"

#define IOCTL_NSI_GET_PARAMETER \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x1, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x120007

#define IOCTL_NSI_SET_ALL_PARAMETERS \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x4, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x120013

#define IOCTL_NSI_GET_ALL_PARAMETERS \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x6, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12001B

typedef struct _NSI_IOCTL_GET_PARAMETER
{
    ULONGLONG Zero0;
    ULONGLONG Zero1;
    PNPI_MODULEID Module;
    ULONG Class;
    ULONG Reserved0;
    NSI_STORE Store;
    NSI_SET_ACTION Action;
    PVOID InputBuffer;
    ULONG InputBufferLength;
    ULONG Reserved1;
    ULONG InputControlCode;
    ULONG Reserved2;
    PVOID OutputBuffer;
    ULONG OutputBufferLength;
    ULONG Reserved3;
} NSI_IOCTL_GET_PARAMETER, *PNSI_IOCTL_GET_PARAMETER;

typedef struct _NSI_IOCTL_SET_ALL_PARAMETERS
{
    ULONGLONG Zero0;
    ULONGLONG Zero1;
    PNPI_MODULEID Module;
    ULONG Class;
    ULONG Reserved0;
    NSI_STORE Store;
    NSI_SET_ACTION Action;
    PVOID InputBuffer;
    ULONG InputBufferLength;
    ULONG Reserved1;
    PVOID OutputBuffer;
    ULONG OutputBufferLength;
    ULONG Reserved2;
} NSI_IOCTL_SET_ALL_PARAMETERS, *PNSI_IOCTL_SET_ALL_PARAMETERS;
#endif

NTSTATUS PhNsiGetParameter(
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID Module,
    _In_ ULONG Class,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _In_ ULONG InputControlCode,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    )
{
#if defined(PH_INLINE_NSI)
    static typeof(&NsiGetParameter) NsiGetParameter_I = NULL;

    if (!NsiGetParameter_I)
    {
        PVOID baseAddress;
        if (baseAddress = PhLoadLibrary(L"nsi.dll"))
        {
            NsiGetParameter_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiGetParameter", 0);
        }
    }

    if (!NsiGetParameter_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return NsiGetParameter_I(
        Store,
        Module,
        Class,
        InputBuffer,
        InputBufferLength,
        InputControlCode,
        OutputBuffer,
        OutputBufferLength,
        0
        );
#else
    NTSTATUS status;
    HANDLE deviceHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    NSI_IOCTL_GET_PARAMETER nsiGetParameter;

    RtlInitUnicodeString(&objectName, NSI_DEVICE_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &deviceHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    RtlZeroMemory(&nsiGetParameter, sizeof(nsiGetParameter));
    nsiGetParameter.Module = Module;
    nsiGetParameter.Class = Class;
    nsiGetParameter.Store = Store;
    nsiGetParameter.InputBuffer = InputBuffer;
    nsiGetParameter.InputBufferLength = InputBufferLength;
    nsiGetParameter.InputControlCode = InputControlCode;
    nsiGetParameter.OutputBuffer = OutputBuffer;
    nsiGetParameter.OutputBufferLength = OutputBufferLength;

    status = NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        IOCTL_NSI_GET_PARAMETER,
        &nsiGetParameter,
        sizeof(nsiGetParameter),
        &nsiGetParameter,
        sizeof(nsiGetParameter)
        );

    NtClose(deviceHandle);

    return status;
#endif
}

NTSTATUS PhNsiSetAllParameters(
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID Module,
    _In_ ULONG Class,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    )
{
#if defined(PH_INLINE_NSI)
    static typeof(&NsiSetAllParameters) NsiSetAllParameters_I = NULL;

    if (!NsiSetAllParameters_I)
    {
        PVOID baseAddress;
        if (baseAddress = PhLoadLibrary(L"nsi.dll"))
        {
            NsiSetAllParameters_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiSetAllParameters", 0);
        }
    }

    if (!NsiSetAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return NsiSetAllParameters_I(
        Store,
        Action,
        Module,
        Class,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength
        );
#else
    NTSTATUS status;
    HANDLE deviceHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    NSI_IOCTL_SET_ALL_PARAMETERS nsiSetAllParameters;

    RtlInitUnicodeString(&objectName, NSI_DEVICE_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &deviceHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    RtlZeroMemory(&nsiSetAllParameters, sizeof(nsiSetAllParameters));
    nsiSetAllParameters.Module = Module;
    nsiSetAllParameters.Class = Class;
    nsiSetAllParameters.Store = Store;
    nsiSetAllParameters.Action = Action;
    nsiSetAllParameters.InputBuffer = InputBuffer;
    nsiSetAllParameters.InputBufferLength = InputBufferLength;
    nsiSetAllParameters.OutputBuffer = OutputBuffer;
    nsiSetAllParameters.OutputBufferLength = OutputBufferLength;

    status = NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        IOCTL_NSI_SET_ALL_PARAMETERS,
        &nsiSetAllParameters,
        sizeof(nsiSetAllParameters),
        &nsiSetAllParameters,
        sizeof(nsiSetAllParameters)
        );

    NtClose(deviceHandle);

    return status;
#endif
}

NTSTATUS PhSetTcpEntry(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    NTSTATUS status;
    NSI_SET_TCP_ENTRY NsiSetTcpEntryData;

    RtlZeroMemory(&NsiSetTcpEntryData, sizeof(NsiSetTcpEntryData));

    if (NetworkItem->ProtocolType == PH_NETWORK_PROTOCOL_TCP4)
    {
        NsiSetTcpEntryData.LocalFamily = AF_INET;
        NsiSetTcpEntryData.LocalPort = _byteswap_ushort((USHORT)NetworkItem->LocalEndpoint.Port);
        memcpy(&NsiSetTcpEntryData.LocalAddressIPv4, &NetworkItem->LocalEndpoint.Address.InAddr, sizeof(NsiSetTcpEntryData.LocalAddressIPv4));

        NsiSetTcpEntryData.RemoteFamily = AF_INET;
        NsiSetTcpEntryData.RemotePort = _byteswap_ushort((USHORT)NetworkItem->RemoteEndpoint.Port);
        memcpy(&NsiSetTcpEntryData.RemoteAddressIPv4, &NetworkItem->RemoteEndpoint.Address.InAddr, sizeof(NsiSetTcpEntryData.LocalAddressIPv4));
    }
    else if (NetworkItem->ProtocolType == PH_NETWORK_PROTOCOL_TCP6)
    {
        NsiSetTcpEntryData.LocalFamily = AF_INET6;
        NsiSetTcpEntryData.LocalPort = _byteswap_ushort((USHORT)NetworkItem->LocalEndpoint.Port);
        memcpy(&NsiSetTcpEntryData.LocalAddressIPv6, &NetworkItem->LocalEndpoint.Address.In6Addr, sizeof(NsiSetTcpEntryData.LocalAddressIPv6));
        NsiSetTcpEntryData.LocalScopeID = NetworkItem->LocalScopeId;

        NsiSetTcpEntryData.RemoteFamily = AF_INET6;
        NsiSetTcpEntryData.RemotePort = _byteswap_ushort((USHORT)NetworkItem->RemoteEndpoint.Port);
        memcpy(&NsiSetTcpEntryData.RemoteAddressIPv6, &NetworkItem->RemoteEndpoint.Address.In6Addr, sizeof(NsiSetTcpEntryData.RemoteAddressIPv6));
        NsiSetTcpEntryData.RemoteScopeID = NetworkItem->RemoteScopeId;
    }

    status = PhNsiSetAllParameters(
        NsiActive,
        NsiSetCreateOrSet,
        &NPI_MS_TCP_MODULEID,
        NsiTcpSetIfEntry,
        &NsiSetTcpEntryData,
        sizeof(NSI_SET_TCP_ENTRY),
        NULL,
        0
        );

    return status;
}

NTSTATUS PhConvertInterfaceIndexToLuid(
    _In_ NET_IFINDEX InterfaceIndex,
    _Out_ PNET_LUID InterfaceLuid
    )
{
    NET_IFINDEX index = InterfaceIndex;

    return PhNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiInterfaceIndexToLuid,
        &index,
        sizeof(NET_IFINDEX),
        2,
        InterfaceLuid,
        sizeof(NET_LUID)
        );
}
