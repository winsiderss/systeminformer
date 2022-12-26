/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2019
 *
 */

#ifndef _PH_APPRESOLVER_P_H
#define _PH_APPRESOLVER_P_H

// "660B90C8-73A9-4B58-8CAE-355B7F55341B"
static CLSID CLSID_StartMenuCacheAndAppResolver_I = { 0x660B90C8, 0x73A9, 0x4B58,{ 0x8C, 0xAE, 0x35, 0x5B, 0x7F, 0x55, 0x34, 0x1B } };
// "46A6EEFF-908E-4DC6-92A6-64BE9177B41C"
static IID IID_IApplicationResolver61_I = { 0x46A6EEFF, 0x908E, 0x4DC6,{ 0x92, 0xA6, 0x64, 0xBE, 0x91, 0x77, 0xB4, 0x1c } };
// "DE25675A-72DE-44b4-9373-05170450C140"
static IID IID_IApplicationResolver62_I = { 0xDE25675A, 0x72DE, 0x44b4,{ 0x93, 0x73, 0x05, 0x17, 0x04, 0x50, 0xC1, 0x40 } };
// "33F71155-C2E9-4FFE-9786-A32D98577CFF"
static IID IID_IStartMenuAppItems61_I = { 0x33F71155, 0xC2E9, 0x4FFE,{ 0x97, 0x86, 0xA3, 0x2D, 0x98, 0x57, 0x7C, 0xFF } };
// "02C5CCF3-805F-4654-A7B7-340A74335365"
static IID IID_IStartMenuAppItems62_I = { 0x02C5CCF3, 0x805F, 0x4654,{ 0xA7, 0xB7, 0x34, 0x0A, 0x74, 0x33, 0x53, 0x65 } };

// "DBCE7E40-7345-439D-B12C-114A11819A09"
static CLSID CLSID_MrtResourceManager_I = { 0xDBCE7E40, 0x7345, 0x439D,{ 0xB1, 0x2C, 0x11, 0x4A, 0x11, 0x81, 0x9A, 0x09 } };
// "130A2F65-2BE7-4309-9A58-A9052FF2B61C"
static IID IID_IMrtResourceManager_I = { 0x130A2F65, 0x2BE7, 0x4309,{ 0x9A, 0x58, 0xA9, 0x05, 0x2F, 0xF2, 0xB6, 0x1C } };
// "E3C22B30-8502-4B2F-9133-559674587E51"
static IID IID_IResourceContext_I = { 0xE3C22B30, 0x8502, 0x4B2F,{ 0x91, 0x33, 0x55, 0x96, 0x74, 0x58, 0x7E, 0x51 } };
// "6E21E72B-B9B0-42AE-A686-983CF784EDCD"
static IID IID_IResourceMap_I = { 0x6E21E72B, 0xB9B0, 0x42AE,{ 0xA6, 0x86, 0x98, 0x3C, 0xF7, 0x84, 0xED, 0xCD } };

static HRESULT (WINAPI* AppContainerDeriveSidFromMoniker_I)( // DeriveAppContainerSidFromAppContainerName
    _In_ PCWSTR AppContainerName,
    _Out_ PSID *AppContainerSid
    ) = NULL;

// Note: LookupAppContainerDisplayName (userenv.dll, ordinal 211) has the same prototype but returns 'PackageName/ContainerName'.
static HRESULT (WINAPI* AppContainerLookupMoniker_I)(
    _In_ PSID AppContainerSid,
    _Out_ PWSTR *PackageFamilyName
    ) = NULL;

static HRESULT (WINAPI* AppContainerRegisterSid_I)(
    _In_ PSID Sid,
    _In_ PCWSTR AppContainerName,
    _In_ PCWSTR DisplayName
    ) = NULL;

static HRESULT (WINAPI* AppContainerUnregisterSid_I)(
    _In_ PSID Sid
    ) = NULL;

static BOOL (WINAPI* AppContainerFreeMemory_I)(
    _Frees_ptr_opt_ PVOID Memory
    ) = NULL;

// rev
static NTSTATUS (NTAPI* PsmGetKeyFromProcess_I)(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID KeyBuffer,
    _Inout_ PULONG KeyLength
    ) = NULL;

// rev
static NTSTATUS (NTAPI* PsmGetKeyFromToken_I)(
    _In_ HANDLE TokenHandle,
    _Out_ PVOID KeyBuffer,
    _Inout_ PULONG KeyLength
    ) = NULL;

// rev
static NTSTATUS (NTAPI* PsmGetApplicationNameFromKey_I)(
    _In_ PVOID KeyBuffer,
    _Out_ PVOID NameBuffer,
    _Inout_ PULONG NameLength
    ) = NULL;

// rev
static NTSTATUS (NTAPI* PsmGetPackageFullNameFromKey_I)(
    _In_ PVOID KeyBuffer,
    _Out_ PVOID NameBuffer,
    _Inout_ PULONG NameLength
    ) = NULL;

// "168EB462-775F-42AE-9111-D714B2306C2E"
static CLSID CLSID_IDesktopAppXActivator_I = { 0x168EB462, 0x775F, 0x42AE, { 0x91, 0x11, 0xD7, 0x14, 0xB2, 0x30, 0x6C, 0x2E } };
// "72e3a5b0-8fea-485c-9f8b-822b16dba17f"
static IID IID_IDesktopAppXActivator1_I = { 0x72e3a5b0, 0x8fea, 0x485c, { 0x9f, 0x8b, 0x82, 0x2b, 0x16, 0xdb, 0xa1, 0x7f } };
// "F158268A-D5A5-45CE-99CF-00D6C3F3FC0A"
static IID IID_IDesktopAppXActivator2_I = { 0xF158268A, 0xD5A5, 0x45CE, { 0x99, 0xCF, 0x00, 0xD6, 0xC3, 0xF3, 0xFC, 0x0A } };

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

#undef INTERFACE
#define INTERFACE IDesktopAppXActivator
DECLARE_INTERFACE_IID(IDesktopAppXActivator, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IDesktopAppXActivator1

    STDMETHOD(Activate)(THIS,
        _In_ PWSTR ApplicationUserModelId,
        _In_ PWSTR PackageRelativeExecutable,
        _In_ PWSTR Arguments,
        _Out_ PHANDLE ProcessHandle
        ) PURE;

    STDMETHOD(ActivateWithOptions)(THIS,
        _In_ PWSTR ApplicationUserModelId,
        _In_ PWSTR Executable,
        _In_ PWSTR Arguments,
        _In_ ULONG ActivationOptions, // DESKTOP_APPX_ACTIVATE_OPTIONS
        _In_opt_ ULONG ParentProcessId,
        _Out_ PHANDLE ProcessHandle
        ) PURE;

    // IDesktopAppXActivator2

    STDMETHOD(ActivateWithOptionsAndArgs)(THIS,
        _In_ PWSTR ApplicationUserModelId,
        _In_ PWSTR Executable,
        _In_ PWSTR Arguments,
        _In_opt_ ULONG ParentProcessId,
        _In_opt_ PVOID ActivatedEventArgs,
        _Out_ PHANDLE ProcessHandle
        ) PURE;

    STDMETHOD(ActivateWithOptionsArgsWorkingDirectoryShowWindow)(THIS,
        _In_ PWSTR ApplicationUserModelId,
        _In_ PWSTR Executable,
        _In_ PWSTR Arguments,
        _In_ ULONG ActivationOptions, // DESKTOP_APPX_ACTIVATE_OPTIONS
        _In_opt_ ULONG ParentProcessId,
        _In_opt_ PVOID ActivatedEventArgs,
        _In_ PWSTR WorkingDirectory,
        _In_ ULONG ShowWindow,
        _Out_ PHANDLE ProcessHandle) PURE;

    END_INTERFACE
};

#define IDesktopAppXActivator_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IDesktopAppXActivator_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IDesktopAppXActivator_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IDesktopAppXActivator_ActivateWithOptions(This, ApplicationUserModelId, Executable, Arguments, ActivationOptions, ParentProcessId, ProcessHandle) \
    ((This)->lpVtbl->ActivateWithOptions(This, ApplicationUserModelId, Executable, Arguments, ActivationOptions, ParentProcessId, ProcessHandle))

typedef enum _START_MENU_APP_ITEMS_FLAGS
{
    SMAIF_DEFAULT = 0,
    SMAIF_EXTENDED = 1,
    SMAIF_USAGEINFO = 2
} START_MENU_APP_ITEMS_FLAGS;

#undef INTERFACE
#define INTERFACE IApplicationResolver61
DECLARE_INTERFACE_IID(IApplicationResolver61, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IApplicationResolver61
    STDMETHOD(GetAppIDForShortcut)(THIS,
        _In_ IShellItem *psi,
        _Outptr_ PWSTR *ppszAppID
        ) PURE;
    STDMETHOD(GetAppIDForWindow)(THIS,
        _In_ HWND hwnd,
        _Outptr_ PWSTR *ppszAppID,
        _Out_opt_ BOOL *pfPinningPrevented,
        _Out_opt_ BOOL *pfExplicitAppID,
        _Out_opt_ BOOL *pfEmbeddedShortcutValid
        ) PURE;
    STDMETHOD(GetAppIDForProcess)(THIS,
        _In_ ULONG dwProcessID,
        _Outptr_ PWSTR *ppszAppID,
        _Out_opt_ BOOL *pfPinningPrevented,
        _Out_opt_ BOOL *pfExplicitAppID,
        _Out_opt_ BOOL *pfEmbeddedShortcutValid
        ) PURE;
    STDMETHOD(GetShortcutForProcess)(THIS,
        _In_ ULONG dwProcessID,
        _Outptr_ IShellItem **ppsi
        ) PURE;
    STDMETHOD(GetBestShortcutForAppID)(THIS,
        _In_ PCWSTR pszAppID,
        _Outptr_ IShellItem **ppsi
        ) PURE;
    STDMETHOD(GetBestShortcutAndAppIDForAppPath)(THIS,
        _In_ PCWSTR pszAppPath,
        _Outptr_opt_ IShellItem **ppsi,
        _Outptr_opt_ PWSTR *ppszAppID
        ) PURE;
    STDMETHOD(CanPinApp)(THIS,
        _In_ IShellItem *psi
        ) PURE;
    STDMETHOD(GetRelaunchProperties)(THIS,
        _In_ HWND hwnd,
        _Outptr_opt_result_maybenull_ PWSTR *ppszAppID,
        _Outptr_opt_result_maybenull_ PWSTR *ppszCmdLine,
        _Outptr_opt_result_maybenull_ PWSTR *ppszIconResource,
        _Outptr_opt_result_maybenull_ PWSTR *ppszDisplayNameResource,
        _Out_opt_ BOOL *pfPinnable
        ) PURE;
    STDMETHOD(GenerateShortcutFromWindowProperties)(THIS,
        _In_ HWND hwnd,
        _Outptr_ IShellItem **ppsi
        ) PURE;
    STDMETHOD(GenerateShortcutFromItemProperties)(THIS,
        _In_ IShellItem2 *psi2,
        _Out_opt_ IShellItem **ppsi
        ) PURE;

    END_INTERFACE
};

#define IApplicationResolver_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IApplicationResolver_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IApplicationResolver_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IApplicationResolver_GetAppIDForShortcut(This, psi, ppszAppID) \
    ((This)->lpVtbl->GetAppIDForShortcut(This, psi, ppszAppID))
#define IApplicationResolver_GetAppIDForWindow(This, hwnd, ppszAppID, pfPinningPrevented, pfExplicitAppID, pfEmbeddedShortcutValid) \
    ((This)->lpVtbl->GetAppIDForWindow(This, hwnd, ppszAppID, pfPinningPrevented, pfExplicitAppID, pfEmbeddedShortcutValid))
#define IApplicationResolver_GetAppIDForProcess(This, dwProcessID, ppszAppID, pfPinningPrevented, pfExplicitAppID, pfEmbeddedShortcutValid) \
    ((This)->lpVtbl->GetAppIDForProcess(This, dwProcessID, ppszAppID, pfPinningPrevented, pfExplicitAppID, pfEmbeddedShortcutValid))
#define IApplicationResolver_GetShortcutForProcess(This, dwProcessID, ppsi) \
    ((This)->lpVtbl->GetShortcutForProcess(This, dwProcessID, ppsi))
#define IApplicationResolver_GetBestShortcutForAppID(This, pszAppID, ppsi) \
    ((This)->lpVtbl->GetBestShortcutForAppID(This, pszAppID, ppsi))
#define IApplicationResolver_GetBestShortcutAndAppIDForAppPath(This, pszAppPath, ppsi, ppszAppID) \
    ((This)->lpVtbl->GetBestShortcutAndAppIDForAppPath(This, pszAppPath, ppsi, ppszAppID))
#define IApplicationResolver_CanPinApp(This, psi) \
    ((This)->lpVtbl->CanPinApp(This, psi))
#define IApplicationResolver_GetRelaunchProperties(This, hwnd, ppszAppID, ppszCmdLine, ppszIconResource, ppszDisplayNameResource, pfPinnable) \
    ((This)->lpVtbl->GetRelaunchProperties(This, hwnd, ppszAppID, ppszCmdLine, ppszIconResource, ppszDisplayNameResource, pfPinnable))
#define IApplicationResolver_GenerateShortcutFromWindowProperties(This, ppsi) \
    ((This)->lpVtbl->GenerateShortcutFromWindowProperties(This, ppsi))
#define IApplicationResolver_GenerateShortcutFromItemProperties(This, psi2, ppsi) \
    ((This)->lpVtbl->GenerateShortcutFromItemProperties(This, psi2, ppsi))

#undef INTERFACE
#define INTERFACE IApplicationResolver62
DECLARE_INTERFACE_IID(IApplicationResolver62, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IApplicationResolver62
    STDMETHOD(GetAppIDForShortcut)(THIS,
        _In_ IShellItem *psi,
        _Outptr_ PWSTR *ppszAppID
        ) PURE;
    STDMETHOD(GetAppIDForShortcutObject)(THIS,
        _In_ IShellLinkW *psl,
        _In_ IShellItem *psi,
        _Outptr_ PWSTR *ppszAppID
        ) PURE;
    STDMETHOD(GetAppIDForWindow)(THIS,
        _In_ HWND hwnd,
        _Outptr_ PWSTR *ppszAppID,
        _Out_opt_ BOOL *pfPinningPrevented,
        _Out_opt_ BOOL *pfExplicitAppID,
        _Out_opt_ BOOL *pfEmbeddedShortcutValid
        ) PURE;
    STDMETHOD(GetAppIDForProcess)(THIS,
        _In_ ULONG dwProcessID,
        _Outptr_ PWSTR *ppszAppID,
        _Out_opt_ BOOL *pfPinningPrevented,
        _Out_opt_ BOOL *pfExplicitAppID,
        _Out_opt_ BOOL *pfEmbeddedShortcutValid
        ) PURE;
    STDMETHOD(GetShortcutForProcess)(THIS,
        _In_ ULONG dwProcessID,
        _Outptr_ IShellItem **ppsi
        ) PURE;
    STDMETHOD(GetBestShortcutForAppID)(THIS,
        _In_ PCWSTR pszAppID,
        _Outptr_ IShellItem **ppsi
        ) PURE;
    STDMETHOD(GetBestShortcutAndAppIDForAppPath)(THIS,
        _In_ PCWSTR pszAppPath,
        _Outptr_opt_ IShellItem **ppsi,
        _Outptr_opt_ PWSTR *ppszAppID
        ) PURE;
    STDMETHOD(CanPinApp)(THIS,
        _In_ IShellItem *psi
        ) PURE;
    STDMETHOD(CanPinAppShortcut)(THIS,
        _In_ IShellLinkW *psl,
        _In_ IShellItem *psi
        ) PURE;
    STDMETHOD(GetRelaunchProperties)(THIS,
        _In_ HWND hwnd,
        _Outptr_opt_result_maybenull_ PWSTR *ppszAppID,
        _Outptr_opt_result_maybenull_ PWSTR *ppszCmdLine,
        _Outptr_opt_result_maybenull_ PWSTR *ppszIconResource,
        _Outptr_opt_result_maybenull_ PWSTR *ppszDisplayNameResource,
        _Out_opt_ BOOL *pfPinnable
        ) PURE;
    STDMETHOD(GenerateShortcutFromWindowProperties)(THIS,
        _In_ HWND hwnd,
        _Outptr_ IShellItem **ppsi
        ) PURE;
    STDMETHOD(GenerateShortcutFromItemProperties)(THIS,
        _In_ IShellItem2 *psi2,
        _Out_opt_ IShellItem **ppsi
        ) PURE;

    // STDMETHOD(GetLauncherAppIDForItem)(THIS)
    // STDMETHOD(GetShortcutForAppID)(THIS)
    // STDMETHOD(GetLauncherAppIDForItemEx)(THIS)

    END_INTERFACE
};

#define IApplicationResolver2_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IApplicationResolver2_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IApplicationResolver2_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IApplicationResolver2_GetAppIDForShortcut(This, psl, psi, ppszAppID) \
    ((This)->lpVtbl->GetAppIDForShortcut(This, psl, psi, ppszAppID))
#define IApplicationResolver2_GetAppIDForShortcutObject(This, psl, ppszAppID) \
    ((This)->lpVtbl->GetAppIDForShortcutObject(This, psl, ppszAppID))
#define IApplicationResolver2_GetAppIDForWindow(This, hwnd, ppszAppID, pfPinningPrevented, pfExplicitAppID, pfEmbeddedShortcutValid) \
    ((This)->lpVtbl->GetAppIDForWindow(This, hwnd, ppszAppID, pfPinningPrevented, pfExplicitAppID, pfEmbeddedShortcutValid))
#define IApplicationResolver2_GetAppIDForProcess(This, dwProcessID, ppszAppID, pfPinningPrevented, pfExplicitAppID, pfEmbeddedShortcutValid) \
    ((This)->lpVtbl->GetAppIDForProcess(This, dwProcessID, ppszAppID, pfPinningPrevented, pfExplicitAppID, pfEmbeddedShortcutValid))
#define IApplicationResolver2_GetShortcutForProcess(This, dwProcessID, ppsi) \
    ((This)->lpVtbl->GetShortcutForProcess(This, dwProcessID, ppsi))
#define IApplicationResolver2_GetBestShortcutForAppID(This, pszAppID, ppsi) \
    ((This)->lpVtbl->GetBestShortcutForAppID(This, pszAppID, ppsi))
#define IApplicationResolver2_GetBestShortcutAndAppIDForAppPath(This, pszAppPath, ppsi, ppszAppID) \
    ((This)->lpVtbl->GetBestShortcutAndAppIDForAppPath(This, pszAppPath, ppsi, ppszAppID))
#define IApplicationResolver2_CanPinApp(This, psi) \
    ((This)->lpVtbl->CanPinApp(This, psi))
#define IApplicationResolver2_CanPinAppShortcut(This, psl, psi) \
    ((This)->lpVtbl->CanPinAppShortcut(This, psl, psi))
#define IApplicationResolver2_GetRelaunchProperties(This, hwnd, ppszAppID, ppszCmdLine, ppszIconResource, ppszDisplayNameResource, pfPinnable) \
    ((This)->lpVtbl->GetRelaunchProperties(This, hwnd, ppszAppID, ppszCmdLine, ppszIconResource, ppszDisplayNameResource, pfPinnable))
#define IApplicationResolver2_GenerateShortcutFromWindowProperties(This, ppsi) \
    ((This)->lpVtbl->GenerateShortcutFromWindowProperties(This, ppsi))
#define IApplicationResolver2_GenerateShortcutFromItemProperties(This, psi2, ppsi) \
    ((This)->lpVtbl->GenerateShortcutFromItemProperties(This, psi2, ppsi))

#undef INTERFACE
#define INTERFACE IStartMenuAppItems61
DECLARE_INTERFACE_IID(IStartMenuAppItems61, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IStartMenuAppItems61
    STDMETHOD(EnumItems)(THIS,
        _In_ START_MENU_APP_ITEMS_FLAGS Flags,
        _In_ REFIID riid, // IID_IEnumObjects, IID_IObjectCollection
        _Outptr_ PVOID *ppvObject
        ) PURE;
    STDMETHOD(GetItem)(THIS,
        _In_ START_MENU_APP_ITEMS_FLAGS Flags,
        _In_ PWSTR AppUserModelId,
        _In_ REFIID riid,
        _Outptr_ PVOID *ppvObject // ppvObject == IPropertyStore, IStartMenuAppItems61
        ) PURE;

    END_INTERFACE
};

#define IStartMenuAppItems_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IStartMenuAppItems_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IStartMenuAppItems_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IStartMenuAppItems_EnumItems(This, Flags, riid, ppvObject) \
    ((This)->lpVtbl->EnumItems(This, Flags, riid, ppvObject))
#define IStartMenuAppItems_GetItem(This, Flags, AppUserModelId, riid, ppvObject) \
    ((This)->lpVtbl->GetItem(This, Flags, AppUserModelId, riid, ppvObject))

#undef INTERFACE
#define INTERFACE IStartMenuAppItems62
DECLARE_INTERFACE_IID(IStartMenuAppItems62, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IStartMenuAppItems62
    STDMETHOD(EnumItems)(THIS,
        _In_ START_MENU_APP_ITEMS_FLAGS Flags,
        _In_ REFIID riid, // IID_IEnumObjects, IID_IObjectCollection
        _Outptr_ PVOID *ppvObject
        ) PURE;
    STDMETHOD(GetItem)(THIS,
        _In_ START_MENU_APP_ITEMS_FLAGS Flags,
        _In_ PWSTR AppUserModelId,
        _In_ REFIID riid,
        _Outptr_ PVOID *ppvObject // ppvObject == IPropertyStore, IStartMenuAppItems61
        ) PURE;
    // STDMETHOD(GetItemByAppPath)(THIS)
    // STDMETHOD(EnumCachedItems)(THIS)

    END_INTERFACE
};

#define IStartMenuAppItems2_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IStartMenuAppItems2_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IStartMenuAppItems2_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IStartMenuAppItems2_EnumItems(This, Flags, riid, ppvObject) \
    ((This)->lpVtbl->EnumItems(This, Flags, riid, ppvObject))
#define IStartMenuAppItems2_GetItem(This, Flags, AppUserModelId, riid, ppvObject) \
    ((This)->lpVtbl->GetItem(This, Flags, AppUserModelId, riid, ppvObject))

#undef INTERFACE
#define INTERFACE IMrtResourceManager
DECLARE_INTERFACE_IID(IMrtResourceManager, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IMrtResourceManager
    STDMETHOD(Initialize)(THIS) PURE;
    STDMETHOD(InitializeForCurrentApplication)(THIS) PURE;
    STDMETHOD(InitializeForPackage)(THIS, PCWSTR PackageName) PURE;
    STDMETHOD(InitializeForFile)(THIS, PCWSTR) PURE;
    STDMETHOD(GetMainResourceMap)(THIS, REFIID riid, PVOID *ppvObject) PURE; // IResourceMap
    STDMETHOD(GetResourceMap)(THIS, PCWSTR, REFIID riid, PVOID *ppvObject) PURE; // IResourceMap
    STDMETHOD(GetDefaultContext)(THIS, REFIID riid, PVOID *ppvObject) PURE; // IResourceContext
    STDMETHOD(GetReference)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD(IsResourceReference)(THIS, PCWSTR, BOOL *IsReference) PURE;

    END_INTERFACE
};

#define IMrtResourceManager_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IMrtResourceManager_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IMrtResourceManager_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IMrtResourceManager_Initialize(This) \
    ((This)->lpVtbl->Initialize(This))
#define IMrtResourceManager_InitializeForCurrentApplication(This) \
    ((This)->lpVtbl->InitializeForCurrentApplication(This))
#define IMrtResourceManager_InitializeForPackage(This, PackageName) \
    ((This)->lpVtbl->InitializeForPackage(This, PackageName))
#define IMrtResourceManager_InitializeForFile(This, FilePath) \
    ((This)->lpVtbl->InitializeForFile(This, FilePath))
#define IMrtResourceManager_GetMainResourceMap(This, riid, ppvObject) \
    ((This)->lpVtbl->GetMainResourceMap(This, riid, ppvObject))
#define IMrtResourceManager_GetResourceMap(This, Name, riid, ppvObject) \
    ((This)->lpVtbl->GetResourceMap(This, Name, riid, ppvObject))
#define IMrtResourceManager_GetDefaultContext(This, riid, ppvObject) \
    ((This)->lpVtbl->GetDefaultContext(This, riid, ppvObject))
#define IMrtResourceManager_GetReference(This, riid, ppvObject) \
    ((This)->lpVtbl->GetReference(This, riid, ppvObject))
#define IMrtResourceManager_IsResourceReference(This, Name, IsReference) \
    ((This)->lpVtbl->IsResourceReference(This, Name, IsReference))

#undef INTERFACE
#define INTERFACE IResourceContext
DECLARE_INTERFACE_IID(IResourceContext, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IResourceContext
    STDMETHOD(GetLanguage)(THIS, PWSTR*) PURE;
    STDMETHOD(GetHomeRegion)(THIS, PCWSTR*) PURE;
    STDMETHOD(GetLayoutDirection)(THIS, ULONG*) PURE; // RESOURCE_LAYOUT_DIRECTION
    STDMETHOD(GetTargetSize)(THIS, USHORT*) PURE;
    STDMETHOD(GetScale)(THIS, ULONG*) PURE; // RESOURCE_SCALE
    STDMETHOD(GetContrast)(THIS, ULONG*) PURE; // RESOURCE_CONTRAST
    STDMETHOD(GetAlternateForm)(THIS, PCWSTR*) PURE;
    STDMETHOD(GetQualifierValue)(THIS, LPCWSTR, LPWSTR*) PURE;
    STDMETHOD(SetLanguage)(THIS, LPCWSTR) PURE;
    STDMETHOD(SetHomeRegion)(THIS, LPCWSTR) PURE;
    STDMETHOD(SetLayoutDirection)(THIS, ULONG) PURE; // RESOURCE_LAYOUT_DIRECTION
    STDMETHOD(SetTargetSize)(THIS, USHORT) PURE;
    STDMETHOD(SetScale)(THIS, ULONG) PURE; // RESOURCE_SCALE
    STDMETHOD(SetContrast)(THIS, ULONG) PURE; // RESOURCE_CONTRAST
    STDMETHOD(SetAlternateForm)(THIS, LPCWSTR) PURE;
    STDMETHOD(SetQualifierValue)(THIS, LPCWSTR, LPCWSTR) PURE;
    STDMETHOD(TrySetQualifierValue)(THIS, LPCWSTR, LPCWSTR, HRESULT*) PURE;
    STDMETHOD(Reset)(THIS) PURE;
    STDMETHOD(ResetQualifierValue)(THIS, LPCWSTR) PURE;
    STDMETHOD(Clone)(THIS, IResourceContext**) PURE;
    STDMETHOD(OverrideToMatch)(THIS, struct IResourceCandidate*) PURE;

    END_INTERFACE
};

#undef INTERFACE
#define INTERFACE IResourceMap
DECLARE_INTERFACE_IID(IResourceMap, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IResourceMap
    STDMETHOD(GetUri)(THIS, PWSTR *UriString) PURE;
    STDMETHOD(GetSubtree)(THIS, PCWSTR *Name, IResourceMap** ResourceMap) PURE;
    STDMETHOD(GetString)(THIS, PCWSTR Key, PWSTR *Value) PURE;
    STDMETHOD(GetStringForContext)(THIS, IResourceContext* Context, PCWSTR Key, PWSTR *Value) PURE;
    STDMETHOD(GetFilePath)(THIS, PCWSTR Key, PWSTR *Value) PURE;
    STDMETHOD(GetFilePathForContext)(THIS, IResourceContext*, PCWSTR, PWSTR*) PURE;
    STDMETHOD(GetNamedResourceCount)(THIS, PULONG) PURE;
    STDMETHOD(GetNamedResourceUri)(THIS, ULONG, PWSTR*) PURE;
    STDMETHOD(GetNamedResource)(THIS, PCWSTR, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD(GetFullyQualifiedReference)(THIS, LPCWSTR, LPCWSTR, LPWSTR*) PURE;
    STDMETHOD(GetFilePathByUri)(THIS, IUri*, LPWSTR*) PURE;
    STDMETHOD(GetFilePathForContextByUri)(THIS, IResourceContext*, IUri*, LPWSTR*) PURE;

    END_INTERFACE
};

#define IResourceMap_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IResourceMap_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IResourceMap_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IResourceMap_GetUri(This, UriString) \
    ((This)->lpVtbl->GetUri(This, UriString))
#define IResourceMap_GetSubtree(This, Name, ResourceMap) \
    ((This)->lpVtbl->GetSubtree(This, Name, ResourceMap))
#define IResourceMap_GetString(This, Key, Value) \
    ((This)->lpVtbl->GetString(This, Key, Value))
#define IResourceMap_GetStringForContext(This, Context, Key, Value) \
    ((This)->lpVtbl->GetStringForContext(This, Context, Key, Value))
#define IResourceMap_GetFilePath(This, Key, Value) \
    ((This)->lpVtbl->GetFilePath(This, Key, Value))
#define IResourceMap_GetFilePathForContext(This) \
    ((This)->lpVtbl->GetFilePathForContext(This))
#define IResourceMap_GetNamedResourceCount(This, Count) \
    ((This)->lpVtbl->GetNamedResourceCount(This, Count))
#define IResourceMap_GetNamedResourceUri(This, Index, Name) \
    ((This)->lpVtbl->GetNamedResourceUri(This, Index, Name))
#define IResourceMap_GetNamedResource(This, Name, rrid, ppvObject) \
    ((This)->lpVtbl->GetNamedResource(This, Name, rrid, ppvObject))
#define IResourceMap_GetFullyQualifiedReference(This) \
    ((This)->lpVtbl->GetFullyQualifiedReference(This))
#define IResourceMap_GetFilePathByUri(This) \
    ((This)->lpVtbl->GetFilePathByUri(This))
#define IResourceMap_GetFilePathForContextByUri(This) \
    ((This)->lpVtbl->GetFilePathForContextByUri(This))

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
static CLSID CLSID_OSTaskCompletion_I = { 0x07fc2b94, 0x5285, 0x417e, { 0x8a, 0xc3, 0xc2, 0xce, 0x52, 0x40, 0xb0, 0xfa } };
// "c7e40572-c36a-43ea-9a40-f3b168da5558"
static IID IID_IOSTaskCompletion_I = { 0xc7e40572, 0xc36a, 0x43ea, { 0x9a, 0x40, 0xf3, 0xb1, 0x68, 0xda, 0x55, 0x58 } };

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

#undef INTERFACE
#define INTERFACE IOSTaskCompletion
DECLARE_INTERFACE_IID(IOSTaskCompletion, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IOSTaskCompletion
    STDMETHOD(BeginTask)(THIS, ULONG ProcessId, PLM_TASKCOMPLETION_CATEGORY_FLAGS Flags) PURE;
    STDMETHOD(BeginTaskByHandle)(THIS, HANDLE ProcessHandle, PLM_TASKCOMPLETION_CATEGORY_FLAGS Flags) PURE;
    STDMETHOD(EndTask)(THIS) PURE;
    STDMETHOD_(ULONG, ValidateStreamedFileTaskCompletionUsage)(THIS, HANDLE ProcessHandle, ULONG ProcessId) PURE; // returns PsmKey

    END_INTERFACE
};

#define IOSTaskCompletion_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IOSTaskCompletion_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IOSTaskCompletion_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IOSTaskCompletion_BeginTask(This, ProcessId, Flags) \
    ((This)->lpVtbl->BeginTask(This, ProcessId, Flags))
#define IOSTaskCompletion_BeginTaskByHandle(This, ProcessHandle, Flags) \
    ((This)->lpVtbl->BeginTaskByHandle(This, ProcessHandle, Flags))
#define IOSTaskCompletion_EndTask(This) \
    ((This)->lpVtbl->EndTask(This))
#define IOSTaskCompletion_ValidateStreamedFileTaskCompletionUsage(This, ProcessHandle, ProcessId) \
    ((This)->lpVtbl->ValidateStreamedFileTaskCompletionUsage(This, ProcessHandle, ProcessId))

#undef INTERFACE
#define INTERFACE IOSTaskCompletion2
DECLARE_INTERFACE_IID(IOSTaskCompletion2, IOSTaskCompletion)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IOSTaskCompletion2
    STDMETHOD(InternalGetTrustLevelStatic)(THIS) PURE;
    STDMETHOD(InternalGetTrustLevelStatic2)(THIS) PURE;
    STDMETHOD(EndAllTasksAndWait)(THIS) PURE;
    STDMETHOD(BeginTaskByHandleEx)(THIS, HANDLE ProcessHandle, PLM_TASKCOMPLETION_CATEGORY_FLAGS Flags, enum PLM_TASKCOMPLETION_BEGIN_TASK_FLAGS TaskFlags) PURE;

    END_INTERFACE
};

#define IOSTaskCompletion_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IOSTaskCompletion_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IOSTaskCompletion_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IOSTaskCompletion_InternalGetTrustLevelStatic(This) \
    ((This)->lpVtbl->InternalGetTrustLevelStatic(This))
#define IOSTaskCompletion_InternalGetTrustLevelStatic2(This) \
    ((This)->lpVtbl->InternalGetTrustLevelStatic2(This))
#define IOSTaskCompletion_EndAllTasksAndWait(This) \
    ((This)->lpVtbl->EndAllTasksAndWait(This))
#define IOSTaskCompletion_BeginTaskByHandleEx(This, ProcessHandle, Flags, TaskFlags) \
    ((This)->lpVtbl->BeginTaskByHandleEx(This, ProcessHandle, Flags, TaskFlags))

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
