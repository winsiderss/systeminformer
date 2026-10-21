/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2017-2026
 *
 */

#ifndef PH_NETPRV_H
#define PH_NETPRV_H

extern PPH_OBJECT_TYPE PhNetworkItemType;
extern BOOLEAN PhEnableNetworkProviderResolve;
extern BOOLEAN PhEnableNetworkBoundConnections;

typedef enum _PH_NETWORK_PROVIDER_FLAG
{
    PH_NETWORK_PROVIDER_FLAG_HOSTNAME = 0x1,
} PH_NETWORK_PROVIDER_FLAG;

extern ULONG PhNetworkProviderFlagsMask;

// Defined in mainwnd.c; netprv.c reads the elapsed time actually spanned by the last provider run. (dmex)
extern PH_PROVIDER_REGISTRATION PhMwpNetworkProviderRegistration;

// begin_phapppub
#define PH_NETWORK_OWNER_INFO_SIZE 16

typedef struct _PH_NETWORK_ITEM
{
    ULONG ProtocolType;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;
    MIB_TCP_STATE State;
    HANDLE ProcessId;

    PPH_STRING ProcessName;
    ULONG_PTR ProcessIconIndex;
    BOOLEAN ProcessIconValid;
    PPH_STRING OwnerName;

    volatile LONG JustResolved;

    PPH_STRING LocalAddressString;
    WCHAR LocalPortString[PH_INT32_STR_LEN_1];
    PPH_STRING RemoteAddressString;
    WCHAR RemotePortString[PH_INT32_STR_LEN_1];
    PPH_STRING LocalHostString;
    PPH_STRING RemoteHostString;
    PPH_STRING HvService;

    LARGE_INTEGER CreateTime;
    ULONG LocalScopeId;
    ULONG RemoteScopeId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG UnknownProcess : 1;
            ULONG SubsystemProcess : 1;
            ULONG Spare : 27;
            ULONG InvalidateHostname : 1;
            ULONG LocalHostnameResolved : 1;
            ULONG RemoteHostnameResolved : 1;
        };
    };

    PPH_PROCESS_ITEM ProcessItem;
    PH_HASH_ENTRY HashEntry;
} PH_NETWORK_ITEM, *PPH_NETWORK_ITEM;

typedef struct _PH_NETWORK_CONNECTION
{
    ULONG ProtocolType;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;
    MIB_TCP_STATE State;
    HANDLE ProcessId;
    LARGE_INTEGER CreateTime;
    ULONGLONG OwnerInfo[PH_NETWORK_OWNER_INFO_SIZE];
    ULONG LocalScopeId; // Ipv6
    ULONG RemoteScopeId; // Ipv6
} PH_NETWORK_CONNECTION, *PPH_NETWORK_CONNECTION;
// end_phapppub

BOOLEAN PhNetworkProviderInitialization(
    VOID
    );

PPH_NETWORK_ITEM PhCreateNetworkItem(
    VOID
    );

// begin_phapppub
PHAPPAPI
PPH_NETWORK_ITEM
NTAPI
PhReferenceNetworkItem(
    _In_ ULONG ProtocolType,
    _In_ PPH_IP_ENDPOINT LocalEndpoint,
    _In_ PPH_IP_ENDPOINT RemoteEndpoint,
    _In_ HANDLE ProcessId
    );

PHAPPAPI
VOID
NTAPI
PhEnumNetworkItems(
    _Out_opt_ PPH_NETWORK_ITEM** NetworkItems,
    _Out_ PULONG NumberOfNetworkItems
    );

PHAPPAPI
VOID
NTAPI
PhEnumNetworkItemsByProcessId(
    _In_opt_ HANDLE ProcessId,
    _Out_opt_ PPH_NETWORK_ITEM** NetworkItems,
    _Out_ PULONG NumberOfNetworkItems
    );

_Success_(return)
PHAPPAPI
BOOLEAN
NTAPI
PhGetNetworkConnections(
    _Out_ PPH_NETWORK_CONNECTION *Connections,
    _Out_ PULONG NumberOfConnections
    );
// end_phapppub

VOID PhFlushNetworkItemResolveCache(
    VOID
    );

//PPH_STRING PhGetHostNameFromAddress(
//    _In_ PPH_IP_ADDRESS Address
//    );

_Function_class_(PH_PROVIDER_FUNCTION)
VOID PhNetworkProviderUpdate(
    _In_ PVOID Object
    );

// begin_phapppub
PHAPPAPI
PCPH_STRINGREF
NTAPI
PhGetProtocolTypeName(
    _In_ ULONG ProtocolType
    );

PHAPPAPI
PCPH_STRINGREF
NTAPI
PhGetTcpStateName(
    _In_ ULONG State
    );

PHAPPAPI
NTSTATUS
PhGetPerTcpConnectionEStats(
    _In_ PMIB_TCPROW Row,
    _In_ TCP_ESTATS_TYPE EstatsType,
    _Out_writes_bytes_opt_(RwSize) PVOID Rw,
    _In_ ULONG RwVersion,
    _In_ ULONG RwSize,
    _Out_writes_bytes_opt_(RosSize) PVOID Ros,
    _In_ ULONG RosVersion,
    _In_ ULONG RosSize,
    _Out_writes_bytes_opt_(RodSize) PVOID Rod,
    _In_ ULONG RodVersion,
    _In_ ULONG RodSize
    );

PHAPPAPI
NTSTATUS
PhGetPerTcp6ConnectionEStats(
    _In_ PMIB_TCP6ROW Row,
    _In_ TCP_ESTATS_TYPE EstatsType,
    _Out_writes_bytes_opt_(RwSize) PVOID Rw,
    _In_ ULONG RwVersion,
    _In_ ULONG RwSize,
    _Out_writes_bytes_opt_(RosSize) PVOID Ros,
    _In_ ULONG RosVersion,
    _In_ ULONG RosSize,
    _Out_writes_bytes_opt_(RodSize) PVOID Rod,
    _In_ ULONG RodVersion,
    _In_ ULONG RodSize
    );

PHAPPAPI
NTSTATUS
PhSetPerTcpConnectionEStats(
    _In_ PMIB_TCPROW Row,
    _In_ TCP_ESTATS_TYPE EstatsType,
    _In_reads_bytes_opt_(RwSize) PUCHAR Rw,
    _In_ ULONG RwVersion,
    _In_ ULONG RwSize,
    _In_ ULONG Offset
    );

PHAPPAPI
NTSTATUS
PhSetPerTcp6ConnectionEStats(
    _In_ PMIB_TCP6ROW Row,
    _In_ TCP_ESTATS_TYPE EstatsType,
    _In_reads_bytes_opt_(RwSize) PUCHAR Rw,
    _In_ ULONG RwVersion,
    _In_ ULONG RwSize,
    _In_ ULONG Offset
    );
// end_phapppub

PHAPPAPI
BOOLEAN
NTAPI
PhIsUdpExemptPort(
    _In_ ULONG Port
    );

// iphlpapi imports

typedef ULONG (WINAPI *_GetExtendedTcpTable)(
    _Out_writes_bytes_opt_(*pdwSize) PVOID pTcpTable,
    _Inout_ PULONG pdwSize,
    _In_ BOOL bOrder,
    _In_ ULONG ulAf,
    _In_ TCP_TABLE_CLASS TableClass,
    _In_ ULONG Reserved
    );

typedef ULONG (WINAPI *_GetExtendedUdpTable)(
    _Out_writes_bytes_opt_(*pdwSize) PVOID pUdpTable,
    _Inout_ PULONG pdwSize,
    _In_ BOOL bOrder,
    _In_ ULONG ulAf,
    _In_ UDP_TABLE_CLASS TableClass,
    _In_ ULONG Reserved
    );

//DECLSPEC_IMPORT ULONG WINAPI InternalGetTcpTableWithOwnerModule(
//    _Out_ PVOID* Tcp4Table, // PMIB_TCPTABLE_OWNER_MODULE
//    _In_ PVOID HeapHandle,
//    _In_opt_ ULONG HeapFlags
//    );
//DECLSPEC_IMPORT ULONG WINAPI InternalGetTcp6TableWithOwnerModule(
//    _Out_ PVOID* Tcp6Table, // PMIB_TCP6TABLE_OWNER_MODULE
//    _In_ PVOID HeapHandle,
//    _In_opt_ ULONG HeapFlags
//    );
//DECLSPEC_IMPORT ULONG WINAPI InternalGetUdpTableWithOwnerModule(
//    _Out_ PVOID* Udp4Table, // PMIB_UDPTABLE_OWNER_MODULE
//    _In_ PVOID HeapHandle,
//    _In_opt_ ULONG HeapFlags
//    );
//DECLSPEC_IMPORT ULONG WINAPI InternalGetUdp6TableWithOwnerModule(
//    _Out_ PVOID* Udp6Table, // PMIB_UDP6TABLE_OWNER_MODULE
//    _In_ PVOID HeapHandle,
//    _In_opt_ ULONG HeapFlags
//    );

typedef ULONG (WINAPI *_InternalGetBoundTcpEndpointTable)(
    _Out_ _When_(return!=0, _Notnull_) PVOID* BoundTcpTable, // PMIB_TCPTABLE2
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG HeapFlags
    );

typedef ULONG (WINAPI *_InternalGetBoundTcp6EndpointTable)(
    _Out_ _When_(return!=0, _Notnull_) PVOID* BoundTcpTable, // PMIB_TCP6TABLE2
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG HeapFlags
    );

// netsup

// begin_phapppub
PHAPPAPI
NTSTATUS
NTAPI
PhSetTcpEntry(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );
// end_phapppub

NTSTATUS
PhGetExtendedTcpTable(
    _Out_writes_bytes_opt_(*pdwSize) PVOID pTcpTable,
    _Inout_ PULONG pdwSize,
    _In_ BOOL bOrder,
    _In_ ULONG ulAf,
    _In_ TCP_TABLE_CLASS TableClass,
    _In_ ULONG Reserved
    );

NTSTATUS
PhGetExtendedUdpTable(
    _Out_writes_bytes_opt_(*pdwSize) PVOID pUdpTable,
    _Inout_ PULONG pdwSize,
    _In_ BOOL bOrder,
    _In_ ULONG ulAf,
    _In_ UDP_TABLE_CLASS TableClass,
    _In_ ULONG Reserved
    );

NTSTATUS
NTAPI
PhGetAdaptersAddresses(
    _In_ ULONG Family,
    _In_ ULONG Flags,
    _Reserved_ PVOID Reserved,
    _Out_writes_bytes_opt_(*SizePointer) PIP_ADAPTER_ADDRESSES AdapterAddresses,
    _Inout_ PULONG SizePointer
    );

NTSTATUS
NTAPI
PhGetNetworkParams(
    _Out_writes_bytes_opt_(*SizePointer) PFIXED_INFO FixedInfo,
    _Inout_ PULONG SizePointer
    );

NTSTATUS
NTAPI
PhCancelMibChangeNotify2(
    _In_ HANDLE NotificationHandle
    );

NTSTATUS
WINAPI
PhInternalGetBoundTcpEndpointTable(
    _Out_ PVOID* BoundTcpTable, // PMIB_TCPTABLE2
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG HeapFlags
    );

NTSTATUS
WINAPI
PhInternalGetBoundTcp6EndpointTable(
    _Out_ PVOID* BoundTcpTable, // PMIB_TCP6TABLE2
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG HeapFlags
    );

NTSTATUS
NTAPI
PhNotifyIpInterfaceChange(
    _In_ ADDRESS_FAMILY Family,
    _In_ PIPINTERFACE_CHANGE_CALLBACK Callback,
    _In_opt_ PVOID CallerContext,
    _In_ BOOLEAN InitialNotification,
    _Inout_ PHANDLE NotificationHandle
    );

NTSTATUS
NTAPI
PhParseNetworkString(
    _In_ PCWSTR NetworkString,
    _In_ ULONG Types,
    _Out_opt_ PNET_ADDRESS_INFO AddressInfo,
    _Out_opt_ PUSHORT PortNumber,
    _Out_opt_ PUCHAR PrefixLength
    );

NTSTATUS
NTAPI
PhNhGetInterfaceNameFromDeviceGuid(
    _In_ PGUID DeviceGuid,
    _Out_writes_(InterfaceDescriptionLength) PWCHAR InterfaceDescription,
    _Inout_ PULONG InterfaceDescriptionLength,
    _In_ BOOLEAN Cache,
    _In_ BOOLEAN Refresh
    );

NTSTATUS
NTAPI
PhGetIpStatistics(
    _Out_ PMIB_IPSTATS Statistics
    );

NTSTATUS
NTAPI
PhGetIpAddrTable(
    _Out_writes_bytes_opt_(*SizePointer) PMIB_IPADDRTABLE IpAddrTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOLEAN Order
    );

NTSTATUS
NTAPI
PhRegisterInterfaceTimestampConfigChange(
    _In_ PINTERFACE_TIMESTAMP_CONFIG_CHANGE_CALLBACK Callback,
    _In_opt_ PVOID CallerContext,
    _Out_ HIFTIMESTAMPCHANGE *NotificationHandle
    );

VOID
NTAPI
PhUnregisterInterfaceTimestampConfigChange(
    _In_ HIFTIMESTAMPCHANGE NotificationHandle
    );

NTSTATUS
NTAPI
PhGetInterfaceHandle(
    _In_ const NET_LUID *InterfaceLuid,
    _Out_ PHANDLE InterfaceHandle
    );

NTSTATUS
NTAPI
PhConvertNdisStatusToError(
    _In_ NTSTATUS Status
    );

BOOLEAN
NTAPI
PhCancelIPChangeNotify(
    _In_ LPOVERLAPPED Overlapped
    );

NTSTATUS
NTAPI
PhAllocateAndGetInterfaceInfoFromStack(
    _Outptr_ PIP_INTERFACE_NAME_INFO *InterfaceInfo,
    _Out_ PULONG SizePointer,
    _In_ BOOLEAN Order
    );

NTSTATUS
NTAPI
PhAllocateAndGetIpAddrTableFromStack(
    _Outptr_ PMIB_IPADDRTABLE *IpAddrTable,
    _Out_ PULONG SizePointer,
    _In_ BOOLEAN Order
    );

NTSTATUS
NTAPI
PhCloseCompartment(
    _In_ HANDLE CompartmentHandle
    );

NTSTATUS
NTAPI
PhCloseGetIPPhysicalInterfaceForDestination(
    _Inout_ PHANDLE Handle
    );

#endif
