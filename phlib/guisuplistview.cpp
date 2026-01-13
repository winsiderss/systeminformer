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
    column.cx = WindowsVersion < WINDOWS_10 ? Width : PhGetDpi(Width, ListViewDpi);
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
    column.cx = WindowsVersion < WINDOWS_10 ? Width : PhGetDpi(Width, ListViewDpi);
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
    itemIndex.iGroup = INT_ERROR;

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

    if (SUCCEEDED(ListView->InsertGroup(MAXUINT, &group, &index)))
        return index;

    return INT_ERROR;
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

PPH_LISTVIEW_CONTEXT PhListView_Initialize(
    _In_ HWND ListViewHandle
    )
{
    PPH_LISTVIEW_CONTEXT context;
    IListView* listviewInterface;

    context = (PPH_LISTVIEW_CONTEXT)PhAllocateZero(sizeof(PH_LISTVIEW_CONTEXT));
    context->ListViewHandle = ListViewHandle;
    context->ThreadId = UlongToHandle(GetWindowThreadProcessId(ListViewHandle, nullptr));

    if (listviewInterface = PhGetListViewInterface(ListViewHandle))
    {
        context->ListViewInterface = listviewInterface;
    }

    return context;
}

VOID PhListView_Destroy(
    _In_ PPH_LISTVIEW_CONTEXT Context
    )
{
    if (Context->ListViewInterface)
    {
        Context->ListViewInterface->Release();
        Context->ListViewInterface = NULL;
    }

    PhFree(Context);
}

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

BOOLEAN PhListView_SetItemCount(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemCount,
    _In_ ULONG Flags
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

_Use_decl_annotations_
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

        if (SendMessage(Context->ListViewHandle, LVM_GETITEMTEXTW, ItemIndex, (LPARAM)&item))
        {
            return TRUE;
        }
    }

    return FALSE;
}

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

        if (SendMessage(Context->ListViewHandle, LVM_SETITEMTEXTW, ItemIndex, (LPARAM)&item))
        {
            return TRUE;
        }
    }

    return FALSE;
}

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

_Use_decl_annotations_
BOOLEAN PhListView_InsertItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LVITEMW* Item,
    _Out_opt_ PLONG ItemIndex
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->InsertItem(Item, ItemIndex)))
            return TRUE;
    }
    else
    {
        LONG index = ListView_InsertItem(Context->ListViewHandle, Item);

        if (index != INT_ERROR)
        {
            if (ItemIndex)
            {
                *ItemIndex = index;
            }

            return TRUE;
        }
    }

    return FALSE;
}

_Use_decl_annotations_
BOOLEAN PhListView_InsertGroup(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG InsertAt,
    _In_ LVGROUP* Group,
    _Out_opt_ PLONG GroupId
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->InsertGroup(InsertAt, Group, GroupId)))
            return TRUE;
    }
    else
    {
        LONG index = (LONG)ListView_InsertGroup(Context->ListViewHandle, InsertAt, Group);

        if (index != INT_ERROR)
        {
            if (GroupId)
            {
                *GroupId = index;
            }

            return TRUE;
        }
    }

    return FALSE;
}

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

        if (HR_SUCCESS(Context->ListViewInterface->GetItemState(ItemIndex, 0, Mask, &state)))
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

BOOLEAN PhListView_SetItemState(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ ULONG State,
    _In_ ULONG Mask
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        if (HR_SUCCESS(Context->ListViewInterface->SetItemState(ItemIndex, 0, State, Mask)))
            return TRUE;
    }
    else
    {
        LVITEM item;

        ZeroMemory(&item, sizeof(LVITEM));
        item.state = State;
        item.stateMask = Mask;
        item.mask = LVIF_STATE;
        item.iItem = ItemIndex;

        if (SendMessage(Context->ListViewHandle, LVM_SETITEMSTATE, ItemIndex, (LPARAM)&item))
        {
            return TRUE;
        }
    }

    return FALSE;
}

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
            (LPARAM)CompareContext,
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
                Context->ListViewInterface->SetItemState(i, 0, Mask, State);
            }
        }
    }
    else
    {
        PhSetStateAllListViewItems(Context->ListViewHandle, State, Mask);
    }
}

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

VOID PhListView_GetSelectedItemParams(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Out_ PVOID** Items,
    _Out_ PULONG NumberOfItems
    )
{
    if (Context->ListViewInterface && NtCurrentThreadId() == Context->ThreadId)
    {
        PhGetSelectedIListViewItemParams(Context->ListViewInterface, Items, NumberOfItems);
    }
    else
    {
        PhGetSelectedListViewItemParams(Context->ListViewHandle, Items, NumberOfItems);
    }
}

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
        return !!GetClientRect(Context->ListViewHandle, ClientRect);
    }
}

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
