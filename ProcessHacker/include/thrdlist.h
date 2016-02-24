#ifndef PH_THRDLIST_H
#define PH_THRDLIST_H

#include <phuisup.h>
#include <colmgr.h>

// Columns

#define PHTHTLC_TID 0
#define PHTHTLC_CPU 1
#define PHTHTLC_CYCLESDELTA 2
#define PHTHTLC_STARTADDRESS 3
#define PHTHTLC_PRIORITY 4
#define PHTHTLC_SERVICE 5

#define PHTHTLC_MAXIMUM 6

// begin_phapppub
typedef struct _PH_THREAD_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    HANDLE ThreadId;
    PPH_THREAD_ITEM ThreadItem;
// end_phapppub

    PH_STRINGREF TextCache[PHTHTLC_MAXIMUM];

    ULONG ValidMask;

    WCHAR CpuUsageText[PH_INT32_STR_LEN_1];
    PPH_STRING CyclesDeltaText; // used for Context Switches Delta as well
    PPH_STRING StartAddressText;
    PPH_STRING PriorityText;
// begin_phapppub
} PH_THREAD_NODE, *PPH_THREAD_NODE;
// end_phapppub

typedef struct _PH_THREAD_LIST_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;
    BOOLEAN UseCycleTime;
    BOOLEAN HasServices;

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;

    BOOLEAN EnableStateHighlighting;
    PPH_POINTER_LIST NodeStateList;
} PH_THREAD_LIST_CONTEXT, *PPH_THREAD_LIST_CONTEXT;

VOID PhInitializeThreadList(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PPH_THREAD_LIST_CONTEXT Context
    );

VOID PhDeleteThreadList(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    );

VOID PhLoadSettingsThreadList(
    _Inout_ PPH_THREAD_LIST_CONTEXT Context
    );

VOID PhSaveSettingsThreadList(
    _Inout_ PPH_THREAD_LIST_CONTEXT Context
    );

PPH_THREAD_NODE PhAddThreadNode(
    _Inout_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ PPH_THREAD_ITEM ThreadItem,
    _In_ BOOLEAN FirstRun
    );

PPH_THREAD_NODE PhFindThreadNode(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ HANDLE ThreadId
    );

VOID PhRemoveThreadNode(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ PPH_THREAD_NODE ThreadNode
    );

VOID PhUpdateThreadNode(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ PPH_THREAD_NODE ThreadNode
    );

VOID PhTickThreadNodes(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    );

PPH_THREAD_ITEM PhGetSelectedThreadItem(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    );

VOID PhGetSelectedThreadItems(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _Out_ PPH_THREAD_ITEM **Threads,
    _Out_ PULONG NumberOfThreads
    );

VOID PhDeselectAllThreadNodes(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    );

#endif
