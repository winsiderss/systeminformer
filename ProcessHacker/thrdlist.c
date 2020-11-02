/*
 * Process Hacker -
 *   thread list
 *
 * Copyright (C) 2011-2012 wj32
 * Copyright (C) 2018-2019 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
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
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_TIDHEX, FALSE, L"TID (Hex)", 50, PH_ALIGN_RIGHT, ULONG_MAX, DT_RIGHT);
    PhAddTreeNewColumn(TreeNewHandle, PH_THREAD_TREELIST_COLUMN_CPUCORECYCLES, FALSE, L"CPU (relative)", 50, PH_ALIGN_RIGHT, ULONG_MAX, DT_RIGHT);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);
    TreeNew_SetTriState(TreeNewHandle, TRUE);
    //TreeNew_SetSort(TreeNewHandle, PHTHTLC_CYCLESDELTA, DescendingSortOrder);

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
    TreeNew_NodesStructured(Context->TreeNewHandle);
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
    sortResult = PhCompareStringZ(node1->IdealProcessorText, node2->IdealProcessorText, TRUE);
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
    sortResult = singlecmp(threadItem1->CpuUsage, threadItem2->CpuUsage); // TODO * core

    if (sortResult == 0)
    {
        if (context->UseCycleTime)
            sortResult = uint64cmp(threadItem1->CyclesDelta.Delta, threadItem2->CyclesDelta.Delta);
        else
            sortResult = uintcmp(threadItem1->ContextSwitchesDelta.Delta, threadItem2->ContextSwitchesDelta.Delta);
    }
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

                        PhInitFormatF(&format, cpuUsage, 2);

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
                        PhInitFormatF(&format[1], 0.01, 2);

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
                    SIZE_T returnLength;
                    PH_FORMAT format[1];

                    PhInitFormatI64UGroupDigits(&format[0], threadItem->CyclesDelta.Value);

                    if (PhFormatToBuffer(format, 1, node->CyclesText, sizeof(node->CyclesText), &returnLength))
                    {
                        getCellText->Text.Buffer = node->CyclesText;
                        getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
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
                    BOOLEAN breakOnTermination;

                    if (threadItem->ThreadHandle)
                    {
                        if (NT_SUCCESS(PhGetThreadBreakOnTermination(threadItem->ThreadHandle, &breakOnTermination)))
                        {
                            if (breakOnTermination)
                                PhInitializeStringRef(&getCellText->Text, L"Critical");
                        }
                    }
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
                    cpuUsage = cpuUsage * (ULONG)PhSystemBasicInformation.NumberOfProcessors; // * 2; // linux style (dmex)

                    if (cpuUsage >= 0.01)
                    {
                        PH_FORMAT format;
                        SIZE_T returnLength;

                        PhInitFormatF(&format, cpuUsage, 2);

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
                        PhInitFormatF(&format[1], 0.01, 2);

                        if (PhFormatToBuffer(format, 2, node->CpuCoreUsageText, sizeof(node->CpuCoreUsageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->CpuCoreUsageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
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
