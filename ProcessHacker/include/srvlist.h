#ifndef PH_SRVLIST_H
#define PH_SRVLIST_H

#include <phuisup.h>

// Columns

#define PHSVTLC_NAME 0
#define PHSVTLC_DISPLAYNAME 1
#define PHSVTLC_TYPE 2
#define PHSVTLC_STATUS 3
#define PHSVTLC_STARTTYPE 4
#define PHSVTLC_PID 5

#define PHSVTLC_BINARYPATH 6
#define PHSVTLC_ERRORCONTROL 7
#define PHSVTLC_GROUP 8
#define PHSVTLC_DESCRIPTION 9
#define PHSVTLC_KEYMODIFIEDTIME 10
#define PHSVTLC_VERIFICATIONSTATUS 11
#define PHSVTLC_VERIFIEDSIGNER 12
#define PHSVTLC_FILENAME 13
#define PHSVTLC_TIMELINE 14

#define PHSVTLC_MAXIMUM 15

#define PHSN_CONFIG 0x1
#define PHSN_DESCRIPTION 0x2
#define PHSN_KEY 0x4

// begin_phapppub
typedef struct _PH_SERVICE_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    PPH_SERVICE_ITEM ServiceItem;

    WCHAR StartTypeText[12 + 24 + 1];
    // Config
    PPH_STRING BinaryPath;
    PPH_STRING LoadOrderGroup;
    // Description
    PPH_STRING Description;
    // Key
    LARGE_INTEGER KeyLastWriteTime;
    PPH_STRING KeyModifiedTimeText;
// end_phapppub
    PPH_STRING TooltipText;
    ULONG ValidMask;
    PH_STRINGREF TextCache[PHSVTLC_MAXIMUM];
// begin_phapppub
} PH_SERVICE_NODE, *PPH_SERVICE_NODE;
// end_phapppub

VOID PhServiceTreeListInitialization(
    VOID
    );

VOID PhInitializeServiceTreeList(
    _In_ HWND hwnd
    );

VOID PhLoadSettingsServiceTreeList(
    VOID
    );

VOID PhSaveSettingsServiceTreeList(
    VOID
    );

// begin_phapppub
PHAPPAPI
struct _PH_TN_FILTER_SUPPORT *
NTAPI
PhGetFilterSupportServiceTreeList(
    VOID
    );
// end_phapppub

PPH_SERVICE_NODE PhAddServiceNode(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ ULONG RunId
    );

// begin_phapppub
PHAPPAPI
PPH_SERVICE_NODE
NTAPI
PhFindServiceNode(
    _In_ PPH_SERVICE_ITEM ServiceItem
    );
// end_phapppub

VOID PhRemoveServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhUpdateServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    );
// end_phapppub

VOID PhTickServiceNodes(
    VOID
    );

// begin_phapppub
PHAPPAPI
PPH_SERVICE_ITEM
NTAPI
PhGetSelectedServiceItem(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhGetSelectedServiceItems(
    _Out_ PPH_SERVICE_ITEM **Services,
    _Out_ PULONG NumberOfServices
    );

PHAPPAPI
VOID
NTAPI
PhDeselectAllServiceNodes(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhSelectAndEnsureVisibleServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    );
// end_phapppub

VOID PhCopyServiceList(
    VOID
    );

VOID PhWriteServiceList(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    );

#endif
