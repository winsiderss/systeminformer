/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2023
 *
 */

#include "toolstatus.h"

SIZE ToolBarImageSize = { 16, 16 };
HIMAGELIST ToolBarImageList = NULL;
HFONT ToolbarWindowFont = NULL;
BOOLEAN ToolbarInitialized = FALSE;
TBBUTTON ToolbarButtons[MAX_TOOLBAR_ITEMS] =
{
    // Default toolbar buttons (displayed)
    { I_IMAGECALLBACK, PHAPP_ID_VIEW_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { I_IMAGECALLBACK, PHAPP_ID_HACKER_OPTIONS, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { 0, 0, 0, BTNS_SEP, { 0 }, 0, 0 },
    { I_IMAGECALLBACK, PHAPP_ID_HACKER_FINDHANDLESORDLLS, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { I_IMAGECALLBACK, PHAPP_ID_VIEW_SYSTEMINFORMATION, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { 0, 0, 0, BTNS_SEP, { 0 }, 0, 0 },
    { I_IMAGECALLBACK, TIDC_FINDWINDOW, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { I_IMAGECALLBACK, TIDC_FINDWINDOWTHREAD, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { I_IMAGECALLBACK, TIDC_FINDWINDOWKILL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { 0, 0, 0, BTNS_SEP, { 0 }, 0, 0 },
    { I_IMAGECALLBACK, PHAPP_ID_HACKER_SHOWDETAILSFORALLPROCESSES, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    // Available toolbar buttons (hidden)
    { I_IMAGECALLBACK, PHAPP_ID_VIEW_ALWAYSONTOP, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { I_IMAGECALLBACK, TIDC_POWERMENUDROPDOWN, TBSTATE_ENABLED, BTNS_WHOLEDROPDOWN | BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
};

VOID RebarBandInsert(
    _In_ UINT BandID,
    _In_ HWND HwndChild,
    _In_ UINT cxMinChild,
    _In_ UINT cyMinChild
    )
{
    UINT index;
    REBARBANDINFO rebarBandInfo =
    {
        sizeof(REBARBANDINFO),
        RBBIM_STYLE | RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE,
        RBBS_USECHEVRON | RBBS_VARIABLEHEIGHT // RBBS_NOGRIPPER | RBBS_HIDETITLE | RBBS_TOPALIGN
    };

    rebarBandInfo.wID = BandID;
    rebarBandInfo.hwndChild = HwndChild;
    rebarBandInfo.cxMinChild = cxMinChild;
    rebarBandInfo.cyMinChild = cyMinChild;

    if (ToolStatusConfig.ToolBarLocked)
    {
        SetFlag(rebarBandInfo.fStyle, RBBS_NOGRIPPER);
    }

    if ((index = (UINT)SendMessage(RebarHandle, RB_IDTOINDEX, REBAR_BAND_ID_SEARCHBOX, 0)) != UINT_MAX)
    {
        SendMessage(RebarHandle, RB_INSERTBAND, (WPARAM)index, (LPARAM)&rebarBandInfo);
    }
    else
    {
        SendMessage(RebarHandle, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rebarBandInfo);
    }
}

VOID RebarBandRemove(
    _In_ UINT BandID
    )
{
    UINT index = (UINT)SendMessage(RebarHandle, RB_IDTOINDEX, (WPARAM)BandID, 0);

    if (index == UINT_MAX)
        return;

    SendMessage(RebarHandle, RB_DELETEBAND, (WPARAM)index, 0);
}

BOOLEAN RebarBandExists(
    _In_ UINT BandID
    )
{
    UINT index = (UINT)SendMessage(RebarHandle, RB_IDTOINDEX, (WPARAM)BandID, 0);

    if (index != UINT_MAX)
        return TRUE;

    return FALSE;
}

VOID RebarCreateOrUpdateWindow(
    _In_ BOOLEAN DpiChanged
    )
{
    if (ToolStatusConfig.ToolBarEnabled)
    {
        if (!ToolBarImageList || DpiChanged)
        {
            LONG windowDpi = SystemInformer_GetWindowDpi();

            if (ToolStatusConfig.ToolBarLargeIcons)
            {
                ToolBarImageSize.cx = PhGetSystemMetrics(SM_CXICON, windowDpi);
                ToolBarImageSize.cy = PhGetSystemMetrics(SM_CYICON, windowDpi);
            }
            else
            {
                ToolBarImageSize.cx = PhGetSystemMetrics(SM_CXSMICON, windowDpi);
                ToolBarImageSize.cy = PhGetSystemMetrics(SM_CYSMICON, windowDpi);
            }

            if (ToolBarImageList)
            {
                PhImageListSetIconSize(ToolBarImageList, ToolBarImageSize.cx, ToolBarImageSize.cy);
            }
            else
            {
                ToolBarImageList = PhImageListCreate(
                    ToolBarImageSize.cx,
                    ToolBarImageSize.cy,
                    ILC_MASK | ILC_COLOR32,
                    MAX_DEFAULT_TOOLBAR_ITEMS,
                    MAX_DEFAULT_IMAGELIST_ITEMS
                    );
            }
        }

        if (DpiChanged)
        {
            if (ToolBarHandle)
            {
                SendMessage(ToolBarHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList);

                // Remove all buttons.
                ToolbarRemoveButons();

                // Re-add/update buttons.
                ToolbarLoadButtonSettings();
            }
        }
    }

    if (ToolStatusConfig.ToolBarEnabled && !RebarHandle)
    {
        REBARINFO rebarInfo;

        ToolbarWindowFont = SystemInformer_GetFont();

        RebarHandle = CreateWindow(
            REBARCLASSNAME,
            NULL,
            WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NODIVIDER | CCS_TOP | RBS_VARHEIGHT, // | RBS_AUTOSIZE  CCS_NOPARENTALIGN
            0, 0, 0, 0,
            MainWindowHandle,
            NULL,
            NULL,
            NULL
            );

        ToolBarHandle = CreateWindow(
            TOOLBARCLASSNAME,
            NULL,
            WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NOPARENTALIGN | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_AUTOSIZE,
            0, 0, 0, 0,
            RebarHandle,
            NULL,
            NULL,
            NULL
            );

        memset(&rebarInfo, 0, sizeof(REBARINFO));
        rebarInfo.cbSize = sizeof(REBARINFO);

        // Set the rebar info with no imagelist.
        SendMessage(RebarHandle, RB_SETBARINFO, 0, (LPARAM)&rebarInfo);
        // Set the toolbar struct size.
        SendMessage(ToolBarHandle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        // Set the toolbar extended toolbar styles.
        SendMessage(ToolBarHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
        // Configure the toolbar imagelist.
        SendMessage(ToolBarHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList);
        // Configure the toolbar font.
        SetWindowFont(ToolBarHandle, ToolbarWindowFont, FALSE);
        // Add the buttons to the toolbar.
        ToolbarLoadButtonSettings();

        if (EnableThemeSupport)
        {
            HWND tooltipWindowHandle;

            if (tooltipWindowHandle = (HWND)SendMessage(ToolBarHandle, TB_GETTOOLTIPS, 0, 0))
            {
                PhSetControlTheme(tooltipWindowHandle, L"DarkMode_Explorer");
                //SendMessage(tooltipWindowHandle, TTM_SETWINDOWTHEME, 0, (LPARAM)L"DarkMode_Explorer");
            }
        }

        // Inset the toolbar into the rebar control, also
        // determine the font size and adjust the toolbar height.
        {
            ULONG toolbarButtonSize = (ULONG)SendMessage(ToolBarHandle, TB_GETBUTTONSIZE, 0, 0);
            LONG toolbarButtonHeight = ToolStatusGetWindowFontSize(ToolBarHandle, ToolbarWindowFont);

            if (RebarBandExists(REBAR_BAND_ID_TOOLBAR))
                RebarBandRemove(REBAR_BAND_ID_TOOLBAR);

            RebarBandInsert(REBAR_BAND_ID_TOOLBAR, ToolBarHandle, LOWORD(toolbarButtonSize), __max(HIWORD(toolbarButtonSize), toolbarButtonHeight));

            if (HIWORD(toolbarButtonSize) < toolbarButtonHeight)
            {
                SendMessage(ToolBarHandle, TB_SETBUTTONSIZE, 0, MAKELPARAM(0, toolbarButtonHeight));
            }
        }
    }

    if (ToolStatusConfig.SearchBoxEnabled && !SearchboxHandle)
    {
        ProcessTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), ProcessTreeFilterCallback, NULL);
        ServiceTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), ServiceTreeFilterCallback, NULL);
        NetworkTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), NetworkTreeFilterCallback, NULL);

        if (SearchboxHandle = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_EDIT,
            NULL,
            WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_LEFT | ES_AUTOHSCROLL,

            0, 0,
            0, 0,
            MainWindowHandle,
            NULL,
            NULL,
            NULL
            ))
        {
            PhCreateSearchControl(
                MainWindowHandle,
                SearchboxHandle,
                L"Search Processes (Ctrl+K)",
                SearchControlCallback,
                NULL
                );
        }
    }

    if (ToolStatusConfig.StatusBarEnabled && !StatusBarHandle)
    {
        StatusBarHandle = CreateWindow(
            STATUSCLASSNAME,
            NULL,
            WS_CHILD | CCS_BOTTOM | SBARS_SIZEGRIP, // SBARS_TOOLTIPS
            0,
            0,
            0,
            0,
            MainWindowHandle,
            NULL,
            NULL,
            NULL
            );

        // Configure the statusbar font.
        SetWindowFont(StatusBarHandle, ToolbarWindowFont, FALSE);

        // Determine the font size and adjust the statusbar height.
        {
            LONG height = ToolStatusGetWindowFontSize(StatusBarHandle, ToolbarWindowFont);
            RECT statusBarRect;

            GetClientRect(StatusBarHandle, &statusBarRect);

            if (statusBarRect.bottom < height)
            {
                SendMessage(StatusBarHandle, SB_SETMINHEIGHT, height, 0);
            }
        }
    }

    // Hide or show controls (Note: don't unload or remove at runtime).
    if (ToolStatusConfig.ToolBarEnabled)
    {
        if (RebarHandle && !IsWindowVisible(RebarHandle))
            ShowWindow(RebarHandle, SW_SHOW);
    }
    else
    {
        if (RebarHandle && IsWindowVisible(RebarHandle))
            ShowWindow(RebarHandle, SW_HIDE);
    }

    if (ToolStatusConfig.SearchBoxEnabled && RebarHandle && SearchboxHandle)
    {
        UINT height = (UINT)SendMessage(RebarHandle, RB_GETROWHEIGHT, REBAR_BAND_ID_TOOLBAR, 0);

        // Add the Searchbox band into the rebar control.
        if (!RebarBandExists(REBAR_BAND_ID_SEARCHBOX))
        {
            LONG dpiValue = SystemInformer_GetWindowDpi();

            RebarBandInsert(REBAR_BAND_ID_SEARCHBOX, SearchboxHandle, PhGetDpi(215, dpiValue), height);
        }

        if (!IsWindowVisible(SearchboxHandle))
            ShowWindow(SearchboxHandle, SW_SHOW);

        if (SearchBoxDisplayMode == SEARCHBOX_DISPLAY_MODE_HIDEINACTIVE)
        {
            if (RebarBandExists(REBAR_BAND_ID_SEARCHBOX))
                RebarBandRemove(REBAR_BAND_ID_SEARCHBOX);
        }
    }
    else
    {
        // Remove the Searchbox band from the rebar control.
        if (RebarBandExists(REBAR_BAND_ID_SEARCHBOX))
            RebarBandRemove(REBAR_BAND_ID_SEARCHBOX);

        if (SearchboxHandle)
        {
            // Clear search text and reset search filters.
            SetFocus(SearchboxHandle);
            PhSetWindowText(SearchboxHandle, L"");

            if (IsWindowVisible(SearchboxHandle))
                ShowWindow(SearchboxHandle, SW_HIDE);
        }
    }

    if (ToolStatusConfig.StatusBarEnabled)
    {
        if (StatusBarHandle && !IsWindowVisible(StatusBarHandle))
            ShowWindow(StatusBarHandle, SW_SHOW);
    }
    else
    {
        if (StatusBarHandle && IsWindowVisible(StatusBarHandle))
            ShowWindow(StatusBarHandle, SW_HIDE);
    }

    ToolbarInitialized = TRUE;
}

VOID ToolbarLoadSettings(
    _In_ BOOLEAN DpiChanged
    )
{
    RebarCreateOrUpdateWindow(DpiChanged);

    if (ToolStatusConfig.ToolBarEnabled && ToolBarHandle)
    {
        INT index;
        INT buttonCount;

        buttonCount = (INT)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);

        for (index = 0; index < buttonCount; index++)
        {
            TBBUTTONINFO buttonInfo =
            {
                sizeof(TBBUTTONINFO),
                TBIF_BYINDEX | TBIF_STYLE | TBIF_COMMAND | TBIF_STATE
            };

            // Get settings for first button
            if (SendMessage(ToolBarHandle, TB_GETBUTTONINFO, index, (LPARAM)&buttonInfo) == INT_ERROR)
                break;

            // Skip separator buttons
            if (buttonInfo.fsStyle == BTNS_SEP)
                continue;

            // Add the button text
            buttonInfo.dwMask |= TBIF_TEXT;
            buttonInfo.pszText = ToolbarGetText(buttonInfo.idCommand);

            switch (DisplayStyle)
            {
            case TOOLBAR_DISPLAY_STYLE_IMAGEONLY:
                buttonInfo.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
                break;
            case TOOLBAR_DISPLAY_STYLE_SELECTIVETEXT:
                {
                    switch (buttonInfo.idCommand)
                    {
                    case PHAPP_ID_VIEW_REFRESH:
                    case PHAPP_ID_HACKER_OPTIONS:
                    case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
                    case PHAPP_ID_VIEW_SYSTEMINFORMATION:
                        buttonInfo.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
                        break;
                    default:
                        buttonInfo.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
                        break;
                    }
                }
                break;
            case TOOLBAR_DISPLAY_STYLE_ALLTEXT:
                buttonInfo.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
                break;
            }

            switch (buttonInfo.idCommand)
            {
            case PHAPP_ID_HACKER_SHOWDETAILSFORALLPROCESSES:
                {
                    if (PhGetOwnTokenAttributes().Elevated)
                    {
                        ClearFlag(buttonInfo.fsState, TBSTATE_ENABLED);
                    }
                }
                break;
            case PHAPP_ID_VIEW_ALWAYSONTOP:
                {
                    // Set the pressed state
                    if (PhGetIntegerSetting(L"MainWindowAlwaysOnTop"))
                    {
                        SetFlag(buttonInfo.fsState, TBSTATE_PRESSED);
                    }
                }
                break;
            case TIDC_POWERMENUDROPDOWN:
                {
                    SetFlag(buttonInfo.fsStyle, BTNS_WHOLEDROPDOWN);
                }
                break;
            }

            // Set updated button info
            SendMessage(ToolBarHandle, TB_SETBUTTONINFO, index, (LPARAM)&buttonInfo);
        }

        // Resize the toolbar
        SendMessage(ToolBarHandle, TB_AUTOSIZE, 0, 0);
    }

    if (ToolStatusConfig.ToolBarEnabled && RebarHandle && ToolBarHandle)
    {
        UINT index;
        REBARBANDINFO rebarBandInfo =
        {
            sizeof(REBARBANDINFO),
            RBBIM_IDEALSIZE
        };

        if ((index = (UINT)SendMessage(RebarHandle, RB_IDTOINDEX, REBAR_BAND_ID_TOOLBAR, 0)) != UINT_MAX)
        {
            // Get settings for Rebar band.
            if (SendMessage(RebarHandle, RB_GETBANDINFO, index, (LPARAM)&rebarBandInfo))
            {
                SIZE idealWidth = { 0, 0 };

                // Reset the cxIdeal for the Chevron
                if (SendMessage(ToolBarHandle, TB_GETIDEALSIZE, FALSE, (LPARAM)&idealWidth))
                {
                    rebarBandInfo.cxIdeal = (UINT)idealWidth.cx;

                    SendMessage(RebarHandle, RB_SETBANDINFO, index, (LPARAM)&rebarBandInfo);
                }
            }
        }
    }

    // Invoke the LayoutPaddingCallback.
    InvalidateMainWindowLayout();
}

VOID ToolbarRemoveButons(
    VOID
    )
{
    INT buttonCount = (INT)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);

    while (buttonCount--)
        SendMessage(ToolBarHandle, TB_DELETEBUTTON, (WPARAM)buttonCount, 0);
}

VOID ToolbarResetSettings(
    VOID
    )
{
    // Remove all buttons.
    ToolbarRemoveButons();

    // Add the default buttons.
    ToolbarLoadDefaultButtonSettings();
}

PWSTR ToolbarGetText(
    _In_ UINT CommandID
    )
{
    switch (CommandID)
    {
    case PHAPP_ID_VIEW_REFRESH:
        return L"Refresh";
    case PHAPP_ID_HACKER_OPTIONS:
        return L"Options";
    case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
        return L"Find handles or DLLs";
    case PHAPP_ID_VIEW_SYSTEMINFORMATION:
        return L"System information";
    case TIDC_FINDWINDOW:
        return L"Find window";
    case TIDC_FINDWINDOWTHREAD:
        return L"Find window and thread";
    case TIDC_FINDWINDOWKILL:
        return L"Find window and kill";
    case PHAPP_ID_VIEW_ALWAYSONTOP:
        return L"Always on top";
    case TIDC_POWERMENUDROPDOWN:
        return L"Computer";
    case PHAPP_ID_HACKER_SHOWDETAILSFORALLPROCESSES:
        return L"Show details for all processes";
    }

    return L"ERROR";
}

HBITMAP ToolbarLoadImageFromIcon(
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ PWSTR Name,
    _In_ LONG DpiValue
    )
{
    HICON icon = PhLoadIcon(PluginInstance->DllBase, Name, 0, Width, Height, DpiValue);
    HBITMAP bitmap = PhIconToBitmap(icon, Width, Height);
    DestroyIcon(icon);
    return bitmap;
}

HBITMAP ToolbarGetImage(
    _In_ UINT CommandID,
    _In_ LONG DpiValue
    )
{
    ULONG id = ULONG_MAX;

    switch (CommandID)
    {
    case PHAPP_ID_VIEW_REFRESH:
        {
            if (ToolStatusConfig.ModernIcons)
            {
                if (EnableThemeSupport)
                    id = IDB_ARROW_REFRESH_MODERN_LIGHT;
                else
                    id = IDB_ARROW_REFRESH_MODERN_DARK;
            }
            else
            {
                id = IDI_ARROW_REFRESH;
            }
        }
        break;
    case PHAPP_ID_HACKER_OPTIONS:
        {
            if (ToolStatusConfig.ModernIcons)
            {
                if (EnableThemeSupport)
                    id = IDB_COG_EDIT_MODERN_LIGHT;
                else
                    id = IDB_COG_EDIT_MODERN_DARK;
            }
            else
            {
                id = IDI_COG_EDIT;
            }
        }
        break;
    case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
        {
            if (ToolStatusConfig.ModernIcons)
            {
                if (EnableThemeSupport)
                    id = IDB_FIND_MODERN_LIGHT;
                else
                    id = IDB_FIND_MODERN_DARK;
            }
            else
            {
                id = IDI_FIND;
            }
        }
        break;
    case PHAPP_ID_VIEW_SYSTEMINFORMATION:
        {
            if (ToolStatusConfig.ModernIcons)
            {
                if (EnableThemeSupport)
                    id = IDB_CHART_LINE_MODERN_LIGHT;
                else
                    id = IDB_CHART_LINE_MODERN_DARK;
            }
            else
            {
                id = IDI_CHART_LINE;
            }
        }
        break;
    case TIDC_FINDWINDOW:
        {
            if (ToolStatusConfig.ModernIcons)
            {
                if (EnableThemeSupport)
                    id = IDB_TBAPPLICATION_MODERN_LIGHT;
                else
                    id = IDB_TBAPPLICATION_MODERN_DARK;
            }
            else
            {
                id = IDI_TBAPPLICATION;
            }
        }
        break;
    case TIDC_FINDWINDOWTHREAD:
        {
            if (ToolStatusConfig.ModernIcons)
            {
                if (EnableThemeSupport)
                    id = IDB_APPLICATION_GO_MODERN_LIGHT;
                else
                    id = IDB_APPLICATION_GO_MODERN_DARK;
            }
            else
            {
                id = IDI_APPLICATION_GO;
            }
        }
        break;
    case TIDC_FINDWINDOWKILL:
        {
            if (ToolStatusConfig.ModernIcons)
            {
                if (EnableThemeSupport)
                    id = IDB_CROSS_MODERN_LIGHT;
                else
                    id = IDB_CROSS_MODERN_DARK;
            }
            else
            {
                id = IDI_CROSS;
            }
        }
        break;
    case PHAPP_ID_VIEW_ALWAYSONTOP:
        {
            if (ToolStatusConfig.ModernIcons)
            {
                if (EnableThemeSupport)
                    id = IDB_APPLICATION_GET_MODERN_LIGHT;
                else
                    id = IDB_APPLICATION_GET_MODERN_DARK;
            }
            else
            {
                id = IDI_APPLICATION_GET;
            }
        }
        break;
    case TIDC_POWERMENUDROPDOWN:
        {
            if (ToolStatusConfig.ModernIcons)
            {
                if (EnableThemeSupport)
                    id = IDB_LIGHTBULB_OFF_MODERN_LIGHT;
                else
                    id = IDB_LIGHTBULB_OFF_MODERN_DARK;
            }
            else
            {
                id = IDI_LIGHTBULB_OFF;
            }
        }
        break;
    case PHAPP_ID_HACKER_SHOWDETAILSFORALLPROCESSES:
        {
            return PhGetShieldBitmap(DpiValue, ToolBarImageSize.cx, ToolBarImageSize.cy);
        }
        break;
    }

    if (id != ULONG_MAX)
    {
        if (ToolStatusConfig.ModernIcons)
        {
            return PhLoadImageFormatFromResource(PluginInstance->DllBase, MAKEINTRESOURCE(id), L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, ToolBarImageSize.cx, ToolBarImageSize.cy);
        }

        return ToolbarLoadImageFromIcon(ToolBarImageSize.cx, ToolBarImageSize.cy, MAKEINTRESOURCE(id), DpiValue);
    }

    return NULL;
}

VOID ToolbarLoadDefaultButtonSettings(
    VOID
    )
{
    LONG dpiValue = SystemInformer_GetWindowDpi();

    // Pre-cache the images in the Toolbar array.

    for (UINT i = 0; i < ARRAYSIZE(ToolbarButtons); i++)
    {
        HBITMAP bitmap;

        if (FlagOn(ToolbarButtons[i].fsStyle, BTNS_SEP))
            continue;

        if (bitmap = ToolbarGetImage(ToolbarButtons[i].idCommand, dpiValue))
        {
            // Add the image, cache the value in the ToolbarButtons array, set the bitmap index.
            ToolbarButtons[i].iBitmap = PhImageListAddBitmap(
                ToolBarImageList,
                bitmap,
                NULL
                );

            DeleteBitmap(bitmap);
        }

        // Note: We have to set the string here because TBIF_TEXT doesn't update
        // the button text length when the button is disabled. (dmex)
        ToolbarButtons[i].iString = (INT_PTR)(PVOID)ToolbarGetText(i);
    }

    // Load default settings
    SendMessage(ToolBarHandle, TB_ADDBUTTONS, MAX_DEFAULT_TOOLBAR_ITEMS, (LPARAM)ToolbarButtons);
}

VOID ToolbarLoadButtonSettings(
    VOID
    )
{
    INT count;
    LONG64 countInteger;
    PPH_STRING settingsString;
    PTBBUTTON buttonArray;
    PH_STRINGREF remaining;
    PH_STRINGREF part;
    LONG dpiValue;

    settingsString = PhGetStringSetting(SETTING_NAME_TOOLBAR_CONFIG);

    if (PhIsNullOrEmptyString(settingsString))
    {
        ToolbarLoadDefaultButtonSettings();
        goto CleanupExit;
    }

    remaining = PhGetStringRef(settingsString);

    // Query the number of buttons to insert
    if (!PhSplitStringRefAtChar(&remaining, L'|', &part, &remaining))
    {
        ToolbarLoadDefaultButtonSettings();
        goto CleanupExit;
    }

    if (!PhStringToInteger64(&part, 10, &countInteger))
    {
        ToolbarLoadDefaultButtonSettings();
        goto CleanupExit;
    }

    count = (INT)countInteger;
    dpiValue = SystemInformer_GetWindowDpi();

    // Allocate the button array
    buttonArray = _malloca(count * sizeof(TBBUTTON));
    if (!buttonArray) goto CleanupExit;
    memset(buttonArray, 0, count * sizeof(TBBUTTON));

    for (INT index = 0; index < count; index++)
    {
        LONG64 commandInteger;
        PH_STRINGREF commandIdPart;

        if (remaining.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, L'|', &commandIdPart, &remaining);
        PhStringToInteger64(&commandIdPart, 10, &commandInteger);

        buttonArray[index].idCommand = (INT)commandInteger;
        buttonArray[index].iBitmap = I_IMAGECALLBACK;
        buttonArray[index].fsState = TBSTATE_ENABLED;

        if (commandInteger)
        {
            buttonArray[index].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
            // Note: We have to set the string here because TBIF_TEXT doesn't update
            // the button text length when the button is disabled. (dmex)
            buttonArray[index].iString = (INT_PTR)(PVOID)ToolbarGetText((UINT)commandInteger);
        }
        else
        {
            buttonArray[index].fsStyle = BTNS_SEP;
        }

        // Pre-cache the image in the Toolbar array on startup.
        for (UINT i = 0; i < ARRAYSIZE(ToolbarButtons); i++)
        {
            if (ToolbarButtons[i].idCommand == buttonArray[index].idCommand)
            {
                HBITMAP bitmap;

                if (FlagOn(buttonArray[index].fsStyle, BTNS_SEP))
                    continue;

                if (bitmap = ToolbarGetImage(ToolbarButtons[i].idCommand, dpiValue))
                {
                    // Add the image, cache the value in the ToolbarButtons array, set the bitmap index.
                    buttonArray[index].iBitmap = ToolbarButtons[i].iBitmap = PhImageListAddBitmap(
                        ToolBarImageList,
                        bitmap,
                        NULL
                        );

                    DeleteBitmap(bitmap);
                }
                break;
            }
        }
    }

    ToolbarRemoveButons();

    SendMessage(ToolBarHandle, TB_ADDBUTTONS, count, (LPARAM)buttonArray);

    _freea(buttonArray);

CleanupExit:
    PhClearReference(&settingsString);
}

VOID ToolbarSaveButtonSettings(
    VOID
    )
{
    INT index;
    INT count;
    PPH_STRING settingsString;
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    count = (INT)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);

    PhAppendFormatStringBuilder(
        &stringBuilder,
        L"%d|",
        count
        );

    for (index = 0; index < count; index++)
    {
        TBBUTTONINFO buttonInfo =
        {
            sizeof(TBBUTTONINFO),
            TBIF_BYINDEX | TBIF_IMAGE | TBIF_STYLE | TBIF_COMMAND
        };

        if ((INT)SendMessage(ToolBarHandle, TB_GETBUTTONINFO, index, (LPARAM)&buttonInfo) == INT_ERROR)
            break;

        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%d|",
            buttonInfo.idCommand
            );
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PhFinalStringBuilderString(&stringBuilder);
    PhSetStringSetting2(SETTING_NAME_TOOLBAR_CONFIG, &settingsString->sr);
    PhDereferenceObject(settingsString);
}

VOID ReBarLoadLayoutSettings(
    VOID
    )
{
    UINT index;
    UINT count;
    PPH_STRING settingsString;
    PH_STRINGREF remaining;

    settingsString = PhGetStringSetting(SETTING_NAME_REBAR_CONFIG);
    remaining = settingsString->sr;

    if (remaining.Length == 0)
        return;

    count = (UINT)SendMessage(RebarHandle, RB_GETBANDCOUNT, 0, 0);

    for (index = 0; index < count; index++)
    {
        PH_STRINGREF idPart;
        PH_STRINGREF cxPart;
        PH_STRINGREF stylePart;
        ULONG64 idInteger;
        ULONG64 cxInteger;
        ULONG64 styleInteger;
        UINT oldBandIndex;
        REBARBANDINFO rebarBandInfo =
        {
            sizeof(REBARBANDINFO),
            RBBIM_STYLE | RBBIM_SIZE
        };

        if (remaining.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, L'|', &idPart, &remaining);
        PhSplitStringRefAtChar(&remaining, L'|', &cxPart, &remaining);
        PhSplitStringRefAtChar(&remaining, L'|', &stylePart, &remaining);

        if (!PhStringToUInt64(&idPart, 10, &idInteger))
            continue;
        if (!PhStringToUInt64(&cxPart, 10, &cxInteger))
            continue;
        if (!PhStringToUInt64(&stylePart, 10, &styleInteger))
            continue;

        if ((oldBandIndex = (UINT)SendMessage(RebarHandle, RB_IDTOINDEX, (UINT)idInteger, 0)) == UINT_MAX)
            continue;

        if (oldBandIndex != index)
        {
            SendMessage(RebarHandle, RB_MOVEBAND, oldBandIndex, index);
        }

        if (SendMessage(RebarHandle, RB_GETBANDINFO, index, (LPARAM)&rebarBandInfo))
        {
            rebarBandInfo.cx = (UINT)cxInteger;
            rebarBandInfo.fStyle |= (UINT)styleInteger;

            SendMessage(RebarHandle, RB_SETBANDINFO, index, (LPARAM)&rebarBandInfo);
        }
    }
}

VOID ReBarSaveLayoutSettings(
    VOID
    )
{
    UINT index;
    UINT count;
    PPH_STRING settingsString;
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    count = (UINT)SendMessage(RebarHandle, RB_GETBANDCOUNT, 0, 0);

    for (index = 0; index < count; index++)
    {
        REBARBANDINFO rebarBandInfo =
        {
            sizeof(REBARBANDINFO),
            RBBIM_STYLE | RBBIM_SIZE | RBBIM_ID
        };

        SendMessage(RebarHandle, RB_GETBANDINFO, index, (LPARAM)&rebarBandInfo);

        if (FlagOn(rebarBandInfo.fStyle, RBBS_GRIPPERALWAYS))
        {
            ClearFlag(rebarBandInfo.fStyle, RBBS_GRIPPERALWAYS);
        }

        if (FlagOn(rebarBandInfo.fStyle, RBBS_NOGRIPPER))
        {
            ClearFlag(rebarBandInfo.fStyle, RBBS_NOGRIPPER);
        }

        if (FlagOn(rebarBandInfo.fStyle, RBBS_FIXEDSIZE))
        {
            ClearFlag(rebarBandInfo.fStyle, RBBS_FIXEDSIZE);
        }

        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%u|%u|%u|",
            rebarBandInfo.wID,
            rebarBandInfo.cx,
            rebarBandInfo.fStyle
            );
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PhFinalStringBuilderString(&stringBuilder);
    PhSetStringSetting2(SETTING_NAME_REBAR_CONFIG, &settingsString->sr);
    PhDereferenceObject(settingsString);
}

VOID RebarAdjustBandHeightLayout(
    _In_ LONG Height
    )
{
    UINT index;
    UINT count;

    count = (UINT)SendMessage(RebarHandle, RB_GETBANDCOUNT, 0, 0);

    for (index = 0; index < count; index++)
    {
        REBARBANDINFO rebarBandInfo =
        {
            sizeof(REBARBANDINFO),
            RBBIM_CHILDSIZE
        };

        SendMessage(RebarHandle, RB_GETBANDINFO, index, (LPARAM)&rebarBandInfo);
        rebarBandInfo.cyMinChild = Height;
        SendMessage(RebarHandle, RB_SETBANDINFO, index, (LPARAM)&rebarBandInfo);
    }
}

LONG ToolStatusGetWindowFontSize(
    _In_ HWND WindowHandle,
    _In_ HFONT WindowFont
    )
{
    LONG height = 0;
    TEXTMETRIC textMetrics;
    HDC hdc;

    if (hdc = GetDC(WindowHandle))
    {
        SelectFont(hdc, WindowFont);
        GetTextMetrics(hdc, &textMetrics);

        // Below we try to match the height as calculated by the toolbar, even if it
        // involves magic numbers. On Vista and above there seems to be extra padding.

        height = textMetrics.tmHeight;
        ReleaseDC(WindowHandle, hdc);
    }

    height += 5; // Add magic padding

    return height;
}
