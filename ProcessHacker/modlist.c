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
#include <extmgri.h>
#include <phplug.h>
#include <emenu.h>

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

LONG PhpModuleTreeNewPostSortFunction(
    __in LONG Result,
    __in PVOID Node1,
    __in PVOID Node2,
    __in PH_SORT_ORDER SortOrder
    );

BOOLEAN NTAPI PhpModuleTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    );

VOID PhInitializeModuleList(
    __in HWND ParentWindowHandle,
    __in HWND TreeNewHandle,
    __in PPH_PROCESS_ITEM ProcessItem,
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
    Context->TreeNewHandle = TreeNewHandle;
    hwnd = TreeNewHandle;
    PhSetControlTheme(hwnd, L"explorer");

    TreeNew_SetCallback(hwnd, PhpModuleTreeNewCallback, Context);

    TreeNew_SetRedraw(hwnd, FALSE);

    // Default columns
    PhAddTreeNewColumn(hwnd, PHMOTLC_NAME, TRUE, L"Name", 100, PH_ALIGN_LEFT, -2, 0);
    PhAddTreeNewColumn(hwnd, PHMOTLC_BASEADDRESS, TRUE, L"Base Address", 80, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumnEx(hwnd, PHMOTLC_SIZE, TRUE, L"Size", 60, PH_ALIGN_RIGHT, 1, DT_RIGHT, TRUE);
    PhAddTreeNewColumn(hwnd, PHMOTLC_DESCRIPTION, TRUE, L"Description", 160, PH_ALIGN_LEFT, 2, 0);

    PhAddTreeNewColumn(hwnd, PHMOTLC_COMPANYNAME, FALSE, L"Company Name", 180, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHMOTLC_VERSION, FALSE, L"Version", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHMOTLC_FILENAME, FALSE, L"File Name", 180, PH_ALIGN_LEFT, -1, DT_PATH_ELLIPSIS);

    PhAddTreeNewColumn(hwnd, PHMOTLC_TYPE, FALSE, L"Type", 80, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumnEx(hwnd, PHMOTLC_LOADCOUNT, FALSE, L"Load Count", 40, PH_ALIGN_LEFT, -1, 0, TRUE);
    PhAddTreeNewColumn(hwnd, PHMOTLC_VERIFICATIONSTATUS, FALSE, L"Verification Status", 70, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHMOTLC_VERIFIEDSIGNER, FALSE, L"Verified Signer", 100, PH_ALIGN_LEFT, -1, 0);

    TreeNew_SetRedraw(hwnd, TRUE);

    TreeNew_SetTriState(hwnd, TRUE);
    TreeNew_SetSort(hwnd, 0, NoSortOrder);

    PhCmInitializeManager(&Context->Cm, hwnd, PHMOTLC_MAXIMUM, PhpModuleTreeNewPostSortFunction);
}

VOID PhDeleteModuleList(
    __in PPH_MODULE_LIST_CONTEXT Context
    )
{
    ULONG i;

    if (Context->BoldFont)
        DeleteObject(Context->BoldFont);

    PhCmDeleteManager(&Context->Cm);

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

VOID PhLoadSettingsModuleList(
    __inout PPH_MODULE_LIST_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"ModuleTreeListColumns");
    sortSettings = PhGetStringSetting(L"ModuleTreeListSort");
    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsModuleList(
    __inout PPH_MODULE_LIST_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, &sortSettings);
    PhSetStringSetting2(L"ModuleTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"ModuleTreeListSort", &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

PPH_MODULE_NODE PhAddModuleNode(
    __inout PPH_MODULE_LIST_CONTEXT Context,
    __in PPH_MODULE_ITEM ModuleItem,
    __in ULONG RunId
    )
{
    PPH_MODULE_NODE moduleNode;

    moduleNode = PhAllocate(PhEmGetObjectSize(EmModuleNodeType, sizeof(PH_MODULE_NODE)));
    memset(moduleNode, 0, sizeof(PH_MODULE_NODE));
    PhInitializeTreeNewNode(&moduleNode->Node);

    if (Context->EnableStateHighlighting && RunId != 1)
    {
        PhChangeShStateTn(
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

    PhEmCallObjectOperation(EmModuleNodeType, moduleNode, EmObjectCreate);

    TreeNew_NodesStructured(Context->TreeNewHandle);

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
    // Remove from the hashtable here to avoid problems in case the key is re-used.
    PhRemoveEntryHashtable(Context->NodeHashtable, &ModuleNode);

    if (Context->EnableStateHighlighting)
    {
        PhChangeShStateTn(
            &ModuleNode->Node,
            &ModuleNode->ShState,
            &Context->NodeStateList,
            RemovingItemState,
            PhCsColorRemoved,
            Context->TreeNewHandle
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
    PhEmCallObjectOperation(EmModuleNodeType, ModuleNode, EmObjectDelete);

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

    // Remove from list and cleanup.

    if ((index = PhFindItemList(Context->NodeList, ModuleNode)) != -1)
        PhRemoveItemList(Context->NodeList, index);

    PhpDestroyModuleNode(ModuleNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhUpdateModuleNode(
    __in PPH_MODULE_LIST_CONTEXT Context,
    __in PPH_MODULE_NODE ModuleNode
    )
{
    memset(ModuleNode->TextCache, 0, sizeof(PH_STRINGREF) * PHMOTLC_MAXIMUM);
    PhSwapReference(&ModuleNode->TooltipText, NULL);

    ModuleNode->ValidMask = 0;
    PhInvalidateTreeNewNode(&ModuleNode->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhTickModuleNodes(
    __in PPH_MODULE_LIST_CONTEXT Context
    )
{
    PH_TICK_SH_STATE_TN(PH_MODULE_NODE, ShState, Context->NodeStateList, PhpRemoveModuleNode, PhCsHighlightingDuration, Context->TreeNewHandle, TRUE, NULL, Context);
}

#define SORT_FUNCTION(Column) PhpModuleTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpModuleTreeNewCompare##Column( \
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
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)moduleItem1->BaseAddress, (ULONG_PTR)moduleItem2->BaseAddress); \
    \
    return PhModifySort(sortResult, ((PPH_MODULE_LIST_CONTEXT)_context)->TreeNewSortOrder); \
}

LONG PhpModuleTreeNewPostSortFunction(
    __in LONG Result,
    __in PVOID Node1,
    __in PVOID Node2,
    __in PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPH_MODULE_NODE)Node1)->ModuleItem->BaseAddress, (ULONG_PTR)((PPH_MODULE_NODE)Node2)->ModuleItem->BaseAddress);

    return PhModifySort(Result, SortOrder);
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
    sortResult = intcmp(moduleItem1->VerifyResult, moduleItem2->VerifyResult);
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

BOOLEAN NTAPI PhpModuleTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    PPH_MODULE_LIST_CONTEXT context;
    PPH_MODULE_NODE node;

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

                if (context->TreeNewSortOrder == NoSortOrder)
                {
                    sortFunction = SORT_FUNCTION(TriState);
                }
                else
                {
                    if (!PhCmForwardSort(
                        (PPH_TREENEW_NODE *)context->NodeList->Items,
                        context->NodeList->Count,
                        context->TreeNewSortColumn,
                        context->TreeNewSortOrder,
                        &context->Cm
                        ))
                    {
                        if (context->TreeNewSortColumn < PHMOTLC_MAXIMUM)
                            sortFunction = sortFunctions[context->TreeNewSortColumn];
                        else
                            sortFunction = NULL;
                    }
                    else
                    {
                        sortFunction = NULL;
                    }
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
            PPH_MODULE_ITEM moduleItem;

            node = (PPH_MODULE_NODE)getCellText->Node;
            moduleItem = node->ModuleItem;

            switch (getCellText->Id)
            {
            case PHMOTLC_NAME:
                getCellText->Text = moduleItem->Name->sr;
                break;
            case PHMOTLC_BASEADDRESS:
                PhInitializeStringRef(&getCellText->Text, moduleItem->BaseAddressString);
                break;
            case PHMOTLC_SIZE:
                if (!node->SizeText)
                    node->SizeText = PhFormatSize(moduleItem->Size, -1);
                getCellText->Text = PhGetStringRef(node->SizeText);
                break;
            case PHMOTLC_DESCRIPTION:
                getCellText->Text = PhGetStringRef(moduleItem->VersionInfo.FileDescription);
                break;
            case PHMOTLC_COMPANYNAME:
                getCellText->Text = PhGetStringRef(moduleItem->VersionInfo.CompanyName);
                break;
            case PHMOTLC_VERSION:
                getCellText->Text = PhGetStringRef(moduleItem->VersionInfo.FileVersion);
                break;
            case PHMOTLC_FILENAME:
                getCellText->Text = PhGetStringRef(moduleItem->FileName);
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

                    PhInitializeStringRef(&getCellText->Text, typeString);
                }
                break;
            case PHMOTLC_LOADCOUNT:
                if (moduleItem->Type == PH_MODULE_TYPE_MODULE || moduleItem->Type == PH_MODULE_TYPE_KERNEL_MODULE ||
                    moduleItem->Type == PH_MODULE_TYPE_WOW64_MODULE)
                {
                    if (moduleItem->LoadCount != (USHORT)-1)
                    {
                        PhPrintInt32(node->LoadCountString, moduleItem->LoadCount);
                        PhInitializeStringRef(&getCellText->Text, node->LoadCountString);
                    }
                    else
                    {
                        PhInitializeStringRef(&getCellText->Text, L"Static");
                    }
                }
                else
                {
                    PhInitializeEmptyStringRef(&getCellText->Text);
                }
                break;
            case PHMOTLC_VERIFICATIONSTATUS:
                if (moduleItem->Type == PH_MODULE_TYPE_MODULE || moduleItem->Type == PH_MODULE_TYPE_KERNEL_MODULE ||
                    moduleItem->Type == PH_MODULE_TYPE_WOW64_MODULE)
                {
                    PhInitializeStringRef(&getCellText->Text,
                        moduleItem->VerifyResult == VrTrusted ? L"Trusted" : L"Not Trusted");
                }
                else
                {
                    PhInitializeEmptyStringRef(&getCellText->Text);
                }
                break;
            case PHMOTLC_VERIFIEDSIGNER:
                getCellText->Text = PhGetStringRefOrEmpty(moduleItem->VerifySignerName);
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
            PPH_MODULE_ITEM moduleItem;

            node = (PPH_MODULE_NODE)getNodeColor->Node;
            moduleItem = node->ModuleItem;

            if (!moduleItem)
                ; // Dummy
            else if (PhCsUseColorDotNet && (moduleItem->Flags & LDRP_COR_IMAGE))
                getNodeColor->BackColor = PhCsColorDotNet;
            else if (PhCsUseColorRelocatedModules && (moduleItem->Flags & LDRP_IMAGE_NOT_AT_BASE))
                getNodeColor->BackColor = PhCsColorRelocatedModules;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewGetNodeFont:
        {
            PPH_TREENEW_GET_NODE_FONT getNodeFont = Parameter1;

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

                getNodeFont->Font = context->BoldFont ? context->BoldFont : NULL;
                getNodeFont->Flags = TN_CACHE;
                return TRUE;
            }
        }
        break;
    case TreeNewGetCellTooltip:
        {
            PPH_TREENEW_GET_CELL_TOOLTIP getCellTooltip = Parameter1;

            node = (PPH_MODULE_NODE)getCellTooltip->Node;

            if (getCellTooltip->Column->Id != 0)
                return FALSE;

            if (!node->TooltipText)
            {
                node->TooltipText = PhFormatImageVersionInfo(
                    node->ModuleItem->FileName,
                    &node->ModuleItem->VersionInfo,
                    NULL,
                    0
                    );

                // Make sure we don't try to create the tooltip text again.
                if (!node->TooltipText)
                    node->TooltipText = PhReferenceEmptyString();
            }

            if (!PhIsNullOrEmptyString(node->TooltipText))
            {
                getCellTooltip->Text = node->TooltipText->sr;
                getCellTooltip->Unfolding = FALSE;
                getCellTooltip->Font = NULL; // use default font
                getCellTooltip->MaximumWidth = 550;
            }
            else
            {
                return FALSE;
            }
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

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_MODULE_COPY, 0);
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
            PhInitializeTreeNewColumnMenu(&data);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT | PH_EMENU_SHOW_NONOTIFY,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    //case TreeNewLeftDoubleClick:
    //    {
    //        SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_MODULE_PROPERTIES, 0);
    //    }
    //    return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

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
    TreeNew_DeselectRange(Context->TreeNewHandle, 0, -1);
}
