/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2019-2022
 *
 */

#include <peview.h>
#include <emenu.h>
#include <cpysave.h>

#include <shobjidl.h>

static GUID CLSID_ShellLink_I = { 0x00021401, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static GUID IID_IShellLinkW_I = { 0x000214f9, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static GUID IID_IPersistFile_I = { 0x0000010b, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

PPH_STRING PvResolveShortcutTarget(
    _In_ PPH_STRING ShortcutFileName
    )
{
    PPH_STRING targetFileName;
    IShellLinkW *shellLink;
    IPersistFile *persistFile;
    WCHAR path[MAX_PATH];

    targetFileName = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_ShellLink_I, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW_I, &shellLink)))
    {
        if (SUCCEEDED(IShellLinkW_QueryInterface(shellLink, &IID_IPersistFile_I, &persistFile)))
        {
            if (SUCCEEDED(IPersistFile_Load(persistFile, ShortcutFileName->Buffer, STGM_READ)) &&
                SUCCEEDED(IShellLinkW_Resolve(shellLink, NULL, SLR_NO_UI)))
            {
                if (SUCCEEDED(IShellLinkW_GetPath(shellLink, path, MAX_PATH, NULL, 0)))
                {
                    targetFileName = PhCreateString(path);
                }
            }

            IPersistFile_Release(persistFile);
        }

        IShellLinkW_Release(shellLink);
    }

    return targetFileName;
}

PPH_STRING PvResolveReparsePointTarget(
    _In_ PPH_STRING FileName
    )
{
    PPH_STRING targetFileName = NULL;
    PREPARSE_DATA_BUFFER reparseBuffer;
    ULONG reparseLength;
    HANDLE fileHandle;

    if (PhIsNullOrEmptyString(FileName))
        return NULL;

    if (!NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(FileName),
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT
        )))
    {
        return NULL;
    }

    reparseLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    reparseBuffer = PhAllocateZero(reparseLength);

    if (NT_SUCCESS(PhDeviceIoControlFile(
        fileHandle,
        FSCTL_GET_REPARSE_POINT,
        NULL,
        0,
        reparseBuffer,
        reparseLength,
        NULL
        )))
    {
        if (
            IsReparseTagMicrosoft(reparseBuffer->ReparseTag) &&
            reparseBuffer->ReparseTag == IO_REPARSE_TAG_APPEXECLINK
            )
        {
            typedef struct _AppExecLinkReparseBuffer
            {
                ULONG StringCount;
                WCHAR StringList[1];
            } AppExecLinkReparseBuffer, *PAppExecLinkReparseBuffer;

            PAppExecLinkReparseBuffer appexeclink;
            PWSTR string;

            appexeclink = (PAppExecLinkReparseBuffer)reparseBuffer->GenericReparseBuffer.DataBuffer;
            string = (PWSTR)appexeclink->StringList;

            for (ULONG i = 0; i < appexeclink->StringCount; i++)
            {
                if (i == 2 && PhDoesFileExistWin32(string))
                {
                    targetFileName = PhCreateString(string);
                    break;
                }

                string += PhCountStringZ(string) + 1;
            }
        }
    }

    PhFree(reparseBuffer);
    NtClose(fileHandle);

    return targetFileName;
}

// Copied from appsup.c

VOID PvCopyListView(
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING text;

    text = PhGetListViewText(ListViewHandle);
    PhSetClipboardString(ListViewHandle, &text->sr);
    PhDereferenceObject(text);
}

VOID PvHandleListViewNotifyForCopy(
    _In_ LPARAM lParam,
    _In_ HWND ListViewHandle
    )
{
    if (((LPNMHDR)lParam)->hwndFrom == ListViewHandle && ((LPNMHDR)lParam)->code == LVN_KEYDOWN)
    {
        LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)lParam;

        switch (keyDown->wVKey)
        {
        case 'C':
            if (GetKeyState(VK_CONTROL) < 0)
                PvCopyListView(ListViewHandle);
            break;
        case 'A':
            if (GetKeyState(VK_CONTROL) < 0)
                PhSetStateAllListViewItems(ListViewHandle, LVIS_SELECTED, LVIS_SELECTED);
            break;
        }
    }
}

typedef struct _PH_COPY_ITEM_CONTEXT
{
    HWND ListViewHandle;
    ULONG Id;
    ULONG SubId;
    PPH_STRING MenuItemText;
} PH_COPY_ITEM_CONTEXT, *PPH_COPY_ITEM_CONTEXT;

VOID NTAPI PhpCopyListViewEMenuItemDeleteFunction(
    _In_ struct _PH_EMENU_ITEM *Item
    )
{
    PPH_COPY_ITEM_CONTEXT context;

    context = Item->Context;
    PhDereferenceObject(context->MenuItemText);
    PhFree(context);
}

BOOLEAN PvInsertCopyListViewEMenuItem(
    _In_ struct _PH_EMENU_ITEM *Menu,
    _In_ ULONG InsertAfterId,
    _In_ HWND ListViewHandle
    )
{
    PPH_EMENU_ITEM parentItem;
    ULONG indexInParent;
    PPH_COPY_ITEM_CONTEXT context;
    PPH_STRING columnText = NULL;
    PPH_STRING escapedText;
    PPH_STRING menuItemText;
    PPH_EMENU_ITEM copyMenuItem;
    POINT location;
    LVHITTESTINFO lvHitInfo;
    HDITEM headerItem;
    WCHAR headerText[MAX_PATH] = L"";

    if (!GetCursorPos(&location))
        return FALSE;
    if (!ScreenToClient(ListViewHandle, &location))
        return FALSE;

    memset(&lvHitInfo, 0, sizeof(LVHITTESTINFO));
    lvHitInfo.pt = location;

    if (ListView_SubItemHitTest(ListViewHandle, &lvHitInfo) == INT_ERROR)
        return FALSE;

    memset(headerText, 0, sizeof(headerText));
    memset(&headerItem, 0, sizeof(HDITEM));
    headerItem.mask = HDI_TEXT;
    headerItem.cchTextMax = RTL_NUMBER_OF(headerText);
    headerItem.pszText = headerText;

    if (!Header_GetItem(ListView_GetHeader(ListViewHandle), lvHitInfo.iSubItem, &headerItem))
        return FALSE;

    columnText = PhaCreateString(headerText);

    if (PhIsNullOrEmptyString(columnText))
        return FALSE;

    if (!PhFindEMenuItemEx(Menu, 0, NULL, InsertAfterId, &parentItem, &indexInParent))
        return FALSE;

    indexInParent++;

    context = PhAllocate(sizeof(PH_COPY_ITEM_CONTEXT));
    context->ListViewHandle = ListViewHandle;
    context->Id = lvHitInfo.iItem;
    context->SubId = lvHitInfo.iSubItem;

    escapedText = PhEscapeStringForMenuPrefix(&columnText->sr);
    menuItemText = PhFormatString(L"Copy \"%s\"", escapedText->Buffer);
    PhDereferenceObject(escapedText);

    copyMenuItem = PhCreateEMenuItem(0, INT_MAX, menuItemText->Buffer, NULL, context);
    copyMenuItem->DeleteFunction = PhpCopyListViewEMenuItemDeleteFunction;
    context->MenuItemText = menuItemText;

    PhInsertEMenuItem(parentItem, copyMenuItem, indexInParent);

    return TRUE;
}

BOOLEAN PvHandleCopyListViewEMenuItem(
    _In_ struct _PH_EMENU_ITEM *SelectedItem
    )
{
    PPH_COPY_ITEM_CONTEXT context;
    PH_STRING_BUILDER stringBuilder;
    ULONG count;
    ULONG selectedCount;
    ULONG i;
    PPH_STRING getItemText;

    if (!SelectedItem)
        return FALSE;
    if (SelectedItem->Id != INT_MAX)
        return FALSE;

    context = SelectedItem->Context;

    PhInitializeStringBuilder(&stringBuilder, 0x100);
    count = ListView_GetItemCount(context->ListViewHandle);
    selectedCount = 0;

    for (i = 0; i < count; i++)
    {
        if (!(ListView_GetItemState(context->ListViewHandle, i, LVIS_SELECTED) & LVIS_SELECTED))
            continue;

        getItemText = PhaGetListViewItemText(context->ListViewHandle, i, context->SubId);

        PhAppendStringBuilder(&stringBuilder, &getItemText->sr);
        PhAppendStringBuilder2(&stringBuilder, L"\r\n");

        selectedCount++;
    }

    if (stringBuilder.String->Length != 0 && selectedCount == 1)
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhSetClipboardString(context->ListViewHandle, &stringBuilder.String->sr);
    PhDeleteStringBuilder(&stringBuilder);

    return TRUE;
}

BOOLEAN PvGetListViewContextMenuPoint(
    _In_ HWND ListViewHandle,
    _Out_ PPOINT Point
    )
{
    INT selectedIndex;
    RECT bounds;
    RECT clientRect;

    // The user pressed a key to display the context menu.
    // Suggest where the context menu should display.
    if ((selectedIndex = PhFindListViewItemByFlags(ListViewHandle, INT_ERROR, LVNI_SELECTED)) != INT_ERROR)
    {
        if (ListView_GetItemRect(ListViewHandle, selectedIndex, &bounds, LVIR_BOUNDS))
        {
            LONG dpiValue = PhGetWindowDpi(ListViewHandle);

            Point->x = bounds.left + PhGetSystemMetrics(SM_CXSMICON, dpiValue) / 2;
            Point->y = bounds.top + PhGetSystemMetrics(SM_CYSMICON, dpiValue) / 2;

            GetClientRect(ListViewHandle, &clientRect);

            if (Point->x < 0 || Point->y < 0 || Point->x >= clientRect.right || Point->y >= clientRect.bottom)
            {
                // The menu is going to be outside of the control. Just put it at the top-left.
                Point->x = 0;
                Point->y = 0;
            }

            ClientToScreen(ListViewHandle, Point);

            return TRUE;
        }
    }

    Point->x = 0;
    Point->y = 0;
    ClientToScreen(ListViewHandle, Point);

    return FALSE;
}

VOID PvHandleListViewCommandCopy(
    _In_ HWND WindowHandle,
    _In_ LPARAM lParam,
    _In_ WPARAM wParam,
    _In_ HWND ListViewHandle
    )
{
    if ((HWND)wParam == ListViewHandle)
    {
        POINT point;
        PPH_EMENU menu;
        PPH_EMENU item;
        PVOID *listviewItems;
        ULONG numberOfItems;

        point.x = GET_X_LPARAM(lParam);
        point.y = GET_Y_LPARAM(lParam);

        if (point.x == -1 && point.y == -1)
            PvGetListViewContextMenuPoint(ListViewHandle, &point);

        PhGetSelectedListViewItemParams(ListViewHandle, &listviewItems, &numberOfItems);

        if (numberOfItems != 0)
        {
            menu = PhCreateEMenu();

            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"&Copy", NULL, NULL), ULONG_MAX);
            PvInsertCopyListViewEMenuItem(menu, USHRT_MAX, ListViewHandle);

            item = PhShowEMenu(
                menu,
                WindowHandle,
                PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                point.x,
                point.y
                );

            if (item)
            {
                if (!PvHandleCopyListViewEMenuItem(item))
                {
                    switch (item->Id)
                    {
                    case USHRT_MAX:
                        {
                            PvCopyListView(ListViewHandle);
                        }
                        break;
                    }
                }
            }

            PhDestroyEMenu(menu);
        }

        PhFree(listviewItems);
    }
}

VOID PvSaveWindowState(
    _In_ HWND WindowHandle
    )
{
    WINDOWPLACEMENT placement = { sizeof(placement) };

    GetWindowPlacement(WindowHandle, &placement);

    if (placement.showCmd == SW_NORMAL)
        PhSetIntegerSetting(L"MainWindowState", SW_NORMAL);
    else if (placement.showCmd == SW_MAXIMIZE)
        PhSetIntegerSetting(L"MainWindowState", SW_MAXIMIZE);
}

VOID PvConfigTreeBorders(
    _In_ HWND WindowHandle
    )
{
    if (!PhGetIntegerSetting(L"EnableTreeListBorder"))
    {
        PhSetWindowStyle(WindowHandle, WS_BORDER, 0);
        PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, 0);
        SetWindowPos(WindowHandle, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

#pragma region copied from appsup.c

VOID PhInitializeTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    )
{
    PhInitializeTreeNewColumnMenuEx(Data, 0);
}

VOID PhInitializeTreeNewColumnMenuEx(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data,
    _In_ ULONG Flags
    )
{
    PPH_EMENU_ITEM resetSortMenuItem = NULL;
    PPH_EMENU_ITEM sizeColumnToFitMenuItem;
    PPH_EMENU_ITEM sizeAllColumnsToFitMenuItem;
    PPH_EMENU_ITEM hideColumnMenuItem = NULL;
    PPH_EMENU_ITEM chooseColumnsMenuItem = NULL;
    ULONG minimumNumberOfColumns;

    Data->Menu = PhCreateEMenu();
    Data->Selection = NULL;
    Data->ProcessedId = 0;

    sizeColumnToFitMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_SIZE_COLUMN_TO_FIT_ID, L"Size column to fit", NULL, NULL);
    sizeAllColumnsToFitMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_SIZE_ALL_COLUMNS_TO_FIT_ID, L"Size all columns to fit", NULL, NULL);

    if (!(Flags & PH_TN_COLUMN_MENU_NO_VISIBILITY))
    {
        hideColumnMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_HIDE_COLUMN_ID, L"Hide column", NULL, NULL);
        chooseColumnsMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID, L"Choose columns...", NULL, NULL);
    }

    if (Flags & PH_TN_COLUMN_MENU_SHOW_RESET_SORT)
    {
        ULONG sortColumn;
        PH_SORT_ORDER sortOrder;

        TreeNew_GetSort(Data->TreeNewHandle, &sortColumn, &sortOrder);

        if (sortOrder != Data->DefaultSortOrder || (Data->DefaultSortOrder != NoSortOrder && sortColumn != Data->DefaultSortColumn))
            resetSortMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_RESET_SORT_ID, L"Reset sort", NULL, NULL);
    }

    PhInsertEMenuItem(Data->Menu, sizeColumnToFitMenuItem, ULONG_MAX);
    PhInsertEMenuItem(Data->Menu, sizeAllColumnsToFitMenuItem, ULONG_MAX);

    if (!(Flags & PH_TN_COLUMN_MENU_NO_VISIBILITY))
    {
        PhInsertEMenuItem(Data->Menu, hideColumnMenuItem, ULONG_MAX);

        if (resetSortMenuItem)
            PhInsertEMenuItem(Data->Menu, resetSortMenuItem, ULONG_MAX);

        PhInsertEMenuItem(Data->Menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(Data->Menu, chooseColumnsMenuItem, ULONG_MAX);

        if (TreeNew_GetFixedColumn(Data->TreeNewHandle))
            minimumNumberOfColumns = 2; // don't allow user to remove all normal columns (the fixed column can never be removed)
        else
            minimumNumberOfColumns = 1;

        if (!Data->MouseEvent || !Data->MouseEvent->Column ||
            Data->MouseEvent->Column->Fixed || // don't allow the fixed column to be hidden
            TreeNew_GetVisibleColumnCount(Data->TreeNewHandle) < minimumNumberOfColumns + 1
            )
        {
            hideColumnMenuItem->Flags |= PH_EMENU_DISABLED;
        }
    }
    else
    {
        if (resetSortMenuItem)
            PhInsertEMenuItem(Data->Menu, resetSortMenuItem, ULONG_MAX);
    }

    if (!Data->MouseEvent || !Data->MouseEvent->Column)
    {
        sizeColumnToFitMenuItem->Flags |= PH_EMENU_DISABLED;
    }
}

VOID PhpEnsureValidSortColumnTreeNew(
    _Inout_ HWND TreeNewHandle,
    _In_ ULONG DefaultSortColumn,
    _In_ PH_SORT_ORDER DefaultSortOrder
    )
{
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    // Make sure the column we're sorting by is actually visible, and if not, don't sort anymore.

    TreeNew_GetSort(TreeNewHandle, &sortColumn, &sortOrder);

    if (sortOrder != NoSortOrder)
    {
        PH_TREENEW_COLUMN column;

        TreeNew_GetColumn(TreeNewHandle, sortColumn, &column);

        if (!column.Visible)
        {
            if (DefaultSortOrder != NoSortOrder)
            {
                // Make sure the default sort column is visible.
                TreeNew_GetColumn(TreeNewHandle, DefaultSortColumn, &column);

                if (!column.Visible)
                {
                    ULONG maxId;
                    ULONG id;
                    BOOLEAN found;

                    // Use the first visible column.
                    maxId = TreeNew_GetMaxId(TreeNewHandle);
                    id = 0;
                    found = FALSE;

                    while (id <= maxId)
                    {
                        if (TreeNew_GetColumn(TreeNewHandle, id, &column))
                        {
                            if (column.Visible)
                            {
                                DefaultSortColumn = id;
                                found = TRUE;
                                break;
                            }
                        }

                        id++;
                    }

                    if (!found)
                    {
                        DefaultSortColumn = 0;
                        DefaultSortOrder = NoSortOrder;
                    }
                }
            }

            TreeNew_SetSort(TreeNewHandle, DefaultSortColumn, DefaultSortOrder);
        }
    }
}

BOOLEAN PhHandleTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    )
{
    if (!Data->Selection)
        return FALSE;

    switch (Data->Selection->Id)
    {
    case PH_TN_COLUMN_MENU_RESET_SORT_ID:
        {
            TreeNew_SetSort(Data->TreeNewHandle, Data->DefaultSortColumn, Data->DefaultSortOrder);
        }
        break;
    case PH_TN_COLUMN_MENU_SIZE_COLUMN_TO_FIT_ID:
        {
            if (Data->MouseEvent && Data->MouseEvent->Column)
            {
                TreeNew_AutoSizeColumn(Data->TreeNewHandle, Data->MouseEvent->Column->Id, 0);
            }
        }
        break;
    case PH_TN_COLUMN_MENU_SIZE_ALL_COLUMNS_TO_FIT_ID:
        {
            ULONG maxId;
            ULONG id;

            maxId = TreeNew_GetMaxId(Data->TreeNewHandle);
            id = 0;

            while (id <= maxId)
            {
                TreeNew_AutoSizeColumn(Data->TreeNewHandle, id, 0);
                id++;
            }
        }
        break;
    case PH_TN_COLUMN_MENU_HIDE_COLUMN_ID:
        {
            PH_TREENEW_COLUMN column;

            if (Data->MouseEvent && Data->MouseEvent->Column && !Data->MouseEvent->Column->Fixed)
            {
                column.Id = Data->MouseEvent->Column->Id;
                column.Visible = FALSE;
                TreeNew_SetColumn(Data->TreeNewHandle, TN_COLUMN_FLAG_VISIBLE, &column);
                PhpEnsureValidSortColumnTreeNew(Data->TreeNewHandle, Data->DefaultSortColumn, Data->DefaultSortOrder);
                InvalidateRect(Data->TreeNewHandle, NULL, FALSE);
            }
        }
        break;
    case PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID:
        {
            PvShowChooseColumnsDialog(Data->TreeNewHandle, Data->TreeNewHandle, PV_CONTROL_TYPE_TREE_NEW);
            PhpEnsureValidSortColumnTreeNew(Data->TreeNewHandle, Data->DefaultSortColumn, Data->DefaultSortOrder);
        }
        break;
    default:
        return FALSE;
    }

    Data->ProcessedId = Data->Selection->Id;

    return TRUE;
}

VOID PhDeleteTreeNewColumnMenu(
    _In_ PPH_TN_COLUMN_MENU_DATA Data
    )
{
    if (Data->Menu)
    {
        PhDestroyEMenu(Data->Menu);
        Data->Menu = NULL;
    }
}

VOID PhInitializeTreeNewFilterSupport(
    _Out_ PPH_TN_FILTER_SUPPORT Support,
    _In_ HWND TreeNewHandle,
    _In_ PPH_LIST NodeList
    )
{
    Support->FilterList = NULL;
    Support->TreeNewHandle = TreeNewHandle;
    Support->NodeList = NodeList;
}

VOID PhDeleteTreeNewFilterSupport(
    _In_ PPH_TN_FILTER_SUPPORT Support
    )
{
    PhDereferenceObject(Support->FilterList);
}

PPH_TN_FILTER_ENTRY PhAddTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_FUNCTION Filter,
    _In_opt_ PVOID Context
    )
{
    PPH_TN_FILTER_ENTRY entry;

    entry = PhAllocate(sizeof(PH_TN_FILTER_ENTRY));
    entry->Filter = Filter;
    entry->Context = Context;

    if (!Support->FilterList)
        Support->FilterList = PhCreateList(2);

    PhAddItemList(Support->FilterList, entry);

    return entry;
}

VOID PhRemoveTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_ENTRY Entry
    )
{
    ULONG index;

    if (!Support->FilterList)
        return;

    index = PhFindItemList(Support->FilterList, Entry);

    if (index != ULONG_MAX)
    {
        PhRemoveItemList(Support->FilterList, index);
        PhFree(Entry);
    }
}

BOOLEAN PhApplyTreeNewFiltersToNode(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TREENEW_NODE Node
    )
{
    BOOLEAN show;
    ULONG i;

    show = TRUE;

    if (Support->FilterList)
    {
        for (i = 0; i < Support->FilterList->Count; i++)
        {
            PPH_TN_FILTER_ENTRY entry;

            entry = Support->FilterList->Items[i];

            if (!entry->Filter(Node, entry->Context))
            {
                show = FALSE;
                break;
            }
        }
    }

    return show;
}

VOID PhApplyTreeNewFilters(
    _In_ PPH_TN_FILTER_SUPPORT Support
    )
{
    ULONG i;

    for (i = 0; i < Support->NodeList->Count; i++)
    {
        PPH_TREENEW_NODE node;

        node = Support->NodeList->Items[i];
        node->Visible = PhApplyTreeNewFiltersToNode(Support, node);

        if (!node->Visible && node->Selected)
        {
            node->Selected = FALSE;
        }
    }

    TreeNew_NodesStructured(Support->TreeNewHandle);
}

#define ID_COPY_CELL 136
#define ID_SYMBOL_COPY 40201

typedef struct _PH_COPY_CELL_CONTEXT
{
    HWND TreeNewHandle;
    ULONG Id; // column ID
    PPH_STRING MenuItemText;
} PH_COPY_CELL_CONTEXT, *PPH_COPY_CELL_CONTEXT;

VOID NTAPI PhpCopyCellEMenuItemDeleteFunction(
    _In_ struct _PH_EMENU_ITEM *Item
    )
{
    PPH_COPY_CELL_CONTEXT context;

    context = Item->Context;
    PhDereferenceObject(context->MenuItemText);
    PhFree(context);
}

BOOLEAN PhInsertCopyCellEMenuItem(
    _In_ struct _PH_EMENU_ITEM *Menu,
    _In_ ULONG InsertAfterId,
    _In_ HWND TreeNewHandle,
    _In_ PPH_TREENEW_COLUMN Column
    )
{
    PPH_EMENU_ITEM parentItem;
    ULONG indexInParent;
    PPH_COPY_CELL_CONTEXT context;
    PH_STRINGREF columnText;
    PPH_STRING escapedText;
    PPH_STRING menuItemText;
    PPH_EMENU_ITEM copyCellItem;

    if (!Column)
        return FALSE;

    if (!PhFindEMenuItemEx(Menu, 0, NULL, InsertAfterId, &parentItem, &indexInParent))
        return FALSE;

    indexInParent++;

    context = PhAllocate(sizeof(PH_COPY_CELL_CONTEXT));
    context->TreeNewHandle = TreeNewHandle;
    context->Id = Column->Id;

    PhInitializeStringRef(&columnText, Column->Text);
    escapedText = PhEscapeStringForMenuPrefix(&columnText);
    menuItemText = PhFormatString(L"Copy \"%s\"", escapedText->Buffer);
    PhDereferenceObject(escapedText);
    copyCellItem = PhCreateEMenuItem(0, ID_COPY_CELL, menuItemText->Buffer, NULL, context);
    copyCellItem->DeleteFunction = PhpCopyCellEMenuItemDeleteFunction;
    context->MenuItemText = menuItemText;

    if (Column->CustomDraw)
        copyCellItem->Flags |= PH_EMENU_DISABLED;

    PhInsertEMenuItem(parentItem, copyCellItem, indexInParent);

    return TRUE;
}

BOOLEAN PhHandleCopyCellEMenuItem(
    _In_ struct _PH_EMENU_ITEM *SelectedItem
    )
{
    PPH_COPY_CELL_CONTEXT context;
    PH_STRING_BUILDER stringBuilder;
    ULONG count;
    ULONG selectedCount;
    ULONG i;
    PPH_TREENEW_NODE node;
    PH_TREENEW_GET_CELL_TEXT getCellText;

    if (!SelectedItem)
        return FALSE;
    if (SelectedItem->Id != ID_COPY_CELL)
        return FALSE;

    context = SelectedItem->Context;

    PhInitializeStringBuilder(&stringBuilder, 0x100);
    count = TreeNew_GetFlatNodeCount(context->TreeNewHandle);
    selectedCount = 0;

    for (i = 0; i < count; i++)
    {
        node = TreeNew_GetFlatNode(context->TreeNewHandle, i);

        if (node && node->Selected)
        {
            selectedCount++;

            getCellText.Flags = 0;
            getCellText.Node = node;
            getCellText.Id = context->Id;
            PhInitializeEmptyStringRef(&getCellText.Text);
            TreeNew_GetCellText(context->TreeNewHandle, &getCellText);

            PhAppendStringBuilder(&stringBuilder, &getCellText.Text);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
        }
    }

    if (stringBuilder.String->Length != 0 && selectedCount == 1)
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhSetClipboardString(context->TreeNewHandle, &stringBuilder.String->sr);
    PhDeleteStringBuilder(&stringBuilder);

    return TRUE;
}

#pragma endregion

VOID PvSetListViewImageList(
    _In_ HWND WindowHandle,
    _In_ HWND ListViewHandle
    )
{
    HIMAGELIST listViewImageList;
    LONG dpiValue = PhGetWindowDpi(WindowHandle);

    if (listViewImageList = ListView_GetImageList(ListViewHandle, LVSIL_SMALL))
    {
        PhImageListSetIconSize(
            listViewImageList,
            2,
            PhGetDpi(20, dpiValue)
            );
    }
    else
    {
        if (listViewImageList = PhImageListCreate(
            2,
            PhGetDpi(20, dpiValue),
            ILC_MASK | ILC_COLOR32,
            1,
            1
            ))
        {
            ListView_SetImageList(ListViewHandle, listViewImageList, LVSIL_SMALL);
        }
    }
}

VOID PvSetTreeViewImageList(
    _In_ HWND WindowHandle,
    _In_ HWND TreeViewHandle
    )
{
    HIMAGELIST treeViewImageList;
    LONG dpiValue = PhGetWindowDpi(WindowHandle);

    if (treeViewImageList = TreeView_GetImageList(TreeViewHandle, TVSIL_NORMAL))
    {
        PhImageListSetIconSize(
            treeViewImageList,
            2,
            PhGetDpi(24, dpiValue)
            );
    }
    else
    {
        if (treeViewImageList = PhImageListCreate(
            2,
            PhGetDpi(24, dpiValue),
            ILC_MASK | ILC_COLOR32,
            1,
            1
            ))
        {
            TreeView_SetImageList(TreeViewHandle, treeViewImageList, TVSIL_NORMAL);
        }
    }
}

PPH_STRING PvHashBuffer(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    PPH_STRING value = NULL;
    PH_HASH_CONTEXT hashContext;
    UCHAR hash[32];

    __try
    {
        PhInitializeHash(&hashContext, Md5HashAlgorithm); // PhGetIntegerSetting(L"HashAlgorithm")
        PhUpdateHash(&hashContext, Buffer, Length);

        if (PhFinalHash(&hashContext, hash, 16, NULL))
        {
            value = PhBufferToHexString(hash, 16);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //message = PH_AUTO(PhGetNtMessage(GetExceptionCode()));
        value = PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode())); // WIN32_FROM_NTSTATUS
    }

    return value;
}
