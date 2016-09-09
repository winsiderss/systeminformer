/*
 * Process Hacker ToolStatus -
 *   toolstatus header
 *
 * Copyright (C) 2011-2016 dmex
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
#define SETTING_NAME_TOOLSTATUS_CONFIG (PLUGIN_NAME L".Config")
#define SETTING_NAME_REBAR_CONFIG (PLUGIN_NAME L".RebarConfig")
#define SETTING_NAME_TOOLBAR_CONFIG (PLUGIN_NAME L".ToolbarConfig")
#define SETTING_NAME_STATUSBAR_CONFIG (PLUGIN_NAME L".StatusbarConfig")
#define SETTING_NAME_TOOLBAR_THEME (PLUGIN_NAME L".ToolbarTheme")
#define SETTING_NAME_TOOLBARDISPLAYSTYLE (PLUGIN_NAME L".ToolbarDisplayStyle")
#define SETTING_NAME_SEARCHBOXDISPLAYMODE (PLUGIN_NAME L".SearchBoxDisplayMode")

#define MAX_DEFAULT_TOOLBAR_ITEMS 9
#define MAX_DEFAULT_STATUSBAR_ITEMS 3
#define MAX_TOOLBAR_ITEMS 12
#define MAX_STATUSBAR_ITEMS 14

#define ID_SEARCH_CLEAR (WM_APP + 1)
#define TIDC_FINDWINDOW (WM_APP + 2)
#define TIDC_FINDWINDOWTHREAD (WM_APP + 3)
#define TIDC_FINDWINDOWKILL (WM_APP + 4)
#define TIDC_POWERMENUDROPDOWN (WM_APP + 5)

typedef enum _TOOLBAR_DISPLAY_STYLE
{
    TOOLBAR_DISPLAY_STYLE_IMAGEONLY,
    TOOLBAR_DISPLAY_STYLE_SELECTIVETEXT,
    TOOLBAR_DISPLAY_STYLE_ALLTEXT
} TOOLBAR_DISPLAY_STYLE;

typedef enum _TOOLBAR_COMMAND_ID
{
    COMMAND_ID_ENABLE_MENU = 1,
    COMMAND_ID_ENABLE_SEARCHBOX,
    COMMAND_ID_ENABLE_CPU_GRAPH,
    COMMAND_ID_ENABLE_MEMORY_GRAPH,
    COMMAND_ID_ENABLE_COMMIT_GRAPH,
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
    SEARCHBOX_DISPLAY_MODE_ALWAYSSHOW,
    SEARCHBOX_DISPLAY_MODE_HIDEINACTIVE,
    //SEARCHBOX_DISPLAY_MODE_AUTOHIDE
} SEARCHBOX_DISPLAY_MODE;

typedef enum _REBAR_BAND_ID
{
    REBAR_BAND_ID_TOOLBAR,
    REBAR_BAND_ID_SEARCHBOX,
    REBAR_BAND_ID_CPUGRAPH,
    REBAR_BAND_ID_MEMGRAPH,
    REBAR_BAND_ID_COMMITGRAPH,
    REBAR_BAND_ID_IOGRAPH
} REBAR_BAND;

typedef enum _REBAR_DISPLAY_LOCATION
{
    REBAR_DISPLAY_LOCATION_TOP,
    REBAR_DISPLAY_LOCATION_LEFT,
    REBAR_DISPLAY_LOCATION_BOTTOM,
    REBAR_DISPLAY_LOCATION_RIGHT,
} REBAR_DISPLAY_LOCATION;

typedef union _TOOLSTATUS_CONFIG
{
    ULONG Flags;
    struct
    {
        ULONG ToolBarEnabled : 1;
        ULONG SearchBoxEnabled : 1;
        ULONG StatusBarEnabled : 1;
        ULONG ToolBarLocked : 1;
        ULONG ResolveGhostWindows : 1;

        ULONG ModernIcons : 1;
        ULONG AutoHideMenu : 1;
        ULONG CpuGraphEnabled : 1;
        ULONG MemGraphEnabled : 1;
        ULONG CommitGraphEnabled : 1;
        ULONG IoGraphEnabled : 1;

        ULONG Spare : 21;
    };
} TOOLSTATUS_CONFIG;

extern TOOLSTATUS_CONFIG ToolStatusConfig;
extern HWND ProcessTreeNewHandle;
extern HWND ServiceTreeNewHandle;
extern HWND NetworkTreeNewHandle;
extern INT SelectedTabIndex;
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
extern TBBUTTON ToolbarButtons[MAX_TOOLBAR_ITEMS];

extern PPH_PLUGIN PluginInstance;
extern PPH_TN_FILTER_ENTRY ProcessTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY ServiceTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY NetworkTreeFilterEntry;

PTOOLSTATUS_TAB_INFO FindTabInfo(
    _In_ INT TabIndex
    );

// toolbar.c

typedef HRESULT (WINAPI *_LoadIconMetric)(
    _In_ HINSTANCE hinst,
    _In_ PCWSTR pszName,
    _In_ int lims,
    _Out_ HICON *phico
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

HBITMAP ToolbarGetImage(
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

// main.c

HWND GetCurrentTreeNewHandle(
    VOID
    );

VOID ShowCustomizeMenu(
    VOID
    );

// options.c

VOID ShowOptionsDialog(
    _In_opt_ HWND Parent
    );

// filter.c

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

NTSTATUS QueryServiceFileName(
    _In_ PPH_STRINGREF ServiceName,
    _Out_ PPH_STRING *ServiceFileName,
    _Out_ PPH_STRING *ServiceBinaryPath
    );

// searchbox.c

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
    HFONT WindowFont;
    HIMAGELIST ImageList;

    HBRUSH BrushNormal;
    HBRUSH BrushPushed;
    HBRUSH BrushHot;
    //COLORREF BackgroundColorRef;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG Hot : 1;
            ULONG Pushed : 1;
            ULONG Spare : 30;
        };
    };
} EDIT_CONTEXT, *PEDIT_CONTEXT;

HBITMAP LoadImageFromResources(
    _In_ UINT Width,
    _In_ UINT Height,
    _In_ PCWSTR Name,
    _In_ BOOLEAN RGBAImage
    );

// graph.c

extern HWND CpuGraphHandle;
extern HWND MemGraphHandle;
extern HWND CommitGraphHandle;
extern HWND IoGraphHandle;

VOID ToolbarCreateGraphs(VOID);
VOID ToolbarUpdateGraphs(VOID);
VOID ToolbarUpdateGraphsInfo(LPNMHDR Header);

// statusbar.c

typedef struct _STATUSBAR_ITEM
{
    ULONG Id;
} STATUSBAR_ITEM, *PSTATUSBAR_ITEM;

extern ULONG ProcessesUpdatedCount;
extern HWND StatusBarHandle;
extern PPH_LIST StatusBarItemList;
extern ULONG StatusBarItems[MAX_STATUSBAR_ITEMS];

VOID StatusBarLoadSettings(
    VOID
    );

VOID StatusBarSaveSettings(
    VOID
    );

VOID StatusBarResetSettings(
    VOID
    );

PWSTR StatusBarGetText(
    _In_ ULONG CommandID
    );

VOID StatusBarUpdate(
    _In_ BOOLEAN ResetMaxWidths
    );

VOID StatusBarShowMenu(
    VOID
    );

// customizetb.c

VOID ToolBarShowCustomizeDialog(
    VOID
    );

// customizesb.c

typedef enum _ID_STATUS
{
    ID_STATUS_NONE,
    ID_STATUS_CPUUSAGE,
    ID_STATUS_COMMITCHARGE,
    ID_STATUS_PHYSICALMEMORY,
    ID_STATUS_NUMBEROFPROCESSES,
    ID_STATUS_NUMBEROFTHREADS,
    ID_STATUS_NUMBEROFHANDLES,
    ID_STATUS_IO_RO,
    ID_STATUS_IO_W,
    ID_STATUS_MAX_CPU_PROCESS,
    ID_STATUS_MAX_IO_PROCESS,
    ID_STATUS_NUMBEROFVISIBLEITEMS,
    ID_STATUS_NUMBEROFSELECTEDITEMS,
    ID_STATUS_INTERVALSTATUS,
    ID_STATUS_FREEMEMORY
} ID_STATUS;

VOID StatusBarShowCustomizeDialog(
    VOID
    );

// Shared by customizetb.c and customizesb.c

typedef struct _BUTTON_CONTEXT
{
    INT IdCommand;
    INT IdBitmap;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG IsVirtual : 1;
            ULONG IsRemovable : 1;
            ULONG IsSeparator : 1;
            ULONG Spare : 29;
        };
    };
} BUTTON_CONTEXT, *PBUTTON_CONTEXT;

typedef struct _CUSTOMIZE_CONTEXT
{
    HWND DialogHandle;
    HWND AvailableListHandle;
    HWND CurrentListHandle;
    HWND MoveUpButtonHandle;
    HWND MoveDownButtonHandle;
    HWND AddButtonHandle;
    HWND RemoveButtonHandle;

    INT BitmapWidth;
    HFONT FontHandle;
    HIMAGELIST ImageListHandle;
} CUSTOMIZE_CONTEXT, *PCUSTOMIZE_CONTEXT;

#endif