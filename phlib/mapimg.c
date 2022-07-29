/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2017-2021
 *
 */

#include <ph.h>
#include <mapimg.h>

VOID PhpMappedImageProbe(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID Address,
    _In_ SIZE_T Length
    );

ULONG PhpLookupMappedImageExportName(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ PSTR Name
    );

NTSTATUS PhInitializeMappedImage(
    _Out_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID ViewBase,
    _In_ SIZE_T Size
    )
{
    PIMAGE_DOS_HEADER dosHeader;
    ULONG ntHeadersOffset;

    MappedImage->ViewBase = ViewBase;
    MappedImage->Size = Size;

    dosHeader = (PIMAGE_DOS_HEADER)ViewBase;

    __try
    {
        PhpMappedImageProbe(MappedImage, dosHeader, sizeof(IMAGE_DOS_HEADER));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    // Check the initial MZ.

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return STATUS_INVALID_IMAGE_NOT_MZ;

    // Get a pointer to the NT headers and probe it.

    ntHeadersOffset = (ULONG)dosHeader->e_lfanew;

    if (ntHeadersOffset == 0)
        return STATUS_INVALID_IMAGE_FORMAT;
    if (ntHeadersOffset >= 0x10000000 || ntHeadersOffset >= Size)
        return STATUS_INVALID_IMAGE_FORMAT;

    MappedImage->NtHeaders = (PIMAGE_NT_HEADERS)PTR_ADD_OFFSET(ViewBase, ntHeadersOffset);

    __try
    {
        PhpMappedImageProbe(
            MappedImage,
            MappedImage->NtHeaders,
            UFIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader)
            );
        PhpMappedImageProbe(
            MappedImage,
            MappedImage->NtHeaders,
            (SIZE_T)UFIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) +
            MappedImage->NtHeaders->FileHeader.SizeOfOptionalHeader +
            MappedImage->NtHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER)
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    // Check the signature and verify the magic.

    if (MappedImage->NtHeaders->Signature != IMAGE_NT_SIGNATURE)
        return STATUS_INVALID_IMAGE_FORMAT;

    MappedImage->Magic = MappedImage->NtHeaders->OptionalHeader.Magic;

    if (
        MappedImage->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
        MappedImage->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC
        )
        return STATUS_INVALID_IMAGE_FORMAT;

    // Get a pointer to the first section.

    MappedImage->NumberOfSections = MappedImage->NtHeaders->FileHeader.NumberOfSections;
    MappedImage->Sections = IMAGE_FIRST_SECTION(MappedImage->NtHeaders);

    return STATUS_SUCCESS;
}

NTSTATUS PhLoadMappedImage(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PVOID viewBase;
    SIZE_T size;

    status = PhMapViewOfEntireFile(
        FileName,
        FileHandle,
        &viewBase,
        &size
        );

    if (NT_SUCCESS(status))
    {
        status = PhInitializeMappedImage(
            MappedImage,
            viewBase,
            size
            );

        if (!NT_SUCCESS(status))
        {
            PhUnloadMappedImage(MappedImage);
        }
    }

    return status;
}

NTSTATUS PhLoadMappedImageEx(
    _In_opt_ PPH_STRINGREF FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PVOID viewBase;
    SIZE_T size;

    status = PhMapViewOfEntireFileEx(
        FileName,
        FileHandle,
        &viewBase,
        &size
        );

    if (NT_SUCCESS(status))
    {
        MappedImage->Signature = *(PUSHORT)viewBase;
        MappedImage->ViewBase = viewBase;
        MappedImage->Size = size;

        switch (MappedImage->Signature)
        {
        case IMAGE_DOS_SIGNATURE:
            {
                status = PhInitializeMappedImage(
                    MappedImage,
                    viewBase,
                    size
                    );
            }
            break;
        case IMAGE_ELF_SIGNATURE:
            {
                status = PhInitializeMappedWslImage(
                    MappedImage,
                    viewBase,
                    size
                    );
            }
            break;
        default:
            status = STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT;
            break;
        }

        if (!NT_SUCCESS(status))
        {
            PhUnloadMappedImage(MappedImage);
        }
    }

    return status;
}

NTSTATUS PhUnloadMappedImage(
    _Inout_ PPH_MAPPED_IMAGE MappedImage
    )
{
    return NtUnmapViewOfSection(
        NtCurrentProcess(),
        MappedImage->ViewBase
        );
}

NTSTATUS PhMapViewOfEntireFile(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PVOID *ViewBase,
    _Out_ PSIZE_T Size
    )
{
    NTSTATUS status;
    BOOLEAN openedFile = FALSE;
    LARGE_INTEGER size;
    HANDLE sectionHandle = NULL;
    SIZE_T viewSize;
    PVOID viewBase;

    if (!FileName && !FileHandle)
        return STATUS_INVALID_PARAMETER_MIX;

    // Open the file if we weren't supplied a file handle.
    if (!FileHandle)
    {
        status = PhCreateFileWin32(
            &FileHandle,
            FileName,
            FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (!NT_SUCCESS(status))
            return status;

        openedFile = TRUE;
    }

    // Get the file size and create the section.

    status = PhGetFileSize(FileHandle, &size);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtCreateSection(
        &sectionHandle,
        SECTION_QUERY | SECTION_MAP_READ,
        NULL,
        &size,
        PAGE_READONLY,
        SEC_COMMIT,
        FileHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Map the section.

    viewSize = (SIZE_T)size.QuadPart;
    viewBase = NULL;

    status = NtMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &viewBase,
        0,
        0,
        NULL,
        &viewSize,
        ViewShare,
        0,
        PAGE_READONLY
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    *ViewBase = viewBase;
    *Size = (SIZE_T)size.QuadPart;

CleanupExit:
    if (sectionHandle)
        NtClose(sectionHandle);
    if (openedFile)
        NtClose(FileHandle);

    return status;
}

NTSTATUS PhMapViewOfEntireFileEx(
    _In_opt_ PPH_STRINGREF FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PVOID *ViewBase,
    _Out_ PSIZE_T Size
    )
{
    NTSTATUS status;
    BOOLEAN openedFile = FALSE;
    LARGE_INTEGER size;
    HANDLE sectionHandle = NULL;
    SIZE_T viewSize;
    PVOID viewBase;

    if (!FileName && !FileHandle)
        return STATUS_INVALID_PARAMETER_MIX;

    // Open the file if we weren't supplied a file handle.
    if (!FileHandle)
    {
        status = PhCreateFile(
            &FileHandle,
            FileName,
            FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (!NT_SUCCESS(status))
            return status;

        openedFile = TRUE;
    }

    // Get the file size and create the section.

    status = PhGetFileSize(FileHandle, &size);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtCreateSection(
        &sectionHandle,
        SECTION_QUERY | SECTION_MAP_READ,
        NULL,
        &size,
        PAGE_READONLY,
        SEC_COMMIT,
        FileHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Map the section.

    viewSize = (SIZE_T)size.QuadPart;
    viewBase = NULL;

    status = NtMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &viewBase,
        0,
        0,
        NULL,
        &viewSize,
        ViewShare,
        0,
        PAGE_READONLY
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    *ViewBase = viewBase;
    *Size = (SIZE_T)size.QuadPart;

CleanupExit:
    if (sectionHandle)
        NtClose(sectionHandle);
    if (openedFile)
        NtClose(FileHandle);

    return status;
}

VOID PhpMappedImageProbe(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID Address,
    _In_ SIZE_T Length
    )
{
    PhProbeAddress(Address, Length, MappedImage->ViewBase, MappedImage->Size, 1);
}

PIMAGE_SECTION_HEADER PhMappedImageRvaToSection(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva
    )
{
    ULONG i;

    for (i = 0; i < MappedImage->NumberOfSections; i++)
    {
        if (
            (Rva >= MappedImage->Sections[i].VirtualAddress) &&
            (Rva < MappedImage->Sections[i].VirtualAddress + MappedImage->Sections[i].SizeOfRawData)
            )
        {
            return &MappedImage->Sections[i];
        }
    }

    return NULL;
}

_Must_inspect_result_
_Ret_maybenull_
PVOID PhMappedImageRvaToVa(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva,
    _Out_opt_ PIMAGE_SECTION_HEADER *Section
    )
{
    PIMAGE_SECTION_HEADER section;

    if (Rva == 0)
        return NULL;

    section = PhMappedImageRvaToSection(MappedImage, Rva);

    if (!section)
        return NULL;

    if (Section)
        *Section = section;

    return PTR_ADD_OFFSET(MappedImage->ViewBase, PTR_ADD_OFFSET(
        PTR_SUB_OFFSET(Rva, section->VirtualAddress),
        section->PointerToRawData
        ));
}

_Must_inspect_result_
_Ret_maybenull_
_Success_(return != NULL)
PVOID PhMappedImageVaToVa(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Va,
    _Out_opt_ PIMAGE_SECTION_HEADER *Section
    )
{
    ULONG rva;
    PIMAGE_SECTION_HEADER section;

    if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        rva = PtrToUlong(PTR_SUB_OFFSET(Va, MappedImage->NtHeaders32->OptionalHeader.ImageBase));
    }
    else if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        rva = PtrToUlong(PTR_SUB_OFFSET(Va, MappedImage->NtHeaders->OptionalHeader.ImageBase));
    }
    else
    {
        return NULL;
    }

    section = PhMappedImageRvaToSection(MappedImage, rva);

    if (!section)
        return NULL;

    if (Section)
        *Section = section;

    return PTR_ADD_OFFSET(MappedImage->ViewBase, PTR_ADD_OFFSET(
        PTR_SUB_OFFSET(rva, section->VirtualAddress),
        section->PointerToRawData
        ));
}

BOOLEAN PhGetMappedImageSectionName(
    _In_ PIMAGE_SECTION_HEADER Section,
    _Out_writes_opt_z_(Count) PWSTR Buffer,
    _In_ ULONG Count,
    _Out_opt_ PULONG ReturnCount
    )
{
    BOOLEAN result;
    SIZE_T returnCount;

    result = PhCopyStringZFromBytes(
        Section->Name,
        IMAGE_SIZEOF_SHORT_NAME,
        Buffer,
        Count,
        &returnCount
        );

    if (ReturnCount)
        *ReturnCount = (ULONG)returnCount;

    return result;
}

NTSTATUS PhGetMappedImageDataEntry(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Index,
    _Out_ PIMAGE_DATA_DIRECTORY *Entry
    )
{
    if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PIMAGE_OPTIONAL_HEADER32 optionalHeader;

        optionalHeader = (PIMAGE_OPTIONAL_HEADER32)&MappedImage->NtHeaders32->OptionalHeader;

        if (Index >= optionalHeader->NumberOfRvaAndSizes)
            return STATUS_INVALID_PARAMETER_2;

        *Entry = &optionalHeader->DataDirectory[Index];
    }
    else if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        PIMAGE_OPTIONAL_HEADER64 optionalHeader;

        optionalHeader = (PIMAGE_OPTIONAL_HEADER64)&MappedImage->NtHeaders->OptionalHeader;

        if (Index >= optionalHeader->NumberOfRvaAndSizes)
            return STATUS_INVALID_PARAMETER_2;

        *Entry = &optionalHeader->DataDirectory[Index];
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

PVOID PhGetMappedImageDirectoryEntry(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Index
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;

    status = PhGetMappedImageDataEntry(
        MappedImage,
        Index,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return NULL;

    return PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );
}

FORCEINLINE NTSTATUS PhpGetMappedImageLoadConfig(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ USHORT Magic,
    _In_ ULONG ProbeLength,
    _Out_ PVOID *LoadConfig
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY entry;
    PVOID loadConfig;

    if (MappedImage->Magic != Magic)
        return STATUS_INVALID_PARAMETER;

    status = PhGetMappedImageDataEntry(MappedImage, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &entry);

    if (!NT_SUCCESS(status))
        return status;

    loadConfig = PhMappedImageRvaToVa(MappedImage, entry->VirtualAddress, NULL);

    if (!loadConfig)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(MappedImage, loadConfig, ProbeLength);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    *LoadConfig = loadConfig;

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageLoadConfig32(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PIMAGE_LOAD_CONFIG_DIRECTORY32 *LoadConfig
    )
{
    return PhpGetMappedImageLoadConfig(
        MappedImage,
        IMAGE_NT_OPTIONAL_HDR32_MAGIC,
        sizeof(IMAGE_LOAD_CONFIG_DIRECTORY32),
        LoadConfig
        );
}

NTSTATUS PhGetMappedImageLoadConfig64(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PIMAGE_LOAD_CONFIG_DIRECTORY64 *LoadConfig
    )
{
    return PhpGetMappedImageLoadConfig(
        MappedImage,
        IMAGE_NT_OPTIONAL_HDR64_MAGIC,
        sizeof(IMAGE_LOAD_CONFIG_DIRECTORY64),
        LoadConfig
        );
}

NTSTATUS PhLoadRemoteMappedImage(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ViewBase,
    _Out_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    )
{
    return PhLoadRemoteMappedImageEx(ProcessHandle, ViewBase, NtReadVirtualMemory, RemoteMappedImage);
}

NTSTATUS PhLoadRemoteMappedImageEx(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ViewBase,
    _In_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _Out_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    )
{
    NTSTATUS status;
    IMAGE_DOS_HEADER dosHeader;
    ULONG ntHeadersOffset;
    IMAGE_NT_HEADERS ntHeaders;
    SIZE_T ntHeadersSize;
    PIMAGE_NT_HEADERS ntHeadersOut;

    RemoteMappedImage->ViewBase = ViewBase;

    status = ReadVirtualMemoryCallback(
        ProcessHandle,
        ViewBase,
        &dosHeader,
        sizeof(IMAGE_DOS_HEADER),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Check the initial MZ.

    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        return STATUS_INVALID_IMAGE_NOT_MZ;

    // Get a pointer to the NT headers and read it in for some basic information.

    ntHeadersOffset = (ULONG)dosHeader.e_lfanew;

    if (ntHeadersOffset == 0 || ntHeadersOffset >= 0x10000000)
        return STATUS_INVALID_IMAGE_FORMAT;

    status = ReadVirtualMemoryCallback(
        ProcessHandle,
        PTR_ADD_OFFSET(ViewBase, ntHeadersOffset),
        &ntHeaders,
        sizeof(IMAGE_NT_HEADERS),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Check the signature and verify the magic.

    if (ntHeaders.Signature != IMAGE_NT_SIGNATURE)
        return STATUS_INVALID_IMAGE_FORMAT;

    RemoteMappedImage->Magic = ntHeaders.OptionalHeader.Magic;

    if (
        RemoteMappedImage->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
        RemoteMappedImage->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC
        )
        return STATUS_INVALID_IMAGE_FORMAT;

    // Get the real size and read in the whole thing.

    RemoteMappedImage->NumberOfSections = ntHeaders.FileHeader.NumberOfSections;
    ntHeadersSize = (SIZE_T)UFIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) +
        ntHeaders.FileHeader.SizeOfOptionalHeader +
        RemoteMappedImage->NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

    if (ntHeadersSize > 1024 * 1024) // 1 MB
        return STATUS_INVALID_IMAGE_FORMAT;

    ntHeadersOut = PhAllocateZero(ntHeadersSize);

    status = ReadVirtualMemoryCallback(
        ProcessHandle,
        PTR_ADD_OFFSET(ViewBase, ntHeadersOffset),
        ntHeadersOut,
        ntHeadersSize,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        PhFree(ntHeadersOut);
        return status;
    }

    RemoteMappedImage->NtHeaders = ntHeadersOut;
    RemoteMappedImage->Sections = IMAGE_FIRST_SECTION(RemoteMappedImage->NtHeaders);

    return STATUS_SUCCESS;
}

NTSTATUS PhUnloadRemoteMappedImage(
    _Inout_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    )
{
    PhFree(RemoteMappedImage->NtHeaders);

    return STATUS_SUCCESS;
}

_Success_(return)
BOOLEAN PhGetRemoteMappedImageDirectoryEntry(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _In_ ULONG Index,
    _Out_ PVOID* DataBuffer,
    _Out_opt_ ULONG* DataLength
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PVOID dataBuffer;
    ULONG dataLength;

    if (RemoteMappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PIMAGE_OPTIONAL_HEADER32 optionalHeader;

        optionalHeader = (PIMAGE_OPTIONAL_HEADER32)&RemoteMappedImage->NtHeaders32->OptionalHeader;

        if (Index >= optionalHeader->NumberOfRvaAndSizes)
            return FALSE;

        dataDirectory = &optionalHeader->DataDirectory[Index];
    }
    else if (RemoteMappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        PIMAGE_OPTIONAL_HEADER64 optionalHeader;

        optionalHeader = (PIMAGE_OPTIONAL_HEADER64)&RemoteMappedImage->NtHeaders->OptionalHeader;

        if (Index >= optionalHeader->NumberOfRvaAndSizes)
            return FALSE;

        dataDirectory = &optionalHeader->DataDirectory[Index];
    }
    else
    {
        return FALSE;
    }

    if (!(dataDirectory->VirtualAddress && dataDirectory->Size))
        return FALSE;

    dataLength = dataDirectory->Size;
    dataBuffer = PhAllocateZero(dataLength);

    status = ReadVirtualMemoryCallback(
        ProcessHandle,
        PTR_ADD_OFFSET(RemoteMappedImage->ViewBase, dataDirectory->VirtualAddress),
        dataBuffer,
        dataLength,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        PhFree(dataBuffer);
        return FALSE;
    }

    if (DataBuffer)
        *DataBuffer = dataBuffer;
    if (DataLength)
        *DataLength = dataLength;

    return TRUE;
}

_Success_(return)
BOOLEAN PhGetRemoteMappedImageDebugEntryByType(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_ ULONG Type,
    _Out_opt_ ULONG* DataLength,
    _Out_ PVOID* DataBuffer
    )
{
    return PhGetRemoteMappedImageDebugEntryByTypeEx(ProcessHandle, RemoteMappedImage, Type, NtReadVirtualMemory, DataLength, DataBuffer);
}

_Success_(return)
BOOLEAN PhGetRemoteMappedImageDebugEntryByTypeEx(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_ ULONG Type,
    _In_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _Out_opt_ ULONG* DataLength,
    _Out_ PVOID* DataBuffer
    )
{
    PIMAGE_DEBUG_DIRECTORY debugDirectory;
    ULONG debugDirectoryLength;
    BOOLEAN result = FALSE;

    if (!PhGetRemoteMappedImageDirectoryEntry(
        ProcessHandle,
        RemoteMappedImage,
        ReadVirtualMemoryCallback,
        IMAGE_DIRECTORY_ENTRY_DEBUG,
        &debugDirectory,
        &debugDirectoryLength
        ))
    {
        return FALSE;
    }

    for (ULONG i = 0; i < debugDirectoryLength / sizeof(IMAGE_DEBUG_DIRECTORY); i++)
    {
        PIMAGE_DEBUG_DIRECTORY entry = PTR_ADD_OFFSET(debugDirectory, i * sizeof(IMAGE_DEBUG_DIRECTORY));

        if (entry->Type == Type)
        {
            PVOID dataBuffer = PhAllocateZero(entry->SizeOfData);

            if (NT_SUCCESS(ReadVirtualMemoryCallback(
                ProcessHandle,
                PTR_ADD_OFFSET(RemoteMappedImage->ViewBase, entry->AddressOfRawData),
                dataBuffer,
                entry->SizeOfData,
                NULL
                )))
            {
                if (DataLength)
                    *DataLength = entry->SizeOfData;

                *DataBuffer = dataBuffer;

                result = TRUE;
            }
            else
            {
                PhFree(dataBuffer);
            }

            break;
        }
    }

    PhFree(debugDirectory);

    return result;
}

_Success_(return)
BOOLEAN PhGetRemoteMappedImageGuardFlags(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _Out_ PULONG GuardFlags
    )
{
    return PhGetRemoteMappedImageGuardFlagsEx(ProcessHandle, RemoteMappedImage, NtReadVirtualMemory, GuardFlags);
}

_Success_(return)
BOOLEAN PhGetRemoteMappedImageGuardFlagsEx(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _Out_ PULONG GuardFlags
    )
{
    BOOLEAN result = FALSE;

    if (RemoteMappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PIMAGE_LOAD_CONFIG_DIRECTORY32 config32 = NULL;

        if (!PhGetRemoteMappedImageDirectoryEntry(
            ProcessHandle,
            RemoteMappedImage,
            ReadVirtualMemoryCallback,
            IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
            &config32,
            NULL
            ))
        {
            return FALSE;
        }

        if (config32)
        {
            if (RTL_CONTAINS_FIELD(config32, config32->Size, GuardFlags))
            {
                *GuardFlags = config32->GuardFlags;
                result = TRUE;
            }

            PhFree(config32);
        }
    }
    else
    {
        PIMAGE_LOAD_CONFIG_DIRECTORY64 config64 = NULL;

        if (!PhGetRemoteMappedImageDirectoryEntry(
            ProcessHandle,
            RemoteMappedImage,
            ReadVirtualMemoryCallback,
            IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
            &config64,
            NULL
            ))
        {
            return FALSE;
        }

        if (config64)
        {
            if (RTL_CONTAINS_FIELD(config64, config64->Size, GuardFlags))
            {
                *GuardFlags = config64->GuardFlags;
                result = TRUE;
            }

            PhFree(config64);
        }
    }

    return result;
}

NTSTATUS PhGetMappedImageExports(
    _Out_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_EXPORT_DIRECTORY exportDirectory;

    // Get a pointer to the export directory.

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    exportDirectory = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!exportDirectory)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(MappedImage, exportDirectory, sizeof(IMAGE_EXPORT_DIRECTORY));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    Exports->MappedImage = MappedImage;
    Exports->DataDirectory = dataDirectory;
    Exports->ExportDirectory = exportDirectory;
    Exports->NumberOfEntries = exportDirectory->NumberOfFunctions;

    // Get pointers to the various tables and probe them.

    Exports->AddressTable = (PULONG)PhMappedImageRvaToVa(
        MappedImage,
        exportDirectory->AddressOfFunctions,
        NULL
        );
    Exports->NamePointerTable = (PULONG)PhMappedImageRvaToVa(
        MappedImage,
        exportDirectory->AddressOfNames,
        NULL
        );
    Exports->OrdinalTable = (PUSHORT)PhMappedImageRvaToVa(
        MappedImage,
        exportDirectory->AddressOfNameOrdinals,
        NULL
        );

    // Note: NamePointerTable and OrdinalTable are null for binaries
    // such as mfc140u.dll yet contain valid exports (dmex)

    if (!Exports->AddressTable)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(
            MappedImage,
            Exports->AddressTable,
            exportDirectory->NumberOfFunctions * sizeof(ULONG)
            );

        if (Exports->NamePointerTable)
        {
            PhpMappedImageProbe(
                MappedImage,
                Exports->NamePointerTable,
                exportDirectory->NumberOfNames * sizeof(ULONG)
                );
        }

        if (Exports->OrdinalTable)
        {
            PhpMappedImageProbe(
                MappedImage,
                Exports->OrdinalTable,  // ordinal list for named exports
                exportDirectory->NumberOfNames * sizeof(USHORT)
                );
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    // The ordinal and name tables are parallel.
    // Getting an index into the name table (e.g. by doing a binary
    // search) and indexing into the ordinal table will produce the
    // ordinal for that name, *unbiased* (unlike in the specification).
    // The unbiased ordinal is an index into the address table.

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageExportEntry(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_EXPORT_ENTRY Entry
    )
{
    ULONG nameIndex = 0;
    BOOLEAN exportByName = FALSE;
    PSTR name;

    if (Index >= Exports->ExportDirectory->NumberOfFunctions)
        return STATUS_PROCEDURE_NOT_FOUND;

    Entry->Ordinal = (USHORT)Index + (USHORT)Exports->ExportDirectory->Base;

    if (Exports->OrdinalTable)
    {
        // look into named exports ordinal list.
        for (nameIndex = 0; nameIndex < Exports->ExportDirectory->NumberOfNames; nameIndex++)
        {
            if (Index == Exports->OrdinalTable[nameIndex])
            {
                exportByName = TRUE;
                break;
            }
        }
    }

    if (Exports->NamePointerTable && exportByName)
    {
        name = PhMappedImageRvaToVa(
            Exports->MappedImage,
            Exports->NamePointerTable[nameIndex],
            NULL
            );

        if (!name)
            return STATUS_INVALID_PARAMETER;

        // TODO: Probe the name.

        Entry->Name = name;
        Entry->Hint = nameIndex;
    }
    else
    {
        Entry->Name = NULL;
        Entry->Hint = 0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageExportFunction(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_opt_ PSTR Name,
    _In_opt_ USHORT Ordinal,
    _Out_ PPH_MAPPED_IMAGE_EXPORT_FUNCTION Function
    )
{
    ULONG rva;

    if (Name)
    {
        ULONG index;

        index = PhpLookupMappedImageExportName(Exports, Name);

        if (index == ULONG_MAX)
            return STATUS_PROCEDURE_NOT_FOUND;

        Ordinal = Exports->OrdinalTable[index] + (USHORT)Exports->ExportDirectory->Base;
    }

    Ordinal -= (USHORT)Exports->ExportDirectory->Base;

    if (Ordinal >= Exports->ExportDirectory->NumberOfFunctions)
        return STATUS_PROCEDURE_NOT_FOUND;

    rva = Exports->AddressTable[Ordinal];

    if (
        (rva >= Exports->DataDirectory->VirtualAddress) &&
        (rva < Exports->DataDirectory->VirtualAddress + Exports->DataDirectory->Size)
        )
    {
        // This is a forwarder RVA.

        Function->ForwardedName = PhMappedImageRvaToVa(
            Exports->MappedImage,
            rva,
            NULL
            );

        if (!Function->ForwardedName)
            return STATUS_INVALID_PARAMETER;

        // TODO: Probe the name.

        Function->Function = NULL;
    }
    else
    {
        Function->Function = UlongToPtr(rva);
        Function->ForwardedName = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageExportFunctionRemote(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_opt_ PSTR Name,
    _In_opt_ USHORT Ordinal,
    _In_ PVOID RemoteBase,
    _Out_ PVOID *Function
    )
{
    ULONG rva;

    if (Name)
    {
        ULONG index;

        index = PhpLookupMappedImageExportName(Exports, Name);

        if (index == ULONG_MAX)
            return STATUS_PROCEDURE_NOT_FOUND;

        Ordinal = Exports->OrdinalTable[index] + (USHORT)Exports->ExportDirectory->Base;
    }

    Ordinal -= (USHORT)Exports->ExportDirectory->Base;

    if (Ordinal >= Exports->ExportDirectory->NumberOfFunctions)
        return STATUS_PROCEDURE_NOT_FOUND;

    rva = Exports->AddressTable[Ordinal];

    if (
        (rva >= Exports->DataDirectory->VirtualAddress) &&
        (rva < Exports->DataDirectory->VirtualAddress + Exports->DataDirectory->Size)
        )
    {
        // This is a forwarder RVA. Not supported for remote lookup.
        return STATUS_NOT_SUPPORTED;
    }
    else
    {
        *Function = PTR_ADD_OFFSET(RemoteBase, rva);
    }

    return STATUS_SUCCESS;
}

ULONG PhpLookupMappedImageExportName(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ PSTR Name
    )
{
    LONG low;
    LONG high;
    LONG i;

    if (Exports->ExportDirectory->NumberOfNames == 0)
        return ULONG_MAX;

    low = 0;
    high = Exports->ExportDirectory->NumberOfNames - 1;

    do
    {
        PSTR name;
        INT comparison;

        i = (low + high) / 2;

        name = PhMappedImageRvaToVa(
            Exports->MappedImage,
            Exports->NamePointerTable[i],
            NULL
            );

        if (!name)
            return ULONG_MAX;

        // TODO: Probe the name.

        comparison = strcmp(Name, name);

        if (comparison == 0)
            return i;
        else if (comparison < 0)
            high = i - 1;
        else
            low = i + 1;
    } while (low <= high);

    return ULONG_MAX;
}

NTSTATUS PhGetMappedImageImports(
    _Out_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_IMPORT_DESCRIPTOR descriptor;
    ULONG i;

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_IMPORT,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    descriptor = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!descriptor)
        return STATUS_INVALID_PARAMETER;

    Imports->MappedImage = MappedImage;
    Imports->Flags = 0;
    Imports->DescriptorTable = descriptor;

    // Do a scan to determine how many import descriptors there are.

    i = 0;

    __try
    {
        while (TRUE)
        {
            PhpMappedImageProbe(MappedImage, descriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR));

            if (descriptor->OriginalFirstThunk == 0 && descriptor->FirstThunk == 0)
                break;

            descriptor++;
            i++;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    Imports->NumberOfDlls = i;

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageImportDll(
    _In_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll
    )
{
    ULONG i;

    if (Index >= Imports->NumberOfDlls)
        return STATUS_INVALID_PARAMETER_2;

    ImportDll->MappedImage = Imports->MappedImage;
    ImportDll->Flags = Imports->Flags;

    if (!(ImportDll->Flags & PH_MAPPED_IMAGE_DELAY_IMPORTS))
    {
        ImportDll->Descriptor = &Imports->DescriptorTable[Index];

        ImportDll->Name = PhMappedImageRvaToVa(
            ImportDll->MappedImage,
            ImportDll->Descriptor->Name,
            NULL
            );

        if (!ImportDll->Name)
            return STATUS_INVALID_PARAMETER;

        // TODO: Probe the name.

        if (ImportDll->Descriptor->OriginalFirstThunk)
        {
            ImportDll->LookupTable = PhMappedImageRvaToVa(
                ImportDll->MappedImage,
                ImportDll->Descriptor->OriginalFirstThunk,
                NULL
                );
        }
        else
        {
            ImportDll->LookupTable = PhMappedImageRvaToVa(
                ImportDll->MappedImage,
                ImportDll->Descriptor->FirstThunk,
                NULL
                );
        }
    }
    else
    {
        ImportDll->DelayDescriptor = &Imports->DelayDescriptorTable[Index];

        // Backwards compatible support for legacy V1 delay imports. (dmex)
        if (ImportDll->DelayDescriptor->Attributes.RvaBased == 0)
        {
            ImportDll->Flags |= PH_MAPPED_IMAGE_DELAY_IMPORTS_V1;
        }

        if (!(ImportDll->Flags & PH_MAPPED_IMAGE_DELAY_IMPORTS_V1))
        {
            ImportDll->Name = PhMappedImageRvaToVa(
                ImportDll->MappedImage,
                ImportDll->DelayDescriptor->DllNameRVA,
                NULL
                );
        }
        else
        {
            ImportDll->Name = PhMappedImageVaToVa(
                ImportDll->MappedImage,
                ImportDll->DelayDescriptor->DllNameRVA,
                NULL
                );
        }

        if (!ImportDll->Name)
            return STATUS_INVALID_PARAMETER;

        // TODO: Probe the name.

        if (!(ImportDll->Flags & PH_MAPPED_IMAGE_DELAY_IMPORTS_V1))
        {
            ImportDll->LookupTable = PhMappedImageRvaToVa(
                ImportDll->MappedImage,
                ImportDll->DelayDescriptor->ImportNameTableRVA,
                NULL
                );
        }
        else
        {
            ImportDll->LookupTable = PhMappedImageVaToVa(
                ImportDll->MappedImage,
                ImportDll->DelayDescriptor->ImportNameTableRVA,
                NULL
                );
        }
    }

    if (!ImportDll->LookupTable)
        return STATUS_INVALID_PARAMETER;

    // Do a scan to determine how many entries there are.

    i = 0;

    if (ImportDll->MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PIMAGE_THUNK_DATA32 entry;

        entry = (PIMAGE_THUNK_DATA32)ImportDll->LookupTable;

        __try
        {
            while (TRUE)
            {
                PhpMappedImageProbe(ImportDll->MappedImage, entry, sizeof(IMAGE_THUNK_DATA32));

                if (entry->u1.AddressOfData == 0)
                    break;

                entry++;
                i++;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    else if (ImportDll->MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        PIMAGE_THUNK_DATA64 entry;

        entry = (PIMAGE_THUNK_DATA64)ImportDll->LookupTable;

        __try
        {
            while (TRUE)
            {
                PhpMappedImageProbe(ImportDll->MappedImage, entry, sizeof(IMAGE_THUNK_DATA64));

                if (entry->u1.AddressOfData == 0)
                    break;

                entry++;
                i++;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    ImportDll->NumberOfEntries = i;

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageImportEntry(
    _In_ PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_IMPORT_ENTRY Entry
    )
{
    PIMAGE_IMPORT_BY_NAME importByName;

    if (Index >= ImportDll->NumberOfEntries)
        return STATUS_INVALID_PARAMETER_2;

    if (ImportDll->MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        IMAGE_THUNK_DATA32 entry;

        entry = ((PIMAGE_THUNK_DATA32)ImportDll->LookupTable)[Index];

        // Is this entry using an ordinal?

        if (IMAGE_SNAP_BY_ORDINAL32(entry.u1.Ordinal))
        {
            Entry->Name = NULL;
            Entry->Ordinal = IMAGE_ORDINAL32(entry.u1.Ordinal);

            return STATUS_SUCCESS;
        }
        else
        {
            if (!(ImportDll->Flags & PH_MAPPED_IMAGE_DELAY_IMPORTS_V1))
            {
                importByName = PhMappedImageRvaToVa(
                    ImportDll->MappedImage,
                    entry.u1.AddressOfData,
                    NULL
                    );
            }
            else
            {
                importByName = PhMappedImageVaToVa(
                    ImportDll->MappedImage,
                    entry.u1.AddressOfData,
                    NULL
                    );
            }
        }
    }
    else if (ImportDll->MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        IMAGE_THUNK_DATA64 entry;

        entry = ((PIMAGE_THUNK_DATA64)ImportDll->LookupTable)[Index];

        // Is this entry using an ordinal?

        if (IMAGE_SNAP_BY_ORDINAL64(entry.u1.Ordinal))
        {
            Entry->Name = NULL;
            Entry->Ordinal = IMAGE_ORDINAL64(entry.u1.Ordinal);

            return STATUS_SUCCESS;
        }
        else
        {
            if (!(ImportDll->Flags & PH_MAPPED_IMAGE_DELAY_IMPORTS_V1))
            {
                importByName = PhMappedImageRvaToVa(
                    ImportDll->MappedImage,
                    (ULONG)entry.u1.AddressOfData,
                    NULL
                    );
            }
            else
            {
                importByName = PhMappedImageVaToVa(
                    ImportDll->MappedImage,
                    (ULONG)entry.u1.AddressOfData,
                    NULL
                    );
            }
        }
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!importByName)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(ImportDll->MappedImage, importByName, sizeof(IMAGE_IMPORT_BY_NAME));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    Entry->Name = (PSTR)importByName->Name;
    Entry->NameHint = importByName->Hint;

    // TODO: Probe the name.

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageDelayImports(
    _Out_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_DELAYLOAD_DESCRIPTOR descriptor;
    ULONG i;

    Imports->MappedImage = MappedImage;
    Imports->Flags = PH_MAPPED_IMAGE_DELAY_IMPORTS;

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    descriptor = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!descriptor)
        return STATUS_INVALID_PARAMETER;

    Imports->DelayDescriptorTable = descriptor;

    // Do a scan to determine how many import descriptors there are.

    i = 0;

    __try
    {
        while (TRUE)
        {
            PhpMappedImageProbe(MappedImage, descriptor, sizeof(PIMAGE_DELAYLOAD_DESCRIPTOR));

            if (descriptor->ImportAddressTableRVA == 0 && descriptor->ImportNameTableRVA == 0)
                break;

            descriptor++;
            i++;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    Imports->NumberOfDlls = i;

    return STATUS_SUCCESS;
}

USHORT PhCheckSum(
    _In_ ULONG Sum,
    _In_reads_(Count) PUSHORT Buffer,
    _In_ ULONG Count
    )
{
    while (Count--)
    {
        Sum += *Buffer++;
        Sum = (Sum >> 16) + (Sum & 0xffff);
    }

    Sum = (Sum >> 16) + Sum;

    return (USHORT)Sum;
}

ULONG PhCheckSumMappedImage(
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    ULONG checkSum;
    USHORT partialSum;
    PUSHORT adjust;

    partialSum = PhCheckSum(0, (PUSHORT)MappedImage->ViewBase, (ULONG)(MappedImage->Size + 1) / 2);

    // This is actually the same for 32-bit and 64-bit executables.
    adjust = (PUSHORT)&MappedImage->NtHeaders->OptionalHeader.CheckSum;

    // Subtract the existing check sum (with carry).
    partialSum -= partialSum < adjust[0];
    partialSum -= adjust[0];
    partialSum -= partialSum < adjust[1];
    partialSum -= adjust[1];

    checkSum = partialSum + (ULONG)MappedImage->Size;

    return checkSum;
}

NTSTATUS PhGetMappedImageCfg64(
    _Out_ PPH_MAPPED_IMAGE_CFG CfgConfig,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_LOAD_CONFIG_DIRECTORY64 config64;

    if (!NT_SUCCESS(status = PhGetMappedImageLoadConfig64(MappedImage, &config64)))
        return status;

    // Not every load configuration defines CFG characteristics
    if (!RTL_CONTAINS_FIELD(config64, config64->Size, GuardFlags))
        return STATUS_INVALID_VIEW_SIZE;

    CfgConfig->MappedImage = MappedImage;
    CfgConfig->EntrySize = RTL_FIELD_SIZE(IMAGE_CFG_ENTRY, Rva) +
        (ULONG)((config64->GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK) >> IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT);
    CfgConfig->CfgInstrumented = !!(config64->GuardFlags & IMAGE_GUARD_CF_INSTRUMENTED);
    CfgConfig->WriteIntegrityChecks = !!(config64->GuardFlags & IMAGE_GUARD_CFW_INSTRUMENTED);
    CfgConfig->CfgFunctionTablePresent = !!(config64->GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT);
    CfgConfig->SecurityCookieUnused = !!(config64->GuardFlags & IMAGE_GUARD_SECURITY_COOKIE_UNUSED);
    CfgConfig->ProtectDelayLoadedIat = !!(config64->GuardFlags & IMAGE_GUARD_PROTECT_DELAYLOAD_IAT);
    CfgConfig->DelayLoadInDidatSection = !!(config64->GuardFlags & IMAGE_GUARD_DELAYLOAD_IAT_IN_ITS_OWN_SECTION);
    CfgConfig->EnableExportSuppression = !!(config64->GuardFlags & IMAGE_GUARD_CF_ENABLE_EXPORT_SUPPRESSION);
    CfgConfig->HasExportSuppressionInfos = !!(config64->GuardFlags & IMAGE_GUARD_CF_EXPORT_SUPPRESSION_INFO_PRESENT);
    CfgConfig->CfgLongJumpTablePresent = !!(config64->GuardFlags & IMAGE_GUARD_CF_LONGJUMP_TABLE_PRESENT);

    CfgConfig->NumberOfGuardFunctionEntries = config64->GuardCFFunctionCount;
    CfgConfig->GuardFunctionTable = PhMappedImageVaToVa(
        MappedImage,
        (ULONG)config64->GuardCFFunctionTable,
        NULL
        );

    if (CfgConfig->GuardFunctionTable && CfgConfig->NumberOfGuardFunctionEntries)
    {
        __try
        {
            PhpMappedImageProbe(
                MappedImage,
                CfgConfig->GuardFunctionTable,
                (SIZE_T)(CfgConfig->EntrySize * CfgConfig->NumberOfGuardFunctionEntries)
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    CfgConfig->NumberOfGuardAdressIatEntries = 0;
    CfgConfig->GuardAdressIatTable = 0;

    if (RTL_CONTAINS_FIELD(config64, config64->Size, GuardAddressTakenIatEntryTable))
    {
        CfgConfig->NumberOfGuardAdressIatEntries = config64->GuardAddressTakenIatEntryCount;
        CfgConfig->GuardAdressIatTable = PhMappedImageVaToVa(
            MappedImage,
            (ULONG)config64->GuardAddressTakenIatEntryTable,
            NULL
            );

        if (CfgConfig->GuardAdressIatTable && CfgConfig->NumberOfGuardAdressIatEntries)
        {
            __try
            {
                PhpMappedImageProbe(
                    MappedImage,
                    CfgConfig->GuardAdressIatTable,
                    (SIZE_T)(CfgConfig->EntrySize * CfgConfig->NumberOfGuardAdressIatEntries)
                    );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
    }

    CfgConfig->NumberOfGuardLongJumpEntries = 0;
    CfgConfig->GuardLongJumpTable = 0;

    if (RTL_CONTAINS_FIELD(config64, config64->Size, GuardLongJumpTargetTable))
    {
        CfgConfig->NumberOfGuardLongJumpEntries = config64->GuardLongJumpTargetCount;
        CfgConfig->GuardLongJumpTable = PhMappedImageVaToVa(
            MappedImage,
            (ULONG)config64->GuardLongJumpTargetTable,
            NULL
            );

        if (CfgConfig->GuardLongJumpTable && CfgConfig->NumberOfGuardLongJumpEntries)
        {
            __try
            {
                PhpMappedImageProbe(
                    MappedImage,
                    CfgConfig->GuardLongJumpTable,
                    (SIZE_T)(CfgConfig->EntrySize * CfgConfig->NumberOfGuardLongJumpEntries)
                    );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageCfg32(
    _Out_ PPH_MAPPED_IMAGE_CFG CfgConfig,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_LOAD_CONFIG_DIRECTORY32 config32;

    if (!NT_SUCCESS(status = PhGetMappedImageLoadConfig32(MappedImage, &config32)))
        return status;
    
    // Not every load configuration defines CFG characteristics
    if (!RTL_CONTAINS_FIELD(config32, config32->Size, GuardFlags))
        return STATUS_INVALID_VIEW_SIZE;

    CfgConfig->MappedImage = MappedImage;
    CfgConfig->EntrySize = RTL_FIELD_SIZE(IMAGE_CFG_ENTRY, Rva) +
        (ULONG)((config32->GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK) >> IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT);
    CfgConfig->CfgInstrumented = !!(config32->GuardFlags & IMAGE_GUARD_CF_INSTRUMENTED);
    CfgConfig->WriteIntegrityChecks = !!(config32->GuardFlags & IMAGE_GUARD_CFW_INSTRUMENTED);
    CfgConfig->CfgFunctionTablePresent = !!(config32->GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT);
    CfgConfig->SecurityCookieUnused = !!(config32->GuardFlags & IMAGE_GUARD_SECURITY_COOKIE_UNUSED);
    CfgConfig->ProtectDelayLoadedIat = !!(config32->GuardFlags & IMAGE_GUARD_PROTECT_DELAYLOAD_IAT);
    CfgConfig->DelayLoadInDidatSection = !!(config32->GuardFlags & IMAGE_GUARD_DELAYLOAD_IAT_IN_ITS_OWN_SECTION);
    CfgConfig->EnableExportSuppression = !!(config32->GuardFlags & IMAGE_GUARD_CF_ENABLE_EXPORT_SUPPRESSION);
    CfgConfig->HasExportSuppressionInfos = !!(config32->GuardFlags & IMAGE_GUARD_CF_EXPORT_SUPPRESSION_INFO_PRESENT);
    CfgConfig->CfgLongJumpTablePresent = !!(config32->GuardFlags & IMAGE_GUARD_CF_LONGJUMP_TABLE_PRESENT);

    CfgConfig->NumberOfGuardFunctionEntries = config32->GuardCFFunctionCount;
    CfgConfig->GuardFunctionTable = PhMappedImageVaToVa(
        MappedImage,
        config32->GuardCFFunctionTable,
        NULL
        );
    
    if (CfgConfig->GuardFunctionTable && CfgConfig->NumberOfGuardFunctionEntries)
    {
        __try
        {
            PhpMappedImageProbe(
                MappedImage,
                CfgConfig->GuardFunctionTable,
                (SIZE_T)(CfgConfig->EntrySize * CfgConfig->NumberOfGuardFunctionEntries)
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    CfgConfig->NumberOfGuardAdressIatEntries = 0;
    CfgConfig->GuardAdressIatTable = 0;

    if (RTL_CONTAINS_FIELD(config32, config32->Size, GuardAddressTakenIatEntryTable))
    {
        CfgConfig->NumberOfGuardAdressIatEntries = config32->GuardAddressTakenIatEntryCount;
        CfgConfig->GuardAdressIatTable = PhMappedImageVaToVa(
            MappedImage,
            config32->GuardAddressTakenIatEntryTable,
            NULL
            );

        if (CfgConfig->GuardAdressIatTable && CfgConfig->NumberOfGuardAdressIatEntries)
        {
            __try
            {
                PhpMappedImageProbe(
                    MappedImage,
                    CfgConfig->GuardAdressIatTable,
                    (SIZE_T)(CfgConfig->EntrySize * CfgConfig->NumberOfGuardAdressIatEntries)
                    );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
    }

    CfgConfig->NumberOfGuardLongJumpEntries = 0;
    CfgConfig->GuardLongJumpTable = 0;

    if (RTL_CONTAINS_FIELD(config32, config32->Size, GuardLongJumpTargetTable))
    {
        CfgConfig->NumberOfGuardLongJumpEntries = config32->GuardLongJumpTargetCount;
        CfgConfig->GuardLongJumpTable = PhMappedImageVaToVa(
            MappedImage,
            config32->GuardLongJumpTargetTable,
            NULL
            );

        if (CfgConfig->GuardLongJumpTable && CfgConfig->NumberOfGuardLongJumpEntries)
        {
            __try
            {
                PhpMappedImageProbe(
                    MappedImage,
                    CfgConfig->GuardLongJumpTable,
                    (SIZE_T)(CfgConfig->EntrySize * CfgConfig->NumberOfGuardLongJumpEntries)
                    );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageCfg(
    _Out_ PPH_MAPPED_IMAGE_CFG CfgConfig,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        return PhGetMappedImageCfg32(CfgConfig, MappedImage);
    }
    else
    {
        return PhGetMappedImageCfg64(CfgConfig, MappedImage);
    }
}

NTSTATUS PhGetMappedImageCfgEntry(
    _In_ PPH_MAPPED_IMAGE_CFG CfgConfig,
    _In_ ULONGLONG Index,
    _In_ CFG_ENTRY_TYPE Type,
    _Out_ PIMAGE_CFG_ENTRY Entry
    )
{
    PULONGLONG guardTable;
    ULONGLONG numberofGuardEntries;
    PIMAGE_CFG_ENTRY cfgMappedEntry;

    switch (Type)
    {
    case ControlFlowGuardFunction:
        {
            guardTable = CfgConfig->GuardFunctionTable;
            numberofGuardEntries = CfgConfig->NumberOfGuardFunctionEntries;
        }
        break;
    case ControlFlowGuardTakenIatEntry:
        {
            guardTable = CfgConfig->GuardAdressIatTable;
            numberofGuardEntries = CfgConfig->NumberOfGuardAdressIatEntries;
        }
        break;
    case ControlFlowGuardLongJump:
        {
            guardTable = CfgConfig->GuardLongJumpTable;
            numberofGuardEntries = CfgConfig->NumberOfGuardLongJumpEntries;
        }
        break;
    default:
        return STATUS_INVALID_PARAMETER_3;
    }

    if (!guardTable || Index >= numberofGuardEntries)
        return STATUS_PROCEDURE_NOT_FOUND;

    cfgMappedEntry = (PIMAGE_CFG_ENTRY)PTR_ADD_OFFSET(guardTable, Index * CfgConfig->EntrySize);

    Entry->Rva = cfgMappedEntry->Rva;

    // Optional header after the rva entry
    if (CfgConfig->EntrySize > RTL_FIELD_SIZE(IMAGE_CFG_ENTRY, Rva))
    {
        Entry->SuppressedCall = cfgMappedEntry->SuppressedCall;
        Entry->ExportSuppressed = cfgMappedEntry->ExportSuppressed;
        Entry->LangExcptHandler = cfgMappedEntry->LangExcptHandler;
        Entry->Xfg = cfgMappedEntry->Xfg;
        Entry->Reserved = cfgMappedEntry->Reserved;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageResources(
    _Out_ PPH_MAPPED_IMAGE_RESOURCES Resources,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_RESOURCE_DIRECTORY resourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY nameDirectory;
    PIMAGE_RESOURCE_DIRECTORY languageDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceType;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceName;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceLanguage;
    ULONG resourceCount = 0;
    ULONG resourceTypeCount;
    ULONG resourceNameCount;
    ULONG resourceLanguageCount;
    PH_ARRAY resourceArray;

    // Get a pointer to the resource directory.

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_RESOURCE,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    resourceDirectory = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!resourceDirectory)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(MappedImage, resourceDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    // NOTE: We can't use LdrEnumResources here because we're using an image mapped with SEC_COMMIT.

    // Do a scan to determine how many resources there are.

    resourceType = PTR_ADD_OFFSET(resourceDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
    resourceTypeCount = resourceDirectory->NumberOfNamedEntries + resourceDirectory->NumberOfIdEntries;

    for (ULONG i = 0; i < resourceTypeCount; ++i, ++resourceType)
    {
        if (!resourceType->DataIsDirectory)
            continue; // return STATUS_RESOURCE_TYPE_NOT_FOUND;

        nameDirectory = PTR_ADD_OFFSET(resourceDirectory, resourceType->OffsetToDirectory);
        resourceName = PTR_ADD_OFFSET(nameDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
        resourceNameCount = nameDirectory->NumberOfNamedEntries + nameDirectory->NumberOfIdEntries;

        for (ULONG j = 0; j < resourceNameCount; ++j, ++resourceName)
        {
            if (!resourceName->DataIsDirectory)
                continue; // return STATUS_RESOURCE_NAME_NOT_FOUND;

            languageDirectory = PTR_ADD_OFFSET(resourceDirectory, resourceName->OffsetToDirectory);
            resourceLanguage = PTR_ADD_OFFSET(languageDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
            resourceLanguageCount = languageDirectory->NumberOfNamedEntries + languageDirectory->NumberOfIdEntries;

            for (ULONG k = 0; k < resourceLanguageCount; ++k, ++resourceLanguage)
            {
                if (resourceLanguage->DataIsDirectory)
                    continue; // return STATUS_RESOURCE_DATA_NOT_FOUND;

                resourceCount++;
            }
        }
    }

    if (resourceCount == 0)
        return STATUS_INVALID_IMAGE_FORMAT;

    // Allocate the number of resources.

    PhInitializeArray(&resourceArray, sizeof(PH_IMAGE_RESOURCE_ENTRY), resourceCount);

    // Enumerate the resources adding them into our buffer.

    resourceType = PTR_ADD_OFFSET(resourceDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
    resourceTypeCount = resourceDirectory->NumberOfNamedEntries + resourceDirectory->NumberOfIdEntries;

    for (ULONG i = 0; i < resourceTypeCount; ++i, ++resourceType)
    {
        if (!resourceType->DataIsDirectory)
            continue;

        nameDirectory = PTR_ADD_OFFSET(resourceDirectory, resourceType->OffsetToDirectory);
        resourceName = PTR_ADD_OFFSET(nameDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
        resourceNameCount = nameDirectory->NumberOfNamedEntries + nameDirectory->NumberOfIdEntries;

        for (ULONG j = 0; j < resourceNameCount; ++j, ++resourceName)
        {
            if (!resourceName->DataIsDirectory)
                continue;

            languageDirectory = PTR_ADD_OFFSET(resourceDirectory, resourceName->OffsetToDirectory);
            resourceLanguage = PTR_ADD_OFFSET(languageDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
            resourceLanguageCount = languageDirectory->NumberOfNamedEntries + languageDirectory->NumberOfIdEntries;

            for (ULONG k = 0; k < resourceLanguageCount; ++k, ++resourceLanguage)
            {
                PIMAGE_RESOURCE_DATA_ENTRY resourceData;

                if (resourceLanguage->DataIsDirectory)
                    continue;

                resourceData = PTR_ADD_OFFSET(resourceDirectory, resourceLanguage->OffsetToData);

                {
                    PH_IMAGE_RESOURCE_ENTRY entry;

                    entry.Type = NAME_FROM_RESOURCE_ENTRY(resourceDirectory, resourceType);
                    entry.Name = NAME_FROM_RESOURCE_ENTRY(resourceDirectory, resourceName);
                    entry.Language = NAME_FROM_RESOURCE_ENTRY(resourceDirectory, resourceLanguage);
                    entry.Offset = resourceData->OffsetToData;
                    entry.Size = resourceData->Size;
                    entry.CodePage = resourceData->CodePage;
                    entry.Data = PhMappedImageRvaToVa(MappedImage, resourceData->OffsetToData, NULL);

                    PhAddItemArray(&resourceArray, &entry);
                }
            }
        }
    }

    Resources->MappedImage = MappedImage;
    Resources->DataDirectory = dataDirectory;
    Resources->ResourceDirectory = resourceDirectory;
    Resources->NumberOfEntries = (ULONG)resourceArray.Count; // resourceCount;
    Resources->ResourceEntries = PhFinalArrayItems(&resourceArray);

    return status;
}

NTSTATUS PhGetMappedImageResource(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _In_ USHORT Language,
    _Out_opt_ ULONG *ResourceLength,
    _Out_opt_ PVOID *ResourceBuffer
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_RESOURCE_DIRECTORY resourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY nameDirectory;
    PIMAGE_RESOURCE_DIRECTORY languageDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceType;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceName;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceLanguage;
    ULONG resourceCount = 0;
    ULONG resourceTypeCount;
    ULONG resourceNameCount;
    ULONG resourceLanguageCount;

    // Get a pointer to the resource directory.

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_RESOURCE,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    resourceDirectory = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!resourceDirectory)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(MappedImage, resourceDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    resourceType = PTR_ADD_OFFSET(resourceDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
    resourceTypeCount = resourceDirectory->NumberOfNamedEntries + resourceDirectory->NumberOfIdEntries;

    for (ULONG i = 0; i < resourceTypeCount; ++i, ++resourceType)
    {
        if (!resourceType->DataIsDirectory)
            continue;

        nameDirectory = PTR_ADD_OFFSET(resourceDirectory, resourceType->OffsetToDirectory);
        resourceName = PTR_ADD_OFFSET(nameDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
        resourceNameCount = nameDirectory->NumberOfNamedEntries + nameDirectory->NumberOfIdEntries;

        for (ULONG j = 0; j < resourceNameCount; ++j, ++resourceName)
        {
            if (!resourceName->DataIsDirectory)
                continue;

            languageDirectory = PTR_ADD_OFFSET(resourceDirectory, resourceName->OffsetToDirectory);
            resourceLanguage = PTR_ADD_OFFSET(languageDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
            resourceLanguageCount = languageDirectory->NumberOfNamedEntries + languageDirectory->NumberOfIdEntries;

            for (ULONG k = 0; k < resourceLanguageCount; ++k, ++resourceLanguage)
            {
                PIMAGE_RESOURCE_DATA_ENTRY resourceData;

                if (resourceLanguage->DataIsDirectory)
                    continue;

                if (IS_INTRESOURCE(Type))
                {
                    if (resourceType->NameIsString)
                        continue;
                    if (resourceType->Id != PtrToUshort(Type))
                        continue;
                }
                else
                {
                    PIMAGE_RESOURCE_DIR_STRING_U resourceString;
                    PH_STRINGREF string1;
                    PH_STRINGREF string2;

                    if (!resourceType->NameIsString)
                        continue;

                    resourceString = PTR_ADD_OFFSET(resourceDirectory, resourceType->NameOffset);
                    string1.Buffer = resourceString->NameString;
                    string1.Length = resourceString->Length * sizeof(WCHAR);
                    PhInitializeStringRefLongHint(&string2, (PWSTR)Type);

                    if (!PhEqualStringRef(&string1, &string2, TRUE))
                        continue;
                }

                if (IS_INTRESOURCE(Name))
                {
                    if (resourceName->NameIsString)
                        continue;
                    if (resourceName->Id != PtrToUshort(Name))
                        continue;
                }
                else
                {
                    PIMAGE_RESOURCE_DIR_STRING_U resourceString;
                    PH_STRINGREF string1;
                    PH_STRINGREF string2;

                    if (!resourceName->NameIsString)
                        continue;

                    resourceString = PTR_ADD_OFFSET(resourceDirectory, resourceName->NameOffset);
                    string1.Buffer = resourceString->NameString;
                    string1.Length = resourceString->Length * sizeof(WCHAR);
                    PhInitializeStringRefLongHint(&string2, (PWSTR)Name);

                    if (!PhEqualStringRef(&string1, &string2, TRUE))
                        continue;
                }

                if (Language)
                {
                    if (resourceLanguage->NameIsString)
                        continue;
                    if (resourceLanguage->Id != Language)
                        continue;
                }

                resourceData = PTR_ADD_OFFSET(resourceDirectory, resourceLanguage->OffsetToData);

                if (ResourceLength)
                {
                    *ResourceLength = resourceData->Size;
                }

                if (ResourceBuffer)
                {
                    *ResourceBuffer = PhMappedImageRvaToVa(MappedImage, resourceData->OffsetToData, NULL);
                }

                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS PhGetMappedImageTlsCallbackDirectory32(
    _Out_ PPH_MAPPED_IMAGE_TLS_CALLBACKS TlsCallbacks,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_TLS_DIRECTORY32 tlsDirectory;
    ULONG_PTR tlsCallbacksOffset;

    // Get a pointer to the resource directory.

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_TLS,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    tlsDirectory = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!tlsDirectory)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(MappedImage, tlsDirectory, sizeof(IMAGE_TLS_DIRECTORY32));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    TlsCallbacks->DataDirectory = dataDirectory;
    TlsCallbacks->TlsDirectory32 = tlsDirectory;

    if (tlsDirectory->StartAddressOfRawData > MappedImage->NtHeaders32->OptionalHeader.ImageBase)
        tlsCallbacksOffset = MappedImage->NtHeaders32->OptionalHeader.ImageBase;
    else
        tlsCallbacksOffset = 0;

    //TlsCallbacks->CallbackIndexes = PhMappedImageRvaToVa(MappedImage, PtrToUlong(PTR_SUB_OFFSET(tlsDirectory->AddressOfIndex, tlsCallbacksOffset)), NULL);
    TlsCallbacks->CallbackAddress = PhMappedImageRvaToVa(MappedImage, PtrToUlong(PTR_SUB_OFFSET(tlsDirectory->AddressOfCallBacks, tlsCallbacksOffset)), NULL);

    if (TlsCallbacks->CallbackAddress)
        return STATUS_SUCCESS;

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS PhGetMappedImageTlsCallbackDirectory64(
    _Out_ PPH_MAPPED_IMAGE_TLS_CALLBACKS TlsCallbacks,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_TLS_DIRECTORY64 tlsDirectory;
    ULONG_PTR tlsCallbacksOffset;

    // Get a pointer to the resource directory.

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_TLS,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    tlsDirectory = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!tlsDirectory)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(MappedImage, tlsDirectory, sizeof(IMAGE_TLS_DIRECTORY64));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    TlsCallbacks->DataDirectory = dataDirectory;
    TlsCallbacks->TlsDirectory64 = tlsDirectory;

    if (tlsDirectory->StartAddressOfRawData > MappedImage->NtHeaders->OptionalHeader.ImageBase)
        tlsCallbacksOffset = MappedImage->NtHeaders->OptionalHeader.ImageBase;
    else
        tlsCallbacksOffset = 0;

    //TlsCallbacks->CallbackIndexes = PhMappedImageRvaToVa(MappedImage, PtrToUlong(PTR_SUB_OFFSET(tlsDirectory->AddressOfIndex, tlsCallbacksOffset)), NULL);
    TlsCallbacks->CallbackAddress = PhMappedImageRvaToVa(MappedImage, PtrToUlong(PTR_SUB_OFFSET(tlsDirectory->AddressOfCallBacks, tlsCallbacksOffset)), NULL);

    if (TlsCallbacks->CallbackAddress)
        return STATUS_SUCCESS;

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS PhGetMappedImageTlsCallbacks(
    _Out_ PPH_MAPPED_IMAGE_TLS_CALLBACKS TlsCallbacks,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    ULONG count = 0;
    PH_ARRAY entryArray;

    // Get a pointer to the TLS directory.

    if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        status = PhGetMappedImageTlsCallbackDirectory32(TlsCallbacks, MappedImage);
    else
        status = PhGetMappedImageTlsCallbackDirectory64(TlsCallbacks, MappedImage);

    if (!NT_SUCCESS(status))
        return status;

    // Do a scan to determine how many callbacks there are.

    if (TlsCallbacks->CallbackAddress)
    {
        if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            PULONG array = (PULONG)(PULONG_PTR)TlsCallbacks->CallbackAddress;

            for (ULONG i = 0; array[i]; i++)
                count++;
        }
        else
        {
            PULONGLONG array = (PULONGLONG)(PULONG_PTR)TlsCallbacks->CallbackAddress;

            for (ULONG i = 0; array[i]; i++)
                count++;
        }
    }

    if (count == 0)
        return STATUS_INVALID_IMAGE_FORMAT;

    // Allocate the number of TLS callbacks.

    PhInitializeArray(&entryArray, sizeof(PH_IMAGE_TLS_CALLBACK_ENTRY), count);

    // Add the TLS callbacks into our buffer.

    if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PULONG array = (PULONG)(PULONG_PTR)TlsCallbacks->CallbackAddress;

        for (ULONG i = 0; i < count; i++)
        {
            PH_IMAGE_TLS_CALLBACK_ENTRY entry;

            entry.Address = array[i];
            entry.Index = 0; // CallbackIndexes

            PhAddItemArray(&entryArray, &entry);
        }
    }
    else
    {
        PULONGLONG array = (PULONGLONG)(PULONG_PTR)TlsCallbacks->CallbackAddress;

        for (ULONG i = 0; i < count; i++)
        {
            PH_IMAGE_TLS_CALLBACK_ENTRY entry;

            entry.Address = array[i];
            entry.Index = 0; // CallbackIndexes

            PhAddItemArray(&entryArray, &entry);
        }
    }

    TlsCallbacks->NumberOfEntries = count;
    TlsCallbacks->Entries = PhFinalArrayItems(&entryArray);

    return status;
}

NTSTATUS PhGetMappedImageProdIdHeader(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_PRODID ProdIdHeader
    )
{
    PIMAGE_DOS_HEADER imageDosHeader = NULL;
    PIMAGE_NT_HEADERS imageNtHeader = NULL;
    PPRODITEM richHeaderStart = NULL; // PTR_ADD_OFFSET(imageDosHeader, 0x80)
    PPRODITEM richHeaderEnd = NULL; // PTR_SUB_OFFSET(imageNtHeader, 0x0);
    PPRODITEM richHeaderChecksum = NULL; // PTR_SUB_OFFSET(imageNtHeader, 0x10);
    ULONG ntHeadersOffset = ULONG_MAX;
    ULONG richHeaderKey = ULONG_MAX;
    ULONG richHeaderValue = ULONG_MAX;
    ULONG richStartSignature = ULONG_MAX;
    ULONG richEndSignature = ULONG_MAX;
    ULONG richHeaderStartOffset = ULONG_MAX;
    ULONG richHeaderEndOffset = ULONG_MAX;
    ULONG richHeaderLength = ULONG_MAX;
    PH_ARRAY richHeaderEntryArray;

    imageDosHeader = (PIMAGE_DOS_HEADER)MappedImage->ViewBase;

    if (imageDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return STATUS_INVALID_IMAGE_NOT_MZ;

    ntHeadersOffset = (ULONG)imageDosHeader->e_lfanew;

    if (ntHeadersOffset == 0 || ntHeadersOffset >= LONG_MAX)
        return STATUS_INVALID_IMAGE_FORMAT;

    imageNtHeader = PTR_ADD_OFFSET(MappedImage->ViewBase, ntHeadersOffset);

    if (imageNtHeader->Signature == IMAGE_NT_SIGNATURE)
    {
        PBYTE startHeaderAddress = PTR_ADD_OFFSET(MappedImage->ViewBase, ntHeadersOffset);
        PBYTE endHeaderAddress = PTR_ADD_OFFSET(MappedImage->ViewBase, sizeof(IMAGE_DOS_HEADER));

        for (PBYTE i = startHeaderAddress; i >= endHeaderAddress; i -= sizeof(ULONG))
        {
            __try
            {
                if (*(PULONG)i == ProdIdTagStart)
                {
                    richHeaderEndOffset = PtrToUlong(PTR_SUB_OFFSET(i, MappedImage->ViewBase));
                    break;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
    }

    if (richHeaderEndOffset == ULONG_MAX)
        return STATUS_FAIL_CHECK;

    richHeaderChecksum = PTR_ADD_OFFSET(MappedImage->ViewBase, richHeaderEndOffset);

    __try
    {
        PhpMappedImageProbe(MappedImage, richHeaderChecksum, sizeof(PRODITEM));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    richHeaderKey = richHeaderChecksum->dwCount;
    richStartSignature = richHeaderChecksum->dwProdid;

    if (richHeaderKey && richStartSignature)
    {
        PBYTE startHeaderAddress = PTR_ADD_OFFSET(MappedImage->ViewBase, ntHeadersOffset);
        PBYTE endHeaderAddress = PTR_ADD_OFFSET(MappedImage->ViewBase, sizeof(IMAGE_DOS_HEADER));

        for (PBYTE i = startHeaderAddress; i >= endHeaderAddress; i -= sizeof(ULONG))
        {
            __try
            {
                if ((*(PULONG)i ^ richHeaderKey) == ProdIdTagEnd)
                {
                    richHeaderStartOffset = PtrToUlong(PTR_SUB_OFFSET(i, MappedImage->ViewBase));
                    break;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
    }

    if (richHeaderStartOffset == ULONG_MAX)
        return STATUS_FAIL_CHECK;

    richHeaderValue = richHeaderStartOffset;
    richHeaderStart = PTR_ADD_OFFSET(MappedImage->ViewBase, richHeaderStartOffset);

    __try
    {
        PhpMappedImageProbe(MappedImage, richHeaderStart, sizeof(PRODITEM));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    richHeaderEnd = PTR_ADD_OFFSET(MappedImage->ViewBase, richHeaderEndOffset + sizeof(PRODITEM));

    __try
    {
        PhpMappedImageProbe(MappedImage, richHeaderEnd, sizeof(PRODITEM));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    richHeaderLength = PtrToUlong(PTR_SUB_OFFSET(richHeaderEnd, richHeaderStart));
    richEndSignature = richHeaderStart->dwProdid ^ richHeaderKey;

    if (richStartSignature == ProdIdTagStart && richEndSignature == ProdIdTagEnd)
    {
        PPH_STRING hashRawContentString = NULL;
        PPH_STRING hashContentString = NULL;
        ULONG currentCount = 0;
        PBYTE currentAddress;
        PBYTE currentEnd;
        PBYTE offset;

        currentAddress = PTR_ADD_OFFSET(richHeaderStart, 0);
        currentEnd = PTR_SUB_OFFSET(richHeaderEnd, 0);

        __try
        {
            PH_HASH_CONTEXT hashContext;
            UCHAR hash[32];

            PhInitializeHash(&hashContext, Md5HashAlgorithm);
            PhUpdateHash(&hashContext, richHeaderStart, richHeaderLength);

            if (PhFinalHash(&hashContext, hash, 16, NULL))
            {
                hashRawContentString = PhBufferToHexString(hash, 16);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }

        if (PhIsNullOrEmptyString(hashRawContentString))
            return STATUS_FAIL_CHECK;

        // VT creates a different hash based on the decrypted header while other tools
        // (including this one) create the hash based on the raw header. So create a second hash
        // from the decrypted header so we can show and search both hashes (dmex)
        {
            PVOID richHeaderContentEnd;
            ULONG richHeaderContentLength;
            PULONG richHeaderContentBuffer;
            PULONG richHeaderContentOffset;
            PH_HASH_CONTEXT hashContext;
            UCHAR hash[32];

            // Recalculate the length needed for the hash since VT doesn't include the remaining entry.
            richHeaderContentEnd = PTR_ADD_OFFSET(MappedImage->ViewBase, richHeaderEndOffset);
            richHeaderContentLength = PtrToUlong(PTR_SUB_OFFSET(richHeaderContentEnd, richHeaderStart));

            // We already probed above so this isn't really needed but probe again just to be sure.
            __try
            {
                PhpMappedImageProbe(MappedImage, richHeaderStart, richHeaderContentLength);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }

            richHeaderContentBuffer = PhAllocateZero(richHeaderContentLength);
            memcpy(richHeaderContentBuffer, richHeaderStart, richHeaderContentLength);

            // Walk the buffer and decrypt the entire thing. Based on the same loop used by yara:
            // https://github.com/VirusTotal/yara/blob/master/libyara/modules/pe/pe.c#L251-L259
            for (
                richHeaderContentOffset = richHeaderContentBuffer;
                richHeaderContentOffset < (PULONG)PTR_ADD_OFFSET(richHeaderContentBuffer, richHeaderContentLength);
                richHeaderContentOffset++
                )
            {
                *richHeaderContentOffset ^= richHeaderKey;
            }

            PhInitializeHash(&hashContext, Md5HashAlgorithm);
            PhUpdateHash(&hashContext, richHeaderContentBuffer, richHeaderContentLength);

            if (PhFinalHash(&hashContext, hash, 16, NULL))
            {
                hashContentString = PhBufferToHexString(hash, 16);
            }

            PhFree(richHeaderContentBuffer);
        }

        if (PhIsNullOrEmptyString(hashContentString))
            return STATUS_FAIL_CHECK;

        // Do a scan to determine how many entries there are.
        for (offset = currentAddress; offset < currentEnd; offset += sizeof(PRODITEM))
        {
            currentCount++;
        }

        // Compute the DOS header and DOS stub checksums.
        for (ULONG i = 0; i < richHeaderStartOffset; i++)
        {
            BYTE value;

            // Skip the e_lfanew field.
            if (i >= UFIELD_OFFSET(IMAGE_DOS_HEADER, e_lfanew) &&
                i <= UFIELD_OFFSET(IMAGE_DOS_HEADER, e_lfanew) + RTL_FIELD_SIZE(IMAGE_DOS_HEADER, e_lfanew) - sizeof(BYTE))
            {
                continue;
            }

            __try
            {
                value = *(PBYTE)PTR_ADD_OFFSET(imageDosHeader, i * sizeof(BYTE));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                continue;
            }

            richHeaderValue += _rotl(value, i);
        }

        // Compute each of the RICH entry value checksums.
        for (ULONG i = 0; i < currentCount; i++)
        {
            PPRODITEM entry;
            ULONG prodid;
            ULONG count;

            entry = PTR_ADD_OFFSET(currentAddress, i * sizeof(PRODITEM));

            __try
            {
                PhpMappedImageProbe(MappedImage, entry, sizeof(PRODITEM));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                continue;
            }

            prodid = entry->dwProdid ^ richHeaderKey;
            count = entry->dwCount ^ richHeaderKey;

            if (count > 0 && count != richHeaderKey)
            {
                richHeaderValue += _rotl((ProdidFromDwProdid(prodid) << 16 | WBuildFromDwProdid(prodid)), count & 0x1F);
            }
        }

        // Allocate the number of product entries.

        PhInitializeArray(&richHeaderEntryArray, sizeof(PH_MAPPED_IMAGE_PRODID_ENTRY), currentCount);

        // Add the product entries into our buffer.

        for (ULONG i = 0; i < currentCount; i++)
        {
            PPRODITEM item = PTR_ADD_OFFSET(currentAddress, i * sizeof(PRODITEM));

            __try
            {
                PhpMappedImageProbe(MappedImage, item, sizeof(PRODITEM));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                //return GetExceptionCode();
                continue;
            }

            // The prodid header can include 3 extra checksum values. Ignore these for now (dmex) 
            if ((item->dwCount ^ richHeaderKey) != richHeaderKey)
            {
                PH_MAPPED_IMAGE_PRODID_ENTRY entry;

                entry.ProductId = ProdidFromDwProdid(item->dwProdid ^ richHeaderKey);
                entry.ProductBuild = WBuildFromDwProdid(item->dwProdid ^ richHeaderKey);
                entry.ProductCount = item->dwCount ^ richHeaderKey;

                PhAddItemArray(&richHeaderEntryArray, &entry);
            }
        }

        //PhPrintPointer(ProdIdHeader->Key, UlongToPtr(richHeaderKey));
        ProdIdHeader->Valid = richHeaderKey == richHeaderValue;
        ProdIdHeader->Key = PhFormatString(L"%lx", richHeaderKey);
        ProdIdHeader->RawHash = hashRawContentString;
        ProdIdHeader->Hash = hashContentString;
        ProdIdHeader->NumberOfEntries = currentCount;
        ProdIdHeader->ProdIdEntries = PhFinalArrayItems(&richHeaderEntryArray);

        //for (offset = currentAddress; offset < currentEnd; offset += sizeof(PRODITEM))
        //{
        //    PPRODITEM entry = (PPRODITEM)offset;
        //    ULONG prodid = ProdidFromDwProdid(entry->dwProdid ^ richHeaderKey);
        //    ULONG build = WBuildFromDwProdid(entry->dwProdid ^ richHeaderKey);
        //    ULONG count = entry->dwCount ^ richHeaderKey;
        //    dprintf("%x %x %x\n", prodid, build, count);
        //}

        return STATUS_SUCCESS;
    }

    return STATUS_FAIL_CHECK;
}

NTSTATUS PhGetMappedImageProdIdExtents(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PULONG ProdIdHeaderStart,
    _Out_ PULONG ProdIdHeaderEnd
    )
{
    PIMAGE_DOS_HEADER imageDosHeader = NULL;
    PIMAGE_NT_HEADERS imageNtHeader = NULL;
    PPRODITEM richHeaderStart = NULL;
    PPRODITEM richHeaderEnd = NULL;
    PPRODITEM richHeaderChecksum = NULL;
    ULONG ntHeadersOffset = ULONG_MAX;
    ULONG richHeaderKey = ULONG_MAX;
    ULONG richStartSignature = ULONG_MAX;
    ULONG richEndSignature = ULONG_MAX;
    ULONG richHeaderStartOffset = ULONG_MAX;
    ULONG richHeaderEndOffset = ULONG_MAX;

    imageDosHeader = (PIMAGE_DOS_HEADER)MappedImage->ViewBase;

    if (imageDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return STATUS_INVALID_IMAGE_NOT_MZ;

    ntHeadersOffset = (ULONG)imageDosHeader->e_lfanew;

    if (ntHeadersOffset == 0 || ntHeadersOffset >= LONG_MAX)
        return STATUS_INVALID_IMAGE_FORMAT;

    imageNtHeader = PTR_ADD_OFFSET(MappedImage->ViewBase, ntHeadersOffset);

    if (imageNtHeader->Signature == IMAGE_NT_SIGNATURE)
    {
        PBYTE startHeaderAddress = PTR_ADD_OFFSET(MappedImage->ViewBase, ntHeadersOffset);
        PBYTE endHeaderAddress = PTR_ADD_OFFSET(MappedImage->ViewBase, sizeof(IMAGE_DOS_HEADER));

        for (PBYTE i = startHeaderAddress; i >= endHeaderAddress; i -= sizeof(ULONG))
        {
            __try
            {
                if (*(PULONG)i == ProdIdTagStart)
                {
                    richHeaderEndOffset = PtrToUlong(PTR_SUB_OFFSET(i, MappedImage->ViewBase));
                    break;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
    }

    if (richHeaderEndOffset == ULONG_MAX)
        return STATUS_FAIL_CHECK;

    richHeaderChecksum = PTR_ADD_OFFSET(MappedImage->ViewBase, richHeaderEndOffset);

    __try
    {
        PhpMappedImageProbe(MappedImage, richHeaderChecksum, sizeof(PRODITEM));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    richHeaderKey = richHeaderChecksum->dwCount;
    richStartSignature = richHeaderChecksum->dwProdid;

    if (richHeaderKey && richStartSignature)
    {
        PBYTE startHeaderAddress = PTR_ADD_OFFSET(MappedImage->ViewBase, ntHeadersOffset);
        PBYTE endHeaderAddress = PTR_ADD_OFFSET(MappedImage->ViewBase, sizeof(IMAGE_DOS_HEADER));

        for (PBYTE i = startHeaderAddress; i >= endHeaderAddress; i -= sizeof(ULONG))
        {
            __try
            {
                if ((*(PULONG)i ^ richHeaderKey) == ProdIdTagEnd)
                {
                    richHeaderStartOffset = PtrToUlong(PTR_SUB_OFFSET(i, MappedImage->ViewBase));
                    break;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
    }

    if (richHeaderStartOffset == ULONG_MAX)
        return STATUS_FAIL_CHECK;

    richHeaderStart = PTR_ADD_OFFSET(MappedImage->ViewBase, richHeaderStartOffset);

    __try
    {
        PhpMappedImageProbe(MappedImage, richHeaderStart, sizeof(PRODITEM));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    richHeaderEnd = PTR_ADD_OFFSET(MappedImage->ViewBase, richHeaderEndOffset + sizeof(PRODITEM));

    __try
    {
        PhpMappedImageProbe(MappedImage, richHeaderEnd, sizeof(PRODITEM));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    richEndSignature = richHeaderStart->dwProdid ^ richHeaderKey;

    if (richStartSignature == ProdIdTagStart && richEndSignature == ProdIdTagEnd)
    {
        ULONG richHeaderTotalLength;
        ULONG currentCount = 0;
        PBYTE currentAddress;
        PBYTE currentEnd;
        PBYTE offset;

        currentAddress = PTR_ADD_OFFSET(richHeaderStart, 0);
        currentEnd = PTR_SUB_OFFSET(richHeaderEnd, 0);

        // Do a scan to determine how many entries there are.
        for (offset = currentAddress; offset < currentEnd; offset += sizeof(PRODITEM))
        {
            currentCount++;
        }

        // Calculate rich header and rich header padding. (Todo: Generate richHeaderKey and validate).
        richHeaderTotalLength = (richHeaderKey >> 5) % 3;
        richHeaderTotalLength += currentCount - 3; // remove 3 fixed checksum entries.
        richHeaderTotalLength *= sizeof(PRODITEM);
        richHeaderTotalLength += sizeof(PRODITEM) * 4; // add 3 fixed checksums and 1 null entry (0x20).
        richHeaderTotalLength += richHeaderStartOffset;

        // If we assert then the image has hidden data or the rich format changed.
        assert(richHeaderTotalLength == ntHeadersOffset);

        *ProdIdHeaderStart = richHeaderStartOffset;
        *ProdIdHeaderEnd = richHeaderTotalLength;
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS PhGetMappedImageDebug(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_DEBUG Debug
    )
{
    NTSTATUS status;
    ULONG currentCount;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_DEBUG_DIRECTORY debugDirectory;
    PH_ARRAY debugEntryArray;

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_DEBUG,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    debugDirectory = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!debugDirectory)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(MappedImage, debugDirectory, sizeof(IMAGE_DEBUG_DIRECTORY));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {   
        return GetExceptionCode();
    }

    currentCount = dataDirectory->Size / sizeof(IMAGE_DEBUG_DIRECTORY);

    // Allocate the number of debug entries.

    PhInitializeArray(&debugEntryArray, sizeof(PH_IMAGE_DEBUG_ENTRY), currentCount);

    // Add the debug entries into our buffer.

    for (ULONG i = 0; i < currentCount; i++)
    {
        PIMAGE_DEBUG_DIRECTORY item = PTR_ADD_OFFSET(debugDirectory, i * sizeof(IMAGE_DEBUG_DIRECTORY));

        __try
        {
            PhpMappedImageProbe(MappedImage, item, sizeof(IMAGE_DEBUG_DIRECTORY));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            break;
        }

        PH_IMAGE_DEBUG_ENTRY entry;

        entry.Characteristics = item->Characteristics;
        entry.TimeDateStamp = item->TimeDateStamp;
        entry.MajorVersion = item->MajorVersion;
        entry.MinorVersion = item->MinorVersion;
        entry.Type = item->Type;
        entry.SizeOfData = item->SizeOfData;
        entry.AddressOfRawData = item->AddressOfRawData;
        entry.PointerToRawData = item->PointerToRawData;

        PhAddItemArray(&debugEntryArray, &entry);
    }

    Debug->MappedImage = MappedImage;
    Debug->DataDirectory = dataDirectory;
    Debug->DebugDirectory = debugDirectory;
    Debug->NumberOfEntries = currentCount;
    Debug->DebugEntries = PhFinalArrayItems(&debugEntryArray);

    return status;
}

NTSTATUS PhGetMappedImageDebugEntryByType(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Type,
    _Out_opt_ ULONG* DataLength,
    _Out_opt_ PVOID* DataBuffer
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_DEBUG_DIRECTORY debugDirectory;
    PIMAGE_DEBUG_DIRECTORY debugEntry;
    ULONG currentCount;
    ULONG i;

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_DEBUG,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    debugDirectory = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!debugDirectory)
        return STATUS_UNSUCCESSFUL;

    __try
    {
        PhpMappedImageProbe(MappedImage, debugDirectory, sizeof(IMAGE_DEBUG_DIRECTORY));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {   
        return GetExceptionCode();
    }

    currentCount = dataDirectory->Size / sizeof(IMAGE_DEBUG_DIRECTORY);

    for (i = 0; i < currentCount; i++)
    {
        debugEntry = PTR_ADD_OFFSET(debugDirectory, i * sizeof(IMAGE_DEBUG_DIRECTORY));

        __try
        {
            PhpMappedImageProbe(MappedImage, debugEntry, sizeof(IMAGE_DEBUG_DIRECTORY));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            break;
        }

        if (debugEntry->Type == Type)
        {
            if (DataLength)
                *DataLength = debugEntry->SizeOfData;
            if (DataBuffer)
                *DataBuffer = PTR_ADD_OFFSET(MappedImage->ViewBase, debugEntry->PointerToRawData);
            return STATUS_SUCCESS;
        }
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS PhGetMappedImageEhCont32(
    _Out_ PPH_MAPPED_IMAGE_EH_CONT EhContConfig,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_LOAD_CONFIG_DIRECTORY32 config32;

    if (!NT_SUCCESS(status = PhGetMappedImageLoadConfig32(MappedImage, &config32)))
        return status;

    // Not every load configuration contains eh continuation
    if (!RTL_CONTAINS_FIELD(config32, config32->Size, GuardEHContinuationCount))
        return STATUS_INVALID_VIEW_SIZE;

    EhContConfig->EhContTable = PhMappedImageVaToVa(MappedImage, config32->GuardEHContinuationTable, NULL);
    EhContConfig->NumberOfEhContEntries = config32->GuardEHContinuationCount;

    // taken from from nt!RtlGuardRestoreContext
    EhContConfig->EntrySize = ((config32->GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK) >> IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT) + sizeof(ULONG);

    if (EhContConfig->EhContTable && EhContConfig->NumberOfEhContEntries)
    {
        __try
        {
            PhpMappedImageProbe(
                MappedImage,
                EhContConfig->EhContTable,
                (SIZE_T)(EhContConfig->NumberOfEhContEntries * EhContConfig->EntrySize)
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageEhCont64(
    _Out_ PPH_MAPPED_IMAGE_EH_CONT EhContConfig,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_LOAD_CONFIG_DIRECTORY64 config64;

    if (!NT_SUCCESS(status = PhGetMappedImageLoadConfig64(MappedImage, &config64)))
        return status;

    // Not every load configuration contains eh continuation
    if (!RTL_CONTAINS_FIELD(config64, config64->Size, GuardEHContinuationCount))
        return STATUS_INVALID_VIEW_SIZE;

    EhContConfig->EhContTable = PhMappedImageVaToVa(MappedImage, (ULONG)config64->GuardEHContinuationTable, NULL);
    EhContConfig->NumberOfEhContEntries = config64->GuardEHContinuationCount;

    // taken from from nt!RtlGuardRestoreContext
    EhContConfig->EntrySize = ((config64->GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK) >> IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT) + sizeof(ULONG);

    if (EhContConfig->EhContTable && EhContConfig->NumberOfEhContEntries)
    {
        __try
        {
            PhpMappedImageProbe(
                MappedImage,
                EhContConfig->EhContTable,
                (SIZE_T)(EhContConfig->NumberOfEhContEntries * EhContConfig->EntrySize)
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageEhCont(
    _Out_ PPH_MAPPED_IMAGE_EH_CONT EhContConfig,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        return PhGetMappedImageEhCont32(EhContConfig, MappedImage);
    }
    else
    {
        return PhGetMappedImageEhCont64(EhContConfig, MappedImage);
    }
}

_Success_(return)
BOOLEAN PhGetMappedImagePogoEntryByName(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PSTR Name,
    _Out_opt_ ULONG* DataLength,
    _Out_opt_ PVOID* DataBuffer
    )
{
    ULONG debugEntryLength;
    PIMAGE_DEBUG_POGO_SIGNATURE debugEntry;

    if (NT_SUCCESS(PhGetMappedImageDebugEntryByType(
        MappedImage,
        IMAGE_DEBUG_TYPE_POGO,
        &debugEntryLength,
        &debugEntry
        )))
    {
        PIMAGE_DEBUG_POGO_ENTRY debugPogoEntry;

        __try
        {
            PhpMappedImageProbe(MappedImage, debugEntry, sizeof(IMAGE_DEBUG_POGO_SIGNATURE));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return FALSE;
        }

        if (debugEntry->Signature != IMAGE_DEBUG_POGO_SIGNATURE_LTCG && debugEntry->Signature != IMAGE_DEBUG_POGO_SIGNATURE_PGU)
        {
            // The signature can be zero but still contain valid entries.
            if (!(debugEntry->Signature == 0 && debugEntryLength > sizeof(IMAGE_DEBUG_POGO_SIGNATURE)))
                return FALSE;
        }

        debugPogoEntry = PTR_ADD_OFFSET(debugEntry, sizeof(IMAGE_DEBUG_POGO_SIGNATURE));

        while ((ULONG_PTR)debugPogoEntry < (ULONG_PTR)PTR_ADD_OFFSET(debugEntry, debugEntryLength))
        {
            __try
            {
                PhpMappedImageProbe(MappedImage, debugPogoEntry, sizeof(IMAGE_DEBUG_POGO_ENTRY));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return FALSE;
            }

            if (!(debugPogoEntry->Rva && debugPogoEntry->Size))
                break;

            if (PhEqualBytesZ(debugPogoEntry->Name, Name, TRUE))
            {
                if (DataLength)
                {
                    *DataLength = debugPogoEntry->Size;
                }

                if (DataBuffer)
                {
                    *DataBuffer = PTR_ADD_OFFSET(MappedImage->ViewBase, debugPogoEntry->Rva);
                }

                return TRUE;
            }

            debugPogoEntry = PTR_ADD_OFFSET(debugPogoEntry, ALIGN_UP(UFIELD_OFFSET(IMAGE_DEBUG_POGO_ENTRY, Name) + strlen(debugPogoEntry->Name) + sizeof(ANSI_NULL), ULONG));
        }
    }

    return FALSE;
}

NTSTATUS PhGetMappedImagePogo(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_DEBUG_POGO PogoDebug
    )
{
    ULONG debugEntryCount = 0;
    ULONG debugEntryLength = 0;
    PIMAGE_DEBUG_POGO_SIGNATURE debugEntry;
    PIMAGE_DEBUG_POGO_ENTRY debugPogoEntry;
    PH_ARRAY pogoArray;

    if (!NT_SUCCESS(PhGetMappedImageDebugEntryByType(
        MappedImage,
        IMAGE_DEBUG_TYPE_POGO,
        &debugEntryLength,
        &debugEntry
        )))
    {
        return STATUS_UNSUCCESSFUL;
    }

    __try
    {
        PhpMappedImageProbe(MappedImage, debugEntry, sizeof(IMAGE_DEBUG_POGO_SIGNATURE));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    if (debugEntry->Signature != IMAGE_DEBUG_POGO_SIGNATURE_LTCG && debugEntry->Signature != IMAGE_DEBUG_POGO_SIGNATURE_PGU)
    {
        // The signature can be zero but still contain valid entries.
        if (!(debugEntry->Signature == 0 && debugEntryLength > sizeof(IMAGE_DEBUG_POGO_SIGNATURE)))
            return STATUS_UNSUCCESSFUL;
    }

    // Skip the signature.

    debugPogoEntry = PTR_ADD_OFFSET(debugEntry, sizeof(IMAGE_DEBUG_POGO_SIGNATURE));

    // Do a scan to determine how many entries there are.

    while ((ULONG_PTR)debugPogoEntry < (ULONG_PTR)PTR_ADD_OFFSET(debugEntry, debugEntryLength))
    {
        __try
        {
            PhpMappedImageProbe(MappedImage, debugPogoEntry, sizeof(IMAGE_DEBUG_POGO_ENTRY));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }

        debugEntryCount++;

        debugPogoEntry = PTR_ADD_OFFSET(debugPogoEntry, ALIGN_UP(UFIELD_OFFSET(IMAGE_DEBUG_POGO_ENTRY, Name) + strlen(debugPogoEntry->Name) + sizeof(ANSI_NULL), ULONG));
    }

    // Allocate the number of pogo entries.

    PhInitializeArray(&pogoArray, sizeof(PH_IMAGE_DEBUG_POGO_ENTRY), debugEntryCount);

    // Add the debug entries into our buffer.

    debugPogoEntry = PTR_ADD_OFFSET(debugEntry, sizeof(IMAGE_DEBUG_POGO_SIGNATURE));

    while ((ULONG_PTR)debugPogoEntry < (ULONG_PTR)PTR_ADD_OFFSET(debugEntry, debugEntryLength))
    {
        if (!(debugPogoEntry->Rva && debugPogoEntry->Size))
            break;

        {
            PH_IMAGE_DEBUG_POGO_ENTRY entry;

            memset(entry.Name, ANSI_NULL, sizeof(entry.Name));
            entry.Rva = debugPogoEntry->Rva;
            entry.Size = debugPogoEntry->Size;
            entry.Data = PhMappedImageRvaToVa(MappedImage, debugPogoEntry->Rva, NULL);

            PhCopyStringZFromBytes(
                debugPogoEntry->Name,
                -1,
                entry.Name,
                RTL_NUMBER_OF(entry.Name),
                NULL
                );

            PhAddItemArray(&pogoArray, &entry);
        }

        debugPogoEntry = PTR_ADD_OFFSET(debugPogoEntry, ALIGN_UP(UFIELD_OFFSET(IMAGE_DEBUG_POGO_ENTRY, Name) + strlen(debugPogoEntry->Name) + sizeof(ANSI_NULL), ULONG));
    }

    PogoDebug->PogoDirectory = debugEntry;
    PogoDebug->NumberOfEntries = debugEntryCount;
    PogoDebug->PogoEntries = PhFinalArrayItems(&pogoArray);

    return STATUS_SUCCESS;
}

NTSTATUS PhGetMappedImageRelocations(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_RELOC Relocations
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_BASE_RELOCATION relocationDirectory;
    PVOID relocationDirectoryEnd;
    PH_ARRAY relocationArray;
    ULONG relocationTotal = 0;
    ULONG relocationIndex = 0;

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_BASERELOC,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    relocationDirectory = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!relocationDirectory)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(MappedImage, relocationDirectory, sizeof(IMAGE_BASE_RELOCATION));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {   
        return GetExceptionCode();
    }

    Relocations->MappedImage = MappedImage;
    Relocations->DataDirectory = dataDirectory;
    Relocations->FirstRelocationDirectory = relocationDirectory;

    //
    // Do a scan to determine how many entries there are. And validate the
    // blocks are within the mapping.
    //

    relocationDirectory = Relocations->FirstRelocationDirectory;
    relocationDirectoryEnd = PTR_ADD_OFFSET(relocationDirectory, dataDirectory->Size);

    while ((ULONG_PTR)relocationDirectory < (ULONG_PTR)relocationDirectoryEnd)
    {
        __try
        {
            PhpMappedImageProbe(MappedImage, relocationDirectory, sizeof(IMAGE_BASE_RELOCATION));
            PhpMappedImageProbe(MappedImage, relocationDirectory, relocationDirectory->SizeOfBlock);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }

        if (relocationDirectory->SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION))
        {
            //
            // Prevent runaway.
            //
            return STATUS_INVALID_IMAGE_FORMAT;
        }

        relocationTotal += (relocationDirectory->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(IMAGE_BASE_RELOCATION_ENTRY);
        relocationDirectory = PTR_ADD_OFFSET(relocationDirectory, relocationDirectory->SizeOfBlock);
    }

    // Allocate the number of relocation entries.

    PhInitializeArray(&relocationArray, sizeof(PH_IMAGE_RELOC_ENTRY), relocationTotal);

    // Add the relocation entries into our buffer.

    relocationDirectory = Relocations->FirstRelocationDirectory;

    while ((ULONG_PTR)relocationDirectory < (ULONG_PTR)relocationDirectoryEnd)
    {
        ULONG relocationCount;
        PIMAGE_BASE_RELOCATION_ENTRY relocations;

        relocationCount = (relocationDirectory->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(IMAGE_BASE_RELOCATION_ENTRY);
        relocations = PTR_ADD_OFFSET(relocationDirectory, RTL_SIZEOF_THROUGH_FIELD(IMAGE_BASE_RELOCATION, SizeOfBlock));

        for (ULONG i = 0; i < relocationCount; i++)
        {
            PH_IMAGE_RELOC_ENTRY entry;

            entry.BlockIndex = relocationIndex;
            entry.Type = relocations[i].Type;
            entry.Offset = relocations[i].Offset;
            entry.BlockRva = relocationDirectory->VirtualAddress;
            if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            {
                entry.ImageBaseVa = PTR_ADD_OFFSET(MappedImage->NtHeaders->OptionalHeader.ImageBase,
                                                   (SIZE_T)entry.BlockRva + entry.Offset);
            }
            else if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                entry.ImageBaseVa = PTR_ADD_OFFSET(MappedImage->NtHeaders32->OptionalHeader.ImageBase,
                                                   (SIZE_T)entry.BlockRva + entry.Offset);
            }
            entry.MappedImageVa = PhMappedImageRvaToVa(MappedImage,
                                                       entry.BlockRva + entry.Offset,
                                                       NULL);
            PhAddItemArray(&relocationArray, &entry);
        }

        relocationDirectory = PTR_ADD_OFFSET(relocationDirectory, relocationDirectory->SizeOfBlock);
        relocationIndex++;
    }

    Relocations->NumberOfEntries = (ULONG)relocationArray.Count;
    Relocations->RelocationEntries = PhFinalArrayItems(&relocationArray);

    return status;
}

VOID PhFreeMappedImageRelocations(
    _In_ PPH_MAPPED_IMAGE_RELOC Relocations
    )
{
    if (Relocations && Relocations->RelocationEntries)
    {
        PhFree(Relocations->RelocationEntries);
        Relocations->RelocationEntries = NULL;
        Relocations->NumberOfEntries = 0;
    }
}

NTSTATUS PhGetMappedImageExceptions(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_EXCEPTIONS Exceptions
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PVOID exceptionDirectory;
    PH_ARRAY exceptionArray;
    ULONG imageMachine;
    ULONG exceptionTotal = 0;
    ULONG exceptionEntrySize = 0;

    if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        imageMachine = MappedImage->NtHeaders32->FileHeader.Machine;
    else
        imageMachine = MappedImage->NtHeaders->FileHeader.Machine;

    // 32bit images require special handling for the SEH table.
    switch (imageMachine)
    {
    case IMAGE_FILE_MACHINE_I386:
        {
            PIMAGE_LOAD_CONFIG_DIRECTORY32 config32;
            PULONG exceptionHandlerTable = NULL;

            status = PhGetMappedImageLoadConfig32(MappedImage, &config32);

            if (!NT_SUCCESS(status))
                return status;

            if (config32->SEHandlerTable)
            {
                exceptionTotal = config32->SEHandlerCount;
                exceptionHandlerTable = PhMappedImageVaToVa(MappedImage, config32->SEHandlerTable, NULL);
            }

            if (!exceptionHandlerTable)
                return STATUS_UNSUCCESSFUL;

            __try
            {
                PhpMappedImageProbe(MappedImage, exceptionHandlerTable, exceptionTotal * sizeof(ULONG));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }

            // Allocate the number of exception entries.

            PhInitializeArray(&exceptionArray, sizeof(ULONG), exceptionTotal);

            // Add the exception entries into our buffer.

            for (ULONG i = 0; i < exceptionTotal; i++)
            {
                ULONG rva = *(PULONG)PTR_ADD_OFFSET(exceptionHandlerTable, i * sizeof(ULONG));

                PhAddItemArray(&exceptionArray, &rva);
            }

            Exceptions->MappedImage = MappedImage;
            Exceptions->DataDirectory = NULL;
            Exceptions->ExceptionDirectory = NULL;
            Exceptions->NumberOfEntries = (ULONG)exceptionArray.Count;
            Exceptions->ExceptionEntries = PhFinalArrayItems(&exceptionArray);

            return STATUS_SUCCESS;
        }
        break;
    }

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_EXCEPTION,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    exceptionDirectory = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!exceptionDirectory)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(MappedImage, exceptionDirectory, dataDirectory->Size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    Exceptions->MappedImage = MappedImage;
    Exceptions->DataDirectory = dataDirectory;
    Exceptions->ExceptionDirectory = exceptionDirectory;

    switch (imageMachine)
    {
    case IMAGE_FILE_MACHINE_AMD64:
    case IMAGE_FILE_MACHINE_IA64:
        {
            exceptionEntrySize = sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);
            exceptionTotal = dataDirectory->Size / exceptionEntrySize;
        }
        break;
    case IMAGE_FILE_MACHINE_ARMNT:
        {
            exceptionEntrySize = sizeof(IMAGE_ARM_RUNTIME_FUNCTION_ENTRY);
            exceptionTotal = dataDirectory->Size / exceptionEntrySize;
        }
        break;
    case IMAGE_FILE_MACHINE_ARM64:
        {
            exceptionEntrySize = sizeof(IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY);
            exceptionTotal = dataDirectory->Size / exceptionEntrySize;
        }
        break;
    }

    // Allocate the number of exception entries.

    PhInitializeArray(&exceptionArray, exceptionEntrySize, exceptionTotal);

    // Add the exception entries into our buffer.

    for (ULONG i = 0; i < exceptionTotal; i++)
    {
        PVOID entry = PTR_ADD_OFFSET(exceptionDirectory, UInt32Mul32To64(i, exceptionEntrySize));

        PhAddItemArray(&exceptionArray, entry);
    }

    Exceptions->NumberOfEntries = (ULONG)exceptionArray.Count;
    Exceptions->ExceptionEntries = PhFinalArrayItems(&exceptionArray);

    return status;
}
