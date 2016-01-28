/*
 * Process Hacker Plugins -
 *   Update Checker Plugin
 *
 * Copyright (C) 2011-2015 dmex
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

#ifndef __UPDATER_H__
#define __UPDATER_H__

#pragma comment(lib, "Winhttp.lib")

#define CINTERFACE
#define COBJMACROS
#define INITGUID
#include <phdk.h>
#include <phappresource.h>
#include <verify.h>
#include <mxml.h>
#include <windowsx.h>
#include <netlistmgr.h>
#include <winhttp.h>
#include <Wincodec.h>

#include "resource.h"

// Force update checks to succeed with debug builds
//#define DEBUG_UPDATE
#define UPDATE_MENUITEM    1001
#define PH_UPDATEISERRORED (WM_APP + 101)
#define PH_UPDATEAVAILABLE (WM_APP + 102)
#define PH_UPDATEISCURRENT (WM_APP + 103)
#define PH_UPDATENEWER     (WM_APP + 104)
#define PH_UPDATESUCCESS   (WM_APP + 105)
#define PH_UPDATEFAILURE   (WM_APP + 106)
#define WM_SHOWDIALOG      (WM_APP + 150)

#define PLUGIN_NAME L"ProcessHacker.UpdateChecker"
#define SETTING_NAME_AUTO_CHECK (PLUGIN_NAME L".PromptStart")
#define SETTING_NAME_LAST_CHECK (PLUGIN_NAME L".LastUpdateCheckTime")

#define MAKEDLLVERULL(major, minor, build, revision) \
    (((ULONGLONG)(major) << 48) | \
    ((ULONGLONG)(minor) << 32) | \
    ((ULONGLONG)(build) << 16) | \
    ((ULONGLONG)(revision) <<  0))

#define Control_Visible(hWnd, visible) \
    ShowWindow(hWnd, visible ? SW_SHOW : SW_HIDE);

extern PPH_PLUGIN PluginInstance;

typedef enum _PH_UPDATER_STATE
{
    PhUpdateDefault = 0,
    PhUpdateDownload = 1,
    PhUpdateInstall = 2,
    PhUpdateMaximum = 3
} PH_UPDATER_STATE;

typedef struct _PH_UPDATER_CONTEXT
{
    BOOLEAN HaveData;
    PH_UPDATER_STATE UpdaterState;
    HBITMAP IconBitmap;
    HICON IconHandle;
    HFONT FontHandle;
    HWND StatusHandle;
    HWND ProgressHandle;
    HWND DialogHandle;

    ULONG MinorVersion;
    ULONG MajorVersion;
    ULONG RevisionVersion;
    ULONG CurrentMinorVersion;
    ULONG CurrentMajorVersion;
    ULONG CurrentRevisionVersion;
    PPH_STRING Version;
    PPH_STRING RevVersion;
    PPH_STRING RelDate;
    PPH_STRING Size;
    PPH_STRING Hash;
    PPH_STRING ReleaseNotesUrl;
    PPH_STRING SetupFileDownloadUrl;
    PPH_STRING SetupFilePath;
} PH_UPDATER_CONTEXT, *PPH_UPDATER_CONTEXT;

VOID ShowUpdateDialog(
    _In_opt_ PPH_UPDATER_CONTEXT Context
    );

VOID StartInitialCheck(
    VOID
    );

PPH_STRING PhGetOpaqueXmlNodeText(
    _In_ mxml_node_t *xmlNode
    );

BOOL PhInstalledUsingSetup(
    VOID
    );

INT_PTR CALLBACK UpdaterWndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif