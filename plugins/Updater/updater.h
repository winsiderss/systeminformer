/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2023
 *
 */

#ifndef UPDATER_H
#define UPDATER_H

#include <phdk.h>
#include <phappresource.h>
#include <json.h>
#include <verify.h>
#include <settings.h>
#include <workqueue.h>

#include <bcrypt.h>

#include "resource.h"

#define UPDATE_MENUITEM 1001
#define PH_SHOWDIALOG (WM_APP + 501)
#define PH_SHOWLATEST (WM_APP + 502)
#define PH_SHOWNEWEST (WM_APP + 503)
#define PH_SHOWUPDATE (WM_APP + 504)
#define PH_SHOWINSTALL (WM_APP + 505)
#define PH_SHOWERROR (WM_APP + 506)

#define PLUGIN_NAME L"ProcessHacker.UpdateChecker"
#define SETTING_NAME_AUTO_CHECK (PLUGIN_NAME L".PromptStart")
#define SETTING_NAME_LAST_CHECK (PLUGIN_NAME L".LastUpdateCheckTime")
#define SETTING_NAME_UPDATE_MODE (PLUGIN_NAME L".UpdateMode")
#define SETTING_NAME_UPDATE_AVAILABLE (PLUGIN_NAME L".UpdateAvailable")
#define SETTING_NAME_UPDATE_DATA (PLUGIN_NAME L".UpdateData")
#define SETTING_NAME_AUTO_CHECK_PAGE (PLUGIN_NAME L".AutoCheckPage")
#define SETTING_NAME_CHANGELOG_WINDOW_POSITION (PLUGIN_NAME L".ChangelogWindowPosition")
#define SETTING_NAME_CHANGELOG_WINDOW_SIZE (PLUGIN_NAME L".ChangelogWindowSize")
#define SETTING_NAME_CHANGELOG_COLUMNS (PLUGIN_NAME L".ChangelogListColumns")
#define SETTING_NAME_CHANGELOG_SORTCOLUMN (PLUGIN_NAME L".ChangelogListSort")

#define MAKE_VERSION_ULONGLONG(major, minor, build, revision) \
    (((ULONGLONG)(major) << 48) | \
    ((ULONGLONG)(minor) << 32) | \
    ((ULONGLONG)(build) << 16) | \
    ((ULONGLONG)(revision) <<  0))

#ifdef _DEBUG
//#define FORCE_UPDATE_CHECK
//#define FORCE_LATEST_VERSION
//#define FORCE_ELEVATION_CHECK
#endif

extern HWND UpdateDialogHandle;
extern PH_EVENT InitializedEvent;
extern PPH_PLUGIN PluginInstance;

typedef enum _UPDATER_TYPE
{
    InvalidUpdaterType,

    //
    // N.B. Order here is important.
    //

    UpdaterTypeNightly,
    UpdaterTypeNightlyLegacy,
    UpdaterTypeRelease,
    UpdaterTypeReleaseLegacy,

    MaxUpdaterType

} UPDATER_TYPE;

typedef struct _PH_UPDATER_CONTEXT
{
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN StartupCheck : 1;
            BOOLEAN HaveData : 1;
            BOOLEAN FixedWindowStyles : 1;
            BOOLEAN Cancel : 1;
            BOOLEAN DirectoryElevationRequired : 1;
            BOOLEAN Cleanup : 1;
            BOOLEAN Spare : 3;
        };
    };

    HWND DialogHandle;
    WNDPROC DefaultWindowProc;

    ULONG ErrorCode;
    ULONG64 CurrentVersion;
    ULONG64 LatestVersion;
    PPH_STRING SetupFilePath;
    PPH_STRING CurrentVersionString;
    PPH_STRING Version;
    PPH_STRING RelDate;
    PPH_STRING SetupFileLength;
    PPH_STRING SetupFileDownloadUrl;
    PPH_STRING SetupFileHash;
    PPH_STRING SetupFileSignature;
    // Nightly builds only
    PPH_STRING CommitHash;
    UPDATER_TYPE Type;
} PH_UPDATER_CONTEXT, *PPH_UPDATER_CONTEXT;

// TDM_NAVIGATE_PAGE can not be called from other threads without comctl32.dll throwing access violations
// after navigating to the page and you press keys such as ctrl, alt, home and insert. (dmex)
#define TaskDialogNavigatePage(WindowHandle, Config) \
    assert(HandleToUlong(NtCurrentThreadId()) == GetWindowThreadProcessId(WindowHandle, NULL)); \
    SendMessage(WindowHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)Config);

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
    _In_ PPH_UPDATER_CONTEXT Context
    );

// page2.c
VOID ShowCheckingForUpdatesDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );

// page3.c
VOID ShowAvailableDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );

// page4.c
VOID ShowProgressDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );

// page5.c

VOID ShowUpdateInstallDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );

VOID ShowLatestVersionDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );

VOID ShowNewerVersionDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );

VOID ShowUpdateFailedDialog(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ BOOLEAN HashFailed,
    _In_ BOOLEAN SignatureFailed
    );

// updater.c

BOOLEAN UpdateShellExecute(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_opt_ HWND WindowHandle
    );

BOOLEAN UpdateCheckDirectoryElevationRequired(
    VOID
    );

VOID ShowUpdateDialog(
    _In_opt_ PPH_UPDATER_CONTEXT Context
    );

VOID StartInitialCheck(
    VOID
    );

VOID ShowStartupUpdateDialog(
    VOID
    );

BOOLEAN UpdaterInstalledUsingSetup(
    VOID
    );

ULONG64 ParseVersionString(
    _Inout_ PPH_STRING VersionString
    );

// options.c

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK TextDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// verify.c

typedef struct _UPDATER_HASH_CONTEXT
{
    BCRYPT_ALG_HANDLE SignAlgHandle;
    BCRYPT_ALG_HANDLE HashAlgHandle;
    BCRYPT_KEY_HANDLE KeyHandle;
    BCRYPT_HASH_HANDLE HashHandle;
    ULONG HashObjectSize;
    ULONG HashSize;
    PVOID HashObject;
    PVOID Hash;
} UPDATER_HASH_CONTEXT, *PUPDATER_HASH_CONTEXT;

PUPDATER_HASH_CONTEXT UpdaterInitializeHash(
    _In_ UPDATER_TYPE Type
    );

BOOLEAN UpdaterUpdateHash(
    _Inout_ PUPDATER_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

BOOLEAN UpdaterVerifyHash(
    _Inout_ PUPDATER_HASH_CONTEXT Context,
    _In_ PPH_STRING Sha2Hash
    );

BOOLEAN UpdaterVerifySignature(
    _Inout_ PUPDATER_HASH_CONTEXT Context,
    _In_ PPH_STRING HexSignature
    );

VOID UpdaterDestroyHash(
    _In_ PUPDATER_HASH_CONTEXT Context
    );

#endif
