#ifndef PH_NETPRV_H
#define PH_NETPRV_H

extern PPH_OBJECT_TYPE PhNetworkItemType;
PHAPPAPI extern PH_CALLBACK PhNetworkItemAddedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhNetworkItemModifiedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhNetworkItemRemovedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhNetworkItemsUpdatedEvent; // phapppub

extern BOOLEAN PhEnableNetworkProviderResolve;

// begin_phapppub
#define PH_NETWORK_OWNER_INFO_SIZE 16

typedef struct _PH_NETWORK_ITEM
{
    ULONG ProtocolType;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;
    ULONG State;
    HANDLE ProcessId;

    PPH_STRING ProcessName;
    HICON ProcessIcon;
    BOOLEAN ProcessIconValid;
    PPH_STRING OwnerName;

    BOOLEAN JustResolved;

    WCHAR LocalAddressString[65];
    WCHAR LocalPortString[PH_INT32_STR_LEN_1];
    WCHAR RemoteAddressString[65];
    WCHAR RemotePortString[PH_INT32_STR_LEN_1];
    PPH_STRING LocalHostString;
    PPH_STRING RemoteHostString;

    LARGE_INTEGER CreateTime;
    ULONGLONG OwnerInfo[PH_NETWORK_OWNER_INFO_SIZE];
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

PPH_STRING PhGetHostNameFromAddress(
    _In_ PPH_IP_ADDRESS Address
    );

VOID PhNetworkProviderUpdate(
    _In_ PVOID Object
    );

// begin_phapppub
PHAPPAPI
PWSTR
NTAPI
PhGetProtocolTypeName(
    _In_ ULONG ProtocolType
    );

PHAPPAPI
PWSTR
NTAPI
PhGetTcpStateName(
    _In_ ULONG State
    );
// end_phapppub

#endif
