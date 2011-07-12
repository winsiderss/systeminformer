/*
 * Process Hacker - 
 *   process tree list
 * 
 * Copyright (C) 2010-2011 wj32
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
#include <phplug.h>
#include <cpysave.h>

VOID PhpEnableColumnCustomDraw(
    __in HWND hwnd,
    __in ULONG Id
    );

VOID PhpRemoveProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    );

VOID PhpUpdateNeedCyclesInformation();

VOID PhpUpdateProcessNodeCycles(
    __inout PPH_PROCESS_NODE ProcessNode
    );

LONG PhpProcessTreeNewPostSortFunction(
    __in LONG Result,
    __in PVOID Node1,
    __in PVOID Node2,
    __in PH_SORT_ORDER SortOrder
    );

BOOLEAN NTAPI PhpProcessTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    );

BOOLEAN PhpApplyProcessTreeFiltersToNode(
    __in PPH_PROCESS_NODE Node
    );

static HWND ProcessTreeListHandle;
static ULONG ProcessTreeListSortColumn;
static PH_SORT_ORDER ProcessTreeListSortOrder;
static PH_CM_MANAGER ProcessTreeListCm;

static PPH_HASH_ENTRY ProcessNodeHashSet[256] = PH_HASH_SET_INIT; // hashtable of all nodes
static PPH_LIST ProcessNodeList; // list of all nodes, used when sorting is enabled
static PPH_LIST ProcessNodeRootList; // list of root nodes

BOOLEAN PhProcessTreeListStateHighlighting = TRUE;
static PPH_POINTER_LIST ProcessNodeStateList = NULL; // list of nodes which need to be processed

static PPH_LIST ProcessTreeFilterList = NULL;
static BOOLEAN NeedCyclesInformation = FALSE;

VOID PhProcessTreeListInitialization()
{
    ProcessNodeList = PhCreateList(40);
    ProcessNodeRootList = PhCreateList(10);
}

VOID PhInitializeProcessTreeList(
    __in HWND hwnd
    )
{
    PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

    ProcessTreeListHandle = hwnd;
    PhSetControlTheme(ProcessTreeListHandle, L"explorer");
    TreeNew_SetExtendedFlags(hwnd, TN_FLAG_ITEM_DRAG_SELECT, TN_FLAG_ITEM_DRAG_SELECT);
    SendMessage(TreeNew_GetTooltips(ProcessTreeListHandle), TTM_SETMAXTIPWIDTH, 0, MAXSHORT);
    SendMessage(TreeNew_GetTooltips(ProcessTreeListHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, 0x7fff);

    TreeNew_SetCallback(hwnd, PhpProcessTreeNewCallback, NULL);

    TreeNew_SetMaxId(hwnd, PHPRTLC_MAXIMUM - 1);

    TreeNew_SetRedraw(hwnd, FALSE);

    // Default columns
    PhAddTreeNewColumn(hwnd, PHPRTLC_NAME, TRUE, L"Name", 200, PH_ALIGN_LEFT, -2, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_PID, TRUE, L"PID", 50, PH_ALIGN_RIGHT, 0, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_CPU, TRUE, L"CPU", 45, PH_ALIGN_RIGHT, 1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOTOTALRATE, TRUE, L"I/O Total Rate", 70, PH_ALIGN_RIGHT, 2, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_PRIVATEBYTES, TRUE, L"Private Bytes", 70, PH_ALIGN_RIGHT, 3, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_USERNAME, TRUE, L"User Name", 140, PH_ALIGN_LEFT, 4, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumn(hwnd, PHPRTLC_DESCRIPTION, TRUE, L"Description", 180, PH_ALIGN_LEFT, 5, 0);

    // Customizable columns (1)
    PhAddTreeNewColumn(hwnd, PHPRTLC_COMPANYNAME, FALSE, L"Company Name", 180, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_VERSION, FALSE, L"Version", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_FILENAME, FALSE, L"File Name", 180, PH_ALIGN_LEFT, -1, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumn(hwnd, PHPRTLC_COMMANDLINE, FALSE, L"Command Line", 180, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_PEAKPRIVATEBYTES, FALSE, L"Peak Private Bytes", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_WORKINGSET, FALSE, L"Working Set", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_PEAKWORKINGSET, FALSE, L"Peak Working Set", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_PRIVATEWS, FALSE, L"Private WS", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_SHAREDWS, FALSE, L"Shared WS", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_SHAREABLEWS, FALSE, L"Shareable WS", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_VIRTUALSIZE, FALSE, L"Virtual Size", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_PEAKVIRTUALSIZE, FALSE, L"Peak Virtual Size", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_PAGEFAULTS, FALSE, L"Page Faults", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_SESSIONID, FALSE, L"Session ID", 45, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_PRIORITYCLASS, FALSE, L"Priority Class", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_BASEPRIORITY, FALSE, L"Base Priority", 45, PH_ALIGN_RIGHT, -1, DT_RIGHT);

    // Customizable columns (2)
    PhAddTreeNewColumn(hwnd, PHPRTLC_THREADS, FALSE, L"Threads", 45, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_HANDLES, FALSE, L"Handles", 45, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_GDIHANDLES, FALSE, L"GDI Handles", 45, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_USERHANDLES, FALSE, L"USER Handles", 45, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IORORATE, FALSE, L"I/O Read+Other Rate", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOWRATE, FALSE, L"I/O Write Rate", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_INTEGRITY, FALSE, L"Integrity", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOPRIORITY, FALSE, L"I/O Priority", 70, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_PAGEPRIORITY, FALSE, L"Page Priority", 45, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_STARTTIME, FALSE, L"Start Time", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_TOTALCPUTIME, FALSE, L"Total CPU Time", 90, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_KERNELCPUTIME, FALSE, L"Kernel CPU Time", 90, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_USERCPUTIME, FALSE, L"User CPU Time", 90, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_VERIFICATIONSTATUS, FALSE, L"Verification Status", 70, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_VERIFIEDSIGNER, FALSE, L"Verified Signer", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_RELATIVESTARTTIME, FALSE, L"Relative Start Time", 180, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_BITS, FALSE, L"Bits", 50, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_ELEVATION, FALSE, L"Elevation", 60, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_WINDOWTITLE, FALSE, L"Window Title", 120, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_WINDOWSTATUS, FALSE, L"Window Status", 60, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_CYCLES, FALSE, L"Cycles", 110, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_CYCLESDELTA, FALSE, L"Cycles Delta", 90, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_CPUHISTORY, FALSE, L"CPU History", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_PRIVATEBYTESHISTORY, FALSE, L"Private Bytes History", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOHISTORY, FALSE, L"I/O History", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_DEPSTATUS, FALSE, L"DEP Status", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_VIRTUALIZED, FALSE, L"Virtualized", 80, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHPRTLC_CONTEXTSWITCHES, FALSE, L"Context Switches", 100, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_CONTEXTSWITCHESDELTA, FALSE, L"Context Switches Delta", 80, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_PAGEFAULTSDELTA, FALSE, L"Page Faults Delta", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);

    // I/O group columns
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOREADS, FALSE, L"I/O Reads", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOWRITES, FALSE, L"I/O Writes", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOOTHER, FALSE, L"I/O Other", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOREADBYTES, FALSE, L"I/O Read Bytes", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOWRITEBYTES, FALSE, L"I/O Write Bytes", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOOTHERBYTES, FALSE, L"I/O Other Bytes", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOREADSDELTA, FALSE, L"I/O Reads Delta", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOWRITESDELTA, FALSE, L"I/O Writes Delta", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHPRTLC_IOOTHERDELTA, FALSE, L"I/O Other Delta", 70, PH_ALIGN_RIGHT, -1, DT_RIGHT);

    TreeNew_SetRedraw(hwnd, TRUE);

    PhpEnableColumnCustomDraw(hwnd, PHPRTLC_CPUHISTORY);
    PhpEnableColumnCustomDraw(hwnd, PHPRTLC_PRIVATEBYTESHISTORY);
    PhpEnableColumnCustomDraw(hwnd, PHPRTLC_IOHISTORY);

    TreeNew_SetTriState(hwnd, TRUE);
    TreeNew_SetSort(hwnd, 0, NoSortOrder);

    PhCmInitializeManager(&ProcessTreeListCm, hwnd, PHPRTLC_MAXIMUM, PhpProcessTreeNewPostSortFunction);

    if (PhPluginsEnabled)
    {
        treeNewInfo.TreeNewHandle = hwnd;
        treeNewInfo.CmData = &ProcessTreeListCm;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessTreeNewInitializing), &treeNewInfo);
    }
}

static VOID PhpEnableColumnCustomDraw(
    __in HWND hwnd,
    __in ULONG Id
    )
{
    PH_TREENEW_COLUMN column;

    column.Id = Id;
    column.CustomDraw = TRUE;
    TreeNew_SetColumn(hwnd, TN_COLUMN_FLAG_CUSTOMDRAW, &column);
}

VOID PhLoadSettingsProcessTreeList()
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"ProcessTreeListColumns");
    sortSettings = PhGetStringSetting(L"ProcessTreeListSort");
    PhCmLoadSettingsEx(ProcessTreeListHandle, &ProcessTreeListCm, &settings->sr, &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);

    if (PhGetIntegerSetting(L"EnableInstantTooltips"))
    {
        SendMessage(TreeNew_GetTooltips(ProcessTreeListHandle), TTM_SETDELAYTIME, TTDT_INITIAL, 0);
    }

    PhpUpdateNeedCyclesInformation();
}

VOID PhSaveSettingsProcessTreeList()
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(ProcessTreeListHandle, &ProcessTreeListCm, &sortSettings);
    PhSetStringSetting2(L"ProcessTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"ProcessTreeListSort", &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhReloadSettingsProcessTreeList()
{
    SendMessage(TreeNew_GetTooltips(ProcessTreeListHandle), TTM_SETDELAYTIME, TTDT_INITIAL,
        PhGetIntegerSetting(L"EnableInstantTooltips") ? 0 : -1);
}

FORCEINLINE BOOLEAN PhCompareProcessNode(
    __in PPH_PROCESS_NODE Value1,
    __in PPH_PROCESS_NODE Value2
    )
{
    return Value1->ProcessId == Value2->ProcessId;
}

FORCEINLINE ULONG PhHashProcessNode(
    __in PPH_PROCESS_NODE Value
    )
{
    return (ULONG)Value->ProcessId / 4;
}

PPH_PROCESS_NODE PhAddProcessNode(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in ULONG RunId
    )
{
    PPH_PROCESS_NODE processNode;
    PPH_PROCESS_NODE parentNode;
    ULONG i;

    processNode = PhAllocate(sizeof(PH_PROCESS_NODE));
    memset(processNode, 0, sizeof(PH_PROCESS_NODE));
    PhInitializeTreeNewNode(&processNode->Node);

    if (PhProcessTreeListStateHighlighting && RunId != 1)
    {
        PhChangeShStateTn(
            &processNode->Node,
            &processNode->ShState,
            &ProcessNodeStateList,
            NewItemState,
            PhCsColorNew,
            NULL
            );
    }

    processNode->ProcessId = ProcessItem->ProcessId;
    processNode->ProcessItem = ProcessItem;
    PhReferenceObject(ProcessItem);

    memset(processNode->TextCache, 0, sizeof(PH_STRINGREF) * PHPRTLC_MAXIMUM);
    processNode->Node.TextCache = processNode->TextCache;
    processNode->Node.TextCacheSize = PHPRTLC_MAXIMUM;

    processNode->Children = PhCreateList(1);

    // Find this process' parent and add the process to it if we found it.
    if (
        (parentNode = PhFindProcessNode(ProcessItem->ParentProcessId)) &&
        parentNode->ProcessItem->CreateTime.QuadPart <= ProcessItem->CreateTime.QuadPart
        )
    {
        PhAddItemList(parentNode->Children, processNode);
        processNode->Parent = parentNode;
    }
    else
    {
        // No parent, add to root list.
        processNode->Parent = NULL;
        PhAddItemList(ProcessNodeRootList, processNode);
    }

    // Find this process' children and move them to this node.

    for (i = 0; i < ProcessNodeRootList->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNodeRootList->Items[i];

        if (
            node != processNode && // for cases where the parent PID = PID (e.g. System Idle Process) 
            node->ProcessItem->ParentProcessId == ProcessItem->ProcessId &&
            ProcessItem->CreateTime.QuadPart <= node->ProcessItem->CreateTime.QuadPart
            )
        {
            node->Parent = processNode;
            PhAddItemList(processNode->Children, node);
        }
    }

    for (i = 0; i < processNode->Children->Count; i++)
    {
        PhRemoveItemList(
            ProcessNodeRootList,
            PhFindItemList(ProcessNodeRootList, processNode->Children->Items[i])
            );
    }

    PhAddEntryHashSet(
        ProcessNodeHashSet,
        PH_HASH_SET_SIZE(ProcessNodeHashSet),
        &processNode->HashEntry,
        PhHashProcessNode(processNode)
        );
    PhAddItemList(ProcessNodeList, processNode);

    if (PhCsCollapseServicesOnStart)
    {
        static PH_STRINGREF servicesBaseName = PH_STRINGREF_INIT(L"\\services.exe");
        static BOOLEAN servicesFound = FALSE;
        static PPH_STRING servicesFileName = NULL;

        if (!servicesFound)
        {
            if (!servicesFileName)
            {
                PPH_STRING systemDirectory;

                systemDirectory = PhGetSystemDirectory();
                servicesFileName = PhConcatStringRef2(&systemDirectory->sr, &servicesBaseName);
                PhDereferenceObject(systemDirectory);
            }

            // If this process is services.exe, collapse the node and free the string.
            if (
                ProcessItem->FileName &&
                PhEqualString(ProcessItem->FileName, servicesFileName, TRUE)
                )
            {
                processNode->Node.Expanded = FALSE;
                PhDereferenceObject(servicesFileName);
                servicesFileName = NULL;
                servicesFound = TRUE;
            }
        }
    }

    if (ProcessTreeFilterList)
        processNode->Node.Visible = PhpApplyProcessTreeFiltersToNode(processNode);

    TreeNew_NodesStructured(ProcessTreeListHandle);

    return processNode;
}

PPH_PROCESS_NODE PhFindProcessNode(
    __in HANDLE ProcessId
    )
{
    PH_PROCESS_NODE lookupNode;
    PPH_HASH_ENTRY entry;
    PPH_PROCESS_NODE node;

    lookupNode.ProcessId = ProcessId;
    entry = PhFindEntryHashSet(
        ProcessNodeHashSet,
        PH_HASH_SET_SIZE(ProcessNodeHashSet),
        PhHashProcessNode(&lookupNode)
        );

    for (; entry; entry = entry->Next)
    {
        node = CONTAINING_RECORD(entry, PH_PROCESS_NODE, HashEntry);

        if (PhCompareProcessNode(&lookupNode, node))
            return node;
    }

    return NULL;
}

VOID PhRemoveProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    )
{
    if (PhProcessTreeListStateHighlighting)
    {
        PhChangeShStateTn(
            &ProcessNode->Node,
            &ProcessNode->ShState,
            &ProcessNodeStateList,
            RemovingItemState,
            PhCsColorRemoved,
            ProcessTreeListHandle
            );
    }
    else
    {
        PhpRemoveProcessNode(ProcessNode);
    }
}

VOID PhpRemoveProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    )
{
    ULONG index;
    ULONG i;

    if (ProcessNode->Parent)
    {
        // Remove the node from its parent.

        if ((index = PhFindItemList(ProcessNode->Parent->Children, ProcessNode)) != -1)
            PhRemoveItemList(ProcessNode->Parent->Children, index);
    }
    else
    {
        // Remove the node from the root list.

        if ((index = PhFindItemList(ProcessNodeRootList, ProcessNode)) != -1)
            PhRemoveItemList(ProcessNodeRootList, index);
    }

    // Move the node's children to the root list.
    for (i = 0; i < ProcessNode->Children->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNode->Children->Items[i];

        node->Parent = NULL;
        PhAddItemList(ProcessNodeRootList, node);
    }

    // Remove from hashtable/list and cleanup.

    PhRemoveEntryHashSet(ProcessNodeHashSet, PH_HASH_SET_SIZE(ProcessNodeHashSet), &ProcessNode->HashEntry);

    if ((index = PhFindItemList(ProcessNodeList, ProcessNode)) != -1)
        PhRemoveItemList(ProcessNodeList, index);

    PhDereferenceObject(ProcessNode->Children);

    if (ProcessNode->WindowText) PhDereferenceObject(ProcessNode->WindowText);

    if (ProcessNode->TooltipText) PhDereferenceObject(ProcessNode->TooltipText);

    if (ProcessNode->IoTotalRateText) PhDereferenceObject(ProcessNode->IoTotalRateText);
    if (ProcessNode->PrivateBytesText) PhDereferenceObject(ProcessNode->PrivateBytesText);
    if (ProcessNode->PeakPrivateBytesText) PhDereferenceObject(ProcessNode->PeakPrivateBytesText);
    if (ProcessNode->WorkingSetText) PhDereferenceObject(ProcessNode->WorkingSetText);
    if (ProcessNode->PeakWorkingSetText) PhDereferenceObject(ProcessNode->PeakWorkingSetText);
    if (ProcessNode->PrivateWsText) PhDereferenceObject(ProcessNode->PrivateWsText);
    if (ProcessNode->SharedWsText) PhDereferenceObject(ProcessNode->SharedWsText);
    if (ProcessNode->ShareableWsText) PhDereferenceObject(ProcessNode->ShareableWsText);
    if (ProcessNode->VirtualSizeText) PhDereferenceObject(ProcessNode->VirtualSizeText);
    if (ProcessNode->PeakVirtualSizeText) PhDereferenceObject(ProcessNode->PeakVirtualSizeText);
    if (ProcessNode->PageFaultsText) PhDereferenceObject(ProcessNode->PageFaultsText);
    if (ProcessNode->IoRoRateText) PhDereferenceObject(ProcessNode->IoRoRateText);
    if (ProcessNode->IoWRateText) PhDereferenceObject(ProcessNode->IoWRateText);
    if (ProcessNode->StartTimeText) PhDereferenceObject(ProcessNode->StartTimeText);
    if (ProcessNode->RelativeStartTimeText) PhDereferenceObject(ProcessNode->RelativeStartTimeText);
    if (ProcessNode->WindowTitleText) PhDereferenceObject(ProcessNode->WindowTitleText);
    if (ProcessNode->CyclesText) PhDereferenceObject(ProcessNode->CyclesText);
    if (ProcessNode->CyclesDeltaText) PhDereferenceObject(ProcessNode->CyclesDeltaText);
    if (ProcessNode->ContextSwitchesText) PhDereferenceObject(ProcessNode->ContextSwitchesText);
    if (ProcessNode->ContextSwitchesDeltaText) PhDereferenceObject(ProcessNode->ContextSwitchesDeltaText);
    if (ProcessNode->PageFaultsDeltaText) PhDereferenceObject(ProcessNode->PageFaultsDeltaText);

    for (i = 0; i < PHPRTLC_IOGROUP_COUNT; i++)
        if (ProcessNode->IoGroupText[i]) PhDereferenceObject(ProcessNode->IoGroupText[i]);

    PhDeleteGraphBuffers(&ProcessNode->CpuGraphBuffers);
    PhDeleteGraphBuffers(&ProcessNode->PrivateGraphBuffers);
    PhDeleteGraphBuffers(&ProcessNode->IoGraphBuffers);

    PhDereferenceObject(ProcessNode->ProcessItem);

    PhFree(ProcessNode);

    TreeNew_NodesStructured(ProcessTreeListHandle);
}

VOID PhUpdateProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    )
{
    memset(ProcessNode->TextCache, 0, sizeof(PH_STRINGREF) * PHPRTLC_MAXIMUM);

    if (ProcessNode->TooltipText)
    {
        PhDereferenceObject(ProcessNode->TooltipText);
        ProcessNode->TooltipText = NULL;
    }

    PhInvalidateTreeNewNode(&ProcessNode->Node, TN_CACHE_COLOR | TN_CACHE_ICON);
    TreeNew_InvalidateNode(ProcessTreeListHandle, &ProcessNode->Node);
}

VOID PhTickProcessNodes()
{
    ULONG i;
    PH_TREENEW_VIEW_PARTS viewParts;
    BOOLEAN fullyInvalidated;
    RECT rect;

    // Text invalidation, node updates

    for (i = 0; i < ProcessNodeList->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNodeList->Items[i];

        // The name and PID never change, so we don't invalidate that.
        memset(&node->TextCache[2], 0, sizeof(PH_STRINGREF) * (PHPRTLC_MAXIMUM - 2));
        node->ValidMask = 0;

        // Invalidate graph buffers.
        node->CpuGraphBuffers.Valid = FALSE;
        node->PrivateGraphBuffers.Valid = FALSE;
        node->IoGraphBuffers.Valid = FALSE;

        // Updates cycles if necessary.
        if (NeedCyclesInformation)
            PhpUpdateProcessNodeCycles(node);
    }

    fullyInvalidated = FALSE;

    if (ProcessTreeListSortOrder != NoSortOrder)
    {
        // Force a rebuild to sort the items.
        TreeNew_NodesStructured(ProcessTreeListHandle);
        fullyInvalidated = TRUE;
    }

    // State highlighting
    PH_TICK_SH_STATE_TN(PH_PROCESS_NODE, ShState, ProcessNodeStateList, PhpRemoveProcessNode, PhCsHighlightingDuration, ProcessTreeListHandle, TRUE, &fullyInvalidated);

    if (!fullyInvalidated)
    {
        // The first column doesn't need to be invalidated because the process name never changes, and 
        // icon changes are handled by the modified event. This small optimization can save more than 
        // 10 million cycles per update (on my machine).
        TreeNew_GetViewParts(ProcessTreeListHandle, &viewParts);
        rect.left = viewParts.NormalLeft;
        rect.top = viewParts.HeaderHeight;
        rect.right = viewParts.ClientRect.right - viewParts.VScrollWidth;
        rect.bottom = viewParts.ClientRect.bottom;
        InvalidateRect(ProcessTreeListHandle, &rect, FALSE);
    }
}

static FLOAT PhpCalculateInclusiveCpuUsage(
    __in PPH_PROCESS_NODE ProcessNode
    )
{
    FLOAT cpuUsage;
    ULONG i;

    cpuUsage = ProcessNode->ProcessItem->CpuUsage;

    for (i = 0; i < ProcessNode->Children->Count; i++)
    {
        cpuUsage += PhpCalculateInclusiveCpuUsage(ProcessNode->Children->Items[i]);
    }

    return cpuUsage;
}

static VOID PhpUpdateProcessNodeWsCounters(
    __inout PPH_PROCESS_NODE ProcessNode
    )
{
    if (!(ProcessNode->ValidMask & PHPN_WSCOUNTERS))
    {
        BOOLEAN success = FALSE;
        HANDLE processHandle;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_INFORMATION,
            ProcessNode->ProcessItem->ProcessId
            )))
        {
            if (NT_SUCCESS(PhGetProcessWsCounters(
                processHandle,
                &ProcessNode->WsCounters
                )))
                success = TRUE;

            NtClose(processHandle);
        }

        if (!success)
            memset(&ProcessNode->WsCounters, 0, sizeof(PH_PROCESS_WS_COUNTERS));

        ProcessNode->ValidMask |= PHPN_WSCOUNTERS;
    }
}

static VOID PhpUpdateProcessNodeGdiUserHandles(
    __inout PPH_PROCESS_NODE ProcessNode
    )
{
    if (!(ProcessNode->ValidMask & PHPN_GDIUSERHANDLES))
    {
        if (ProcessNode->ProcessItem->QueryHandle)
        {
            ProcessNode->GdiHandles = GetGuiResources(ProcessNode->ProcessItem->QueryHandle, GR_GDIOBJECTS);
            ProcessNode->UserHandles = GetGuiResources(ProcessNode->ProcessItem->QueryHandle, GR_USEROBJECTS);
        }
        else
        {
            ProcessNode->GdiHandles = 0;
            ProcessNode->UserHandles = 0;
        }

        ProcessNode->ValidMask |= PHPN_GDIUSERHANDLES;
    }
}

static VOID PhpUpdateProcessNodeIoPagePriority(
    __inout PPH_PROCESS_NODE ProcessNode
    )
{
    if (!(ProcessNode->ValidMask & PHPN_IOPAGEPRIORITY))
    {
        if (ProcessNode->ProcessItem->QueryHandle)
        {
            if (!NT_SUCCESS(PhGetProcessIoPriority(ProcessNode->ProcessItem->QueryHandle, &ProcessNode->IoPriority)))
                ProcessNode->IoPriority = -1;
            if (!NT_SUCCESS(PhGetProcessPagePriority(ProcessNode->ProcessItem->QueryHandle, &ProcessNode->PagePriority)))
                ProcessNode->PagePriority = -1;
        }
        else
        {
            ProcessNode->IoPriority = -1;
            ProcessNode->PagePriority = -1;
        }

        ProcessNode->ValidMask |= PHPN_IOPAGEPRIORITY;
    }
}

static BOOL CALLBACK PhpEnumProcessNodeWindowsProc(
    __in HWND hwnd,
    __in LPARAM lParam
    )
{
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)lParam;
    ULONG threadId;
    ULONG processId;

    threadId = GetWindowThreadProcessId(hwnd, &processId);

    if (UlongToHandle(processId) == processNode->ProcessId)
    {
        HWND parentWindow;

        if (
            !IsWindowVisible(hwnd) || // skip invisible windows
            ((parentWindow = GetParent(hwnd)) && IsWindowVisible(parentWindow)) || // skip windows with visible parents
            GetWindowTextLength(hwnd) == 0 // skip windows with no title
            )
            return TRUE;

        processNode->WindowHandle = hwnd;
        return FALSE;
    }

    return TRUE;
}

static VOID PhpUpdateProcessNodeWindow(
    __inout PPH_PROCESS_NODE ProcessNode
    )
{
    if (!(ProcessNode->ValidMask & PHPN_WINDOW))
    {
        ProcessNode->WindowHandle = NULL;
        EnumWindows(PhpEnumProcessNodeWindowsProc, (LPARAM)ProcessNode);

        PhSwapReference(&ProcessNode->WindowText, NULL);

        if (ProcessNode->WindowHandle)
        {
            ProcessNode->WindowText = PhGetWindowText(ProcessNode->WindowHandle);
            ProcessNode->WindowHung = !!IsHungAppWindow(ProcessNode->WindowHandle);
        }

        ProcessNode->ValidMask |= PHPN_WINDOW;
    }
}

static VOID PhpUpdateProcessNodeDepStatus(
    __inout PPH_PROCESS_NODE ProcessNode
    )
{
    if (!(ProcessNode->ValidMask & PHPN_DEPSTATUS))
    {
        HANDLE processHandle;
        ULONG depStatus;

        depStatus = 0;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_INFORMATION,
            ProcessNode->ProcessItem->ProcessId
            )))
        {
            PhGetProcessDepStatus(processHandle, &depStatus);
            NtClose(processHandle);
        }

        ProcessNode->DepStatus = depStatus;

        ProcessNode->ValidMask |= PHPN_DEPSTATUS;
    }
}

static VOID PhpUpdateProcessNodeToken(
    __inout PPH_PROCESS_NODE ProcessNode
    )
{
    if (!(ProcessNode->ValidMask & PHPN_TOKEN))
    {
        HANDLE tokenHandle;

        ProcessNode->VirtualizationAllowed = FALSE;
        ProcessNode->VirtualizationEnabled = FALSE;

        if (WINDOWS_HAS_UAC && ProcessNode->ProcessItem->QueryHandle)
        {
            if (NT_SUCCESS(PhOpenProcessToken(
                &tokenHandle,
                TOKEN_QUERY,
                ProcessNode->ProcessItem->QueryHandle
                )))
            {
                if (NT_SUCCESS(PhGetTokenIsVirtualizationAllowed(tokenHandle, &ProcessNode->VirtualizationAllowed)) &&
                    ProcessNode->VirtualizationAllowed)
                {
                    if (!NT_SUCCESS(PhGetTokenIsVirtualizationEnabled(tokenHandle, &ProcessNode->VirtualizationEnabled)))
                    {
                        ProcessNode->VirtualizationAllowed = FALSE; // display N/A on error
                    }
                }

                NtClose(tokenHandle);
            }
        }

        ProcessNode->ValidMask |= PHPN_TOKEN;
    }
}

static VOID PhpUpdateNeedCyclesInformation()
{
    PH_TREENEW_COLUMN column;

    NeedCyclesInformation = FALSE;

    if (!WINDOWS_HAS_CYCLE_TIME)
        return;

    TreeNew_GetColumn(ProcessTreeListHandle, PHPRTLC_CYCLES, &column);

    if (column.Visible)
    {
        NeedCyclesInformation = TRUE;
        return;
    }

    TreeNew_GetColumn(ProcessTreeListHandle, PHPRTLC_CYCLESDELTA, &column);

    if (column.Visible)
    {
        NeedCyclesInformation = TRUE;
        return;
    }
}

static VOID PhpUpdateProcessNodeCycles(
    __inout PPH_PROCESS_NODE ProcessNode
    )
{
    if (ProcessNode->ProcessId == SYSTEM_IDLE_PROCESS_ID)
    {
        PULARGE_INTEGER idleThreadCycleTimes;
        ULONG64 cycleTime;
        ULONG i;

        // System Idle Process requires special treatment.

        idleThreadCycleTimes = PhAllocate(
            sizeof(ULARGE_INTEGER) * (ULONG)PhSystemBasicInformation.NumberOfProcessors
            );

        if (NT_SUCCESS(NtQuerySystemInformation(
            SystemProcessorIdleCycleTimeInformation,
            idleThreadCycleTimes,
            sizeof(ULARGE_INTEGER) * (ULONG)PhSystemBasicInformation.NumberOfProcessors,
            NULL
            )))
        {
            cycleTime = 0;

            for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
                cycleTime += idleThreadCycleTimes[i].QuadPart;

            PhUpdateDelta(&ProcessNode->CyclesDelta, cycleTime);
        }

        PhFree(idleThreadCycleTimes);
    }
    else if (ProcessNode->ProcessItem->QueryHandle)
    {
        ULONG64 cycleTime;

        if (NT_SUCCESS(PhGetProcessCycleTime(ProcessNode->ProcessItem->QueryHandle, &cycleTime)))
        {
            PhUpdateDelta(&ProcessNode->CyclesDelta, cycleTime);
        }
    }

    if (ProcessNode->CyclesDelta.Value != 0)
        PhSwapReference2(&ProcessNode->CyclesText, PhFormatUInt64(ProcessNode->CyclesDelta.Value, TRUE));
    else
        PhSwapReference2(&ProcessNode->CyclesText, NULL);

    if (ProcessNode->CyclesDelta.Delta != 0)
        PhSwapReference2(&ProcessNode->CyclesDeltaText, PhFormatUInt64(ProcessNode->CyclesDelta.Delta, TRUE));
    else
        PhSwapReference2(&ProcessNode->CyclesDeltaText, NULL);
}

#define SORT_FUNCTION(Column) PhpProcessTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpProcessTreeNewCompare##Column( \
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
        sortResult = intptrcmp((LONG_PTR)processItem1->ProcessId, (LONG_PTR)processItem2->ProcessId); \
    \
    return PhModifySort(sortResult, ProcessTreeListSortOrder); \
}

LONG PhpProcessTreeNewPostSortFunction(
    __in LONG Result,
    __in PVOID Node1,
    __in PVOID Node2,
    __in PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = intptrcmp((LONG_PTR)((PPH_PROCESS_NODE)Node1)->ProcessItem->ProcessId, (LONG_PTR)((PPH_PROCESS_NODE)Node2)->ProcessItem->ProcessId);

    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareString(processItem1->ProcessName, processItem2->ProcessName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Pid)
{
    // Use signed int so DPCs and Interrupts are placed above System Idle Process.
    sortResult = intptrcmp((LONG_PTR)processItem1->ProcessId, (LONG_PTR)processItem2->ProcessId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Cpu)
{
    sortResult = singlecmp(processItem1->CpuUsage, processItem2->CpuUsage);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoTotalRate)
{
    sortResult = uint64cmp(
        processItem1->IoReadDelta.Delta + processItem1->IoWriteDelta.Delta + processItem1->IoOtherDelta.Delta,
        processItem2->IoReadDelta.Delta + processItem2->IoWriteDelta.Delta + processItem2->IoOtherDelta.Delta
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PrivateBytes)
{
    sortResult = uintptrcmp(processItem1->VmCounters.PagefileUsage, processItem2->VmCounters.PagefileUsage);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(UserName)
{
    sortResult = PhCompareStringWithNull(processItem1->UserName, processItem2->UserName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Description)
{
    sortResult = PhCompareStringWithNull(
        processItem1->VersionInfo.FileDescription,
        processItem2->VersionInfo.FileDescription,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(CompanyName)
{
    sortResult = PhCompareStringWithNull(
        processItem1->VersionInfo.CompanyName,
        processItem2->VersionInfo.CompanyName,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Version)
{
    sortResult = PhCompareStringWithNull(
        processItem1->VersionInfo.FileVersion,
        processItem2->VersionInfo.FileVersion,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(FileName)
{
    sortResult = PhCompareStringWithNull(
        processItem1->FileName,
        processItem2->FileName,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(CommandLine)
{
    sortResult = PhCompareStringWithNull(
        processItem1->CommandLine,
        processItem2->CommandLine,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PeakPrivateBytes)
{
    sortResult = uintptrcmp(processItem1->VmCounters.PeakPagefileUsage, processItem2->VmCounters.PeakPagefileUsage);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(WorkingSet)
{
    sortResult = uintptrcmp(processItem1->VmCounters.WorkingSetSize, processItem2->VmCounters.WorkingSetSize);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PeakWorkingSet)
{
    sortResult = uintptrcmp(processItem1->VmCounters.PeakWorkingSetSize, processItem2->VmCounters.PeakWorkingSetSize);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PrivateWs)
{
    PhpUpdateProcessNodeWsCounters(node1);
    PhpUpdateProcessNodeWsCounters(node2);
    sortResult = uintcmp(node1->WsCounters.NumberOfPrivatePages, node2->WsCounters.NumberOfPrivatePages);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(SharedWs)
{
    PhpUpdateProcessNodeWsCounters(node1);
    PhpUpdateProcessNodeWsCounters(node2);
    sortResult = uintcmp(node1->WsCounters.NumberOfSharedPages, node2->WsCounters.NumberOfSharedPages);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ShareableWs)
{
    PhpUpdateProcessNodeWsCounters(node1);
    PhpUpdateProcessNodeWsCounters(node2);
    sortResult = uintcmp(node1->WsCounters.NumberOfShareablePages, node2->WsCounters.NumberOfShareablePages);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(VirtualSize)
{
    sortResult = uintptrcmp(processItem1->VmCounters.VirtualSize, processItem2->VmCounters.VirtualSize);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PeakVirtualSize)
{
    sortResult = uintptrcmp(processItem1->VmCounters.PeakVirtualSize, processItem2->VmCounters.PeakVirtualSize);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PageFaults)
{
    sortResult = uintcmp(processItem1->VmCounters.PageFaultCount, processItem2->VmCounters.PageFaultCount);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(SessionId)
{
    sortResult = uintcmp(processItem1->SessionId, processItem2->SessionId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(BasePriority)
{
    sortResult = intcmp(processItem1->BasePriority, processItem2->BasePriority);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Threads)
{
    sortResult = uintcmp(processItem1->NumberOfThreads, processItem2->NumberOfThreads);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Handles)
{
    sortResult = uintcmp(processItem1->NumberOfHandles, processItem2->NumberOfHandles);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(GdiHandles)
{
    sortResult = uintcmp(node1->GdiHandles, node2->GdiHandles);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(UserHandles)
{
    sortResult = uintcmp(node1->UserHandles, node2->UserHandles);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoRoRate)
{
    sortResult = uint64cmp(
        processItem1->IoReadDelta.Delta + processItem1->IoOtherDelta.Delta,
        processItem2->IoReadDelta.Delta + processItem2->IoOtherDelta.Delta
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoWRate)
{
    sortResult = uint64cmp(
        processItem1->IoWriteDelta.Delta,
        processItem2->IoWriteDelta.Delta
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Integrity)
{
    sortResult = uintcmp(processItem1->IntegrityLevel, processItem2->IntegrityLevel);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoPriority)
{
    sortResult = uintcmp(node1->IoPriority, node2->IoPriority);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PagePriority)
{
    sortResult = uintcmp(node1->PagePriority, node2->PagePriority);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StartTime)
{
    sortResult = int64cmp(processItem1->CreateTime.QuadPart, processItem2->CreateTime.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(TotalCpuTime)
{
    sortResult = uint64cmp(
        processItem1->KernelTime.QuadPart + processItem1->UserTime.QuadPart,
        processItem2->KernelTime.QuadPart + processItem2->UserTime.QuadPart
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(KernelCpuTime)
{
    sortResult = uint64cmp(
        processItem1->KernelTime.QuadPart,
        processItem2->KernelTime.QuadPart
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(UserCpuTime)
{
    sortResult = uint64cmp(
        processItem1->UserTime.QuadPart,
        processItem2->UserTime.QuadPart
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(VerificationStatus)
{
    sortResult = intcmp(processItem1->VerifyResult, processItem2->VerifyResult);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(VerifiedSigner)
{
    sortResult = PhCompareStringWithNull(
        processItem1->VerifySignerName,
        processItem2->VerifySignerName,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Reserved1)
{
    sortResult = 0;
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RelativeStartTime)
{
    sortResult = -int64cmp(processItem1->CreateTime.QuadPart, processItem2->CreateTime.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Bits)
{
    sortResult = intcmp(processItem1->IsWow64, processItem2->IsWow64);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Elevation)
{
    sortResult = intcmp(processItem1->ElevationType, processItem2->ElevationType);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(WindowTitle)
{
    PhpUpdateProcessNodeWindow(node1);
    PhpUpdateProcessNodeWindow(node2);
    sortResult = PhCompareStringWithNull(node1->WindowText, node2->WindowText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(WindowStatus)
{
    PhpUpdateProcessNodeWindow(node1);
    PhpUpdateProcessNodeWindow(node2);
    sortResult = intcmp(node1->WindowHung, node2->WindowHung);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Cycles)
{
    sortResult = uint64cmp(node1->CyclesDelta.Value, node2->CyclesDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(CyclesDelta)
{
    sortResult = uint64cmp(node1->CyclesDelta.Delta, node2->CyclesDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(DepStatus)
{
    PhpUpdateProcessNodeDepStatus(node1);
    PhpUpdateProcessNodeDepStatus(node2);
    sortResult = uintcmp(node1->DepStatus, node2->DepStatus);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Virtualized)
{
    PhpUpdateProcessNodeToken(node1);
    PhpUpdateProcessNodeToken(node2);
    sortResult = intcmp(node1->VirtualizationEnabled, node2->VirtualizationEnabled);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ContextSwitches)
{
    sortResult = uintcmp(processItem1->ContextSwitchesDelta.Value, processItem2->ContextSwitchesDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ContextSwitchesDelta)
{
    sortResult = intcmp((LONG)processItem1->ContextSwitchesDelta.Delta, (LONG)processItem2->ContextSwitchesDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PageFaultsDelta)
{
    sortResult = uintcmp(processItem1->PageFaultsDelta.Delta, processItem2->PageFaultsDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoReads)
{
    sortResult = uint64cmp(processItem1->IoReadCountDelta.Value, processItem2->IoReadCountDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoWrites)
{
    sortResult = uint64cmp(processItem1->IoWriteCountDelta.Value, processItem2->IoWriteCountDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoOther)
{
    sortResult = uint64cmp(processItem1->IoOtherCountDelta.Value, processItem2->IoOtherCountDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoReadBytes)
{
    sortResult = uint64cmp(processItem1->IoReadDelta.Value, processItem2->IoReadDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoWriteBytes)
{
    sortResult = uint64cmp(processItem1->IoWriteDelta.Value, processItem2->IoWriteDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoOtherBytes)
{
    sortResult = uint64cmp(processItem1->IoOtherDelta.Value, processItem2->IoOtherDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoReadsDelta)
{
    sortResult = uint64cmp(processItem1->IoReadCountDelta.Delta, processItem2->IoReadCountDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoWritesDelta)
{
    sortResult = uint64cmp(processItem1->IoWriteCountDelta.Delta, processItem2->IoWriteCountDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoOtherDelta)
{
    sortResult = uint64cmp(processItem1->IoOtherCountDelta.Delta, processItem2->IoOtherCountDelta.Delta);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpProcessTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    PPH_PROCESS_NODE node;

    if (PhCmForwardMessage(hwnd, Message, Parameter1, Parameter2, &ProcessTreeListCm))
        return TRUE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            node = (PPH_PROCESS_NODE)getChildren->Node;

            if (ProcessTreeListSortOrder == NoSortOrder)
            {
                if (!node)
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)ProcessNodeRootList->Items;
                    getChildren->NumberOfChildren = ProcessNodeRootList->Count;
                }
                else
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)node->Children->Items;
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
                        SORT_FUNCTION(IoTotalRate),
                        SORT_FUNCTION(PrivateBytes),
                        SORT_FUNCTION(UserName),
                        SORT_FUNCTION(Description),
                        SORT_FUNCTION(CompanyName),
                        SORT_FUNCTION(Version),
                        SORT_FUNCTION(FileName),
                        SORT_FUNCTION(CommandLine),
                        SORT_FUNCTION(PeakPrivateBytes),
                        SORT_FUNCTION(WorkingSet),
                        SORT_FUNCTION(PeakWorkingSet),
                        SORT_FUNCTION(PrivateWs),
                        SORT_FUNCTION(SharedWs),
                        SORT_FUNCTION(ShareableWs),
                        SORT_FUNCTION(VirtualSize),
                        SORT_FUNCTION(PeakVirtualSize),
                        SORT_FUNCTION(PageFaults),
                        SORT_FUNCTION(SessionId),
                        SORT_FUNCTION(BasePriority), // Priority Class
                        SORT_FUNCTION(BasePriority),
                        SORT_FUNCTION(Threads),
                        SORT_FUNCTION(Handles),
                        SORT_FUNCTION(GdiHandles),
                        SORT_FUNCTION(UserHandles),
                        SORT_FUNCTION(IoRoRate),
                        SORT_FUNCTION(IoWRate),
                        SORT_FUNCTION(Integrity),
                        SORT_FUNCTION(IoPriority),
                        SORT_FUNCTION(PagePriority),
                        SORT_FUNCTION(StartTime),
                        SORT_FUNCTION(TotalCpuTime),
                        SORT_FUNCTION(KernelCpuTime),
                        SORT_FUNCTION(UserCpuTime),
                        SORT_FUNCTION(VerificationStatus),
                        SORT_FUNCTION(VerifiedSigner),
                        SORT_FUNCTION(Reserved1),
                        SORT_FUNCTION(RelativeStartTime),
                        SORT_FUNCTION(Bits),
                        SORT_FUNCTION(Elevation),
                        SORT_FUNCTION(WindowTitle),
                        SORT_FUNCTION(WindowStatus),
                        SORT_FUNCTION(Cycles),
                        SORT_FUNCTION(CyclesDelta),
                        SORT_FUNCTION(Cpu), // CPU History
                        SORT_FUNCTION(PrivateBytes), // Private Bytes History
                        SORT_FUNCTION(IoTotalRate), // I/O History
                        SORT_FUNCTION(DepStatus),
                        SORT_FUNCTION(Virtualized),
                        SORT_FUNCTION(ContextSwitches),
                        SORT_FUNCTION(ContextSwitchesDelta),
                        SORT_FUNCTION(PageFaultsDelta),
                        SORT_FUNCTION(IoReads),
                        SORT_FUNCTION(IoWrites),
                        SORT_FUNCTION(IoOther),
                        SORT_FUNCTION(IoReadBytes),
                        SORT_FUNCTION(IoWriteBytes),
                        SORT_FUNCTION(IoOtherBytes),
                        SORT_FUNCTION(IoReadsDelta),
                        SORT_FUNCTION(IoWritesDelta),
                        SORT_FUNCTION(IoOtherDelta)
                    };
                    int (__cdecl *sortFunction)(const void *, const void *);

                    if (!PhCmForwardSort(
                        (PPH_TREENEW_NODE *)ProcessNodeList->Items,
                        ProcessNodeList->Count,
                        ProcessTreeListSortColumn,
                        ProcessTreeListSortOrder,
                        &ProcessTreeListCm
                        ))
                    {
                        if (ProcessTreeListSortColumn < PHPRTLC_MAXIMUM)
                            sortFunction = sortFunctions[ProcessTreeListSortColumn];
                        else
                            sortFunction = NULL;

                        if (sortFunction)
                        {
                            qsort(ProcessNodeList->Items, ProcessNodeList->Count, sizeof(PVOID), sortFunction);
                        }
                    }

                    getChildren->Children = (PPH_TREENEW_NODE *)ProcessNodeList->Items;
                    getChildren->NumberOfChildren = ProcessNodeList->Count;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            node = (PPH_PROCESS_NODE)isLeaf->Node;

            if (ProcessTreeListSortOrder == NoSortOrder)
                isLeaf->IsLeaf = node->Children->Count == 0;
            else
                isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PPH_PROCESS_ITEM processItem;

            node = (PPH_PROCESS_NODE)getCellText->Node;
            processItem = node->ProcessItem;

            switch (getCellText->Id)
            {
            case PHPRTLC_NAME:
                getCellText->Text = processItem->ProcessName->sr;
                break;
            case PHPRTLC_PID:
                PhInitializeStringRef(&getCellText->Text, processItem->ProcessIdString);
                break;
            case PHPRTLC_CPU:
                {
                    FLOAT cpuUsage;

                    if (!PhCsPropagateCpuUsage || node->Node.Expanded || ProcessTreeListSortOrder != NoSortOrder)
                    {
                        cpuUsage = processItem->CpuUsage * 100;
                    }
                    else
                    {
                        cpuUsage = PhpCalculateInclusiveCpuUsage(node) * 100;
                    }

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
                }
                break;
            case PHPRTLC_IOTOTALRATE:
                {
                    ULONG64 number;

                    if (processItem->IoReadDelta.Delta != processItem->IoReadDelta.Value) // delta is wrong on first run of process provider
                    {
                        number = processItem->IoReadDelta.Delta + processItem->IoWriteDelta.Delta + processItem->IoOtherDelta.Delta;
                        number *= 1000;
                        number /= PhCsUpdateInterval;
                    }
                    else
                    {
                        number = 0;
                    }

                    if (number != 0)
                    {
                        PH_FORMAT format[2];

                        PhInitFormatSize(&format[0], number);
                        PhInitFormatS(&format[1], L"/s");
                        PhSwapReference2(&node->IoTotalRateText, PhFormat(format, 2, 0));
                        getCellText->Text = node->IoTotalRateText->sr;
                    }
                }
                break;
            case PHPRTLC_PRIVATEBYTES:
                PhSwapReference2(&node->PrivateBytesText, PhFormatSize(processItem->VmCounters.PagefileUsage, -1));
                getCellText->Text = node->PrivateBytesText->sr;
                break;
            case PHPRTLC_USERNAME:
                getCellText->Text = PhGetStringRefOrEmpty(processItem->UserName);
                break;
            case PHPRTLC_DESCRIPTION:
                getCellText->Text = PhGetStringRefOrEmpty(processItem->VersionInfo.FileDescription);
                break;
            case PHPRTLC_COMPANYNAME:
                getCellText->Text = PhGetStringRefOrEmpty(processItem->VersionInfo.CompanyName);
                break;
            case PHPRTLC_VERSION:
                getCellText->Text = PhGetStringRefOrEmpty(processItem->VersionInfo.FileVersion);
                break;
            case PHPRTLC_FILENAME:
                getCellText->Text = PhGetStringRefOrEmpty(processItem->FileName);
                break;
            case PHPRTLC_COMMANDLINE:
                getCellText->Text = PhGetStringRefOrEmpty(processItem->CommandLine);
                break;
            case PHPRTLC_PEAKPRIVATEBYTES:
                PhSwapReference2(&node->PeakPrivateBytesText, PhFormatSize(processItem->VmCounters.PeakPagefileUsage, -1));
                getCellText->Text = node->PeakPrivateBytesText->sr;
                break;
            case PHPRTLC_WORKINGSET:
                PhSwapReference2(&node->WorkingSetText, PhFormatSize(processItem->VmCounters.WorkingSetSize, -1));
                getCellText->Text = node->WorkingSetText->sr;
                break;
            case PHPRTLC_PEAKWORKINGSET:
                PhSwapReference2(&node->PeakWorkingSetText, PhFormatSize(processItem->VmCounters.PeakWorkingSetSize, -1));
                getCellText->Text = node->PeakWorkingSetText->sr;
                break;
            case PHPRTLC_PRIVATEWS:
                PhpUpdateProcessNodeWsCounters(node);
                PhSwapReference2(&node->PrivateWsText, PhFormatSize(UInt32x32To64(node->WsCounters.NumberOfPrivatePages, PAGE_SIZE), -1));
                getCellText->Text = node->PrivateWsText->sr;
                break;
            case PHPRTLC_SHAREDWS:
                PhpUpdateProcessNodeWsCounters(node);
                PhSwapReference2(&node->SharedWsText, PhFormatSize(UInt32x32To64(node->WsCounters.NumberOfSharedPages, PAGE_SIZE), -1));
                getCellText->Text = node->SharedWsText->sr;
                break;
            case PHPRTLC_SHAREABLEWS:
                PhpUpdateProcessNodeWsCounters(node);
                PhSwapReference2(&node->ShareableWsText, PhFormatSize(UInt32x32To64(node->WsCounters.NumberOfShareablePages, PAGE_SIZE), -1));
                getCellText->Text = node->ShareableWsText->sr;
                break;
            case PHPRTLC_VIRTUALSIZE:
                PhSwapReference2(&node->VirtualSizeText, PhFormatSize(processItem->VmCounters.VirtualSize, -1));
                getCellText->Text = node->VirtualSizeText->sr;
                break;
            case PHPRTLC_PEAKVIRTUALSIZE:
                PhSwapReference2(&node->PeakVirtualSizeText, PhFormatSize(processItem->VmCounters.PeakVirtualSize, -1));
                getCellText->Text = node->PeakVirtualSizeText->sr;
                break;
            case PHPRTLC_PAGEFAULTS:
                PhSwapReference2(&node->PageFaultsText, PhFormatUInt64(processItem->VmCounters.PageFaultCount, TRUE));
                getCellText->Text = node->PageFaultsText->sr;
                break;
            case PHPRTLC_SESSIONID:
                PhInitializeStringRef(&getCellText->Text, processItem->SessionIdString);
                break;
            case PHPRTLC_PRIORITYCLASS:
                PhInitializeStringRef(&getCellText->Text, PhGetProcessPriorityClassString(processItem->PriorityClass));
                break;
            case PHPRTLC_BASEPRIORITY:
                PhPrintInt32(node->BasePriorityText, processItem->BasePriority);
                PhInitializeStringRef(&getCellText->Text, node->BasePriorityText);
                break;
            case PHPRTLC_THREADS:
                PhPrintUInt32(node->ThreadsText, processItem->NumberOfThreads);
                PhInitializeStringRef(&getCellText->Text, node->ThreadsText);
                break;
            case PHPRTLC_HANDLES:
                PhPrintUInt32(node->HandlesText, processItem->NumberOfHandles);
                PhInitializeStringRef(&getCellText->Text, node->HandlesText);
                break;
            case PHPRTLC_GDIHANDLES:
                PhpUpdateProcessNodeGdiUserHandles(node);
                PhPrintUInt32(node->GdiHandlesText, node->GdiHandles);
                PhInitializeStringRef(&getCellText->Text, node->GdiHandlesText);
                break;
            case PHPRTLC_USERHANDLES:
                PhpUpdateProcessNodeGdiUserHandles(node);
                PhPrintUInt32(node->UserHandlesText, node->UserHandles);
                PhInitializeStringRef(&getCellText->Text, node->UserHandlesText);
                break;
            case PHPRTLC_IORORATE:
                {
                    ULONG64 number;

                    if (processItem->IoReadDelta.Delta != processItem->IoReadDelta.Value)
                    {
                        number = processItem->IoReadDelta.Delta + processItem->IoOtherDelta.Delta;
                        number *= 1000;
                        number /= PhCsUpdateInterval;
                    }
                    else
                    {
                        number = 0;
                    }

                    if (number != 0)
                    {
                        PH_FORMAT format[2];

                        PhInitFormatSize(&format[0], number);
                        PhInitFormatS(&format[1], L"/s");
                        PhSwapReference2(&node->IoRoRateText, PhFormat(format, 2, 0));
                        getCellText->Text = node->IoRoRateText->sr;
                    }
                }
                break;
            case PHPRTLC_IOWRATE:
                {
                    ULONG64 number;

                    if (processItem->IoReadDelta.Delta != processItem->IoReadDelta.Value)
                    {
                        number = processItem->IoWriteDelta.Delta;
                        number *= 1000;
                        number /= PhCsUpdateInterval;
                    }
                    else
                    {
                        number = 0;
                    }

                    if (number != 0)
                    {
                        PH_FORMAT format[2];

                        PhInitFormatSize(&format[0], number);
                        PhInitFormatS(&format[1], L"/s");
                        PhSwapReference2(&node->IoWRateText, PhFormat(format, 2, 0));
                        getCellText->Text = node->IoWRateText->sr;
                    }
                }
                break;
            case PHPRTLC_INTEGRITY:
                if (processItem->IntegrityString)
                    PhInitializeStringRef(&getCellText->Text, processItem->IntegrityString);
                break;
            case PHPRTLC_IOPRIORITY:
                PhpUpdateProcessNodeIoPagePriority(node);

                if (node->IoPriority != -1)
                {
                    if (node->IoPriority < MaxIoPriorityTypes)
                        PhInitializeStringRef(&getCellText->Text, PhIoPriorityHintNames[node->IoPriority]);
                }

                break;
            case PHPRTLC_PAGEPRIORITY:
                PhpUpdateProcessNodeIoPagePriority(node);

                if (node->PagePriority != -1)
                {
                    PhPrintUInt32(node->PagePriorityText, node->PagePriority);
                    PhInitializeStringRef(&getCellText->Text, node->PagePriorityText);
                }

                break;
            case PHPRTLC_STARTTIME:
                {
                    SYSTEMTIME systemTime;

                    if (processItem->CreateTime.QuadPart != 0)
                    {
                        PhLargeIntegerToLocalSystemTime(&systemTime, &processItem->CreateTime);
                        PhSwapReference2(&node->StartTimeText, PhFormatDateTime(&systemTime));
                        getCellText->Text = node->StartTimeText->sr;
                    }
                }
                break;
            case PHPRTLC_TOTALCPUTIME:
                PhPrintTimeSpan(node->TotalCpuTimeText,
                    processItem->KernelTime.QuadPart + processItem->UserTime.QuadPart,
                    PH_TIMESPAN_HMSM);
                PhInitializeStringRef(&getCellText->Text, node->TotalCpuTimeText);
                break;
            case PHPRTLC_KERNELCPUTIME:
                PhPrintTimeSpan(node->KernelCpuTimeText, processItem->KernelTime.QuadPart, PH_TIMESPAN_HMSM);
                PhInitializeStringRef(&getCellText->Text, node->KernelCpuTimeText);
                break;
            case PHPRTLC_USERCPUTIME:
                PhPrintTimeSpan(node->UserCpuTimeText, processItem->UserTime.QuadPart, PH_TIMESPAN_HMSM);
                PhInitializeStringRef(&getCellText->Text, node->UserCpuTimeText);
                break;
            case PHPRTLC_VERIFICATIONSTATUS:
                if (processItem->VerifyResult == VrTrusted)
                    PhInitializeStringRef(&getCellText->Text, L"Trusted");
                break;
            case PHPRTLC_VERIFIEDSIGNER:
                getCellText->Text = PhGetStringRefOrEmpty(processItem->VerifySignerName);
                break;
            case PHPRTLC_RELATIVESTARTTIME:
                {
                    if (processItem->CreateTime.QuadPart != 0)
                    {
                        LARGE_INTEGER currentTime;

                        PhQuerySystemTime(&currentTime);
                        PhSwapReference2(&node->RelativeStartTimeText,
                            PhFormatTimeSpanRelative(currentTime.QuadPart - processItem->CreateTime.QuadPart));
                        getCellText->Text = node->RelativeStartTimeText->sr;
                    }
                }
                break;
            case PHPRTLC_BITS:
#ifdef _M_X64
                PhInitializeStringRef(&getCellText->Text, processItem->IsWow64 ? L"32" : L"64");
#else
                PhInitializeStringRef(&getCellText->Text, L"32");
#endif
                break;
            case PHPRTLC_ELEVATION:
                {
                    PWSTR type;

                    if (WINDOWS_HAS_UAC)
                    {
                        switch (processItem->ElevationType)
                        {
                        case TokenElevationTypeDefault:
                            type = L"N/A";
                            break;
                        case TokenElevationTypeLimited:
                            type = L"Limited";
                            break;
                        case TokenElevationTypeFull:
                            type = L"Full";
                            break;
                        default:
                            type = L"N/A";
                            break;
                        }
                    }
                    else
                    {
                        type = L"";
                    }

                    PhInitializeStringRef(&getCellText->Text, type);
                }
                break;
            case PHPRTLC_WINDOWTITLE:
                PhpUpdateProcessNodeWindow(node);
                PhSwapReference(&node->WindowTitleText, node->WindowText);
                getCellText->Text = PhGetStringRef(node->WindowTitleText);
                break;
            case PHPRTLC_WINDOWSTATUS:
                PhpUpdateProcessNodeWindow(node);

                if (node->WindowHandle)
                    PhInitializeStringRef(&getCellText->Text, node->WindowHung ? L"Not responding" : L"Running");

                break;
            case PHPRTLC_CYCLES:
                getCellText->Text = PhGetStringRef(node->CyclesText);
                break;
            case PHPRTLC_CYCLESDELTA:
                getCellText->Text = PhGetStringRef(node->CyclesDeltaText);
                break;
            case PHPRTLC_DEPSTATUS:
                PhpUpdateProcessNodeDepStatus(node);

                if (node->DepStatus & PH_PROCESS_DEP_ENABLED)
                {
                    if (node->DepStatus & PH_PROCESS_DEP_PERMANENT)
                        PhInitializeStringRef(&getCellText->Text, L"DEP (Permanent)");
                    else
                        PhInitializeStringRef(&getCellText->Text, L"DEP");
                }

                break;
            case PHPRTLC_VIRTUALIZED:
                PhpUpdateProcessNodeToken(node);

                if (node->VirtualizationEnabled)
                    PhInitializeStringRef(&getCellText->Text, L"Virtualized");

                break;
            case PHPRTLC_CONTEXTSWITCHES:
                if (processItem->ContextSwitchesDelta.Value != 0)
                {
                    PhSwapReference2(&node->ContextSwitchesText, PhFormatUInt64(processItem->ContextSwitchesDelta.Value, TRUE));
                    getCellText->Text = node->ContextSwitchesText->sr;
                }
                break;
            case PHPRTLC_CONTEXTSWITCHESDELTA:
                if ((LONG)processItem->ContextSwitchesDelta.Delta > 0) // the delta may be negative if a thread exits - just don't show anything
                {
                    PhSwapReference2(&node->ContextSwitchesDeltaText, PhFormatUInt64(processItem->ContextSwitchesDelta.Delta, TRUE));
                    getCellText->Text = node->ContextSwitchesDeltaText->sr;
                }
                break;
            case PHPRTLC_PAGEFAULTSDELTA:
                if (processItem->PageFaultsDelta.Delta != 0)
                {
                    PhSwapReference2(&node->PageFaultsDeltaText, PhFormatUInt64(processItem->PageFaultsDelta.Delta, TRUE));
                    getCellText->Text = node->PageFaultsDeltaText->sr;
                }
                break;
            case PHPRTLC_IOREADS:
                if (processItem->IoReadCountDelta.Value != 0)
                {
                    PhSwapReference2(&node->IoGroupText[0], PhFormatUInt64(processItem->IoReadCountDelta.Value, TRUE));
                    getCellText->Text = node->IoGroupText[0]->sr;
                }
                break;
            case PHPRTLC_IOWRITES:
                if (processItem->IoWriteCountDelta.Value != 0)
                {
                    PhSwapReference2(&node->IoGroupText[1], PhFormatUInt64(processItem->IoWriteCountDelta.Value, TRUE));
                    getCellText->Text = node->IoGroupText[1]->sr;
                }
                break;
            case PHPRTLC_IOOTHER:
                if (processItem->IoOtherCountDelta.Value != 0)
                {
                    PhSwapReference2(&node->IoGroupText[2], PhFormatUInt64(processItem->IoOtherCountDelta.Value, TRUE));
                    getCellText->Text = node->IoGroupText[2]->sr;
                }
                break;
            case PHPRTLC_IOREADBYTES:
                if (processItem->IoReadDelta.Value != 0)
                {
                    PhSwapReference2(&node->IoGroupText[3], PhFormatSize(processItem->IoReadDelta.Value, -1));
                    getCellText->Text = node->IoGroupText[3]->sr;
                }
                break;
            case PHPRTLC_IOWRITEBYTES:
                if (processItem->IoWriteDelta.Value != 0)
                {
                    PhSwapReference2(&node->IoGroupText[4], PhFormatSize(processItem->IoWriteDelta.Value, -1));
                    getCellText->Text = node->IoGroupText[4]->sr;
                }
                break;
            case PHPRTLC_IOOTHERBYTES:
                if (processItem->IoOtherDelta.Value != 0)
                {
                    PhSwapReference2(&node->IoGroupText[5], PhFormatSize(processItem->IoOtherDelta.Value, -1));
                    getCellText->Text = node->IoGroupText[5]->sr;
                }
                break;
            case PHPRTLC_IOREADSDELTA:
                if (processItem->IoReadCountDelta.Delta != 0)
                {
                    PhSwapReference2(&node->IoGroupText[6], PhFormatUInt64(processItem->IoReadCountDelta.Delta, TRUE));
                    getCellText->Text = node->IoGroupText[6]->sr;
                }
                break;
            case PHPRTLC_IOWRITESDELTA:
                if (processItem->IoWriteCountDelta.Delta != 0)
                {
                    PhSwapReference2(&node->IoGroupText[7], PhFormatUInt64(processItem->IoWriteCountDelta.Delta, TRUE));
                    getCellText->Text = node->IoGroupText[7]->sr;
                }
                break;
            case PHPRTLC_IOOTHERDELTA:
                if (processItem->IoOtherCountDelta.Delta != 0)
                {
                    PhSwapReference2(&node->IoGroupText[8], PhFormatUInt64(processItem->IoOtherCountDelta.Delta, TRUE));
                    getCellText->Text = node->IoGroupText[8]->sr;
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
            PPH_PROCESS_ITEM processItem;

            node = (PPH_PROCESS_NODE)getNodeColor->Node;
            processItem = node->ProcessItem;

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_GET_HIGHLIGHTING_COLOR getHighlightingColor;

                getHighlightingColor.Parameter = processItem;
                getHighlightingColor.BackColor = RGB(0xff, 0xff, 0xff);
                getHighlightingColor.Handled = FALSE;
                getHighlightingColor.Cache = FALSE;

                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackGetProcessHighlightingColor), &getHighlightingColor);

                if (getHighlightingColor.Handled)
                {
                    getNodeColor->BackColor = getHighlightingColor.BackColor;
                    getNodeColor->Flags = TN_AUTO_FORECOLOR;

                    if (getHighlightingColor.Cache)
                        getNodeColor->Flags |= TN_CACHE;

                    return TRUE;
                }
            }

            if (!processItem)
                ; // Dummy
            else if (PhCsUseColorDebuggedProcesses && processItem->IsBeingDebugged)
                getNodeColor->BackColor = PhCsColorDebuggedProcesses;
            else if (PhCsUseColorSuspended && processItem->IsSuspended)
                getNodeColor->BackColor = PhCsColorSuspended;
            else if (PhCsUseColorElevatedProcesses && processItem->IsElevated)
                getNodeColor->BackColor = PhCsColorElevatedProcesses;
            else if (PhCsUseColorPosixProcesses && processItem->IsPosix)
                getNodeColor->BackColor = PhCsColorPosixProcesses;
            else if (PhCsUseColorWow64Processes && processItem->IsWow64)
                getNodeColor->BackColor = PhCsColorWow64Processes;
            else if (PhCsUseColorJobProcesses && processItem->IsInSignificantJob)
                getNodeColor->BackColor = PhCsColorJobProcesses;
            //else if (
            //    PhCsUseColorPacked &&
            //    (processItem->VerifyResult != VrUnknown &&
            //    processItem->VerifyResult != VrNoSignature &&
            //    processItem->VerifyResult != VrTrusted
            //    ))
            //    getNodeColor->BackColor = PhCsColorPacked;
            else if (PhCsUseColorDotNet && processItem->IsDotNet)
                getNodeColor->BackColor = PhCsColorDotNet;
            else if (PhCsUseColorPacked && processItem->IsPacked)
                getNodeColor->BackColor = PhCsColorPacked;
            else if (PhCsUseColorServiceProcesses && processItem->ServiceList && processItem->ServiceList->Count != 0)
                getNodeColor->BackColor = PhCsColorServiceProcesses;
            else if (
                PhCsUseColorSystemProcesses &&
                processItem->UserName &&
                PhEqualString(processItem->UserName, PhLocalSystemName, TRUE)
                )
                getNodeColor->BackColor = PhCsColorSystemProcesses;
            else if (
                PhCsUseColorOwnProcesses &&
                processItem->UserName &&
                PhCurrentUserName &&
                PhEqualString(processItem->UserName, PhCurrentUserName, TRUE)
                )
                getNodeColor->BackColor = PhCsColorOwnProcesses;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewGetNodeIcon:
        {
            PPH_TREENEW_GET_NODE_ICON getNodeIcon = Parameter1;

            node = (PPH_PROCESS_NODE)getNodeIcon->Node;

            if (node->ProcessItem->SmallIcon)
            {
                getNodeIcon->Icon = node->ProcessItem->SmallIcon;
            }
            else
            {
                PhGetStockApplicationIcon(&getNodeIcon->Icon, NULL);
            }

            getNodeIcon->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetCellTooltip:
        {
            PPH_TREENEW_GET_CELL_TOOLTIP getCellTooltip = Parameter1;

            node = (PPH_PROCESS_NODE)getCellTooltip->Node;

            if (getCellTooltip->Column->Id != 0)
                return FALSE;

            if (!node->TooltipText)
                node->TooltipText = PhGetProcessTooltipText(node->ProcessItem);

            if (!PhIsNullOrEmptyString(node->TooltipText))
            {
                getCellTooltip->Text = node->TooltipText->sr;
                getCellTooltip->Unfolding = FALSE;
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;
            PPH_PROCESS_ITEM processItem;
            RECT rect;
            PH_GRAPH_DRAW_INFO drawInfo;
            POINT origPoint;

            node = (PPH_PROCESS_NODE)customDraw->Node;
            processItem = node->ProcessItem;
            rect = customDraw->CellRect;

            if (rect.right - rect.left <= 1)
                break; // nothing to draw

            // Generic graph pre-processing
            switch (customDraw->Column->Id)
            {
            case PHPRTLC_CPUHISTORY:
            case PHPRTLC_PRIVATEBYTESHISTORY:
            case PHPRTLC_IOHISTORY:
                memset(&drawInfo, 0, sizeof(PH_GRAPH_DRAW_INFO));
                drawInfo.Width = rect.right - rect.left - 1; // leave a small gap
                drawInfo.Height = rect.bottom - rect.top - 1; // leave a small gap
                drawInfo.Step = 2;
                drawInfo.BackColor = RGB(0x00, 0x00, 0x00);
                break;
            }

            // Specific graph processing
            switch (customDraw->Column->Id)
            {
            case PHPRTLC_CPUHISTORY:
                {
                    drawInfo.Flags = PH_GRAPH_USE_LINE_2;
                    drawInfo.LineColor1 = PhCsColorCpuKernel;
                    drawInfo.LineColor2 = PhCsColorCpuUser;
                    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorCpuKernel);
                    drawInfo.LineBackColor2 = PhHalveColorBrightness(PhCsColorCpuUser);

                    PhGetDrawInfoGraphBuffers(
                        &node->CpuGraphBuffers,
                        &drawInfo,
                        processItem->CpuKernelHistory.Count
                        );

                    if (!node->CpuGraphBuffers.Valid)
                    {
                        PhCopyCircularBuffer_FLOAT(&processItem->CpuKernelHistory,
                            node->CpuGraphBuffers.Data1, drawInfo.LineDataCount);
                        PhCopyCircularBuffer_FLOAT(&processItem->CpuUserHistory,
                            node->CpuGraphBuffers.Data2, drawInfo.LineDataCount);
                        node->CpuGraphBuffers.Valid = TRUE;
                    }
                }
                break;
            case PHPRTLC_PRIVATEBYTESHISTORY:
                {
                    drawInfo.Flags = 0;
                    drawInfo.LineColor1 = PhCsColorPrivate;
                    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorPrivate);

                    PhGetDrawInfoGraphBuffers(
                        &node->PrivateGraphBuffers,
                        &drawInfo,
                        processItem->PrivateBytesHistory.Count
                        );

                    if (!node->PrivateGraphBuffers.Valid)
                    {
                        ULONG i;
                        FLOAT total;
                        FLOAT max;

                        for (i = 0; i < drawInfo.LineDataCount; i++)
                        {
                            node->PrivateGraphBuffers.Data1[i] =
                                (FLOAT)PhGetItemCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, i);
                        }

                        // This makes it easier for the user to see what processes are hogging memory.
                        // Scaling is still *not* consistent across all graphs.
                        total = (FLOAT)PhPerfInformation.CommittedPages * PAGE_SIZE / 4; // divide by 4 to make the scaling a bit better
                        max = (FLOAT)processItem->VmCounters.PeakPagefileUsage;

                        if (max < total)
                            max = total;

                        if (max != 0)
                        {
                            // Scale the data.
                            PhxfDivideSingle2U(
                                node->PrivateGraphBuffers.Data1,
                                max,
                                drawInfo.LineDataCount
                                );
                        }

                        node->PrivateGraphBuffers.Valid = TRUE;
                    }
                }
                break;
            case PHPRTLC_IOHISTORY:
                {
                    drawInfo.Flags = PH_GRAPH_USE_LINE_2;
                    drawInfo.LineColor1 = PhCsColorIoReadOther;
                    drawInfo.LineColor2 = PhCsColorIoWrite;
                    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorIoReadOther);
                    drawInfo.LineBackColor2 = PhHalveColorBrightness(PhCsColorIoWrite);

                    PhGetDrawInfoGraphBuffers(
                        &node->IoGraphBuffers,
                        &drawInfo,
                        processItem->IoReadHistory.Count
                        );

                    if (!node->IoGraphBuffers.Valid)
                    {
                        ULONG i;
                        FLOAT total;
                        FLOAT max = 0;

                        for (i = 0; i < drawInfo.LineDataCount; i++)
                        {
                            FLOAT data1;
                            FLOAT data2;

                            node->IoGraphBuffers.Data1[i] = data1 =
                                (FLOAT)PhGetItemCircularBuffer_ULONG64(&processItem->IoReadHistory, i) +
                                (FLOAT)PhGetItemCircularBuffer_ULONG64(&processItem->IoOtherHistory, i);
                            node->IoGraphBuffers.Data2[i] = data2 =
                                (FLOAT)PhGetItemCircularBuffer_ULONG64(&processItem->IoWriteHistory, i);

                            if (max < data1 + data2)
                                max = data1 + data2;
                        }

                        // Make the scaling a bit more consistent across the processes.
                        // It does *not* scale all graphs using the same maximum.
                        total = (FLOAT)(PhIoReadDelta.Delta + PhIoWriteDelta.Delta + PhIoOtherDelta.Delta);

                        if (max < total)
                            max = total;

                        if (max != 0)
                        {
                            // Scale the data.

                            PhxfDivideSingle2U(
                                node->IoGraphBuffers.Data1,
                                max,
                                drawInfo.LineDataCount
                                );
                            PhxfDivideSingle2U(
                                node->IoGraphBuffers.Data2,
                                max,
                                drawInfo.LineDataCount
                                );
                        }

                        node->IoGraphBuffers.Valid = TRUE;
                    }
                }
                break;
            }

            // Draw the graph.
            switch (customDraw->Column->Id)
            {
            case PHPRTLC_CPUHISTORY:
            case PHPRTLC_PRIVATEBYTESHISTORY:
            case PHPRTLC_IOHISTORY:
                SetViewportOrgEx(customDraw->Dc, rect.left, rect.top + 1, &origPoint); // + 1 for small gap
                PhDrawGraph(customDraw->Dc, &drawInfo);
                SetViewportOrgEx(customDraw->Dc, origPoint.x, origPoint.y, NULL);
                break;
            }
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &ProcessTreeListSortColumn, &ProcessTreeListSortOrder);
            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_PROCESS_COPY, 0);
                break;
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
    case TreeNewHeaderRightClick:
        {
            HMENU menu;
            HMENU subMenu;
            POINT point;

            menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_PROCESSHEADER));
            subMenu = GetSubMenu(menu, 0);
            GetCursorPos(&point);

            if ((UINT)TrackPopupMenu(
                subMenu,
                TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                point.x,
                point.y,
                0,
                hwnd,
                NULL
                ) == ID_HEADER_CHOOSECOLUMNS)
            {
                PhShowChooseColumnsDialog(hwnd, hwnd, PH_CONTROL_TYPE_TREE_NEW);
                PhpUpdateNeedCyclesInformation();

                // Make sure the column we're sorting by is actually visible, 
                // and if not, don't sort any more.
                if (ProcessTreeListSortOrder != NoSortOrder)
                {
                    PH_TREENEW_COLUMN column;

                    TreeNew_GetColumn(ProcessTreeListHandle, ProcessTreeListSortColumn, &column);

                    if (!column.Visible)
                        TreeNew_SetSort(ProcessTreeListHandle, 0, NoSortOrder);
                }
            }

            DestroyMenu(menu);
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(PhMainWndHandle, WM_COMMAND, ID_PROCESS_PROPERTIES, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPOINT location = Parameter1;

            PhShowProcessContextMenu(*location);
        }
        return TRUE;
    case TreeNewNodeExpanding:
        {
            node = Parameter1;

            if (PhCsPropagateCpuUsage)
                PhUpdateProcessNode(node);
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
            PhAddItemList(list, node->ProcessItem);
        }
    }

    *Processes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfProcesses = list->Count;

    PhDereferenceObject(list);
}

VOID PhDeselectAllProcessNodes()
{
    TreeNew_DeselectRange(ProcessTreeListHandle, 0, -1);
}

VOID PhInvalidateAllProcessNodes()
{
    ULONG i;

    for (i = 0; i < ProcessNodeList->Count; i++)
    {
        PPH_PROCESS_NODE node = ProcessNodeList->Items[i];

        memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PHPRTLC_MAXIMUM);
        PhInvalidateTreeNewNode(&node->Node, TN_CACHE_COLOR);
        node->ValidMask = 0;
    }

    InvalidateRect(ProcessTreeListHandle, NULL, TRUE);
}

VOID PhSelectAndEnsureVisibleProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    )
{
    PPH_PROCESS_NODE processNode;
    BOOLEAN needsRestructure = FALSE;

    PhDeselectAllProcessNodes();

    if (!ProcessNode->Node.Visible)
        return;

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

    if (needsRestructure)
        TreeNew_NodesStructured(ProcessTreeListHandle);

    TreeNew_SetFocusNode(ProcessTreeListHandle, &ProcessNode->Node);
    TreeNew_SetMarkNode(ProcessTreeListHandle, &ProcessNode->Node);
    TreeNew_EnsureVisible(ProcessTreeListHandle, &ProcessNode->Node);
    TreeNew_InvalidateNode(ProcessTreeListHandle, &ProcessNode->Node);
}

PPH_PROCESS_TREE_FILTER_ENTRY PhAddProcessTreeFilter(
    __in PPH_PROCESS_TREE_FILTER Filter,
    __in_opt PVOID Context
    )
{
    PPH_PROCESS_TREE_FILTER_ENTRY entry;

    entry = PhAllocate(sizeof(PH_PROCESS_TREE_FILTER_ENTRY));
    entry->Filter = Filter;
    entry->Context = Context;

    if (!ProcessTreeFilterList)
        ProcessTreeFilterList = PhCreateList(2);

    PhAddItemList(ProcessTreeFilterList, entry);

    return entry;
}

VOID PhRemoveProcessTreeFilter(
    __in PPH_PROCESS_TREE_FILTER_ENTRY Entry
    )
{
    ULONG index;

    if (!ProcessTreeFilterList)
        return;

    index = PhFindItemList(ProcessTreeFilterList, Entry);

    if (index != -1)
    {
        PhRemoveItemList(ProcessTreeFilterList, index);
        PhFree(Entry);
    }
}

BOOLEAN PhpApplyProcessTreeFiltersToNode(
    __in PPH_PROCESS_NODE Node
    )
{
    BOOLEAN show;
    ULONG i;

    show = TRUE;

    if (ProcessTreeFilterList)
    {
        for (i = 0; i < ProcessTreeFilterList->Count; i++)
        {
            PPH_PROCESS_TREE_FILTER_ENTRY entry;

            entry = ProcessTreeFilterList->Items[i];

            if (!entry->Filter(Node, entry->Context))
            {
                show = FALSE;
                break;
            }
        }
    }

    return show;
}

VOID PhApplyProcessTreeFilters()
{
    ULONG i;

    for (i = 0; i < ProcessNodeList->Count; i++)
    {
        PPH_PROCESS_NODE node;

        node = ProcessNodeList->Items[i];
        node->Node.Visible = PhpApplyProcessTreeFiltersToNode(node);

        if (!node->Node.Visible && node->Node.Selected)
        {
            node->Node.Selected = FALSE;
        }
    }

    TreeNew_NodesStructured(ProcessTreeListHandle);
}

VOID PhpPopulateTableWithProcessNodes(
    __in HWND TreeListHandle,
    __in PPH_PROCESS_NODE Node,
    __in ULONG Level,
    __in PPH_STRING **Table,
    __inout PULONG Index,
    __in PULONG DisplayToId,
    __in ULONG Columns
    )
{
    ULONG i;

    for (i = 0; i < Columns; i++)
    {
        PH_TREENEW_GET_CELL_TEXT getCellText;
        PPH_STRING text;

        getCellText.Node = &Node->Node;
        getCellText.Id = DisplayToId[i];
        PhInitializeEmptyStringRef(&getCellText.Text);
        TreeNew_GetCellText(TreeListHandle, &getCellText);

        if (i != 0)
        {
            text = PhaCreateStringEx(getCellText.Text.Buffer, getCellText.Text.Length);
        }
        else
        {
            // If this is the first column in the row, add some indentation.
            text = PhaCreateStringEx(
                NULL,
                getCellText.Text.Length + Level * 2 * sizeof(WCHAR)
                );
            wmemset(text->Buffer, ' ', Level * 2);
            memcpy(&text->Buffer[Level * 2], getCellText.Text.Buffer, getCellText.Text.Length);
        }

        Table[*Index][i] = text;
    }

    (*Index)++;

    // Process the children.
    for (i = 0; i < Node->Children->Count; i++)
    {
        PhpPopulateTableWithProcessNodes(
            TreeListHandle,
            Node->Children->Items[i],
            Level + 1,
            Table,
            Index,
            DisplayToId,
            Columns
            );
    }
}

PPH_LIST PhGetProcessTreeListLines(
    __in HWND TreeListHandle,
    __in ULONG NumberOfNodes,
    __in PPH_LIST RootNodes,
    __in ULONG Mode
    )
{
    PH_AUTO_POOL autoPool;
    PPH_LIST lines;
    // The number of rows in the table (including +1 for the column headers).
    ULONG rows;
    // The number of columns.
    ULONG columns;
    // A column display index to ID map.
    ULONG displayToId[PHPRTLC_MAXIMUM];
    // A column display index to text map.
    PWSTR displayToText[PHPRTLC_MAXIMUM];
    // The actual string table.
    PPH_STRING **table;
    ULONG i;
    ULONG j;

    // Use a local auto-pool to make memory mangement a bit less painful.
    PhInitializeAutoPool(&autoPool);

    rows = NumberOfNodes + 1;

    // Create the display index to ID map.
    PhMapDisplayIndexTreeNew(TreeListHandle, PHPRTLC_MAXIMUM, displayToId, displayToText, &columns);

    PhaCreateTextTable(&table, rows, columns);

    // Populate the first row with the column headers.
    for (i = 0; i < columns; i++)
    {
        table[0][i] = PhaCreateString(displayToText[i]);
    }

    // Go through the nodes in the process tree and populate each cell of the table.

    j = 1; // index starts at one because the first row contains the column headers.

    for (i = 0; i < RootNodes->Count; i++)
    {
        PhpPopulateTableWithProcessNodes(
            TreeListHandle,
            RootNodes->Items[i],
            0,
            table,
            &j,
            displayToId,
            columns
            );
    }

    lines = PhaFormatTextTable(table, rows, columns, Mode);

    PhDeleteAutoPool(&autoPool);

    return lines;
}

VOID PhCopyProcessTree()
{
    PPH_FULL_STRING text;

    text = PhGetTreeNewText(ProcessTreeListHandle, PHPRTLC_MAXIMUM);
    PhSetClipboardStringEx(ProcessTreeListHandle, text->Buffer, text->Length);
    PhDereferenceObject(text);
}

VOID PhWriteProcessTree(
    __inout PPH_FILE_STREAM FileStream,
    __in ULONG Mode
    )
{
    PPH_LIST lines;
    ULONG i;

    lines = PhGetProcessTreeListLines(
        ProcessTreeListHandle,
        ProcessNodeList->Count,
        ProcessNodeRootList,
        Mode
        );

    for (i = 0; i < lines->Count; i++)
    {
        PPH_STRING line;

        line = lines->Items[i];
        PhWriteStringAsAnsiFileStream(FileStream, &line->sr);
        PhDereferenceObject(line);
        PhWriteStringAsAnsiFileStream2(FileStream, L"\r\n");
    }

    PhDereferenceObject(lines);
}
