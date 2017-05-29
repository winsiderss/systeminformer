/*
 * Process Hacker Toolchain -
 *   project setup
 *
 * Copyright (C) 2017 dmex
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

#ifndef _SETUP_H
#define _SETUP_H

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "WindowsCodecs.lib")

#define CINTERFACE
#define COBJMACROS
#include <ph.h>
#include <guisup.h>
#include <prsht.h>
#include <appsup.h>

#include <aclapi.h>
#include <WindowsX.h>
#include <Wincodec.h>
#include <uxtheme.h>
#include <Shlwapi.h>
#include <shlobj.h>
#include <sddl.h>

#include "resource.h"

// Version Information
#include "..\..\ProcessHacker\include\phappres.h"

// Win32 PropertySheet Control IDs
#define IDD_PROPSHEET_ID            1006  // ID of the propsheet dialog template in comctl32.dll
#define IDC_PROPSHEET_CANCEL        0x0002
#define IDC_PROPSHEET_APPLYNOW      0x3021
#define IDC_PROPSHEET_DLGFRAME      0x3022
#define IDC_PROPSHEET_BACK          0x3023
#define IDC_PROPSHEET_NEXT          0x3024
#define IDC_PROPSHEET_FINISH        0x3025
#define IDC_PROPSHEET_DIVIDER       0x3026
#define IDC_PROPSHEET_TOPDIVIDER    0x3027

//
#define WM_START_SETUP (WM_APP + 1)
#define WM_UPDATE_SETUP (WM_APP + 2)
#define WM_END_SETUP (WM_APP + 3)

extern HANDLE MutantHandle;

typedef enum _SETUP_COMMAND_TYPE
{
    SETUP_COMMAND_INSTALL,
    SETUP_COMMAND_UNINSTALL,
    SETUP_COMMAND_UPDATE,
    SETUP_COMMAND_REPAIR,
} SETUP_COMMAND_TYPE;

typedef struct _PH_SETUP_CONTEXT
{
    HWND DialogHandle;
    HWND PropSheetBackHandle;
    HWND PropSheetForwardHandle;
    HWND PropSheetCancelHandle;

    HWND WelcomePageHandle;
    HWND EulaPageHandle;
    HWND ConfigPageHandle;
    HWND DownloadPageHandle;
    HWND ExtractPageHandle;
    HWND FinalPageHandle;
    HWND ErrorPageHandle;

    HICON IconSmallHandle;
    HICON IconLargeHandle;

    PPH_STRING SetupInstallPath;
    BOOLEAN SetupCreateDesktopShortcut;
    BOOLEAN SetupCreateDesktopShortcutAllUsers;
    BOOLEAN SetupCreateDefaultTaskManager;
    BOOLEAN SetupCreateSystemStartup;
    BOOLEAN SetupCreateMinimizedSystemStartup;
    BOOLEAN SetupInstallDebuggingTools;
    BOOLEAN SetupInstallPeViewAssociations;
    BOOLEAN SetupInstallKphService;
    BOOLEAN SetupResetSettings;
    BOOLEAN SetupStartAppAfterExit;

    ULONG ErrorCode;
    PPH_STRING Version;
    PPH_STRING RevVersion;
    PPH_STRING RelDate;
    PPH_STRING Size;
    PPH_STRING Hash;
    PPH_STRING Signature;
    PPH_STRING ReleaseNotesUrl;

    PPH_STRING BinFileDownloadUrl;
    //PPH_STRING SetupFileDownloadUrl;
    PPH_STRING SetupFilePath;

    HWND MainHeaderHandle;
    HWND StatusHandle;
    HWND SubStatusHandle;
    HWND ProgressHandle;

    BOOLEAN SetupRunning;
    ULONG CurrentMajorVersion;
    ULONG CurrentMinorVersion;
    ULONG CurrentRevisionVersion;
    ULONG LatestMajorVersion;
    ULONG LatestMinorVersion;
    ULONG LatestRevisionVersion;
} PH_SETUP_CONTEXT, *PPH_SETUP_CONTEXT;

VOID SetupLoadImage(
    _In_ HWND WindowHandle,
    _In_ PWSTR Name
    );

INT_PTR CALLBACK SetupPropPage1_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    );

INT_PTR CALLBACK SetupPropPage2_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    );

INT_PTR CALLBACK SetupPropPage3_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    );

INT_PTR CALLBACK SetupPropPage4_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    );

INT_PTR CALLBACK SetupPropPage5_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    );

INT_PTR CALLBACK SetupErrorPage_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    );

// page4.c

extern ULONG64 ExtractCurrentLength;
extern ULONG64 ExtractTotalLength;

// setup.c

VOID SetupStartKph(
    _In_ PPH_SETUP_CONTEXT Context
    );

BOOLEAN SetupUninstallKph(
    _In_ PPH_SETUP_CONTEXT Context
    );

NTSTATUS SetupCreateUninstallKey(
    _In_ PPH_SETUP_CONTEXT Context
    );

NTSTATUS SetupDeleteUninstallKey(
    VOID
    );

VOID SetupSetWindowsOptions(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID SetupDeleteWindowsOptions(
    _In_ PPH_SETUP_CONTEXT Context
    );

BOOLEAN SetupCreateUninstallFile(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID SetupDeleteUninstallFile(
    _In_ PPH_SETUP_CONTEXT Context
    );

BOOLEAN SetupExecuteProcessHacker(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID SetupUpgradeSettingsFile(
    VOID
    );

// download.c

BOOLEAN SetupQueryUpdateData(
    _In_ PPH_SETUP_CONTEXT Context
    );

BOOLEAN UpdateDownloadUpdateData(
    _In_ PPH_SETUP_CONTEXT Context
    );

// extract.c

BOOLEAN SetupExtractBuild(
    _In_ PPH_SETUP_CONTEXT Context
    );

 // update.c

VOID SetupShowUpdateDialog(
    VOID
    );

// uninstall.c

VOID SetupShowUninstallDialog(
    VOID
    );

#endif