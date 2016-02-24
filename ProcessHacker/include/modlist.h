#ifndef PH_MODLIST_H
#define PH_MODLIST_H

#include <phuisup.h>
#include <colmgr.h>

// Columns

#define PHMOTLC_NAME 0
#define PHMOTLC_BASEADDRESS 1
#define PHMOTLC_SIZE 2
#define PHMOTLC_DESCRIPTION 3

#define PHMOTLC_COMPANYNAME 4
#define PHMOTLC_VERSION 5
#define PHMOTLC_FILENAME 6

#define PHMOTLC_TYPE 7
#define PHMOTLC_LOADCOUNT 8
#define PHMOTLC_VERIFICATIONSTATUS 9
#define PHMOTLC_VERIFIEDSIGNER 10
#define PHMOTLC_ASLR 11
#define PHMOTLC_TIMESTAMP 12
#define PHMOTLC_CFGUARD 13
#define PHMOTLC_LOADTIME 14
#define PHMOTLC_LOADREASON 15
#define PHMOTLC_FILEMODIFIEDTIME 16
#define PHMOTLC_FILESIZE 17

#define PHMOTLC_MAXIMUM 18

// begin_phapppub
typedef struct _PH_MODULE_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    PPH_MODULE_ITEM ModuleItem;
// end_phapppub

    PH_STRINGREF TextCache[PHMOTLC_MAXIMUM];

    ULONG ValidMask;

    PPH_STRING TooltipText;

    PPH_STRING SizeText;
    WCHAR LoadCountText[PH_INT32_STR_LEN_1];
    PPH_STRING TimeStampText;
    PPH_STRING LoadTimeText;
    PPH_STRING FileModifiedTimeText;
    PPH_STRING FileSizeText;
// begin_phapppub
} PH_MODULE_NODE, *PPH_MODULE_NODE;
// end_phapppub

typedef struct _PH_MODULE_LIST_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;

    BOOLEAN EnableStateHighlighting;
    PPH_POINTER_LIST NodeStateList;

    HFONT BoldFont;
} PH_MODULE_LIST_CONTEXT, *PPH_MODULE_LIST_CONTEXT;

VOID PhInitializeModuleList(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PPH_MODULE_LIST_CONTEXT Context
    );

VOID PhDeleteModuleList(
    _In_ PPH_MODULE_LIST_CONTEXT Context
    );

VOID PhLoadSettingsModuleList(
    _Inout_ PPH_MODULE_LIST_CONTEXT Context
    );

VOID PhSaveSettingsModuleList(
    _Inout_ PPH_MODULE_LIST_CONTEXT Context
    );

PPH_MODULE_NODE PhAddModuleNode(
    _Inout_ PPH_MODULE_LIST_CONTEXT Context,
    _In_ PPH_MODULE_ITEM ModuleItem,
    _In_ ULONG RunId
    );

PPH_MODULE_NODE PhFindModuleNode(
    _In_ PPH_MODULE_LIST_CONTEXT Context,
    _In_ PPH_MODULE_ITEM ModuleItem
    );

VOID PhRemoveModuleNode(
    _In_ PPH_MODULE_LIST_CONTEXT Context,
    _In_ PPH_MODULE_NODE ModuleNode
    );

VOID PhUpdateModuleNode(
    _In_ PPH_MODULE_LIST_CONTEXT Context,
    _In_ PPH_MODULE_NODE ModuleNode
    );

VOID PhTickModuleNodes(
    _In_ PPH_MODULE_LIST_CONTEXT Context
    );

PPH_MODULE_ITEM PhGetSelectedModuleItem(
    _In_ PPH_MODULE_LIST_CONTEXT Context
    );

VOID PhGetSelectedModuleItems(
    _In_ PPH_MODULE_LIST_CONTEXT Context,
    _Out_ PPH_MODULE_ITEM **Modules,
    _Out_ PULONG NumberOfModules
    );

VOID PhDeselectAllModuleNodes(
    _In_ PPH_MODULE_LIST_CONTEXT Context
    );

#endif
