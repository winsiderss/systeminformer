/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2017-2023
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
} PH_NETWORK_ITEM, *PPH_NETWORK_ITEM;
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
// end_phapppub

VOID PhFlushNetworkItemResolveCache(
    VOID
    );

//PPH_STRING PhGetHostNameFromAddress(
//    _In_ PPH_IP_ADDRESS Address
//    );

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
// end_phapppub

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

#endif
