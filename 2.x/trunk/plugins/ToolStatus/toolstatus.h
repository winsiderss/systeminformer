/*
 * Process Hacker ToolStatus -
 *   toolstatus header
 *
 * Copyright (C) 2010-2012 wj32
 * Copyright (C) 2011-2012 dmex
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

#ifndef TOOLSTATUS_H
#define TOOLSTATUS_H

#include <phdk.h>

#include "resource.h"
#include "btncontrol.h"

#include <phapppub.h>
#include <phplug.h>
#include <phappresource.h>
#include <windowsx.h>

#define TIDC_FINDWINDOW (WM_USER + 1)
#define TIDC_FINDWINDOWTHREAD (WM_USER + 2)
#define TIDC_FINDWINDOWKILL (WM_USER + 3)

#define STATUS_COUNT 10
#define STATUS_MINIMUM 0x1
#define STATUS_CPUUSAGE 0x1
#define STATUS_COMMIT 0x2
#define STATUS_PHYSICAL 0x4
#define STATUS_NUMBEROFPROCESSES 0x8
#define STATUS_NUMBEROFTHREADS 0x10
#define STATUS_NUMBEROFHANDLES 0x20
#define STATUS_IOREADOTHER 0x40
#define STATUS_IOWRITE 0x80
#define STATUS_MAXCPUPROCESS 0x100
#define STATUS_MAXIOPROCESS 0x200
#define STATUS_MAXIMUM 0x400

typedef enum _TOOLBAR_DISPLAY_STYLE
{
    ImageOnly = 0,
    SelectiveText = 1,
    AllText = 2
} TOOLBAR_DISPLAY_STYLE;

BOOLEAN EnableToolBar;
BOOLEAN EnableStatusBar;
BOOLEAN EnableSearch;
TOOLBAR_DISPLAY_STYLE DisplayStyle;
HACCEL AcceleratorTable;

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION LayoutPaddingCallbackRegistration;
PH_CALLBACK_REGISTRATION TabPageCallbackRegistration;

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

// Statusbar.c
HWND StatusBarHandle;
RECT StatusBarRect;
ULONG StatusMask;
ULONG ProcessesUpdatedCount;
ULONG StatusBarMaxWidths[STATUS_COUNT];

VOID StatusBarCreate(
    __in HWND ParentHandle
    );
VOID UpdateStatusBar(
    VOID
    );
VOID ShowStatusMenu(
    __in PPOINT Point
    );

// Rebar.c
HWND ReBarHandle;
RECT ReBarRect;

VOID RebarCreate(
    __in HWND ParentHandle
    );
VOID RebarAddMenuItem(
    __in HWND WindowHandle,
    __in HWND ChildHandle,
    __in UINT ID,
    __in UINT cyMinChild,
    __in UINT cxMinChild
    );

// Toolbar.c
#define ID_SEARCH_CLEAR (WM_USER + 1)

HWND ToolBarHandle;
HIMAGELIST ToolBarImageList;
HWND TextboxHandle;
HFONT TextboxFontHandle;

VOID ToolBarCreate(
    __in HWND ParentHandle
    );
VOID ToolbarCreateSearch(
    __in HWND ParentHandle
    );
VOID ToolBarCreateImageList(
    __in HWND WindowHandle
    );
VOID ToolbarAddMenuItems(
    __in HWND WindowHandle
    );

// searchfilter.c
BOOLEAN ProcessTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    );
BOOLEAN ServiceTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    );
BOOLEAN NetworkTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    );
#endif
