/*
 * Process Hacker ToolStatus -
 *   toolstatus header
 *
 * Copyright (C) 2011-2015 dmex
 * Copyright (C) 2010-2013 wj32
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

#ifndef _TOOLSTATUS_H
#define _TOOLSTATUS_H

#pragma comment(lib, "WindowsCodecs.lib")

#define CINTERFACE
#define COBJMACROS
#define INITGUID
#include <phdk.h>
#include <phappresource.h>
#include <windowsx.h>
#include <Wincodec.h>
#include <toolstatusintf.h>

#include "resource.h"

#define PLUGIN_NAME TOOLSTATUS_PLUGIN_NAME
#define SETTING_NAME_ENABLE_TOOLBAR (PLUGIN_NAME L".EnableToolBar")
#define SETTING_NAME_ENABLE_SEARCHBOX (PLUGIN_NAME L".EnableSearchBox")
#define SETTING_NAME_ENABLE_STATUSBAR (PLUGIN_NAME L".EnableStatusBar")
#define SETTING_NAME_ENABLE_MODERNICONS (PLUGIN_NAME L".EnableModernIcons")
#define SETTING_NAME_ENABLE_RESOLVEGHOSTWINDOWS (PLUGIN_NAME L".ResolveGhostWindows")
#define SETTING_NAME_ENABLE_AUTOHIDE_MENU (PLUGIN_NAME L".AutoHideMenu")
#define SETTING_NAME_TOOLBAR_THEME (PLUGIN_NAME L".ToolbarTheme")
#define SETTING_NAME_TOOLBAR_CONFIG (PLUGIN_NAME L".ToolbarButtonConfig")
#define SETTING_NAME_TOOLBARDISPLAYSTYLE (PLUGIN_NAME L".ToolbarDisplayStyle")
#define SETTING_NAME_TOOLBAR_LOCKED (PLUGIN_NAME L".ToolbarLocked")
#define SETTING_NAME_TOOLBAR_ENABLE_CPUGRAPH (PLUGIN_NAME L".ToolbarCpuGraphEnabled")
#define SETTING_NAME_TOOLBAR_ENABLE_MEMGRAPH (PLUGIN_NAME L".ToolbarMemGraphEnabled")
#define SETTING_NAME_TOOLBAR_ENABLE_IOGRAPH (PLUGIN_NAME L".ToolbarIoGraphEnabled")
#define SETTING_NAME_SEARCHBOXDISPLAYMODE (PLUGIN_NAME L".SearchBoxDisplayMode")
#define SETTING_NAME_REBAR_CONFIG (PLUGIN_NAME L".RebarConfig")
#define SETTING_NAME_STATUSBAR_CONFIG (PLUGIN_NAME L".StatusConfig")

#define MAX_DEFAULT_TOOLBAR_ITEMS 9
#define MAX_DEFAULT_STATUSBAR_ITEMS 3

#define ID_SEARCH_CLEAR (WM_APP + 1)
#define TIDC_FINDWINDOW (WM_APP + 2)
#define TIDC_FINDWINDOWTHREAD (WM_APP + 3)
#define TIDC_FINDWINDOWKILL (WM_APP + 4)
#define TIDC_POWERMENUDROPDOWN (WM_APP + 5)

typedef enum _TOOLBAR_DISPLAY_STYLE
{
    ToolbarDisplayImageOnly,
    ToolbarDisplaySelectiveText,
    ToolbarDisplayAllText
} TOOLBAR_DISPLAY_STYLE;

typedef enum _TOOLBAR_COMMAND_ID
{
    COMMAND_ID_ENABLE_MENU = 1,
    COMMAND_ID_ENABLE_SEARCHBOX,
    COMMAND_ID_ENABLE_CPU_GRAPH,
    COMMAND_ID_ENABLE_MEMORY_GRAPH,
    COMMAND_ID_ENABLE_IO_GRAPH,
    COMMAND_ID_TOOLBAR_LOCKUNLOCK,
    COMMAND_ID_TOOLBAR_CUSTOMIZE,
} TOOLBAR_COMMAND_ID;

typedef enum _TOOLBAR_THEME
{
    TOOLBAR_THEME_NONE,
    TOOLBAR_THEME_BLACK,
    TOOLBAR_THEME_BLUE
} TOOLBAR_THEME;

typedef enum _SEARCHBOX_DISPLAY_MODE
{
    SearchBoxDisplayAlwaysShow = 0,
    SearchBoxDisplayHideInactive = 1,
    //SearchBoxDisplayAutoHide = 2
} SEARCHBOX_DISPLAY_MODE;

typedef enum _REBAR_BAND_ID
{
    REBAR_BAND_ID_TOOLBAR,
    REBAR_BAND_ID_SEARCHBOX,
    REBAR_BAND_ID_CPUGRAPH,
    REBAR_BAND_ID_MEMGRAPH,
    REBAR_BAND_ID_IOGRAPH
} REBAR_BAND;

typedef enum _REBAR_DISPLAY_LOCATION
{
    RebarLocationTop = 0,
    RebarLocationLeft = 1,
    RebarLocationBottom = 2,
    RebarLocationRight = 3,
} REBAR_DISPLAY_LOCATION;

extern HWND ProcessTreeNewHandle;
extern HWND ServiceTreeNewHandle;
extern HWND NetworkTreeNewHandle;
extern INT SelectedTabIndex;
extern BOOLEAN EnableToolBar;
extern BOOLEAN EnableSearchBox;
extern BOOLEAN EnableStatusBar;
extern BOOLEAN AutoHideMenu;
extern BOOLEAN ToolBarLocked;
extern BOOLEAN UpdateAutomatically;
extern BOOLEAN UpdateGraphs;
extern TOOLBAR_THEME ToolBarTheme;
extern TOOLBAR_DISPLAY_STYLE DisplayStyle;
extern SEARCHBOX_DISPLAY_MODE SearchBoxDisplayMode;
extern REBAR_DISPLAY_LOCATION RebarDisplayLocation;

extern HWND RebarHandle;
extern HWND ToolBarHandle;
extern HWND SearchboxHandle;

extern HMENU MainMenu;
extern HACCEL AcceleratorTable;
extern PPH_STRING SearchboxText;
extern PH_PLUGIN_SYSTEM_STATISTICS SystemStatistics;

extern HIMAGELIST ToolBarImageList;
extern TBBUTTON ToolbarButtons[11];

extern PPH_PLUGIN PluginInstance;
extern PPH_TN_FILTER_ENTRY ProcessTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY ServiceTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY NetworkTreeFilterEntry;

PTOOLSTATUS_TAB_INFO FindTabInfo(
    _In_ INT TabIndex
    );

VOID RebarBandInsert(
    _In_ UINT BandID,
    _In_ HWND HwndChild,
    _In_ UINT cyMinChild,
    _In_ UINT cxMinChild
    );

VOID RebarBandRemove(
    _In_ UINT BandID
    );

BOOLEAN RebarBandExists(
    _In_ UINT BandID
    );

VOID ToolbarLoadSettings(
    VOID
    );

VOID ToolbarResetSettings(
    VOID
    );

PWSTR ToolbarGetText(
    _In_ INT CommandID
    );

VOID ToolbarLoadButtonSettings(
    VOID
    );

VOID ToolbarSaveButtonSettings(
    VOID
    );

VOID ReBarLoadLayoutSettings(
    VOID
    );

VOID ReBarSaveLayoutSettings(
    VOID
    );

HWND GetCurrentTreeNewHandle(
    VOID
    );

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

BOOLEAN WordMatchStringRef(
    _In_ PPH_STRINGREF Text
    );

BOOLEAN ProcessTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );
BOOLEAN ServiceTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );
BOOLEAN NetworkTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

HWND CreateSearchControl(
    _In_ UINT CmdId
    );

typedef struct _EDIT_CONTEXT
{
    UINT CommandID;
    LONG CXWidth;
    INT CXBorder;
    INT ImageWidth;
    INT ImageHeight;  

    HWND WindowHandle;
    HIMAGELIST ImageList;

    HBRUSH BrushNormal;
    HBRUSH BrushPushed;
    HBRUSH BrushHot;
    COLORREF BackgroundColorRef;

    BOOLEAN Hot;
    BOOLEAN Pushed;
} EDIT_CONTEXT, *PEDIT_CONTEXT;

HBITMAP LoadImageFromResources(
    _In_ UINT Width,
    _In_ UINT Height,
    _In_ PCWSTR Name
    );

// customizetb.c

VOID ShowCustomizeDialog(
    VOID
    );

// graph.c

extern BOOLEAN ToolBarEnableCpuGraph;
extern BOOLEAN ToolBarEnableMemGraph;
extern BOOLEAN ToolBarEnableIoGraph;
extern HWND CpuGraphHandle;
extern HWND MemGraphHandle;
extern HWND IoGraphHandle;

VOID ToolbarCreateGraphs(VOID);
VOID ToolbarUpdateGraphs(VOID);
VOID ToolbarUpdateGraphsInfo(LPNMHDR Header);

// statusbar.c

typedef struct _STATUSBAR_ITEM
{
    ULONG Id;
    PWSTR Name;
} STATUSBAR_ITEM, *PSTATUSBAR_ITEM;

extern ULONG ProcessesUpdatedCount;
extern HWND StatusBarHandle;
extern PPH_LIST StatusBarItemList;
extern STATUSBAR_ITEM StatusBarItems[14];

VOID StatusBarLoadSettings(
    VOID
    );

VOID StatusBarSaveSettings(
    VOID
    );

VOID StatusBarResetSettings(
    VOID
    );

VOID StatusBarUpdate(
    _In_ BOOLEAN ResetMaxWidths
    );

VOID StatusBarShowMenu(
    _In_ PPOINT Point
    );

// customizesb.c

VOID StatusBarShowCustomizeDialog(
    VOID
    );

#endif