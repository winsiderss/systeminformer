/*
 * Process Hacker -
 *   thread list
 *
 * Copyright (C) 2011 wj32
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
#include <settings.h>
#include <extmgri.h>
#include <phplug.h>
#include <emenu.h>
#include <procprpp.h>

BOOLEAN PhpThreadNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG PhpThreadNodeHashtableHashFunction(
    __in PVOID Entry
    );

VOID PhpDestroyThreadNode(
    __in PPH_THREAD_NODE ThreadNode
    );

VOID PhpRemoveThreadNode(
    __in PPH_THREAD_NODE ThreadNode,
    __in PPH_THREAD_LIST_CONTEXT Context
    );

LONG PhpThreadTreeNewPostSortFunction(
    __in LONG Result,
    __in PVOID Node1,
    __in PVOID Node2,
    __in PH_SORT_ORDER SortOrder
    );

BOOLEAN NTAPI PhpThreadTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    );

VOID PhInitializeThreadList(
    __in HWND ParentWindowHandle,
    __in HWND TreeNewHandle,
    __in PPH_PROCESS_ITEM ProcessItem,
    __out PPH_THREAD_LIST_CONTEXT Context
    )
{
    HWND hwnd;

    memset(Context, 0, sizeof(PH_THREAD_LIST_CONTEXT));
    Context->EnableStateHighlighting = TRUE;

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPH_THREAD_NODE),
        PhpThreadNodeHashtableCompareFunction,
        PhpThreadNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    hwnd = TreeNewHandle;
    PhSetControlTheme(hwnd, L"explorer");

    TreeNew_SetCallback(hwnd, PhpThreadTreeNewCallback, Context);

    TreeNew_SetRedraw(hwnd, FALSE);

    // Default columns
    PhAddTreeNewColumn(hwnd, PHTHTLC_TID, TRUE, L"TID", 50, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumnEx(hwnd, PHTHTLC_CPU, TRUE, L"CPU", 45, PH_ALIGN_RIGHT, 1, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(hwnd, PHTHTLC_CYCLESDELTA, TRUE, L"Cycles Delta", 80, PH_ALIGN_RIGHT, 2, DT_RIGHT, TRUE);
    PhAddTreeNewColumn(hwnd, PHTHTLC_STARTADDRESS, TRUE, L"Start Address", 180, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumnEx(hwnd, PHTHTLC_PRIORITY, TRUE, L"Priority", 80, PH_ALIGN_LEFT, 4, 0, TRUE);
    PhAddTreeNewColumn(hwnd, PHTHTLC_SERVICE, FALSE, L"Service", 100, PH_ALIGN_LEFT, -1, 0);

    TreeNew_SetRedraw(hwnd, TRUE);

    TreeNew_SetSort(hwnd, PHTHTLC_CYCLESDELTA, DescendingSortOrder);

    PhCmInitializeManager(&Context->Cm, hwnd, PHTHTLC_MAXIMUM, PhpThreadTreeNewPostSortFunction);
}

VOID PhDeleteThreadList(
    __in PPH_THREAD_LIST_CONTEXT Context
    )
{
    ULONG i;

    PhCmDeleteManager(&Context->Cm);

    for (i = 0; i < Context->NodeList->Count; i++)
        PhpDestroyThreadNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

BOOLEAN PhpThreadNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PPH_THREAD_NODE threadNode1 = *(PPH_THREAD_NODE *)Entry1;
    PPH_THREAD_NODE threadNode2 = *(PPH_THREAD_NODE *)Entry2;

    return threadNode1->ThreadId == threadNode2->ThreadId;
}

ULONG PhpThreadNodeHashtableHashFunction(
    __in PVOID Entry
    )
{
    return (ULONG)(*(PPH_THREAD_NODE *)Entry)->ThreadId / 4;
}

VOID PhLoadSettingsThreadList(
    __inout PPH_THREAD_LIST_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;
    PH_TREENEW_COLUMN column;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    if (!Context->UseCycleTime)
    {
        column.Id = PHTHTLC_CYCLESDELTA;
        column.Text = L"Context Switches Delta";
        TreeNew_SetColumn(Context->TreeNewHandle, TN_COLUMN_TEXT, &column);
    }

    if (Context->HasServices)
    {
        column.Id = PHTHTLC_SERVICE;
        column.Visible = TRUE;
        TreeNew_SetColumn(Context->TreeNewHandle, TN_COLUMN_FLAG_VISIBLE, &column);
    }

    settings = PhGetStringSetting(L"ThreadTreeListColumns");
    sortSettings = PhGetStringSetting(L"ThreadTreeListSort");
    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, PH_CM_COLUMN_WIDTHS_ONLY, &settings->sr, &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);

    TreeNew_GetSort(Context->TreeNewHandle, &sortColumn, &sortOrder);

    // Make sure we're not sorting by an invisible column.
    if (sortOrder != NoSortOrder && !(TreeNew_GetColumn(Context->TreeNewHandle, sortColumn, &column) && column.Visible))
    {
        TreeNew_SetSort(Context->TreeNewHandle, PHTHTLC_CYCLESDELTA, DescendingSortOrder);
    }
}

VOID PhSaveSettingsThreadList(
    __inout PPH_THREAD_LIST_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, PH_CM_COLUMN_WIDTHS_ONLY, &sortSettings);
    PhSetStringSetting2(L"ThreadTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"ThreadTreeListSort", &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

PPH_THREAD_NODE PhAddThreadNode(
    __inout PPH_THREAD_LIST_CONTEXT Context,
    __in PPH_THREAD_ITEM ThreadItem,
    __in ULONG RunId
    )
{
    PPH_THREAD_NODE threadNode;

    threadNode = PhAllocate(PhEmGetObjectSize(EmThreadNodeType, sizeof(PH_THREAD_NODE)));
    memset(threadNode, 0, sizeof(PH_THREAD_NODE));
    PhInitializeTreeNewNode(&threadNode->Node);

    if (Context->EnableStateHighlighting && RunId != 1)
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

    threadNode->ThreadId = ThreadItem->ThreadId;
    threadNode->ThreadItem = ThreadItem;
    PhReferenceObject(ThreadItem);

    memset(threadNode->TextCache, 0, sizeof(PH_STRINGREF) * PHTHTLC_MAXIMUM);
    threadNode->Node.TextCache = threadNode->TextCache;
    threadNode->Node.TextCacheSize = PHTHTLC_MAXIMUM;

    PhAddEntryHashtable(Context->NodeHashtable, &threadNode);
    PhAddItemList(Context->NodeList, threadNode);

    PhEmCallObjectOperation(EmThreadNodeType, threadNode, EmObjectCreate);

    TreeNew_NodesStructured(Context->TreeNewHandle);

    return threadNode;
}

PPH_THREAD_NODE PhFindThreadNode(
    __in PPH_THREAD_LIST_CONTEXT Context,
    __in HANDLE ThreadId
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
    __in PPH_THREAD_LIST_CONTEXT Context,
    __in PPH_THREAD_NODE ThreadNode
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
    __in PPH_THREAD_NODE ThreadNode
    )
{
    PhEmCallObjectOperation(EmThreadNodeType, ThreadNode, EmObjectDelete);

    if (ThreadNode->CyclesDeltaText) PhDereferenceObject(ThreadNode->CyclesDeltaText);
    if (ThreadNode->StartAddressText) PhDereferenceObject(ThreadNode->StartAddressText);
    if (ThreadNode->PriorityText) PhDereferenceObject(ThreadNode->PriorityText);

    PhDereferenceObject(ThreadNode->ThreadItem);

    PhFree(ThreadNode);
}

VOID PhpRemoveThreadNode(
    __in PPH_THREAD_NODE ThreadNode,
    __in PPH_THREAD_LIST_CONTEXT Context // PH_TICK_SH_STATE requires this parameter to be after ThreadNode
    )
{
    ULONG index;

    // Remove from list and cleanup.

    if ((index = PhFindItemList(Context->NodeList, ThreadNode)) != -1)
        PhRemoveItemList(Context->NodeList, index);

    PhpDestroyThreadNode(ThreadNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhUpdateThreadNode(
    __in PPH_THREAD_LIST_CONTEXT Context,
    __in PPH_THREAD_NODE ThreadNode
    )
{
    memset(ThreadNode->TextCache, 0, sizeof(PH_STRINGREF) * PHTHTLC_MAXIMUM);

    ThreadNode->ValidMask = 0;
    PhInvalidateTreeNewNode(&ThreadNode->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhTickThreadNodes(
    __in PPH_THREAD_LIST_CONTEXT Context
    )
{
    PH_TICK_SH_STATE_TN(PH_THREAD_NODE, ShState, Context->NodeStateList, PhpRemoveThreadNode, PhCsHighlightingDuration, Context->TreeNewHandle, TRUE, NULL, Context);
}

#define SORT_FUNCTION(Column) PhpThreadTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpThreadTreeNewCompare##Column( \
    __in void *_context, \
    __in const void *_elem1, \
    __in const void *_elem2 \
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
    __in LONG Result,
    __in PVOID Node1,
    __in PVOID Node2,
    __in PH_SORT_ORDER SortOrder
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

BEGIN_SORT_FUNCTION(Priority)
{
    sortResult = intcmp(threadItem1->PriorityWin32, threadItem2->PriorityWin32);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Service)
{
    sortResult = PhCompareStringWithNull(threadItem1->ServiceName, threadItem2->ServiceName, TRUE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpThreadTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    PPH_THREAD_LIST_CONTEXT context;
    PPH_THREAD_NODE node;

    context = Context;

    if (PhCmForwardMessage(hwnd, Message, Parameter1, Parameter2, &context->Cm))
        return TRUE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Tid),
                    SORT_FUNCTION(Cpu),
                    SORT_FUNCTION(CyclesDelta),
                    SORT_FUNCTION(StartAddress),
                    SORT_FUNCTION(Priority),
                    SORT_FUNCTION(Service)
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
                    if (context->TreeNewSortColumn < PHTHTLC_MAXIMUM)
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

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PPH_THREAD_ITEM threadItem;

            node = (PPH_THREAD_NODE)getCellText->Node;
            threadItem = node->ThreadItem;

            switch (getCellText->Id)
            {
            case PHTHTLC_TID:
                PhInitializeStringRef(&getCellText->Text, threadItem->ThreadIdString);
                break;
            case PHTHTLC_CPU:
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
                            getCellText->Text.Length = (USHORT)(returnLength - sizeof(WCHAR)); // minus null terminator
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
                            getCellText->Text.Length = (USHORT)(returnLength - sizeof(WCHAR));
                        }
                    }
                }
                break;
            case PHTHTLC_CYCLESDELTA:
                if (context->UseCycleTime)
                {
                    if (threadItem->CyclesDelta.Delta != threadItem->CyclesDelta.Value && threadItem->CyclesDelta.Delta != 0)
                    {
                        PhSwapReference2(&node->CyclesDeltaText, PhFormatUInt64(threadItem->CyclesDelta.Delta, TRUE));
                        getCellText->Text = node->CyclesDeltaText->sr;
                    }
                }
                else
                {
                    if (threadItem->ContextSwitchesDelta.Delta != threadItem->ContextSwitchesDelta.Value && threadItem->ContextSwitchesDelta.Delta != 0)
                    {
                        PhSwapReference2(&node->CyclesDeltaText, PhFormatUInt64(threadItem->ContextSwitchesDelta.Delta, TRUE));
                        getCellText->Text = node->CyclesDeltaText->sr;
                    }
                }
                break;
            case PHTHTLC_STARTADDRESS:
                PhSwapReference(&node->StartAddressText, threadItem->StartAddressString);
                getCellText->Text = PhGetStringRef(node->StartAddressText);
                break;
            case PHTHTLC_PRIORITY:
                PhSwapReference2(&node->PriorityText, PhGetThreadPriorityWin32String(threadItem->PriorityWin32));
                getCellText->Text = PhGetStringRef(node->PriorityText);
                break;
            case PHTHTLC_SERVICE:
                getCellText->Text = PhGetStringRef(threadItem->ServiceName);
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

            node = (PPH_THREAD_NODE)getNodeColor->Node;
            threadItem = node->ThreadItem;

            if (!threadItem)
                ; // Dummy
            else if (PhCsUseColorSuspended && threadItem->WaitReason == Suspended)
                getNodeColor->BackColor = PhCsColorSuspended;
            else if (PhCsUseColorGuiThreads && threadItem->IsGuiThread)
                getNodeColor->BackColor = PhCsColorGuiThreads;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);
            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewSelectionChanged:
        {
            SendMessage(context->ParentWindowHandle, WM_PH_THREAD_SELECTION_CHANGED, 0, 0);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_THREAD_COPY, 0);
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
            data.DefaultSortColumn = PHTHTLC_CYCLESDELTA;
            data.DefaultSortOrder = DescendingSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_NO_VISIBILITY);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT | PH_EMENU_SHOW_NONOTIFY,
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
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_SHOWCONTEXTMENU, MAKELONG(mouseEvent->Location.x, mouseEvent->Location.y));
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
    __in PPH_THREAD_LIST_CONTEXT Context
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
    __in PPH_THREAD_LIST_CONTEXT Context,
    __out PPH_THREAD_ITEM **Threads,
    __out PULONG NumberOfThreads
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_THREAD_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node->ThreadItem);
        }
    }

    *Threads = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfThreads = list->Count;

    PhDereferenceObject(list);
}

VOID PhDeselectAllThreadNodes(
    __in PPH_THREAD_LIST_CONTEXT Context
    )
{
    TreeNew_DeselectRange(Context->TreeNewHandle, 0, -1);
}
