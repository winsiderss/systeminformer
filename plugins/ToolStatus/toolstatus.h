/*
 * Process Hacker ToolStatus -
 *   toolstatus header
 *
 * Copyright (C) 2011-2013 dmex
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

#ifndef TOOLSTATUS_H
#define TOOLSTATUS_H

#define CINTERFACE
#define COBJMACROS
#define INITGUID
#include <phdk.h>
#include <phapppub.h>
#include <phplug.h>
#include <phappresource.h>
#include <windowsx.h>

#include "resource.h"

typedef enum _TOOLBAR_DISPLAY_STYLE
{
    ImageOnly = 0,
    SelectiveText = 1,
    AllText = 2
} TOOLBAR_DISPLAY_STYLE;

#define IDC_MENU_REBAR 55400
#define IDC_MENU_REBAR_TOOLBAR 55401
#define IDC_MENU_REBAR_SEARCH 55402
#define ID_SEARCH_CLEAR (WM_USER + 1)

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

extern PPH_PLUGIN PluginInstance;
extern PPH_TN_FILTER_ENTRY ProcessTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY ServiceTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY NetworkTreeFilterEntry;
extern HACCEL AcceleratorTable;
extern HWND ReBarHandle;
extern HWND ToolBarHandle;
extern HWND TextboxHandle;
extern HFONT TextboxFontHandle;
extern HIMAGELIST ToolBarImageList;
extern BOOLEAN EnableToolBar;
extern BOOLEAN EnableStatusBar;
extern BOOLEAN EnableSearch;
extern TOOLBAR_DISPLAY_STYLE DisplayStyle;
extern PH_LAYOUT_MANAGER LayoutManager;  

extern HWND StatusBarHandle;
extern ULONG StatusMask;
extern ULONG ProcessesUpdatedCount;

VOID UpdateStatusBar(
    VOID
    );
VOID ShowStatusMenu(
    __in PPOINT Point
    );

VOID ApplyToolbarSettings(
    VOID
    );

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

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

HBITMAP LoadImageFromResources(
    __in LPCTSTR Name,
    __in LPCTSTR Type
    );

HBITMAP LoadScaledImageFromResources(  
    __in UINT Width,
    __in UINT Height,
    __in LPCTSTR Name,
    __in LPCTSTR Type
    );

BOOLEAN InsertButton(
    __in HWND WindowHandle,
    __in UINT CmdId
    );

typedef HRESULT (WINAPI *_GetThemeColor)(
    __in HTHEME hTheme,
    __in INT iPartId,
    __in INT iStateId,
    __in INT iPropId,
    __out COLORREF *pColor
    );
typedef HRESULT (WINAPI *_SetWindowTheme)(
    __in HWND hwnd,
    __in LPCWSTR pszSubAppName,
    __in LPCWSTR pszSubIdList
    );

typedef HRESULT (WINAPI *_GetThemeFont)(
    __in HTHEME hTheme,
    __in HDC hdc,
    __in INT iPartId,
    __in INT iStateId,
    __in INT iPropId,
    __out LOGFONTW *pFont
    );

typedef HRESULT (WINAPI *_GetThemeSysFont)(
    __in HTHEME hTheme,
    __in INT iFontId,
    __out LOGFONTW *plf
    );

typedef BOOL (WINAPI *_IsThemeBackgroundPartiallyTransparent)(
    __in HTHEME hTheme,
    __in INT iPartId,
    __in INT iStateId
    );

typedef HRESULT (WINAPI *_GetThemeBackgroundContentRect)(
    __in HTHEME hTheme,
    __in HDC hdc,
    __in INT iPartId,
    __in INT iStateId,
    __inout LPCRECT pBoundingRect,
    __out LPRECT pContentRect
    );

typedef struct _NC_CONTEXT
{
    UINT CommandID; // sent in a WM_COMMAND message

    INT cxLeftEdge; // size of the current window borders.
    INT cxRightEdge;  // size of the current window borders.
    INT cyTopEdge;
    INT cyBottomEdge;

    SIZE ImgSize;
    BOOLEAN HasCapture;
    BOOL IsThemeActive;
    BOOL IsThemeBackgroundActive;
    HTHEME UxThemeHandle;
    HMODULE UxThemeModule;
    COLORREF clrUxThemeFillRef;
    COLORREF clrUxThemeBackgroundRef;

    HIMAGELIST ImageList;
    HWND ParentWindow;
    HWND HwndWindow;
} NC_CONTEXT;

#endif