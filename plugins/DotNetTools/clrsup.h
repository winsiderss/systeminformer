/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2013
 *     dmex    2016-2023
 *
 */

#ifndef CLRSUP_H
#define CLRSUP_H

#include "clr/clrdata.h"
#include "clr/sospriv.h"
#include "clr/dacprivate.h"

typedef struct _CLR_PROCESS_SUPPORT
{
    ICLRDataTarget* DataTarget;
    IXCLRDataProcess *DataProcess;
    PVOID DacDllBase;
} CLR_PROCESS_SUPPORT, *PCLR_PROCESS_SUPPORT;

PCLR_PROCESS_SUPPORT CreateClrProcessSupport(
    _In_ HANDLE ProcessId
    );

VOID FreeClrProcessSupport(
    _In_ PCLR_PROCESS_SUPPORT Support
    );

_Success_(return != NULL)
PPH_STRING GetRuntimeNameByAddressClrProcess(
    _In_ PCLR_PROCESS_SUPPORT Support,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 Displacement
    );

PPH_STRING DnGetFileNameByAddressClrProcess(
    _In_ PCLR_PROCESS_SUPPORT Support,
    _In_ ULONG64 Address
    );

PPH_STRING GetNameXClrDataAppDomain(
    _In_ PVOID AppDomain
    );

PPH_STRING DnGetClrThreadAppDomain(
    _In_ PCLR_PROCESS_SUPPORT Support,
    _In_ HANDLE ThreadId
    );

PPH_LIST DnGetClrAppDomainAssemblyList(
    _In_ PCLR_PROCESS_SUPPORT Support
    );

HRESULT CreateXCLRDataProcess(
    _In_ HANDLE ProcessId,
    _In_ ICLRDataTarget *Target,
    _Out_ struct IXCLRDataProcess **DataProcess,
    _Out_ PVOID *BaseAddress
    );

typedef struct _DN_DOTNET_ASSEMBLY_ENTRY
{
    HRESULT Status;
    CLRDataModuleFlag ModuleFlag;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG IsDynamicAssembly : 1;
            ULONG IsReflection : 1;
            ULONG IsPEFile : 1;
            ULONG Spare : 29;
        };
    };

    PVOID ModuleAddress;
    PVOID BaseAddress;

    ULONG64 AssemblyID;
    ULONG64 ModuleID;
    ULONG64 ModuleIndex;

    PPH_STRING AssemblyName;
    PPH_STRING DisplayName;
    PPH_STRING ModuleName;
    PPH_STRING NativeFileName;

    GUID Mvid;
} DN_DOTNET_ASSEMBLY_ENTRY, *PDN_DOTNET_ASSEMBLY_ENTRY;

typedef enum _DN_CLR_APPDOMAIN_TYPE
{
    DN_CLR_APPDOMAIN_TYPE_DYNAMIC,
    DN_CLR_APPDOMAIN_TYPE_SHARED,
    DN_CLR_APPDOMAIN_TYPE_SYSTEM
} DN_CLR_APPDOMAIN_TYPE;

typedef struct _DN_PROCESS_APPDOMAIN_ENTRY
{
    HRESULT Status;
    ULONG32 AppDomainType;
    ULONG32 AppDomainNumber;
    ULONG64 AppDomainID;
    PPH_STRING AppDomainName;
    PPH_LIST AssemblyList;
} DN_PROCESS_APPDOMAIN_ENTRY, *PDN_PROCESS_APPDOMAIN_ENTRY;

HRESULT DnGetProcessDotNetAppDomainList(
    _In_ ICLRDataTarget* ClrDataTarget,
    _In_ ISOSDacInterface* DacInterface,
    _Out_ PPH_LIST* ProcessAppdomainList
    );

VOID DnDestroyProcessDotNetAppDomainList(
    _In_ PPH_LIST ProcessAppdomainList
    );

PPH_BYTES DnProcessAppDomainListSerialize(
    _In_ PPH_LIST ProcessAppdomainList
    );

PPH_LIST DnProcessAppDomainListDeserialize(
    _In_ PPH_BYTES String
    );

typedef struct _DnCLRDataTarget
{
    ICLRDataTargetVtbl* VTable;
    ULONG RefCount;
    HANDLE ProcessId;
    HANDLE ProcessHandle;
    BOOLEAN IsWow64;
    BOOLEAN SelfContained;
    PVOID DataTargetDllBase;
    PPH_STRING DaccorePath;
} DnCLRDataTarget;

#define MAX_LONGPATH 1024

ICLRDataTarget* DnCLRDataTarget_Create(
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

// CLR guids

EXTERN_GUID(CLR_ID_ONECORE_CLR, 0xb1ee760d, 0x6c4a, 0x4533, 0xba, 0x41, 0x6f, 0x4f, 0x66, 0x1f, 0xab, 0xaf);
DEFINE_GUID(IID_ICLRDataTarget, 0x3e11ccee, 0xd08b, 0x43e5, 0xaf, 0x01, 0x32, 0x71, 0x7a, 0x64, 0xda, 0x03);
DEFINE_GUID(IID_ICLRDataTarget2, 0x6d05fae3, 0x189c, 0x4630, 0xa6, 0xdc, 0x1c, 0x25, 0x1e, 0x1c, 0x01, 0xab);
DEFINE_GUID(IID_ICLRDataTarget3, 0xa5664f95, 0x0af4, 0x4a1b, 0x96, 0x0e, 0x2f, 0x33, 0x46, 0xb4, 0x21, 0x4c);
DEFINE_GUID(IID_ICLRRuntimeLocator, 0xb760bf44, 0x9377, 0x4597, 0x8b, 0xe7, 0x58, 0x08, 0x3b, 0xdc, 0x51, 0x46);
DEFINE_GUID(IID_ICLRMetadataLocator, 0xaa8fa804, 0xbc05, 0x4642, 0xb2, 0xc5, 0xc3, 0x53, 0xed, 0x22, 0xfc, 0x63);
DEFINE_GUID(IID_ICLRDataEnumMemoryRegionsCallback, 0xbcdd6908, 0xba2d, 0x4ec5, 0x96, 0xcf, 0xdf, 0x4d, 0x5c, 0xdc, 0xb4, 0xa4);
DEFINE_GUID(IID_ICLRDataEnumMemoryRegionsCallback2, 0x3721a26f, 0x8b91, 0x4d98, 0xa3, 0x88, 0xdb, 0x17, 0xb3, 0x56, 0xfa, 0xdb);
DEFINE_GUID(IID_ICLRDataEnumMemoryRegions, 0x471c35b4, 0x7c2f, 0x4ef0, 0xa9, 0x45, 0x00, 0xf8, 0xc3, 0x80, 0x56, 0xf1);
DEFINE_GUID(IID_ISOSEnum, 0x286ca186, 0xe763, 0x4f61, 0x97, 0x60, 0x48, 0x7d, 0x43, 0xae, 0x43, 0x41);
DEFINE_GUID(IID_ISOSHandleEnum, 0x3e269830, 0x4a2b, 0x4301, 0x8e, 0xe2, 0xd6, 0x80, 0x5b, 0x29, 0xb2, 0xfa);
DEFINE_GUID(IID_ISOSStackRefErrorEnum, 0x774f4e1b, 0xfb7b, 0x491b, 0x97, 0x6d, 0xa8, 0x13, 0x0f, 0xe3, 0x55, 0xe9);
DEFINE_GUID(IID_ISOSStackRefEnum, 0x8fa642bd, 0x9f10, 0x4799, 0x9a, 0xa3, 0x51, 0x2a, 0xe7, 0x8c, 0x77, 0xee);
DEFINE_GUID(IID_ISOSDacInterface, 0x436f00f2, 0xb42a, 0x4b9f, 0x87, 0x0c, 0xe7, 0x3d, 0xb6, 0x6a, 0xe9, 0x30);
DEFINE_GUID(IID_ISOSDacInterface2, 0xa16026ec, 0x96f4, 0x40ba, 0x87, 0xfb, 0x55, 0x75, 0x98, 0x6f, 0xb7, 0xaf);
DEFINE_GUID(IID_ISOSDacInterface3, 0xb08c5cdc, 0xfd8a, 0x49c5, 0xab, 0x38, 0x5f, 0xee, 0xf3, 0x52, 0x35, 0xb4);
DEFINE_GUID(IID_ISOSDacInterface4, 0x74b9d34c, 0xa612, 0x4b07, 0x93, 0xdd, 0x54, 0x62, 0x17, 0x8f, 0xce, 0x11);
DEFINE_GUID(IID_ISOSDacInterface5, 0x127d6abe, 0x6c86, 0x4e48, 0x8e, 0x7b, 0x22, 0x07, 0x81, 0xc5, 0x81, 0x01);
DEFINE_GUID(IID_ISOSDacInterface6, 0x11206399, 0x4B66, 0x4EDB, 0x98, 0xEA, 0x85, 0x65, 0x4E, 0x59, 0xAD, 0x45);
DEFINE_GUID(IID_ISOSDacInterface7, 0xc1020dde, 0xfe98, 0x4536, 0xa5, 0x3b, 0xf3, 0x5a, 0x74, 0xc3, 0x27, 0xeb);
DEFINE_GUID(IID_ISOSDacInterface8, 0xc12f35a9, 0xe55c, 0x4520, 0xa8, 0x94, 0xb3, 0xdc, 0x51, 0x65, 0xdf, 0xce);
DEFINE_GUID(IID_ISOSDacInterface9, 0x4eca42d8, 0x7e7b, 0x4c8a, 0xa1, 0x16, 0x7b, 0xfb, 0xf6, 0x92, 0x92, 0x67);
DEFINE_GUID(IID_ISOSDacInterface10, 0x90b8fcc3, 0x7251, 0x4b0a, 0xae, 0x3d, 0x5c, 0x13, 0xa6, 0x7e, 0xc9, 0xaa);
DEFINE_GUID(IID_ISOSDacInterface11, 0x96ba1db9, 0x14cd, 0x4492, 0x80, 0x65, 0x1c, 0xaa, 0xec, 0xf6, 0xe5, 0xcf);
DEFINE_GUID(IID_IXCLRDataTarget3, 0x59d9b5e1, 0x4a6f, 0x4531, 0x84, 0xc3, 0x51, 0xd1, 0x2d, 0xa2, 0x2f, 0xd4);
DEFINE_GUID(IID_IXCLRLibrarySupport, 0xe5f3039d, 0x2c0c, 0x4230, 0xa6, 0x9e, 0x12, 0xaf, 0x1c, 0x3e, 0x56, 0x3c);
DEFINE_GUID(IID_IXCLRDisassemblySupport, 0x1f0f7134, 0xd3f3, 0x47de, 0x8e, 0x9b, 0xc2, 0xfd, 0x35, 0x8a, 0x29, 0x36);
DEFINE_GUID(IID_IXCLRDataDisplay, 0xA3C1704A, 0x4559, 0x4a67, 0x8D, 0x28, 0xE8, 0xF4, 0xFE, 0x3B, 0x3F, 0x62);
DEFINE_GUID(IID_IXCLRDataProcess, 0x5c552ab6, 0xfc09, 0x4cb3, 0x8e, 0x36, 0x22, 0xfa, 0x03, 0xc7, 0x98, 0xb7);
DEFINE_GUID(IID_IXCLRDataProcess2, 0x5c552ab6, 0xfc09, 0x4cb3, 0x8e, 0x36, 0x22, 0xfa, 0x03, 0xc7, 0x98, 0xb8);
DEFINE_GUID(IID_IXCLRDataAppDomain, 0x7ca04601, 0xc702, 0x4670, 0xa6, 0x3c, 0xfa, 0x44, 0xf7, 0xda, 0x7b, 0xd5);
DEFINE_GUID(IID_IXCLRDataAssembly, 0x2fa17588, 0x43c2, 0x46ab, 0x9b, 0x51, 0xc8, 0xf0, 0x1e, 0x39, 0xc9, 0xac);
DEFINE_GUID(IID_IXCLRDataModule, 0x88e32849, 0x0a0a, 0x4cb0, 0x90, 0x22, 0x7c, 0xd2, 0xe9, 0xe1, 0x39, 0xe2);
DEFINE_GUID(IID_IXCLRDataModule2, 0x34625881, 0x7eb3, 0x4524, 0x81, 0x7b, 0x8d, 0xb9, 0xd0, 0x64, 0xc7, 0x60);
DEFINE_GUID(IID_IXCLRDataTypeDefinition, 0x4675666c, 0xc275, 0x45b8, 0x9f, 0x6c, 0xab, 0x16, 0x5d, 0x5c, 0x1e, 0x09);
DEFINE_GUID(IID_IXCLRDataTypeInstance, 0x4d078d91, 0x9cb3, 0x4b0d, 0x97, 0xac, 0x28, 0xc8, 0xa5, 0xa8, 0x25, 0x97);
DEFINE_GUID(IID_IXCLRDataMethodDefinition, 0xaaf60008, 0xfb2c, 0x420b, 0x8f, 0xb1, 0x42, 0xd2, 0x44, 0xa5, 0x4a, 0x97);
DEFINE_GUID(IID_IXCLRDataMethodInstance, 0xecd73800, 0x22ca, 0x4b0d, 0xab, 0x55, 0xe9, 0xba, 0x7e, 0x63, 0x18, 0xa5);
DEFINE_GUID(IID_IXCLRDataTask, 0xa5b0beea, 0xec62, 0x4618, 0x80, 0x12, 0xa2, 0x4f, 0xfc, 0x23, 0x93, 0x4c);
DEFINE_GUID(IID_IXCLRDataStackWalk, 0xe59d8d22, 0xada7, 0x49a2, 0x89, 0xb5, 0xa4, 0x15, 0xaf, 0xcf, 0xc9, 0x5f);
DEFINE_GUID(IID_IXCLRDataFrame, 0x271498c2, 0x4085, 0x4766, 0xbc, 0x3a, 0x7f, 0x8e, 0xd1, 0x88, 0xa1, 0x73);
DEFINE_GUID(IID_IXCLRDataFrame2, 0x1c4d9a4b, 0x702d, 0x4cf6, 0xb2, 0x90, 0x1d, 0xb6, 0xf4, 0x30, 0x50, 0xd0);
DEFINE_GUID(IID_IXCLRDataExceptionState, 0x75da9e4c, 0xbd33, 0x43c8, 0x8f, 0x5c, 0x96, 0xe8, 0xa5, 0x24, 0x1f, 0x57);
DEFINE_GUID(IID_IXCLRDataValue, 0x96ec93c7, 0x1000, 0x4e93, 0x89, 0x91, 0x98, 0xd8, 0x76, 0x6e, 0x66, 0x66);
DEFINE_GUID(IID_IXCLRDataExceptionNotification, 0x2d95a079, 0x42a1, 0x4837, 0x81, 0x8f, 0x0b, 0x97, 0xd7, 0x04, 0x8e, 0x0e);
DEFINE_GUID(IID_IXCLRDataExceptionNotification2, 0x31201a94, 0x4337, 0x49b7, 0xae, 0xf7, 0x0c, 0x75, 0x50, 0x54, 0x09, 0x1f);
DEFINE_GUID(IID_IXCLRDataExceptionNotification3, 0x31201a94, 0x4337, 0x49b7, 0xae, 0xf7, 0x0c, 0x75, 0x50, 0x54, 0x09, 0x20);
DEFINE_GUID(IID_IXCLRDataExceptionNotification4, 0xc25e926e, 0x5f09, 0x4aa2, 0xbb, 0xad, 0xb7, 0xfc, 0x7f, 0x10, 0xcf, 0xd7);
DEFINE_GUID(IID_IXCLRDataExceptionNotification5, 0xe77a39ea, 0x3548, 0x44d9, 0xb1, 0x71, 0x85, 0x69, 0xed, 0x1a, 0x94, 0x23);

#endif CLRSUP_H
