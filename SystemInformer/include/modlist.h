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
#define PHMOTLC_ENTRYPOINT 18
#define PHMOTLC_PARENTBASEADDRESS 19
#define PHMOTLC_CET 20
#define PHMOTLC_COHERENCY 21
#define PHMOTLC_TIMELINE 22
#define PHMOTLC_ORIGINALNAME 23
#define PHMOTLC_SERVICE 24

#define PHMOTLC_MAXIMUM 25

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
    PPH_STRING ImageCoherencyText;
    PPH_STRING ServiceText;

    struct _PH_MODULE_NODE *Parent;
    PPH_LIST Children;
// begin_phapppub
} PH_MODULE_NODE, *PPH_MODULE_NODE;
// end_phapppub

#define PH_MODULE_FLAGS_DYNAMIC_OPTION 1
#define PH_MODULE_FLAGS_MAPPED_OPTION 2
#define PH_MODULE_FLAGS_STATIC_OPTION 3
#define PH_MODULE_FLAGS_SIGNED_OPTION 4
#define PH_MODULE_FLAGS_HIGHLIGHT_UNSIGNED_OPTION 5
#define PH_MODULE_FLAGS_HIGHLIGHT_DOTNET_OPTION 6
#define PH_MODULE_FLAGS_HIGHLIGHT_IMMERSIVE_OPTION 7
#define PH_MODULE_FLAGS_HIGHLIGHT_RELOCATED_OPTION 8
#define PH_MODULE_FLAGS_LOAD_MODULE_OPTION 9
#define PH_MODULE_FLAGS_MODULE_STRINGS_OPTION 10
#define PH_MODULE_FLAGS_SYSTEM_OPTION 11
#define PH_MODULE_FLAGS_HIGHLIGHT_SYSTEM_OPTION 12
#define PH_MODULE_FLAGS_LOWIMAGECOHERENCY_OPTION 13
#define PH_MODULE_FLAGS_HIGHLIGHT_LOWIMAGECOHERENCY_OPTION 14
#define PH_MODULE_FLAGS_IMAGEKNOWNDLL_OPTION 15
#define PH_MODULE_FLAGS_HIGHLIGHT_IMAGEKNOWNDLL 16
#define PH_MODULE_FLAGS_SAVE_OPTION 40 // Always last (dmex)

typedef struct _PH_MODULE_LIST_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_TN_FILTER_SUPPORT TreeFilterSupport;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;

    HANDLE ProcessId;
    LARGE_INTEGER ProcessCreateTime;
    BOOLEAN HasServices;
    BOOLEAN EnableStateHighlighting;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG Reserved : 1;
            ULONG HideDynamicModules : 1;
            ULONG HideMappedModules : 1;
            ULONG HideSignedModules : 1;
            ULONG HideStaticModules : 1;
            ULONG HighlightUntrustedModules : 1;
            ULONG HighlightDotNetModules : 1;
            ULONG HighlightImmersiveModules : 1;
            ULONG HighlightRelocatedModules : 1;
            ULONG HideSystemModules : 1;
            ULONG HighlightSystemModules : 1;
            ULONG HideLowImageCoherency : 1;
            ULONG HighlightLowImageCoherency : 1;
            ULONG HideImageKnownDll : 1;
            ULONG HighlightImageKnownDll : 1;
            ULONG Spare : 17;
        };
    };

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
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

VOID PhSetOptionsModuleList(
    _Inout_ PPH_MODULE_LIST_CONTEXT Context,
    _In_ ULONG Options
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

VOID PhExpandAllModuleNodes(
    _In_ PPH_MODULE_LIST_CONTEXT Context,
    _In_ BOOLEAN Expand
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

BOOLEAN PhShouldShowModuleCoherency(
    _In_ PPH_MODULE_ITEM ModuleItem,
    _In_ BOOLEAN CheckThreshold
    );

#endif
