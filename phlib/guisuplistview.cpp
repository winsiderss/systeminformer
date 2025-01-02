/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <guisup.h>
#include <guisupview.h>

#include <commoncontrols.h>
#include <wincodec.h>
#include <uxtheme.h>

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
    column.cx = Width < 0 ? -Width : PhGetDpi(Width, ListViewDpi);
    column.pszText = const_cast<PWSTR>(Text);
    column.iSubItem = SubItemIndex;
    column.iOrder = DisplayIndex;

    return ListView_InsertColumn(ListViewHandle, Index, &column);
}

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
    column.cx = Width < 0 ? -Width : PhGetDpi(Width, ListViewDpi);
    column.pszText = const_cast<PWSTR>(Text);
    column.iSubItem = SubItemIndex;
    column.iOrder = DisplayIndex;

    if (SUCCEEDED(ListView->InsertColumn(Index, &column, &index)))
        return index;

    return INT_ERROR;
}

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
        return INT_MAX;

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

LONG PhFindListViewItemByFlags(
    _In_ HWND ListViewHandle,
    _In_ LONG StartIndex,
    _In_ ULONG Flags
    )
{
    return ListView_GetNextItem(ListViewHandle, StartIndex, Flags);
}

LONG PhFindIListViewItemByFlags(
    _In_ IListView* ListView,
    _In_ LONG StartIndex,
    _In_ ULONG Flags
    )
{
    LVITEMINDEX itemIndex;
    LVITEMINDEX nextItemIndex;

    itemIndex.iItem = StartIndex;
    itemIndex.iGroup = -1;

    if (SUCCEEDED(ListView->GetNextItem(itemIndex, Flags, &nextItemIndex)))
        return nextItemIndex.iItem;

    return INT_ERROR;
}

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

LONG PhFindIListViewItemByParam(
    _In_ IListView* ListView,
    _In_ LONG StartIndex,
    _In_opt_ PVOID Param
    )
{
    LVITEMINDEX itemIndex;
    LVITEMINDEX nextItemIndex;
    LVFINDINFO findInfo;

    itemIndex.iItem = StartIndex;
    itemIndex.iGroup = -1;

    findInfo.flags = LVFI_PARAM;
    findInfo.lParam = reinterpret_cast<LPARAM>(Param);

    if (SUCCEEDED(ListView->FindItem(itemIndex, &findInfo, &nextItemIndex)))
        return nextItemIndex.iItem;

    return INT_ERROR;
}

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

VOID PhRemoveListViewItem(
    _In_ HWND ListViewHandle,
    _In_ LONG Index
    )
{
    ListView_DeleteItem(ListViewHandle, Index);
}

VOID PhRemoveIListViewItem(
    _In_ IListView* ListView,
    _In_ LONG Index
    )
{
    ListView->DeleteItem(Index);
}

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

VOID PhSetIListViewGroupSubItem(
    _In_ IListView* ListView,
    _In_ LONG GroupId,
    _In_ LONG Index,
    _In_ LONG SubItemIndex,
    _In_ PCWSTR Text
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT | LVIF_GROUPID;
    item.iItem = Index;
    item.iSubItem = SubItemIndex;
    item.iGroupId = GroupId;
    item.pszText = const_cast<PWSTR>(Text);

    ListView->SetItem(&item);
}

VOID PhRedrawListViewItems(
    _In_ HWND ListViewHandle
    )
{
    ListView_RedrawItems(ListViewHandle, 0, INT_MAX);
    // Note: UpdateWindow() is a workaround for ListView_RedrawItems() failing to send LVN_GETDISPINFO
    // and fixes RedrawItems() graphical artifacts when the listview doesn't have foreground focus. (dmex)
    UpdateWindow(ListViewHandle);
}

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

    ListView->InsertGroup(MAXUINT, &group, &index);
    return index;
}

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

LONG PhAddIListViewGroupItem(
    _In_ IListView* ListView,
    _In_ LONG GroupId,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    )
{
    LVITEM item;
    LONG index = 0;

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

    ListView->InsertItem(&item, &index);
    return index;
}

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

VOID PhSetStateAllIListViewItems(
    _In_ IListView* ListView,
    _In_ ULONG State,
    _In_ ULONG Mask
    )
{
    LONG i;
    LONG count;

    if (ListView->GetItemCount(&count) != S_OK)
        return;

    if (count <= 0)
        return;

    for (i = 0; i < count; i++)
    {
        ListView->SetItemState(i, 0, Mask, State);
    }
}

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

PVOID PhGetSelectedIListViewItemParam(
    _In_ IListView* ListView
    )
{
    LONG index;
    PVOID param;

    index = PhFindIListViewItemByFlags(
        ListView,
        INT_ERROR,
        LVNI_SELECTED
        );

    if (index != INT_ERROR)
    {
        if (PhGetIListViewItemParam(ListView, index, &param))
            return param;
    }

    return nullptr;
}

VOID PhGetSelectedListViewItemParams(
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
    *Items = static_cast<PVOID*>(PhFinalArrayItems(&array));
}

VOID PhGetSelectedIListViewItemParams(
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
    *Items = static_cast<PVOID*>(PhFinalArrayItems(&array));
}

BOOLEAN PhGetIListViewClientRect(
    _In_ IListView* ListView,
    _Inout_ PRECT ClientRect
    )
{
    return SUCCEEDED(ListView->GetClientRect(FALSE, ClientRect));
}

BOOLEAN PhGetIListViewItemRect(
    _In_ IListView* ListView,
    _In_ LONG StartIndex,
    _In_ ULONG Flags, // LVIR_SELECTBOUNDS | LVIR_BOUNDS
    _Inout_ PRECT ItemRect
    )
{
    LVITEMINDEX itemIndex;

    itemIndex.iItem = StartIndex;
    itemIndex.iGroup = -1;

    return SUCCEEDED(ListView->GetItemRect(itemIndex, Flags, ItemRect));
}
