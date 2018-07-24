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

#include <ph.h>
#include <guisup.h>
#include <prsht.h>
#include <workqueue.h>
#include <setupsup.h>
#include <json.h>

#include <aclapi.h>
#include <wincodec.h>
#include <uxtheme.h>
#include <shlwapi.h>
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

typedef enum _SETUP_COMMAND_TYPE
{
    SETUP_COMMAND_INSTALL,
    SETUP_COMMAND_UNINSTALL,
    SETUP_COMMAND_UPDATE,
    SETUP_COMMAND_REPAIR,
    SETUP_COMMAND_SILENTINSTALL,
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

    union
    {
        ULONG Flags;
        struct
        {
            ULONG SetupCreateDesktopShortcut : 1;
            ULONG SetupCreateDesktopShortcutAllUsers : 1;
            ULONG SetupCreateDefaultTaskManager : 1;
            ULONG SetupCreateSystemStartup : 1;
            ULONG SetupCreateMinimizedSystemStartup : 1;
            ULONG SetupInstallDebuggingTools : 1;
            ULONG SetupInstallPeViewAssociations : 1;
            ULONG SetupInstallKphService : 1;
            ULONG SetupResetSettings : 1;
            ULONG SetupStartAppAfterExit : 1;
            ULONG SetupKphInstallRequired : 1;
            ULONG Spare : 21;
        };
    };

    ULONG ErrorCode;
    PPH_STRING FilePath;

    PPH_STRING RelDate;
    PPH_STRING RelVersion;

    PPH_STRING BinFileDownloadUrl;
    PPH_STRING BinFileLength;
    PPH_STRING BinFileHash;
    PPH_STRING BinFileSignature;
    PPH_STRING SetupFileDownloadUrl;
    PPH_STRING SetupFileLength;
    PPH_STRING SetupFileHash;
    PPH_STRING SetupFileSignature;

    PPH_STRING WebSetupFileDownloadUrl;
    PPH_STRING WebSetupFileVersion;
    PPH_STRING WebSetupFileLength;
    PPH_STRING WebSetupFileHash;
    PPH_STRING WebSetupFileSignature;

    HWND MainHeaderHandle;
    HWND StatusHandle;
    HWND SubStatusHandle;
    HWND ProgressHandle;

    BOOLEAN SetupRunning;
    ULONG CurrentMajorVersion;
    ULONG CurrentMinorVersion;
    ULONG CurrentRevisionVersion;
    WNDPROC TaskDialogWndProc;
} PH_SETUP_CONTEXT, *PPH_SETUP_CONTEXT;

extern SETUP_COMMAND_TYPE SetupMode;
extern PPH_STRING SetupInstallPath;

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

INT_PTR CALLBACK SetupInstallPropPage_WndProc(
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
    _In_ PPH_SETUP_CONTEXT Context,
    _In_ BOOLEAN ForceInstall
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

VOID SetupCreateImageFileExecutionOptions(
    VOID
    );

// download.c

#define MAKE_VERSION_ULONGLONG(major, minor, build, revision) \
    (((ULONGLONG)(major) << 48) | \
    ((ULONGLONG)(minor) << 32) | \
    ((ULONGLONG)(build) << 16) | \
    ((ULONGLONG)(revision) <<  0))

ULONG64 ParseVersionString(
    _Inout_ PPH_STRING VersionString
    );

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

// updatesetup.c

NTSTATUS SetupUpdateWebSetupBuild(
    _In_ PPH_SETUP_CONTEXT Context
    );

// uninstall.c

VOID SetupShowUninstallDialog(
    VOID
    );

#endif