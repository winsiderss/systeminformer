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

#define WM_PV_CERTIFICATE_PROPERTIES (WM_APP + 801)
#define WM_PV_CERTIFICATE_CONTEXTMENU (WM_APP + 802)

typedef struct _PV_PE_CERTIFICATE_CONTEXT
{
    HWND WindowHandle;
    HWND LabelHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
    PPH_TN_FILTER_ENTRY TreeFilterEntry;
    PPH_STRING SearchText;
    ULONG TotalSize;
    ULONG TotalCount;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeRootList;
    PPH_LIST NodeList;
} PV_PE_CERTIFICATE_CONTEXT, *PPV_PE_CERTIFICATE_CONTEXT;

typedef enum _PV_CERTIFICATE_TREE_COLUMN_NAME
{
    PV_CERTIFICATE_TREE_COLUMN_NAME_NAME,
    PV_CERTIFICATE_TREE_COLUMN_NAME_INDEX,
    PV_CERTIFICATE_TREE_COLUMN_NAME_TYPE,
    PV_CERTIFICATE_TREE_COLUMN_NAME_ISSUER,
    PV_CERTIFICATE_TREE_COLUMN_NAME_DATEFROM,
    PV_CERTIFICATE_TREE_COLUMN_NAME_DATETO,
    PV_CERTIFICATE_TREE_COLUMN_NAME_THUMBPRINT,
    PV_CERTIFICATE_TREE_COLUMN_NAME_SIZE,
    PV_CERTIFICATE_TREE_COLUMN_NAME_ALG,
    PV_CERTIFICATE_TREE_COLUMN_NAME_MAXIMUM
} PV_CERTIFICATE_TREE_COLUMN_NAME;

typedef enum _PV_CERTIFICATE_NODE_TYPE
{
    PV_CERTIFICATE_NODE_TYPE_IMAGE,
    PV_CERTIFICATE_NODE_TYPE_IMAGEARRAY,
    PV_CERTIFICATE_NODE_TYPE_NESTED,
    PV_CERTIFICATE_NODE_TYPE_NESTEDARRAY,
    PV_CERTIFICATE_NODE_TYPE_MAXIMUM
} PV_CERTIFICATE_NODE_TYPE;

typedef struct _PV_CERTIFICATE_NODE
{
    PH_TREENEW_NODE Node;

    PCCERT_CONTEXT CertContext;
    PV_CERTIFICATE_NODE_TYPE Type;
    //ULONG64 UniqueId;
    ULONG64 Size;
    LARGE_INTEGER TimeFrom;
    LARGE_INTEGER TimeTo;

    PPH_STRING Name;
    PPH_STRING Issuer;
    PPH_STRING DateFrom;
    PPH_STRING DateTo;
    PPH_STRING Thumbprint;
    PPH_STRING Algorithm;

    PPH_STRING IndexString;
    PPH_STRING SizeString;
    WCHAR Pointer[PH_PTR_STR_LEN_1];

    struct _PV_CERTIFICATE_NODE* Parent;
    PPH_LIST Children;

    PH_STRINGREF TextCache[PV_CERTIFICATE_TREE_COLUMN_NAME_MAXIMUM];
} PV_CERTIFICATE_NODE, *PPV_CERTIFICATE_NODE;

BOOLEAN PvCertificateNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );
ULONG PvCertificateNodeHashtableHashFunction(
    _In_ PVOID Entry
    );
VOID PvDestroyCertificateNode(
    _In_ PPV_CERTIFICATE_NODE CertificateNode
    );
BOOLEAN NTAPI PvCertificateTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

BOOLEAN PvpPeFillNodeCertificateInfo(
    _Inout_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ PV_CERTIFICATE_NODE_TYPE CertType,
    _In_ PCCERT_CONTEXT CertificateContext,
    _In_ PPV_CERTIFICATE_NODE CertificateNode
    );

VOID PvpPeEnumerateNestedSignatures(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ PCMSG_SIGNER_INFO SignerInfo
    );

static BOOLEAN WordMatchStringRef(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = PhGetStringRef(Context->SearchText);

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

BOOLEAN PvCertificateTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPV_CERTIFICATE_NODE certificateNode = (PPV_CERTIFICATE_NODE)Node;
    PPV_PE_CERTIFICATE_CONTEXT context = Context;

    if (!context)
        return TRUE;
    if (PhIsNullOrEmptyString(context->SearchText))
        return TRUE;

    if (!PhIsNullOrEmptyString(certificateNode->Name))
    {
        if (WordMatchStringRef(context, &certificateNode->Name->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(certificateNode->Issuer))
    {
        if (WordMatchStringRef(context, &certificateNode->Issuer->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(certificateNode->DateFrom))
    {
        if (WordMatchStringRef(context, &certificateNode->DateFrom->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(certificateNode->DateTo))
    {
        if (WordMatchStringRef(context, &certificateNode->DateTo->sr))
            return TRUE;
    }

    return FALSE;
}

VOID PvInitializeCertificateTree(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context
    )
{
    PPH_STRING settings;

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PV_CERTIFICATE_NODE),
        PvCertificateNodeHashtableEqualFunction,
        PvCertificateNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);
    Context->NodeRootList = PhCreateList(30);

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetCallback(Context->TreeNewHandle, PvCertificateTreeNewCallback, Context);
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAddTreeNewColumn(Context->TreeNewHandle, PV_CERTIFICATE_TREE_COLUMN_NAME_NAME, TRUE, L"Name", 200, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_CERTIFICATE_TREE_COLUMN_NAME_INDEX, TRUE, L"#", 25, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_CERTIFICATE_TREE_COLUMN_NAME_TYPE, TRUE, L"Type", 50, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_CERTIFICATE_TREE_COLUMN_NAME_ISSUER, TRUE, L"Issuer", 100, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_CERTIFICATE_TREE_COLUMN_NAME_DATEFROM, TRUE, L"From", 100, PH_ALIGN_LEFT, 4, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_CERTIFICATE_TREE_COLUMN_NAME_DATETO, TRUE, L"To", 100, PH_ALIGN_LEFT, 5, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_CERTIFICATE_TREE_COLUMN_NAME_THUMBPRINT, TRUE, L"Thumbprint", 100, PH_ALIGN_LEFT, 6, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_CERTIFICATE_TREE_COLUMN_NAME_SIZE, TRUE, L"Size", 50, PH_ALIGN_LEFT, 7, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_CERTIFICATE_TREE_COLUMN_NAME_ALG, TRUE, L"Algorithm", 50, PH_ALIGN_LEFT, 8, 0);

    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);
    TreeNew_SetSort(Context->TreeNewHandle, PV_CERTIFICATE_TREE_COLUMN_NAME_INDEX, NoSortOrder);
    TreeNew_SetRowHeight(Context->TreeNewHandle, 22);

    settings = PhGetStringSetting(L"ImageSecurityTreeColumns");
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);

    Context->SearchText = PhReferenceEmptyString();
    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);
    Context->TreeFilterEntry = PhAddTreeNewFilter(&Context->FilterSupport, PvCertificateTreeFilterCallback, Context);
}

VOID PvDeleteCertificateTree(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context
    )
{
    PPH_STRING settings;
    ULONG i;

    PhRemoveTreeNewFilter(&Context->FilterSupport, Context->TreeFilterEntry);
    if (Context->SearchText) PhDereferenceObject(Context->SearchText);

    PhDeleteTreeNewFilterSupport(&Context->FilterSupport);

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(L"ImageSecurityTreeColumns", &settings->sr);
    PhDereferenceObject(settings);

    for (i = 0; i < Context->NodeList->Count; i++)
        PvDestroyCertificateNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
    PhDereferenceObject(Context->NodeRootList);
}

BOOLEAN PvCertificateNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPV_CERTIFICATE_NODE node1 = *(PPV_CERTIFICATE_NODE*)Entry1;
    PPV_CERTIFICATE_NODE node2 = *(PPV_CERTIFICATE_NODE*)Entry2;

    return node1->Node.Index == node2->Node.Index;
}

ULONG PvCertificateNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)(*(PPV_CERTIFICATE_NODE*)Entry)->Node.Index);
}

PPV_CERTIFICATE_NODE PvAddCertificateNode(
    _Inout_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ PV_CERTIFICATE_NODE_TYPE CertType,
    _In_ PCCERT_CONTEXT CertContext
    )
{
    //static ULONG64 index = 0;
    PPV_CERTIFICATE_NODE node;

    node = PhAllocateZero(sizeof(PV_CERTIFICATE_NODE));
    //node->UniqueId = ++index;
    PhInitializeTreeNewNode(&node->Node);

    memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PV_CERTIFICATE_TREE_COLUMN_NAME_MAXIMUM);
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = PV_CERTIFICATE_TREE_COLUMN_NAME_MAXIMUM;

    node->Type = CertType;
    node->CertContext = CertContext;
    node->Children = PhCreateList(1);
    PvpPeFillNodeCertificateInfo(Context, CertType, CertContext, node);

    PhAddEntryHashtable(Context->NodeHashtable, &node);
    PhAddItemList(Context->NodeList, node);

    if (Context->FilterSupport.FilterList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &node->Node);

    return node;
}

PPV_CERTIFICATE_NODE PvAddChildCertificateNode(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_opt_ PPV_CERTIFICATE_NODE ParentNode,
    _In_ PV_CERTIFICATE_NODE_TYPE CertType,
    _In_ PCCERT_CONTEXT CertContext
    )
{
    PPV_CERTIFICATE_NODE childNode;

    childNode = PvAddCertificateNode(Context, CertType, CertContext);
    //PvFillCertificateInfo(Context, childNode);

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

PPV_CERTIFICATE_NODE PvFindCertificateNode(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ ULONG UniqueId
    )
{
    PV_CERTIFICATE_NODE lookupNode;
    PPV_CERTIFICATE_NODE lookupNodePtr = &lookupNode;
    PPV_CERTIFICATE_NODE* node;

    lookupNode.Node.Index = UniqueId;

    node = (PPV_CERTIFICATE_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupNodePtr
        );

    if (node)
        return *node;
    else
        return NULL;
}

VOID PvRemoveCertificateNode(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ PPV_CERTIFICATE_NODE Node
    )
{
    ULONG index;

    // Remove from hashtable/list and cleanup.

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);

    PvDestroyCertificateNode(Node);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PvDestroyCertificateNode(
    _In_ PPV_CERTIFICATE_NODE Node
    )
{
    PhDereferenceObject(Node->Children);

    if (Node->Name) PhDereferenceObject(Node->Name);
    if (Node->Issuer) PhDereferenceObject(Node->Issuer);
    if (Node->DateFrom) PhDereferenceObject(Node->DateFrom);
    if (Node->DateTo) PhDereferenceObject(Node->DateTo);
    if (Node->Thumbprint) PhDereferenceObject(Node->Thumbprint);
    if (Node->IndexString) PhDereferenceObject(Node->IndexString);
    if (Node->SizeString) PhDereferenceObject(Node->SizeString);

    //if (Node->WindowText) PhDereferenceObject(Node->WindowText);
    //if (Node->ThreadString) PhDereferenceObject(Node->ThreadString);
    //if (Node->ModuleString) PhDereferenceObject(Node->ModuleString);

    PhFree(Node);
}

#define SORT_FUNCTION(Column) PvCertificateTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvCertificateTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPV_CERTIFICATE_NODE node1 = *(PPV_CERTIFICATE_NODE *)_elem1; \
    PPV_CERTIFICATE_NODE node2 = *(PPV_CERTIFICATE_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    return PhModifySort(sortResult, ((PPV_PE_CERTIFICATE_CONTEXT)_context)->TreeNewSortOrder); \
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

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Type, (ULONG_PTR)node2->Type);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Issuer)
{
    sortResult = PhCompareString(node1->Issuer, node2->Issuer, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(DateFrom)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->TimeFrom.QuadPart, (ULONG_PTR)node2->TimeFrom.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(DateTo)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->TimeTo.QuadPart, (ULONG_PTR)node2->TimeTo.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Thumbprint)
{
    sortResult = PhCompareString(node1->Thumbprint, node2->Thumbprint, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Size)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Size, (ULONG_PTR)node2->Size);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Alg)
{
    sortResult = PhCompareString(node1->Algorithm, node2->Algorithm, TRUE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PvCertificateTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPV_PE_CERTIFICATE_CONTEXT context = Context;
    PPV_CERTIFICATE_NODE node;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            node = (PPV_CERTIFICATE_NODE)getChildren->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
            {
                if (!node)
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)context->NodeRootList->Items;
                    getChildren->NumberOfChildren = context->NodeRootList->Count;
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
                        SORT_FUNCTION(Index),
                        SORT_FUNCTION(Type),
                        SORT_FUNCTION(Issuer),
                        SORT_FUNCTION(DateFrom),
                        SORT_FUNCTION(DateTo),
                        SORT_FUNCTION(Thumbprint),
                        SORT_FUNCTION(Size),
                        SORT_FUNCTION(Alg)
                    };
                    int (__cdecl *sortFunction)(void *, const void *, const void *);

                    if (context->TreeNewSortColumn < PV_CERTIFICATE_TREE_COLUMN_NAME_MAXIMUM)
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
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            if (!isLeaf)
                break;

            node = (PPV_CERTIFICATE_NODE)isLeaf->Node;

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

            node = (PPV_CERTIFICATE_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PV_CERTIFICATE_TREE_COLUMN_NAME_NAME:
                {
                    getCellText->Text = PhGetStringRef(node->Name);
                }
                break;
            case PV_CERTIFICATE_TREE_COLUMN_NAME_INDEX:
                {
                    PhMoveReference(&node->IndexString, PhFormatUInt64(node->Node.Index + 1, TRUE));
                    getCellText->Text = node->IndexString->sr;
                }
                break;
            case PV_CERTIFICATE_TREE_COLUMN_NAME_TYPE:
                {
                    switch (node->Type)
                    {
                    case PV_CERTIFICATE_NODE_TYPE_IMAGE:
                    case PV_CERTIFICATE_NODE_TYPE_IMAGEARRAY:
                        PhInitializeStringRef(&getCellText->Text, L"Image");
                        break;
                    //case PV_CERTIFICATE_NODE_TYPE_IMAGEARRAY:
                    //    PhInitializeStringRef(&getCellText->Text, L"Chained");
                    //    break;
                    case PV_CERTIFICATE_NODE_TYPE_NESTED:
                    case PV_CERTIFICATE_NODE_TYPE_NESTEDARRAY:
                        PhInitializeStringRef(&getCellText->Text, L"Nested");
                        break;
                    //case PV_CERTIFICATE_NODE_TYPE_NESTEDARRAY:
                    //    PhInitializeStringRef(&getCellText->Text, L"Chained");
                    //    break;
                    }
                }
                break;
            case PV_CERTIFICATE_TREE_COLUMN_NAME_ISSUER:
                {
                    getCellText->Text = PhGetStringRef(node->Issuer);
                }
                break;
            case PV_CERTIFICATE_TREE_COLUMN_NAME_DATEFROM:
                {
                    getCellText->Text = PhGetStringRef(node->DateFrom);
                }
                break;
            case PV_CERTIFICATE_TREE_COLUMN_NAME_DATETO:
                {
                    getCellText->Text = PhGetStringRef(node->DateTo);
                }
                break;
            case PV_CERTIFICATE_TREE_COLUMN_NAME_THUMBPRINT:
                {
                    getCellText->Text = PhGetStringRef(node->Thumbprint);
                }
                break;
            case PV_CERTIFICATE_TREE_COLUMN_NAME_SIZE:
                {
                    PhMoveReference(&node->SizeString, PhFormatSize(node->Size, ULONG_MAX));
                    getCellText->Text = node->SizeString->sr;
                }
                break;
            case PV_CERTIFICATE_TREE_COLUMN_NAME_ALG:
                {
                    getCellText->Text = PhGetStringRef(node->Algorithm);
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

            node = (PPV_CERTIFICATE_NODE)getNodeColor->Node;

            //if (!node->WindowVisible)
            //    getNodeColor->ForeColor = RGB(0x55, 0x55, 0x55);

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
            SendMessage(context->WindowHandle, WM_COMMAND, WM_PV_CERTIFICATE_PROPERTIES, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(context->WindowHandle, WM_COMMAND, WM_PV_CERTIFICATE_CONTEXTMENU, (LPARAM)contextMenuEvent);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

VOID PvClearCertificateTree(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PvDestroyCertificateNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
    PhClearList(Context->NodeRootList);
}

PPV_CERTIFICATE_NODE PvGetSelectedCertificateNode(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context
    )
{
    PPV_CERTIFICATE_NODE node = NULL;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

BOOLEAN PvGetSelectedCertificateNodes(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _Out_ PPV_CERTIFICATE_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_CERTIFICATE_NODE node = Context->NodeList->Items[i];

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

VOID PvExpandAllCertificateNodes(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ BOOLEAN Expand
    )
{
    ULONG i;
    BOOLEAN needsRestructure = FALSE;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_CERTIFICATE_NODE node = Context->NodeList->Items[i];

        if (node->Children->Count != 0 && node->Node.Expanded != Expand)
        {
            node->Node.Expanded = Expand;
            needsRestructure = TRUE;
        }
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PvDeselectAllCertificateNodes(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context
    )
{
    TreeNew_DeselectRange(Context->TreeNewHandle, 0, -1);
}

VOID PvSelectAndEnsureVisibleCertificateNodes(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ PPV_CERTIFICATE_NODE* CertificateNodes,
    _In_ ULONG NumberOfCertificateNodes
    )
{
    ULONG i;
    PPV_CERTIFICATE_NODE leader = NULL;
    PPV_CERTIFICATE_NODE node;
    BOOLEAN needsRestructure = FALSE;

    PvDeselectAllCertificateNodes(Context);

    for (i = 0; i < NumberOfCertificateNodes; i++)
    {
        if (CertificateNodes[i]->Node.Visible)
        {
            leader = CertificateNodes[i];
            break;
        }
    }

    if (!leader)
        return;

    // Expand recursively upwards, and select the nodes.

    for (i = 0; i < NumberOfCertificateNodes; i++)
    {
        if (!CertificateNodes[i]->Node.Visible)
            continue;

        node = CertificateNodes[i]->Parent;

        while (node)
        {
            if (!node->Node.Expanded)
                needsRestructure = TRUE;

            node->Node.Expanded = TRUE;
            node = node->Parent;
        }

        CertificateNodes[i]->Node.Selected = TRUE;
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);

    TreeNew_SetFocusNode(Context->TreeNewHandle, &leader->Node);
    TreeNew_SetMarkNode(Context->TreeNewHandle, &leader->Node);
    TreeNew_EnsureVisible(Context->TreeNewHandle, &leader->Node);
    TreeNew_InvalidateNode(Context->TreeNewHandle, &leader->Node);
}

PPH_STRING PvpPeFormatDateTime(
    _In_ PSYSTEMTIME SystemTime
    )
{
    return PhFormatString(
        L"%s, %s\n",
        PH_AUTO_T(PH_STRING, PhFormatDate(SystemTime, L"dddd, MMMM d, yyyy"))->Buffer,
        PH_AUTO_T(PH_STRING, PhFormatTime(SystemTime, L"hh:mm:ss tt"))->Buffer
        );
}

PPH_STRING PvpPeGetRelativeTimeString(
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
    timeString = PH_AUTO(PvpPeFormatDateTime(&timeFields));

    return PhFormatString(L"%s (%s ago)", timeString->Buffer, timeRelativeString->Buffer);
}

typedef BOOLEAN (CALLBACK* PH_CERT_ENUM_CALLBACK)(
    _In_ PCCERT_CONTEXT CertContext,
    _In_opt_ PVOID Context
    );

VOID PhEnumImageCertificates(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ PH_CERT_ENUM_CALLBACK Callback,
    _In_ PCCERT_CONTEXT CertificateContext,
    _In_opt_ PVOID CallbackContext
    )
{
    CERT_ENHKEY_USAGE enhkeyUsage;
    CERT_USAGE_MATCH certUsage;
    CERT_CHAIN_PARA chainPara;
    PCCERT_CHAIN_CONTEXT chainContext;

    enhkeyUsage.cUsageIdentifier = 0;
    enhkeyUsage.rgpszUsageIdentifier = NULL;
    certUsage.dwType = USAGE_MATCH_TYPE_AND;
    certUsage.Usage = enhkeyUsage;
    chainPara.cbSize = sizeof(CERT_CHAIN_PARA);
    chainPara.RequestedUsage = certUsage;

    if (CertGetCertificateChain(
        NULL,
        CertificateContext,
        NULL,
        NULL,
        &chainPara,
        0,
        NULL,
        &chainContext
        ))
    {
        for (ULONG i = 0; i < chainContext->cChain; i++)
        {
            PCERT_SIMPLE_CHAIN chain = chainContext->rgpChain[i];

            for (ULONG ii = 0; ii < chain->cElement; ii++)
            {
                PCERT_CHAIN_ELEMENT element = chain->rgpElement[ii];

                if (element->pCertContext == CertificateContext) // skip parent
                    continue;

                Callback(element->pCertContext, CallbackContext);
            }
        }

        //CertFreeCertificateChain(chainContext);
    }
}

BOOLEAN PvpPeFillNodeCertificateInfo(
    _Inout_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ PV_CERTIFICATE_NODE_TYPE CertType,
    _In_ PCCERT_CONTEXT CertificateContext,
    _In_ PPV_CERTIFICATE_NODE CertificateNode
    )
{
    ULONG dataLength;
    LARGE_INTEGER currentTime;
    LARGE_INTEGER time;
    PPH_STRING timeString;

    CertificateNode->TimeFrom.LowPart = CertificateContext->pCertInfo->NotBefore.dwLowDateTime;
    CertificateNode->TimeFrom.HighPart = CertificateContext->pCertInfo->NotBefore.dwHighDateTime;
    CertificateNode->TimeTo.LowPart = CertificateContext->pCertInfo->NotAfter.dwLowDateTime;
    CertificateNode->TimeTo.HighPart = CertificateContext->pCertInfo->NotAfter.dwHighDateTime;

    PhQuerySystemTime(&currentTime);
    time.QuadPart = CertificateNode->TimeTo.QuadPart - currentTime.QuadPart;

    if (time.QuadPart > 0)
    {
        SYSTEMTIME timeFields;
        PPH_STRING timeRelativeString;

        timeRelativeString = PH_AUTO(PhFormatTimeSpanRelative(time.QuadPart));
        PhLargeIntegerToLocalSystemTime(&timeFields, &CertificateNode->TimeTo);
        timeString = PH_AUTO(PvpPeFormatDateTime(&timeFields));
        timeString = PhFormatString(L"%s (%s)", timeString->Buffer, timeRelativeString->Buffer);
    }
    else
    {
        SYSTEMTIME timeFields;
        PhLargeIntegerToLocalSystemTime(&timeFields, &CertificateNode->TimeTo);
        timeString = PvpPeFormatDateTime(&timeFields);
    }

    CertificateNode->DateFrom = PvpPeGetRelativeTimeString(&CertificateNode->TimeFrom);
    CertificateNode->DateTo = timeString;
    CertificateNode->Size = CertificateContext->cbCertEncoded;

    if (CertType != PV_CERTIFICATE_NODE_TYPE_IMAGE)
        Context->TotalSize += CertificateContext->cbCertEncoded;
    Context->TotalCount++;

    if (dataLength = CertGetNameString(CertificateContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0))
    {
        PPH_STRING data = PhCreateStringEx(NULL, dataLength * sizeof(WCHAR));

        if (CertGetNameString(
            CertificateContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            0,
            NULL,
            data->Buffer,
            (ULONG)data->Length / sizeof(WCHAR)
            ))
        {
            CertificateNode->Name = data;
        }
        else
        {
            PhDereferenceObject(data);
        }
    }

    if (dataLength = CertGetNameString(CertificateContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, NULL, 0))
    {
        PPH_STRING data = PhCreateStringEx(NULL, dataLength * sizeof(WCHAR));

        if (CertGetNameString(
            CertificateContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            CERT_NAME_ISSUER_FLAG, NULL,
            data->Buffer,
            (ULONG)data->Length / sizeof(WCHAR)
            ))
        {
            CertificateNode->Issuer = data;
        }
        else
        {
            PhDereferenceObject(data);
        }
    }

    dataLength = 0;

    if (CertGetCertificateContextProperty(
        CertificateContext,
        CERT_HASH_PROP_ID,
        NULL,
        &dataLength
        ) && dataLength > 0)
    {
        PBYTE hash = PhAllocateZero(dataLength);

        if (CertGetCertificateContextProperty(CertificateContext, CERT_HASH_PROP_ID, hash, &dataLength))
        {
            CertificateNode->Thumbprint = PhBufferToHexString(hash, dataLength);
        }

        PhFree(hash);
    }

    //if (
    //    ((ULONG_PTR)indirect->Data.pszObjId >> 16) == 0 ||
    //    !RtlEqualMemory(indirect->Data.pszObjId, SPC_PE_IMAGE_DATA_OBJID, sizeof(SPC_PE_IMAGE_DATA_OBJID)) &&
    //    !RtlEqualMemory(indirect->Data.pszObjId, SPC_CAB_DATA_OBJID, sizeof(SPC_CAB_DATA_OBJID))
    //    )
    //{
    //    return TRUST_E_NOSIGNATURE;
    //}

    for (ULONG i = 0; i < CertificateContext->pCertInfo->cExtension; i++)
    {
        //dprintf("%s\n", CertificateContext->pCertInfo->rgExtension[i].pszObjId);
    }

    //if (CertificateContext->pCertInfo && CertificateContext->pCertInfo->SignatureAlgorithm.pszObjId)
    //{
    //    PCCRYPT_OID_INFO oidinfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, CertificateContext->pCertInfo->SignatureAlgorithm.pszObjId, 0);
    //
    //    if (oidinfo && oidinfo->pwszName)
    //    {
    //        CertificateNode->Algorithm = PhCreateString((PWSTR)oidinfo->pwszName);
    //    }
    //}
    //
    //if (PhIsNullOrEmptyString(CertificateNode->Algorithm))
    {
        dataLength = 0;

        if (CertGetCertificateContextProperty(
            CertificateContext,
            CERT_SIGN_HASH_CNG_ALG_PROP_ID,
            NULL,
            &dataLength
            ) && dataLength > 0)
        {
            PPH_STRING data = PhCreateStringEx(NULL, dataLength);

            if (CertGetCertificateContextProperty(CertificateContext, CERT_SIGN_HASH_CNG_ALG_PROP_ID, data->Buffer, &dataLength))
                CertificateNode->Algorithm = data;
            else
                PhDereferenceObject(data);
        }
    }

    return TRUE;
}

PCMSG_SIGNER_INFO PvpPeGetSignerInfoIndex(
    _In_ HCRYPTMSG CryptMessageHandle,
    _In_ ULONG Index
    )
{
    ULONG signerInfoLength = 0;
    PCMSG_SIGNER_INFO signerInfo;

    if (!CryptMsgGetParam(
        CryptMessageHandle,
        CMSG_SIGNER_INFO_PARAM,
        Index,
        NULL,
        &signerInfoLength
        ))
    {
        return NULL;
    }

    signerInfo = PhAllocateZero(signerInfoLength);

    if (!CryptMsgGetParam(
        CryptMessageHandle,
        CMSG_SIGNER_INFO_PARAM,
        Index,
        signerInfo,
        &signerInfoLength
        ))
    {
        PhFree(signerInfo);
        return NULL;
    }

    return signerInfo;
}

typedef struct _PV_CERT_ENUM_CONTEXT
{
    PPV_PE_CERTIFICATE_CONTEXT WindowContext;
    PPV_CERTIFICATE_NODE RootNode;
    PV_CERTIFICATE_NODE_TYPE RootNodeType;
} PV_CERT_ENUM_CONTEXT, *PPV_CERT_ENUM_CONTEXT;

BOOLEAN CALLBACK PvpPeEnumSecurityCallback(
    _In_ PCCERT_CONTEXT CertContext,
    _In_ PVOID Context
    )
{
    PPV_CERT_ENUM_CONTEXT context = Context;
    PV_CERT_ENUM_CONTEXT enumContext;

    enumContext.RootNodeType = context->RootNodeType;
    enumContext.WindowContext = context->WindowContext;
    enumContext.RootNode = PvAddChildCertificateNode(
        context->WindowContext,
        context->RootNode,
        context->RootNodeType,
        CertContext
        );

    PhEnumImageCertificates(Context, PvpPeEnumSecurityCallback, CertContext, &enumContext);

    return FALSE;
}

VOID PvpPeEnumerateCounterSignSignatures(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ PCMSG_SIGNER_INFO SignerInfo
    )
{
    HCERTSTORE cryptStoreHandle = NULL;
    HCRYPTMSG cryptMessageHandle = NULL;
    PCCERT_CONTEXT certificateContext = NULL;
    PCMSG_SIGNER_INFO cryptMessageSignerInfo = NULL;
    ULONG certificateEncoding;
    ULONG certificateContentType;
    ULONG certificateFormatType;
    ULONG index = ULONG_MAX;

    for (ULONG i = 0; i < SignerInfo->UnauthAttrs.cAttr; i++)
    {
        // Do we need to support szOID_RSA_counterSign?
        if (PhEqualBytesZ(SignerInfo->UnauthAttrs.rgAttr[i].pszObjId, szOID_RFC3161_counterSign, FALSE))
        {
            index = i;
            break;
        }
    }

    if (index == ULONG_MAX)
        return;

    if (CryptQueryObject(
        CERT_QUERY_OBJECT_BLOB,
        SignerInfo->UnauthAttrs.rgAttr[index].rgValue,
        CERT_QUERY_CONTENT_FLAG_ALL,
        CERT_QUERY_FORMAT_FLAG_ALL,
        0,
        &certificateEncoding,
        &certificateContentType,
        &certificateFormatType,
        &cryptStoreHandle,
        &cryptMessageHandle,
        NULL
        ))
    {
        ULONG signerCount = 0;
        ULONG signerLength = sizeof(ULONG);

        while (certificateContext = CertEnumCertificatesInStore(cryptStoreHandle, certificateContext))
        {
            PV_CERT_ENUM_CONTEXT enumContext;

            enumContext.RootNode = PvAddChildCertificateNode(Context, NULL, PV_CERTIFICATE_NODE_TYPE_NESTED, certificateContext);
            enumContext.RootNodeType = PV_CERTIFICATE_NODE_TYPE_NESTEDARRAY;
            enumContext.WindowContext = Context;

            PhEnumImageCertificates(Context, PvpPeEnumSecurityCallback, certificateContext, &enumContext);
        }

        if (CryptMsgGetParam(cryptMessageHandle, CMSG_SIGNER_COUNT_PARAM, 0, &signerCount, &signerLength))
        {
            for (ULONG i = 0; i < signerCount; i++)
            {
                if (cryptMessageSignerInfo = PvpPeGetSignerInfoIndex(cryptMessageHandle, i))
                {
                    PvpPeEnumerateNestedSignatures(Context, cryptMessageSignerInfo);

                    PvpPeEnumerateCounterSignSignatures(Context, cryptMessageSignerInfo);

                    PhFree(cryptMessageSignerInfo);
                }
            }
        }
    }
}

VOID PvpPeEnumerateNestedSignatures(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context,
    _In_ PCMSG_SIGNER_INFO SignerInfo
    )
{
    HCERTSTORE cryptStoreHandle = NULL;
    HCRYPTMSG cryptMessageHandle = NULL;
    PCCERT_CONTEXT certificateContext = NULL;
    PCMSG_SIGNER_INFO cryptMessageSignerInfo = NULL;
    ULONG certificateEncoding;
    ULONG certificateContentType;
    ULONG certificateFormatType;
    ULONG index = ULONG_MAX;

    for (ULONG i = 0; i < SignerInfo->UnauthAttrs.cAttr; i++)
    {
        if (PhEqualBytesZ(SignerInfo->UnauthAttrs.rgAttr[i].pszObjId, szOID_NESTED_SIGNATURE, FALSE))
        {
            index = i;
            break;
        }
    }

    if (index == ULONG_MAX)
        return;

    if (CryptQueryObject(
        CERT_QUERY_OBJECT_BLOB,
        SignerInfo->UnauthAttrs.rgAttr[index].rgValue,
        CERT_QUERY_CONTENT_FLAG_ALL,
        CERT_QUERY_FORMAT_FLAG_ALL,
        0,
        &certificateEncoding,
        &certificateContentType,
        &certificateFormatType,
        &cryptStoreHandle,
        &cryptMessageHandle,
        NULL
        ))
    {
        ULONG signerCount = 0;
        ULONG signerLength = sizeof(ULONG);

        while (certificateContext = CertEnumCertificatesInStore(cryptStoreHandle, certificateContext))
        {
            PV_CERT_ENUM_CONTEXT enumContext;

            enumContext.RootNode = PvAddChildCertificateNode(Context, NULL, PV_CERTIFICATE_NODE_TYPE_NESTED, certificateContext);
            enumContext.RootNodeType = PV_CERTIFICATE_NODE_TYPE_NESTEDARRAY;
            enumContext.WindowContext = Context;

            PhEnumImageCertificates(Context, PvpPeEnumSecurityCallback, certificateContext, &enumContext);
        }

        if (CryptMsgGetParam(cryptMessageHandle, CMSG_SIGNER_COUNT_PARAM, 0, &signerCount, &signerLength))
        {
            for (ULONG i = 0; i < signerCount; i++)
            {
                if (cryptMessageSignerInfo = PvpPeGetSignerInfoIndex(cryptMessageHandle, i))
                {
                    PvpPeEnumerateNestedSignatures(Context, cryptMessageSignerInfo);

                    PvpPeEnumerateCounterSignSignatures(Context, cryptMessageSignerInfo);

                    PhFree(cryptMessageSignerInfo);
                }
            }
        }

        //if (certificateContext) CertFreeCertificateContext(certificateContext);
        //if (cryptStoreHandle) CertCloseStore(cryptStoreHandle, 0);
        //if (cryptMessageHandle) CryptMsgClose(cryptMessageHandle);
    }
}

VOID PvpPeEnumerateFileCertificates(
    _In_ PPV_PE_CERTIFICATE_CONTEXT Context
    )
{
    PIMAGE_DATA_DIRECTORY dataDirectory;
    HCERTSTORE cryptStoreHandle = NULL;
    PCCERT_CONTEXT certificateContext = NULL;
    HCRYPTMSG cryptMessageHandle = NULL;
    PCMSG_SIGNER_INFO cryptMessageSignerInfo = NULL;
    ULONG certificateDirectoryLength = 0;
    ULONG certificateEncoding;
    ULONG certificateContentType;
    ULONG certificateFormatType;

    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_SECURITY, &dataDirectory)))
    {
        LPWIN_CERTIFICATE certificateDirectory = PTR_ADD_OFFSET(PvMappedImage.ViewBase, dataDirectory->VirtualAddress);
        CERT_BLOB certificateBlob = { certificateDirectory->dwLength, certificateDirectory->bCertificate };

        if (certificateDirectory->wCertificateType == WIN_CERT_TYPE_PKCS_SIGNED_DATA)
        {
            //RTL_FIELD_SIZE(WIN_CERTIFICATE, dwLength) - RTL_FIELD_SIZE(WIN_CERTIFICATE, wRevision) - RTL_FIELD_SIZE(WIN_CERTIFICATE, wCertificateType)
            //CRYPT_DATA_BLOB certificateData = { certificateDirectory->dwLength, certificateDirectory->bCertificate };
            //HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_PKCS7, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, 0, &certificateData);

            CryptQueryObject(
                CERT_QUERY_OBJECT_BLOB,
                &certificateBlob,
                CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                CERT_QUERY_FORMAT_FLAG_BINARY,
                0,
                &certificateEncoding,
                &certificateContentType,
                &certificateFormatType,
                &cryptStoreHandle,
                &cryptMessageHandle,
                NULL
                );
        }

        certificateDirectoryLength = certificateDirectory->dwLength;
    }

    if (!(cryptStoreHandle && cryptMessageHandle))
    {
        CryptQueryObject(
            CERT_QUERY_OBJECT_FILE,
            PhGetString(PvFileName),
            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
            CERT_QUERY_FORMAT_FLAG_BINARY,
            0,
            &certificateEncoding,
            &certificateContentType,
            &certificateFormatType,
            &cryptStoreHandle,
            &cryptMessageHandle,
            NULL
            );
    }

    if (cryptStoreHandle)
    {
        while (certificateContext = CertEnumCertificatesInStore(cryptStoreHandle, certificateContext))
        {
            PV_CERT_ENUM_CONTEXT enumContext;

            enumContext.RootNode = PvAddChildCertificateNode(Context, NULL, PV_CERTIFICATE_NODE_TYPE_IMAGE, certificateContext);
            enumContext.RootNodeType = PV_CERTIFICATE_NODE_TYPE_IMAGEARRAY;
            enumContext.WindowContext = Context;

            PhEnumImageCertificates(Context, PvpPeEnumSecurityCallback, certificateContext, &enumContext);
        }

        //if (certificateContext) CertFreeCertificateContext(certificateContext);
        //if (cryptStoreHandle) CertCloseStore(cryptStoreHandle, 0);
    }

    if (cryptMessageHandle)
    {
        ULONG signerCount = 0;
        ULONG signerLength = sizeof(ULONG);

        if (CryptMsgGetParam(cryptMessageHandle, CMSG_SIGNER_COUNT_PARAM, 0, &signerCount, &signerLength))
        {
            for (ULONG i = 0; i < signerCount; i++)
            {
                if (cryptMessageSignerInfo = PvpPeGetSignerInfoIndex(cryptMessageHandle, i))
                {
                    PvpPeEnumerateNestedSignatures(Context, cryptMessageSignerInfo);

                    PvpPeEnumerateCounterSignSignatures(Context, cryptMessageSignerInfo);

                    PhFree(cryptMessageSignerInfo);
                }
            }
        }

        //if (cryptMessageHandle) CryptMsgClose(cryptMessageHandle);
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);

    if (certificateDirectoryLength)
    {
        PhSetWindowText(Context->LabelHandle, PhaFormatString(
            L"Size: %s (Certs: %s)",
            PhaFormatSize(certificateDirectoryLength, ULONG_MAX)->Buffer,
            PhaFormatSize(Context->TotalSize, ULONG_MAX)->Buffer
            )->Buffer);
        ShowWindow(Context->LabelHandle, SW_SHOW);
    }
}

VOID PvpPeViewCertificateContext(
    _In_ HWND WindowHandle,
    _In_ PCCERT_CONTEXT CertContext
    )
{
    CRYPTUI_VIEWCERTIFICATE_STRUCT cryptViewCertInfo;
    HCERTSTORE certStore = CertContext->hCertStore;

    memset(&cryptViewCertInfo, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));
    cryptViewCertInfo.dwSize = sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
    //cryptViewCertInfo.dwFlags = CRYPTUI_ENABLE_REVOCATION_CHECKING | CRYPTUI_ENABLE_REVOCATION_CHECK_END_CERT;
    cryptViewCertInfo.hwndParent = WindowHandle;
    cryptViewCertInfo.pCertContext = CertContext;
    cryptViewCertInfo.cStores = 1;
    cryptViewCertInfo.rghStores = &certStore;

    if (CryptUIDlgViewCertificate)
    {
#pragma warning(push)
#pragma warning(disable:6387)
        CryptUIDlgViewCertificate(&cryptViewCertInfo, NULL);
#pragma warning(pop)
    }

    //if (CryptUIDlgViewContext)
    //{
    //    CryptUIDlgViewContext(CERT_STORE_CERTIFICATE_CONTEXT, CertContext, WindowHandle, NULL, 0, NULL);
    //}
}

VOID PvpPeSaveCertificateContext(
    _In_ HWND WindowHandle,
    _In_ PCCERT_CONTEXT CertContext
    )
{
    CRYPTUI_WIZ_EXPORT_INFO cryptExportCertInfo;
    HCERTSTORE certStore = CertContext->hCertStore;

    memset(&cryptExportCertInfo, 0, sizeof(CRYPTUI_WIZ_EXPORT_INFO));
    cryptExportCertInfo.dwSize = sizeof(CRYPTUI_WIZ_EXPORT_INFO);
    cryptExportCertInfo.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CERT_CONTEXT;
    cryptExportCertInfo.pCertContext = CertContext;
    cryptExportCertInfo.cStores = 1;
    cryptExportCertInfo.rghStores = &certStore;

    if (CryptUIWizExport)
    {
        CryptUIWizExport(0, WindowHandle, NULL, &cryptExportCertInfo, NULL);
    }
}

INT_PTR CALLBACK PvpPeSecurityDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PE_CERTIFICATE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_PE_CERTIFICATE_CONTEXT));
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
            context->WindowHandle = hwndDlg;
            context->LabelHandle = GetDlgItem(hwndDlg, IDC_NAME);
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);
            context->SearchHandle = GetDlgItem(hwndDlg, IDC_TREESEARCH);
            context->SearchText = PhReferenceEmptyString();

            PvCreateSearchControl(context->SearchHandle, L"Search Certificates (Ctrl+K)");
            PvInitializeCertificateTree(context);
            PvConfigTreeBorders(context->TreeNewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->LabelHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PvpPeEnumerateFileCertificates(context);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PvDeleteCertificateTree(context);

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

                    if (!PhEqualString(context->SearchText, newSearchboxText, FALSE))
                    {
                        PhSwapReference(&context->SearchText, newSearchboxText);

                        if (!PhIsNullOrEmptyString(context->SearchText))
                        {
                            PvExpandAllCertificateNodes(context, TRUE);
                            PvDeselectAllCertificateNodes(context);
                        }

                        PhApplyTreeNewFilters(&context->FilterSupport);
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case WM_PV_CERTIFICATE_PROPERTIES:
                {
                    PPV_CERTIFICATE_NODE node;

                    if (node = PvGetSelectedCertificateNode(context))
                    {
                        PvpPeViewCertificateContext(hwndDlg, node->CertContext);
                    }
                }
                break;
            case WM_PV_CERTIFICATE_CONTEXTMENU:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    PPV_CERTIFICATE_NODE* nodes = NULL;
                    ULONG numberOfNodes = 0;

                    if (!PvGetSelectedCertificateNodes(context, &nodes, &numberOfNodes))
                        break;

                    if (numberOfNodes != 0)
                    {
                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"View certificate...", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"Save certificate...", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
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
                                case 1:
                                    PvpPeViewCertificateContext(hwndDlg, nodes[0]->CertContext);
                                    break;
                                case 2:
                                    PvpPeSaveCertificateContext(hwndDlg, nodes[0]->CertContext);
                                    break;
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
            case IDC_RESET:
                {
                    PvClearCertificateTree(context);
                    context->TotalSize = 0;
                    context->TotalCount = 0;
                    PvpPeEnumerateFileCertificates(context);
                }
                break;
            }
        }
        break;
     case WM_NOTIFY:
         {
             LPNMHDR header = (LPNMHDR)lParam;
             LPPSHNOTIFY pageNotify = (LPPSHNOTIFY)header;

             switch (pageNotify->hdr.code)
             {
             case PSN_QUERYINITIALFOCUS:
                 SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)context->TreeNewHandle);
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
