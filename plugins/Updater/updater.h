/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2024
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

#define UPDATE_MENUITEM_UPDATE  1001
#define UPDATE_MENUITEM_SWITCH  1002
#define UPDATE_SWITCH_RELEASE   1003
#define UPDATE_SWITCH_PREVIEW   1004
#define UPDATE_SWITCH_CANARY    1005
#define UPDATE_SWITCH_DEVELOPER 1006

#define PH_SHOWDIALOG  (WM_APP + 501)
#define PH_SHOWLATEST  (WM_APP + 502)
#define PH_SHOWNEWEST  (WM_APP + 503)
#define PH_SHOWUPDATE  (WM_APP + 504)
#define PH_SHOWINSTALL (WM_APP + 505)
#define PH_SHOWERROR   (WM_APP + 506)

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
//#define FORCE_FUTURE_VERSION
//#define FORCE_LATEST_VERSION
//#define FORCE_ELEVATION_CHECK
//
//#define FORCE_NO_STATUS_TIMER
//#define FORCE_SLOW_STATUS_TIMER
//#define FORCE_FAST_STATUS_TIMER
#endif

extern HWND UpdateDialogHandle;
extern PH_EVENT InitializedEvent;
extern PPH_PLUGIN PluginInstance;

typedef struct _PH_UPDATER_CONTEXT
{
    HWND DialogHandle;
    WNDPROC DefaultWindowProc;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN StartupCheck : 1;
            BOOLEAN HaveData : 1;
            BOOLEAN Cancel : 1;
            BOOLEAN Cleanup : 1;
            BOOLEAN ElevationRequired : 1;
            BOOLEAN ProgressMarquee : 1;
            BOOLEAN ProgressTimer : 1;
            BOOLEAN PortableMode : 1;
        };
    };

    ULONG ErrorCode;
    ULONG64 CurrentVersion;
    ULONG64 LatestVersion;
    PPH_STRING SetupFilePath;
    PPH_STRING Version;
    PPH_STRING RelDate;
    PPH_STRING SetupFileLength;
    PPH_STRING SetupFileDownloadUrl;
    PPH_STRING SetupFileHash;
    PPH_STRING SetupFileSignature;
    PPH_STRING CommitHash;
    PH_RELEASE_CHANNEL Channel;
    BOOLEAN SwitchingChannel;

    // Timer support
    LONG64 ProgressTotal;
    LONG64 ProgressDownloaded;
    LONG64 ProgressBitsPerSecond;
} PH_UPDATER_CONTEXT, *PPH_UPDATER_CONTEXT;

// TDM_NAVIGATE_PAGE can not be called from other threads without comctl32.dll throwing access violations
// after navigating to the page and you press keys such as ctrl, alt, home and insert. (dmex)
#define TaskDialogNavigatePage(WindowHandle, Config) \
    assert(HandleToUlong(NtCurrentThreadId()) == GetWindowThreadProcessId(WindowHandle, NULL)); \
    SendMessage(WindowHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)(Config));

#ifdef FORCE_FAST_STATUS_TIMER
#define SETTING_NAME_STATUS_TIMER_INTERVAL USER_TIMER_MINIMUM
#else
#define SETTING_NAME_STATUS_TIMER_INTERVAL 20
#endif

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

NTSTATUS UpdateShellExecute(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_opt_ HWND WindowHandle
    );

BOOLEAN UpdateCheckDirectoryElevationRequired(
    VOID
    );

VOID ShowUpdateDialog(
    _In_opt_ PPH_UPDATER_CONTEXT Context
    );

PPH_UPDATER_CONTEXT CreateUpdateContext(
    _In_ BOOLEAN StartupCheck
    );

VOID StartInitialCheck(
    VOID
    );

VOID ShowStartupUpdateDialog(
    VOID
    );

ULONG64 ParseVersionString(
    _Inout_ PPH_STRING VersionString
    );

// options.c

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
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
    _In_ PH_RELEASE_CHANNEL Channel
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
