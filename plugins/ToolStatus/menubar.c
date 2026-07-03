/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     Codex   2026
 *
 */

#include "toolstatus.h"

HWND TsMenuBarWindowHandle = NULL;
HWND TsMenuBarParentWindowHandle = NULL;
HWND TsMenuBarOldFocusWindowHandle = NULL;
HHOOK TsMenuBarMessageHookHandle = NULL;
HMENU TsMenuBarSelectedMenuHandle = NULL;
POINT TsMenuBarLastMouseClientPoint = { 0 };
LONG TsMenuBarHotItemIndex = INT_ERROR;
LONG TsMenuBarPressedItemIndex = INT_ERROR;
LONG TsMenuBarSelectedMenuItemIndex = INT_ERROR;
ULONG TsMenuBarSelectedMenuFlags = 0;
HTHEME TsMenuBarThemeHandle = NULL;
BOOLEAN TsMenuBarMenuActive = FALSE;
BOOLEAN TsMenuBarContinueHotTrack = FALSE;
BOOLEAN TsMenuBarSelectFromKeyboard = FALSE;
BOOLEAN TsMenuBarDelayedActivation = FALSE;
BOOLEAN TsMenuBarRtlLayout = FALSE;

#define TOOLSTATUS_MENUBAR_LOG(format, ...) dprintf("[ToolStatus.MenuBar] " format "\n" __VA_OPT__(,) __VA_ARGS__)
#define TOOLSTATUS_MENUBAR_SHOWPOPUP TB_CUSTOMIZE

BOOLEAN ToolStatusMenuBarInstallHook(
    VOID
    );

VOID ToolStatusMenuBarRemoveHook(
    VOID
    );

VOID ToolStatusMenuBarResetTrackingState(
    VOID
    )
{
    TOOLSTATUS_MENUBAR_LOG("ResetTrackingState");

    TsMenuBarHotItemIndex = INT_ERROR;
    TsMenuBarPressedItemIndex = INT_ERROR;
    TsMenuBarSelectedMenuHandle = NULL;
    TsMenuBarSelectedMenuItemIndex = INT_ERROR;
    TsMenuBarSelectedMenuFlags = 0;
    TsMenuBarMenuActive = FALSE;
    TsMenuBarContinueHotTrack = FALSE;
    TsMenuBarSelectFromKeyboard = FALSE;
}

VOID ToolStatusMenuBarUpdateUiState(
    _In_ BOOLEAN KeyboardActivity
    )
{
    WORD action;
    BOOL showAcceleratorsAlways;

    if (!TsMenuBarWindowHandle)
        return;

    TOOLSTATUS_MENUBAR_LOG("UpdateUiState keyboard=%lu", KeyboardActivity);

    if (KeyboardActivity)
    {
        action = UIS_CLEAR;
    }
    else
    {
        if (!PhGetSystemParametersInfo(SPI_GETMENUUNDERLINES, 0, &showAcceleratorsAlways, PhGetWindowDpi(TsMenuBarWindowHandle)))
            showAcceleratorsAlways = TRUE;

        action = showAcceleratorsAlways ? UIS_CLEAR : UIS_SET;
    }

    PostMessage(
        TsMenuBarWindowHandle,
        WM_CHANGEUISTATE,
        MAKEWPARAM(action, UISF_HIDEACCEL),
        0
        );
}

 VOID ToolStatusMenuBarDeactivate(
    _In_ BOOLEAN RestoreFocus
    )
{
    if (!TsMenuBarWindowHandle)
        return;

    TOOLSTATUS_MENUBAR_LOG(
        "Deactivate restoreFocus=%lu oldFocus=%p",
        RestoreFocus,
        TsMenuBarOldFocusWindowHandle
        );

    ToolStatusMenuBarUpdateUiState(FALSE);
    SendMessage(TsMenuBarWindowHandle, TB_SETHOTITEM, INT_ERROR, 0);
    ToolStatusMenuBarResetTrackingState();

    // Force a repaint now that all hot/pressed/menu-active state is cleared. The
    // TB_SETHOTITEM above is a no-op (and triggers no repaint) when the hot item was
    // already cleared earlier in the close sequence, which would otherwise leave the
    // last button painted in its hot/selected state. (dmex)
    InvalidateRect(TsMenuBarWindowHandle, NULL, FALSE);

    if (RestoreFocus && TsMenuBarOldFocusWindowHandle)
    {
        SetFocus(TsMenuBarOldFocusWindowHandle);
        TsMenuBarOldFocusWindowHandle = NULL;
    }
}

 VOID ToolStatusMenuBarSetHotItem(
    _In_ LONG Index
    )
{
    if (Index < 0)
        Index = INT_ERROR;

    TOOLSTATUS_MENUBAR_LOG("SetHotItem index=%ld", Index);

    TsMenuBarHotItemIndex = Index;

    if (TsMenuBarWindowHandle)
    {
        SendMessage(
            TsMenuBarWindowHandle,
            TB_SETHOTITEM,
            Index,
            0
            );
    }
}

LONG ToolStatusMenuBarGetButtonCount(
    VOID
    )
{
    if (!TsMenuBarWindowHandle)
        return 0;

    return (LONG)SendMessage(
        TsMenuBarWindowHandle,
        TB_BUTTONCOUNT,
        0,
        0
        );
}

LONG ToolStatusMenuBarWrapButtonIndex(
    _In_ LONG Index
    )
{
    LONG count;

    count = ToolStatusMenuBarGetButtonCount();

    if (count <= 0)
        return INT_ERROR;

    if (Index < 0)
        return count - 1;
    if (Index >= count)
        return 0;

    return Index;
}

BOOLEAN ToolStatusMenuBarGetButtonRect(
    _In_ LONG Index,
    _Out_ PRECT Rect
    )
{
    if (!TsMenuBarWindowHandle)
        return FALSE;

    if (!SendMessage(
        TsMenuBarWindowHandle,
        TB_GETITEMRECT,
        Index,
        (LPARAM)Rect
        ))
    {
        return FALSE;
    }

    MapWindowPoints(TsMenuBarWindowHandle, HWND_DESKTOP, (PPOINT)Rect, 2);
    return TRUE;
}

LONG ToolStatusMenuBarHitTest(
    _In_ PPOINT ClientPoint
    )
{
    return (LONG)SendMessage(
        TsMenuBarWindowHandle,
        TB_HITTEST,
        0,
        (LPARAM)ClientPoint
        );
}

VOID ToolStatusMenuBarResetHotItemFromMouse(
    VOID
    )
{
    POINT cursorPos;
    LONG itemIndex;

    if (!TsMenuBarWindowHandle)
        return;

    if (!GetCursorPos(&cursorPos))
        return;

    MapWindowPoints(HWND_DESKTOP, TsMenuBarWindowHandle, &cursorPos, 1);
    itemIndex = ToolStatusMenuBarHitTest(&cursorPos);

    if (itemIndex < 0)
        itemIndex = INT_ERROR;

    ToolStatusMenuBarSetHotItem(itemIndex);
}

LONG ToolStatusMenuBarMapAccelerator(
    _In_ WCHAR Character
    )
{
    LONG index;

    if (!TsMenuBarWindowHandle)
        return INT_ERROR;

    index = INT_ERROR;

    if (SendMessage(
        TsMenuBarWindowHandle,
        TB_MAPACCELERATOR,
        RtlDowncaseUnicodeChar(Character),
        (LPARAM)&index
        ))
    {
        TOOLSTATUS_MENUBAR_LOG("MapAccelerator char=%lc index=%ld", Character, index);
        return index;
    }

    TOOLSTATUS_MENUBAR_LOG("MapAccelerator char=%lc index=none", Character);
    return INT_ERROR;
}

VOID ToolStatusMenuBarChangeDropdown(
    _In_ LONG ItemIndex,
    _In_ BOOLEAN FromKeyboard
    )
{
    TOOLSTATUS_MENUBAR_LOG("ChangeDropdown item=%ld fromKeyboard=%lu", ItemIndex, FromKeyboard);

    TsMenuBarPressedItemIndex = ItemIndex;
    TsMenuBarSelectFromKeyboard = FromKeyboard;
    TsMenuBarContinueHotTrack = TRUE;
    SendMessage(TsMenuBarWindowHandle, WM_CANCELMODE, 0, 0);
}

VOID ToolStatusMenuBarQueueDropdown(
    _In_ LONG ItemIndex,
    _In_ BOOLEAN FromKeyboard
    )
{
    TOOLSTATUS_MENUBAR_LOG("QueueDropdown item=%ld fromKeyboard=%lu", ItemIndex, FromKeyboard);

    TsMenuBarPressedItemIndex = ItemIndex;
    TsMenuBarSelectFromKeyboard = FromKeyboard;
    PostMessage(TsMenuBarWindowHandle, TOOLSTATUS_MENUBAR_SHOWPOPUP, 0, 0);
}

BOOLEAN ToolStatusMenuBarTrackPopupMenu(
    _In_ LONG ButtonIndex,
    _In_ BOOLEAN FromKeyboard
    )
{
    BOOLEAN commandHandled;
    PPH_EMENU selectedMenu;
    PPH_EMENU_ITEM selectedMenuItem;

    if (!TsMenuBarWindowHandle)
        return FALSE;

    TOOLSTATUS_MENUBAR_LOG("TrackPopupMenu begin button=%ld fromKeyboard=%lu", ButtonIndex, FromKeyboard);

    TsMenuBarOldFocusWindowHandle = GetFocus();
    TsMenuBarPressedItemIndex = ButtonIndex;
    TsMenuBarSelectFromKeyboard = FromKeyboard;
    TsMenuBarMenuActive = TRUE;

    if (!ToolStatusMenuBarInstallHook())
    {
        TsMenuBarMenuActive = FALSE;
        TOOLSTATUS_MENUBAR_LOG("TrackPopupMenu abort hook install failed");
        return FALSE;
    }

    SetFocus(TsMenuBarWindowHandle);
    commandHandled = FALSE;
    selectedMenu = NULL;
    selectedMenuItem = NULL;
    TsMenuBarContinueHotTrack = TRUE;

    while (TsMenuBarContinueHotTrack)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM selectedItem;
        PH_EMENU_DATA menuData;
        HMENU popupMenu;
        RECT buttonRect;
        DWORD buttonState;
        //TPMPARAMS trackParams;
        //ULONG popupFlags;
        //ULONG popupResult;
        LONG itemIndex;

        itemIndex = TsMenuBarPressedItemIndex;
        TsMenuBarSelectFromKeyboard = FromKeyboard;
        TsMenuBarContinueHotTrack = FALSE;

        TOOLSTATUS_MENUBAR_LOG("TrackPopupMenu loop item=%ld", itemIndex);

        menu = SystemInformer_GetMainSubMenu(itemIndex);

        if (!menu || !ToolStatusMenuBarGetButtonRect(itemIndex, &buttonRect))
        {
            TOOLSTATUS_MENUBAR_LOG("TrackPopupMenu missing submenu/rect item=%ld menu=%p", itemIndex, menu);
            if (menu)
                PhDestroyEMenuItem(menu);
            break;
        }

        PhInitializeEMenuData(&menuData);
        popupMenu = NULL;
        selectedItem = NULL;

        ToolStatusMenuBarSetHotItem(itemIndex);
        buttonState = (DWORD)SendMessage(
            TsMenuBarWindowHandle,
            TB_GETSTATE,
            itemIndex,
            0
            );

        SendMessage(
            TsMenuBarWindowHandle,
            TB_SETSTATE,
            itemIndex,
            MAKELONG(buttonState | TBSTATE_PRESSED, 0)
            );

        if (TsMenuBarSelectFromKeyboard)
        {
            keybd_event(VK_DOWN, 0, 0, 0);
            keybd_event(VK_DOWN, 0, KEYEVENTF_KEYUP, 0);
            TsMenuBarSelectFromKeyboard = FALSE;
        }

        if (MainWindowHandle)
        {
            TOOLSTATUS_MENUBAR_LOG("TrackPopupMenu foreground window=%p", MainWindowHandle);
            SetForegroundWindow(MainWindowHandle);
        }

        selectedItem = PhShowEMenu(
            menu,
            TsMenuBarWindowHandle,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            TsMenuBarRtlLayout ? buttonRect.right : buttonRect.left,
            buttonRect.bottom
            );

        //memset(&trackParams, 0, sizeof(TPMPARAMS));
        //trackParams.cbSize = sizeof(TPMPARAMS);
        //trackParams.rcExclude = buttonRect;

        //popupFlags = TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN;

        //if (TsMenuBarRtlLayout)
        //    popupFlags |= TPM_LAYOUTRTL;

        //popupMenu = PhEMenuToHMenu(menu, PH_EMENU_CONVERT_ID, &menuData);

        //if (popupMenu)
        //{
        //    popupResult = TrackPopupMenuEx(
        //        popupMenu,
        //        popupFlags,
        //        TsMenuBarRtlLayout ? buttonRect.right : buttonRect.left,
        //        buttonRect.bottom,
        //        TsMenuBarWindowHandle,
        //        &trackParams
        //        );

        //    if (popupResult != 0)
        //        selectedItem = PhItemList(menuData.IdToItem, popupResult - 1);

        //    DestroyMenu(popupMenu);
        //}

        TOOLSTATUS_MENUBAR_LOG(
            "TrackPopupMenu result item=%ld selected=%p id=%lu continue=%lu pressed=%ld",
            itemIndex,
            selectedItem,
            selectedItem ? selectedItem->Id : 0,
            TsMenuBarContinueHotTrack,
            TsMenuBarPressedItemIndex
            );

        if (MainWindowHandle)
            PostMessage(MainWindowHandle, WM_NULL, 0, 0);

        SendMessage(
            TsMenuBarWindowHandle,
            TB_SETSTATE,
            itemIndex,
            MAKELONG(buttonState, 0)
            );

        if (selectedItem)
        {
            selectedMenu = menu;
            selectedMenuItem = selectedItem;
            commandHandled = TRUE;
        }

        PhDeleteEMenuData(&menuData);

        if (!selectedItem)
            PhDestroyEMenuItem(menu);

        FromKeyboard = TsMenuBarSelectFromKeyboard;

        if (selectedItem)
            break;

        if (TsMenuBarPressedItemIndex < 0)
            break;
    }

    ToolStatusMenuBarResetHotItemFromMouse();
    ToolStatusMenuBarRemoveHook();
    TsMenuBarMenuActive = FALSE;

    ToolStatusMenuBarDeactivate(TRUE);

    if (selectedMenuItem)
    {
        TOOLSTATUS_MENUBAR_LOG("TrackPopupMenu dispatch id=%lu context=%p", selectedMenuItem->Id, selectedMenuItem);
        SystemInformer_SetMainSubCmd(selectedMenuItem->Id, selectedMenuItem);
    }

    if (selectedMenu)
        PhDestroyEMenuItem(selectedMenu);

    TOOLSTATUS_MENUBAR_LOG("TrackPopupMenu end handled=%lu", commandHandled);

    return commandHandled;
}

BOOLEAN ToolStatusMenuBarMoveHotItem(
    _In_ LONG Direction
    )
{
    LONG nextIndex;

    nextIndex = ToolStatusMenuBarWrapButtonIndex(TsMenuBarHotItemIndex + Direction);

    TOOLSTATUS_MENUBAR_LOG(
        "MoveHotItem direction=%ld current=%ld next=%ld",
        Direction,
        TsMenuBarHotItemIndex,
        nextIndex
        );

    if (nextIndex < 0)
        return FALSE;

    ToolStatusMenuBarSetHotItem(nextIndex);
    ToolStatusMenuBarUpdateUiState(TRUE);
    return TRUE;
}

BOOLEAN ToolStatusMenuBarShowFromHotItem(
    _In_ BOOLEAN FromKeyboard
    )
{
    if (TsMenuBarHotItemIndex < 0)
        return FALSE;

    TOOLSTATUS_MENUBAR_LOG(
        "ShowFromHotItem index=%ld fromKeyboard=%lu",
        TsMenuBarHotItemIndex,
        FromKeyboard
        );

    ToolStatusMenuBarTrackPopupMenu(
        TsMenuBarHotItemIndex,
        FromKeyboard
        );
    return TRUE;
}

LRESULT CALLBACK ToolStatusMenuBarHookProc(
    _In_ INT Code,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PMSG message;

    if (Code == MSGF_MENU && lParam)
    {
        LONG virtualKey;

        message = (PMSG)lParam;

        switch (message->message)
        {
        case WM_MENUSELECT:
            TsMenuBarSelectedMenuHandle = (HMENU)message->lParam;
            TsMenuBarSelectedMenuItemIndex = LOWORD(message->wParam);
            TsMenuBarSelectedMenuFlags = HIWORD(message->wParam);
            TOOLSTATUS_MENUBAR_LOG(
                "Hook WM_MENUSELECT menu=%p item=%ld flags=0x%lx",
                TsMenuBarSelectedMenuHandle,
                TsMenuBarSelectedMenuItemIndex,
                TsMenuBarSelectedMenuFlags
                );
            break;
        case WM_MOUSEMOVE:
            {
                POINT clientPoint;
                LONG itemIndex;

                clientPoint = message->pt;
                MapWindowPoints(HWND_DESKTOP, TsMenuBarWindowHandle, &clientPoint, 1);
                itemIndex = ToolStatusMenuBarHitTest(&clientPoint);

                if (
                    (TsMenuBarLastMouseClientPoint.x != clientPoint.x ||
                    TsMenuBarLastMouseClientPoint.y != clientPoint.y) &&
                    itemIndex >= 0 &&
                    itemIndex != TsMenuBarPressedItemIndex &&
                    itemIndex < ToolStatusMenuBarGetButtonCount()
                    )
                {
                    TsMenuBarLastMouseClientPoint = clientPoint;
                    TOOLSTATUS_MENUBAR_LOG("Hook WM_MOUSEMOVE item=%ld", itemIndex);
                    ToolStatusMenuBarChangeDropdown(itemIndex, FALSE);
                }
            }
            break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            virtualKey = (LONG)message->wParam;

            if (TsMenuBarRtlLayout)
            {
                if (virtualKey == VK_LEFT)
                    virtualKey = VK_RIGHT;
                else if (virtualKey == VK_RIGHT)
                    virtualKey = VK_LEFT;
            }

            switch (virtualKey)
            {
            case VK_MENU:
            case VK_F10:
                TOOLSTATUS_MENUBAR_LOG("Hook key cancel vk=%ld", virtualKey);
                ToolStatusMenuBarChangeDropdown(INT_ERROR, TRUE);
                return 1;
            case VK_LEFT:
                TOOLSTATUS_MENUBAR_LOG("Hook key left");
                ToolStatusMenuBarChangeDropdown(
                    ToolStatusMenuBarWrapButtonIndex(TsMenuBarPressedItemIndex - 1),
                    TRUE
                    );
                return 1;
                break;
            case VK_RIGHT:
                if (
                    !TsMenuBarSelectedMenuHandle ||
                    !(TsMenuBarSelectedMenuFlags & MF_POPUP) ||
                    (TsMenuBarSelectedMenuFlags & (MF_GRAYED | MF_DISABLED))
                    )
                {
                    TOOLSTATUS_MENUBAR_LOG("Hook key right");
                    ToolStatusMenuBarChangeDropdown(
                        ToolStatusMenuBarWrapButtonIndex(TsMenuBarPressedItemIndex + 1),
                        TRUE
                        );
                    return 1;
                }
                break;
            }
            break;
        }
    }

    return CallNextHookEx(TsMenuBarMessageHookHandle, Code, wParam, lParam);
}

BOOLEAN ToolStatusMenuBarInstallHook(
    VOID
    )
{
    HHOOK messageHookHandle;

    if (TsMenuBarMessageHookHandle)
    {
        TOOLSTATUS_MENUBAR_LOG("InstallHook already-installed=%p", TsMenuBarMessageHookHandle);
        return TRUE;
    }

    TOOLSTATUS_MENUBAR_LOG("InstallHook");

    messageHookHandle = SetWindowsHookEx(
        WH_MSGFILTER,
        ToolStatusMenuBarHookProc,
        NULL,
        HandleToUlong(NtCurrentThreadId())
        );

    if (messageHookHandle)
    {
        TsMenuBarMessageHookHandle = messageHookHandle;
        TOOLSTATUS_MENUBAR_LOG("InstallHook success hook=%p", messageHookHandle);
        return TRUE;
    }

    TOOLSTATUS_MENUBAR_LOG("InstallHook failed gle=%lu", GetLastError());
    return FALSE;
}

VOID ToolStatusMenuBarRemoveHook(
    VOID
    )
{
    if (TsMenuBarMessageHookHandle)
    {
        TOOLSTATUS_MENUBAR_LOG("RemoveHook hook=%p", TsMenuBarMessageHookHandle);
        UnhookWindowsHookEx(TsMenuBarMessageHookHandle);
        TsMenuBarMessageHookHandle = NULL;
    }

    TsMenuBarSelectedMenuHandle = NULL;
    TsMenuBarSelectedMenuItemIndex = INT_ERROR;
    TsMenuBarSelectedMenuFlags = 0;
}

HWND ToolStatusMenuBarCreateWindow(
    _In_ HWND ParentWindowHandle
    )
{
    HWND windowHandle;

    TOOLSTATUS_MENUBAR_LOG("CreateWindow parent=%p", ParentWindowHandle);

    windowHandle = PhCreateWindow(
        TOOLBARCLASSNAME,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NOPARENTALIGN | CCS_NODIVIDER |
        TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_AUTOSIZE,
        TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS,
        0,
        0,
        0,
        ParentWindowHandle,
        NULL,
        NULL,
        NULL
        );

    if (!windowHandle)
        return NULL;

    SendMessage(windowHandle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    //SendMessage(windowHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS);
    //SendMessage(windowHandle, TB_SETBITMAPSIZE, 0, MAKELONG(5, 5));
    SendMessage(windowHandle, TB_SETPADDING, 0, MAKELONG(20, 6));

    TsMenuBarWindowHandle = windowHandle;
    TsMenuBarParentWindowHandle = ParentWindowHandle;
    TsMenuBarOldFocusWindowHandle = NULL;
    TsMenuBarMessageHookHandle = NULL;
    TsMenuBarSelectedMenuHandle = NULL;
    TsMenuBarLastMouseClientPoint.x = 0;
    TsMenuBarLastMouseClientPoint.y = 0;
    TsMenuBarHotItemIndex = INT_ERROR;
    TsMenuBarPressedItemIndex = INT_ERROR;
    TsMenuBarSelectedMenuItemIndex = INT_ERROR;
    TsMenuBarSelectedMenuFlags = 0;
    TsMenuBarThemeHandle = NULL;
    TsMenuBarMenuActive = FALSE;
    TsMenuBarContinueHotTrack = FALSE;
    TsMenuBarSelectFromKeyboard = FALSE;
    TsMenuBarDelayedActivation = FALSE;
    TsMenuBarRtlLayout =
        !!(GetWindowLongPtr(windowHandle, GWL_EXSTYLE) & WS_EX_LAYOUTRTL);

    ToolStatusMenuBarUpdateUiState(FALSE);
    TOOLSTATUS_MENUBAR_LOG("CreateWindow success WindowHandle=%p", windowHandle);
    return windowHandle;
}

BOOLEAN ToolStatusMenuBarLoadMenu(
    _In_ HWND WindowHandle,
    _In_ HMENU MainMenuHandle
    )
{
    LONG menuCount;
    LONG i;

    if (!WindowHandle || !MainMenuHandle)
        return FALSE;

    TOOLSTATUS_MENUBAR_LOG("LoadMenu WindowHandle=%p menu=%p", WindowHandle, MainMenuHandle);

    TsMenuBarWindowHandle = WindowHandle;
    TsMenuBarParentWindowHandle = GetParent(WindowHandle);
    TsMenuBarRtlLayout =
        !!(GetWindowLongPtr(WindowHandle, GWL_EXSTYLE) & WS_EX_LAYOUTRTL);

    while (SendMessage(WindowHandle, TB_BUTTONCOUNT, 0, 0) > 0)
    {
        SendMessage(WindowHandle, TB_DELETEBUTTON, 0, 0);
    }

    {
        PPH_EMENU menu;

        menu = SystemInformer_GetMainMenu();

        if (!menu || !menu->Items)
        {
            if (menu)
                PhDestroyEMenuItem(menu);
            return FALSE;
        }

        menuCount = (LONG)menu->Items->Count;
        TOOLSTATUS_MENUBAR_LOG("LoadMenu count=%ld", menuCount);

        for (i = 0; i < menuCount; i++)
        {
            PPH_EMENU_ITEM menuItem;
            TBBUTTON button;

            menuItem = menu->Items->Items[i];

            memset(&button, 0, sizeof(TBBUTTON));
            button.iBitmap = I_IMAGENONE;
            button.idCommand = i;
            button.dwData = i;

            if (!FlagOn(menuItem->Flags, PH_EMENU_DISABLED))
                button.fsState |= TBSTATE_ENABLED;

            if (FlagOn(menuItem->Flags, PH_EMENU_SEPARATOR))
            {
                button.fsStyle = BTNS_SEP;
                button.iBitmap = 10;
                SendMessage(WindowHandle, TB_ADDBUTTONS, 1, (LPARAM)&button);
                TOOLSTATUS_MENUBAR_LOG("LoadMenu add separator index=%ld", i);
                continue;
            }

            if (!menuItem->Items || menuItem->Items->Count == 0)
            {
                TOOLSTATUS_MENUBAR_LOG("LoadMenu skip non-popup index=%ld", i);
                continue;
            }

            button.fsStyle = BTNS_DROPDOWN | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
            button.iString = (INT_PTR)menuItem->Text;
            SendMessage(WindowHandle, TB_ADDBUTTONS, 1, (LPARAM)&button);
            TOOLSTATUS_MENUBAR_LOG("LoadMenu add popup index=%ld text=%ls", i, menuItem->Text);
        }

        PhDestroyEMenuItem(menu);
    }

    ToolStatusMenuBarDeactivate(FALSE);
    TOOLSTATUS_MENUBAR_LOG("LoadMenu complete");
    return TRUE;
}

LRESULT CALLBACK ToolStatusMenuBarDrawToolbar(
    _In_ LPNMTBCUSTOMDRAW DrawInfo
    )
{
    switch (DrawInfo->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
    case CDDS_ITEMPREPAINT:
        {
            TBBUTTONINFO buttonInfo =
            {
                sizeof(TBBUTTONINFO),
                TBIF_STYLE | TBIF_COMMAND | TBIF_STATE | TBIF_IMAGE
            };

            SetBkMode(DrawInfo->nmcd.hdc, TRANSPARENT);

            LONG dpiValue;
            LONG bitmapWidth = 0;
            LONG bitmapHeight = 0;

            ULONG currentIndex = (ULONG)SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_COMMANDTOINDEX,
                DrawInfo->nmcd.dwItemSpec,
                0
            );
            BOOLEAN isHighlighted = SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_GETHOTITEM,
                0,
                0
            ) == currentIndex;
            BOOLEAN isMenuOpen =
                TsMenuBarMenuActive &&
                (LONG)currentIndex == TsMenuBarPressedItemIndex;
            BOOLEAN isEnabled = SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_ISBUTTONENABLED,
                DrawInfo->nmcd.dwItemSpec,
                0
            ) != 0;

            if (SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_GETBUTTONINFO,
                (ULONG)DrawInfo->nmcd.dwItemSpec,
                (LPARAM)&buttonInfo
                ) == INT_ERROR)
            {
                break;
            }

            BOOLEAN isDropDown = !!(buttonInfo.fsStyle & BTNS_WHOLEDROPDOWN);

            if (!TsMenuBarThemeHandle)
            {
                TsMenuBarThemeHandle = PhOpenThemeData(
                    DrawInfo->nmcd.hdr.hwndFrom,
                    L"Toolbar",
                    PhGetWindowDpi(DrawInfo->nmcd.hdr.hwndFrom)
                );
            }

            if (TsMenuBarThemeHandle)
            {
                LONG stateId;

                if (!isEnabled)
                    stateId = 4; // TS_DISABLED
                else if (isMenuOpen)
                    stateId = 3; // TS_PRESSED (menu open / selected)
                else if (isHighlighted)
                    stateId = 2; // TS_HOT
                else
                    stateId = 1; // TS_NORMAL

                PhDrawThemeBackground(
                    TsMenuBarThemeHandle,
                    DrawInfo->nmcd.hdc,
                    1, // TP_BUTTON
                    stateId,
                    &DrawInfo->nmcd.rc,
                    &DrawInfo->nmcd.rc
                );
            }
            else
            {
                if (isHighlighted || isMenuOpen)
                {
                    SetDCBrushColor(DrawInfo->nmcd.hdc, GetSysColor(COLOR_HOTLIGHT));
                    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhGetStockBrush(DC_BRUSH));
                }
                else
                {
                    SetDCBrushColor(DrawInfo->nmcd.hdc, GetSysColor(COLOR_WINDOW));
                    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhGetStockBrush(DC_BRUSH));
                }
            }

            dpiValue = PhGetWindowDpi(DrawInfo->nmcd.hdr.hwndFrom);

            SelectFont(DrawInfo->nmcd.hdc, GetWindowFont(DrawInfo->nmcd.hdr.hwndFrom));

            if (buttonInfo.iImage != I_IMAGECALLBACK)
            {
                HIMAGELIST toolbarImageList;

                if (toolbarImageList = (HIMAGELIST)SendMessage(
                    DrawInfo->nmcd.hdr.hwndFrom,
                    TB_GETIMAGELIST,
                    0,
                    0
                    ))
                {
                    //LONG x;
                    //LONG y;

                    //PhImageListGetIconSize(toolbarImageList, &bitmapWidth, &bitmapHeight);

                    //if (buttonInfo.fsStyle & BTNS_SHOWTEXT)
                    //{
                    //    DrawInfo->nmcd.rc.left += PhGetSystemMetrics(SM_CXEDGE, dpiValue); // PhScaleToDisplay(5, dpiValue);
                    //    x = DrawInfo->nmcd.rc.left;// + ((DrawInfo->nmcd.rc.right - DrawInfo->nmcd.rc.left) - PhSmallIconSize.X) / 2;
                    //    y = DrawInfo->nmcd.rc.top + ((DrawInfo->nmcd.rc.bottom - DrawInfo->nmcd.rc.top) - bitmapHeight) / 2;
                    //}
                    //else
                    //{
                    //    x = DrawInfo->nmcd.rc.left + ((DrawInfo->nmcd.rc.right - DrawInfo->nmcd.rc.left) - bitmapWidth) / 2 - (isDropDown * 4);
                    //    y = DrawInfo->nmcd.rc.top + ((DrawInfo->nmcd.rc.bottom - DrawInfo->nmcd.rc.top) - bitmapHeight) / 2;
                    //}

                    //PhImageListDrawIcon(
                    //    toolbarImageList,
                    //    buttonInfo.iImage,
                    //    DrawInfo->nmcd.hdc,
                    //    x,
                    //    y,
                    //    ILD_NORMAL,
                    //    !isEnabled
                    //);

    /*                if (isDropDown)
                    {
                        HDC hdc = DrawInfo->nmcd.hdc;
                        LPRECT rc = &DrawInfo->nmcd.rc;
                        int triangleLeft = rc->right - 11, triangleTop = (rc->bottom - rc->top) / 2 - 2;
                        POINT vertices[] = { {triangleLeft, triangleTop}, {triangleLeft + 6, triangleTop}, {triangleLeft + 3, triangleTop + 3} };
                        SetDCPenColor(hdc, RGB(0xDE, 0xDE, 0xDE));
                        SetDCBrushColor(hdc, RGB(0xDE, 0xDE, 0xDE));
                        SelectPen(hdc, PhGetStockPen(DC_PEN));
                        SelectBrush(hdc, PhGetStockBrush(DC_BRUSH));
                        Polygon(hdc, vertices, _countof(vertices));
                    }*/

                    //return CDRF_SKIPDEFAULT | CDRF_NOTIFYPOSTPAINT;
                }
            }
            else
            {
                return CDRF_DODEFAULT; // Required for I_IMAGECALLBACK (dmex)
            }

            if (buttonInfo.fsStyle & BTNS_SHOWTEXT)
            {
                RECT textRect = DrawInfo->nmcd.rc;
                WCHAR buttonText[MAX_PATH] = L"";

                SendMessage(
                    DrawInfo->nmcd.hdr.hwndFrom,
                    TB_GETBUTTONTEXT,
                    (ULONG)DrawInfo->nmcd.dwItemSpec,
                    (LPARAM)buttonText
                    );

                textRect.left += bitmapWidth - (isDropDown * 12); // PhScaleToDisplay(10, dpiValue);
                DrawText(
                    DrawInfo->nmcd.hdc,
                    buttonText,
                    (ULONG)PhCountStringZ(buttonText),
                    &textRect,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE |
                    ((DrawInfo->nmcd.uItemState & CDIS_SHOWKEYBOARDCUES) ? 0 : DT_HIDEPREFIX)
                    );
            }

            //DrawInfo->clrText = RGB(0x0, 0xff, 0);
            //return TBCDRF_USECDCOLORS | CDRF_NEWFONT;
        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

BOOLEAN ToolStatusMenuBarHandleNotify(
    _In_ LPNMHDR Header,
    _Out_opt_ LRESULT* Result
    )
{
    LPNMTOOLBAR toolbarHeader;
    LPNMTBHOTITEM hotItemHeader;

    if (Result)
        *Result = 0;

    if (!Header || Header->hwndFrom != TsMenuBarWindowHandle)
        return FALSE;

    switch (Header->code)
    {
    case TBN_DROPDOWN:
        toolbarHeader = (LPNMTOOLBAR)Header;
        TOOLSTATUS_MENUBAR_LOG("Notify TBN_DROPDOWN item=%ld", toolbarHeader->iItem);
        ToolStatusMenuBarQueueDropdown(toolbarHeader->iItem, FALSE);
        if (Result)
            *Result = TBDDRET_DEFAULT;
        return TRUE;
    case TBN_HOTITEMCHANGE:
        hotItemHeader = (LPNMTBHOTITEM)Header;
        TsMenuBarHotItemIndex =
            (hotItemHeader->dwFlags & HICF_LEAVING) ? INT_ERROR : hotItemHeader->idNew;
        TOOLSTATUS_MENUBAR_LOG(
            "Notify TBN_HOTITEMCHANGE old=%ld new=%ld flags=0x%lx",
            hotItemHeader->idOld,
            hotItemHeader->idNew,
            hotItemHeader->dwFlags
            );
        return FALSE;
    case NM_CUSTOMDRAW:
        {
            *Result = ToolStatusMenuBarDrawToolbar((LPNMTBCUSTOMDRAW)Header);
        }
        return TRUE;
    case TBN_GETDISPINFO:
        {
            LPNMTBDISPINFO toolbarDisplayInfo = (LPNMTBDISPINFO)Header;

            dprintf("");
        }
        return FALSE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN ToolStatusMenuBarHandleMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_opt_ LRESULT* Result
    )
{
    LONG buttonIndex;

    if (!TsMenuBarWindowHandle)
        return FALSE;

    if (Result)
        *Result = 0;

    switch (WindowMessage)
    {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        {
            LONG virtualKey;

            virtualKey = (LONG)wParam;
            TOOLSTATUS_MENUBAR_LOG("HandleMessage keydown msg=0x%lx vk=%ld lParam=0x%p", WindowMessage, virtualKey, (PVOID)lParam);

            if (
                virtualKey == VK_F10 ||
                virtualKey == VK_MENU
                )
            {
                if (!TsMenuBarMenuActive && GetFocus() == TsMenuBarWindowHandle)
                {
                    // Already keyboard-activated (no popup open): second Alt/F10 deactivates.
                    TOOLSTATUS_MENUBAR_LOG("HandleMessage toggle off vk=%ld", virtualKey);
                    ToolStatusMenuBarDeactivate(TRUE);
                    return TRUE;
                }

                if (
                    TsMenuBarMenuActive ||
                    TsMenuBarDelayedActivation ||
                    (virtualKey == VK_F10 && (GetKeyState(VK_SHIFT) & 0x8000)) ||
                    (virtualKey == VK_MENU && WindowMessage != WM_SYSKEYDOWN) ||
                    (HIWORD(lParam) & KF_REPEAT)
                    )
                {
                    break;
                }

                if (virtualKey == VK_MENU)
                    ToolStatusMenuBarUpdateUiState(TRUE);

                TsMenuBarDelayedActivation = TRUE;
                TOOLSTATUS_MENUBAR_LOG("HandleMessage delayed activation set vk=%ld", virtualKey);
                return TRUE;
            }

            if (TsMenuBarRtlLayout)
            {
                if (virtualKey == VK_LEFT)
                    virtualKey = VK_RIGHT;
                else if (virtualKey == VK_RIGHT)
                    virtualKey = VK_LEFT;
            }

            switch (virtualKey)
            {
            case VK_ESCAPE:
                if (TsMenuBarHotItemIndex >= 0)
                {
                    TOOLSTATUS_MENUBAR_LOG("HandleMessage escape deactivate");
                    ToolStatusMenuBarDeactivate(TRUE);
                    return TRUE;
                }
                break;
            case VK_LEFT:
                if (TsMenuBarHotItemIndex >= 0)
                    return ToolStatusMenuBarMoveHotItem(-1);
                break;
            case VK_RIGHT:
                if (TsMenuBarHotItemIndex >= 0)
                    return ToolStatusMenuBarMoveHotItem(1);
                break;
            case VK_DOWN:
            case VK_UP:
            case VK_RETURN:
                if (TsMenuBarHotItemIndex >= 0)
                {
                    ToolStatusMenuBarShowFromHotItem(TRUE);
                    return TRUE;
                }
                break;
            }
        }
        break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        TOOLSTATUS_MENUBAR_LOG("HandleMessage keyup msg=0x%lx vk=%ld lParam=0x%p", WindowMessage, (LONG)wParam, (PVOID)lParam);
        if (wParam == VK_F10 || wParam == VK_MENU)
        {
            if (!TsMenuBarDelayedActivation)
            {
                ToolStatusMenuBarUpdateUiState(FALSE);
                break;
            }

            if (wParam == VK_MENU && WindowMessage != WM_SYSKEYUP)
            {
                ToolStatusMenuBarUpdateUiState(FALSE);
                TsMenuBarDelayedActivation = FALSE;
                break;
            }

            TsMenuBarDelayedActivation = FALSE;
            TsMenuBarOldFocusWindowHandle = GetFocus();
            SetFocus(TsMenuBarWindowHandle);
            ToolStatusMenuBarSetHotItem(0);
            ToolStatusMenuBarUpdateUiState(TRUE);
            TOOLSTATUS_MENUBAR_LOG("HandleMessage activate menubar from keyup");
            return TRUE;
        }
        break;
    case WM_SYSCHAR:
        TOOLSTATUS_MENUBAR_LOG("HandleMessage syschar ch=%lc lParam=0x%p", (WCHAR)wParam, (PVOID)lParam);
        if (wParam != VK_MENU && (lParam & 0x20000000))
        {
            buttonIndex = ToolStatusMenuBarMapAccelerator((WCHAR)wParam);

            if (buttonIndex >= 0)
            {
                TsMenuBarDelayedActivation = FALSE;
                TsMenuBarOldFocusWindowHandle = GetFocus();
                SetFocus(TsMenuBarWindowHandle);
                ToolStatusMenuBarSetHotItem(buttonIndex);
                ToolStatusMenuBarUpdateUiState(TRUE);
                ToolStatusMenuBarTrackPopupMenu(buttonIndex, TRUE);
                TOOLSTATUS_MENUBAR_LOG("HandleMessage syschar activate index=%ld", buttonIndex);

                if (Result)
                    *Result = 0;

                return TRUE;
            }

            TsMenuBarDelayedActivation = FALSE;
        }
        break;
    case WM_KILLFOCUS:
        TOOLSTATUS_MENUBAR_LOG("HandleMessage killfocus newFocus=%p active=%lu", (HWND)wParam, TsMenuBarMenuActive);
        if ((HWND)wParam != TsMenuBarWindowHandle && !TsMenuBarMenuActive)
        {
            TsMenuBarOldFocusWindowHandle = NULL;
            ToolStatusMenuBarDeactivate(FALSE);
        }
        break;
    case TOOLSTATUS_MENUBAR_SHOWPOPUP:
        if (TsMenuBarPressedItemIndex >= 0)
        {
            TOOLSTATUS_MENUBAR_LOG(
                "HandleMessage showpopup item=%ld fromKeyboard=%lu",
                TsMenuBarPressedItemIndex,
                TsMenuBarSelectFromKeyboard
                );
            ToolStatusMenuBarTrackPopupMenu(
                TsMenuBarPressedItemIndex,
                TsMenuBarSelectFromKeyboard
                );
            return TRUE;
        }
        break;
    case WM_EXITMENULOOP:
        TOOLSTATUS_MENUBAR_LOG("HandleMessage exitmenuloop pressed=%ld", TsMenuBarPressedItemIndex);
        if (!TsMenuBarMenuActive)
        {
            ToolStatusMenuBarRemoveHook();
            if (TsMenuBarPressedItemIndex < 0)
                ToolStatusMenuBarDeactivate(TRUE);
        }
        return FALSE;
    case WM_STYLECHANGED:
        if (wParam == GWL_EXSTYLE)
        {
            STYLESTRUCT* style = (STYLESTRUCT*)lParam;
            TsMenuBarRtlLayout = !!(style->styleNew & WS_EX_LAYOUTRTL);
            TOOLSTATUS_MENUBAR_LOG("HandleMessage stylechanged rtl=%lu", TsMenuBarRtlLayout);
        }
        break;
    case WM_THEMECHANGED:
        if (TsMenuBarThemeHandle)
        {
            PhCloseThemeData(TsMenuBarThemeHandle);
            TsMenuBarThemeHandle = NULL;
        }
        break;
    case WM_DESTROY:
        TOOLSTATUS_MENUBAR_LOG("HandleMessage destroy");
        ToolStatusMenuBarRemoveHook();

        if (TsMenuBarThemeHandle)
        {
            PhCloseThemeData(TsMenuBarThemeHandle);
            TsMenuBarThemeHandle = NULL;
        }

        TsMenuBarWindowHandle = NULL;
        TsMenuBarParentWindowHandle = NULL;
        TsMenuBarOldFocusWindowHandle = NULL;
        TsMenuBarMessageHookHandle = NULL;
        TsMenuBarSelectedMenuHandle = NULL;
        TsMenuBarLastMouseClientPoint.x = 0;
        TsMenuBarLastMouseClientPoint.y = 0;
        TsMenuBarHotItemIndex = 0;
        TsMenuBarPressedItemIndex = 0;
        TsMenuBarSelectedMenuItemIndex = 0;
        TsMenuBarSelectedMenuFlags = 0;
        TsMenuBarThemeHandle = NULL;
        TsMenuBarMenuActive = FALSE;
        TsMenuBarContinueHotTrack = FALSE;
        TsMenuBarSelectFromKeyboard = FALSE;
        TsMenuBarDelayedActivation = FALSE;
        TsMenuBarRtlLayout = FALSE;
        break;
    }

    return FALSE;
}
