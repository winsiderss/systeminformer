/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2026
 *
 */

#include <ph.h>
#include <guisup.h>
#include <guisupview.h>

#include <commoncontrols.h>
#include <wincodec.h>
#include <uxtheme.h>

/**
 * Adds a column to a list-view control with DPI scaling applied to width.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param ListViewDpi The DPI value used for scaling the column width.
 * \param Index The index of the new column.
 * \param DisplayIndex The display position order index.
 * \param SubItemIndex The sub-item index.
 * \param Format Alignment and formatting flags.
 * \param Width The unscaled width of the column in pixels.
 * \param Text The column header text.
 * \return LONG The index of the new column, or INT_ERROR on failure.
 */
LONG PhAddListViewColumnDpi(
    _In_ HWND ListViewHandle,
    _In_ LONG ListViewDpi,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    )
{
    LVCOLUMN column;

    memset(&column, 0, sizeof(LVCOLUMN));
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
    column.fmt = Format;
    column.cx = WindowsVersion < WINDOWS_10 ? Width : PhScaleToDisplay(Width, ListViewDpi);
    column.pszText = const_cast<PWSTR>(Text);
    column.iSubItem = SubItemIndex;
    column.iOrder = DisplayIndex;

    return ListView_InsertColumn(ListViewHandle, Index, &column);
}

/**
 * Adds a column via the IListView interface with DPI scaling applied to width.
 *
 * \param ListView A pointer to the IListView interface.
 * \param ListViewDpi The DPI value used for scaling the column width.
 * \param Index The index of the new column.
 * \param DisplayIndex The display position order index.
 * \param SubItemIndex The sub-item index.
 * \param Format Alignment and formatting flags.
 * \param Width The unscaled width of the column in pixels.
 * \param Text The column header text.
 * \return LONG The index of the new column, or INT_ERROR on failure.
 */
LONG PhAddIListViewColumnDpi(
    _In_ IListView* ListView,
    _In_ LONG ListViewDpi,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    )
{
    LVCOLUMN column;
    LONG index;

    memset(&column, 0, sizeof(LVCOLUMN));
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
    column.fmt = Format;
    column.cx = WindowsVersion < WINDOWS_10 ? Width : PhScaleToDisplay(Width, ListViewDpi);
    column.pszText = const_cast<PWSTR>(Text);
    column.iSubItem = SubItemIndex;
    column.iOrder = DisplayIndex;

    if (SUCCEEDED(ListView->InsertColumn(Index, &column, &index)))
        return index;

    return INT_ERROR;
}

/**
 * Adds a column to a list-view control using the window DPI.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param Index The index of the new column.
 * \param DisplayIndex The display position order index.
 * \param SubItemIndex The sub-item index.
 * \param Format Alignment and formatting flags.
 * \param Width The unscaled width of the column in pixels.
 * \param Text The column header text.
 * \return LONG The index of the new column, or INT_ERROR on failure.
 */
LONG PhAddListViewColumn(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    )
{
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(ListViewHandle);

    return PhAddListViewColumnDpi(
        ListViewHandle,
        dpiValue,
        Index,
        DisplayIndex,
        SubItemIndex,
        Format,
        Width,
        Text
        );
}

/**
 * Adds a column via the IListView interface using the window DPI.
 *
 * \param ListView A pointer to the IListView interface.
 * \param Index The index of the new column.
 * \param DisplayIndex The display position order index.
 * \param SubItemIndex The sub-item index.
 * \param Format Alignment and formatting flags.
 * \param Width The unscaled width of the column in pixels.
 * \param Text The column header text.
 * \return LONG The index of the new column, or INT_ERROR on failure.
 */
LONG PhAddIListViewColumn(
    _In_ IListView* ListView,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    )
{
    LONG dpiValue;
    HWND windowHandle;

    if (!SUCCEEDED(ListView->GetHeaderControl(&windowHandle)))
        return INT_ERROR;

    dpiValue = PhGetWindowDpi(windowHandle);

    return PhAddIListViewColumnDpi(
        ListView,
        dpiValue,
        Index,
        DisplayIndex,
        SubItemIndex,
        Format,
        Width,
        Text
        );
}

/**
 * Adds an item to a list-view control.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param Index The index to insert the item at.
 * \param Text The item text.
 * \param Param Optional user-defined parameter value.
 * \return LONG The index of the new item, or INT_ERROR on failure.
 */
LONG PhAddListViewItem(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = Index;
    item.iSubItem = 0;
    item.pszText = const_cast<PWSTR>(Text);
    item.lParam = reinterpret_cast<LPARAM>(Param);

    return ListView_InsertItem(ListViewHandle, &item);
}

/**
 * Adds an item to a list-view via the IListView interface.
 *
 * \param ListView A pointer to the IListView interface.
 * \param Index The index to insert the item at.
 * \param Text The item text.
 * \param Param Optional user-defined parameter value.
 * \return LONG The index of the new item, or INT_ERROR on failure.
 */
LONG PhAddIListViewItem(
    _In_ IListView* ListView,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    )
{
    LVITEM item;
    LONG index;

    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = Index;
    item.iSubItem = 0;
    item.pszText = const_cast<PWSTR>(Text);
    item.lParam = reinterpret_cast<LPARAM>(Param);

    if (SUCCEEDED(ListView->InsertItem(&item, &index)))
        return index;

    return INT_ERROR;
}

/**
 * Searches for an item in a list-view matching search flags.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param StartIndex The index of the item to begin searching from.
 * \param Flags Search relationship flags.
 * \return LONG The index of the matching item, or INT_ERROR if not found.
 */
LONG PhFindListViewItemByFlags(
    _In_ HWND ListViewHandle,
    _In_ LONG StartIndex,
    _In_ ULONG Flags
    )
{
    return ListView_GetNextItem(ListViewHandle, StartIndex, Flags);
}

/**
 * Searches for an item via the IListView interface matching search flags.
 *
 * \param ListView A pointer to the IListView interface.
 * \param StartIndex The index of the item to begin searching from.
 * \param Flags Search relationship flags.
 * \return LONG The index of the matching item, or INT_ERROR if not found.
 */
LONG PhFindIListViewItemByFlags(
    _In_ IListView* ListView,
    _In_ LONG StartIndex,
    _In_ ULONG Flags
    )
{
    LVITEMINDEX itemIndex;
    LVITEMINDEX nextItemIndex;

    itemIndex.iItem = StartIndex;
    itemIndex.iGroup = INT_ERROR;

    if (SUCCEEDED(ListView->GetNextItem(itemIndex, Flags, &nextItemIndex)))
        return nextItemIndex.iItem;

    return INT_ERROR;
}

/**
 * Searches for an item in a list-view with a matching parameter value.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param StartIndex The index of the item to begin searching from.
 * \param Param The user parameter value to match.
 * \return LONG The index of the matching item, or INT_ERROR if not found.
 */
LONG PhFindListViewItemByParam(
    _In_ HWND ListViewHandle,
    _In_ LONG StartIndex,
    _In_opt_ PVOID Param
    )
{
    LVFINDINFO findInfo;

    findInfo.flags = LVFI_PARAM;
    findInfo.lParam = reinterpret_cast<LPARAM>(Param);

    return ListView_FindItem(ListViewHandle, StartIndex, &findInfo);
}

/**
 * Searches for an item via IListView with a matching parameter value.
 *
 * \param ListView A pointer to the IListView interface.
 * \param StartIndex The index of the item to begin searching from.
 * \param Param The user parameter value to match.
 * \return LONG The index of the matching item, or INT_ERROR if not found.
 */
LONG PhFindIListViewItemByParam(
    _In_ IListView* ListView,
    _In_ LONG StartIndex,
    _In_opt_ PVOID Param
    )
{
    LVITEMINDEX itemIndex;
    LVITEMINDEX foundIndex;
    LVFINDINFO findInfo;

    itemIndex.iItem = StartIndex;
    itemIndex.iGroup = INT_ERROR;

    findInfo.flags = LVFI_PARAM;
    findInfo.lParam = reinterpret_cast<LPARAM>(Param);

    if (SUCCEEDED(ListView->FindItem(itemIndex, &findInfo, &foundIndex)))
    {
#if DEBUG
        HWND windowHandle = nullptr;
        ListView->GetHeaderControl(&windowHandle);
        windowHandle = GetParent(windowHandle);
        LONG index = PhFindListViewItemByParam(windowHandle, StartIndex, Param);
        assert(index == foundIndex.iItem); // Items changed during enumeration. (dmex)
#endif

        return foundIndex.iItem;
    }

    return INT_ERROR;
}

/**
 * Retrieves the image index for a list-view item.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param Index The item index.
 * \param ImageIndex Receives the image list index.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhGetListViewItemImageIndex(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _Out_ PLONG ImageIndex
    )
{
    LVITEM item;

    item.mask = LVIF_IMAGE;
    item.iItem = Index;
    item.iSubItem = 0;

    if (!ListView_GetItem(ListViewHandle, &item))
        return FALSE;

    *ImageIndex = item.iImage;

    return TRUE;
}

/**
 * Retrieves the image index for a list-view item via IListView.
 *
 * \param ListView A pointer to the IListView interface.
 * \param Index The item index.
 * \param ImageIndex Receives the image list index.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhGetIListViewItemImageIndex(
    _In_ IListView* ListView,
    _In_ LONG Index,
    _Out_ PLONG ImageIndex
    )
{
    LVITEM item;

    item.mask = LVIF_IMAGE;
    item.iItem = Index;
    item.iSubItem = 0;

    if (!SUCCEEDED(ListView->GetItem(&item)))
        return FALSE;

    *ImageIndex = item.iImage;

    return TRUE;
}

/**
 * Retrieves the user-defined parameter value for a list-view item.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param Index The item index.
 * \param Param Receives the item parameter pointer.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhGetListViewItemParam(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _Outptr_ PVOID* Param
    )
{
    LVITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = Index;
    item.iSubItem = 0;

    if (!ListView_GetItem(ListViewHandle, &item))
        return FALSE;

    *Param = reinterpret_cast<PVOID>(item.lParam);

    return TRUE;
}

/**
 * Retrieves the user-defined parameter value for a list-view item via IListView.
 *
 * \param ListView A pointer to the IListView interface.
 * \param Index The item index.
 * \param Param Receives the item parameter pointer.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhGetIListViewItemParam(
    _In_ IListView* ListView,
    _In_ LONG Index,
    _Outptr_ PVOID* Param
    )
{
    LVITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = Index;
    item.iSubItem = 0;

    if (!SUCCEEDED(ListView->GetItem(&item)))
        return FALSE;

    *Param = reinterpret_cast<PVOID>(item.lParam);

    return TRUE;
}

/**
 * Sets the user-defined parameter value for a list-view item.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param Index The item index.
 * \param Param The parameter value to set.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhSetListViewItemParam(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ PVOID Param
    )
{
    LVITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = Index;
    item.lParam = reinterpret_cast<LPARAM>(Param);

    return !!ListView_SetItem(ListViewHandle, &item);
}

/**
 * Sets the user-defined parameter value for a list-view item via IListView.
 *
 * \param ListView A pointer to the IListView interface.
 * \param Index The item index.
 * \param Param The parameter value to set.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhSetIListViewItemParam(
    _In_ IListView* ListView,
    _In_ LONG Index,
    _In_ PVOID Param
    )
{
    LVITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = Index;
    item.lParam = reinterpret_cast<LPARAM>(Param);

    return SUCCEEDED(ListView->SetItem(&item));
}

/**
 * Removes an item from a list-view control.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param Index The item index to remove.
 */
VOID PhRemoveListViewItem(
    _In_ HWND ListViewHandle,
    _In_ LONG Index
    )
{
    ListView_DeleteItem(ListViewHandle, Index);
}

/**
 * Removes an item from a list-view via IListView.
 *
 * \param ListView A pointer to the IListView interface.
 * \param Index The item index to remove.
 */
VOID PhRemoveIListViewItem(
    _In_ IListView* ListView,
    _In_ LONG Index
    )
{
    ListView->DeleteItem(Index);
}

/**
 * Sets the image index for a list-view item.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param Index The item index.
 * \param ImageIndex The image list index to assign.
 */
VOID PhSetListViewItemImageIndex(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ LONG ImageIndex
    )
{
    LVITEM item;

    item.mask = LVIF_IMAGE;
    item.iItem = Index;
    item.iSubItem = 0;
    item.iImage = ImageIndex;

    ListView_SetItem(ListViewHandle, &item);
}

/**
 * Sets the image index for a list-view item via IListView.
 *
 * \param ListView A pointer to the IListView interface.
 * \param Index The item index.
 * \param ImageIndex The image list index to assign.
 */
VOID PhSetIListViewItemImageIndex(
    _In_ IListView* ListView,
    _In_ LONG Index,
    _In_ LONG ImageIndex
    )
{
    LVITEM item;

    item.mask = LVIF_IMAGE;
    item.iItem = Index;
    item.iSubItem = 0;
    item.iImage = ImageIndex;

    ListView->SetItem(&item);
}

/**
 * Sets the text for a sub-item in a list-view control.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param Index The item index.
 * \param SubItemIndex The sub-item column index.
 * \param Text The text to set.
 */
VOID PhSetListViewSubItem(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ LONG SubItemIndex,
    _In_ PCWSTR Text
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT;
    item.iItem = Index;
    item.iSubItem = SubItemIndex;
    item.pszText = const_cast<PWSTR>(Text);

    ListView_SetItem(ListViewHandle, &item);
}

/**
 * Sets the text for a sub-item via IListView.
 *
 * \param ListView A pointer to the IListView interface.
 * \param Index The item index.
 * \param SubItemIndex The sub-item column index.
 * \param Text The text to set.
 */
VOID PhSetIListViewSubItem(
    _In_ IListView* ListView,
    _In_ LONG Index,
    _In_ LONG SubItemIndex,
    _In_ PCWSTR Text
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT;
    item.iItem = Index;
    item.iSubItem = SubItemIndex;
    item.pszText = const_cast<PWSTR>(Text);

    ListView->SetItem(&item);
}

/**
 * Invalidates all items in a list-view control to trigger redrawing.
 *
 * \param ListViewHandle A handle to the list-view window.
 */
VOID PhRedrawListViewItems(
    _In_ HWND ListViewHandle
    )
{
    ListView_RedrawItems(ListViewHandle, 0, INT_MAX);
    // Note: UpdateWindow() is a workaround for ListView_RedrawItems() failing to send LVN_GETDISPINFO
    // and fixes RedrawItems() graphical artifacts when the listview doesn't have foreground focus. (dmex)
    UpdateWindow(ListViewHandle);
}

/**
 * Invalidates all items via IListView to trigger redrawing.
 *
 * \param ListView A pointer to the IListView interface.
 * \param ListViewHandle A handle to the list-view window.
 */
VOID PhRedrawIListViewItems(
    _In_ IListView* ListView,
    _In_ HWND ListViewHandle
    )
{
    ListView->RedrawItems(0, INT_MAX);
    // Note: UpdateWindow() is a workaround for ListView_RedrawItems() failing to send LVN_GETDISPINFO
    // and fixes RedrawItems() graphical artifacts when the listview doesn't have foreground focus. (dmex)
    UpdateWindow(ListViewHandle);
}

/**
 * Adds a group header to a list-view control.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param GroupId The unique identifier of the group.
 * \param Text The group title text.
 * \return LONG The group index, or INT_ERROR on failure.
 */
LONG PhAddListViewGroup(
    _In_ HWND ListViewHandle,
    _In_ LONG GroupId,
    _In_ PCWSTR Text
    )
{
    LVGROUP group;

    memset(&group, 0, sizeof(LVGROUP));
    group.cbSize = sizeof(LVGROUP);
    group.mask = LVGF_HEADER | LVGF_ALIGN | LVGF_STATE | LVGF_GROUPID;
    group.uAlign = LVGA_HEADER_LEFT;
    group.state = LVGS_COLLAPSIBLE;
    group.iGroupId = GroupId;
    group.pszHeader = const_cast<PWSTR>(Text);

    return static_cast<LONG>(ListView_InsertGroup(ListViewHandle, MAXUINT, &group));
}

/**
 * Adds a group header via IListView.
 *
 * \param ListView A pointer to the IListView interface.
 * \param GroupId The unique identifier of the group.
 * \param Text The group title text.
 * \return LONG The group index, or INT_ERROR on failure.
 */
LONG PhAddIListViewGroup(
    _In_ IListView* ListView,
    _In_ LONG GroupId,
    _In_ PCWSTR Text
    )
{
    LVGROUP group;
    LONG index = 0;

    memset(&group, 0, sizeof(LVGROUP));
    group.cbSize = sizeof(LVGROUP);
    group.mask = LVGF_HEADER | LVGF_ALIGN | LVGF_STATE | LVGF_GROUPID;
    group.uAlign = LVGA_HEADER_LEFT;
    group.state = LVGS_COLLAPSIBLE;
    group.iGroupId = GroupId;
    group.pszHeader = const_cast<PWSTR>(Text);

    if (SUCCEEDED(ListView->InsertGroup(MAXUINT, &group, &index)))
        return index;

    return INT_ERROR;
}

/**
 * Adds an item assigned to a specific group in a list-view control.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \param GroupId The group identifier to assign the item to.
 * \param Index The item index.
 * \param Text The item text.
 * \param Param Optional user-defined parameter value.
 * \return LONG The index of the new item, or INT_ERROR on failure.
 */
LONG PhAddListViewGroupItem(
    _In_ HWND ListViewHandle,
    _In_ LONG GroupId,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT | LVIF_GROUPID;
    item.iItem = Index;
    item.iSubItem = 0;
    item.pszText = const_cast<PWSTR>(Text);
    item.iGroupId = GroupId;

    if (Param)
    {
        item.mask |= LVIF_PARAM;
        item.lParam = reinterpret_cast<LPARAM>(Param);
    }

    return ListView_InsertItem(ListViewHandle, &item);
}

/**
 * Adds an item assigned to a specific group via IListView.
 *
 * \param ListView A pointer to the IListView interface.
 * \param GroupId The group identifier to assign the item to.
 * \param Index The item index.
 * \param Text The item text.
 * \param Param Optional user-defined parameter value.
 * \return LONG The index of the new item, or INT_ERROR on failure.
 */
LONG PhAddIListViewGroupItem(
    _In_ IListView* ListView,
    _In_ LONG GroupId,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    )
{
    LVITEM item;
    LONG index;

    item.mask = LVIF_TEXT | LVIF_GROUPID;
    item.iItem = Index;
    item.iSubItem = 0;
    item.pszText = const_cast<PWSTR>(Text);
    item.iGroupId = GroupId;

    if (Param)
    {
        item.mask |= LVIF_PARAM;
        item.lParam = reinterpret_cast<LPARAM>(Param);
    }

    if (SUCCEEDED(ListView->InsertItem(&item, &index)))
        return index;

    return INT_ERROR;
}

/**
 * Sets state flags for all items in a list-view control.
 *
 * \param WindowHandle A handle to the list-view window.
 * \param State The state flags to set.
 * \param Mask Mask specifying which state bits to modify.
 */
VOID PhSetStateAllListViewItems(
    _In_ HWND WindowHandle,
    _In_ ULONG State,
    _In_ ULONG Mask
    )
{
    LONG i;
    LONG count;

    count = ListView_GetItemCount(WindowHandle);

    if (count <= 0)
        return;

    for (i = 0; i < count; i++)
    {
        ListView_SetItemState(WindowHandle, i, State, Mask);
    }
}

/**
 * Retrieves the parameter value of the first selected item in a list-view control.
 *
 * \param WindowHandle A handle to the list-view window.
 * \return PVOID The item parameter pointer, or NULL if no item is selected.
 */
PVOID PhGetSelectedListViewItemParam(
    _In_ HWND WindowHandle
    )
{
    LONG index;
    PVOID param;

    index = PhFindListViewItemByFlags(
        WindowHandle,
        INT_ERROR,
        LVNI_SELECTED
        );

    if (index != INT_ERROR)
    {
        if (PhGetListViewItemParam(
            WindowHandle,
            index,
            &param
            ))
        {
            return param;
        }
    }

    return nullptr;
}

/**
 * Retrieves an allocated array of parameter pointers for all selected list-view items.
 *
 * \param WindowHandle A handle to the list-view window.
 * \param Items Receives an allocated array of item parameter pointers.
 * \param NumberOfItems Receives the count of selected items.
 * \return BOOLEAN TRUE if selected items were retrieved, FALSE otherwise.
 */
BOOLEAN PhGetSelectedListViewItemParams(
    _In_ HWND WindowHandle,
    _Out_ PVOID **Items,
    _Out_ PULONG NumberOfItems
    )
{
    PH_ARRAY array;
    LONG index;
    PVOID param;

    PhInitializeArray(&array, sizeof(PVOID), 2);
    index = INT_ERROR;

    while ((index = PhFindListViewItemByFlags(
        WindowHandle,
        index,
        LVNI_SELECTED
        )) != INT_ERROR)
    {
        if (PhGetListViewItemParam(WindowHandle, index, &param))
            PhAddItemArray(&array, &param);
    }

    *NumberOfItems = static_cast<ULONG>(PhFinalArrayCount(&array));

    if (*NumberOfItems == 0)
    {
        *Items = nullptr;
        PhDeleteArray(&array);
        return FALSE;
    }

    *Items = static_cast<PVOID*>(PhFinalArrayItems(&array));
    return TRUE;
}

/**
 * Retrieves an allocated array of parameter pointers for all selected items via IListView.
 *
 * \param ListView A pointer to the IListView interface.
 * \param Items Receives an allocated array of item parameter pointers.
 * \param NumberOfItems Receives the count of selected items.
 * \return BOOLEAN TRUE if selected items were retrieved, FALSE otherwise.
 */
BOOLEAN PhGetSelectedIListViewItemParams(
    _In_ IListView* ListView,
    _Out_ PVOID **Items,
    _Out_ PULONG NumberOfItems
    )
{
    PH_ARRAY array;
    LONG index;
    PVOID param;

    PhInitializeArray(&array, sizeof(PVOID), 2);
    index = INT_ERROR;

    while ((index = PhFindIListViewItemByFlags(
        ListView,
        index,
        LVNI_SELECTED
        )) != INT_ERROR)
    {
        if (PhGetIListViewItemParam(ListView, index, &param))
            PhAddItemArray(&array, &param);
    }

    *NumberOfItems = static_cast<ULONG>(PhFinalArrayCount(&array));

    if (*NumberOfItems == 0)
    {
        *Items = nullptr;
        PhDeleteArray(&array);
        return FALSE;
    }

    *Items = static_cast<PVOID*>(PhFinalArrayItems(&array));
    return TRUE;
}

/**
 * Retrieves the client bounding rectangle via IListView.
 *
 * \param ListView A pointer to the IListView interface.
 * \param ClientRect Receives the client rectangle.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhGetIListViewClientRect(
    _In_ IListView* ListView,
    _Inout_ PRECT ClientRect
    )
{
    return SUCCEEDED(ListView->GetClientRectangle(FALSE, ClientRect));
}

/**
 * Retrieves the bounding rectangle for an item via IListView.
 *
 * \param ListView A pointer to the IListView interface.
 * \param StartIndex The item index.
 * \param Flags Portion flags for the rectangle.
 * \param ItemRect Receives the bounding rectangle.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhGetIListViewItemRect(
    _In_ IListView* ListView,
    _In_ LONG StartIndex,
    _In_ ULONG Flags, // LVIR_SELECTBOUNDS | LVIR_BOUNDS
    _Inout_ PRECT ItemRect
    )
{
    LVITEMINDEX itemIndex;

    itemIndex.iItem = StartIndex;
    itemIndex.iGroup = INT_ERROR;

    return SUCCEEDED(ListView->GetItemRect(itemIndex, Flags, ItemRect));
}

/**
 * Initializes a wrapper context for list-view operations using either IListView or standard messages.
 *
 * \param ListViewHandle A handle to the list-view window.
 * \return PPH_LISTVIEW_CONTEXT A pointer to the initialized list-view context.
 */
PPH_LISTVIEW_CONTEXT PhListView_Initialize(
    _In_ HWND ListViewHandle
    )
{
    PPH_LISTVIEW_CONTEXT context;
    IListView* listviewInterface;

    context = static_cast<PPH_LISTVIEW_CONTEXT>(PhAllocateZero(sizeof(PH_LISTVIEW_CONTEXT)));
    context->ListViewHandle = ListViewHandle;
    context->ThreadId = UlongToHandle(GetWindowThreadProcessId(ListViewHandle, nullptr));

    if (listviewInterface = PhGetListViewInterface(ListViewHandle))
    {
        context->ListViewInterface = listviewInterface;
    }

    return context;
}

/**
 * Destroys a list-view context and releases any associated COM interfaces.
 *
 * \param Context A pointer to the list-view context.
 */
VOID PhListView_Destroy(
    _In_ PPH_LISTVIEW_CONTEXT Context
    )
{
    if (Context->ListViewInterface)
    {
        Context->ListViewInterface->Release();
        Context->ListViewInterface = nullptr;
    }

    PhFree(Context);
}

/**
 * Retrieves the number of items in the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param ItemCount Receives the total item count.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Use_decl_annotations_
BOOLEAN PhListView_GetItemCount(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Out_ PLONG ItemCount
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->GetItemCount(ItemCount)))
        {
            return TRUE;
        }
    }
    else
    {
        LONG count = ListView_GetItemCount(Context->ListViewHandle);

        if (count != INT_ERROR)
        {
            *ItemCount = count;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Sets the virtual item count for a list-view control.
 *
 * \param Context A pointer to the list-view context.
 * \param ItemCount The new item count.
 * \param Flags Count update flags.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_SetItemCount(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemCount,
    _In_ LV_LISTVIEW_SETITEMCOUNT_FLAGS Flags
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->SetItemCount(ItemCount, Flags)))
        {
            return TRUE;
        }
    }
    else
    {
        if (ListView_SetItemCountEx(Context->ListViewHandle, ItemCount, Flags))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Retrieves item attributes from the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param Item A pointer to the LVITEM structure to populate.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_GetItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Inout_ LVITEM* Item
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->GetItem(Item)))
            return TRUE;
    }
    else
    {
        if (ListView_GetItem(Context->ListViewHandle, Item))
            return TRUE;
    }

    return FALSE;
}

/**
 * Sets item attributes in the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param Item A pointer to the LVITEM structure containing attributes to set.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_SetItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LVITEM* Item
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->SetItem(Item)))
            return TRUE;
    }
    else
    {
        if (ListView_SetItem(Context->ListViewHandle, Item))
            return TRUE;
    }

    return FALSE;
}

/**
 * Retrieves the text of an item or sub-item in the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param ItemIndex The item index.
 * \param SubItemIndex The sub-item column index.
 * \param Buffer Buffer to receive the item text.
 * \param BufferSize Size of the buffer in characters.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhListView_GetItemText(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ LONG SubItemIndex,
    _Out_writes_(BufferSize) PWSTR Buffer,
    _In_ LONG BufferSize
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->GetItemText(
            ItemIndex,
            SubItemIndex,
            Buffer,
            BufferSize
            )))
        {
            return TRUE;
        }
    }
    else
    {
        LVITEM item;

        ZeroMemory(&item, sizeof(LVITEM));
        item.iSubItem = SubItemIndex;
        item.cchTextMax = BufferSize;
        item.pszText = Buffer;

        if (SendMessage(Context->ListViewHandle, LVM_GETITEMTEXTW, ItemIndex, reinterpret_cast<LPARAM>(&item)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Sets the text of an item or sub-item in the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param ItemIndex The item index.
 * \param SubItemIndex The sub-item column index.
 * \param Text The text string to set.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_SetItemText(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ LONG SubItemIndex,
    _In_ PWSTR Text
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->SetItemText(ItemIndex, SubItemIndex, Text)))
        {
            return TRUE;
        }
    }
    else
    {
        LVITEM item;

        ZeroMemory(&item, sizeof(LVITEM));
        item.iSubItem = SubItemIndex;
        item.pszText = Text;

        if (SendMessage(Context->ListViewHandle, LVM_SETITEMTEXTW, ItemIndex, reinterpret_cast<LPARAM>(&item)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Deletes an item from the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param ItemIndex The index of the item to delete.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_DeleteItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->DeleteItem(ItemIndex)))
        {
            return TRUE;
        }
    }
    else
    {
        if (ListView_DeleteItem(Context->ListViewHandle, ItemIndex))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Removes all items from the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_DeleteAllItems(
    _In_ PPH_LISTVIEW_CONTEXT Context
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->DeleteAllItems()))
            return TRUE;
    }
    else
    {
        if (ListView_DeleteAllItems(Context->ListViewHandle))
            return TRUE;
    }

    return FALSE;
}

/**
 * Inserts an item into the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param Item A pointer to the LVITEMW structure.
 * \param ItemIndex Optional receives the inserted item index.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Use_decl_annotations_
BOOLEAN PhListView_InsertItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LVITEMW* Item,
    _Out_opt_ PLONG ItemIndex
    )
{
    LONG index;

    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (!HR_SUCCESS(Context->ListViewInterface->InsertItem(Item, &index)))
            return FALSE;
    }
    else
    {
        index = ListView_InsertItem(Context->ListViewHandle, Item);

        if (index == INT_ERROR)
            return FALSE;
    }

    if (ItemIndex)
        *ItemIndex = index;

    return TRUE;
}

/**
 * Inserts a group into the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param InsertAt The position index to insert at.
 * \param Group A pointer to the LVGROUP structure.
 * \param GroupId Optional receives the inserted group ID.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Use_decl_annotations_
BOOLEAN PhListView_InsertGroup(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG InsertAt,
    _In_ LVGROUP* Group,
    _Out_opt_ PLONG GroupId
    )
{
    LONG index;

    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (!HR_SUCCESS(Context->ListViewInterface->InsertGroup(InsertAt, Group, &index)))
            return FALSE;
    }
    else
    {
        index = static_cast<LONG>(ListView_InsertGroup(Context->ListViewHandle, InsertAt, Group));

        if (index == INT_ERROR)
            return FALSE;
    }

    if (GroupId)
        *GroupId = index;

    return TRUE;
}

/**
 * Retrieves the state flags of a list-view item.
 *
 * \param Context A pointer to the list-view context.
 * \param ItemIndex The item index.
 * \param Mask Mask specifying the state bits to query.
 * \param State Receives the item state flags.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Use_decl_annotations_
BOOLEAN PhListView_GetItemState(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ ULONG Mask,
    _Out_ PULONG State
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        ULONG state = 0;

        if (HR_SUCCESS(Context->ListViewInterface->GetItemState(
            ItemIndex,
            0,
            static_cast<LV_LISTVIEW_ITEM_STATE_FLAGS>(Mask),
            reinterpret_cast<LV_LISTVIEW_ITEM_STATE_FLAGS*>(&state)
            )))
        {
            *State = state;
            return TRUE;
        }
    }
    else
    {
        *State = ListView_GetItemState(Context->ListViewHandle, ItemIndex, Mask);
        return TRUE;
    }

    return FALSE;
}

/**
 * Sets the state flags of a list-view item.
 *
 * \param Context A pointer to the list-view context.
 * \param ItemIndex The item index.
 * \param State The state flags to set.
 * \param Mask Mask specifying the state bits to modify.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_SetItemState(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ ULONG State,
    _In_ ULONG Mask
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->SetItemState(
            ItemIndex,
            0,
            static_cast<LV_LISTVIEW_ITEM_STATE_FLAGS>(Mask),
            static_cast<LV_LISTVIEW_ITEM_STATE_FLAGS>(State)
            )))
        {
            return TRUE;
        }
    }
    else
    {
        LVITEM item;

        ZeroMemory(&item, sizeof(LVITEM));
        item.state = State;
        item.stateMask = Mask;
        item.mask = LVIF_STATE;
        item.iItem = ItemIndex;

        if (SendMessage(Context->ListViewHandle, LVM_SETITEMSTATE, ItemIndex, reinterpret_cast<LPARAM>(&item)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Sorts the items in a list-view control using a comparison callback.
 *
 * \param Context A pointer to the list-view context.
 * \param SortingByIndex TRUE to pass item indices to compare, FALSE to pass item lParam values.
 * \param Compare The comparison callback function.
 * \param CompareContext User-defined context passed to the comparison function.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_SortItems(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ BOOL SortingByIndex,
    _In_ PFNLVCOMPARE Compare,
    _In_ PVOID CompareContext
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->SortItems(
            SortingByIndex,
            reinterpret_cast<LPARAM>(CompareContext),
            Compare
            )))
        {
            return TRUE;
        }
    }
    else
    {
        if (SortingByIndex)
        {
            if (ListView_SortItemsEx(
                Context->ListViewHandle,
                Compare,
                CompareContext
                ))
            {
                return TRUE;
            }
        }
        else
        {
            if (ListView_SortItems(
                Context->ListViewHandle,
                Compare,
                CompareContext
                ))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/**
 * Retrieves column attributes from the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param ColumnIndex The column index.
 * \param Column A pointer to the LV_COLUMN structure to populate.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_GetColumn(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ ULONG ColumnIndex,
    _Inout_ LV_COLUMN* Column
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->GetColumn(ColumnIndex, Column)))
            return TRUE;
    }
    else
    {
        if (ListView_GetColumn(Context->ListViewHandle, ColumnIndex, Column))
            return TRUE;
    }

    return FALSE;
}

/**
 * Sets column attributes in the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param ColumnIndex The column index.
 * \param Column A pointer to the LV_COLUMN structure containing attributes to set.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_SetColumn(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ ULONG ColumnIndex,
    _In_ LV_COLUMN* Column
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->SetColumn(ColumnIndex, Column)))
            return TRUE;
    }
    else
    {
        if (ListView_SetColumn(Context->ListViewHandle, ColumnIndex, Column))
            return TRUE;
    }

    return FALSE;
}

/**
 * Sets the width of a list-view column.
 *
 * \param Context A pointer to the list-view context.
 * \param ColumnIndex The column index.
 * \param Width The column width in pixels.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_SetColumnWidth(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ ULONG ColumnIndex,
    _In_ ULONG Width
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->SetColumnWidth(ColumnIndex, Width)))
            return TRUE;
    }
    else
    {
        if (ListView_SetColumnWidth(Context->ListViewHandle, ColumnIndex, Width))
            return TRUE;
    }

    return FALSE;
}

/**
 * Retrieves the handle to the header control of the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param WindowHandle Receives the header control window handle.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhListView_GetHeader(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Out_ HWND* WindowHandle
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        HWND headerWindowHandle = nullptr;

        if (HR_SUCCESS(Context->ListViewInterface->GetHeaderControl(&headerWindowHandle)))
        {
            *WindowHandle = headerWindowHandle;
            return TRUE;
        }
    }
    else
    {
        HWND headerWindowHandle;

        if (headerWindowHandle = ListView_GetHeader(Context->ListViewHandle))
        {
            *WindowHandle = headerWindowHandle;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Retrieves the handle to the tooltip control of the list-view.
 *
 * \param Context A pointer to the list-view context.
 * \param WindowHandle Receives the tooltip control window handle.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhListView_GetToolTip(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Out_ HWND* WindowHandle
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        HWND tooltipWindowHandle = nullptr;

        if (HR_SUCCESS(Context->ListViewInterface->GetToolTip(&tooltipWindowHandle)))
        {
            *WindowHandle = tooltipWindowHandle;
            return TRUE;
        }
    }
    else
    {
        HWND tooltipWindowHandle;

        if (tooltipWindowHandle = ListView_GetToolTips(Context->ListViewHandle))
        {
            *WindowHandle = tooltipWindowHandle;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Adds a column to the list-view via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param Index The index of the new column.
 * \param DisplayIndex The display position order index.
 * \param SubItemIndex The sub-item index.
 * \param Format Alignment and formatting flags.
 * \param Width The column width in pixels.
 * \param Text The column header text.
 * \return LONG The index of the new column, or INT_ERROR on failure.
 */
LONG PhListView_AddColumn(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return PhAddIListViewColumn(Context->ListViewInterface, Index, DisplayIndex, SubItemIndex, Format, Width, Text);
    }
    else
    {
        return PhAddListViewColumn(Context->ListViewHandle, Index, DisplayIndex, SubItemIndex, Format, Width, Text);
    }
}

/**
 * Adds an item to the list-view via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param Index The index to insert the item at.
 * \param Text The item text.
 * \param Param Optional user-defined parameter value.
 * \return LONG The index of the new item, or INT_ERROR on failure.
 */
LONG PhListView_AddItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return PhAddIListViewItem(Context->ListViewInterface, Index, Text, Param);
    }
    else
    {
        return PhAddListViewItem(Context->ListViewHandle, Index, Text, Param);
    }
}

/**
 * Searches for an item in the list-view matching search flags via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param StartIndex The index of the item to begin searching from.
 * \param Flags Search relationship flags.
 * \return LONG The index of the matching item, or INT_ERROR if not found.
 */
LONG PhListView_FindItemByFlags(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG StartIndex,
    _In_ ULONG Flags
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return PhFindIListViewItemByFlags(Context->ListViewInterface, StartIndex, Flags);
    }
    else
    {
        return PhFindListViewItemByFlags(Context->ListViewHandle, StartIndex, Flags);
    }
}

/**
 * Searches for an item in the list-view with a matching parameter value via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param StartIndex The index of the item to begin searching from.
 * \param Param The user parameter value to match.
 * \return LONG The index of the matching item, or INT_ERROR if not found.
 */
LONG PhListView_FindItemByParam(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG StartIndex,
    _In_opt_ PVOID Param
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return PhFindIListViewItemByParam(Context->ListViewInterface, StartIndex, Param);
    }
    else
    {
        return PhFindListViewItemByParam(Context->ListViewHandle, StartIndex, Param);
    }
}

/**
 * Retrieves the parameter value of a list-view item via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param Index The item index.
 * \param Param Receives the item parameter pointer.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhListView_GetItemParam(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG Index,
    _Outptr_ PVOID* Param
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return PhGetIListViewItemParam(Context->ListViewInterface, Index, Param);
    }
    else
    {
        return PhGetListViewItemParam(Context->ListViewHandle, Index, Param);
    }
}

/**
 * Sets the sub-item text in the list-view via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param Index The item index.
 * \param SubItemIndex The sub-item column index.
 * \param Text The text to set.
 */
VOID PhListView_SetSubItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG Index,
    _In_ LONG SubItemIndex,
    _In_ PCWSTR Text
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        PhSetIListViewSubItem(Context->ListViewInterface, Index, SubItemIndex, Text);
    }
    else
    {
        PhSetListViewSubItem(Context->ListViewHandle, Index, SubItemIndex, Text);
    }
}

/**
 * Invalidates all items in the list-view to trigger redrawing via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 */
VOID PhListView_RedrawItems(
    _In_ PPH_LISTVIEW_CONTEXT Context
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        PhRedrawIListViewItems(Context->ListViewInterface, Context->ListViewHandle);
    }
    else
    {
        PhRedrawListViewItems(Context->ListViewHandle);
    }
}

/**
 * Adds a group header to the list-view via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param GroupId The unique identifier of the group.
 * \param Text The group title text.
 * \return LONG The group index, or INT_ERROR on failure.
 */
LONG PhListView_AddGroup(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ PCWSTR Text
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return PhAddIListViewGroup(Context->ListViewInterface, GroupId, Text);
    }
    else
    {
        return PhAddListViewGroup(Context->ListViewHandle, GroupId, Text);
    }
}

/**
 * Adds an item assigned to a specific group via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param GroupId The group identifier to assign the item to.
 * \param Index The item index.
 * \param Text The item text.
 * \param Param Optional user-defined parameter value.
 * \return LONG The index of the new item, or INT_ERROR on failure.
 */
LONG PhListView_AddGroupItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return PhAddIListViewGroupItem(Context->ListViewInterface, GroupId, Index, Text, Param);
    }
    else
    {
        return PhAddListViewGroupItem(Context->ListViewHandle, GroupId, Index, Text, Param);
    }
}

/**
 * Sets state flags for all items in the list-view via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param State The state flags to set.
 * \param Mask Mask specifying which state bits to modify.
 */
VOID PhListView_SetStateAllItems(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ ULONG State,
    _In_ ULONG Mask
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        LONG i;
        LONG count;

        if (HR_SUCCESS(Context->ListViewInterface->GetItemCount(&count)))
        {
            for (i = 0; i < count; i++)
            {
                Context->ListViewInterface->SetItemState(i, 0, static_cast<LV_LISTVIEW_ITEM_STATE_FLAGS>(Mask), static_cast<LV_LISTVIEW_ITEM_STATE_FLAGS>(State));
            }
        }
    }
    else
    {
        PhSetStateAllListViewItems(Context->ListViewHandle, State, Mask);
    }
}

/**
 * Retrieves the parameter value of the first selected item in the list-view via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \return PVOID The item parameter pointer, or NULL if no item is selected.
 */
PVOID PhListView_GetSelectedItemParam(
    _In_ PPH_LISTVIEW_CONTEXT Context
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        LONG index;
        PVOID param;

        index = PhFindIListViewItemByFlags(
            Context->ListViewInterface,
            INT_ERROR,
            LVNI_SELECTED
            );

        if (index != INT_ERROR)
        {
            if (PhGetIListViewItemParam(Context->ListViewInterface, index, &param))
                return param;
        }

        return nullptr;
    }
    else
    {
        return PhGetSelectedListViewItemParam(Context->ListViewHandle);
    }
}

/**
 * Retrieves the number of selected items in the list-view via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param SelectedCount Receives the selected item count.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhListView_GetSelectedCount(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Out_ PLONG SelectedCount
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return HR_SUCCESS(Context->ListViewInterface->GetSelectedCount(SelectedCount));
    }
    else
    {
        LONG count = ListView_GetSelectedCount(Context->ListViewHandle);
        
        if (count != INT_ERROR)
        {
            *SelectedCount = count;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Retrieves an allocated array of parameter pointers for all selected items via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param Items Receives an allocated array of item parameter pointers.
 * \param NumberOfItems Receives the count of selected items.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_GetSelectedItemParams(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Out_ PVOID** Items,
    _Out_ PULONG NumberOfItems
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return PhGetSelectedIListViewItemParams(Context->ListViewInterface, Items, NumberOfItems);
    }
    else
    {
        return PhGetSelectedListViewItemParams(Context->ListViewHandle, Items, NumberOfItems);
    }
}

/**
 * Retrieves the client bounding rectangle of the list-view via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param ClientRect Receives the client rectangle.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_GetClientRect(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Inout_ PRECT ClientRect
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return PhGetIListViewClientRect(Context->ListViewInterface, ClientRect);
    }
    else
    {
        //ListView_GetViewRect(Context->ListViewHandle, ClientRect);
        return !!GetClientRect(Context->ListViewHandle, ClientRect);
    }
}

/**
 * Retrieves the bounding rectangle for an item via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param StartIndex The item index.
 * \param Flags Portion flags for the rectangle.
 * \param ItemRect Receives the bounding rectangle.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_GetItemRect(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG StartIndex,
    _In_ ULONG Flags,
    _Inout_ PRECT ItemRect
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return PhGetIListViewItemRect(Context->ListViewInterface, StartIndex, Flags, ItemRect);
    }
    else
    {
        return !!ListView_GetItemRect(Context->ListViewHandle, StartIndex, ItemRect, Flags);
    }
}
 
/**
 * Enables or disables group view mode in the list-view via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param Enable TRUE to enable group view, FALSE to disable.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_EnableGroupView(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ BOOLEAN Enable
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return HR_SUCCESS(Context->ListViewInterface->EnableGroupView(Enable));
    }
    else
    {
        return !!ListView_EnableGroupView(Context->ListViewHandle, Enable);
    }
}

/**
 * Ensures that a list-view item is visible, scrolling if necessary, via the wrapper context.
 *
 * \param Context A pointer to the list-view context.
 * \param ItemIndex The item index.
 * \param PartialOk TRUE if a partially visible item does not need scrolling.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhListView_EnsureItemVisible(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ BOOLEAN PartialOk
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        LVITEMINDEX itemIndex;
        
        itemIndex.iItem = ItemIndex;
        itemIndex.iGroup = INT_ERROR;
        
        return SUCCEEDED(Context->ListViewInterface->EnsureItemVisible(itemIndex, PartialOk));
    }
    else
    {
        return !!ListView_EnsureVisible(Context->ListViewHandle, ItemIndex, PartialOk);
    }
}

/**
 * Checks if a list-view item is currently visible in the view.
 *
 * \param Context A pointer to the list-view context.
 * \param ItemIndex The item index.
 * \param Visible Receives TRUE if the item is visible, FALSE otherwise.
 * \return BOOLEAN TRUE if successful, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhListView_IsItemVisible(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _Out_ PBOOLEAN Visible
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        LVITEMINDEX itemIndex;
        BOOL visible;
        
        itemIndex.iItem = ItemIndex;
        itemIndex.iGroup = INT_ERROR;
        
        if (SUCCEEDED(Context->ListViewInterface->IsItemVisible(itemIndex, &visible)))
        {
            *Visible = !!visible;
            return TRUE;
        }
        
        return FALSE;
    }
    else
    {
        RECT itemRect;
        RECT clientRect;
        
        if (!ListView_GetItemRect(Context->ListViewHandle, ItemIndex, &itemRect, LVIR_BOUNDS))
            return FALSE;
        
        if (!GetClientRect(Context->ListViewHandle, &clientRect))
            return FALSE;
        
        // Check if the item rectangle intersects with the client area
        *Visible = (itemRect.top < clientRect.bottom && itemRect.bottom > clientRect.top);
        return TRUE;
    }
}

/**
 * Determines which list-view item and sub-item is at a specified point.
 *
 * \param Context A pointer to the list-view context.
 * \param HitTestInfo A pointer to the LVHITTESTINFO structure containing coordinates and receiving results.
 * \return BOOLEAN TRUE if an item was hit, FALSE otherwise.
 */
BOOLEAN PhListView_HitTestSubItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Inout_ LVHITTESTINFO* HitTestInfo
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        return HR_SUCCESS(Context->ListViewInterface->HitTestSubItem(HitTestInfo));
    }
    else
    {
        return ListView_SubItemHitTest(Context->ListViewHandle, HitTestInfo) != INT_ERROR;
    }
}
