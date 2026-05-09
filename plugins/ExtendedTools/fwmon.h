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

#ifndef FWMON_H
#define FWMON_H

/* =========================================================================
 * FWPM_NET_EVENT_DATA_TYPE
 *
 * Selects which event data set FwpmNetEventCreateEnumHandleEx opens.
 * Passed as RDX (zero-extended byte) at the call site.
 *   0  →  standard WFP classification/drop events  ("netevents.xml")
 *   1  →  IKE/AuthIP main-mode & quick-mode events ("ikeevents.xml")
 * =========================================================================*/
typedef enum FWPM_NET_EVENT_DATA_TYPE_
{
    FWPM_NET_EVENT_DATA_WFP = 0,
    FWPM_NET_EVENT_DATA_IKE = 1,
} FWPM_NET_EVENT_DATA_TYPE;

/* =========================================================================
 * FWPM_NET_EVENT_INTERNAL_TYPE
 * Types 0-10 are the public set; 11 = PUBLIC_MAX sentinel;
 * 12 = INTERNAL_MIN sentinel; 13-20 are IKE/AuthIP-internal only (returned
 * when dataType == FWPM_NET_EVENT_DATA_IKE).
 * =========================================================================*/
typedef enum FWPM_NET_EVENT_INTERNAL_TYPE_
{
    /* ---- public types (mirror FWPM_NET_EVENT_TYPE / public SDK) ---- */
    FWPM_NET_EVENT_TYPE_PUBLIC_IKEEXT_MM_FAILURE    = 0,
    FWPM_NET_EVENT_TYPE_PUBLIC_IKEEXT_QM_FAILURE    = 1,
    FWPM_NET_EVENT_TYPE_PUBLIC_IKEEXT_EM_FAILURE    = 2,
    FWPM_NET_EVENT_TYPE_PUBLIC_CLASSIFY_DROP        = 3,
    FWPM_NET_EVENT_TYPE_PUBLIC_IPSEC_KERNEL_DROP    = 4,
    FWPM_NET_EVENT_TYPE_PUBLIC_IPSEC_DOSP_DROP      = 5,
    FWPM_NET_EVENT_TYPE_PUBLIC_CLASSIFY_ALLOW       = 6,
    FWPM_NET_EVENT_TYPE_PUBLIC_CAPABILITY_DROP      = 7,
    FWPM_NET_EVENT_TYPE_PUBLIC_CAPABILITY_ALLOW     = 8,
    FWPM_NET_EVENT_TYPE_PUBLIC_CLASSIFY_DROP_MAC    = 9,
    FWPM_NET_EVENT_TYPE_PUBLIC_LPM_PACKET_ARRIVAL   = 10,
    FWPM_NET_EVENT_TYPE_PUBLIC_MAX                  = 11,  /* sentinel */

    /* ---- internal / IKE types (dataType == FWPM_NET_EVENT_DATA_IKE) ---- */
    FWPM_NET_EVENT_TYPE_INTERNAL_MIN                    = 12,  /* sentinel */
    FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_MM_ESTABLISHED  = 13,
    FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_MM_DELETED      = 14,
    FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_MM_FAILURE      = 15,
    FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_QM_ESTABLISHED  = 16,
    FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_QM_DELETED      = 17,
    FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_QM_FAILURE      = 18,
    FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_NO_QM_FOR_MMSA  = 19,
    FWPM_NET_EVENT_TYPE_INTERNAL_IKEEXT_POLICY_CHANGE   = 20,
} FWPM_NET_EVENT_INTERNAL_TYPE;

typedef enum FWPM_NET_EVENT_INTERNAL_MM_NEGOTIATE_REASON_
{
    FWPM_NET_EVENT_INTERNAL_IKE_MM_INITIATOR        = 0,
    FWPM_NET_EVENT_INTERNAL_IKE_MM_RESPONDER        = 1,
    FWPM_NET_EVENT_INTERNAL_IKE_MM_DRIVER_INITIATED = 2,
    FWPM_NET_EVENT_INTERNAL_IKE_MM_REKEY_INITIATOR  = 3,
    FWPM_NET_EVENT_INTERNAL_IKE_MM_REKEY_RESPONDER  = 4,
} FWPM_NET_EVENT_INTERNAL_MM_NEGOTIATE_REASON;

typedef enum FWPM_NET_EVENT_INTERNAL_QM_NEGOTIATE_REASON_
{
    FWPM_NET_EVENT_INTERNAL_IKE_QM_INITIATOR   = 0,
    FWPM_NET_EVENT_INTERNAL_IKE_QM_RESPONDER   = 1,
    FWPM_NET_EVENT_INTERNAL_IKE_QM_DEMAND_DIAL = 2,
} FWPM_NET_EVENT_INTERNAL_QM_NEGOTIATE_REASON;

typedef enum FWPM_NET_EVENT_INTERNAL_IKE_POLICY_CHANGE_TYPE_
{
    FWPM_NET_EVENT_INTERNAL_IKE_POLICY_CHANGE_TYPE_MAIN_MODE  = 0,
    FWPM_NET_EVENT_INTERNAL_IKE_POLICY_CHANGE_TYPE_QUICK_MODE = 1,
} FWPM_NET_EVENT_INTERNAL_IKE_POLICY_CHANGE_TYPE;

//typedef enum FWPM_APPC_NETWORK_CAPABILITY_TYPE_
//{
//    FWPM_APPC_NETWORK_CAPABILITY_INTERNET_CLIENT          = 0,
//    FWPM_APPC_NETWORK_CAPABILITY_INTERNET_CLIENT_SERVER   = 1,
//    FWPM_APPC_NETWORK_CAPABILITY_INTERNET_PRIVATE_NETWORK = 2,
//} FWPM_APPC_NETWORK_CAPABILITY_TYPE;

typedef enum FWPP_SUBLAYER_INTERNAL_
{
    FWPP_SUBLAYER_INTERNAL_INSPECTION            = 0,
    FWPP_SUBLAYER_INTERNAL_TEREDO                = 1,
    FWPP_SUBLAYER_INTERNAL_FIREWALL_WF           = 2,
    FWPP_SUBLAYER_INTERNAL_FIREWALL_WSH          = 3,
    FWPP_SUBLAYER_INTERNAL_FIREWALL_QUARANTINE   = 4,
    FWPP_SUBLAYER_INTERNAL_FIREWALL_EDP          = 5,
    FWPP_SUBLAYER_INTERNAL_FIREWALL_TENANT_RESTRICTIONS = 6,
    FWPP_SUBLAYER_INTERNAL_FIREWALL_APP_ISOLATION = 7,
} FWPP_SUBLAYER_INTERNAL;

typedef struct FWPP_TERMINATING_FILTER_INFO0_
{
    ULONGLONG FilterId;
    FWPP_SUBLAYER_INTERNAL SubLayer;
    FWP_ACTION_TYPE ActionType;
} FWPP_TERMINATING_FILTER_INFO0, *PFWPP_TERMINATING_FILTER_INFO0;

typedef struct FWPM_NET_EVENT_IKEEXT_MM_FAILURE2_EX_
{
    ULONG FailureErrorCode;
    IPSEC_FAILURE_POINT FailurePoint;
    ULONG Flags;
    IKEEXT_KEY_MODULE_TYPE keyingModuleType;
    IKEEXT_MM_SA_STATE mmState;
    IKEEXT_SA_ROLE saRole;
    IKEEXT_AUTHENTICATION_METHOD_TYPE mmAuthMethod;
    UINT8 EndCertHash[20];
    ULONGLONG mmId;
    ULONGLONG mmFilterId;
    PCWSTR LocalPrincipalNameForAuth;
    PCWSTR RemotePrincipalNameForAuth;
    ULONG LocalPrincipalGroupSidsCount;
    ULONG _pad0;
    PCWSTR* localPrincipalGroupSids;
    ULONG RemotePrincipalGroupSidsCount;
    ULONG _pad1;
    PCWSTR* RemotePrincipalGroupSids;
    PGUID ProviderContextKey;
} FWPM_NET_EVENT_IKEEXT_MM_FAILURE2_EX, *PFWPM_NET_EVENT_IKEEXT_MM_FAILURE2_EX;

typedef struct FWPM_NET_EVENT_CLASSIFY_DROP_MAC0_EX_
{
    FWP_BYTE_ARRAY6 LocalMacAddr;
    FWP_BYTE_ARRAY6 RemoteMacAddr;
    ULONG MediaType;
    ULONG IfType;
    USHORT EtherType;
    USHORT _padding0;
    ULONG NdisPortNumber;
    ULONG Reserved;
    USHORT vlanTag;
    USHORT _padding1;
    ULONG _padding2;
    ULONGLONG ifLuid;
    ULONGLONG filterId;
    USHORT layerId;
    USHORT _padding3;
    ULONG ReauthReason;
    ULONG originalProfile;
    ULONG CurrentProfile;
    FWP_DIRECTION Direction;
    ULONG IsLoopback;
    FWP_BYTE_BLOB vSwitchId;
    ULONG vSwitchSourcePort;
    ULONG vSwitchDestinationPort;
} FWPM_NET_EVENT_CLASSIFY_DROP_MAC0_EX, *PFWPM_NET_EVENT_CLASSIFY_DROP_MAC0_EX;

typedef struct FWPM_NET_EVENT_IPSEC_KERNEL_DROP0_EX_
{
    NTSTATUS FailureStatus;
    FWP_DIRECTION Direction;
    ULONG Spi;
    ULONG _pad0;
    ULONGLONG filterId;
    USHORT layerId;
} FWPM_NET_EVENT_IPSEC_KERNEL_DROP0_EX, *PFWPM_NET_EVENT_IPSEC_KERNEL_DROP0_EX;

typedef struct FWPM_NET_EVENT_IPSEC_DOSP_DROP0_EX_
{
    FWP_IP_VERSION ipVersion;
    union
    {
        IN_ADDR publicHostV4Addr;
        IN6_ADDR publicHostV6Addr2;
    } publicHostAddr;
    union
    {
        IN_ADDR internalHostV4Addr;
        IN6_ADDR internalHostV6Addr;
    } internalHostAddr;
    NTSTATUS failureStatus;
    FWP_DIRECTION direction;
} FWPM_NET_EVENT_IPSEC_DOSP_DROP0_EX, *PFWPM_NET_EVENT_IPSEC_DOSP_DROP0_EX;

typedef struct FWPM_NET_EVENT_CAPABILITY_DROP0_EX_
{
    FWPM_APPC_NETWORK_CAPABILITY_TYPE networkCapabilityId;
    ULONG _pad0;
    ULONGLONG filterId;
    ULONG isLoopback;
} FWPM_NET_EVENT_CAPABILITY_DROP0_EX, *PFWPM_NET_EVENT_CAPABILITY_DROP0_EX;

typedef struct FWPM_NET_EVENT_CAPABILITY_ALLOW0_EX_
{
    FWPM_APPC_NETWORK_CAPABILITY_TYPE networkCapabilityId;
    ULONG _pad0;
    ULONGLONG FilterId;
    ULONG isLoopback;
} FWPM_NET_EVENT_CAPABILITY_ALLOW0_EX, *PFWPM_NET_EVENT_CAPABILITY_ALLOW0_EX;

typedef struct FWPM_NET_EVENT_LPM_PACKET_ARRIVAL0_EX_
{
    ULONG spi;
} FWPM_NET_EVENT_LPM_PACKET_ARRIVAL0_EX, *PFWPM_NET_EVENT_LPM_PACKET_ARRIVAL0_EX;

typedef struct FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_ESTABLISHED0_EX_
{
    ULONGLONG mmLuid;
    ULONGLONG olderMMLuid;
    FWPM_NET_EVENT_INTERNAL_MM_NEGOTIATE_REASON Reason;
    ULONG NumOccurrences;
    FILETIME InitialOccurrence;
    FILETIME FinalOccurrence;
} FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_ESTABLISHED0_EX, *PFWPM_NET_EVENT_INTERNAL_IKEEXT_MM_ESTABLISHED0_EX;

typedef struct FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_DELETED0_EX_
{
    ULONGLONG MmLuid;
    ULONGLONG NewMMLuid;
    ULONG ErrorCode;
    ULONG NumOccurrences;
    FILETIME InitialOccurrence;
    FILETIME FinalOccurrence;
} FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_DELETED0_EX, *PFWPM_NET_EVENT_INTERNAL_IKEEXT_MM_DELETED0_EX;

typedef struct FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_FAILURE0_EX_
{
    ULONGLONG MmLuid;
    FWPM_NET_EVENT_INTERNAL_MM_NEGOTIATE_REASON Reason;
    ULONG _pad0;
    ULONGLONG OtherMMLuid;
    ULONG ErrorCode;
    ULONG NumOccurrences;
    FILETIME InitialOccurrence;
    FILETIME FinalOccurrence;
} FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_FAILURE0_EX, *PFWPM_NET_EVENT_INTERNAL_IKEEXT_MM_FAILURE0_EX;

typedef struct FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_ESTABLISHED0_EX_
{
    ULONGLONG MmLuid;
    ULONGLONG QmSaId;
    FWPM_NET_EVENT_INTERNAL_QM_NEGOTIATE_REASON Reason;
    ULONG NumOccurrences;
    FILETIME initialOccurrence;
    FILETIME FinalOccurrence;
} FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_ESTABLISHED0_EX, *PFWPM_NET_EVENT_INTERNAL_IKEEXT_QM_ESTABLISHED0_EX;

typedef struct FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_DELETED0_EX_
{
    ULONGLONG MmLuid;
    ULONGLONG QmSaId;
    ULONG ErrorCode;
    ULONG NumOccurrences;
    FILETIME InitialOccurrence;
    FILETIME FinalOccurrence;
} FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_DELETED0_EX, *PFWPM_NET_EVENT_INTERNAL_IKEEXT_QM_DELETED0_EX;

typedef struct FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_FAILURE0_EX_
{
    ULONGLONG MmLuid;
    ULONGLONG qmSaId;
    FWPM_NET_EVENT_INTERNAL_QM_NEGOTIATE_REASON Reason;
    ULONG ErrorCode;
    ULONG NumOccurrences;
    FILETIME initialOccurrence;
    FILETIME FinalOccurrence;
} FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_FAILURE0_EX, *PFWPM_NET_EVENT_INTERNAL_IKEEXT_QM_FAILURE0_EX;

typedef struct FWPM_NET_EVENT_INTERNAL_IKEEXT_NO_QM_PRESENT0_EX_
{
    ULONGLONG MmLuid;
    ULONG NumOccurrences;
    FILETIME initialOccurrence;
    FILETIME FinalOccurrence;
} FWPM_NET_EVENT_INTERNAL_IKEEXT_NO_QM_PRESENT0_EX, *PFWPM_NET_EVENT_INTERNAL_IKEEXT_NO_QM_PRESENT0_EX;

typedef struct FWPM_NET_EVENT_INTERNAL_IKEEXT_POLICY_CHANGED0_EX_
{
    ULONGLONG MmLuid;
    FWPM_NET_EVENT_INTERNAL_IKE_POLICY_CHANGE_TYPE PolicyChangeType;
    ULONG NumOccurrences;
    FILETIME initialOccurrence;
    FILETIME FinalOccurrence;
} FWPM_NET_EVENT_INTERNAL_IKEEXT_POLICY_CHANGED0_EX, *PFWPM_NET_EVENT_INTERNAL_IKEEXT_POLICY_CHANGED0_EX;

typedef union FWPM_NET_EVENT_INTERNAL_BODY_UNION_
{
    FWPM_NET_EVENT_IKEEXT_MM_FAILURE2* IkeMmFailure;
    FWPM_NET_EVENT_IKEEXT_QM_FAILURE1* IkeQmFailure;
    FWPM_NET_EVENT_IKEEXT_EM_FAILURE1* IkeEmFailure;
    FWPM_NET_EVENT_CLASSIFY_DROP2* ClassifyDrop;
    FWPM_NET_EVENT_IPSEC_KERNEL_DROP0_EX* IpsecKernelDrop;
    FWPM_NET_EVENT_IPSEC_DOSP_DROP0_EX* IpsecDospDrop;
    FWPM_NET_EVENT_CLASSIFY_ALLOW0* ClassifyAllow;
    FWPM_NET_EVENT_CAPABILITY_DROP0_EX* CapabilityDrop;
    FWPM_NET_EVENT_CAPABILITY_ALLOW0_EX* CapabilityAllow;
    FWPM_NET_EVENT_CLASSIFY_DROP_MAC0_EX* ClassifyDropMac;
    FWPM_NET_EVENT_LPM_PACKET_ARRIVAL0_EX* LpmPacketArrival;

    FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_ESTABLISHED0_EX* IkeInternalMmEstablished;
    FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_DELETED0_EX* IkeInternalMmDeleted;
    FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_FAILURE0_EX* IkeInternalMmFailure;
    FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_ESTABLISHED0_EX* IkeInternalQmEstablished;
    FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_DELETED0_EX* IkeInternalQmDeleted;
    FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_FAILURE0_EX* IikeInternalQmFailure;
    FWPM_NET_EVENT_INTERNAL_IKEEXT_NO_QM_PRESENT0_EX* IkeInternalNoQmPresent;
    FWPM_NET_EVENT_INTERNAL_IKEEXT_POLICY_CHANGED0_EX* IkeInternalPolicyChanged;

    PVOID Reserved;
} FWPM_NET_EVENT_INTERNAL_BODY_UNION;

// internalFlags
#define FWPM_NET_EVENT_INTERNAL_FLAG_FQBN_SET           0x00000001 // fqbnVersion + fqbnName valid
#define FWPM_NET_EVENT_INTERNAL_FLAG_FILTER_ORIGIN_SET  0x00000002 // filterOrigin valid
#define FWPM_NET_EVENT_INTERNAL_FLAG_POLICY_APP_ID_SET  0x00000004 // policyAppId valid

// Capabilities
#define FWP_CAPABILITIES_FLAG_INTERNET_CLIENT         0x00000001
#define FWP_CAPABILITIES_FLAG_INTERNET_CLIENT_SERVER  0x00000002
#define FWP_CAPABILITIES_FLAG_PRIVATE_NETWORK         0x00000004
#define FWP_CAPABILITIES_FLAG_NETWORK_LOOPBACK        0x00000008

/* =========================================================================
 * FWPM_NET_EVENT_INTERNAL_FIELDS0 (0x58 bytes (ends after serviceSids pointer))
 *
 * Extended per-event metadata appended after the public structure inside
 * FWPM_NET_EVENT_INTERNAL (at offset+0x98 from the structure base).
 * =========================================================================
 */ 
typedef struct _FWPM_NET_EVENT_INTERNAL_FIELDS0
{
    ULONG InternalFlags;                // FWPM_NET_EVENT_INTERNAL_FLAG_*
    ULONG Capabilities;                 // FWP_CAPABILITIES_FLAG_*
    ULONG64 FqbnVersion;                // The version stamp of the Fully Qualified Binary Name (FQBN)
    PCWSTR FqbnName;                    // Full native NT image path or the packaged application name.
    USHORT TerminatingFiltersInfoCount; // The number of entries present in the TerminatingFiltersInfo array.
    UCHAR _Reserved1[6];
    PFWPP_TERMINATING_FILTER_INFO0 TerminatingFiltersInfo;
    PCWSTR FilterOrigin;                // A pointer to a wide string indicating the source or owner of the terminating filter.
    NET_LUID InterfaceLuid;             // The Locally Unique Identifier (NET_LUID) of the network adapter on which the traffic was intercepted.
    PCWSTR PolicyAppId;
    NET_IF_COMPARTMENT_ID CompartmentId; // The network routing compartment identifier used for network isolation (Hyper-V, WSL).
    ULONG _Reserved2;                     // CompartmentId
    ULONG ProcessId;                     // The identifier of the process.
    ULONG _Reserved3;
    PCWSTR ServiceSids;                  // I_QueryTagInformation
} FWPM_NET_EVENT_INTERNAL_FIELDS0, *PFWPM_NET_EVENT_INTERNAL_FIELDS0;

/* =========================================================================
 * FWPM_NET_EVENT_INTERNAL
 *
 * The actual structure returned in the entries array from FwpmNetEventEnum5
 * Wraps the public FWPM_NET_EVENT_HEADER3 and appends an internal type discriminator,
 * typed-event body pointer (union), and the FWPM_NET_EVENT_INTERNAL_FIELDS0
 * extended metadata block.
 *
 *  +0x00  FWPM_NET_EVENT_HEADER3              header        (136 = 0x88 bytes)
 *  +0x88  FWPM_NET_EVENT_INTERNAL_TYPE        type
 *  +0x8C  ULONG                              _pad
 *  +0x90  void*                               eventBody     (typed by 'type')
 *  +0x98  FWPM_NET_EVENT_INTERNAL_FIELDS0     internalFields
 *
 * eventBody union (pointer at +0x90, NULL for sentinels / unknown types):
 *   type  0  FWPM_NET_EVENT_IKEEXT_MM_FAILURE2*
 *   type  1  FWPM_NET_EVENT_IKEEXT_QM_FAILURE1*
 *   type  2  FWPM_NET_EVENT_IKEEXT_EM_FAILURE1*
 *   type  3  FWPM_NET_EVENT_CLASSIFY_DROP2*
 *   type  4  FWPM_NET_EVENT_IPSEC_KERNEL_DROP0*
 *   type  5  FWPM_NET_EVENT_IPSEC_DOSP_DROP0*
 *   type  6  FWPM_NET_EVENT_CLASSIFY_ALLOW0*
 *   type  7  FWPM_NET_EVENT_CAPABILITY_DROP0*
 *   type  8  FWPM_NET_EVENT_CAPABILITY_ALLOW0*
 *   type  9  FWPM_NET_EVENT_CLASSIFY_DROP_MAC0*
 *   type 10  FWPM_NET_EVENT_LPM_PACKET_ARRIVAL0*
 *   type 13  FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_ESTABLISHED0*   (IKE-only)
 *   type 14  FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_DELETED0*       (IKE-only)
 *   type 15  FWPM_NET_EVENT_INTERNAL_IKEEXT_MM_FAILURE0*       (IKE-only)
 *   type 16  FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_ESTABLISHED0*   (IKE-only)
 *   type 17  FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_DELETED0*       (IKE-only)
 *   type 18  FWPM_NET_EVENT_INTERNAL_IKEEXT_QM_FAILURE0*       (IKE-only)
 *   type 19  FWPM_NET_EVENT_INTERNAL_IKEEXT_NO_QM_FOR_MMSA0*   (IKE-only)
 *   type 20  FWPM_NET_EVENT_INTERNAL_IKEEXT_POLICY_CHANGED0*   (IKE-only)
 *   type 11/12 (sentinels): eventBody = NULL; body written via INTERNAL_FIELDS
 * =========================================================================*/
typedef struct FWPM_NET_EVENT_INTERNAL_
{
    FWPM_NET_EVENT_HEADER3 Header;
    FWPM_NET_EVENT_INTERNAL_TYPE Type;
    ULONG Reserved;
    FWPM_NET_EVENT_INTERNAL_BODY_UNION EventBody;
    FWPM_NET_EVENT_INTERNAL_FIELDS0 InternalFields;
} FWPM_NET_EVENT_INTERNAL, *PFWPM_NET_EVENT_INTERNAL;

/* =========================================================================
 * FwpmNetEventCreateEnumHandleEx
 *
 * Extended enumeration-handle factory.  Unlike FwpmNetEventCreateEnumHandle0
 * it accepts a dataType selector to choose between the public WFP event set
 * and the internal IKE/AuthIP event set.
 *
 * IAT slot: 0x180034F68 (netsh WFP helper)
 * Caller:   FwppDiagFwpmNetEventEnumAllEx @ 0x18002460C
 *
 *   RCX = engineHandle
 *   RDX = dataType           (MOVZX from byte — FWPM_NET_EVENT_DATA_TYPE)
 *   R8  = enumTemplate       (NULL or ptr, via SBB R8,R8 / AND R8,RAX)
 *   R9  = &enumHandle        (local HANDLE, zero-initialised)
 * =========================================================================*/
DECLSPEC_IMPORT
DWORD
WINAPI
FwpmNetEventCreateEnumHandleEx(
    _In_ HANDLE EngineHandle,
    _In_ FWPM_NET_EVENT_DATA_TYPE DataType,
    _In_opt_ const FWPM_NET_EVENT_ENUM_TEMPLATE0* enumTemplate,
    _Out_ HANDLE* EnumHandle
    );

/* =========================================================================
 * FwpmNetEventEnum5
 *
 * Enumerates events into an array of FWPM_NET_EVENT_INTERNAL* using
 * a handle from FwpmNetEventCreateEnumHandleEx.
 *
 * On success *entries points to a fwpuclnt-allocated array; free with
 * FwpmFreeMemory0(&entries).
 * =========================================================================*/

DECLSPEC_IMPORT
ULONG
WINAPI
FwpmNetEventEnum5_(
    _In_ HANDLE EngineHandle,
    _In_ HANDLE EnumHandle,
    _In_ ULONG NumEntriesRequested,
    _Outptr_result_buffer_(*NumEntriesReturned) FWPM_NET_EVENT_INTERNAL*** Entries,
    _Out_ PULONG NumEntriesReturned
    );

typeof(&FwpmNetEventCreateEnumHandleEx) FwpmNetEventCreateEnumHandleEx_I;
typeof(&FwpmNetEventEnum5_) FwpmNetEventEnum5_I;


#endif
