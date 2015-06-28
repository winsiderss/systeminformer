/*
 * Process Hacker Extended Tools -
 *   ETW disk monitoring
 *
 * Copyright (C) 2011-2015 wj32
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

#include "exttools.h"
#include "etwmon.h"
#include "resource.h"
#include <toolstatusintf.h>
#include "disktabp.h"

static BOOLEAN DiskTreeNewCreated = FALSE;
static HWND DiskTreeNewHandle;
static ULONG DiskTreeNewSortColumn;
static PH_SORT_ORDER DiskTreeNewSortOrder;

static PPH_HASHTABLE DiskNodeHashtable; // hashtable of all nodes
static PPH_LIST DiskNodeList; // list of all nodes

static PH_CALLBACK_REGISTRATION DiskItemAddedRegistration;
static PH_CALLBACK_REGISTRATION DiskItemModifiedRegistration;
static PH_CALLBACK_REGISTRATION DiskItemRemovedRegistration;
static PH_CALLBACK_REGISTRATION DiskItemsUpdatedRegistration;
static BOOLEAN DiskNeedsRedraw = FALSE;

static PH_TN_FILTER_SUPPORT FilterSupport;
static PTOOLSTATUS_INTERFACE ToolStatusInterface;
static PH_CALLBACK_REGISTRATION SearchChangedRegistration;

VOID EtInitializeDiskTab(
    VOID
    )
{
    PH_ADDITIONAL_TAB_PAGE tabPage;
    PPH_ADDITIONAL_TAB_PAGE addedTabPage;
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(TOOLSTATUS_PLUGIN_NAME))
    {
        ToolStatusInterface = PhGetPluginInformation(toolStatusPlugin)->Interface;

        if (ToolStatusInterface->Version < TOOLSTATUS_INTERFACE_VERSION)
            ToolStatusInterface = NULL;
    }

    memset(&tabPage, 0, sizeof(PH_ADDITIONAL_TAB_PAGE));
    tabPage.Text = L"Disk";
    tabPage.CreateFunction = EtpDiskTabCreateFunction;
    tabPage.Index = MAXINT;
    tabPage.SelectionChangedCallback = EtpDiskTabSelectionChangedCallback;
    tabPage.SaveContentCallback = EtpDiskTabSaveContentCallback;
    tabPage.FontChangedCallback = EtpDiskTabFontChangedCallback;
    addedTabPage = ProcessHacker_AddTabPage(PhMainWndHandle, &tabPage);

    if (ToolStatusInterface)
    {
        PTOOLSTATUS_TAB_INFO tabInfo;

        tabInfo = ToolStatusInterface->RegisterTabInfo(addedTabPage->Index);
        tabInfo->BannerText = L"Search Disk";
        tabInfo->ActivateContent = EtpToolStatusActivateContent;
        tabInfo->GetTreeNewHandle = EtpToolStatusGetTreeNewHandle;
    }
}

HWND NTAPI EtpDiskTabCreateFunction(
    _In_ PVOID Context
    )
{
    HWND hwnd;

    if (EtEtwEnabled)
    {
        ULONG thinRows;

        thinRows = PhGetIntegerSetting(L"ThinRows") ? TN_STYLE_THIN_ROWS : 0;
        hwnd = CreateWindow(
            PH_TREENEW_CLASSNAME,
            NULL,
            WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | thinRows,
            0,
            0,
            3,
            3,
            PhMainWndHandle,
            NULL,
            NULL,
            NULL
            );

        if (!hwnd)
            return NULL;
    }
    else
    {
        return CreateDialog(
            PluginInstance->DllBase,
            MAKEINTRESOURCE(IDD_DISKTABERROR),
            PhMainWndHandle,
            EtpDiskTabErrorDialogProc
            );
    }

    DiskTreeNewCreated = TRUE;

    DiskNodeHashtable = PhCreateHashtable(
        sizeof(PET_DISK_NODE),
        EtpDiskNodeHashtableCompareFunction,
        EtpDiskNodeHashtableHashFunction,
        100
        );
    DiskNodeList = PhCreateList(100);

    EtInitializeDiskTreeList(hwnd);

    PhRegisterCallback(
        &EtDiskItemAddedEvent,
        EtpDiskItemAddedHandler,
        NULL,
        &DiskItemAddedRegistration
        );
    PhRegisterCallback(
        &EtDiskItemModifiedEvent,
        EtpDiskItemModifiedHandler,
        NULL,
        &DiskItemModifiedRegistration
        );
    PhRegisterCallback(
        &EtDiskItemRemovedEvent,
        EtpDiskItemRemovedHandler,
        NULL,
        &DiskItemRemovedRegistration
        );
    PhRegisterCallback(
        &EtDiskItemsUpdatedEvent,
        EtpDiskItemsUpdatedHandler,
        NULL,
        &DiskItemsUpdatedRegistration
        );

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    EtInitializeDiskInformation();
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    return hwnd;
}

VOID NTAPI EtpDiskTabSelectionChangedCallback(
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3,
    _In_ PVOID Context
    )
{
    if ((BOOLEAN)Parameter1)
    {
        if (DiskTreeNewHandle)
            SetFocus(DiskTreeNewHandle);
    }
}

VOID NTAPI EtpDiskTabSaveContentCallback(
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3,
    _In_ PVOID Context
    )
{
    PPH_FILE_STREAM fileStream = Parameter1;
    ULONG mode = PtrToUlong(Parameter2);

    if (!EtEtwEnabled)
        return;

    EtWriteDiskList(fileStream, mode);
}

VOID NTAPI EtpDiskTabFontChangedCallback(
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3,
    _In_ PVOID Context
    )
{
    if (DiskTreeNewHandle)
        SendMessage(DiskTreeNewHandle, WM_SETFONT, (WPARAM)Parameter1, TRUE);
}

BOOLEAN EtpDiskNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PET_DISK_NODE diskNode1 = *(PET_DISK_NODE *)Entry1;
    PET_DISK_NODE diskNode2 = *(PET_DISK_NODE *)Entry2;

    return diskNode1->DiskItem == diskNode2->DiskItem;
}

ULONG EtpDiskNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)(*(PET_DISK_NODE *)Entry)->DiskItem);
}

VOID EtInitializeDiskTreeList(
    _In_ HWND hwnd
    )
{
    DiskTreeNewHandle = hwnd;
    PhSetControlTheme(DiskTreeNewHandle, L"explorer");
    SendMessage(TreeNew_GetTooltips(DiskTreeNewHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, 0x7fff);

    TreeNew_SetCallback(hwnd, EtpDiskTreeNewCallback, NULL);

    TreeNew_SetRedraw(hwnd, FALSE);

    // Default columns
    PhAddTreeNewColumn(hwnd, ETDSTNC_NAME, TRUE, L"Name", 100, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, ETDSTNC_FILE, TRUE, L"File", 400, PH_ALIGN_LEFT, 1, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumnEx(hwnd, ETDSTNC_READRATEAVERAGE, TRUE, L"Read Rate Average", 70, PH_ALIGN_RIGHT, 2, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(hwnd, ETDSTNC_WRITERATEAVERAGE, TRUE, L"Write Rate Average", 70, PH_ALIGN_RIGHT, 3, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(hwnd, ETDSTNC_TOTALRATEAVERAGE, TRUE, L"Total Rate Average", 70, PH_ALIGN_RIGHT, 4, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(hwnd, ETDSTNC_IOPRIORITY, TRUE, L"I/O Priority", 70, PH_ALIGN_LEFT, 5, 0, TRUE);
    PhAddTreeNewColumnEx(hwnd, ETDSTNC_RESPONSETIME, TRUE, L"Response Time (ms)", 70, PH_ALIGN_RIGHT, 6, 0, TRUE);

    TreeNew_SetRedraw(hwnd, TRUE);

    TreeNew_SetSort(hwnd, ETDSTNC_TOTALRATEAVERAGE, DescendingSortOrder);

    EtLoadSettingsDiskTreeList();

    PhInitializeTreeNewFilterSupport(&FilterSupport, hwnd, DiskNodeList);

    if (ToolStatusInterface)
    {
        PhRegisterCallback(ToolStatusInterface->SearchChangedEvent, EtpSearchChangedHandler, NULL, &SearchChangedRegistration);
        PhAddTreeNewFilter(&FilterSupport, EtpSearchDiskListFilterCallback, NULL);
    }
}

VOID EtLoadSettingsDiskTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;

    settings = PhGetStringSetting(SETTING_NAME_DISK_TREE_LIST_COLUMNS);
    PhCmLoadSettings(DiskTreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);

    sortSettings = PhGetIntegerPairSetting(SETTING_NAME_DISK_TREE_LIST_SORT);
    TreeNew_SetSort(DiskTreeNewHandle, (ULONG)sortSettings.X, (PH_SORT_ORDER)sortSettings.Y);
}

VOID EtSaveSettingsDiskTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    if (!DiskTreeNewCreated)
        return;

    settings = PhCmSaveSettings(DiskTreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_DISK_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    TreeNew_GetSort(DiskTreeNewHandle, &sortColumn, &sortOrder);
    sortSettings.X = sortColumn;
    sortSettings.Y = sortOrder;
    PhSetIntegerPairSetting(SETTING_NAME_DISK_TREE_LIST_SORT, sortSettings);
}

PET_DISK_NODE EtAddDiskNode(
    _In_ PET_DISK_ITEM DiskItem
    )
{
    PET_DISK_NODE diskNode;

    diskNode = PhAllocate(sizeof(ET_DISK_NODE));
    memset(diskNode, 0, sizeof(ET_DISK_NODE));
    PhInitializeTreeNewNode(&diskNode->Node);

    PhSetReference(&diskNode->DiskItem, DiskItem);

    memset(diskNode->TextCache, 0, sizeof(PH_STRINGREF) * ETDSTNC_MAXIMUM);
    diskNode->Node.TextCache = diskNode->TextCache;
    diskNode->Node.TextCacheSize = ETDSTNC_MAXIMUM;

    diskNode->ProcessNameText = EtpGetDiskItemProcessName(DiskItem);

    PhAddEntryHashtable(DiskNodeHashtable, &diskNode);
    PhAddItemList(DiskNodeList, diskNode);

    if (FilterSupport.NodeList)
        diskNode->Node.Visible = PhApplyTreeNewFiltersToNode(&FilterSupport, &diskNode->Node);

    TreeNew_NodesStructured(DiskTreeNewHandle);

    return diskNode;
}

PET_DISK_NODE EtFindDiskNode(
    _In_ PET_DISK_ITEM DiskItem
    )
{
    ET_DISK_NODE lookupDiskNode;
    PET_DISK_NODE lookupDiskNodePtr = &lookupDiskNode;
    PET_DISK_NODE *diskNode;

    lookupDiskNode.DiskItem = DiskItem;

    diskNode = (PET_DISK_NODE *)PhFindEntryHashtable(
        DiskNodeHashtable,
        &lookupDiskNodePtr
        );

    if (diskNode)
        return *diskNode;
    else
        return NULL;
}

VOID EtRemoveDiskNode(
    _In_ PET_DISK_NODE DiskNode
    )
{
    ULONG index;

    // Remove from the hashtable/list and cleanup.

    PhRemoveEntryHashtable(DiskNodeHashtable, &DiskNode);

    if ((index = PhFindItemList(DiskNodeList, DiskNode)) != -1)
        PhRemoveItemList(DiskNodeList, index);

    if (DiskNode->ProcessNameText) PhDereferenceObject(DiskNode->ProcessNameText);
    if (DiskNode->ReadRateAverageText) PhDereferenceObject(DiskNode->ReadRateAverageText);
    if (DiskNode->WriteRateAverageText) PhDereferenceObject(DiskNode->WriteRateAverageText);
    if (DiskNode->TotalRateAverageText) PhDereferenceObject(DiskNode->TotalRateAverageText);
    if (DiskNode->ResponseTimeText) PhDereferenceObject(DiskNode->ResponseTimeText);
    if (DiskNode->TooltipText) PhDereferenceObject(DiskNode->TooltipText);

    PhDereferenceObject(DiskNode->DiskItem);

    PhFree(DiskNode);

    TreeNew_NodesStructured(DiskTreeNewHandle);
}

VOID EtUpdateDiskNode(
    _In_ PET_DISK_NODE DiskNode
    )
{
    memset(DiskNode->TextCache, 0, sizeof(PH_STRINGREF) * ETDSTNC_MAXIMUM);

    PhInvalidateTreeNewNode(&DiskNode->Node, TN_CACHE_ICON);
    TreeNew_NodesStructured(DiskTreeNewHandle);
}

#define SORT_FUNCTION(Column) EtpDiskTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl EtpDiskTreeNewCompare##Column( \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PET_DISK_NODE node1 = *(PET_DISK_NODE *)_elem1; \
    PET_DISK_NODE node2 = *(PET_DISK_NODE *)_elem2; \
    PET_DISK_ITEM diskItem1 = node1->DiskItem; \
    PET_DISK_ITEM diskItem2 = node2->DiskItem; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = PhCompareString(diskItem1->FileNameWin32, diskItem2->FileNameWin32, TRUE); \
    \
    return PhModifySort(sortResult, DiskTreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Process)
{
    sortResult = PhCompareString(node1->ProcessNameText, node2->ProcessNameText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(File)
{
    sortResult = PhCompareString(diskItem1->FileNameWin32, diskItem2->FileNameWin32, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ReadRateAverage)
{
    sortResult = uint64cmp(diskItem1->ReadAverage, diskItem2->ReadAverage);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(WriteRateAverage)
{
    sortResult = uint64cmp(diskItem1->WriteAverage, diskItem2->WriteAverage);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(TotalRateAverage)
{
    sortResult = uint64cmp(diskItem1->ReadAverage + diskItem1->WriteAverage, diskItem2->ReadAverage + diskItem2->WriteAverage);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IoPriority)
{
    sortResult = uintcmp(diskItem1->IoPriority, diskItem2->IoPriority);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ResponseTime)
{
    sortResult = singlecmp(diskItem1->ResponseTimeAverage, diskItem2->ResponseTimeAverage);
}
END_SORT_FUNCTION

BOOLEAN NTAPI EtpDiskTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PET_DISK_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Process),
                    SORT_FUNCTION(File),
                    SORT_FUNCTION(ReadRateAverage),
                    SORT_FUNCTION(WriteRateAverage),
                    SORT_FUNCTION(TotalRateAverage),
                    SORT_FUNCTION(IoPriority),
                    SORT_FUNCTION(ResponseTime)
                };
                int (__cdecl *sortFunction)(const void *, const void *);

                if (DiskTreeNewSortColumn < ETDSTNC_MAXIMUM)
                    sortFunction = sortFunctions[DiskTreeNewSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    qsort(DiskNodeList->Items, DiskNodeList->Count, sizeof(PVOID), sortFunction);
                }

                getChildren->Children = (PPH_TREENEW_NODE *)DiskNodeList->Items;
                getChildren->NumberOfChildren = DiskNodeList->Count;
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
            PET_DISK_ITEM diskItem;

            node = (PET_DISK_NODE)getCellText->Node;
            diskItem = node->DiskItem;

            switch (getCellText->Id)
            {
            case ETDSTNC_NAME:
                getCellText->Text = node->ProcessNameText->sr;
                break;
            case ETDSTNC_FILE:
                getCellText->Text = diskItem->FileNameWin32->sr;
                break;
            case ETDSTNC_READRATEAVERAGE:
                EtFormatRate(diskItem->ReadAverage, &node->ReadRateAverageText, &getCellText->Text);
                break;
            case ETDSTNC_WRITERATEAVERAGE:
                EtFormatRate(diskItem->WriteAverage, &node->WriteRateAverageText, &getCellText->Text);
                break;
            case ETDSTNC_TOTALRATEAVERAGE:
                EtFormatRate(diskItem->ReadAverage + diskItem->WriteAverage, &node->TotalRateAverageText, &getCellText->Text);
                break;
            case ETDSTNC_IOPRIORITY:
                switch (diskItem->IoPriority)
                {
                case IoPriorityVeryLow:
                    PhInitializeStringRef(&getCellText->Text, L"Very Low");
                    break;
                case IoPriorityLow:
                    PhInitializeStringRef(&getCellText->Text, L"Low");
                    break;
                case IoPriorityNormal:
                    PhInitializeStringRef(&getCellText->Text, L"Normal");
                    break;
                case IoPriorityHigh:
                    PhInitializeStringRef(&getCellText->Text, L"High");
                    break;
                case IoPriorityCritical:
                    PhInitializeStringRef(&getCellText->Text, L"Critical");
                    break;
                default:
                    PhInitializeStringRef(&getCellText->Text, L"Unknown");
                    break;
                }
                break;
            case ETDSTNC_RESPONSETIME:
                {
                    PH_FORMAT format;

                    PhInitFormatF(&format, diskItem->ResponseTimeAverage, 0);
                    PhMoveReference(&node->ResponseTimeText, PhFormat(&format, 1, 0));
                    getCellText->Text = node->ResponseTimeText->sr;
                }
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeIcon:
        {
            PPH_TREENEW_GET_NODE_ICON getNodeIcon = Parameter1;

            node = (PET_DISK_NODE)getNodeIcon->Node;

            if (node->DiskItem->ProcessIcon)
            {
                getNodeIcon->Icon = node->DiskItem->ProcessIcon->Icon;
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
            PPH_PROCESS_NODE processNode;

            node = (PET_DISK_NODE)getCellTooltip->Node;

            if (getCellTooltip->Column->Id != 0)
                return FALSE;

            if (!node->TooltipText)
            {
                if (processNode = PhFindProcessNode(node->DiskItem->ProcessId))
                {
                    PPH_TREENEW_CALLBACK callback;
                    PVOID callbackContext;
                    PPH_TREENEW_COLUMN fixedColumn;
                    PH_TREENEW_GET_CELL_TOOLTIP fakeGetCellTooltip;

                    // HACK: Get the tooltip text by using the treenew callback of the process tree.
                    if (TreeNew_GetCallback(ProcessTreeNewHandle, &callback, &callbackContext) &&
                        (fixedColumn = TreeNew_GetFixedColumn(ProcessTreeNewHandle)))
                    {
                        fakeGetCellTooltip.Flags = 0;
                        fakeGetCellTooltip.Node = &processNode->Node;
                        fakeGetCellTooltip.Column = fixedColumn;
                        fakeGetCellTooltip.Unfolding = FALSE;
                        PhInitializeEmptyStringRef(&fakeGetCellTooltip.Text);
                        fakeGetCellTooltip.Font = getCellTooltip->Font;
                        fakeGetCellTooltip.MaximumWidth = getCellTooltip->MaximumWidth;

                        if (callback(ProcessTreeNewHandle, TreeNewGetCellTooltip, &fakeGetCellTooltip, NULL, callbackContext))
                        {
                            node->TooltipText = PhCreateString2(&fakeGetCellTooltip.Text);
                        }
                    }
                }
            }

            if (!PhIsNullOrEmptyString(node->TooltipText))
            {
                getCellTooltip->Text = node->TooltipText->sr;
                getCellTooltip->Unfolding = FALSE;
                getCellTooltip->MaximumWidth = -1;
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &DiskTreeNewSortColumn, &DiskTreeNewSortOrder);
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
                    EtHandleDiskCommand(ID_DISK_COPY);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(DiskTreeNewHandle, 0, -1);
                break;
            case VK_RETURN:
                EtHandleDiskCommand(ID_DISK_OPENFILELOCATION);
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
            EtHandleDiskCommand(ID_DISK_OPENFILELOCATION);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

            EtShowDiskContextMenu(mouseEvent->Location);
        }
        return TRUE;
    case TreeNewDestroying:
        {
            EtSaveSettingsDiskTreeList();
        }
        return TRUE;
    }

    return FALSE;
}

PPH_STRING EtpGetDiskItemProcessName(
    _In_ PET_DISK_ITEM DiskItem
    )
{
    PH_FORMAT format[4];

    if (!DiskItem->ProcessId)
        return PhCreateString(L"No Process");

    PhInitFormatS(&format[1], L" (");
    PhInitFormatU(&format[2], HandleToUlong(DiskItem->ProcessId));
    PhInitFormatC(&format[3], ')');

    if (DiskItem->ProcessName)
        PhInitFormatSR(&format[0], DiskItem->ProcessName->sr);
    else
        PhInitFormatS(&format[0], L"Unknown Process");

    return PhFormat(format, 4, 96);
}

PET_DISK_ITEM EtGetSelectedDiskItem(
    VOID
    )
{
    PET_DISK_ITEM diskItem = NULL;
    ULONG i;

    for (i = 0; i < DiskNodeList->Count; i++)
    {
        PET_DISK_NODE node = DiskNodeList->Items[i];

        if (node->Node.Selected)
        {
            diskItem = node->DiskItem;
            break;
        }
    }

    return diskItem;
}

VOID EtGetSelectedDiskItems(
    _Out_ PET_DISK_ITEM **DiskItems,
    _Out_ PULONG NumberOfDiskItems
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < DiskNodeList->Count; i++)
    {
        PET_DISK_NODE node = DiskNodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node->DiskItem);
        }
    }

    *DiskItems = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfDiskItems = list->Count;

    PhDereferenceObject(list);
}

VOID EtDeselectAllDiskNodes(
    VOID
    )
{
    TreeNew_DeselectRange(DiskTreeNewHandle, 0, -1);
}

VOID EtSelectAndEnsureVisibleDiskNode(
    _In_ PET_DISK_NODE DiskNode
    )
{
    EtDeselectAllDiskNodes();

    if (!DiskNode->Node.Visible)
        return;

    TreeNew_SetFocusNode(DiskTreeNewHandle, &DiskNode->Node);
    TreeNew_SetMarkNode(DiskTreeNewHandle, &DiskNode->Node);
    TreeNew_SelectRange(DiskTreeNewHandle, DiskNode->Node.Index, DiskNode->Node.Index);
    TreeNew_EnsureVisible(DiskTreeNewHandle, &DiskNode->Node);
}

VOID EtCopyDiskList(
    VOID
    )
{
    PPH_STRING text;

    text = PhGetTreeNewText(DiskTreeNewHandle, 0);
    PhSetClipboardString(DiskTreeNewHandle, &text->sr);
    PhDereferenceObject(text);
}

VOID EtWriteDiskList(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    )
{
    PPH_LIST lines;
    ULONG i;

    lines = PhGetGenericTreeNewLines(DiskTreeNewHandle, Mode);

    for (i = 0; i < lines->Count; i++)
    {
        PPH_STRING line;

        line = lines->Items[i];
        PhWriteStringAsUtf8FileStream(FileStream, &line->sr);
        PhDereferenceObject(line);
        PhWriteStringAsUtf8FileStream2(FileStream, L"\r\n");
    }

    PhDereferenceObject(lines);
}

VOID EtHandleDiskCommand(
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case ID_DISK_GOTOPROCESS:
        {
            PET_DISK_ITEM diskItem = EtGetSelectedDiskItem();
            PPH_PROCESS_NODE processNode;

            if (diskItem)
            {
                PhReferenceObject(diskItem);

                if (diskItem->ProcessRecord)
                {
                    // Check if this is really the process that we want, or if it's just a case of PID re-use.
                    if ((processNode = PhFindProcessNode(diskItem->ProcessId)) &&
                        processNode->ProcessItem->CreateTime.QuadPart == diskItem->ProcessRecord->CreateTime.QuadPart)
                    {
                        ProcessHacker_SelectTabPage(PhMainWndHandle, 0);
                        PhSelectAndEnsureVisibleProcessNode(processNode);
                    }
                    else
                    {
                        PhShowProcessRecordDialog(PhMainWndHandle, diskItem->ProcessRecord);
                    }
                }
                else
                {
                    PhShowError(PhMainWndHandle, L"The process does not exist.");
                }

                PhDereferenceObject(diskItem);
            }
        }
        break;
    case ID_DISK_OPENFILELOCATION:
        {
            PET_DISK_ITEM diskItem = EtGetSelectedDiskItem();

            if (diskItem)
            {
                PhShellExploreFile(PhMainWndHandle, diskItem->FileNameWin32->Buffer);
            }
        }
        break;
    case ID_DISK_COPY:
        {
            EtCopyDiskList();
        }
        break;
    case ID_DISK_PROPERTIES:
        {
            PET_DISK_ITEM diskItem = EtGetSelectedDiskItem();

            if (diskItem)
            {
                PhShellProperties(PhMainWndHandle, diskItem->FileNameWin32->Buffer);
            }
        }
        break;
    }
}

VOID EtpInitializeDiskMenu(
    _In_ PPH_EMENU Menu,
    _In_ PET_DISK_ITEM *DiskItems,
    _In_ ULONG NumberOfDiskItems
    )
{
    PPH_EMENU_ITEM item;

    if (NumberOfDiskItems == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfDiskItems == 1)
    {
        PPH_PROCESS_ITEM processItem;

        // If we have a process record and the process has terminated, we can only show
        // process properties.
        if (DiskItems[0]->ProcessRecord)
        {
            if (processItem = PhReferenceProcessItemForRecord(DiskItems[0]->ProcessRecord))
            {
                PhDereferenceObject(processItem);
            }
            else
            {
                if (item = PhFindEMenuItem(Menu, 0, NULL, ID_DISK_GOTOPROCESS))
                {
                    item->Text = L"Process Properties";
                    item->Flags &= ~PH_EMENU_TEXT_OWNED;
                }
            }
        }
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_DISK_COPY, TRUE);
    }
}

VOID EtShowDiskContextMenu(
    _In_ POINT Location
    )
{
    PET_DISK_ITEM *diskItems;
    ULONG numberOfDiskItems;

    EtGetSelectedDiskItems(&diskItems, &numberOfDiskItems);

    if (numberOfDiskItems != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PluginInstance->DllBase, MAKEINTRESOURCE(IDR_DISK), 0);
        PhSetFlagsEMenuItem(menu, ID_DISK_OPENFILELOCATION, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        EtpInitializeDiskMenu(menu, diskItems, numberOfDiskItems);

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
            EtHandleDiskCommand(item->Id);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(diskItems);
}

static VOID NTAPI EtpDiskItemAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PET_DISK_ITEM diskItem = (PET_DISK_ITEM)Parameter;

    PhReferenceObject(diskItem);
    ProcessHacker_Invoke(PhMainWndHandle, EtpOnDiskItemAdded, diskItem);
}

static VOID NTAPI EtpDiskItemModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ProcessHacker_Invoke(PhMainWndHandle, EtpOnDiskItemModified, (PET_DISK_ITEM)Parameter);
}

static VOID NTAPI EtpDiskItemRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ProcessHacker_Invoke(PhMainWndHandle, EtpOnDiskItemRemoved, (PET_DISK_ITEM)Parameter);
}

static VOID NTAPI EtpDiskItemsUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ProcessHacker_Invoke(PhMainWndHandle, EtpOnDiskItemsUpdated, NULL);
}

static VOID NTAPI EtpOnDiskItemAdded(
    _In_ PVOID Parameter
    )
{
    PET_DISK_ITEM diskItem = Parameter;
    PET_DISK_NODE diskNode;

    if (!DiskNeedsRedraw)
    {
        TreeNew_SetRedraw(DiskTreeNewHandle, FALSE);
        DiskNeedsRedraw = TRUE;
    }

    diskNode = EtAddDiskNode(diskItem);
    PhDereferenceObject(diskItem);
}

static VOID NTAPI EtpOnDiskItemModified(
    _In_ PVOID Parameter
    )
{
    PET_DISK_ITEM diskItem = Parameter;

    EtUpdateDiskNode(EtFindDiskNode(diskItem));
}

static VOID NTAPI EtpOnDiskItemRemoved(
    _In_ PVOID Parameter
    )
{
    PET_DISK_ITEM diskItem = Parameter;

    if (!DiskNeedsRedraw)
    {
        TreeNew_SetRedraw(DiskTreeNewHandle, FALSE);
        DiskNeedsRedraw = TRUE;
    }

    EtRemoveDiskNode(EtFindDiskNode(diskItem));
}

static VOID NTAPI EtpOnDiskItemsUpdated(
    _In_ PVOID Parameter
    )
{
    ULONG i;

    if (DiskNeedsRedraw)
    {
        TreeNew_SetRedraw(DiskTreeNewHandle, TRUE);
        DiskNeedsRedraw = FALSE;
    }

    // Text invalidation

    for (i = 0; i < DiskNodeList->Count; i++)
    {
        PET_DISK_NODE node = DiskNodeList->Items[i];

        // The name and file name never change, so we don't invalidate that.
        memset(&node->TextCache[2], 0, sizeof(PH_STRINGREF) * (ETDSTNC_MAXIMUM - 2));
        // Always get the newest tooltip text from the process tree.
        PhClearReference(&node->TooltipText);
    }

    InvalidateRect(DiskTreeNewHandle, NULL, FALSE);
}

VOID NTAPI EtpSearchChangedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (!EtEtwEnabled)
        return;

    PhApplyTreeNewFilters(&FilterSupport);
}

BOOLEAN NTAPI EtpSearchDiskListFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PET_DISK_NODE diskNode = (PET_DISK_NODE)Node;
    PTOOLSTATUS_WORD_MATCH wordMatch = ToolStatusInterface->WordMatch;

    if (PhIsNullOrEmptyString(ToolStatusInterface->GetSearchboxText()))
        return TRUE;

    if (wordMatch(&diskNode->ProcessNameText->sr))
        return TRUE;

    if (wordMatch(&diskNode->DiskItem->FileNameWin32->sr))
        return TRUE;

    return FALSE;
}

VOID NTAPI EtpToolStatusActivateContent(
    _In_ BOOLEAN Select
    )
{
    SetFocus(DiskTreeNewHandle);

    if (Select)
    {
        if (TreeNew_GetFlatNodeCount(DiskTreeNewHandle) > 0)
            EtSelectAndEnsureVisibleDiskNode((PET_DISK_NODE)TreeNew_GetFlatNode(DiskTreeNewHandle, 0));
    }
}

HWND NTAPI EtpToolStatusGetTreeNewHandle(
    VOID
    )
{
    return DiskTreeNewHandle;
}

INT_PTR CALLBACK EtpDiskTabErrorDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            if (!PhElevated)
            {
                SendMessage(GetDlgItem(hwndDlg, IDC_RESTART), BCM_SETSHIELD, 0, TRUE);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_ERROR, L"Unable to start the kernel event tracing session.");
                ShowWindow(GetDlgItem(hwndDlg, IDC_RESTART), SW_HIDE);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_RESTART:
                ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);

                if (PhShellProcessHacker(
                    PhMainWndHandle,
                    L"-v -selecttab Disk",
                    SW_SHOW,
                    PH_SHELL_EXECUTE_ADMIN,
                    PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                    0,
                    NULL
                    ))
                {
                    ProcessHacker_Destroy(PhMainWndHandle);
                }
                else
                {
                    ProcessHacker_CancelEarlyShutdown(PhMainWndHandle);
                }

                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }

    return FALSE;
}
