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

#define CINTERFACE
#define COBJMACROS
#define INITGUID
#include <phdk.h>
#include <phappresource.h>
#include <verify.h>
#include <mxml.h>
#include <windowsx.h>
#include <winhttp.h>

#include "resource.h"

#ifdef _DEBUG
// Force update checks to succeed (most of the below flags require this to be defined).
//#define FORCE_UPDATE_CHECK
// Force update check to show the current version as the latest version.
//#define FORCE_LATEST_VERSION
// Force the download error state.
//#define FORCE_DOWNLOAD_ERROR
// Hash and Signature errors can be combined.
//#define FORCE_HASH_CHECK_ERROR
//#define FORCE_SIGNATURE_CHECK_ERROR
// Forces the same result as having no internet connection.
//#define FORCE_NO_INTERNET
// Disable startup update check.
//#define DISABLE_STARTUP_CHECK
#endif

#define UPDATE_MENUITEM    1001
#define PH_UPDATEISERRORED (WM_APP + 501)
#define PH_UPDATEAVAILABLE (WM_APP + 502)
#define PH_UPDATEISCURRENT (WM_APP + 503)
#define PH_UPDATENEWER     (WM_APP + 504)
#define PH_UPDATESUCCESS   (WM_APP + 505)
#define PH_UPDATEFAILURE   (WM_APP + 506)
#define WM_SHOWDIALOG      (WM_APP + 550)

#define PLUGIN_NAME L"ProcessHacker.UpdateChecker"
#define SETTING_NAME_AUTO_CHECK (PLUGIN_NAME L".PromptStart")
#define SETTING_NAME_LAST_CHECK (PLUGIN_NAME L".LastUpdateCheckTime")

#define MAKEDLLVERULL(major, minor, build, revision) \
    (((ULONGLONG)(major) << 48) | \
    ((ULONGLONG)(minor) << 32) | \
    ((ULONGLONG)(build) << 16) | \
    ((ULONGLONG)(revision) <<  0))

extern HWND UpdateDialogHandle;
extern PH_EVENT InitializedEvent;
extern PPH_PLUGIN PluginInstance;

typedef struct _PH_UPDATER_CONTEXT
{
    BOOLEAN StartupCheck;
    BOOLEAN HaveData;
    BOOLEAN FixedWindowStyles;

    HICON IconSmallHandle;
    HICON IconLargeHandle;

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

VOID TaskDialogLinkClicked(
    _In_ PPH_UPDATER_CONTEXT Context
    );

NTSTATUS UpdateCheckThread(
    _In_ PVOID Parameter
    );

NTSTATUS UpdateDownloadThread(
    _In_ PVOID Parameter
    );

// page1.c
VOID ShowCheckForUpdatesDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context
    );

// page2.c
VOID ShowCheckingForUpdatesDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context
    );

// page3.c
VOID ShowAvailableDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context
    );

// page4.c
VOID ShowProgressDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context
    );

// page5.c

VOID ShowUpdateInstallDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context
    );

VOID ShowLatestVersionDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context
    );

VOID ShowNewerVersionDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context
    );

VOID ShowUpdateFailedDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context,
    _In_ BOOLEAN HashFailed,
    _In_ BOOLEAN SignatureFailed
    );

// updater.c

VOID ShowUpdateDialog(
    _In_opt_ PPH_UPDATER_CONTEXT Context
    );

VOID StartInitialCheck(
    VOID
    );

BOOLEAN UpdaterInstalledUsingSetup(
    VOID
    );

// options.c

VOID ShowOptionsDialog(
    _In_opt_ HWND Parent
    );

#endif