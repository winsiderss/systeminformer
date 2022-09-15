/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2012
 *     dmex    2018-2022
 *
 */

#include <phapp.h>
#include <thrdlist.h>

#include <emenu.h>
#include <settings.h>

#include <extmgri.h>
#include <phsettings.h>
#include <thrdprv.h>

BOOLEAN PhpThreadNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG PhpThreadNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PhpDestroyThreadNode(
    _In_ PPH_THREAD_NODE ThreadNode
    );

VOID PhpRemoveThreadNode(
    _In_ PPH_THREAD_NODE ThreadNode,
    _In_ PPH_THREAD_LIST_CONTEXT Context
    );

LONG PhpThreadTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    );

BOOLEAN NTAPI PhpThreadTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

PPH_STRING PhGetSystemCallNumberName(
    _In_ USHORT SystemCallNumber
    );
PPH_STRING PhGetApartmentStateString(
    _In_ OLETLSFLAGS ApartmentState
    );

VOID PhInitializeThreadList(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PPH_THREAD_LIST_CONTEXT Context
    )
{
    memset(Context, 0, sizeof(PH_THREAD_LIST_CONTEXT));
    Context->EnableStateHighlighting = TRUE;

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPH_THREAD_NODE),
        PhpThreadNodeHashtableEqualFunction,
        PhpThreadNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;

    PhSetControlTheme(TreeNewHandle, L"explorer");
    TreeNew_SetCallback(TreeNewHandle, PhpThreadTreeNewCallback, Context);
    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    // Default columns
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_TID, TRUE, L"TID", 50, PH_ALIGN_RIGHT, 0, DT_RIGHT);
    PhAddTreeNewColumnEx(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CPU, TRUE, L"CPU", 45, PH_ALIGN_RIGHT, 1, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CYCLESDELTA, TRUE, L"Cycles delta", 80, PH_ALIGN_RIGHT, 2, DT_RIGHT, TRUE);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_STARTADDRESS, TRUE, L"Start address", 180, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumnEx(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_PRIORITYSYMBOLIC, TRUE, L"Priority (symbolic)", 80, PH_ALIGN_LEFT, 4, 0, TRUE);
    // Available columns
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_SERVICE, FALSE, L"Service", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_NAME, FALSE, L"Name", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_STARTED, FALSE, L"Created", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_STARTMODULE, FALSE, L"Start module", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CONTEXTSWITCHES, FALSE, L"Context switches", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumnEx(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CONTEXTSWITCHESDELTA, FALSE, L"Context switches delta", 100, PH_ALIGN_RIGHT, ULONG_MAX, DT_RIGHT, TRUE);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_PRIORITY, FALSE, L"Priority", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_BASEPRIORITY, FALSE, L"Base priority", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_PAGEPRIORITY, FALSE, L"Page priority", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_IOPRIORITY, FALSE, L"I/O priority", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CYCLES, FALSE, L"Cycles", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_STATE, FALSE, L"State", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_KERNELTIME, FALSE, L"Kernel time", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_USERTIME, FALSE, L"User time", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_IDEALPROCESSOR, FALSE, L"Ideal processor", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CRITICAL, FALSE, L"Critical", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_TIDHEX, FALSE, L"TID (hex)", 50, PH_ALIGN_RIGHT, ULONG_MAX, DT_RIGHT);
    PhAddTreeNewColumnEx(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CPUCORECYCLES, FALSE, L"CPU (relative)", 50, PH_ALIGN_RIGHT, ULONG_MAX, DT_RIGHT, TRUE);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_TOKEN_STATE, FALSE, L"Impersonation", 50, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_PENDINGIRP, FALSE, L"Pending IRP", 50, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_LASTSYSTEMCALL, FALSE, L"Last system call", 50, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_LASTSTATUSCODE, FALSE, L"Last status code", 50, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_TIMELINE, FALSE, L"Timeline", 100, PH_ALIGN_LEFT, ULONG_MAX, 0, TN_COLUMN_FLAG_CUSTOMDRAW | TN_COLUMN_FLAG_SORTDESCENDING);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_APARTMENTSTATE, FALSE, L"COM apartment", 50, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_FIBER, FALSE, L"Fiber", 50, PH_ALIGN_RIGHT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_PRIORITYBOOST, FALSE, L"Priority boost", 50, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumnEx(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CPUUSER, FALSE, L"CPU (user)", 50, PH_ALIGN_LEFT, ULONG_MAX, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CPUKERNEL, FALSE, L"CPU (kernel)", 50, PH_ALIGN_RIGHT, ULONG_MAX, DT_RIGHT, TRUE);
    //PhAddTreeNewColumnEx2(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CPUHISTORY, FALSE, L"CPU history", 100, PH_ALIGN_LEFT, ULONG_MAX, 0, TN_COLUMN_FLAG_CUSTOMDRAW | TN_COLUMN_FLAG_SORTDESCENDING);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_STACKUSAGE, FALSE, L"Stack usage", 50, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_WAITTIME, FALSE, L"Wait time", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);
    TreeNew_SetTriState(TreeNewHandle, TRUE);
    TreeNew_SetSort(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CYCLESDELTA, DescendingSortOrder);

    PhCmInitializeManager(&Context->Cm, TreeNewHandle, PH_THREAD_TREELIST_COLUMN_MAXIMUM, PhpThreadTreeNewPostSortFunction);

    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, Context->TreeNewHandle, Context->NodeList);
}

VOID PhDeleteThreadList(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    )
{
    ULONG i;

    PhDeleteTreeNewFilterSupport(&Context->TreeFilterSupport);

    PhCmDeleteManager(&Context->Cm);

    for (i = 0; i < Context->NodeList->Count; i++)
        PhpDestroyThreadNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

BOOLEAN PhpThreadNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_THREAD_NODE threadNode1 = *(PPH_THREAD_NODE *)Entry1;
    PPH_THREAD_NODE threadNode2 = *(PPH_THREAD_NODE *)Entry2;

    return threadNode1->ThreadId == threadNode2->ThreadId;
}

ULONG PhpThreadNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return HandleToUlong((*(PPH_THREAD_NODE *)Entry)->ThreadId) / 4;
}

VOID PhLoadSettingsThreadList(
    _Inout_ PPH_THREAD_LIST_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;
    PH_TREENEW_COLUMN column;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    settings = PhGetStringSetting(L"ThreadTreeListColumns");
    sortSettings = PhGetStringSetting(L"ThreadTreeListSort");
    Context->Flags = PhGetIntegerSetting(L"ThreadTreeListFlags");

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);

    TreeNew_GetSort(Context->TreeNewHandle, &sortColumn, &sortOrder);

    // Make sure we're not sorting by an invisible column.
    if (sortOrder != NoSortOrder && !(TreeNew_GetColumn(Context->TreeNewHandle, sortColumn, &column) && column.Visible))
    {
        TreeNew_SetSort(Context->TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CYCLESDELTA, DescendingSortOrder);
    }
}

VOID PhSaveSettingsThreadList(
    _Inout_ PPH_THREAD_LIST_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);

    PhSetIntegerSetting(L"ThreadTreeListFlags", Context->Flags);
    PhSetStringSetting2(L"ThreadTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"ThreadTreeListSort", &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSetOptionsThreadList(
    _Inout_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ ULONG Options
    )
{
    switch (Options)
    {
    case PH_THREAD_TREELIST_MENUITEM_HIDE_SUSPENDED:
        Context->HideSuspended = !Context->HideSuspended;
        break;
    case PH_THREAD_TREELIST_MENUITEM_HIDE_GUITHREADS:
        Context->HideGuiThreads = !Context->HideGuiThreads;
        break;
    case PH_THREAD_TREELIST_MENUITEM_HIGHLIGHT_SUSPENDED:
        Context->HighlightSuspended = !Context->HighlightSuspended;
        break;
    case PH_THREAD_TREELIST_MENUITEM_HIGHLIGHT_GUITHREADS:
        Context->HighlightGuiThreads = !Context->HighlightGuiThreads;
        break;
    }
}

PPH_THREAD_NODE PhAddThreadNode(
    _Inout_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ PPH_THREAD_ITEM ThreadItem,
    _In_ BOOLEAN FirstRun
    )
{
    PPH_THREAD_NODE threadNode;

    threadNode = PhAllocate(PhEmGetObjectSize(EmThreadNodeType, sizeof(PH_THREAD_NODE)));
    memset(threadNode, 0, sizeof(PH_THREAD_NODE));
    PhInitializeTreeNewNode(&threadNode->Node);

    if (Context->EnableStateHighlighting && !FirstRun)
    {
        PhChangeShStateTn(
            &threadNode->Node,
            &threadNode->ShState,
            &Context->NodeStateList,
            NewItemState,
            PhCsColorNew,
            NULL
            );
    }

    PhReferenceObject(ThreadItem);
    threadNode->ThreadId = ThreadItem->ThreadId;
    threadNode->ThreadItem = ThreadItem;
    threadNode->PagePriority = MEMORY_PRIORITY_NORMAL + 1;
    threadNode->IoPriority = MaxIoPriorityTypes;
    threadNode->LastSystemCallNumber = USHRT_MAX;

    memset(threadNode->TextCache, 0, sizeof(PH_STRINGREF) * PH_THREAD_TREELIST_COLUMN_MAXIMUM);
    threadNode->Node.TextCache = threadNode->TextCache;
    threadNode->Node.TextCacheSize = PH_THREAD_TREELIST_COLUMN_MAXIMUM;

    PhAddEntryHashtable(Context->NodeHashtable, &threadNode);
    PhAddItemList(Context->NodeList, threadNode);

    PhEmCallObjectOperation(EmThreadNodeType, threadNode, EmObjectCreate);

    TreeNew_NodesStructured(Context->TreeNewHandle);

    return threadNode;
}

PPH_THREAD_NODE PhFindThreadNode(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ HANDLE ThreadId
    )
{
    PH_THREAD_NODE lookupThreadNode;
    PPH_THREAD_NODE lookupThreadNodePtr = &lookupThreadNode;
    PPH_THREAD_NODE *threadNode;

    lookupThreadNode.ThreadId = ThreadId;

    threadNode = (PPH_THREAD_NODE *)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupThreadNodePtr
        );

    if (threadNode)
        return *threadNode;
    else
        return NULL;
}

VOID PhRemoveThreadNode(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ PPH_THREAD_NODE ThreadNode
    )
{
    // Remove from the hashtable here to avoid problems in case the key is re-used.
    PhRemoveEntryHashtable(Context->NodeHashtable, &ThreadNode);

    if (Context->EnableStateHighlighting)
    {
        PhChangeShStateTn(
            &ThreadNode->Node,
            &ThreadNode->ShState,
            &Context->NodeStateList,
            RemovingItemState,
            PhCsColorRemoved,
            Context->TreeNewHandle
            );
    }
    else
    {
        PhpRemoveThreadNode(ThreadNode, Context);
    }
}

VOID PhpDestroyThreadNode(
    _In_ PPH_THREAD_NODE ThreadNode
    )
{
    PhEmCallObjectOperation(EmThreadNodeType, ThreadNode, EmObjectDelete);

    if (ThreadNode->CyclesDeltaText) PhDereferenceObject(ThreadNode->CyclesDeltaText);
    if (ThreadNode->StartAddressText) PhDereferenceObject(ThreadNode->StartAddressText);
    if (ThreadNode->PrioritySymbolicText) PhDereferenceObject(ThreadNode->PrioritySymbolicText);
    if (ThreadNode->CreatedText) PhDereferenceObject(ThreadNode->CreatedText);
    if (ThreadNode->NameText) PhDereferenceObject(ThreadNode->NameText);
    if (ThreadNode->StateText) PhDereferenceObject(ThreadNode->StateText);

    if (ThreadNode->LastErrorCodeText) PhDereferenceObject(ThreadNode->LastErrorCodeText);
    if (ThreadNode->LastSystemCallText) PhDereferenceObject(ThreadNode->LastSystemCallText);
    if (ThreadNode->ThreadContextHandle) NtClose(ThreadNode->ThreadContextHandle);
    if (ThreadNode->ThreadReadVmHandle) NtClose(ThreadNode->ThreadReadVmHandle);

    PhDereferenceObject(ThreadNode->ThreadItem);

    PhFree(ThreadNode);
}

VOID PhpRemoveThreadNode(
    _In_ PPH_THREAD_NODE ThreadNode,
    _In_ PPH_THREAD_LIST_CONTEXT Context // PH_TICK_SH_STATE requires this parameter to be after ThreadNode
    )
{
    ULONG index;

    // Remove from list and cleanup.

    if ((index = PhFindItemList(Context->NodeList, ThreadNode)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);

    PhpDestroyThreadNode(ThreadNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhUpdateThreadNode(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ PPH_THREAD_NODE ThreadNode
    )
{
    memset(ThreadNode->TextCache, 0, sizeof(PH_STRINGREF) * PH_THREAD_TREELIST_COLUMN_MAXIMUM);

    ThreadNode->ValidMask = 0;
    PhInvalidateTreeNewNode(&ThreadNode->Node, TN_CACHE_COLOR);
    TreeNew_InvalidateNode(Context->TreeNewHandle, &ThreadNode->Node);
}

VOID PhTickThreadNodes(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    )
{
    // Text invalidation, node updates

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_THREAD_NODE node = Context->NodeList->Items[i];

        // The TID never changes, so we don't invalidate that.
        memset(&node->TextCache[1], 0, sizeof(PH_STRINGREF) * (PH_THREAD_TREELIST_COLUMN_MAXIMUM - 1));
        node->ValidMask = 0; // Items that always remain valid
    }

    if (Context->TreeNewSortOrder != NoSortOrder)
    {
        // Force a rebuild to sort the items.
        TreeNew_NodesStructured(Context->TreeNewHandle);
    }

    // State highlighting
    PH_TICK_SH_STATE_TN(PH_THREAD_NODE, ShState, Context->NodeStateList, PhpRemoveThreadNode, PhCsHighlightingDuration, Context->TreeNewHandle, TRUE, NULL, Context);
}

#define SORT_FUNCTION(Column) PhpThreadTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpThreadTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_THREAD_NODE node1 = *(PPH_THREAD_NODE *)_elem1; \
    PPH_THREAD_NODE node2 = *(PPH_THREAD_NODE *)_elem2; \
    PPH_THREAD_ITEM threadItem1 = node1->ThreadItem; \
    PPH_THREAD_ITEM threadItem2 = node2->ThreadItem; \
    PPH_THREAD_LIST_CONTEXT context = (PPH_THREAD_LIST_CONTEXT)_context; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->ThreadId, (ULONG_PTR)node2->ThreadId); \
    \
    return PhModifySort(sortResult, context->TreeNewSortOrder); \
}

LONG PhpThreadTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPH_THREAD_NODE)Node1)->ThreadId, (ULONG_PTR)((PPH_THREAD_NODE)Node2)->ThreadId);

    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(Tid)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->ThreadId, (ULONG_PTR)node2->ThreadId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Cpu)
{
    sortResult = singlecmp(threadItem1->CpuUsage, threadItem2->CpuUsage);

    if (sortResult == 0)
    {
        if (context->UseCycleTime)
            sortResult = uint64cmp(threadItem1->CyclesDelta.Delta, threadItem2->CyclesDelta.Delta);
        else
            sortResult = uintcmp(threadItem1->ContextSwitchesDelta.Delta, threadItem2->ContextSwitchesDelta.Delta);
    }
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(CyclesDelta)
{
    if (context->UseCycleTime)
        sortResult = uint64cmp(threadItem1->CyclesDelta.Delta, threadItem2->CyclesDelta.Delta);
    else
        sortResult = uintcmp(threadItem1->ContextSwitchesDelta.Delta, threadItem2->ContextSwitchesDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StartAddress)
{
    sortResult = PhCompareStringWithNull(threadItem1->StartAddressString, threadItem2->StartAddressString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PrioritySymbolic)
{
    sortResult = intcmp(threadItem1->BasePriorityIncrement, threadItem2->BasePriorityIncrement);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Service)
{
    sortResult = PhCompareStringWithNull(threadItem1->ServiceName, threadItem2->ServiceName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNull(node1->NameText, node2->NameText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Created)
{
    sortResult = uint64cmp(threadItem1->CreateTime.QuadPart, threadItem2->CreateTime.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StartModule)
{
    sortResult = PhCompareStringWithNull(threadItem1->StartAddressFileName, threadItem2->StartAddressFileName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ContextSwitches)
{
    sortResult = uint64cmp(threadItem1->ContextSwitchesDelta.Value, threadItem2->ContextSwitchesDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ContextSwitchesDelta)
{
    sortResult = uint64cmp(threadItem1->ContextSwitchesDelta.Delta, threadItem2->ContextSwitchesDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Priority)
{
    sortResult = intcmp(threadItem1->Priority, threadItem2->Priority);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(BasePriority)
{
    sortResult = intcmp(threadItem1->BasePriority, threadItem2->BasePriority);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PagePriority)
{
    sortResult = uintcmp(node1->PagePriority, node2->PagePriority);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoPriority)
{
    sortResult = uintcmp(node1->IoPriority, node2->IoPriority);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Cycles)
{
    sortResult = uint64cmp(threadItem1->CyclesDelta.Value, threadItem2->CyclesDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(State)
{
    ULONG threadState1 = ULONG_MAX;
    ULONG threadState2 = ULONG_MAX;

    if (threadItem1->State != Waiting)
    {
        if ((ULONG)threadItem1->State < MaximumThreadState)
            threadState1 = (ULONG)threadItem1->State;
        else
            threadState1 = (ULONG)MaximumThreadState;
    }
    else
    {
        if ((ULONG)threadItem1->WaitReason < MaximumWaitReason)
            threadState1 = (ULONG)threadItem1->WaitReason;
        else
            threadState1 = (ULONG)MaximumWaitReason;
    }

    if (threadItem2->State != Waiting)
    {
        if ((ULONG)threadItem2->State < MaximumThreadState)
            threadState2 = (ULONG)threadItem2->State;
        else
            threadState2 = (ULONG)MaximumThreadState;
    }
    else
    {
        if ((ULONG)threadItem2->WaitReason < MaximumWaitReason)
            threadState2 = (ULONG)threadItem2->WaitReason;
        else
            threadState2 = (ULONG)MaximumWaitReason;
    }

    sortResult = uintcmp(threadState1, threadState2);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(KernelTime)
{
    sortResult = uint64cmp(threadItem1->KernelTime.QuadPart, threadItem2->KernelTime.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(UserTime)
{
    sortResult = uint64cmp(threadItem1->UserTime.QuadPart, threadItem2->UserTime.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IdealProcessor)
{
    sortResult = int64cmp(node1->IdealProcessorMask, node2->IdealProcessorMask);
    //sortResult = PhCompareStringZ(node1->IdealProcessorText, node2->IdealProcessorText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Critical)
{
    sortResult = ucharcmp(node1->BreakOnTermination, node2->BreakOnTermination);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(TidHex)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->ThreadId, (ULONG_PTR)node2->ThreadId);
    //sortResult = ucharcmp(node1->ThreadIdHexText, node2->ThreadIdHexText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(CpuCore)
{
    DOUBLE cpuUsage1;
    DOUBLE cpuUsage2;

    cpuUsage1 = threadItem1->CpuUsage * 100;
    cpuUsage1 *= PhSystemProcessorInformation.NumberOfProcessors;

    cpuUsage2 = threadItem2->CpuUsage * 100;
    cpuUsage2 *= PhSystemProcessorInformation.NumberOfProcessors;

    sortResult = doublecmp(cpuUsage1, cpuUsage2);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(TokenState)
{
    sortResult = uintcmp(node1->TokenState, node2->TokenState);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PendingIrp)
{
    sortResult = ucharcmp(node1->PendingIrp, node2->PendingIrp);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LastSystemCall)
{
    sortResult = ushortcmp(node1->LastSystemCallNumber, node2->LastSystemCallNumber);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LastStatusCode)
{
    sortResult = uintcmp(node1->LastStatusCode, node2->LastStatusCode);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ApartmentState)
{
    sortResult = uintcmp(node1->ApartmentState, node2->ApartmentState);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Fiber)
{
    sortResult = ucharcmp(node1->Fiber, node2->Fiber);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PriorityBoost)
{
    sortResult = ucharcmp(node1->PriorityBoost, node2->PriorityBoost);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(CpuUser)
{
    sortResult = singlecmp(threadItem1->CpuUserUsage, threadItem2->CpuUserUsage);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(CpuKernel)
{
    sortResult = singlecmp(threadItem1->CpuKernelUsage, threadItem2->CpuKernelUsage);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StackUsage)
{
    sortResult = singlecmp(node1->StackUsage, node2->StackUsage);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(WaitTime)
{
    sortResult = uint64cmp(threadItem1->WaitTime, threadItem2->WaitTime);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpThreadTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPH_THREAD_LIST_CONTEXT context = Context;
    PPH_THREAD_NODE node;

    if (!context)
        return FALSE;
    if (PhCmForwardMessage(hwnd, Message, Parameter1, Parameter2, &context->Cm))
        return TRUE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Tid),
                    SORT_FUNCTION(Cpu),
                    SORT_FUNCTION(CyclesDelta),
                    SORT_FUNCTION(StartAddress),
                    SORT_FUNCTION(PrioritySymbolic),
                    SORT_FUNCTION(Service),
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Created),
                    SORT_FUNCTION(StartModule),
                    SORT_FUNCTION(ContextSwitches),
                    SORT_FUNCTION(ContextSwitchesDelta), // delta
                    SORT_FUNCTION(Priority),
                    SORT_FUNCTION(BasePriority),
                    SORT_FUNCTION(PagePriority),
                    SORT_FUNCTION(IoPriority),
                    SORT_FUNCTION(Cycles),
                    SORT_FUNCTION(State),
                    SORT_FUNCTION(KernelTime),
                    SORT_FUNCTION(UserTime),
                    SORT_FUNCTION(IdealProcessor),
                    SORT_FUNCTION(Critical),
                    SORT_FUNCTION(TidHex),
                    SORT_FUNCTION(CpuCore),
                    SORT_FUNCTION(TokenState),
                    SORT_FUNCTION(PendingIrp),
                    SORT_FUNCTION(LastSystemCall),
                    SORT_FUNCTION(LastStatusCode),
                    SORT_FUNCTION(Created), // Timeline
                    SORT_FUNCTION(ApartmentState),
                    SORT_FUNCTION(Fiber),
                    SORT_FUNCTION(PriorityBoost),
                    SORT_FUNCTION(CpuUser),
                    SORT_FUNCTION(CpuKernel),
                    SORT_FUNCTION(StackUsage),
                    SORT_FUNCTION(WaitTime),
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                if (!PhCmForwardSort(
                    (PPH_TREENEW_NODE *)context->NodeList->Items,
                    context->NodeList->Count,
                    context->TreeNewSortColumn,
                    context->TreeNewSortOrder,
                    &context->Cm
                    ))
                {
                    if (context->TreeNewSortColumn < PH_THREAD_TREELIST_COLUMN_MAXIMUM)
                        sortFunction = sortFunctions[context->TreeNewSortColumn];
                    else
                        sortFunction = NULL;
                }
                else
                {
                    sortFunction = NULL;
                }

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
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            if (!isLeaf)
                break;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PPH_THREAD_ITEM threadItem;

            if (!getCellText)
                break;

            node = (PPH_THREAD_NODE)getCellText->Node;
            threadItem = node->ThreadItem;

            switch (getCellText->Id)
            {
            case PH_THREAD_TREELIST_COLUMN_TID:
                {
                    PH_FORMAT format;
                    SIZE_T returnLength;

                    PhInitFormatIU(&format, HandleToUlong(threadItem->ThreadId));

                    if (PhFormatToBuffer(&format, 1, node->ThreadIdText, sizeof(node->ThreadIdText), &returnLength))
                    {
                        getCellText->Text.Buffer = node->ThreadIdText;
                        getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_CPU:
                {
                    FLOAT cpuUsage;

                    cpuUsage = threadItem->CpuUsage * 100;

                    if (cpuUsage >= 0.01)
                    {
                        PH_FORMAT format;
                        SIZE_T returnLength;

                        PhInitFormatF(&format, cpuUsage, PhMaxPrecisionUnit);

                        if (PhFormatToBuffer(&format, 1, node->CpuUsageText, sizeof(node->CpuUsageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->CpuUsageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                    else if (cpuUsage != 0 && PhCsShowCpuBelow001)
                    {
                        PH_FORMAT format[2];
                        SIZE_T returnLength;

                        PhInitFormatS(&format[0], L"< ");
                        PhInitFormatF(&format[1], cpuUsage, PhMaxPrecisionUnit);

                        if (PhFormatToBuffer(format, 2, node->CpuUsageText, sizeof(node->CpuUsageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->CpuUsageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_CYCLESDELTA:
                {
                    if (context->UseCycleTime)
                    {
                        if (threadItem->CyclesDelta.Delta != threadItem->CyclesDelta.Value && threadItem->CyclesDelta.Delta != 0)
                        {
                            PhMoveReference(&node->CyclesDeltaText, PhFormatUInt64(threadItem->CyclesDelta.Delta, TRUE));
                            getCellText->Text = node->CyclesDeltaText->sr;
                        }
                    }
                    else
                    {
                        if (threadItem->ContextSwitchesDelta.Delta != threadItem->ContextSwitchesDelta.Value && threadItem->ContextSwitchesDelta.Delta != 0)
                        {
                            PhMoveReference(&node->CyclesDeltaText, PhFormatUInt64(threadItem->ContextSwitchesDelta.Delta, TRUE));
                            getCellText->Text = node->CyclesDeltaText->sr;
                        }
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_STARTADDRESS:
                PhSwapReference(&node->StartAddressText, threadItem->StartAddressString);
                getCellText->Text = PhGetStringRef(node->StartAddressText);
                break;
            case PH_THREAD_TREELIST_COLUMN_PRIORITYSYMBOLIC:
                PhMoveReference(&node->PrioritySymbolicText, PhGetBasePriorityIncrementString(threadItem->BasePriorityIncrement));
                getCellText->Text = PhGetStringRef(node->PrioritySymbolicText);
                break;
            case PH_THREAD_TREELIST_COLUMN_SERVICE:
                getCellText->Text = PhGetStringRef(threadItem->ServiceName);
                break;
            case PH_THREAD_TREELIST_COLUMN_NAME:
                {
                    if (threadItem->ThreadHandle && WindowsVersion >= WINDOWS_10_RS1)
                    {
                        PPH_STRING threadName;

                        if (NT_SUCCESS(PhGetThreadName(threadItem->ThreadHandle, &threadName)))
                        {
                            PhMoveReference(&node->NameText, threadName);
                        }
                    }

                    getCellText->Text = PhGetStringRef(node->NameText);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_STARTED:
                {
                    SYSTEMTIME time;

                    PhLargeIntegerToLocalSystemTime(&time, &threadItem->CreateTime);
                    PhMoveReference(&node->CreatedText, PhFormatDateTime(&time));

                    getCellText->Text = PhGetStringRef(node->CreatedText);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_STARTMODULE:
                getCellText->Text = PhGetStringRef(threadItem->StartAddressFileName);
                break;
            case PH_THREAD_TREELIST_COLUMN_CONTEXTSWITCHES:
                {
                    SIZE_T returnLength;
                    PH_FORMAT format[1];

                    PhInitFormatI64UGroupDigits(&format[0], threadItem->ContextSwitchesDelta.Value);

                    if (PhFormatToBuffer(format, 1, node->ContextSwitchesText, sizeof(node->ContextSwitchesText), &returnLength))
                    {
                        getCellText->Text.Buffer = node->ContextSwitchesText;
                        getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_CONTEXTSWITCHESDELTA:
                {
                    if ((LONG)threadItem->ContextSwitchesDelta.Delta >= 0) // the delta may be negative if a thread exits - just don't show anything
                    {
                        ULONG value = threadItem->ContextSwitchesDelta.Delta;

                        if (value != 0)
                        {
                            PhMoveReference(&node->ContextSwitchesDeltaText, PhFormatUInt64(value, TRUE));
                            getCellText->Text = node->ContextSwitchesDeltaText->sr;
                        }
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_PRIORITY:
                {
                    SIZE_T returnLength;
                    PH_FORMAT format[1];

                    PhInitFormatD(&format[0], threadItem->Priority);

                    if (PhFormatToBuffer(format, 1, node->PriorityText, sizeof(node->PriorityText), &returnLength))
                    {
                        getCellText->Text.Buffer = node->PriorityText;
                        getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_BASEPRIORITY:
                {
                    SIZE_T returnLength;
                    PH_FORMAT format[1];

                    PhInitFormatD(&format[0], threadItem->BasePriority);

                    if (PhFormatToBuffer(format, 1, node->BasePriorityText, sizeof(node->BasePriorityText), &returnLength))
                    {
                        getCellText->Text.Buffer = node->BasePriorityText;
                        getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_PAGEPRIORITY:
                {
                    ULONG pagePriorityInteger = MEMORY_PRIORITY_NORMAL + 1;
                    PWSTR pagePriority = L"N/A";

                    if (threadItem->ThreadHandle)
                    {
                        if (NT_SUCCESS(PhGetThreadPagePriority(threadItem->ThreadHandle, &pagePriorityInteger)) && pagePriorityInteger <= MEMORY_PRIORITY_NORMAL)
                        {
                            node->PagePriority = pagePriorityInteger;
                            pagePriority = PhPagePriorityNames[pagePriorityInteger];
                        }
                    }

                    PhInitializeStringRefLongHint(&getCellText->Text, pagePriority);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_IOPRIORITY:
                {
                    IO_PRIORITY_HINT ioPriorityInteger = MaxIoPriorityTypes;
                    PWSTR ioPriority = L"N/A";

                    if (threadItem->ThreadHandle)
                    {
                        if (NT_SUCCESS(PhGetThreadIoPriority(threadItem->ThreadHandle, &ioPriorityInteger)) && ioPriorityInteger < MaxIoPriorityTypes)
                        {
                            node->IoPriority = ioPriorityInteger;
                            ioPriority = PhIoPriorityHintNames[ioPriorityInteger];
                        }
                    }

                    PhInitializeStringRefLongHint(&getCellText->Text, ioPriority);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_CYCLES:
                {
                    if (threadItem->CyclesDelta.Value != 0)
                    {
                        SIZE_T returnLength;
                        PH_FORMAT format[1];

                        PhInitFormatI64UGroupDigits(&format[0], threadItem->CyclesDelta.Value);

                        if (PhFormatToBuffer(format, 1, node->CyclesText, sizeof(node->CyclesText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->CyclesText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_STATE:
                {
                    ULONG suspendCount;

                    if (threadItem->State != Waiting)
                    {
                        if ((ULONG)threadItem->State < MaximumThreadState)
                            PhMoveReference(&node->StateText, PhCreateString(PhKThreadStateNames[(ULONG)threadItem->State]));
                        else
                            PhMoveReference(&node->StateText, PhCreateString(L"Unknown"));
                    }
                    else
                    {
                        if ((ULONG)threadItem->WaitReason < MaximumWaitReason)
                            PhMoveReference(&node->StateText, PhConcatStrings2(L"Wait:", PhKWaitReasonNames[(ULONG)threadItem->WaitReason]));
                        else
                            PhMoveReference(&node->StateText, PhCreateString(L"Waiting"));
                    }

                    if (threadItem->ThreadHandle)
                    {
                        if (threadItem->WaitReason == Suspended && NT_SUCCESS(PhGetThreadSuspendCount(threadItem->ThreadHandle, &suspendCount)))
                        {
                            PH_FORMAT format[4];

                            PhInitFormatSR(&format[0], node->StateText->sr);
                            PhInitFormatS(&format[1], L" (");
                            PhInitFormatU(&format[2], suspendCount);
                            PhInitFormatS(&format[3], L")");

                            PhMoveReference(&node->StateText, PhFormat(format, 4, 30));
                        }
                    }

                    getCellText->Text = PhGetStringRef(node->StateText);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_KERNELTIME:
                {
                    PhPrintTimeSpan(node->KernelTimeText, threadItem->KernelTime.QuadPart, PH_TIMESPAN_HMSM);

                    PhInitializeStringRefLongHint(&getCellText->Text, node->KernelTimeText);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_USERTIME:
                {
                    PhPrintTimeSpan(node->UserTimeText, threadItem->UserTime.QuadPart, PH_TIMESPAN_HMSM);

                    PhInitializeStringRefLongHint(&getCellText->Text, node->UserTimeText);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_IDEALPROCESSOR:
                {
                    PROCESSOR_NUMBER idealProcessorNumber;
                    SIZE_T returnLength;
                    PH_FORMAT format[3];

                    if (threadItem->ThreadHandle)
                    {
                        if (NT_SUCCESS(PhGetThreadIdealProcessor(threadItem->ThreadHandle, &idealProcessorNumber)))
                        {
                            node->IdealProcessorMask = MAKELONG(idealProcessorNumber.Number, idealProcessorNumber.Group);

                            PhInitFormatU(&format[0], idealProcessorNumber.Group);
                            PhInitFormatC(&format[1], L':');
                            PhInitFormatU(&format[2], idealProcessorNumber.Number);

                            if (PhFormatToBuffer(format, 3, node->IdealProcessorText, sizeof(node->IdealProcessorText), &returnLength))
                            {
                                getCellText->Text.Buffer = node->IdealProcessorText;
                                getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                            }
                        }
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_CRITICAL:
                {
                    BOOLEAN breakOnTermination = FALSE;

                    if (threadItem->ThreadHandle)
                    {
                        PhGetThreadBreakOnTermination(threadItem->ThreadHandle, &breakOnTermination);
                    }

                    if (breakOnTermination)
                        PhInitializeStringRef(&getCellText->Text, L"Critical");
                    else
                        PhInitializeEmptyStringRef(&getCellText->Text);

                    node->BreakOnTermination = breakOnTermination;
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_TIDHEX:
                {
                    PH_FORMAT format;
                    SIZE_T returnLength;

                    PhInitFormatIX(&format, HandleToUlong(threadItem->ThreadId));

                    if (PhFormatToBuffer(&format, 1, node->ThreadIdHexText, sizeof(node->ThreadIdHexText), &returnLength))
                    {
                        getCellText->Text.Buffer = node->ThreadIdHexText;
                        getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_CPUCORECYCLES:
                {
                    FLOAT cpuUsage;

                    cpuUsage = threadItem->CpuUsage * 100;
                    cpuUsage *= PhSystemProcessorInformation.NumberOfProcessors; // linux style (dmex)

                    if (cpuUsage >= 0.01)
                    {
                        PH_FORMAT format;
                        SIZE_T returnLength;

                        PhInitFormatF(&format, cpuUsage, PhMaxPrecisionUnit);

                        if (PhFormatToBuffer(&format, 1, node->CpuCoreUsageText, sizeof(node->CpuCoreUsageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->CpuCoreUsageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                    else if (cpuUsage != 0 && PhCsShowCpuBelow001)
                    {
                        PH_FORMAT format[2];
                        SIZE_T returnLength;

                        PhInitFormatS(&format[0], L"< ");
                        PhInitFormatF(&format[1], cpuUsage, PhMaxPrecisionUnit);

                        if (PhFormatToBuffer(format, 2, node->CpuCoreUsageText, sizeof(node->CpuCoreUsageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->CpuCoreUsageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_TOKEN_STATE:
                {
                    if (threadItem->ThreadHandle)
                    {
                        NTSTATUS status;
                        HANDLE tokenHandle;

                        // Since the system must verify that the thread is impersonating before it can perform
                        // an access check on the target token, we can reliably determine the token's presence
                        // even if we fail the subsequent access check. (diversenok)

                        status = NtOpenThreadToken(threadItem->ThreadHandle, 0, TRUE, &tokenHandle);

                        if (status == STATUS_NO_TOKEN)
                        {
                            node->TokenState = PH_THREAD_TOKEN_STATE_NOT_PRESENT;
                            PhInitializeEmptyStringRef(&getCellText->Text);
                        }
                        else if (status == STATUS_CANT_OPEN_ANONYMOUS)
                        {
                            node->TokenState = PH_THREAD_TOKEN_STATE_ANONYMOUS;
                            PhInitializeStringRef(&getCellText->Text, L"Anonymous");
                        }
                        else
                        {
                            node->TokenState = PH_THREAD_TOKEN_STATE_PRESENT;
                            PhInitializeStringRef(&getCellText->Text, L"Yes");
                        }

                        if (NT_SUCCESS(status))
                            NtClose(tokenHandle);
                    }
                    else
                    {
                        node->TokenState = PH_THREAD_TOKEN_STATE_UNKNOWN;
                        PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_PENDINGIRP:
                {
                    BOOLEAN pendingIrp = FALSE;

                    if (threadItem->ThreadHandle)
                    {
                        PhGetThreadIsIoPending(threadItem->ThreadHandle, &pendingIrp);
                    }

                    if (pendingIrp)
                        PhInitializeStringRef(&getCellText->Text, L"Yes");
                    else
                        PhInitializeEmptyStringRef(&getCellText->Text);

                    node->PendingIrp = pendingIrp;
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_LASTSYSTEMCALL:
                {
                    NTSTATUS status = STATUS_UNSUCCESSFUL;
                    THREAD_LAST_SYSCALL_INFORMATION lastSystemCall;

                    if (context->ProcessId == SYSTEM_IDLE_PROCESS_ID || context->ProcessId == SYSTEM_PROCESS_ID)
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                        break;
                    }

                    if (!node->ThreadContextHandleValid)
                    {
                        if (!node->ThreadContextHandle)
                        {
                            HANDLE threadHandle;

                            if (NT_SUCCESS(PhOpenThread(
                                &threadHandle,
                                THREAD_GET_CONTEXT,
                                threadItem->ThreadId
                                )))
                            {
                                node->ThreadContextHandle = threadHandle;
                            }
                        }

                        node->ThreadContextHandleValid = TRUE;
                    }

                    if (node->ThreadContextHandle)
                    {
                        status = PhGetThreadLastSystemCall(
                            node->ThreadContextHandle,
                            &lastSystemCall
                            );
                    }

                    if (NT_SUCCESS(status))
                    {
                        PPH_STRING systemCallName;
                        PH_FORMAT format[6];

                        if (lastSystemCall.SystemCallNumber == 0 && !lastSystemCall.FirstArgument)
                        {
                            // If the thread was created in a frozen/suspended process and hasn't executed, the
                            // ThreadLastSystemCall returns status_success but the values are invalid. (dmex)
                            PhMoveReference(&node->LastSystemCallText, PhReferenceEmptyString());
                        }
                        else
                        {
                            if (systemCallName = PhGetSystemCallNumberName(lastSystemCall.SystemCallNumber))
                            {
                                PhInitFormatSR(&format[0], systemCallName->sr);
                                PhInitFormatS(&format[1], L" (0x");
                                PhInitFormatX(&format[2], lastSystemCall.SystemCallNumber);
                                PhInitFormatS(&format[3], L") (Arg0: 0x");
                                PhInitFormatI64X(&format[4], (ULONG64)lastSystemCall.FirstArgument);
                                PhInitFormatC(&format[5], L')');

                                PhMoveReference(&node->LastSystemCallText, PhFormat(format, 6, 0x40));
                                PhDereferenceObject(systemCallName);
                            }
                            else
                            {
                                PhInitFormatS(&format[0], L"0x");
                                PhInitFormatX(&format[1], lastSystemCall.SystemCallNumber);
                                PhInitFormatS(&format[2], L" (Arg0: 0x");
                                PhInitFormatI64X(&format[3], (ULONG64)lastSystemCall.FirstArgument);
                                PhInitFormatC(&format[4], L')');

                                PhMoveReference(&node->LastSystemCallText, PhFormat(format, 5, 0x40));
                            }
                        }

                        node->LastSystemCallNumber = lastSystemCall.SystemCallNumber;
                    }
                    else
                    {
                        PH_FORMAT format[2];

                        PhInitFormatS(&format[0], L"0x");
                        PhInitFormatX(&format[1], status);

                        node->LastSystemCallNumber = USHRT_MAX;
                        PhMoveReference(&node->LastSystemCallText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                    }

                    getCellText->Text = PhGetStringRef(node->LastSystemCallText);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_LASTSTATUSCODE:
                {
                    NTSTATUS status = STATUS_UNSUCCESSFUL;
                    NTSTATUS lastStatusValue = STATUS_SUCCESS;
                    PPH_STRING errorMessage;

                    if (context->ProcessId == SYSTEM_IDLE_PROCESS_ID || context->ProcessId == SYSTEM_PROCESS_ID)
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                        break;
                    }

                    if (!node->ThreadReadVmHandleValid)
                    {
                        if (!node->ThreadReadVmHandle)
                        {
                            if (threadItem->ThreadHandle)
                            {
                                HANDLE processHandle;

                                if (NT_SUCCESS(PhOpenProcess(
                                    &processHandle,
                                    PROCESS_VM_READ | (WindowsVersion > WINDOWS_7 ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_QUERY_INFORMATION),
                                    context->ProcessId
                                    )))
                                {
                                    node->ThreadReadVmHandle = processHandle;
                                }
                            }
                        }

                        node->ThreadReadVmHandleValid = TRUE;
                    }

                    if (threadItem->ThreadHandle && node->ThreadReadVmHandle)
                    {
                        status = PhGetThreadLastStatusValue(
                            threadItem->ThreadHandle,
                            node->ThreadReadVmHandle,
                            &lastStatusValue
                            );
                    }

                    if (NT_SUCCESS(status))
                    {
                        if (lastStatusValue == STATUS_SUCCESS)
                        {
                            node->LastStatusCode = 0;
                            PhClearReference(&node->LastErrorCodeText);
                        }
                        else
                        {
                            PH_FORMAT format[5];

                            PhInitFormatS(&format[0], L"0x");
                            PhInitFormatX(&format[1], lastStatusValue);

                            if (errorMessage = PhGetStatusMessage(lastStatusValue, 0))
                            {
                                PhInitFormatS(&format[2], L" (");
                                PhInitFormatSR(&format[3], errorMessage->sr);
                                PhInitFormatC(&format[4], L')');

                                PhMoveReference(&node->LastErrorCodeText, PhFormat(format, 5, 0x40));
                                PhDereferenceObject(errorMessage);
                            }
                            else
                            {
                                PhMoveReference(&node->LastErrorCodeText, PhFormat(format, 2, 0x40));
                            }

                            node->LastStatusCode = lastStatusValue;
                        }
                    }
                    else
                    {
                        PH_FORMAT format[2];

                        PhInitFormatS(&format[0], L"0x");
                        PhInitFormatX(&format[1], status);

                        node->LastStatusCode = 0;
                        PhMoveReference(&node->LastErrorCodeText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                    }

                    getCellText->Text = PhGetStringRef(node->LastErrorCodeText);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_APARTMENTSTATE:
                {
                    NTSTATUS status = STATUS_UNSUCCESSFUL;
                    OLETLSFLAGS apartmentState = 0;

                    if (context->ProcessId == SYSTEM_IDLE_PROCESS_ID || context->ProcessId == SYSTEM_PROCESS_ID)
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                        break;
                    }

                    if (!node->ThreadReadVmHandleValid)
                    {
                        if (!node->ThreadReadVmHandle)
                        {
                            if (threadItem->ThreadHandle)
                            {
                                HANDLE processHandle;

                                if (NT_SUCCESS(PhOpenProcess(
                                    &processHandle,
                                    PROCESS_VM_READ | (WindowsVersion > WINDOWS_7 ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_QUERY_INFORMATION),
                                    context->ProcessId
                                    )))
                                {
                                    node->ThreadReadVmHandle = processHandle;
                                }
                            }
                        }

                        node->ThreadReadVmHandleValid = TRUE;
                    }

                    if (threadItem->ThreadHandle && node->ThreadReadVmHandle)
                    {
                        status = PhGetThreadApartmentState(
                            threadItem->ThreadHandle,
                            node->ThreadReadVmHandle,
                            &apartmentState
                            );
                    }

                    if (NT_SUCCESS(status) && apartmentState)
                    {
                        PPH_STRING apartmentStateString;

                        if (apartmentStateString = PhGetApartmentStateString(apartmentState))
                        {
                            PhMoveReference(&node->ApartmentStateText, apartmentStateString);
                        }
                        else
                        {
                            PhClearReference(&node->ApartmentStateText);
                        }

                        node->ApartmentState = apartmentState;
                    }
                    else
                    {
                        node->ApartmentState = 0;
                        PhClearReference(&node->ApartmentStateText);
                    }

                    getCellText->Text = PhGetStringRef(node->ApartmentStateText);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_FIBER:
                {
                    NTSTATUS status = STATUS_UNSUCCESSFUL;
                    BOOLEAN threadIsFiber = FALSE;

                    if (context->ProcessId == SYSTEM_IDLE_PROCESS_ID || context->ProcessId == SYSTEM_PROCESS_ID)
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                        break;
                    }

                    if (!node->ThreadReadVmHandleValid)
                    {
                        if (!node->ThreadReadVmHandle)
                        {
                            if (threadItem->ThreadHandle)
                            {
                                HANDLE processHandle;

                                if (NT_SUCCESS(PhOpenProcess(
                                    &processHandle,
                                    PROCESS_VM_READ | (WindowsVersion > WINDOWS_7 ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_QUERY_INFORMATION),
                                    context->ProcessId
                                    )))
                                {
                                    node->ThreadReadVmHandle = processHandle;
                                }
                            }
                        }

                        node->ThreadReadVmHandleValid = TRUE;
                    }

                    if (threadItem->ThreadHandle && node->ThreadReadVmHandle)
                    {
                        status = PhGetThreadIsFiber(
                            threadItem->ThreadHandle,
                            node->ThreadReadVmHandle,
                            &threadIsFiber
                            );
                    }

                    node->Fiber = threadIsFiber;

                    if (NT_SUCCESS(status) && threadIsFiber)
                    {
                        PhInitializeStringRef(&getCellText->Text, L"Yes");
                    }
                    else
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_PRIORITYBOOST:
                {
                    BOOLEAN priorityBoost = FALSE;

                    if (threadItem->ThreadHandle)
                    {
                        PhGetThreadPriorityBoost(threadItem->ThreadHandle, &priorityBoost);
                    }

                    node->PriorityBoost = priorityBoost;

                    if (priorityBoost)
                        PhInitializeStringRef(&getCellText->Text, L"Yes");
                    else
                        PhInitializeEmptyStringRef(&getCellText->Text);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_CPUUSER:
                {
                    FLOAT cpuUsage;

                    cpuUsage = threadItem->CpuUserUsage * 100;

                    if (cpuUsage >= 0.01)
                    {
                        PH_FORMAT format;
                        SIZE_T returnLength;

                        PhInitFormatF(&format, cpuUsage, PhMaxPrecisionUnit);

                        if (PhFormatToBuffer(&format, 1, node->CpuUserUsageText, sizeof(node->CpuUserUsageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->CpuUserUsageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                    else if (cpuUsage != 0 && PhCsShowCpuBelow001)
                    {
                        PH_FORMAT format[2];
                        SIZE_T returnLength;

                        PhInitFormatS(&format[0], L"< ");
                        PhInitFormatF(&format[1], cpuUsage, PhMaxPrecisionUnit);

                        if (PhFormatToBuffer(format, 2, node->CpuUserUsageText, sizeof(node->CpuUserUsageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->CpuUserUsageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_CPUKERNEL:
                {
                    FLOAT cpuUsage;

                    cpuUsage = threadItem->CpuKernelUsage * 100;

                    if (cpuUsage >= 0.01)
                    {
                        PH_FORMAT format;
                        SIZE_T returnLength;

                        PhInitFormatF(&format, cpuUsage, PhMaxPrecisionUnit);

                        if (PhFormatToBuffer(&format, 1, node->CpuKernelUsageText, sizeof(node->CpuKernelUsageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->CpuKernelUsageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                    else if (cpuUsage != 0 && PhCsShowCpuBelow001)
                    {
                        PH_FORMAT format[2];
                        SIZE_T returnLength;

                        PhInitFormatS(&format[0], L"< ");
                        PhInitFormatF(&format[1], cpuUsage, PhMaxPrecisionUnit);

                        if (PhFormatToBuffer(format, 2, node->CpuKernelUsageText, sizeof(node->CpuKernelUsageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->CpuKernelUsageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_STACKUSAGE:
                {
                    NTSTATUS status = STATUS_UNSUCCESSFUL;
                    ULONG_PTR stackUsage = 0;
                    ULONG_PTR stackLimit = 0;

                    if (context->ProcessId == SYSTEM_IDLE_PROCESS_ID || context->ProcessId == SYSTEM_PROCESS_ID)
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                        break;
                    }

                    if (!node->ThreadReadVmHandleValid)
                    {
                        if (!node->ThreadReadVmHandle)
                        {
                            if (threadItem->ThreadHandle)
                            {
                                HANDLE processHandle;

                                if (NT_SUCCESS(PhOpenProcess(
                                    &processHandle,
                                    PROCESS_VM_READ | (WindowsVersion > WINDOWS_7 ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_QUERY_INFORMATION),
                                    context->ProcessId
                                    )))
                                {
                                    node->ThreadReadVmHandle = processHandle;
                                }
                            }
                        }

                        node->ThreadReadVmHandleValid = TRUE;
                    }

                    if (threadItem->ThreadHandle && node->ThreadReadVmHandle)
                    {
                        status = PhGetThreadStackSize(
                            threadItem->ThreadHandle,
                            node->ThreadReadVmHandle,
                            &stackUsage,
                            &stackLimit
                            );
                    }

                    if (NT_SUCCESS(status) && stackUsage && stackLimit)
                    {
                        FLOAT percent = (FLOAT)stackUsage / stackLimit * 100;
                        PH_FORMAT format[6];

                        // %s / %s (%.0f%%)
                        PhInitFormatSize(&format[0], stackUsage);
                        PhInitFormatS(&format[1], L" | ");
                        PhInitFormatSize(&format[2], stackLimit);
                        PhInitFormatS(&format[3], L" (");
                        PhInitFormatF(&format[4], percent, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[5], L"%)");

                        PhMoveReference(&node->StackUsageText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                        node->StackUsage = percent;
                    }
                    else
                    {
                        PhClearReference(&node->StackUsageText);
                        node->StackUsage = 0;
                    }

                    getCellText->Text = PhGetStringRef(node->StackUsageText);
                }
                break;
            case PH_THREAD_TREELIST_COLUMN_WAITTIME:
                {
                    PhPrintTimeSpan(node->WaitTimeText, threadItem->WaitTime, PH_TIMESPAN_HMSM);

                    PhInitializeStringRefLongHint(&getCellText->Text, node->WaitTimeText);
                }
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
            PPH_THREAD_ITEM threadItem;

            if (!getNodeColor)
                break;

            node = (PPH_THREAD_NODE)getNodeColor->Node;
            threadItem = node->ThreadItem;

            if (!threadItem)
                NOTHING;
            //else if (context->HighlightUnknownStartAddress && threadItem->StartAddressResolveLevel == PhsrlAddress)
            //    getNodeColor->BackColor = PhCsColorUnknown;
            else if (context->HighlightSuspended && threadItem->WaitReason == Suspended)
                getNodeColor->BackColor = PhCsColorSuspended;
            else if (context->HighlightGuiThreads && threadItem->IsGuiThread)
                getNodeColor->BackColor = PhCsColorGuiThreads;

            getNodeColor->Flags = TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;
            PPH_THREAD_ITEM threadItem;
            RECT rect;

            if (!customDraw)
                break;

            node = (PPH_THREAD_NODE)customDraw->Node;
            threadItem = node->ThreadItem;
            rect = customDraw->CellRect;

            if (rect.right - rect.left <= 1)
                break; // nothing to draw

            switch (customDraw->Column->Id)
            {
            case PH_THREAD_TREELIST_COLUMN_TIMELINE:
                {
                    if (threadItem->CreateTime.QuadPart == 0)
                        break; // nothing to draw

                    PhCustomDrawTreeTimeLine(
                        customDraw->Dc,
                        customDraw->CellRect,
                        PhEnableThemeSupport ? PH_DRAW_TIMELINE_OVERFLOW | PH_DRAW_TIMELINE_DARKTHEME : PH_DRAW_TIMELINE_OVERFLOW,
                        &context->ProcessCreateTime,
                        &threadItem->CreateTime
                        );
                }
                break;
            }
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);
            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            if (!keyEvent)
                break;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_THREAD_COPY, 0);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(context->TreeNewHandle, 0, -1);
                break;
            case VK_DELETE:
                SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_THREAD_TERMINATE, 0);
                break;
            case VK_RETURN:
                SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_THREAD_INSPECT, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            // Customizable columns are disabled until we can figure out how to make it
            // co-operate with the column adjustments (e.g. Cycles Delta vs Context Switches Delta,
            // Service column).

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = PH_THREAD_TREELIST_COLUMN_CYCLESDELTA;
            data.DefaultSortOrder = DescendingSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_THREAD_INSPECT, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenu = Parameter1;

            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_SHOWCONTEXTMENU, (LPARAM)contextMenu);
        }
        return TRUE;
    case TreeNewGetDialogCode:
        {
            PULONG code = Parameter2;

            if (PtrToUlong(Parameter1) == VK_RETURN)
            {
                *code = DLGC_WANTMESSAGE;
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

PPH_THREAD_ITEM PhGetSelectedThreadItem(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    )
{
    PPH_THREAD_ITEM threadItem = NULL;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_THREAD_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            threadItem = node->ThreadItem;
            break;
        }
    }

    return threadItem;
}

VOID PhGetSelectedThreadItems(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _Out_ PPH_THREAD_ITEM **Threads,
    _Out_ PULONG NumberOfThreads
    )
{
    PH_ARRAY array;
    ULONG i;

    PhInitializeArray(&array, sizeof(PVOID), 2);

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_THREAD_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            PhAddItemArray(&array, &node->ThreadItem);
    }

    *NumberOfThreads = (ULONG)array.Count;
    *Threads = PhFinalArrayItems(&array);
}

VOID PhDeselectAllThreadNodes(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    )
{
    TreeNew_DeselectRange(Context->TreeNewHandle, 0, -1);
}

// TODO: Move the below code to a better location. (dmex)
#include <mapimg.h>
//#include <symprv.h>

#define NUMBER_SERVICE_TABLES 2
#define NTOS_SERVICE_INDEX 0
#define WIN32K_SERVICE_INDEX 1
#define SERVICE_NUMBER_MASK ((1 << 12) -  1)

typedef struct _PH_SYSCALL_ENTRY
{
    ULONG_PTR Address;
    PPH_STRING Name;
} PH_SYSCALL_ENTRY, *PPH_SYSCALL_ENTRY;

typedef union _PH_SYSTEMCALL_ENTRY
{
    USHORT SystemCallNumber;
    struct
    {
        USHORT SystemCallIndex : 12;
        USHORT SystemServiceIndex : 4;
    };
} PH_SYSTEMCALL_ENTRY, *PPH_SYSTEMCALL_ENTRY;

static int __cdecl PhpSystemCallListIndexSort(
    _In_ const void* Context,
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_SYSCALL_ENTRY entry1 = *(PPH_SYSCALL_ENTRY*)elem1;
    PPH_SYSCALL_ENTRY entry2 = *(PPH_SYSCALL_ENTRY*)elem2;

    return uintptrcmp(entry1->Address, entry2->Address);
}

//VOID PhpGenerateWin32kSyscallListLegacy(
//    _Out_ PPH_LIST* Win32kSystemCallList
//    )
//{
//    static PH_STRINGREF system32FileNameSr = PH_STRINGREF_INIT(L"\\System32\\");
//    static PH_STRINGREF win32kFileNameSr = PH_STRINGREF_INIT(L"win32k.sys");
//    PPH_LIST win32kSystemCallList = NULL;
//    PH_MAPPED_IMAGE mappedImage;
//    PH_STRINGREF systemRootString;
//    PPH_STRING fileName;
//
//    PhGetSystemRoot(&systemRootString);
//    fileName = PhConcatStringRef3(&systemRootString, &system32FileNameSr, &win32kFileNameSr);
//
//    if (NT_SUCCESS(PhLoadMappedImage(PhGetString(fileName), NULL, &mappedImage)))
//    {
//        PPH_SYMBOL_PROVIDER symbolProvider;
//        ULONG serviceTableLimit = 0;
//        PVOID serviceTableAddress = NULL;
//
//        if (symbolProvider = PhCreateSymbolProvider(NULL))
//        {
//            PhLoadSymbolProviderOptions(symbolProvider);
//
//            if (PhLoadModuleSymbolProvider(
//                symbolProvider,
//                PhGetString(fileName),
//                (ULONG64)mappedImage.ViewBase,
//                (ULONG)mappedImage.Size
//                ))
//            {
//                PH_SYMBOL_INFORMATION symbolInfo;
//
//                if (PhGetSymbolFromName(
//                    symbolProvider,
//                    L"W32pServiceTable",
//                    &symbolInfo
//                    ))
//                {
//                    serviceTableAddress = PhMappedImageRvaToVa(
//                        &mappedImage,
//                        PtrToUlong(PTR_SUB_OFFSET(symbolInfo.Address, mappedImage.ViewBase)),
//                        NULL
//                        );
//                }
//
//                if (PhGetSymbolFromName(
//                    symbolProvider,
//                    L"W32pServiceLimit",
//                    &symbolInfo
//                    ))
//                {
//                    PVOID serviceLimitSymbol;
//
//                    if (serviceLimitSymbol = PhMappedImageRvaToVa(
//                        &mappedImage,
//                        PtrToUlong(PTR_SUB_OFFSET(symbolInfo.Address, mappedImage.ViewBase)),
//                        NULL
//                        ))
//                    {
//                        serviceTableLimit = *(PULONG)serviceLimitSymbol;
//                    }
//                }
//            }
//
//            if (serviceTableLimit > 0 && serviceTableLimit < PAGE_SIZE)
//            {
//                win32kSystemCallList = PhCreateList(serviceTableLimit);
//
//                for (ULONG i = 0; i < serviceTableLimit; i++)
//                {
//                    PPH_STRING name;
//                    PPH_STRING symbol = NULL;
//                    ULONG_PTR entry;
//
//                    entry = ((PULONG_PTR)serviceTableAddress)[i];
//                    entry = (ULONG_PTR)PTR_SUB_OFFSET(entry, mappedImage.NtHeaders->OptionalHeader.ImageBase);
//                    entry = (ULONG_PTR)PTR_ADD_OFFSET(entry, mappedImage.ViewBase);
//
//                    name = PhGetSymbolFromAddress(
//                        symbolProvider,
//                        (ULONG64)entry,
//                        NULL,
//                        NULL,
//                        &symbol,
//                        NULL
//                        );
//
//                    PhAddItemList(win32kSystemCallList, symbol);
//                    PhDereferenceObject(name);
//                }
//            }
//
//            PhDereferenceObject(symbolProvider);
//        }
//
//        PhUnloadMappedImage(&mappedImage);
//    }
//
//    *Win32kSystemCallList = win32kSystemCallList;
//
//    PhDereferenceObject(fileName);
//}

VOID PhpGenerateSyscallLists(
    _Out_ PPH_LIST* NtdllSystemCallList,
    _Out_ PPH_LIST* Win32kSystemCallList
    )
{
    static PH_STRINGREF ntdllPath = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntdll.dll");
    static PH_STRINGREF win32kPath = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\win32k.sys");
    static PH_STRINGREF win32uPath = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\win32u.sys");
    PPH_LIST ntdllSystemCallList = NULL;
    PPH_LIST win32kSystemCallList = NULL;
    PH_MAPPED_IMAGE mappedImage;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
    PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;

    if (NT_SUCCESS(PhLoadMappedImageEx(&ntdllPath, NULL, &mappedImage)))
    {
        if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
        {
            PPH_LIST list = PhCreateList(exports.NumberOfEntries);

            for (ULONG i = 0; i < exports.NumberOfEntries; i++)
            {
                if (NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)))
                {
                    if (!(exportEntry.Name && strncmp(exportEntry.Name, "Zw", 2) == 0))
                        continue;

                    if (NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction)))
                    {
                        PPH_SYSCALL_ENTRY entry;

                        entry = PhAllocate(sizeof(PH_SYSCALL_ENTRY));
                        entry->Address = (ULONG_PTR)exportFunction.Function;
                        entry->Name = PhZeroExtendToUtf16(exportEntry.Name);
                        entry->Name->Buffer[0] = L'N';
                        entry->Name->Buffer[1] = L't';

                        PhAddItemList(list, entry);
                    }
                }
            }

            qsort_s(list->Items, list->Count, sizeof(PVOID), PhpSystemCallListIndexSort, NULL);

            if (list->Count)
            {
                ntdllSystemCallList = PhCreateList(list->Count);

                for (ULONG i = 0; i < list->Count; i++)
                {
                    PPH_SYSCALL_ENTRY entry = list->Items[i];
                    PhAddItemList(ntdllSystemCallList, entry->Name);
                    PhFree(entry);
                }
            }

            PhDereferenceObject(list);
        }

        PhUnloadMappedImage(&mappedImage);
    }

    if (WindowsVersion >= WINDOWS_10_20H1)
    {
        // Win32k has two types of syscall filtering. The first is a process mitigation disabling every win32k syscall
        // while the second is a specific filter for specific individual system call indexes. The CreateWin32KFilterBitmap
        // function enumerates the exports from win32k.sys starting with __win32kstub_ for building the syscall table and
        // more reliable than win32u.dll since syscalls are deleted instead of being replaced with RaiseFailFastException. (dmex)

        if (NT_SUCCESS(PhLoadMappedImageEx(&win32kPath, NULL, &mappedImage)))
        {
            if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
            {
                PPH_LIST list = PhCreateList(exports.NumberOfEntries);

                for (ULONG i = 0; i < exports.NumberOfEntries; i++)
                {
                    if (NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)))
                    {
                        if (!(exportEntry.Name && strncmp(exportEntry.Name, "__win32kstub_", 13) == 0))
                            continue;

                        if (NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction)))
                        {
                            PPH_SYSCALL_ENTRY entry;

                            entry = PhAllocate(sizeof(PH_SYSCALL_ENTRY));
                            entry->Address = (ULONG_PTR)exportFunction.Function;
                            entry->Name = PhZeroExtendToUtf16(exportEntry.Name);
                            PhSkipStringRef(&entry->Name->sr, 13 * sizeof(WCHAR));

                            PhAddItemList(list, entry);
                        }
                    }
                }

                qsort_s(list->Items, list->Count, sizeof(PVOID), PhpSystemCallListIndexSort, NULL);

                if (list->Count)
                {
                    win32kSystemCallList = PhCreateList(list->Count);

                    for (ULONG i = 0; i < list->Count; i++)
                    {
                        PPH_SYSCALL_ENTRY entry = list->Items[i];
                        PhAddItemList(win32kSystemCallList, entry->Name);
                        PhFree(entry);
                    }
                }

                PhDereferenceObject(list);
            }

            PhUnloadMappedImage(&mappedImage);
        }
    }
    else
    {
        if (NT_SUCCESS(PhLoadMappedImageEx(&win32uPath, NULL, &mappedImage)))
        {
            if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
            {
                PPH_LIST list = PhCreateList(exports.NumberOfEntries);

                for (ULONG i = 0; i < exports.NumberOfEntries; i++)
                {
                    if (NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)))
                    {
                        if (!(exportEntry.Name && strncmp(exportEntry.Name, "Nt", 2) == 0))
                            continue;

                        if (NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction)))
                        {
                            PPH_SYSCALL_ENTRY entry;

                            entry = PhAllocate(sizeof(PH_SYSCALL_ENTRY));
                            entry->Address = (ULONG_PTR)exportFunction.Function;
                            entry->Name = PhZeroExtendToUtf16(exportEntry.Name);

                            // NOTE: When system calls are deleted from win32k.sys the win32u.dll interface gets
                            // replaced with stubs that call RaiseFailFastException. This breaks the sorting via
                            // RVA based on exports since these exports still exist so we'll check the prologue. (dmex)
                            if (WindowsVersion >= WINDOWS_10_21H1)
                            {
                                PBYTE exportAddress;

                                if (exportAddress = PhMappedImageRvaToVa(&mappedImage, PtrToUlong(exportFunction.Function), NULL))
                                {
                                    PPH_SYSTEMCALL_ENTRY systemCallEntry = NULL;

                                #ifdef _WIN64
                                    BYTE prologue[4] = { 0x4C, 0x8B, 0xD1, 0xB8 };

                                    if (RtlEqualMemory(exportAddress, prologue, sizeof(prologue)))
                                    {
                                        systemCallEntry = PTR_ADD_OFFSET(exportAddress, sizeof(prologue));
                                    }
                                #else
                                    BYTE prologue[1] = { 0xB8 };

                                    if (RtlEqualMemory(exportAddress, prologue, sizeof(prologue)))
                                    {
                                        systemCallEntry = PTR_ADD_OFFSET(exportAddress, sizeof(prologue));
                                    }
                                #endif

                                    if (systemCallEntry && systemCallEntry->SystemServiceIndex == WIN32K_SERVICE_INDEX)
                                    {
                                        entry->Address = systemCallEntry->SystemCallIndex;
                                        PhAddItemList(list, entry);
                                    }
                                }
                            }
                            else
                            {
                                PhAddItemList(list, entry);
                            }
                        }
                    }
                }

                qsort_s(list->Items, list->Count, sizeof(PVOID), PhpSystemCallListIndexSort, NULL);

                if (list->Count)
                {
                    win32kSystemCallList = PhCreateList(list->Count);

                    for (ULONG i = 0; i < list->Count; i++)
                    {
                        PPH_SYSCALL_ENTRY entry = list->Items[i];
                        PhAddItemList(win32kSystemCallList, entry->Name);
                        PhFree(entry);
                    }
                }

                PhDereferenceObject(list);
            }

            PhUnloadMappedImage(&mappedImage);
        }
    }

    *NtdllSystemCallList = ntdllSystemCallList;
    *Win32kSystemCallList = win32kSystemCallList;
}

PPH_STRING PhGetSystemCallNumberName(
    _In_ USHORT SystemCallNumber
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_LIST ntdllSystemCallList = NULL;
    static PPH_LIST win32kSystemCallList = NULL;
    PPH_SYSTEMCALL_ENTRY systemCallEntry = (PPH_SYSTEMCALL_ENTRY)&SystemCallNumber;

    if (PhBeginInitOnce(&initOnce))
    {
        PhpGenerateSyscallLists(&ntdllSystemCallList, &win32kSystemCallList);
        PhEndInitOnce(&initOnce);
    }

    switch (systemCallEntry->SystemServiceIndex)
    {
    case NTOS_SERVICE_INDEX:
        {
            if (ntdllSystemCallList && systemCallEntry->SystemCallIndex <= ntdllSystemCallList->Count)
            {
                PPH_STRING entry = ntdllSystemCallList->Items[systemCallEntry->SystemCallIndex];

                return PhReferenceObject(entry);
            }
        }
        break;
    case WIN32K_SERVICE_INDEX:
        {
            if (win32kSystemCallList && systemCallEntry->SystemCallIndex <= win32kSystemCallList->Count)
            {
                PPH_STRING entry = win32kSystemCallList->Items[systemCallEntry->SystemCallIndex];

                return PhReferenceObject(entry);
            }
        }
        break;
    }

    return NULL;
}

PPH_STRING PhGetApartmentStateString(
    _In_ OLETLSFLAGS ApartmentState
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (ApartmentState & OLETLS_LOCALTID)
        PhAppendStringBuilder2(&stringBuilder, L"Local TID, ");
    if (ApartmentState & OLETLS_UUIDINITIALIZED)
        PhAppendStringBuilder2(&stringBuilder, L"UUID initialized, ");
    if (ApartmentState & OLETLS_INTHREADDETACH)
        PhAppendStringBuilder2(&stringBuilder, L"Inside thread detach, ");
    if (ApartmentState & OLETLS_CHANNELTHREADINITIALZED)
        PhAppendStringBuilder2(&stringBuilder, L"Channel thread initialzed, ");
    if (ApartmentState & OLETLS_WOWTHREAD)
        PhAppendStringBuilder2(&stringBuilder, L"WOW Thread, ");
    if (ApartmentState & OLETLS_THREADUNINITIALIZING)
        PhAppendStringBuilder2(&stringBuilder, L"Thread Uninitializing, ");
    if (ApartmentState & OLETLS_DISABLE_OLE1DDE)
        PhAppendStringBuilder2(&stringBuilder, L"OLE1DDE disabled, ");
    if (ApartmentState & OLETLS_APARTMENTTHREADED)
        PhAppendStringBuilder2(&stringBuilder, L"Single threaded (STA), ");
    if (ApartmentState & OLETLS_MULTITHREADED)
        PhAppendStringBuilder2(&stringBuilder, L"Multi threaded (MTA), ");
    if (ApartmentState & OLETLS_IMPERSONATING)
        PhAppendStringBuilder2(&stringBuilder, L"Impersonating, ");
    if (ApartmentState & OLETLS_DISABLE_EVENTLOGGER)
        PhAppendStringBuilder2(&stringBuilder, L"Eventlogger disabled, ");
    if (ApartmentState & OLETLS_INNEUTRALAPT)
        PhAppendStringBuilder2(&stringBuilder, L"Neutral threaded (NTA), ");
    if (ApartmentState & OLETLS_DISPATCHTHREAD)
        PhAppendStringBuilder2(&stringBuilder, L"Dispatch thread, ");
    if (ApartmentState & OLETLS_HOSTTHREAD)
        PhAppendStringBuilder2(&stringBuilder, L"HOSTTHREAD, ");
    if (ApartmentState & OLETLS_ALLOWCOINIT)
        PhAppendStringBuilder2(&stringBuilder, L"ALLOWCOINIT, ");
    if (ApartmentState & OLETLS_PENDINGUNINIT)
        PhAppendStringBuilder2(&stringBuilder, L"PENDINGUNINIT, ");
    if (ApartmentState & OLETLS_FIRSTMTAINIT)
        PhAppendStringBuilder2(&stringBuilder, L"FIRSTMTAINIT, ");
    if (ApartmentState & OLETLS_FIRSTNTAINIT)
        PhAppendStringBuilder2(&stringBuilder, L"FIRSTNTAINIT, ");
    if (ApartmentState & OLETLS_APTINITIALIZING)
        PhAppendStringBuilder2(&stringBuilder, L"APTIN INITIALIZING, ");
    if (ApartmentState & OLETLS_UIMSGSINMODALLOOP)
        PhAppendStringBuilder2(&stringBuilder, L"UIMSGS IN MODAL LOOP, ");
    if (ApartmentState & OLETLS_MARSHALING_ERROR_OBJECT)
        PhAppendStringBuilder2(&stringBuilder, L"Marshaling error object, ");
    if (ApartmentState & OLETLS_WINRT_INITIALIZE)
        PhAppendStringBuilder2(&stringBuilder, L"WinRT initialized, ");
    if (ApartmentState & OLETLS_APPLICATION_STA)
        PhAppendStringBuilder2(&stringBuilder, L"ApplicationSTA, ");
    if (ApartmentState & OLETLS_IN_SHUTDOWN_CALLBACKS)
        PhAppendStringBuilder2(&stringBuilder, L"IN_SHUTDOWN_CALLBACKS, ");
    if (ApartmentState & OLETLS_POINTER_INPUT_BLOCKED)
        PhAppendStringBuilder2(&stringBuilder, L"POINTER_INPUT_BLOCKED, ");
    if (ApartmentState & OLETLS_IN_ACTIVATION_FILTER)
        PhAppendStringBuilder2(&stringBuilder, L"IN_ACTIVATION_FILTER, ");
    if (ApartmentState & OLETLS_ASTATOASTAEXEMPT_QUIRK)
        PhAppendStringBuilder2(&stringBuilder, L"ASTATOASTAEXEMPT_QUIRK, ");
    if (ApartmentState & OLETLS_ASTATOASTAEXEMPT_PROXY)
        PhAppendStringBuilder2(&stringBuilder, L"ASTATOASTAEXEMPT_PROXY, ");
    if (ApartmentState & OLETLS_ASTATOASTAEXEMPT_INDOUBT)
        PhAppendStringBuilder2(&stringBuilder, L"ASTATOASTAEXEMPT_INDOUBT, ");
    if (ApartmentState & OLETLS_DETECTED_USER_INITIALIZED)
        PhAppendStringBuilder2(&stringBuilder, L"DETECTED_USER_INITIALIZED, ");
    if (ApartmentState & OLETLS_BRIDGE_STA)
        PhAppendStringBuilder2(&stringBuilder, L"BRIDGE_STA, ");
    if (ApartmentState & OLETLS_NAINITIALIZING)
        PhAppendStringBuilder2(&stringBuilder, L"NA_INITIALIZING, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(ApartmentState));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}
