/*
 * Process Hacker - 
 *   module list
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

BOOLEAN PhpModuleNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG PhpModuleNodeHashtableHashFunction(
    __in PVOID Entry
    );

VOID PhpDestroyModuleNode(
    __in PPH_MODULE_NODE ModuleNode
    );

VOID PhpRemoveModuleNode(
    __in PPH_MODULE_NODE ModuleNode,
    __in PPH_MODULE_LIST_CONTEXT Context
    );

BOOLEAN NTAPI PhpModuleTreeListCallback(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    );

VOID PhInitializeModuleList(
    __in HWND ParentWindowHandle,
    __in HWND TreeListHandle,
    __out PPH_MODULE_LIST_CONTEXT Context
    )
{
    HWND hwnd;

    memset(Context, 0, sizeof(PH_MODULE_LIST_CONTEXT));
    Context->EnableStateHighlighting = TRUE;

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPH_MODULE_NODE),
        PhpModuleNodeHashtableCompareFunction,
        PhpModuleNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeListHandle = TreeListHandle;
    hwnd = TreeListHandle;
    TreeList_EnableExplorerStyle(hwnd);

    TreeList_SetCallback(hwnd, PhpModuleTreeListCallback);
    TreeList_SetContext(hwnd, Context);

    // Default columns
    PhAddTreeListColumn(hwnd, PHMOTLC_NAME, TRUE, L"Name", 100, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeListColumn(hwnd, PHMOTLC_BASEADDRESS, TRUE, L"Base Address", 80, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeListColumn(hwnd, PHMOTLC_SIZE, TRUE, L"Size", 60, PH_ALIGN_RIGHT, 2, DT_RIGHT);
    PhAddTreeListColumn(hwnd, PHMOTLC_DESCRIPTION, TRUE, L"Description", 160, PH_ALIGN_LEFT, 3, 0);

    PhAddTreeListColumn(hwnd, PHMOTLC_COMPANYNAME, FALSE, L"Company Name", 180, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeListColumn(hwnd, PHMOTLC_VERSION, FALSE, L"Version", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeListColumn(hwnd, PHMOTLC_FILENAME, FALSE, L"File Name", 180, PH_ALIGN_LEFT, -1, DT_PATH_ELLIPSIS);

    PhAddTreeListColumn(hwnd, PHMOTLC_TYPE, FALSE, L"Type", 80, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeListColumn(hwnd, PHMOTLC_LOADCOUNT, FALSE, L"Load Count", 40, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeListColumn(hwnd, PHMOTLC_VERIFICATIONSTATUS, FALSE, L"Verification Status", 70, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeListColumn(hwnd, PHMOTLC_VERIFIEDSIGNER, FALSE, L"Verified Signer", 100, PH_ALIGN_LEFT, -1, 0);

    TreeList_SetTriState(hwnd, TRUE);
    TreeList_SetSort(hwnd, 0, NoSortOrder);
}

VOID PhDeleteModuleList(
    __in PPH_MODULE_LIST_CONTEXT Context
    )
{
    ULONG i;

    if (Context->BoldFont)
        DeleteObject(Context->BoldFont);

    for (i = 0; i < Context->NodeList->Count; i++)
        PhpDestroyModuleNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

BOOLEAN PhpModuleNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PPH_MODULE_NODE moduleNode1 = *(PPH_MODULE_NODE *)Entry1;
    PPH_MODULE_NODE moduleNode2 = *(PPH_MODULE_NODE *)Entry2;

    return moduleNode1->ModuleItem == moduleNode2->ModuleItem;
}

ULONG PhpModuleNodeHashtableHashFunction(
    __in PVOID Entry
    )
{
#ifdef _M_IX86
    return PhHashInt32((ULONG)(*(PPH_MODULE_NODE *)Entry)->ModuleItem);
#else
    return PhHashInt64((ULONG64)(*(PPH_MODULE_NODE *)Entry)->ModuleItem);
#endif
}

VOID PhLoadSettingsModuleTreeList(
    __inout PPH_MODULE_LIST_CONTEXT Context
    )
{
    PH_INTEGER_PAIR pair;

    PhLoadTreeListColumnsFromSetting(L"ModuleTreeListColumns", Context->TreeListHandle);

    pair = PhGetIntegerPairSetting(L"ModuleTreeListSort");
    TreeList_SetSort(Context->TreeListHandle, pair.X, pair.Y);
}

VOID PhSaveSettingsModuleTreeList(
    __inout PPH_MODULE_LIST_CONTEXT Context
    )
{
    PH_INTEGER_PAIR pair;

    PhSaveTreeListColumnsToSetting(L"ModuleTreeListColumns", Context->TreeListHandle);

    TreeList_GetSort(Context->TreeListHandle, &pair.X, &pair.Y);
    PhSetIntegerPairSetting(L"ModuleTreeListSort", pair);
}

PPH_MODULE_NODE PhAddModuleNode(
    __inout PPH_MODULE_LIST_CONTEXT Context,
    __in PPH_MODULE_ITEM ModuleItem,
    __in ULONG RunId
    )
{
    PPH_MODULE_NODE moduleNode;

    moduleNode = PhAllocate(sizeof(PH_MODULE_NODE));
    memset(moduleNode, 0, sizeof(PH_MODULE_NODE));
    PhInitializeTreeListNode(&moduleNode->Node);

    if (Context->EnableStateHighlighting && RunId != 1)
    {
        PhChangeShState(
            &moduleNode->Node,
            &moduleNode->ShState,
            &Context->NodeStateList,
            NewItemState,
            PhCsColorNew,
            NULL
            );
    }

    moduleNode->ModuleItem = ModuleItem;
    PhReferenceObject(ModuleItem);

    memset(moduleNode->TextCache, 0, sizeof(PH_STRINGREF) * PHMOTLC_MAXIMUM);
    moduleNode->Node.TextCache = moduleNode->TextCache;
    moduleNode->Node.TextCacheSize = PHMOTLC_MAXIMUM;

    PhAddEntryHashtable(Context->NodeHashtable, &moduleNode);
    PhAddItemList(Context->NodeList, moduleNode);

    TreeList_NodesStructured(Context->TreeListHandle);

    return moduleNode;
}

PPH_MODULE_NODE PhFindModuleNode(
    __in PPH_MODULE_LIST_CONTEXT Context,
    __in PPH_MODULE_ITEM ModuleItem
    )
{
    PH_MODULE_NODE lookupModuleNode;
    PPH_MODULE_NODE lookupModuleNodePtr = &lookupModuleNode;
    PPH_MODULE_NODE *moduleNode;

    lookupModuleNode.ModuleItem = ModuleItem;

    moduleNode = (PPH_MODULE_NODE *)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupModuleNodePtr
        );

    if (moduleNode)
        return *moduleNode;
    else
        return NULL;
}

VOID PhRemoveModuleNode(
    __in PPH_MODULE_LIST_CONTEXT Context,
    __in PPH_MODULE_NODE ModuleNode
    )
{
    if (Context->EnableStateHighlighting)
    {
        PhChangeShState(
            &ModuleNode->Node,
            &ModuleNode->ShState,
            &Context->NodeStateList,
            RemovingItemState,
            PhCsColorRemoved,
            Context->TreeListHandle
            );
    }
    else
    {
        PhpRemoveModuleNode(ModuleNode, Context);
    }
}

VOID PhpDestroyModuleNode(
    __in PPH_MODULE_NODE ModuleNode
    )
{
    if (ModuleNode->TooltipText) PhDereferenceObject(ModuleNode->TooltipText);

    if (ModuleNode->SizeText) PhDereferenceObject(ModuleNode->SizeText);

    PhDereferenceObject(ModuleNode->ModuleItem);

    PhFree(ModuleNode);
}

VOID PhpRemoveModuleNode(
    __in PPH_MODULE_NODE ModuleNode,
    __in PPH_MODULE_LIST_CONTEXT Context // PH_TICK_SH_STATE requires this parameter to be after ModuleNode
    )
{
    ULONG index;

    // Remove from hashtable/list and cleanup.

    PhRemoveEntryHashtable(Context->NodeHashtable, &ModuleNode);

    if ((index = PhFindItemList(Context->NodeList, ModuleNode)) != -1)
        PhRemoveItemList(Context->NodeList, index);

    PhpDestroyModuleNode(ModuleNode);

    TreeList_NodesStructured(Context->TreeListHandle);
}

VOID PhUpdateModuleNode(
    __in PPH_MODULE_LIST_CONTEXT Context,
    __in PPH_MODULE_NODE ModuleNode
    )
{
    memset(ModuleNode->TextCache, 0, sizeof(PH_STRINGREF) * PHMOTLC_MAXIMUM);
    PhSwapReference(&ModuleNode->TooltipText, NULL);

    ModuleNode->ValidMask = 0;
    PhInvalidateTreeListNode(&ModuleNode->Node, TLIN_COLOR);
    TreeList_NodesStructured(Context->TreeListHandle);
}

VOID PhTickModuleNodes(
    __in PPH_MODULE_LIST_CONTEXT Context
    )
{
    PH_TICK_SH_STATE(PH_MODULE_NODE, ShState, Context->NodeStateList, PhpRemoveModuleNode, PhCsHighlightingDuration, Context->TreeListHandle, TRUE, Context);
}

#define SORT_FUNCTION(Column) PhpModuleTreeListCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpModuleTreeListCompare##Column( \
    __in void *_context, \
    __in const void *_elem1, \
    __in const void *_elem2 \
    ) \
{ \
    PPH_MODULE_NODE node1 = *(PPH_MODULE_NODE *)_elem1; \
    PPH_MODULE_NODE node2 = *(PPH_MODULE_NODE *)_elem2; \
    PPH_MODULE_ITEM moduleItem1 = node1->ModuleItem; \
    PPH_MODULE_ITEM moduleItem2 = node2->ModuleItem; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    return PhModifySort(sortResult, ((PPH_MODULE_LIST_CONTEXT)_context)->TreeListSortOrder); \
}

BEGIN_SORT_FUNCTION(TriState)
{
    if (moduleItem1->IsFirst)
    {
        sortResult = -1;
    }
    else if (moduleItem2->IsFirst)
    {
        sortResult = 1;
    }
    else
    {
        sortResult = PhCompareString(moduleItem1->Name, moduleItem2->Name, TRUE); // fall back to sorting by name

        if (sortResult == 0)
            sortResult = uintptrcmp((ULONG_PTR)moduleItem1->BaseAddress, (ULONG_PTR)moduleItem2->BaseAddress); // last resort: sort by base address
    }
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareString(moduleItem1->Name, moduleItem2->Name, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(BaseAddress)
{
    sortResult = uintptrcmp((ULONG_PTR)moduleItem1->BaseAddress, (ULONG_PTR)moduleItem2->BaseAddress);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Size)
{
    sortResult = uintcmp(moduleItem1->Size, moduleItem2->Size);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Description)
{
    sortResult = PhCompareStringWithNull(moduleItem1->VersionInfo.FileDescription, moduleItem2->VersionInfo.FileDescription, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(CompanyName)
{
    sortResult = PhCompareStringWithNull(moduleItem1->VersionInfo.CompanyName, moduleItem2->VersionInfo.CompanyName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Version)
{
    sortResult = PhCompareStringWithNull(moduleItem1->VersionInfo.FileVersion, moduleItem2->VersionInfo.FileVersion, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(FileName)
{
    sortResult = PhCompareStringWithNull(moduleItem1->FileName, moduleItem2->FileName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = uintcmp(moduleItem1->Type, moduleItem2->Type);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LoadCount)
{
    sortResult = uintcmp(moduleItem1->LoadCount, moduleItem2->LoadCount);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(VerificationStatus)
{
    sortResult = intcmp(moduleItem1->VerifyResult == VrTrusted, moduleItem2->VerifyResult == VrTrusted);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(VerifiedSigner)
{
    sortResult = PhCompareStringWithNull(
        moduleItem1->VerifySignerName,
        moduleItem2->VerifySignerName,
        TRUE
        );
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpModuleTreeListCallback(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    PPH_MODULE_LIST_CONTEXT context;
    PPH_MODULE_NODE node;

    context = Context;

    switch (Message)
    {
    case TreeListGetChildren:
        {
            PPH_TREELIST_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(BaseAddress),
                    SORT_FUNCTION(Size),
                    SORT_FUNCTION(Description),
                    SORT_FUNCTION(CompanyName),
                    SORT_FUNCTION(Version),
                    SORT_FUNCTION(FileName),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(LoadCount),
                    SORT_FUNCTION(VerificationStatus),
                    SORT_FUNCTION(VerifiedSigner)
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                if (context->TreeListSortOrder == NoSortOrder)
                    sortFunction = SORT_FUNCTION(TriState);
                else if (context->TreeListSortColumn < PHMOTLC_MAXIMUM)
                    sortFunction = sortFunctions[context->TreeListSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                }

                getChildren->Children = (PPH_TREELIST_NODE *)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;
    case TreeListIsLeaf:
        {
            PPH_TREELIST_IS_LEAF isLeaf = Parameter1;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeListGetNodeText:
        {
            PPH_TREELIST_GET_NODE_TEXT getNodeText = Parameter1;
            PPH_MODULE_ITEM moduleItem;

            node = (PPH_MODULE_NODE)getNodeText->Node;
            moduleItem = node->ModuleItem;

            switch (getNodeText->Id)
            {
            case PHMOTLC_NAME:
                getNodeText->Text = moduleItem->Name->sr;
                break;
            case PHMOTLC_BASEADDRESS:
                PhInitializeStringRef(&getNodeText->Text, moduleItem->BaseAddressString);
                break;
            case PHMOTLC_SIZE:
                if (!node->SizeText)
                    node->SizeText = PhFormatSize(moduleItem->Size, -1);
                getNodeText->Text = PhGetStringRef(node->SizeText);
                break;
            case PHMOTLC_DESCRIPTION:
                getNodeText->Text = PhGetStringRef(moduleItem->VersionInfo.FileDescription);
                break;
            case PHMOTLC_COMPANYNAME:
                getNodeText->Text = PhGetStringRef(moduleItem->VersionInfo.CompanyName);
                break;
            case PHMOTLC_VERSION:
                getNodeText->Text = PhGetStringRef(moduleItem->VersionInfo.FileVersion);
                break;
            case PHMOTLC_FILENAME:
                getNodeText->Text = PhGetStringRef(moduleItem->FileName);
                break;
            case PHMOTLC_TYPE:
                {
                    PWSTR typeString;

                    switch (moduleItem->Type)
                    {
                    case PH_MODULE_TYPE_MODULE:
                        typeString = L"DLL";
                        break;
                    case PH_MODULE_TYPE_MAPPED_FILE:
                        typeString = L"Mapped File";
                        break;
                    case PH_MODULE_TYPE_MAPPED_IMAGE:
                        typeString = L"Mapped Image";
                        break;
                    case PH_MODULE_TYPE_WOW64_MODULE:
                        typeString = L"WOW64 DLL";
                        break;
                    case PH_MODULE_TYPE_KERNEL_MODULE:
                        typeString = L"Kernel Module";
                        break;
                    default:
                        typeString = L"Unknown";
                        break;
                    }

                    PhInitializeStringRef(&getNodeText->Text, typeString);
                }
                break;
            case PHMOTLC_LOADCOUNT:
                if (moduleItem->Type == PH_MODULE_TYPE_MODULE || moduleItem->Type == PH_MODULE_TYPE_KERNEL_MODULE ||
                    moduleItem->Type == PH_MODULE_TYPE_WOW64_MODULE)
                {
                    if (moduleItem->LoadCount != (USHORT)-1)
                    {
                        PhPrintInt32(node->LoadCountString, moduleItem->LoadCount);
                        PhInitializeStringRef(&getNodeText->Text, node->LoadCountString);
                    }
                    else
                    {
                        PhInitializeStringRef(&getNodeText->Text, L"Static");
                    }
                }
                else
                {
                    PhInitializeEmptyStringRef(&getNodeText->Text);
                }
                break;
            case PHMOTLC_VERIFICATIONSTATUS:
                if (moduleItem->Type == PH_MODULE_TYPE_MODULE || moduleItem->Type == PH_MODULE_TYPE_KERNEL_MODULE ||
                    moduleItem->Type == PH_MODULE_TYPE_WOW64_MODULE)
                {
                    PhInitializeStringRef(&getNodeText->Text,
                        moduleItem->VerifyResult == VrTrusted ? L"Trusted" : L"Not Trusted");
                }
                else
                {
                    PhInitializeEmptyStringRef(&getNodeText->Text);
                }
                break;
            case PHMOTLC_VERIFIEDSIGNER:
                getNodeText->Text = PhGetStringRefOrEmpty(moduleItem->VerifySignerName);
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
            PPH_MODULE_ITEM moduleItem;

            node = (PPH_MODULE_NODE)getNodeColor->Node;
            moduleItem = node->ModuleItem;

            if (!moduleItem)
                ; // Dummy
            else if (PhCsUseColorDotNet && (moduleItem->Flags & LDRP_COR_IMAGE))
                getNodeColor->BackColor = PhCsColorDotNet;
            else if (PhCsUseColorRelocatedModules && (moduleItem->Flags & LDRP_IMAGE_NOT_AT_BASE))
                getNodeColor->BackColor = PhCsColorRelocatedModules;

            getNodeColor->Flags = TLC_CACHE | TLGNC_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeListGetNodeFont:
        {
            PPH_TREELIST_GET_NODE_FONT getNodeFont = Parameter1;

            node = (PPH_MODULE_NODE)getNodeFont->Node;

            // Make the executable file module item bold.
            if (node->ModuleItem->IsFirst)
            {
                if (!context->BoldFont)
                {
                    LOGFONT logFont;

                    if (GetObject((HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0), sizeof(LOGFONT), &logFont))
                    {
                        logFont.lfWeight = FW_BOLD;
                        context->BoldFont = CreateFontIndirect(&logFont);
                    }
                }

                getNodeFont->Font = context->BoldFont ? context->BoldFont : PhBoldListViewFont;
                getNodeFont->Flags = TLC_CACHE;
                return TRUE;
            }
        }
        break;
    case TreeListGetNodeTooltip:
        {
            PPH_TREELIST_GET_NODE_TOOLTIP getNodeTooltip = Parameter1;

            node = (PPH_MODULE_NODE)getNodeTooltip->Node;

            if (!node->TooltipText)
            {
                node->TooltipText = PhFormatImageVersionInfo(
                    node->ModuleItem->FileName,
                    &node->ModuleItem->VersionInfo,
                    NULL,
                    80
                    );

                // Make sure we don't try to create the tooltip text again.
                if (!node->TooltipText)
                    node->TooltipText = PhReferenceEmptyString();
            }

            if (!PhIsNullOrEmptyString(node->TooltipText))
                getNodeTooltip->Text = node->TooltipText->sr;
            else
                return FALSE;
        }
        return TRUE;
    case TreeListSortChanged:
        {
            TreeList_GetSort(hwnd, &context->TreeListSortColumn, &context->TreeListSortOrder);
            // Force a rebuild to sort the items.
            TreeList_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeListKeyDown:
        {
            switch ((SHORT)Parameter1)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_MODULE_COPY, 0);
                break;
            }
        }
        return TRUE;
    case TreeListHeaderRightClick:
        {
            HMENU menu;
            HMENU subMenu;
            POINT point;

            menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_GENERICHEADER));
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
                PhShowChooseColumnsDialog(hwnd, hwnd);

                // Make sure the column we're sorting by is actually visible, 
                // and if not, don't sort any more.
                if (context->TreeListSortOrder != NoSortOrder)
                {
                    PH_TREELIST_COLUMN column;

                    column.Id = context->TreeListSortColumn;
                    TreeList_GetColumn(hwnd, &column);

                    if (!column.Visible)
                        TreeList_SetSort(hwnd, 0, AscendingSortOrder);
                }
            }

            DestroyMenu(menu);
        }
        return TRUE;
    //case TreeListLeftDoubleClick:
    //    {
    //        SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_MODULE_PROPERTIES, 0);
    //    }
    //    return TRUE;
    case TreeListContextMenu:
        {
            PPH_TREELIST_MOUSE_EVENT mouseEvent = Parameter2;

            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_SHOWCONTEXTMENU, MAKELONG(mouseEvent->Location.x, mouseEvent->Location.y));
        }
        return TRUE;
    }

    return FALSE;
}

PPH_MODULE_ITEM PhGetSelectedModuleItem(
    __in PPH_MODULE_LIST_CONTEXT Context
    )
{
    PPH_MODULE_ITEM moduleItem = NULL;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_MODULE_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            moduleItem = node->ModuleItem;
            break;
        }
    }

    return moduleItem;
}

VOID PhGetSelectedModuleItems(
    __in PPH_MODULE_LIST_CONTEXT Context,
    __out PPH_MODULE_ITEM **Modules,
    __out PULONG NumberOfModules
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_MODULE_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node->ModuleItem);
        }
    }

    *Modules = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfModules = list->Count;

    PhDereferenceObject(list);
}

VOID PhDeselectAllModuleNodes(
    __in PPH_MODULE_LIST_CONTEXT Context
    )
{
    TreeList_SetStateAll(Context->TreeListHandle, 0, LVIS_SELECTED);
    InvalidateRect(Context->TreeListHandle, NULL, TRUE);
}
