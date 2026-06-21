// Licensed to the .NET Foundation under one or more agreements.
// Minimal ICorDebug interface subset for non-invasive GC heap enumeration.
// Vtable layouts and GUIDs match the .NET runtime (MIT license).

#ifndef HAVE_COREDBG_DELEGATES_H
#define HAVE_COREDBG_DELEGATES_H

#ifndef CORDB_ADDRESS_DEFINED
#define CORDB_ADDRESS_DEFINED
typedef ULONG64 CORDB_ADDRESS;
#endif

// COR_TYPEID: token1 == MethodTable address in .NET Core
typedef struct _COR_TYPEID
{
    UINT64 token1;
    UINT64 token2;
} COR_TYPEID;

typedef struct _COR_HEAPOBJECT
{
    CORDB_ADDRESS address;
    ULONG64       size;
    COR_TYPEID    type;
} COR_HEAPOBJECT;

typedef enum _CorDebugPlatform
{
    CORDB_PLATFORM_WINDOWS_X86   = 0,
    CORDB_PLATFORM_WINDOWS_AMD64 = 1,
    CORDB_PLATFORM_WINDOWS_IA64  = 2,
    CORDB_PLATFORM_WINDOWS_ARM   = 5,
    CORDB_PLATFORM_WINDOWS_ARM64 = 7,
} CorDebugPlatform;

typedef struct _CLR_DEBUGGING_VERSION
{
    WORD wStructVersion;
    WORD wMajor;
    WORD wMinor;
    WORD wBuild;
    WORD wRevision;
} CLR_DEBUGGING_VERSION;

typedef ULONG CLR_DEBUGGING_PROCESS_FLAGS;
#define CLR_DEBUGGING_MANAGED_EVENT_PENDING 0x1

// --- ICorDebugDataTarget ---

typedef struct ICorDebugDataTarget ICorDebugDataTarget;

typedef struct ICorDebugDataTargetVtbl
{
    BEGIN_INTERFACE
    DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
    HRESULT (STDMETHODCALLTYPE *QueryInterface)     (ICorDebugDataTarget*, REFIID, void**);
    DECLSPEC_XFGVIRT(IUnknown, AddRef)
    ULONG   (STDMETHODCALLTYPE *AddRef)             (ICorDebugDataTarget*);
    DECLSPEC_XFGVIRT(IUnknown, Release)
    ULONG   (STDMETHODCALLTYPE *Release)            (ICorDebugDataTarget*);
    DECLSPEC_XFGVIRT(ICorDebugDataTarget, GetPlatform)
    HRESULT (STDMETHODCALLTYPE *GetPlatform)        (ICorDebugDataTarget*, CorDebugPlatform*);
    DECLSPEC_XFGVIRT(ICorDebugDataTarget, ReadVirtualMemory)
    HRESULT (STDMETHODCALLTYPE *ReadVirtualMemory)  (ICorDebugDataTarget*, CORDB_ADDRESS, BYTE*, ULONG32, ULONG32*);
    DECLSPEC_XFGVIRT(ICorDebugDataTarget, GetThreadContext)
    HRESULT (STDMETHODCALLTYPE *GetThreadContext)   (ICorDebugDataTarget*, ULONG32, ULONG32, ULONG32, BYTE*);
    END_INTERFACE
} ICorDebugDataTargetVtbl;

struct ICorDebugDataTarget { const ICorDebugDataTargetVtbl* lpVtbl; };

// --- ICLRDebuggingLibraryProvider ---

typedef struct ICLRDebuggingLibraryProvider ICLRDebuggingLibraryProvider;

typedef struct ICLRDebuggingLibraryProviderVtbl
{
    BEGIN_INTERFACE
    DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
    HRESULT (STDMETHODCALLTYPE *QueryInterface)  (ICLRDebuggingLibraryProvider*, REFIID, void**);
    DECLSPEC_XFGVIRT(IUnknown, AddRef)
    ULONG   (STDMETHODCALLTYPE *AddRef)          (ICLRDebuggingLibraryProvider*);
    DECLSPEC_XFGVIRT(IUnknown, Release)
    ULONG   (STDMETHODCALLTYPE *Release)         (ICLRDebuggingLibraryProvider*);
    DECLSPEC_XFGVIRT(ICLRDebuggingLibraryProvider, ProvideLibrary)
    HRESULT (STDMETHODCALLTYPE *ProvideLibrary)  (ICLRDebuggingLibraryProvider*, PCWSTR, DWORD, DWORD, HMODULE*);
    END_INTERFACE
} ICLRDebuggingLibraryProviderVtbl;

struct ICLRDebuggingLibraryProvider { const ICLRDebuggingLibraryProviderVtbl* lpVtbl; };

// --- ICLRDebugging ---

typedef struct ICLRDebugging ICLRDebugging;

typedef struct ICLRDebuggingVtbl
{
    BEGIN_INTERFACE
    DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
    HRESULT (STDMETHODCALLTYPE *QueryInterface)     (ICLRDebugging*, REFIID, void**);
    DECLSPEC_XFGVIRT(IUnknown, AddRef)
    ULONG   (STDMETHODCALLTYPE *AddRef)             (ICLRDebugging*);
    DECLSPEC_XFGVIRT(IUnknown, Release)
    ULONG   (STDMETHODCALLTYPE *Release)            (ICLRDebugging*);
    DECLSPEC_XFGVIRT(ICLRDebugging, OpenVirtualProcess)
    HRESULT (STDMETHODCALLTYPE *OpenVirtualProcess) (ICLRDebugging*, ULONG64, IUnknown*,
                                                     ICLRDebuggingLibraryProvider*, CLR_DEBUGGING_VERSION*,
                                                     REFIID, IUnknown**, CLR_DEBUGGING_VERSION*,
                                                     CLR_DEBUGGING_PROCESS_FLAGS*);
    DECLSPEC_XFGVIRT(ICLRDebugging, CanUnloadNow)
    HRESULT (STDMETHODCALLTYPE *CanUnloadNow)       (ICLRDebugging*, HMODULE);
    END_INTERFACE
} ICLRDebuggingVtbl;

struct ICLRDebugging { const ICLRDebuggingVtbl* lpVtbl; };

#define ICLRDebugging_OpenVirtualProcess(T,a,b,c,d,e,f,g,h) \
    ((T)->lpVtbl->OpenVirtualProcess((T),(a),(b),(c),(d),(e),(f),(g),(h)))
#define ICLRDebugging_Release(T) ((T)->lpVtbl->Release(T))

// --- ICorDebugHeapEnum (IUnknown → ICorDebugEnum → ICorDebugHeapEnum) ---

typedef struct ICorDebugHeapEnum ICorDebugHeapEnum;

typedef struct ICorDebugHeapEnumVtbl
{
    BEGIN_INTERFACE
    DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
    HRESULT (STDMETHODCALLTYPE *QueryInterface) (ICorDebugHeapEnum*, REFIID, void**);
    DECLSPEC_XFGVIRT(IUnknown, AddRef)
    ULONG   (STDMETHODCALLTYPE *AddRef)         (ICorDebugHeapEnum*);
    DECLSPEC_XFGVIRT(IUnknown, Release)
    ULONG   (STDMETHODCALLTYPE *Release)        (ICorDebugHeapEnum*);
    // ICorDebugEnum:
    DECLSPEC_XFGVIRT(ICorDebugEnum, Skip)
    HRESULT (STDMETHODCALLTYPE *Skip)           (ICorDebugHeapEnum*, ULONG32);
    DECLSPEC_XFGVIRT(ICorDebugEnum, Reset)
    HRESULT (STDMETHODCALLTYPE *Reset)          (ICorDebugHeapEnum*);
    DECLSPEC_XFGVIRT(ICorDebugEnum, Clone)
    HRESULT (STDMETHODCALLTYPE *Clone)          (ICorDebugHeapEnum*, ICorDebugHeapEnum**);
    DECLSPEC_XFGVIRT(ICorDebugEnum, GetCount)
    HRESULT (STDMETHODCALLTYPE *GetCount)       (ICorDebugHeapEnum*, ULONG32*);
    // ICorDebugHeapEnum:
    DECLSPEC_XFGVIRT(ICorDebugHeapEnum, Next)
    HRESULT (STDMETHODCALLTYPE *Next)           (ICorDebugHeapEnum*, ULONG32, COR_HEAPOBJECT*, ULONG32*);
    END_INTERFACE
} ICorDebugHeapEnumVtbl;

struct ICorDebugHeapEnum { const ICorDebugHeapEnumVtbl* lpVtbl; };

#define ICorDebugHeapEnum_Next(T,c,o,f) ((T)->lpVtbl->Next((T),(c),(o),(f)))
#define ICorDebugHeapEnum_Release(T)    ((T)->lpVtbl->Release(T))

// --- COR_HEAPINFO (returned by ICorDebugProcess5::GetGCHeapInformation) ---

typedef enum _CorDebugGCType
{
    CorDebugWorkstationGC = 0x0,
    CorDebugServerGC      = 0x1,
} CorDebugGCType;

typedef struct _COR_HEAPINFO
{
    BOOL           areGCStructuresValid; // FALSE → heap not enumerable right now
    DWORD          pointerSize;
    DWORD          numHeaps;
    BOOL           concurrent;
    CorDebugGCType gcType;
} COR_HEAPINFO;

// --- ICorDebugProcess5 ---

typedef struct ICorDebugProcess5 ICorDebugProcess5;

typedef struct ICorDebugProcess5Vtbl
{
    BEGIN_INTERFACE
    DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
    HRESULT (STDMETHODCALLTYPE *QueryInterface)       (ICorDebugProcess5*, REFIID, void**);
    DECLSPEC_XFGVIRT(IUnknown, AddRef)
    ULONG   (STDMETHODCALLTYPE *AddRef)               (ICorDebugProcess5*);
    DECLSPEC_XFGVIRT(IUnknown, Release)
    ULONG   (STDMETHODCALLTYPE *Release)              (ICorDebugProcess5*);
    DECLSPEC_XFGVIRT(ICorDebugProcess5, GetGCHeapInformation)
    HRESULT (STDMETHODCALLTYPE *GetGCHeapInformation) (ICorDebugProcess5*, COR_HEAPINFO*);
    DECLSPEC_XFGVIRT(ICorDebugProcess5, EnumerateHeap)
    HRESULT (STDMETHODCALLTYPE *EnumerateHeap)        (ICorDebugProcess5*, ICorDebugHeapEnum**);
    // Remaining methods omitted — not called
    END_INTERFACE
} ICorDebugProcess5Vtbl;

struct ICorDebugProcess5 { const ICorDebugProcess5Vtbl* lpVtbl; };

#define ICorDebugProcess5_GetGCHeapInformation(T,p) ((T)->lpVtbl->GetGCHeapInformation((T),(p)))
#define ICorDebugProcess5_EnumerateHeap(T,pp)       ((T)->lpVtbl->EnumerateHeap((T),(pp)))
#define ICorDebugProcess5_Release(T)                ((T)->lpVtbl->Release(T))

// --- GUIDs ---

EXTERN_GUID(CLSID_CLRDebugging,
    0xbacc578d, 0xfbdd, 0x48a4, 0x96, 0x9f, 0x02, 0xd9, 0x32, 0xb7, 0x46, 0x34);
EXTERN_GUID(IID_ICLRDebugging,
    0xd28f3c5a, 0x9634, 0x4206, 0xa5, 0x09, 0x47, 0x75, 0x52, 0xee, 0xfb, 0x10);
EXTERN_GUID(IID_ICorDebugDataTarget,
    0xfe06dc28, 0x49fb, 0x4636, 0xa4, 0xa3, 0xe8, 0x0d, 0xb4, 0xae, 0x11, 0x6c);
EXTERN_GUID(IID_ICorDebugProcess5,
    0x21e9d9c0, 0xfcb8, 0x11df, 0x8c, 0xff, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66);
// IID_ICorDebugProcess is one of the accepted riidProcess values for OpenVirtualProcess
EXTERN_GUID(IID_ICorDebugProcess,
    0x3d6f5f64, 0x7538, 0x11d3, 0x8d, 0x5b, 0x00, 0x10, 0x4b, 0x35, 0xe7, 0xef);

#endif
