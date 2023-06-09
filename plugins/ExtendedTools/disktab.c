/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2018-2023
 *
 */

#include "exttools.h"
#include "etwmon.h"
#include <toolstatusintf.h>
#include "disktabp.h"

static PPH_MAIN_TAB_PAGE DiskPage = NULL;
static BOOLEAN DiskTreeNewCreated = FALSE;
static HWND DiskTreeNewHandle = NULL;
static ULONG DiskTreeNewSortColumn = 0;
static PH_SORT_ORDER DiskTreeNewSortOrder = NoSortOrder;
static PH_STRINGREF DiskTreeEmptyText = PH_STRINGREF_INIT(L"Disk monitoring requires System Informer to be restarted with administrative privileges.");
static PPH_STRING DiskTreeErrorText = NULL;

static PPH_HASHTABLE DiskNodeHashtable = NULL; // hashtable of all nodes
static PPH_LIST DiskNodeList = NULL; // list of all nodes

static PH_PROVIDER_EVENT_QUEUE EtpDiskEventQueue;
static PH_CALLBACK_REGISTRATION DiskItemAddedRegistration;
static PH_CALLBACK_REGISTRATION DiskItemModifiedRegistration;
static PH_CALLBACK_REGISTRATION DiskItemRemovedRegistration;
static PH_CALLBACK_REGISTRATION DiskItemsUpdatedRegistration;

static PH_TN_FILTER_SUPPORT FilterSupport;
static PTOOLSTATUS_INTERFACE ToolStatusInterface;
static PH_CALLBACK_REGISTRATION SearchChangedRegistration;

VOID EtInitializeDiskTab(
    VOID
    )
{
    PH_MAIN_TAB_PAGE page;
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(TOOLSTATUS_PLUGIN_NAME))
    {
        ToolStatusInterface = PhGetPluginInformation(toolStatusPlugin)->Interface;

        if (ToolStatusInterface->Version < TOOLSTATUS_INTERFACE_VERSION)
            ToolStatusInterface = NULL;
    }

    memset(&page, 0, sizeof(PH_MAIN_TAB_PAGE));
    PhInitializeStringRef(&page.Name, L"Disk");
    page.Callback = EtpDiskPageCallback;
    DiskPage = PhPluginCreateTabPage(&page);

    if (ToolStatusInterface)
    {
        PTOOLSTATUS_TAB_INFO tabInfo;

        tabInfo = ToolStatusInterface->RegisterTabInfo(DiskPage->Index);
        tabInfo->BannerText = L"Search Disk";
        tabInfo->ActivateContent = EtpToolStatusActivateContent;
        tabInfo->GetTreeNewHandle = EtpToolStatusGetTreeNewHandle;
    }
}

BOOLEAN EtpDiskPageCallback(
    _In_ struct _PH_MAIN_TAB_PAGE *Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MainTabPageCreateWindow:
        {
            HWND hwnd;
            ULONG thinRows;
            ULONG treelistBorder;
            ULONG treelistCustomColors;
            PH_TREENEW_CREATEPARAMS treelistCreateParams = { 0 };

            thinRows = PhGetIntegerSetting(L"ThinRows") ? TN_STYLE_THIN_ROWS : 0;
            treelistBorder = (PhGetIntegerSetting(L"TreeListBorderEnable") && !!PhGetIntegerSetting(L"TreeListBorderEnable")) ? WS_BORDER : 0;
            treelistCustomColors =  PhGetIntegerSetting(L"TreeListCustomColorsEnable") ? TN_STYLE_CUSTOM_COLORS : 0;

            if (treelistCustomColors)
            {
                treelistCreateParams.TextColor = PhGetIntegerSetting(L"TreeListCustomColorText");
                treelistCreateParams.FocusColor = PhGetIntegerSetting(L"TreeListCustomColorFocus");
                treelistCreateParams.SelectionColor = PhGetIntegerSetting(L"TreeListCustomColorSelection");
            }

            hwnd = CreateWindow(
                PH_TREENEW_CLASSNAME,
                NULL,
                WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | thinRows | treelistBorder | treelistCustomColors,
                0,
                0,
                3,
                3,
                PhMainWndHandle,
                NULL,
                NULL,
                &treelistCreateParams
                );

            if (!hwnd)
                return FALSE;

            if (PhGetIntegerSetting(L"EnableThemeSupport"))
            {
                PhInitializeWindowTheme(hwnd, TRUE); // HACK (dmex)
                TreeNew_ThemeSupport(hwnd, TRUE);
            }

            DiskTreeNewCreated = TRUE;

            DiskNodeHashtable = PhCreateHashtable(
                sizeof(PET_DISK_NODE),
                EtpDiskNodeHashtableEqualFunction,
                EtpDiskNodeHashtableHashFunction,
                100
                );
            DiskNodeList = PhCreateList(100);

            EtInitializeDiskTreeList(hwnd);

            if (!EtEtwEnabled)
            {
                if (EtEtwStatus != ERROR_SUCCESS)
                {
                    PPH_STRING statusMessage;

                    if (statusMessage = PhGetStatusMessage(0, EtEtwStatus))
                    {
                        DiskTreeErrorText = PhFormatString(
                            L"%s %s (%lu)",
                            L"Unable to start the kernel event tracing session: ",
                            statusMessage->Buffer,
                            EtEtwStatus
                            );
                        PhDereferenceObject(statusMessage);
                    }
                    else
                    {
                        DiskTreeErrorText = PhFormatString(
                            L"%s (%lu)",
                            L"Unable to start the kernel event tracing session: ",
                            EtEtwStatus
                            );
                    }

                    TreeNew_SetEmptyText(hwnd, &DiskTreeErrorText->sr, 0);
                }
                else
                {
                    TreeNew_SetEmptyText(hwnd, &DiskTreeEmptyText, 0);
                }
            }

            PhInitializeProviderEventQueue(&EtpDiskEventQueue, 100);

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

            PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
            EtInitializeDiskInformation();
            PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

            if (Parameter1)
            {
                *(HWND*)Parameter1 = hwnd;
            }
        }
        return TRUE;
    case MainTabPageLoadSettings:
        {
            NOTHING;
        }
        return TRUE;
    case MainTabPageSaveSettings:
        {
            EtSaveSettingsDiskTreeList();
        }
        return TRUE;
    case MainTabPageSelected:
        {
            BOOLEAN selected = (BOOLEAN)Parameter1;

            if (selected)
            {
                EtDiskEnabled = TRUE;
            }
            else
            {
                EtDiskEnabled = FALSE;
            }
        }
        break;
    case MainTabPageExportContent:
        {
            PPH_MAIN_TAB_PAGE_EXPORT_CONTENT exportContent = Parameter1;

            if (!(EtEtwEnabled && exportContent))
                return FALSE;

            EtWriteDiskList(exportContent->FileStream, exportContent->Mode);
        }
        return TRUE;
    case MainTabPageFontChanged:
        {
            HFONT font = (HFONT)Parameter1;

            if (DiskTreeNewHandle)
                SetWindowFont(DiskTreeNewHandle, font, TRUE);
        }
        break;
    }

    return FALSE;
}

BOOLEAN EtpDiskNodeHashtableEqualFunction(
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
    TreeNew_SetImageList(hwnd, PhGetProcessSmallImageList());

    TreeNew_SetRedraw(hwnd, FALSE);

    // Default columns
    PhAddTreeNewColumn(hwnd, ETDSTNC_NAME, TRUE, L"Name", 100, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, ETDSTNC_FILE, TRUE, L"File", 400, PH_ALIGN_LEFT, 1, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumnEx(hwnd, ETDSTNC_READRATEAVERAGE, TRUE, L"Read rate average", 70, PH_ALIGN_RIGHT, 2, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(hwnd, ETDSTNC_WRITERATEAVERAGE, TRUE, L"Write rate average", 70, PH_ALIGN_RIGHT, 3, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(hwnd, ETDSTNC_TOTALRATEAVERAGE, TRUE, L"Total rate average", 70, PH_ALIGN_RIGHT, 4, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(hwnd, ETDSTNC_IOPRIORITY, TRUE, L"I/O priority", 70, PH_ALIGN_LEFT, 5, 0, TRUE);
    PhAddTreeNewColumnEx(hwnd, ETDSTNC_RESPONSETIME, TRUE, L"Response time (ms)", 70, PH_ALIGN_RIGHT, 6, 0, TRUE);
    PhAddTreeNewColumn(hwnd, ETDSTNC_PID, FALSE, L"PID", 50, PH_ALIGN_RIGHT, ULONG_MAX, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, ETDSTNC_ORIGINALNAME, FALSE, L"Original name", 200, PH_ALIGN_LEFT, ULONG_MAX, DT_PATH_ELLIPSIS);

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
    PH_INTEGER_PAIR sortSettings;

    PhCmLoadSettings(DiskTreeNewHandle, &PhaGetStringSetting(SETTING_NAME_DISK_TREE_LIST_COLUMNS)->sr);

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

    settings = PH_AUTO(PhCmSaveSettings(DiskTreeNewHandle));
    PhSetStringSetting2(SETTING_NAME_DISK_TREE_LIST_COLUMNS, &settings->sr);

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

    if ((index = PhFindItemList(DiskNodeList, DiskNode)) != ULONG_MAX)
        PhRemoveItemList(DiskNodeList, index);

    if (DiskNode->ProcessNameText) PhDereferenceObject(DiskNode->ProcessNameText);
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

    PhClearReference(&DiskNode->TooltipText);

    PhInvalidateTreeNewNode(&DiskNode->Node, TN_CACHE_ICON);
    TreeNew_NodesStructured(DiskTreeNewHandle);
}

VOID EtTickDiskNodes(
    VOID
    )
{
    // Text invalidation

    for (ULONG i = 0; i < DiskNodeList->Count; i++)
    {
        PET_DISK_NODE node = DiskNodeList->Items[i];

        // The name and file name never change, so we don't invalidate that.
        memset(&node->TextCache[2], 0, sizeof(PH_STRINGREF) * (ETDSTNC_MAXIMUM - 2));

        // Always get the newest tooltip text from the process tree.
        PhClearReference(&node->TooltipText);
    }

    InvalidateRect(DiskTreeNewHandle, NULL, FALSE);
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
    if (sortResult == 0 && diskItem1->FileNameWin32 && diskItem2->FileNameWin32) \
        sortResult = PhCompareString(diskItem1->FileNameWin32, diskItem2->FileNameWin32, TRUE); \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)diskItem1->FileObject, (ULONG_PTR)diskItem2->FileObject); \
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

BEGIN_SORT_FUNCTION(Pid)
{
    sortResult = intptrcmp((LONG_PTR)diskItem1->ProcessId, (LONG_PTR)diskItem2->ProcessId);
}
END_SORT_FUNCTION

BOOLEAN NTAPI EtpDiskTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
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
                    SORT_FUNCTION(ResponseTime),
                    SORT_FUNCTION(Pid),
                    SORT_FUNCTION(File),
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
                getCellText->Text = PhGetStringRef(node->ProcessNameText);
                break;
            case ETDSTNC_FILE:
                getCellText->Text = PhGetStringRef(diskItem->FileNameWin32);
                break;
            case ETDSTNC_READRATEAVERAGE:
                {
                    ULONG64 number;

                    number = diskItem->ReadAverage;
                    number *= 1000;
                    number /= EtUpdateInterval;

                    if (number != 0)
                    {
                        SIZE_T returnLength;
                        PH_FORMAT format[2];

                        PhInitFormatSize(&format[0], number);
                        PhInitFormatS(&format[1], L"/s");

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), node->ReadRateAverageText, sizeof(node->ReadRateAverageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->ReadRateAverageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                }
                break;
            case ETDSTNC_WRITERATEAVERAGE:
                {
                    ULONG64 number;

                    number = diskItem->WriteAverage;
                    number *= 1000;
                    number /= EtUpdateInterval;

                    if (number != 0)
                    {
                        SIZE_T returnLength;
                        PH_FORMAT format[2];

                        PhInitFormatSize(&format[0], number);
                        PhInitFormatS(&format[1], L"/s");

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), node->WriteRateAverageText, sizeof(node->WriteRateAverageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->WriteRateAverageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                }
                break;
            case ETDSTNC_TOTALRATEAVERAGE:
                {
                    ULONG64 number;

                    number = diskItem->ReadAverage + diskItem->WriteAverage;
                    number *= 1000;
                    number /= EtUpdateInterval;

                    if (number != 0)
                    {
                        SIZE_T returnLength;
                        PH_FORMAT format[2];

                        PhInitFormatSize(&format[0], number);
                        PhInitFormatS(&format[1], L"/s");

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), node->TotalRateAverageText, sizeof(node->TotalRateAverageText), &returnLength))
                        {
                            getCellText->Text.Buffer = node->TotalRateAverageText;
                            getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                        }
                    }
                }
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
                    SIZE_T returnLength;
                    PH_FORMAT format;

                    PhInitFormatF(&format, diskItem->ResponseTimeAverage, 0);

                    if (PhFormatToBuffer(&format, 1, node->ResponseTimeText, sizeof(node->ResponseTimeText), &returnLength))
                    {
                        getCellText->Text.Buffer = node->ResponseTimeText;
                        getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                    }
                }
                break;
            case ETDSTNC_PID:
                PhInitializeStringRefLongHint(&getCellText->Text, diskItem->ProcessIdString);
                break;
            case ETDSTNC_ORIGINALNAME:
                getCellText->Text = PhGetStringRef(diskItem->FileName);
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
            getNodeIcon->Icon = (HICON)node->DiskItem->ProcessIconIndex;
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
                getCellTooltip->Text = PhGetStringRef(node->TooltipText);
                getCellTooltip->Unfolding = FALSE;
                getCellTooltip->MaximumWidth = ULONG_MAX;
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(WindowHandle, &DiskTreeNewSortColumn, &DiskTreeNewSortOrder);
            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(WindowHandle);
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
                    EtHandleDiskCommand(WindowHandle, ID_DISK_COPY);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(DiskTreeNewHandle, 0, -1);
                break;
            case VK_RETURN:
                EtHandleDiskCommand(WindowHandle, ID_DISK_OPENFILELOCATION);
                break;
            }
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = WindowHandle;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = ETDSTNC_TOTALRATEAVERAGE;
            data.DefaultSortOrder = DescendingSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(data.Menu, WindowHandle, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            EtHandleDiskCommand(WindowHandle, ID_DISK_OPENFILELOCATION);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            EtShowDiskContextMenu(WindowHandle, contextMenuEvent);
        }
        return TRUE;
    }

    return FALSE;
}

PPH_STRING EtpGetDiskItemProcessName(
    _In_ PET_DISK_ITEM DiskItem
    )
{
    PH_FORMAT format[1];

    if (DiskItem->ProcessId)
    {
        if (DiskItem->ProcessName)
            PhInitFormatSR(&format[0], DiskItem->ProcessName->sr);
        else
            PhInitFormatS(&format[0], L"Unknown process");
    }
    else
    {
        PhInitFormatS(&format[0], L"No process");
    }

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
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

_Success_(return)
BOOLEAN EtGetSelectedDiskItems(
    _Out_ PET_DISK_ITEM **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < DiskNodeList->Count; i++)
    {
        PET_DISK_NODE node = DiskNodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node->DiskItem);
        }
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    return FALSE;
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
    _In_ HWND WindowHandle,
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
                    BOOLEAN found = FALSE;

                    if (EtWindowsVersion >= WINDOWS_10_RS3 && !EtIsExecutingInWow64)
                    {
                        if ((processNode = PhFindProcessNode(diskItem->ProcessId)) &&
                            processNode->ProcessItem->ProcessSequenceNumber == diskItem->ProcessRecord->ProcessSequenceNumber)
                        {
                            found = TRUE;
                        }
                    }
                    else
                    {
                        // Check if this is really the process that we want, or if it's just a case of PID re-use. (wj32)
                        if ((processNode = PhFindProcessNode(diskItem->ProcessId)) &&
                            processNode->ProcessItem->CreateTime.QuadPart == diskItem->ProcessRecord->CreateTime.QuadPart)
                        {
                            found = TRUE;
                        }
                    }

                    if (found)
                    {
                        ProcessHacker_SelectTabPage(0);
                        PhSelectAndEnsureVisibleProcessNode(processNode);
                    }
                    else
                    {
                        PhShowProcessRecordDialog(PhMainWndHandle, diskItem->ProcessRecord);
                    }
                }
                else
                {
                    PhShowError2(WindowHandle, L"Unable to select the process.", L"%s", L"The process does not exist.");
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
                ULONG_PTR streamIndex;
                PPH_STRING fileName;

                // Strip ADS from path (dmex)
                fileName = PhReferenceObject(diskItem->FileNameWin32);
                streamIndex = PhFindLastCharInStringRef(&fileName->sr, L':', FALSE);

                if (streamIndex != SIZE_MAX && streamIndex != 1)
                {
                    PhMoveReference(&fileName, PhSubstring(fileName, 0, streamIndex));
                }

                PhShellExploreFile(WindowHandle, fileName->Buffer);
                PhDereferenceObject(fileName);
            }
        }
        break;
    case ID_DISK_COPY:
        {
            EtCopyDiskList();
        }
        break;
    case ID_DISK_INSPECT:
        {
            PET_DISK_ITEM diskItem = EtGetSelectedDiskItem();

            if (diskItem)
            {
                ULONG_PTR streamIndex;
                PPH_STRING fileName;

                // Strip ADS from path (dmex)
                fileName = PhReferenceObject(diskItem->FileNameWin32);
                streamIndex = PhFindLastCharInStringRef(&fileName->sr, L':', FALSE);

                if (streamIndex != SIZE_MAX && streamIndex != 1)
                {
                    PhMoveReference(&fileName, PhSubstring(fileName, 0, streamIndex));
                }

                if (PhDoesFileExistWin32(PhGetString(fileName)))
                {
                    PhShellExecuteUserString(
                        WindowHandle,
                        L"ProgramInspectExecutables",
                        fileName->Buffer,
                        FALSE,
                        L"Make sure the PE Viewer executable file is present."
                        );
                }

                PhDereferenceObject(fileName);
            }
        }
        break;
    case ID_DISK_PROPERTIES:
        {
            PET_DISK_ITEM diskItem = EtGetSelectedDiskItem();

            if (diskItem)
            {
                ULONG_PTR streamIndex;
                PPH_STRING fileName;

                // Strip ADS from path (dmex)
                fileName = PhReferenceObject(diskItem->FileNameWin32);
                streamIndex = PhFindLastCharInStringRef(&fileName->sr, L':', FALSE);

                if (streamIndex != SIZE_MAX && streamIndex != 1)
                {
                    PhMoveReference(&fileName, PhSubstring(fileName, 0, streamIndex));
                }

                PhShellProperties(WindowHandle, fileName->Buffer);
                PhDereferenceObject(fileName);
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
    _In_ HWND TreeWindowHandle,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenuEvent
    )
{
    PET_DISK_ITEM *diskItems;
    ULONG numberOfDiskItems;

    if (!EtGetSelectedDiskItems(&diskItems, &numberOfDiskItems))
        return;

    if (numberOfDiskItems != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;

        menu = PhCreateEMenu();
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_DISK_GOTOPROCESS, L"&Go to process", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_DISK_OPENFILELOCATION, L"Open &file location\bEnter", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_DISK_INSPECT, L"&Inspect", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_DISK_PROPERTIES, L"P&roperties", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_DISK_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
        PhInsertCopyCellEMenuItem(menu, ID_DISK_COPY, TreeWindowHandle, ContextMenuEvent->Column);
        PhSetFlagsEMenuItem(menu, ID_DISK_OPENFILELOCATION, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        EtpInitializeDiskMenu(menu, diskItems, numberOfDiskItems);

        item = PhShowEMenu(
            menu,
            TreeWindowHandle,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            ContextMenuEvent->Location.x,
            ContextMenuEvent->Location.y
            );

        if (item)
        {
            BOOLEAN handled = FALSE;

            handled = PhHandleCopyCellEMenuItem(item);

            if (!handled)
                EtHandleDiskCommand(TreeWindowHandle, item->Id);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(diskItems);
}

VOID NTAPI EtpDiskItemAddedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PET_DISK_ITEM diskItem = (PET_DISK_ITEM)Parameter;

    PhReferenceObject(diskItem);
    PhPushProviderEventQueue(&EtpDiskEventQueue, ProviderAddedEvent, Parameter, EtRunCount);
}

VOID NTAPI EtpDiskItemModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PhPushProviderEventQueue(&EtpDiskEventQueue, ProviderModifiedEvent, Parameter, EtRunCount);
}

VOID NTAPI EtpDiskItemRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PhPushProviderEventQueue(&EtpDiskEventQueue, ProviderRemovedEvent, Parameter, EtRunCount);
}

VOID NTAPI EtpDiskItemsUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ProcessHacker_Invoke(EtpOnDiskItemsUpdated, EtRunCount);
}

VOID NTAPI EtpOnDiskItemsUpdated(
    _In_ ULONG RunId
    )
{
    PPH_PROVIDER_EVENT events;
    ULONG count;
    ULONG i;

    events = PhFlushProviderEventQueue(&EtpDiskEventQueue, RunId, &count);

    if (events)
    {
        TreeNew_SetRedraw(DiskTreeNewHandle, FALSE);

        for (i = 0; i < count; i++)
        {
            PH_PROVIDER_EVENT_TYPE type = PH_PROVIDER_EVENT_TYPE(events[i]);
            PET_DISK_ITEM diskItem = PH_PROVIDER_EVENT_OBJECT(events[i]);

            switch (type)
            {
            case ProviderAddedEvent:
                EtAddDiskNode(diskItem);
                PhDereferenceObject(diskItem);
                break;
            case ProviderModifiedEvent:
                EtUpdateDiskNode(EtFindDiskNode(diskItem));
                break;
            case ProviderRemovedEvent:
                EtRemoveDiskNode(EtFindDiskNode(diskItem));
                break;
            }
        }

        PhFree(events);
    }

    EtTickDiskNodes();

    if (count != 0)
        TreeNew_SetRedraw(DiskTreeNewHandle, TRUE);
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

    // Hide nodes without filenames (dmex)
    //if (PhIsNullOrEmptyString(diskNode->DiskItem->FileName))
    //    return FALSE;

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

//INT_PTR CALLBACK EtpDiskTabErrorDialogProc(
//    _In_ HWND hwndDlg,
//    _In_ UINT uMsg,
//    _In_ WPARAM wParam,
//    _In_ LPARAM lParam
//    )
//{
//    switch (uMsg)
//    {
//    case WM_INITDIALOG:
//        {
//            if (!PhGetOwnTokenAttributes().Elevated)
//            {
//                Button_SetElevationRequiredState(GetDlgItem(hwndDlg, IDC_RESTART), TRUE);
//            }
//            else
//            {
//                PhSetDialogItemText(hwndDlg, IDC_ERROR, L"Unable to start the kernel event tracing session.");
//                ShowWindow(GetDlgItem(hwndDlg, IDC_RESTART), SW_HIDE);
//            }
//
//            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
//        }
//        break;
//    case WM_COMMAND:
//        {
//            switch (GET_WM_COMMAND_ID(wParam, lParam))
//            {
//            case IDC_RESTART:
//                ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);
//
//                if (PhShellProcessHacker(
//                    PhMainWndHandle,
//                    L"-v -selecttab Disk",
//                    SW_SHOW,
//                    PH_SHELL_EXECUTE_ADMIN | PH_SHELL_EXECUTE_NOASYNC,
//                    PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
//                    0,
//                    NULL
//                    ))
//                {
//                    ProcessHacker_Destroy(PhMainWndHandle);
//                }
//                else
//                {
//                    ProcessHacker_CancelEarlyShutdown(PhMainWndHandle);
//                }
//
//                break;
//            }
//        }
//        break;
//    case WM_CTLCOLORBTN:
//    case WM_CTLCOLORSTATIC:
//        {
//            SetBkMode((HDC)wParam, TRANSPARENT);
//            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
//        }
//        break;
//    }
//
//    return FALSE;
//}
