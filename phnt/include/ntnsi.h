/*
 * NSI definitions
 *
 * This file is part of System Informer.
 */

#ifndef _NTNSI_H
#define _NTNSI_H

#ifndef __WINDOT11_H__
#define __WINDOT11_H__
#endif

//
// Newer Windows SDK headers (netiodef.h pulls in mstcpip.h) use ASSERT() inside
// inline helpers. When phnt's RtlAssert-based ASSERT macro is active, that
// expands to RtlAssert before its prototype is visible. Neutralize ASSERT for
// the duration of these includes, then restore it afterwards. The guard must
// wrap netiodef.h as well, since it now includes mstcpip.h transitively.
//
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

#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2def.h>
#include <windns.h>
#include <nldef.h>
#include <netiodef.h>
#include <iphlpapi.h>
#include <mstcpip.h>
#include <tcpestats.h>

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

#ifndef _NETIODEF_
typedef GUID NPIID, *PNPIID;

/**
 * The NPI_MODULEID_TYPE values say how an NPI_MODULEID identifies its module: by GUID (all NSI providers) or by
 * interface LUID.
 */
typedef enum _NPI_MODULEID_TYPE
{
    MIT_GUID = 0x1,
    MIT_IF_LUID = 0x2
} NPI_MODULEID_TYPE, *PNPI_MODULEID_TYPE;

/**
 * The NPI_MODULEID structure identifies a Network Programming Interface (NPI) module, such as NPI_MS_TCP_MODULEID or
 * NPI_MS_NDIS_MODULEID. Every NSI routine takes a module ID to select the provider (tcpip.sys, ndis.sys, ...) that owns
 * the object being queried or set.
 *
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/netioddk/ns-netioddk-_npi_moduleid
 */
typedef struct _NPI_MODULEID
{
    USHORT Length;
    NPI_MODULEID_TYPE Type;
    union
    {
        GUID Guid;
        LUID IfLuid;
    };
} NPI_MODULEID, *PNPI_MODULEID;
#endif

#define NPI_MS_IPV4_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A00, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_IPV6_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A01, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_UDP_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A02, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_TCP_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A03, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_NMR_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A04, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_TUN_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A05, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL4L_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A0E, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL6L_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A0F, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_EQOS_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A1D, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_WANARPV4_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A09, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL48_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A0A, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL68_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A0B, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_WANARPV6_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A0C, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL_WANARP_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A0D, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_TCPIP_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A35, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_NDIS_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A11, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL4T_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A1B, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL6T_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A1C, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FLRDMA_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A1E, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_RAW_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A07, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_WSK_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A0D, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_PACER_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A19, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_RSS_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A22, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_EQOSPROXY_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A2A, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FSE_CLIENT_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A32, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_IPSEC_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xEB004A34, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_IPS_LB_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, .Guid = { 0xD49FE21E, 0x1F02, 0x400F, { 0xA9, 0xCB, 0xD9, 0xC8, 0x0A, 0x6E, 0x2E, 0x69 } } }
#define NPI_FRAMING_LAYER_ID_INIT { 0x2227E801, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_NETWORK_LAYER_ID_INIT { 0x2227E802, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_WSK_INTERFACE_ID_INIT { 0x2227E803, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_TRANSPORT_LAYER_ID_INIT { 0x2227E804, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_NSI_MODULE_ID_INIT { 0x2227E806, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_NL_PNP_EVENT_ACCESS_ID_INIT { 0x2227E808, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_WSK_RDMA_INTERFACE_ID_INIT { 0x2227E80A, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_QOS_INTERFACE_ID_INIT { 0x2227E80B, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_RSS_CONTROL_INTERFACE_ID_INIT { 0x2227E80D, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_RSS_HASH_INTERFACE_ID_INIT { 0x2227E80F, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_UQOS_INTERFACE_ID_INIT { 0x2227E810, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_IP_SWITCHING_ID_INIT { 0x2227E812, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_HTTP_KE_INTERFACE_ID_INIT { 0x2227E813, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_PKTMON_INTERFACE_ID_INIT { 0x2227E815, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_FSE_LOCAL_SERVER_INTERFACE_ID_INIT { 0x2227E816, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }
#define NPI_FL_SWITCHING_ID_INIT { 0x2227E818, 0x8D8B, 0x11D4, { 0xAB, 0xAD, 0x00, 0x90, 0x27, 0x71, 0x9E, 0x09 } }

#define NSI_IP_OBJECT_BANDWIDTH_RW_SIZE 0x30
#define NSI_IP_OBJECT_BANDWIDTH_ROS_SIZE 0x18
#define NSI_IP_OBJECT_ADDRESS_MANAGER_RW_SIZE 0x30
#define NSI_IP_OBJECT_ADDRESS_MANAGER_ROS_SIZE 0x18

#define NSI_DEVICE_NAME L"\\Device\\Nsi"

#define IOCTL_NSI_GET_PARAMETER \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x1, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x120007; in: NSI_GET_PARAMETER_EX_REQUEST; out: NSI_GET_PARAMETER_EX_REQUEST

#define IOCTL_NSI_SET_PARAMETER \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x2, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12000B; in: NSI_SET_PARAMETER_EX_REQUEST; out: NSI_SET_PARAMETER_EX_REQUEST

#define IOCTL_NSI_GET_ALL_PARAMETERS_EX \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x3, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12000F; in: NSI_GET_ALL_PARAMETERS_EX_REQUEST; out: NSI_GET_ALL_PARAMETERS_EX_REQUEST

#define IOCTL_NSI_SET_ALL_PARAMETERS \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x4, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x120013; in: NSI_SET_ALL_PARAMETERS_EX_REQUEST; out: NSI_SET_ALL_PARAMETERS_EX_REQUEST

#define IOCTL_NSI_GET_ALL_PARAMETERS \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x6, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12001B; in: NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX; out: NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX

#define IOCTL_NSI_REGISTER_CHANGE_NOTIFICATION_EX \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x7, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12001F; in: NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST; out: NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST

#define IOCTL_NSI_DEREGISTER_CHANGE_NOTIFICATION_EX \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x8, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x120023; in: NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST; out: NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST

#define IOCTL_NSI_GET_OBJECT_SECURITY \
    CTL_CODE(FILE_DEVICE_NETWORK, 0xD, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x120037; in: PVOID[6]; out: PVOID[6]

#define IOCTL_NSI_SET_OBJECT_SECURITY \
    CTL_CODE(FILE_DEVICE_NETWORK, 0xE, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12003B; in: PVOID[6]; out: PVOID[6]

#define IOCTL_NSI_REQUEST_CHANGE_NOTIFICATION_EX \
    CTL_CODE(FILE_DEVICE_NETWORK, 0xF, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12003F; in: NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST; out: NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST

#define IOCTL_NSI_CANCEL_CHANGE_NOTIFICATION \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x10, METHOD_BUFFERED, FILE_ANY_ACCESS) // 0x120040; in: HANDLE; out: none

#define IOCTL_NSI_ENUMERATE_OBJECTS_ALL_PERSISTENT_PARAMETERS_WITH_MASK \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x11, METHOD_NEITHER, FILE_ANY_ACCESS)   // 0x120047; in: PHP_NSI_PERSISTENT_ENUMERATE_REQUEST; out: PHP_NSI_PERSISTENT_ENUMERATE_REQUEST

#define IOCTL_NSI_GET_ALL_PERSISTENT_PARAMETERS_WITH_MASK \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x12, METHOD_NEITHER, FILE_ANY_ACCESS)   // 0x12004B; in: PHP_NSI_PERSISTENT_GET_REQUEST; out: PHP_NSI_PERSISTENT_GET_REQUEST

#define IOCTL_NSI_SET_ALL_PERSISTENT_PARAMETERS_WITH_MASK \
    CTL_CODE(FILE_DEVICE_NETWORK, 0x13, METHOD_NEITHER, FILE_ANY_ACCESS)   // 0x12004F; in: PHP_NSI_PERSISTENT_SET_REQUEST; out: PHP_NSI_PERSISTENT_SET_REQUEST

#define NSI_ESTATS_SND_CONG_RW_VERSION 0
#define TCP_ESTATS_DATA_ROD_VERSION 0
#define TCP_ESTATS_DATA_RW_VERSION 0
#define TCP_ESTATS_SND_CONG_ROD_VERSION 0
#define TCP_ESTATS_SND_CONG_ROS_VERSION 0
#define TCP_ESTATS_SYN_OPTS_ROS_VERSION 0

EXTERN_C CONST NPI_MODULEID NPI_MS_IPV4_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_IPV6_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_UDP_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_TCP_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_NMR_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_TUN_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_FL4L_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_FL6L_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_EQOS_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_WANARPV4_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_FL48_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_FL68_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_WANARPV6_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_FL_WANARP_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_TCPIP_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_NDIS_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_FL4T_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_FL6T_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_FLRDMA_MODULEID;

EXTERN_C CONST NPIID NPI_FL_SWITCHING_ID;
EXTERN_C CONST NPIID NPI_FRAMING_LAYER_ID;
EXTERN_C CONST NPIID NPI_FSE_LOCAL_SERVER_INTERFACE_ID;
EXTERN_C CONST NPIID NPI_HTTP_KE_INTERFACE_ID;
EXTERN_C CONST NPIID NPI_IP_SWITCHING_ID;
EXTERN_C CONST NPI_MODULEID NPI_MS_EQOSPROXY_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_FSE_CLIENT_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_IPS_LB_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_IPSEC_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_PACER_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_RAW_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_RSS_MODULEID;
EXTERN_C CONST NPI_MODULEID NPI_MS_WSK_MODULEID;
EXTERN_C CONST NPIID NPI_NETWORK_LAYER_ID;
EXTERN_C CONST NPIID NPI_NL_PNP_EVENT_ACCESS_ID;
EXTERN_C CONST NPIID NPI_NSI_MODULE_ID;
EXTERN_C CONST NPIID NPI_PKTMON_INTERFACE_ID;
EXTERN_C CONST NPIID NPI_QOS_INTERFACE_ID;
EXTERN_C CONST NPIID NPI_RSS_CONTROL_INTERFACE_ID;
EXTERN_C CONST NPIID NPI_RSS_HASH_INTERFACE_ID;
EXTERN_C CONST NPIID NPI_TRANSPORT_LAYER_ID;
EXTERN_C CONST NPIID NPI_UQOS_INTERFACE_ID;
EXTERN_C CONST NPIID NPI_WSK_INTERFACE_ID;
EXTERN_C CONST NPIID NPI_WSK_RDMA_INTERFACE_ID;

/**
 * The NSI_STORE values select which copy of an object's parameters an NSI call works on. NsiActive is the running
 * configuration, NsiPersistent the copy stored in the registry and reapplied at boot, NsiBoth applies a set to both,
 * and NsiCurrent reads the active values merged with persistent defaults. NsiBootFirmwareTable is the configuration
 * supplied by the firmware (iSCSI/network boot tables).
 * \remarks NsiEnumerateObjectsAllParametersEx rejects NsiBoth.
 */
typedef enum _NSI_STORE
{
    NsiPersistent,
    NsiActive,
    NsiBoth,
    NsiCurrent,
    NsiBootFirmwareTable
} NSI_STORE;

/**
 * The NSI_SET_ACTION values select what NsiSetParameter(Ex) and NsiSetAllParameters(Ex) do with the object named by
 * the key: change it, create it, create-or-change it, or delete it. The *WithReference variants keep a reference count so
 * that several callers can create and delete the same object (for example port reservations); Reset and Clear restore
 * or clear the stored parameters.
 * \remarks The value is the SetAction field of NSI_EX_REQUEST_COMMON.
 */
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

/**
 * The NSI_GET_ACTION enumeration selects which object instance a get or enumerate request returns. It is carried in the
 * request header (NSI_EX_REQUEST_COMMON.GetAction) and passed unchanged to the information provider.
 * \remarks netio.sys accepts 0-2 for NsiGetParameter(Ex)/NsiGetAllParameters(Ex), only NsiGetFirst and NsiGetNext
 *          for NsiEnumerateObjectsAllParameters(Ex), and only NsiGetFirst for persistent-store enumeration; anything else fails with
 *          STATUS_INVALID_PARAMETER (STATUS_NOT_IMPLEMENTED for the persistent store).
 */
typedef enum _NSI_GET_ACTION
{
    NsiGetExact = 0,    // The instance identified by the key (used by NsiGetParameter/NsiGetAllParameters).
    NsiGetFirst = 1,    // Start at the first instance (NsiAllocateAndGetTable and IP Helper always pass this).
    NsiGetNext = 2      // Continue with the instance that follows the key in the first KeyTable entry.
} NSI_GET_ACTION;

typedef ULONG NSI_OBJECT_ID, *PNSI_OBJECT_ID;

/**
 * The NSI_INTERFACE_INFORMATION_CLASS values select a conversion between interface identifiers (NET_LUID, interface
 * GUID and interface index). netsup.c uses NsiInterfaceIndexToLuid.
 */
typedef enum _NSI_INTERFACE_INFORMATION_CLASS
{
    NsiInterfaceLuidToGuid = 0,
    NsiInterfaceGuidToLuid = 1,
    NsiInterfaceIndexToLuid = 2,
} NSI_INTERFACE_INFORMATION_CLASS;

/**
 * The NSI_ICMP_OBJECT_ID values are the NSI object IDs of the ICMP echo provider (NPI_MS_IPV4_MODULEID or
 * NPI_MS_IPV6_MODULEID). NsiIcmpObjectEchoRequest is the object an ICMP handle registers a change notification on to
 * receive echo completions (ICMP_NOTIFICATION_MESSAGE).
 */
typedef enum _NSI_ICMP_OBJECT_ID
{
    NsiIcmpObjectEchoRequest = 4,
} NSI_ICMP_OBJECT_ID;

/**
 * The NSI_ICMP_SET_ACTION_OBJECT_ID values are the object IDs passed to NsiSetAllParameters to submit an echo request
 * (the key is ICMP_REQUEST_RW, followed by the ICMP_REQUEST_V4/V6 read/write block) and to acknowledge its completion.
 * They are the same values as ICMP_NSI_SET_ALL_PARAMETERS_OBJECT_ID.
 */
typedef enum _NSI_ICMP_SET_ACTION_OBJECT_ID
{
    NsiIcmpSetActionObjectSubmitEchoRequest = 6,
    NsiIcmpSetActionObjectCompleteEchoRequest = 7
} NSI_ICMP_SET_ACTION_OBJECT_ID;

/**
 * The NSI_FL_OBJECT_ID values are NSI object IDs of the framing-layer modules (NPI_MS_FL48_MODULEID and
 * NPI_MS_FL68_MODULEID). NsiFlObjectVirtualInterface is the virtual interface object (NSI_FL_VIRTUAL_INTERFACE_KEY).
 */
typedef enum _NSI_FL_OBJECT_ID
{
    NsiFlObjectVirtualInterface = 11
} NSI_FL_OBJECT_ID;

/**
 * The NSI_IP_BEST_ROUTE_PARAMETER_ID values are byte offsets passed to NsiGetParameter to read a single field of the
 * best-route read-only dynamic block (NsiIpObjectBestRoute). Both select the interface index of the best route
 * (NSI_IP_BEST_ROUTE_ROD_V4/V6.InterfaceIndex); GetBestInterfaceEx uses them.
 */
typedef enum _NSI_IP_BEST_ROUTE_PARAMETER_ID
{
    NsiIpv4BestRouteType1BestIfIndex = 0x058,
    NsiIpv6BestRouteType1BestIfIndex = 0x07C
} NSI_IP_BEST_ROUTE_PARAMETER_ID;

/**
 * The NSI_IP_INTERFACE_PARAMETER_ID values are byte offsets passed to NsiGetParameter to read a single field of an IP
 * interface (NsiIpObjectIpInterface). NsiIpInterfaceType0Metric is the interface metric in the read/write block
 * (parameter type 0).
 */
typedef enum _NSI_IP_INTERFACE_PARAMETER_ID
{
    NsiIpInterfaceType0Metric = 0x01C
} NSI_IP_INTERFACE_PARAMETER_ID;

/**
 * The NSI_IP_OBJECT_ID values are the NSI object IDs of the IPv4 and IPv6 network-layer modules
 * (NPI_MS_IPV4_MODULEID and NPI_MS_IPV6_MODULEID). Each object has its own key and parameter blocks, for example
 * NsiIpObjectRoute (MIB_IP_ROUTE_*), NsiIpObjectNeighbor (MIB_IP_NEIGHBOR_*) and NsiIpObjectPath (NSI_IP_PATH_*).
 */
typedef enum _NSI_IP_OBJECT_ID
{
    NsiIpObjectBestRoute = 0,
    NsiIpObjectCompartment = 2,
    NsiIpObjectIcmp = 3,
    NsiIpObjectGlobal = 6,                 // IP global settings and statistics (tcpip!IpGetAllGlobalParameters)
    NsiIpObjectIpInterface = 7,
    NsiIpObjectAnycastAddress = 8,
    NsiIpObjectMulticastAddress = 9,
    NsiIpObjectUnicastAddress = 10,
    NsiIpObjectNeighbor = 11,
    NsiIpObjectPath = 12,
    NsiIpObjectPrefixPolicy = 14,
    NsiIpObjectRoute = 16,
    NsiIpObjectAddressManager = 28,
    NsiIpObjectBandwidth = 30
} NSI_IP_OBJECT_ID;

/**
 * The NSI_NDIS_COMPARTMENT_GUID_PARAMETER_ID values are byte offsets for NsiNdisObjectCompartmentGuid (key: compartment
 * GUID). NsiNdisCompartmentGuidType2Id reads the compartment ID from the read-only static block; ConvertCompartmentGuidToId
 * uses it.
 */
typedef enum _NSI_NDIS_COMPARTMENT_GUID_PARAMETER_ID
{
    NsiNdisCompartmentGuidType2Id = 0x000
} NSI_NDIS_COMPARTMENT_GUID_PARAMETER_ID;

/**
 * The NSI_NDIS_COMPARTMENT_PARAMETER_ID values are byte offsets into NDIS_NSI_COMPARTMENT_RW (NsiNdisObjectCompartment,
 * read/write block). NsiNdisCompartmentType0NamespaceGuid is NDIS_NSI_COMPARTMENT_RW.NamespaceGuid.
 */
typedef enum _NSI_NDIS_COMPARTMENT_PARAMETER_ID
{
    NsiNdisCompartmentType0NamespaceGuid = 0x654
} NSI_NDIS_COMPARTMENT_PARAMETER_ID;

/**
 * The NSI_NDIS_CURRENT_THREAD_COMPARTMENT_PARAMETER_ID values are byte offsets for NsiNdisObjectCurrentThreadCompartment
 * (no key). NsiNdisCurrentThreadCompartmentType0Id reads the calling thread's compartment ID
 * (GetCurrentThreadCompartmentId).
 */
typedef enum _NSI_NDIS_CURRENT_THREAD_COMPARTMENT_PARAMETER_ID
{
    NsiNdisCurrentThreadCompartmentType0Id = 0x000
} NSI_NDIS_CURRENT_THREAD_COMPARTMENT_PARAMETER_ID;

/**
 * The NSI_NDIS_INTERFACE_INDEX_PARAMETER_ID values are byte offsets for NsiNdisObjectInterfaceIndex (key: NET_IFINDEX).
 * NsiNdisInterfaceIndexToLuid reads the interface's NET_LUID from the read-only static block (ConvertInterfaceIndexToLuid).
 */
typedef enum _NSI_NDIS_INTERFACE_INDEX_PARAMETER_ID
{
    NsiNdisInterfaceIndexToLuid = 0x000
} NSI_NDIS_INTERFACE_INDEX_PARAMETER_ID;

/**
 * The NSI_NDIS_INTERFACE_RW_PARAMETER_ID values are byte offsets into NDIS_NSI_INTERFACE_INFORMATION_RW
 * (NsiNdisObjectInterface, read/write block). NsiNdisIfType0Alias is ifAlias (ConvertInterfaceLuidToAlias).
 */
typedef enum _NSI_NDIS_INTERFACE_RW_PARAMETER_ID
{
    NsiNdisIfType0Alias = 0x014
} NSI_NDIS_INTERFACE_RW_PARAMETER_ID;

/**
 * The NSI_NDIS_INTERFACE_DYNAMIC_PARAMETER_ID values are byte offsets into NDIS_NSI_INTERFACE_INFORMATION_ROD
 * (NsiNdisObjectInterface, read-only dynamic block). NsiNdisIfType1CompartmentId is the interface's compartment ID.
 */
typedef enum _NSI_NDIS_INTERFACE_DYNAMIC_PARAMETER_ID
{
    NsiNdisIfType1CompartmentId = 0x0D0
} NSI_NDIS_INTERFACE_DYNAMIC_PARAMETER_ID;

/**
 * The NSI_NDIS_INTERFACE_STATIC_PARAMETER_ID values are byte offsets into NDIS_NSI_INTERFACE_INFORMATION_ROS
 * (NsiNdisObjectInterface, read-only static block): ifIndex, ifDescr and InterfaceGuid. IP Helper's
 * ConvertInterfaceLuidToIndex/ToGuid and the description lookup use them.
 */
typedef enum _NSI_NDIS_INTERFACE_STATIC_PARAMETER_ID
{
    NsiNdisInterfaceLuidToIndex = 0x000,
    NsiNdisInterfaceLuidToDescription = 0x004,
    NsiNdisInterfaceLuidToGuid = 0x218
} NSI_NDIS_INTERFACE_STATIC_PARAMETER_ID;

/**
 * The NSI_NDIS_JOB_COMPARTMENT_PARAMETER_ID values are byte offsets for NsiNdisObjectJobCompartment.
 * NsiNdisJobCompartmentType0Id reads the compartment ID assigned to a job object.
 */
typedef enum _NSI_NDIS_JOB_COMPARTMENT_PARAMETER_ID
{
    NsiNdisJobCompartmentType0Id = 0x000
} NSI_NDIS_JOB_COMPARTMENT_PARAMETER_ID;

/**
 * The NSI_NDIS_OBJECT_ID values are the NSI object IDs of the NDIS module (NPI_MS_NDIS_MODULEID): interfaces (with and
 * without statistics), index-to-LUID mapping, networks, compartments (by ID, by GUID, and the current thread's, session's
 * and job's compartment), the interface stack and isolation settings.
 */
typedef enum _NSI_NDIS_OBJECT_ID
{
    NsiNdisObjectInterface = 0,
    NsiNdisObjectInterfaceWithoutStatistics = 1,
    NsiNdisObjectInterfaceIndex = 2,
    NsiNdisObjectNetwork = 6,
    NsiNdisObjectCompartment = 7,
    NsiNdisObjectCurrentThreadCompartment = 8,
    NsiNdisObjectSessionCompartment = 9,
    NsiNdisObjectCompartmentGuid = 11,
    NsiNdisObjectInterfaceRaw = 12,
    NsiNdisObjectIfStack = 14,
    NsiNdisObjectPersistentIsolation = 15,
    NsiNdisObjectJobCompartment = 16
} NSI_NDIS_OBJECT_ID;

/**
 * The NSI_NDIS_SESSION_COMPARTMENT_PARAMETER_ID values are byte offsets for NsiNdisObjectSessionCompartment.
 * NsiNdisSessionCompartmentType0Id reads the compartment ID assigned to a logon session.
 */
typedef enum _NSI_NDIS_SESSION_COMPARTMENT_PARAMETER_ID
{
    NsiNdisSessionCompartmentType0Id = 0x000
} NSI_NDIS_SESSION_COMPARTMENT_PARAMETER_ID;

/**
 * The NSI_TABLE_ID values are the object IDs netsup.c passes to NsiAllocateAndGetTable to enumerate whole tables. The
 * module decides the meaning, so several names share a value (for example NsiTableIdUdp and NsiTableIdTcp are used with
 * NPI_MS_UDP_MODULEID and NPI_MS_TCP_MODULEID respectively).
 */
typedef enum _NSI_TABLE_ID
{
    NsiTableIdUdp = 1,
    NsiTableIdTcp = 3,
    NsiTableIdCompartment = 7,
    NsiTableIdIpAddress = 8,
    NsiTableIdAnycastAddress = NsiTableIdIpAddress,
    NsiTableIdUnicastAddress = 10,
    NsiTableIdNeighbor = 11,
    NsiTableIdPrefixPolicy = 14,
    NsiTableIdIfStack = NsiTableIdPrefixPolicy,
    NsiTableIdRoute = 16,
    NsiTableIdTcp6 = 35
} NSI_TABLE_ID;

/**
 * The NSI_TCP_APPLIED_SETTING values identify the TCP setting template applied to a connection
 * (Get-NetTCPSetting: Internet, Datacenter, Compat and their custom variants). Reported in
 * NSI_TCP_STATUS_ROW.AppliedSetting.
 */
typedef enum _NSI_TCP_APPLIED_SETTING
{
    NsiTcpAppliedSettingInternet = 0,
    NsiTcpAppliedSettingDatacenter = 1,
    NsiTcpAppliedSettingCompat = 2,
    NsiTcpAppliedSettingInternetCustom = 3,
    NsiTcpAppliedSettingDatacenterCustom = 4,
} NSI_TCP_APPLIED_SETTING;

/**
 * The NSI_TCP_ENUMERATE_CONNECTION_TYPE values select which TCP endpoints a tcpip.sys connection enumeration returns.
 */
typedef enum _NSI_TCP_ENUMERATE_CONNECTION_TYPE
{
    NsiTcpEnumerateConnectionTypeBasic = 0,             // IPv6 and IPv4 connection TCBs (synchronizing, established, closing and TIME_WAIT); no listeners.
    NsiTcpEnumerateConnectionTypeListening = 1,         // Listening endpoints only.
    NsiTcpEnumerateConnectionTypeAll = 2,               // Connection TCBs followed by listeners (the GetTcpTable view).
    NsiTcpEnumerateConnectionTypeBoundConnections = 3   // Bound endpoints that are neither connected nor listening.
} NSI_TCP_ENUMERATE_CONNECTION_TYPE;

/**
 * The MIB_TCP_ESTATS_TYPE values are the NSI object IDs of the per-connection TCP extended statistics (ESTATS) objects
 * of NPI_MS_TCP_MODULEID, keyed by NSI_ESTATS_TCP_KEY. netsup.c maps each TCP_ESTATS_TYPE value
 * (GetPerTcpConnectionEStats/SetPerTcpConnectionEStats) to one of them.
 */
typedef enum _MIB_TCP_ESTATS_TYPE
{
    MibTcpEstatsTypeSynOpts = 6,
    MibTcpEstatsTypeData = 7,
    MibTcpEstatsTypeSndCong = 8,
    MibTcpEstatsTypePath = 9,
    MibTcpEstatsTypeSendBuff = 10,
    MibTcpEstatsTypeRec = 11,
    MibTcpEstatsTypeObsRec = 12,
    MibTcpEstatsTypeBandwidth = 14,
    MibTcpEstatsTypeFineRtt = 15
} MIB_TCP_ESTATS_TYPE;

/**
 * The NSI_TCP_OBJECT_ID values are the NSI object IDs of the TCP module (NPI_MS_TCP_MODULEID): global statistics and
 * settings, connections, and port reservations and exclusions.
 */
typedef enum _NSI_TCP_OBJECT_ID
{
    NsiTcpObjectGlobal = 0,
    NsiTcpObjectConnection = 16,           // TCP connection instances (key: 56-byte local/remote 4-tuple); SetTcpEntry deletes the TCB through it
    NsiTcpObjectPortReservationGranting = 19,
    NsiTcpObjectPortReservation = 20,
    NsiTcpObjectPortProperty = 25,
    NsiTcpObjectPortExclusionGranting = 29,
    NsiTcpObjectPortExclusion = 34
} NSI_TCP_OBJECT_ID;

/**
 * The NSI_UDP_OBJECT_ID values are the NSI object IDs of the UDP module (NPI_MS_UDP_MODULEID): global statistics, and
 * port reservations and exclusions.
 */
typedef enum _NSI_UDP_OBJECT_ID
{
    NsiUdpObjectGlobal = 0,
    NsiUdpObjectPortReservationGranting = 3,
    NsiUdpObjectPortReservation = 4,
    NsiUdpObjectPortExclusionGranting = 5
} NSI_UDP_OBJECT_ID;

/**
 * The NSI_FL6T_OBJECT_ID values are the NSI object IDs of the Teredo framing-layer module (NPI_MS_FL6T_MODULEID).
 * NsiFl6tObjectClientPort is the Teredo client port that NotifyTeredoPortChange watches.
 */
typedef enum _NSI_FL6T_OBJECT_ID
{
    NsiFl6tObjectClientPort = 3
} NSI_FL6T_OBJECT_ID;

/**
 * The NSI_PARAMETER_TYPE values select which parameter block of an object NsiGetParameter and NsiSetParameter access:
 * the read/write block (configuration), the read-only dynamic block (state and statistics) or the read-only static block
 * (properties fixed when the object is created).
 */
typedef enum _NSI_PARAMETER_TYPE
{
    NsiParameterTypeReadWrite = 0,
    NsiParameterTypeReadOnlyDynamic = 1,
    NsiParameterTypeReadOnlyStatic = 2
} NSI_PARAMETER_TYPE;

/**
 * The NSI_SET_TCP_ENTRY structure is the key of a TCP connection (NPI_MS_TCP_MODULEID, NsiTcpObjectConnection): the local
 * and remote endpoints as SOCKADDR_IN/SOCKADDR_IN6 values (56 bytes). SetTcpEntry passes it with MIB_TCP_STATE_DELETE_TCB
 * to reset a connection; System Informer uses it to close connections from the network tab.
 * \remarks Layout matches NSI_TCP_KEY_ROW.
 */
typedef struct _NSI_SET_TCP_ENTRY
{
    union
    {
        union
        {
            SOCKADDR_IN LocalTcpEntryIpV4;
            SOCKADDR_IN6 LocalTcpEntryIpV6;
        } DUMMYUNIONNAME;
        struct
        {
            ADDRESS_FAMILY LocalFamily;
            USHORT LocalPort;
            IN_ADDR LocalAddressIPv4;
            IN6_ADDR LocalAddressIPv6;
            ULONG LocalScopeID;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        union
        {
            SOCKADDR_IN RemoteTcpEntryIpV4;
            SOCKADDR_IN6 RemoteTcpEntryIpV6;
        } DUMMYUNIONNAME2;
        struct
        {
            ADDRESS_FAMILY RemoteFamily;
            USHORT RemotePort;
            IN_ADDR RemoteAddressIPv4;
            IN6_ADDR RemoteAddressIPv6;
            ULONG RemoteScopeID;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;
} NSI_SET_TCP_ENTRY, *PNSI_SET_TCP_ENTRY;

/**
 * The NSI_IP_INTERFACE_ROD structure is the read-only dynamic row of an IP interface (NPI_MS_IPV4_MODULEID or
 * NPI_MS_IPV6_MODULEID, NsiIpObjectIpInterface; key: NET_LUID). IP Helper reads it for GetIpInterfaceEntry and
 * GetIpInterfaceTable (MIB_IPINTERFACE_ROW) and for GetIpNetworkConnectionBandwidthEstimates.
 */
typedef struct _NSI_IP_INTERFACE_ROD
{
    NET_IFINDEX InterfaceIndex;             // IP interface index.
    NET_IF_COMPARTMENT_ID CompartmentId;    // Compartment that owns the interface.
    BOOLEAN SupportsWakeUpPatterns;         // MIB_IPINTERFACE_ROW.SupportsWakeUpPatterns.
    UCHAR Reserved1[3];                     // alignment padding; nothing reads or writes it
    ULONG ReachableTime;                    // MIB_IPINTERFACE_ROW.ReachableTime (milliseconds).
    ULONG Connected;                        // Non-zero when the interface is connected (MIB_IPINTERFACE_ROW.Connected).
    ULONG TimerRemaining;                   // Best-effort name: milliseconds until the interface's periodic timer fires
                                            //      (tcpip converts the remaining ticks with 500 ms per tick). IP Helper ignores it.
    ULONG64 FlInterfaceContext;             // Kernel address of the framing-layer interface context; tcpip returns 0 to
                                            //      user-mode callers. IP Helper ignores it.
    UCHAR TransmitOffload;                  // MIB_IPINTERFACE_ROW.TransmitOffload (NL_INTERFACE_OFFLOAD_ROD bits).
    UCHAR ReceiveOffload;                   // MIB_IPINTERFACE_ROW.ReceiveOffload (NL_INTERFACE_OFFLOAD_ROD bits).
    UCHAR Reserved2[6];                     // alignment padding; nothing reads or writes it
    NL_BANDWIDTH_INFORMATION InboundBandwidthInformation;  // MIB_IP_NETWORK_CONNECTION_BANDWIDTH_ESTIMATES.InboundBandwidthInformation.
    NL_BANDWIDTH_INFORMATION OutboundBandwidthInformation; // MIB_IP_NETWORK_CONNECTION_BANDWIDTH_ESTIMATES.OutboundBandwidthInformation.
} NSI_IP_INTERFACE_ROD, *PNSI_IP_INTERFACE_ROD;

/**
 * The NETIO_TCP_STATS_DYNAMIC structure is the read-only dynamic parameter block of the TCP global object
 * (NPI_MS_TCP_MODULEID, NsiTcpObjectGlobal; key: ADDRESS_FAMILY). Tcpip sums its per-processor TCP statistics into it.
 * \remarks The field names are tcpip's own: they come from the TraceLogging event "TcpGlobalRoParameters", which
 *          tcpip!TcpTraceConnectionSummary fills from the same per-processor blocks with this exact 0x68-byte layout.
 * TcpGetAllGlobalParameters fills this block only when the buffer holds at least 104 bytes.
 */
typedef struct _NETIO_TCP_STATS_DYNAMIC
{
    ULONG ActiveOpens;              // MIB_TCPSTATS2.dwActiveOpens
    ULONG PassiveOpens;             // MIB_TCPSTATS2.dwPassiveOpens
    ULONG AttemptFails;             // MIB_TCPSTATS2.dwAttemptFails
    ULONG EstabResets;              // MIB_TCPSTATS2.dwEstabResets
    ULONG CurrEstab;                // MIB_TCPSTATS2.dwCurrEstab
    ULONG TimeWaitCollisions;       // Connection attempts whose 4-tuple collided with a TIME_WAIT TCB (perf counter
                                    //      "TCPIP Extended Performance Diagnostics\TWTCB collisions"). IP Helper does not export it.
    LONG64 InSegs;                  // MIB_TCPSTATS2.dw64InSegs
    LONG64 OutSegs;                 // MIB_TCPSTATS2.dw64OutSegs
    ULONG RetransSegs;              // MIB_TCPSTATS2.dwRetransSegs
    ULONG OutRsts;                  // MIB_TCPSTATS2.dwOutRsts
    ULONG InErrs;                   // MIB_TCPSTATS2.dwInErrs
    ULONG NumConns;                 // MIB_TCPSTATS2.dwNumConns (TcpListenerCount plus the per-processor connection counts)
    ULONG64 BytesBuffered;          // Bytes of received data queued in TCB receive buffers (TcpDeliverInput adds,
                                    //      TcpFlushTcbInput subtracts). tcpip's NSI handler leaves it 0; only the
                                    //      TcpGlobalRoParameters telemetry event reports it.
    ULONG64 SendAvailable;          // Bytes of application send data queued on TCBs (TcpEnqueueTcbSend adds,
                                    //      TcpDequeueTcbSend subtracts). Left 0 by the NSI handler, like BytesBuffered.
    ULONG SilentModeRcvDrops;       // Received segments discarded silently by TcpDiscardReceive (silent-drop flag set).
                                    //      Left 0 by the NSI handler.
    ULONG SilentModeOutRstDrops;    // Of those, non-RST segments whose RST reply was suppressed. Left 0 by the NSI handler.
    ULONG FastopenActiveOpens;      // Client connections whose TCP Fast Open (RFC 7413) SYN data was accepted by the
                                    //      peer (TcpTcbCarefulDatagram, Fast Open state 7).
    ULONG FastopenPassiveOpens;     // Server connections that accepted data in the SYN with a valid Fast Open cookie
                                    //      (TcpHandleServerSideFastopenOption, Fast Open state 1).
    ULONG FastopenAttemptsFailed;   // Fast Open attempts that fell back to a normal handshake: cookie rejected, SYN
                                    //      data not acknowledged, or SYN retransmitted (Fast Open states 14, 15, 16, 18).
    ULONG FirstSynRexmits;          // Connections whose SYN was retransmitted at least once (first SYN RTO, TcpRtoTimeout).
    ULONG FirstSynRexmitsWithFastopen; // Subset of FirstSynRexmits for connections attempting Fast Open.
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
} NETIO_TCP_STATS_DYNAMIC, *PNETIO_TCP_STATS_DYNAMIC;

/**
 * The NSI_TCP_GLOBAL_ROS structure is the read-only static parameter block of the TCP global object
 * (NPI_MS_TCP_MODULEID, NsiTcpObjectGlobal).
 * \remarks TcpGetAllGlobalParameters requires at least sizeof(NSI_TCP_GLOBAL_ROS) (20) bytes; GetTcpStatisticsEx2 copies the first 16 bytes.
 */
typedef struct _NSI_TCP_GLOBAL_ROS
{
    ULONG RtoAlgorithm;             // MIB_TCPSTATS2.RtoAlgorithm (TCP_RTO_ALGORITHM)
    ULONG RtoMin;                   // MIB_TCPSTATS2.dwRtoMin
    ULONG RtoMax;                   // MIB_TCPSTATS2.dwRtoMax
    ULONG MaxConn;                  // MIB_TCPSTATS2.dwMaxConn
    BOOLEAN AutoTuningLevelLocalSet; // TRUE once NSI_TCP_GLOBAL_RW.AutoTuningLevelLocal was set to a level (not -2).
                                    //      While FALSE and no group policy level is set, TcpSelectRcvWndScaleFactor may pick the
                                    //      next higher receive-window scale. TcpUpdateGlobalParameters writes it. IP Helper does not export it.
    UCHAR Reserved[3];              // alignment padding; nothing reads or writes it
} NSI_TCP_GLOBAL_ROS, *PNSI_TCP_GLOBAL_ROS;

/**
 * The NSI_TCP_GLOBAL_RW structure is the read/write parameter block of the TCP global object (NPI_MS_TCP_MODULEID,
 * NsiTcpObjectGlobal; key: ADDRESS_FAMILY). These are the "netsh interface tcp set global" settings. On a set,
 * 0xFF (BOOLEAN/UCHAR) or 0xFFFFFFFF (ULONG/LONG) leaves a field unchanged.
 * \remarks The field names are tcpip's own, taken from its "TcpGlobalRwParameters" TraceLogging event and the
 * "TCP: Global parameters updated" ETW message. 
 */
typedef struct _NSI_TCP_GLOBAL_RW
{
    BOOLEAN EnablePMtuDiscovery;    // Default TRUE.
    BOOLEAN UseRfc1122UrgentPointer; // Default FALSE.
    BOOLEAN DisableTaskOffload;     // Default FALSE.
    BOOLEAN EnablePMtuBHDetect;     // PMTU black-hole detection. Default TRUE.
    BOOLEAN DisableTcpChimneyOffload; // Legacy chimney offload setting. Ignored on a set; always reported as 0.
    BOOLEAN DisableRss;             // Receive-side scaling. A get reports the global RSS state, not a per-family value.
    BOOLEAN EcnCapability;          // Explicit Congestion Notification.
    UCHAR MaxDataRetransmissions;   // Default 5.
    ULONG KeepAliveTime;            // Milliseconds. Default 7200000.
    ULONG KeepAliveInterval;        // Milliseconds. Default 1000.
    ULONG TimedWaitDelay;           // Seconds, 2..300. Default 120.
    ULONG SillyWindowTimeout;       // Milliseconds, 200..60000. Default 5000.
    ULONG FinWait2Timeout;          // Seconds, 30..2073600. Default 120.
    UCHAR CongestionAlgorithm;      // 0..7. Default 0.
    UCHAR UseRfc1323Timestamps;     // 0 = disabled, 1 = enabled, 2 = allowed (default).
    UCHAR Reserved1[2];             // alignment padding; nothing reads or writes it
    LONG AutoTuningLevelLocal;      // Receive-window auto-tuning level, -2..4 (-2 = not configured). Default 3.
    LONG AutoTuningLevelGroupPolicy; // Group-policy auto-tuning level, -2..4 (-2 = not configured). Default -2.
    BOOLEAN DisableRsc;             // Receive segment coalescing (OlmUpdateTcpGlobalParameters).
    BOOLEAN NotifyOnAllRetransmitTimeouts;
    UCHAR Reserved2[2];             // alignment padding; nothing reads or writes it
    ULONG PacingProfile;            // TCP pacing profile, 0..4; TcpCacheAfOptions copies it into each TCB unless the
                                    //      TcpPacingRate override is set. Default 0. Not in tcpip's ETW events (best-effort name).
} NSI_TCP_GLOBAL_RW, *PNSI_TCP_GLOBAL_RW;

/**
 * The NETIO_UDP_STATS_DYNAMIC structure is the read-only dynamic parameter block of the UDP global object
 * (NPI_MS_UDP_MODULEID, NsiUdpObjectGlobal; key: ADDRESS_FAMILY). Tcpip sums its per-processor UDP statistics into it.
 * \remarks  SwSegments and HwSegments
 *          use the names of tcpip's "UDP: Endpoint segment message" ETW event.
 */
typedef struct _NETIO_UDP_STATS_DYNAMIC
{
    LONG64 InDatagrams;             // MIB_UDPSTATS2.dw64InDatagrams
    ULONG NoPorts;                  // MIB_UDPSTATS2.dwNoPorts
    ULONG InErrors;                 // MIB_UDPSTATS2.dwInErrors
    LONG64 OutDatagrams;            // MIB_UDPSTATS2.dw64OutDatagrams
    ULONG NumAddrs;                 // MIB_UDPSTATS2.dwNumAddrs
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    ULONG64 SwSegments;             // Datagrams that tcpip split in software from a segmented (UDP_SEND_MSG_SIZE) send.
                                    //      IP Helper does not export it.
    ULONG64 HwSegments;             // Datagrams the adapter produced through UDP segmentation offload (USO).
                                    //      IP Helper does not export it.
} NETIO_UDP_STATS_DYNAMIC, *PNETIO_UDP_STATS_DYNAMIC;

/**
 * The NSI_UDP_GLOBAL_RW structure is the read/write parameter block of the UDP global object (NPI_MS_UDP_MODULEID,
 * NsiUdpObjectGlobal; key: ADDRESS_FAMILY). These are the "netsh interface udp set global" settings. On a set, 0xFF leaves a
 * field unchanged.
 */
typedef struct _NSI_UDP_GLOBAL_RW
{
    BOOLEAN DisableUro;             // UDP receive offload (receive segment coalescing). UdpOffloadSetUro applies it.
    BOOLEAN DisableUso;             // UDP segmentation offload. UdpUpdateUsoDisabledMask applies it.
} NSI_UDP_GLOBAL_RW, *PNSI_UDP_GLOBAL_RW;

/**
 * The ICMP_ECHO_REQUEST_CONTEXT structure tracks one outstanding echo request on an ICMP handle until the
 * completion notification arrives. It holds the caller's completion mechanism (event, or APC routine and context run on a
 * duplicated thread handle) and the reply buffer the notification is decoded into.
 * \remarks Mirrors the 0x48-byte block iphlpapi.dll!IcmpSendEcho2Ex allocates and links into ICMP_HANDLE_INTERNAL.RequestList
 *          10.0.26100.
 */
typedef struct _ICMP_ECHO_REQUEST_CONTEXT
{
    HANDLE EventHandle;
    HANDLE ThreadHandle;
    PIO_APC_ROUTINE ApcRoutine;
    PVOID ApcContext;
    PVOID ReplyBuffer;
    IO_STATUS_BLOCK IoStatusBlock;
    LIST_ENTRY Link;
} ICMP_ECHO_REQUEST_CONTEXT, *PICMP_ECHO_REQUEST_CONTEXT;

/**
 * The ICMP_HANDLE_INTERNAL structure is the object behind an ICMP handle (IcmpCreateFile/Icmp6CreateFile). It records
 * the owning process, the address family and NSI module used to submit echo requests, and the list of requests that are
 * still waiting for a reply.
 * \remarks Mirrors the 0x50-byte block iphlpapi.dll!IcmpCreateFileEx allocates 10.0.26100.
 */
typedef struct _ICMP_HANDLE_INTERNAL
{
    ULONG ProcessId;
    BOOLEAN IsIpv4;
    BYTE Reserved0[3];              // alignment padding; nothing reads or writes it
    CONST NPI_MODULEID* ModuleId;
    CRITICAL_SECTION Lock;
    LONG ActiveRequestCount;
    ULONG Reserved1;                // alignment padding; nothing reads or writes it
    LIST_ENTRY RequestList;
} ICMP_HANDLE_INTERNAL, *PICMP_HANDLE_INTERNAL;

/**
 * The ICMP_NOTIFICATION_HEADER structure starts every echo-completion notification tcpip.sys sends through the ICMP NSI
 * change notification. Status is the NTSTATUS of the request; when it is STATUS_UNSUCCESSFUL with HasIcmpError set, the
 * reply was an ICMP error whose type and code are given, and the caller converts them to an IP_STATUS.
 */
typedef struct _ICMP_NOTIFICATION_HEADER
{
    ULONG Status;
    ULONG HasIcmpError;
    ULONG IcmpType;
    ULONG IcmpCode;
} ICMP_NOTIFICATION_HEADER, *PICMP_NOTIFICATION_HEADER;

/**
 * The ICMP_NOTIFICATION_V4 structure is the IPv4 echo-completion notification. IP Helper converts it into an
 * ICMP_ECHO_REPLY followed by the echo data and IP options.
 */
typedef struct _ICMP_NOTIFICATION_V4
{
    ICMP_NOTIFICATION_HEADER Header;
    ULONG DataLength;               // Bytes of echo data in the reply (tcpip writes a ULONG; IP Helper reads the low USHORT).
    UCHAR Ttl;                      // TTL of the reply (copied to ICMP_ECHO_REPLY.Options.Ttl)
    UCHAR Tos;                      // TOS of the reply (copied to ICMP_ECHO_REPLY.Options.Tos)
    BYTE Reserved[2];               // alignment padding; nothing reads or writes it
    ULONG RoundTripTime;            // milliseconds
    IPAddr Address;                 // replying address
    UCHAR Flags;                    // IP_FLAG_REVERSE/IP_FLAG_DF (ipexport.h); copied to ICMP_ECHO_REPLY.Options.Flags
    UCHAR OptionsSize;
} ICMP_NOTIFICATION_V4, *PICMP_NOTIFICATION_V4;

/**
 * The ICMP_NOTIFICATION_V6 structure is the IPv6 echo-completion notification. IP Helper converts it into an
 * ICMPV6_ECHO_REPLY followed by the echo data.
 */
typedef struct _ICMP_NOTIFICATION_V6
{
    ICMP_NOTIFICATION_HEADER Header;
    ULONG DataLength;               // Bytes of echo data in the reply.
    UCHAR HopLimit;                 // Hop limit of the reply's IPv6 header; IP Helper does not report it.
    UCHAR TrafficClass;             // Traffic class of the reply's IPv6 header; IP Helper does not report it.
    BYTE Reserved[2];               // alignment padding; nothing reads or writes it
    ULONG RoundTripTime;            // milliseconds
    IN6_ADDR Address;               // replying address
    ULONG ScopeId;
} ICMP_NOTIFICATION_V6, *PICMP_NOTIFICATION_V6;

/**
 * The ICMP_NOTIFICATION_MESSAGE structure is the parameter-data argument of the ICMP NSI change-notification callback.
 * NotificationData points to an ICMP_NOTIFICATION_V4 or ICMP_NOTIFICATION_V6 depending on the module the handle was
 * registered with.
 */
typedef struct _ICMP_NOTIFICATION_MESSAGE
{
    ULONG NotificationDataLength;   // Best-effort name: length half of the RPC counted buffer winnsi.dll unmarshals
                                    // NotificationData into (NsiWorkerThread); IP Helper ignores it.
    union
    {
        PVOID Pointer;
        PICMP_NOTIFICATION_HEADER Header;
        PICMP_NOTIFICATION_V4 V4;
        PICMP_NOTIFICATION_V6 V6;
    } NotificationData;
} ICMP_NOTIFICATION_MESSAGE, *PICMP_NOTIFICATION_MESSAGE;

/**
 * The ICMP_REQUEST_RW structure is the NSI key of an echo request (NsiSetAllParameters with
 * IcmpNsiSetAllObjectSubmitEchoRequest/CompleteEchoRequest). Tcpip.sys stores it with the request and returns it in the
 * completion notification, which is how the caller finds its ICMP_ECHO_REQUEST_CONTEXT again.
 */
typedef struct _ICMP_REQUEST_RW
{
    PICMP_HANDLE_INTERNAL Handle;
    ULONG ProcessId;
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    PVOID ReplyBuffer;
    PICMP_ECHO_REQUEST_CONTEXT RequestContext;
} ICMP_REQUEST_RW, *PICMP_REQUEST_RW;

/**
 * The ICMP_REQUEST_V4 structure is the buffer System Informer submits to send an IPv4 echo request: the
 * ICMP_REQUEST_RW key followed by the read/write block tcpip.sys reads (request data, reply buffer, timeout, IP options,
 * TTL, DF/reverse flags and the source and destination addresses).
 */
typedef struct _ICMP_REQUEST_V4
{
    ICMP_REQUEST_RW ReadWrite;      // NSI key of the echo request
    ULONG RequestSize;              // (the read/write block tcpip!Ipv4SetEchoRequestCreate reads starts here)
    ULONG ReplySize;
    PVOID RequestData;
    PVOID ReplyBuffer;
    ULONG ReplyHeaderSize;
    ULONG Timeout;
    PUCHAR OptionsData;
    USHORT OptionsSize;
    BYTE Reserved0[2];              // alignment padding; nothing reads or writes it
    ULONG SourceScopeId;            // Scope of SourceAddress; tcpip uses it only when SourceAddress != 0.
    ULONG DestinationScopeId;       // Scope of DestinationAddress.
    UCHAR Ttl;
    UCHAR Reverse;                  // IP_FLAG_REVERSE
    BYTE Reserved1[2];              // alignment padding; nothing reads or writes it
    BOOLEAN DontFragment;           // IP_FLAG_DF
    BYTE Reserved2[3];              // alignment padding; nothing reads or writes it
    IPAddr SourceAddress;
    IPAddr DestinationAddress;
    ULONG Reserved3;                // alignment padding; nothing reads or writes it
} ICMP_REQUEST_V4, *PICMP_REQUEST_V4;

/**
 * The ICMP_REQUEST_V6 structure is the IPv6 counterpart of ICMP_REQUEST_V4 (Icmp6SendEcho2).
 */
typedef struct _ICMP_REQUEST_V6
{
    ICMP_REQUEST_RW ReadWrite;
    ULONG RequestSize;
    ULONG ReplySize;
    PVOID RequestData;
    PVOID ReplyBuffer;
    ULONG ReplyHeaderSize;
    ULONG Timeout;
    PUCHAR OptionsData;
    USHORT OptionsSize;
    UCHAR Reserved1[2];             // alignment padding; nothing reads or writes it
    ULONG SourceScopeId;
    ULONG DestinationScopeId;
    UCHAR Ttl;
    UCHAR Reverse;
    UCHAR Reserved2[2];             // alignment padding; nothing reads or writes it
    IN6_ADDR SourceAddress;
    IN6_ADDR DestinationAddress;
} ICMP_REQUEST_V6, *PICMP_REQUEST_V6;

/**
 * The ICMP_NSI_SET_ALL_PARAMETERS_OBJECT_ID values are the object IDs netsup.c passes to NsiSetAllParameters to submit
 * an echo request and to acknowledge its completion. They are the same values as NSI_ICMP_SET_ACTION_OBJECT_ID.
 */
typedef enum _ICMP_NSI_SET_ALL_PARAMETERS_OBJECT_ID
{
    IcmpNsiSetAllObjectSubmitEchoRequest = 6,
    IcmpNsiSetAllObjectCompleteEchoRequest = 7
} ICMP_NSI_SET_ALL_PARAMETERS_OBJECT_ID;

/**
 * The NSI_COUNTED_WCHAR_PARAMETER structure is a counted Unicode string as NSI providers return it for single string
 * parameters (interface alias and description, compartment and network names). Length is in bytes and the buffer is not
 * necessarily null-terminated. It has the same layout as IF_COUNTED_STRING (516 bytes).
 */
typedef struct _NSI_COUNTED_WCHAR_PARAMETER
{
    USHORT Length;
    _Field_size_part_(257, Length / sizeof(WCHAR))
    WCHAR Buffer[257];
} NSI_COUNTED_WCHAR_PARAMETER, *PNSI_COUNTED_WCHAR_PARAMETER;

/**
 * The NET_IF_INTERFACE_FLAGS bits are stored in NDIS_NSI_INTERFACE_INFORMATION_ROS.ifFlags and NDIS_NSI_INTERFACE_ENUM_ROS.ifFlags
 * (ndis.sys NDIS_IF_BLOCK ifFlags). IP Helper
 * converts them into MIB_IF_ROW2.InterfaceAndOperStatusFlags, and ConvertInterfacePhysicalAddressToLuid skips filter interfaces.
 * \remarks Each bit is derived from NDIS_MINIPORT_BLOCK state when the interface is initialized; IP Helper
 *          (FillIfEntryWithInterfaceInfo) only reads 0x1, 0x2 and 0x40.
 */
typedef enum _NET_IF_INTERFACE_FLAGS
{
    NetIfInterfaceFlagHardwareInterface = 0x00000001, // Miniport registered NDIS_MINIPORT_ATTRIBUTES_HARDWARE_DEVICE (MIB_IF_ROW2 HardwareInterface).
    NetIfInterfaceFlagFilterInterface = 0x00000002,   // Miniport with more than one bind path (MIB_IF_ROW2 FilterInterface).
    NetIfInterfaceFlagNdisWdm = 0x00000020,           // Miniport registered NDIS_MINIPORT_ATTRIBUTES_NDIS_WDM: an NDIS-WDM miniport whose lower edge is a
                                                      // WDM bus driver (USB, IEEE 1394) instead of claimed hardware resources.
    NetIfInterfaceFlagEndPointInterface = 0x00000040, // *NdisDeviceType = NDIS_DEVICE_TYPE_ENDPOINT (MIB_IF_ROW2 EndPointInterface).
    NetIfInterfaceFlagAoAcCapable = 0x00000100        // Miniport (or a filter above it) supports connected standby (AOAC).
} NET_IF_INTERFACE_FLAGS;

typedef NSI_COUNTED_WCHAR_PARAMETER NSI_STRING_256, *PNSI_STRING_256;

/**
 * The NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST structure is the input of NsiDeregisterChangeNotificationEx. It
 * cancels a notification registered with NsiRegisterChangeNotificationEx.
 */
typedef struct _NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST
{
    ULONG ModuleHandle;             // 0 = look the module up by ModuleId
    ULONG Reserved04;               // alignment padding; nothing reads or writes it
    PNPI_MODULEID ModuleId;
    NSI_OBJECT_ID ObjectId;
    ULONG Reserved14;               // alignment padding; nothing reads or writes it
    PVOID NotificationHandle;
    ULONG ProcessId;
    ULONG Reserved24;               // alignment padding; nothing reads or writes it
} NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST, *PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST;

/**
 * The NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX structure is the input of NsiEnumerateObjectsAllParametersEx. It
 * describes up to four parallel tables (key, read/write, read-only dynamic, read-only static) that the provider fills with
 * one row per object; on input Count is the number of rows the tables can hold, on output the number returned.
 */
typedef struct _NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX
{
    ULONGLONG MustBeZero;           // netio.sys fails the request with STATUS_NOT_IMPLEMENTED unless 0 (winnsi.dll: ERROR_CALL_NOT_IMPLEMENTED)
    ULONG ModuleHandle;             // 0 = look the module up by ModuleId
    ULONG Reserved0C;               // alignment padding; nothing reads or writes it
    PNPI_MODULEID ModuleId;
    NSI_OBJECT_ID ObjectId;
    ULONG Reserved1C;               // alignment padding; nothing reads or writes it
    NSI_STORE Store;                // NsiPersistent, NsiActive or NsiCurrent (NsiBoth is rejected)
    NSI_GET_ACTION Action;          // NsiGetFirst or NsiGetNext only
    PVOID KeyTable;
    ULONG KeyEntrySize;
    ULONG Reserved34;               // alignment padding; nothing reads or writes it
    PVOID ReadWriteTable;
    ULONG ReadWriteEntrySize;
    ULONG Reserved44;               // alignment padding; nothing reads or writes it
    PVOID DynamicTable;
    ULONG DynamicEntrySize;
    ULONG Reserved54;               // alignment padding; nothing reads or writes it
    PVOID StaticTable;
    ULONG StaticEntrySize;
    ULONG Reserved64;               // alignment padding; nothing reads or writes it
    ULONG Count;
    ULONG Reserved6C;               // alignment padding; nothing reads or writes it
} NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, *PNSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX;

/**
 * The NSI_ESTATS_REC_ROD_INTERNAL structure is the read-only dynamic block of the receive extended statistics of a TCP
 * connection (TcpConnectionEstatsRec). GetPerTcpConnectionEStats converts it to TCP_ESTATS_REC_ROD_v0.
 */
typedef struct _NSI_ESTATS_REC_ROD_INTERNAL
{
    ULONG CurRwinSent;
    ULONG MaxRwinSent;
    ULONG MinRwinSent;
    ULONG LimRwin;
    ULONG DupAckEpisodes;
    ULONG DupAcksOut;
    ULONG CeRcvd;
    ULONG EcnSent;
    ULONG EcnNoncesRcvd;
    ULONG CurReasmQueue;
    ULONG MaxReasmQueue;
    ULONG CurAppRQueue;
    ULONG MaxAppRQueue;
    ULONG WinScaleSent;
} NSI_ESTATS_REC_ROD_INTERNAL, *PNSI_ESTATS_REC_ROD_INTERNAL;

/**
 * The NSI_ESTATS_SND_CONG_RW_v0 structure is the read/write block of the send-congestion extended statistics of a TCP
 * connection. Setting EnableCollection starts or stops collection (SetPerTcpConnectionEStats).
 */
typedef struct _NSI_ESTATS_SND_CONG_RW_v0
{
    BOOLEAN EnableCollection;
} NSI_ESTATS_SND_CONG_RW_v0, *PNSI_ESTATS_SND_CONG_RW_v0;

/**
 * The NSI_ESTATS_TCP_KEY structure is the key that selects a TCP connection for the extended statistics (ESTATS)
 * objects. It holds both an IPv4 and an IPv6 address slot for each endpoint; only the slot matching the address family
 * is used.
 */
typedef struct _NSI_ESTATS_TCP_KEY
{
    ADDRESS_FAMILY AddressFamily;
    USHORT LocalPort;
    IN_ADDR LocalAddressIPv4;
    IN6_ADDR LocalAddressIPv6;
    ULONG LocalScopeId;
    ADDRESS_FAMILY RemoteAddressFamily;
    USHORT RemotePort;
    IN_ADDR RemoteAddressIPv4;
    IN6_ADDR RemoteAddressIPv6;
    ULONG RemoteScopeId;
} NSI_ESTATS_TCP_KEY, *PNSI_ESTATS_TCP_KEY;

/**
 * The NSI_ESTATS_TCP_KEY_V4 structure is the AF_INET view of NSI_ESTATS_TCP_KEY. System Informer fills it from a
 * MIB_TCPROW to query the extended statistics of an IPv4 connection.
 */
typedef struct _NSI_ESTATS_TCP_KEY_V4
{
    ADDRESS_FAMILY AddressFamily;   // AF_INET
    USHORT LocalPort;
    ULONG LocalAddress;
    ULONG Reserved08[5];            // unused IPv6 address and scope slots of the AF_INET key; nothing reads or writes it
    ADDRESS_FAMILY RemoteAddressFamily; // AF_INET
    USHORT RemotePort;
    ULONG RemoteAddress;
    ULONG Reserved24[5];            // unused IPv6 address and scope slots of the AF_INET key; nothing reads or writes it
} NSI_ESTATS_TCP_KEY_V4, *PNSI_ESTATS_TCP_KEY_V4;

/**
 * The NSI_EX_REQUEST_COMMON structure is the header shared by the *Ex request blocks that nsi.dll passes to netio.sys
 * (NsiGetParameterEx, NsiGetAllParametersEx, NsiSetParameterEx, NsiSetAllParametersEx). It selects the module, object and
 * store, and the get or set action.
 */
typedef struct _NSI_EX_REQUEST_COMMON
{
    ULONGLONG MustBeZero;           // netio.sys fails the request with STATUS_NOT_IMPLEMENTED unless 0 (winnsi.dll: ERROR_CALL_NOT_IMPLEMENTED)
    ULONG ModuleHandle;             // module handle (see NsiRequestChangeNotification); 0 = look the module up by ModuleId
    ULONG Reserved0C;               // alignment padding; nothing reads or writes it
    PNPI_MODULEID ModuleId;
    NSI_OBJECT_ID ObjectId;
    ULONG Reserved1C;               // alignment padding; nothing reads or writes it
    NSI_STORE Store;
    union                           // 0x24
    {
        NSI_GET_ACTION GetAction;   // get/enumerate requests
        NSI_SET_ACTION SetAction;   // set requests
    };
} NSI_EX_REQUEST_COMMON, *PNSI_EX_REQUEST_COMMON;

/**
 * The NSI_GET_ALL_PARAMETERS_EX_REQUEST structure is the input of NsiGetAllParametersEx. It reads the read/write,
 * read-only dynamic and read-only static blocks of one object in a single call; any buffer may be NULL.
 * \remarks Nsi.dll fails the call with STATUS_INVALID_PARAMETER if the provider returns a read/write block smaller than
 * ReadWriteBufferSize. 
 */
typedef struct _NSI_GET_ALL_PARAMETERS_EX_REQUEST
{
    NSI_EX_REQUEST_COMMON Header;
    PVOID Key;
    ULONG KeySize;
    ULONG Reserved34;               // alignment padding; nothing reads or writes it
    PVOID ReadWriteBuffer;
    ULONG ReadWriteBufferSize;
    ULONG Reserved44;               // alignment padding; nothing reads or writes it
    PVOID DynamicBuffer;
    ULONG DynamicBufferSize;
    ULONG Reserved54;               // alignment padding; nothing reads or writes it
    PVOID StaticBuffer;
    ULONG StaticBufferSize;
    ULONG Reserved64;               // alignment padding; nothing reads or writes it
} NSI_GET_ALL_PARAMETERS_EX_REQUEST, *PNSI_GET_ALL_PARAMETERS_EX_REQUEST;

/**
 * The NSI_GET_PARAMETER_EX_REQUEST structure is the input of NsiGetParameterEx. It reads BufferLength bytes at
 * ParameterOffset of one block (ParameterType) of one object, for example a single interface's alias.
 */
typedef struct _NSI_GET_PARAMETER_EX_REQUEST
{
    NSI_EX_REQUEST_COMMON Header;
    PVOID Key;
    ULONG KeySize;
    ULONG Reserved34;               // alignment padding; nothing reads or writes it
    NSI_PARAMETER_TYPE ParameterType;
    ULONG Reserved3C;               // alignment padding; nothing reads or writes it
    PVOID Buffer;
    ULONG BufferLength;             // in: size of Buffer (also used as the IOCTL output length); out: bytes returned
    ULONG ParameterOffset;          // byte offset of the field within the ParameterType row
} NSI_GET_PARAMETER_EX_REQUEST, *PNSI_GET_PARAMETER_EX_REQUEST;

/**
 * The MIB_IP_ANYCAST_KEY_ROW_V4 structure is the key of an IPv4 anycast address (NPI_MS_IPV4_MODULEID,
 * NsiIpObjectAnycastAddress). IP Helper converts it to MIB_ANYCASTIPADDRESS_ROW.
 */
typedef struct _MIB_IP_ANYCAST_KEY_ROW_V4
{
    NET_LUID InterfaceLuid;
    IN_ADDR Address;
    UCHAR Reserved[4];              // alignment padding; nothing reads or writes it
} MIB_IP_ANYCAST_KEY_ROW_V4, *PMIB_IP_ANYCAST_KEY_ROW_V4;

/**
 * The MIB_IP_ANYCAST_KEY_ROW_V6 structure is the key of an IPv6 anycast address (NPI_MS_IPV6_MODULEID,
 * NsiIpObjectAnycastAddress).
 */
typedef struct _MIB_IP_ANYCAST_KEY_ROW_V6
{
    NET_LUID InterfaceLuid;
    IN6_ADDR Address;
} MIB_IP_ANYCAST_KEY_ROW_V6, *PMIB_IP_ANYCAST_KEY_ROW_V6;

/**
 * The MIB_IP_NEIGHBOR_DYNAMIC_ROW structure is the read-only dynamic row of a neighbor (NsiIpObjectNeighbor), that is an
 * ARP (IPv4) or neighbor discovery (IPv6) cache entry. IP Helper converts it to MIB_IPNET_ROW2.
 */
typedef struct _MIB_IP_NEIGHBOR_DYNAMIC_ROW
{
    NL_NEIGHBOR_STATE State;
    ULONG ReachabilityTime;         // MIB_IPNET_ROW2.ReachabilityTime (LastReachable/LastUnreachable)
    BOOLEAN IsRouter;               // MIB_IPNET_ROW2.IsRouter
    BOOLEAN IsUnreachable;          // MIB_IPNET_ROW2.IsUnreachable
    USHORT PhysicalAddressLength;
    NET_IF_COMPARTMENT_ID CompartmentId;
} MIB_IP_NEIGHBOR_DYNAMIC_ROW, *PMIB_IP_NEIGHBOR_DYNAMIC_ROW;

/**
 * The MIB_IP_NEIGHBOR_KEY_ROW_V4 structure is the key of an IPv4 neighbor (NPI_MS_IPV4_MODULEID, NsiIpObjectNeighbor).
 * \remarks Tcpip copies only the address length of Address.
 */
typedef struct _MIB_IP_NEIGHBOR_KEY_ROW_V4
{
    NET_LUID InterfaceLuid;         // IP interface.
    NET_LUID SubInterfaceLuid;      // Sub-interface the neighbor was resolved on (tcpip!IppFindSubInterfaceByLuid).
    IN_ADDR Address;
    UCHAR Reserved[4];              // alignment padding; nothing reads or writes it
} MIB_IP_NEIGHBOR_KEY_ROW_V4, *PMIB_IP_NEIGHBOR_KEY_ROW_V4;

/**
 * The MIB_IP_NEIGHBOR_KEY_ROW_V6 structure is the key of an IPv6 neighbor (NPI_MS_IPV6_MODULEID, NsiIpObjectNeighbor).
 */
typedef struct _MIB_IP_NEIGHBOR_KEY_ROW_V6
{
    NET_LUID InterfaceLuid;         // IP interface.
    NET_LUID SubInterfaceLuid;      // Sub-interface the neighbor was resolved on (tcpip!IppFindSubInterfaceByLuid).
    IN6_ADDR Address;
} MIB_IP_NEIGHBOR_KEY_ROW_V6, *PMIB_IP_NEIGHBOR_KEY_ROW_V6;

/**
 * The MIB_IP_NEIGHBOR_RW_ROW structure is the read/write row of a neighbor (NsiIpObjectNeighbor): its link-layer
 * address. The valid length is MIB_IP_NEIGHBOR_DYNAMIC_ROW.PhysicalAddressLength. CreateIpNetEntry2/SetIpNetEntry2 write it.
 */
typedef struct _MIB_IP_NEIGHBOR_RW_ROW
{
    UCHAR PhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
} MIB_IP_NEIGHBOR_RW_ROW, *PMIB_IP_NEIGHBOR_RW_ROW;

/**
 * The MIB_IP_NEIGHBOR_STATIC_ROW structure is the read-only static row of a neighbor (NsiIpObjectNeighbor).
 */
typedef struct _MIB_IP_NEIGHBOR_STATIC_ROW
{
    NET_IFINDEX InterfaceIndex;     // IP interface index (tcpip!IppFillNeighborParametersUnderLock; MIB_IPNET_ROW2.InterfaceIndex).
} MIB_IP_NEIGHBOR_STATIC_ROW, *PMIB_IP_NEIGHBOR_STATIC_ROW;

/**
 * The MIB_IP_ROUTE_DYNAMIC_ROW_V4 structure is the read-only dynamic row of an IPv4 route (NsiIpObjectRoute). Age is
 * MIB_IPFORWARD_ROW2.Age (seconds since the route was added); the rest is provider state IP Helper does not expose.
 */
typedef struct _MIB_IP_ROUTE_DYNAMIC_ROW_V4
{
    ULONG Age;
    BYTE InternalState[16];
} MIB_IP_ROUTE_DYNAMIC_ROW_V4, *PMIB_IP_ROUTE_DYNAMIC_ROW_V4;

/**
 * The MIB_IP_ROUTE_DYNAMIC_ROW_V6 structure is the IPv6 counterpart of MIB_IP_ROUTE_DYNAMIC_ROW_V4.
 */
typedef struct _MIB_IP_ROUTE_DYNAMIC_ROW_V6
{
    ULONG Age;
    BYTE InternalState[28];
} MIB_IP_ROUTE_DYNAMIC_ROW_V6, *PMIB_IP_ROUTE_DYNAMIC_ROW_V6;

/**
 * The MIB_IP_ROUTE_KEY_ROW_V4 structure is the key of an IPv4 unicast route (NPI_MS_IPV4_MODULEID, NsiIpObjectRoute).
 */
typedef struct _MIB_IP_ROUTE_KEY_ROW_V4
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    IN_ADDR DestinationPrefix;
    UCHAR DestinationPrefixLength;
    UCHAR Reserved1[3];                     // alignment padding; nothing reads or writes it
    IN_ADDR SourcePrefix;                   // Source-specific route prefix (0.0.0.0/0 for ordinary routes).
    UCHAR SourcePrefixLength;
    UCHAR Reserved2[7];                     // alignment padding; nothing reads or writes it
    NET_LUID InterfaceLuid;
    NET_LUID SubInterfaceLuid;              // Sub-interface of the next hop.
    IN_ADDR NextHop;
    ULONG Reserved3;                        // alignment padding; nothing reads or writes it
} MIB_IP_ROUTE_KEY_ROW_V4, *PMIB_IP_ROUTE_KEY_ROW_V4;

/**
 * The MIB_IP_ROUTE_KEY_ROW_V6 structure is the key of an IPv6 unicast route (NPI_MS_IPV6_MODULEID, NsiIpObjectRoute).
 */
typedef struct _MIB_IP_ROUTE_KEY_ROW_V6
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    IN6_ADDR DestinationPrefix;
    UCHAR DestinationPrefixLength;
    UCHAR Reserved1;                        // alignment padding; nothing reads or writes it
    IN6_ADDR SourcePrefix;                  // Source-specific route prefix (::/0 for ordinary routes).
    UCHAR SourcePrefixLength;
    UCHAR Reserved2;                        // alignment padding; nothing reads or writes it
    NET_LUID InterfaceLuid;
    NET_LUID SubInterfaceLuid;              // Sub-interface of the next hop.
    IN6_ADDR NextHop;
} MIB_IP_ROUTE_KEY_ROW_V6, *PMIB_IP_ROUTE_KEY_ROW_V6;

/**
 * The MIB_IP_ROUTE_RW_ROW structure is the read/write parameter block of a unicast route (NsiIpObjectRoute). IP Helper
 * converts it to and from MIB_IPFORWARD_ROW2. On a set, 0xFF (BOOLEAN) or 0xFFFFFFFF (ULONG) leaves a field unchanged.
 */
typedef struct _MIB_IP_ROUTE_RW_ROW
{
    UCHAR SitePrefixLength;                 // MIB_IPFORWARD_ROW2.SitePrefixLength
    UCHAR Reserved[3];                      // alignment padding; nothing reads it (IP Helper fills it with 0xFF on a set)
    ULONG ValidLifetime;                    // seconds
    ULONG PreferredLifetime;                // seconds
    ULONG Metric;
    NL_ROUTE_PROTOCOL Protocol;
    BOOLEAN Loopback;
    BOOLEAN AutoconfigureAddress;           // Advertised as the A flag of the router advertisement prefix option.
    BOOLEAN Publish;                        // Advertised in router advertisements.
    BOOLEAN Immortal;
    BOOLEAN AdvertiseDefaultValidLifetime;  // Router advertisements carry the default 30-day valid lifetime instead of
                                            //      the route's remaining lifetime (set by default on new routes).
    BOOLEAN AdvertiseDefaultPreferredLifetime; // Same for the 7-day preferred lifetime (set by default on new routes).
    BOOLEAN OnLink;                         // Route without a gateway; advertised as the L (on-link) flag.
    UCHAR Reserved2;                        // alignment padding; nothing reads or writes it
    ULONG DgdState;                         // Dead-gateway-detection state (tcpip!IppUpdateRouteDgdState).
} MIB_IP_ROUTE_RW_ROW, *PMIB_IP_ROUTE_RW_ROW;

/**
 * The MIB_IP_ROUTE_STATIC_ROW structure is the read-only static row of a unicast route (NsiIpObjectRoute).
 */
typedef struct _MIB_IP_ROUTE_STATIC_ROW
{
    NL_ROUTE_ORIGIN Origin;         // MIB_IPFORWARD_ROW2.Origin
    NET_IFINDEX InterfaceIndex;     // MIB_IPFORWARD_ROW2.InterfaceIndex
} MIB_IP_ROUTE_STATIC_ROW, *PMIB_IP_ROUTE_STATIC_ROW;

/**
 * The MIB_IP_UNICAST_DYNAMIC_ROW structure is the read-only dynamic row of a unicast address
 * (NsiIpObjectUnicastAddress): the address scope and duplicate address detection state (MIB_UNICASTIPADDRESS_ROW
 * ScopeId and DadState).
 */
typedef struct _MIB_IP_UNICAST_DYNAMIC_ROW
{
    SCOPE_ID ScopeId;
    NL_DAD_STATE DadState;
} MIB_IP_UNICAST_DYNAMIC_ROW, *PMIB_IP_UNICAST_DYNAMIC_ROW;

/**
 * The MIB_IP_UNICAST_RW_ROW structure is the read/write parameter block of a unicast address
 * (NsiIpObjectUnicastAddress). On a set, 0xFF (BOOLEAN) or 0xFFFFFFFF (ULONG) leaves a field unchanged.
 */
typedef struct _MIB_IP_UNICAST_RW_ROW
{
    ULONG PreferredLifetime;
    ULONG ValidLifetime;
    NL_PREFIX_ORIGIN PrefixOrigin;
    NL_SUFFIX_ORIGIN SuffixOrigin;
    UCHAR OnLinkPrefixLength;
    BOOLEAN SkipAsSource;
    USHORT Reserved;                // alignment padding; nothing reads or writes it
    ULONG SourcePreference;         // Best-effort name: stored in the address; changing it invalidates the destination
                                    //      cache, so it influences source address selection. IP Helper does not expose it.
    BOOLEAN SkipAsSourceOverride;   // Best-effort name: address flag 0x80; tcpip changes it only while SkipAsSource is set.
    BOOLEAN SkipDad;                // Create only: the new address starts in IpDadStatePreferred without duplicate address detection.
    UCHAR Reserved2[2];             // alignment padding; nothing reads or writes it
} MIB_IP_UNICAST_RW_ROW, *PMIB_IP_UNICAST_RW_ROW;

/**
 * The MIB_IP_UNICAST_KEY_ROW_V4 structure is the key of an IPv4 unicast address (NPI_MS_IPV4_MODULEID,
 * NsiIpObjectUnicastAddress). IP Helper converts it to MIB_UNICASTIPADDRESS_ROW.
 */
typedef struct _MIB_IP_UNICAST_KEY_ROW_V4
{
    NET_LUID InterfaceLuid;
    IN_ADDR Address;
    UCHAR Reserved[4];              // alignment padding; nothing reads or writes it
} MIB_IP_UNICAST_KEY_ROW_V4, *PMIB_IP_UNICAST_KEY_ROW_V4;

/**
 * The MIB_IP_UNICAST_KEY_ROW_V6 structure is the key of an IPv6 unicast address (NPI_MS_IPV6_MODULEID,
 * NsiIpObjectUnicastAddress).
 */
typedef struct _MIB_IP_UNICAST_KEY_ROW_V6
{
    NET_LUID InterfaceLuid;
    IN6_ADDR Address;
} MIB_IP_UNICAST_KEY_ROW_V6, *PMIB_IP_UNICAST_KEY_ROW_V6;

/**
 * The MIB_IP_UNICAST_STATIC_ROW structure is the read-only static row of a unicast address: the time the address was
 * created (MIB_UNICASTIPADDRESS_ROW.CreationTimeStamp).
 */
typedef struct _MIB_IP_UNICAST_STATIC_ROW
{
    ULONGLONG CreationTimestamp;
} MIB_IP_UNICAST_STATIC_ROW, *PMIB_IP_UNICAST_STATIC_ROW;

/**
 * The MIB_NDIS_COMPARTMENT_KEY_ROW structure is the key of an NDIS network compartment (NPI_MS_NDIS_MODULEID,
 * NsiNdisObjectCompartment). Its read/write row is NDIS_NSI_COMPARTMENT_RW.
 */
typedef struct _MIB_NDIS_COMPARTMENT_KEY_ROW
{
    NET_IF_COMPARTMENT_ID CompartmentId;
} MIB_NDIS_COMPARTMENT_KEY_ROW, *PMIB_NDIS_COMPARTMENT_KEY_ROW;

// Prefer the SDK declaration (ndis/objectheader.h, included by ntddndis.h) so that header can be included in either
// order. Keep the definition below for SDKs that do not provide it.
#if defined(__has_include)
#if __has_include(<ndis/objectheader.h>)
#include <ndis/objectheader.h>
#define PHNT_NDIS_OBJECT_HEADER_DEFINED
#endif
#endif

#ifndef PHNT_NDIS_OBJECT_HEADER_DEFINED
#define PHNT_NDIS_OBJECT_HEADER_DEFINED
/**
 * The NDIS_OBJECT_HEADER structure identifies the type, revision and size of a versioned NDIS structure. NDIS validates
 * all three before it reads the rest of the structure, which lets a newer driver accept older callers' layouts.
 *
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/objectheader/ns-objectheader-ndis_object_header
 */
typedef struct _NDIS_OBJECT_HEADER
{
    UCHAR Type;                     // NDIS_OBJECT_TYPE_* value that identifies the structure.
    UCHAR Revision;                 // Structure revision; higher revisions append fields.
    USHORT Size;                    // Size in bytes of the structure, including this header.
} NDIS_OBJECT_HEADER, *PNDIS_OBJECT_HEADER;
#endif

#ifndef NDIS_OBJECT_TYPE_NSI_COMPARTMENT_RW_STRUCT
#define NDIS_OBJECT_TYPE_NSI_COMPARTMENT_RW_STRUCT 0xAD // ntddndis.h
#endif

#define NDIS_NSI_COMPARTMENT_RW_REVISION_1 1 // through CompartmentGuid
#define NDIS_NSI_COMPARTMENT_RW_REVISION_3 3 // through NamespaceGuid (current; written by iphlpapi.dll!CreateCompartment)

#define NDIS_SIZEOF_NSI_COMPARTMENT_RW_REVISION_1 RTL_SIZEOF_THROUGH_FIELD(NDIS_NSI_COMPARTMENT_RW, CompartmentGuid)
#define NDIS_SIZEOF_NSI_COMPARTMENT_RW_REVISION_3 RTL_SIZEOF_THROUGH_FIELD(NDIS_NSI_COMPARTMENT_RW, NamespaceGuid)

/**
 * The NDIS_COMPARTMENT_FLAGS bits are stored in NDIS_NSI_COMPARTMENT_RW.Flags when a compartment is created through
 * NsiSetAllParameters(NPI_MS_NDIS_MODULEID, NdisNsiObjectCompartment) and copied into the ndis.sys compartment block.
 * \remarks  Ndis.sys copies Flags into the
 *          compartment block only when Header.Revision >= 2.
 */
typedef enum _NDIS_COMPARTMENT_FLAGS
{
    NdisCompartmentFlagNonPersistent = 0x00000001,  // The loopback interface's NET_LUID index is allocated without being recorded in the registry,
                                                    // so it is not reserved across reboots. iphlpapi.dll!CreateCompartment always sets it.
    NdisCompartmentFlagNotEnumerable = 0x00000002,  // Hidden from enumeration and from callers in other compartments.
    NdisCompartmentFlagOwnNamespace = 0x00000004    // The compartment GUID becomes its namespace GUID (callers in that namespace may access it).
} NDIS_COMPARTMENT_FLAGS;

/**
 * The NDIS_NSI_COMPARTMENT_RW structure is the read/write parameter block of an NDIS network compartment
 * (NPI_MS_NDIS_MODULEID, NdisNsiObjectCompartment). Callers fill the header, CompartmentGuid, CompartmentName and Flags to
 * create a compartment; ndis.sys creates the compartment's network and software loopback interface, fills the loopback and
 * network fields, keeps a copy in the compartment block and returns that copy from enumeration.
 * \remarks The ndis.sys symbol name is _NDIS_NSI_COMPARTMENT_RW. Setting compartment parameters requires a 0x668-byte input. Persisted records with Revision < 3 or Size < 0x664 are upgraded during compartment initialization.
 */
typedef struct _NDIS_NSI_COMPARTMENT_RW
{
    NDIS_OBJECT_HEADER Header;              // Type = NDIS_OBJECT_TYPE_NSI_COMPARTMENT_RW_STRUCT, Revision = NDIS_NSI_COMPARTMENT_RW_REVISION_3, Size = sizeof(NDIS_NSI_COMPARTMENT_RW).
    ULONG Reserved;                         // alignment padding; nothing reads or writes it
    NET_LUID LoopbackInterfaceLuid;         // [out] LUID of the compartment's software loopback interface (IfType IF_TYPE_SOFTWARE_LOOPBACK).
    NET_IF_NETWORK_GUID NetworkGuid;        // [out] GUID of the network ndis.sys creates for the compartment; the loopback interface joins it.
    GUID LoopbackInterfaceGuid;             // [out] Interface GUID generated for the loopback interface.
    IF_COUNTED_STRING LoopbackInterfaceAlias;       // [out] Loopback friendly name (ifAlias), suffixed with the compartment ID.
    IF_COUNTED_STRING LoopbackInterfaceDescription; // [out] Loopback description (ifDescr), suffixed with the compartment ID.
    GUID CompartmentGuid;                   // [in] Identifies the compartment; ConvertCompartmentGuidToId looks it up.
    IF_COUNTED_STRING CompartmentName;      // [in] Length is in bytes; the default compartment is named "Default Compartment".
    ULONG SystemCompartment;                // Best-effort name: 1 for compartments ndis.sys creates or upgrades itself (the default
                                            // compartment and migrated persisted records), 0 for iphlpapi CreateCompartment.
                                            // Stored and returned by enumeration; ndis.sys never tests it.
    ULONG Flags;                            // [in] NDIS_COMPARTMENT_FLAGS (Revision >= 2).
    GUID NamespaceGuid;                     // [in] Namespace the compartment belongs to; replaced with CompartmentGuid when
                                            // NdisCompartmentFlagOwnNamespace is set, otherwise inherited from the default compartment.
} NDIS_NSI_COMPARTMENT_RW, *PNDIS_NSI_COMPARTMENT_RW;

/**
 * The MIB_NDIS_IFSTACK_KEY_ROW structure is the key of an interface stack entry (NPI_MS_NDIS_MODULEID,
 * NsiNdisObjectIfStack): one higher/lower layer relationship between two interfaces, for example a filter or a LBFO team
 * above a physical adapter. IP Helper returns it as MIB_IFSTACK_ROW (GetIfStackTable).
 */
typedef struct _MIB_NDIS_IFSTACK_KEY_ROW
{
    NET_IFINDEX HigherLayerInterfaceIndex;
    NET_IFINDEX LowerLayerInterfaceIndex;
} MIB_NDIS_IFSTACK_KEY_ROW, *PMIB_NDIS_IFSTACK_KEY_ROW;

/**
 * The NDIS_NSI_INTERFACE_PURPOSE values describe why an interface exists (NDIS_NSI_INTERFACE_INFORMATION_RW.ifPurpose).
 */
typedef enum _NDIS_NSI_INTERFACE_PURPOSE
{
    NDIS_NSI_INTERFACE_PURPOSE_DEFAULT = 0,
    NDIS_NSI_INTERFACE_PURPOSE_INTERNAL_VSWITCH_NIC = 1,    // Host vNIC of a Hyper-V virtual switch.
    NDIS_NSI_INTERFACE_PURPOSE_SECONDARY_OR_ONDEMAND = 2,
    NDIS_NSI_INTERFACE_PURPOSE_IN_DISCONNECTED_STANDBY = 3,
    NDIS_NSI_INTERFACE_PURPOSE_IN_FAILOVER = 4,
    NDIS_NSI_INTERFACE_PURPOSE_MAX = 5
} NDIS_NSI_INTERFACE_PURPOSE;

/**
 * The NDIS_NSI_INTERFACE_INFORMATION_RW structure is the read/write parameter block of an NDIS interface
 * (NPI_MS_NDIS_MODULEID, NsiNdisObjectInterface; key: NET_LUID).
 * \remarks The ndis.sys symbol name is _NDIS_NSI_INTERFACE_INFORMATION_RW. 
 */
typedef struct _NDIS_NSI_INTERFACE_INFORMATION_RW
{
    NET_IF_NETWORK_GUID NetworkGuid;        // Network the interface is connected to.
    NET_IF_ADMIN_STATUS ifAdminStatus;
    IF_COUNTED_STRING ifAlias;              // Friendly name; Length is in bytes.
    IF_PHYSICAL_ADDRESS ifPhysAddress;      // Current physical address (read-only; ndis.sys rejects a set that changes it).
    IF_COUNTED_STRING ifL2NetworkInfo;      // Layer-2 network description.
    NDIS_NSI_INTERFACE_PURPOSE ifPurpose;
} NDIS_NSI_INTERFACE_INFORMATION_RW, *PNDIS_NSI_INTERFACE_INFORMATION_RW;

/**
 * The read-only dynamic parameter block of NsiNdisObjectInterface is the public NDIS_INTERFACE_INFORMATION (ifdef.h).
 */
typedef NDIS_INTERFACE_INFORMATION NDIS_NSI_INTERFACE_INFORMATION_ROD, *PNDIS_NSI_INTERFACE_INFORMATION_ROD;

/**
 * The NDIS_NSI_INTERFACE_INFORMATION_ROS structure is the read-only static parameter block of NsiNdisObjectInterface.
 */
typedef struct _NDIS_NSI_INTERFACE_INFORMATION_ROS
{
    NET_IFINDEX ifIndex;
    IF_COUNTED_STRING ifDescr;              // Description; Length is in bytes.
    USHORT ifType;                          // IFTYPE (IF_TYPE_*)
    USHORT Reserved;                        // alignment padding; nothing reads or writes it
    NET_IF_ACCESS_TYPE AccessType;
    NET_IF_DIRECTION_TYPE DirectionType;
    NET_IF_CONNECTION_TYPE ConnectionType;
    GUID InterfaceGuid;
    BOOLEAN ifConnectorPresent;
    UCHAR Reserved1;                        // alignment padding; nothing reads or writes it
    IF_PHYSICAL_ADDRESS PermanentPhysAddress;
    ULONG ifFlags;                          // NET_IF_INTERFACE_FLAGS
    ULONG MediaType;                        // NDIS_MEDIUM
    ULONG PhysicalMediumType;               // NDIS_PHYSICAL_MEDIUM
} NDIS_NSI_INTERFACE_INFORMATION_ROS, *PNDIS_NSI_INTERFACE_INFORMATION_ROS;

/**
 * The NDIS_NSI_INTERFACE_ENUM_ROD structure is the read-only dynamic row of NsiNdisObjectInterfaceWithoutStatistics
 * (key: NET_LUID); it omits the statistics that make NsiNdisObjectInterface expensive to enumerate.
 * \remarks The ndis.sys symbol name is _NDIS_NSI_INTERFACE_ENUM_ROD. 
 */
typedef struct _NDIS_NSI_INTERFACE_ENUM_ROD
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    NET_IF_NETWORK_GUID NetworkGuid;
    IF_COUNTED_STRING ifAlias;              // Friendly name; Length is in bytes.
    NET_IF_OPER_STATUS ifOperStatus;
    ULONG ifOperStatusFlags;                // NET_IF_OPER_STATUS_* reason bits
    ULONG ifMtu;
    IF_PHYSICAL_ADDRESS ifPhysAddress;
    IF_PHYSICAL_ADDRESS PermanentPhysAddress;
    NET_IF_ADMIN_STATUS ifAdminStatus;
    ULONG Reserved;                         // alignment padding; nothing reads or writes it
    ULONG64 XmitLinkSpeed;                  // bits per second
    ULONG64 RcvLinkSpeed;                   // bits per second
    BOOLEAN ifPromiscuousMode;
    BOOLEAN ifDeviceWakeUpEnable;
    BOOLEAN ifHideInterfaceInUi;
    UCHAR Reserved1;                        // alignment padding; nothing reads or writes it
    NET_IF_MEDIA_CONNECT_STATE MediaConnectState;
    NET_IF_MEDIA_DUPLEX_STATE MediaDuplexState;
    ULONG Reserved2;                        // alignment padding; nothing reads or writes it
} NDIS_NSI_INTERFACE_ENUM_ROD, *PNDIS_NSI_INTERFACE_ENUM_ROD;

/**
 * The NDIS_NSI_INTERFACE_ENUM_ROS structure is the read-only static row of NsiNdisObjectInterfaceWithoutStatistics.
 * \remarks The ndis.sys symbol name is _NDIS_NSI_INTERFACE_ENUM_ROS. 
 */
typedef struct _NDIS_NSI_INTERFACE_ENUM_ROS
{
    NET_IFINDEX ifIndex;
    IF_COUNTED_STRING ifDescr;              // Description; Length is in bytes.
    USHORT ifType;                          // IFTYPE (IF_TYPE_*)
    USHORT Reserved;                        // alignment padding; nothing reads or writes it
    NET_IF_ACCESS_TYPE AccessType;
    NET_IF_DIRECTION_TYPE DirectionType;
    NET_IF_CONNECTION_TYPE ConnectionType;
    GUID InterfaceGuid;
    BOOLEAN ifConnectorPresent;
    UCHAR Reserved1[3];                     // alignment padding; nothing reads or writes it
    ULONG ifFlags;                          // NET_IF_INTERFACE_FLAGS
    ULONG MediaType;                        // NDIS_MEDIUM
    ULONG PhysicalMediumType;               // NDIS_PHYSICAL_MEDIUM
} NDIS_NSI_INTERFACE_ENUM_ROS, *PNDIS_NSI_INTERFACE_ENUM_ROS;

#ifndef NDIS_OBJECT_TYPE_NSI_NETWORK_RW_STRUCT
#define NDIS_OBJECT_TYPE_NSI_NETWORK_RW_STRUCT 0xAC // ntddndis.h
#endif
#ifndef NDIS_OBJECT_TYPE_NSI_INTERFACE_PERSIST_RW_STRUCT
#define NDIS_OBJECT_TYPE_NSI_INTERFACE_PERSIST_RW_STRUCT 0xAE // ntddndis.h
#endif

#define NDIS_NSI_NETWORK_RW_REVISION_1 1

/**
 * The NDIS_NSI_NETWORK_RW structure is the read/write parameter block of an NDIS network (NPI_MS_NDIS_MODULEID,
 * NsiNdisObjectNetwork; key: NET_IF_NETWORK_GUID).
 * \remarks The ndis.sys symbol name is _NDIS_NSI_NETWORK_RW. 
 */
typedef struct _NDIS_NSI_NETWORK_RW
{
    NDIS_OBJECT_HEADER Header;              // Type = NDIS_OBJECT_TYPE_NSI_NETWORK_RW_STRUCT, Revision = NDIS_NSI_NETWORK_RW_REVISION_1, Size = sizeof(NDIS_NSI_NETWORK_RW).
    NET_IF_COMPARTMENT_ID CompartmentId;    // Compartment the network belongs to.
    ULONG SiteId;
    IF_COUNTED_STRING NetworkName;          // Length is in bytes.
} NDIS_NSI_NETWORK_RW, *PNDIS_NSI_NETWORK_RW;

/**
 * The NSI_NEIGHBOR_IPV4_ROW structure is a System Informer-defined row (not a Windows structure). netsup.c builds it
 * from the key, read/write and dynamic rows of the IPv4 neighbor table so callers get one flat record per ARP entry.
 */
typedef struct _NSI_NEIGHBOR_IPV4_ROW
{
    IN_ADDR Address;
    NET_LUID InterfaceLuid;
    NET_IFINDEX InterfaceIndex;
    UCHAR PhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
    ULONG PhysicalAddressLength;
    NET_IF_COMPARTMENT_ID CompartmentId;
    NL_NEIGHBOR_STATE State;
    UCHAR IsRouter : 1;
    UCHAR IsUnreachable : 1;
    UCHAR FlagsReserved : 6;
} NSI_NEIGHBOR_IPV4_ROW, *PNSI_NEIGHBOR_IPV4_ROW;

/**
 * The NSI_NEIGHBOR_IPV4_TABLE structure is a System Informer-defined table of NSI_NEIGHBOR_IPV4_ROW entries (not a
 * Windows structure).
 */
typedef struct _NSI_NEIGHBOR_IPV4_TABLE
{
    ULONG Count;
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    NSI_NEIGHBOR_IPV4_ROW Entries[ANYSIZE_ARRAY];
} NSI_NEIGHBOR_IPV4_TABLE, *PNSI_NEIGHBOR_IPV4_TABLE;

/**
 * The NSI_NEIGHBOR_IPV6_ROW structure is the System Informer-defined IPv6 counterpart of NSI_NEIGHBOR_IPV4_ROW.
 */
typedef struct _NSI_NEIGHBOR_IPV6_ROW
{
    IN6_ADDR Address;
    NET_LUID InterfaceLuid;
    NET_IFINDEX InterfaceIndex;
    UCHAR PhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
    ULONG PhysicalAddressLength;
    NET_IF_COMPARTMENT_ID CompartmentId;
    NL_NEIGHBOR_STATE State;
    UCHAR IsRouter : 1;
    UCHAR IsUnreachable : 1;
    UCHAR FlagsReserved : 6;
} NSI_NEIGHBOR_IPV6_ROW, *PNSI_NEIGHBOR_IPV6_ROW;

/**
 * The NSI_NEIGHBOR_IPV6_TABLE structure is a System Informer-defined table of NSI_NEIGHBOR_IPV6_ROW entries (not a
 * Windows structure).
 */
typedef struct _NSI_NEIGHBOR_IPV6_TABLE
{
    ULONG Count;
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    NSI_NEIGHBOR_IPV6_ROW Entries[ANYSIZE_ARRAY];
} NSI_NEIGHBOR_IPV6_TABLE, *PNSI_NEIGHBOR_IPV6_TABLE;

/**
 * The NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST structure is the input of NsiRegisterChangeNotificationEx. Netio.sys
 * calls ChangeCallback(CallerContext, ...) whenever an object of ObjectId is added, deleted or changed; with NotifyCurrent
 * set it is also called once per existing object. The handle returned in *NotificationHandle is passed to
 * NsiDeregisterChangeNotificationEx.
 */
typedef struct _NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST
{
    ULONG ModuleHandle;             // 0 = look the module up by ModuleId
    ULONG Reserved04;               // alignment padding; nothing reads or writes it
    PNPI_MODULEID ModuleId;
    NSI_OBJECT_ID ObjectId;
    ULONG Reserved14;               // alignment padding; nothing reads or writes it
    PVOID ChangeCallback;
    BOOLEAN NotifyCurrent;
    UCHAR Reserved21[7];            // alignment padding; nothing reads or writes it
    PVOID CallerContext;
    ULONG ScopeOrCompartment0;
    ULONG ScopeOrCompartment1;
    ULONG ProcessId;
    ULONG Reserved3C;               // alignment padding; nothing reads or writes it
    _Outptr_result_maybenull_ PVOID* NotificationHandle;
} NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST, *PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST;

/**
 * The NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST structure is the input of NsiRequestChangeNotificationEx: a one-shot
 * change notification for an object that completes the caller's OVERLAPPED.
 */
typedef struct _NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST
{
    ULONG ModuleHandle;             // 0 = look the module up by ModuleId
    PNPI_MODULEID ModuleId;
    ULONG ObjectIndex;
    LPOVERLAPPED Overlapped;
    PHANDLE NotificationHandle;
} NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST, *PNSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST;

/**
 * The NSI_SET_ALL_PARAMETERS_EX_REQUEST structure is the input of NsiSetAllParametersEx. It writes the whole
 * read/write block of one object; Header.SetAction selects set, create or delete.
 */
typedef struct _NSI_SET_ALL_PARAMETERS_EX_REQUEST
{
    NSI_EX_REQUEST_COMMON Header;   // (Header.Action = NSI_SET_ACTION)
    PVOID Key;
    ULONG KeySize;
    ULONG Reserved34;               // alignment padding; nothing reads or writes it
    PVOID ReadWriteBuffer;
    ULONG ReadWriteBufferSize;
    ULONG Reserved44;               // alignment padding; nothing reads or writes it
} NSI_SET_ALL_PARAMETERS_EX_REQUEST, *PNSI_SET_ALL_PARAMETERS_EX_REQUEST;

/**
 * The NSI_SET_PARAMETER_EX_REQUEST structure is the input of NsiSetParameterEx. It writes ParameterBufferSize bytes at
 * ParameterOffset of one block of one object.
 */
typedef struct _NSI_SET_PARAMETER_EX_REQUEST
{
    NSI_EX_REQUEST_COMMON Header;   // (Header.Action = NSI_SET_ACTION)
    PVOID Key;
    ULONG KeySize;
    ULONG Reserved34;               // alignment padding; nothing reads or writes it
    NSI_PARAMETER_TYPE ParameterType;
    ULONG Reserved3C;               // alignment padding; nothing reads or writes it
    PVOID ParameterBuffer;
    ULONG ParameterBufferSize;      // in: size of ParameterBuffer; out: overwritten with the IOCTL bytes-returned count
    ULONG ParameterOffset;          // byte offset of the field within the ParameterType row
} NSI_SET_PARAMETER_EX_REQUEST, *PNSI_SET_PARAMETER_EX_REQUEST;

/**
 * The NSI_TCP_IPV4_ROW structure is a System Informer-defined row (not a Windows structure). netsup.c builds it from the
 * NSI_TCP_KEY_ROW, NSI_TCP_STATUS_ROW and NSI_TCP_OWNER_ROW of one IPv4 TCP endpoint.
 */
typedef struct _NSI_TCP_IPV4_ROW
{
    MIB_TCP_STATE State;
    IN_ADDR LocalAddress;
    USHORT LocalPort;
    UCHAR Reserved1[2];             // alignment padding; nothing reads or writes it
    IN_ADDR RemoteAddress;
    USHORT RemotePort;
    UCHAR Reserved2[2];             // alignment padding; nothing reads or writes it
    NSI_TCP_APPLIED_SETTING AppliedSetting;
    ULONG OwningPid;
    UCHAR Reserved3[4];             // alignment padding; nothing reads or writes it
    LARGE_INTEGER CreateTimestamp;
    TCP_CONNECTION_OFFLOAD_STATE OffloadState;
    UCHAR Reserved4[4];             // alignment padding; nothing reads or writes it
} NSI_TCP_IPV4_ROW, *PNSI_TCP_IPV4_ROW;

/**
 * The NSI_TCP_IPV4_TABLE structure is a System Informer-defined table of NSI_TCP_IPV4_ROW entries (not a Windows structure).
 */
typedef struct _NSI_TCP_IPV4_TABLE
{
    ULONG Count;
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    NSI_TCP_IPV4_ROW Entries[ANYSIZE_ARRAY];
} NSI_TCP_IPV4_TABLE, *PNSI_TCP_IPV4_TABLE;

/**
 * The NSI_TCP_IPV6_ROW structure is the System Informer-defined IPv6 counterpart of NSI_TCP_IPV4_ROW.
 */
typedef struct _NSI_TCP_IPV6_ROW
{
    IN6_ADDR LocalAddress;
    SCOPE_ID LocalScopeId;
    USHORT LocalPort;
    UCHAR Reserved1[2];             // alignment padding; nothing reads or writes it
    IN6_ADDR RemoteAddress;
    SCOPE_ID RemoteScopeId;
    USHORT RemotePort;
    UCHAR Reserved2[2];             // alignment padding; nothing reads or writes it
    MIB_TCP_STATE State;
    NSI_TCP_APPLIED_SETTING AppliedSetting;
    ULONG OwningPid;
    UCHAR Reserved3[4];             // alignment padding; nothing reads or writes it
    LARGE_INTEGER CreateTimestamp;
    TCP_CONNECTION_OFFLOAD_STATE OffloadState;
    UCHAR Reserved4[4];             // alignment padding; nothing reads or writes it
} NSI_TCP_IPV6_ROW, *PNSI_TCP_IPV6_ROW;

/**
 * The NSI_TCP_IPV6_TABLE structure is a System Informer-defined table of NSI_TCP_IPV6_ROW entries (not a Windows structure).
 */
typedef struct _NSI_TCP_IPV6_TABLE
{
    ULONG Count;
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    NSI_TCP_IPV6_ROW Entries[ANYSIZE_ARRAY];
} NSI_TCP_IPV6_TABLE, *PNSI_TCP_IPV6_TABLE;

/**
 * The NSI_TCP_KEY_ROW structure is the key row of a TCP endpoint returned by the TCP connection and listener
 * enumerations (NPI_MS_TCP_MODULEID): the local and remote endpoints as SOCKADDR_INET values (remote is zero for
 * listeners). IPv4 endpoints use the SOCKADDR_IN view.
 */
typedef struct _NSI_TCP_KEY_ROW
{
    SOCKADDR_INET LocalAddress;
    SOCKADDR_INET RemoteAddress;
} NSI_TCP_KEY_ROW, *PNSI_TCP_KEY_ROW;

/**
 * The NSI_TCP_OWNER_ROW structure is the read-only static row returned for each TCP endpoint by the TCP connection
 * enumerations. It reports the owning process and the TCP options that were negotiated during the handshake.
 * \remarks  The option fields are 0 for listeners and TIME_WAIT entries; for SYN_RCVD entries
 * they come from the half-open SYN TCB.
 */
typedef struct _NSI_TCP_OWNER_ROW
{
    BOOLEAN SackEnabled;            // SACK-permitted was negotiated (RFC 2018); the TCB emits SACK blocks.
    BOOLEAN TimestampsEnabled;      // TCP timestamps were negotiated (RFC 7323); also set by the TCP_TIMESTAMPS socket option.
    BOOLEAN EcnEnabled;             // ECN was negotiated (RFC 3168); the TCB sets ECE/CWR on outgoing segments.
    BOOLEAN DualMode;               // listener: IPv6 wildcard socket that also accepts IPv4 (IP Helper lists it as 0.0.0.0 in the IPv4 table)
    ULONG SendWindowScale;          // Window-scale shift advertised by the peer (0-14); applied to the peer's receive window.
    ULONG ReceiveWindowScale;       // Window-scale shift this host advertised; applied to the local receive window.
    ULONG OwningPid;
    LARGE_INTEGER CreateTimestamp;
    ULONGLONG OwningModuleInfo;
} NSI_TCP_OWNER_ROW, *PNSI_TCP_OWNER_ROW;

/**
 * The NSI_TCP_STATUS_ROW structure is the read-only dynamic row returned for each TCP endpoint by the TCP connection
 * enumerations. IP Helper GetTcpTable2 copies State and OffloadState into MIB_TCPROW2.
 */
typedef struct _NSI_TCP_STATUS_ROW
{
    MIB_TCP_STATE State;
    BOOLEAN NagleEnabled;           // TRUE unless TCP_NODELAY is set on the endpoint (always FALSE for SYN_RCVD and TIME_WAIT entries).
    UCHAR Reserved1[3];             // alignment padding; nothing reads or writes it
    TCP_CONNECTION_OFFLOAD_STATE OffloadState;
    NSI_TCP_APPLIED_SETTING AppliedSetting; // listeners report 6, which has no NSI_TCP_APPLIED_SETTING name.
    ULONG ConnectionDuration;       // Milliseconds since the TCB was created (enumeration only; a single-object get leaves it 0).
    UCHAR QosFlowTag;
    UCHAR Reserved2[3];             // alignment padding; nothing reads or writes it
} NSI_TCP_STATUS_ROW, *PNSI_TCP_STATUS_ROW;

/**
 * The NSI_UDP_IPV4_ROW structure is a System Informer-defined row (not a Windows structure). netsup.c builds it from the
 * NSI_UDP_KEY_ROW and NSI_UDP_OWNER_ROW of one IPv4 UDP endpoint.
 */
typedef struct _NSI_UDP_IPV4_ROW
{
    IN_ADDR LocalAddress;
    USHORT LocalPort;
    UCHAR Reserved1[2];             // alignment padding; nothing reads or writes it
    ULONG OwningPid;
    UCHAR Reserved2[4];             // alignment padding; nothing reads or writes it
    LARGE_INTEGER CreateTimestamp;
} NSI_UDP_IPV4_ROW, *PNSI_UDP_IPV4_ROW;

/**
 * The NSI_UDP_IPV4_TABLE structure is a System Informer-defined table of NSI_UDP_IPV4_ROW entries (not a Windows structure).
 */
typedef struct _NSI_UDP_IPV4_TABLE
{
    ULONG Count;
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    NSI_UDP_IPV4_ROW Entries[ANYSIZE_ARRAY];
} NSI_UDP_IPV4_TABLE, *PNSI_UDP_IPV4_TABLE;

/**
 * The NSI_UDP_IPV6_ROW structure is the System Informer-defined IPv6 counterpart of NSI_UDP_IPV4_ROW.
 */
typedef struct _NSI_UDP_IPV6_ROW
{
    IN6_ADDR LocalAddress;
    SCOPE_ID LocalScopeId;
    USHORT LocalPort;
    UCHAR Reserved1[2];             // alignment padding; nothing reads or writes it
    ULONG OwningPid;
    UCHAR Reserved2[4];             // alignment padding; nothing reads or writes it
    LARGE_INTEGER CreateTimestamp;
} NSI_UDP_IPV6_ROW, *PNSI_UDP_IPV6_ROW;

/**
 * The NSI_UDP_IPV6_TABLE structure is a System Informer-defined table of NSI_UDP_IPV6_ROW entries (not a Windows structure).
 */
typedef struct _NSI_UDP_IPV6_TABLE
{
    ULONG Count;
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    NSI_UDP_IPV6_ROW Entries[ANYSIZE_ARRAY];
} NSI_UDP_IPV6_TABLE, *PNSI_UDP_IPV6_TABLE;

/**
 * The NSI_UDP_KEY_ROW structure is the key row of a UDP endpoint returned by the UDP endpoint enumeration
 * (NPI_MS_UDP_MODULEID): the local endpoint as a SOCKADDR_INET value.
 * \remarks  For AF_INET endpoints tcpip writes the IPv4
 *          address at offset 4.
 */
typedef struct _NSI_UDP_KEY_ROW
{
    ADDRESS_FAMILY AddressFamily;
    USHORT LocalPort;
    IN_ADDR LocalAddressV4;
    IN6_ADDR LocalAddressV6;
    SCOPE_ID LocalScopeId;
} NSI_UDP_KEY_ROW, *PNSI_UDP_KEY_ROW;

/**
 * The NSI_UDP_OWNER_FLAGS bits are stored in NSI_UDP_OWNER_ROW.Flags; IP Helper copies them into MIB_UDPROW_OWNER_MODULE.dwFlags.
 */
typedef enum _NSI_UDP_OWNER_FLAGS
{
    NsiUdpOwnerFlagSpecificPortBind = 0x00000001 // The socket was bound to a specific port (MIB_UDPROW_OWNER_MODULE SpecificPortBind).
} NSI_UDP_OWNER_FLAGS;

/**
 * The NSI_UDP_OWNER_ROW structure is the read-only static row of a UDP endpoint: owning process, creation time, flags
 * and the owning-module cookie used by GetOwnerModuleFromUdpEntry. IP Helper converts it to MIB_UDPROW_OWNER_MODULE.
 */
typedef struct _NSI_UDP_OWNER_ROW
{
    ULONG OwningPid;
    BOOLEAN DualMode;               // IPv6 wildcard socket that also accepts IPv4 (IP Helper lists it as 0.0.0.0 in the IPv4 table)
    UCHAR Reserved05[3];            // alignment padding; nothing reads or writes it
    LARGE_INTEGER CreateTimestamp;
    union
    {
        ULONG Flags;                // NSI_UDP_OWNER_FLAGS
        struct
        {
            ULONG SpecificPortBind : 1;
            ULONG ReservedFlags : 31;   // unused bits; tcpip writes the whole ULONG (0 or SpecificPortBind) and nothing reads them
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    ULONGLONG OwningModuleInfo;
} NSI_UDP_OWNER_ROW, *PNSI_UDP_OWNER_ROW;

/**
 * The NETIO_RECOVERED_128 union is a 16-byte untyped block. It is used where only the size of a value is known, so the
 * bytes can be copied and without assuming a layout.
 */
typedef union _NETIO_RECOVERED_128
{
    UCHAR Byte[16];
    USHORT Word[8];
    ULONG Dword[4];
    ULONGLONG Qword[2];
} NETIO_RECOVERED_128, *PNETIO_RECOVERED_128;

/**
 * The NETIO_TCP_STATISTICS_EX2 structure is the output of PhGetTcpStatisticsEx2 and has the layout of MIB_TCPSTATS2:
 * the 16-byte RTO/connection-limit block (NSI_TCP_GLOBAL_ROS without its last field) followed by the counters from
 * NETIO_TCP_STATS_DYNAMIC.
 */
typedef struct _NETIO_TCP_STATISTICS_EX2
{
    NETIO_RECOVERED_128 Header;
    ULONG ActiveOpens;
    ULONG PassiveOpens;
    ULONG AttemptFails;
    ULONG EstabResets;
    ULONG CurrEstab;
    ULONG Reserved24;               // alignment padding; nothing reads or writes it
    LONG64 InSegs;
    LONG64 OutSegs;
    ULONG RetransSegs;
    ULONG OutRsts;
    ULONG InErrs;
    ULONG NumConns;
} NETIO_TCP_STATISTICS_EX2, *PNETIO_TCP_STATISTICS_EX2;

/**
 * The NETIO_UDP_STATISTICS_EX2 structure is a System Informer-defined 32-byte UDP statistics buffer made of two
 * untyped 16-byte blocks. Prefer NETIO_UDP_STATS_DYNAMIC, which has the real field layout.
 */
typedef struct _NETIO_UDP_STATISTICS_EX2
{
    NETIO_RECOVERED_128 Header;
    NETIO_RECOVERED_128 Dynamic;
} NETIO_UDP_STATISTICS_EX2, *PNETIO_UDP_STATISTICS_EX2;

/**
 * The NETIO_CONNECTIVITY_HINT structure is the network connectivity hint passed to
 * PNET_NETWORK_CONNECTIVITY_HINT_CHANGE_CALLBACK. It has the layout of NL_NETWORK_CONNECTIVITY_HINT
 * (GetNetworkConnectivityHint): the connectivity level and the cost of the current network.
 */
typedef struct _NETIO_CONNECTIVITY_HINT
{
    LONG64 ConnectivityLevel;
    ULONG ConnectivityCost;
} NETIO_CONNECTIVITY_HINT, *PNETIO_CONNECTIVITY_HINT;

/**
 * The NETIO_INTERFACE_HARDWARE_CROSSTIMESTAMP structure is a correlated pair of system and NIC hardware clock readings,
 * with the hardware clock frequency, so packet timestamps from the adapter can be converted to system time. It has the
 * layout of INTERFACE_HARDWARE_CROSSTIMESTAMP (CaptureInterfaceHardwareCrossTimestamp).
 */
typedef struct _NETIO_INTERFACE_HARDWARE_CROSSTIMESTAMP
{
    LONG64 SystemTimestamp;
    LONG64 HardwareClockFrequency;
    LONG64 HardwareTimestamp;
} NETIO_INTERFACE_HARDWARE_CROSSTIMESTAMP, *PNETIO_INTERFACE_HARDWARE_CROSSTIMESTAMP;

typedef VOID (NTAPI *PNET_NETWORK_CONNECTIVITY_HINT_CHANGE_CALLBACK)(
    _In_opt_ PVOID CallerContext,
    _In_ PNETIO_CONNECTIVITY_HINT ConnectivityHint
    );

/**
 * The NETIO_CHANGE_CALLBACK_CONTEXT structure is the per-module context IP Helper passes to NsiRpcRegisterChangeNotification
 * for the Notify*Change2 routines; its NSI callback converts the NSI row and calls Callback(CallerContext, Row, Type).
 */
typedef struct _NETIO_CHANGE_CALLBACK_CONTEXT
{
    ADDRESS_FAMILY Family;          // Family reported to the caller (AF_INET for the IPv4 module, AF_INET6 for the IPv6 module).
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    PVOID Callback;                 // Caller's callback.
    PVOID CallerContext;
} NETIO_CHANGE_CALLBACK_CONTEXT, *PNETIO_CHANGE_CALLBACK_CONTEXT;

/**
 * The NETIO_CHANGE_REGISTRATION_HEADER structure starts every notification handle returned by the IP Helper Notify*Change
 * routines. CancelMibChangeNotify2 deregisters each non-NULL module handle and then frees the block.
 */
typedef struct _NETIO_CHANGE_REGISTRATION_HEADER
{
    PVOID Reserved;                         // nothing reads or writes it (IP Helper's register, callback and cancel paths start at ObjectId)
    ULONG ObjectId;                         // NSI object the registration watches.
    ULONG Reserved1;                        // alignment padding; nothing reads or writes it
    PVOID Ipv4NotificationHandle;           // NsiRpcRegisterChangeNotification handle for NPI_MS_IPV4_MODULEID.
    PVOID Ipv6NotificationHandle;           // Handle for NPI_MS_IPV6_MODULEID.
    PVOID TeredoNotificationHandle;         // Handle for NPI_MS_FL6T_MODULEID.
    PVOID NdisNotificationHandle;           // Handle for NPI_MS_NDIS_MODULEID.
    PVOID ConnectivityHintRegistration;     // Non-NULL for NotifyNetworkConnectivityHintChange registrations.
    PVOID Ipv4CallbackContext;              // Context registered with the IPv4 (or only) module.
    PVOID Ipv6CallbackContext;              // Context registered with the IPv6 module.
} NETIO_CHANGE_REGISTRATION_HEADER, *PNETIO_CHANGE_REGISTRATION_HEADER;

/**
 * The NETIO_COMPARTMENT_CHANGE_ROW structure is the row IP Helper passes to a NotifyCompartmentChange callback.
 */
typedef struct _NETIO_COMPARTMENT_CHANGE_ROW
{
    GUID CompartmentGuid;                   // Copied from the NDIS_NSI_COMPARTMENT_RW of add and parameter-change notifications.
    NET_IF_COMPARTMENT_ID CompartmentId;    // Key of the notification.
    UCHAR Reserved[0x208];                  // nothing reads or writes it (IP Helper zeroes the row and never fills this part)
} NETIO_COMPARTMENT_CHANGE_ROW, *PNETIO_COMPARTMENT_CHANGE_ROW;

/**
 * The NETIO_NDIS_COMPARTMENT_CHANGE_REGISTRATION structure is the 0x278-byte notification handle IP Helper's
 * NotifyCompartmentChange allocates. NetioCompartmentChangeCallBack fills Row and calls Callback(CallerContext, &Row, Type).
 */
typedef struct _NETIO_NDIS_COMPARTMENT_CHANGE_REGISTRATION
{
    NETIO_CHANGE_REGISTRATION_HEADER Header; // ObjectId = NsiNdisObjectCompartment; Ipv4CallbackContext = &Callback
    PVOID Callback;
    PVOID CallerContext;
    NETIO_COMPARTMENT_CHANGE_ROW Row;
} NETIO_NDIS_COMPARTMENT_CHANGE_REGISTRATION, *PNETIO_NDIS_COMPARTMENT_CHANGE_REGISTRATION;

/**
 * The NETIO_IP_INTERFACE_CHANGE_REGISTRATION structure is the 0x1C8-byte notification handle NotifyIpInterfaceChange
 * allocates, with one callback context and MIB_IPINTERFACE_ROW buffer per address family.
 */
typedef struct _NETIO_IP_INTERFACE_CHANGE_REGISTRATION
{
    NETIO_CHANGE_REGISTRATION_HEADER Header; // ObjectId = NsiIpObjectIpInterface
    NETIO_CHANGE_CALLBACK_CONTEXT Ipv4Callback; // Header.Ipv4CallbackContext points here.
    MIB_IPINTERFACE_ROW Ipv4Row;            // Row filled before the IPv4 callback is called.
    NETIO_CHANGE_CALLBACK_CONTEXT Ipv6Callback; // Header.Ipv6CallbackContext points here.
    MIB_IPINTERFACE_ROW Ipv6Row;
} NETIO_IP_INTERFACE_CHANGE_REGISTRATION, *PNETIO_IP_INTERFACE_CHANGE_REGISTRATION;

/**
 * The NETIO_ROUTE_CHANGE_REGISTRATION structure is the 0x148-byte notification handle NotifyRouteChange2 allocates,
 * with one callback context and MIB_IPFORWARD_ROW2 buffer per address family.
 */
typedef struct _NETIO_ROUTE_CHANGE_REGISTRATION
{
    NETIO_CHANGE_REGISTRATION_HEADER Header; // ObjectId = NsiIpObjectRoute
    NETIO_CHANGE_CALLBACK_CONTEXT Ipv4Callback; // Header.Ipv4CallbackContext points here.
    MIB_IPFORWARD_ROW2 Ipv4Row;             // Row NetioRouteChangeCallBack fills before calling the IPv4 callback.
    NETIO_CHANGE_CALLBACK_CONTEXT Ipv6Callback; // Header.Ipv6CallbackContext points here.
    MIB_IPFORWARD_ROW2 Ipv6Row;
} NETIO_ROUTE_CHANGE_REGISTRATION, *PNETIO_ROUTE_CHANGE_REGISTRATION;

/**
 * The NETIO_TEREDO_PORT_CHANGE_REGISTRATION structure is the 0x58-byte notification handle NotifyTeredoPortChange
 * allocates; it is registered with the Teredo framing-layer module (NPI_MS_FL6T_MODULEID).
 */
typedef struct _NETIO_TEREDO_PORT_CHANGE_REGISTRATION
{
    NETIO_CHANGE_REGISTRATION_HEADER Header; // ObjectId = 3 (NPI_MS_FL6T_MODULEID); Ipv4CallbackContext = &Callback
    PTEREDO_PORT_CHANGE_CALLBACK Callback;
    PVOID CallerContext;
} NETIO_TEREDO_PORT_CHANGE_REGISTRATION, *PNETIO_TEREDO_PORT_CHANGE_REGISTRATION;

/**
 * The TCP_KEY_ROW structure is the IPv6 view of NSI_TCP_KEY_ROW (two SOCKADDR_INET values).
 */
typedef struct _TCP_KEY_ROW
{
    USHORT Family;                  // Local address family.
    USHORT LocalPort;
    IN_ADDR LocalAddressV4;         // AF_INET: the IPv4 address (SOCKADDR_IN.sin_addr); AF_INET6: sin6_flowinfo (0).
    IN6_ADDR LocalAddress;
    SCOPE_ID LocalScopeId;
    USHORT RemoteFamily;            // Remote address family.
    USHORT RemotePort;
    IN_ADDR RemoteAddressV4;        // AF_INET: the IPv4 address; AF_INET6: sin6_flowinfo (0).
    IN6_ADDR RemoteAddress;
    SCOPE_ID RemoteScopeId;
} TCP_KEY_ROW, *PTCP_KEY_ROW;

/**
 * The UDP_KEY_ROW structure is the IPv6 view of NSI_UDP_KEY_ROW (a SOCKADDR_INET value).
 */
typedef struct _UDP_KEY_ROW
{
    USHORT Family;
    USHORT Port;
    IN_ADDR AddressV4;              // AF_INET: the IPv4 address (tcpip!UdpEnumerateEndpoints); AF_INET6: 0.
    IN6_ADDR Address;
    SCOPE_ID ScopeId;
} UDP_KEY_ROW, *PUDP_KEY_ROW;

/**
 * The NSI_FL_VIRTUAL_INTERFACE_KEY structure is the key of a framing-layer virtual interface (NPI_MS_FL48_MODULEID or
 * NPI_MS_FL68_MODULEID, NsiFlObjectVirtualInterface). IP Helper uses it for Get/Set/Create/DeleteFlVirtualInterface.
 */
typedef struct _NSI_FL_VIRTUAL_INTERFACE_KEY
{
    NET_LUID InterfaceLuid;                 // Framing-layer (physical) interface.
    GUID CompartmentGuid;                   // Get: compartment of the virtual interface; create: compartment to put it in.
    ULONG VirtualInterfaceId;               // 0 selects the interface's default client interface.
    ULONG IsolationMode;                    // 0..2 (get: from the NDIS isolation mode; set: values above 2 are rejected).
} NSI_FL_VIRTUAL_INTERFACE_KEY, *PNSI_FL_VIRTUAL_INTERFACE_KEY;

/**
 * The NSI_FL_VIRTUAL_INTERFACE_RW structure is the read/write block of a framing-layer virtual interface
 * (NsiFlObjectVirtualInterface). CreateFlVirtualInterface and SetFlVirtualInterface write it.
 */
typedef struct _NSI_FL_VIRTUAL_INTERFACE_RW
{
    BOOLEAN ClientInterfaceFlag;            // Best-effort name: flag 0x02 of the client interface; 0xFF leaves it unchanged on a set.
} NSI_FL_VIRTUAL_INTERFACE_RW, *PNSI_FL_VIRTUAL_INTERFACE_RW;

/**
 * The NSI_FL_VIRTUAL_INTERFACE_ROD structure is the read-only dynamic block of a framing-layer virtual interface: its
 * index and the per-processor statistics of its client interface summed over all processors.
 */
typedef struct _NSI_FL_VIRTUAL_INTERFACE_ROD
{
    ULONG VirtualInterfaceIndex;            // Best-effort name: index FlIfCreateVirtualInterface returned for the virtual interface.
    ULONG Counters32[2];                    // Sums of the client interface's per-processor statistics (+0x00, +0x04);
                                            //      IP Helper copies them verbatim; individual meanings not identified.
    ULONG Reserved;                         // alignment padding; nothing reads or writes it
    ULONG64 Counters64[15];                 // Sums of the per-processor statistics (+0x08..+0x78); copied verbatim by IP Helper.
} NSI_FL_VIRTUAL_INTERFACE_ROD, *PNSI_FL_VIRTUAL_INTERFACE_ROD;

/**
 * The NSI_FL_VIRTUAL_INTERFACE_ROS structure is the read-only static block of a framing-layer virtual interface.
 */
typedef struct _NSI_FL_VIRTUAL_INTERFACE_ROS
{
    NET_LUID VirtualInterfaceLuid;          // Best-effort name: LUID FlIfCreateVirtualInterface returned for the virtual interface.
    ULONG VirtualInterfaceType;             // Best-effort name: 2 for the default client interface.
    ULONG Reserved;                         // alignment padding; nothing reads or writes it
} NSI_FL_VIRTUAL_INTERFACE_ROS, *PNSI_FL_VIRTUAL_INTERFACE_ROS;

/**
 * The NSI_IP_STATISTICS_ROD structure is the read-only dynamic parameter block of the IP global object
 * (NPI_MS_IPV4_MODULEID or NPI_MS_IPV6_MODULEID, NsiIpObjectGlobal; no key). Tcpip sums its per-processor IP statistics
 * into it. The read-only static block is a ULONG (MIB_IPSTATS.dwReasmTimeout, seconds).
 * \remarks  The counters follow the RFC 4293 ipSystemStatsTable
 *          order: the 64-bit block at 0x10 is ipSystemStatsHCInReceives..HCOutBcastPkts, and the writers checked
 *          (IppReceiveHeaderBatch, IppPacketizeDatagrams, IppReceiveUdpEspInternal, IppAddUdpEspHeader) match. When the CLAT
 *          DHCP option 108 feature is enabled tcpip appends a BOOLEAN (IPv6-only capable, RFC 8925) at 0xD0 if the
 *          buffer is at least 0xD8 bytes.
 */
typedef struct _NSI_IP_STATISTICS_ROD
{
    ULONG NumCompartments;          // netsh "Number of Compartments" (IppInsertCompartment/IppRemoveCompartment).
    ULONG NumNlClients;             // netsh "Number of NL clients" (attached transport-layer clients).
    ULONG NumFlProviders;           // netsh "Number of FL providers" (Ipv4/Ipv6AttachFlProvider).
    ULONG NumInterfaces;            // IP interfaces (IppCreateInterface/IpFlcDeleteInterface). IP Helper does not export it.
    ULONG64 InReceives;             // MIB_IPSTATS.dwInReceives
    ULONG64 InOctets;               // RFC 4293 ipSystemStatsHCInOctets
    ULONG64 ForwDatagrams;          // MIB_IPSTATS.dwForwDatagrams
    ULONG64 InDelivers;             // MIB_IPSTATS.dwInDelivers
    ULONG64 OutRequests;            // MIB_IPSTATS.dwOutRequests
    ULONG64 OutForwDatagrams;       // RFC 4293 ipSystemStatsHCOutForwDatagrams
    ULONG64 OutTransmits;           // RFC 4293 ipSystemStatsHCOutTransmits
    ULONG64 OutOctets;              // RFC 4293 ipSystemStatsHCOutOctets
    ULONG64 InMcastPkts;            // RFC 4293 ipSystemStatsHCInMcastPkts
    ULONG64 InMcastOctets;          // RFC 4293 ipSystemStatsHCInMcastOctets
    ULONG64 OutMcastPkts;           // RFC 4293 ipSystemStatsHCOutMcastPkts
    ULONG64 OutMcastOctets;         // RFC 4293 ipSystemStatsHCOutMcastOctets
    ULONG64 InBcastPkts;            // RFC 4293 ipSystemStatsHCInBcastPkts
    ULONG64 OutBcastPkts;           // RFC 4293 ipSystemStatsHCOutBcastPkts
    ULONG InHdrErrors;              // MIB_IPSTATS.dwInHdrErrors
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    ULONG InAddrErrors;             // MIB_IPSTATS.dwInAddrErrors
    ULONG InUnknownProtos;          // MIB_IPSTATS.dwInUnknownProtos
    ULONG InTruncatedPkts;          // RFC 4293 ipSystemStatsInTruncatedPkts. IP Helper does not export it.
    ULONG ReasmReqds;               // MIB_IPSTATS.dwReasmReqds
    ULONG ReasmOks;                 // MIB_IPSTATS.dwReasmOks
    ULONG ReasmFails;               // MIB_IPSTATS.dwReasmFails
    ULONG InDiscards;               // MIB_IPSTATS.dwInDiscards
    ULONG OutNoRoutes;              // MIB_IPSTATS.dwOutNoRoutes
    ULONG OutDiscards;              // MIB_IPSTATS.dwOutDiscards
    ULONG FragReqds;                // FragOks + FragFails (RFC 4293 ipSystemStatsOutFragReqds); IP Helper does not export it.
    ULONG FragOks;                  // MIB_IPSTATS.dwFragOks
    ULONG FragFails;                // MIB_IPSTATS.dwFragFails
    ULONG FragCreates;              // MIB_IPSTATS.dwFragCreates
    ULONG RoutingDiscards;          // MIB_IPSTATS.dwRoutingDiscards
    ULONG TemporaryRandomTime;      // Seconds. Random offset currently subtracted from the temporary (privacy) address
                                    //      preferred lifetime; picked in 0..NSI_IP_GLOBAL_RW.MaxTemporaryRandomTime.
    ULONG InUdpEspPackets;          // UDP-encapsulated ESP packets received (RFC 3948; IppReceiveUdpEspInternal).
    ULONG OutUdpEspPackets;         // UDP-encapsulated ESP packets sent (IppAddUdpEspHeader).
    ULONG Reserved1;                // alignment padding; nothing reads or writes it
} NSI_IP_STATISTICS_ROD, *PNSI_IP_STATISTICS_ROD;

/**
 * The NSI_IP_GLOBAL_RW structure is the read/write parameter block of the IP global object (NPI_MS_IPV4_MODULEID or
 * NPI_MS_IPV6_MODULEID, NsiIpObjectGlobal). It holds the "netsh interface ipv4|ipv6 set global" and "set privacy"
 * settings (MSFT_NetIPv4Protocol / MSFT_NetIPv6Protocol in WMI). On a set, 0xFF (BOOLEAN/UCHAR), 0xFFFF (USHORT) or
 * 0xFFFFFFFF (ULONG) leaves a field unchanged. Times are in seconds; tcpip stores them as 500 ms ticks.
 * \remarks  Names in quotes are the netsh
 *          labels/tokens; fields netsh and WMI do not expose have best-effort names from their tcpip readers.
 */
typedef struct _NSI_IP_GLOBAL_RW
{
    ULONG UseTemporaryAddresses;    // netsh privacy "state" (RFC 4941 temporary addresses). IPv4 accepts only 0. Default 1.
    ULONG MaxTemporaryRandomTime;   // netsh privacy "maxrandomtime" (WMI MaxTemporaryDesyncTime). Default 600.
    ULONG MaxTemporaryValidLifetime; // netsh privacy "maxvalidlifetime". Default 604800.
    ULONG MaxTemporaryPreferredLifetime; // netsh privacy "maxpreferredlifetime"; at most MaxTemporaryValidLifetime. Default 86400.
    ULONG MaxTemporaryDadAttempts;  // netsh privacy "maxdadattempts" (IppDadFailed, IppRestartDad). Default 3.
    ULONG TemporaryRegenerateTime;  // netsh privacy "regeneratetime"; TemporaryRegenerateTime + MaxTemporaryRandomTime must be
                                    //      below MaxTemporaryPreferredLifetime. Default 5.
    ULONG DefaultHopLimit;          // "defaultcurhoplimit", 1..255 (stored as a UCHAR). MIB_IPSTATS.dwDefaultTTL when the
                                    //      compartment has none.
    ULONG NeighborCacheLimit;       // "neighborcachelimit", entries per interface.
    ULONG RouteCacheLimit;          // "routecachelimit", entries per compartment.
    ULONG ReassemblyLimit;          // "reassemblylimit", bytes.
    ULONG Reserved28;               // alignment padding; nothing reads or writes it
    BOOLEAN RetiredSetting2C;       // Retired setting: IppValidateGlobalRwParameters rejects any value but 0xFF and a get returns 0.
    UCHAR Reserved2D[3];            // alignment padding; nothing reads or writes it
    ULONG DeadRouteProbeTimeout;    // Seconds a dead-gateway-detection probe state lasts before the route returns to dead
                                    //      (IppGetRouteState). Best-effort name.
    ULONG DeadRouteTimeout;         // Seconds a route stays dead before it is probed again (IppGetRouteState). Best-effort name.
    ULONG PathUnreachabilityWindow; // Seconds within which PathUnreachabilityThreshold indications must arrive before a
                                    //      path's route is suspected (IpNlpSuspectPathReachability). Best-effort name.
    ULONG DeadRouteProbePercent;    // 1..100 (stored as a UCHAR): share of traffic sent to a probed dead route
                                    //      (IppIsRouteDead). Best-effort name.
    ULONG DefaultLinkLocalAddressBehavior; // NL_LINK_LOCAL_ADDRESS_BEHAVIOR used for new interfaces whose framing layer
                                    //      supports it (IpFlcAddInterface). IPv6 accepts only 2 (always on).
    ULONG LinkLayerSuffixPrefix[4]; // IPv4 only, write-only (a get returns 0). IN_ADDR in the first ULONG: prefix for addresses
                                    //      formed from the link-layer suffix (Ipv4pAddLinkLayerSuffixAddresses); must be unicast.
    ULONG LinkLayerSuffixPrefixLength; // IPv4 only, write-only: 1..32.
    ULONG SourceRoutingBehavior;    // "sourceroutingbehavior" (0 = forward, 1 = dontforward, 2 = drop).
    ULONG MldLevel;                 // "mldlevel" (0 = none, 1 = sendonly, 2 = all).
    ULONG MldVersion;               // "mldversion": 2 = version1 (IPv4 only), 3 = version2, 4 = version3.
    ULONG DefaultDadTransmits;      // 0..3: duplicate address detection transmits for new interfaces (IpFlcAddInterface);
                                    //      0xFFFFFFFF uses the framing layer's value. Server SKUs start at 0.
    BOOLEAN DefaultForwardingDisabled; // TRUE when new compartments start with NSI_IP_COMPARTMENT_RW.Forwarding = 0
                                    //      (IppCreateCompartment). Best-effort name.
    BOOLEAN IcmpRedirects;          // "icmpredirects"
    BOOLEAN AddressMaskReply;       // "addressmaskreply"
    BOOLEAN DisableTaskOffload;     // netsh "taskoffload" shows the inverse.
    BOOLEAN NeighborUpdateRequiresLinkLayerAddress; // IppUpdateNeighbor applies an unsolicited update only when it carries
                                    //      a link-layer address. Best-effort name.
    BOOLEAN DisableDhcpMediaSense;  // netsh "dhcpmediasense" shows the inverse (IppSetDhcpOperationalStatus).
    BOOLEAN DisableMediaSenseEventLog; // netsh "mediasenseeventlog" shows the inverse.
    BOOLEAN MulticastForwarding;    // "multicastforwarding" (default for NSI_IP_COMPARTMENT_RW.MulticastForwarding).
    BOOLEAN GroupForwardedFragments; // "groupforwardedfragments" (default for the compartment field).
    BOOLEAN RandomizeIdentifiers;   // "randomizeidentifiers"
    UCHAR Reserved72;               // alignment padding; nothing reads or writes it
    BOOLEAN DisablePrefixLengthSorting; // Write-only (a get returns 0): address sorting ignores the common prefix length
                                    //      (IppFillSortingElement). Best-effort name.
    BOOLEAN LimitAddressesPerInterface; // Write-only (a get returns 0): IppFindOrCreateLocalAddress stops creating addresses of
                                    //      origin 68 on an interface after 100 and logs an event. Best-effort name.
    UCHAR Reserved75[3];            // alignment padding; nothing reads or writes it
    ULONG MinMtu;                   // "minmtu", IPv4 only, 352..576 (stored as a USHORT).
    BOOLEAN DeadGatewayDetection;   // WMI DeadGatewayDetection (IppGetRouteState, IpNlpSuspectPathReachability).
    UCHAR PathUnreachabilityThreshold; // Unreachability indications needed within PathUnreachabilityWindow
                                    //      (IpNlpSuspectPathReachability). Best-effort name.
    UCHAR DeadRouteMaxProbes;       // Probes sent on a dead route per probe state (IppIsRouteDead). Best-effort name.
    UCHAR Reserved7F;               // alignment padding; nothing reads or writes it
    BOOLEAN FlowLabel;              // "flowlabel". IPv4 accepts only 0.
    BOOLEAN LoopbackLargeMtu;       // "loopbacklargemtu"
    UCHAR LoopbackWorkerCount;      // "loopbackworkercount", 1..16.
    UCHAR Reserved83;               // alignment padding; nothing reads or writes it
    ULONG LoopbackExecutionMode;    // "loopbackexecutionmode" (0 = inline, 1 = worker, 2 = adaptive).
    BOOLEAN SourceBasedEcmp;        // "SourceBasedEcmp"
    UCHAR Reserved89;               // alignment padding; nothing reads or writes it
    USHORT ReassemblyOutOfOrderLimit; // "reassemblyoutoforderlimit", fragments.
    BOOLEAN MultipleArpAnnouncements; // "multiplearpannouncements", IPv4 only.
    UCHAR Reserved8D[3];            // alignment padding; nothing reads or writes it
    ULONG SlaacPrivacyLevel;        // "slaacprivacylevel", 0..2, applied for IPv6 only; when both are set, 1 requires
                                    //      RandomizeIdentifiers TRUE and 0 requires FALSE.
    ULONG SlaacMaxDadAttempts;      // "slaacmaxdadattempts", non-zero.
    BOOLEAN RoutePolicies;          // "routepolicies"
    BOOLEAN RecursiveReassembly;    // "recursivereassembly", IPv6 only.
    BOOLEAN IcmpJumbograms;         // "icmpjumbograms", IPv6 only.
    BOOLEAN ReceiveAddressSetLookup; // Receive paths look destinations up in the local address set first
                                    //      (IppDispatchReceivePacketHelper, IpFlcReceivePreValidatedPackets). Best-effort name.
} NSI_IP_GLOBAL_RW, *PNSI_IP_GLOBAL_RW;

/**
 * The NSI_IP_COMPARTMENT_RW structure is the read/write parameter block of an IP compartment (NPI_MS_IPV4_MODULEID or
 * NPI_MS_IPV6_MODULEID, NsiIpObjectCompartment; key: NET_IF_COMPARTMENT_ID, 0 = the caller's compartment). On a set,
 * 0xFF (BOOLEAN) or 0xFFFFFFFF (ULONG) leaves a field unchanged.
 */
typedef struct _NSI_IP_COMPARTMENT_RW
{
    ULONG Forwarding;               // 0..2; IP Helper reports MIB_IP_FORWARDING only for 1.
    BOOLEAN CompartmentFlag02;      // Compartment flag 0x02 (meaning not identified).
    BOOLEAN CompartmentFlag04;      // Compartment flag 0x04 (meaning not identified).
    UCHAR Reserved[2];              // alignment padding; nothing reads or writes it
    ULONG DefaultHopLimit;          // 1..255 (stored as a UCHAR). IP Helper falls back to NSI_IP_GLOBAL_RW.DefaultHopLimit
                                    //      when it reads 0xFFFFFFFF, which tcpip does not return.
    BOOLEAN MulticastForwarding;    // Compartment flag 0x08: multicast forwarding (IppCreateCompartment seeds it from
                                    //      NSI_IP_GLOBAL_RW.MulticastForwarding).
    BOOLEAN GroupForwardedFragments; // Compartment flag 0x10: IppFragmentPackets groups forwarded fragments (IppGroupFragments).
    BOOLEAN ForwardLayer2Packets;   // Compartment flag 0x20: received packets for other sub-interfaces are forwarded at layer 2
                                    //      (IppDispatchReceivePacketHelper -> IppForwardLayer2Packets).
    UCHAR Reserved1;                // alignment padding; nothing reads or writes it
} NSI_IP_COMPARTMENT_RW, *PNSI_IP_COMPARTMENT_RW;

/**
 * The NSI_IP_COMPARTMENT_ROD structure is the read-only dynamic parameter block of an IP compartment (NsiIpObjectCompartment).
 */
typedef struct _NSI_IP_COMPARTMENT_ROD
{
    ULONG NumIf;                    // MIB_IPSTATS.dwNumIf
    ULONG NumRoutes;                // MIB_IPSTATS.dwNumRoutes (nodes in the compartment's route table)
    ULONG NumPaths;                 // Path (destination cache) entries in the compartment (+0x4AC; IppAllocatePathUnderLock,
                                    //      Ipv4/Ipv6EnumerateAllPaths). IP Helper does not export it.
    ULONG NumAddr;                  // MIB_IPSTATS.dwNumAddr
} NSI_IP_COMPARTMENT_ROD, *PNSI_IP_COMPARTMENT_ROD;

/**
 * The NSI_IP_BEST_ROUTE_ROD_V4 structure is the read-only dynamic parameter block of the IPv4 best-route object
 * (NPI_MS_IPV4_MODULEID, NsiIpObjectBestRoute; key: NSI_IP_BEST_ROUTE_KEY4). Tcpip writes the route key, the route
 * read/write block, the route dynamic and static blocks and the best source address into it, letting later blocks
 * overwrite the tail of earlier ones.
 */
typedef struct _NSI_IP_BEST_ROUTE_ROD_V4
{
    MIB_IP_ROUTE_KEY_ROW_V4 Key;
    union
    {
        MIB_IP_ROUTE_RW_ROW Route;          // DgdState is not returned (overwritten by Age).
        struct
        {
            UCHAR RouteParameters[0x1C];
            ULONG Age;                      // MIB_IPFORWARD_ROW2.Age
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG InternalState;                    // First ULONG of MIB_IP_ROUTE_DYNAMIC_ROW_V4.InternalState.
    NL_ROUTE_ORIGIN Origin;                 // MIB_IPFORWARD_ROW2.Origin
    NET_IFINDEX InterfaceIndex;             // MIB_IPFORWARD_ROW2.InterfaceIndex
    SCOPE_ID SourceScopeId;                 // Scope of the best source address.
    IN_ADDR SourceAddress;                  // Best source address (GetBestRoute2 BestSourceAddress).
    ULONG Reserved;                         // alignment padding; nothing reads or writes it
} NSI_IP_BEST_ROUTE_ROD_V4, *PNSI_IP_BEST_ROUTE_ROD_V4;

/**
 * The NSI_IP_BEST_ROUTE_ROD_V6 structure is the IPv6 counterpart of NSI_IP_BEST_ROUTE_ROD_V4 (key: NSI_IP_BEST_ROUTE_KEY6).
 */
typedef struct _NSI_IP_BEST_ROUTE_ROD_V6
{
    MIB_IP_ROUTE_KEY_ROW_V6 Key;
    union
    {
        MIB_IP_ROUTE_RW_ROW Route;          // DgdState is not returned (overwritten by Age).
        struct
        {
            UCHAR RouteParameters[0x1C];
            ULONG Age;                      // MIB_IPFORWARD_ROW2.Age
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG InternalState[3];                 // First ULONGs of MIB_IP_ROUTE_DYNAMIC_ROW_V6.InternalState.
    ULONG Reserved;                         // alignment padding; nothing reads or writes it
    NL_ROUTE_ORIGIN Origin;                 // MIB_IPFORWARD_ROW2.Origin
    NET_IFINDEX InterfaceIndex;             // MIB_IPFORWARD_ROW2.InterfaceIndex
    SCOPE_ID SourceScopeId;                 // Scope of the best source address.
    IN6_ADDR SourceAddress;                 // Best source address (GetBestRoute2 BestSourceAddress).
    ULONG Reserved1;                        // alignment padding; nothing reads or writes it
} NSI_IP_BEST_ROUTE_ROD_V6, *PNSI_IP_BEST_ROUTE_ROD_V6;

/**
 * The NSI_IP_PATH_KEY_V4 structure is the key of an IPv4 path (NPI_MS_IPV4_MODULEID, NsiIpObjectPath): the
 * destination cache entry for one source/destination pair in a compartment. Its read-only dynamic row is NSI_IP_PATH_ROD_V4.
 * \remarks The IPv4 path key occupies 24 bytes.
 */
typedef struct _NSI_IP_PATH_KEY_V4
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    NET_LUID InterfaceLuid;
    IN_ADDR Destination;
    IN_ADDR Source;
} NSI_IP_PATH_KEY_V4, *PNSI_IP_PATH_KEY_V4;

/**
 * The NSI_IP_PATH_KEY_V6 structure is the IPv6 counterpart of NSI_IP_PATH_KEY_V4 (48 bytes).
 */
typedef struct _NSI_IP_PATH_KEY_V6
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    ULONG Reserved;                 // alignment padding; nothing reads or writes it
    NET_LUID InterfaceLuid;
    IN6_ADDR Destination;
    IN6_ADDR Source;
} NSI_IP_PATH_KEY_V6, *PNSI_IP_PATH_KEY_V6;

/**
 * The NSI_IP_PATH_ROD_V4 structure is the read-only dynamic row of an IPv4 path (NPI_MS_IPV4_MODULEID, NsiIpObjectPath;
 * key: NSI_IP_PATH_KEY_V4). IP Helper converts it to MIB_IPPATH_ROW.
 * \remarks The dynamic path row occupies 104 bytes for IPv4 and 112 bytes for IPv6.
 */
typedef struct _NSI_IP_PATH_ROD_V4
{
    ULONG PathMtu;                          // MIB_IPPATH_ROW.PathMtu
    ULONG UpperLayerMtu;                    // PathMtu minus the IP header size.
    ULONG RttMean;                          // microseconds (IP Helper divides by 1000 for MIB_IPPATH_ROW.RttMean)
    ULONG RttDeviation;                     // microseconds (MIB_IPPATH_ROW.RttDeviation)
    ULONG ReachabilityTime;                 // milliseconds (MIB_IPPATH_ROW.LastReachable / LastUnreachable)
    BOOLEAN IsReachable;                    // MIB_IPPATH_ROW.IsReachable
    BOOLEAN PathFlag40;                     // PathFlags & 0x40 (meaning not identified); IP Helper ignores it.
    UCHAR Reserved[2];                      // alignment padding; nothing reads or writes it
    NL_BANDWIDTH_INFORMATION InboundBandwidthInformation;
    NL_BANDWIDTH_INFORMATION OutboundBandwidthInformation;
    ULONG64 LinkTransmitSpeed;              // MIB_IPPATH_ROW.LinkTransmitSpeed
    ULONG64 LinkReceiveSpeed;               // MIB_IPPATH_ROW.LinkReceiveSpeed
    ULONG PathState;                        // Copied from the path (+0x24; meaning not identified); IP Helper ignores it.
    ULONG PathFlags;                        // Copied from the path (+0x28); IP Helper ignores it.
    IN_ADDR CurrentNextHop;                 // MIB_IPPATH_ROW.CurrentNextHop
    ULONG Reserved1;                        // alignment padding; nothing reads or writes it
} NSI_IP_PATH_ROD_V4, *PNSI_IP_PATH_ROD_V4;

/**
 * The NSI_IP_PATH_ROD_V6 structure is the IPv6 counterpart of NSI_IP_PATH_ROD_V4 (112 bytes).
 */
typedef struct _NSI_IP_PATH_ROD_V6
{
    ULONG PathMtu;
    ULONG UpperLayerMtu;
    ULONG RttMean;                          // microseconds
    ULONG RttDeviation;                     // microseconds
    ULONG ReachabilityTime;                 // milliseconds
    BOOLEAN IsReachable;
    BOOLEAN PathFlag40;
    UCHAR Reserved[2];                      // alignment padding; nothing reads or writes it
    NL_BANDWIDTH_INFORMATION InboundBandwidthInformation;
    NL_BANDWIDTH_INFORMATION OutboundBandwidthInformation;
    ULONG64 LinkTransmitSpeed;
    ULONG64 LinkReceiveSpeed;
    ULONG PathState;
    ULONG PathFlags;
    IN6_ADDR CurrentNextHop;
} NSI_IP_PATH_ROD_V6, *PNSI_IP_PATH_ROD_V6;

/**
 * The NSI_IP_BEST_ROUTE_KEY4 structure is the key of the IPv4 best-route object (NPI_MS_IPV4_MODULEID,
 * NsiIpObjectBestRoute). The provider selects the route and source address it would use to reach DestinationAddress,
 * optionally constrained to SourceAddress and InterfaceLuid. GetBestRoute2 and GetBestInterfaceEx use it.
 */
typedef struct _NSI_IP_BEST_ROUTE_KEY4
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    IN_ADDR DestinationAddress;
    SCOPE_ID DestinationScopeId;
    IN_ADDR SourceAddress;
    SCOPE_ID SourceScopeId;
    NET_LUID InterfaceLuid;
} NSI_IP_BEST_ROUTE_KEY4, *PNSI_IP_BEST_ROUTE_KEY4;

/**
 * The NSI_IP_BEST_ROUTE_KEY6 structure is the IPv6 counterpart of NSI_IP_BEST_ROUTE_KEY4.
 */
typedef struct _NSI_IP_BEST_ROUTE_KEY6
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    IN6_ADDR DestinationAddress;
    SCOPE_ID DestinationScopeId;
    IN6_ADDR SourceAddress;
    SCOPE_ID SourceScopeId;
    NET_LUID InterfaceLuid;
} NSI_IP_BEST_ROUTE_KEY6, *PNSI_IP_BEST_ROUTE_KEY6;

static_assert(sizeof(TCP_ESTATS_SND_CONG_ROS_v0) == 4, "Unexpected SND_CONG ROS size");
static_assert(sizeof(NSI_ESTATS_REC_ROD_INTERNAL) == 56, "Unexpected internal REC ROD size");
static_assert(sizeof(NSI_TCP_IPV4_ROW) == 48, "NSI_TCP_IPV4_ROW size mismatch");
static_assert(sizeof(MIB_IP_ROUTE_DYNAMIC_ROW_V4) == 0x14, "NSI_IP_ROUTE_DYNAMIC_ROW_V4 size mismatch");
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V4, DataLength) == 0x10, "Unexpected IPv4 notification layout");
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V4, Ttl) == 0x14, "Unexpected IPv4 notification layout");
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V4, RoundTripTime) == 0x18, "Unexpected IPv4 notification layout");
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V4, Address) == 0x1C, "Unexpected IPv4 notification layout");
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V4, Flags) == 0x20, "Unexpected IPv4 notification layout");
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V4, OptionsSize) == 0x21, "Unexpected IPv4 notification layout");
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V6, DataLength) == 0x10, "Unexpected IPv6 notification layout");
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V6, RoundTripTime) == 0x18, "Unexpected IPv6 notification layout");
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V6, Address) == 0x1C, "Unexpected IPv6 notification layout");
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V6, ScopeId) == 0x2C, "Unexpected IPv6 notification layout");
static_assert(FIELD_OFFSET(MIB_IP_NEIGHBOR_DYNAMIC_ROW, PhysicalAddressLength) == 0x0A, "MIB_IP_NEIGHBOR_DYNAMIC_ROW address length offset mismatch");
static_assert(FIELD_OFFSET(MIB_IP_NEIGHBOR_DYNAMIC_ROW, CompartmentId) == 0x0C, "MIB_IP_NEIGHBOR_DYNAMIC_ROW compartment offset mismatch");
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, DestinationPrefix) == 0x04, "MIB_IP_ROUTE_KEY_ROW_V4 destination offset mismatch");
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, DestinationPrefixLength) == 0x08, "MIB_IP_ROUTE_KEY_ROW_V4 prefix length offset mismatch");
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, InterfaceLuid) == 0x18, "MIB_IP_ROUTE_KEY_ROW_V4 LUID offset mismatch");
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, NextHop) == 0x28, "MIB_IP_ROUTE_KEY_ROW_V4 next hop offset mismatch");
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, DestinationPrefix) == 0x04, "MIB_IP_ROUTE_KEY_ROW_V6 destination offset mismatch");
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, DestinationPrefixLength) == 0x14, "MIB_IP_ROUTE_KEY_ROW_V6 prefix length offset mismatch");
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, InterfaceLuid) == 0x28, "MIB_IP_ROUTE_KEY_ROW_V6 LUID offset mismatch");
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, NextHop) == 0x38, "MIB_IP_ROUTE_KEY_ROW_V6 next hop offset mismatch");
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, LoopbackInterfaceLuid) == 0x008, "NDIS_NSI_COMPARTMENT_RW loopback LUID offset mismatch");
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, NetworkGuid) == 0x010, "NDIS_NSI_COMPARTMENT_RW network GUID offset mismatch");
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, LoopbackInterfaceGuid) == 0x020, "NDIS_NSI_COMPARTMENT_RW loopback GUID offset mismatch");
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, LoopbackInterfaceAlias) == 0x030, "NDIS_NSI_COMPARTMENT_RW loopback alias offset mismatch");
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, LoopbackInterfaceDescription) == 0x234, "NDIS_NSI_COMPARTMENT_RW loopback description offset mismatch");
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, CompartmentGuid) == 0x438, "NDIS_NSI_COMPARTMENT_RW compartment GUID offset mismatch");
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, CompartmentName) == 0x448, "NDIS_NSI_COMPARTMENT_RW name offset mismatch");
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, SystemCompartment) == 0x64C, "NDIS_NSI_COMPARTMENT_RW system compartment offset mismatch");
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, Flags) == 0x650, "NDIS_NSI_COMPARTMENT_RW flags offset mismatch");
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, NamespaceGuid) == 0x654, "NDIS_NSI_COMPARTMENT_RW namespace GUID offset mismatch");
static_assert(NDIS_SIZEOF_NSI_COMPARTMENT_RW_REVISION_1 == 0x448, "NDIS_NSI_COMPARTMENT_RW revision 1 size mismatch");
static_assert(NDIS_SIZEOF_NSI_COMPARTMENT_RW_REVISION_3 == 0x664, "NDIS_NSI_COMPARTMENT_RW revision 3 size mismatch");
static_assert(FIELD_OFFSET(NSI_TCP_IPV4_TABLE, Entries) == 8, "NSI_TCP_IPV4_TABLE header mismatch");
static_assert(FIELD_OFFSET(NSI_TCP_IPV6_TABLE, Entries) == 8, "NSI_TCP_IPV6_TABLE header mismatch");
static_assert(FIELD_OFFSET(NSI_UDP_IPV4_TABLE, Entries) == 8, "NSI_UDP_IPV4_TABLE header mismatch");
static_assert(FIELD_OFFSET(NSI_UDP_IPV6_TABLE, Entries) == 8, "NSI_UDP_IPV6_TABLE header mismatch");
static_assert(sizeof(ICMP_NOTIFICATION_MESSAGE) == sizeof(PVOID) * 2, "Unexpected notification message layout");
static_assert(sizeof(NSI_COUNTED_WCHAR_PARAMETER) == 516, "NSI counted string size mismatch");
static_assert(sizeof(NSI_ESTATS_SND_CONG_RW_v0) == 1, "Unexpected SND_CONG RW size");
static_assert(sizeof(MIB_IP_ANYCAST_KEY_ROW_V4) == 0x10, "MIB_IP_ANYCAST_KEY_ROW_V4 size mismatch");
static_assert(sizeof(MIB_IP_ANYCAST_KEY_ROW_V6) == 0x18, "MIB_IP_ANYCAST_KEY_ROW_V6 size mismatch");
static_assert(sizeof(NSI_IP_BEST_ROUTE_KEY4) == 32, "IPv4 best-route key size mismatch");
static_assert(sizeof(NSI_IP_BEST_ROUTE_KEY6) == 56, "IPv6 best-route key size mismatch");
static_assert(sizeof(MIB_IP_NEIGHBOR_DYNAMIC_ROW) == 0x10, "MIB_IP_NEIGHBOR_DYNAMIC_ROW size mismatch");
static_assert(sizeof(MIB_IP_NEIGHBOR_KEY_ROW_V4) == 0x18, "MIB_IP_NEIGHBOR_KEY_ROW_V4 size mismatch");
static_assert(sizeof(MIB_IP_NEIGHBOR_KEY_ROW_V6) == 0x20, "MIB_IP_NEIGHBOR_KEY_ROW_V6 size mismatch");
static_assert(sizeof(MIB_IP_NEIGHBOR_RW_ROW) == 0x20, "MIB_IP_NEIGHBOR_RW_ROW size mismatch");
static_assert(sizeof(MIB_IP_ROUTE_DYNAMIC_ROW_V6) == 0x20, "MIB_IP_ROUTE_DYNAMIC_ROW_V6 size mismatch");
static_assert(sizeof(MIB_IP_ROUTE_KEY_ROW_V4) == 0x30, "MIB_IP_ROUTE_KEY_ROW_V4 size mismatch");
static_assert(sizeof(MIB_IP_ROUTE_KEY_ROW_V6) == 0x48, "MIB_IP_ROUTE_KEY_ROW_V6 size mismatch");
static_assert(sizeof(MIB_IP_ROUTE_STATIC_ROW) == 8, "MIB_IP_ROUTE_STATIC_ROW size mismatch");
static_assert(sizeof(MIB_IP_UNICAST_DYNAMIC_ROW) == 8, "MIB_IP_UNICAST_DYNAMIC_ROW size mismatch");
static_assert(sizeof(MIB_IP_UNICAST_KEY_ROW_V4) == 0x10, "MIB_IP_UNICAST_KEY_ROW_V4 size mismatch");
static_assert(sizeof(MIB_IP_UNICAST_KEY_ROW_V6) == 0x18, "MIB_IP_UNICAST_KEY_ROW_V6 size mismatch");
static_assert(sizeof(MIB_IP_UNICAST_RW_ROW) == 0x1C, "MIB_IP_UNICAST_RW_ROW size mismatch");
static_assert(sizeof(MIB_IP_UNICAST_STATIC_ROW) == 8, "MIB_IP_UNICAST_STATIC_ROW size mismatch");
static_assert(sizeof(MIB_NDIS_COMPARTMENT_KEY_ROW) == 4, "MIB_NDIS_COMPARTMENT_KEY_ROW size mismatch");
static_assert(sizeof(NDIS_NSI_COMPARTMENT_RW) == 0x668, "NDIS_NSI_COMPARTMENT_RW size mismatch");
static_assert(sizeof(MIB_NDIS_IFSTACK_KEY_ROW) == 8, "MIB_NDIS_IFSTACK_KEY_ROW size mismatch");
static_assert(sizeof(NSI_TCP_IPV6_ROW) == 80, "NSI_TCP_IPV6_ROW size mismatch");
static_assert(sizeof(NSI_TCP_KEY_ROW) == 56, "NSI_TCP_KEY_ROW size mismatch");
static_assert(sizeof(NSI_TCP_OWNER_ROW) == 32, "NSI_TCP_OWNER_ROW size mismatch");
static_assert(sizeof(NSI_TCP_STATUS_ROW) == 24, "NSI_TCP_STATUS_ROW size mismatch");
static_assert(sizeof(NSI_UDP_IPV4_ROW) == 24, "NSI_UDP_IPV4_ROW size mismatch");
static_assert(sizeof(NSI_UDP_IPV6_ROW) == 40, "NSI_UDP_IPV6_ROW size mismatch");
static_assert(sizeof(NSI_UDP_KEY_ROW) == 28, "NSI_UDP_KEY_ROW size mismatch");
static_assert(sizeof(NSI_UDP_OWNER_ROW) == 32, "NSI_UDP_OWNER_ROW size mismatch");
static_assert(sizeof(NDIS_NSI_INTERFACE_ENUM_ROD) == 0x290);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifOperStatus) == 0x218);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifPhysAddress) == 0x224);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifAdminStatus) == 0x268);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, XmitLinkSpeed) == 0x270);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, MediaConnectState) == 0x284);
static_assert(sizeof(NDIS_NSI_INTERFACE_ENUM_ROS) == 0x238);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, ifType) == 0x208);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, InterfaceGuid) == 0x218);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, ifFlags) == 0x22C);
static_assert(sizeof(NDIS_NSI_INTERFACE_INFORMATION_RW) == 0x444);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_INFORMATION_RW, ifPhysAddress) == 0x218);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_INFORMATION_RW, ifL2NetworkInfo) == 0x23A);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_INFORMATION_RW, ifPurpose) == 0x440);
static_assert(sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROD) == 0xD8);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_INFORMATION_ROD, CompartmentId) == 0xD0);
static_assert(sizeof(NDIS_NSI_INTERFACE_INFORMATION_ROS) == 0x258);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_INFORMATION_ROS, PermanentPhysAddress) == 0x22A);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_INFORMATION_ROS, ifFlags) == 0x24C);
static_assert(sizeof(NDIS_NSI_NETWORK_RW) == 0x210);
static_assert(FIELD_OFFSET(NDIS_NSI_NETWORK_RW, NetworkName) == 0x0C);
static_assert(sizeof(NSI_IP_INTERFACE_ROD) == 0x58);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, InboundBandwidthInformation) == 0x28);
static_assert(sizeof(NETIO_TCP_STATS_DYNAMIC) == 104);
static_assert(FIELD_OFFSET(NETIO_TCP_STATS_DYNAMIC, BytesBuffered) == 0x38);
static_assert(FIELD_OFFSET(NETIO_TCP_STATS_DYNAMIC, FastopenActiveOpens) == 0x50);
static_assert(FIELD_OFFSET(NETIO_TCP_STATS_DYNAMIC, FirstSynRexmitsWithFastopen) == 0x60);
static_assert(sizeof(NSI_TCP_GLOBAL_ROS) == 20);
static_assert(sizeof(NSI_TCP_GLOBAL_RW) == 48);
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_RW, AutoTuningLevelLocal) == 0x20);
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_RW, PacingProfile) == 0x2C);
static_assert(sizeof(NETIO_UDP_STATS_DYNAMIC) == 48);
static_assert(sizeof(NSI_UDP_GLOBAL_RW) == 2);
#ifdef _WIN64 // pointer-containing layouts defined here for x64
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, SourceScopeId) == 0x4C);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, DontFragment) == 0x58);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, SourceAddress) == 0x5C);
static_assert(sizeof(ICMP_REQUEST_V4) == 0x68);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V6, SourceAddress) == 0x58);
static_assert(sizeof(ICMP_REQUEST_V6) == 0x78);
#endif
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V6, HopLimit) == 0x14);
static_assert(sizeof(NSI_ESTATS_TCP_KEY_V4) == 0x38);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, RemoteAddressFamily) == 0x1C);
static_assert(FIELD_OFFSET(MIB_IP_NEIGHBOR_KEY_ROW_V4, Address) == 0x10);
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, SourcePrefix) == 0x0C);
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, SubInterfaceLuid) == 0x20);
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, SourcePrefix) == 0x16);
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, SubInterfaceLuid) == 0x30);
static_assert(sizeof(MIB_IP_ROUTE_RW_ROW) == 0x20);
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_RW_ROW, Protocol) == 0x10);
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_RW_ROW, DgdState) == 0x1C);
static_assert(FIELD_OFFSET(MIB_IP_UNICAST_RW_ROW, SourcePreference) == 0x14);
static_assert(FIELD_OFFSET(MIB_IP_UNICAST_RW_ROW, SkipDad) == 0x19);
static_assert(sizeof(NSI_IP_BEST_ROUTE_ROD_V4) == 0x68);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V4, Age) == 0x4C);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V4, SourceAddress) == 0x60);
static_assert(sizeof(NSI_IP_BEST_ROUTE_ROD_V6) == 0x98);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, Age) == 0x64);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, SourceAddress) == 0x84);
static_assert(sizeof(NSI_IP_PATH_ROD_V4) == 104);
static_assert(sizeof(NSI_IP_PATH_ROD_V6) == 112);
static_assert(FIELD_OFFSET(NSI_IP_PATH_ROD_V4, CurrentNextHop) == 0x60);
static_assert(sizeof(NSI_IP_STATISTICS_ROD) == 208);
static_assert(FIELD_OFFSET(NSI_IP_STATISTICS_ROD, InHdrErrors) == 0x80);
static_assert(FIELD_OFFSET(NSI_IP_STATISTICS_ROD, RoutingDiscards) == 0xBC);
static_assert(FIELD_OFFSET(NSI_IP_STATISTICS_ROD, TemporaryRandomTime) == 0xC0);
static_assert(sizeof(NSI_IP_GLOBAL_RW) == 156);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, DefaultHopLimit) == 0x18);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, DeadRouteProbeTimeout) == 0x30);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, LinkLayerSuffixPrefixLength) == 0x54);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, SourceRoutingBehavior) == 0x58);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, DefaultForwardingDisabled) == 0x68);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, RandomizeIdentifiers) == 0x71);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, MinMtu) == 0x78);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, DeadGatewayDetection) == 0x7C);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, FlowLabel) == 0x80);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, LoopbackExecutionMode) == 0x84);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, ReassemblyOutOfOrderLimit) == 0x8A);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, SlaacPrivacyLevel) == 0x90);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, RoutePolicies) == 0x98);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, ReceiveAddressSetLookup) == 0x9B);
static_assert(sizeof(NSI_IP_COMPARTMENT_RW) == 16);
static_assert(sizeof(NSI_IP_COMPARTMENT_ROD) == 16);
static_assert(sizeof(NSI_FL_VIRTUAL_INTERFACE_KEY) == 32);
static_assert(sizeof(NSI_FL_VIRTUAL_INTERFACE_ROD) == 136);
static_assert(sizeof(NSI_FL_VIRTUAL_INTERFACE_ROS) == 16);
static_assert(sizeof(TCP_KEY_ROW) == sizeof(NSI_TCP_KEY_ROW));
static_assert(sizeof(UDP_KEY_ROW) == sizeof(NSI_UDP_KEY_ROW));
#ifdef _WIN64 // pointer-containing layouts defined here for x64
static_assert(sizeof(NETIO_CHANGE_REGISTRATION_HEADER) == 0x48);
static_assert(sizeof(NETIO_ROUTE_CHANGE_REGISTRATION) == 0x148);
static_assert(sizeof(NETIO_IP_INTERFACE_CHANGE_REGISTRATION) == 0x1C8);
static_assert(sizeof(NETIO_TEREDO_PORT_CHANGE_REGISTRATION) == 0x58);
static_assert(sizeof(NETIO_NDIS_COMPARTMENT_CHANGE_REGISTRATION) == 0x278);
#endif
static_assert(FIELD_OFFSET(MIB_IP_NEIGHBOR_DYNAMIC_ROW, IsUnreachable) == 0x09);
static_assert(FIELD_OFFSET(NSI_TCP_OWNER_ROW, DualMode) == 3);
static_assert(FIELD_OFFSET(NSI_TCP_OWNER_ROW, SendWindowScale) == 4);
static_assert(FIELD_OFFSET(NSI_TCP_OWNER_ROW, ReceiveWindowScale) == 8);
static_assert(FIELD_OFFSET(NSI_TCP_OWNER_ROW, OwningPid) == 12);
static_assert(FIELD_OFFSET(NSI_TCP_STATUS_ROW, NagleEnabled) == 4);
static_assert(FIELD_OFFSET(NSI_TCP_STATUS_ROW, AppliedSetting) == 12);
static_assert(FIELD_OFFSET(NSI_TCP_STATUS_ROW, QosFlowTag) == 20);
static_assert(FIELD_OFFSET(NSI_UDP_OWNER_ROW, DualMode) == 4);
static_assert(FIELD_OFFSET(NSI_UDP_OWNER_ROW, Flags) == 16);
static_assert(sizeof(NSI_STRING_256) == 516);

/**
 * Allocates separate NSI key, read-write, dynamic and static tables; entry sizes are bytes and Count is the
 * row count. Release returned allocations with NsiFreeTable. The thirteenth ULONG argument is required by the
 * x86 ABI (52 stack bytes); inspected callers pass both 0 and 1. It is not read by the inspected
 * implementations, so no flag meaning or must-be-zero rule is asserted. A zero-sized table can leave its
 * output slot untouched; initialize optional slots before calling. This routine performs NSI device I/O.
 */
NTSYSAPI
ULONG
NTAPI
NsiAllocateAndGetTable(
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

/**
 * The NsiAllocateAndGetPersistentDataWithMaskTable routine enumerates the persistent store of an NSI object and returns
 * heap-allocated key, data and mask tables (one row per persisted instance). Used to read back configuration that has been
 * written with NsiSetAllPersistentParametersWithMask, e.g. Persisted interface or route settings.
 *
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object to query.
 * \param KeyTable Optional; receives the allocated key table (*Count rows of KeyEntrySize bytes). Not allocated when KeyEntrySize is 0.
 * \param KeyEntrySize The size of each key entry in bytes.
 * \param ParameterDataTable Optional; receives the allocated parameter data table (*Count rows of ParameterDataEntrySize bytes).
 * \param ParameterMaskTable Optional; receives the allocated mask table. It has the same row size as the data table.
 * \param ParameterDataEntrySize The size of each data row and each mask row in bytes. Neither table is allocated when 0.
 * \param Count Receives the number of rows returned.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks Free the tables with NsiFreePersistentDataWithMaskTable. The routine sizes the tables with a count query and retries
 * up to three times while the store keeps growing (ERROR_MORE_DATA). 
 */
NTSYSAPI
ULONG
NTAPI
NsiAllocateAndGetPersistentDataWithMaskTable(
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _Outptr_result_bytebuffer_maybenull_(*Count * KeyEntrySize) _Out_opt_ PVOID* KeyTable,
    _In_ ULONG KeyEntrySize,
    _Outptr_result_bytebuffer_maybenull_(*Count * ParameterDataEntrySize) _Out_opt_ PVOID* ParameterDataTable,
    _Outptr_result_bytebuffer_maybenull_(*Count * ParameterDataEntrySize) _Out_opt_ PVOID* ParameterMaskTable,
    _In_ ULONG ParameterDataEntrySize,
    _Out_ PULONG Count
    );

/**
 * The NsiDeregisterChangeNotification routine unregisters an NSI change notification callback.
 *
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param NotificationHandle Optional handle to the notification registration to deregister.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiDeregisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
    );

/**
 * The NsiCancelChangeNotification routine cancels a pending NSI change notification request.
 *
 * \param NotificationHandle Optional handle to the change notification to cancel.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiCancelChangeNotification(
    _In_opt_ HANDLE NotificationHandle
    );

/**
 * The NsiDeregisterChangeNotificationEx routine unregisters an extended NSI change notification callback.
 *
 * \param DeregistrationRequest A pointer to the extended deregistration request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiDeregisterChangeNotificationEx(
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST DeregistrationRequest
    );

/**
 * The NsiEnumerateObjectsAllParameters routine enumerates all objects and parameters for an NSI module object.
 *
 * \param Store The NSI store to query (active or persistent).
 * \param Action NsiGetFirst to enumerate from the beginning (what NsiAllocateAndGetTable and IP Helper pass) or NsiGetNext to
 *        continue after the key in KeyTable[0]. netio.sys rejects any other value with STATUS_INVALID_PARAMETER.
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object to enumerate.
 * \param KeyTable Optional buffer receiving the array of key entries.
 * \param KeyEntrySize The size of each key entry in bytes.
 * \param RwTable Optional buffer receiving read-write entries.
 * \param RwEntrySize The size of each read-write entry in bytes.
 * \param RodTable Optional buffer receiving dynamic read-only entries.
 * \param RodEntrySize The size of each dynamic read-only entry in bytes.
 * \param RosTable Optional buffer receiving static read-only entries.
 * \param RosEntrySize The size of each static read-only entry in bytes.
 * \param Count A pointer specifying capacity and receiving the number of entries returned.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiEnumerateObjectsAllParameters(
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

/**
 * The NsiEnumerateObjectsAllParametersEx routine enumerates all objects and parameters using an extended request structure.
 *
 * \param EnumerateRequest A pointer to the extended enumeration request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiEnumerateObjectsAllParametersEx(
    _Inout_ PNSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX EnumerateRequest
    );

/**
 * The NsiEnumerateObjectsAllPersistentParametersWithMask routine enumerates the persistent store of an NSI object into
 * caller-supplied key, data and mask tables (IOCTL 0x120047). Pass NULL tables to obtain the row count only.
 *
 * \param Action Must be NsiGetFirst (the value NsiAllocateAndGetPersistentDataWithMaskTable passes). nsiproxy.sys forwards it
 *        to the same netio.sys check that validates NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX.Action for persistent enumeration,
 *        which fails anything else with STATUS_NOT_IMPLEMENTED.
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object to enumerate.
 * \param KeyTable Optional buffer receiving *Count key entries.
 * \param KeyEntrySize The size of each key entry in bytes.
 * \param ParameterDataTable Optional buffer receiving *Count parameter data rows.
 * \param ParameterMaskTable Optional buffer receiving *Count parameter mask rows.
 * \param ParameterDataEntrySize The size of each data row and each mask row in bytes.
 * \param Count On input, the capacity of the tables in rows; on output, the number of rows (also on ERROR_MORE_DATA).
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiEnumerateObjectsAllPersistentParametersWithMask(
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

/**
 * The NsiGetParameter routine reads one field of one NSI object instance, e.g. The interface index for an interface LUID
 * (ConvertInterfaceLuidToIndex) or the alias of an interface (ConvertInterfaceLuidToAlias).
 *
 * \param Store The NSI store to query.
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param Key A pointer to the key identifying the object instance.
 * \param KeySize The size of the key in bytes.
 * \param ParameterType The row that contains the field (read-write, read-only dynamic or read-only static).
 * \param Buffer Optional buffer receiving the field value.
 * \param BufferLength The size of Buffer in bytes (the number of bytes copied from the row).
 * \param ParameterOffset The byte offset of the field within the ParameterType row (e.g. FIELD_OFFSET of the row struct).
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks For example, ParameterType 2, ParameterOffset 536 for the
 * interface GUID in the NDIS interface static row.
 */
NTSYSAPI
ULONG
NTAPI
NsiGetParameter(
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

/**
 * The NsiGetParameterEx routine retrieves a single parameter using an extended request structure.
 *
 * \param GetParameterRequest A pointer to the extended parameter retrieval request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiGetParameterEx(
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST GetParameterRequest
    );

/**
 * The NsiGetAllParameters routine retrieves all parameters for a single NSI object instance.
 *
 * \param Store The NSI store to query.
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param Key A pointer to the key identifying the object instance.
 * \param KeySize The size of the key in bytes.
 * \param Rw Optional buffer receiving read-write parameters.
 * \param RwSize The size of the read-write buffer in bytes.
 * \param Rod Optional buffer receiving dynamic read-only parameters.
 * \param RodSize The size of the dynamic read-only buffer in bytes.
 * \param Ros Optional buffer receiving static read-only parameters.
 * \param RosSize The size of the static read-only buffer in bytes.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiGetAllParameters(
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

/**
 * The NsiGetAllParametersEx routine retrieves all parameters using an extended request structure.
 *
 * \param GetAllParametersRequest A pointer to the extended request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiGetAllParametersEx(
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST GetAllParametersRequest
    );

/**
 * The NsiSetParameter routine writes one field of one NSI object instance, e.g. The administrative status of
 * an interface (SetIfEntry) or the extended TCP statistics collection switches of a connection
 * (SetPerTcpConnectionEStats).
 *
 * \param Store The NSI store to update.
 * \param Action The set action (NSI_SET_ACTION): NsiSetDefault to update an existing instance,
 * NsiSetCreateOnly, NsiSetCreateOrSet, NsiSetDelete, NsiSetReset (flush), NsiSetClear, or the
 * reference-counted NsiSetCreateOrSetWithReference/NsiSetDeleteWithReference (used by
 * EnableRouter/UnenableRouter).
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param Key A pointer to the key identifying the object instance.
 * \param KeySize The size of the key in bytes.
 * \param ParameterType The row that contains the field (normally NsiParameterTypeReadWrite).
 * \param ParameterBuffer Optional buffer containing the new field value.
 * \param ParameterBufferSize The size of ParameterBuffer in bytes.
 * \param ParameterOffset The byte offset of the field within the ParameterType row.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiSetParameter(
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

/**
 * The NsiSetParameterEx routine sets a single parameter using an extended request structure.
 *
 * \param SetParameterRequest A pointer to the extended parameter set request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiSetParameterEx(
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST SetParameterRequest
    );

/**
 * The NsiSetAllParameters routine creates, updates, deletes or flushes an NSI object instance by writing its whole read-write
 * row. This is how iphlpapi implements CreateIpForwardEntry2/DeleteIpForwardEntry2, CreateIpNetEntry/DeleteIpNetEntry,
 * AddIPAddress/DeleteIPAddress, SetIpInterfaceEntry, FlushIpNetTable and SetTcpEntry (closing a TCP connection).
 *
 * \param Store The NSI store to update (NsiPersistent, NsiActive or NsiBoth).
 * \param Action The set action (NSI_SET_ACTION): NsiSetDefault to update an existing instance, NsiSetCreateOnly,
 *        NsiSetCreateOrSet, NsiSetDelete, NsiSetReset (flush), NsiSetClear, or the reference-counted
 *        NsiSetCreateOrSetWithReference/NsiSetDeleteWithReference (used by EnableRouter/UnenableRouter).
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param Key Optional pointer to the key identifying the object instance (e.g. The interface LUID, or LUID + address).
 * \param KeySize The size of the key in bytes.
 * \param Rw Optional read-write row to write. Usually NULL for NsiSetDelete and NsiSetReset.
 * \param RwSize The size of Rw in bytes.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks Uses IOCTL 0x120013 with a 0x48-byte request. For example,
 *          InternalSetIpInterfaceEntry: NsiSetAllParameters(NsiActive, NsiSetDefault, ModuleId, 7, &Luid, 8, &RwRow, 240).
 */
NTSYSAPI
ULONG
NTAPI
NsiSetAllParameters(
    _In_ NSI_STORE Store,
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_opt_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(RwSize) PVOID Rw,
    _In_ ULONG RwSize
    );

/**
 * The NsiSetAllParametersEx routine sets all parameters using an extended request structure.
 *
 * \param SetAllParametersRequest A pointer to the extended request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiSetAllParametersEx(
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST SetAllParametersRequest
    );

/**
 * The NsiRegisterChangeNotification routine registers a callback to receive NSI change notifications.
 *
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object to observe.
 * \param ChangeCallback Must be NULL in user mode: nsi.dll fails the request with ERROR_INVALID_PARAMETER when a callback is
 *        supplied (callbacks are only honoured for kernel-mode NSI clients). User-mode callers should use
 *        NsiRpcRegisterChangeNotification or NsiRequestChangeNotification instead.
 * \param NotifyCurrent TRUE to notify immediately of current state; FALSE otherwise.
 * \param CallerContext Optional context value passed to the callback function.
 * \param NotificationHandle Receives a handle to the change notification registration.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiRegisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ CONST NSI_OBJECT_ID ObjectId,
    _In_opt_ PVOID ChangeCallback,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Outptr_result_maybenull_ PHANDLE NotificationHandle
    );

/**
 * The NsiRegisterChangeNotificationEx routine registers an extended change notification (IOCTL 0x12001F, 0x48-byte request).
 *
 * \param RegistrationRequest A pointer to the extended registration request structure. ChangeCallback must be NULL in user
 *        mode (ERROR_INVALID_PARAMETER otherwise).
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiRegisterChangeNotificationEx(
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST RegistrationRequest
    );

/**
 * The NsiRequestChangeNotification routine requests asynchronous change notifications for an NSI object.
 *
 * \param ModuleHandle The NSI module handle, or 0 to identify the module by ModuleId (IP Helper always passes 0).
 *        This is not a flags value: nsiproxy.sys copies it into the module identity of the kernel registration, and netio.sys
 *        resolves a non-zero value through its module handle table (the value NsiGetModuleHandle returns in kernel mode).
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectIndex The object index to monitor.
 * \param OverLapped Optional pointer to an OVERLAPPED structure for asynchronous completion.
 * \param NotificationHandle Receives a notification handle.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiRequestChangeNotification(
    _In_ ULONG ModuleHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ ULONG ObjectIndex,
    _In_opt_ LPOVERLAPPED OverLapped,
    _Out_ PHANDLE NotificationHandle
    );

/**
 * The NsiRequestChangeNotificationEx routine requests an asynchronous change notification for an NSI object
 * (IOCTL 0x12003F). The request completes through Request->Overlapped when the object changes, which lets user-mode
 * callers wait for changes without a callback.
 *
 * \param Request A pointer to the notification request (0x28 bytes). When Request->Overlapped is not NULL and
 *        Request->NotificationHandle is not NULL, the NSI device handle used for the request is stored in
 *        *Request->NotificationHandle so it can be passed to NsiCancelChangeNotification or GetOverlappedResult.
 * \return A Win32 error code (ERROR_IO_PENDING for an overlapped request that has not completed yet).
 */
NTSYSAPI
ULONG
NTAPI
NsiRequestChangeNotificationEx(
    _Inout_ PNSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST Request
    );

/**
 * The NsiGetAllPersistentParametersWithMask routine reads the persisted (registry-backed) configuration of
 * one NSI object instance together with a mask that marks which fields have actually been persisted, e.g. The
 * persisted settings of an IP interface.
 *
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param Key A pointer to the key identifying the object instance.
 * \param KeySize The size of the key in bytes.
 * \param ParameterData Optional buffer receiving the persisted row.
 * \param ParameterMask Optional buffer receiving the field mask (same size as ParameterData; 0xFF bytes mark
 * persisted fields).
 * \param ParameterDataSize The size of ParameterData and of ParameterMask in bytes.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiGetAllPersistentParametersWithMask(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _Out_writes_bytes_opt_(ParameterDataSize) PVOID ParameterData,
    _Out_writes_bytes_opt_(ParameterDataSize) PVOID ParameterMask,
    _In_ ULONG ParameterDataSize
    );

/**
 * The NsiSetAllPersistentParametersWithMask routine writes the persisted (registry-backed) configuration of one NSI object
 * instance, merging only the fields selected by the mask, e.g. SetIpInterfaceEntry persisting changed interface settings.
 *
 * \param Action The set action (NSI_SET_ACTION): NsiSetDefault to update an existing instance, NsiSetCreateOnly,
 *        NsiSetCreateOrSet, NsiSetDelete, NsiSetReset (flush), NsiSetClear, or the reference-counted
 *        NsiSetCreateOrSetWithReference/NsiSetDeleteWithReference (used by EnableRouter/UnenableRouter).
 *        netio.sys validates the value (NsiSetReset and NsiSetClear are handled specially); iphlpapi passes NsiSetCreateOrSet.
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param Key A pointer to the key identifying the object instance.
 * \param KeySize The size of the key in bytes.
 * \param ParameterData Optional buffer containing the row to persist.
 * \param ParameterMask Optional field mask (same size as ParameterData; 0xFF bytes select the fields to persist).
 * \param ParameterDataSize The size of ParameterData and of ParameterMask in bytes.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiSetAllPersistentParametersWithMask(
    _In_ NSI_SET_ACTION Action,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_reads_bytes_(KeySize) PVOID Key,
    _In_ ULONG KeySize,
    _In_reads_bytes_opt_(ParameterDataSize) PVOID ParameterData,
    _In_reads_bytes_opt_(ParameterDataSize) PVOID ParameterMask,
    _In_ ULONG ParameterDataSize
    );

/**
 * Submits an opaque NSI security request in place, using 48 bytes on x64 and 24 bytes on x86. The annotation
 * describes the request extent, not a six-pointer schema. Queries object security through IOCTL 0x120037. The
 * same buffer is input and output; kernel-side field validation and authorization are separate contracts.
 */
NTSYSAPI
ULONG
NTAPI
NsiGetObjectSecurity(
    _Inout_updates_bytes_(6 * sizeof(PVOID)) PVOID ObjectSecurity
    );

/**
 * Submits an opaque NSI security request in place, using 48 bytes on x64 and 24 bytes on x86. The annotation
 * describes the request extent, not a six-pointer schema. Changes object security through IOCTL 0x12003B. The
 * same buffer is input and output; kernel-side field validation and authorization are separate contracts.
 */
NTSYSAPI
ULONG
NTAPI
NsiSetObjectSecurity(
    _Inout_updates_bytes_(6 * sizeof(PVOID)) PVOID ObjectSecurity
    );

/**
 * Creates and configures a local ncalrpc binding for NSI RPC calls. A non-NULL Reserved argument returns NULL
 * before binding creation without setting a new last error. Binding-creation failures do set last error.
 * Release successful bindings with NsiDisconnectFromServer. Binding creation alone does not query an NSI
 * table.
 */
NTSYSAPI
PVOID
WINAPI
NsiConnectToServer(
    _Reserved_ PVOID Reserved
    );

/**
 * Releases the RPC binding value through RpcBindingFree using a temporary local slot. The caller variable is
 * not cleared; discard its value after successful cleanup. This releases a binding, not the remote server.
 */
NTSYSAPI
ULONG
WINAPI
NsiDisconnectFromServer(
    _In_opt_ PVOID RpcHandle
    );

/**
 * Frees the key, data and mask allocations from the persistent-table allocator using the NSI heap. Each
 * pointer is optional. Pass allocation values, not output-slot addresses. Caller variables are not cleared,
 * and there is no status result.
 */
NTSYSAPI
VOID
NTAPI
NsiFreePersistentDataWithMaskTable(
    _Frees_ptr_opt_ PVOID KeyTable,
    _Frees_ptr_opt_ PVOID ParameterDataTable,
    _Frees_ptr_opt_ PVOID ParameterMaskTable
    );

/**
 * Frees up to four independent NSI table allocations using the NSI heap. NULL entries are skipped. Pass
 * allocation values returned by NsiAllocateAndGetTable, not output-slot addresses. Caller variables are not
 * cleared, and there is no status result. Repeated allocation values would cause repeated frees.
 */
NTSYSAPI
VOID
NTAPI
NsiFreeTable(
    _Frees_ptr_opt_ PVOID KeyTable,
    _Frees_ptr_opt_ PVOID RwTable,
    _Frees_ptr_opt_ PVOID RodTable,
    _Frees_ptr_opt_ PVOID RosTable
    );

/**
 * The NsiRpcDeregisterChangeNotification routine unregisters an NSI change notification that was registered over RPC.
 *
 * \param RpcHandle The RPC binding handle used for the registration (required; NULL fails with ERROR_INVALID_PARAMETER).
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param NotificationHandle The handle returned by NsiRpcRegisterChangeNotification.
 * \return A Win32 error code. ERROR_SUCCESS on success; ERROR_INVALID_PARAMETER if the handle is unknown.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcDeregisterChangeNotification(
    _In_ HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
    );

/**
 * The NsiRpcDeregisterChangeNotificationEx routine unregisters a change notification that was registered over RPC.
 *
 * \param RpcHandle The RPC binding handle used for the registration (required; NULL fails with ERROR_INVALID_PARAMETER).
 * \param DeregistrationRequest The deregistration request; NotificationHandle is the handle returned at registration.
 * \return A Win32 error code. ERROR_SUCCESS on success; ERROR_INVALID_PARAMETER if the handle is unknown.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcDeregisterChangeNotificationEx(
    _In_ HANDLE RpcHandle,
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST DeregistrationRequest
    );

/**
 * The NsiRpcEnumerateObjectsAllParameters routine enumerates all instances of an NSI object, locally or through the NSI RPC
 * server. Same parameters and results as NsiEnumerateObjectsAllParameters.
 *
 * \param RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally
 *        through the matching nsi.dll routine (no RPC).
 * \param Store The NSI store to query. NsiBoth is rejected over RPC (ERROR_INVALID_PARAMETER).
 * \param Action NsiGetFirst to enumerate from the beginning (what NsiAllocateAndGetTable and IP Helper pass) or NsiGetNext to
 *        continue after the key in KeyTable[0]. netio.sys rejects any other value with STATUS_INVALID_PARAMETER.
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object to enumerate.
 * \param KeyTable Optional buffer receiving key entries.
 * \param KeyEntrySize The size of each key entry in bytes.
 * \param RwTable Optional buffer receiving read-write entries.
 * \param RwEntrySize The size of each read-write entry in bytes.
 * \param RodTable Optional buffer receiving dynamic read-only entries.
 * \param RodEntrySize The size of each dynamic read-only entry in bytes.
 * \param RosTable Optional buffer receiving static read-only entries.
 * \param RosEntrySize The size of each static read-only entry in bytes.
 * \param Count A pointer specifying capacity and receiving the number of entries returned.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcEnumerateObjectsAllParameters(
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

/**
 * The NsiRpcGetAllParameters routine retrieves all parameters of one NSI object instance, locally or through the NSI RPC
 * server. Same parameters and results as NsiGetAllParameters.
 *
 * \param RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally
 *        through the matching nsi.dll routine (no RPC).
 * \param Store The NSI store to query. NsiBoth is rejected over RPC (ERROR_INVALID_PARAMETER).
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param Key A pointer to the key identifying the object instance.
 * \param KeySize The size of the key in bytes.
 * \param Rw Optional buffer receiving read-write parameters.
 * \param RwSize The size of the read-write buffer in bytes.
 * \param Rod Optional buffer receiving dynamic read-only parameters.
 * \param RodSize The size of the dynamic read-only buffer in bytes.
 * \param Ros Optional buffer receiving static read-only parameters.
 * \param RosSize The size of the static read-only buffer in bytes.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcGetAllParameters(
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

/**
 * The NsiRpcGetAllParametersEx routine is the request-structure form of NsiRpcGetAllParameters.
 *
 * \param RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally
 *        through the matching nsi.dll routine (no RPC).
 * \param GetAllParametersRequest A pointer to the request (passed to NsiGetAllParametersEx when RpcHandle is NULL).
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks Over RPC the request must have MustBeZero == 0 (else ERROR_CALL_NOT_IMPLEMENTED) and a ModuleHandle or ModuleId
 *          (else ERROR_INVALID_PARAMETER).
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcGetAllParametersEx(
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST GetAllParametersRequest
    );

/**
 * The NsiRpcGetParameter routine reads one field of one NSI object instance, locally or through the NSI RPC server.
 * Same parameters and results as NsiGetParameter.
 *
 * \param RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally
 *        through the matching nsi.dll routine (no RPC).
 * \param Store The NSI store to query. NsiBoth is rejected over RPC (ERROR_INVALID_PARAMETER).
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param Key A pointer to the key identifying the object instance.
 * \param KeySize The size of the key in bytes.
 * \param ParameterType The row that contains the field.
 * \param Buffer Optional buffer receiving the field value.
 * \param BufferLength The size of Buffer in bytes.
 * \param ParameterOffset The byte offset of the field within the ParameterType row.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcGetParameter(
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

/**
 * The NsiRpcGetParameterEx routine is the request-structure form of NsiRpcGetParameter.
 *
 * \param RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally
 *        through the matching nsi.dll routine (no RPC).
 * \param GetParameterRequest A pointer to the request (passed to NsiGetParameterEx when RpcHandle is NULL).
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks Over RPC the request must have MustBeZero == 0 (else ERROR_CALL_NOT_IMPLEMENTED) and a ModuleHandle or ModuleId
 *          (else ERROR_INVALID_PARAMETER).
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcGetParameterEx(
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST GetParameterRequest
    );

/**
 * The NsiRpcRegisterChangeNotification routine registers a user-mode callback for NSI change notifications. Unlike
 * NsiRegisterChangeNotification it accepts a callback, because the notification is delivered by the NSI RPC server and
 * dispatched in the caller's process by winnsi.dll (used by System Informer for ICMP echo/ping notifications).
 *
 * \param RpcHandle An RPC binding handle from NsiConnectToServer (required; NULL fails with ERROR_INVALID_PARAMETER).
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object to observe (for ICMP, an NSI_ICMP_OBJECT_ID value).
 * \param CallbackRoutine Optional pointer to the callback routine.
 * \param NotifyCurrent TRUE to notify immediately of current state; FALSE otherwise.
 * \param CallerContext Optional context value passed to the callback routine.
 * \param NotificationHandle Optional pointer receiving a notification handle for NsiRpcDeregisterChangeNotification.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcRegisterChangeNotification(
    _In_ HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ PVOID CallbackRoutine,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Out_opt_ HANDLE *NotificationHandle
    );

/**
 * The NsiRpcRegisterChangeNotificationEx routine is the request-structure form of NsiRpcRegisterChangeNotification.
 *
 * \param RpcHandle An RPC binding handle from NsiConnectToServer (required; NULL fails with ERROR_INVALID_PARAMETER).
 * \param RegistrationRequest The registration request. When both ScopeOrCompartment fields are 0, winnsi.dll fills them in by
 *        querying the NDIS module (object 8) before registering.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcRegisterChangeNotificationEx(
    _In_ HANDLE RpcHandle,
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST RegistrationRequest
    );

/**
 * The NsiRpcSetAllParameters routine creates, updates, deletes or flushes an NSI object instance, locally or through the NSI
 * RPC server. Same parameters and results as NsiSetAllParameters.
 *
 * \param RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally
 *        through the matching nsi.dll routine (no RPC).
 * \param Store The NSI store to update. NsiCurrent is rejected over RPC (ERROR_INVALID_PARAMETER).
 * \param Action The set action (NSI_SET_ACTION); see NsiSetAllParameters.
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param Key Optional pointer to the key identifying the object instance.
 * \param KeySize The size of the key in bytes.
 * \param Rw Optional read-write row to write.
 * \param RwSize The size of Rw in bytes.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcSetAllParameters(
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

/**
 * The NsiRpcSetAllParametersEx routine is the request-structure form of NsiRpcSetAllParameters.
 *
 * \param RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally
 *        through the matching nsi.dll routine (no RPC).
 * \param SetAllParametersRequest A pointer to the request (passed to NsiSetAllParametersEx when RpcHandle is NULL).
 *        Store NsiCurrent is rejected over RPC.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks Over RPC the request must have MustBeZero == 0 (else ERROR_CALL_NOT_IMPLEMENTED) and a ModuleHandle or ModuleId
 *          (else ERROR_INVALID_PARAMETER).
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcSetAllParametersEx(
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST SetAllParametersRequest
    );

/**
 * The NsiRpcSetParameter routine writes one field of one NSI object instance, locally or through the NSI RPC server.
 * Same parameters and results as NsiSetParameter.
 *
 * \param RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally
 *        through the matching nsi.dll routine (no RPC).
 * \param Store The NSI store to update. NsiCurrent is rejected over RPC (ERROR_INVALID_PARAMETER).
 * \param Action The set action (NSI_SET_ACTION); see NsiSetParameter.
 * \param ModuleId A pointer to the NPI module identifier.
 * \param ObjectId The identifier of the NSI object.
 * \param Key A pointer to the key identifying the object instance.
 * \param KeySize The size of the key in bytes.
 * \param ParameterType The row that contains the field.
 * \param ParameterBuffer Optional buffer containing the new field value.
 * \param ParameterBufferSize The size of ParameterBuffer in bytes.
 * \param ParameterOffset The byte offset of the field within the ParameterType row.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcSetParameter(
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

/**
 * The NsiRpcSetParameterEx routine is the request-structure form of NsiRpcSetParameter.
 *
 * \param RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally
 *        through the matching nsi.dll routine (no RPC).
 * \param SetParameterRequest A pointer to the request (passed to NsiSetParameterEx when RpcHandle is NULL).
 *        Store NsiCurrent is rejected over RPC.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks Over RPC the request must have MustBeZero == 0 (else ERROR_CALL_NOT_IMPLEMENTED) and a ModuleHandle or ModuleId
 *          (else ERROR_INVALID_PARAMETER).
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcSetParameterEx(
    _In_opt_ HANDLE RpcHandle,
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST SetParameterRequest
    );

#endif
