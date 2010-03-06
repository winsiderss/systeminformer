/*
 * Process Hacker - 
 *   mapped image
 * 
 * Copyright (C) 2010 wj32
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

#include <ph.h>

VOID PhpMappedImageProbe(
    __in PPH_MAPPED_IMAGE MappedImage,
    __in PVOID Address,
    __in SIZE_T Length
    );

ULONG PhpLookupMappedImageExportName(
    __in PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in PSTR Name
    );

NTSTATUS PhInitializeMappedImage(
    __out PPH_MAPPED_IMAGE MappedImage,
    __in PVOID ViewBase,
    __in SIZE_T Size
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

    MappedImage->Sections = (PIMAGE_SECTION_HEADER)(
        ((PCHAR)&MappedImage->NtHeaders->OptionalHeader) +
        MappedImage->NtHeaders->FileHeader.SizeOfOptionalHeader
        );

    return STATUS_SUCCESS;
}

NTSTATUS PhLoadMappedImage(
    __in_opt PWSTR FileName,
    __in_opt HANDLE FileHandle,
    __in BOOLEAN ReadOnly,
    __out PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    BOOLEAN openedFile = FALSE;
    LARGE_INTEGER size;
    HANDLE sectionHandle = NULL;

    if (!FileName && !FileHandle)
        return STATUS_INVALID_PARAMETER_MIX;

    // Open the file if we weren't supplied a file handle.
    if (!FileHandle)
    {
        FileHandle = CreateFile(
            FileName,
            (FILE_EXECUTE | FILE_READ_ATTRIBUTES | FILE_READ_DATA) |
            (!ReadOnly ? (FILE_APPEND_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA) : 0),
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

        if (FileHandle == INVALID_HANDLE_VALUE)
            return NTSTATUS_FROM_WIN32(GetLastError());

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
        ReadOnly ? PAGE_EXECUTE_READ : PAGE_EXECUTE_READWRITE,
        SEC_COMMIT,
        FileHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Map the section.

    MappedImage->Size = (SIZE_T)size.QuadPart;
    MappedImage->ViewBase = NULL;

    status = NtMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &MappedImage->ViewBase,
        0,
        0,
        NULL,
        &MappedImage->Size,
        ViewShare,
        0,
        ReadOnly ? PAGE_EXECUTE_READ : PAGE_EXECUTE_READWRITE
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Initialize the mapped file.

    status = PhInitializeMappedImage(
        MappedImage,
        MappedImage->ViewBase,
        MappedImage->Size
        );

    if (!NT_SUCCESS(status))
    {
        NtUnmapViewOfSection(NtCurrentProcess(), MappedImage->ViewBase);
    }

CleanupExit:
    if (sectionHandle)
        NtClose(sectionHandle);
    if (openedFile)
        NtClose(FileHandle);

    return status;
}

NTSTATUS PhUnloadMappedImage(
    __inout PPH_MAPPED_IMAGE MappedImage
    )
{
    return NtUnmapViewOfSection(
        NtCurrentProcess(),
        MappedImage->ViewBase
        );
}

VOID PhpMappedImageProbe(
    __in PPH_MAPPED_IMAGE MappedImage,
    __in PVOID Address,
    __in SIZE_T Length
    )
{
    PhProbeAddress(Address, Length, MappedImage->ViewBase, MappedImage->Size, 1);
}

PIMAGE_SECTION_HEADER PhMappedImageRvaToSection(
    __in PPH_MAPPED_IMAGE MappedImage,
    __in ULONG Rva
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
    __in PPH_MAPPED_IMAGE MappedImage,
    __in ULONG Rva,
    __out_opt PIMAGE_SECTION_HEADER *Section
    )
{
    PIMAGE_SECTION_HEADER section;

    section = PhMappedImageRvaToSection(MappedImage, Rva);

    if (!section)
        return NULL;

    if (Section)
        *Section = section;

    return (PVOID)(
        (ULONG_PTR)MappedImage->ViewBase +
        (Rva - section->VirtualAddress) +
        section->PointerToRawData
        );
}

BOOLEAN PhGetMappedImageSectionName(
    __in PIMAGE_SECTION_HEADER Section,
    __out_ecount_z_opt(Count) PSTR Buffer,
    __in ULONG Count,
    __out_opt PULONG ReturnCount
    )
{
    ULONG i = 0;
    BOOLEAN copied;

    // Determine the length of the section name.

    while (i < IMAGE_SIZEOF_SHORT_NAME && Section->Name[i])
        i++;

    // Copy the name if there is enough room.

    if (Buffer && Count >= i + 1) // need one byte for null terminator
    {
        memcpy(Buffer, Section->Name, i);
        Buffer[i] = 0;
        copied = TRUE;
    }
    else
    {
        copied = FALSE;
    }

    if (ReturnCount)
        *ReturnCount = i + 1;

    return copied;
}

NTSTATUS PhGetMappedImageDataEntry(
    __in PPH_MAPPED_IMAGE MappedImage,
    __in ULONG Index,
    __out PIMAGE_DATA_DIRECTORY *Entry
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
    __in PPH_MAPPED_IMAGE MappedImage,
    __in USHORT Magic,
    __in ULONG ProbeLength,
    __out PPVOID LoadConfig
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
    __in PPH_MAPPED_IMAGE MappedImage,
    __out PIMAGE_LOAD_CONFIG_DIRECTORY32 *LoadConfig
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
    __in PPH_MAPPED_IMAGE MappedImage,
    __out PIMAGE_LOAD_CONFIG_DIRECTORY64 *LoadConfig
    )
{
    return PhpGetMappedImageLoadConfig(
        MappedImage,
        IMAGE_NT_OPTIONAL_HDR64_MAGIC,
        sizeof(IMAGE_LOAD_CONFIG_DIRECTORY64),
        LoadConfig
        );
}

NTSTATUS PhInitializeMappedImageExports(
    __out PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in PPH_MAPPED_IMAGE MappedImage
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
            Exports->OrdinalTable,
            exportDirectory->NumberOfFunctions * sizeof(USHORT)
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
    __in PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in ULONG Index,
    __out PPH_MAPPED_IMAGE_EXPORT_ENTRY Entry
    )
{
    PSTR name;

    if (Index >= Exports->ExportDirectory->NumberOfFunctions)
        return STATUS_PROCEDURE_NOT_FOUND;

    Entry->Ordinal = Exports->OrdinalTable[Index] + (USHORT)Exports->ExportDirectory->Base;

    if (Index < Exports->ExportDirectory->NumberOfNames)
    {
        name = PhMappedImageRvaToVa(
            Exports->MappedImage,
            Exports->NamePointerTable[Index],
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
    __in PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in_opt PSTR Name,
    __in_opt USHORT Ordinal,
    __out PPH_MAPPED_IMAGE_EXPORT_FUNCTION Function
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
        Function->Function = PhMappedImageRvaToVa(
            Exports->MappedImage,
            rva,
            NULL
            );
        Function->ForwardedName = NULL;
    }

    return STATUS_SUCCESS;
}

ULONG PhpLookupMappedImageExportName(
    __in PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in PSTR Name
    )
{
    ULONG low = 0;
    ULONG high = Exports->ExportDirectory->NumberOfNames - 1;

    while (low <= high)
    {
        ULONG i;
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
        else if (comparison > 0)
            low = i + 1;
        else
            high = i - 1;
    }

    return -1;
}

NTSTATUS PhInitializeMappedImageImports(
    __out PPH_MAPPED_IMAGE_IMPORTS Imports,
    __in PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_IMPORT_DESCRIPTOR descriptor;
    ULONG i;

    Imports->MappedImage = MappedImage;

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
    __in PPH_MAPPED_IMAGE_IMPORTS Imports,
    __in ULONG Index,
    __out PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll
    )
{
    ULONG i;

    if (Index >= Imports->NumberOfDlls)
        return STATUS_INVALID_PARAMETER_2;

    ImportDll->MappedImage = Imports->MappedImage;
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

    if (!ImportDll->LookupTable)
        return STATUS_INVALID_PARAMETER;

    // Do a scan to determine how many entries there are.

    i = 0;

    if (ImportDll->MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PULONG entry;

        entry = (PULONG)ImportDll->LookupTable;

        __try
        {
            while (TRUE)
            {
                PhpMappedImageProbe(
                    ImportDll->MappedImage,
                    entry,
                    sizeof(ULONG)
                    );

                if (*entry == 0)
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
        PULONG64 entry;

        entry = (PULONG64)ImportDll->LookupTable;

        __try
        {
            while (TRUE)
            {
                PhpMappedImageProbe(
                    ImportDll->MappedImage,
                    entry,
                    sizeof(ULONG64)
                    );

                if (*entry == 0)
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
    __in PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll,
    __in ULONG Index,
    __out PPH_MAPPED_IMAGE_IMPORT_ENTRY Entry
    )
{
    PIMAGE_IMPORT_BY_NAME importByName;

    if (Index >= ImportDll->NumberOfEntries)
        return STATUS_INVALID_PARAMETER_2;

    if (ImportDll->MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        ULONG entry;

        entry = ((PULONG)ImportDll->LookupTable)[Index];

        // Is this entry using an ordinal?
        if (entry & 0x80000000)
        {
            Entry->Ordinal = (USHORT)(entry & 0xffff);
            Entry->NameHint = 0;
            Entry->Name = NULL;

            return STATUS_SUCCESS;
        }
        else
        {
            importByName = PhMappedImageRvaToVa(
                ImportDll->MappedImage,
                entry,
                NULL
                );
        }
    }
    else if (ImportDll->MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        ULONG64 entry;

        entry = ((PULONG64)ImportDll->LookupTable)[Index];

        // Is this entry using an ordinal?
        if (entry & 0x8000000000000000)
        {
            Entry->Ordinal = (USHORT)(entry & 0xffff);
            Entry->NameHint = 0;
            Entry->Name = NULL;

            return STATUS_SUCCESS;
        }
        else
        {
            importByName = PhMappedImageRvaToVa(
                ImportDll->MappedImage,
                (ULONG)entry,
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
        PhpMappedImageProbe(
            ImportDll->MappedImage,
            importByName,
            sizeof(IMAGE_IMPORT_BY_NAME)
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    Entry->Ordinal = 0;
    Entry->NameHint = importByName->Hint;
    Entry->Name = (PSTR)importByName->Name;

    // TODO: Probe the name.

    return STATUS_SUCCESS;
}
