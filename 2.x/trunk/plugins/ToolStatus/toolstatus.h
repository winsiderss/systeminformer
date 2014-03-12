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

#pragma comment(lib, "WindowsCodecs.lib")

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

extern BOOLEAN EnableToolBar;
extern BOOLEAN EnableSearchBox;
extern BOOLEAN EnableStatusBar;
extern TOOLBAR_DISPLAY_STYLE DisplayStyle;
extern HWND ReBarHandle;
extern HWND ToolBarHandle;
extern HWND TextboxHandle;
extern HACCEL AcceleratorTable;
extern PPH_STRING SearchboxText;
extern PPH_TN_FILTER_ENTRY ProcessTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY ServiceTreeFilterEntry;
extern PPH_TN_FILTER_ENTRY NetworkTreeFilterEntry;
extern PPH_PLUGIN PluginInstance;

extern HWND StatusBarHandle;
extern ULONG StatusMask;
extern ULONG ProcessesUpdatedCount;

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

BOOLEAN InsertButton(
    _In_ HWND WindowHandle,
    _In_ UINT CmdId
    );

typedef HRESULT (WINAPI *_GetThemeColor)(
    _In_ HTHEME hTheme,
    _In_ INT iPartId,
    _In_ INT iStateId,
    _In_ INT iPropId,
    _Out_ COLORREF *pColor
    );
typedef HRESULT (WINAPI *_SetWindowTheme)(
    _In_ HWND hwnd,
    _In_ LPCWSTR pszSubAppName,
    _In_ LPCWSTR pszSubIdList
    );

typedef HRESULT (WINAPI *_GetThemeFont)(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ INT iPartId,
    _In_ INT iStateId,
    _In_ INT iPropId,
    _Out_ LOGFONTW *pFont
    );

typedef HRESULT (WINAPI *_GetThemeSysFont)(
    _In_ HTHEME hTheme,
    _In_ INT iFontId,
    _Out_ LOGFONTW *plf
    );

typedef BOOL (WINAPI *_IsThemeBackgroundPartiallyTransparent)(
    _In_ HTHEME hTheme,
    _In_ INT iPartId,
    _In_ INT iStateId
    );

typedef HRESULT (WINAPI *_DrawThemeParentBackground)(
    _In_ HWND hwnd,
    _In_ HDC hdc,
    _In_opt_ const RECT* prc
    );

typedef HRESULT (WINAPI *_GetThemeBackgroundContentRect)(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ INT iPartId,
    _In_ INT iStateId,
    _Inout_ LPCRECT pBoundingRect,
    _Out_ LPRECT pContentRect
    );

typedef struct _NC_CONTEXT
{
    INT cxLeftEdge;
    INT cxRightEdge;
    INT cyTopEdge;
    INT cyBottomEdge;
    LONG cxImgSize;

    BOOL IsThemeActive;
    BOOL IsThemeBackgroundActive;
    COLORREF clrUxThemeFillRef;
    COLORREF clrUxThemeBackgroundRef;
   
    UINT CommandID;
    HFONT FontHandle;
    HIMAGELIST ImageList;
    HBITMAP ActiveBitmap;
    HBITMAP InactiveBitmap;

    HTHEME UxThemeHandle;
    HMODULE UxThemeModule;
} NC_CONTEXT;

HBITMAP LoadImageFromResources(
    _In_ UINT Width,
    _In_ UINT Height,
    _In_ LPCTSTR Name
    );

#endif