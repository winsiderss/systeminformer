/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015
 *     dmex    2017-2018
 *
 */

#include <phapp.h>
#include <memlist.h>

#include <emenu.h>

#include <extmgri.h>
#include <memprv.h>
#include <settings.h>
#include <phsettings.h>

VOID PhpClearMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context
    );

VOID PhpDestroyMemoryNode(
    _In_ PPH_MEMORY_NODE MemoryNode
    );

LONG PhpMemoryTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    );

BOOLEAN NTAPI PhpMemoryTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

VOID PhInitializeMemoryList(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PPH_MEMORY_LIST_CONTEXT Context
    )
{
    memset(Context, 0, sizeof(PH_MEMORY_LIST_CONTEXT));

    Context->AllocationBaseNodeList = PhCreateList(100);
    Context->RegionNodeList = PhCreateList(400);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetCallback(TreeNewHandle, PhpMemoryTreeNewCallback, Context);

    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    // Default columns
    PhAddTreeNewColumn(TreeNewHandle, PHMMTLC_BASEADDRESS, TRUE, L"Base address", 120, PH_ALIGN_LEFT|PH_ALIGN_MONOSPACE_FONT, -2, DT_RIGHT);
    PhAddTreeNewColumn(TreeNewHandle, PHMMTLC_TYPE, TRUE, L"Type", 90, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumnEx(TreeNewHandle, PHMMTLC_SIZE, TRUE, L"Size", 80, PH_ALIGN_RIGHT, 1, DT_RIGHT, TRUE);
    PhAddTreeNewColumn(TreeNewHandle, PHMMTLC_PROTECTION, TRUE, L"Protection", 60, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(TreeNewHandle, PHMMTLC_USE, TRUE, L"Use", 200, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumnEx(TreeNewHandle, PHMMTLC_TOTALWS, TRUE, L"Total WS", 80, PH_ALIGN_RIGHT, 4, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(TreeNewHandle, PHMMTLC_PRIVATEWS, TRUE, L"Private WS", 80, PH_ALIGN_RIGHT, 5, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(TreeNewHandle, PHMMTLC_SHAREABLEWS, TRUE, L"Shareable WS", 80, PH_ALIGN_RIGHT, 6, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(TreeNewHandle, PHMMTLC_SHAREDWS, TRUE, L"Shared WS", 80, PH_ALIGN_RIGHT, 7, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(TreeNewHandle, PHMMTLC_LOCKEDWS, TRUE, L"Locked WS", 80, PH_ALIGN_RIGHT, 8, DT_RIGHT, TRUE);

    PhAddTreeNewColumnEx(TreeNewHandle, PHMMTLC_COMMITTED, FALSE, L"Committed", 80, PH_ALIGN_RIGHT, 9, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(TreeNewHandle, PHMMTLC_PRIVATE, FALSE, L"Private", 80, PH_ALIGN_RIGHT, 10, DT_RIGHT, TRUE);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);

    TreeNew_SetTriState(TreeNewHandle, TRUE);
    TreeNew_SetSort(TreeNewHandle, 0, NoSortOrder);

    PhCmInitializeManager(&Context->Cm, TreeNewHandle, PHMMTLC_MAXIMUM, PhpMemoryTreeNewPostSortFunction);

    PhInitializeTreeNewFilterSupport(&Context->AllocationTreeFilterSupport, TreeNewHandle, Context->AllocationBaseNodeList);
    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, TreeNewHandle, Context->RegionNodeList);
}

VOID PhpClearMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->AllocationBaseNodeList->Count; i++)
        PhpDestroyMemoryNode(Context->AllocationBaseNodeList->Items[i]);
    for (i = 0; i < Context->RegionNodeList->Count; i++)
        PhpDestroyMemoryNode(Context->RegionNodeList->Items[i]);

    PhClearList(Context->AllocationBaseNodeList);
    PhClearList(Context->RegionNodeList);
}

VOID PhDeleteMemoryList(
    _In_ PPH_MEMORY_LIST_CONTEXT Context
    )
{
    PhDeleteTreeNewFilterSupport(&Context->AllocationTreeFilterSupport);
    PhDeleteTreeNewFilterSupport(&Context->TreeFilterSupport);

    PhCmDeleteManager(&Context->Cm);

    PhpClearMemoryList(Context);
    PhDereferenceObject(Context->AllocationBaseNodeList);
    PhDereferenceObject(Context->RegionNodeList);
}

VOID PhLoadSettingsMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context
    )
{
    ULONG flags;
    PPH_STRING settings;
    PPH_STRING sortSettings;

    flags = PhGetIntegerSetting(L"MemoryListFlags");
    settings = PhGetStringSetting(L"MemoryTreeListColumns");
    sortSettings = PhGetStringSetting(L"MemoryTreeListSort");

    Context->Flags = flags;
    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);

    PhSetIntegerSetting(L"MemoryListFlags", Context->Flags);
    PhSetStringSetting2(L"MemoryTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"MemoryTreeListSort", &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSetOptionsMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ ULONG Options
    )
{
    switch (Options)
    {
    case PH_MEMORY_FLAGS_FREE_OPTION:
        Context->HideFreeRegions = !Context->HideFreeRegions;
        break;
    case PH_MEMORY_FLAGS_RESERVED_OPTION:
        Context->HideReservedRegions = !Context->HideReservedRegions;
        break;
    case PH_MEMORY_FLAGS_PRIVATE_OPTION:
        Context->HighlightPrivatePages = !Context->HighlightPrivatePages;
        break;
    case PH_MEMORY_FLAGS_SYSTEM_OPTION:
        Context->HighlightSystemPages = !Context->HighlightSystemPages;
        break;
    case PH_MEMORY_FLAGS_CFG_OPTION:
        Context->HighlightCfgPages = !Context->HighlightCfgPages;
        break;
    case PH_MEMORY_FLAGS_EXECUTE_OPTION:
        Context->HighlightExecutePages = !Context->HighlightExecutePages;
        break;
    case PH_MEMORY_FLAGS_GUARD_OPTION:
        Context->HideGuardRegions = !Context->HideGuardRegions;
        break;
    }
}

VOID PhpDestroyMemoryNode(
    _In_ PPH_MEMORY_NODE MemoryNode
    )
{
    PhEmCallObjectOperation(EmMemoryNodeType, MemoryNode, EmObjectDelete);

    if (MemoryNode->SizeText)
        PhDereferenceObject(MemoryNode->SizeText);
    if (MemoryNode->UseText)
        PhDereferenceObject(MemoryNode->UseText);
    if (MemoryNode->TotalWsText)
        PhDereferenceObject(MemoryNode->TotalWsText);
    if (MemoryNode->PrivateWsText)
        PhDereferenceObject(MemoryNode->PrivateWsText);
    if (MemoryNode->ShareableWsText)
        PhDereferenceObject(MemoryNode->ShareableWsText);
    if (MemoryNode->SharedWsText)
        PhDereferenceObject(MemoryNode->SharedWsText);
    if (MemoryNode->LockedWsText)
        PhDereferenceObject(MemoryNode->LockedWsText);
    if (MemoryNode->CommittedText)
        PhDereferenceObject(MemoryNode->CommittedText);
    if (MemoryNode->PrivateText)
        PhDereferenceObject(MemoryNode->PrivateText);
    if (MemoryNode->Children)
        PhDereferenceObject(MemoryNode->Children);

    PhDereferenceObject(MemoryNode->MemoryItem);

    PhFree(MemoryNode);
}

PPH_MEMORY_NODE PhpAddAllocationBaseNode(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ PVOID AllocationBase
    )
{
    PPH_MEMORY_NODE memoryNode;
    PPH_MEMORY_ITEM memoryItem;

    memoryNode = PhAllocate(PhEmGetObjectSize(EmMemoryNodeType, sizeof(PH_MEMORY_NODE)));
    memset(memoryNode, 0, sizeof(PH_MEMORY_NODE));
    PhInitializeTreeNewNode(&memoryNode->Node);
    memoryNode->Node.Expanded = FALSE;

    memoryNode->IsAllocationBase = TRUE;
    memoryItem = PhCreateMemoryItem();
    memoryNode->MemoryItem = memoryItem;

    memoryItem->BaseAddress = AllocationBase;
    memoryItem->AllocationBase = AllocationBase;

    memoryNode->Children = PhCreateList(1);

    memset(memoryNode->TextCache, 0, sizeof(PH_STRINGREF) * PHMMTLC_MAXIMUM);
    memoryNode->Node.TextCache = memoryNode->TextCache;
    memoryNode->Node.TextCacheSize = PHMMTLC_MAXIMUM;

    PhAddItemList(Context->AllocationBaseNodeList, memoryNode);

    if (Context->AllocationTreeFilterSupport.FilterList)
        memoryNode->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->AllocationTreeFilterSupport, &memoryNode->Node);

    PhEmCallObjectOperation(EmMemoryNodeType, memoryNode, EmObjectCreate);

    return memoryNode;
}

PPH_MEMORY_NODE PhpAddRegionNode(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ PPH_MEMORY_ITEM MemoryItem
    )
{
    PPH_MEMORY_NODE memoryNode;

    memoryNode = PhAllocate(PhEmGetObjectSize(EmMemoryNodeType, sizeof(PH_MEMORY_NODE)));
    memset(memoryNode, 0, sizeof(PH_MEMORY_NODE));
    PhInitializeTreeNewNode(&memoryNode->Node);

    memoryNode->IsAllocationBase = FALSE;
    memoryNode->MemoryItem = MemoryItem;
    PhReferenceObject(MemoryItem);

    memset(memoryNode->TextCache, 0, sizeof(PH_STRINGREF) * PHMMTLC_MAXIMUM);
    memoryNode->Node.TextCache = memoryNode->TextCache;
    memoryNode->Node.TextCacheSize = PHMMTLC_MAXIMUM;

    PhAddItemList(Context->RegionNodeList, memoryNode);

    if (Context->TreeFilterSupport.FilterList)
        memoryNode->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->TreeFilterSupport, &memoryNode->Node);

    PhEmCallObjectOperation(EmMemoryNodeType, memoryNode, EmObjectCreate);

    return memoryNode;
}

VOID PhpCopyMemoryRegionTypeInfo(
    _In_ PPH_MEMORY_ITEM Source,
    _Inout_ PPH_MEMORY_ITEM Destination
    )
{
    if (Destination->RegionType == CustomRegion)
        PhClearReference(&Destination->u.Custom.Text);
    else if (Destination->RegionType == MappedFileRegion)
        PhClearReference(&Destination->u.MappedFile.FileName);

    Destination->RegionType = Source->RegionType;
    Destination->u = Source->u;

    if (Destination->RegionType == CustomRegion)
        PhReferenceObject(Destination->u.Custom.Text);
    else if (Destination->RegionType == MappedFileRegion)
        PhReferenceObject(Destination->u.MappedFile.FileName);
}

VOID PhReplaceMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_opt_ PPH_MEMORY_ITEM_LIST List
    )
{
    PLIST_ENTRY listEntry;
    PPH_MEMORY_NODE allocationBaseNode = NULL;

    PhpClearMemoryList(Context);

    if (!List)
    {
        TreeNew_NodesStructured(Context->TreeNewHandle);
        return;
    }

    for (listEntry = List->ListHead.Flink; listEntry != &List->ListHead; listEntry = listEntry->Flink)
    {
        PPH_MEMORY_ITEM memoryItem = CONTAINING_RECORD(listEntry, PH_MEMORY_ITEM, ListEntry);
        PPH_MEMORY_NODE memoryNode;

        if (memoryItem->AllocationBaseItem == memoryItem)
            allocationBaseNode = PhpAddAllocationBaseNode(Context, memoryItem->AllocationBase);

        memoryNode = PhpAddRegionNode(Context, memoryItem);

        if (Context->HideFreeRegions && (memoryItem->State & MEM_FREE))
            memoryNode->Node.Visible = FALSE;
        if (Context->HideGuardRegions && (memoryItem->Protect & PAGE_GUARD))
            memoryNode->Node.Visible = FALSE;

        if (allocationBaseNode && memoryItem->AllocationBase == allocationBaseNode->MemoryItem->BaseAddress)
        {
            if (!(memoryItem->State & MEM_FREE))
            {
                memoryNode->Parent = allocationBaseNode;
                PhAddItemList(allocationBaseNode->Children, memoryNode);
            }

            // Aggregate various statistics.
            allocationBaseNode->MemoryItem->RegionSize += memoryItem->RegionSize;
            allocationBaseNode->MemoryItem->CommittedSize += memoryItem->CommittedSize;
            allocationBaseNode->MemoryItem->PrivateSize += memoryItem->PrivateSize;
            allocationBaseNode->MemoryItem->TotalWorkingSetPages += memoryItem->TotalWorkingSetPages;
            allocationBaseNode->MemoryItem->PrivateWorkingSetPages += memoryItem->PrivateWorkingSetPages;
            allocationBaseNode->MemoryItem->SharedWorkingSetPages += memoryItem->SharedWorkingSetPages;
            allocationBaseNode->MemoryItem->ShareableWorkingSetPages += memoryItem->ShareableWorkingSetPages;
            allocationBaseNode->MemoryItem->LockedWorkingSetPages += memoryItem->LockedWorkingSetPages;

            if (memoryItem->AllocationBaseItem == memoryItem)
            {
                allocationBaseNode->MemoryItem->Protect = memoryItem->AllocationProtect;
                allocationBaseNode->MemoryItem->State = memoryItem->State;
                allocationBaseNode->MemoryItem->Type = memoryItem->Type;

                PhGetMemoryProtectionString(allocationBaseNode->MemoryItem->Protect, allocationBaseNode->ProtectionText);

                if (memoryItem->RegionType != CustomRegion || memoryItem->u.Custom.PropertyOfAllocationBase)
                    PhpCopyMemoryRegionTypeInfo(memoryItem, allocationBaseNode->MemoryItem);

                if (Context->HideFreeRegions && (allocationBaseNode->MemoryItem->State & MEM_FREE))
                    allocationBaseNode->Node.Visible = FALSE;
                if (Context->HideGuardRegions && (allocationBaseNode->MemoryItem->State & PAGE_GUARD))
                    memoryNode->Node.Visible = FALSE;
            }
            else
            {
                if (memoryItem->RegionType == UnknownRegion)
                    PhpCopyMemoryRegionTypeInfo(allocationBaseNode->MemoryItem, memoryItem);
            }
        }

        PhGetMemoryProtectionString(memoryItem->Protect, memoryNode->ProtectionText);
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhRemoveMemoryNode(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ PPH_MEMORY_ITEM_LIST List,
    _In_ PPH_MEMORY_NODE MemoryNode
    )
{
    ULONG index;

    // Remove from list and cleanup.

    PhRemoveElementAvlTree(&List->Set, &MemoryNode->MemoryItem->Links);
    RemoveEntryList(&MemoryNode->MemoryItem->ListEntry);

    if ((index = PhFindItemList(Context->RegionNodeList, MemoryNode)) != -1)
        PhRemoveItemList(Context->RegionNodeList, index);

    if (MemoryNode->MemoryItem->AllocationBaseItem == MemoryNode->MemoryItem)
    {
        if ((index = PhFindItemList(Context->AllocationBaseNodeList, MemoryNode->Parent)) != -1)
            PhRemoveItemList(Context->AllocationBaseNodeList, index);
    }

    PhpDestroyMemoryNode(MemoryNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhUpdateMemoryNode(
    _In_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ PPH_MEMORY_NODE MemoryNode
    )
{
    PhGetMemoryProtectionString(MemoryNode->MemoryItem->Protect, MemoryNode->ProtectionText);
    memset(MemoryNode->TextCache, 0, sizeof(PH_STRINGREF) * PHMMTLC_MAXIMUM);
    TreeNew_InvalidateNode(Context->TreeNewHandle, &MemoryNode->Node);
}

VOID PhExpandAllMemoryNodes(
    _In_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ BOOLEAN Expand
    )
{
    ULONG i;
    BOOLEAN needsRestructure = FALSE;

    for (i = 0; i < Context->AllocationBaseNodeList->Count; i++)
    {
        PPH_MEMORY_NODE node = Context->AllocationBaseNodeList->Items[i];

        if (node->Node.Expanded != Expand)
        {
            node->Node.Expanded = Expand;
            needsRestructure = TRUE;
        }
    }

    for (i = 0; i < Context->RegionNodeList->Count; i++)
    {
        PPH_MEMORY_NODE node = Context->RegionNodeList->Items[i];

        if (node->Node.Expanded != Expand)
        {
            node->Node.Expanded = Expand;
            needsRestructure = TRUE;
        }
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);
}

PPH_STRING PhGetMemoryRegionUseText(
    _In_ PPH_MEMORY_ITEM MemoryItem
    )
{
    PH_MEMORY_REGION_TYPE type = MemoryItem->RegionType;

    switch (type)
    {
    case UnknownRegion:
        return PhReferenceEmptyString();
    case CustomRegion:
        PhReferenceObject(MemoryItem->u.Custom.Text);
        return MemoryItem->u.Custom.Text;
    case UnusableRegion:
        return PhReferenceEmptyString();
    case MappedFileRegion:
        PhReferenceObject(MemoryItem->u.MappedFile.FileName);
        return MemoryItem->u.MappedFile.FileName;
    case UserSharedDataRegion:
        return PhCreateString(L"USER_SHARED_DATA");
    case HypervisorSharedDataRegion:
        return PhCreateString(L"HYPERVISOR_SHARED_DATA");
    case PebRegion:
    case Peb32Region:
        return PhFormatString(L"PEB%s", type == Peb32Region ? L" 32-bit" : L"");
    case TebRegion:
    case Teb32Region:
        return PhFormatString(L"TEB%s (thread %lu)",
            type == Teb32Region ? L" 32-bit" : L"", HandleToUlong(MemoryItem->u.Teb.ThreadId));
    case StackRegion:
    case Stack32Region:
        return PhFormatString(L"Stack%s (thread %lu)",
            type == Stack32Region ? L" 32-bit" : L"", HandleToUlong(MemoryItem->u.Stack.ThreadId));
    case HeapRegion:
    case Heap32Region:
        return PhFormatString(L"Heap%s (ID %lu)",
            type == Heap32Region ? L" 32-bit" : L"", (ULONG)MemoryItem->u.Heap.Index + 1);
    case HeapSegmentRegion:
    case HeapSegment32Region:
        return PhFormatString(L"Heap segment%s (ID %lu)",
            type == HeapSegment32Region ? L" 32-bit" : L"", (ULONG)MemoryItem->u.HeapSegment.HeapItem->u.Heap.Index + 1);
    case CfgBitmapRegion:
    case CfgBitmap32Region:
        return PhFormatString(L"CFG Bitmap%s",
            type == CfgBitmap32Region ? L" 32-bit" : L"");
    case ApiSetMapRegion:
        return PhFormatString(L"ApiSetMap");
    case ReadOnlySharedMemoryRegion:
        return PhFormatString(L"CSR shared memory");
    case CodePageDataRegion:
        return PhFormatString(L"CodePage data");
    case GdiSharedHandleTableRegion:
        return PhFormatString(L"GDI shared handle table");
    case ShimDataRegion:
        return PhFormatString(L"Shim data");
    case ActivationContextDataRegion:
        return PhFormatString(L"Activation context data");
    case SystemDefaultActivationContextDataRegion:
        return PhFormatString(L"Default activation context data");
    default:
        return PhReferenceEmptyString();
    }
}

VOID PhpUpdateMemoryNodeUseText(
    _Inout_ PPH_MEMORY_NODE MemoryNode
    )
{
    if (!MemoryNode->UseText)
        MemoryNode->UseText = PhGetMemoryRegionUseText(MemoryNode->MemoryItem);
}

PPH_STRING PhpFormatSizeIfNonZero(
    _In_ ULONG64 Size
    )
{
    if (Size != 0)
        return PhFormatSize(Size, -1);
    else
        return NULL;
}

#define SORT_FUNCTION(Column) PhpMemoryTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpMemoryTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_MEMORY_NODE node1 = *(PPH_MEMORY_NODE *)_elem1; \
    PPH_MEMORY_NODE node2 = *(PPH_MEMORY_NODE *)_elem2; \
    PPH_MEMORY_ITEM memoryItem1 = node1->MemoryItem; \
    PPH_MEMORY_ITEM memoryItem2 = node2->MemoryItem; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)memoryItem1->BaseAddress, (ULONG_PTR)memoryItem2->BaseAddress); \
    \
    return PhModifySort(sortResult, ((PPH_MEMORY_LIST_CONTEXT)_context)->TreeNewSortOrder); \
}

LONG PhpMemoryTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPH_MEMORY_NODE)Node1)->MemoryItem->BaseAddress, (ULONG_PTR)((PPH_MEMORY_NODE)Node2)->MemoryItem->BaseAddress);

    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(BaseAddress)
{
    sortResult = uintptrcmp((ULONG_PTR)memoryItem1->BaseAddress, (ULONG_PTR)memoryItem2->BaseAddress);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = uintcmp(memoryItem1->Type | memoryItem1->State, memoryItem2->Type | memoryItem2->State);

    if (sortResult == 0)
        sortResult = intcmp(memoryItem1->RegionType, memoryItem2->RegionType);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Size)
{
    sortResult = uintptrcmp(memoryItem1->RegionSize, memoryItem2->RegionSize);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Protection)
{
    sortResult = PhCompareStringZ(node1->ProtectionText, node2->ProtectionText, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Use)
{
    PhpUpdateMemoryNodeUseText(node1);
    PhpUpdateMemoryNodeUseText(node2);
    sortResult = PhCompareStringWithNull(node1->UseText, node2->UseText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(TotalWs)
{
    sortResult = uintptrcmp(memoryItem1->TotalWorkingSetPages, memoryItem2->TotalWorkingSetPages);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PrivateWs)
{
    sortResult = uintptrcmp(memoryItem1->PrivateWorkingSetPages, memoryItem2->PrivateWorkingSetPages);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ShareableWs)
{
    sortResult = uintptrcmp(memoryItem1->ShareableWorkingSetPages, memoryItem2->ShareableWorkingSetPages);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(SharedWs)
{
    sortResult = uintptrcmp(memoryItem1->SharedWorkingSetPages, memoryItem2->SharedWorkingSetPages);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LockedWs)
{
    sortResult = uintptrcmp(memoryItem1->LockedWorkingSetPages, memoryItem2->LockedWorkingSetPages);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Committed)
{
    sortResult = uintptrcmp(memoryItem1->CommittedSize, memoryItem2->CommittedSize);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Private)
{
    sortResult = uintptrcmp(memoryItem1->PrivateSize, memoryItem2->PrivateSize);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpMemoryTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPH_MEMORY_LIST_CONTEXT context = Context;
    PPH_MEMORY_NODE node;

    if (!context)
        return TRUE;
    if (PhCmForwardMessage(hwnd, Message, Parameter1, Parameter2, &context->Cm))
        return TRUE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            node = (PPH_MEMORY_NODE)getChildren->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
            {
                if (!node)
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)context->AllocationBaseNodeList->Items;
                    getChildren->NumberOfChildren = context->AllocationBaseNodeList->Count;
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
                        SORT_FUNCTION(BaseAddress),
                        SORT_FUNCTION(Type),
                        SORT_FUNCTION(Size),
                        SORT_FUNCTION(Protection),
                        SORT_FUNCTION(Use),
                        SORT_FUNCTION(TotalWs),
                        SORT_FUNCTION(PrivateWs),
                        SORT_FUNCTION(ShareableWs),
                        SORT_FUNCTION(SharedWs),
                        SORT_FUNCTION(LockedWs),
                        SORT_FUNCTION(Committed),
                        SORT_FUNCTION(Private)
                    };
                    int (__cdecl *sortFunction)(void *, const void *, const void *);

                    if (!PhCmForwardSort(
                        (PPH_TREENEW_NODE *)context->RegionNodeList->Items,
                        context->RegionNodeList->Count,
                        context->TreeNewSortColumn,
                        context->TreeNewSortOrder,
                        &context->Cm
                        ))
                    {
                        if (context->TreeNewSortColumn < PHMMTLC_MAXIMUM)
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
                        qsort_s(context->RegionNodeList->Items, context->RegionNodeList->Count, sizeof(PVOID), sortFunction, context);
                    }

                    getChildren->Children = (PPH_TREENEW_NODE *)context->RegionNodeList->Items;
                    getChildren->NumberOfChildren = context->RegionNodeList->Count;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            if (!isLeaf)
                break;

            node = (PPH_MEMORY_NODE)isLeaf->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
                isLeaf->IsLeaf = !node->Children || node->Children->Count == 0;
            else
                isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PPH_MEMORY_ITEM memoryItem;

            if (!getCellText)
                break;

            node = (PPH_MEMORY_NODE)getCellText->Node;
            memoryItem = node->MemoryItem;

            switch (getCellText->Id)
            {
            case PHMMTLC_BASEADDRESS:
                PhPrintPointer(node->BaseAddressText, memoryItem->BaseAddress);
                PhInitializeStringRefLongHint(&getCellText->Text, node->BaseAddressText);
                break;
            case PHMMTLC_TYPE:
                if (memoryItem->State & MEM_FREE)
                {
                    if (memoryItem->RegionType == UnusableRegion)
                        PhInitializeStringRef(&getCellText->Text, L"Free (Unusable)");
                    else
                        PhInitializeStringRef(&getCellText->Text, L"Free");
                }
                else if (node->IsAllocationBase)
                {
                    PhInitializeStringRefLongHint(&getCellText->Text, PhGetMemoryTypeString(memoryItem->Type));
                }
                else
                {
                    PH_FORMAT format[3];
                    SIZE_T returnLength;

                    PhInitFormatS(&format[0], PhGetMemoryTypeString(memoryItem->Type));
                    PhInitFormatS(&format[1], L": ");
                    PhInitFormatS(&format[2], PhGetMemoryStateString(memoryItem->State));

                    if (PhFormatToBuffer(format, 3, node->TypeText, sizeof(node->TypeText), &returnLength))
                    {
                        getCellText->Text.Buffer = node->TypeText;
                        getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                    }
                }
                break;
            case PHMMTLC_SIZE:
                PhMoveReference(&node->SizeText, PhFormatSize(memoryItem->RegionSize, -1));
                getCellText->Text = PhGetStringRef(node->SizeText);
                break;
            case PHMMTLC_PROTECTION:
                PhInitializeStringRefLongHint(&getCellText->Text, node->ProtectionText);
                break;
            case PHMMTLC_USE:
                PhpUpdateMemoryNodeUseText(node);
                getCellText->Text = PhGetStringRef(node->UseText);
                break;
            case PHMMTLC_TOTALWS:
                PhMoveReference(&node->TotalWsText, PhpFormatSizeIfNonZero((ULONG64)memoryItem->TotalWorkingSetPages * PAGE_SIZE));
                getCellText->Text = PhGetStringRef(node->TotalWsText);
                break;
            case PHMMTLC_PRIVATEWS:
                PhMoveReference(&node->PrivateWsText, PhpFormatSizeIfNonZero((ULONG64)memoryItem->PrivateWorkingSetPages * PAGE_SIZE));
                getCellText->Text = PhGetStringRef(node->PrivateWsText);
                break;
            case PHMMTLC_SHAREABLEWS:
                PhMoveReference(&node->ShareableWsText, PhpFormatSizeIfNonZero((ULONG64)memoryItem->ShareableWorkingSetPages * PAGE_SIZE));
                getCellText->Text = PhGetStringRef(node->ShareableWsText);
                break;
            case PHMMTLC_SHAREDWS:
                PhMoveReference(&node->SharedWsText, PhpFormatSizeIfNonZero((ULONG64)memoryItem->SharedWorkingSetPages * PAGE_SIZE));
                getCellText->Text = PhGetStringRef(node->SharedWsText);
                break;
            case PHMMTLC_LOCKEDWS:
                PhMoveReference(&node->LockedWsText, PhpFormatSizeIfNonZero((ULONG64)memoryItem->LockedWorkingSetPages * PAGE_SIZE));
                getCellText->Text = PhGetStringRef(node->LockedWsText);
                break;
            case PHMMTLC_COMMITTED:
                PhMoveReference(&node->CommittedText, PhpFormatSizeIfNonZero(memoryItem->CommittedSize));
                getCellText->Text = PhGetStringRef(node->CommittedText);
                break;
            case PHMMTLC_PRIVATE:
                PhMoveReference(&node->PrivateText, PhpFormatSizeIfNonZero(memoryItem->PrivateSize));
                getCellText->Text = PhGetStringRef(node->PrivateText);
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
            PPH_MEMORY_ITEM memoryItem;

            if (!getNodeColor)
                break;

            node = (PPH_MEMORY_NODE)getNodeColor->Node;
            memoryItem = node->MemoryItem;

            if (!memoryItem)
                NOTHING; 
            else if (
                context->HighlightExecutePages && (
                memoryItem->Protect & PAGE_EXECUTE || 
                memoryItem->Protect & PAGE_EXECUTE_READ || 
                memoryItem->Protect & PAGE_EXECUTE_READWRITE || 
                memoryItem->Protect & PAGE_EXECUTE_WRITECOPY
                ))
            {
                getNodeColor->BackColor = PhCsColorPacked;
            }
            else if (
                context->HighlightCfgPages && (
                memoryItem->RegionType == CfgBitmapRegion ||
                memoryItem->RegionType == CfgBitmap32Region
                ))
            {
                getNodeColor->BackColor = PhCsColorElevatedProcesses;
            }
            else if (
                context->HighlightSystemPages && (
                memoryItem->Type & SEC_IMAGE
                ))
            {
                getNodeColor->BackColor = PhCsColorSystemProcesses;
            }
            else if (context->HighlightPrivatePages && memoryItem->Type & MEM_PRIVATE)
            {
                getNodeColor->BackColor = PhCsColorOwnProcesses;
            }
            //else if (
            //    memoryItem->RegionType == StackRegion || memoryItem->RegionType == Stack32Region ||
            //    memoryItem->RegionType == HeapRegion || memoryItem->RegionType == Heap32Region ||
            //    memoryItem->RegionType == HeapSegmentRegion || memoryItem->RegionType == HeapSegment32Region
            //    ((memoryItem->Protect & PAGE_EXECUTE_WRITECOPY || memoryItem->Protect & PAGE_EXECUTE_READWRITE || 
            //    memoryItem->Protect & PAGE_READWRITE) && !(memoryItem->Type & SEC_IMAGE))
            //    )
            //{
            //    getNodeColor->BackColor = PhCsColorElevatedProcesses;
            //}
            //else if (memoryItem->Type & SEC_IMAGE)
            //    getNodeColor->BackColor = PhCsColorImmersiveProcesses;

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
                    SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_MEMORY_COPY, 0);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(context->TreeNewHandle, 0, -1);
                break;
            case VK_RETURN:
                SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_MEMORY_READWRITEMEMORY, 0);
                break;
            case VK_F5:
                SendMessage(context->ParentWindowHandle, WM_COMMAND, IDC_REFRESH, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = NoSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

            if (!mouseEvent)
                break;

            node = (PPH_MEMORY_NODE)mouseEvent->Node;

            if (node && node->IsAllocationBase)
                TreeNew_SetNodeExpanded(hwnd, node, !node->Node.Expanded);
            else
                SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_MEMORY_READWRITEMEMORY, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenu = Parameter1;

            if (!contextMenu)
                break;

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

PPH_MEMORY_NODE PhGetSelectedMemoryNode(
    _In_ PPH_MEMORY_LIST_CONTEXT Context
    )
{
    ULONG i;

    if (Context->TreeNewSortOrder == NoSortOrder)
    {
        for (i = 0; i < Context->AllocationBaseNodeList->Count; i++)
        {
            PPH_MEMORY_NODE node = Context->AllocationBaseNodeList->Items[i];

            if (node->Node.Selected)
                return node;
        }
    }

    for (i = 0; i < Context->RegionNodeList->Count; i++)
    {
        PPH_MEMORY_NODE node = Context->RegionNodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

VOID PhGetSelectedMemoryNodes(
    _In_ PPH_MEMORY_LIST_CONTEXT Context,
    _Out_ PPH_MEMORY_NODE **MemoryNodes,
    _Out_ PULONG NumberOfMemoryNodes
    )
{
    PH_ARRAY array;
    ULONG i;

    PhInitializeArray(&array, sizeof(PVOID), 2);

    if (Context->TreeNewSortOrder == NoSortOrder)
    {
        for (i = 0; i < Context->AllocationBaseNodeList->Count; i++)
        {
            PPH_MEMORY_NODE node = Context->AllocationBaseNodeList->Items[i];

            if (node->Node.Selected)
                PhAddItemArray(&array, &node);
        }
    }

    for (i = 0; i < Context->RegionNodeList->Count; i++)
    {
        PPH_MEMORY_NODE node = Context->RegionNodeList->Items[i];

        if (node->Node.Selected)
            PhAddItemArray(&array, &node);
    }

    *NumberOfMemoryNodes = (ULONG)array.Count;
    *MemoryNodes = PhFinalArrayItems(&array);
}

VOID PhDeselectAllMemoryNodes(
    _In_ PPH_MEMORY_LIST_CONTEXT Context
    )
{
    TreeNew_DeselectRange(Context->TreeNewHandle, 0, -1);
}
