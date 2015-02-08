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
#include <phapppub.h>
#include <phplug.h>
#include <phappresource.h>
#include <windowsx.h>
#include <Wincodec.h>

#include "resource.h"

#define SETTING_PREFIX L"ProcessHacker.ToolStatus"
#define SETTING_NAME_ENABLE_TOOLBAR (SETTING_PREFIX L".EnableToolBar")
#define SETTING_NAME_ENABLE_SEARCHBOX (SETTING_PREFIX L".EnableSearchBox")
#define SETTING_NAME_ENABLE_STATUSBAR (SETTING_PREFIX L".EnableStatusBar")
#define SETTING_NAME_ENABLE_RESOLVEGHOSTWINDOWS (SETTING_PREFIX L".ResolveGhostWindows")
#define SETTING_NAME_ENABLE_STATUSMASK (SETTING_PREFIX L".StatusMask")
#define SETTING_NAME_ENABLE_TOOLBARDISPLAYSTYLE (SETTING_PREFIX L".ToolbarDisplayStyle")

#define MAX_DEFAULT_TOOLBAR_ITEMS 9

#define TIDC_FINDWINDOW (WM_APP + 1)
#define TIDC_FINDWINDOWTHREAD (WM_APP + 2)
#define TIDC_FINDWINDOWKILL (WM_APP + 3)
#define ID_SEARCH_CLEAR (WM_APP + 4)

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
    ToolbarDisplayImageOnly,
    ToolbarDisplaySelectiveText,
    ToolbarDisplayAllText
} TOOLBAR_DISPLAY_STYLE;

extern BOOLEAN EnableToolBar;
extern BOOLEAN EnableSearchBox;
extern BOOLEAN EnableStatusBar;
extern TOOLBAR_DISPLAY_STYLE DisplayStyle;
extern ULONG StatusMask;
extern ULONG ProcessesUpdatedCount;

extern HWND RebarHandle;
extern HWND ToolBarHandle;
extern HWND SearchboxHandle;
extern HWND StatusBarHandle;
extern HACCEL AcceleratorTable;
extern PPH_STRING SearchboxText;

extern TBBUTTON ToolbarButtons[10];
extern TBSAVEPARAMSW ToolbarSaveParams;

extern PPH_PLUGIN PluginInstance;
extern PPH_TN_FILTER_ENTRY ProcessTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY ServiceTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY NetworkTreeFilterEntry;

VOID UpdateStatusBar(
    VOID
    );

VOID ShowStatusMenu(
    _In_ PPOINT Point
    );

VOID LoadToolbarSettings(
    VOID
    );

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
    INT CXBorder;
    INT CYBorder;
    INT ImageWidth;
    INT ImageHeight;
    LONG cxImgSize;

    HWND WindowHandle;
    HFONT WindowFont;
    HIMAGELIST ImageList;

    HBRUSH BrushNormal;
    HBRUSH BrushFocused;
    HBRUSH BrushHot;
    HBRUSH BrushFill;
    COLORREF BackgroundColorRef;

    BOOLEAN MouseInClient;
} EDIT_CONTEXT, *PEDIT_CONTEXT;

HBITMAP LoadImageFromResources(
    _In_ UINT Width,
    _In_ UINT Height,
    _In_ PCWSTR Name
    );

#endif