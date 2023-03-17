/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2018
 *
 */

#ifndef _PH_APPRESOLVER_H
#define _PH_APPRESOLVER_H

EXTERN_C_START

_Success_(return)
BOOLEAN PhAppResolverGetAppIdForProcess(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING *ApplicationUserModelId
    );

_Success_(return)
BOOLEAN PhAppResolverGetAppIdForWindow(
    _In_ HWND WindowHandle,
    _Out_ PPH_STRING *ApplicationUserModelId
    );

HRESULT PhAppResolverActivateAppId(
    _In_ PPH_STRING ApplicationUserModelId,
    _In_opt_ PWSTR CommandLine,
    _Out_opt_ HANDLE *ProcessId
    );

HRESULT PhAppResolverPackageTerminateProcess(
    _In_ PPH_STRING PackageFullName
    );

typedef struct _PH_PACKAGE_TASK_ENTRY
{
    PPH_STRING TaskName;
    GUID TaskGuid;
} PH_PACKAGE_TASK_ENTRY, *PPH_PACKAGE_TASK_ENTRY;

PPH_LIST PhAppResolverEnumeratePackageBackgroundTasks(
    _In_ PPH_STRING PackageFullName
    );

HRESULT PhAppResolverPackageStopSessionRedirection(
    _In_ PPH_STRING PackageFullName
    );

PPH_STRING PhGetAppContainerName(
    _In_ PSID AppContainerSid
    );

PPH_STRING PhGetAppContainerSidFromName(
    _In_ PWSTR AppContainerName
    );

PPH_STRING PhGetAppContainerPackageName(
    _In_ PSID Sid
    );

BOOLEAN PhIsPackageCapabilitySid(
    _In_ PSID AppContainerSid,
    _In_ PSID Sid
    );

PPH_STRING PhGetPackagePath(
    _In_ PPH_STRING PackageFullName
    );

PPH_LIST PhGetPackageAssetsFromResourceFile(
    _In_ PWSTR FilePath
    );

typedef struct _PH_APPUSERMODELID_ENUM_ENTRY
{
    PPH_STRING AppUserModelId;
    PPH_STRING DisplayName;
    PPH_STRING PackageInstallPath;
    PPH_STRING PackageFullName;
    PPH_STRING SmallLogoPath;
} PH_APPUSERMODELID_ENUM_ENTRY, *PPH_APPUSERMODELID_ENUM_ENTRY;

PPH_LIST PhEnumerateApplicationUserModelIds(
    VOID
    );

HRESULT PhAppResolverGetPackageResourceFilePath(
    _In_ PCWSTR PackageFullName,
    _In_ PCWSTR Key,
    _Out_ PWSTR* FilePath
    );

_Success_(return)
BOOLEAN PhAppResolverGetPackageIcon(
    _In_ HANDLE ProcessId,
    _In_ PPH_STRING PackageFullName,
    _Out_opt_ HICON* IconLarge,
    _Out_opt_ HICON* IconSmall,
    _In_ LONG SystemDpi
    );

// Immersive PLM task support

HRESULT PhAppResolverBeginCrashDumpTask(
    _In_ HANDLE ProcessId,
    _Out_ HANDLE* TaskHandle
    );

HRESULT PhAppResolverBeginCrashDumpTaskByHandle(
    _In_ HANDLE ProcessHandle,
    _Out_ HANDLE* TaskHandle
    );

HRESULT PhAppResolverEndCrashDumpTask(
    _In_ HANDLE TaskHandle
    );

// Desktop Bridge

HRESULT PhCreateProcessDesktopPackage(
    _In_ PWSTR ApplicationUserModelId,
    _In_ PWSTR Executable,
    _In_ PWSTR Arguments,
    _In_ BOOLEAN PreventBreakaway,
    _In_opt_ HANDLE ParentProcessId,
    _Out_opt_ PHANDLE ProcessHandle
    );

// Windows Runtime

#ifndef __hstring_h__
typedef struct HSTRING__* HSTRING;
#endif

// Note: The HSTRING structures can be found in the Windows SDK
// \cppwinrt\winrt\base.h under MIT License. (dmex)

typedef struct _WSTRING_HEADER // HSTRING_HEADER (WinSDK)
{
    union
    {
        PVOID Reserved1;
#if defined(_WIN64)
        char Reserved2[24];
#else
        char Reserved2[20];
#endif
    } Reserved;
} WSTRING_HEADER;

#define HSTRING_REFERENCE_FLAG 0x1 // hstring_reference_flag (WinSDK)

typedef struct _HSTRING_REFERENCE // Stack
{
    union
    {
        WSTRING_HEADER Header;
        struct
        {
            UINT32 Flags;
            UINT32 Length;
            UINT32 Padding1;
            UINT32 Padding2;
            PCWSTR Buffer;
        };
    };
} HSTRING_REFERENCE;

typedef struct _HSTRING_INSTANCE // Heap
{
    // Header
    union
    {
        WSTRING_HEADER Header;
        struct
        {
            UINT32 Flags;
            UINT32 Length;
            UINT32 Padding1;
            UINT32 Padding2;
            PCWSTR Buffer;
        };
    };

    // Data
    volatile LONG ReferenceCount;
    WCHAR Data[ANYSIZE_ARRAY];
} HSTRING_INSTANCE;

#define HSTRING_FROM_STRING(Header) \
    ((HSTRING)&(Header))

PHLIBAPI
PPH_STRING
NTAPI
PhCreateStringFromWindowsRuntimeString(
    _In_ HSTRING String
    );

PHLIBAPI
HRESULT
NTAPI
PhCreateWindowsRuntimeStringReference(
    _In_ PCWSTR SourceString,
    _Out_ PVOID String
    );

PHLIBAPI
HRESULT
NTAPI
PhCreateWindowsRuntimeString(
    _In_ PCWSTR SourceString,
    _Out_ HSTRING* String
    );

PHLIBAPI
VOID
NTAPI
PhDeleteWindowsRuntimeString(
    _In_opt_ HSTRING String
    );

PHLIBAPI
UINT32
NTAPI
PhGetWindowsRuntimeStringLength(
    _In_opt_ HSTRING String
    );

PHLIBAPI
PCWSTR
NTAPI
PhGetWindowsRuntimeStringBuffer(
    _In_opt_ HSTRING String,
    _Out_opt_ PUINT32 Length
    );

PHLIBAPI
HRESULT
NTAPI
PhGetProcessSystemIdentification(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING* SystemIdForPublisher,
    _Out_ PPH_STRING* SystemIdForUser
    );

EXTERN_C_END

#endif
