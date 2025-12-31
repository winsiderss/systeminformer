/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2011-2023
 *
 */

#ifndef _TOOLSTATUS_H
#define _TOOLSTATUS_H

#include <phdk.h>
#include <phapppub.h>
#include <phappresource.h>
#include <phconsole.h>
#include <hndlinfo.h>
#include <settings.h>
#include <searchbox.h>

#include <toolstatusintf.h>

#include "resource.h"

#define PLUGIN_NAME L"ToolStatus"
#define SETTING_NAME_TOOLSTATUS_CONFIG (PLUGIN_NAME L".Config")
#define SETTING_NAME_REBAR_CONFIG (PLUGIN_NAME L".RebarConfig")
#define SETTING_NAME_TOOLBAR_CONFIG (PLUGIN_NAME L".ToolbarButtonConfig")
#define SETTING_NAME_TOOLBAR_GRAPH_CONFIG (PLUGIN_NAME L".ToolbarGraphConfig")
#define SETTING_NAME_STATUSBAR_CONFIG (PLUGIN_NAME L".StatusbarConfig")
#define SETTING_NAME_DELAYED_INITIALIZATION_MAX (PLUGIN_NAME L".DelayConfig")
#define SETTING_NAME_TOOLBARDISPLAYSTYLE (PLUGIN_NAME L".ToolbarDisplayStyle")
#define SETTING_NAME_SEARCHBOXDISPLAYMODE (PLUGIN_NAME L".SearchBoxDisplayMode")
#define SETTING_NAME_TASKBARDISPLAYSTYLE (PLUGIN_NAME L".TaskbarDisplayStyle")
#define SETTING_NAME_SHOWSYSINFOGRAPH (PLUGIN_NAME L".ToolbarShowSystemInfoGraph")
#define SETTING_NAME_RESTOREROWAFTERSEARCH (PLUGIN_NAME L".RestoreSelectionAfterSearch")

#define MAX_DEFAULT_TOOLBAR_ITEMS 11
#define MAX_DEFAULT_STATUSBAR_ITEMS 3
#define MAX_DEFAULT_IMAGELIST_ITEMS 1
#define MAX_TOOLBAR_ITEMS 13
#define MAX_STATUSBAR_ITEMS 17

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
        ULONG ToolBarLargeIcons : 1;
        ULONG Spare : 19;
    };
} TOOLSTATUS_CONFIG;

extern TOOLSTATUS_CONFIG ToolStatusConfig;
extern HWND ProcessTreeNewHandle;
extern HWND ServiceTreeNewHandle;
extern HWND NetworkTreeNewHandle;
extern INT SelectedTabIndex;
extern BOOLEAN UpdateAutomatically;
extern BOOLEAN UpdateGraphs;
extern BOOLEAN EnableThemeSupport;
extern BOOLEAN EnableAvxSupport;
extern BOOLEAN EnableGraphMaxScale;
extern TOOLBAR_DISPLAY_STYLE DisplayStyle;
extern SEARCHBOX_DISPLAY_MODE SearchBoxDisplayMode;
extern REBAR_DISPLAY_LOCATION RebarDisplayLocation;

extern HWND RebarHandle;
extern HWND ToolBarHandle;
extern HWND SearchboxHandle;
extern HWND MainWindowHandle;
extern HMENU MainMenu;
extern HACCEL AcceleratorTable;
extern ULONG_PTR SearchMatchHandle;
extern PH_PLUGIN_SYSTEM_STATISTICS SystemStatistics;

extern SIZE ToolBarImageSize;
extern HIMAGELIST ToolBarImageList;
extern TBBUTTON ToolbarButtons[MAX_TOOLBAR_ITEMS];
extern HFONT ToolbarWindowFont;
extern BOOLEAN ToolbarInitialized;

extern PPH_PLUGIN PluginInstance;
extern PPH_TN_FILTER_ENTRY ProcessTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY ServiceTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY NetworkTreeFilterEntry;

// plugin.cpp

extern CONST TOOLSTATUS_INTERFACE PluginInterface;

VOID PluginInterfaceInitialize(
    VOID
    );

VOID PluginInterfaceInvokeSearchChangedEvent(
    _In_ ULONG_PTR Context
    );

HWND GetTabIndexTreeNewHandle(
    _In_ LONG TabIndex
    );

PPH_STRING GetTabIndexBannerText(
    _In_ LONG TabIndex,
    _In_opt_ PCPH_STRINGREF AppendString
    );

// toolbar.c

BOOLEAN RebarBandInsert(
    _In_ REBAR_BAND BandID,
    _In_ HWND ChildWindowHandle,
    _In_ ULONG ChildMinimumWidth,
    _In_ ULONG ChildMinimumHeight
    );

VOID RebarBandRemove(
    _In_ REBAR_BAND BandID
    );

BOOLEAN RebarBandMove(
    _In_ ULONG OldIndex,
    _In_ ULONG NewIndex
    );

ULONG RebarBandToIndex(
    _In_ REBAR_BAND BandID
    );

FORCEINLINE
BOOLEAN
RebarBandExists(
    _In_ REBAR_BAND BandID
    )
{
    return RebarBandToIndex(BandID) != ULONG_MAX;
}

_Success_(return)
BOOLEAN RebarGetBandCount(
    _Out_ PULONG Count
    );

LONG RebarGetRowHeight(
    _In_ REBAR_BAND BandID
    );

VOID RebarSetBarInfo(
    VOID
    );

_Success_(return)
BOOLEAN RebarGetBandIndexStyle(
    _In_ ULONG BandIndex,
    _Out_ PULONG Style
    );

BOOLEAN RebarSetBandIndexStyle(
    _In_ ULONG BandIndex,
    _In_ ULONG Style
    );

typedef struct _BAND_CHILD_SIZE
{
    ULONG InitialChildHeight;
    ULONG MaximumChildHeight;
    ULONG MinChildWidth;
    ULONG MinChildHeight;
    ULONG ResizeStepValue;
} BAND_CHILD_SIZE, * PBAND_CHILD_SIZE;

_Success_(return)
BOOLEAN RebarGetBandIndexChildSize(
    _In_ ULONG BandIndex,
    _Out_ PBAND_CHILD_SIZE BandSize
    );

BOOLEAN RebarSetBandIndexChildSize(
    _In_ ULONG BandIndex,
    _In_ PBAND_CHILD_SIZE BandSize
    );

typedef struct _BAND_STYLE_SIZE
{
    ULONG BandStyle;
    ULONG BandWidth;
} BAND_STYLE_SIZE, *PBAND_STYLE_SIZE;

_Success_(return)
BOOLEAN RebarGetBandIndexStyleSize(
    _In_ ULONG BandIndex,
    _Out_ PBAND_STYLE_SIZE BandStyleSize
    );

BOOLEAN RebarSetBandIndexStyleSize(
    _In_ ULONG BandIndex,
    _In_ PBAND_STYLE_SIZE RebarBandInfo
    );

VOID ToolbarLoadSettings(
    _In_ BOOLEAN DpiChanged
    );

VOID ToolbarRemoveButtons(
    VOID
    );

VOID ToolbarResetSettings(
    VOID
    );

PWSTR ToolbarGetText(
    _In_ UINT CommandID
    );

HBITMAP ToolbarGetImage(
    _In_ UINT CommandID,
    _In_ LONG DpiValue
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

VOID InvalidateMainWindowLayout(
    VOID
    );

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI SearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    );

// options.c

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// filter.c

BOOLEAN SearchWordMatchStringRefCallback(
    _In_ PCPH_STRINGREF Text
    );

_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN ProcessTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN ServiceTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN NetworkTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

// graph.c

VOID ToolbarGraphLoadSettings(
    VOID
    );

VOID ToolbarGraphSaveSettings(
    VOID
    );

VOID ToolbarGraphsInitialize(
    VOID
    );

VOID ToolbarGraphsInitializeDpi(
    VOID
    );

VOID ToolbarRegisterGraph(
    _In_ PPH_PLUGIN Plugin,
    _In_ ULONG Id,
    _In_ PWSTR Text,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context,
    _In_ PTOOLSTATUS_GRAPH_CALLBACK GraphCallback
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

VOID ToolbarUpdateVisibleGraph(
    _In_ PVOID Context
    );

typedef struct _PH_TOOLBAR_GRAPH PH_TOOLBAR_GRAPH, *PPH_TOOLBAR_GRAPH;

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
    _In_ ULONG GraphId
    );

PPH_TOOLBAR_GRAPH ToolbarGraphFindByName(
    _In_ PCPH_STRINGREF PluginName,
    _In_ ULONG GraphId
    );

VOID ToolbarGraphCreateMenu(
    _In_ PPH_EMENU ParentMenu,
    _In_ ULONG MenuId
    );

VOID ToolbarGraphCreatePluginMenu(
    _In_ PPH_EMENU ParentMenu,
    _In_ ULONG MenuId
    );

_Function_class_(TOOLSTATUS_GRAPH_CALLBACK)
BOOLEAN CpuHistoryGraphMessageCallback(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    );

_Function_class_(TOOLSTATUS_GRAPH_CALLBACK)
BOOLEAN PhysicalHistoryGraphMessageCallback(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    );

_Function_class_(TOOLSTATUS_GRAPH_CALLBACK)
BOOLEAN CommitHistoryGraphMessageCallback(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    );

_Function_class_(TOOLSTATUS_GRAPH_CALLBACK)
BOOLEAN IoHistoryGraphMessageCallback(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    );

// statusbar.c

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
    _In_ HWND WindowHandle
    );

// customizetb.c

VOID ToolBarShowCustomizeDialog(
    _In_ HWND ParentWindowHandle
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
    ID_STATUS_SELECTEDWORKINGSET,
    ID_STATUS_SELECTEDPRIVATEBYTES,
    ID_STATUS_KSICOUNTER,
} ID_STATUS;

VOID StatusBarShowCustomizeDialog(
    _In_ HWND ParentWindowHandle
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

    LONG WindowDpi;
    INT CXWidth;
    INT ImageWidth;
    INT ImageHeight;

    HWND WindowHandle;
    HWND AvailableListHandle;
    HWND CurrentListHandle;
    HWND MoveUpButtonHandle;
    HWND MoveDownButtonHandle;
    HWND AddButtonHandle;
    HWND RemoveButtonHandle;
} CUSTOMIZE_CONTEXT, *PCUSTOMIZE_CONTEXT;

HICON CustomizeGetToolbarIcon(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT CommandID,
    _In_ LONG DpiValue
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
extern BOOLEAN TaskbarMainWndExiting;

VOID NTAPI TaskbarInitialize(
    VOID
    );

VOID NTAPI TaskbarUpdateEvents(
    VOID
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS TaskbarIconUpdateThread(
    _In_opt_ PVOID Context
    );

HICON PhUpdateIconCpuHistory(
    _In_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    );

HICON PhUpdateIconIoHistory(
    _In_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    );

HICON PhUpdateIconCommitHistory(
    _In_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    );

HICON PhUpdateIconPhysicalHistory(
    _In_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    );

HICON PhUpdateIconCpuUsage(
    _In_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    );

#endif
