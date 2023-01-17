/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

#ifndef _SETUP_H
#define _SETUP_H

#include <ph.h>
#include <guisup.h>
#include <prsht.h>
#include <svcsup.h>
#include <json.h>
#include <phnet.h>

#include <aclapi.h>
#include <io.h>
#include <netlistmgr.h>
#include <propvarutil.h>
#include <propkey.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <sddl.h>
#include <winhttp.h>
#include <uxtheme.h>

#include "resource.h"
#include "..\..\SystemInformer\include\phappres.h"

#define SETUP_SHOWDIALOG (WM_APP + 1)
#define SETUP_SHOWINSTALL (WM_APP + 2)
#define SETUP_SHOWFINAL (WM_APP + 3)
#define SETUP_SHOWERROR (WM_APP + 4)
#define SETUP_SHOWUNINSTALL (WM_APP + 5)
#define SETUP_SHOWUNINSTALLFINAL (WM_APP + 6)
#define SETUP_SHOWUNINSTALLERROR (WM_APP + 7)
#define SETUP_SHOWUPDATE (WM_APP + 8)
#define SETUP_SHOWUPDATEFINAL (WM_APP + 9)
#define SETUP_SHOWUPDATEERROR (WM_APP + 10)

#define TaskDialogNavigatePage(WindowHandle, Config) \
    assert(HandleToUlong(NtCurrentThreadId()) == GetWindowThreadProcessId((WindowHandle), NULL)); \
    SendMessage((WindowHandle), TDM_NAVIGATE_PAGE, 0, (LPARAM)(Config));

#ifdef DEBUG
//#define FORCE_TEST_UPDATE_LOCAL_INSTALL 1
#endif

DECLSPEC_SELECTANY GUID FOLDERID_ProgramData = { 0x62AB5D82, 0xFDC1, 0x4DC3, { 0xA9, 0xDD, 0x07, 0x0D, 0x1D, 0x49, 0x5D, 0x97 } };
DECLSPEC_SELECTANY GUID FOLDERID_PublicDesktop = { 0xC4AA340D, 0xF20F, 0x4863, { 0xAF, 0xEF, 0xF8, 0x7E, 0xF2, 0xE6, 0xBA, 0x25 } };

typedef enum _SETUP_COMMAND_TYPE
{
    SETUP_COMMAND_INSTALL,
    SETUP_COMMAND_SILENTINSTALL,
    SETUP_COMMAND_UNINSTALL,
    SETUP_COMMAND_UPDATE,
} SETUP_COMMAND_TYPE;

typedef struct _PH_SETUP_CONTEXT
{
    HWND DialogHandle;
    HICON IconSmallHandle;
    HICON IconLargeHandle;
    WNDPROC TaskDialogWndProc;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG SetupRemoveAppData: 1;
            ULONG SetupIsLegacyUpdate : 1;
            ULONG Spare : 30;
        };
    };

    SETUP_COMMAND_TYPE SetupMode;
    PPH_STRING SetupInstallPath;
    PPH_STRING SetupServiceName;

    PVOID ZipDownloadBuffer;
    ULONG ZipDownloadBufferLength;

    ULONG ErrorCode;

    PPH_STRING RelDate;
    PPH_STRING RelVersion;

    PPH_STRING BinFileDownloadUrl;
    ULONGLONG BinFileLength;
    PPH_STRING BinFileHash;
    PPH_STRING BinFileSignature;
    PPH_STRING SetupFileDownloadUrl;
    ULONGLONG SetupFileLength;
    PPH_STRING SetupFileHash;
    PPH_STRING SetupFileSignature;

    //PPH_STRING WebSetupFileDownloadUrl;
    //PPH_STRING WebSetupFileVersion;
    //PPH_STRING WebSetupFileLength;
    //PPH_STRING WebSetupFileHash;
    //PPH_STRING WebSetupFileSignature;

    ULONG CurrentMajorVersion;
    ULONG CurrentMinorVersion;
    ULONG CurrentRevisionVersion;

    BOOLEAN NeedsReboot;
} PH_SETUP_CONTEXT, *PPH_SETUP_CONTEXT;

VOID SetupParseCommandLine(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowWelcomePageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowConfigPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowConfigDirectoryNonEmptyDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowDownloadPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowCompletedPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowErrorPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowInstallPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowUninstallPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowUninstallingPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowUninstallCompletedPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowUninstallErrorPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowUpdatePageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowUpdateCompletedPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID ShowUpdateErrorPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    );

// util.c

PPH_STRING SetupFindInstallDirectory(
    VOID
    );

VOID SetupDeleteAppdataDirectory(
    _In_ PPH_SETUP_CONTEXT Context
    );

BOOLEAN SetupUninstallDriver(
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

VOID SetupChangeNotifyShortcuts(
    _In_ PPH_SETUP_CONTEXT Context
    );

BOOLEAN SetupCreateUninstallFile(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID SetupDeleteUninstallFile(
    _In_ PPH_SETUP_CONTEXT Context
    );

BOOLEAN SetupExecuteApplication(
    _In_ PPH_SETUP_CONTEXT Context
    );

VOID SetupUpgradeSettingsFile(
    VOID
    );

extern PH_STRINGREF UninstallKeyName;

typedef struct _SETUP_EXTRACT_FILE
{
    PSTR FileName;
    PWSTR ExtractFileName;
} SETUP_EXTRACT_FILE, *PSETUP_EXTRACT_FILE;

typedef struct _SETUP_REMOVE_FILE
{
    PWSTR FileName;
} SETUP_REMOVE_FILE, *PSETUP_REMOVE_FILE;

VOID SetupCreateLink(
    _In_ PWSTR LinkFilePath,
    _In_ PWSTR FilePath,
    _In_ PWSTR FileParentDir,
    _In_ PWSTR AppId
    );

BOOLEAN CheckApplicationInstalled(
    VOID
    );

BOOLEAN CheckApplicationInstallPathLegacy(
    _In_ PPH_STRING Directory
    );

PPH_STRING GetApplicationInstallPath(
    VOID
    );

BOOLEAN SetupShutdownApplication(
    _In_ PPH_SETUP_CONTEXT Context
    );

NTSTATUS QueryProcessesUsingVolumeOrFile(
    _In_ HANDLE VolumeOrFileHandle,
    _Out_ PFILE_PROCESS_IDS_USING_FILE_INFORMATION *Information
    );

PPH_STRING SetupCreateFullPath(
    _In_ PPH_STRING Path,
    _In_ PWSTR FileName
    );

BOOLEAN SetupOverwriteFile(
    _In_ PPH_STRING FileName,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    );

BOOLEAN SetupHashFile(
    _In_ PPH_STRING FileName,
    _Out_writes_all_(256 / 8) PBYTE Buffer
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
    _Inout_ PPH_SETUP_CONTEXT Context
    );

BOOLEAN UpdateDownloadUpdateData(
    _In_ PPH_SETUP_CONTEXT Context
    );

// extract.c

NTSTATUS SetupProgressThread(
    _In_ PPH_SETUP_CONTEXT Context
    );

BOOLEAN SetupExtractBuild(
    _In_ PPH_SETUP_CONTEXT Context
    );

#endif
