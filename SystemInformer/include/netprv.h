#ifndef PH_NETPRV_H
#define PH_NETPRV_H

extern PPH_OBJECT_TYPE PhNetworkItemType;
extern BOOLEAN PhEnableNetworkProviderResolve;
extern BOOLEAN PhEnableNetworkBoundConnections;

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

    ULONG LocalAddressStringLength;
    ULONG RemoteAddressStringLength;
    WCHAR LocalAddressString[INET6_ADDRSTRLEN];
    WCHAR LocalPortString[PH_INT32_STR_LEN_1];
    WCHAR RemoteAddressString[INET6_ADDRSTRLEN];
    WCHAR RemotePortString[PH_INT32_STR_LEN_1];
    PPH_STRING LocalHostString;
    PPH_STRING RemoteHostString;

    LARGE_INTEGER CreateTime;
    ULONGLONG OwnerInfo[PH_NETWORK_OWNER_INFO_SIZE];
    ULONG LocalScopeId;
    ULONG RemoteScopeId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG UnknownProcess : 1;
            ULONG SubsystemProcess : 1;
            ULONG Spare : 28;
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
// end_phapppub

//PPH_STRING PhGetHostNameFromAddress(
//    _In_ PPH_IP_ADDRESS Address
//    );

VOID PhNetworkProviderUpdate(
    _In_ PVOID Object
    );

// begin_phapppub
PHAPPAPI
PH_STRINGREF
NTAPI
PhGetProtocolTypeName(
    _In_ ULONG ProtocolType
    );

PHAPPAPI
PH_STRINGREF
NTAPI
PhGetTcpStateName(
    _In_ ULONG State
    );
// end_phapppub

// iphlpapi imports

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

DECLSPEC_IMPORT ULONG WINAPI InternalGetBoundTcpEndpointTable(
    _Out_ PVOID* BoundTcpTable, // PMIB_TCPTABLE2
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG HeapFlags
    );

DECLSPEC_IMPORT ULONG WINAPI InternalGetBoundTcp6EndpointTable(
    _Out_ PVOID* BoundTcpTable, // PMIB_TCP6TABLE2
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG HeapFlags
    );

#endif
