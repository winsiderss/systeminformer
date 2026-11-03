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

#include <phapp.h>
#include <ntnsi.h>
#include <mapldr.h>
#include <netprv.h>
#include <netsup.h>
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
#define TCPIP_ADAPTER_PREFIX_LENGTH (RTL_NUMBER_OF(TCPIP_ADAPTER_PREFIX) - 1)
#define PH_IPV4_MAPPED_IN6_ADDRESS_OFFSET (FIELD_OFFSET(IN6_ADDR, s6_addr) + RTL_FIELD_SIZE(IN6_ADDR, s6_addr) - sizeof(IN_ADDR))
#define PH_NETIO_IPNET_FAMILY_COUNT 2
#define PH_ICMP_ECHO_DATA_SIZE 32
#define PH_ICMP_ECHO_PATTERN_LENGTH 23
#define PH_ICMP_ECHO_REPLY_BUFFER_SIZE 0x1000
#define PH_ICMP_ECHO_TIMEOUT 5000
#define PH_NETIO_RTT_SCALE 1000
#define PH_NSI_CONNECTIVITY_LOCAL_MASK 0x00000014
#define PH_NSI_CONNECTIVITY_INTERNET_MASK 0x00000028
#define PH_NSI_CONNECTIVITY_CONSTRAINED_MASK 0x000000C0
#define PH_NSI_CONNECTIVITY_COST_SHIFT 32
#define PH_NSI_CONNECTIVITY_COST_UNDETERMINED 0x04
#define PH_NSI_CONNECTIVITY_COST_FIXED 0x02
#define PH_NSI_CONNECTIVITY_COST_UNRESTRICTED 0x01
// WNF state name of the NCSI network connectivity state (8-byte value) watched by iphlpapi!NotifyNetworkConnectivityHintChange.
#define PH_WNF_CONNECTIVITY_STATE_NAME 0x418B0223A3BC2075ULL
#define PH_NET_LUID_INDEX_MASK 0x00FFFFFF
#define PH_NSI_INTERFACE_OBJECT_NAME_BUFFER_LENGTH 64
#define PH_INTERFACE_NAME_BUFFER_LENGTH 24
#define PH_INTERFACE_NAME_MAX_LENGTH 21
#define PH_MAC_ADDRESS_LENGTH 6
#define PH_MAC_ADDRESS_STRING_LENGTH (PH_MAC_ADDRESS_LENGTH * 3 - 1)
#define PH_IPV4_NETWORK_ADDRESS_BUFFER_LENGTH 22
#define PH_IPV6_NETWORK_ADDRESS_BUFFER_LENGTH 65
#define PH_DNS_UTF8_NAME_BUFFER_LENGTH 512
#define PH_NSI_ENUMERATION_MAX_ATTEMPTS 3
#define PH_NSI_ENUMERATION_GROWTH_DIVISOR 8
#define PH_NSI_ENUMERATION_MIN_GROWTH 10
#define PH_CONNECTIVITY_CALLBACK_PROCESSING 2
#define PH_TIMESTAMP_CONFIG_CHANGE_TAG 0x656D6974 // "time"

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
static RPC_BINDING_HANDLE NotificationRpcHandle;

static PH_QUEUED_LOCK PhpConnectivityHintLock = PH_QUEUED_LOCK_INIT;
static LIST_ENTRY PhpConnectivityHintListHead = { &PhpConnectivityHintListHead, &PhpConnectivityHintListHead };
static PVOID PhpConnectivitySubscriptionHandle;
static ULONG PhpConnectivityLevel;
static ULONG PhpConnectivityCost;

static LONG PingNotificationCountV4;
static LONG PingNotificationCountV6;
static HANDLE PingNotificationHandleV4;
static HANDLE PingNotificationHandleV6;
static PH_QUEUED_LOCK PingNotificationLock = PH_QUEUED_LOCK_INIT;

#pragma region NSI Wrappers

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
 * Converts a Win32 error code returned by nsi.dll, winnsi.dll or other IP Helper components into an NTSTATUS value.
 *
 * \param DosError The Win32 error code.
 * \return The NTSTATUS value. ERROR_MORE_DATA maps to STATUS_BUFFER_OVERFLOW (not the informational
 * STATUS_MORE_ENTRIES) so that NT_SUCCESS() fails and callers can retry with a larger buffer.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-rtlntstatustodoserror
 */
NTSTATUS PhNetioDosErrorToNtStatus(
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

/**
 * Retrieves a single NSI parameter block for an object (nsi!NsiGetParameter).
 *
 * \param Store The NSI store to query (active or persistent).
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param Key The key that identifies the object instance.
 * \param KeySize The size, in bytes, of Key.
 * \param ParameterType The parameter block to access (read-write, read-only dynamic or read-only static).
 * \param ParameterBuffer The buffer that receives the parameter data.
 * \param ParameterBufferSize The size, in bytes, of ParameterBuffer.
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
    _Out_writes_bytes_opt_(ParameterBufferSize) PVOID ParameterBuffer,
    _In_ ULONG ParameterBufferSize
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
    nsiGetParameter.Header.RequestHeader.ModuleId = ModuleId;
    nsiGetParameter.Header.RequestHeader.ObjectId = ObjectId;
    nsiGetParameter.Header.Store = Store;
    nsiGetParameter.Key = Key;
    nsiGetParameter.KeySize = KeySize;
    nsiGetParameter.ParameterType = ParameterType;
    nsiGetParameter.Buffer = ParameterBuffer;
    nsiGetParameter.BufferLength = ParameterBufferSize;
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

    return PhNetioDosErrorToNtStatus(NsiGetParameter_I(
        Store,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        ParameterType,
        ParameterBuffer,
        ParameterBufferSize,
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
 * \param ReadWriteBuffer The read-write parameter block.
 * \param ReadWriteBufferSize The size, in bytes, of the read-write parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntdeviceiocontrolfile
 */
NTSTATUS PhNsiSetAllParameters(
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(ReadWriteBufferSize) PVOID ReadWriteBuffer,
    _In_ ULONG ReadWriteBufferSize
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
    nsiSetAllParameters.Header.RequestHeader.ModuleId = ModuleId;
    nsiSetAllParameters.Header.RequestHeader.ObjectId = ObjectId;
    nsiSetAllParameters.Header.Store = Store;
    nsiSetAllParameters.Header.SetAction = Action;
    nsiSetAllParameters.Key = Key;
    nsiSetAllParameters.KeySize = KeySize;
    nsiSetAllParameters.ReadWriteBuffer = ReadWriteBuffer;
    nsiSetAllParameters.ReadWriteBufferSize = ReadWriteBufferSize;

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

    return PhNetioDosErrorToNtStatus(NsiSetAllParameters_I(
        Store,
        Action,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        ReadWriteBuffer,
        ReadWriteBufferSize
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
NTSTATUS PhNsiDeviceIoControl(
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
 */
NTSTATUS PhNsiAllocateTableBuffer(
    _In_ ULONG Count,
    _In_ ULONG EntrySize,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    SIZE_T size;

    *Buffer = NULL;

    if (!EntrySize)
        return STATUS_SUCCESS;

    status = RtlSIZETMult(Count, EntrySize, &size);

    if (!NT_SUCCESS(status))
        return status;

    if (!(*Buffer = PhAllocateZeroSafe(size)))
        return STATUS_NO_MEMORY;

    return STATUS_SUCCESS;
}

static HANDLE PhpNsiAsyncDeviceHandle = INVALID_HANDLE_VALUE;

/**
 * Returns a cached overlapped handle to \\Device\\Nsi used for change notifications, opening it on first use.
 *
 * \param DeviceHandle Receives the device handle. The handle is cached and must not be closed by the caller.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntdeviceiocontrolfile
 */
NTSTATUS PhNsiGetAsyncDeviceHandle(
    _Out_ PHANDLE DeviceHandle
    )
{
    HANDLE deviceHandle;
    HANDLE previousHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING deviceName;
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;

    deviceHandle = InterlockedCompareExchangePointer(
        (PVOID volatile*)&PhpNsiAsyncDeviceHandle,
        INVALID_HANDLE_VALUE,
        INVALID_HANDLE_VALUE
        );

    if (deviceHandle != INVALID_HANDLE_VALUE)
    {
        *DeviceHandle = deviceHandle;
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&deviceName, L"\\Device\\Nsi");
    InitializeObjectAttributes(
        &objectAttributes, 
        &deviceName,
        OBJ_CASE_INSENSITIVE,
        NULL, 
        NULL
        );

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
        return status;

    previousHandle = InterlockedCompareExchangePointer(
        (PVOID volatile*)&PhpNsiAsyncDeviceHandle,
        deviceHandle,
        INVALID_HANDLE_VALUE
        );

    if (previousHandle != INVALID_HANDLE_VALUE)
    {
        NtClose(deviceHandle);
        deviceHandle = previousHandle;
    }

    *DeviceHandle = deviceHandle;
    return STATUS_SUCCESS;
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
NTSTATUS PhNsiAsyncDeviceIoControl(
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

/**
 * Enumerates all instances of an NSI object and allocates the key and parameter tables (nsi!NsiAllocateAndGetTable). The enumeration is retried when the table grows between calls.
 *
 * \param Store The NSI store to query (active or persistent).
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param KeyTable Receives the allocated key table.
 * \param KeyEntrySize The size, in bytes, of each key entry.
 * \param ReadWriteTable Receives the allocated read-write table.
 * \param ReadWriteEntrySize The size, in bytes, of each read-write entry.
 * \param DynamicTable Receives the allocated read-only dynamic table.
 * \param DynamicEntrySize The size, in bytes, of each read-only dynamic entry.
 * \param StaticTable Receives the allocated read-only static table.
 * \param StaticEntrySize The size, in bytes, of each read-only static entry.
 * \param Count Receives the number of entries.
 * \param Unused Unused; pass zero.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \remarks Free the tables with PhNsiFreeTable.
 */
NTSTATUS PhNsiAllocateAndGetTable(
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * KeyEntrySize) PVOID* KeyTable,
    _In_ ULONG KeyEntrySize,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * ReadWriteEntrySize) PVOID* ReadWriteTable,
    _In_ ULONG ReadWriteEntrySize,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * DynamicEntrySize) PVOID* DynamicTable,
    _In_ ULONG DynamicEntrySize,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * StaticEntrySize) PVOID* StaticTable,
    _In_ ULONG StaticEntrySize,
    _Out_ PULONG Count,
    _In_ ULONG Unused
    )
{
#if !defined(PH_INLINE_NSI)
    NTSTATUS status;
    ULONG capacity;
    ULONG count;
    ULONG attempt;
    PVOID keyBuffer;
    PVOID rwBuffer;
    PVOID rodBuffer;
    PVOID rosBuffer;

    for (attempt = 0; attempt < PH_NSI_ENUMERATION_MAX_ATTEMPTS; attempt++)
    {
        count = 0;

        status = PhNsiEnumerateObjectsAllParameters(
            Store,
            NsiGetFirst,
            ModuleId,
            ObjectId,
            NULL,
            KeyEntrySize,
            NULL,
            ReadWriteEntrySize,
            NULL,
            DynamicEntrySize,
            NULL,
            StaticEntrySize,
            &count
            );

        if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW)
            return status;

        status = RtlULongAdd(count, max(count / PH_NSI_ENUMERATION_GROWTH_DIVISOR, PH_NSI_ENUMERATION_MIN_GROWTH), &capacity);

        if (!NT_SUCCESS(status))
            return status;

        keyBuffer = NULL;
        rwBuffer = NULL;
        rodBuffer = NULL;
        rosBuffer = NULL;

        if (KeyTable)
        {
            status = PhNsiAllocateTableBuffer(capacity, KeyEntrySize, &keyBuffer);

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }

        if (ReadWriteTable)
        {
            status = PhNsiAllocateTableBuffer(capacity, ReadWriteEntrySize, &rwBuffer);

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }

        if (DynamicTable)
        {
            status = PhNsiAllocateTableBuffer(capacity, DynamicEntrySize, &rodBuffer);

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }

        if (StaticTable)
        {
            status = PhNsiAllocateTableBuffer(capacity, StaticEntrySize, &rosBuffer);

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }

        count = capacity;

        status = PhNsiEnumerateObjectsAllParameters(
            Store,
            NsiGetFirst,
            ModuleId,
            ObjectId,
            keyBuffer,
            KeyEntrySize,
            rwBuffer,
            ReadWriteEntrySize,
            rodBuffer,
            DynamicEntrySize,
            rosBuffer,
            StaticEntrySize,
            &count);

        if (status == STATUS_BUFFER_OVERFLOW && attempt + 1 < PH_NSI_ENUMERATION_MAX_ATTEMPTS)
        {
            PhNsiFreeTable(keyBuffer, rwBuffer, rodBuffer, rosBuffer);
            continue;
        }

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        if (KeyTable && keyBuffer) *KeyTable = keyBuffer;
        if (ReadWriteTable && rwBuffer) *ReadWriteTable = rwBuffer;
        if (DynamicTable && rodBuffer) *DynamicTable = rodBuffer;
        if (StaticTable && rosBuffer) *StaticTable = rosBuffer;
        *Count = count;
        return STATUS_SUCCESS;
    }

    return STATUS_BUFFER_OVERFLOW;

CleanupExit:
    PhNsiFreeTable(keyBuffer, rwBuffer, rodBuffer, rosBuffer);
    return status;
#else
    if (!PhNsiApiInitialized() || !NsiAllocateAndGetTable_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiAllocateAndGetTable_I(
        Store,
        ModuleId,
        ObjectId,
        KeyTable,
        KeyEntrySize,
        ReadWriteTable,
        ReadWriteEntrySize,
        DynamicTable,
        DynamicEntrySize,
        StaticTable,
        StaticEntrySize,
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
 */
NTSTATUS PhNsiAllocateAndGetPersistentDataWithMaskTable(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * KeyEntrySize) PVOID* KeyTable,
    _In_ ULONG KeyEntrySize,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * ParameterDataEntrySize) PVOID* ParameterDataTable,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * ParameterDataEntrySize) PVOID* ParameterMaskTable,
    _In_ ULONG ParameterDataEntrySize,
    _Out_ PULONG Count
    )
{
#if !defined(PH_INLINE_NSI)
    NTSTATUS status;
    ULONG capacity;
    ULONG count;
    ULONG attempt;
    PVOID keyBuffer;
    PVOID dataBuffer;
    PVOID maskBuffer;

    for (attempt = 0; attempt < PH_NSI_ENUMERATION_MAX_ATTEMPTS; attempt++)
    {
        count = 0;
        status = PhNsiEnumerateObjectsAllPersistentParametersWithMask(
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

        if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW)
            return status;

        status = RtlULongAdd(count, max(count / PH_NSI_ENUMERATION_GROWTH_DIVISOR, PH_NSI_ENUMERATION_MIN_GROWTH), &capacity);

        if (!NT_SUCCESS(status))
            return status;

        keyBuffer = NULL;
        dataBuffer = NULL;
        maskBuffer = NULL;

        if (KeyTable)
        {
            status = PhNsiAllocateTableBuffer(capacity, KeyEntrySize, &keyBuffer);

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }

        if (ParameterDataTable)
        {
            status = PhNsiAllocateTableBuffer(capacity, ParameterDataEntrySize, &dataBuffer);

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }

        if (ParameterMaskTable)
        {
            status = PhNsiAllocateTableBuffer(capacity, ParameterDataEntrySize, &maskBuffer);

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }

        count = capacity;
        status = PhNsiEnumerateObjectsAllPersistentParametersWithMask(
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

        if (status == STATUS_BUFFER_OVERFLOW && attempt + 1 < PH_NSI_ENUMERATION_MAX_ATTEMPTS)
        {
            PhNsiFreePersistentDataWithMaskTable(keyBuffer, dataBuffer, maskBuffer);
            continue;
        }

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        if (KeyTable && keyBuffer)
            *KeyTable = keyBuffer;
        if (ParameterDataTable && dataBuffer)
            *ParameterDataTable = dataBuffer;
        if (ParameterMaskTable && maskBuffer)
            *ParameterMaskTable = maskBuffer;

        *Count = count;

        return STATUS_SUCCESS;
    }

    return STATUS_BUFFER_OVERFLOW;

CleanupExit:
    PhNsiFreePersistentDataWithMaskTable(keyBuffer, dataBuffer, maskBuffer);
    return status;
#else
    if (!PhNsiApiInitialized() || !NsiAllocateAndGetPersistentDataWithMaskTable_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiAllocateAndGetPersistentDataWithMaskTable_I(
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
 */
NTSTATUS PhNsiDeregisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
)
{
#if !defined(PH_INLINE_NSI)
    NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST request;

    RtlZeroMemory(&request, sizeof(NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST));
    request.ModuleId = ModuleId;
    request.ObjectId = ObjectId;
    request.NotificationHandle = NotificationHandle;

    return PhNsiDeregisterChangeNotificationEx(&request);
#else
    if (!PhNsiApiInitialized() || !NsiDeregisterChangeNotification_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiDeregisterChangeNotification_I(
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
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-cancelipchangenotify
 */
NTSTATUS PhNsiCancelChangeNotification(
    _In_opt_ HANDLE NotificationHandle
    )
{
#if !defined(PH_INLINE_NSI)
    HANDLE deviceHandle;
    NTSTATUS status;

    status = PhNsiGetAsyncDeviceHandle(&deviceHandle);

    if (!NT_SUCCESS(status))
        return status;

    return PhNsiAsyncDeviceIoControl(
        deviceHandle,
        IOCTL_NSI_CANCEL_CHANGE_NOTIFICATION,
        &NotificationHandle,
        sizeof(HANDLE)
        );
#else
    if (!PhNsiApiInitialized() || !NsiCancelChangeNotification_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiCancelChangeNotification_I(
        NotificationHandle
        ));
#endif
}

/**
 * Removes a change notification using a prepared request (nsi!NsiDeregisterChangeNotificationEx).
 *
 * \param Request The deregistration request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiDeregisterChangeNotificationEx(
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    )
{
#if !defined(PH_INLINE_NSI)
    return PhNsiDeviceIoControl(
        IOCTL_NSI_DEREGISTER_CHANGE_NOTIFICATION_EX,
        Request,
        sizeof(NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST),
        Request,
        sizeof(NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiDeregisterChangeNotificationEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiDeregisterChangeNotificationEx_I(
        Request
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
 * \param ReadWriteTable The table of read-write parameter blocks.
 * \param ReadWriteEntrySize The size, in bytes, of each read-write entry.
 * \param DynamicTable The table of read-only dynamic parameter blocks.
 * \param DynamicEntrySize The size, in bytes, of each read-only dynamic entry.
 * \param StaticTable The table of read-only static parameter blocks.
 * \param StaticEntrySize The size, in bytes, of each read-only static entry.
 * \param Count On input, the capacity of the tables in entries; on output, the number of entries returned.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
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
    )
{
#if !defined(PH_INLINE_NSI)
        NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX request;
        NTSTATUS status;
    
        RtlZeroMemory(&request, sizeof(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX));
        request.RequestHeader.ModuleId = ModuleId;
        request.RequestHeader.ObjectId = ObjectId;
        request.Store = Store;
        request.Action = Action;
        request.KeyTable = KeyTable;
        request.KeyEntrySize = KeyEntrySize;
        request.ReadWriteTable = ReadWriteTable;
        request.ReadWriteEntrySize = ReadWriteEntrySize;
        request.DynamicTable = DynamicTable;
        request.DynamicEntrySize = DynamicEntrySize;
        request.StaticTable = StaticTable;
        request.StaticEntrySize = StaticEntrySize;
        request.Count = *Count;

        status = PhNsiEnumerateObjectsAllParametersEx(&request);
        *Count = request.Count;

        return status;
#else
    if (!PhNsiApiInitialized() || !NsiEnumerateObjectsAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiEnumerateObjectsAllParameters_I(
        Store,
        Action,
        ModuleId,
        ObjectId,
        KeyTable,
        KeyEntrySize,
        ReadWriteTable,
        ReadWriteEntrySize,
        DynamicTable,
        DynamicEntrySize,
        StaticTable,
        StaticEntrySize,
        Count
        ));
#endif
}

/**
 * Enumerates the instances of an NSI object using a prepared request (nsi!NsiEnumerateObjectsAllParametersEx).
 *
 * \param Request The enumeration request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiEnumerateObjectsAllParametersEx(
    _Inout_ PNSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX Request
    )
{
#if !defined(PH_INLINE_NSI)
    return PhNsiDeviceIoControl(
        IOCTL_NSI_GET_ALL_PARAMETERS,
        Request,
        sizeof(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX),
        Request,
        sizeof(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiEnumerateObjectsAllParametersEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiEnumerateObjectsAllParametersEx_I(
        Request
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
 */
NTSTATUS PhNsiEnumerateObjectsAllPersistentParametersWithMask(
    _In_ NSI_GET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _Out_writes_bytes_opt_(*Count* KeyEntrySize) PVOID KeyTable,
    _In_ ULONG KeyEntrySize,
    _Out_writes_bytes_opt_(*Count* ParameterDataEntrySize) PVOID ParameterDataTable,
    _Out_writes_bytes_opt_(*Count* ParameterDataEntrySize) PVOID ParameterMaskTable,
    _In_ ULONG ParameterDataEntrySize,
    _Inout_ PULONG Count
)
{
#if !defined(PH_INLINE_NSI)
    NSI_PERSISTENT_ENUMERATE_REQUEST request;
    NTSTATUS status;

    RtlZeroMemory(&request, sizeof(NSI_PERSISTENT_ENUMERATE_REQUEST));
    request.RequestHeader.ModuleId = ModuleId;
    request.RequestHeader.ObjectId = ObjectId;
    request.Action = Action;
    request.KeyTable = KeyTable;
    request.KeyEntrySize = KeyEntrySize;
    request.ParameterDataTable = ParameterDataTable;
    request.ParameterMaskTable = ParameterMaskTable;
    request.ParameterDataEntrySize = ParameterDataEntrySize;
    request.Count = *Count;

    status = PhNsiDeviceIoControl(
        IOCTL_NSI_ENUMERATE_OBJECTS_ALL_PERSISTENT_PARAMETERS_WITH_MASK,
        &request,
        sizeof(NSI_PERSISTENT_ENUMERATE_REQUEST),
        &request,
        sizeof(NSI_PERSISTENT_ENUMERATE_REQUEST),
        NULL
        );

    *Count = request.Count;

    return status;
#else
    if (!PhNsiApiInitialized() || !NsiEnumerateObjectsAllPersistentParametersWithMask_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiEnumerateObjectsAllPersistentParametersWithMask_I(
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
 * \param Request The get parameter request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiGetParameterEx(
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST Request
    )
{
#if !defined(PH_INLINE_NSI)
    ULONG returnedLength;

    return PhNsiDeviceIoControl(
        IOCTL_NSI_GET_PARAMETER,
        Request,
        sizeof(NSI_GET_PARAMETER_EX_REQUEST),
        Request,
        sizeof(NSI_GET_PARAMETER_EX_REQUEST),
        &returnedLength
        );
#else
    if (!PhNsiApiInitialized() || !NsiGetParameterEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiGetParameterEx_I(
        Request
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
 * \param ReadWriteBuffer The read-write parameter block.
 * \param ReadWriteBufferSize The size, in bytes, of the read-write parameter block.
 * \param DynamicBuffer The read-only dynamic parameter block.
 * \param DynamicBufferSize The size, in bytes, of the read-only dynamic parameter block.
 * \param StaticBuffer The read-only static parameter block.
 * \param StaticBufferSize The size, in bytes, of the read-only static parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
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
    )
{
#if !defined(PH_INLINE_NSI)
    NSI_GET_ALL_PARAMETERS_EX_REQUEST request;
    NTSTATUS status;

    RtlZeroMemory(&request, sizeof(NSI_GET_ALL_PARAMETERS_EX_REQUEST));
    request.Header.RequestHeader.ModuleId = ModuleId;
    request.Header.RequestHeader.ObjectId = ObjectId;
    request.Header.Store = Store;
    request.Key = Key;
    request.KeySize = KeySize;
    request.ReadWriteBuffer = ReadWriteBuffer;
    request.ReadWriteBufferSize = ReadWriteBufferSize;
    request.DynamicBuffer = DynamicBuffer;
    request.DynamicBufferSize = DynamicBufferSize;
    request.StaticBuffer = StaticBuffer;
    request.StaticBufferSize = StaticBufferSize;

    status = PhNsiGetAllParametersEx(&request);

    if (NT_SUCCESS(status) && request.ReadWriteBufferSize < ReadWriteBufferSize)
        return STATUS_INVALID_PARAMETER;

    return status;
#else
    if (!PhNsiApiInitialized() || !NsiGetAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiGetAllParameters_I(
        Store,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        ReadWriteBuffer,
        ReadWriteBufferSize,
        DynamicBuffer,
        DynamicBufferSize,
        StaticBuffer,
        StaticBufferSize
        ));
#endif
}

/**
 * Retrieves all parameter blocks of an NSI object instance using a prepared request (nsi!NsiGetAllParametersEx).
 *
 * \param Request The get all parameters request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiGetAllParametersEx(
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST Request
    )
{
#if !defined(PH_INLINE_NSI)
    return PhNsiDeviceIoControl(
        IOCTL_NSI_GET_ALL_PARAMETERS_EX,
        Request,
        sizeof(NSI_GET_ALL_PARAMETERS_EX_REQUEST),
        Request,
        sizeof(NSI_GET_ALL_PARAMETERS_EX_REQUEST),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiGetAllParametersEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiGetAllParametersEx_I(
        Request
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
    NSI_SET_PARAMETER_EX_REQUEST request;

    RtlZeroMemory(&request, sizeof(NSI_SET_PARAMETER_EX_REQUEST));
    request.Header.RequestHeader.ModuleId = ModuleId;
    request.Header.RequestHeader.ObjectId = ObjectId;
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

    return PhNetioDosErrorToNtStatus(NsiSetParameter_I(
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
 * \param Request The set parameter request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiSetParameterEx(
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST Request
    )
{
#if !defined(PH_INLINE_NSI)
    ULONG returnedLength;

    return PhNsiDeviceIoControl(
        IOCTL_NSI_SET_PARAMETER,
        Request,
        sizeof(NSI_SET_PARAMETER_EX_REQUEST),
        Request,
        sizeof(NSI_SET_PARAMETER_EX_REQUEST),
        &returnedLength
        );
#else
    if (!PhNsiApiInitialized() || !NsiSetParameterEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiSetParameterEx_I(
        Request
        ));
#endif
}

/**
 * Sets the read-write parameters of an NSI object using a prepared request (nsi!NsiSetAllParametersEx).
 *
 * \param Request The set all parameters request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiSetAllParametersEx(
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST Request
    )
{
#if !defined(PH_INLINE_NSI)
    return PhNsiDeviceIoControl(
        IOCTL_NSI_SET_ALL_PARAMETERS,
        Request,
        sizeof(NSI_SET_ALL_PARAMETERS_EX_REQUEST),
        Request,
        sizeof(NSI_SET_ALL_PARAMETERS_EX_REQUEST),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiSetAllParametersEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiSetAllParametersEx_I(
        Request
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
 */
NTSTATUS PhNsiRegisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ PNSI_CHANGE_CALLBACK ChangeCallback,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Outptr_result_maybenull_ PHANDLE NotificationHandle
    )
{
#if !defined(PH_INLINE_NSI)
    NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST request;

    RtlZeroMemory(&request, sizeof(NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST));
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

    return PhNetioDosErrorToNtStatus(NsiRegisterChangeNotification_I(
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
 * \param Request The registration request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiRegisterChangeNotificationEx(
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    )
{
#if !defined(PH_INLINE_NSI)
    if (Request->ChangeCallback)
        return STATUS_INVALID_PARAMETER;

    return PhNsiDeviceIoControl(
        IOCTL_NSI_REGISTER_CHANGE_NOTIFICATION_EX,
        Request,
        sizeof(NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST),
        Request,
        sizeof(NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiRegisterChangeNotificationEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRegisterChangeNotificationEx_I(
        Request
        ));
#endif
}

/**
 * Requests a one-shot change notification on an NSI object (nsi!NsiRequestChangeNotification).
 *
 * \param[in] ModuleHandle The module handle; pass zero.
 * \param[in] ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param[in] ObjectId The NSI object identifier within the module.
 * \param[in] Overlapped An optional OVERLAPPED structure for asynchronous completion.
 * \param[out] NotificationHandle Receives the handle used to wait for the notification.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-notifyaddrchange
 */
NTSTATUS PhNsiRequestChangeNotification(
    _In_ ULONG ModuleHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ LPOVERLAPPED Overlapped,
    _Out_ PHANDLE NotificationHandle
    )
{
#if !defined(PH_INLINE_NSI)
    NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST request;

    RtlZeroMemory(&request, sizeof(NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST));
    request.ModuleHandle = ModuleHandle;
    request.ModuleId = ModuleId;
    request.ObjectIndex = ObjectId;
    request.Overlapped = Overlapped;
    request.NotificationHandle = NotificationHandle;

    return PhNsiRequestChangeNotificationEx(&request);
#else
    if (!PhNsiApiInitialized() || !NsiRequestChangeNotification_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRequestChangeNotification_I(
        ModuleHandle,
        ModuleId,
        ObjectId,
        Overlapped,
        NotificationHandle
        ));
#endif
}

/**
 * Requests a one-shot change notification using a prepared request (nsi!NsiRequestChangeNotificationEx).
 *
 * \param Request The change notification request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiRequestChangeNotificationEx(
    _Inout_ PNSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST Request
    )
{
#if !defined(PH_INLINE_NSI)
    HANDLE deviceHandle;
    NTSTATUS status;

    status = PhNsiGetAsyncDeviceHandle(&deviceHandle);

    if (!NT_SUCCESS(status))
        return status;

    if (Request->Overlapped)
    {
        Request->Overlapped->Internal = STATUS_PENDING;
        Request->Overlapped->InternalHigh = 0;

        status = NtDeviceIoControlFile(
            deviceHandle,
            Request->Overlapped->hEvent,
            NULL,
            NULL,
            (PIO_STATUS_BLOCK)Request->Overlapped,
            IOCTL_NSI_REQUEST_CHANGE_NOTIFICATION_EX,
            Request,
            sizeof(NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST),
            NULL,
            0
            );
    }
    else
    {
        status = PhNsiAsyncDeviceIoControl(
            deviceHandle, 
            IOCTL_NSI_REQUEST_CHANGE_NOTIFICATION_EX, 
            Request, 
            sizeof(NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST)
            );
    }

    if (Request->Overlapped && Request->NotificationHandle)
        *Request->NotificationHandle = deviceHandle;

    return status;
#else
    if (!PhNsiApiInitialized() || !NsiRequestChangeNotificationEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRequestChangeNotificationEx_I(
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
    NSI_PERSISTENT_GET_REQUEST request;

    RtlZeroMemory(&request, sizeof(NSI_PERSISTENT_GET_REQUEST));
    request.RequestHeader.ModuleId = ModuleId;
    request.RequestHeader.ObjectId = ObjectId;
    request.Key = Key;
    request.KeySize = KeySize;
    request.ParameterData = ParameterData;
    request.ParameterMask = ParameterMask;
    request.ParameterDataSize = ParameterDataSize;

    return PhNsiDeviceIoControl(
        IOCTL_NSI_GET_ALL_PERSISTENT_PARAMETERS_WITH_MASK,
        &request,
        sizeof(NSI_PERSISTENT_GET_REQUEST),
        &request,
        sizeof(NSI_PERSISTENT_GET_REQUEST),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiGetAllPersistentParametersWithMask_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiGetAllPersistentParametersWithMask_I(
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
    NSI_PERSISTENT_SET_REQUEST request;

    RtlZeroMemory(&request, sizeof(NSI_PERSISTENT_SET_REQUEST));
    request.RequestHeader.ModuleId = ModuleId;
    request.RequestHeader.ObjectId = ObjectId;
    request.Action = Action;
    request.Key = Key;
    request.KeySize = KeySize;
    request.ParameterData = ParameterData;
    request.ParameterMask = ParameterMask;
    request.ParameterDataSize = ParameterDataSize;

    return PhNsiDeviceIoControl(
        IOCTL_NSI_SET_ALL_PERSISTENT_PARAMETERS_WITH_MASK,
        &request,
        sizeof(NSI_PERSISTENT_SET_REQUEST),
        &request,
        sizeof(NSI_PERSISTENT_SET_REQUEST),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiSetAllPersistentParametersWithMask_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiSetAllPersistentParametersWithMask_I(
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
 * \param Request The object security request block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiGetObjectSecurity(
    _Inout_ PNSI_OBJECT_SECURITY_REQUEST Request
    )
{
#if !defined(PH_INLINE_NSI)
    return PhNsiDeviceIoControl(
        IOCTL_NSI_GET_OBJECT_SECURITY,
        Request,
        sizeof(NSI_OBJECT_SECURITY_REQUEST),
        Request,
        sizeof(NSI_OBJECT_SECURITY_REQUEST),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiGetObjectSecurity_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiGetObjectSecurity_I(
        Request
        ));
#endif
}

/**
 * Sets the security descriptor of an NSI object (nsi!NsiSetObjectSecurity).
 *
 * \param Request The object security request block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiSetObjectSecurity(
    _Inout_ PNSI_OBJECT_SECURITY_REQUEST Request
    )
{
#if !defined(PH_INLINE_NSI)
    return PhNsiDeviceIoControl(
        IOCTL_NSI_SET_OBJECT_SECURITY,
        Request,
        sizeof(NSI_OBJECT_SECURITY_REQUEST),
        Request,
        sizeof(NSI_OBJECT_SECURITY_REQUEST),
        NULL
        );
#else
    if (!PhNsiApiInitialized() || !NsiSetObjectSecurity_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiSetObjectSecurity_I(
        Request
        ));
#endif
}

/**
 * Writes the security descriptor of an NSI registry key to the debug console in SDDL form.
 *
 * \param KeyHandle A handle to the key, opened with READ_CONTROL access.
 * \param ModuleName The name of the parent module key, or NULL if the key is a module key.
 * \param ObjectName The name of the key.
 */
static VOID PhpNsiDumpKeySecurity(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRING ModuleName,
    _In_ PPH_STRING ObjectName
    )
{
    const SECURITY_INFORMATION securityInformation = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
    PSECURITY_DESCRIPTOR securityDescriptor;
    PPH_STRING sddl = NULL;
    NTSTATUS status;

    status = PhGetObjectSecurity(KeyHandle, securityInformation, &securityDescriptor);

    if (NT_SUCCESS(status))
    {
        status = PhGetSecurityDescriptorAsString(securityDescriptor, securityInformation, &sddl);
        PhFree(securityDescriptor);
    }

    if (ModuleName)
    {
        if (NT_SUCCESS(status))
            dprintf("[NSI] %ws\\%ws SDDL=%ws\n", ModuleName->Buffer, ObjectName->Buffer, sddl->Buffer);
        else
            dprintf("[NSI] %ws\\%ws security status=0x%08lx\n", ModuleName->Buffer, ObjectName->Buffer, status);
    }
    else
    {
        if (NT_SUCCESS(status))
            dprintf("[NSI] %ws SDDL=%ws\n", ObjectName->Buffer, sddl->Buffer);
        else
            dprintf("[NSI] %ws security status=0x%08lx\n", ObjectName->Buffer, status);
    }

    if (NT_SUCCESS(status))
        PhDereferenceObject(sddl);
}

/**
 * Key enumeration callback that dumps the security descriptor of an NSI object key.
 *
 * \param RootDirectory A handle to the parent module key.
 * \param Information A KEY_BASIC_INFORMATION structure describing the object key.
 * \param Context The module name (PPH_STRING) of the parent key.
 * \return TRUE to continue the enumeration.
 */
static BOOLEAN NTAPI PhpNsiDumpObjectSecurityKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    PKEY_BASIC_INFORMATION basicInformation = Information;
    PPH_STRING moduleName = Context;
    PH_STRINGREF keyName;
    PPH_STRING objectName;
    HANDLE keyHandle;
    NTSTATUS status;

    keyName.Buffer = basicInformation->Name;
    keyName.Length = basicInformation->NameLength;
    objectName = PhCreateString2(&keyName);

    status = PhOpenKey(&keyHandle, READ_CONTROL, RootDirectory, &keyName, 0);

    if (NT_SUCCESS(status))
    {
        PhpNsiDumpKeySecurity(keyHandle, moduleName, objectName);
        NtClose(keyHandle);
    }
    else
    {
        dprintf("[NSI] %ws\\%ws open status=0x%08lx\n", moduleName->Buffer, objectName->Buffer, status);
    }

    PhDereferenceObject(objectName);

    return TRUE;
}

/**
 * Key enumeration callback that dumps the security descriptor of an NSI module key
 * and of each object key beneath it.
 *
 * \param RootDirectory A handle to the parent NSI key.
 * \param Information A KEY_BASIC_INFORMATION structure describing the module key.
 * \param Context Unused.
 * \return TRUE to continue the enumeration.
 */
static BOOLEAN NTAPI PhpNsiDumpModuleSecurityKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    PKEY_BASIC_INFORMATION basicInformation = Information;
    PH_STRINGREF keyName;
    PPH_STRING moduleName;
    HANDLE keyHandle;
    NTSTATUS status;

    keyName.Buffer = basicInformation->Name;
    keyName.Length = basicInformation->NameLength;
    moduleName = PhCreateString2(&keyName);

    status = PhOpenKey(&keyHandle, READ_CONTROL, RootDirectory, &keyName, 0);

    if (NT_SUCCESS(status))
    {
        PhpNsiDumpKeySecurity(keyHandle, NULL, moduleName);
        NtClose(keyHandle);
    }
    else
    {
        dprintf("[NSI] %ws security open status=0x%08lx\n", moduleName->Buffer, status);
    }

    status = PhOpenKey(&keyHandle, KEY_ENUMERATE_SUB_KEYS, RootDirectory, &keyName, 0);

    if (NT_SUCCESS(status))
    {
        status = PhEnumerateKey(keyHandle, KeyBasicInformation, PhpNsiDumpObjectSecurityKeyCallback, moduleName);

        if (!NT_SUCCESS(status))
            dprintf("[NSI] %ws enumerate status=0x%08lx\n", moduleName->Buffer, status);

        NtClose(keyHandle);
    }
    else
    {
        dprintf("[NSI] %ws enumeration open status=0x%08lx\n", moduleName->Buffer, status);
    }

    PhDereferenceObject(moduleName);

    return TRUE;
}

/**
 * The PhNsiDumpObjectSecurityDescriptors routine prints the SDDL of every NSI module and explicit object key.
 * The module key also supplies the security descriptor for object IDs without their own key.
 */
VOID PhNsiDumpObjectSecurityDescriptors(
    VOID
    )
{
    static const PH_STRINGREF nsiKeyName = PH_STRINGREF_INIT(L"SYSTEM\\CurrentControlSet\\Control\\Nsi");
    HANDLE keyHandle;
    NTSTATUS status;

    status = PhOpenKey(&keyHandle, KEY_ENUMERATE_SUB_KEYS, PH_KEY_LOCAL_MACHINE, &nsiKeyName, 0);

    if (!NT_SUCCESS(status))
    {
        dprintf("[NSI] open status=0x%08lx\n", status);
        return;
    }

    status = PhEnumerateKey(keyHandle, KeyBasicInformation, PhpNsiDumpModuleSecurityKeyCallback, NULL);

    if (!NT_SUCCESS(status))
        dprintf("[NSI] enumerate status=0x%08lx\n", status);

    NtClose(keyHandle);
}

/**
 * Connects to the NSI RPC server (nsi!NsiConnectToServer).
 *
 * \param Reserved Reserved; pass NULL.
 * \param RpcHandle Receives the RPC binding handle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiConnectToServer(
    _Reserved_ PVOID Reserved,
    _Out_ RPC_BINDING_HANDLE* RpcHandle
    )
{
    RPC_BINDING_HANDLE rpcHandle;
    NTSTATUS status;

    if (!PhNsiApiInitialized() || !NsiConnectToServer_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    rpcHandle = NsiConnectToServer_I(
        Reserved
        );

    if (!rpcHandle)
    {
        status = PhNetioDosErrorToNtStatus(PhGetLastError());

        if (NT_SUCCESS(status))
            status = STATUS_UNSUCCESSFUL;

        return status;
    }

    *RpcHandle = rpcHandle;
    return STATUS_SUCCESS;
}

/**
 * Disconnects from the NSI RPC server (nsi!NsiDisconnectFromServer).
 *
 * \param RpcHandle The RPC binding handle returned by PhNsiConnectToServer.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiDisconnectFromServer(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle
    )
{
    if (!PhNsiApiInitialized() || !NsiDisconnectFromServer_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiDisconnectFromServer_I(
        RpcHandle
        ));
}

/**
 * Frees the tables returned by PhNsiAllocateAndGetPersistentDataWithMaskTable (nsi!NsiFreePersistentDataWithMaskTable).
 *
 * \param KeyTable The key table.
 * \param ParameterDataTable The parameter data table.
 * \param ParameterMaskTable The parameter mask table.
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
 * \param ReadWriteTable The read-write table.
 * \param DynamicTable The read-only dynamic table.
 * \param StaticTable The read-only static table.
 */
VOID PhNsiFreeTable(
    _Frees_ptr_opt_ PVOID KeyTable,
    _Frees_ptr_opt_ PVOID ReadWriteTable,
    _Frees_ptr_opt_ PVOID DynamicTable,
    _Frees_ptr_opt_ PVOID StaticTable
    )
{
#if !defined(PH_INLINE_NSI)
    if (KeyTable) PhFree(KeyTable);
    if (ReadWriteTable) PhFree(ReadWriteTable);
    if (DynamicTable) PhFree(DynamicTable);
    if (StaticTable) PhFree(StaticTable);
#else
    if (!PhNsiApiInitialized() || !NsiFreeTable_I)
        return;

    NsiFreeTable_I(
        KeyTable,
        ReadWriteTable,
        DynamicTable,
        StaticTable
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
 */
NTSTATUS PhNsiRpcDeregisterChangeNotification(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcDeregisterChangeNotification_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcDeregisterChangeNotification_I(
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
 * \param Request The deregistration request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiRpcDeregisterChangeNotificationEx(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcDeregisterChangeNotificationEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcDeregisterChangeNotificationEx_I(
        RpcHandle,
        Request
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
 * \param ReadWriteTable The table of read-write parameter blocks.
 * \param ReadWriteEntrySize The size, in bytes, of each read-write entry.
 * \param DynamicTable The table of read-only dynamic parameter blocks.
 * \param DynamicEntrySize The size, in bytes, of each read-only dynamic entry.
 * \param StaticTable The table of read-only static parameter blocks.
 * \param StaticEntrySize The size, in bytes, of each read-only static entry.
 * \param Count On input, the capacity of the tables in entries; on output, the number of entries returned.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
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
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcEnumerateObjectsAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcEnumerateObjectsAllParameters_I(
        RpcHandle,
        Store,
        Action,
        ModuleId,
        ObjectId,
        KeyTable,
        KeyEntrySize,
        ReadWriteTable,
        ReadWriteEntrySize,
        DynamicTable,
        DynamicEntrySize,
        StaticTable,
        StaticEntrySize,
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
 * \param ReadWriteBuffer The read-write parameter block.
 * \param ReadWriteBufferSize The size, in bytes, of the read-write parameter block.
 * \param DynamicBuffer The read-only dynamic parameter block.
 * \param DynamicBufferSize The size, in bytes, of the read-only dynamic parameter block.
 * \param StaticBuffer The read-only static parameter block.
 * \param StaticBufferSize The size, in bytes, of the read-only static parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
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
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcGetAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcGetAllParameters_I(
        RpcHandle,
        Store,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        ReadWriteBuffer,
        ReadWriteBufferSize,
        DynamicBuffer,
        DynamicBufferSize,
        StaticBuffer,
        StaticBufferSize
        ));
}

/**
 * Retrieves all parameter blocks over RPC using a prepared request (nsi!NsiRpcGetAllParametersEx).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param Request The get all parameters request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiRpcGetAllParametersEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST Request
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcGetAllParametersEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcGetAllParametersEx_I(
        RpcHandle,
        Request
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
 * \param ParameterBuffer The buffer that receives the parameter data.
 * \param ParameterBufferSize The size, in bytes, of ParameterBuffer.
 * \param ParameterOffset The offset, in bytes, of the parameter within the parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
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
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcGetParameter_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcGetParameter_I(
        RpcHandle,
        Store,
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
 * Retrieves a single NSI parameter over RPC using a prepared request (nsi!NsiRpcGetParameterEx).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param Request The get parameter request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiRpcGetParameterEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST Request
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcGetParameterEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcGetParameterEx_I(
        RpcHandle,
        Request
        ));
}

/**
 * Registers a callback for NSI object changes over RPC (nsi!NsiRpcRegisterChangeNotification).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param ModuleId The NPI module identifier of the provider (for example NPI_MS_TCP_MODULEID).
 * \param ObjectId The NSI object identifier within the module.
 * \param ChangeCallback The callback invoked when the object changes.
 * \param NotifyCurrent TRUE to invoke the callback immediately for the current state.
 * \param CallerContext A caller-defined context passed to the callback.
 * \param NotificationHandle Receives the notification handle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiRpcRegisterChangeNotification(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ PNSI_CHANGE_CALLBACK ChangeCallback,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Out_opt_ PHANDLE NotificationHandle
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcRegisterChangeNotification_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcRegisterChangeNotification_I(
        RpcHandle,
        ModuleId,
        ObjectId,
        ChangeCallback,
        NotifyCurrent,
        CallerContext,
        NotificationHandle
        ));
}

/**
 * Registers a callback for NSI object changes over RPC using a prepared request (nsi!NsiRpcRegisterChangeNotificationEx).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param Request The registration request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiRpcRegisterChangeNotificationEx(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcRegisterChangeNotificationEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcRegisterChangeNotificationEx_I(
        RpcHandle,
        Request
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
 * \param ReadWriteBuffer The read-write parameter block.
 * \param ReadWriteBufferSize The size, in bytes, of the read-write parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
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
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcSetAllParameters_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcSetAllParameters_I(
        RpcHandle,
        Store,
        Action,
        ModuleId,
        ObjectId,
        Key,
        KeySize,
        ReadWriteBuffer,
        ReadWriteBufferSize
        ));
}

/**
 * Sets the read-write parameters over RPC using a prepared request (nsi!NsiRpcSetAllParametersEx).
 *
 * \param RpcHandle The NSI RPC binding handle returned by PhNsiConnectToServer.
 * \param Request The set all parameters request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiRpcSetAllParametersEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST Request
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcSetAllParametersEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcSetAllParametersEx_I(
        RpcHandle,
        Request
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
 */
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
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcSetParameter_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcSetParameter_I(
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
 * \param Request The set parameter request.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiRpcSetParameterEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST Request
    )
{
    if (!PhNsiApiInitialized() || !NsiRpcSetParameterEx_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    return PhNetioDosErrorToNtStatus(NsiRpcSetParameterEx_I(
        RpcHandle,
        Request
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
 * \param ParameterBuffer The buffer that receives the parameter data.
 * \param ParameterBufferSize The size, in bytes, of ParameterBuffer.
 * \param ParameterOffset The offset, in bytes, of the parameter within the parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
static NTSTATUS PhpNsiGetParameter(
    _In_ NSI_STORE Store,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_ NSI_PARAMETER_TYPE ParameterType,
    _Out_writes_bytes_opt_(ParameterBufferSize) PVOID ParameterBuffer,
    _In_ ULONG ParameterBufferSize,
    _In_ ULONG ParameterOffset
    )
{
    NSI_GET_PARAMETER_EX_REQUEST request;

    RtlZeroMemory(&request, sizeof(NSI_GET_PARAMETER_EX_REQUEST));
    request.Header.RequestHeader.ModuleId = ModuleId;
    request.Header.RequestHeader.ObjectId = ObjectId;
    request.Header.Store = Store;
    request.Key = Key;
    request.KeySize = KeySize;
    request.ParameterType = ParameterType;
    request.Buffer = ParameterBuffer;
    request.BufferLength = ParameterBufferSize;
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
 * \param ReadWriteBuffer The read-write parameter block.
 * \param ReadWriteBufferSize The size, in bytes, of the read-write parameter block.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
static NTSTATUS PhpNsiSetAllParameters(
    _In_ NSI_STORE Store,
    _In_ ULONG Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(ReadWriteBufferSize) PVOID ReadWriteBuffer,
    _In_ ULONG ReadWriteBufferSize
    )
{
    NSI_SET_ALL_PARAMETERS_EX_REQUEST request;

    RtlZeroMemory(&request, sizeof(NSI_SET_ALL_PARAMETERS_EX_REQUEST));
    request.Header.RequestHeader.ModuleId = ModuleId;
    request.Header.RequestHeader.ObjectId = ObjectId;
    request.Header.Store = Store;
    request.Header.SetAction = (NSI_SET_ACTION)Action;
    request.Key = Key;
    request.KeySize = KeySize;
    request.ReadWriteBuffer = ReadWriteBuffer;
    request.ReadWriteBufferSize = ReadWriteBufferSize;

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
        NsiSetTcpEntryData.LocalScopeID.Value = NetworkItem->LocalScopeId;

        NsiSetTcpEntryData.RemoteFamily = AF_INET6;
        NsiSetTcpEntryData.RemotePort = _byteswap_ushort((USHORT)NetworkItem->RemoteEndpoint.Port);
        memcpy(&NsiSetTcpEntryData.RemoteAddressIPv6, &NetworkItem->RemoteEndpoint.Address.In6Addr, sizeof(IN6_ADDR));
        NsiSetTcpEntryData.RemoteScopeID.Value = NetworkItem->RemoteScopeId;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
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

    context = (ICMP_ECHO_REQUEST_CONTEXT*)ApcArgument1;
    context->ApcRoutine(context->ApcContext, &context->IoStatusBlock, 0);

    PhIcmpDestroyEchoRequestContext(context);
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

    RtlZeroMemory(CompartmentGuid, sizeof(GUID));

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
 * \return The compartment identifier, or NET_IF_COMPARTMENT_ID_PRIMARY on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getcurrentthreadcompartmentid
 */
NET_IF_COMPARTMENT_ID PhGetCurrentThreadCompartmentId(
    VOID
    )
{
    NET_IF_COMPARTMENT_ID current;

    current = NET_IF_COMPARTMENT_ID_UNSPECIFIED;

    if (NT_SUCCESS(PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectCurrentThreadCompartment,
        NULL,
        0,
        NsiParameterTypeReadWrite,
        &current,
        sizeof(NET_IF_COMPARTMENT_ID),
        NsiNdisCurrentThreadCompartmentType0Id
        )))
    {
        return current;
    }

    return NET_IF_COMPARTMENT_ID_PRIMARY;
}

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

    if (!InterfaceAlias || !InterfaceLuid || PhCountStringZ(InterfaceAlias) > MAXIMUM_FILENAME_LENGTH)
        return STATUS_INVALID_PARAMETER;

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

    if (!InterfaceLuid || !InterfaceAlias)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(&Alias, sizeof(NSI_STRING_256));
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

    CurrentLength = Alias.CountedString.Length >> 1;
    if (CurrentLength > MAXIMUM_FILENAME_LENGTH)
    {
        CurrentLength = MAXIMUM_FILENAME_LENGTH;
    }

    Alias.CountedString.String[CurrentLength] = UNICODE_NULL;
    return PhCopyStringZ(Alias.CountedString.String, CurrentLength, InterfaceAlias, Length, NULL);
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
        RtlZeroMemory(InterfaceGuid, sizeof(GUID));
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
    WCHAR guidString[RTL_GUID_STRING_SIZE + 1];
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE fileHandle;
    PH_STRINGREF stringRef;
    SIZE_T returnLength;
    PH_FORMAT format[2];
    WCHAR formatBuffer[PH_NSI_INTERFACE_OBJECT_NAME_BUFFER_LENGTH];
    NTSTATUS status;

    if (InterfaceLuid == NULL || InterfaceHandle == NULL)
        return STATUS_INVALID_PARAMETER;

    *InterfaceHandle = NULL;
    fileHandle = NULL;
    interfaceGuid = GUID_NULL;

    status = PhConvertInterfaceLuidToGuid(InterfaceLuid, &interfaceGuid);
    if (!NT_SUCCESS(status))
        return status;

    status = PhFormatGuidToBuffer(&interfaceGuid, guidString, sizeof(guidString), NULL);
    if (!NT_SUCCESS(status))
        return status;

    PhInitFormatS(&format[0], TCPIP_ADAPTER_PREFIX);
    PhInitFormatS(&format[1], guidString);

    if (!PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        return STATUS_NAME_TOO_LONG;

    stringRef.Length = returnLength - sizeof(UNICODE_NULL);
    stringRef.Buffer = formatBuffer;

    if (!PhStringRefToUnicodeString(&stringRef, &objectName))
        return STATUS_NAME_TOO_LONG;

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

    if (!NT_SUCCESS(status))
        return status;

    *InterfaceHandle = fileHandle;

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
        else if (NT_SUCCESS(Status))
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
 */
static VOID PhFillVirtualInterfaceRow(
    _Out_ PMIB_FL_VIRTUAL_INTERFACE_ROW InterfaceRow,
    _In_ ADDRESS_FAMILY Family,
    _In_ const NSI_FL_VIRTUAL_INTERFACE_KEY* Key,
    _In_ const NSI_FL_VIRTUAL_INTERFACE_RW* ReadWrite,
    _In_ const NSI_FL_VIRTUAL_INTERFACE_ROD* ReadOnlyDynamic,
    _In_ const NSI_FL_VIRTUAL_INTERFACE_ROS* ReadOnlyStatic
    )
{
    RtlZeroMemory(InterfaceRow, sizeof(MIB_FL_VIRTUAL_INTERFACE_ROW));
    InterfaceRow->Family = Family;
    InterfaceRow->IfLuid = Key->InterfaceLuid;
    InterfaceRow->VirtualIfId = Key->VirtualInterfaceId;
    InterfaceRow->CompartmentGuid = Key->CompartmentGuid;
    InterfaceRow->IsolationMode = Key->IsolationMode;
    InterfaceRow->Origin = ReadOnlyStatic->VirtualInterfaceType;
    InterfaceRow->VirtualIfLuid = ReadOnlyStatic->VirtualInterfaceLuid;
    InterfaceRow->VirtualIfIndex = ReadOnlyDynamic->VirtualInterfaceIndex;
    InterfaceRow->AllowLocalNd = ReadWrite->ClientInterfaceFlag;
    InterfaceRow->AttachedFlsnpiClients = ReadOnlyDynamic->AttachedFlsnpiClients;
    InterfaceRow->FlsnpiClientConfigErrors = ReadOnlyDynamic->FlsnpiClientConfigErrors;
    InterfaceRow->FlsnpiClientInjectErrors = ReadOnlyDynamic->FlsnpiClientInjectErrors;
    InterfaceRow->FlsnpiClientCloneErrors = ReadOnlyDynamic->FlsnpiClientCloneErrors;
    InterfaceRow->InFlsnpiIndicatedPackets = ReadOnlyDynamic->InFlsnpiIndicatedPackets;
    InterfaceRow->InFlsnpiClientReturnedPackets = ReadOnlyDynamic->InFlsnpiClientReturnedPackets;
    InterfaceRow->InFlsnpiClientSilentlyDroppedPackets = ReadOnlyDynamic->InFlsnpiClientSilentlyDroppedPackets;
    InterfaceRow->InFlsnpiClientDroppedPackets = ReadOnlyDynamic->InFlsnpiClientDroppedPackets;
    InterfaceRow->InFlsnpiClientInjectedPackets = ReadOnlyDynamic->InFlsnpiClientInjectedPackets;
    InterfaceRow->InFlsnpiClientClonedPackets = ReadOnlyDynamic->InFlsnpiClientClonedPackets;
    InterfaceRow->OutFlsnpiIndicatedPackets = ReadOnlyDynamic->OutFlsnpiIndicatedPackets;
    InterfaceRow->OutFlsnpiClientReturnedPackets = ReadOnlyDynamic->OutFlsnpiClientReturnedPackets;
    InterfaceRow->OutFlsnpiClientDroppedPackets = ReadOnlyDynamic->OutFlsnpiClientDroppedPackets;
    InterfaceRow->OutFlsnpiClientSilentlyDroppedPackets = ReadOnlyDynamic->OutFlsnpiClientSilentlyDroppedPackets;
    InterfaceRow->OutFlsnpiClientInjectedPackets = ReadOnlyDynamic->OutFlsnpiClientInjectedPackets;
    InterfaceRow->OutFlsnpiClientClonedPackets = ReadOnlyDynamic->OutFlsnpiClientClonedPackets;
    InterfaceRow->OutFlsnpiClientClonedPacketsForNbSplit = ReadOnlyDynamic->OutFlsnpiClientClonedPacketsForNbSplit;
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
    WCHAR PrefixBuffer[PH_INTERFACE_NAME_BUFFER_LENGTH];
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
    WCHAR LocalName[PH_INTERFACE_NAME_BUFFER_LENGTH];
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

    Remaining = PH_INTERFACE_NAME_MAX_LENGTH;
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
    Remaining = PH_INTERFACE_NAME_MAX_LENGTH;
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
    if (!PhStringToInteger64(&indexSr, 10, &value) || value > 0xFFFFFF)
        return STATUS_OBJECT_NAME_INVALID;

    InterfaceLuid->Info.NetLuidIndex = (ULONG)value;

    if (wcsncmp(LocalName, Prefix, PhCountStringZ(Prefix)) == 0)
    {
        PH_STRINGREF typeSr;

        PhInitializeStringRef(&typeSr, LocalName + PhCountStringZ(Prefix));
        typeSr.Length = RTL_PTR_OFFSET(typeSr.Buffer, Separator);

        if (PhStringToInteger64(&typeSr, 10, &value) && value <= MAXUSHORT)
            InterfaceLuid->Info.IfType = (USHORT)value;
        else
            return STATUS_OBJECT_NAME_INVALID;

        return STATUS_SUCCESS;
    }

    for (Index = 0; Index < ARRAYSIZE(IfTypeStringTable); ++Index)
    {
        size_t PrefixLength = (size_t)(Separator - LocalName);

        if (PrefixLength == PhCountStringZ(IfTypeStringTable[Index].Name) &&
            wcsncmp(LocalName, IfTypeStringTable[Index].Name, PrefixLength) == 0)
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

    RtlZeroMemory(&Alias, sizeof(NSI_STRING_256));

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
        CurrentLength = Alias.CountedString.Length >> 1;
        if (CurrentLength > MAXIMUM_FILENAME_LENGTH)
        {
            CurrentLength = MAXIMUM_FILENAME_LENGTH;
        }

        Alias.CountedString.String[CurrentLength] = UNICODE_NULL;
        AliasString = Alias.CountedString.String;
    }
    else
    {
        AliasString = L"";
    }

    if (!NT_SUCCESS(Status))
    {
        if (Length)
            InterfaceAlias[0] = UNICODE_NULL;
        return Status;
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
        RtlZeroMemory(InterfaceGuid, sizeof(GUID));
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

    RtlZeroMemory(&UnicodeGuid, sizeof(UNICODE_STRING));

    if (SourceString && Guid)
    {
        RtlInitUnicodeString(&UnicodeGuid, SourceString);

        if (NT_SUCCESS(RtlGUIDFromString(&UnicodeGuid, Guid)))
        {
            return STATUS_SUCCESS;
        }
    }

    return STATUS_INVALID_PARAMETER;
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
    _Out_writes_bytes_(PH_MAC_ADDRESS_LENGTH) PBYTE InterfacePhysicalAddress
    )
{
    size_t Length;
    ULONG Index;

    Length = PhCountStringZ(InterfacePhysicalAddressString);

    if (Length != PH_MAC_ADDRESS_STRING_LENGTH)
    {
        return STATUS_INVALID_PARAMETER;
    }

    for (Index = 0; Index < PH_MAC_ADDRESS_STRING_LENGTH; ++Index)
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

/**
 * Converts an ICMPv4 error type and code into an IP_STATUS code.
 *
 * \param Type The ICMPv4 message type (ICMP4_*).
 * \param Code The ICMPv4 message code.
 * \return The IP_STATUS code; IP_GENERAL_FAILURE for unrecognized messages.
 */
ULONG PhIcmpv4ErrorToIpStatus(
    _In_ LONG Type,
    _In_ ULONG Code
    )
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

/**
 * Converts an ICMPv6 error type and code into an IP_STATUS code.
 *
 * \param Type The ICMPv6 message type (ICMP6_*).
 * \param Code The ICMPv6 message code.
 * \return The IP_STATUS code; IP_GENERAL_FAILURE for unrecognized messages.
 */
ULONG PhIcmpv6ErrorToIpStatus(
    _In_ LONG Type, 
    _In_ LONG Code
    )
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
 * \param StatusBlock The notification header of the request (status, ICMP error indicator, type and code).
 * \param Ipv4 TRUE for an IPv4 request, FALSE for IPv6.
 * \return The IP_STATUS value.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2
 */
ULONG PhNtStatusToIpStatus(
    _In_ const ICMP_NOTIFICATION_HEADER* StatusBlock,
    _In_ BOOLEAN Ipv4
    )
{
    if (StatusBlock->Status == NO_ERROR)
        return IP_SUCCESS;

    if (StatusBlock->Status == STATUS_UNSUCCESSFUL)
    {
        if (StatusBlock->HasIcmpError == 0)
            return IP_GENERAL_FAILURE;

        return Ipv4
            ? PhIcmpv4ErrorToIpStatus((LONG)(StatusBlock->IcmpType), StatusBlock->IcmpCode)
            : PhIcmpv6ErrorToIpStatus((LONG)(StatusBlock->IcmpType), (LONG)(StatusBlock->IcmpCode));
    }

    if (StatusBlock->Status == STATUS_INSUFFICIENT_RESOURCES)
        return IP_NO_RESOURCES;
    if (StatusBlock->Status == STATUS_DUPLICATE_NAME)
        return IP_DUPLICATE_ADDRESS;
    if (StatusBlock->Status == STATUS_CANCELLED)
        return IP_GENERAL_FAILURE;
    if (StatusBlock->Status == STATUS_INVALID_BUFFER_SIZE)
        return IP_PACKET_TOO_BIG;
    if (StatusBlock->Status == STATUS_IPSEC_QUEUE_OVERFLOW)
        return IP_NEGOTIATING_IPSEC;
    if (StatusBlock->Status == STATUS_TIMEOUT)
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
 * \param RpcHandle Receives the shared RPC binding handle. The handle is cached and must not be disconnected by the caller.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhNsiNotificationConnectToServer(
    _Out_ RPC_BINDING_HANDLE* RpcHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PhAcquireQueuedLockExclusive(&NotificationCriticalSection);

    if (NotificationRpcHandle == NULL)
    {
        status = PhNsiConnectToServer(NULL, &NotificationRpcHandle);
    }

    if (NT_SUCCESS(status))
    {
        *RpcHandle = NotificationRpcHandle;
    }

    PhReleaseQueuedLockExclusive(&NotificationCriticalSection);

    return status;
}

/**
 * NSI callback that receives an IPv4 echo reply, copies it into the caller's reply buffer and completes the request.
 *
 * \param CallerContext The ICMP handle passed at registration (unused).
 * \param ObjectKey The echo request that completed (ICMP_REQUEST_RW**).
 * \param ObjectParameter The NSI notification message that carries the reply (ICMP_NOTIFICATION_MESSAGE*).
 * \param NotificationType The kind of change (unused).
 * \return ERROR_SUCCESS (NO_ERROR) on success, otherwise a Win32 error code.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2
 */
ULONG NTAPI PhIcmpIndicateParameterChange(
    _In_opt_ PVOID CallerContext,
    _In_opt_ PVOID ObjectKey,
    _In_opt_ PVOID ObjectParameter,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    ICMP_REQUEST_RW** RequestPointer = ObjectKey;
    ICMP_NOTIFICATION_MESSAGE* Notification = ObjectParameter;
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
        NsiIcmpSetActionCompleteEchoRequest,
        handle->ModuleId,
        NsiIpObjectEchoRequest,
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
    RtlZeroMemory(reply, sizeof(ICMP_ECHO_REPLY));

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

    reply->Status = PhNtStatusToIpStatus(&notificationData->Header, TRUE);
    requestContext->IoStatusBlock.Status = notificationData->Header.Status;
    requestContext->IoStatusBlock.Information = dataLength + optionsSize + sizeof(ICMP_ECHO_REPLY);
    PhIcmpEchoRequestComplete((ULONG_PTR)requestContext);
    return reply->Status;
}

/**
 * NSI callback that receives an IPv6 echo reply, copies it into the caller's reply buffer and completes the request.
 *
 * \param CallerContext The ICMP handle passed at registration (unused).
 * \param ObjectKey The echo request that completed (ICMP_REQUEST_RW**).
 * \param ObjectParameter The NSI notification message that carries the reply (ICMP_NOTIFICATION_MESSAGE*).
 * \param NotificationType The kind of change (unused).
 * \return ERROR_SUCCESS (NO_ERROR) on success, otherwise a Win32 error code.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6sendecho2
 */
ULONG NTAPI PhIcmp6IndicateParameterChange(
    _In_opt_ PVOID CallerContext,
    _In_opt_ PVOID ObjectKey,
    _In_opt_ PVOID ObjectParameter,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    ICMP_REQUEST_RW** RequestPointer = ObjectKey;
    ICMP_NOTIFICATION_MESSAGE* Notification = ObjectParameter;
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
        NsiIcmpSetActionCompleteEchoRequest,
        handle->ModuleId,
        NsiIpObjectEchoRequest,
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
    RtlZeroMemory(reply, sizeof(ICMPV6_ECHO_REPLY));

    if ((notificationData->Header.Status == STATUS_UNSUCCESSFUL && notificationData->Header.HasIcmpError != 0)
        || notificationData->Header.Status == 0)
    {
        memcpy(reply->Address.sin6_addr, &notificationData->Address, sizeof(IN6_ADDR));
        reply->Address.sin6_scope_id = notificationData->ScopeId.Value;
        reply->RoundTripTime = notificationData->RoundTripTime;
        replySize = sizeof(ICMPV6_ECHO_REPLY) + notificationData->DataLength;
    }

    reply->Status = PhNtStatusToIpStatus(&notificationData->Header, FALSE);
    requestContext->IoStatusBlock.Status = notificationData->Header.Status;
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
        NsiIcmpSetActionSubmitEchoRequest,
        handle->ModuleId,
        NsiIpObjectEchoRequest,
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

    if (Context->ThreadHandle)
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

    if (WaitEvent)
        *WaitEvent = NULL;

    completionEvent = Event;
    duplicatedThread = NULL;

    if (Event)
    {
        *IsAsynchronous = TRUE;
    }
    else if (ApcRoutine)
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
        NTSTATUS status = PhCreateEvent(
            &completionEvent, 
            EVENT_ALL_ACCESS,
            SynchronizationEvent, 
            FALSE
            );

        if (!NT_SUCCESS(status))
            return status;

        if (WaitEvent)
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

    RtlZeroMemory(context, sizeof(ICMP_ECHO_REQUEST_CONTEXT));
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
 * \param IcmpHandle Receives the ICMP handle. Close it with PhIcmpCloseHandle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpcreatefile
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6createfile
 */
NTSTATUS PhIcmpCreateFileEx(
    _In_ const NPI_MODULEID* ModuleId,
    _In_ BOOLEAN IsIpv4,
    _Out_ PHANDLE IcmpHandle
    )
{
    ICMP_HANDLE_INTERNAL* handle;
    RPC_BINDING_HANDLE rpcHandle;
    NTSTATUS status;

    handle = (ICMP_HANDLE_INTERNAL*)PhAllocateZeroSafe(sizeof(ICMP_HANDLE_INTERNAL));
    if (handle == NULL)
        return STATUS_NO_MEMORY;

    status = PhNsiNotificationConnectToServer(&rpcHandle);
    if (!NT_SUCCESS(status))
    {
        PhFree(handle);
        return status;
    }

    PhAcquireQueuedLockExclusive(&PingNotificationLock);

    if (IsIpv4)
    {
        if (PingNotificationCountV4 == 0)
        {
            PingNotificationHandleV4 = NULL;

            status = PhNsiRpcRegisterChangeNotification(
                rpcHandle,
                ModuleId,
                NsiIpObjectEchoRequest,
                PhIcmpIndicateParameterChange,
                FALSE,
                handle,
                &PingNotificationHandleV4
                );

            if (!NT_SUCCESS(status))
            {
                PhReleaseQueuedLockExclusive(&PingNotificationLock);
                PhFree(handle);
                return status;
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
                NsiIpObjectEchoRequest,
                PhIcmp6IndicateParameterChange,
                FALSE,
                handle,
                &PingNotificationHandleV6
                );

            if (!NT_SUCCESS(status))
            {
                PhReleaseQueuedLockExclusive(&PingNotificationLock);
                PhFree(handle);
                return status;
            }
        }

        ++PingNotificationCountV6;
    }

    PhReleaseQueuedLockExclusive(&PingNotificationLock);

    RtlZeroMemory(handle, sizeof(ICMP_HANDLE_INTERNAL));
    handle->ProcessId = HandleToUlong(NtCurrentProcessId());
    handle->IsIpv4 = IsIpv4;
    handle->ModuleId = ModuleId;
    RtlInitializeCriticalSection(&handle->Lock);
    InitializeListHead(&handle->RequestList);

    *IcmpHandle = handle;
    return STATUS_SUCCESS;
}

/**
 * Opens a handle on which IPv6 ICMP echo requests can be issued.
 *
 * \param IcmpHandle Receives the ICMP handle. Close it with PhIcmpCloseHandle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6createfile
 */
NTSTATUS PhIcmp6CreateFile(
    _Out_ PHANDLE IcmpHandle
    )
{
    return PhIcmpCreateFileEx(&NPI_MS_IPV6_MODULEID, FALSE, IcmpHandle);
}

/**
 * Opens a handle on which IPv4 ICMP echo requests can be issued.
 *
 * \param IcmpHandle Receives the ICMP handle. Close it with PhIcmpCloseHandle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpcreatefile
 */
NTSTATUS PhIcmpCreateFile(
    _Out_ PHANDLE IcmpHandle
    )
{
    return PhIcmpCreateFileEx(&NPI_MS_IPV4_MODULEID, TRUE, IcmpHandle);
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
 * \param ReplyCount Receives 1 if the buffer holds a successful reply, otherwise 0; the IP_STATUS of a failed request is
 * in the Status field of the reply.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6parsereplies
 */
NTSTATUS PhIcmp6ParseReplies(
    _In_reads_bytes_(ReplySize) PVOID ReplyBuffer,
    _In_ ULONG ReplySize,
    _Out_ PULONG ReplyCount
    )
{
    IP_STATUS ipStatus;

    if (ReplyBuffer == NULL || ReplySize < sizeof(ICMPV6_ECHO_REPLY))
        return STATUS_INVALID_PARAMETER;

    ipStatus = ((PICMPV6_ECHO_REPLY)(ReplyBuffer))->Status;
    *ReplyCount = (ipStatus == IP_SUCCESS || ipStatus == IP_TTL_EXPIRED_TRANSIT) ? 1 : 0;
    return STATUS_SUCCESS;
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
 * \param ReplyCount Receives the number of replies for synchronous requests; the IP_STATUS of a failed request is in the
 * Status field of the reply.
 * \return An NTSTATUS value; STATUS_PENDING for asynchronous requests. Use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6sendecho2
 */
NTSTATUS PhIcmp6SendEcho2(
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
    _In_ ULONG Timeout,
    _Out_opt_ PULONG ReplyCount
    )
{
    ICMP_HANDLE_INTERNAL* handle;
    ICMP_ECHO_REQUEST_CONTEXT* requestContext;
    ICMP_REQUEST_V6 request;
    HANDLE waitEvent;
    BOOLEAN asynchronous;
    NTSTATUS status;
    ULONG replyCount;

    if (PhIsInvalidIcmpHandle(IcmpHandle))
        return STATUS_INVALID_HANDLE;

    handle = (ICMP_HANDLE_INTERNAL*)(IcmpHandle);
    if (handle->IsIpv4 || ReplySize < sizeof(ICMPV6_ECHO_REPLY))
        return STATUS_INVALID_PARAMETER;

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
        return status;

    RtlZeroMemory(&request, sizeof(ICMP_REQUEST_V6));
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
        request.Reverse = (RequestOptions->Flags & IP_FLAG_REVERSE) != 0;
        request.OptionsSize = RequestOptions->OptionsSize;
        request.OptionsData = RequestOptions->OptionsData;
    }
    else
    {
        request.Ttl = MAXUCHAR;
    }
    request.SourceScopeId.Value = SourceAddress->sin6_scope_id;
    request.DestinationScopeId.Value = DestinationAddress->sin6_scope_id;
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

        PhIcmpDestroyEchoRequestContext(requestContext);
        return status;
    }

    if (asynchronous)
        return STATUS_PENDING;

    NtWaitForSingleObject(waitEvent, FALSE, NULL);
    NtClose(waitEvent);

    status = PhIcmp6ParseReplies(ReplyBuffer, ReplySize, &replyCount);

    if (NT_SUCCESS(status) && ReplyCount)
        *ReplyCount = replyCount;

    return status;
}

/**
 * Parses the reply buffer of an IPv4 echo request.
 *
 * \param ReplyBuffer The buffer that receives the echo replies.
 * \param ReplySize The size, in bytes, of ReplyBuffer.
 * \param ReplyCount Receives the number of replies in the buffer; when it is 0 the IP_STATUS of the failed request is in
 * the Status field of the reply.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpparsereplies
 */
NTSTATUS PhIcmpParseReplies(
    _In_reads_bytes_(ReplySize) PVOID ReplyBuffer,
    _In_ ULONG ReplySize,
    _Out_ PULONG ReplyCount
    )
{
    PICMP_ECHO_REPLY reply;

    if (ReplyBuffer == NULL || ReplySize < sizeof(ICMP_ECHO_REPLY))
        return STATUS_INVALID_PARAMETER;

    reply = (PICMP_ECHO_REPLY)(ReplyBuffer);
    *ReplyCount = reply->Reserved;
    reply->Reserved = 0;
    return STATUS_SUCCESS;
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
 * \param ReplyCount Receives the number of replies for synchronous requests; when it is 0 the IP_STATUS of the failed
 * request is in the Status field of the reply.
 * \return An NTSTATUS value; STATUS_PENDING for asynchronous requests. Use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2ex
 */
NTSTATUS PhIcmpSendEcho2Ex(
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
    _In_ ULONG Timeout,
    _Out_opt_ PULONG ReplyCount
    )
{
    ICMP_HANDLE_INTERNAL* handle;
    ICMP_ECHO_REQUEST_CONTEXT* requestContext;
    ICMP_REQUEST_V4 request;
    HANDLE waitEvent;
    BOOLEAN asynchronous;
    NTSTATUS status;
    ULONG parsedReplySize;
    ULONG replyCount;
    PICMP_ECHO_REPLY reply;

    if (PhIsInvalidIcmpHandle(IcmpHandle))
        return STATUS_INVALID_HANDLE;

    handle = (ICMP_HANDLE_INTERNAL*)(IcmpHandle);
    if (!handle->IsIpv4 || ReplySize < sizeof(ICMP_ECHO_REPLY))
        return STATUS_INVALID_PARAMETER;

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
        return status;

    RtlZeroMemory(&request, sizeof(ICMP_REQUEST_V4));
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
        request.Reverse = (RequestOptions->Flags & IP_FLAG_REVERSE) != 0;
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
        if (waitEvent)
            NtClose(waitEvent);
        PhIcmpDestroyEchoRequestContext(requestContext);
        return status;
    }

    if (asynchronous)
        return STATUS_PENDING;

    NtWaitForSingleObject(waitEvent, FALSE, NULL);
    NtClose(waitEvent);

    reply = (PICMP_ECHO_REPLY)(ReplyBuffer);
    parsedReplySize = sizeof(ICMP_ECHO_REPLY) + reply->DataSize + reply->Options.OptionsSize;
    status = PhIcmpParseReplies(ReplyBuffer, parsedReplySize, &replyCount);

    if (NT_SUCCESS(status) && ReplyCount)
        *ReplyCount = replyCount;

    return status;
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
 * \param ReplyCount Receives the number of replies; when it is 0 the IP_STATUS of the failed request is in the Status
 * field of the reply.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho
 */
NTSTATUS PhIcmpSendEcho(
    _In_ HANDLE IcmpHandle,
    _In_ IPAddr DestinationAddress,
    _In_reads_bytes_(RequestSize) PVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) PVOID ReplyBuffer,
    _In_ ULONG ReplySize,
    _In_ ULONG Timeout,
    _Out_opt_ PULONG ReplyCount
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
        Timeout,
        ReplyCount
        );
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
 * \param ReplyCount Receives the number of replies for synchronous requests; when it is 0 the IP_STATUS of the failed
 * request is in the Status field of the reply.
 * \return An NTSTATUS value; STATUS_PENDING for asynchronous requests. Use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho2
 */
NTSTATUS PhIcmpSendEcho2(
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
    _In_ ULONG Timeout,
    _Out_opt_ PULONG ReplyCount
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
        Timeout,
        ReplyCount
        );
}

/**
 * Retrieves ICMP statistics for IPv4 or IPv6.
 *
 * \param Statistics Receives the statistics.
 * \param Family The address family (AF_INET or AF_INET6).
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

    RtlZeroMemory(buffer, sizeof(MIB_ICMP_EX_XPSP1));
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

    RtlZeroMemory(&extendedStatistics, sizeof(MIB_ICMP_EX));
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
 * \param IcmpHandle Receives the ICMP handle. Close it with PhIcmpCloseHandle.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpcreatefile
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmp6createfile
 */
NTSTATUS PhInternalIcmpCreateFileEx(
    _In_ BOOLEAN IsIpv4,
    _Out_ PHANDLE IcmpHandle
    )
{
    return PhIcmpCreateFileEx(IsIpv4 ? &NPI_MS_IPV4_MODULEID : &NPI_MS_IPV6_MODULEID, IsIpv4 != 0, IcmpHandle);
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
 * \param InterfaceIndex Receives the interface index.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-convertinterfaceluidtoindex
 */
NTSTATUS PhQueryNeighborInterfaceIndex(
    _In_ const NET_LUID* InterfaceLuid,
    _Out_ PNET_IFINDEX InterfaceIndex
    )
{
    NTSTATUS status;
    NET_IFINDEX ifIndex = 0;
    NET_LUID interfaceLuid = *InterfaceLuid;

    status = PhpNsiGetParameter(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &interfaceLuid,
        sizeof(NET_LUID),
        NsiParameterTypeReadOnlyStatic,
        &ifIndex,
        sizeof(NET_IFINDEX),
        NsiNdisInterfaceLuidToIndex
        );

    if (!NT_SUCCESS(status))
        return status;

    *InterfaceIndex = ifIndex;
    return STATUS_SUCCESS;
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
    NSI_OBJECT_ID objectId;
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
            objectId = NsiTcpObjectEstatsSynOpts;
            rwBufferSize = 0;
            rodBufferSize = 0;
            rosBufferSize = sizeof(TCP_ESTATS_SYN_OPTS_ROS_v0);
            break;
        case TcpConnectionEstatsData:
            objectId = NsiTcpObjectEstatsData;
            rwBufferSize = sizeof(TCP_ESTATS_DATA_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_DATA_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsSndCong:
            objectId = NsiTcpObjectEstatsSndCong;
            rwBufferSize = sizeof(NSI_ESTATS_SND_CONG_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_SND_CONG_ROD_v0);
            rosBufferSize = sizeof(TCP_ESTATS_SND_CONG_ROS_v0);
            break;
        case TcpConnectionEstatsPath:
            objectId = NsiTcpObjectEstatsPath;
            rwBufferSize = sizeof(TCP_ESTATS_PATH_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_PATH_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsSendBuff:
            objectId = NsiTcpObjectEstatsSendBuff;
            rwBufferSize = sizeof(TCP_ESTATS_SEND_BUFF_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_SEND_BUFF_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsRec:
            objectId = NsiTcpObjectEstatsRec;
            rwBufferSize = sizeof(TCP_ESTATS_REC_RW_v0);
            rodBufferSize = sizeof(NSI_ESTATS_REC_ROD_INTERNAL);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsObsRec:
            objectId = NsiTcpObjectEstatsObsRec;
            rwBufferSize = sizeof(TCP_ESTATS_OBS_REC_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_OBS_REC_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsBandwidth:
            objectId = NsiTcpObjectEstatsBandwidth;
            rwBufferSize = sizeof(TCP_ESTATS_BANDWIDTH_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_BANDWIDTH_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsFineRtt:
            objectId = NsiTcpObjectEstatsFineRtt;
            rwBufferSize = sizeof(TCP_ESTATS_FINE_RTT_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_FINE_RTT_ROD_v0);
            rosBufferSize = 0;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }

        RtlZeroMemory(&key, sizeof(NSI_ESTATS_TCP_KEY_V4));
        key.AddressFamily = AF_INET;
        key.LocalPort = (USHORT)Row->dwLocalPort;
        key.LocalAddress = Row->dwLocalAddr;
        key.RemoteAddressFamily = AF_INET;
        key.RemoteAddress = Row->dwRemoteAddr;
        key.RemotePort = (USHORT)Row->dwRemotePort;

        if (RwSize && rwBufferSize != 0)
        {
            rwBuffer = PhAllocateSafe(rwBufferSize);

            if (!rwBuffer)
            {
                status = STATUS_NO_MEMORY;
                goto Cleanup;
            }
        }

        if (RodSize && rodBufferSize != 0)
        {
            rodBuffer = PhAllocateSafe(rodBufferSize);

            if (!rodBuffer)
            {
                status = STATUS_NO_MEMORY;
                goto Cleanup;
            }
        }

        if (RosSize && rosBufferSize != 0)
        {
            rosBuffer = PhAllocateSafe(rosBufferSize);

            if (!rosBuffer)
            {
                status = STATUS_NO_MEMORY;
                goto Cleanup;
            }
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

                    NSI_ESTATS_REC_ROD_INTERNAL* src;
                    PTCP_ESTATS_REC_ROD_v0 dst;
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
    NSI_OBJECT_ID objectId;
    NTSTATUS status;
    NSI_ESTATS_TCP_KEY key;

    if ((!RwSize || Rw) && (!RosSize || Ros) && (Rod || !RodSize))
    {
        // Determine object ID and buffer sizes based on EstatsType
        switch (EstatsType)
        {
        case TcpConnectionEstatsSynOpts:
            objectId = NsiTcpObjectEstatsSynOpts;
            rwBufferSize = 0;
            rodBufferSize = 0;
            rosBufferSize = sizeof(TCP_ESTATS_SYN_OPTS_ROS_v0);
            break;
        case TcpConnectionEstatsData:
            objectId = NsiTcpObjectEstatsData;
            rwBufferSize = sizeof(TCP_ESTATS_DATA_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_DATA_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsSndCong:
            objectId = NsiTcpObjectEstatsSndCong;
            rwBufferSize = sizeof(NSI_ESTATS_SND_CONG_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_SND_CONG_ROD_v0);
            rosBufferSize = sizeof(TCP_ESTATS_SND_CONG_ROS_v0);
            break;
        case TcpConnectionEstatsPath:
            objectId = NsiTcpObjectEstatsPath;
            rwBufferSize = sizeof(TCP_ESTATS_PATH_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_PATH_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsSendBuff:
            objectId = NsiTcpObjectEstatsSendBuff;
            rwBufferSize = sizeof(TCP_ESTATS_SEND_BUFF_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_SEND_BUFF_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsRec:
            objectId = NsiTcpObjectEstatsRec;
            rwBufferSize = sizeof(TCP_ESTATS_REC_RW_v0);
            rodBufferSize = sizeof(NSI_ESTATS_REC_ROD_INTERNAL);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsObsRec:
            objectId = NsiTcpObjectEstatsObsRec;
            rwBufferSize = sizeof(TCP_ESTATS_OBS_REC_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_OBS_REC_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsBandwidth:
            objectId = NsiTcpObjectEstatsBandwidth;
            rwBufferSize = sizeof(TCP_ESTATS_BANDWIDTH_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_BANDWIDTH_ROD_v0);
            rosBufferSize = 0;
            break;
        case TcpConnectionEstatsFineRtt:
            objectId = NsiTcpObjectEstatsFineRtt;
            rwBufferSize = sizeof(TCP_ESTATS_FINE_RTT_RW_v0);
            rodBufferSize = sizeof(TCP_ESTATS_FINE_RTT_ROD_v0);
            rosBufferSize = 0;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }

        RtlZeroMemory(&key, sizeof(NSI_ESTATS_TCP_KEY));
        key.AddressFamily = AF_INET6;
        key.LocalPort = (USHORT)Row->dwLocalPort;
        key.LocalAddressIPv6 = Row->LocalAddr;
        key.LocalScopeId.Value = Row->dwLocalScopeId;
        key.RemoteAddressFamily = AF_INET6;
        key.RemotePort = (USHORT)Row->dwRemotePort;
        key.RemoteAddressIPv6 = Row->RemoteAddr;
        key.RemoteScopeId.Value = Row->dwRemoteScopeId;

        //key.LocalPort = (USHORT)_byteswap_ushort(Row->dwLocalPort);
        //key.RemotePort = (USHORT)_byteswap_ushort(Row->dwRemotePort);

        if (RwSize && rwBufferSize != 0)
        {
            rwBuffer = PhAllocateSafe(rwBufferSize);

            if (!rwBuffer)
            {
                status = STATUS_NO_MEMORY;
                goto Cleanup;
            }
        }

        if (RodSize && rodBufferSize != 0)
        {
            rodBuffer = PhAllocateSafe(rodBufferSize);

            if (!rodBuffer)
            {
                status = STATUS_NO_MEMORY;
                goto Cleanup;
            }
        }

        if (RosSize && rosBufferSize != 0)
        {
            rosBuffer = PhAllocateSafe(rosBufferSize);

            if (!rosBuffer)
            {
                status = STATUS_NO_MEMORY;
                goto Cleanup;
            }
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
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpclosehandle
 */
NTSTATUS PhIcmpCloseHandle(
    _In_ HANDLE IcmpHandle
    )
{
    ICMP_HANDLE_INTERNAL* handle;
    HANDLE notificationHandle;
    BOOLEAN deregister;

    if (PhIsInvalidIcmpHandle(IcmpHandle))
        return STATUS_INVALID_HANDLE;

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
            NsiIpObjectEchoRequest,
            notificationHandle
            );
    }

    PhReleaseQueuedLockExclusive(&PingNotificationLock);
    RtlDeleteCriticalSection(&handle->Lock);
    PhFree(handle);
    return STATUS_SUCCESS;
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
        NsiTcpObjectAllConnections,
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

    if (!NT_SUCCESS(status))
        return status;

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTcpObjectBoundEndpoint,
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

    if (!NT_SUCCESS(status))
    {
        PhNsiFreeTable(firstKeyTable, NULL, firstStatusTable, firstOwnerTable);
        return status;
    }

    status = RtlSIZETAdd(firstCount, secondCount, &totalRows);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = RtlSIZETMult(totalRows, sizeof(NSI_TCP_IPV4_ROW), &rowBytes);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = RtlSIZETAdd(FIELD_OFFSET(NSI_TCP_IPV4_TABLE, Entries), rowBytes, &ipv4TableSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = RtlSIZETMult(totalRows, sizeof(NSI_TCP_IPV6_ROW), &rowBytes);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = RtlSIZETAdd(FIELD_OFFSET(NSI_TCP_IPV6_TABLE, Entries), rowBytes, &ipv6TableSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!(ipv4TcpTable = PhAllocateZeroSafe(ipv4TableSize)))
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    if (!(ipv6TcpTable = PhAllocateZeroSafe(ipv6TableSize)))
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

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

                RtlZeroMemory(ipv4Row, sizeof(NSI_TCP_IPV4_ROW));
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

                RtlZeroMemory(ipv6Row, sizeof(NSI_TCP_IPV6_ROW));
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

                RtlZeroMemory(ipv4Row, sizeof(NSI_TCP_IPV4_ROW));
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

                RtlZeroMemory(ipv6Row, sizeof(NSI_TCP_IPV6_ROW));
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
    ipv4TcpTable = NULL;
    ipv6TcpTable = NULL;
    status = STATUS_SUCCESS;

CleanupExit:
    if (ipv6TcpTable)
        PhFree(ipv6TcpTable);
    if (ipv4TcpTable)
        PhFree(ipv4TcpTable);

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
    NSI_OBJECT_ID objectId;

    if (RwVersion || Offset)
        return STATUS_NOT_SUPPORTED;
    if (!RwSize)
        return STATUS_SUCCESS;
    if (!Rw)
        return STATUS_INVALID_PARAMETER;

    switch (EstatsType)
    {
    case TcpConnectionEstatsSynOpts:   objectId = NsiTcpObjectEstatsSynOpts;   break;
    case TcpConnectionEstatsData:      objectId = NsiTcpObjectEstatsData;      break;
    case TcpConnectionEstatsSndCong:   objectId = NsiTcpObjectEstatsSndCong;   break;
    case TcpConnectionEstatsPath:      objectId = NsiTcpObjectEstatsPath;      break;
    case TcpConnectionEstatsSendBuff:  objectId = NsiTcpObjectEstatsSendBuff;  break;
    case TcpConnectionEstatsRec:       objectId = NsiTcpObjectEstatsRec;       break;
    case TcpConnectionEstatsObsRec:    objectId = NsiTcpObjectEstatsObsRec;    break;
    case TcpConnectionEstatsBandwidth: objectId = NsiTcpObjectEstatsBandwidth; break;
    case TcpConnectionEstatsFineRtt:   objectId = NsiTcpObjectEstatsFineRtt;   break;
    default: return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&key, sizeof(NSI_ESTATS_TCP_KEY));
    key.AddressFamily = AF_INET6;
    key.LocalPort = (USHORT)Row->dwLocalPort;
    key.LocalAddressIPv6 = Row->LocalAddr;
    key.LocalScopeId.Value = Row->dwLocalScopeId;
    key.RemoteAddressFamily = AF_INET6;
    key.RemotePort = (USHORT)Row->dwRemotePort;
    key.RemoteAddressIPv6 = Row->RemoteAddr;
    key.RemoteScopeId.Value = Row->dwRemoteScopeId;

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
    NSI_OBJECT_ID objectId;

    if (RwVersion || Offset)
        return STATUS_NOT_SUPPORTED;
    if (!RwSize)
        return STATUS_SUCCESS;
    if (!Rw)
        return STATUS_INVALID_PARAMETER;

    switch (EstatsType)
    {
    case TcpConnectionEstatsSynOpts:   
        objectId = NsiTcpObjectEstatsSynOpts;   
        break;
    case TcpConnectionEstatsData:      
        objectId = NsiTcpObjectEstatsData;      
        break;
    case TcpConnectionEstatsSndCong:   
        objectId = NsiTcpObjectEstatsSndCong;   
        break;
    case TcpConnectionEstatsPath:      
        objectId = NsiTcpObjectEstatsPath;      
        break;
    case TcpConnectionEstatsSendBuff:  
        objectId = NsiTcpObjectEstatsSendBuff;  
        break;
    case TcpConnectionEstatsRec:       
        objectId = NsiTcpObjectEstatsRec;       
        break;
    case TcpConnectionEstatsObsRec:    
        objectId = NsiTcpObjectEstatsObsRec;    
        break;
    case TcpConnectionEstatsBandwidth: 
        objectId = NsiTcpObjectEstatsBandwidth; 
        break;
    case TcpConnectionEstatsFineRtt:   
        objectId = NsiTcpObjectEstatsFineRtt;   
        break;
    default: 
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&key, sizeof(NSI_ESTATS_TCP_KEY));
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
        && memcmp(Left->Data4, Right->Data4, RTL_NUMBER_OF(Left->Data4)) == 0;
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

    RtlZeroMemory(Row, sizeof(MIB_IF_ROW2));
    Row->InterfaceLuid = *InterfaceLuid;
    Row->InterfaceIndex = Ros->ifIndex;
    Row->InterfaceGuid = Ros->InterfaceGuid;
    length = min(Rw->ifAlias.Length, (sizeof(WCHAR) * RTL_NUMBER_OF(Row->Alias)) - sizeof(WCHAR));
    memcpy(Row->Alias, Rw->ifAlias.String, length);
    length = min(Ros->ifDescr.Length, (sizeof(WCHAR) * RTL_NUMBER_OF(Row->Description)) - sizeof(WCHAR));
    memcpy(Row->Description, Ros->ifDescr.String, length);
    Row->PhysicalAddressLength = min(Rw->ifPhysAddress.Length, RTL_NUMBER_OF(Row->PhysicalAddress));
    memcpy(Row->PhysicalAddress, Rw->ifPhysAddress.Address, Row->PhysicalAddressLength);
    length = min(Ros->PermanentPhysAddress.Length, RTL_NUMBER_OF(Row->PermanentPhysicalAddress));
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
    NTSTATUS status;
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

        status = PhConvertInterfaceIndexToLuid(Row->InterfaceIndex, &interfaceLuid);

        if (!NT_SUCCESS(status))
            return status;
    }

    status = PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_NDIS_MODULEID,
        NsiNdisObjectInterface,
        &interfaceLuid,
        sizeof(NET_LUID),
        &rw,
        sizeof(NDIS_NSI_INTERFACE_INFORMATION_RW),
        &rod,
        sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROD),
        &ros,
        sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROS));

    if (!NT_SUCCESS(status))
        return status;

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
    return PhNetioGetAddress(NsiActive, PhNetioAddressAnycast, Row);
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
    return PhNetioEnumerateAddress(NsiActive, Family, PhNetioAddressAnycast, Table);
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
    return PhNetioGetAddress(NsiActive, PhNetioAddressUnicast, Row);
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
    return PhNetioEnumerateAddress(NsiActive, Family, PhNetioAddressUnicast, Table);
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
    return PhNetioGetAddress(NsiActive, PhNetioAddressMulticast, Row);
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
    return PhNetioEnumerateAddress(NsiActive, Family, PhNetioAddressMulticast, Table);
}

/**
 * Checks that a caller buffer can hold a table with a count header and the specified rows.
 *
 * \param HeaderSize The offset of the row array in the table (FIELD_OFFSET(<table>, table)).
 * \param RowCount The number of rows.
 * \param RowSize The size, in bytes, of each row.
 * \param Table The caller buffer.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required or used size.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getiftable
 */
static NTSTATUS PhpNetioMibCheckTableBufferSize(
    _In_ ULONG HeaderSize,
    _In_ ULONG RowCount,
    _In_ ULONG RowSize,
    _In_opt_ PVOID Table,
    _Inout_ PULONG SizePointer
    )
{
    NTSTATUS status;
    ULONG requiredSize;
    ULONG rowBytes;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    status = RtlULongMult(RowCount, RowSize, &rowBytes);

    if (!NT_SUCCESS(status))
        return status;

    status = RtlULongAdd(HeaderSize, rowBytes, &requiredSize);

    if (!NT_SUCCESS(status))
        return status;

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
 * \param HeaderSize The offset of the row array in the table (FIELD_OFFSET(<table>, table)).
 * \param RowCount The number of rows.
 * \param RowSize The size, in bytes, of each row.
 * \param BufferSize The size, in bytes, of the buffer.
 * \return TRUE if the rows fit, otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-freemibtable
 */
static BOOLEAN PhpNetioMibTableHasCapacity(
    _In_ ULONG HeaderSize,
    _In_ ULONG RowCount,
    _In_ ULONG RowSize,
    _In_ ULONG BufferSize
    )
{
    ULONG requiredSize;

    if (!NT_SUCCESS(RtlULongMult(RowCount, RowSize, &requiredSize)))
        return FALSE;

    if (!NT_SUCCESS(RtlULongAdd(HeaderSize, requiredSize, &requiredSize)))
        return FALSE;

    return BufferSize >= requiredSize;
}

/**
 * Computes the size of a MIB table with a count header followed by rows.
 *
 * \param HeaderSize The offset of the row array in the table (FIELD_OFFSET(<table>, table)).
 * \param RowCount The number of rows.
 * \param RowSize The size, in bytes, of each row.
 * \param IncludeOneEntry TRUE if the header already includes one row (ANY_SIZE array).
 * \param Size Receives the table size, in bytes.
 * \return TRUE on success, FALSE on overflow.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-freemibtable
 */
static BOOLEAN PhpNetioMibSizeofTable(
    _In_ ULONG HeaderSize,
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

    if (!NT_SUCCESS(RtlULongAdd(RowCount, IncludeOneEntry ? 1 : 0, &rows)))
        return FALSE;

    if (!NT_SUCCESS(RtlULongMult(rows, RowSize, &tableSize)))
        return FALSE;

    if (!NT_SUCCESS(RtlULongAdd(HeaderSize, tableSize, &tableSize)))
        return FALSE;

    *Size = tableSize;
    return TRUE;
}

// N.B. Tables whose rows contain 8-byte fields put the row array at offset 8, not 4.
static_assert(FIELD_OFFSET(MIB_TCPTABLE_OWNER_MODULE, table) == 8, "MIB_TCPTABLE_OWNER_MODULE header size");
static_assert(FIELD_OFFSET(MIB_TCP6TABLE_OWNER_MODULE, table) == 8, "MIB_TCP6TABLE_OWNER_MODULE header size");
static_assert(FIELD_OFFSET(MIB_UDPTABLE_OWNER_MODULE, table) == 8, "MIB_UDPTABLE_OWNER_MODULE header size");
static_assert(FIELD_OFFSET(MIB_UDP6TABLE_OWNER_MODULE, table) == 8, "MIB_UDP6TABLE_OWNER_MODULE header size");

// N.B. qsort_s requires a comparator; passing NULL invokes the invalid parameter handler.
static int __cdecl PhpCompareMibTcp6Row2(
    _In_opt_ PVOID Context,
    _In_ const void* Row1,
    _In_ const void* Row2
    );

/**
 * Sort routine that orders MIB_TCP6ROW entries by local and remote endpoint.
 */
int __cdecl PhpCompareMibTcp6Row(
    _In_opt_ PVOID Context,
    _In_ const void* Row1,
    _In_ const void* Row2
    )
{
    // State comes first in MIB_TCP6ROW; compare LocalAddr through the end of the row.
    return memcmp(
        &((PMIB_TCP6ROW)Row1)->LocalAddr,
        &((PMIB_TCP6ROW)Row2)->LocalAddr,
        sizeof(MIB_TCP6ROW) - FIELD_OFFSET(MIB_TCP6ROW, LocalAddr)
        );
}

/**
 * Sort routine that orders MIB_UDP6ROW entries by local endpoint.
 */
int __cdecl PhpCompareMibUdp6Row(
    _In_opt_ PVOID Context,
    _In_ const void* Row1,
    _In_ const void* Row2
    )
{
    return memcmp(Row1, Row2, sizeof(MIB_UDP6ROW));
}

/**
 * Sort routine that orders MIB_IFROW entries by interface index.
 */
int __cdecl PhpCompareMibIfRow(
    _In_opt_ PVOID Context,
    _In_ const void* Row1,
    _In_ const void* Row2
    )
{
    ULONG index1 = ((PMIB_IFROW)Row1)->dwIndex;
    ULONG index2 = ((PMIB_IFROW)Row2)->dwIndex;

    return (index1 > index2) - (index1 < index2);
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

    RtlZeroMemory(IfRow, sizeof(MIB_IFROW));
    IfRow->dwIndex = ros->ifIndex;
    IfRow->dwType = ros->ifType;
    IfRow->dwMtu = rod->ifMtu;
    IfRow->dwSpeed = (ULONG)min(rod->XmitLinkSpeed, MAXULONG);
    IfRow->dwPhysAddrLen = min(rw->ifPhysAddress.Length, RTL_NUMBER_OF(IfRow->bPhysAddr));
    memcpy(IfRow->bPhysAddr, rw->ifPhysAddress.Address, IfRow->dwPhysAddrLen);
    IfRow->dwAdminStatus = rw->ifAdminStatus;
    IfRow->dwOperStatus = (INTERNAL_IF_OPER_STATUS)rod->ifOperStatus;
    IfRow->dwLastChange = (ULONG)rod->ifLastChange;
    IfRow->dwInOctets = (ULONG)rod->ifHCInOctets;
    IfRow->dwInUcastPkts = (ULONG)rod->ifHCInUcastPkts;
    IfRow->dwInNUcastPkts = (ULONG)(rod->ifHCInMulticastPkts + rod->ifHCInBroadcastPkts);
    IfRow->dwInDiscards = (ULONG)rod->ifInDiscards;
    IfRow->dwInErrors = (ULONG)rod->ifInErrors;
    IfRow->dwInUnknownProtos = (ULONG)rod->ifInUnknownProtos;
    IfRow->dwOutOctets = (ULONG)rod->ifHCOutOctets;
    IfRow->dwOutUcastPkts = (ULONG)rod->ifHCOutUcastPkts;
    IfRow->dwOutNUcastPkts = (ULONG)(rod->ifHCOutMulticastPkts + rod->ifHCOutBroadcastPkts);
    IfRow->dwOutDiscards = (ULONG)rod->ifOutDiscards;
    IfRow->dwOutErrors = (ULONG)rod->ifOutErrors;

    PPH_BYTES description = PhConvertUtf16ToMultiByteEx(
        ros->ifDescr.String,
        ros->ifDescr.Length
        );

    if (description)
    {
        descriptionLength = min(description->Length, RTL_NUMBER_OF(IfRow->bDescr));
        memcpy(IfRow->bDescr, description->Buffer, descriptionLength);
        IfRow->dwDescrLen = (ULONG)descriptionLength;
        PhDereferenceObject(description);
    }
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
    _In_ PH_TCP_OWNER_CLASS OwnerClass
    )
{
    NTSTATUS status;
    ULONG rowSize;
    ULONG headerSize;
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
    case PhTcpOwnerBasic:
        rowSize = sizeof(MIB_TCPROW);
        headerSize = FIELD_OFFSET(MIB_TCPTABLE, table);
        break;
    case PhTcpOwnerBasic2:
        rowSize = sizeof(MIB_TCPROW2);
        headerSize = FIELD_OFFSET(MIB_TCPTABLE2, table);
        break;
    case PhTcpOwnerModule:
        rowSize = sizeof(MIB_TCPROW_OWNER_MODULE);
        headerSize = FIELD_OFFSET(MIB_TCPTABLE_OWNER_MODULE, table);
        break;
    case PhTcpOwnerPid:
        rowSize = sizeof(MIB_TCPROW_OWNER_PID);
        headerSize = FIELD_OFFSET(MIB_TCPTABLE_OWNER_PID, table);
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTcpObjectAllConnections,
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

    if (!NT_SUCCESS(status))
        return status;

    if (!PhpNetioMibSizeofTable(headerSize, count, rowSize, FALSE, &requiredSize))
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
            headerSize,
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
        case PhTcpOwnerBasic:
            {
                PMIB_TCPTABLE table;
                PMIB_TCPROW row;

                table = (PMIB_TCPTABLE)TcpTable;
                row = &table->table[table->dwNumEntries];
                RtlZeroMemory(row, sizeof(MIB_TCPROW));
                row->dwState = statusTable[i].State;
                row->dwLocalAddr = key->LocalAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwLocalPort = key->LocalAddress.Ipv4.sin_port;
                row->dwRemoteAddr = key->RemoteAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwRemotePort = key->RemoteAddress.Ipv4.sin_port;
            }
            break;
        case PhTcpOwnerBasic2:
            {
                PMIB_TCPTABLE2 table;
                PMIB_TCPROW2 row;

                table = (PMIB_TCPTABLE2)TcpTable;
                row = &table->table[table->dwNumEntries];
                RtlZeroMemory(row, sizeof(MIB_TCPROW2));
                row->dwState = statusTable[i].State;
                row->dwLocalAddr = key->LocalAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwLocalPort = key->LocalAddress.Ipv4.sin_port;
                row->dwRemoteAddr = key->RemoteAddress.Ipv4.sin_addr.S_un.S_addr;
                row->dwRemotePort = key->RemoteAddress.Ipv4.sin_port;
                row->dwOwningPid = ownerTable[i].OwningPid;
                row->dwOffloadState = statusTable[i].OffloadState;
            }
            break;
        case PhTcpOwnerModule:
            {
                PMIB_TCPTABLE_OWNER_MODULE table;
                PMIB_TCPROW_OWNER_MODULE row;

                table = (PMIB_TCPTABLE_OWNER_MODULE)TcpTable;
                row = &table->table[table->dwNumEntries];
                RtlZeroMemory(row, sizeof(MIB_TCPROW_OWNER_MODULE));
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
        case PhTcpOwnerPid:
            {
                PMIB_TCPTABLE_OWNER_PID table;
                PMIB_TCPROW_OWNER_PID row;

                table = (PMIB_TCPTABLE_OWNER_PID)TcpTable;
                row = &table->table[table->dwNumEntries];
                RtlZeroMemory(row, sizeof(MIB_TCPROW_OWNER_PID));
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
        headerSize,
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
        RtlZeroMemory(&Address->Ipv4, sizeof(SOCKADDR_IN));
        Address->Ipv4.sin_family = AF_INET;
        Address->Ipv4.sin_addr = *(const IN_ADDR*)KeyAddress;
        return;
    }

    RtlZeroMemory(&Address->Ipv6, sizeof(SOCKADDR_IN6));
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
    _In_ PH_NETIO_ADDRESS_TYPE AddressType,
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
    case PhNetioAddressUnicast:
        {
            PMIB_UNICASTIPADDRESS_ROW row = Row;
            const MIB_IP_UNICAST_RW_ROW* rw = Rw;
            const MIB_IP_UNICAST_DYNAMIC_ROW* rod = Rod;
            const MIB_IP_UNICAST_STATIC_ROW* ros = Ros;

            RtlZeroMemory(row, sizeof(MIB_UNICASTIPADDRESS_ROW));
            row->InterfaceLuid = interfaceLuid;
            if (NT_SUCCESS(PhConvertInterfaceLuidToIndex(&interfaceLuid, &interfaceIndex)))
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
                row->CreationTimeStamp = ros->CreationTimestamp;
        }
        break;
    case PhNetioAddressMulticast:
        {
            PMIB_MULTICASTIPADDRESS_ROW row = Row;
            ULONG scopeId = Rod ? *(const ULONG*)Rod : 0;

            RtlZeroMemory(row, sizeof(MIB_MULTICASTIPADDRESS_ROW));
            row->InterfaceLuid = interfaceLuid;

            if (Ros)
                row->InterfaceIndex = *(const NET_IFINDEX*)Ros;
            else if (NT_SUCCESS(PhConvertInterfaceLuidToIndex(&interfaceLuid, &interfaceIndex)))
                row->InterfaceIndex = interfaceIndex;

            PhpNetioSetSocketAddress(Family, &row->Address, keyAddress, scopeId);

            if (Rod)
                row->ScopeId.Value = scopeId;
        }
        break;
    case PhNetioAddressAnycast:
        {
            PMIB_ANYCASTIPADDRESS_ROW row = Row;

            RtlZeroMemory(row, sizeof(MIB_ANYCASTIPADDRESS_ROW));
            row->InterfaceLuid = interfaceLuid;
            if (NT_SUCCESS(PhConvertInterfaceLuidToIndex(&interfaceLuid, &interfaceIndex)))
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
NTSTATUS PhNetioGetAddress(
    _In_ NSI_STORE Store,
    _In_ PH_NETIO_ADDRESS_TYPE AddressType,
    _Inout_ PVOID Row
    )
{
    NSI_IP_ADDRESS_KEY key;
    MIB_IP_UNICAST_RW_ROW rw;
    ULONG rod[3]; // largest dynamic row (multicast, 0xC bytes)
    MIB_IP_UNICAST_STATIC_ROW ros;
    PSOCKADDR_INET address;
    PNET_LUID interfaceLuid;
    NET_IFINDEX interfaceIndex;
    ULONG keySize;
    NTSTATUS status;

    if (!Row || (ULONG)AddressType >= RTL_NUMBER_OF(PhpNetioAddressTypeInfo))
        return STATUS_INVALID_PARAMETER;

    static_assert((sizeof(ULONG) * RTL_NUMBER_OF(rod)) >= sizeof(MIB_IP_UNICAST_DYNAMIC_ROW));
    static_assert(sizeof(MIB_IP_UNICAST_STATIC_ROW) >= sizeof(NET_IFINDEX));

    switch (AddressType)
    {
    case PhNetioAddressUnicast:
        address = &((PMIB_UNICASTIPADDRESS_ROW)Row)->Address;
        interfaceLuid = &((PMIB_UNICASTIPADDRESS_ROW)Row)->InterfaceLuid;
        interfaceIndex = ((PMIB_UNICASTIPADDRESS_ROW)Row)->InterfaceIndex;
        break;
    case PhNetioAddressMulticast:
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

    RtlZeroMemory(&key, sizeof(NSI_IP_ADDRESS_KEY));

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
        status = PhConvertInterfaceIndexToLuid(interfaceIndex, &key.V6.InterfaceLuid);
        if (!NT_SUCCESS(status))
            return status;
    }

    RtlZeroMemory(&rw, sizeof(MIB_IP_UNICAST_RW_ROW));
    RtlZeroMemory(rod, (sizeof(ULONG) * RTL_NUMBER_OF(rod)));
    RtlZeroMemory(&ros, sizeof(MIB_IP_UNICAST_STATIC_ROW));

    status = PhNsiGetAllParameters(
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

    if (!NT_SUCCESS(status))
        return status;

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
NTSTATUS PhNetioEnumerateAddress(
    _In_ NSI_STORE Store,
    _In_ ADDRESS_FAMILY Family,
    _In_ PH_NETIO_ADDRESS_TYPE AddressType,
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
    NTSTATUS status;

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

        status = PhNsiAllocateAndGetTable(
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
        if (status == STATUS_NOT_FOUND || status == STATUS_NOT_SUPPORTED)
        {
            counts[i] = 0;
            continue;
        }

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = RtlULongAdd(counts[0], counts[1], &totalCount);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = RtlSIZETMult(totalCount, rowSize, &tableSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = RtlSIZETAdd(FIELD_OFFSET(MIB_UNICASTIPADDRESS_TABLE, Table), tableSize, &tableSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!(table = PhAllocateZeroSafe(tableSize)))
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }
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
    status = STATUS_SUCCESS;

CleanupExit:
    for (ULONG i = 0; i < RTL_NUMBER_OF(families); i++)
        PhNsiFreeTable(keys[i], rw[i], rod[i], ros[i]);

    return status;
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
    _In_ PH_TCP_OWNER_CLASS OwnerClass
    )
{
    NTSTATUS status;
    ULONG rowSize;
    ULONG headerSize;
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
    case PhTcpOwnerBasic:
        rowSize = sizeof(MIB_TCP6ROW);
        headerSize = FIELD_OFFSET(MIB_TCP6TABLE, table);
        break;
    case PhTcpOwnerBasic2:
        rowSize = sizeof(MIB_TCP6ROW2);
        headerSize = FIELD_OFFSET(MIB_TCP6TABLE2, table);
        break;
    case PhTcpOwnerModule:
        rowSize = sizeof(MIB_TCP6ROW_OWNER_MODULE);
        headerSize = FIELD_OFFSET(MIB_TCP6TABLE_OWNER_MODULE, table);
        break;
    case PhTcpOwnerPid:
        rowSize = sizeof(MIB_TCP6ROW_OWNER_PID);
        headerSize = FIELD_OFFSET(MIB_TCP6TABLE_OWNER_PID, table);
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTcpObjectBoundEndpoint,
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

    if (!NT_SUCCESS(status))
        return status;

    if (!PhpNetioMibSizeofTable(headerSize, count, rowSize, FALSE, &requiredSize))
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
            headerSize,
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
        case PhTcpOwnerBasic:
            {
                PMIB_TCP6TABLE table = (PMIB_TCP6TABLE)TcpTable;
                PMIB_TCP6ROW row = &table->table[table->dwNumEntries];

                RtlZeroMemory(row, sizeof(MIB_TCP6ROW));
                row->State = statusTable[i].State;
                row->LocalAddr = key->LocalAddress.Ipv6.sin6_addr;
                row->dwLocalScopeId = key->LocalAddress.Ipv6.sin6_scope_id;
                row->dwLocalPort = key->LocalAddress.Ipv6.sin6_port;
                row->RemoteAddr = key->RemoteAddress.Ipv6.sin6_addr;
                row->dwRemoteScopeId = key->RemoteAddress.Ipv6.sin6_scope_id;
                row->dwRemotePort = key->RemoteAddress.Ipv6.sin6_port;
            }
            break;
        case PhTcpOwnerBasic2:
            {
                PMIB_TCP6TABLE2 table = (PMIB_TCP6TABLE2)TcpTable;
                PMIB_TCP6ROW2 row = &table->table[table->dwNumEntries];

                RtlZeroMemory(row, sizeof(MIB_TCP6ROW2));
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
        case PhTcpOwnerModule:
            {
                PMIB_TCP6TABLE_OWNER_MODULE table = (PMIB_TCP6TABLE_OWNER_MODULE)TcpTable;
                PMIB_TCP6ROW_OWNER_MODULE row = &table->table[table->dwNumEntries];

                RtlZeroMemory(row, sizeof(MIB_TCP6ROW_OWNER_MODULE));
                row->dwState = statusTable[i].State;
                memcpy(row->ucLocalAddr, key->LocalAddress.Ipv6.sin6_addr.s6_addr, RTL_NUMBER_OF(row->ucLocalAddr));
                row->dwLocalScopeId = key->LocalAddress.Ipv6.sin6_scope_id;
                row->dwLocalPort = key->LocalAddress.Ipv6.sin6_port;
                memcpy(row->ucRemoteAddr, key->RemoteAddress.Ipv6.sin6_addr.s6_addr, RTL_NUMBER_OF(row->ucRemoteAddr));
                row->dwRemoteScopeId = key->RemoteAddress.Ipv6.sin6_scope_id;
                row->dwRemotePort = key->RemoteAddress.Ipv6.sin6_port;
                row->dwOwningPid = ownerTable[i].OwningPid;
                row->liCreateTimestamp = ownerTable[i].CreateTimestamp;
                row->OwningModuleInfo[0] = ownerTable[i].OwningModuleInfo;
            }
            break;
        case PhTcpOwnerPid:
            {
                PMIB_TCP6TABLE_OWNER_PID table = (PMIB_TCP6TABLE_OWNER_PID)TcpTable;
                PMIB_TCP6ROW_OWNER_PID row = &table->table[table->dwNumEntries];

                RtlZeroMemory(row, sizeof(MIB_TCP6ROW_OWNER_PID));
                row->dwState = statusTable[i].State;
                memcpy(row->ucLocalAddr, key->LocalAddress.Ipv6.sin6_addr.s6_addr, RTL_NUMBER_OF(row->ucLocalAddr));
                row->dwLocalScopeId = key->LocalAddress.Ipv6.sin6_scope_id;
                row->dwLocalPort = key->LocalAddress.Ipv6.sin6_port;
                memcpy(row->ucRemoteAddr, key->RemoteAddress.Ipv6.sin6_addr.s6_addr, RTL_NUMBER_OF(row->ucRemoteAddr));
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
        headerSize,
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
    NTSTATUS status;
    ULONG rowSize;
    ULONG headerSize;
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
        headerSize = FIELD_OFFSET(MIB_UDP6TABLE, table);
        break;
    case UDP_TABLE_OWNER_PID:
        rowSize = sizeof(MIB_UDP6ROW_OWNER_PID);
        headerSize = FIELD_OFFSET(MIB_UDP6TABLE_OWNER_PID, table);
        break;
    case UDP_TABLE_OWNER_MODULE:
        rowSize = sizeof(MIB_UDP6ROW_OWNER_MODULE);
        headerSize = FIELD_OFFSET(MIB_UDP6TABLE_OWNER_MODULE, table);
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_UDP_MODULEID,
        NsiUdpObjectEndpoint,
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

    if (!NT_SUCCESS(status))
        return status;

    if (!PhpNetioMibSizeofTable(headerSize, count, rowSize, FALSE, &requiredSize))
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
            headerSize,
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

                RtlZeroMemory(row, sizeof(MIB_UDP6ROW));
                row->dwLocalAddr = key->LocalAddressV6;
                row->dwLocalScopeId = key->LocalScopeId.Value;
                row->dwLocalPort = key->LocalPort;
            }
            break;
        case UDP_TABLE_OWNER_PID:
            {
                PMIB_UDP6TABLE_OWNER_PID table = (PMIB_UDP6TABLE_OWNER_PID)UdpTable;
                PMIB_UDP6ROW_OWNER_PID row = &table->table[table->dwNumEntries];

                RtlZeroMemory(row, sizeof(MIB_UDP6ROW_OWNER_PID));
                memcpy(row->ucLocalAddr, key->LocalAddressV6.s6_addr, RTL_NUMBER_OF(row->ucLocalAddr));
                row->dwLocalScopeId = key->LocalScopeId.Value;
                row->dwLocalPort = key->LocalPort;
                row->dwOwningPid = ownerTable[i].OwningPid;
            }
            break;
        case UDP_TABLE_OWNER_MODULE:
            {
                PMIB_UDP6TABLE_OWNER_MODULE table = (PMIB_UDP6TABLE_OWNER_MODULE)UdpTable;
                PMIB_UDP6ROW_OWNER_MODULE row = &table->table[table->dwNumEntries];

                RtlZeroMemory(row, sizeof(MIB_UDP6ROW_OWNER_MODULE));
                memcpy(row->ucLocalAddr, key->LocalAddressV6.s6_addr, RTL_NUMBER_OF(row->ucLocalAddr));
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
        headerSize,
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
    NTSTATUS status;
    ULONG rowSize;
    ULONG headerSize;
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
        headerSize = FIELD_OFFSET(MIB_UDPTABLE, table);
        break;
    case UDP_TABLE_OWNER_PID:
        rowSize = sizeof(MIB_UDPROW_OWNER_PID);
        headerSize = FIELD_OFFSET(MIB_UDPTABLE_OWNER_PID, table);
        break;
    case UDP_TABLE_OWNER_MODULE:
        rowSize = sizeof(MIB_UDPROW_OWNER_MODULE);
        headerSize = FIELD_OFFSET(MIB_UDPTABLE_OWNER_MODULE, table);
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_UDP_MODULEID,
        NsiUdpObjectEndpoint,
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

    if (!NT_SUCCESS(status))
        return status;

    if (!PhpNetioMibSizeofTable(headerSize, count, rowSize, FALSE, &requiredSize))
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
            headerSize,
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

                RtlZeroMemory(row, sizeof(MIB_UDPROW));
                row->dwLocalAddr = PhpNetioIpv4AddressFromInAddress(key->LocalAddressV4);
                row->dwLocalPort = key->LocalPort;
            }
            break;
        case UDP_TABLE_OWNER_PID:
            {
                PMIB_UDPTABLE_OWNER_PID table = (PMIB_UDPTABLE_OWNER_PID)UdpTable;
                PMIB_UDPROW_OWNER_PID row = &table->table[table->dwNumEntries];

                RtlZeroMemory(row, sizeof(MIB_UDPROW_OWNER_PID));
                row->dwLocalAddr = PhpNetioIpv4AddressFromInAddress(key->LocalAddressV4);
                row->dwLocalPort = key->LocalPort;
                row->dwOwningPid = ownerTable[i].OwningPid;
            }
            break;
        case UDP_TABLE_OWNER_MODULE:
            {
                PMIB_UDPTABLE_OWNER_MODULE table = (PMIB_UDPTABLE_OWNER_MODULE)UdpTable;
                PMIB_UDPROW_OWNER_MODULE row = &table->table[table->dwNumEntries];

                RtlZeroMemory(row, sizeof(MIB_UDPROW_OWNER_MODULE));
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
        headerSize,
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
    NTSTATUS status;

    status = PhGetBestInterfaceEx(DestinationAddress, BestIfIndex);

    if (NT_SUCCESS(status) && BestIfIndexAlias)
        *BestIfIndexAlias = *BestIfIndex;

    return status;
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
    PIP_ADDR_STRING newEntry = PhAllocateZeroSafe(sizeof(IP_ADDR_STRING));
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

    if (!DnsServers->AddrCount)
        return;

    PhpSetAddressString(DnsServerList, (PIN_ADDR)&DnsServers->AddrArray[0], (PIN_ADDR)&broadcastAddress);

    for (ULONG i = 1; i < DnsServers->AddrCount; i++)
    {
        PhpAddAddressStringToAdapter((PIN_ADDR)&DnsServers->AddrArray[i], (PIN_ADDR)&broadcastAddress, &tail);
    }
}

/**
 * Allocates and fills a FIXED_INFO structure with the host name, domain name and DNS server list.
 *
 * \param FixedInfo Receives the FIXED_INFO structure. Free the DNS server list with PhFreeAddressStringList, then the structure with PhFree.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhpAllocateAndGetFixedInfo(
    _Outptr_result_maybenull_ PVOID *FixedInfo
    )
{
    PFIXED_INFO fixedInfo;
    ULONG pBufLen;
    HLOCAL hMem;

    if (!FixedInfo)
        return STATUS_INVALID_PARAMETER;

    *FixedInfo = NULL;

    fixedInfo = PhAllocateZeroSafe(sizeof(FIXED_INFO));
    if (!fixedInfo)
        return STATUS_NO_MEMORY;

    pBufLen = sizeof(IP4_ARRAY);
    hMem = NULL;

    if (DnsQueryConfig(DnsConfigDnsServerList, DNS_CONFIG_FLAG_ALLOC, NULL, NULL, &hMem, &pBufLen) == ERROR_SUCCESS)
    {
        PhpAddDnsServerAddressInfo(&fixedInfo->DnsServerList, (PIP4_ARRAY)hMem);
        LocalFree(hMem);
    }

    pBufLen = RTL_NUMBER_OF(fixedInfo->HostName);
    DnsQueryConfig(DnsConfigHostName_A, 0, NULL, NULL, fixedInfo->HostName, &pBufLen);

    pBufLen = RTL_NUMBER_OF(fixedInfo->DomainName);
    DnsQueryConfig(DnsConfigPrimaryDomainName_A, 0, NULL, NULL, fixedInfo->DomainName, &pBufLen);

    // NetBT Parameters
    HANDLE keyHandle;
    PH_STRINGREF keyName;

    PhInitializeStringRef(&keyName, L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters");

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_QUERY_VALUE,
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        )))
    {
        ULONG nodeType = PhQueryRegistryUlongZ(keyHandle, L"NodeType");
        if (nodeType == ULONG_MAX)
            nodeType = PhQueryRegistryUlongZ(keyHandle, L"DhcpNodeType");
        if (nodeType == ULONG_MAX)
            nodeType = 8;

        fixedInfo->NodeType = nodeType;

        PPH_STRING scopeIdStr = PhQueryRegistryStringZ(keyHandle, L"ScopeId");
        if (!scopeIdStr)
            scopeIdStr = PhQueryRegistryStringZ(keyHandle, L"DhcpScopeId");

        if (scopeIdStr)
        {
            PhConvertUtf16ToUtf8Buffer(
                fixedInfo->ScopeId,
                RTL_NUMBER_OF(fixedInfo->ScopeId),
                NULL,
                scopeIdStr->Buffer,
                scopeIdStr->Length
            );
            PhDereferenceObject(scopeIdStr);
        }
        else
        {
            fixedInfo->ScopeId[0] = UNICODE_NULL;
        }

        ULONG enableProxy = PhQueryRegistryUlongZ(keyHandle, L"EnableProxy");
        if (enableProxy != ULONG_MAX)
            fixedInfo->EnableProxy = enableProxy;
        else
            fixedInfo->EnableProxy = 0;

        ULONG enableDns = PhQueryRegistryUlongZ(keyHandle, L"EnableDns");
        if (enableDns != ULONG_MAX)
            fixedInfo->EnableDns = enableDns;
        else
            fixedInfo->EnableDns = 0;

        NtClose(keyHandle);
    }
    else
    {
        fixedInfo->NodeType = 8;
        fixedInfo->ScopeId[0] = UNICODE_NULL;
        fixedInfo->EnableProxy = 0;
        fixedInfo->EnableDns = 0;
    }

    MIB_IPSTATS stats;
    if (NT_SUCCESS(PhGetIpStatisticsEx(&stats, AF_INET)))
    {
        fixedInfo->EnableRouting = (stats.dwForwarding == MIB_IP_FORWARDING);
    }
    else
    {
        fixedInfo->EnableRouting = 0;
    }

    *FixedInfo = fixedInfo;
    return STATUS_SUCCESS;
}

/**
 * Computes the size of the additional entries of an IP_ADDR_STRING list.
 *
 * \param AddressString The first list entry.
 * \return The size, in bytes, excluding the first entry.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_addr_string
 */
static ULONG PhGetSizeofAddressStringList(
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

    return PhNetioDosErrorToNtStatus(getCapabilities(InterfaceLuid, TimestampCapabilities));
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
VOID PhNsiFillRouteEntry(
    _In_ ADDRESS_FAMILY Family,
    _Out_ PMIB_IPFORWARD_ROW2 Route,
    _In_ const VOID *Key,
    _In_ const MIB_IP_ROUTE_RW_ROW *Rw,
    _In_ const VOID *Rod,
    _In_ const MIB_IP_ROUTE_STATIC_ROW *Ros
    )
{
    RtlZeroMemory(Route, sizeof(MIB_IPFORWARD_ROW2));
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
    _Out_ PNSI_UNICAST_TABLE Table
    )
{
    RtlZeroMemory(Table, sizeof(NSI_UNICAST_TABLE));

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
    _Inout_ PNSI_UNICAST_TABLE Table
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
    NSI_UNICAST_TABLE ipv4;
    NSI_UNICAST_TABLE ipv6;
    ULONG interfaceCount = 0;
    NTSTATUS status;
    SIZE_T requiredSize;
    PUCHAR cursor;
    PIP_ADAPTER_ADDRESSES previous = NULL;
    NET_IF_COMPARTMENT_ID compartmentId;

    *Addresses = NULL;
    if (AddressSize)
        *AddressSize = 0;

    RtlZeroMemory(&ipv4, sizeof(NSI_UNICAST_TABLE));
    RtlZeroMemory(&ipv6, sizeof(NSI_UNICAST_TABLE));

    status = PhNsiAllocateAndGetTable(
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

    if (!NT_SUCCESS(status))
        return status;

    if (!(Flags & GAA_FLAG_SKIP_UNICAST))
    {
        if (Family == AF_UNSPEC || Family == AF_INET)
        {
            status = PhpNsiGetUnicastTable(&NPI_MS_IPV4_MODULEID, sizeof(MIB_IP_UNICAST_KEY_ROW_V4), &ipv4);
            if (!NT_SUCCESS(status))
                goto Cleanup;
        }

        if (Family == AF_UNSPEC || Family == AF_INET6)
        {
            status = PhpNsiGetUnicastTable(&NPI_MS_IPV6_MODULEID, sizeof(MIB_IP_UNICAST_KEY_ROW_V6), &ipv6);
            if (!NT_SUCCESS(status))
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
        status = STATUS_INTEGER_OVERFLOW;
        goto Cleanup;
    }

    if (!(*Addresses = PhAllocateZeroSafe(requiredSize ? requiredSize : 1)))
    {
        status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

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
        cursor += ALIGN_UP_BY(sizeof(IP_ADAPTER_ADDRESSES), sizeof(ULONG64));
        adapter->Length = sizeof(IP_ADAPTER_ADDRESSES);
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
        adapter->PhysicalAddressLength = min(dynamicRows[i].ifPhysAddress.Length, RTL_NUMBER_OF(adapter->PhysicalAddress));
        memcpy(adapter->PhysicalAddress, dynamicRows[i].ifPhysAddress.Address, adapter->PhysicalAddressLength);

        adapter->AdapterName = (PCHAR)cursor;
        if (NT_SUCCESS(PhFormatGuidToBuffer(&staticRows[i].InterfaceGuid, guidString, (sizeof(WCHAR) * RTL_NUMBER_OF(guidString)), NULL)))
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
            PNSI_UNICAST_TABLE table = familyIndex ? &ipv6 : &ipv4;

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
                cursor += ALIGN_UP_BY(sizeof(IP_ADAPTER_UNICAST_ADDRESS), sizeof(ULONG64));
                socketAddress = (PSOCKADDR_INET)cursor;
                cursor += ALIGN_UP_BY(sizeof(SOCKADDR_INET), sizeof(ULONG64));

                unicast->Length = sizeof(IP_ADAPTER_UNICAST_ADDRESS);
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
        status = STATUS_NO_MORE_ENTRIES;
    }
    else
    {
        if (AddressSize)
            *AddressSize = (ULONG)(cursor - (PUCHAR)*Addresses);
        status = STATUS_SUCCESS;
    }

Cleanup:
    PhpNsiFreeUnicastTable(&ipv4);
    PhpNsiFreeUnicastTable(&ipv6);
    PhNsiFreeTable(interfaceKeys, NULL, dynamicRows, staticRows);
    return status;
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
    NTSTATUS status;

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
    status = PhNetioDosErrorToNtStatus(getAdaptersAddresses(Family, Flags, NULL, NULL, &size));

    if (status != STATUS_BUFFER_TOO_SMALL)
        return status;

    if (!size)
        return STATUS_DATA_ERROR;

    if (!(*Addresses = PhAllocateZeroSafe(size)))
        return STATUS_NO_MEMORY;

    status = PhNetioDosErrorToNtStatus(getAdaptersAddresses(Family, Flags, NULL, *Addresses, &size));

    if (!NT_SUCCESS(status))
    {
        PhFree(*Addresses);
        *Addresses = NULL;
    }
    else if (AddressSize)
    {
        *AddressSize = size;
    }

    return status;
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
    NTSTATUS status;
    IF_INDEX dwIndex;
    NET_LUID interfaceLuid;
    NDIS_NSI_INTERFACE_INFORMATION_ROD dynamicRow;
    NDIS_NSI_INTERFACE_INFORMATION_ROS staticRow;
    NDIS_NSI_INTERFACE_INFORMATION_RW readWriteRow;

    interfaceLuid.Value = 0;
    RtlZeroMemory(&readWriteRow, sizeof(NDIS_NSI_INTERFACE_INFORMATION_RW));
    RtlZeroMemory(&dynamicRow, sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROD));
    RtlZeroMemory(&staticRow, sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROS));

    if (!IfRow)
        return STATUS_INVALID_PARAMETER;

    dwIndex = IfRow->dwIndex;
    status = PhpNsiGetParameter(
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

    if (!NT_SUCCESS(status))
        return status;

    status = PhNsiGetAllParameters(
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

    if (!NT_SUCCESS(status))
        return status;

    PhpFillIfEntry(IfRow, &readWriteRow, &dynamicRow, &staticRow);
    return STATUS_SUCCESS;
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
    SIZE_T tableSize;
    NTSTATUS status;
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

    status = PhNsiAllocateAndGetTable(NsiActive, &NPI_MS_NDIS_MODULEID, NsiNdisObjectInterface,
        &keys, sizeof(NET_LUID), &rw, sizeof(NDIS_NSI_INTERFACE_INFORMATION_RW),
        &rod, sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROD),
        &ros, sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROS), &count, 0);
    if (!NT_SUCCESS(status))
        return status;

    status = RtlSIZETMult(count, sizeof(MIB_IF_ROW2), &tableSize);

    if (!NT_SUCCESS(status))
    {
        PhNsiFreeTable(keys, rw, rod, ros);
        return status;
    }

    status = RtlSIZETAdd(FIELD_OFFSET(MIB_IF_TABLE2, Table), tableSize, &tableSize);

    if (!NT_SUCCESS(status))
    {
        PhNsiFreeTable(keys, rw, rod, ros);
        return status;
    }

    compartmentId = PhGetCurrentThreadCompartmentId();
    if (!(table = PhAllocateZeroSafe(tableSize)))
    {
        PhNsiFreeTable(keys, rw, rod, ros);
        return STATUS_NO_MEMORY;
    }
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
    NTSTATUS status;
    SIZE_T networkNameLength;
    NDIS_NSI_NETWORK_RW networkInformation;
    WCHAR pszSrc[MAXIMUM_FILENAME_LENGTH + 1];

    RtlZeroMemory(&networkInformation, sizeof(NDIS_NSI_NETWORK_RW));

    status = PhpNsiGetParameter(
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

    if (NT_SUCCESS(status))
    {
        networkNameLength = networkInformation.NetworkName.Length >> 1;

        if (networkNameLength > MAXIMUM_FILENAME_LENGTH)
            networkNameLength = MAXIMUM_FILENAME_LENGTH;
        
        pszSrc[networkNameLength] = UNICODE_NULL;
        memcpy(pszSrc, networkInformation.NetworkName.String, networkNameLength * sizeof(WCHAR));
        status = PhCopyStringZ(pszSrc, SIZE_MAX, NetworkName, Length, NULL);

        if (NT_SUCCESS(status))
        {
            *CompartmentId = networkInformation.CompartmentId;
            *SiteId = networkInformation.SiteId;
            return STATUS_SUCCESS;
        }

        return status;
    }

    return status;
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
    NTSTATUS status;

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

    status = PhNsiAllocateAndGetTable(
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

    if (NT_SUCCESS(status))
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

    status = PhpNsiGetParameter(
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

    if (!NT_SUCCESS(status))
        *IfIndex = 0;

    return status;
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
    NTSTATUS status;
    MIB_TCPSTATS_LH statistics;
    PTCP_KEY_ROW keyTable = NULL;
    PNSI_TCP_STATUS_ROW dynamicTable = NULL;
    ULONG count = 0;
    ULONG errorCount = 0;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(&statistics, sizeof(MIB_TCPSTATS_LH));
    status = PhGetTcpStatisticsEx(&statistics, AF_INET6);

    if (!NT_SUCCESS(status))
        return status;

    status = PhpNetioMibCheckTableBufferSize(
        FIELD_OFFSET(MIB_TCP6TABLE, table),
        statistics.dwNumConns,
        sizeof(MIB_TCP6ROW),
        TcpTable,
        SizePointer
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTcpObjectAllConnections,
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

    if (NT_SUCCESS(status))
    {
        TcpTable->dwNumEntries = 0;

        for (ULONG i = 0; i < count; i++)
        {
            const PTCP_KEY_ROW key = &keyTable[i];
            
            if (key->Family == AF_INET6 &&
                !IN6_IS_ADDR_V4MAPPED(&key->LocalAddress))
            {
                if (PhpNetioMibTableHasCapacity(FIELD_OFFSET(MIB_TCP6TABLE, table), TcpTable->dwNumEntries + 1, sizeof(MIB_TCP6ROW), *SizePointer))
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
            FIELD_OFFSET(MIB_TCP6TABLE, table),
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
                    qsort_s(TcpTable->table, TcpTable->dwNumEntries, sizeof(MIB_TCP6ROW), PhpCompareMibTcp6Row, NULL);
                }
                return STATUS_SUCCESS;
            }
        }
        else
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    return status;
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
    NTSTATUS status;
    MIB_TCPSTATS_LH statistics;
    PTCP_KEY_ROW keyTable = NULL;
    PNSI_TCP_STATUS_ROW dynamicTable = NULL;
    PNSI_TCP_OWNER_ROW staticTable = NULL;
    ULONG count = 0;
    ULONG errorCount = 0;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(&statistics, sizeof(MIB_TCPSTATS_LH));
    status = PhGetTcpStatisticsEx(&statistics, AF_INET6);
    if (!NT_SUCCESS(status))
        return status;

    status = PhpNetioMibCheckTableBufferSize(FIELD_OFFSET(MIB_TCP6TABLE2, table), statistics.dwNumConns, sizeof(MIB_TCP6ROW2), TcpTable, SizePointer);
    if (!NT_SUCCESS(status))
        return status;

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTcpObjectAllConnections,
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

    if (NT_SUCCESS(status))
    {
        TcpTable->dwNumEntries = 0;

        for (ULONG i = 0; i < count; i++)
        {
            const PTCP_KEY_ROW key = &keyTable[i];
            
            if (key->Family == AF_INET6 &&
                !IN6_IS_ADDR_V4MAPPED(&key->LocalAddress))
            {
                if (PhpNetioMibTableHasCapacity(
                    FIELD_OFFSET(MIB_TCP6TABLE2, table),
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
            FIELD_OFFSET(MIB_TCP6TABLE2, table),
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
                    qsort_s(TcpTable->table, TcpTable->dwNumEntries, sizeof(MIB_TCP6ROW2), PhpCompareMibTcp6Row2, NULL);
                }
                return STATUS_SUCCESS;
            }
        }
        else
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    return status;
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
    NTSTATUS status;
    MIB_UDPSTATS statistics;
    PUDP_KEY_ROW keyTable = NULL;
    ULONG count = 0;
    ULONG errorCount = 0;
    ULONG requiredSize = 0;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(&statistics, sizeof(MIB_UDPSTATS));
    status = PhGetUdpStatisticsEx(&statistics, AF_INET6);

    if (!NT_SUCCESS(status))
        return status;

    status = PhpNetioMibCheckTableBufferSize(
        FIELD_OFFSET(MIB_UDP6TABLE, table),
        statistics.dwNumAddrs,
        sizeof(MIB_UDP6ROW),
        Udp6Table,
        SizePointer
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhNsiAllocateAndGetTable(
        NsiActive,
        &NPI_MS_UDP_MODULEID,
        NsiUdpObjectEndpoint,
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

    if (NT_SUCCESS(status))
    {
        Udp6Table->dwNumEntries = 0;

        for (ULONG i = 0; i < count; i++)
        {
            PUDP_KEY_ROW row = &keyTable[i];
            
            if (row->Family == AF_INET6 &&
                !IN6_IS_ADDR_V4MAPPED(&row->Address))
            {
                if (PhpNetioMibTableHasCapacity(FIELD_OFFSET(MIB_UDP6TABLE, table), Udp6Table->dwNumEntries + 1, sizeof(MIB_UDP6ROW), *SizePointer))
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
            FIELD_OFFSET(MIB_UDP6TABLE, table),
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
                    qsort_s(Udp6Table->table, Udp6Table->dwNumEntries, sizeof(MIB_UDP6ROW), PhpCompareMibUdp6Row, NULL);
                }
                return STATUS_SUCCESS;
            }
        }
        else
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    return status;
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
    _In_opt_ PVOID CallerContext,
    _In_opt_ PVOID ObjectKey,
    _In_opt_ PVOID ObjectParameter,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    PPH_ROUTE_CHANGE_CONTEXT Context = CallerContext;
    PPH_ROUTE_CHANGE_KEY ChangeKey = ObjectKey;
    PPH_ROUTE_CHANGE_PARAMETER ChangeParameter = ObjectParameter;
    PMIB_IPFORWARD_ROW2 row = NULL;

    if (ChangeKey && ChangeKey->Key && ChangeKey->KeyLength)
    {
        row = &Context->Row;
        RtlZeroMemory(row, sizeof(MIB_IPFORWARD_ROW2));

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

    Context->Callback.RouteCallback(
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
    RPC_BINDING_HANDLE rpcHandle;
    NTSTATUS status;
    NTSTATUS ipv4Result = STATUS_NOT_SUPPORTED;

    if ((AddressFamily != AF_UNSPEC && AddressFamily != AF_INET && AddressFamily != AF_INET6) ||
        !Callback || !NotificationHandle)
        return STATUS_INVALID_PARAMETER;

    registration = PhAllocateZeroSafe(sizeof(NETIO_ROUTE_CHANGE_REGISTRATION));

    if (!registration)
        return STATUS_NO_MEMORY;

    registration->Header.ObjectId = NsiIpObjectRoute;
    registration->Header.Ipv4CallbackContext = &registration->Ipv4Callback;
    registration->Header.Ipv6CallbackContext = &registration->Ipv6Callback;
    registration->Ipv4Callback.Family = AF_INET;
    registration->Ipv4Callback.RouteCallback = Callback;
    registration->Ipv4Callback.CallerContext = CallerContext;
    registration->Ipv6Callback.Family = AF_INET6;
    registration->Ipv6Callback.RouteCallback = Callback;
    registration->Ipv6Callback.CallerContext = CallerContext;

    status = PhNsiNotificationConnectToServer(&rpcHandle);

    if (!NT_SUCCESS(status))
    {
        PhFree(registration);
        return status;
    }

    if (AddressFamily != AF_INET6)
    {
        ipv4Result = PhNsiRpcRegisterChangeNotification(rpcHandle, &NPI_MS_IPV4_MODULEID,
            NsiIpObjectRoute, PhpRouteChangeCallback, InitialNotification,
            &registration->Ipv4Callback, &registration->Header.Ipv4NotificationHandle);
    }

    status = ipv4Result;
    if (AddressFamily != AF_INET)
    {
        status = PhNsiRpcRegisterChangeNotification(rpcHandle, &NPI_MS_IPV6_MODULEID,
            NsiIpObjectRoute, PhpRouteChangeCallback, InitialNotification,
            &registration->Ipv6Callback, &registration->Header.Ipv6NotificationHandle);
        if (AddressFamily == AF_UNSPEC)
            status = (!NT_SUCCESS(ipv4Result) && !NT_SUCCESS(status)) ? ipv4Result : STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status))
    {
        PhCancelMibChangeNotify2(registration);
        return status;
    }

    *NotificationHandle = registration;
    return STATUS_SUCCESS;
}

/**
 * Retrieves TCP statistics for IPv4 or IPv6.
 *
 * \param Statistics Receives the statistics.
 * \param Family The address family (AF_INET or AF_INET6).
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcpstatisticsex
 */
NTSTATUS PhGetTcpStatisticsEx(
    _Out_ PMIB_TCPSTATS Statistics,
    _In_ ULONG Family
    )
{
    NTSTATUS status;
    ADDRESS_FAMILY familyKey;
    NSI_TCP_GLOBAL_ROS readOnlyParams;
    NETIO_TCP_STATS_DYNAMIC dynamicParams;

    if (!Statistics)
        return STATUS_INVALID_PARAMETER;

    if (Family != AF_INET && Family != AF_INET6)
        return STATUS_NOT_SUPPORTED;

    familyKey = (USHORT)Family;
    RtlZeroMemory(&dynamicParams, sizeof(NETIO_TCP_STATS_DYNAMIC));
    RtlZeroMemory(&readOnlyParams, sizeof(NSI_TCP_GLOBAL_ROS));

    status = PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTcpObjectGlobal,
        &familyKey,
        sizeof(familyKey),
        NULL,
        0,
        &dynamicParams,
        sizeof(NETIO_TCP_STATS_DYNAMIC),
        &readOnlyParams,
        sizeof(NSI_TCP_GLOBAL_ROS)
        );

    if (NT_SUCCESS(status))
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

    return status;
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
    NTSTATUS status;
    USHORT portValue = 0;

    if (!Port)
        return STATUS_INVALID_PARAMETER;

    status = PhNsiGetAllParameters(
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

    if (NT_SUCCESS(status))
        *Port = portValue;

    return status;
}

/**
 * Retrieves UDP statistics for IPv4 or IPv6.
 *
 * \param Statistics Receives the statistics.
 * \param Family The address family (AF_INET or AF_INET6).
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getudpstatisticsex
 */
NTSTATUS PhGetUdpStatisticsEx(
    _Out_ PMIB_UDPSTATS Statistics,
    _In_ ULONG Family
    )
{
    NTSTATUS status;
    ADDRESS_FAMILY familyKey;
    NETIO_UDP_STATS_DYNAMIC dynamicParams;

    if (!Statistics)
        return STATUS_INVALID_PARAMETER;

    if (Family != AF_INET && Family != AF_INET6)
        return STATUS_NOT_SUPPORTED;

    familyKey = (USHORT)Family;
    RtlZeroMemory(&dynamicParams, sizeof(NETIO_UDP_STATS_DYNAMIC));

    status = PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_UDP_MODULEID,
        NsiUdpObjectGlobal,
        &familyKey,
        sizeof(familyKey),
        NULL,
        0,
        &dynamicParams,
        sizeof(NETIO_UDP_STATS_DYNAMIC),
        NULL,
        0
        );

    if (NT_SUCCESS(status))
    {
        Statistics->dwInDatagrams = (ULONG)dynamicParams.InDatagrams;
        Statistics->dwNoPorts = dynamicParams.NoPorts;
        Statistics->dwInErrors = dynamicParams.InErrors;
        Statistics->dwOutDatagrams = (ULONG)dynamicParams.OutDatagrams;
        Statistics->dwNumAddrs = dynamicParams.NumAddrs;
    }

    return status;
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
    NTSTATUS status;

    if (!NumberOfInterfaces)
        return STATUS_INVALID_PARAMETER;

    status = PhNsiEnumerateObjectsAllParameters(
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

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
    {
        *NumberOfInterfaces = count;
        return STATUS_SUCCESS;
    }

    return status;
}

/**
 * Retrieves TCP statistics with 64-bit counters for IPv4 or IPv6.
 *
 * \param[out] Statistics Receives the statistics.
 * \param[in] Family The address family (AF_INET or AF_INET6).
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcpstatisticsex2
 */
NTSTATUS PhGetTcpStatisticsEx2(
    _Out_ PNETIO_TCP_STATISTICS_EX2 Statistics,
    _In_ ADDRESS_FAMILY Family
    )
{
    NTSTATUS status;
    ADDRESS_FAMILY familyKey;
    NETIO_TCP_STATS_DYNAMIC dynamicParams;
    NSI_TCP_GLOBAL_ROS readOnlyParams;

    if (!Statistics)
        return STATUS_INVALID_PARAMETER;

    if (Family != AF_INET && Family != AF_INET6)
        return STATUS_NOT_SUPPORTED;

    familyKey = Family;
    RtlZeroMemory(&dynamicParams, sizeof(NETIO_TCP_STATS_DYNAMIC));
    RtlZeroMemory(&readOnlyParams, sizeof(NSI_TCP_GLOBAL_ROS));

    status = PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_TCP_MODULEID,
        NsiTcpObjectGlobal,
        &familyKey,
        sizeof(familyKey),
        NULL,
        0,
        &dynamicParams,
        sizeof(NETIO_TCP_STATS_DYNAMIC),
        &readOnlyParams,
        sizeof(NSI_TCP_GLOBAL_ROS)
        );

    if (NT_SUCCESS(status))
    {
        Statistics->GlobalParameters = readOnlyParams.GlobalParameters;
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

    return status;
}

/**
 * Retrieves UDP statistics with 64-bit counters for IPv4 or IPv6.
 *
 * \param[out] Statistics Receives the statistics.
 * \param[in] Family The address family (AF_INET or AF_INET6).
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getudpstatisticsex2
 */
NTSTATUS PhGetUdpStatisticsEx2(
    _Out_ PNETIO_UDP_STATISTICS_EX2 Statistics,
    _In_ ADDRESS_FAMILY Family
    )
{
    NTSTATUS status;
    ADDRESS_FAMILY familyKey;
    NETIO_UDP_STATS_DYNAMIC dynamicParams;

    if (!Statistics)
        return STATUS_INVALID_PARAMETER;

    if (Family != AF_INET && Family != AF_INET6)
        return STATUS_NOT_SUPPORTED;

    familyKey = Family;
    RtlZeroMemory(&dynamicParams, sizeof(NETIO_UDP_STATS_DYNAMIC));

    status = PhNsiGetAllParameters(
        NsiActive,
        &NPI_MS_UDP_MODULEID,
        NsiUdpObjectGlobal,
        &familyKey,
        sizeof(familyKey),
        NULL,
        0,
        &dynamicParams,
        sizeof(NETIO_UDP_STATS_DYNAMIC),
        NULL,
        0
        );

    if (NT_SUCCESS(status))
    {
        memcpy(&Statistics->NativeStatistics, &dynamicParams, sizeof(Statistics->NativeStatistics));
    }

    return status;
}

/**
 * Retrieves the legacy interface table for all compartments.
 *
 * \param IfTable The buffer that receives the table.
 * \param SizePointer On input, the size of the buffer in bytes; on output, the required size.
 * \param Order TRUE to sort the returned table.
 * \return An NTSTATUS value. STATUS_BUFFER_TOO_SMALL is returned when the buffer is too small and SizePointer receives the required size.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getiftable
 */
NTSTATUS PhGetIfTable(
    _Out_writes_bytes_opt_(*SizePointer) PMIB_IFTABLE IfTable,
    _Inout_ PULONG SizePointer,
    _In_ BOOL Order
    )
{
    NTSTATUS status;
    ULONG entryCount;
    NET_IF_COMPARTMENT_ID currentCompartmentId;
    ULONG count;
    NET_IF_COMPARTMENT_ID i;
    NDIS_NSI_INTERFACE_INFORMATION_ROD* dynamicRow;
    NDIS_NSI_INTERFACE_INFORMATION_ROD* dynamicTable = NULL;
    NDIS_NSI_INTERFACE_INFORMATION_RW* readWriteTable = NULL;
    NET_LUID* keyTable = NULL;
    NDIS_NSI_INTERFACE_INFORMATION_ROS* staticTable = NULL;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    status = PhNsiAllocateAndGetTable(
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

    if (NT_SUCCESS(status))
    {
        entryCount = PhpNetioMibCheckTableBufferSize(
            FIELD_OFFSET(MIB_IFTABLE, table),
            count,
            sizeof(MIB_IFROW),
            IfTable,
            SizePointer
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
                qsort_s(IfTable->table, entryCount, sizeof(MIB_IFROW), PhpCompareMibIfRow, NULL);
            }
            
            entryCount = (entryCount == 0) ? STATUS_NO_MORE_ENTRIES : 0;
        }

        PhNsiFreeTable(keyTable, readWriteTable, dynamicTable, staticTable);
        
        return entryCount;
    }

    return status;
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
    _Outptr_ PMIB_IF_TABLE2 *Table
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
        PH_TCP_OWNER_CLASS ownerClass;

        switch (TableClass)
        {
        case TCP_TABLE_BASIC_LISTENER:
            ownerClass = PhTcpOwnerBasic;
            break;
        case TCP_TABLE_BASIC_CONNECTIONS:
            ownerClass = PhTcpOwnerBasic;
            break;
        case TCP_TABLE_BASIC_ALL:
            ownerClass = PhTcpOwnerBasic;
            break;
        case TCP_TABLE_OWNER_PID_LISTENER:
            ownerClass = PhTcpOwnerPid;
            break;
        case TCP_TABLE_OWNER_PID_CONNECTIONS:
            ownerClass = PhTcpOwnerPid;
            break;
        case TCP_TABLE_OWNER_PID_ALL:
            ownerClass = PhTcpOwnerPid;
            break;
        case TCP_TABLE_OWNER_MODULE_LISTENER:
            ownerClass = PhTcpOwnerModule;
            break;
        case TCP_TABLE_OWNER_MODULE_CONNECTIONS:
            ownerClass = PhTcpOwnerModule;
            break;
        case TCP_TABLE_OWNER_MODULE_ALL:
            ownerClass = PhTcpOwnerModule;
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
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhTcpOwnerBasic);
    case TCP_TABLE_BASIC_CONNECTIONS:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhTcpOwnerBasic);
    case TCP_TABLE_BASIC_ALL:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhTcpOwnerBasic);
    case TCP_TABLE_OWNER_PID_LISTENER:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhTcpOwnerPid);
    case TCP_TABLE_OWNER_PID_CONNECTIONS:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhTcpOwnerPid);
    case TCP_TABLE_OWNER_PID_ALL:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhTcpOwnerPid);
    case TCP_TABLE_OWNER_MODULE_LISTENER:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhTcpOwnerModule);
    case TCP_TABLE_OWNER_MODULE_CONNECTIONS:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhTcpOwnerModule);
    case TCP_TABLE_OWNER_MODULE_ALL:
        return PhpGetTcp6TableInternal(TcpTable, SizePointer, Order, TableClass, PhTcpOwnerModule);
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
 * \param Family The address family (AF_INET or AF_INET6).
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
    NTSTATUS status;
    ULONG reassemblyTimeout;
    NET_IF_COMPARTMENT_ID compartmentKey;
    NSI_IP_GLOBAL_RW globalRw;
    NSI_IP_STATISTICS_ROD statistics;
    NSI_IP_COMPARTMENT_RW compartmentRw;
    NSI_IP_COMPARTMENT_ROD compartmentRod;

    RtlZeroMemory(&globalRw, sizeof(NSI_IP_GLOBAL_RW));
    RtlZeroMemory(&statistics, sizeof(NSI_IP_STATISTICS_ROD));
    RtlZeroMemory(&compartmentRw, sizeof(NSI_IP_COMPARTMENT_RW));
    RtlZeroMemory(&compartmentRod, sizeof(NSI_IP_COMPARTMENT_ROD));
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

    status = PhNsiGetAllParameters(
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

    if (!NT_SUCCESS(status))
        return status;

    compartmentKey = PhGetCurrentThreadCompartmentId();
    status = PhNsiGetAllParameters(
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

    if (!NT_SUCCESS(status))
        return status;

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
    RtlZeroMemory(Row, sizeof(MIB_IPINTERFACE_ROW));
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
    memcpy(Row->ZoneIndices, Rw->ZoneIndices, (sizeof(ULONG) * RTL_NUMBER_OF(Row->ZoneIndices)));
    Row->SitePrefixLength = Rw->SitePrefixLength;
    Row->Metric = Rw->Metric;
    Row->NlMtu = Rw->NlMtu;
    Row->Connected = !!Rod->Connected;
    Row->SupportsWakeUpPatterns = Rod->SupportsWakeUpPatterns;
    Row->SupportsNeighborDiscovery = Ros->SupportsNeighborDiscovery;
    Row->SupportsRouterDiscovery = Ros->SupportsRouterDiscovery;
    Row->ReachableTime = Rod->ReachableTime;
    memcpy(&Row->TransmitOffload, &Rod->TransmitOffload, sizeof(NL_INTERFACE_OFFLOAD_ROD));
    memcpy(&Row->ReceiveOffload, &Rod->ReceiveOffload, sizeof(NL_INTERFACE_OFFLOAD_ROD));
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
    NTSTATUS status;
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

        status = PhConvertInterfaceIndexToLuid(Row->InterfaceIndex, &interfaceLuid);
        if (!NT_SUCCESS(status))
            return status;
    }

    status = PhNsiGetAllParameters(
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

    if (!NT_SUCCESS(status))
        return status;

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
    SIZE_T tableSize;
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
    NTSTATUS status;
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

        status = PhNsiAllocateAndGetTable(
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

        if (status == STATUS_NOT_FOUND || status == STATUS_NOT_SUPPORTED)
            continue;

        if (!NT_SUCCESS(status))
            goto Cleanup;

        for (j = 0; j < families[i].Count; j++)
        {
            if (families[i].Rod[j].CompartmentId == compartmentId)
            {
                if (count == ULONG_MAX)
                {
                    status = STATUS_INTEGER_OVERFLOW;
                    goto Cleanup;
                }

                count++;
            }
        }
    }

    status = RtlSIZETMult(count, sizeof(MIB_IPINTERFACE_ROW), &tableSize);

    if (!NT_SUCCESS(status))
        goto Cleanup;

    status = RtlSIZETAdd(FIELD_OFFSET(MIB_IPINTERFACE_TABLE, Table), tableSize, &tableSize);

    if (!NT_SUCCESS(status))
        goto Cleanup;

    if (!(table = PhAllocateZeroSafe(tableSize)))
    {
        status = STATUS_NO_MEMORY;
        goto Cleanup;
    }
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
    status = STATUS_SUCCESS;

Cleanup:
    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (families[i].Keys || families[i].Rw || families[i].Rod || families[i].Ros)
            PhNsiFreeTable(families[i].Keys, families[i].Rw, families[i].Rod, families[i].Ros);
    }

    return status;
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

    RtlZeroMemory(Row, sizeof(MIB_IPNET_ROW2));

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

    RtlZeroMemory(families, sizeof(PH_NETIO_IPNET_FAMILY) * RTL_NUMBER_OF(families));

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
        NTSTATUS status = PhNsiAllocateAndGetTable(
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

        if (status != STATUS_SUCCESS)
        {
            families[i].Count = 0;
            families[i].KeyTable = NULL;
            families[i].RwTable = NULL;
            families[i].RodTable = NULL;
        }

        total += families[i].Count;
    }

    table = PhAllocateZeroSafe(FIELD_OFFSET(MIB_IPNET_TABLE2, Table) + (SIZE_T)total * sizeof(MIB_IPNET_ROW2));

    if (!table)
    {
        for (i = 0; i < familyCount; i++)
        {
            if (families[i].KeyTable || families[i].RwTable || families[i].RodTable)
                PhNsiFreeTable(families[i].KeyTable, families[i].RwTable, families[i].RodTable, NULL);
        }

        return STATUS_NO_MEMORY;
    }

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
    RtlZeroMemory(&interfaceLuid, sizeof(NET_LUID));
    RtlZeroMemory(&bandwidthRow, sizeof(NSI_IP_INTERFACE_ROD));

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

    if (!NT_SUCCESS(status))
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

    if (!NT_SUCCESS(status))
        return status;

    RtlZeroMemory(BandwidthEstimates, sizeof(MIB_IP_NETWORK_CONNECTION_BANDWIDTH_ESTIMATES));
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
    SIZE_T tableSize;
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
    NTSTATUS status;
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

        status = PhNsiAllocateAndGetTable(
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

        if (status == STATUS_NOT_FOUND || status == STATUS_NOT_SUPPORTED)
            continue;

        if (!NT_SUCCESS(status))
            goto Cleanup;

        for (j = 0; j < families[i].Count; j++)
        {
            const ULONG *key = PTR_ADD_OFFSET(families[i].Keys, (SIZE_T)j * families[i].KeySize);
            if (*key == compartmentId)
                count++;
        }
    }

    status = RtlSIZETMult(count, sizeof(MIB_IPFORWARD_ROW2), &tableSize);

    if (!NT_SUCCESS(status))
        goto Cleanup;

    status = RtlSIZETAdd(FIELD_OFFSET(MIB_IPFORWARD_TABLE2, Table), tableSize, &tableSize);

    if (!NT_SUCCESS(status))
        goto Cleanup;

    if (!(table = PhAllocateZeroSafe(tableSize)))
    {
        status = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    entryIndex = 0;

    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        for (j = 0; j < families[i].Count; j++)
        {
            const VOID *key = PTR_ADD_OFFSET(families[i].Keys, (SIZE_T)j * families[i].KeySize);
            const VOID *rod = PTR_ADD_OFFSET(families[i].Rod, (SIZE_T)j * families[i].RodSize);

            if (*(const ULONG *)key != compartmentId)
                continue;

            PhNsiFillRouteEntry(families[i].Family, &table->Table[entryIndex], key,
                &families[i].Rw[j], rod, &families[i].Ros[j]);
            entryIndex++;
        }
    }

    table->NumEntries = entryIndex;
    *Table = table;
    status = STATUS_SUCCESS;

Cleanup:
    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (families[i].Keys || families[i].Rw || families[i].Rod || families[i].Ros)
            PhNsiFreeTable(families[i].Keys, families[i].Rw, families[i].Rod, families[i].Ros);
    }

    return status;
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
    NTSTATUS status;
    NET_LUID InterfaceLuid;

    InterfaceLuid.Value = 0;
    status = PhConvertInterfaceAliasToLuid(InterfaceName, &InterfaceLuid);
    if (NT_SUCCESS(status))
        return PhConvertInterfaceLuidToGuid(&InterfaceLuid, InterfaceGuid);

    return status;
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
    NTSTATUS status;
    NET_LUID InterfaceLuid;

    InterfaceLuid.Value = 0;
    status = PhConvertInterfaceGuidToLuid(InterfaceGuid, &InterfaceLuid);

    if (NT_SUCCESS(status))
    {
        status = PhConvertInterfaceLuidToAlias(&InterfaceLuid, InterfaceName, *InterfaceNameLength >> 1);

        if (status == STATUS_NO_MEMORY)
        {
            *InterfaceNameLength = (IF_MAX_STRING_SIZE + 1) * sizeof(WCHAR);
        }
    }

    return status;
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

    RtlZeroMemory(&key, sizeof(NSI_TCP_KEY_ROW));
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
        ReadWriteBuffer,
        ReadWriteBufferSize,
        DynamicBuffer,
        DynamicBufferSize,
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

    RtlZeroMemory(&key, sizeof(NSI_TCP_KEY_ROW));
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
        ReadWriteBuffer,
        ReadWriteBufferSize,
        DynamicBuffer,
        DynamicBufferSize,
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
    _Outptr_ PMIB_IFSTACK_TABLE *Table
    )
{
    NTSTATUS status;
    PMIB_IFSTACK_TABLE memory;
    ULONG count = 0;
    ULONG dwBytes;
    MIB_NDIS_IFSTACK_KEY_ROW* keyTable = NULL;

    if (!Table)
        return STATUS_INVALID_PARAMETER;

    *Table = NULL;

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

            if (PhpNetioMibSizeofTable(FIELD_OFFSET(MIB_IFSTACK_TABLE, Table), count, sizeof(MIB_IFSTACK_ROW), FALSE, &dwBytes))
            {
                memory = PhAllocateSafe(dwBytes);
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
    _Outptr_ PMIB_INVERTEDIFSTACK_TABLE *Table
    )
{
    NTSTATUS status;
    PMIB_IFSTACK_TABLE stackTable;

    if (!Table)
        return STATUS_INVALID_PARAMETER;

    *Table = NULL;
    status = PhGetIfStackTable(&stackTable);
    if (!NT_SUCCESS(status))
        return status;

    for (ULONG index = 0; index < stackTable->NumEntries; index++)
    {
        NET_IFINDEX higherIndex = stackTable->Table[index].HigherLayerInterfaceIndex;
        stackTable->Table[index].HigherLayerInterfaceIndex = stackTable->Table[index].LowerLayerInterfaceIndex;
        stackTable->Table[index].LowerLayerInterfaceIndex = higherIndex;
    }

    *Table = (PMIB_INVERTEDIFSTACK_TABLE)stackTable;
    return STATUS_SUCCESS;
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
    NTSTATUS status;
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

        status = PhConvertInterfaceIndexToLuid(Row->InterfaceIndex, &interfaceLuid);

        if (!NT_SUCCESS(status))
            return status;
    }

    if (Row->DestinationPrefix.Prefix.si_family == AF_INET)
    {
        if (Row->DestinationPrefix.PrefixLength > 32 || Row->NextHop.si_family != AF_INET)
            return STATUS_INVALID_PARAMETER;

        moduleId = (PNPI_MODULEID)&NPI_MS_IPV4_MODULEID;
        RtlZeroMemory(&keyV4, sizeof(MIB_IP_ROUTE_KEY_ROW_V4));
        keyV4.CompartmentId = PhGetCurrentThreadCompartmentId();
        keyV4.DestinationPrefix = Row->DestinationPrefix.Prefix.Ipv4.sin_addr;
        keyV4.DestinationPrefixLength = Row->DestinationPrefix.PrefixLength;
        keyV4.InterfaceLuid = interfaceLuid;
        keyV4.NextHop = Row->NextHop.Ipv4.sin_addr;

        status = PhNsiGetAllParameters(
            NsiActive, 
            moduleId, 
            NsiIpObjectRoute,
            &keyV4, 
            sizeof(MIB_IP_ROUTE_KEY_ROW_V4),
            &rw, 
            sizeof(MIB_IP_ROUTE_RW_ROW),
            &rodV4, 
            sizeof(MIB_IP_ROUTE_DYNAMIC_ROW_V4),
            &ros,
            sizeof(MIB_IP_ROUTE_STATIC_ROW)
            );

        if (NT_SUCCESS(status))
        {
            PhNsiFillRouteEntry(AF_INET, Row, &keyV4, &rw, &rodV4, &ros);
        }
    }
    else
    {
        if (Row->DestinationPrefix.PrefixLength > 128 || Row->NextHop.si_family != AF_INET6)
            return STATUS_INVALID_PARAMETER;

        moduleId = (PNPI_MODULEID)&NPI_MS_IPV6_MODULEID;
        RtlZeroMemory(&keyV6, sizeof(MIB_IP_ROUTE_KEY_ROW_V6));
        keyV6.CompartmentId = PhGetCurrentThreadCompartmentId();
        keyV6.DestinationPrefix = Row->DestinationPrefix.Prefix.Ipv6.sin6_addr;
        keyV6.DestinationPrefixLength = Row->DestinationPrefix.PrefixLength;
        keyV6.InterfaceLuid = interfaceLuid;
        keyV6.NextHop = Row->NextHop.Ipv6.sin6_addr;

        status = PhNsiGetAllParameters(
            NsiActive, 
            moduleId, 
            NsiIpObjectRoute,
            &keyV6, 
            sizeof(MIB_IP_ROUTE_KEY_ROW_V6), 
            &rw,
            sizeof(MIB_IP_ROUTE_RW_ROW), 
            &rodV6, 
            sizeof(MIB_IP_ROUTE_DYNAMIC_ROW_V6), 
            &ros, 
            sizeof(MIB_IP_ROUTE_STATIC_ROW)
            );

        if (NT_SUCCESS(status))
        {
            PhNsiFillRouteEntry(AF_INET6, Row, &keyV6, &rw, &rodV6, &ros);
        }
    }

    return status;
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

    RtlZeroMemory(&destination, sizeof(struct sockaddr_in));
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

        RtlZeroMemory(&keyV4, sizeof(NSI_IP_BEST_ROUTE_KEY4));
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

        RtlZeroMemory(&keyV6, sizeof(NSI_IP_BEST_ROUTE_KEY6));
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
        return PhNetioDosErrorToNtStatus(ReleaseParameters(AdapterName, Reserved));
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
        return PhNetioDosErrorToNtStatus(AcquireParameters(AdapterName, Reserved));
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

    SIZE_T length = PhCountStringZ(AdapterInfo->Name);

    if (length > TCPIP_ADAPTER_PREFIX_LENGTH)
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

    SIZE_T length = PhCountStringZ(AdapterInfo->Name);

    if (length > TCPIP_ADAPTER_PREFIX_LENGTH)
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
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success. STATUS_UNSUCCESSFUL is returned when the destination
 * returned an unexpected IP_STATUS, the timing probes got no reply, or the destination was not reached within MaxHops.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getrttandhopcount
 */
NTSTATUS PhGetRTTAndHopCount(
    _In_ IPAddr DestIpAddress,
    _Out_ PULONG HopCount,
    _In_ ULONG MaxHops,
    _Out_ PULONG RTT
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    IP_STATUS ipStatus;
    ULONG replyCount;
    PVOID replyBuffer;
    struct ip_option_information requestOptions;
    UCHAR requestData[PH_ICMP_ECHO_DATA_SIZE];

    if (!HopCount || !RTT || DestIpAddress == INADDR_NONE)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(&requestOptions, sizeof(struct ip_option_information));

    status = PhIcmpCreateFileEx(&NPI_MS_IPV4_MODULEID, TRUE, &fileHandle);

    if (!NT_SUCCESS(status))
        return status;

    replyBuffer = PhAllocateSafe(PH_ICMP_ECHO_REPLY_BUFFER_SIZE);

    if (!replyBuffer)
    {
        PhIcmpCloseHandle(fileHandle);
        return STATUS_NO_MEMORY;
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(requestData); i++)
    {
        requestData[i] = 'a' + (UCHAR)(i % PH_ICMP_ECHO_PATTERN_LENGTH);
    }

    requestOptions.Tos = 0;
    status = STATUS_UNSUCCESSFUL;

    for (ULONG ttl = 1; ttl <= min(MaxHops, MAXUCHAR); ttl++)
    {
        requestOptions.Ttl = (UCHAR)ttl;

        status = PhIcmpSendEcho2Ex(
            fileHandle,
            NULL,
            NULL,
            NULL,
            0,
            DestIpAddress,
            requestData,
            RTL_NUMBER_OF(requestData),
            &requestOptions,
            replyBuffer,
            PH_ICMP_ECHO_REPLY_BUFFER_SIZE,
            PH_ICMP_ECHO_TIMEOUT,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;

        ipStatus = ((PICMP_ECHO_REPLY)replyBuffer)->Status;

        if (ipStatus == IP_SUCCESS)
        {
            // Two further probes at the same TTL; the RTT comes from the second.
            for (ULONG probe = 0; probe < 2; probe++)
            {
                status = PhIcmpSendEcho2Ex(
                    fileHandle,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    DestIpAddress,
                    requestData,
                    RTL_NUMBER_OF(requestData),
                    &requestOptions,
                    replyBuffer,
                    PH_ICMP_ECHO_REPLY_BUFFER_SIZE,
                    PH_ICMP_ECHO_TIMEOUT,
                    &replyCount
                    );

                if (NT_SUCCESS(status) && replyCount == 0)
                    status = STATUS_UNSUCCESSFUL;

                if (!NT_SUCCESS(status))
                    break;
            }

            if (NT_SUCCESS(status))
            {
                *HopCount = requestOptions.Ttl;
                *RTT = ((PICMP_ECHO_REPLY)replyBuffer)->RoundTripTime;
            }

            break;
        }

        status = STATUS_UNSUCCESSFUL;

        if (ipStatus != IP_TTL_EXPIRED_TRANSIT && ipStatus != IP_REQ_TIMED_OUT)
            break;
    }

    PhFree(replyBuffer);
    PhIcmpCloseHandle(fileHandle);
    return status;
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
    RtlZeroMemory(Row, sizeof(MIB_IPPATH_ROW));

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
        Row->RttMean = rod->RttMean / PH_NETIO_RTT_SCALE;
        Row->RttDeviation = rod->RttDeviation / PH_NETIO_RTT_SCALE;
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
        Row->RttMean = rod->RttMean / PH_NETIO_RTT_SCALE;
        Row->RttDeviation = rod->RttDeviation / PH_NETIO_RTT_SCALE;
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
    NTSTATUS status;
    NET_IF_COMPARTMENT_ID CurrentThreadCompartmentId;
    NET_LUID* keyTable = NULL;
    PNDIS_NSI_INTERFACE_ENUM_ROD rodTable = NULL;
    PNDIS_NSI_INTERFACE_ENUM_ROS rosTable = NULL;
    ULONG count = 0;
    ULONG outputCount = 0;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    status = PhNsiAllocateAndGetTable(
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

    if (NT_SUCCESS(status))
    {
        CurrentThreadCompartmentId = PhGetCurrentThreadCompartmentId();

        for (ULONG i = 0; i < count; i++)
        {
            if (rodTable[i].CompartmentId == CurrentThreadCompartmentId)
            {
                if (PhpIsIpv4Interface(&keyTable[i], rosTable[i].ifType))
                {
                    if (IfTable && PhpNetioMibTableHasCapacity(FIELD_OFFSET(IP_INTERFACE_INFO, Adapter), outputCount + 1, sizeof(IP_ADAPTER_INDEX_MAP), *SizePointer))
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

        status = PhpNetioMibCheckTableBufferSize(
            FIELD_OFFSET(IP_INTERFACE_INFO, Adapter),
            outputCount,
            sizeof(IP_ADAPTER_INDEX_MAP),
            IfTable,
            SizePointer
            );

        if (NT_SUCCESS(status) && outputCount == 0)
        {
            status = STATUS_NO_MORE_ENTRIES;
        }

        PhNsiFreeTable(keyTable, NULL, rodTable, rosTable);
    }

    return status;
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
        return PhNetioDosErrorToNtStatus(GetSettings(Settings));
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
        return PhNetioDosErrorToNtStatus(GetInterfaceSettings(InterfaceGuid, Settings));
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
    NTSTATUS status;
    DNS_SETTINGS2 request;

    if (!Settings ||
        Settings->Version < DNS_SETTINGS_VERSION1 ||
        Settings->Version > DNS_SETTINGS_VERSION2)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(&request, sizeof(DNS_SETTINGS2));
    request.Version = DNS_SETTINGS_VERSION1;
    request.Flags = Settings->Flags;

    status = PhDnsGetSettings((DNS_SETTINGS*)&request);

    if (NT_SUCCESS(status))
    {
        Settings->Hostname = request.Hostname;
        Settings->Domain = request.Domain;
        Settings->SearchList = request.SearchList;
        if (Settings->Version == DNS_SETTINGS_VERSION2)
        {
            ((DNS_SETTINGS2*)Settings)->SettingFlags = request.SettingFlags;
        }
    }
    return status;
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
            (sizeof(WCHAR) * RTL_NUMBER_OF(systemName)),
            systemName,
            (sizeof(WCHAR) * RTL_NUMBER_OF(systemName)),
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

        RtlZeroMemory(&tagInformation, sizeof(TAG_INFO_NAME_FROM_TAG));
        tagInformation.InParams.ProcessId = ProcessId;
        tagInformation.InParams.ServiceTag = (ULONG)Info[0];
        status = PhNetioDosErrorToNtStatus(queryTagInformation(NULL, eTagInfoLevelNameFromTag, &tagInformation));
        if (!NT_SUCCESS(status))
            return status;
        if (!tagInformation.OutParams.Name)
            return STATUS_NOT_FOUND;

        serviceNameLength = wcslen(tagInformation.OutParams.Name) * sizeof(WCHAR);
        if (serviceNameLength > MAXULONG)
        {
            LocalFree((HLOCAL)tagInformation.OutParams.Name);
            return STATUS_INTEGER_OVERFLOW;
        }
        status = PhpCopyOwnerModuleInfo(tagInformation.OutParams.Name, (ULONG)serviceNameLength,
            tagInformation.OutParams.Name, (ULONG)serviceNameLength, Buffer, SizePointer);
        LocalFree((HLOCAL)tagInformation.OutParams.Name);
        return status;
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
    status = PhpCopyOwnerModuleInfo(imageName, imageNameLength,
        imagePath->Buffer, (ULONG)imagePath->Length, Buffer, SizePointer);
    PhDereferenceObject(imagePath);
    return status;
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
    SIZE_T tableSize;
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
    NTSTATUS status;
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

        status = PhNsiAllocateAndGetTable(
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

        if (status == STATUS_NOT_FOUND || status == STATUS_NOT_SUPPORTED)
            continue;

        if (!NT_SUCCESS(status))
            goto Cleanup;

        for (j = 0; j < families[i].Count; j++)
        {
            const ULONG *key = PTR_ADD_OFFSET(families[i].Keys, (SIZE_T)j * families[i].KeySize);
            if (*key == compartmentId)
                count++;
        }
    }

    status = RtlSIZETMult(count, sizeof(MIB_IPPATH_ROW), &tableSize);

    if (!NT_SUCCESS(status))
        goto Cleanup;

    status = RtlSIZETAdd(FIELD_OFFSET(MIB_IPPATH_TABLE, Table), tableSize, &tableSize);

    if (!NT_SUCCESS(status))
        goto Cleanup;

    if (!(table = PhAllocateZeroSafe(tableSize)))
    {
        status = STATUS_NO_MEMORY;
        goto Cleanup;
    }
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
    status = STATUS_SUCCESS;

Cleanup:
    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (families[i].Keys || families[i].Rod)
            PhNsiFreeTable(families[i].Keys, NULL, families[i].Rod, NULL);
    }

    return status;
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
    HMODULE iphlpapiModule;
    ULONG lastError;

    if (!Size)
        return STATUS_INVALID_PARAMETER;

    iphlpapiModule = LoadLibraryExW(L"iphlpapi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!iphlpapiModule)
        return PhNetioDosErrorToNtStatus(PhGetLastError());

    length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
        iphlpapiModule,
        ErrorCode,
        0,
        (PWSTR)&message,
        0,
        NULL
        );
    lastError = PhGetLastError();
    FreeLibrary(iphlpapiModule);

    if (!length)
        return PhNetioDosErrorToNtStatus(lastError);

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
    return IfIndex & PH_NET_LUID_INDEX_MASK;
}

/**
 * Retrieves the network compartment scope and identifier of the current thread.
 *
 * \param CompartmentScope Receives the compartment scope.
 * \param CompartmentId Receives the compartment identifier.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getcurrentthreadcompartmentscope
 */
NTSTATUS PhGetCurrentThreadCompartmentScope(
    _Out_ PNET_IF_COMPARTMENT_SCOPE CompartmentScope,
    _Out_opt_ PNET_IF_COMPARTMENT_ID CompartmentId
    )
{
    typedef struct _PH_NETIO_CURRENT_THREAD_COMPARTMENT
    {
        NET_IF_COMPARTMENT_ID CompartmentId;
        NET_IF_COMPARTMENT_SCOPE CompartmentScope;
    } PH_NETIO_CURRENT_THREAD_COMPARTMENT;
    PH_NETIO_CURRENT_THREAD_COMPARTMENT currentCompartment;
    NTSTATUS status;

    RtlZeroMemory(&currentCompartment, sizeof(PH_NETIO_CURRENT_THREAD_COMPARTMENT));

    status = PhNsiGetAllParameters(
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
        );

    if (!NT_SUCCESS(status))
        return status;

    *CompartmentScope = currentCompartment.CompartmentScope;
    if (CompartmentId)
        *CompartmentId = currentCompartment.CompartmentId;

    return STATUS_SUCCESS;
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
    ULONG cost;
    ULONG fetched;
    NTSTATUS status;
    HRESULT hr;

    if (!ConnectivityHint)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(ConnectivityHint, sizeof(NETIO_CONNECTIVITY_HINT));

    status = PhConvertInterfaceIndexToLuid(InterfaceIndex, &interfaceLuid);
    if (!NT_SUCCESS(status)) return status;
    status = PhConvertInterfaceLuidToGuid(&interfaceLuid, &interfaceGuid);
    if (!NT_SUCCESS(status)) return status;

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
    _Inout_ PPH_CONNECTIVITY_HINT_CONTEXT Context
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
    PPH_CONNECTIVITY_HINT_CONTEXT ctx = (PPH_CONNECTIVITY_HINT_CONTEXT)Context;
    PNET_NETWORK_CONNECTIVITY_HINT_CHANGE_CALLBACK callback = (PNET_NETWORK_CONNECTIVITY_HINT_CHANGE_CALLBACK)ctx->Callback;
    NETIO_CONNECTIVITY_HINT hint;

    TpCallbackMayRunLong(Instance);

    do
    {
        PhAcquireQueuedLockExclusive(&PhpConnectivityHintLock);
        hint.ConnectivityLevel = PhpConnectivityLevel;
        hint.ConnectivityCost = PhpConnectivityCost;
        InterlockedExchange(&ctx->State, PH_CONNECTIVITY_CALLBACK_PROCESSING);
        PhReleaseQueuedLockExclusive(&PhpConnectivityHintLock);

        callback(ctx->CallerContext, &hint);
    }
    while (InterlockedCompareExchange(&ctx->State, 0, PH_CONNECTIVITY_CALLBACK_PROCESSING) == 1);
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

    RtlZeroMemory(Hint, sizeof(NETIO_CONNECTIVITY_HINT));

    if ((val & PH_NSI_CONNECTIVITY_CONSTRAINED_MASK) != 0)
        level = NetworkConnectivityLevelHintConstrainedInternetAccess;
    else if ((val & PH_NSI_CONNECTIVITY_INTERNET_MASK) != 0)
        level = NetworkConnectivityLevelHintInternetAccess;
    else if ((val & PH_NSI_CONNECTIVITY_LOCAL_MASK) != 0)
        level = NetworkConnectivityLevelHintLocalAccess;
    else
        level = (ULONG)val != 0 ? NetworkConnectivityLevelHintNone : NetworkConnectivityLevelHintUnknown;

    Hint->ConnectivityLevel = level;

    UCHAR byte4 = (UCHAR)(val >> PH_NSI_CONNECTIVITY_COST_SHIFT);

    if ((byte4 & PH_NSI_CONNECTIVITY_COST_UNDETERMINED) == 0)
    {
        if ((byte4 & PH_NSI_CONNECTIVITY_COST_FIXED) != 0)
            cost = NetworkConnectivityCostHintFixed;
        else
            cost = (byte4 & PH_NSI_CONNECTIVITY_COST_UNRESTRICTED) ? NetworkConnectivityCostHintUnrestricted : NetworkConnectivityCostHintUnknown;
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
                PPH_CONNECTIVITY_HINT_CONTEXT ctx = CONTAINING_RECORD(entry, PH_CONNECTIVITY_HINT_CONTEXT, ListEntry);
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
    WNF_STATE_NAME stateName = { .Value = PH_WNF_CONNECTIVITY_STATE_NAME };
    WNF_CHANGE_STAMP changeStamp = 0;
    ULONG64 stateData = 0;
    ULONG stateDataLength = sizeof(ULONG64);
    NETIO_CONNECTIVITY_HINT hint;
    NTSTATUS status;

    if (PhpConnectivitySubscriptionHandle)
        return STATUS_SUCCESS;

    status = NtQueryWnfStateData(
        &stateName,
        NULL,
        NULL,
        &changeStamp,
        &stateData,
        &stateDataLength
        );

    if (!NT_SUCCESS(status) || stateDataLength != sizeof(ULONG64))
        stateData = 0;

    PhpConnectivityHintFromConnectivityState(&stateData, &hint);
    PhpConnectivityLevel = (ULONG)hint.ConnectivityLevel;
    PhpConnectivityCost = hint.ConnectivityCost;

    status = RtlSubscribeWnfStateChangeNotification(
        (PWNF_USER_SUBSCRIPTION*)&PhpConnectivitySubscriptionHandle,
        stateName,
        changeStamp,
        PhpConnectivityStateChangeCallback,
        NULL,
        NULL,
        0,
        0
        );

    if (!NT_SUCCESS(status))
    {
        PhpConnectivitySubscriptionHandle = NULL;
        return status;
    }

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
    PMIB_NOTIFICATION_HANDLE handle;
    NTSTATUS lastError;
    PPH_CONNECTIVITY_HINT_CONTEXT context;

    *NotificationHandle = NULL;

    handle = PhAllocateZeroSafe(sizeof(MIB_NOTIFICATION_HANDLE));
    if (!handle)
        return STATUS_NO_MEMORY;

    context = PhAllocateSafe(sizeof(PH_CONNECTIVITY_HINT_CONTEXT));
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
    _In_opt_ PVOID CallerContext,
    _In_opt_ PVOID ObjectKey,
    _In_opt_ PVOID ObjectParameter,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    PPH_TEREDO_PORT_CHANGE_CONTEXT Context = CallerContext;
    PNSI_CHANGE_DATA ChangeData = ObjectParameter;
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
    RPC_BINDING_HANDLE rpcHandle;
    NTSTATUS status;

    if (!NotificationHandle || !Callback)
        return STATUS_INVALID_PARAMETER;

    registration = PhAllocateZeroSafe(sizeof(NETIO_TEREDO_PORT_CHANGE_REGISTRATION));

    if (!registration)
        return STATUS_NO_MEMORY;

    registration->Header.ObjectId = NsiFl6tObjectClientPort;
    registration->Header.Ipv4CallbackContext = &registration->Callback;
    registration->Callback = Callback;
    registration->CallerContext = CallerContext;

    status = PhNsiNotificationConnectToServer(&rpcHandle);

    if (!NT_SUCCESS(status))
    {
        PhFree(registration);
        return status;
    }

    status = PhNsiRpcRegisterChangeNotification(
        rpcHandle,
        &NPI_MS_FL6T_MODULEID,
        NsiFl6tObjectClientPort,
        PhpTeredoPortChangeCallback,
        InitialNotification,
        &registration->Callback,
        &registration->Header.TeredoNotificationHandle
        );

    if (!NT_SUCCESS(status))
    {
        PhFree(registration);
        return status;
    }

    *NotificationHandle = registration;
    return STATUS_SUCCESS;
}

/**
 * Retrieves the interface indexes of the adapters in the current thread's compartment.
 *
 * \param AdapterOrderMap Receives the adapter order map. Free it with LocalFree.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadapterordermap
 */
NTSTATUS PhGetAdapterOrderMap(
    _Out_ PIP_ADAPTER_ORDER_MAP* AdapterOrderMap
    )
{
    NTSTATUS status;
    ULONG count;
    NET_IF_COMPARTMENT_ID currentCompartmentId;
    NDIS_NSI_INTERFACE_ENUM_ROD* aliasTable = NULL;
    NDIS_NSI_INTERFACE_ENUM_ROS* guidTable = NULL;
    NET_LUID* keyTable = NULL;
    ULONG activeCount = 0;
    PIP_ADAPTER_ORDER_MAP orderMap;

    status = PhNsiAllocateAndGetTable(
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
        );

    if (!NT_SUCCESS(status))
        return status;

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

    if (!orderMap)
        return STATUS_NO_MEMORY;

    *AdapterOrderMap = orderMap;
    return STATUS_SUCCESS;
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

    NTSTATUS status;
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

    status = PhpAllocateAndGetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, &addresses, NULL);

    if (!NT_SUCCESS(status))
        return status;

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

    RtlZeroMemory(AdapterInfo, requiredSize);

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
                RTL_NUMBER_OF(info->Description),
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
            if (!NT_SUCCESS(PhConvertLengthToIpv4Mask(unicast->OnLinkPrefixLength, &mask.S_un.S_addr)))
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
    NTSTATUS status;
    ULONG count;
    ULONG requiredSize;
    PIP_ADAPTER_ADDRESSES addresses = NULL;
    PIP_ADAPTER_ADDRESSES adapter;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    status = PhpAllocateAndGetAdaptersAddresses(
        AF_INET,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
        &addresses,
        NULL);

    if (!NT_SUCCESS(status))
        return status;

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

    status = RtlULongMult(count, sizeof(IPAddr), &requiredSize);

    if (!NT_SUCCESS(status))
    {
        PhpFreeAdaptersAddresses(addresses);
        return status;
    }

    status = RtlULongAdd(sizeof(IP_UNIDIRECTIONAL_ADAPTER_ADDRESS), requiredSize, &requiredSize);

    if (!NT_SUCCESS(status))
    {
        PhpFreeAdaptersAddresses(addresses);
        return status;
    }

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
    NTSTATUS status;
    PVOID memory = NULL;
    
    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;
    
    status = PhpAllocateAndGetFixedInfo(&memory);
    if (!NT_SUCCESS(status))
        return status;
    
    PFIXED_INFO sourceFixedInfo = (PFIXED_INFO)memory;
    PIP_ADDR_STRING dnsListHead = &sourceFixedInfo->DnsServerList;
    
    ULONG dnsListSize = PhGetSizeofAddressStringList(dnsListHead);
    ULONG sizeRequired = sizeof(FIXED_INFO) + dnsListSize;
    
    if (!FixedInfo || *SizePointer < sizeRequired)
    {
        *SizePointer = sizeRequired;
        PhFreeAddressStringList(dnsListHead);
        PhFree(memory);
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    RtlZeroMemory(FixedInfo, *SizePointer);
    memcpy(FixedInfo, memory, sizeof(FIXED_INFO));
    
    PIP_ADDR_STRING dest = (PIP_ADDR_STRING)(FixedInfo + 1);
    PIP_ADDR_STRING src = dnsListHead->Next;
    PIP_ADDR_STRING tail = &FixedInfo->DnsServerList;
    
    FixedInfo->DnsServerList.Next = NULL;
    
    while (src)
    {
        tail->Next = dest;
        memcpy(dest, src, sizeof(IP_ADDR_STRING));
        tail = dest;
        dest = (PIP_ADDR_STRING)((PCHAR)dest + sizeof(IP_ADDR_STRING));
        src = src->Next;
    }
    
    if (tail)
        tail->Next = NULL;
    
    PhFreeAddressStringList(dnsListHead);
    PhFree(memory);
    *SizePointer = sizeRequired;
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
    NTSTATUS status;

    if (!ConnectivityHint)
        return STATUS_INVALID_PARAMETER;

    PhAcquireQueuedLockExclusive(&PhpConnectivityHintLock);
    status = PhpMonitorConnectivityState();
    if (NT_SUCCESS(status))
    {
        ConnectivityHint->ConnectivityLevel = PhpConnectivityLevel;
        ConnectivityHint->ConnectivityCost = PhpConnectivityCost;
    }
    PhReleaseQueuedLockExclusive(&PhpConnectivityHintLock);

    return status;
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
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE deviceHandle;
    NET_LUID interfaceLuid;
    NETIO_NDIS_CROSSTIMESTAMP_REQUEST request;
    NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA crossTimestamp;

    RtlZeroMemory(&request, sizeof(NETIO_NDIS_CROSSTIMESTAMP_REQUEST));

    if (!Timestamp)
        return STATUS_INVALID_PARAMETER;

    deviceHandle = NULL;
    status = PhConvertInterfaceIndexToLuid(InterfaceIndex, &interfaceLuid);

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetInterfaceHandle(&interfaceLuid, &deviceHandle);

    if (!NT_SUCCESS(status))
        return status;

    RtlZeroMemory(Timestamp, sizeof(NETIO_INTERFACE_HARDWARE_CROSSTIMESTAMP));

    request.RequestHeader.Header.Type = NETIO_NDIS_CROSSTIMESTAMP_REQUEST_TYPE;
    request.RequestHeader.Header.Revision = NETIO_NDIS_CROSSTIMESTAMP_REQUEST_REVISION;
    request.RequestHeader.Header.Size = FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Data);
    request.RequestHeader.Oid = NETIO_OID_TIMESTAMP_GET_CROSSTIMESTAMP;
    request.RequestHeader.DataOffset = FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Data);

    status = NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        IOCTL_NETIO_NDIS_CROSSTIMESTAMP,
        &request,
        sizeof(NETIO_NDIS_CROSSTIMESTAMP_REQUEST),
        &request,
        sizeof(NETIO_NDIS_CROSSTIMESTAMP_REQUEST)
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(deviceHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = ioStatusBlock.Status;
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhConvertNdisStatusToError(request.RequestHeader.Status);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (request.RequestHeader.DataOffset < FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Data) ||
        request.RequestHeader.DataOffset > sizeof(NETIO_NDIS_CROSSTIMESTAMP_REQUEST) - sizeof(NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA))
    {
        status = STATUS_DATA_ERROR;
        goto CleanupExit;
    }

    memcpy(&crossTimestamp, PTR_ADD_OFFSET(&request, request.RequestHeader.DataOffset), sizeof(NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA));
    Timestamp->SystemTimestamp1 = crossTimestamp.SystemTimestamp1;
    Timestamp->HardwareClockTimestamp = crossTimestamp.HardwareClockTimestamp;
    Timestamp->SystemTimestamp2 = crossTimestamp.SystemTimestamp2;

CleanupExit:
    NtClose(deviceHandle);
    return status;
}

#ifdef _WIN64
static_assert(sizeof(PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION) == 0x20);
static_assert(FIELD_OFFSET(PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, NsiNotificationHandle) == 0x08);
static_assert(FIELD_OFFSET(PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, CallerContext) == 0x10);
static_assert(FIELD_OFFSET(PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, Callback) == 0x18);
#else
static_assert(sizeof(PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION) == 0x10);
static_assert(FIELD_OFFSET(PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, NsiNotificationHandle) == 0x04);
static_assert(FIELD_OFFSET(PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, CallerContext) == 0x08);
static_assert(FIELD_OFFSET(PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION, Callback) == 0x0C);
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
    _In_opt_ PVOID CallerContext,
    _In_opt_ PVOID ObjectKey,
    _In_opt_ PVOID ObjectParameter,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    PPH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION Registration = CallerContext;

    if (Registration->Tag == PH_TIMESTAMP_CONFIG_CHANGE_TAG)
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
    PPH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION registration;
    RPC_BINDING_HANDLE rpcHandle;
    NTSTATUS status;

    if (!Callback || !NotificationHandle)
        return STATUS_INVALID_PARAMETER;

    *NotificationHandle = NULL;
    registration = PhAllocateSafe(sizeof(PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION));

    if (!registration)
        return STATUS_NO_MEMORY;

    RtlZeroMemory(registration, sizeof(PH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION));
    registration->Tag = PH_TIMESTAMP_CONFIG_CHANGE_TAG;
    registration->Callback = Callback;
    registration->CallerContext = CallerContext;

    status = PhNsiNotificationConnectToServer(&rpcHandle);

    if (NT_SUCCESS(status))
    {
        status = PhNsiRpcRegisterChangeNotification(
            rpcHandle,
            &NPI_MS_NDIS_MODULEID,
            NsiNdisObjectTimestampConfigChange,
            PhpInterfaceTimestampConfigChangeCallback,
            FALSE,
            registration,
            &registration->NsiNotificationHandle
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(registration);
        return status;
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
    PPH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION registration;

    registration = (PPH_TIMESTAMP_CONFIG_CHANGE_REGISTRATION)NotificationHandle;

    if (!registration || registration->Tag != PH_TIMESTAMP_CONFIG_CHANGE_TAG)
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
    NTSTATUS status;
    NET_LUID interfaceLuid;
    NSI_COUNTED_WCHAR_PARAMETER description;

    interfaceLuid.Value = 0;
    RtlZeroMemory(&description, sizeof(NSI_COUNTED_WCHAR_PARAMETER));

    status = PhConvertInterfaceGuidToLuid(InterfaceGuid, &interfaceLuid);

    if (!NT_SUCCESS(status))
        return status;

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
        ULONG length = description.CountedString.Length / sizeof(WCHAR);
        if (length > MAXIMUM_FILENAME_LENGTH)
            length = MAXIMUM_FILENAME_LENGTH;
        description.CountedString.String[length] = UNICODE_NULL;
    }

    if (!NT_SUCCESS(status))
        return status;

    ULONG requiredLength = ((ULONG)PhCountStringZ(description.CountedString.String) + 1) * sizeof(WCHAR);
    NTSTATUS copyStatus = PhCopyStringZ(
        description.CountedString.String,
        SIZE_MAX,
        InterfaceDescription,
        *InterfaceDescriptionLength / sizeof(WCHAR),
        NULL
        );

    if (copyStatus == STATUS_BUFFER_TOO_SMALL)
        *InterfaceDescriptionLength = requiredLength;

    return copyStatus;
}

/**
 * Retrieves a framing layer virtual interface entry.
 *
 * \param InterfaceRow On input, the key fields of the row; on output, the complete row.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhGetFlVirtualInterface(
    _Inout_ PMIB_FL_VIRTUAL_INTERFACE_ROW InterfaceRow
    )
{
    NTSTATUS status;
    const NPI_MODULEID* moduleId;
    USHORT family;
    NSI_FL_VIRTUAL_INTERFACE_KEY key;
    NSI_FL_VIRTUAL_INTERFACE_RW readWrite;
    NSI_FL_VIRTUAL_INTERFACE_ROD readOnlyDynamic;
    NSI_FL_VIRTUAL_INTERFACE_ROS readOnlyStatic;

    family = InterfaceRow->Family;
    RtlZeroMemory(&key, sizeof(NSI_FL_VIRTUAL_INTERFACE_KEY));
    RtlZeroMemory(&readWrite, sizeof(NSI_FL_VIRTUAL_INTERFACE_RW));
    RtlZeroMemory(&readOnlyDynamic, sizeof(NSI_FL_VIRTUAL_INTERFACE_ROD));
    RtlZeroMemory(&readOnlyStatic, sizeof(NSI_FL_VIRTUAL_INTERFACE_ROS));

    if (family == AF_INET)
        moduleId = &NPI_MS_FL48_MODULEID;
    else if (family == AF_INET6)
        moduleId = &NPI_MS_FL68_MODULEID;
    else
        return STATUS_INVALID_PARAMETER;

    key.InterfaceLuid = InterfaceRow->IfLuid;
    key.VirtualInterfaceId = InterfaceRow->VirtualIfId;

    status = PhNsiGetAllParameters(
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

    if (NT_SUCCESS(status))
    {
        PhFillVirtualInterfaceRow(InterfaceRow, family, &key, &readWrite, &readOnlyDynamic, &readOnlyStatic);
        return 0;
    }

    return status;
}

/**
 * Retrieves the framing layer virtual interface table.
 *
 * \param Family The address family (AF_INET, AF_INET6 or AF_UNSPEC).
 * \param Table Receives the allocated table. Free it with PhFreeMibTable/FreeMibTable.
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 */
NTSTATUS PhGetFlVirtualInterfaceTable(
    _In_ ADDRESS_FAMILY Family,
    _Outptr_ PMIB_FL_VIRTUAL_INTERFACE_TABLE *Table
    )
{
    SIZE_T tableSize;
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
    NTSTATUS status;
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

        status = PhNsiAllocateAndGetTable(NsiActive, families[i].ModuleId, NsiFlObjectVirtualInterface,
            &families[i].Keys, sizeof(NSI_FL_VIRTUAL_INTERFACE_KEY),
            &families[i].Rw, sizeof(NSI_FL_VIRTUAL_INTERFACE_RW),
            &families[i].Rod, sizeof(NSI_FL_VIRTUAL_INTERFACE_ROD),
            &families[i].Ros, sizeof(NSI_FL_VIRTUAL_INTERFACE_ROS), &families[i].Count, 0);
        if (status == STATUS_NOT_FOUND || status == STATUS_NOT_SUPPORTED)
            continue;
        if (!NT_SUCCESS(status))
            goto Cleanup;

        status = RtlULongAdd(count, families[i].Count, &count);

        if (!NT_SUCCESS(status))
            goto Cleanup;
    }

    status = RtlSIZETMult(count, sizeof(MIB_FL_VIRTUAL_INTERFACE_ROW), &tableSize);

    if (!NT_SUCCESS(status))
        goto Cleanup;

    status = RtlSIZETAdd(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_TABLE, Table), tableSize, &tableSize);

    if (!NT_SUCCESS(status))
        goto Cleanup;

    if (!(table = PhAllocateZeroSafe(tableSize)))
    {
        status = STATUS_NO_MEMORY;
        goto Cleanup;
    }
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
    status = STATUS_SUCCESS;

Cleanup:
    for (i = 0; i < RTL_NUMBER_OF(families); i++)
    {
        if (families[i].Keys || families[i].Rw || families[i].Rod || families[i].Ros)
            PhNsiFreeTable(families[i].Keys, families[i].Rw, families[i].Rod, families[i].Ros);
    }
    return status;
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
    NTSTATUS status;
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

    RtlZeroMemory(&neighborCount, sizeof(ULONG));
    
    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    status = PhNsiAllocateAndGetTable(
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

    if (NT_SUCCESS(status))
    {
        tableResult = PhpNetioMibCheckTableBufferSize(
            FIELD_OFFSET(MIB_IPNETTABLE, table),
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
                        status = PhConvertInterfaceLuidToIndex(&keyElement->InterfaceLuid, &row->dwIndex);

                        if (!NT_SUCCESS(status))
                        {
                            PhNsiFreeTable(keyTable, readWriteTable, dynamicTable, NULL);
                            return status;
                        }

                        ULONG physAddrLen = dynamicElement->PhysicalAddressLength;
                        if (physAddrLen > RTL_NUMBER_OF(row->bPhysAddr))
                            physAddrLen = RTL_NUMBER_OF(row->bPhysAddr);
                        row->dwPhysAddrLen = physAddrLen;

                        PVOID physAddr = PTR_ADD_OFFSET(readWriteTable, neighborIndex * sizeof(MIB_IP_NEIGHBOR_RW_ROW));
                        memcpy(row->bPhysAddr, physAddr, physAddrLen);

                        row->dwAddr = keyElement->Address.S_un.S_addr;

                        NL_NEIGHBOR_STATE neighborState = dynamicElement->State;
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
    return status;
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

    RtlZeroMemory(BestRoute, sizeof(MIB_IPFORWARD_ROW2));
    RtlZeroMemory(BestSourceAddress, sizeof(SOCKADDR_INET));

    NET_LUID luid;

    RtlZeroMemory(&luid, sizeof(NET_LUID));

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
        RtlZeroMemory(&key, sizeof(NSI_IP_BEST_ROUTE_KEY4));
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
        RtlZeroMemory(&resultRow, sizeof(NSI_IP_BEST_ROUTE_ROD_V4));

        status = PhNsiGetAllParameters(
            NsiActive,
            &NPI_MS_IPV4_MODULEID,
            NsiIpObjectBestRoute,
            &key,
            sizeof(NSI_IP_BEST_ROUTE_KEY4),
            NULL,
            0,
            &resultRow,
            sizeof(NSI_IP_BEST_ROUTE_ROD_V4),
            NULL,
            0
            );

        if (NT_SUCCESS(status))
        {
            MIB_IP_ROUTE_DYNAMIC_ROW_V4 routeDynamic;
            MIB_IP_ROUTE_STATIC_ROW routeStatic;

            RtlZeroMemory(&routeDynamic, sizeof(MIB_IP_ROUTE_DYNAMIC_ROW_V4));
            RtlZeroMemory(&routeStatic, sizeof(MIB_IP_ROUTE_STATIC_ROW));

            routeDynamic.Age = resultRow.Age;
            routeStatic.Origin = resultRow.Origin;
            routeStatic.InterfaceIndex = resultRow.InterfaceIndex;

            PhNsiFillRouteEntry(
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

                RtlZeroMemory(&a6, sizeof(SOCKADDR_IN6));
                RtlZeroMemory(&v44, sizeof(SOCKADDR_IN6));

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
        RtlZeroMemory(&key, sizeof(NSI_IP_BEST_ROUTE_KEY6));
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
        RtlZeroMemory(&resultRow, sizeof(NSI_IP_BEST_ROUTE_ROD_V6));

        status = PhNsiGetAllParameters(
            NsiActive,
            &NPI_MS_IPV6_MODULEID,
            NsiIpObjectBestRoute,
            &key,
            sizeof(NSI_IP_BEST_ROUTE_KEY6),
            NULL,
            0,
            &resultRow,
            sizeof(NSI_IP_BEST_ROUTE_ROD_V6),
            NULL,
            0
            );

        if (NT_SUCCESS(status))
        {
            MIB_IP_ROUTE_DYNAMIC_ROW_V6 routeDynamic;
            MIB_IP_ROUTE_STATIC_ROW routeStatic;

            RtlZeroMemory(&routeDynamic, sizeof(MIB_IP_ROUTE_DYNAMIC_ROW_V6));
            RtlZeroMemory(&routeStatic, sizeof(MIB_IP_ROUTE_STATIC_ROW));

            routeDynamic.Age = resultRow.Age;
            routeStatic.Origin = resultRow.Origin;
            routeStatic.InterfaceIndex = resultRow.InterfaceIndex;
            PhNsiFillRouteEntry(AF_INET6, BestRoute, &resultRow.Key,
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
MIB_IPNET_TYPE PhpNeighborStateToMibState(
    _In_ NL_NEIGHBOR_STATE State
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
 * \param InterfaceType The interface type (IF_TYPE_*).
 * \return TRUE if the interface is an IPv4 interface, otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getipinterfaceentry
 */
BOOLEAN PhpIsIpv4Interface(
    _In_ NET_LUID *InterfaceLuid,
    _In_ NET_IFTYPE InterfaceType
    )
{
    if (InterfaceType == IF_TYPE_SOFTWARE_LOOPBACK)
        return FALSE;

    if (NT_SUCCESS(PhNsiGetAllParameters(
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
        )))
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
    NTSTATUS status = STATUS_SUCCESS;
    PMIB_NOTIFICATION_HANDLE handle = (PMIB_NOTIFICATION_HANDLE)NotificationHandle;

    if (!handle)
        return STATUS_INVALID_PARAMETER;

    NSI_IP_OBJECT_ID objectId = handle->ObjectId;
    RPC_BINDING_HANDLE rpcHandle = NotificationRpcHandle;

    if (handle->NotificationHandleV4)
    {
        status = PhNsiRpcDeregisterChangeNotification(rpcHandle, &NPI_MS_IPV4_MODULEID, objectId, handle->NotificationHandleV4);
    }
    if (handle->NotificationHandleV6)
    {
        status = PhNsiRpcDeregisterChangeNotification(rpcHandle, &NPI_MS_IPV6_MODULEID, objectId, handle->NotificationHandleV6);
    }
    if (handle->NotificationHandleTeredo)
    {
        status = PhNsiRpcDeregisterChangeNotification(rpcHandle, &NPI_MS_FL6T_MODULEID, objectId, handle->NotificationHandleTeredo);
    }
    if (handle->NotificationHandleNdis)
    {
        status = PhNsiRpcDeregisterChangeNotification(rpcHandle, &NPI_MS_NDIS_MODULEID, objectId, handle->NotificationHandleNdis);
    }
    if (handle->ConnectivityHintContext)
    {
        PPH_CONNECTIVITY_HINT_CONTEXT context = (PPH_CONNECTIVITY_HINT_CONTEXT)handle->ConnectivityHintContext;

        PhAcquireQueuedLockExclusive(&PhpConnectivityHintLock);
        RemoveEntryList(&context->ListEntry);
        PhReleaseQueuedLockExclusive(&PhpConnectivityHintLock);

        if (context->ThreadpoolWork)
        {
            TpWaitForWork(context->ThreadpoolWork, TRUE);
            TpReleaseWork(context->ThreadpoolWork);
        }
        PhFree(context);
    }

    PhFree(handle);
    return status;
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
    PIP_ADAPTER_ADDRESSES addresses = NULL;
    ULONG entryCount = 0;

    if (!SizePointer)
        return STATUS_INVALID_PARAMETER;

    status = PhpAllocateAndGetAdaptersAddresses(
        AF_INET,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
        &addresses,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        if (addresses)
            PhpFreeAdaptersAddresses(addresses);
        return status;
    }

    if (!addresses)
    {
        *SizePointer = 0;
        return STATUS_SUCCESS;
    }

    PIP_ADAPTER_ADDRESSES adapter = (PIP_ADAPTER_ADDRESSES)addresses;

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
        PhpFreeAdaptersAddresses(addresses);
        return STATUS_BUFFER_TOO_SMALL;
    }

    IpAddrTable->dwNumEntries = entryCount;

    ULONG index = 0;
    adapter = (PIP_ADAPTER_ADDRESSES)addresses;

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
                    
                    status = PhConvertLengthToIpv4Mask(unicast->OnLinkPrefixLength, &row->dwMask);

                    if (!NT_SUCCESS(status))
                    {
                        PhpFreeAdaptersAddresses(addresses);
                        return status;
                    }

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

    PhpFreeAdaptersAddresses(addresses);
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

    table = PhAllocateSafe(count * sizeof(IP_INTERFACE_NAME_INFO));
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
            NET_IFTYPE interfaceType = publicEntry->ifType;
            NET_LUID* luid = &luidTable[i];

            if (PhpIsIpv4Interface(luid, interfaceType))
            {
                PIP_INTERFACE_NAME_INFO dest = &table[outputCount];
                outputCount++;

                dest->Index = publicEntry->ifIndex;
                dest->MediaType = interfaceType;
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

    table = (PMIB_IPADDRTABLE)PhAllocateSafe(size);

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
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcptable2
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-gettcp6table2
 */
static NTSTATUS PhpAllocateAndGetBoundTcpTable(
    _In_ ADDRESS_FAMILY Family,
    _Out_ PVOID* Table
    )
{
    NTSTATUS status;
    NSI_TCP_KEY_ROW* keyTable = NULL;
    NSI_TCP_STATUS_ROW* statusTable = NULL;
    NSI_TCP_OWNER_ROW* ownerTable = NULL;
    ULONG count = 0;
    ULONG numberOfEntries = 0;
    SIZE_T tableSize;
    PVOID table;

    *Table = NULL;

    status = PhNsiAllocateAndGetTable(
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

    if (!NT_SUCCESS(status))
        return status;

    if (Family == AF_INET)
        tableSize = FIELD_OFFSET(MIB_TCPTABLE2, table) + sizeof(MIB_TCPROW2) * (SIZE_T)max(count, 1);
    else
        tableSize = FIELD_OFFSET(MIB_TCP6TABLE2, table) + sizeof(MIB_TCP6ROW2) * (SIZE_T)max(count, 1);

    if (!(table = PhAllocateZeroSafe(tableSize)))
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
            RtlZeroMemory(row, sizeof(MIB_TCPROW2));
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
            RtlZeroMemory(row, sizeof(MIB_TCP6ROW2));
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
    // N.B. This is the only write of dwNumEntries and the heap block is not zeroed.

    if (Family == AF_INET)
    {
        ((PMIB_TCPTABLE2)table)->dwNumEntries = numberOfEntries;

        //if (numberOfEntries > 1)
        //{
        //    qsort_s(((PMIB_TCPTABLE2)table)->table, numberOfEntries, sizeof(MIB_TCPROW2), PhpCompareMibTcpRow2, NULL);
        //}
    }
    else
    {
        ((PMIB_TCP6TABLE2)table)->dwNumEntries = numberOfEntries;

        //if (numberOfEntries > 1)
        //{
        //    qsort_s(((PMIB_TCP6TABLE2)table)->table, numberOfEntries, sizeof(MIB_TCP6ROW2), PhpCompareMibTcp6Row2, NULL);
        //}
    }

    *Table = table;
    return STATUS_SUCCESS;
}

/**
 * Retrieves the IPv4 TCP endpoints that are bound but not connected or listening.
 *
 * \param BoundTcpTable Receives a MIB_TCPTABLE2
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \remarks Reimplements iphlpapi!InternalGetBoundTcpEndpointTable.
 */
NTSTATUS WINAPI PhInternalGetBoundTcpEndpointTable(
    _Outptr_ PMIB_TCPTABLE2* BoundTcpTable
    )
{
    NTSTATUS status;
    PVOID table;

    status = PhpAllocateAndGetBoundTcpTable(AF_INET, &table);

    if (NT_SUCCESS(status))
        *BoundTcpTable = table;

    return status;
}

/**
 * Retrieves the IPv6 TCP endpoints that are bound but not connected or listening.
 *
 * \param BoundTcpTable Receives a MIB_TCP6TABLE2
 * \return An NTSTATUS value; use NT_SUCCESS() to check for success.
 * \remarks Reimplements iphlpapi!InternalGetBoundTcp6EndpointTable.
 */
NTSTATUS WINAPI PhInternalGetBoundTcp6EndpointTable(
    _Outptr_ PMIB_TCP6TABLE2* BoundTcpTable
    )
{
    NTSTATUS status;
    PVOID table;

    status = PhpAllocateAndGetBoundTcpTable(AF_INET6, &table);

    if (NT_SUCCESS(status))
        *BoundTcpTable = table;

    return status;
}

//
// NotifyIpInterfaceChange
//

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
    _In_opt_ PVOID CallerContext,
    _In_opt_ PVOID ObjectKey,
    _In_opt_ PVOID ObjectParameter,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    )
{
    PPH_IP_INTERFACE_CHANGE_CONTEXT Context = CallerContext;
    PNSI_CHANGE_KEY ChangeKey = ObjectKey;
    PMIB_IPINTERFACE_ROW row = NULL;

    // The initial notification has no key and reports a NULL row.
    if (ChangeKey->KeyLength)
    {
        row = &Context->Row;
        RtlZeroMemory(row, sizeof(MIB_IPINTERFACE_ROW));
        row->Family = Context->Callback.Family;
        row->InterfaceLuid = *(PNET_LUID)ChangeKey->Key;

        if (!NT_SUCCESS(PhpNsiGetParameter(
            NsiActive,
            &NPI_MS_NDIS_MODULEID,
            NsiNdisObjectInterface,
            &row->InterfaceLuid,
            sizeof(NET_LUID),
            NsiParameterTypeReadOnlyStatic,
            &row->InterfaceIndex,
            sizeof(NET_IFINDEX),
            0
            )))
        {
            row->InterfaceIndex = 0;
        }
    }

    Context->Callback.InterfaceCallback(
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
    PPH_IP_INTERFACE_CHANGE_CONTEXT ipv4Context;
    PPH_IP_INTERFACE_CHANGE_CONTEXT ipv6Context;
    RPC_BINDING_HANDLE rpcHandle;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS result4 = STATUS_SUCCESS;

    if ((Family != AF_UNSPEC && Family != AF_INET && Family != AF_INET6) || !Callback || !NotificationHandle)
        return STATUS_INVALID_PARAMETER;

    // PhCancelMibChangeNotify2 deregisters through the shared header and frees with PhFree.
    registration = PhAllocateZeroSafe(sizeof(NETIO_IP_INTERFACE_CHANGE_REGISTRATION));

    if (!registration)
        return STATUS_NO_MEMORY;

    registration->Header.ObjectId = NsiIpObjectIpInterface;
    registration->Header.Ipv4CallbackContext = &registration->Ipv4Callback;
    registration->Header.Ipv6CallbackContext = &registration->Ipv6Callback;

    ipv4Context = (PPH_IP_INTERFACE_CHANGE_CONTEXT)&registration->Ipv4Callback;
    ipv4Context->Callback.Family = AF_INET;
    ipv4Context->Callback.InterfaceCallback = Callback;
    ipv4Context->Callback.CallerContext = CallerContext;

    ipv6Context = (PPH_IP_INTERFACE_CHANGE_CONTEXT)&registration->Ipv6Callback;
    ipv6Context->Callback.Family = AF_INET6;
    ipv6Context->Callback.InterfaceCallback = Callback;
    ipv6Context->Callback.CallerContext = CallerContext;

    status = PhNsiNotificationConnectToServer(&rpcHandle);

    if (!NT_SUCCESS(status))
    {
        PhFree(registration);
        *NotificationHandle = NULL;
        return status;
    }

    if (Family != AF_INET6)
    {
        status = result4 = PhNsiRpcRegisterChangeNotification(
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
        status = PhNsiRpcRegisterChangeNotification(
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
            status = (!NT_SUCCESS(result4) && !NT_SUCCESS(status)) ? result4 : STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status))
    {
        PhCancelMibChangeNotify2(registration);
        *NotificationHandle = NULL;
        return status;
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
    IN_ADDR ipv4Address;
    IN6_ADDR ipv6Address;
    ULONG scopeId = 0;
    USHORT port = 0;
    PCWSTR separator;
    UCHAR prefix;

    RtlZeroMemory(&ipv4Address, sizeof(IN_ADDR));
    RtlZeroMemory(&ipv6Address, sizeof(IN6_ADDR));

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
            RtlZeroMemory(&AddressInfo->Ipv4Address, sizeof(SOCKADDR_IN));
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
        WCHAR addressString[PH_IPV4_NETWORK_ADDRESS_BUFFER_LENGTH];

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
                    RtlZeroMemory(&AddressInfo->Ipv4Address, sizeof(SOCKADDR_IN));
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
        WCHAR addressString[PH_IPV6_NETWORK_ADDRESS_BUFFER_LENGTH];

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

            for (; hostBitsClear && index < RTL_NUMBER_OF(ipv6Address.u.Byte); index++)
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
            nameLength = PhCountStringZ(NetworkString);
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
            CHAR utf8Name[PH_DNS_UTF8_NAME_BUFFER_LENGTH];
            ULONG utf8Length;
            NTSTATUS status;

            status = RtlUnicodeToUTF8N(
                utf8Name,
                RTL_NUMBER_OF(utf8Name) - 2,
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

            if (!NT_SUCCESS(RtlStringCchCopyNW(
                AddressInfo->NamedAddress.Address,
                RTL_NUMBER_OF(AddressInfo->NamedAddress.Address),
                NetworkString,
                nameLength
                )))
            {
                return STATUS_BUFFER_TOO_SMALL;
            }

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
