#ifndef CLRSUP_H
#define CLRSUP_H

#define CINTERFACE
#define COBJMACROS
#include <clrdata.h>
#undef CINTERFACE
#undef COBJMACROS

// General interfaces

typedef struct _CLR_PROCESS_SUPPORT
{
    struct IXCLRDataProcess *DataProcess;
} CLR_PROCESS_SUPPORT, *PCLR_PROCESS_SUPPORT;

PCLR_PROCESS_SUPPORT CreateClrProcessSupport(
    __in HANDLE ProcessId
    );

VOID FreeClrProcessSupport(
    __in PCLR_PROCESS_SUPPORT Support
    );

PPH_STRING GetRuntimeNameByAddressClrProcess(
    __in PCLR_PROCESS_SUPPORT Support,
    __in ULONG64 Address,
    __out_opt PULONG64 Displacement
    );

PPH_STRING GetNameXClrDataAppDomain(
    __in PVOID AppDomain
    );

PVOID LoadMscordacwks(
    __in BOOLEAN IsClrV4
    );

HRESULT CreateXCLRDataProcess(
    __in HANDLE ProcessId,
    __in ICLRDataTarget *Target,
    __out struct IXCLRDataProcess **DataProcess
    );

// xclrdata

typedef ULONG64 CLRDATA_ENUM;

typedef struct IXCLRDataProcess IXCLRDataProcess;
typedef struct IXCLRDataAppDomain IXCLRDataAppDomain;
typedef struct IXCLRDataTask IXCLRDataTask;

typedef struct IXCLRDataProcessVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        __in IXCLRDataProcess *This,
        __in REFIID riid,
        __deref_out void **ppvObject
        );

    ULONG (STDMETHODCALLTYPE *AddRef)(
        __in IXCLRDataProcess *This
        );

    ULONG (STDMETHODCALLTYPE *Release)(
        __in IXCLRDataProcess *This
        );

    HRESULT (STDMETHODCALLTYPE *Flush)(
        __in IXCLRDataProcess *This
        );

    HRESULT (STDMETHODCALLTYPE *StartEnumTasks)(
        __in IXCLRDataProcess *This,
        __out CLRDATA_ENUM *handle
        );

    HRESULT (STDMETHODCALLTYPE *EnumTask)(
        __in IXCLRDataProcess *This,
        __inout CLRDATA_ENUM *handle,
        __out IXCLRDataTask **task
        );

    HRESULT (STDMETHODCALLTYPE *EndEnumTasks)(
        __in IXCLRDataProcess *This,
        __in CLRDATA_ENUM handle
        );

    HRESULT (STDMETHODCALLTYPE *GetTaskByOSThreadID)(
        __in IXCLRDataProcess *This,
        __in ULONG32 osThreadID,
        __out IXCLRDataTask **task
        );

    PVOID GetTaskByUniqueID;
    PVOID GetFlags;
    PVOID IsSameObject;
    PVOID GetManagedObject;
    PVOID GetDesiredExecutionState;
    PVOID SetDesiredExecutionState;
    PVOID GetAddressType;

    HRESULT (STDMETHODCALLTYPE *GetRuntimeNameByAddress)(
        __in IXCLRDataProcess *This,
        __in CLRDATA_ADDRESS address,
        __in ULONG32 flags,
        __in ULONG32 bufLen,
        __out ULONG32 *nameLen,
        __out WCHAR *nameBuf,
        __out CLRDATA_ADDRESS *displacement
        );

    // ...
} IXCLRDataProcessVtbl;

typedef struct IXCLRDataProcess
{
    struct IXCLRDataProcessVtbl *lpVtbl;
} IXCLRDataProcess;

#define IXCLRDataProcess_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))

#define IXCLRDataProcess_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))

#define IXCLRDataProcess_Release(This) \
    ((This)->lpVtbl->Release(This))

#define IXCLRDataProcess_GetRuntimeNameByAddress(This, address, flags, bufLen, nameLen, nameBuf, displacement) \
    ((This)->lpVtbl->GetRuntimeNameByAddress(This, address, flags, bufLen, nameLen, nameBuf, displacement))

#define IXCLRDataProcess_Flush(This) \
    ((This)->lpVtbl->Flush(This))

#define IXCLRDataProcess_StartEnumTasks(This, handle) \
    ((This)->lpVtbl->StartEnumTasks(This, handle))

#define IXCLRDataProcess_EnumTask(This, handle, task) \
    ((This)->lpVtbl->EnumTask(This, handle, task))

#define IXCLRDataProcess_EndEnumTasks(This, handle) \
    ((This)->lpVtbl->EndEnumTasks(This, handle))

#define IXCLRDataProcess_GetTaskByOSThreadID(This, osThreadID, task) \
    ((This)->lpVtbl->GetTaskByOSThreadID(This, osThreadID, task))

typedef struct IXCLRDataAppDomainVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        __in IXCLRDataAppDomain *This,
        __in REFIID riid,
        __deref_out void **ppvObject
        );

    ULONG (STDMETHODCALLTYPE *AddRef)(
        __in IXCLRDataAppDomain *This
        );

    ULONG (STDMETHODCALLTYPE *Release)(
        __in IXCLRDataAppDomain *This
        );

    HRESULT (STDMETHODCALLTYPE *GetProcess)(
        __in IXCLRDataAppDomain *This,
        __out IXCLRDataProcess **process
        );

    HRESULT (STDMETHODCALLTYPE *GetName)(
        __in IXCLRDataAppDomain *This,
        __in ULONG32 bufLen,
        __out ULONG32 *nameLen,
        __out WCHAR *name
        );

    HRESULT (STDMETHODCALLTYPE *GetUniqueID)(
        __in IXCLRDataAppDomain *This,
        __out ULONG64 *id
        );

    // ...
} IXCLRDataAppDomainVtbl;

typedef struct IXCLRDataAppDomain
{
    struct IXCLRDataAppDomainVtbl *lpVtbl;
} IXCLRDataAppDomain;

#define IXCLRDataAppDomain_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))

#define IXCLRDataAppDomain_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))

#define IXCLRDataAppDomain_Release(This) \
    ((This)->lpVtbl->Release(This))

#define IXCLRDataAppDomain_GetProcess(This, process) \
    ((This)->lpVtbl->GetProcess(This, process))

#define IXCLRDataAppDomain_GetName(This, bufLen, nameLen, name) \
    ((This)->lpVtbl->GetName(This, bufLen, nameLen, name))

#define IXCLRDataAppDomain_GetUniqueID(This, id) \
    ((This)->lpVtbl->GetUniqueID(This, id))

typedef struct IXCLRDataTaskVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        __in IXCLRDataTask *This,
        __in REFIID riid,
        __deref_out void **ppvObject
        );

    ULONG (STDMETHODCALLTYPE *AddRef)(
        __in IXCLRDataTask *This
        );

    ULONG (STDMETHODCALLTYPE *Release)(
        __in IXCLRDataTask *This
        );

    HRESULT (STDMETHODCALLTYPE *GetProcess)(
        __in IXCLRDataTask *This,
        __out IXCLRDataProcess **process
        );

    HRESULT (STDMETHODCALLTYPE *GetCurrentAppDomain)(
        __in IXCLRDataTask *This,
        __out IXCLRDataAppDomain **appDomain
        );

    HRESULT (STDMETHODCALLTYPE *GetUniqueID)(
        __in IXCLRDataTask *This,
        __out ULONG64 *id
        );

    HRESULT (STDMETHODCALLTYPE *GetFlags)(
        __in IXCLRDataTask *This,
        __out ULONG32 *flags
        );

    PVOID IsSameObject;
    PVOID GetManagedObject;
    PVOID GetDesiredExecutionState;
    PVOID SetDesiredExecutionState;
    PVOID CreateStackWalk;

    HRESULT (STDMETHODCALLTYPE *GetOSThreadID)(
        __in IXCLRDataTask *This,
        __out ULONG32 *id
        );

    PVOID GetContext;
    PVOID SetContext;
    PVOID GetCurrentExceptionState;
    PVOID Request;

    HRESULT (STDMETHODCALLTYPE *GetName)(
        __in IXCLRDataTask *This,
        __in ULONG32 bufLen,
        __out ULONG32 *nameLen,
        __out WCHAR *name
        );

    PVOID GetLastExceptionState;
} IXCLRDataTaskVtbl;

typedef struct IXCLRDataTask
{
    struct IXCLRDataTaskVtbl *lpVtbl;
} IXCLRDataTask;

#define IXCLRDataTask_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))

#define IXCLRDataTask_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))

#define IXCLRDataTask_Release(This) \
    ((This)->lpVtbl->Release(This))

#define IXCLRDataTask_GetProcess(This, process) \
    ((This)->lpVtbl->GetProcess(This, process))

#define IXCLRDataTask_GetCurrentAppDomain(This, appDomain) \
    ((This)->lpVtbl->GetCurrentAppDomain(This, appDomain))

#define IXCLRDataTask_GetUniqueID(This, id) \
    ((This)->lpVtbl->GetUniqueID(This, id))

#define IXCLRDataTask_GetFlags(This, flags) \
    ((This)->lpVtbl->GetFlags(This, flags))

#define IXCLRDataTask_GetOSThreadID(This, id) \
    ((This)->lpVtbl->GetOSThreadID(This, id))

#define IXCLRDataTask_GetName(This, bufLen, nameLen, name) \
    ((This)->lpVtbl->GetName(This, bufLen, nameLen, name))

// DnCLRDataTarget

typedef struct
{
    ICLRDataTargetVtbl *VTable;

    ULONG RefCount;

    HANDLE ProcessId;
    HANDLE ProcessHandle;
    BOOLEAN IsWow64;
} DnCLRDataTarget;

ICLRDataTarget *DnCLRDataTarget_Create(
    __in HANDLE ProcessId
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_QueryInterface(
    __in ICLRDataTarget *This,
    __in REFIID Riid,
    __out PVOID *Object
    );

ULONG STDMETHODCALLTYPE DnCLRDataTarget_AddRef(
    __in ICLRDataTarget *This
    );

ULONG STDMETHODCALLTYPE DnCLRDataTarget_Release(
    __in ICLRDataTarget *This
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetMachineType(
    __in ICLRDataTarget *This,
    __out ULONG32 *machineType
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetPointerSize(
    __in ICLRDataTarget *This,
    __out ULONG32 *pointerSize
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetImageBase(
    __in ICLRDataTarget *This,
    __in LPCWSTR imagePath,
    __out CLRDATA_ADDRESS *baseAddress
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_ReadVirtual(
    __in ICLRDataTarget *This,
    __in CLRDATA_ADDRESS address,
    __out BYTE *buffer,
    __in ULONG32 bytesRequested,
    __out ULONG32 *bytesRead
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_WriteVirtual(
    __in ICLRDataTarget *This,
    __in CLRDATA_ADDRESS address,
    __in BYTE *buffer,
    __in ULONG32 bytesRequested,
    __out ULONG32 *bytesWritten
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetTLSValue(
    __in ICLRDataTarget *This,
    __in ULONG32 threadID,
    __in ULONG32 index,
    __out CLRDATA_ADDRESS *value
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetTLSValue(
    __in ICLRDataTarget *This,
    __in ULONG32 threadID,
    __in ULONG32 index,
    __in CLRDATA_ADDRESS value
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetCurrentThreadID(
    __in ICLRDataTarget *This,
    __out ULONG32 *threadID
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetThreadContext(
    __in ICLRDataTarget *This,
    __in ULONG32 threadID,
    __in ULONG32 contextFlags,
    __in ULONG32 contextSize,
    __out BYTE *context
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetThreadContext(
    __in ICLRDataTarget *This,
    __in ULONG32 threadID,
    __in ULONG32 contextSize,
    __in BYTE *context
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_Request(
    __in ICLRDataTarget *This,
    __in ULONG32 reqCode,
    __in ULONG32 inBufferSize,
    __in BYTE *inBuffer,
    __in ULONG32 outBufferSize,
    __out BYTE *outBuffer
    );

typedef struct _PHP_GET_IMAGE_BASE_CONTEXT
{
    PH_STRINGREF ImagePath;
    PVOID BaseAddress;
} PHP_GET_IMAGE_BASE_CONTEXT, *PPHP_GET_IMAGE_BASE_CONTEXT;

#endif
