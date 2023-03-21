/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2022
 *
 */

#include <peview.h>
#include <cryptuiapi.h>
#include "colmgr.h"

#define WM_PV_LAYOUT_CONTEXTMENU (WM_APP + 801)

typedef struct _PV_PE_LAYOUT_CONTEXT
{
    HWND WindowHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    HWND ParentWindowHandle;
    ULONG TreeNewSortColumn;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
    PPH_STRING SearchboxText;
    PPH_STRING StatusMessage;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_PE_LAYOUT_CONTEXT, *PPV_PE_LAYOUT_CONTEXT;

typedef enum _PV_LAYOUT_TREE_COLUMN_NAME
{
    PV_LAYOUT_TREE_COLUMN_NAME_NAME,
    PV_LAYOUT_TREE_COLUMN_NAME_VALUE,
    PV_LAYOUT_TREE_COLUMN_NAME_MAXIMUM
} PV_LAYOUT_TREE_COLUMN_NAME;

typedef struct _PV_LAYOUT_NODE
{
    PH_TREENEW_NODE Node;

    ULONG64 UniqueId;
    PPH_STRING Name;
    PPH_STRING Value;

    struct _PV_LAYOUT_NODE* Parent;
    PPH_LIST Children;

    PH_STRINGREF TextCache[PV_LAYOUT_TREE_COLUMN_NAME_MAXIMUM];
} PV_LAYOUT_NODE, *PPV_LAYOUT_NODE;

BOOLEAN PvLayoutNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );
ULONG PvLayoutNodeHashtableHashFunction(
    _In_ PVOID Entry
    );
VOID PvDestroyLayoutNode(
    _In_ PPV_LAYOUT_NODE CertificateNode
    );
BOOLEAN NTAPI PvLayoutTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

VOID PvInitializeLayoutTree(
    _In_ PPV_PE_LAYOUT_CONTEXT Context
    )
{
    PPH_STRING settings;

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PV_LAYOUT_NODE),
        PvLayoutNodeHashtableEqualFunction,
        PvLayoutNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(10);
    Context->NodeRootList = PhCreateList(10);

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetCallback(Context->TreeNewHandle, PvLayoutTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, PV_LAYOUT_TREE_COLUMN_NAME_NAME, TRUE, L"Name", 200, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_LAYOUT_TREE_COLUMN_NAME_VALUE, TRUE, L"Value", 800, PH_ALIGN_LEFT, 1, 0);

    //TreeNew_SetTriState(Context->TreeNewHandle, TRUE);
    //TreeNew_SetSort(Context->TreeNewHandle, PV_LAYOUT_TREE_COLUMN_NAME_NAME, NoSortOrder);
    //TreeNew_SetRowHeight(Context->TreeNewHandle, 22);

    settings = PhGetStringSetting(L"ImageLayoutTreeColumns");
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);
}

VOID PvDeleteLayoutTree(
    _In_ PPV_PE_LAYOUT_CONTEXT Context
    )
{
    PPH_STRING settings;
    ULONG i;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(L"ImageLayoutTreeColumns", &settings->sr);
    PhDereferenceObject(settings);

    for (i = 0; i < Context->NodeList->Count; i++)
        PvDestroyLayoutNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
    PhDereferenceObject(Context->NodeRootList);
}

BOOLEAN PvLayoutNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPV_LAYOUT_NODE layoutNode1 = *(PPV_LAYOUT_NODE*)Entry1;
    PPV_LAYOUT_NODE layoutNode2 = *(PPV_LAYOUT_NODE*)Entry2;

    return layoutNode1->UniqueId == layoutNode2->UniqueId;
}

ULONG PvLayoutNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashInt64((ULONG_PTR)(*(PPV_LAYOUT_NODE*)Entry)->UniqueId);
}

PPV_LAYOUT_NODE PvAddLayoutNode(
    _Inout_ PPV_PE_LAYOUT_CONTEXT Context,
    _In_ PWSTR Name,
    _In_opt_ PPH_STRING Value
    )
{
    static ULONG64 index = 0;
    PPV_LAYOUT_NODE layoutNode;

    layoutNode = PhAllocateZero(sizeof(PV_LAYOUT_NODE));
    layoutNode->UniqueId = ++index;
    PhInitializeTreeNewNode(&layoutNode->Node);

    memset(layoutNode->TextCache, 0, sizeof(PH_STRINGREF) * PV_LAYOUT_TREE_COLUMN_NAME_MAXIMUM);
    layoutNode->Node.TextCache = layoutNode->TextCache;
    layoutNode->Node.TextCacheSize = PV_LAYOUT_TREE_COLUMN_NAME_MAXIMUM;

    layoutNode->Name = PhCreateString(Name);
    layoutNode->Value = Value;
    layoutNode->Children = PhCreateList(1);

    PhAddEntryHashtable(Context->NodeHashtable, &layoutNode);
    PhAddItemList(Context->NodeList, layoutNode);

    //if (Context->FilterSupport.FilterList)
    //   layoutNode->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &layoutNode->Node);

    return layoutNode;
}

PPV_LAYOUT_NODE PvAddChildLayoutNode(
    _In_ PPV_PE_LAYOUT_CONTEXT Context,
    _In_opt_ PPV_LAYOUT_NODE ParentNode,
    _In_ PWSTR Name,
    _In_opt_ PPH_STRING Value
    )
{
    PPV_LAYOUT_NODE childNode;

    childNode = PvAddLayoutNode(Context, Name, Value);

    if (ParentNode)
    {
        // This is a child node.
        childNode->Node.Expanded = TRUE;
        childNode->Parent = ParentNode;

        PhAddItemList(ParentNode->Children, childNode);
    }
    else
    {
        // This is a root node.
        childNode->Node.Expanded = TRUE;
        PhAddItemList(Context->NodeRootList, childNode);
    }

    return childNode;
}

PPV_LAYOUT_NODE PvFindLayoutNode(
    _In_ PPV_PE_LAYOUT_CONTEXT Context,
    _In_ ULONG UniqueId
    )
{
    PV_LAYOUT_NODE lookupLayoutNode;
    PPV_LAYOUT_NODE lookupLayoutNodePtr = &lookupLayoutNode;
    PPV_LAYOUT_NODE* layoutNode;

    lookupLayoutNode.Node.Index = UniqueId;

    layoutNode = (PPV_LAYOUT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupLayoutNodePtr
        );

    if (layoutNode)
        return *layoutNode;
    else
        return NULL;
}

VOID PvRemoveLayoutNode(
    _In_ PPV_PE_LAYOUT_CONTEXT Context,
    _In_ PPV_LAYOUT_NODE LayoutNode
    )
{
    ULONG index;

    // Remove from hashtable/list and cleanup.

    PhRemoveEntryHashtable(Context->NodeHashtable, &LayoutNode);

    if ((index = PhFindItemList(Context->NodeList, LayoutNode)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);

    PvDestroyLayoutNode(LayoutNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PvDestroyLayoutNode(
    _In_ PPV_LAYOUT_NODE LayoutNode
    )
{
    PhDereferenceObject(LayoutNode->Children);

    if (LayoutNode->Name) PhDereferenceObject(LayoutNode->Name);
    if (LayoutNode->Value) PhDereferenceObject(LayoutNode->Value);

    PhFree(LayoutNode);
}

#define SORT_FUNCTION(Column) PvLayoutTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvLayoutTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPV_LAYOUT_NODE node1 = *(PPV_LAYOUT_NODE *)_elem1; \
    PPV_LAYOUT_NODE node2 = *(PPV_LAYOUT_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    return PhModifySort(sortResult, ((PPV_PE_LAYOUT_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareString(node1->Name, node2->Name, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Index)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PvLayoutTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPV_PE_LAYOUT_CONTEXT context = Context;
    PPV_LAYOUT_NODE node;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            node = (PPV_LAYOUT_NODE)getChildren->Node;

            if (!node)
            {
                getChildren->Children = (PPH_TREENEW_NODE*)context->NodeRootList->Items;
                getChildren->NumberOfChildren = context->NodeRootList->Count;
            }
            else
            {
                getChildren->Children = (PPH_TREENEW_NODE*)node->Children->Items;
                getChildren->NumberOfChildren = node->Children->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            if (!isLeaf)
                break;

            node = (PPV_LAYOUT_NODE)isLeaf->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
                isLeaf->IsLeaf = !(node->Children && node->Children->Count);
            else
                isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;

            if (!getCellText)
                break;

            node = (PPV_LAYOUT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PV_LAYOUT_TREE_COLUMN_NAME_NAME:
                {
                    getCellText->Text = PhGetStringRef(node->Name);
                }
                break;
            case PV_LAYOUT_TREE_COLUMN_NAME_VALUE:
                {
                    getCellText->Text = PhGetStringRef(node->Value);
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

            if (!getNodeColor)
                break;

            node = (PPV_LAYOUT_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_AUTO_FORECOLOR | TN_CACHE;
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
                //if (GetKeyState(VK_CONTROL) < 0)
                    //SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_WINDOW_COPY, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            //SendMessage(context->WindowHandle, WM_COMMAND, PROPERTIES, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(context->WindowHandle, WM_COMMAND, WM_PV_LAYOUT_CONTEXTMENU, (LPARAM)contextMenuEvent);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            //PH_TN_COLUMN_MENU_DATA data;

            //data.TreeNewHandle = hwnd;
            //data.MouseEvent = Parameter1;
            //data.DefaultSortColumn = 0;
            //data.DefaultSortOrder = AscendingSortOrder;
            //PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            //data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
            //    PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            //PhHandleTreeNewColumnMenu(&data);
            //PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

VOID PvClearLayoutTree(
    _In_ PPV_PE_LAYOUT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PvDestroyLayoutNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
    PhClearList(Context->NodeRootList);
}

PPV_LAYOUT_NODE PvGetSelectedLayoutNode(
    _In_ PPV_PE_LAYOUT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_LAYOUT_NODE layoutNode = Context->NodeList->Items[i];

        if (layoutNode->Node.Selected)
            return layoutNode;
    }

    return NULL;
}

_Success_(return)
BOOLEAN PvGetSelectedLayoutNodes(
    _In_ PPV_PE_LAYOUT_CONTEXT Context,
    _Out_ PPV_LAYOUT_NODE** Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_LAYOUT_NODE node = Context->NodeList->Items[i];

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

VOID PvExpandAllLayoutNodes(
    _In_ PPV_PE_LAYOUT_CONTEXT Context,
    _In_ BOOLEAN Expand
    )
{
    ULONG i;
    BOOLEAN needsRestructure = FALSE;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_LAYOUT_NODE node = Context->NodeList->Items[i];

        if (node->Children->Count != 0 && node->Node.Expanded != Expand)
        {
            node->Node.Expanded = Expand;
            needsRestructure = TRUE;
        }
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PvDeselectAllLayoutNodes(
    _In_ PPV_PE_LAYOUT_CONTEXT Context
    )
{
    TreeNew_DeselectRange(Context->TreeNewHandle, 0, -1);
}

VOID PvSelectAndEnsureVisibleLayoutNodes(
    _In_ PPV_PE_LAYOUT_CONTEXT Context,
    _In_ PPV_LAYOUT_NODE* Nodes,
    _In_ ULONG NumberOfNodes
    )
{
    ULONG i;
    PPV_LAYOUT_NODE leader = NULL;
    PPV_LAYOUT_NODE node;
    BOOLEAN needsRestructure = FALSE;

    PvDeselectAllLayoutNodes(Context);

    for (i = 0; i < NumberOfNodes; i++)
    {
        if (Nodes[i]->Node.Visible)
        {
            leader = Nodes[i];
            break;
        }
    }

    if (!leader)
        return;

    // Expand recursively upwards, and select the nodes.

    for (i = 0; i < NumberOfNodes; i++)
    {
        if (!Nodes[i]->Node.Visible)
            continue;

        node = Nodes[i]->Parent;

        while (node)
        {
            if (!node->Node.Expanded)
                needsRestructure = TRUE;

            node->Node.Expanded = TRUE;
            node = node->Parent;
        }

        Nodes[i]->Node.Selected = TRUE;
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);

    TreeNew_SetFocusNode(Context->TreeNewHandle, &leader->Node);
    TreeNew_SetMarkNode(Context->TreeNewHandle, &leader->Node);
    TreeNew_EnsureVisible(Context->TreeNewHandle, &leader->Node);
    TreeNew_InvalidateNode(Context->TreeNewHandle, &leader->Node);
}

PPH_STRING PvLayoutPeFormatDateTime(
    _In_ PSYSTEMTIME SystemTime
    )
{
    return PhFormatString(
        L"%s, %s\n",
        PH_AUTO_T(PH_STRING, PhFormatDate(SystemTime, L"dddd, MMMM d, yyyy"))->Buffer,
        PH_AUTO_T(PH_STRING, PhFormatTime(SystemTime, L"hh:mm:ss tt"))->Buffer
        );
}

PPH_STRING PvLayoutGetRelativeTimeString(
    _In_ PLARGE_INTEGER Time
    )
{
    LARGE_INTEGER time;
    LARGE_INTEGER currentTime;
    SYSTEMTIME timeFields;
    PPH_STRING timeRelativeString;
    PPH_STRING timeString;

    time = *Time;
    PhQuerySystemTime(&currentTime);
    timeRelativeString = PH_AUTO(PhFormatTimeSpanRelative(currentTime.QuadPart - time.QuadPart));

    PhLargeIntegerToLocalSystemTime(&timeFields, &time);
    timeString = PH_AUTO(PvLayoutPeFormatDateTime(&timeFields));

    return PhFormatString(L"%s (%s ago)", timeString->Buffer, timeRelativeString->Buffer);
}

PWSTR PvLayoutNameFlagsToString(
    _In_ ULONG Flags
    )
{
    if (Flags == 0)
    {
        return L"HLINK Name";
    }

    if (Flags & FILE_LAYOUT_NAME_ENTRY_PRIMARY)
    {
        return L"NTFS Name";
    }

    if (Flags & FILE_LAYOUT_NAME_ENTRY_DOS)
    {
        return L"DOS Name";
    }

    return L"UNKNOWN";
}

PPH_STRING PvLayoutSteamFlagsToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (Flags & STREAM_LAYOUT_ENTRY_IMMOVABLE)
        PhAppendStringBuilder2(&stringBuilder, L"Immovable, ");
    if (Flags & STREAM_LAYOUT_ENTRY_PINNED)
        PhAppendStringBuilder2(&stringBuilder, L"Pinned, ");
    if (Flags & STREAM_LAYOUT_ENTRY_RESIDENT)
        PhAppendStringBuilder2(&stringBuilder, L"Resident, ");
    if (Flags & STREAM_LAYOUT_ENTRY_NO_CLUSTERS_ALLOCATED)
        PhAppendStringBuilder2(&stringBuilder, L"No clusters allocated, ");
    if (Flags & STREAM_LAYOUT_ENTRY_HAS_INFORMATION)
        PhAppendStringBuilder2(&stringBuilder, L"Has parsed information, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Flags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PvLayoutGetParentIdName(
    _In_ HANDLE FileHandle,
    _In_ LONGLONG ParentFileId)
{
    PPH_STRING parentName = NULL;
    NTSTATUS status;
    HANDLE linkHandle;
    PH_FILE_ID_DESCRIPTOR fileId;

    memset(&fileId, 0, sizeof(PH_FILE_ID_DESCRIPTOR));
    fileId.Type = FileIdType;
    fileId.FileId.QuadPart = ParentFileId;

    status = PhOpenFileById(
        &linkHandle,
        FileHandle,
        &fileId,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        status = PhGetFileHandleName(linkHandle, &parentName);
        NtClose(linkHandle);
    }

    return parentName;
}

VOID PvLayoutSetStatusMessage(
    _Inout_ PPV_PE_LAYOUT_CONTEXT Context,
    _In_ ULONG Status
    )
{
    PPH_STRING statusMessage;

    statusMessage = PhGetStatusMessage(Status, 0);
    PhMoveReference(&Context->StatusMessage, PhConcatStrings2(
        L"Unable to query file layout information:\n",
        PhGetStringOrDefault(statusMessage, L"Unknown error.")
        ));
    TreeNew_SetEmptyText(Context->TreeNewHandle, &Context->StatusMessage->sr, 0);
    PhClearReference(&statusMessage);
}

BOOLEAN PvLayoutWordMatchStringRef(
    _In_ PPV_PE_LAYOUT_CONTEXT Context,
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = PhGetStringRef(Context->SearchboxText);

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, L'|', &part, &remainingPart);

        if (part.Length)
        {
            if (PhFindStringInStringRef(Text, &part, TRUE) != SIZE_MAX)
                return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN PvLayoutWordMatchStringZ(
    _In_ PPV_PE_LAYOUT_CONTEXT Context,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);
    return PvLayoutWordMatchStringRef(Context, &text);
}

BOOLEAN PvLayoutTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_ PVOID Context
    )
{
    PPV_PE_LAYOUT_CONTEXT context = Context;
    PPV_LAYOUT_NODE node = (PPV_LAYOUT_NODE)Node;

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    if (!PhIsNullOrEmptyString(node->Name))
    {
        if (PvLayoutWordMatchStringRef(context, &node->Name->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->Value))
    {
        if (PvLayoutWordMatchStringRef(context, &node->Value->sr))
            return TRUE;
    }

    return FALSE;
}

PPH_STRING PvLayoutFormatSize(
    _In_ ULONG64 Size
    )
{
    PH_FORMAT format[5];

    PhInitFormatI64U(&format[0], Size);
    PhInitFormatS(&format[1], L" (");
    PhInitFormatSize(&format[2], Size);
    PhInitFormatC(&format[3], L')');

    return PhFormat(format, 4, 16);
}

#define FILE_LAYOUT_ENTRY_VERSION 0x1
#define STREAM_LAYOUT_ENTRY_VERSION 0x1
#define PH_FIRST_LAYOUT_ENTRY(LayoutEntry) \
    ((PFILE_LAYOUT_ENTRY)(PTR_ADD_OFFSET(LayoutEntry, \
    ((PQUERY_FILE_LAYOUT_OUTPUT)LayoutEntry)->FirstFileOffset)))
#define PH_NEXT_LAYOUT_ENTRY(LayoutEntry) ( \
    ((PFILE_LAYOUT_ENTRY)(LayoutEntry))->NextFileOffset ? \
    (PFILE_LAYOUT_ENTRY)(PTR_ADD_OFFSET((LayoutEntry), \
    ((PFILE_LAYOUT_ENTRY)(LayoutEntry))->NextFileOffset)) : \
    NULL \
    )

#define ATTRIBUTE_TYPECODE_STANDARD_INFORMATION 0x10
#define ATTRIBUTE_TYPECODE_ATTRIBUTE_LIST 0x20
#define ATTRIBUTE_TYPECODE_FILE_NAME 0x30
#define ATTRIBUTE_TYPECODE_OBJECT_ID 0x40
#define ATTRIBUTE_TYPECODE_SECURITY_DESCRIPTOR 0x50
#define ATTRIBUTE_TYPECODE_VOLUME_NAME 0x60
#define ATTRIBUTE_TYPECODE_VOLUME_INFORMATION 0x70
#define ATTRIBUTE_TYPECODE_DATA 0x80
#define ATTRIBUTE_TYPECODE_INDEX_ROOT 0x90
#define ATTRIBUTE_TYPECODE_INDEX_ALLOCATION 0xA0
#define ATTRIBUTE_TYPECODE_BITMAP 0xB0
#define ATTRIBUTE_TYPECODE_SYMBOLIC_LINK 0xC0
#define ATTRIBUTE_TYPECODE_EA_INFORMATION 0xD0
#define ATTRIBUTE_TYPECODE_EA 0xE0

typedef enum _FILE_METADATA_OPTIMIZATION_STATE
{
    FileMetadataOptimizationNone = 0,
    FileMetadataOptimizationInProgress,
    FileMetadataOptimizationPending
} FILE_METADATA_OPTIMIZATION_STATE, *PFILE_METADATA_OPTIMIZATION_STATE;

typedef struct _FILE_QUERY_METADATA_OPTIMIZATION_OUTPUT
{
    FILE_METADATA_OPTIMIZATION_STATE State;
    ULONG AttributeListSize;
    ULONG MetadataSpaceUsed;
    ULONG MetadataSpaceAllocated;
    ULONG NumberOfFileRecords;
    ULONG NumberOfResidentAttributes;
    ULONG NumberOfNonresidentAttributes;
    ULONG TotalInProgress;
    ULONG TotalPending;
} FILE_QUERY_METADATA_OPTIMIZATION_OUTPUT, *PFILE_QUERY_METADATA_OPTIMIZATION_OUTPUT;

NTSTATUS PvLayoutGetMetadataOptimization(
    _Out_ PFILE_QUERY_METADATA_OPTIMIZATION_OUTPUT FileMetadataOptimization
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhDeviceIoControlFile(
        fileHandle,
        FSCTL_QUERY_FILE_METADATA_OPTIMIZATION,
        NULL,
        0,
        FileMetadataOptimization,
        sizeof(FILE_QUERY_METADATA_OPTIMIZATION_OUTPUT),
        NULL
        );

    NtClose(fileHandle);

    return status;
}

NTSTATUS PvLayoutSetMetadataOptimization(
    VOID
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    HANDLE tokenHandle;

    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        status = PhDeviceIoControlFile(
            fileHandle,
            FSCTL_INITIATE_FILE_METADATA_OPTIMIZATION,
            NULL,
            0,
            NULL,
            0,
            NULL
            );

        NtClose(fileHandle);
    }

    if (status == STATUS_SUCCESS)
        return status;

    status = PhOpenProcessToken(NtCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &tokenHandle);

    if (!NT_SUCCESS(status))
        return status;

    if (!PhSetTokenPrivilege2(tokenHandle, SE_MANAGE_VOLUME_PRIVILEGE, SE_PRIVILEGE_ENABLED))
    {
        NtClose(tokenHandle);
        return STATUS_UNSUCCESSFUL;
    }

    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        status = PhDeviceIoControlFile(
            fileHandle,
            FSCTL_INITIATE_FILE_METADATA_OPTIMIZATION,
            NULL,
            0,
            NULL,
            0,
            NULL
            );

        NtClose(fileHandle);
    }

    PhSetTokenPrivilege2(tokenHandle, SE_MANAGE_VOLUME_PRIVILEGE, 0);
    NtClose(tokenHandle);

    return status;
}

NTSTATUS PvGetFileAllocatedRanges(
    _In_ HANDLE FileHandle,
    _In_ PV_FILE_ALLOCATION_CALLBACK Callback,
    _In_ PVOID Context
    )
{
    NTSTATUS status;
    ULONG outputCount;
    ULONG outputLength;
    LARGE_INTEGER fileSize;
    FILE_ALLOCATED_RANGE_BUFFER input;
    PFILE_ALLOCATED_RANGE_BUFFER output;
    ULONG returnLength;

    status = PhGetFileSize(FileHandle, &fileSize);

    if (!NT_SUCCESS(status))
        return status;

    memset(&input, 0, sizeof(FILE_ALLOCATED_RANGE_BUFFER));
    input.FileOffset.QuadPart = 0;
    input.Length.QuadPart = fileSize.QuadPart;

    outputLength = sizeof(FILE_ALLOCATED_RANGE_BUFFER) * PAGE_SIZE;
    output = PhAllocateZero(outputLength);

    while (TRUE)
    {
        status = PhDeviceIoControlFile(
            FileHandle,
            FSCTL_QUERY_ALLOCATED_RANGES,
            &input,
            sizeof(FILE_ALLOCATED_RANGE_BUFFER),
            output,
            outputLength,
            &returnLength
            );

        if (!NT_SUCCESS(status))
            break;

        outputCount = returnLength / sizeof(FILE_ALLOCATED_RANGE_BUFFER);

        if (outputCount == 0)
            break;

        for (ULONG i = 0; i < outputCount; i++)
        {
            if (Callback)
            {
                if (!Callback(&output[i], Context))
                    break;
            }
        }

        input.FileOffset.QuadPart = output[outputCount - 1].FileOffset.QuadPart + output[outputCount - 1].Length.QuadPart;
        input.Length.QuadPart = fileSize.QuadPart - input.FileOffset.QuadPart;

        if (input.Length.QuadPart == 0)
            break;
    }

    return status;
}

NTSTATUS PvLayoutEnumerateFileLayouts(
    _In_ PPV_PE_LAYOUT_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE fileHandle = NULL;
    HANDLE volumeHandle = NULL;
    PPH_STRING volumeName;
    PH_STRINGREF firstPart;
    PH_STRINGREF lastPart;
    ULONG outputLength;
    FILE_INTERNAL_INFORMATION internalInfo;
    FILE_QUERY_METADATA_OPTIMIZATION_OUTPUT fileMetadataOptimization;
    QUERY_FILE_LAYOUT_INPUT input;
    PQUERY_FILE_LAYOUT_OUTPUT output;
    PFILE_LAYOUT_ENTRY fileLayoutEntry;
    PFILE_LAYOUT_NAME_ENTRY fileLayoutNameEntry;
    PSTREAM_LAYOUT_ENTRY fileLayoutSteamEntry;
    PFILE_LAYOUT_INFO_ENTRY fileLayoutInfoEntry;

    if (!PhSplitStringRefAtChar(&PvFileName->sr, L':', &firstPart, &lastPart))
        return STATUS_UNSUCCESSFUL;
    if (firstPart.Length != sizeof(L':'))
        return STATUS_UNSUCCESSFUL;

    //WCHAR volumeName[MAX_PATH + 1];
    //if (!GetVolumePathName(PvFileName->Buffer, volumeName, MAX_PATH))
    //    return PhGetLastWin32ErrorAsNtStatus();

    volumeName = PhCreateString2(&firstPart);
    PhMoveReference(&volumeName, PhConcatStrings(3, L"\\??\\", PhGetStringOrEmpty(volumeName), L":"));

    if (PhDetermineDosPathNameType(PhGetString(volumeName)) != RtlPathTypeRooted)
    {
        PhDereferenceObject(volumeName);
        return STATUS_UNSUCCESSFUL;
    }

    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    memset(&fileMetadataOptimization, 0, sizeof(FILE_QUERY_METADATA_OPTIMIZATION_OUTPUT));
    status = PhDeviceIoControlFile(
        fileHandle,
        FSCTL_QUERY_FILE_METADATA_OPTIMIZATION,
        NULL,
        0,
        &fileMetadataOptimization,
        sizeof(FILE_QUERY_METADATA_OPTIMIZATION_OUTPUT),
        NULL
        );

    //if (!NT_SUCCESS(status))
    //    goto CleanupExit;

    status = PhGetFileIndexNumber(
        fileHandle,
        &internalInfo
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateFile(
        &volumeHandle,
        &volumeName->sr,
        FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE, // magic value
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    memset(&input, 0, sizeof(QUERY_FILE_LAYOUT_INPUT));
    input.Flags =
        QUERY_FILE_LAYOUT_RESTART |
        QUERY_FILE_LAYOUT_INCLUDE_NAMES |
        QUERY_FILE_LAYOUT_INCLUDE_STREAMS |
        QUERY_FILE_LAYOUT_INCLUDE_EXTENTS |
        QUERY_FILE_LAYOUT_INCLUDE_EXTRA_INFO |
        QUERY_FILE_LAYOUT_INCLUDE_STREAMS_WITH_NO_CLUSTERS_ALLOCATED |
        QUERY_FILE_LAYOUT_INCLUDE_FULL_PATH_IN_NAMES |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_DSC_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_TXF_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_EFS_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_FILES_WITH_DSC_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_DATA_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_REPARSE_ATTRIBUTE |
        QUERY_FILE_LAYOUT_INCLUDE_STREAM_INFORMATION_FOR_EA_ATTRIBUTE;

    input.FilterEntryCount = 1;
    input.FilterType = QUERY_FILE_LAYOUT_FILTER_TYPE_FILEID;
    input.Filter.FileReferenceRanges->StartingFileReferenceNumber = internalInfo.IndexNumber.QuadPart;
    input.Filter.FileReferenceRanges->EndingFileReferenceNumber = internalInfo.IndexNumber.QuadPart;

    outputLength = 0x2000000; // magic value
    output = PhAllocateZero(outputLength);

    while (TRUE)
    {
        status = PhDeviceIoControlFile(
            volumeHandle,
            FSCTL_QUERY_FILE_LAYOUT,
            &input,
            sizeof(QUERY_FILE_LAYOUT_INPUT),
            output,
            outputLength,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;

        for (fileLayoutEntry = PH_FIRST_LAYOUT_ENTRY(output); fileLayoutEntry; fileLayoutEntry = PH_NEXT_LAYOUT_ENTRY(fileLayoutEntry))
        {
            if (fileLayoutEntry->Version != FILE_LAYOUT_ENTRY_VERSION)
            {
                status = STATUS_INVALID_KERNEL_INFO_VERSION;
                break;
            }

            //if (fileLayoutEntry->Flags == 1 && fileLayoutEntry->FileReferenceNumber == 1)
            //    continue;

            fileLayoutSteamEntry = PTR_ADD_OFFSET(fileLayoutEntry, fileLayoutEntry->FirstStreamOffset);
            fileLayoutInfoEntry = PTR_ADD_OFFSET(fileLayoutEntry, fileLayoutEntry->ExtraInfoOffset);

            PvAddChildLayoutNode(Context, NULL, L"File reference number", PhFormatString(L"%I64u (0x%I64x)", fileLayoutEntry->FileReferenceNumber, fileLayoutEntry->FileReferenceNumber));
            PvAddChildLayoutNode(Context, NULL, L"File attributes", PhFormatUInt64(fileLayoutEntry->FileAttributes, FALSE));
            PvAddChildLayoutNode(Context, NULL, L"File entry flags", PhFormatUInt64(fileLayoutEntry->Flags, FALSE));
            PvAddChildLayoutNode(Context, NULL, L"Creation time", PvLayoutGetRelativeTimeString(&fileLayoutInfoEntry->BasicInformation.CreationTime));
            //PvAddChildLayoutNode(Context, NULL, L"Last access time", PvLayoutGetRelativeTimeString(&fileLayoutInfoEntry->BasicInformation.LastAccessTime));
            PvAddChildLayoutNode(Context, NULL, L"Last write time", PvLayoutGetRelativeTimeString(&fileLayoutInfoEntry->BasicInformation.LastWriteTime));
            PvAddChildLayoutNode(Context, NULL, L"Change time", PvLayoutGetRelativeTimeString(&fileLayoutInfoEntry->BasicInformation.ChangeTime));
            PvAddChildLayoutNode(Context, NULL, L"LastUsn", PhFormatUInt64(fileLayoutInfoEntry->Usn, FALSE));
            PvAddChildLayoutNode(Context, NULL, L"OwnerId", PhFormatUInt64(fileLayoutInfoEntry->OwnerId, FALSE));
            PvAddChildLayoutNode(Context, NULL, L"SecurityId", PhFormatUInt64(fileLayoutInfoEntry->SecurityId, FALSE));
            PvAddChildLayoutNode(Context, NULL, L"StorageReserveId", PhFormatUInt64(fileLayoutInfoEntry->StorageReserveId, FALSE));
            PvAddChildLayoutNode(Context, NULL, L"Attribute list size", PvLayoutFormatSize(fileMetadataOptimization.AttributeListSize));
            PvAddChildLayoutNode(Context, NULL, L"Metadata space used", PvLayoutFormatSize(fileMetadataOptimization.MetadataSpaceUsed));
            PvAddChildLayoutNode(Context, NULL, L"Metadata space allocated", PvLayoutFormatSize(fileMetadataOptimization.MetadataSpaceAllocated));
            PvAddChildLayoutNode(Context, NULL, L"Number of file records", PhFormatUInt64(fileMetadataOptimization.NumberOfFileRecords, TRUE));
            PvAddChildLayoutNode(Context, NULL, L"Number of resident attributes", PhFormatUInt64(fileMetadataOptimization.NumberOfResidentAttributes, TRUE));
            PvAddChildLayoutNode(Context, NULL, L"Number of nonresident attributes", PhFormatUInt64(fileMetadataOptimization.NumberOfNonresidentAttributes, TRUE));

            if (fileLayoutEntry->FirstNameOffset)
            {
                fileLayoutNameEntry = PTR_ADD_OFFSET(fileLayoutEntry, fileLayoutEntry->FirstNameOffset);

                while (TRUE)
                {
                    PPV_LAYOUT_NODE parentNode;
                    PPH_STRING layoutFileName;

                    if (fileLayoutNameEntry->FileNameLength >= sizeof(UNICODE_NULL))
                        layoutFileName = PhCreateStringEx(fileLayoutNameEntry->FileName, fileLayoutNameEntry->FileNameLength);
                    else
                        layoutFileName = PhReferenceEmptyString();

                    parentNode = PvAddChildLayoutNode(Context, NULL, L"Filename", NULL);
                    PvAddChildLayoutNode(Context, parentNode, PvLayoutNameFlagsToString(fileLayoutNameEntry->Flags), layoutFileName);
                    //PvAddChildLayoutNode(Context, parentNode, L"Parent Name", PvLayoutGetParentIdName(fileHandle, fileLayoutNameEntry->ParentFileReferenceNumber));
                    PvAddChildLayoutNode(Context, parentNode, L"Parent ID", PhFormatString(L"%I64u (0x%I64x)", fileLayoutNameEntry->ParentFileReferenceNumber, fileLayoutNameEntry->ParentFileReferenceNumber));

                    if (fileLayoutNameEntry->NextNameOffset == 0)
                        break;

                    fileLayoutNameEntry = PTR_ADD_OFFSET(fileLayoutNameEntry, fileLayoutNameEntry->NextNameOffset);
                }
            }

            while (TRUE)
            {
                PPV_LAYOUT_NODE parentNode;

                if (fileLayoutSteamEntry->Version != STREAM_LAYOUT_ENTRY_VERSION)
                {
                    status = STATUS_INVALID_KERNEL_INFO_VERSION;
                    break;
                }

                parentNode = PvAddChildLayoutNode(Context, NULL, L"Stream", NULL);

                if (fileLayoutSteamEntry->AttributeTypeCode == ATTRIBUTE_TYPECODE_DATA)
                    PvAddChildLayoutNode(Context, parentNode, L"Name", PhCreateString(L"::$DATA"));
                else
                    PvAddChildLayoutNode(Context, parentNode, L"Name", PhCreateStringEx(fileLayoutSteamEntry->StreamIdentifier, fileLayoutSteamEntry->StreamIdentifierLength));
                PvAddChildLayoutNode(Context, parentNode, L"Attributes", PhFormatUInt64(fileLayoutSteamEntry->AttributeFlags, FALSE));
                PvAddChildLayoutNode(Context, parentNode, L"Attribute typecode", PhFormatString(L"0x%x", fileLayoutSteamEntry->AttributeTypeCode));
                PvAddChildLayoutNode(Context, parentNode, L"Flags", PvLayoutSteamFlagsToString(fileLayoutSteamEntry->Flags));
                PvAddChildLayoutNode(Context, parentNode, L"Size", PvLayoutFormatSize(fileLayoutSteamEntry->EndOfFile.QuadPart));
                PvAddChildLayoutNode(Context, parentNode, L"Allocated Size", PvLayoutFormatSize(fileLayoutSteamEntry->AllocationSize.QuadPart));

                if (fileLayoutSteamEntry->StreamInformationOffset)
                {
                    PSTREAM_INFORMATION_ENTRY streamInformationEntry;

                    streamInformationEntry = PTR_ADD_OFFSET(fileLayoutSteamEntry, fileLayoutSteamEntry->StreamInformationOffset);

                    if (streamInformationEntry->Version == 1)
                    {
                        if (fileLayoutSteamEntry->AttributeTypeCode == ATTRIBUTE_TYPECODE_DATA)
                        {
                            PvAddChildLayoutNode(Context, parentNode, L"Valid Data Length", PvLayoutFormatSize(streamInformationEntry->StreamInformation.DataStream.Vdl));
                        }

                        if (fileLayoutSteamEntry->AttributeTypeCode == ATTRIBUTE_TYPECODE_EA)
                        {
                            PFILE_FULL_EA_INFORMATION eainfo;
                            PFILE_FULL_EA_INFORMATION i;

                            eainfo = PTR_ADD_OFFSET(streamInformationEntry, streamInformationEntry->StreamInformation.Ea.EaInformationOffset);

                            for (i = PH_FIRST_FILE_EA(eainfo); i; i = PH_NEXT_FILE_EA(i))
                            {
                                PPV_LAYOUT_NODE parentAttributeNode = PvAddChildLayoutNode(Context, parentNode, L"Extended Attributes", NULL);
                                PvAddChildLayoutNode(Context, parentAttributeNode, L"Name", PhConvertUtf8ToUtf16Ex(i->EaName, i->EaNameLength));
                                PvAddChildLayoutNode(Context, parentAttributeNode, L"Size", PvLayoutFormatSize(i->EaValueLength));
                            }
                        }
                    }
                }

                if (fileLayoutSteamEntry->ExtentInformationOffset)
                {
                    PSTREAM_EXTENT_ENTRY streamExtentEntry = PTR_ADD_OFFSET(fileLayoutSteamEntry, fileLayoutSteamEntry->ExtentInformationOffset);

                    PvAddChildLayoutNode(Context, parentNode, L"Extents", PhFormatUInt64(streamExtentEntry->ExtentInformation.RetrievalPointers.ExtentCount, FALSE));
                    //PvAddChildLayoutNode(Context, parentNode, L"StartingVcn", PhFormatUInt64(streamExtentEntry->ExtentInformation.RetrievalPointers.StartingVcn.QuadPart, FALSE));

                    //for (ULONG i = 0; i < streamExtentEntry->ExtentInformation.RetrievalPointers.ExtentCount; i++)
                    //{
                    //    PPV_LAYOUT_NODE parentExtentNode = PvAddChildLayoutNode(Context, parentNode, L"Extents", NULL);
                    //    //PvAddChildLayoutNode(Context, parentExtentNode, L"Index", PhFormatUInt64(i, TRUE));
                    //    PvAddChildLayoutNode(Context, parentExtentNode, L"Lcn", PhFormatUInt64(streamExtentEntry->ExtentInformation.RetrievalPointers.Extents[i].Lcn.QuadPart, FALSE));
                    //    PvAddChildLayoutNode(Context, parentExtentNode, L"NextVcn", PhFormatUInt64(streamExtentEntry->ExtentInformation.RetrievalPointers.Extents[i].NextVcn.QuadPart, FALSE));
                    //}
                }

                if (fileLayoutSteamEntry->NextStreamOffset == 0)
                    break;

                fileLayoutSteamEntry = PTR_ADD_OFFSET(fileLayoutSteamEntry, fileLayoutSteamEntry->NextStreamOffset);
            }
        }

        if (!NT_SUCCESS(status))
            break;

        if (input.Flags & QUERY_FILE_LAYOUT_RESTART)
        {
            input.Flags &= ~QUERY_FILE_LAYOUT_RESTART;
        }
    }

    if (status == STATUS_END_OF_FILE)
    {
        status = STATUS_SUCCESS;
    }

CleanupExit:

    if (volumeHandle)
    {
        NtClose(volumeHandle);
    }

    if (fileHandle)
    {
        NtClose(fileHandle);
    }

    PhDereferenceObject(volumeName);

    return status;
}

INT_PTR CALLBACK PvpPeLayoutDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PE_LAYOUT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_PE_LAYOUT_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
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
            context->SearchHandle = GetDlgItem(hwndDlg, IDC_TREESEARCH);
            context->SearchboxText = PhReferenceEmptyString();

            PvCreateSearchControl(context->SearchHandle, L"Search Layout (Ctrl+K)");
            PvConfigTreeBorders(context->TreeNewHandle);

            PvInitializeLayoutTree(context);
            PhAddTreeNewFilter(&context->FilterSupport, PvLayoutTreeFilterCallback, context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            if (!NT_SUCCESS(status = PvLayoutEnumerateFileLayouts(context)))
            {
                PvLayoutSetStatusMessage(context, status);
            }

            TreeNew_NodesStructured(context->TreeNewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PvDeleteLayoutTree(context);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    PPH_STRING newSearchboxText;

                    newSearchboxText = PH_AUTO(PhGetWindowText(context->SearchHandle));

                    if (!PhEqualString(context->SearchboxText, newSearchboxText, FALSE))
                    {
                        PhSwapReference(&context->SearchboxText, newSearchboxText);

                        if (!PhIsNullOrEmptyString(context->SearchboxText))
                        {
                            //PhExpandAllProcessNodes(TRUE);
                            //PhDeselectAllProcessNodes();
                        }

                        PhApplyTreeNewFilters(&context->FilterSupport);
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case WM_PV_LAYOUT_CONTEXTMENU:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    PPV_LAYOUT_NODE* nodes = NULL;
                    ULONG numberOfNodes = 0;

                    if (!PvGetSelectedLayoutNodes(context, &nodes, &numberOfNodes))
                        break;

                    if (numberOfNodes != 0)
                    {
                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"Copy", NULL, NULL), ULONG_MAX);
                        PhInsertCopyCellEMenuItem(menu, USHRT_MAX, context->TreeNewHandle, contextMenuEvent->Column);

                        selectedItem = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            contextMenuEvent->Location.x,
                            contextMenuEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
                        {
                            if (!PhHandleCopyCellEMenuItem(selectedItem))
                            {
                                switch (selectedItem->Id)
                                {
                                case USHRT_MAX:
                                    {
                                        PPH_STRING text;

                                        text = PhGetTreeNewText(context->TreeNewHandle, 0);
                                        PhSetClipboardString(context->TreeNewHandle, &text->sr);
                                        PhDereferenceObject(text);
                                    }
                                    break;
                                }
                            }
                        }

                        PhDestroyEMenu(menu);
                    }

                    PhFree(nodes);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->TreeNewHandle);
                return TRUE;
            }
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
