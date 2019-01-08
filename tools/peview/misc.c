/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2011 wj32
 * Copyright (C) 2019 dmex
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

#include <peview.h>
#include <emenu.h>

#include <shobjidl.h>

#include <cpysave.h>

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
    WCHAR headerText[MAX_PATH];

    if (!GetCursorPos(&location))
        return FALSE;
    if (!ScreenToClient(ListViewHandle, &location))
        return FALSE;

    memset(&lvHitInfo, 0, sizeof(LVHITTESTINFO));
    lvHitInfo.pt = location;

    if (ListView_SubItemHitTest(ListViewHandle, &lvHitInfo) == -1)
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

    if ((selectedIndex = ListView_GetNextItem(ListViewHandle, -1, LVNI_SELECTED)) != -1)
    {
        if (ListView_GetItemRect(ListViewHandle, selectedIndex, &bounds, LVIR_BOUNDS))
        {
            Point->x = bounds.left + PhSmallIconSize.X / 2;
            Point->y = bounds.top + PhSmallIconSize.Y / 2;

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
            PvGetListViewContextMenuPoint((HWND)wParam, &point);

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
