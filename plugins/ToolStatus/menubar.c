/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2026
 *
 */

#include "toolstatus.h"
#include <vsstyle.h>
#include <uxtheme.h>

HWND TsMenuBarWindowHandle = NULL;
HWND TsMenuBarParentWindowHandle = NULL;
HWND TsMenuBarOldFocusWindowHandle = NULL;
HHOOK TsMenuBarMessageHookHandle = NULL;
HMENU TsMenuBarSelectedMenuHandle = NULL;
POINT TsMenuBarLastMouseClientPoint = { 0 };
LONG TsMenuBarHotItemIndex = INT_ERROR;
BOOLEAN TsMenuBarShowAccelerators = FALSE;
LONG TsMenuBarPressedItemIndex = INT_ERROR;
LONG TsMenuBarSelectedMenuItemIndex = INT_ERROR;
ULONG TsMenuBarSelectedMenuFlags = 0;
BOOLEAN TsMenuBarMenuActive = FALSE;
BOOLEAN TsMenuBarContinueHotTrack = FALSE;
BOOLEAN TsMenuBarSelectFromKeyboard = FALSE;
BOOLEAN TsMenuBarDelayedActivation = FALSE;
BOOLEAN TsMenuBarRtlLayout = FALSE;

#define TSMENU_DPRINTF(format, ...) dprintf("[ToolStatus.MenuBar] " format "\n" __VA_OPT__(,) __VA_ARGS__)

BOOLEAN ToolStatusMenuBarInstallHook(
    VOID
    );

VOID ToolStatusMenuBarRemoveHook(
    VOID
    );

/**
 * Resets the tracking state of the menu, clearing hot items, pressed items, and menu flags.
 */
VOID ToolStatusMenuBarResetTrackingState(
    VOID
    )
{
    TSMENU_DPRINTF("ResetTrackingState");

    TsMenuBarHotItemIndex = INT_ERROR;
    TsMenuBarPressedItemIndex = INT_ERROR;
    TsMenuBarSelectedMenuHandle = NULL;
    TsMenuBarSelectedMenuItemIndex = INT_ERROR;
    TsMenuBarSelectedMenuFlags = 0;
    TsMenuBarMenuActive = FALSE;
    TsMenuBarContinueHotTrack = FALSE;
    TsMenuBarSelectFromKeyboard = FALSE;
}

/**
 * Updates the UI state of the menu based on keyboard activity, such as hiding or showing accelerators.
 *
 * \param[in] KeyboardActivity A boolean value indicating whether the update was triggered by keyboard activity.
 */
VOID ToolStatusMenuBarUpdateUiState(
    _In_ BOOLEAN KeyboardActivity
    )
{
    WORD action;
    BOOL showAcceleratorsAlways;

    if (!TsMenuBarWindowHandle)
        return;

    TSMENU_DPRINTF("UpdateUiState keyboard=%lu", KeyboardActivity);

    if (KeyboardActivity)
    {
        action = UIS_CLEAR;
    }
    else
    {
        // SystemParametersInfoForDpi only supports the font/metrics actions and
        // fails for SPI_GETMENUUNDERLINES, so query without a DPI. (dmex)

        if (!PhGetSystemParametersInfo(SPI_GETMENUUNDERLINES, 0, &showAcceleratorsAlways, 0))
        {
            showAcceleratorsAlways = FALSE;
        }

        action = showAcceleratorsAlways ? UIS_CLEAR : UIS_SET;
    }

    // Track the accelerator state ourselves instead of relying on WM_CHANGEUISTATE.
    // The message is forwarded to the top-level window, which only broadcasts
    // WM_UPDATEUISTATE back to its children when its own state changes, so the
    // toolbar state can get stuck showing underlines. The items are custom drawn,
    // so the flag is all the draw code needs. (dmex)

    if (TsMenuBarShowAccelerators != (action == UIS_CLEAR))
    {
        TsMenuBarShowAccelerators = (action == UIS_CLEAR);
        InvalidateRect(TsMenuBarWindowHandle, NULL, FALSE);
    }
}

/**
 * Deactivates the menu, clearing the hot item and optionally restoring focus to the previous window.
 *
 * \param[in] RestoreFocus A boolean value indicating whether to restore focus to the previously focused window.
 */
VOID ToolStatusMenuBarDeactivate(
    _In_ BOOLEAN RestoreFocus
    )
{
    if (!TsMenuBarWindowHandle)
        return;

    TSMENU_DPRINTF("Deactivate restoreFocus=%lu oldFocus=%p", RestoreFocus, TsMenuBarOldFocusWindowHandle);

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

/**
 * Sets the hot item index for the menu and updates the toolbar control.
 *
 * \param[in] Index The index of the item to set as hot.
 */
VOID ToolStatusMenuBarSetHotItem(
    _In_ LONG Index
    )
{
    if (Index < 0)
    {
        Index = INT_ERROR;
    }

    TSMENU_DPRINTF("SetHotItem index=%ld", Index);

    TsMenuBarHotItemIndex = Index;

    if (TsMenuBarWindowHandle)
    {
        SendMessage(TsMenuBarWindowHandle, TB_SETHOTITEM, Index, 0);
    }
}

/**
 * Retrieves the number of buttons currently in the menu.
 *
 * \return The number of buttons in the menu bar.
 */
LONG ToolStatusMenuBarGetButtonCount(
    VOID
    )
{
    if (TsMenuBarWindowHandle)
    {
        return (LONG)SendMessage(TsMenuBarWindowHandle, TB_BUTTONCOUNT, 0, 0);
    }

    return 0;
}

/**
 * Wraps the specified button index around the ends of the menu button array.
 *
 * \param[in] Index The index to wrap.
 * \return The wrapped index.
 */
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

/**
 * Retrieves the bounding rectangle for the specified button in the menu.
 *
 * \param[in] Index The index of the button.
 * \param[out] Rect A pointer to a RECT structure that receives the bounding rectangle.
 * \return TRUE if successful, otherwise FALSE.
 */
BOOLEAN ToolStatusMenuBarGetButtonRect(
    _In_ LONG Index,
    _Out_ PRECT Rect
    )
{
    if (TsMenuBarWindowHandle)
    {
        if (SendMessage(TsMenuBarWindowHandle, TB_GETITEMRECT, Index, (LPARAM)Rect))
        {
            MapWindowRect(TsMenuBarWindowHandle, HWND_DESKTOP, Rect);
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Performs a hit test on the menu bar to determine the item at the specified client coordinates.
 *
 * \param[in] ClientPoint A pointer to a POINT structure containing the client coordinates to test.
 * \return The index of the item at the specified coordinates, or INT_ERROR if no item is at the coordinates.
 */
LONG ToolStatusMenuBarHitTest(
    _In_ PPOINT ClientPoint
    )
{
    return (LONG)SendMessage(TsMenuBarWindowHandle, TB_HITTEST, 0, (LPARAM)ClientPoint);
}

/**
 * Resets the hot item based on the current mouse cursor position.
 */
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
    {
        itemIndex = INT_ERROR;
    }

    ToolStatusMenuBarSetHotItem(itemIndex);
}

/**
 * Maps an accelerator character to the corresponding menu bar button index.
 *
 * \param[in] Character The accelerator character to map.
 * \return The index of the button corresponding to the accelerator, or INT_ERROR if not found.
 */
LONG ToolStatusMenuBarMapAccelerator(
    _In_ WCHAR Character
    )
{
    if (TsMenuBarWindowHandle)
    {
        LONG index = INT_ERROR;

        if (SendMessage(TsMenuBarWindowHandle, TB_MAPACCELERATOR, PhDowncaseUnicodeChar(Character), (LPARAM)&index))
        {
            TSMENU_DPRINTF("MapAccelerator char=%lc index=%ld", Character, index);
            return index;
        }
    }

    TSMENU_DPRINTF("MapAccelerator char=%lc index=none", Character);

    return INT_ERROR;
}

/**
 * Changes the active dropdown menu to the specified item index.
 *
 * \param[in] ItemIndex The index of the item to dropdown.
 * \param[in] FromKeyboard A boolean value indicating whether the change was triggered by the keyboard.
 */
VOID ToolStatusMenuBarChangeDropdown(
    _In_ LONG ItemIndex,
    _In_ BOOLEAN FromKeyboard
    )
{
    TSMENU_DPRINTF("ChangeDropdown item=%ld fromKeyboard=%lu", ItemIndex, FromKeyboard);

    TsMenuBarPressedItemIndex = ItemIndex;
    TsMenuBarSelectFromKeyboard = FromKeyboard;
    TsMenuBarContinueHotTrack = TRUE;
    SendMessage(TsMenuBarWindowHandle, WM_CANCELMODE, 0, 0);
}

/**
 * Queues a dropdown menu activation for the specified item index.
 *
 * \param[in] ItemIndex The index of the item to dropdown.
 * \param[in] FromKeyboard A boolean value indicating whether the dropdown was triggered by the keyboard.
 */
VOID ToolStatusMenuBarQueueDropdown(
    _In_ LONG ItemIndex,
    _In_ BOOLEAN FromKeyboard
    )
{
    TSMENU_DPRINTF("QueueDropdown item=%ld fromKeyboard=%lu", ItemIndex, FromKeyboard);

    TsMenuBarPressedItemIndex = ItemIndex;
    TsMenuBarSelectFromKeyboard = FromKeyboard;
    PostMessage(TsMenuBarWindowHandle, TB_CUSTOMIZE, 0, 0);
}

/**
 * Tracks a popup menu for the specified button index, handling the menu loop and selection.
 *
 * \param[in] ButtonIndex The index of the button for which to track the popup menu.
 * \param[in] FromKeyboard A boolean value indicating whether the menu is being tracked from the keyboard.
 * \return TRUE if a command was handled, otherwise FALSE.
 */
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

    TSMENU_DPRINTF("TrackPopupMenu begin button=%ld fromKeyboard=%lu", ButtonIndex, FromKeyboard);

    TsMenuBarOldFocusWindowHandle = GetFocus();
    TsMenuBarPressedItemIndex = ButtonIndex;
    TsMenuBarSelectFromKeyboard = FromKeyboard;
    TsMenuBarMenuActive = TRUE;

    if (!ToolStatusMenuBarInstallHook())
    {
        TsMenuBarMenuActive = FALSE;
        TSMENU_DPRINTF("TrackPopupMenu abort hook install failed");
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
        ULONG buttonState;
        //TPMPARAMS trackParams;
        //ULONG popupFlags;
        //ULONG popupResult;
        LONG itemIndex;

        itemIndex = TsMenuBarPressedItemIndex;
        TsMenuBarSelectFromKeyboard = FromKeyboard;
        TsMenuBarContinueHotTrack = FALSE;

        TSMENU_DPRINTF("TrackPopupMenu loop item=%ld", itemIndex);

        menu = SystemInformer_GetMainSubMenu(itemIndex);

        if (!menu || !ToolStatusMenuBarGetButtonRect(itemIndex, &buttonRect))
        {
            TSMENU_DPRINTF("TrackPopupMenu missing submenu/rect item=%ld menu=%p", itemIndex, menu);
            if (menu)
                PhDestroyEMenuItem(menu);
            break;
        }

        PhInitializeEMenuData(&menuData);
        popupMenu = NULL;
        selectedItem = NULL;

        ToolStatusMenuBarSetHotItem(itemIndex);

        buttonState = (ULONG)SendMessage(TsMenuBarWindowHandle, TB_GETSTATE, itemIndex, 0);
        SendMessage(TsMenuBarWindowHandle, TB_SETSTATE, itemIndex, MAKELONG(buttonState | TBSTATE_PRESSED, 0));

        if (TsMenuBarSelectFromKeyboard)
        {
            keybd_event(VK_DOWN, 0, 0, 0);
            keybd_event(VK_DOWN, 0, KEYEVENTF_KEYUP, 0);
            TsMenuBarSelectFromKeyboard = FALSE;
        }

        if (MainWindowHandle)
        {
            TSMENU_DPRINTF("TrackPopupMenu foreground window=%p", MainWindowHandle);
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

        TSMENU_DPRINTF(
            "TrackPopupMenu result item=%ld selected=%p id=%lu continue=%lu pressed=%ld",
            itemIndex,
            selectedItem,
            selectedItem ? selectedItem->Id : 0,
            TsMenuBarContinueHotTrack,
            TsMenuBarPressedItemIndex
            );

        if (MainWindowHandle)
        {
            PostMessage(MainWindowHandle, WM_NULL, 0, 0);
        }

        SendMessage(TsMenuBarWindowHandle, TB_SETSTATE, itemIndex, MAKELONG(buttonState, 0));

        if (selectedItem)
        {
            selectedMenu = menu;
            selectedMenuItem = selectedItem;
            commandHandled = TRUE;
        }

        PhDeleteEMenuData(&menuData);

        if (!selectedItem)
        {
            PhDestroyEMenuItem(menu);
        }

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
        TSMENU_DPRINTF("TrackPopupMenu dispatch id=%lu context=%p", selectedMenuItem->Id, selectedMenuItem);
        SystemInformer_SetMainSubCmd(selectedMenuItem->Id, selectedMenuItem);
    }

    if (selectedMenu)
    {
        PhDestroyEMenuItem(selectedMenu);
    }

    TSMENU_DPRINTF("TrackPopupMenu end handled=%lu", commandHandled);

    return commandHandled;
}

/**
 * Moves the hot item in the specified direction, wrapping around if necessary.
 *
 * \param[in] Direction The direction to move the hot item (-1 for left, 1 for right).
 * \return TRUE if the hot item was moved, otherwise FALSE.
 */
BOOLEAN ToolStatusMenuBarMoveHotItem(
    _In_ LONG Direction
    )
{
    LONG nextIndex;

    nextIndex = ToolStatusMenuBarWrapButtonIndex(TsMenuBarHotItemIndex + Direction);

    TSMENU_DPRINTF("MoveHotItem direction=%ld current=%ld next=%ld", Direction, TsMenuBarHotItemIndex, nextIndex);

    if (nextIndex < 0)
        return FALSE;

    ToolStatusMenuBarSetHotItem(nextIndex);
    ToolStatusMenuBarUpdateUiState(TRUE);
    return TRUE;
}

/**
 * Shows the dropdown menu for the currently hot item.
 *
 * \param[in] FromKeyboard A boolean value indicating whether the menu is being shown from the keyboard.
 * \return TRUE if the menu was shown, otherwise FALSE.
 */
BOOLEAN ToolStatusMenuBarShowFromHotItem(
    _In_ BOOLEAN FromKeyboard
    )
{
    if (TsMenuBarHotItemIndex < 0)
        return FALSE;

    TSMENU_DPRINTF("ShowFromHotItem index=%ld fromKeyboard=%lu", TsMenuBarHotItemIndex, FromKeyboard);

    ToolStatusMenuBarTrackPopupMenu(TsMenuBarHotItemIndex, FromKeyboard);

    return TRUE;
}

/**
 * The message hook procedure used to monitor messages for the menu bar while a menu is active.
 *
 * \param[in] Code The hook code.
 * \param[in] wParam The message parameter.
 * \param[in] lParam The message parameter.
 * \return The result of CallNextHookEx.
 */
LRESULT CALLBACK ToolStatusMenuBarHookProc(
    _In_ INT Code,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    if (Code == MSGF_MENU && lParam)
    {
        PMSG message = (PMSG)lParam;
        LONG virtualKey;

        switch (message->message)
        {
        case WM_MENUSELECT:
            {
                TsMenuBarSelectedMenuHandle = (HMENU)message->lParam;
                TsMenuBarSelectedMenuItemIndex = LOWORD(message->wParam);
                TsMenuBarSelectedMenuFlags = HIWORD(message->wParam);

                TSMENU_DPRINTF(
                    "Hook WM_MENUSELECT menu=%p item=%ld flags=0x%lx",
                    TsMenuBarSelectedMenuHandle,
                    TsMenuBarSelectedMenuItemIndex,
                    TsMenuBarSelectedMenuFlags
                    );
            }
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
                    TSMENU_DPRINTF("Hook WM_MOUSEMOVE item=%ld", itemIndex);
                    ToolStatusMenuBarChangeDropdown(itemIndex, FALSE);
                }
            }
            break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            {
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
                    {
                        TSMENU_DPRINTF("Hook key cancel vk=%ld", virtualKey);

                        ToolStatusMenuBarChangeDropdown(INT_ERROR, TRUE);
                    }
                    return 1;
                case VK_LEFT:
                    {
                        TSMENU_DPRINTF("Hook key left");

                        ToolStatusMenuBarChangeDropdown(ToolStatusMenuBarWrapButtonIndex(TsMenuBarPressedItemIndex - 1), TRUE);
                    }
                    return 1;
                case VK_RIGHT:
                    {
                        if (
                            !TsMenuBarSelectedMenuHandle ||
                            !(TsMenuBarSelectedMenuFlags & MF_POPUP) ||
                            (TsMenuBarSelectedMenuFlags & (MF_GRAYED | MF_DISABLED))
                            )
                        {
                            TSMENU_DPRINTF("Hook key right");

                            ToolStatusMenuBarChangeDropdown(ToolStatusMenuBarWrapButtonIndex(TsMenuBarPressedItemIndex + 1), TRUE);

                            return 1;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    return CallNextHookEx(TsMenuBarMessageHookHandle, Code, wParam, lParam);
}

/**
 * Installs the message filter hook used by the menu bar during menu tracking.
 *
 * \return TRUE if the hook was successfully installed, otherwise FALSE.
 */
BOOLEAN ToolStatusMenuBarInstallHook(
    VOID
    )
{
    HHOOK messageHookHandle;

    if (TsMenuBarMessageHookHandle)
    {
        TSMENU_DPRINTF("InstallHook already-installed=%p", TsMenuBarMessageHookHandle);
        return TRUE;
    }

    TSMENU_DPRINTF("InstallHook");

    messageHookHandle = SetWindowsHookEx(
        WH_MSGFILTER,
        ToolStatusMenuBarHookProc,
        NULL,
        HandleToUlong(NtCurrentThreadId())
        );

    if (messageHookHandle)
    {
        TsMenuBarMessageHookHandle = messageHookHandle;
        TSMENU_DPRINTF("InstallHook success hook=%p", messageHookHandle);
        return TRUE;
    }

    TSMENU_DPRINTF("InstallHook failed gle=%lu", GetLastError());
    return FALSE;
}

/**
 * Removes the message filter hook used by the menu bar.
 */
VOID ToolStatusMenuBarRemoveHook(
    VOID
    )
{
    TSMENU_DPRINTF("RemoveHook hook=%p", TsMenuBarMessageHookHandle);

    if (TsMenuBarMessageHookHandle)
    {
        UnhookWindowsHookEx(TsMenuBarMessageHookHandle);
        TsMenuBarMessageHookHandle = NULL;
    }

    TsMenuBarSelectedMenuHandle = NULL;
    TsMenuBarSelectedMenuItemIndex = INT_ERROR;
    TsMenuBarSelectedMenuFlags = 0;
}

/**
 * Creates the toolbar control window used for the menu bar.
 *
 * \param[in] ParentWindowHandle The handle to the parent window.
 * \return The handle to the created menu bar window, or NULL if creation failed.
 */
HWND ToolStatusMenuBarCreateWindow(
    _In_ HWND ParentWindowHandle
    )
{
    HWND windowHandle;

    windowHandle = PhCreateWindowEx(
        TOOLBARCLASSNAME,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER |
        TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_AUTOSIZE,
        WS_EX_TOOLWINDOW,
        0,
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
    SendMessage(windowHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
    // Text-only; don't reserve the (DPI-scaled) default bitmap space.
    SendMessage(windowHandle, TB_SETBITMAPSIZE, 0, MAKELONG(0, 0));
    // No vertical padding; the button height is set from the font height
    // (ToolStatusGetWindowFontSize) and extra padding only adds row height.
    // The menu bar is recreated on DPI changes, so scale for the current DPI.
    SendMessage(windowHandle, TB_SETPADDING, 0, MAKELONG(PhScaleToDisplay(20, PhGetWindowDpi(ParentWindowHandle)), 0));

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
    TsMenuBarMenuActive = FALSE;
    TsMenuBarContinueHotTrack = FALSE;
    TsMenuBarSelectFromKeyboard = FALSE;
    TsMenuBarDelayedActivation = FALSE;
    TsMenuBarRtlLayout = !!(PhGetWindowStyleEx(windowHandle) & WS_EX_LAYOUTRTL);

    ToolStatusMenuBarUpdateUiState(FALSE);

    TSMENU_DPRINTF("CreateWindow success WindowHandle=%p", windowHandle);

    return windowHandle;
}

/**
 * Loads the main menu items into the menu bar toolbar control.
 *
 * \param[in] WindowHandle The handle to the menu bar window.
 * \param[in] MainMenuHandle The handle to the main menu.
 * \return TRUE if the menu was successfully loaded, otherwise FALSE.
 */
BOOLEAN ToolStatusMenuBarLoadMenu(
    _In_ HWND WindowHandle,
    _In_ HMENU MainMenuHandle
    )
{
    LONG menuCount;
    LONG i;

    if (!WindowHandle || !MainMenuHandle)
        return FALSE;

    TSMENU_DPRINTF("LoadMenu WindowHandle=%p menu=%p", WindowHandle, MainMenuHandle);

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
        TSMENU_DPRINTF("LoadMenu count=%ld", menuCount);

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
            {
                SetFlag(button.fsState, TBSTATE_ENABLED);
            }

            if (FlagOn(menuItem->Flags, PH_EMENU_SEPARATOR))
            {
                button.fsStyle = BTNS_SEP;
                button.iBitmap = PhScaleToDisplay(10, PhGetWindowDpi(WindowHandle));
                SendMessage(WindowHandle, TB_ADDBUTTONS, 1, (LPARAM)&button);
                TSMENU_DPRINTF("LoadMenu add separator index=%ld", i);
                continue;
            }

            if (!menuItem->Items || menuItem->Items->Count == 0)
            {
                TSMENU_DPRINTF("LoadMenu skip non-popup index=%ld", i);
                continue;
            }

            button.fsStyle = BTNS_DROPDOWN | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
            button.iString = SendMessage(WindowHandle, TB_ADDSTRING, 0, (LPARAM)menuItem->Text);
            SendMessage(WindowHandle, TB_ADDBUTTONS, 1, (LPARAM)&button);

            TSMENU_DPRINTF("LoadMenu add popup index=%ld text=%ls", i, menuItem->Text);
        }

        PhDestroyEMenuItem(menu);
    }

    ToolStatusMenuBarDeactivate(FALSE);
    TSMENU_DPRINTF("LoadMenu complete");
    return TRUE;
}

/**
 * Handles custom drawing for the menu bar toolbar control.
 *
 * \param[in] DrawInfo A pointer to an NMTBCUSTOMDRAW structure containing drawing information.
 * \return A value indicating how the custom drawing should proceed.
 */
LRESULT CALLBACK ToolStatusMenuBarDrawToolbar(
    _In_ LPNMTBCUSTOMDRAW DrawInfo
    )
{
    switch (DrawInfo->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            // Match the main toolbar background, including padding and space between buttons.
            ToolbarDrawRebarBackground(
                DrawInfo->nmcd.hdr.hwndFrom,
                DrawInfo->nmcd.hdc,
                &DrawInfo->nmcd.rc
                );
        }
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
        {
            TBBUTTONINFO buttonInfo = { sizeof(TBBUTTONINFO), TBIF_STYLE | TBIF_STATE };
            HDC hdc = DrawInfo->nmcd.hdc;
            HWND toolBarHandle = DrawInfo->nmcd.hdr.hwndFrom;
            RECT textRect = DrawInfo->nmcd.rc;
            LONG currentIndex;
            BOOLEAN isHighlighted;
            BOOLEAN isMenuOpen;
            BOOLEAN isEnabled;
            COLORREF textColor;
            COLORREF oldTextColor;
            INT oldBkMode;
            HFONT oldFont;
            ULONG textFlags;
            LONG_PTR textLength;
            PWSTR buttonText;

            if (SendMessage(toolBarHandle, TB_GETBUTTONINFO, DrawInfo->nmcd.dwItemSpec, (LPARAM)&buttonInfo) == INT_ERROR)
                return CDRF_DODEFAULT;

            if (FlagOn(buttonInfo.fsStyle, BTNS_SEP))
                return CDRF_SKIPDEFAULT;

            currentIndex = (LONG)SendMessage(toolBarHandle, TB_COMMANDTOINDEX, DrawInfo->nmcd.dwItemSpec, 0);
            isEnabled = !!FlagOn(buttonInfo.fsState, TBSTATE_ENABLED);
            isHighlighted = isEnabled && SendMessage(toolBarHandle, TB_GETHOTITEM, 0, 0) == currentIndex;
            isMenuOpen = isEnabled && (
                (TsMenuBarMenuActive && currentIndex == TsMenuBarPressedItemIndex) ||
                FlagOn(buttonInfo.fsState, TBSTATE_PRESSED)
                );

            // Use the same command bar renderer as the main toolbar. Normal buttons
            // keep the command bar background from CDDS_PREPAINT.
            if (isMenuOpen || isHighlighted)
            {
                ToolbarDrawItemBackground(toolBarHandle, hdc, &DrawInfo->nmcd.rc, isMenuOpen);
            }

            textColor = ToolbarGetItemTextColor(isEnabled);

            oldTextColor = SetTextColor(hdc, textColor);
            oldBkMode = SetBkMode(hdc, TRANSPARENT);
            oldFont = SelectFont(hdc, GetWindowFont(toolBarHandle));

            textFlags = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
            if (!TsMenuBarShowAccelerators)
                textFlags |= DT_HIDEPREFIX;
            if (TsMenuBarRtlLayout)
                textFlags |= DT_RTLREADING;

            // TB_GETBUTTONTEXT has no buffer-size parameter. Query the length
            // instead of assuming plugin-provided menu labels fit MAX_PATH.
            textLength = SendMessage(toolBarHandle, TB_GETBUTTONTEXT, DrawInfo->nmcd.dwItemSpec, 0);
            if (FlagOn(buttonInfo.fsStyle, BTNS_SHOWTEXT) && textLength > 0)
            {
                buttonText = PhAllocate(((SIZE_T)textLength + 1) * sizeof(WCHAR));
                if (SendMessage(toolBarHandle, TB_GETBUTTONTEXT, DrawInfo->nmcd.dwItemSpec, (LPARAM)buttonText) != INT_ERROR)
                {
                    DrawText(hdc, buttonText, (INT)textLength, &textRect, textFlags);
                }
                PhFree(buttonText);
            }

            SelectFont(hdc, oldFont);
            SetBkMode(hdc, oldBkMode);
            SetTextColor(hdc, oldTextColor);
        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

/**
 * Handles notification messages from the menu bar toolbar control.
 *
 * \param[in] Header A pointer to an NMHDR structure containing notification information.
 * \param[out] Result A pointer to a variable that receives the result of the message processing.
 * \return TRUE if the notification was handled, otherwise FALSE.
 */
BOOLEAN ToolStatusMenuBarHandleNotify(
    _In_ LPNMHDR Header,
    _Out_opt_ LRESULT* Result
    )
{
    LPNMTOOLBAR toolbarHeader;
    LPNMTBHOTITEM hotItemHeader;

    if (Result)
    {
        *Result = 0;
    }

    if (!Header || Header->hwndFrom != TsMenuBarWindowHandle)
        return FALSE;

    switch (Header->code)
    {
    case TBN_DROPDOWN:
        {
            toolbarHeader = (LPNMTOOLBAR)Header;

            TSMENU_DPRINTF("Notify TBN_DROPDOWN item=%ld", toolbarHeader->iItem);

            ToolStatusMenuBarQueueDropdown(toolbarHeader->iItem, FALSE);

            if (Result)
            {
                *Result = TBDDRET_DEFAULT;
            }
        }
        return TRUE;
    case TBN_HOTITEMCHANGE:
        {
            hotItemHeader = (LPNMTBHOTITEM)Header;

            TsMenuBarHotItemIndex = (hotItemHeader->dwFlags & HICF_LEAVING) ? INT_ERROR : hotItemHeader->idNew;

            TSMENU_DPRINTF(
                "Notify TBN_HOTITEMCHANGE old=%ld new=%ld flags=0x%lx",
                hotItemHeader->idOld,
                hotItemHeader->idNew,
                hotItemHeader->dwFlags
                );
        }
        return FALSE;
    case NM_CUSTOMDRAW:
        {
            *Result = ToolStatusMenuBarDrawToolbar((LPNMTBCUSTOMDRAW)Header);
        }
        return TRUE;
    case TBN_GETDISPINFO:
        {
            LPNMTBDISPINFO toolbarDisplayInfo = (LPNMTBDISPINFO)Header;
        }
        return FALSE;
    }

    return FALSE;
}

/**
 * Handles window messages for the menu bar, such as keyboard input and focus changes.
 *
 * \param[in] WindowHandle The handle to the window receiving the message.
 * \param[in] WindowMessage The message identifier.
 * \param[in] wParam The message parameter.
 * \param[in] lParam The message parameter.
 * \param[out] Result A pointer to a variable that receives the result of the message processing.
 * \return TRUE if the message was handled, otherwise FALSE.
 */
_Success_(return)
BOOLEAN ToolStatusMenuBarHandleMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_opt_ LRESULT * Result
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
        LONG virtualKey = (LONG)wParam;

        TSMENU_DPRINTF("HandleMessage keydown msg=0x%lx vk=%ld lParam=0x%p", WindowMessage, virtualKey, (PVOID)lParam);

        if (virtualKey == VK_F10 || virtualKey == VK_MENU)
        {
            if (!TsMenuBarMenuActive && GetFocus() == TsMenuBarWindowHandle)
            {
                // Already keyboard-activated (no popup open): second Alt/F10 deactivates.
                TSMENU_DPRINTF("HandleMessage toggle off vk=%ld", virtualKey);
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
            {
                ToolStatusMenuBarUpdateUiState(TRUE);
            }

            TsMenuBarDelayedActivation = TRUE;
            TSMENU_DPRINTF("HandleMessage delayed activation set vk=%ld", virtualKey);
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
            {
                if (TsMenuBarHotItemIndex >= 0)
                {
                    TSMENU_DPRINTF("HandleMessage escape deactivate");
                    ToolStatusMenuBarDeactivate(TRUE);
                    return TRUE;
                }
            }
            break;
        case VK_LEFT:
            {
                if (TsMenuBarHotItemIndex >= 0)
                {
                    return ToolStatusMenuBarMoveHotItem(-1);
                }
            }
            break;
        case VK_RIGHT:
            {
                if (TsMenuBarHotItemIndex >= 0)
                {
                    return ToolStatusMenuBarMoveHotItem(1);
                }
            }
            break;
        case VK_DOWN:
        case VK_UP:
        case VK_RETURN:
            {
                if (TsMenuBarHotItemIndex >= 0)
                {
                    ToolStatusMenuBarShowFromHotItem(TRUE);
                    return TRUE;
                }
            }
            break;
        }
    }
    break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        {
            TSMENU_DPRINTF("HandleMessage keyup msg=0x%lx vk=%ld lParam=0x%p", WindowMessage, (LONG)wParam, (PVOID)lParam);

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

                TSMENU_DPRINTF("HandleMessage activate menubar from keyup");
                return TRUE;
            }
        }
        break;
    case WM_SYSCHAR:
        {
            TSMENU_DPRINTF("HandleMessage syschar ch=%lc lParam=0x%p", (WCHAR)wParam, (PVOID)lParam);

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

                    TSMENU_DPRINTF("HandleMessage syschar activate index=%ld", buttonIndex);

                    if (Result)
                        *Result = 0;

                    return TRUE;
                }

                TsMenuBarDelayedActivation = FALSE;
            }
        }
        break;
    case WM_KILLFOCUS:
        {
            TSMENU_DPRINTF("HandleMessage killfocus newFocus=%p active=%lu", (HWND)wParam, TsMenuBarMenuActive);

            if ((HWND)wParam != TsMenuBarWindowHandle && !TsMenuBarMenuActive)
            {
                TsMenuBarOldFocusWindowHandle = NULL;
                ToolStatusMenuBarDeactivate(FALSE);
            }
        }
        break;
    case TB_CUSTOMIZE:
        {
            if (TsMenuBarPressedItemIndex >= 0)
            {
                TSMENU_DPRINTF(
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
        }
        break;
    case WM_EXITMENULOOP:
        {
            TSMENU_DPRINTF("HandleMessage exitmenuloop pressed=%ld", TsMenuBarPressedItemIndex);

            if (!TsMenuBarMenuActive)
            {
                ToolStatusMenuBarRemoveHook();

                if (TsMenuBarPressedItemIndex < 0)
                {
                    ToolStatusMenuBarDeactivate(TRUE);
                }
            }
        }
        return FALSE;
    case WM_STYLECHANGED:
        {
            if (wParam == GWL_EXSTYLE)
            {
                STYLESTRUCT* style = (STYLESTRUCT*)lParam;
                TsMenuBarRtlLayout = !!(style->styleNew & WS_EX_LAYOUTRTL);
                TSMENU_DPRINTF("HandleMessage stylechanged rtl=%lu", TsMenuBarRtlLayout);
            }
        }
        break;
    case WM_THEMECHANGED:
    case WM_SYSCOLORCHANGE:
    case WM_SETTINGCHANGE:
        {
            ToolStatusMenuBarUpdateUiState(TsMenuBarMenuActive || GetFocus() == TsMenuBarWindowHandle);
            RebarUpdateBandColors();
            InvalidateRect(TsMenuBarWindowHandle, NULL, TRUE);
        }
        break;
    case WM_DESTROY:
        {
            TSMENU_DPRINTF("HandleMessage destroy");
            ToolStatusMenuBarRemoveHook();

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
            TsMenuBarMenuActive = FALSE;
            TsMenuBarContinueHotTrack = FALSE;
            TsMenuBarSelectFromKeyboard = FALSE;
            TsMenuBarDelayedActivation = FALSE;
            TsMenuBarRtlLayout = FALSE;
        }
        break;
    }

    return FALSE;
}

#define TOOLBAR_AERO_CORNER_RADIUS 3

/**
 * Blends two colors at a specified position.
 *
 * \param Color1 The starting color.
 * \param Color2 The ending color.
 * \param Position The position within the blend range.
 * \param Length The length of the blend range.
 * \return The blended color.
 */
COLORREF ToolbarBlendColor(
    _In_ COLORREF Color1,
    _In_ COLORREF Color2,
    _In_ LONG Position,
    _In_ LONG Length
    )
{
    if (Length <= 0)
        return Color2;

    Position = __max(0, __min(Position, Length));

    return RGB(
        GetRValue(Color1) + (GetRValue(Color2) - GetRValue(Color1)) * Position / Length,
        GetGValue(Color1) + (GetGValue(Color2) - GetGValue(Color1)) * Position / Length,
        GetBValue(Color1) + (GetBValue(Color2) - GetBValue(Color1)) * Position / Length
        );
}

/**
 * Determines whether the current window theme is dark.
 *
 * \return TRUE if the current theme is dark; otherwise, FALSE.
 */
BOOLEAN ToolbarIsDarkTheme(
    VOID
    )
{
    const PH_WINDOW_THEME_PALETTE* palette;
    COLORREF backgroundColor;
    ULONG luminance;

    if (!EnableThemeSupport)
        return FALSE;

    palette = PhGetWindowThemePalette();
    backgroundColor = palette->BackgroundColor;
    luminance =
        GetRValue(backgroundColor) * 299 +
        GetGValue(backgroundColor) * 587 +
        GetBValue(backgroundColor) * 114;

    return luminance < 128000;
}

/**
 * Retrieves an interpolated color from a gradient profile.
 *
 * \param Colors The gradient color profile.
 * \param ColorCount The number of colors in the profile.
 * \param Position The position within the gradient.
 * \param Length The length of the gradient.
 * \return The interpolated color.
 */
COLORREF ToolbarGetProfileColor(
    _In_reads_(ColorCount) const COLORREF* Colors,
    _In_ ULONG ColorCount,
    _In_ LONG Position,
    _In_ LONG Length
    )
{
    LONG colorPosition;
    LONG colorIndex;
    LONG colorRemainder;

    if (ColorCount == 0)
        return RGB(0, 0, 0);

    if (ColorCount == 1 || Length <= 1)
        return Colors[0];

    Position = __max(0, __min(Position, Length - 1));
    colorPosition = Position * (LONG)(ColorCount - 1);
    colorIndex = colorPosition / (Length - 1);
    colorRemainder = colorPosition % (Length - 1);

    if (colorIndex >= (LONG)ColorCount - 1)
        return Colors[ColorCount - 1];

    return ToolbarBlendColor(
        Colors[colorIndex],
        Colors[colorIndex + 1],
        colorRemainder,
        Length - 1
        );
}

/**
 * Draws a vertical gradient profile.
 *
 * \param Hdc The device context to draw into.
 * \param DrawRect The rectangle to draw.
 * \param GradientRect The rectangle defining the gradient range.
 * \param Colors The gradient color profile.
 * \param ColorCount The number of colors in the profile.
 */
VOID ToolbarDrawProfile(
    _In_ HDC Hdc,
    _In_ PRECT DrawRect,
    _In_ PRECT GradientRect,
    _In_reads_(ColorCount) const COLORREF* Colors,
    _In_ ULONG ColorCount
    )
{
    RECT lineRect;
    LONG gradientHeight;
    LONG y;

    if (!Hdc || !DrawRect || !GradientRect || IsRectEmpty(DrawRect))
        return;

    gradientHeight = GradientRect->bottom - GradientRect->top;

    if (gradientHeight <= 0)
        return;

    lineRect.left = DrawRect->left;
    lineRect.right = DrawRect->right;

    for (y = DrawRect->top; y < DrawRect->bottom; y++)
    {
        COLORREF color;

        color = ToolbarGetProfileColor(
            Colors,
            ColorCount,
            y - GradientRect->top,
            gradientHeight
            );
        lineRect.top = y;
        lineRect.bottom = y + 1;
        SetDCBrushColor(Hdc, color);
        FillRect(Hdc, &lineRect, PhGetStockBrush(DC_BRUSH));
    }
}

/**
 * Draws a vertical three-color gradient.
 *
 * \param Hdc The device context to draw into.
 * \param Rect The rectangle to draw.
 * \param TopColor The top gradient color.
 * \param MiddleColor The middle gradient color.
 * \param BottomColor The bottom gradient color.
 */
VOID ToolbarDrawThreePartGradient(
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ COLORREF TopColor,
    _In_ COLORREF MiddleColor,
    _In_ COLORREF BottomColor
    )
{
    RECT lineRect;
    LONG height;
    LONG middle;
    LONG y;

    height = Rect->bottom - Rect->top;
    middle = __max(1, height / 3);
    lineRect.left = Rect->left;
    lineRect.right = Rect->right;

    for (y = 0; y < height; y++)
    {
        COLORREF color;

        if (y <= middle)
        {
            color = ToolbarBlendColor(TopColor, MiddleColor, y, middle);
        }
        else
        {
            color = ToolbarBlendColor(
                MiddleColor,
                BottomColor,
                y - middle,
                __max(1, height - middle - 1)
                );
        }

        lineRect.top = Rect->top + y;
        lineRect.bottom = lineRect.top + 1;
        SetDCBrushColor(Hdc, color);
        FillRect(Hdc, &lineRect, PhGetStockBrush(DC_BRUSH));
    }
}

/**
 * Draws the command bar menu background.
 *
 * \param Hdc The device context to draw into.
 * \param Rect The rectangle to draw.
 */
VOID ToolbarDrawMenuBackground(
    _In_ HDC Hdc,
    _In_ PRECT Rect
    )
{
    RECT lineRect;
    LONG height;
    LONG y;

    height = Rect->bottom - Rect->top;
    lineRect.left = Rect->left;
    lineRect.right = Rect->right;

    for (y = 0; y < height; y++)
    {
        LONG gradientPosition;
        COLORREF color;

        gradientPosition = __max(0, y - height / 3);
        color = ToolbarBlendColor(
            RGB(18, 28, 44),
            RGB(54, 72, 94),
            gradientPosition,
            __max(1, height - height / 3 - 1)
            );
        lineRect.top = Rect->top + y;
        lineRect.bottom = lineRect.top + 1;
        SetDCBrushColor(Hdc, color);
        FillRect(Hdc, &lineRect, PhGetStockBrush(DC_BRUSH));
    }

    SetDCPenColor(Hdc, RGB(35, 66, 97));
    SelectPen(Hdc, PhGetStockPen(DC_PEN));
    MoveToEx(Hdc, Rect->left, Rect->top, NULL);
    LineTo(Hdc, Rect->right, Rect->top);
    SetDCPenColor(Hdc, RGB(232, 243, 252));
    MoveToEx(Hdc, Rect->left, Rect->bottom - 1, NULL);
    LineTo(Hdc, Rect->right, Rect->bottom - 1);
}

/**
 * Draws the dark command bar background.
 *
 * \param Hdc The device context to draw into.
 * \param Rect The rectangle to draw.
 */
VOID ToolbarDrawDarkBackground(
    _In_ HDC Hdc,
    _In_ PRECT Rect
    )
{
    const PH_WINDOW_THEME_PALETTE* palette;
    COLORREF topColor;
    COLORREF middleColor;

    palette = PhGetWindowThemePalette();
    topColor = ToolbarBlendColor(palette->BackgroundColor, palette->ForegroundColor, 1, 10);
    middleColor = ToolbarBlendColor(palette->Background2Color, palette->ForegroundColor, 1, 8);

    ToolbarDrawThreePartGradient(
        Hdc,
        Rect,
        topColor,
        middleColor,
        palette->Background2Color
        );

    SetDCPenColor(Hdc, palette->BorderColor);
    SelectPen(Hdc, PhGetStockPen(DC_PEN));
    MoveToEx(Hdc, Rect->left, Rect->top, NULL);
    LineTo(Hdc, Rect->right, Rect->top);
    SetDCPenColor(Hdc, middleColor);
    MoveToEx(Hdc, Rect->left, Rect->bottom - 1, NULL);
    LineTo(Hdc, Rect->right, Rect->bottom - 1);
}

/**
 * Draws a rounded rectangle outline.
 *
 * \param Hdc The device context to draw into.
 * \param Rect The rectangle bounds.
 * \param Color The outline color.
 * \param Radius The corner radius.
 */
VOID ToolbarDrawRoundedOutline(
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ COLORREF Color,
    _In_ LONG Radius
    )
{
    HGDIOBJ oldBrush;
    HGDIOBJ oldPen;

    SetDCPenColor(Hdc, Color);
    oldPen = SelectPen(Hdc, PhGetStockPen(DC_PEN));
    oldBrush = SelectBrush(Hdc, PhGetStockBrush(NULL_BRUSH));
    RoundRect(Hdc, Rect->left, Rect->top, Rect->right, Rect->bottom, Radius * 2, Radius * 2);
    SelectBrush(Hdc, oldBrush);
    SelectPen(Hdc, oldPen);
}

/**
 * Draws a rounded gradient profile with an outline.
 *
 * \param Hdc The device context to draw into.
 * \param Rect The rectangle to draw.
 * \param Colors The gradient color profile.
 * \param ColorCount The number of colors in the profile.
 * \param BorderColor The outline color.
 * \param Radius The corner radius.
 */
VOID ToolbarDrawRoundedProfile(
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_reads_(ColorCount) const COLORREF* Colors,
    _In_ ULONG ColorCount,
    _In_ COLORREF BorderColor,
    _In_ LONG Radius
    )
{
    HRGN clipRegion;
    INT savedDc;

    savedDc = SaveDC(Hdc);
    clipRegion = CreateRoundRectRgn(
        Rect->left,
        Rect->top,
        Rect->right + 1,
        Rect->bottom + 1,
        Radius * 2,
        Radius * 2
        );

    if (clipRegion)
    {
        ExtSelectClipRgn(Hdc, clipRegion, RGN_AND);
        ToolbarDrawProfile(Hdc, Rect, Rect, Colors, ColorCount);
        DeleteRgn(clipRegion);
    }

    RestoreDC(Hdc, savedDc);
    ToolbarDrawRoundedOutline(Hdc, Rect, BorderColor, Radius);
}

/**
 * Draws a command bar button background.
 *
 * \param ToolbarHandle A handle to the toolbar window.
 * \param Hdc The device context to draw into.
 * \param Rect The button rectangle.
 * \param DarkMode Whether dark mode is active.
 * \param Pressed Whether the button is pressed.
 * \param DpiValue The current window DPI.
 */
VOID ToolbarDrawButtonBackground(
    _In_ HWND ToolbarHandle,
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ BOOLEAN DarkMode,
    _In_ BOOLEAN Pressed,
    _In_ LONG DpiValue
    )
{
    const PH_WINDOW_THEME_PALETTE* palette;
    RECT buttonRect;
    COLORREF colors[3];
    COLORREF borderColor;
    LONG radius;

    UNREFERENCED_PARAMETER(ToolbarHandle);

    if (!DarkMode)
    {
        HTHEME themeHandle;
        BOOLEAN drawn = FALSE;

        // Use the native toolbar visual style so buttons match a standard toolbar.
        // Don't pass the toolbar window; that associates the theme handle with the
        // control and closing it here would affect the control's own theme.
        if (themeHandle = PhOpenThemeData(NULL, VSCLASS_TOOLBAR, DpiValue))
        {
            drawn = PhDrawThemeBackground(themeHandle, Hdc, TP_BUTTON, Pressed ? TS_PRESSED : TS_HOT, Rect, Rect);
            PhCloseThemeData(themeHandle);
        }

        if (drawn)
            return;
    }

    palette = PhGetWindowThemePalette();
    buttonRect = *Rect;
    radius = __max(1, PhScaleToDisplay(TOOLBAR_AERO_CORNER_RADIUS, DpiValue));

    if (DarkMode)
    {
        colors[0] = Pressed ? palette->PressedColor : palette->HighlightColor;
        colors[1] = Pressed ? palette->HighlightColor : palette->Highlight2Color;
        colors[2] = Pressed ? palette->Background2Color : palette->HighlightColor;
        borderColor = palette->BorderColor;
    }
    else if (Pressed)
    {
        colors[0] = RGB(184, 215, 242);
        colors[1] = RGB(204, 228, 247);
        colors[2] = RGB(226, 241, 253);
        borderColor = RGB(84, 138, 186);
    }
    else
    {
        colors[0] = RGB(255, 255, 255);
        colors[1] = RGB(226, 241, 253);
        colors[2] = RGB(184, 215, 242);
        borderColor = RGB(110, 160, 204);
    }

    ToolbarDrawRoundedProfile(
        Hdc,
        &buttonRect,
        colors,
        RTL_NUMBER_OF(colors),
        borderColor,
        radius
        );
}

/**
 * Draws the divider for a split command bar button.
 *
 * \param Hdc The device context to draw into.
 * \param Rect The button rectangle.
 * \param DarkMode Whether dark mode is active.
 * \param Pressed Whether the button is pressed.
 * \param DpiValue The current window DPI.
 */
VOID ToolbarDrawSplitButtonDivider(
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ BOOLEAN DarkMode,
    _In_ BOOLEAN Pressed,
    _In_ LONG DpiValue
    )
{
    const PH_WINDOW_THEME_PALETTE* palette;
    COLORREF dividerColor;
    LONG dividerInset;
    LONG dividerX;

    palette = PhGetWindowThemePalette();
    dividerColor = DarkMode ? palette->BorderColor :
        (Pressed ? RGB(84, 138, 186) : RGB(110, 160, 204));
    dividerInset = PhScaleToDisplay(3, DpiValue);
    dividerX = Rect->right - PhScaleToDisplay(13, DpiValue);

    SelectPen(Hdc, PhGetStockPen(DC_PEN));
    SetDCPenColor(Hdc, dividerColor);
    MoveToEx(Hdc, dividerX, Rect->top + dividerInset, NULL);
    LineTo(Hdc, dividerX, Rect->bottom - dividerInset);
}

/**
 * Draws a command bar separator.
 *
 * \param Hdc The device context to draw into.
 * \param Rect The separator rectangle.
 * \param DarkMode Whether dark mode is active.
 * \param DpiValue The current window DPI.
 */
VOID ToolbarDrawSeparator(
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ BOOLEAN DarkMode,
    _In_ LONG DpiValue
    )
{
    const PH_WINDOW_THEME_PALETTE* palette;
    COLORREF shadowColor;
    COLORREF highlightColor;
    LONG inset;
    LONG separatorX;

    palette = PhGetWindowThemePalette();
    inset = PhScaleToDisplay(4, DpiValue);
    separatorX = (Rect->left + Rect->right) / 2;

    if (DarkMode)
    {
        shadowColor = palette->BorderColor;
        highlightColor = palette->Highlight2Color;
    }
    else
    {
        shadowColor = RGB(180, 190, 205);
        highlightColor = RGB(255, 255, 255);
    }

    SelectPen(Hdc, PhGetStockPen(DC_PEN));
    SetDCPenColor(Hdc, shadowColor);
    MoveToEx(Hdc, separatorX, Rect->top + inset, NULL);
    LineTo(Hdc, separatorX, Rect->bottom - inset);
    SetDCPenColor(Hdc, highlightColor);
    MoveToEx(Hdc, separatorX + 1, Rect->top + inset, NULL);
    LineTo(Hdc, separatorX + 1, Rect->bottom - inset);
}

/**
 * Draws a command bar background.
 *
 * \param Hdc The device context to draw into.
 * \param DrawRect The rectangle to draw.
 * \param GradientRect The rectangle defining the gradient range.
 */
VOID ToolbarDrawCommandBarBackground(
    _In_ HDC Hdc,
    _In_ PRECT DrawRect,
    _In_ PRECT GradientRect
    )
{
    UNREFERENCED_PARAMETER(GradientRect);

    if (ToolbarIsDarkTheme())
    {
        ToolbarDrawDarkBackground(Hdc, DrawRect);
    }
    else
    {
        // Match the rebar's default band background.
        SetDCBrushColor(Hdc, GetSysColor(COLOR_WINDOW));
        FillRect(Hdc, DrawRect, PhGetStockBrush(DC_BRUSH));
    }
}

/**
 * Draws the command bar background for a toolbar hosted in the rebar, with the
 * gradient mapped to the rebar client area so every band lines up.
 *
 * \param ToolbarHandle A handle to the toolbar window.
 * \param Hdc The device context to draw into.
 * \param Rect The rectangle to draw.
 */
EXTERN_C
VOID ToolbarDrawRebarBackground(
    _In_ HWND ToolbarHandle,
    _In_ HDC Hdc,
    _In_ PRECT Rect
    )
{
    HWND rebarHandle;
    RECT gradientRect;
    INT savedDc;

    rebarHandle = GetParent(ToolbarHandle);
    savedDc = SaveDC(Hdc);

    if (rebarHandle && GetClientRect(rebarHandle, &gradientRect))
    {
        MapWindowPoints(rebarHandle, ToolbarHandle, (PPOINT)&gradientRect, 2);
        ToolbarDrawCommandBarBackground(Hdc, Rect, &gradientRect);
    }
    else
    {
        ToolbarDrawCommandBarBackground(Hdc, Rect, Rect);
    }

    if (savedDc)
        RestoreDC(Hdc, savedDc);
}

/**
 * Draws a command bar background variant.
 *
 * \param Hdc The device context to draw into.
 * \param Rect The rectangle to draw.
 * \param Variant The background variant.
 */
EXTERN_C
VOID ToolbarDrawBackground(
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ ULONG Variant
    )
{
    INT savedDc;

    if (!Hdc || !Rect || IsRectEmpty(Rect))
        return;

    savedDc = SaveDC(Hdc);

    if (Variant == 2)
    {
        ToolbarDrawMenuBackground(Hdc, Rect);
    }
    else
    {
        ToolbarDrawCommandBarBackground(Hdc, Rect, Rect);
    }

    if (savedDc)
        RestoreDC(Hdc, savedDc);
}

/**
 * Draws the hot/pressed command bar button background so other rebar bands
 * (e.g. the menu bar) match the main toolbar.
 *
 * \param ToolbarHandle A handle to the toolbar window.
 * \param Hdc The device context to draw into.
 * \param Rect The button rectangle.
 * \param Pressed Whether the button is pressed.
 */
EXTERN_C
VOID ToolbarDrawItemBackground(
    _In_ HWND ToolbarHandle,
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ BOOLEAN Pressed
    )
{
    INT savedDc;

    savedDc = SaveDC(Hdc);
    ToolbarDrawButtonBackground(ToolbarHandle, Hdc, Rect, ToolbarIsDarkTheme(), Pressed, PhGetWindowDpi(ToolbarHandle));

    if (savedDc)
        RestoreDC(Hdc, savedDc);
}

/**
 * Retrieves the command bar button text color.
 *
 * \param Enabled Whether the button is enabled.
 * \return The button text color.
 */
EXTERN_C
COLORREF ToolbarGetItemTextColor(
    _In_ BOOLEAN Enabled
    )
{
    const PH_WINDOW_THEME_PALETTE* palette = PhGetWindowThemePalette();

    if (ToolbarIsDarkTheme())
        return Enabled ? palette->TextColor : palette->DisabledTextColor;

    return Enabled ? RGB(0, 0, 0) : GetSysColor(COLOR_GRAYTEXT);
}

/**
 * Handles toolbar custom drawing.
 *
 * \param DrawInfo The toolbar custom-draw notification information.
 * \return Custom-draw flags controlling subsequent toolbar rendering.
 */
EXTERN_C
LRESULT CALLBACK ToolbarDrawToolbar(
    _In_ LPNMTBCUSTOMDRAW DrawInfo
    )
{
    const PH_WINDOW_THEME_PALETTE* palette;
    BOOLEAN darkMode;

    darkMode = ToolbarIsDarkTheme();
    palette = PhGetWindowThemePalette();

    switch (DrawInfo->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            ToolbarDrawRebarBackground(
                DrawInfo->nmcd.hdr.hwndFrom,
                DrawInfo->nmcd.hdc,
                &DrawInfo->nmcd.rc
                );

            return CDRF_NOTIFYITEMDRAW;
        }
    case CDDS_ITEMPREPAINT:
        {
            TBBUTTONINFO buttonInfo =
            {
                sizeof(TBBUTTONINFO),
                TBIF_STYLE | TBIF_COMMAND | TBIF_STATE | TBIF_IMAGE
            };
            HWND toolbarHandle;
            ULONG currentIndex;
            BOOLEAN isEnabled;
            BOOLEAN isHot;
            BOOLEAN isPressed;
            BOOLEAN isChecked;
            LONG dpiValue;

            toolbarHandle = DrawInfo->nmcd.hdr.hwndFrom;

            if (SendMessage(
                toolbarHandle,
                TB_GETBUTTONINFO,
                DrawInfo->nmcd.dwItemSpec,
                (LPARAM)&buttonInfo
                ) == INT_ERROR)
            {
                return CDRF_DODEFAULT;
            }

            dpiValue = PhGetWindowDpi(toolbarHandle);

            if (buttonInfo.fsStyle & BTNS_SEP)
            {
                INT savedDc;

                savedDc = SaveDC(DrawInfo->nmcd.hdc);
                ToolbarDrawSeparator(
                    DrawInfo->nmcd.hdc,
                    &DrawInfo->nmcd.rc,
                    darkMode,
                    dpiValue
                    );

                if (savedDc)
                    RestoreDC(DrawInfo->nmcd.hdc, savedDc);

                return CDRF_SKIPDEFAULT;
            }

            currentIndex = (ULONG)SendMessage(
                toolbarHandle,
                TB_COMMANDTOINDEX,
                DrawInfo->nmcd.dwItemSpec,
                0
                );
            isEnabled =
                !(DrawInfo->nmcd.uItemState & CDIS_DISABLED) &&
                SendMessage(toolbarHandle, TB_ISBUTTONENABLED, DrawInfo->nmcd.dwItemSpec, 0) != 0;
            isHot =
                !!(DrawInfo->nmcd.uItemState & CDIS_HOT) ||
                SendMessage(toolbarHandle, TB_GETHOTITEM, 0, 0) == currentIndex;
            isPressed =
                !!(DrawInfo->nmcd.uItemState & CDIS_SELECTED) ||
                !!(buttonInfo.fsState & TBSTATE_PRESSED);
            isChecked = !!(buttonInfo.fsState & TBSTATE_CHECKED);

            if (isEnabled && (isPressed || isChecked || isHot))
            {
                INT savedDc;

                savedDc = SaveDC(DrawInfo->nmcd.hdc);
                ToolbarDrawButtonBackground(
                    toolbarHandle,
                    DrawInfo->nmcd.hdc,
                    &DrawInfo->nmcd.rc,
                    darkMode,
                    !!(isPressed || isChecked),
                    dpiValue
                    );

                if (savedDc)
                    RestoreDC(DrawInfo->nmcd.hdc, savedDc);

                if (
                    (buttonInfo.fsStyle & BTNS_DROPDOWN) &&
                    !(buttonInfo.fsStyle & BTNS_WHOLEDROPDOWN)
                    )
                {
                    savedDc = SaveDC(DrawInfo->nmcd.hdc);
                    ToolbarDrawSplitButtonDivider(
                        DrawInfo->nmcd.hdc,
                        &DrawInfo->nmcd.rc,
                        darkMode,
                        !!(isPressed || isChecked),
                        dpiValue
                        );

                    if (savedDc)
                        RestoreDC(DrawInfo->nmcd.hdc, savedDc);
                }
            }

            DrawInfo->clrText = darkMode ? palette->TextColor : RGB(0, 0, 0);
            DrawInfo->clrTextHighlight = DrawInfo->clrText;
            DrawInfo->clrBtnFace = darkMode ? palette->BackgroundColor : RGB(240, 244, 250);
            DrawInfo->clrBtnHighlight = darkMode ? palette->HighlightColor : RGB(255, 255, 255);
            DrawInfo->clrHighlightHotTrack = darkMode ? palette->Highlight2Color : RGB(226, 241, 253);

            return CDRF_DODEFAULT | TBCDRF_NOBACKGROUND | TBCDRF_USECDCOLORS;
        }
    }

    return CDRF_DODEFAULT;
}
