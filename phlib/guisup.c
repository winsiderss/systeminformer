/*
 * Process Hacker - 
 *   GUI support functions
 * 
 * Copyright (C) 2009-2010 wj32
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

#define GUISUP_PRIVATE
#include <phgui.h>
#include <windowsx.h>

_ChangeWindowMessageFilter ChangeWindowMessageFilter_I;
_RunFileDlg RunFileDlg;
_SetWindowTheme SetWindowTheme_I;
_IsThemeActive IsThemeActive_I;
_OpenThemeData OpenThemeData_I;
_CloseThemeData CloseThemeData_I;
_DrawThemeBackground DrawThemeBackground_I;
_SHAutoComplete SHAutoComplete_I;
_SHOpenFolderAndSelectItems SHOpenFolderAndSelectItems_I;
_SHParseDisplayName SHParseDisplayName_I;
_StrCmpLogicalW StrCmpLogicalW_I;
_TaskDialogIndirect TaskDialogIndirect_I;

VOID PhGuiSupportInitialization()
{
    LoadLibrary(L"shell32.dll");
    LoadLibrary(L"shlwapi.dll");
    LoadLibrary(L"uxtheme.dll");

    if (WINDOWS_HAS_UAC)
        ChangeWindowMessageFilter_I = PhGetProcAddress(L"user32.dll", "ChangeWindowMessageFilter");
    RunFileDlg = PhGetProcAddress(L"shell32.dll", (PSTR)61);
    SetWindowTheme_I = PhGetProcAddress(L"uxtheme.dll", "SetWindowTheme");
    IsThemeActive_I = PhGetProcAddress(L"uxtheme.dll", "IsThemeActive");
    OpenThemeData_I = PhGetProcAddress(L"uxtheme.dll", "OpenThemeData");
    CloseThemeData_I = PhGetProcAddress(L"uxtheme.dll", "CloseThemeData");
    DrawThemeBackground_I = PhGetProcAddress(L"uxtheme.dll", "DrawThemeBackground");
    SHAutoComplete_I = PhGetProcAddress(L"shlwapi.dll", "SHAutoComplete");
    SHOpenFolderAndSelectItems_I = PhGetProcAddress(L"shell32.dll", "SHOpenFolderAndSelectItems");
    SHParseDisplayName_I = PhGetProcAddress(L"shell32.dll", "SHParseDisplayName");
    StrCmpLogicalW_I = PhGetProcAddress(L"shlwapi.dll", "StrCmpLogicalW");
    TaskDialogIndirect_I = PhGetProcAddress(L"comctl32.dll", "TaskDialogIndirect");
}

VOID PhSetControlTheme(
    __in HWND Handle,
    __in PWSTR Theme
    )
{
    if (WindowsVersion >= WINDOWS_VISTA)
    {
        if (SetWindowTheme_I)
            SetWindowTheme_I(Handle, Theme, NULL);
    }
}

HWND PhCreateListViewControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    )
{
    return CreateWindow(
        WC_LISTVIEW,
        L"",
        WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS,
        0,
        0,
        3,
        3,
        ParentHandle,
        (HMENU)Id,
        PhInstanceHandle,
        NULL
        );
}

INT PhAddListViewColumn(
    __in HWND ListViewHandle,
    __in INT Index,
    __in INT DisplayIndex,
    __in INT SubItemIndex,
    __in INT Format,
    __in INT Width,
    __in PWSTR Text
    )
{
    LVCOLUMN column;

    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
    column.fmt = Format;
    column.cx = Width;
    column.pszText = Text;
    column.iSubItem = SubItemIndex;
    column.iOrder = DisplayIndex;

    return ListView_InsertColumn(ListViewHandle, Index, &column);
}

INT PhAddListViewItem(
    __in HWND ListViewHandle,
    __in INT Index,
    __in PWSTR Text,
    __in PVOID Param
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = Index;
    item.iSubItem = 0;
    item.pszText = Text;
    item.lParam = (LPARAM)Param;

    return ListView_InsertItem(ListViewHandle, &item);
}

INT PhFindListViewItemByFlags(
    __in HWND ListViewHandle,
    __in INT StartIndex,
    __in ULONG Flags
    )
{
    return ListView_GetNextItem(ListViewHandle, StartIndex, Flags);
}

INT PhFindListViewItemByParam(
    __in HWND ListViewHandle,
    __in INT StartIndex,
    __in PVOID Param
    )
{
    LVFINDINFO findInfo;

    findInfo.flags = LVFI_PARAM;
    findInfo.lParam = (LPARAM)Param;

    return ListView_FindItem(ListViewHandle, StartIndex, &findInfo);
}

LOGICAL PhGetListViewItemImageIndex(
    __in HWND ListViewHandle,
    __in INT Index,
    __out PINT ImageIndex
    )
{
    LOGICAL result;
    LVITEM item;

    item.mask = LVIF_IMAGE;
    item.iItem = Index;
    item.iSubItem = 0;

    result = ListView_GetItem(ListViewHandle, &item);

    if (!result)
        return result;

    *ImageIndex = item.iImage;

    return result;
}

LOGICAL PhGetListViewItemParam(
    __in HWND ListViewHandle,
    __in INT Index,
    __out PPVOID Param
    )
{
    LOGICAL result;
    LVITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = Index;
    item.iSubItem = 0;

    result = ListView_GetItem(ListViewHandle, &item);

    if (!result)
        return result;

    *Param = (PVOID)item.lParam;

    return result;
}

VOID PhRemoveListViewItem(
    __in HWND ListViewHandle,
    __in INT Index
    )
{
    ListView_DeleteItem(ListViewHandle, Index);
}

VOID PhSetListViewItemImageIndex(
    __in HWND ListViewHandle,
    __in INT Index,
    __in INT ImageIndex
    )
{
    LVITEM item;

    item.mask = LVIF_IMAGE;
    item.iItem = Index;
    item.iSubItem = 0;
    item.iImage = ImageIndex;

    ListView_SetItem(ListViewHandle, &item);
}

VOID PhSetListViewItemStateImage(
    __in HWND ListViewHandle,
    __in INT Index,
    __in INT StateImage
    )
{
    LVITEM item;

    item.mask = LVIF_STATE;
    item.iItem = Index;
    item.iSubItem = 0;
    item.state = INDEXTOSTATEIMAGEMASK(StateImage);
    item.stateMask = LVIS_STATEIMAGEMASK;

    ListView_SetItem(ListViewHandle, &item);
}

VOID PhSetListViewSubItem(
    __in HWND ListViewHandle,
    __in INT Index,
    __in INT SubItemIndex,
    __in PWSTR Text
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT;
    item.iItem = Index;
    item.iSubItem = SubItemIndex;
    item.pszText = Text;

    ListView_SetItem(ListViewHandle, &item);
}

HWND PhCreateTabControl(
    __in HWND ParentHandle
    )
{
    HWND tabControlHandle;

    tabControlHandle = CreateWindow(
        WC_TABCONTROL,
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        0,
        0,
        3,
        3,
        ParentHandle,
        NULL,
        PhInstanceHandle,
        NULL
        );

    if (tabControlHandle)
    {
        SendMessage(tabControlHandle, WM_SETFONT, (WPARAM)PhApplicationFont, FALSE);
    }

    return tabControlHandle;
}

INT PhAddTabControlTab(
    __in HWND TabControlHandle,
    __in INT Index,
    __in PWSTR Text
    )
{
    TCITEM item;

    item.mask = TCIF_TEXT;
    item.pszText = Text;

    return TabCtrl_InsertItem(TabControlHandle, Index, &item);
}

PPH_STRING PhGetWindowText(
    __in HWND hwnd
    )
{
    PPH_STRING string; 
    ULONG length;

    length = GetWindowTextLength(hwnd);

    if (length == 0)
        return PhCreateString(L"");

    string = PhCreateStringEx(NULL, length * 2);

    if (GetWindowText(hwnd, string->Buffer, string->Length / 2 + 1))
    {
        return string;
    }
    else
    {
        PhDereferenceObject(string);
        return NULL;
    }
}

PPH_STRING PhGetComboBoxString(
    __in HWND hwnd,
    __in INT Index
    )
{
    PPH_STRING string;
    ULONG length;

    if (Index == -1)
    {
        Index = ComboBox_GetCurSel(hwnd);

        if (Index == -1)
            return NULL;
    }

    length = ComboBox_GetLBTextLen(hwnd, Index);

    if (length == CB_ERR)
        return NULL;
    if (length == 0)
        return PhCreateString(L"");

    string = PhCreateStringEx(NULL, length * 2);

    if (ComboBox_GetLBText(hwnd, Index, string->Buffer) != CB_ERR)
    {
        return string;
    }
    else
    {
        PhDereferenceObject(string);
        return NULL;
    }
}

VOID PhShowContextMenu(
    __in HWND hwnd,
    __in HWND subHwnd,
    __in HMENU menu,
    __in POINT point
    )
{
    ClientToScreen(subHwnd, &point);

    TrackPopupMenu(
        menu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
        point.x,
        point.y,
        0,
        hwnd,
        NULL
        );
}

UINT PhShowContextMenu2(
    __in HWND hwnd,
    __in HWND subHwnd,
    __in HMENU menu,
    __in POINT point
    )
{
    ClientToScreen(subHwnd, &point);

    return (UINT)TrackPopupMenu(
        menu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
        point.x,
        point.y,
        0,
        hwnd,
        NULL
        );
}

VOID PhSetRadioCheckMenuItem(
    __in HMENU Menu,
    __in ULONG Id,
    __in BOOLEAN RadioCheck
    )
{
    MENUITEMINFO info = { sizeof(info) };

    info.fMask = MIIM_FTYPE;
    GetMenuItemInfo(Menu, Id, FALSE, &info);

    if (RadioCheck)
        info.fType |= MFT_RADIOCHECK;
    else
        info.fType &= ~MFT_RADIOCHECK;

    SetMenuItemInfo(Menu, Id, FALSE, &info);
}

VOID PhEnableMenuItem(
    __in HMENU Menu,
    __in ULONG Id,
    __in BOOLEAN Enable
    )
{
    EnableMenuItem(Menu, Id, Enable ? MF_ENABLED : (MF_DISABLED | MF_GRAYED));
}

VOID PhEnableAllMenuItems(
    __in HMENU Menu,
    __in BOOLEAN Enable
    )
{
    ULONG i;
    ULONG count = GetMenuItemCount(Menu);

    if (count == -1)
        return;

    if (Enable)
    {
        for (i = 0; i < count; i++)
        {
            EnableMenuItem(Menu, i, MF_ENABLED | MF_BYPOSITION);
        }
    }
    else
    {
        for (i = 0; i < count; i++)
        {
            EnableMenuItem(Menu, i, MF_DISABLED | MF_GRAYED | MF_BYPOSITION);
        }
    }
}

PVOID PhGetSelectedListViewItemParam(
    __in HWND hWnd
    )
{
    INT index;
    PVOID param;

    index = PhFindListViewItemByFlags(
        hWnd,
        -1,
        LVNI_SELECTED
        );

    if (index != -1)
    {
        if (PhGetListViewItemParam(
            hWnd,
            index,
            &param
            ))
        {
            return param;
        }
    }

    return NULL;
}

VOID PhGetSelectedListViewItemParams(
    __in HWND hWnd,
    __out PVOID **Items,
    __out PULONG NumberOfItems
    )
{
    PPH_LIST list;
    ULONG index;
    PVOID param;

    list = PhCreateList(2);
    index = -1;

    while ((index = PhFindListViewItemByFlags(
        hWnd,
        index,
        LVNI_SELECTED
        )) != -1)
    {
        if (PhGetListViewItemParam(
            hWnd,
            index,
            &param
            ))
        {
            PhAddListItem(list, param);
        }
    }

    *Items = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfItems = list->Count;

    PhDereferenceObject(list);
}

VOID PhSetImageListBitmap(
    __in HIMAGELIST ImageList,
    __in INT Index,
    __in HINSTANCE InstanceHandle,
    __in LPCWSTR BitmapName
    )
{
    HBITMAP bitmap;

    bitmap = LoadImage(InstanceHandle, BitmapName, IMAGE_BITMAP, 0, 0, 0);

    if (bitmap)
    {
        ImageList_Replace(ImageList, Index, bitmap, NULL);
        DeleteObject(bitmap);
    }
}

VOID PhInitializeImageListWrapper(
    __out PPH_IMAGE_LIST_WRAPPER Wrapper,
    __in ULONG Width,
    __in ULONG Height,
    __in ULONG Flags
    )
{
    Wrapper->Handle = ImageList_Create(Width, Height, Flags, 0, 10);
    Wrapper->FreeList = PhCreateList(10);
}

VOID PhDeleteImageListWrapper(
    __inout PPH_IMAGE_LIST_WRAPPER Wrapper
    )
{
    ImageList_Destroy(Wrapper->Handle);
    PhDereferenceObject(Wrapper->FreeList);
}

INT PhImageListWrapperAddIcon(
    __in PPH_IMAGE_LIST_WRAPPER Wrapper,
    __in HICON Icon
    )
{
    INT index;

    if (Wrapper->FreeList->Count != 0)
    {
        index = (INT)Wrapper->FreeList->Items[Wrapper->FreeList->Count - 1];
        PhRemoveListItem(Wrapper->FreeList, Wrapper->FreeList->Count - 1);
    }
    else
    {
        index = -1;
    }

    return ImageList_ReplaceIcon(Wrapper->Handle, index, Icon);
}

VOID PhImageListWrapperRemove(
    __in PPH_IMAGE_LIST_WRAPPER Wrapper,
    __in INT Index
    )
{
    // We don't actually remove the icon; this is to keep the indicies 
    // stable.
    PhAddListItem(Wrapper->FreeList, (PVOID)Index);
}

VOID PhInitializeLayoutManager(
    __out PPH_LAYOUT_MANAGER Manager,
    __in HWND RootWindowHandle
    )
{
    Manager->List = PhCreateList(4);
    Manager->LayoutNumber = 0;

    Manager->RootItem.Handle = RootWindowHandle;
    GetClientRect(Manager->RootItem.Handle, &Manager->RootItem.Rect);
    Manager->RootItem.OrigRect = Manager->RootItem.Rect;
    Manager->RootItem.ParentItem = NULL;
    Manager->RootItem.LayoutNumber = 0;
    Manager->RootItem.NumberOfChildren = 0;
    Manager->RootItem.DeferHandle = NULL;
}

VOID PhDeleteLayoutManager(
    __inout PPH_LAYOUT_MANAGER Manager
    )
{
    ULONG i;

    for (i = 0; i < Manager->List->Count; i++)
        PhFree(Manager->List->Items[i]);

    PhDereferenceObject(Manager->List);
}

PPH_LAYOUT_ITEM PhAddLayoutItem(
    __inout PPH_LAYOUT_MANAGER Manager,
    __in HWND Handle,
    __in PPH_LAYOUT_ITEM ParentItem,
    __in ULONG Anchor
    )
{
    PPH_LAYOUT_ITEM layoutItem;
    RECT dummy = { 0 };

    layoutItem = PhAddLayoutItemEx(
        Manager,
        Handle,
        ParentItem,
        Anchor,
        dummy
        );

    layoutItem->Margin = layoutItem->Rect;
    PhConvertRect(&layoutItem->Margin, &layoutItem->ParentItem->Rect);

    return layoutItem;
}

PPH_LAYOUT_ITEM PhAddLayoutItemEx(
    __inout PPH_LAYOUT_MANAGER Manager,
    __in HWND Handle,
    __in PPH_LAYOUT_ITEM ParentItem,
    __in ULONG Anchor,
    __in RECT Margin
    )
{
    PPH_LAYOUT_ITEM item;

    if (!ParentItem)
        ParentItem = &Manager->RootItem;

    item = PhAllocate(sizeof(PH_LAYOUT_ITEM));
    item->Handle = Handle;
    item->ParentItem = ParentItem;
    item->LayoutNumber = Manager->LayoutNumber;
    item->NumberOfChildren = 0;
    item->DeferHandle = NULL;
    item->Anchor = Anchor;

    item->ParentItem->NumberOfChildren++;

    GetWindowRect(Handle, &item->Rect);
    MapWindowPoints(NULL, item->ParentItem->Handle, (POINT *)&item->Rect, 2);

    item->Margin = Margin;

    item->OrigRect = item->Rect;

    PhAddListItem(Manager->List, item);

    return item;
}

VOID PhpLayoutItemLayout(
    __inout PPH_LAYOUT_MANAGER Manager,
    __inout PPH_LAYOUT_ITEM Item
    )
{
    RECT rect;

    if (Item->NumberOfChildren > 0 && !Item->DeferHandle)
        Item->DeferHandle = BeginDeferWindowPos(Item->NumberOfChildren);

    if (Item->LayoutNumber == Manager->LayoutNumber)
        return;

    // If this is the root item we must stop here.
    if (!Item->ParentItem)
        return;

    PhpLayoutItemLayout(Manager, Item->ParentItem);

    GetWindowRect(Item->Handle, &Item->Rect);
    MapWindowPoints(NULL, Item->ParentItem->Handle, (POINT *)&Item->Rect, 2);

    // Convert right/bottom into margins to make the calculations 
    // easier.
    rect = Item->Rect;
    PhConvertRect(&rect, &Item->ParentItem->Rect);

    if (!(Item->Anchor & (PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT)))
    {
        // TODO
        PhRaiseStatus(STATUS_NOT_IMPLEMENTED);
    }
    else if (Item->Anchor & PH_ANCHOR_RIGHT)
    {
        if (Item->Anchor & PH_ANCHOR_LEFT)
        {
            rect.left = Item->Margin.left;
            rect.right = Item->Margin.right;
        }
        else
        {
            ULONG diff = Item->Margin.right - rect.right;

            rect.left -= diff;
            rect.right += diff;
        }
    }

    if (!(Item->Anchor & (PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM)))
    {
        // TODO
        PhRaiseStatus(STATUS_NOT_IMPLEMENTED);
    }
    else if (Item->Anchor & PH_ANCHOR_BOTTOM)
    {
        if (Item->Anchor & PH_ANCHOR_TOP)
        {
            rect.top = Item->Margin.top;
            rect.bottom = Item->Margin.bottom;
        }
        else
        {
            ULONG diff = Item->Margin.bottom - rect.bottom;

            rect.top -= diff;
            rect.bottom += diff;
        }
    }

    // Convert the right/bottom back into co-ordinates.
    PhConvertRect(&rect, &Item->ParentItem->Rect);
    Item->Rect = rect;

    Item->ParentItem->DeferHandle = DeferWindowPos(
        Item->ParentItem->DeferHandle, Item->Handle,
        NULL, rect.left, rect.top,
        rect.right - rect.left, rect.bottom - rect.top,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    Item->LayoutNumber = Manager->LayoutNumber;
}

VOID PhLayoutManagerLayout(
    __inout PPH_LAYOUT_MANAGER Manager
    )
{
    ULONG i;

    Manager->LayoutNumber++;

    GetClientRect(Manager->RootItem.Handle, &Manager->RootItem.Rect);

    for (i = 0; i < Manager->List->Count; i++)
    {
        PPH_LAYOUT_ITEM item = (PPH_LAYOUT_ITEM)Manager->List->Items[i];

        PhpLayoutItemLayout(Manager, item);
    }

    for (i = 0; i < Manager->List->Count; i++)
    {
        PPH_LAYOUT_ITEM item = (PPH_LAYOUT_ITEM)Manager->List->Items[i];

        if (item->DeferHandle)
        {
            EndDeferWindowPos(item->DeferHandle);
            item->DeferHandle = NULL;
        }

        if (item->Anchor & PH_FORCE_INVALIDATE)
        {
            InvalidateRect(item->Handle, NULL, FALSE);
        }
    }

    if (Manager->RootItem.DeferHandle)
    {
        EndDeferWindowPos(Manager->RootItem.DeferHandle);
        Manager->RootItem.DeferHandle = NULL;
    }
}
