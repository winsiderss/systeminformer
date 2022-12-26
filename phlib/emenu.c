/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2019
 *
 */

#include <ph.h>
#include <emenu.h>
#include <settings.h>
#include <guisup.h>

static const PH_FLAG_MAPPING EMenuTypeMappings[] =
{
    { PH_EMENU_MENUBARBREAK, MFT_MENUBARBREAK },
    { PH_EMENU_MENUBREAK, MFT_MENUBREAK },
    { PH_EMENU_RADIOCHECK, MFT_RADIOCHECK }
};

static const PH_FLAG_MAPPING EMenuStateMappings[] =
{
    { PH_EMENU_CHECKED, MFS_CHECKED },
    { PH_EMENU_DEFAULT, MFS_DEFAULT },
    { PH_EMENU_DISABLED, MFS_DISABLED },
    { PH_EMENU_HIGHLIGHT, MFS_HILITE }
};

/**
 * Creates a menu item.
 *
 * \param Flags A combination of the following:
 * \li \c PH_EMENU_DISABLED The menu item is greyed and cannot be selected.
 * \li \c PH_EMENU_CHECKED A check mark is displayed.
 * \li \c PH_EMENU_HIGHLIGHT The menu item is highlighted.
 * \li \c PH_EMENU_MENUBARBREAK Places the menu item in a new column, separated by a vertical line.
 * \li \c PH_EMENU_MENUBREAK Places the menu item in a new column, with no vertical line.
 * \li \c PH_EMENU_DEFAULT The menu item is displayed as the default item. This causes the text to
 * be bolded.
 * \li \c PH_EMENU_RADIOCHECK Uses a radio-button mark instead of a check mark.
 * \param Id A unique identifier for the menu item.
 * \param Text The text displayed for the menu item.
 * \param Bitmap A bitmap image for the menu item.
 * \param Context A user-defined value.
 */
PPH_EMENU_ITEM PhCreateEMenuItem(
    _In_ ULONG Flags,
    _In_ ULONG Id,
    _In_opt_ PWSTR Text,
    _In_opt_ HBITMAP Bitmap,
    _In_opt_ PVOID Context
    )
{
    PPH_EMENU_ITEM item;

    item = PhAllocate(sizeof(PH_EMENU_ITEM));
    memset(item, 0, sizeof(PH_EMENU_ITEM));

    item->Flags = Flags;
    item->Id = Id;
    item->Text = Text;
    item->Bitmap = Bitmap;
    item->Context = Context;

    return item;
}

/**
 * Frees resources used by a menu item and its children.
 *
 * \param Item The menu item.
 *
 * \remarks The menu item is NOT automatically removed from its parent. It is safe to call this
 * function while enumerating menu items.
 */
VOID PhpDestroyEMenuItem(
    _In_ PPH_EMENU_ITEM Item
    )
{
    if (Item->DeleteFunction)
        Item->DeleteFunction(Item);

    if ((Item->Flags & PH_EMENU_TEXT_OWNED) && Item->Text)
        PhFree(Item->Text);
    if ((Item->Flags & PH_EMENU_BITMAP_OWNED) && Item->Bitmap)
        DeleteBitmap(Item->Bitmap);

    if (Item->Items)
    {
        for (ULONG i = 0; i < Item->Items->Count; i++)
            PhpDestroyEMenuItem(Item->Items->Items[i]);

        PhDereferenceObject(Item->Items);
    }

    PhFree(Item);
}

/**
 * Frees resources used by a menu item and its children.
 *
 * \param Item The menu item.
 *
 * \remarks The menu item is automatically removed from its parent.
 */
VOID PhDestroyEMenuItem(
    _In_ PPH_EMENU_ITEM Item
    )
{
    // Remove the item from its parent, if it has one.
    if (Item->Parent)
        PhRemoveEMenuItem(NULL, Item, ULONG_MAX);

    PhpDestroyEMenuItem(Item);
}

/**
 * Finds a child menu item.
 *
 * \param Item The parent menu item.
 * \param Flags A combination of the following:
 * \li \c PH_EMENU_FIND_DESCEND Searches recursively within child menu items.
 * \li \c PH_EMENU_FIND_STARTSWITH Performs a partial text search instead of an exact search.
 * \li \c PH_EMENU_FIND_LITERAL Performs a literal search instead of ignoring prefix characters
 * (ampersands).
 * \param Text The text of the menu item to find. If NULL, the text is ignored.
 * \param Id The identifier of the menu item to find. If 0, the identifier is ignored.
 *
 * \return The found menu item, or NULL if the menu item could not be found.
 */
PPH_EMENU_ITEM PhFindEMenuItem(
    _In_ PPH_EMENU_ITEM Item,
    _In_ ULONG Flags,
    _In_opt_ PWSTR Text,
    _In_opt_ ULONG Id
    )
{
    return PhFindEMenuItemEx(Item, Flags, Text, Id, NULL, NULL);
}

/**
 * Finds a child menu item.
 *
 * \param Item The parent menu item.
 * \param Flags A combination of the following:
 * \li \c PH_EMENU_FIND_DESCEND Searches recursively within child menu items.
 * \li \c PH_EMENU_FIND_STARTSWITH Performs a partial text search instead of an exact search.
 * \li \c PH_EMENU_FIND_LITERAL Performs a literal search instead of ignoring prefix characters
 * (ampersands).
 * \param Text The text of the menu item to find. If NULL, the text is ignored.
 * \param Id The identifier of the menu item to find. If 0, the identifier is ignored.
 * \param FoundParent A variable which receives the parent of the found menu item.
 * \param FoundIndex A variable which receives the index of the found menu item.
 *
 * \return The found menu item, or NULL if the menu item could not be found.
 */
PPH_EMENU_ITEM PhFindEMenuItemEx(
    _In_ PPH_EMENU_ITEM Item,
    _In_ ULONG Flags,
    _In_opt_ PWSTR Text,
    _In_opt_ ULONG Id,
    _Out_opt_ PPH_EMENU_ITEM *FoundParent,
    _Out_opt_ PULONG FoundIndex
    )
{
    PH_STRINGREF searchText;
    ULONG i;
    PPH_EMENU_ITEM item;

    if (!Item->Items)
        return NULL;

    if (Text && (Flags & PH_EMENU_FIND_LITERAL))
        PhInitializeStringRef(&searchText, Text);

    for (i = 0; i < Item->Items->Count; i++)
    {
        item = Item->Items->Items[i];

        if (Text)
        {
            if (Flags & PH_EMENU_FIND_LITERAL)
            {
                PH_STRINGREF text;

                PhInitializeStringRef(&text, item->Text);

                if (Flags & PH_EMENU_FIND_STARTSWITH)
                {
                    if (PhStartsWithStringRef(&text, &searchText, TRUE))
                        goto FoundItemHere;
                }
                else
                {
                    if (PhEqualStringRef(&text, &searchText, TRUE))
                        goto FoundItemHere;
                }
            }
            else
            {
                if (PhCompareUnicodeStringZIgnoreMenuPrefix(Text, item->Text,
                    TRUE, !!(Flags & PH_EMENU_FIND_STARTSWITH)) == 0)
                    goto FoundItemHere;
            }
        }

        if (Id && item->Id == Id)
            goto FoundItemHere;

        if (Flags & PH_EMENU_FIND_DESCEND)
        {
            PPH_EMENU_ITEM foundItem = NULL;
            PPH_EMENU_ITEM foundParent = NULL;
            ULONG foundIndex = 0;

            foundItem = PhFindEMenuItemEx(item, Flags, Text, Id, &foundParent, &foundIndex);

            if (foundItem)
            {
                if (FoundParent)
                    *FoundParent = foundParent;
                if (FoundIndex)
                    *FoundIndex = foundIndex;

                return foundItem;
            }
        }
    }

    return NULL;

FoundItemHere:
    if (FoundParent)
        *FoundParent = Item;
    if (FoundIndex)
        *FoundIndex = i;

    return item;
}

/**
 * Determines the index of a menu item.
 *
 * \param Parent The parent menu item.
 * \param Item The child menu item.
 *
 * \return The index of the menu item, or -1 if the menu item was not found in the parent menu item.
 */
ULONG PhIndexOfEMenuItem(
    _In_ PPH_EMENU_ITEM Parent,
    _In_ PPH_EMENU_ITEM Item
    )
{
    if (!Parent->Items)
        return ULONG_MAX;

    return PhFindItemList(Parent->Items, Item);
}

/**
 * Inserts a menu item into a parent menu item.
 *
 * \param Parent The parent menu item.
 * \param Item The menu item to insert.
 * \param Index The index at which to insert the menu item. If the index is too large, the menu item
 * is inserted at the last position.
 */
VOID PhInsertEMenuItem(
    _Inout_ PPH_EMENU_ITEM Parent,
    _Inout_ PPH_EMENU_ITEM Item,
    _In_ ULONG Index
    )
{
    // Remove the item from its old parent if it has one.
    if (Item->Parent)
        PhRemoveEMenuItem(Item->Parent, Item, 0);

    if (!Parent->Items)
        Parent->Items = PhCreateList(16);

    if (Index > Parent->Items->Count)
        Index = Parent->Items->Count;

    if (Index == ULONG_MAX)
        PhAddItemList(Parent->Items, Item);
    else
        PhInsertItemList(Parent->Items, Index, Item);

    Item->Parent = Parent;
}

/**
 * Removes a menu item from its parent.
 *
 * \param Parent The parent menu item. If \a Item is NULL, this parameter must be specified.
 * \param Item The child menu item. This may be NULL if \a Index is specified.
 * \param Index The index of the menu item to remove. If \a Item is specified, this parameter is
 * ignored.
 */
BOOLEAN PhRemoveEMenuItem(
    _Inout_opt_ PPH_EMENU_ITEM Parent,
    _In_opt_ PPH_EMENU_ITEM Item,
    _In_opt_ ULONG Index
    )
{
    if (Item)
    {
        if (!Parent)
            Parent = Item->Parent;
        if (!Parent->Items)
            return FALSE;

        Index = PhFindItemList(Parent->Items, Item);

        if (Index == ULONG_MAX)
            return FALSE;
    }
    else
    {
        if (!Parent)
            return FALSE;
        if (!Parent->Items)
            return FALSE;
    }

    Item = Parent->Items->Items[Index];
    PhRemoveItemList(Parent->Items, Index);
    Item->Parent = NULL;

    return TRUE;
}

/**
 * Removes all children from a menu item.
 *
 * \param Parent The parent menu item.
 */
VOID PhRemoveAllEMenuItems(
    _Inout_ PPH_EMENU_ITEM Parent
    )
{
    ULONG i;

    if (!Parent->Items)
        return;

    for (i = 0; i < Parent->Items->Count; i++)
    {
        PhpDestroyEMenuItem(Parent->Items->Items[i]);
    }

    PhClearList(Parent->Items);
}

/**
 * Creates a root menu.
 */
PPH_EMENU PhCreateEMenu(
    VOID
    )
{
    PPH_EMENU menu;

    menu = PhAllocate(sizeof(PH_EMENU));
    memset(menu, 0, sizeof(PH_EMENU));
    menu->Items = PhCreateList(16);

    return menu;
}

/**
 * Frees resources used by a root menu and its children.
 *
 * \param Menu A root menu.
 */
VOID PhDestroyEMenu(
    _In_ PPH_EMENU Menu
    )
{
    ULONG i;

    for (i = 0; i < Menu->Items->Count; i++)
    {
        PhpDestroyEMenuItem(Menu->Items->Items[i]);
    }

    PhDereferenceObject(Menu->Items);
    PhFree(Menu);
}

/**
 * Initializes a data structure containing additional information resulting from a call to
 * PhEMenuToHMenu().
 */
VOID PhInitializeEMenuData(
    _Out_ PPH_EMENU_DATA Data
    )
{
    Data->IdToItem = PhCreateList(16);
}

/**
 * Frees resources used by a data structure initialized by PhInitializeEMenuData().
 */
VOID PhDeleteEMenuData(
    _Inout_ PPH_EMENU_DATA Data
    )
{
    PhDereferenceObject(Data->IdToItem);
}

/**
 * Converts an EMENU to a Windows menu object.
 *
 * \param Menu The menu item to convert.
 * \param Flags A combination of the following:
 * \li \c PH_EMENU_CONVERT_ID Automatically assigns a unique identifier to each converted menu item.
 * The resulting mappings are placed in \a Data.
 * \param Data Additional data resulting from the conversion. The data structure must be initialized
 * by PhInitializeEMenuData() prior to calling this function.
 *
 * \return A menu handle. The menu object must be destroyed using DestroyMenu() when it is no longer
 * needed.
 */
HMENU PhEMenuToHMenu(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG Flags,
    _Inout_opt_ PPH_EMENU_DATA Data
    )
{
    HMENU menuHandle;

    if ((Menu->Flags & PH_EMENU_MAINMENU) == PH_EMENU_MAINMENU)
    {
        if (!(menuHandle = CreateMenu()))
            return NULL;
    }
    else
    {
        if (!(menuHandle = CreatePopupMenu()))
            return NULL;
    }

    PhEMenuToHMenu2(menuHandle, Menu, Flags, Data);

    if (!(Menu->Flags & PH_EMENU_SEPARATECHECKSPACE))
    {
        MENUINFO menuInfo;

        memset(&menuInfo, 0, sizeof(MENUINFO));
        menuInfo.cbSize = sizeof(MENUINFO);
        menuInfo.fMask = MIM_STYLE;
        menuInfo.dwStyle = MNS_CHECKORBMP;

        if (WindowsVersion < WINDOWS_10_19H2 && PhGetIntegerSetting(L"EnableThemeSupport"))
        {
            static HBRUSH themeBrush = NULL;

            if (!themeBrush)
                themeBrush = CreateSolidBrush(PhThemeWindowBackgroundColor);

            menuInfo.fMask |= MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
            menuInfo.hbrBack = themeBrush;
        }

        SetMenuInfo(menuHandle, &menuInfo);
    }

    return menuHandle;
}

/**
 * Converts an EMENU to a Windows menu object.
 *
 * \param MenuHandle A handle to a Windows menu object.
 * \param Menu The menu item to convert. The items are appended to \a MenuHandle.
 * \param Flags A combination of the following:
 * \li \c PH_EMENU_CONVERT_ID Automatically assigns a unique identifier to each converted menu item.
 * The resulting mappings are placed in \a Data.
 * \param Data Additional data resulting from the conversion. The data structure must be initialized
 * by PhInitializeEMenuData() prior to calling this function.
 */
VOID PhEMenuToHMenu2(
    _In_ HMENU MenuHandle,
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG Flags,
    _Inout_opt_ PPH_EMENU_DATA Data
    )
{
    ULONG i;
    PPH_EMENU_ITEM item;
    MENUITEMINFO menuItemInfo;

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
            menuItemInfo.hbmpItem = item->Bitmap;

            if (WindowsVersion < WINDOWS_10_19H2)
            {
                if (!PhGetIntegerSetting(L"EnableThemeSupport"))
                    menuItemInfo.fMask |= MIIM_BITMAP;
            }
            else
            {
                menuItemInfo.fMask |= MIIM_BITMAP;
            }
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
            menuItemInfo.hSubMenu = PhEMenuToHMenu(item, Flags, Data);
        }

        // Themes
        if (WindowsVersion < WINDOWS_10_19H2)
        {
            if (PhGetIntegerSetting(L"EnableThemeSupport"))
            {
                menuItemInfo.fType |= MFT_OWNERDRAW;
            }
        }
        else
        {
            if (item->Flags & PH_EMENU_MAINMENU)
            {
                if (PhGetIntegerSetting(L"EnableThemeSupport"))
                {
                    menuItemInfo.fType |= MFT_OWNERDRAW;
                }
            }
        }

        InsertMenuItem(MenuHandle, MAXUINT, TRUE, &menuItemInfo);
    }
}

/**
 * Converts a Windows menu object to an EMENU.
 *
 * \param MenuItem The menu item in which the converted menu items will be placed.
 * \param MenuHandle A menu handle.
 */
VOID PhHMenuToEMenuItem(
    _Inout_ PPH_EMENU_ITEM MenuItem,
    _In_ HMENU MenuHandle
    )
{
    INT i;
    INT count;

    count = GetMenuItemCount(MenuHandle);

    if (count == INT_ERROR)
        return;

    for (i = 0; i < count; i++)
    {
        MENUITEMINFO menuItemInfo;
        PPH_EMENU_ITEM menuItem;
        WCHAR buffer[MAX_PATH] = L"";

        menuItemInfo.cbSize = sizeof(menuItemInfo);
        menuItemInfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING | MIIM_SUBMENU;
        menuItemInfo.cch = RTL_NUMBER_OF(buffer);
        menuItemInfo.dwTypeData = buffer;

        if (!GetMenuItemInfo(MenuHandle, i, TRUE, &menuItemInfo))
            continue;

        menuItem = PhCreateEMenuItem(
            PH_EMENU_TEXT_OWNED,
            menuItemInfo.wID,
            PhDuplicateStringZ(buffer),
            NULL,
            NULL
            );

        if (menuItemInfo.fType & MFT_SEPARATOR)
            menuItem->Flags |= PH_EMENU_SEPARATOR;

        PhMapFlags2(
            &menuItem->Flags,
            menuItemInfo.fType,
            EMenuTypeMappings,
            RTL_NUMBER_OF(EMenuTypeMappings)
            );
        PhMapFlags2(
            &menuItem->Flags,
            menuItemInfo.fState,
            EMenuStateMappings,
            RTL_NUMBER_OF(EMenuStateMappings)
            );

        if (menuItemInfo.hSubMenu)
            PhHMenuToEMenuItem(menuItem, menuItemInfo.hSubMenu);

        PhInsertEMenuItem(MenuItem, menuItem, ULONG_MAX);
    }
}

/**
 * Loads a menu resource and converts it to an EMENU.
 *
 * \param MenuItem The menu item in which the converted menu items will be placed.
 * \param InstanceHandle The module containing the menu resource.
 * \param Resource The resource identifier.
 * \param SubMenuIndex The index of the sub menu to use, or -1 to use the root menu.
 */
VOID PhLoadResourceEMenuItem(
    _Inout_ PPH_EMENU_ITEM MenuItem,
    _In_ HINSTANCE InstanceHandle,
    _In_ PWSTR Resource,
    _In_ INT SubMenuIndex
    )
{
    HMENU menu;
    HMENU realMenu;

    menu = PhLoadMenu(InstanceHandle, Resource);

    if (SubMenuIndex != INT_ERROR)
        realMenu = GetSubMenu(menu, SubMenuIndex);
    else
        realMenu = menu;

    PhHMenuToEMenuItem(MenuItem, realMenu);

    DestroyMenu(menu);
}

/**
 * Displays a menu.
 *
 * \param Menu A menu.
 * \param WindowHandle The window that owns the popup menu.
 * \param Flags A combination of the following:
 * \li \c PH_EMENU_SHOW_SEND_COMMAND A WM_COMMAND message is sent to the window when the user clicks
 * on a menu item.
 * \li \c PH_EMENU_SHOW_LEFTRIGHT The user can select menu items with both the left and right mouse
 * buttons.
 * \param Align The alignment of the menu.
 * \param X The horizontal location of the menu.
 * \param Y The vertical location of the menu.
 *
 * \return The selected menu item, or NULL if the menu was cancelled.
 */
PPH_EMENU_ITEM PhShowEMenu(
    _In_ PPH_EMENU Menu,
    _In_ HWND WindowHandle,
    _In_ ULONG Flags,
    _In_ ULONG Align,
    _In_ ULONG X,
    _In_ ULONG Y
    )
{
    PPH_EMENU_ITEM selectedItem;
    ULONG result;
    ULONG flags;
    PH_EMENU_DATA data;
    HMENU popupMenu;

    selectedItem = NULL;
    flags = TPM_RETURNCMD | TPM_NONOTIFY;

    // Flags

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

    if (popupMenu = PhEMenuToHMenu(Menu, PH_EMENU_CONVERT_ID, &data))
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

    if ((Flags & PH_EMENU_SHOW_SEND_COMMAND) && selectedItem && selectedItem->Id != 0)
        SendMessage(WindowHandle, WM_COMMAND, MAKEWPARAM(selectedItem->Id, 0), 0);

    return selectedItem;
}

/**
 * Sets the flags of a menu item.
 *
 * \param Item The parent menu item.
 * \param Id The identifier of the child menu item.
 * \param Mask The flags to modify.
 * \param Value The new value of the flags.
 */
BOOLEAN PhSetFlagsEMenuItem(
    _Inout_ PPH_EMENU_ITEM Item,
    _In_ ULONG Id,
    _In_ ULONG Mask,
    _In_ ULONG Value
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

/**
 * Sets flags for all children of a menu item.
 *
 * \param Item The parent menu item.
 * \param Mask The flags to modify.
 * \param Value The new value of the flags.
 */
VOID PhSetFlagsAllEMenuItems(
    _In_ PPH_EMENU_ITEM Item,
    _In_ ULONG Mask,
    _In_ ULONG Value
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

VOID PhModifyEMenuItem(
    _Inout_ PPH_EMENU_ITEM Item,
    _In_ ULONG ModifyFlags,
    _In_ ULONG OwnedFlags,
    _In_opt_ PWSTR Text,
    _In_opt_ HBITMAP Bitmap
    )
{
    if (ModifyFlags & PH_EMENU_MODIFY_TEXT)
    {
        if ((Item->Flags & PH_EMENU_TEXT_OWNED) && Item->Text)
            PhFree(Item->Text);

        Item->Text = Text;
        Item->Flags &= ~PH_EMENU_TEXT_OWNED;
        Item->Flags |= OwnedFlags & PH_EMENU_TEXT_OWNED;
    }

    if (ModifyFlags & PH_EMENU_MODIFY_BITMAP)
    {
        if ((Item->Flags & PH_EMENU_BITMAP_OWNED) && Item->Bitmap)
            DeleteBitmap(Item->Bitmap);

        Item->Bitmap = Bitmap;
        Item->Flags &= ~PH_EMENU_BITMAP_OWNED;
        Item->Flags |= OwnedFlags & PH_EMENU_BITMAP_OWNED;
    }
}
