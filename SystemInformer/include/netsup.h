/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2025-2026
 *
 */

#ifndef PH_NETSUP_H
#define PH_NETSUP_H

typedef enum _PH_NETIO_ADDRESS_TYPE
{
    PhNetioAddressUnicast, // Unicast address rows (MIB_UNICASTIPADDRESS_ROW).
    PhNetioAddressMulticast, // Multicast address rows (MIB_MULTICASTIPADDRESS_ROW).
    PhNetioAddressAnycast // Anycast address rows (MIB_ANYCASTIPADDRESS_ROW).
} PH_NETIO_ADDRESS_TYPE;

typedef enum _PH_TCP_OWNER_CLASS
{
    PhTcpOwnerNone,
    PhTcpOwnerBasic,   // Basic TCP rows with connection state and endpoints.
    PhTcpOwnerBasic2,  // TCP rows with owning process ID and offload state.
    PhTcpOwnerModule,  // TCP rows with owning process ID, creation time and module information.
    PhTcpOwnerPid      // TCP rows with owning process ID.
} PH_TCP_OWNER_CLASS;

static_assert(sizeof(PH_TCP_OWNER_CLASS) == sizeof(ULONG));
static_assert(sizeof(PMIB_TCPTABLE2) == sizeof(PVOID));
static_assert(sizeof(PMIB_TCP6TABLE2) == sizeof(PVOID));

typedef struct _TCPIP_IF_TYPE_STRING_ENTRY
{
    PCWSTR Name;
    USHORT IfType;
} TCPIP_IF_TYPE_STRING_ENTRY;

typedef struct _MIB_NOTIFICATION_HANDLE
{
    PVOID Reserved;
    NSI_IP_OBJECT_ID ObjectId;
    PVOID NotificationHandleV4;
    PVOID NotificationHandleV6;
    PVOID NotificationHandleTeredo;
    PVOID NotificationHandleNdis;
    PVOID ConnectivityHintContext;
} MIB_NOTIFICATION_HANDLE, * PMIB_NOTIFICATION_HANDLE;

typedef struct _PH_CONNECTIVITY_HINT_CONTEXT
{
    LIST_ENTRY ListEntry;
    PVOID Callback;
    PVOID CallerContext;
    PTP_WORK ThreadpoolWork;
    LONG State;
} PH_CONNECTIVITY_HINT_CONTEXT, * PPH_CONNECTIVITY_HINT_CONTEXT;

typedef struct _PH_ROUTE_CHANGE_KEY
{
    PVOID Key;
    ULONG KeyLength;
} PH_ROUTE_CHANGE_KEY, *PPH_ROUTE_CHANGE_KEY;

typedef struct _PH_ROUTE_CHANGE_CONTEXT
{
    NETIO_CHANGE_CALLBACK_CONTEXT Callback;
    MIB_IPFORWARD_ROW2 Row;
} PH_ROUTE_CHANGE_CONTEXT, *PPH_ROUTE_CHANGE_CONTEXT;

typedef struct _PH_ROUTE_CHANGE_PARAMETER
{
    NSI_PARAMETER_TYPE Type;
    PVOID Data;
    ULONG Length;
    ULONG Offset;
} PH_ROUTE_CHANGE_PARAMETER, *PPH_ROUTE_CHANGE_PARAMETER;

typedef struct _PH_TEREDO_PORT_CHANGE_CONTEXT
{
    PTEREDO_PORT_CHANGE_CALLBACK Callback;
    PVOID CallerContext;
} PH_TEREDO_PORT_CHANGE_CONTEXT, *PPH_TEREDO_PORT_CHANGE_CONTEXT;

typedef struct _PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION
{
    ULONG Tag;
    HANDLE NsiNotificationHandle;
    PVOID CallerContext;
    PINTERFACE_TIMESTAMP_CONFIG_CHANGE_CALLBACK Callback;
} PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, *PPH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION;

// Matches the NETIO_CHANGE_CALLBACK_CONTEXT + MIB_IPINTERFACE_ROW pairs of NETIO_IP_INTERFACE_CHANGE_REGISTRATION.
typedef struct _PH_IP_INTERFACE_CHANGE_CONTEXT
{
    NETIO_CHANGE_CALLBACK_CONTEXT Callback;
    MIB_IPINTERFACE_ROW Row;
} PH_IP_INTERFACE_CHANGE_CONTEXT, *PPH_IP_INTERFACE_CHANGE_CONTEXT;

NTSTATUS PhNetioGetAddress(
    _In_ NSI_STORE Store,
    _In_ PH_NETIO_ADDRESS_TYPE AddressType,
    _Inout_ PVOID Row
    );

NTSTATUS PhNetioEnumerateAddress(
    _In_ NSI_STORE Store,
    _In_ ADDRESS_FAMILY Family,
    _In_ PH_NETIO_ADDRESS_TYPE AddressType,
    _Outptr_ PVOID *Table
    );

NTSTATUS PhGetIfTable2(
    _Outptr_ PMIB_IF_TABLE2 *Table
    );

NTSTATUS PhGetTcpTable2(
    _Out_writes_bytes_opt_(*SizePointer) PMIB_TCPTABLE2 TcpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order
    );

NTSTATUS PhGetIpInterfaceEntry(
    _Inout_ PMIB_IPINTERFACE_ROW Row
    );

NTSTATUS PhGetIpInterfaceTable(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_IPINTERFACE_TABLE *Table
    );

NTSTATUS PhGetIpNetTable2(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_IPNET_TABLE2 *Table
    );

NTSTATUS PhGetIpNetEntry2(
    _Inout_ PMIB_IPNET_ROW2 Row
    );

NTSTATUS PhGetIpForwardTable2(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_IPFORWARD_TABLE2 *Table
    );

NTSTATUS PhGetIpForwardEntry2(
    _Inout_ PMIB_IPFORWARD_ROW2 Row
    );

NTSTATUS PhGetBestInterfaceEx(
    _In_ struct sockaddr *DestinationAddress,
    _Out_ PULONG BestIfIndex
    );

NTSTATUS PhGetIpStatisticsEx(
    _Out_ PMIB_IPSTATS Statistics,
    _In_ ULONG Family
    );

NTSTATUS PhGetTcpStatisticsEx(
    _Out_ PMIB_TCPSTATS Statistics,
    _In_ ULONG Family
    );

NTSTATUS PhGetUdpStatisticsEx(
    _Out_ PMIB_UDPSTATS Statistics,
    _In_ ULONG Family
    );

MIB_IPNET_TYPE PhpNeighborStateToMibState(
    _In_ NL_NEIGHBOR_STATE State
    );

VOID PhIcmpDestroyEchoRequestContext(
    _Inout_ ICMP_ECHO_REQUEST_CONTEXT* Context
    );

VOID PhPrintTcpTableStatus(
    _In_ ULONG Status,
    _In_ PCWSTR Action
);

BOOLEAN PhTcpipGuidEquals(
    _In_ const GUID* Left,
    _In_ const GUID* Right
);

BOOLEAN PhpIsIpv4Interface(
    _In_ NET_LUID *InterfaceLuid,
    _In_ NET_IFTYPE InterfaceType
    );

NTSTATUS PhNsiGetParameter(
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_ NSI_PARAMETER_TYPE ParameterType,
    _Out_writes_bytes_opt_(ParameterBufferSize) PVOID ParameterBuffer,
    _In_ ULONG ParameterBufferSize
    );

NTSTATUS PhNsiSetAllParameters(
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(ReadWriteBufferSize) PVOID ReadWriteBuffer,
    _In_ ULONG ReadWriteBufferSize
    );

NTSTATUS PhNsiAllocateAndGetPersistentDataWithMaskTable(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * KeyEntrySize) PVOID* KeyTable,
    _In_ ULONG KeyEntrySize,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * ParameterDataEntrySize) PVOID* ParameterDataTable,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * ParameterDataEntrySize) PVOID* ParameterMaskTable,
    _In_ ULONG ParameterDataEntrySize,
    _Out_ PULONG Count
    );

NTSTATUS PhNsiDeregisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
    );

NTSTATUS PhNsiCancelChangeNotification(
    _In_opt_ HANDLE NotificationHandle
    );

NTSTATUS PhNsiDeregisterChangeNotificationEx(
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    );

NTSTATUS PhNsiEnumerateObjectsAllParameters(
    _In_ NSI_STORE Store,
    _In_ NSI_GET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _Inout_updates_bytes_opt_(*Count * KeyEntrySize) PVOID KeyTable,
    _In_ ULONG KeyEntrySize,
    _Inout_updates_bytes_opt_(*Count * ReadWriteEntrySize) PVOID ReadWriteTable,
    _In_ ULONG ReadWriteEntrySize,
    _Inout_updates_bytes_opt_(*Count * DynamicEntrySize) PVOID DynamicTable,
    _In_ ULONG DynamicEntrySize,
    _Inout_updates_bytes_opt_(*Count * StaticEntrySize) PVOID StaticTable,
    _In_ ULONG StaticEntrySize,
    _Inout_ PULONG Count
    );

NTSTATUS PhNsiEnumerateObjectsAllParametersEx(
    _Inout_ PNSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX Request
    );

NTSTATUS PhNsiEnumerateObjectsAllPersistentParametersWithMask(
    _In_ NSI_GET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _Out_writes_bytes_opt_(*Count * KeyEntrySize) PVOID KeyTable,
    _In_ ULONG KeyEntrySize,
    _Out_writes_bytes_opt_(*Count * ParameterDataEntrySize) PVOID ParameterDataTable,
    _Out_writes_bytes_opt_(*Count * ParameterDataEntrySize) PVOID ParameterMaskTable,
    _In_ ULONG ParameterDataEntrySize,
    _Inout_ PULONG Count
    );

NTSTATUS PhNsiGetParameterEx(
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST Request
    );

NTSTATUS PhNsiGetAllParameters(
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _Out_writes_bytes_opt_(ReadWriteBufferSize) PVOID ReadWriteBuffer,
    _In_ ULONG ReadWriteBufferSize,
    _Out_writes_bytes_opt_(DynamicBufferSize) PVOID DynamicBuffer,
    _In_ ULONG DynamicBufferSize,
    _Out_writes_bytes_opt_(StaticBufferSize) PVOID StaticBuffer,
    _In_ ULONG StaticBufferSize
    );

NTSTATUS PhNsiGetAllParametersEx(
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST Request
    );

NTSTATUS PhNsiSetParameter(
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_ NSI_PARAMETER_TYPE ParameterType,
    _In_reads_bytes_opt_(ParameterBufferSize) PVOID ParameterBuffer,
    _In_ ULONG ParameterBufferSize,
    _In_ ULONG ParameterOffset
    );

NTSTATUS PhNsiSetParameterEx(
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST Request
    );

NTSTATUS PhNsiSetAllParametersEx(
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST Request
    );

NTSTATUS PhNsiRegisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ PNSI_CHANGE_CALLBACK ChangeCallback,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Outptr_result_maybenull_ PHANDLE NotificationHandle
    );

NTSTATUS PhNsiRegisterChangeNotificationEx(
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    );

NTSTATUS PhNsiRequestChangeNotification(
    _In_ ULONG ModuleHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ LPOVERLAPPED Overlapped,
    _Out_ PHANDLE NotificationHandle
    );

NTSTATUS PhNsiRequestChangeNotificationEx(
    _Inout_ PNSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST Request
    );

NTSTATUS PhNsiGetAllPersistentParametersWithMask(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _Out_writes_bytes_opt_(ParameterDataSize) PVOID ParameterData,
    _Out_writes_bytes_opt_(ParameterDataSize) PVOID ParameterMask,
    _In_ ULONG ParameterDataSize
    );

NTSTATUS PhNsiSetAllPersistentParametersWithMask(
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(ParameterDataSize) PVOID ParameterData,
    _In_reads_bytes_opt_(ParameterDataSize) PVOID ParameterMask,
    _In_ ULONG ParameterDataSize
    );

NTSTATUS PhNsiGetObjectSecurity(
    _Inout_ PNSI_OBJECT_SECURITY_REQUEST Request
    );

NTSTATUS PhNsiSetObjectSecurity(
    _Inout_ PNSI_OBJECT_SECURITY_REQUEST Request
    );

VOID PhNsiDumpObjectSecurityDescriptors(
    VOID
    );

NTSTATUS PhNsiConnectToServer(
    _Reserved_ PVOID Reserved,
    _Out_ RPC_BINDING_HANDLE* RpcHandle
    );

NTSTATUS PhNsiDisconnectFromServer(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle
    );

VOID PhNsiFreePersistentDataWithMaskTable(
    _Frees_ptr_opt_ PVOID KeyTable,
    _Frees_ptr_opt_ PVOID ParameterDataTable,
    _Frees_ptr_opt_ PVOID ParameterMaskTable
    );

NTSTATUS PhNsiRpcDeregisterChangeNotification(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
    );

NTSTATUS PhNsiRpcDeregisterChangeNotificationEx(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    );

NTSTATUS PhNsiRpcEnumerateObjectsAllParameters(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _In_ NSI_STORE Store,
    _In_ NSI_GET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _Inout_updates_bytes_opt_(*Count * KeyEntrySize) PVOID KeyTable,
    _In_ ULONG KeyEntrySize,
    _Inout_updates_bytes_opt_(*Count * ReadWriteEntrySize) PVOID ReadWriteTable,
    _In_ ULONG ReadWriteEntrySize,
    _Inout_updates_bytes_opt_(*Count * DynamicEntrySize) PVOID DynamicTable,
    _In_ ULONG DynamicEntrySize,
    _Inout_updates_bytes_opt_(*Count * StaticEntrySize) PVOID StaticTable,
    _In_ ULONG StaticEntrySize,
    _Inout_ PULONG Count
    );

NTSTATUS PhNsiRpcGetAllParameters(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _Out_writes_bytes_opt_(ReadWriteBufferSize) PVOID ReadWriteBuffer,
    _In_ ULONG ReadWriteBufferSize,
    _Out_writes_bytes_opt_(DynamicBufferSize) PVOID DynamicBuffer,
    _In_ ULONG DynamicBufferSize,
    _Out_writes_bytes_opt_(StaticBufferSize) PVOID StaticBuffer,
    _In_ ULONG StaticBufferSize
    );

NTSTATUS PhNsiRpcGetAllParametersEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST Request
    );

NTSTATUS PhNsiRpcGetParameter(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_ NSI_PARAMETER_TYPE ParameterType,
    _Out_writes_bytes_opt_(ParameterBufferSize) PVOID ParameterBuffer,
    _In_ ULONG ParameterBufferSize,
    _In_ ULONG ParameterOffset
    );

NTSTATUS PhNsiRpcGetParameterEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST Request
    );

NTSTATUS PhNsiRpcRegisterChangeNotification(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ PNSI_CHANGE_CALLBACK ChangeCallback,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Out_opt_ PHANDLE NotificationHandle
    );

NTSTATUS PhNsiRpcRegisterChangeNotificationEx(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    );

NTSTATUS PhNsiRpcSetAllParameters(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(ReadWriteBufferSize) PVOID ReadWriteBuffer,
    _In_ ULONG ReadWriteBufferSize
    );

NTSTATUS PhNsiRpcSetAllParametersEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST Request
    );

NTSTATUS PhNsiRpcSetParameter(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_ NSI_PARAMETER_TYPE ParameterType,
    _In_reads_bytes_opt_(ParameterBufferSize) PVOID ParameterBuffer,
    _In_ ULONG ParameterBufferSize,
    _In_ ULONG ParameterOffset
    );

NTSTATUS PhNsiRpcSetParameterEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST Request
    );

NTSTATUS PhConvertInterfaceLuidToNameW(
    _In_ const NET_LUID* InterfaceLuid,
    _Out_writes_(Length) PWSTR InterfaceName,
    _In_ SIZE_T Length
);

NTSTATUS PhConvertInterfaceNameToLuidW(
    _In_z_ PCWSTR InterfaceName,
    _Out_ NET_LUID* InterfaceLuid
);

#endif
