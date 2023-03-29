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
    PPH_STRING PackageDisplayName;
    PPH_STRING PackageName;
    PPH_STRING PackageInstallPath;
    PPH_STRING PackageFullName;
    PPH_STRING SmallLogoPath;
} PH_APPUSERMODELID_ENUM_ENTRY, *PPH_APPUSERMODELID_ENUM_ENTRY;

PPH_LIST PhEnumApplicationUserModelIds(
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

//
// Windows Runtime
//

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

#pragma region Activation Factory

// 00000035-0000-0000-C000-000000000046
DEFINE_GUID(IID_IActivationFactory, 0x00000035, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
// AF86E2E0-B12D-4c6a-9C5A-D7AA65101E90
DEFINE_GUID(IID_IInspectable, 0xAF86E2E0, 0xB12D, 0x4c6a, 0x9C, 0x5A, 0xD7, 0xAA, 0x65, 0x10, 0x1E, 0x90);

#ifdef interface
#ifndef __IInspectable_FWD_DEFINED__
#define __IInspectable_FWD_DEFINED__
typedef interface IInspectable IInspectable;
#endif
#ifndef __IInspectable_INTERFACE_DEFINED__
#define __IInspectable_INTERFACE_DEFINED__
#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("AF86E2E0-B12D-4c6a-9C5A-D7AA65101E90")
IInspectable : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetIids(
        __RPC__out ULONG * iidCount,
        __RPC__deref_out_ecount_full_opt(*iidCount) IID * *iids) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRuntimeClassName(
        __RPC__deref_out_opt HSTRING* className) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTrustLevel(
        __RPC__out TrustLevel* trustLevel) = 0;
};
#else
typedef struct IInspectableVtbl
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(
        __RPC__in IInspectable* This,
        __RPC__in REFIID riid,
        _COM_Outptr_ void** ppvObject);

    DECLSPEC_XFGVIRT(IUnknown, AddRef)
    ULONG(STDMETHODCALLTYPE* AddRef)(
        __RPC__in IInspectable* This);

    DECLSPEC_XFGVIRT(IUnknown, Release)
    ULONG(STDMETHODCALLTYPE* Release)(
        __RPC__in IInspectable* This);

    DECLSPEC_XFGVIRT(IInspectable, GetIids)
    HRESULT(STDMETHODCALLTYPE* GetIids)(
        __RPC__in IInspectable* This,
        __RPC__out ULONG* iidCount,
        __RPC__deref_out_ecount_full_opt(*iidCount) IID** iids);

    DECLSPEC_XFGVIRT(IInspectable, GetRuntimeClassName)
    HRESULT(STDMETHODCALLTYPE* GetRuntimeClassName)(
        __RPC__in IInspectable* This,
        __RPC__deref_out_opt HSTRING* className);

    DECLSPEC_XFGVIRT(IInspectable, GetTrustLevel)
    HRESULT(STDMETHODCALLTYPE* GetTrustLevel)(
        __RPC__in IInspectable* This,
        __RPC__out ULONG* trustLevel); // TrustLevel

    END_INTERFACE
} IInspectableVtbl;

interface IInspectable
{
    CONST_VTBL struct IInspectableVtbl* lpVtbl;
};

#ifdef COBJMACROS
#define IInspectable_QueryInterface(This,riid,ppvObject) \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject)) 
#define IInspectable_AddRef(This) \
    ((This)->lpVtbl->AddRef(This)) 
#define IInspectable_Release(This) \
    ((This)->lpVtbl->Release(This)) 
#define IInspectable_GetIids(This,iidCount,iids) \
    ((This)->lpVtbl->GetIids(This,iidCount,iids)) 
#define IInspectable_GetRuntimeClassName(This,className) \
    ((This)->lpVtbl->GetRuntimeClassName(This,className)) 
#define IInspectable_GetTrustLevel(This,trustLevel)	\
    ((This)->lpVtbl->GetTrustLevel(This,trustLevel)) 
#endif
#endif
#endif

#ifndef __IActivationFactory_FWD_DEFINED__
#define __IActivationFactory_FWD_DEFINED__
typedef interface IActivationFactory IActivationFactory;
#endif
#ifndef __IActivationFactory_INTERFACE_DEFINED__
#define __IActivationFactory_INTERFACE_DEFINED__
#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("00000035-0000-0000-C000-000000000046")
IActivationFactory : public IInspectable
{
public:
    virtual HRESULT STDMETHODCALLTYPE ActivateInstance(
        __RPC__deref_out_opt IInspectable * *instance) = 0;

};
#else
typedef struct IActivationFactoryVtbl
{
    BEGIN_INTERFACE

    DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
    HRESULT (STDMETHODCALLTYPE* QueryInterface)(
        __RPC__in IActivationFactory* This,
        __RPC__in REFIID riid,
        _COM_Outptr_ void** ppvObject);

    DECLSPEC_XFGVIRT(IUnknown, AddRef)
    ULONG (STDMETHODCALLTYPE* AddRef)(
        __RPC__in IActivationFactory* This);

    DECLSPEC_XFGVIRT(IUnknown, Release)
    ULONG (STDMETHODCALLTYPE* Release)(
        __RPC__in IActivationFactory* This);

    DECLSPEC_XFGVIRT(IInspectable, GetIids)
    HRESULT (STDMETHODCALLTYPE* GetIids)(
        __RPC__in IActivationFactory* This,
        __RPC__out ULONG* iidCount,
        __RPC__deref_out_ecount_full_opt(*iidCount) IID** iids);

    DECLSPEC_XFGVIRT(IInspectable, GetRuntimeClassName)
    HRESULT (STDMETHODCALLTYPE* GetRuntimeClassName)(
        __RPC__in IActivationFactory* This,
        __RPC__deref_out_opt HSTRING* className);

    DECLSPEC_XFGVIRT(IInspectable, GetTrustLevel)
    HRESULT (STDMETHODCALLTYPE* GetTrustLevel)(
        __RPC__in IActivationFactory* This,
        __RPC__out ULONG* trustLevel); // TrustLevel enum

    DECLSPEC_XFGVIRT(IActivationFactory, ActivateInstance)
    HRESULT (STDMETHODCALLTYPE* ActivateInstance)(
        __RPC__in IActivationFactory* This,
        __RPC__deref_out_opt IInspectable** instance);

    END_INTERFACE
} IActivationFactoryVtbl;

interface IActivationFactory
{
    CONST_VTBL struct IActivationFactoryVtbl* lpVtbl;
};

#ifdef COBJMACROS
#define IActivationFactory_QueryInterface(This,riid,ppvObject) \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject)) 
#define IActivationFactory_AddRef(This) \
    ((This)->lpVtbl->AddRef(This)) 
#define IActivationFactory_Release(This) \
    ((This)->lpVtbl->Release(This)) 
#define IActivationFactory_GetIids(This,iidCount,iids) \
    ((This)->lpVtbl->GetIids(This,iidCount,iids)) 
#define IActivationFactory_GetRuntimeClassName(This,className) \
    ((This)->lpVtbl->GetRuntimeClassName(This,className)) 
#define IActivationFactory_GetTrustLevel(This,trustLevel) \
    ((This)->lpVtbl->GetTrustLevel(This,trustLevel)) 
#define IActivationFactory_ActivateInstance(This,instance) \
    ((This)->lpVtbl->ActivateInstance(This,instance)) 
#endif
#endif
#endif

#endif

#pragma endregion

EXTERN_C_END

#endif
