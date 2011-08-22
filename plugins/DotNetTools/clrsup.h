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

PVOID LoadMscordacwks(
    __in BOOLEAN IsClrV4
    );

HRESULT CreateXCLRDataProcess(
    __in HANDLE ProcessId,
    __in ICLRDataTarget *Target,
    __out struct IXCLRDataProcess **DataProcess
    );

// IXCLRDataProcess

typedef struct IXCLRDataProcess IXCLRDataProcess;

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

    // Members we don't need
    PVOID Flush;
    PVOID StartEnumTasks;
    PVOID EnumTask;
    PVOID EndEnumTasks;
    PVOID GetTaskByOSThreadID;
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
