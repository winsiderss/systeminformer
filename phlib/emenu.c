/*
 * Process Hacker - 
 *   extended menus
 * 
 * Copyright (C) 2010 wj32
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

#include <phgui.h>
#include <emenu.h>

static PH_FLAG_MAPPING EMenuTypeMappings[] =
{
    { PH_EMENU_MENUBARBREAK, MFT_MENUBARBREAK },
    { PH_EMENU_MENUBREAK, MFT_MENUBREAK },
    { PH_EMENU_RADIOCHECK, MFT_RADIOCHECK }
};

static PH_FLAG_MAPPING EMenuStateMappings[] =
{
    { PH_EMENU_CHECKED, MFS_CHECKED },
    { PH_EMENU_DEFAULT, MFS_DEFAULT },
    { PH_EMENU_DISABLED, MFS_DISABLED },
    { PH_EMENU_HIGHLIGHT, MFS_HILITE }
};

PPH_EMENU_ITEM PhAllocateEMenuItem()
{
    PPH_EMENU_ITEM item;

    item = PhAllocate(sizeof(PH_EMENU_ITEM));
    memset(item, 0, sizeof(PH_EMENU_ITEM));

    return item;
}

PPH_EMENU_ITEM PhCreateEMenuItem(
    __in ULONG Flags,
    __in ULONG Id,
    __in PWSTR Text,
    __in_opt PWSTR Bitmap,
    __in_opt PVOID Context
    )
{
    PPH_EMENU_ITEM item;

    item = PhAllocateEMenuItem();

    item->Flags = Flags;
    item->Id = Id;
    item->Text = Text;

    if (Bitmap)
    {
        // TODO
    }

    item->Context = Context;

    return item;
}

VOID PhDestroyEMenuItem(
    __in PPH_EMENU_ITEM Item
    )
{
    if (Item->DeleteFunction)
        Item->DeleteFunction(Item);

    if (Item->Flags & PH_EMENU_TEXT_OWNED)
        PhFree(Item->Text);

    if (Item->Items)
    {
        ULONG i;

        for (i = 0; i < Item->Items->Count; i++)
        {
            PhDestroyEMenuItem(Item->Items->Items[i]);
        }

        PhDereferenceObject(Item->Items);
    }

    PhFree(Item);
}

PPH_EMENU_ITEM PhFindEMenuItem(
    __in PPH_EMENU_ITEM Item,
    __in ULONG Flags,
    __in_opt PWSTR Text,
    __in_opt ULONG Id
    )
{
    ULONG i;
    PH_STRINGREF searchText;

    if (!Item->Items)
        return NULL;

    if (Text)
        PhInitializeStringRef(&searchText, Text);

    for (i = 0; i < Item->Items->Count; i++)
    {
        PPH_EMENU_ITEM item;

        item = Item->Items->Items[i];

        if (Text)
        {
            PH_STRINGREF text;

            PhInitializeStringRef(&text, item->Text);

            if (Flags & PH_EMENU_FIND_STARTSWITH)
            {
                if (PhStartsWithStringRef(&text, &searchText, TRUE))
                    return item;
            }
            else
            {
                if (PhEqualStringRef(&text, &searchText, TRUE))
                    return item;
            }
        }

        if (Id && item->Id == Id)
            return item;

        if (Flags & PH_EMENU_FIND_DESCEND)
        {
            PPH_EMENU_ITEM foundItem;

            foundItem = PhFindEMenuItem(item, Flags, Text, Id);

            if (foundItem)
                return foundItem;
        }
    }

    return NULL;
}

ULONG PhIndexOfEMenuItem(
    __in PPH_EMENU_ITEM Parent,
    __in PPH_EMENU_ITEM Item
    )
{
    if (!Parent->Items)
        return -1;

    return PhFindItemList(Parent->Items, Item);
}

VOID PhInsertEMenuItem(
    __inout PPH_EMENU_ITEM Parent,
    __inout PPH_EMENU_ITEM Item,
    __in ULONG Index
    )
{
    // Remove the item from its old parent if it has one.
    if (Item->Parent)
        PhRemoveEMenuItem(Item->Parent, Item, 0);

    if (!Parent->Items)
        Parent->Items = PhCreateList(16);

    if (Index > Parent->Items->Count)
        Index = Parent->Items->Count;

    if (Index == -1)
        PhAddItemList(Parent->Items, Item);
    else
        PhInsertItemList(Parent->Items, Index, Item);

    Item->Parent = Parent;
}

BOOLEAN PhRemoveEMenuItem(
    __inout_opt PPH_EMENU_ITEM Parent,
    __in_opt PPH_EMENU_ITEM Item,
    __in_opt ULONG Index
    )
{
    if (Item)
    {
        if (!Parent)
            Parent = Item->Parent;
        if (!Parent->Items)
            return FALSE;

        Index = PhFindItemList(Parent->Items, Item);

        if (Index == -1)
            return FALSE;
    }
    else
    {
        if (!Parent)
            return FALSE;
        if (!Parent->Items)
            return FALSE;
    }

    PhRemoveItemList(Parent->Items, Index);

    return TRUE;
}

VOID PhRemoveAllEMenuItems(
    __inout PPH_EMENU_ITEM Parent
    )
{
    ULONG i;

    if (!Parent->Items)
        return;

    for (i = 0; i < Parent->Items->Count; i++)
    {
        PhDestroyEMenuItem(Parent->Items->Items[i]);
    }

    PhClearList(Parent->Items);
}

PPH_EMENU PhCreateEMenu()
{
    PPH_EMENU menu;

    menu = PhAllocate(sizeof(PH_EMENU));
    memset(menu, 0, sizeof(PH_EMENU));
    menu->Items = PhCreateList(16);

    return menu;
}

VOID PhDestroyEMenu(
    __in PPH_EMENU Menu
    )
{
    ULONG i;

    for (i = 0; i < Menu->Items->Count; i++)
    {
        PhDestroyEMenuItem(Menu->Items->Items[i]);
    }

    PhDereferenceObject(Menu->Items);
    PhFree(Menu);
}

VOID PhInitializeEMenuData(
    __out PPH_EMENU_DATA Data
    )
{
    Data->IdToItem = PhCreateList(16);
}

VOID PhDeleteEMenuData(
    __inout PPH_EMENU_DATA Data
    )
{
    PhDereferenceObject(Data->IdToItem);
}

HMENU PhEMenuToPopupMenu(
    __in PPH_EMENU_ITEM Menu,
    __in ULONG Flags,
    __inout_opt PPH_EMENU_DATA Data
    )
{
    HMENU popupMenu;
    ULONG i;
    PPH_EMENU_ITEM item;
    MENUITEMINFO menuItemInfo;

    popupMenu = CreatePopupMenu();

    for (i = 0; i < Menu->Items->Count; i++)
    {
        item = Menu->Items->Items[i];

        memset(&menuItemInfo, 0, sizeof(MENUITEMINFO));
        menuItemInfo.cbSize = sizeof(MENUITEMINFO);

        // Type

        menuItemInfo.fMask = MIIM_FTYPE | MIIM_STATE;

        if (item->Flags & PH_EMENU_SEPARATOR)
        {
            menuItemInfo.fType = MFT_SEPARATOR;
        }
        else
        {
            menuItemInfo.fType = MFT_STRING;
            menuItemInfo.fMask |= MIIM_STRING;
            menuItemInfo.dwTypeData = item->Text;
        }

        PhMapFlags1(
            &menuItemInfo.fType,
            item->Flags,
            EMenuTypeMappings,
            sizeof(EMenuTypeMappings) / sizeof(PH_FLAG_MAPPING)
            );

        // Bitmap

        if (item->Bitmap)
        {
            menuItemInfo.fMask |= MIIM_BITMAP;
            menuItemInfo.hbmpItem = item->Bitmap;
        }

        // Id

        if (Flags & PH_EMENU_CONVERT_ID)
        {
            ULONG id;

            if (Data)
            {
                PhAddItemList(Data->IdToItem, item);
                id = Data->IdToItem->Count;

                menuItemInfo.fMask |= MIIM_ID;
                menuItemInfo.wID = id;
            }
        }
        else
        {
            if (item->Id)
            {
                menuItemInfo.fMask |= MIIM_ID;
                menuItemInfo.wID = item->Id;
            }
        }

        // State

        PhMapFlags1(
            &menuItemInfo.fState,
            item->Flags,
            EMenuStateMappings,
            sizeof(EMenuStateMappings) / sizeof(PH_FLAG_MAPPING)
            );

        // Context

        menuItemInfo.fMask |= MIIM_DATA;
        menuItemInfo.dwItemData = (ULONG_PTR)item;

        // Submenu

        if (item->Items && item->Items->Count != 0)
        {
            menuItemInfo.fMask |= MIIM_SUBMENU;
            menuItemInfo.hSubMenu = PhEMenuToPopupMenu(item, Flags, Data);
        }

        InsertMenuItem(popupMenu, MAXINT, TRUE, &menuItemInfo);
    }

    return popupMenu;
}

VOID PhPopupMenuToEMenuItem(
    __inout PPH_EMENU_ITEM MenuItem,
    __in HMENU PopupMenu
    )
{
    ULONG i;
    ULONG count;

    count = GetMenuItemCount(PopupMenu);

    if (count != -1)
    {
        for (i = 0; i < count; i++)
        {
            MENUITEMINFO menuItemInfo;
            WCHAR buffer[256];
            PPH_EMENU_ITEM menuItem;

            menuItemInfo.cbSize = sizeof(menuItemInfo);
            menuItemInfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING | MIIM_SUBMENU;
            menuItemInfo.cch = sizeof(buffer) / sizeof(WCHAR);
            menuItemInfo.dwTypeData = buffer;

            if (!GetMenuItemInfo(PopupMenu, i, TRUE, &menuItemInfo))
                continue;

            menuItem = PhCreateEMenuItem(
                PH_EMENU_TEXT_OWNED,
                menuItemInfo.wID,
                PhDuplicateUnicodeStringZ(buffer),
                NULL,
                NULL
                );

            if (menuItemInfo.fType & MFT_SEPARATOR)
                menuItem->Flags |= PH_EMENU_SEPARATOR;

            PhMapFlags2(
                &menuItem->Flags,
                menuItemInfo.fType,
                EMenuTypeMappings,
                sizeof(EMenuTypeMappings) / sizeof(PH_FLAG_MAPPING)
                );
            PhMapFlags2(
                &menuItem->Flags,
                menuItemInfo.fState,
                EMenuStateMappings,
                sizeof(EMenuStateMappings) / sizeof(PH_FLAG_MAPPING)
                );

            if (menuItemInfo.hSubMenu)
                PhPopupMenuToEMenuItem(menuItem, menuItemInfo.hSubMenu);

            PhInsertEMenuItem(MenuItem, menuItem, -1);
        }
    }
}

VOID PhLoadResourceEMenuItem(
    __inout PPH_EMENU_ITEM MenuItem,
    __in HINSTANCE InstanceHandle,
    __in PWSTR Resource,
    __in ULONG SubMenuIndex
    )
{
    HMENU menu;
    HMENU realMenu;

    menu = LoadMenu(InstanceHandle, Resource);

    if (SubMenuIndex != -1)
        realMenu = GetSubMenu(menu, SubMenuIndex);
    else
        realMenu = menu;

    PhPopupMenuToEMenuItem(MenuItem, realMenu);

    DestroyMenu(menu);
}

PPH_EMENU_ITEM PhShowEMenu(
    __in PPH_EMENU Menu,
    __in HWND WindowHandle,
    __in ULONG Flags,
    __in ULONG Align,
    __in ULONG X,
    __in ULONG Y
    )
{
    PPH_EMENU_ITEM selectedItem;
    ULONG result;
    ULONG flags;
    PH_EMENU_DATA data;
    HMENU popupMenu;

    selectedItem = NULL;
    flags = TPM_RETURNCMD;

    // Flags

    if (Flags & PH_EMENU_SHOW_NONOTIFY)
        flags |= TPM_NONOTIFY;

    if (Flags & PH_EMENU_SHOW_LEFTRIGHT)
        flags |= TPM_RIGHTBUTTON;
    else
        flags |= TPM_LEFTBUTTON;

    // Align

    if (Align & PH_ALIGN_LEFT)
        flags |= TPM_LEFTALIGN;
    else if (Align & PH_ALIGN_RIGHT)
        flags |= TPM_RIGHTALIGN;
    else
        flags |= TPM_CENTERALIGN;

    if (Align & PH_ALIGN_TOP)
        flags |= TPM_TOPALIGN;
    else if (Align & PH_ALIGN_BOTTOM)
        flags |= TPM_BOTTOMALIGN;
    else
        flags |= TPM_VCENTERALIGN;

    PhInitializeEMenuData(&data);

    if (popupMenu = PhEMenuToPopupMenu(Menu, PH_EMENU_CONVERT_ID, &data))
    {
        result = TrackPopupMenu(
            popupMenu,
            flags,
            X,
            Y,
            0,
            WindowHandle,
            NULL
            );

        if (result != 0)
        {
            selectedItem = data.IdToItem->Items[result - 1];
        }

        DestroyMenu(popupMenu);
    }

    PhDeleteEMenuData(&data);

    return selectedItem;
}

BOOLEAN PhSetFlagsEMenuItem(
    __in PPH_EMENU_ITEM Item,
    __in ULONG Id,
    __in ULONG Mask,
    __in ULONG Value
    )
{
    PPH_EMENU_ITEM item;

    item = PhFindEMenuItem(Item, PH_EMENU_FIND_DESCEND, NULL, Id);

    if (item)
    {
        item->Flags &= ~Mask;
        item->Flags |= Value;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID PhSetFlagsAllEMenuItems(
    __in PPH_EMENU_ITEM Item,
    __in ULONG Mask,
    __in ULONG Value
    )
{
    ULONG i;

    for (i = 0; i < Item->Items->Count; i++)
    {
        PPH_EMENU_ITEM item = Item->Items->Items[i];

        item->Flags &= ~Mask;
        item->Flags |= Value;
    }
}
