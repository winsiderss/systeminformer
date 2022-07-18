/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2013
 *     dmex    2017-2022
 *
 */

#include <phapp.h>
#include <hndllist.h>

#include <emenu.h>
#include <secedit.h>
#include <settings.h>

#include <extmgri.h>
#include <hndlprv.h>
#include <phsettings.h>

BOOLEAN PhpHandleNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG PhpHandleNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PhpDestroyHandleNode(
    _In_ PPH_HANDLE_NODE HandleNode
    );

VOID PhpRemoveHandleNode(
    _In_ PPH_HANDLE_NODE HandleNode,
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    );

LONG PhpHandleTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    );

BOOLEAN NTAPI PhpHandleTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

VOID PhInitializeHandleList(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PPH_HANDLE_LIST_CONTEXT Context
    )
{
    HWND hwnd;

    memset(Context, 0, sizeof(PH_HANDLE_LIST_CONTEXT));
    Context->EnableStateHighlighting = TRUE;

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPH_HANDLE_NODE),
        PhpHandleNodeHashtableEqualFunction,
        PhpHandleNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    hwnd = TreeNewHandle;
    PhSetControlTheme(hwnd, L"explorer");

    TreeNew_SetCallback(hwnd, PhpHandleTreeNewCallback, Context);

    TreeNew_SetRedraw(hwnd, FALSE);

    // Default columns
    PhAddTreeNewColumn(hwnd, PHHNTLC_TYPE, TRUE, L"Type", 100, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, PHHNTLC_NAME, TRUE, L"Name", 200, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(hwnd, PHHNTLC_GRANTEDACCESSSYMBOLIC, TRUE, L"Granted access (symbolic)", 140, PH_ALIGN_LEFT, 2, 0);

    PhAddTreeNewColumn(hwnd, PHHNTLC_HANDLE, FALSE, L"Handle", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(hwnd, PHHNTLC_OBJECTADDRESS, FALSE, L"Object address", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumnEx(hwnd, PHHNTLC_ATTRIBUTES, FALSE, L"Attributes", 120, PH_ALIGN_LEFT, ULONG_MAX, 0, TRUE);
    PhAddTreeNewColumn(hwnd, PHHNTLC_GRANTEDACCESS, FALSE, L"Granted access", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(hwnd, PHHNTLC_ORIGINALNAME, FALSE, L"Original name", 200, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumnEx(hwnd, PHHNTLC_FILESHAREACCESS, FALSE, L"File share access", 50, PH_ALIGN_LEFT, ULONG_MAX, 0, TRUE);

    TreeNew_SetRedraw(hwnd, TRUE);

    TreeNew_SetSort(hwnd, 0, AscendingSortOrder);

    PhCmInitializeManager(&Context->Cm, hwnd, PHHNTLC_MAXIMUM, PhpHandleTreeNewPostSortFunction);

    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, hwnd, Context->NodeList);
}

VOID PhDeleteHandleList(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    )
{
    ULONG i;

    PhDeleteTreeNewFilterSupport(&Context->TreeFilterSupport);

    PhCmDeleteManager(&Context->Cm);

    for (i = 0; i < Context->NodeList->Count; i++)
        PhpDestroyHandleNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

BOOLEAN PhpHandleNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_HANDLE_NODE handleNode1 = *(PPH_HANDLE_NODE *)Entry1;
    PPH_HANDLE_NODE handleNode2 = *(PPH_HANDLE_NODE *)Entry2;

    return handleNode1->Handle == handleNode2->Handle;
}

ULONG PhpHandleNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return HandleToUlong((*(PPH_HANDLE_NODE *)Entry)->Handle) / 4;
}

VOID PhLoadSettingsHandleList(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"HandleTreeListColumns");
    sortSettings = PhGetStringSetting(L"HandleTreeListSort");
    Context->Flags = PhGetIntegerSetting(L"HandleTreeListFlags");

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsHandleList(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);

    PhSetIntegerSetting(L"HandleTreeListFlags", Context->Flags);
    PhSetStringSetting2(L"HandleTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"HandleTreeListSort", &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSetOptionsHandleList(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ ULONG Options
    )
{
    switch (Options)
    {
    case PH_HANDLE_TREE_MENUITEM_HIDE_PROTECTED_HANDLES:
        Context->HideProtectedHandles = !Context->HideProtectedHandles;
        break;
    case PH_HANDLE_TREE_MENUITEM_HIDE_INHERIT_HANDLES:
        Context->HideInheritHandles = !Context->HideInheritHandles;
        break;
    case PH_HANDLE_TREE_MENUITEM_HIDE_UNNAMED_HANDLES:
        Context->HideUnnamedHandles = !Context->HideUnnamedHandles;
        break;
    case PH_HANDLE_TREE_MENUITEM_HIDE_ETW_HANDLES:
        Context->HideEtwHandles = !Context->HideEtwHandles;
        break;
    }
}

PPH_HANDLE_NODE PhAddHandleNode(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ PPH_HANDLE_ITEM HandleItem,
    _In_ ULONG RunId
    )
{
    PPH_HANDLE_NODE handleNode;

    handleNode = PhAllocate(PhEmGetObjectSize(EmHandleNodeType, sizeof(PH_HANDLE_NODE)));
    memset(handleNode, 0, sizeof(PH_HANDLE_NODE));
    PhInitializeTreeNewNode(&handleNode->Node);

    if (Context->EnableStateHighlighting && RunId != 1)
    {
        PhChangeShStateTn(
            &handleNode->Node,
            &handleNode->ShState,
            &Context->NodeStateList,
            NewItemState,
            PhCsColorNew,
            NULL
            );
    }

    handleNode->Handle = HandleItem->Handle;
    handleNode->HandleItem = HandleItem;
    PhReferenceObject(HandleItem);

    memset(handleNode->TextCache, 0, sizeof(PH_STRINGREF) * PHHNTLC_MAXIMUM);
    handleNode->Node.TextCache = handleNode->TextCache;
    handleNode->Node.TextCacheSize = PHHNTLC_MAXIMUM;

    PhAddEntryHashtable(Context->NodeHashtable, &handleNode);
    PhAddItemList(Context->NodeList, handleNode);

    if (Context->TreeFilterSupport.FilterList)
        handleNode->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->TreeFilterSupport, &handleNode->Node);

    PhEmCallObjectOperation(EmHandleNodeType, handleNode, EmObjectCreate);

    TreeNew_NodesStructured(Context->TreeNewHandle);

    return handleNode;
}

PPH_HANDLE_NODE PhFindHandleNode(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ HANDLE Handle
    )
{
    PH_HANDLE_NODE lookupHandleNode;
    PPH_HANDLE_NODE lookupHandleNodePtr = &lookupHandleNode;
    PPH_HANDLE_NODE *handleNode;

    lookupHandleNode.Handle = Handle;

    handleNode = (PPH_HANDLE_NODE *)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupHandleNodePtr
        );

    if (handleNode)
        return *handleNode;
    else
        return NULL;
}

VOID PhRemoveHandleNode(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ PPH_HANDLE_NODE HandleNode
    )
{
    // Remove from the hashtable here to avoid problems in case the key is re-used.
    PhRemoveEntryHashtable(Context->NodeHashtable, &HandleNode);

    if (Context->EnableStateHighlighting)
    {
        PhChangeShStateTn(
            &HandleNode->Node,
            &HandleNode->ShState,
            &Context->NodeStateList,
            RemovingItemState,
            PhCsColorRemoved,
            Context->TreeNewHandle
            );
    }
    else
    {
        PhpRemoveHandleNode(HandleNode, Context);
    }
}

VOID PhpDestroyHandleNode(
    _In_ PPH_HANDLE_NODE HandleNode
    )
{
    PhEmCallObjectOperation(EmHandleNodeType, HandleNode, EmObjectDelete);

    if (HandleNode->GrantedAccessSymbolicText) PhDereferenceObject(HandleNode->GrantedAccessSymbolicText);

    PhDereferenceObject(HandleNode->HandleItem);

    PhFree(HandleNode);
}

VOID PhpRemoveHandleNode(
    _In_ PPH_HANDLE_NODE HandleNode,
    _In_ PPH_HANDLE_LIST_CONTEXT Context // PH_TICK_SH_STATE requires this parameter to be after HandleNode
    )
{
    ULONG index;

    // Remove from list and cleanup.

    if ((index = PhFindItemList(Context->NodeList, HandleNode)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);

    PhpDestroyHandleNode(HandleNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhUpdateHandleNode(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ PPH_HANDLE_NODE HandleNode
    )
{
    memset(HandleNode->TextCache, 0, sizeof(PH_STRINGREF) * PHHNTLC_MAXIMUM);

    PhInvalidateTreeNewNode(&HandleNode->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhExpandAllHandleNodes(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ BOOLEAN Expand
    )
{
    ULONG i;
    BOOLEAN needsRestructure = FALSE;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_HANDLE_NODE node = Context->NodeList->Items[i];

        if (node->Node.Expanded != Expand)
        {
            node->Node.Expanded = Expand;
            needsRestructure = TRUE;
        }
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhTickHandleNodes(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    )
{
    PH_TICK_SH_STATE_TN(PH_HANDLE_NODE, ShState, Context->NodeStateList, PhpRemoveHandleNode, PhCsHighlightingDuration, Context->TreeNewHandle, TRUE, NULL, Context);
}

#define SORT_FUNCTION(Column) PhpHandleTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpHandleTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_HANDLE_NODE node1 = *(PPH_HANDLE_NODE *)_elem1; \
    PPH_HANDLE_NODE node2 = *(PPH_HANDLE_NODE *)_elem2; \
    PPH_HANDLE_ITEM handleItem1 = node1->HandleItem; \
    PPH_HANDLE_ITEM handleItem2 = node2->HandleItem; \
    PPH_HANDLE_LIST_CONTEXT context = (PPH_HANDLE_LIST_CONTEXT)_context; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->Handle, (ULONG_PTR)node2->Handle); \
    \
    return PhModifySort(sortResult, context->TreeNewSortOrder); \
}

LONG PhpHandleTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPH_HANDLE_NODE)Node1)->Handle, (ULONG_PTR)((PPH_HANDLE_NODE)Node2)->Handle);

    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = PhCompareStringWithNullSortOrder(handleItem1->TypeName, handleItem2->TypeName, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNullSortOrder(handleItem1->BestObjectName, handleItem2->BestObjectName, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Handle)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Handle, (ULONG_PTR)node2->Handle);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ObjectAddress)
{
    sortResult = uintptrcmp((ULONG_PTR)handleItem1->Object, (ULONG_PTR)handleItem2->Object);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Attributes)
{
    sortResult = uintcmp(handleItem1->Attributes, handleItem2->Attributes);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(GrantedAccess)
{
    sortResult = uintcmp(handleItem1->GrantedAccess, handleItem2->GrantedAccess);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(OriginalName)
{
    sortResult = PhCompareStringWithNull(handleItem1->ObjectName, handleItem2->ObjectName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(FileShareAccess)
{
    sortResult = uintcmp(handleItem1->FileFlags & (PH_HANDLE_FILE_SHARED_MASK), handleItem2->FileFlags & (PH_HANDLE_FILE_SHARED_MASK));

    // Make sure all file handles get grouped together regardless of share access.
    if (sortResult == 0 && !PhIsNullOrEmptyString(handleItem1->TypeName))
        sortResult = intcmp(PhEqualString2(handleItem1->TypeName, L"File", TRUE), PhEqualString2(handleItem2->TypeName, L"File", TRUE));
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpHandleTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLE_LIST_CONTEXT context = Context;
    PPH_HANDLE_NODE node;

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
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Handle),
                    SORT_FUNCTION(ObjectAddress),
                    SORT_FUNCTION(Attributes),
                    SORT_FUNCTION(GrantedAccess),
                    SORT_FUNCTION(GrantedAccess), // Granted Access (Symbolic)
                    SORT_FUNCTION(OriginalName),
                    SORT_FUNCTION(FileShareAccess)
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
                    if (context->TreeNewSortColumn < PHHNTLC_MAXIMUM)
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
            PPH_HANDLE_ITEM handleItem;

            if (!getCellText)
                break;

            node = (PPH_HANDLE_NODE)getCellText->Node;
            handleItem = node->HandleItem;

            switch (getCellText->Id)
            {
            case PHHNTLC_TYPE:
                getCellText->Text = PhGetStringRef(handleItem->TypeName);
                break;
            case PHHNTLC_NAME:
                getCellText->Text = PhGetStringRef(handleItem->BestObjectName);
                break;
            case PHHNTLC_HANDLE:
                PhPrintPointer(handleItem->HandleString, (PVOID)handleItem->Handle);
                PhInitializeStringRefLongHint(&getCellText->Text, handleItem->HandleString);
                break;
            case PHHNTLC_OBJECTADDRESS:
                {
                    if (handleItem->Object)
                        PhPrintPointer(node->ObjectString, handleItem->Object);

                    PhInitializeStringRefLongHint(&getCellText->Text, node->ObjectString);
                }
                break;
            case PHHNTLC_ATTRIBUTES:
                switch (handleItem->Attributes & (OBJ_PROTECT_CLOSE | OBJ_INHERIT))
                {
                case OBJ_PROTECT_CLOSE:
                    PhInitializeStringRef(&getCellText->Text, L"Protected");
                    break;
                case OBJ_INHERIT:
                    PhInitializeStringRef(&getCellText->Text, L"Inherit");
                    break;
                case OBJ_PROTECT_CLOSE | OBJ_INHERIT:
                    PhInitializeStringRef(&getCellText->Text, L"Protected, Inherit");
                    break;
                }
                break;
            case PHHNTLC_GRANTEDACCESS:
                PhPrintPointer(handleItem->GrantedAccessString, UlongToPtr(handleItem->GrantedAccess));
                PhInitializeStringRefLongHint(&getCellText->Text, handleItem->GrantedAccessString);
                break;
            case PHHNTLC_GRANTEDACCESSSYMBOLIC:
                if (handleItem->GrantedAccess != 0)
                {
                    if (!node->GrantedAccessSymbolicText)
                    {
                        PPH_ACCESS_ENTRY accessEntries;
                        ULONG numberOfAccessEntries;

                        if (PhGetAccessEntries(PhGetStringOrEmpty(handleItem->TypeName), &accessEntries, &numberOfAccessEntries))
                        {
                            node->GrantedAccessSymbolicText = PhGetAccessString(handleItem->GrantedAccess, accessEntries, numberOfAccessEntries);
                            PhFree(accessEntries);
                        }
                    }

                    getCellText->Text = PhGetStringRef(node->GrantedAccessSymbolicText);
                }
                break;
            case PHHNTLC_ORIGINALNAME:
                getCellText->Text = PhGetStringRef(handleItem->ObjectName);
                break;
            case PHHNTLC_FILESHAREACCESS:
                if (handleItem->FileFlags & PH_HANDLE_FILE_SHARED_MASK)
                {
                    node->FileShareAccessText[0] = L'-';
                    node->FileShareAccessText[1] = L'-';
                    node->FileShareAccessText[2] = L'-';
                    node->FileShareAccessText[3] = UNICODE_NULL;

                    if (handleItem->FileFlags & PH_HANDLE_FILE_SHARED_READ)
                        node->FileShareAccessText[0] = L'R';
                    if (handleItem->FileFlags & PH_HANDLE_FILE_SHARED_WRITE)
                        node->FileShareAccessText[1] = L'W';
                    if (handleItem->FileFlags & PH_HANDLE_FILE_SHARED_DELETE)
                        node->FileShareAccessText[2] = L'D';

                    PhInitializeStringRefLongHint(&getCellText->Text, node->FileShareAccessText);
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
            PPH_HANDLE_ITEM handleItem;

            if (!getNodeColor)
                break;

            node = (PPH_HANDLE_NODE)getNodeColor->Node;
            handleItem = node->HandleItem;

            if (!handleItem)
                ; // Dummy
            else if (PhCsUseColorProtectedHandles && (handleItem->Attributes & OBJ_PROTECT_CLOSE))
                getNodeColor->BackColor = PhCsColorProtectedHandles;
            else if (PhCsUseColorInheritHandles && (handleItem->Attributes & OBJ_INHERIT))
                getNodeColor->BackColor = PhCsColorInheritHandles;

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
                    SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_HANDLE_COPY, 0);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(context->TreeNewHandle, 0, -1);
                break;
            case VK_DELETE:
                // Pass a 1 in lParam to indicate that warnings should be enabled.
                SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_HANDLE_CLOSE, 1);
                break;
            case VK_RETURN:
                if (GetKeyState(VK_CONTROL) >= 0)
                    SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_HANDLE_PROPERTIES, 0);
                else
                    SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_HANDLE_OBJECTPROPERTIES1, 0);
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
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenu(&data);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_HANDLE_PROPERTIES, 0);
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

PPH_HANDLE_ITEM PhGetSelectedHandleItem(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    )
{
    PPH_HANDLE_ITEM handleItem = NULL;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_HANDLE_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            handleItem = node->HandleItem;
            break;
        }
    }

    return handleItem;
}

VOID PhGetSelectedHandleItems(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _Out_ PPH_HANDLE_ITEM **Handles,
    _Out_ PULONG NumberOfHandles
    )
{
    PH_ARRAY array;
    ULONG i;

    PhInitializeArray(&array, sizeof(PVOID), 2);

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_HANDLE_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            PhAddItemArray(&array, &node->HandleItem);
    }

    *NumberOfHandles = (ULONG)array.Count;
    *Handles = PhFinalArrayItems(&array);
}

VOID PhDeselectAllHandleNodes(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    )
{
    TreeNew_DeselectRange(Context->TreeNewHandle, 0, -1);
}
