/*
 * Process Hacker Extra Plugins -
 *   Debug View Plugin
 *
 * Copyright (C) 2015 dmex
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

#ifndef _DBG_H_
#define _DBG_H_

#define SETTING_PREFIX L"dmex.DbgView"
#define SETTING_NAME_WINDOW_POSITION (SETTING_PREFIX L".WindowPosition")
#define SETTING_NAME_WINDOW_SIZE (SETTING_PREFIX L".WindowSize")
#define SETTING_NAME_COLUMNS (SETTING_PREFIX L".WindowColumns")
#define SETTING_NAME_ALWAYSONTOP (SETTING_PREFIX L".AlwaysOnTop")
#define SETTING_NAME_AUTOSCROLL (SETTING_PREFIX L".AutoScroll")
#define SETTING_NAME_MAX_ENTRIES (SETTING_PREFIX L".MaxEntries")

#define DBGVIEW_MENUITEM     1000
#define WM_SHOWDIALOG        (WM_APP + 100)
#define WM_DEBUG_LOG_UPDATED (WM_APP + 101)

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>
#include <colmgr.h>

#include <windowsx.h>
#include <Sddl.h>

#include "resource.h"

#define DBWIN_BUFFER_READY L"DBWIN_BUFFER_READY"
#define DBWIN_DATA_READY L"DBWIN_DATA_READY"
#define DBWIN_BUFFER L"DBWIN_BUFFER"

#define DBWIN_BUFFER_READY_OBJECT_NAME L"\\BaseNamedObjects\\DBWIN_BUFFER_READY"
#define DBWIN_DATA_READY_OBJECT_NAME L"\\BaseNamedObjects\\DBWIN_DATA_READY"
#define DBWIN_BUFFER_SECTION_NAME L"\\BaseNamedObjects\\DBWIN_BUFFER"

// The Win32 OutputDebugString buffer.
typedef struct _DBWIN_PAGE_BUFFER
{
    ULONG ProcessId; /** The ID of the process. */
    CHAR Buffer[PAGE_SIZE - sizeof(ULONG)]; /** The buffer containing the debug message. */
} DBWIN_PAGE_BUFFER, *PDBWIN_PAGE_BUFFER;

extern PPH_PLUGIN PluginInstance;
extern PH_CALLBACK DbgLoggedCallback;

typedef enum _FILTER_BY_TYPE
{
    FilterByUnknown,
    FilterByPid,
    FilterByName
} FILTER_BY_TYPE;

typedef struct _DBG_FILTER_TYPE
{
    FILTER_BY_TYPE Type;
    HANDLE ProcessId;
    PPH_STRING ProcessName;
} DBG_FILTER_TYPE, *PDBG_FILTER_TYPE;

typedef enum _COMMAND_ID
{
    ID_CAPTURE_WIN32 = 1,
    ID_CAPTURE_WIN32_GLOBAL,
    ID_RESET_FILTERS,
    ID_CLEAR_EVENTS,
    ID_SAVE_EVENTS,
} COMMAND_ID;

typedef struct _PH_DBGEVENTS_CONTEXT
{
    BOOLEAN AlwaysOnTop;
    BOOLEAN AutoScroll;
    BOOLEAN CaptureLocalEnabled;
    BOOLEAN CaptureGlobalEnabled;

    ULONG ListViewCount;

    HWND DialogHandle;
    HWND ListViewHandle;
    HWND AutoScrollHandle;
    HWND OptionsHandle;
    HIMAGELIST ListViewImageList;

    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION DebugLoggedRegistration;
    SECURITY_ATTRIBUTES SecurityAttributes;

    PPH_LIST ExcludeList;
    PPH_LIST LogMessageList;

    HANDLE LocalBufferReadyEvent;
    HANDLE LocalDataReadyEvent;
    HANDLE LocalDataBufferHandle;
    PDBWIN_PAGE_BUFFER LocalDebugBuffer;
    HANDLE GlobalBufferReadyEvent;
    HANDLE GlobalDataReadyEvent;
    HANDLE GlobalDataBufferHandle;
    PDBWIN_PAGE_BUFFER GlobalDebugBuffer;
} PH_DBGEVENTS_CONTEXT, *PPH_DBGEVENTS_CONTEXT;

typedef struct _DEBUG_LOG_ENTRY
{
    INT ImageIndex;
    LARGE_INTEGER Time;
    HANDLE ProcessId;
    PPH_STRING Message;
    PPH_STRING ProcessName;
    PPH_STRING FilePath;
} DEBUG_LOG_ENTRY, *PDEBUG_LOG_ENTRY;

INT_PTR CALLBACK DbgPropDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID ShowDebugEventsDialog(
    VOID
    );

// log.c

VOID DbgClearLogEntries(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context
    );

BOOLEAN DbgCreateSecurityAttributes(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context
    );

BOOLEAN DbgCleanupSecurityAttributes(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context
    );

BOOLEAN DbgEventsCreate(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context,
    _In_ BOOLEAN GlobalEvents
    );

VOID DbgEventsCleanup(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context,
    _In_ BOOLEAN CleanupGlobal
    );

// Filter.c

VOID AddFilterType(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context,
    _In_ FILTER_BY_TYPE Type,
    _In_ HANDLE ProcessID,
    _In_ PPH_STRING ProcessName
    );

VOID ResetFilters(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context
    );

#endif