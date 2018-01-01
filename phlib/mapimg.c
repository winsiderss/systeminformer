/*
 * Process Hacker -
 *   mapped image
 *
 * Copyright (C) 2010 wj32
 * Copyright (C) 2017 dmex
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

/*
 * This file contains functions to load and retrieve information for image files (exe, dll). The
 * file format for image files is explained in the PE/COFF specification located at:
 *
 * http://www.microsoft.com/whdc/system/platform/firmware/PECOFF.mspx
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
            FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader)
            );
        PhpMappedImageProbe(
            MappedImage,
            MappedImage->NtHeaders,
            FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) +
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
    _In_ BOOLEAN ReadOnly,
    _Out_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PVOID viewBase;
    SIZE_T size;

    status = PhMapViewOfEntireFile(
        FileName,
        FileHandle,
        ReadOnly,
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
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _In_ BOOLEAN ReadOnly,
    _Out_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PVOID viewBase;
    SIZE_T size;

    status = PhMapViewOfEntireFile(
        FileName,
        FileHandle,
        ReadOnly,
        &viewBase,
        &size
        );

    if (NT_SUCCESS(status))
    {
        PUSHORT imageHeaderSignature = viewBase;

        __try
        {
            PhProbeAddress(imageHeaderSignature, sizeof(USHORT), viewBase, size, 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }

        MappedImage->Signature = *imageHeaderSignature;

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
    _In_ BOOLEAN ReadOnly,
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
            ((FILE_READ_ATTRIBUTES | FILE_READ_DATA) |
            (!ReadOnly ? (FILE_APPEND_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA) : 0)) | SYNCHRONIZE,
            0,
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
        SECTION_ALL_ACCESS,
        NULL,
        &size,
        ReadOnly ? PAGE_READONLY : PAGE_READWRITE,
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
        ReadOnly ? PAGE_READONLY : PAGE_READWRITE
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

PVOID PhMappedImageRvaToVa(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva,
    _Out_opt_ PIMAGE_SECTION_HEADER *Section
    )
{
    PIMAGE_SECTION_HEADER section;

    section = PhMappedImageRvaToSection(MappedImage, Rva);

    if (!section)
        return NULL;

    if (Section)
        *Section = section;

    return PTR_ADD_OFFSET(
        MappedImage->ViewBase, 
        (Rva - section->VirtualAddress) +
        section->PointerToRawData
        );
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

        optionalHeader = (PIMAGE_OPTIONAL_HEADER32)&MappedImage->NtHeaders->OptionalHeader;

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
    NTSTATUS status;
    IMAGE_DOS_HEADER dosHeader;
    ULONG ntHeadersOffset;
    IMAGE_NT_HEADERS32 ntHeaders;
    ULONG ntHeadersSize;

    RemoteMappedImage->ViewBase = ViewBase;

    status = NtReadVirtualMemory(
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

    status = NtReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(ViewBase, ntHeadersOffset),
        &ntHeaders,
        sizeof(IMAGE_NT_HEADERS32),
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
    ntHeadersSize = FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) +
        ntHeaders.FileHeader.SizeOfOptionalHeader +
        RemoteMappedImage->NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

    if (ntHeadersSize > 1024 * 1024) // 1 MB
        return STATUS_INVALID_IMAGE_FORMAT;

    RemoteMappedImage->NtHeaders = PhAllocate(ntHeadersSize);

    status = NtReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(ViewBase, ntHeadersOffset),
        RemoteMappedImage->NtHeaders,
        ntHeadersSize,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        PhFree(RemoteMappedImage->NtHeaders);
        return status;
    }

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

NTSTATUS PhGetMappedImageExports(
    _Out_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_EXPORT_DIRECTORY exportDirectory;

    Exports->MappedImage = MappedImage;

    // Get a pointer to the export directory.

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &Exports->DataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    exportDirectory = PhMappedImageRvaToVa(
        MappedImage,
        Exports->DataDirectory->VirtualAddress,
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

    if (
        !Exports->AddressTable ||
        !Exports->NamePointerTable ||
        !Exports->OrdinalTable
        )
        return STATUS_INVALID_PARAMETER;

    __try
    {
        PhpMappedImageProbe(
            MappedImage,
            Exports->AddressTable,
            exportDirectory->NumberOfFunctions * sizeof(ULONG)
            );
        PhpMappedImageProbe(
            MappedImage,
            Exports->NamePointerTable,
            exportDirectory->NumberOfNames * sizeof(ULONG)
            );
        PhpMappedImageProbe(
            MappedImage,
            Exports->OrdinalTable,  // ordinal list for named exports
            exportDirectory->NumberOfNames * sizeof(USHORT)
            );
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

    // look into named exports ordinal list.
    for (nameIndex = 0; nameIndex < Exports->ExportDirectory->NumberOfNames; nameIndex++)
    {
        if (Index == Exports->OrdinalTable[nameIndex])
        {
            exportByName = TRUE;
            break;
        }
    }


    if (exportByName)
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
    }
    else
    {
        Entry->Name = NULL;
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

        if (index == -1)
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

        if (index == -1)
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
        return -1;

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
            return -1;

        // TODO: Probe the name.

        comparison = strcmp(Name, name);

        if (comparison == 0)
            return i;
        else if (comparison < 0)
            high = i - 1;
        else
            low = i + 1;
    } while (low <= high);

    return -1;
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

    Imports->MappedImage = MappedImage;
    Imports->Flags = 0;

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

        ImportDll->Name = PhMappedImageRvaToVa(
            ImportDll->MappedImage,
            ImportDll->DelayDescriptor->DllNameRVA,
            NULL
            );

        if (!ImportDll->Name)
            return STATUS_INVALID_PARAMETER;

        // TODO: Probe the name.

        ImportDll->LookupTable = PhMappedImageRvaToVa(
            ImportDll->MappedImage,
            ImportDll->DelayDescriptor->ImportNameTableRVA,
            NULL
            );
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
            importByName = PhMappedImageRvaToVa(
                ImportDll->MappedImage,
                entry.u1.AddressOfData,
                NULL
                );
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
            importByName = PhMappedImageRvaToVa(
                ImportDll->MappedImage,
                (ULONG)entry.u1.AddressOfData,
                NULL
                );
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
    CfgConfig->EntrySize = sizeof(FIELD_OFFSET(IMAGE_CFG_ENTRY, Rva)) +
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
    CfgConfig->GuardFunctionTable = PhMappedImageRvaToVa(
        MappedImage,
        (ULONG)(config64->GuardCFFunctionTable - MappedImage->NtHeaders->OptionalHeader.ImageBase),
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
        CfgConfig->GuardAdressIatTable = PhMappedImageRvaToVa(
            MappedImage,
            (ULONG)(config64->GuardAddressTakenIatEntryTable - MappedImage->NtHeaders->OptionalHeader.ImageBase),
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
        CfgConfig->GuardLongJumpTable = PhMappedImageRvaToVa(
            MappedImage,
            (ULONG)(config64->GuardLongJumpTargetTable - MappedImage->NtHeaders->OptionalHeader.ImageBase),
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
    CfgConfig->EntrySize = sizeof(FIELD_OFFSET(IMAGE_CFG_ENTRY, Rva)) +
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
    CfgConfig->GuardFunctionTable = PhMappedImageRvaToVa(
        MappedImage,
        config32->GuardCFFunctionTable - MappedImage->NtHeaders32->OptionalHeader.ImageBase,
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
        CfgConfig->GuardAdressIatTable = PhMappedImageRvaToVa(
            MappedImage,
            config32->GuardAddressTakenIatEntryTable - MappedImage->NtHeaders32->OptionalHeader.ImageBase,
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
        CfgConfig->GuardLongJumpTable = PhMappedImageRvaToVa(
            MappedImage,
            config32->GuardLongJumpTargetTable - MappedImage->NtHeaders32->OptionalHeader.ImageBase,
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
    case ControlFlowGuardtakenIatEntry:
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
    if (CfgConfig->EntrySize > sizeof(FIELD_OFFSET(IMAGE_CFG_ENTRY, Rva)))
    {
        Entry->SuppressedCall = cfgMappedEntry->SuppressedCall;
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
    PIMAGE_RESOURCE_DIRECTORY resourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY nameDirectory;
    PIMAGE_RESOURCE_DIRECTORY languageDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceType;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceName;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceLanguage;
    ULONG resourceCount = 0;
    ULONG resourceIndex = 0;
    ULONG resourceTypeCount;
    ULONG resourceNameCount;
    ULONG resourceLanguageCount;

    // Get a pointer to the resource directory.

    status = PhGetMappedImageDataEntry(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_RESOURCE,
        &Resources->DataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    resourceDirectory = PhMappedImageRvaToVa(
        MappedImage,
        Resources->DataDirectory->VirtualAddress,
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

    Resources->ResourceDirectory = resourceDirectory;

    // NOTE: We can't use LdrEnumResources here because we're using an image mapped with SEC_COMMIT.

    // Do a scan to determine how many resources there are.

    resourceType = PTR_ADD_OFFSET(resourceDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
    resourceTypeCount = resourceDirectory->NumberOfNamedEntries + resourceDirectory->NumberOfIdEntries;

    for (ULONG i = 0; i < resourceTypeCount; ++i, ++resourceType)
    {
        if (!resourceType->DataIsDirectory)
            return STATUS_RESOURCE_TYPE_NOT_FOUND;

        nameDirectory = PTR_ADD_OFFSET(resourceDirectory, resourceType->OffsetToDirectory);
        resourceName = PTR_ADD_OFFSET(nameDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
        resourceNameCount = nameDirectory->NumberOfNamedEntries + nameDirectory->NumberOfIdEntries;

        for (ULONG j = 0; j < resourceNameCount; ++j, ++resourceName)
        {
            if (!resourceName->DataIsDirectory)
                return STATUS_RESOURCE_NAME_NOT_FOUND;

            languageDirectory = PTR_ADD_OFFSET(resourceDirectory, resourceName->OffsetToDirectory);
            resourceLanguage = PTR_ADD_OFFSET(languageDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
            resourceLanguageCount = languageDirectory->NumberOfNamedEntries + languageDirectory->NumberOfIdEntries;

            for (ULONG k = 0; k < resourceLanguageCount; ++k, ++resourceLanguage)
            {
                if (resourceLanguage->DataIsDirectory)
                    return STATUS_RESOURCE_DATA_NOT_FOUND;

                resourceCount++;
            }
        }
    }

    if (resourceCount == 0)
        return STATUS_INVALID_IMAGE_FORMAT;

    // Allocate the number of resources.

    Resources->NumberOfEntries = resourceCount;
    Resources->ResourceEntries = PhAllocate(sizeof(PH_IMAGE_RESOURCE_ENTRY) * resourceCount);
    memset(Resources->ResourceEntries, 0, sizeof(PH_IMAGE_RESOURCE_ENTRY) * resourceCount);

    // Enumerate the resources adding them into our buffer.

    resourceType = PTR_ADD_OFFSET(resourceDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
    resourceTypeCount = resourceDirectory->NumberOfNamedEntries + resourceDirectory->NumberOfIdEntries;

    for (ULONG i = 0; i < resourceTypeCount; ++i, ++resourceType)
    {
        if (!resourceType->DataIsDirectory)
            goto CleanupExit;

        nameDirectory = PTR_ADD_OFFSET(resourceDirectory, resourceType->OffsetToDirectory);
        resourceName = PTR_ADD_OFFSET(nameDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
        resourceNameCount = nameDirectory->NumberOfNamedEntries + nameDirectory->NumberOfIdEntries;

        for (ULONG j = 0; j < resourceNameCount; ++j, ++resourceName)
        {
            if (!resourceName->DataIsDirectory)
                goto CleanupExit;

            languageDirectory = PTR_ADD_OFFSET(resourceDirectory, resourceName->OffsetToDirectory);
            resourceLanguage = PTR_ADD_OFFSET(languageDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
            resourceLanguageCount = languageDirectory->NumberOfNamedEntries + languageDirectory->NumberOfIdEntries;

            for (ULONG k = 0; k < resourceLanguageCount; ++k, ++resourceLanguage)
            {
                PIMAGE_RESOURCE_DATA_ENTRY resourceData;

                if (resourceLanguage->DataIsDirectory)
                    goto CleanupExit;

                resourceData = PTR_ADD_OFFSET(resourceDirectory, resourceLanguage->OffsetToData);

                Resources->ResourceEntries[resourceIndex].Type = NAME_FROM_RESOURCE_ENTRY(resourceDirectory, resourceType);
                Resources->ResourceEntries[resourceIndex].Name = NAME_FROM_RESOURCE_ENTRY(resourceDirectory, resourceName);
                Resources->ResourceEntries[resourceIndex].Language = NAME_FROM_RESOURCE_ENTRY(resourceDirectory, resourceLanguage);
                Resources->ResourceEntries[resourceIndex].Data = PTR_ADD_OFFSET(MappedImage->ViewBase, resourceData->OffsetToData);
                Resources->ResourceEntries[resourceIndex++].Size = resourceData->Size;
            }
        }
    }

CleanupExit:
    return status;
}
    