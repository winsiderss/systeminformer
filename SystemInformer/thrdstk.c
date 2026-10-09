/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2026
 *
 */

#include <phapp.h>
#include <phsettings.h>

#include <cpysave.h>
#include <emenu.h>
#include <kphuser.h>
#include <ksisup.h>
#include <symprv.h>

#include <actions.h>
#include <colmgr.h>
#include <phplug.h>
#include <settings.h>
#include <thrdprv.h>
#include <memprv.h>

#define WM_PH_COMPLETED (WM_APP + 301)
//#define WM_PH_STATUS_UPDATE (WM_APP + 302)
#define WM_PH_SHOWSTACKMENU (WM_APP + 303)
#define WM_PH_SHOWSTACKDEFAULT (WM_APP + 304)
#define WM_PH_FRAMES_READY (WM_APP + 305)    // posted when deferred phase 1 frames are populated
#define WM_PH_SYMBOL_RESOLVED (WM_APP + 306) // wParam = frame index; posted per-frame during phase 2
#define WM_PH_SYMBOLS_COMPLETE (WM_APP + 307)
#define WM_PH_SYMBOL_STATUS (WM_APP + 308)   // StatusContent changed; update the status label

static PPH_OBJECT_TYPE PhThreadStackContextType = NULL;
static RECT MinimumSize = { -1, -1, -1, -1 };

typedef struct _PH_THREAD_STACK_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ThreadId;
    HANDLE ThreadHandle;
    PPH_THREAD_PROVIDER ThreadProvider;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    PH_CFG_TARGET_CONTEXT CfgTargetContext;
    NT_TIB NativeTib;
    NT_TIB32 Wow64Tib;
    KPH_KERNEL_STACK_INFORMATION KernelStackInformation;
    BOOLEAN CfgTargetContextValid;
    BOOLEAN NativeTibValid;
    BOOLEAN Wow64TibValid;
    BOOLEAN KernelStackInformationValid;
    BOOLEAN CustomWalkActive;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG CustomWalk : 1;
            ULONG Spare1 : 1;
            ULONG EnableCloseDialog : 1;
            ULONG HighlightSystemPages : 1;
            ULONG HighlightUserPages : 1;
            ULONG HideSystemPages : 1;
            ULONG HideUserPages : 1;
            ULONG HighlightInlineFrames : 1;
            ULONG HideInlineFrames : 1;
            ULONG DeferSymbols : 1;     // Phase-1/phase-2 (deferred) symbol resolution is enabled.
            ULONG Spare2 : 1;
            ULONG Spare : 21;
        };
    };

    PPH_LIST List;
    PPH_LIST NewList;
    PH_TN_FILTER_SUPPORT TreeFilterSupport;
    PPH_TN_FILTER_ENTRY TreeFilterEntry;

    HWND TaskDialogHandle;

    PPH_STRING StatusMessage;
    PPH_STRING StatusContent;
    PH_QUEUED_LOCK StatusLock;

    BOOLEAN SymbolProgressMarquee;
    BOOLEAN SymbolProgressReset;

    ULONG SymbolProgress;
    NTSTATUS WalkStatus;
    LONG WindowDpi;

    PH_LAYOUT_MANAGER LayoutManager;

    HWND WindowHandle;
    HWND ParentHandle;
    HWND TreeNewHandle;
    HFONT TreeNewFont;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
    WNDPROC ThreadStackStatusDefaultWindowProc;
    PH_CALLBACK_REGISTRATION SymbolProviderEventRegistration;

    HANDLE WorkerCompletedEvent;
    ULONG WorkerGeneration;     // Bumped by the UI thread to start/cancel a deferred worker.
    ULONG ActiveGeneration;     // Generation of the deferred worker currently walking (0 for legacy walks).
    BOOLEAN RefreshPending;     // UI thread only: refresh requested while a deferred worker was running.

    // Accessed from both the UI and worker threads; kept out of the Flags bitfield so
    // concurrent writes do not clobber neighbouring bits.
    BOOLEAN StopWalk;
    BOOLEAN SkipSymbolPass;     // Set on the worker context to suppress symbol calls during the walk callback.

    // Phase 2 results (THREAD_STACK_RESOLVED_ITEM) pushed by the worker, popped by the UI thread.
    SLIST_HEADER ResolvedListHead;

    BOOLEAN IsWow64Process; // WOW64 (or ARM32 on ARM64); selects PH_WALK_USER_WOW64_STACK.

    HWND SearchboxHandle;
    HWND StatusHandle;
    ULONG_PTR SearchMatchHandle;
    BOOLEAN StatusUpdatePending;  // Coalesces WM_PH_SYMBOL_STATUS posts.
    PH_CALLBACK_REGISTRATION DeferredSymbolEventRegistration;
    PPH_HASHTABLE SourceLinkCache; // module base -> PPH_BYTES Source Link document (NULL if none). UI thread only.
} PH_THREAD_STACK_CONTEXT, *PPH_THREAD_STACK_CONTEXT;

typedef struct _THREAD_STACK_ITEM
{
    PH_THREAD_STACK_FRAME StackFrame;
    ULONG Index;
    PPH_STRING Symbol;
    PPH_STRING FileName;
    PPH_STRING LineText;
    PPH_STRING Protection;
    PPH_STRING MemoryType;
    PPH_STRING StackValid;
    PPH_STRING UnwindMethod;
    PPH_STRING CfgTarget;
    PPH_STRING Language;
    PVOID DiagnosticAddress;
    PVOID FrameContext; // Copy of the unwound register context (PH_THREAD_STACK_FRAME::ContextRecord).
} THREAD_STACK_ITEM, *PTHREAD_STACK_ITEM;

// Worker-private copy of the frame data needed for deferred symbol resolution.
typedef struct _THREAD_STACK_DEFERRED_FRAME
{
    PH_THREAD_STACK_FRAME StackFrame;
    PVOID DiagnosticAddress;
    ULONG Index;
} THREAD_STACK_DEFERRED_FRAME, *PTHREAD_STACK_DEFERRED_FRAME;

// Symbol result handed from the deferred worker to the UI thread via ResolvedListHead.
typedef struct _THREAD_STACK_RESOLVED_ITEM
{
    SLIST_ENTRY ListEntry;
    ULONG Generation;
    ULONG Index;
    PPH_STRING Symbol;
    PPH_STRING FileName;
    PPH_STRING LineText;
    PPH_STRING Language;
} THREAD_STACK_RESOLVED_ITEM, *PTHREAD_STACK_RESOLVED_ITEM;

typedef enum _PH_STACK_TREE_COLUMN_ITEM_NAME
{
    PH_STACK_TREE_COLUMN_INDEX,
    PH_STACK_TREE_COLUMN_SYMBOL,
    PH_STACK_TREE_COLUMN_STACKADDRESS,
    PH_STACK_TREE_COLUMN_FRAMEADDRESS,
    PH_STACK_TREE_COLUMN_PARAMETER1,
    PH_STACK_TREE_COLUMN_PARAMETER2,
    PH_STACK_TREE_COLUMN_PARAMETER3,
    PH_STACK_TREE_COLUMN_PARAMETER4,
    PH_STACK_TREE_COLUMN_CONTROLADDRESS,
    PH_STACK_TREE_COLUMN_RETURNADDRESS,
    PH_STACK_TREE_COLUMN_FILENAME,
    PH_STACK_TREE_COLUMN_LINETEXT,
    PH_STACK_TREE_COLUMN_ARCHITECTURE,
    PH_STACK_TREE_COLUMN_FRAMEDISTANCE,
    PH_STACK_TREE_COLUMN_PROTECTION,
    PH_STACK_TREE_COLUMN_MEMORYTYPE,
    PH_STACK_TREE_COLUMN_STACKVALID,
    PH_STACK_TREE_COLUMN_UNWINDMETHOD,
    PH_STACK_TREE_COLUMN_CFGTARGET,
    PH_STACK_TREE_COLUMN_LANGUAGE,
    TREE_COLUMN_ITEM_MAXIMUM
} PH_STACK_TREE_COLUMN_ITEM_NAME;

typedef struct _PH_STACK_TREE_ROOT_NODE
{
    PH_TREENEW_NODE Node;

    PH_THREAD_STACK_FRAME StackFrame;
    PVOID FrameContext; // Borrowed from THREAD_STACK_ITEM.

    ULONG Index;
    ULONG FrameDistance;
    PPH_STRING TooltipText;
    PPH_STRING IndexString;
    PPH_STRING SymbolString;
    PPH_STRING FileNameString;
    PPH_STRING LineTextString;
    WCHAR StackAddressString[PH_PTR_STR_LEN_1];
    WCHAR FrameAddressString[PH_PTR_STR_LEN_1];
    WCHAR Parameter1String[PH_PTR_STR_LEN_1];
    WCHAR Parameter2String[PH_PTR_STR_LEN_1];
    WCHAR Parameter3String[PH_PTR_STR_LEN_1];
    WCHAR Parameter4String[PH_PTR_STR_LEN_1];
    WCHAR PcAddressString[PH_PTR_STR_LEN_1];
    WCHAR ReturnAddressString[PH_PTR_STR_LEN_1];
    PH_STRINGREF Architecture;
    PPH_STRING FrameDistanceString;
    PPH_STRING ProtectionString;
    PPH_STRING MemoryTypeString;
    PPH_STRING StackValidString;
    PPH_STRING UnwindMethodString;
    PPH_STRING CfgTargetString;
    PPH_STRING LanguageString;

    PH_STRINGREF TextCache[TREE_COLUMN_ITEM_MAXIMUM];
} PH_STACK_TREE_ROOT_NODE, *PPH_STACK_TREE_ROOT_NODE;

typedef enum _PH_THREAD_STACK_MENUITEM
{
    PH_THREAD_STACK_MENUITEM_INSPECT = 1,
    PH_THREAD_STACK_MENUITEM_OPENFILELOCATION,
    PH_THREAD_STACK_MENUITEM_REGISTERS,
    PH_THREAD_STACK_MENUITEM_OPENSOURCELINK,
    PH_THREAD_STACK_MENUITEM_COPYSOURCELINK,
} PH_THREAD_STACK_MENUITEM;

/**
 * Gets a browsable Source Link URL for a stack frame address, caching the module's Source Link document.
 *
 * \param Context The thread stack context.
 * \param Address The frame address.
 * \param Url A pointer to a variable that receives the URL.
 * \return TRUE on success, FALSE otherwise.
 */
_Success_(return)
static BOOLEAN PhpGetThreadStackSourceLinkUrl(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ PVOID Address,
    _Out_ PPH_STRING* Url
    )
{
    BOOLEAN result;
    PVOID baseAddress;
    PVOID* entry;
    PPH_BYTES sourceLink;
    PPH_STRING fileName;
    PH_SYMBOL_LINE_INFORMATION lineInfo;

    if (!(baseAddress = PhGetModuleFromAddress(Context->SymbolProvider, Address, NULL)))
        return FALSE;

    // Line information implies the PDB is loaded, so a missing Source Link document is final and safe to cache.
    if (!PhGetLineFromAddress(Context->SymbolProvider, Address, &fileName, NULL, &lineInfo))
        return FALSE;

    if (!Context->SourceLinkCache)
        Context->SourceLinkCache = PhCreateSimpleHashtable(8);

    if (entry = PhFindItemSimpleHashtable(Context->SourceLinkCache, baseAddress))
    {
        sourceLink = *entry;
    }
    else
    {
        if (!PhGetSymbolProviderSourceLink(Context->SymbolProvider, baseAddress, &sourceLink))
            sourceLink = NULL;

        PhAddItemSimpleHashtable(Context->SourceLinkCache, baseAddress, sourceLink);
    }

    if (sourceLink)
        result = PhResolveSourceLinkUrl(sourceLink, &fileName->sr, lineInfo.LineNumber, Url);
    else
        result = FALSE;

    PhDereferenceObject(fileName);

    return result;
}

INT_PTR CALLBACK PhpThreadStackDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static VOID PhpFreeThreadStackResolvedItem(
    _In_ PTHREAD_STACK_RESOLVED_ITEM ResolvedItem
    )
{
    if (ResolvedItem->Symbol) PhDereferenceObject(ResolvedItem->Symbol);
    if (ResolvedItem->FileName) PhDereferenceObject(ResolvedItem->FileName);
    if (ResolvedItem->LineText) PhDereferenceObject(ResolvedItem->LineText);
    if (ResolvedItem->Language) PhDereferenceObject(ResolvedItem->Language);
    PhFree(ResolvedItem);
}

static VOID PhpSetThreadStackStatus(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ _Assume_refs_(1) PPH_STRING Status
    )
{
    PhAcquireQueuedLockExclusive(&Context->StatusLock);
    PhMoveReference(&Context->StatusContent, Status);
    PhReleaseQueuedLockExclusive(&Context->StatusLock);

    // Only the deferred mode shows the status label; the legacy mode uses the task dialog.
    if (Context->DeferSymbols && Context->WindowHandle && !_InterlockedExchange8((PCHAR)&Context->StatusUpdatePending, TRUE))
        PostMessage(Context->WindowHandle, WM_PH_SYMBOL_STATUS, 0, 0);
}

static BOOLEAN PhpThreadStackWalkCancelled(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    return ReadBooleanAcquire(&Context->StopWalk) ||
        ReadULongAcquire(&Context->ActiveGeneration) != ReadULongAcquire(&Context->WorkerGeneration);
}

VOID PhpFreeThreadStackItem(
    _In_ PTHREAD_STACK_ITEM StackItem
    );

NTSTATUS PhpRefreshThreadStack(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_STACK_CONTEXT ThreadStackContext
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID PhpSymbolProviderEventCallbackHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID PhpApplyThreadStackFrames(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    );

#define SORT_FUNCTION(Column) ThreadStackTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl ThreadStackTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_THREAD_STACK_CONTEXT context = ((PPH_THREAD_STACK_CONTEXT)_context); \
    PPH_STACK_TREE_ROOT_NODE node1 = *(PPH_STACK_TREE_ROOT_NODE*)_elem1; \
    PPH_STACK_TREE_ROOT_NODE node2 = *(PPH_STACK_TREE_ROOT_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, context->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Index)
{
    sortResult = uint64cmp(node1->Index, node2->Index);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Symbol)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->SymbolString, node2->SymbolString, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StackAddress)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->StackFrame.StackAddress, (ULONG_PTR)node2->StackFrame.StackAddress);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(FrameAddress)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->StackFrame.FrameAddress, (ULONG_PTR)node2->StackFrame.FrameAddress);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StackParameter1)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->StackFrame.Params[0], (ULONG_PTR)node2->StackFrame.Params[0]);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StackParameter2)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->StackFrame.Params[1], (ULONG_PTR)node2->StackFrame.Params[1]);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StackParameter3)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->StackFrame.Params[2], (ULONG_PTR)node2->StackFrame.Params[2]);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StackParameter4)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->StackFrame.Params[3], (ULONG_PTR)node2->StackFrame.Params[3]);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ControlAddress)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->StackFrame.PcAddress, (ULONG_PTR)node2->StackFrame.PcAddress);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ReturnAddress)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->StackFrame.ReturnAddress, (ULONG_PTR)node2->StackFrame.ReturnAddress);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(FileName)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->FileNameString, node2->FileNameString, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LineText)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->LineTextString, node2->LineTextString, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Architecture)
{
    sortResult = ushortcmp(node1->StackFrame.Machine, node2->StackFrame.Machine);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(FrameDistance)
{
    sortResult = uintcmp(node1->FrameDistance, node2->FrameDistance);
}
END_SORT_FUNCTION

#define STRING_SORT_FUNCTION(Name, Field) \
BEGIN_SORT_FUNCTION(Name) \
{ \
    sortResult = PhCompareStringWithNullSortOrder(node1->Field, node2->Field, context->TreeNewSortOrder, TRUE); \
} \
END_SORT_FUNCTION

STRING_SORT_FUNCTION(Protection, ProtectionString)
STRING_SORT_FUNCTION(MemoryType, MemoryTypeString)
STRING_SORT_FUNCTION(StackValid, StackValidString)
STRING_SORT_FUNCTION(UnwindMethod, UnwindMethodString)
STRING_SORT_FUNCTION(CfgTarget, CfgTargetString)
STRING_SORT_FUNCTION(Language, LanguageString)

VOID ThreadStackLoadSettingsTreeList(
    _Inout_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhGetStringSetting(SETTING_THREAD_STACK_TREE_LIST_COLUMNS);
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID ThreadStackSaveSettingsTreeList(
    _Inout_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_THREAD_STACK_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);
}

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN ThreadStackNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_STACK_TREE_ROOT_NODE node1 = *(PPH_STACK_TREE_ROOT_NODE *)Entry1;
    PPH_STACK_TREE_ROOT_NODE node2 = *(PPH_STACK_TREE_ROOT_NODE *)Entry2;

    return node1->Index == node2->Index;
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG ThreadStackNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashInt32((*(PPH_STACK_TREE_ROOT_NODE*)Entry)->Index);
}

VOID DestroyThreadStackNode(
    _In_ PPH_STACK_TREE_ROOT_NODE Node
    )
{
    if (Node->TooltipText)
        PhDereferenceObject(Node->TooltipText);
    if (Node->IndexString)
        PhDereferenceObject(Node->IndexString);
    if (Node->FrameDistanceString)
        PhDereferenceObject(Node->FrameDistanceString);
    PhDereferenceObject(Node);
}

PPH_STACK_TREE_ROOT_NODE AddThreadStackNode(
    _Inout_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ ULONG Index
    )
{
    PPH_STACK_TREE_ROOT_NODE threadStackNode;

    threadStackNode = PhCreateAlloc(sizeof(PH_STACK_TREE_ROOT_NODE));
    memset(threadStackNode, 0, sizeof(PH_STACK_TREE_ROOT_NODE));

    PhInitializeTreeNewNode(&threadStackNode->Node);

    memset(threadStackNode->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);
    threadStackNode->Node.TextCache = threadStackNode->TextCache;
    threadStackNode->Node.TextCacheSize = TREE_COLUMN_ITEM_MAXIMUM;

    threadStackNode->Index = Index;

    PhAddEntryHashtable(Context->NodeHashtable, &threadStackNode);
    PhAddItemList(Context->NodeList, threadStackNode);

    // TreeNew_NodesStructured(Context->TreeNewHandle);

    return threadStackNode;
}

PPH_STACK_TREE_ROOT_NODE FindThreadStackNode(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ ULONG Index
    )
{
    PH_STACK_TREE_ROOT_NODE lookupThreadStackNode;
    PPH_STACK_TREE_ROOT_NODE lookupThreadStackNodePtr = &lookupThreadStackNode;
    PPH_STACK_TREE_ROOT_NODE *threadStackNode;

    lookupThreadStackNode.Index = Index;

    threadStackNode = (PPH_STACK_TREE_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupThreadStackNodePtr
        );

    if (threadStackNode)
        return *threadStackNode;
    else
        return NULL;
}

VOID RemoveThreadStackNode(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ PPH_STACK_TREE_ROOT_NODE Node
)
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    DestroyThreadStackNode(Node);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID UpdateThreadStackNode(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ PPH_STACK_TREE_ROOT_NODE Node
    )
{
    memset(Node->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&Node->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

BOOLEAN NTAPI ThreadStackTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_THREAD_STACK_CONTEXT context = Context;
    PPH_STACK_TREE_ROOT_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPH_STACK_TREE_ROOT_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static CONST _CoreCrtSecureSearchSortCompareFunction sortFunctions[] =
                {
                    SORT_FUNCTION(Index),
                    SORT_FUNCTION(Symbol),
                    SORT_FUNCTION(StackAddress),
                    SORT_FUNCTION(FrameAddress),
                    SORT_FUNCTION(StackParameter1),
                    SORT_FUNCTION(StackParameter2),
                    SORT_FUNCTION(StackParameter3),
                    SORT_FUNCTION(StackParameter4),
                    SORT_FUNCTION(ControlAddress),
                    SORT_FUNCTION(ReturnAddress),
                    SORT_FUNCTION(FileName),
                    SORT_FUNCTION(LineText),
                    SORT_FUNCTION(Architecture),
                    SORT_FUNCTION(FrameDistance),
                    SORT_FUNCTION(Protection),
                    SORT_FUNCTION(MemoryType),
                    SORT_FUNCTION(StackValid),
                    SORT_FUNCTION(UnwindMethod),
                    SORT_FUNCTION(CfgTarget),
                    SORT_FUNCTION(Language),
                };
                _CoreCrtSecureSearchSortCompareFunction sortFunction;

                static_assert(RTL_NUMBER_OF(sortFunctions) == TREE_COLUMN_ITEM_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < TREE_COLUMN_ITEM_MAXIMUM)
                    sortFunction = sortFunctions[context->TreeNewSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                }

                getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;
            node = (PPH_STACK_TREE_ROOT_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPH_STACK_TREE_ROOT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PH_STACK_TREE_COLUMN_INDEX:
                {
                    PhMoveReference(&node->IndexString, PhFormatUInt64(node->Index, TRUE));
                    getCellText->Text = PhGetStringRef(node->IndexString);
                }
                break;
            case PH_STACK_TREE_COLUMN_SYMBOL:
                getCellText->Text = PhGetStringRef(node->SymbolString);
                break;
            case PH_STACK_TREE_COLUMN_STACKADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->StackAddressString);
                break;
            case PH_STACK_TREE_COLUMN_FRAMEADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->FrameAddressString);
                break;
            case PH_STACK_TREE_COLUMN_PARAMETER1:
                PhInitializeStringRefLongHint(&getCellText->Text, node->Parameter1String);
                break;
            case PH_STACK_TREE_COLUMN_PARAMETER2:
                PhInitializeStringRefLongHint(&getCellText->Text, node->Parameter2String);
                break;
            case PH_STACK_TREE_COLUMN_PARAMETER3:
                PhInitializeStringRefLongHint(&getCellText->Text, node->Parameter3String);
                break;
            case PH_STACK_TREE_COLUMN_PARAMETER4:
                PhInitializeStringRefLongHint(&getCellText->Text, node->Parameter4String);
                break;
            case PH_STACK_TREE_COLUMN_CONTROLADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->PcAddressString);
                break;
            case PH_STACK_TREE_COLUMN_RETURNADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->ReturnAddressString);
                break;
            case PH_STACK_TREE_COLUMN_FILENAME:
                getCellText->Text = PhGetStringRef(node->FileNameString);
                break;
            case PH_STACK_TREE_COLUMN_LINETEXT:
                getCellText->Text = PhGetStringRef(node->LineTextString);
                break;
            case PH_STACK_TREE_COLUMN_ARCHITECTURE:
                getCellText->Text = node->Architecture;
                break;
            case PH_STACK_TREE_COLUMN_FRAMEDISTANCE:
                {
                    if (node->FrameDistance)
                        PhMoveReference(&node->FrameDistanceString, PhFormatSize(node->FrameDistance, ULONG_MAX));
                    getCellText->Text = PhGetStringRef(node->FrameDistanceString);
                }
                break;
            case PH_STACK_TREE_COLUMN_PROTECTION:
                getCellText->Text = PhGetStringRef(node->ProtectionString);
                break;
            case PH_STACK_TREE_COLUMN_MEMORYTYPE:
                getCellText->Text = PhGetStringRef(node->MemoryTypeString);
                break;
            case PH_STACK_TREE_COLUMN_STACKVALID:
                getCellText->Text = PhGetStringRef(node->StackValidString);
                break;
            case PH_STACK_TREE_COLUMN_UNWINDMETHOD:
                getCellText->Text = PhGetStringRef(node->UnwindMethodString);
                break;
            case PH_STACK_TREE_COLUMN_CFGTARGET:
                getCellText->Text = PhGetStringRef(node->CfgTargetString);
                break;
            case PH_STACK_TREE_COLUMN_LANGUAGE:
                getCellText->Text = PhGetStringRef(node->LanguageString);
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
            node = (PPH_STACK_TREE_ROOT_NODE)getNodeColor->Node;

            if (context->HighlightInlineFrames && PhIsStackFrameTypeInline(node->StackFrame.InlineFrameContext))
            {
                getNodeColor->BackColor = PhGetIntegerSetting(SETTING_COLOR_INLINE_THREAD_STACK);
            }
            else if (context->HighlightSystemPages && (ULONG_PTR)node->StackFrame.PcAddress > PhSystemBasicInformation.MaximumUserModeAddress)
            {
                getNodeColor->BackColor = PhGetIntegerSetting(SETTING_COLOR_SYSTEM_THREAD_STACK);
            }
            else if (context->HighlightUserPages && (ULONG_PTR)node->StackFrame.PcAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
            {
                getNodeColor->BackColor = PhGetIntegerSetting(SETTING_COLOR_USER_THREAD_STACK);
            }

            getNodeColor->Flags = TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->TreeNewSortColumn = sorting->SortColumn;
            context->TreeNewSortOrder = sorting->SortOrder;

            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(WindowHandle);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(
                context->WindowHandle,
                WM_COMMAND,
                WM_PH_SHOWSTACKMENU,
                (LPARAM)contextMenuEvent
                );
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case VK_F5:
                SendMessage(context->WindowHandle, WM_COMMAND, IDC_REFRESH, 0);
                break;
            case VK_RETURN:
                SendMessage(context->WindowHandle, WM_COMMAND, WM_PH_SHOWSTACKDEFAULT, 0);
                break;
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->WindowHandle, WM_COMMAND, IDC_COPY, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(context->WindowHandle, WM_COMMAND, WM_PH_SHOWSTACKDEFAULT, 0);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = WindowHandle;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(
                data.Menu,
                WindowHandle,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                data.MouseEvent->ScreenLocation.x,
                data.MouseEvent->ScreenLocation.y
                );

            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewGetCellTooltip:
        {
            PPH_TREENEW_GET_CELL_TOOLTIP getCellTooltip = Parameter1;
            node = (PPH_STACK_TREE_ROOT_NODE)getCellTooltip->Node;

            if (getCellTooltip->Column->Id != 0)
                return FALSE;

            if (!node->TooltipText)
            {
                PH_STRING_BUILDER stringBuilder;
                PPH_STRING fileName;
                PH_SYMBOL_LINE_INFORMATION lineInfo;

                PhInitializeStringBuilder(&stringBuilder, 40);

                if (PhGetLineFromAddress(
                    context->SymbolProvider,
                    node->StackFrame.PcAddress,
                    &fileName,
                    NULL,
                    &lineInfo
                    ))
                {
                    PH_FORMAT format[5];
                    SIZE_T returnLength;
                    WCHAR buffer[0x100];

                    PhMoveReference(&fileName, PhGetFileName(fileName));

                    // File: %s: line %lu\n
                    PhInitFormatS(&format[0], L"File: ");
                    PhInitFormatSR(&format[1], fileName->sr);
                    PhInitFormatS(&format[2], L": line ");
                    PhInitFormatU(&format[3], lineInfo.LineNumber);
                    PhInitFormatS(&format[4], L"\n");

                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), &returnLength))
                    {
                        PhAppendStringBuilderEx(&stringBuilder, buffer, returnLength - sizeof(UNICODE_NULL));
                    }
                    else
                    {
                        PhAppendFormatStringBuilder(
                            &stringBuilder,
                            L"File: %s: line %lu\n",
                            fileName->Buffer,
                            lineInfo.LineNumber
                            );
                    }

                    PhDereferenceObject(fileName);

                    {
                        PPH_STRING sourceLinkUrl;

                        if (PhpGetThreadStackSourceLinkUrl(context, node->StackFrame.PcAddress, &sourceLinkUrl))
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"Source: ");
                            PhAppendStringBuilder(&stringBuilder, &sourceLinkUrl->sr);
                            PhAppendCharStringBuilder(&stringBuilder, L'\n');
                            PhDereferenceObject(sourceLinkUrl);
                        }
                    }
                }

                if (stringBuilder.String->Length != 0)
                    PhRemoveEndStringBuilder(&stringBuilder, 1);

                if (PhPluginsEnabled)
                {
                    PH_PLUGIN_THREAD_STACK_CONTROL control;

                    control.Type = PluginThreadStackGetTooltip;
                    control.UniqueKey = context;
                    control.u.GetTooltip.StackFrame = &node->StackFrame;
                    control.u.GetTooltip.StringBuilder = &stringBuilder;
                    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
                }

                node->TooltipText = PhFinalStringBuilderString(&stringBuilder);
            }

            if (!PhIsNullOrEmptyString(node->TooltipText))
            {
                getCellTooltip->Text = node->TooltipText->sr;
                getCellTooltip->Unfolding = FALSE;
                getCellTooltip->Font = NULL; // use default font
                getCellTooltip->MaximumWidth = 550;
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case TreeNewGetDialogCode:
        {
            PULONG code = Parameter2;

            // Enter opens the frame registers instead of pressing the Close button.
            if (PtrToUlong(Parameter1) == VK_F5 || PtrToUlong(Parameter1) == VK_RETURN)
            {
                *code = DLGC_WANTMESSAGE;
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

VOID ClearThreadStackTree(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        DestroyThreadStackNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPH_STACK_TREE_ROOT_NODE GetSelectedThreadStackNode(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    PPH_STACK_TREE_ROOT_NODE stackNode = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        stackNode = Context->NodeList->Items[i];

        if (stackNode->Node.Selected)
            return stackNode;
    }

    return NULL;
}

_Success_(return)
BOOLEAN GetSelectedThreadStackNodes(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _Out_ PPH_STACK_TREE_ROOT_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_STACK_TREE_ROOT_NODE node = (PPH_STACK_TREE_ROOT_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    PhDereferenceObject(list);
    return FALSE;
}

_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN PhpThreadStackTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_ PVOID Context
    )
{
    PPH_THREAD_STACK_CONTEXT stackContext = Context;
    PPH_STACK_TREE_ROOT_NODE stackNode = (PPH_STACK_TREE_ROOT_NODE)Node;

    if (stackContext->HideSystemPages && (ULONG_PTR)stackNode->StackFrame.PcAddress > PhSystemBasicInformation.MaximumUserModeAddress)
        return FALSE;
    if (stackContext->HideUserPages && (ULONG_PTR)stackNode->StackFrame.PcAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
        return FALSE;
    if (stackContext->HideInlineFrames && PhIsStackFrameTypeInline(stackNode->StackFrame.InlineFrameContext))
        return FALSE;

    if (stackContext->SearchMatchHandle)
    {
        if (!PhIsNullOrEmptyString(stackNode->IndexString) && PhSearchControlMatch(stackContext->SearchMatchHandle, &stackNode->IndexString->sr))
            return TRUE;
        if (!PhIsNullOrEmptyString(stackNode->SymbolString) && PhSearchControlMatch(stackContext->SearchMatchHandle, &stackNode->SymbolString->sr))
            return TRUE;
        if (!PhIsNullOrEmptyString(stackNode->FileNameString) && PhSearchControlMatch(stackContext->SearchMatchHandle, &stackNode->FileNameString->sr))
            return TRUE;
        if (!PhIsNullOrEmptyString(stackNode->LineTextString) && PhSearchControlMatch(stackContext->SearchMatchHandle, &stackNode->LineTextString->sr))
            return TRUE;
        if (!PhIsNullOrEmptyString(stackNode->LanguageString) && PhSearchControlMatch(stackContext->SearchMatchHandle, &stackNode->LanguageString->sr))
            return TRUE;
        if (stackNode->PcAddressString[0] && PhSearchControlMatchZ(stackContext->SearchMatchHandle, stackNode->PcAddressString))
            return TRUE;
        if (stackNode->ReturnAddressString[0] && PhSearchControlMatchZ(stackContext->SearchMatchHandle, stackNode->ReturnAddressString))
            return TRUE;
        if (stackNode->StackAddressString[0] && PhSearchControlMatchZ(stackContext->SearchMatchHandle, stackNode->StackAddressString))
            return TRUE;
        if (stackNode->FrameAddressString[0] && PhSearchControlMatchZ(stackContext->SearchMatchHandle, stackNode->FrameAddressString))
            return TRUE;

        return FALSE;
    }

    return TRUE;
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
static VOID NTAPI PhpThreadStackSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPH_THREAD_STACK_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    PhApplyTreeNewFilters(&context->TreeFilterSupport);
}

VOID InitializeThreadStackTree(
    _Inout_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    Context->NodeList = PhCreateList(100);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPH_STACK_TREE_ROOT_NODE),
        ThreadStackNodeHashtableEqualFunction,
        ThreadStackNodeHashtableHashFunction,
        100
        );

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);
    TreeNew_SetCallback(Context->TreeNewHandle, ThreadStackTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_INDEX, TRUE, L"#", 30, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_SYMBOL, TRUE, L"Name", 250, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_STACKADDRESS, FALSE, L"Stack address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_FRAMEADDRESS, FALSE, L"Frame address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER1, FALSE, L"Stack parameter #1", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER2, FALSE, L"Stack parameter #2", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER3, FALSE, L"Stack parameter #3", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER4, FALSE, L"Stack parameter #4", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_CONTROLADDRESS, FALSE, L"Control address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_RETURNADDRESS, FALSE, L"Return address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_FILENAME, FALSE, L"File name", 100, PH_ALIGN_LEFT, ULONG_MAX, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_LINETEXT, FALSE, L"Line number", 100, PH_ALIGN_LEFT, ULONG_MAX, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_ARCHITECTURE, FALSE, L"Architecture", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_FRAMEDISTANCE, FALSE, L"Frame distance", 100, PH_ALIGN_RIGHT, ULONG_MAX, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PROTECTION, TRUE, L"Protection", 90, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_MEMORYTYPE, TRUE, L"Type", 90, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_STACKVALID, TRUE, L"Stack valid", 90, PH_ALIGN_LEFT, 4, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_UNWINDMETHOD, TRUE, L"Unwind", 120, PH_ALIGN_LEFT, 5, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_CFGTARGET, TRUE, L"CFG target", 90, PH_ALIGN_LEFT, 6, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_LANGUAGE, TRUE, L"Language", 90, PH_ALIGN_LEFT, 7, 0);

    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, Context->TreeNewHandle, Context->NodeList);
    Context->TreeFilterEntry = PhAddTreeNewFilter(&Context->TreeFilterSupport, PhpThreadStackTreeFilterCallback, Context);

    TreeNew_SetSort(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_INDEX, AscendingSortOrder);
    TreeNew_SetTriState(Context->TreeNewHandle, FALSE);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    ThreadStackLoadSettingsTreeList(Context);
}

VOID DeleteThreadStackTree(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    PhRemoveTreeNewFilter(&Context->TreeFilterSupport, Context->TreeFilterEntry);
    PhDeleteTreeNewFilterSupport(&Context->TreeFilterSupport);

    ThreadStackSaveSettingsTreeList(Context);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        DestroyThreadStackNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpThreadStackContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_THREAD_STACK_CONTEXT context = (PPH_THREAD_STACK_CONTEXT)Object;
    PSLIST_ENTRY listEntry;

    listEntry = RtlInterlockedFlushSList(&context->ResolvedListHead);

    while (listEntry)
    {
        PTHREAD_STACK_RESOLVED_ITEM resolvedItem = CONTAINING_RECORD(listEntry, THREAD_STACK_RESOLVED_ITEM, ListEntry);

        listEntry = listEntry->Next;
        PhpFreeThreadStackResolvedItem(resolvedItem);
    }

    if (context->StatusMessage) PhDereferenceObject(context->StatusMessage);
    if (context->StatusContent) PhDereferenceObject(context->StatusContent);
    if (context->NewList)
    {
        for (ULONG i = 0; i < context->NewList->Count; i++)
            PhpFreeThreadStackItem(context->NewList->Items[i]);

        PhDereferenceObject(context->NewList);
    }

    if (context->List)
    {
        for (ULONG i = 0; i < context->List->Count; i++)
            PhpFreeThreadStackItem(context->List->Items[i]);

        PhDereferenceObject(context->List);
    }

    if (context->ThreadHandle)
        NtClose(context->ThreadHandle);

    if (context->WorkerCompletedEvent)
        NtClose(context->WorkerCompletedEvent);

    if (context->ThreadProvider)
        PhDereferenceObject(context->ThreadProvider);
}

_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS PhpThreadStackDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    HWND windowHandle;
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    // No owner window: the dialog runs on its own thread so it stays responsive and
    // outlives the process properties window that opened it.
    windowHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_THRDSTACK),
        NULL,
        PhpThreadStackDlgProc,
        Parameter
        );

    // WM_INITDIALOG can destroy the window (walk cancelled or failed); WM_DESTROY has
    // already released the context in that case.
    if (windowHandle)
    {
        ShowWindow(windowHandle, SW_SHOW);
        SetForegroundWindow(windowHandle);

        while (result = GetMessage(&message, NULL, 0, 0))
        {
            if (result == INT_ERROR)
                break;

            if (!IsDialogMessage(windowHandle, &message))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }

            PhDrainAutoPool(&autoPool);
        }
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID PhShowThreadStackDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    NTSTATUS status;
    HANDLE threadHandle;
    PPH_THREAD_STACK_CONTEXT context;

    // If the user is trying to view a system thread stack
    // but KSystemInformer is not loaded, show an error message.
    if (ProcessId == SYSTEM_PROCESS_ID && (KsiLevel() < KphLevelMed))
    {
        PhShowKsiNotConnected(
            ParentWindowHandle,
            L"Inspecting kernel stacks requires a connection to the kernel driver."
            );
        return;
    }

    // The idle process has pseudo CIDs (dmex)
    if (ProcessId == SYSTEM_IDLE_PROCESS_ID &&
        HandleToUlong(ThreadId) < PhSystemProcessorInformation.NumberOfProcessors)
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the thread.", STATUS_UNSUCCESSFUL, 0);
        return;
    }

    if (!NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
        ThreadId
        )))
    {
        status = PhOpenThread(
            &threadHandle,
            THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT,
            ThreadId
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the thread.", status, 0);
        return;
    }

    if (PhBeginInitOnce(&initOnce))
    {
        PhThreadStackContextType = PhCreateObjectType(L"ThreadStackContext", 0, PhpThreadStackContextDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    context = PhCreateObjectZero(sizeof(PH_THREAD_STACK_CONTEXT), PhThreadStackContextType);
    context->List = PhCreateList(10);
    context->NewList = PhCreateList(10);
    PhInitializeQueuedLock(&context->StatusLock);
    PhInitializeSListHead(&context->ResolvedListHead);
    // Manual-reset event, initially signalled (no worker running).
    PhCreateEvent(&context->WorkerCompletedEvent, EVENT_ALL_ACCESS, NotificationEvent, TRUE);
    context->DeferSymbols = !!PhGetIntegerSetting(SETTING_ENABLE_THREAD_STACK_DEFERRED_SYMBOLS);

    context->ParentHandle = ParentWindowHandle;
    context->ThreadHandle = threadHandle;
    context->ProcessId = ProcessId;
    context->ThreadId = ThreadId;
    context->ThreadProvider = PhReferenceObject(ThreadProvider); // The caller's properties window may close first.
    context->SymbolProvider = ThreadProvider->SymbolProvider;

#if defined(_WIN64)
    if (context->SymbolProvider->ProcessHandle)
        PhGetProcessIsWow64(context->SymbolProvider->ProcessHandle, &context->IsWow64Process);
#endif

    if (!NT_SUCCESS(status = PhCreateThread2(PhpThreadStackDialogThreadStart, context)))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to create the window.", status, 0);
        PhDereferenceObject(context);
    }
}

VOID PhShowThreadStackFrameRegisters(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ PPH_STACK_TREE_ROOT_NODE Node
    )
{
    PH_FORMAT format[4];
    ULONG count = 2;
    PPH_STRING frameTitle;

    if (!Node->FrameContext)
        return;

    PhInitFormatC(&format[0], L'#');
    PhInitFormatU(&format[1], Node->Index);

    if (!PhIsNullOrEmptyString(Node->SymbolString))
    {
        PhInitFormatC(&format[2], L' ');
        PhInitFormatSR(&format[3], Node->SymbolString->sr);
        count = 4;
    }

    frameTitle = PhFormat(format, count, 0);

    PhShowThreadFrameContextDialog(
        WindowHandle,
        Context->ProcessId,
        Context->ThreadId,
        Node->StackFrame.Machine,
        Node->FrameContext,
        PhGetString(frameTitle)
        );

    PhDereferenceObject(frameTitle);
}

INT_PTR CALLBACK PhpThreadStackDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_THREAD_STACK_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_THREAD_STACK_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status;

            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);
            context->SearchboxHandle = GetDlgItem(hwndDlg, IDC_SEARCH);
            context->StatusHandle = GetDlgItem(hwndDlg, IDC_MESSAGE);
            context->HighlightUserPages = !!PhGetIntegerSetting(SETTING_USE_COLOR_USER_THREAD_STACK);
            context->HighlightSystemPages = !!PhGetIntegerSetting(SETTING_USE_COLOR_SYSTEM_THREAD_STACK);
            context->HighlightInlineFrames = !!PhGetIntegerSetting(SETTING_USE_COLOR_INLINE_THREAD_STACK);
            context->WindowDpi = PhGetWindowDpi(hwndDlg);
            PhSetWindowExStyle(context->TreeNewHandle, WS_EX_CLIENTEDGE, 0);

            PhSetApplicationWindowIcon(hwndDlg);

            PhSetWindowText(hwndDlg, PhaFormatString(L"Stack - thread %lu", HandleToUlong(context->ThreadId))->Buffer);

            InitializeThreadStackTree(context);

            if (PhTreeWindowFont)
            {
                context->TreeNewFont = PhCreateTreeWindowFont(PhGetWindowDpi(hwndDlg));
                SetWindowFont(context->TreeNewHandle, context->TreeNewFont, FALSE);
            }

            PhCreateSearchControl(
                hwndDlg,
                context->SearchboxHandle,
                L"Search stack frames",
                PhpThreadStackSearchControlCallback,
                context
                );

            PhSetWindowText(context->StatusHandle, L"");

            if (context->DeferSymbols)
            {
                // The legacy mode registers per task dialog; the deferred mode reports progress in the status label.
                PhRegisterCallback(
                    &PhSymbolEventCallback,
                    PhpSymbolProviderEventCallbackHandler,
                    context,
                    &context->DeferredSymbolEventRegistration
                    );
            }

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_OPTIONS), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP);
            PhAddLayoutItem(&context->LayoutManager, context->StatusHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->SearchboxHandle, NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_COPY), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 190;
                rect.bottom = 120;
                MapDialogRect(hwndDlg, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }

            if (PhValidWindowPlacementFromSetting(SETTING_THREAD_STACK_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_THREAD_STACK_WINDOW_POSITION, SETTING_THREAD_STACK_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentHandle);
            PhSetDialogFocus(hwndDlg, context->TreeNewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_THREAD_STACK_CONTROL control;

                control.Type = PluginThreadStackInitializing;
                control.UniqueKey = context;
                control.u.Initializing.ProcessId = context->ProcessId;
                control.u.Initializing.ThreadId = context->ThreadId;
                control.u.Initializing.ThreadHandle = context->ThreadHandle;
                control.u.Initializing.ProcessHandle = context->SymbolProvider->ProcessHandle;
                control.u.Initializing.SymbolProvider = context->SymbolProvider;
                control.u.Initializing.CustomWalk = FALSE;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

                context->CustomWalk = control.u.Initializing.CustomWalk;
            }

            status = PhpRefreshThreadStack(hwndDlg, context);

            if (status == STATUS_ABANDONED)
                DestroyWindow(hwndDlg);
            else if (!NT_SUCCESS(status))
            {
                // The window has no owner; show the error unowned before closing it.
                PhShowStatus(NULL, L"Unable to load the stack.", status, 0);
                DestroyWindow(hwndDlg);
            }
        }
        break;
    case WM_DESTROY:
        {
            WriteBooleanRelease(&context->StopWalk, TRUE);

            if (context->DeferSymbols)
                PhUnregisterCallback(&PhSymbolEventCallback, &context->DeferredSymbolEventRegistration);

            DeleteThreadStackTree(context);

            if (context->SourceLinkCache)
            {
                PH_HASHTABLE_ENUM_CONTEXT enumContext;
                PPH_KEY_VALUE_PAIR entry;

                PhBeginEnumHashtable(context->SourceLinkCache, &enumContext);

                while (entry = PhNextEnumHashtable(&enumContext))
                {
                    if (entry->Value)
                        PhDereferenceObject(entry->Value);
                }

                PhDereferenceObject(context->SourceLinkCache);
                context->SourceLinkCache = NULL;
            }

            if (context->TreeNewFont)
                DeleteFont(context->TreeNewFont);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_THREAD_STACK_CONTROL control;
                control.Type = PluginThreadStackUninitializing;
                control.UniqueKey = context;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
            }

            PhSaveWindowPlacementToSetting(SETTING_THREAD_STACK_WINDOW_POSITION, SETTING_THREAD_STACK_WINDOW_SIZE, hwndDlg);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhDereferenceObject(context);

            PostQuitMessage(0);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_REFRESH:
                {
                    NTSTATUS status;

                    if (!NT_SUCCESS(status = PhpRefreshThreadStack(hwndDlg, context)))
                    {
                        PhShowStatus(hwndDlg, L"Unable to refresh the stack.", status, 0);
                    }
                }
                break;
            case WM_PH_SHOWSTACKMENU:
                {
                    PPH_EMENU menu;
                    PPH_STACK_TREE_ROOT_NODE selectedNode;
                    PPH_EMENU_ITEM selectedItem;
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;

                    if (selectedNode = GetSelectedThreadStackNode(context))
                    {
                        PPH_STRING sourceLinkUrl = NULL;

                        PhpGetThreadStackSourceLinkUrl(context, selectedNode->StackFrame.PcAddress, &sourceLinkUrl);

                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_DEFAULT | (selectedNode->FrameContext ? 0 : PH_EMENU_DISABLED), PH_THREAD_STACK_MENUITEM_REGISTERS, L"&Registers...", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_THREAD_STACK_MENUITEM_INSPECT, L"&Inspect", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_THREAD_STACK_MENUITEM_OPENFILELOCATION, L"Open &file location", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(sourceLinkUrl ? 0 : PH_EMENU_DISABLED, PH_THREAD_STACK_MENUITEM_OPENSOURCELINK, L"Open &source link", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(sourceLinkUrl ? 0 : PH_EMENU_DISABLED, PH_THREAD_STACK_MENUITEM_COPYSOURCELINK, L"Copy source &link", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"Copy", NULL, NULL), ULONG_MAX);
                        PhInsertCopyCellEMenuItem(menu, IDC_COPY, context->TreeNewHandle, contextMenuEvent->Column);

                        selectedItem = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            contextMenuEvent->Location.x,
                            contextMenuEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
                        {
                            if (PhHandleCopyCellEMenuItem(selectedItem))
                                NOTHING;
                            else switch (selectedItem->Id)
                            {
                            case PH_THREAD_STACK_MENUITEM_INSPECT:
                                {
                                    if (!PhIsNullOrEmptyString(selectedNode->FileNameString) && PhDoesFileExistWin32(PhGetString(selectedNode->FileNameString)))
                                    {
                                        PhShellExecuteUserString(
                                            hwndDlg,
                                            SETTING_PROGRAM_INSPECT_EXECUTABLES,
                                            PhGetString(selectedNode->FileNameString),
                                            FALSE,
                                            L"Make sure the PE Viewer executable file is present."
                                            );
                                    }
                                }
                                break;
                            case PH_THREAD_STACK_MENUITEM_OPENFILELOCATION:
                                {
                                    if (!PhIsNullOrEmptyString(selectedNode->FileNameString) && PhDoesFileExistWin32(PhGetString(selectedNode->FileNameString)))
                                    {
                                        PhShellExecuteUserString(
                                            hwndDlg,
                                            SETTING_FILE_BROWSE_EXECUTABLE,
                                            PhGetString(selectedNode->FileNameString),
                                            FALSE,
                                            L"Make sure the Explorer executable file is present."
                                            );
                                    }
                                }
                                break;
                            case PH_THREAD_STACK_MENUITEM_REGISTERS:
                                {
                                    PhShowThreadStackFrameRegisters(hwndDlg, context, selectedNode);
                                }
                                break;
                            case PH_THREAD_STACK_MENUITEM_OPENSOURCELINK:
                                {
                                    if (sourceLinkUrl)
                                    {
                                        PhShellExecute(hwndDlg, PhGetString(sourceLinkUrl), NULL);
                                    }
                                }
                                break;
                            case PH_THREAD_STACK_MENUITEM_COPYSOURCELINK:
                                {
                                    if (sourceLinkUrl)
                                    {
                                        PhSetClipboardString(context->TreeNewHandle, &sourceLinkUrl->sr);
                                    }
                                }
                                break;
                            case IDC_COPY:
                                {
                                    PPH_STRING text;

                                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                                    PhSetClipboardString(context->TreeNewHandle, &text->sr);
                                    PhDereferenceObject(text);
                                }
                                break;
                            }
                        }

                        PhDestroyEMenu(menu);

                        if (sourceLinkUrl)
                            PhDereferenceObject(sourceLinkUrl);
                    }
                }
                break;
            case WM_PH_SHOWSTACKDEFAULT:
                {
                    PPH_STACK_TREE_ROOT_NODE selectedNode;

                    // Double click / Enter open the frame registers (the default menu item).
                    if (selectedNode = GetSelectedThreadStackNode(context))
                    {
                        PhShowThreadStackFrameRegisters(hwndDlg, context, selectedNode);
                    }
                }
                break;
            case IDC_OPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM hideUserItem;
                    PPH_EMENU_ITEM hideSystemItem;
                    PPH_EMENU_ITEM hideInlineItem;
                    PPH_EMENU_ITEM userItem;
                    PPH_EMENU_ITEM systemItem;
                    PPH_EMENU_ITEM inlineItem;
                    PPH_EMENU_ITEM selectedItem;

                    if (!PhGetWindowRect(GET_WM_COMMAND_HWND(wParam, lParam), &rect))
                        break;

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, hideUserItem = PhCreateEMenuItem(0, 1, L"Hide user frames", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, hideSystemItem = PhCreateEMenuItem(0, 2, L"Hide system frames", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, hideInlineItem = PhCreateEMenuItem(0, 3, L"Hide inline frames", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, userItem = PhCreateEMenuItem(0, 4, L"Highlight user frames", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, systemItem = PhCreateEMenuItem(0, 5, L"Highlight system frames", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, inlineItem = PhCreateEMenuItem(0, 6, L"Highlight inline frames", NULL, NULL), ULONG_MAX);

                    if (context->HideUserPages)
                        hideUserItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideSystemPages)
                        hideSystemItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideInlineFrames)
                        hideInlineItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightUserPages)
                        userItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightSystemPages)
                        systemItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightInlineFrames)
                        inlineItem->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        GET_WM_COMMAND_HWND(wParam, lParam),
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id)
                    {
                        if (selectedItem->Id == 1)
                        {
                            context->HideUserPages = !context->HideUserPages;
                        }
                        else if (selectedItem->Id == 2)
                        {
                            context->HideSystemPages = !context->HideSystemPages;
                        }
                        else if (selectedItem->Id == 3)
                        {
                            context->HideInlineFrames = !context->HideInlineFrames;
                        }
                        else if (selectedItem->Id == 4)
                        {
                            context->HighlightUserPages = !context->HighlightUserPages;
                            PhSetIntegerSetting(SETTING_USE_COLOR_USER_THREAD_STACK, context->HighlightUserPages);
                        }
                        else if (selectedItem->Id == 5)
                        {
                            context->HighlightSystemPages = !context->HighlightSystemPages;
                            PhSetIntegerSetting(SETTING_USE_COLOR_SYSTEM_THREAD_STACK, context->HighlightSystemPages);
                        }
                        else if (selectedItem->Id == 6)
                        {
                            context->HighlightInlineFrames = !context->HighlightInlineFrames;
                            PhSetIntegerSetting(SETTING_USE_COLOR_INLINE_THREAD_STACK, context->HighlightInlineFrames);
                        }

                        PhApplyTreeNewFilters(&context->TreeFilterSupport);
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            case IDC_COPY:
                {
                    PPH_STRING text;

                    TreeNew_SelectRange(context->TreeNewHandle, 0, -1);
                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    TreeNew_DeselectRange(context->TreeNewHandle, 0, -1);

                    PhSetClipboardString(context->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            }
        }
        break;
    case WM_PH_FRAMES_READY:
        {
            // wParam: 0 = success, install frames; non-zero = walk failed/cancelled, discard.
            if (wParam == 0)
            {
                PhpApplyThreadStackFrames(context);
            }
            else
            {
                for (ULONG i = 0; i < context->NewList->Count; i++)
                    PhpFreeThreadStackItem(context->NewList->Items[i]);
                PhClearList(context->NewList);
            }
        }
        return TRUE;
    case WM_PH_SYMBOL_RESOLVED:
        {
            PSLIST_ENTRY listEntry;
            PSLIST_ENTRY reversed = NULL;

            // The worker pushes results onto ResolvedListHead and posts this message whenever
            // the list transitions from empty. Flush everything and apply in push order.
            listEntry = RtlInterlockedFlushSList(&context->ResolvedListHead);

            while (listEntry)
            {
                PSLIST_ENTRY next = listEntry->Next;

                listEntry->Next = reversed;
                reversed = listEntry;
                listEntry = next;
            }

            while (reversed)
            {
                PTHREAD_STACK_RESOLVED_ITEM resolvedItem = CONTAINING_RECORD(reversed, THREAD_STACK_RESOLVED_ITEM, ListEntry);
                PPH_STACK_TREE_ROOT_NODE node;
                PTHREAD_STACK_ITEM item;

                reversed = reversed->Next;

                // Drop results from an older (cancelled) worker.
                if (
                    resolvedItem->Generation == context->WorkerGeneration &&
                    resolvedItem->Index < context->List->Count &&
                    (node = FindThreadStackNode(context, resolvedItem->Index))
                    )
                {
                    item = context->List->Items[resolvedItem->Index];

                    // Transfer ownership of the resolved strings to the item. The node borrows
                    // them, matching PhpApplyThreadStackFrames.
                    PhMoveReference(&item->Symbol, resolvedItem->Symbol);
                    PhMoveReference(&item->FileName, resolvedItem->FileName);
                    PhMoveReference(&item->LineText, resolvedItem->LineText);
                    resolvedItem->Symbol = NULL;
                    resolvedItem->FileName = NULL;
                    resolvedItem->LineText = NULL;

                    if (resolvedItem->Language)
                    {
                        PhMoveReference(&item->Language, resolvedItem->Language);
                        resolvedItem->Language = NULL;
                    }

                    node->SymbolString = item->Symbol;
                    node->FileNameString = item->FileName;
                    node->LineTextString = item->LineText;
                    node->LanguageString = item->Language;

                    UpdateThreadStackNode(context, node);
                }

                PhpFreeThreadStackResolvedItem(resolvedItem);
            }

            // Symbols changed; re-evaluate the search filter.
            if (context->SearchMatchHandle)
                PhApplyTreeNewFilters(&context->TreeFilterSupport);
        }
        return TRUE;
    case WM_PH_SYMBOL_STATUS:
        {
            PPH_STRING status = NULL;

            InterlockedExchange8((PCHAR)&context->StatusUpdatePending, FALSE);

            PhAcquireQueuedLockShared(&context->StatusLock);
            if (context->StatusContent)
                status = PhReferenceObject(context->StatusContent);
            PhReleaseQueuedLockShared(&context->StatusLock);

            PhSetWindowText(context->StatusHandle, PhGetStringOrEmpty(status));
            PhClearReference(&status);
        }
        return TRUE;
    case WM_PH_SYMBOLS_COMPLETE:
        {
            if ((ULONG)wParam == context->WorkerGeneration && !context->RefreshPending)
            {
                PhpSetThreadStackStatus(context, PhFormatString(
                    L"%lu frames, symbols loaded",
                    context->List->Count
                    ));
            }

            // A refresh requested while the worker was still running is started now that it
            // has exited, so two workers never share NewList or the diagnostics state.
            if (context->RefreshPending)
            {
                NTSTATUS status;

                context->RefreshPending = FALSE;

                if (!NT_SUCCESS(status = PhpRefreshThreadStack(hwndDlg, context)))
                    PhShowStatus(hwndDlg, L"Unable to load the stack.", status, 0);
            }
        }
        return TRUE;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);

            if (PhTreeWindowFont)
            {
                HFONT treeNewFont;

                if (treeNewFont = PhCreateTreeWindowFont(LOWORD(wParam)))
                    PhSwapReferenceFont(&context->TreeNewFont, context->TreeNewHandle, treeNewFont, TRUE);
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID PhpFreeThreadStackItem(
    _In_ PTHREAD_STACK_ITEM StackItem
    )
{
    if (StackItem->Symbol) PhDereferenceObject(StackItem->Symbol);
    if (StackItem->FileName) PhDereferenceObject(StackItem->FileName);
    if (StackItem->LineText) PhDereferenceObject(StackItem->LineText);
    if (StackItem->Protection) PhDereferenceObject(StackItem->Protection);
    if (StackItem->MemoryType) PhDereferenceObject(StackItem->MemoryType);
    if (StackItem->StackValid) PhDereferenceObject(StackItem->StackValid);
    if (StackItem->UnwindMethod) PhDereferenceObject(StackItem->UnwindMethod);
    if (StackItem->CfgTarget) PhDereferenceObject(StackItem->CfgTarget);
    if (StackItem->Language) PhDereferenceObject(StackItem->Language);
    if (StackItem->FrameContext) PhFree(StackItem->FrameContext);

    PhFree(StackItem);
}

static PPH_STRING PhpGetThreadStackLanguage(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID Address
    )
{
    PH_DIA_SYMBOL_INFORMATION symbolInformation;
    PPH_STRING language = NULL;

    memset(&symbolInformation, 0, sizeof(symbolInformation));

    if (Address && PhGetDiaSymbolInformation(SymbolProvider, Address, &symbolInformation))
    {
        language = symbolInformation.SymbolLangugage;
        symbolInformation.SymbolLangugage = NULL;
    }

    PhClearReference(&symbolInformation.UndecoratedName);
    PhClearReference(&symbolInformation.SymbolInformation);
    PhClearReference(&symbolInformation.SymbolLangugage);

    return language;
}

static VOID PhpInitializeThreadStackDiagnostics(
    _Inout_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    ULONG_PTR tebAddress;

    Context->CfgTargetContextValid = NT_SUCCESS(PhInitializeProcessCfgTargetContext(
        Context->SymbolProvider->ProcessHandle,
        &Context->CfgTargetContext
        ));
    Context->NativeTibValid = FALSE;
    Context->Wow64TibValid = FALSE;
    Context->KernelStackInformationValid = FALSE;

    if (NT_SUCCESS(PhGetThreadTeb(Context->ThreadHandle, &tebAddress)))
    {
        Context->NativeTibValid = NT_SUCCESS(PhReadVirtualMemory(
            Context->SymbolProvider->ProcessHandle,
            (PVOID)tebAddress,
            &Context->NativeTib,
            sizeof(Context->NativeTib),
            NULL
            ));
    }

    if (NT_SUCCESS(PhGetThreadTeb32(Context->ThreadHandle, &tebAddress)))
    {
        Context->Wow64TibValid = NT_SUCCESS(PhReadVirtualMemory(
            Context->SymbolProvider->ProcessHandle,
            (PVOID)tebAddress,
            &Context->Wow64Tib,
            sizeof(Context->Wow64Tib),
            NULL
            ));
    }

    if (KsiLevel() >= KphLevelMed)
    {
        Context->KernelStackInformationValid = NT_SUCCESS(KphQueryInformationThread(
            Context->ThreadHandle,
            KphThreadKernelStackInformation,
            &Context->KernelStackInformation,
            sizeof(Context->KernelStackInformation),
            NULL
            ));
    }
}

static VOID PhpAnalyzeThreadStackItem(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _Inout_ PTHREAD_STACK_ITEM Item
    )
{
    BOOLEAN kernelFrame;
    BOOLEAN wow64Frame;
    BOOLEAN stackBoundsAvailable;
    BOOLEAN stackValid = FALSE;
    MEMORY_BASIC_INFORMATION basicInfo;
    PH_CFG_TARGET_STATUS cfgStatus;

    kernelFrame = !!(StackFrame->Flags & PH_THREAD_STACK_FRAME_KERNEL) ||
        (ULONG_PTR)StackFrame->PcAddress > PhSystemBasicInformation.MaximumUserModeAddress;
    wow64Frame = StackFrame->Machine == IMAGE_FILE_MACHINE_I386 ||
        StackFrame->Machine == IMAGE_FILE_MACHINE_CHPE_X86;
    Item->DiagnosticAddress = StackFrame->PcAddress;

    if (kernelFrame)
    {
        Item->Protection = PhCreateString(L"N/A");
        Item->MemoryType = PhCreateString(L"N/A");
        Item->CfgTarget = PhCreateString(L"N/A");
        Item->Language = PhCreateString(L"N/A");
        stackBoundsAvailable = Context->KernelStackInformationValid;

        if (stackBoundsAvailable && StackFrame->StackAddress)
        {
            ULONG_PTR stackLow;
            ULONG_PTR stackHigh;

            stackLow = (ULONG_PTR)Context->KernelStackInformation.KernelStack;
            stackHigh = (ULONG_PTR)Context->KernelStackInformation.InitialStack;

            if (!stackLow)
                stackLow = (ULONG_PTR)Context->KernelStackInformation.StackLimit;
            if (!stackHigh)
                stackHigh = (ULONG_PTR)Context->KernelStackInformation.StackBase;

            stackValid = stackLow && stackHigh &&
                (ULONG_PTR)StackFrame->StackAddress >= stackLow &&
                (ULONG_PTR)StackFrame->StackAddress < stackHigh;
        }
    }
    else
    {
        WCHAR protection[16];
        if (NT_SUCCESS(NtQueryVirtualMemory(
            Context->SymbolProvider->ProcessHandle,
            StackFrame->PcAddress,
            MemoryBasicInformation,
            &basicInfo,
            sizeof(MEMORY_BASIC_INFORMATION),
            NULL
            )))
        {
            PhGetMemoryProtectionString(basicInfo.Protect, protection);
            Item->Protection = PhCreateString(protection);

            switch (basicInfo.Type)
            {
            case MEM_IMAGE:
                Item->MemoryType = PhCreateString(L"MEM_IMAGE");
                break;
            case MEM_MAPPED:
                Item->MemoryType = PhCreateString(L"MEM_MAPPED");
                break;
            case MEM_PRIVATE:
                Item->MemoryType = PhCreateString(L"MEM_PRIVATE");
                break;
            default:
                Item->MemoryType = PhCreateString(L"Unavailable");
                break;
            }
        }
        else
        {
            Item->Protection = PhCreateString(L"Unavailable");
            Item->MemoryType = PhCreateString(L"Unavailable");
        }

        stackBoundsAvailable = wow64Frame ? Context->Wow64TibValid : Context->NativeTibValid;

        if (stackBoundsAvailable && StackFrame->StackAddress)
        {
            if (wow64Frame)
            {
                stackValid = (ULONG_PTR)StackFrame->StackAddress >= Context->Wow64Tib.StackLimit &&
                    (ULONG_PTR)StackFrame->StackAddress < Context->Wow64Tib.StackBase;
            }
            else
            {
                stackValid = (ULONG_PTR)StackFrame->StackAddress >= (ULONG_PTR)Context->NativeTib.StackLimit &&
                    (ULONG_PTR)StackFrame->StackAddress < (ULONG_PTR)Context->NativeTib.StackBase;
            }
        }

        if (Context->CfgTargetContextValid)
        {
            cfgStatus = PhQueryProcessCfgTarget(
                Context->SymbolProvider->ProcessHandle,
                &Context->CfgTargetContext,
                StackFrame->PcAddress,
                wow64Frame
                );
        }
        else
        {
            cfgStatus = PhCfgTargetUnavailable;
        }

        switch (cfgStatus)
        {
        case PhCfgTargetValid:
            Item->CfgTarget = PhCreateString(L"Valid");
            break;
        case PhCfgTargetInvalid:
            Item->CfgTarget = PhCreateString(L"Invalid");
            break;
        case PhCfgTargetDisabled:
            Item->CfgTarget = PhCreateString(L"Disabled");
            break;
        default:
            Item->CfgTarget = PhCreateString(L"Unavailable");
            break;
        }
    }

    if (stackBoundsAvailable && StackFrame->StackAddress)
    {
        Item->StackValid = PhCreateString(stackValid ? L"Yes" : L"No");
    }
    else
    {
        Item->StackValid = PhCreateString(L"Unavailable");
    }

    if (Context->CustomWalkActive)
    {
        Item->UnwindMethod = PhCreateString(L"Custom/plugin");
    }
    else if (StackFrame->Flags & PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT)
    {
        Item->UnwindMethod = PhCreateString(
            StackFrame->Machine == IMAGE_FILE_MACHINE_I386 ? L"FPO" : L".pdata"
            );
    }
    else
    {
        Item->UnwindMethod = PhCreateString(L"No unwind metadata");
    }
}

static ULONG PhpGetThreadStackFrameContextSize(
    _In_ USHORT Machine
    )
{
#if defined(_AMD64_)
    if (Machine == IMAGE_FILE_MACHINE_AMD64)
        return sizeof(CONTEXT);
    if (Machine == IMAGE_FILE_MACHINE_I386)
        return sizeof(WOW64_CONTEXT);
#elif defined(_ARM64_)
    if (Machine == IMAGE_FILE_MACHINE_ARM64)
        return sizeof(CONTEXT);
    if (Machine == IMAGE_FILE_MACHINE_I386)
        return sizeof(WOW64_CONTEXT);
    if (Machine == IMAGE_FILE_MACHINE_ARMNT)
        return sizeof(ARM_NT_CONTEXT);
    if (Machine == IMAGE_FILE_MACHINE_AMD64)
        return sizeof(ARM64EC_NT_CONTEXT);
    if (Machine == IMAGE_FILE_MACHINE_ARM64EC)
        return sizeof(CONTEXT);
#else
    if (Machine == IMAGE_FILE_MACHINE_I386)
        return sizeof(CONTEXT);
#endif

    return 0;
}

_Function_class_(PH_WALK_THREAD_STACK_CALLBACK)
_Function_class_(PH_PLUGIN_WALK_THREAD_STACK_CALLBACK)
BOOLEAN NTAPI PhpWalkThreadStackCallback(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_ PVOID Context
    )
{
    PPH_THREAD_STACK_CONTEXT threadStackContext = (PPH_THREAD_STACK_CONTEXT)Context;
    PPH_STRING symbol = NULL;
    PPH_STRING fileName = NULL;
    PPH_STRING lineText = NULL;
    PTHREAD_STACK_ITEM item;
    PVOID baseAddress = NULL;
    BOOLEAN enableStackFrameInlineInfo;
    BOOLEAN enableStackFrameLineInfo;
    BOOLEAN skipSymbolPass;

    if (PhpThreadStackWalkCancelled(threadStackContext))
        return FALSE;

    skipSymbolPass = !!ReadBooleanAcquire(&threadStackContext->SkipSymbolPass);
    enableStackFrameInlineInfo = !skipSymbolPass && !!PhGetIntegerSetting(SETTING_ENABLE_THREAD_STACK_INLINE_SYMBOLS);
    enableStackFrameLineInfo = !skipSymbolPass && !!PhGetIntegerSetting(SETTING_ENABLE_THREAD_STACK_LINE_INFORMATION);

    PhAcquireQueuedLockExclusive(&threadStackContext->StatusLock);
    {
        if (threadStackContext->NewList->Count)
        {
            PH_FORMAT format[3];

            PhInitFormatS(&format[0], L"Processing stack frame #");
            PhInitFormatU(&format[1], threadStackContext->NewList->Count);
            PhInitFormatS(&format[2], L"...");

            PhMoveReference(&threadStackContext->StatusMessage, PhFormat(format, RTL_NUMBER_OF(format), 0));
        }
        else
        {
            PhMoveReference(&threadStackContext->StatusMessage, PhCreateString(L"Processing stack frames..."));
        }
    }
    PhReleaseQueuedLockExclusive(&threadStackContext->StatusLock);

    if (skipSymbolPass)
    {
        // Defer symbol/file/line resolution to phase 2. Leave symbol/fileName/lineText NULL.
        NOTHING;
    }
    else if (enableStackFrameInlineInfo && PhSymbolProviderInlineContextSupported())
    {
        symbol = PhGetSymbolFromInlineContext(
            threadStackContext->SymbolProvider,
            StackFrame,
            NULL,
            &fileName,
            NULL,
            NULL,
            &baseAddress
            );

        if (symbol &&
            (StackFrame->Machine == IMAGE_FILE_MACHINE_I386) &&
            !(StackFrame->Flags & PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT))
        {
            PhMoveReference(&symbol, PhConcatStringRefZ(&symbol->sr, L" (No unwind info)"));
        }

        if (PhPluginsEnabled)
        {
            PH_PLUGIN_THREAD_STACK_CONTROL control;

            control.Type = PluginThreadStackResolveSymbol;
            control.UniqueKey = threadStackContext;
            control.u.ResolveSymbol.StackFrame = StackFrame;
            control.u.ResolveSymbol.Symbol = symbol;
            control.u.ResolveSymbol.FileName = fileName;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

            symbol = control.u.ResolveSymbol.Symbol;
            fileName = control.u.ResolveSymbol.FileName;
        }

        if (enableStackFrameLineInfo)
        {
            PPH_STRING lineFileName;
            PH_SYMBOL_LINE_INFORMATION lineInfo;

            if (PhGetLineFromInlineContext(
                threadStackContext->SymbolProvider,
                StackFrame,
                baseAddress,
                &lineFileName,
                NULL,
                &lineInfo
                ))
            {
                PH_FORMAT format[3];

                PhInitFormatSR(&format[0], lineFileName->sr);
                PhInitFormatS(&format[1], L" @ ");
                PhInitFormatU(&format[2], lineInfo.LineNumber);

                lineText = PhFormat(format, RTL_NUMBER_OF(format), 0);
                PhDereferenceObject(lineFileName);
            }
        }

        if (symbol && PhIsStackFrameTypeInline(StackFrame->InlineFrameContext))
        {
            PhMoveReference(&symbol, PhConcatStringRefZ(&symbol->sr, L" (Inline function)"));
        }
    }
    else
    {
        symbol = PhGetSymbolFromAddress(
            threadStackContext->SymbolProvider,
            StackFrame->PcAddress,
            NULL,
            &fileName,
            NULL,
            NULL
            );

        if (symbol &&
            (StackFrame->Machine == IMAGE_FILE_MACHINE_I386) &&
            !(StackFrame->Flags & PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT))
        {
            PhMoveReference(&symbol, PhConcatStringRefZ(&symbol->sr, L" (No unwind info)"));
        }

        if (PhPluginsEnabled)
        {
            PH_PLUGIN_THREAD_STACK_CONTROL control;

            control.Type = PluginThreadStackResolveSymbol;
            control.UniqueKey = threadStackContext;
            control.u.ResolveSymbol.StackFrame = StackFrame;
            control.u.ResolveSymbol.Symbol = symbol;
            control.u.ResolveSymbol.FileName = fileName;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

            symbol = control.u.ResolveSymbol.Symbol;
            fileName = control.u.ResolveSymbol.FileName;
        }

        if (enableStackFrameLineInfo)
        {
            PPH_STRING lineFileName;
            PH_SYMBOL_LINE_INFORMATION lineInfo;

            if (PhGetLineFromAddress(
                threadStackContext->SymbolProvider,
                StackFrame->PcAddress,
                &lineFileName,
                NULL,
                &lineInfo
                ))
            {
                PH_FORMAT format[3];

                PhInitFormatSR(&format[0], lineFileName->sr);
                PhInitFormatS(&format[1], L" @ ");
                PhInitFormatU(&format[2], lineInfo.LineNumber);

                lineText = PhFormat(format, RTL_NUMBER_OF(format), 0);
                PhDereferenceObject(lineFileName);
            }
        }
    }

    item = PhAllocateZero(sizeof(THREAD_STACK_ITEM));
    item->StackFrame = *StackFrame;
    item->StackFrame.ContextRecord = NULL; // Only valid during the callback.

    if (FlagOn(StackFrame->Flags, PH_THREAD_STACK_FRAME_CONTEXT_PRESENT) && StackFrame->ContextRecord)
    {
        ULONG contextSize = PhpGetThreadStackFrameContextSize(StackFrame->Machine);

        if (contextSize)
            item->FrameContext = PhAllocateCopy(StackFrame->ContextRecord, contextSize);
    }

    if (!item->FrameContext)
        ClearFlag(item->StackFrame.Flags, PH_THREAD_STACK_FRAME_CONTEXT_PRESENT);
    item->Index = threadStackContext->NewList->Count;
    item->Symbol = symbol;
    item->FileName = fileName;
    item->LineText = lineText;
    PhpAnalyzeThreadStackItem(threadStackContext, StackFrame, item);

    if (!skipSymbolPass && !item->Language)
        item->Language = PhpGetThreadStackLanguage(threadStackContext->SymbolProvider, StackFrame->PcAddress);
    if (!item->Language)
        item->Language = PhCreateString(L"Unavailable");

    PhAddItemList(threadStackContext->NewList, item);

    if ( // Zero inline frames so the stack matches windbg output. (dmex)
        PhSymbolProviderInlineContextSupported() &&
        PhIsStackFrameTypeInline(StackFrame->InlineFrameContext)
        )
    {
        // Note: Only zero the item->StackFrame local copy. (dmex)
        item->StackFrame.PcAddress = 0;
        item->StackFrame.ReturnAddress = 0;
        item->StackFrame.FrameAddress = 0;
        item->StackFrame.StackAddress = 0;
        memset(item->StackFrame.Params, 0, sizeof(item->StackFrame.Params));
    }

    return TRUE;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpRefreshThreadStackThreadStart(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    NTSTATUS status;
    PPH_THREAD_STACK_CONTEXT threadStackContext = Parameter;
    CLIENT_ID clientId;
    BOOLEAN defaultWalk;

    PhInitializeAutoPool(&autoPool);
    PhpInitializeThreadStackDiagnostics(threadStackContext);

    PhLoadSymbolProviderOptions(threadStackContext->SymbolProvider);
    PhLoadSymbolProviderModules(threadStackContext->SymbolProvider, threadStackContext->ProcessId);

    clientId.UniqueProcess = threadStackContext->ProcessId;
    clientId.UniqueThread = threadStackContext->ThreadId;
    defaultWalk = TRUE;

    if (threadStackContext->CustomWalk)
    {
        PH_PLUGIN_THREAD_STACK_CONTROL control;

        control.Type = PluginThreadStackWalkStack;
        control.UniqueKey = threadStackContext;
        control.u.WalkStack.Status = STATUS_UNSUCCESSFUL;
        control.u.WalkStack.ThreadHandle = threadStackContext->ThreadHandle;
        control.u.WalkStack.ProcessHandle = threadStackContext->SymbolProvider->ProcessHandle;
        control.u.WalkStack.ClientId = &clientId;
        control.u.WalkStack.Flags = (threadStackContext->IsWow64Process ? PH_WALK_USER_WOW64_STACK : 0) | PH_WALK_USER_STACK | PH_WALK_KERNEL_STACK;
        control.u.WalkStack.Callback = PhpWalkThreadStackCallback;
        control.u.WalkStack.CallbackContext = threadStackContext;
        threadStackContext->CustomWalkActive = TRUE;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
        threadStackContext->CustomWalkActive = FALSE;
        status = control.u.WalkStack.Status;

        if (NT_SUCCESS(status))
            defaultWalk = FALSE;
    }

    if (defaultWalk)
    {
        PH_PLUGIN_THREAD_STACK_CONTROL control;

        control.UniqueKey = threadStackContext;

        if (PhPluginsEnabled)
        {
            control.Type = PluginThreadStackBeginDefaultWalkStack;
            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
        }

        status = PhWalkThreadStack(
            threadStackContext->ThreadHandle,
            threadStackContext->SymbolProvider->ProcessHandle,
            &clientId,
            threadStackContext->SymbolProvider,
            (threadStackContext->IsWow64Process ? PH_WALK_USER_WOW64_STACK : 0) | PH_WALK_USER_STACK | PH_WALK_KERNEL_STACK,
            PhpWalkThreadStackCallback,
            threadStackContext
            );

        if (PhPluginsEnabled)
        {
            control.Type = PluginThreadStackEndDefaultWalkStack;
            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
        }
    }

    if (threadStackContext->NewList->Count != 0)
        status = STATUS_SUCCESS;

    threadStackContext->WalkStatus = status;
    PostMessage(threadStackContext->TaskDialogHandle, WM_PH_COMPLETED, 0, 0);

    PhDeleteAutoPool(&autoPool);
    PhDereferenceObject(threadStackContext);

    return STATUS_SUCCESS;
}

LRESULT CALLBACK PhpThreadStackTaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_THREAD_STACK_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hwndDlg, 0xF)))
        return 0;

    oldWndProc = context->ThreadStackStatusDefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(hwndDlg, oldWndProc);
            PhRemoveWindowContext(hwndDlg, 0xF);
        }
        break;
    case WM_PH_COMPLETED:
        {
            context->EnableCloseDialog = TRUE;
            SendMessage(hwndDlg, TDM_CLICK_BUTTON, IDOK, 0);
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwndDlg, uMsg, wParam, lParam);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID PhpSymbolProviderEventCallbackHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_SYMBOL_EVENT_DATA event = Parameter;
    PPH_THREAD_STACK_CONTEXT context = Context;
    PPH_STRING statusMessage = NULL;
    ULONG statusProgress = 0;

    if (!event) return;
    if (!context) return;

    switch (event->EventType)
    {
    case PH_SYMBOL_EVENT_TYPE_LOAD_START:
        statusMessage = PhReferenceObject(event->EventMessage);
        break;
    case PH_SYMBOL_EVENT_TYPE_LOAD_END:
        statusMessage = PhCreateString(L"Loading symbols...");
        break;
    case PH_SYMBOL_EVENT_TYPE_PROGRESS:
        {
            statusMessage = PhReferenceObject(event->EventMessage);
            statusProgress = (ULONG)event->EventProgress;
        }
        break;
    }

    if (statusMessage)
    {
        context->SymbolProgress = statusProgress;
        PhpSetThreadStackStatus(context, statusMessage);
    }
}

HRESULT CALLBACK PhpThreadStackTaskDialogCallback(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_THREAD_STACK_CONTEXT context = (PPH_THREAD_STACK_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_DIALOG_CONSTRUCTED:
        {
            NTSTATUS status;

            context->TaskDialogHandle = hwndDlg;
            context->WindowDpi = PhGetWindowDpi(hwndDlg);

            PhSetApplicationWindowIcon(hwndDlg);
            //SendMessage(hwndDlg, TDM_UPDATE_ICON, TDIE_ICON_MAIN, (LPARAM)PhGetApplicationIcon(FALSE, PhGetWindowDpi(hwndDlg)));

            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);
            InterlockedExchange8(&context->SymbolProgressMarquee, TRUE);

            context->ThreadStackStatusDefaultWindowProc = PhGetWindowProcedure(hwndDlg);
            PhSetWindowContext(hwndDlg, 0xF, context);
            PhSetWindowProcedure(hwndDlg, PhpThreadStackTaskDialogSubclassProc);

            PhRegisterCallback(
                &PhSymbolEventCallback,
                PhpSymbolProviderEventCallbackHandler,
                context,
                &context->SymbolProviderEventRegistration
                );

            PhReferenceObject(context);

            if (!NT_SUCCESS(status = PhCreateThread2(PhpRefreshThreadStackThreadStart, context)))
            {
                PhDereferenceObject(context);
                context->WalkStatus = status;
                PostMessage(hwndDlg, WM_PH_COMPLETED, 0, 0);
            }
        }
        break;
    case TDN_DESTROYED:
        {
            // TDN_DESTROYED can occur without TDN_DIALOG_CONSTRUCTED
            if (context->TaskDialogHandle)
                PhUnregisterCallback(&PhSymbolEventCallback, &context->SymbolProviderEventRegistration);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCANCEL)
            {
                WriteBooleanRelease(&context->StopWalk, TRUE);
                context->SymbolProvider->Terminating = TRUE;
            }

            //if (!context->EnableCloseDialog)
            //    return S_FALSE;
        }
        break;
    case TDN_TIMER:
        {
            PPH_STRING message;
            PPH_STRING content;
            ULONG progress = 0;

            PhAcquireQueuedLockShared(&context->StatusLock);
            PhSetReference(&message, context->StatusMessage);
            PhSetReference(&content, context->StatusContent);
            progress = context->SymbolProgress;
            PhReleaseQueuedLockShared(&context->StatusLock);

            SendMessage(context->TaskDialogHandle, TDM_SET_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)PhGetStringOrDefault(message, L"Processing stack frames..."));
            SendMessage(context->TaskDialogHandle, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)PhGetStringOrDefault(content, L"Loading symbols for image..."));

            PhClearReference(&message);
            PhClearReference(&content);

            if (ReadBooleanAcquire(&context->SymbolProgressReset))
            {
                SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
                SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

                PhAcquireQueuedLockExclusive(&context->StatusLock);
                InterlockedExchange8(&context->SymbolProgressMarquee, TRUE);
                InterlockedExchange8(&context->SymbolProgressReset, FALSE);
                PhReleaseQueuedLockExclusive(&context->StatusLock);
            }

            if (progress)
            {
                if (ReadBooleanAcquire(&context->SymbolProgressMarquee))
                {
                    SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
                    SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, FALSE, 0);

                    PhAcquireQueuedLockExclusive(&context->StatusLock);
                    InterlockedExchange8(&context->SymbolProgressMarquee, FALSE);
                    PhReleaseQueuedLockExclusive(&context->StatusLock);
                }

                SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_POS, (WPARAM)progress, 0);
            }
            else
            {
                if (!ReadBooleanAcquire(&context->SymbolProgressMarquee))
                {
                    SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
                    SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

                    PhAcquireQueuedLockExclusive(&context->StatusLock);
                    InterlockedExchange8(&context->SymbolProgressMarquee, TRUE);
                    PhReleaseQueuedLockExclusive(&context->StatusLock);
                }
            }
        }
        break;
    }

    return S_OK;
}

BOOLEAN PhpShowThreadStackWindow(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;
    INT result;

    {
        PPH_STRING windowText;
        HWND control;

        if (control = GetDlgItem(Context->ParentHandle, IDC_STATE))
        {
            if (windowText = PhGetWindowText(control))
            {
                PhMoveReference(&Context->StatusContent, windowText);
            }
        }
    }

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags =
        TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION |
        TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SHOW_MARQUEE_PROGRESS_BAR |
        TDF_CALLBACK_TIMER;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE, Context->WindowDpi);
    config.pfCallback = PhpThreadStackTaskDialogCallback;
    config.lpCallbackData = (LONG_PTR)Context;
    config.hwndParent = Context->WindowHandle;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Processing stack frames...";
    config.pszContent = PhGetStringOrDefault(Context->StatusContent, L"Loading symbols for image...");
    config.cxWidth = 200;

    return PhShowTaskDialog(&config, &result, NULL, NULL) && result != IDCANCEL;
}

VOID PhpApplyThreadStackFrames(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->List->Count; i++)
        PhpFreeThreadStackItem(Context->List->Items[i]);

    PhDereferenceObject(Context->List);
    Context->List = Context->NewList;
    Context->NewList = PhCreateList(10);

    ClearThreadStackTree(Context);

    // Suspend redraw so the per-node TreeNew_NodesStructured calls coalesce into a
    // single restructure/layout pass instead of running O(n) work per frame, which
    // makes deep call stacks O(n^2) and freezes the UI. (issue #2914)
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    for (i = 0; i < Context->List->Count; i++)
    {
        PTHREAD_STACK_ITEM item = Context->List->Items[i];
        PPH_STACK_TREE_ROOT_NODE stackNode;

        stackNode = AddThreadStackNode(Context, item->Index);
        stackNode->StackFrame = item->StackFrame;
        stackNode->FrameContext = item->FrameContext;
        stackNode->SymbolString = item->Symbol;
        stackNode->FileNameString = item->FileName;
        stackNode->LineTextString = item->LineText;
        stackNode->ProtectionString = item->Protection;
        stackNode->MemoryTypeString = item->MemoryType;
        stackNode->StackValidString = item->StackValid;
        stackNode->UnwindMethodString = item->UnwindMethod;
        stackNode->CfgTargetString = item->CfgTarget;
        stackNode->LanguageString = item->Language;

        if (stackNode->FileNameString)
            stackNode->FileNameString = PhGetFileName(stackNode->FileNameString);

        if (item->StackFrame.StackAddress)
            PhPrintPointer(stackNode->StackAddressString, item->StackFrame.StackAddress);
        if (item->StackFrame.FrameAddress)
            PhPrintPointer(stackNode->FrameAddressString, item->StackFrame.FrameAddress);

        // There are no params for kernel-mode stack traces.
        if ((ULONG_PTR)item->StackFrame.PcAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
        {
            if (item->StackFrame.Params[0])
                PhPrintPointer(stackNode->Parameter1String, item->StackFrame.Params[0]);
            if (item->StackFrame.Params[1])
                PhPrintPointer(stackNode->Parameter2String, item->StackFrame.Params[1]);
            if (item->StackFrame.Params[2])
                PhPrintPointer(stackNode->Parameter3String, item->StackFrame.Params[2]);
            if (item->StackFrame.Params[3])
                PhPrintPointer(stackNode->Parameter4String, item->StackFrame.Params[3]);
        }

        if (item->StackFrame.PcAddress)
            PhPrintPointer(stackNode->PcAddressString, item->StackFrame.PcAddress);
        if (item->StackFrame.ReturnAddress)
            PhPrintPointer(stackNode->ReturnAddressString, item->StackFrame.ReturnAddress);

        switch (stackNode->StackFrame.Machine)
        {
        case IMAGE_FILE_MACHINE_ARM64EC:
            PhInitializeStringRef(&stackNode->Architecture, L"ARM64EC");
            break;
        case IMAGE_FILE_MACHINE_CHPE_X86:
            PhInitializeStringRef(&stackNode->Architecture, L"CHPE");
            break;
        case IMAGE_FILE_MACHINE_ARM64:
            PhInitializeStringRef(&stackNode->Architecture, L"ARM64");
            break;
        case IMAGE_FILE_MACHINE_ARM:
            PhInitializeStringRef(&stackNode->Architecture, L"ARM");
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            PhInitializeStringRef(&stackNode->Architecture, L"x64");
            break;
        case IMAGE_FILE_MACHINE_I386:
            PhInitializeStringRef(&stackNode->Architecture, L"x86");
            break;
        default:
            PhInitializeStringRef(&stackNode->Architecture, L"");
            break;
        }

        if (i > 0 && item->StackFrame.StackAddress)
        {
            PTHREAD_STACK_ITEM previousFrame = Context->List->Items[i - 1];

            // Windbg "k f" displays the distance between adjacent frames. (dmex)
            if (previousFrame->StackFrame.StackAddress)
            {
                stackNode->FrameDistance = (ULONG)((ULONG_PTR)item->StackFrame.StackAddress - (ULONG_PTR)previousFrame->StackFrame.StackAddress);
            }
        }

        UpdateThreadStackNode(Context, stackNode);
    }

    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    TreeNew_NodesStructured(Context->TreeNewHandle);

    PhApplyTreeNewFilters(&Context->TreeFilterSupport);
}

// Phase 2: resolve symbols for each frame (already shown in the tree) on the worker thread.
// The worker only reads its private copy of the frames; results are pushed onto
// ResolvedListHead and applied by the UI thread on WM_PH_SYMBOL_RESOLVED.
VOID PhpResolveDeferredThreadStackSymbols(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _In_reads_(NumberOfFrames) PTHREAD_STACK_DEFERRED_FRAME Frames,
    _In_ ULONG NumberOfFrames,
    _In_ ULONG Generation
    )
{
    BOOLEAN enableInline;
    BOOLEAN enableLine;

    enableInline = !!PhGetIntegerSetting(SETTING_ENABLE_THREAD_STACK_INLINE_SYMBOLS);
    enableLine = !!PhGetIntegerSetting(SETTING_ENABLE_THREAD_STACK_LINE_INFORMATION);

    for (ULONG i = 0; i < NumberOfFrames; i++)
    {
        PTHREAD_STACK_DEFERRED_FRAME item;
        PTHREAD_STACK_RESOLVED_ITEM resolvedItem;
        PPH_STRING symbol = NULL;
        PPH_STRING fileName = NULL;
        PPH_STRING lineText = NULL;
        PPH_STRING language = NULL;
        PVOID baseAddress = NULL;

        if (PhpThreadStackWalkCancelled(Context))
            break;

        item = &Frames[i];

        if (enableInline && PhSymbolProviderInlineContextSupported())
        {
            symbol = PhGetSymbolFromInlineContext(
                Context->SymbolProvider,
                &item->StackFrame,
                NULL,
                &fileName,
                NULL,
                NULL,
                &baseAddress
                );

            if (symbol &&
                (item->StackFrame.Machine == IMAGE_FILE_MACHINE_I386) &&
                !(item->StackFrame.Flags & PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT))
            {
                PhMoveReference(&symbol, PhConcatStringRefZ(&symbol->sr, L" (No unwind info)"));
            }

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_THREAD_STACK_CONTROL control;

                control.Type = PluginThreadStackResolveSymbol;
                control.UniqueKey = Context;
                control.u.ResolveSymbol.StackFrame = &item->StackFrame;
                control.u.ResolveSymbol.Symbol = symbol;
                control.u.ResolveSymbol.FileName = fileName;

                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

                symbol = control.u.ResolveSymbol.Symbol;
                fileName = control.u.ResolveSymbol.FileName;
            }

            if (enableLine)
            {
                PPH_STRING lineFileName;
                PH_SYMBOL_LINE_INFORMATION lineInfo;

                if (PhGetLineFromInlineContext(
                    Context->SymbolProvider,
                    &item->StackFrame,
                    baseAddress,
                    &lineFileName,
                    NULL,
                    &lineInfo
                    ))
                {
                    PH_FORMAT format[3];

                    PhInitFormatSR(&format[0], lineFileName->sr);
                    PhInitFormatS(&format[1], L" @ ");
                    PhInitFormatU(&format[2], lineInfo.LineNumber);

                    lineText = PhFormat(format, RTL_NUMBER_OF(format), 0);
                    PhDereferenceObject(lineFileName);
                }
            }

            if (symbol && PhIsStackFrameTypeInline(item->StackFrame.InlineFrameContext))
            {
                PhMoveReference(&symbol, PhConcatStringRefZ(&symbol->sr, L" (Inline function)"));
            }
        }
        else
        {
            symbol = PhGetSymbolFromAddress(
                Context->SymbolProvider,
                item->StackFrame.PcAddress,
                NULL,
                &fileName,
                NULL,
                NULL
                );

            if (symbol &&
                (item->StackFrame.Machine == IMAGE_FILE_MACHINE_I386) &&
                !(item->StackFrame.Flags & PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT))
            {
                PhMoveReference(&symbol, PhConcatStringRefZ(&symbol->sr, L" (No unwind info)"));
            }

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_THREAD_STACK_CONTROL control;

                control.Type = PluginThreadStackResolveSymbol;
                control.UniqueKey = Context;
                control.u.ResolveSymbol.StackFrame = &item->StackFrame;
                control.u.ResolveSymbol.Symbol = symbol;
                control.u.ResolveSymbol.FileName = fileName;

                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

                symbol = control.u.ResolveSymbol.Symbol;
                fileName = control.u.ResolveSymbol.FileName;
            }

            if (enableLine)
            {
                PPH_STRING lineFileName;
                PH_SYMBOL_LINE_INFORMATION lineInfo;

                if (PhGetLineFromAddress(
                    Context->SymbolProvider,
                    item->StackFrame.PcAddress,
                    &lineFileName,
                    NULL,
                    &lineInfo
                    ))
                {
                    PH_FORMAT format[3];

                    PhInitFormatSR(&format[0], lineFileName->sr);
                    PhInitFormatS(&format[1], L" @ ");
                    PhInitFormatU(&format[2], lineInfo.LineNumber);

                    lineText = PhFormat(format, RTL_NUMBER_OF(format), 0);
                    PhDereferenceObject(lineFileName);
                }
            }
        }

        if (item->DiagnosticAddress &&
            (ULONG_PTR)item->DiagnosticAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
            language = PhpGetThreadStackLanguage(Context->SymbolProvider, item->DiagnosticAddress);

        // Hand off ownership of the resolved strings to the UI thread. The interlocked push
        // publishes the record; only post when the list was empty since the UI flushes it all.
        resolvedItem = PhAllocateZero(sizeof(THREAD_STACK_RESOLVED_ITEM));
        resolvedItem->Generation = Generation;
        resolvedItem->Index = item->Index;
        resolvedItem->Symbol = symbol;
        resolvedItem->FileName = fileName;
        resolvedItem->LineText = lineText;
        resolvedItem->Language = language;

        if (!RtlInterlockedPushEntrySList(&Context->ResolvedListHead, &resolvedItem->ListEntry))
            PostMessage(Context->WindowHandle, WM_PH_SYMBOL_RESOLVED, 0, 0);
    }
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpDeferredThreadStackThreadStart(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    NTSTATUS status;
    PPH_THREAD_STACK_CONTEXT context = Parameter;
    CLIENT_ID clientId;
    BOOLEAN defaultWalk = TRUE;
    ULONG generation;
    PTHREAD_STACK_DEFERRED_FRAME frames = NULL;
    ULONG numberOfFrames = 0;

    // Only one deferred worker runs at a time (see PhpRefreshThreadStack), so this worker owns
    // NewList and the diagnostics fields until it hands NewList to the UI thread.
    generation = ReadULongAcquire(&context->WorkerGeneration);
    WriteULongRelease(&context->ActiveGeneration, generation);

    PhInitializeAutoPool(&autoPool);
    PhpInitializeThreadStackDiagnostics(context);

    PhLoadSymbolProviderOptions(context->SymbolProvider);

    clientId.UniqueProcess = context->ProcessId;
    clientId.UniqueThread = context->ThreadId;

    // Phase 1: walk frames without symbol lookup. Modules are NOT pre-loaded into the symbol
    // provider here -- that is deferred to phase 2 below.
    WriteBooleanRelease(&context->SkipSymbolPass, TRUE);
    PhpSetThreadStackStatus(context, PhCreateString(L"Walking stack..."));

    if (context->CustomWalk)
    {
        PH_PLUGIN_THREAD_STACK_CONTROL control;

        control.Type = PluginThreadStackWalkStack;
        control.UniqueKey = context;
        control.u.WalkStack.Status = STATUS_UNSUCCESSFUL;
        control.u.WalkStack.ThreadHandle = context->ThreadHandle;
        control.u.WalkStack.ProcessHandle = context->SymbolProvider->ProcessHandle;
        control.u.WalkStack.ClientId = &clientId;
        control.u.WalkStack.Flags = (context->IsWow64Process ? PH_WALK_USER_WOW64_STACK : 0) | PH_WALK_USER_STACK | PH_WALK_KERNEL_STACK | PH_WALK_NO_SYMBOL_LOOKUP;
        control.u.WalkStack.Callback = PhpWalkThreadStackCallback;
        control.u.WalkStack.CallbackContext = context;
        context->CustomWalkActive = TRUE;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
        context->CustomWalkActive = FALSE;
        status = control.u.WalkStack.Status;

        if (NT_SUCCESS(status))
            defaultWalk = FALSE;
    }

    if (defaultWalk)
    {
        PH_PLUGIN_THREAD_STACK_CONTROL pluginControl;

        pluginControl.UniqueKey = context;

        if (PhPluginsEnabled)
        {
            pluginControl.Type = PluginThreadStackBeginDefaultWalkStack;
            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &pluginControl);
        }

        status = PhWalkThreadStack(
            context->ThreadHandle,
            context->SymbolProvider->ProcessHandle,
            &clientId,
            context->SymbolProvider,
            (context->IsWow64Process ? PH_WALK_USER_WOW64_STACK : 0) | PH_WALK_USER_STACK | PH_WALK_KERNEL_STACK | PH_WALK_NO_SYMBOL_LOOKUP,
            PhpWalkThreadStackCallback,
            context
            );

        if (PhPluginsEnabled)
        {
            pluginControl.Type = PluginThreadStackEndDefaultWalkStack;
            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &pluginControl);
        }
    }

    context->WalkStatus = status;

    // Keep a private copy of the frames for phase 2. After WM_PH_FRAMES_READY the items belong
    // to the UI thread and may be freed at any time (refresh), so the worker must not touch them.
    if (!PhpThreadStackWalkCancelled(context) && NT_SUCCESS(status) && context->NewList->Count)
    {
        numberOfFrames = context->NewList->Count;
        frames = PhAllocate(sizeof(THREAD_STACK_DEFERRED_FRAME) * numberOfFrames);

        for (ULONG i = 0; i < numberOfFrames; i++)
        {
            PTHREAD_STACK_ITEM item = context->NewList->Items[i];

            frames[i].StackFrame = item->StackFrame;
            frames[i].DiagnosticAddress = item->DiagnosticAddress;
            frames[i].Index = item->Index;
        }
    }

    // Hand the frames to the UI. Use SendMessage so the worker never touches NewList afterwards.
    if (frames)
        SendMessage(context->WindowHandle, WM_PH_FRAMES_READY, 0, 0);
    else
        SendMessage(context->WindowHandle, WM_PH_FRAMES_READY, 1, 0); // walk failed/cancelled -- discard NewList

    // Phase 2: resolve symbols (PDB downloads happen here) and push per-frame updates.
    if (frames && !PhpThreadStackWalkCancelled(context))
    {
        WriteBooleanRelease(&context->SkipSymbolPass, FALSE);
        PhpSetThreadStackStatus(context, PhCreateString(L"Loading symbols..."));
        PhLoadSymbolProviderModules(context->SymbolProvider, context->ProcessId);
        PhpResolveDeferredThreadStackSymbols(context, frames, numberOfFrames, generation);
    }

    if (frames)
        PhFree(frames);

    // Signal before posting so a pending refresh started from WM_PH_SYMBOLS_COMPLETE sees
    // the worker as finished.
    NtSetEvent(context->WorkerCompletedEvent, NULL);
    PostMessage(context->WindowHandle, WM_PH_SYMBOLS_COMPLETE, generation, 0);

    PhDeleteAutoPool(&autoPool);
    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

NTSTATUS PhpRefreshThreadStack(
    _In_ HWND hwnd,
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    NTSTATUS status;
    ULONG i;

    PhMoveReference(&Context->StatusMessage, PhCreateString(L"Processing stack frames..."));

    if (Context->DeferSymbols)
    {
        LARGE_INTEGER timeout;

        // Deferred mode: skip the modal task dialog. Spawn the worker; phase 1 will deliver
        // frames via WM_PH_FRAMES_READY and phase 2 will stream symbol updates via
        // WM_PH_SYMBOL_RESOLVED. Close cancels the worker through StopWalk.
        //
        // If a worker is still running, cancel it (generation bump) and defer the refresh until
        // it posts WM_PH_SYMBOLS_COMPLETE. This avoids blocking the UI thread on PDB downloads
        // and never runs two workers against the same NewList.
        timeout.QuadPart = 0;

        if (NtWaitForSingleObject(Context->WorkerCompletedEvent, FALSE, &timeout) == STATUS_TIMEOUT)
        {
            WriteULongRelease(&Context->WorkerGeneration, Context->WorkerGeneration + 1);
            Context->RefreshPending = TRUE;
            return STATUS_SUCCESS;
        }

        WriteULongRelease(&Context->WorkerGeneration, Context->WorkerGeneration + 1);
        NtResetEvent(Context->WorkerCompletedEvent, NULL);
        PhReferenceObject(Context);

        if (!NT_SUCCESS(status = PhCreateThread2(PhpDeferredThreadStackThreadStart, Context)))
        {
            NtSetEvent(Context->WorkerCompletedEvent, NULL);
            PhDereferenceObject(Context);
            return status;
        }

        return STATUS_SUCCESS;
    }

    // Legacy modal task dialog flow.
    if (!PhpShowThreadStackWindow(Context))
    {
        return STATUS_ABANDONED;
    }

    if (!ReadBooleanAcquire(&Context->StopWalk) && NT_SUCCESS(Context->WalkStatus))
    {
        PhpApplyThreadStackFrames(Context);
        PhSetWindowText(Context->StatusHandle, PhaFormatString(L"%lu frames, symbols loaded", Context->List->Count)->Buffer);
    }
    else
    {
        for (i = 0; i < Context->NewList->Count; i++)
            PhpFreeThreadStackItem(Context->NewList->Items[i]);

        PhClearList(Context->NewList);
    }

    if (ReadBooleanAcquire(&Context->StopWalk))
        return STATUS_ABANDONED;

    return Context->WalkStatus;
}
