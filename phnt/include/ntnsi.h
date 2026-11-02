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
#include <rpc.h>
#include <ws2tcpip.h>
#include <ws2def.h>
#include <windns.h>
#include <nldef.h>
#include <ndis/types.h>
#include <ndis/oidtypes.h>
#include <ntddndis.h>
#include <ip2string.h>
#include <ipexport.h>
#include <ipmib.h>
#include <netiodef.h>
#include <iphlpapi.h>
#include <mstcpip.h>
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

#ifndef _NETIODEF_
typedef GUID NPIID, *PNPIID;

/**
 * The NPI_MODULEID_TYPE values say how an NPI_MODULEID identifies its module: by GUID (all NSI providers) or by
 * interface LUID.
 */
typedef enum _NPI_MODULEID_TYPE
{
    MIT_GUID = 0x1, // The module identifier is stored in NPI_MODULEID.Guid.
    MIT_IF_LUID = 0x2 // The module identifier is stored in NPI_MODULEID.IfLuid.
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

#define NPI_MS_IPV4_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A00, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_IPV6_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A01, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_UDP_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A02, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_TCP_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A03, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_NMR_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A04, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_TUN_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A05, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL4L_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A0E, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL6L_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A0F, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_EQOS_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A1D, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_WANARPV4_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A09, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL48_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A0A, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL68_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A0B, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_WANARPV6_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A0C, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL_WANARP_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A0D, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_TCPIP_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A35, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_NDIS_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A11, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL4T_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A1B, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FL6T_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A1C, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FLRDMA_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A1E, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_RAW_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A07, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_WSK_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A0D, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_PACER_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A19, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_RSS_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A22, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_EQOSPROXY_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A2A, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_FSE_CLIENT_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A32, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_IPSEC_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xEB004A34, 0x9B1A, 0x11D4, { 0x91, 0x23, 0x00, 0x50, 0x04, 0x77, 0x59, 0xBC } } }
#define NPI_MS_IPS_LB_MODULEID_INIT { sizeof(NPI_MODULEID), MIT_GUID, { 0xD49FE21E, 0x1F02, 0x400F, { 0xA9, 0xCB, 0xD9, 0xC8, 0x0A, 0x6E, 0x2E, 0x69 } } }
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

#define NSI_ESTATS_SND_CONG_RW_VERSION 0
#define TCP_ESTATS_DATA_ROD_VERSION 0
#define TCP_ESTATS_DATA_RW_VERSION 0
#define TCP_ESTATS_SND_CONG_ROD_VERSION 0
#define TCP_ESTATS_SND_CONG_ROS_VERSION 0
#define TCP_ESTATS_SYN_OPTS_ROS_VERSION 0

#define NSI_IP_OBJECT_BANDWIDTH_RW_SIZE 0x30
#define NSI_IP_OBJECT_BANDWIDTH_ROS_SIZE 0x18
#define NSI_IP_OBJECT_ADDRESS_MANAGER_RW_SIZE 0x30
#define NSI_IP_OBJECT_ADDRESS_MANAGER_ROS_SIZE 0x18

#define NSI_DEVICE_NAME L"\\Device\\Nsi"

#define IOCTL_NSI_GET_PARAMETER                                             CTL_CODE(FILE_DEVICE_NETWORK, 0x1, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x120007; in: NSI_GET_PARAMETER_EX_REQUEST; out: NSI_GET_PARAMETER_EX_REQUEST
#define IOCTL_NSI_SET_PARAMETER                                             CTL_CODE(FILE_DEVICE_NETWORK, 0x2, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12000B; in: NSI_SET_PARAMETER_EX_REQUEST; out: NSI_SET_PARAMETER_EX_REQUEST
#define IOCTL_NSI_GET_ALL_PARAMETERS_EX                                     CTL_CODE(FILE_DEVICE_NETWORK, 0x3, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12000F; in: NSI_GET_ALL_PARAMETERS_EX_REQUEST; out: NSI_GET_ALL_PARAMETERS_EX_REQUEST
#define IOCTL_NSI_SET_ALL_PARAMETERS                                        CTL_CODE(FILE_DEVICE_NETWORK, 0x4, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x120013; in: NSI_SET_ALL_PARAMETERS_EX_REQUEST; out: NSI_SET_ALL_PARAMETERS_EX_REQUEST
#define IOCTL_NSI_GET_ALL_PARAMETERS                                        CTL_CODE(FILE_DEVICE_NETWORK, 0x6, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12001B; in: NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX; out: NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX
#define IOCTL_NSI_REGISTER_CHANGE_NOTIFICATION_EX                           CTL_CODE(FILE_DEVICE_NETWORK, 0x7, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12001F; in: NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST; out: NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST
#define IOCTL_NSI_DEREGISTER_CHANGE_NOTIFICATION_EX                         CTL_CODE(FILE_DEVICE_NETWORK, 0x8, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x120023; in: NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST; out: NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST
#define IOCTL_NSI_GET_OBJECT_SECURITY                                       CTL_CODE(FILE_DEVICE_NETWORK, 0xD, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x120037; in/out: NSI_OBJECT_SECURITY_REQUEST
#define IOCTL_NSI_SET_OBJECT_SECURITY                                       CTL_CODE(FILE_DEVICE_NETWORK, 0xE, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12003B; in/out: NSI_OBJECT_SECURITY_REQUEST
#define IOCTL_NSI_REQUEST_CHANGE_NOTIFICATION_EX                            CTL_CODE(FILE_DEVICE_NETWORK, 0xF, METHOD_NEITHER, FILE_ANY_ACCESS)    // 0x12003F; in: NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST; out: NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST
#define IOCTL_NSI_CANCEL_CHANGE_NOTIFICATION                                CTL_CODE(FILE_DEVICE_NETWORK, 0x10, METHOD_BUFFERED, FILE_ANY_ACCESS)  // 0x120040; in: HANDLE; out: none
#define IOCTL_NSI_ENUMERATE_OBJECTS_ALL_PERSISTENT_PARAMETERS_WITH_MASK     CTL_CODE(FILE_DEVICE_NETWORK, 0x11, METHOD_NEITHER, FILE_ANY_ACCESS)   // 0x120047; in: NSI_PERSISTENT_ENUMERATE_REQUEST; out: NSI_PERSISTENT_ENUMERATE_REQUEST
#define IOCTL_NSI_GET_ALL_PERSISTENT_PARAMETERS_WITH_MASK                   CTL_CODE(FILE_DEVICE_NETWORK, 0x12, METHOD_NEITHER, FILE_ANY_ACCESS)   // 0x12004B; in: NSI_PERSISTENT_GET_REQUEST; out: NSI_PERSISTENT_GET_REQUEST
#define IOCTL_NSI_SET_ALL_PERSISTENT_PARAMETERS_WITH_MASK                   CTL_CODE(FILE_DEVICE_NETWORK, 0x13, METHOD_NEITHER, FILE_ANY_ACCESS)   // 0x12004F; in: NSI_PERSISTENT_SET_REQUEST; out: NSI_PERSISTENT_SET_REQUEST

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
 * \remarks NsiEnumerateObjectsAllParametersEx rejects NsiBoth. NsiGetParameterEx rejects NsiBoth and any value above
 * NsiCurrent; NsiPersistent reads the registry copy (netio!NsipReadPersistentData) without calling the provider.
 */
typedef enum _NSI_STORE
{
    NsiPersistent,              // Registry-backed configuration reapplied at boot.
    NsiActive,                  // Active configuration used by the running system.
    NsiBoth,                    // Apply changes to both the active and persistent stores.
    NsiCurrent,                 // Active values merged with persistent defaults.
    NsiBootFirmwareTable        // Configuration supplied by firmware for network boot.
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
    NsiSetDefault = 0,                      // Update the parameters of an existing object.
    NsiSetCreateOnly = 1,                   // Create an object without updating an existing object.
    NsiSetCreateOrSet = 2,                  // Create an object or update its parameters if it already exists.
    NsiSetDelete = 3,                       // Delete the object identified by the key.
    NsiSetReset = 4,                        // Restore the stored parameters to their defaults.
    NsiSetClear = 5,                        // Clear the stored parameters.
    NsiSetCreateOrSetWithReference = 6,     // Create or update an object and retain a reference to it.
    NsiSetDeleteWithReference = 7,          // Release a reference to the object, deleting it when no references remain.
} NSI_SET_ACTION;

/**
 * The NSI_GET_ACTION enumeration selects which object instance a get or enumerate request returns. It is carried in the
 * request header (NSI_EX_REQUEST_COMMON.GetAction) and passed unchanged to the information provider.
 * \remarks netio.sys accepts 0-2 for NsiGetParameter(Ex)/NsiGetAllParameters(Ex), only NsiGetFirst and NsiGetNext
 * for NsiEnumerateObjectsAllParameters(Ex), and only NsiGetFirst for persistent-store enumeration; anything else fails with
 * STATUS_INVALID_PARAMETER (STATUS_NOT_IMPLEMENTED for the persistent store).
 */
typedef enum _NSI_GET_ACTION
{
    NsiGetExact = 0,            // The instance identified by the key (used by NsiGetParameter/NsiGetAllParameters).
    NsiGetFirst = 1,            // Start at the first instance (NsiAllocateAndGetTable and IP Helper always pass this).
    NsiGetNext = 2              // Continue with the instance that follows the key in the first KeyTable entry.
} NSI_GET_ACTION;

typedef ULONG NSI_OBJECT_ID, *PNSI_OBJECT_ID;

/**
 * The NSI_INTERFACE_INFORMATION_CLASS values select a conversion between interface identifiers (NET_LUID, interface
 * GUID and interface index). netsup.c uses NsiInterfaceIndexToLuid.
 */
typedef enum _NSI_INTERFACE_INFORMATION_CLASS
{
    NsiInterfaceLuidToGuid = 0,         // Convert an interface NET_LUID to its interface GUID.
    NsiInterfaceGuidToLuid = 1,         // Convert an interface GUID to its NET_LUID.
    NsiInterfaceIndexToLuid = 2,        // Convert an interface index to its NET_LUID.
} NSI_INTERFACE_INFORMATION_CLASS;

/**
 * The NSI_ICMP_OBJECT_ID values are the NSI object IDs of the ICMP echo provider (NPI_MS_IPV4_MODULEID or
 * NPI_MS_IPV6_MODULEID). NsiIcmpObjectEchoRequest is the object an ICMP handle registers a change notification on to
 * receive echo completions (ICMP_NOTIFICATION_MESSAGE).
 */
typedef enum _NSI_ICMP_OBJECT_ID
{
    NsiIcmpObjectEchoRequest = 4,       // ICMP echo request object used for completion notifications.
} NSI_ICMP_OBJECT_ID;

/**
 * The NSI_ICMP_SET_ACTION_OBJECT_ID values are not object IDs: they are the NSI_SET_ACTION values passed to
 * NsiSetAllParameters on NsiIcmpObjectEchoRequest. tcpip!Ipv4/Ipv6SetAllEchoRequestParameters switch on the set action:
 * NsiSetCreateOrSetWithReference submits an echo request (the key is ICMP_REQUEST_RW, followed by the ICMP_REQUEST_V4/V6
 * read/write block) and NsiSetDeleteWithReference acknowledges its completion; anything else fails with
 * STATUS_INVALID_PARAMETER. They are the same values as ICMP_NSI_SET_ALL_PARAMETERS_OBJECT_ID.
 */
typedef enum _NSI_ICMP_SET_ACTION_OBJECT_ID
{
    NsiIcmpSetActionObjectSubmitEchoRequest = NsiSetCreateOrSetWithReference,   // Submit an ICMP echo request.
    NsiIcmpSetActionObjectCompleteEchoRequest = NsiSetDeleteWithReference       // Acknowledge completion of an ICMP echo request.
} NSI_ICMP_SET_ACTION_OBJECT_ID;

/**
 * The NSI_FL_OBJECT_ID values are NSI object IDs of the framing-layer modules (NPI_MS_FL48_MODULEID for IPv4 and
 * NPI_MS_FL68_MODULEID for IPv6; tcpip!Fl48Start/Fl68Start). Each table has 12 objects; IDs not listed have no handlers.
 */
typedef enum _NSI_FL_OBJECT_ID
{
    NsiFlObjectGlobal = 0,                  // No key -- RW: NSI_FL_GLOBAL_RW -- ROD: NSI_FL_GLOBAL_ROD
    NsiFlObjectInterface = 2,               // Key: NET_LUID -- RW: NSI_FL_INTERFACE_RW
    NsiFlObjectWolPattern = 7,              // Key: NSI_FL_WOL_PATTERN_KEY; SetAll only (NsiSetCreateOnly adds, NsiSetDelete removes).
    NsiFlObjectNetBTGlobalTriage = 9,       // NPI_MS_FL48_MODULEID only. Key: NSI_FL_NETBT_GLOBAL_TRIAGE_KEY; SetAll only.
    NsiFlObjectVirtualInterface = 11        // Key: NSI_FL_VIRTUAL_INTERFACE_KEY -- RW 1 byte -- ROD: NSI_FL_VIRTUAL_INTERFACE_ROD -- ROS: NSI_FL_VIRTUAL_INTERFACE_ROS
} NSI_FL_OBJECT_ID;

/**
 * NsiGetParameter on an object whose provider registers no GetParameter handler (the IP objects below, and the NDIS
 * session, job and current-thread compartment objects) is served by netio!NsiGetParameterEx itself: it allocates the
 * whole registered block, calls the provider's GetAll handler for the requested parameter type and copies
 * [offset, offset + length) out of it. Any offset works as long as offset + length does not exceed the registered block
 * size (STATUS_INVALID_PARAMETER otherwise); there is no field-boundary check.
 */

/**
 * The NSI_IP_BEST_ROUTE_PARAMETER_ID values are byte offsets passed to NsiGetParameter to read fields of the best-route
 * read-only dynamic block (NsiIpObjectBestRoute, NsiParameterTypeReadOnlyDynamic; tcpip!Ipv4/Ipv6GetAllBestRouteParameters,
 * served through the GetAll fallback). GetBestInterfaceEx uses the BestIfIndex values.
 */
typedef enum _NSI_IP_BEST_ROUTE_PARAMETER_ID
{
    NsiIpv4BestRouteType1Key = 0x000,                   // MIB_IP_ROUTE_KEY_ROW_V4 Key (0x30 bytes).
    NsiIpv4BestRouteType1Route = 0x030,                 // MIB_IP_ROUTE_RW_ROW Route (0x1C bytes before Age).
    NsiIpv4BestRouteType1Age = 0x04C,                   // ULONG Age.
    NsiIpv4BestRouteType1Origin = 0x054,                // NL_ROUTE_ORIGIN Origin.
    NsiIpv4BestRouteType1BestIfIndex = 0x058,           // NET_IFINDEX InterfaceIndex.
    NsiIpv4BestRouteType1SourceScopeId = 0x05C,         // SCOPE_ID SourceScopeId.
    NsiIpv4BestRouteType1SourceAddress = 0x060,         // IN_ADDR SourceAddress.

    NsiIpv6BestRouteType1Key = 0x000,                   // MIB_IP_ROUTE_KEY_ROW_V6 Key (0x48 bytes).
    NsiIpv6BestRouteType1Route = 0x048,                 // MIB_IP_ROUTE_RW_ROW Route (0x1C bytes before Age).
    NsiIpv6BestRouteType1Age = 0x064,                   // ULONG Age.
    NsiIpv6BestRouteType1Origin = 0x078,                // NL_ROUTE_ORIGIN Origin.
    NsiIpv6BestRouteType1BestIfIndex = 0x07C,           // NET_IFINDEX InterfaceIndex.
    NsiIpv6BestRouteType1SourceScopeId = 0x080,         // SCOPE_ID SourceScopeId.
    NsiIpv6BestRouteType1SourceAddress = 0x084          // IN6_ADDR SourceAddress.
} NSI_IP_BEST_ROUTE_PARAMETER_ID;

/**
 * The NSI_IP_INTERFACE_PARAMETER_ID values are byte offsets passed to NsiGetParameter to read fields of an IP interface
 * (NsiIpObjectIpInterface; key: NET_LUID; tcpip!IpGetAllInterfaceParameters, served through the GetAll fallback).
 * Type0 values index NSI_IP_INTERFACE_RW, Type1 NSI_IP_INTERFACE_ROD and Type2 NSI_IP_INTERFACE_ROS.
 */
typedef enum _NSI_IP_INTERFACE_PARAMETER_ID
{
    NsiIpInterfaceType0AdvertisingEnabled = 0x000,                      // BOOLEAN AdvertisingEnabled.
    NsiIpInterfaceType0ForwardingEnabled = 0x001,                       // BOOLEAN ForwardingEnabled.
    NsiIpInterfaceType0WeakHostSend = 0x003,                            // BOOLEAN WeakHostSend.
    NsiIpInterfaceType0WeakHostReceive = 0x004,                         // BOOLEAN WeakHostReceive.
    NsiIpInterfaceType0UseNeighborUnreachabilityDetection = 0x005,      // BOOLEAN UseNeighborUnreachabilityDetection.
    NsiIpInterfaceType0UseAutomaticMetric = 0x006,                      // BOOLEAN UseAutomaticMetric.
    NsiIpInterfaceType0ManagedAddressConfigurationSupported = 0x00A,    // BOOLEAN ManagedAddressConfigurationSupported.
    NsiIpInterfaceType0OtherStatefulConfigurationSupported = 0x00B,     // BOOLEAN OtherStatefulConfigurationSupported.
    NsiIpInterfaceType0AdvertiseDefaultRoute = 0x00C,                   // BOOLEAN AdvertiseDefaultRoute.
    NsiIpInterfaceType0RouterDiscoveryBehavior = 0x014,                 // NL_ROUTER_DISCOVERY_BEHAVIOR RouterDiscoveryBehavior.
    NsiIpInterfaceType0Metric = 0x01C,                                  // ULONG Metric.
    NsiIpInterfaceType0BaseReachableTime = 0x020,                       // ULONG BaseReachableTime.
    NsiIpInterfaceType0RetransmitTime = 0x024,                          // ULONG RetransmitTime.
    NsiIpInterfaceType0PathMtuDiscoveryTimeout = 0x028,                 // ULONG PathMtuDiscoveryTimeout.
    NsiIpInterfaceType0DadTransmits = 0x02C,                            // ULONG DadTransmits.
    NsiIpInterfaceType0LinkLocalAddressBehavior = 0x030,                // NL_LINK_LOCAL_ADDRESS_BEHAVIOR LinkLocalAddressBehavior.
    NsiIpInterfaceType0LinkLocalAddressTimeout = 0x034,                 // ULONG LinkLocalAddressTimeout.
    NsiIpInterfaceType0ZoneIndices = 0x038,                             // ULONG ZoneIndices[ScopeLevelCount] (0x40 bytes), indexed by SCOPE_LEVEL; the entry for level L is at 0x038 + 4 * L.
    NsiIpInterfaceType0ZoneIndexInterface = 0x03C,                      // ZoneIndices[ScopeLevelInterface].
    NsiIpInterfaceType0ZoneIndexLink = 0x040,                           // ZoneIndices[ScopeLevelLink].
    NsiIpInterfaceType0ZoneIndexSubnet = 0x044,                         // ZoneIndices[ScopeLevelSubnet].
    NsiIpInterfaceType0ZoneIndexAdmin = 0x048,                          // ZoneIndices[ScopeLevelAdmin].
    NsiIpInterfaceType0ZoneIndexSite = 0x04C,                           // ZoneIndices[ScopeLevelSite].
    NsiIpInterfaceType0ZoneIndexOrganization = 0x058,                   // ZoneIndices[ScopeLevelOrganization].
    NsiIpInterfaceType0ZoneIndexGlobal = 0x070,                         // ZoneIndices[ScopeLevelGlobal].
    NsiIpInterfaceType0NlMtu = 0x078,                                   // ULONG NlMtu.
    NsiIpInterfaceType0SitePrefixLength = 0x07C,                        // ULONG SitePrefixLength.
    NsiIpInterfaceType0DisableDefaultRoutes = 0x098,                    // BOOLEAN DisableDefaultRoutes.

    NsiIpInterfaceType1InterfaceIndex = 0x000,                          // NET_IFINDEX InterfaceIndex.
    NsiIpInterfaceType1CompartmentId = 0x004,                           // NET_IF_COMPARTMENT_ID CompartmentId.
    NsiIpInterfaceType1SupportsWakeUpPatterns = 0x008,                  // BOOLEAN SupportsWakeUpPatterns.
    NsiIpInterfaceType1ReachableTime = 0x00C,                           // ULONG ReachableTime.
    NsiIpInterfaceType1Connected = 0x010,                               // LOGICAL Connected.
    NsiIpInterfaceType1TimerRemaining = 0x014,                          // ULONG TimerRemaining.
    NsiIpInterfaceType1FlInterfaceContext = 0x018,                      // ULONG64 FlInterfaceContext (0 for user-mode callers).
    NsiIpInterfaceType1TransmitOffload = 0x020,                         // NL_INTERFACE_OFFLOAD_ROD TransmitOffload (1 byte).
    NsiIpInterfaceType1ReceiveOffload = 0x021,                          // NL_INTERFACE_OFFLOAD_ROD ReceiveOffload (1 byte).
    NsiIpInterfaceType1InboundBandwidthInformation = 0x028,             // NL_BANDWIDTH_INFORMATION InboundBandwidthInformation.
    NsiIpInterfaceType1OutboundBandwidthInformation = 0x040,            // NL_BANDWIDTH_INFORMATION OutboundBandwidthInformation.

    NsiIpInterfaceType2SupportsNeighborDiscovery = 0x000,               // BOOLEAN SupportsNeighborDiscovery.
    NsiIpInterfaceType2SupportsRouterDiscovery = 0x001                  // BOOLEAN SupportsRouterDiscovery.
} NSI_IP_INTERFACE_PARAMETER_ID;

/**
 * The NSI_IP_OBJECT_ID values are the NSI object IDs of the IPv4 and IPv6 network-layer modules
 * (NPI_MS_IPV4_MODULEID and NPI_MS_IPV6_MODULEID). Each object has its own key and parameter blocks, for example
 * NsiIpObjectRoute (MIB_IP_ROUTE_*), NsiIpObjectNeighbor (MIB_IP_NEIGHBOR_*) and NsiIpObjectPath (NSI_IP_PATH_*).
 * Both modules register the same 37-entry table (tcpip!IppStartNsip); "IPv6 only" objects have no handlers in the IPv4
 * table, and IDs not listed have no handlers in either. Block sizes are IPv4/IPv6 where they differ.
 */
typedef enum _NSI_IP_OBJECT_ID
{
    NsiIpObjectBestRoute = 0,                       // Key: NSI_IP_BEST_ROUTE_KEY4 / NSI_IP_BEST_ROUTE_KEY6 -- ROD: NSI_IP_BEST_ROUTE_ROD_V4 / NSI_IP_BEST_ROUTE_ROD_V6
    NsiIpObjectCompartmentForwarding = 1,           // Key: NET_IF_COMPARTMENT_ID; no blocks. SetAll only: NsiSetCreateOrSetWithReference / NsiSetDeleteWithReference toggle a compartment flag.
    NsiIpObjectCompartment = 2,                     // Key: NET_IF_COMPARTMENT_ID -- RW: NSI_IP_COMPARTMENT_RW -- ROD: NSI_IP_COMPARTMENT_ROD
    NsiIpObjectIcmp = 3,                            // No key -- ROD: NSI_IP_ICMP_ROD (MIB_ICMP_EX_XPSP1)
    NsiIpObjectEchoRequest = 4,                     // Key: ICMP_REQUEST_RW -- RW: the part of ICMP_REQUEST_V4 / ICMP_REQUEST_V6 after ReadWrite (0x48/0x58 bytes on x64) -- ROD 0x1C bytes. Same as NsiIcmpObjectEchoRequest.
    NsiIpObjectEchoSequenceRequest = 5,             // No key -- ROD: NSI_IP_ECHO_SEQUENCE_REQUEST_ROD
    NsiIpObjectGlobal = 6,                          // No key; RW: NSI_IP_GLOBAL_RW -- ROD: NSI_IP_STATISTICS_ROD
    NsiIpObjectIpInterface = 7,                     // Key: NET_LUID; RW: NSI_IP_INTERFACE_RW -- ROD: NSI_IP_INTERFACE_ROD -- ROS: NSI_IP_INTERFACE_ROS
    NsiIpObjectAnycastAddress = 8,                  // Key: MIB_IP_ANYCAST_KEY_ROW_V4 / MIB_IP_ANYCAST_KEY_ROW_V6
    NsiIpObjectMulticastAddress = 9,                // Key: NSI_IP_MULTICAST_ADDRESS_KEY_V4 / _V6 -- ROD: NSI_IP_MULTICAST_ADDRESS_ROD -- ROS: NSI_IP_MULTICAST_ADDRESS_ROS
    NsiIpObjectUnicastAddress = 10,                 // Key: MIB_IP_UNICAST_KEY_ROW_V4 / MIB_IP_UNICAST_KEY_ROW_V6 -- RW: MIB_IP_UNICAST_RW_ROW -- ROD: MIB_IP_UNICAST_DYNAMIC_ROW -- ROS: MIB_IP_UNICAST_STATIC_ROW
    NsiIpObjectNeighbor = 11,                       // Key: MIB_IP_NEIGHBOR_KEY_ROW_V4 / MIB_IP_NEIGHBOR_KEY_ROW_V6 -- RW: MIB_IP_NEIGHBOR_RW_ROW -- ROD: MIB_IP_NEIGHBOR_DYNAMIC_ROW -- ROS: MIB_IP_NEIGHBOR_STATIC_ROW
    NsiIpObjectPath = 12,                           // Key: NSI_IP_PATH_KEY_V4 / NSI_IP_PATH_KEY_V6 -- ROD: NSI_IP_PATH_ROD_V4 / NSI_IP_PATH_ROD_V6.
    NsiIpObjectPotentialRouter = 13,                // IPv6 only. Key: NSI_IP_POTENTIAL_ROUTER_KEY -- ROD: NSI_IP_POTENTIAL_ROUTER_ROD
    NsiIpObjectPrefixPolicy = 14,                   // IPv6 only. Key: NSI_IP_PREFIX_POLICY_KEY -- RW: NSI_IP_PREFIX_POLICY_RW
    NsiIpObjectProxyNeighbor = 15,                  // Key: NSI_IP_PROXY_NEIGHBOR_KEY_V4 / _V6 -- RW: NSI_IP_PROXY_NEIGHBOR_RW
    NsiIpObjectRoute = 16,                          // Key: MIB_IP_ROUTE_KEY_ROW_V4 / MIB_IP_ROUTE_KEY_ROW_V6 -- RW: MIB_IP_ROUTE_RW_ROW -- ROD: MIB_IP_ROUTE_DYNAMIC_ROW_V4 / MIB_IP_ROUTE_DYNAMIC_ROW_V6 -- ROS: MIB_IP_ROUTE_STATIC_ROW
    NsiIpObjectSitePrefix = 17,                     // IPv6 only. Key: NSI_IP_SITE_PREFIX_KEY -- ROD: NSI_IP_SITE_PREFIX_ROD
    NsiIpObjectSubInterface = 18,                   // Key: NSI_IP_SUB_INTERFACE_KEY -- RW: NSI_IP_SUB_INTERFACE_RW -- ROD: NSI_IP_SUB_INTERFACE_ROD
    NsiIpObjectWakeUpPattern = 19,                  // Key: NSI_IP_WAKE_UP_PATTERN_KEY; SetAll only (NsiSetCreateOrSet adds, NsiSetDelete removes).
    NsiIpObjectResolveNeighbor = 20,                // Key: NSI_IP_RESOLVE_NEIGHBOR_KEY_V4 / _V6 -- RW: MIB_IP_NEIGHBOR_RW_ROW -- ROD: MIB_IP_NEIGHBOR_DYNAMIC_ROW -- ROS: MIB_IP_NEIGHBOR_STATIC_ROW (ResolveIpNetEntry2).
    NsiIpObjectSortedAddress = 21,                  // IPv6 only. Key: NSI_IP_SORTED_ADDRESS_KEY -- ROD: NSI_IP_SORTED_ADDRESS_ROD (CreateSortedAddressPairs).
    NsiIpObjectMulticastForwarding = 22,            // Key: NSI_IP_MULTICAST_FORWARDING_KEY_V4 / _V6 -- RW: NSI_IP_MULTICAST_FORWARDING_RW -- ROD: NSI_IP_MULTICAST_FORWARDING_ROD
    NsiIpObjectMulticastForwardingChange = 23,      // No key -- ROD: NSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V4 / _V6; change notifications only.
    NsiIpObjectInterfaceHop = 24,                   // Key: NET_LUID -- ROD: NSI_IP_INTERFACE_HOP_ROD
    NsiIpObjectInterfaceUnprivileged = 25,          // IPv6 only. Key: NET_LUID; SetAll only (NsiSetClear restarts router discovery).
    NsiIpObjectTunnelPhysicalInterface = 26,        // Key: NSI_IP_TUNNEL_PHYSICAL_INTERFACE_KEY -- RW 8 bytes (not used by GetAll) -- ROD: NSI_IP_TUNNEL_PHYSICAL_INTERFACE_ROD
    NsiIpObjectFeatureFallback = 27,                // IPv6 only. Key: NSI_IP_FEATURE_FALLBACK_KEY -- ROD: NSI_IP_FEATURE_FALLBACK_ROD; enumeration only.
    NsiIpObjectAddressManager = 28,                 // NSI_IP_OBJECT_ADDRESS_MANAGER_RW_SIZE / NSI_IP_OBJECT_ADDRESS_MANAGER_ROS_SIZE (no handlers)
    NsiIpObjectLocalityPrivate = 29,                // IPv6 only. No key -- RW: NSI_IP_LOCALITY_PRIVATE_RW; SetAll only.
    NsiIpObjectBandwidth = 30,                      // NSI_IP_OBJECT_BANDWIDTH_RW_SIZE / NSI_IP_OBJECT_BANDWIDTH_ROS_SIZE (no handlers)
    NsiIpObjectTimer = 31,                          // No key -- ROD: NSI_IP_TIMER_ROD
    NsiIpObjectDisconnectInterface = 32,            // Key: NSI_IP_DISCONNECT_INTERFACE_KEY; SetAll only (NsiSetReset).
    NsiIpObjectRouterInformation = 33,              // IPv6 only. Key: NET_LUID -- ROD: NSI_IP_ROUTER_INFORMATION_ROD
    NsiIpObjectReloadRegistryConfig = 34,           // No key or blocks; SetParameter reloads the registry configuration.
    NsiIpObjectSlaacSecretKey = 35,                 // IPv6 only. No key -- RW: NSI_IP_SLAAC_SECRET_KEY_RW (administrators and network operators only).
    NsiIpObjectRoutingEpoch = 36                    // No key -- ROD: NSI_IP_ROUTING_EPOCH_ROD; change notifications only.
} NSI_IP_OBJECT_ID;

/**
 * The NSI_NDIS_COMPARTMENT_GUID_PARAMETER_ID values are byte offsets for NsiNdisObjectCompartmentGuid (key: GUID, the
 * compartment GUID). NsiNdisCompartmentGuidType2Id reads the compartment ID from the read-only static block;
 * ConvertCompartmentGuidToId uses it.
 *
 * ndis.sys registers only a GetParameter handler (ndisNsiGetCompartmentIdForGuid); there is no GetAll or enumeration.
 * The request must use a 16-byte key, NsiParameterTypeReadOnlyStatic, offset 0, a length of exactly 4 and NsiGetExact;
 * anything else fails with STATUS_INVALID_PARAMETER. An unknown GUID fails with STATUS_OBJECT_NAME_NOT_FOUND.
 */
typedef enum _NSI_NDIS_COMPARTMENT_GUID_PARAMETER_ID
{
    NsiNdisCompartmentGuidType2Id = 0x000           // NET_IF_COMPARTMENT_ID CompartmentId (4 bytes).
} NSI_NDIS_COMPARTMENT_GUID_PARAMETER_ID;

/**
 * The NSI_NDIS_COMPARTMENT_PARAMETER_ID values are byte offsets into NDIS_NSI_COMPARTMENT_RW (NsiNdisObjectCompartment,
 * read/write block; key: MIB_NDIS_COMPARTMENT_KEY_ROW).
 *
 * ndis.sys (ndisNsiGetCompartmentInfo) accepts only these offsets; any other offset fails with STATUS_INVALID_PARAMETER,
 * and any block other than read/write fails with STATUS_INVALID_DEVICE_REQUEST. Each field is copied whole, so the
 * parameter length must be at least the field size; a longer length continues into the following fields. Reserved
 * (0x004) and SystemCompartment (0x64C) are not addressable and are skipped when a read continues past them.
 *
 * NsiGetExact looks up the key's compartment ID (STATUS_OBJECT_NAME_NOT_FOUND if absent). NsiGetFirst/NsiGetNext walk
 * the compartments in ID order, write the found ID back into the key and end with STATUS_NO_MORE_ENTRIES. A compartment
 * whose NamespaceGuid differs from the caller's is visible only when the caller is in NET_IF_COMPARTMENT_ID_PRIMARY and
 * the compartment is not NotEnumerable. The caller's compartment is the thread's, else its session's, else
 * NET_IF_COMPARTMENT_ID_PRIMARY.
 */
typedef enum _NSI_NDIS_COMPARTMENT_PARAMETER_ID
{
    NsiNdisCompartmentType0Header = 0x000,                          // NDIS_OBJECT_HEADER Header (4 bytes).
    NsiNdisCompartmentType0LoopbackInterfaceLuid = 0x008,           // NET_LUID LoopbackInterfaceLuid (8 bytes).
    NsiNdisCompartmentType0NetworkGuid = 0x010,                     // NET_IF_NETWORK_GUID NetworkGuid (16 bytes).
    NsiNdisCompartmentType0LoopbackInterfaceGuid = 0x020,           // GUID LoopbackInterfaceGuid (16 bytes).
    NsiNdisCompartmentType0LoopbackInterfaceAlias = 0x030,          // IF_COUNTED_STRING LoopbackInterfaceAlias (0x204 bytes).
    NsiNdisCompartmentType0LoopbackInterfaceDescription = 0x234,    // IF_COUNTED_STRING LoopbackInterfaceDescription (0x204 bytes).
    NsiNdisCompartmentType0CompartmentGuid = 0x438,                 // GUID CompartmentGuid (16 bytes).
    NsiNdisCompartmentType0CompartmentName = 0x448,                 // IF_COUNTED_STRING CompartmentName (0x204 bytes); a continued read skips SystemCompartment.
    NsiNdisCompartmentType0Flags = 0x650,                           // ULONG Flags (4 bytes).
    NsiNdisCompartmentType0NamespaceGuid = 0x654                    // GUID NamespaceGuid (16 bytes); last field.
} NSI_NDIS_COMPARTMENT_PARAMETER_ID;

/**
 * The NSI_NDIS_CURRENT_THREAD_COMPARTMENT_PARAMETER_ID values are byte offsets for NsiNdisObjectCurrentThreadCompartment
 * (no key; 8-byte read/write block: NET_IF_COMPARTMENT_ID CompartmentId, NET_IF_COMPARTMENT_SCOPE CompartmentScope).
 * NsiNdisCurrentThreadCompartmentType0Id is used by GetCurrentThreadCompartmentId and
 * NsiNdisCurrentThreadCompartmentType0Scope by GetCurrentThreadCompartmentScope.
 *
 * ndis.sys registers GetAll, SetParameter and SetAll handlers, read/write block only:
 * - GetAll (ndisNsiGetAllThreadInformation): returns the thread's compartment and scope. When the thread has no
 *   compartment, CompartmentId is the session's compartment (from the impersonation token's session, else the thread's),
 *   else NET_IF_COMPARTMENT_ID_PRIMARY, and CompartmentScope is 0.
 * - SetParameter (ndisNsiSetThreadInformation): NsiActive only (STATUS_NOT_IMPLEMENTED otherwise), NsiSetDefault and a
 *   non-NULL parameter; offset 0 calls NdisSetThreadObjectCompartmentId and offset 4 NdisSetThreadObjectCompartmentScope.
 *   Any other offset fails with STATUS_INVALID_PARAMETER.
 */
typedef enum _NSI_NDIS_CURRENT_THREAD_COMPARTMENT_PARAMETER_ID
{
    NsiNdisCurrentThreadCompartmentType0Id = 0x000,     // NET_IF_COMPARTMENT_ID CompartmentId (4 bytes).
    NsiNdisCurrentThreadCompartmentType0Scope = 0x004   // NET_IF_COMPARTMENT_SCOPE CompartmentScope (4 bytes).
} NSI_NDIS_CURRENT_THREAD_COMPARTMENT_PARAMETER_ID;

/**
 * The NSI_NDIS_INTERFACE_INDEX_PARAMETER_ID values are byte offsets for NsiNdisObjectInterfaceIndex (key: NET_IFINDEX).
 * NsiNdisInterfaceIndexToLuid reads the interface's NET_LUID from the read-only static block (ConvertInterfaceIndexToLuid).
 *
 * ndis.sys registers only a GetParameter handler (ndisNsiGetIfNameForIfIndex); there is no GetAll or enumeration.
 * The request must use a 4-byte key, NsiParameterTypeReadOnlyStatic, offset 0, a length of exactly 8 and NsiGetExact;
 * anything else fails with STATUS_INVALID_PARAMETER. An unknown index fails with STATUS_OBJECT_NAME_NOT_FOUND.
 */
typedef enum _NSI_NDIS_INTERFACE_INDEX_PARAMETER_ID
{
    NsiNdisInterfaceIndexToLuid = 0x000                 // NET_LUID InterfaceLuid (8 bytes).
} NSI_NDIS_INTERFACE_INDEX_PARAMETER_ID;

/**
 * The NSI_NDIS_INTERFACE_*_PARAMETER_ID values below are the byte offsets that the ndis.sys interface handlers accept for
 * NsiGetParameter on NsiNdisObjectInterface (and NsiNdisObjectInterfaceRaw) and NsiNdisObjectInterfaceWithoutStatistics
 * (and NsiNdisObjectInterfaceWithoutStatisticsRaw). Any other offset fails with STATUS_INVALID_PARAMETER. Each field is
 * copied whole, so the parameter length must be at least the field size; a longer length continues into the following
 * fields until the last one. Fields marked "provider OID" are queried from the interface provider when ndis.sys is not
 * the provider (or always, where noted) and are zeroed if that query fails.
 */

/**
 * The NSI_NDIS_INTERFACE_RW_PARAMETER_ID values are byte offsets into NDIS_NSI_INTERFACE_INFORMATION_RW
 * (NsiNdisObjectInterface, read/write block; ndisNsiGetInterfaceRwInformation). NsiNdisIfType0Alias is ifAlias
 * (ConvertInterfaceLuidToAlias).
 */
typedef enum _NSI_NDIS_INTERFACE_RW_PARAMETER_ID
{
    NsiNdisIfType0NetworkGuid = 0x000,                  // NET_IF_NETWORK_GUID NetworkGuid (16 bytes).
    NsiNdisIfType0AdminStatus = 0x010,                  // NET_IF_ADMIN_STATUS ifAdminStatus (4 bytes); always provider OID_GEN_ADMIN_STATUS.
    NsiNdisIfType0Alias = 0x014,                        // IF_COUNTED_STRING ifAlias (0x204 bytes); provider OID_GEN_ALIAS. Filter interfaces get a generated "<miniport>-<filter>-<suffix>" alias.
    NsiNdisIfType0PhysAddress = 0x218,                  // IF_PHYSICAL_ADDRESS ifPhysAddress (0x22 bytes); provider OID_802_3_CURRENT_ADDRESS.
    NsiNdisIfType0L2NetworkInfo = 0x23A,                // IF_COUNTED_STRING ifL2NetworkInfo (0x204 bytes); zeroed when not set.
    NsiNdisIfType0Purpose = 0x440                       // NDIS_NSI_INTERFACE_PURPOSE ifPurpose (4 bytes); last field.
} NSI_NDIS_INTERFACE_RW_PARAMETER_ID;

/**
 * The NSI_NDIS_INTERFACE_DYNAMIC_PARAMETER_ID values are byte offsets into NDIS_NSI_INTERFACE_INFORMATION_ROD
 * (NDIS_INTERFACE_INFORMATION; NsiNdisObjectInterface, read-only dynamic block; ndisNsiGetInterfaceRodInformation).
 * Most counters are provider OIDs. Offset 0 with a length of at least 0xD8 (non-filter interfaces) and offset 0x40 with a
 * length of at least 0x90 (NDIS-provided interfaces) are each satisfied by a single bulk provider query.
 */
typedef enum _NSI_NDIS_INTERFACE_DYNAMIC_PARAMETER_ID
{
    NsiNdisIfType1OperStatus = 0x000,                   // NET_IF_OPER_STATUS ifOperStatus (4 bytes); provider OID.
    NsiNdisIfType1OperStatusFlags = 0x004,              // ULONG ifOperStatusFlags (4 bytes).
    NsiNdisIfType1MediaConnectState = 0x008,            // NET_IF_MEDIA_CONNECT_STATE MediaConnectState (4 bytes).
    NsiNdisIfType1MediaDuplexState = 0x00C,             // NET_IF_MEDIA_DUPLEX_STATE MediaDuplexState (4 bytes).
    NsiNdisIfType1Mtu = 0x010,                          // ULONG ifMtu (4 bytes); always provider OID.
    NsiNdisIfType1PromiscuousMode = 0x014,              // BOOLEAN ifPromiscuousMode (1 byte).
    NsiNdisIfType1DeviceWakeUpEnable = 0x015,           // BOOLEAN ifDeviceWakeUpEnable (1 byte).
    NsiNdisIfType1XmitLinkSpeed = 0x018,                // ULONG64 XmitLinkSpeed (8 bytes).
    NsiNdisIfType1RcvLinkSpeed = 0x020,                 // ULONG64 RcvLinkSpeed (8 bytes).
    NsiNdisIfType1LastChange = 0x028,                   // ULONG64 ifLastChange (8 bytes); provider OID.
    NsiNdisIfType1CounterDiscontinuityTime = 0x030,     // ULONG64 ifCounterDiscontinuityTime (8 bytes); provider OID.
    NsiNdisIfType1InUnknownProtos = 0x038,              // ULONG64 ifInUnknownProtos (8 bytes); provider OID.
    NsiNdisIfType1InDiscards = 0x040,                   // ULONG64 ifInDiscards (8 bytes); provider OID.
    NsiNdisIfType1InErrors = 0x048,                     // ULONG64 ifInErrors (8 bytes); provider OID.
    NsiNdisIfType1HCInOctets = 0x050,                   // ULONG64 ifHCInOctets (8 bytes); provider OID.
    NsiNdisIfType1HCInUcastPkts = 0x058,                // ULONG64 ifHCInUcastPkts (8 bytes); provider OID.
    NsiNdisIfType1HCInMulticastPkts = 0x060,            // ULONG64 ifHCInMulticastPkts (8 bytes); provider OID.
    NsiNdisIfType1HCInBroadcastPkts = 0x068,            // ULONG64 ifHCInBroadcastPkts (8 bytes); provider OID.
    NsiNdisIfType1HCOutOctets = 0x070,                  // ULONG64 ifHCOutOctets (8 bytes); provider OID.
    NsiNdisIfType1HCOutUcastPkts = 0x078,               // ULONG64 ifHCOutUcastPkts (8 bytes); provider OID.
    NsiNdisIfType1HCOutMulticastPkts = 0x080,           // ULONG64 ifHCOutMulticastPkts (8 bytes); provider OID.
    NsiNdisIfType1HCOutBroadcastPkts = 0x088,           // ULONG64 ifHCOutBroadcastPkts (8 bytes); provider OID.
    NsiNdisIfType1OutErrors = 0x090,                    // ULONG64 ifOutErrors (8 bytes); provider OID.
    NsiNdisIfType1OutDiscards = 0x098,                  // ULONG64 ifOutDiscards (8 bytes); provider OID.
    NsiNdisIfType1HCInUcastOctets = 0x0A0,              // ULONG64 ifHCInUcastOctets (8 bytes); provider OID.
    NsiNdisIfType1HCInMulticastOctets = 0x0A8,          // ULONG64 ifHCInMulticastOctets (8 bytes); provider OID.
    NsiNdisIfType1HCInBroadcastOctets = 0x0B0,          // ULONG64 ifHCInBroadcastOctets (8 bytes); provider OID.
    NsiNdisIfType1HCOutUcastOctets = 0x0B8,             // ULONG64 ifHCOutUcastOctets (8 bytes); provider OID.
    NsiNdisIfType1HCOutMulticastOctets = 0x0C0,         // ULONG64 ifHCOutMulticastOctets (8 bytes); provider OID.
    NsiNdisIfType1HCOutBroadcastOctets = 0x0C8,         // ULONG64 ifHCOutBroadcastOctets (8 bytes); provider OID.
    NsiNdisIfType1CompartmentId = 0x0D0,                // NET_IF_COMPARTMENT_ID CompartmentId (4 bytes).
    NsiNdisIfType1SupportedStatistics = 0x0D4           // ULONG SupportedStatistics (4 bytes); last field.
} NSI_NDIS_INTERFACE_DYNAMIC_PARAMETER_ID;

/**
 * The NSI_NDIS_INTERFACE_STATIC_PARAMETER_ID values are byte offsets into NDIS_NSI_INTERFACE_INFORMATION_ROS
 * (NsiNdisObjectInterface, read-only static block). IP Helper's ConvertInterfaceLuidToIndex/ToGuid and the description
 * lookup use the first three.
 *
 * ndis.sys (ndisNsiGetInterfaceRosInformation) accepts only these offsets; any other offset fails with
 * STATUS_INVALID_PARAMETER. Each field is copied whole, so the parameter length must be at least the field size. A longer
 * length continues into the following fields.
 */
typedef enum _NSI_NDIS_INTERFACE_STATIC_PARAMETER_ID
{
    NsiNdisInterfaceLuidToIndex = 0x000,                // NET_IFINDEX ifIndex (4 bytes).
    NsiNdisInterfaceLuidToDescription = 0x004,          // IF_COUNTED_STRING ifDescr (0x204 bytes).
    NsiNdisInterfaceLuidToType = 0x208,                 // NET_IFTYPE ifType (2 bytes).
    NsiNdisInterfaceLuidToAccessType = 0x20C,           // NET_IF_ACCESS_TYPE AccessType (4 bytes).
    NsiNdisInterfaceLuidToDirectionType = 0x210,        // NET_IF_DIRECTION_TYPE DirectionType (4 bytes).
    NsiNdisInterfaceLuidToConnectionType = 0x214,       // NET_IF_CONNECTION_TYPE ConnectionType (4 bytes).
    NsiNdisInterfaceLuidToGuid = 0x218,                 // GUID InterfaceGuid (16 bytes).
    NsiNdisInterfaceLuidToConnectorPresent = 0x228,     // BOOLEAN ifConnectorPresent (1 byte).
    NsiNdisInterfaceLuidToPermanentPhysAddress = 0x22A, // IF_PHYSICAL_ADDRESS PermanentPhysAddress (0x22 bytes); for non-NDIS providers it is queried from the provider (OID_802_3_PERMANENT_ADDRESS).
    NsiNdisInterfaceLuidToFlags = 0x24C,                // ULONG ifFlags (4 bytes).
    NsiNdisInterfaceLuidToMediaType = 0x250,            // NDIS_MEDIUM MediaType (4 bytes).
    NsiNdisInterfaceLuidToPhysicalMediumType = 0x254    // NDIS_PHYSICAL_MEDIUM PhysicalMediumType (4 bytes); last field, the walk stops here.
} NSI_NDIS_INTERFACE_STATIC_PARAMETER_ID;

/**
 * The NSI_NDIS_INTERFACE_ENUM_DYNAMIC_PARAMETER_ID values are byte offsets into NDIS_NSI_INTERFACE_ENUM_ROD
 * (NsiNdisObjectInterfaceWithoutStatistics, read-only dynamic block; ndisNsiGetInterfaceRodEnumObject).
 */
typedef enum _NSI_NDIS_INTERFACE_ENUM_DYNAMIC_PARAMETER_ID
{
    NsiNdisIfEnumRodCompartmentId = 0x000,              // NET_IF_COMPARTMENT_ID CompartmentId (4 bytes).
    NsiNdisIfEnumRodNetworkGuid = 0x004,                // NET_IF_NETWORK_GUID NetworkGuid (16 bytes).
    NsiNdisIfEnumRodAlias = 0x014,                      // IF_COUNTED_STRING ifAlias (0x204 bytes); provider OID_GEN_ALIAS.
    NsiNdisIfEnumRodOperStatus = 0x218,                 // NET_IF_OPER_STATUS ifOperStatus (4 bytes); always provider OID.
    NsiNdisIfEnumRodOperStatusFlags = 0x21C,            // ULONG ifOperStatusFlags (4 bytes).
    NsiNdisIfEnumRodMtu = 0x220,                        // ULONG ifMtu (4 bytes); always provider OID.
    NsiNdisIfEnumRodPhysAddress = 0x224,                // IF_PHYSICAL_ADDRESS ifPhysAddress (0x22 bytes); provider OID_802_3_CURRENT_ADDRESS.
    NsiNdisIfEnumRodPermanentPhysAddress = 0x246,       // IF_PHYSICAL_ADDRESS PermanentPhysAddress (0x22 bytes); provider OID_802_3_PERMANENT_ADDRESS.
    NsiNdisIfEnumRodAdminStatus = 0x268,                // NET_IF_ADMIN_STATUS ifAdminStatus (4 bytes); always provider OID_GEN_ADMIN_STATUS.
    NsiNdisIfEnumRodXmitLinkSpeed = 0x270,              // ULONG64 XmitLinkSpeed (8 bytes).
    NsiNdisIfEnumRodRcvLinkSpeed = 0x278,               // ULONG64 RcvLinkSpeed (8 bytes).
    NsiNdisIfEnumRodPromiscuousMode = 0x280,            // BOOLEAN ifPromiscuousMode (1 byte).
    NsiNdisIfEnumRodDeviceWakeUpEnable = 0x281,         // BOOLEAN ifDeviceWakeUpEnable (1 byte).
    NsiNdisIfEnumRodHideInterfaceInUi = 0x282,          // BOOLEAN ifHideInterfaceInUi (1 byte).
    NsiNdisIfEnumRodMediaConnectState = 0x284,          // NET_IF_MEDIA_CONNECT_STATE MediaConnectState (4 bytes).
    NsiNdisIfEnumRodMediaDuplexState = 0x288            // NET_IF_MEDIA_DUPLEX_STATE MediaDuplexState (4 bytes); last field.
} NSI_NDIS_INTERFACE_ENUM_DYNAMIC_PARAMETER_ID;

/**
 * The NSI_NDIS_INTERFACE_ENUM_STATIC_PARAMETER_ID values are byte offsets into NDIS_NSI_INTERFACE_ENUM_ROS
 * (NsiNdisObjectInterfaceWithoutStatistics, read-only static block; ndisNsiGetInterfaceRosEnumObject).
 */
typedef enum _NSI_NDIS_INTERFACE_ENUM_STATIC_PARAMETER_ID
{
    NsiNdisIfEnumRosIndex = 0x000,                      // NET_IFINDEX ifIndex (4 bytes).
    NsiNdisIfEnumRosDescription = 0x004,                // IF_COUNTED_STRING ifDescr (0x204 bytes).
    NsiNdisIfEnumRosType = 0x208,                       // NET_IFTYPE ifType (2 bytes).
    NsiNdisIfEnumRosAccessType = 0x20C,                 // NET_IF_ACCESS_TYPE AccessType (4 bytes).
    NsiNdisIfEnumRosDirectionType = 0x210,              // NET_IF_DIRECTION_TYPE DirectionType (4 bytes).
    NsiNdisIfEnumRosConnectionType = 0x214,             // NET_IF_CONNECTION_TYPE ConnectionType (4 bytes).
    NsiNdisIfEnumRosGuid = 0x218,                       // GUID InterfaceGuid (16 bytes).
    NsiNdisIfEnumRosConnectorPresent = 0x228,           // BOOLEAN ifConnectorPresent (1 byte).
    NsiNdisIfEnumRosFlags = 0x22C,                      // ULONG ifFlags (4 bytes).
    NsiNdisIfEnumRosMediaType = 0x230,                  // NDIS_MEDIUM MediaType (4 bytes).
    NsiNdisIfEnumRosPhysicalMediumType = 0x234          // NDIS_PHYSICAL_MEDIUM PhysicalMediumType (4 bytes); last field.
} NSI_NDIS_INTERFACE_ENUM_STATIC_PARAMETER_ID;

/**
 * The NSI_NDIS_JOB_COMPARTMENT_PARAMETER_ID values are byte offsets into NDIS_NSI_JOB_COMPARTMENT_RW
 * (NsiNdisObjectJobCompartment; key: NDIS_NSI_JOB_COMPARTMENT_KEY). NsiNdisJobCompartmentType0Id is the compartment ID
 * assigned to a job object.
 *
 * ndis.sys registers only GetAll and SetAll handlers for this object (no enumeration), and only the read/write block. Both
 * reference the job handle from the caller's handle table with JOB_OBJECT_QUERY access (UserMode):
 * - GetAll (ndisNsiGetAllJobInformation): NdisGetJobObjectCompartmentId.
 * - SetAll (ndisNsiSetAllJobInformation): NdisSetJobObjectCompartmentId. Requires NsiActive, NsiSetDefault and a
 *   read/write block.
 */
typedef enum _NSI_NDIS_JOB_COMPARTMENT_PARAMETER_ID
{
    NsiNdisJobCompartmentType0Id = 0x000                // NET_IF_COMPARTMENT_ID CompartmentId (4 bytes).
} NSI_NDIS_JOB_COMPARTMENT_PARAMETER_ID;

/**
 * The NSI_NDIS_OBJECT_ID values are the NSI object IDs of the NDIS module (NPI_MS_NDIS_MODULEID): interfaces (with and
 * without statistics), index-to-LUID mapping, networks, compartments (by ID, by GUID, and the current thread's, session's
 * and job's compartment), the interface stack and isolation settings.
 */
typedef enum _NSI_NDIS_OBJECT_ID
{
    NsiNdisObjectInterface = 0,                         // Key: NET_LUID; RW: NDIS_NSI_INTERFACE_INFORMATION_RW; ROD: NDIS_NSI_INTERFACE_INFORMATION_ROD; ROS: NDIS_NSI_INTERFACE_INFORMATION_ROS.
    NsiNdisObjectInterfaceWithoutStatistics = 1,        // Key: NET_LUID; ROD: NDIS_NSI_INTERFACE_ENUM_ROD; ROS: NDIS_NSI_INTERFACE_ENUM_ROS.
    NsiNdisObjectInterfaceIndex = 2,                    // Key: NET_IFINDEX; ROS: NET_LUID.
    NsiNdisObjectInterfaceReceiveAddress = 3,           // Key: NDIS_NSI_RCV_ADDRESS_KEY; ROD: NDIS_NSI_RCV_ADDRESS_ROD (interface receive address table).
    NsiNdisObjectIfStackBasic = 4,                      // Key: MIB_NDIS_IFSTACK_KEY_ROW; enumeration only (ndisNsiEnumerateAllIfStackEntries).
    NsiNdisObjectInvertedIfStack = 5,                   // Key: MIB_NDIS_IFSTACK_KEY_ROW; enumeration only (ndisNsiEnumerateAllInvertedIfStackEntries).
    NsiNdisObjectNetwork = 6,                           // Key: NET_IF_NETWORK_GUID; RW: NDIS_NSI_NETWORK_RW.
    NsiNdisObjectCompartment = 7,                       // Key: MIB_NDIS_COMPARTMENT_KEY_ROW; RW: NDIS_NSI_COMPARTMENT_RW.
    NsiNdisObjectCurrentThreadCompartment = 8,          // key; None. RW: NET_IF_COMPARTMENT_ID.
    NsiNdisObjectSessionCompartment = 9,                // Key: NDIS_NSI_SESSION_COMPARTMENT_KEY; RW: NDIS_NSI_SESSION_COMPARTMENT_RW.
    NsiNdisObjectInterfacePersisted = 10,               // Key: NET_LUID; RW: NDIS_NSI_INTERFACE_PERSIST_RW. Persistent store only (no provider handlers).
    NsiNdisObjectCompartmentGuid = 11,                  // Key: GUID; ROS compartment ID: NET_IF_COMPARTMENT_ID;
    NsiNdisObjectInterfaceRaw = 12,                     // Key: NDIS_NSI_INTERFACE_RAW_KEY; RW: NDIS_NSI_INTERFACE_RAW_RW; ROD: NDIS_NSI_INTERFACE_RAW_ROD; ROS: NDIS_NSI_INTERFACE_RAW_ROS (get only, no enumeration).
    NsiNdisObjectInterfaceWithoutStatisticsRaw = 13,    // Key: NDIS_NSI_INTERFACE_ENUM_RAW_KEY; ROD: NDIS_NSI_INTERFACE_ENUM_RAW_ROD; ROS: NDIS_NSI_INTERFACE_ENUM_RAW_ROS.
    NsiNdisObjectIfStack = 14,                          // Key: MIB_NDIS_IFSTACK_KEY_ROW; enumeration only (ndisNsiEnumerateAllIfStackEntriesFull).
    NsiNdisObjectPersistentIsolation = 15,              // Key: NDIS_NSI_PERSISTENT_ISOLATION_KEY; RW: NDIS_NSI_PERSISTENT_ISOLATION_RW. Persistent store only (no provider handlers); used by tcpip.sys.
    NsiNdisObjectJobCompartment = 16,                   // Key: NDIS_NSI_JOB_COMPARTMENT_KEY; RW: NDIS_NSI_JOB_COMPARTMENT_RW.
    NsiNdisObjectTimestampConfigChange = 17             // Key: NET_LUID; ROD: NDIS_NSI_TIMESTAMP_ROD.
} NSI_NDIS_OBJECT_ID;

/**
 * The NSI_NDIS_SESSION_COMPARTMENT_PARAMETER_ID values are byte offsets into NDIS_NSI_SESSION_COMPARTMENT_RW
 * (NsiNdisObjectSessionCompartment; key: NDIS_NSI_SESSION_COMPARTMENT_KEY). NsiNdisSessionCompartmentType0Id is the
 * compartment ID assigned to a session.
 *
 * ndis.sys registers only GetAll, SetAll and EnumerateObjectsAll handlers for this object, and only the read/write block:
 * - GetAll (ndisNsiGetAllSessionInformation): NdisGetSessionCompartmentId.
 * - SetAll (ndisNsiSetAllSessionInformation): NdisSetSessionCompartmentId. Requires NsiActive and NsiSetDefault; a NULL
 *   read/write block sets compartment 0 (clears the assignment).
 * - EnumerateObjectsAll (ndisNsiEnumerateAllSessionInformation): one row per session that has an assigned compartment.
 */
typedef enum _NSI_NDIS_SESSION_COMPARTMENT_PARAMETER_ID
{
    NsiNdisSessionCompartmentType0Id = 0x000            // NET_IF_COMPARTMENT_ID CompartmentId (4 bytes).
} NSI_NDIS_SESSION_COMPARTMENT_PARAMETER_ID;

/**
 * The NSI_TABLE_ID values are the object IDs netsup.c passes to NsiAllocateAndGetTable to enumerate whole tables. The
 * module decides the meaning, so several names share a value (for example NsiTableIdUdp and NsiTableIdTcp are used with
 * NPI_MS_UDP_MODULEID and NPI_MS_TCP_MODULEID respectively).
 */
typedef enum _NSI_TABLE_ID
{
    NsiTableIdUdp = 1,                                  // UDP endpoint table in NPI_MS_UDP_MODULEID.
    NsiTableIdTcp = 3,                                  // TCP connection table in NPI_MS_TCP_MODULEID.
    NsiTableIdCompartment = 7,                          // Network compartment table in NPI_MS_NDIS_MODULEID.
    NsiTableIdIpAddress = 8,                            // Anycast address table in the IPv4 or IPv6 module.
    NsiTableIdAnycastAddress = NsiTableIdIpAddress,     // Alias for the anycast address table ID.
    NsiTableIdUnicastAddress = 10,                      // Unicast address table in the IPv4 or IPv6 module.
    NsiTableIdNeighbor = 11,                            // Neighbor table in the IPv4 or IPv6 module.
    NsiTableIdPrefixPolicy = 14,                        // Address selection prefix policy table in the IP module.
    NsiTableIdIfStack = NsiTableIdPrefixPolicy,         // Interface stack table in NPI_MS_NDIS_MODULEID.
    NsiTableIdRoute = 16                                // Routing table in the IPv4 or IPv6 module.
} NSI_TABLE_ID;

/**
 * The NSI_TCP_APPLIED_SETTING values identify the TCP setting template applied to a connection
 * (Get-NetTCPSetting: Internet, Datacenter, Compat and their custom variants). Reported in
 * NSI_TCP_STATUS_ROW.AppliedSetting.
 */
typedef enum _NSI_TCP_APPLIED_SETTING
{
    NsiTcpAppliedSettingInternet = 0,               // Internet TCP setting template.
    NsiTcpAppliedSettingDatacenter = 1,             // Datacenter TCP setting template.
    NsiTcpAppliedSettingCompat = 2,                 // Compatibility TCP setting template.
    NsiTcpAppliedSettingInternetCustom = 3,         // Custom Internet TCP setting template.
    NsiTcpAppliedSettingDatacenterCustom = 4,       // Custom Datacenter TCP setting template.
    NsiTcpAppliedSettingUnspecified = 6,            // IP Helper renders this value as an empty string.
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
    MibTcpEstatsTypeSynOpts = 6,        // TCP SYN option statistics.
    MibTcpEstatsTypeData = 7,           // TCP data transfer statistics.
    MibTcpEstatsTypeSndCong = 8,        // TCP sender congestion statistics.
    MibTcpEstatsTypePath = 9,           // TCP network path statistics.
    MibTcpEstatsTypeSendBuff = 10,      // TCP send buffer statistics.
    MibTcpEstatsTypeRec = 11,           // TCP local receiver statistics.
    MibTcpEstatsTypeObsRec = 12,        // TCP observed remote receiver statistics.
    MibTcpEstatsTypeBandwidth = 14,     // TCP bandwidth estimation statistics.
    MibTcpEstatsTypeFineRtt = 15        // TCP fine-grained round-trip time statistics.
} MIB_TCP_ESTATS_TYPE;

/**
 * The NSI_TCP_OBJECT_ID values are the NSI object IDs of the TCP module (NPI_MS_TCP_MODULEID): global statistics and
 * settings, connections, bound endpoints, and port reservations and exclusions. The table has 38 entries; IDs not listed
 * have no handlers. The port pool objects (18-20, 29, 34) are installed per compartment by tcpip!TcpCreateCompartment
 * (InetCreatePortPool). The connection ESTATS objects (6-15) are MIB_TCP_ESTATS_TYPE.
 */
typedef enum _NSI_TCP_OBJECT_ID
{
    NsiTcpObjectGlobal = 0,                     // Key: ADDRESS_FAMILY -- RW: NSI_TCP_GLOBAL_RW -- ROD: NETIO_TCP_STATS_DYNAMIC -- ROS: NSI_TCP_GLOBAL_ROS
    NsiTcpObjectInterface = 1,                  // Key: NET_LUID -- RW: NSI_TCP_INTERFACE_RW -- ROS: NSI_TCP_INTERFACE_ROS (optional).
    NsiTcpObjectInterfaceConnectionOffload = 2, // Key: NET_LUID -- ROD: NSI_TCP_INTERFACE_CONNECTION_OFFLOAD_ROD (always zeroed).
    NsiTcpObjectAllConnections = 3,             // Key: NSI_TCP_KEY_ROW -- ROD: NSI_TCP_STATUS_ROW -- ROS: NSI_TCP_OWNER_ROW; connections and listeners (TcpEnumerateAllConnections).
    NsiTcpObjectBasicConnections = 4,           // Same blocks; connections only (TcpEnumerateBasicConnections).
    NsiTcpObjectListeningConnections = 5,       // Same blocks; listeners only (TcpEnumerateListeningConnections).
    NsiTcpObjectConnection = 16,                // Key: NSI_TCP_KEY_ROW (NSI_SET_TCP_ENTRY); no blocks. SetAll only; SetTcpEntry deletes the TCB through it.
    NsiTcpObjectSecurity = 17,                  // No key -- RW: NSI_TCP_SECURITY_RW (netsh tcp security mpp/profiles) -- ROD: NSI_TCP_SECURITY_ROD
    NsiTcpObjectPortPool = 18,                  // Per-compartment port pool.
    NsiTcpObjectPortReservationGranting = 19,   // TCP port reservation granting object.
    NsiTcpObjectPortReservation = 20,           // TCP port reservation object.
    NsiTcpObjectPortProperty = 25,              // Key: NSI_TCP_PORT_PROPERTY_KEY -- RW: NSI_TCP_PORT_PROPERTY_RW
    NsiTcpObjectTemplate = 26,                  // Key: ULONG template index (0-4) -- RW: NSI_TCP_TEMPLATE_RW (TCP setting templates).
    NsiTcpObjectDefaultTemplate = 27,           // Key: ULONG, must be 6 -- RW: NSI_TCP_DEFAULT_TEMPLATE_RW
    NsiTcpObjectGlobalEx = 28,                  // Key: ADDRESS_FAMILY -- RW: NSI_TCP_GLOBAL_EX_RW -- ROS 1 byte.
    NsiTcpObjectPortExclusionGranting = 29,     // TCP port exclusion granting object.
    NsiTcpObjectRscStats = 30,                  // Key: NET_LUID -- ROD: NSI_TCP_RSC_STATS_ROD (receive segment coalescing state and statistics).
    NsiTcpObjectNcContext = 31,                 // Key: NSI_TCP_NC_CONTEXT_KEY -- RW 0x2C8 bytes (not decoded) -- ROD: NSI_TCP_NC_CONTEXT_ROD
    NsiTcpObjectRetransmitTimeout = 32,         // Key: NSI_TCP_KEY_ROW; no blocks; change notifications only.
    NsiTcpObjectPortExclusion = 34,             // TCP excluded port range object.
    NsiTcpObjectBoundEndpoint = 35,             // Key: NSI_TCP_KEY_ROW -- ROD: NSI_TCP_STATUS_ROW -- ROS: NSI_TCP_OWNER_ROW; bound endpoints, enumeration only.
    NsiTcpObjectReloadRegistryConfig = 36,      // No key or blocks; SetParameter reloads the registry configuration.
    NsiTcpObjectTraceRundown = 37               // No key or blocks; SetParameter emits the TCP trace rundown.
} NSI_TCP_OBJECT_ID;

/**
 * The NSI_UDP_OBJECT_ID values are the NSI object IDs of the UDP module (NPI_MS_UDP_MODULEID): global statistics,
 * endpoints, and port reservations and exclusions. The table has 7 entries. The port pool objects (2-5) are installed
 * per compartment by tcpip!UdpCreateCompartment (InetCreatePortPool).
 */
typedef enum _NSI_UDP_OBJECT_ID
{
    NsiUdpObjectGlobal = 0,                     // Key: ADDRESS_FAMILY -- RW: NSI_UDP_GLOBAL_RW -- ROD: NETIO_UDP_STATS_DYNAMIC
    NsiUdpObjectEndpoint = 1,                   // Key: NSI_UDP_KEY_ROW -- ROD: NSI_UDP_STATUS_ROW -- ROS: NSI_UDP_OWNER_ROW; enumeration only.
    NsiUdpObjectPortPool = 2,                   // Per-compartment port pool.
    NsiUdpObjectPortReservationGranting = 3,    // UDP port reservation granting object.
    NsiUdpObjectPortReservation = 4,            // UDP port reservation object.
    NsiUdpObjectPortExclusionGranting = 5,      // UDP port exclusion granting object.
    NsiUdpObjectUroStats = 6                    // Key: NET_LUID (exactly 8 bytes) -- ROD 8 bytes of UDP receive offload statistics (tcpip!UdpOffloadUroGetAllStats; layout not decoded).
} NSI_UDP_OBJECT_ID;

/**
 * The NSI_FL6T_OBJECT_ID values are the NSI object IDs of the Teredo framing-layer module (NPI_MS_FL6T_MODULEID;
 * tcpip!Fl6tStart). The table has 11 entries; IDs not listed have no handlers. NsiFl6tObjectClientPort is the Teredo
 * client port that NotifyTeredoPortChange watches.
 */
typedef enum _NSI_FL6T_OBJECT_ID
{
    NsiFl6tObjectTunnel = 0,                    // No key -- RW: NSI_FL6T_TUNNEL_RW -- ROD: NSI_FL6T_TUNNEL_ROD; change notifications.
    NsiFl6tObjectTeredo = 1,                    // No key -- RW: NSI_FL6T_TEREDO_RW; change notifications.
    NsiFl6tObjectInterface = 2,                 // Key: NET_LUID -- RW: NSI_FL_INTERFACE_RW
    NsiFl6tObjectClientPort = 3,                // No key -- RW: NSI_FL6T_CLIENT_PORT_RW; the Teredo client port NotifyTeredoPortChange watches.
    NsiFl6tObjectInterfaceRemoteAddress = 4,    // Key: NET_LUID -- RW: NSI_FL6T_INTERFACE_REMOTE_ADDRESS_RW
    NsiFl6tObjectTunnelPhysicalInterfaceAssociation = 6, // Key: NET_LUID -- RW: NSI_FL6T_TUNNEL_PHYSICAL_INTERFACE_ASSOCIATION_RW; SetAll only.
    NsiFl6tObjectTunnelPotentialRouter = 8,     // Key: NET_LUID -- RW: NSI_FL6T_TUNNEL_POTENTIAL_ROUTER_RW; SetAll only.
    NsiFl6tObjectTeredoShunt = 10               // No key -- RW: NSI_FL6T_TEREDO_SHUNT_RW
} NSI_FL6T_OBJECT_ID;

/**
 * The NSI_FL_RDMA_OBJECT_ID values are the NSI object IDs of the tcpip RDMA framing-layer provider (tcpip!FlRdmaStart).
 */
typedef enum _NSI_FL_RDMA_OBJECT_ID
{
    NsiFlRdmaObjectInterface = 0,               // Key: NET_IFINDEX -- ROD: NSI_FL_RDMA_INTERFACE_ROD -- ROS: NSI_FL_RDMA_INTERFACE_ROS; change notifications.
    NsiFlRdmaObjectGlobal = 1,                  // No key -- RW: NSI_FL_RDMA_GLOBAL_RW
    NsiFlRdmaObjectConnection = 2,              // Key: NSI_FL_RDMA_CONNECTION_KEY -- ROS: NSI_FL_RDMA_ENDPOINT_ROS; enumeration only.
    NsiFlRdmaObjectLocalEndpoint = 3            // Key: NSI_FL_RDMA_LOCAL_ENDPOINT_KEY -- ROS: NSI_FL_RDMA_ENDPOINT_ROS; enumeration only.
} NSI_FL_RDMA_OBJECT_ID;

/**
 * The NSI_RAW_OBJECT_ID values are the NSI object IDs of the raw IP transport module (NPI_MS_RAW_MODULEID).
 */
typedef enum _NSI_RAW_OBJECT_ID
{
    NsiRawObjectGlobal = 0                      // Key: ADDRESS_FAMILY (AF_INET or AF_INET6) -- ROD: NSI_RAW_GLOBAL_ROD (MIB_UDPSTATS2 layout).
} NSI_RAW_OBJECT_ID;

/**
 * The NSI_RSS_OBJECT_ID values are the NSI object IDs of the tcpip receive side scaling provider (tcpip!RssStartNsiProvider).
 */
typedef enum _NSI_RSS_OBJECT_ID
{
    NsiRssObjectInterface = 0                   // Key: NET_IFINDEX -- ROD: NSI_RSS_INTERFACE_ROD; change notifications.
} NSI_RSS_OBJECT_ID;

/**
 * The NSI_PARAMETER_TYPE values select which parameter block of an object NsiGetParameter and NsiSetParameter access:
 * the read/write block (configuration), the read-only dynamic block (state and statistics) or the read-only static block
 * (properties fixed when the object is created).
 */
typedef enum _NSI_PARAMETER_TYPE
{
    NsiParameterTypeReadWrite = 0,          // Configuration parameters that can be queried and changed.
    NsiParameterTypeReadOnlyDynamic = 1,    // Changing state and statistics available for queries.
    NsiParameterTypeReadOnlyStatic = 2      // Properties fixed when the object is created.
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
            SOCKADDR_INET LocalSockaddr;
            SOCKADDR_IN LocalTcpEntryIpV4;
            SOCKADDR_IN6 LocalTcpEntryIpV6;
        } DUMMYUNIONNAME;
        struct
        {
            ADDRESS_FAMILY LocalFamily;
            USHORT LocalPort;
            union
            {
                IN_ADDR LocalAddressIPv4;
                IPAddr LocalAddressIPv4Value;
            } DUMMYUNIONNAME3;
            union
            {
                IN6_ADDR LocalAddressIPv6;
                IPv6Addr LocalAddressIPv6Value;
            } DUMMYUNIONNAME5;
            SCOPE_ID LocalScopeID;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        union
        {
            SOCKADDR_INET RemoteSockaddr;
            SOCKADDR_IN RemoteTcpEntryIpV4;
            SOCKADDR_IN6 RemoteTcpEntryIpV6;
        } DUMMYUNIONNAME2;
        struct
        {
            ADDRESS_FAMILY RemoteFamily;
            USHORT RemotePort;
            union
            {
                IN_ADDR RemoteAddressIPv4;
                IPAddr RemoteAddressIPv4Value;
            } DUMMYUNIONNAME4;
            union
            {
                IN6_ADDR RemoteAddressIPv6;
                IPv6Addr RemoteAddressIPv6Value;
            } DUMMYUNIONNAME6;
            SCOPE_ID RemoteScopeID;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;
} NSI_SET_TCP_ENTRY, *PNSI_SET_TCP_ENTRY;

/**
 * The NSI_IP_INTERFACE_RW structure is the read/write row of an IP interface (NPI_MS_IPV4_MODULEID or
 * NPI_MS_IPV6_MODULEID, NsiIpObjectIpInterface; key: NET_LUID). The fields used by GetIpInterfaceEntry and
 * GetIpInterfaceTable map to MIB_IPINTERFACE_ROW.
 */
typedef struct _NSI_IP_INTERFACE_RW
{
    BOOLEAN AdvertisingEnabled;
    BOOLEAN ForwardingEnabled;
    UCHAR Reserved2;
    BOOLEAN WeakHostSend;
    BOOLEAN WeakHostReceive;
    BOOLEAN UseNeighborUnreachabilityDetection;
    BOOLEAN UseAutomaticMetric;
    UCHAR Reserved7[3];
    BOOLEAN ManagedAddressConfigurationSupported;
    BOOLEAN OtherStatefulConfigurationSupported;
    BOOLEAN AdvertiseDefaultRoute;
    UCHAR ReservedD[7];
    NL_ROUTER_DISCOVERY_BEHAVIOR RouterDiscoveryBehavior;
    ULONG Reserved18;
    ULONG Metric;
    ULONG BaseReachableTime;
    ULONG RetransmitTime;
    ULONG PathMtuDiscoveryTimeout;
    ULONG DadTransmits;
    NL_LINK_LOCAL_ADDRESS_BEHAVIOR LinkLocalAddressBehavior;
    ULONG LinkLocalAddressTimeout;
    ULONG ZoneIndices[ScopeLevelCount];
    ULONG NlMtu;
    ULONG SitePrefixLength;
    UCHAR Reserved80[0x18];
    BOOLEAN DisableDefaultRoutes;
    UCHAR Reserved99[0x57];
} NSI_IP_INTERFACE_RW, *PNSI_IP_INTERFACE_RW;

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
    LOGICAL Connected;                      // Non-zero when the interface is connected (MIB_IPINTERFACE_ROW.Connected).
    ULONG TimerRemaining;                   // Best-effort name: milliseconds until the interface's periodic timer fires
                                            //      (tcpip converts the remaining ticks with 500 ms per tick).
    union
    {
        ULONG64 FlInterfaceContext;         // Kernel address; tcpip returns 0 to user-mode callers.
        PVOID64 FlInterfaceContextPointer;
    } DUMMYUNIONNAME3;
    union
    {
        UCHAR TransmitOffload;                             // MIB_IPINTERFACE_ROW.TransmitOffload (NL_INTERFACE_OFFLOAD_ROD bits).
        NL_INTERFACE_OFFLOAD_ROD TransmitOffloadFlags;
    } DUMMYUNIONNAME;
    union
    {
        UCHAR ReceiveOffload;                              // MIB_IPINTERFACE_ROW.ReceiveOffload (NL_INTERFACE_OFFLOAD_ROD bits).
        NL_INTERFACE_OFFLOAD_ROD ReceiveOffloadFlags;
    } DUMMYUNIONNAME2;
    UCHAR Reserved2[6];
    NL_BANDWIDTH_INFORMATION InboundBandwidthInformation;  // MIB_IP_NETWORK_CONNECTION_BANDWIDTH_ESTIMATES.InboundBandwidthInformation.
    NL_BANDWIDTH_INFORMATION OutboundBandwidthInformation; // MIB_IP_NETWORK_CONNECTION_BANDWIDTH_ESTIMATES.OutboundBandwidthInformation.
} NSI_IP_INTERFACE_ROD, *PNSI_IP_INTERFACE_ROD;

/**
 * The NSI_IP_INTERFACE_ROS structure is the read-only static row of an IP interface.
 */
typedef struct _NSI_IP_INTERFACE_ROS
{
    BOOLEAN SupportsNeighborDiscovery;
    BOOLEAN SupportsRouterDiscovery;
    UCHAR Reserved[0x1e];
} NSI_IP_INTERFACE_ROS, *PNSI_IP_INTERFACE_ROS;

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
                                    //      "TCPIP Extended Performance Diagnostics\TWTCB collisions").
    ULONG64 InSegs;                  // MIB_TCPSTATS2.dw64InSegs
    ULONG64 OutSegs;                 // MIB_TCPSTATS2.dw64OutSegs
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
 * The NSI_TCP_GLOBAL_HEADER structure is the shared 16-byte RTO and connection-limit prefix of TCP statistics.
 */
typedef struct _NSI_TCP_GLOBAL_HEADER
{
    TCP_RTO_ALGORITHM RtoAlgorithm;
    ULONG RtoMin;
    ULONG RtoMax;
    ULONG MaxConn;
} NSI_TCP_GLOBAL_HEADER, *PNSI_TCP_GLOBAL_HEADER;

/**
 * The NSI_TCP_GLOBAL_ROS structure is the read-only static parameter block of the TCP global object
 * (NPI_MS_TCP_MODULEID, NsiTcpObjectGlobal).
 * \remarks TcpGetAllGlobalParameters requires at least sizeof(NSI_TCP_GLOBAL_ROS) (20) bytes; GetTcpStatisticsEx2 copies the first 16 bytes.
 */
typedef struct _NSI_TCP_GLOBAL_ROS
{
    union
    {
        NSI_TCP_GLOBAL_HEADER GlobalParameters;
        struct
        {
            TCP_RTO_ALGORITHM RtoAlgorithm;             // MIB_TCPSTATS2.RtoAlgorithm (TCP_RTO_ALGORITHM)
            ULONG RtoMin;                               // MIB_TCPSTATS2.dwRtoMin
            ULONG RtoMax;                               // MIB_TCPSTATS2.dwRtoMax
            ULONG MaxConn;                              // MIB_TCPSTATS2.dwMaxConn
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    BOOLEAN AutoTuningLevelLocalSet;            // TRUE once NSI_TCP_GLOBAL_RW.AutoTuningLevelLocal was set to a level (not -2).
                                                //      While FALSE and no group policy level is set, TcpSelectRcvWndScaleFactor may pick the
                                                //      next higher receive-window scale. TcpUpdateGlobalParameters writes it.
    UCHAR Reserved[3];                          // alignment padding; nothing reads or writes it
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
    BOOLEAN EnablePMtuDiscovery;        // Default TRUE.
    BOOLEAN UseRfc1122UrgentPointer;    // Default FALSE.
    BOOLEAN DisableTaskOffload;         // Default FALSE.
    BOOLEAN EnablePMtuBHDetect;         // PMTU black-hole detection. Default TRUE.
    BOOLEAN DisableTcpChimneyOffload;   // Legacy chimney offload setting. Ignored on a set; always reported as 0.
    BOOLEAN DisableRss;                 // Receive-side scaling. A get reports the global RSS state, not a per-family value.
    BOOLEAN EcnCapability;              // Explicit Congestion Notification.
    UCHAR MaxDataRetransmissions;       // Default 5.
    ULONG KeepAliveTime;                // Milliseconds. Default 7200000.
    ULONG KeepAliveInterval;            // Milliseconds. Default 1000.
    ULONG TimedWaitDelay;               // Seconds, 2..300. Default 120.
    ULONG SillyWindowTimeout;           // Milliseconds, 200..60000. Default 5000.
    ULONG FinWait2Timeout;              // Seconds, 30..2073600. Default 120.
    UCHAR CongestionAlgorithm;          // 0..7. Default 0.
    UCHAR UseRfc1323Timestamps;         // 0 = disabled, 1 = enabled, 2 = allowed (default).
    UCHAR Reserved1[2];                 // alignment padding; nothing reads or writes it
    LONG AutoTuningLevelLocal;          // Receive-window auto-tuning level, -2..4 (-2 = not configured). Default 3.
    LONG AutoTuningLevelGroupPolicy;    // Group-policy auto-tuning level, -2..4 (-2 = not configured). Default -2.
    BOOLEAN DisableRsc;                 // Receive segment coalescing (OlmUpdateTcpGlobalParameters).
    BOOLEAN NotifyOnAllRetransmitTimeouts;
    UCHAR Reserved2[2];                 // alignment padding; nothing reads or writes it
    ULONG PacingProfile;                // TCP pacing profile, 0..4; TcpCacheAfOptions copies it into each TCB unless the
                                        //      TcpPacingRate override is set. Default 0. Not in tcpip's ETW events (best-effort name).
} NSI_TCP_GLOBAL_RW, *PNSI_TCP_GLOBAL_RW;

/**
 * The NETIO_UDP_STATS_DYNAMIC structure is the read-only dynamic parameter block of the UDP global object
 * (NPI_MS_UDP_MODULEID, NsiUdpObjectGlobal; key: ADDRESS_FAMILY). Tcpip sums its per-processor UDP statistics into it.
 * \remarks  SwSegments and HwSegments use the names of tcpip's "UDP: Endpoint segment message" ETW event.
 */
typedef struct _NETIO_UDP_STATS_DYNAMIC
{
    ULONG64 InDatagrams;                // MIB_UDPSTATS2.dw64InDatagrams
    ULONG NoPorts;                      // MIB_UDPSTATS2.dwNoPorts
    ULONG InErrors;                     // MIB_UDPSTATS2.dwInErrors
    ULONG64 OutDatagrams;               // MIB_UDPSTATS2.dw64OutDatagrams
    ULONG NumAddrs;                     // MIB_UDPSTATS2.dwNumAddrs
    ULONG Reserved;                     // alignment padding; nothing reads or writes it
    ULONG64 SwSegments;                 // Datagrams that tcpip split in software from a segmented (UDP_SEND_MSG_SIZE) send.
    ULONG64 HwSegments;                 // Datagrams the adapter produced through UDP segmentation offload (USO).
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
 * \remarks Mirrors the 0x48-byte block iphlpapi.dll!IcmpSendEcho2Ex allocates and links into ICMP_HANDLE_INTERNAL.RequestList.
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
 * \remarks Mirrors the 0x50-byte block iphlpapi.dll!IcmpCreateFileEx allocates.
 */
typedef struct _ICMP_HANDLE_INTERNAL
{
    ULONG ProcessId;
    BOOLEAN IsIpv4;
    UCHAR Reserved0[3];              // alignment padding; nothing reads or writes it
    CONST NPI_MODULEID* ModuleId;
    RTL_CRITICAL_SECTION Lock;
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
    NTSTATUS Status;
    LOGICAL HasIcmpError;
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
    ULONG DataLength;                   // Bytes of echo data in the reply (tcpip writes a ULONG; IP Helper reads the low USHORT).
    UCHAR Ttl;                          // TTL of the reply (copied to ICMP_ECHO_REPLY.Options.Ttl)
    UCHAR Tos;                          // TOS of the reply (copied to ICMP_ECHO_REPLY.Options.Tos)
    UCHAR Reserved[2];                  // alignment padding; nothing reads or writes it
    ULONG RoundTripTime;                // milliseconds
    union
    {
        IPAddr Address;                 // replying address
        IN_ADDR AddressV4;
    } DUMMYUNIONNAME3;
    union
    {
        UCHAR Flags;                    // IP_FLAG_REVERSE/IP_FLAG_DF; copied to ICMP_ECHO_REPLY.Options.Flags.
        struct
        {
            UCHAR Reverse : 1;
            UCHAR DontFragment : 1;
            UCHAR ReservedFlags : 6;
        } FlagsBits;
    } DUMMYUNIONNAME4;
    UCHAR OptionsSize;
} ICMP_NOTIFICATION_V4, *PICMP_NOTIFICATION_V4;

/**
 * The ICMP_NOTIFICATION_V6 structure is the IPv6 echo-completion notification. IP Helper converts it into an
 * ICMPV6_ECHO_REPLY followed by the echo data.
 */
typedef struct _ICMP_NOTIFICATION_V6
{
    ICMP_NOTIFICATION_HEADER Header;
    ULONG DataLength;                   // Bytes of echo data in the reply.
    UCHAR HopLimit;                     // Hop limit of the reply's IPv6 header; IP Helper does not report it.
    UCHAR TrafficClass;                 // Traffic class of the reply's IPv6 header; IP Helper does not report it.
    UCHAR Reserved[2];                  // alignment padding; nothing reads or writes it
    ULONG RoundTripTime;                // milliseconds
    union
    {
        IN6_ADDR Address;               // replying address
        IPv6Addr AddressValue;
    } DUMMYUNIONNAME;
    SCOPE_ID ScopeId;
} ICMP_NOTIFICATION_V6, *PICMP_NOTIFICATION_V6;

/**
 * The ICMP_NOTIFICATION_MESSAGE structure is the parameter-data argument of the ICMP NSI change-notification callback.
 * NotificationData points to an ICMP_NOTIFICATION_V4 or ICMP_NOTIFICATION_V6 depending on the module the handle was
 * registered with.
 */
typedef struct _ICMP_NOTIFICATION_MESSAGE
{
    ULONG NotificationDataLength;   // Best-effort name: length half of the RPC counted buffer winnsi.dll unmarshals
                                    // NotificationData into (NsiWorkerThread);
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
    UCHAR Reserved0[2];                 // alignment padding; nothing reads or writes it
    SCOPE_ID SourceScopeId;             // Scope of SourceAddress; tcpip uses it only when SourceAddress != 0.
    SCOPE_ID DestinationScopeId;        // Scope of DestinationAddress.
    UCHAR Ttl;
    BOOLEAN Reverse;                    // IP_FLAG_REVERSE
    UCHAR Reserved1[2];                 // alignment padding; nothing reads or writes it
    BOOLEAN DontFragment;               // IP_FLAG_DF
    UCHAR Reserved2[3];                 // alignment padding; nothing reads or writes it
    union
    {
        IPAddr SourceAddress;
        IN_ADDR SourceAddressV4;
    } DUMMYUNIONNAME5;
    union
    {
        IPAddr DestinationAddress;
        IN_ADDR DestinationAddressV4;
    } DUMMYUNIONNAME6;
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
    SCOPE_ID SourceScopeId;
    SCOPE_ID DestinationScopeId;
    UCHAR Ttl;
    BOOLEAN Reverse;
    UCHAR Reserved2[2];             // alignment padding; nothing reads or writes it
    union
    {
        IN6_ADDR SourceAddress;
        IPv6Addr SourceAddressValue;
    } DUMMYUNIONNAME;
    union
    {
        IN6_ADDR DestinationAddress;
        IPv6Addr DestinationAddressValue;
    } DUMMYUNIONNAME2;
} ICMP_REQUEST_V6, *PICMP_REQUEST_V6;

/**
 * The ICMP_NSI_SET_ALL_PARAMETERS_OBJECT_ID values are the NSI_SET_ACTION values netsup.c passes to NsiSetAllParameters
 * on NsiIcmpObjectEchoRequest to submit an echo request and to acknowledge its completion (see
 * NSI_ICMP_SET_ACTION_OBJECT_ID; despite the name they are not object IDs).
 */
typedef enum _ICMP_NSI_SET_ALL_PARAMETERS_OBJECT_ID
{
    IcmpNsiSetAllObjectSubmitEchoRequest = NsiSetCreateOrSetWithReference, // Submit an ICMP echo request with its read/write parameters.
    IcmpNsiSetAllObjectCompleteEchoRequest = NsiSetDeleteWithReference     // Acknowledge completion of an ICMP echo request.
} ICMP_NSI_SET_ALL_PARAMETERS_OBJECT_ID;

/**
 * The NSI_COUNTED_WCHAR_PARAMETER structure is a counted Unicode string as NSI providers return it for single string
 * parameters (interface alias and description, compartment and network names). Length is in bytes and the buffer is not
 * necessarily null-terminated. It has the same layout as IF_COUNTED_STRING (516 bytes).
 */
typedef struct _NSI_COUNTED_WCHAR_PARAMETER
{
    union
    {
        IF_COUNTED_STRING CountedString;
        struct
        {
            USHORT Length;
            _Field_size_part_(257, Length / sizeof(WCHAR))
            WCHAR Buffer[257];
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
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
 * The NSI_OBJECT_SECURITY_REQUEST structure selects an NSI information object and supplies its security descriptor.
 * NsiGetObjectSecurity writes SecurityDescriptorLength and, when the buffer is large enough, SecurityDescriptor.
 * NsiSetObjectSecurity reads SecurityDescriptor; a zero SecurityInformation selects DACL_SECURITY_INFORMATION.
 */
typedef struct _NSI_OBJECT_SECURITY_REQUEST
{
    ULONG ModuleHandle;             // 0 = look the module up by ModuleId
    PNPI_MODULEID ModuleId;
    NSI_OBJECT_ID ObjectId;
#ifdef _WIN64
    ULONG Padding;                  // alignment padding in the x64 IOCTL buffer
#endif
    SECURITY_INFORMATION SecurityInformation;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG SecurityDescriptorLength;
} NSI_OBJECT_SECURITY_REQUEST, *PNSI_OBJECT_SECURITY_REQUEST;

static_assert(sizeof(NSI_OBJECT_SECURITY_REQUEST) == 6 * sizeof(PVOID));

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
    HANDLE NotificationHandle;
    ULONG ProcessId;
    ULONG Reserved24;               // alignment padding; nothing reads or writes it
} NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST, *PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST;

/**
 * The NSI_REQUEST_HEADER structure is the common identity prefix of extended NSI requests.
 * Four-byte packing excludes tail padding on x86; the containing requests retain their original alignment.
 */
#include <pshpack4.h>
typedef struct _NSI_REQUEST_HEADER
{
    ULONGLONG MustBeZero;           // Must be zero.
    ULONG ModuleHandle;             // 0 selects the module by ModuleId.
    ULONG Reserved0C;
    PNPI_MODULEID ModuleId;
    NSI_OBJECT_ID ObjectId;
    ULONG Reserved1C;
} NSI_REQUEST_HEADER, *PNSI_REQUEST_HEADER;
#include <poppack.h>

/**
 * The NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX structure is the input of NsiEnumerateObjectsAllParametersEx. It
 * describes up to four parallel tables (key, read/write, read-only dynamic, read-only static) that the provider fills with
 * one row per object; on input Count is the number of rows the tables can hold, on output the number returned.
 */
typedef struct _NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX
{
    union
    {
        NSI_REQUEST_HEADER RequestHeader;
        struct
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
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
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
    union
    {
        SOCKADDR_INET LocalSockaddr;
        SOCKADDR_IN LocalSockaddrV4;
        SOCKADDR_IN6 LocalSockaddrV6;
        struct
        {
            ADDRESS_FAMILY AddressFamily;
            USHORT LocalPort;
            union
            {
                IN_ADDR LocalAddressIPv4;
                IPAddr LocalAddressIPv4Value;
            } DUMMYUNIONNAME3;
            union
            {
                IN6_ADDR LocalAddressIPv6;
                IPv6Addr LocalAddressIPv6Value;
            } DUMMYUNIONNAME5;
            SCOPE_ID LocalScopeId;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        SOCKADDR_INET RemoteSockaddr;
        SOCKADDR_IN RemoteSockaddrV4;
        SOCKADDR_IN6 RemoteSockaddrV6;
        struct
        {
            ADDRESS_FAMILY RemoteAddressFamily;
            USHORT RemotePort;
            union
            {
                IN_ADDR RemoteAddressIPv4;
                IPAddr RemoteAddressIPv4Value;
            } DUMMYUNIONNAME4;
            union
            {
                IN6_ADDR RemoteAddressIPv6;
                IPv6Addr RemoteAddressIPv6Value;
            } DUMMYUNIONNAME6;
            SCOPE_ID RemoteScopeId;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;
} NSI_ESTATS_TCP_KEY, *PNSI_ESTATS_TCP_KEY;

/**
 * The NSI_ESTATS_TCP_KEY_V4 structure is the AF_INET view of NSI_ESTATS_TCP_KEY. System Informer fills it from a
 * MIB_TCPROW to query the extended statistics of an IPv4 connection.
 */
typedef struct _NSI_ESTATS_TCP_KEY_V4
{
    union
    {
        SOCKADDR_IN LocalSockaddrV4;
        struct
        {
            ADDRESS_FAMILY AddressFamily;   // AF_INET
            USHORT LocalPort;
            union
            {
                IPAddr LocalAddress;
                IN_ADDR LocalAddressV4;
            } DUMMYUNIONNAME3;
            ULONG Reserved08[5];            // unused IPv6 address and scope slots of the AF_INET key; nothing reads or writes it
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        SOCKADDR_IN RemoteSockaddrV4;
        struct
        {
            ADDRESS_FAMILY RemoteAddressFamily; // AF_INET
            USHORT RemotePort;
            union
            {
                IPAddr RemoteAddress;
                IN_ADDR RemoteAddressV4;
            } DUMMYUNIONNAME4;
            ULONG Reserved24[5];            // unused IPv6 address and scope slots of the AF_INET key; nothing reads or writes it
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;
} NSI_ESTATS_TCP_KEY_V4, *PNSI_ESTATS_TCP_KEY_V4;

/**
 * The NSI_EX_REQUEST_COMMON structure is the header shared by the *Ex request blocks that nsi.dll passes to netio.sys
 * (NsiGetParameterEx, NsiGetAllParametersEx, NsiSetParameterEx, NsiSetAllParametersEx). It selects the module, object and
 * store, and the get or set action.
 */
typedef struct _NSI_EX_REQUEST_COMMON
{
    union
    {
        NSI_REQUEST_HEADER RequestHeader;
        struct
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
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} NSI_EX_REQUEST_COMMON, *PNSI_EX_REQUEST_COMMON;

/**
 * The NSI_PERSISTENT_GET_REQUEST structure is the IOCTL request for persistent parameter data and its mask.
 */
typedef struct _NSI_PERSISTENT_GET_REQUEST
{
    union
    {
        NSI_REQUEST_HEADER RequestHeader;
        struct
        {
            ULONGLONG MustBeZero;
            ULONG ModuleHandle;
            ULONG Reserved0C;
            PNPI_MODULEID ModuleId;
            NSI_OBJECT_ID ObjectId;
            ULONG Reserved1C;
            PVOID Key;
            ULONG KeySize;
            ULONG Reserved2C;
            PVOID ParameterData;
            PVOID ParameterMask;
            ULONG ParameterDataSize;
            ULONG Reserved44;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} NSI_PERSISTENT_GET_REQUEST, *PNSI_PERSISTENT_GET_REQUEST;

/**
 * The NSI_PERSISTENT_SET_REQUEST structure is the IOCTL request to change persistent parameter data and its mask.
 */
typedef struct _NSI_PERSISTENT_SET_REQUEST
{
    union
    {
        NSI_REQUEST_HEADER RequestHeader;
        struct
        {
            ULONGLONG MustBeZero;
            ULONG ModuleHandle;
            ULONG Reserved0C;
            PNPI_MODULEID ModuleId;
            NSI_OBJECT_ID ObjectId;
            ULONG Reserved1C;
            NSI_SET_ACTION Action;
            ULONG Reserved24;
            PVOID Key;
            ULONG KeySize;
            ULONG Reserved34;
            PVOID ParameterData;
            PVOID ParameterMask;
            ULONG ParameterDataSize;
            ULONG Reserved4C;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} NSI_PERSISTENT_SET_REQUEST, *PNSI_PERSISTENT_SET_REQUEST;

/**
 * The NSI_PERSISTENT_ENUMERATE_REQUEST structure is the IOCTL request to enumerate persistent parameter data and masks.
 */
typedef struct _NSI_PERSISTENT_ENUMERATE_REQUEST
{
    union
    {
        NSI_REQUEST_HEADER RequestHeader;
        struct
        {
            ULONGLONG MustBeZero;
            ULONG ModuleHandle;
            ULONG Reserved0C;
            PNPI_MODULEID ModuleId;
            NSI_OBJECT_ID ObjectId;
            ULONG Reserved1C;
            NSI_GET_ACTION Action;
            ULONG Reserved24;
            PVOID KeyTable;
            ULONG KeyEntrySize;
            ULONG Reserved34;
            PVOID ParameterDataTable;
            PVOID ParameterMaskTable;
            ULONG ParameterDataEntrySize;
            ULONG Reserved4C;
            ULONG Count;
            ULONG Reserved54;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} NSI_PERSISTENT_ENUMERATE_REQUEST, *PNSI_PERSISTENT_ENUMERATE_REQUEST;

#if defined(_WIN64)
static_assert(sizeof(NSI_PERSISTENT_GET_REQUEST) == 0x48);
static_assert(sizeof(NSI_PERSISTENT_SET_REQUEST) == 0x50);
static_assert(sizeof(NSI_PERSISTENT_ENUMERATE_REQUEST) == 0x58);
#else
static_assert(sizeof(NSI_PERSISTENT_GET_REQUEST) == 0x38);
static_assert(sizeof(NSI_PERSISTENT_SET_REQUEST) == 0x40);
static_assert(sizeof(NSI_PERSISTENT_ENUMERATE_REQUEST) == 0x48);
#endif

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
    union
    {
        IN_ADDR Address;
        IPAddr AddressValue;
    } DUMMYUNIONNAME;
    UCHAR Reserved[4];              // alignment padding; nothing reads or writes it
} MIB_IP_ANYCAST_KEY_ROW_V4, *PMIB_IP_ANYCAST_KEY_ROW_V4;

/**
 * The MIB_IP_ANYCAST_KEY_ROW_V6 structure is the key of an IPv6 anycast address (NPI_MS_IPV6_MODULEID,
 * NsiIpObjectAnycastAddress).
 */
typedef struct _MIB_IP_ANYCAST_KEY_ROW_V6
{
    NET_LUID InterfaceLuid;
    union
    {
        IN6_ADDR Address;
        IPv6Addr AddressValue;
    } DUMMYUNIONNAME;
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
    union
    {
        IN_ADDR Address;
        IPAddr AddressValue;
    } DUMMYUNIONNAME;
    UCHAR Reserved[4];              // alignment padding; nothing reads or writes it
} MIB_IP_NEIGHBOR_KEY_ROW_V4, *PMIB_IP_NEIGHBOR_KEY_ROW_V4;

/**
 * The MIB_IP_NEIGHBOR_KEY_ROW_V6 structure is the key of an IPv6 neighbor (NPI_MS_IPV6_MODULEID, NsiIpObjectNeighbor).
 */
typedef struct _MIB_IP_NEIGHBOR_KEY_ROW_V6
{
    NET_LUID InterfaceLuid;         // IP interface.
    NET_LUID SubInterfaceLuid;      // Sub-interface the neighbor was resolved on (tcpip!IppFindSubInterfaceByLuid).
    union
    {
        IN6_ADDR Address;
        IPv6Addr AddressValue;
    } DUMMYUNIONNAME;
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
    UCHAR InternalState[16];
} MIB_IP_ROUTE_DYNAMIC_ROW_V4, *PMIB_IP_ROUTE_DYNAMIC_ROW_V4;

/**
 * The MIB_IP_ROUTE_DYNAMIC_ROW_V6 structure is the IPv6 counterpart of MIB_IP_ROUTE_DYNAMIC_ROW_V4.
 */
typedef struct _MIB_IP_ROUTE_DYNAMIC_ROW_V6
{
    ULONG Age;
    UCHAR InternalState[28];
} MIB_IP_ROUTE_DYNAMIC_ROW_V6, *PMIB_IP_ROUTE_DYNAMIC_ROW_V6;

/**
 * The MIB_IP_ROUTE_KEY_ROW_V4 structure is the key of an IPv4 unicast route (NPI_MS_IPV4_MODULEID, NsiIpObjectRoute).
 */
typedef struct _MIB_IP_ROUTE_KEY_ROW_V4
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    union
    {
        IN_ADDR DestinationPrefix;
        IPAddr DestinationPrefixValue;
    } DUMMYUNIONNAME;
    UCHAR DestinationPrefixLength;
    UCHAR Reserved1[3];                     // alignment padding; nothing reads or writes it
    union
    {
        IN_ADDR SourcePrefix;                   // Source-specific route prefix (0.0.0.0/0 for ordinary routes).
        IPAddr SourcePrefixValue;
    } DUMMYUNIONNAME2;
    UCHAR SourcePrefixLength;
    UCHAR Reserved2[7];                     // alignment padding; nothing reads or writes it
    NET_LUID InterfaceLuid;
    NET_LUID SubInterfaceLuid;              // Sub-interface of the next hop.
    union
    {
        IN_ADDR NextHop;
        IPAddr NextHopValue;
    } DUMMYUNIONNAME3;
    ULONG Reserved3;                        // alignment padding; nothing reads or writes it
} MIB_IP_ROUTE_KEY_ROW_V4, *PMIB_IP_ROUTE_KEY_ROW_V4;

/**
 * The MIB_IP_ROUTE_KEY_ROW_V6 structure is the key of an IPv6 unicast route (NPI_MS_IPV6_MODULEID, NsiIpObjectRoute).
 */
typedef struct _MIB_IP_ROUTE_KEY_ROW_V6
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    union
    {
        IN6_ADDR DestinationPrefix;
        IPv6Addr DestinationPrefixValue;
    } DUMMYUNIONNAME;
    UCHAR DestinationPrefixLength;
    UCHAR Reserved1;                        // alignment padding; nothing reads or writes it
    union
    {
        IN6_ADDR SourcePrefix;                  // Source-specific route prefix (::/0 for ordinary routes).
        IPv6Addr SourcePrefixValue;
    } DUMMYUNIONNAME2;
    UCHAR SourcePrefixLength;
    UCHAR Reserved2;                        // alignment padding; nothing reads or writes it
    NET_LUID InterfaceLuid;
    NET_LUID SubInterfaceLuid;              // Sub-interface of the next hop.
    union
    {
        IN6_ADDR NextHop;
        IPv6Addr NextHopValue;
    } DUMMYUNIONNAME3;
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
    union
    {
        IN_ADDR Address;
        IPAddr AddressValue;
    } DUMMYUNIONNAME;
    UCHAR Reserved[4];
} MIB_IP_UNICAST_KEY_ROW_V4, *PMIB_IP_UNICAST_KEY_ROW_V4;

/**
 * The MIB_IP_UNICAST_KEY_ROW_V6 structure is the key of an IPv6 unicast address (NPI_MS_IPV6_MODULEID,
 * NsiIpObjectUnicastAddress).
 */
typedef struct _MIB_IP_UNICAST_KEY_ROW_V6
{
    NET_LUID InterfaceLuid;
    union
    {
        IN6_ADDR Address;
        IPv6Addr AddressValue;
    } DUMMYUNIONNAME;
} MIB_IP_UNICAST_KEY_ROW_V6, *PMIB_IP_UNICAST_KEY_ROW_V6;

/**
 * The MIB_IP_UNICAST_STATIC_ROW structure is the read-only static row of a unicast address: the time the address was
 * created (MIB_UNICASTIPADDRESS_ROW.CreationTimeStamp).
 */
typedef struct _MIB_IP_UNICAST_STATIC_ROW
{
    LARGE_INTEGER CreationTimestamp;
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
 * \remarks Ndis.sys copies Flags into the compartment block only when Header.Revision >= 2.
 */
typedef enum _NDIS_COMPARTMENT_FLAGS
{
    NdisCompartmentFlagNonPersistent = 0x00000001,  // The loopback interface's NET_LUID index is allocated without being recorded in the registry (not reserved across reboots). iphlpapi.dll!CreateCompartment always sets it.
    NdisCompartmentFlagNotEnumerable = 0x00000002,  // Hidden from enumeration and from callers in other compartments.
    NdisCompartmentFlagOwnNamespace = 0x00000004    // The compartment GUID becomes its namespace GUID (callers in that namespace may access it).
} NDIS_COMPARTMENT_FLAGS;

/**
 * The NDIS_NSI_COMPARTMENT_RW structure is the read/write parameter block of an NDIS network compartment
 * (NPI_MS_NDIS_MODULEID, NdisNsiObjectCompartment). Callers fill the header, CompartmentGuid, CompartmentName and Flags to
 * create a compartment; ndis.sys creates the compartment's network and software loopback interface, fills the loopback and
 * network fields, keeps a copy in the compartment block and returns that copy from enumeration.
 * \remarks Setting compartment parameters requires a 0x668-byte input. Persisted records with Revision < 3 or Size < 0x664 are upgraded during compartment initialization.
 */
typedef struct _NDIS_NSI_COMPARTMENT_RW
{
    NDIS_OBJECT_HEADER Header;                          // Type = NDIS_OBJECT_TYPE_NSI_COMPARTMENT_RW_STRUCT, Revision = NDIS_NSI_COMPARTMENT_RW_REVISION_3, Size = sizeof(NDIS_NSI_COMPARTMENT_RW).
    ULONG Reserved;                                     // alignment padding; nothing reads or writes it
    NET_LUID LoopbackInterfaceLuid;                     // [out] LUID of the compartment's software loopback interface (IfType IF_TYPE_SOFTWARE_LOOPBACK).
    NET_IF_NETWORK_GUID NetworkGuid;                    // [out] GUID of the network ndis.sys creates for the compartment; the loopback interface joins it.
    GUID LoopbackInterfaceGuid;                         // [out] Interface GUID generated for the loopback interface.
    IF_COUNTED_STRING LoopbackInterfaceAlias;           // [out] Loopback friendly name (ifAlias), suffixed with the compartment ID.
    IF_COUNTED_STRING LoopbackInterfaceDescription;     // [out] Loopback description (ifDescr), suffixed with the compartment ID.
    GUID CompartmentGuid;                               // [in] Identifies the compartment; ConvertCompartmentGuidToId looks it up.
    IF_COUNTED_STRING CompartmentName;                  // [in] Length is in bytes; the default compartment is named "Default Compartment".
    LOGICAL SystemCompartment;                          // [out] 1 for ndis.sys compartments (the default compartment and migrated persisted records), 0 for iphlpapi compartments // enumeration only; ndis.sys never tests it.
    union
    {
        ULONG Flags;                                    // [in] NDIS_COMPARTMENT_FLAGS (Revision >= 2).
        struct
        {
            ULONG NonPersistent : 1;
            ULONG NotEnumerable : 1;
            ULONG OwnNamespace : 1;
            ULONG ReservedFlags : 29;
        } FlagsBits;
    } DUMMYUNIONNAME;
    GUID NamespaceGuid;                             // [in] Namespace the compartment belongs to; replaced with CompartmentGuid when NdisCompartmentFlagOwnNamespace is set, otherwise inherited from the default compartment.
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
    NDIS_NSI_INTERFACE_PURPOSE_DEFAULT = 0,                     // Default interface purpose.
    NDIS_NSI_INTERFACE_PURPOSE_INTERNAL_VSWITCH_NIC = 1,        // Host vNIC of a Hyper-V virtual switch.
    NDIS_NSI_INTERFACE_PURPOSE_SECONDARY_OR_ONDEMAND = 2,       // Secondary or on-demand interface.
    NDIS_NSI_INTERFACE_PURPOSE_IN_DISCONNECTED_STANDBY = 3,     // Interface in disconnected standby.
    NDIS_NSI_INTERFACE_PURPOSE_IN_FAILOVER = 4,                 // Interface in failover.
    NDIS_NSI_INTERFACE_PURPOSE_MAX = 5                          // Upper bound of the interface purpose values.
} NDIS_NSI_INTERFACE_PURPOSE;

/**
 * The NDIS_NSI_INTERFACE_INFORMATION_RW structure is the read/write parameter block of an NDIS interface
 * (NPI_MS_NDIS_MODULEID, NsiNdisObjectInterface; key: NET_LUID).
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
    IF_COUNTED_STRING ifDescr;                  // Description; Length is in bytes.
    NET_IFTYPE ifType;                              // IFTYPE (IF_TYPE_*)
    USHORT Reserved;                            // alignment padding; nothing reads or writes it
    NET_IF_ACCESS_TYPE AccessType;
    NET_IF_DIRECTION_TYPE DirectionType;
    NET_IF_CONNECTION_TYPE ConnectionType;
    GUID InterfaceGuid;
    BOOLEAN ifConnectorPresent;
    UCHAR Reserved1;                            // alignment padding; nothing reads or writes it
    IF_PHYSICAL_ADDRESS PermanentPhysAddress;
    union
    {
        ULONG ifFlags;                          // NET_IF_INTERFACE_FLAGS
        struct
        {
            ULONG HardwareInterface : 1;
            ULONG FilterInterface : 1;
            ULONG ReservedFlags1 : 3;
            ULONG NdisWdm : 1;
            ULONG EndPointInterface : 1;
            ULONG ReservedFlags2 : 1;
            ULONG AoAcCapable : 1;
            ULONG ReservedFlags3 : 23;
        } FlagsBits;
    } DUMMYUNIONNAME;
    NDIS_MEDIUM MediaType;
    NDIS_PHYSICAL_MEDIUM PhysicalMediumType;
} NDIS_NSI_INTERFACE_INFORMATION_ROS, *PNDIS_NSI_INTERFACE_INFORMATION_ROS;

/**
 * The NDIS_NSI_INTERFACE_ENUM_ROD structure is the read-only dynamic row of NsiNdisObjectInterfaceWithoutStatistics
 * (key: NET_LUID); it omits the statistics that make NsiNdisObjectInterface expensive to enumerate.
 */
typedef struct _NDIS_NSI_INTERFACE_ENUM_ROD
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    NET_IF_NETWORK_GUID NetworkGuid;
    IF_COUNTED_STRING ifAlias;              // Friendly name; Length is in bytes.
    NET_IF_OPER_STATUS ifOperStatus;
    union
    {
        ULONG ifOperStatusFlags;            // NET_IF_OPER_STATUS_* reason bits
        struct
        {
            ULONG DownNotAuthenticated : 1;
            ULONG DownNotMediaConnected : 1;
            ULONG DormantPaused : 1;
            ULONG DormantLowPower : 1;
            ULONG ReservedFlags : 28;
        } OperStatusFlagsBits;
    } DUMMYUNIONNAME;
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
 */
typedef struct _NDIS_NSI_INTERFACE_ENUM_ROS
{
    NET_IFINDEX ifIndex;
    IF_COUNTED_STRING ifDescr;                   // Description; Length is in bytes.
    NET_IFTYPE ifType;                               // IFTYPE (IF_TYPE_*)
    USHORT Reserved;                             // alignment padding; nothing reads or writes it
    NET_IF_ACCESS_TYPE AccessType;
    NET_IF_DIRECTION_TYPE DirectionType;
    NET_IF_CONNECTION_TYPE ConnectionType;
    GUID InterfaceGuid;
    BOOLEAN ifConnectorPresent;
    UCHAR Reserved1[3];                          // alignment padding; nothing reads or writes it
    union
    {
        ULONG ifFlags;                           // NET_IF_INTERFACE_FLAGS
        struct
        {
            ULONG HardwareInterface : 1;
            ULONG FilterInterface : 1;
            ULONG ReservedFlags1 : 3;
            ULONG NdisWdm : 1;
            ULONG EndPointInterface : 1;
            ULONG ReservedFlags2 : 1;
            ULONG AoAcCapable : 1;
            ULONG ReservedFlags3 : 23;
        } FlagsBits;
    } DUMMYUNIONNAME;
    NDIS_MEDIUM MediaType;
    NDIS_PHYSICAL_MEDIUM PhysicalMediumType;
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
 */
typedef struct _NDIS_NSI_NETWORK_RW
{
    NDIS_OBJECT_HEADER Header;              // Type = NDIS_OBJECT_TYPE_NSI_NETWORK_RW_STRUCT, Revision = NDIS_NSI_NETWORK_RW_REVISION_1, Size = sizeof(NDIS_NSI_NETWORK_RW).
    NET_IF_COMPARTMENT_ID CompartmentId;    // Compartment the network belongs to.
    ULONG SiteId;
    IF_COUNTED_STRING NetworkName;          // Length is in bytes.
} NDIS_NSI_NETWORK_RW, *PNDIS_NSI_NETWORK_RW;

/**
 * The NDIS_NSI_SESSION_COMPARTMENT_KEY structure is the key of NsiNdisObjectSessionCompartment (NPI_MS_NDIS_MODULEID).
 */
typedef struct _NDIS_NSI_SESSION_COMPARTMENT_KEY
{
    ULONG SessionId;                        // Logon session ID.
} NDIS_NSI_SESSION_COMPARTMENT_KEY, *PNDIS_NSI_SESSION_COMPARTMENT_KEY;

/**
 * The NDIS_NSI_SESSION_COMPARTMENT_RW structure is the read/write parameter block of NsiNdisObjectSessionCompartment.
 */
typedef struct _NDIS_NSI_SESSION_COMPARTMENT_RW
{
    NET_IF_COMPARTMENT_ID CompartmentId;    // NdisGetSessionCompartmentId.
} NDIS_NSI_SESSION_COMPARTMENT_RW, *PNDIS_NSI_SESSION_COMPARTMENT_RW;

/**
 * The NDIS_NSI_JOB_COMPARTMENT_KEY structure is the key of NsiNdisObjectJobCompartment (NPI_MS_NDIS_MODULEID).
 */
typedef struct _NDIS_NSI_JOB_COMPARTMENT_KEY
{
    HANDLE JobHandle;                       // Job object handle in the caller's handle table; requires JOB_OBJECT_QUERY.
} NDIS_NSI_JOB_COMPARTMENT_KEY, *PNDIS_NSI_JOB_COMPARTMENT_KEY;

/**
 * The NDIS_NSI_JOB_COMPARTMENT_RW structure is the read/write parameter block of NsiNdisObjectJobCompartment.
 */
typedef struct _NDIS_NSI_JOB_COMPARTMENT_RW
{
    NET_IF_COMPARTMENT_ID CompartmentId;    // NdisGetJobObjectCompartmentId.
} NDIS_NSI_JOB_COMPARTMENT_RW, *PNDIS_NSI_JOB_COMPARTMENT_RW;

/**
 * The NDIS_NSI_TIMESTAMP_ROD structure is the read-only dynamic parameter block of NsiNdisObjectTimestampConfigChange
 * (key: NET_LUID). Only NsiGetAllParameters is supported, and RW/ROS must be NULL.
 */
typedef struct _NDIS_NSI_TIMESTAMP_ROD
{
    ULONG TimestampFlags;                   // The miniport's timestamping capability/configuration flags (0 if no miniport).
} NDIS_NSI_TIMESTAMP_ROD, *PNDIS_NSI_TIMESTAMP_ROD;

/**
 * The NDIS_NSI_INTERFACE_RAW_KEY structure is the key of NsiNdisObjectInterfaceRaw (NPI_MS_NDIS_MODULEID).
 * NsiNdisObjectInterfaceRaw is served by the same handlers as NsiNdisObjectInterface (get only, no enumeration),
 * so its parameter blocks are the NsiNdisObjectInterface blocks.
 */
typedef struct _NDIS_NSI_INTERFACE_RAW_KEY
{
    NET_LUID InterfaceLuid;
} NDIS_NSI_INTERFACE_RAW_KEY, *PNDIS_NSI_INTERFACE_RAW_KEY;

typedef NDIS_NSI_INTERFACE_INFORMATION_RW NDIS_NSI_INTERFACE_RAW_RW, *PNDIS_NSI_INTERFACE_RAW_RW;
typedef NDIS_NSI_INTERFACE_INFORMATION_ROD NDIS_NSI_INTERFACE_RAW_ROD, *PNDIS_NSI_INTERFACE_RAW_ROD;
typedef NDIS_NSI_INTERFACE_INFORMATION_ROS NDIS_NSI_INTERFACE_RAW_ROS, *PNDIS_NSI_INTERFACE_RAW_ROS;

/**
 * The NDIS_NSI_INTERFACE_ENUM_RAW_KEY structure is the key of NsiNdisObjectInterfaceWithoutStatisticsRaw
 * (NPI_MS_NDIS_MODULEID). NsiNdisObjectInterfaceWithoutStatisticsRaw is served by the same handlers as
 * NsiNdisObjectInterfaceWithoutStatistics (get, get all and enumerate), so its parameter blocks are the
 * NsiNdisObjectInterfaceWithoutStatistics blocks.
 */
typedef struct _NDIS_NSI_INTERFACE_ENUM_RAW_KEY
{
    NET_LUID InterfaceLuid;
} NDIS_NSI_INTERFACE_ENUM_RAW_KEY, *PNDIS_NSI_INTERFACE_ENUM_RAW_KEY;

typedef NDIS_NSI_INTERFACE_ENUM_ROD NDIS_NSI_INTERFACE_ENUM_RAW_ROD, *PNDIS_NSI_INTERFACE_ENUM_RAW_ROD;
typedef NDIS_NSI_INTERFACE_ENUM_ROS NDIS_NSI_INTERFACE_ENUM_RAW_ROS, *PNDIS_NSI_INTERFACE_ENUM_RAW_ROS;

/**
 * The NDIS_NSI_INTERFACE_PERSIST_RW structure is the read/write parameter block of NsiNdisObjectInterfacePersisted
 * (NPI_MS_NDIS_MODULEID; key: NET_LUID). The object is registered without provider handlers; it exists only in the NSI
 * persistent store, which ndis.sys updates with NsiSetParameterEx (ndisIfUpdatePersistedInterfaceInfo) when an
 * interface's alias, description, media type or physical addresses change.
 */
typedef struct _NDIS_NSI_INTERFACE_PERSIST_RW
{
    NDIS_OBJECT_HEADER Header;              // Presumed Type = NDIS_OBJECT_TYPE_NSI_INTERFACE_PERSIST_RW_STRUCT; ndis.sys never writes it.
    IF_COUNTED_STRING ifAlias;              // Friendly name; Length is in bytes (ndisIfAliasChange).
    IF_COUNTED_STRING ifDescr;              // Description; Length is in bytes (ndisIfSetIfDescr, ndisIfUpdateInterfaceOnAddDevice).
    UCHAR Reserved[0x28];                   // Not written by ndis.sys.
    NDIS_MEDIUM MediaType;                  // ndisIfUpdateInterfaceOnInitialize.
    NDIS_PHYSICAL_MEDIUM PhysicalMediumType;
    ULONG Reserved1;                        // Not written by ndis.sys.
    IF_PHYSICAL_ADDRESS ifPhysAddress;      // ndisIfUpdateInterfaceOnInitialize.
    IF_PHYSICAL_ADDRESS PermanentPhysAddress;
} NDIS_NSI_INTERFACE_PERSIST_RW, *PNDIS_NSI_INTERFACE_PERSIST_RW;

/**
 * The NDIS_NSI_RCV_ADDRESS_KEY structure is the key of NsiNdisObjectInterfaceReceiveAddress (NPI_MS_NDIS_MODULEID): one
 * entry of an interface's receive address table (IF-MIB ifRcvAddressTable). ndis.sys fills the table from the miniport's
 * current address and multicast list (ndisIfGetRcvAddressFromMiniport).
 * \remarks ndisNsiGetIfRcvAddress requires a 0x28-byte key and accepts NsiGetExact, NsiGetFirst and NsiGetNext; NsiGetFirst
 * with InterfaceLuid -1 starts at the first interface. The found InterfaceLuid and Address are written back into the key.
 * Only the read-only dynamic block is returned.
 */
typedef struct _NDIS_NSI_RCV_ADDRESS_KEY
{
    NET_LUID InterfaceLuid;
    UCHAR Address[32];                      // Link-layer address; the valid length is NDIS_NSI_RCV_ADDRESS_ROD.AddressLength.
} NDIS_NSI_RCV_ADDRESS_KEY, *PNDIS_NSI_RCV_ADDRESS_KEY;

/**
 * The NDIS_NSI_RCV_ADDRESS_ROD structure is the read-only dynamic block of NsiNdisObjectInterfaceReceiveAddress
 * (ndisNsiGetIfRcvAddressRodInformation accepts offsets 0 and 4 only).
 */
typedef struct _NDIS_NSI_RCV_ADDRESS_ROD
{
    ULONG Type;                             // IF-MIB ifRcvAddressType; ndis.sys always sets 2 (volatile).
    USHORT AddressLength;                   // 6 for Ethernet.
    USHORT Reserved;                        // Not returned (the parameter walk stops after AddressLength).
} NDIS_NSI_RCV_ADDRESS_ROD, *PNDIS_NSI_RCV_ADDRESS_ROD;

/**
 * The NDIS_NSI_PERSISTENT_ISOLATION_KEY structure is the key of NsiNdisObjectPersistentIsolation. The object has no
 * provider handlers; it exists only in the NSI persistent store. tcpip.sys reads it with
 * NsiAllocateAndGetTable(NsiPersistent) (InternalCleanPersistentIsolationInfo) and deletes a record with
 * NsiSetAllParameters(NsiPersistent, NsiSetDelete) when a framing-layer virtual interface goes away
 * (FlIfDeleteVirtualInterface).
 */
typedef struct _NDIS_NSI_PERSISTENT_ISOLATION_KEY
{
    NET_LUID InterfaceLuid;                 // Virtual interface the record belongs to.
    ULONG Value08;                          // Virtual interface value (+0x38); meaning unknown.
    ULONG Reserved;
} NDIS_NSI_PERSISTENT_ISOLATION_KEY, *PNDIS_NSI_PERSISTENT_ISOLATION_KEY;

/**
 * The NDIS_NSI_PERSISTENT_ISOLATION_RW structure is the read/write block of NsiNdisObjectPersistentIsolation. For each
 * record that matches an interface, tcpip deletes the persistent IP addresses, routes, neighbors and IP interface
 * settings of IsolatedInterfaceLuid.
 */
typedef struct _NDIS_NSI_PERSISTENT_ISOLATION_RW
{
    UCHAR Reserved00[8];                    // Not read by tcpip.sys.
    NET_LUID IsolatedInterfaceLuid;
    UCHAR Reserved10[16];                   // Not read by tcpip.sys.
} NDIS_NSI_PERSISTENT_ISOLATION_RW, *PNDIS_NSI_PERSISTENT_ISOLATION_RW;

static_assert(sizeof(NDIS_NSI_RCV_ADDRESS_KEY) == 0x28);
static_assert(sizeof(NDIS_NSI_RCV_ADDRESS_ROD) == 0x8);
static_assert(sizeof(NDIS_NSI_PERSISTENT_ISOLATION_KEY) == 0x10);
static_assert(sizeof(NDIS_NSI_PERSISTENT_ISOLATION_RW) == 0x20);
static_assert(FIELD_OFFSET(NDIS_NSI_PERSISTENT_ISOLATION_RW, IsolatedInterfaceLuid) == 0x8);

/**
 * The NSI_NEIGHBOR_IPV4_ROW structure is a System Informer-defined row (not a Windows structure). netsup.c builds it
 * from the key, read/write and dynamic rows of the IPv4 neighbor table so callers get one flat record per ARP entry.
 */
typedef struct _NSI_NEIGHBOR_IPV4_ROW
{
    union
    {
        IN_ADDR Address;
        IPAddr AddressValue;
    } DUMMYUNIONNAME;
    NET_LUID InterfaceLuid;
    NET_IFINDEX InterfaceIndex;
    UCHAR PhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
    ULONG PhysicalAddressLength;
    NET_IF_COMPARTMENT_ID CompartmentId;
    NL_NEIGHBOR_STATE State;
    BOOLEAN IsRouter : 1;
    BOOLEAN IsUnreachable : 1;
    UCHAR FlagsReserved : 6;
} NSI_NEIGHBOR_IPV4_ROW, *PNSI_NEIGHBOR_IPV4_ROW;

/**
 * The NSI_NEIGHBOR_IPV4_TABLE structure is a System Informer-defined table of NSI_NEIGHBOR_IPV4_ROW entries (not a Windows structure).
 */
typedef struct _NSI_NEIGHBOR_IPV4_TABLE
{
    ULONG Count;
    ULONG Reserved;
    NSI_NEIGHBOR_IPV4_ROW Entries[ANYSIZE_ARRAY];
} NSI_NEIGHBOR_IPV4_TABLE, *PNSI_NEIGHBOR_IPV4_TABLE;

/**
 * The NSI_NEIGHBOR_IPV6_ROW structure is the System Informer-defined IPv6 counterpart of NSI_NEIGHBOR_IPV4_ROW.
 */
typedef struct _NSI_NEIGHBOR_IPV6_ROW
{
    union
    {
        IN6_ADDR Address;
        IPv6Addr AddressValue;
    } DUMMYUNIONNAME;
    NET_LUID InterfaceLuid;
    NET_IFINDEX InterfaceIndex;
    UCHAR PhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
    ULONG PhysicalAddressLength;
    NET_IF_COMPARTMENT_ID CompartmentId;
    NL_NEIGHBOR_STATE State;
    BOOLEAN IsRouter : 1;
    BOOLEAN IsUnreachable : 1;
    UCHAR FlagsReserved : 6;
} NSI_NEIGHBOR_IPV6_ROW, *PNSI_NEIGHBOR_IPV6_ROW;

/**
 * The NSI_NEIGHBOR_IPV6_TABLE structure is a System Informer-defined table of NSI_NEIGHBOR_IPV6_ROW entries (not a
 * Windows structure).
 */
typedef struct _NSI_NEIGHBOR_IPV6_TABLE
{
    ULONG Count;
    ULONG Reserved;
    NSI_NEIGHBOR_IPV6_ROW Entries[ANYSIZE_ARRAY];
} NSI_NEIGHBOR_IPV6_TABLE, *PNSI_NEIGHBOR_IPV6_TABLE;

typedef union _NSI_IP_ADDRESS_KEY
{
    MIB_IP_UNICAST_KEY_ROW_V4 V4;
    MIB_IP_UNICAST_KEY_ROW_V6 V6;
} NSI_IP_ADDRESS_KEY, *PNSI_IP_ADDRESS_KEY;

typedef struct _NSI_UNICAST_TABLE
{
    PVOID Keys;
    PMIB_IP_UNICAST_RW_ROW ReadWrite;
    PMIB_IP_UNICAST_DYNAMIC_ROW Dynamic;
    ULONG Count;
} NSI_UNICAST_TABLE, *PNSI_UNICAST_TABLE;

typedef struct _NSI_CHANGE_DATA
{
    PVOID Reserved;
    PVOID Data;
} NSI_CHANGE_DATA, *PNSI_CHANGE_DATA;

// Second argument of the NSI RPC change callback for keyed objects (iphlpapi!NetioInterfaceChangeCallBack).
typedef struct _NSI_CHANGE_KEY
{
    PVOID Key;
    ULONG KeyLength;
} NSI_CHANGE_KEY, *PNSI_CHANGE_KEY;

/**
 * The NSI change notification callback. It is called with the CallerContext passed at registration, the key and
 * parameter block of the object that changed (NULL for the initial notification), and the kind of change.
 *
 * \param[in] CallerContext The caller-defined context passed at registration.
 * \param[in] ObjectKey The key of the object that changed, or NULL.
 * \param[in] ObjectParameter The parameter block of the object that changed, or NULL.
 * \param[in] NotificationType The kind of change.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
typedef ULONG (NTAPI *PNSI_CHANGE_CALLBACK)(
    _In_opt_ PVOID CallerContext,
    _In_opt_ PVOID ObjectKey,
    _In_opt_ PVOID ObjectParameter,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    );

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
    PNSI_CHANGE_CALLBACK ChangeCallback;
    BOOLEAN NotifyCurrent;
    UCHAR Reserved21[7];            // alignment padding; nothing reads or writes it
    PVOID CallerContext;
    ULONG ScopeOrCompartment0;
    ULONG ScopeOrCompartment1;
    ULONG ProcessId;
    ULONG Reserved3C;               // alignment padding; nothing reads or writes it
    _Outptr_result_maybenull_ PHANDLE NotificationHandle;
} NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST, *PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST;

/**
 * The NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST structure is the input of NsiRequestChangeNotificationEx: a one-shot
 * change notification for an object that completes the caller's OVERLAPPED.
 */
typedef struct _NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST
{
    ULONG ModuleHandle;             // 0 = look the module up by ModuleId
    PNPI_MODULEID ModuleId;
    NSI_OBJECT_ID ObjectIndex;
    LPOVERLAPPED Overlapped;
    PHANDLE NotificationHandle;
} NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST, *PNSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST;

/**
 * The NSI_SET_ALL_PARAMETERS_EX_REQUEST structure is the input of NsiSetAllParametersEx. It writes the whole
 * read/write block of one object; Header.SetAction selects set, create or delete.
 */
typedef struct _NSI_SET_ALL_PARAMETERS_EX_REQUEST
{
    NSI_EX_REQUEST_COMMON Header;   // (Header.SetAction = NSI_SET_ACTION)
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
    NSI_EX_REQUEST_COMMON Header;   // (Header.SetAction = NSI_SET_ACTION)
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
    union
    {
        IN_ADDR LocalAddress;
        IPAddr LocalAddressValue;
    } DUMMYUNIONNAME;
    USHORT LocalPort;
    UCHAR Reserved1[2];             // alignment padding; nothing reads or writes it
    union
    {
        IN_ADDR RemoteAddress;
        IPAddr RemoteAddressValue;
    } DUMMYUNIONNAME2;
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
    ULONG Reserved;
    NSI_TCP_IPV4_ROW Entries[ANYSIZE_ARRAY];
} NSI_TCP_IPV4_TABLE, *PNSI_TCP_IPV4_TABLE;

/**
 * The NSI_TCP_IPV6_ROW structure is the System Informer-defined IPv6 counterpart of NSI_TCP_IPV4_ROW.
 */
typedef struct _NSI_TCP_IPV6_ROW
{
    union
    {
        IN6_ADDR LocalAddress;
        IPv6Addr LocalAddressValue;
    } DUMMYUNIONNAME;
    SCOPE_ID LocalScopeId;
    USHORT LocalPort;
    UCHAR Reserved1[2];             // alignment padding; nothing reads or writes it
    union
    {
        IN6_ADDR RemoteAddress;
        IPv6Addr RemoteAddressValue;
    } DUMMYUNIONNAME2;
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
    union
    {
        SOCKADDR_INET LocalAddress;
        SOCKADDR_IN LocalSockaddrV4;
        SOCKADDR_IN6 LocalSockaddrV6;
        struct
        {
            ADDRESS_FAMILY LocalFamily;
            USHORT LocalPort;
            union
            {
                IN_ADDR LocalAddressV4;
                IPAddr LocalAddressV4Value;
            } DUMMYUNIONNAME3;
            union
            {
                IN6_ADDR LocalAddressV6;
                IPv6Addr LocalAddressV6Value;
            } DUMMYUNIONNAME2;
            SCOPE_ID LocalScopeId;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        SOCKADDR_INET RemoteAddress;
        SOCKADDR_IN RemoteSockaddrV4;
        SOCKADDR_IN6 RemoteSockaddrV6;
        struct
        {
            ADDRESS_FAMILY RemoteFamily;
            USHORT RemotePort;
            union
            {
                IN_ADDR RemoteAddressV4;
                IPAddr RemoteAddressV4Value;
            } DUMMYUNIONNAME4;
            union
            {
                IN6_ADDR RemoteAddressV6;
                IPv6Addr RemoteAddressV6Value;
            } DUMMYUNIONNAME5;
            SCOPE_ID RemoteScopeId;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
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
    BOOLEAN NagleEnabled;                   // TRUE unless TCP_NODELAY is set on the endpoint (always FALSE for SYN_RCVD and TIME_WAIT entries).
    UCHAR Reserved1[3];                     // alignment padding; nothing reads or writes it
    TCP_CONNECTION_OFFLOAD_STATE OffloadState;
    NSI_TCP_APPLIED_SETTING AppliedSetting; // listeners report 6, which has no NSI_TCP_APPLIED_SETTING name.
    ULONG ConnectionDuration;               // Milliseconds since the TCB was created (enumeration only; a single-object get leaves it 0).
    UCHAR QosFlowTag;
    UCHAR Reserved2[3];                     // alignment padding; nothing reads or writes it
} NSI_TCP_STATUS_ROW, *PNSI_TCP_STATUS_ROW;

/**
 * The NSI_UDP_IPV4_ROW structure is a System Informer-defined row (not a Windows structure). netsup.c builds it from the
 * NSI_UDP_KEY_ROW and NSI_UDP_OWNER_ROW of one IPv4 UDP endpoint.
 */
typedef struct _NSI_UDP_IPV4_ROW
{
    union
    {
        IN_ADDR LocalAddress;
        IPAddr LocalAddressValue;
    } DUMMYUNIONNAME;
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
    union
    {
        IN6_ADDR LocalAddress;
        IPv6Addr LocalAddressValue;
    } DUMMYUNIONNAME;
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
    ULONG Reserved;
    NSI_UDP_IPV6_ROW Entries[ANYSIZE_ARRAY];
} NSI_UDP_IPV6_TABLE, *PNSI_UDP_IPV6_TABLE;

/**
 * The NSI_UDP_KEY_ROW structure is the key row of a UDP endpoint returned by the UDP endpoint enumeration
 * (NPI_MS_UDP_MODULEID): the local endpoint as a SOCKADDR_INET value.
 * \remarks For AF_INET endpoints tcpip writes the IPv4 address at offset 4.
 */
typedef struct _NSI_UDP_KEY_ROW
{
    union
    {
        SOCKADDR_INET LocalSockaddr;
        SOCKADDR_IN LocalSockaddrV4;
        SOCKADDR_IN6 LocalSockaddrV6;
        struct
        {
            ADDRESS_FAMILY AddressFamily;
            USHORT LocalPort;
            union
            {
                IN_ADDR LocalAddressV4;
                IPAddr LocalAddressV4Value;
            } DUMMYUNIONNAME3;
            union
            {
                IN6_ADDR LocalAddressV6;
                IPv6Addr LocalAddressV6Value;
            } DUMMYUNIONNAME2;
            SCOPE_ID LocalScopeId;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
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
 * The NSI_UDP_STATUS_ROW structure is the read-only dynamic row of a UDP endpoint.
 * The remote address describes the connected peer; TrafficClass is returned by NetioQueryFlowTrafficClass.
 */
typedef struct _NSI_UDP_STATUS_ROW
{
    union
    {
        SOCKADDR_INET RemoteAddress;
        SOCKADDR_IN RemoteSockaddrV4;
        SOCKADDR_IN6 RemoteSockaddrV6;
        struct
        {
            ADDRESS_FAMILY RemoteFamily;
            USHORT RemotePort;
            union
            {
                IN_ADDR RemoteAddressV4;
                IPAddr RemoteAddressV4Value;
            } DUMMYUNIONNAME3;
            union
            {
                IN6_ADDR RemoteAddressV6;
                IPv6Addr RemoteAddressV6Value;
            } DUMMYUNIONNAME2;
            SCOPE_ID RemoteScopeId;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG TrafficClass; // NDIS_QOS_MAXIMUM_TRAFFIC_CLASSES
} NSI_UDP_STATUS_ROW, *PNSI_UDP_STATUS_ROW;

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
 * The NETIO_TCP_STATISTICS_EX2 structure is a System Informer-defined output of PhGetTcpStatisticsEx2:
 * the 16-byte RTO/connection-limit prefix followed by counters from NETIO_TCP_STATS_DYNAMIC.
 * The counter order is custom: OutRsts precedes InErrs, unlike MIB_TCPSTATS2. Do not reinterpret this buffer as MIB_TCPSTATS2.
 */
typedef struct _NETIO_TCP_STATISTICS_EX2
{
    union
    {
        NETIO_RECOVERED_128 Header;
        NSI_TCP_GLOBAL_HEADER GlobalParameters;
    } DUMMYUNIONNAME;
    ULONG ActiveOpens;
    ULONG PassiveOpens;
    ULONG AttemptFails;
    ULONG EstabResets;
    ULONG CurrEstab;
    ULONG Reserved24;               // alignment padding; nothing reads or writes it
    ULONG64 InSegs;
    ULONG64 OutSegs;
    ULONG RetransSegs;
    ULONG OutRsts;
    ULONG InErrs;
    ULONG NumConns;
} NETIO_TCP_STATISTICS_EX2, *PNETIO_TCP_STATISTICS_EX2;

/**
 * The NETIO_UDP_STATISTICS_EX2 structure is a 32-byte UDP statistics buffer with an SDK MIB_UDPSTATS2 view
 * and the original two raw 16-byte blocks. NETIO_UDP_STATS_DYNAMIC additionally contains offload counters.
 */
typedef struct _NETIO_UDP_STATISTICS_EX2
{
    union
    {
        MIB_UDPSTATS2 NativeStatistics;
        struct
        {
            NETIO_RECOVERED_128 Header;
            NETIO_RECOVERED_128 Dynamic;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} NETIO_UDP_STATISTICS_EX2, *PNETIO_UDP_STATISTICS_EX2;

/**
 * The NETIO_CONNECTIVITY_HINT structure is the network connectivity hint passed to
 * PNET_NETWORK_CONNECTIVITY_HINT_CHANGE_CALLBACK. It has the layout of NL_NETWORK_CONNECTIVITY_HINT
 * (GetNetworkConnectivityHint): the connectivity level and the cost of the current network.
 */
typedef NL_NETWORK_CONNECTIVITY_HINT NETIO_CONNECTIVITY_HINT, *PNETIO_CONNECTIVITY_HINT;

/**
 * The NETIO_INTERFACE_HARDWARE_CROSSTIMESTAMP structure contains two system clock readings around one NIC hardware
 * clock reading. It has the layout of INTERFACE_HARDWARE_CROSSTIMESTAMP (CaptureInterfaceHardwareCrossTimestamp).
 */
typedef struct _NETIO_INTERFACE_HARDWARE_CROSSTIMESTAMP
{
    ULONG64 SystemTimestamp1;
    ULONG64 HardwareClockTimestamp;
    ULONG64 SystemTimestamp2;
} NETIO_INTERFACE_HARDWARE_CROSSTIMESTAMP, *PNETIO_INTERFACE_HARDWARE_CROSSTIMESTAMP;

#define NETIO_NDIS_CROSSTIMESTAMP_REQUEST_TYPE 0xB9
#define NETIO_NDIS_CROSSTIMESTAMP_REQUEST_REVISION 1
#define NETIO_OID_TIMESTAMP_CAPABILITY 0x00A00001
#define NETIO_OID_TIMESTAMP_CURRENT_CONFIG 0x00A00002
#define NETIO_OID_TIMESTAMP_GET_CROSSTIMESTAMP 0x00A00003
#define IOCTL_NETIO_NDIS_CROSSTIMESTAMP CTL_CODE(FILE_DEVICE_PHYSICAL_NETCARD, 0x27, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * The NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA structure is the NDIS_HARDWARE_CROSSTIMESTAMP payload returned for
 * OID_TIMESTAMP_GET_CROSSTIMESTAMP. Its timestamp fields begin eight bytes after the payload header.
 */
#include <pshpack4.h>
typedef struct _NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA
{
    union
    {
        NDIS_OBJECT_HEADER Header;
        struct
        {
            UCHAR Type;
            UCHAR Revision;
            USHORT Size;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG Flags;
    ULONG64 SystemTimestamp1;
    ULONG64 HardwareClockTimestamp;
    ULONG64 SystemTimestamp2;
} NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA, *PNETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA;

/**
 * The NETIO_NDIS_REQUEST_HEADER structure is the shared 0x2C-byte prefix of NETIO NDIS timestamp requests.
 * This definition is inside the four-byte packing region used by both request layouts.
 */
typedef struct _NETIO_NDIS_REQUEST_HEADER
{
    NDIS_OBJECT_HEADER Header;
    ULONG Reserved04[2];
    NDIS_OID Oid;
    UCHAR Reserved10[20];
    NDIS_STATUS Status;
    USHORT DataOffset;
    USHORT Reserved2A;
} NETIO_NDIS_REQUEST_HEADER, *PNETIO_NDIS_REQUEST_HEADER;

/**
 * The NETIO_NDIS_CROSSTIMESTAMP_REQUEST structure is the in/out buffer used by IP Helper's hardware cross timestamp
 * query. The observed buffer is 0x50 bytes on x64 and 0x4C bytes on x86; DataOffset points to Data at byte 0x2C.
 */
typedef struct _NETIO_NDIS_CROSSTIMESTAMP_REQUEST
{
    union
    {
        NETIO_NDIS_REQUEST_HEADER RequestHeader;
        struct
        {
            NDIS_OBJECT_HEADER Header;
            ULONG Reserved04[2];
            NDIS_OID Oid;
            UCHAR Reserved10[20];
            NDIS_STATUS Status;
            USHORT DataOffset;
            USHORT Reserved2A;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA Data;
#if defined(_WIN64)
    ULONG Reserved4C;
#endif
} NETIO_NDIS_CROSSTIMESTAMP_REQUEST, *PNETIO_NDIS_CROSSTIMESTAMP_REQUEST;
#include <poppack.h>

static_assert(sizeof(NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA) == 0x20);
static_assert(sizeof(NETIO_INTERFACE_HARDWARE_CROSSTIMESTAMP) == 0x18);
static_assert(FIELD_OFFSET(NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA, SystemTimestamp1) == 0x08);
static_assert(FIELD_OFFSET(NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA, HardwareClockTimestamp) == 0x10);
static_assert(FIELD_OFFSET(NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA, SystemTimestamp2) == 0x18);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Oid) == 0x0C);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Status) == 0x24);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, DataOffset) == 0x28);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Data) == 0x2C);
#if defined(_WIN64)
static_assert(sizeof(NETIO_NDIS_CROSSTIMESTAMP_REQUEST) == 0x50);
#else
static_assert(sizeof(NETIO_NDIS_CROSSTIMESTAMP_REQUEST) == 0x4C);
#endif

/**
 * The NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA structure is the NDIS_TIMESTAMP_CAPABILITIES payload returned for
 * OID_TIMESTAMP_CAPABILITY and OID_TIMESTAMP_CURRENT_CONFIG. TimestampFlags holds eleven hardware flags followed by
 * three software flags (NDIS_TIMESTAMP_CAPABILITY_FLAGS).
 */
#include <pshpack4.h>
typedef struct _NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA
{
    union
    {
        NDIS_OBJECT_HEADER Header;
        struct
        {
            UCHAR Type;
            UCHAR Revision;
            USHORT Size;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG Reserved04;               // alignment padding; nothing reads or writes it
    ULONG64 HardwareClockFrequencyHz;
    BOOLEAN CrossTimestamp;
    UCHAR Reserved11[7];            // alignment padding; nothing reads or writes it
    ULONG64 Reserved1;
    ULONG64 Reserved2;
    union
    {
        BOOLEAN TimestampFlags[14];
        NDIS_TIMESTAMP_CAPABILITY_FLAGS TimestampFlagsInfo;
    } DUMMYUNIONNAME2;
    UCHAR Reserved36[2];            // alignment padding; nothing reads or writes it
} NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA, *PNETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA;

#define NETIO_NDIS_TIMESTAMP_HARDWARE_FLAG_COUNT 11
#define NETIO_NDIS_TIMESTAMP_SOFTWARE_FLAG_COUNT 3

/**
 * The NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST structure is the in/out buffer used by IP Helper's interface timestamp
 * capability query (IOCTL_NETIO_NDIS_CROSSTIMESTAMP). It shares the NETIO_NDIS_CROSSTIMESTAMP_REQUEST header;
 * the observed buffer is 0x68 bytes on x64.
 */
typedef struct _NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST
{
    union
    {
        NETIO_NDIS_REQUEST_HEADER RequestHeader;
        struct
        {
            NDIS_OBJECT_HEADER Header;
            ULONG Reserved04[2];
            NDIS_OID Oid;
            UCHAR Reserved10[20];
            NDIS_STATUS Status;
            USHORT DataOffset;
            USHORT Reserved2A;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA Data;
#if defined(_WIN64)
    ULONG Reserved64;
#endif
} NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, *PNETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST;
#include <poppack.h>

static_assert(sizeof(NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA) == 0x38);
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA, HardwareClockFrequencyHz) == 0x08);
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA, CrossTimestamp) == 0x10);
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA, TimestampFlags) == 0x28);
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, Oid) == 0x0C);
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, Status) == 0x24);
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, DataOffset) == 0x28);
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, Data) == 0x2C);
#if defined(_WIN64)
static_assert(sizeof(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST) == 0x68);
#else
static_assert(sizeof(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST) == 0x64);
#endif

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
    union
    {
        PVOID Callback;             // Caller's callback; select the typed view matching the registration.
        PIPINTERFACE_CHANGE_CALLBACK InterfaceCallback;
        PIPFORWARD_CHANGE_CALLBACK RouteCallback;
    } DUMMYUNIONNAME;
    PVOID CallerContext;
} NETIO_CHANGE_CALLBACK_CONTEXT, *PNETIO_CHANGE_CALLBACK_CONTEXT;

/**
 * The NETIO_CHANGE_REGISTRATION_HEADER structure starts every notification handle returned by the IP Helper Notify*Change
 * routines. CancelMibChangeNotify2 deregisters each non-NULL module handle and then frees the block.
 */
typedef struct _NETIO_CHANGE_REGISTRATION_HEADER
{
    PVOID Reserved;                             // nothing reads or writes it (IP Helper's register, callback and cancel paths start at ObjectId)
    NSI_OBJECT_ID ObjectId;                     // NSI object the registration watches.
    ULONG Reserved1;                            // alignment padding; nothing reads or writes it
    HANDLE Ipv4NotificationHandle;              // NsiRpcRegisterChangeNotification handle for NPI_MS_IPV4_MODULEID.
    HANDLE Ipv6NotificationHandle;              // Handle for NPI_MS_IPV6_MODULEID.
    HANDLE TeredoNotificationHandle;            // Handle for NPI_MS_FL6T_MODULEID.
    HANDLE NdisNotificationHandle;              // Handle for NPI_MS_NDIS_MODULEID.
    HANDLE ConnectivityHintRegistration;        // Non-NULL for NotifyNetworkConnectivityHintChange registrations.
    PVOID Ipv4CallbackContext;                  // Context registered with the IPv4 (or only) module.
    PVOID Ipv6CallbackContext;                  // Context registered with the IPv6 module.
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
    NETIO_CHANGE_REGISTRATION_HEADER Header;        // ObjectId = NsiIpObjectIpInterface
    NETIO_CHANGE_CALLBACK_CONTEXT Ipv4Callback;     // Header.Ipv4CallbackContext points here.
    MIB_IPINTERFACE_ROW Ipv4Row;                    // Row filled before the IPv4 callback is called.
    NETIO_CHANGE_CALLBACK_CONTEXT Ipv6Callback;     // Header.Ipv6CallbackContext points here.
    MIB_IPINTERFACE_ROW Ipv6Row;
} NETIO_IP_INTERFACE_CHANGE_REGISTRATION, *PNETIO_IP_INTERFACE_CHANGE_REGISTRATION;

/**
 * The NETIO_ROUTE_CHANGE_REGISTRATION structure is the 0x148-byte notification handle NotifyRouteChange2 allocates,
 * with one callback context and MIB_IPFORWARD_ROW2 buffer per address family.
 */
typedef struct _NETIO_ROUTE_CHANGE_REGISTRATION
{
    NETIO_CHANGE_REGISTRATION_HEADER Header;    // ObjectId = NsiIpObjectRoute
    NETIO_CHANGE_CALLBACK_CONTEXT Ipv4Callback; // Header.Ipv4CallbackContext points here.
    MIB_IPFORWARD_ROW2 Ipv4Row;                 // Row NetioRouteChangeCallBack fills before calling the IPv4 callback.
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
    union
    {
        SOCKADDR_INET LocalSockaddr;
        SOCKADDR_IN LocalSockaddrV4;
        SOCKADDR_IN6 LocalSockaddrV6;
        struct
        {
            ADDRESS_FAMILY Family;              // Local address family.
            USHORT LocalPort;
            union
            {
                IN_ADDR LocalAddressV4;         // AF_INET: the IPv4 address (SOCKADDR_IN.sin_addr); AF_INET6: sin6_flowinfo (0).
                IPAddr LocalAddressV4Value;
            } DUMMYUNIONNAME3;
            union
            {
                IN6_ADDR LocalAddress;
                IPv6Addr LocalAddressValue;
            } DUMMYUNIONNAME2;
            SCOPE_ID LocalScopeId;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        SOCKADDR_INET RemoteSockaddr;
        SOCKADDR_IN RemoteSockaddrV4;
        SOCKADDR_IN6 RemoteSockaddrV6;
        struct
        {
            ADDRESS_FAMILY RemoteFamily;        // Remote address family.
            USHORT RemotePort;
            union
            {
                IN_ADDR RemoteAddressV4;        // AF_INET: the IPv4 address; AF_INET6: sin6_flowinfo (0).
                IPAddr RemoteAddressV4Value;
            } DUMMYUNIONNAME4;
            union
            {
                IN6_ADDR RemoteAddress;
                IPv6Addr RemoteAddressValue;
            } DUMMYUNIONNAME5;
            SCOPE_ID RemoteScopeId;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} TCP_KEY_ROW, *PTCP_KEY_ROW;

/**
 * The UDP_KEY_ROW structure is the IPv6 view of NSI_UDP_KEY_ROW (a SOCKADDR_INET value).
 */
typedef struct _UDP_KEY_ROW
{
    union
    {
        SOCKADDR_INET LocalSockaddr;
        SOCKADDR_IN LocalSockaddrV4;
        SOCKADDR_IN6 LocalSockaddrV6;
        struct
        {
            ADDRESS_FAMILY Family;
            USHORT Port;
            union
            {
                IN_ADDR AddressV4;              // AF_INET: the IPv4 address (tcpip!UdpEnumerateEndpoints); AF_INET6: 0.
                IPAddr AddressV4Value;
            } DUMMYUNIONNAME3;
            union
            {
                IN6_ADDR Address;
                IPv6Addr AddressValue;
            } DUMMYUNIONNAME2;
            SCOPE_ID ScopeId;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
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
    NET_FL_ISOLATION_MODE IsolationMode;                    // 0..2 (get: from the NDIS isolation mode; set: values above 2 are rejected).
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
    NET_IFINDEX VirtualInterfaceIndex;      // Best-effort name: index FlIfCreateVirtualInterface returned for the virtual interface.
    union
    {
        ULONG Counters32[2]; // Per-processor client interface statistics.
        struct
        {
            ULONG AttachedFlsnpiClients;
            ULONG FlsnpiClientConfigErrors;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG Reserved;             // alignment padding; nothing reads or writes it
    union
    {
        ULONG64 Counters64[15]; // Per-processor client interface statistics.
        struct
        {
            ULONG64 FlsnpiClientInjectErrors;
            ULONG64 FlsnpiClientCloneErrors;
            ULONG64 InFlsnpiIndicatedPackets;
            ULONG64 InFlsnpiClientReturnedPackets;
            ULONG64 InFlsnpiClientSilentlyDroppedPackets;
            ULONG64 InFlsnpiClientDroppedPackets;
            ULONG64 InFlsnpiClientInjectedPackets;
            ULONG64 InFlsnpiClientClonedPackets;
            ULONG64 OutFlsnpiIndicatedPackets;
            ULONG64 OutFlsnpiClientReturnedPackets;
            ULONG64 OutFlsnpiClientDroppedPackets;
            ULONG64 OutFlsnpiClientSilentlyDroppedPackets;
            ULONG64 OutFlsnpiClientInjectedPackets;
            ULONG64 OutFlsnpiClientClonedPackets;
            ULONG64 OutFlsnpiClientClonedPacketsForNbSplit;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;
} NSI_FL_VIRTUAL_INTERFACE_ROD, *PNSI_FL_VIRTUAL_INTERFACE_ROD;

/**
 * The NSI_FL_VIRTUAL_INTERFACE_ROS structure is the read-only static block of a framing-layer virtual interface.
 */
typedef struct _NSI_FL_VIRTUAL_INTERFACE_ROS
{
    NET_LUID VirtualInterfaceLuid;                                      // Best-effort name: LUID FlIfCreateVirtualInterface returned for the virtual interface.
    NET_FL_VIRTUAL_INTERFACE_ORIGIN VirtualInterfaceType;               // Best-effort name: 2 for the default client interface.
    ULONG Reserved;                                                     // alignment padding; nothing reads or writes it
} NSI_FL_VIRTUAL_INTERFACE_ROS, *PNSI_FL_VIRTUAL_INTERFACE_ROS;

/**
 * The NSI_FL_GLOBAL_RW structure is the read/write block of NsiFlObjectGlobal (no key; tcpip!FlGetAllGlobalParameters).
 * The values exist only when the framing-layer module has an optional callback registered; otherwise GetAll returns zeros
 * and SetAll fails with STATUS_NOT_SUPPORTED. SetAll requires NsiSetCreateOrSet and notifies the module's interfaces.
 */
typedef struct _NSI_FL_GLOBAL_RW
{
    UCHAR Value[2];                         // Meaning unknown.
} NSI_FL_GLOBAL_RW, *PNSI_FL_GLOBAL_RW;

/**
 * The NSI_FL_GLOBAL_ROD structure is the read-only dynamic block of NsiFlObjectGlobal.
 */
typedef struct _NSI_FL_GLOBAL_ROD
{
    ULONG Value00;                          // Copied from the framing-layer module (+0x1C8); meaning unknown.
    UCHAR Value04;                          // Copied from the framing-layer module (+0x178); meaning unknown.
    UCHAR Reserved[3];
} NSI_FL_GLOBAL_ROD, *PNSI_FL_GLOBAL_ROD;

/**
 * The NSI_FL_INTERFACE_RW structure is the read/write block of NsiFlObjectInterface (key: NET_LUID;
 * tcpip!FlGetAllInterfaceParameters). GetAll copies the interface's address bytes, up to the interface's own address
 * length (at most 32); the rest is left as passed in.
 */
typedef struct _NSI_FL_INTERFACE_RW
{
    UCHAR Address[32];                      // Most likely the link-layer address.
} NSI_FL_INTERFACE_RW, *PNSI_FL_INTERFACE_RW;

/**
 * The NSI_FL_WOL_PATTERN_KEY structure is the key of NsiFlObjectWolPattern (SetAll only, tcpip!FlSetAllWolPatternParameters).
 * The whole key is passed to FlpSetWolPattern; NsiSetCreateOnly adds the pattern and NsiSetDelete removes it.
 */
typedef struct _NSI_FL_WOL_PATTERN_KEY
{
    NET_LUID InterfaceLuid;
    UCHAR Pattern[0x98];
} NSI_FL_WOL_PATTERN_KEY, *PNSI_FL_WOL_PATTERN_KEY;

/**
 * The NSI_FL_NETBT_GLOBAL_TRIAGE_KEY structure is the key of NsiFlObjectNetBTGlobalTriage (NPI_MS_FL48_MODULEID only;
 * SetAll only, tcpip!FlSetAllFlNetBTGlobalTriageParameters). NsiSetCreateOnly stores the value in tcpip's
 * gNetBTGlobalTriageBlock and NsiSetDelete clears it; both require NsiActive.
 */
typedef struct _NSI_FL_NETBT_GLOBAL_TRIAGE_KEY
{
    ULONG64 TriageBlock;                    // Address of NetBT's triage block (kernel callers).
} NSI_FL_NETBT_GLOBAL_TRIAGE_KEY, *PNSI_FL_NETBT_GLOBAL_TRIAGE_KEY;

static_assert(sizeof(NSI_FL_GLOBAL_RW) == 2);
static_assert(sizeof(NSI_FL_GLOBAL_ROD) == 8);
static_assert(sizeof(NSI_FL_INTERFACE_RW) == 0x20);
static_assert(sizeof(NSI_FL_WOL_PATTERN_KEY) == 0xA0);
static_assert(sizeof(NSI_FL_NETBT_GLOBAL_TRIAGE_KEY) == 8);

//
// Teredo framing layer (NPI_MS_FL6T_MODULEID) object blocks; tcpip!Fl*Teredo*/Fl*Tunnel* handlers.
//

/**
 * The NSI_FL6T_TUNNEL_RW structure is the read/write block of NsiFl6tObjectTunnel (no key).
 */
typedef struct _NSI_FL6T_TUNNEL_RW
{
    BOOLEAN TriggerClient;                  // tcpip!TeredoTriggerClient.
} NSI_FL6T_TUNNEL_RW, *PNSI_FL6T_TUNNEL_RW;

/**
 * The NSI_FL6T_TUNNEL_ROD structure is the read-only dynamic block of NsiFl6tObjectTunnel.
 */
typedef struct _NSI_FL6T_TUNNEL_ROD
{
    ULONG State;                            // tcpip!Fl6tTunnelReadOnlyDynamic.
    ULONG Reserved;                         // Not written.
} NSI_FL6T_TUNNEL_ROD, *PNSI_FL6T_TUNNEL_ROD;

/**
 * The NSI_FL6T_TEREDO_RW structure is the read/write block of NsiFl6tObjectTeredo (no key).
 */
typedef struct _NSI_FL6T_TEREDO_RW
{
    ULONG State;                            // tcpip!Fl6tTeredoStateReadWrite.
    ULONG Value04;                          // Meaning unknown (possibly an IPv4 address).
    USHORT Value08;                         // Meaning unknown (possibly a port).
    USHORT Reserved;
} NSI_FL6T_TEREDO_RW, *PNSI_FL6T_TEREDO_RW;

/**
 * The NSI_FL6T_CLIENT_PORT_RW structure is the read/write block of NsiFl6tObjectClientPort (no key). GetAll fails with
 * STATUS_DEVICE_NOT_READY until the Teredo client has a port.
 */
typedef struct _NSI_FL6T_CLIENT_PORT_RW
{
    USHORT Port;                            // tcpip!Fl6tTeredoPortReadWrite.
} NSI_FL6T_CLIENT_PORT_RW, *PNSI_FL6T_CLIENT_PORT_RW;

/**
 * The NSI_FL6T_INTERFACE_REMOTE_ADDRESS_RW structure is the read/write block of NsiFl6tObjectInterfaceRemoteAddress
 * (key: NET_LUID). GetAll copies the bytes that follow the interface's local address (NSI_FL_INTERFACE_RW), up to the
 * same address length (at most 32).
 */
typedef struct _NSI_FL6T_INTERFACE_REMOTE_ADDRESS_RW
{
    UCHAR RemoteAddress[32];                // Most likely the tunnel's remote address.
} NSI_FL6T_INTERFACE_REMOTE_ADDRESS_RW, *PNSI_FL6T_INTERFACE_REMOTE_ADDRESS_RW;

/**
 * The NSI_FL6T_TUNNEL_PHYSICAL_INTERFACE_ASSOCIATION_RW structure is the read/write block of
 * NsiFl6tObjectTunnelPhysicalInterfaceAssociation (key: NET_LUID; SetAll only, NsiSetDefault). tcpip forwards it
 * unchanged to the interface as NDIS OID 0x0F010106.
 */
typedef struct _NSI_FL6T_TUNNEL_PHYSICAL_INTERFACE_ASSOCIATION_RW
{
    UCHAR Data[0x14];                       // OID payload; layout unknown.
} NSI_FL6T_TUNNEL_PHYSICAL_INTERFACE_ASSOCIATION_RW, *PNSI_FL6T_TUNNEL_PHYSICAL_INTERFACE_ASSOCIATION_RW;

/**
 * The NSI_FL6T_TUNNEL_POTENTIAL_ROUTER_RW structure is the read/write block of NsiFl6tObjectTunnelPotentialRouter
 * (key: NET_LUID; SetAll only). NsiSetCreateOrSet with the block, or NsiSetReset without it, is forwarded to the interface
 * as NDIS OID 0x0F010117.
 */
typedef struct _NSI_FL6T_TUNNEL_POTENTIAL_ROUTER_RW
{
    ULONG Value;                            // OID payload; meaning unknown (possibly an IPv4 address).
} NSI_FL6T_TUNNEL_POTENTIAL_ROUTER_RW, *PNSI_FL6T_TUNNEL_POTENTIAL_ROUTER_RW;

/**
 * The NSI_FL6T_TEREDO_SHUNT_RW structure is the read/write block of NsiFl6tObjectTeredoShunt (no key).
 */
typedef struct _NSI_FL6T_TEREDO_SHUNT_RW
{
    BOOLEAN Enabled;                        // tcpip!Fl6tTeredoShuntReadWrite.
    UCHAR Reserved[7];
    ULONG64 Value08;                        // Meaning unknown.
    ULONG64 Value10;                        // Meaning unknown.
} NSI_FL6T_TEREDO_SHUNT_RW, *PNSI_FL6T_TEREDO_SHUNT_RW;

static_assert(sizeof(NSI_FL6T_TUNNEL_ROD) == 8);
static_assert(sizeof(NSI_FL6T_TEREDO_RW) == 0xC);
static_assert(sizeof(NSI_FL6T_INTERFACE_REMOTE_ADDRESS_RW) == 0x20);
static_assert(sizeof(NSI_FL6T_TUNNEL_PHYSICAL_INTERFACE_ASSOCIATION_RW) == 0x14);
static_assert(sizeof(NSI_FL6T_TUNNEL_POTENTIAL_ROUTER_RW) == 4);
static_assert(sizeof(NSI_FL6T_TEREDO_SHUNT_RW) == 0x18);

//
// tcpip RDMA framing-layer provider (tcpip!FlRdmaNsi*).
//

/**
 * The NSI_FL_RDMA_INTERFACE_ROD structure is the read-only dynamic block of NsiFlRdmaObjectInterface (key: NET_IFINDEX).
 */
typedef struct _NSI_FL_RDMA_INTERFACE_ROD
{
    BOOLEAN NdkOperational;                 // TRUE when the interface's NDK state flags allow RDMA.
} NSI_FL_RDMA_INTERFACE_ROD, *PNSI_FL_RDMA_INTERFACE_ROD;

/**
 * The NSI_FL_RDMA_INTERFACE_ROS structure is the read-only static block of NsiFlRdmaObjectInterface.
 */
typedef struct _NSI_FL_RDMA_INTERFACE_ROS
{
    ULONG64 Value;                          // Copied from the framing-layer interface (+0x440); meaning unknown.
} NSI_FL_RDMA_INTERFACE_ROS, *PNSI_FL_RDMA_INTERFACE_ROS;

/**
 * The NSI_FL_RDMA_GLOBAL_RW structure is the read/write block of NsiFlRdmaObjectGlobal (no key).
 */
typedef struct _NSI_FL_RDMA_GLOBAL_RW
{
    BOOLEAN NdkEnabled;                     // !FlRdmaIsGlobalNDKDisabled().
} NSI_FL_RDMA_GLOBAL_RW, *PNSI_FL_RDMA_GLOBAL_RW;

/**
 * The NSI_FL_RDMA_LOCAL_ENDPOINT_KEY structure is the key of NsiFlRdmaObjectLocalEndpoint (enumeration only,
 * tcpip!FlRdmaNsiEnumerateNdkItemsCommon).
 */
#include <pshpack4.h>
typedef struct _NSI_FL_RDMA_LOCAL_ENDPOINT_KEY
{
    NET_IFINDEX InterfaceIndex;
    SOCKADDR_INET LocalAddress;
} NSI_FL_RDMA_LOCAL_ENDPOINT_KEY, *PNSI_FL_RDMA_LOCAL_ENDPOINT_KEY;

/**
 * The NSI_FL_RDMA_CONNECTION_KEY structure is the key of NsiFlRdmaObjectConnection (enumeration only).
 */
typedef struct _NSI_FL_RDMA_CONNECTION_KEY
{
    NET_IFINDEX InterfaceIndex;
    SOCKADDR_INET LocalAddress;
    SOCKADDR_INET RemoteAddress;
} NSI_FL_RDMA_CONNECTION_KEY, *PNSI_FL_RDMA_CONNECTION_KEY;
#include <poppack.h>

/**
 * The NSI_FL_RDMA_ENDPOINT_ROS structure is the read-only static block of NsiFlRdmaObjectConnection and
 * NsiFlRdmaObjectLocalEndpoint.
 */
typedef struct _NSI_FL_RDMA_ENDPOINT_ROS
{
    UCHAR Reserved00;                       // Not written.
    UCHAR Value01;                          // Meaning unknown.
    UCHAR Value02;                          // Meaning unknown.
    UCHAR Reserved03;                       // Not written.
    ULONG Value04;                          // Meaning unknown.
} NSI_FL_RDMA_ENDPOINT_ROS, *PNSI_FL_RDMA_ENDPOINT_ROS;

static_assert(sizeof(NSI_FL_RDMA_INTERFACE_ROS) == 8);
static_assert(sizeof(NSI_FL_RDMA_LOCAL_ENDPOINT_KEY) == 0x20);
static_assert(sizeof(NSI_FL_RDMA_CONNECTION_KEY) == 0x3C);
static_assert(sizeof(NSI_FL_RDMA_ENDPOINT_ROS) == 8);

/**
 * The read-only dynamic block of NsiRawObjectGlobal (key: ADDRESS_FAMILY; tcpip!RawGetAllGlobalParameters) has the
 * MIB_UDPSTATS2 layout: the raw transport's per-processor datagram counters summed over all processors.
 */
typedef MIB_UDPSTATS2 NSI_RAW_GLOBAL_ROD, *PNSI_RAW_GLOBAL_ROD;

/**
 * The NSI_RSS_INTERFACE_ROD structure is the read-only dynamic block of NsiRssObjectInterface (key: NET_IFINDEX;
 * tcpip!RssGetAllInterfaceParameters).
 */
typedef struct _NSI_RSS_INTERFACE_ROD
{
    BOOLEAN Value;                          // Flag 0x40 of the RSS binding; meaning unknown.
} NSI_RSS_INTERFACE_ROD, *PNSI_RSS_INTERFACE_ROD;

static_assert(sizeof(NSI_RAW_GLOBAL_ROD) == 0x20);

//
// TCP module (NPI_MS_TCP_MODULEID) object blocks not covered elsewhere.
//

/**
 * The NSI_TCP_INTERFACE_RW structure is the read/write block of NsiTcpObjectInterface (key: NET_LUID; the per-interface
 * TCP settings of tcpip's offload/OLM interface; tcpip!OlmQueryTcpInterfaceParametersByLuid,
 * OlmSetTcpInterfaceParametersByLuid). On set, 0xFFFF leaves a field unchanged; SetAll accepts NsiSetDefault and
 * NsiSetCreateOrSet, and OlmValidateTcpInterfaceParameters rejects out-of-range values without failing the request.
 */
typedef struct _NSI_TCP_INTERFACE_RW
{
    USHORT Value00;                         // 1-255; meaning unknown (stored at OLM interface +0x50).
    USHORT Value02;                         // 1-600; tcpip stores it multiplied by 100 and divides on read; meaning unknown.
    USHORT Value04;                         // 0 or 1; meaning unknown.
} NSI_TCP_INTERFACE_RW, *PNSI_TCP_INTERFACE_RW;

/**
 * The NSI_TCP_INTERFACE_ROS structure is the optional read-only static block of NsiTcpObjectInterface: a copy of 16 bytes
 * of the OLM interface (+0x58) that begin with the "explicitly configured" flags of NSI_TCP_INTERFACE_RW.
 */
typedef struct _NSI_TCP_INTERFACE_ROS
{
    BOOLEAN Value02Configured;              // Set when Value02 was set explicitly.
    UCHAR Reserved01[3];
    BOOLEAN Value00Configured;              // Set when Value00 was set explicitly.
    UCHAR Reserved05[11];                   // Rest of the copied interface data; meaning unknown.
} NSI_TCP_INTERFACE_ROS, *PNSI_TCP_INTERFACE_ROS;

/**
 * The NSI_TCP_INTERFACE_CONNECTION_OFFLOAD_ROD structure is the read-only dynamic block of
 * NsiTcpObjectInterfaceConnectionOffload (key: NET_LUID). Current builds always return it zeroed.
 */
typedef struct _NSI_TCP_INTERFACE_CONNECTION_OFFLOAD_ROD
{
    ULONG Value00;
    ULONG Value04;
    UCHAR Flags08;                          // Bits 0-2.
    UCHAR Reserved09[3];
    ULONG Flags0C;                          // Bits 0-3.
} NSI_TCP_INTERFACE_CONNECTION_OFFLOAD_ROD, *PNSI_TCP_INTERFACE_CONNECTION_OFFLOAD_ROD;

/**
 * The NSI_TCP_SECURITY_RW structure is the read/write block of NsiTcpObjectSecurity (no key): the settings of
 * "netsh interface tcp set security mpp=... profiles=..." (tcpip!TcpGetAllSecurityParameters, TcpSecuritySetAllParameters,
 * TcpMppSetAllParameters). On set, -1 leaves a field unchanged; NsiSetReset restores MemoryPressureProtection = 2 and
 * Profiles = 1. GetAll returns each field as a boolean (1 when enabled).
 */
typedef struct _NSI_TCP_SECURITY_RW
{
    UCHAR Reserved00;                       // Always 0 on read; not read on set.
    UCHAR Reserved01[3];
    ULONG MemoryPressureProtection;         // 0 disabled, 1 enabled, 2 default (treated as enabled); values above 2 fail.
    ULONG Profiles;                         // 0 disabled, 1 enabled; values above 1 fail.
} NSI_TCP_SECURITY_RW, *PNSI_TCP_SECURITY_RW;

/**
 * The NSI_TCP_SECURITY_ROD structure is the read-only dynamic block of NsiTcpObjectSecurity: a copy of tcpip!SecurityRod,
 * the counters of the SYN attack and memory pressure protection (MPP) code. Names are inferred from the functions that
 * update each counter.
 */
typedef struct _NSI_TCP_SECURITY_ROD
{
    ULONG SynRcvdLimitHits;                 // TcpCheckSynRcvdLimit.
    ULONG ReassemblyQuotaHits;              // TcpTcbReassemblyCheckQuota, TcpTcbReassemblyInsertNetBufferList.
    ULONG64 Reserved08;                     // Not updated by any function found.
    ULONG CarefulDatagramDrops;             // TcpSynTcbReceive, TcpTcbCarefulDatagram.
    ULONG Reserved14;
    ULONG64 MppEpisodes;                    // TcpMppStartEpisode.
    ULONG64 MppEvaluation;                  // Amount added by TcpPartitionStartOrContinueMppEvaluation; meaning unknown.
    ULONG64 MppTimeouts;                    // TcpTimerMppTimeoutHandler, TcpMppCheckNewEpisode.
} NSI_TCP_SECURITY_ROD, *PNSI_TCP_SECURITY_ROD;

/**
 * The NSI_TCP_RSC_STATS_ROD structure is the read-only dynamic block of NsiTcpObjectRscStats (key: NET_LUID, exactly
 * 8 bytes; tcpip!OlmTcpRscGetAllStats): the receive segment coalescing state of the interface (Get-NetAdapterRsc) and its
 * statistics. The NDIS counters come from OID_TCP_RSC_STATISTICS (NDIS_RSC_STATISTICS_INFO) of the IPv4 or IPv6
 * interface on which RSC is operational, and are zero otherwise.
 */
typedef struct _NSI_TCP_RSC_STATS_ROD
{
    BOOLEAN IPv4Enabled;
    BOOLEAN IPv6Enabled;
    BOOLEAN IPv4OperationalState;
    BOOLEAN IPv6OperationalState;
    ULONG Value04;                          // OLM interface +0x6C; meaning unknown.
    ULONG64 TcpCounter08;                   // Summed over processors (TCP per-processor +0x18); meaning unknown.
    ULONG64 TcpCounter10;                   // Summed over processors (TCP per-processor +0x20); meaning unknown.
    ULONG IPv4FailureReason;
    ULONG IPv6FailureReason;
    ULONG64 CoalescedPkts;                  // NDIS_RSC_STATISTICS_INFO.CoalescedPkts.
    ULONG64 CoalescedOctets;                // NDIS_RSC_STATISTICS_INFO.CoalescedOctets.
    ULONG64 CoalesceEvents;                 // NDIS_RSC_STATISTICS_INFO.CoalesceEvents.
    ULONG64 Aborts;                         // NDIS_RSC_STATISTICS_INFO.Aborts.
} NSI_TCP_RSC_STATS_ROD, *PNSI_TCP_RSC_STATS_ROD;

/**
 * The NSI_TCP_NC_CONTEXT_KEY structure is the key of NsiTcpObjectNcContext (exactly 16 bytes): a netio notification
 * channel context (NCM; likely the keep-alive offload used by push notification channels). Only the process registered
 * with netio's notification channel manager may query it; others get STATUS_ACCESS_DENIED.
 */
typedef struct _NSI_TCP_NC_CONTEXT_KEY
{
    ULONG ChannelId;                        // Hash table key of the notification channel.
    ULONG Reserved;
    ULONG64 ContextId;                      // Context within the channel.
} NSI_TCP_NC_CONTEXT_KEY, *PNSI_TCP_NC_CONTEXT_KEY;

/**
 * The NSI_TCP_NC_CONTEXT_ROD structure is the read-only dynamic block of NsiTcpObjectNcContext
 * (netio!NetioNcmGetAllNotificationChannelContextParameters). The read/write block (0x2C8 bytes) is consumed by
 * netio!NetioNcmNotificationChannelContextRequest and has not been decoded.
 */
typedef struct _NSI_TCP_NC_CONTEXT_ROD
{
    ULONG Value00;                          // Context +0x120; meaning unknown.
    ULONG Value04;                          // Bits 4-7 of the context flags.
    ULONG Value08;                          // Bits 8-11 of the context flags.
    ULONG Value0C;                          // Bits 0-3 of the context's second flags word.
    USHORT Value10;                         // Context +0x124; meaning unknown.
    UCHAR Value12;                          // From the first attached transport object, if any; meaning unknown.
    UCHAR Reserved13;
    ULONG Value14;                          // Context +0x128; meaning unknown.
} NSI_TCP_NC_CONTEXT_ROD, *PNSI_TCP_NC_CONTEXT_ROD;

static_assert(sizeof(NSI_TCP_INTERFACE_ROS) == 0x10);
static_assert(sizeof(NSI_TCP_SECURITY_ROD) == 0x30);
static_assert(sizeof(NSI_TCP_RSC_STATS_ROD) == 0x40);
static_assert(FIELD_OFFSET(NSI_TCP_RSC_STATS_ROD, IPv4FailureReason) == 0x18);
static_assert(FIELD_OFFSET(NSI_TCP_RSC_STATS_ROD, CoalescedPkts) == 0x20);
static_assert(sizeof(NSI_TCP_NC_CONTEXT_KEY) == 0x10);
static_assert(sizeof(NSI_TCP_NC_CONTEXT_ROD) == 0x18);

/**
 * The NSI_TCP_PORT_PROPERTY_KEY structure is the key of NsiTcpObjectPortProperty. GetAll matches it exactly; other
 * lookups match any port in [StartPort, StartPort + NumberOfPorts).
 */
typedef struct _NSI_TCP_PORT_PROPERTY_KEY
{
    USHORT StartPort;                       // Network byte order.
    USHORT NumberOfPorts;
} NSI_TCP_PORT_PROPERTY_KEY, *PNSI_TCP_PORT_PROPERTY_KEY;

/**
 * The NSI_TCP_PORT_PROPERTY_RW structure is the read/write block of NsiTcpObjectPortProperty.
 */
typedef struct _NSI_TCP_PORT_PROPERTY_RW
{
    ULONG Property;                         // Meaning unknown.
} NSI_TCP_PORT_PROPERTY_RW, *PNSI_TCP_PORT_PROPERTY_RW;

/**
 * The NSI_TCP_TEMPLATE_RW structure is the read/write block of NsiTcpObjectTemplate (key: ULONG template index 0-4,
 * NSI_TCP_APPLIED_SETTING; tcpip!TcpGetAllTemplateParameters copies one 20-byte tcpip!TcpTemplates entry).
 */
typedef struct _NSI_TCP_TEMPLATE_RW
{
    ULONG Value00;                          // Meaning unknown.
    ULONG Value04;                          // Meaning unknown.
    ULONG Value08;                          // Meaning unknown.
    UCHAR Value0C;                          // Meaning unknown.
    UCHAR Value0D;                          // Meaning unknown.
    UCHAR Reserved0E[2];                    // Not written.
    UCHAR Value10;                          // Meaning unknown.
    UCHAR Value11;                          // Meaning unknown.
    UCHAR Value12;                          // Meaning unknown.
    UCHAR Reserved13;                       // Not written.
} NSI_TCP_TEMPLATE_RW, *PNSI_TCP_TEMPLATE_RW;

/**
 * The NSI_TCP_DEFAULT_TEMPLATE_RW structure is the read/write block of NsiTcpObjectDefaultTemplate. The key is a ULONG
 * that must be 6.
 */
typedef struct _NSI_TCP_DEFAULT_TEMPLATE_RW
{
    NSI_TCP_APPLIED_SETTING DefaultTemplate; // tcpip!TcpGlobalDefaultTemplate.
} NSI_TCP_DEFAULT_TEMPLATE_RW, *PNSI_TCP_DEFAULT_TEMPLATE_RW;

/**
 * The NSI_TCP_GLOBAL_EX_RW structure is the read/write block of NsiTcpObjectGlobalEx (key: ADDRESS_FAMILY, AF_INET or
 * AF_INET6; tcpip!TcpGetAllGlobalExParameters). The read-only static block, when requested, is 1 byte.
 */
typedef struct _NSI_TCP_GLOBAL_EX_RW
{
    USHORT Value00;                         // Meaning unknown.
    USHORT Reserved02;
    UCHAR Value04;                          // Meaning unknown.
    UCHAR Reserved05[3];
    UCHAR Value08[7];                       // Meaning unknown.
    UCHAR Reserved0F;
} NSI_TCP_GLOBAL_EX_RW, *PNSI_TCP_GLOBAL_EX_RW;

static_assert(sizeof(NSI_TCP_INTERFACE_RW) == 6);
static_assert(sizeof(NSI_TCP_INTERFACE_CONNECTION_OFFLOAD_ROD) == 0x10);
static_assert(sizeof(NSI_TCP_SECURITY_RW) == 0xC);
static_assert(sizeof(NSI_TCP_PORT_PROPERTY_KEY) == 4);
static_assert(sizeof(NSI_TCP_TEMPLATE_RW) == 0x14);
static_assert(sizeof(NSI_TCP_GLOBAL_EX_RW) == 0x10);

static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, CompartmentId) == NsiNdisIfEnumRodCompartmentId);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, NetworkGuid) == NsiNdisIfEnumRodNetworkGuid);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifAlias) == NsiNdisIfEnumRodAlias);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifOperStatus) == NsiNdisIfEnumRodOperStatus);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifOperStatusFlags) == NsiNdisIfEnumRodOperStatusFlags);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifMtu) == NsiNdisIfEnumRodMtu);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifPhysAddress) == NsiNdisIfEnumRodPhysAddress);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, PermanentPhysAddress) == NsiNdisIfEnumRodPermanentPhysAddress);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifAdminStatus) == NsiNdisIfEnumRodAdminStatus);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, XmitLinkSpeed) == NsiNdisIfEnumRodXmitLinkSpeed);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, RcvLinkSpeed) == NsiNdisIfEnumRodRcvLinkSpeed);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifPromiscuousMode) == NsiNdisIfEnumRodPromiscuousMode);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifDeviceWakeUpEnable) == NsiNdisIfEnumRodDeviceWakeUpEnable);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifHideInterfaceInUi) == NsiNdisIfEnumRodHideInterfaceInUi);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, MediaConnectState) == NsiNdisIfEnumRodMediaConnectState);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, MediaDuplexState) == NsiNdisIfEnumRodMediaDuplexState);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, ifIndex) == NsiNdisIfEnumRosIndex);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, ifDescr) == NsiNdisIfEnumRosDescription);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, ifType) == NsiNdisIfEnumRosType);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, AccessType) == NsiNdisIfEnumRosAccessType);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, DirectionType) == NsiNdisIfEnumRosDirectionType);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, ConnectionType) == NsiNdisIfEnumRosConnectionType);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, InterfaceGuid) == NsiNdisIfEnumRosGuid);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, ifConnectorPresent) == NsiNdisIfEnumRosConnectorPresent);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, ifFlags) == NsiNdisIfEnumRosFlags);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, MediaType) == NsiNdisIfEnumRosMediaType);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, PhysicalMediumType) == NsiNdisIfEnumRosPhysicalMediumType);

static_assert(sizeof(MIB_FL_VIRTUAL_INTERFACE_ROW) == 0xC0);
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, IfLuid) == 0x08);
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, VirtualIfId) == 0x10);
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, AllowLocalNd) == 0x3C);
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, AttachedFlsnpiClients) == 0x40);
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 0x48);

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
    ULONG NumInterfaces;            // IP interfaces (IppCreateInterface/IpFlcDeleteInterface).
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
    ULONG InTruncatedPkts;          // RFC 4293 ipSystemStatsInTruncatedPkts.
    ULONG ReasmReqds;               // MIB_IPSTATS.dwReasmReqds
    ULONG ReasmOks;                 // MIB_IPSTATS.dwReasmOks
    ULONG ReasmFails;               // MIB_IPSTATS.dwReasmFails
    ULONG InDiscards;               // MIB_IPSTATS.dwInDiscards
    ULONG OutNoRoutes;              // MIB_IPSTATS.dwOutNoRoutes
    ULONG OutDiscards;              // MIB_IPSTATS.dwOutDiscards
    ULONG FragReqds;                // FragOks + FragFails (RFC 4293 ipSystemStatsOutFragReqds);
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
    ULONG UseTemporaryAddresses;                // netsh privacy "state" (RFC 4941 temporary addresses). IPv4 accepts only 0. Default 1.
    ULONG MaxTemporaryRandomTime;               // netsh privacy "maxrandomtime" (WMI MaxTemporaryDesyncTime). Default 600.
    ULONG MaxTemporaryValidLifetime;            // netsh privacy "maxvalidlifetime". Default 604800.
    ULONG MaxTemporaryPreferredLifetime;        // netsh privacy "maxpreferredlifetime"; at most MaxTemporaryValidLifetime. Default 86400.
    ULONG MaxTemporaryDadAttempts;              // netsh privacy "maxdadattempts" (IppDadFailed, IppRestartDad). Default 3.
    ULONG TemporaryRegenerateTime;              // netsh privacy "regeneratetime"; TemporaryRegenerateTime + MaxTemporaryRandomTime must be
                                                //      below MaxTemporaryPreferredLifetime. Default 5.
    ULONG DefaultHopLimit;                      // "defaultcurhoplimit", 1..255 (stored as a UCHAR). MIB_IPSTATS.dwDefaultTTL when the
                                                //      compartment has none.
    ULONG NeighborCacheLimit;                   // "neighborcachelimit", entries per interface.
    ULONG RouteCacheLimit;                      // "routecachelimit", entries per compartment.
    ULONG ReassemblyLimit;                      // "reassemblylimit", bytes.
    ULONG Reserved28;                           // alignment padding; nothing reads or writes it
    BOOLEAN RetiredSetting2C;                   // Retired setting: IppValidateGlobalRwParameters rejects any value but 0xFF and a get returns 0.
    UCHAR Reserved2D[3];                        // alignment padding; nothing reads or writes it
    ULONG DeadRouteProbeTimeout;                // Seconds a dead-gateway-detection probe state lasts before the route returns to dead
                                                //      (IppGetRouteState). Best-effort name.
    ULONG DeadRouteTimeout;                     // Seconds a route stays dead before it is probed again (IppGetRouteState). Best-effort name.
    ULONG PathUnreachabilityWindow;             // Seconds within which PathUnreachabilityThreshold indications must arrive before a
                                                //      path's route is suspected (IpNlpSuspectPathReachability). Best-effort name.
    ULONG DeadRouteProbePercent;                // 1..100 (stored as a UCHAR): share of traffic sent to a probed dead route
                                                //      (IppIsRouteDead). Best-effort name.
    NL_LINK_LOCAL_ADDRESS_BEHAVIOR DefaultLinkLocalAddressBehavior; // NL_LINK_LOCAL_ADDRESS_BEHAVIOR used for new interfaces whose framing layer
                                                //      supports it (IpFlcAddInterface). IPv6 accepts only 2 (always on).
    union
    {
        ULONG LinkLayerSuffixPrefix[4];         // IPv4 only, write-only (a get returns 0). The prefix occupies the first ULONG;
                                                //      the remaining three ULONGs retain their original storage.
        IPAddr LinkLayerSuffixPrefixV4;
        IN_ADDR LinkLayerSuffixPrefixAddress;   // Unicast prefix for addresses formed from the link-layer suffix.
    } DUMMYUNIONNAME;
    ULONG LinkLayerSuffixPrefixLength;          // IPv4 only, write-only: 1..32.
    ULONG SourceRoutingBehavior;                // "sourceroutingbehavior" (0 = forward, 1 = dontforward, 2 = drop).
    ULONG MldLevel;                             // "mldlevel" (0 = none, 1 = sendonly, 2 = all).
    ULONG MldVersion;                           // "mldversion": 2 = version1 (IPv4 only), 3 = version2, 4 = version3.
    ULONG DefaultDadTransmits;                  // 0..3: duplicate address detection transmits for new interfaces (IpFlcAddInterface);
                                                //      0xFFFFFFFF uses the framing layer's value. Server SKUs start at 0.
    BOOLEAN DefaultForwardingDisabled;          // TRUE when new compartments start with NSI_IP_COMPARTMENT_RW.Forwarding = 0
                                                //      (IppCreateCompartment). Best-effort name.
    BOOLEAN IcmpRedirects;                      // "icmpredirects"
    BOOLEAN AddressMaskReply;                   // "addressmaskreply"
    BOOLEAN DisableTaskOffload;                 // netsh "taskoffload" shows the inverse.
    BOOLEAN NeighborUpdateRequiresLinkLayerAddress; // IppUpdateNeighbor applies an unsolicited update only when it carries
                                                //      a link-layer address. Best-effort name.
    BOOLEAN DisableDhcpMediaSense;              // netsh "dhcpmediasense" shows the inverse (IppSetDhcpOperationalStatus).
    BOOLEAN DisableMediaSenseEventLog;          // netsh "mediasenseeventlog" shows the inverse.
    BOOLEAN MulticastForwarding;                // "multicastforwarding" (default for NSI_IP_COMPARTMENT_RW.MulticastForwarding).
    BOOLEAN GroupForwardedFragments;            // "groupforwardedfragments" (default for the compartment field).
    BOOLEAN RandomizeIdentifiers;               // "randomizeidentifiers"
    UCHAR Reserved72;                           // alignment padding; nothing reads or writes it
    BOOLEAN DisablePrefixLengthSorting;         // Write-only (a get returns 0): address sorting ignores the common prefix length
                                                //      (IppFillSortingElement). Best-effort name.
    BOOLEAN LimitAddressesPerInterface;         // Write-only (a get returns 0): IppFindOrCreateLocalAddress stops creating addresses of
                                                //      origin 68 on an interface after 100 and logs an event. Best-effort name.
    UCHAR Reserved75[3];                        // alignment padding; nothing reads or writes it
    ULONG MinMtu;                               // "minmtu", IPv4 only, 352..576 (stored as a USHORT).
    BOOLEAN DeadGatewayDetection;               // WMI DeadGatewayDetection (IppGetRouteState, IpNlpSuspectPathReachability).
    UCHAR PathUnreachabilityThreshold;          // Unreachability indications needed within PathUnreachabilityWindow
                                                //      (IpNlpSuspectPathReachability). Best-effort name.
    UCHAR DeadRouteMaxProbes;                   // Probes sent on a dead route per probe state (IppIsRouteDead). Best-effort name.
    UCHAR Reserved7F;                           // alignment padding; nothing reads or writes it
    BOOLEAN FlowLabel;                          // "flowlabel". IPv4 accepts only 0.
    BOOLEAN LoopbackLargeMtu;                   // "loopbacklargemtu"
    UCHAR LoopbackWorkerCount;                  // "loopbackworkercount", 1..16.
    UCHAR Reserved83;                           // alignment padding; nothing reads or writes it
    ULONG LoopbackExecutionMode;                // "loopbackexecutionmode" (0 = inline, 1 = worker, 2 = adaptive).
    BOOLEAN SourceBasedEcmp;                    // "SourceBasedEcmp"
    UCHAR Reserved89;                           // alignment padding; nothing reads or writes it
    USHORT ReassemblyOutOfOrderLimit;           // "reassemblyoutoforderlimit", fragments.
    BOOLEAN MultipleArpAnnouncements;           // "multiplearpannouncements", IPv4 only.
    UCHAR Reserved8D[3];                        // alignment padding; nothing reads or writes it
    ULONG SlaacPrivacyLevel;                    // "slaacprivacylevel", 0..2, applied for IPv6 only; when both are set, 1 requires
                                                //      RandomizeIdentifiers TRUE and 0 requires FALSE.
    ULONG SlaacMaxDadAttempts;                  // "slaacmaxdadattempts", non-zero.
    BOOLEAN RoutePolicies;                      // "routepolicies"
    BOOLEAN RecursiveReassembly;                // "recursivereassembly", IPv6 only.
    BOOLEAN IcmpJumbograms;                     // "icmpjumbograms", IPv6 only.
    BOOLEAN ReceiveAddressSetLookup;            // Receive paths look destinations up in the local address set first
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
                                    //      Ipv4/Ipv6EnumerateAllPaths).
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
    union
    {
        IN_ADDR SourceAddress;              // Best source address (GetBestRoute2 BestSourceAddress).
        IPAddr SourceAddressValue;
    } DUMMYUNIONNAME2;
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
    union
    {
        IN6_ADDR SourceAddress;             // Best source address (GetBestRoute2 BestSourceAddress).
        IPv6Addr SourceAddressValue;
    } DUMMYUNIONNAME2;
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
    ULONG Reserved;                         // alignment padding; nothing reads or writes it
    NET_LUID InterfaceLuid;
    union
    {
        IN_ADDR Destination;
        IPAddr DestinationValue;
    } DUMMYUNIONNAME;
    union
    {
        IN_ADDR Source;
        IPAddr SourceValue;
    } DUMMYUNIONNAME2;
} NSI_IP_PATH_KEY_V4, *PNSI_IP_PATH_KEY_V4;

/**
 * The NSI_IP_PATH_KEY_V6 structure is the IPv6 counterpart of NSI_IP_PATH_KEY_V4 (48 bytes).
 */
typedef struct _NSI_IP_PATH_KEY_V6
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    ULONG Reserved;                         // alignment padding; nothing reads or writes it
    NET_LUID InterfaceLuid;
    union
    {
        IN6_ADDR Destination;
        IPv6Addr DestinationValue;
    } DUMMYUNIONNAME;
    union
    {
        IN6_ADDR Source;
        IPv6Addr SourceValue;
    } DUMMYUNIONNAME2;
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
    BOOLEAN PathFlag40;                     // PathFlags & 0x40 (meaning not identified);
    UCHAR Reserved[2];                      // alignment padding; nothing reads or writes it
    NL_BANDWIDTH_INFORMATION InboundBandwidthInformation;
    NL_BANDWIDTH_INFORMATION OutboundBandwidthInformation;
    ULONG64 LinkTransmitSpeed;              // MIB_IPPATH_ROW.LinkTransmitSpeed
    ULONG64 LinkReceiveSpeed;               // MIB_IPPATH_ROW.LinkReceiveSpeed
    ULONG PathState;                        // Copied from the path (+0x24; meaning not identified);
    ULONG PathFlags;                        // Copied from the path (+0x28);
    union
    {
        IN_ADDR CurrentNextHop;             // MIB_IPPATH_ROW.CurrentNextHop
        IPAddr CurrentNextHopValue;
    } DUMMYUNIONNAME;
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
    UCHAR Reserved[2];
    NL_BANDWIDTH_INFORMATION InboundBandwidthInformation;
    NL_BANDWIDTH_INFORMATION OutboundBandwidthInformation;
    ULONG64 LinkTransmitSpeed;
    ULONG64 LinkReceiveSpeed;
    ULONG PathState;
    ULONG PathFlags;
    union
    {
        IN6_ADDR CurrentNextHop;
        IPv6Addr CurrentNextHopValue;
    } DUMMYUNIONNAME;
} NSI_IP_PATH_ROD_V6, *PNSI_IP_PATH_ROD_V6;

/**
 * The NSI_IP_BEST_ROUTE_KEY4 structure is the key of the IPv4 best-route object (NPI_MS_IPV4_MODULEID,
 * NsiIpObjectBestRoute). The provider selects the route and source address it would use to reach DestinationAddress,
 * optionally constrained to SourceAddress and InterfaceLuid. GetBestRoute2 and GetBestInterfaceEx use it.
 */
typedef struct _NSI_IP_BEST_ROUTE_KEY4
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    union
    {
        IN_ADDR DestinationAddress;
        IPAddr DestinationAddressValue;
    } DUMMYUNIONNAME;
    SCOPE_ID DestinationScopeId;
    union
    {
        IN_ADDR SourceAddress;
        IPAddr SourceAddressValue;
    } DUMMYUNIONNAME2;
    SCOPE_ID SourceScopeId;
    NET_LUID InterfaceLuid;
} NSI_IP_BEST_ROUTE_KEY4, *PNSI_IP_BEST_ROUTE_KEY4;

/**
 * The NSI_IP_BEST_ROUTE_KEY6 structure is the IPv6 counterpart of NSI_IP_BEST_ROUTE_KEY4.
 */
typedef struct _NSI_IP_BEST_ROUTE_KEY6
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    union
    {
        IN6_ADDR DestinationAddress;
        IPv6Addr DestinationAddressValue;
    } DUMMYUNIONNAME;
    SCOPE_ID DestinationScopeId;
    union
    {
        IN6_ADDR SourceAddress;
        IPv6Addr SourceAddressValue;
    } DUMMYUNIONNAME2;
    SCOPE_ID SourceScopeId;
    NET_LUID InterfaceLuid;
} NSI_IP_BEST_ROUTE_KEY6, *PNSI_IP_BEST_ROUTE_KEY6;

//
// IP module (NPI_MS_IPV4_MODULEID / NPI_MS_IPV6_MODULEID) object blocks not covered above. Layouts come from the tcpip.sys
// GetAll/SetAll handlers; field names marked "meaning unknown" record only what the handler copies.
//

/**
 * The NSI_IP_ECHO_SEQUENCE_REQUEST_ROD structure is the read-only dynamic block of NsiIpObjectEchoSequenceRequest (no
 * key). Each read returns the next echo sequence number (tcpip!IppGetAllEchoSequenceRequestParameters); the value is
 * always at least 0x10000.
 */
typedef struct _NSI_IP_ECHO_SEQUENCE_REQUEST_ROD
{
    ULONG Sequence;
} NSI_IP_ECHO_SEQUENCE_REQUEST_ROD, *PNSI_IP_ECHO_SEQUENCE_REQUEST_ROD;

/**
 * The key of NsiIpObjectMulticastAddress and NsiIpObjectProxyNeighbor has the anycast address key layout.
 */
typedef MIB_IP_ANYCAST_KEY_ROW_V4 NSI_IP_MULTICAST_ADDRESS_KEY_V4, *PNSI_IP_MULTICAST_ADDRESS_KEY_V4;
typedef MIB_IP_ANYCAST_KEY_ROW_V6 NSI_IP_MULTICAST_ADDRESS_KEY_V6, *PNSI_IP_MULTICAST_ADDRESS_KEY_V6;
typedef MIB_IP_ANYCAST_KEY_ROW_V4 NSI_IP_PROXY_NEIGHBOR_KEY_V4, *PNSI_IP_PROXY_NEIGHBOR_KEY_V4;
typedef MIB_IP_ANYCAST_KEY_ROW_V6 NSI_IP_PROXY_NEIGHBOR_KEY_V6, *PNSI_IP_PROXY_NEIGHBOR_KEY_V6;

/**
 * The NSI_IP_MULTICAST_ADDRESS_ROD structure is the read-only dynamic block of NsiIpObjectMulticastAddress
 * (tcpip!IppFillMulticastAddressRod).
 */
typedef struct _NSI_IP_MULTICAST_ADDRESS_ROD
{
    SCOPE_ID ScopeId;                       // External scope ID of the address.
    ULONG Value04;                          // Copied from the local multicast address (+0x78); meaning unknown.
    UCHAR Value08;                          // A per-address value on some interface types (+0x84), otherwise 1; meaning unknown.
    UCHAR Reserved[3];
} NSI_IP_MULTICAST_ADDRESS_ROD, *PNSI_IP_MULTICAST_ADDRESS_ROD;

/**
 * The NSI_IP_MULTICAST_ADDRESS_ROS structure is the read-only static block of NsiIpObjectMulticastAddress.
 */
typedef struct _NSI_IP_MULTICAST_ADDRESS_ROS
{
    NET_IFINDEX InterfaceIndex;
} NSI_IP_MULTICAST_ADDRESS_ROS, *PNSI_IP_MULTICAST_ADDRESS_ROS;

/**
 * The NSI_IP_POTENTIAL_ROUTER_KEY structure is the key of NsiIpObjectPotentialRouter (IPv6 only).
 */
typedef struct _NSI_IP_POTENTIAL_ROUTER_KEY
{
    NET_LUID InterfaceLuid;
    IN6_ADDR Address;
} NSI_IP_POTENTIAL_ROUTER_KEY, *PNSI_IP_POTENTIAL_ROUTER_KEY;

/**
 * The NSI_IP_POTENTIAL_ROUTER_ROD structure is the read-only dynamic block of NsiIpObjectPotentialRouter.
 */
typedef struct _NSI_IP_POTENTIAL_ROUTER_ROD
{
    ULONG Value;                            // Copied from the potential router entry (+0x18); meaning unknown.
} NSI_IP_POTENTIAL_ROUTER_ROD, *PNSI_IP_POTENTIAL_ROUTER_ROD;

/**
 * The NSI_IP_PREFIX_POLICY_KEY structure is the key of NsiIpObjectPrefixPolicy (IPv6 only; the RFC 6724 policy table).
 */
typedef struct _NSI_IP_PREFIX_POLICY_KEY
{
    IN6_ADDR Prefix;
    ULONG PrefixLength;
} NSI_IP_PREFIX_POLICY_KEY, *PNSI_IP_PREFIX_POLICY_KEY;

/**
 * The NSI_IP_PREFIX_POLICY_RW structure is the read/write block of NsiIpObjectPrefixPolicy.
 */
typedef struct _NSI_IP_PREFIX_POLICY_RW
{
    ULONG Precedence;
    ULONG Label;
} NSI_IP_PREFIX_POLICY_RW, *PNSI_IP_PREFIX_POLICY_RW;

/**
 * The NSI_IP_PROXY_NEIGHBOR_RW structure is the read/write block of NsiIpObjectProxyNeighbor.
 */
typedef struct _NSI_IP_PROXY_NEIGHBOR_RW
{
    UCHAR Value;                            // Copied from the proxy neighbor entry (+0x10); meaning unknown.
} NSI_IP_PROXY_NEIGHBOR_RW, *PNSI_IP_PROXY_NEIGHBOR_RW;

/**
 * The NSI_IP_SITE_PREFIX_KEY structure is the key of NsiIpObjectSitePrefix (IPv6 only;
 * tcpip!IppFindSitePrefixEntryUnderSpinLock).
 */
typedef struct _NSI_IP_SITE_PREFIX_KEY
{
    ULONG Value00;                          // Matched against the site prefix entry (+0x38); meaning unknown.
    ULONG Reserved04;
    NET_LUID InterfaceLuid;
    IN6_ADDR Prefix;
    ULONG PrefixLength;
    ULONG Reserved24;
} NSI_IP_SITE_PREFIX_KEY, *PNSI_IP_SITE_PREFIX_KEY;

/**
 * The NSI_IP_SITE_PREFIX_ROD structure is the read-only dynamic block of NsiIpObjectSitePrefix (tcpip!IppFillSitePrefixRod).
 */
typedef struct _NSI_IP_SITE_PREFIX_ROD
{
    ULONG ValidLifetime;                    // Seconds.
    ULONG PreferredLifetime;                // Seconds.
} NSI_IP_SITE_PREFIX_ROD, *PNSI_IP_SITE_PREFIX_ROD;

/**
 * The NSI_IP_SUB_INTERFACE_KEY structure is the key of NsiIpObjectSubInterface.
 */
typedef struct _NSI_IP_SUB_INTERFACE_KEY
{
    NET_LUID InterfaceLuid;
    NET_LUID SubInterfaceLuid;
} NSI_IP_SUB_INTERFACE_KEY, *PNSI_IP_SUB_INTERFACE_KEY;

/**
 * The NSI_IP_SUB_INTERFACE_RW structure is the read/write block of NsiIpObjectSubInterface.
 */
typedef struct _NSI_IP_SUB_INTERFACE_RW
{
    ULONG Value;                            // Copied from the sub-interface (+0x5C); meaning unknown.
} NSI_IP_SUB_INTERFACE_RW, *PNSI_IP_SUB_INTERFACE_RW;

/**
 * The NSI_IP_SUB_INTERFACE_ROD structure is the read-only dynamic block of NsiIpObjectSubInterface: one value followed by
 * counters that tcpip!IpGetAllSubInterfaceParameters sums over the per-processor statistics. The counter meanings are unknown.
 */
typedef struct _NSI_IP_SUB_INTERFACE_ROD
{
    ULONG Value;                            // Copied from the sub-interface (+0x60).
    ULONG Reserved04;
    ULONG64 Counters08[4];
    ULONG Counters28[6];
    ULONG64 Counters40[4];
} NSI_IP_SUB_INTERFACE_ROD, *PNSI_IP_SUB_INTERFACE_ROD;

/**
 * The NSI_IP_WAKE_UP_PATTERN_KEY structure is the key of NsiIpObjectWakeUpPattern (SetAll only). tcpip passes the whole
 * key to the framing layer; NsiSetCreateOrSet adds the pattern and NsiSetDelete removes it.
 */
typedef struct _NSI_IP_WAKE_UP_PATTERN_KEY
{
    NET_LUID InterfaceLuid;
    UCHAR Pattern[0x98];
} NSI_IP_WAKE_UP_PATTERN_KEY, *PNSI_IP_WAKE_UP_PATTERN_KEY;

/**
 * NsiIpObjectResolveNeighbor (ResolveIpNetEntry2) uses the NsiIpObjectNeighbor blocks. A local address resolves to
 * State 6 with the interface's own link-layer address.
 */
typedef MIB_IP_NEIGHBOR_KEY_ROW_V4 NSI_IP_RESOLVE_NEIGHBOR_KEY_V4, *PNSI_IP_RESOLVE_NEIGHBOR_KEY_V4;
typedef MIB_IP_NEIGHBOR_KEY_ROW_V6 NSI_IP_RESOLVE_NEIGHBOR_KEY_V6, *PNSI_IP_RESOLVE_NEIGHBOR_KEY_V6;

#define NSI_IP_SORTED_ADDRESS_MAX 500

/**
 * The NSI_IP_SORTED_ADDRESS_KEY structure is the key of NsiIpObjectSortedAddress (IPv6 only; CreateSortedAddressPairs).
 * tcpip!IpGetAllSortedAddressParameters rejects more than NSI_IP_SORTED_ADDRESS_MAX destinations.
 */
typedef struct _NSI_IP_SORTED_ADDRESS_KEY
{
    NET_IF_COMPARTMENT_ID CompartmentId;    // 0 or a compartment accessible by the caller.
    SOCKADDR_IN6 DestinationAddresses[NSI_IP_SORTED_ADDRESS_MAX];
    ULONG DestinationAddressCount;
    ULONG Options;
} NSI_IP_SORTED_ADDRESS_KEY, *PNSI_IP_SORTED_ADDRESS_KEY;

/**
 * The NSI_IP_SORTED_ADDRESS_ROD structure is the read-only dynamic block of NsiIpObjectSortedAddress
 * (tcpip!IppCreateSortedAddressPairsEx).
 */
typedef struct _NSI_IP_SORTED_ADDRESS_ROD
{
    SOCKADDR_IN6 SortedAddresses[NSI_IP_SORTED_ADDRESS_MAX];
    ULONG SortedAddressCount;
    UCHAR SortData[4000];                   // Array the sort writes; layout unknown.
    ULONG SortDataCount;
} NSI_IP_SORTED_ADDRESS_ROD, *PNSI_IP_SORTED_ADDRESS_ROD;

/**
 * The NSI_IP_MULTICAST_FORWARDING_KEY_V4/V6 structures are the keys of NsiIpObjectMulticastForwarding (one multicast
 * forwarding entry). The field order is inferred from the 0x10/0x28 key sizes.
 */
typedef struct _NSI_IP_MULTICAST_FORWARDING_KEY_V4
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    IN_ADDR Group;
    IN_ADDR Source;
    ULONG Reserved;
} NSI_IP_MULTICAST_FORWARDING_KEY_V4, *PNSI_IP_MULTICAST_FORWARDING_KEY_V4;

typedef struct _NSI_IP_MULTICAST_FORWARDING_KEY_V6
{
    NET_IF_COMPARTMENT_ID CompartmentId;
    IN6_ADDR Group;
    IN6_ADDR Source;
    ULONG Reserved;
} NSI_IP_MULTICAST_FORWARDING_KEY_V6, *PNSI_IP_MULTICAST_FORWARDING_KEY_V6;

#define NSI_IP_MULTICAST_FORWARDING_MAX_OUT 256

/**
 * The NSI_IP_MULTICAST_FORWARDING_RW structure is the read/write block of NsiIpObjectMulticastForwarding
 * (tcpip!IppGetAllMulticastForwardingParameters).
 */
typedef struct _NSI_IP_MULTICAST_FORWARDING_RW
{
    NET_IFINDEX IncomingInterfaceIndex;     // Forwarding entry +0x28.
    ULONG Value04;                          // Forwarding entry +0x58; meaning unknown.
    ULONG OutgoingInterfaceCount;
    struct
    {
        NET_IFINDEX InterfaceIndex;
        ULONG Value04;                      // Meaning unknown.
    } OutgoingInterfaces[NSI_IP_MULTICAST_FORWARDING_MAX_OUT];
} NSI_IP_MULTICAST_FORWARDING_RW, *PNSI_IP_MULTICAST_FORWARDING_RW;

/**
 * The NSI_IP_MULTICAST_FORWARDING_ROD structure is the read-only dynamic block of NsiIpObjectMulticastForwarding.
 */
typedef struct _NSI_IP_MULTICAST_FORWARDING_ROD
{
    NET_IFINDEX IncomingInterfaceIndex;
    ULONG OutgoingInterfaceCount;
    ULONG Value08;                          // Forwarding entry +0x8C; meaning unknown.
    ULONG Reserved0C;
    ULONG64 Value10;                        // Forwarding entry +0x90; meaning unknown.
    ULONG Value18;                          // Forwarding entry +0x98; meaning unknown.
    ULONG OutgoingValues[NSI_IP_MULTICAST_FORWARDING_MAX_OUT + 1]; // One per outgoing interface; meaning unknown.
} NSI_IP_MULTICAST_FORWARDING_ROD, *PNSI_IP_MULTICAST_FORWARDING_ROD;

/**
 * The NSI_IP_INTERFACE_HOP_ROD structure is the read-only dynamic block of NsiIpObjectInterfaceHop (key: NET_LUID).
 * Reading it resets the interface's tracked value (tcpip!IpGetAllInterfaceHopParameters).
 */
typedef struct _NSI_IP_INTERFACE_HOP_ROD
{
    UCHAR HopValue;                         // (-value) & 0x1F of the tracked interface byte; meaning unknown.
} NSI_IP_INTERFACE_HOP_ROD, *PNSI_IP_INTERFACE_HOP_ROD;

/**
 * The NSI_IP_TUNNEL_PHYSICAL_INTERFACE_KEY structure is the key of NsiIpObjectTunnelPhysicalInterface: a request to find
 * the physical interface that carries traffic from SourceAddress to DestinationAddress (tcpip!IpGetAllTunnelPhysicalInterface,
 * IppCreatePhysicalInterfaceRequest, IppProcessPhysicalInterfaceRequest). Only NsiGetExact is accepted.
 * \remarks DestinationAddress.ss_family must match the module and ProcessId must be the caller's; a duplicate outstanding
 * request fails with STATUS_DUPLICATE_OBJECTID. SourceAddress may be unspecified; otherwise it must have the same family and
 * be a unicast address of this computer. Requests time out after 120 timer ticks.
 */
typedef struct _NSI_IP_TUNNEL_PHYSICAL_INTERFACE_KEY
{
    SOCKADDR_STORAGE DestinationAddress;    // SOCKADDR_IN or SOCKADDR_IN6.
    SOCKADDR_STORAGE SourceAddress;         // SOCKADDR_IN or SOCKADDR_IN6; may be the unspecified address.
    NET_LUID InterfaceLuid;                 // Optional (0); constrains the route lookup to this interface.
    ULONG64 Reserved108;                    // Not read by the request processing.
    ULONG ProcessId;                        // Must be the calling process ID.
    ULONG Reserved114;
} NSI_IP_TUNNEL_PHYSICAL_INTERFACE_KEY, *PNSI_IP_TUNNEL_PHYSICAL_INTERFACE_KEY;

/**
 * The NSI_IP_TUNNEL_PHYSICAL_INTERFACE_ROD structure is the read-only dynamic block of NsiIpObjectTunnelPhysicalInterface.
 * It must be exactly 16 bytes (STATUS_INVALID_BUFFER_SIZE otherwise).
 */
typedef struct _NSI_IP_TUNNEL_PHYSICAL_INTERFACE_ROD
{
    NTSTATUS Status;                        // Result of the lookup; STATUS_PENDING while the request is outstanding.
    ULONG Reserved04;
    NET_LUID InterfaceLuid;                 // Interface of the next hop toward DestinationAddress.
} NSI_IP_TUNNEL_PHYSICAL_INTERFACE_ROD, *PNSI_IP_TUNNEL_PHYSICAL_INTERFACE_ROD;

/**
 * The read-only dynamic block of NsiIpObjectIcmp (no key; tcpip!IpGetAllIcmpParameters) is a copy of the module's
 * MIB_ICMP_EX_XPSP1 counters (inbound then outbound MIBICMPSTATS_EX). NsiGetExact and NsiGetFirst return it; NsiGetNext
 * returns STATUS_NO_MORE_ENTRIES.
 */
typedef MIB_ICMP_EX_XPSP1 NSI_IP_ICMP_ROD, *PNSI_IP_ICMP_ROD;

#define NSI_IP_MULTICAST_FORWARDING_CHANGE_MAX_PACKET 256
#define NSI_IP_MULTICAST_FORWARDING_CHANGE_KEYS 20

/**
 * The NSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V4/V6 structures are the notification payload of
 * NsiIpObjectMulticastForwardingChange (no key; change notifications only, tcpip!IppNotifyMfeChange /
 * IppNotifyMfeChangeWorker). The object's notification key is the compartment ID.
 * \remarks NotificationType 0 and 2 carry the start of the packet that triggered the notification; type 1 carries
 * KeyCount NSI_IP_MULTICAST_FORWARDING_KEY entries. With more than NSI_IP_MULTICAST_FORWARDING_CHANGE_KEYS keys the
 * payload grows to 8 + KeyCount * sizeof(key). The meaning of each type is inferred (likely no-cache, deleted entries and
 * wrong-interface).
 */
typedef struct _NSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V4
{
    ULONG NotificationType;
    union
    {
        NET_IFINDEX InterfaceIndex;         // Types 0 and 2.
        ULONG KeyCount;                     // Type 1.
    } DUMMYUNIONNAME;
    union
    {
        struct
        {
            ULONG Reserved;
            ULONG PacketLength;             // At most NSI_IP_MULTICAST_FORWARDING_CHANGE_MAX_PACKET.
            UCHAR Packet[NSI_IP_MULTICAST_FORWARDING_CHANGE_MAX_PACKET];
        } DUMMYSTRUCTNAME;
        NSI_IP_MULTICAST_FORWARDING_KEY_V4 Keys[NSI_IP_MULTICAST_FORWARDING_CHANGE_KEYS];
    } DUMMYUNIONNAME2;
} NSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V4, *PNSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V4;

typedef struct _NSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V6
{
    ULONG NotificationType;
    union
    {
        NET_IFINDEX InterfaceIndex;         // Types 0 and 2.
        ULONG KeyCount;                     // Type 1.
    } DUMMYUNIONNAME;
    union
    {
        struct
        {
            ULONG Reserved;
            ULONG PacketLength;             // At most NSI_IP_MULTICAST_FORWARDING_CHANGE_MAX_PACKET.
            UCHAR Packet[NSI_IP_MULTICAST_FORWARDING_CHANGE_MAX_PACKET];
        } DUMMYSTRUCTNAME;
        NSI_IP_MULTICAST_FORWARDING_KEY_V6 Keys[NSI_IP_MULTICAST_FORWARDING_CHANGE_KEYS];
    } DUMMYUNIONNAME2;
} NSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V6, *PNSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V6;

/**
 * The NSI_IP_FEATURE_FALLBACK_KEY and NSI_IP_FEATURE_FALLBACK_ROD structures are the rows of NsiIpObjectFeatureFallback
 * (IPv6 only; enumeration only, tcpip!IpEnumerateFeatureFallbackParameters).
 */
typedef struct _NSI_IP_FEATURE_FALLBACK_KEY
{
    UCHAR Value[16];                        // 16-byte hash table key (likely an IPv6 address).
    ULONG Reserved;
} NSI_IP_FEATURE_FALLBACK_KEY, *PNSI_IP_FEATURE_FALLBACK_KEY;

typedef struct _NSI_IP_FEATURE_FALLBACK_ROD
{
    ULONG Value;                            // Meaning unknown.
    UCHAR Flags;                            // Bits 0 and 1 are set from the entry; meaning unknown.
    UCHAR Value05;                          // Meaning unknown.
    USHORT Reserved;
} NSI_IP_FEATURE_FALLBACK_ROD, *PNSI_IP_FEATURE_FALLBACK_ROD;

/**
 * The NSI_IP_LOCALITY_PRIVATE_RW structure is the read/write block of NsiIpObjectLocalityPrivate (IPv6 only; SetAll only,
 * tcpip!IpSetAllLocalityPrivateParameters).
 */
typedef struct _NSI_IP_LOCALITY_PRIVATE_RW
{
    ULONG Operation;                        // 0 loads or creates the network's location table; 1 persists and deletes it.
    GUID NetworkId;
} NSI_IP_LOCALITY_PRIVATE_RW, *PNSI_IP_LOCALITY_PRIVATE_RW;

/**
 * The NSI_IP_TIMER_ROD structure is the read-only dynamic block of NsiIpObjectTimer (tcpip!IpTimerGetAllParameters).
 */
typedef struct _NSI_IP_TIMER_ROD
{
    UCHAR Table0[0xA8];                     // Copy of a tcpip timer table; layout unknown.
    UCHAR Table1[0xA8];                     // Copy of a second tcpip timer table; layout unknown.
    ULONG Expirations;                      // IppTimerExpirations.
    ULONG SpuriousExpirations;              // IppTimerSpuriousExpirations.
    ULONG ForcedRescheduleEpisodes;         // IppTimerForcedRescheduleEpisodes.
} NSI_IP_TIMER_ROD, *PNSI_IP_TIMER_ROD;

/**
 * The NSI_IP_DISCONNECT_INTERFACE_KEY structure is the key of NsiIpObjectDisconnectInterface (SetAll only, NsiSetReset).
 */
typedef struct _NSI_IP_DISCONNECT_INTERFACE_KEY
{
    NET_LUID InterfaceLuid;
    ULONG Flags;                            // Bit 0 changes how the interface is handled; meaning unknown.
    ULONG Reserved;
} NSI_IP_DISCONNECT_INTERFACE_KEY, *PNSI_IP_DISCONNECT_INTERFACE_KEY;

/**
 * The NSI_IP_ROUTER_INFORMATION_ROD structure is the read-only dynamic block of NsiIpObjectRouterInformation (IPv6 only;
 * key: NET_LUID; tcpip!Ipv6GetAllRouterInformation). The two lists are the router advertisement DNS options (RFC 8106);
 * lifetimes are remaining seconds.
 */
typedef struct _NSI_IP_ROUTER_INFORMATION_ROD
{
    USHORT DnsServerCount;
    USHORT DnsSearchListCount;
    ULONG Reserved04;
    struct
    {
        IN6_ADDR Address;
        ULONG Lifetime;
    } DnsServers[2];
    UCHAR Reserved30[0x10];                 // Filled only when the CLAT PREF64 feature is enabled.
    struct
    {
        ULONG Lifetime;
        CHAR Name[256];
    } DnsSearchList[3];
} NSI_IP_ROUTER_INFORMATION_ROD, *PNSI_IP_ROUTER_INFORMATION_ROD;

/**
 * The NSI_IP_SLAAC_SECRET_KEY_RW structure is the read/write block of NsiIpObjectSlaacSecretKey (IPv6 only).
 */
typedef struct _NSI_IP_SLAAC_SECRET_KEY_RW
{
    UCHAR SecretKey[16];                    // Secret used for stable privacy addresses (RFC 7217).
} NSI_IP_SLAAC_SECRET_KEY_RW, *PNSI_IP_SLAAC_SECRET_KEY_RW;

/**
 * The NSI_IP_ROUTING_EPOCH_ROD structure is the read-only dynamic block of NsiIpObjectRoutingEpoch (no key; change
 * notifications only).
 */
typedef struct _NSI_IP_ROUTING_EPOCH_ROD
{
    ULONG Epoch;
} NSI_IP_ROUTING_EPOCH_ROD, *PNSI_IP_ROUTING_EPOCH_ROD;

static_assert(sizeof(NSI_IP_ECHO_SEQUENCE_REQUEST_ROD) == 0x4);
static_assert(sizeof(NSI_IP_MULTICAST_ADDRESS_ROD) == 0xC);
static_assert(sizeof(NSI_IP_POTENTIAL_ROUTER_KEY) == 0x18);
static_assert(sizeof(NSI_IP_PREFIX_POLICY_KEY) == 0x14);
static_assert(sizeof(NSI_IP_PREFIX_POLICY_RW) == 0x8);
static_assert(sizeof(NSI_IP_SITE_PREFIX_KEY) == 0x28);
static_assert(FIELD_OFFSET(NSI_IP_SITE_PREFIX_KEY, PrefixLength) == 0x20);
static_assert(sizeof(NSI_IP_SITE_PREFIX_ROD) == 0x8);
static_assert(sizeof(NSI_IP_SUB_INTERFACE_KEY) == 0x10);
static_assert(sizeof(NSI_IP_SUB_INTERFACE_ROD) == 0x60);
static_assert(FIELD_OFFSET(NSI_IP_SUB_INTERFACE_ROD, Counters28) == 0x28);
static_assert(FIELD_OFFSET(NSI_IP_SUB_INTERFACE_ROD, Counters40) == 0x40);
static_assert(sizeof(NSI_IP_WAKE_UP_PATTERN_KEY) == 0xA0);
static_assert(sizeof(NSI_IP_SORTED_ADDRESS_KEY) == 0x36BC);
static_assert(FIELD_OFFSET(NSI_IP_SORTED_ADDRESS_KEY, DestinationAddressCount) == 0x36B4);
static_assert(sizeof(NSI_IP_SORTED_ADDRESS_ROD) == 0x4658);
static_assert(FIELD_OFFSET(NSI_IP_SORTED_ADDRESS_ROD, SortData) == 0x36B4);
static_assert(FIELD_OFFSET(NSI_IP_SORTED_ADDRESS_ROD, SortDataCount) == 0x4654);
static_assert(sizeof(NSI_IP_MULTICAST_FORWARDING_KEY_V4) == 0x10);
static_assert(sizeof(NSI_IP_MULTICAST_FORWARDING_KEY_V6) == 0x28);
static_assert(sizeof(NSI_IP_MULTICAST_FORWARDING_RW) == 0x80C);
static_assert(sizeof(NSI_IP_MULTICAST_FORWARDING_ROD) == 0x420);
static_assert(FIELD_OFFSET(NSI_IP_MULTICAST_FORWARDING_ROD, OutgoingValues) == 0x1C);
static_assert(sizeof(NSI_IP_TUNNEL_PHYSICAL_INTERFACE_KEY) == 0x118);
static_assert(FIELD_OFFSET(NSI_IP_TUNNEL_PHYSICAL_INTERFACE_KEY, ProcessId) == 0x110);
static_assert(FIELD_OFFSET(NSI_IP_TUNNEL_PHYSICAL_INTERFACE_KEY, SourceAddress) == 0x80);
static_assert(FIELD_OFFSET(NSI_IP_TUNNEL_PHYSICAL_INTERFACE_KEY, InterfaceLuid) == 0x100);
static_assert(sizeof(NSI_IP_TUNNEL_PHYSICAL_INTERFACE_ROD) == 0x10);
static_assert(FIELD_OFFSET(NSI_IP_TUNNEL_PHYSICAL_INTERFACE_ROD, InterfaceLuid) == 0x8);
static_assert(sizeof(NSI_IP_ICMP_ROD) == 0x810);
static_assert(sizeof(NSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V4) == 0x148);
static_assert(sizeof(NSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V6) == 0x328);
static_assert(FIELD_OFFSET(NSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V4, Packet) == 0x10);
static_assert(FIELD_OFFSET(NSI_IP_MULTICAST_FORWARDING_CHANGE_ROD_V4, Keys) == 0x8);
static_assert(sizeof(NSI_IP_FEATURE_FALLBACK_KEY) == 0x14);
static_assert(sizeof(NSI_IP_FEATURE_FALLBACK_ROD) == 0x8);
static_assert(sizeof(NSI_IP_LOCALITY_PRIVATE_RW) == 0x14);
static_assert(sizeof(NSI_IP_TIMER_ROD) == 0x15C);
static_assert(sizeof(NSI_IP_DISCONNECT_INTERFACE_KEY) == 0x10);
static_assert(sizeof(NSI_IP_ROUTER_INFORMATION_ROD) == 0x34C);
static_assert(FIELD_OFFSET(NSI_IP_ROUTER_INFORMATION_ROD, DnsSearchList) == 0x40);
static_assert(sizeof(NSI_IP_SLAAC_SECRET_KEY_RW) == 0x10);
static_assert(sizeof(NSI_IP_ROUTING_EPOCH_ROD) == 0x4);

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
static_assert(sizeof(NSI_UDP_STATUS_ROW) == 32, "NSI_UDP_STATUS_ROW size mismatch");
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
static_assert(sizeof(NDIS_NSI_SESSION_COMPARTMENT_KEY) == 0x04);
static_assert(sizeof(NDIS_NSI_SESSION_COMPARTMENT_RW) == 0x04);
static_assert(FIELD_OFFSET(NDIS_NSI_SESSION_COMPARTMENT_RW, CompartmentId) == NsiNdisSessionCompartmentType0Id);
static_assert(FIELD_OFFSET(NDIS_NSI_JOB_COMPARTMENT_RW, CompartmentId) == NsiNdisJobCompartmentType0Id);
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, LoopbackInterfaceLuid) == NsiNdisCompartmentType0LoopbackInterfaceLuid);
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, NetworkGuid) == NsiNdisCompartmentType0NetworkGuid);
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, LoopbackInterfaceGuid) == NsiNdisCompartmentType0LoopbackInterfaceGuid);
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, LoopbackInterfaceAlias) == NsiNdisCompartmentType0LoopbackInterfaceAlias);
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, LoopbackInterfaceDescription) == NsiNdisCompartmentType0LoopbackInterfaceDescription);
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, CompartmentGuid) == NsiNdisCompartmentType0CompartmentGuid);
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, CompartmentName) == NsiNdisCompartmentType0CompartmentName);
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, Flags) == NsiNdisCompartmentType0Flags);
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, NamespaceGuid) == NsiNdisCompartmentType0NamespaceGuid);
static_assert(sizeof(NDIS_NSI_JOB_COMPARTMENT_KEY) == sizeof(HANDLE));
static_assert(sizeof(NDIS_NSI_JOB_COMPARTMENT_RW) == 0x04);
static_assert(sizeof(NDIS_NSI_TIMESTAMP_ROD) == 0x04);
static_assert(sizeof(NDIS_NSI_INTERFACE_RAW_KEY) == 0x08);
static_assert(sizeof(NDIS_NSI_INTERFACE_RAW_RW) == 0x444);
static_assert(sizeof(NDIS_NSI_INTERFACE_RAW_ROD) == 0xD8);
static_assert(sizeof(NDIS_NSI_INTERFACE_RAW_ROS) == 0x258);
static_assert(sizeof(NDIS_NSI_INTERFACE_ENUM_RAW_KEY) == 0x08);
static_assert(sizeof(NDIS_NSI_INTERFACE_ENUM_RAW_ROD) == 0x290);
static_assert(sizeof(NDIS_NSI_INTERFACE_ENUM_RAW_ROS) == 0x238);
static_assert(sizeof(NDIS_NSI_INTERFACE_PERSIST_RW) == 0x484);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_PERSIST_RW, ifAlias) == 0x004);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_PERSIST_RW, ifDescr) == 0x208);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_PERSIST_RW, MediaType) == 0x434);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_PERSIST_RW, ifPhysAddress) == 0x440);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_PERSIST_RW, PermanentPhysAddress) == 0x462);
static_assert(sizeof(NSI_IP_INTERFACE_RW) == 0xf0);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, Metric) == 0x1c);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ZoneIndices) == 0x38);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, DisableDefaultRoutes) == 0x98);
static_assert(sizeof(NSI_IP_INTERFACE_ROD) == 0x58);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, InboundBandwidthInformation) == 0x28);
static_assert(sizeof(NSI_IP_INTERFACE_ROS) == 0x20);
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
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V4, Key) == NsiIpv4BestRouteType1Key);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V4, Route) == NsiIpv4BestRouteType1Route);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V4, Age) == NsiIpv4BestRouteType1Age);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V4, Origin) == NsiIpv4BestRouteType1Origin);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V4, InterfaceIndex) == NsiIpv4BestRouteType1BestIfIndex);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V4, SourceScopeId) == NsiIpv4BestRouteType1SourceScopeId);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V4, SourceAddress) == NsiIpv4BestRouteType1SourceAddress);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, Key) == NsiIpv6BestRouteType1Key);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, Route) == NsiIpv6BestRouteType1Route);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, Age) == NsiIpv6BestRouteType1Age);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, Origin) == NsiIpv6BestRouteType1Origin);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, InterfaceIndex) == NsiIpv6BestRouteType1BestIfIndex);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, SourceScopeId) == NsiIpv6BestRouteType1SourceScopeId);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, SourceAddress) == NsiIpv6BestRouteType1SourceAddress);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, AdvertisingEnabled) == NsiIpInterfaceType0AdvertisingEnabled);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ForwardingEnabled) == NsiIpInterfaceType0ForwardingEnabled);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, WeakHostSend) == NsiIpInterfaceType0WeakHostSend);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, WeakHostReceive) == NsiIpInterfaceType0WeakHostReceive);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, UseNeighborUnreachabilityDetection) == NsiIpInterfaceType0UseNeighborUnreachabilityDetection);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, UseAutomaticMetric) == NsiIpInterfaceType0UseAutomaticMetric);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ManagedAddressConfigurationSupported) == NsiIpInterfaceType0ManagedAddressConfigurationSupported);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, OtherStatefulConfigurationSupported) == NsiIpInterfaceType0OtherStatefulConfigurationSupported);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, AdvertiseDefaultRoute) == NsiIpInterfaceType0AdvertiseDefaultRoute);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, RouterDiscoveryBehavior) == NsiIpInterfaceType0RouterDiscoveryBehavior);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, Metric) == NsiIpInterfaceType0Metric);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, BaseReachableTime) == NsiIpInterfaceType0BaseReachableTime);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, RetransmitTime) == NsiIpInterfaceType0RetransmitTime);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, PathMtuDiscoveryTimeout) == NsiIpInterfaceType0PathMtuDiscoveryTimeout);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, DadTransmits) == NsiIpInterfaceType0DadTransmits);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, LinkLocalAddressBehavior) == NsiIpInterfaceType0LinkLocalAddressBehavior);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, LinkLocalAddressTimeout) == NsiIpInterfaceType0LinkLocalAddressTimeout);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ZoneIndices) == NsiIpInterfaceType0ZoneIndices);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, NlMtu) == NsiIpInterfaceType0NlMtu);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ZoneIndices) + ScopeLevelInterface * sizeof(ULONG) == NsiIpInterfaceType0ZoneIndexInterface);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ZoneIndices) + ScopeLevelLink * sizeof(ULONG) == NsiIpInterfaceType0ZoneIndexLink);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ZoneIndices) + ScopeLevelSubnet * sizeof(ULONG) == NsiIpInterfaceType0ZoneIndexSubnet);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ZoneIndices) + ScopeLevelAdmin * sizeof(ULONG) == NsiIpInterfaceType0ZoneIndexAdmin);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ZoneIndices) + ScopeLevelSite * sizeof(ULONG) == NsiIpInterfaceType0ZoneIndexSite);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ZoneIndices) + ScopeLevelOrganization * sizeof(ULONG) == NsiIpInterfaceType0ZoneIndexOrganization);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ZoneIndices) + ScopeLevelGlobal * sizeof(ULONG) == NsiIpInterfaceType0ZoneIndexGlobal);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, SitePrefixLength) == NsiIpInterfaceType0SitePrefixLength);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, DisableDefaultRoutes) == NsiIpInterfaceType0DisableDefaultRoutes);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, InterfaceIndex) == NsiIpInterfaceType1InterfaceIndex);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, CompartmentId) == NsiIpInterfaceType1CompartmentId);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, SupportsWakeUpPatterns) == NsiIpInterfaceType1SupportsWakeUpPatterns);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, ReachableTime) == NsiIpInterfaceType1ReachableTime);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, Connected) == NsiIpInterfaceType1Connected);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, TimerRemaining) == NsiIpInterfaceType1TimerRemaining);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, FlInterfaceContext) == NsiIpInterfaceType1FlInterfaceContext);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, TransmitOffload) == NsiIpInterfaceType1TransmitOffload);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, ReceiveOffload) == NsiIpInterfaceType1ReceiveOffload);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, InboundBandwidthInformation) == NsiIpInterfaceType1InboundBandwidthInformation);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, OutboundBandwidthInformation) == NsiIpInterfaceType1OutboundBandwidthInformation);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROS, SupportsNeighborDiscovery) == NsiIpInterfaceType2SupportsNeighborDiscovery);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROS, SupportsRouterDiscovery) == NsiIpInterfaceType2SupportsRouterDiscovery);
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
// The native UDP statistics view preserves the original 32-byte output, including padding.
static_assert(sizeof(NETIO_UDP_STATISTICS_EX2) == 0x20);
static_assert(sizeof(NETIO_UDP_STATISTICS_EX2) == sizeof(MIB_UDPSTATS2));
static_assert(__alignof(NETIO_UDP_STATISTICS_EX2) == __alignof(MIB_UDPSTATS2));
static_assert(FIELD_OFFSET(NETIO_UDP_STATISTICS_EX2, NativeStatistics) == 0);
static_assert(FIELD_OFFSET(NETIO_UDP_STATISTICS_EX2, Header) == 0);
static_assert(FIELD_OFFSET(NETIO_UDP_STATISTICS_EX2, Dynamic) == 0x10);
static_assert(FIELD_OFFSET(MIB_UDPSTATS2, dw64InDatagrams) == 0);
static_assert(FIELD_OFFSET(MIB_UDPSTATS2, dw64InDatagrams) == FIELD_OFFSET(NETIO_UDP_STATS_DYNAMIC, InDatagrams));
static_assert(sizeof(((MIB_UDPSTATS2*)0)->dw64InDatagrams) == sizeof(((NETIO_UDP_STATS_DYNAMIC*)0)->InDatagrams));
static_assert(FIELD_OFFSET(MIB_UDPSTATS2, dwNoPorts) == 8);
static_assert(FIELD_OFFSET(MIB_UDPSTATS2, dwNoPorts) == FIELD_OFFSET(NETIO_UDP_STATS_DYNAMIC, NoPorts));
static_assert(sizeof(((MIB_UDPSTATS2*)0)->dwNoPorts) == sizeof(((NETIO_UDP_STATS_DYNAMIC*)0)->NoPorts));
static_assert(FIELD_OFFSET(MIB_UDPSTATS2, dwInErrors) == 12);
static_assert(FIELD_OFFSET(MIB_UDPSTATS2, dwInErrors) == FIELD_OFFSET(NETIO_UDP_STATS_DYNAMIC, InErrors));
static_assert(sizeof(((MIB_UDPSTATS2*)0)->dwInErrors) == sizeof(((NETIO_UDP_STATS_DYNAMIC*)0)->InErrors));
static_assert(FIELD_OFFSET(MIB_UDPSTATS2, dw64OutDatagrams) == 16);
static_assert(FIELD_OFFSET(MIB_UDPSTATS2, dw64OutDatagrams) == FIELD_OFFSET(NETIO_UDP_STATS_DYNAMIC, OutDatagrams));
static_assert(sizeof(((MIB_UDPSTATS2*)0)->dw64OutDatagrams) == sizeof(((NETIO_UDP_STATS_DYNAMIC*)0)->OutDatagrams));
static_assert(FIELD_OFFSET(MIB_UDPSTATS2, dwNumAddrs) == 24);
static_assert(FIELD_OFFSET(MIB_UDPSTATS2, dwNumAddrs) == FIELD_OFFSET(NETIO_UDP_STATS_DYNAMIC, NumAddrs));
static_assert(sizeof(((MIB_UDPSTATS2*)0)->dwNumAddrs) == sizeof(((NETIO_UDP_STATS_DYNAMIC*)0)->NumAddrs));

// Native Boolean, lock, string, flag, and shared-header views preserve the existing ABI.
static_assert(sizeof(LOGICAL) == sizeof(ULONG));
static_assert(sizeof(RTL_CRITICAL_SECTION) == sizeof(CRITICAL_SECTION));
static_assert(__alignof(RTL_CRITICAL_SECTION) == __alignof(CRITICAL_SECTION));
static_assert(sizeof(((ICMP_REQUEST_V4*)0)->Reverse) == sizeof(UCHAR));
static_assert(sizeof(((ICMP_REQUEST_V6*)0)->Reverse) == sizeof(UCHAR));
static_assert(sizeof(NSI_COUNTED_WCHAR_PARAMETER) == sizeof(IF_COUNTED_STRING));
static_assert(__alignof(NSI_COUNTED_WCHAR_PARAMETER) == __alignof(IF_COUNTED_STRING));
static_assert(FIELD_OFFSET(NSI_COUNTED_WCHAR_PARAMETER, CountedString) == 0);
static_assert(FIELD_OFFSET(NSI_COUNTED_WCHAR_PARAMETER, Length) == FIELD_OFFSET(IF_COUNTED_STRING, Length));
static_assert(FIELD_OFFSET(NSI_COUNTED_WCHAR_PARAMETER, Buffer) == FIELD_OFFSET(IF_COUNTED_STRING, String));
static_assert(sizeof(((ICMP_NOTIFICATION_V4*)0)->FlagsBits) == sizeof(UCHAR));
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V4, FlagsBits) == FIELD_OFFSET(ICMP_NOTIFICATION_V4, Flags));
static_assert(sizeof(NSI_TCP_GLOBAL_HEADER) == 0x10);
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_ROS, GlobalParameters) == 0);
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_ROS, AutoTuningLevelLocalSet) == 0x10);
static_assert(FIELD_OFFSET(NETIO_TCP_STATISTICS_EX2, Header) == 0);
static_assert(FIELD_OFFSET(NETIO_TCP_STATISTICS_EX2, GlobalParameters) == 0);
static_assert(sizeof(((NETIO_TCP_STATISTICS_EX2*)0)->Header) == sizeof(NSI_TCP_GLOBAL_HEADER));
static_assert(FIELD_OFFSET(NETIO_TCP_STATISTICS_EX2, OutRsts) == 0x3C);
static_assert(FIELD_OFFSET(NETIO_TCP_STATISTICS_EX2, InErrs) == 0x40);
static_assert(sizeof(NETIO_NDIS_REQUEST_HEADER) == 0x2C);
static_assert(__alignof(NETIO_NDIS_REQUEST_HEADER) == 4);
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_HEADER, RtoAlgorithm) == 0 * sizeof(ULONG));
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_ROS, RtoAlgorithm) == FIELD_OFFSET(NSI_TCP_GLOBAL_HEADER, RtoAlgorithm));
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_HEADER, RtoMin) == 1 * sizeof(ULONG));
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_ROS, RtoMin) == FIELD_OFFSET(NSI_TCP_GLOBAL_HEADER, RtoMin));
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_HEADER, RtoMax) == 2 * sizeof(ULONG));
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_ROS, RtoMax) == FIELD_OFFSET(NSI_TCP_GLOBAL_HEADER, RtoMax));
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_HEADER, MaxConn) == 3 * sizeof(ULONG));
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_ROS, MaxConn) == FIELD_OFFSET(NSI_TCP_GLOBAL_HEADER, MaxConn));
static_assert(FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Header) == 0);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Header) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Header));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, Header) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Header));
static_assert(FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Reserved04) == 4);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Reserved04) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Reserved04));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, Reserved04) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Reserved04));
static_assert(FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Oid) == 12);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Oid) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Oid));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, Oid) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Oid));
static_assert(FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Reserved10) == 16);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Reserved10) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Reserved10));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, Reserved10) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Reserved10));
static_assert(FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Status) == 36);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Status) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Status));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, Status) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Status));
static_assert(FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, DataOffset) == 40);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, DataOffset) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, DataOffset));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, DataOffset) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, DataOffset));
static_assert(FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Reserved2A) == 42);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Reserved2A) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Reserved2A));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, Reserved2A) == FIELD_OFFSET(NETIO_NDIS_REQUEST_HEADER, Reserved2A));
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, RequestHeader) == 0);
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Data) == sizeof(NETIO_NDIS_REQUEST_HEADER));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, RequestHeader) == 0);
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, Data) == sizeof(NETIO_NDIS_REQUEST_HEADER));

static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, Connected) == 0x10);
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_HEADER, HasIcmpError) == 0x4);
static_assert(FIELD_OFFSET(NSI_COUNTED_WCHAR_PARAMETER, Length) == 0x0);
static_assert(FIELD_OFFSET(NSI_COUNTED_WCHAR_PARAMETER, Buffer) == 0x2);
#ifdef _WIN64
static_assert(FIELD_OFFSET(ICMP_HANDLE_INTERNAL, Lock) == 0x10);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, Reverse) == 0x55);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V6, Reverse) == 0x55);
#else
static_assert(FIELD_OFFSET(ICMP_HANDLE_INTERNAL, Lock) == 0xC);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, Reverse) == 0x3D);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V6, Reverse) == 0x3D);
#endif

// Unsigned statistics and native byte storage preserve existing field widths.
static_assert(sizeof(((NETIO_TCP_STATS_DYNAMIC*)0)->InSegs) == sizeof(ULONG64));
static_assert(sizeof(((NETIO_TCP_STATS_DYNAMIC*)0)->OutSegs) == sizeof(ULONG64));
static_assert(sizeof(((NETIO_UDP_STATS_DYNAMIC*)0)->InDatagrams) == sizeof(ULONG64));
static_assert(sizeof(((NETIO_UDP_STATS_DYNAMIC*)0)->OutDatagrams) == sizeof(ULONG64));
static_assert(sizeof(((ICMP_HANDLE_INTERNAL*)0)->Reserved0) == 3 * sizeof(UCHAR));
static_assert(sizeof(((ICMP_NOTIFICATION_V4*)0)->Reserved) == 2 * sizeof(UCHAR));
static_assert(sizeof(((ICMP_NOTIFICATION_V6*)0)->Reserved) == 2 * sizeof(UCHAR));
static_assert(sizeof(((ICMP_REQUEST_V4*)0)->Reserved0) == 2 * sizeof(UCHAR));
static_assert(sizeof(((ICMP_REQUEST_V4*)0)->Reserved1) == 2 * sizeof(UCHAR));
static_assert(sizeof(((ICMP_REQUEST_V4*)0)->Reserved2) == 3 * sizeof(UCHAR));
static_assert(sizeof(((MIB_IP_ROUTE_DYNAMIC_ROW_V4*)0)->InternalState) == 16 * sizeof(UCHAR));
static_assert(sizeof(((MIB_IP_ROUTE_DYNAMIC_ROW_V6*)0)->InternalState) == 28 * sizeof(UCHAR));
static_assert(sizeof(((NETIO_TCP_STATISTICS_EX2*)0)->InSegs) == sizeof(ULONG64));
static_assert(sizeof(((NETIO_TCP_STATISTICS_EX2*)0)->OutSegs) == sizeof(ULONG64));

static_assert(FIELD_OFFSET(NETIO_TCP_STATS_DYNAMIC, InSegs) == 0x18);
static_assert(FIELD_OFFSET(NETIO_TCP_STATS_DYNAMIC, OutSegs) == 0x20);
static_assert(FIELD_OFFSET(NETIO_UDP_STATS_DYNAMIC, InDatagrams) == 0x0);
static_assert(FIELD_OFFSET(NETIO_UDP_STATS_DYNAMIC, OutDatagrams) == 0x10);
static_assert(FIELD_OFFSET(ICMP_HANDLE_INTERNAL, Reserved0) == 0x5);
static_assert(sizeof(ICMP_NOTIFICATION_V4) == 0x24);
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V4, Reserved) == 0x16);
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V6, Reserved) == 0x16);
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_DYNAMIC_ROW_V4, InternalState) == 0x4);
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_DYNAMIC_ROW_V6, InternalState) == 0x4);
static_assert(sizeof(NETIO_TCP_STATISTICS_EX2) == 0x48);
static_assert(FIELD_OFFSET(NETIO_TCP_STATISTICS_EX2, InSegs) == 0x28);
static_assert(FIELD_OFFSET(NETIO_TCP_STATISTICS_EX2, OutSegs) == 0x30);
#ifdef _WIN64
static_assert(sizeof(ICMP_HANDLE_INTERNAL) == 0x50);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, Reserved0) == 0x4A);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, Reserved1) == 0x56);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, Reserved2) == 0x59);
#else
static_assert(sizeof(ICMP_HANDLE_INTERNAL) == 0x34);
static_assert(sizeof(ICMP_REQUEST_V4) == 0x50);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, Reserved0) == 0x32);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, Reserved1) == 0x3E);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, Reserved2) == 0x41);
#endif

// Native timestamp, pointer, and counter views retain the original row layouts.
static_assert(sizeof(MIB_IP_UNICAST_STATIC_ROW) == 8);
static_assert(FIELD_OFFSET(MIB_IP_UNICAST_STATIC_ROW, CreationTimestamp) == 0);
static_assert(sizeof(((MIB_IP_UNICAST_STATIC_ROW*)0)->CreationTimestamp) == sizeof(LARGE_INTEGER));
static_assert(sizeof(PVOID64) == sizeof(ULONG64));
static_assert(sizeof(NSI_IP_INTERFACE_ROD) == 0x58);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, FlInterfaceContext) == 0x18);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, FlInterfaceContextPointer) == 0x18);
static_assert(sizeof(((NSI_IP_INTERFACE_ROD*)0)->FlInterfaceContextPointer) == sizeof(((NSI_IP_INTERFACE_ROD*)0)->FlInterfaceContext));
static_assert(sizeof(NSI_FL_VIRTUAL_INTERFACE_ROD) == 0x88);
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters32) == 4);
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Reserved) == 0x0C);
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) == 0x10);
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->Counters32) == 2 * sizeof(ULONG));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->Counters64) == 15 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, AttachedFlsnpiClients) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters32) + 0 * sizeof(ULONG));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->AttachedFlsnpiClients) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->AttachedFlsnpiClients));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, AttachedFlsnpiClients) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, AttachedFlsnpiClients) == 0 * sizeof(ULONG));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, FlsnpiClientConfigErrors) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters32) + 1 * sizeof(ULONG));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->FlsnpiClientConfigErrors) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->FlsnpiClientConfigErrors));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientConfigErrors) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, AttachedFlsnpiClients) == 1 * sizeof(ULONG));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, FlsnpiClientInjectErrors) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 0 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->FlsnpiClientInjectErrors) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->FlsnpiClientInjectErrors));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 0 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, FlsnpiClientCloneErrors) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 1 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->FlsnpiClientCloneErrors) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->FlsnpiClientCloneErrors));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientCloneErrors) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 1 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, InFlsnpiIndicatedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 2 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->InFlsnpiIndicatedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->InFlsnpiIndicatedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, InFlsnpiIndicatedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 2 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, InFlsnpiClientReturnedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 3 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->InFlsnpiClientReturnedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->InFlsnpiClientReturnedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, InFlsnpiClientReturnedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 3 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, InFlsnpiClientSilentlyDroppedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 4 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->InFlsnpiClientSilentlyDroppedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->InFlsnpiClientSilentlyDroppedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, InFlsnpiClientSilentlyDroppedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 4 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, InFlsnpiClientDroppedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 5 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->InFlsnpiClientDroppedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->InFlsnpiClientDroppedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, InFlsnpiClientDroppedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 5 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, InFlsnpiClientInjectedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 6 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->InFlsnpiClientInjectedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->InFlsnpiClientInjectedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, InFlsnpiClientInjectedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 6 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, InFlsnpiClientClonedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 7 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->InFlsnpiClientClonedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->InFlsnpiClientClonedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, InFlsnpiClientClonedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 7 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, OutFlsnpiIndicatedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 8 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->OutFlsnpiIndicatedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->OutFlsnpiIndicatedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, OutFlsnpiIndicatedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 8 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, OutFlsnpiClientReturnedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 9 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->OutFlsnpiClientReturnedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->OutFlsnpiClientReturnedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, OutFlsnpiClientReturnedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 9 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, OutFlsnpiClientDroppedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 10 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->OutFlsnpiClientDroppedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->OutFlsnpiClientDroppedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, OutFlsnpiClientDroppedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 10 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, OutFlsnpiClientSilentlyDroppedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 11 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->OutFlsnpiClientSilentlyDroppedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->OutFlsnpiClientSilentlyDroppedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, OutFlsnpiClientSilentlyDroppedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 11 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, OutFlsnpiClientInjectedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 12 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->OutFlsnpiClientInjectedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->OutFlsnpiClientInjectedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, OutFlsnpiClientInjectedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 12 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, OutFlsnpiClientClonedPackets) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 13 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->OutFlsnpiClientClonedPackets) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->OutFlsnpiClientClonedPackets));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, OutFlsnpiClientClonedPackets) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 13 * sizeof(ULONG64));
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, OutFlsnpiClientClonedPacketsForNbSplit) == FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, Counters64) + 14 * sizeof(ULONG64));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->OutFlsnpiClientClonedPacketsForNbSplit) == sizeof(((MIB_FL_VIRTUAL_INTERFACE_ROW*)0)->OutFlsnpiClientClonedPacketsForNbSplit));
static_assert(FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, OutFlsnpiClientClonedPacketsForNbSplit) - FIELD_OFFSET(MIB_FL_VIRTUAL_INTERFACE_ROW, FlsnpiClientInjectErrors) == 14 * sizeof(ULONG64));

// IPv6 typedef aliases retain the existing address storage and layout.
static_assert(sizeof(IPv6Addr) == sizeof(IN6_ADDR));
static_assert(__alignof(IPv6Addr) == __alignof(IN6_ADDR));
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteAddressIPv6Value) == FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteAddressIPv6));
static_assert(sizeof(((NSI_SET_TCP_ENTRY*)0)->RemoteAddressIPv6Value) == sizeof(((NSI_SET_TCP_ENTRY*)0)->RemoteAddressIPv6));
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalAddressIPv6Value) == FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalAddressIPv6));
static_assert(sizeof(((NSI_SET_TCP_ENTRY*)0)->LocalAddressIPv6Value) == sizeof(((NSI_SET_TCP_ENTRY*)0)->LocalAddressIPv6));
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V6, AddressValue) == FIELD_OFFSET(ICMP_NOTIFICATION_V6, Address));
static_assert(sizeof(((ICMP_NOTIFICATION_V6*)0)->AddressValue) == sizeof(((ICMP_NOTIFICATION_V6*)0)->Address));
static_assert(FIELD_OFFSET(ICMP_REQUEST_V6, DestinationAddressValue) == FIELD_OFFSET(ICMP_REQUEST_V6, DestinationAddress));
static_assert(sizeof(((ICMP_REQUEST_V6*)0)->DestinationAddressValue) == sizeof(((ICMP_REQUEST_V6*)0)->DestinationAddress));
static_assert(FIELD_OFFSET(ICMP_REQUEST_V6, SourceAddressValue) == FIELD_OFFSET(ICMP_REQUEST_V6, SourceAddress));
static_assert(sizeof(((ICMP_REQUEST_V6*)0)->SourceAddressValue) == sizeof(((ICMP_REQUEST_V6*)0)->SourceAddress));
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteAddressIPv6Value) == FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteAddressIPv6));
static_assert(sizeof(((NSI_ESTATS_TCP_KEY*)0)->RemoteAddressIPv6Value) == sizeof(((NSI_ESTATS_TCP_KEY*)0)->RemoteAddressIPv6));
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalAddressIPv6Value) == FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalAddressIPv6));
static_assert(sizeof(((NSI_ESTATS_TCP_KEY*)0)->LocalAddressIPv6Value) == sizeof(((NSI_ESTATS_TCP_KEY*)0)->LocalAddressIPv6));
static_assert(FIELD_OFFSET(MIB_IP_ANYCAST_KEY_ROW_V6, AddressValue) == FIELD_OFFSET(MIB_IP_ANYCAST_KEY_ROW_V6, Address));
static_assert(sizeof(((MIB_IP_ANYCAST_KEY_ROW_V6*)0)->AddressValue) == sizeof(((MIB_IP_ANYCAST_KEY_ROW_V6*)0)->Address));
static_assert(FIELD_OFFSET(MIB_IP_NEIGHBOR_KEY_ROW_V6, AddressValue) == FIELD_OFFSET(MIB_IP_NEIGHBOR_KEY_ROW_V6, Address));
static_assert(sizeof(((MIB_IP_NEIGHBOR_KEY_ROW_V6*)0)->AddressValue) == sizeof(((MIB_IP_NEIGHBOR_KEY_ROW_V6*)0)->Address));
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, NextHopValue) == FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, NextHop));
static_assert(sizeof(((MIB_IP_ROUTE_KEY_ROW_V6*)0)->NextHopValue) == sizeof(((MIB_IP_ROUTE_KEY_ROW_V6*)0)->NextHop));
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, SourcePrefixValue) == FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, SourcePrefix));
static_assert(sizeof(((MIB_IP_ROUTE_KEY_ROW_V6*)0)->SourcePrefixValue) == sizeof(((MIB_IP_ROUTE_KEY_ROW_V6*)0)->SourcePrefix));
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, DestinationPrefixValue) == FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, DestinationPrefix));
static_assert(sizeof(((MIB_IP_ROUTE_KEY_ROW_V6*)0)->DestinationPrefixValue) == sizeof(((MIB_IP_ROUTE_KEY_ROW_V6*)0)->DestinationPrefix));
static_assert(FIELD_OFFSET(MIB_IP_UNICAST_KEY_ROW_V6, AddressValue) == FIELD_OFFSET(MIB_IP_UNICAST_KEY_ROW_V6, Address));
static_assert(sizeof(((MIB_IP_UNICAST_KEY_ROW_V6*)0)->AddressValue) == sizeof(((MIB_IP_UNICAST_KEY_ROW_V6*)0)->Address));
static_assert(FIELD_OFFSET(NSI_NEIGHBOR_IPV6_ROW, AddressValue) == FIELD_OFFSET(NSI_NEIGHBOR_IPV6_ROW, Address));
static_assert(sizeof(((NSI_NEIGHBOR_IPV6_ROW*)0)->AddressValue) == sizeof(((NSI_NEIGHBOR_IPV6_ROW*)0)->Address));
static_assert(FIELD_OFFSET(NSI_TCP_IPV6_ROW, RemoteAddressValue) == FIELD_OFFSET(NSI_TCP_IPV6_ROW, RemoteAddress));
static_assert(sizeof(((NSI_TCP_IPV6_ROW*)0)->RemoteAddressValue) == sizeof(((NSI_TCP_IPV6_ROW*)0)->RemoteAddress));
static_assert(FIELD_OFFSET(NSI_TCP_IPV6_ROW, LocalAddressValue) == FIELD_OFFSET(NSI_TCP_IPV6_ROW, LocalAddress));
static_assert(sizeof(((NSI_TCP_IPV6_ROW*)0)->LocalAddressValue) == sizeof(((NSI_TCP_IPV6_ROW*)0)->LocalAddress));
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteAddressV6Value) == FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteAddressV6));
static_assert(sizeof(((NSI_TCP_KEY_ROW*)0)->RemoteAddressV6Value) == sizeof(((NSI_TCP_KEY_ROW*)0)->RemoteAddressV6));
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalAddressV6Value) == FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalAddressV6));
static_assert(sizeof(((NSI_TCP_KEY_ROW*)0)->LocalAddressV6Value) == sizeof(((NSI_TCP_KEY_ROW*)0)->LocalAddressV6));
static_assert(FIELD_OFFSET(NSI_UDP_IPV6_ROW, LocalAddressValue) == FIELD_OFFSET(NSI_UDP_IPV6_ROW, LocalAddress));
static_assert(sizeof(((NSI_UDP_IPV6_ROW*)0)->LocalAddressValue) == sizeof(((NSI_UDP_IPV6_ROW*)0)->LocalAddress));
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalAddressV6Value) == FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalAddressV6));
static_assert(sizeof(((NSI_UDP_KEY_ROW*)0)->LocalAddressV6Value) == sizeof(((NSI_UDP_KEY_ROW*)0)->LocalAddressV6));
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteAddressV6Value) == FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteAddressV6));
static_assert(sizeof(((NSI_UDP_STATUS_ROW*)0)->RemoteAddressV6Value) == sizeof(((NSI_UDP_STATUS_ROW*)0)->RemoteAddressV6));
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemoteAddressValue) == FIELD_OFFSET(TCP_KEY_ROW, RemoteAddress));
static_assert(sizeof(((TCP_KEY_ROW*)0)->RemoteAddressValue) == sizeof(((TCP_KEY_ROW*)0)->RemoteAddress));
static_assert(FIELD_OFFSET(TCP_KEY_ROW, LocalAddressValue) == FIELD_OFFSET(TCP_KEY_ROW, LocalAddress));
static_assert(sizeof(((TCP_KEY_ROW*)0)->LocalAddressValue) == sizeof(((TCP_KEY_ROW*)0)->LocalAddress));
static_assert(FIELD_OFFSET(UDP_KEY_ROW, AddressValue) == FIELD_OFFSET(UDP_KEY_ROW, Address));
static_assert(sizeof(((UDP_KEY_ROW*)0)->AddressValue) == sizeof(((UDP_KEY_ROW*)0)->Address));
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, SourceAddressValue) == FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, SourceAddress));
static_assert(sizeof(((NSI_IP_BEST_ROUTE_ROD_V6*)0)->SourceAddressValue) == sizeof(((NSI_IP_BEST_ROUTE_ROD_V6*)0)->SourceAddress));
static_assert(FIELD_OFFSET(NSI_IP_PATH_KEY_V6, SourceValue) == FIELD_OFFSET(NSI_IP_PATH_KEY_V6, Source));
static_assert(sizeof(((NSI_IP_PATH_KEY_V6*)0)->SourceValue) == sizeof(((NSI_IP_PATH_KEY_V6*)0)->Source));
static_assert(FIELD_OFFSET(NSI_IP_PATH_KEY_V6, DestinationValue) == FIELD_OFFSET(NSI_IP_PATH_KEY_V6, Destination));
static_assert(sizeof(((NSI_IP_PATH_KEY_V6*)0)->DestinationValue) == sizeof(((NSI_IP_PATH_KEY_V6*)0)->Destination));
static_assert(FIELD_OFFSET(NSI_IP_PATH_ROD_V6, CurrentNextHopValue) == FIELD_OFFSET(NSI_IP_PATH_ROD_V6, CurrentNextHop));
static_assert(sizeof(((NSI_IP_PATH_ROD_V6*)0)->CurrentNextHopValue) == sizeof(((NSI_IP_PATH_ROD_V6*)0)->CurrentNextHop));
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_KEY6, SourceAddressValue) == FIELD_OFFSET(NSI_IP_BEST_ROUTE_KEY6, SourceAddress));
static_assert(sizeof(((NSI_IP_BEST_ROUTE_KEY6*)0)->SourceAddressValue) == sizeof(((NSI_IP_BEST_ROUTE_KEY6*)0)->SourceAddress));
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_KEY6, DestinationAddressValue) == FIELD_OFFSET(NSI_IP_BEST_ROUTE_KEY6, DestinationAddress));
static_assert(sizeof(((NSI_IP_BEST_ROUTE_KEY6*)0)->DestinationAddressValue) == sizeof(((NSI_IP_BEST_ROUTE_KEY6*)0)->DestinationAddress));

static_assert(sizeof(NSI_IP_PATH_KEY_V6) == 0x30);
static_assert(sizeof(NSI_NEIGHBOR_IPV6_ROW) == 0x50);
static_assert(sizeof(ICMP_NOTIFICATION_V6) == 0x30);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_KEY6, DestinationAddressValue) == 0x4);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_KEY6, SourceAddressValue) == 0x18);
static_assert(FIELD_OFFSET(NSI_IP_PATH_ROD_V6, CurrentNextHopValue) == 0x60);
static_assert(FIELD_OFFSET(NSI_IP_PATH_KEY_V6, DestinationValue) == 0x10);
static_assert(FIELD_OFFSET(NSI_IP_PATH_KEY_V6, SourceValue) == 0x20);
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V6, SourceAddressValue) == 0x84);
static_assert(FIELD_OFFSET(UDP_KEY_ROW, AddressValue) == 0x8);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, LocalAddressValue) == 0x8);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemoteAddressValue) == 0x24);
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteAddressV6Value) == 0x8);
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalAddressV6Value) == 0x8);
static_assert(FIELD_OFFSET(NSI_UDP_IPV6_ROW, LocalAddressValue) == 0x0);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalAddressV6Value) == 0x8);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteAddressV6Value) == 0x24);
static_assert(FIELD_OFFSET(NSI_TCP_IPV6_ROW, LocalAddressValue) == 0x0);
static_assert(FIELD_OFFSET(NSI_TCP_IPV6_ROW, RemoteAddressValue) == 0x18);
static_assert(FIELD_OFFSET(NSI_NEIGHBOR_IPV6_ROW, AddressValue) == 0x0);
static_assert(FIELD_OFFSET(MIB_IP_UNICAST_KEY_ROW_V6, AddressValue) == 0x8);
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, DestinationPrefixValue) == 0x4);
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, SourcePrefixValue) == 0x16);
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V6, NextHopValue) == 0x38);
static_assert(FIELD_OFFSET(MIB_IP_NEIGHBOR_KEY_ROW_V6, AddressValue) == 0x10);
static_assert(FIELD_OFFSET(MIB_IP_ANYCAST_KEY_ROW_V6, AddressValue) == 0x8);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalAddressIPv6Value) == 0x8);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteAddressIPv6Value) == 0x24);
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V6, AddressValue) == 0x1C);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalAddressIPv6Value) == 0x8);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteAddressIPv6Value) == 0x24);
#ifdef _WIN64
static_assert(FIELD_OFFSET(ICMP_REQUEST_V6, SourceAddressValue) == 0x58);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V6, DestinationAddressValue) == 0x68);
#else
static_assert(sizeof(ICMP_REQUEST_V6) == 0x60);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V6, SourceAddressValue) == 0x40);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V6, DestinationAddressValue) == 0x50);
#endif

// Native byte/short types and header views preserve their existing storage.
static_assert(sizeof(ADDRESS_FAMILY) == sizeof(USHORT));
static_assert(sizeof(NET_IFTYPE) == sizeof(USHORT));
static_assert(sizeof(BOOLEAN) == sizeof(UCHAR));
static_assert(sizeof(NDIS_OBJECT_HEADER) == 4);
static_assert(sizeof(NDIS_TIMESTAMP_CAPABILITY_FLAGS) == 14);
static_assert(FIELD_OFFSET(NSI_REQUEST_HEADER, MustBeZero) == 0);
static_assert(FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleHandle) == 8);
static_assert(FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved0C) == 0x0C);
static_assert(FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleId) == 0x10);
static_assert(__alignof(NSI_REQUEST_HEADER) == 4);
#ifdef _WIN64
static_assert(sizeof(NSI_REQUEST_HEADER) == 0x20);
static_assert(FIELD_OFFSET(NSI_REQUEST_HEADER, ObjectId) == 0x18);
static_assert(FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved1C) == 0x1C);
#else
static_assert(sizeof(NSI_REQUEST_HEADER) == 0x1C);
static_assert(FIELD_OFFSET(NSI_REQUEST_HEADER, ObjectId) == 0x14);
static_assert(FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved1C) == 0x18);
#endif
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, RequestHeader) == 0);
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, MustBeZero) == FIELD_OFFSET(NSI_REQUEST_HEADER, MustBeZero));
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, ModuleHandle) == FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleHandle));
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, Reserved0C) == FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved0C));
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, ModuleId) == FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleId));
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, ObjectId) == FIELD_OFFSET(NSI_REQUEST_HEADER, ObjectId));
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, Reserved1C) == FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved1C));
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, RequestHeader) == 0);
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, MustBeZero) == FIELD_OFFSET(NSI_REQUEST_HEADER, MustBeZero));
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, ModuleHandle) == FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleHandle));
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, Reserved0C) == FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved0C));
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, ModuleId) == FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleId));
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, ObjectId) == FIELD_OFFSET(NSI_REQUEST_HEADER, ObjectId));
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, Reserved1C) == FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved1C));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_GET_REQUEST, RequestHeader) == 0);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_GET_REQUEST, MustBeZero) == FIELD_OFFSET(NSI_REQUEST_HEADER, MustBeZero));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_GET_REQUEST, ModuleHandle) == FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleHandle));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_GET_REQUEST, Reserved0C) == FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved0C));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_GET_REQUEST, ModuleId) == FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleId));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_GET_REQUEST, ObjectId) == FIELD_OFFSET(NSI_REQUEST_HEADER, ObjectId));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_GET_REQUEST, Reserved1C) == FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved1C));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, RequestHeader) == 0);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, MustBeZero) == FIELD_OFFSET(NSI_REQUEST_HEADER, MustBeZero));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, ModuleHandle) == FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleHandle));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, Reserved0C) == FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved0C));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, ModuleId) == FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleId));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, ObjectId) == FIELD_OFFSET(NSI_REQUEST_HEADER, ObjectId));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, Reserved1C) == FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved1C));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, RequestHeader) == 0);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, MustBeZero) == FIELD_OFFSET(NSI_REQUEST_HEADER, MustBeZero));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, ModuleHandle) == FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleHandle));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, Reserved0C) == FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved0C));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, ModuleId) == FIELD_OFFSET(NSI_REQUEST_HEADER, ModuleId));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, ObjectId) == FIELD_OFFSET(NSI_REQUEST_HEADER, ObjectId));
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, Reserved1C) == FIELD_OFFSET(NSI_REQUEST_HEADER, Reserved1C));
static_assert(FIELD_OFFSET(NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA, Header) == 0);
static_assert(FIELD_OFFSET(NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA, Type) == FIELD_OFFSET(NDIS_OBJECT_HEADER, Type));
static_assert(FIELD_OFFSET(NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA, Revision) == FIELD_OFFSET(NDIS_OBJECT_HEADER, Revision));
static_assert(FIELD_OFFSET(NETIO_NDIS_HARDWARE_CROSSTIMESTAMP_DATA, Size) == FIELD_OFFSET(NDIS_OBJECT_HEADER, Size));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA, Header) == 0);
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA, Type) == FIELD_OFFSET(NDIS_OBJECT_HEADER, Type));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA, Revision) == FIELD_OFFSET(NDIS_OBJECT_HEADER, Revision));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA, Size) == FIELD_OFFSET(NDIS_OBJECT_HEADER, Size));
static_assert(FIELD_OFFSET(NETIO_NDIS_CROSSTIMESTAMP_REQUEST, Header) == 0);
static_assert(sizeof(((NETIO_NDIS_CROSSTIMESTAMP_REQUEST*)0)->Header) == sizeof(NDIS_OBJECT_HEADER));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST, Header) == 0);
static_assert(sizeof(((NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST*)0)->Header) == sizeof(NDIS_OBJECT_HEADER));
static_assert(FIELD_OFFSET(NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA, TimestampFlagsInfo) == 0x28);
static_assert(sizeof(((NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA*)0)->TimestampFlags) == sizeof(((NETIO_NDIS_TIMESTAMP_CAPABILITIES_DATA*)0)->TimestampFlagsInfo));

static_assert(sizeof(NSI_EX_REQUEST_COMMON) == 0x28);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, AdvertisingEnabled) == 0x0);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ForwardingEnabled) == 0x1);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, WeakHostSend) == 0x3);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, WeakHostReceive) == 0x4);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, UseNeighborUnreachabilityDetection) == 0x5);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, UseAutomaticMetric) == 0x6);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, ManagedAddressConfigurationSupported) == 0xA);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, OtherStatefulConfigurationSupported) == 0xB);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, AdvertiseDefaultRoute) == 0xC);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROS, SupportsNeighborDiscovery) == 0x0);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROS, SupportsRouterDiscovery) == 0x1);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_INFORMATION_ROS, ifType) == 0x208);
static_assert(sizeof(NSI_NEIGHBOR_IPV4_ROW) == 0x48);
#ifdef _WIN64
static_assert(sizeof(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX) == 0x70);
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, Store) == 0x20);
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, Action) == 0x24);
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, KeyTable) == 0x28);
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, Store) == 0x20);
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, GetAction) == 0x24);
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, SetAction) == 0x24);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_GET_REQUEST, Key) == 0x20);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_GET_REQUEST, KeySize) == 0x28);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, Action) == 0x20);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, Reserved24) == 0x24);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, Key) == 0x28);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, KeySize) == 0x30);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, Action) == 0x20);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, Reserved24) == 0x24);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, KeyTable) == 0x28);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, KeyEntrySize) == 0x30);
#else
static_assert(sizeof(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX) == 0x60);
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, Store) == 0x1C);
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, Action) == 0x20);
static_assert(FIELD_OFFSET(NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX, KeyTable) == 0x24);
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, Store) == 0x1C);
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, GetAction) == 0x20);
static_assert(FIELD_OFFSET(NSI_EX_REQUEST_COMMON, SetAction) == 0x20);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_GET_REQUEST, Key) == 0x1C);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_GET_REQUEST, KeySize) == 0x20);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, Action) == 0x1C);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, Reserved24) == 0x20);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, Key) == 0x24);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_SET_REQUEST, KeySize) == 0x28);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, Action) == 0x1C);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, Reserved24) == 0x20);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, KeyTable) == 0x24);
static_assert(FIELD_OFFSET(NSI_PERSISTENT_ENUMERATE_REQUEST, KeyEntrySize) == 0x28);
#endif

// Scalar IPv4 views inside socket endpoints preserve the address slots.
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteAddressIPv4Value) == 32);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteAddressIPv4Value) == FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteAddressIPv4));
static_assert(sizeof(((NSI_SET_TCP_ENTRY*)0)->RemoteAddressIPv4Value) == sizeof(((NSI_SET_TCP_ENTRY*)0)->RemoteAddressIPv4));
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalAddressIPv4Value) == 4);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalAddressIPv4Value) == FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalAddressIPv4));
static_assert(sizeof(((NSI_SET_TCP_ENTRY*)0)->LocalAddressIPv4Value) == sizeof(((NSI_SET_TCP_ENTRY*)0)->LocalAddressIPv4));
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteAddressIPv4Value) == 32);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteAddressIPv4Value) == FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteAddressIPv4));
static_assert(sizeof(((NSI_ESTATS_TCP_KEY*)0)->RemoteAddressIPv4Value) == sizeof(((NSI_ESTATS_TCP_KEY*)0)->RemoteAddressIPv4));
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalAddressIPv4Value) == 4);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalAddressIPv4Value) == FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalAddressIPv4));
static_assert(sizeof(((NSI_ESTATS_TCP_KEY*)0)->LocalAddressIPv4Value) == sizeof(((NSI_ESTATS_TCP_KEY*)0)->LocalAddressIPv4));
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteAddressV4Value) == 32);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteAddressV4Value) == FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteAddressV4));
static_assert(sizeof(((NSI_TCP_KEY_ROW*)0)->RemoteAddressV4Value) == sizeof(((NSI_TCP_KEY_ROW*)0)->RemoteAddressV4));
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalAddressV4Value) == 4);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalAddressV4Value) == FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalAddressV4));
static_assert(sizeof(((NSI_TCP_KEY_ROW*)0)->LocalAddressV4Value) == sizeof(((NSI_TCP_KEY_ROW*)0)->LocalAddressV4));
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalAddressV4Value) == 4);
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalAddressV4Value) == FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalAddressV4));
static_assert(sizeof(((NSI_UDP_KEY_ROW*)0)->LocalAddressV4Value) == sizeof(((NSI_UDP_KEY_ROW*)0)->LocalAddressV4));
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteAddressV4Value) == 4);
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteAddressV4Value) == FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteAddressV4));
static_assert(sizeof(((NSI_UDP_STATUS_ROW*)0)->RemoteAddressV4Value) == sizeof(((NSI_UDP_STATUS_ROW*)0)->RemoteAddressV4));
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemoteAddressV4Value) == 32);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemoteAddressV4Value) == FIELD_OFFSET(TCP_KEY_ROW, RemoteAddressV4));
static_assert(sizeof(((TCP_KEY_ROW*)0)->RemoteAddressV4Value) == sizeof(((TCP_KEY_ROW*)0)->RemoteAddressV4));
static_assert(FIELD_OFFSET(TCP_KEY_ROW, LocalAddressV4Value) == 4);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, LocalAddressV4Value) == FIELD_OFFSET(TCP_KEY_ROW, LocalAddressV4));
static_assert(sizeof(((TCP_KEY_ROW*)0)->LocalAddressV4Value) == sizeof(((TCP_KEY_ROW*)0)->LocalAddressV4));
static_assert(FIELD_OFFSET(UDP_KEY_ROW, AddressV4Value) == 4);
static_assert(FIELD_OFFSET(UDP_KEY_ROW, AddressV4Value) == FIELD_OFFSET(UDP_KEY_ROW, AddressV4));
static_assert(sizeof(((UDP_KEY_ROW*)0)->AddressV4Value) == sizeof(((UDP_KEY_ROW*)0)->AddressV4));

// Structured and scalar IPv4 address views share the existing field storage.
static_assert(FIELD_OFFSET(MIB_IP_ANYCAST_KEY_ROW_V4, AddressValue) == FIELD_OFFSET(MIB_IP_ANYCAST_KEY_ROW_V4, Address));
static_assert(sizeof(((MIB_IP_ANYCAST_KEY_ROW_V4*)0)->AddressValue) == sizeof(((MIB_IP_ANYCAST_KEY_ROW_V4*)0)->Address));
static_assert(FIELD_OFFSET(MIB_IP_NEIGHBOR_KEY_ROW_V4, AddressValue) == FIELD_OFFSET(MIB_IP_NEIGHBOR_KEY_ROW_V4, Address));
static_assert(sizeof(((MIB_IP_NEIGHBOR_KEY_ROW_V4*)0)->AddressValue) == sizeof(((MIB_IP_NEIGHBOR_KEY_ROW_V4*)0)->Address));
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, NextHopValue) == FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, NextHop));
static_assert(sizeof(((MIB_IP_ROUTE_KEY_ROW_V4*)0)->NextHopValue) == sizeof(((MIB_IP_ROUTE_KEY_ROW_V4*)0)->NextHop));
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, SourcePrefixValue) == FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, SourcePrefix));
static_assert(sizeof(((MIB_IP_ROUTE_KEY_ROW_V4*)0)->SourcePrefixValue) == sizeof(((MIB_IP_ROUTE_KEY_ROW_V4*)0)->SourcePrefix));
static_assert(FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, DestinationPrefixValue) == FIELD_OFFSET(MIB_IP_ROUTE_KEY_ROW_V4, DestinationPrefix));
static_assert(sizeof(((MIB_IP_ROUTE_KEY_ROW_V4*)0)->DestinationPrefixValue) == sizeof(((MIB_IP_ROUTE_KEY_ROW_V4*)0)->DestinationPrefix));
static_assert(FIELD_OFFSET(MIB_IP_UNICAST_KEY_ROW_V4, AddressValue) == FIELD_OFFSET(MIB_IP_UNICAST_KEY_ROW_V4, Address));
static_assert(sizeof(((MIB_IP_UNICAST_KEY_ROW_V4*)0)->AddressValue) == sizeof(((MIB_IP_UNICAST_KEY_ROW_V4*)0)->Address));
static_assert(FIELD_OFFSET(NSI_NEIGHBOR_IPV4_ROW, AddressValue) == FIELD_OFFSET(NSI_NEIGHBOR_IPV4_ROW, Address));
static_assert(sizeof(((NSI_NEIGHBOR_IPV4_ROW*)0)->AddressValue) == sizeof(((NSI_NEIGHBOR_IPV4_ROW*)0)->Address));
static_assert(FIELD_OFFSET(NSI_TCP_IPV4_ROW, RemoteAddressValue) == FIELD_OFFSET(NSI_TCP_IPV4_ROW, RemoteAddress));
static_assert(sizeof(((NSI_TCP_IPV4_ROW*)0)->RemoteAddressValue) == sizeof(((NSI_TCP_IPV4_ROW*)0)->RemoteAddress));
static_assert(FIELD_OFFSET(NSI_TCP_IPV4_ROW, LocalAddressValue) == FIELD_OFFSET(NSI_TCP_IPV4_ROW, LocalAddress));
static_assert(sizeof(((NSI_TCP_IPV4_ROW*)0)->LocalAddressValue) == sizeof(((NSI_TCP_IPV4_ROW*)0)->LocalAddress));
static_assert(FIELD_OFFSET(NSI_UDP_IPV4_ROW, LocalAddressValue) == FIELD_OFFSET(NSI_UDP_IPV4_ROW, LocalAddress));
static_assert(sizeof(((NSI_UDP_IPV4_ROW*)0)->LocalAddressValue) == sizeof(((NSI_UDP_IPV4_ROW*)0)->LocalAddress));
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V4, SourceAddressValue) == FIELD_OFFSET(NSI_IP_BEST_ROUTE_ROD_V4, SourceAddress));
static_assert(sizeof(((NSI_IP_BEST_ROUTE_ROD_V4*)0)->SourceAddressValue) == sizeof(((NSI_IP_BEST_ROUTE_ROD_V4*)0)->SourceAddress));
static_assert(FIELD_OFFSET(NSI_IP_PATH_KEY_V4, SourceValue) == FIELD_OFFSET(NSI_IP_PATH_KEY_V4, Source));
static_assert(sizeof(((NSI_IP_PATH_KEY_V4*)0)->SourceValue) == sizeof(((NSI_IP_PATH_KEY_V4*)0)->Source));
static_assert(FIELD_OFFSET(NSI_IP_PATH_KEY_V4, DestinationValue) == FIELD_OFFSET(NSI_IP_PATH_KEY_V4, Destination));
static_assert(sizeof(((NSI_IP_PATH_KEY_V4*)0)->DestinationValue) == sizeof(((NSI_IP_PATH_KEY_V4*)0)->Destination));
static_assert(FIELD_OFFSET(NSI_IP_PATH_ROD_V4, CurrentNextHopValue) == FIELD_OFFSET(NSI_IP_PATH_ROD_V4, CurrentNextHop));
static_assert(sizeof(((NSI_IP_PATH_ROD_V4*)0)->CurrentNextHopValue) == sizeof(((NSI_IP_PATH_ROD_V4*)0)->CurrentNextHop));
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_KEY4, SourceAddressValue) == FIELD_OFFSET(NSI_IP_BEST_ROUTE_KEY4, SourceAddress));
static_assert(sizeof(((NSI_IP_BEST_ROUTE_KEY4*)0)->SourceAddressValue) == sizeof(((NSI_IP_BEST_ROUTE_KEY4*)0)->SourceAddress));
static_assert(FIELD_OFFSET(NSI_IP_BEST_ROUTE_KEY4, DestinationAddressValue) == FIELD_OFFSET(NSI_IP_BEST_ROUTE_KEY4, DestinationAddress));
static_assert(sizeof(((NSI_IP_BEST_ROUTE_KEY4*)0)->DestinationAddressValue) == sizeof(((NSI_IP_BEST_ROUTE_KEY4*)0)->DestinationAddress));

// Typed handles and callback views preserve pointer-sized NSI storage.
static_assert(sizeof(HANDLE) == sizeof(PVOID));
static_assert(sizeof(PHANDLE) == sizeof(PVOID*));
static_assert(sizeof(RPC_BINDING_HANDLE) == sizeof(PVOID));
static_assert(sizeof(((NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST*)0)->NotificationHandle) == sizeof(PVOID));
static_assert(sizeof(((NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST*)0)->NotificationHandle) == sizeof(PVOID*));
static_assert(sizeof(((NETIO_CHANGE_CALLBACK_CONTEXT*)0)->InterfaceCallback) == sizeof(PVOID));
static_assert(sizeof(((NETIO_CHANGE_CALLBACK_CONTEXT*)0)->RouteCallback) == sizeof(PVOID));
static_assert(FIELD_OFFSET(NETIO_CHANGE_CALLBACK_CONTEXT, InterfaceCallback) == FIELD_OFFSET(NETIO_CHANGE_CALLBACK_CONTEXT, Callback));
static_assert(FIELD_OFFSET(NETIO_CHANGE_CALLBACK_CONTEXT, RouteCallback) == FIELD_OFFSET(NETIO_CHANGE_CALLBACK_CONTEXT, Callback));
static_assert(sizeof(((NETIO_CHANGE_REGISTRATION_HEADER*)0)->Ipv4NotificationHandle) == sizeof(PVOID));
static_assert(sizeof(((NETIO_CHANGE_REGISTRATION_HEADER*)0)->Ipv6NotificationHandle) == sizeof(PVOID));
static_assert(sizeof(((NETIO_CHANGE_REGISTRATION_HEADER*)0)->TeredoNotificationHandle) == sizeof(PVOID));
static_assert(sizeof(((NETIO_CHANGE_REGISTRATION_HEADER*)0)->NdisNotificationHandle) == sizeof(PVOID));
static_assert(sizeof(((NETIO_CHANGE_REGISTRATION_HEADER*)0)->ConnectivityHintRegistration) == sizeof(PVOID));

static_assert(FIELD_OFFSET(NETIO_CHANGE_CALLBACK_CONTEXT, Callback) == 0x8);
#ifdef _WIN64
static_assert(sizeof(NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST) == 0x28);
static_assert(FIELD_OFFSET(NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST, NotificationHandle) == 0x18);
static_assert(sizeof(NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST) == 0x48);
static_assert(FIELD_OFFSET(NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST, NotificationHandle) == 0x40);
static_assert(sizeof(NETIO_CHANGE_CALLBACK_CONTEXT) == 0x18);
static_assert(FIELD_OFFSET(NETIO_CHANGE_CALLBACK_CONTEXT, CallerContext) == 0x10);
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, Ipv4NotificationHandle) == 0x10);
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, Ipv6NotificationHandle) == 0x18);
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, TeredoNotificationHandle) == 0x20);
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, NdisNotificationHandle) == 0x28);
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, ConnectivityHintRegistration) == 0x30);
#else
static_assert(sizeof(NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST) == 0x20);
static_assert(FIELD_OFFSET(NSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST, NotificationHandle) == 0x14);
static_assert(sizeof(NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST) == 0x38);
static_assert(FIELD_OFFSET(NSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST, NotificationHandle) == 0x34);
static_assert(sizeof(NETIO_CHANGE_CALLBACK_CONTEXT) == 0x10);
static_assert(FIELD_OFFSET(NETIO_CHANGE_CALLBACK_CONTEXT, CallerContext) == 0xC);
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, Ipv4NotificationHandle) == 0xC);
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, Ipv6NotificationHandle) == 0x10);
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, TeredoNotificationHandle) == 0x14);
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, NdisNotificationHandle) == 0x18);
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, ConnectivityHintRegistration) == 0x1C);
#endif

// The object identifier retains the notification request ABI.
static_assert(sizeof(NSI_OBJECT_ID) == sizeof(ULONG));
static_assert(sizeof(NSI_TCP_APPLIED_SETTING) == sizeof(ULONG));
static_assert(sizeof(NL_NEIGHBOR_STATE) == sizeof(ULONG));
static_assert(sizeof(MIB_IPNET_TYPE) == sizeof(ULONG));
static_assert(sizeof(NTSTATUS) == sizeof(ULONG));
static_assert(sizeof(((NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST*)0)->ObjectIndex) == sizeof(ULONG));
#ifdef _WIN64
static_assert(sizeof(NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST) == 0x28);
static_assert(FIELD_OFFSET(NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST, ObjectIndex) == 0x10);
#else
static_assert(sizeof(NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST) == 0x14);
static_assert(FIELD_OFFSET(NSI_REQUEST_CHANGE_NOTIFICATION_EX_REQUEST, ObjectIndex) == 0x08);
#endif

// Native scalar types and structured views preserve the NSI field widths and layouts.
static_assert(sizeof(((NSI_IP_INTERFACE_RW*)0)->RouterDiscoveryBehavior) == sizeof(ULONG));
static_assert(sizeof(((NSI_IP_INTERFACE_RW*)0)->LinkLocalAddressBehavior) == sizeof(ULONG));
static_assert(sizeof(((NSI_TCP_GLOBAL_ROS*)0)->RtoAlgorithm) == sizeof(ULONG));
static_assert(sizeof(((ICMP_NOTIFICATION_HEADER*)0)->Status) == sizeof(ULONG));
static_assert(sizeof(((NDIS_NSI_INTERFACE_INFORMATION_ROS*)0)->MediaType) == sizeof(ULONG));
static_assert(sizeof(((NDIS_NSI_INTERFACE_INFORMATION_ROS*)0)->PhysicalMediumType) == sizeof(ULONG));
static_assert(sizeof(((NDIS_NSI_INTERFACE_ENUM_ROS*)0)->MediaType) == sizeof(ULONG));
static_assert(sizeof(((NDIS_NSI_INTERFACE_ENUM_ROS*)0)->PhysicalMediumType) == sizeof(ULONG));
static_assert(sizeof(((NETIO_NDIS_CROSSTIMESTAMP_REQUEST*)0)->Oid) == sizeof(ULONG));
static_assert(sizeof(((NETIO_NDIS_CROSSTIMESTAMP_REQUEST*)0)->Status) == sizeof(ULONG));
static_assert(sizeof(((NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST*)0)->Oid) == sizeof(ULONG));
static_assert(sizeof(((NETIO_NDIS_TIMESTAMP_CAPABILITIES_REQUEST*)0)->Status) == sizeof(ULONG));
static_assert(sizeof(((NETIO_CHANGE_REGISTRATION_HEADER*)0)->ObjectId) == sizeof(ULONG));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_KEY*)0)->IsolationMode) == sizeof(ULONG));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROD*)0)->VirtualInterfaceIndex) == sizeof(ULONG));
static_assert(sizeof(((NSI_FL_VIRTUAL_INTERFACE_ROS*)0)->VirtualInterfaceType) == sizeof(ULONG));
static_assert(sizeof(((NSI_IP_GLOBAL_RW*)0)->DefaultLinkLocalAddressBehavior) == sizeof(ULONG));
static_assert(sizeof(((NDIS_NSI_INTERFACE_ENUM_ROD*)0)->OperStatusFlagsBits) == sizeof(ULONG));
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, OperStatusFlagsBits) == FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifOperStatusFlags));
static_assert(sizeof(((NSI_IP_GLOBAL_RW*)0)->LinkLayerSuffixPrefix) == 16);
static_assert(sizeof(((NSI_IP_GLOBAL_RW*)0)->LinkLayerSuffixPrefixV4) == sizeof(ULONG));
static_assert(sizeof(((NSI_IP_GLOBAL_RW*)0)->LinkLayerSuffixPrefixAddress) == sizeof(ULONG));
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, LinkLayerSuffixPrefixV4) == FIELD_OFFSET(NSI_IP_GLOBAL_RW, LinkLayerSuffixPrefix));
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, LinkLayerSuffixPrefixAddress) == FIELD_OFFSET(NSI_IP_GLOBAL_RW, LinkLayerSuffixPrefix));

static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, RouterDiscoveryBehavior) == 0x14);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_RW, LinkLocalAddressBehavior) == 0x30);
static_assert(FIELD_OFFSET(NSI_TCP_GLOBAL_ROS, RtoAlgorithm) == 0x0);
static_assert(sizeof(ICMP_NOTIFICATION_HEADER) == 0x10);
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_HEADER, Status) == 0x0);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_INFORMATION_ROS, MediaType) == 0x250);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_INFORMATION_ROS, PhysicalMediumType) == 0x254);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, MediaType) == 0x230);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, PhysicalMediumType) == 0x234);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifOperStatusFlags) == 0x21C);
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROD, ifMtu) == 0x220);
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_KEY, IsolationMode) == 0x1C);
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROD, VirtualInterfaceIndex) == 0x0);
static_assert(FIELD_OFFSET(NSI_FL_VIRTUAL_INTERFACE_ROS, VirtualInterfaceType) == 0x8);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, DefaultLinkLocalAddressBehavior) == 0x40);
static_assert(FIELD_OFFSET(NSI_IP_GLOBAL_RW, LinkLayerSuffixPrefix) == 0x44);
#ifdef _WIN64
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, ObjectId) == 0x8);
#else
static_assert(sizeof(NETIO_CHANGE_REGISTRATION_HEADER) == 0x28);
static_assert(FIELD_OFFSET(NETIO_CHANGE_REGISTRATION_HEADER, ObjectId) == 0x4);
#endif

// Scalar and structured IPv4 address views share the same storage.
static_assert(sizeof(IPAddr) == sizeof(IN_ADDR));
static_assert(sizeof(ULONG) == sizeof(SCOPE_ID));
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V4, AddressV4) == FIELD_OFFSET(ICMP_NOTIFICATION_V4, Address));
static_assert(sizeof(((ICMP_NOTIFICATION_V4*)0)->AddressV4) == sizeof(((ICMP_NOTIFICATION_V4*)0)->Address));
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, SourceAddressV4) == FIELD_OFFSET(ICMP_REQUEST_V4, SourceAddress));
static_assert(sizeof(((ICMP_REQUEST_V4*)0)->SourceAddressV4) == sizeof(((ICMP_REQUEST_V4*)0)->SourceAddress));
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, DestinationAddressV4) == FIELD_OFFSET(ICMP_REQUEST_V4, DestinationAddress));
static_assert(sizeof(((ICMP_REQUEST_V4*)0)->DestinationAddressV4) == sizeof(((ICMP_REQUEST_V4*)0)->DestinationAddress));

// Scope IDs use the native type without changing their storage or offsets.
static_assert(sizeof(((NSI_SET_TCP_ENTRY*)0)->LocalScopeID) == sizeof(ULONG));
static_assert(sizeof(((NSI_SET_TCP_ENTRY*)0)->RemoteScopeID) == sizeof(ULONG));
static_assert(sizeof(((ICMP_NOTIFICATION_V6*)0)->ScopeId) == sizeof(ULONG));
static_assert(FIELD_OFFSET(ICMP_NOTIFICATION_V6, ScopeId) == 0x2C);
static_assert(sizeof(((ICMP_REQUEST_V4*)0)->SourceScopeId) == sizeof(ULONG));
static_assert(sizeof(((ICMP_REQUEST_V4*)0)->DestinationScopeId) == sizeof(ULONG));
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, SourceScopeId) == FIELD_OFFSET(ICMP_REQUEST_V4, OptionsSize) + sizeof(USHORT) + 2);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V4, DestinationScopeId) == FIELD_OFFSET(ICMP_REQUEST_V4, SourceScopeId) + sizeof(SCOPE_ID));
static_assert(sizeof(((ICMP_REQUEST_V6*)0)->SourceScopeId) == sizeof(ULONG));
static_assert(sizeof(((ICMP_REQUEST_V6*)0)->DestinationScopeId) == sizeof(ULONG));
static_assert(FIELD_OFFSET(ICMP_REQUEST_V6, SourceScopeId) == FIELD_OFFSET(ICMP_REQUEST_V6, OptionsSize) + sizeof(USHORT) + 2);
static_assert(FIELD_OFFSET(ICMP_REQUEST_V6, DestinationScopeId) == FIELD_OFFSET(ICMP_REQUEST_V6, SourceScopeId) + sizeof(SCOPE_ID));
static_assert(sizeof(((NSI_ESTATS_TCP_KEY*)0)->LocalScopeId) == sizeof(ULONG));
static_assert(sizeof(((NSI_ESTATS_TCP_KEY*)0)->RemoteScopeId) == sizeof(ULONG));

// Typed offload and flag views must preserve the raw NSI fields.
static_assert(sizeof(NL_INTERFACE_OFFLOAD_ROD) == sizeof(UCHAR));
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, TransmitOffload) == 0x20);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, TransmitOffloadFlags) == 0x20);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, ReceiveOffload) == 0x21);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, ReceiveOffloadFlags) == 0x21);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, Reserved2) == 0x22);
static_assert(FIELD_OFFSET(NSI_IP_INTERFACE_ROD, OutboundBandwidthInformation) == 0x40);
static_assert(sizeof(NDIS_NSI_COMPARTMENT_RW) == 0x668);
static_assert(sizeof(((NDIS_NSI_COMPARTMENT_RW*)0)->FlagsBits) == sizeof(ULONG));
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, Flags) == 0x650);
static_assert(FIELD_OFFSET(NDIS_NSI_COMPARTMENT_RW, FlagsBits) == 0x650);
static_assert(sizeof(((NDIS_NSI_INTERFACE_INFORMATION_ROS*)0)->FlagsBits) == sizeof(ULONG));
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_INFORMATION_ROS, FlagsBits) == 0x24C);
static_assert(sizeof(((NDIS_NSI_INTERFACE_ENUM_ROS*)0)->FlagsBits) == sizeof(ULONG));
static_assert(FIELD_OFFSET(NDIS_NSI_INTERFACE_ENUM_ROS, FlagsBits) == 0x22C);


// Socket-address views must preserve the NSI endpoint layouts.
static_assert(sizeof(NSI_SET_TCP_ENTRY) == 56);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalFamily) == 0);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalPort) == 2);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalAddressIPv4) == 4);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalAddressIPv6) == 8);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalScopeID) == 24);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteFamily) == 28);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemotePort) == 30);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteAddressIPv4) == 32);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteAddressIPv6) == 36);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteScopeID) == 52);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalSockaddr) == 0);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalTcpEntryIpV4) == 0);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, LocalTcpEntryIpV6) == 0);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteSockaddr) == 28);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteTcpEntryIpV4) == 28);
static_assert(FIELD_OFFSET(NSI_SET_TCP_ENTRY, RemoteTcpEntryIpV6) == 28);

static_assert(sizeof(NSI_ESTATS_TCP_KEY) == 56);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, AddressFamily) == 0);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalPort) == 2);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalAddressIPv4) == 4);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalAddressIPv6) == 8);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalScopeId) == 24);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteAddressFamily) == 28);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemotePort) == 30);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteAddressIPv4) == 32);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteAddressIPv6) == 36);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteScopeId) == 52);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalSockaddr) == 0);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalSockaddrV4) == 0);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, LocalSockaddrV6) == 0);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteSockaddr) == 28);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteSockaddrV4) == 28);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY, RemoteSockaddrV6) == 28);

static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, LocalAddressV4) == 4);
static_assert(sizeof(((NSI_ESTATS_TCP_KEY_V4*)0)->LocalAddressV4) == sizeof(((NSI_ESTATS_TCP_KEY_V4*)0)->LocalAddress));
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, RemoteAddressV4) == 32);
static_assert(sizeof(((NSI_ESTATS_TCP_KEY_V4*)0)->RemoteAddressV4) == sizeof(((NSI_ESTATS_TCP_KEY_V4*)0)->RemoteAddress));
static_assert(sizeof(NSI_ESTATS_TCP_KEY_V4) == 56);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, AddressFamily) == 0);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, LocalPort) == 2);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, LocalAddress) == 4);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, Reserved08) == 8);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, RemoteAddressFamily) == 28);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, RemotePort) == 30);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, RemoteAddress) == 32);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, Reserved24) == 36);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, LocalSockaddrV4) == 0);
static_assert(FIELD_OFFSET(NSI_ESTATS_TCP_KEY_V4, RemoteSockaddrV4) == 28);

static_assert(sizeof(NSI_TCP_KEY_ROW) == 56);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalFamily) == 0);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalPort) == 2);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalAddressV4) == 4);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalAddressV6) == 8);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalScopeId) == 24);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteFamily) == 28);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemotePort) == 30);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteAddressV4) == 32);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteAddressV6) == 36);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteScopeId) == 52);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalAddress) == 0);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteAddress) == 28);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalSockaddrV4) == 0);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, LocalSockaddrV6) == 0);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteSockaddrV4) == 28);
static_assert(FIELD_OFFSET(NSI_TCP_KEY_ROW, RemoteSockaddrV6) == 28);

static_assert(sizeof(NSI_UDP_KEY_ROW) == 28);
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, AddressFamily) == 0);
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalPort) == 2);
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalAddressV4) == 4);
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalAddressV6) == 8);
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalScopeId) == 24);
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalSockaddr) == 0);
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalSockaddrV4) == 0);
static_assert(FIELD_OFFSET(NSI_UDP_KEY_ROW, LocalSockaddrV6) == 0);

static_assert(sizeof(NSI_UDP_STATUS_ROW) == 32);
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteFamily) == 0);
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemotePort) == 2);
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteAddressV4) == 4);
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteAddressV6) == 8);
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteScopeId) == 24);
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteSockaddrV4) == 0);
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteSockaddrV6) == 0);

static_assert(sizeof(TCP_KEY_ROW) == 56);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, Family) == 0);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, LocalPort) == 2);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, LocalAddressV4) == 4);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, LocalAddress) == 8);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, LocalScopeId) == 24);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemoteFamily) == 28);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemotePort) == 30);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemoteAddressV4) == 32);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemoteAddress) == 36);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemoteScopeId) == 52);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, LocalSockaddr) == 0);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, LocalSockaddrV4) == 0);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, LocalSockaddrV6) == 0);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemoteSockaddr) == 28);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemoteSockaddrV4) == 28);
static_assert(FIELD_OFFSET(TCP_KEY_ROW, RemoteSockaddrV6) == 28);

static_assert(sizeof(UDP_KEY_ROW) == 28);
static_assert(FIELD_OFFSET(UDP_KEY_ROW, Family) == 0);
static_assert(FIELD_OFFSET(UDP_KEY_ROW, Port) == 2);
static_assert(FIELD_OFFSET(UDP_KEY_ROW, AddressV4) == 4);
static_assert(FIELD_OFFSET(UDP_KEY_ROW, Address) == 8);
static_assert(FIELD_OFFSET(UDP_KEY_ROW, ScopeId) == 24);
static_assert(FIELD_OFFSET(UDP_KEY_ROW, LocalSockaddr) == 0);
static_assert(FIELD_OFFSET(UDP_KEY_ROW, LocalSockaddrV4) == 0);
static_assert(FIELD_OFFSET(UDP_KEY_ROW, LocalSockaddrV6) == 0);

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
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, RemoteAddress) == 0);
static_assert(FIELD_OFFSET(NSI_UDP_STATUS_ROW, TrafficClass) == 28);
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
    );

/**
 * The NsiAllocateAndGetPersistentDataWithMaskTable routine enumerates the persistent store of an NSI object and returns
 * heap-allocated key, data and mask tables (one row per persisted instance). Used to read back configuration that has been
 * written with NsiSetAllPersistentParametersWithMask, e.g. Persisted interface or route settings.
 *
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object to query.
 * \param[out] KeyTable Optional; receives the allocated key table (*Count rows of KeyEntrySize bytes). Not allocated when KeyEntrySize is 0.
 * \param[in] KeyEntrySize The size of each key entry in bytes.
 * \param[out] ParameterDataTable Optional; receives the allocated parameter data table (*Count rows of ParameterDataEntrySize bytes).
 * \param[out] ParameterMaskTable Optional; receives the allocated mask table. It has the same row size as the data table.
 * \param[in] ParameterDataEntrySize The size of each data row and each mask row in bytes. Neither table is allocated when 0.
 * \param[out] Count Receives the number of rows returned.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks Free the tables with NsiFreePersistentDataWithMaskTable. The routine sizes the tables with a count query and retries
 * up to three times while the store keeps growing (ERROR_MORE_DATA). 
 */
NTSYSAPI
ULONG
NTAPI
NsiAllocateAndGetPersistentDataWithMaskTable(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * KeyEntrySize) PVOID* KeyTable,
    _In_ ULONG KeyEntrySize,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * ParameterDataEntrySize) PVOID* ParameterDataTable,
    _Outptr_opt_result_bytebuffer_maybenull_(*Count * ParameterDataEntrySize) PVOID* ParameterMaskTable,
    _In_ ULONG ParameterDataEntrySize,
    _Out_ PULONG Count
    );

/**
 * The NsiDeregisterChangeNotification routine unregisters an NSI change notification callback.
 *
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] NotificationHandle Optional handle to the notification registration to deregister.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiDeregisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
    );

/**
 * The NsiCancelChangeNotification routine cancels a pending NSI change notification request.
 *
 * \param[in] NotificationHandle Optional handle to the change notification to cancel.
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
 * \param[in,out] Request A pointer to the extended deregistration request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiDeregisterChangeNotificationEx(
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    );

/**
 * The NsiEnumerateObjectsAllParameters routine enumerates all objects and parameters for an NSI module object.
 *
 * \param[in] Store The NSI store to query (active or persistent).
 * \param[in] Action NsiGetFirst to enumerate from the beginning (what NsiAllocateAndGetTable and IP Helper pass) or NsiGetNext to
 *        continue after the key in KeyTable[0]. netio.sys rejects any other value with STATUS_INVALID_PARAMETER.
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object to enumerate.
 * \param[in,out] KeyTable Optional buffer receiving the array of key entries.
 * \param[in] KeyEntrySize The size of each key entry in bytes.
 * \param[in,out] ReadWriteTable Optional buffer receiving read-write entries.
 * \param[in] ReadWriteEntrySize The size of each read-write entry in bytes.
 * \param[in,out] DynamicTable Optional buffer receiving dynamic read-only entries.
 * \param[in] DynamicEntrySize The size of each dynamic read-only entry in bytes.
 * \param[in,out] StaticTable Optional buffer receiving static read-only entries.
 * \param[in] StaticEntrySize The size of each static read-only entry in bytes.
 * \param[in,out] Count A pointer specifying capacity and receiving the number of entries returned.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiEnumerateObjectsAllParameters(
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

/**
 * The NsiEnumerateObjectsAllParametersEx routine enumerates all objects and parameters using an extended request structure.
 *
 * \param[in,out] Request A pointer to the extended enumeration request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiEnumerateObjectsAllParametersEx(
    _Inout_ PNSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX Request
    );

/**
 * The NsiEnumerateObjectsAllPersistentParametersWithMask routine enumerates the persistent store of an NSI object into
 * caller-supplied key, data and mask tables (IOCTL 0x120047). Pass NULL tables to obtain the row count only.
 *
 * \param[in] Action Must be NsiGetFirst (the value NsiAllocateAndGetPersistentDataWithMaskTable passes). nsiproxy.sys forwards it
 *        to the same netio.sys check that validates NSI_ENUMERATE_OBJECTS_ALL_PARAMETERS_EX.Action for persistent enumeration,
 *        which fails anything else with STATUS_NOT_IMPLEMENTED.
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object to enumerate.
 * \param[out] KeyTable Optional buffer receiving *Count key entries.
 * \param[in] KeyEntrySize The size of each key entry in bytes.
 * \param[out] ParameterDataTable Optional buffer receiving *Count parameter data rows.
 * \param[out] ParameterMaskTable Optional buffer receiving *Count parameter mask rows.
 * \param[in] ParameterDataEntrySize The size of each data row and each mask row in bytes.
 * \param[in,out] Count On input, the capacity of the tables in rows; on output, the number of rows (also on ERROR_MORE_DATA).
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiEnumerateObjectsAllPersistentParametersWithMask(
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

/**
 * The NsiGetParameter routine reads one field of one NSI object instance, e.g. The interface index for an interface LUID
 * (ConvertInterfaceLuidToIndex) or the alias of an interface (ConvertInterfaceLuidToAlias).
 *
 * \param[in] Store The NSI store to query.
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] Key A pointer to the key identifying the object instance.
 * \param[in] KeySize The size of the key in bytes.
 * \param[in] ParameterType The row that contains the field (read-write, read-only dynamic or read-only static).
 * \param[out] ParameterBuffer Optional buffer receiving the field value.
 * \param[in] ParameterBufferSize The size of ParameterBuffer in bytes (the number of bytes copied from the row).
 * \param[in] ParameterOffset The byte offset of the field within the ParameterType row (e.g. FIELD_OFFSET of the row struct).
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
    _Out_writes_bytes_opt_(ParameterBufferSize) PVOID ParameterBuffer,
    _In_ ULONG ParameterBufferSize,
    _In_ ULONG ParameterOffset
    );

/**
 * The NsiGetParameterEx routine retrieves a single parameter using an extended request structure.
 *
 * \param[in,out] Request A pointer to the extended parameter retrieval request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiGetParameterEx(
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST Request
    );

/**
 * The NsiGetAllParameters routine retrieves all parameters for a single NSI object instance.
 *
 * \param[in] Store The NSI store to query.
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] Key A pointer to the key identifying the object instance.
 * \param[in] KeySize The size of the key in bytes.
 * \param[out] ReadWriteBuffer Optional buffer receiving read-write parameters.
 * \param[in] ReadWriteBufferSize The size of the read-write buffer in bytes.
 * \param[out] DynamicBuffer Optional buffer receiving dynamic read-only parameters.
 * \param[in] DynamicBufferSize The size of the dynamic read-only buffer in bytes.
 * \param[out] StaticBuffer Optional buffer receiving static read-only parameters.
 * \param[in] StaticBufferSize The size of the static read-only buffer in bytes.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiGetAllParameters(
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

/**
 * The NsiGetAllParametersEx routine retrieves all parameters using an extended request structure.
 *
 * \param[in,out] Request A pointer to the extended request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiGetAllParametersEx(
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST Request
    );

/**
 * The NsiSetParameter routine writes one field of one NSI object instance, e.g. The administrative status of
 * an interface (SetIfEntry) or the extended TCP statistics collection switches of a connection
 * (SetPerTcpConnectionEStats).
 *
 * \param[in] Store The NSI store to update.
 * \param[in] Action The set action (NSI_SET_ACTION): NsiSetDefault to update an existing instance,
 * NsiSetCreateOnly, NsiSetCreateOrSet, NsiSetDelete, NsiSetReset (flush), NsiSetClear, or the
 * reference-counted NsiSetCreateOrSetWithReference/NsiSetDeleteWithReference (used by
 * EnableRouter/UnenableRouter).
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] Key A pointer to the key identifying the object instance.
 * \param[in] KeySize The size of the key in bytes.
 * \param[in] ParameterType The row that contains the field (normally NsiParameterTypeReadWrite).
 * \param[in] ParameterBuffer Optional buffer containing the new field value.
 * \param[in] ParameterBufferSize The size of ParameterBuffer in bytes.
 * \param[in] ParameterOffset The byte offset of the field within the ParameterType row.
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
 * \param[in,out] Request A pointer to the extended parameter set request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiSetParameterEx(
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST Request
    );

/**
 * The NsiSetAllParameters routine creates, updates, deletes or flushes an NSI object instance by writing its whole read-write
 * row. This is how iphlpapi implements CreateIpForwardEntry2/DeleteIpForwardEntry2, CreateIpNetEntry/DeleteIpNetEntry,
 * AddIPAddress/DeleteIPAddress, SetIpInterfaceEntry, FlushIpNetTable and SetTcpEntry (closing a TCP connection).
 *
 * \param[in] Store The NSI store to update (NsiPersistent, NsiActive or NsiBoth).
 * \param[in] Action The set action (NSI_SET_ACTION): NsiSetDefault to update an existing instance, NsiSetCreateOnly,
 *        NsiSetCreateOrSet, NsiSetDelete, NsiSetReset (flush), NsiSetClear, or the reference-counted
 *        NsiSetCreateOrSetWithReference/NsiSetDeleteWithReference (used by EnableRouter/UnenableRouter).
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] Key Optional pointer to the key identifying the object instance (e.g. The interface LUID, or LUID + address).
 * \param[in] KeySize The size of the key in bytes.
 * \param[in] ReadWriteBuffer Optional read-write row to write. Usually NULL for NsiSetDelete and NsiSetReset.
 * \param[in] ReadWriteBufferSize The size of ReadWriteBuffer in bytes.
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
    _In_reads_bytes_opt_(ReadWriteBufferSize) PVOID ReadWriteBuffer,
    _In_ ULONG ReadWriteBufferSize
    );

/**
 * The NsiSetAllParametersEx routine sets all parameters using an extended request structure.
 *
 * \param[in,out] Request A pointer to the extended request structure.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiSetAllParametersEx(
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST Request
    );

/**
 * The NsiRegisterChangeNotification routine registers a callback to receive NSI change notifications.
 *
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object to observe.
 * \param[in] ChangeCallback Must be NULL in user mode: nsi.dll fails the request with ERROR_INVALID_PARAMETER when a callback is
 *        supplied (callbacks are only honoured for kernel-mode NSI clients). User-mode callers should use
 *        NsiRpcRegisterChangeNotification or NsiRequestChangeNotification instead.
 * \param[in] NotifyCurrent TRUE to notify immediately of current state; FALSE otherwise.
 * \param[in] CallerContext Optional context value passed to the callback function.
 * \param[out] NotificationHandle Receives a handle to the change notification registration.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiRegisterChangeNotification(
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ PNSI_CHANGE_CALLBACK ChangeCallback,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Outptr_result_maybenull_ PHANDLE NotificationHandle
    );

/**
 * The NsiRegisterChangeNotificationEx routine registers an extended change notification (IOCTL 0x12001F, 0x48-byte request).
 *
 * \param[in,out] Request A pointer to the extended registration request structure. ChangeCallback must be NULL in user
 *        mode (ERROR_INVALID_PARAMETER otherwise).
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiRegisterChangeNotificationEx(
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    );

/**
 * The NsiRequestChangeNotification routine requests asynchronous change notifications for an NSI object.
 *
 * \param[in] ModuleHandle The NSI module handle, or 0 to identify the module by ModuleId (IP Helper always passes 0).
 *        This is not a flags value: nsiproxy.sys copies it into the module identity of the kernel registration, and netio.sys
 *        resolves a non-zero value through its module handle table (the value NsiGetModuleHandle returns in kernel mode).
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The object index to monitor.
 * \param[in] Overlapped Optional pointer to an OVERLAPPED structure for asynchronous completion.
 * \param[out] NotificationHandle Receives a notification handle.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiRequestChangeNotification(
    _In_ ULONG ModuleHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ LPOVERLAPPED Overlapped,
    _Out_ PHANDLE NotificationHandle
    );

/**
 * The NsiRequestChangeNotificationEx routine requests an asynchronous change notification for an NSI object
 * (IOCTL 0x12003F). The request completes through Request->Overlapped when the object changes, which lets user-mode
 * callers wait for changes without a callback.
 *
 * \param[in,out] Request A pointer to the notification request (0x28 bytes). When Request->Overlapped is not NULL and
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
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] Key A pointer to the key identifying the object instance.
 * \param[in] KeySize The size of the key in bytes.
 * \param[out] ParameterData Optional buffer receiving the persisted row.
 * \param[out] ParameterMask Optional buffer receiving the field mask (same size as ParameterData; 0xFF bytes mark
 * persisted fields).
 * \param[in] ParameterDataSize The size of ParameterData and of ParameterMask in bytes.
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
 * \param[in] Action The set action (NSI_SET_ACTION): NsiSetDefault to update an existing instance, NsiSetCreateOnly,
 *        NsiSetCreateOrSet, NsiSetDelete, NsiSetReset (flush), NsiSetClear, or the reference-counted
 *        NsiSetCreateOrSetWithReference/NsiSetDeleteWithReference (used by EnableRouter/UnenableRouter).
 *        netio.sys validates the value (NsiSetReset and NsiSetClear are handled specially); iphlpapi passes NsiSetCreateOrSet.
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] Key A pointer to the key identifying the object instance.
 * \param[in] KeySize The size of the key in bytes.
 * \param[in] ParameterData Optional buffer containing the row to persist.
 * \param[in] ParameterMask Optional field mask (same size as ParameterData; 0xFF bytes select the fields to persist).
 * \param[in] ParameterDataSize The size of ParameterData and of ParameterMask in bytes.
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
 * The NsiGetObjectSecurity routine queries an NSI information object's security descriptor through IOCTL 0x120037.
 *
 * \param[in,out] Request The input/output security request.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiGetObjectSecurity(
    _Inout_ PNSI_OBJECT_SECURITY_REQUEST Request
    );

/**
 * The NsiSetObjectSecurity routine changes an NSI information object's security descriptor through IOCTL 0x12003B.
 *
 * \param[in,out] Request The input/output security request.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
NTAPI
NsiSetObjectSecurity(
    _Inout_ PNSI_OBJECT_SECURITY_REQUEST Request
    );

/**
 * The NsiConnectToServer routine creates and configures a local ncalrpc binding for NSI RPC calls. A
 * non-NULL Reserved argument returns NULL before binding creation without setting a new last error.
 * Binding-creation failures do set last error. Release successful bindings with NsiDisconnectFromServer.
 * Binding creation alone does not query an NSI table.
 *
 * \param[in] Reserved Must be NULL.
 * \return An RPC binding handle, or NULL on failure.
 */
NTSYSAPI
RPC_BINDING_HANDLE
WINAPI
NsiConnectToServer(
    _Reserved_ PVOID Reserved
    );

/**
 * The NsiDisconnectFromServer routine releases the RPC binding value through RpcBindingFree using a
 * temporary local slot. The caller variable is not cleared; discard its value after successful cleanup. This
 * releases a binding, not the remote server.
 *
 * \param[in] RpcHandle Optional RPC binding handle to release.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiDisconnectFromServer(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle
    );

/**
 * The NsiFreePersistentDataWithMaskTable routine frees the key, data and mask allocations from the
 * persistent-table allocator using the NSI heap. Each pointer is optional. Pass allocation values, not
 * output-slot addresses. Caller variables are not cleared, and there is no status result.
 *
 * \param[in] KeyTable Optional key-table allocation to free.
 * \param[in] ParameterDataTable Optional parameter-data allocation to free.
 * \param[in] ParameterMaskTable Optional parameter-mask allocation to free.
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
 * The NsiFreeTable routine frees up to four independent NSI table allocations using the NSI heap. NULL
 * entries are skipped. Pass allocation values returned by NsiAllocateAndGetTable, not output-slot addresses.
 * Caller variables are not cleared, and there is no status result. Repeated allocation values would cause
 * repeated frees.
 *
 * \param[in] KeyTable Optional key-table allocation to free.
 * \param[in] ReadWriteTable Optional read/write table allocation to free.
 * \param[in] DynamicTable Optional read-only dynamic table allocation to free.
 * \param[in] StaticTable Optional read-only static table allocation to free.
 */
NTSYSAPI
VOID
NTAPI
NsiFreeTable(
    _Frees_ptr_opt_ PVOID KeyTable,
    _Frees_ptr_opt_ PVOID ReadWriteTable,
    _Frees_ptr_opt_ PVOID DynamicTable,
    _Frees_ptr_opt_ PVOID StaticTable
    );

/**
 * The NsiRpcDeregisterChangeNotification routine unregisters an NSI change notification that was registered over RPC.
 *
 * \param[in] RpcHandle The RPC binding handle used for the registration (required; NULL fails with ERROR_INVALID_PARAMETER).
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] NotificationHandle The handle returned by NsiRpcRegisterChangeNotification.
 * \return A Win32 error code. ERROR_SUCCESS on success; ERROR_INVALID_PARAMETER if the handle is unknown.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcDeregisterChangeNotification(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ HANDLE NotificationHandle
    );

/**
 * The NsiRpcDeregisterChangeNotificationEx routine unregisters a change notification that was registered over RPC.
 *
 * \param[in] RpcHandle The RPC binding handle used for the registration (required; NULL fails with ERROR_INVALID_PARAMETER).
 * \param[in,out] Request The deregistration request; NotificationHandle is the handle returned at registration.
 * \return A Win32 error code. ERROR_SUCCESS on success; ERROR_INVALID_PARAMETER if the handle is unknown.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcDeregisterChangeNotificationEx(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_DEREGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    );

/**
 * The NsiRpcEnumerateObjectsAllParameters routine enumerates all instances of an NSI object, locally or through the NSI RPC
 * server. Same parameters and results as NsiEnumerateObjectsAllParameters.
 *
 * \param[in] RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally
 *        through the matching nsi.dll routine (no RPC).
 * \param[in] Store The NSI store to query. NsiBoth is rejected over RPC (ERROR_INVALID_PARAMETER).
 * \param[in] Action NsiGetFirst to enumerate from the beginning (what NsiAllocateAndGetTable and IP Helper pass) or NsiGetNext to
 *        continue after the key in KeyTable[0]. netio.sys rejects any other value with STATUS_INVALID_PARAMETER.
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object to enumerate.
 * \param[in,out] KeyTable Optional buffer receiving key entries.
 * \param[in] KeyEntrySize The size of each key entry in bytes.
 * \param[in,out] ReadWriteTable Optional buffer receiving read-write entries.
 * \param[in] ReadWriteEntrySize The size of each read-write entry in bytes.
 * \param[in,out] DynamicTable Optional buffer receiving dynamic read-only entries.
 * \param[in] DynamicEntrySize The size of each dynamic read-only entry in bytes.
 * \param[in,out] StaticTable Optional buffer receiving static read-only entries.
 * \param[in] StaticEntrySize The size of each static read-only entry in bytes.
 * \param[in,out] Count A pointer specifying capacity and receiving the number of entries returned.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcEnumerateObjectsAllParameters(
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

/**
 * The NsiRpcGetAllParameters routine retrieves all parameters of one NSI object instance, locally or through the NSI RPC
 * server. Same parameters and results as NsiGetAllParameters.
 *
 * \param[in] RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally through the matching nsi.dll routine (no RPC).
 * \param[in] Store The NSI store to query. NsiBoth is rejected over RPC (ERROR_INVALID_PARAMETER).
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] Key A pointer to the key identifying the object instance.
 * \param[in] KeySize The size of the key in bytes.
 * \param[out] ReadWriteBuffer Optional buffer receiving read-write parameters.
 * \param[in] ReadWriteBufferSize The size of the read-write buffer in bytes.
 * \param[out] DynamicBuffer Optional buffer receiving dynamic read-only parameters.
 * \param[in] DynamicBufferSize The size of the dynamic read-only buffer in bytes.
 * \param[out] StaticBuffer Optional buffer receiving static read-only parameters.
 * \param[in] StaticBufferSize The size of the static read-only buffer in bytes.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcGetAllParameters(
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

/**
 * The NsiRpcGetAllParametersEx routine is the request-structure form of NsiRpcGetAllParameters.
 *
 * \param[in] RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally through the matching nsi.dll routine (no RPC).
 * \param[in,out] Request A pointer to the request (passed to NsiGetAllParametersEx when RpcHandle is NULL).
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks Over RPC the request must have MustBeZero == 0 (else ERROR_CALL_NOT_IMPLEMENTED) and a ModuleHandle or ModuleId (else ERROR_INVALID_PARAMETER).
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcGetAllParametersEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_GET_ALL_PARAMETERS_EX_REQUEST Request
    );

/**
 * The NsiRpcGetParameter routine reads one field of one NSI object instance, locally or through the NSI RPC server.
 * Same parameters and results as NsiGetParameter.
 *
 * \param[in] RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally through the matching nsi.dll routine (no RPC).
 * \param[in] Store The NSI store to query. NsiBoth is rejected over RPC (ERROR_INVALID_PARAMETER).
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] Key A pointer to the key identifying the object instance.
 * \param[in] KeySize The size of the key in bytes.
 * \param[in] ParameterType The row that contains the field.
 * \param[out] ParameterBuffer Optional buffer receiving the field value.
 * \param[in] ParameterBufferSize The size of ParameterBuffer in bytes.
 * \param[in] ParameterOffset The byte offset of the field within the ParameterType row.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcGetParameter(
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

/**
 * The NsiRpcGetParameterEx routine is the request-structure form of NsiRpcGetParameter.
 *
 * \param[in] RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally through the matching nsi.dll routine (no RPC).
 * \param[in,out] Request A pointer to the request (passed to NsiGetParameterEx when RpcHandle is NULL).
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks Over RPC the request must have MustBeZero == 0 (else ERROR_CALL_NOT_IMPLEMENTED) and a ModuleHandle or ModuleId (else ERROR_INVALID_PARAMETER).
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcGetParameterEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_GET_PARAMETER_EX_REQUEST Request
    );

/**
 * The NsiRpcRegisterChangeNotification routine registers a user-mode callback for NSI change notifications. Unlike
 * NsiRegisterChangeNotification it accepts a callback, because the notification is delivered by the NSI RPC server and
 * dispatched in the caller's process by winnsi.dll (used by System Informer for ICMP echo/ping notifications).
 *
 * \param[in] RpcHandle An RPC binding handle from NsiConnectToServer (required; NULL fails with ERROR_INVALID_PARAMETER).
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object to observe (for ICMP, an NSI_ICMP_OBJECT_ID value).
 * \param[in] ChangeCallback Optional pointer to the callback routine.
 * \param[in] NotifyCurrent TRUE to notify immediately of current state; FALSE otherwise.
 * \param[in] CallerContext Optional context value passed to the callback routine.
 * \param[out] NotificationHandle Optional pointer receiving a notification handle for NsiRpcDeregisterChangeNotification.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcRegisterChangeNotification(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _In_ PNPI_MODULEID ModuleId,
    _In_ NSI_OBJECT_ID ObjectId,
    _In_opt_ PNSI_CHANGE_CALLBACK ChangeCallback,
    _In_ BOOLEAN NotifyCurrent,
    _In_opt_ PVOID CallerContext,
    _Out_opt_ PHANDLE NotificationHandle
    );

/**
 * The NsiRpcRegisterChangeNotificationEx routine is the request-structure form of NsiRpcRegisterChangeNotification.
 *
 * \param[in] RpcHandle An RPC binding handle from NsiConnectToServer (required; NULL fails with ERROR_INVALID_PARAMETER).
 * \param[in,out] Request The registration request. When both ScopeOrCompartment fields are 0, winnsi.dll fills them in by querying the NDIS module (object 8) before registering.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcRegisterChangeNotificationEx(
    _In_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_REGISTER_CHANGE_NOTIFICATION_EX_REQUEST Request
    );

/**
 * The NsiRpcSetAllParameters routine creates, updates, deletes or flushes an NSI object instance, locally or through the NSI
 * RPC server. Same parameters and results as NsiSetAllParameters.
 *
 * \param[in] RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally through the matching nsi.dll routine (no RPC).
 * \param[in] Store The NSI store to update. NsiCurrent is rejected over RPC (ERROR_INVALID_PARAMETER).
 * \param[in] Action The set action (NSI_SET_ACTION); see NsiSetAllParameters.
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] Key Optional pointer to the key identifying the object instance.
 * \param[in] KeySize The size of the key in bytes.
 * \param[in] ReadWriteBuffer Optional read-write row to write.
 * \param[in] ReadWriteBufferSize The size of ReadWriteBuffer in bytes.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcSetAllParameters(
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

/**
 * The NsiRpcSetAllParametersEx routine is the request-structure form of NsiRpcSetAllParameters.
 *
 * \param[in] RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally through the matching nsi.dll routine (no RPC).
 * \param[in,out] Request A pointer to the request (passed to NsiSetAllParametersEx when RpcHandle is NULL). Store NsiCurrent is rejected over RPC.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks Over RPC the request must have MustBeZero == 0 (else ERROR_CALL_NOT_IMPLEMENTED) and a ModuleHandle or ModuleId (else ERROR_INVALID_PARAMETER).
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcSetAllParametersEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_SET_ALL_PARAMETERS_EX_REQUEST Request
    );

/**
 * The NsiRpcSetParameter routine writes one field of one NSI object instance, locally or through the NSI RPC server.
 * Same parameters and results as NsiSetParameter.
 *
 * \param[in] RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally through the matching nsi.dll routine (no RPC).
 * \param[in] Store The NSI store to update. NsiCurrent is rejected over RPC (ERROR_INVALID_PARAMETER).
 * \param[in] Action The set action (NSI_SET_ACTION); see NsiSetParameter.
 * \param[in] ModuleId A pointer to the NPI module identifier.
 * \param[in] ObjectId The identifier of the NSI object.
 * \param[in] Key A pointer to the key identifying the object instance.
 * \param[in] KeySize The size of the key in bytes.
 * \param[in] ParameterType The row that contains the field.
 * \param[in] ParameterBuffer Optional buffer containing the new field value.
 * \param[in] ParameterBufferSize The size of ParameterBuffer in bytes.
 * \param[in] ParameterOffset The byte offset of the field within the ParameterType row.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcSetParameter(
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

/**
 * The NsiRpcSetParameterEx routine is the request-structure form of NsiRpcSetParameter.
 *
 * \param[in] RpcHandle Optional RPC binding handle from NsiConnectToServer. NULL performs the request locally through the matching nsi.dll routine (no RPC).
 * \param[in,out] Request A pointer to the request (passed to NsiSetParameterEx when RpcHandle is NULL). Store NsiCurrent is rejected over RPC.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 * \remarks Over RPC the request must have MustBeZero == 0 (else ERROR_CALL_NOT_IMPLEMENTED) and a ModuleHandle or ModuleId (else ERROR_INVALID_PARAMETER).
 */
NTSYSAPI
ULONG
WINAPI
NsiRpcSetParameterEx(
    _In_opt_ RPC_BINDING_HANDLE RpcHandle,
    _Inout_ PNSI_SET_PARAMETER_EX_REQUEST Request
    );

#endif
