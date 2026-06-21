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
#include <mapimg.h>
#include <guisup.h>
#include <appresolver.h>

#include "resource.h"

EXTERN_C_START

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

#define PLUGIN_NAME L"UpdateChecker"
#define SETTING_NAME_AUTO_CHECK (PLUGIN_NAME L".PromptStart")
#define SETTING_NAME_LAST_CHECK (PLUGIN_NAME L".UpdateCheck")
#define SETTING_NAME_UPDATE_INTERVAL (PLUGIN_NAME L".UpdateInterval")
#define SETTING_NAME_UPDATE_MODE (PLUGIN_NAME L".UpdateMode")
#define SETTING_NAME_UPDATE_AVAILABLE (PLUGIN_NAME L".UpdateAvailable")
#define SETTING_NAME_UPDATE_DATA (PLUGIN_NAME L".UpdateData")
#define SETTING_NAME_AUTO_CHECK_PAGE (PLUGIN_NAME L".AutoCheckPage")
#define SETTING_NAME_SHOW_NOTIFICATION (PLUGIN_NAME L".ShowNotification")
#define SETTING_NAME_TOAST_NOTIFICATIONS (PLUGIN_NAME L".ShowToastNotification")
#define SETTING_NAME_DOWNLOAD_METHOD (PLUGIN_NAME L".DownloadMethod")
#define SETTING_NAME_CHANGELOG_WINDOW_POSITION (PLUGIN_NAME L".ChangelogWindowPosition")
#define SETTING_NAME_CHANGELOG_WINDOW_SIZE (PLUGIN_NAME L".ChangelogWindowSize")
#define SETTING_NAME_CHANGELOG_COLUMNS (PLUGIN_NAME L".ChangelogListColumns")
#define SETTING_NAME_CHANGELOG_SORTCOLUMN (PLUGIN_NAME L".ChangelogListSort")

typedef enum _UPDATE_DOWNLOAD_METHOD
{
    UpdateDownloadMethodAutomatic = 0,
    UpdateDownloadMethodWinHttp,
    UpdateDownloadMethodBits,
    UpdateDownloadMethodDeliveryOptimization,
    UpdateDownloadMethodMaximum
} UPDATE_DOWNLOAD_METHOD;

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

typedef enum _UPDATER_CRYPTO_BACKEND
{
    UpdaterCryptoBackendSymCrypt = 0,
    UpdaterCryptoBackendBCrypt,
} UPDATER_CRYPTO_BACKEND;

typedef struct _PH_UPDATER_CONTEXT
{
    HWND DialogHandle;
    WNDPROC DefaultWindowProc;
    LONG WindowDpi;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG StartupCheck : 1;
            ULONG HaveData : 1;
            ULONG Cancel : 1;
            ULONG Cleanup : 1;
            ULONG ElevationRequired : 1;
            ULONG ProgressMarquee : 1;
            ULONG ProgressTimer : 1;
            ULONG PortableMode : 1;
            ULONG ToastMode : 1;
        };
    };
    BOOLEAN ProgressFinalizing;

    ULONG UpdateStatus;
    ULONG64 CurrentVersion;
    ULONG64 LatestVersion;
    HANDLE SetupFileHandle;
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
    UPDATER_CRYPTO_BACKEND CryptoBackend;

    // Timer support
    LONG64 ProgressTotal;
    LONG64 ProgressDownloaded;
    LONG64 ProgressBitsPerSecond;
} PH_UPDATER_CONTEXT, *PPH_UPDATER_CONTEXT;

typedef struct _UPDATER_DOWNLOAD_RESULT
{
    BOOLEAN DownloadSuccess;
    BOOLEAN HashSuccess;
    BOOLEAN SignatureSuccess;
} UPDATER_DOWNLOAD_RESULT, *PUPDATER_DOWNLOAD_RESULT;

#ifdef FORCE_FAST_STATUS_TIMER
#define SETTING_NAME_STATUS_TIMER_INTERVAL USER_TIMER_MINIMUM
#else
#define SETTING_NAME_STATUS_TIMER_INTERVAL 20
#endif

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UpdateCheckThread(
    _In_ PVOID Parameter
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UpdateDownloadThread(
    _In_ PVOID Parameter
    );

// download.c

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UpdateInstallerDownloadThreadStage1(
    _In_ PVOID Parameter
    );

NTSTATUS UpdateDownloadInstallerWithWinHttpStage2(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PUPDATER_DOWNLOAD_RESULT Result
    );

NTSTATUS UpdateVerifyCacheFileSignature(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PUPDATER_DOWNLOAD_RESULT Result
    );

PPH_STRING UpdateParseDownloadFileName(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ PPH_STRING DownloadUrlPath
    );

NTSTATUS UpdateNtStatusFromHresult(
    _In_ HRESULT Result
    );

VOID UpdateSetProgressState(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ ULONG64 TotalLength,
    _In_ ULONG64 ReadLength,
    _In_ ULONG64 BitsPerSecond
    );

VOID UpdateSetProgressFinalizingState(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ PCWSTR MainInstruction
    );

VOID UpdatePostDownloadResult(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ PUPDATER_DOWNLOAD_RESULT Result
    );

NTSTATUS UpdateDownloadFileWithBits(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PUPDATER_DOWNLOAD_RESULT Result
    );

NTSTATUS UpdateDownloadFileWithDeliveryOptimization(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PUPDATER_DOWNLOAD_RESULT Result
    );

// downloadmsix.cpp

NTSTATUS UpdaterMsixCheckForUpdates(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PBOOLEAN UpdateAvailable
    );

NTSTATUS UpdaterMsixDownloadAndInstall(
    _In_ PPH_UPDATER_CONTEXT Context
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UpdateMsixDownloadThread(
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

// utils.c

NTSTATUS UpdateShellExecute(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_opt_ HWND WindowHandle
    );

BOOLEAN UpdateCheckDirectoryElevationRequired(
    VOID
    );

VOID TaskDialogLinkClicked(
    _In_ PPH_UPDATER_CONTEXT Context
    );

BOOLEAN LastUpdateCheckExpired(
    VOID
    );

PPH_STRING UpdateVersionString(
    VOID
    );

PPH_STRING UpdateWindowsString(
    VOID
    );

ULONG64 ParseVersionString(
    _In_ PPH_STRING VersionString
    );

BOOLEAN UpdateValidateFileName(
    _In_ PPH_STRING FileName
    );

// updater.c

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
    _In_ PPH_STRING CacheString
    );

// options.c

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK TextDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// toast.c

BOOLEAN UpdaterShowAvailableToast(
    _In_ PPH_UPDATER_CONTEXT Context
    );

BOOLEAN UpdaterShowProgressToast(
    _In_ PPH_UPDATER_CONTEXT Context
    );

BOOLEAN UpdaterShowReadyToInstallToast(
    _In_ PPH_UPDATER_CONTEXT Context
    );

BOOLEAN UpdaterShowFailedToast(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ BOOLEAN HashFailed,
    _In_ BOOLEAN SignatureFailed
    );

VOID UpdaterUpdateProgressToast(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_opt_ PCWSTR StatusText
    );

VOID UpdaterHideActiveToasts(
    VOID
    );

VOID UpdaterLaunchInstaller(
    _In_ PPH_UPDATER_CONTEXT Context
    );

// verify.c

#include <bcrypt.h>

#if __has_include("../../phlib/include/phcrypt.h")
#include "../../phlib/include/phcrypt.h"
#else
#error "ThirdParty.lib is missing"
#endif

typedef struct _PH_SYMCRYPT_PSS_PADDING_INFO
{
    PCWSTR pszAlgId;
    ULONG cbSalt;
} PH_SYMCRYPT_PSS_PADDING_INFO, *PPH_SYMCRYPT_PSS_PADDING_INFO;

typedef enum _UPDATER_SIGNING_GENERATION
{
    UpdaterSigningGenerationCurrent,
    UpdaterSigningGenerationNext,
    MaxUpdaterSigningGeneration,
} UPDATER_SIGNING_GENERATION, *PUPDATER_SIGNING_GENERATION;

typedef struct _UPDATER_SIGNING
{
    PVOID PaddingInfo;
    ULONG HashSize;
    ULONG PaddingFlags;
    // BCrypt
    BCRYPT_ALG_HANDLE SignAlgHandle;
    BCRYPT_KEY_HANDLE KeyHandle;
    BCRYPT_ALG_HANDLE HashAlgHandle;
    BCRYPT_HASH_HANDLE HashHandle;
    // SymCrypt
    PH_SYMCRYPT_HASH_CONTEXT HashContext;
    PCWSTR SymCryptBlobType;
    const UCHAR* SymCryptKeyBlob;
    ULONG SymCryptKeyBlobLength;
    UCHAR HashBuffer[PH_SYMCRYPT_SHA512_RESULT_SIZE];
} UPDATER_SIGNING, *PUPDATER_SIGNING;

typedef struct _UPDATER_HASH_CONTEXT
{
    UPDATER_CRYPTO_BACKEND Backend;
    ULONG HashSize;
    // BCrypt
    BCRYPT_ALG_HANDLE HashAlgHandle;
    BCRYPT_HASH_HANDLE HashHandle;
    // SymCrypt
    PH_SYMCRYPT_HASH_CONTEXT HashContext;
    UPDATER_SIGNING Sign[MaxUpdaterSigningGeneration];
    UCHAR HashBuffer[PH_SYMCRYPT_SHA512_RESULT_SIZE];
} UPDATER_HASH_CONTEXT, *PUPDATER_HASH_CONTEXT;

NTSTATUS UpdaterInitializeHash(
    _Out_ PUPDATER_HASH_CONTEXT* Context,
    _In_ PH_RELEASE_CHANNEL Channel
    );

NTSTATUS UpdaterInitializeHashSymCrypt(
    _Out_ PUPDATER_HASH_CONTEXT* Context,
    _In_ PH_RELEASE_CHANNEL Channel
    );

NTSTATUS UpdaterHashData(
    _In_ PUPDATER_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

NTSTATUS UpdaterHashDataSymCrypt(
    _In_ PUPDATER_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

NTSTATUS UpdaterVerifyHash(
    _In_ PUPDATER_HASH_CONTEXT Context,
    _In_ PPH_STRING Sha2Hash
    );

NTSTATUS UpdaterVerifyHashSymCrypt(
    _In_ PUPDATER_HASH_CONTEXT Context,
    _In_ PPH_STRING Sha2Hash
    );

NTSTATUS UpdaterVerifySignature(
    _In_ PUPDATER_HASH_CONTEXT Context,
    _In_ PPH_STRING HexSignature
    );

NTSTATUS UpdaterVerifySignatureSymCrypt(
    _In_ PUPDATER_HASH_CONTEXT Context,
    _In_ PPH_STRING HexSignature
    );

VOID UpdaterDestroyHash(
    _Frees_ptr_opt_ PUPDATER_HASH_CONTEXT Context
    );

VOID UpdaterDestroyHashSymCrypt(
    _Frees_ptr_opt_ PUPDATER_HASH_CONTEXT Context
    );

FORCEINLINE NTSTATUS UpdaterInitializeHashForContext(
    _Out_ PUPDATER_HASH_CONTEXT* Context,
    _In_ struct _PH_UPDATER_CONTEXT* UpdaterContext
    )
{
    if (UpdaterContext->CryptoBackend == UpdaterCryptoBackendSymCrypt)
        return UpdaterInitializeHashSymCrypt(Context, UpdaterContext->Channel);
    return UpdaterInitializeHash(Context, UpdaterContext->Channel);
}

FORCEINLINE NTSTATUS UpdaterHashDataForContext(
    _In_ PUPDATER_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    if (Context->Backend == UpdaterCryptoBackendSymCrypt)
        return UpdaterHashDataSymCrypt(Context, Buffer, Length);
    return UpdaterHashData(Context, Buffer, Length);
}

FORCEINLINE NTSTATUS UpdaterVerifyHashForContext(
    _In_ PUPDATER_HASH_CONTEXT Context,
    _In_ PPH_STRING Sha2Hash
    )
{
    if (Context->Backend == UpdaterCryptoBackendSymCrypt)
        return UpdaterVerifyHashSymCrypt(Context, Sha2Hash);
    return UpdaterVerifyHash(Context, Sha2Hash);
}

FORCEINLINE NTSTATUS UpdaterVerifySignatureForContext(
    _In_ PUPDATER_HASH_CONTEXT Context,
    _In_ PPH_STRING HexSignature
    )
{
    if (Context->Backend == UpdaterCryptoBackendSymCrypt)
        return UpdaterVerifySignatureSymCrypt(Context, HexSignature);
    return UpdaterVerifySignature(Context, HexSignature);
}

FORCEINLINE VOID UpdaterDestroyHashForContext(
    _Frees_ptr_opt_ PUPDATER_HASH_CONTEXT Context
    )
{
    if (!Context)
        return;

    if (Context->Backend == UpdaterCryptoBackendSymCrypt)
        UpdaterDestroyHashSymCrypt(Context);
    else
        UpdaterDestroyHash(Context);
}

EXTERN_C_END

#endif
