/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022
 *
 */

#ifndef NDIS_WLAN_H
#define NDIS_WLAN_H

//#include <wlantypes.h>
//#include <wlanapi.h>
#define WLAN_API_VERSION_2_0 0x00000002
#define DOT11_SSID_MAX_LENGTH 32 // 32 bytes
#define DOT11_RATE_SET_MAX_LENGTH 126 // 126 bytes
#define L2_PROFILE_MAX_NAME_LENGTH 256  // Profile name max length, in characters
#define WLAN_MAX_NAME_LENGTH L2_PROFILE_MAX_NAME_LENGTH // maximum length of name, in characters
#define WLAN_TIMESTAMP_TIME_UNIT 10 * 1024
#define WLAN_TIMESTAMP_SEC 1000000ULL
#define WLAN_TIMESTAMP_MIN (WLAN_TIMESTAMP_SEC * 60ULL)
#define WLAN_TIMESTAMP_HOUR (WLAN_TIMESTAMP_MIN * 60ULL)
#define WLAN_TIMESTAMP_DAY (WLAN_TIMESTAMP_HOUR * 24ULL)

typedef UCHAR DOT11_MAC_ADDRESS[6];
typedef DOT11_MAC_ADDRESS* PDOT11_MAC_ADDRESS;
typedef ULONG WLAN_SIGNAL_QUALITY, *PWLAN_SIGNAL_QUALITY;

typedef enum _DOT11_BSS_TYPE
{
    dot11_BSS_type_infrastructure = 1,
    dot11_BSS_type_independent = 2,
    dot11_BSS_type_any = 3
} DOT11_BSS_TYPE, *PDOT11_BSS_TYPE;

typedef enum _DOT11_PHY_TYPE
{
    dot11_phy_type_unknown = 0,
    dot11_phy_type_any = dot11_phy_type_unknown,
    dot11_phy_type_fhss = 1,
    dot11_phy_type_dsss = 2,
    dot11_phy_type_irbaseband = 3,
    dot11_phy_type_ofdm = 4, // 11a
    dot11_phy_type_hrdsss = 5, // 11b
    dot11_phy_type_erp = 6, // 11g
    dot11_phy_type_ht = 7, // 11n
    dot11_phy_type_vht = 8, // 11ac
    dot11_phy_type_dmg = 9, // 11ad
    dot11_phy_type_he = 10, // 11ax
    dot11_phy_type_eht = 11, // 11be
    dot11_phy_type_IHV_start = 0x80000000,
    dot11_phy_type_IHV_end = 0xffffffff
} DOT11_PHY_TYPE, *PDOT11_PHY_TYPE;

typedef enum _WLAN_INTF_OPCODE
{
    wlan_intf_opcode_autoconf_start = 0x000000000,
    wlan_intf_opcode_autoconf_enabled,
    wlan_intf_opcode_background_scan_enabled,
    wlan_intf_opcode_media_streaming_mode,
    wlan_intf_opcode_radio_state,
    wlan_intf_opcode_bss_type,
    wlan_intf_opcode_interface_state,
    wlan_intf_opcode_current_connection,
    wlan_intf_opcode_channel_number,
    wlan_intf_opcode_supported_infrastructure_auth_cipher_pairs,
    wlan_intf_opcode_supported_adhoc_auth_cipher_pairs,
    wlan_intf_opcode_supported_country_or_region_string_list,
    wlan_intf_opcode_current_operation_mode,
    wlan_intf_opcode_supported_safe_mode,
    wlan_intf_opcode_certified_safe_mode,
    wlan_intf_opcode_hosted_network_capable,
    wlan_intf_opcode_management_frame_protection_capable,
    wlan_intf_opcode_secondary_sta_interfaces,
    wlan_intf_opcode_secondary_sta_synchronized_connections,
    wlan_intf_opcode_autoconf_end = 0x0fffffff,
    wlan_intf_opcode_msm_start = 0x10000100,
    wlan_intf_opcode_statistics,
    wlan_intf_opcode_rssi,
    wlan_intf_opcode_msm_end = 0x1fffffff,
    wlan_intf_opcode_security_start = 0x20010000,
    wlan_intf_opcode_security_end = 0x2fffffff,
    wlan_intf_opcode_ihv_start = 0x30000000,
    wlan_intf_opcode_ihv_end = 0x3fffffff
} WLAN_INTF_OPCODE, *PWLAN_INTF_OPCODE;

typedef enum _WLAN_OPCODE_VALUE_TYPE
{
    wlan_opcode_value_type_query_only = 0,
    wlan_opcode_value_type_set_by_group_policy,
    wlan_opcode_value_type_set_by_user,
    wlan_opcode_value_type_invalid
} WLAN_OPCODE_VALUE_TYPE, *PWLAN_OPCODE_VALUE_TYPE;

typedef enum _WLAN_INTERFACE_STATE
{
    wlan_interface_state_not_ready,
    wlan_interface_state_connected,
    wlan_interface_state_ad_hoc_network_formed,
    wlan_interface_state_disconnecting,
    wlan_interface_state_disconnected,
    wlan_interface_state_associating,
    wlan_interface_state_discovering,
    wlan_interface_state_authenticating
} WLAN_INTERFACE_STATE, *PWLAN_INTERFACE_STATE;

typedef enum _WLAN_CONNECTION_MODE
{
    wlan_connection_mode_profile = 0,
    wlan_connection_mode_temporary_profile,
    wlan_connection_mode_discovery_secure,
    wlan_connection_mode_discovery_unsecure,
    wlan_connection_mode_auto,
    wlan_connection_mode_invalid
} WLAN_CONNECTION_MODE, *PWLAN_CONNECTION_MODE;

typedef enum _DOT11_AUTH_ALGORITHM
{
    DOT11_AUTH_ALGO_80211_OPEN = 1,
    DOT11_AUTH_ALGO_80211_SHARED_KEY = 2,
    DOT11_AUTH_ALGO_WPA = 3,
    DOT11_AUTH_ALGO_WPA_PSK = 4,
    DOT11_AUTH_ALGO_WPA_NONE = 5, // used in NatSTA only
    DOT11_AUTH_ALGO_RSNA = 6,
    DOT11_AUTH_ALGO_RSNA_PSK = 7,
    DOT11_AUTH_ALGO_WPA3 = 8, // means WPA3 Enterprise 192 bits
    DOT11_AUTH_ALGO_WPA3_ENT_192 = DOT11_AUTH_ALGO_WPA3,
    DOT11_AUTH_ALGO_WPA3_SAE = 9,
    DOT11_AUTH_ALGO_OWE = 10,
    DOT11_AUTH_ALGO_WPA3_ENT = 11,
    DOT11_AUTH_ALGO_IHV_START = 0x80000000,
    DOT11_AUTH_ALGO_IHV_END = 0xffffffff
} DOT11_AUTH_ALGORITHM, *PDOT11_AUTH_ALGORITHM;

typedef enum _DOT11_CIPHER_ALGORITHM
{
    DOT11_CIPHER_ALGO_NONE = 0x00,
    DOT11_CIPHER_ALGO_WEP40 = 0x01,
    DOT11_CIPHER_ALGO_TKIP = 0x02,
    DOT11_CIPHER_ALGO_CCMP = 0x04,
    DOT11_CIPHER_ALGO_WEP104 = 0x05,
    DOT11_CIPHER_ALGO_BIP = 0x06, // BIP-CMAC-128
    DOT11_CIPHER_ALGO_GCMP = 0x08, // GCMP-128
    DOT11_CIPHER_ALGO_GCMP_256 = 0x09, // GCMP-256
    DOT11_CIPHER_ALGO_CCMP_256 = 0x0a, // CCMP-256
    DOT11_CIPHER_ALGO_BIP_GMAC_128 = 0x0b, // BIP-GMAC-128
    DOT11_CIPHER_ALGO_BIP_GMAC_256 = 0x0c, // BIP-GMAC-256
    DOT11_CIPHER_ALGO_BIP_CMAC_256 = 0x0d, // BIP-CMAC-256
    DOT11_CIPHER_ALGO_WPA_USE_GROUP = 0x100,
    DOT11_CIPHER_ALGO_RSN_USE_GROUP = 0x100,
    DOT11_CIPHER_ALGO_WEP = 0x101,
    DOT11_CIPHER_ALGO_IHV_START = 0x80000000,
    DOT11_CIPHER_ALGO_IHV_END = 0xffffffff
} DOT11_CIPHER_ALGORITHM, *PDOT11_CIPHER_ALGORITHM;

typedef struct _DOT11_SSID
{
    ULONG SIDLength;
    UCHAR SSID[DOT11_SSID_MAX_LENGTH];
} DOT11_SSID, *PDOT11_SSID;

typedef struct _WLAN_RATE_SET
{
    ULONG RateSetLength;
    _Field_size_part_(DOT11_RATE_SET_MAX_LENGTH, RateSetLength) USHORT RateSet[DOT11_RATE_SET_MAX_LENGTH];
} WLAN_RATE_SET, *PWLAN_RATE_SET;

typedef struct _WLAN_BSS_ENTRY
{
    DOT11_SSID Ssid;
    ULONG PhyId;
    DOT11_MAC_ADDRESS Bssid;
    DOT11_BSS_TYPE BssType;
    DOT11_PHY_TYPE BssPhyType;
    LONG Rssi;
    ULONG LinkQuality;
    BOOLEAN InRegDomain;
    USHORT BeaconPeriod;
    ULONGLONG Timestamp;
    ULONGLONG HostTimestamp;
    USHORT CapabilityInformation;
    ULONG ChCenterFrequency;
    WLAN_RATE_SET RateSet;
    // the beginning of the IE blob
    // the offset is w.r.t. the beginning of the entry
    ULONG IeOffset;
    // size of the IE blob
    ULONG IeSize;
} WLAN_BSS_ENTRY, *PWLAN_BSS_ENTRY;

typedef struct _WLAN_BSS_LIST
{
    ULONG TotalSize;
    ULONG NumberOfItems;
    WLAN_BSS_ENTRY BssEntries[1];
} WLAN_BSS_LIST, *PWLAN_BSS_LIST;

typedef struct _WLAN_ASSOCIATION_ATTRIBUTES
{
    DOT11_SSID Ssid;
    DOT11_BSS_TYPE BssType;
    DOT11_MAC_ADDRESS Bssid;
    DOT11_PHY_TYPE PhyType;
    ULONG PhyIndex;
    WLAN_SIGNAL_QUALITY SignalQuality;
    ULONG RxRate;
    ULONG TxRate;
} WLAN_ASSOCIATION_ATTRIBUTES, *PWLAN_ASSOCIATION_ATTRIBUTES;

typedef struct _WLAN_SECURITY_ATTRIBUTES
{
    BOOL SecurityEnabled;
    BOOL OneXEnabled;
    DOT11_AUTH_ALGORITHM AuthAlgorithm;
    DOT11_CIPHER_ALGORITHM CipherAlgorithm;
} WLAN_SECURITY_ATTRIBUTES, *PWLAN_SECURITY_ATTRIBUTES;

typedef struct _WLAN_CONNECTION_ATTRIBUTES
{
    WLAN_INTERFACE_STATE State;
    WLAN_CONNECTION_MODE ConnectionMode;
    WCHAR ProfileName[WLAN_MAX_NAME_LENGTH];
    WLAN_ASSOCIATION_ATTRIBUTES AssociationAttributes;
    WLAN_SECURITY_ATTRIBUTES SecurityAttributes;
} WLAN_CONNECTION_ATTRIBUTES, *PWLAN_CONNECTION_ATTRIBUTES;

ULONG WINAPI WlanOpenHandle(
    _In_ ULONG Version,
    _Reserved_ PVOID Reserved,
    _Out_ PULONG NegotiatedVersion,
    _Out_ PHANDLE ClientHandle
    );

ULONG WINAPI WlanCloseHandle(
    _In_ HANDLE ClientHandle,
    _Reserved_ PVOID Reserved
    );

VOID WINAPI WlanFreeMemory(
    _In_ PVOID Memory
    );

ULONG WINAPI WlanGetNetworkBssList(
    _In_ HANDLE ClientHandle,
    _In_ CONST GUID* InterfaceGuid,
    _In_opt_ CONST PDOT11_SSID Ssid,
    _In_ DOT11_BSS_TYPE BssType,
    _In_ BOOL SecurityEnabled,
    _Reserved_ PVOID Reserved,
    _Out_ PWLAN_BSS_LIST* WlanBssList
    );

ULONG WINAPI WlanQueryInterface(
    _In_ HANDLE ClientHandle,
    _In_ CONST GUID* InterfaceGuid,
    _In_ WLAN_INTF_OPCODE OpCode,
    _Reserved_ PVOID Reserved,
    _Out_ PULONG DataSize,
    _Outptr_result_bytebuffer_(*DataSize) PVOID* Data,
    _Out_opt_ PWLAN_OPCODE_VALUE_TYPE WlanOpcodeValueType
    );

// Attributes

#define WLAN_EXTENDED_ID_BSS_LOAD 11

typedef struct _WLAN_EXTENDED_HEADER
{
    BYTE Id;
    BYTE Length;
} WLAN_EXTENDED_HEADER, *PWLAN_EXTENDED_HEADER;

typedef struct _WLAN_EXTENDED_BSS_LOAD
{
    BYTE ConnectedStationsLow;
    BYTE ConnectedStationsHigh;
    BYTE ChannelUtilization;
} WLAN_EXTENDED_BSS_LOAD, *PWLAN_EXTENDED_BSS_LOAD;

#endif
