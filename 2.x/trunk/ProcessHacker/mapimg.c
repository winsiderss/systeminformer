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

USHORT PhpExportIndexToOrdinal(
    __in PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in ULONG Index
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
    PCHAR start;
    ULONG ntHeadersOffset;

    MappedImage->ViewBase = ViewBase;
    MappedImage->Size = Size;

    __try
    {
        PhpMappedImageProbe(MappedImage, ViewBase, 0x3c + sizeof(ULONG));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    // Check the initial MZ.

    start = ViewBase;

    if (start[0] != 'M' || start[1] != 'Z')
        return STATUS_INVALID_IMAGE_NOT_MZ;

    // Get a pointer to the NT headers.

    ntHeadersOffset = *(PULONG)(start + 0x3c);

    if (ntHeadersOffset == 0)
        return STATUS_INVALID_IMAGE_FORMAT;
    if (ntHeadersOffset >= 0x10000000 || ntHeadersOffset >= Size)
        return STATUS_INVALID_IMAGE_FORMAT;

    MappedImage->NtHeaders = (PIMAGE_NT_HEADERS)(start + ntHeadersOffset);

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

    // Get a pointer to the first section.

    MappedImage->NumberOfSections = MappedImage->NtHeaders->FileHeader.NumberOfSections;

    MappedImage->Sections = (PIMAGE_SECTION_HEADER)(
        ((PCHAR)&MappedImage->NtHeaders->OptionalHeader) +
        MappedImage->NtHeaders->FileHeader.SizeOfOptionalHeader
        );

    // Verify the magic.

    MappedImage->Magic = MappedImage->NtHeaders->OptionalHeader.Magic;

    if (
        MappedImage->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
        MappedImage->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC
        )
        return STATUS_INVALID_IMAGE_FORMAT;

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
            return STATUS_UNSUCCESSFUL;

        openedFile = TRUE;
    }

    // Get the file size and create the section.

    status = PhGetFileSize(FileHandle, &size);

    if (!NT_SUCCESS(status))
        goto ExitAndCleanup;

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
        goto ExitAndCleanup;

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
        goto ExitAndCleanup;

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

ExitAndCleanup:
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
    __in ULONG Rva
    )
{
    PIMAGE_SECTION_HEADER section;

    section = PhMappedImageRvaToSection(MappedImage, Rva);

    if (!section)
        return NULL;

    return (PVOID)(
        (ULONG_PTR)MappedImage->ViewBase +
        section->PointerToRawData -
        section->VirtualAddress +
        Rva
        );
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

    loadConfig = PhMappedImageRvaToVa(MappedImage, entry->VirtualAddress);

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
        Exports->DataDirectory->VirtualAddress
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
        exportDirectory->AddressOfFunctions
        );
    Exports->NamePointerTable = (PULONG)PhMappedImageRvaToVa(
        MappedImage,
        exportDirectory->AddressOfNames
        );
    Exports->OrdinalTable = (PUSHORT)PhMappedImageRvaToVa(
        MappedImage,
        exportDirectory->AddressOfNameOrdinals
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

    return STATUS_SUCCESS;
}

USHORT PhpExportIndexToOrdinal(
    __in PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in ULONG Index
    )
{
    return (USHORT)(Exports->OrdinalTable[Index] + Exports->ExportDirectory->Base);
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

    Entry->Ordinal = PhpExportIndexToOrdinal(Exports, Index);

    if (Index < Exports->ExportDirectory->NumberOfNames)
    {
        name = PhMappedImageRvaToVa(
            Exports->MappedImage,
            Exports->NamePointerTable[Index]
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

        Ordinal = PhpExportIndexToOrdinal(Exports, index);
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
            rva
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
            rva
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
            Exports->NamePointerTable[i]
            );

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
