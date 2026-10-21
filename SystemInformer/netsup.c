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
#include <ntnsi.h>
#include <mapldr.h>
#include <netprv.h>
#include <ntstrsafe.h>
#include <subprocesstag.h>
#include <limits.h>
#include <netlistmgr.h>

CONST NPI_MODULEID NPI_MS_IPV4_MODULEID = NPI_MS_IPV4_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_IPV6_MODULEID = NPI_MS_IPV6_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_UDP_MODULEID = NPI_MS_UDP_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_TCP_MODULEID = NPI_MS_TCP_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_NMR_MODULEID = NPI_MS_NMR_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_TUN_MODULEID = NPI_MS_TUN_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_FL4L_MODULEID = NPI_MS_FL4L_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_FL6L_MODULEID = NPI_MS_FL6L_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_EQOS_MODULEID = NPI_MS_EQOS_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_WANARPV4_MODULEID = NPI_MS_WANARPV4_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_FL48_MODULEID = NPI_MS_FL48_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_FL68_MODULEID = NPI_MS_FL68_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_WANARPV6_MODULEID = NPI_MS_WANARPV6_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_FL_WANARP_MODULEID = NPI_MS_FL_WANARP_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_TCPIP_MODULEID = NPI_MS_TCPIP_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_NDIS_MODULEID = NPI_MS_NDIS_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_FL4T_MODULEID = NPI_MS_FL4T_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_FL6T_MODULEID = NPI_MS_FL6T_MODULEID_INIT; // Teredo
CONST NPI_MODULEID NPI_MS_FLRDMA_MODULEID = NPI_MS_FLRDMA_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_RAW_MODULEID = NPI_MS_RAW_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_WSK_MODULEID = NPI_MS_WSK_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_PACER_MODULEID = NPI_MS_PACER_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_RSS_MODULEID = NPI_MS_RSS_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_EQOSPROXY_MODULEID = NPI_MS_EQOSPROXY_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_FSE_CLIENT_MODULEID = NPI_MS_FSE_CLIENT_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_IPSEC_MODULEID = NPI_MS_IPSEC_MODULEID_INIT;
CONST NPI_MODULEID NPI_MS_IPS_LB_MODULEID = NPI_MS_IPS_LB_MODULEID_INIT;
CONST NPIID NPI_FRAMING_LAYER_ID = NPI_FRAMING_LAYER_ID_INIT;
CONST NPIID NPI_NETWORK_LAYER_ID = NPI_NETWORK_LAYER_ID_INIT;
CONST NPIID NPI_WSK_INTERFACE_ID = NPI_WSK_INTERFACE_ID_INIT;
CONST NPIID NPI_TRANSPORT_LAYER_ID = NPI_TRANSPORT_LAYER_ID_INIT;
CONST NPIID NPI_NSI_MODULE_ID = NPI_NSI_MODULE_ID_INIT;
CONST NPIID NPI_NL_PNP_EVENT_ACCESS_ID = NPI_NL_PNP_EVENT_ACCESS_ID_INIT;
CONST NPIID NPI_WSK_RDMA_INTERFACE_ID = NPI_WSK_RDMA_INTERFACE_ID_INIT;
CONST NPIID NPI_QOS_INTERFACE_ID = NPI_QOS_INTERFACE_ID_INIT;
CONST NPIID NPI_RSS_CONTROL_INTERFACE_ID = NPI_RSS_CONTROL_INTERFACE_ID_INIT;
CONST NPIID NPI_RSS_HASH_INTERFACE_ID = NPI_RSS_HASH_INTERFACE_ID_INIT;
CONST NPIID NPI_UQOS_INTERFACE_ID = NPI_UQOS_INTERFACE_ID_INIT;
CONST NPIID NPI_IP_SWITCHING_ID = NPI_IP_SWITCHING_ID_INIT;
CONST NPIID NPI_HTTP_KE_INTERFACE_ID = NPI_HTTP_KE_INTERFACE_ID_INIT;
CONST NPIID NPI_PKTMON_INTERFACE_ID = NPI_PKTMON_INTERFACE_ID_INIT;
CONST NPIID NPI_FSE_LOCAL_SERVER_INTERFACE_ID = NPI_FSE_LOCAL_SERVER_INTERFACE_ID_INIT;
CONST NPIID NPI_FL_SWITCHING_ID = NPI_FL_SWITCHING_ID_INIT;

#define TCPIP_ADAPTER_PREFIX L"\\DEVICE\\TCPIP_"
#define TCPIP_ADAPTER_PREFIX_LENGTH (sizeof(TCPIP_ADAPTER_PREFIX) / sizeof(WCHAR) - 1)
#define PH_IPV4_MAPPED_IN6_ADDRESS_OFFSET (FIELD_OFFSET(IN6_ADDR, s6_addr) + RTL_FIELD_SIZE(IN6_ADDR, s6_addr) - sizeof(IN_ADDR))
#define PH_NETIO_IPNET_FAMILY_COUNT 2
#define PHP_ICMP_ECHO_DATA_SIZE 32
#define PHP_ICMP_ECHO_PATTERN_LENGTH 23
#define PHP_ICMP_ECHO_REPLY_BUFFER_SIZE 0x1000
#define PHP_ICMP_ECHO_TIMEOUT 5000
#define PHP_NETIO_RTT_SCALE 1000
#define PHP_NSI_CONNECTIVITY_LOCAL_MASK 0x00000014
#define PHP_NSI_CONNECTIVITY_INTERNET_MASK 0x00000028
#define PHP_NSI_CONNECTIVITY_CONSTRAINED_MASK 0x000000C0
#define PHP_NSI_CONNECTIVITY_COST_SHIFT 32
#define PHP_NSI_CONNECTIVITY_COST_UNDETERMINED 0x04
#define PHP_NSI_CONNECTIVITY_COST_FIXED 0x02
#define PHP_NSI_CONNECTIVITY_COST_UNRESTRICTED 0x01
// WNF state name of the NCSI network connectivity state (8-byte value) watched by iphlpapi!NotifyNetworkConnectivityHintChange.
#define PHP_WNF_CONNECTIVITY_STATE_NAME 0x418B0223A3BC2075ULL
#define PHP_NET_LUID_INDEX_MASK 0x00FFFFFF
#define PHP_ICMP_STATUS_BLOCK_LENGTH 4
#define PHP_ICMP_ECHO_ERROR_TYPE_INDEX 2
#define PHP_ICMP_ECHO_ERROR_CODE_INDEX 3
#define PHP_NSI_INTERFACE_OBJECT_NAME_BUFFER_LENGTH 64
#define PHP_INTERFACE_NAME_BUFFER_LENGTH 24
#define PHP_INTERFACE_NAME_MAX_LENGTH 21
#define PHP_MAC_ADDRESS_LENGTH 6
#define PHP_MAC_ADDRESS_STRING_LENGTH (PHP_MAC_ADDRESS_LENGTH * 3 - 1)
#define PHP_IPV4_NETWORK_ADDRESS_BUFFER_LENGTH 22
#define PHP_IPV6_NETWORK_ADDRESS_BUFFER_LENGTH 65
#define PHP_DNS_UTF8_NAME_BUFFER_LENGTH 512
#define PHP_NSI_ENUMERATION_MAX_ATTEMPTS 3
#define PHP_NSI_ENUMERATION_GROWTH_DIVISOR 8
#define PHP_NSI_ENUMERATION_MIN_GROWTH 10
#define PHP_NSI_OBJECT_SECURITY_POINTER_COUNT 6
#define PHP_CONNECTIVITY_CALLBACK_PROCESSING 2
#define PHP_TIMESTAMP_CONFIG_CHANGE_TAG 0x656D6974 // "time"

typedef enum _PHP_NETIO_ADDRESS_TYPE
{
    PhpNetioAddressUnicast,
    PhpNetioAddressMulticast,
    PhpNetioAddressAnycast
} PHP_NETIO_ADDRESS_TYPE;

typedef enum _PHP_TCP_OWNER_CLASS
{
    PhpTcpOwnerBasic,
    PhpTcpOwnerBasic2,
    PhpTcpOwnerModule,
    PhpTcpOwnerPid
} PHP_TCP_OWNER_CLASS;

// Forward declarations (definitions appear later in this file)
static NTSTATUS PhNetioGetAddress(
    _In_ NSI_STORE Store,
    _In_ PHP_NETIO_ADDRESS_TYPE AddressType,
    _Inout_ PVOID Row
    );
static NTSTATUS PhNetioEnumerateAddress(
    _In_ NSI_STORE Store,
    _In_ ADDRESS_FAMILY Family,
    _In_ PHP_NETIO_ADDRESS_TYPE AddressType,
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
NTSTATUS PhGetExtendedTcpTable(
    _Out_writes_bytes_opt_(*SizePointer) PVOID TcpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order,
    _In_ ULONG Family,
    _In_ TCP_TABLE_CLASS TableClass,
    _In_ ULONG Reserved
    );
NTSTATUS PhGetExtendedUdpTable(
    _Out_writes_bytes_opt_(*SizePointer) PVOID UdpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order,
    _In_ ULONG Family,
    _In_ UDP_TABLE_CLASS TableClass,
    _In_ ULONG Reserved
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
ULONG PhpNeighborStateToMibState(
    _In_ ULONG State
    );

VOID PhIcmpDestroyEchoRequestContext(
    _Inout_ ICMP_ECHO_REQUEST_CONTEXT* Context
    );

VOID PhPrintTcpTableStatus(
    _In_ ULONG Status,
    _In_ PCWSTR Action
);

typedef struct _TCPIP_IF_TYPE_STRING_ENTRY
{
    PCWSTR Name;
    USHORT IfType;
} TCPIP_IF_TYPE_STRING_ENTRY;

static const TCPIP_IF_TYPE_STRING_ENTRY IfTypeStringTable[] =
{
    { L"atm", IF_TYPE_ATM },
    { L"ethernet", IF_TYPE_ETHERNET_CSMACD },
    { L"ieee1394", IF_TYPE_IEEE1394 },
    { L"loopback", IF_TYPE_SOFTWARE_LOOPBACK },
    { L"other", IF_TYPE_OTHER },
    { L"ppp", IF_TYPE_PPP },
    { L"tokenring", IF_TYPE_ISO88025_TOKENRING },
    { L"tunnel", IF_TYPE_TUNNEL },
    { L"wireless", IF_TYPE_IEEE80211 }
};

// Defines the loopback address (::1) locally for each file that includes this header
DECLSPEC_SELECTANY CONST IN6_ADDR in6addr_loopback = { {{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }} };

static const ULONG Ipv4Mask[sizeof(IN_ADDR) * CHAR_BIT + 1] =
{
    0x00000000,
    0x00000080,
    0x000000C0,
    0x000000E0,
    0x000000F0,
    0x000000F8,
    0x000000FC,
    0x000000FE,
    0x000000FF,
    0x000080FF,
    0x0000C0FF,
    0x0000E0FF,
    0x0000F0FF,
    0x0000F8FF,
    0x0000FCFF,
    0x0000FEFF,
    0x0000FFFF,
    0x0080FFFF,
    0x00C0FFFF,
    0x00E0FFFF,
    0x00F0FFFF,
    0x00F8FFFF,
    0x00FCFFFF,
    0x00FEFFFF,
    0x00FFFFFF,
    0x80FFFFFF,
    0xC0FFFFFF,
    0xE0FFFFFF,
    0xF0FFFFFF,
    0xF8FFFFFF,
    0xFCFFFFFF,
    0xFEFFFFFF,
    0xFFFFFFFF
};

static PH_QUEUED_LOCK NotificationCriticalSection = PH_QUEUED_LOCK_INIT;
static PH_INITONCE NotificationInitOnce = PH_INITONCE_INIT;
static PVOID NotificationRpcHandle;

typedef struct _PHP_MIB_NOTIFICATION_HANDLE
{
    PVOID Reserved;
    NSI_ICMP_OBJECT_ID ObjectId;
    PVOID NotificationHandleV4;
    PVOID NotificationHandleV6;
    PVOID NotificationHandleTeredo;
    PVOID NotificationHandleNdis;
    PVOID ConnectivityHintContext;
} PHP_MIB_NOTIFICATION_HANDLE, * PPHP_MIB_NOTIFICATION_HANDLE;

typedef struct _PHP_CONNECTIVITY_HINT_CONTEXT
{
    LIST_ENTRY ListEntry;
    PVOID Callback;
    PVOID CallerContext;
    PTP_WORK ThreadpoolWork;
    LONG State;
} PHP_CONNECTIVITY_HINT_CONTEXT, * PPHP_CONNECTIVITY_HINT_CONTEXT;

static PH_QUEUED_LOCK PhpConnectivityHintLock = PH_QUEUED_LOCK_INIT;
static LIST_ENTRY PhpConnectivityHintListHead;
static PVOID PhpConnectivitySubscriptionHandle;
static ULONG PhpConnectivityLevel;
static ULONG PhpConnectivityCost;

static LONG PingNotificationCountV4;
static LONG PingNotificationCountV6;
static HANDLE PingNotificationHandleV4;
static HANDLE PingNotificationHandleV6;
static PH_QUEUED_LOCK PingNotificationLock = PH_QUEUED_LOCK_INIT;

static BOOLEAN PhpIsIpv4Interface(
    _In_ NET_LUID* InterfaceLuid,
    _In_ USHORT MediaType
);

BOOLEAN PhTcpipGuidEquals(
    _In_ const GUID* Left,
    _In_ const GUID* Right
);


/**
 * Converts a Win32 error code returned by nsi.dll, winnsi.dll or other IP Helper components into an NTSTATUS value.
 *
 * \param DosError The Win32 error code.
 * \return The NTSTATUS value. ERROR_MORE_DATA maps to STATUS_BUFFER_OVERFLOW (not the informational
 * STATUS_MORE_ENTRIES) so that NT_SUCCESS() fails and callers can retry with a larger buffer.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-rtlntstatustodoserror
 */
NTSTATUS PhpNetioDosErrorToNtStatus(
    _In_ ULONG DosError
    )
{
    switch (DosError)
    {
    case ERROR_SUCCESS:
        return STATUS_SUCCESS;
    case ERROR_MORE_DATA:
        return STATUS_BUFFER_OVERFLOW;
    case ERROR_BUFFER_OVERFLOW:
        return STATUS_BUFFER_TOO_SMALL;
    case ERROR_IO_PENDING:
        return STATUS_PENDING;
    case ERROR_NO_DATA:
        return STATUS_NO_MORE_ENTRIES;
    case ERROR_GEN_FAILURE:
        return STATUS_UNSUCCESSFUL;
    case ERROR_ARITHMETIC_OVERFLOW:
        return STATUS_INTEGER_OVERFLOW;
    case ERROR_INVALID_USER_BUFFER:
        return STATUS_INVALID_USER_BUFFER;
    case ERROR_OUTOFMEMORY:
        return STATUS_NO_MEMORY;
    }

    return PhDosErrorToNtStatus(DosError);
}

#pragma region NSI Wrappers

NTSTATUS PhNsiGetParameter(
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_ NSI_PARAMETER_TYPE ParameterType,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    );

NTSTATUS PhNsiSetAllParameters(
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(RwSize) PVOID Rw,
    _In_ ULONG RwSize
    );

NTSTATUS PhNsiAllocateAndGetTable(
    _In_ CONST NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _Outptr_result_bytebuffer_maybenull_(*Count * KeyEntrySize) _Out_opt_ PVOID* KeyTable,
    _In_ CONST ULONG KeyEntrySize,
    _Outptr_result_bytebuffer_maybenull_(*Count * RwEntrySize) _Out_opt_ PVOID* RwTable,
    _In_ CONST ULONG RwEntrySize,
    _Outptr_result_bytebuffer_maybenull_(*Count * RodEntrySize) _Out_opt_ PVOID* RodTable,
    _In_ CONST ULONG RodEntrySize,
    _Outptr_result_bytebuffer_maybenull_(*Count * RosEntrySize) _Out_opt_ PVOID* RosTable,
    _In_ CONST ULONG RosEntrySize,
    _Out_ PULONG Count,
    _In_ ULONG Unused
    );

NTSTATUS PhNsiAllocateAndGetPersistentDataWithMaskTable(
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _Outptr_result_bytebuffer_maybenull_(*Count * KeyEntrySize) _Out_opt_ PVOID* KeyTable,
    _In_ ULONG KeyEntrySize,
    _Outptr_result_bytebuffer_maybenull_(*Count * ParameterDataEntrySize) _Out_opt_ PVOID* ParameterDataTable,
    _Outptr_result_bytebuffer_maybenull_(*Count * ParameterDataEntrySize) _Out_opt_ PVOID* ParameterMaskTable,
    _In_ ULONG ParameterDataEntrySize,
    _Out_ PULONG Count
    );

NTSTATUS PhNsiDeregisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
    );

NTSTATUS PhNsiCancelChangeNotification(
    _In_opt_ HANDLE NotificationHandle
    );

NTSTATUS PhNsiDeregisterChangeNotificationEx(
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST DeregistrationRequest
    );

NTSTATUS PhNsiEnumerateObjectsAllParameters(
    _In_ CONST NSI_STORE Store,
    _In_ NSI_GET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _Inout_updates_bytes_opt_(*Count * KeyEntrySize) PVOID KeyTable,
    _In_ CONST ULONG KeyEntrySize,
    _Inout_updates_bytes_opt_(*Count * RwEntrySize) PVOID RwTable,
    _In_ CONST ULONG RwEntrySize,
    _Inout_updates_bytes_opt_(*Count * RodEntrySize) PVOID RodTable,
    _In_ CONST ULONG RodEntrySize,
    _Inout_updates_bytes_opt_(*Count * RosEntrySize) PVOID RosTable,
    _In_ CONST ULONG RosEntrySize,
    _Inout_ PULONG Count
    );

NTSTATUS PhNsiEnumerateObjectsAllParametersEx(
    _Inout_ PNSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX EnumerateRequest
    );

NTSTATUS PhNsiEnumerateObjectsAllPersistentParametersWithMask(
    _In_ NSI_GET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _Out_writes_bytes_opt_(*Count * KeyEntrySize) PVOID KeyTable,
    _In_ CONST ULONG KeyEntrySize,
    _Out_writes_bytes_opt_(*Count * ParameterDataEntrySize) PVOID ParameterDataTable,
    _Out_writes_bytes_opt_(*Count * ParameterDataEntrySize) PVOID ParameterMaskTable,
    _In_ CONST ULONG ParameterDataEntrySize,
    _Inout_ PULONG Count
    );

NTSTATUS PhNsiGetParameterEx(
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST GetParameterRequest
    );

NTSTATUS PhNsiGetAllParameters(
    _In_ CONST NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ CONST ULONG KeySize,
    _Out_writes_bytes_opt_(RwSize) PVOID Rw,
    _In_ CONST ULONG RwSize,
    _Out_writes_bytes_opt_(RodSize) PVOID Rod,
    _In_ CONST ULONG RodSize,
    _Out_writes_bytes_opt_(RosSize) PVOID Ros,
    _In_ CONST ULONG RosSize
    );

NTSTATUS PhNsiGetAllParametersEx(
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST GetAllParametersRequest
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
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST SetParameterRequest
    );

NTSTATUS PhNsiSetAllParametersEx(
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST SetAllParametersRequest
    );

NTSTATUS PhNsiRegisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _In_opt_ PVOID ChangeCallback,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Outptr_result_maybenull_ PHANDLE NotificationHandle
    );

NTSTATUS PhNsiRegisterChangeNotificationEx(
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST RegistrationRequest
    );

NTSTATUS PhNsiRequestChangeNotification(
    _In_ ULONG ModuleHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ ULONG ObjectIndex,
    _In_opt_ LPOVERLAPPED OverLapped,
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
    _Inout_updates_bytes_(6 * sizeof(PVOID)) PVOID ObjectSecurity
    );

NTSTATUS PhNsiSetObjectSecurity(
    _Inout_updates_bytes_(6 * sizeof(PVOID)) PVOID ObjectSecurity
    );

PVOID PhNsiConnectToServer(
    _Reserved_ PVOID Reserved
    );

NTSTATUS PhNsiDisconnectFromServer(
    _In_opt_ PVOID RpcHandle
    );

VOID PhNsiFreePersistentDataWithMaskTable(
    _Frees_ptr_opt_ PVOID KeyTable,
    _Frees_ptr_opt_ PVOID ParameterDataTable,
    _Frees_ptr_opt_ PVOID ParameterMaskTable
    );

VOID PhNsiFreeTable(
    _Frees_ptr_opt_ PVOID KeyTable,
    _Frees_ptr_opt_ PVOID RwTable,
    _Frees_ptr_opt_ PVOID RodTable,
    _Frees_ptr_opt_ PVOID RosTable
    );

NTSTATUS PhNsiRpcDeregisterChangeNotification(
    _In_ HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
    );

NTSTATUS PhNsiRpcDeregisterChangeNotificationEx(
    _In_ HANDLE RpcHandle,
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST DeregistrationRequest
    );

NTSTATUS PhNsiRpcEnumerateObjectsAllParameters(
    _In_opt_ HANDLE RpcHandle,
    _In_ CONST NSI_STORE Store,
    _In_ NSI_GET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _Inout_updates_bytes_opt_(*Count * KeyEntrySize) PVOID KeyTable,
    _In_ CONST ULONG KeyEntrySize,
    _Inout_updates_bytes_opt_(*Count * RwEntrySize) PVOID RwTable,
    _In_ CONST ULONG RwEntrySize,
    _Inout_updates_bytes_opt_(*Count * RodEntrySize) PVOID RodTable,
    _In_ CONST ULONG RodEntrySize,
    _Inout_updates_bytes_opt_(*Count * RosEntrySize) PVOID RosTable,
    _In_ CONST ULONG RosEntrySize,
    _Inout_ PULONG Count
    );

NTSTATUS PhNsiRpcGetAllParameters(
    _In_opt_ HANDLE RpcHandle,
    _In_ CONST NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ CONST ULONG KeySize,
    _Out_writes_bytes_opt_(RwSize) PVOID Rw,
    _In_ CONST ULONG RwSize,
    _Out_writes_bytes_opt_(RodSize) PVOID Rod,
    _In_ CONST ULONG RodSize,
    _Out_writes_bytes_opt_(RosSize) PVOID Ros,
    _In_ CONST ULONG RosSize
    );

NTSTATUS PhNsiRpcGetAllParametersEx(
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST GetAllParametersRequest
    );

NTSTATUS PhNsiRpcGetParameter(
    _In_opt_ HANDLE RpcHandle,
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_ NSI_PARAMETER_TYPE ParameterType,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_ ULONG ParameterOffset
    );

NTSTATUS PhNsiRpcGetParameterEx(
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST GetParameterRequest
    );

NTSTATUS PhNsiRpcRegisterChangeNotification(
    _In_ HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ PVOID CallbackRoutine,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Out_opt_ HANDLE *NotificationHandle
    );

NTSTATUS PhNsiRpcRegisterChangeNotificationEx(
    _In_ HANDLE RpcHandle,
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST RegistrationRequest
    );

NTSTATUS PhNsiRpcSetAllParameters(
    _In_opt_ HANDLE RpcHandle,
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(RwSize) PVOID Rw,
    _In_ ULONG RwSize
    );

NTSTATUS PhNsiRpcSetAllParametersEx(
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST SetAllParametersRequest
    );

NTSTATUS PhNsiRpcSetParameter(
    _In_opt_ HANDLE RpcHandle,
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
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST SetParameterRequest
    );

static typeof(&NsiGetParameter) NsiGetParameter_I;
static typeof(&NsiSetAllParameters) NsiSetAllParameters_I;
static typeof(&NsiAllocateAndGetTable) NsiAllocateAndGetTable_I;
static typeof(&NsiAllocateAndGetPersistentDataWithMaskTable) NsiAllocateAndGetPersistentDataWithMaskTable_I;
static typeof(&NsiDeregisterChangeNotification) NsiDeregisterChangeNotification_I;
static typeof(&NsiCancelChangeNotification) NsiCancelChangeNotification_I;
static typeof(&NsiDeregisterChangeNotificationEx) NsiDeregisterChangeNotificationEx_I;
static typeof(&NsiEnumerateObjectsAllParameters) NsiEnumerateObjectsAllParameters_I;
static typeof(&NsiEnumerateObjectsAllParametersEx) NsiEnumerateObjectsAllParametersEx_I;
static typeof(&NsiEnumerateObjectsAllPersistentParametersWithMask) NsiEnumerateObjectsAllPersistentParametersWithMask_I;
static typeof(&NsiGetParameterEx) NsiGetParameterEx_I;
static typeof(&NsiGetAllParameters) NsiGetAllParameters_I;
static typeof(&NsiGetAllParametersEx) NsiGetAllParametersEx_I;
static typeof(&NsiSetParameter) NsiSetParameter_I;
static typeof(&NsiSetParameterEx) NsiSetParameterEx_I;
static typeof(&NsiSetAllParametersEx) NsiSetAllParametersEx_I;
static typeof(&NsiRegisterChangeNotification) NsiRegisterChangeNotification_I;
static typeof(&NsiRegisterChangeNotificationEx) NsiRegisterChangeNotificationEx_I;
static typeof(&NsiRequestChangeNotification) NsiRequestChangeNotification_I;
static typeof(&NsiRequestChangeNotificationEx) NsiRequestChangeNotificationEx_I;
static typeof(&NsiGetAllPersistentParametersWithMask) NsiGetAllPersistentParametersWithMask_I;
static typeof(&NsiSetAllPersistentParametersWithMask) NsiSetAllPersistentParametersWithMask_I;
static typeof(&NsiGetObjectSecurity) NsiGetObjectSecurity_I;
static typeof(&NsiSetObjectSecurity) NsiSetObjectSecurity_I;
static typeof(&NsiConnectToServer) NsiConnectToServer_I;
static typeof(&NsiDisconnectFromServer) NsiDisconnectFromServer_I;
static typeof(&NsiFreePersistentDataWithMaskTable) NsiFreePersistentDataWithMaskTable_I;
static typeof(&NsiFreeTable) NsiFreeTable_I;
static typeof(&NsiRpcDeregisterChangeNotification) NsiRpcDeregisterChangeNotification_I;
static typeof(&NsiRpcDeregisterChangeNotificationEx) NsiRpcDeregisterChangeNotificationEx_I;
static typeof(&NsiRpcEnumerateObjectsAllParameters) NsiRpcEnumerateObjectsAllParameters_I;
static typeof(&NsiRpcGetAllParameters) NsiRpcGetAllParameters_I;
static typeof(&NsiRpcGetAllParametersEx) NsiRpcGetAllParametersEx_I;
static typeof(&NsiRpcGetParameter) NsiRpcGetParameter_I;
static typeof(&NsiRpcGetParameterEx) NsiRpcGetParameterEx_I;
static typeof(&NsiRpcRegisterChangeNotification) NsiRpcRegisterChangeNotification_I;
static typeof(&NsiRpcRegisterChangeNotificationEx) NsiRpcRegisterChangeNotificationEx_I;
static typeof(&NsiRpcSetAllParameters) NsiRpcSetAllParameters_I;
static typeof(&NsiRpcSetAllParametersEx) NsiRpcSetAllParametersEx_I;
static typeof(&NsiRpcSetParameter) NsiRpcSetParameter_I;
static typeof(&NsiRpcSetParameterEx) NsiRpcSetParameterEx_I;

/**
 * Resolves the optional nsi.dll exports used by the RPC-based NSI wrappers. The lookup runs once.
 *
 * \return TRUE if nsi.dll was loaded, otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
static BOOLEAN PhNsiApiInitialized(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PVOID baseAddress;

    if (PhBeginInitOnce(&initOnce))
    {
#if defined(PH_INLINE_NSI)
        if (baseAddress = PhLoadLibrary(L"nsi.dll"))
        {
            NsiGetParameter_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiGetParameter", 0);
            NsiSetAllParameters_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiSetAllParameters", 0);
            NsiAllocateAndGetTable_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiAllocateAndGetTable", 0);
            NsiAllocateAndGetPersistentDataWithMaskTable_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiAllocateAndGetPersistentDataWithMaskTable", 0);
            NsiDeregisterChangeNotification_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiDeregisterChangeNotification", 0);
            NsiCancelChangeNotification_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiCancelChangeNotification", 0);
            NsiDeregisterChangeNotificationEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiDeregisterChangeNotificationEx", 0);
            NsiEnumerateObjectsAllParameters_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiEnumerateObjectsAllParameters", 0);
            NsiEnumerateObjectsAllParametersEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiEnumerateObjectsAllParametersEx", 0);
            NsiEnumerateObjectsAllPersistentParametersWithMask_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiEnumerateObjectsAllPersistentParametersWithMask", 0);
            NsiGetParameterEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiGetParameterEx", 0);
            NsiGetAllParameters_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiGetAllParameters", 0);
            NsiGetAllParametersEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiGetAllParametersEx", 0);
            NsiSetParameter_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiSetParameter", 0);
            NsiSetParameterEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiSetParameterEx", 0);
            NsiSetAllParametersEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiSetAllParametersEx", 0);
            NsiRegisterChangeNotification_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRegisterChangeNotification", 0);
            NsiRegisterChangeNotificationEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRegisterChangeNotificationEx", 0);
            NsiRequestChangeNotification_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRequestChangeNotification", 0);
            NsiRequestChangeNotificationEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRequestChangeNotificationEx", 0);
            NsiGetAllPersistentParametersWithMask_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiGetAllPersistentParametersWithMask", 0);
            NsiSetAllPersistentParametersWithMask_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiSetAllPersistentParametersWithMask", 0);
            NsiGetObjectSecurity_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiGetObjectSecurity", 0);
            NsiSetObjectSecurity_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiSetObjectSecurity", 0);
            NsiFreePersistentDataWithMaskTable_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiFreePersistentDataWithMaskTable", 0);
            NsiFreeTable_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiFreeTable", 0);
        }
#endif

        if (baseAddress = PhLoadLibrary(L"winnsi.dll"))
        {
            NsiConnectToServer_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiConnectToServer", 0);
            NsiDisconnectFromServer_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiDisconnectFromServer", 0);
            NsiRpcDeregisterChangeNotification_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcDeregisterChangeNotification", 0);
            NsiRpcDeregisterChangeNotificationEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcDeregisterChangeNotificationEx", 0);
            NsiRpcEnumerateObjectsAllParameters_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcEnumerateObjectsAllParameters", 0);
            NsiRpcGetAllParameters_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcGetAllParameters", 0);
            NsiRpcGetAllParametersEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcGetAllParametersEx", 0);
            NsiRpcGetParameter_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcGetParameter", 0);
            NsiRpcGetParameterEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcGetParameterEx", 0);
            NsiRpcRegisterChangeNotification_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcRegisterChangeNotification", 0);
            NsiRpcRegisterChangeNotificationEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcRegisterChangeNotificationEx", 0);
            NsiRpcSetAllParameters_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcSetAllParameters", 0);
            NsiRpcSetAllParametersEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcSetAllParametersEx", 0);
            NsiRpcSetParameter_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcSetParameter", 0);
            NsiRpcSetParameterEx_I = PhGetDllBaseProcedureAddress(baseAddress, "NsiRpcSetParameterEx", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    return TRUE;
}

/**
 * Retrieves a single NSI parameter block for an object (nsi!NsiGetParameter).
 *
 * \param Store The NSI store to query (active or persistent).
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param ParameterType The parameter block to access (read-write, read-only dynamic or read-only static).
 * \param Buffer The buffer that receives the parameter data.
 * \param BufferLength The size, in bytes, of Buffer.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntdeviceiocontrolfile
 */
NTSTATUS PhNsiGetParameter(
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_ NSI_PARAMETER_TYPE ParameterType,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
#if !defined(PH_INLINE_NSI)
    NTSTATUS status;
    HANDLE deviceHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    NSI_GET_PARAMETER_EX_REQUEST nsiGetParameter;

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

    RtlZeroMemory(&nsiGetParameter, sizeof(NSI_GET_PARAMETER_EX_REQUEST));
    nsiGetParameter.Header.ModuleId = ModuleId;
    nsiGetParameter.Header.ObjectId = ObjectId;
    nsiGetParameter.Header.Store = Store;
    nsiGetParameter.Key = Key;
    nsiGetParameter.KeySize = KeySize;
    nsiGetParameter.ParameterType = ParameterType;
    nsiGetParameter.Buffer = Buffer;
    nsiGetParameter.BufferLength = BufferLength;
    nsiGetParameter.ParameterOffset = 0;

    status = NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        IOCTL_NSI_GET_PARAMETER,
        &nsiGetParameter,
        sizeof(NSI_GET_PARAMETER_EX_REQUEST),
        &nsiGetParameter,
        sizeof(NSI_GET_PARAMETER_EX_REQUEST)
        );

    NtClose(deviceHandle);

    return status;
#else
    if (!PhNsiApiInitialized() || !NsiGetParameter_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiGetParameter_I(
        Store,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        ParameterType,
        Buffer,
        BufferLength,
        0
        ));
#endif
}

/**
 * Sets the read-write parameters of an NSI object (nsi!NsiSetAllParameters).
 *
 * \param Store The NSI store to query (active or persistent).
 * \param Action The get or set action to perform.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param Rw The read-write parameter block.
 * \param RwSize The size, in bytes, of the read-write parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntdeviceiocontrolfile
 */
NTSTATUS PhNsiSetAllParameters(
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(RwSize) PVOID Rw,
    _In_ ULONG RwSize
    )
{
#if !defined(PH_INLINE_NSI)
    NTSTATUS status;
    HANDLE deviceHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    NSI_SET_ALL_PARAMETERS_EX_REQUEST nsiSetAllParameters;

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

    RtlZeroMemory(&nsiSetAllParameters, sizeof(NSI_SET_ALL_PARAMETERS_EX_REQUEST));
    nsiSetAllParameters.Header.ModuleId = ModuleId;
    nsiSetAllParameters.Header.ObjectId = ObjectId;
    nsiSetAllParameters.Header.Store = Store;
    nsiSetAllParameters.Header.SetAction = Action;
    nsiSetAllParameters.Key = Key;
    nsiSetAllParameters.KeySize = KeySize;
    nsiSetAllParameters.ReadWriteBuffer = Rw;
    nsiSetAllParameters.ReadWriteBufferSize = RwSize;

    status = NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        IOCTL_NSI_SET_ALL_PARAMETERS,
        &nsiSetAllParameters,
        sizeof(NSI_SET_ALL_PARAMETERS_EX_REQUEST),
        &nsiSetAllParameters,
        sizeof(NSI_SET_ALL_PARAMETERS_EX_REQUEST)
        );

    NtClose(deviceHandle);

    return status;
#else
    if (!PhNsiApiInitialized() || !NsiSetAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiSetAllParameters_I(
        Store,
        Action,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        Rw,
        RwSize
        ));
#endif
}

#if !defined(PH_INLINE_NSI)

/**
 * Sends a synchronous IOCTL to the \\Device\\Nsi device.
 *
 * \param ControlCode The IOCTL_NSI_* control code.
 * \param InputBuffer The request buffer.
 * \param InputLength The size, in bytes, of InputBuffer.
 * \param OutputBuffer The buffer that receives the output.
 * \param OutputLength The size, in bytes, of OutputBuffer.
 * \param ReturnedLength Receives the number of bytes returned.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntdeviceiocontrolfile
 */
static NTSTATUS PhpNsiDeviceIoControl(
    _In_ ULONG ControlCode,
    _In_reads_bytes_(InputLength) PVOID InputBuffer,
    _In_ ULONG InputLength,
    _Out_writes_bytes_opt_(OutputLength) PVOID OutputBuffer,
    _In_ ULONG OutputLength,
    _Out_opt_ PULONG ReturnedLength
    )
{
    NTSTATUS status;
    HANDLE deviceHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    RtlInitUnicodeString(&objectName, NSI_DEVICE_NAME);
    InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

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

    status = NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        ControlCode,
        InputBuffer,
        InputLength,
        OutputBuffer,
        OutputLength
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(deviceHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = ioStatusBlock.Status;
    }

    NtClose(deviceHandle);

    if (ReturnedLength && (NT_SUCCESS(status) || status == STATUS_MORE_ENTRIES))
        *ReturnedLength = (ULONG)ioStatusBlock.Information;

    // STATUS_MORE_ENTRIES is informational; report it as a warning so NT_SUCCESS() fails and callers can retry.
    return status == STATUS_MORE_ENTRIES ? STATUS_BUFFER_OVERFLOW : NT_SUCCESS(status) ? STATUS_SUCCESS : status;
}


/**
 * Allocates a zeroed table buffer for an NSI enumeration, checking for overflow.
 *
 * \param Count The number of entries.
 * \param EntrySize The size, in bytes, of each entry. A zero size returns a NULL buffer.
 * \param Buffer Receives the allocated buffer, or NULL.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
static NTSTATUS PhpNsiAllocateTableBuffer(
    _In_ ULONG Count,
    _In_ ULONG EntrySize,
    _Out_ PVOID *Buffer
    )
{
    SIZE_T size;

    *Buffer = NULL;

    if (!EntrySize)
        return STATUS_SUCCESS;

    size = (SIZE_T)Count * EntrySize;

    if (Count && size / Count != EntrySize)
        return STATUS_INTEGER_OVERFLOW;

    *Buffer = PhAllocateZero(size);

    return STATUS_SUCCESS;
}

static HANDLE PhpNsiAsyncDeviceHandle = INVALID_HANDLE_VALUE;

/**
 * Returns a cached overlapped handle to \\Device\\Nsi used for change notifications, opening it on first use.
 *
 * \return The device handle, or INVALID_HANDLE_VALUE on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntdeviceiocontrolfile
 */
static HANDLE PhpNsiGetAsyncDeviceHandle(
    VOID
    )
{
    HANDLE deviceHandle;
    HANDLE previousHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING deviceName;
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;

    deviceHandle = InterlockedCompareExchangePointer(
        (PVOID *)&PhpNsiAsyncDeviceHandle,
        INVALID_HANDLE_VALUE,
        INVALID_HANDLE_VALUE
        );

    if (deviceHandle != INVALID_HANDLE_VALUE)
        return deviceHandle;

    RtlInitUnicodeString(&deviceName, L"\\Device\\Nsi");
    InitializeObjectAttributes(&objectAttributes, &deviceName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtCreateFile(
        &deviceHandle,
        0,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        0,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
    {
        PhSetLastError(PhNtStatusToDosError(status));
        return INVALID_HANDLE_VALUE;
    }

    previousHandle = InterlockedCompareExchangePointer(
        (PVOID *)&PhpNsiAsyncDeviceHandle,
        deviceHandle,
        INVALID_HANDLE_VALUE
        );

    if (previousHandle != INVALID_HANDLE_VALUE)
    {
        NtClose(deviceHandle);
        deviceHandle = previousHandle;
    }

    return deviceHandle;
}

/**
 * Sends an IOCTL to the asynchronous NSI device handle and waits for it to complete.
 *
 * \param DeviceHandle The overlapped NSI device handle.
 * \param ControlCode The IOCTL_NSI_* control code.
 * \param InputBuffer The request buffer, also used for output.
 * \param InputLength The size, in bytes, of InputBuffer.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntdeviceiocontrolfile
 */
static NTSTATUS PhpNsiAsyncDeviceIoControl(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG ControlCode,
    _In_reads_bytes_(InputLength) PVOID InputBuffer,
    _In_ ULONG InputLength
    )
{
    HANDLE eventHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;

    status = PhCreateEvent(
        &eventHandle,
        EVENT_ALL_ACCESS,
        SynchronizationEvent,
        FALSE
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtDeviceIoControlFile(
        DeviceHandle,
        eventHandle,
        NULL,
        NULL,
        &ioStatusBlock,
        ControlCode,
        InputBuffer,
        InputLength,
        NULL,
        0
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(eventHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
        {
            status = ioStatusBlock.Status;
        }
    }

    NtClose(eventHandle);

    // STATUS_MORE_ENTRIES is informational; report it as a warning so NT_SUCCESS() fails and callers can retry.
    return status == STATUS_MORE_ENTRIES ? STATUS_BUFFER_OVERFLOW : NT_SUCCESS(status) ? STATUS_SUCCESS : status;
}

#endif

// NSI entry points use nsi.dll with PH_INLINE_NSI and direct device requests otherwise.
// The existing PhNsiGetParameter and PhNsiSetAllParameters wrappers remain above.

/**
 * Enumerates all instances of an NSI object and allocates the key and parameter tables (nsi!NsiAllocateAndGetTable). The enumeration is retried when the table grows between calls.
 *
 * \param Store The NSI store to query (active or persistent).
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param KeyTable Receives the allocated key table.
 * \param KeyEntrySize The size, in bytes, of each key entry.
 * \param RwTable Receives the allocated read-write table.
 * \param RwEntrySize The size, in bytes, of each read-write entry.
 * \param RodTable Receives the allocated read-only dynamic table.
 * \param RodEntrySize The size, in bytes, of each read-only dynamic entry.
 * \param RosTable Receives the allocated read-only static table.
 * \param RosEntrySize The size, in bytes, of each read-only static entry.
 * \param Count Receives the number of entries.
 * \param Unused Unused; pass zero.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \remarks Free the tables with PhNsiFreeTable.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiAllocateAndGetTable(
    _In_ CONST NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _Outptr_result_bytebuffer_maybenull_(*Count * KeyEntrySize) _Out_opt_ PVOID* KeyTable,
    _In_ CONST ULONG KeyEntrySize,
    _Outptr_result_bytebuffer_maybenull_(*Count * RwEntrySize) _Out_opt_ PVOID* RwTable,
    _In_ CONST ULONG RwEntrySize,
    _Outptr_result_bytebuffer_maybenull_(*Count * RodEntrySize) _Out_opt_ PVOID* RodTable,
    _In_ CONST ULONG RodEntrySize,
    _Outptr_result_bytebuffer_maybenull_(*Count * RosEntrySize) _Out_opt_ PVOID* RosTable,
    _In_ CONST ULONG RosEntrySize,
    _Out_ PULONG Count,
    _In_ ULONG Unused
    )
{
#if !defined(PH_INLINE_NSI)
    NTSTATUS result;
    ULONG capacity;
    ULONG count;
    ULONG attempt;
    PVOID keyBuffer;
    PVOID rwBuffer;
    PVOID rodBuffer;
    PVOID rosBuffer;

    for (attempt = 0; attempt < PHP_NSI_ENUMERATION_MAX_ATTEMPTS; attempt++)
    {
        count = 0;
        result = PhNsiEnumerateObjectsAllParameters(Store, NsiGetFirst, ModuleId, ObjectId,
            NULL, KeyEntrySize, NULL, RwEntrySize, NULL, RodEntrySize, NULL, RosEntrySize, &count);
        if (!NT_SUCCESS(result) && result != STATUS_BUFFER_OVERFLOW)
            return result;

        capacity = count + max(count / PHP_NSI_ENUMERATION_GROWTH_DIVISOR, PHP_NSI_ENUMERATION_MIN_GROWTH);
        if (capacity < count)
            return STATUS_INTEGER_OVERFLOW;

        keyBuffer = NULL;
        rwBuffer = NULL;
        rodBuffer = NULL;
        rosBuffer = NULL;
        result = KeyTable ? PhpNsiAllocateTableBuffer(capacity, KeyEntrySize, &keyBuffer) : STATUS_SUCCESS;
        if (NT_SUCCESS(result) && RwTable)
            result = PhpNsiAllocateTableBuffer(capacity, RwEntrySize, &rwBuffer);
        if (NT_SUCCESS(result) && RodTable)
            result = PhpNsiAllocateTableBuffer(capacity, RodEntrySize, &rodBuffer);
        if (NT_SUCCESS(result) && RosTable)
            result = PhpNsiAllocateTableBuffer(capacity, RosEntrySize, &rosBuffer);
        if (!NT_SUCCESS(result))
        {
            PhNsiFreeTable(keyBuffer, rwBuffer, rodBuffer, rosBuffer);
            return result;
        }

        count = capacity;
        result = PhNsiEnumerateObjectsAllParameters(Store, NsiGetFirst, ModuleId, ObjectId,
            keyBuffer, KeyEntrySize, rwBuffer, RwEntrySize, rodBuffer, RodEntrySize,
            rosBuffer, RosEntrySize, &count);
        if (result == STATUS_BUFFER_OVERFLOW && attempt + 1 < PHP_NSI_ENUMERATION_MAX_ATTEMPTS)
        {
            PhNsiFreeTable(keyBuffer, rwBuffer, rodBuffer, rosBuffer);
            continue;
        }
        if (!NT_SUCCESS(result))
        {
            PhNsiFreeTable(keyBuffer, rwBuffer, rodBuffer, rosBuffer);
            return result;
        }

        if (KeyTable && keyBuffer) *KeyTable = keyBuffer;
        if (RwTable && rwBuffer) *RwTable = rwBuffer;
        if (RodTable && rodBuffer) *RodTable = rodBuffer;
        if (RosTable && rosBuffer) *RosTable = rosBuffer;
        *Count = count;
        return STATUS_SUCCESS;
    }

    return STATUS_BUFFER_OVERFLOW;
#else
    if (!PhNsiApiInitialized() || !NsiAllocateAndGetTable_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiAllocateAndGetTable_I(
        Store,
        ModuleId,
        ObjectId,
        KeyTable,
        KeyEntrySize,
        RwTable,
        RwEntrySize,
        RodTable,
        RodEntrySize,
        RosTable,
        RosEntrySize,
        Count,
        Unused
        ));
#endif
}

/**
 * Enumerates the persistent store of an NSI object and allocates the key, data and mask tables (nsi!NsiAllocateAndGetPersistentDataWithMaskTable).
 *
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param KeyTable Receives the allocated key table.
 * \param KeyEntrySize The size, in bytes, of each key entry.
 * \param ParameterDataTable Receives the allocated parameter data table.
 * \param ParameterMaskTable Receives the allocated parameter mask table.
 * \param ParameterDataEntrySize The size, in bytes, of each parameter data and mask entry.
 * \param Count Receives the number of entries.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \remarks Free the tables with PhNsiFreePersistentDataWithMaskTable.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiAllocateAndGetPersistentDataWithMaskTable(
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _Outptr_result_bytebuffer_maybenull_(*Count* KeyEntrySize) _Out_opt_ PVOID* KeyTable,
    _In_ ULONG KeyEntrySize,
    _Outptr_result_bytebuffer_maybenull_(*Count* ParameterDataEntrySize) _Out_opt_ PVOID* ParameterDataTable,
    _Outptr_result_bytebuffer_maybenull_(*Count* ParameterDataEntrySize) _Out_opt_ PVOID* ParameterMaskTable,
    _In_ ULONG ParameterDataEntrySize,
    _Out_ PULONG Count
)
{
#if !defined(PH_INLINE_NSI)
    NTSTATUS result;
    ULONG capacity;
    ULONG count;
    ULONG attempt;
    PVOID keyBuffer;
    PVOID dataBuffer;
    PVOID maskBuffer;

    for (attempt = 0; attempt < PHP_NSI_ENUMERATION_MAX_ATTEMPTS; attempt++)
    {
        count = 0;
        result = PhNsiEnumerateObjectsAllPersistentParametersWithMask(
            NsiGetFirst,
            ModuleId,
            ObjectId,
            NULL,
            KeyEntrySize,
            NULL,
            NULL,
            ParameterDataEntrySize,
            &count
            );

        if (!NT_SUCCESS(result) && result != STATUS_BUFFER_OVERFLOW)
            return result;

        capacity = count + max(count / PHP_NSI_ENUMERATION_GROWTH_DIVISOR, PHP_NSI_ENUMERATION_MIN_GROWTH);
        if (capacity < count)
            return STATUS_INTEGER_OVERFLOW;

        keyBuffer = NULL;
        dataBuffer = NULL;
        maskBuffer = NULL;

        result = KeyTable ? PhpNsiAllocateTableBuffer(capacity, KeyEntrySize, &keyBuffer) : STATUS_SUCCESS;
        if (NT_SUCCESS(result) && ParameterDataTable)
            result = PhpNsiAllocateTableBuffer(capacity, ParameterDataEntrySize, &dataBuffer);
        if (NT_SUCCESS(result) && ParameterMaskTable)
            result = PhpNsiAllocateTableBuffer(capacity, ParameterDataEntrySize, &maskBuffer);
        if (!NT_SUCCESS(result))
        {
            PhNsiFreePersistentDataWithMaskTable(keyBuffer, dataBuffer, maskBuffer);
            return result;
        }

        count = capacity;
        result = PhNsiEnumerateObjectsAllPersistentParametersWithMask(
            NsiGetFirst,
            ModuleId,
            ObjectId,
            keyBuffer,
            KeyEntrySize,
            dataBuffer,
            maskBuffer,
            ParameterDataEntrySize,
            &count
            );

        if (result == STATUS_BUFFER_OVERFLOW && attempt + 1 < PHP_NSI_ENUMERATION_MAX_ATTEMPTS)
        {
            PhNsiFreePersistentDataWithMaskTable(keyBuffer, dataBuffer, maskBuffer);
            continue;
        }

        if (!NT_SUCCESS(result))
        {
            PhNsiFreePersistentDataWithMaskTable(keyBuffer, dataBuffer, maskBuffer);
            return result;
        }

        if (KeyTable && keyBuffer) *KeyTable = keyBuffer;
        if (ParameterDataTable && dataBuffer) *ParameterDataTable = dataBuffer;
        if (ParameterMaskTable && maskBuffer) *ParameterMaskTable = maskBuffer;
        *Count = count;
        return STATUS_SUCCESS;
    }

    return STATUS_BUFFER_OVERFLOW;
#else
    if (!PhNsiApiInitialized() || !NsiAllocateAndGetPersistentDataWithMaskTable_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiAllocateAndGetPersistentDataWithMaskTable_I(
        ModuleId,
        ObjectId,
        KeyTable,
        KeyEntrySize,
        ParameterDataTable,
        ParameterMaskTable,
        ParameterDataEntrySize,
        Count
    ));
#endif
}

/**
 * Removes a change notification registered with PhNsiRegisterChangeNotification (nsi!NsiDeregisterChangeNotification).
 *
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param NotificationHandle The notification handle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiDeregisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
)
{
#if !defined(PH_INLINE_NSI)
    NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST request = { 0 };

    request.ModuleId = ModuleId;
    request.ObjectId = ObjectId;
    request.NotificationHandle = NotificationHandle;

    return PhNsiDeregisterChangeNotificationEx(&request);
#else
    if (!PhNsiApiInitialized() || !NsiDeregisterChangeNotification_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiDeregisterChangeNotification_I(
        ModuleId,
        ObjectId,
        NotificationHandle
        ));
#endif
}

/**
 * Cancels a pending change notification request (nsi!NsiCancelChangeNotification).
 *
 * \param NotificationHandle The OVERLAPPED pointer used for the pending request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-cancelipchangenotify
 */
NTSTATUS PhNsiCancelChangeNotification(
    _In_opt_ HANDLE NotificationHandle
    )
{
#if !defined(PH_INLINE_NSI)
    HANDLE deviceHandle;

    deviceHandle = PhpNsiGetAsyncDeviceHandle();

    if (deviceHandle == INVALID_HANDLE_VALUE)
        return PhpNetioDosErrorToNtStatus(PhGetLastError());

    return PhpNsiAsyncDeviceIoControl(
        deviceHandle,
        IOCTL_NSI_CANCEL_CHANGE_NOTIFICATION,
        &NotificationHandle,
        sizeof(NotificationHandle)
        );
#else
    if (!PhNsiApiInitialized() || !NsiCancelChangeNotification_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiCancelChangeNotification_I(
        NotificationHandle
        ));
#endif
}

/**
 * Removes a change notification using a prepared request (nsi!NsiDeregisterChangeNotificationEx).
 *
 * \param DeregistrationRequest The deregistration request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiDeregisterChangeNotificationEx(
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST DeregistrationRequest
    )
{
#if !defined(PH_INLINE_NSI)
    return PhpNsiDeviceIoControl(
        IOCTL_NSI_DEREGISTER_CHANGE_NOTIFICATION_EX,
        DeregistrationRequest,
        sizeof(*DeregistrationRequest),
        DeregistrationRequest,
        sizeof(*DeregistrationRequest),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiDeregisterChangeNotificationEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiDeregisterChangeNotificationEx_I(
        DeregistrationRequest
        ));
#endif
}

/**
 * Enumerates the instances of an NSI object into caller-supplied tables (nsi!NsiEnumerateObjectsAllParameters).
 *
 * \param Store The NSI store to query (active or persistent).
 * \param Action The get or set action to perform.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param KeyTable The table of keys.
 * \param KeyEntrySize The size, in bytes, of each key entry.
 * \param RwTable The table of read-write parameter blocks.
 * \param RwEntrySize The size, in bytes, of each read-write entry.
 * \param RodTable The table of read-only dynamic parameter blocks.
 * \param RodEntrySize The size, in bytes, of each read-only dynamic entry.
 * \param RosTable The table of read-only static parameter blocks.
 * \param RosEntrySize The size, in bytes, of each read-only static entry.
 * \param Count On input, the capacity of the tables in entries; on output, the number of entries returned.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiEnumerateObjectsAllParameters(
    _In_ CONST NSI_STORE Store,
    _In_ NSI_GET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _Inout_updates_bytes_opt_(*Count * KeyEntrySize) PVOID KeyTable,
    _In_ CONST ULONG KeyEntrySize,
    _Inout_updates_bytes_opt_(*Count * RwEntrySize) PVOID RwTable,
    _In_ CONST ULONG RwEntrySize,
    _Inout_updates_bytes_opt_(*Count * RodEntrySize) PVOID RodTable,
    _In_ CONST ULONG RodEntrySize,
    _Inout_updates_bytes_opt_(*Count * RosEntrySize) PVOID RosTable,
    _In_ CONST ULONG RosEntrySize,
    _Inout_ PULONG Count
    )
{
#if !defined(PH_INLINE_NSI)
    NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX request = { 0 };
        NTSTATUS result;
    
        request.ModuleId = ModuleId;
        request.ObjectId = ObjectId;
        request.Store = Store;
        request.Action = Action;
        request.KeyTable = KeyTable;
        request.KeyEntrySize = KeyEntrySize;
        request.ReadWriteTable = RwTable;
        request.ReadWriteEntrySize = RwEntrySize;
        request.DynamicTable = RodTable;
        request.DynamicEntrySize = RodEntrySize;
        request.StaticTable = RosTable;
        request.StaticEntrySize = RosEntrySize;
        request.Count = *Count;
        result = PhNsiEnumerateObjectsAllParametersEx(&request);
        *Count = request.Count;
        return result;
#else
    if (!PhNsiApiInitialized() || !NsiEnumerateObjectsAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiEnumerateObjectsAllParameters_I(
        Store,
        Action,
        ModuleId,
        ObjectId,
        KeyTable,
        KeyEntrySize,
        RwTable,
        RwEntrySize,
        RodTable,
        RodEntrySize,
        RosTable,
        RosEntrySize,
        Count
        ));
#endif
}

/**
 * Enumerates the instances of an NSI object using a prepared request (nsi!NsiEnumerateObjectsAllParametersEx).
 *
 * \param EnumerateRequest The enumeration request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiEnumerateObjectsAllParametersEx(
    _Inout_ PNSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX EnumerateRequest
    )
{
#if !defined(PH_INLINE_NSI)
    return PhpNsiDeviceIoControl(
        IOCTL_NSI_GET_ALL_PARAMETERS,
        EnumerateRequest,
        sizeof(*EnumerateRequest),
        EnumerateRequest,
        sizeof(*EnumerateRequest),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiEnumerateObjectsAllParametersEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiEnumerateObjectsAllParametersEx_I(
        EnumerateRequest
        ));
#endif
}

/**
 * Enumerates the persistent instances of an NSI object into caller-supplied tables (nsi!NsiEnumerateObjectsAllPersistentParametersWithMask).
 *
 * \param Action The get or set action to perform.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param KeyTable The table of keys.
 * \param KeyEntrySize The size, in bytes, of each key entry.
 * \param ParameterDataTable The table of persistent parameter data.
 * \param ParameterMaskTable The table of persistent parameter masks.
 * \param ParameterDataEntrySize The size, in bytes, of each parameter data and mask entry.
 * \param Count On input, the capacity of the tables in entries; on output, the number of entries returned.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiEnumerateObjectsAllPersistentParametersWithMask(
    _In_ NSI_GET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _Out_writes_bytes_opt_(*Count* KeyEntrySize) PVOID KeyTable,
    _In_ CONST ULONG KeyEntrySize,
    _Out_writes_bytes_opt_(*Count* ParameterDataEntrySize) PVOID ParameterDataTable,
    _Out_writes_bytes_opt_(*Count* ParameterDataEntrySize) PVOID ParameterMaskTable,
    _In_ CONST ULONG ParameterDataEntrySize,
    _Inout_ PULONG Count
)
{
#if !defined(PH_INLINE_NSI)
    NSI_PERSISTENT_ENUMERATE_REQUEST request = { 0 };
    NTSTATUS result;

    request.ModuleId = ModuleId;
    request.ObjectId = ObjectId;
    request.Action = Action;
    request.KeyTable = KeyTable;
    request.KeyEntrySize = KeyEntrySize;
    request.ParameterDataTable = ParameterDataTable;
    request.ParameterMaskTable = ParameterMaskTable;
    request.ParameterDataEntrySize = ParameterDataEntrySize;
    request.Count = *Count;

    result = PhpNsiDeviceIoControl(
        IOCTL_NSI_ENUMERATE_OBJECTS_ALL_PERSISTENT_PARAMETERS_WITH_MASK,
        &request,
        sizeof(request),
        &request,
        sizeof(request),
        NULL
        );

    *Count = request.Count;

    return result;
#else
    if (!PhNsiApiInitialized() || !NsiEnumerateObjectsAllPersistentParametersWithMask_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiEnumerateObjectsAllPersistentParametersWithMask_I(
        Action,
        ModuleId,
        ObjectId,
        KeyTable,
        KeyEntrySize,
        ParameterDataTable,
        ParameterMaskTable,
        ParameterDataEntrySize,
        Count
    ));
#endif
}

/**
 * Retrieves a single NSI parameter using a prepared request (nsi!NsiGetParameterEx).
 *
 * \param GetParameterRequest The get parameter request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiGetParameterEx(
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST GetParameterRequest
    )
{
#if !defined(PH_INLINE_NSI)
    return PhpNsiDeviceIoControl(
        IOCTL_NSI_GET_PARAMETER,
        GetParameterRequest,
        sizeof(*GetParameterRequest),
        GetParameterRequest,
        GetParameterRequest->BufferLength,
        &GetParameterRequest->BufferLength
        );
#else
    if (!PhNsiApiInitialized() || !NsiGetParameterEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiGetParameterEx_I(
        GetParameterRequest
        ));
#endif
}

/**
 * Retrieves all parameter blocks of an NSI object instance (nsi!NsiGetAllParameters).
 *
 * \param Store The NSI store to query (active or persistent).
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param Rw The read-write parameter block.
 * \param RwSize The size, in bytes, of the read-write parameter block.
 * \param Rod The read-only dynamic parameter block.
 * \param RodSize The size, in bytes, of the read-only dynamic parameter block.
 * \param Ros The read-only static parameter block.
 * \param RosSize The size, in bytes, of the read-only static parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiGetAllParameters(
    _In_ CONST NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ CONST ULONG KeySize,
    _Out_writes_bytes_opt_(RwSize) PVOID Rw,
    _In_ CONST ULONG RwSize,
    _Out_writes_bytes_opt_(RodSize) PVOID Rod,
    _In_ CONST ULONG RodSize,
    _Out_writes_bytes_opt_(RosSize) PVOID Ros,
    _In_ CONST ULONG RosSize
    )
{
#if !defined(PH_INLINE_NSI)
    NSI_GET_ALL_PARAMETERS_EX_REQUEST request = { 0 };
    NTSTATUS result;

    request.Header.ModuleId = ModuleId;
    request.Header.ObjectId = ObjectId;
    request.Header.Store = Store;
    request.Key = Key;
    request.KeySize = KeySize;
    request.ReadWriteBuffer = Rw;
    request.ReadWriteBufferSize = RwSize;
    request.DynamicBuffer = Rod;
    request.DynamicBufferSize = RodSize;
    request.StaticBuffer = Ros;
    request.StaticBufferSize = RosSize;
    result = PhNsiGetAllParametersEx(&request);
    if (NT_SUCCESS(result) && request.ReadWriteBufferSize < RwSize)
        return STATUS_INVALID_PARAMETER;
    return result;
#else
    if (!PhNsiApiInitialized() || !NsiGetAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiGetAllParameters_I(
        Store,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        Rw,
        RwSize,
        Rod,
        RodSize,
        Ros,
        RosSize
        ));
#endif
}

/**
 * Retrieves all parameter blocks of an NSI object instance using a prepared request (nsi!NsiGetAllParametersEx).
 *
 * \param GetAllParametersRequest The get all parameters request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiGetAllParametersEx(
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST GetAllParametersRequest
    )
{
#if !defined(PH_INLINE_NSI)
    return PhpNsiDeviceIoControl(
        IOCTL_NSI_GET_ALL_PARAMETERS_EX,
        GetAllParametersRequest,
        sizeof(*GetAllParametersRequest),
        GetAllParametersRequest,
        sizeof(*GetAllParametersRequest),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiGetAllParametersEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiGetAllParametersEx_I(
        GetAllParametersRequest
        ));
#endif
}

/**
 * Sets a single NSI parameter of an object instance (nsi!NsiSetParameter).
 *
 * \param Store The NSI store to query (active or persistent).
 * \param Action The get or set action to perform.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param ParameterType The parameter block to access (read-write, read-only dynamic or read-only static).
 * \param ParameterBuffer The buffer that contains the parameter data.
 * \param ParameterBufferSize The size, in bytes, of ParameterBuffer.
 * \param ParameterOffset The offset, in bytes, of the parameter within the parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
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
    )
{
#if !defined(PH_INLINE_NSI)
    NSI_SET_PARAMETER_EX_REQUEST request = { 0 };

    request.Header.ModuleId = ModuleId;
    request.Header.ObjectId = ObjectId;
    request.Header.Store = Store;
    request.Header.SetAction = Action;
    request.Key = Key;
    request.KeySize = KeySize;
    request.ParameterType = ParameterType;
    request.ParameterBuffer = ParameterBuffer;
    request.ParameterBufferSize = ParameterBufferSize;
    request.ParameterOffset = ParameterOffset;
    return PhNsiSetParameterEx(&request);
#else
    if (!PhNsiApiInitialized() || !NsiSetParameter_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiSetParameter_I(
        Store,
        Action,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        ParameterType,
        ParameterBuffer,
        ParameterBufferSize,
        ParameterOffset
        ));
#endif
}

/**
 * Sets a single NSI parameter using a prepared request (nsi!NsiSetParameterEx).
 *
 * \param SetParameterRequest The set parameter request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiSetParameterEx(
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST SetParameterRequest
    )
{
#if !defined(PH_INLINE_NSI)
    return PhpNsiDeviceIoControl(
        IOCTL_NSI_SET_PARAMETER,
        SetParameterRequest,
        sizeof(*SetParameterRequest),
        SetParameterRequest,
        SetParameterRequest->ParameterBufferSize,
        &SetParameterRequest->ParameterBufferSize
        );
#else
    if (!PhNsiApiInitialized() || !NsiSetParameterEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiSetParameterEx_I(
        SetParameterRequest
        ));
#endif
}

/**
 * Sets the read-write parameters of an NSI object using a prepared request (nsi!NsiSetAllParametersEx).
 *
 * \param SetAllParametersRequest The set all parameters request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiSetAllParametersEx(
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST SetAllParametersRequest
    )
{
#if !defined(PH_INLINE_NSI)
    return PhpNsiDeviceIoControl(
        IOCTL_NSI_SET_ALL_PARAMETERS,
        SetAllParametersRequest,
        sizeof(*SetAllParametersRequest),
        SetAllParametersRequest,
        sizeof(*SetAllParametersRequest),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiSetAllParametersEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiSetAllParametersEx_I(
        SetAllParametersRequest
        ));
#endif
}

/**
 * Registers for change notifications on an NSI object (nsi!NsiRegisterChangeNotification).
 *
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param ChangeCallback The callback invoked when the object changes.
 * \param NotifyCurrent TRUE to invoke the callback immediately for the current state.
 * \param CallerContext A caller-defined context passed to the callback.
 * \param NotificationHandle Receives the notification handle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRegisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _In_opt_ PVOID ChangeCallback,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Outptr_result_maybenull_ PHANDLE NotificationHandle
    )
{
#if !defined(PH_INLINE_NSI)
    NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST request = { 0 };

    request.ModuleId = ModuleId;
    request.ObjectId = ObjectId;
    request.ChangeCallback = ChangeCallback;
    request.NotifyCurrent = NotifyCurrent;
    request.CallerContext = CallerContext;
    request.NotificationHandle = (PVOID *)NotificationHandle;
    return PhNsiRegisterChangeNotificationEx(&request);
#else
    if (!PhNsiApiInitialized() || !NsiRegisterChangeNotification_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRegisterChangeNotification_I(
        ModuleId,
        ObjectId,
        ChangeCallback,
        NotifyCurrent,
        CallerContext,
        NotificationHandle
        ));
#endif
}

/**
 * Registers for change notifications using a prepared request (nsi!NsiRegisterChangeNotificationEx). Callback-based registrations are not supported over the device interface.
 *
 * \param RegistrationRequest The registration request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRegisterChangeNotificationEx(
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST RegistrationRequest
    )
{
#if !defined(PH_INLINE_NSI)
    if (RegistrationRequest->ChangeCallback)
        return STATUS_INVALID_PARAMETER;

    return PhpNsiDeviceIoControl(
        IOCTL_NSI_REGISTER_CHANGE_NOTIFICATION_EX,
        RegistrationRequest,
        sizeof(*RegistrationRequest),
        RegistrationRequest,
        sizeof(*RegistrationRequest),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiRegisterChangeNotificationEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRegisterChangeNotificationEx_I(
        RegistrationRequest
        ));
#endif
}

/**
 * Requests a one-shot change notification on an NSI object (nsi!NsiRequestChangeNotification).
 *
 * \param ModuleHandle The module handle; pass zero.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectIndex The NSI object identifier within the module.
 * \param OverLapped An optional OVERLAPPED structure for asynchronous completion.
 * \param NotificationHandle Receives the handle used to wait for the notification.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-notifyaddrchange
 */
NTSTATUS PhNsiRequestChangeNotification(
    _In_ ULONG ModuleHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ ULONG ObjectIndex,
    _In_opt_ LPOVERLAPPED OverLapped,
    _Out_ PHANDLE NotificationHandle
    )
{
#if !defined(PH_INLINE_NSI)
    NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST request = { 0 };

    request.ModuleHandle = ModuleHandle;
    request.ModuleId = ModuleId;
    request.ObjectIndex = ObjectIndex;
    request.Overlapped = OverLapped;
    request.NotificationHandle = NotificationHandle;
    return PhNsiRequestChangeNotificationEx(&request);
#else
    if (!PhNsiApiInitialized() || !NsiRequestChangeNotification_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRequestChangeNotification_I(
        ModuleHandle,
        ModuleId,
        ObjectIndex,
        OverLapped,
        NotificationHandle
        ));
#endif
}

/**
 * Requests a one-shot change notification using a prepared request (nsi!NsiRequestChangeNotificationEx).
 *
 * \param Request The change notification request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRequestChangeNotificationEx(
    _Inout_ PNSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST Request
    )
{
#if !defined(PH_INLINE_NSI)
    HANDLE deviceHandle;
    NTSTATUS result;

    deviceHandle = PhpNsiGetAsyncDeviceHandle();
    if (deviceHandle == INVALID_HANDLE_VALUE)
        return PhpNetioDosErrorToNtStatus(PhGetLastError());

    if (Request->Overlapped)
    {
        Request->Overlapped->Internal = STATUS_PENDING;
        Request->Overlapped->InternalHigh = 0;
        result = NtDeviceIoControlFile(
            deviceHandle,
            Request->Overlapped->hEvent,
            NULL,
            NULL,
            (PIO_STATUS_BLOCK)Request->Overlapped,
            IOCTL_NSI_REQUEST_CHANGE_NOTIFICATION_EX,
            Request,
            sizeof(*Request),
            NULL,
            0
            );
    }
    else
    {
        result = PhpNsiAsyncDeviceIoControl(deviceHandle, IOCTL_NSI_REQUEST_CHANGE_NOTIFICATION_EX, Request, sizeof(*Request));
    }

    if (Request->Overlapped && Request->NotificationHandle)
        *Request->NotificationHandle = deviceHandle;

    return result;
#else
    if (!PhNsiApiInitialized() || !NsiRequestChangeNotificationEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRequestChangeNotificationEx_I(
        Request
        ));
#endif
}

/**
 * Retrieves the persistent parameters and mask of an NSI object instance (nsi!NsiGetAllPersistentParametersWithMask).
 *
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param ParameterData The persistent parameter data.
 * \param ParameterMask The mask that selects the valid fields of ParameterData.
 * \param ParameterDataSize The size, in bytes, of ParameterData and ParameterMask.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiGetAllPersistentParametersWithMask(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _Out_writes_bytes_opt_(ParameterDataSize) PVOID ParameterData,
    _Out_writes_bytes_opt_(ParameterDataSize) PVOID ParameterMask,
    _In_ ULONG ParameterDataSize
    )
{
#if !defined(PH_INLINE_NSI)
    NSI_PERSISTENT_GET_REQUEST request = { 0 };

    request.ModuleId = ModuleId;
    request.ObjectId = ObjectId;
    request.Key = Key;
    request.KeySize = KeySize;
    request.ParameterData = ParameterData;
    request.ParameterMask = ParameterMask;
    request.ParameterDataSize = ParameterDataSize;
    return PhpNsiDeviceIoControl(
        IOCTL_NSI_GET_ALL_PERSISTENT_PARAMETERS_WITH_MASK,
        &request,
        sizeof(request),
        &request,
        sizeof(request),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiGetAllPersistentParametersWithMask_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiGetAllPersistentParametersWithMask_I(
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        ParameterData,
        ParameterMask,
        ParameterDataSize
        ));
#endif
}

/**
 * Sets the persistent parameters of an NSI object instance using a mask (nsi!NsiSetAllPersistentParametersWithMask).
 *
 * \param Action The get or set action to perform.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param ParameterData The persistent parameter data.
 * \param ParameterMask The mask that selects the valid fields of ParameterData.
 * \param ParameterDataSize The size, in bytes, of ParameterData and ParameterMask.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiSetAllPersistentParametersWithMask(
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(ParameterDataSize) PVOID ParameterData,
    _In_reads_bytes_opt_(ParameterDataSize) PVOID ParameterMask,
    _In_ ULONG ParameterDataSize
    )
{
#if !defined(PH_INLINE_NSI)
    NSI_PERSISTENT_SET_REQUEST request = { 0 };

    request.ModuleId = ModuleId;
    request.ObjectId = ObjectId;
    request.Action = Action;
    request.Key = Key;
    request.KeySize = KeySize;
    request.ParameterData = ParameterData;
    request.ParameterMask = ParameterMask;
    request.ParameterDataSize = ParameterDataSize;
    return PhpNsiDeviceIoControl(
        IOCTL_NSI_SET_ALL_PERSISTENT_PARAMETERS_WITH_MASK,
        &request,
        sizeof(request),
        &request,
        sizeof(request),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiSetAllPersistentParametersWithMask_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiSetAllPersistentParametersWithMask_I(
        Action,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        ParameterData,
        ParameterMask,
        ParameterDataSize
        ));
#endif
}

/**
 * Retrieves the security descriptor of an NSI object (nsi!NsiGetObjectSecurity).
 *
 * \param ObjectSecurity The object security request block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiGetObjectSecurity(
    _Inout_updates_bytes_(PHP_NSI_OBJECT_SECURITY_POINTER_COUNT * sizeof(PVOID)) PVOID ObjectSecurity
    )
{
#if !defined(PH_INLINE_NSI)
    return PhpNsiDeviceIoControl(
        IOCTL_NSI_GET_OBJECT_SECURITY,
        ObjectSecurity,
        PHP_NSI_OBJECT_SECURITY_POINTER_COUNT * sizeof(PVOID),
        ObjectSecurity,
        PHP_NSI_OBJECT_SECURITY_POINTER_COUNT * sizeof(PVOID),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiGetObjectSecurity_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiGetObjectSecurity_I(
        ObjectSecurity
        ));
#endif
}

/**
 * Sets the security descriptor of an NSI object (nsi!NsiSetObjectSecurity).
 *
 * \param ObjectSecurity The object security request block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiSetObjectSecurity(
    _Inout_updates_bytes_(PHP_NSI_OBJECT_SECURITY_POINTER_COUNT * sizeof(PVOID)) PVOID ObjectSecurity
    )
{
#if !defined(PH_INLINE_NSI)
    return PhpNsiDeviceIoControl(
        IOCTL_NSI_SET_OBJECT_SECURITY,
        ObjectSecurity,
        PHP_NSI_OBJECT_SECURITY_POINTER_COUNT * sizeof(PVOID),
        ObjectSecurity,
        PHP_NSI_OBJECT_SECURITY_POINTER_COUNT * sizeof(PVOID),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiSetObjectSecurity_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiSetObjectSecurity_I(
        ObjectSecurity
        ));
#endif
}

/**
 * Connects to the NSI RPC server (nsi!NsiConnectToServer).
 *
 * \param Reserved Reserved; pass NULL.
 * \return The RPC binding handle, or NULL on failure.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
PVOID PhNsiConnectToServer(
    _Reserved_ PVOID Reserved
    )
{
    if (!PhNsiApiInitialized() || !NsiConnectToServer_I)
        return NULL;

    return NsiConnectToServer_I(
        Reserved
        );
}

/**
 * Disconnects from the NSI RPC server (nsi!NsiDisconnectFromServer).
 *
 * \param RpcHandle The RPC binding handle returned by PhNsiConnectToServer.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiDisconnectFromServer(
    _In_opt_ PVOID RpcHandle
    )
{
    if (!PhNsiApiInitialized() || !NsiDisconnectFromServer_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiDisconnectFromServer_I(
        RpcHandle
        ));
}

/**
 * Frees the tables returned by PhNsiAllocateAndGetPersistentDataWithMaskTable (nsi!NsiFreePersistentDataWithMaskTable).
 *
 * \param KeyTable The key table.
 * \param ParameterDataTable The parameter data table.
 * \param ParameterMaskTable The parameter mask table.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
VOID PhNsiFreePersistentDataWithMaskTable(
    _Frees_ptr_opt_ PVOID KeyTable,
    _Frees_ptr_opt_ PVOID ParameterDataTable,
    _Frees_ptr_opt_ PVOID ParameterMaskTable
    )
{
#if !defined(PH_INLINE_NSI)
    if (KeyTable) PhFree(KeyTable);
    if (ParameterDataTable) PhFree(ParameterDataTable);
    if (ParameterMaskTable) PhFree(ParameterMaskTable);
#else
    if (!PhNsiApiInitialized() || !NsiFreePersistentDataWithMaskTable_I)
        return;

    NsiFreePersistentDataWithMaskTable_I(
        KeyTable,
        ParameterDataTable,
        ParameterMaskTable
        );
#endif
}

/**
 * Frees the tables returned by PhNsiAllocateAndGetTable (nsi!NsiFreeTable).
 *
 * \param KeyTable The key table.
 * \param RwTable The read-write table.
 * \param RodTable The read-only dynamic table.
 * \param RosTable The read-only static table.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
VOID PhNsiFreeTable(
    _Frees_ptr_opt_ PVOID KeyTable,
    _Frees_ptr_opt_ PVOID RwTable,
    _Frees_ptr_opt_ PVOID RodTable,
    _Frees_ptr_opt_ PVOID RosTable
)
{
#if !defined(PH_INLINE_NSI)
    if (KeyTable) PhFree(KeyTable);
    if (RwTable) PhFree(RwTable);
    if (RodTable) PhFree(RodTable);
    if (RosTable) PhFree(RosTable);
#else
    if (!PhNsiApiInitialized() || !NsiFreeTable_I)
        return;

    NsiFreeTable_I(
        KeyTable,
        RwTable,
        RodTable,
        RosTable
        );
#endif
}

/**
 * Removes an RPC change notification (nsi!NsiRpcDeregisterChangeNotification).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param NotificationHandle The notification handle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcDeregisterChangeNotification(
    _In_ HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcDeregisterChangeNotification_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcDeregisterChangeNotification_I(
        RpcHandle,
        ModuleId,
        ObjectId,
        NotificationHandle
        ));
}

/**
 * Removes an RPC change notification using a prepared request (nsi!NsiRpcDeregisterChangeNotificationEx).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param DeregistrationRequest The deregistration request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcDeregisterChangeNotificationEx(
    _In_ HANDLE RpcHandle,
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST DeregistrationRequest
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcDeregisterChangeNotificationEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcDeregisterChangeNotificationEx_I(
        RpcHandle,
        DeregistrationRequest
        ));
}

/**
 * Enumerates the instances of an NSI object over RPC (nsi!NsiRpcEnumerateObjectsAllParameters).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param Store The NSI store to query (active or persistent).
 * \param Action The get or set action to perform.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param KeyTable The table of keys.
 * \param KeyEntrySize The size, in bytes, of each key entry.
 * \param RwTable The table of read-write parameter blocks.
 * \param RwEntrySize The size, in bytes, of each read-write entry.
 * \param RodTable The table of read-only dynamic parameter blocks.
 * \param RodEntrySize The size, in bytes, of each read-only dynamic entry.
 * \param RosTable The table of read-only static parameter blocks.
 * \param RosEntrySize The size, in bytes, of each read-only static entry.
 * \param Count On input, the capacity of the tables in entries; on output, the number of entries returned.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcEnumerateObjectsAllParameters(
    _In_opt_ HANDLE RpcHandle,
    _In_ CONST NSI_STORE Store,
    _In_ NSI_GET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _Inout_updates_bytes_opt_(*Count * KeyEntrySize) PVOID KeyTable,
    _In_ CONST ULONG KeyEntrySize,
    _Inout_updates_bytes_opt_(*Count * RwEntrySize) PVOID RwTable,
    _In_ CONST ULONG RwEntrySize,
    _Inout_updates_bytes_opt_(*Count * RodEntrySize) PVOID RodTable,
    _In_ CONST ULONG RodEntrySize,
    _Inout_updates_bytes_opt_(*Count * RosEntrySize) PVOID RosTable,
    _In_ CONST ULONG RosEntrySize,
    _Inout_ PULONG Count
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcEnumerateObjectsAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcEnumerateObjectsAllParameters_I(
        RpcHandle,
        Store,
        Action,
        ModuleId,
        ObjectId,
        KeyTable,
        KeyEntrySize,
        RwTable,
        RwEntrySize,
        RodTable,
        RodEntrySize,
        RosTable,
        RosEntrySize,
        Count
        ));
}

/**
 * Retrieves all parameter blocks of an NSI object instance over RPC (nsi!NsiRpcGetAllParameters).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param Store The NSI store to query (active or persistent).
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param Rw The read-write parameter block.
 * \param RwSize The size, in bytes, of the read-write parameter block.
 * \param Rod The read-only dynamic parameter block.
 * \param RodSize The size, in bytes, of the read-only dynamic parameter block.
 * \param Ros The read-only static parameter block.
 * \param RosSize The size, in bytes, of the read-only static parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcGetAllParameters(
    _In_opt_ HANDLE RpcHandle,
    _In_ CONST NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ CONST ULONG KeySize,
    _Out_writes_bytes_opt_(RwSize) PVOID Rw,
    _In_ CONST ULONG RwSize,
    _Out_writes_bytes_opt_(RodSize) PVOID Rod,
    _In_ CONST ULONG RodSize,
    _Out_writes_bytes_opt_(RosSize) PVOID Ros,
    _In_ CONST ULONG RosSize
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcGetAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcGetAllParameters_I(
        RpcHandle,
        Store,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        Rw,
        RwSize,
        Rod,
        RodSize,
        Ros,
        RosSize
        ));
}

/**
 * Retrieves all parameter blocks over RPC using a prepared request (nsi!NsiRpcGetAllParametersEx).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param GetAllParametersRequest The get all parameters request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcGetAllParametersEx(
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST GetAllParametersRequest
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcGetAllParametersEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcGetAllParametersEx_I(
        RpcHandle,
        GetAllParametersRequest
        ));
}

/**
 * Retrieves a single NSI parameter over RPC (nsi!NsiRpcGetParameter).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param Store The NSI store to query (active or persistent).
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param ParameterType The parameter block to access (read-write, read-only dynamic or read-only static).
 * \param Buffer The buffer that receives the parameter data.
 * \param BufferLength The size, in bytes, of Buffer.
 * \param ParameterOffset The offset, in bytes, of the parameter within the parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcGetParameter(
    _In_opt_ HANDLE RpcHandle,
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_ NSI_PARAMETER_TYPE ParameterType,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_ ULONG ParameterOffset
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcGetParameter_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcGetParameter_I(
        RpcHandle,
        Store,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        ParameterType,
        Buffer,
        BufferLength,
        ParameterOffset
        ));
}

/**
 * Retrieves a single NSI parameter over RPC using a prepared request (nsi!NsiRpcGetParameterEx).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param GetParameterRequest The get parameter request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcGetParameterEx(
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST GetParameterRequest
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcGetParameterEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcGetParameterEx_I(
        RpcHandle,
        GetParameterRequest
        ));
}

/**
 * Registers a callback for NSI object changes over RPC (nsi!NsiRpcRegisterChangeNotification).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param CallbackRoutine The callback invoked when the object changes.
 * \param NotifyCurrent TRUE to invoke the callback immediately for the current state.
 * \param CallerContext A caller-defined context passed to the callback.
 * \param NotificationHandle Receives the notification handle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcRegisterChangeNotification(
    _In_ HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ PVOID CallbackRoutine,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Out_opt_ HANDLE *NotificationHandle
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcRegisterChangeNotification_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcRegisterChangeNotification_I(
        RpcHandle,
        ModuleId,
        ObjectId,
        CallbackRoutine,
        NotifyCurrent,
        CallerContext,
        NotificationHandle
        ));
}

/**
 * Registers a callback for NSI object changes over RPC using a prepared request (nsi!NsiRpcRegisterChangeNotificationEx).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param RegistrationRequest The registration request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcRegisterChangeNotificationEx(
    _In_ HANDLE RpcHandle,
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST RegistrationRequest
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcRegisterChangeNotificationEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcRegisterChangeNotificationEx_I(
        RpcHandle,
        RegistrationRequest
        ));
}

/**
 * Sets the read-write parameters of an NSI object over RPC (nsi!NsiRpcSetAllParameters).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param Store The NSI store to query (active or persistent).
 * \param Action The get or set action to perform.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param Rw The read-write parameter block.
 * \param RwSize The size, in bytes, of the read-write parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcSetAllParameters(
    _In_opt_ HANDLE RpcHandle,
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(RwSize) PVOID Rw,
    _In_ ULONG RwSize
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcSetAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcSetAllParameters_I(
        RpcHandle,
        Store,
        Action,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        Rw,
        RwSize
        ));
}

/**
 * Sets the read-write parameters over RPC using a prepared request (nsi!NsiRpcSetAllParametersEx).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param SetAllParametersRequest The set all parameters request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcSetAllParametersEx(
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST SetAllParametersRequest
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcSetAllParametersEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcSetAllParametersEx_I(
        RpcHandle,
        SetAllParametersRequest
        ));
}

/**
 * Sets a single NSI parameter over RPC (nsi!NsiRpcSetParameter).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param Store The NSI store to query (active or persistent).
 * \param Action The get or set action to perform.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param ParameterType The parameter block to access (read-write, read-only dynamic or read-only static).
 * \param ParameterBuffer The buffer that contains the parameter data.
 * \param ParameterBufferSize The size, in bytes, of ParameterBuffer.
 * \param ParameterOffset The offset, in bytes, of the parameter within the parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcSetParameter(
    _In_opt_ HANDLE RpcHandle,
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
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcSetParameter_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcSetParameter_I(
        RpcHandle,
        Store,
        Action,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        ParameterType,
        ParameterBuffer,
        ParameterBufferSize,
        ParameterOffset
        ));
}

/**
 * Sets a single NSI parameter over RPC using a prepared request (nsi!NsiRpcSetParameterEx).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param SetParameterRequest The set parameter request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhNsiRpcSetParameterEx(
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST SetParameterRequest
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcSetParameterEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhpNetioDosErrorToNtStatus(NsiRpcSetParameterEx_I(
        RpcHandle,
        SetParameterRequest
        ));
}

/**
 * Retrieves a single NSI parameter through the device interface.
 *
 * \param Store The NSI store to query (active or persistent).
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param ParameterType The parameter block to access (read-write, read-only dynamic or read-only static).
 * \param Buffer The buffer that receives the parameter data.
 * \param BufferLength The size, in bytes, of Buffer.
 * \param ParameterOffset The offset, in bytes, of the parameter within the parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
static NTSTATUS PhpNsiGetParameter(
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_ NSI_PARAMETER_TYPE ParameterType,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_ ULONG ParameterOffset
    )
{
    NSI_GET_PARAMETER_EX_REQUEST request = { 0 };

    request.Header.ModuleId = ModuleId;
    request.Header.ObjectId = ObjectId;
    request.Header.Store = Store;
    request.Key = Key;
    request.KeySize = KeySize;
    request.ParameterType = ParameterType;
    request.Buffer = Buffer;
    request.BufferLength = BufferLength;
    request.ParameterOffset = ParameterOffset;

    return PhNsiGetParameterEx(&request);
}

/**
 * Sets the read-write parameters of an NSI object through the device interface.
 *
 * \param Store The NSI store to query (active or persistent).
 * \param Action The get or set action to perform.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param Rw The read-write parameter block.
 * \param RwSize The size, in bytes, of the read-write parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
static NTSTATUS PhpNsiSetAllParameters(
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(RwSize) PVOID Rw,
    _In_ ULONG RwSize
    )
{
    NSI_SET_ALL_PARAMETERS_EX_REQUEST request = { 0 };

    request.Header.ModuleId = ModuleId;
    request.Header.ObjectId = ObjectId;
    request.Header.Store = Store;
    request.Header.SetAction = Action;
    request.Key = Key;
    request.KeySize = KeySize;
    request.ReadWriteBuffer = Rw;
    request.ReadWriteBufferSize = RwSize;

    return PhNsiSetAllParametersEx(&request);
}

#pragma endregion

#pragma region TCP/IP Utilities

/**
 * Sets the state of a TCP connection. Only MIB_TCP_STATE_DELETE_TCB (terminate the connection) is supported.
 *
 * \param NetworkItem The network item that identifies the connection.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-settcpentry
 */
NTSTATUS PhSetTcpEntry(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    NTSTATUS status;
    NSI_SET_TCP_ENTRY NsiSetTcpEntryData;

    RtlZeroMemory(&NsiSetTcpEntryData, sizeof(NSI_SET_TCP_ENTRY));

    if (NetworkItem->ProtocolType == PH_NETWORK_PROTOCOL_TCP4)
    {
        NsiSetTcpEntryData.LocalFamily = AF_INET;
        NsiSetTcpEntryData.LocalPort = _byteswap_ushort((USHORT)NetworkItem->LocalEndpoint.Port);
        memcpy(&NsiSetTcpEntryData.LocalAddressIPv4, &NetworkItem->LocalEndpoint.Address.InAddr, sizeof(IN_ADDR));

        NsiSetTcpEntryData.RemoteFamily = AF_INET;
        NsiSetTcpEntryData.RemotePort = _byteswap_ushort((USHORT)NetworkItem->RemoteEndpoint.Port);
        memcpy(&NsiSetTcpEntryData.RemoteAddressIPv4, &NetworkItem->RemoteEndpoint.Address.InAddr, sizeof(IN_ADDR));
    }
    else if (NetworkItem->ProtocolType == PH_NETWORK_PROTOCOL_TCP6)
    {
        NsiSetTcpEntryData.LocalFamily = AF_INET6;
        NsiSetTcpEntryData.LocalPort = _byteswap_ushort((USHORT)NetworkItem->LocalEndpoint.Port);
        memcpy(&NsiSetTcpEntryData.LocalAddressIPv6, &NetworkItem->LocalEndpoint.Address.In6Addr, sizeof(IN6_ADDR));
        NsiSetTcpEntryData.LocalScopeID = NetworkItem->LocalScopeId;

        NsiSetTcpEntryData.RemoteFamily = AF_INET6;
        NsiSetTcpEntryData.RemotePort = _byteswap_ushort((USHORT)NetworkItem->RemoteEndpoint.Port);
        memcpy(&NsiSetTcpEntryData.RemoteAddressIPv6, &NetworkItem->RemoteEndpoint.Address.In6Addr, sizeof(IN6_ADDR));
        NsiSetTcpEntryData.RemoteScopeID = NetworkItem->RemoteScopeId;
    }

    status = PhNsiSetAllParameters(
        NsiActive,
        NsiSetCreateOrSet,
        &NPI_MS_TCP_MODULEID,
        NsiTcpObjectConnection,
        &NsiSetTcpEntryData,
        sizeof(NSI_SET_TCP_ENTRY),
        NULL,
        0
        );

    return status;
}

#ifdef ASSERT
#pragma push_macro("ASSERT")
#undef ASSERT
#define PHNT_RESTORE_ASSERT
#ifdef ASSERTMSG
#pragma push_macro("ASSERTMSG")
#undef ASSERTMSG
#define PHNT_RESTORE_ASSERTMSG
#endif
#endif

#ifndef ASSERT
#define PHNT_MSTCPIP_ASSERT_UNDEFINED
#define ASSERT(exp) ((VOID)0)
#endif

#include <ip2string.h>
#include <ipexport.h>
#include <ipmib.h>
#include <netiodef.h>
#include <tcpestats.h>
#include <tcpmib.h>

#ifdef PHNT_MSTCPIP_ASSERT_UNDEFINED
#undef ASSERT
#undef PHNT_MSTCPIP_ASSERT_UNDEFINED
#endif

#ifdef PHNT_RESTORE_ASSERT
#undef PHNT_RESTORE_ASSERT
#pragma pop_macro("ASSERT")
#endif

#ifdef PHNT_RESTORE_ASSERTMSG
#undef PHNT_RESTORE_ASSERTMSG
#pragma pop_macro("ASSERTMSG")
#endif

#define WIN32_LEAN_AND_MEAN

// tcpip!NsiAllocateAndGetTable uses a module-relative object ID as its third argument.
// The same numeric value can mean different things for different module IDs.
//
// Recovered tcpip.sys object sets are emitted as local literals in helper functions,
// not as one shared global array. Current binary-backed NsiAllocateAndGetTable usage:
//   NDIS: 1, 7, 15
//   IP family: 7, 8, 10, 11, 14, 16, 28
//   TCP-side module: 19, 25, 29
//   UDP-side module: 3, 5
// Object 35 is also observed in tcpip.sys, but on IPv6 NsiGetAllParameters /
// NsiSetAllParameters paths rather than a recovered NsiAllocateAndGetTable caller.
// Port-pool startup also uses NsiGetAllParameters on TCP objects 20/34 and UDP object 4.
// Exhausting the recovered NsiAllocateAndGetTable import-thunk caller set for this
// image did not produce a binary-backed allocator caller for object 0 or 35.
// Repo call sites additionally exercise:
//   NDIS: 0, 1
//   TCP: 3, 35
//   UDP: 1

typedef ULONG(WINAPI* PFN_NSI_RPC_GET_PARAMETER)(
    HANDLE RpcHandle,
    NSI_STORE Store,
    PNPI_MODULEID ModuleId,
    NSI_OBJECT_ID ObjectId,
    PVOID Key,
    ULONG KeySize,
    NSI_PARAMETER_TYPE Type,
    PVOID Buffer,
    ULONG BufferLength,
    ULONG ParameterId
    );

typedef VOID(NTAPI* PFN_ICMP_APC_ROUTINE)(PVOID, PIO_STATUS_BLOCK, ULONG);

NTSTATUS PhConvertInterfaceLuidToNameW(
    _In_ const NET_LUID* InterfaceLuid,
    _Out_writes_(Length) PWSTR InterfaceName,
    _In_ SIZE_T Length
);

NTSTATUS PhConvertInterfaceNameToLuidW(
    _In_z_ PCWSTR InterfaceName,
    _Out_ NET_LUID* InterfaceLuid
);

NTSTATUS PhConvertStringToGuidW(
    _In_z_ PCWSTR SourceString,
    _Out_ GUID* Guid
);

/**
 * Converts an interface index to an interface LUID using the NDIS NSI provider.
 *
 * \param InterfaceIndex The local index of the network interface.
 * \param InterfaceLuid Receives the interface LUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceindextoluid
 */
NTSTATUS PhNsiConvertInterfaceIndexToLuid(
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
        NsiParameterTypeReadOnlyStatic,
        (PVOID)InterfaceLuid,
        sizeof(NET_LUID)
        );
}

/**
 * Returns the display name of a TCP applied template setting.
 *
 * \param AppliedSetting The NsiTcpAppliedSetting* value.
 * \return A static string, or an empty string for unknown values.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getipinterfaceentry
 */
PCWSTR PhNsiTcpAppliedSettingToString(
    _In_ ULONG AppliedSetting
    )
{
    switch (AppliedSetting)
    {
    case NsiTcpAppliedSettingInternet:
        return L"Internet";
    case NsiTcpAppliedSettingDatacenter:
        return L"Datacenter";
    case NsiTcpAppliedSettingCompat:
        return L"Compat";
    case NsiTcpAppliedSettingInternetCustom:
        return L"InternetCustom";
    case NsiTcpAppliedSettingDatacenterCustom:
        return L"DatacenterCustom";
    case NsiTcpAppliedSettingUnspecified:
        return L"";
    }

    return L"[ERROR]";
}

/**
 * Returns the display name of a TCP connection state.
 *
 * \param State The MIB_TCP_STATE value.
 * \return A static string describing the state.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/tcpmib/ns-tcpmib-mib_tcprow_lh
 */
PCWSTR PhTcpStateToString(
    _In_ MIB_TCP_STATE State
    )
{
    switch (State)
    {
    case MIB_TCP_STATE_CLOSED:
        return L"Closed";
    case MIB_TCP_STATE_LISTEN:
        return L"Listen";
    case MIB_TCP_STATE_SYN_SENT:
        return L"SynSent";
    case MIB_TCP_STATE_SYN_RCVD:
        return L"SynReceived";
    case MIB_TCP_STATE_ESTAB:
        return L"Established";
    case MIB_TCP_STATE_FIN_WAIT1:
        return L"FinWait1";
    case MIB_TCP_STATE_FIN_WAIT2:
        return L"FinWait2";
    case MIB_TCP_STATE_CLOSE_WAIT:
        return L"CloseWait";
    case MIB_TCP_STATE_CLOSING:
        return L"Closing";
    case MIB_TCP_STATE_LAST_ACK:
        return L"LastAck";
    case MIB_TCP_STATE_TIME_WAIT:
        return L"TimeWait";
    case MIB_TCP_STATE_DELETE_TCB:
        return L"DELETE";
    case MIB_TCP_STATE_RESERVED: // Bound
        return L"Bound";
    default:
        return L"Unknown";
    }
}

/**
 * Determines whether a value cannot be a valid ICMP handle (NULL, pseudo-handles or INVALID_HANDLE_VALUE).
 *
 * \param IcmpHandle The handle to test.
 * \return TRUE if the handle is invalid, otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpcreatefile
 */
BOOLEAN PhIsInvalidIcmpHandle(
    _In_ HANDLE IcmpHandle
    )
{
    const ULONG_PTR value = (ULONG_PTR)(IcmpHandle);

    return value == 0
        || value == (ULONG_PTR)NtCurrentProcess()
        || value == (ULONG_PTR)NtCurrentThread()
        || value == (ULONG_PTR)NtCurrentSession();
}

/**
 * APC routine that forwards a completed asynchronous echo request to the caller's APC routine and releases the request context.
 *
 * \param ApcArgument1 The ICMP_ECHO_REQUEST_CONTEXT of the request.
 * \param ApcArgument2 Unused.
 * \param ApcArgument3 Unused.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2
 */
_Function_class_(PS_APC_ROUTINE)
VOID NTAPI PhIcmpEchoRequestCompleteWorker(
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    )
{
    ICMP_ECHO_REQUEST_CONTEXT* context;
    PFN_ICMP_APC_ROUTINE apcRoutine;

    context = (ICMP_ECHO_REQUEST_CONTEXT*)ApcArgument1;
    apcRoutine = (PFN_ICMP_APC_ROUTINE)(context->ApcRoutine);
    apcRoutine(context->ApcContext, &context->IoStatusBlock, 0);
    PhIcmpDestroyEchoRequestContext(context);
}

/**
 * Converts an HRESULT from an address string routine into an NTSTATUS value.
 *
 * \param Status The HRESULT to convert.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winerror/nf-winerror-hresult_code
 */
NTSTATUS PhTcpipResolveStringStatus(
    _In_ HRESULT Status
    )
{
    if (HR_SUCCESS(Status))
    {
        return STATUS_SUCCESS;
    }

    //if (Status == STRSAFE_E_INVALID_PARAMETER || Status == STRSAFE_E_INSUFFICIENT_BUFFER)
    //{
    //    return STATUS_BUFFER_TOO_SMALL;
    //}

    return PhNtStatusFromHResult(Status);
}

/**
 * Normalizes an NTSTATUS from an Rtl address string routine.
 *
 * \param Status The NTSTATUS to convert.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-rtlntstatustodoserror
 */
NTSTATUS PhTcpipResolveNtStringStatus(
    _In_ ULONG Status
    )
{
    if (NT_SUCCESS(Status))
    {
        return STATUS_SUCCESS;
    }

    return Status;
}

/**
 * Converts an HRESULT from a NETIO string routine into an NTSTATUS value.
 *
 * \param Status The HRESULT to convert.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winerror/nf-winerror-hresult_code
 */
NTSTATUS PhTcpipResolveNetioStringStatus(
    _In_ HRESULT Status
    )
{
    if (HR_SUCCESS(Status))
    {
        return STATUS_SUCCESS;
    }

    //if (Status == STRSAFE_E_INVALID_PARAMETER || Status == STRSAFE_E_INSUFFICIENT_BUFFER)
    //{
    //    return STATUS_NO_MEMORY;
    //}

    return PhNtStatusFromHResult(Status);
}

/**
 * Converts a network compartment GUID to a compartment identifier.
 *
 * \param CompartmentGuid The compartment GUID.
 * \param CompartmentId Receives the compartment identifier.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertcompartmentguidtoid
 */
NTSTATUS PhConvertCompartmentGuidToId(
    _In_ const GUID* CompartmentGuid,
    _Out_ PNET_IF_COMPARTMENT_ID CompartmentId
    )
{
    GUID Key;
    NTSTATUS Status;

    if (CompartmentGuid == NULL || CompartmentId == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Key = *CompartmentGuid;
    Status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectCompartmentGuid,
        &Key,
        sizeof(GUID),
        NsiParameterTypeReadOnlyStatic,
        CompartmentId,
        sizeof(NET_IF_COMPARTMENT_ID),
        NsiNdisCompartmentGuidType2Id
        );

    if (!NT_SUCCESS(Status))
    {
        *CompartmentId = 0;
    }

    return Status;
}

/**
 * Converts a network compartment identifier to a compartment GUID.
 *
 * \param CompartmentId The network compartment identifier.
 * \param CompartmentGuid Receives the compartment GUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertcompartmentidtoguid
 */
NTSTATUS PhConvertCompartmentIdToGuid(
    _In_ NET_IF_COMPARTMENT_ID CompartmentId,
    _Out_ GUID* CompartmentGuid
    )
{
    NTSTATUS Status;
    ULONG Count;
    NET_IF_COMPARTMENT_ID* KeyTable;
    NDIS_NSI_COMPARTMENT_RW* ReadWriteTable;
    ULONG Index;

    KeyTable = NULL;
    ReadWriteTable = NULL;
    Count = 0;

    if (CompartmentGuid == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ZeroMemory(CompartmentGuid, sizeof(GUID));

    Status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectCompartment,
        &KeyTable,
        sizeof(NET_IF_COMPARTMENT_ID),
        &ReadWriteTable,
        sizeof(NDIS_NSI_COMPARTMENT_RW),
        NULL,
        0,
        NULL,
        0,
        &Count,
        0
        );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = STATUS_NOT_FOUND;

    for (Index = 0; Index < Count; ++Index)
    {
        if (KeyTable[Index] == CompartmentId)
        {
            *CompartmentGuid = ReadWriteTable[Index].CompartmentGuid;
            Status = STATUS_SUCCESS;
            break;
        }
    }

    PhNsiFreeTable(KeyTable, ReadWriteTable, NULL, NULL);
    return Status;
}

/**
 * Retrieves the network compartment identifier of the current thread.
 *
 * \return The compartment identifier, or NET_IF_COMPARTMENT_ID_UNSPECIFIED on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getcurrentthreadcompartmentid
 */
NET_IF_COMPARTMENT_ID PhGetCurrentThreadCompartmentId(
    VOID
    )
{
    NET_IF_COMPARTMENT_ID current;

    current = NET_IF_COMPARTMENT_ID_UNSPECIFIED;

    if (!PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectCurrentThreadCompartment,
        NULL,
        0,
        NsiParameterTypeReadWrite,
        &current,
        sizeof(NET_IF_COMPARTMENT_ID),
        NsiNdisCurrentThreadCompartmentType0Id
        ))
    {
        return current;
    }

    return NET_IF_COMPARTMENT_ID_PRIMARY;
}

//
//ULONG PhConvertGuidToStringA(
//    _In_ const GUID* Guid,
//    _Out_writes_(Length) PSTR GuidString,
//    _In_ SIZE_T Length
//    )
//{
//   // ULONG status;
//
//    if (Guid == NULL || GuidString == NULL)
//    {
//        return STATUS_INVALID_PARAMETER;
//    }
//
//    //status = RtlStringCchPrintfA(
//    //    GuidString,
//    //    Length,
//    //    "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
//    //    Guid->Data1,
//    //    Guid->Data2,
//    //    Guid->Data3,
//    //    Guid->Data4[0],
//    //    Guid->Data4[1],
//    //    Guid->Data4[2],
//    //    Guid->Data4[3],
//    //    Guid->Data4[4],
//    //    Guid->Data4[5],
//    //    Guid->Data4[6],
//    //    Guid->Data4[7]);
//
//    return PhTcpipResolveNtStringStatus(status);
//}
//
///* Source: A:\tcpip\convert.c */
//
//ULONG PhConvertGuidToStringW(
//    _In_ const GUID* Guid,
//    _Out_writes_(Length) PWSTR GuidString,
//    _In_ SIZE_T Length
//    )
//{
//    HRESULT Result;
//
//    if (Guid == NULL || GuidString == NULL)
//    {
//        return STATUS_INVALID_PARAMETER;
//    }
//
//    Result = StringCchPrintfW(
//        GuidString,
//        Length,
//        L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
//        Guid->Data1,
//        Guid->Data2,
//        Guid->Data3,
//        Guid->Data4[0],
//        Guid->Data4[1],
//        Guid->Data4[2],
//        Guid->Data4[3],
//        Guid->Data4[4],
//        Guid->Data4[5],
//        Guid->Data4[6],
//        Guid->Data4[7]);
//
//    return PhTcpipResolveStringStatus(Result);
//}

/**
 * Converts an interface alias to an interface LUID.
 *
 * \param InterfaceAlias The interface alias (friendly name).
 * \param InterfaceLuid Receives the interface LUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfacealiastoluid
 */
NTSTATUS PhConvertInterfaceAliasToLuid(
    _In_z_ PCWSTR InterfaceAlias,
    _Out_ PNET_LUID InterfaceLuid
    )
{
    NTSTATUS Status;
    ULONG Count;
    NET_LUID* KeyTable;
    NDIS_NSI_INTERFACE_ENUM_ROD* DynamicTable;
    ULONG Index;

    KeyTable = NULL;
    DynamicTable = NULL;
    Count = 0;

    //if (InterfaceAlias == NULL || InterfaceLuid == NULL || !NT_SUCCESS(RtlStringCchLengthW(InterfaceAlias, MAXIMUM_FILENAME_LENGTH, AliasLength)))
    //{
    //    return STATUS_INVALID_PARAMETER;
    //}

    Status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceWithoutStatistics,
        &KeyTable,
        sizeof(NET_LUID),
        NULL,
        0,
        &DynamicTable,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROD),
        NULL,
        0,
        &Count,
        0
        );

    if (!NT_SUCCESS(Status))
    {
        InterfaceLuid->Value = 0;
        return Status;
    }

    for (Index = 0; Index < Count; ++Index)
    {
        ULONG CurrentLength = DynamicTable[Index].ifAlias.Length >> 1;

        if (CurrentLength > MAXIMUM_FILENAME_LENGTH)
        {
            CurrentLength = MAXIMUM_FILENAME_LENGTH;
        }

        DynamicTable[Index].ifAlias.String[CurrentLength] = UNICODE_NULL;

        if (PhEqualStringZ(DynamicTable[Index].ifAlias.String, InterfaceAlias, TRUE))
        {
            InterfaceLuid->Value = KeyTable[Index].Value;
            PhNsiFreeTable(KeyTable, NULL, DynamicTable, NULL);
            return STATUS_SUCCESS;
        }
    }

    for (Index = 0; Index < Count; ++Index)
    {
        PH_STRINGREF aliasSr;

        PhInitializeStringRef(&aliasSr, DynamicTable[Index].ifAlias.String);

        if (PhStartsWithStringRef2(&aliasSr, InterfaceAlias, TRUE))
        {
            InterfaceLuid->Value = KeyTable[Index].Value;

            PhNsiFreeTable(KeyTable, NULL, DynamicTable, NULL);

            return STATUS_SUCCESS;
        }
    }

    InterfaceLuid->Value = 0;

    PhNsiFreeTable(KeyTable, NULL, DynamicTable, NULL);

    return STATUS_INVALID_PARAMETER;
}

/**
 * Converts an interface GUID to an interface LUID.
 *
 * \param InterfaceGuid The GUID of the network interface.
 * \param InterfaceLuid Receives the interface LUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceguidtoluid
 */
NTSTATUS PhConvertInterfaceGuidToLuid(
    _In_ const GUID* InterfaceGuid,
    _Out_ PNET_LUID InterfaceLuid
    )
{
    NTSTATUS Status;
    ULONG Count;
    NET_LUID* KeyTable;
    NDIS_NSI_INTERFACE_ENUM_ROS* StaticTable;
    ULONG Index;

    KeyTable = NULL;
    StaticTable = NULL;
    Count = 0;

    if (InterfaceGuid == NULL || InterfaceLuid == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceWithoutStatistics,
        &KeyTable,
        sizeof(NET_LUID),
        NULL,
        0,
        NULL,
        0,
        &StaticTable,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROS),
        &Count,
        0
        );

    if (!NT_SUCCESS(Status))
    {
        InterfaceLuid->Value = 0;
        return Status;
    }

    Status = STATUS_INVALID_PARAMETER;
    InterfaceLuid->Value = 0;

    for (Index = 0; Index < Count; ++Index)
    {
        //if (PhTcpipGuidEquals(&StaticTable[Index].InterfaceGuid, InterfaceGuid))
        //{
        //    InterfaceLuid->Value = KeyTable[Index].Value;
        //    Status = STATUS_SUCCESS;
        //    break;
        //}
    }

    PhNsiFreeTable(KeyTable, NULL, NULL, StaticTable);
    return Status;
}

/**
 * Converts an interface index to an interface LUID.
 *
 * \param InterfaceIndex The local index of the network interface.
 * \param InterfaceLuid Receives the interface LUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceindextoluid
 */
NTSTATUS PhConvertInterfaceIndexToLuid(
    _In_ NET_IFINDEX InterfaceIndex,
    _Out_ PNET_LUID InterfaceLuid
    )
{
    NTSTATUS Status;
    NET_IFINDEX Key;

    Key = InterfaceIndex;

    if (InterfaceLuid == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceIndex,
        &Key,
        sizeof(NET_IFINDEX),
        NsiParameterTypeReadOnlyStatic,
        InterfaceLuid,
        sizeof(NET_LUID),
        NsiNdisInterfaceIndexToLuid
        );

    if (!NT_SUCCESS(Status))
    {
        InterfaceLuid->Value = 0;
    }

    return Status;
}

/**
 * Converts an interface LUID to its alias.
 *
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param InterfaceAlias Receives the null-terminated alias.
 * \param Length The size of InterfaceAlias, in characters.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoalias
 */
NTSTATUS PhConvertInterfaceLuidToAlias(
    _In_ const NET_LUID* InterfaceLuid,
    _Out_writes_z_(Length) PWSTR InterfaceAlias,
    _In_ SIZE_T Length
    )
{
    NTSTATUS Status;
    NET_LUID Key;
    NSI_STRING_256 Alias;
    ULONG CurrentLength;

    ZeroMemory(&Alias, sizeof(NSI_STRING_256));
    Key = *InterfaceLuid;

    Status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &Key,
        sizeof(NET_LUID),
        NsiParameterTypeReadWrite,
        &Alias,
        sizeof(NSI_STRING_256),
        NsiNdisIfType0Alias
        );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CurrentLength = Alias.Length >> 1;
    if (CurrentLength > MAXIMUM_FILENAME_LENGTH)
    {
        CurrentLength = MAXIMUM_FILENAME_LENGTH;
    }

    Alias.Buffer[CurrentLength] = UNICODE_NULL;
    return 0;// PhTcpipResolveNetioStringStatus(StringCchPrintfW(InterfaceAlias, Length, L"%s", Alias.Buffer));
}

/**
 * Converts an interface LUID to its GUID.
 *
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param InterfaceGuid Receives the interface GUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoguid
 */
NTSTATUS PhConvertInterfaceLuidToGuid(
    _In_ const NET_LUID* InterfaceLuid,
    _Out_ GUID* InterfaceGuid
    )
{
    NTSTATUS Status;
    ULONG64 Key;

    if (InterfaceLuid == NULL || InterfaceGuid == NULL)
        return STATUS_INVALID_PARAMETER;

    Key = InterfaceLuid->Value;

    Status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &Key,
        sizeof(ULONG64),
        NsiParameterTypeReadOnlyStatic,
        InterfaceGuid,
        sizeof(GUID),
        NsiNdisInterfaceLuidToGuid
        );

    if (!NT_SUCCESS(Status))
    {
        ZeroMemory(InterfaceGuid, sizeof(GUID));
    }

    return Status;
}

/**
 * Opens a handle to the NDIS device object of a network interface.
 *
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param InterfaceHandle Receives the device handle. Close it with NtClose.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntdeviceiocontrolfile
 */
NTSTATUS PhGetInterfaceHandle(
    _In_ const NET_LUID* InterfaceLuid,
    _Out_ PHANDLE InterfaceHandle
    )
{
    GUID interfaceGuid;
    UNICODE_STRING objectName;
    UNICODE_STRING guidString;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE fileHandle;
    WCHAR objectBuffer[PHP_NSI_INTERFACE_OBJECT_NAME_BUFFER_LENGTH];
    NTSTATUS status;

    if (InterfaceLuid == NULL || InterfaceHandle == NULL)
        return STATUS_INVALID_PARAMETER;

    *InterfaceHandle = NULL;
    fileHandle = NULL;
    interfaceGuid = GUID_NULL;
    guidString.Buffer = NULL;
    guidString.Length = 0;
    guidString.MaximumLength = 0;

    RtlInitEmptyUnicodeString(&objectName, objectBuffer, sizeof(objectBuffer));

    status = PhConvertInterfaceLuidToGuid(InterfaceLuid, &interfaceGuid);
    if (!NT_SUCCESS(status))
        return status;

    status = RtlStringFromGUID(&interfaceGuid, &guidString);
    if (!NT_SUCCESS(status))
        return status;

    status = RtlAppendUnicodeToString(&objectName, TCPIP_ADAPTER_PREFIX);
    if (NT_SUCCESS(status))
        status = RtlAppendUnicodeStringToString(&objectName, &guidString);

    if (NT_SUCCESS(status))
    {
        InitializeObjectAttributes(
            &objectAttributes,
            &objectName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = NtOpenFile(
            &fileHandle,
            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
            &objectAttributes,
            &ioStatusBlock,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (NT_SUCCESS(status))
            *InterfaceHandle = fileHandle;
        else
            status = status;
    }
    else
    {
        status = status;
    }

    RtlFreeUnicodeString(&guidString);
    return status;
}

/**
 * Normalizes an NDIS status into an NTSTATUS error value.
 *
 * \param Status The status to convert.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-rtlntstatustodoserror
 */
NTSTATUS PhConvertNdisStatusToError(
    _In_ NTSTATUS Status
    )
{
    if (Status)
    {
        if (Status == STATUS_BUFFER_OVERFLOW)
            Status = STATUS_INTERNAL_ERROR;
        else if (Status >= 0)
            Status = STATUS_UNSUCCESSFUL;

        return Status;
    }

    return 0;
}

/**
 * Builds a MIB_FL_VIRTUAL_INTERFACE_ROW from the NSI key and parameter blocks.
 *
 * \param InterfaceRow Receives the row.
 * \param Family The address family of the row.
 * \param Key The NSI key.
 * \param ReadWrite The read-write parameter block.
 * \param ReadOnlyDynamic The read-only dynamic parameter block.
 * \param ReadOnlyStatic The read-only static parameter block.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
static VOID PhFillVirtualInterfaceRow(
    _Out_ PMIB_FL_VIRTUAL_INTERFACE_ROW InterfaceRow,
    _In_ USHORT Family,
    _In_ const NSI_FL_VIRTUAL_INTERFACE_KEY* Key,
    _In_ const NSI_FL_VIRTUAL_INTERFACE_RW* ReadWrite,
    _In_ const NSI_FL_VIRTUAL_INTERFACE_ROD* ReadOnlyDynamic,
    _In_ const NSI_FL_VIRTUAL_INTERFACE_ROS* ReadOnlyStatic
    )
{
    RtlZeroMemory(InterfaceRow, sizeof(*InterfaceRow));

    InterfaceRow->Family = Family;
    InterfaceRow->IfLuid = Key->InterfaceLuid;
    InterfaceRow->VirtualIfId = Key->VirtualInterfaceId;
    InterfaceRow->CompartmentGuid = Key->CompartmentGuid;
    InterfaceRow->IsolationMode = Key->IsolationMode;
    InterfaceRow->Origin = ReadOnlyStatic->VirtualInterfaceType;
    InterfaceRow->VirtualIfLuid = ReadOnlyStatic->VirtualInterfaceLuid;
    InterfaceRow->VirtualIfIndex = ReadOnlyDynamic->VirtualInterfaceIndex;
    InterfaceRow->AllowLocalNd = ReadWrite->ClientInterfaceFlag;
    memcpy(&InterfaceRow->AttachedFlsnpiClients, ReadOnlyDynamic->Counters32, sizeof(ReadOnlyDynamic->Counters32));
    memcpy(&InterfaceRow->FlsnpiClientInjectErrors, ReadOnlyDynamic->Counters64, sizeof(ReadOnlyDynamic->Counters64));
}

/**
 * Converts an interface LUID to its local index.
 *
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param InterfaceIndex Receives the interface index.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoindex
 */
NTSTATUS PhConvertInterfaceLuidToIndex(
    _In_ const NET_LUID* InterfaceLuid,
    _Out_ PNET_IFINDEX InterfaceIndex
    )
{
    NTSTATUS Status;
    ULONG64 Key;

    if (InterfaceLuid == NULL || InterfaceIndex == NULL)
        return STATUS_INVALID_PARAMETER;

    Key = InterfaceLuid->Value;

    Status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &Key,
        sizeof(ULONG64),
        NsiParameterTypeReadOnlyStatic,
        InterfaceIndex,
        sizeof(NET_IFINDEX),
        NsiNdisInterfaceLuidToIndex
        );

    if (!NT_SUCCESS(Status))
    {
        *InterfaceIndex = 0;
    }

    return Status;
}

/**
 * Converts an interface LUID to its interface name (for example "ethernet_32768").
 *
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param InterfaceName Receives the null-terminated interface name.
 * \param Length The size of InterfaceName, in characters.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtonamew
 */
NTSTATUS PhConvertInterfaceLuidToNameW(
    _In_ const NET_LUID* InterfaceLuid,
    _Out_writes_(Length) PWSTR InterfaceName,
    _In_ SIZE_T Length
    )
{
    NTSTATUS Status;
    ULONG Index;
    PCWSTR Prefix;
    WCHAR PrefixBuffer[PHP_INTERFACE_NAME_BUFFER_LENGTH];
    HRESULT Result;

    if (InterfaceLuid == NULL || InterfaceName == NULL)
        return STATUS_INVALID_PARAMETER;

    Status = STATUS_SUCCESS;
    Prefix = NULL;

    for (Index = 0; Index < ARRAYSIZE(IfTypeStringTable); ++Index)
    {
        if (IfTypeStringTable[Index].IfType == InterfaceLuid->Info.IfType)
        {
            Prefix = IfTypeStringTable[Index].Name;
            break;
        }
    }

    if (Prefix == NULL)
    {
        PPH_STRING prefixString;

        prefixString = PhFormatString(L"iftype%I64u", (ULONGLONG)(InterfaceLuid->Info.IfType));
        if (!prefixString)
            return STATUS_NO_MEMORY;

        if (!NT_SUCCESS(PhCopyStringZ(prefixString->Buffer, prefixString->Length / sizeof(WCHAR), PrefixBuffer, RTL_NUMBER_OF(PrefixBuffer), NULL)))
        {
            PhDereferenceObject(prefixString);
            return STATUS_NO_MEMORY;
        }

        PhDereferenceObject(prefixString);
        Prefix = PrefixBuffer;
    }

    {
        PPH_STRING interfaceName;

        interfaceName = PhFormatString(L"%s_%I64u", Prefix, (ULONGLONG)(InterfaceLuid->Info.NetLuidIndex));
        if (!interfaceName)
            return STATUS_NO_MEMORY;

        Result = PhCopyStringZ(interfaceName->Buffer, interfaceName->Length / sizeof(WCHAR), InterfaceName, Length, NULL);
        PhDereferenceObject(interfaceName);

        if (NT_SUCCESS(Result))
            return Status;
    }

    return Result;
}

/**
 * Converts an interface name (for example "ethernet_32768") to an interface LUID.
 *
 * \param InterfaceName The interface name.
 * \param InterfaceLuid Receives the interface LUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfacenametoluidw
 */
NTSTATUS PhConvertInterfaceNameToLuidW(
    _In_z_ PCWSTR InterfaceName,
    _Out_ NET_LUID* InterfaceLuid
    )
{
    ULONG Index;
    LONG Remaining;
    PCWSTR Source;
    WCHAR* Destination;
    WCHAR* Terminator;
    WCHAR LocalName[PHP_INTERFACE_NAME_BUFFER_LENGTH];
    PCWSTR Prefix = L"iftype";
    WCHAR* Separator;
    PH_STRINGREF sr;
    ULONG_PTR index;
    PH_STRINGREF indexSr;
    LONG64 value;

    if (InterfaceLuid == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Index = 0;
    InterfaceLuid->Value = 0;

    if (InterfaceName == NULL)
    {
        return STATUS_OBJECT_NAME_INVALID;
    }

    Remaining = PHP_INTERFACE_NAME_MAX_LENGTH;
    Source = InterfaceName;
    while (Remaining != 0 && *Source != UNICODE_NULL)
    {
        ++Source;
        --Remaining;
    }

    if (Remaining == 0)
    {
        return STATUS_OBJECT_NAME_INVALID;
    }

    Source = InterfaceName;
    Destination = LocalName;
    Remaining = PHP_INTERFACE_NAME_MAX_LENGTH;
    while (Remaining != 0 && *Source != UNICODE_NULL)
    {
        *Destination++ = *Source++;
        --Remaining;
    }

    Terminator = Destination;
    if (Remaining == 0)
    {
        Terminator = Destination - 1;
    }

    *Terminator = UNICODE_NULL;

    PhInitializeStringRef(&sr, LocalName);
    index = PhFindCharInStringRef(&sr, L'_', FALSE);
    if (index == SIZE_MAX)
    {
        return STATUS_OBJECT_NAME_INVALID;
    }

    Separator = LocalName + index;

    PhInitializeStringRef(&indexSr, Separator + 1);
    if (PhStringToInteger64(&indexSr, 10, &value))
        InterfaceLuid->Info.NetLuidIndex = (ULONG)value;
    else
        InterfaceLuid->Info.NetLuidIndex = 0;

    if (wcsncmp(LocalName, Prefix, PhCountStringZ(Prefix)) == 0)
    {
        PH_STRINGREF typeSr;

        PhInitializeStringRef(&typeSr, LocalName + PhCountStringZ(Prefix));

        if (PhStringToInteger64(&typeSr, 10, &value))
            InterfaceLuid->Info.IfType = (USHORT)value;
        else
            InterfaceLuid->Info.IfType = 0;

        return STATUS_SUCCESS;
    }

    for (Index = 0; Index < ARRAYSIZE(IfTypeStringTable); ++Index)
    {
        size_t PrefixLength = (size_t)(Separator - LocalName);

        if (wcsncmp(LocalName, IfTypeStringTable[Index].Name, PrefixLength) == 0)
        {
            InterfaceLuid->Info.IfType = IfTypeStringTable[Index].IfType;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_OBJECT_NAME_INVALID;
}

/**
 * Finds the interface whose physical (MAC) address matches the specified address.
 *
 * \param InterfacePhysicalAddress The physical address.
 * \param Length The size, in bytes, of the physical address.
 * \param InterfaceLuid Receives the interface LUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceindextoluid
 */
NTSTATUS PhConvertInterfacePhysicalAddressToLuid(
    _In_reads_bytes_(Length) const void* InterfacePhysicalAddress,
    _In_ SIZE_T Length,
    _Out_ PNET_LUID InterfaceLuid
    )
{
    NTSTATUS Status;
    ULONG Count;
    NET_LUID* KeyTable;
    NDIS_NSI_INTERFACE_INFORMATION_RW* ReadWriteTable;
    NDIS_NSI_INTERFACE_INFORMATION_ROS* StaticTable;
    BOOLEAN Found;
    ULONG Index;

    Count = 0;
    KeyTable = NULL;
    ReadWriteTable = NULL;
    StaticTable = NULL;
    Found = FALSE;

    if (InterfaceLuid == NULL || InterfacePhysicalAddress == NULL || Length == 0)
        return STATUS_INVALID_PARAMETER;

    InterfaceLuid->Value = 0;

    Status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &KeyTable,
        sizeof(NET_LUID),
        &ReadWriteTable,
        sizeof(NDIS_NSI_INTERFACE_INFORMATION_RW),
        NULL,
        0,
        &StaticTable,
        sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROS),
        &Count,
        0
        );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    for (Index = 0; Index < Count; ++Index)
    {
        if ((StaticTable[Index].ifFlags & NetIfInterfaceFlagFilterInterface) == 0 && ReadWriteTable[Index].ifPhysAddress.Length == Length)
        {
            if (memcmp(InterfacePhysicalAddress, ReadWriteTable[Index].ifPhysAddress.Address, Length) == 0)
            {
                Found = TRUE;
                InterfaceLuid->Value = KeyTable[Index].Value;
            }
        }
    }

    PhNsiFreeTable(KeyTable, ReadWriteTable, NULL, StaticTable);
    return Found ? STATUS_SUCCESS  : STATUS_NOT_FOUND;
}

/**
 * Converts an IPv4 subnet mask to a prefix length.
 *
 * \param Mask The subnet mask, in network byte order.
 * \param MaskLength Receives the prefix length.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertipv4masktolength
 */
NTSTATUS PhConvertIpv4MaskToLength(
    _In_ ULONG Mask,
    _Out_ PUCHAR MaskLength
    )
{
    UCHAR Index;

    for (Index = 0;; ++Index)
    {
        if (Index >= RTL_NUMBER_OF(Ipv4Mask))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (Mask == Ipv4Mask[Index])
        {
            break;
        }
    }

    *MaskLength = Index;
    return STATUS_SUCCESS;
}

/**
 * Converts a prefix length to an IPv4 subnet mask.
 *
 * \param MaskLength The prefix length (0 to 32).
 * \param Mask Receives the subnet mask, or MAXULONG (INADDR_NONE) on failure.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertlengthtoipv4mask
 */
NTSTATUS PhConvertLengthToIpv4Mask(
    _In_ ULONG MaskLength,
    _Out_ PULONG Mask
    )
{
    if (MaskLength >= RTL_NUMBER_OF(Ipv4Mask))
    {
        *Mask = MAXULONG;
        return STATUS_INVALID_PARAMETER;
    }

    *Mask = Ipv4Mask[MaskLength];
    return STATUS_SUCCESS;
}

/**
 * Converts the alias of an interface in another site to an interface LUID.
 *
 * \param SiteId The site (compartment) identifier of the remote interface.
 * \param InterfaceAlias The interface alias (friendly name).
 * \param InterfaceLuid Receives the interface LUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfacealiastoluid
 */
NTSTATUS PhConvertRemoteInterfaceAliasToLuid(
    _In_ ULONG SiteId,
    _In_z_ PCWSTR InterfaceAlias,
    _Out_ PNET_LUID InterfaceLuid
    )
{
    NTSTATUS Status;
    ULONG Count;
    NET_LUID* KeyTable;
    NDIS_NSI_INTERFACE_ENUM_ROD* DynamicTable;
    ULONG Index;

    Count = 0;
    KeyTable = NULL;
    DynamicTable = NULL;

    if (InterfaceAlias == NULL || InterfaceLuid == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (SiteId != 0)
    {
        return STATUS_NOT_SUPPORTED;
    }

    Status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceWithoutStatistics,
        &KeyTable,
        sizeof(NET_LUID),
        NULL,
        0,
        &DynamicTable,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROD),
        NULL,
        0,
        &Count,
        0
        );

    if (!NT_SUCCESS(Status))
    {
        InterfaceLuid->Value = 0;
        return Status;
    }

    for (Index = 0; Index < Count; ++Index)
    {
        ULONG CurrentLength = DynamicTable[Index].ifAlias.Length >> 1;
        if (CurrentLength > MAXIMUM_FILENAME_LENGTH)
        {
            CurrentLength = MAXIMUM_FILENAME_LENGTH;
        }

        DynamicTable[Index].ifAlias.String[CurrentLength] = UNICODE_NULL;

        if (PhEqualStringZ(DynamicTable[Index].ifAlias.String, InterfaceAlias, TRUE))
        {
            InterfaceLuid->Value = KeyTable[Index].Value;
            PhNsiFreeTable(KeyTable, NULL, DynamicTable, NULL);
            return STATUS_SUCCESS;
        }
    }

    for (Index = 0; Index < Count; ++Index)
    {
        PH_STRINGREF aliasSr;
        PhInitializeStringRef(&aliasSr, DynamicTable[Index].ifAlias.String);
        if (PhStartsWithStringRef2(&aliasSr, InterfaceAlias, TRUE))
        {
            InterfaceLuid->Value = KeyTable[Index].Value;
            PhNsiFreeTable(KeyTable, NULL, DynamicTable, NULL);
            return STATUS_SUCCESS;
        }
    }

    InterfaceLuid->Value = 0;
    PhNsiFreeTable(KeyTable, NULL, DynamicTable, NULL);
    return STATUS_INVALID_PARAMETER;
}

/**
 * Converts the GUID of an interface in another site to an interface LUID.
 *
 * \param SiteId The site (compartment) identifier of the remote interface.
 * \param InterfaceGuid The GUID of the network interface.
 * \param InterfaceLuid Receives the interface LUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceguidtoluid
 */
NTSTATUS PhConvertRemoteInterfaceGuidToLuid(
    _In_ ULONG SiteId,
    _In_ const GUID* InterfaceGuid,
    _Out_ PNET_LUID InterfaceLuid
    )
{
    NTSTATUS Status;
    ULONG Count;
    NET_LUID* KeyTable;
    NDIS_NSI_INTERFACE_ENUM_ROS* StaticTable;
    ULONG Index;

    Count = 0;
    KeyTable = NULL;
    StaticTable = NULL;

    if (InterfaceGuid == NULL || InterfaceLuid == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (SiteId != 0)
    {
        return STATUS_NOT_SUPPORTED;
    }

    Status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceWithoutStatistics,
        &KeyTable,
        sizeof(NET_LUID),
        NULL,
        0,
        NULL,
        0,
        &StaticTable,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROS),
        &Count,
        0
        );

    if (!NT_SUCCESS(Status))
    {
        InterfaceLuid->Value = 0;
        return Status;
    }

    Status = STATUS_INVALID_PARAMETER;
    InterfaceLuid->Value = 0;

    for (Index = 0; Index < Count; ++Index)
    {
        if (PhTcpipGuidEquals(&StaticTable[Index].InterfaceGuid, InterfaceGuid))
        {
            InterfaceLuid->Value = KeyTable[Index].Value;
            Status = STATUS_SUCCESS;
            break;
        }
    }

    PhNsiFreeTable(KeyTable, NULL, NULL, StaticTable);
    return Status;
}

/**
 * Converts the index of an interface in another site to an interface LUID.
 *
 * \param SiteId The site (compartment) identifier of the remote interface.
 * \param InterfaceIndex The local index of the network interface.
 * \param InterfaceLuid Receives the interface LUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceindextoluid
 */
NTSTATUS PhConvertRemoteInterfaceIndexToLuid(
    _In_ ULONG SiteId,
    _In_ NET_IFINDEX InterfaceIndex,
    _Out_ PNET_LUID InterfaceLuid
    )
{
    NTSTATUS Status;
    NET_IFINDEX Key;

    Key = InterfaceIndex;

    if (SiteId != 0)
        return STATUS_NOT_SUPPORTED;

    Status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceIndex,
        &Key,
        sizeof(NET_IFINDEX),
        NsiParameterTypeReadOnlyStatic,
        InterfaceLuid,
        sizeof(NET_LUID),
        NsiNdisInterfaceIndexToLuid
        );

    if (!NT_SUCCESS(Status))
    {
        InterfaceLuid->Value = 0;
    }

    return Status;
}

/**
 * Converts the LUID of an interface in another site to its alias.
 *
 * \param SiteId The site (compartment) identifier of the remote interface.
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param InterfaceAlias Receives the null-terminated alias.
 * \param Length The size of InterfaceAlias, in characters.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoalias
 */
NTSTATUS PhConvertRemoteInterfaceLuidToAlias(
    _In_ ULONG SiteId,
    _In_ const NET_LUID* InterfaceLuid,
    _Out_writes_z_(Length) PWSTR InterfaceAlias,
    _In_ SIZE_T Length
    )
{
    NTSTATUS Status;
    ULONG64 Key;
    NSI_STRING_256 Alias;
    PCWSTR AliasString;
    ULONG CurrentLength;

    ZeroMemory(&Alias, sizeof(NSI_STRING_256));

    if (InterfaceLuid == NULL || InterfaceAlias == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Key = InterfaceLuid->Value;
    if (SiteId != 0)
        return STATUS_NOT_SUPPORTED;

    Status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &Key,
        sizeof(ULONG64),
        NsiParameterTypeReadWrite,
        &Alias,
        sizeof(NSI_STRING_256),
        NsiNdisIfType0Alias
        );

    if (NT_SUCCESS(Status))
    {
        CurrentLength = Alias.Length >> 1;
        if (CurrentLength > MAXIMUM_FILENAME_LENGTH)
        {
            CurrentLength = MAXIMUM_FILENAME_LENGTH;
        }

        Alias.Buffer[CurrentLength] = UNICODE_NULL;
        AliasString = Alias.Buffer;
    }
    else
    {
        AliasString = L"";
    }

    if (!NT_SUCCESS(Status))
    {
        return PhCopyStringZ(AliasString, SIZE_MAX, InterfaceAlias, Length, NULL);
    }

    return PhCopyStringZ(AliasString, SIZE_MAX, InterfaceAlias, Length, NULL);
}

/**
 * Converts the LUID of an interface in another site to its GUID.
 *
 * \param SiteId The site (compartment) identifier of the remote interface.
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param InterfaceGuid Receives the interface GUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoguid
 */
NTSTATUS PhConvertRemoteInterfaceLuidToGuid(
    _In_ ULONG SiteId,
    _In_ const NET_LUID* InterfaceLuid,
    _Out_ GUID* InterfaceGuid
    )
{
    NTSTATUS Status;
    ULONG64 Key;

    if (InterfaceLuid == NULL || InterfaceGuid == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Key = InterfaceLuid->Value;
    if (SiteId != 0)
        return STATUS_NOT_SUPPORTED;

    Status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &Key,
        sizeof(ULONG64),
        NsiParameterTypeReadOnlyStatic,
        InterfaceGuid,
        sizeof(GUID),
        NsiNdisInterfaceLuidToGuid);

    if (!NT_SUCCESS(Status))
    {
        ZeroMemory(InterfaceGuid, sizeof(GUID));
    }

    return Status;
}

/**
 * Converts the LUID of an interface in another site to its index.
 *
 * \param SiteId The site (compartment) identifier of the remote interface.
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param InterfaceIndex Receives the interface index.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoindex
 */
NTSTATUS PhConvertRemoteInterfaceLuidToIndex(
    _In_ ULONG SiteId,
    _In_ const NET_LUID* InterfaceLuid,
    _Out_ PNET_IFINDEX InterfaceIndex
    )
{
    NTSTATUS Status;
    ULONG64 Key;

    if (InterfaceLuid == NULL || InterfaceIndex == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Key = InterfaceLuid->Value;
    if (SiteId != 0)
        return STATUS_NOT_SUPPORTED;

    Status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &Key,
        sizeof(ULONG64),
        NsiParameterTypeReadOnlyStatic,
        InterfaceIndex,
        sizeof(NET_IFINDEX),
        NsiNdisInterfaceLuidToIndex
        );

    if (!NT_SUCCESS(Status))
    {
        *InterfaceIndex = 0;
    }

    return Status;
}

/**
 * Converts a GUID string to a GUID.
 *
 * \param SourceString The GUID string, with or without braces.
 * \param Guid Receives the GUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlguidfromstring
 */
NTSTATUS PhConvertStringToGuidW(
    _In_z_ PCWSTR SourceString,
    _Out_ GUID* Guid
    )
{
    UNICODE_STRING UnicodeGuid;

    ZeroMemory(&UnicodeGuid, sizeof(UNICODE_STRING));

    if (SourceString != NULL && Guid != NULL)
    {
        RtlInitUnicodeString(&UnicodeGuid, SourceString);

        if (RtlGUIDFromString(&UnicodeGuid, Guid) >= 0)
        {
            return STATUS_SUCCESS;
        }
    }

    return STATUS_INVALID_PARAMETER;
}

/**
 * Converts an ANSI GUID string to a GUID.
 *
 * \param SourceString The GUID string, with or without braces.
 * \param Guid Receives the GUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlguidfromstring
 */
NTSTATUS PhConvertStringToGuidA(
    _In_z_ PCSTR SourceString,
    _Out_ GUID* Guid
    )
{
    WCHAR WideGuid[RTL_GUID_STRING_SIZE + 1];
    NTSTATUS Status;

    if (SourceString == NULL || Guid == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = PhCopyStringZFromMultiByte(SourceString, strlen(SourceString), WideGuid, RTL_NUMBER_OF(WideGuid), NULL);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return PhConvertStringToGuidW(WideGuid, Guid);
}

/**
 * Parses a MAC address string ("AA-BB-CC-DD-EE-FF" or "AA:BB:CC:DD:EE:FF").
 *
 * \param InterfacePhysicalAddressString The MAC address string.
 * \param InterfacePhysicalAddress Receives the 6-byte physical address.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlethernetstringtoaddressw
 */
NTSTATUS PhConvertStringToInterfacePhysicalAddress(
    _In_z_ PCWSTR InterfacePhysicalAddressString,
    _Out_writes_bytes_(PHP_MAC_ADDRESS_LENGTH) PBYTE InterfacePhysicalAddress
)
{
    size_t Length;
    ULONG Index;

    Length = 0;

    Length = PhCountStringZ(InterfacePhysicalAddressString);

    if (Length != PHP_MAC_ADDRESS_STRING_LENGTH)
    {
        return STATUS_INVALID_PARAMETER;
    }

    for (Index = 0; Index < PHP_MAC_ADDRESS_STRING_LENGTH; ++Index)
    {
        WCHAR Character;
        ULONG ByteIndex;
        ULONG Column;
        BYTE Nibble;

        Character = InterfacePhysicalAddressString[Index];
        ByteIndex = Index / 3;
        Column = Index % 3;

        if (Column == 2)
        {
            if (Character != L'-')
            {
                return STATUS_INVALID_PARAMETER;
            }

            continue;
        }

        if (Character >= L'0' && Character <= L'9')
        {
            Nibble = (BYTE)(Character - L'0');
        }
        else if (Character >= L'a' && Character <= L'f')
        {
            Nibble = (BYTE)(Character - L'a' + 10);
        }
        else if (Character >= L'A' && Character <= L'F')
        {
            Nibble = (BYTE)(Character - L'A' + 10);
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (Column == 0)
        {
            InterfacePhysicalAddress[ByteIndex] = (BYTE)(Nibble << 4);
        }
        else if (Column == 1)
        {
            InterfacePhysicalAddress[ByteIndex] |= Nibble;
        }
    }

    return STATUS_SUCCESS;
}

ULONG PhIcmpv4ErrorToIpStatus(_In_ int Type, _In_ ULONG Code)
{
    switch (Type)
    {
    case ICMP4_DST_UNREACH:
        switch (Code)
        {
        case ICMP4_UNREACH_NET:
        case ICMP4_UNREACH_NET_UNKNOWN:
        case ICMP4_UNREACH_ISOLATED:
        case ICMP4_UNREACH_NET_ADMIN:
        case ICMP4_UNREACH_NET_TOS:
        case ICMP4_UNREACH_ADMIN:
            return IP_DEST_NET_UNREACHABLE;
        case ICMP4_UNREACH_HOST:
        case ICMP4_UNREACH_HOST_UNKNOWN:
        case ICMP4_UNREACH_HOST_ADMIN:
        case ICMP4_UNREACH_HOST_TOS:
            return IP_DEST_HOST_UNREACHABLE;
        case ICMP4_UNREACH_PROTOCOL:
            return IP_DEST_PROT_UNREACHABLE;
        case ICMP4_UNREACH_PORT:
            return IP_DEST_PORT_UNREACHABLE;
        case ICMP4_UNREACH_FRAG_NEEDED:
            return IP_PACKET_TOO_BIG;
        case ICMP4_UNREACH_SOURCEROUTE_FAILED:
            return IP_BAD_ROUTE;
        default:
            return IP_DEST_NET_UNREACHABLE;
        }
    case ICMP4_SOURCE_QUENCH:
        return IP_SOURCE_QUENCH;
    case ICMP4_TIME_EXCEEDED:
        if (Code == ICMP4_TIME_EXCEED_TRANSIT)
            return IP_TTL_EXPIRED_TRANSIT;
        if (Code == ICMP4_TIME_EXCEED_REASSEMBLY)
            return IP_TTL_EXPIRED_REASSEM;
        return IP_GENERAL_FAILURE;
    case ICMP4_PARAM_PROB:
        return IP_PARAM_PROBLEM;
    default:
        return IP_GENERAL_FAILURE;
    }
}

ULONG PhIcmpv6ErrorToIpStatus(_In_ int Type, _In_ int Code)
{
    switch (Type)
    {
    case ICMP6_DST_UNREACH:
        switch (Code)
        {
        case ICMP6_DST_UNREACH_NOROUTE:
        case ICMP6_DST_UNREACH_ADMIN:
        default:
            return IP_DEST_NO_ROUTE;
        case ICMP6_DST_UNREACH_BEYONDSCOPE:
            return IP_DEST_SCOPE_MISMATCH;
        case ICMP6_DST_UNREACH_ADDR:
            return IP_DEST_HOST_UNREACHABLE;
        case ICMP6_DST_UNREACH_NOPORT:
            return IP_DEST_PORT_UNREACHABLE;
        }
    case ICMP6_TIME_EXCEEDED:
        if (Code == ICMP6_TIME_EXCEED_TRANSIT)
            return IP_HOP_LIMIT_EXCEEDED;
        if (Code == ICMP6_TIME_EXCEED_REASSEMBLY)
            return IP_REASSEMBLY_TIME_EXCEEDED;
        return IP_GENERAL_FAILURE;
    case ICMP6_PARAM_PROB:
        return IP_PARAM_PROBLEM;
    default:
        return IP_GENERAL_FAILURE;
    }
}

/**
 * Converts the status block of a completed ICMP request into an IP_STATUS code.
 *
 * \param StatusBlock The status block of the request (status, reserved, ICMP type, ICMP code).
 * \param Ipv4 TRUE for an IPv4 request, FALSE for IPv6.
 * \return The IP_STATUS value.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2
 */
ULONG PhNtStatusToIpStatus(
    _In_reads_(PHP_ICMP_STATUS_BLOCK_LENGTH) const ULONG* StatusBlock,
    _In_ BOOLEAN Ipv4
    )
{
    if (StatusBlock[0] == NO_ERROR)
        return IP_SUCCESS;

    if (StatusBlock[0] == STATUS_UNSUCCESSFUL)
    {
        if (StatusBlock[1] == 0)
            return IP_GENERAL_FAILURE;

        return Ipv4
            ? PhIcmpv4ErrorToIpStatus((int)(StatusBlock[PHP_ICMP_ECHO_ERROR_TYPE_INDEX]), StatusBlock[PHP_ICMP_ECHO_ERROR_CODE_INDEX])
            : PhIcmpv6ErrorToIpStatus((int)(StatusBlock[PHP_ICMP_ECHO_ERROR_TYPE_INDEX]), (int)(StatusBlock[PHP_ICMP_ECHO_ERROR_CODE_INDEX]));
    }

    if (StatusBlock[0] == STATUS_INSUFFICIENT_RESOURCES)
        return IP_NO_RESOURCES;
    if (StatusBlock[0] == STATUS_DUPLICATE_NAME)
        return IP_DUPLICATE_ADDRESS;
    if (StatusBlock[0] == STATUS_CANCELLED)
        return IP_GENERAL_FAILURE;
    if (StatusBlock[0] == STATUS_INVALID_BUFFER_SIZE)
        return IP_PACKET_TOO_BIG;
    if (StatusBlock[0] == STATUS_IPSEC_QUEUE_OVERFLOW)
        return IP_NEGOTIATING_IPSEC;
    if (StatusBlock[0] == STATUS_TIMEOUT)
        return IP_REQ_TIMED_OUT;

    return IP_GENERAL_FAILURE;
}

#pragma endregion

#pragma region ICMP

/**
 * Completes an asynchronous echo request by signaling the caller's event or queueing the caller's APC.
 *
 * \param Data The ICMP_ECHO_REQUEST_CONTEXT of the request.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2
 */
VOID PhIcmpEchoRequestComplete(
    _In_ ULONG_PTR Data
    )
{
    ICMP_ECHO_REQUEST_CONTEXT* context;

    context = (ICMP_ECHO_REQUEST_CONTEXT*)(Data);
    
    if (context->EventHandle)
    {
        NtSetEvent(context->EventHandle, NULL);
        PhIcmpDestroyEchoRequestContext(context);
        return;
    }

    if (!NT_SUCCESS(NtQueueApcThread(
        context->ThreadHandle,
        PhIcmpEchoRequestCompleteWorker,
        (PVOID)Data,
        NULL,
        NULL
        )))
    {
        PhIcmpDestroyEchoRequestContext(context);
    }
}

/**
 * Returns the shared NSI RPC binding used for ICMP notifications, connecting on first use.
 *
 * \return The RPC binding handle, or NULL on failure (the last error is set).
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
PVOID PhNsiNotificationConnectToServer(
    VOID
    )
{
    ULONG lastError;

    lastError = ERROR_SUCCESS;
    if (PhBeginInitOnce(&NotificationInitOnce))
    {
        InitializeListHead(&PhpConnectivityHintListHead);
        PhEndInitOnce(&NotificationInitOnce);
    }

    PhAcquireQueuedLockExclusive(&NotificationCriticalSection);
    if (NotificationRpcHandle == NULL)
    {
        NotificationRpcHandle = PhNsiConnectToServer(0);
        lastError = PhGetLastError();
    }
    PhReleaseQueuedLockExclusive(&NotificationCriticalSection);

    PhSetLastError(lastError);
    return NotificationRpcHandle;
}

/**
 * NSI callback that receives an IPv4 echo reply, copies it into the caller's reply buffer and completes the request.
 *
 * \param Unknown Unused.
 * \param RequestPointer The echo request that completed.
 * \param Notification The NSI notification message that carries the reply.
 * \return ERROR_SUCCESS (NO_ERROR) on success, otherwise a Win32 error code.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2
 */
ULONG PhIcmpIndicateParameterChange(
    _In_opt_ PVOID Unknown,
    _Inout_ ICMP_REQUEST_RW** RequestPointer,
    _In_ ICMP_NOTIFICATION_MESSAGE* Notification
    )
{
    ICMP_REQUEST_RW* request;
    ICMP_HANDLE_INTERNAL* handle;
    ICMP_ECHO_REQUEST_CONTEXT* requestContext;
    ICMP_NOTIFICATION_V4* notificationData;
    PICMP_ECHO_REPLY reply;
    ULONG optionsSize;
    ULONG dataLength;

    request = *RequestPointer;
    if (request->ProcessId != HandleToUlong(NtCurrentProcessId()))
        return HandleToUlong(NtCurrentProcessId());

    handle = request->Handle;
    requestContext = request->RequestContext;
    notificationData = Notification->NotificationData.V4;

    PhpNsiSetAllParameters(
        NsiActive,
        IcmpNsiSetAllObjectCompleteEchoRequest,
        handle->ModuleId,
        NsiIcmpObjectEchoRequest,
        request,
        sizeof(ICMP_REQUEST_RW),
        NULL,
        0);

    RtlEnterCriticalSection(&handle->Lock);
    --handle->ActiveRequestCount;
    requestContext->Link.Blink->Flink = requestContext->Link.Flink;
    requestContext->Link.Flink->Blink = requestContext->Link.Blink;
    RtlLeaveCriticalSection(&handle->Lock);

    reply = (PICMP_ECHO_REPLY)(requestContext->ReplyBuffer);
    ZeroMemory(reply, sizeof(ICMP_ECHO_REPLY));

    dataLength = 0;
    optionsSize = 0;

    if ((notificationData->Header.Status == STATUS_UNSUCCESSFUL && notificationData->Header.HasIcmpError != 0)
        || notificationData->Header.Status == 0)
    {
        dataLength = (USHORT)notificationData->DataLength;
        optionsSize = notificationData->OptionsSize;
        reply->Address = notificationData->Address;
        reply->RoundTripTime = notificationData->RoundTripTime;
        reply->DataSize = (USHORT)(dataLength);
        reply->Reserved = 1;
        reply->Data = PTR_ADD_OFFSET(reply, sizeof(ICMP_ECHO_REPLY));
        reply->Options.Ttl = notificationData->Ttl;
        reply->Options.Tos = notificationData->Tos;
        reply->Options.Flags = notificationData->Flags;
        reply->Options.OptionsSize = (UCHAR)(optionsSize);
        reply->Options.OptionsData = PTR_ADD_OFFSET(reply->Data, dataLength);
    }

    reply->Status = PhNtStatusToIpStatus((const ULONG*)(&notificationData->Header), TRUE);
    requestContext->IoStatusBlock.Status = (NTSTATUS)(notificationData->Header.Status);
    requestContext->IoStatusBlock.Information = dataLength + optionsSize + sizeof(ICMP_ECHO_REPLY);
    PhIcmpEchoRequestComplete((ULONG_PTR)requestContext);
    return reply->Status;
}

/**
 * NSI callback that receives an IPv6 echo reply, copies it into the caller's reply buffer and completes the request.
 *
 * \param Unknown Unused.
 * \param RequestPointer The echo request that completed.
 * \param Notification The NSI notification message that carries the reply.
 * \return ERROR_SUCCESS (NO_ERROR) on success, otherwise a Win32 error code.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6sendecho2
 */
ULONG PhIcmp6IndicateParameterChange(
    _In_opt_ PVOID Unknown,
    _Inout_ ICMP_REQUEST_RW** RequestPointer,
    _In_ ICMP_NOTIFICATION_MESSAGE* Notification
    )
{
    ICMP_REQUEST_RW* request;
    ICMP_HANDLE_INTERNAL* handle;
    ICMP_ECHO_REQUEST_CONTEXT* requestContext;
    ICMP_NOTIFICATION_V6* notificationData;
    PICMPV6_ECHO_REPLY reply;
    ULONG replySize;

    request = *RequestPointer;
    if (request->ProcessId != HandleToUlong(NtCurrentProcessId()))
        return HandleToUlong(NtCurrentProcessId());

    handle = request->Handle;
    requestContext = request->RequestContext;
    notificationData = Notification->NotificationData.V6;
    replySize = sizeof(ICMPV6_ECHO_REPLY);

    PhpNsiSetAllParameters(
        NsiActive,
        IcmpNsiSetAllObjectCompleteEchoRequest,
        handle->ModuleId,
        NsiIcmpObjectEchoRequest,
        request,
        sizeof(ICMP_REQUEST_RW),
        NULL,
        0);

    RtlEnterCriticalSection(&handle->Lock);
    --handle->ActiveRequestCount;
    requestContext->Link.Blink->Flink = requestContext->Link.Flink;
    requestContext->Link.Flink->Blink = requestContext->Link.Blink;
    RtlLeaveCriticalSection(&handle->Lock);

    reply =(PICMPV6_ECHO_REPLY)(requestContext->ReplyBuffer);
    ZeroMemory(reply, sizeof(ICMPV6_ECHO_REPLY));

    if ((notificationData->Header.Status == STATUS_UNSUCCESSFUL && notificationData->Header.HasIcmpError != 0)
        || notificationData->Header.Status == 0)
    {
        memcpy(reply->Address.sin6_addr, &notificationData->Address, sizeof(notificationData->Address));
        reply->Address.sin6_scope_id = notificationData->ScopeId;
        reply->RoundTripTime = notificationData->RoundTripTime;
        replySize = sizeof(ICMPV6_ECHO_REPLY) + notificationData->DataLength;
    }

    reply->Status = PhNtStatusToIpStatus((const ULONG*)(&notificationData->Header), FALSE);
    requestContext->IoStatusBlock.Status = (NTSTATUS)(notificationData->Header.Status);
    requestContext->IoStatusBlock.Information = replySize;
    PhIcmpEchoRequestComplete((ULONG_PTR)(requestContext));
    return reply->Status;
}

/**
 * Submits an echo request to the ICMP NSI provider.
 *
 * \param RequestBuffer The ICMP_REQUEST_V4 or ICMP_REQUEST_V6 request.
 * \param RequestLength The size, in bytes, of RequestBuffer.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2
 */
NTSTATUS PhSubmitIcmpRequest(
    _In_reads_bytes_opt_(RequestLength) PVOID RequestBuffer,
    _In_ SIZE_T RequestLength)
{
    const ICMP_REQUEST_RW* request;
    ICMP_HANDLE_INTERNAL* handle;

    if (RequestBuffer == NULL || RequestLength < sizeof(ICMP_REQUEST_RW))
        return STATUS_INVALID_PARAMETER;

    request = (const ICMP_REQUEST_RW*)RequestBuffer;
    handle = request->Handle;

    if (handle == NULL)
        return STATUS_INVALID_PARAMETER;

    return PhpNsiSetAllParameters(
        NsiActive,
        IcmpNsiSetAllObjectSubmitEchoRequest,
        handle->ModuleId,
        NsiIcmpObjectEchoRequest,
        RequestBuffer,
        sizeof(ICMP_REQUEST_RW),
        PTR_ADD_OFFSET(RequestBuffer, sizeof(ICMP_REQUEST_RW)),
        (ULONG)(RequestLength - sizeof(ICMP_REQUEST_RW))
        );
}

/**
 * Releases an echo request context and the handles it owns.
 *
 * \param Context The request context. NULL is ignored.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpclosehandle
 */
VOID PhIcmpDestroyEchoRequestContext(
    _Inout_ ICMP_ECHO_REQUEST_CONTEXT* Context
    )
{
    if (Context == NULL)
        return;

    if (Context->ThreadHandle != NULL)
        NtClose(Context->ThreadHandle);

    PhFree(Context);
}

/**
 * Allocates the context of an echo request and links it to the ICMP handle. Synchronous requests get a private wait event.
 *
 * \param IcmpHandle The internal ICMP handle.
 * \param Event An optional event signaled when the reply arrives (asynchronous mode).
 * \param ApcRoutine An optional APC routine called when the reply arrives (asynchronous mode).
 * \param ApcContext An optional context passed to ApcRoutine.
 * \param ReplyBuffer The caller's reply buffer.
 * \param RequestContext Receives the request context.
 * \param WaitEvent Receives the event to wait on for synchronous requests.
 * \param IsAsynchronous Receives TRUE if Event or ApcRoutine was specified.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2
 */
NTSTATUS PhAllocateEchoRequestContext(
    _Inout_ ICMP_HANDLE_INTERNAL* IcmpHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _In_opt_ PVOID ReplyBuffer,
    _Outptr_ ICMP_ECHO_REQUEST_CONTEXT** RequestContext,
    _Out_opt_ HANDLE* WaitEvent,
    _Out_ BOOLEAN* IsAsynchronous
    )
{
    HANDLE completionEvent;
    HANDLE duplicatedThread;
    ICMP_ECHO_REQUEST_CONTEXT* context;

    *RequestContext = NULL;
    *IsAsynchronous = FALSE;

    if (WaitEvent != NULL)
        *WaitEvent = NULL;

    completionEvent = Event;
    duplicatedThread = NULL;

    if (Event != NULL)
    {
        *IsAsynchronous = TRUE;
    }
    else if (ApcRoutine != NULL)
    {
        HANDLE currentProcess;

        *IsAsynchronous = TRUE;
        currentProcess = NtCurrentProcess();
        {
            NTSTATUS status = NtDuplicateObject(
                currentProcess,
                NtCurrentThread(),
                currentProcess,
                &duplicatedThread,
                0,
                0,
                DUPLICATE_SAME_ACCESS
                );

            if (!NT_SUCCESS(status))
                return status;
        }
    }
    else
    {
        NTSTATUS status = NtCreateEvent(&completionEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
        if (!NT_SUCCESS(status))
            return status;
        if (WaitEvent != NULL)
            *WaitEvent = completionEvent;
    }

    context = PhAllocateZeroSafe(sizeof(ICMP_ECHO_REQUEST_CONTEXT));
    if (context == NULL)
    {
        if (WaitEvent && *WaitEvent)
        {
            NtClose(*WaitEvent);
        }

        if (duplicatedThread)
        {
            NtClose(duplicatedThread);
        }

        return STATUS_NO_MEMORY;
    }

    memset(context, 0, sizeof(ICMP_ECHO_REQUEST_CONTEXT));
    context->EventHandle = completionEvent;
    context->ThreadHandle = duplicatedThread;
    context->ApcRoutine = ApcRoutine;
    context->ApcContext = ApcContext;
    context->ReplyBuffer = ReplyBuffer;

    RtlEnterCriticalSection(&IcmpHandle->Lock);
    ++IcmpHandle->ActiveRequestCount;
    InsertTailList(&IcmpHandle->RequestList, &context->Link);
    RtlLeaveCriticalSection(&IcmpHandle->Lock);

    *RequestContext = context;
    return STATUS_SUCCESS;
}

/**
 * Opens an ICMP handle for the specified network layer module.
 *
 * \param ModuleId NPI_MS_IPV4_MODULEID or NPI_MS_IPV6_MODULEID.
 * \param IsIpv4 TRUE for an IPv4 handle, FALSE for IPv6.
 * \return The ICMP handle, or INVALID_HANDLE_VALUE on failure (the last error is set).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpcreatefile
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6createfile
 */
HANDLE PhIcmpCreateFileEx(
    _In_ const NPI_MODULEID* ModuleId,
    _In_ BOOLEAN IsIpv4)
{
    ICMP_HANDLE_INTERNAL* handle;
    PVOID rpcHandle;
    NTSTATUS status;

    handle = (ICMP_HANDLE_INTERNAL*)PhAllocate(sizeof(ICMP_HANDLE_INTERNAL));
    if (handle == NULL)
        return INVALID_HANDLE_VALUE;
    memset(handle, 0, sizeof(ICMP_HANDLE_INTERNAL));

    rpcHandle = PhNsiNotificationConnectToServer();
    if (rpcHandle == NULL)
    {
        ULONG lastError;

        lastError = PhGetLastError();
        PhFree(handle);
        PhSetLastError(lastError);
        return INVALID_HANDLE_VALUE;
    }

    NotificationRpcHandle = rpcHandle;

    PhAcquireQueuedLockExclusive(&PingNotificationLock);

    if (IsIpv4)
    {
        if (PingNotificationCountV4 == 0)
        {
            PingNotificationHandleV4 = NULL;

            status = PhNsiRpcRegisterChangeNotification(
                rpcHandle,
                ModuleId,
                NsiIcmpObjectEchoRequest,
                PhIcmpIndicateParameterChange,
                FALSE,
                handle,
                &PingNotificationHandleV4);

            if (!NT_SUCCESS(status))
            {
                PhReleaseQueuedLockExclusive(&PingNotificationLock);
                PhFree(handle);
                PhSetLastError(PhNtStatusToDosError(status));
                return INVALID_HANDLE_VALUE;
            }
        }

        ++PingNotificationCountV4;
    }
    else
    {
        if (PingNotificationCountV6 == 0)
        {
            PingNotificationHandleV6 = NULL;

            status = PhNsiRpcRegisterChangeNotification(
                rpcHandle,
                ModuleId,
                NsiIcmpObjectEchoRequest,
                PhIcmp6IndicateParameterChange,
                FALSE,
                handle,
                &PingNotificationHandleV6
                );

            if (!NT_SUCCESS(status))
            {
                PhReleaseQueuedLockExclusive(&PingNotificationLock);
                PhFree(handle);
                PhSetLastError(PhNtStatusToDosError(status));
                return INVALID_HANDLE_VALUE;
            }
        }

        ++PingNotificationCountV6;
    }

    PhReleaseQueuedLockExclusive(&PingNotificationLock);

    memset(handle, 0, sizeof(ICMP_HANDLE_INTERNAL));
    handle->ProcessId = HandleToUlong(NtCurrentProcessId());
    handle->IsIpv4 = IsIpv4;
    handle->ModuleId = ModuleId;
    RtlInitializeCriticalSection(&handle->Lock);
    InitializeListHead(&handle->RequestList);
    return handle;
}

/**
 * Opens a handle on which IPv6 ICMP echo requests can be issued.
 *
 * \return The ICMP handle, or INVALID_HANDLE_VALUE on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6createfile
 */
HANDLE PhIcmp6CreateFile(
    VOID
    )
{
    return PhIcmpCreateFileEx(&NPI_MS_IPV6_MODULEID, FALSE);
}

/**
 * Opens a handle on which IPv4 ICMP echo requests can be issued.
 *
 * \return The ICMP handle, or INVALID_HANDLE_VALUE on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpcreatefile
 */
HANDLE PhIcmpCreateFile(
    VOID
    )
{
    return PhIcmpCreateFileEx(&NPI_MS_IPV4_MODULEID, TRUE);
}

/**
 * Unlinks an echo request context from its ICMP handle.
 *
 * \param IcmpHandle The internal ICMP handle.
 * \param RequestContext The request context.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpclosehandle
 */
static VOID PhpRemoveEchoRequestContext(
    _Inout_ ICMP_HANDLE_INTERNAL* IcmpHandle,
    _Inout_ ICMP_ECHO_REQUEST_CONTEXT* RequestContext
    )
{
    RtlEnterCriticalSection(&IcmpHandle->Lock);
    --IcmpHandle->ActiveRequestCount;
    RemoveEntryList(&RequestContext->Link);
    RtlLeaveCriticalSection(&IcmpHandle->Lock);
}

/**
 * Validates the reply buffer of an IPv6 echo request.
 *
 * \param ReplyBuffer The buffer that receives the echo replies.
 * \param ReplySize The size, in bytes, of ReplyBuffer.
 * \return 1 if the buffer holds a successful reply, otherwise 0 (the last error is set).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6parsereplies
 */
ULONG PhIcmp6ParseReplies(
    _In_reads_bytes_(ReplySize) PVOID ReplyBuffer,
    _In_ ULONG ReplySize
    )
{
    NTSTATUS status;

    if (ReplyBuffer == NULL || ReplySize == 0)
    {
        PhSetLastError(ERROR_GEN_FAILURE);
        return 0;
    }

    status = ((PICMPV6_ECHO_REPLY)(ReplyBuffer))->Status;
    if (status == IP_SUCCESS || status == IP_TTL_EXPIRED_TRANSIT)
        return 1;

    PhSetLastError(status);
    return 0;
}

/**
 * Sends an IPv6 ICMP echo request, synchronously or asynchronously.
 *
 * \param IcmpHandle The ICMP handle returned by PhIcmpCreateFile or PhIcmp6CreateFile.
 * \param Event An optional event signaled when the reply arrives (asynchronous mode).
 * \param ApcRoutine An optional APC routine called when the reply arrives (asynchronous mode).
 * \param ApcContext An optional context passed to ApcRoutine.
 * \param SourceAddress The source address.
 * \param DestinationAddress The destination address.
 * \param RequestData The data to send in the echo request.
 * \param RequestSize The size, in bytes, of RequestData.
 * \param RequestOptions Optional IP header options for the request.
 * \param ReplyBuffer The buffer that receives the echo replies.
 * \param ReplySize The size, in bytes, of ReplyBuffer.
 * \param Timeout The time, in milliseconds, to wait for replies.
 * \return The number of replies for synchronous requests, otherwise 0 (the last error is set; ERROR_IO_PENDING for asynchronous requests).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6sendecho2
 */
ULONG PhIcmp6SendEcho2(
    _In_ HANDLE IcmpHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _In_ PSOCKADDR_IN6 SourceAddress,
    _In_ PSOCKADDR_IN6 DestinationAddress,
    _In_reads_bytes_(RequestSize) PVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) PVOID ReplyBuffer,
    _In_ ULONG ReplySize,
    _In_ ULONG Timeout
    )
{
    ICMP_HANDLE_INTERNAL* handle;
    ICMP_ECHO_REQUEST_CONTEXT* requestContext;
    ICMP_REQUEST_V6 request;
    HANDLE waitEvent;
    BOOLEAN asynchronous;
    NTSTATUS status;

    if (PhIsInvalidIcmpHandle(IcmpHandle))
    {
        PhSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    handle = (ICMP_HANDLE_INTERNAL*)(IcmpHandle);
    if (handle->IsIpv4)
    {
        PhSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    status = PhAllocateEchoRequestContext(
        handle,
        Event,
        ApcRoutine,
        ApcContext,
        ReplyBuffer,
        &requestContext,
        &waitEvent,
        &asynchronous
        );

    if (!NT_SUCCESS(status))
    {
        PhSetLastError(PhNtStatusToDosError(status));
        return 0;
    }

    ZeroMemory(&request, sizeof(ICMP_REQUEST_V6));
    request.ReadWrite.Handle = handle;
    request.ReadWrite.ProcessId = handle->ProcessId;
    request.ReadWrite.ReplyBuffer = ReplyBuffer;
    request.ReadWrite.RequestContext = requestContext;
    request.RequestSize = RequestSize;
    request.ReplySize = ReplySize;
    request.RequestData = RequestData;
    request.ReplyBuffer = ReplyBuffer;
    request.ReplyHeaderSize = sizeof(ICMPV6_ECHO_REPLY);
    request.Timeout = Timeout;

    if (RequestOptions)
    {
        request.Ttl = RequestOptions->Ttl;
        request.Reverse = RequestOptions->Flags & IP_FLAG_REVERSE;
        request.OptionsSize = RequestOptions->OptionsSize;
        request.OptionsData = RequestOptions->OptionsData;
    }
    else
    {
        request.Ttl = MAXUCHAR;
    }
    request.SourceScopeId = SourceAddress->sin6_scope_id;
    request.DestinationScopeId = DestinationAddress->sin6_scope_id;
    request.SourceAddress = SourceAddress->sin6_addr;
    request.DestinationAddress = DestinationAddress->sin6_addr;

    status = PhSubmitIcmpRequest(&request, sizeof(ICMP_REQUEST_V6));

    if (!NT_SUCCESS(status))
    {
        PhpRemoveEchoRequestContext(handle, requestContext);

        if (waitEvent)
        {
            NtClose(waitEvent);
        }

        PhSetLastError(PhNtStatusToDosError(status));
        PhIcmpDestroyEchoRequestContext(requestContext);
        return 0;
    }

    if (asynchronous)
    {
        PhSetLastError(ERROR_IO_PENDING);
        return 0;
    }

    NtWaitForSingleObject(waitEvent, FALSE, NULL);
    NtClose(waitEvent);

    return PhIcmp6ParseReplies(ReplyBuffer, ReplySize);
}

/**
 * Parses the reply buffer of an IPv4 echo request.
 *
 * \param ReplyBuffer The buffer that receives the echo replies.
 * \param ReplySize The size, in bytes, of ReplyBuffer.
 * \return The number of replies in the buffer, or 0 on failure (the last error is set).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpparsereplies
 */
ULONG PhIcmpParseReplies(
    _In_reads_bytes_(ReplySize) PVOID ReplyBuffer,
    _In_ ULONG ReplySize
    )
{
    PICMP_ECHO_REPLY reply;
    ULONG count;

    if (ReplyBuffer == NULL || ReplySize == 0)
    {
        PhSetLastError(ERROR_GEN_FAILURE);
        return 0;
    }

    reply = (PICMP_ECHO_REPLY)(ReplyBuffer);
    count = reply->Reserved;
    reply->Reserved = 0;
    if (count == 0)
        PhSetLastError(reply->Status);
    return count;
}

/**
 * Sends an IPv4 ICMP echo request from the specified source address, synchronously or asynchronously.
 *
 * \param IcmpHandle The ICMP handle returned by PhIcmpCreateFile or PhIcmp6CreateFile.
 * \param Event An optional event signaled when the reply arrives (asynchronous mode).
 * \param ApcRoutine An optional APC routine called when the reply arrives (asynchronous mode).
 * \param ApcContext An optional context passed to ApcRoutine.
 * \param SourceAddress The source address.
 * \param DestinationAddress The destination address.
 * \param RequestData The data to send in the echo request.
 * \param RequestSize The size, in bytes, of RequestData.
 * \param RequestOptions Optional IP header options for the request.
 * \param ReplyBuffer The buffer that receives the echo replies.
 * \param ReplySize The size, in bytes, of ReplyBuffer.
 * \param Timeout The time, in milliseconds, to wait for replies.
 * \return The number of replies for synchronous requests, otherwise 0 (the last error is set; ERROR_IO_PENDING for asynchronous requests).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2ex
 */
ULONG PhIcmpSendEcho2Ex(
    _In_ HANDLE IcmpHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _In_ IPAddr SourceAddress,
    _In_ IPAddr DestinationAddress,
    _In_reads_bytes_(RequestSize) PVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) PVOID ReplyBuffer,
    _In_ ULONG ReplySize,
    _In_ ULONG Timeout
    )
{
    ICMP_HANDLE_INTERNAL* handle;
    ICMP_ECHO_REQUEST_CONTEXT* requestContext;
    ICMP_REQUEST_V4 request;
    HANDLE waitEvent;
    BOOLEAN asynchronous;
    NTSTATUS status;
    ULONG parsedReplySize;
    PICMP_ECHO_REPLY reply;

    if (PhIsInvalidIcmpHandle(IcmpHandle))
    {
        PhSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    handle = (ICMP_HANDLE_INTERNAL*)(IcmpHandle);
    if (!handle->IsIpv4 || ReplySize < sizeof(ICMP_ECHO_REPLY))
    {
        PhSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    status = PhAllocateEchoRequestContext(
        handle,
        Event, 
        ApcRoutine, 
        ApcContext,
        ReplyBuffer, 
        &requestContext,
        &waitEvent, 
        &asynchronous);

    if (!NT_SUCCESS(status))
    {
        PhSetLastError(PhNtStatusToDosError(status));
        return 0;
    }

    ZeroMemory(&request, sizeof(ICMP_REQUEST_V4));
    request.ReadWrite.Handle = handle;
    request.ReadWrite.ProcessId = handle->ProcessId;
    request.ReadWrite.ReplyBuffer = ReplyBuffer;
    request.ReadWrite.RequestContext = requestContext;
    request.RequestSize = RequestSize;
    request.ReplySize = ReplySize;
    request.RequestData = RequestData;
    request.ReplyBuffer = ReplyBuffer;
    request.ReplyHeaderSize = sizeof(ICMP_ECHO_REPLY);
    request.Timeout = Timeout;

    if (RequestOptions)
    {
        request.Ttl = RequestOptions->Ttl;
        request.Reverse = RequestOptions->Flags & IP_FLAG_REVERSE;
        request.DontFragment = (RequestOptions->Flags & IP_FLAG_DF) != 0;
        request.OptionsSize = RequestOptions->OptionsSize;
        request.OptionsData = RequestOptions->OptionsData;
    }
    else
    {
        request.Ttl = MAXUCHAR;
    }
    request.SourceAddress = SourceAddress;
    request.DestinationAddress = DestinationAddress;

    status = PhSubmitIcmpRequest(&request, sizeof(ICMP_REQUEST_V4));

    if (!NT_SUCCESS(status))
    {
        PhpRemoveEchoRequestContext(handle, requestContext);
        if (waitEvent != NULL)
            NtClose(waitEvent);
        PhSetLastError(PhNtStatusToDosError(status));
        PhIcmpDestroyEchoRequestContext(requestContext);
        return 0;
    }

    if (asynchronous)
    {
        PhSetLastError(ERROR_IO_PENDING);
        return 0;
    }

    NtWaitForSingleObject(waitEvent, FALSE, NULL);
    NtClose(waitEvent);

    reply = (PICMP_ECHO_REPLY)(ReplyBuffer);
    parsedReplySize = sizeof(ICMP_ECHO_REPLY) + reply->DataSize + reply->Options.OptionsSize;
    return PhIcmpParseReplies(ReplyBuffer, parsedReplySize);
}

/**
 * Sends an IPv4 ICMP echo request and waits for the replies.
 *
 * \param IcmpHandle The ICMP handle returned by PhIcmpCreateFile or PhIcmp6CreateFile.
 * \param DestinationAddress The destination address.
 * \param RequestData The data to send in the echo request.
 * \param RequestSize The size, in bytes, of RequestData.
 * \param RequestOptions Optional IP header options for the request.
 * \param ReplyBuffer The buffer that receives the echo replies.
 * \param ReplySize The size, in bytes, of ReplyBuffer.
 * \param Timeout The time, in milliseconds, to wait for replies.
 * \return The number of replies, or 0 on failure (the last error is set).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho
 */
ULONG PhIcmpSendEcho(
    _In_ HANDLE IcmpHandle,
    _In_ IPAddr DestinationAddress,
    _In_reads_bytes_(RequestSize) PVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) PVOID ReplyBuffer,
    _In_ ULONG ReplySize,
    _In_ ULONG Timeout
    )
{
    return PhIcmpSendEcho2Ex(
        IcmpHandle,
        NULL,
        NULL,
        NULL,
        0,
        DestinationAddress,
        RequestData,
        RequestSize,
        RequestOptions,
        ReplyBuffer,
        ReplySize,
        Timeout);
}

/**
 * Sends an IPv4 ICMP echo request, synchronously or asynchronously.
 *
 * \param IcmpHandle The ICMP handle returned by PhIcmpCreateFile or PhIcmp6CreateFile.
 * \param Event An optional event signaled when the reply arrives (asynchronous mode).
 * \param ApcRoutine An optional APC routine called when the reply arrives (asynchronous mode).
 * \param ApcContext An optional context passed to ApcRoutine.
 * \param DestinationAddress The destination address.
 * \param RequestData The data to send in the echo request.
 * \param RequestSize The size, in bytes, of RequestData.
 * \param RequestOptions Optional IP header options for the request.
 * \param ReplyBuffer The buffer that receives the echo replies.
 * \param ReplySize The size, in bytes, of ReplyBuffer.
 * \param Timeout The time, in milliseconds, to wait for replies.
 * \return The number of replies for synchronous requests, otherwise 0 (the last error is set; ERROR_IO_PENDING for asynchronous requests).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2
 */
ULONG PhIcmpSendEcho2(
    _In_ HANDLE IcmpHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _In_ IPAddr DestinationAddress,
    _In_reads_bytes_(RequestSize) PVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) PVOID ReplyBuffer,
    _In_ ULONG ReplySize,
    _In_ ULONG Timeout
    )
{
    return PhIcmpSendEcho2Ex(
        IcmpHandle,
        Event,
        ApcRoutine,
        ApcContext,
        0,
        DestinationAddress,
        RequestData,
        RequestSize,
        RequestOptions,
        ReplyBuffer,
        ReplySize,
        Timeout);
}

/**
 * Retrieves ICMP statistics for IPv4 or IPv6.
 *
 * \param Statistics Receives the statistics.
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-geticmpstatisticsex
 */
NTSTATUS PhGetIcmpStatisticsEx(
    _Out_ PMIB_ICMP_EX Statistics,
    _In_ ULONG Family
    )
{
    const NPI_MODULEID* moduleId;
    BYTE buffer[sizeof(MIB_ICMP_EX_XPSP1)];
    NTSTATUS status;

    memset(buffer, 0, sizeof(MIB_ICMP_EX_XPSP1));
    if (Statistics == NULL)
        return STATUS_INVALID_PARAMETER;

    if (Family == AF_INET)
        moduleId = &NPI_MS_IPV4_MODULEID;
    else if (Family == AF_INET6)
        moduleId = &NPI_MS_IPV6_MODULEID;
    else
        return STATUS_INVALID_PARAMETER;

    status = PhNsiGetAllParameters(
        NsiActive,
        moduleId,
        NsiIpObjectIcmp,
        NULL,
        0,
        NULL,
        0,
        buffer,
            sizeof(MIB_ICMP_EX_XPSP1),
        NULL,
        0);

    if (!NT_SUCCESS(status))
        return status;

    memcpy(Statistics, buffer, sizeof(MIB_ICMP_EX_XPSP1));
    return STATUS_SUCCESS;
}

/**
 * Retrieves IPv4 ICMP statistics.
 *
 * \param Statistics Receives the statistics.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-geticmpstatistics
 */
NTSTATUS PhGetIcmpStatistics(
    _Out_ PMIB_ICMP Statistics
    )
{
    MIB_ICMP_EX extendedStatistics;
    NTSTATUS status;

    memset(&extendedStatistics, 0, sizeof(MIB_ICMP_EX));
    status = PhGetIcmpStatisticsEx(&extendedStatistics, AF_INET);

    if (!NT_SUCCESS(status))
        return status;

    Statistics->stats.icmpInStats.dwMsgs = extendedStatistics.icmpInStats.dwMsgs;
    Statistics->stats.icmpInStats.dwErrors = extendedStatistics.icmpInStats.dwErrors;
    Statistics->stats.icmpInStats.dwDestUnreachs = extendedStatistics.icmpInStats.rgdwTypeCount[ICMP4_DST_UNREACH];
    Statistics->stats.icmpInStats.dwTimeExcds = extendedStatistics.icmpInStats.rgdwTypeCount[ICMP4_TIME_EXCEEDED];
    Statistics->stats.icmpInStats.dwParmProbs = extendedStatistics.icmpInStats.rgdwTypeCount[ICMP4_PARAM_PROB];
    Statistics->stats.icmpInStats.dwSrcQuenchs = extendedStatistics.icmpInStats.rgdwTypeCount[ICMP4_SOURCE_QUENCH];
    Statistics->stats.icmpInStats.dwRedirects = extendedStatistics.icmpInStats.rgdwTypeCount[ICMP4_REDIRECT];
    Statistics->stats.icmpInStats.dwEchos = extendedStatistics.icmpInStats.rgdwTypeCount[ICMP4_ECHO_REQUEST];
    Statistics->stats.icmpInStats.dwEchoReps = extendedStatistics.icmpInStats.rgdwTypeCount[ICMP4_ECHO_REPLY];
    Statistics->stats.icmpInStats.dwTimestamps = extendedStatistics.icmpInStats.rgdwTypeCount[ICMP4_TIMESTAMP_REQUEST];
    Statistics->stats.icmpInStats.dwTimestampReps = extendedStatistics.icmpInStats.rgdwTypeCount[ICMP4_TIMESTAMP_REPLY];
    Statistics->stats.icmpInStats.dwAddrMasks = extendedStatistics.icmpInStats.rgdwTypeCount[ICMP4_MASK_REQUEST];
    Statistics->stats.icmpInStats.dwAddrMaskReps = extendedStatistics.icmpInStats.rgdwTypeCount[ICMP4_MASK_REPLY];
    Statistics->stats.icmpOutStats.dwMsgs = extendedStatistics.icmpOutStats.dwMsgs;
    Statistics->stats.icmpOutStats.dwErrors = extendedStatistics.icmpOutStats.dwErrors;
    Statistics->stats.icmpOutStats.dwDestUnreachs = extendedStatistics.icmpOutStats.rgdwTypeCount[ICMP4_DST_UNREACH];
    Statistics->stats.icmpOutStats.dwTimeExcds = extendedStatistics.icmpOutStats.rgdwTypeCount[ICMP4_TIME_EXCEEDED];
    Statistics->stats.icmpOutStats.dwParmProbs = extendedStatistics.icmpOutStats.rgdwTypeCount[ICMP4_PARAM_PROB];
    Statistics->stats.icmpOutStats.dwSrcQuenchs = extendedStatistics.icmpOutStats.rgdwTypeCount[ICMP4_SOURCE_QUENCH];
    Statistics->stats.icmpOutStats.dwRedirects = extendedStatistics.icmpOutStats.rgdwTypeCount[ICMP4_REDIRECT];
    Statistics->stats.icmpOutStats.dwEchos = extendedStatistics.icmpOutStats.rgdwTypeCount[ICMP4_ECHO_REQUEST];
    Statistics->stats.icmpOutStats.dwEchoReps = extendedStatistics.icmpOutStats.rgdwTypeCount[ICMP4_ECHO_REPLY];
    Statistics->stats.icmpOutStats.dwTimestamps = extendedStatistics.icmpOutStats.rgdwTypeCount[ICMP4_TIMESTAMP_REQUEST];
    Statistics->stats.icmpOutStats.dwTimestampReps = extendedStatistics.icmpOutStats.rgdwTypeCount[ICMP4_TIMESTAMP_REPLY];
    Statistics->stats.icmpOutStats.dwAddrMasks = extendedStatistics.icmpOutStats.rgdwTypeCount[ICMP4_MASK_REQUEST];
    Statistics->stats.icmpOutStats.dwAddrMaskReps = extendedStatistics.icmpOutStats.rgdwTypeCount[ICMP4_MASK_REPLY];
    return STATUS_SUCCESS;
}

/**
 * Opens an IPv4 or IPv6 ICMP handle.
 *
 * \param IsIpv4 TRUE for an IPv4 handle, FALSE for IPv6.
 * \return The ICMP handle, or INVALID_HANDLE_VALUE on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpcreatefile
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6createfile
 */
HANDLE PhInternalIcmpCreateFileEx(
    _In_ BOOLEAN IsIpv4
    )
{
    return PhIcmpCreateFileEx(IsIpv4 ? &NPI_MS_IPV4_MODULEID : &NPI_MS_IPV6_MODULEID, IsIpv4 != 0);
}

#pragma endregion

#pragma region Network Tables and Queries

//
// ARP and IPv6 Neighbor management routines.
//

/**
 * Retrieves the interface index of an interface LUID.
 *
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \return The interface index, or 0 on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoindex
 */
ULONG PhQueryNeighborInterfaceIndex(
    _In_ const NET_LUID* InterfaceLuid
    )
{
    ULONG ifIndex = 0;
    NET_LUID interfaceLuid = *InterfaceLuid;

    if (PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &interfaceLuid,
        sizeof(NET_LUID),
        NsiParameterTypeReadOnlyStatic,
        &ifIndex,
        sizeof(ULONG),
        NsiNdisInterfaceLuidToIndex
        ) != STATUS_SUCCESS)
    {
        ifIndex = 0;
    }

    return ifIndex;
}

/**
 * Retrieves extended statistics for an IPv4 TCP connection.
 *
 * \param Row The TCP row that identifies the connection.
 * \param EstatsType The type of extended statistics.
 * \param Rw Receives the read-write information.
 * \param RwVersion The version of the read-write structure; must be zero.
 * \param RwSize The size, in bytes, of Rw.
 * \param Ros Receives the read-only static information.
 * \param RosVersion The version of the read-only static structure; must be zero.
 * \param RosSize The size, in bytes, of Ros.
 * \param Rod Receives the read-only dynamic information.
 * \param RodVersion The version of the read-only dynamic structure; must be zero.
 * \param RodSize The size, in bytes, of Rod.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getpertcpconnectionestats
 */
_Success_(NT_SUCCESS(return))
NTSTATUS PhGetPerTcpConnectionEStats(
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
    )
{
    PVOID rwBuffer = NULL;
    PVOID rodBuffer = NULL;
    PVOID rosBuffer = NULL;
    ULONG rwBufferSize = 0;
    ULONG rodBufferSize = 0;
    ULONG rosBufferSize = 0;
    ULONG objectId;
    NTSTATUS status;
    NSI_ESTATS_TCP_KEY_V4 key;

    //if (Rw) *Rw = NULL;
    //if (Ros) *Ros = NULL;
    //if (Rod) *Rod = NULL;

    // Keep the local wrapper aligned with tcpip!GetPerTcpConnectionEStats.
    if ((!RwSize || Rw) && (!RosSize || Ros) && (Rod || !RodSize))
    {
        switch (EstatsType)
        {
        case TcpConnectionEstatsSynOpts:
            objectId = MibTcpEstatsTypeSynOpts;
            rwBufferSize = 0;
            rodBufferSize = 0;
            rosBufferSize = sizeof(TCP_ESTATS_SYN_OPTS_ROS_v0);
            break;
        case TcpConnectionEstatsData:
            objectId = MibTcpEstatsTypeData;
            rwBufferSize = sizeof(TCP_ESTATS_DATA_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_DATA_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsSndCong:
            objectId = MibTcpEstatsTypeSndCong;
            rwBufferSize = sizeof(NSI_ESTATS_SND_CONG_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_SND_CONG_ROD_v0);
            rosBufferSize = sizeof(TCP_ESTATS_SND_CONG_ROS_v0);
            break;
        case TcpConnectionEstatsPath:
            objectId = MibTcpEstatsTypePath;
            rwBufferSize = sizeof(TCP_ESTATS_PATH_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_PATH_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsSendBuff:
            objectId = MibTcpEstatsTypeSendBuff;
            rwBufferSize = sizeof(TCP_ESTATS_SEND_BUFF_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_SEND_BUFF_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsRec:
            objectId = MibTcpEstatsTypeRec;
            rwBufferSize = sizeof(TCP_ESTATS_REC_RW_v0);
            rodBufferSize = sizeof(NSI_ESTATS_REC_ROD_INTERNAL);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsObsRec:
            objectId = MibTcpEstatsTypeObsRec;
            rwBufferSize = sizeof(TCP_ESTATS_OBS_REC_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_OBS_REC_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsBandwidth:
            objectId = MibTcpEstatsTypeBandwidth;
            rwBufferSize = sizeof(TCP_ESTATS_BANDWIDTH_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_BANDWIDTH_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsFineRtt:
            objectId = MibTcpEstatsTypeFineRtt;
            rwBufferSize = sizeof(TCP_ESTATS_FINE_RTT_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_FINE_RTT_ROD_v0);
            rosBufferSize = 0;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }

        memset(&key, 0, sizeof(NSI_ESTATS_TCP_KEY_V4));
        key.AddressFamily = AF_INET;
        key.LocalPort = (USHORT)Row->dwLocalPort;
        key.LocalAddress = Row->dwLocalAddr;
        key.RemoteAddressFamily = AF_INET;
        key.RemoteAddress = Row->dwRemoteAddr;
        key.RemotePort = (USHORT)Row->dwRemotePort;

        if (RwSize && rwBufferSize != 0)
        {
            rwBuffer = PhAllocate(rwBufferSize);
        }

        if (RodSize && rodBufferSize != 0)
        {
            rodBuffer = PhAllocate(rodBufferSize);
        }

        if (RosSize && rosBufferSize != 0)
        {
            rosBuffer = PhAllocate(rosBufferSize);
        }

        status = PhNsiGetAllParameters(
            NsiActive,
            &NPI_MS_TCP_MODULEID,
            objectId,
            &key,
            sizeof(NSI_ESTATS_TCP_KEY_V4),
            rwBuffer,
            RwSize ? rwBufferSize : 0,
            rodBuffer,
            RodSize ? rodBufferSize : 0,
            rosBuffer,
            RosSize ? rosBufferSize : 0
            );

        if (!NT_SUCCESS(status))
            goto Cleanup;

        switch (EstatsType)
        {
        case TcpConnectionEstatsSynOpts:
            if (RosSize)
            {
                if (RosVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RosSize != sizeof(TCP_ESTATS_SYN_OPTS_ROS_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_SYN_OPTS_ROS_v0)Ros = *(PTCP_ESTATS_SYN_OPTS_ROS_v0)rosBuffer;
            }
            break;

        case TcpConnectionEstatsData:
            if (RodSize)
            {
                if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RodSize != sizeof(TCP_ESTATS_DATA_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_DATA_ROD_v0)Rod = *(PTCP_ESTATS_DATA_ROD_v0)rodBuffer;
            }
            if (RwSize)
            {
                if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RwSize != sizeof(TCP_ESTATS_DATA_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_DATA_RW_v0)Rw = *(PTCP_ESTATS_DATA_RW_v0)rwBuffer;
            }
            break;

        case TcpConnectionEstatsSndCong:
            if (RodSize)
            {
                if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RodSize != sizeof(TCP_ESTATS_SND_CONG_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_SND_CONG_ROD_v0)Rod = *(PTCP_ESTATS_SND_CONG_ROD_v0)rodBuffer;
            }
            if (RwSize)
            {
                if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RwSize != sizeof(NSI_ESTATS_SND_CONG_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PNSI_ESTATS_SND_CONG_RW_v0)Rw = *(PNSI_ESTATS_SND_CONG_RW_v0)rwBuffer;
            }
            if (RosSize)
            {
                if (RosVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RosSize != sizeof(TCP_ESTATS_SND_CONG_ROS_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_SND_CONG_ROS_v0)Ros = *(PTCP_ESTATS_SND_CONG_ROS_v0)rosBuffer;
            }
            break;

        case TcpConnectionEstatsPath:
            if (RodSize)
            {
                if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RodSize != sizeof(TCP_ESTATS_PATH_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_PATH_ROD_v0)Rod = *(PTCP_ESTATS_PATH_ROD_v0)rodBuffer;
            }
            if (RwSize)
            {
                if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RwSize != sizeof(TCP_ESTATS_PATH_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_PATH_RW_v0)Rw = *(PTCP_ESTATS_PATH_RW_v0)rwBuffer;
            }
            break;

        case TcpConnectionEstatsSendBuff:
            {
                if (RodSize)
                {
                    if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                    if (RodSize != sizeof(TCP_ESTATS_SEND_BUFF_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                    *(PTCP_ESTATS_SEND_BUFF_ROD_v0)Rod = *(PTCP_ESTATS_SEND_BUFF_ROD_v0)rodBuffer;
                }
                if (RwSize)
                {
                    if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                    if (RwSize != sizeof(TCP_ESTATS_SEND_BUFF_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                    *(PTCP_ESTATS_SEND_BUFF_RW_v0)Rw = *(PTCP_ESTATS_SEND_BUFF_RW_v0)rwBuffer;
                }
            }
            break;
        case TcpConnectionEstatsRec:
            {
                if (RodSize)
                {
                    if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                    if (RodSize != sizeof(TCP_ESTATS_REC_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                    *(PTCP_ESTATS_REC_ROD_v0)Rod = *(PTCP_ESTATS_REC_ROD_v0)rodBuffer;

                    NSI_ESTATS_REC_ROD_INTERNAL* src;
                    PTCP_ESTATS_REC_ROD_v0 dst;
                    if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                    if (RodSize != sizeof(TCP_ESTATS_REC_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                    src = (NSI_ESTATS_REC_ROD_INTERNAL*)rodBuffer;
                    dst = (PTCP_ESTATS_REC_ROD_v0)Rod;
                    dst->CurRwinSent = src->CurRwinSent;
                    dst->MaxRwinSent = src->MaxRwinSent;
                    dst->MinRwinSent = src->MinRwinSent;
                    dst->LimRwin = src->LimRwin;
                    dst->DupAckEpisodes = src->DupAckEpisodes;
                    dst->DupAcksOut = src->DupAcksOut;
                    dst->CeRcvd = src->CeRcvd;
                    dst->EcnSent = src->EcnSent;
                    dst->EcnNoncesRcvd = src->EcnNoncesRcvd;
                    dst->CurReasmQueue = src->CurReasmQueue;
                    dst->MaxReasmQueue = src->MaxReasmQueue;
                    dst->CurAppRQueue = src->CurAppRQueue;
                    dst->MaxAppRQueue = src->MaxAppRQueue;
                    dst->WinScaleSent = (UCHAR)src->WinScaleSent;
                }
                if (RwSize)
                {
                    if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                    if (RwSize != sizeof(TCP_ESTATS_REC_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                    *(PTCP_ESTATS_REC_RW_v0)Rw = *(PTCP_ESTATS_REC_RW_v0)rwBuffer;
                }
            }
            break;
        case TcpConnectionEstatsObsRec:
            {
                if (RodSize)
                {
                    if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                    if (RodSize != sizeof(TCP_ESTATS_OBS_REC_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                    *(PTCP_ESTATS_OBS_REC_ROD_v0)Rod = *(PTCP_ESTATS_OBS_REC_ROD_v0)rodBuffer;
                }
                if (RwSize)
                {
                    if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                    if (RwSize != sizeof(TCP_ESTATS_OBS_REC_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                    *(PTCP_ESTATS_OBS_REC_RW_v0)Rw = *(PTCP_ESTATS_OBS_REC_RW_v0)rwBuffer;
                }
            }
            break;
        case TcpConnectionEstatsBandwidth:
            {
                if (RodSize)
                {
                    if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                    if (RodSize != sizeof(TCP_ESTATS_BANDWIDTH_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                    *(PTCP_ESTATS_BANDWIDTH_ROD_v0)Rod = *(PTCP_ESTATS_BANDWIDTH_ROD_v0)rodBuffer;
                }
                if (RwSize)
                {
                    if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                    if (RwSize != sizeof(TCP_ESTATS_BANDWIDTH_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                    *(PTCP_ESTATS_BANDWIDTH_RW_v0)Rw = *(PTCP_ESTATS_BANDWIDTH_RW_v0)rwBuffer;
                }
            }
            break;
        case TcpConnectionEstatsFineRtt:
            {
                if (RodSize)
                {
                    if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                    if (RodSize != sizeof(TCP_ESTATS_FINE_RTT_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                    *(PTCP_ESTATS_FINE_RTT_ROD_v0)Rod = *(PTCP_ESTATS_FINE_RTT_ROD_v0)rodBuffer;
                }
                if (RwSize)
                {
                    if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                    if (RwSize != sizeof(TCP_ESTATS_FINE_RTT_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                    *(PTCP_ESTATS_FINE_RTT_RW_v0)Rw = *(PTCP_ESTATS_FINE_RTT_RW_v0)rwBuffer;
                }
            }
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        }

    Cleanup:
        PhFree(rwBuffer);
        PhFree(rodBuffer);
        PhFree(rosBuffer);
        return status;
    }
    return STATUS_INVALID_PARAMETER;
}

/**
 * Retrieves extended statistics for an IPv6 TCP connection.
 *
 * \param Row The TCP row that identifies the connection.
 * \param EstatsType The type of extended statistics.
 * \param Rw Receives the read-write information.
 * \param RwVersion The version of the read-write structure; must be zero.
 * \param RwSize The size, in bytes, of Rw.
 * \param Ros Receives the read-only static information.
 * \param RosVersion The version of the read-only static structure; must be zero.
 * \param RosSize The size, in bytes, of Ros.
 * \param Rod Receives the read-only dynamic information.
 * \param RodVersion The version of the read-only dynamic structure; must be zero.
 * \param RodSize The size, in bytes, of Rod.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getpertcp6connectionestats
 */
NTSTATUS PhGetPerTcp6ConnectionEStats(
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
    _In_ ULONG RodSize)
{
    PVOID rwBuffer = NULL;
    PVOID rodBuffer = NULL;
    PVOID rosBuffer = NULL;
    ULONG rwBufferSize = 0;
    ULONG rodBufferSize = 0;
    ULONG rosBufferSize = 0;
    ULONG objectId;
    NTSTATUS status;
    NSI_ESTATS_TCP_KEY key;

    if ((!RwSize || Rw) && (!RosSize || Ros) && (Rod || !RodSize))
    {
        // Determine object ID and buffer sizes based on EstatsType
        switch (EstatsType)
        {
        case TcpConnectionEstatsSynOpts:
            objectId = MibTcpEstatsTypeSynOpts;
            rwBufferSize = 0;
            rodBufferSize = 0;
            rosBufferSize = sizeof(TCP_ESTATS_SYN_OPTS_ROS_v0);
            break;
        case TcpConnectionEstatsData:
            objectId = MibTcpEstatsTypeData;
            rwBufferSize = sizeof(TCP_ESTATS_DATA_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_DATA_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsSndCong:
            objectId = MibTcpEstatsTypeSndCong;
            rwBufferSize = sizeof(NSI_ESTATS_SND_CONG_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_SND_CONG_ROD_v0);
            rosBufferSize = sizeof(TCP_ESTATS_SND_CONG_ROS_v0);
            break;
        case TcpConnectionEstatsPath:
            objectId = MibTcpEstatsTypePath;
            rwBufferSize = sizeof(TCP_ESTATS_PATH_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_PATH_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsSendBuff:
            objectId = MibTcpEstatsTypeSendBuff;
            rwBufferSize = sizeof(TCP_ESTATS_SEND_BUFF_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_SEND_BUFF_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsRec:
            objectId = MibTcpEstatsTypeRec;
            rwBufferSize = sizeof(TCP_ESTATS_REC_RW_v0);
            rodBufferSize = sizeof(NSI_ESTATS_REC_ROD_INTERNAL);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsObsRec:
            objectId = MibTcpEstatsTypeObsRec;
            rwBufferSize = sizeof(TCP_ESTATS_OBS_REC_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_OBS_REC_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsBandwidth:
            objectId = MibTcpEstatsTypeBandwidth;
            rwBufferSize = sizeof(TCP_ESTATS_BANDWIDTH_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_BANDWIDTH_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsFineRtt:
            objectId = MibTcpEstatsTypeFineRtt;
            rwBufferSize = sizeof(TCP_ESTATS_FINE_RTT_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_FINE_RTT_ROD_v0);
            rosBufferSize = 0;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }

        memset(&key, 0, sizeof(NSI_ESTATS_TCP_KEY));
        key.AddressFamily = AF_INET6;
        key.LocalPort = (USHORT)Row->dwLocalPort;
        key.LocalAddressIPv6 = Row->LocalAddr;
        key.LocalScopeId = Row->dwLocalScopeId;
        key.RemoteAddressFamily = AF_INET6;
        key.RemotePort = (USHORT)Row->dwRemotePort;
        key.RemoteAddressIPv6 = Row->RemoteAddr;
        key.RemoteScopeId = Row->dwRemoteScopeId;

        //key.LocalPort = (USHORT)_byteswap_ushort(Row->dwLocalPort);
        //key.RemotePort = (USHORT)_byteswap_ushort(Row->dwRemotePort);

        if (RwSize && rwBufferSize != 0)
        {
            rwBuffer = PhAllocate(rwBufferSize);
        }

        if (RodSize && rodBufferSize != 0)
        {
            rodBuffer = PhAllocate(rodBufferSize);
        }

        if (RosSize && rosBufferSize != 0)
        {
            rosBuffer = PhAllocate(rosBufferSize);
        }

        status = PhNsiGetAllParameters(
            NsiActive,
            &NPI_MS_TCP_MODULEID,
            objectId,
            &key,
            sizeof(NSI_ESTATS_TCP_KEY),
            rwBuffer,
            RwSize ? rwBufferSize : 0,
            rodBuffer,
            RodSize ? rodBufferSize : 0,
            rosBuffer,
            RosSize ? rosBufferSize : 0
            );

        if (!NT_SUCCESS(status))
            goto Cleanup;

        switch (EstatsType)
        {
        case TcpConnectionEstatsSynOpts:
            if (RosSize)
            {
                if (RosVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RosSize != sizeof(TCP_ESTATS_SYN_OPTS_ROS_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_SYN_OPTS_ROS_v0)Ros = *(PTCP_ESTATS_SYN_OPTS_ROS_v0)rosBuffer;
            }
            break;

        case TcpConnectionEstatsData:
            if (RodSize)
            {
                if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RodSize != sizeof(TCP_ESTATS_DATA_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_DATA_ROD_v0)Rod = *(PTCP_ESTATS_DATA_ROD_v0)rodBuffer;
            }
            if (RwSize)
            {
                if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RwSize != sizeof(TCP_ESTATS_DATA_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_DATA_RW_v0)Rw = *(PTCP_ESTATS_DATA_RW_v0)rwBuffer;
            }
            break;

        case TcpConnectionEstatsSndCong:
            if (RodSize)
            {
                if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RodSize != sizeof(TCP_ESTATS_SND_CONG_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_SND_CONG_ROD_v0)Rod = *(PTCP_ESTATS_SND_CONG_ROD_v0)rodBuffer;
            }
            if (RwSize)
            {
                if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RwSize != sizeof(NSI_ESTATS_SND_CONG_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PNSI_ESTATS_SND_CONG_RW_v0)Rw = *(PNSI_ESTATS_SND_CONG_RW_v0)rwBuffer;
            }
            if (RosSize)
            {
                if (RosVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RosSize != sizeof(TCP_ESTATS_SND_CONG_ROS_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_SND_CONG_ROS_v0)Ros = *(PTCP_ESTATS_SND_CONG_ROS_v0)rosBuffer;
            }
            break;

        case TcpConnectionEstatsPath:
            if (RodSize)
            {
                if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RodSize != sizeof(TCP_ESTATS_PATH_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_PATH_ROD_v0)Rod = *(PTCP_ESTATS_PATH_ROD_v0)rodBuffer;
            }
            if (RwSize)
            {
                if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RwSize != sizeof(TCP_ESTATS_PATH_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_PATH_RW_v0)Rw = *(PTCP_ESTATS_PATH_RW_v0)rwBuffer;
            }
            break;

        case TcpConnectionEstatsSendBuff:
            if (RodSize)
            {
                TCP_ESTATS_SEND_BUFF_ROD_v0* src;
                PTCP_ESTATS_SEND_BUFF_ROD_v0 dst;
                if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RodSize != sizeof(TCP_ESTATS_SEND_BUFF_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                src = (TCP_ESTATS_SEND_BUFF_ROD_v0*)rodBuffer;
                dst = (PTCP_ESTATS_SEND_BUFF_ROD_v0)Rod;
                dst->CurRetxQueue = src->CurRetxQueue;
                dst->MaxRetxQueue = src->MaxRetxQueue;
                dst->CurAppWQueue = src->CurAppWQueue;
                dst->MaxAppWQueue = src->MaxAppWQueue;
            }
            if (RwSize)
            {
                if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RwSize != sizeof(TCP_ESTATS_SEND_BUFF_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_SEND_BUFF_RW_v0)Rw = *(PTCP_ESTATS_SEND_BUFF_RW_v0)rwBuffer;
            }
            break;

        case TcpConnectionEstatsRec:
            if (RodSize)
            {
                NSI_ESTATS_REC_ROD_INTERNAL* src;
                PTCP_ESTATS_REC_ROD_v0 dst;
                if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RodSize != sizeof(TCP_ESTATS_REC_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                src = (NSI_ESTATS_REC_ROD_INTERNAL*)rodBuffer;
                dst = (PTCP_ESTATS_REC_ROD_v0)Rod;
                dst->CurRwinSent = src->CurRwinSent;
                dst->MaxRwinSent = src->MaxRwinSent;
                dst->MinRwinSent = src->MinRwinSent;
                dst->LimRwin = src->LimRwin;
                dst->DupAckEpisodes = src->DupAckEpisodes;
                dst->DupAcksOut = src->DupAcksOut;
                dst->CeRcvd = src->CeRcvd;
                dst->EcnSent = src->EcnSent;
                dst->EcnNoncesRcvd = src->EcnNoncesRcvd;
                dst->CurReasmQueue = src->CurReasmQueue;
                dst->MaxReasmQueue = src->MaxReasmQueue;
                dst->CurAppRQueue = src->CurAppRQueue;
                dst->MaxAppRQueue = src->MaxAppRQueue;
                dst->WinScaleSent = (UCHAR)src->WinScaleSent;
            }
            if (RwSize)
            {
                if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RwSize != sizeof(TCP_ESTATS_REC_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_REC_RW_v0)Rw = *(PTCP_ESTATS_REC_RW_v0)rwBuffer;
            }
            break;

        case TcpConnectionEstatsObsRec:
            if (RodSize)
            {
                if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RodSize != sizeof(TCP_ESTATS_OBS_REC_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_OBS_REC_ROD_v0)Rod = *(PTCP_ESTATS_OBS_REC_ROD_v0)rodBuffer;
            }
            if (RwSize)
            {
                if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RwSize != sizeof(TCP_ESTATS_OBS_REC_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_OBS_REC_RW_v0)Rw = *(PTCP_ESTATS_OBS_REC_RW_v0)rwBuffer;
            }
            break;

        case TcpConnectionEstatsBandwidth:
            if (RodSize)
            {
                if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RodSize != sizeof(TCP_ESTATS_BANDWIDTH_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_BANDWIDTH_ROD_v0)Rod = *(PTCP_ESTATS_BANDWIDTH_ROD_v0)rodBuffer;
            }
            if (RwSize)
            {
                if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RwSize != sizeof(TCP_ESTATS_BANDWIDTH_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_BANDWIDTH_RW_v0)Rw = *(PTCP_ESTATS_BANDWIDTH_RW_v0)rwBuffer;
            }
            break;

        case TcpConnectionEstatsFineRtt:
            if (RodSize)
            {
                if (RodVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RodSize != sizeof(TCP_ESTATS_FINE_RTT_ROD_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_FINE_RTT_ROD_v0)Rod = *(PTCP_ESTATS_FINE_RTT_ROD_v0)rodBuffer;
            }
            if (RwSize)
            {
                if (RwVersion) { status = STATUS_NOT_SUPPORTED; break; }
                if (RwSize != sizeof(TCP_ESTATS_FINE_RTT_RW_v0)) { status = STATUS_INVALID_USER_BUFFER; break; }
                *(PTCP_ESTATS_FINE_RTT_RW_v0)Rw = *(PTCP_ESTATS_FINE_RTT_RW_v0)rwBuffer;
            }
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        }

    Cleanup:
        PhFree(rwBuffer);
        PhFree(rodBuffer);
        PhFree(rosBuffer);
        return status;
    }
    return STATUS_INVALID_PARAMETER;
}

/* Source: A:\tcpip\utils.c */

/**
 * Formats a FILETIME timestamp as a local date and time string.
 *
 * \param timestamp The timestamp, in 100-nanosecond intervals since 1601. Zero formats as "N/A".
 * \param buffer Receives the null-terminated string.
 * \param bufferSize The size of buffer, in characters.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-filetimetosystemtime
 */
VOID PhFormatTimestamp(
    _In_ LARGE_INTEGER timestamp,
    _Out_writes_(bufferSize) PWSTR buffer,
    _In_ SIZE_T bufferSize
    )
{
    LARGE_INTEGER convertedTime;
    SYSTEMTIME systemTime;

    if (timestamp.QuadPart == 0)
    {
        PhCopyStringZ(L"<invalid time>", RTL_NUMBER_OF(L"<invalid time>") - 1, buffer, bufferSize, NULL);
        return;
    }

    if (NT_SUCCESS(PhSystemTimeToLocalTime(&timestamp, &convertedTime)))
    {
        PhLargeIntegerToSystemTime(&systemTime, &convertedTime);
        PhFormatDateTimeToBuffer(&systemTime, buffer, bufferSize, NULL);
    }
    else
    {
        PhCopyStringZ(L"<conversion error>", RTL_NUMBER_OF(L"<conversion error>") - 1, buffer, bufferSize, NULL);
    }
}

/**
 * Closes an ICMP handle and deregisters its reply notification.
 *
 * \param IcmpHandle The ICMP handle returned by PhIcmpCreateFile or PhIcmp6CreateFile.
 * \return TRUE on success, otherwise FALSE (the last error is set).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpclosehandle
 */
BOOL PhIcmpCloseHandle(
    _In_ HANDLE IcmpHandle
    )
{
    ICMP_HANDLE_INTERNAL* handle;
    HANDLE notificationHandle;
    BOOLEAN deregister;

    if (PhIsInvalidIcmpHandle(IcmpHandle))
    {
        PhSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    handle = (ICMP_HANDLE_INTERNAL*)(IcmpHandle);
    for (;;)
    {
        RtlEnterCriticalSection(&handle->Lock);
        if (handle->ActiveRequestCount == 0)
        {
            RtlLeaveCriticalSection(&handle->Lock);
            break;
        }

        RtlLeaveCriticalSection(&handle->Lock);
        PhDelayExecution(10);
    }

    PhAcquireQueuedLockExclusive(&PingNotificationLock);
    if (handle->IsIpv4)
    {
        notificationHandle = PingNotificationHandleV4;
        deregister = (--PingNotificationCountV4 == 0);
    }
    else
    {
        notificationHandle = PingNotificationHandleV6;
        deregister = (--PingNotificationCountV6 == 0);
    }

    if (deregister)
    {
        PhNsiRpcDeregisterChangeNotification(
            NotificationRpcHandle,
            handle->ModuleId,
            NsiIcmpObjectEchoRequest,
            notificationHandle
            );
    }

    PhReleaseQueuedLockExclusive(&PingNotificationLock);
    RtlDeleteCriticalSection(&handle->Lock);
    PhFree(handle);
    return TRUE;
}

/**
 * Retrieves the IPv4 and IPv6 TCP connection tables from the TCP NSI provider.
 *
 * \param Tcp_Ipv4Table Receives the allocated IPv4 table.
 * \param Tcp_Ipv6Table Receives the allocated IPv6 table.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcptable
 */
NTSTATUS PhGetTcpTable(
    _Out_ NSI_TCP_IPV4_TABLE** Tcp_Ipv4Table,
    _Out_ NSI_TCP_IPV6_TABLE** Tcp_Ipv6Table
    )
{
    NTSTATUS status;
    ULONG firstCount = 0;
    ULONG secondCount = 0;
    ULONG ipv4Count = 0;
    ULONG ipv6Count = 0;
    SIZE_T totalRows;
    SIZE_T rowBytes;
    SIZE_T ipv4TableSize;
    SIZE_T ipv6TableSize;
    NSI_TCP_KEY_ROW* firstKeyTable = NULL;
    NSI_TCP_KEY_ROW* secondKeyTable = NULL;
    NSI_TCP_STATUS_ROW* firstStatusTable = NULL;
    NSI_TCP_STATUS_ROW* secondStatusTable = NULL;
    NSI_TCP_OWNER_ROW* firstOwnerTable = NULL;
    NSI_TCP_OWNER_ROW* secondOwnerTable = NULL;
    NSI_TCP_IPV4_TABLE* ipv4TcpTable = NULL;
    NSI_TCP_IPV6_TABLE* ipv6TcpTable = NULL;

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTableIdTcp,
        &firstKeyTable,
        sizeof(NSI_TCP_KEY_ROW),
        NULL,
        0,
        &firstStatusTable,
        sizeof(NSI_TCP_STATUS_ROW),
        &firstOwnerTable,
        sizeof(NSI_TCP_OWNER_ROW),
        &firstCount,
        0
        );

    if (status)
        return status;

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTableIdTcp6,
        &secondKeyTable,
        sizeof(NSI_TCP_KEY_ROW),
        NULL,
        0,
        &secondStatusTable,
        sizeof(NSI_TCP_STATUS_ROW),
        &secondOwnerTable,
        sizeof(NSI_TCP_OWNER_ROW),
        &secondCount,
        0
        );

    if (status)
    {
        PhNsiFreeTable(firstKeyTable, NULL, firstStatusTable, firstOwnerTable);
        return status;
    }

    if (!NT_SUCCESS(RtlSizeTAdd(firstCount, secondCount, &totalRows)) ||
        !NT_SUCCESS(RtlSizeTMult(totalRows, sizeof(NSI_TCP_IPV4_ROW), &rowBytes)) ||
        !NT_SUCCESS(RtlSizeTAdd(FIELD_OFFSET(NSI_TCP_IPV4_TABLE, Entries), rowBytes, &ipv4TableSize)) ||
        !NT_SUCCESS(RtlSizeTMult(totalRows, sizeof(NSI_TCP_IPV6_ROW), &rowBytes)) ||
        !NT_SUCCESS(RtlSizeTAdd(FIELD_OFFSET(NSI_TCP_IPV6_TABLE, Entries), rowBytes, &ipv6TableSize)))
    {
        PhNsiFreeTable(firstKeyTable, NULL, firstStatusTable, firstOwnerTable);
        PhNsiFreeTable(secondKeyTable, NULL, secondStatusTable, secondOwnerTable);
        return STATUS_INTEGER_OVERFLOW;
    }

    ipv4TcpTable = PhAllocateZero(ipv4TableSize);
    ipv6TcpTable = PhAllocateZero(ipv6TableSize);

    for (ULONG i = 0; i < firstCount; ++i)
    {
        const NSI_TCP_KEY_ROW* const keyRow = &firstKeyTable[i];

        //assert(keyRow->LocalAddress.si_family == keyRow->RemoteAddress.si_family);

        switch (keyRow->LocalAddress.si_family)
        {
        case AF_INET:
            {
                const NSI_TCP_STATUS_ROW* const statusRow = &firstStatusTable[i];
                const NSI_TCP_OWNER_ROW* const ownerRow = &firstOwnerTable[i];
                NSI_TCP_IPV4_ROW* const ipv4Row = &ipv4TcpTable->Entries[ipv4Count++];

                ZeroMemory(ipv4Row, sizeof(NSI_TCP_IPV4_ROW));
                ipv4Row->State = statusRow->State;
                ipv4Row->LocalAddress = keyRow->LocalAddress.Ipv4.sin_addr;
                ipv4Row->LocalPort = keyRow->LocalAddress.Ipv4.sin_port;
                ipv4Row->RemoteAddress = keyRow->RemoteAddress.Ipv4.sin_addr;
                ipv4Row->RemotePort = keyRow->RemoteAddress.Ipv4.sin_port;
                ipv4Row->AppliedSetting = statusRow->AppliedSetting;
                ipv4Row->OwningPid = ownerRow->OwningPid;
                ipv4Row->CreateTimestamp = ownerRow->CreateTimestamp;
                ipv4Row->OffloadState = statusRow->OffloadState;
            }
            break;
        case AF_INET6:
            {
                const NSI_TCP_STATUS_ROW* const statusRow = &firstStatusTable[i];
                const NSI_TCP_OWNER_ROW* const ownerRow = &firstOwnerTable[i];
                NSI_TCP_IPV6_ROW* const ipv6Row = &ipv6TcpTable->Entries[ipv6Count++];

                ZeroMemory(ipv6Row, sizeof(NSI_TCP_IPV6_ROW));
                ipv6Row->LocalAddress = keyRow->LocalAddress.Ipv6.sin6_addr;
                ipv6Row->LocalScopeId.Value = keyRow->LocalAddress.Ipv6.sin6_scope_id;
                ipv6Row->LocalPort = keyRow->LocalAddress.Ipv6.sin6_port;
                ipv6Row->RemoteAddress = keyRow->RemoteAddress.Ipv6.sin6_addr;
                ipv6Row->RemoteScopeId.Value = keyRow->RemoteAddress.Ipv6.sin6_scope_id;
                ipv6Row->RemotePort = keyRow->RemoteAddress.Ipv6.sin6_port;
                ipv6Row->State = statusRow->State;
                ipv6Row->AppliedSetting = statusRow->AppliedSetting;
                ipv6Row->OwningPid = ownerRow->OwningPid;
                ipv6Row->CreateTimestamp = ownerRow->CreateTimestamp;
                ipv6Row->OffloadState = statusRow->OffloadState;
            }
            break;
        }
    }

    for (ULONG i = 0; i < secondCount; ++i)
    {
        const NSI_TCP_KEY_ROW* const keyRow = &secondKeyTable[i];

       // assert(keyRow->LocalAddress.si_family == keyRow->RemoteAddress.si_family);

        switch (keyRow->LocalAddress.si_family)
        {
        case AF_INET:
            {
                const NSI_TCP_STATUS_ROW* const statusRow = &secondStatusTable[i];
                const NSI_TCP_OWNER_ROW* const ownerRow = &secondOwnerTable[i];
                NSI_TCP_IPV4_ROW* const ipv4Row = &ipv4TcpTable->Entries[ipv4Count++];

                ZeroMemory(ipv4Row, sizeof(NSI_TCP_IPV4_ROW));
                ipv4Row->State = statusRow->State;
                ipv4Row->LocalAddress = keyRow->LocalAddress.Ipv4.sin_addr;
                ipv4Row->LocalPort = keyRow->LocalAddress.Ipv4.sin_port;
                ipv4Row->RemoteAddress = keyRow->RemoteAddress.Ipv4.sin_addr;
                ipv4Row->RemotePort = keyRow->RemoteAddress.Ipv4.sin_port;
                ipv4Row->AppliedSetting = statusRow->AppliedSetting;
                ipv4Row->OwningPid = ownerRow->OwningPid;
                ipv4Row->CreateTimestamp = ownerRow->CreateTimestamp;
                ipv4Row->OffloadState = statusRow->OffloadState;
            }
            break;
        case AF_INET6:
            {
                const NSI_TCP_STATUS_ROW* const statusRow = &secondStatusTable[i];
                const NSI_TCP_OWNER_ROW* const ownerRow = &secondOwnerTable[i];
                NSI_TCP_IPV6_ROW* const ipv6Row = &ipv6TcpTable->Entries[ipv6Count++];

                ZeroMemory(ipv6Row, sizeof(NSI_TCP_IPV6_ROW));
                ipv6Row->LocalAddress = keyRow->LocalAddress.Ipv6.sin6_addr;
                ipv6Row->LocalScopeId.Value = keyRow->LocalAddress.Ipv6.sin6_scope_id;
                ipv6Row->LocalPort = keyRow->LocalAddress.Ipv6.sin6_port;
                ipv6Row->RemoteAddress = keyRow->RemoteAddress.Ipv6.sin6_addr;
                ipv6Row->RemoteScopeId.Value = keyRow->RemoteAddress.Ipv6.sin6_scope_id;
                ipv6Row->RemotePort = keyRow->RemoteAddress.Ipv6.sin6_port;
                ipv6Row->State = statusRow->State;
                ipv6Row->AppliedSetting = statusRow->AppliedSetting;
                ipv6Row->OwningPid = ownerRow->OwningPid;
                ipv6Row->CreateTimestamp = ownerRow->CreateTimestamp;
                ipv6Row->OffloadState = statusRow->OffloadState;
            }
            break;
        }
    }

    ipv4TcpTable->Count = ipv4Count;
    ipv6TcpTable->Count = ipv6Count;

    *Tcp_Ipv4Table = ipv4TcpTable;
    *Tcp_Ipv6Table = ipv6TcpTable;

    PhNsiFreeTable(firstKeyTable, NULL, firstStatusTable, firstOwnerTable);
    PhNsiFreeTable(secondKeyTable, NULL, secondStatusTable, secondOwnerTable);
    return status;
}

/**
 * Enables or disables extended statistics collection for an IPv6 TCP connection.
 *
 * \param Row The TCP row that identifies the connection.
 * \param EstatsType The type of extended statistics.
 * \param Rw The read-write information to set.
 * \param RwVersion The version of the read-write structure; must be zero.
 * \param RwSize The size, in bytes, of Rw.
 * \param Offset The offset into the read-write structure; must be zero.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-setpertcp6connectionestats
 */
NTSTATUS PhSetPerTcp6ConnectionEStats(
    _In_ PMIB_TCP6ROW Row,
    _In_ TCP_ESTATS_TYPE EstatsType,
    _In_reads_bytes_opt_(RwSize) PUCHAR Rw,
    _In_ ULONG RwVersion,
    _In_ ULONG RwSize,
    _In_ ULONG Offset)
{
    NSI_ESTATS_TCP_KEY key;
    ULONG objectId;

    if (RwVersion || Offset)
        return STATUS_NOT_SUPPORTED;
    if (!RwSize)
        return STATUS_SUCCESS;
    if (!Rw)
        return STATUS_INVALID_PARAMETER;

    switch (EstatsType)
    {
    case TcpConnectionEstatsSynOpts:   objectId = MibTcpEstatsTypeSynOpts;   break;
    case TcpConnectionEstatsData:      objectId = MibTcpEstatsTypeData;      break;
    case TcpConnectionEstatsSndCong:   objectId = MibTcpEstatsTypeSndCong;   break;
    case TcpConnectionEstatsPath:      objectId = MibTcpEstatsTypePath;      break;
    case TcpConnectionEstatsSendBuff:  objectId = MibTcpEstatsTypeSendBuff;  break;
    case TcpConnectionEstatsRec:       objectId = MibTcpEstatsTypeRec;       break;
    case TcpConnectionEstatsObsRec:    objectId = MibTcpEstatsTypeObsRec;    break;
    case TcpConnectionEstatsBandwidth: objectId = MibTcpEstatsTypeBandwidth; break;
    case TcpConnectionEstatsFineRtt:   objectId = MibTcpEstatsTypeFineRtt;   break;
    default: return STATUS_INVALID_PARAMETER;
    }

    memset(&key, 0, sizeof(NSI_ESTATS_TCP_KEY));
    key.AddressFamily = AF_INET6;
    key.LocalPort = (USHORT)Row->dwLocalPort;
    key.LocalAddressIPv6 = Row->LocalAddr;
    key.LocalScopeId = Row->dwLocalScopeId;
    key.RemoteAddressFamily = AF_INET6;
    key.RemotePort = (USHORT)Row->dwRemotePort;
    key.RemoteAddressIPv6 = Row->RemoteAddr;
    key.RemoteScopeId = Row->dwRemoteScopeId;

    return PhNsiSetParameter(
        NsiActive,
        NsiSetCreateOrSet,
        &NPI_MS_TCP_MODULEID,
        objectId,
        &key,
        sizeof(NSI_ESTATS_TCP_KEY),
        NsiParameterTypeReadWrite,
        Rw,
        RwSize,
        0
        );
}

/**
 * Enables or disables extended statistics collection for an IPv4 TCP connection.
 *
 * \param Row The TCP row that identifies the connection.
 * \param EstatsType The type of extended statistics.
 * \param Rw The read-write information to set.
 * \param RwVersion The version of the read-write structure; must be zero.
 * \param RwSize The size, in bytes, of Rw.
 * \param Offset The offset into the read-write structure; must be zero.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-setpertcpconnectionestats
 */
_Success_(NT_SUCCESS(return))
NTSTATUS PhSetPerTcpConnectionEStats(
    _In_ PMIB_TCPROW Row,
    _In_ TCP_ESTATS_TYPE EstatsType,
    _In_reads_bytes_opt_(RwSize) PUCHAR Rw,
    _In_ ULONG RwVersion,
    _In_ ULONG RwSize,
    _In_ ULONG Offset)
{
    NSI_ESTATS_TCP_KEY key;
    ULONG objectId;

    if (RwVersion || Offset)
        return STATUS_NOT_SUPPORTED;
    if (!RwSize)
        return STATUS_SUCCESS;
    if (!Rw)
        return STATUS_INVALID_PARAMETER;

    switch (EstatsType)
    {
    case TcpConnectionEstatsSynOpts:   
        objectId = MibTcpEstatsTypeSynOpts;   
        break;
    case TcpConnectionEstatsData:      
        objectId = MibTcpEstatsTypeData;      
        break;
    case TcpConnectionEstatsSndCong:   
        objectId = MibTcpEstatsTypeSndCong;   
        break;
    case TcpConnectionEstatsPath:      
        objectId = MibTcpEstatsTypePath;      
        break;
    case TcpConnectionEstatsSendBuff:  
        objectId = MibTcpEstatsTypeSendBuff;  
        break;
    case TcpConnectionEstatsRec:       
        objectId = MibTcpEstatsTypeRec;       
        break;
    case TcpConnectionEstatsObsRec:    
        objectId = MibTcpEstatsTypeObsRec;    
        break;
    case TcpConnectionEstatsBandwidth: 
        objectId = MibTcpEstatsTypeBandwidth; 
        break;
    case TcpConnectionEstatsFineRtt:   
        objectId = MibTcpEstatsTypeFineRtt;   
        break;
    default: 
        return STATUS_INVALID_PARAMETER;
    }

    memset(&key, 0, sizeof(NSI_ESTATS_TCP_KEY));
    key.AddressFamily = AF_INET;
    key.LocalPort = (USHORT)Row->dwLocalPort;
    key.LocalAddressIPv4.s_addr = Row->dwLocalAddr;
    key.RemoteAddressFamily = AF_INET;
    key.RemoteAddressIPv4.s_addr = Row->dwRemoteAddr;
    key.RemotePort = (USHORT)Row->dwRemotePort;

    return PhNsiSetParameter(
        NsiActive,
        NsiSetCreateOrSet,
        &NPI_MS_TCP_MODULEID,
        objectId,
        &key,
        sizeof(NSI_ESTATS_TCP_KEY),
        NsiParameterTypeReadWrite,
        Rw,
        RwSize,
        0
        );
}

/**
 * Returns the display name of a neighbor (ARP/ND) state.
 *
 * \param state The NL_NEIGHBOR_STATE value.
 * \return A static string describing the state.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/nldef/ne-nldef-nl_neighbor_state
 */
PCWSTR PhNeighborStateToString(
    _In_ NL_NEIGHBOR_STATE state
    )
{
    switch (state)
    {
    case NlnsUnreachable:
        return L"Unreachable";
    case NlnsIncomplete:
        return L"Incomplete";
    case NlnsProbe:
        return L"Probe";
    case NlnsDelay:
        return L"Delay";
    case NlnsStale:
        return L"Stale";
    case NlnsReachable:
        return L"Reachable";
    case NlnsPermanent:
        return L"Permanent";
    default:
        return L"Unknown";
    }
}

 VOID PhSystemTimeToTm(
    _In_  const SYSTEMTIME* st,
    _Out_ struct tm* tm
    )
{
    tm->tm_year = st->wYear - 1900;   // tm_year = years since 1900
    tm->tm_mon = st->wMonth - 1;     // tm_mon  = [0, 11]
    tm->tm_mday = st->wDay;
    tm->tm_hour = st->wHour;
    tm->tm_min = st->wMinute;
    tm->tm_sec = st->wSecond;
    tm->tm_wday = st->wDayOfWeek;     // Sunday = 0 (matches tm)
    tm->tm_yday = -1;                 // Let CRT recompute if needed
    tm->tm_isdst = -1;                // Unknown DST
}

/**
 * Compares two GUIDs.
 *
 * \param Left The first GUID.
 * \param Right The second GUID.
 * \return TRUE if the GUIDs are equal, otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/guiddef/nf-guiddef-isequalguid
 */
BOOLEAN PhTcpipGuidEquals(
    _In_ const GUID* Left,
    _In_ const GUID* Right
    )
{
    return Left->Data1 == Right->Data1
        && Left->Data2 == Right->Data2
        && Left->Data3 == Right->Data3
        && memcmp(Left->Data4, Right->Data4, sizeof(Left->Data4)) == 0;
}

/**
 * Formats a neighbor physical address as hyphen-separated hex bytes.
 *
 * \param Address The physical address.
 * \param AddressLength The size, in bytes, of Address.
 * \param Buffer Receives the null-terminated string.
 * \param BufferCount The size of Buffer, in characters.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/ns-netioapi-mib_ipnet_row2
 */
VOID PhFormatNeighborMacAddress(
    _In_reads_bytes_(AddressLength) const UCHAR* Address,
    _In_ ULONG AddressLength,
    _Out_writes_(BufferCount) PWSTR Buffer,
    _In_ SIZE_T BufferCount
    )
{
    static const WCHAR hexDigits[] = L"0123456789ABCDEF";
    SIZE_T offset = 0;
    ULONG i;

    if (!Buffer || !BufferCount)
        return;

    Buffer[0] = UNICODE_NULL;

    if (!AddressLength)
    {
        RtlStringCchCopyW(Buffer, BufferCount, L"(none)");
        return;
    }

    if (!Address)
        return;

    for (i = 0; i < AddressLength; i++)
    {
        if (offset + (i ? 3 : 2) >= BufferCount)
            break;

        if (i)
            Buffer[offset++] = L'-';
        Buffer[offset++] = hexDigits[Address[i] >> 4];
        Buffer[offset++] = hexDigits[Address[i] & 0xf];
    }

    Buffer[offset] = UNICODE_NULL;
}

/**
 * Builds a MIB_IF_ROW2 from the NDIS NSI interface parameter blocks.
 *
 * \param Row Receives the row.
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param Rw The read-write parameter block.
 * \param Rod The read-only dynamic parameter block.
 * \param Ros The read-only static parameter block.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/ns-netioapi-mib_if_row2
 */
static VOID PhpFillIfRow2(
    _Out_ PMIB_IF_ROW2 Row,
    _In_ const NET_LUID *InterfaceLuid,
    _In_ const NDIS_NSI_INTERFACE_INFORMATION_RW *Rw,
    _In_ const NDIS_NSI_INTERFACE_INFORMATION_ROD *Rod,
    _In_ const NDIS_NSI_INTERFACE_INFORMATION_ROS *Ros
    )
{
    SIZE_T length;

    memset(Row, 0, sizeof(MIB_IF_ROW2));
    Row->InterfaceLuid = *InterfaceLuid;
    Row->InterfaceIndex = Ros->ifIndex;
    Row->InterfaceGuid = Ros->InterfaceGuid;
    length = min(Rw->ifAlias.Length, sizeof(Row->Alias) - sizeof(WCHAR));
    memcpy(Row->Alias, Rw->ifAlias.String, length);
    length = min(Ros->ifDescr.Length, sizeof(Row->Description) - sizeof(WCHAR));
    memcpy(Row->Description, Ros->ifDescr.String, length);
    Row->PhysicalAddressLength = min(Rw->ifPhysAddress.Length, sizeof(Row->PhysicalAddress));
    memcpy(Row->PhysicalAddress, Rw->ifPhysAddress.Address, Row->PhysicalAddressLength);
    length = min(Ros->PermanentPhysAddress.Length, sizeof(Row->PermanentPhysicalAddress));
    memcpy(Row->PermanentPhysicalAddress, Ros->PermanentPhysAddress.Address, length);
    Row->Mtu = Rod->ifMtu;
    Row->Type = Ros->ifType;
    Row->MediaType = Ros->MediaType;
    Row->PhysicalMediumType = Ros->PhysicalMediumType;
    Row->AccessType = Ros->AccessType;
    Row->DirectionType = Ros->DirectionType;
    Row->InterfaceAndOperStatusFlags.HardwareInterface = !!(Ros->ifFlags & NetIfInterfaceFlagHardwareInterface);
    Row->InterfaceAndOperStatusFlags.FilterInterface = !!(Ros->ifFlags & NetIfInterfaceFlagFilterInterface);
    Row->InterfaceAndOperStatusFlags.ConnectorPresent = Ros->ifConnectorPresent;
    Row->InterfaceAndOperStatusFlags.NotAuthenticated = !!(Rod->ifOperStatusFlags & NET_IF_OPER_STATUS_DOWN_NOT_AUTHENTICATED);
    Row->InterfaceAndOperStatusFlags.NotMediaConnected = !!(Rod->ifOperStatusFlags & NET_IF_OPER_STATUS_DOWN_NOT_MEDIA_CONNECTED);
    Row->InterfaceAndOperStatusFlags.Paused = !!(Rod->ifOperStatusFlags & NET_IF_OPER_STATUS_DORMANT_PAUSED);
    Row->InterfaceAndOperStatusFlags.LowPower = !!(Rod->ifOperStatusFlags & NET_IF_OPER_STATUS_DORMANT_LOW_POWER);
    Row->InterfaceAndOperStatusFlags.EndPointInterface = !!(Ros->ifFlags & NetIfInterfaceFlagEndPointInterface);
    Row->OperStatus = (IF_OPER_STATUS)Rod->ifOperStatus;
    Row->AdminStatus = Rw->ifAdminStatus;
    Row->MediaConnectState = Rod->MediaConnectState;
    Row->NetworkGuid = Rw->NetworkGuid;
    Row->ConnectionType = Ros->ConnectionType;
    Row->TransmitLinkSpeed = Rod->XmitLinkSpeed;
    Row->ReceiveLinkSpeed = Rod->RcvLinkSpeed;
    Row->InOctets = Rod->ifHCInOctets;
    Row->InUcastPkts = Rod->ifHCInUcastPkts;
    Row->InNUcastPkts = Rod->ifHCInMulticastPkts + Rod->ifHCInBroadcastPkts;
    Row->InDiscards = Rod->ifInDiscards;
    Row->InErrors = Rod->ifInErrors;
    Row->InUnknownProtos = Rod->ifInUnknownProtos;
    Row->InUcastOctets = Rod->ifHCInUcastOctets;
    Row->InMulticastOctets = Rod->ifHCInMulticastOctets;
    Row->InBroadcastOctets = Rod->ifHCInBroadcastOctets;
    Row->OutOctets = Rod->ifHCOutOctets;
    Row->OutUcastPkts = Rod->ifHCOutUcastPkts;
    Row->OutNUcastPkts = Rod->ifHCOutMulticastPkts + Rod->ifHCOutBroadcastPkts;
    Row->OutDiscards = Rod->ifOutDiscards;
    Row->OutErrors = Rod->ifOutErrors;
    Row->OutUcastOctets = Rod->ifHCOutUcastOctets;
    Row->OutMulticastOctets = Rod->ifHCOutMulticastOctets;
    Row->OutBroadcastOctets = Rod->ifHCOutBroadcastOctets;
}

/**
 * Retrieves information for a network interface.
 *
 * \param Row On input, InterfaceLuid or InterfaceIndex identifies the interface; on output, the interface row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getifentry2
 */
NTSTATUS PhGetIfEntry2(
    _Inout_ PMIB_IF_ROW2 Row
    )
{
    NTSTATUS result;
    NET_LUID interfaceLuid;
    NDIS_NSI_INTERFACE_INFORMATION_RW rw;
    NDIS_NSI_INTERFACE_INFORMATION_ROD rod;
    NDIS_NSI_INTERFACE_INFORMATION_ROS ros;

    if (!Row)
        return STATUS_INVALID_PARAMETER;

    interfaceLuid = Row->InterfaceLuid;
    if (!interfaceLuid.Value)
    {
        if (!Row->InterfaceIndex)
            return STATUS_INVALID_PARAMETER;

        result = PhConvertInterfaceIndexToLuid(Row->InterfaceIndex, &interfaceLuid);

        if (result)
            return result;
    }

    result = PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &interfaceLuid,
        sizeof(interfaceLuid),
        &rw,
        sizeof(rw),
        &rod,
        sizeof(rod),
        &ros,
        sizeof(ros));

    if (result)
        return result;

    if (rod.CompartmentId != PhGetCurrentThreadCompartmentId())
        return STATUS_NOT_FOUND;

    PhpFillIfRow2(Row, &interfaceLuid, &rw, &rod, &ros);
    return STATUS_SUCCESS;
}

/**
 * Retrieves an anycast IP address entry.
 *
 * \param Row On input, the key fields of the row; on output, the complete row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getanycastipaddressentry
 */
NTSTATUS PhGetAnycastIpAddressEntry(
    _Inout_ PMIB_ANYCASTIPADDRESS_ROW Row
    )
{
    return PhNetioGetAddress(NsiActive, PhpNetioAddressAnycast, Row);
}

/**
 * Retrieves the anycast IP address table.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the allocated table. Free it with PhFreeMibTable/FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getanycastipaddresstable
 */
NTSTATUS PhGetAnycastIpAddressTable(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_ANYCASTIPADDRESS_TABLE* Table
    )
{
    return PhNetioEnumerateAddress(NsiActive, Family, PhpNetioAddressAnycast, Table);
}

/**
 * Retrieves a unicast IP address entry.
 *
 * \param Row On input, the key fields of the row; on output, the complete row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getunicastipaddressentry
 */
NTSTATUS PhGetUnicastIpAddressEntry(
    _Inout_ PMIB_UNICASTIPADDRESS_ROW Row
    )
{
    return PhNetioGetAddress(NsiActive, PhpNetioAddressUnicast, Row);
}

/**
 * Retrieves the unicast IP address table.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the allocated table. Free it with PhFreeMibTable/FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getunicastipaddresstable
 */
NTSTATUS PhGetUnicastIpAddressTable(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_UNICASTIPADDRESS_TABLE* Table
    )
{
    return PhNetioEnumerateAddress(NsiActive, Family, PhpNetioAddressUnicast, Table);
}

/**
 * Retrieves a multicast IP address entry.
 *
 * \param Row On input, the key fields of the row; on output, the complete row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getmulticastipaddressentry
 */
NTSTATUS PhGetMulticastIpAddressEntry(
    _Inout_ PMIB_MULTICASTIPADDRESS_ROW Row
    )
{
    return PhNetioGetAddress(NsiActive, PhpNetioAddressMulticast, Row);
}

/**
 * Retrieves the multicast IP address table.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the allocated table. Free it with PhFreeMibTable/FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getmulticastipaddresstable
 */
NTSTATUS PhGetMulticastIpAddressTable(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_MULTICASTIPADDRESS_TABLE* Table
    )
{
    return PhNetioEnumerateAddress(NsiActive, Family, PhpNetioAddressMulticast, Table);
}

/**
 * Checks that a caller buffer can hold a table with a count header and the specified rows.
 *
 * \param RowCount The number of rows.
 * \param RowSize The size, in bytes, of each row.
 * \param Table The caller buffer.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getiftable
 */
static NTSTATUS PhpNetioMibCheckTableBufferSize(
    _In_ ULONG RowCount,
    _In_ ULONG RowSize,
    _In_opt_ PVOID Table,
    _Inout_ PULONG SizePointer
    )
{
    ULONG requiredSize;
    ULONG rowBytes;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    if (RtlULongMult(RowCount, RowSize, &rowBytes) != STATUS_SUCCESS)
        return STATUS_INTEGER_OVERFLOW;

    if (RtlULongAdd(FIELD_OFFSET(MIB_IFTABLE, table), rowBytes, &requiredSize) != STATUS_SUCCESS)
        return STATUS_INTEGER_OVERFLOW;

    if (!Table || *SizePointer < requiredSize)
    {
        *SizePointer = requiredSize;
        return STATUS_BUFFER_TOO_SMALL;
    }

    *SizePointer = requiredSize;
    return STATUS_SUCCESS;
}

/**
 * Determines whether a buffer can hold the specified rows without overflow.
 *
 * \param RowCount The number of rows.
 * \param RowSize The size, in bytes, of each row.
 * \param BufferSize The size, in bytes, of the buffer.
 * \return TRUE if the rows fit, otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-freemibtable
 */
static BOOLEAN PhpNetioMibTableHasCapacity(
    _In_ ULONG RowCount,
    _In_ ULONG RowSize,
    _In_ ULONG BufferSize
    )
{
    ULONG requiredSize;

    if (RtlULongMult(RowCount, RowSize, &requiredSize) != STATUS_SUCCESS)
        return FALSE;

    if (RtlULongAdd(FIELD_OFFSET(MIB_IFTABLE, table), requiredSize, &requiredSize) != STATUS_SUCCESS)
        return FALSE;

    return BufferSize >= requiredSize;
}

/**
 * Computes the size of a MIB table with a count header followed by rows.
 *
 * \param RowCount The number of rows.
 * \param RowSize The size, in bytes, of each row.
 * \param IncludeOneEntry TRUE if the header already includes one row (ANY_SIZE array).
 * \param Size Receives the table size, in bytes.
 * \return TRUE on success, FALSE on overflow.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-freemibtable
 */
static BOOLEAN PhpNetioMibSizeofTable(
    _In_ ULONG RowCount,
    _In_ ULONG RowSize,
    _In_ BOOLEAN IncludeOneEntry,
    _Out_ PULONG Size
    )
{
    ULONG rows;
    ULONG tableSize;

    if (!Size)
        return FALSE;

    rows = RowCount + (IncludeOneEntry ? 1 : 0);

    if (RtlULongMult(rows, RowSize, &tableSize) != STATUS_SUCCESS)
        return FALSE;

    if (RtlULongAdd(FIELD_OFFSET(MIB_IFTABLE, table), tableSize, Size) != STATUS_SUCCESS)
        return FALSE;

    return TRUE;
}

/**
 * Extracts the IPv4 address from an IPv4-mapped IPv6 address.
 *
 * \param Address The address.
 * \return The IPv4 address, in network byte order.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/in6addr/ns-in6addr-in6_addr
 */
static ULONG PhpNetioIpv4AddressFromIn6Address(
    _In_ const IN6_ADDR* Address
    )
{
    ULONG address;

    memcpy(&address, &Address->s6_addr[PH_IPV4_MAPPED_IN6_ADDRESS_OFFSET], sizeof(ULONG));

    return address;
}

/**
 * Returns an IN_ADDR as a ULONG.
 *
 * \param Address The address.
 * \return The IPv4 address, in network byte order.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winsock2/ns-winsock2-in_addr
 */
static ULONG PhpNetioIpv4AddressFromInAddress(
    _In_ IN_ADDR Address
    )
{
    return Address.s_addr;
}

/**
 * Builds a legacy MIB_IFROW from the NDIS NSI interface parameter blocks.
 *
 * \param IfRow Receives the row.
 * \param ReadWriteRow The NDIS_NSI_INTERFACE_INFORMATION_RW block.
 * \param DynamicRow The NDIS_NSI_INTERFACE_INFORMATION_ROD block.
 * \param StaticRow The NDIS_NSI_INTERFACE_INFORMATION_ROS block.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ifmib/ns-ifmib-mib_ifrow
 */
static VOID PhpFillIfEntry(
    _Out_ PMIB_IFROW IfRow,
    _In_ PVOID ReadWriteRow,
    _In_ PVOID DynamicRow,
    _In_ PVOID StaticRow
    )
{
    const NDIS_NSI_INTERFACE_INFORMATION_RW* rw = ReadWriteRow;
    const NDIS_NSI_INTERFACE_INFORMATION_ROD* rod = DynamicRow;
    const NDIS_NSI_INTERFACE_INFORMATION_ROS* ros = StaticRow;
    SIZE_T descriptionLength;

    memset(IfRow, 0, sizeof(MIB_IFROW));
    IfRow->dwIndex = ros->ifIndex;
    IfRow->dwType = ros->ifType;
    IfRow->dwMtu = rod->ifMtu;
    IfRow->dwSpeed = (DWORD)min(rod->XmitLinkSpeed, MAXULONG);
    IfRow->dwPhysAddrLen = min(rw->ifPhysAddress.Length, sizeof(IfRow->bPhysAddr));
    memcpy(IfRow->bPhysAddr, rw->ifPhysAddress.Address, IfRow->dwPhysAddrLen);
    IfRow->dwAdminStatus = rw->ifAdminStatus;
    IfRow->dwOperStatus = (INTERNAL_IF_OPER_STATUS)rod->ifOperStatus;
    IfRow->dwLastChange = (DWORD)rod->ifLastChange;
    IfRow->dwInOctets = (DWORD)rod->ifHCInOctets;
    IfRow->dwInUcastPkts = (DWORD)rod->ifHCInUcastPkts;
    IfRow->dwInNUcastPkts = (DWORD)(rod->ifHCInMulticastPkts + rod->ifHCInBroadcastPkts);
    IfRow->dwInDiscards = (DWORD)rod->ifInDiscards;
    IfRow->dwInErrors = (DWORD)rod->ifInErrors;
    IfRow->dwInUnknownProtos = (DWORD)rod->ifInUnknownProtos;
    IfRow->dwOutOctets = (DWORD)rod->ifHCOutOctets;
    IfRow->dwOutUcastPkts = (DWORD)rod->ifHCOutUcastPkts;
    IfRow->dwOutNUcastPkts = (DWORD)(rod->ifHCOutMulticastPkts + rod->ifHCOutBroadcastPkts);
    IfRow->dwOutDiscards = (DWORD)rod->ifOutDiscards;
    IfRow->dwOutErrors = (DWORD)rod->ifOutErrors;
    descriptionLength = min(ros->ifDescr.Length, sizeof(IfRow->bDescr));
    memcpy(IfRow->bDescr, ros->ifDescr.String, descriptionLength);
    IfRow->dwDescrLen = (DWORD)descriptionLength;
}

/**
 * Builds an IPv4 TCP table of the requested class from the TCP NSI provider.
 *
 * \param TcpTable The buffer that receives the TCP table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param TableClass The type of table to retrieve.
 * \param OwnerClass The owner information class to include in each row.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getextendedtcptable
 */
static NTSTATUS PhpGetTcpTableInternal(
    _Inout_updates_bytes_opt_(*SizePointer) PVOID TcpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order,
    _In_ ADDRESS_FAMILY Family,
    _In_ TCP_TABLE_CLASS TableClass,
    _In_ ULONG OwnerClass
    )
{
    NTSTATUS result;
    ULONG rowSize;
    NSI_TCP_KEY_ROW* keyTable = NULL;
    NSI_TCP_STATUS_ROW* statusTable = NULL;
    NSI_TCP_OWNER_ROW* ownerTable = NULL;
    ULONG count = 0;
    ULONG errorCount = 0;
    ULONG requiredSize = 0;
    PMIB_TCPTABLE tcpTable;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    switch (OwnerClass)
    {
    case PhpTcpOwnerBasic:
        rowSize = sizeof(MIB_TCPROW);
        break;
    case PhpTcpOwnerBasic2:
        rowSize = sizeof(MIB_TCPROW2);
        break;
    case PhpTcpOwnerModule:
        rowSize = sizeof(MIB_TCPROW_OWNER_MODULE);
        break;
    case PhpTcpOwnerPid:
        rowSize = sizeof(MIB_TCPROW_OWNER_PID);
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTableIdTcp,
        &keyTable,
        sizeof(NSI_TCP_KEY_ROW),
        NULL,
        0,
        &statusTable,
        sizeof(NSI_TCP_STATUS_ROW),
        (OwnerClass == 0) ? NULL : &ownerTable,
        (OwnerClass == 0) ? 0 : sizeof(NSI_TCP_OWNER_ROW),
        &count,
        0
        );

    if (!NT_SUCCESS(result))
        return result;

    if (!PhpNetioMibSizeofTable(count, rowSize, FALSE, &requiredSize))
    {
        PhNsiFreeTable(keyTable, NULL, statusTable, ownerTable);
        return STATUS_UNSUCCESSFUL;
    }

    if (!TcpTable || *SizePointer < requiredSize)
    {
        *SizePointer = requiredSize;
        PhNsiFreeTable(keyTable, NULL, statusTable, ownerTable);
        return STATUS_BUFFER_TOO_SMALL;
    }

    tcpTable = (PMIB_TCPTABLE)TcpTable;
    tcpTable->dwNumEntries = 0;

    for (ULONG i = 0; i < count; i++)
    {
        NSI_TCP_KEY_ROW* key = &keyTable[i];

        if (key->LocalAddress.si_family != Family)
            continue;

        switch (TableClass)
        {
        case TCP_TABLE_BASIC_LISTENER:
        case TCP_TABLE_OWNER_PID_LISTENER:
        case TCP_TABLE_OWNER_MODULE_LISTENER:
            if (statusTable[i].State != MIB_TCP_STATE_LISTEN)
                continue;
            break;
        case TCP_TABLE_BASIC_CONNECTIONS:
        case TCP_TABLE_OWNER_PID_CONNECTIONS:
        case TCP_TABLE_OWNER_MODULE_CONNECTIONS:
            if (statusTable[i].State == MIB_TCP_STATE_LISTEN)
                continue;
            break;
        }

        if (!PhpNetioMibTableHasCapacity(
            tcpTable->dwNumEntries + 1,
            rowSize,
            *SizePointer
            ))
        {
            errorCount++;
            continue;
        }

        switch (OwnerClass)
        {
        case PhpTcpOwnerBasic:
            {
                PMIB_TCPTABLE table;
                PMIB_TCPROW row;

                table = (PMIB_TCPTABLE)TcpTable;
                row = &table->table[table->dwNumEntries];
                memset(row, 0, sizeof(MIB_TCPROW));
                row->dwState = statusTable[i].State;
                row->dwLocalAddr = key->LocalAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwLocalPort = key->LocalAddress.Ipv4.sin_port;
                row->dwRemoteAddr = key->RemoteAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwRemotePort = key->RemoteAddress.Ipv4.sin_port;
            }
            break;
        case PhpTcpOwnerBasic2:
            {
                PMIB_TCPTABLE2 table;
                PMIB_TCPROW2 row;

                table = (PMIB_TCPTABLE2)TcpTable;
                row = &table->table[table->dwNumEntries];
                memset(row, 0, sizeof(MIB_TCPROW2));
                row->dwState = statusTable[i].State;
                row->dwLocalAddr = key->LocalAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwLocalPort = key->LocalAddress.Ipv4.sin_port;
                row->dwRemoteAddr = key->RemoteAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwRemotePort = key->RemoteAddress.Ipv4.sin_port;
                row->dwOwningPid = ownerTable[i].OwningPid;
                row->dwOffloadState = statusTable[i].OffloadState;
            }
            break;
        case PhpTcpOwnerModule:
            {
                PMIB_TCPTABLE_OWNER_MODULE table;
                PMIB_TCPROW_OWNER_MODULE row;

                table = (PMIB_TCPTABLE_OWNER_MODULE)TcpTable;
                row = &table->table[table->dwNumEntries];
                memset(row, 0, sizeof(MIB_TCPROW_OWNER_MODULE));
                row->dwState = statusTable[i].State;
                row->dwLocalAddr = key->LocalAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwLocalPort = key->LocalAddress.Ipv4.sin_port;
                row->dwRemoteAddr = key->RemoteAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwRemotePort = key->RemoteAddress.Ipv4.sin_port;
                row->dwOwningPid = ownerTable[i].OwningPid;
                row->liCreateTimestamp = ownerTable[i].CreateTimestamp;
                row->OwningModuleInfo[0] = ownerTable[i].OwningModuleInfo;
            }
            break;
        case PhpTcpOwnerPid:
            {
                PMIB_TCPTABLE_OWNER_PID table;
                PMIB_TCPROW_OWNER_PID row;

                table = (PMIB_TCPTABLE_OWNER_PID)TcpTable;
                row = &table->table[table->dwNumEntries];
                memset(row, 0, sizeof(MIB_TCPROW_OWNER_PID));
                row->dwState = statusTable[i].State;
                row->dwLocalAddr = key->LocalAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwLocalPort = key->LocalAddress.Ipv4.sin_port;
                row->dwRemoteAddr = key->RemoteAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwRemotePort = key->RemoteAddress.Ipv4.sin_port;
                row->dwOwningPid = ownerTable[i].OwningPid;
            }
            break;
        }

        tcpTable->dwNumEntries++;
    }

    PhNsiFreeTable(keyTable, NULL, statusTable, ownerTable);

    if (!PhpNetioMibSizeofTable(
        errorCount + tcpTable->dwNumEntries,
        rowSize,
        errorCount != 0,
        &requiredSize
        ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    *SizePointer = requiredSize;

    if (errorCount)
        return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

// NSI object and parameter block sizes per address type (iphlpapi!NetioGetAddress/NetioEnumerateAddress).
// Multicast has a 12-byte dynamic row (ScopeId first) and a 4-byte static row (InterfaceIndex); anycast has only keys.
static const struct
{
    NSI_OBJECT_ID ObjectId;
    ULONG RwSize;
    ULONG RodSize;
    ULONG RosSize;
    ULONG RowSize;
} PhpNetioAddressTypeInfo[] =
{
    { NsiIpObjectUnicastAddress, sizeof(MIB_IP_UNICAST_RW_ROW), sizeof(MIB_IP_UNICAST_DYNAMIC_ROW), sizeof(MIB_IP_UNICAST_STATIC_ROW), sizeof(MIB_UNICASTIPADDRESS_ROW) },
    { NsiIpObjectMulticastAddress, 0, 0xC, sizeof(NET_IFINDEX), sizeof(MIB_MULTICASTIPADDRESS_ROW) },
    { NsiIpObjectAnycastAddress, 0, 0, 0, sizeof(MIB_ANYCASTIPADDRESS_ROW) },
};

static_assert(FIELD_OFFSET(MIB_IP_UNICAST_KEY_ROW_V4, Address) == FIELD_OFFSET(MIB_IP_UNICAST_KEY_ROW_V6, Address));
static_assert(FIELD_OFFSET(MIB_IP_ANYCAST_KEY_ROW_V4, Address) == FIELD_OFFSET(MIB_IP_UNICAST_KEY_ROW_V4, Address));
static_assert(FIELD_OFFSET(MIB_UNICASTIPADDRESS_TABLE, Table) == FIELD_OFFSET(MIB_MULTICASTIPADDRESS_TABLE, Table));
static_assert(FIELD_OFFSET(MIB_UNICASTIPADDRESS_TABLE, Table) == FIELD_OFFSET(MIB_ANYCASTIPADDRESS_TABLE, Table));

/**
 * Stores an NSI address key into a SOCKADDR_INET (iphlpapi!NetioSetAddress). For IPv6 the scope ID is cleared for
 * loopback and global addresses, and reduced to its zone when its level matches the address scope.
 */
VOID PhpNetioSetSocketAddress(
    _In_ ADDRESS_FAMILY Family,
    _Out_ PSOCKADDR_INET Address,
    _In_ const VOID* KeyAddress,
    _In_ ULONG ScopeId
    )
{
    const IN6_ADDR* address6;
    ULONG level;

    if (Family != AF_INET6)
    {
        memset(&Address->Ipv4, 0, sizeof(Address->Ipv4));
        Address->Ipv4.sin_family = AF_INET;
        Address->Ipv4.sin_addr = *(const IN_ADDR*)KeyAddress;
        return;
    }

    memset(&Address->Ipv6, 0, sizeof(Address->Ipv6));
    Address->Ipv6.sin6_family = AF_INET6;
    Address->Ipv6.sin6_addr = *(const IN6_ADDR*)KeyAddress;
    Address->Ipv6.sin6_scope_id = ScopeId;
    address6 = &Address->Ipv6.sin6_addr;

    if (address6->s6_bytes[0] == 0xFF)
        level = address6->s6_bytes[1] & 0xF; // multicast scope field
    else if (IN6_IS_ADDR_LINKLOCAL(address6) || IN6_IS_ADDR_LOOPBACK(address6))
        level = ScopeLevelLink;
    else if (IN6_IS_ADDR_SITELOCAL(address6))
        level = ScopeLevelSite;
    else
        level = ScopeLevelGlobal;

    if (IN6_IS_ADDR_LOOPBACK(address6) || level == ScopeLevelGlobal)
    {
        Address->Ipv6.sin6_scope_id = 0;
        ScopeId = 0;
    }

    if ((ScopeId >> 28) == level)
        Address->Ipv6.sin6_scope_id = ScopeId & 0x0FFFFFFF;
}

/**
 * Converts an NSI address key and its parameter blocks into a MIB_*IPADDRESS_ROW (iphlpapi!NetioFillAddressRow).
 */
VOID PhpNetioFillAddressRow(
    _In_ ADDRESS_FAMILY Family,
    _In_ PHP_NETIO_ADDRESS_TYPE AddressType,
    _Out_ PVOID Row,
    _In_ const VOID* Key,
    _In_opt_ const VOID* Rw,
    _In_opt_ const VOID* Rod,
    _In_opt_ const VOID* Ros
    )
{
    NET_LUID interfaceLuid = *(const NET_LUID*)Key;
    const VOID* keyAddress = PTR_ADD_OFFSET(Key, FIELD_OFFSET(MIB_IP_UNICAST_KEY_ROW_V4, Address));
    NET_IFINDEX interfaceIndex;

    switch (AddressType)
    {
    case PhpNetioAddressUnicast:
        {
            PMIB_UNICASTIPADDRESS_ROW row = Row;
            const MIB_IP_UNICAST_RW_ROW* rw = Rw;
            const MIB_IP_UNICAST_DYNAMIC_ROW* rod = Rod;
            const MIB_IP_UNICAST_STATIC_ROW* ros = Ros;

            memset(row, 0, sizeof(MIB_UNICASTIPADDRESS_ROW));
            row->InterfaceLuid = interfaceLuid;
            if (PhConvertInterfaceLuidToIndex(&interfaceLuid, &interfaceIndex) == STATUS_SUCCESS)
                row->InterfaceIndex = interfaceIndex;

            PhpNetioSetSocketAddress(Family, &row->Address, keyAddress, rod ? rod->ScopeId.Value : 0);

            if (rw)
            {
                row->PrefixOrigin = rw->PrefixOrigin;
                row->SuffixOrigin = rw->SuffixOrigin;
                row->ValidLifetime = rw->ValidLifetime;
                row->PreferredLifetime = rw->PreferredLifetime;
                row->OnLinkPrefixLength = rw->OnLinkPrefixLength;
                row->SkipAsSource = rw->SkipAsSource;
            }

            if (rod)
            {
                row->DadState = rod->DadState;
                row->ScopeId = rod->ScopeId;
            }

            if (ros)
                row->CreationTimeStamp.QuadPart = (LONGLONG)ros->CreationTimestamp;
        }
        break;
    case PhpNetioAddressMulticast:
        {
            PMIB_MULTICASTIPADDRESS_ROW row = Row;
            ULONG scopeId = Rod ? *(const ULONG*)Rod : 0;

            memset(row, 0, sizeof(MIB_MULTICASTIPADDRESS_ROW));
            row->InterfaceLuid = interfaceLuid;

            if (Ros)
                row->InterfaceIndex = *(const NET_IFINDEX*)Ros;
            else if (PhConvertInterfaceLuidToIndex(&interfaceLuid, &interfaceIndex) == STATUS_SUCCESS)
                row->InterfaceIndex = interfaceIndex;

            PhpNetioSetSocketAddress(Family, &row->Address, keyAddress, scopeId);

            if (Rod)
                row->ScopeId.Value = scopeId;
        }
        break;
    case PhpNetioAddressAnycast:
        {
            PMIB_ANYCASTIPADDRESS_ROW row = Row;

            memset(row, 0, sizeof(MIB_ANYCASTIPADDRESS_ROW));
            row->InterfaceLuid = interfaceLuid;
            if (PhConvertInterfaceLuidToIndex(&interfaceLuid, &interfaceIndex) == STATUS_SUCCESS)
                row->InterfaceIndex = interfaceIndex;

            PhpNetioSetSocketAddress(Family, &row->Address, keyAddress, 0);
        }
        break;
    }
}

/**
 * Retrieves a unicast, multicast or anycast address row from the IP NSI provider (iphlpapi!NetioGetAddress).
 *
 * \param Store The NSI store to query (active or persistent).
 * \param AddressType The type of address.
 * \param Row On input, the key fields of the address row; on output, the complete row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getunicastipaddressentry
 */
static NTSTATUS PhNetioGetAddress(
    _In_ NSI_STORE Store,
    _In_ PHP_NETIO_ADDRESS_TYPE AddressType,
    _Inout_ PVOID Row
    )
{
    union
    {
        MIB_IP_UNICAST_KEY_ROW_V4 V4;
        MIB_IP_UNICAST_KEY_ROW_V6 V6;
    } key;
    MIB_IP_UNICAST_RW_ROW rw;
    ULONG rod[3]; // largest dynamic row (multicast, 0xC bytes)
    MIB_IP_UNICAST_STATIC_ROW ros;
    PSOCKADDR_INET address;
    PNET_LUID interfaceLuid;
    NET_IFINDEX interfaceIndex;
    ULONG keySize;
    NTSTATUS result;

    if (!Row || (ULONG)AddressType >= RTL_NUMBER_OF(PhpNetioAddressTypeInfo))
        return STATUS_INVALID_PARAMETER;

    static_assert(sizeof(rod) >= sizeof(MIB_IP_UNICAST_DYNAMIC_ROW));
    static_assert(sizeof(ros) >= sizeof(NET_IFINDEX));

    switch (AddressType)
    {
    case PhpNetioAddressUnicast:
        address = &((PMIB_UNICASTIPADDRESS_ROW)Row)->Address;
        interfaceLuid = &((PMIB_UNICASTIPADDRESS_ROW)Row)->InterfaceLuid;
        interfaceIndex = ((PMIB_UNICASTIPADDRESS_ROW)Row)->InterfaceIndex;
        break;
    case PhpNetioAddressMulticast:
        address = &((PMIB_MULTICASTIPADDRESS_ROW)Row)->Address;
        interfaceLuid = &((PMIB_MULTICASTIPADDRESS_ROW)Row)->InterfaceLuid;
        interfaceIndex = ((PMIB_MULTICASTIPADDRESS_ROW)Row)->InterfaceIndex;
        break;
    default:
        address = &((PMIB_ANYCASTIPADDRESS_ROW)Row)->Address;
        interfaceLuid = &((PMIB_ANYCASTIPADDRESS_ROW)Row)->InterfaceLuid;
        interfaceIndex = ((PMIB_ANYCASTIPADDRESS_ROW)Row)->InterfaceIndex;
        break;
    }

    memset(&key, 0, sizeof(key));

    if (address->si_family == AF_INET)
    {
        key.V4.Address = address->Ipv4.sin_addr;
        keySize = sizeof(MIB_IP_UNICAST_KEY_ROW_V4);
    }
    else if (address->si_family == AF_INET6)
    {
        key.V6.Address = address->Ipv6.sin6_addr;
        keySize = sizeof(MIB_IP_UNICAST_KEY_ROW_V6);
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    // The row may identify the interface by index alone.
    if (interfaceLuid->Value)
    {
        key.V6.InterfaceLuid = *interfaceLuid;
    }
    else
    {
        result = PhConvertInterfaceIndexToLuid(interfaceIndex, &key.V6.InterfaceLuid);
        if (!NT_SUCCESS(result))
            return result;
    }

    memset(&rw, 0, sizeof(rw));
    memset(rod, 0, sizeof(rod));
    memset(&ros, 0, sizeof(ros));

    result = PhNsiGetAllParameters(
        Store,
        address->si_family == AF_INET6 ? &NPI_MS_IPV6_MODULEID : &NPI_MS_IPV4_MODULEID,
        PhpNetioAddressTypeInfo[AddressType].ObjectId,
        &key,
        keySize,
        PhpNetioAddressTypeInfo[AddressType].RwSize ? &rw : NULL,
        PhpNetioAddressTypeInfo[AddressType].RwSize,
        PhpNetioAddressTypeInfo[AddressType].RodSize ? rod : NULL,
        PhpNetioAddressTypeInfo[AddressType].RodSize,
        PhpNetioAddressTypeInfo[AddressType].RosSize ? &ros : NULL,
        PhpNetioAddressTypeInfo[AddressType].RosSize
        );

    if (!NT_SUCCESS(result))
        return result;

    PhpNetioFillAddressRow(
        address->si_family,
        AddressType,
        Row,
        &key,
        PhpNetioAddressTypeInfo[AddressType].RwSize ? &rw : NULL,
        PhpNetioAddressTypeInfo[AddressType].RodSize ? rod : NULL,
        PhpNetioAddressTypeInfo[AddressType].RosSize ? &ros : NULL
        );

    return STATUS_SUCCESS;
}

/**
 * Retrieves the unicast, multicast or anycast address table from the IP NSI provider (iphlpapi!NetioEnumerateAddress).
 *
 * \param Store The NSI store to query (active or persistent).
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param AddressType The type of address.
 * \param Table Receives the allocated table. Free it with FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getunicastipaddresstable
 */
static NTSTATUS PhNetioEnumerateAddress(
    _In_ NSI_STORE Store,
    _In_ ADDRESS_FAMILY Family,
    _In_ PHP_NETIO_ADDRESS_TYPE AddressType,
    _Outptr_ PVOID *Table
    )
{
    static const ADDRESS_FAMILY families[2] = { AF_INET, AF_INET6 };
    PVOID keys[2] = { NULL, NULL };
    PVOID rw[2] = { NULL, NULL };
    PVOID rod[2] = { NULL, NULL };
    PVOID ros[2] = { NULL, NULL };
    ULONG counts[2] = { 0, 0 };
    ULONG keySizes[2] = { sizeof(MIB_IP_UNICAST_KEY_ROW_V4), sizeof(MIB_IP_UNICAST_KEY_ROW_V6) };
    ULONG rwSize;
    ULONG rodSize;
    ULONG rosSize;
    ULONG rowSize;
    ULONG totalCount;
    SIZE_T tableSize;
    PULONG table;
    ULONG entryIndex;
    NTSTATUS result;

    if (!Table || (ULONG)AddressType >= RTL_NUMBER_OF(PhpNetioAddressTypeInfo))
        return STATUS_INVALID_PARAMETER;
    if (Family != AF_UNSPEC && Family != AF_INET && Family != AF_INET6)
        return STATUS_INVALID_PARAMETER;

    rwSize = PhpNetioAddressTypeInfo[AddressType].RwSize;
    rodSize = PhpNetioAddressTypeInfo[AddressType].RodSize;
    rosSize = PhpNetioAddressTypeInfo[AddressType].RosSize;
    rowSize = PhpNetioAddressTypeInfo[AddressType].RowSize;

    for (ULONG i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (Family != AF_UNSPEC && Family != families[i])
            continue;

        result = PhNsiAllocateAndGetTable(
            Store,
            families[i] == AF_INET6 ? &NPI_MS_IPV6_MODULEID : &NPI_MS_IPV4_MODULEID,
            PhpNetioAddressTypeInfo[AddressType].ObjectId,
            &keys[i],
            keySizes[i],
            rwSize ? &rw[i] : NULL,
            rwSize,
            rodSize ? &rod[i] : NULL,
            rodSize,
            rosSize ? &ros[i] : NULL,
            rosSize,
            &counts[i],
            0
            );

        // A family that is not installed or has no addresses contributes no rows.
        if (result == STATUS_NOT_FOUND || result == STATUS_NOT_SUPPORTED)
        {
            counts[i] = 0;
            continue;
        }

        if (!NT_SUCCESS(result))
            goto CleanupExit;
    }

    totalCount = counts[0] + counts[1];

    if (totalCount < counts[0] ||
        totalCount > (SIZE_MAX - FIELD_OFFSET(MIB_UNICASTIPADDRESS_TABLE, Table)) / rowSize)
    {
        result = STATUS_INTEGER_OVERFLOW;
        goto CleanupExit;
    }

    tableSize = FIELD_OFFSET(MIB_UNICASTIPADDRESS_TABLE, Table) + (SIZE_T)totalCount * rowSize;
    table = PhAllocateZero(tableSize);
    entryIndex = 0;

    for (ULONG i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        for (ULONG j = 0; j < counts[i]; j++)
        {
            PhpNetioFillAddressRow(
                families[i],
                AddressType,
                PTR_ADD_OFFSET(table, FIELD_OFFSET(MIB_UNICASTIPADDRESS_TABLE, Table) + (SIZE_T)entryIndex * rowSize),
                PTR_ADD_OFFSET(keys[i], (SIZE_T)j * keySizes[i]),
                rw[i] ? PTR_ADD_OFFSET(rw[i], (SIZE_T)j * rwSize) : NULL,
                rod[i] ? PTR_ADD_OFFSET(rod[i], (SIZE_T)j * rodSize) : NULL,
                ros[i] ? PTR_ADD_OFFSET(ros[i], (SIZE_T)j * rosSize) : NULL
                );
            entryIndex++;
        }
    }

    *table = entryIndex; // NumEntries
    *Table = table;
    result = STATUS_SUCCESS;

CleanupExit:
    for (ULONG i = 0; i < RTL_NUMBER_OF(families); i++)
        PhNsiFreeTable(keys[i], rw[i], rod[i], ros[i]);

    return result;
}

/**
 * Builds an IPv6 TCP table of the requested class from the TCP NSI provider.
 *
 * \param TcpTable The buffer that receives the TCP table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \param TableClass The type of table to retrieve.
 * \param OwnerClass The owner information class to include in each row.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getextendedtcptable
 */
static NTSTATUS PhpGetTcp6TableInternal(
    _Inout_updates_bytes_opt_(*SizePointer) PVOID TcpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order,
    _In_ TCP_TABLE_CLASS TableClass,
    _In_ ULONG OwnerClass
    )
{
    NTSTATUS result;
    ULONG rowSize;
    NSI_TCP_KEY_ROW* keyTable = NULL;
    NSI_TCP_STATUS_ROW* statusTable = NULL;
    NSI_TCP_OWNER_ROW* ownerTable = NULL;
    ULONG count = 0;
    ULONG errorCount = 0;
    ULONG requiredSize = 0;
    PMIB_TCP6TABLE tcp6Table;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    switch (OwnerClass)
    {
    case PhpTcpOwnerBasic:
        rowSize = sizeof(MIB_TCP6ROW);
        break;
    case PhpTcpOwnerBasic2:
        rowSize = sizeof(MIB_TCP6ROW2);
        break;
    case PhpTcpOwnerModule:
        rowSize = sizeof(MIB_TCP6ROW_OWNER_MODULE);
        break;
    case PhpTcpOwnerPid:
        rowSize = sizeof(MIB_TCP6ROW_OWNER_PID);
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTableIdTcp6,
        &keyTable,
        sizeof(NSI_TCP_KEY_ROW),
        NULL,
        0,
        &statusTable,
        sizeof(NSI_TCP_STATUS_ROW),
        (OwnerClass == 0) ? NULL : &ownerTable,
        (OwnerClass == 0) ? 0 : sizeof(NSI_TCP_OWNER_ROW),
        &count,
        0
        );

    if (!NT_SUCCESS(result))
        return result;

    if (!PhpNetioMibSizeofTable(count, rowSize, FALSE, &requiredSize))
    {
        PhNsiFreeTable(keyTable, NULL, statusTable, ownerTable);
        return STATUS_UNSUCCESSFUL;
    }

    if (!TcpTable || *SizePointer < requiredSize)
    {
        *SizePointer = requiredSize;
        PhNsiFreeTable(keyTable, NULL, statusTable, ownerTable);
        return STATUS_BUFFER_TOO_SMALL;
    }

    tcp6Table = (PMIB_TCP6TABLE)TcpTable;
    tcp6Table->dwNumEntries = 0;

    for (ULONG i = 0; i < count; i++)
    {
        NSI_TCP_KEY_ROW* key = &keyTable[i];

        if (key->LocalAddress.si_family != AF_INET6)
            continue;

        if (IN6_IS_ADDR_V4MAPPED(&key->LocalAddress.Ipv6.sin6_addr))
        {
            continue;
        }

        switch (TableClass)
        {
        case TCP_TABLE_BASIC_LISTENER:
        case TCP_TABLE_OWNER_PID_LISTENER:
        case TCP_TABLE_OWNER_MODULE_LISTENER:
            if (statusTable[i].State != MIB_TCP_STATE_LISTEN)
                continue;
            break;
        case TCP_TABLE_BASIC_CONNECTIONS:
        case TCP_TABLE_OWNER_PID_CONNECTIONS:
        case TCP_TABLE_OWNER_MODULE_CONNECTIONS:
            if (statusTable[i].State == MIB_TCP_STATE_LISTEN)
                continue;
            break;
        }

        if (!PhpNetioMibTableHasCapacity(
            tcp6Table->dwNumEntries + 1,
            rowSize,
            *SizePointer
            ))
        {
            errorCount++;
            continue;
        }

        switch (OwnerClass)
        {
        case PhpTcpOwnerBasic:
            {
                PMIB_TCP6TABLE table = (PMIB_TCP6TABLE)TcpTable;
                PMIB_TCP6ROW row = &table->table[table->dwNumEntries];

                memset(row, 0, sizeof(MIB_TCP6ROW));
                row->State = statusTable[i].State;
                row->LocalAddr = key->LocalAddress.Ipv6.sin6_addr;
                row->dwLocalScopeId = key->LocalAddress.Ipv6.sin6_scope_id;
                row->dwLocalPort = key->LocalAddress.Ipv6.sin6_port;
                row->RemoteAddr = key->RemoteAddress.Ipv6.sin6_addr;
                row->dwRemoteScopeId = key->RemoteAddress.Ipv6.sin6_scope_id;
                row->dwRemotePort = key->RemoteAddress.Ipv6.sin6_port;
            }
            break;
        case PhpTcpOwnerBasic2:
            {
                PMIB_TCP6TABLE2 table = (PMIB_TCP6TABLE2)TcpTable;
                PMIB_TCP6ROW2 row = &table->table[table->dwNumEntries];

                memset(row, 0, sizeof(MIB_TCP6ROW2));
                row->LocalAddr = key->LocalAddress.Ipv6.sin6_addr;
                row->dwLocalScopeId = key->LocalAddress.Ipv6.sin6_scope_id;
                row->dwLocalPort = key->LocalAddress.Ipv6.sin6_port;
                row->RemoteAddr = key->RemoteAddress.Ipv6.sin6_addr;
                row->dwRemoteScopeId = key->RemoteAddress.Ipv6.sin6_scope_id;
                row->dwRemotePort = key->RemoteAddress.Ipv6.sin6_port;
                row->State = statusTable[i].State;
                row->dwOwningPid = ownerTable[i].OwningPid;
                row->dwOffloadState = statusTable[i].OffloadState;
            }
            break;
        case PhpTcpOwnerModule:
            {
                PMIB_TCP6TABLE_OWNER_MODULE table = (PMIB_TCP6TABLE_OWNER_MODULE)TcpTable;
                PMIB_TCP6ROW_OWNER_MODULE row = &table->table[table->dwNumEntries];

                memset(row, 0, sizeof(MIB_TCP6ROW_OWNER_MODULE));
                row->dwState = statusTable[i].State;
                memcpy(row->ucLocalAddr, key->LocalAddress.Ipv6.sin6_addr.s6_addr, sizeof(row->ucLocalAddr));
                row->dwLocalScopeId = key->LocalAddress.Ipv6.sin6_scope_id;
                row->dwLocalPort = key->LocalAddress.Ipv6.sin6_port;
                memcpy(row->ucRemoteAddr, key->RemoteAddress.Ipv6.sin6_addr.s6_addr, sizeof(row->ucRemoteAddr));
                row->dwRemoteScopeId = key->RemoteAddress.Ipv6.sin6_scope_id;
                row->dwRemotePort = key->RemoteAddress.Ipv6.sin6_port;
                row->dwOwningPid = ownerTable[i].OwningPid;
                row->liCreateTimestamp = ownerTable[i].CreateTimestamp;
                row->OwningModuleInfo[0] = ownerTable[i].OwningModuleInfo;
            }
            break;
        case PhpTcpOwnerPid:
            {
                PMIB_TCP6TABLE_OWNER_PID table = (PMIB_TCP6TABLE_OWNER_PID)TcpTable;
                PMIB_TCP6ROW_OWNER_PID row = &table->table[table->dwNumEntries];

                memset(row, 0, sizeof(MIB_TCP6ROW_OWNER_PID));
                row->dwState = statusTable[i].State;
                memcpy(row->ucLocalAddr, key->LocalAddress.Ipv6.sin6_addr.s6_addr, sizeof(row->ucLocalAddr));
                row->dwLocalScopeId = key->LocalAddress.Ipv6.sin6_scope_id;
                row->dwLocalPort = key->LocalAddress.Ipv6.sin6_port;
                memcpy(row->ucRemoteAddr, key->RemoteAddress.Ipv6.sin6_addr.s6_addr, sizeof(row->ucRemoteAddr));
                row->dwRemoteScopeId = key->RemoteAddress.Ipv6.sin6_scope_id;
                row->dwRemotePort = key->RemoteAddress.Ipv6.sin6_port;
                row->dwOwningPid = ownerTable[i].OwningPid;
            }
            break;
        }

        tcp6Table->dwNumEntries++;
    }

    PhNsiFreeTable(keyTable, NULL, statusTable, ownerTable);

    if (!PhpNetioMibSizeofTable(
        errorCount + tcp6Table->dwNumEntries,
        rowSize,
        errorCount != 0,
        &requiredSize
        ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    *SizePointer = requiredSize;

    if (errorCount)
        return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

/**
 * Builds an IPv6 UDP table of the requested class from the UDP NSI provider.
 *
 * \param UdpTable The buffer that receives the UDP table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \param TableClass The type of table to retrieve.
 * \param Reserved Reserved; must be zero or NULL.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getextendedudptable
 */
static NTSTATUS PhGetUdp6TableInternal(
    _Inout_updates_bytes_opt_(*SizePointer) PVOID UdpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order,
    _In_ UDP_TABLE_CLASS TableClass,
    _Reserved_ ULONG Reserved
    )
{
    NTSTATUS result;
    ULONG rowSize;
    PNSI_UDP_KEY_ROW keyTable = NULL;
    PNSI_UDP_OWNER_ROW ownerTable = NULL;
    ULONG count = 0;
    ULONG errorCount = 0;
    ULONG requiredSize = 0;
    PMIB_UDP6TABLE udp6Table;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    switch (TableClass)
    {
    case UDP_TABLE_BASIC:
        rowSize = sizeof(MIB_UDP6ROW);
        break;
    case UDP_TABLE_OWNER_PID:
        rowSize = sizeof(MIB_UDP6ROW_OWNER_PID);
        break;
    case UDP_TABLE_OWNER_MODULE:
        rowSize = sizeof(MIB_UDP6ROW_OWNER_MODULE);
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_UDP_MODULEID,
        NsiTableIdUdp,
        &keyTable,
        sizeof(NSI_UDP_KEY_ROW),
        NULL,
        0,
        NULL,
        0,
        (TableClass == UDP_TABLE_BASIC) ? NULL : &ownerTable,
        (TableClass == UDP_TABLE_BASIC) ? 0 : sizeof(NSI_UDP_OWNER_ROW),
        &count,
        0
        );

    if (!NT_SUCCESS(result))
        return result;

    if (!PhpNetioMibSizeofTable(count, rowSize, FALSE, &requiredSize))
    {
        PhNsiFreeTable(keyTable, NULL, NULL, ownerTable);
        return STATUS_UNSUCCESSFUL;
    }

    if (!UdpTable || *SizePointer < requiredSize)
    {
        *SizePointer = requiredSize;
        PhNsiFreeTable(keyTable, NULL, NULL, ownerTable);
        return STATUS_BUFFER_TOO_SMALL;
    }

    udp6Table = (PMIB_UDP6TABLE)UdpTable;
    udp6Table->dwNumEntries = 0;

    for (ULONG i = 0; i < count; i++)
    {
        PNSI_UDP_KEY_ROW key = &keyTable[i];

        if (key->AddressFamily != AF_INET6)
            continue;

        if (IN6_IS_ADDR_V4MAPPED(&key->LocalAddressV6))
        {
            continue;
        }

        if (!PhpNetioMibTableHasCapacity(
            udp6Table->dwNumEntries + 1,
            rowSize,
            *SizePointer
            ))
        {
            errorCount++;
            continue;
        }

        switch (TableClass)
        {
        case UDP_TABLE_BASIC:
            {
                PMIB_UDP6TABLE table = (PMIB_UDP6TABLE)UdpTable;
                PMIB_UDP6ROW row = &table->table[table->dwNumEntries];

                memset(row, 0, sizeof(MIB_UDP6ROW));
                row->dwLocalAddr = key->LocalAddressV6;
                row->dwLocalScopeId = key->LocalScopeId.Value;
                row->dwLocalPort = key->LocalPort;
            }
            break;
        case UDP_TABLE_OWNER_PID:
            {
                PMIB_UDP6TABLE_OWNER_PID table = (PMIB_UDP6TABLE_OWNER_PID)UdpTable;
                PMIB_UDP6ROW_OWNER_PID row = &table->table[table->dwNumEntries];

                memset(row, 0, sizeof(MIB_UDP6ROW_OWNER_PID));
                memcpy(row->ucLocalAddr, key->LocalAddressV6.s6_addr, sizeof(row->ucLocalAddr));
                row->dwLocalScopeId = key->LocalScopeId.Value;
                row->dwLocalPort = key->LocalPort;
                row->dwOwningPid = ownerTable[i].OwningPid;
            }
            break;
        case UDP_TABLE_OWNER_MODULE:
            {
                PMIB_UDP6TABLE_OWNER_MODULE table = (PMIB_UDP6TABLE_OWNER_MODULE)UdpTable;
                PMIB_UDP6ROW_OWNER_MODULE row = &table->table[table->dwNumEntries];

                memset(row, 0, sizeof(MIB_UDP6ROW_OWNER_MODULE));
                memcpy(row->ucLocalAddr, key->LocalAddressV6.s6_addr, sizeof(row->ucLocalAddr));
                row->dwLocalScopeId = key->LocalScopeId.Value;
                row->dwLocalPort = key->LocalPort;
                row->dwOwningPid = ownerTable[i].OwningPid;
                row->liCreateTimestamp = ownerTable[i].CreateTimestamp;
                row->OwningModuleInfo[0] = ownerTable[i].OwningModuleInfo;
            }
            break;
        }

        udp6Table->dwNumEntries++;
    }

    PhNsiFreeTable(keyTable, NULL, NULL, ownerTable);

    if (!PhpNetioMibSizeofTable(
        errorCount + udp6Table->dwNumEntries,
        rowSize,
        errorCount != 0,
        &requiredSize
        ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    *SizePointer = requiredSize;

    if (errorCount)
        return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

/**
 * Builds an IPv4 UDP table of the requested class from the UDP NSI provider.
 *
 * \param UdpTable The buffer that receives the UDP table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \param TableClass The type of table to retrieve.
 * \param Reserved Reserved; must be zero or NULL.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getextendedudptable
 */
static NTSTATUS PhGetUdpTableInternal(
    _Inout_updates_bytes_opt_(*SizePointer) PVOID UdpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order,
    _In_ UDP_TABLE_CLASS TableClass,
    _Reserved_ ULONG Reserved
    )
{
    NTSTATUS result;
    ULONG rowSize;
    PNSI_UDP_KEY_ROW keyTable = NULL;
    PNSI_UDP_OWNER_ROW ownerTable = NULL;
    ULONG count = 0;
    ULONG errorCount = 0;
    ULONG requiredSize = 0;
    PMIB_UDPTABLE udpTable;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    switch (TableClass)
    {
    case UDP_TABLE_BASIC:
        rowSize = sizeof(MIB_UDPROW);
        break;
    case UDP_TABLE_OWNER_PID:
        rowSize = sizeof(MIB_UDPROW_OWNER_PID);
        break;
    case UDP_TABLE_OWNER_MODULE:
        rowSize = sizeof(MIB_UDPROW_OWNER_MODULE);
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_UDP_MODULEID,
        NsiTableIdUdp,
        &keyTable,
        sizeof(NSI_UDP_KEY_ROW),
        NULL,
        0,
        NULL,
        0,
        (TableClass == UDP_TABLE_BASIC) ? NULL : &ownerTable,
        (TableClass == UDP_TABLE_BASIC) ? 0 : sizeof(NSI_UDP_OWNER_ROW),
        &count,
        0
        );

    if (!NT_SUCCESS(result))
        return result;

    if (!PhpNetioMibSizeofTable(count, rowSize, FALSE, &requiredSize))
    {
        PhNsiFreeTable(keyTable, NULL, NULL, ownerTable);
        return STATUS_UNSUCCESSFUL;
    }

    if (!UdpTable || *SizePointer < requiredSize)
    {
        *SizePointer = requiredSize;
        PhNsiFreeTable(keyTable, NULL, NULL, ownerTable);
        return STATUS_BUFFER_TOO_SMALL;
    }

    udpTable = (PMIB_UDPTABLE)UdpTable;
    udpTable->dwNumEntries = 0;

    for (ULONG i = 0; i < count; i++)
    {
        PNSI_UDP_KEY_ROW key = &keyTable[i];

        if (key->AddressFamily != AF_INET)
            continue;

        if (!PhpNetioMibTableHasCapacity(
            udpTable->dwNumEntries + 1,
            rowSize,
            *SizePointer
            ))
        {
            errorCount++;
            continue;
        }

        switch (TableClass)
        {
        case UDP_TABLE_BASIC:
            {
                PMIB_UDPTABLE table = (PMIB_UDPTABLE)UdpTable;
                PMIB_UDPROW row = &table->table[table->dwNumEntries];

                memset(row, 0, sizeof(MIB_UDPROW));
                row->dwLocalAddr = PhpNetioIpv4AddressFromInAddress(key->LocalAddressV4);
                row->dwLocalPort = key->LocalPort;
            }
            break;
        case UDP_TABLE_OWNER_PID:
            {
                PMIB_UDPTABLE_OWNER_PID table = (PMIB_UDPTABLE_OWNER_PID)UdpTable;
                PMIB_UDPROW_OWNER_PID row = &table->table[table->dwNumEntries];

                memset(row, 0, sizeof(MIB_UDPROW_OWNER_PID));
                row->dwLocalAddr = PhpNetioIpv4AddressFromInAddress(key->LocalAddressV4);
                row->dwLocalPort = key->LocalPort;
                row->dwOwningPid = ownerTable[i].OwningPid;
            }
            break;
        case UDP_TABLE_OWNER_MODULE:
            {
                PMIB_UDPTABLE_OWNER_MODULE table = (PMIB_UDPTABLE_OWNER_MODULE)UdpTable;
                PMIB_UDPROW_OWNER_MODULE row = &table->table[table->dwNumEntries];

                memset(row, 0, sizeof(MIB_UDPROW_OWNER_MODULE));
                row->dwLocalAddr = PhpNetioIpv4AddressFromInAddress(key->LocalAddressV4);
                row->dwLocalPort = key->LocalPort;
                row->dwOwningPid = ownerTable[i].OwningPid;
                row->liCreateTimestamp = ownerTable[i].CreateTimestamp;
                row->OwningModuleInfo[0] = ownerTable[i].OwningModuleInfo;
            }
            break;
        }

        udpTable->dwNumEntries++;
    }

    PhNsiFreeTable(keyTable, NULL, NULL, ownerTable);

    if (!PhpNetioMibSizeofTable(
        errorCount + udpTable->dwNumEntries,
        rowSize,
        errorCount != 0,
        &requiredSize
        ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    *SizePointer = requiredSize;

    if (errorCount)
        return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

/**
 * Retrieves the best interface for a destination and optionally its alias index.
 *
 * \param DestinationAddress The destination address.
 * \param BestIfIndex Receives the index of the best interface.
 * \param BestIfIndexAlias Receives the index of the best interface (alias).
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getbestinterfaceex
 */
static NTSTATUS PhInternalGetBestInterface(
    _In_ PSOCKADDR DestinationAddress,
    _Out_ PULONG BestIfIndex,
    _Out_opt_ PULONG BestIfIndexAlias
    )
{
    NTSTATUS result;

    result = PhGetBestInterfaceEx(DestinationAddress, BestIfIndex);

    if (NT_SUCCESS(result) && BestIfIndexAlias)
        *BestIfIndexAlias = *BestIfIndex;

    return result;
}

/**
 * Converts a GUID to a braced string.
 *
 * \param Guid The GUID.
 * \param Buffer Receives the string.
 * \param Count The size of Buffer, in characters.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-stringfromguid2
 */
static NTSTATUS PhConvertGuidToStringW(
    _In_ const GUID *Guid,
    _Out_writes_(Count) PWSTR Buffer,
    _In_ ULONG Count
    )
{
    if (Count > USHRT_MAX / sizeof(WCHAR))
        return STATUS_INVALID_PARAMETER;

    return PhFormatGuidToBuffer(Guid, Buffer, (USHORT)(Count * sizeof(WCHAR)), NULL);
}

/**
 * Fills an IP_ADDR_STRING with an IPv4 address and mask.
 *
 * \param AddressString Receives the address strings.
 * \param Address The IPv4 address.
 * \param Mask The IPv4 mask.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_addr_string
 */
static VOID PhpSetAddressString(
    _Out_ PIP_ADDR_STRING AddressString,
    _In_ PIN_ADDR Address,
    _In_ PIN_ADDR Mask
    )
{
    RtlIpv4AddressToStringA(Address, AddressString->IpAddress.String);
    RtlIpv4AddressToStringA(Mask, AddressString->IpMask.String);
    AddressString->Context = Address->S_un.S_addr;
}

/**
 * Allocates an IP_ADDR_STRING entry and appends it to a list.
 *
 * \param Address The IPv4 address.
 * \param Mask The IPv4 mask.
 * \param Tail The tail of the list; updated to the new entry.
 * \return The new entry, or NULL.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_addr_string
 */
static PIP_ADDR_STRING PhpAddAddressStringToAdapter(
    _In_ PIN_ADDR Address,
    _In_ PIN_ADDR Mask,
    _Inout_ PIP_ADDR_STRING *Tail
    )
{
    PIP_ADDR_STRING newEntry = PhAllocateZero(sizeof(IP_ADDR_STRING));
    if (newEntry)
    {
        PhpSetAddressString(newEntry, Address, Mask);
        (*Tail)->Next = newEntry;
        *Tail = newEntry;
    }
    return newEntry;
}

/**
 * Fills an IP_ADDR_STRING list with DNS server addresses.
 *
 * \param DnsServerList The first list entry.
 * \param DnsServers The DNS server addresses.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-fixed_info_w2ksp1
 */
static VOID PhpAddDnsServerAddressInfo(
    _Inout_ PIP_ADDR_STRING DnsServerList,
    _In_ PIP4_ARRAY DnsServers
    )
{
    static const IN_ADDR broadcastAddress = { .S_un = { .S_addr = INADDR_NONE } }; // 255.255.255.255
    PIP_ADDR_STRING tail = DnsServerList;

    PhpSetAddressString(DnsServerList, (PIN_ADDR)&DnsServers->AddrArray[0], (PIN_ADDR)&broadcastAddress);

    for (ULONG i = 1; i < DnsServers->AddrCount; i++)
    {
        PhpAddAddressStringToAdapter((PIN_ADDR)&DnsServers->AddrArray[i], (PIN_ADDR)&broadcastAddress, &tail);
    }
}

//static ULONG PhAllocateAndGetFixedInfo(
//    _Outptr_result_maybenull_ PVOID *FixedInfo
//    )
//{
//    PFIXED_INFO fixedInfo;
//    ULONG pBufLen;
//    HLOCAL hMem;
//
//    if (!FixedInfo)
//        return ERROR_INVALID_PARAMETER;
//
//    *FixedInfo = NULL;
//
//    fixedInfo = PhAllocateZero(sizeof(FIXED_INFO));
//    if (!fixedInfo)
//        return ERROR_NOT_ENOUGH_MEMORY;
//
//    pBufLen = sizeof(IP4_ARRAY);
//    hMem = NULL;
//
//    if (DnsQueryConfig(DnsConfigDnsServerList, DNS_CONFIG_FLAG_ALLOC, NULL, NULL, &hMem, &pBufLen) == ERROR_SUCCESS)
//    {
//        PhpAddDnsServerAddressInfo(&fixedInfo->DnsServerList, (PIP4_ARRAY)hMem);
//        LocalFree(hMem);
//    }
//
//    pBufLen = sizeof(fixedInfo->HostName);
//    DnsQueryConfig(DnsConfigHostName_A, 0, NULL, NULL, fixedInfo->HostName, &pBufLen);
//
//    pBufLen = sizeof(fixedInfo->DomainName);
//    DnsQueryConfig(DnsConfigPrimaryDomainName_A, 0, NULL, NULL, fixedInfo->DomainName, &pBufLen);
//
//    // NetBT Parameters
//    HANDLE keyHandle;
//    PH_STRINGREF keyName;
//
//    PhInitializeStringRef(&keyName, L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters");
//
//    if (NT_SUCCESS(PhOpenKey(
//        &keyHandle,
//        KEY_QUERY_VALUE,
//        PH_KEY_LOCAL_MACHINE,
//        &keyName,
//        0
//        )))
//    {
//        ULONG nodeType = PhQueryRegistryUlongZ(keyHandle, L"NodeType");
//        if (nodeType == ULONG_MAX)
//            nodeType = PhQueryRegistryUlongZ(keyHandle, L"DhcpNodeType");
//        if (nodeType == ULONG_MAX)
//            nodeType = 8;
//
//        fixedInfo->NodeType = nodeType;
//
//        PPH_STRING scopeIdStr = PhQueryRegistryStringZ(keyHandle, L"ScopeId");
//        if (!scopeIdStr)
//            scopeIdStr = PhQueryRegistryStringZ(keyHandle, L"DhcpScopeId");
//
//        if (scopeIdStr)
//        {
//            PhConvertUtf16ToUtf8Buffer(
//                fixedInfo->ScopeId,
//                sizeof(fixedInfo->ScopeId),
//                NULL,
//                scopeIdStr->Buffer,
//                scopeIdStr->Length
//            );
//            PhDereferenceObject(scopeIdStr);
//        }
//        else
//        {
//            fixedInfo->ScopeId[0] = UNICODE_NULL;
//        }
//
//        ULONG enableProxy = PhQueryRegistryUlongZ(keyHandle, L"EnableProxy");
//        if (enableProxy != ULONG_MAX)
//            fixedInfo->EnableProxy = enableProxy;
//        else
//            fixedInfo->EnableProxy = 0;
//
//        ULONG enableDns = PhQueryRegistryUlongZ(keyHandle, L"EnableDns");
//        if (enableDns != ULONG_MAX)
//            fixedInfo->EnableDns = enableDns;
//        else
//            fixedInfo->EnableDns = 0;
//
//        NtClose(keyHandle);
//    }
//    else
//    {
//        fixedInfo->NodeType = 8;
//        fixedInfo->ScopeId[0] = UNICODE_NULL;
//        fixedInfo->EnableProxy = 0;
//        fixedInfo->EnableDns = 0;
//    }
//
//    MIB_IPSTATS stats;
//    if (PhGetIpStatisticsEx(&stats, AF_INET) == NO_ERROR)
//    {
//        fixedInfo->EnableRouting = (stats.dwForwarding == MIB_IP_FORWARDING);
//    }
//    else
//    {
//        fixedInfo->EnableRouting = 0;
//    }
//
//    *FixedInfo = fixedInfo;
//    return ERROR_SUCCESS;
//}

/**
 * Computes the size of the additional entries of an IP_ADDR_STRING list.
 *
 * \param AddressString The first list entry.
 * \return The size, in bytes, excluding the first entry.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_addr_string
 */
static NTSTATUS PhGetSizeofAddressStringList(
    _In_opt_ PIP_ADDR_STRING AddressString
    )
{
    ULONG size = 0;
    PIP_ADDR_STRING entry;

    if (!AddressString)
        return 0;

    for (entry = AddressString->Next; entry; entry = entry->Next)
    {
        size += sizeof(IP_ADDR_STRING);
    }

    return size;
}

/**
 * Frees the additional entries of an IP_ADDR_STRING list.
 *
 * \param AddressString The first list entry (not freed).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_addr_string
 */
static VOID PhFreeAddressStringList(
    _In_opt_ PIP_ADDR_STRING AddressString
    )
{
    PIP_ADDR_STRING entry = AddressString;

    if (!entry)
        return;

    entry = entry->Next;

    while (entry)
    {
        PIP_ADDR_STRING next = entry->Next;
        PhFree(entry);
        entry = next;
    }
}

/**
 * Retrieves the timestamp capabilities or the current timestamp configuration of a network interface.
 *
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param TimestampType NETIO_OID_TIMESTAMP_CAPABILITY or NETIO_OID_TIMESTAMP_CURRENT_CONFIG.
 * \param TimestampCapabilities Receives the timestamp capabilities.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getinterfacesupportedtimestampcapabilities
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getinterfaceactivetimestampcapabilities
 */
NTSTATUS PhGetInterfaceTimestampCapabilities(
    _In_ const NET_LUID* InterfaceLuid,
    _In_ ULONG TimestampType,
    _Out_ PVOID TimestampCapabilities
    )
{
    typedef ULONG (WINAPI *PGET_INTERFACE_TIMESTAMP_CAPABILITIES)(
        _In_ const NET_LUID* InterfaceLuid,
        _Out_ PINTERFACE_TIMESTAMP_CAPABILITIES TimestampCapabilities
        );
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PGET_INTERFACE_TIMESTAMP_CAPABILITIES getActiveCapabilities;
    static PGET_INTERFACE_TIMESTAMP_CAPABILITIES getSupportedCapabilities;
    PGET_INTERFACE_TIMESTAMP_CAPABILITIES getCapabilities;
    PVOID baseAddress;

    if (!InterfaceLuid || !TimestampCapabilities)
        return STATUS_INVALID_PARAMETER;

    if (PhBeginInitOnce(&initOnce))
    {
        if (baseAddress = PhLoadLibrary(L"iphlpapi.dll"))
        {
            getActiveCapabilities = PhGetDllBaseProcedureAddress(baseAddress, "GetInterfaceActiveTimestampCapabilities", 0);
            getSupportedCapabilities = PhGetDllBaseProcedureAddress(baseAddress, "GetInterfaceSupportedTimestampCapabilities", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    switch (TimestampType)
    {
    case NETIO_OID_TIMESTAMP_CAPABILITY:
        getCapabilities = getSupportedCapabilities;
        break;
    case NETIO_OID_TIMESTAMP_CURRENT_CONFIG:
        getCapabilities = getActiveCapabilities;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    if (!getCapabilities)
        return STATUS_NOT_SUPPORTED;

    return PhpNetioDosErrorToNtStatus(getCapabilities(InterfaceLuid, TimestampCapabilities));
}

/**
 * Converts a strsafe HRESULT into an NTSTATUS value.
 *
 * \param StringResult The HRESULT.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winerror/nf-winerror-hresult_code
 */
static NTSTATUS PhStringResultToStatus(
    _In_ ULONG StringResult
    )
{
    return HR_SUCCESS((HRESULT)StringResult) ? STATUS_SUCCESS : PhNtStatusFromHResult((HRESULT)StringResult);
}

/**
 * Builds a MIB_IPFORWARD_ROW2 from the IP route NSI key and parameter blocks.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Route Receives the route row.
 * \param Key The key that identifies the object instance.
 * \param Rw The read-write parameter block.
 * \param Rod The read-only dynamic parameter block.
 * \param Ros The read-only static parameter block.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/ns-netioapi-mib_ipforward_row2
 */
static VOID PhpFillRouteEntry(
    _In_ ADDRESS_FAMILY Family,
    _Out_ PMIB_IPFORWARD_ROW2 Route,
    _In_ const VOID *Key,
    _In_ const MIB_IP_ROUTE_RW_ROW *Rw,
    _In_ const VOID *Rod,
    _In_ const MIB_IP_ROUTE_STATIC_ROW *Ros
    )
{
    memset(Route, 0, sizeof(MIB_IPFORWARD_ROW2));
    Route->DestinationPrefix.Prefix.si_family = Family;
    Route->NextHop.si_family = Family;

    if (Family == AF_INET)
    {
        const MIB_IP_ROUTE_KEY_ROW_V4 *key = Key;
        const MIB_IP_ROUTE_DYNAMIC_ROW_V4 *dynamic = Rod;

        Route->InterfaceLuid = key->InterfaceLuid;
        Route->DestinationPrefix.Prefix.Ipv4.sin_family = AF_INET;
        Route->DestinationPrefix.Prefix.Ipv4.sin_addr = key->DestinationPrefix;
        Route->DestinationPrefix.PrefixLength = key->DestinationPrefixLength;
        Route->NextHop.Ipv4.sin_family = AF_INET;
        Route->NextHop.Ipv4.sin_addr = key->NextHop;
        Route->Age = dynamic->Age;
    }
    else
    {
        const MIB_IP_ROUTE_KEY_ROW_V6 *key = Key;
        const MIB_IP_ROUTE_DYNAMIC_ROW_V6 *dynamic = Rod;

        Route->InterfaceLuid = key->InterfaceLuid;
        Route->DestinationPrefix.Prefix.Ipv6.sin6_family = AF_INET6;
        Route->DestinationPrefix.Prefix.Ipv6.sin6_addr = key->DestinationPrefix;
        Route->DestinationPrefix.PrefixLength = key->DestinationPrefixLength;
        Route->NextHop.Ipv6.sin6_family = AF_INET6;
        Route->NextHop.Ipv6.sin6_addr = key->NextHop;
        Route->Age = dynamic->Age;
    }

    Route->InterfaceIndex = Ros->InterfaceIndex;
    Route->SitePrefixLength = Rw->SitePrefixLength;
    Route->ValidLifetime = Rw->ValidLifetime;
    Route->PreferredLifetime = Rw->PreferredLifetime;
    Route->Metric = Rw->Metric;
    Route->Protocol = Rw->Protocol;
    Route->Loopback = Rw->Loopback;
    Route->AutoconfigureAddress = Rw->AutoconfigureAddress;
    Route->Publish = Rw->Publish;
    Route->Immortal = Rw->Immortal;
    Route->Origin = Ros->Origin;
}

typedef struct _PHP_NSI_UNICAST_TABLE
{
    PVOID Keys;
    PMIB_IP_UNICAST_RW_ROW ReadWrite;
    PMIB_IP_UNICAST_DYNAMIC_ROW Dynamic;
    ULONG Count;
} PHP_NSI_UNICAST_TABLE, *PPHP_NSI_UNICAST_TABLE;

/**
 * Enumerates the unicast address table of the specified IP module.
 *
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param KeySize The size, in bytes, of each address key.
 * \param Table Receives the tables. Free them with PhpNsiFreeUnicastTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getunicastipaddresstable
 */
static NTSTATUS PhpNsiGetUnicastTable(
    _In_ const NPI_MODULEID* ModuleId,
    _In_ ULONG KeySize,
    _Out_ PPHP_NSI_UNICAST_TABLE Table
    )
{
    memset(Table, 0, sizeof(*Table));

    return PhNsiAllocateAndGetTable(
        NsiActive,
        ModuleId,
        NsiIpObjectUnicastAddress,
        &Table->Keys,
        KeySize,
        &Table->ReadWrite,
        sizeof(MIB_IP_UNICAST_RW_ROW),
        &Table->Dynamic,
        sizeof(MIB_IP_UNICAST_DYNAMIC_ROW),
        NULL,
        0,
        &Table->Count,
        0
        );
}

/**
 * Frees the tables returned by PhpNsiGetUnicastTable.
 *
 * \param Table The tables.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-freemibtable
 */
static VOID PhpNsiFreeUnicastTable(
    _Inout_ PPHP_NSI_UNICAST_TABLE Table
    )
{
    PhNsiFreeTable(Table->Keys, Table->ReadWrite, Table->Dynamic, NULL);
}

/**
 * Builds an IP_ADAPTER_ADDRESSES list from the NDIS and IP NSI providers.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Flags Flags that control which addresses are returned (GAA_FLAG_*).
 * \param Addresses Receives the allocated adapter address list.
 * \param AddressSize Receives the size, in bytes, of the adapter address list.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
 */
static NTSTATUS PhpNsiAllocateAndGetAdaptersAddresses(
    _In_ ULONG Family,
    _In_ ULONG Flags,
    _Outptr_result_maybenull_ PIP_ADAPTER_ADDRESSES *Addresses,
    _Out_opt_ PULONG AddressSize
    )
{
    NET_LUID* interfaceKeys = NULL;
    PNDIS_NSI_INTERFACE_ENUM_ROD dynamicRows = NULL;
    PNDIS_NSI_INTERFACE_ENUM_ROS staticRows = NULL;
    PHP_NSI_UNICAST_TABLE ipv4;
    PHP_NSI_UNICAST_TABLE ipv6;
    ULONG interfaceCount = 0;
    NTSTATUS result;
    SIZE_T requiredSize;
    PUCHAR cursor;
    PIP_ADAPTER_ADDRESSES previous = NULL;
    NET_IF_COMPARTMENT_ID compartmentId;

    *Addresses = NULL;
    if (AddressSize)
        *AddressSize = 0;

    memset(&ipv4, 0, sizeof(ipv4));
    memset(&ipv6, 0, sizeof(ipv6));

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceWithoutStatistics,
        &interfaceKeys,
        sizeof(NET_LUID),
        NULL,
        0,
        &dynamicRows,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROD),
        &staticRows,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROS),
        &interfaceCount,
        0
        );

    if (!NT_SUCCESS(result))
        return result;

    if (!(Flags & GAA_FLAG_SKIP_UNICAST))
    {
        if (Family == AF_UNSPEC || Family == AF_INET)
        {
            result = PhpNsiGetUnicastTable(&NPI_MS_IPV4_MODULEID, sizeof(MIB_IP_UNICAST_KEY_ROW_V4), &ipv4);
            if (!NT_SUCCESS(result))
                goto Cleanup;
        }

        if (Family == AF_UNSPEC || Family == AF_INET6)
        {
            result = PhpNsiGetUnicastTable(&NPI_MS_IPV6_MODULEID, sizeof(MIB_IP_UNICAST_KEY_ROW_V6), &ipv6);
            if (!NT_SUCCESS(result))
                goto Cleanup;
        }
    }

    // Each row and its strings are eight-byte aligned so all embedded pointers
    // can be relocated by PhGetAdaptersAddresses without a second allocation.
    requiredSize = (SIZE_T)interfaceCount *
        (ALIGN_UP_BY(sizeof(IP_ADAPTER_ADDRESSES), sizeof(ULONG64)) +
        ALIGN_UP_BY(RTL_GUID_STRING_SIZE + 1, sizeof(ULONG64)) +
        2 * ALIGN_UP_BY((IF_MAX_STRING_SIZE + 1) * sizeof(WCHAR), sizeof(ULONG64)));
    requiredSize += (SIZE_T)(ipv4.Count + ipv6.Count) *
        (ALIGN_UP_BY(sizeof(IP_ADAPTER_UNICAST_ADDRESS), sizeof(ULONG64)) +
        ALIGN_UP_BY(sizeof(SOCKADDR_INET), sizeof(ULONG64)));

    if (requiredSize > ULONG_MAX)
    {
        result = STATUS_INTEGER_OVERFLOW;
        goto Cleanup;
    }

    *Addresses = PhAllocateZero(requiredSize ? requiredSize : 1);
    cursor = (PUCHAR)*Addresses;
    compartmentId = PhGetCurrentThreadCompartmentId();

    for (ULONG i = 0; i < interfaceCount; i++)
    {
        PIP_ADAPTER_ADDRESSES adapter;
        WCHAR guidString[RTL_GUID_STRING_SIZE + 1];
        ULONG addressLength;
        PIP_ADAPTER_UNICAST_ADDRESS previousUnicast = NULL;

        if (!(Flags & GAA_FLAG_INCLUDE_ALL_COMPARTMENTS) && dynamicRows[i].CompartmentId != compartmentId)
            continue;

        adapter = (PIP_ADAPTER_ADDRESSES)cursor;
        cursor += ALIGN_UP_BY(sizeof(*adapter), sizeof(ULONG64));
        adapter->Length = sizeof(*adapter);
        adapter->IfIndex = staticRows[i].ifIndex;
        adapter->Ipv6IfIndex = staticRows[i].ifIndex;
        adapter->Luid = interfaceKeys[i];
        adapter->CompartmentId = dynamicRows[i].CompartmentId;
        adapter->NetworkGuid = dynamicRows[i].NetworkGuid;
        adapter->ConnectionType = staticRows[i].ConnectionType;
        adapter->IfType = staticRows[i].ifType;
        adapter->OperStatus = (IF_OPER_STATUS)dynamicRows[i].ifOperStatus;
        adapter->Mtu = dynamicRows[i].ifMtu;
        adapter->TransmitLinkSpeed = dynamicRows[i].XmitLinkSpeed;
        adapter->ReceiveLinkSpeed = dynamicRows[i].RcvLinkSpeed;
        adapter->PhysicalAddressLength = min(dynamicRows[i].ifPhysAddress.Length, sizeof(adapter->PhysicalAddress));
        memcpy(adapter->PhysicalAddress, dynamicRows[i].ifPhysAddress.Address, adapter->PhysicalAddressLength);

        adapter->AdapterName = (PCHAR)cursor;
        if (NT_SUCCESS(PhFormatGuidToBuffer(&staticRows[i].InterfaceGuid, guidString, sizeof(guidString), NULL)))
        {
            for (ULONG j = 0; j <= RTL_GUID_STRING_SIZE; j++)
                adapter->AdapterName[j] = (CHAR)guidString[j];
        }
        cursor += ALIGN_UP_BY(RTL_GUID_STRING_SIZE + 1, sizeof(ULONG64));

        adapter->FriendlyName = (PWCHAR)cursor;
        addressLength = min(dynamicRows[i].ifAlias.Length, IF_MAX_STRING_SIZE * sizeof(WCHAR));
        memcpy(adapter->FriendlyName, dynamicRows[i].ifAlias.String, addressLength);
        cursor += ALIGN_UP_BY((IF_MAX_STRING_SIZE + 1) * sizeof(WCHAR), sizeof(ULONG64));

        adapter->Description = (PWCHAR)cursor;
        addressLength = min(staticRows[i].ifDescr.Length, IF_MAX_STRING_SIZE * sizeof(WCHAR));
        memcpy(adapter->Description, staticRows[i].ifDescr.String, addressLength);
        cursor += ALIGN_UP_BY((IF_MAX_STRING_SIZE + 1) * sizeof(WCHAR), sizeof(ULONG64));

        for (ULONG familyIndex = 0; familyIndex < 2; familyIndex++)
        {
            PPHP_NSI_UNICAST_TABLE table = familyIndex ? &ipv6 : &ipv4;

            for (ULONG j = 0; j < table->Count; j++)
            {
                PIP_ADAPTER_UNICAST_ADDRESS unicast;
                PSOCKADDR_INET socketAddress;
                NET_LUID* luid = familyIndex ?
                    &((PMIB_IP_UNICAST_KEY_ROW_V6)table->Keys)[j].InterfaceLuid :
                    &((PMIB_IP_UNICAST_KEY_ROW_V4)table->Keys)[j].InterfaceLuid;

                if (luid->Value != interfaceKeys[i].Value)
                    continue;

                unicast = (PIP_ADAPTER_UNICAST_ADDRESS)cursor;
                cursor += ALIGN_UP_BY(sizeof(*unicast), sizeof(ULONG64));
                socketAddress = (PSOCKADDR_INET)cursor;
                cursor += ALIGN_UP_BY(sizeof(*socketAddress), sizeof(ULONG64));

                unicast->Length = sizeof(*unicast);
                unicast->Address.lpSockaddr = (PSOCKADDR)socketAddress;
                unicast->Address.iSockaddrLength = familyIndex ? sizeof(SOCKADDR_IN6) : sizeof(SOCKADDR_IN);
                unicast->PrefixOrigin = table->ReadWrite[j].PrefixOrigin;
                unicast->SuffixOrigin = table->ReadWrite[j].SuffixOrigin;
                unicast->DadState = table->Dynamic[j].DadState;
                unicast->ValidLifetime = table->ReadWrite[j].ValidLifetime;
                unicast->PreferredLifetime = table->ReadWrite[j].PreferredLifetime;
                unicast->OnLinkPrefixLength = table->ReadWrite[j].OnLinkPrefixLength;

                if (familyIndex)
                {
                    socketAddress->Ipv6.sin6_family = AF_INET6;
                    socketAddress->Ipv6.sin6_addr = ((PMIB_IP_UNICAST_KEY_ROW_V6)table->Keys)[j].Address;
                    socketAddress->Ipv6.sin6_scope_id = table->Dynamic[j].ScopeId.Value;
                    adapter->Ipv6Enabled = TRUE;
                }
                else
                {
                    socketAddress->Ipv4.sin_family = AF_INET;
                    socketAddress->Ipv4.sin_addr = ((PMIB_IP_UNICAST_KEY_ROW_V4)table->Keys)[j].Address;
                    adapter->Ipv4Enabled = TRUE;
                }

                if (previousUnicast)
                    previousUnicast->Next = unicast;
                else
                    adapter->FirstUnicastAddress = unicast;
                previousUnicast = unicast;
            }
        }

        if (previous)
            previous->Next = adapter;
        previous = adapter;
    }

    if (!previous)
    {
        PhFree(*Addresses);
        *Addresses = NULL;
        result = STATUS_NO_MORE_ENTRIES;
    }
    else
    {
        if (AddressSize)
            *AddressSize = (ULONG)(cursor - (PUCHAR)*Addresses);
        result = STATUS_SUCCESS;
    }

Cleanup:
    PhpNsiFreeUnicastTable(&ipv4);
    PhpNsiFreeUnicastTable(&ipv6);
    PhNsiFreeTable(interfaceKeys, NULL, dynamicRows, staticRows);
    return result;
}

/**
 * Allocates and retrieves the IP_ADAPTER_ADDRESSES list, growing the buffer as needed.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Flags Flags that control which addresses are returned (GAA_FLAG_*).
 * \param Addresses Receives the allocated adapter address list.
 * \param AddressSize Receives the size, in bytes, of the adapter address list.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
 */
static NTSTATUS PhpAllocateAndGetAdaptersAddresses(
    _In_ ULONG Family,
    _In_ ULONG Flags,
    _Outptr_result_maybenull_ PIP_ADAPTER_ADDRESSES *Addresses,
    _Out_opt_ PULONG AddressSize
    )
{
    typedef ULONG (WINAPI *PGET_ADAPTERS_ADDRESSES)(
        _In_ ULONG Family,
        _In_ ULONG Flags,
        _Reserved_ PVOID Reserved,
        _Out_writes_bytes_opt_(*SizePointer) PIP_ADAPTER_ADDRESSES AdapterAddresses,
        _Inout_ PULONG SizePointer
        );
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PGET_ADAPTERS_ADDRESSES getAdaptersAddresses;
    PVOID baseAddress;
    ULONG size;
    NTSTATUS result;

    if (!Addresses)
        return STATUS_INVALID_PARAMETER;

    *Addresses = NULL;
    if (AddressSize)
        *AddressSize = 0;

    if ((Family == AF_UNSPEC || Family == AF_INET || Family == AF_INET6) &&
        (Flags & (GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER)) ==
        (GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER) &&
        !(Flags & ~(GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
        GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_INCLUDE_ALL_COMPARTMENTS)))
    {
        return PhpNsiAllocateAndGetAdaptersAddresses(Family, Flags, Addresses, AddressSize);
    }

    if (PhBeginInitOnce(&initOnce))
    {
        if (baseAddress = PhLoadLibrary(L"iphlpapi.dll"))
            getAdaptersAddresses = PhGetDllBaseProcedureAddress(baseAddress, "GetAdaptersAddresses", 0);

        PhEndInitOnce(&initOnce);
    }

    if (!getAdaptersAddresses)
        return STATUS_PROCEDURE_NOT_FOUND;

    size = 0;
    result = PhpNetioDosErrorToNtStatus(getAdaptersAddresses(Family, Flags, NULL, NULL, &size));

    if (result != STATUS_BUFFER_TOO_SMALL)
        return result;

    if (!size)
        return STATUS_DATA_ERROR;

    *Addresses = PhAllocateZero(size);
    result = PhpNetioDosErrorToNtStatus(getAdaptersAddresses(Family, Flags, NULL, *Addresses, &size));

    if (!NT_SUCCESS(result))
    {
        PhFree(*Addresses);
        *Addresses = NULL;
    }
    else if (AddressSize)
    {
        *AddressSize = size;
    }

    return result;
}

/**
 * Frees an adapter address list.
 *
 * \param Addresses The adapter address list.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
 */
static VOID PhpFreeAdaptersAddresses(
    _Frees_ptr_opt_ PIP_ADAPTER_ADDRESSES Addresses
    )
{
    if (Addresses)
        PhFree(Addresses);
}

/**
 * Retrieves the unicast address table once it is stable, or registers to be notified when it is.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the table if it is already stable.
 * \param PreviousTable The previous table.
 * \param CallerCallback The callback invoked when the table becomes stable.
 * \param CallerContext A caller-defined context passed to the callback.
 * \param NotificationHandle Receives the notification handle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-notifystableunicastipaddresstable
 */
static NTSTATUS PhpNotifyStableUnicastIpAddressTable(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_result_maybenull_ PMIB_UNICASTIPADDRESS_TABLE *Table,
    _Inout_opt_ PMIB_UNICASTIPADDRESS_TABLE *PreviousTable,
    _In_opt_ PSTABLE_UNICAST_IPADDRESS_TABLE_CALLBACK CallerCallback,
    _In_opt_ PVOID CallerContext,
    _Out_ PHANDLE NotificationHandle
    )
{

    if (NotificationHandle)
        *NotificationHandle = NULL;

    if (Table)
        *Table = NULL;

    return PhGetUnicastIpAddressTable(Family, Table);
}

/**
 * Requests a notification when the IPv4 address table changes.
 *
 * \param Handle Receives the handle used to wait for the notification.
 * \param overlapped An optional OVERLAPPED structure for asynchronous notification.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-notifyaddrchange
 */
NTSTATUS PhNotifyAddrChange(
    _Out_opt_ PHANDLE Handle,
    _In_opt_ LPOVERLAPPED overlapped
    )
{
    return PhNsiRequestChangeNotification(0, &NPI_MS_IPV4_MODULEID, NsiIpObjectUnicastAddress, overlapped, Handle);
}

/**
 * Retrieves IPv4 TCP statistics.
 *
 * \param Statistics Receives the statistics.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcpstatistics
 */
NTSTATUS PhGetTcpStatistics(
    _Out_ PMIB_TCPSTATS Statistics
    )
{
    return PhGetTcpStatisticsEx(Statistics, 2u);
}

/**
 * Retrieves IPv4 UDP statistics.
 *
 * \param Stats Receives the statistics.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getudpstatistics
 */
NTSTATUS PhGetUdpStatistics(
    _Out_ PMIB_UDPSTATS Stats
    )
{
    return PhGetUdpStatisticsEx(Stats, 2u);
}

/**
 * Retrieves legacy information for a network interface.
 *
 * \param IfRow On input, dwIndex identifies the interface; on output, the interface row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getifentry
 */
NTSTATUS PhGetIfEntry(
    _Inout_ PMIB_IFROW IfRow
    )
{
    NTSTATUS result;
    IF_INDEX dwIndex;
    NET_LUID interfaceLuid;
    NDIS_NSI_INTERFACE_INFORMATION_ROD dynamicRow;
    NDIS_NSI_INTERFACE_INFORMATION_ROS staticRow;
    NDIS_NSI_INTERFACE_INFORMATION_RW readWriteRow;

    interfaceLuid.Value = 0;
    memset(&readWriteRow, 0, sizeof(NDIS_NSI_INTERFACE_INFORMATION_RW));
    memset(&dynamicRow, 0, sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROD));
    memset(&staticRow, 0, sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROS));

    if (!IfRow)
        return STATUS_INVALID_PARAMETER;

    dwIndex = IfRow->dwIndex;
    result = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceIndex,
        &dwIndex,
        sizeof(IF_INDEX),
        NsiParameterTypeReadOnlyStatic,
        &interfaceLuid,
        sizeof(NET_LUID),
        NsiNdisInterfaceIndexToLuid
    );

    if (!result)
    {
        result = PhNsiGetAllParameters(
            NsiActive,
            &NPI_MS_NDIS_MODULEID,
            NsiNdisObjectInterface,
            &interfaceLuid,
            sizeof(NET_LUID),
            &readWriteRow,
            sizeof(NDIS_NSI_INTERFACE_INFORMATION_RW),
            &dynamicRow,
            sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROD),
            &staticRow,
            sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROS)
            );
        
        if (!result)
        {
            PhpFillIfEntry(IfRow, &readWriteRow, &dynamicRow, &staticRow);
            return STATUS_SUCCESS;
        }
    }

    return result;
}

/**
 * Retrieves the network interface table.
 *
 * \param Table Receives the allocated table. Free it with PhFreeMibTable/FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getiftable2
 */
NTSTATUS PhGetIfTable2(
    _Outptr_ PMIB_IF_TABLE2 *Table
    )
{
    NTSTATUS result;
    ULONG count;
    ULONG outputCount;
    ULONG i;
    NET_IF_COMPARTMENT_ID compartmentId;
    PNET_LUID keys = NULL;
    PNDIS_NSI_INTERFACE_INFORMATION_RW rw = NULL;
    PNDIS_NSI_INTERFACE_INFORMATION_ROD rod = NULL;
    PNDIS_NSI_INTERFACE_INFORMATION_ROS ros = NULL;
    PMIB_IF_TABLE2 table;

    if (!Table)
        return STATUS_INVALID_PARAMETER;

    result = PhNsiAllocateAndGetTable(NsiActive, &NPI_MS_NDIS_MODULEID, NsiNdisObjectInterface,
        &keys, sizeof(NET_LUID), &rw, sizeof(NDIS_NSI_INTERFACE_INFORMATION_RW),
        &rod, sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROD),
        &ros, sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROS), &count, 0);
    if (result)
        return result;

    if (count > (SIZE_MAX - FIELD_OFFSET(MIB_IF_TABLE2, Table)) / sizeof(MIB_IF_ROW2))
    {
        PhNsiFreeTable(keys, rw, rod, ros);
        return STATUS_INTEGER_OVERFLOW;
    }

    compartmentId = PhGetCurrentThreadCompartmentId();
    table = PhAllocateZero(FIELD_OFFSET(MIB_IF_TABLE2, Table) + (SIZE_T)count * sizeof(MIB_IF_ROW2));
    outputCount = 0;

    for (i = 0; i < count; i++)
    {
        if (rod[i].CompartmentId != compartmentId)
            continue;

        PhpFillIfRow2(&table->Table[outputCount], &keys[i], &rw[i], &rod[i], &ros[i]);
        outputCount++;
    }

    table->NumEntries = outputCount;
    *Table = table;
    PhNsiFreeTable(keys, rw, rod, ros);
    return STATUS_SUCCESS;
}

/**
 * Retrieves the compartment, site and name of a network.
 *
 * \param NetworkGuid The network GUID.
 * \param CompartmentId Receives the compartment identifier.
 * \param SiteId Receives the site identifier.
 * \param NetworkName Receives the null-terminated network name.
 * \param Length The size of NetworkName, in characters.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getnetworkinformation
 */
NTSTATUS PhGetNetworkInformation(
    _In_ const NET_IF_NETWORK_GUID* NetworkGuid,
    _Out_ PNET_IF_COMPARTMENT_ID CompartmentId,
    _Out_ PULONG SiteId,
    _Out_writes_z_(Length) PWCHAR NetworkName,
    _In_ ULONG Length
    )
{
    NTSTATUS result;
    SIZE_T networkNameLength;
    NDIS_NSI_NETWORK_RW networkInformation;
    WCHAR pszSrc[MAXIMUM_FILENAME_LENGTH + 1];

    memset(&networkInformation, 0, sizeof(NDIS_NSI_NETWORK_RW));

    result = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectNetwork,
        (PVOID)NetworkGuid,
        sizeof(NET_IF_NETWORK_GUID),
        NsiParameterTypeReadWrite,
        &networkInformation,
        sizeof(NDIS_NSI_NETWORK_RW),
        0
        );

    if (!result)
    {
        networkNameLength = networkInformation.NetworkName.Length >> 1;

        if (networkNameLength > MAXIMUM_FILENAME_LENGTH)
            networkNameLength = MAXIMUM_FILENAME_LENGTH;
        
        pszSrc[networkNameLength] = UNICODE_NULL;
        memcpy(pszSrc, networkInformation.NetworkName.String, networkNameLength * sizeof(WCHAR));
        result = PhCopyStringZ(pszSrc, SIZE_MAX, NetworkName, Length, NULL);

        if (result >= 0)
        {
            *CompartmentId = networkInformation.CompartmentId;
            *SiteId = networkInformation.SiteId;
            return STATUS_SUCCESS;
        }

        return result;
    }

    return result;
}

/**
 * Retrieves the index of an adapter from its name ("\\DEVICE\\TCPIP_{GUID}").
 *
 * \param AdapterName The adapter name.
 * \param IfIndex Receives the interface index.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadapterindex
 */
NTSTATUS PhGetAdapterIndex(
    _In_z_ PCWSTR AdapterName,
    _Out_ PULONG IfIndex
    )
{
    GUID guid;
    UNICODE_STRING guidString;
    NET_LUID* keyTable = NULL;
    PNDIS_NSI_INTERFACE_ENUM_ROS publicTable = NULL;
    ULONG count = 0;
    NET_LUID matchLuid;
    NTSTATUS result;

    if (!AdapterName || !IfIndex)
        return STATUS_INVALID_PARAMETER;

    if (PhCountStringZ(AdapterName) <= TCPIP_ADAPTER_PREFIX_LENGTH)
        return STATUS_INVALID_PARAMETER;

    RtlInitUnicodeString(&guidString, AdapterName + TCPIP_ADAPTER_PREFIX_LENGTH);

    if (!NT_SUCCESS(RtlGUIDFromString(&guidString, &guid)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    matchLuid.Value = 0;

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceWithoutStatistics,
        &keyTable,
        sizeof(NET_LUID),
        NULL,
        0,
        NULL,
        0,
        &publicTable,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROS),
        &count,
        0
        );

    if (NT_SUCCESS(result))
    {
        for (ULONG i = 0; i < count; i++)
        {
            if (IsEqualGUID(&publicTable[i].InterfaceGuid, &guid))
            {
                matchLuid = keyTable[i];
                break;
            }
        }
        PhNsiFreeTable(keyTable, NULL, NULL, publicTable);
    }

    if (matchLuid.Value == 0)
    {
        return STATUS_NOT_FOUND;
    }

    result = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &matchLuid,
        sizeof(NET_LUID),
        NsiParameterTypeReadOnlyStatic,
        IfIndex,
        sizeof(ULONG),
        NsiNdisInterfaceLuidToIndex
        );

    if (!NT_SUCCESS(result))
        *IfIndex = 0;

    return result;
}

/**
 * Retrieves the IPv6 TCP connection table.
 *
 * \param TcpTable The buffer that receives the TCP table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcp6table
 */
NTSTATUS PhGetTcp6Table(
    _Out_writes_bytes_opt_(*SizePointer) PMIB_TCP6TABLE TcpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order
    )
{
    NTSTATUS result;
    MIB_TCPSTATS_LH statistics;
    PTCP_KEY_ROW keyTable = NULL;
    PNSI_TCP_STATUS_ROW dynamicTable = NULL;
    ULONG count = 0;
    ULONG errorCount = 0;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    memset(&statistics, 0, sizeof(MIB_TCPSTATS_LH));
    result = PhGetTcpStatisticsEx(&statistics, AF_INET6);

    if (!NT_SUCCESS(result))
        return result;

    result = PhpNetioMibCheckTableBufferSize(
        statistics.dwNumConns,
        sizeof(MIB_TCP6ROW),
        TcpTable,
        SizePointer
        );

    if (!NT_SUCCESS(result))
        return result;

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTableIdTcp,
        &keyTable,
        sizeof(TCP_KEY_ROW),
        NULL,
        0,
        &dynamicTable,
        sizeof(NSI_TCP_STATUS_ROW),
        NULL,
        0,
        &count,
        0
        );

    if (NT_SUCCESS(result))
    {
        TcpTable->dwNumEntries = 0;

        for (ULONG i = 0; i < count; i++)
        {
            const PTCP_KEY_ROW key = &keyTable[i];
            
            if (key->Family == AF_INET6 &&
                !IN6_IS_ADDR_V4MAPPED(&key->LocalAddress))
            {
                if (PhpNetioMibTableHasCapacity(TcpTable->dwNumEntries + 1, sizeof(MIB_TCP6ROW), *SizePointer))
                {
                    const ULONG idx = TcpTable->dwNumEntries;

                    TcpTable->table[idx].State = dynamicTable[i].State;
                    TcpTable->table[idx].LocalAddr = key->LocalAddress;
                    TcpTable->table[idx].dwLocalScopeId = key->LocalScopeId.Value;
                    TcpTable->table[idx].dwLocalPort = key->LocalPort;
                    TcpTable->table[idx].RemoteAddr = key->RemoteAddress;
                    TcpTable->table[idx].dwRemoteScopeId = key->RemoteScopeId.Value;
                    TcpTable->table[idx].dwRemotePort = key->RemotePort;
                    TcpTable->dwNumEntries++;
                }
                else
                {
                    errorCount++;
                }
            }
        }

        PhNsiFreeTable(keyTable, NULL, dynamicTable, NULL);

        ULONG requiredSize = 0;

        if (PhpNetioMibSizeofTable(
            errorCount + TcpTable->dwNumEntries,
            sizeof(MIB_TCP6ROW),
            errorCount != 0,
            &requiredSize
            ))
        {
            *SizePointer = requiredSize;

            if (errorCount)
            {
                return STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                if (Order && TcpTable->dwNumEntries > 0)
                {
                    qsort_s(TcpTable->table, TcpTable->dwNumEntries, sizeof(MIB_TCP6ROW), NULL, NULL);
                }
                return STATUS_SUCCESS;
            }
        }
        else
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    return result;
}

/**
 * Retrieves the IPv6 TCP connection table with owner and offload information.
 *
 * \param TcpTable The buffer that receives the TCP table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcp6table2
 */
NTSTATUS PhGetTcp6Table2(
    _Out_writes_bytes_opt_(*SizePointer) PMIB_TCP6TABLE2 TcpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order
    )
{
    NTSTATUS result;
    MIB_TCPSTATS_LH statistics;
    PTCP_KEY_ROW keyTable = NULL;
    PNSI_TCP_STATUS_ROW dynamicTable = NULL;
    PNSI_TCP_OWNER_ROW staticTable = NULL;
    ULONG count = 0;
    ULONG errorCount = 0;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    memset(&statistics, 0, sizeof(MIB_TCPSTATS_LH));
    result = PhGetTcpStatisticsEx(&statistics, AF_INET6);
    if (!NT_SUCCESS(result))
        return result;

    result = PhpNetioMibCheckTableBufferSize(statistics.dwNumConns, sizeof(MIB_TCP6ROW2), TcpTable, SizePointer);
    if (!NT_SUCCESS(result))
        return result;

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTableIdTcp,
        &keyTable,
        sizeof(TCP_KEY_ROW),
        NULL,
        0,
        &dynamicTable,
        sizeof(NSI_TCP_STATUS_ROW),
        &staticTable,
        sizeof(NSI_TCP_OWNER_ROW),
        &count,
        0
    );

    if (NT_SUCCESS(result))
    {
        TcpTable->dwNumEntries = 0;

        for (ULONG i = 0; i < count; i++)
        {
            const PTCP_KEY_ROW key = &keyTable[i];
            
            if (key->Family == AF_INET6 &&
                !IN6_IS_ADDR_V4MAPPED(&key->LocalAddress))
            {
                if (PhpNetioMibTableHasCapacity(
                    TcpTable->dwNumEntries + 1,
                    sizeof(MIB_TCP6ROW2),
                    *SizePointer
                    ))
                {
                    const ULONG idx = TcpTable->dwNumEntries;

                    TcpTable->table[idx].State = dynamicTable[i].State;
                    TcpTable->table[idx].LocalAddr = key->LocalAddress;
                    TcpTable->table[idx].dwLocalScopeId = key->LocalScopeId.Value;
                    TcpTable->table[idx].dwLocalPort = key->LocalPort;
                    TcpTable->table[idx].RemoteAddr = key->RemoteAddress;
                    TcpTable->table[idx].dwRemoteScopeId = key->RemoteScopeId.Value;
                    TcpTable->table[idx].dwRemotePort = key->RemotePort;
                    TcpTable->table[idx].dwOffloadState = dynamicTable[i].OffloadState;
                    TcpTable->table[idx].dwOwningPid = staticTable[i].OwningPid;
                    TcpTable->dwNumEntries++;
                }
                else
                {
                    errorCount++;
                }
            }
        }

        PhNsiFreeTable(keyTable, NULL, dynamicTable, staticTable);

        ULONG requiredSize = 0;

        if (PhpNetioMibSizeofTable(
            errorCount + TcpTable->dwNumEntries,
            sizeof(MIB_TCP6ROW2),
            errorCount != 0,
            &requiredSize
            ))
        {
            *SizePointer = requiredSize;

            if (errorCount)
            {
                return STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                if (Order && TcpTable->dwNumEntries > 0)
                {
                    qsort_s(TcpTable->table, TcpTable->dwNumEntries, sizeof(MIB_TCP6ROW2), NULL, NULL);
                }
                return STATUS_SUCCESS;
            }
        }
        else
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    return result;
}

/**
 * Retrieves the IPv6 UDP listener table.
 *
 * \param Udp6Table The buffer that receives the table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getudp6table
 */
NTSTATUS PhGetUdp6Table(
    _In_ PMIB_UDP6TABLE Udp6Table,
    _In_ PULONG SizePointer,
    _In_ BOOL Order
    )
{
    NTSTATUS result;
    MIB_UDPSTATS statistics;
    PUDP_KEY_ROW keyTable = NULL;
    ULONG count = 0;
    ULONG errorCount = 0;
    ULONG requiredSize = 0;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    memset(&statistics, 0, sizeof(MIB_UDPSTATS));
    result = PhGetUdpStatisticsEx(&statistics, AF_INET6);

    if (!NT_SUCCESS(result))
        return result;

    result = PhpNetioMibCheckTableBufferSize(
        statistics.dwNumAddrs,
        sizeof(MIB_UDP6ROW),
        Udp6Table,
        SizePointer
        );

    if (!NT_SUCCESS(result))
        return result;

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_UDP_MODULEID,
        NsiTableIdUdp,
        &keyTable,
        sizeof(UDP_KEY_ROW),
        NULL,
        0,
        NULL,
        0,
        NULL,
        0,
        &count,
        0
        );

    if (NT_SUCCESS(result))
    {
        Udp6Table->dwNumEntries = 0;

        for (ULONG i = 0; i < count; i++)
        {
            PUDP_KEY_ROW row = &keyTable[i];
            
            if (row->Family == AF_INET6 &&
                !IN6_IS_ADDR_V4MAPPED(&row->Address))
            {
                if (PhpNetioMibTableHasCapacity(Udp6Table->dwNumEntries + 1, sizeof(MIB_UDP6ROW), *SizePointer))
                {
                    ULONG idx = Udp6Table->dwNumEntries;
                    Udp6Table->table[idx].dwLocalAddr = row->Address;
                    Udp6Table->table[idx].dwLocalScopeId = row->ScopeId.Value;
                    Udp6Table->table[idx].dwLocalPort = row->Port;
                    Udp6Table->dwNumEntries++;
                }
                else
                {
                    errorCount++;
                }
            }
        }

        PhNsiFreeTable(keyTable, NULL, NULL, NULL);

        if (PhpNetioMibSizeofTable(
            errorCount + Udp6Table->dwNumEntries,
            sizeof(MIB_UDP6ROW),
            errorCount != 0,
            &requiredSize
            ))
        {
            *SizePointer = requiredSize;

            if (errorCount)
            {
                return STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                if (Order && Udp6Table->dwNumEntries > 0)
                {
                    qsort_s(Udp6Table->table, Udp6Table->dwNumEntries, sizeof(MIB_UDP6ROW), NULL, NULL);
                }
                return STATUS_SUCCESS;
            }
        }
        else
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    return result;
}

/**
 * Requests a notification when the IPv4 routing table changes.
 *
 * \param Handle Receives the handle used to wait for the notification.
 * \param overlapped An optional OVERLAPPED structure for asynchronous notification.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-notifyroutechange
 */
NTSTATUS PhNotifyRouteChange(
    _In_ PHANDLE Handle,
    _In_ LPOVERLAPPED overlapped
    )
{
    return PhNsiRequestChangeNotification(0, &NPI_MS_IPV4_MODULEID, NsiIpObjectBestRoute, overlapped, Handle);
}

typedef struct _PHP_ROUTE_CHANGE_KEY
{
    PVOID Key;
    ULONG KeyLength;
} PHP_ROUTE_CHANGE_KEY, *PPHP_ROUTE_CHANGE_KEY;

typedef struct _PHP_ROUTE_CHANGE_CONTEXT
{
    NETIO_CHANGE_CALLBACK_CONTEXT Callback;
    MIB_IPFORWARD_ROW2 Row;
} PHP_ROUTE_CHANGE_CONTEXT, *PPHP_ROUTE_CHANGE_CONTEXT;

typedef struct _PHP_ROUTE_CHANGE_PARAMETER
{
    NSI_PARAMETER_TYPE Type;
    PVOID Data;
    ULONG Length;
    ULONG Offset;
} PHP_ROUTE_CHANGE_PARAMETER, *PPHP_ROUTE_CHANGE_PARAMETER;

/**
 * NSI callback that converts a route change into a MIB_IPFORWARD_ROW2 and forwards it to the caller.
 *
 * \param Context The context.
 * \param ChangeKey The key of the object that changed.
 * \param ChangeParameter The changed parameters.
 * \param NotificationType The type of change.
 * \return ERROR_SUCCESS (NO_ERROR) on success, otherwise a Win32 error code.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-notifyroutechange2
 */
static ULONG NTAPI PhpRouteChangeCallback(
    _In_ PPHP_ROUTE_CHANGE_CONTEXT Context,
    _In_ PPHP_ROUTE_CHANGE_KEY ChangeKey,
    _In_opt_ PPHP_ROUTE_CHANGE_PARAMETER ChangeParameter,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    PMIB_IPFORWARD_ROW2 row = NULL;

    if (ChangeKey && ChangeKey->Key && ChangeKey->KeyLength)
    {
        row = &Context->Row;
        memset(row, 0, sizeof(MIB_IPFORWARD_ROW2));

        if (Context->Callback.Family == AF_INET && ChangeKey->KeyLength >= sizeof(MIB_IP_ROUTE_KEY_ROW_V4))
        {
            const MIB_IP_ROUTE_KEY_ROW_V4 *key = ChangeKey->Key;
            row->InterfaceLuid = key->InterfaceLuid;
            row->DestinationPrefix.Prefix.si_family = AF_INET;
            row->DestinationPrefix.Prefix.Ipv4.sin_addr = key->DestinationPrefix;
            row->DestinationPrefix.PrefixLength = key->DestinationPrefixLength;
            row->NextHop.si_family = AF_INET;
            row->NextHop.Ipv4.sin_addr = key->NextHop;
        }
        else if (Context->Callback.Family == AF_INET6 && ChangeKey->KeyLength >= sizeof(MIB_IP_ROUTE_KEY_ROW_V6))
        {
            const MIB_IP_ROUTE_KEY_ROW_V6 *key = ChangeKey->Key;
            row->InterfaceLuid = key->InterfaceLuid;
            row->DestinationPrefix.Prefix.si_family = AF_INET6;
            row->DestinationPrefix.Prefix.Ipv6.sin6_addr = key->DestinationPrefix;
            row->DestinationPrefix.PrefixLength = key->DestinationPrefixLength;
            row->NextHop.si_family = AF_INET6;
            row->NextHop.Ipv6.sin6_addr = key->NextHop;
        }
        else
        {
            row = NULL;
        }

        if (row)
        {
            PhConvertInterfaceLuidToIndex(&row->InterfaceLuid, &row->InterfaceIndex);

            // Notifications contain the route key and, for protocol changes, the new value.
            if (ChangeParameter &&
                ChangeParameter->Type == NsiParameterTypeReadWrite &&
                ChangeParameter->Data &&
                ChangeParameter->Length == sizeof(NL_ROUTE_PROTOCOL) &&
                ChangeParameter->Offset == FIELD_OFFSET(MIB_IP_ROUTE_RW_ROW, Protocol))
            {
                row->Protocol = *(NL_ROUTE_PROTOCOL *)ChangeParameter->Data;
            }
        }
    }

    ((PIPFORWARD_CHANGE_CALLBACK)Context->Callback.Callback)(
        Context->Callback.CallerContext, row, NotificationType);
    return ERROR_SUCCESS;
}

/**
 * Registers to be notified of changes to IP route entries.
 *
 * \param AddressFamily The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Callback The callback invoked on each change.
 * \param CallerContext A caller-defined context passed to the callback.
 * \param InitialNotification TRUE to invoke the callback immediately after registration.
 * \param NotificationHandle Receives the notification handle. Cancel it with PhCancelMibChangeNotify2.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-notifyroutechange2
 */
NTSTATUS PhNotifyRouteChange2(
    _In_ ADDRESS_FAMILY AddressFamily,
    _In_ PIPFORWARD_CHANGE_CALLBACK Callback,
    _In_opt_ PVOID CallerContext,
    _In_ BOOLEAN InitialNotification,
    _Out_ PHANDLE NotificationHandle
    )
{
    PNETIO_ROUTE_CHANGE_REGISTRATION registration;
    PVOID rpcHandle;
    NTSTATUS result;
    NTSTATUS ipv4Result = STATUS_NOT_SUPPORTED;

    if ((AddressFamily != AF_UNSPEC && AddressFamily != AF_INET && AddressFamily != AF_INET6) ||
        !Callback || !NotificationHandle)
        return STATUS_INVALID_PARAMETER;

    registration = PhAllocateZero(sizeof(NETIO_ROUTE_CHANGE_REGISTRATION));
    registration->Header.ObjectId = NsiIpObjectRoute;
    registration->Header.Ipv4CallbackContext = &registration->Ipv4Callback;
    registration->Header.Ipv6CallbackContext = &registration->Ipv6Callback;
    registration->Ipv4Callback.Family = AF_INET;
    registration->Ipv4Callback.Callback = Callback;
    registration->Ipv4Callback.CallerContext = CallerContext;
    registration->Ipv6Callback.Family = AF_INET6;
    registration->Ipv6Callback.Callback = Callback;
    registration->Ipv6Callback.CallerContext = CallerContext;

    if (!(rpcHandle = PhNsiNotificationConnectToServer()))
    {
        PhFree(registration);
        return STATUS_INVALID_HANDLE;
    }

    if (AddressFamily != AF_INET6)
    {
        ipv4Result = PhNsiRpcRegisterChangeNotification(rpcHandle, &NPI_MS_IPV4_MODULEID,
            NsiIpObjectRoute, PhpRouteChangeCallback, InitialNotification,
            &registration->Ipv4Callback, &registration->Header.Ipv4NotificationHandle);
    }

    result = ipv4Result;
    if (AddressFamily != AF_INET)
    {
        result = PhNsiRpcRegisterChangeNotification(rpcHandle, &NPI_MS_IPV6_MODULEID,
            NsiIpObjectRoute, PhpRouteChangeCallback, InitialNotification,
            &registration->Ipv6Callback, &registration->Header.Ipv6NotificationHandle);
        if (AddressFamily == AF_UNSPEC)
            result = (!NT_SUCCESS(ipv4Result) && !NT_SUCCESS(result)) ? ipv4Result : STATUS_SUCCESS;
    }

    if (result)
    {
        PhCancelMibChangeNotify2(registration);
        return result;
    }

    *NotificationHandle = registration;
    return STATUS_SUCCESS;
}

/**
 * Retrieves TCP statistics for IPv4 or IPv6.
 *
 * \param Statistics Receives the statistics.
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcpstatisticsex
 */
NTSTATUS PhGetTcpStatisticsEx(
    _Out_ PMIB_TCPSTATS Statistics,
    _In_ ULONG Family
    )
{
    NTSTATUS result;
    USHORT familyKey;
    NSI_TCP_GLOBAL_ROS readOnlyParams;
    NETIO_TCP_STATS_DYNAMIC dynamicParams;

    if (!Statistics)
        return STATUS_INVALID_PARAMETER;

    if (Family != AF_INET && Family != AF_INET6)
        return STATUS_NOT_SUPPORTED;

    familyKey = (USHORT)Family;
    memset(&dynamicParams, 0, sizeof(NETIO_TCP_STATS_DYNAMIC));
    memset(&readOnlyParams, 0, sizeof(NSI_TCP_GLOBAL_ROS));

    result = PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTcpObjectGlobal,
        &familyKey,
        sizeof(USHORT),
        NULL,
        0,
        &dynamicParams,
        sizeof(NETIO_TCP_STATS_DYNAMIC),
        &readOnlyParams,
        sizeof(NSI_TCP_GLOBAL_ROS)
        );

    if (NT_SUCCESS(result))
    {
        Statistics->dwRtoAlgorithm = readOnlyParams.RtoAlgorithm;
        Statistics->dwRtoMin = readOnlyParams.RtoMin;
        Statistics->dwRtoMax = readOnlyParams.RtoMax;
        Statistics->dwMaxConn = readOnlyParams.MaxConn;
        Statistics->dwActiveOpens = dynamicParams.ActiveOpens;
        Statistics->dwPassiveOpens = dynamicParams.PassiveOpens;
        Statistics->dwAttemptFails = dynamicParams.AttemptFails;
        Statistics->dwEstabResets = dynamicParams.EstabResets;
        Statistics->dwCurrEstab = dynamicParams.CurrEstab;
        Statistics->dwInSegs = (ULONG)dynamicParams.InSegs;
        Statistics->dwOutSegs = (ULONG)dynamicParams.OutSegs;
        Statistics->dwRetransSegs = dynamicParams.RetransSegs;
        Statistics->dwInErrs = dynamicParams.InErrs;
        Statistics->dwOutRsts = dynamicParams.OutRsts;
        Statistics->dwNumConns = dynamicParams.NumConns;
    }

    return result;
}

/**
 * Retrieves the IPv4 TCP connection table with owner and offload information.
 *
 * \param TcpTable The buffer that receives the TCP table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcptable2
 */
NTSTATUS PhGetTcpTable2(
    _Out_writes_bytes_opt_(*SizePointer) PMIB_TCPTABLE2 TcpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order
    )
{
    return PhpGetTcpTableInternal(TcpTable, SizePointer, Order, AF_INET, TCP_TABLE_OWNER_PID_ALL, 1);
}

/**
 * Retrieves the UDP port used by the Teredo client.
 *
 * \param Port Receives the port number.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getteredoport
 */
NTSTATUS PhGetTeredoPort(
    _Out_ PUSHORT Port
    )
{
    NTSTATUS result;
    USHORT portValue = 0;

    if (!Port)
        return STATUS_INVALID_PARAMETER;

    result = PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_FL6T_MODULEID,
        NsiFl6tObjectClientPort,
        NULL,
        0,
        &portValue,
        sizeof(USHORT),
        NULL,
        0,
        NULL,
        0
        );

    if (NT_SUCCESS(result))
        *Port = portValue;

    return result;
}

/**
 * Retrieves UDP statistics for IPv4 or IPv6.
 *
 * \param Statistics Receives the statistics.
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getudpstatisticsex
 */
NTSTATUS PhGetUdpStatisticsEx(
    _Out_ PMIB_UDPSTATS Statistics,
    _In_ ULONG Family
    )
{
    NTSTATUS result;
    USHORT familyKey;
    NETIO_UDP_STATS_DYNAMIC dynamicParams;

    if (!Statistics)
        return STATUS_INVALID_PARAMETER;

    if (Family != AF_INET && Family != AF_INET6)
        return STATUS_NOT_SUPPORTED;

    familyKey = (USHORT)Family;
    memset(&dynamicParams, 0, sizeof(NETIO_UDP_STATS_DYNAMIC));

    result = PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_UDP_MODULEID,
        NsiUdpObjectGlobal,
        &familyKey,
        sizeof(USHORT),
        NULL,
        0,
        &dynamicParams,
        sizeof(NETIO_UDP_STATS_DYNAMIC),
        NULL,
        0
        );

    if (NT_SUCCESS(result))
    {
        Statistics->dwInDatagrams = (ULONG)dynamicParams.InDatagrams;
        Statistics->dwNoPorts = dynamicParams.NoPorts;
        Statistics->dwInErrors = dynamicParams.InErrors;
        Statistics->dwOutDatagrams = (ULONG)dynamicParams.OutDatagrams;
        Statistics->dwNumAddrs = dynamicParams.NumAddrs;
    }

    return result;
}

/**
 * Retrieves the number of network interfaces.
 *
 * \param NumberOfInterfaces Receives the number of interfaces.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getnumberofinterfaces
 */
NTSTATUS PhGetNumberOfInterfaces(
    _Out_ PULONG NumberOfInterfaces
    )
{
    ULONG count = 0;
    NTSTATUS result;

    if (!NumberOfInterfaces)
        return STATUS_INVALID_PARAMETER;

    result = PhNsiEnumerateObjectsAllParameters(
        NsiActive,
        NsiGetFirst,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceWithoutStatistics,
        NULL,
        0,
        NULL,
        0,
        NULL,
        0,
        NULL,
        0,
        &count
        );

    *NumberOfInterfaces = count;

    if (result != STATUS_BUFFER_TOO_SMALL)
        return result;

    return STATUS_SUCCESS;
}

/**
 * Retrieves TCP statistics with 64-bit counters for IPv4 or IPv6.
 *
 * \param Statistics Receives the statistics.
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcpstatisticsex2
 */
NTSTATUS PhGetTcpStatisticsEx2(
    _In_ PNETIO_TCP_STATISTICS_EX2 Statistics,
    _In_ USHORT Family
    )
{
    NTSTATUS result;
    USHORT familyKey;
    NETIO_TCP_STATS_DYNAMIC dynamicParams;
    NSI_TCP_GLOBAL_ROS readOnlyParams;

    if (!Statistics)
        return STATUS_INVALID_PARAMETER;

    if (Family != AF_INET && Family != AF_INET6)
        return STATUS_NOT_SUPPORTED;

    familyKey = Family;
    memset(&dynamicParams, 0, sizeof(NETIO_TCP_STATS_DYNAMIC));
    memset(&readOnlyParams, 0, sizeof(NSI_TCP_GLOBAL_ROS));

    result = PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTcpObjectGlobal,
        &familyKey,
        sizeof(USHORT),
        NULL,
        0,
        &dynamicParams,
        sizeof(NETIO_TCP_STATS_DYNAMIC),
        &readOnlyParams,
        sizeof(NSI_TCP_GLOBAL_ROS)
        );

    if (NT_SUCCESS(result))
    {
        memcpy(&Statistics->Header, &readOnlyParams, sizeof(NETIO_RECOVERED_128));
        Statistics->ActiveOpens = dynamicParams.ActiveOpens;
        Statistics->PassiveOpens = dynamicParams.PassiveOpens;
        Statistics->AttemptFails = dynamicParams.AttemptFails;
        Statistics->EstabResets = dynamicParams.EstabResets;
        Statistics->CurrEstab = dynamicParams.CurrEstab;
        
        Statistics->InSegs = dynamicParams.InSegs;
        Statistics->OutSegs = dynamicParams.OutSegs;
        
        Statistics->RetransSegs = dynamicParams.RetransSegs;
        Statistics->OutRsts = dynamicParams.OutRsts;
        Statistics->InErrs = dynamicParams.InErrs;
        Statistics->NumConns = dynamicParams.NumConns;
    }

    return result;
}

/**
 * Retrieves UDP statistics with 64-bit counters for IPv4 or IPv6.
 *
 * \param Statistics Receives the statistics.
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getudpstatisticsex2
 */
NTSTATUS PhGetUdpStatisticsEx2(
    _In_ PNETIO_UDP_STATISTICS_EX2 Statistics,
    _In_ USHORT Family
    )
{
    NTSTATUS result;
    USHORT familyKey;
    NETIO_UDP_STATS_DYNAMIC dynamicParams;

    if (!Statistics)
        return STATUS_INVALID_PARAMETER;

    if (Family != AF_INET && Family != AF_INET6)
        return STATUS_NOT_SUPPORTED;

    familyKey = Family;
    memset(&dynamicParams, 0, sizeof(NETIO_UDP_STATS_DYNAMIC));

    result = PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_UDP_MODULEID,
        NsiUdpObjectGlobal,
        &familyKey,
        sizeof(USHORT),
        NULL,
        0,
        &dynamicParams,
        sizeof(NETIO_UDP_STATS_DYNAMIC),
        NULL,
        0
        );

    if (NT_SUCCESS(result))
    {
        memcpy(&Statistics->Header, &dynamicParams, sizeof(NETIO_RECOVERED_128));
        memcpy(&Statistics->Dynamic, (PUCHAR)&dynamicParams + sizeof(NETIO_RECOVERED_128), sizeof(NETIO_RECOVERED_128));
    }

    return result;
}

/**
 * Retrieves the legacy interface table for all compartments.
 *
 * \param IfTable The buffer that receives the table.
 * \param Size On input, the size of the buffer in bytes; on output, the required size.
 * \param Order TRUE to sort the returned table.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getiftable
 */
NTSTATUS PhGetIfTable(
    _In_ PMIB_IFTABLE IfTable,
    _In_ PULONG Size,
    _In_ BOOL Order
    )
{
    NTSTATUS result;
    ULONG entryCount;
    NET_IF_COMPARTMENT_ID currentCompartmentId;
    ULONG count;
    NET_IF_COMPARTMENT_ID i;
    NDIS_NSI_INTERFACE_INFORMATION_ROD* dynamicRow;
    NDIS_NSI_INTERFACE_INFORMATION_ROD* dynamicTable = NULL;
    NDIS_NSI_INTERFACE_INFORMATION_RW* readWriteTable = NULL;
    NET_LUID* keyTable = NULL;
    NDIS_NSI_INTERFACE_INFORMATION_ROS* staticTable = NULL;

    if (!Size)
        return STATUS_INVALID_PARAMETER;

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &keyTable,
        sizeof(NET_LUID),
        &readWriteTable,
        sizeof(NDIS_NSI_INTERFACE_INFORMATION_RW),
        &dynamicTable,
        sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROD),
        &staticTable,
        sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROS),
        &count,
        0
        );

    if (!result)
    {
        entryCount = PhpNetioMibCheckTableBufferSize(
            count,
            sizeof(MIB_IFROW),
            IfTable,
            Size
            );

        if (!entryCount)
        {
            currentCompartmentId = PhGetCurrentThreadCompartmentId();
            
            for (i = 0; i < count; ++i)
            {
                dynamicRow = &dynamicTable[i];

                if (dynamicRow->CompartmentId == currentCompartmentId)
                {
                    PhpFillIfEntry(
                        &IfTable->table[entryCount],
                        &readWriteTable[i],
                        dynamicRow,
                        &staticTable[i]
                        );
                    ++entryCount;
                }
            }
            
            IfTable->dwNumEntries = entryCount;

            if (Order)
            {
                qsort_s(IfTable->table, entryCount, sizeof(MIB_IFROW), NULL, NULL);
            }
            
            entryCount = (entryCount == 0) ? STATUS_NO_MORE_ENTRIES : 0;
        }

        PhNsiFreeTable(keyTable, readWriteTable, dynamicTable, staticTable);
        
        return entryCount;
    }

    return result;
}

/**
 * Retrieves the network interface table at the specified level.
 *
 * \param Level The level of interface information to retrieve.
 * \param Table Receives the allocated table. Free it with PhFreeMibTable/FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getiftable2ex
 */
NTSTATUS PhGetIfTable2Ex(
    _In_ MIB_IF_TABLE_LEVEL Level,
    _In_ PMIB_IF_TABLE2 *Table
    )
{
    if (Level == MibIfTableNormal)
        return PhGetIfTable2(Table);

    if (Level == MibIfTableRaw)
        return PhGetIfTable2(Table);

    if (Level == MibIfTableNormalWithoutStatistics)
        return PhGetIfTable2(Table);

    return STATUS_INVALID_PARAMETER;
}

/**
 * Retrieves a TCP table of the requested class for IPv4 or IPv6.
 *
 * \param TcpTable The buffer that receives the TCP table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param TableClass The type of table to retrieve.
 * \param Reserved Reserved; must be zero or NULL.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getextendedtcptable
 */
NTSTATUS PhGetExtendedTcpTable(
    _Out_writes_bytes_opt_(*SizePointer) PVOID TcpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order,
    _In_ ULONG Family,
    _In_ TCP_TABLE_CLASS TableClass,
    _In_ ULONG Reserved
    )
{
    if (Family == AF_INET)
    {
        ULONG ownerClass;

        switch (TableClass)
        {
        case TCP_TABLE_BASIC_LISTENER:
            ownerClass = PhpTcpOwnerBasic;
            break;
        case TCP_TABLE_BASIC_CONNECTIONS:
            ownerClass = PhpTcpOwnerBasic;
            break;
        case TCP_TABLE_BASIC_ALL:
            ownerClass = PhpTcpOwnerBasic;
            break;
        case TCP_TABLE_OWNER_PID_LISTENER:
            ownerClass = PhpTcpOwnerPid;
            break;
        case TCP_TABLE_OWNER_PID_CONNECTIONS:
            ownerClass = PhpTcpOwnerPid;
            break;
        case TCP_TABLE_OWNER_PID_ALL:
            ownerClass = PhpTcpOwnerPid;
            break;
        case TCP_TABLE_OWNER_MODULE_LISTENER:
            ownerClass = PhpTcpOwnerModule;
            break;
        case TCP_TABLE_OWNER_MODULE_CONNECTIONS:
            ownerClass = PhpTcpOwnerModule;
            break;
        case TCP_TABLE_OWNER_MODULE_ALL:
            ownerClass = PhpTcpOwnerModule;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }

        return PhpGetTcpTableInternal(TcpTable, SizePointer, Order, AF_INET, TableClass, ownerClass);
    }

    if (Family != AF_INET6)
        return STATUS_INVALID_PARAMETER;

    switch (TableClass)
    {
    case TCP_TABLE_BASIC_LISTENER:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhpTcpOwnerBasic);
    case TCP_TABLE_BASIC_CONNECTIONS:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhpTcpOwnerBasic);
    case TCP_TABLE_BASIC_ALL:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhpTcpOwnerBasic);
    case TCP_TABLE_OWNER_PID_LISTENER:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhpTcpOwnerPid);
    case TCP_TABLE_OWNER_PID_CONNECTIONS:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhpTcpOwnerPid);
    case TCP_TABLE_OWNER_PID_ALL:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhpTcpOwnerPid);
    case TCP_TABLE_OWNER_MODULE_LISTENER:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhpTcpOwnerModule);
    case TCP_TABLE_OWNER_MODULE_CONNECTIONS:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhpTcpOwnerModule);
    case TCP_TABLE_OWNER_MODULE_ALL:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhpTcpOwnerModule);
    }

    return STATUS_INVALID_PARAMETER;
}

/**
 * Retrieves a UDP table of the requested class for IPv4 or IPv6.
 *
 * \param UdpTable The buffer that receives the UDP table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param TableClass The type of table to retrieve.
 * \param Reserved Reserved; must be zero or NULL.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getextendedudptable
 */
NTSTATUS PhGetExtendedUdpTable(
    _Out_writes_bytes_opt_(*SizePointer) PVOID UdpTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order,
    _In_ ULONG Family,
    _In_ UDP_TABLE_CLASS TableClass,
    _In_ ULONG Reserved
    )
{
    if (Family == AF_INET)
    {
        return PhGetUdpTableInternal(UdpTable, SizePointer, Order, TableClass, Reserved);
    }

    if (Family != AF_INET6)
        return STATUS_INVALID_PARAMETER;

    switch (TableClass)
    {
    case UDP_TABLE_BASIC:
        return PhGetUdp6Table(UdpTable, SizePointer, Order);
    case UDP_TABLE_OWNER_PID:
        return PhGetUdp6TableInternal(UdpTable, SizePointer, Order, UDP_TABLE_OWNER_PID, Reserved);
    case UDP_TABLE_OWNER_MODULE:
        return PhGetUdp6TableInternal(UdpTable, SizePointer, Order, UDP_TABLE_OWNER_MODULE, Reserved);
    }

    return STATUS_INVALID_PARAMETER;
}

/**
 * Retrieves IP statistics for IPv4 or IPv6.
 *
 * \param Statistics Receives the statistics.
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getipstatisticsex
 */
NTSTATUS PhGetIpStatisticsEx(
    _Out_ PMIB_IPSTATS Statistics,
    _In_ ULONG Family
    )
{
    ULONG forwarding;
    const NPI_MODULEID *moduleId;
    NTSTATUS result;
    ULONG reassemblyTimeout;
    NET_IF_COMPARTMENT_ID compartmentKey;
    NSI_IP_GLOBAL_RW globalRw;
    NSI_IP_STATISTICS_ROD statistics;
    NSI_IP_COMPARTMENT_RW compartmentRw;
    NSI_IP_COMPARTMENT_ROD compartmentRod;

    memset(&globalRw, 0, sizeof(NSI_IP_GLOBAL_RW));
    memset(&statistics, 0, sizeof(NSI_IP_STATISTICS_ROD));
    memset(&compartmentRw, 0, sizeof(NSI_IP_COMPARTMENT_RW));
    memset(&compartmentRod, 0, sizeof(NSI_IP_COMPARTMENT_ROD));
    forwarding = 0;
    reassemblyTimeout = 0;

    if (!Statistics)
        return STATUS_INVALID_PARAMETER;

    if (Family == AF_INET)
        moduleId = &NPI_MS_IPV4_MODULEID;
    else if (Family == AF_INET6)
        moduleId = &NPI_MS_IPV6_MODULEID;
    else
        return STATUS_INVALID_PARAMETER;

    result = PhNsiGetAllParameters(
        NsiActive,
        moduleId,
        NsiIpObjectGlobal,
        NULL,
        0,
        &globalRw,
        sizeof(NSI_IP_GLOBAL_RW),
        &statistics,
        sizeof(NSI_IP_STATISTICS_ROD),
        &reassemblyTimeout,
        sizeof(ULONG)
        );

    if (!result)
    {
        compartmentKey = PhGetCurrentThreadCompartmentId();
        result = PhNsiGetAllParameters(
            NsiActive,
            moduleId,
            NsiIpObjectCompartment,
            &compartmentKey,
            sizeof(NET_IF_COMPARTMENT_ID),
            &compartmentRw,
            sizeof(NSI_IP_COMPARTMENT_RW),
            &compartmentRod,
            sizeof(NSI_IP_COMPARTMENT_ROD),
            NULL,
            0
            );

        if (!result)
        {
            forwarding = compartmentRw.Forwarding == 1;
            Statistics->dwInReceives = (ULONG)statistics.InReceives;
            Statistics->dwInHdrErrors = statistics.InHdrErrors;
            Statistics->dwInAddrErrors = statistics.InAddrErrors;
            Statistics->dwForwDatagrams = (ULONG)statistics.ForwDatagrams;
            Statistics->dwInUnknownProtos = statistics.InUnknownProtos;
            Statistics->dwInDiscards = statistics.InDiscards;
            Statistics->dwInDelivers = (ULONG)statistics.InDelivers;
            Statistics->dwOutRequests = (ULONG)statistics.OutRequests;
            Statistics->dwRoutingDiscards = statistics.RoutingDiscards;
            Statistics->dwOutDiscards = statistics.OutDiscards;
            Statistics->dwOutNoRoutes = statistics.OutNoRoutes;
            Statistics->dwReasmTimeout = reassemblyTimeout;
            Statistics->dwReasmReqds = statistics.ReasmReqds;
            Statistics->dwReasmOks = statistics.ReasmOks;
            Statistics->dwReasmFails = statistics.ReasmFails;
            Statistics->dwFragOks = statistics.FragOks;
            Statistics->dwFragFails = statistics.FragFails;
            Statistics->dwFragCreates = statistics.FragCreates;
            Statistics->dwForwarding = forwarding + 1;
            Statistics->dwDefaultTTL = compartmentRw.DefaultHopLimit;
            if (Statistics->dwDefaultTTL == ULONG_MAX)
                Statistics->dwDefaultTTL = globalRw.DefaultHopLimit;
            Statistics->dwNumIf = compartmentRod.NumIf;
            Statistics->dwNumAddr = compartmentRod.NumAddr;
            Statistics->dwNumRoutes = compartmentRod.NumRoutes;
            return STATUS_SUCCESS;
        }
    }

    return result;
}

/**
 * Builds a MIB_IPINTERFACE_ROW from the IP interface NSI parameter blocks.
 *
 * \param Row Receives the row.
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param Rw The read-write parameter block.
 * \param Rod The read-only dynamic parameter block.
 * \param Ros The read-only static parameter block.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/ns-netioapi-mib_ipinterface_row
 */
static VOID PhpFillIpInterfaceRow(
    _Out_ PMIB_IPINTERFACE_ROW Row,
    _In_ ADDRESS_FAMILY Family,
    _In_ const NET_LUID *InterfaceLuid,
    _In_ const NSI_IP_INTERFACE_RW *Rw,
    _In_ const NSI_IP_INTERFACE_ROD *Rod,
    _In_ const NSI_IP_INTERFACE_ROS *Ros
    )
{
    memset(Row, 0, sizeof(MIB_IPINTERFACE_ROW));
    Row->Family = Family;
    Row->InterfaceLuid = *InterfaceLuid;
    PhConvertInterfaceLuidToIndex(InterfaceLuid, &Row->InterfaceIndex);
    Row->MaxReassemblySize = 0xc8;
    Row->MaxRouterAdvertisementInterval = 0x258;
    Row->AdvertisingEnabled = Rw->AdvertisingEnabled;
    Row->ForwardingEnabled = Rw->ForwardingEnabled;
    Row->WeakHostSend = Rw->WeakHostSend;
    Row->WeakHostReceive = Rw->WeakHostReceive;
    Row->UseAutomaticMetric = Rw->UseAutomaticMetric;
    Row->UseNeighborUnreachabilityDetection = Rw->UseNeighborUnreachabilityDetection;
    Row->ManagedAddressConfigurationSupported = Rw->ManagedAddressConfigurationSupported;
    Row->OtherStatefulConfigurationSupported = Rw->OtherStatefulConfigurationSupported;
    Row->AdvertiseDefaultRoute = Rw->AdvertiseDefaultRoute;
    Row->RouterDiscoveryBehavior = Rw->RouterDiscoveryBehavior;
    Row->DadTransmits = Rw->DadTransmits;
    Row->BaseReachableTime = Rw->BaseReachableTime;
    Row->RetransmitTime = Rw->RetransmitTime;
    Row->PathMtuDiscoveryTimeout = Rw->PathMtuDiscoveryTimeout;
    Row->LinkLocalAddressBehavior = Rw->LinkLocalAddressBehavior;
    Row->LinkLocalAddressTimeout = Rw->LinkLocalAddressTimeout;
    memcpy(Row->ZoneIndices, Rw->ZoneIndices, sizeof(Row->ZoneIndices));
    Row->SitePrefixLength = Rw->SitePrefixLength;
    Row->Metric = Rw->Metric;
    Row->NlMtu = Rw->NlMtu;
    Row->Connected = !!Rod->Connected;
    Row->SupportsWakeUpPatterns = Rod->SupportsWakeUpPatterns;
    Row->SupportsNeighborDiscovery = Ros->SupportsNeighborDiscovery;
    Row->SupportsRouterDiscovery = Ros->SupportsRouterDiscovery;
    Row->ReachableTime = Rod->ReachableTime;
    memcpy(&Row->TransmitOffload, &Rod->TransmitOffload, sizeof(Row->TransmitOffload));
    memcpy(&Row->ReceiveOffload, &Rod->ReceiveOffload, sizeof(Row->ReceiveOffload));
    Row->DisableDefaultRoutes = Rw->DisableDefaultRoutes;
}

/**
 * Retrieves IP information for an interface.
 *
 * \param Row On input, the key fields of the row; on output, the complete row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getipinterfaceentry
 */
NTSTATUS PhGetIpInterfaceEntry(
    _Inout_ PMIB_IPINTERFACE_ROW Row
    )
{
    NTSTATUS result;
    PNPI_MODULEID moduleId;
    NET_LUID interfaceLuid;
    NSI_IP_INTERFACE_RW rw;
    NSI_IP_INTERFACE_ROD rod;
    NSI_IP_INTERFACE_ROS ros;

    if (!Row || (Row->Family != AF_INET && Row->Family != AF_INET6))
        return STATUS_INVALID_PARAMETER;

    moduleId = Row->Family == AF_INET ? (PNPI_MODULEID)&NPI_MS_IPV4_MODULEID : (PNPI_MODULEID)&NPI_MS_IPV6_MODULEID;
    interfaceLuid = Row->InterfaceLuid;

    if (!interfaceLuid.Value)
    {
        if (!Row->InterfaceIndex)
            return STATUS_INVALID_PARAMETER;

        result = PhConvertInterfaceIndexToLuid(Row->InterfaceIndex, &interfaceLuid);
        if (result)
            return result;
    }

    result = PhNsiGetAllParameters(
        NsiActive,
        moduleId,
        NsiIpObjectIpInterface,
        &interfaceLuid,
        sizeof(NET_LUID),
        &rw,
        sizeof(NSI_IP_INTERFACE_RW),
        &rod,
        sizeof(NSI_IP_INTERFACE_ROD),
        &ros,
        sizeof(NSI_IP_INTERFACE_ROS)
        );

    if (result)
        return result;

    if (rod.CompartmentId != PhGetCurrentThreadCompartmentId())
        return STATUS_NOT_FOUND;

    PhpFillIpInterfaceRow(Row, Row->Family, &interfaceLuid, &rw, &rod, &ros);
    return STATUS_SUCCESS;
}

/**
 * Retrieves the IP interface table.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the allocated table. Free it with PhFreeMibTable/FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getipinterfacetable
 */
NTSTATUS PhGetIpInterfaceTable(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_IPINTERFACE_TABLE *Table
    )
{
    struct
    {
        ADDRESS_FAMILY Family;
        PNPI_MODULEID ModuleId;
        PNET_LUID Keys;
        PNSI_IP_INTERFACE_RW Rw;
        PNSI_IP_INTERFACE_ROD Rod;
        PNSI_IP_INTERFACE_ROS Ros;
        ULONG Count;
    } families[2] = {
        { AF_INET, (PNPI_MODULEID)&NPI_MS_IPV4_MODULEID },
        { AF_INET6, (PNPI_MODULEID)&NPI_MS_IPV6_MODULEID }
    };
    NTSTATUS result;
    ULONG count;
    ULONG i;
    ULONG j;
    ULONG entryIndex;
    NET_IF_COMPARTMENT_ID compartmentId;
    PMIB_IPINTERFACE_TABLE table;

    if (!Table || (Family != AF_UNSPEC && Family != AF_INET && Family != AF_INET6))
        return STATUS_INVALID_PARAMETER;

    compartmentId = PhGetCurrentThreadCompartmentId();
    count = 0;

    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (Family != AF_UNSPEC && Family != families[i].Family)
            continue;

        result = PhNsiAllocateAndGetTable(
            NsiActive,
            families[i].ModuleId,
            NsiIpObjectIpInterface,
            &families[i].Keys,
            sizeof(NET_LUID),
            &families[i].Rw,
            sizeof(NSI_IP_INTERFACE_RW),
            &families[i].Rod,
            sizeof(NSI_IP_INTERFACE_ROD),
            &families[i].Ros,
            sizeof(NSI_IP_INTERFACE_ROS),
            &families[i].Count,
            0
            );

        if (result == STATUS_NOT_FOUND || result == STATUS_NOT_SUPPORTED)
            continue;

        if (result)
            goto Cleanup;

        for (j = 0; j < families[i].Count; j++)
        {
            if (families[i].Rod[j].CompartmentId == compartmentId)
                count++;
        }
    }

    if (count > (SIZE_MAX - FIELD_OFFSET(MIB_IPINTERFACE_TABLE, Table)) / sizeof(MIB_IPINTERFACE_ROW))
    {
        result = STATUS_INTEGER_OVERFLOW;
        goto Cleanup;
    }

    table = PhAllocateZero(FIELD_OFFSET(MIB_IPINTERFACE_TABLE, Table) + (SIZE_T)count * sizeof(MIB_IPINTERFACE_ROW));
    entryIndex = 0;

    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        for (j = 0; j < families[i].Count; j++)
        {
            if (families[i].Rod[j].CompartmentId != compartmentId)
                continue;

            PhpFillIpInterfaceRow(&table->Table[entryIndex], families[i].Family,
                &families[i].Keys[j], &families[i].Rw[j], &families[i].Rod[j], &families[i].Ros[j]);
            entryIndex++;
        }
    }

    table->NumEntries = entryIndex;
    *Table = table;
    result = STATUS_SUCCESS;

Cleanup:
    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (families[i].Keys || families[i].Rw || families[i].Rod || families[i].Ros)
            PhNsiFreeTable(families[i].Keys, families[i].Rw, families[i].Rod, families[i].Ros);
    }

    return result;
}

/**
 * Builds a MIB_IPNET_ROW2 from the IP neighbor NSI parameter blocks.
 *
 * \param Row Receives the row.
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Key The neighbor key.
 * \param Rw The neighbor read-write block.
 * \param Rod The neighbor read-only dynamic block.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/ns-netioapi-mib_ipnet_row2
 */
static VOID PhpFillIpNetRow2(
    _Out_ PMIB_IPNET_ROW2 Row,
    _In_ ADDRESS_FAMILY Family,
    _In_reads_bytes_(sizeof(MIB_IP_NEIGHBOR_KEY_ROW_V4)) const UCHAR *Key,
    _In_reads_bytes_(sizeof(MIB_IP_NEIGHBOR_RW_ROW)) const UCHAR *Rw,
    _In_reads_bytes_(sizeof(MIB_IP_NEIGHBOR_DYNAMIC_ROW)) const UCHAR *Rod
    )
{
    ULONG physicalAddressLength;

    memset(Row, 0, sizeof(MIB_IPNET_ROW2));

    // Neighbor key layout: NET_LUID at offset 0; IPv4 address at offset 16
    // (mirrors PhGetIpNetTable). The IPv6 address follows the LUID at offset 8.
    Row->InterfaceLuid = *(const NET_LUID *)PTR_ADD_OFFSET(Key, FIELD_OFFSET(MIB_IP_NEIGHBOR_KEY_ROW_V4, InterfaceLuid));
    PhConvertInterfaceLuidToIndex(&Row->InterfaceLuid, &Row->InterfaceIndex);

    if (Family == AF_INET)
    {
        Row->Address.si_family = AF_INET;
        Row->Address.Ipv4.sin_family = AF_INET;
        Row->Address.Ipv4.sin_addr = *(const IN_ADDR *)PTR_ADD_OFFSET(Key, FIELD_OFFSET(MIB_IP_NEIGHBOR_KEY_ROW_V4, Address));
    }
    else
    {
        Row->Address.si_family = AF_INET6;
        Row->Address.Ipv6.sin6_family = AF_INET6;
        Row->Address.Ipv6.sin6_addr = *(const IN6_ADDR *)PTR_ADD_OFFSET(Key, FIELD_OFFSET(MIB_IP_NEIGHBOR_KEY_ROW_V6, Address));
    }

    // ROD layout: neighbor state at offset 0; physical-address length at offset 10.
    physicalAddressLength = *(const UCHAR *)PTR_ADD_OFFSET(Rod, FIELD_OFFSET(MIB_IP_NEIGHBOR_DYNAMIC_ROW, PhysicalAddressLength));
    if (physicalAddressLength > IF_MAX_PHYS_ADDRESS_LENGTH)
        physicalAddressLength = IF_MAX_PHYS_ADDRESS_LENGTH;
    Row->PhysicalAddressLength = physicalAddressLength;
    memcpy(Row->PhysicalAddress, Rw, physicalAddressLength);

    Row->State = (NL_NEIGHBOR_STATE)(*(const ULONG *)PTR_ADD_OFFSET(Rod, FIELD_OFFSET(MIB_IP_NEIGHBOR_DYNAMIC_ROW, State)));
}

/**
 * Retrieves the IP neighbor table.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the allocated table. Free it with PhFreeMibTable/FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getipnettable2
 */
NTSTATUS PhGetIpNetTable2(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_IPNET_TABLE2 *Table
    )
{
    // Reimplemented over the NSI IP neighbor object (NsiIpObjectNeighbor),
    // mirroring the IPv4 decode in PhGetIpNetTable and extending it to MIB_IPNET_ROW2.
    // The IPv6 neighbor key address offset should be validated against tools/tests/tcpip.
    typedef struct _PH_NETIO_IPNET_FAMILY
    {
        const NPI_MODULEID *ModuleId;
        ADDRESS_FAMILY Family;
        PVOID KeyTable;
        PVOID RwTable;
        PVOID RodTable;
        ULONG Count;
    } PH_NETIO_IPNET_FAMILY;
    PH_NETIO_IPNET_FAMILY families[PH_NETIO_IPNET_FAMILY_COUNT];
    ULONG familyCount = 0;
    NET_IF_COMPARTMENT_ID compartment;
    ULONG total = 0;
    ULONG i;
    ULONG j;
    ULONG entryIndex;
    PMIB_IPNET_TABLE2 table;

    if (!Table)
        return STATUS_INVALID_PARAMETER;

    *Table = NULL;

    if (Family != AF_INET && Family != AF_INET6 && Family != AF_UNSPEC)
        return STATUS_INVALID_PARAMETER;

    memset(families, 0, sizeof(PH_NETIO_IPNET_FAMILY) * RTL_NUMBER_OF(families));

    if (Family == AF_INET || Family == AF_UNSPEC)
    {
        families[familyCount].ModuleId = &NPI_MS_IPV4_MODULEID;
        families[familyCount].Family = AF_INET;
        familyCount++;
    }
    if (Family == AF_INET6 || Family == AF_UNSPEC)
    {
        families[familyCount].ModuleId = &NPI_MS_IPV6_MODULEID;
        families[familyCount].Family = AF_INET6;
        familyCount++;
    }

    compartment = PhGetCurrentThreadCompartmentId();

    for (i = 0; i < familyCount; i++)
    {
        NTSTATUS result = PhNsiAllocateAndGetTable(
            NsiActive,
            families[i].ModuleId,
            NsiIpObjectNeighbor,
            &families[i].KeyTable,
            sizeof(MIB_IP_NEIGHBOR_KEY_ROW_V4),
            &families[i].RwTable,
            sizeof(MIB_IP_NEIGHBOR_RW_ROW),
            &families[i].RodTable,
            sizeof(MIB_IP_NEIGHBOR_DYNAMIC_ROW),
            NULL,
            0,
            &families[i].Count,
            0
            );

        if (result != STATUS_SUCCESS)
        {
            families[i].Count = 0;
            families[i].KeyTable = NULL;
            families[i].RwTable = NULL;
            families[i].RodTable = NULL;
        }

        total += families[i].Count;
    }

    table = PhAllocateZero(FIELD_OFFSET(MIB_IPNET_TABLE2, Table) + (SIZE_T)total * sizeof(MIB_IPNET_ROW2));

    entryIndex = 0;
    for (i = 0; i < familyCount; i++)
    {
        for (j = 0; j < families[i].Count; j++)
        {
            const UCHAR *key = PTR_ADD_OFFSET(families[i].KeyTable, sizeof(MIB_IP_NEIGHBOR_KEY_ROW_V4) * j);
            const UCHAR *rw = PTR_ADD_OFFSET(families[i].RwTable, sizeof(MIB_IP_NEIGHBOR_RW_ROW) * j);
            const UCHAR *rod = PTR_ADD_OFFSET(families[i].RodTable, sizeof(MIB_IP_NEIGHBOR_DYNAMIC_ROW) * j);

            // Offset 12 of the ROD element is the compartment ID.
            if (*(const ULONG *)PTR_ADD_OFFSET(rod, FIELD_OFFSET(MIB_IP_NEIGHBOR_DYNAMIC_ROW, CompartmentId)) != compartment)
                continue;

            PhpFillIpNetRow2(&table->Table[entryIndex], families[i].Family, key, rw, rod);
            entryIndex++;
        }

        if (families[i].KeyTable || families[i].RwTable || families[i].RodTable)
            PhNsiFreeTable(families[i].KeyTable, families[i].RwTable, families[i].RodTable, NULL);
    }

    table->NumEntries = entryIndex;
    *Table = table;
    return STATUS_SUCCESS;
}

/**
 * Retrieves the bandwidth estimates of the network connection on an interface.
 *
 * \param InterfaceIndex The local index of the network interface.
 * \param AddressFamily The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param BandwidthEstimates Receives the bandwidth estimates.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getipnetworkconnectionbandwidthestimates
 */
NTSTATUS PhGetIpNetworkConnectionBandwidthEstimates(
    _In_ NET_IFINDEX InterfaceIndex,
    _In_ ADDRESS_FAMILY AddressFamily,
    _Out_ PMIB_IP_NETWORK_CONNECTION_BANDWIDTH_ESTIMATES BandwidthEstimates
    )
{
    NTSTATUS status;
    PNPI_MODULEID moduleId;
    NET_IFINDEX interfaceIndex;
    NET_LUID interfaceLuid;
    NSI_IP_INTERFACE_ROD bandwidthRow;

    if (AddressFamily == AF_INET)
        moduleId = (PNPI_MODULEID)&NPI_MS_IPV4_MODULEID;
    else if (AddressFamily == AF_INET6)
        moduleId = (PNPI_MODULEID)&NPI_MS_IPV6_MODULEID;
    else
        return STATUS_INVALID_PARAMETER;

    interfaceIndex = InterfaceIndex;
    memset(&interfaceLuid, 0, sizeof(NET_LUID));
    memset(&bandwidthRow, 0, sizeof(NSI_IP_INTERFACE_ROD));

    status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceIndex,
        &interfaceIndex,
        sizeof(NET_IFINDEX),
        NsiParameterTypeReadOnlyStatic,
        &interfaceLuid,
        sizeof(NET_LUID),
        NsiNdisInterfaceIndexToLuid
        );

    if (status)
        return status;

    status = PhNsiGetAllParameters(
        NsiActive,
        moduleId,
        NsiIpObjectIpInterface,
        &interfaceLuid,
        sizeof(NET_LUID),
        NULL,
        0,
        &bandwidthRow,
        sizeof(NSI_IP_INTERFACE_ROD),
        NULL,
        0
        );

    if (status)
        return status;

    memset(BandwidthEstimates, 0, sizeof(MIB_IP_NETWORK_CONNECTION_BANDWIDTH_ESTIMATES));
    BandwidthEstimates->InboundBandwidthInformation = bandwidthRow.InboundBandwidthInformation;
    BandwidthEstimates->OutboundBandwidthInformation = bandwidthRow.OutboundBandwidthInformation;

    return STATUS_SUCCESS;
}

/**
 * Retrieves the IP route table.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the allocated table. Free it with PhFreeMibTable/FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getipforwardtable2
 */
NTSTATUS PhGetIpForwardTable2(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_IPFORWARD_TABLE2 *Table
    )
{
    struct
    {
        ADDRESS_FAMILY Family;
        PNPI_MODULEID ModuleId;
        ULONG KeySize;
        ULONG RodSize;
        PVOID Keys;
        PMIB_IP_ROUTE_RW_ROW Rw;
        PVOID Rod;
        PMIB_IP_ROUTE_STATIC_ROW Ros;
        ULONG Count;
    } families[2] = {
        { AF_INET, (PNPI_MODULEID)&NPI_MS_IPV4_MODULEID, sizeof(MIB_IP_ROUTE_KEY_ROW_V4), sizeof(MIB_IP_ROUTE_DYNAMIC_ROW_V4) },
        { AF_INET6, (PNPI_MODULEID)&NPI_MS_IPV6_MODULEID, sizeof(MIB_IP_ROUTE_KEY_ROW_V6), sizeof(MIB_IP_ROUTE_DYNAMIC_ROW_V6) }
    };
    NTSTATUS result;
    ULONG count;
    ULONG i;
    ULONG j;
    ULONG entryIndex;
    NET_IF_COMPARTMENT_ID compartmentId;
    PMIB_IPFORWARD_TABLE2 table;

    if (!Table || (Family != AF_UNSPEC && Family != AF_INET && Family != AF_INET6))
        return STATUS_INVALID_PARAMETER;

    compartmentId = PhGetCurrentThreadCompartmentId();
    count = 0;

    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (Family != AF_UNSPEC && Family != families[i].Family)
            continue;

        result = PhNsiAllocateAndGetTable(
            NsiActive,
            families[i].ModuleId,
            NsiIpObjectRoute,
            &families[i].Keys,
            families[i].KeySize,
            &families[i].Rw,
            sizeof(MIB_IP_ROUTE_RW_ROW),
            &families[i].Rod,
            families[i].RodSize,
            &families[i].Ros,
            sizeof(MIB_IP_ROUTE_STATIC_ROW),
            &families[i].Count,
            0
            );

        if (result == STATUS_NOT_FOUND || result == STATUS_NOT_SUPPORTED)
            continue;

        if (result)
            goto Cleanup;

        for (j = 0; j < families[i].Count; j++)
        {
            const ULONG *key = PTR_ADD_OFFSET(families[i].Keys, (SIZE_T)j * families[i].KeySize);
            if (*key == compartmentId)
                count++;
        }
    }

    if (count > (SIZE_MAX - FIELD_OFFSET(MIB_IPFORWARD_TABLE2, Table)) / sizeof(MIB_IPFORWARD_ROW2))
    {
        result = STATUS_INTEGER_OVERFLOW;
        goto Cleanup;
    }

    table = PhAllocateZero(FIELD_OFFSET(MIB_IPFORWARD_TABLE2, Table) + (SIZE_T)count * sizeof(MIB_IPFORWARD_ROW2));
    entryIndex = 0;

    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        for (j = 0; j < families[i].Count; j++)
        {
            const VOID *key = PTR_ADD_OFFSET(families[i].Keys, (SIZE_T)j * families[i].KeySize);
            const VOID *rod = PTR_ADD_OFFSET(families[i].Rod, (SIZE_T)j * families[i].RodSize);

            if (*(const ULONG *)key != compartmentId)
                continue;

            PhpFillRouteEntry(families[i].Family, &table->Table[entryIndex], key,
                &families[i].Rw[j], rod, &families[i].Ros[j]);
            entryIndex++;
        }
    }

    table->NumEntries = entryIndex;
    *Table = table;
    result = STATUS_SUCCESS;

Cleanup:
    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (families[i].Keys || families[i].Rw || families[i].Rod || families[i].Ros)
            PhNsiFreeTable(families[i].Keys, families[i].Rw, families[i].Rod, families[i].Ros);
    }

    return result;
}

/**
 * Converts an interface alias to an interface GUID (iphlpapi!NhGetGuidFromInterfaceName).
 *
 * \param InterfaceName The interface name.
 * \param InterfaceGuid Receives the interface GUID.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfacealiastoluid
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoguid
 */
NTSTATUS PhNhGetGuidFromInterfaceName(
    _In_ PWSTR InterfaceName,
    _Out_ GUID *InterfaceGuid
    )
{
    NTSTATUS result;
    NET_LUID InterfaceLuid;

    InterfaceLuid.Value = 0;
    result = PhConvertInterfaceAliasToLuid(InterfaceName, &InterfaceLuid);
    if (!result)
        return PhConvertInterfaceLuidToGuid(&InterfaceLuid, InterfaceGuid);

    return result;
}

/**
 * Converts an interface GUID to its alias (iphlpapi!NhGetInterfaceNameFromGuid).
 *
 * \param InterfaceGuid The GUID of the network interface.
 * \param InterfaceName Receives the null-terminated alias.
 * \param InterfaceNameLength On input, the size of InterfaceName in bytes; on output, the required size.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceguidtoluid
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoalias
 */
NTSTATUS PhNhGetInterfaceNameFromGuid(
    _In_ const GUID *InterfaceGuid,
    _Out_writes_bytes_(*InterfaceNameLength) WCHAR *InterfaceName,
    _Inout_ PULONG InterfaceNameLength
    )
{
    NTSTATUS result;
    NET_LUID InterfaceLuid;

    InterfaceLuid.Value = 0;
    result = PhConvertInterfaceGuidToLuid(InterfaceGuid, &InterfaceLuid);
    if (!result)
    {
        result = PhConvertInterfaceLuidToAlias(&InterfaceLuid, InterfaceName, *InterfaceNameLength >> 1);
        if (result == STATUS_NO_MEMORY)
            *InterfaceNameLength = (IF_MAX_STRING_SIZE + 1) * sizeof(WCHAR);
    }

    return result;
}

/**
 * Retrieves the parameter blocks of an IPv4 TCP connection from the TCP NSI provider.
 *
 * \param TcpRow The TCP row that identifies the connection.
 * \param ObjectId The NSI TCP object identifier.
 * \param DynamicBuffer Receives the read-only dynamic block.
 * \param DynamicBufferSize The size, in bytes, of DynamicBuffer.
 * \param StaticBuffer Receives the read-only static block.
 * \param StaticBufferSize The size, in bytes, of StaticBuffer.
 * \param ReadWriteBuffer Receives the read-write block.
 * \param ReadWriteBufferSize The size, in bytes, of ReadWriteBuffer.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getpertcpconnectionestats
 */
NTSTATUS PhGetPerTcpConnectionStats(
    _In_ PMIB_TCPROW TcpRow,
    _In_ ULONG ObjectId,
    _Out_writes_bytes_opt_(DynamicBufferSize) PUCHAR DynamicBuffer,
    _In_ ULONG DynamicBufferSize,
    _Out_writes_bytes_opt_(StaticBufferSize) PUCHAR StaticBuffer,
    _In_ ULONG StaticBufferSize,
    _Out_writes_bytes_opt_(ReadWriteBufferSize) PUCHAR ReadWriteBuffer,
    _In_ ULONG ReadWriteBufferSize)
{
    NSI_TCP_KEY_ROW key;

    memset(&key, 0, sizeof(NSI_TCP_KEY_ROW));
    key.LocalAddress.Ipv4.sin_family = AF_INET;
    key.LocalAddress.Ipv4.sin_addr.S_un.S_addr = TcpRow->dwLocalAddr;
    key.LocalAddress.Ipv4.sin_port = (USHORT)TcpRow->dwLocalPort;
    key.RemoteAddress.Ipv4.sin_family = AF_INET;
    key.RemoteAddress.Ipv4.sin_addr.S_un.S_addr = TcpRow->dwRemoteAddr;
    key.RemoteAddress.Ipv4.sin_port = (USHORT)TcpRow->dwRemotePort;
    
    return PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        ObjectId,
        &key,
        sizeof(NSI_TCP_KEY_ROW),
        DynamicBuffer,
        DynamicBufferSize,
        ReadWriteBuffer,
        ReadWriteBufferSize,
        StaticBuffer,
        StaticBufferSize
        );
}

/**
 * Retrieves the parameter blocks of an IPv6 TCP connection from the TCP NSI provider.
 *
 * \param TcpRow The TCP row that identifies the connection.
 * \param ObjectId The NSI TCP object identifier.
 * \param DynamicBuffer Receives the read-only dynamic block.
 * \param DynamicBufferSize The size, in bytes, of DynamicBuffer.
 * \param StaticBuffer Receives the read-only static block.
 * \param StaticBufferSize The size, in bytes, of StaticBuffer.
 * \param ReadWriteBuffer Receives the read-write block.
 * \param ReadWriteBufferSize The size, in bytes, of ReadWriteBuffer.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getpertcp6connectionestats
 */
NTSTATUS PhGetPerTcp6ConnectionStats(
    _In_ PMIB_TCP6ROW TcpRow,
    _In_ ULONG ObjectId,
    _Out_writes_bytes_opt_(DynamicBufferSize) PUCHAR DynamicBuffer,
    _In_ ULONG DynamicBufferSize,
    _Out_writes_bytes_opt_(StaticBufferSize) PUCHAR StaticBuffer,
    _In_ ULONG StaticBufferSize,
    _Out_writes_bytes_opt_(ReadWriteBufferSize) PUCHAR ReadWriteBuffer,
    _In_ ULONG ReadWriteBufferSize)
{
    NSI_TCP_KEY_ROW key;

    memset(&key, 0, sizeof(NSI_TCP_KEY_ROW));
    key.LocalAddress.Ipv6.sin6_family = AF_INET6;
    key.LocalAddress.Ipv6.sin6_port = (USHORT)TcpRow->dwLocalPort;
    key.LocalAddress.Ipv6.sin6_scope_id = TcpRow->dwLocalScopeId;
    memcpy(&key.LocalAddress.Ipv6.sin6_addr, &TcpRow->LocalAddr, sizeof(IN6_ADDR));
    key.RemoteAddress.Ipv6.sin6_family = AF_INET6;
    key.RemoteAddress.Ipv6.sin6_port = (USHORT)TcpRow->dwRemotePort;
    key.RemoteAddress.Ipv6.sin6_scope_id = TcpRow->dwRemoteScopeId;
    memcpy(&key.RemoteAddress.Ipv6.sin6_addr, &TcpRow->RemoteAddr, sizeof(IN6_ADDR));
    
    return PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        ObjectId,
        &key,
        sizeof(NSI_TCP_KEY_ROW),
        DynamicBuffer,
        DynamicBufferSize,
        ReadWriteBuffer,
        ReadWriteBufferSize,
        StaticBuffer,
        StaticBufferSize
        );
}

/**
 * Retrieves the interface stack table.
 *
 * \param Table Receives the allocated table. Free it with FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getifstacktable
 */
NTSTATUS PhGetIfStackTable(
    _In_ PMIB_IFSTACK_TABLE *Table
    )
{
    NTSTATUS status;
    PMIB_IFSTACK_TABLE memory;
    ULONG count = 0;
    ULONG dwBytes;
    MIB_NDIS_IFSTACK_KEY_ROW* keyTable = NULL;

    if (!Table)
        return STATUS_INVALID_PARAMETER;

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectIfStack,
        &keyTable,
        sizeof(MIB_NDIS_IFSTACK_KEY_ROW),
        NULL,
        0,
        NULL,
        0,
        NULL,
        0,
        &count,
        0
        );

    if (NT_SUCCESS(status))
    {
        if (count)
        {
            dwBytes = 0;

            if (PhpNetioMibSizeofTable(count, sizeof(MIB_IFSTACK_ROW), FALSE, &dwBytes))
            {
                memory = PhAllocate(dwBytes);
                if (memory)
                {
                    memory->NumEntries = count;

                    for (ULONG i = 0; i < count; i++)
                    {
                        memory->Table[i].HigherLayerInterfaceIndex = keyTable[i].HigherLayerInterfaceIndex;
                        memory->Table[i].LowerLayerInterfaceIndex = keyTable[i].LowerLayerInterfaceIndex;
                    }
                    *Table = memory;
                }
                else
                {
                    status = STATUS_NO_MEMORY;
                }
            }
            else
            {
                status = STATUS_INTEGER_OVERFLOW;
            }
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }

        PhNsiFreeTable(keyTable, NULL, NULL, NULL);
    }

    return status;
}

/**
 * Retrieves the inverted interface stack table.
 *
 * \param Table Receives the allocated table. Free it with FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getinvertedifstacktable
 */
NTSTATUS PhGetInvertedIfStackTable(
    _In_ PMIB_INVERTEDIFSTACK_TABLE *Table
    )
{
    return PhGetIfStackTable((PMIB_IFSTACK_TABLE *)Table);
}

/**
 * Retrieves information for a network interface at the specified access level.
 *
 * \param AccessType The level of interface information to retrieve.
 * \param Row On input, InterfaceLuid or InterfaceIndex identifies the interface; on output, the interface row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getifentry2ex
 */
NTSTATUS PhGetIfEntry2Ex(
    _In_ NET_IF_ACCESS_TYPE AccessType,
    _Inout_ PMIB_IF_ROW2 Row
    )
{
    if (!AccessType)
        return PhGetIfEntry2(Row);

    if (AccessType == NET_IF_ACCESS_BROADCAST)
        return PhGetIfEntry2(Row);

    return STATUS_INVALID_PARAMETER;
}

/**
 * Retrieves an IP route entry.
 *
 * \param Row On input, the key fields of the row; on output, the complete row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getipforwardentry2
 */
NTSTATUS PhGetIpForwardEntry2(
    _Inout_ PMIB_IPFORWARD_ROW2 Row
    )
{
    NTSTATUS result;
    PNPI_MODULEID moduleId;
    NET_LUID interfaceLuid;
    MIB_IP_ROUTE_RW_ROW rw;
    MIB_IP_ROUTE_STATIC_ROW ros;
    MIB_IP_ROUTE_KEY_ROW_V4 keyV4;
    MIB_IP_ROUTE_KEY_ROW_V6 keyV6;
    MIB_IP_ROUTE_DYNAMIC_ROW_V4 rodV4;
    MIB_IP_ROUTE_DYNAMIC_ROW_V6 rodV6;

    if (!Row || (Row->DestinationPrefix.Prefix.si_family != AF_INET && Row->DestinationPrefix.Prefix.si_family != AF_INET6))
        return STATUS_INVALID_PARAMETER;

    interfaceLuid = Row->InterfaceLuid;
    if (!interfaceLuid.Value)
    {
        if (!Row->InterfaceIndex)
            return STATUS_INVALID_PARAMETER;

        result = PhConvertInterfaceIndexToLuid(Row->InterfaceIndex, &interfaceLuid);
        if (result)
            return result;
    }

    if (Row->DestinationPrefix.Prefix.si_family == AF_INET)
    {
        if (Row->DestinationPrefix.PrefixLength > 32 || Row->NextHop.si_family != AF_INET)
            return STATUS_INVALID_PARAMETER;

        moduleId = (PNPI_MODULEID)&NPI_MS_IPV4_MODULEID;
        memset(&keyV4, 0, sizeof(keyV4));
        keyV4.CompartmentId = PhGetCurrentThreadCompartmentId();
        keyV4.DestinationPrefix = Row->DestinationPrefix.Prefix.Ipv4.sin_addr;
        keyV4.DestinationPrefixLength = Row->DestinationPrefix.PrefixLength;
        keyV4.InterfaceLuid = interfaceLuid;
        keyV4.NextHop = Row->NextHop.Ipv4.sin_addr;
        result = PhNsiGetAllParameters(NsiActive, moduleId, NsiIpObjectRoute,
            &keyV4, sizeof(keyV4), &rw, sizeof(rw), &rodV4, sizeof(rodV4), &ros, sizeof(ros));
        if (!result)
            PhpFillRouteEntry(AF_INET, Row, &keyV4, &rw, &rodV4, &ros);
    }
    else
    {
        if (Row->DestinationPrefix.PrefixLength > 128 || Row->NextHop.si_family != AF_INET6)
            return STATUS_INVALID_PARAMETER;

        moduleId = (PNPI_MODULEID)&NPI_MS_IPV6_MODULEID;
        memset(&keyV6, 0, sizeof(keyV6));
        keyV6.CompartmentId = PhGetCurrentThreadCompartmentId();
        keyV6.DestinationPrefix = Row->DestinationPrefix.Prefix.Ipv6.sin6_addr;
        keyV6.DestinationPrefixLength = Row->DestinationPrefix.PrefixLength;
        keyV6.InterfaceLuid = interfaceLuid;
        keyV6.NextHop = Row->NextHop.Ipv6.sin6_addr;
        result = PhNsiGetAllParameters(NsiActive, moduleId, NsiIpObjectRoute,
            &keyV6, sizeof(keyV6), &rw, sizeof(rw), &rodV6, sizeof(rodV6), &ros, sizeof(ros));
        if (!result)
            PhpFillRouteEntry(AF_INET6, Row, &keyV6, &rw, &rodV6, &ros);
    }

    return result;
}

/**
 * Retrieves the index of the best interface for an IPv4 destination.
 *
 * \param DestinationAddress The IPv4 destination address, in network byte order.
 * \param BestIfIndex Receives the index of the best interface.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getbestinterface
 */
NTSTATUS PhGetBestInterface(
    _In_ IPAddr DestinationAddress,
    _Out_ PULONG BestIfIndex
    )
{
    struct sockaddr_in destination;

    memset(&destination, 0, sizeof(struct sockaddr_in));
    destination.sin_family = AF_INET;
    destination.sin_addr.S_un.S_addr = DestinationAddress;
    
    return PhInternalGetBestInterface(
        (struct sockaddr *)&destination,
        BestIfIndex,
        BestIfIndex
        );
}

/**
 * Retrieves the index of the best interface for an IPv4 or IPv6 destination.
 *
 * \param DestinationAddress The destination address.
 * \param BestIfIndex Receives the index of the best interface.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getbestinterfaceex
 */
NTSTATUS PhGetBestInterfaceEx(
    _In_ struct sockaddr *DestinationAddress,
    _Out_ PULONG BestIfIndex
    )
{
    ADDRESS_FAMILY family;
    const NPI_MODULEID *moduleId;
    ULONG inputSize;
    ULONG controlCode;
    PVOID keyBuffer;
    NSI_IP_BEST_ROUTE_KEY4 keyV4;
    NSI_IP_BEST_ROUTE_KEY6 keyV6;

    if (!BestIfIndex || !DestinationAddress)
        return STATUS_INVALID_PARAMETER;

    family = DestinationAddress->sa_family;

    if (family == AF_INET)
    {
        const PSOCKADDR_IN sockAddrIn4 = (PSOCKADDR_IN)DestinationAddress;
        
        moduleId = &NPI_MS_IPV4_MODULEID;

        memset(&keyV4, 0, sizeof(NSI_IP_BEST_ROUTE_KEY4));
        keyV4.DestinationAddress = sockAddrIn4->sin_addr;

        keyBuffer = &keyV4;
        inputSize = sizeof(NSI_IP_BEST_ROUTE_KEY4);
        controlCode = NsiIpv4BestRouteType1BestIfIndex;
    }
    else if (family == AF_INET6)
    {
        const PSOCKADDR_IN6 sockAddrIn6 = (PSOCKADDR_IN6)DestinationAddress;

        moduleId = &NPI_MS_IPV6_MODULEID;

        if (sockAddrIn6->sin6_scope_struct.Level == ScopeLevelInterface)
        {
            *BestIfIndex = sockAddrIn6->sin6_scope_struct.Zone;
            return STATUS_SUCCESS;
        }

        memset(&keyV6, 0, sizeof(NSI_IP_BEST_ROUTE_KEY6));
        keyV6.DestinationAddress = sockAddrIn6->sin6_addr;
        keyV6.DestinationScopeId.Value = sockAddrIn6->sin6_scope_id;

        keyBuffer = &keyV6;
        inputSize = sizeof(NSI_IP_BEST_ROUTE_KEY6);
        controlCode = NsiIpv6BestRouteType1BestIfIndex;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    return PhpNsiGetParameter(
        NsiActive,
        moduleId,
        NsiIpObjectBestRoute,
        keyBuffer,
        inputSize,
        NsiParameterTypeReadOnlyDynamic,
        BestIfIndex,
        sizeof(ULONG),
        controlCode
        );
}

#pragma endregion

#pragma region DHCP and IP Path

static PVOID PhpDhcpcsvcHandle = NULL;

/**
 * Resolves an export of dhcpcsvc.dll, loading the module on first use.
 *
 * \param ProcedureName The name of the export to resolve.
 * \return The export address, or NULL.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress
 */
static PVOID PhpGetDhcpcsvcProcedureAddress(
    _In_ PCSTR ProcedureName
    )
{
    PVOID moduleHandle;
    PVOID newModuleHandle;

    moduleHandle = ReadPointerAcquire(&PhpDhcpcsvcHandle);

    if (!moduleHandle)
    {
        newModuleHandle = PhLoadLibrary(L"dhcpcsvc.dll");

        if (!newModuleHandle)
            return NULL;

        moduleHandle = InterlockedCompareExchangePointer(&PhpDhcpcsvcHandle, newModuleHandle, NULL);

        if (moduleHandle)
            PhFreeLibrary(newModuleHandle);
        else
            moduleHandle = newModuleHandle;
    }

    return PhGetDllBaseProcedureAddress(moduleHandle, ProcedureName, 0);
}

/**
 * Releases the DHCP lease of an adapter (dhcpcsvc!DhcpReleaseParameters).
 *
 * \param AdapterName The adapter GUID string.
 * \param Reserved Reserved; must be zero or NULL.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-ipreleaseaddress
 */
NTSTATUS PhDhcpReleaseParameters(
    _In_ PWSTR AdapterName,
    _In_opt_ PVOID Reserved
    )
{
    typedef ULONG (WINAPI *ReleaseParameters_I)(_In_ PWSTR AdapterName, _In_opt_ PVOID Reserved);
    static ReleaseParameters_I ReleaseParameters = NULL;

    if (!ReleaseParameters)
    {
        ReleaseParameters = PhpGetDhcpcsvcProcedureAddress("DhcpReleaseParameters");
    }

    if (ReleaseParameters)
    {
        return PhpNetioDosErrorToNtStatus(ReleaseParameters(AdapterName, Reserved));
    }

    return STATUS_NOT_SUPPORTED;
}

/**
 * Renews the DHCP lease of an adapter (dhcpcsvc!DhcpAcquireParameters).
 *
 * \param AdapterName The adapter GUID string.
 * \param Reserved Reserved; must be zero or NULL.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-iprenewaddress
 */
NTSTATUS PhDhcpAcquireParameters(
    _In_ PWSTR AdapterName,
    _In_opt_ PVOID Reserved
    )
{
    typedef ULONG (WINAPI *AcquireParameters_I)(_In_ PWSTR AdapterName, _In_opt_ PVOID Reserved);
    static AcquireParameters_I AcquireParameters = NULL;

    if (!AcquireParameters)
    {
        AcquireParameters = PhpGetDhcpcsvcProcedureAddress("DhcpAcquireParameters");
    }

    if (AcquireParameters)
    {
        return PhpNetioDosErrorToNtStatus(AcquireParameters(AdapterName, Reserved));
    }

    return STATUS_NOT_SUPPORTED;
}

/**
 * Releases the IPv4 address obtained through DHCP for an adapter.
 *
 * \param AdapterInfo The adapter index map entry that identifies the adapter.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-ipreleaseaddress
 */
NTSTATUS PhIpReleaseAddress(
    _In_ PIP_ADAPTER_INDEX_MAP AdapterInfo
    )
{
    if (!AdapterInfo)
        return STATUS_INVALID_PARAMETER;

    SIZE_T len = PhCountStringZ(AdapterInfo->Name);
    if (len > TCPIP_ADAPTER_PREFIX_LENGTH)
        return PhDhcpReleaseParameters(&AdapterInfo->Name[TCPIP_ADAPTER_PREFIX_LENGTH], NULL);
    else
        return STATUS_INVALID_PARAMETER;
}

/**
 * Renews the IPv4 DHCP lease for an adapter.
 *
 * \param AdapterInfo The adapter index map entry that identifies the adapter.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-iprenewaddress
 */
NTSTATUS PhIpRenewAddress(
    _In_ PIP_ADAPTER_INDEX_MAP AdapterInfo
    )
{
    if (!AdapterInfo)
        return STATUS_INVALID_PARAMETER;

    SIZE_T len = PhCountStringZ(AdapterInfo->Name);
    if (len > TCPIP_ADAPTER_PREFIX_LENGTH)
        return PhDhcpAcquireParameters(&AdapterInfo->Name[TCPIP_ADAPTER_PREFIX_LENGTH], NULL);
    else
        return STATUS_INVALID_PARAMETER;
}

/**
 * Determines the round-trip time and hop count to a destination.
 *
 * \param DestIpAddress The IPv4 destination address.
 * \param HopCount Receives the hop count.
 * \param MaxHops The maximum number of hops to search.
 * \param RTT Receives the round-trip time, in milliseconds.
 * \return TRUE on success, otherwise FALSE (the last error is set).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getrttandhopcount
 */
BOOL PhGetRTTAndHopCount(
    _In_ IPAddr DestIpAddress,
    _Out_ PULONG HopCount,
    _In_ ULONG MaxHops,
    _Out_ PULONG RTT
    )
{
    HANDLE fileHandle;
    ULONG lastError;
    PVOID replyBuffer;
    BOOLEAN success = TRUE;
    struct ip_option_information requestOptions;
    UCHAR requestData[PHP_ICMP_ECHO_DATA_SIZE];

    if (!HopCount || !RTT || DestIpAddress == INADDR_NONE)
    {
        PhSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    memset(&requestOptions, 0, sizeof(struct ip_option_information));

    fileHandle = PhIcmpCreateFileEx(&NPI_MS_IPV4_MODULEID, TRUE);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        PhSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    replyBuffer = PhAllocate(PHP_ICMP_ECHO_REPLY_BUFFER_SIZE);
    if (!replyBuffer)
    {
        PhIcmpCloseHandle(fileHandle);
        PhSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(requestData); i++)
    {
        requestData[i] = 'a' + (UCHAR)(i % PHP_ICMP_ECHO_PATTERN_LENGTH);
    }

    requestOptions.Tos = 0;
    
    for (UCHAR ttl = 1; ttl <= MaxHops; ttl++)
    {
        requestOptions.Ttl = ttl;
        
        if (PhIcmpSendEcho2Ex(fileHandle, NULL, NULL, NULL, 0, DestIpAddress, requestData, sizeof(requestData), &requestOptions, replyBuffer, PHP_ICMP_ECHO_REPLY_BUFFER_SIZE, PHP_ICMP_ECHO_TIMEOUT))
        {
            lastError = ((PICMP_ECHO_REPLY)replyBuffer)->Status;
        }
        else
        {
            lastError = PhGetLastError();
        }

        if (lastError == 0)
        {
            *HopCount = requestOptions.Ttl;
            
            if (PhIcmpSendEcho2Ex(fileHandle, NULL, NULL, NULL, 0, DestIpAddress, requestData, sizeof(requestData), &requestOptions, replyBuffer, PHP_ICMP_ECHO_REPLY_BUFFER_SIZE, PHP_ICMP_ECHO_TIMEOUT) &&
                PhIcmpSendEcho2Ex(fileHandle, NULL, NULL, NULL, 0, DestIpAddress, requestData, sizeof(requestData), &requestOptions, replyBuffer, PHP_ICMP_ECHO_REPLY_BUFFER_SIZE, PHP_ICMP_ECHO_TIMEOUT))
            {
                *RTT = ((PICMP_ECHO_REPLY)replyBuffer)->RoundTripTime;
            }
            else
            {
                success = FALSE;
            }
            
            PhFree(replyBuffer);
            PhIcmpCloseHandle(fileHandle);
            return success;
        }

        if (lastError != IP_TTL_EXPIRED_TRANSIT && lastError != IP_REQ_TIMED_OUT)
        {
            success = FALSE;
            PhFree(replyBuffer);
            PhIcmpCloseHandle(fileHandle);
            return success;
        }
    }

    PhFree(replyBuffer);
    PhIcmpCloseHandle(fileHandle);
    return FALSE;
}

/**
 * Builds a MIB_IPPATH_ROW from the IP path NSI key and parameter blocks.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Row Receives the row.
 * \param KeyRow The path key.
 * \param RodRow The path read-only dynamic block.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/ns-netioapi-mib_ippath_row
 */
VOID PhpFillIpPathRow(
    _In_ ADDRESS_FAMILY Family,
    _Out_ PMIB_IPPATH_ROW Row,
    _In_ const VOID* KeyRow,
    _In_ const VOID* RodRow
    )
{
    memset(Row, 0, sizeof(MIB_IPPATH_ROW));

    if (Family == AF_INET)
    {
        const NSI_IP_PATH_KEY_V4* key = (const NSI_IP_PATH_KEY_V4*)KeyRow;
        const NSI_IP_PATH_ROD_V4* rod = (const NSI_IP_PATH_ROD_V4*)RodRow;

        Row->Source.si_family = AF_INET;
        Row->Source.Ipv4.sin_addr = key->Source;

        Row->Destination.si_family = AF_INET;
        Row->Destination.Ipv4.sin_addr = key->Destination;

        Row->InterfaceLuid = key->InterfaceLuid;
        PhConvertInterfaceLuidToIndex(&Row->InterfaceLuid, &Row->InterfaceIndex);

        Row->CurrentNextHop.si_family = AF_INET;
        Row->CurrentNextHop.Ipv4.sin_addr = rod->CurrentNextHop;
        Row->PathMtu = rod->PathMtu;
        Row->RttMean = rod->RttMean / PHP_NETIO_RTT_SCALE;
        Row->RttDeviation = rod->RttDeviation / PHP_NETIO_RTT_SCALE;
        Row->LastReachable = rod->ReachabilityTime;
        Row->IsReachable = rod->IsReachable;
        Row->LinkTransmitSpeed = rod->LinkTransmitSpeed;
        Row->LinkReceiveSpeed = rod->LinkReceiveSpeed;
    }
    else if (Family == AF_INET6)
    {
        const NSI_IP_PATH_KEY_V6* key = (const NSI_IP_PATH_KEY_V6*)KeyRow;
        const NSI_IP_PATH_ROD_V6* rod = (const NSI_IP_PATH_ROD_V6*)RodRow;

        Row->Source.si_family = AF_INET6;
        Row->Source.Ipv6.sin6_addr = key->Source;

        Row->Destination.si_family = AF_INET6;
        Row->Destination.Ipv6.sin6_addr = key->Destination;

        Row->InterfaceLuid = key->InterfaceLuid;
        PhConvertInterfaceLuidToIndex(&Row->InterfaceLuid, &Row->InterfaceIndex);

        Row->CurrentNextHop.si_family = AF_INET6;
        Row->CurrentNextHop.Ipv6.sin6_addr = rod->CurrentNextHop;
        Row->PathMtu = rod->PathMtu;
        Row->RttMean = rod->RttMean / PHP_NETIO_RTT_SCALE;
        Row->RttDeviation = rod->RttDeviation / PHP_NETIO_RTT_SCALE;
        Row->LastReachable = rod->ReachabilityTime;
        Row->IsReachable = rod->IsReachable;
        Row->LinkTransmitSpeed = rod->LinkTransmitSpeed;
        Row->LinkReceiveSpeed = rod->LinkReceiveSpeed;
    }
}

/**
 * Retrieves an IP path entry.
 *
 * \param Row On input, the key fields of the row; on output, the complete row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getippathentry
 */
NTSTATUS PhGetIpPathEntry(
    _Inout_ PMIB_IPPATH_ROW Row
    )
{
    ADDRESS_FAMILY sin_family;
    ADDRESS_FAMILY sourceFamily;
    NET_IF_COMPARTMENT_ID CurrentThreadCompartmentId;
    VOID* keyTable = NULL;
    VOID* rodTable = NULL;
    ULONG count = 0;
    NTSTATUS status;

    if (!Row)
        return STATUS_INVALID_PARAMETER;

    sin_family = Row->Destination.Ipv4.sin_family;
    if (sin_family != AF_INET && sin_family != AF_INET6)
        return STATUS_INVALID_PARAMETER;

    sourceFamily = Row->Source.Ipv4.sin_family;
    if (Row->Source.Ipv4.sin_family != sin_family)
    {
        if (sourceFamily)
            return STATUS_INVALID_PARAMETER;
    }

    CurrentThreadCompartmentId = PhGetCurrentThreadCompartmentId();
    NET_LUID interfaceLuid;
    interfaceLuid = Row->InterfaceLuid;

    if (!interfaceLuid.Value)
    {
        if (Row->InterfaceIndex)
        {
            status = PhConvertInterfaceIndexToLuid(Row->InterfaceIndex, &interfaceLuid);
            if (!NT_SUCCESS(status))
                return status;
        }
        else
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    ULONG keySize = (sin_family == AF_INET) ? sizeof(NSI_IP_PATH_KEY_V4) : sizeof(NSI_IP_PATH_KEY_V6);
    ULONG rodSize = (sin_family == AF_INET) ? sizeof(NSI_IP_PATH_ROD_V4) : sizeof(NSI_IP_PATH_ROD_V6);

    const NPI_MODULEID* moduleId = (sin_family == AF_INET) ? &NPI_MS_IPV4_MODULEID : &NPI_MS_IPV6_MODULEID;

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        (PNPI_MODULEID)moduleId,
        NsiIpObjectPath,
        &keyTable,
        keySize,
        NULL,
        0,
        &rodTable,
        rodSize,
        NULL,
        0,
        &count,
        0
        );

    if (NT_SUCCESS(status))
    {
        status = STATUS_NOT_FOUND;

        for (ULONG i = 0; i < count; i++)
        {
            if (sin_family == AF_INET)
            {
                const NSI_IP_PATH_KEY_V4* key = &((const NSI_IP_PATH_KEY_V4*)keyTable)[i];

                if (key->CompartmentId == CurrentThreadCompartmentId && key->InterfaceLuid.Value == interfaceLuid.Value)
                {
                    if (key->Destination.S_un.S_addr == Row->Destination.Ipv4.sin_addr.S_un.S_addr)
                    {
                        if (sourceFamily == 0 || key->Source.S_un.S_addr == Row->Source.Ipv4.sin_addr.S_un.S_addr)
                        {
                            PhpFillIpPathRow(AF_INET, Row, key, &((const NSI_IP_PATH_ROD_V4*)rodTable)[i]);
                            status = STATUS_SUCCESS;
                            break;
                        }
                    }
                }
            }
            else
            {
                const NSI_IP_PATH_KEY_V6* key = &((const NSI_IP_PATH_KEY_V6*)keyTable)[i];

                if (key->CompartmentId == CurrentThreadCompartmentId && key->InterfaceLuid.Value == interfaceLuid.Value)
                {
                    if (IN6_ADDR_EQUAL(&key->Destination, &Row->Destination.Ipv6.sin6_addr))
                    {
                        if (sourceFamily == 0 || IN6_ADDR_EQUAL(&key->Source, &Row->Source.Ipv6.sin6_addr))
                        {
                            PhpFillIpPathRow(AF_INET6, Row, key, &((const NSI_IP_PATH_ROD_V6*)rodTable)[i]);
                            status = STATUS_SUCCESS;
                            break;
                        }
                    }
                }
            }
        }

        PhNsiFreeTable(keyTable, NULL, rodTable, NULL);
    }

    return status;
}

/**
 * Retrieves the list of IPv4 network adapters.
 *
 * \param IfTable The buffer that receives the adapter list.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getinterfaceinfo
 */
NTSTATUS PhGetInterfaceInfo(
    _Out_writes_bytes_opt_(*SizePointer) PIP_INTERFACE_INFO IfTable,
    _Inout_ PULONG SizePointer
    )
{
    NTSTATUS result;
    NET_IF_COMPARTMENT_ID CurrentThreadCompartmentId;
    NET_LUID* keyTable = NULL;
    PNDIS_NSI_INTERFACE_ENUM_ROD rodTable = NULL;
    PNDIS_NSI_INTERFACE_ENUM_ROS rosTable = NULL;
    ULONG count = 0;
    ULONG outputCount = 0;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceWithoutStatistics,
        &keyTable,
        sizeof(NET_LUID),
        NULL,
        0,
        &rodTable,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROD),
        &rosTable,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROS),
        &count,
        0
        );

    if (NT_SUCCESS(result))
    {
        CurrentThreadCompartmentId = PhGetCurrentThreadCompartmentId();

        for (ULONG i = 0; i < count; i++)
        {
            if (rodTable[i].CompartmentId == CurrentThreadCompartmentId)
            {
                if (PhpIsIpv4Interface(&keyTable[i], rosTable[i].ifType))
                {
                    if (IfTable && PhpNetioMibTableHasCapacity(outputCount + 1, sizeof(IP_ADAPTER_INDEX_MAP), *SizePointer))
                    {
                        PIP_ADAPTER_INDEX_MAP dest = &IfTable->Adapter[outputCount];
                        dest->Index = rosTable[i].ifIndex;

                        WCHAR guidString[RTL_GUID_STRING_SIZE + 1];

                        if (NT_SUCCESS(PhConvertGuidToStringW(&rosTable[i].InterfaceGuid, guidString, RTL_NUMBER_OF(guidString))))
                        {
                            PPH_STRING interfaceName = PhFormatString(L"\\DEVICE\\TCPIP_%s", guidString);

                            if (interfaceName)
                            {
                                PhCopyStringZ(interfaceName->Buffer, interfaceName->Length / sizeof(WCHAR), dest->Name, RTL_NUMBER_OF(dest->Name), NULL);
                                PhDereferenceObject(interfaceName);
                            }
                        }
                    }
                    outputCount++;
                }
            }
        }

        if (IfTable && *SizePointer >= sizeof(IP_INTERFACE_INFO))
        {
            IfTable->NumAdapters = outputCount;
        }

        result = PhpNetioMibCheckTableBufferSize(
            outputCount,
            sizeof(IP_ADAPTER_INDEX_MAP),
            IfTable,
            SizePointer
            );

        if (NT_SUCCESS(result) && outputCount == 0)
        {
            result = STATUS_NO_MORE_ENTRIES;
        }

        PhNsiFreeTable(keyTable, NULL, rodTable, rosTable);
    }

    return result;
}

#pragma endregion

#pragma region DNS

static PVOID PhDnsapiHandle = NULL;

/**
 * Resolves an export of dnsapi.dll, loading the module on first use.
 *
 * \param ProcedureName The name of the export to resolve.
 * \return The export address, or NULL.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress
 */
static PVOID PhpGetDnsapiProcedureAddress(
    _In_ PCSTR ProcedureName
    )
{
    PVOID moduleHandle;
    PVOID newModuleHandle;

    moduleHandle = ReadPointerAcquire(&PhDnsapiHandle);

    if (!moduleHandle)
    {
        newModuleHandle = PhLoadLibrary(L"dnsapi.dll");

        if (!newModuleHandle)
            return NULL;

        moduleHandle = InterlockedCompareExchangePointer(&PhDnsapiHandle, newModuleHandle, NULL);

        if (moduleHandle)
            PhFreeLibrary(newModuleHandle);
        else
            moduleHandle = newModuleHandle;
    }

    return PhGetDllBaseProcedureAddress(moduleHandle, ProcedureName, 0);
}

/**
 * Retrieves the global DNS settings (dnsapi!DnsGetSettings).
 *
 * \param Settings On input, the version of the structure; on output, the settings.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getdnssettings
 */
NTSTATUS PhDnsGetSettings(
    _Inout_ PVOID Settings
    )
{
    typedef ULONG (WINAPI *GetSettings_I)(_Inout_ PVOID Settings);
    static GetSettings_I GetSettings = NULL;

    if (!GetSettings)
    {
        GetSettings = PhpGetDnsapiProcedureAddress("DnsGetSettings");
    }

    if (GetSettings)
    {
        return PhpNetioDosErrorToNtStatus(GetSettings(Settings));
    }

    return STATUS_NOT_SUPPORTED;
}

/**
 * Retrieves the DNS settings of an interface (dnsapi!DnsGetInterfaceSettings).
 *
 * \param InterfaceGuid The interface GUID.
 * \param Settings On input, the version of the structure; on output, the settings.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getinterfacednssettings
 */
NTSTATUS PhDnsGetInterfaceSettings(
    _In_ LPCGUID InterfaceGuid,
    _Inout_ PVOID Settings
    )
{
    typedef ULONG (WINAPI *GetInterfaceSettings_I)(_In_ LPCGUID InterfaceGuid, _Inout_ PVOID Settings);
    static GetInterfaceSettings_I GetInterfaceSettings = NULL;

    if (!GetInterfaceSettings)
    {
        GetInterfaceSettings = PhpGetDnsapiProcedureAddress("DnsGetInterfaceSettings");
    }

    if (GetInterfaceSettings)
    {
        return PhpNetioDosErrorToNtStatus(GetInterfaceSettings(InterfaceGuid, Settings));
    }

    return STATUS_NOT_SUPPORTED;
}

/**
 * Frees the DNS server properties returned in DNS interface settings (dnsapi!FreeDnsServerProperties).
 *
 * \param ServerPropertyCount The number of server properties.
 * \param ServerProperties The server properties.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-freeinterfacednssettings
 */
VOID PhFreeDnsServerProperties(
    _In_ ULONG ServerPropertyCount,
    _Inout_ PVOID ServerProperties
    )
{
    typedef VOID (WINAPI *FreeDnsServerProperties_I)(_In_ ULONG ServerPropertyCount, _Inout_ PVOID ServerProperties);
    static FreeDnsServerProperties_I FreeDnsServerProperties = NULL;

    if (!FreeDnsServerProperties)
    {
        FreeDnsServerProperties = PhpGetDnsapiProcedureAddress("DnsFreeDnsServerProperties");
    }

    if (FreeDnsServerProperties)
    {
        FreeDnsServerProperties(ServerPropertyCount, ServerProperties);
    }
}

/**
 * Retrieves the global DNS settings.
 *
 * \param Settings On input, the version of the structure; on output, the settings.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getdnssettings
 */
NTSTATUS PhGetDnsSettings(
    _Inout_ DNS_SETTINGS *Settings
    )
{
    NTSTATUS result;
    DNS_SETTINGS2 request;

    if (!Settings ||
        Settings->Version < DNS_SETTINGS_VERSION1 ||
        Settings->Version > DNS_SETTINGS_VERSION2)
        return STATUS_INVALID_PARAMETER;

    memset(&request, 0, sizeof(DNS_SETTINGS2));
    request.Version = DNS_SETTINGS_VERSION1;
    request.Flags = Settings->Flags;

    result = PhDnsGetSettings((DNS_SETTINGS*)&request);

    if (result == 0)
    {
        Settings->Hostname = request.Hostname;
        Settings->Domain = request.Domain;
        Settings->SearchList = request.SearchList;
        if (Settings->Version == DNS_SETTINGS_VERSION2)
        {
            ((DNS_SETTINGS2*)Settings)->SettingFlags = request.SettingFlags;
        }
    }
    return result;
}

#pragma endregion

#pragma region Network Configuration and Adapters

/**
 * Copies an owner module name and path into a TCPIP_OWNER_MODULE_BASIC_INFO buffer.
 *
 * \param Name The image name.
 * \param NameLength The size, in bytes, of Name including the terminator.
 * \param Path The image path.
 * \param PathLength The size, in bytes, of Path including the terminator.
 * \param Buffer The buffer that receives the module information.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iprtrmib/ns-iprtrmib-tcpip_owner_module_basic_info
 */
static NTSTATUS PhpCopyOwnerModuleInfo(
    _In_reads_bytes_(NameLength) PCWSTR Name,
    _In_ ULONG NameLength,
    _In_reads_bytes_(PathLength) PCWSTR Path,
    _In_ ULONG PathLength,
    _Out_writes_bytes_opt_(*SizePointer) PVOID Buffer,
    _Inout_ PULONG SizePointer
    )
{
    ULONG requiredSize;
    PTCPIP_OWNER_MODULE_BASIC_INFO information;
    PWSTR nameBuffer;
    PWSTR pathBuffer;

    if (NameLength > MAXULONG - sizeof(TCPIP_OWNER_MODULE_BASIC_INFO) - 2 * sizeof(WCHAR) ||
        PathLength > MAXULONG - sizeof(TCPIP_OWNER_MODULE_BASIC_INFO) - 2 * sizeof(WCHAR) - NameLength)
        return STATUS_INTEGER_OVERFLOW;

    requiredSize = sizeof(TCPIP_OWNER_MODULE_BASIC_INFO) + NameLength + PathLength + 2 * sizeof(WCHAR);
    if (*SizePointer < requiredSize || !Buffer)
    {
        *SizePointer = requiredSize;
        return STATUS_BUFFER_TOO_SMALL;
    }

    information = Buffer;
    nameBuffer = (PWSTR)(information + 1);
    pathBuffer = (PWSTR)((PUCHAR)nameBuffer + NameLength + sizeof(WCHAR));
    information->pModuleName = nameBuffer;
    information->pModulePath = pathBuffer;
    memcpy(nameBuffer, Name, NameLength);
    nameBuffer[NameLength / sizeof(WCHAR)] = UNICODE_NULL;
    memcpy(pathBuffer, Path, PathLength);
    pathBuffer[PathLength / sizeof(WCHAR)] = UNICODE_NULL;
    return STATUS_SUCCESS;
}

/**
 * Retrieves the owner module information for a process.
 *
 * \param ProcessId The owning process identifier.
 * \param Info The owning module information from the table row.
 * \param Class The owner module information class. Only TCPIP_OWNER_MODULE_INFO_BASIC is supported.
 * \param Buffer The buffer that receives the module information.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getownermodulefromtcpentry
 */
static NTSTATUS PhpGetOwnerModuleFromPidAndInfo(
    _In_ ULONG ProcessId,
    _In_ PULONG64 Info,
    _In_ TCPIP_OWNER_MODULE_INFO_CLASS Class,
    _Out_ PVOID Buffer,
    _Inout_ PULONG SizePointer
    )
{
    HANDLE processHandle;
    PPH_STRING imagePath;
    PCWSTR imageName;
    ULONG imageNameLength;
    NTSTATUS result;
    NTSTATUS status;

    if (!SizePointer || Class != TCPIP_OWNER_MODULE_INFO_BASIC)
        return STATUS_INVALID_PARAMETER;
    if (!ProcessId)
        return STATUS_NOT_FOUND;

    if (ProcessId == 4)
    {
        static const WCHAR systemName[] = L"System";

        // IP Helper includes the existing terminators in these string lengths.
        return PhpCopyOwnerModuleInfo(
            systemName,
            sizeof(systemName),
            systemName,
            sizeof(systemName),
            Buffer,
            SizePointer
            );
    }

    if (Info[0])
    {
        PQUERY_TAG_INFORMATION queryTagInformation;
        TAG_INFO_NAME_FROM_TAG tagInformation;
        SIZE_T serviceNameLength;

        queryTagInformation = PhGetDllProcedureAddressZ(L"sechost.dll", "I_QueryTagInformation", 0);
        if (!queryTagInformation)
            return STATUS_PROCEDURE_NOT_FOUND;

        memset(&tagInformation, 0, sizeof(tagInformation));
        tagInformation.InParams.ProcessId = ProcessId;
        tagInformation.InParams.ServiceTag = (ULONG)Info[0];
        result = PhpNetioDosErrorToNtStatus(queryTagInformation(NULL, eTagInfoLevelNameFromTag, &tagInformation));
        if (result)
            return result;
        if (!tagInformation.OutParams.Name)
            return STATUS_NOT_FOUND;

        serviceNameLength = wcslen(tagInformation.OutParams.Name) * sizeof(WCHAR);
        if (serviceNameLength > MAXULONG)
        {
            LocalFree((HLOCAL)tagInformation.OutParams.Name);
            return STATUS_INTEGER_OVERFLOW;
        }
        result = PhpCopyOwnerModuleInfo(tagInformation.OutParams.Name, (ULONG)serviceNameLength,
            tagInformation.OutParams.Name, (ULONG)serviceNameLength, Buffer, SizePointer);
        LocalFree((HLOCAL)tagInformation.OutParams.Name);
        return result;
    }

    status = PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, UlongToHandle(ProcessId));
    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcessImageFileNameWin32(processHandle, &imagePath);
    NtClose(processHandle);
    if (!NT_SUCCESS(status))
        return status;

    if (imagePath->Length > MAXULONG)
    {
        PhDereferenceObject(imagePath);
        return STATUS_INTEGER_OVERFLOW;
    }

    // The module name is the final component of the Win32 image path.
    imageName = imagePath->Buffer;
    for (ULONG i = 0; i < (ULONG)(imagePath->Length / sizeof(WCHAR)); i++)
    {
        if (imagePath->Buffer[i] == L'\\' || imagePath->Buffer[i] == L'/')
            imageName = &imagePath->Buffer[i + 1];
    }
    imageNameLength = (ULONG)imagePath->Length - (ULONG)((PUCHAR)imageName - (PUCHAR)imagePath->Buffer);
    result = PhpCopyOwnerModuleInfo(imageName, imageNameLength,
        imagePath->Buffer, (ULONG)imagePath->Length, Buffer, SizePointer);
    PhDereferenceObject(imagePath);
    return result;
}

/**
 * Retrieves the owner module of an IPv4 TCP connection.
 *
 * \param TcpEntry The TCP table row that identifies the connection.
 * \param Class The owner module information class. Only TCPIP_OWNER_MODULE_INFO_BASIC is supported.
 * \param Buffer The buffer that receives the module information.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getownermodulefromtcpentry
 */
NTSTATUS PhGetOwnerModuleFromTcpEntry(
    _In_ PMIB_TCPROW_OWNER_MODULE TcpEntry,
    _In_ TCPIP_OWNER_MODULE_INFO_CLASS Class,
    _Out_writes_bytes_opt_(*SizePointer) PVOID Buffer,
    _Inout_ PULONG SizePointer
    )
{
    if (TcpEntry && Class == TCPIP_OWNER_MODULE_INFO_BASIC && SizePointer)
    {
        return PhpGetOwnerModuleFromPidAndInfo(
            TcpEntry->dwOwningPid,
            TcpEntry->OwningModuleInfo,
            TCPIP_OWNER_MODULE_INFO_BASIC,
            Buffer,
            SizePointer);
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }
}

/**
 * Retrieves the owner module of an IPv4 UDP endpoint.
 *
 * \param UdpEntry The UDP table row that identifies the endpoint.
 * \param Class The owner module information class. Only TCPIP_OWNER_MODULE_INFO_BASIC is supported.
 * \param Buffer The buffer that receives the module information.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getownermodulefromudpentry
 */
NTSTATUS PhGetOwnerModuleFromUdpEntry(
    _In_ PMIB_UDPROW_OWNER_MODULE UdpEntry,
    _In_ TCPIP_OWNER_MODULE_INFO_CLASS Class,
    _Out_writes_bytes_opt_(*SizePointer) PVOID Buffer,
    _Inout_ PULONG SizePointer
    )
{
    if (UdpEntry && Class == TCPIP_OWNER_MODULE_INFO_BASIC && SizePointer)
    {
        return PhpGetOwnerModuleFromPidAndInfo(
            UdpEntry->dwOwningPid,
            UdpEntry->OwningModuleInfo,
            TCPIP_OWNER_MODULE_INFO_BASIC,
            Buffer,
            SizePointer);
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }
}

/**
 * Retrieves the owner module of an IPv6 TCP connection.
 *
 * \param TcpEntry The TCP table row that identifies the connection.
 * \param Class The owner module information class. Only TCPIP_OWNER_MODULE_INFO_BASIC is supported.
 * \param Buffer The buffer that receives the module information.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getownermodulefromtcp6entry
 */
NTSTATUS PhGetOwnerModuleFromTcp6Entry(
    _In_ PMIB_TCP6ROW_OWNER_MODULE TcpEntry,
    _In_ TCPIP_OWNER_MODULE_INFO_CLASS Class,
    _Out_writes_bytes_opt_(*SizePointer) PVOID Buffer,
    _Inout_ PULONG SizePointer
    )
{
    if (TcpEntry && Class == TCPIP_OWNER_MODULE_INFO_BASIC && SizePointer)
        return PhpGetOwnerModuleFromPidAndInfo(
            TcpEntry->dwOwningPid,
            TcpEntry->OwningModuleInfo,
            TCPIP_OWNER_MODULE_INFO_BASIC,
            Buffer,
            SizePointer);
    else
    {
        return STATUS_INVALID_PARAMETER;
    }
}

/**
 * Retrieves the owner module of an IPv6 UDP endpoint.
 *
 * \param UdpEntry The UDP table row that identifies the endpoint.
 * \param Class The owner module information class. Only TCPIP_OWNER_MODULE_INFO_BASIC is supported.
 * \param Buffer The buffer that receives the module information.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getownermodulefromudp6entry
 */
NTSTATUS PhGetOwnerModuleFromUdp6Entry(
    _In_ PMIB_UDP6ROW_OWNER_MODULE UdpEntry,
    _In_ TCPIP_OWNER_MODULE_INFO_CLASS Class,
    _Out_writes_bytes_opt_(*SizePointer) PVOID Buffer,
    _Inout_ PULONG SizePointer
    )
{
    if (UdpEntry && Class == TCPIP_OWNER_MODULE_INFO_BASIC && SizePointer)
        return PhpGetOwnerModuleFromPidAndInfo(
            UdpEntry->dwOwningPid,
            UdpEntry->OwningModuleInfo,
            TCPIP_OWNER_MODULE_INFO_BASIC,
            Buffer,
            SizePointer);
    else
    {
        return STATUS_INVALID_PARAMETER;
    }
}

/**
 * Retrieves an IP neighbor entry.
 *
 * \param Row On input, the key fields of the row; on output, the complete row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getipnetentry2
 */
NTSTATUS PhGetIpNetEntry2(
    _Inout_ PMIB_IPNET_ROW2 Row
    )
{
    // Resolve the entry through the neighbor table to reuse the verified NSI
    // key/ROD decode rather than duplicating the single-row layout.
    PMIB_IPNET_TABLE2 table = NULL;
    NTSTATUS status;
    ULONG i;

    if (!Row)
        return STATUS_INVALID_PARAMETER;

    if (Row->Address.si_family != AF_INET && Row->Address.si_family != AF_INET6)
        return STATUS_INVALID_PARAMETER;

    status = PhGetIpNetTable2(Row->Address.si_family, &table);

    if (!NT_SUCCESS(status))
        return status;

    status = STATUS_NOT_FOUND;

    for (i = 0; i < table->NumEntries; i++)
    {
        PMIB_IPNET_ROW2 entry = &table->Table[i];
        BOOLEAN addressMatch;

        if (entry->Address.si_family != Row->Address.si_family)
            continue;

        if (Row->Address.si_family == AF_INET)
        {
            addressMatch = entry->Address.Ipv4.sin_addr.S_un.S_addr ==
                Row->Address.Ipv4.sin_addr.S_un.S_addr;
        }
        else
        {
            addressMatch = memcmp(
                &entry->Address.Ipv6.sin6_addr,
                &Row->Address.Ipv6.sin6_addr,
                sizeof(IN6_ADDR)) == 0;
        }

        if (!addressMatch)
            continue;

        // Match the interface when the caller specified one (index or LUID).
        if (Row->InterfaceIndex && entry->InterfaceIndex != Row->InterfaceIndex)
            continue;
        if (Row->InterfaceLuid.Value && entry->InterfaceLuid.Value != Row->InterfaceLuid.Value)
            continue;

        *Row = *entry;
        status = STATUS_SUCCESS;
        break;
    }

    PhFree(table);
    return status;
}

/**
 * Retrieves the IP path table.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the allocated table. Free it with PhFreeMibTable/FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getippathtable
 */
NTSTATUS PhGetIpPathTable(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_IPPATH_TABLE *Table
    )
{
    struct
    {
        ADDRESS_FAMILY Family;
        PNPI_MODULEID ModuleId;
        ULONG KeySize;
        ULONG RodSize;
        PVOID Keys;
        PVOID Rod;
        ULONG Count;
    } families[2] = {
        { AF_INET, (PNPI_MODULEID)&NPI_MS_IPV4_MODULEID, sizeof(NSI_IP_PATH_KEY_V4), sizeof(NSI_IP_PATH_ROD_V4) },
        { AF_INET6, (PNPI_MODULEID)&NPI_MS_IPV6_MODULEID, sizeof(NSI_IP_PATH_KEY_V6), sizeof(NSI_IP_PATH_ROD_V6) }
    };
    NTSTATUS result;
    ULONG count;
    ULONG i;
    ULONG j;
    ULONG entryIndex;
    NET_IF_COMPARTMENT_ID compartmentId;
    PMIB_IPPATH_TABLE table;

    if (!Table || (Family != AF_UNSPEC && Family != AF_INET && Family != AF_INET6))
        return STATUS_INVALID_PARAMETER;

    compartmentId = PhGetCurrentThreadCompartmentId();
    count = 0;

    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (Family != AF_UNSPEC && Family != families[i].Family)
            continue;

        result = PhNsiAllocateAndGetTable(
            NsiActive,
            families[i].ModuleId,
            NsiIpObjectPath,
            &families[i].Keys,
            families[i].KeySize,
            NULL,
            0,
            &families[i].Rod,
            families[i].RodSize,
            NULL,
            0,
            &families[i].Count,
            0
            );

        if (result == STATUS_NOT_FOUND || result == STATUS_NOT_SUPPORTED)
            continue;

        if (result)
            goto Cleanup;

        for (j = 0; j < families[i].Count; j++)
        {
            const ULONG *key = PTR_ADD_OFFSET(families[i].Keys, (SIZE_T)j * families[i].KeySize);
            if (*key == compartmentId)
                count++;
        }
    }

    if (count > (SIZE_MAX - FIELD_OFFSET(MIB_IPPATH_TABLE, Table)) / sizeof(MIB_IPPATH_ROW))
    {
        result = STATUS_INTEGER_OVERFLOW;
        goto Cleanup;
    }

    table = PhAllocateZero(FIELD_OFFSET(MIB_IPPATH_TABLE, Table) + (SIZE_T)count * sizeof(MIB_IPPATH_ROW));
    entryIndex = 0;

    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        for (j = 0; j < families[i].Count; j++)
        {
            const VOID *key = PTR_ADD_OFFSET(families[i].Keys, (SIZE_T)j * families[i].KeySize);
            const VOID *rod = PTR_ADD_OFFSET(families[i].Rod, (SIZE_T)j * families[i].RodSize);

            if (*(const ULONG *)key != compartmentId)
                continue;

            PhpFillIpPathRow(families[i].Family, &table->Table[entryIndex], key, rod);
            entryIndex++;
        }
    }

    table->NumEntries = entryIndex;
    *Table = table;
    result = STATUS_SUCCESS;

Cleanup:
    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (families[i].Keys || families[i].Rod)
            PhNsiFreeTable(families[i].Keys, NULL, families[i].Rod, NULL);
    }

    return result;
}

/**
 * Retrieves the message for an IP_STATUS error code.
 *
 * \param ErrorCode The IP_STATUS error code.
 * \param Buffer The buffer that receives the message.
 * \param Size On input, the size of Buffer in characters; on output, the required size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getiperrorstring
 */
NTSTATUS PhGetIpErrorString(
    _In_ IP_STATUS ErrorCode,
    _Out_writes_opt_(*Size) PWSTR Buffer,
    _Inout_ PULONG Size
    )
{
    ULONG length;
    PWSTR message = NULL;

    if (!Size)
        return STATUS_INVALID_PARAMETER;

    length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        ErrorCode,
        0,
        (PWSTR)&message,
        0,
        NULL
        );

    if (!length)
        return PhpNetioDosErrorToNtStatus(PhGetLastError());

    if (!Buffer || *Size <= length)
    {
        *Size = length + 1;
        LocalFree(message);
        return STATUS_BUFFER_TOO_SMALL;
    }

    PhCopyStringZ(message, SIZE_MAX, Buffer, *Size, NULL);
    *Size = length;
    LocalFree(message);

    return STATUS_SUCCESS;
}

/**
 * Converts an interface index to a friendly (24-bit) index.
 *
 * \param IfIndex The interface index.
 * \return The friendly interface index.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getfriendlyifindex
 */
ULONG PhGetFriendlyIfIndex(
    _In_ ULONG IfIndex
    )
{
    return IfIndex & PHP_NET_LUID_INDEX_MASK;
}

/**
 * Retrieves the network compartment scope and identifier of the current thread.
 *
 * \param CompartmentScope Receives the compartment scope.
 * \param CompartmentId Receives the compartment identifier.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getcurrentthreadcompartmentscope
 */
VOID PhGetCurrentThreadCompartmentScope(
    _Out_ PNET_IF_COMPARTMENT_SCOPE CompartmentScope,
    _Out_ PNET_IF_COMPARTMENT_ID CompartmentId
    )
{
    typedef struct _PH_NETIO_CURRENT_THREAD_COMPARTMENT
    {
        NET_IF_COMPARTMENT_ID CompartmentId;
        NET_IF_COMPARTMENT_SCOPE CompartmentScope;
    } PH_NETIO_CURRENT_THREAD_COMPARTMENT;
    PH_NETIO_CURRENT_THREAD_COMPARTMENT currentCompartment;

    memset(&currentCompartment, 0, sizeof(PH_NETIO_CURRENT_THREAD_COMPARTMENT));

    if (PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectCurrentThreadCompartment,
        0,
        0,
        &currentCompartment,
        sizeof(PH_NETIO_CURRENT_THREAD_COMPARTMENT),
        0,
        0,
        0,
        0
        ))
    {
        *CompartmentScope = 0;
        if (CompartmentId)
            *CompartmentId = 1;
    }
    else
    {
        *CompartmentScope = currentCompartment.CompartmentScope;
        if (CompartmentId)
            *CompartmentId = currentCompartment.CompartmentId;
    }
}

/**
 * Retrieves the connectivity level and cost hint for a network interface using the Network List Manager.
 *
 * \param InterfaceIndex The local index of the network interface.
 * \param ConnectivityHint Receives the connectivity hint.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getnetworkconnectivityhintforinterface
 */
NTSTATUS PhGetNetworkConnectivityHintForInterface(
    _In_ NET_IFINDEX InterfaceIndex,
    _Out_ PNETIO_CONNECTIVITY_HINT ConnectivityHint
    )
{
    static const GUID networkListClass = { 0xA47979D2, 0xC419, 0x11D9, { 0xA5, 0xB4, 0x00, 0x11, 0x85, 0xAD, 0x2B, 0x89 } };
    static const GUID networkListInterface = { 0xDCB00000, 0x570F, 0x4A9B, { 0x8D, 0x69, 0x19, 0x9F, 0xDB, 0xA5, 0x72, 0x3B } };
    static const GUID networkConnectionCostInterface = { 0xDCB0000A, 0x570F, 0x4A9B, { 0x8D, 0x69, 0x19, 0x9F, 0xDB, 0xA5, 0x72, 0x3B } };
    INetworkListManager* manager = NULL;
    IEnumNetworkConnections* enumerator = NULL;
    INetworkConnection* connection = NULL;
    INetworkConnectionCost* connectionCost = NULL;
    NET_LUID interfaceLuid;
    GUID interfaceGuid;
    GUID adapterGuid;
    NLM_CONNECTIVITY connectivity;
    DWORD cost;
    ULONG fetched;
    NTSTATUS result;
    HRESULT hr;

    if (!ConnectivityHint)
        return STATUS_INVALID_PARAMETER;

    memset(ConnectivityHint, 0, sizeof(*ConnectivityHint));

    result = PhConvertInterfaceIndexToLuid(InterfaceIndex, &interfaceLuid);
    if (result) return result;
    result = PhConvertInterfaceLuidToGuid(&interfaceLuid, &interfaceGuid);
    if (result) return result;

    hr = CoCreateInstance(
        &networkListClass,
        NULL,
        CLSCTX_LOCAL_SERVER,
        &networkListInterface,
        (PVOID*)&manager
        );

    if (HR_SUCCESS(hr))
    {
        hr = INetworkListManager_GetNetworkConnections(manager, &enumerator);
    }

    if (HR_SUCCESS(hr))
    {
        for (;;)
        {
            fetched = 0;
            hr = IEnumNetworkConnections_Next(enumerator, 1, &connection, &fetched);
            if (FAILED(hr))
                break;
            if (hr == S_FALSE || !fetched)
            {
                hr = HRESULT_FROM_NT(STATUS_NOT_FOUND);
                break;
            }

            hr = INetworkConnection_GetAdapterId(connection, &adapterGuid);
            if (HR_SUCCESS(hr) && IsEqualGUID(&interfaceGuid, &adapterGuid))
                break;

            INetworkConnection_Release(connection);
            connection = NULL;
        }
    }

    if (HR_SUCCESS(hr) && connection)
    {
        hr = INetworkConnection_GetConnectivity(connection, &connectivity);
        if (HR_SUCCESS(hr))
            hr = INetworkConnection_QueryInterface(connection, &networkConnectionCostInterface, (PVOID*)&connectionCost);
        if (HR_SUCCESS(hr))
            hr = INetworkConnectionCost_GetCost(connectionCost, &cost);

        if (HR_SUCCESS(hr))
        {
            if (connectivity & (NLM_CONNECTIVITY_IPV4_INTERNET | NLM_CONNECTIVITY_IPV6_INTERNET))
                ConnectivityHint->ConnectivityLevel = NetworkConnectivityLevelHintInternetAccess;
            else if (connectivity & (NLM_CONNECTIVITY_IPV4_SUBNET | NLM_CONNECTIVITY_IPV4_LOCALNETWORK | NLM_CONNECTIVITY_IPV6_SUBNET | NLM_CONNECTIVITY_IPV6_LOCALNETWORK))
                ConnectivityHint->ConnectivityLevel = NetworkConnectivityLevelHintLocalAccess;
            else
                ConnectivityHint->ConnectivityLevel = connectivity ? NetworkConnectivityLevelHintNone : NetworkConnectivityLevelHintUnknown;

            if (cost & NLM_CONNECTION_COST_VARIABLE)
                ConnectivityHint->ConnectivityCost = NetworkConnectivityCostHintVariable;
            else if (cost & NLM_CONNECTION_COST_FIXED)
                ConnectivityHint->ConnectivityCost = NetworkConnectivityCostHintFixed;
            else if (cost & NLM_CONNECTION_COST_UNRESTRICTED)
                ConnectivityHint->ConnectivityCost = NetworkConnectivityCostHintUnrestricted;
            else
                ConnectivityHint->ConnectivityCost = NetworkConnectivityCostHintUnknown;

            ConnectivityHint->ApproachingDataLimit = !!(cost & NLM_CONNECTION_COST_APPROACHINGDATALIMIT);
            ConnectivityHint->OverDataLimit = !!(cost & NLM_CONNECTION_COST_OVERDATALIMIT);
            ConnectivityHint->Roaming = !!(cost & NLM_CONNECTION_COST_ROAMING);
        }
    }

    if (connectionCost)
        INetworkConnectionCost_Release(connectionCost);
    if (connection)
        INetworkConnection_Release(connection);
    if (enumerator)
        IEnumNetworkConnections_Release(enumerator);
    if (manager)
        manager->lpVtbl->Release(manager);

    return HR_SUCCESS(hr) ? STATUS_SUCCESS : PhNtStatusFromHResult(hr);
}

/**
 * Queues the connectivity hint thread pool work unless it is already pending.
 *
 * \param Context The connectivity hint registration.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-submitthreadpoolwork
 */
static VOID PhpNotifyClientAsync(
    _Inout_ PPHP_CONNECTIVITY_HINT_CONTEXT Context
    )
{
    if (InterlockedExchange(&Context->State, 1) == 0)
    {
        TpPostWork(Context->ThreadpoolWork);
    }
}

/**
 * Thread pool callback that delivers the current connectivity hint to the registered callback.
 *
 * \param Instance The thread pool callback instance.
 * \param Context The connectivity hint registration.
 * \param Work The thread pool work object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-submitthreadpoolwork
 */
static void NTAPI PhpConnectivityThreadpoolWorkCallback(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _Inout_ PTP_WORK Work
    )
{
    PPHP_CONNECTIVITY_HINT_CONTEXT ctx = (PPHP_CONNECTIVITY_HINT_CONTEXT)Context;
    PNET_NETWORK_CONNECTIVITY_HINT_CHANGE_CALLBACK callback = (PNET_NETWORK_CONNECTIVITY_HINT_CHANGE_CALLBACK)ctx->Callback;
    NETIO_CONNECTIVITY_HINT hint;

    TpCallbackMayRunLong(Instance);
    do
    {
        PhAcquireQueuedLockExclusive(&PhpConnectivityHintLock);
        hint.ConnectivityLevel = PhpConnectivityLevel;
        hint.ConnectivityCost = PhpConnectivityCost;
        InterlockedExchange(&ctx->State, PHP_CONNECTIVITY_CALLBACK_PROCESSING);
        PhReleaseQueuedLockExclusive(&PhpConnectivityHintLock);

        callback(ctx->CallerContext, &hint);
    }
    while (InterlockedCompareExchange(&ctx->State, 0, PHP_CONNECTIVITY_CALLBACK_PROCESSING) == 1);
}

/**
 * Converts the NCSI connectivity WNF state into a NETIO_CONNECTIVITY_HINT.
 *
 * \param State The 64-bit connectivity state.
 * \param Hint Receives the connectivity hint.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getnetworkconnectivityhint
 */
static void PhpConnectivityHintFromConnectivityState(
    _In_ PVOID State,
    _Out_ PNETIO_CONNECTIVITY_HINT Hint
    )
{
    ULONG64 val = *(PULONG64)State;
    ULONG level;
    NL_NETWORK_CONNECTIVITY_COST_HINT cost = NetworkConnectivityCostHintVariable;

    memset(Hint, 0, sizeof(NETIO_CONNECTIVITY_HINT));

    if ((val & PHP_NSI_CONNECTIVITY_CONSTRAINED_MASK) != 0)
        level = NetworkConnectivityLevelHintConstrainedInternetAccess;
    else if ((val & PHP_NSI_CONNECTIVITY_INTERNET_MASK) != 0)
        level = NetworkConnectivityLevelHintInternetAccess;
    else if ((val & PHP_NSI_CONNECTIVITY_LOCAL_MASK) != 0)
        level = NetworkConnectivityLevelHintLocalAccess;
    else
        level = (ULONG)val != 0 ? NetworkConnectivityLevelHintNone : NetworkConnectivityLevelHintUnknown;

    Hint->ConnectivityLevel = level;

    UCHAR byte4 = (UCHAR)(val >> PHP_NSI_CONNECTIVITY_COST_SHIFT);
    if ((byte4 & PHP_NSI_CONNECTIVITY_COST_UNDETERMINED) == 0)
    {
        if ((byte4 & PHP_NSI_CONNECTIVITY_COST_FIXED) != 0)
            cost = NetworkConnectivityCostHintFixed;
        else
            cost = (byte4 & PHP_NSI_CONNECTIVITY_COST_UNRESTRICTED) ? NetworkConnectivityCostHintUnrestricted : NetworkConnectivityCostHintUnknown;
    }
    Hint->ConnectivityCost = cost;

}

/**
 * WNF callback that updates the cached connectivity hint and notifies the registered clients.
 *
 * \param StateName The WNF state name.
 * \param ChangeStamp The change stamp of the state.
 * \param TypeId The type identifier of the state.
 * \param CallbackContext Unused.
 * \param Buffer The state data.
 * \param Length The size, in bytes, of Buffer.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-notifynetworkconnectivityhintchange
 */
static NTSTATUS NTAPI PhpConnectivityStateChangeCallback(
    _In_ WNF_STATE_NAME StateName,
    _In_ WNF_CHANGE_STAMP ChangeStamp,
    _In_opt_ PWNF_TYPE_ID TypeId,
    _In_opt_ PVOID CallbackContext,
    _In_reads_bytes_opt_(Length) const VOID* Buffer,
    _In_ ULONG Length
    )
{
    NETIO_CONNECTIVITY_HINT hint;

    if (Buffer && Length == sizeof(ULONG64))
    {
        PhpConnectivityHintFromConnectivityState((PVOID)Buffer, &hint);

        PhAcquireQueuedLockExclusive(&PhpConnectivityHintLock);
        BOOLEAN changed = (hint.ConnectivityLevel != PhpConnectivityLevel || hint.ConnectivityCost != PhpConnectivityCost);
        if (changed)
        {
            PhpConnectivityLevel = (ULONG)hint.ConnectivityLevel;
            PhpConnectivityCost = hint.ConnectivityCost;

            PLIST_ENTRY entry = PhpConnectivityHintListHead.Flink;
            while (entry != &PhpConnectivityHintListHead)
            {
                PPHP_CONNECTIVITY_HINT_CONTEXT ctx = CONTAINING_RECORD(entry, PHP_CONNECTIVITY_HINT_CONTEXT, ListEntry);
                PhpNotifyClientAsync(ctx);
                entry = entry->Flink;
            }
        }
        PhReleaseQueuedLockExclusive(&PhpConnectivityHintLock);
    }
    return STATUS_SUCCESS;
}

/**
 * Seeds the cached connectivity hint from the NCSI connectivity WNF state and subscribes to its changes
 * (iphlpapi!MonitorConnectivityState). The subscription is created once and kept for the process lifetime.
 * \remarks The caller holds PhpConnectivityHintLock exclusively.
 */
static NTSTATUS PhpMonitorConnectivityState(void)
{
    //WNF_STATE_NAME stateName = { .Value = PHP_WNF_CONNECTIVITY_STATE_NAME };
    //WNF_CHANGE_STAMP changeStamp = 0;
    //ULONG64 stateData = 0;
    //ULONG stateDataLength = sizeof(stateData);
    //NETIO_CONNECTIVITY_HINT hint;
    //NTSTATUS status;
    //
    //if (PhpConnectivitySubscriptionHandle)
    //    return STATUS_SUCCESS;
    //
    //// A failed query leaves the state zeroed, which maps to an unknown level (as iphlpapi does).
    //status = NtQueryWnfStateData(
    //    &stateName,
    //    NULL,
    //    NULL,
    //    &changeStamp,
    //    &stateData,
    //    &stateDataLength
    //    );
    //
    //if (!NT_SUCCESS(status) || stateDataLength != sizeof(stateData))
    //    stateData = 0;
    //
    //PhpConnectivityHintFromConnectivityState(&stateData, &hint);
    //PhpConnectivityLevel = (ULONG)hint.ConnectivityLevel;
    //PhpConnectivityCost = hint.ConnectivityCost;
    //
    //// Subscribing from the queried change stamp delivers only later changes.
    //status = RtlSubscribeWnfStateChangeNotification(
    //    (PWNF_USER_SUBSCRIPTION*)&PhpConnectivitySubscriptionHandle,
    //    stateName,
    //    changeStamp,
    //    PhpConnectivityStateChangeCallback,
    //    NULL,
    //    NULL,
    //    0,
    //    0
    //    );
    //
    //if (!NT_SUCCESS(status))
    //{
    //    PhpConnectivitySubscriptionHandle = NULL;
    //    return status;
    //}

    return STATUS_SUCCESS;
}

/**
 * Registers to be notified when the network connectivity hint changes.
 *
 * \param Callback The callback invoked on each change.
 * \param CallerContext A caller-defined context passed to the callback.
 * \param InitialNotification TRUE to invoke the callback immediately after registration.
 * \param NotificationHandle Receives the notification handle. Cancel it with PhCancelMibChangeNotify2.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-notifynetworkconnectivityhintchange
 */
NTSTATUS NTAPI PhNotifyNetworkConnectivityHintChange(
    _In_ PNET_NETWORK_CONNECTIVITY_HINT_CHANGE_CALLBACK Callback,
    _In_opt_ PVOID CallerContext,
    _In_ BOOLEAN InitialNotification,
    _Out_ PHANDLE NotificationHandle
    )
{
    PPHP_MIB_NOTIFICATION_HANDLE handle;
    NTSTATUS lastError;
    PPHP_CONNECTIVITY_HINT_CONTEXT context;

    *NotificationHandle = NULL;

    handle = PhAllocate(sizeof(PHP_MIB_NOTIFICATION_HANDLE));
    if (!handle)
        return STATUS_NO_MEMORY;

    context = PhAllocate(sizeof(PHP_CONNECTIVITY_HINT_CONTEXT));
    if (!context)
    {
        PhFree(handle);
        return STATUS_NO_MEMORY;
    }

    context->Callback = Callback;
    context->CallerContext = CallerContext;
    context->State = 0;

    NTSTATUS status = TpAllocWork(
        &context->ThreadpoolWork,
        (PTP_WORK_CALLBACK)PhpConnectivityThreadpoolWorkCallback,
        context,
        NULL
        );
    if (!NT_SUCCESS(status))
    {
        PhFree(context);
        PhFree(handle);
        return status;
    }

    PhAcquireQueuedLockExclusive(&PhpConnectivityHintLock);
    lastError = PhpMonitorConnectivityState();
    if (!NT_SUCCESS(lastError))
    {
        PhReleaseQueuedLockExclusive(&PhpConnectivityHintLock);
        TpReleaseWork(context->ThreadpoolWork);
        PhFree(context);
        PhFree(handle);
        return lastError;
    }

    InsertTailList(&PhpConnectivityHintListHead, &context->ListEntry);

    if (InitialNotification)
        PhpNotifyClientAsync(context);

    handle->ConnectivityHintContext = context;
    *NotificationHandle = handle;
    PhReleaseQueuedLockExclusive(&PhpConnectivityHintLock);

    return STATUS_SUCCESS;
}

/**
 * Retrieves the unicast address table once it is stable, or registers to be notified when it is.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the table if it is already stable.
 * \param CallerCallback The callback invoked when the table becomes stable.
 * \param CallerContext A caller-defined context passed to the callback.
 * \param NotificationHandle Receives the notification handle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-notifystableunicastipaddresstable
 */
NTSTATUS PhNotifyStableUnicastIpAddressTable(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_result_maybenull_ PMIB_UNICASTIPADDRESS_TABLE *Table,
    _In_opt_ PSTABLE_UNICAST_IPADDRESS_TABLE_CALLBACK CallerCallback,
    _In_opt_ PVOID CallerContext,
    _Out_ PHANDLE NotificationHandle
    )
{
    return PhpNotifyStableUnicastIpAddressTable(
        Family,
        Table,
        Table,
        CallerCallback,
        CallerContext,
        NotificationHandle);
}

typedef struct _PHP_NSI_CHANGE_DATA
{
    PVOID Reserved;
    PVOID Data;
} PHP_NSI_CHANGE_DATA, *PPHP_NSI_CHANGE_DATA;

typedef struct _PHP_TEREDO_PORT_CHANGE_CONTEXT
{
    PTEREDO_PORT_CHANGE_CALLBACK Callback;
    PVOID CallerContext;
} PHP_TEREDO_PORT_CHANGE_CONTEXT, *PPHP_TEREDO_PORT_CHANGE_CONTEXT;

/**
 * NSI callback that forwards Teredo port changes to the caller.
 *
 * \param Context The Teredo registration.
 * \param ChangeKey The key of the object that changed.
 * \param ChangeData The changed data.
 * \param NotificationType The type of change.
 * \return ERROR_SUCCESS (NO_ERROR) on success, otherwise a Win32 error code.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-notifyteredoportchange
 */
static ULONG NTAPI PhpTeredoPortChangeCallback(
    _In_ PPHP_TEREDO_PORT_CHANGE_CONTEXT Context,
    _In_opt_ PVOID ChangeKey,
    _In_opt_ PPHP_NSI_CHANGE_DATA ChangeData,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    USHORT port = MIB_INVALID_TEREDO_PORT_NUMBER;

    if (ChangeData && ChangeData->Data)
        port = *(PUSHORT)ChangeData->Data;

    Context->Callback(Context->CallerContext, port, NotificationType);
    return ERROR_SUCCESS;
}

/**
 * Registers to be notified of changes to the Teredo client port.
 *
 * \param Callback The callback invoked on each change.
 * \param CallerContext A caller-defined context passed to the callback.
 * \param InitialNotification TRUE to invoke the callback immediately after registration.
 * \param NotificationHandle Receives the notification handle. Cancel it with PhCancelMibChangeNotify2.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-notifyteredoportchange
 */
NTSTATUS PhNotifyTeredoPortChange(
    _In_ PTEREDO_PORT_CHANGE_CALLBACK Callback,
    _In_opt_ PVOID CallerContext,
    _In_ BOOLEAN InitialNotification,
    _Out_ PHANDLE NotificationHandle
    )
{
    PNETIO_TEREDO_PORT_CHANGE_REGISTRATION registration;
    PVOID rpcHandle;
    NTSTATUS result;

    if (!NotificationHandle || !Callback)
        return STATUS_INVALID_PARAMETER;

    registration = PhAllocateZero(sizeof(NETIO_TEREDO_PORT_CHANGE_REGISTRATION));
    registration->Header.ObjectId = NsiFl6tObjectClientPort;
    registration->Header.Ipv4CallbackContext = &registration->Callback;
    registration->Callback = Callback;
    registration->CallerContext = CallerContext;

    if (!(rpcHandle = PhNsiNotificationConnectToServer()))
    {
        PhFree(registration);
        return STATUS_INVALID_HANDLE;
    }

    result = PhNsiRpcRegisterChangeNotification(
        rpcHandle,
        &NPI_MS_FL6T_MODULEID,
        NsiFl6tObjectClientPort,
        PhpTeredoPortChangeCallback,
        InitialNotification,
        &registration->Callback,
        &registration->Header.TeredoNotificationHandle
        );

    if (result)
    {
        PhFree(registration);
        return result;
    }

    *NotificationHandle = registration;
    return STATUS_SUCCESS;
}

PIP_ADAPTER_ORDER_MAP PhGetAdapterOrderMap()
{
    ULONG count;
    NET_IF_COMPARTMENT_ID currentCompartmentId;
    NDIS_NSI_INTERFACE_ENUM_ROD* aliasTable = NULL;
    NDIS_NSI_INTERFACE_ENUM_ROS* guidTable = NULL;
    NET_LUID* keyTable = NULL;
    ULONG activeCount = 0;
    PIP_ADAPTER_ORDER_MAP orderMap;

    if (PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceWithoutStatistics,
        &keyTable,
        sizeof(NET_LUID),
        NULL,
        0,
        &aliasTable,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROD),
        &guidTable,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROS),
        &count,
        0
        ))
    {
        return NULL;
    }

    currentCompartmentId = PhGetCurrentThreadCompartmentId();

    for (ULONG i = 0; i < count; i++)
    {
        if (aliasTable[i].CompartmentId == currentCompartmentId)
        {
            activeCount++;
        }
    }

    orderMap = LocalAlloc(0, FIELD_OFFSET(IP_ADAPTER_ORDER_MAP, AdapterOrder) + sizeof(ULONG) * activeCount);
    if (orderMap)
    {
        ULONG j = 0;
        for (ULONG i = 0; i < count; i++)
        {
            if (aliasTable[i].CompartmentId == currentCompartmentId)
            {
                orderMap->AdapterOrder[j++] = guidTable[i].ifIndex;
            }
        }
        orderMap->NumAdapters = j;
    }

    PhNsiFreeTable(keyTable, NULL, aliasTable, guidTable);
    
    return orderMap;
}

/**
 * Retrieves legacy IPv4 adapter information.
 *
 * \param AdapterInfo The buffer that receives the adapter list.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersinfo
 */
NTSTATUS PhGetAdaptersInfo(
    _Out_writes_bytes_opt_(*SizePointer) PIP_ADAPTER_INFO AdapterInfo,
    _Inout_ PULONG SizePointer
    )
{
    // Build the legacy adapter view from GetAdaptersAddresses.
    // GetAdaptersInfo reports IPv4 adapters in a single contiguous caller buffer;
    // the additional IP_ADDR_STRING entries chain into the trailing area of that buffer.
    static const IN_ADDR broadcastMask = { .S_un = { .S_addr = INADDR_NONE } }; // 255.255.255.255

    NTSTATUS result;
    PIP_ADAPTER_ADDRESSES addresses = NULL;
    PIP_ADAPTER_ADDRESSES adapter;
    ULONG adapterCount = 0;
    ULONG extraNodeCount = 0;
    SIZE_T requiredSize;
    PIP_ADAPTER_INFO infoArray;
    PIP_ADDR_STRING nodePool;
    ULONG nodeIndex;
    ULONG infoIndex;
    PIP_ADAPTER_INFO previousInfo;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    result = PhpAllocateAndGetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, &addresses, NULL);

    if (!NT_SUCCESS(result))
        return result;

    // Pass 1: count qualifying adapters (those with at least one IPv4 unicast address)
    // and the additional IP_ADDR_STRING nodes needed beyond the inline members.
    for (adapter = addresses; adapter; adapter = adapter->Next)
    {
        ULONG unicastCount = 0;
        ULONG gatewayCount = 0;
        PIP_ADAPTER_UNICAST_ADDRESS unicast;
        PIP_ADAPTER_GATEWAY_ADDRESS gateway;

        for (unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
        {
            if (unicast->Address.lpSockaddr && unicast->Address.lpSockaddr->sa_family == AF_INET)
                unicastCount++;
        }

        if (unicastCount == 0)
            continue;

        for (gateway = adapter->FirstGatewayAddress; gateway; gateway = gateway->Next)
        {
            if (gateway->Address.lpSockaddr && gateway->Address.lpSockaddr->sa_family == AF_INET)
                gatewayCount++;
        }

        adapterCount++;
        if (unicastCount > 1)
            extraNodeCount += unicastCount - 1;
        if (gatewayCount > 1)
            extraNodeCount += gatewayCount - 1;
    }

    if (adapterCount == 0)
    {
        PhpFreeAdaptersAddresses(addresses);
        return STATUS_NO_MORE_ENTRIES;
    }

    requiredSize = (SIZE_T)adapterCount * sizeof(IP_ADAPTER_INFO) +
        (SIZE_T)extraNodeCount * sizeof(IP_ADDR_STRING);

    if (!AdapterInfo || *SizePointer < requiredSize)
    {
        *SizePointer = (ULONG)requiredSize;
        PhpFreeAdaptersAddresses(addresses);
        return STATUS_BUFFER_TOO_SMALL;
    }

    memset(AdapterInfo, 0, requiredSize);

    infoArray = AdapterInfo;
    nodePool = (PIP_ADDR_STRING)((PUCHAR)AdapterInfo + (SIZE_T)adapterCount * sizeof(IP_ADAPTER_INFO));
    nodeIndex = 0;
    infoIndex = 0;
    previousInfo = NULL;

    // Pass 2: fill.
    for (adapter = addresses; adapter; adapter = adapter->Next)
    {
        PIP_ADAPTER_INFO info;
        PIP_ADAPTER_UNICAST_ADDRESS unicast;
        PIP_ADAPTER_GATEWAY_ADDRESS gateway;
        PIP_ADDR_STRING addressTail = NULL;
        PIP_ADDR_STRING gatewayTail = NULL;
        ULONG unicastCount = 0;
        ULONG physicalLength;

        for (unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
        {
            if (unicast->Address.lpSockaddr && unicast->Address.lpSockaddr->sa_family == AF_INET)
                unicastCount++;
        }

        if (unicastCount == 0)
            continue;

        info = &infoArray[infoIndex++];

        if (previousInfo)
            previousInfo->Next = info;
        previousInfo = info;

        if (adapter->AdapterName)
        {
            PhCopyBytesZ(
                adapter->AdapterName,
                SIZE_MAX,
                info->AdapterName,
                RTL_NUMBER_OF(info->AdapterName),
                NULL
                );
        }

        if (adapter->Description)
        {
            PhConvertUtf16ToUtf8Buffer(
                info->Description,
                sizeof(info->Description),
                NULL,
                adapter->Description,
                PhCountStringZ(adapter->Description) * sizeof(WCHAR)
                );
        }

        physicalLength = adapter->PhysicalAddressLength;
        if (physicalLength > MAX_ADAPTER_ADDRESS_LENGTH)
            physicalLength = MAX_ADAPTER_ADDRESS_LENGTH;
        info->AddressLength = physicalLength;
        memcpy(info->Address, adapter->PhysicalAddress, physicalLength);

        info->Index = adapter->IfIndex;
        info->Type = adapter->IfType;
        info->DhcpEnabled = (adapter->Flags & IP_ADAPTER_DHCP_ENABLED) ? 1 : 0;

        // IpAddressList: first inline, extras chained into the trailing node pool.
        addressTail = &info->IpAddressList;
        for (unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
        {
            IN_ADDR address;
            IN_ADDR mask;

            if (!unicast->Address.lpSockaddr || unicast->Address.lpSockaddr->sa_family != AF_INET)
                continue;

            address = ((struct sockaddr_in *)unicast->Address.lpSockaddr)->sin_addr;
            if (PhConvertLengthToIpv4Mask(unicast->OnLinkPrefixLength, &mask.S_un.S_addr) != STATUS_SUCCESS)
                mask.S_un.S_addr = 0;

            if (addressTail == &info->IpAddressList && info->IpAddressList.IpAddress.String[0] == UNICODE_NULL)
            {
                PhpSetAddressString(&info->IpAddressList, &address, &mask);
            }
            else
            {
                PIP_ADDR_STRING node = &nodePool[nodeIndex++];
                PhpSetAddressString(node, &address, &mask);
                addressTail->Next = node;
                addressTail = node;
            }
        }

        info->CurrentIpAddress = &info->IpAddressList;

        // GatewayList: first inline, extras chained. Gateways use a zero mask.
        gatewayTail = &info->GatewayList;
        for (gateway = adapter->FirstGatewayAddress; gateway; gateway = gateway->Next)
        {
            IN_ADDR address;
            IN_ADDR mask;

            if (!gateway->Address.lpSockaddr || gateway->Address.lpSockaddr->sa_family != AF_INET)
                continue;

            address = ((struct sockaddr_in *)gateway->Address.lpSockaddr)->sin_addr;
            mask.S_un.S_addr = 0;

            if (gatewayTail == &info->GatewayList && info->GatewayList.IpAddress.String[0] == UNICODE_NULL)
            {
                PhpSetAddressString(&info->GatewayList, &address, &mask);
            }
            else
            {
                PIP_ADDR_STRING node = &nodePool[nodeIndex++];
                PhpSetAddressString(node, &address, &mask);
                gatewayTail->Next = node;
                gatewayTail = node;
            }
        }

        // DhcpServer (single entry, broadcast mask).
        if (info->DhcpEnabled &&
            adapter->Dhcpv4Server.lpSockaddr &&
            adapter->Dhcpv4Server.lpSockaddr->sa_family == AF_INET)
        {
            IN_ADDR address = ((struct sockaddr_in *)adapter->Dhcpv4Server.lpSockaddr)->sin_addr;
            PhpSetAddressString(&info->DhcpServer, &address, (PIN_ADDR)&broadcastMask);
        }

        // WINS and lease timers are not exposed by PhGetAdaptersAddresses.
        info->HaveWins = FALSE;
    }

    PhpFreeAdaptersAddresses(addresses);
    return STATUS_SUCCESS;
}

/**
 * Retrieves the IPv4 addresses of receive-only (unidirectional) adapters.
 *
 * \param IpIfInfo The buffer that receives the addresses.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getunidirectionaladapterinfo
 */
NTSTATUS PhGetUniDirectionalAdapterInfo(
    _Out_writes_bytes_opt_(*SizePointer) PIP_UNIDIRECTIONAL_ADAPTER_ADDRESS IpIfInfo,
    _Inout_ PULONG SizePointer
    )
{
    NTSTATUS result;
    ULONG count;
    ULONG requiredSize;
    PIP_ADAPTER_ADDRESSES addresses = NULL;
    PIP_ADAPTER_ADDRESSES adapter;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    result = PhpAllocateAndGetAdaptersAddresses(
        AF_INET,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
        &addresses,
        NULL);

    if (result)
        return result;

    count = 0;
    for (adapter = addresses; adapter; adapter = adapter->Next)
    {
        PIP_ADAPTER_UNICAST_ADDRESS unicast;

        if (!(adapter->Flags & IP_ADAPTER_RECEIVE_ONLY) || adapter->OperStatus != IfOperStatusUp)
            continue;

        for (unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
        {
            if (unicast->DadState >= IpDadStateDeprecated)
            {
                count++;
                break;
            }
        }
    }

    if (count > (ULONG_MAX - sizeof(IP_UNIDIRECTIONAL_ADAPTER_ADDRESS)) / sizeof(IPAddr))
    {
        PhpFreeAdaptersAddresses(addresses);
        return STATUS_INTEGER_OVERFLOW;
    }

    requiredSize = sizeof(IP_UNIDIRECTIONAL_ADAPTER_ADDRESS) + count * sizeof(IPAddr);

    if (!IpIfInfo || *SizePointer < requiredSize)
    {
        if (IpIfInfo && *SizePointer >= sizeof(IP_UNIDIRECTIONAL_ADAPTER_ADDRESS))
            IpIfInfo->NumAdapters = count;
        *SizePointer = requiredSize;
        PhpFreeAdaptersAddresses(addresses);
        return STATUS_BUFFER_OVERFLOW;
    }

    IpIfInfo->NumAdapters = count;
    count = 0;
    for (adapter = addresses; adapter; adapter = adapter->Next)
    {
        PIP_ADAPTER_UNICAST_ADDRESS unicast;

        if (!(adapter->Flags & IP_ADAPTER_RECEIVE_ONLY) || adapter->OperStatus != IfOperStatusUp)
            continue;

        for (unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
        {
            if (unicast->DadState >= IpDadStateDeprecated)
            {
                if (unicast->Address.lpSockaddr && unicast->Address.lpSockaddr->sa_family == AF_INET)
                    IpIfInfo->Address[count++] = ((PSOCKADDR_IN)unicast->Address.lpSockaddr)->sin_addr.S_un.S_addr;
                break;
            }
        }
    }

    IpIfInfo->NumAdapters = count;
    PhpFreeAdaptersAddresses(addresses);
    return STATUS_SUCCESS;
}

/**
 * Retrieves the DNS settings of an interface.
 *
 * \param Interface The GUID of the network interface.
 * \param Settings On input, the version of the structure; on output, the settings.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getinterfacednssettings
 */
NTSTATUS PhGetInterfaceDnsSettings(
    _In_ GUID *Interface,
    _Inout_ DNS_INTERFACE_SETTINGS *Settings
    )
{
    if (Settings &&
        Settings->Version >= DNS_INTERFACE_SETTINGS_VERSION1 &&
        Settings->Version <= DNS_INTERFACE_SETTINGS_VERSION4)
    {
        return PhDnsGetInterfaceSettings(Interface, Settings);
    }
    
    return STATUS_INVALID_PARAMETER;
}

/**
 * Retrieves the network parameters of the local computer.
 *
 * \param FixedInfo The buffer that receives the parameters.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getnetworkparams
 */
NTSTATUS PhGetNetworkParams(
    _Out_writes_bytes_opt_(*SizePointer) PFIXED_INFO FixedInfo,
    _Inout_ PULONG SizePointer
    )
{
    //ULONG status;
    //PVOID memory = NULL;
    //
    //if (!SizePointer)
    //    return STATUS_INVALID_PARAMETER;
    //
    //status = PhAllocateAndGetFixedInfo(&memory);
    //if (!NT_SUCCESS(status))
    //    return status;
    //
    //PFIXED_INFO sourceFixedInfo = (PFIXED_INFO)memory;
    //PIP_ADDR_STRING dnsListHead = &sourceFixedInfo->DnsServerList;
    //
    //ULONG dnsListSize = PhGetSizeofAddressStringList(dnsListHead);
    //ULONG sizeRequired = sizeof(FIXED_INFO) + dnsListSize;
    //
    //if (!FixedInfo || *SizePointer < sizeRequired)
    //{
    //    *SizePointer = sizeRequired;
    //    PhFreeAddressStringList(dnsListHead);
    //    PhFree(memory);
    //    return STATUS_BUFFER_TOO_SMALL;
    //}
    //
    //memset(FixedInfo, 0, *SizePointer);
    //memcpy(FixedInfo, memory, sizeof(FIXED_INFO));
    //
    //PIP_ADDR_STRING dest = (PIP_ADDR_STRING)(FixedInfo + 1);
    //PIP_ADDR_STRING src = dnsListHead->Next;
    //PIP_ADDR_STRING tail = &FixedInfo->DnsServerList;
    //
    //FixedInfo->DnsServerList.Next = NULL;
    //
    //while (src)
    //{
    //    tail->Next = dest;
    //    memcpy(dest, src, sizeof(IP_ADDR_STRING));
    //    tail = dest;
    //    dest = (PIP_ADDR_STRING)((PCHAR)dest + sizeof(IP_ADDR_STRING));
    //    src = src->Next;
    //}
    //
    //if (tail)
    //    tail->Next = NULL;
    //
    //PhFreeAddressStringList(dnsListHead);
    //PhFree(memory);
    return STATUS_SUCCESS;
}

/**
 * Retrieves the aggregate connectivity level and cost hint of the system.
 *
 * \param ConnectivityHint Receives the connectivity hint.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getnetworkconnectivityhint
 */
NTSTATUS PhGetNetworkConnectivityHint(
    _Out_ PNETIO_CONNECTIVITY_HINT ConnectivityHint
    )
{
    if (!ConnectivityHint)
        return STATUS_INVALID_PARAMETER;

    PhAcquireQueuedLockExclusive(&PhpConnectivityHintLock);
    ConnectivityHint->ConnectivityLevel = PhpConnectivityLevel;
    ConnectivityHint->ConnectivityCost = PhpConnectivityCost;
    PhReleaseQueuedLockExclusive(&PhpConnectivityHintLock);

    return STATUS_SUCCESS;
}

/**
 * Retrieves the active timestamp configuration of a network interface.
 *
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param TimestampCapabilities Receives the timestamp capabilities.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getinterfaceactivetimestampcapabilities
 */
NTSTATUS PhGetInterfaceActiveTimestampCapabilities(
    _In_ const NET_LUID* InterfaceLuid,
    _Out_ INTERFACE_TIMESTAMP_CAPABILITIES* TimestampCapabilities
    )
{
    if (TimestampCapabilities)
        return PhGetInterfaceTimestampCapabilities(InterfaceLuid, NETIO_OID_TIMESTAMP_CURRENT_CONFIG, TimestampCapabilities);
    return STATUS_INVALID_PARAMETER;
}

/**
 * Retrieves the supported timestamp capabilities of a network interface.
 *
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param TimestampCapabilities Receives the timestamp capabilities.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getinterfacesupportedtimestampcapabilities
 */
NTSTATUS PhGetInterfaceSupportedTimestampCapabilities(
    _In_ const NET_LUID* InterfaceLuid,
    _Out_ INTERFACE_TIMESTAMP_CAPABILITIES* TimestampCapabilities
    )
{
    if (TimestampCapabilities)
        return PhGetInterfaceTimestampCapabilities(InterfaceLuid, NETIO_OID_TIMESTAMP_CAPABILITY, TimestampCapabilities);
    return STATUS_INVALID_PARAMETER;
}

/**
 * Captures a hardware cross timestamp from a network interface.
 *
 * \param InterfaceIndex The local index of the network interface.
 * \param Timestamp Receives the cross timestamp.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-captureinterfacehardwarecrosstimestamp
 */
NTSTATUS PhCaptureInterfaceHardwareCrossTimestamp(
    _In_ NET_IFINDEX InterfaceIndex,
    _Out_ PNETIO_INTERFACE_HARDWARE_CROSSTIMESTAMP Timestamp
    )
{
    NTSTATUS result;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE deviceHandle;
    NET_LUID interfaceLuid;
    NETIO_NDIS_CROSSTIMESTAMP_REQUEST request = { 0 };
    NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA crossTimestamp;

    if (!Timestamp)
        return STATUS_INVALID_PARAMETER;

    deviceHandle = NULL;
    result = PhConvertInterfaceIndexToLuid(InterfaceIndex, &interfaceLuid);

    if (result)
        return result;

    result = PhGetInterfaceHandle(&interfaceLuid, &deviceHandle);

    if (result)
        return result;

    RtlZeroMemory(Timestamp, sizeof(*Timestamp));

    request.Header.Type = NETIO_NDIS_CROSSTIMESTAMP_REQUEST_TYPE;
    request.Header.Revision = NETIO_NDIS_CROSSTIMESTAMP_REQUEST_REVISION;
    request.Header.Size = FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Data);
    request.Oid = NETIO_OID_TIMESTAMP_GET_CROSSTIMESTAMP;
    request.DataOffset = FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Data);

    result = NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        IOCTL_NETIO_NDIS_CROSSTIMESTAMP,
        &request,
        sizeof(request),
        &request,
        sizeof(request)
        );

    if (result == STATUS_PENDING)
    {
        result = NtWaitForSingleObject(deviceHandle, FALSE, NULL);

        if (NT_SUCCESS(result))
            result = ioStatusBlock.Status;
    }

    if (NT_SUCCESS(result))
    {
        result = PhConvertNdisStatusToError(request.Status);

        if (!result)
        {
            if (request.DataOffset < FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Data) ||
                request.DataOffset > sizeof(request) - sizeof(crossTimestamp))
            {
                result = STATUS_DATA_ERROR;
            }
            else
            {
                memcpy(&crossTimestamp, PTR_ADD_OFFSET(&request, request.DataOffset), sizeof(crossTimestamp));
                Timestamp->SystemTimestamp1 = crossTimestamp.SystemTimestamp1;
                Timestamp->HardwareClockTimestamp = crossTimestamp.HardwareClockTimestamp;
                Timestamp->SystemTimestamp2 = crossTimestamp.SystemTimestamp2;
            }
        }
    }

    NtClose(deviceHandle);
    return result;
}

typedef struct _PHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION
{
    ULONG Tag;
    HANDLE NsiNotificationHandle;
    PVOID CallerContext;
    PINTERFACE_TIMESTAMP_CONFIG_CHANGE_CALLBACK Callback;
} PHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, *PPHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION;

#ifdef _WIN64
static_assert(sizeof(PHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION) == 0x20);
static_assert(FIELD_OFFSET(PHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, NsiNotificationHandle) == 0x08);
static_assert(FIELD_OFFSET(PHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, CallerContext) == 0x10);
static_assert(FIELD_OFFSET(PHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, Callback) == 0x18);
#else
static_assert(sizeof(PHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION) == 0x10);
static_assert(FIELD_OFFSET(PHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, NsiNotificationHandle) == 0x04);
static_assert(FIELD_OFFSET(PHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, CallerContext) == 0x08);
static_assert(FIELD_OFFSET(PHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, Callback) == 0x0C);
#endif

/**
 * NSI callback that forwards timestamp configuration changes to the caller.
 *
 * \param Registration The registration context.
 * \param ChangeKey The key of the object that changed.
 * \param Reserved Unused.
 * \param NotificationType The type of change.
 * \return ERROR_SUCCESS (NO_ERROR) on success, otherwise a Win32 error code.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-registerinterfacetimestampconfigchange
 */
static ULONG NTAPI PhpInterfaceTimestampConfigChangeCallback(
    _In_ PPHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION Registration,
    _In_opt_ PVOID ChangeKey,
    _In_opt_ PVOID Reserved,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    if (Registration->Tag == PHP_TIMESTAMP_CONFIG_CHANGE_TAG)
    {
        Registration->Callback(Registration->CallerContext);
    }

    return NO_ERROR;
}

/**
 * Registers to be notified of changes to interface timestamp configuration.
 *
 * \param Callback The callback invoked on each change.
 * \param CallerContext A caller-defined context passed to the callback.
 * \param NotificationHandle Receives the notification handle. Close it with PhUnregisterInterfaceTimestampConfigChange.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-registerinterfacetimestampconfigchange
 */
NTSTATUS NTAPI PhRegisterInterfaceTimestampConfigChange(
    _In_ PINTERFACE_TIMESTAMP_CONFIG_CHANGE_CALLBACK Callback,
    _In_opt_ PVOID CallerContext,
    _Out_ HIFTIMESTAMPCHANGE* NotificationHandle
    )
{
    PPHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION registration;
    PVOID rpcHandle;
    NTSTATUS result;

    if (!Callback || !NotificationHandle)
        return STATUS_INVALID_PARAMETER;

    *NotificationHandle = NULL;
    registration = PhAllocateSafe(sizeof(*registration));

    if (!registration)
        return STATUS_NO_MEMORY;

    memset(registration, 0, sizeof(*registration));
    registration->Tag = PHP_TIMESTAMP_CONFIG_CHANGE_TAG;
    registration->Callback = Callback;
    registration->CallerContext = CallerContext;

    rpcHandle = PhNsiNotificationConnectToServer();

    if (!rpcHandle)
    {
        result = PhpNetioDosErrorToNtStatus(PhGetLastError());
        if (!result)
            result = STATUS_UNSUCCESSFUL;
    }
    else
    {
        result = PhNsiRpcRegisterChangeNotification(
            rpcHandle,
            &NPI_MS_NDIS_MODULEID,
            NsiNdisObjectTimestampConfigChange,
            PhpInterfaceTimestampConfigChangeCallback,
            FALSE,
            registration,
            &registration->NsiNotificationHandle
            );
    }

    if (!NT_SUCCESS(result))
    {
        PhFree(registration);
        return result;
    }

    *NotificationHandle = (HIFTIMESTAMPCHANGE)registration;
    return STATUS_SUCCESS;
}

/**
 * Cancels a registration created by PhRegisterInterfaceTimestampConfigChange.
 *
 * \param NotificationHandle The notification handle.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-unregisterinterfacetimestampconfigchange
 */
VOID NTAPI PhUnregisterInterfaceTimestampConfigChange(
    _In_ HIFTIMESTAMPCHANGE NotificationHandle
    )
{
    PPHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION registration;

    registration = (PPHP_TIMESTAMP_CONFIG_CHANGE_REGISTRATION)NotificationHandle;

    if (!registration || registration->Tag != PHP_TIMESTAMP_CONFIG_CHANGE_TAG)
        return;

    PhNsiRpcDeregisterChangeNotification(
        NotificationRpcHandle,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectTimestampConfigChange,
        registration->NsiNotificationHandle
        );

    registration->Tag = 0;
    PhFree(registration);
}

/**
 * Retrieves the description of an interface from its GUID.
 *
 * \param InterfaceGuid The GUID of the network interface.
 * \param InterfaceDescription Receives the null-terminated description.
 * \param InterfaceDescriptionLength On input, the size of InterfaceDescription in bytes; on output, the required size.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getifentry2
 */
NTSTATUS PhNhGetInterfaceDescriptionFromGuid(
    _In_ GUID *InterfaceGuid,
    _Out_writes_bytes_(*InterfaceDescriptionLength) PWSTR InterfaceDescription,
    _Inout_ PULONG InterfaceDescriptionLength
    )
{
    NTSTATUS result;
    NTSTATUS status;
    NET_LUID interfaceLuid;
    NSI_COUNTED_WCHAR_PARAMETER description;

    interfaceLuid.Value = 0;
    memset(&description, 0, sizeof(NSI_COUNTED_WCHAR_PARAMETER));

    result = PhConvertInterfaceGuidToLuid(InterfaceGuid, &interfaceLuid);

    if (!NT_SUCCESS(result))
        return result;

    status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &interfaceLuid,
        sizeof(NET_LUID),
        NsiParameterTypeReadOnlyStatic,
        &description,
        sizeof(NSI_COUNTED_WCHAR_PARAMETER),
        NsiNdisInterfaceLuidToDescription
        );

    if (NT_SUCCESS(status))
    {
        ULONG length = description.Length / sizeof(WCHAR);
        if (length > MAXIMUM_FILENAME_LENGTH)
            length = MAXIMUM_FILENAME_LENGTH;
        description.Buffer[length] = UNICODE_NULL;
    }

    NTSTATUS copyStatus = PhCopyStringZ(
        (NT_SUCCESS(status)) ? description.Buffer : L"",
        SIZE_MAX,
        InterfaceDescription,
        *InterfaceDescriptionLength / sizeof(WCHAR),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        ULONG stringStatus = PhStringResultToStatus(copyStatus);
        if (stringStatus == STATUS_BUFFER_TOO_SMALL)
        {
            *InterfaceDescriptionLength = sizeof(description.Buffer);
        }
        return stringStatus;
    }

    return status;
}

/**
 * Retrieves a framing layer virtual interface entry.
 *
 * \param InterfaceRow On input, the key fields of the row; on output, the complete row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhGetFlVirtualInterface(
    _Inout_ PMIB_FL_VIRTUAL_INTERFACE_ROW InterfaceRow
    )
{
    NTSTATUS result;
    const NPI_MODULEID* moduleId;
    USHORT family;
    NSI_FL_VIRTUAL_INTERFACE_KEY key;
    NSI_FL_VIRTUAL_INTERFACE_RW readWrite;
    NSI_FL_VIRTUAL_INTERFACE_ROD readOnlyDynamic;
    NSI_FL_VIRTUAL_INTERFACE_ROS readOnlyStatic;

    family = InterfaceRow->Family;
    memset(&key, 0, sizeof(NSI_FL_VIRTUAL_INTERFACE_KEY));
    memset(&readWrite, 0, sizeof(NSI_FL_VIRTUAL_INTERFACE_RW));
    memset(&readOnlyDynamic, 0, sizeof(NSI_FL_VIRTUAL_INTERFACE_ROD));
    memset(&readOnlyStatic, 0, sizeof(NSI_FL_VIRTUAL_INTERFACE_ROS));

    if (family == AF_INET)
        moduleId = &NPI_MS_FL48_MODULEID;
    else if (family == AF_INET6)
        moduleId = &NPI_MS_FL68_MODULEID;
    else
        return STATUS_INVALID_PARAMETER;

    key.InterfaceLuid = InterfaceRow->IfLuid;
    key.VirtualInterfaceId = InterfaceRow->VirtualIfId;

    result = PhNsiGetAllParameters(
        NsiActive,
        moduleId,
        NsiFlObjectVirtualInterface,
        &key,
        sizeof(NSI_FL_VIRTUAL_INTERFACE_KEY),
        &readWrite,
        sizeof(NSI_FL_VIRTUAL_INTERFACE_RW),
        &readOnlyDynamic,
        sizeof(NSI_FL_VIRTUAL_INTERFACE_ROD),
        &readOnlyStatic,
        sizeof(NSI_FL_VIRTUAL_INTERFACE_ROS)
        );

    if (!result)
    {
        PhFillVirtualInterfaceRow(InterfaceRow, family, &key, &readWrite, &readOnlyDynamic, &readOnlyStatic);
        return 0;
    }

    return result;
}

/**
 * Retrieves the framing layer virtual interface table.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the allocated table. Free it with PhFreeMibTable/FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/network/introduction-to-the-network-module-registrar
 */
NTSTATUS PhGetFlVirtualInterfaceTable(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_FL_VIRTUAL_INTERFACE_TABLE *Table
    )
{
    struct
    {
        ADDRESS_FAMILY Family;
        PNPI_MODULEID ModuleId;
        PNSI_FL_VIRTUAL_INTERFACE_KEY Keys;
        PNSI_FL_VIRTUAL_INTERFACE_RW Rw;
        PNSI_FL_VIRTUAL_INTERFACE_ROD Rod;
        PNSI_FL_VIRTUAL_INTERFACE_ROS Ros;
        ULONG Count;
    } families[2] = {
        { AF_INET, (PNPI_MODULEID)&NPI_MS_FL48_MODULEID },
        { AF_INET6, (PNPI_MODULEID)&NPI_MS_FL68_MODULEID }
    };
    NTSTATUS result;
    ULONG count;
    ULONG i;
    ULONG j;
    ULONG entryIndex;
    PMIB_FL_VIRTUAL_INTERFACE_TABLE table;

    if (!Table || (Family != AF_UNSPEC && Family != AF_INET && Family != AF_INET6))
        return STATUS_INVALID_PARAMETER;

    count = 0;
    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (Family != AF_UNSPEC && Family != families[i].Family)
            continue;

        result = PhNsiAllocateAndGetTable(NsiActive, families[i].ModuleId, NsiFlObjectVirtualInterface,
            &families[i].Keys, sizeof(NSI_FL_VIRTUAL_INTERFACE_KEY),
            &families[i].Rw, sizeof(NSI_FL_VIRTUAL_INTERFACE_RW),
            &families[i].Rod, sizeof(NSI_FL_VIRTUAL_INTERFACE_ROD),
            &families[i].Ros, sizeof(NSI_FL_VIRTUAL_INTERFACE_ROS), &families[i].Count, 0);
        if (result == STATUS_NOT_FOUND || result == STATUS_NOT_SUPPORTED)
            continue;
        if (result)
            goto Cleanup;

        if (count > ULONG_MAX - families[i].Count)
        {
            result = STATUS_INTEGER_OVERFLOW;
            goto Cleanup;
        }
        count += families[i].Count;
    }

    if (count > (SIZE_MAX - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_TABLE, Table)) / sizeof(MIB_FL_VIRTUAL_INTERFACE_ROW))
    {
        result = STATUS_INTEGER_OVERFLOW;
        goto Cleanup;
    }

    table = PhAllocateZero(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_TABLE, Table) + (SIZE_T)count * sizeof(MIB_FL_VIRTUAL_INTERFACE_ROW));
    entryIndex = 0;
    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        for (j = 0; j < families[i].Count; j++)
        {
            PhFillVirtualInterfaceRow(&table->Table[entryIndex], families[i].Family,
                &families[i].Keys[j], &families[i].Rw[j], &families[i].Rod[j], &families[i].Ros[j]);
            entryIndex++;
        }
    }

    table->NumEntries = entryIndex;
    *Table = table;
    result = STATUS_SUCCESS;

Cleanup:
    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (families[i].Keys || families[i].Rw || families[i].Rod || families[i].Ros)
            PhNsiFreeTable(families[i].Keys, families[i].Rw, families[i].Rod, families[i].Ros);
    }
    return result;
}

/**
 * Retrieves the legacy IPv4 ARP table.
 *
 * \param IpNetTable The buffer that receives the table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getipnettable
 */
NTSTATUS PhGetIpNetTable(
    _Out_writes_bytes_opt_(*SizePointer) PMIB_IPNETTABLE IpNetTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order
    )
{
    NTSTATUS result;
    NTSTATUS tableResult;
    NET_IF_COMPARTMENT_ID CurrentThreadCompartmentId;
    ULONG outputCount;
    ULONG neighborIndex;
    PVOID dynamicTable;
    PVOID keyTable;
    PVOID readWriteTable;
    ULONG neighborCount;

    keyTable = 0;
    readWriteTable = 0;
    dynamicTable = 0;

    memset(&neighborCount, 0, sizeof(ULONG));
    
    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_IPV4_MODULEID,
        NsiIpObjectNeighbor,
        &keyTable,
        sizeof(MIB_IP_NEIGHBOR_KEY_ROW_V4),
        &readWriteTable,
        sizeof(MIB_IP_NEIGHBOR_RW_ROW),
        &dynamicTable,
        sizeof(MIB_IP_NEIGHBOR_DYNAMIC_ROW),
        NULL,
        0,
        &neighborCount,
        0
        );

    if (!result)
    {
        tableResult = PhpNetioMibCheckTableBufferSize(
            neighborCount,
            sizeof(MIB_IPNETROW),
            IpNetTable,
            SizePointer);

        if (!tableResult)
        {
            CurrentThreadCompartmentId = PhGetCurrentThreadCompartmentId();
            outputCount = 0;
            neighborIndex = 0;

            if (neighborCount)
            {
                do
                {
                    // Offset 12 of dynamic element is the compartment ID
                    PMIB_IP_NEIGHBOR_DYNAMIC_ROW dynamicElement = PTR_ADD_OFFSET(dynamicTable, neighborIndex * sizeof(MIB_IP_NEIGHBOR_DYNAMIC_ROW));

                    if (dynamicElement->CompartmentId == CurrentThreadCompartmentId)
                    {
                        PMIB_IPNETROW row = &IpNetTable->table[outputCount];
                        outputCount++;

                        row->dwAddr = 0;
                        row->dwType = 0;

                        PMIB_IP_NEIGHBOR_KEY_ROW_V4 keyElement = PTR_ADD_OFFSET(keyTable, sizeof(MIB_IP_NEIGHBOR_KEY_ROW_V4) * neighborIndex);
                        PhConvertInterfaceLuidToIndex(&keyElement->InterfaceLuid, &row->dwIndex);

                        ULONG physAddrLen = dynamicElement->PhysicalAddressLength;
                        if (physAddrLen > sizeof(row->bPhysAddr))
                            physAddrLen = sizeof(row->bPhysAddr);
                        row->dwPhysAddrLen = physAddrLen;

                        PVOID physAddr = PTR_ADD_OFFSET(readWriteTable, neighborIndex * sizeof(MIB_IP_NEIGHBOR_RW_ROW));
                        memcpy(row->bPhysAddr, physAddr, physAddrLen);

                        row->dwAddr = keyElement->Address.S_un.S_addr;

                        ULONG neighborState = dynamicElement->State;
                        row->dwType = PhpNeighborStateToMibState(neighborState);
                    }
                    ++neighborIndex;
                } while (neighborIndex < neighborCount);
            }
            IpNetTable->dwNumEntries = outputCount;
            if (FALSE && Order)
                qsort_s(IpNetTable->table, outputCount, sizeof(MIB_IPNETROW), NULL, 0);
            tableResult = IpNetTable->dwNumEntries == 0 ? STATUS_NO_MORE_ENTRIES : STATUS_SUCCESS;
        }
        PhNsiFreeTable(keyTable, readWriteTable, dynamicTable, NULL);
        return tableResult;
    }
    return result;
}

/**
 * Retrieves the best route and source address for a destination.
 *
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param InterfaceIndex The local index of the network interface.
 * \param SourceAddress The source address.
 * \param DestinationAddress The destination address.
 * \param AddressSortOptions Options for address sorting; reserved.
 * \param BestRoute Receives the best route.
 * \param BestSourceAddress Receives the best source address.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getbestroute2
 */
NTSTATUS PhGetBestRoute2(
    _In_opt_ NET_LUID *InterfaceLuid,
    _In_ NET_IFINDEX InterfaceIndex,
    _In_opt_ const SOCKADDR_INET *SourceAddress,
    _In_ const SOCKADDR_INET *DestinationAddress,
    _In_ ULONG AddressSortOptions,
    _Out_ PMIB_IPFORWARD_ROW2 BestRoute,
    _Out_ SOCKADDR_INET *BestSourceAddress
    )
{
    NTSTATUS status;
    ADDRESS_FAMILY family;
    BOOLEAN hasSource = FALSE;
    BOOLEAN isMapped = FALSE;
    
    if (!BestRoute || !BestSourceAddress || !DestinationAddress)
        return STATUS_INVALID_PARAMETER;

    family = DestinationAddress->si_family;
    if (family != AF_INET && family != AF_INET6)
        return STATUS_INVALID_PARAMETER;

    if (SourceAddress && SourceAddress->si_family == family)
        hasSource = TRUE;

    memset(BestRoute, 0, sizeof(MIB_IPFORWARD_ROW2));
    memset(BestSourceAddress, 0, sizeof(SOCKADDR_INET));

    NET_LUID luid = { 0 };
    if (InterfaceLuid)
    {
        luid = *InterfaceLuid;
    }
    else if (InterfaceIndex)
    {
        status = PhConvertInterfaceIndexToLuid(InterfaceIndex, &luid);

        if (!NT_SUCCESS(status))
            return status;
    }

    if (family == AF_INET6 && IN6_IS_ADDR_V4MAPPED(&DestinationAddress->Ipv6.sin6_addr))
    {
        family = AF_INET;
        isMapped = TRUE;
    }

    if (family == AF_INET)
    {
        NSI_IP_BEST_ROUTE_KEY4 key;
        memset(&key, 0, sizeof(NSI_IP_BEST_ROUTE_KEY4));

        key.CompartmentId = PhGetCurrentThreadCompartmentId();
        
        if (isMapped)
            key.DestinationAddress = *(PIN_ADDR)(&DestinationAddress->Ipv6.sin6_addr.s6_addr[PH_IPV4_MAPPED_IN6_ADDRESS_OFFSET]);
        else
            key.DestinationAddress = DestinationAddress->Ipv4.sin_addr;

        key.DestinationScopeId.Value = (DestinationAddress->si_family == AF_INET6) ? DestinationAddress->Ipv6.sin6_scope_id : 0;

        if (hasSource)
        {
            if (SourceAddress->si_family == AF_INET6)
            {
                if (!IN6_IS_ADDR_V4MAPPED(&SourceAddress->Ipv6.sin6_addr))
                {
                    return STATUS_INVALID_PARAMETER;
                }

                key.SourceAddress = *(PIN_ADDR)(&SourceAddress->Ipv6.sin6_addr.s6_addr[PH_IPV4_MAPPED_IN6_ADDRESS_OFFSET]);
            }
            else
            {
                key.SourceAddress = SourceAddress->Ipv4.sin_addr;
            }

            key.SourceScopeId.Value = (SourceAddress->si_family == AF_INET6) ? SourceAddress->Ipv6.sin6_scope_id : 0;
        }

        key.InterfaceLuid = luid;

        NSI_IP_BEST_ROUTE_ROD_V4 resultRow;
        memset(&resultRow, 0, sizeof(NSI_IP_BEST_ROUTE_ROD_V4));

        status = PhNsiGetAllParameters(
            NsiActive,
            &NPI_MS_IPV4_MODULEID,
            NsiIpObjectBestRoute,
            &key,
            sizeof(NSI_IP_BEST_ROUTE_KEY4),
            NULL,
            0,
            &resultRow,
            sizeof(resultRow),
            NULL,
            0
            );

        if (NT_SUCCESS(status))
        {
            MIB_IP_ROUTE_DYNAMIC_ROW_V4 routeDynamic = { 0 };
            MIB_IP_ROUTE_STATIC_ROW routeStatic = { 0 };

            routeDynamic.Age = resultRow.Age;
            routeStatic.Origin = resultRow.Origin;
            routeStatic.InterfaceIndex = resultRow.InterfaceIndex;

            PhpFillRouteEntry(
                AF_INET,
                BestRoute,
                &resultRow.Key,
                &resultRow.Route,
                &routeDynamic,
                &routeStatic);

            INETADDR_SETSOCKADDR(
                AF_INET,
                (PSOCKADDR)BestSourceAddress,
                &resultRow.SourceAddress,
                resultRow.SourceScopeId,
                0
                );

            if (isMapped)
            {
                SOCKADDR_IN6 a6;
                SOCKADDR_IN6 v44;
                SCOPE_ID destinationScope;
                SCOPE_ID nextHopScope;
                SCOPE_ID sourceScope;

                memset(&a6, 0, sizeof(SOCKADDR_IN6));
                memset(&v44, 0, sizeof(SOCKADDR_IN6));

                destinationScope.Value = BestRoute->DestinationPrefix.Prefix.Ipv6.sin6_scope_id;
                nextHopScope.Value = BestRoute->NextHop.Ipv6.sin6_scope_id;
                sourceScope = resultRow.SourceScopeId;

                IN6ADDR_SETV4MAPPED(&a6, &BestRoute->DestinationPrefix.Prefix.Ipv4.sin_addr, destinationScope, BestRoute->DestinationPrefix.Prefix.Ipv4.sin_port);
                IN6ADDR_SETV4MAPPED(&v44, &BestRoute->NextHop.Ipv4.sin_addr, nextHopScope, BestRoute->NextHop.Ipv4.sin_port);

                BestRoute->DestinationPrefix.PrefixLength += (sizeof(IN6_ADDR) - sizeof(IN_ADDR)) * CHAR_BIT;
                BestRoute->DestinationPrefix.Prefix.Ipv6 = a6;
                BestRoute->NextHop.Ipv6 = v44;

                IN6ADDR_SETV4MAPPED((PSOCKADDR_IN6)BestSourceAddress, &resultRow.SourceAddress, sourceScope, 0);
            }
        }
    }
    else
    {
        NSI_IP_BEST_ROUTE_KEY6 key;
        memset(&key, 0, sizeof(NSI_IP_BEST_ROUTE_KEY6));

        key.CompartmentId = PhGetCurrentThreadCompartmentId();
        key.DestinationAddress = DestinationAddress->Ipv6.sin6_addr;
        key.DestinationScopeId.Value = DestinationAddress->Ipv6.sin6_scope_id;

        if (hasSource)
        {
            key.SourceAddress = SourceAddress->Ipv6.sin6_addr;
            key.SourceScopeId.Value = SourceAddress->Ipv6.sin6_scope_id;
        }

        key.InterfaceLuid = luid;

        NSI_IP_BEST_ROUTE_ROD_V6 resultRow;
        memset(&resultRow, 0, sizeof(NSI_IP_BEST_ROUTE_ROD_V6));

        status = PhNsiGetAllParameters(
            NsiActive,
            &NPI_MS_IPV6_MODULEID,
            NsiIpObjectBestRoute,
            &key,
            sizeof(NSI_IP_BEST_ROUTE_KEY6),
            NULL,
            0,
            &resultRow,
            sizeof(resultRow),
            NULL,
            0
            );

        if (NT_SUCCESS(status))
        {
            MIB_IP_ROUTE_DYNAMIC_ROW_V6 routeDynamic = { 0 };
            MIB_IP_ROUTE_STATIC_ROW routeStatic = { 0 };

            routeDynamic.Age = resultRow.Age;
            routeStatic.Origin = resultRow.Origin;
            routeStatic.InterfaceIndex = resultRow.InterfaceIndex;
            PhpFillRouteEntry(AF_INET6, BestRoute, &resultRow.Key,
                &resultRow.Route, &routeDynamic, &routeStatic);
            INETADDR_SETSOCKADDR(AF_INET6, (PSOCKADDR)BestSourceAddress, &resultRow.SourceAddress, resultRow.SourceScopeId, 0);
        }
    }

    return status;
}

#pragma endregion

#pragma region Internal Network Helpers

/**
 * Rebases a pointer that lies within a block that was copied to a new address.
 *
 * \param Pointer The pointer to relocate.
 * \param OldBase The original base address of the block.
 * \param NewBase The new base address of the block.
 * \param Size The size of the block, in bytes.
 * \return The relocated pointer, or the original pointer if it lies outside the block.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
 */
static PVOID PhpRelocatePointer(
    _In_ PVOID Pointer,
    _In_ PVOID OldBase,
    _In_ PVOID NewBase,
    _In_ SIZE_T Size
    )
{
    if (Pointer >= OldBase && (ULONG_PTR)Pointer < (ULONG_PTR)OldBase + Size)
    {
        return PTR_ADD_OFFSET(NewBase, PTR_SUB_OFFSET(Pointer, OldBase));
    }
    return Pointer;
}

/**
 * Sort routine that orders MIB_IPADDRROW entries by address.
 *
 * \param context Unused sort context.
 * \param elem1 The first element.
 * \param elem2 The second element.
 * \return A negative, zero or positive value.
 * \sa https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/qsort-s
 */
static int __cdecl PhpCompareMibIpAddrRow(
    _In_ void *context,
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    return memcmp(elem1, elem2, sizeof(IN_ADDR));
}

/**
 * Sort routine that orders IP_INTERFACE_NAME_INFO entries by index.
 *
 * \param context Unused sort context.
 * \param elem1 The first element.
 * \param elem2 The second element.
 * \return A negative, zero or positive value.
 * \sa https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/qsort-s
 */
static int __cdecl PhpCompareIpInterfaceNameInfo(
    _In_ void *context,
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    IP_INTERFACE_NAME_INFO *info1 = (IP_INTERFACE_NAME_INFO*)elem1;
    IP_INTERFACE_NAME_INFO *info2 = (IP_INTERFACE_NAME_INFO*)elem2;
    if (info1->Index < info2->Index)
        return -1;
    if (info1->Index > info2->Index)
        return 1;
    return 0;
}

/**
 * Converts a neighbor state to a legacy MIB_IPNET_TYPE value.
 *
 * \param State The NL_NEIGHBOR_STATE value.
 * \return The MIB_IPNET_TYPE value.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getipnettable
 */
ULONG PhpNeighborStateToMibState(
    _In_ ULONG State
    )
{
    switch (State)
    {
    case NlnsUnreachable:
        return MIB_IPNET_TYPE_INVALID;
    case NlnsIncomplete:
    case NlnsProbe:
    case NlnsDelay:
    case NlnsStale:
    case NlnsReachable:
    case NlnsPermanent:
        return MIB_IPNET_TYPE_DYNAMIC;
    default:
        return MIB_IPNET_TYPE_OTHER;
    }
}

/**
 * Determines whether an interface has an IPv4 interface instance and is not a loopback interface.
 *
 * \param InterfaceLuid The locally unique identifier (LUID) of the network interface.
 * \param MediaType The interface media type (IF_TYPE_*).
 * \return TRUE if the interface is an IPv4 interface, otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getipinterfaceentry
 */
BOOLEAN PhpIsIpv4Interface(
    _In_ NET_LUID *InterfaceLuid,
    _In_ USHORT MediaType
    )
{
    if (MediaType == IF_TYPE_SOFTWARE_LOOPBACK)
        return FALSE;

    if (PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_IPV4_MODULEID,
        NsiIpObjectIpInterface,
        InterfaceLuid,
        sizeof(NET_LUID),
        NULL,
        0,
        NULL,
        0,
        NULL,
        0
        ) == 0)
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * Retrieves the addresses associated with the network adapters.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Flags Flags that control which addresses are returned (GAA_FLAG_*).
 * \param Reserved Reserved; must be zero or NULL.
 * \param AdapterAddresses The buffer that receives the adapter list.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
 */
NTSTATUS NTAPI PhGetAdaptersAddresses(
    _In_ ULONG Family,
    _In_ ULONG Flags,
    _Reserved_ PVOID Reserved,
    _Out_writes_bytes_opt_(*SizePointer) PIP_ADAPTER_ADDRESSES AdapterAddresses,
    _Inout_ PULONG SizePointer
    )
{
    NTSTATUS status;
    PIP_ADAPTER_ADDRESSES lpMem = NULL;
    ULONG blockSize;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    status = PhpAllocateAndGetAdaptersAddresses(Family, Flags, &lpMem, &blockSize);
    if (!NT_SUCCESS(status))
    {
        if (lpMem)
            PhpFreeAdaptersAddresses(lpMem);
        return status;
    }

    if (!lpMem)
    {
        *SizePointer = 0;
        return STATUS_SUCCESS;
    }

    if (!AdapterAddresses || *SizePointer < blockSize)
    {
        *SizePointer = blockSize;
        PhpFreeAdaptersAddresses(lpMem);
        return STATUS_BUFFER_TOO_SMALL;
    }

    memcpy(AdapterAddresses, lpMem, blockSize);

    PIP_ADAPTER_ADDRESSES curr = AdapterAddresses;
    while (curr)
    {
        curr->Next = PhpRelocatePointer(curr->Next, lpMem, AdapterAddresses, blockSize);

        curr->AdapterName = PhpRelocatePointer(curr->AdapterName, lpMem, AdapterAddresses, blockSize);
        curr->DnsSuffix = PhpRelocatePointer(curr->DnsSuffix, lpMem, AdapterAddresses, blockSize);
        curr->Description = PhpRelocatePointer(curr->Description, lpMem, AdapterAddresses, blockSize);
        curr->FriendlyName = PhpRelocatePointer(curr->FriendlyName, lpMem, AdapterAddresses, blockSize);

        curr->Dhcpv4Server.lpSockaddr = PhpRelocatePointer(curr->Dhcpv4Server.lpSockaddr, lpMem, AdapterAddresses, blockSize);
        curr->Dhcpv6Server.lpSockaddr = PhpRelocatePointer(curr->Dhcpv6Server.lpSockaddr, lpMem, AdapterAddresses, blockSize);

        if (curr->FirstUnicastAddress)
        {
            curr->FirstUnicastAddress = PhpRelocatePointer(curr->FirstUnicastAddress, lpMem, AdapterAddresses, blockSize);
            PIP_ADAPTER_UNICAST_ADDRESS unicast = curr->FirstUnicastAddress;
            while (unicast)
            {
                unicast->Next = PhpRelocatePointer(unicast->Next, lpMem, AdapterAddresses, blockSize);
                unicast->Address.lpSockaddr = PhpRelocatePointer(unicast->Address.lpSockaddr, lpMem, AdapterAddresses, blockSize);
                unicast = unicast->Next;
            }
        }

        if (curr->FirstAnycastAddress)
        {
            curr->FirstAnycastAddress = PhpRelocatePointer(curr->FirstAnycastAddress, lpMem, AdapterAddresses, blockSize);
            PIP_ADAPTER_ANYCAST_ADDRESS anycast = curr->FirstAnycastAddress;
            while (anycast)
            {
                anycast->Next = PhpRelocatePointer(anycast->Next, lpMem, AdapterAddresses, blockSize);
                anycast->Address.lpSockaddr = PhpRelocatePointer(anycast->Address.lpSockaddr, lpMem, AdapterAddresses, blockSize);
                anycast = anycast->Next;
            }
        }

        if (curr->FirstMulticastAddress)
        {
            curr->FirstMulticastAddress = PhpRelocatePointer(curr->FirstMulticastAddress, lpMem, AdapterAddresses, blockSize);
            PIP_ADAPTER_MULTICAST_ADDRESS multicast = curr->FirstMulticastAddress;
            while (multicast)
            {
                multicast->Next = PhpRelocatePointer(multicast->Next, lpMem, AdapterAddresses, blockSize);
                multicast->Address.lpSockaddr = PhpRelocatePointer(multicast->Address.lpSockaddr, lpMem, AdapterAddresses, blockSize);
                multicast = multicast->Next;
            }
        }

        if (curr->FirstDnsServerAddress)
        {
            curr->FirstDnsServerAddress = PhpRelocatePointer(curr->FirstDnsServerAddress, lpMem, AdapterAddresses, blockSize);
            PIP_ADAPTER_DNS_SERVER_ADDRESS dns = curr->FirstDnsServerAddress;
            while (dns)
            {
                dns->Next = PhpRelocatePointer(dns->Next, lpMem, AdapterAddresses, blockSize);
                dns->Address.lpSockaddr = PhpRelocatePointer(dns->Address.lpSockaddr, lpMem, AdapterAddresses, blockSize);
                dns = dns->Next;
            }
        }

        if (curr->FirstPrefix)
        {
            curr->FirstPrefix = PhpRelocatePointer(curr->FirstPrefix, lpMem, AdapterAddresses, blockSize);
            PIP_ADAPTER_PREFIX prefix = curr->FirstPrefix;
            while (prefix)
            {
                prefix->Next = PhpRelocatePointer(prefix->Next, lpMem, AdapterAddresses, blockSize);
                prefix->Address.lpSockaddr = PhpRelocatePointer(prefix->Address.lpSockaddr, lpMem, AdapterAddresses, blockSize);
                prefix = prefix->Next;
            }
        }

        if (curr->FirstWinsServerAddress)
        {
            curr->FirstWinsServerAddress = PhpRelocatePointer(curr->FirstWinsServerAddress, lpMem, AdapterAddresses, blockSize);
            PIP_ADAPTER_WINS_SERVER_ADDRESS wins = curr->FirstWinsServerAddress;
            while (wins)
            {
                wins->Next = PhpRelocatePointer(wins->Next, lpMem, AdapterAddresses, blockSize);
                wins->Address.lpSockaddr = PhpRelocatePointer(wins->Address.lpSockaddr, lpMem, AdapterAddresses, blockSize);
                wins = wins->Next;
            }
        }

        if (curr->FirstGatewayAddress)
        {
            curr->FirstGatewayAddress = PhpRelocatePointer(curr->FirstGatewayAddress, lpMem, AdapterAddresses, blockSize);
            PIP_ADAPTER_GATEWAY_ADDRESS gateway = curr->FirstGatewayAddress;
            while (gateway)
            {
                gateway->Next = PhpRelocatePointer(gateway->Next, lpMem, AdapterAddresses, blockSize);
                gateway->Address.lpSockaddr = PhpRelocatePointer(gateway->Address.lpSockaddr, lpMem, AdapterAddresses, blockSize);
                gateway = gateway->Next;
            }
        }

        curr = curr->Next;
    }

    *SizePointer = blockSize;
    PhpFreeAdaptersAddresses(lpMem);
    return STATUS_SUCCESS;
}

/**
 * Cancels a change notification registered with one of the Notify* routines.
 *
 * \param NotificationHandle The notification handle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-cancelmibchangenotify2
 */
NTSTATUS NTAPI PhCancelMibChangeNotify2(
    _In_ HANDLE NotificationHandle
    )
{
    NTSTATUS result = STATUS_SUCCESS;
    PPHP_MIB_NOTIFICATION_HANDLE handle = (PPHP_MIB_NOTIFICATION_HANDLE)NotificationHandle;

    if (!handle)
        return STATUS_INVALID_PARAMETER;

    NSI_ICMP_OBJECT_ID objectId = handle->ObjectId;
    PVOID rpcHandle = NotificationRpcHandle;

    if (handle->NotificationHandleV4)
    {
        result = PhNsiRpcDeregisterChangeNotification(rpcHandle, &NPI_MS_IPV4_MODULEID, objectId, handle->NotificationHandleV4);
    }
    if (handle->NotificationHandleV6)
    {
        result = PhNsiRpcDeregisterChangeNotification(rpcHandle, &NPI_MS_IPV6_MODULEID, objectId, handle->NotificationHandleV6);
    }
    if (handle->NotificationHandleTeredo)
    {
        result = PhNsiRpcDeregisterChangeNotification(rpcHandle, &NPI_MS_FL6T_MODULEID, objectId, handle->NotificationHandleTeredo);
    }
    if (handle->NotificationHandleNdis)
    {
        result = PhNsiRpcDeregisterChangeNotification(rpcHandle, &NPI_MS_NDIS_MODULEID, objectId, handle->NotificationHandleNdis);
    }
    if (handle->ConnectivityHintContext)
    {
        PPHP_CONNECTIVITY_HINT_CONTEXT context = (PPHP_CONNECTIVITY_HINT_CONTEXT)handle->ConnectivityHintContext;

        PhAcquireQueuedLockExclusive(&PhpConnectivityHintLock);
        RemoveEntryList(&context->ListEntry);
        PhReleaseQueuedLockExclusive(&PhpConnectivityHintLock);

        if (context->ThreadpoolWork)
            TpReleaseWork(context->ThreadpoolWork);
        PhFree(context);
    }

    PhFree(handle);
    return result;
}

/**
 * Retrieves the alias of an interface from its device GUID (iphlpapi!NhGetInterfaceNameFromDeviceGuid).
 *
 * \param DeviceGuid The device GUID.
 * \param InterfaceDescription Receives the null-terminated alias.
 * \param InterfaceDescriptionLength On input, the size of InterfaceDescription in bytes; on output, the required size.
 * \param Cache Unused.
 * \param Refresh Unused.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoalias
 */
NTSTATUS NTAPI PhNhGetInterfaceNameFromDeviceGuid(
    _In_ PGUID DeviceGuid,
    _Out_writes_(InterfaceDescriptionLength) PWCHAR InterfaceDescription,
    _Inout_ PULONG InterfaceDescriptionLength,
    _In_ BOOLEAN Cache,
    _In_ BOOLEAN Refresh
    )
{
    return (ULONG)PhNhGetInterfaceNameFromGuid(DeviceGuid, InterfaceDescription, InterfaceDescriptionLength);
}

/**
 * Retrieves IPv4 statistics.
 *
 * \param Statistics Receives the statistics.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getipstatistics
 */
NTSTATUS NTAPI PhGetIpStatistics(
    _Out_ PMIB_IPSTATS Statistics
    )
{
    return PhGetIpStatisticsEx(Statistics, AF_INET);
}

/**
 * Retrieves the legacy IPv4 address table.
 *
 * \param IpAddrTable The buffer that receives the table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \param Order TRUE to sort the returned table.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getipaddrtable
 */
NTSTATUS NTAPI PhGetIpAddrTable(
    _Out_writes_bytes_opt_(*SizePointer) PMIB_IPADDRTABLE IpAddrTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOLEAN Order
    )
{
    NTSTATUS status;
    PIP_ADAPTER_ADDRESSES lpMem = NULL;
    ULONG entryCount = 0;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    status = PhpAllocateAndGetAdaptersAddresses(
        AF_INET,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
        &lpMem,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        if (lpMem)
            PhpFreeAdaptersAddresses(lpMem);
        return status;
    }

    if (!lpMem)
    {
        *SizePointer = 0;
        return STATUS_SUCCESS;
    }

    PIP_ADAPTER_ADDRESSES adapter = (PIP_ADAPTER_ADDRESSES)lpMem;
    while (adapter)
    {
        if (adapter->OperStatus == IfOperStatusUp)
        {
            PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress;
            while (unicast)
            {
                if (unicast->DadState >= IpDadStateDeprecated)
                {
                    entryCount++;
                }
                unicast = unicast->Next;
            }
        }
        adapter = adapter->Next;
    }

    ULONG sizeRequired = FIELD_OFFSET(MIB_IPADDRTABLE, table) + entryCount * sizeof(MIB_IPADDRROW);
    if (!IpAddrTable || *SizePointer < sizeRequired)
    {
        *SizePointer = sizeRequired;
        PhpFreeAdaptersAddresses(lpMem);
        return STATUS_BUFFER_TOO_SMALL;
    }

    IpAddrTable->dwNumEntries = entryCount;

    ULONG index = 0;
    adapter = (PIP_ADAPTER_ADDRESSES)lpMem;
    while (adapter)
    {
        if (adapter->OperStatus == IfOperStatusUp)
        {
            PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress;
            while (unicast)
            {
                if (unicast->DadState >= IpDadStateDeprecated)
                {
                    PMIB_IPADDRROW row = &IpAddrTable->table[index];
                    row->dwAddr = ((PSOCKADDR_IN)unicast->Address.lpSockaddr)->sin_addr.S_un.S_addr;
                    row->dwIndex = adapter->IfIndex;
                    
                    PhConvertLengthToIpv4Mask(unicast->OnLinkPrefixLength, &row->dwMask);
                    row->dwBCastAddr = 1;
                    row->dwReasmSize = MAXUSHORT;
                    row->unused1 = 0;
                    
                    USHORT type = 0;
                    if (FALSE)
                    {
                        type |= MIB_IPADDR_TRANSIENT;
                    }
                    if (unicast->SuffixOrigin == IpSuffixOriginDhcp || unicast->SuffixOrigin == IpSuffixOriginRandom)
                    {
                        type |= MIB_IPADDR_DYNAMIC;
                    }
                    if (unicast == adapter->FirstUnicastAddress)
                    {
                        type |= MIB_IPADDR_PRIMARY;
                    }
                    if (adapter->OperStatus != IfOperStatusUp)
                    {
                        type |= MIB_IPADDR_DISCONNECTED;
                    }
                    if (unicast->DadState == IpDadStateDeprecated)
                    {
                        type |= MIB_IPADDR_DELETED;
                    }
                    row->wType = type;
                    index++;
                }
                unicast = unicast->Next;
            }
        }
        adapter = adapter->Next;
    }

    if (Order && index > 0)
    {
        qsort_s(IpAddrTable->table, index, sizeof(MIB_IPADDRROW), PhpCompareMibIpAddrRow, NULL);
    }

    PhpFreeAdaptersAddresses(lpMem);
    return STATUS_SUCCESS;
}

/**
 * Cancels a notification requested with PhNotifyAddrChange or PhNotifyRouteChange.
 *
 * \param Overlapped The OVERLAPPED structure passed to the notification request.
 * \return TRUE on success, otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-cancelipchangenotify
 */
BOOLEAN NTAPI PhCancelIPChangeNotify(
    _In_ LPOVERLAPPED Overlapped
    )
{
    return PhNsiCancelChangeNotification(Overlapped) == 0;
}

/**
 * Allocates and retrieves the IP_INTERFACE_NAME_INFO table (iphlpapi!NhpAllocateAndGetInterfaceInfoFromStack).
 *
 * \param InterfaceInfo Receives the allocated table. Free it with PhFree.
 * \param SizePointer Receives the number of entries.
 * \param Order TRUE to sort the returned table.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getinterfaceinfo
 */
NTSTATUS NTAPI PhAllocateAndGetInterfaceInfoFromStack(
    _Outptr_ PIP_INTERFACE_NAME_INFO* InterfaceInfo,
    _Out_ PULONG SizePointer,
    _In_ BOOLEAN Order
    )
{
    NTSTATUS status;
    NET_LUID* luidTable = NULL;
    NDIS_NSI_INTERFACE_ENUM_ROD* ndisTable = NULL;
    NDIS_NSI_INTERFACE_ENUM_ROS* ndisTable2 = NULL;
    ULONG count = 0;
    PIP_INTERFACE_NAME_INFO table = NULL;

    if (!InterfaceInfo || !SizePointer)
        return STATUS_INVALID_PARAMETER;

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterfaceWithoutStatistics,
        &luidTable,
        sizeof(NET_LUID),
        NULL,
        0,
        &ndisTable,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROD),
        &ndisTable2,
        sizeof(NDIS_NSI_INTERFACE_ENUM_ROS),
        &count,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    if (count == 0)
    {
        PhNsiFreeTable(luidTable, NULL, ndisTable, ndisTable2);
        *InterfaceInfo = NULL;
        *SizePointer = 0;
        return STATUS_NO_MORE_ENTRIES;
    }

    table = PhAllocate(count * sizeof(IP_INTERFACE_NAME_INFO));
    if (!table)
    {
        PhNsiFreeTable(luidTable, NULL, ndisTable, ndisTable2);
        return STATUS_NO_MEMORY;
    }

    NET_IF_COMPARTMENT_ID currentCompartment = PhGetCurrentThreadCompartmentId();
    ULONG outputCount = 0;

    for (ULONG i = 0; i < count; i++)
    {
        NDIS_NSI_INTERFACE_ENUM_ROD* ndisElement = &ndisTable[i];
        if (ndisElement->CompartmentId == currentCompartment)
        {
            PNDIS_NSI_INTERFACE_ENUM_ROS publicEntry = (PNDIS_NSI_INTERFACE_ENUM_ROS)&ndisTable2[i];
            USHORT mediaType = publicEntry->ifType;
            NET_LUID* luid = &luidTable[i];

            if (PhpIsIpv4Interface(luid, mediaType))
            {
                PIP_INTERFACE_NAME_INFO dest = &table[outputCount];
                outputCount++;

                dest->Index = publicEntry->ifIndex;
                dest->MediaType = mediaType;
                dest->ConnectionType = publicEntry->ConnectionType;
                dest->AccessType = publicEntry->AccessType;
                dest->DeviceGuid = publicEntry->InterfaceGuid;
                dest->InterfaceGuid = publicEntry->InterfaceGuid;
            }
        }
    }

    if (Order && outputCount > 0)
    {
        qsort_s(table, outputCount, sizeof(IP_INTERFACE_NAME_INFO), PhpCompareIpInterfaceNameInfo, NULL);
    }

    PhNsiFreeTable(luidTable, NULL, ndisTable, ndisTable2);

    *InterfaceInfo = table;
    *SizePointer = outputCount;
    return STATUS_SUCCESS;
}

/**
 * Allocates and retrieves the legacy IPv4 address table (iphlpapi!AllocateAndGetIpAddrTableFromStack).
 *
 * \param IpAddrTable Receives the allocated table. Free it with PhFree.
 * \param SizePointer Receives the size, in bytes, of the table.
 * \param Order TRUE to sort the returned table.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getipaddrtable
 */
NTSTATUS NTAPI PhAllocateAndGetIpAddrTableFromStack(
    _Outptr_ PMIB_IPADDRTABLE *IpAddrTable,
    _Out_ PULONG SizePointer,
    _In_ BOOLEAN Order
    )
{
    NTSTATUS status;
    ULONG size = 0;
    PMIB_IPADDRTABLE table = NULL;

    status = PhGetIpAddrTable(NULL, &size, Order);

    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        return status;
    }

    table = (PMIB_IPADDRTABLE)PhAllocate(size);

    if (!table)
    {
        return STATUS_NO_MEMORY;
    }

    status = PhGetIpAddrTable(table, &size, Order);
    
    if (!NT_SUCCESS(status))
    {
        PhFree(table);
        return status;
    }

    *IpAddrTable = table;

    if (SizePointer)
    {
        *SizePointer = size;
    }

    return STATUS_SUCCESS;
}

/**
 * Closes a compartment handle.
 *
 * \param CompartmentHandle The compartment handle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntclose
 */
NTSTATUS NTAPI PhCloseCompartment(
    _In_ HANDLE CompartmentHandle
    )
{
    return NtClose(CompartmentHandle);
}

/**
 * Closes a handle returned by GetIPPhysicalInterfaceForDestination. Implemented as a stub.
 *
 * \param Handle The handle; cleared on return.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getbestinterfaceex
 */
NTSTATUS NTAPI PhCloseGetIPPhysicalInterfaceForDestination(
    _Inout_ PHANDLE Handle
    )
{
    // The previous implementation byte-scanned iphlpapi!CloseGetIPPhysicalInterfaceForDestination
    // to locate iphlpapi's internal context lock/list. There is no native open-side
    // counterpart (PhGetIPPhysicalInterfaceForDestination) and no production caller, so
    // this is a no-op stub that no longer depends on iphlpapi.
    if (!Handle)
        return STATUS_INVALID_PARAMETER;

    if (!*Handle)
        return STATUS_SUCCESS;

    return STATUS_NOT_SUPPORTED;
}

/**
 * Determines whether an IPv6 address is an IPv4-mapped address.
 *
 * \param Address The address.
 * \return TRUE if the address is IPv4-mapped, otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/in6addr/ns-in6addr-in6_addr
 */
static BOOLEAN PhpIsIpv4MappedAddress(
    _In_ const IN6_ADDR* Address
    )
{
    return IN6_IS_ADDR_V4MAPPED(Address);
}

/**
 * Determines whether an IPv6 address is the unspecified address (::).
 *
 * \param Address The address.
 * \return TRUE if the address is unspecified, otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/in6addr/ns-in6addr-in6_addr
 */
static BOOLEAN PhpIsUnspecifiedIpv6Address(
    _In_ const IN6_ADDR* Address
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(Address->u.Word); i++)
    {
        if (Address->u.Word[i] != 0)
            return FALSE;
    }

    return TRUE;
}

/**
 * Sort routine that orders MIB_TCPROW2 entries by local and remote endpoint.
 *
 * \param Context Unused.
 * \param Row1 The first row.
 * \param Row2 The second row.
 * \return A negative, zero or positive value.
 * \sa https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/qsort-s
 */
static int __cdecl PhpCompareMibTcpRow2(
    _In_opt_ PVOID Context,
    _In_ const void* Row1,
    _In_ const void* Row2
    )
{
    // CompareMibTcpRow: local address/port, remote address/port.
    return memcmp(&((PMIB_TCPROW2)Row1)->dwLocalAddr, &((PMIB_TCPROW2)Row2)->dwLocalAddr, sizeof(ULONG) * 4);
}

/**
 * Sort routine that orders MIB_TCP6ROW2 entries by local and remote endpoint.
 *
 * \param Context Unused.
 * \param Row1 The first row.
 * \param Row2 The second row.
 * \return A negative, zero or positive value.
 * \sa https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/qsort-s
 */
static int __cdecl PhpCompareMibTcp6Row2(
    _In_opt_ PVOID Context,
    _In_ const void* Row1,
    _In_ const void* Row2
    )
{
    // CompareMibTcp6Row2: local address/scope/port, remote address/scope/port.
    return memcmp(Row1, Row2, FIELD_OFFSET(MIB_TCP6ROW2, State));
}

/**
 * Builds a table of the TCP endpoints that are bound but not connected or listening.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the allocated table. Free it with RtlFreeHeap.
 * \param HeapHandle The heap to allocate from.
 * \param HeapFlags Flags passed to RtlAllocateHeap.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcptable2
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcp6table2
 */
static NTSTATUS PhpAllocateAndGetBoundTcpTable(
    _In_ ADDRESS_FAMILY Family,
    _Out_ PVOID* Table,
    _In_ PVOID HeapHandle,
    _In_ ULONG HeapFlags
    )
{
    NTSTATUS result;
    NSI_TCP_KEY_ROW* keyTable = NULL;
    NSI_TCP_STATUS_ROW* statusTable = NULL;
    NSI_TCP_OWNER_ROW* ownerTable = NULL;
    ULONG count = 0;
    ULONG numberOfEntries = 0;
    SIZE_T tableSize;
    PVOID table;

    *Table = NULL;

    result = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTcpObjectBoundEndpoint,
        &keyTable,
        sizeof(NSI_TCP_KEY_ROW),
        NULL,
        0,
        &statusTable,
        sizeof(NSI_TCP_STATUS_ROW),
        &ownerTable,
        sizeof(NSI_TCP_OWNER_ROW),
        &count,
        0
        );

    if (!NT_SUCCESS(result))
        return result;

    if (Family == AF_INET)
        tableSize = FIELD_OFFSET(MIB_TCPTABLE2, table) + sizeof(MIB_TCPROW2) * (SIZE_T)max(count, 1);
    else
        tableSize = FIELD_OFFSET(MIB_TCP6TABLE2, table) + sizeof(MIB_TCP6ROW2) * (SIZE_T)max(count, 1);

    if (!(table = RtlAllocateHeap(HeapHandle, HeapFlags, tableSize)))
    {
        PhNsiFreeTable(keyTable, NULL, statusTable, ownerTable);
        return STATUS_NO_MEMORY;
    }

    for (ULONG i = 0; i < count; i++)
    {
        NSI_TCP_KEY_ROW* key = &keyTable[i];

        if (Family == AF_INET)
        {
            PMIB_TCPROW2 row;
            ULONG localAddress;
            ULONG remoteAddress;

            if (key->LocalAddress.si_family == AF_INET)
            {
                localAddress = key->LocalAddress.Ipv4.sin_addr.s_addr;
                remoteAddress = key->RemoteAddress.Ipv4.sin_addr.s_addr;
            }
            else if (key->LocalAddress.si_family == AF_INET6 && (
                PhpIsIpv4MappedAddress(&key->LocalAddress.Ipv6.sin6_addr) ||
                (ownerTable[i].DualMode && PhpIsUnspecifiedIpv6Address(&key->LocalAddress.Ipv6.sin6_addr))
                ))
            {
                // IPv4-mapped endpoint, or a dual-mode IPv6 wildcard socket (listed as 0.0.0.0).
                localAddress = *(PULONG)&key->LocalAddress.Ipv6.sin6_addr.u.Byte[12];
                remoteAddress = *(PULONG)&key->RemoteAddress.Ipv6.sin6_addr.u.Byte[12];
            }
            else
            {
                continue;
            }

            row = &((PMIB_TCPTABLE2)table)->table[numberOfEntries++];
            memset(row, 0, sizeof(MIB_TCPROW2));
            row->dwState = statusTable[i].State;
            row->dwLocalAddr = localAddress;
            row->dwLocalPort = key->LocalAddress.Ipv4.sin_port;
            row->dwRemoteAddr = remoteAddress;
            row->dwRemotePort = key->RemoteAddress.Ipv4.sin_port;
            row->dwOwningPid = ownerTable[i].OwningPid;
            row->dwOffloadState = statusTable[i].OffloadState;
        }
        else
        {
            PMIB_TCP6ROW2 row;

            if (key->LocalAddress.si_family != AF_INET6 || PhpIsIpv4MappedAddress(&key->LocalAddress.Ipv6.sin6_addr))
                continue;

            row = &((PMIB_TCP6TABLE2)table)->table[numberOfEntries++];
            memset(row, 0, sizeof(MIB_TCP6ROW2));
            row->LocalAddr = key->LocalAddress.Ipv6.sin6_addr;
            row->dwLocalScopeId = key->LocalAddress.Ipv6.sin6_scope_id;
            row->dwLocalPort = key->LocalAddress.Ipv6.sin6_port;
            row->RemoteAddr = key->RemoteAddress.Ipv6.sin6_addr;
            row->dwRemoteScopeId = key->RemoteAddress.Ipv6.sin6_scope_id;
            row->dwRemotePort = key->RemoteAddress.Ipv6.sin6_port;
            row->State = statusTable[i].State;
            row->dwOwningPid = ownerTable[i].OwningPid;
            row->dwOffloadState = statusTable[i].OffloadState;
        }
    }

    PhNsiFreeTable(keyTable, NULL, statusTable, ownerTable);

    // InternalGetTable always requests a sorted table.

    //if (Family == AF_INET)
    //{
    //    ((PMIB_TCPTABLE2)table)->dwNumEntries = numberOfEntries;
    //    qsort_s(((PMIB_TCPTABLE2)table)->table, numberOfEntries, sizeof(MIB_TCPROW2), PhpCompareMibTcpRow2, NULL);
    //}
    //else
    //{
    //    ((PMIB_TCP6TABLE2)table)->dwNumEntries = numberOfEntries;
    //    qsort_s(((PMIB_TCP6TABLE2)table)->table, numberOfEntries, sizeof(MIB_TCP6ROW2), PhpCompareMibTcp6Row2, NULL);
    //}

    *Table = table;
    return STATUS_SUCCESS;
}

/**
 * Retrieves the IPv4 TCP endpoints that are bound but not connected or listening.
 *
 * \param BoundTcpTable Receives a MIB_TCPTABLE2 allocated from HeapHandle. Free it with RtlFreeHeap.
 * \param HeapHandle The heap to allocate the table from.
 * \param HeapFlags Flags passed to RtlAllocateHeap.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \remarks Reimplements iphlpapi!InternalGetBoundTcpEndpointTable.
 */
NTSTATUS WINAPI PhInternalGetBoundTcpEndpointTable(
    _Out_ PVOID* BoundTcpTable,
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG HeapFlags
    )
{
    return PhpAllocateAndGetBoundTcpTable(AF_INET, BoundTcpTable, HeapHandle, HeapFlags);
}

/**
 * Retrieves the IPv6 TCP endpoints that are bound but not connected or listening.
 *
 * \param BoundTcpTable Receives a MIB_TCP6TABLE2 allocated from HeapHandle. Free it with RtlFreeHeap.
 * \param HeapHandle The heap to allocate the table from.
 * \param HeapFlags Flags passed to RtlAllocateHeap.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \remarks Reimplements iphlpapi!InternalGetBoundTcp6EndpointTable.
 */
NTSTATUS WINAPI PhInternalGetBoundTcp6EndpointTable(
    _Out_ PVOID* BoundTcpTable,
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG HeapFlags
    )
{
    return PhpAllocateAndGetBoundTcpTable(AF_INET6, BoundTcpTable, HeapHandle, HeapFlags);
}

//
// NotifyIpInterfaceChange
//

// Second argument of the NSI RPC change callback for keyed objects (iphlpapi!NetioInterfaceChangeCallBack).
typedef struct _PHP_NSI_CHANGE_KEY
{
    PVOID Key;
    ULONG KeyLength;
} PHP_NSI_CHANGE_KEY, *PPHP_NSI_CHANGE_KEY;

// Matches the NETIO_CHANGE_CALLBACK_CONTEXT + MIB_IPINTERFACE_ROW pairs of NETIO_IP_INTERFACE_CHANGE_REGISTRATION.
typedef struct _PHP_IP_INTERFACE_CHANGE_CONTEXT
{
    NETIO_CHANGE_CALLBACK_CONTEXT Callback;
    MIB_IPINTERFACE_ROW Row;
} PHP_IP_INTERFACE_CHANGE_CONTEXT, *PPHP_IP_INTERFACE_CHANGE_CONTEXT;

/**
 * NSI callback that converts an IP interface change into a MIB_IPINTERFACE_ROW and forwards it to the caller.
 *
 * \param Context The context.
 * \param ChangeKey The key of the object that changed.
 * \param Reserved Unused.
 * \param NotificationType The type of change.
 * \return ERROR_SUCCESS (NO_ERROR) on success, otherwise a Win32 error code.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-notifyipinterfacechange
 */
static ULONG NTAPI PhpIpInterfaceChangeCallback(
    _In_ PPHP_IP_INTERFACE_CHANGE_CONTEXT Context,
    _In_ PPHP_NSI_CHANGE_KEY ChangeKey,
    _In_opt_ PVOID Reserved,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    PMIB_IPINTERFACE_ROW row = NULL;

    // The initial notification has no key and reports a NULL row.
    if (ChangeKey->KeyLength)
    {
        row = &Context->Row;
        memset(row, 0, sizeof(MIB_IPINTERFACE_ROW));
        row->Family = Context->Callback.Family;
        row->InterfaceLuid = *(PNET_LUID)ChangeKey->Key;

        if (PhpNsiGetParameter(
            NsiActive,
            &NPI_MS_NDIS_MODULEID,
            NsiNdisObjectInterface,
            &row->InterfaceLuid,
            sizeof(NET_LUID),
            NsiParameterTypeReadOnlyStatic,
            &row->InterfaceIndex,
            sizeof(NET_IFINDEX),
            0
            ) != STATUS_SUCCESS)
        {
            row->InterfaceIndex = 0;
        }
    }

    ((PIPINTERFACE_CHANGE_CALLBACK)Context->Callback.Callback)(
        Context->Callback.CallerContext,
        row,
        NotificationType
        );

    return NO_ERROR;
}

/**
 * Registers to be notified of changes to IP interfaces.
 *
 * \param Family AF_INET, AF_INET6 or AF_UNSPEC.
 * \param Callback The callback to invoke on a change.
 * \param CallerContext A user context passed to the callback.
 * \param InitialNotification TRUE to invoke the callback immediately after registration.
 * \param NotificationHandle Receives a handle to pass to PhCancelMibChangeNotify2.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \remarks Reimplements iphlpapi!NotifyIpInterfaceChange (NETIO_IP_INTERFACE_CHANGE_REGISTRATION).
 */
NTSTATUS NTAPI PhNotifyIpInterfaceChange(
    _In_ ADDRESS_FAMILY Family,
    _In_ PIPINTERFACE_CHANGE_CALLBACK Callback,
    _In_opt_ PVOID CallerContext,
    _In_ BOOLEAN InitialNotification,
    _Inout_ PHANDLE NotificationHandle
    )
{
    PNETIO_IP_INTERFACE_CHANGE_REGISTRATION registration;
    PPHP_IP_INTERFACE_CHANGE_CONTEXT ipv4Context;
    PPHP_IP_INTERFACE_CHANGE_CONTEXT ipv6Context;
    PVOID rpcHandle;
    NTSTATUS result = STATUS_SUCCESS;
    NTSTATUS result4 = STATUS_SUCCESS;

    if ((Family != AF_UNSPEC && Family != AF_INET && Family != AF_INET6) || !Callback || !NotificationHandle)
        return STATUS_INVALID_PARAMETER;

    // PhCancelMibChangeNotify2 deregisters through the shared header and frees with PhFree.
    registration = PhAllocateZero(sizeof(NETIO_IP_INTERFACE_CHANGE_REGISTRATION));
    registration->Header.ObjectId = NsiIpObjectIpInterface;
    registration->Header.Ipv4CallbackContext = &registration->Ipv4Callback;
    registration->Header.Ipv6CallbackContext = &registration->Ipv6Callback;

    ipv4Context = (PPHP_IP_INTERFACE_CHANGE_CONTEXT)&registration->Ipv4Callback;
    ipv4Context->Callback.Family = AF_INET;
    ipv4Context->Callback.Callback = Callback;
    ipv4Context->Callback.CallerContext = CallerContext;

    ipv6Context = (PPHP_IP_INTERFACE_CHANGE_CONTEXT)&registration->Ipv6Callback;
    ipv6Context->Callback.Family = AF_INET6;
    ipv6Context->Callback.Callback = Callback;
    ipv6Context->Callback.CallerContext = CallerContext;

    if (!(rpcHandle = PhNsiNotificationConnectToServer()))
    {
        PhFree(registration);
        *NotificationHandle = NULL;
        return STATUS_INVALID_HANDLE;
    }

    if (Family != AF_INET6)
    {
        result = result4 = PhNsiRpcRegisterChangeNotification(
            rpcHandle,
            &NPI_MS_IPV4_MODULEID,
            NsiIpObjectIpInterface,
            PhpIpInterfaceChangeCallback,
            InitialNotification,
            ipv4Context,
            &registration->Header.Ipv4NotificationHandle
            );
    }

    if (Family != AF_INET)
    {
        result = PhNsiRpcRegisterChangeNotification(
            rpcHandle,
            &NPI_MS_IPV6_MODULEID,
            NsiIpObjectIpInterface,
            PhpIpInterfaceChangeCallback,
            InitialNotification,
            ipv6Context,
            &registration->Header.Ipv6NotificationHandle
            );

        // InternalRegisterChangeNotification: AF_UNSPEC succeeds when either module registers.
        if (Family == AF_UNSPEC)
            result = (!NT_SUCCESS(result4) && !NT_SUCCESS(result)) ? result4 : STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(result))
    {
        PhCancelMibChangeNotify2(registration);
        *NotificationHandle = NULL;
        return result;
    }

    *NotificationHandle = registration;
    return STATUS_SUCCESS;
}

//
// ParseNetworkString
//

static typeof(&DnsValidateName_W) PhpDnsValidateName_W = NULL;
static typeof(&DnsValidateName_UTF8) PhpDnsValidateName_UTF8 = NULL;

/**
 * Parses a decimal network prefix length.
 *
 * \param String The string to parse.
 * \return The prefix length, or MAXUCHAR if the string is invalid or out of range.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-parsenetworkstring
 */
static UCHAR PhpParsePrefixLength(
    _In_ PCWSTR String
    )
{
    ULONG value = 0;
    ULONG i;

    for (i = 0; String[i]; i++)
    {
        if (!iswdigit(String[i]))
            break;

        value = value * 10 + (String[i] - L'0');

        if (value > sizeof(IN6_ADDR) * CHAR_BIT)
            break;
    }

    return String[i] ? UCHAR_MAX : (UCHAR)value;
}

/**
 * Parses a decimal port number.
 *
 * \param String The string to parse.
 * \return The port number, or 0 if the string is invalid or out of range.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-parsenetworkstring
 */
static USHORT PhpParsePortNumber(
    _In_ PCWSTR String
    )
{
    ULONG value = 0;
    ULONG i;

    for (i = 0; String[i]; i++)
    {
        if (!iswdigit(String[i]))
            break;

        value = value * 10 + (String[i] - L'0');

        if (value > USHRT_MAX)
            break;
    }

    return String[i] ? 0 : (USHORT)value;
}

/**
 * Completes a partial dotted IPv4 address by appending zero octets.
 *
 * \param Buffer The address string.
 * \param BufferCount The size of Buffer, in characters.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-parsenetworkstring
 */
static VOID PhpCompleteIfPartialIpv4(
    _Inout_updates_z_(BufferCount) PWSTR Buffer,
    _In_ SIZE_T BufferCount
    )
{
    ULONG dots = 0;

    for (PWSTR p = Buffer; *p; p++)
    {
        if (*p == L'.')
            dots++;
    }

    // "10/8" -> "10.0.0.0/8"
    while (dots < 3)
    {
        if (!NT_SUCCESS(RtlStringCchCatW(Buffer, BufferCount, L".0")))
            break;

        dots++;
    }
}

/**
 * Determines whether a string is a valid DNS name or label.
 *
 * \param Utf8 TRUE if Name is UTF-8, FALSE if UTF-16.
 * \param Name The name to validate.
 * \return TRUE if the name is valid, otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/windns/nf-windns-dnsvalidatename_w
 */
static BOOLEAN PhpDnsNameIsValid(
    _In_ BOOLEAN Utf8,
    _In_ PCVOID Name
    )
{
    DNS_STATUS status;

    for (ULONG i = 0; i < 2; i++)
    {
        DNS_NAME_FORMAT format = i == 0 ? DnsNameDomain : DnsNameDomainLabel;

        if (Utf8)
            status = PhpDnsValidateName_UTF8(Name, format);
        else
            status = PhpDnsValidateName_W(Name, format);

        if (status == ERROR_SUCCESS || status == DNS_ERROR_NON_RFC_NAME)
            return TRUE;
    }

    return FALSE;
}

/**
 * Parses a network address string and validates it against the requested address types.
 *
 * \param NetworkString The string to parse.
 * \param Types A combination of NET_STRING_* flags.
 * \param AddressInfo Receives the parsed address.
 * \param PortNumber Receives the port number (host byte order).
 * \param PrefixLength Receives the prefix length, or 0xFF when the string has none.
 * \return An NTSTATUS value. STATUS_INVALID_PARAMETER when the string does not match any requested type.
 * \remarks Reimplements iphlpapi!ParseNetworkString.
 */
NTSTATUS NTAPI PhParseNetworkString(
    _In_ PCWSTR NetworkString,
    _In_ ULONG Types,
    _Out_opt_ PNET_ADDRESS_INFO AddressInfo,
    _Out_opt_ PUSHORT PortNumber,
    _Out_opt_ PUCHAR PrefixLength
    )
{
    IN_ADDR ipv4Address = { 0 };
    IN6_ADDR ipv6Address = { 0 };
    ULONG scopeId = 0;
    USHORT port = 0;
    PCWSTR separator;
    UCHAR prefix;

    if (!NetworkString)
        return STATUS_INVALID_PARAMETER;

    if (AddressInfo)
        AddressInfo->Format = NET_ADDRESS_FORMAT_UNSPECIFIED;

    // IPv4 address, with or without a port.

    if ((Types & (NET_STRING_IPV4_ADDRESS | NET_STRING_IPV4_SERVICE)) &&
        PhIpv4StringToAddressEx(NetworkString, TRUE, &ipv4Address, &port) == STATUS_SUCCESS &&
        (port ? (Types & NET_STRING_IPV4_SERVICE) : (Types & NET_STRING_IPV4_ADDRESS)))
    {
        if (AddressInfo)
        {
            AddressInfo->Format = NET_ADDRESS_IPV4;
            memset(&AddressInfo->Ipv4Address, 0, sizeof(SOCKADDR_IN));
            AddressInfo->Ipv4Address.sin_family = AF_INET;
            AddressInfo->Ipv4Address.sin_port = port;
            AddressInfo->Ipv4Address.sin_addr = ipv4Address;
        }

        if (PortNumber)
            *PortNumber = _byteswap_ushort(port);
        if (PrefixLength)
            *PrefixLength = UCHAR_MAX;

        return STATUS_SUCCESS;
    }

    // IPv4 network: "a.b.c.d/len" (partial addresses are completed with zeros).

    if ((Types & NET_STRING_IPV4_NETWORK) &&
        (separator = wcschr(NetworkString, L'/')) && separator[1] &&
        (prefix = PhpParsePrefixLength(separator + 1)) <= sizeof(IN_ADDR) * CHAR_BIT)
    {
        WCHAR addressString[PHP_IPV4_NETWORK_ADDRESS_BUFFER_LENGTH];

        if (NT_SUCCESS(RtlStringCchCopyNW(addressString, RTL_NUMBER_OF(addressString), NetworkString, separator - NetworkString)))
        {
            PhpCompleteIfPartialIpv4(addressString, RTL_NUMBER_OF(addressString));

            if (PhIpv4StringToAddressEx(addressString, TRUE, &ipv4Address, &port) == STATUS_SUCCESS &&
                port == 0 &&
                (prefix == sizeof(IN_ADDR) * CHAR_BIT || (_byteswap_ulong(ipv4Address.s_addr) << prefix) == 0))
            {
                if (AddressInfo)
                {
                    AddressInfo->Format = NET_ADDRESS_IPV4;
                    memset(&AddressInfo->Ipv4Address, 0, sizeof(SOCKADDR_IN));
                    AddressInfo->Ipv4Address.sin_family = AF_INET;
                    AddressInfo->Ipv4Address.sin_addr = ipv4Address;
                }

                if (PrefixLength)
                    *PrefixLength = prefix;
                if (PortNumber)
                    *PortNumber = 0;

                return STATUS_SUCCESS;
            }
        }
    }

    // IPv6 address, with or without a port and scope.

    if ((Types & (NET_STRING_IPV6_ADDRESS | NET_STRING_IPV6_ADDRESS_NO_SCOPE | NET_STRING_IPV6_SERVICE | NET_STRING_IPV6_SERVICE_NO_SCOPE)) &&
        PhIpv6StringToAddressEx(NetworkString, &ipv6Address, &scopeId, &port) == STATUS_SUCCESS)
    {
        BOOLEAN portAllowed;
        BOOLEAN scopeAllowed;

        if (port)
            portAllowed = !!(Types & (NET_STRING_IPV6_SERVICE | NET_STRING_IPV6_SERVICE_NO_SCOPE));
        else
            portAllowed = !!(Types & (NET_STRING_IPV6_ADDRESS | NET_STRING_IPV6_ADDRESS_NO_SCOPE));

        scopeAllowed =
            (scopeId == 0 && (Types & (NET_STRING_IPV6_ADDRESS_NO_SCOPE | NET_STRING_IPV6_SERVICE_NO_SCOPE))) ||
            (Types & (NET_STRING_IPV6_ADDRESS | NET_STRING_IPV6_SERVICE));

        if (portAllowed && scopeAllowed)
        {
            if (AddressInfo)
            {
                SCOPE_ID scope;

                scope.Value = scopeId;
                AddressInfo->Format = NET_ADDRESS_IPV6;
                IN6ADDR_SETSOCKADDR(&AddressInfo->Ipv6Address, &ipv6Address, scope, port);
            }

            if (PortNumber)
                *PortNumber = _byteswap_ushort(port);
            if (PrefixLength)
                *PrefixLength = UCHAR_MAX;

            return STATUS_SUCCESS;
        }
    }

    // IPv6 network: "addr/len" (no scope, no port, host bits clear).

    if ((Types & NET_STRING_IPV6_NETWORK) &&
        (separator = wcschr(NetworkString, L'/')) && separator[1] &&
        (prefix = PhpParsePrefixLength(separator + 1)) != UCHAR_MAX)
    {
        WCHAR addressString[PHP_IPV6_NETWORK_ADDRESS_BUFFER_LENGTH];

        if (NT_SUCCESS(RtlStringCchCopyNW(addressString, RTL_NUMBER_OF(addressString), NetworkString, separator - NetworkString)) &&
            PhIpv6StringToAddressEx(addressString, &ipv6Address, &scopeId, &port) == STATUS_SUCCESS &&
            port == 0 &&
            scopeId == 0)
        {
            ULONG index = prefix >> 3;
            BOOLEAN hostBitsClear = TRUE;

            if (prefix & 7)
            {
                if ((UCHAR)(ipv6Address.u.Byte[index] << (prefix & 7)) != 0)
                    hostBitsClear = FALSE;

                index++;
            }

            for (; hostBitsClear && index < sizeof(ipv6Address.u.Byte); index++)
            {
                if (ipv6Address.u.Byte[index] != 0)
                    hostBitsClear = FALSE;
            }

            if (hostBitsClear)
            {
                if (AddressInfo)
                {
                    SCOPE_ID scope;

                    scope.Value = 0;
                    AddressInfo->Format = NET_ADDRESS_IPV6;
                    IN6ADDR_SETSOCKADDR(&AddressInfo->Ipv6Address, &ipv6Address, scope, 0);
                }

                if (PrefixLength)
                    *PrefixLength = prefix;
                if (PortNumber)
                    *PortNumber = 0;

                return STATUS_SUCCESS;
            }
        }
    }

    // DNS name, with or without a port.

    if (Types & (NET_STRING_NAMED_ADDRESS | NET_STRING_NAMED_SERVICE))
    {
        PCWSTR colon;
        SIZE_T nameLength;
        WCHAR asciiName[DNS_MAX_NAME_BUFFER_LENGTH];
        LONG asciiLength;
        BOOLEAN valid;

        colon = wcschr(NetworkString, L':');

        if (colon ? !(Types & NET_STRING_NAMED_SERVICE) : !(Types & NET_STRING_NAMED_ADDRESS))
            return STATUS_INVALID_PARAMETER;

        port = 0;

        if (colon)
        {
            if (!colon[1] || !(port = PhpParsePortNumber(colon + 1)))
                return STATUS_INVALID_PARAMETER;

            nameLength = colon - NetworkString;
        }
        else
        {
            nameLength = PhCountStringZ((PWSTR)NetworkString);
        }

        if (!PhpDnsValidateName_W)
            PhpDnsValidateName_W = PhpGetDnsapiProcedureAddress("DnsValidateName_W");
        if (!PhpDnsValidateName_UTF8)
            PhpDnsValidateName_UTF8 = PhpGetDnsapiProcedureAddress("DnsValidateName_UTF8");
        if (!PhpDnsValidateName_W || !PhpDnsValidateName_UTF8)
            return STATUS_PROCEDURE_NOT_FOUND;

        asciiLength = RTL_NUMBER_OF(asciiName) - 1;

        if (!NT_SUCCESS(RtlIdnToAscii(0, NetworkString, (LONG)nameLength, asciiName, &asciiLength)))
            asciiLength = 0;

        if (asciiLength > 0)
        {
            asciiName[asciiLength] = UNICODE_NULL;
            valid = PhpDnsNameIsValid(FALSE, asciiName);
        }
        else
        {
            CHAR utf8Name[PHP_DNS_UTF8_NAME_BUFFER_LENGTH];
            ULONG utf8Length;
            NTSTATUS status;

            status = RtlUnicodeToUTF8N(
                utf8Name,
                sizeof(utf8Name) - 2,
                &utf8Length,
                NetworkString,
                (ULONG)(nameLength * sizeof(WCHAR))
                );

            if (!NT_SUCCESS(status) || !utf8Length)
                return STATUS_INVALID_PARAMETER;

            utf8Name[utf8Length] = ANSI_NULL;
            valid = PhpDnsNameIsValid(TRUE, utf8Name);
        }

        if (!valid)
            return STATUS_INVALID_PARAMETER;

        if (AddressInfo)
        {
            AddressInfo->Format = NET_ADDRESS_DNS_NAME;

            if (!NT_SUCCESS(RtlStringCchCopyNW(AddressInfo->NamedAddress.Address, RTL_NUMBER_OF(AddressInfo->NamedAddress.Address), NetworkString, nameLength)))
                return STATUS_BUFFER_TOO_SMALL;

            RtlStringCchCopyW(AddressInfo->NamedAddress.Port, RTL_NUMBER_OF(AddressInfo->NamedAddress.Port), colon ? colon + 1 : L"");
        }

        if (PrefixLength)
            *PrefixLength = UCHAR_MAX;
        if (PortNumber)
            *PortNumber = port;

        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

#pragma endregion
