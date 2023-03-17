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
PLDR_DATA_TABLE_ENTRY
NTAPI
PhFindLoaderEntry(
    _In_opt_ PVOID DllBase,
    _In_opt_ PPH_STRINGREF FullDllName,
    _In_opt_ PPH_STRINGREF BaseDllName
    );

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
    _In_ PPH_STRINGREF FileName,
    _Out_opt_ PVOID *BaseAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadLibraryAsImageResourceWin32(
    _In_ PPH_STRINGREF FileName,
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
    _In_ PWSTR DllName
    );

PHLIBAPI
PVOID
NTAPI
PhGetModuleProcAddress(
    _In_ PWSTR ModuleName,
    _In_opt_ PSTR ProcedureName
    );

PHLIBAPI
PVOID
NTAPI
PhGetProcedureAddress(
    _In_ PVOID DllHandle,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ ULONG ProcedureNumber
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcedureAddressRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF FileName,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber,
    _Out_ PVOID *ProcedureAddress,
    _Out_opt_ PVOID *DllBase
    );

FORCEINLINE
NTSTATUS
NTAPI
PhGetProcedureAddressRemoteZ(
    _In_ HANDLE ProcessHandle,
    _In_ PWSTR FileName,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber,
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
        ProcedureNumber,
        ProcedureAddress,
        DllBase
        );
}

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhLoadResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _Out_opt_ ULONG *ResourceLength,
    _Out_opt_ PVOID *ResourceBuffer
    );

_Success_(return)
PHLIBAPI
BOOLEAN
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
    _In_ PPH_STRINGREF SourceString
    );

PHLIBAPI
_Success_(return != NULL)
PPH_STRING
NTAPI
PhGetDllFileName(
    _In_ PVOID DllBase,
    _Out_opt_ PULONG IndexOfFileName
    );

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
    _In_opt_ PPH_STRINGREF FullDllName,
    _In_opt_ PPH_STRINGREF BaseDllName
    );

FORCEINLINE
PVOID
NTAPI
PhGetLoaderEntryDllBaseZ(
    _In_ PWSTR DllName
    )
{
    PH_STRINGREF baseDllName;

    PhInitializeStringRef(&baseDllName, DllName);

    return PhGetLoaderEntryDllBase(NULL, &baseDllName);
}

PHLIBAPI
PVOID
NTAPI
PhGetDllBaseProcedureAddress(
    _In_ PVOID DllBase,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber
    );

PHLIBAPI
PVOID
NTAPI
PhGetDllBaseProcedureAddressWithHint(
    _In_ PVOID BaseAddress,
    _In_ PSTR ProcedureName,
    _In_ USHORT ProcedureHint
    );

PHLIBAPI
PVOID
NTAPI
PhGetDllProcedureAddress(
    _In_ PWSTR DllName,
    _In_opt_ PSTR ProcedureName,
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
PVOID
NTAPI
PhGetLoaderEntryImageExportFunction(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_DATA_DIRECTORY DataDirectory,
    _In_ PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    _In_opt_ PSTR ExportName,
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
    _In_ PSTR ImportName,
    _In_ PSTR ProcedureName,
    _In_ PVOID FunctionAddress,
    _Out_opt_ PVOID* OriginalAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoaderEntryLoadDll(
    _In_ PPH_STRINGREF FileName,
    _Out_ PVOID* BaseAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoaderEntryLoadAllImportsForDll(
    _In_ PVOID BaseAddress,
    _In_ PSTR ImportDllName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadAllImportsForDll(
    _In_ PWSTR TargetDllName,
    _In_ PSTR ImportDllName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadPluginImage(
    _In_ PPH_STRINGREF FileName,
    _Out_opt_ PVOID *BaseAddress
    );

EXTERN_C_END

#endif
