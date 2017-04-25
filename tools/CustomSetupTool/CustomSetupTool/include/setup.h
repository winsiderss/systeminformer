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
#include <aclapi.h>
#include <WindowsX.h>
#include <Wincodec.h>
#include <uxtheme.h>
#include <Shlwapi.h>
#include <shlobj.h>
#include <sddl.h>

#include "resource.h"
#include <appsup.h>

#include "..\..\..\ProcessHacker\include\phappres.h"
#include "..\..\..\ProcessHacker\include\phapprev.h"

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
extern PPH_STRING SetupInstallPath;
extern BOOLEAN SetupCreateDesktopShortcut;
extern BOOLEAN SetupCreateDesktopShortcutAllUsers;
extern BOOLEAN SetupCreateDefaultTaskManager;
extern BOOLEAN SetupCreateSystemStartup;
extern BOOLEAN SetupCreateMinimizedSystemStartup;
extern BOOLEAN SetupInstallDebuggingTools;
extern BOOLEAN SetupInstallPeViewAssociations;
extern BOOLEAN SetupInstallKphService;
extern BOOLEAN SetupResetSettings;
extern BOOLEAN SetupStartAppAfterExit;

typedef enum _SETUP_COMMAND_TYPE
{
    SETUP_COMMAND_INSTALL,
    SETUP_COMMAND_UNINSTALL,
    SETUP_COMMAND_UPDATE,
    SETUP_COMMAND_REPAIR,
} SETUP_COMMAND_TYPE;

typedef struct _SETUP_INSTANCE
{
    ULONG Version;
    HWND WindowHandle;
} SETUP_INSTANCE, *PSETUP_INSTANCE;

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

// page4.c

typedef struct _SETUP_PROGRESS_THREAD
{
    HWND DialogHandle;
    HWND PropSheetHandle;
    HICON PropSheetIcon;
} SETUP_PROGRESS_THREAD, *PSETUP_PROGRESS_THREAD;

extern BOOLEAN SetupRunning;
extern ULONG64 ExtractCurrentLength;
extern ULONG64 ExtractTotalLength;

// setup.c

VOID SetupInstallKph(
    VOID
    );

ULONG SetupUninstallKph(
    VOID
    );

NTSTATUS SetupCreateUninstallKey(
    VOID
    );

NTSTATUS SetupDeleteUninstallKey(
    VOID
    );

VOID SetupSetWindowsOptions(
    VOID
    );

VOID SetupDeleteWindowsOptions(
    VOID
    );

VOID SetupCreateUninstallFile(
    VOID
    );

BOOLEAN SetupExecuteProcessHacker(
    _In_ HWND Parent
    );

VOID SetupUpgradeSettingsFile(
    VOID
    );

// extract.c

BOOLEAN SetupExtractBuild(
    _In_ PSETUP_PROGRESS_THREAD Context
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