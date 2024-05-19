#ifndef PH_HNDLLIST_H
#define PH_HNDLLIST_H

#include <phuisup.h>
#include <colmgr.h>

// Columns

#define PHHNTLC_TYPE 0
#define PHHNTLC_NAME 1
#define PHHNTLC_HANDLE 2

#define PHHNTLC_OBJECTADDRESS 3
#define PHHNTLC_ATTRIBUTES 4
#define PHHNTLC_GRANTEDACCESS 5
#define PHHNTLC_GRANTEDACCESSSYMBOLIC 6
#define PHHNTLC_ORIGINALNAME 7
#define PHHNTLC_FILESHAREACCESS 8

#define PHHNTLC_MAXIMUM 9

// begin_phapppub
typedef enum _PH_HANDLE_TREE_MENUITEM
{
    PH_HANDLE_TREE_MENUITEM_NONE,

    PH_HANDLE_TREE_MENUITEM_HIDE_PROTECTED_HANDLES,
    PH_HANDLE_TREE_MENUITEM_HIDE_INHERIT_HANDLES,
    PH_HANDLE_TREE_MENUITEM_HIDE_UNNAMED_HANDLES,
    PH_HANDLE_TREE_MENUITEM_HIDE_ETW_HANDLES,

    PH_HANDLE_TREE_MENUITEM_HIGHLIGHT_PROTECTED_HANDLES,
    PH_HANDLE_TREE_MENUITEM_HIGHLIGHT_INHERIT_HANDLES,

    PH_HANDLE_TREE_MENUITEM_HANDLESTATS,
    PH_HANDLE_TREE_MENUITEM_MAXIMUM
} PH_HANDLE_TREE_MENUITEM;
// end_phapppub

// begin_phapppub
typedef struct _PH_HANDLE_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    HANDLE Handle;
    PPH_HANDLE_ITEM HandleItem;
// end_phapppub

    PH_STRINGREF TextCache[PHHNTLC_MAXIMUM];

    PPH_STRING GrantedAccessSymbolicText;
    WCHAR FileShareAccessText[4];
// begin_phapppub
} PH_HANDLE_NODE, *PPH_HANDLE_NODE;
// end_phapppub

typedef struct _PH_HANDLE_LIST_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;

    PH_TN_FILTER_SUPPORT TreeFilterSupport;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;

    BOOLEAN EnableStateHighlighting;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG Reserved : 1;
            ULONG HideUnnamedHandles : 1;
            ULONG HideEtwHandles : 1;
            ULONG HideProtectedHandles : 1;
            ULONG HideInheritHandles : 1;
            ULONG Spare : 27;
        };
    };

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_POINTER_LIST NodeStateList;
} PH_HANDLE_LIST_CONTEXT, *PPH_HANDLE_LIST_CONTEXT;

VOID PhInitializeHandleList(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PPH_HANDLE_LIST_CONTEXT Context
    );

VOID PhDeleteHandleList(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    );

VOID PhLoadSettingsHandleList(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context
    );

VOID PhSaveSettingsHandleList(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context
    );

VOID PhSetOptionsHandleList(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ ULONG Options
    );

PPH_HANDLE_NODE PhAddHandleNode(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ PPH_HANDLE_ITEM HandleItem,
    _In_ ULONG RunId
    );

PPH_HANDLE_NODE PhFindHandleNode(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ HANDLE Handle
    );

VOID PhRemoveHandleNode(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ PPH_HANDLE_NODE HandleNode
    );

VOID PhUpdateHandleNode(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ PPH_HANDLE_NODE HandleNode
    );

VOID PhExpandAllHandleNodes(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ BOOLEAN Expand
    );

VOID PhTickHandleNodes(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    );

PPH_HANDLE_ITEM PhGetSelectedHandleItem(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    );

VOID PhGetSelectedHandleItems(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _Out_ PPH_HANDLE_ITEM **Handles,
    _Out_ PULONG NumberOfHandles
    );

VOID PhDeselectAllHandleNodes(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    );

#endif
