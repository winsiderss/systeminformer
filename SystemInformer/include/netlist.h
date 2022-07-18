#ifndef PH_NETLIST_H
#define PH_NETLIST_H

#include <phuisup.h>

// Columns

#define PHNETLC_PROCESS 0
#define PHNETLC_LOCALADDRESS 1
#define PHNETLC_LOCALPORT 2
#define PHNETLC_REMOTEADDRESS 3
#define PHNETLC_REMOTEPORT 4
#define PHNETLC_PROTOCOL 5
#define PHNETLC_STATE 6
#define PHNETLC_OWNER 7
#define PHNETLC_TIMESTAMP 8
#define PHNETLC_LOCALHOSTNAME 9
#define PHNETLC_REMOTEHOSTNAME 10
#define PHNETLC_PID 11
#define PHNETLC_TIMELINE 12
#define PHNETLC_MAXIMUM 13

// begin_phapppub
typedef struct _PH_NETWORK_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    PPH_NETWORK_ITEM NetworkItem;

    PPH_STRING ProcessNameText;
    PPH_STRING TimeStampText;
    PPH_STRING PidText;
// end_phapppub

    PPH_STRING TooltipText;

    ULONG64 UniqueId;

    PH_STRINGREF TextCache[PHNETLC_MAXIMUM];
// begin_phapppub
} PH_NETWORK_NODE, *PPH_NETWORK_NODE;
// end_phapppub

VOID PhNetworkTreeListInitialization(
    VOID
    );

VOID PhInitializeNetworkTreeList(
    _In_ HWND hwnd
    );

VOID PhLoadSettingsNetworkTreeList(
    VOID
    );

VOID PhSaveSettingsNetworkTreeList(
    VOID
    );

// begin_phapppub
PHAPPAPI
struct _PH_TN_FILTER_SUPPORT *
NTAPI
PhGetFilterSupportNetworkTreeList(
    VOID
    );
// end_phapppub

PPH_NETWORK_NODE PhAddNetworkNode(
    _In_ PPH_NETWORK_ITEM NetworkItem,
    _In_ ULONG RunId
    );

// begin_phapppub
PHAPPAPI
PPH_NETWORK_NODE
NTAPI
PhFindNetworkNode(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );
// end_phapppub

VOID PhRemoveNetworkNode(
    _In_ PPH_NETWORK_NODE NetworkNode
    );

VOID PhUpdateNetworkNode(
    _In_ PPH_NETWORK_NODE NetworkNode
    );

VOID PhTickNetworkNodes(
    VOID
    );

PPH_NETWORK_ITEM PhGetSelectedNetworkItem(
    VOID
    );

VOID PhGetSelectedNetworkItems(
    _Out_ PPH_NETWORK_ITEM **NetworkItems,
    _Out_ PULONG NumberOfNetworkItems
    );

VOID PhDeselectAllNetworkNodes(
    VOID
    );

VOID PhSelectAndEnsureVisibleNetworkNode(
    _In_ PPH_NETWORK_NODE NetworkNode
    );

VOID PhCopyNetworkList(
    VOID
    );

VOID PhWriteNetworkList(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    );

#endif
