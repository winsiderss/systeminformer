/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#ifndef _PH_EMENU_H
#define _PH_EMENU_H

EXTERN_C_START

#define PH_EMENU_DISABLED 0x1
#define PH_EMENU_CHECKED 0x2
#define PH_EMENU_HIGHLIGHT 0x4
#define PH_EMENU_MENUBARBREAK 0x8
#define PH_EMENU_MENUBREAK 0x10
#define PH_EMENU_DEFAULT 0x20
#define PH_EMENU_MOUSESELECT 0x40
#define PH_EMENU_RADIOCHECK 0x80
#define PH_EMENU_RIGHTORDER 0x100

#define PH_EMENU_SEPARATECHECKSPACE 0x100000
#define PH_EMENU_SEPARATOR 0x200000
#define PH_EMENU_MAINMENU 0x400000
#define PH_EMENU_CALLBACK 0x800000

#define PH_EMENU_TEXT_OWNED 0x80000000
#define PH_EMENU_BITMAP_OWNED 0x40000000

typedef struct _PH_EMENU_ITEM *PPH_EMENU_ITEM;

typedef _Function_class_(PH_EMENU_ITEM_DELETE_FUNCTION)
VOID NTAPI PH_EMENU_ITEM_DELETE_FUNCTION(
    _In_ PPH_EMENU_ITEM Item
    );
typedef PH_EMENU_ITEM_DELETE_FUNCTION* PPH_EMENU_ITEM_DELETE_FUNCTION;

typedef _Function_class_(PH_EMENU_ITEM_DELAY_FUNCTION)
VOID NTAPI PH_EMENU_ITEM_DELAY_FUNCTION(
    _In_ HMENU Menu,
    _In_ PPH_EMENU_ITEM Item
    );
typedef PH_EMENU_ITEM_DELAY_FUNCTION* PPH_EMENU_ITEM_DELAY_FUNCTION;

typedef struct _PH_EMENU_ITEM
{
    ULONG Flags;
    ULONG Id;
    PWSTR Text;
    HBITMAP Bitmap;

    PVOID Parameter;
    PVOID Context;

    PPH_EMENU_ITEM_DELETE_FUNCTION DeleteFunction;
    PPH_EMENU_ITEM_DELAY_FUNCTION DelayFunction;

    PPH_EMENU_ITEM Parent;
    PPH_LIST Items;
} PH_EMENU_ITEM, *PPH_EMENU_ITEM;

typedef struct _PH_EMENU_ITEM PH_EMENU, *PPH_EMENU;

PHLIBAPI
PPH_EMENU_ITEM
NTAPI
PhCreateEMenuItem(
    _In_ ULONG Flags,
    _In_ ULONG Id,
    _In_opt_ PCWSTR Text,
    _In_opt_ HBITMAP Bitmap,
    _In_opt_ PVOID Context
    );

PPH_EMENU_ITEM
PhCreateEMenuItemCallback(
    _In_ ULONG Flags,
    _In_ ULONG Id,
    _In_opt_ PCWSTR Text,
    _In_opt_ HBITMAP Bitmap,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_EMENU_ITEM_DELAY_FUNCTION DelayFunction
    );

PHLIBAPI
VOID
NTAPI
PhDestroyEMenuItem(
    _In_ PPH_EMENU_ITEM Item
    );

#define PH_EMENU_FIND_DESCEND 0x1
#define PH_EMENU_FIND_STARTSWITH 0x2
#define PH_EMENU_FIND_LITERAL 0x4

PHLIBAPI
PPH_EMENU_ITEM
NTAPI
PhFindEMenuItem(
    _In_ PPH_EMENU_ITEM Item,
    _In_ ULONG Flags,
    _In_opt_ PCWSTR Text,
    _In_opt_ ULONG Id
    );

PHLIBAPI
_Success_(return != NULL)
PPH_EMENU_ITEM
NTAPI
PhFindEMenuItemEx(
    _In_ PPH_EMENU_ITEM Item,
    _In_ ULONG Flags,
    _In_opt_ PCWSTR Text,
    _In_opt_ ULONG Id,
    _Out_opt_ PPH_EMENU_ITEM *FoundParent,
    _Out_opt_ PULONG FoundIndex
    );

PHLIBAPI
ULONG
NTAPI
PhIndexOfEMenuItem(
    _In_ PPH_EMENU_ITEM Parent,
    _In_ PPH_EMENU_ITEM Item
    );

PHLIBAPI
VOID
NTAPI
PhInsertEMenuItem(
    _Inout_ PPH_EMENU_ITEM Parent,
    _Inout_ PPH_EMENU_ITEM Item,
    _In_ ULONG Index
    );

PHLIBAPI
BOOLEAN
NTAPI
PhRemoveEMenuItem(
    _Inout_opt_ PPH_EMENU_ITEM Parent,
    _In_opt_ PPH_EMENU_ITEM Item,
    _In_opt_ ULONG Index
    );

PHLIBAPI
VOID PhRemoveAllEMenuItems(
    _Inout_ PPH_EMENU_ITEM Parent
    );

PHLIBAPI
PPH_EMENU
NTAPI
PhCreateEMenu(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhDestroyEMenu(
    _In_ PPH_EMENU Menu
    );

#define PH_EMENU_CONVERT_ID 0x1

typedef struct _PH_EMENU_DATA
{
    PPH_LIST IdToItem;
} PH_EMENU_DATA, *PPH_EMENU_DATA;

PHLIBAPI
VOID
NTAPI
PhInitializeEMenuData(
    _Out_ PPH_EMENU_DATA Data
    );

PHLIBAPI
VOID
NTAPI
PhDeleteEMenuData(
    _Inout_ PPH_EMENU_DATA Data
    );

PHLIBAPI
HMENU
NTAPI
PhEMenuToHMenu(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG Flags,
    _Inout_opt_ PPH_EMENU_DATA Data
    );

PHLIBAPI
VOID
NTAPI
PhEMenuToHMenu2(
    _In_ HMENU MenuHandle,
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG Flags,
    _Inout_opt_ PPH_EMENU_DATA Data
    );

PHLIBAPI
VOID
NTAPI
PhHMenuToEMenuItem(
    _Inout_ PPH_EMENU_ITEM MenuItem,
    _In_ HMENU MenuHandle
    );

PHLIBAPI
VOID
NTAPI
PhLoadResourceEMenuItem(
    _Inout_ PPH_EMENU_ITEM MenuItem,
    _In_ HINSTANCE InstanceHandle,
    _In_ PCWSTR Resource,
    _In_ LONG SubMenuIndex
    );

#define PH_EMENU_SHOW_SEND_COMMAND 0x1
#define PH_EMENU_SHOW_LEFTRIGHT 0x2

PHLIBAPI
PPH_EMENU_ITEM
NTAPI
PhShowEMenu(
    _In_ PPH_EMENU Menu,
    _In_ HWND WindowHandle,
    _In_ ULONG Flags,
    _In_ ULONG Align,
    _In_ ULONG X,
    _In_ ULONG Y
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSetFlagsEMenuItem(
    _Inout_ PPH_EMENU_ITEM Item,
    _In_ ULONG Id,
    _In_ ULONG Mask,
    _In_ ULONG Value
    );

PHLIBAPI
VOID
NTAPI
PhSetFlagsAllEMenuItems(
    _In_ PPH_EMENU_ITEM Item,
    _In_ ULONG Mask,
    _In_ ULONG Value
    );

#define PH_EMENU_MODIFY_TEXT 0x1
#define PH_EMENU_MODIFY_BITMAP 0x2

PHLIBAPI
VOID
NTAPI
PhModifyEMenuItem(
    _Inout_ PPH_EMENU_ITEM Item,
    _In_ ULONG ModifyFlags,
    _In_ ULONG OwnedFlags,
    _In_opt_ PWSTR Text,
    _In_opt_ HBITMAP Bitmap
    );

VOID PhSetHMenuStyle(
    _In_ HMENU Menu,
    _In_ BOOLEAN MainMenu
    );

BOOLEAN PhSetHMenuWindow(
    _In_ HWND WindowHandle,
    _In_ HMENU MenuHandle
    );

BOOLEAN PhSetHMenuNotify(
    _In_ HMENU MenuHandle
    );

VOID PhDeleteHMenu(
    _In_ HMENU Menu
    );

_Success_(return)
BOOLEAN PhGetHMenuStringToBuffer(
    _In_ HMENU Menu,
    _In_ ULONG Id,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

PPH_EMENU_ITEM PhGetMenuData(
    _In_ HMENU Menu,
    _In_ ULONG Index
    );

VOID PhMenuCallbackDispatch(
    _In_ HMENU Menu,
    _In_ ULONG Index
    );

// Convenience functions

FORCEINLINE
PPH_EMENU_ITEM PhCreateEMenuSeparator(
    VOID
    )
{
    return PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL);
}

FORCEINLINE
PPH_EMENU_ITEM PhCreateEMenuItemEmpty(
    VOID
    )
{
    return PhCreateEMenuItem(0, USHRT_MAX, NULL, NULL, NULL);
}

FORCEINLINE
BOOLEAN PhEnableEMenuItem(
    _Inout_ PPH_EMENU_ITEM Item,
    _In_ ULONG Id,
    _In_ BOOLEAN Enable
    )
{
    return PhSetFlagsEMenuItem(Item, Id, PH_EMENU_DISABLED, Enable ? 0 : PH_EMENU_DISABLED);
}

FORCEINLINE
VOID PhSetDisabledEMenuItem(
    _In_ PPH_EMENU_ITEM Item
    )
{
    Item->Flags |= PH_EMENU_DISABLED;
}

FORCEINLINE
VOID PhSetEnabledEMenuItem(
    _In_ PPH_EMENU_ITEM Item,
    _In_ BOOLEAN Enable
    )
{
    if (Enable)
        Item->Flags &= ~PH_EMENU_DISABLED;
    else
        Item->Flags |= PH_EMENU_DISABLED;
}

EXTERN_C_END

#endif
