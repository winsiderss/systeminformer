/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2023
 *
 */

#ifndef _PH_APPRESOLVER_P_H
#define _PH_APPRESOLVER_P_H

// "660B90C8-73A9-4B58-8CAE-355B7F55341B"
DEFINE_GUID(CLSID_StartMenuCacheAndAppResolver_I, 0x660B90C8, 0x73A9, 0x4B58, 0x8C, 0xAE, 0x35, 0x5B, 0x7F, 0x55, 0x34, 0x1B);
// "46A6EEFF-908E-4DC6-92A6-64BE9177B41C"
DEFINE_GUID(IID_IApplicationResolver_I, 0x46A6EEFF, 0x908E, 0x4DC6, 0x92, 0xA6, 0x64, 0xBE, 0x91, 0x77, 0xB4, 0x1c);
// "DE25675A-72DE-44b4-9373-05170450C140"
DEFINE_GUID(IID_IApplicationResolver2_I, 0xDE25675A, 0x72DE, 0x44b4, 0x93, 0x73, 0x05, 0x17, 0x04, 0x50, 0xC1, 0x40);
// "33F71155-C2E9-4FFE-9786-A32D98577CFF"
DEFINE_GUID(IID_IStartMenuAppItems_I, 0x33F71155, 0xC2E9, 0x4FFE, 0x97, 0x86, 0xA3, 0x2D, 0x98, 0x57, 0x7C, 0xFF);
// "02C5CCF3-805F-4654-A7B7-340A74335365"
DEFINE_GUID(IID_IStartMenuAppItems2_I, 0x02C5CCF3, 0x805F, 0x4654, 0xA7, 0xB7, 0x34, 0x0A, 0x74, 0x33, 0x53, 0x65);

// ba5a92ae_bfd7_4916_854f_6b3a402b84a8
// 21cbc515_2dde_4d66_8292_ba34bd25094a
// 17541a17_f509_4cb6_8d08_651dc68d8227
// 67383c80_7543_4e04_9e97_c990f5e3878d
// 3af46105_16ce_776f_43a2_341172b6e95a
// 6b1d7151_7a17_4c66_abc4_c9b1bc6c0e8d
// ffd2cca4_adfb_46e9_9a02_49d019db2a68
// 8dda7cc4_ae71_4e2a_bc8d_7f16ec66f540

// "ba5a92ae-bfd7-4916-854f-6b3a402b84a8"
//DEFINE_GUID(IID_IStartMenuItemsCache, 0xba5a92ae, 0xbfd7, 0x4916, 0x85, 0x4f, 0x6b, 0x3a, 0x40, 0x2b, 0x84, 0xa8);
// "21cbc515-2dde-4d66-8292-ba34bd25094a"
DEFINE_GUID(IID_IDesktopTileActivator, 0x21cbc515, 0x2dde, 0x4d66, 0x82, 0x92, 0xba, 0x34, 0xbd, 0x25, 0x09, 0x4a);
// "17541a17-f509-4cb6-8d08-651dc68d8227"
DEFINE_GUID(IID_IAppResolverPinToStart, 0x17541a17, 0xf509, 0x4cb6, 0x8d, 0x08, 0x65, 0x1d, 0xc6, 0x8d, 0x82, 0x27);
// "67383c80-7543-4e04-9e97-c990f5e3878d"
DEFINE_GUID(IID_ILauncherAppList, 0x67383c80, 0x7543, 0x4e04, 0x9e, 0x97, 0xc9, 0x90, 0xf5, 0xe3, 0x87, 0x8d);
// "3af46105-16ce-776f-43a2-341172b6e95a"
DEFINE_GUID(IID_IStartMenuRankManager, 0x3af46105, 0x16ce, 0x776f, 0x43, 0xa2, 0x34, 0x11, 0x72, 0xb6, 0xe9, 0x5a);
// "6b1d7151-7a17-4c66-abc4-c9b1bc6c0e8d"
DEFINE_GUID(IID_IAppResolverCacheAccessor, 0x6b1d7151, 0x7a17, 0x4c66, 0xab, 0xc4, 0xc9, 0xb1, 0xbc, 0x6c, 0x0e, 0x8d);
// "ffd2cca4-adfb-46e9-9a02-49d019db2a68"
DEFINE_GUID(IID_IAppResolverPinToStartFF, 0xffd2cca4, 0xadfb, 0x46e9, 0x9a, 0x02, 0x49, 0xd0, 0x19, 0xdb, 0x2a, 0x68);
// "8dda7cc4-ae71-4e2a-bc8d-7f16ec66f540"
DEFINE_GUID(IID_IAppResolverPinToStartF, 0x8dda7cc4, 0xae71, 0x4e2a, 0xbc, 0x8d, 0x7f, 0x16, 0xec, 0x66, 0xf5, 0x40);
// "DE25675A-72DE-44b4-9373-05170450C140"

// IStartMenuItemsCache
// IDesktopTileActivator
// IAppResolverPinToStart
// ILauncherAppList
// IStartMenuRankManager
// IAppResolverCacheAccessor

// "DBCE7E40-7345-439D-B12C-114A11819A09"
DEFINE_GUID(CLSID_MrtResourceManager_I, 0xDBCE7E40, 0x7345, 0x439D, 0xB1, 0x2C, 0x11, 0x4A, 0x11, 0x81, 0x9A, 0x09);
// "130A2F65-2BE7-4309-9A58-A9052FF2B61C"
DEFINE_GUID(IID_IMrtResourceManager_I, 0x130A2F65, 0x2BE7, 0x4309, 0x9A, 0x58, 0xA9, 0x05, 0x2F, 0xF2, 0xB6, 0x1C);
// "E3C22B30-8502-4B2F-9133-559674587E51"
DEFINE_GUID(IID_IResourceContext_I, 0xE3C22B30, 0x8502, 0x4B2F, 0x91, 0x33, 0x55, 0x96, 0x74, 0x58, 0x7E, 0x51);
// "6E21E72B-B9B0-42AE-A686-983CF784EDCD"
DEFINE_GUID(IID_IResourceMap_I, 0x6E21E72B, 0xB9B0, 0x42AE, 0xA6, 0x86, 0x98, 0x3C, 0xF7, 0x84, 0xED, 0xCD);

NTSYSAPI
HRESULT
NTAPI
AppContainerDeriveSidFromMoniker( // DeriveAppContainerSidFromAppContainerName
    _In_ PCWSTR AppContainerName,
    _Out_ PSID *AppContainerSid
    );

// Note: LookupAppContainerDisplayName (userenv.dll, ordinal 211) has the same prototype but returns 'PackageName/ContainerName'.
NTSYSAPI
HRESULT
NTAPI
AppContainerLookupMoniker(
    _In_ PSID AppContainerSid,
    _Out_ PWSTR *PackageFamilyName
    );

NTSYSAPI
HRESULT
NTAPI
AppContainerRegisterSid(
    _In_ PSID Sid,
    _In_ PCWSTR AppContainerName,
    _In_ PCWSTR DisplayName
    );

NTSYSAPI
HRESULT
NTAPI
AppContainerUnregisterSid(
    _In_ PSID Sid
    );

NTSYSAPI
BOOL
NTAPI
AppContainerFreeMemory(
    _Frees_ptr_opt_ PVOID Memory
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
PsmGetKeyFromProcess(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID KeyBuffer,
    _Inout_ PULONG KeyLength
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
PsmGetKeyFromToken(
    _In_ HANDLE TokenHandle,
    _Out_ PVOID KeyBuffer,
    _Inout_ PULONG KeyLength
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
PsmGetApplicationNameFromKey(
    _In_ PVOID KeyBuffer,
    _Out_ PVOID NameBuffer,
    _Inout_ PULONG NameLength
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
PsmGetPackageFullNameFromKey(
    _In_ PVOID KeyBuffer,
    _Out_ PVOID NameBuffer,
    _Inout_ PULONG NameLength
    );

// "168EB462-775F-42AE-9111-D714B2306C2E"
DEFINE_GUID(CLSID_IDesktopAppXActivator_I, 0x168EB462, 0x775F, 0x42AE, 0x91, 0x11, 0xD7, 0x14, 0xB2, 0x30, 0x6C, 0x2E);
// "72e3a5b0-8fea-485c-9f8b-822b16dba17f"
DEFINE_GUID(IID_IDesktopAppXActivator1_I, 0x72e3a5b0, 0x8fea, 0x485c, 0x9f, 0x8b, 0x82, 0x2b, 0x16, 0xdb, 0xa1, 0x7f);
// "F158268A-D5A5-45CE-99CF-00D6C3F3FC0A"
DEFINE_GUID(IID_IDesktopAppXActivator2_I, 0xF158268A, 0xD5A5, 0x45CE, 0x99, 0xCF, 0x00, 0xD6, 0xC3, 0xF3, 0xFC, 0x0A);

typedef enum _DESKTOP_APPX_ACTIVATE_OPTIONS
{
    DAXAO_NONE = 0,
    DAXAO_ELEVATE = 1,
    DAXAO_NONPACKAGED_EXE = 2,
    DAXAO_NONPACKAGED_EXE_PROCESS_TREE = 4,
    DAXAO_NONPACKAGED_EXE_FLAGS = 6,
    DAXAO_NO_ERROR_UI = 8,
    DAXAO_CHECK_FOR_APPINSTALLER_UPDATES = 16,
    DAXAO_CENTENNIAL_PROCESS = 32,
    DAXAO_UNIVERSAL_PROCESS = 64,
    DAXAO_WIN32ALACARTE_PROCESS = 128,
    DAXAO_RUNTIME_BEHAVIOR_FLAGS = 224,
    DAXAO_PARTIAL_TRUST = 256,
    DAXAO_UNIVERSAL_CONSOLE = 512,
    DAXAO_APP_SILO = 1024,
    DAXAO_TRUST_LEVEL_FLAGS = 1280
} DESKTOP_APPX_ACTIVATE_OPTIONS, *PDESKTOP_APPX_ACTIVATE_OPTIONS;

// IDesktopAppXActivator
#ifndef __IDesktopAppXActivator_INTERFACE_DEFINED__
#define __IDesktopAppXActivator_INTERFACE_DEFINED__

typedef struct IDesktopAppXActivator IDesktopAppXActivator;

typedef struct IDesktopAppXActivatorVtbl
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IDesktopAppXActivator, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(
        _In_ IDesktopAppXActivator* This,
        _In_ REFIID riid,
        _COM_Outptr_ void** ppvObject
        );

    DECLSPEC_XFGVIRT(IDesktopAppXActivator, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(
        _In_ IDesktopAppXActivator* This
        );

    DECLSPEC_XFGVIRT(IDesktopAppXActivator, Release)
    ULONG (STDMETHODCALLTYPE* Release)(
        _In_ IDesktopAppXActivator* This
        );

    // IDesktopAppXActivator1
    DECLSPEC_XFGVIRT(IDesktopAppXActivator, Activate)
    HRESULT (STDMETHODCALLTYPE* Activate)(
        _In_ IDesktopAppXActivator* This,
        _In_ PCWSTR ApplicationUserModelId,
        _In_ PCWSTR PackageRelativeExecutable,
        _In_ PCWSTR Arguments,
        _Out_ PHANDLE ProcessHandle
        );

    DECLSPEC_XFGVIRT(IDesktopAppXActivator, ActivateWithOptions)
    HRESULT (STDMETHODCALLTYPE* ActivateWithOptions)(
        _In_ IDesktopAppXActivator* This,
        _In_ PCWSTR ApplicationUserModelId,
        _In_ PCWSTR Executable,
        _In_ PCWSTR Arguments,
        _In_ ULONG ActivationOptions,
        _In_opt_ ULONG ParentProcessId,
        _Out_ PHANDLE ProcessHandle
        );

    // IDesktopAppXActivator2
    DECLSPEC_XFGVIRT(IDesktopAppXActivator, ActivateWithOptionsAndArgs)
    HRESULT (STDMETHODCALLTYPE* ActivateWithOptionsAndArgs)(
        _In_ IDesktopAppXActivator* This,
        _In_ PCWSTR ApplicationUserModelId,
        _In_ PCWSTR Executable,
        _In_ PCWSTR Arguments,
        _In_opt_ ULONG ParentProcessId,
        _In_opt_ PVOID ActivatedEventArgs,
        _Out_ PHANDLE ProcessHandle
        );

    DECLSPEC_XFGVIRT(IDesktopAppXActivator, ActivateWithOptionsArgsWorkingDirectoryShowWindow)
    HRESULT (STDMETHODCALLTYPE* ActivateWithOptionsArgsWorkingDirectoryShowWindow)(
        _In_ IDesktopAppXActivator* This,
        _In_ PCWSTR ApplicationUserModelId,
        _In_ PCWSTR Executable,
        _In_ PCWSTR Arguments,
        _In_ ULONG ActivationOptions,
        _In_opt_ ULONG ParentProcessId,
        _In_opt_ PVOID ActivatedEventArgs,
        _In_ PCWSTR WorkingDirectory,
        _In_ ULONG ShowWindow,
        _Out_ PHANDLE ProcessHandle
        );

    END_INTERFACE
} IDesktopAppXActivatorVtbl;

interface IDesktopAppXActivator
{
    CONST_VTBL struct IDesktopAppXActivatorVtbl* lpVtbl;
};

#ifdef COBJMACROS
#define IDesktopAppXActivator_QueryInterface(This,riid,ppvObject) \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IDesktopAppXActivator_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IDesktopAppXActivator_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IDesktopAppXActivator_Activate(This,AUMID,PRE,Args,ProcessHandle) \
    ((This)->lpVtbl->Activate(This,AUMID,PRE,Args,ProcessHandle))
#define IDesktopAppXActivator_ActivateWithOptions(This,AUMID,Exe,Args,Options,ParentPid,ProcessHandle) \
    ((This)->lpVtbl->ActivateWithOptions(This,AUMID,Exe,Args,Options,ParentPid,ProcessHandle))
#define IDesktopAppXActivator_ActivateWithOptionsAndArgs(This,AUMID,Exe,Args,ParentPid,EventArgs,ProcessHandle) \
    ((This)->lpVtbl->ActivateWithOptionsAndArgs(This,AUMID,Exe,Args,ParentPid,EventArgs,ProcessHandle))
#define IDesktopAppXActivator_ActivateWithOptionsArgsWorkingDirectoryShowWindow(This,AUMID,Exe,Args,Options,ParentPid,EventArgs,WorkingDir,ShowWindow,ProcessHandle) \
    ((This)->lpVtbl->ActivateWithOptionsArgsWorkingDirectoryShowWindow(This,AUMID,Exe,Args,Options,ParentPid,EventArgs,WorkingDir,ShowWindow,ProcessHandle))
#endif // COBJMACROS

#endif // __IDesktopAppXActivator_INTERFACE_DEFINED__
// "ba5a92ae-bfd7-4916-854f-6b3a402b84a8"
DEFINE_GUID(IID_IStartMenuItemsCache, 0xba5a92ae, 0xbfd7, 0x4916, 0x85, 0x4f, 0x6b, 0x3a, 0x40, 0x2b, 0x84, 0xa8);

// Note: Interface checks GetModuleFileNameW is equal L"EXPLORER.EXE",
// else L"PPISHELL.EXE", L"TE.PROCESSHOST.EXE" from HKLM "AppResolverHostProcess"

// IStartMenuItemsCache
#ifndef __IStartMenuItemsCache_INTERFACE_DEFINED__
#define __IStartMenuItemsCache_INTERFACE_DEFINED__

typedef struct IStartMenuItemsCache IStartMenuItemsCache;

typedef struct IStartMenuItemsCacheVtbl
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(
        _In_ IStartMenuItemsCache* This,
        _In_ REFIID riid,
        _COM_Outptr_ void** ppvObject);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(
        _In_ IStartMenuItemsCache* This);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, Release)
    ULONG (STDMETHODCALLTYPE* Release)(
        _In_ IStartMenuItemsCache* This);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, OnChangeNotify)
    HRESULT (STDMETHODCALLTYPE* OnChangeNotify)(
        _In_ IStartMenuItemsCache* This,
        _In_ ULONG,
        _In_ LONG,
        _In_ PCIDLIST_ABSOLUTE,
        _In_ PCIDLIST_ABSOLUTE);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, RegisterForNotifications)
    HRESULT (STDMETHODCALLTYPE* RegisterForNotifications)(
        _In_ IStartMenuItemsCache* This);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, UnregisterForNotifications)
    HRESULT (STDMETHODCALLTYPE* UnregisterForNotifications)(
        _In_ IStartMenuItemsCache* This);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, PauseNotifications)
    HRESULT (STDMETHODCALLTYPE* PauseNotifications)(
        _In_ IStartMenuItemsCache* This);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, ResumeNotifications)
    HRESULT (STDMETHODCALLTYPE* ResumeNotifications)(
        _In_ IStartMenuItemsCache* This);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, RegisterARNotify)
    HRESULT (STDMETHODCALLTYPE* RegisterARNotify)(
        _In_ IStartMenuItemsCache* This,
        _In_ struct IAppResolverNotify* AppResolverNotify);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, RefreshCache)
    HRESULT (STDMETHODCALLTYPE* RefreshCache)(
        _In_ IStartMenuItemsCache* This,
        _In_ enum START_MENU_REFRESH_CACHE_FLAGS Flags);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, ReleaseGlobalCacheObject)
    HRESULT (STDMETHODCALLTYPE* ReleaseGlobalCacheObject)(
        _In_ IStartMenuItemsCache* This);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, IsCacheMatchingLanguage)
    HRESULT (STDMETHODCALLTYPE* IsCacheMatchingLanguage)(
        _In_ IStartMenuItemsCache* This,
        _Out_ PLONG LanguageId);

    DECLSPEC_XFGVIRT(IStartMenuItemsCache, EnableAppUsageData)
    HRESULT (STDMETHODCALLTYPE* EnableAppUsageData)(
        _In_ IStartMenuItemsCache* This);

    END_INTERFACE
} IStartMenuItemsCacheVtbl;

interface IStartMenuItemsCache
{
    CONST_VTBL struct IStartMenuItemsCacheVtbl* lpVtbl;
};

#ifdef COBJMACROS
#define IStartMenuItemsCache_QueryInterface(This,riid,ppvObject) ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IStartMenuItemsCache_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IStartMenuItemsCache_Release(This) ((This)->lpVtbl->Release(This))
#define IStartMenuItemsCache_OnChangeNotify(This,a,b,c,d) ((This)->lpVtbl->OnChangeNotify(This,a,b,c,d))
#define IStartMenuItemsCache_RegisterForNotifications(This) ((This)->lpVtbl->RegisterForNotifications(This))
#define IStartMenuItemsCache_UnregisterForNotifications(This) ((This)->lpVtbl->UnregisterForNotifications(This))
#define IStartMenuItemsCache_PauseNotifications(This) ((This)->lpVtbl->PauseNotifications(This))
#define IStartMenuItemsCache_ResumeNotifications(This) ((This)->lpVtbl->ResumeNotifications(This))
#define IStartMenuItemsCache_RegisterARNotify(This,p) ((This)->lpVtbl->RegisterARNotify(This,p))
#define IStartMenuItemsCache_RefreshCache(This,Flags) ((This)->lpVtbl->RefreshCache(This,Flags))
#define IStartMenuItemsCache_ReleaseGlobalCacheObject(This) ((This)->lpVtbl->ReleaseGlobalCacheObject(This))
#define IStartMenuItemsCache_IsCacheMatchingLanguage(This,pLong) ((This)->lpVtbl->IsCacheMatchingLanguage(This,pLong))
#define IStartMenuItemsCache_EnableAppUsageData(This) ((This)->lpVtbl->EnableAppUsageData(This))
#endif // COBJMACROS

#endif // __IStartMenuItemsCache_INTERFACE_DEFINED__



typedef enum _START_MENU_APP_ITEMS_FLAGS
{
    SMAIF_DEFAULT = 0,
    SMAIF_EXTENDED = 1,
    SMAIF_USAGEINFO = 2
} START_MENU_APP_ITEMS_FLAGS;


// IApplicationResolver
#ifndef __IApplicationResolver_INTERFACE_DEFINED__
#define __IApplicationResolver_INTERFACE_DEFINED__

typedef struct IApplicationResolver IApplicationResolver;

typedef struct IApplicationResolverVtbl {
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IApplicationResolver, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(
        _In_ IApplicationResolver* This,
        _In_ REFIID riid,
        _COM_Outptr_ void** ppvObject
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(
        _In_ IApplicationResolver* This
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, Release)
    ULONG (STDMETHODCALLTYPE* Release)(
        _In_ IApplicationResolver* This
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, GetAppIDForShortcut)
    HRESULT (STDMETHODCALLTYPE* GetAppIDForShortcut)(
        _In_ IApplicationResolver* This,
        _In_ IShellItem* psi,
        _Outptr_ PWSTR* ppszAppID
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, GetAppIDForWindow)
    HRESULT (STDMETHODCALLTYPE* GetAppIDForWindow)(
        _In_ IApplicationResolver* This,
        _In_ HWND WindowHandle,
        _Outptr_ PWSTR* ppszAppID,
        _Out_opt_ PBOOL pfPinningPrevented,
        _Out_opt_ PBOOL pfExplicitAppID,
        _Out_opt_ PBOOL pfEmbeddedShortcutValid
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, GetAppIDForProcess)
    HRESULT (STDMETHODCALLTYPE* GetAppIDForProcess)(
        _In_ IApplicationResolver* This,
        _In_ ULONG dwProcessID,
        _Outptr_ PWSTR* ppszAppID,
        _Out_opt_ PBOOL pfPinningPrevented,
        _Out_opt_ PBOOL pfExplicitAppID,
        _Out_opt_ PBOOL pfEmbeddedShortcutValid
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, GetShortcutForProcess)
    HRESULT (STDMETHODCALLTYPE* GetShortcutForProcess)(
        _In_ IApplicationResolver* This,
        _In_ ULONG dwProcessID,
        _Outptr_ IShellItem** ppsi
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, GetBestShortcutForAppID)
    HRESULT (STDMETHODCALLTYPE* GetBestShortcutForAppID)(
        _In_ IApplicationResolver* This,
        _In_ PCWSTR pszAppID,
        _Outptr_ IShellItem** ppsi
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, GetBestShortcutAndAppIDForAppPath)
    HRESULT (STDMETHODCALLTYPE* GetBestShortcutAndAppIDForAppPath)(
        _In_ IApplicationResolver* This,
        _In_ PCWSTR pszAppPath,
        _Outptr_opt_ IShellItem** ppsi,
        _Outptr_opt_ PWSTR* ppszAppID
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, CanPinApp)
    HRESULT (STDMETHODCALLTYPE* CanPinApp)(
        _In_ IApplicationResolver* This,
        _In_ IShellItem* psi
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, GetRelaunchProperties)
    HRESULT (STDMETHODCALLTYPE* GetRelaunchProperties)(
        _In_ IApplicationResolver* This,
        _In_ HWND WindowHandle,
        _Outptr_opt_result_maybenull_ PWSTR* ppszAppID,
        _Outptr_opt_result_maybenull_ PWSTR* ppszCmdLine,
        _Outptr_opt_result_maybenull_ PWSTR* ppszIconResource,
        _Outptr_opt_result_maybenull_ PWSTR* ppszDisplayNameResource,
        _Out_opt_ BOOL* pfPinnable
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, GenerateShortcutFromWindowProperties)
    HRESULT (STDMETHODCALLTYPE* GenerateShortcutFromWindowProperties)(
        _In_ IApplicationResolver* This,
        _Outptr_ IShellItem** ppsi
        );

    DECLSPEC_XFGVIRT(IApplicationResolver, GenerateShortcutFromItemProperties)
    HRESULT (STDMETHODCALLTYPE* GenerateShortcutFromItemProperties)(
        _In_ IApplicationResolver* This,
        _In_ IShellItem2* psi2,
        _Out_opt_ IShellItem** ppsi
        );

    END_INTERFACE
} IApplicationResolverVtbl;

interface IApplicationResolver
{
    CONST_VTBL struct IApplicationResolverVtbl* lpVtbl;
};

#ifdef COBJMACROS
#define IApplicationResolver_QueryInterface(This,riid,ppvObject) ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IApplicationResolver_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IApplicationResolver_Release(This) ((This)->lpVtbl->Release(This))
#define IApplicationResolver_GetAppIDForShortcut(This,psi,ppszAppID) ((This)->lpVtbl->GetAppIDForShortcut(This,psi,ppszAppID))
#define IApplicationResolver_GetAppIDForWindow(This,hWnd,ppszAppID,pPrev,pExp,pEmb) ((This)->lpVtbl->GetAppIDForWindow(This,hWnd,ppszAppID,pPrev,pExp,pEmb))
#define IApplicationResolver_GetAppIDForProcess(This,pid,ppszAppID,pPrev,pExp,pEmb) ((This)->lpVtbl->GetAppIDForProcess(This,pid,ppszAppID,pPrev,pExp,pEmb))
#define IApplicationResolver_GetShortcutForProcess(This,pid,ppsi) ((This)->lpVtbl->GetShortcutForProcess(This,pid,ppsi))
#define IApplicationResolver_GetBestShortcutForAppID(This,appId,ppsi) ((This)->lpVtbl->GetBestShortcutForAppID(This,appId,ppsi))
#define IApplicationResolver_GetBestShortcutAndAppIDForAppPath(This,path,ppsi,ppszAppID) ((This)->lpVtbl->GetBestShortcutAndAppIDForAppPath(This,path,ppsi,ppszAppID))
#define IApplicationResolver_CanPinApp(This,psi) ((This)->lpVtbl->CanPinApp(This,psi))
#define IApplicationResolver_GetRelaunchProperties(This,hWnd,ppszAppID,ppszCmd,ppszIcon,ppszDisplay,pPinnable) ((This)->lpVtbl->GetRelaunchProperties(This,hWnd,ppszAppID,ppszCmd,ppszIcon,ppszDisplay,pPinnable))
#define IApplicationResolver_GenerateShortcutFromWindowProperties(This,ppsi) ((This)->lpVtbl->GenerateShortcutFromWindowProperties(This,ppsi))
#define IApplicationResolver_GenerateShortcutFromItemProperties(This,psi2,ppsi) ((This)->lpVtbl->GenerateShortcutFromItemProperties(This,psi2,ppsi))
#endif // COBJMACROS

#endif // __IApplicationResolver_INTERFACE_DEFINED__


typedef enum tagAPP_RESOLVER_ITEM_FILTER_FLAGS
{
    ARIFF_NONE = 0,
    ARIFF_REQUIRE_PREVENT_PINNING_NOT_SET = 1,
    ARIFF_REQUIRE_PINNABLE = 2,
} APP_RESOLVER_ITEM_FILTER_FLAGS;


// IApplicationResolver2
#ifndef __IApplicationResolver2_INTERFACE_DEFINED__
#define __IApplicationResolver2_INTERFACE_DEFINED__

typedef struct IApplicationResolver2 IApplicationResolver2;

typedef struct IApplicationResolver2Vtbl 
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IApplicationResolver2, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(
        _In_ IApplicationResolver2* This,
        _In_ REFIID riid,
        _COM_Outptr_ void** ppvObject
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(
        _In_ IApplicationResolver2* This
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, Release)
    ULONG (STDMETHODCALLTYPE* Release)(
        _In_ IApplicationResolver2* This
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GetAppIDForShortcut)
    HRESULT (STDMETHODCALLTYPE* GetAppIDForShortcut)(
        _In_ IApplicationResolver2* This,
        _In_ IShellItem* psi,
        _Outptr_ PWSTR* ppszAppID
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GetAppIDForShortcutObject)
    HRESULT (STDMETHODCALLTYPE* GetAppIDForShortcutObject)(
        _In_ IApplicationResolver2* This,
        _In_ IShellLinkW* psl,
        _In_ IShellItem* psi,
        _Outptr_ PWSTR* ppszAppID
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GetAppIDForWindow)
    HRESULT (STDMETHODCALLTYPE* GetAppIDForWindow)(
        _In_ IApplicationResolver2* This,
        _In_ HWND WindowHandle,
        _Outptr_ PWSTR* ppszAppID,
        _Out_opt_ BOOL* pfPinningPrevented,
        _Out_opt_ BOOL* pfExplicitAppID,
        _Out_opt_ BOOL* pfEmbeddedShortcutValid
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GetAppIDForProcess)
    HRESULT (STDMETHODCALLTYPE* GetAppIDForProcess)(
        _In_ IApplicationResolver2* This,
        _In_ ULONG dwProcessID,
        _Outptr_ PWSTR* ppszAppID,
        _Out_opt_ BOOL* pfPinningPrevented,
        _Out_opt_ BOOL* pfExplicitAppID,
        _Out_opt_ BOOL* pfEmbeddedShortcutValid
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GetShortcutForProcess)
    HRESULT (STDMETHODCALLTYPE* GetShortcutForProcess)(
        _In_ IApplicationResolver2* This,
        _In_ ULONG dwProcessID,
        _Outptr_ IShellItem** ppsi
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GetBestShortcutForAppID)
    HRESULT (STDMETHODCALLTYPE* GetBestShortcutForAppID)(
        _In_ IApplicationResolver2* This,
        _In_ PCWSTR pszAppID,
        _Outptr_ IShellItem** ppsi
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GetBestShortcutAndAppIDForAppPath)
    HRESULT (STDMETHODCALLTYPE* GetBestShortcutAndAppIDForAppPath)(
        _In_ IApplicationResolver2* This,
        _In_ PCWSTR pszAppPath,
        _Outptr_opt_ IShellItem** ppsi,
        _Outptr_opt_ PWSTR* ppszAppID
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, CanPinApp)
    HRESULT (STDMETHODCALLTYPE* CanPinApp)(
        _In_ IApplicationResolver2* This,
        _In_ IShellItem* psi
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, CanPinAppShortcut)
    HRESULT (STDMETHODCALLTYPE* CanPinAppShortcut)(
        _In_ IApplicationResolver2* This,
        _In_ IShellLinkW* psl,
        _In_ IShellItem* psi
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GetRelaunchProperties)
    HRESULT (STDMETHODCALLTYPE* GetRelaunchProperties)(
        _In_ IApplicationResolver2* This,
        _In_ HWND WindowHandle,
        _Outptr_opt_result_maybenull_ PWSTR* ppszAppID,
        _Outptr_opt_result_maybenull_ PWSTR* ppszCmdLine,
        _Outptr_opt_result_maybenull_ PWSTR* ppszIconResource,
        _Outptr_opt_result_maybenull_ PWSTR* ppszDisplayNameResource,
        _Out_opt_ BOOL* pfPinnable
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GenerateShortcutFromWindowProperties)
    HRESULT (STDMETHODCALLTYPE* GenerateShortcutFromWindowProperties)(
        _In_ IApplicationResolver2* This,
        _Outptr_ IShellItem** ppsi
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GenerateShortcutFromItemProperties)
    HRESULT (STDMETHODCALLTYPE* GenerateShortcutFromItemProperties)(
        _In_ IApplicationResolver2* This,
        _In_ IShellItem2* psi2,
        _Out_opt_ IShellItem** ppsi
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GetLauncherAppIDForItem)
    HRESULT (STDMETHODCALLTYPE* GetLauncherAppIDForItem)(
        _In_ IApplicationResolver2* This,
        _In_ IShellItem* psi,
        _Outptr_opt_ PWSTR* ppszAppID
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GetShortcutForAppID)
    HRESULT (STDMETHODCALLTYPE* GetShortcutForAppID)(
        _In_ IApplicationResolver2* This,
        _In_ PCWSTR ppszAppID,
        _Out_opt_ IShellItem** ppsi
        );

    DECLSPEC_XFGVIRT(IApplicationResolver2, GetLauncherAppIDForItemEx)
    HRESULT (STDMETHODCALLTYPE* GetLauncherAppIDForItemEx)(
        _In_ IApplicationResolver2* This,
        _In_ IShellItem* psi,
        _In_ APP_RESOLVER_ITEM_FILTER_FLAGS Flags,
        _Outptr_opt_ PWSTR* ppszAppID
        );

    END_INTERFACE
} IApplicationResolver2Vtbl;

interface IApplicationResolver2 
{ 
    CONST_VTBL struct IApplicationResolver2Vtbl* lpVtbl; 
};

#ifdef COBJMACROS
#define IApplicationResolver2_QueryInterface(This,riid,ppvObject) ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IApplicationResolver2_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IApplicationResolver2_Release(This) ((This)->lpVtbl->Release(This))
#define IApplicationResolver2_GetAppIDForShortcut(This,psi,ppszAppID) ((This)->lpVtbl->GetAppIDForShortcut(This,psi,ppszAppID))
#define IApplicationResolver2_GetAppIDForShortcutObject(This,psl,psi,ppszAppID) ((This)->lpVtbl->GetAppIDForShortcutObject(This,psl,psi,ppszAppID))
#define IApplicationResolver2_GetAppIDForWindow(This,hWnd,ppszAppID,pPrev,pExp,pEmb) ((This)->lpVtbl->GetAppIDForWindow(This,hWnd,ppszAppID,pPrev,pExp,pEmb))
#define IApplicationResolver2_GetAppIDForProcess(This,pid,ppszAppID,pPrev,pExp,pEmb) ((This)->lpVtbl->GetAppIDForProcess(This,pid,ppszAppID,pPrev,pExp,pEmb))
#define IApplicationResolver2_GetShortcutForProcess(This,pid,ppsi) ((This)->lpVtbl->GetShortcutForProcess(This,pid,ppsi))
#define IApplicationResolver2_GetBestShortcutForAppID(This,appId,ppsi) ((This)->lpVtbl->GetBestShortcutForAppID(This,appId,ppsi))
#define IApplicationResolver2_GetBestShortcutAndAppIDForAppPath(This,path,ppsi,ppszAppID) ((This)->lpVtbl->GetBestShortcutAndAppIDForAppPath(This,path,ppsi,ppszAppID))
#define IApplicationResolver2_CanPinApp(This,psi) ((This)->lpVtbl->CanPinApp(This,psi))
#define IApplicationResolver2_CanPinAppShortcut(This,psl,psi) ((This)->lpVtbl->CanPinAppShortcut(This,psl,psi))
#define IApplicationResolver2_GetRelaunchProperties(This,hWnd,ppszAppID,ppszCmd,ppszIcon,ppszDisplay,pPinnable) ((This)->lpVtbl->GetRelaunchProperties(This,hWnd,ppszAppID,ppszCmd,ppszIcon,ppszDisplay,pPinnable))
#define IApplicationResolver2_GenerateShortcutFromWindowProperties(This,ppsi) ((This)->lpVtbl->GenerateShortcutFromWindowProperties(This,ppsi))
#define IApplicationResolver2_GenerateShortcutFromItemProperties(This,psi2,ppsi) ((This)->lpVtbl->GenerateShortcutFromItemProperties(This,psi2,ppsi))
#define IApplicationResolver2_GetLauncherAppIDForItem(This,psi,ppszAppID) ((This)->lpVtbl->GetLauncherAppIDForItem(This,psi,ppszAppID))
#define IApplicationResolver2_GetShortcutForAppID(This,appId,ppsi) ((This)->lpVtbl->GetShortcutForAppID(This,appId,ppsi))
#define IApplicationResolver2_GetLauncherAppIDForItemEx(This,psi,Flags,ppszAppID) ((This)->lpVtbl->GetLauncherAppIDForItemEx(This,psi,Flags,ppszAppID))
#endif // COBJMACROS

#endif // __IApplicationResolver2_INTERFACE_DEFINED__


// IStartMenuAppItems
#ifndef __IStartMenuAppItems_INTERFACE_DEFINED__
#define __IStartMenuAppItems_INTERFACE_DEFINED__

typedef struct IStartMenuAppItems IStartMenuAppItems;

typedef struct IStartMenuAppItemsVtbl
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IStartMenuAppItems, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(
        _In_ IStartMenuAppItems* This, 
        _In_ REFIID riid,
        _COM_Outptr_ void** ppvObject
        );

    DECLSPEC_XFGVIRT(IStartMenuAppItems, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(
        _In_ IStartMenuAppItems* This);

    DECLSPEC_XFGVIRT(IStartMenuAppItems, Release)
    ULONG (STDMETHODCALLTYPE* Release)(
        _In_ IStartMenuAppItems* This
    );

    DECLSPEC_XFGVIRT(IStartMenuAppItems, EnumItems)
    HRESULT (STDMETHODCALLTYPE* EnumItems)(
        _In_ IStartMenuAppItems* This, 
        _In_ START_MENU_APP_ITEMS_FLAGS Flags,
        _In_ REFIID riid,
         _Outptr_ PVOID* ppvObject
        );

    DECLSPEC_XFGVIRT(IStartMenuAppItems, GetItem)
    HRESULT (STDMETHODCALLTYPE* GetItem)(
        _In_ IStartMenuAppItems* This,
        _In_ START_MENU_APP_ITEMS_FLAGS Flags,
        _In_ PCWSTR AppUserModelId,
        _In_ REFIID riid,
        _Outptr_ PVOID* ppvObject
        );

    END_INTERFACE
} IStartMenuAppItemsVtbl;

struct IStartMenuAppItems
{ 
    CONST_VTBL struct IStartMenuAppItemsVtbl* lpVtbl; 
};

#ifdef COBJMACROS
#define IStartMenuAppItems_QueryInterface(This,riid,ppvObject) ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IStartMenuAppItems_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IStartMenuAppItems_Release(This) ((This)->lpVtbl->Release(This))
#define IStartMenuAppItems_EnumItems(This,Flags,riid,ppvObject) ((This)->lpVtbl->EnumItems(This,Flags,riid,ppvObject))
#define IStartMenuAppItems_GetItem(This,Flags,AUMID,riid,ppvObject) ((This)->lpVtbl->GetItem(This,Flags,AUMID,riid,ppvObject))
#endif // COBJMACROS

#endif // __IStartMenuAppItems_INTERFACE_DEFINED__

// IStartMenuAppItems2
#ifndef __IStartMenuAppItems2_INTERFACE_DEFINED__
#define __IStartMenuAppItems2_INTERFACE_DEFINED__

typedef struct IStartMenuAppItems2 IStartMenuAppItems2;

typedef struct IStartMenuAppItems2Vtbl 
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IStartMenuAppItems2, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(
        _In_ IStartMenuAppItems2* This,
        _In_ REFIID riid,
        _COM_Outptr_ void** ppvObject
        );

    DECLSPEC_XFGVIRT(IStartMenuAppItems2, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(
        _In_ IStartMenuAppItems2* This
        );

    DECLSPEC_XFGVIRT(IStartMenuAppItems2, Release)
    ULONG (STDMETHODCALLTYPE* Release)(
        _In_ IStartMenuAppItems2* This
        );

    DECLSPEC_XFGVIRT(IStartMenuAppItems2, EnumItems)
    HRESULT (STDMETHODCALLTYPE* EnumItems)(
        _In_ IStartMenuAppItems2* This,
        _In_ START_MENU_APP_ITEMS_FLAGS Flags,
        _In_ REFIID riid,
        _Outptr_ PVOID* ppvObject
        );

    DECLSPEC_XFGVIRT(IStartMenuAppItems2, GetItem)
    HRESULT (STDMETHODCALLTYPE* GetItem)(
        _In_ IStartMenuAppItems2* This,
        _In_ START_MENU_APP_ITEMS_FLAGS Flags,
        _In_ PCWSTR AppUserModelId,
        _In_ REFIID riid,
        _Outptr_ PVOID* ppvObject
        );

    DECLSPEC_XFGVIRT(IStartMenuAppItems2, GetItemByAppPath)
    HRESULT (STDMETHODCALLTYPE* GetItemByAppPath)(
        _In_ IStartMenuAppItems2* This,
        _In_ PCWSTR AppPath,
        _In_ REFIID riid,
        _Outptr_ PVOID* ppvObject
        );

    DECLSPEC_XFGVIRT(IStartMenuAppItems2, EnumCachedItems)
    HRESULT (STDMETHODCALLTYPE* EnumCachedItems)(
        _In_ IStartMenuAppItems2* This,
        _In_ PCIDLIST_ABSOLUTE pidl,
        _In_ REFIID riid,
        _Outptr_ PVOID* ppvObject
        );

    END_INTERFACE
} IStartMenuAppItems2Vtbl;

interface IStartMenuAppItems2 
{ 
    CONST_VTBL struct IStartMenuAppItems2Vtbl* lpVtbl;
};

#ifdef COBJMACROS
#define IStartMenuAppItems2_QueryInterface(This,riid,ppvObject) ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IStartMenuAppItems2_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IStartMenuAppItems2_Release(This) ((This)->lpVtbl->Release(This))
#define IStartMenuAppItems2_EnumItems(This,Flags,riid,ppvObject) ((This)->lpVtbl->EnumItems(This,Flags,riid,ppvObject))
#define IStartMenuAppItems2_GetItem(This,Flags,AUMID,riid,ppvObject) ((This)->lpVtbl->GetItem(This,Flags,AUMID,riid,ppvObject))
#define IStartMenuAppItems2_GetItemByAppPath(This,AppPath,riid,ppvObject) ((This)->lpVtbl->GetItemByAppPath(This,AppPath,riid,ppvObject))
#define IStartMenuAppItems2_EnumCachedItems(This,pidl,riid,ppvObject) ((This)->lpVtbl->EnumCachedItems(This,pidl,riid,ppvObject))
#endif // COBJMACROS

#endif // __IStartMenuAppItems2_INTERFACE_DEFINED__

// IMrtResourceManager
#ifndef __IMrtResourceManager_INTERFACE_DEFINED__
#define __IMrtResourceManager_INTERFACE_DEFINED__

typedef struct IMrtResourceManager IMrtResourceManager;

typedef struct IMrtResourceManagerVtbl
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IMrtResourceManager, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(_In_ IMrtResourceManager* This, _In_ REFIID riid, _COM_Outptr_ void** ppvObject);

    DECLSPEC_XFGVIRT(IMrtResourceManager, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(_In_ IMrtResourceManager* This);

    DECLSPEC_XFGVIRT(IMrtResourceManager, Release)
    ULONG (STDMETHODCALLTYPE* Release)(_In_ IMrtResourceManager* This);

    DECLSPEC_XFGVIRT(IMrtResourceManager, Initialize)
    HRESULT (STDMETHODCALLTYPE* Initialize)(_In_ IMrtResourceManager* This);

    DECLSPEC_XFGVIRT(IMrtResourceManager, InitializeForCurrentApplication)
    HRESULT (STDMETHODCALLTYPE* InitializeForCurrentApplication)(_In_ IMrtResourceManager* This);

    DECLSPEC_XFGVIRT(IMrtResourceManager, InitializeForPackage)
    HRESULT (STDMETHODCALLTYPE* InitializeForPackage)(_In_ IMrtResourceManager* This, _In_ PCWSTR PackageName);

    DECLSPEC_XFGVIRT(IMrtResourceManager, InitializeForFile)
    HRESULT (STDMETHODCALLTYPE* InitializeForFile)(_In_ IMrtResourceManager* This, _In_ PCWSTR FilePath);

    DECLSPEC_XFGVIRT(IMrtResourceManager, GetMainResourceMap)
    HRESULT (STDMETHODCALLTYPE* GetMainResourceMap)(_In_ IMrtResourceManager* This, _In_ REFIID riid, _Outptr_ PVOID* ppvObject);

    DECLSPEC_XFGVIRT(IMrtResourceManager, GetResourceMap)
    HRESULT (STDMETHODCALLTYPE* GetResourceMap)(_In_ IMrtResourceManager* This, _In_ PCWSTR Name, _In_ REFIID riid, _Outptr_ PVOID* ppvObject);

    DECLSPEC_XFGVIRT(IMrtResourceManager, GetDefaultContext)
    HRESULT (STDMETHODCALLTYPE* GetDefaultContext)(_In_ IMrtResourceManager* This, _In_ REFIID riid, _Outptr_ PVOID* ppvObject);

    DECLSPEC_XFGVIRT(IMrtResourceManager, GetReference)
    HRESULT (STDMETHODCALLTYPE* GetReference)(_In_ IMrtResourceManager* This, _In_ REFIID riid, _Outptr_ PVOID* ppvObject);

    DECLSPEC_XFGVIRT(IMrtResourceManager, IsResourceReference)
    HRESULT (STDMETHODCALLTYPE* IsResourceReference)(_In_ IMrtResourceManager* This, _In_ PCWSTR Name, _Out_ BOOL* IsReference);

    END_INTERFACE
} IMrtResourceManagerVtbl;

interface IMrtResourceManager 
{ 
    CONST_VTBL struct IMrtResourceManagerVtbl* lpVtbl; 
};

#ifdef COBJMACROS
#define IMrtResourceManager_QueryInterface(This,riid,ppvObject) ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IMrtResourceManager_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IMrtResourceManager_Release(This) ((This)->lpVtbl->Release(This))
#define IMrtResourceManager_Initialize(This) ((This)->lpVtbl->Initialize(This))
#define IMrtResourceManager_InitializeForCurrentApplication(This) ((This)->lpVtbl->InitializeForCurrentApplication(This))
#define IMrtResourceManager_InitializeForPackage(This,PackageName) ((This)->lpVtbl->InitializeForPackage(This,PackageName))
#define IMrtResourceManager_InitializeForFile(This,FilePath) ((This)->lpVtbl->InitializeForFile(This,FilePath))
#define IMrtResourceManager_GetMainResourceMap(This,riid,ppvObject) ((This)->lpVtbl->GetMainResourceMap(This,riid,ppvObject))
#define IMrtResourceManager_GetResourceMap(This,Name,riid,ppvObject) ((This)->lpVtbl->GetResourceMap(This,Name,riid,ppvObject))
#define IMrtResourceManager_GetDefaultContext(This,riid,ppvObject) ((This)->lpVtbl->GetDefaultContext(This,riid,ppvObject))
#define IMrtResourceManager_GetReference(This,riid,ppvObject) ((This)->lpVtbl->GetReference(This,riid,ppvObject))
#define IMrtResourceManager_IsResourceReference(This,Name,IsReference) ((This)->lpVtbl->IsResourceReference(This,Name,IsReference))
#endif // COBJMACROS

#endif // __IMrtResourceManager_INTERFACE_DEFINED__


// IResourceContext
#ifndef __IResourceContext_INTERFACE_DEFINED__
#define __IResourceContext_INTERFACE_DEFINED__

typedef struct IResourceContext IResourceContext;

typedef struct IResourceContextVtbl 
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IResourceContext, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(
        _In_ IResourceContext* This,
        _In_ REFIID riid,
        _COM_Outptr_ void** ppvObject
        );

    DECLSPEC_XFGVIRT(IResourceContext, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(
        _In_ IResourceContext* This
        );

    DECLSPEC_XFGVIRT(IResourceContext, Release)
    ULONG (STDMETHODCALLTYPE* Release)(
        _In_ IResourceContext* This
        );

    DECLSPEC_XFGVIRT(IResourceContext, GetLanguage)
    HRESULT (STDMETHODCALLTYPE* GetLanguage)(
        _In_ IResourceContext* This,
        _COM_Outptr_result_maybenull_ PWSTR* Language
        );

    DECLSPEC_XFGVIRT(IResourceContext, GetHomeRegion)
    HRESULT (STDMETHODCALLTYPE* GetHomeRegion)(
        _In_ IResourceContext* This,
        _COM_Outptr_result_maybenull_ PCWSTR* Region
        );

    DECLSPEC_XFGVIRT(IResourceContext, GetLayoutDirection)
    HRESULT (STDMETHODCALLTYPE* GetLayoutDirection)(
        _In_ IResourceContext* This,
        _Out_ ULONG* LayoutDirection
        );

    DECLSPEC_XFGVIRT(IResourceContext, GetTargetSize)
    HRESULT (STDMETHODCALLTYPE* GetTargetSize)(
        _In_ IResourceContext* This,
        _Out_ USHORT* TargetSize
        );

    DECLSPEC_XFGVIRT(IResourceContext, GetScale)
    HRESULT (STDMETHODCALLTYPE* GetScale)(
        _In_ IResourceContext* This,
        _Out_ ULONG* Scale
        );

    DECLSPEC_XFGVIRT(IResourceContext, GetContrast)
    HRESULT (STDMETHODCALLTYPE* GetContrast)(
        _In_ IResourceContext* This,
        _Out_ ULONG* Contrast
        );

    DECLSPEC_XFGVIRT(IResourceContext, GetAlternateForm)
    HRESULT (STDMETHODCALLTYPE* GetAlternateForm)(
        _In_ IResourceContext* This,
        _COM_Outptr_result_maybenull_ PCWSTR* AlternateForm
        );

    DECLSPEC_XFGVIRT(IResourceContext, GetQualifierValue)
    HRESULT (STDMETHODCALLTYPE* GetQualifierValue)(
        _In_ IResourceContext* This,
        _In_ PCWSTR QualifierName,
        _COM_Outptr_result_maybenull_ PWSTR* QualifierValue
        );

    DECLSPEC_XFGVIRT(IResourceContext, SetLanguage)
    HRESULT (STDMETHODCALLTYPE* SetLanguage)(
        _In_ IResourceContext* This,
        _In_ PCWSTR Language
        );

    DECLSPEC_XFGVIRT(IResourceContext, SetHomeRegion)
    HRESULT (STDMETHODCALLTYPE* SetHomeRegion)(
        _In_ IResourceContext* This,
        _In_ PCWSTR Region
        );

    DECLSPEC_XFGVIRT(IResourceContext, SetLayoutDirection)
    HRESULT (STDMETHODCALLTYPE* SetLayoutDirection)(
        _In_ IResourceContext* This,
        _In_ ULONG LayoutDirection
        );

    DECLSPEC_XFGVIRT(IResourceContext, SetTargetSize)
    HRESULT (STDMETHODCALLTYPE* SetTargetSize)(
        _In_ IResourceContext* This,
        _In_ USHORT TargetSize
        );

    DECLSPEC_XFGVIRT(IResourceContext, SetScale)
    HRESULT (STDMETHODCALLTYPE* SetScale)(
        _In_ IResourceContext* This,
        _In_ ULONG Scale
        );

    DECLSPEC_XFGVIRT(IResourceContext, SetContrast)
    HRESULT (STDMETHODCALLTYPE* SetContrast)(
        _In_ IResourceContext* This,
        _In_ ULONG Contrast
        );

    DECLSPEC_XFGVIRT(IResourceContext, SetAlternateForm)
    HRESULT (STDMETHODCALLTYPE* SetAlternateForm)(
        _In_ IResourceContext* This,
        _In_ PCWSTR AlternateForm
        );

    DECLSPEC_XFGVIRT(IResourceContext, SetQualifierValue)
    HRESULT (STDMETHODCALLTYPE* SetQualifierValue)(
        _In_ IResourceContext* This,
        _In_ PCWSTR QualifierName,
        _In_ PCWSTR QualifierValue
        );

    DECLSPEC_XFGVIRT(IResourceContext, TrySetQualifierValue)
    HRESULT (STDMETHODCALLTYPE* TrySetQualifierValue)(
        _In_ IResourceContext* This,
        _In_ PCWSTR QualifierName,
        _In_ LPCWSTR QualifierValue,
        _Out_ HRESULT* Result
        );

    DECLSPEC_XFGVIRT(IResourceContext, Reset)
    HRESULT (STDMETHODCALLTYPE* Reset)(
        _In_ IResourceContext* This
        );

    DECLSPEC_XFGVIRT(IResourceContext, ResetQualifierValue)
    HRESULT (STDMETHODCALLTYPE* ResetQualifierValue)(
        _In_ IResourceContext* This,
        _In_ LPCWSTR QualifierName
        );

    DECLSPEC_XFGVIRT(IResourceContext, Clone)
    HRESULT (STDMETHODCALLTYPE* Clone)(
        _In_ IResourceContext* This,
        _COM_Outptr_ IResourceContext** CloneContext
        );

    DECLSPEC_XFGVIRT(IResourceContext, OverrideToMatch)
    HRESULT (STDMETHODCALLTYPE* OverrideToMatch)(
        _In_ IResourceContext* This,
        _In_ struct IResourceCandidate* Candidate
        );

    END_INTERFACE
} IResourceContextVtbl;

interface IResourceContext 
{ 
    CONST_VTBL struct IResourceContextVtbl* lpVtbl; 
};

#ifdef COBJMACROS
#define IResourceContext_QueryInterface(This,riid,ppvObject) ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IResourceContext_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IResourceContext_Release(This) ((This)->lpVtbl->Release(This))
#define IResourceContext_GetLanguage(This,ppLang) ((This)->lpVtbl->GetLanguage(This,ppLang))
#define IResourceContext_GetHomeRegion(This,ppRegion) ((This)->lpVtbl->GetHomeRegion(This,ppRegion))
#define IResourceContext_GetLayoutDirection(This,pDir) ((This)->lpVtbl->GetLayoutDirection(This,pDir))
#define IResourceContext_GetTargetSize(This,pSize) ((This)->lpVtbl->GetTargetSize(This,pSize))
#define IResourceContext_GetScale(This,pScale) ((This)->lpVtbl->GetScale(This,pScale))
#define IResourceContext_GetContrast(This,pContrast) ((This)->lpVtbl->GetContrast(This,pContrast))
#define IResourceContext_GetAlternateForm(This,ppForm) ((This)->lpVtbl->GetAlternateForm(This,ppForm))
#define IResourceContext_GetQualifierValue(This,key,ppVal) ((This)->lpVtbl->GetQualifierValue(This,key,ppVal))
#define IResourceContext_SetLanguage(This,val) ((This)->lpVtbl->SetLanguage(This,val))
#define IResourceContext_SetHomeRegion(This,val) ((This)->lpVtbl->SetHomeRegion(This,val))
#define IResourceContext_SetLayoutDirection(This,val) ((This)->lpVtbl->SetLayoutDirection(This,val))
#define IResourceContext_SetTargetSize(This,val) ((This)->lpVtbl->SetTargetSize(This,val))
#define IResourceContext_SetScale(This,val) ((This)->lpVtbl->SetScale(This,val))
#define IResourceContext_SetContrast(This,val) ((This)->lpVtbl->SetContrast(This,val))
#define IResourceContext_SetAlternateForm(This,val) ((This)->lpVtbl->SetAlternateForm(This,val))
#define IResourceContext_SetQualifierValue(This,key,val) ((This)->lpVtbl->SetQualifierValue(This,key,val))
#define IResourceContext_TrySetQualifierValue(This,key,val,pHr) ((This)->lpVtbl->TrySetQualifierValue(This,key,val,pHr))
#define IResourceContext_Reset(This) ((This)->lpVtbl->Reset(This))
#define IResourceContext_ResetQualifierValue(This,key) ((This)->lpVtbl->ResetQualifierValue(This,key))
#define IResourceContext_Clone(This,ppCtx) ((This)->lpVtbl->Clone(This,ppCtx))
#define IResourceContext_OverrideToMatch(This,pCand) ((This)->lpVtbl->OverrideToMatch(This,pCand))
#endif // COBJMACROS

#endif // __IResourceContext_INTERFACE_DEFINED__


// IResourceMap
#ifndef __IResourceMap_INTERFACE_DEFINED__
#define __IResourceMap_INTERFACE_DEFINED__

typedef struct IResourceMap IResourceMap;

typedef struct IResourceMapVtbl 
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IResourceMap, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(
        _In_ IResourceMap* This,
        _In_ REFIID riid,
        _COM_Outptr_ void** ppvObject
        );

    DECLSPEC_XFGVIRT(IResourceMap, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(
        _In_ IResourceMap* This
        );

    DECLSPEC_XFGVIRT(IResourceMap, Release)
    ULONG (STDMETHODCALLTYPE* Release)(
        _In_ IResourceMap* This
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetUri)
    HRESULT (STDMETHODCALLTYPE* GetUri)(
        _In_ IResourceMap* This,
        _COM_Outptr_ PWSTR* UriString
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetSubtree)
    HRESULT (STDMETHODCALLTYPE* GetSubtree)(
        _In_ IResourceMap* This,
        _In_ PCWSTR* Name,
        _COM_Outptr_ IResourceMap** ResourceMap
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetString)
    HRESULT (STDMETHODCALLTYPE* GetString)(
        _In_ IResourceMap* This,
        _In_ PCWSTR Key,
        _COM_Outptr_ PWSTR* Value
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetStringForContext)
    HRESULT (STDMETHODCALLTYPE* GetStringForContext)(
        _In_ IResourceMap* This,
        _In_ IResourceContext* Context,
        _In_ PCWSTR Key,
        _COM_Outptr_ PWSTR* Value
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetFilePath)
    HRESULT (STDMETHODCALLTYPE* GetFilePath)(
        _In_ IResourceMap* This,
        _In_ PCWSTR Key,
        _COM_Outptr_ PWSTR* Value
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetFilePathForContext)
    HRESULT (STDMETHODCALLTYPE* GetFilePathForContext)(
        _In_ IResourceMap* This,
        _In_ IResourceContext* Context,
        _In_ PCWSTR Key,
        _COM_Outptr_ PWSTR* Value
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetNamedResourceCount)
    HRESULT (STDMETHODCALLTYPE* GetNamedResourceCount)(
        _In_ IResourceMap* This,
        _Out_ PULONG Count
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetNamedResourceUri)
    HRESULT (STDMETHODCALLTYPE* GetNamedResourceUri)(
        _In_ IResourceMap* This,
        _In_ ULONG Index,
        _COM_Outptr_ PWSTR* UriString
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetNamedResource)
    HRESULT (STDMETHODCALLTYPE* GetNamedResource)(
        _In_ IResourceMap* This,
        _In_ PCWSTR,
        _In_ REFIID riid,
        _Outptr_ PVOID* ppvObject
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetFullyQualifiedReference)
    HRESULT (STDMETHODCALLTYPE* GetFullyQualifiedReference)(
        _In_ IResourceMap* This,
        _In_ LPCWSTR,
        _In_ LPCWSTR,
        _COM_Outptr_ PWSTR*
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetFilePathByUri)
    HRESULT (STDMETHODCALLTYPE* GetFilePathByUri)(
        _In_ IResourceMap* This,
        _In_ IUri* Uri,
        _COM_Outptr_ PWSTR* Path
        );

    DECLSPEC_XFGVIRT(IResourceMap, GetFilePathForContextByUri)
    HRESULT (STDMETHODCALLTYPE* GetFilePathForContextByUri)(
        _In_ IResourceMap* This,
        _In_ IResourceContext* Context,
        _In_ IUri* Uri,
        _COM_Outptr_ PWSTR* Path
        );

    END_INTERFACE
} IResourceMapVtbl;

interface IResourceMap 
{ 
    CONST_VTBL struct IResourceMapVtbl* lpVtbl; 
};

#ifdef COBJMACROS
#define IResourceMap_QueryInterface(This,riid,ppvObject) ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IResourceMap_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IResourceMap_Release(This) ((This)->lpVtbl->Release(This))
#define IResourceMap_GetUri(This,ppUri) ((This)->lpVtbl->GetUri(This,ppUri))
#define IResourceMap_GetSubtree(This,pName,ppMap) ((This)->lpVtbl->GetSubtree(This,pName,ppMap))
#define IResourceMap_GetString(This,key,ppVal) ((This)->lpVtbl->GetString(This,key,ppVal))
#define IResourceMap_GetStringForContext(This,ctx,key,ppVal) ((This)->lpVtbl->GetStringForContext(This,ctx,key,ppVal))
#define IResourceMap_GetFilePath(This,key,ppVal) ((This)->lpVtbl->GetFilePath(This,key,ppVal))
#define IResourceMap_GetFilePathForContext(This,ctx,key,ppVal) ((This)->lpVtbl->GetFilePathForContext(This,ctx,key,ppVal))
#define IResourceMap_GetNamedResourceCount(This,pCount) ((This)->lpVtbl->GetNamedResourceCount(This,pCount))
#define IResourceMap_GetNamedResourceUri(This,index,ppName) ((This)->lpVtbl->GetNamedResourceUri(This,index,ppName))
#define IResourceMap_GetNamedResource(This,name,riid,ppv) ((This)->lpVtbl->GetNamedResource(This,name,riid,ppv))
#define IResourceMap_GetFullyQualifiedReference(This,a,b,ppRef) ((This)->lpVtbl->GetFullyQualifiedReference(This,a,b,ppRef))
#define IResourceMap_GetFilePathByUri(This,pUri,ppPath) ((This)->lpVtbl->GetFilePathByUri(This,pUri,ppPath))
#define IResourceMap_GetFilePathForContextByUri(This,ctx,pUri,ppPath) ((This)->lpVtbl->GetFilePathForContextByUri(This,ctx,pUri,ppPath))
#endif // COBJMACROS

#endif // __IResourceMap_INTERFACE_DEFINED__

// Note: Documented PKEY_AppUserModel_XYZ keys can be found in propkey.h
DEFINE_PROPERTYKEY(PKEY_AppUserModel_ID, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 5);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_HostEnvironment, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 14);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_PackageInstallPath, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 15);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_PackageFamilyName, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 17);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_ParentID, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 19);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_PackageFullName, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 21);
// PKEY_AppUserModel_ExcludeFromShowInNewInstall {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 8
//3 PKEY_AppUserModel_IsDestListSeparator {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 6
//4 PKEY_AppUserModel_IsDualMode {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 11
//5 PKEY_AppUserModel_PreventPinning {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 9
//6 PKEY_AppUserModel_RelaunchCommand {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 2
//7 PKEY_AppUserModel_RelaunchDisplayNameResource {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 4
//8 PKEY_AppUserModel_RelaunchIconResource {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 3
//9 PKEY_AppUserModel_StartPinOption {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 12
//10 PKEY_AppUserModel_ToastActivatorCLSID {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 26
//12 PKEY_AppUserModel_RecordState {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 16
//14 PKEY_AppUserModel_DestListProvidedTitle {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 27
//15 PKEY_AppUserModel_DestListProvidedDescription {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 28
//18 PKEY_AppUserModel_Relevance {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 13
//19 PKEY_AppUserModel_PackageRelativeApplicationID {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 22
//20 PKEY_AppUserModel_ExcludedFromLauncher {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 23
//21 PKEY_AppUserModel_DestListLogoUri {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 29
//22 PKEY_AppUserModel_ActivationContext {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 20
//24 PKEY_AppUserModel_BestShortcut {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 10
//25 PKEY_AppUserModel_IsDestListLink {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 7
//26 PKEY_AppUserModel_InstalledBy {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 18
//27 PKEY_AppUserModel_RunFlags {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 25
//28 PKEY_AppUserModel_DestListProvidedGroupName {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 30
DEFINE_PROPERTYKEY(PKEY_Tile_SmallLogoPath, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 2);
DEFINE_PROPERTYKEY(PKEY_Tile_Background, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 4);
DEFINE_PROPERTYKEY(PKEY_Tile_Foreground, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 5);
DEFINE_PROPERTYKEY(PKEY_Tile_LongDisplayName, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 11);
DEFINE_PROPERTYKEY(PKEY_Tile_Flags, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 14);
DEFINE_PROPERTYKEY(PKEY_Tile_SuiteDisplayName, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 16);
DEFINE_PROPERTYKEY(PKEY_Tile_SuiteSortName, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 17);
DEFINE_PROPERTYKEY(PKEY_Tile_DisplayNameLanguage, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 18);
DEFINE_PROPERTYKEY(PKEY_Tile_BadgeLogoPath, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 15);
DEFINE_PROPERTYKEY(PKEY_Tile_Square150x150LogoPath, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 12);
DEFINE_PROPERTYKEY(PKEY_Tile_Wide310x150LogoPath, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 13);
DEFINE_PROPERTYKEY(PKEY_Tile_Square310x310LogoPath, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 19);
DEFINE_PROPERTYKEY(PKEY_Tile_Square70x70LogoPath, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 20);
DEFINE_PROPERTYKEY(PKEY_Tile_FencePost, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 21);
DEFINE_PROPERTYKEY(PKEY_Tile_InstallProgress, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 22);
DEFINE_PROPERTYKEY(PKEY_Tile_EncodedTargetPath, 0x86D40B4D, 0x9069, 0x443C, 0x81, 0x9A, 0x2A, 0x54, 0x09, 0x0D, 0xCC, 0xEC, 23);

// Immersive PLM task support

// "07fc2b94-5285-417e-8ac3-c2ce5240b0fa"
DEFINE_GUID(CLSID_OSTaskCompletion_I, 0x07fc2b94, 0x5285, 0x417e, 0x8a, 0xc3, 0xc2, 0xce, 0x52, 0x40, 0xb0, 0xfa);
// "c7e40572-c36a-43ea-9a40-f3b168da5558"
DEFINE_GUID(IID_IOSTaskCompletion_I, 0xc7e40572, 0xc36a, 0x43ea, 0x9a, 0x40, 0xf3, 0xb1, 0x68, 0xda, 0x55, 0x58);

typedef enum _PLM_TASKCOMPLETION_CATEGORY_FLAGS
{
    PT_TC_NONE = 0,
    PT_TC_PBM = 1,
    PT_TC_FILEOPENPICKER = 2,
    PT_TC_SHARING = 4,
    PT_TC_PRINTING = 8,
    PT_TC_GENERIC = 16,
    PT_TC_CAMERA_DCA = 32,
    PT_TC_PRINTER_DCA = 64,
    PT_TC_PLAYTO = 128,
    PT_TC_FILESAVEPICKER = 256,
    PT_TC_CONTACTPICKER = 512,
    PT_TC_CACHEDFILEUPDATER_LOCAL = 1024,
    PT_TC_CACHEDFILEUPDATER_REMOTE = 2048,
    PT_TC_ERROR_REPORT = 8192,
    PT_TC_DATA_PACKAGE = 16384,
    PT_TC_CRASHDUMP = 0x10000,
    PT_TC_STREAMEDFILE = 0x20000,
    PT_TC_PBM_COMMUNICATION = 0x80000,
    PT_TC_HOSTEDAPPLICATION = 0x100000,
    PT_TC_MEDIA_CONTROLS_ACTIVE = 0x200000,
    PT_TC_EMPTYHOST = 0x400000,
    PT_TC_SCANNING = 0x800000,
    PT_TC_ACTIONS = 0x1000000,
    PT_TC_KERNEL_MODE = 0x20000000,
    PT_TC_REALTIMECOMM = 0x40000000,
    PT_TC_IGNORE_NAV_LEVEL_FOR_CS = 0x80000000
} PLM_TASKCOMPLETION_CATEGORY_FLAGS;

// IOSTaskCompletion
#ifndef __IOSTaskCompletion_INTERFACE_DEFINED__
#define __IOSTaskCompletion_INTERFACE_DEFINED__

typedef struct IOSTaskCompletion IOSTaskCompletion;

typedef struct IOSTaskCompletionVtbl 
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IOSTaskCompletion, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(
        _In_ IOSTaskCompletion* This,
        _In_ REFIID riid,
        _COM_Outptr_ void** ppvObject
        );

    DECLSPEC_XFGVIRT(IOSTaskCompletion, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(
        _In_ IOSTaskCompletion* This
        );

    DECLSPEC_XFGVIRT(IOSTaskCompletion, Release)
    ULONG (STDMETHODCALLTYPE* Release)(
        _In_ IOSTaskCompletion* This
        );

    DECLSPEC_XFGVIRT(IOSTaskCompletion, BeginTask)
    HRESULT (STDMETHODCALLTYPE* BeginTask)(
        _In_ IOSTaskCompletion* This,
        _In_ ULONG ProcessId,
        _In_ PLM_TASKCOMPLETION_CATEGORY_FLAGS Flags
    );

    DECLSPEC_XFGVIRT(IOSTaskCompletion, BeginTaskByHandle)
    HRESULT (STDMETHODCALLTYPE* BeginTaskByHandle)(
        _In_ IOSTaskCompletion* This,
        _In_ HANDLE ProcessHandle,
        _In_ PLM_TASKCOMPLETION_CATEGORY_FLAGS Flags
        );

    DECLSPEC_XFGVIRT(IOSTaskCompletion, EndTask)
    HRESULT (STDMETHODCALLTYPE* EndTask)(
        _In_ IOSTaskCompletion* This
        );

    DECLSPEC_XFGVIRT(IOSTaskCompletion, ValidateStreamedFileTaskCompletionUsage)
    ULONG (STDMETHODCALLTYPE* ValidateStreamedFileTaskCompletionUsage)(
        _In_ IOSTaskCompletion* This,
        _In_ HANDLE ProcessHandle,
        _In_ ULONG ProcessId
        );

    END_INTERFACE
} IOSTaskCompletionVtbl;

interface IOSTaskCompletion 
{ 
    CONST_VTBL struct IOSTaskCompletionVtbl* lpVtbl;
};

#ifdef COBJMACROS
#define IOSTaskCompletion_QueryInterface(This,riid,ppvObject) ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IOSTaskCompletion_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IOSTaskCompletion_Release(This) ((This)->lpVtbl->Release(This))
#define IOSTaskCompletion_BeginTask(This,pid,Flags) ((This)->lpVtbl->BeginTask(This,pid,Flags))
#define IOSTaskCompletion_BeginTaskByHandle(This,hProc,Flags) ((This)->lpVtbl->BeginTaskByHandle(This,hProc,Flags))
#define IOSTaskCompletion_EndTask(This) ((This)->lpVtbl->EndTask(This))
#define IOSTaskCompletion_ValidateStreamedFileTaskCompletionUsage(This,hProc,pid) ((This)->lpVtbl->ValidateStreamedFileTaskCompletionUsage(This,hProc,pid))
#endif // COBJMACROS

#endif // __IOSTaskCompletion_INTERFACE_DEFINED__

// IOSTaskCompletion2
#ifndef __IOSTaskCompletion2_INTERFACE_DEFINED__
#define __IOSTaskCompletion2_INTERFACE_DEFINED__

typedef struct IOSTaskCompletion2 IOSTaskCompletion2;

typedef struct IOSTaskCompletion2Vtbl 
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IOSTaskCompletion2, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(
        _In_ IOSTaskCompletion2* This,
        _In_ REFIID riid,
        _COM_Outptr_ PVOID* ppvObject
        );

    DECLSPEC_XFGVIRT(IOSTaskCompletion2, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(
        _In_ IOSTaskCompletion2* This
        );

    DECLSPEC_XFGVIRT(IOSTaskCompletion2, Release)
    ULONG (STDMETHODCALLTYPE* Release)(
        _In_ IOSTaskCompletion2* This
        );

    DECLSPEC_XFGVIRT(IOSTaskCompletion2, InternalGetTrustLevelStatic)
    HRESULT (STDMETHODCALLTYPE* InternalGetTrustLevelStatic)(
        _In_ IOSTaskCompletion2* This
        );

    DECLSPEC_XFGVIRT(IOSTaskCompletion2, InternalGetTrustLevelStatic2)
    HRESULT (STDMETHODCALLTYPE* InternalGetTrustLevelStatic2)(
        _In_ IOSTaskCompletion2* This
        );

    DECLSPEC_XFGVIRT(IOSTaskCompletion2, EndAllTasksAndWait)
    HRESULT (STDMETHODCALLTYPE* EndAllTasksAndWait)(
        _In_ IOSTaskCompletion2* This
        );

    DECLSPEC_XFGVIRT(IOSTaskCompletion2, BeginTaskByHandleEx)
    HRESULT (STDMETHODCALLTYPE* BeginTaskByHandleEx)(
        _In_ IOSTaskCompletion2* This,
        _In_ HANDLE ProcessHandle,
        _In_ PLM_TASKCOMPLETION_CATEGORY_FLAGS Flags,
        _In_ enum PLM_TASKCOMPLETION_BEGIN_TASK_FLAGS TaskFlags
        );

    END_INTERFACE
} IOSTaskCompletion2Vtbl;

interface IOSTaskCompletion2 
{ 
    CONST_VTBL struct IOSTaskCompletion2Vtbl* lpVtbl;
};

#ifdef COBJMACROS
#define IOSTaskCompletion2_QueryInterface(This,riid,ppvObject) ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IOSTaskCompletion2_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define IOSTaskCompletion2_Release(This) ((This)->lpVtbl->Release(This))
#define IOSTaskCompletion2_InternalGetTrustLevelStatic(This) ((This)->lpVtbl->InternalGetTrustLevelStatic(This))
#define IOSTaskCompletion2_InternalGetTrustLevelStatic2(This) ((This)->lpVtbl->InternalGetTrustLevelStatic2(This))
#define IOSTaskCompletion2_EndAllTasksAndWait(This) ((This)->lpVtbl->EndAllTasksAndWait(This))
#define IOSTaskCompletion2_BeginTaskByHandleEx(This,hProc,Flags,TaskFlags) ((This)->lpVtbl->BeginTaskByHandleEx(This,hProc,Flags,TaskFlags))
#endif // COBJMACROS

#endif // __IOSTaskCompletion2_INTERFACE_DEFINED__

// EDP

typedef enum _EDP_CONTEXT_STATES
{
    EDP_CONTEXT_NONE = 0,
    EDP_CONTEXT_IS_EXEMPT = 1,
    EDP_CONTEXT_IS_ENLIGHTENED = 2,
    EDP_CONTEXT_IS_UNENLIGHTENED_ALLOWED = 4,
    EDP_CONTEXT_IS_PERMISSIVE = 8,
    EDP_CONTEXT_IS_COPY_EXEMPT = 16,
    EDP_CONTEXT_IS_DENIED = 32,
} EDP_CONTEXT_STATES;

typedef struct _EDP_CONTEXT
{
    EDP_CONTEXT_STATES contextStates;
    ULONG allowedEnterpriseIdCount;
    PWSTR enterpriseIdForUIEnforcement;
    WCHAR allowedEnterpriseIds[1];
} EDP_CONTEXT, *PEDP_CONTEXT;

static HRESULT (WINAPI* EdpGetContextForWindow_I)(
    _In_ HWND WindowHandle,
    _Out_ PEDP_CONTEXT* EdpContext
    ) = NULL;

static HRESULT (WINAPI* EdpGetContextForProcess_I)(
    _In_ ULONG ProcessId,
    _Out_ PEDP_CONTEXT* EdpContext
    ) = NULL;

static VOID (WINAPI* EdpFreeContext_I)(
    _In_ PEDP_CONTEXT EdpContext
    ) = NULL;

#endif
