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

#define MAINWND_PRIVATE
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

static HANDLE TreeListHandle;
static PPH_HASHTABLE ProcessNodeHashtable;
static PPH_LIST ProcessNodeList;
static PPH_LIST ProcessNodeRootList;

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
    TreeListHandle = hwnd;

    TreeList_SetCallback(hwnd, PhpProcessTreeListCallback);
    TreeList_SetPlusMinus(
        hwnd,
        PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDB_PLUS), IMAGE_BITMAP),
        PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDB_MINUS), IMAGE_BITMAP)
        );

    PhAddTreeListColumn(hwnd, 0, TRUE, L"Name", 100, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeListColumn(hwnd, 1, TRUE, L"PID", 50, PH_ALIGN_RIGHT, 1, DT_RIGHT);
    PhAddTreeListColumn(hwnd, 2, TRUE, L"User Name", 140, PH_ALIGN_LEFT, 2, 0);
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

    PhAddListItem(ProcessNodeList, processNode);

    TreeList_NodesStructured(TreeListHandle);
}

PPH_PROCESS_NODE PhFindProcessNode(
   __in HANDLE ProcessId
   )
{
    ULONG i;

    for (i = 0; i < ProcessNodeList->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNodeList->Items[i];

        if (node->ProcessId == ProcessId)
            return node;
    }

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

    if ((index = PhIndexOfListItem(ProcessNodeList, ProcessNode)) != -1)
        PhRemoveListItem(ProcessNodeList, index);

    PhDereferenceObject(ProcessNode->Children);

    TreeList_NodesStructured(TreeListHandle);
}

VOID PhUpdateProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    )
{
    memset(ProcessNode->TextCache, 0, sizeof(PH_STRINGREF) * PHTLC_MAXIMUM);
    PhInvalidateTreeListNode(&ProcessNode->Node, TLIN_COLOR | TLIN_ICON);
}

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
        return TRUE;
    case TreeListIsLeaf:
        {
            PPH_TREELIST_IS_LEAF isLeaf = Parameter1;

            node = (PPH_PROCESS_NODE)isLeaf->Node;

            isLeaf->IsLeaf = node->Children->Count == 0; 
        }
        return TRUE;
    case TreeListGetNodeText:
        {
            PPH_TREELIST_GET_NODE_TEXT getNodeText = Parameter1;

            node = (PPH_PROCESS_NODE)getNodeText->Node;

            switch (getNodeText->Id)
            {
            case PHTLC_NAME:
                getNodeText->Text = node->ProcessItem->ProcessName->sr;
                break;
            case PHTLC_PID:
                PhInitializeStringRef(&getNodeText->Text, node->ProcessItem->ProcessIdString);
                break;
            case PHTLC_USERNAME:
                getNodeText->Text = PhGetStringRefOrEmpty(node->ProcessItem->UserName);
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

            if (PhCsUseColorServiceProcesses && processItem->ServiceList->Count != 0)
                getNodeColor->BackColor = PhCsColorServiceProcesses;

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
    }

    return FALSE;
}
