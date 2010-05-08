/*
 * Process Hacker - 
 *   process tree list
 * 
 * Copyright (C) 2010 wj32
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

BOOLEAN NTAPI PhpProcessNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpProcessNodeHashtableHashFunction(
    __in PVOID Entry
    );

BOOLEAN NTAPI PhpProcessTreeListCallback(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Context
    );

static HANDLE ProcessTreeListHandle;
static ULONG ProcessTreeListSortColumn;
static PH_SORT_ORDER ProcessTreeListSortOrder;

static PPH_HASHTABLE ProcessNodeHashtable; // hashtable of all nodes
static PPH_LIST ProcessNodeList; // list of all nodes, used when sorting is enabled
static PPH_LIST ProcessNodeRootList; // list of root nodes

static HICON StockAppIcon;

VOID PhProcessTreeListInitialization()
{
    ProcessNodeHashtable = PhCreateHashtable(
        sizeof(PPH_PROCESS_NODE),
        PhpProcessNodeHashtableCompareFunction,
        PhpProcessNodeHashtableHashFunction,
        40
        );

    ProcessNodeList = PhCreateList(40);
    ProcessNodeRootList = PhCreateList(10);
}

VOID PhInitializeProcessTreeList(
    __in HWND hwnd
    )
{
    ProcessTreeListHandle = hwnd;

    TreeList_SetCallback(hwnd, PhpProcessTreeListCallback);
    TreeList_SetPlusMinus(
        hwnd,
        PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDB_PLUS), IMAGE_BITMAP),
        PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDB_MINUS), IMAGE_BITMAP)
        );

    PhAddTreeListColumn(hwnd, PHTLC_NAME, TRUE, L"Name", 200, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeListColumn(hwnd, PHTLC_PID, TRUE, L"PID", 50, PH_ALIGN_RIGHT, 1, DT_RIGHT);
    PhAddTreeListColumn(hwnd, PHTLC_CPU, TRUE, L"CPU", 45, PH_ALIGN_RIGHT, 2, DT_RIGHT);
    PhAddTreeListColumn(hwnd, PHTLC_PVTMEMORY, TRUE, L"Pvt. Memory", 70, PH_ALIGN_RIGHT, 3, DT_RIGHT);
    PhAddTreeListColumn(hwnd, PHTLC_USERNAME, TRUE, L"User Name", 140, PH_ALIGN_LEFT, 4, 0);

    TreeList_SetTriState(hwnd, TRUE);
    TreeList_SetSort(hwnd, 0, NoSortOrder);
}

static BOOLEAN NTAPI PhpProcessNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    return (*(PPH_PROCESS_NODE *)Entry1)->ProcessId == (*(PPH_PROCESS_NODE *)Entry2)->ProcessId;
}

static ULONG NTAPI PhpProcessNodeHashtableHashFunction(
    __in PVOID Entry
    )
{
    return (ULONG)(*(PPH_PROCESS_NODE *)Entry)->ProcessId / 4;
}

VOID PhCreateProcessNode(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_NODE processNode;
    PPH_PROCESS_NODE parentNode;
    ULONG i;

    processNode = PhAllocate(sizeof(PH_PROCESS_NODE));
    memset(processNode, 0, sizeof(PH_PROCESS_NODE));
    PhInitializeTreeListNode(&processNode->Node);

    processNode->ProcessId = ProcessItem->ProcessId;
    processNode->ProcessItem = ProcessItem;

    memset(processNode->TextCache, 0, sizeof(PH_STRINGREF) * PHTLC_MAXIMUM);
    processNode->Node.TextCache = processNode->TextCache;
    processNode->Node.TextCacheSize = PHTLC_MAXIMUM;

    processNode->Children = PhCreateList(1);

    // Find this process' parent and add the process to it if we found it.
    if (ProcessItem->HasParent && (parentNode = PhFindProcessNode(ProcessItem->ParentProcessId)))
    {
        PhAddListItem(parentNode->Children, processNode);
        processNode->Parent = parentNode;
    }
    else
    {
        // No parent, add to root list.
        processNode->Parent = NULL;
        PhAddListItem(ProcessNodeRootList, processNode);
    }

    // Find this process' children and move them to this node.

    for (i = 0; i < ProcessNodeRootList->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNodeRootList->Items[i];

        if (node->ProcessItem->HasParent && node->ProcessItem->ParentProcessId == ProcessItem->ProcessId)
        {
            node->Parent = processNode;
            PhAddListItem(processNode->Children, node);
        }
    }

    for (i = 0; i < processNode->Children->Count; i++)
    {
        PhRemoveListItem(
            ProcessNodeRootList,
            PhIndexOfListItem(ProcessNodeRootList, processNode->Children->Items[i])
            );
    }

    PhAddHashtableEntry(ProcessNodeHashtable, &processNode);
    PhAddListItem(ProcessNodeList, processNode);

    TreeList_NodesStructured(ProcessTreeListHandle);
}

PPH_PROCESS_NODE PhFindProcessNode(
   __in HANDLE ProcessId
   )
{
    PH_PROCESS_NODE lookupNode;
    PPH_PROCESS_NODE lookupNodePtr = &lookupNode;
    PPH_PROCESS_NODE *node;

    lookupNode.ProcessId = ProcessId;

    node = PhGetHashtableEntry(ProcessNodeHashtable, &lookupNodePtr);

    if (node)
        return *node;
    else
        return NULL;
}

VOID PhRemoveProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    )
{
    ULONG index;
    ULONG i;

    if (ProcessNode->Parent)
    {
        // Remove the node from its parent.

        if ((index = PhIndexOfListItem(ProcessNode->Parent->Children, ProcessNode)) != -1)
            PhRemoveListItem(ProcessNode->Parent->Children, index);
    }
    else
    {
        // Remove the node from the root list.

        if ((index = PhIndexOfListItem(ProcessNodeRootList, ProcessNode)) != -1)
            PhRemoveListItem(ProcessNodeRootList, index);
    }

    // Move the node's children to the root list.
    for (i = 0; i < ProcessNode->Children->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNode->Children->Items[i];

        node->Parent = NULL;
        PhAddListItem(ProcessNodeRootList, node);
    }

    // Remove from hashtable/list and cleanup.

    PhRemoveHashtableEntry(ProcessNodeHashtable, &ProcessNode);

    if ((index = PhIndexOfListItem(ProcessNodeList, ProcessNode)) != -1)
        PhRemoveListItem(ProcessNodeList, index);

    PhDereferenceObject(ProcessNode->Children);

    if (ProcessNode->PrivateMemoryText) PhDereferenceObject(ProcessNode->PrivateMemoryText);
    if (ProcessNode->TooltipText) PhDereferenceObject(ProcessNode->TooltipText);

    TreeList_NodesStructured(ProcessTreeListHandle);
}

VOID PhUpdateProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    )
{
    memset(ProcessNode->TextCache, 0, sizeof(PH_STRINGREF) * PHTLC_MAXIMUM);

    if (ProcessNode->TooltipText)
    {
        PhDereferenceObject(ProcessNode->TooltipText);
        ProcessNode->TooltipText = NULL;
    }

    PhInvalidateTreeListNode(&ProcessNode->Node, TLIN_COLOR | TLIN_ICON);

    TreeList_UpdateNode(ProcessTreeListHandle, &ProcessNode->Node);

    if (ProcessTreeListSortOrder != NoSortOrder)
    {
        // Force a rebuild to sort the items.
        TreeList_NodesStructured(ProcessTreeListHandle);
    }
}

#define SORT_FUNCTION(Column) PhpProcessTreeListCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpProcessTreeListCompare##Column( \
    __in const void *_elem1, \
    __in const void *_elem2 \
    ) \
{ \
    PPH_PROCESS_NODE node1 = *(PPH_PROCESS_NODE *)_elem1; \
    PPH_PROCESS_NODE node2 = *(PPH_PROCESS_NODE *)_elem2; \
    PPH_PROCESS_ITEM processItem1 = node1->ProcessItem; \
    PPH_PROCESS_ITEM processItem2 = node2->ProcessItem; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)processItem1->ProcessId, (ULONG_PTR)processItem2->ProcessId); \
    \
    return PhModifySort(sortResult, ProcessTreeListSortOrder); \
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhStringCompare(processItem1->ProcessName, processItem2->ProcessName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Pid)
{
    // Use signed int so DPCs and Interrupts are placed above System Idle Process.
    sortResult = intcmp((LONG)processItem1->ProcessId, (LONG)processItem2->ProcessId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Cpu)
{
    sortResult = singlecmp(processItem1->CpuUsage, processItem2->CpuUsage);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PvtMemory)
{
    sortResult = uintptrcmp(processItem1->VmCounters.PrivateUsage, processItem2->VmCounters.PrivateUsage);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(UserName)
{
    sortResult = PhStringCompareWithNull(processItem1->UserName, processItem2->UserName, TRUE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpProcessTreeListCallback(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Context
    )
{
    PPH_PROCESS_NODE node;

    switch (Message)
    {
    case TreeListGetChildren:
        {
            PPH_TREELIST_GET_CHILDREN getChildren = Parameter1;

            node = (PPH_PROCESS_NODE)getChildren->Node;

            if (ProcessTreeListSortOrder == NoSortOrder)
            {
                if (!node)
                {
                    getChildren->Children = (PPH_TREELIST_NODE *)ProcessNodeRootList->Items;
                    getChildren->NumberOfChildren = ProcessNodeRootList->Count;
                }
                else
                {
                    getChildren->Children = (PPH_TREELIST_NODE *)node->Children->Items;
                    getChildren->NumberOfChildren = node->Children->Count;
                }
            }
            else
            {
                if (!node)
                {
                    static PVOID sortFunctions[] =
                    {
                        SORT_FUNCTION(Name),
                        SORT_FUNCTION(Pid),
                        SORT_FUNCTION(Cpu),
                        SORT_FUNCTION(PvtMemory),
                        SORT_FUNCTION(UserName)
                    };
                    int (__cdecl *sortFunction)(const void *, const void *);

                    if (ProcessTreeListSortColumn < PHTLC_MAXIMUM)
                        sortFunction = sortFunctions[ProcessTreeListSortColumn];

                    if (sortFunction)
                    {
                        // Don't use PhSortList to avoid overhead.
                        qsort(ProcessNodeList->Items, ProcessNodeList->Count, sizeof(PVOID), sortFunction);
                    }

                    getChildren->Children = (PPH_TREELIST_NODE *)ProcessNodeList->Items;
                    getChildren->NumberOfChildren = ProcessNodeList->Count;
                }
            }
        }
        return TRUE;
    case TreeListIsLeaf:
        {
            PPH_TREELIST_IS_LEAF isLeaf = Parameter1;

            node = (PPH_PROCESS_NODE)isLeaf->Node;

            if (ProcessTreeListSortOrder == NoSortOrder)
                isLeaf->IsLeaf = node->Children->Count == 0;
            else
                isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeListGetNodeText:
        {
            PPH_TREELIST_GET_NODE_TEXT getNodeText = Parameter1;
            PPH_PROCESS_ITEM processItem;

            node = (PPH_PROCESS_NODE)getNodeText->Node;
            processItem = node->ProcessItem;

            switch (getNodeText->Id)
            {
            case PHTLC_NAME:
                getNodeText->Text = processItem->ProcessName->sr;
                break;
            case PHTLC_PID:
                PhInitializeStringRef(&getNodeText->Text, processItem->ProcessIdString);
                break;
            case PHTLC_CPU:
                PhInitializeStringRef(&getNodeText->Text, processItem->CpuUsageString);
                break;
            case PHTLC_PVTMEMORY:
                if (node->PrivateMemoryText) PhDereferenceObject(node->PrivateMemoryText);
                node->PrivateMemoryText = PhFormatSize(processItem->VmCounters.PrivateUsage, -1);
                getNodeText->Text = node->PrivateMemoryText->sr;
                break;
            case PHTLC_USERNAME:
                getNodeText->Text = PhGetStringRefOrEmpty(processItem->UserName);
                break;
            default:
                return FALSE;
            }

            getNodeText->Flags = TLC_CACHE;
        }
        return TRUE;
    case TreeListGetNodeColor:
        {
            PPH_TREELIST_GET_NODE_COLOR getNodeColor = Parameter1;
            PPH_PROCESS_ITEM processItem;

            node = (PPH_PROCESS_NODE)getNodeColor->Node;
            processItem = node->ProcessItem;

            if (!processItem)
                ; // Dummy
            else if (PhCsUseColorDebuggedProcesses && processItem->IsBeingDebugged)
                getNodeColor->BackColor = PhCsColorDebuggedProcesses;
            else if (PhCsUseColorElevatedProcesses && processItem->IsElevated)
                getNodeColor->BackColor = PhCsColorElevatedProcesses;
            else if (PhCsUseColorPosixProcesses && processItem->IsPosix)
                getNodeColor->BackColor = PhCsColorPosixProcesses;
            else if (PhCsUseColorWow64Processes && processItem->IsWow64)
                getNodeColor->BackColor = PhCsColorWow64Processes;
            else if (PhCsUseColorJobProcesses && processItem->IsInSignificantJob)
                getNodeColor->BackColor = PhCsColorJobProcesses;
            else if (
                PhCsUseColorPacked &&
                (processItem->VerifyResult != VrUnknown &&
                processItem->VerifyResult != VrNoSignature &&
                processItem->VerifyResult != VrTrusted
                ))
                getNodeColor->BackColor = PhCsColorPacked;
            else if (PhCsUseColorDotNet && processItem->IsDotNet)
                getNodeColor->BackColor = PhCsColorDotNet;
            else if (PhCsUseColorPacked && processItem->IsPacked)
                getNodeColor->BackColor = PhCsColorPacked;
            else if (PhCsUseColorServiceProcesses && processItem->ServiceList->Count != 0)
                getNodeColor->BackColor = PhCsColorServiceProcesses;
            else if (
                PhCsUseColorSystemProcesses &&
                processItem->UserName &&
                PhStringEquals(processItem->UserName, PhLocalSystemName, TRUE) // TODO: localize
                )
                getNodeColor->BackColor = PhCsColorSystemProcesses;
            else if (
                PhCsUseColorOwnProcesses &&
                processItem->UserName &&
                PhCurrentUserName &&
                PhStringEquals(processItem->UserName, PhCurrentUserName, TRUE)
                )
                getNodeColor->BackColor = PhCsColorOwnProcesses;

            getNodeColor->Flags = TLC_CACHE | TLGNC_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeListGetNodeIcon:
        {
            PPH_TREELIST_GET_NODE_ICON getNodeIcon = Parameter1;

            node = (PPH_PROCESS_NODE)getNodeIcon->Node;

            if (node->ProcessItem->SmallIcon)
            {
                getNodeIcon->Icon = node->ProcessItem->SmallIcon;
            }
            else
            {
                if (!StockAppIcon)
                {
                    SHFILEINFO fileInfo = { 0 };

                    SHGetFileInfo(
                        L".exe",
                        FILE_ATTRIBUTE_NORMAL,
                        &fileInfo,
                        sizeof(SHFILEINFO),
                        SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON
                        );
                    StockAppIcon = fileInfo.hIcon;
                }

                getNodeIcon->Icon = StockAppIcon;
            }

            getNodeIcon->Flags = TLC_CACHE;
        }
        return TRUE;
    case TreeListGetNodeTooltip:
        {
            PPH_TREELIST_GET_NODE_TOOLTIP getNodeTooltip = Parameter1;

            node = (PPH_PROCESS_NODE)getNodeTooltip->Node;

            if (!node->TooltipText)
                node->TooltipText = PhGetProcessTooltipText(node->ProcessItem);

            if (node->TooltipText)
                getNodeTooltip->Text = node->TooltipText->sr;
            else
                return FALSE;
        }
        return TRUE;
    case TreeListSortChanged:
        {
            TreeList_GetSort(hwnd, &ProcessTreeListSortColumn, &ProcessTreeListSortOrder);
            // Force a rebuild to sort the items.
            TreeList_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeListKeyDown:
        {
            switch ((SHORT)Parameter1)
            {
            case VK_DELETE:
                if (GetKeyState(VK_SHIFT) >= 0)
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_PROCESS_TERMINATE, 0);
                else
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_PROCESS_TERMINATETREE, 0);

                break;
            case VK_RETURN:
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_PROCESS_PROPERTIES, 0);
                break;
            }
        }
        return TRUE;
    case TreeListNodeRightClick:
        {
            PPH_TREELIST_MOUSE_EVENT mouseEvent = Parameter2;

            PhShowProcessContextMenu(mouseEvent->Location);
        }
        return TRUE;
    case TreeListNodeLeftDoubleClick:
        {
            SendMessage(PhMainWndHandle, WM_COMMAND, ID_PROCESS_PROPERTIES, 0);
        }
        return TRUE;
    }

    return FALSE;
}

PPH_PROCESS_ITEM PhGetSelectedProcessItem()
{
    PPH_PROCESS_ITEM processItem = NULL;
    ULONG i;

    for (i = 0; i < ProcessNodeList->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNodeList->Items[i];

        if (node->Node.Selected)
        {
            processItem = node->ProcessItem;
            break;
        }
    }

    return processItem;
}

VOID PhGetSelectedProcessItems(
    __out PPH_PROCESS_ITEM **Processes,
    __out PULONG NumberOfProcesses
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < ProcessNodeList->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddListItem(list, node->ProcessItem);
        }
    }

    *Processes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfProcesses = list->Count;

    PhDereferenceObject(list);
}

VOID PhDeselectAllProcessNodes()
{
    ULONG i;

    for (i = 0; i < ProcessNodeList->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNodeList->Items[i];

        node->Node.Selected = FALSE;
        PhInvalidateTreeListNode(&node->Node, TLIN_STATE);
    }

    InvalidateRect(ProcessTreeListHandle, NULL, TRUE);
}

VOID PhInvalidateAllProcessNodes()
{
    ULONG i;

    for (i = 0; i < ProcessNodeList->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNodeList->Items[i];

        memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PHTLC_MAXIMUM);
        PhInvalidateTreeListNode(&node->Node, TLIN_COLOR);
    }

    InvalidateRect(ProcessTreeListHandle, NULL, TRUE);
}

VOID PhInvalidateAllTextProcessNodes()
{
    ULONG i;

    for (i = 0; i < ProcessNodeList->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNodeList->Items[i];

        // The name and PID never change, so we don't invalidate that.
        memset(&node->TextCache[2], 0, sizeof(PH_STRINGREF) * (PHTLC_MAXIMUM - 2));
    }

    InvalidateRect(ProcessTreeListHandle, NULL, FALSE);
}

VOID PhSelectAndEnsureVisibleProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    )
{
    PPH_PROCESS_NODE processNode;
    BOOLEAN needsRestructure = FALSE;

    PhDeselectAllProcessNodes();

    // Expand recursively, upwards.

    processNode = ProcessNode->Parent;

    while (processNode)
    {
        if (!processNode->Node.Expanded)
            needsRestructure = TRUE;

        processNode->Node.Expanded = TRUE;
        processNode = processNode->Parent;
    }

    ProcessNode->Node.Selected = TRUE;
    PhInvalidateTreeListNode(&ProcessNode->Node, TLIN_STATE);

    if (needsRestructure)
        TreeList_NodesStructured(ProcessTreeListHandle);

    TreeList_EnsureVisible(ProcessTreeListHandle, &ProcessNode->Node, FALSE);
}
