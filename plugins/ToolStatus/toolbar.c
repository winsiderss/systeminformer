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

SIZE ToolBarImageSize = { 16, 16 };
HIMAGELIST ToolBarImageList = NULL;
HFONT ToolbarWindowFont = NULL;
BOOLEAN ToolbarInitialized = FALSE;
ULONG ToolbarSearchRebarIndex = ULONG_MAX;
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

/**
 * Creates a toolbar window.
 *
 * \param ParentWindowHandle A handle to the parent window.
 * \return A handle to the created toolbar window.
 */
HWND ToolbarCreateWindow(
    _In_ HWND ParentWindowHandle
    )
{
    HWND toolbarHandle = PhCreateWindow(
        TOOLBARCLASSNAME,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NOPARENTALIGN | CCS_NODIVIDER |
        TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_AUTOSIZE,
        0, 0, 0, 0,
        ParentWindowHandle,
        NULL,
        NULL,
        NULL
        );

    // Set the toolbar struct size.
    SendMessage(toolbarHandle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    // Set the toolbar extended toolbar styles.
    SendMessage(toolbarHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);

    return toolbarHandle;
}

/**
 * Creates a rebar window.
 *
 * \param ParentWindowHandle A handle to the parent window.
 * \return A handle to the created rebar window.
 */
HWND RebarCreateWindow(
    _In_ HWND ParentWindowHandle
    )
{
    HWND rebarHandle = PhCreateWindow(
        REBARCLASSNAME,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NODIVIDER | CCS_TOP | RBS_VARHEIGHT,
        0, 0, 0, 0,
        ParentWindowHandle,
        NULL,
        NULL,
        NULL
        );

    RebarSetBarInfo();

    return rebarHandle;
}

/**
 * Updates the toolbar window font.
 */
VOID ToolbarUpdateFont(
    VOID
    )
{
    ToolbarWindowFont = SystemInformer_GetFont();
}

/**
 * Updates the toolbar image list.
 *
 * \param DpiChanged Whether the DPI has changed.
 */
VOID ToolbarUpdateImageList(
    _In_ BOOLEAN DpiChanged
    )
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
}

/**
 * Creates the rebar control.
 */
VOID RebarCreate(
    VOID
    )
{
    RebarHandle = RebarCreateWindow(MainWindowHandle);
}

#if TOOLSTATUS_ENABLE_MENUBAR
static VOID MenuBarInitializeLayout(
    VOID
    )
{
    ULONG menuBarIndex;
    ULONG toolbarIndex;
    ULONG toolbarStyle;

    if (!MenuBarHandle || !ToolBarHandle)
        return;

    menuBarIndex = RebarBandToIndex(REBAR_BAND_ID_MENUBAR);

    if (menuBarIndex != ULONG_MAX && menuBarIndex != 0)
        RebarBandMove(menuBarIndex, 0);

    toolbarIndex = RebarBandToIndex(REBAR_BAND_ID_TOOLBAR);

    if (toolbarIndex != ULONG_MAX && toolbarIndex != 1)
    {
        RebarBandMove(toolbarIndex, 1);
        toolbarIndex = RebarBandToIndex(REBAR_BAND_ID_TOOLBAR);
    }

    if (toolbarIndex != ULONG_MAX && RebarGetBandIndexStyle(toolbarIndex, &toolbarStyle))
    {
        SetFlag(toolbarStyle, RBBS_BREAK);
        RebarSetBandIndexStyle(toolbarIndex, toolbarStyle);
    }

    if (toolbarIndex != ULONG_MAX)
    {
        BAND_CHILD_SIZE bandSize;

        if (RebarGetBandIndexChildSize(toolbarIndex, &bandSize))
        {
            // Recompute the toolbar band width and height from the toolbar button size. Enabling
            // the menu bar at runtime inserts the menu bar band onto the toolbar's row, which eats
            // into the toolbar height; the RBBS_BREAK above moves the toolbar to its own row but
            // the collapsed height persists. Mirror the canonical resize path (UpdateLayoutMetrics)
            // by setting the control button height and the band child height to the same value,
            // unconditionally, with the same 22px floor, so the toolbar reclaims its full height.
            ULONG toolbarButtonSize = (ULONG)SendMessage(ToolBarHandle, TB_GETBUTTONSIZE, 0, 0);
            LONG toolbarButtonHeight = ToolStatusGetWindowFontSize(ToolBarHandle, ToolbarWindowFont);
            toolbarButtonHeight = __max((LONG)HIWORD(toolbarButtonSize), toolbarButtonHeight);
            toolbarButtonHeight = __max(22, toolbarButtonHeight); // 22/default toolbar button height

            SendMessage(ToolBarHandle, TB_SETBUTTONSIZE, 0, MAKELPARAM(0, toolbarButtonHeight));

            bandSize.MinChildWidth = LOWORD(toolbarButtonSize);
            bandSize.InitialChildHeight = toolbarButtonHeight;
            bandSize.MinChildHeight = toolbarButtonHeight;
            RebarSetBandIndexChildSize(toolbarIndex, &bandSize);
        }
    }

    SendMessage(ToolBarHandle, TB_AUTOSIZE, 0, 0);
    SendMessage(RebarHandle, WM_SIZE, 0, 0);
}

/**
 * Creates the menu bar control.
 */
VOID MenuBarCreate(
    VOID
    )
{
    MenuBarHandle = ToolStatusMenuBarCreateWindow(RebarHandle);

    if (!MenuBarHandle)
        return;

    if (ToolbarWindowFont)
        SetWindowFont(MenuBarHandle, ToolbarWindowFont, FALSE);

    if (ToolbarWindowFont && ToolStatusMenuBarLoadMenu(MenuBarHandle, MainMenu))
    {
        ULONG menuBarButtonSize = (ULONG)SendMessage(MenuBarHandle, TB_GETBUTTONSIZE, 0, 0);
        LONG menuBarButtonHeight = ToolStatusGetWindowFontSize(MenuBarHandle, ToolbarWindowFont);

        RebarBandInsert(
            REBAR_BAND_ID_MENUBAR,
            MenuBarHandle,
            LOWORD(menuBarButtonSize),
            __max((LONG)HIWORD(menuBarButtonSize), menuBarButtonHeight)
            );

        if ((LONG)HIWORD(menuBarButtonSize) < menuBarButtonHeight)
        {
            SendMessage(MenuBarHandle, TB_SETBUTTONSIZE, 0, MAKELPARAM(0, menuBarButtonHeight));
        }
    }
    else
    {
        DestroyWindow(MenuBarHandle);
        MenuBarHandle = NULL;
    }

    MenuBarInitializeLayout();
}
#endif

/**
 * Creates the toolbar control.
 */
VOID ToolBarCreate(
    VOID
    )
{
    ToolBarHandle = ToolbarCreateWindow(RebarHandle);

    if (ToolBarImageList)
        SendMessage(ToolBarHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList);

    if (ToolbarWindowFont)
        SetWindowFont(ToolBarHandle, ToolbarWindowFont, FALSE);

    ToolbarLoadButtonSettings();

    if (EnableThemeSupport)
    {
        HWND tooltipWindowHandle;

        if (tooltipWindowHandle = (HWND)SendMessage(ToolBarHandle, TB_GETTOOLTIPS, 0, 0))
        {
            PhSetControlTheme(tooltipWindowHandle, L"DarkMode_Explorer");
        }
    }

    if (ToolbarWindowFont)
    {
        ULONG toolbarButtonSize = (ULONG)SendMessage(ToolBarHandle, TB_GETBUTTONSIZE, 0, 0);
        LONG toolbarButtonHeight = ToolStatusGetWindowFontSize(ToolBarHandle, ToolbarWindowFont);
        LONG height = (LONG)HIWORD(toolbarButtonSize);
        LONG barheight = __max((LONG)HIWORD(toolbarButtonSize), toolbarButtonHeight);
        LONG barwidth = LOWORD(toolbarButtonSize);

        if (height > toolbarButtonHeight)
        {
            SendMessage(ToolBarHandle, TB_SETBUTTONSIZE, 0, MAKELPARAM(0, height));
        }

        RebarBandInsert(REBAR_BAND_ID_TOOLBAR, ToolBarHandle, barwidth, barheight);
    }

#if TOOLSTATUS_ENABLE_MENUBAR
    MenuBarInitializeLayout();
#endif
}

/**
 * Creates the search box control.
 */
VOID SearchBoxCreate(
    VOID
    )
{
    ProcessTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), ProcessTreeFilterCallback, NULL);
    ServiceTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), ServiceTreeFilterCallback, NULL);
    NetworkTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), NetworkTreeFilterCallback, NULL);

    SearchboxHandle = PhCreateWindowEx(
        WC_EDIT,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | ES_LEFT | ES_AUTOHSCROLL,
        WS_EX_CLIENTEDGE,
        0, 0, 0, 0,
        MainWindowHandle,
        NULL,
        NULL,
        NULL
        );

    PhCreateSearchControl(
        MainWindowHandle,
        SearchboxHandle,
        L"Search Processes (Ctrl+K)",
        SearchControlCallback,
        NULL
        );
}

/**
 * Updates the search box rebar band.
 */
VOID SearchBoxUpdateRebarBand(
    VOID
    )
{
    ULONG bandIndex;
    LONG dpiValue;
    LONG height;
    LONG width;

    if (!RebarHandle || !SearchboxHandle)
        return;

    dpiValue = SystemInformer_GetWindowDpi();
    height = RebarGetRowHeight(REBAR_BAND_ID_TOOLBAR);

    if (height <= 0)
        height = PhScaleToDisplay(22, dpiValue);

    width = PhScaleToDisplay(255, dpiValue);

    if ((bandIndex = RebarBandToIndex(REBAR_BAND_ID_SEARCHBOX)) == ULONG_MAX)
    {
        RebarBandInsert(
            REBAR_BAND_ID_SEARCHBOX,
            SearchboxHandle,
            width,
            height
            );
    }
    else
    {
        BAND_CHILD_SIZE bandSize;

        if (RebarGetBandIndexChildSize(bandIndex, &bandSize))
        {
            bandSize.InitialChildHeight = height;
            bandSize.MinChildHeight = height;
            bandSize.MinChildWidth = width;
            RebarSetBandIndexChildSize(bandIndex, &bandSize);
        }
    }
}

/**
 * Creates the status bar control.
 */
VOID StatusBarCreate(
    VOID
    )
{
    StatusBarHandle = PhCreateWindow(
        STATUSCLASSNAME,
        NULL,
        WS_CHILD | CCS_BOTTOM | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        MainWindowHandle,
        NULL,
        NULL,
        NULL
        );

    if (ToolbarWindowFont)
        SetWindowFont(StatusBarHandle, ToolbarWindowFont, FALSE);

    if (ToolbarWindowFont)
    {
        LONG height = ToolStatusGetWindowFontSize(StatusBarHandle, ToolbarWindowFont);
        RECT statusBarRect;

        if (PhGetClientRect(StatusBarHandle, &statusBarRect))
        {
            if (statusBarRect.bottom < height)
                SendMessage(StatusBarHandle, SB_SETMINHEIGHT, height, 0);
        }
    }
}

/**
 * Destroys the search box control.
 */
VOID SearchBoxDestroy(
    VOID
    )
{
    if (!SearchboxHandle)
        return;

    if (ProcessTreeFilterEntry)
    {
        PhRemoveTreeNewFilter(PhGetFilterSupportProcessTreeList(), ProcessTreeFilterEntry);
        ProcessTreeFilterEntry = NULL;
    }

    if (ServiceTreeFilterEntry)
    {
        PhRemoveTreeNewFilter(PhGetFilterSupportServiceTreeList(), ServiceTreeFilterEntry);
        ServiceTreeFilterEntry = NULL;
    }

    if (NetworkTreeFilterEntry)
    {
        PhRemoveTreeNewFilter(PhGetFilterSupportNetworkTreeList(), NetworkTreeFilterEntry);
        NetworkTreeFilterEntry = NULL;
    }

    DestroyWindow(SearchboxHandle);
    SearchboxHandle = NULL;
}

#if TOOLSTATUS_ENABLE_MENUBAR
/**
 * Destroys the menu bar control.
 */
VOID MenuBarDestroy(
    VOID
    )
{
    if (!MenuBarHandle)
        return;

    DestroyWindow(MenuBarHandle);
    MenuBarHandle = NULL;
}
#endif

/**
 * Destroys the toolbar control.
 */
VOID ToolBarDestroy(
    VOID
    )
{
    if (!ToolBarHandle)
        return;

    DestroyWindow(ToolBarHandle);
    ToolBarHandle = NULL;
}

/**
 * Destroys the rebar control.
 */
VOID RebarDestroy(
    VOID
    )
{
    if (!RebarHandle)
        return;

    DestroyWindow(RebarHandle);
    RebarHandle = NULL;
}

#if TOOLSTATUS_ENABLE_MENUBAR
/**
 * Applies settings to the menu bar.
 */
VOID MenuBarApplySettings(
    VOID
    )
{
    ULONG index;
    REBARBANDINFO rebarBandInfo =
    {
        sizeof(REBARBANDINFO),
        RBBIM_IDEALSIZE
    };

    if (!MenuBarHandle)
        return;

    if (ToolbarWindowFont)
        SetWindowFont(MenuBarHandle, ToolbarWindowFont, FALSE);

    ToolStatusMenuBarLoadMenu(MenuBarHandle, MainMenu);
    SendMessage(MenuBarHandle, TB_AUTOSIZE, 0, 0);

    if (RebarHandle && (index = RebarBandToIndex(REBAR_BAND_ID_MENUBAR)) != ULONG_MAX)
    {
        if (SendMessage(RebarHandle, RB_GETBANDINFO, index, (LPARAM)&rebarBandInfo))
        {
            SIZE idealWidth = { 0, 0 };

            if (SendMessage(MenuBarHandle, TB_GETIDEALSIZE, FALSE, (LPARAM)&idealWidth))
            {
                rebarBandInfo.cxIdeal = (ULONG)idealWidth.cx;
                SendMessage(RebarHandle, RB_SETBANDINFO, index, (LPARAM)&rebarBandInfo);
            }
        }
    }
}
#endif

/**
 * Applies settings to the toolbar.
 *
 * \param DpiChanged Whether the DPI has changed.
 */
VOID ToolBarApplySettings(
    _In_ BOOLEAN DpiChanged
    )
{
    LONG index;
    LONG buttonCount;
    ULONG bandIndex;
    REBARBANDINFO rebarBandInfo =
    {
        sizeof(REBARBANDINFO),
        RBBIM_IDEALSIZE
    };

    if (!ToolBarHandle)
        return;

    if (DpiChanged)
    {
        if (ToolBarImageList)
        {
            SendMessage(ToolBarHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList);
        }

        ToolbarRemoveButtons();
        ToolbarLoadButtonSettings();
    }

    buttonCount = (LONG)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);

    for (index = 0; index < buttonCount; index++)
    {
        TBBUTTONINFO buttonInfo =
        {
            sizeof(TBBUTTONINFO),
            TBIF_BYINDEX | TBIF_STYLE | TBIF_COMMAND | TBIF_STATE
        };

        if (SendMessage(ToolBarHandle, TB_GETBUTTONINFO, index, (LPARAM)&buttonInfo) == INT_ERROR)
            break;

        if (buttonInfo.fsStyle == BTNS_SEP)
            continue;

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
                if (PhGetIntegerSetting(SETTING_MAIN_WINDOW_ALWAYS_ON_TOP))
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

        SendMessage(ToolBarHandle, TB_SETBUTTONINFO, index, (LPARAM)&buttonInfo);
    }

    SendMessage(ToolBarHandle, TB_AUTOSIZE, 0, 0);

    if (RebarHandle && (bandIndex = RebarBandToIndex(REBAR_BAND_ID_TOOLBAR)) != ULONG_MAX)
    {
        if (SendMessage(RebarHandle, RB_GETBANDINFO, bandIndex, (LPARAM)&rebarBandInfo))
        {
            SIZE idealWidth = { 0, 0 };

            if (SendMessage(ToolBarHandle, TB_GETIDEALSIZE, FALSE, (LPARAM)&idealWidth))
            {
                rebarBandInfo.cxIdeal = (ULONG)idealWidth.cx;
                SendMessage(RebarHandle, RB_SETBANDINFO, bandIndex, (LPARAM)&rebarBandInfo);
            }
        }
    }
}

/**
 * Applies settings to the search box.
 */
VOID SearchBoxApplySettings(
    VOID
    )
{
    if (ToolStatusConfig.SearchBoxEnabled && RebarHandle && SearchboxHandle)
    {
        ShowWindow(SearchboxHandle, SW_SHOW);

        if (SearchBoxDisplayMode == SEARCHBOX_DISPLAY_MODE_HIDEINACTIVE)
        {
            RebarBandRemove(REBAR_BAND_ID_SEARCHBOX);
        }
        else
        {
            SearchBoxUpdateRebarBand();
        }
    }
    else
    {
        RebarBandRemove(REBAR_BAND_ID_SEARCHBOX);

        if (SearchboxHandle)
        {
            PhSearchControlClear(SearchboxHandle);
            ShowWindow(SearchboxHandle, SW_HIDE);

            if (!ToolStatusConfig.SearchBoxEnabled || !RebarHandle)
            {
                SearchBoxDestroy();
            }
        }
    }
}

/**
 * Applies settings to the status bar.
 */
VOID StatusBarApplySettings(
    VOID
    )
{
    if (ToolStatusConfig.StatusBarEnabled)
    {
        if (StatusBarHandle && !IsWindowVisible(StatusBarHandle))
        {
            ShowWindow(StatusBarHandle, SW_SHOW);
        }
    }
    else
    {
        if (StatusBarHandle && IsWindowVisible(StatusBarHandle))
        {
            ShowWindow(StatusBarHandle, SW_HIDE);
        }
    }
}

/**
 * Loads toolbar settings.
 *
 * \param DpiChanged Whether the DPI has changed.
 */
VOID ToolbarLoadSettings(
    _In_ BOOLEAN DpiChanged
    )
{
    ToolbarUpdateFont();

    if (ToolStatusConfig.ToolBarEnabled)
    {
        ToolbarUpdateImageList(DpiChanged);

        if (!RebarHandle)
        {
            RebarCreate();

#if TOOLSTATUS_ENABLE_MENUBAR
            if (ToolStatusConfig.EnableMenuBar && MainMenu && !MenuBarHandle)
                MenuBarCreate();
#endif

            if (!ToolBarHandle)
                ToolBarCreate();
        }

        if (ToolStatusConfig.SearchBoxEnabled && !SearchboxHandle)
        {
            SearchBoxCreate();
        }

        if (ToolStatusConfig.StatusBarEnabled && !StatusBarHandle)
        {
            StatusBarCreate();
        }

#if TOOLSTATUS_ENABLE_MENUBAR
        MenuBarApplySettings();
#endif
        ToolBarApplySettings(DpiChanged);

        if (RebarHandle && !IsWindowVisible(RebarHandle))
            ShowWindow(RebarHandle, SW_SHOW);
    }
    else
    {
        if (RebarHandle)
        {
#if TOOLSTATUS_ENABLE_MENUBAR
            MenuBarDestroy();
#endif
            ToolBarDestroy();
            RebarDestroy();
        }

        if (ToolBarImageList)
        {
            PhImageListDestroy(ToolBarImageList);
            ToolBarImageList = NULL;
        }
    }

    SearchBoxApplySettings();
    StatusBarApplySettings();

    ToolbarInitialized = TRUE;

    // Invoke the LayoutPaddingCallback.
    InvalidateMainWindowLayout();
}

/**
 * Removes all buttons from the toolbar.
 */
VOID ToolbarRemoveButtons(
    VOID
    )
{
    LONG buttonCount = (LONG)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);

    while (buttonCount--)
        SendMessage(ToolBarHandle, TB_DELETEBUTTON, (WPARAM)buttonCount, 0);
}

/**
 * Creates the toolbar controls.
 */
VOID ToolbarCreateControls(
    VOID
    )
{
    ToolbarLoadSettings(FALSE);
    ToolbarCreateGraphs();
    ReBarLoadLayoutSettings();

#if TOOLSTATUS_ENABLE_MENUBAR
    MenuBarApplySettings();
#endif

    ToolStatusApplyMainMenuVisibility(MainWindowHandle);

    if (ToolStatusConfig.SearchBoxEnabled && ToolStatusConfig.SearchAutoFocus && SearchboxHandle)
    {
        SetFocus(SearchboxHandle);
    }
}

/**
 * Destroys the toolbar controls.
 */
VOID ToolbarDestroyControls(
    VOID
    )
{
    ToolbarDestroyGraphs();
    SearchBoxDestroy();
#if TOOLSTATUS_ENABLE_MENUBAR
    MenuBarDestroy();
#endif
    ToolBarDestroy();
    RebarDestroy();
}

/**
 * Resets the toolbar settings to the default values.
 */
VOID ToolbarResetSettings(
    VOID
    )
{
    // Remove all buttons.
    ToolbarRemoveButtons();

    // Add the default buttons.
    ToolbarLoadDefaultButtonSettings();
}

/**
 * Maps a persisted (stable) toolbar id to the current live PHAPP_ID_* command id when loading
 * settings.
 *
 * \param CommandID The stable command identifier.
 * \return The corresponding live command identifier.
 *
 * \remarks The stable namespace is owned by this plugin and never changes, so the layout
 * survives shifts in the main application's auto-generated resource ids. Legacy on-disk values
 * (original historical ids, and raw PHAPP_ID_* from older builds) are migrated here too; the next
 * save rewrites everything in the stable namespace. TIDC_* and separators (0) pass through.
 */
static ULONG ToolbarMapStableToCommandId(
    _In_ ULONG CommandID
    )
{
    switch (CommandID)
    {
    // Current stable namespace (what new saves write)
    case TOOLBAR_SAVE_ID_REFRESH:
        return PHAPP_ID_VIEW_REFRESH;
    case TOOLBAR_SAVE_ID_OPTIONS:
        return PHAPP_ID_HACKER_OPTIONS;
    case TOOLBAR_SAVE_ID_FINDHANDLESORDLLS:
        return PHAPP_ID_HACKER_FINDHANDLESORDLLS;
    case TOOLBAR_SAVE_ID_SYSTEMINFORMATION:
        return PHAPP_ID_VIEW_SYSTEMINFORMATION;
    case TOOLBAR_SAVE_ID_SHOWDETAILSFORALLPROCESSES:
        return PHAPP_ID_HACKER_SHOWDETAILSFORALLPROCESSES;
    case TOOLBAR_SAVE_ID_ALWAYSONTOP:
        return PHAPP_ID_VIEW_ALWAYSONTOP;
    case TOOLBAR_SAVE_ID_RUNASADMINISTRATOR:
        return PHAPP_ID_HACKER_RUNASADMINISTRATOR;

    // Migration only: original historical ids from older configs
    case 40079:
        return PHAPP_ID_HACKER_SHOWDETAILSFORALLPROCESSES;
    case 40082:
        return PHAPP_ID_HACKER_FINDHANDLESORDLLS;
    case 40083:
        return PHAPP_ID_HACKER_OPTIONS;
    case 40091:
        return PHAPP_ID_VIEW_SYSTEMINFORMATION;
    case 40098:
        return PHAPP_ID_VIEW_REFRESH;
    case 40153:
        return PHAPP_ID_VIEW_ALWAYSONTOP;
    case 40077:
        return PHAPP_ID_HACKER_RUNASADMINISTRATOR;
    }

    // TIDC_*, separators (0), and raw PHAPP_ID_* (already-live) configs fall through unchanged.
    return CommandID;
}

/**
 * Maps a live PHAPP_ID_* command id to the stable id used for persistence when saving settings.
 *
 * \param CommandID The live command identifier.
 * \return The corresponding stable command identifier.
 *
 * \remarks TIDC_* and separators (0) pass through.
 */
static ULONG ToolbarMapCommandIdToStable(
    _In_ ULONG CommandID
    )
{
    switch (CommandID)
    {
    case PHAPP_ID_VIEW_REFRESH:
        return TOOLBAR_SAVE_ID_REFRESH;
    case PHAPP_ID_HACKER_OPTIONS:
        return TOOLBAR_SAVE_ID_OPTIONS;
    case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
        return TOOLBAR_SAVE_ID_FINDHANDLESORDLLS;
    case PHAPP_ID_VIEW_SYSTEMINFORMATION:
        return TOOLBAR_SAVE_ID_SYSTEMINFORMATION;
    case PHAPP_ID_HACKER_SHOWDETAILSFORALLPROCESSES:
        return TOOLBAR_SAVE_ID_SHOWDETAILSFORALLPROCESSES;
    case PHAPP_ID_VIEW_ALWAYSONTOP:
        return TOOLBAR_SAVE_ID_ALWAYSONTOP;
    case PHAPP_ID_HACKER_RUNASADMINISTRATOR:
        return TOOLBAR_SAVE_ID_RUNASADMINISTRATOR;
    }

    return CommandID;
}

/**
 * Gets the text associated with a toolbar command.
 *
 * \param CommandID The command identifier.
 * \return A pointer to the text string.
 */
PWSTR ToolbarGetText(
    _In_ ULONG CommandID
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

/**
 * Loads a bitmap from an icon resource.
 *
 * \param Width The width of the image.
 * \param Height The height of the image.
 * \param Name The name or resource identifier of the icon.
 * \param DpiValue The DPI value for scaling.
 * \return A handle to the created bitmap.
 */
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

/**
 * Gets the image associated with a toolbar command.
 *
 * \param CommandID The command identifier.
 * \param DpiValue The DPI value for scaling.
 * \return A handle to the bitmap image.
 */
HBITMAP ToolbarGetImage(
    _In_ ULONG CommandID,
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

/**
 * Loads the default toolbar button settings.
 */
VOID ToolbarLoadDefaultButtonSettings(
    VOID
    )
{
    LONG dpiValue = SystemInformer_GetWindowDpi();

    // Pre-cache the images in the Toolbar array.

    for (ULONG i = 0; i < ARRAYSIZE(ToolbarButtons); i++)
    {
        PTBBUTTON toolbarButton = &ToolbarButtons[i];
        HBITMAP bitmap;

        if (FlagOn(toolbarButton->fsStyle, BTNS_SEP))
            continue;

        if (bitmap = ToolbarGetImage(toolbarButton->idCommand, dpiValue))
        {
            // Add the image, cache the value in the ToolbarButtons array, set the bitmap index.
            toolbarButton->iBitmap = PhImageListAddBitmap(
                ToolBarImageList,
                bitmap,
                NULL
                );

            DeleteBitmap(bitmap);
        }

        // Note: We have to set the string here because TBIF_TEXT doesn't update
        // the button text length when the button is disabled. (dmex)
        toolbarButton->iString = (INT_PTR)(PVOID)ToolbarGetText(toolbarButton->idCommand);
    }

    // Load default settings
    SendMessage(ToolBarHandle, TB_ADDBUTTONS, MAX_DEFAULT_TOOLBAR_ITEMS, (LPARAM)ToolbarButtons);
}

/**
 * Loads the toolbar button settings from the configuration.
 */
VOID ToolbarLoadButtonSettings(
    VOID
    )
{
    ULONG count;
    ULONG64 countInteger;
    ULONG insertedCount = 0;
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

    if (!PhStringToUInt64(&part, 10, &countInteger))
    {
        ToolbarLoadDefaultButtonSettings();
        goto CleanupExit;
    }

    count = (ULONG)countInteger;
    dpiValue = SystemInformer_GetWindowDpi();

    // Allocate the button array
    buttonArray = PhAllocateStack(count * sizeof(TBBUTTON));
    if (!buttonArray) goto CleanupExit;
    memset(buttonArray, 0, count * sizeof(TBBUTTON));

    for (ULONG index = 0; index < count; index++)
    {
        PTBBUTTON button = &buttonArray[index];
        ULONG64 commandInteger;
        PH_STRINGREF commandIdPart;

        if (remaining.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, L'|', &commandIdPart, &remaining);

        if (!PhStringToUInt64(&commandIdPart, 10, &commandInteger))
            continue;

        button->idCommand = (LONG)ToolbarMapStableToCommandId((ULONG)commandInteger);
        button->iBitmap = I_IMAGECALLBACK;
        button->fsState = TBSTATE_ENABLED;

        if (commandInteger)
        {
            button->fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
            // Note: We have to set the string here because TBIF_TEXT doesn't update
            // the button text length when the button is disabled. (dmex)
            button->iString = (INT_PTR)(PVOID)ToolbarGetText((ULONG)button->idCommand);
        }
        else
        {
            button->fsStyle = BTNS_SEP;
        }

        // Pre-cache the image in the Toolbar array on startup.
        for (ULONG i = 0; i < ARRAYSIZE(ToolbarButtons); i++)
        {
            PTBBUTTON toolbarButton = &ToolbarButtons[i];

            if (toolbarButton->idCommand == button->idCommand)
            {
                HBITMAP bitmap;

                if (FlagOn(button->fsStyle, BTNS_SEP))
                    continue;

                if (bitmap = ToolbarGetImage(toolbarButton->idCommand, dpiValue))
                {
                    // Add the image, cache the value in the ToolbarButtons array, set the bitmap index.
                    button->iBitmap = toolbarButton->iBitmap = PhImageListAddBitmap(
                        ToolBarImageList,
                        bitmap,
                        NULL
                        );

                    DeleteBitmap(bitmap);
                }
                break;
            }
        }

        insertedCount++;
    }

    ToolbarRemoveButtons();

    SendMessage(ToolBarHandle, TB_ADDBUTTONS, insertedCount, (LPARAM)buttonArray);

    PhFreeStack(buttonArray);

CleanupExit:
    PhClearReference(&settingsString);
}

/**
 * Saves the toolbar button settings to the configuration.
 */
VOID ToolbarSaveButtonSettings(
    VOID
    )
{
    LONG index;
    LONG count;
    PPH_STRING settingsString;
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    count = (LONG)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);

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

        if (SendMessage(ToolBarHandle, TB_GETBUTTONINFO, index, (LPARAM)&buttonInfo) == INT_ERROR)
            break;

        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%d|",
            ToolbarMapCommandIdToStable(buttonInfo.idCommand)
            );
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PhFinalStringBuilderString(&stringBuilder);
    PhSetStringSetting2(SETTING_NAME_TOOLBAR_CONFIG, &settingsString->sr);
    PhDereferenceObject(settingsString);
}

/**
 * Loads the rebar layout settings from the configuration.
 */
VOID ReBarLoadLayoutSettings(
    VOID
    )
{
    ULONG index;
    ULONG count;
    PPH_STRING settingsString;
    PH_STRINGREF remaining;

    settingsString = PhGetStringSetting(
#if TOOLSTATUS_ENABLE_MENUBAR
        ToolStatusConfig.EnableMenuBar ? SETTING_NAME_REBAR_MENUBAR_CONFIG :
#endif
        SETTING_NAME_REBAR_CONFIG
        );
    remaining = settingsString->sr;

    if (remaining.Length == 0)
        return;

    if (!RebarGetBandCount(&count))
        return;

    for (index = 0; index < count; index++)
    {
        PH_STRINGREF idPart;
        PH_STRINGREF cxPart;
        PH_STRINGREF stylePart;
        ULONG64 idInteger;
        ULONG64 cxInteger;
        ULONG64 styleInteger;
        ULONG oldBandIndex;
        ULONG bandid;
        ULONG bandstyle;
        ULONG bandwidth;
        BAND_STYLE_SIZE rebarBandInfo;

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
        if (!NT_SUCCESS(RtlULong64ToULong(idInteger, &bandid)))
            continue;
        if (!NT_SUCCESS(RtlULong64ToULong(cxInteger, &bandwidth)))
            continue;
        if (!NT_SUCCESS(RtlULong64ToULong(styleInteger, &bandstyle)))
            continue;

        if ((oldBandIndex = RebarBandToIndex(bandid)) != ULONG_MAX && oldBandIndex != index)
        {
            RebarBandMove(oldBandIndex, index);
        }

        if (RebarGetBandIndexStyleSize(index, &rebarBandInfo))
        {
            SetFlag(rebarBandInfo.BandStyle, bandstyle);
            rebarBandInfo.BandWidth = bandwidth;
            RebarSetBandIndexStyleSize(index, &rebarBandInfo);
        }
    }
}

/**
 * Saves the rebar layout settings to the configuration.
 */
VOID ReBarSaveLayoutSettings(
    VOID
    )
{
    ULONG index;
    ULONG count;
    PPH_STRING settingsString;
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    if (RebarGetBandCount(&count))
    {
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
    }

    settingsString = PhFinalStringBuilderString(&stringBuilder);
    PhSetStringSetting2(
#if TOOLSTATUS_ENABLE_MENUBAR
        ToolStatusConfig.EnableMenuBar ? SETTING_NAME_REBAR_MENUBAR_CONFIG :
#endif
        SETTING_NAME_REBAR_CONFIG,
        &settingsString->sr
        );
    PhDereferenceObject(settingsString);
}

/**
 * Adjusts the height of all bands in the rebar control.
 *
 * \param Height The new height for the bands.
 */
VOID RebarAdjustBandHeightLayout(
    _In_ LONG Height
    )
{
    ULONG index;
    ULONG count;

    if (!RebarGetBandCount(&count))
        return;

    for (index = 0; index < count; index++)
    {
        BAND_CHILD_SIZE rebarBandInfo;

        if (RebarGetBandIndexChildSize(index, &rebarBandInfo))
        {
            rebarBandInfo.InitialChildHeight = Height;
            rebarBandInfo.MinChildHeight = Height;
            RebarSetBandIndexChildSize(index, &rebarBandInfo);
        }
    }
}

/**
 * Calculates the font size for a window.
 *
 * \param WindowHandle A handle to the window.
 * \param WindowFont A handle to the font.
 * \return The calculated height of the font, including padding.
 */
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
