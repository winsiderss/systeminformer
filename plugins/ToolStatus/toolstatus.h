/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2011-2020
 *
 */

#ifndef _TOOLSTATUS_H
#define _TOOLSTATUS_H

#include <phdk.h>
#include <phapppub.h>
#include <phappresource.h>
#include <settings.h>

#include <uxtheme.h>
#include <vssym32.h>

#include "resource.h"
#include <toolstatusintf.h>

#define PLUGIN_NAME TOOLSTATUS_PLUGIN_NAME
#define SETTING_NAME_TOOLSTATUS_CONFIG (PLUGIN_NAME L".Config")
#define SETTING_NAME_REBAR_CONFIG (PLUGIN_NAME L".RebarConfig")
#define SETTING_NAME_TOOLBAR_CONFIG (PLUGIN_NAME L".ToolbarButtonConfig")
#define SETTING_NAME_TOOLBAR_GRAPH_CONFIG (PLUGIN_NAME L".ToolbarGraphConfig")
#define SETTING_NAME_STATUSBAR_CONFIG (PLUGIN_NAME L".StatusbarConfig")
//#define SETTING_NAME_TOOLBAR_THEME (PLUGIN_NAME L".ToolbarTheme")
#define SETTING_NAME_TOOLBARDISPLAYSTYLE (PLUGIN_NAME L".ToolbarDisplayStyle")
#define SETTING_NAME_SEARCHBOXDISPLAYMODE (PLUGIN_NAME L".SearchBoxDisplayMode")
#define SETTING_NAME_TASKBARDISPLAYSTYLE (PLUGIN_NAME L".TaskbarDisplayStyle")
#define SETTING_NAME_SHOWSYSINFOGRAPH (PLUGIN_NAME L".ToolbarShowSystemInfoGraph")

#define MAX_DEFAULT_TOOLBAR_ITEMS 11
#define MAX_DEFAULT_STATUSBAR_ITEMS 3
#define MAX_TOOLBAR_ITEMS 13
#define MAX_STATUSBAR_ITEMS 15

#define TIDC_FINDWINDOW (WM_APP + 1)
#define TIDC_FINDWINDOWTHREAD (WM_APP + 2)
#define TIDC_FINDWINDOWKILL (WM_APP + 3)
#define TIDC_POWERMENUDROPDOWN (WM_APP + 4)

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
    COMMAND_ID_TOOLBAR_LOCKUNLOCK,
    COMMAND_ID_TOOLBAR_CUSTOMIZE,
    COMMAND_ID_GRAPHS_CUSTOMIZE,
} TOOLBAR_COMMAND_ID;

//typedef enum _TOOLBAR_THEME
//{
//    TOOLBAR_THEME_NONE,
//    TOOLBAR_THEME_BLACK,
//    TOOLBAR_THEME_BLUE
//} TOOLBAR_THEME;

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
        ULONG Reserved : 4;
        ULONG SearchAutoFocus : 1;
        ULONG Spare : 20;
    };
} TOOLSTATUS_CONFIG;

extern TOOLSTATUS_CONFIG ToolStatusConfig;
extern HWND ProcessTreeNewHandle;
extern HWND ServiceTreeNewHandle;
extern HWND NetworkTreeNewHandle;
extern INT SelectedTabIndex;
extern BOOLEAN UpdateAutomatically;
extern BOOLEAN UpdateGraphs;
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
extern HFONT ToolbarWindowFont;

extern PPH_PLUGIN PluginInstance;
extern PPH_TN_FILTER_ENTRY ProcessTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY ServiceTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY NetworkTreeFilterEntry;

PTOOLSTATUS_TAB_INFO FindTabInfo(
    _In_ INT TabIndex
    );

// toolbar.c

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

VOID ToolbarLoadDefaultButtonSettings(
    VOID
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

VOID RebarAdjustBandHeightLayout(
    _In_ LONG Height
    );

LONG ToolStatusGetWindowFontSize(
    _In_ HWND WindowHandle,
    _In_ HFONT WindowFont
    );

// main.c

HWND GetCurrentTreeNewHandle(
    VOID
    );

VOID ShowCustomizeMenu(
    _In_ HWND WindowHandle
    );

// options.c

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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

// graph.c

typedef struct _PH_TOOLBAR_GRAPH
{
    struct _PH_PLUGIN *Plugin;

    ULONG Flags;
    ULONG GraphId;

    HWND GraphHandle;
    PVOID Context;
    PWSTR Text;

    PTOOLSTATUS_GRAPH_MESSAGE_CALLBACK MessageCallback;
    PH_GRAPH_STATE GraphState;
} PH_TOOLBAR_GRAPH, *PPH_TOOLBAR_GRAPH;

VOID ToolbarGraphLoadSettings(
    VOID
    );

VOID ToolbarGraphSaveSettings(
    VOID
    );

VOID ToolbarGraphsInitialize(
    VOID
    );

VOID ToolbarRegisterGraph(
    _In_ struct _PH_PLUGIN *Plugin,
    _In_ ULONG Id,
    _In_ PWSTR Text,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context,
    _In_opt_ PTOOLSTATUS_GRAPH_MESSAGE_CALLBACK MessageCallback
    );

VOID ToolbarCreateGraphs(
    VOID
    );

VOID ToolbarUpdateGraphs(
    VOID
    );

VOID ToolbarUpdateGraphVisualStates(
    VOID
    );

BOOLEAN ToolbarUpdateGraphsInfo(
    _In_ HWND WindowHandle,
    _In_ LPNMHDR Header
    );

VOID ToolbarSetVisibleGraph(
    _In_ PPH_TOOLBAR_GRAPH Icon,
    _In_ BOOLEAN Visible
    );

BOOLEAN ToolbarGraphsEnabled(
    VOID
    );

PPH_TOOLBAR_GRAPH ToolbarGraphFindById(
    _In_ ULONG SubId
    );

PPH_TOOLBAR_GRAPH ToolbarGraphFindByName(
    _In_ PPH_STRINGREF PluginName,
    _In_ ULONG SubId
    );

VOID ToolbarGraphCreateMenu(
    _In_ PPH_EMENU ParentMenu,
    _In_ ULONG MenuId
    );

VOID ToolbarGraphCreatePluginMenu(
    _In_ PPH_EMENU ParentMenu,
    _In_ ULONG MenuId
    );

// statusbar.c

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
    ID_STATUS_FREEMEMORY,
    ID_STATUS_SELECTEDWORKINGSET
} ID_STATUS;

VOID StatusBarShowCustomizeDialog(
    VOID
    );

// Shared by customizetb.c and customizesb.c

typedef struct _BUTTON_CONTEXT
{
    INT IdCommand;
    HICON IconHandle;

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
    HFONT FontHandle;
    HBRUSH BrushNormal;
    HBRUSH BrushPushed;
    HBRUSH BrushHot;
    COLORREF TextColor;
    INT CXWidth;
    INT ImageWidth;
    INT ImageHeight;

    HWND DialogHandle;
    HWND AvailableListHandle;
    HWND CurrentListHandle;
    HWND MoveUpButtonHandle;
    HWND MoveDownButtonHandle;
    HWND AddButtonHandle;
    HWND RemoveButtonHandle;
} CUSTOMIZE_CONTEXT, *PCUSTOMIZE_CONTEXT;

HICON CustomizeGetToolbarIcon(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT CommandID
    );

// searchbox.c

BOOLEAN CreateSearchboxControl(
    VOID
    );

// taskbar.c

typedef enum _PH_TASKBAR_ICON
{
    TASKBAR_ICON_NONE,
    TASKBAR_ICON_CPU_HISTORY,
    TASKBAR_ICON_IO_HISTORY,
    TASKBAR_ICON_COMMIT_HISTORY,
    TASKBAR_ICON_PHYSICAL_HISTORY,
    TASKBAR_ICON_CPU_USAGE,
} PH_TASKBAR_ICON;

extern PH_TASKBAR_ICON TaskbarListIconType;
extern BOOLEAN TaskbarIsDirty;

VOID NTAPI TaskbarUpdateGraphs(
    VOID
    );

#endif
