/*
 * Process Hacker Firewall Monitor -
 *   firewall events tab
 *
 * Copyright (C) 2012 wj32
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

#include "fwmon.h"
#include "resource.h"
#include <colmgr.h>
#include "fwtabp.h"

static BOOLEAN FwTreeNewCreated = FALSE;
static HWND FwTreeNewHandle;
static ULONG FwTreeNewSortColumn;
static PH_SORT_ORDER FwTreeNewSortOrder;

static PPH_HASHTABLE FwNodeHashtable; // hashtable of all nodes
static PPH_LIST FwNodeList; // list of all nodes

static PH_CALLBACK_REGISTRATION FwItemAddedRegistration;
static PH_CALLBACK_REGISTRATION FwItemModifiedRegistration;
static PH_CALLBACK_REGISTRATION FwItemRemovedRegistration;
static PH_CALLBACK_REGISTRATION FwItemsUpdatedRegistration;
static BOOLEAN FwNeedsRedraw = FALSE;

VOID InitializeFwTab(
    VOID
    )
{
    PH_ADDITIONAL_TAB_PAGE tabPage;

    memset(&tabPage, 0, sizeof(PH_ADDITIONAL_TAB_PAGE));
    tabPage.Text = L"Firewall";
    tabPage.CreateFunction = FwTabCreateFunction;
    tabPage.Index = MAXINT;
    tabPage.SelectionChangedCallback = FwTabSelectionChangedCallback;
    tabPage.SaveContentCallback = FwTabSaveContentCallback;
    tabPage.FontChangedCallback = FwTabFontChangedCallback;
    ProcessHacker_AddTabPage(PhMainWndHandle, &tabPage);
}

HWND NTAPI FwTabCreateFunction(
    __in PVOID Context
    )
{
    HWND hwnd;

    hwnd = CreateWindow(
        PH_TREENEW_CLASSNAME,
        NULL,
        WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED,
        0,
        0,
        3,
        3,
        PhMainWndHandle,
        (HMENU)PhPluginReserveIds(1),
        PluginInstance->DllBase,
        NULL
        );

    if (!hwnd)
        return NULL;

    FwTreeNewCreated = TRUE;

    FwNodeHashtable = PhCreateHashtable(
        sizeof(PFW_EVENT_NODE),
        FwNodeHashtableCompareFunction,
        FwNodeHashtableHashFunction,
        100
        );
    FwNodeList = PhCreateList(100);

    InitializeFwTreeList(hwnd);

    PhRegisterCallback(
        &FwItemAddedEvent,
        FwItemAddedHandler,
        NULL,
        &FwItemAddedRegistration
        );
    PhRegisterCallback(
        &FwItemModifiedEvent,
        FwItemModifiedHandler,
        NULL,
        &FwItemModifiedRegistration
        );
    PhRegisterCallback(
        &FwItemRemovedEvent,
        FwItemRemovedHandler,
        NULL,
        &FwItemRemovedRegistration
        );
    PhRegisterCallback(
        &FwItemsUpdatedEvent,
        FwItemsUpdatedHandler,
        NULL,
        &FwItemsUpdatedRegistration
        );

    return hwnd;
}

VOID NTAPI FwTabSelectionChangedCallback(
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Parameter3,
    __in PVOID Context
    )
{
    if ((BOOLEAN)Parameter1)
    {
        SetFocus(FwTreeNewHandle);
    }
}

VOID NTAPI FwTabSaveContentCallback(
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Parameter3,
    __in PVOID Context
    )
{
    PPH_FILE_STREAM fileStream = Parameter1;
    ULONG mode = PtrToUlong(Parameter2);

    WriteFwList(fileStream, mode);
}

VOID NTAPI FwTabFontChangedCallback(
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Parameter3,
    __in PVOID Context
    )
{
    if (FwTreeNewHandle)
        SendMessage(FwTreeNewHandle, WM_SETFONT, (WPARAM)Parameter1, TRUE);
}

BOOLEAN FwNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PFW_EVENT_NODE FwNode1 = *(PFW_EVENT_NODE *)Entry1;
    PFW_EVENT_NODE FwNode2 = *(PFW_EVENT_NODE *)Entry2;

    return FwNode1->EventItem == FwNode2->EventItem;
}

ULONG FwNodeHashtableHashFunction(
    __in PVOID Entry
    )
{
#ifdef _M_IX86
    return PhHashInt32((ULONG)(*(PFW_EVENT_NODE *)Entry)->EventItem);
#else
    return PhHashInt64((ULONG64)(*(PFW_EVENT_NODE *)Entry)->EventItem);
#endif
}

VOID InitializeFwTreeList(
    __in HWND hwnd
    )
{
    FwTreeNewHandle = hwnd;
    PhSetControlTheme(FwTreeNewHandle, L"explorer");
    SendMessage(TreeNew_GetTooltips(FwTreeNewHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, 0x7fff);

    TreeNew_SetCallback(hwnd, FwTreeNewCallback, NULL);

    TreeNew_SetRedraw(hwnd, FALSE);

    // Default columns
    PhAddTreeNewColumnEx(hwnd, FWTNC_TIME, TRUE, L"Time", 100, PH_ALIGN_LEFT, 0, 0, TRUE);
    PhAddTreeNewColumn(hwnd, FWTNC_PROCESS, TRUE, L"Process", 200, PH_ALIGN_LEFT, 1, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumn(hwnd, FWTNC_USER, TRUE, L"User", 120, PH_ALIGN_LEFT, 2, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumnEx(hwnd, FWTNC_LOCALADDRESS, TRUE, L"Local Address", 140, PH_ALIGN_RIGHT, 3, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(hwnd, FWTNC_LOCALPORT, TRUE, L"Local Address", 70, PH_ALIGN_RIGHT, 4, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(hwnd, FWTNC_REMOTEADDRESS, TRUE, L"Local Address", 140, PH_ALIGN_RIGHT, 5, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(hwnd, FWTNC_REMOTEPORT, TRUE, L"Local Address", 70, PH_ALIGN_RIGHT, 6, DT_RIGHT, TRUE);
    PhAddTreeNewColumn(hwnd, FWTNC_PROTOCOL, TRUE, L"Protocol", 100, PH_ALIGN_LEFT, 7, 0);

    TreeNew_SetRedraw(hwnd, TRUE);

    TreeNew_SetSort(hwnd, FWTNC_TIME, DescendingSortOrder);

    LoadSettingsFwTreeList();
}

VOID LoadSettingsFwTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;

    settings = PhGetStringSetting(SETTING_NAME_FW_TREE_LIST_COLUMNS);
    PhCmLoadSettings(FwTreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);

    sortSettings = PhGetIntegerPairSetting(SETTING_NAME_FW_TREE_LIST_SORT);
    TreeNew_SetSort(FwTreeNewHandle, (ULONG)sortSettings.X, (PH_SORT_ORDER)sortSettings.Y);
}

VOID SaveSettingsFwTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    if (!FwTreeNewCreated)
        return;

    settings = PhCmSaveSettings(FwTreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_FW_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    TreeNew_GetSort(FwTreeNewHandle, &sortColumn, &sortOrder);
    sortSettings.X = sortColumn;
    sortSettings.Y = sortOrder;
    PhSetIntegerPairSetting(SETTING_NAME_FW_TREE_LIST_SORT, sortSettings);
}

PFW_EVENT_NODE AddFwNode(
    __in PFW_EVENT_ITEM FwItem
    )
{
    PFW_EVENT_NODE FwNode;

    FwNode = PhAllocate(sizeof(FW_EVENT_NODE));
    memset(FwNode, 0, sizeof(FW_EVENT_NODE));
    PhInitializeTreeNewNode(&FwNode->Node);

    FwNode->EventItem = FwItem;
    PhReferenceObject(FwItem);

    memset(FwNode->TextCache, 0, sizeof(PH_STRINGREF) * FWTNC_MAXIMUM);
    FwNode->Node.TextCache = FwNode->TextCache;
    FwNode->Node.TextCacheSize = FWTNC_MAXIMUM;

    PhAddEntryHashtable(FwNodeHashtable, &FwNode);
    PhAddItemList(FwNodeList, FwNode);

    TreeNew_NodesStructured(FwTreeNewHandle);

    return FwNode;
}

PFW_EVENT_NODE FindFwNode(
    __in PFW_EVENT_ITEM FwItem
    )
{
    FW_EVENT_NODE lookupFwNode;
    PFW_EVENT_NODE lookupFwNodePtr = &lookupFwNode;
    PFW_EVENT_NODE *FwNode;

    lookupFwNode.EventItem = FwItem;

    FwNode = (PFW_EVENT_NODE *)PhFindEntryHashtable(
        FwNodeHashtable,
        &lookupFwNodePtr
        );

    if (FwNode)
        return *FwNode;
    else
        return NULL;
}

VOID RemoveFwNode(
    __in PFW_EVENT_NODE FwNode
    )
{
    ULONG index;

    // Remove from the hashtable/list and cleanup.

    PhRemoveEntryHashtable(FwNodeHashtable, &FwNode);

    if ((index = PhFindItemList(FwNodeList, FwNode)) != -1)
        PhRemoveItemList(FwNodeList, index);

    if (FwNode->TooltipText) PhDereferenceObject(FwNode->TooltipText);

    PhDereferenceObject(FwNode->EventItem);

    PhFree(FwNode);

    TreeNew_NodesStructured(FwTreeNewHandle);
}

VOID UpdateFwNode(
    __in PFW_EVENT_NODE FwNode
    )
{
    memset(FwNode->TextCache, 0, sizeof(PH_STRINGREF) * FWTNC_MAXIMUM);

    PhInvalidateTreeNewNode(&FwNode->Node, TN_CACHE_ICON);
    TreeNew_NodesStructured(FwTreeNewHandle);
}

#define SORT_FUNCTION(Column) FwTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl FwTreeNewCompare##Column( \
    __in const void *_elem1, \
    __in const void *_elem2 \
    ) \
{ \
    PFW_EVENT_NODE node1 = *(PFW_EVENT_NODE *)_elem1; \
    PFW_EVENT_NODE node2 = *(PFW_EVENT_NODE *)_elem2; \
    PFW_EVENT_ITEM fwItem1 = node1->EventItem; \
    PFW_EVENT_ITEM fwItem2 = node2->EventItem; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uint64cmp(fwItem1->Time.QuadPart, fwItem2->Time.QuadPart); \
    \
    return PhModifySort(sortResult, FwTreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Time)
{
    sortResult = uint64cmp(fwItem1->Time.QuadPart, fwItem2->Time.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Process)
{
    //sortResult = PhCompareString(node1->?, node2->?, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(User)
{
    //sortResult = PhCompareString(fwItem1->?, fwItem2->?, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalAddress)
{
    //sortResult = PhCompareString(fwItem1->?, fwItem2->?, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalPort)
{
    //sortResult = PhCompareString(fwItem1->?, fwItem2->?, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemoteAddress)
{
    //sortResult = PhCompareString(fwItem1->?, fwItem2->?, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemotePort)
{
    //sortResult = PhCompareString(fwItem1->?, fwItem2->?, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Protocol)
{
    //sortResult = PhCompareString(fwItem1->?, fwItem2->?, TRUE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI FwTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    PFW_EVENT_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Time),
                    SORT_FUNCTION(Process),
                    SORT_FUNCTION(User),
                    SORT_FUNCTION(LocalAddress),
                    SORT_FUNCTION(LocalPort),
                    SORT_FUNCTION(RemoteAddress),
                    SORT_FUNCTION(RemotePort),
                    SORT_FUNCTION(Protocol)
                };
                int (__cdecl *sortFunction)(const void *, const void *);

                if (FwTreeNewSortColumn < FWTNC_MAXIMUM)
                    sortFunction = sortFunctions[FwTreeNewSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    qsort(FwNodeList->Items, FwNodeList->Count, sizeof(PVOID), sortFunction);
                }

                getChildren->Children = (PPH_TREENEW_NODE *)FwNodeList->Items;
                getChildren->NumberOfChildren = FwNodeList->Count;
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
            PFW_EVENT_ITEM fwItem;

            node = (PFW_EVENT_NODE)getCellText->Node;
            fwItem = node->EventItem;

            switch (getCellText->Id)
            {
            case FWTNC_TIME:
                //getCellText->Text = node->Time?;
                break;
            case FWTNC_PROCESS:
                //getCellText->Text = ??;
                break;
            case FWTNC_USER:
                //getCellText->Text = ??;
                break;
            case FWTNC_LOCALADDRESS:
                //getCellText->Text = ??;
                break;
            case FWTNC_LOCALPORT:
                //getCellText->Text = ??;
                break;
            case FWTNC_REMOTEADDRESS:
                //getCellText->Text = ??;
                break;
            case FWTNC_REMOTEPORT:
                //getCellText->Text = ??;
                break;
            case FWTNC_PROTOCOL:
                //getCellText->Text = ??;
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    //case TreeNewGetNodeIcon:
    //    {
    //        PPH_TREENEW_GET_NODE_ICON getNodeIcon = Parameter1;

    //        node = (PFW_EVENT_NODE)getNodeIcon->Node;

    //        if (node->FwItem->ProcessIcon)
    //        {
    //            getNodeIcon->Icon = node->FwItem->ProcessIcon->Icon;
    //        }
    //        else
    //        {
    //            PhGetStockApplicationIcon(&getNodeIcon->Icon, NULL);
    //        }

    //        getNodeIcon->Flags = TN_CACHE;
    //    }
    //    return TRUE;
    case TreeNewGetCellTooltip:
        {
            PPH_TREENEW_GET_CELL_TOOLTIP getCellTooltip = Parameter1;

            node = (PFW_EVENT_NODE)getCellTooltip->Node;

            if (getCellTooltip->Column->Id != 0)
                return FALSE;

            if (!node->TooltipText)
            {
                // TODO
            }
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &FwTreeNewSortColumn, &FwTreeNewSortOrder);
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
                    HandleFwCommand(ID_EVENT_COPY);
                break;
            case 'A':
                TreeNew_SelectRange(FwTreeNewHandle, 0, -1);
                break;
            case VK_RETURN:
                //EtHandleDiskCommand(ID_EVENT_?);
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

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT | PH_EMENU_SHOW_NONOTIFY,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            //HandleFwCommand(ID_EVENT_?);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

            ShowFwContextMenu(mouseEvent->Location);
        }
        return TRUE;
    case TreeNewDestroying:
        {
            SaveSettingsFwTreeList();
        }
        return TRUE;
    }

    return FALSE;
}

PFW_EVENT_ITEM GetSelectedFwItem(
    VOID
    )
{
    PFW_EVENT_ITEM FwItem = NULL;
    ULONG i;

    for (i = 0; i < FwNodeList->Count; i++)
    {
        PFW_EVENT_NODE node = FwNodeList->Items[i];

        if (node->Node.Selected)
        {
            FwItem = node->EventItem;
            break;
        }
    }

    return FwItem;
}

VOID GetSelectedFwItems(
    __out PFW_EVENT_ITEM **FwItems,
    __out PULONG NumberOfFwItems
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < FwNodeList->Count; i++)
    {
        PFW_EVENT_NODE node = FwNodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node->EventItem);
        }
    }

    *FwItems = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfFwItems = list->Count;

    PhDereferenceObject(list);
}

VOID DeselectAllFwNodes(
    VOID
    )
{
    TreeNew_DeselectRange(FwTreeNewHandle, 0, -1);
}

VOID SelectAndEnsureVisibleFwNode(
    __in PFW_EVENT_NODE FwNode
    )
{
    DeselectAllFwNodes();

    if (!FwNode->Node.Visible)
        return;

    TreeNew_SetFocusNode(FwTreeNewHandle, &FwNode->Node);
    TreeNew_SetMarkNode(FwTreeNewHandle, &FwNode->Node);
    TreeNew_SelectRange(FwTreeNewHandle, FwNode->Node.Index, FwNode->Node.Index);
    TreeNew_EnsureVisible(FwTreeNewHandle, &FwNode->Node);
}

VOID CopyFwList(
    VOID
    )
{
    PPH_STRING text;

    text = PhGetTreeNewText(FwTreeNewHandle, 0);
    PhSetClipboardStringEx(FwTreeNewHandle, text->Buffer, text->Length);
    PhDereferenceObject(text);
}

VOID WriteFwList(
    __inout PPH_FILE_STREAM FileStream,
    __in ULONG Mode
    )
{
    PPH_LIST lines;
    ULONG i;

    lines = PhGetGenericTreeNewLines(FwTreeNewHandle, Mode);

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

VOID HandleFwCommand(
    __in ULONG Id
    )
{
    switch (Id)
    {
        // Handle commands
    }
}

VOID InitializeFwMenu(
    __in PPH_EMENU Menu,
    __in PFW_EVENT_ITEM *FwItems,
    __in ULONG NumberOfFwItems
    )
{
    PPH_EMENU_ITEM item;

    if (NumberOfFwItems == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfFwItems == 1)
    {
        // Stuff
        item = PhFindEMenuItem(Menu, 0, L"?", 0);

        // Stuff
        item->Flags |= 0;
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_EVENT_COPY, TRUE);
    }
}

VOID ShowFwContextMenu(
    __in POINT Location
    )
{
    PFW_EVENT_ITEM *fwItems;
    ULONG numberOfFwItems;

    GetSelectedFwItems(&fwItems, &numberOfFwItems);

    if (numberOfFwItems != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PluginInstance->DllBase, MAKEINTRESOURCE(IDR_FW), 0);
        //PhSetFlagsEMenuItem(menu, ID_EVENT_?, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        InitializeFwMenu(menu, fwItems, numberOfFwItems);

        item = PhShowEMenu(
            menu,
            PhMainWndHandle,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            Location.x,
            Location.y
            );

        if (item)
        {
            HandleFwCommand(item->Id);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(fwItems);
}

static VOID NTAPI FwItemAddedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PFW_EVENT_ITEM fwItem = (PFW_EVENT_ITEM)Parameter;

    PhReferenceObject(fwItem);
    ProcessHacker_Invoke(PhMainWndHandle, OnFwItemAdded, fwItem);
}

static VOID NTAPI FwItemModifiedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    ProcessHacker_Invoke(PhMainWndHandle, OnFwItemModified, (PFW_EVENT_ITEM)Parameter);
}

static VOID NTAPI FwItemRemovedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    ProcessHacker_Invoke(PhMainWndHandle, OnFwItemRemoved, (PFW_EVENT_ITEM)Parameter);
}

static VOID NTAPI FwItemsUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    ProcessHacker_Invoke(PhMainWndHandle, OnFwItemsUpdated, NULL);
}

static VOID NTAPI OnFwItemAdded(
    __in PVOID Parameter
    )
{
    PFW_EVENT_ITEM fwItem = Parameter;
    PFW_EVENT_NODE fwNode;

    if (!FwNeedsRedraw)
    {
        TreeNew_SetRedraw(FwTreeNewHandle, FALSE);
        FwNeedsRedraw = TRUE;
    }

    fwNode = AddFwNode(fwItem);
    PhDereferenceObject(fwItem);
}

static VOID NTAPI OnFwItemModified(
    __in PVOID Parameter
    )
{
    PFW_EVENT_ITEM fwItem = Parameter;

    UpdateFwNode(FindFwNode(fwItem));
}

static VOID NTAPI OnFwItemRemoved(
    __in PVOID Parameter
    )
{
    PFW_EVENT_ITEM fwItem = Parameter;

    if (!FwNeedsRedraw)
    {
        TreeNew_SetRedraw(FwTreeNewHandle, FALSE);
        FwNeedsRedraw = TRUE;
    }

    RemoveFwNode(FindFwNode(fwItem));
}

static VOID NTAPI OnFwItemsUpdated(
    __in PVOID Parameter
    )
{
    ULONG i;

    if (FwNeedsRedraw)
    {
        TreeNew_SetRedraw(FwTreeNewHandle, TRUE);
        FwNeedsRedraw = FALSE;
    }

    // Text invalidation

    for (i = 0; i < FwNodeList->Count; i++)
    {
        PFW_EVENT_NODE node = FwNodeList->Items[i];

        // The ??? never change, so we don't invalidate that.
        // memset(&node->TextCache[2], 0, sizeof(PH_STRINGREF) * (FWTNC_MAXIMUM - 2));
        // Always get the newest tooltip text from the process tree.
        PhSwapReference(&node->TooltipText, NULL);
    }

    InvalidateRect(FwTreeNewHandle, NULL, FALSE);
}
