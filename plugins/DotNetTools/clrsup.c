/*
 * Process Hacker .NET Tools - 
 *   CLR debugging functions
 * 
 * Copyright (C) 2011 wj32
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
    __in HANDLE ProcessId
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
    __in PCLR_PROCESS_SUPPORT Support
    )
{
    IXCLRDataProcess_Release(Support->DataProcess);
    PhFree(Support);
}

PPH_STRING GetRuntimeNameByAddressClrProcess(
    __in PCLR_PROCESS_SUPPORT Support,
    __in ULONG64 Address,
    __out_opt PULONG64 Displacement
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

    return buffer;
}

PVOID LoadMscordacwks(
    __in BOOLEAN IsClrV4
    )
{
    PVOID dllBase;
    PH_STRINGREF systemRootString;
    PH_STRINGREF mscordacwksPathString;
    PPH_STRING mscordacwksFileName;

    LoadLibrary(L"mscoree.dll");

    PhInitializeStringRef(&systemRootString, USER_SHARED_DATA->NtSystemRoot);

    if (IsClrV4)
    {
#ifdef _M_X64
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework64\\v4.0.30319\\mscordacwks.dll");
#else
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework\\v4.0.30319\\mscordacwks.dll");
#endif
    }
    else
    {
#ifdef _M_X64
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework64\\v2.0.50727\\mscordacwks.dll");
#else
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework\\v2.0.50727\\mscordacwks.dll");
#endif
    }

    // Make sure the system root string doesn't end with a backslash.
    if (systemRootString.Buffer[systemRootString.Length / 2 - 1] == '\\')
        systemRootString.Length -= 2;

    mscordacwksFileName = PhConcatStringRef2(&systemRootString, &mscordacwksPathString);
    dllBase = LoadLibrary(mscordacwksFileName->Buffer);
    PhDereferenceObject(mscordacwksFileName);

    return dllBase;
}

HRESULT CreateXCLRDataProcess(
    __in HANDLE ProcessId,
    __in ICLRDataTarget *Target,
    __out struct IXCLRDataProcess **DataProcess
    )
{
    PPH_IS_DOT_NET_CONTEXT isDotNetContext;
    ULONG flags;
    BOOLEAN clrV4;
    HMODULE dllBase;
    HRESULT (__stdcall *clrDataCreateInstance)(REFIID, ICLRDataTarget *, void **);

    clrV4 = FALSE;

    if (NT_SUCCESS(PhCreateIsDotNetContext(&isDotNetContext, NULL, 0)))
    {
        if (PhGetProcessIsDotNetFromContext(isDotNetContext, ProcessId, &flags))
        {
            if (flags & PH_IS_DOT_NET_VERSION_4)
                clrV4 = TRUE;
        }

        PhFreeIsDotNetContext(isDotNetContext);
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

    clrDataCreateInstance = (PVOID)GetProcAddress(dllBase, "CLRDataCreateInstance");

    if (!clrDataCreateInstance)
        return E_FAIL;

    return clrDataCreateInstance(&IID_IXCLRDataProcess, Target, DataProcess);
}

ICLRDataTarget *DnCLRDataTarget_Create(
    __in HANDLE ProcessId
    )
{
    DnCLRDataTarget *dataTarget;
    HANDLE processHandle;
    BOOLEAN isWow64;

    if (!NT_SUCCESS(PhOpenProcess(&processHandle, ProcessQueryAccess | PROCESS_VM_READ, ProcessId)))
        return NULL;

#ifdef _M_X64
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
    __in ICLRDataTarget *This,
    __in REFIID Riid,
    __out PVOID *Object
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
    __in ICLRDataTarget *This
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

    this->RefCount++;

    return this->RefCount;
}

ULONG STDMETHODCALLTYPE DnCLRDataTarget_Release(
    __in ICLRDataTarget *This
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
    __in ICLRDataTarget *This,
    __out ULONG32 *machineType
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

#ifdef _M_X64
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
    __in ICLRDataTarget *This,
    __out ULONG32 *pointerSize
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

#ifdef _M_X64
    if (!this->IsWow64)
#endif
        *pointerSize = sizeof(PVOID);
#ifdef _M_X64
    else
        *pointerSize = sizeof(ULONG);
#endif

    return S_OK;
}

BOOLEAN NTAPI PhpGetImageBaseCallback(
    __in PLDR_DATA_TABLE_ENTRY Module,
    __in_opt PVOID Context
    )
{
    PPHP_GET_IMAGE_BASE_CONTEXT context = Context;

    if (RtlEqualUnicodeString(&Module->FullDllName, &context->ImagePath.us, TRUE) ||
        RtlEqualUnicodeString(&Module->BaseDllName, &context->ImagePath.us, TRUE))
    {
        context->BaseAddress = Module->DllBase;
        return FALSE;
    }

    return TRUE;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetImageBase(
    __in ICLRDataTarget *This,
    __in LPCWSTR imagePath,
    __out CLRDATA_ADDRESS *baseAddress
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;
    PHP_GET_IMAGE_BASE_CONTEXT context;

    PhInitializeStringRef(&context.ImagePath, (PWSTR)imagePath);
    context.BaseAddress = NULL;
    PhEnumProcessModules(this->ProcessHandle, PhpGetImageBaseCallback, &context);

#ifdef _M_X64
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
    __in ICLRDataTarget *This,
    __in CLRDATA_ADDRESS address,
    __out BYTE *buffer,
    __in ULONG32 bytesRequested,
    __out ULONG32 *bytesRead
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
        return HRESULT_FROM_WIN32(RtlNtStatusToDosError(status));
    }
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_WriteVirtual(
    __in ICLRDataTarget *This,
    __in CLRDATA_ADDRESS address,
    __in BYTE *buffer,
    __in ULONG32 bytesRequested,
    __out ULONG32 *bytesWritten
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetTLSValue(
    __in ICLRDataTarget *This,
    __in ULONG32 threadID,
    __in ULONG32 index,
    __out CLRDATA_ADDRESS *value
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetTLSValue(
    __in ICLRDataTarget *This,
    __in ULONG32 threadID,
    __in ULONG32 index,
    __in CLRDATA_ADDRESS value
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetCurrentThreadID(
    __in ICLRDataTarget *This,
    __out ULONG32 *threadID
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetThreadContext(
    __in ICLRDataTarget *This,
    __in ULONG32 threadID,
    __in ULONG32 contextFlags,
    __in ULONG32 contextSize,
    __out BYTE *context
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetThreadContext(
    __in ICLRDataTarget *This,
    __in ULONG32 threadID,
    __in ULONG32 contextSize,
    __in BYTE *context
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_Request(
    __in ICLRDataTarget *This,
    __in ULONG32 reqCode,
    __in ULONG32 inBufferSize,
    __in BYTE *inBuffer,
    __in ULONG32 outBufferSize,
    __out BYTE *outBuffer
    )
{
    return E_NOTIMPL;
}
