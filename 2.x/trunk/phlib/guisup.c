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

#define _PH_GUISUP_PRIVATE
#include <phgui.h>
#include <guisupp.h>
#include <windowsx.h>

_ChangeWindowMessageFilter ChangeWindowMessageFilter_I;
_RunFileDlg RunFileDlg;
_SetWindowTheme SetWindowTheme_I;
_IsThemeActive IsThemeActive_I;
_OpenThemeData OpenThemeData_I;
_CloseThemeData CloseThemeData_I;
_DrawThemeBackground DrawThemeBackground_I;
_DrawThemeText DrawThemeText_I;
_SHAutoComplete SHAutoComplete_I;
_SHCreateShellItem SHCreateShellItem_I;
_SHOpenFolderAndSelectItems SHOpenFolderAndSelectItems_I;
_SHParseDisplayName SHParseDisplayName_I;
_TaskDialogIndirect TaskDialogIndirect_I;

VOID PhGuiSupportInitialization()
{
    HMODULE shell32Handle;
    HMODULE shlwapiHandle;
    HMODULE uxthemeHandle;

    shell32Handle = LoadLibrary(L"shell32.dll");
    shlwapiHandle = LoadLibrary(L"shlwapi.dll");
    uxthemeHandle = LoadLibrary(L"uxtheme.dll");

    if (WINDOWS_HAS_UAC)
        ChangeWindowMessageFilter_I = PhGetProcAddress(L"user32.dll", "ChangeWindowMessageFilter");
    RunFileDlg = (PVOID)GetProcAddress(shell32Handle, (PSTR)61);
    SetWindowTheme_I = (PVOID)GetProcAddress(uxthemeHandle, "SetWindowTheme");
    IsThemeActive_I = (PVOID)GetProcAddress(uxthemeHandle, "IsThemeActive");
    OpenThemeData_I = (PVOID)GetProcAddress(uxthemeHandle, "OpenThemeData");
    CloseThemeData_I = (PVOID)GetProcAddress(uxthemeHandle, "CloseThemeData");
    DrawThemeBackground_I = (PVOID)GetProcAddress(uxthemeHandle, "DrawThemeBackground");
    DrawThemeText_I = (PVOID)GetProcAddress(uxthemeHandle, "DrawThemeText");
    SHAutoComplete_I = (PVOID)GetProcAddress(shlwapiHandle, "SHAutoComplete");
    SHCreateShellItem_I = (PVOID)GetProcAddress(shell32Handle, "SHCreateShellItem");
    SHOpenFolderAndSelectItems_I = (PVOID)GetProcAddress(shell32Handle, "SHOpenFolderAndSelectItems");
    SHParseDisplayName_I = (PVOID)GetProcAddress(shell32Handle, "SHParseDisplayName");
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
        PhLibImageBase,
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
    __in_opt PVOID Param
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
    __in_opt PVOID Param
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

BOOLEAN PhLoadListViewColumnSettings(
    __in HWND ListViewHandle,
    __in PPH_STRING Settings
    )
{
#define ORDER_LIMIT 50
    ULONG i;
    ULONG length;
    ULONG columnIndex;
    ULONG indexOfComma;
    ULONG indexOfPipe;
    PH_STRINGREF stringRef;
    ULONG64 integer;
    LVCOLUMN lvColumn;
    INT orderArray[ORDER_LIMIT]; // HACK, but reasonable limit
    INT maxOrder;

    if (Settings->Length == 0)
        return FALSE;

    i = 0;
    length = (ULONG)Settings->Length / 2;
    columnIndex = 0;
    lvColumn.mask = LVCF_WIDTH;

    memset(orderArray, 0, sizeof(orderArray));
    maxOrder = 0;

    while (i < length)
    {
        indexOfComma = PhFindCharInString(Settings, i, ',');

        if (indexOfComma == -1)
            return FALSE;

        indexOfPipe = PhFindCharInString(Settings, i, '|');

        if (indexOfPipe == -1) // last pair in string
            indexOfPipe = Settings->Length / 2;

        // Order

        stringRef.Buffer = &Settings->Buffer[i];
        stringRef.Length = (USHORT)((indexOfComma - i) * 2);

        if (!PhStringToInteger64(&stringRef, 10, &integer))
            return FALSE;

        if ((INT)integer >= 0 && (INT)integer < ORDER_LIMIT)
        {
            orderArray[(INT)integer] = columnIndex;

            if (maxOrder < (INT)integer + 1)
                maxOrder = (INT)integer + 1;
        }

        // Width

        stringRef.Buffer = &Settings->Buffer[indexOfComma + 1];
        stringRef.Length = (USHORT)((indexOfPipe - indexOfComma - 1) * 2);

        if (!PhStringToInteger64(&stringRef, 10, &integer))
            return FALSE;

        lvColumn.cx = (ULONG)integer;

        ListView_SetColumn(ListViewHandle, columnIndex, &lvColumn);

        i = indexOfPipe + 1;
        columnIndex++;
    }

    ListView_SetColumnOrderArray(ListViewHandle, maxOrder, orderArray);

    return TRUE;
}

PPH_STRING PhSaveListViewColumnSettings(
    __in HWND ListViewHandle
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i = 0;
    LVCOLUMN lvColumn;

    PhInitializeStringBuilder(&stringBuilder, 20);

    lvColumn.mask = LVCF_WIDTH | LVCF_ORDER;

    while (ListView_GetColumn(ListViewHandle, i, &lvColumn))
    {
        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%u,%u|",
            lvColumn.iOrder,
            lvColumn.cx
            );
        i++;
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    return PhFinalStringBuilderString(&stringBuilder);
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
        PhLibImageBase,
        NULL
        );

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
        return PhReferenceEmptyString();

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

VOID PhAddComboBoxStrings(
    __in HWND hWnd,
    __in PWSTR *Strings,
    __in ULONG NumberOfStrings
    )
{
    ULONG i;

    for (i = 0; i < NumberOfStrings; i++)
        ComboBox_AddString(hWnd, Strings[i]);
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
        return PhReferenceEmptyString();

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

PPH_STRING PhGetListBoxString(
    __in HWND hwnd,
    __in INT Index
    )
{
    PPH_STRING string;
    ULONG length;

    if (Index == -1)
    {
        Index = ListBox_GetCurSel(hwnd);

        if (Index == -1)
            return NULL;
    }

    length = ListBox_GetTextLen(hwnd, Index);

    if (length == LB_ERR)
        return NULL;
    if (length == 0)
        return PhReferenceEmptyString();

    string = PhCreateStringEx(NULL, length * 2);

    if (ListBox_GetText(hwnd, Index, string->Buffer) != LB_ERR)
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

VOID PhSetMenuItemBitmap(
    __in HMENU Menu,
    __in ULONG Item,
    __in BOOLEAN ByPosition,
    __in HBITMAP Bitmap
    )
{
    MENUITEMINFO info = { sizeof(info) };

    info.fMask = MIIM_BITMAP;
    info.hbmpItem = Bitmap;

    SetMenuItemInfo(Menu, Item, ByPosition, &info);
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

VOID PhSetStateAllListViewItems(
    __in HWND hWnd,
    __in ULONG State,
    __in ULONG Mask
    )
{
    ULONG i;
    ULONG count;

    count = ListView_GetItemCount(hWnd);

    if (count == -1)
        return;

    for (i = 0; i < count; i++)
    {
        ListView_SetItemState(hWnd, i, State, Mask);
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
            PhAddItemList(list, param);
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
        PhRemoveItemList(Wrapper->FreeList, Wrapper->FreeList->Count - 1);
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
    PhAddItemList(Wrapper->FreeList, (PVOID)Index);
}

/**
 * Gets the default icon used for executable files.
 *
 * \param SmallIcon A variable which receives the small default executable icon. 
 * Do not destroy the icon using DestroyIcon(); it is shared between callers.
 * \param LargeIcon A variable which receives the large default executable icon. 
 * Do not destroy the icon using DestroyIcon(); it is shared between callers.
 */
VOID PhGetStockApplicationIcon(
    __out_opt HICON *SmallIcon,
    __out_opt HICON *LargeIcon
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HICON smallIcon = NULL;
    static HICON largeIcon = NULL;

    // This no longer uses SHGetFileInfo because it is *very* slow and causes 
    // many other DLLs to be loaded, increasing memory usage. The worst thing 
    // about it, however, is that it is horribly incompatible with multi-threading. 
    // The first time it is called, it tries to perform some one-time initialization. 
    // It guards this with a lock, but when multiple threads try to call the function 
    // at the same time, instead of waiting for initialization to finish it simply 
    // fails the other threads.

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_STRING systemDirectory;
        PPH_STRING dllFileName;

        // user32,0 (Vista and above) or shell32,2 (XP) contains the default application icon.

        if (systemDirectory = PhGetSystemDirectory())
        {
            PH_STRINGREF dllBaseName;
            ULONG index;

            // TODO: Find a better solution.

            if (WindowsVersion >= WINDOWS_VISTA)
            {
                PhInitializeStringRef(&dllBaseName, L"\\user32.dll");
                index = 0;
            }
            else
            {
                PhInitializeStringRef(&dllBaseName, L"\\shell32.dll");
                index = 2;
            }

            dllFileName = PhConcatStringRef2(&systemDirectory->sr, &dllBaseName);
            PhDereferenceObject(systemDirectory);

            ExtractIconEx(dllFileName->Buffer, index, &largeIcon, &smallIcon, 1);
            PhDereferenceObject(dllFileName);
        }

        // Fallback icons - this is bad, because the icon isn't scaled correctly.
        if (!smallIcon)
            smallIcon = LoadIcon(NULL, IDI_APPLICATION);
        if (!largeIcon)
            largeIcon = LoadIcon(NULL, IDI_APPLICATION);

        PhEndInitOnce(&initOnce);
    }

    if (SmallIcon)
        *SmallIcon = smallIcon;
    if (LargeIcon)
        *LargeIcon = largeIcon;
}

HICON PhGetFileShellIcon(
    __in_opt PWSTR FileName,
    __in_opt PWSTR DefaultExtension,
    __in BOOLEAN LargeIcon
    )
{
    SHFILEINFO fileInfo;
    ULONG iconFlag;
    HICON icon;

    if (DefaultExtension && PhEqualStringZ(DefaultExtension, L".exe", TRUE))
    {
        // Special case for executable files (see above for reasoning).

        icon = NULL;

        if (FileName)
        {
            ExtractIconEx(
                FileName,
                0,
                LargeIcon ? &icon : NULL,
                !LargeIcon ? &icon : NULL,
                1
                );
        }

        if (!icon)
        {
            PhGetStockApplicationIcon(
                !LargeIcon ? &icon : NULL,
                LargeIcon ? &icon : NULL
                );

            if (icon)
                icon = DuplicateIcon(NULL, icon);
        }

        return icon;
    }

    iconFlag = LargeIcon ? SHGFI_LARGEICON : SHGFI_SMALLICON;
    icon = NULL;

    if (FileName && SHGetFileInfo(
        FileName,
        0,
        &fileInfo,
        sizeof(SHFILEINFO),
        SHGFI_ICON | iconFlag
        ))
    {
        icon = fileInfo.hIcon;
    }

    if (!icon && DefaultExtension)
    {
        if (SHGetFileInfo(
            DefaultExtension,
            FILE_ATTRIBUTE_NORMAL,
            &fileInfo,
            sizeof(SHFILEINFO),
            SHGFI_ICON | iconFlag | SHGFI_USEFILEATTRIBUTES
            ))
            icon = fileInfo.hIcon;
    }

    return icon;
}

VOID PhpSetClipboardData(
    __in HWND hWnd,
    __in ULONG Format,
    __in HANDLE Data
    )
{
    if (OpenClipboard(hWnd))
    {
        if (!EmptyClipboard())
            goto Fail;

        if (!SetClipboardData(Format, Data))
            goto Fail;

        CloseClipboard();

        return;
    }

Fail:
    GlobalFree(Data);
}

VOID PhSetClipboardString(
    __in HWND hWnd,
    __in PPH_STRINGREF String
    )
{
    PhSetClipboardStringEx(hWnd, String->Buffer, String->Length);
}

VOID PhSetClipboardStringEx(
    __in HWND hWnd,
    __in PWSTR Buffer,
    __in SIZE_T Length
    )
{
    HANDLE data;
    PVOID memory;

    data = GlobalAlloc(GMEM_MOVEABLE, Length + 2);
    memory = GlobalLock(data);

    memcpy(memory, Buffer, Length);
    *(PWCHAR)((PCHAR)memory + Length) = 0;

    GlobalUnlock(memory);

    PhpSetClipboardData(hWnd, CF_UNICODETEXT, data);
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
    Manager->RootItem.LayoutParentItem = NULL;
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

// HACK: The maths below is all horribly broken, especially the HACK for multiline tab 
// controls.

PPH_LAYOUT_ITEM PhAddLayoutItem(
    __inout PPH_LAYOUT_MANAGER Manager,
    __in HWND Handle,
    __in_opt PPH_LAYOUT_ITEM ParentItem,
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

    if (layoutItem->ParentItem != layoutItem->LayoutParentItem)
    {
        // Fix the margin because the item has a dummy parent. They share 
        // the same layout parent item.
        layoutItem->Margin.top -= layoutItem->ParentItem->Rect.top;
        layoutItem->Margin.left -= layoutItem->ParentItem->Rect.left;
        layoutItem->Margin.right = layoutItem->ParentItem->Margin.right;
        layoutItem->Margin.bottom = layoutItem->ParentItem->Margin.bottom;
    }

    return layoutItem;
}

PPH_LAYOUT_ITEM PhAddLayoutItemEx(
    __inout PPH_LAYOUT_MANAGER Manager,
    __in HWND Handle,
    __in_opt PPH_LAYOUT_ITEM ParentItem,
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

    item->LayoutParentItem = item->ParentItem;

    while ((item->LayoutParentItem->Anchor & PH_LAYOUT_DUMMY_MASK) &&
        item->LayoutParentItem->LayoutParentItem)
    {
        item->LayoutParentItem = item->LayoutParentItem->LayoutParentItem;
    }

    item->LayoutParentItem->NumberOfChildren++;

    GetWindowRect(Handle, &item->Rect);
    MapWindowPoints(NULL, item->LayoutParentItem->Handle, (POINT *)&item->Rect, 2);

    if (item->Anchor & PH_LAYOUT_TAB_CONTROL)
    {
        // We want to convert the tab control rectangle to the tab page display rectangle.
        TabCtrl_AdjustRect(Handle, FALSE, &item->Rect);
    }

    item->Margin = Margin;

    item->OrigRect = item->Rect;

    PhAddItemList(Manager->List, item);

    return item;
}

VOID PhpLayoutItemLayout(
    __inout PPH_LAYOUT_MANAGER Manager,
    __inout PPH_LAYOUT_ITEM Item
    )
{
    RECT rect;
    BOOLEAN hasDummyParent;

    if (Item->NumberOfChildren > 0 && !Item->DeferHandle)
        Item->DeferHandle = BeginDeferWindowPos(Item->NumberOfChildren);

    if (Item->LayoutNumber == Manager->LayoutNumber)
        return;

    // If this is the root item we must stop here.
    if (!Item->ParentItem)
        return;

    PhpLayoutItemLayout(Manager, Item->ParentItem);

    if (Item->ParentItem != Item->LayoutParentItem)
    {
        PhpLayoutItemLayout(Manager, Item->LayoutParentItem);
        hasDummyParent = TRUE;
    }
    else
    {
        hasDummyParent = FALSE;
    }

    GetWindowRect(Item->Handle, &Item->Rect);
    MapWindowPoints(NULL, Item->LayoutParentItem->Handle, (POINT *)&Item->Rect, 2);

    if (Item->Anchor & PH_LAYOUT_TAB_CONTROL)
    {
        // We want to convert the tab control rectangle to the tab page display rectangle.
        TabCtrl_AdjustRect(Item->Handle, FALSE, &Item->Rect);
    }

    if (!(Item->Anchor & PH_LAYOUT_DUMMY_MASK))
    {
        // Convert right/bottom into margins to make the calculations 
        // easier.
        rect = Item->Rect;
        PhConvertRect(&rect, &Item->LayoutParentItem->Rect);

        if (!(Item->Anchor & (PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT)))
        {
            // TODO
            PhRaiseStatus(STATUS_NOT_IMPLEMENTED);
        }
        else if (Item->Anchor & PH_ANCHOR_RIGHT)
        {
            if (Item->Anchor & PH_ANCHOR_LEFT)
            {
                rect.left = (hasDummyParent ? Item->ParentItem->Rect.left : 0) + Item->Margin.left;
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
                // tab control hack
                rect.top = (hasDummyParent ? Item->ParentItem->Rect.top : 0) + Item->Margin.top;
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
        PhConvertRect(&rect, &Item->LayoutParentItem->Rect);
        Item->Rect = rect;

        if (!(Item->Anchor & PH_LAYOUT_IMMEDIATE_RESIZE))
        {
            Item->LayoutParentItem->DeferHandle = DeferWindowPos(
                Item->LayoutParentItem->DeferHandle, Item->Handle,
                NULL, rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top,
                SWP_NOACTIVATE | SWP_NOZORDER
                );
        }
        else
        {
            // This is needed for tab controls, so that TabCtrl_AdjustRect will give us an up-to-date result.
            SetWindowPos(
                Item->Handle,
                NULL, rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top,
                SWP_NOACTIVATE | SWP_NOZORDER
                );
        }
    }

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

        if (item->Anchor & PH_LAYOUT_FORCE_INVALIDATE)
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
