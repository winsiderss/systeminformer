/*
 * Process Hacker -
 *   Appmodel support functions
 *
 * Copyright (C) 2017-2018 dmex
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
#define IApplicationResolver_AddRef(This)	\
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

    END_INTERFACE
};

#define IApplicationResolver2_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IApplicationResolver2_AddRef(This)	\
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
        _In_ ULONG Flags,
        _In_ REFIID riid,
        _Outptr_ IEnumObjects *ppvObject
        ) PURE;
    STDMETHOD(GetItem)(THIS,
        _In_ ULONG Flags,
        _In_ PWSTR AppUserModelId,
        _In_ REFIID riid,
        _Outptr_ PVOID *ppvObject // ppvObject == IPropertyStore, IStartMenuAppItems61
        ) PURE;

    END_INTERFACE
};

#define IStartMenuAppItems_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IStartMenuAppItems_AddRef(This)	\
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
        _In_ ULONG Flags,
        _In_ REFIID riid,
        _Outptr_ IEnumObjects *ppvObject
        ) PURE;
    STDMETHOD(GetItem)(THIS,
        _In_ ULONG Flags,
        _In_ PWSTR AppUserModelId,
        _In_ REFIID riid,
        _Outptr_ PVOID *ppvObject // ppvObject == IPropertyStore, IStartMenuAppItems61
        ) PURE;
    // STDMETHOD(Unknown)(THIS)
    // STDMETHOD(Unknown)(THIS)

    END_INTERFACE
};

#define IStartMenuAppItems2_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define IStartMenuAppItems2_AddRef(This)	\
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
#define IMrtResourceManager_AddRef(This)	\
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
#define IResourceMap_AddRef(This)	\
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
#define IResourceMap_GetNamedResource(This) \
    ((This)->lpVtbl->GetNamedResource(This)) 
#define IResourceMap_GetFullyQualifiedReference(This) \
    ((This)->lpVtbl->GetFullyQualifiedReference(This)) 
#define IResourceMap_GetFilePathByUri(This) \
    ((This)->lpVtbl->GetFilePathByUri(This)) 
#define IResourceMap_GetFilePathForContextByUri(This) \
    ((This)->lpVtbl->GetFilePathForContextByUri(This)) 

// Note: Documented PKEY_AppUserModel_XYZ keys can be found in propkey.h
DEFINE_PROPERTYKEY(PKEY_AppUserModel_HostEnvironment, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 14);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_PackageInstallPath, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 15);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_PackageFamilyName, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 17);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_ParentID, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 19);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_PackageFullName, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 21);
// PKEY_AppUserModel_ExcludeFromShowInNewInstall {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 8
//2	PKEY_AppUserModel_ID {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 5
//3	PKEY_AppUserModel_IsDestListSeparator {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 6
//4	PKEY_AppUserModel_IsDualMode {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 11
//5	PKEY_AppUserModel_PreventPinning {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 9
//6	PKEY_AppUserModel_RelaunchCommand {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 2
//7	PKEY_AppUserModel_RelaunchDisplayNameResource {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 4
//8	PKEY_AppUserModel_RelaunchIconResource {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 3
//9	PKEY_AppUserModel_StartPinOption {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 12
//10 PKEY_AppUserModel_ToastActivatorCLSID {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 26
//11 PKEY_AppUserModel_PackageInstallPath {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 15
//12 PKEY_AppUserModel_RecordState {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 16
//13 PKEY_AppUserModel_PackageFullName {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 21
//14 PKEY_AppUserModel_DestListProvidedTitle {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 27
//15 PKEY_AppUserModel_DestListProvidedDescription {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 28
//16 PKEY_AppUserModel_ParentID	{9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 19
//17 PKEY_AppUserModel_HostEnvironment {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 14
//18 PKEY_AppUserModel_Relevance {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 13
//19 PKEY_AppUserModel_PackageRelativeApplicationID	{9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 22
//20 PKEY_AppUserModel_ExcludedFromLauncher {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 23
//21 PKEY_AppUserModel_DestListLogoUri {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 29
//22 PKEY_AppUserModel_ActivationContext {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 20
//23 PKEY_AppUserModel_PackageFamilyName {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 17
//24 PKEY_AppUserModel_BestShortcut	{9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 10
//25 PKEY_AppUserModel_IsDestListLink {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 7
//26 PKEY_AppUserModel_InstalledBy {9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 18
//27 PKEY_AppUserModel_RunFlags	{9f4c2855-9f79-4b39-a8d0-e1d42de1d5f3} 25
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

#endif
