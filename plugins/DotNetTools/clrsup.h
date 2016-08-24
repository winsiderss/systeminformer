/*
 * Process Hacker .NET Tools -
 *   CLR data access functions
 *
 * Copyright (C) 2011-2015 wj32
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

#ifndef CLRSUP_H
#define CLRSUP_H

#define CINTERFACE
#define COBJMACROS
#include "clr/clrdata.h"
#undef CINTERFACE
#undef COBJMACROS

// General interfaces

typedef struct _CLR_PROCESS_SUPPORT
{
    struct IXCLRDataProcess *DataProcess;
} CLR_PROCESS_SUPPORT, *PCLR_PROCESS_SUPPORT;

PCLR_PROCESS_SUPPORT CreateClrProcessSupport(
    _In_ HANDLE ProcessId
    );

VOID FreeClrProcessSupport(
    _In_ PCLR_PROCESS_SUPPORT Support
    );

PPH_STRING GetRuntimeNameByAddressClrProcess(
    _In_ PCLR_PROCESS_SUPPORT Support,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 Displacement
    );

PPH_STRING GetNameXClrDataAppDomain(
    _In_ PVOID AppDomain
    );

PVOID LoadMscordacwks(
    _In_ BOOLEAN IsClrV4
    );

HRESULT CreateXCLRDataProcess(
    _In_ HANDLE ProcessId,
    _In_ ICLRDataTarget *Target,
    _Out_ struct IXCLRDataProcess **DataProcess
    );

// xclrdata

typedef ULONG64 CLRDATA_ENUM;

typedef struct IXCLRDataProcess IXCLRDataProcess;
typedef struct IXCLRDataAppDomain IXCLRDataAppDomain;
typedef struct IXCLRDataTask IXCLRDataTask;
typedef struct IXCLRDataStackWalk IXCLRDataStackWalk;
typedef struct IXCLRDataFrame IXCLRDataFrame;

typedef struct IXCLRDataProcessVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        _In_ IXCLRDataProcess *This,
        _In_ REFIID riid,
        _Outptr_ void **ppvObject
        );

    ULONG (STDMETHODCALLTYPE *AddRef)(
        _In_ IXCLRDataProcess *This
        );

    ULONG (STDMETHODCALLTYPE *Release)(
        _In_ IXCLRDataProcess *This
        );

    HRESULT (STDMETHODCALLTYPE *Flush)(
        _In_ IXCLRDataProcess *This
        );

    HRESULT (STDMETHODCALLTYPE *StartEnumTasks)(
        _In_ IXCLRDataProcess *This,
        _Out_ CLRDATA_ENUM *handle
        );

    HRESULT (STDMETHODCALLTYPE *EnumTask)(
        _In_ IXCLRDataProcess *This,
        _Inout_ CLRDATA_ENUM *handle,
        _Out_ IXCLRDataTask **task
        );

    HRESULT (STDMETHODCALLTYPE *EndEnumTasks)(
        _In_ IXCLRDataProcess *This,
        _In_ CLRDATA_ENUM handle
        );

    HRESULT (STDMETHODCALLTYPE *GetTaskByOSThreadID)(
        _In_ IXCLRDataProcess *This,
        _In_ ULONG32 osThreadID,
        _Out_ IXCLRDataTask **task
        );

    PVOID GetTaskByUniqueID;
    PVOID GetFlags;
    PVOID IsSameObject;
    PVOID GetManagedObject;
    PVOID GetDesiredExecutionState;
    PVOID SetDesiredExecutionState;
    PVOID GetAddressType;

    HRESULT (STDMETHODCALLTYPE *GetRuntimeNameByAddress)(
        _In_ IXCLRDataProcess *This,
        _In_ CLRDATA_ADDRESS address,
        _In_ ULONG32 flags,
        _In_ ULONG32 bufLen,
        _Out_ ULONG32 *nameLen,
        _Out_ WCHAR *nameBuf,
        _Out_ CLRDATA_ADDRESS *displacement
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
        _In_ IXCLRDataAppDomain *This,
        _In_ REFIID riid,
        _Outptr_ void **ppvObject
        );

    ULONG (STDMETHODCALLTYPE *AddRef)(
        _In_ IXCLRDataAppDomain *This
        );

    ULONG (STDMETHODCALLTYPE *Release)(
        _In_ IXCLRDataAppDomain *This
        );

    HRESULT (STDMETHODCALLTYPE *GetProcess)(
        _In_ IXCLRDataAppDomain *This,
        _Out_ IXCLRDataProcess **process
        );

    HRESULT (STDMETHODCALLTYPE *GetName)(
        _In_ IXCLRDataAppDomain *This,
        _In_ ULONG32 bufLen,
        _Out_ ULONG32 *nameLen,
        _Out_ WCHAR *name
        );

    HRESULT (STDMETHODCALLTYPE *GetUniqueID)(
        _In_ IXCLRDataAppDomain *This,
        _Out_ ULONG64 *id
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
        _In_ IXCLRDataTask *This,
        _In_ REFIID riid,
        _Outptr_ void **ppvObject
        );

    ULONG (STDMETHODCALLTYPE *AddRef)(
        _In_ IXCLRDataTask *This
        );

    ULONG (STDMETHODCALLTYPE *Release)(
        _In_ IXCLRDataTask *This
        );

    HRESULT (STDMETHODCALLTYPE *GetProcess)(
        _In_ IXCLRDataTask *This,
        _Out_ IXCLRDataProcess **process
        );

    HRESULT (STDMETHODCALLTYPE *GetCurrentAppDomain)(
        _In_ IXCLRDataTask *This,
        _Out_ IXCLRDataAppDomain **appDomain
        );

    HRESULT (STDMETHODCALLTYPE *GetUniqueID)(
        _In_ IXCLRDataTask *This,
        _Out_ ULONG64 *id
        );

    HRESULT (STDMETHODCALLTYPE *GetFlags)(
        _In_ IXCLRDataTask *This,
        _Out_ ULONG32 *flags
        );

    PVOID IsSameObject;
    PVOID GetManagedObject;
    PVOID GetDesiredExecutionState;
    PVOID SetDesiredExecutionState;

    HRESULT (STDMETHODCALLTYPE *CreateStackWalk)(
        _In_ IXCLRDataTask *This,
        _In_ ULONG32 flags,
        _Out_ IXCLRDataStackWalk **stackWalk
        );

    HRESULT (STDMETHODCALLTYPE *GetOSThreadID)(
        _In_ IXCLRDataTask *This,
        _Out_ ULONG32 *id
        );

    PVOID GetContext;
    PVOID SetContext;
    PVOID GetCurrentExceptionState;
    PVOID Request;

    HRESULT (STDMETHODCALLTYPE *GetName)(
        _In_ IXCLRDataTask *This,
        _In_ ULONG32 bufLen,
        _Out_ ULONG32 *nameLen,
        _Out_ WCHAR *name
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

#define IXCLRDataTask_CreateStackWalk(This, flags, stackWalk) \
    ((This)->lpVtbl->CreateStackWalk(This, flags, stackWalk))

#define IXCLRDataTask_GetOSThreadID(This, id) \
    ((This)->lpVtbl->GetOSThreadID(This, id))

#define IXCLRDataTask_GetName(This, bufLen, nameLen, name) \
    ((This)->lpVtbl->GetName(This, bufLen, nameLen, name))

typedef enum
{
    CLRDATA_SIMPFRAME_UNRECOGNIZED = 0x1,
    CLRDATA_SIMPFRAME_MANAGED_METHOD = 0x2,
    CLRDATA_SIMPFRAME_RUNTIME_MANAGED_CODE = 0x4,
    CLRDATA_SIMPFRAME_RUNTIME_UNMANAGED_CODE = 0x8
} CLRDataSimpleFrameType;

typedef enum
{
    CLRDATA_DETFRAME_UNRECOGNIZED,
    CLRDATA_DETFRAME_UNKNOWN_STUB,
    CLRDATA_DETFRAME_CLASS_INIT,
    CLRDATA_DETFRAME_EXCEPTION_FILTER,
    CLRDATA_DETFRAME_SECURITY,
    CLRDATA_DETFRAME_CONTEXT_POLICY,
    CLRDATA_DETFRAME_INTERCEPTION,
    CLRDATA_DETFRAME_PROCESS_START,
    CLRDATA_DETFRAME_THREAD_START,
    CLRDATA_DETFRAME_TRANSITION_TO_MANAGED,
    CLRDATA_DETFRAME_TRANSITION_TO_UNMANAGED,
    CLRDATA_DETFRAME_COM_INTEROP_STUB,
    CLRDATA_DETFRAME_DEBUGGER_EVAL,
    CLRDATA_DETFRAME_CONTEXT_SWITCH,
    CLRDATA_DETFRAME_FUNC_EVAL,
    CLRDATA_DETFRAME_FINALLY
} CLRDataDetailedFrameType;

typedef enum
{
    CLRDATA_STACK_SET_UNWIND_CONTEXT  = 0x00000000,
    CLRDATA_STACK_SET_CURRENT_CONTEXT = 0x00000001
} CLRDataStackSetContextFlag;

typedef struct IXCLRDataStackWalkVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        _In_ IXCLRDataStackWalk *This,
        _In_ REFIID riid,
        _Outptr_ void **ppvObject
        );

    ULONG (STDMETHODCALLTYPE *AddRef)(
        _In_ IXCLRDataStackWalk *This
        );

    ULONG (STDMETHODCALLTYPE *Release)(
        _In_ IXCLRDataStackWalk *This
        );

    HRESULT (STDMETHODCALLTYPE *GetContext)(
        _In_ IXCLRDataStackWalk *This,
        _In_ ULONG32 contextFlags,
        _In_ ULONG32 contextBufSize,
        _Out_ ULONG32 *contextSize,
        _Out_ BYTE *contextBuf
        );

    PVOID SetContext;

    HRESULT (STDMETHODCALLTYPE *Next)(
        _In_ IXCLRDataStackWalk *This
        );

    HRESULT (STDMETHODCALLTYPE *GetStackSizeSkipped)(
        _In_ IXCLRDataStackWalk *This,
        _Out_ ULONG64 *stackSizeSkipped
        );

    HRESULT (STDMETHODCALLTYPE *GetFrameType)(
        _In_ IXCLRDataStackWalk *This,
        _Out_ CLRDataSimpleFrameType *simpleType,
        _Out_ CLRDataDetailedFrameType *detailedType
        );

    HRESULT (STDMETHODCALLTYPE *GetFrame)(
        _In_ IXCLRDataStackWalk *This,
        _Out_ PVOID *frame
        );

    HRESULT (STDMETHODCALLTYPE *Request)(
        _In_ IXCLRDataStackWalk *This,
        _In_ ULONG32 reqCode,
        _In_ ULONG32 inBufferSize,
        _In_ BYTE *inBuffer,
        _In_ ULONG32 outBufferSize,
        _Out_ BYTE *outBuffer
        );

    HRESULT (STDMETHODCALLTYPE *SetContext2)(
        _In_ IXCLRDataStackWalk *This,
        _In_ ULONG32 flags,
        _In_ ULONG32 contextSize,
        _In_ BYTE *context
        );
} IXCLRDataStackWalkVtbl;

typedef struct IXCLRDataStackWalk
{
    struct IXCLRDataStackWalkVtbl *lpVtbl;
} IXCLRDataStackWalk;

#define IXCLRDataStackWalk_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))

#define IXCLRDataStackWalk_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))

#define IXCLRDataStackWalk_Release(This) \
    ((This)->lpVtbl->Release(This))

#define IXCLRDataStackWalk_GetContext(This, contextFlags, contextBufSize, contextSize, contextBuf) \
    ((This)->lpVtbl->GetContext(This, contextFlags, contextBufSize, contextSize, contextBuf))

#define IXCLRDataStackWalk_Next(This) \
    ((This)->lpVtbl->Next(This))

#define IXCLRDataStackWalk_GetStackSizeSkipped(This, stackSizeSkipped) \
    ((This)->lpVtbl->GetStackSizeSkipped(This, stackSizeSkipped))

#define IXCLRDataStackWalk_GetFrameType(This, simpleType, detailedType) \
    ((This)->lpVtbl->GetFrameType(This, simpleType, detailedType))

#define IXCLRDataStackWalk_GetFrame(This, frame) \
    ((This)->lpVtbl->GetFrame(This, frame))

#define IXCLRDataStackWalk_Request(This, reqCode, inBufferSize, inBuffer, outBufferSize, outBuffer) \
    ((This)->lpVtbl->SetContext2(This, reqCode, inBufferSize, inBuffer, outBufferSize, outBuffer))

#define IXCLRDataStackWalk_SetContext2(This, flags, contextSize, context) \
    ((This)->lpVtbl->SetContext2(This, flags, contextSize, context))

typedef struct IXCLRDataFrameVtbl
{
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        _In_ IXCLRDataFrame *This,
        _In_ REFIID riid,
        _Outptr_ void **ppvObject
        );

    ULONG (STDMETHODCALLTYPE *AddRef)(
        _In_ IXCLRDataFrame *This
        );

    ULONG (STDMETHODCALLTYPE *Release)(
        _In_ IXCLRDataFrame *This
        );

    HRESULT (STDMETHODCALLTYPE *GetFrameType)(
        _In_ IXCLRDataFrame *This,
        _Out_ CLRDataSimpleFrameType *simpleType,
        _Out_ CLRDataDetailedFrameType *detailedType
        );

    HRESULT (STDMETHODCALLTYPE *GetContext)(
        _In_ IXCLRDataFrame *This,
        _In_ ULONG32 contextFlags,
        _In_ ULONG32 contextBufSize,
        _Out_ ULONG32 *contextSize,
        _Out_ BYTE *contextBuf
        );

    PVOID GetAppDomain;
    PVOID GetNumArguments;
    PVOID GetArgumentByIndex;
    PVOID GetNumLocalVariables;
    PVOID GetLocalVariableByIndex;

    HRESULT (STDMETHODCALLTYPE *GetCodeName)(
        _In_ IXCLRDataFrame *This,
        _In_ ULONG32 *flags,
        _In_ ULONG32 *bufLen,
        _Out_ ULONG32 *nameLen,
        _Out_ WCHAR *nameBuf
        );
} IXCLRDataFrameVtbl;

typedef struct IXCLRDataFrame
{
    IXCLRDataFrameVtbl *lpVtbl;
} IXCLRDataFrame;

#define IXCLRDataFrame_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))

#define IXCLRDataFrame_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))

#define IXCLRDataFrame_Release(This) \
    ((This)->lpVtbl->Release(This))

#define IXCLRDataFrame_GetFrameType(This, simpleType, detailedType) \
    ((This)->lpVtbl->GetFrameType(This, simpleType, detailedType))

#define IXCLRDataFrame_GetContext(This, contextFlags, contextBufSize, contextSize, contextBuf) \
    ((This)->lpVtbl->GetContext(This, contextFlags, contextBufSize, contextSize, contextBuf))

#define IXCLRDataFrame_GetCodeName(This, flags, bufLen, nameLen, nameBuf) \
    ((This)->lpVtbl->GetCodeName(This, flags, bufLen, nameLen, nameBuf))

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
    _In_ HANDLE ProcessId
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_QueryInterface(
    _In_ ICLRDataTarget *This,
    _In_ REFIID Riid,
    _Out_ PVOID *Object
    );

ULONG STDMETHODCALLTYPE DnCLRDataTarget_AddRef(
    _In_ ICLRDataTarget *This
    );

ULONG STDMETHODCALLTYPE DnCLRDataTarget_Release(
    _In_ ICLRDataTarget *This
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetMachineType(
    _In_ ICLRDataTarget *This,
    _Out_ ULONG32 *machineType
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetPointerSize(
    _In_ ICLRDataTarget *This,
    _Out_ ULONG32 *pointerSize
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetImageBase(
    _In_ ICLRDataTarget *This,
    _In_ LPCWSTR imagePath,
    _Out_ CLRDATA_ADDRESS *baseAddress
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_ReadVirtual(
    _In_ ICLRDataTarget *This,
    _In_ CLRDATA_ADDRESS address,
    _Out_ BYTE *buffer,
    _In_ ULONG32 bytesRequested,
    _Out_ ULONG32 *bytesRead
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_WriteVirtual(
    _In_ ICLRDataTarget *This,
    _In_ CLRDATA_ADDRESS address,
    _In_ BYTE *buffer,
    _In_ ULONG32 bytesRequested,
    _Out_ ULONG32 *bytesWritten
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetTLSValue(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 index,
    _Out_ CLRDATA_ADDRESS *value
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetTLSValue(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 index,
    _In_ CLRDATA_ADDRESS value
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetCurrentThreadID(
    _In_ ICLRDataTarget *This,
    _Out_ ULONG32 *threadID
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetThreadContext(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 contextFlags,
    _In_ ULONG32 contextSize,
    _Out_ BYTE *context
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetThreadContext(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 contextSize,
    _In_ BYTE *context
    );

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_Request(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 reqCode,
    _In_ ULONG32 inBufferSize,
    _In_ BYTE *inBuffer,
    _In_ ULONG32 outBufferSize,
    _Out_ BYTE *outBuffer
    );

typedef struct _PHP_GET_IMAGE_BASE_CONTEXT
{
    UNICODE_STRING ImagePath;
    PVOID BaseAddress;
} PHP_GET_IMAGE_BASE_CONTEXT, *PPHP_GET_IMAGE_BASE_CONTEXT;

#endif
