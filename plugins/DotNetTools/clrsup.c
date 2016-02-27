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

#include "dn.h"
#include "clrsup.h"

static GUID IID_ICLRDataTarget_I = { 0x3e11ccee, 0xd08b, 0x43e5, { 0xaf, 0x01, 0x32, 0x71, 0x7a, 0x64, 0xda, 0x03 } };
static GUID IID_IXCLRDataProcess = { 0x5c552ab6, 0xfc09, 0x4cb3, { 0x8e, 0x36, 0x22, 0xfa, 0x03, 0xc7, 0x98, 0xb7 } };

static ICLRDataTargetVtbl DnCLRDataTarget_VTable =
{
    DnCLRDataTarget_QueryInterface,
    DnCLRDataTarget_AddRef,
    DnCLRDataTarget_Release,
    DnCLRDataTarget_GetMachineType,
    DnCLRDataTarget_GetPointerSize,
    DnCLRDataTarget_GetImageBase,
    DnCLRDataTarget_ReadVirtual,
    DnCLRDataTarget_WriteVirtual,
    DnCLRDataTarget_GetTLSValue,
    DnCLRDataTarget_SetTLSValue,
    DnCLRDataTarget_GetCurrentThreadID,
    DnCLRDataTarget_GetThreadContext,
    DnCLRDataTarget_SetThreadContext,
    DnCLRDataTarget_Request
};

PCLR_PROCESS_SUPPORT CreateClrProcessSupport(
    _In_ HANDLE ProcessId
    )
{
    PCLR_PROCESS_SUPPORT support;
    ICLRDataTarget *dataTarget;
    IXCLRDataProcess *dataProcess;

    dataTarget = DnCLRDataTarget_Create(ProcessId);

    if (!dataTarget)
        return NULL;

    dataProcess = NULL;
    CreateXCLRDataProcess(ProcessId, dataTarget, &dataProcess);
    ICLRDataTarget_Release(dataTarget);

    if (!dataProcess)
        return NULL;

    support = PhAllocate(sizeof(CLR_PROCESS_SUPPORT));
    support->DataProcess = dataProcess;

    return support;
}

VOID FreeClrProcessSupport(
    _In_ PCLR_PROCESS_SUPPORT Support
    )
{
    IXCLRDataProcess_Release(Support->DataProcess);
    PhFree(Support);
}

PPH_STRING GetRuntimeNameByAddressClrProcess(
    _In_ PCLR_PROCESS_SUPPORT Support,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 Displacement
    )
{
    PPH_STRING buffer;
    ULONG bufferLength;
    ULONG returnLength;
    ULONG64 displacement;

    bufferLength = 33;
    buffer = PhCreateStringEx(NULL, (bufferLength - 1) * 2);

    returnLength = 0;

    if (!SUCCEEDED(IXCLRDataProcess_GetRuntimeNameByAddress(
        Support->DataProcess,
        Address,
        0,
        bufferLength,
        &returnLength,
        buffer->Buffer,
        &displacement
        )))
    {
        PhDereferenceObject(buffer);
        return NULL;
    }

    // Try again if our buffer was too small.
    if (returnLength > bufferLength)
    {
        PhDereferenceObject(buffer);
        bufferLength = returnLength;
        buffer = PhCreateStringEx(NULL, (bufferLength - 1) * 2);

        if (!SUCCEEDED(IXCLRDataProcess_GetRuntimeNameByAddress(
            Support->DataProcess,
            Address,
            0,
            bufferLength,
            &returnLength,
            buffer->Buffer,
            &displacement
            )))
        {
            PhDereferenceObject(buffer);
            return NULL;
        }
    }

    if (Displacement)
        *Displacement = displacement;

    buffer->Length = (returnLength - 1) * 2;

    return buffer;
}

PPH_STRING GetNameXClrDataAppDomain(
    _In_ PVOID AppDomain
    )
{
    IXCLRDataAppDomain *appDomain;
    PPH_STRING buffer;
    ULONG bufferLength;
    ULONG returnLength;

    appDomain = AppDomain;

    bufferLength = 33;
    buffer = PhCreateStringEx(NULL, (bufferLength - 1) * 2);

    returnLength = 0;

    if (!SUCCEEDED(IXCLRDataAppDomain_GetName(appDomain, bufferLength, &returnLength, buffer->Buffer)))
    {
        PhDereferenceObject(buffer);
        return NULL;
    }

    // Try again if our buffer was too small.
    if (returnLength > bufferLength)
    {
        PhDereferenceObject(buffer);
        bufferLength = returnLength;
        buffer = PhCreateStringEx(NULL, (bufferLength - 1) * 2);

        if (!SUCCEEDED(IXCLRDataAppDomain_GetName(appDomain, bufferLength, &returnLength, buffer->Buffer)))
        {
            PhDereferenceObject(buffer);
            return NULL;
        }
    }

    buffer->Length = (returnLength - 1) * 2;

    return buffer;
}

PVOID LoadMscordacwks(
    _In_ BOOLEAN IsClrV4
    )
{
    PVOID dllBase;
    PH_STRINGREF systemRootString;
    PH_STRINGREF mscordacwksPathString;
    PPH_STRING mscordacwksFileName;

    LoadLibrary(L"mscoree.dll");

    PhGetSystemRoot(&systemRootString);

    if (IsClrV4)
    {
#ifdef _WIN64
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework64\\v4.0.30319\\mscordacwks.dll");
#else
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework\\v4.0.30319\\mscordacwks.dll");
#endif
    }
    else
    {
#ifdef _WIN64
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework64\\v2.0.50727\\mscordacwks.dll");
#else
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework\\v2.0.50727\\mscordacwks.dll");
#endif
    }

    mscordacwksFileName = PhConcatStringRef2(&systemRootString, &mscordacwksPathString);
    dllBase = LoadLibrary(mscordacwksFileName->Buffer);
    PhDereferenceObject(mscordacwksFileName);

    return dllBase;
}

HRESULT CreateXCLRDataProcess(
    _In_ HANDLE ProcessId,
    _In_ ICLRDataTarget *Target,
    _Out_ struct IXCLRDataProcess **DataProcess
    )
{
    ULONG flags;
    BOOLEAN clrV4;
    HMODULE dllBase;
    HRESULT (__stdcall *clrDataCreateInstance)(REFIID, ICLRDataTarget *, void **);

    clrV4 = FALSE;

    if (NT_SUCCESS(PhGetProcessIsDotNetEx(ProcessId, NULL, 0, NULL, &flags)))
    {
        if (flags & PH_CLR_VERSION_4_ABOVE)
            clrV4 = TRUE;
    }

    // Load the correct version of mscordacwks.dll.

    if (clrV4)
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        static HMODULE mscordacwksDllBase;

        if (PhBeginInitOnce(&initOnce))
        {
            mscordacwksDllBase = LoadMscordacwks(TRUE);
            PhEndInitOnce(&initOnce);
        }

        dllBase = mscordacwksDllBase;
    }
    else
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        static HMODULE mscordacwksDllBase;

        if (PhBeginInitOnce(&initOnce))
        {
            mscordacwksDllBase = LoadMscordacwks(FALSE);
            PhEndInitOnce(&initOnce);
        }

        dllBase = mscordacwksDllBase;
    }

    if (!dllBase)
        return E_FAIL;

    clrDataCreateInstance = PhGetProcedureAddress(dllBase, "CLRDataCreateInstance", 0);

    if (!clrDataCreateInstance)
        return E_FAIL;

    return clrDataCreateInstance(&IID_IXCLRDataProcess, Target, DataProcess);
}

ICLRDataTarget *DnCLRDataTarget_Create(
    _In_ HANDLE ProcessId
    )
{
    DnCLRDataTarget *dataTarget;
    HANDLE processHandle;
    BOOLEAN isWow64;

    if (!NT_SUCCESS(PhOpenProcess(&processHandle, ProcessQueryAccess | PROCESS_VM_READ, ProcessId)))
        return NULL;

#ifdef _WIN64
    if (!NT_SUCCESS(PhGetProcessIsWow64(processHandle, &isWow64)))
    {
        NtClose(processHandle);
        return NULL;
    }
#else
    isWow64 = FALSE;
#endif

    dataTarget = PhAllocate(sizeof(DnCLRDataTarget));
    dataTarget->VTable = &DnCLRDataTarget_VTable;
    dataTarget->RefCount = 1;

    dataTarget->ProcessId = ProcessId;
    dataTarget->ProcessHandle = processHandle;
    dataTarget->IsWow64 = isWow64;

    return (ICLRDataTarget *)dataTarget;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_QueryInterface(
    _In_ ICLRDataTarget *This,
    _In_ REFIID Riid,
    _Out_ PVOID *Object
    )
{
    if (
        IsEqualIID(Riid, &IID_IUnknown) ||
        IsEqualIID(Riid, &IID_ICLRDataTarget_I)
        )
    {
        DnCLRDataTarget_AddRef(This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE DnCLRDataTarget_AddRef(
    _In_ ICLRDataTarget *This
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

    this->RefCount++;

    return this->RefCount;
}

ULONG STDMETHODCALLTYPE DnCLRDataTarget_Release(
    _In_ ICLRDataTarget *This
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

    this->RefCount--;

    if (this->RefCount == 0)
    {
        NtClose(this->ProcessHandle);

        PhFree(this);

        return 0;
    }

    return this->RefCount;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetMachineType(
    _In_ ICLRDataTarget *This,
    _Out_ ULONG32 *machineType
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

#ifdef _WIN64
    if (!this->IsWow64)
        *machineType = IMAGE_FILE_MACHINE_AMD64;
    else
        *machineType = IMAGE_FILE_MACHINE_I386;
#else
    *machineType = IMAGE_FILE_MACHINE_I386;
#endif

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetPointerSize(
    _In_ ICLRDataTarget *This,
    _Out_ ULONG32 *pointerSize
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

#ifdef _WIN64
    if (!this->IsWow64)
#endif
        *pointerSize = sizeof(PVOID);
#ifdef _WIN64
    else
        *pointerSize = sizeof(ULONG);
#endif

    return S_OK;
}

BOOLEAN NTAPI PhpGetImageBaseCallback(
    _In_ PLDR_DATA_TABLE_ENTRY Module,
    _In_opt_ PVOID Context
    )
{
    PPHP_GET_IMAGE_BASE_CONTEXT context = Context;

    if (RtlEqualUnicodeString(&Module->FullDllName, &context->ImagePath, TRUE) ||
        RtlEqualUnicodeString(&Module->BaseDllName, &context->ImagePath, TRUE))
    {
        context->BaseAddress = Module->DllBase;
        return FALSE;
    }

    return TRUE;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetImageBase(
    _In_ ICLRDataTarget *This,
    _In_ LPCWSTR imagePath,
    _Out_ CLRDATA_ADDRESS *baseAddress
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;
    PHP_GET_IMAGE_BASE_CONTEXT context;

    RtlInitUnicodeString(&context.ImagePath, (PWSTR)imagePath);
    context.BaseAddress = NULL;
    PhEnumProcessModules(this->ProcessHandle, PhpGetImageBaseCallback, &context);

#ifdef _WIN64
    if (this->IsWow64)
        PhEnumProcessModules32(this->ProcessHandle, PhpGetImageBaseCallback, &context);
#endif

    if (context.BaseAddress)
    {
        *baseAddress = (CLRDATA_ADDRESS)context.BaseAddress;

        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_ReadVirtual(
    _In_ ICLRDataTarget *This,
    _In_ CLRDATA_ADDRESS address,
    _Out_ BYTE *buffer,
    _In_ ULONG32 bytesRequested,
    _Out_ ULONG32 *bytesRead
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;
    NTSTATUS status;
    SIZE_T numberOfBytesRead;

    if (NT_SUCCESS(status = PhReadVirtualMemory(
        this->ProcessHandle,
        (PVOID)address,
        buffer,
        bytesRequested,
        &numberOfBytesRead
        )))
    {
        *bytesRead = (ULONG32)numberOfBytesRead;

        return S_OK;
    }
    else
    {
        ULONG result;

        result = RtlNtStatusToDosError(status);

        return HRESULT_FROM_WIN32(result);
    }
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_WriteVirtual(
    _In_ ICLRDataTarget *This,
    _In_ CLRDATA_ADDRESS address,
    _In_ BYTE *buffer,
    _In_ ULONG32 bytesRequested,
    _Out_ ULONG32 *bytesWritten
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetTLSValue(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 index,
    _Out_ CLRDATA_ADDRESS *value
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetTLSValue(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 index,
    _In_ CLRDATA_ADDRESS value
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetCurrentThreadID(
    _In_ ICLRDataTarget *This,
    _Out_ ULONG32 *threadID
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetThreadContext(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 contextFlags,
    _In_ ULONG32 contextSize,
    _Out_ BYTE *context
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    CONTEXT buffer;

    if (contextSize < sizeof(CONTEXT))
        return E_INVALIDARG;

    memset(&buffer, 0, sizeof(CONTEXT));
    buffer.ContextFlags = contextFlags;

    if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_GET_CONTEXT, UlongToHandle(threadID))))
    {
        status = PhGetThreadContext(threadHandle, &buffer);
        NtClose(threadHandle);
    }

    if (NT_SUCCESS(status))
    {
        memcpy(context, &buffer, sizeof(CONTEXT));

        return S_OK;
    }
    else
    {
        return HRESULT_FROM_WIN32(RtlNtStatusToDosError(status));
    }
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetThreadContext(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 contextSize,
    _In_ BYTE *context
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_Request(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 reqCode,
    _In_ ULONG32 inBufferSize,
    _In_ BYTE *inBuffer,
    _In_ ULONG32 outBufferSize,
    _Out_ BYTE *outBuffer
    )
{
    return E_NOTIMPL;
}
