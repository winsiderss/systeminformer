/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#ifndef _PH_MAPLDR_H
#define _PH_MAPLDR_H

EXTERN_C_START

PHLIBAPI
PVOID
NTAPI
PhLoadLibrary(
    _In_ PCWSTR FileName
    );

PHLIBAPI
BOOLEAN
NTAPI
PhFreeLibrary(
    _In_ PVOID BaseAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadLibraryAsImageResource(
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName,
    _Out_opt_ PVOID *BaseAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFreeLibraryAsImageResource(
    _In_ PVOID BaseAddress
    );

PHLIBAPI
PVOID
NTAPI
PhGetDllHandle(
    _In_ PCWSTR DllName
    );

PHLIBAPI
PVOID
NTAPI
PhGetModuleProcAddress(
    _In_ PCWSTR ModuleName,
    _In_opt_ PCSTR ProcedureName
    );

PHLIBAPI
PVOID
NTAPI
PhGetProcedureAddress(
    _In_ PVOID DllHandle,
    _In_opt_ PCSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcedureAddressRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PCPH_STRINGREF FileName,
    _In_ PCSTR ProcedureName,
    _Out_ PVOID *ProcedureAddress,
    _Out_opt_ PVOID *DllBase
    );

FORCEINLINE
NTSTATUS
NTAPI
PhGetProcedureAddressRemoteZ(
    _In_ HANDLE ProcessHandle,
    _In_ PCWSTR FileName,
    _In_ PCSTR ProcedureName,
    _Out_ PVOID *ProcedureAddress,
    _Out_opt_ PVOID *DllBase
    )
{
    PH_STRINGREF fileName;

    PhInitializeStringRef(&fileName, FileName);

    return PhGetProcedureAddressRemote(
        ProcessHandle,
        &fileName,
        ProcedureName,
        ProcedureAddress,
        DllBase
        );
}

PHLIBAPI
NTSTATUS
NTAPI
PhLoadResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _Out_opt_ ULONG *ResourceLength,
    _Out_opt_ PVOID *ResourceBuffer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadResourceCopy(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _Out_opt_ ULONG *ResourceLength,
    _Out_opt_ PVOID *ResourceBuffer
    );

PHLIBAPI
PPH_STRING
NTAPI
PhLoadIndirectString(
    _In_ PCPH_STRINGREF SourceString
    );

_Success_(return != NULL)
PHLIBAPI
PPH_STRING
NTAPI
PhGetDllFileName(
    _In_ PVOID DllBase,
    _Out_opt_ PULONG IndexOfFileName
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetLoaderEntryData(
    _In_ PCPH_STRINGREF BaseDllName,
    _Out_opt_ PVOID* DllBase,
    _Out_opt_ ULONG* SizeOfImage,
    _Out_opt_ PPH_STRING* FullName
    );

FORCEINLINE
BOOLEAN
NTAPI
PhGetLoaderEntryDataZ(
    _In_ PCWSTR BaseDllName,
    _Out_opt_ PVOID* DllBase,
    _Out_opt_ ULONG* SizeOfImage,
    _Out_opt_ PPH_STRING* FullName
    )
{
    PH_STRINGREF baseDllName;

    PhInitializeStringRef(&baseDllName, BaseDllName);

    return PhGetLoaderEntryData(
        &baseDllName,
        DllBase,
        SizeOfImage,
        FullName
        );
}

PHLIBAPI
PVOID
NTAPI
PhGetLoaderEntryAddressDllBase(
    _In_ PVOID PcAddress
    );

PHLIBAPI
PVOID
NTAPI
PhGetLoaderEntryDllBase(
    _In_opt_ PCPH_STRINGREF FullDllName,
    _In_opt_ PCPH_STRINGREF BaseDllName
    );

FORCEINLINE
PVOID
NTAPI
PhGetLoaderEntryDllBaseZ(
    _In_ PCWSTR DllName
    )
{
    PH_STRINGREF baseDllName;

    PhInitializeStringRef(&baseDllName, DllName);

    return PhGetLoaderEntryDllBase(NULL, &baseDllName);
}

PHLIBAPI
NTSTATUS
NTAPI
PhCaptureSystemDllInitBlock(
    _In_ PVOID Source,
    _Out_ PPS_SYSTEM_DLL_INIT_BLOCK Destination
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSystemDllInitBlock(
    _Out_ PPS_SYSTEM_DLL_INIT_BLOCK SystemDllInitBlock
    );

PHLIBAPI
PVOID
NTAPI
PhGetDllBaseProcedureAddress(
    _In_ PVOID DllBase,
    _In_opt_ PCSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber
    );

PHLIBAPI
PVOID
NTAPI
PhGetDllBaseProcedureAddressWithHint(
    _In_ PVOID BaseAddress,
    _In_ PCSTR ProcedureName,
    _In_ USHORT ProcedureHint
    );

PHLIBAPI
PVOID
NTAPI
PhGetDllProcedureAddress(
    _In_ PCWSTR DllName,
    _In_opt_ PCSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetLoaderEntryImageNtHeaders(
    _In_ PVOID BaseAddress,
    _Out_ PIMAGE_NT_HEADERS *ImageNtHeaders
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetLoaderEntryImageDirectory(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _In_ ULONG ImageDirectoryIndex,
    _Out_ PIMAGE_DATA_DIRECTORY *ImageDataDirectoryEntry,
    _Out_ PVOID *ImageDirectoryEntry,
    _Out_opt_ SIZE_T *ImageDirectoryLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetLoaderEntryImageVaToSection(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _In_ PVOID ImageDirectoryAddress,
    _Out_ PVOID *ImageSectionAddress,
    _Out_ SIZE_T *ImageSectionLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoaderEntryImageRvaToSection(
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _In_ ULONG Rva,
    _Out_ PIMAGE_SECTION_HEADER *ImageSection,
    _Out_ SIZE_T *ImageSectionLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoaderEntryImageRvaToVa(
    _In_ PVOID BaseAddress,
    _In_ ULONG Rva,
    _Out_ PVOID *Va
    );

PHLIBAPI
BOOLEAN
NTAPI
PhLoaderEntryImageExportSupressionPresent(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeader
    );

PHLIBAPI
VOID
NTAPI
PhLoaderEntryGrantSuppressedCall(
    _In_ PVOID ExportAddress
    );

PHLIBAPI
PVOID
NTAPI
PhGetLoaderEntryImageExportFunction(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_DATA_DIRECTORY DataDirectory,
    _In_ PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    _In_opt_ PCSTR ExportName,
    _In_opt_ USHORT ExportOrdinal
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetExportNameFromOrdinal(
    _In_ PVOID DllBase,
    _In_opt_ USHORT ProcedureNumber
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoaderEntryDetourImportProcedure(
    _In_ PVOID BaseAddress,
    _In_ PCSTR ImportName,
    _In_ PCSTR ProcedureName,
    _In_ PVOID FunctionAddress,
    _Out_opt_ PVOID* OriginalAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoaderEntryLoadDll(
    _In_ PCPH_STRINGREF FileName,
    _In_opt_ HANDLE RootDirectory,
    _Out_ PVOID* BaseAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoaderEntryLoadAllImportsForDll(
    _In_ PVOID BaseAddress,
    _In_ PCSTR ImportDllName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadAllImportsForDll(
    _In_ PCWSTR TargetDllName,
    _In_ PCSTR ImportDllName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadPluginImage(
    _In_ PCPH_STRINGREF FileName,
    _In_opt_ HANDLE RootDirectory,
    _Out_opt_ PVOID *BaseAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileBinaryTypeWin32(
    _In_ PCWSTR FileName,
    _Out_ PULONG BinaryType
    );

EXTERN_C_END

#endif
