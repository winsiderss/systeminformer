/*
 * Process Hacker -
 *   ELF library support
 *
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

#include <ph.h>
#include <mapimg.h>

NTSTATUS PhInitializeMappedWslImage(
    _Out_ PPH_MAPPED_IMAGE MappedWslImage,
    _In_ PVOID ViewBase,
    _In_ SIZE_T Size
    )
{
    MappedWslImage->ViewBase = ViewBase;
    MappedWslImage->Size = Size;
    MappedWslImage->Header = (PELF_IMAGE_HEADER)ViewBase;

    __try
    {
        PhProbeAddress(
            MappedWslImage->Header, 
            sizeof(ELF_IMAGE_HEADER), 
            MappedWslImage->ViewBase, 
            MappedWslImage->Size,
            1
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    // Check the magic number.

    if (!RtlEqualMemory(MappedWslImage->Header->e_ident, ELFMAG, sizeof(ELFMAG)))
        return STATUS_FAIL_CHECK;

    // Check the class type.

    if (MappedWslImage->Header->e_ident[EI_CLASS] == ELFCLASS64)
    {
        MappedWslImage->Headers64 = PTR_ADD_OFFSET(MappedWslImage->Header, sizeof(ELF_IMAGE_HEADER));

        __try
        {
            PhProbeAddress(
                MappedWslImage->Headers64, 
                sizeof(ELF64_IMAGE_SECTION_HEADER), 
                MappedWslImage->ViewBase, 
                MappedWslImage->Size, 
                1
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    else
    {
        // TODO: Add 32bit ELF support.
        return STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT;
    }

    if (MappedWslImage->Headers64->e_phentsize != sizeof(ELF64_IMAGE_SEGMENT_HEADER))
        return STATUS_FAIL_CHECK;
    if (MappedWslImage->Headers64->e_shentsize != sizeof(ELF64_IMAGE_SECTION_HEADER))
        return STATUS_FAIL_CHECK;

    return STATUS_SUCCESS;
}

PELF64_IMAGE_SEGMENT_HEADER PhGetMappedWslImageSegment(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _In_ USHORT Index
    )
{
    return IMAGE_ELF64_SEGMENT_BY_INDEX(IMAGE_FIRST_ELF64_SEGMENT(MappedWslImage), Index);
}

PELF64_IMAGE_SEGMENT_HEADER PhGetWslImageSegmentByType(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _In_opt_ USHORT Type
    )
{
    PELF64_IMAGE_SEGMENT_HEADER segmentHeader;
    USHORT i;

    segmentHeader = IMAGE_FIRST_ELF64_SEGMENT(MappedWslImage);

    for (i = 0; i < MappedWslImage->Headers64->e_phnum; i++)
    {
        if (segmentHeader[i].p_type == Type)
            return segmentHeader;
    }

    return NULL;
}

PVOID PhGetMappedWslImageSectionData(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _In_opt_ PSTR Name,
    _In_opt_ USHORT Index
    )
{
    if (Name)
    {
        PELF64_IMAGE_SECTION_HEADER sectionHeader;
        PELF64_IMAGE_SECTION_HEADER stringSection;
        PELF64_IMAGE_SECTION_HEADER section;
        PVOID stringTable;
        USHORT i;

        sectionHeader = IMAGE_FIRST_ELF64_SECTION(MappedWslImage);
        stringSection = IMAGE_ELF64_SECTION_BY_INDEX(sectionHeader, MappedWslImage->Headers64->e_shstrndx);
        stringTable = PTR_ADD_OFFSET(MappedWslImage->Header, stringSection->sh_offset);

        for (i = 0; i < MappedWslImage->Headers64->e_shnum; i++)
        {
            section = IMAGE_ELF64_SECTION_BY_INDEX(sectionHeader, i);

            if (strcmp(Name, PTR_ADD_OFFSET(stringTable, section->sh_name)) == 0)
            {
                return PTR_ADD_OFFSET(MappedWslImage->Header, section->sh_offset);
            }
        }

        return NULL;
    }
    else
    {
        PELF64_IMAGE_SECTION_HEADER sectionHeader;
        PELF64_IMAGE_SECTION_HEADER section;

        sectionHeader = IMAGE_FIRST_ELF64_SECTION(MappedWslImage);
        section = IMAGE_ELF64_SECTION_BY_INDEX(sectionHeader, Index);

        return PTR_ADD_OFFSET(MappedWslImage->Header, section->sh_offset);
    }
}

PVOID PhGetMappedWslImageSectionDataByType(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _In_ UINT32 Type
    )
{
    PELF64_IMAGE_SECTION_HEADER sectionHeader;
    PELF64_IMAGE_SECTION_HEADER section;
    USHORT i;

    sectionHeader = IMAGE_FIRST_ELF64_SECTION(MappedWslImage);

    for (i = 0; i < MappedWslImage->Headers64->e_shnum; i++)
    {
        section = IMAGE_ELF64_SECTION_BY_INDEX(sectionHeader, i);

        if (section->sh_type == Type)
        {
            return PTR_ADD_OFFSET(MappedWslImage->Header, section->sh_offset);
        }
    }

    return NULL;
}

PELF64_IMAGE_SECTION_HEADER PhGetMappedWslImageSectionByIndex(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _In_ USHORT Index
    )
{
    return IMAGE_ELF64_SECTION_BY_INDEX(IMAGE_FIRST_ELF64_SECTION(MappedWslImage), Index);
}

PELF64_IMAGE_SECTION_HEADER PhGetMappedWslImageSectionByType(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _In_ UINT32 Type
    )
{
    PELF64_IMAGE_SECTION_HEADER sectionHeader;
    PELF64_IMAGE_SECTION_HEADER section;
    USHORT i;

    sectionHeader = IMAGE_FIRST_ELF64_SECTION(MappedWslImage);

    for (i = 0; i < MappedWslImage->Headers64->e_shnum; i++)
    {
        section = IMAGE_ELF64_SECTION_BY_INDEX(sectionHeader, i);

        if (section->sh_type == Type)
            return section;
    }

    return NULL;
}

// TODO: Check this is actually correct.
// https://stackoverflow.com/questions/18296276/base-address-of-elf
ULONG64 PhGetMappedWslImageBaseAddress(
    _In_ PPH_MAPPED_IMAGE MappedWslImage
    )
{
    ULONG64 baseAddress = MAXULONG64;
    PELF64_IMAGE_SEGMENT_HEADER segment;
    USHORT i;

    segment = IMAGE_FIRST_ELF64_SEGMENT(MappedWslImage);

    for (i = 0; i < MappedWslImage->Headers64->e_phnum; i++)
    {
        if (segment[i].p_type == PT_LOAD)
        {
            if (segment[i].p_paddr < baseAddress)
                baseAddress = segment[i].p_paddr;
        }
    }

    if (baseAddress == MAXULONG64)
        baseAddress = 0;

    return baseAddress;
}

BOOLEAN PhGetMappedWslImageSections(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _Out_ USHORT *NumberOfSections,
    _Out_ PPH_ELF_IMAGE_SECTION *ImageSections
    )
{
    USHORT count;
    USHORT i;
    PPH_ELF_IMAGE_SECTION sections;
    PELF64_IMAGE_SECTION_HEADER sectionHeader;
    PELF64_IMAGE_SECTION_HEADER stringSection;
    PVOID stringTableAddress;

    count = MappedWslImage->Headers64->e_shnum;
    sections = PhAllocate(sizeof(PH_ELF_IMAGE_SECTION) * count);
    memset(sections, 0, sizeof(PH_ELF_IMAGE_SECTION) * count);

    // Get the first section.
    sectionHeader = IMAGE_FIRST_ELF64_SECTION(MappedWslImage);

    // Get the string section.
    stringSection = IMAGE_ELF64_SECTION_BY_INDEX(sectionHeader, MappedWslImage->Headers64->e_shstrndx);

    // Get the string table (The string table is an array of null terminated strings).
    stringTableAddress = PTR_ADD_OFFSET(MappedWslImage->Header, stringSection->sh_offset);
                                                                                  
    // Enumerate the sections.
    for (i = 0; i < count; i++)
    {
        sections[i].Type = sectionHeader[i].sh_type;
        sections[i].Flags = sectionHeader[i].sh_flags;
        sections[i].Address = sectionHeader[i].sh_addr;
        sections[i].Offset = sectionHeader[i].sh_offset;
        sections[i].Size = sectionHeader[i].sh_size;

        if (sectionHeader[i].sh_name)
        {
            // Get the section name from the ELF string table and convert to unicode.
            PhCopyStringZFromBytes(
                PTR_ADD_OFFSET(stringTableAddress, sectionHeader[i].sh_name),
                -1,
                sections[i].Name,
                sizeof(sections[i].Name),
                NULL
                );
        }
    }

    *NumberOfSections = count;
    *ImageSections = sections;

    return TRUE;
}

typedef struct _PH_ELF_VERSION_RECORD
{
    USHORT Version;
    PSTR Name;
    PSTR FileName;
} PH_ELF_VERSION_RECORD, *PPH_ELF_VERSION_RECORD;

static PPH_LIST PhpParseMappedWslImageVersionRecords(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _In_ PVOID SymbolStringTable
    )
{
    PPH_LIST recordsList;
    PELF_VERSION_NEED version;

    if (!(version = PhGetMappedWslImageSectionDataByType(MappedWslImage, SHT_SUNW_verneed)))
        return NULL;

    recordsList = PhCreateList(10);

    while (TRUE)
    {
        PELF_VERSION_AUX versionAux = PTR_ADD_OFFSET(version, version->vn_aux);

        while (TRUE)
        {
            PPH_ELF_VERSION_RECORD versionInfo;

            versionInfo = PhAllocate(sizeof(PH_ELF_VERSION_RECORD));
            memset(versionInfo, 0, sizeof(PH_ELF_VERSION_RECORD));
            versionInfo->Version = versionAux->vna_other;
            versionInfo->Name = PTR_ADD_OFFSET(SymbolStringTable, versionAux->vna_name);
            versionInfo->FileName = PTR_ADD_OFFSET(SymbolStringTable, version->vn_file);

            PhAddItemList(recordsList, versionInfo);

            if (versionAux->vna_next == 0)
                break;

            versionAux = PTR_ADD_OFFSET(versionAux, versionAux->vna_next);
        }

        if (version->vn_next == 0)
            break;

        version = PTR_ADD_OFFSET(version, version->vn_next);
    }

    return recordsList;
}

static VOID PhpFreeMappedWslImageVersionRecords(
    _In_ PPH_LIST RecordsList
    )
{
    for (ULONG i = 0; i < RecordsList->Count; i++)
        PhFree(RecordsList->Items[i]);

    PhDereferenceObject(RecordsList);
}

static PSTR PhpFindWslImageVersionRecordName(
    _In_ PPH_LIST VersionRecordList,
    _In_ USHORT VersionIndex
    )
{
    // Note: 'ldconfig -v' and 'ldconfig -p' can be used to locate the path from the FileName.
    for (ULONG i = 0; i < VersionRecordList->Count; i++)
    {
        PPH_ELF_VERSION_RECORD versionInfo = VersionRecordList->Items[i];

        if (versionInfo->Version == VersionIndex)
            return versionInfo->FileName;
    }

    return NULL;
}

// TODO: Optimize this function.
BOOLEAN PhGetMappedWslImageSymbols(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _Out_ PPH_LIST *ImageSymbols
    )
{
    PELF64_IMAGE_SECTION_HEADER section;
    PPH_LIST symbols = PhCreateList(2000);

    if (section = PhGetMappedWslImageSectionByType(MappedWslImage, SHT_SYMTAB))
    {
        // TODO: Need to find a WSL binary with a symbol table.
        // SHT_SYMTAB should be parsed idential to SHT_DYNSYM?
    }
    
    if (section = PhGetMappedWslImageSectionByType(MappedWslImage, SHT_DYNSYM))
    {
        ULONGLONG count;
        ULONGLONG i;
        PELF_IMAGE_SYMBOL_ENTRY entry;
        PVOID stringTable;
        PELF_VERSION_TABLE versionTable;
        PPH_LIST versionRecords;

        if (section->sh_entsize != sizeof(ELF_IMAGE_SYMBOL_ENTRY))
            return FALSE;

        count = section->sh_size / sizeof(ELF_IMAGE_SYMBOL_ENTRY);
        entry = PTR_ADD_OFFSET(MappedWslImage->Header, section->sh_offset);
        stringTable = PhGetMappedWslImageSectionData(MappedWslImage, NULL, section->sh_link);

        versionTable = PhGetMappedWslImageSectionDataByType(MappedWslImage, SHT_SUNW_versym);
        versionRecords = PhpParseMappedWslImageVersionRecords(MappedWslImage, stringTable);

        for (i = 1; i < count; i++)
        {
            if (entry[i].st_shndx == SHN_UNDEF)
            {
                PPH_ELF_IMAGE_SYMBOL_ENTRY import;

                import = PhAllocate(sizeof(PH_ELF_IMAGE_SYMBOL_ENTRY));
                memset(import, 0, sizeof(PH_ELF_IMAGE_SYMBOL_ENTRY));

                import->ImportSymbol = TRUE;
                import->Address = entry[i].st_value;
                import->Size = entry[i].st_size;
                import->TypeInfo = entry[i].st_info;

                // function name
                PhCopyStringZFromBytes(
                    PTR_ADD_OFFSET(stringTable, entry[i].st_name),
                    -1,
                    import->Name,
                    sizeof(import->Name),
                    NULL
                    );

                // import library name
                if (versionTable && versionRecords)
                {
                    PSTR moduleName;

                    if (moduleName = PhpFindWslImageVersionRecordName(versionRecords, versionTable[i].vs_vers))
                    {
                        PhCopyStringZFromBytes(
                            moduleName,
                            -1,
                            import->Module,
                            sizeof(import->Module),
                            NULL
                            );
                    }
                }

                PhAddItemList(symbols, import);
            }
            else if (entry[i].st_shndx != SHN_UNDEF && entry[i].st_value != 0)
            {
                PPH_ELF_IMAGE_SYMBOL_ENTRY export;

                if (ELF_ST_TYPE(entry[i].st_info) == STT_SECTION) // Ignore section symbol types.
                    continue;

                export = PhAllocate(sizeof(PH_ELF_IMAGE_SYMBOL_ENTRY));
                memset(export, 0, sizeof(PH_ELF_IMAGE_SYMBOL_ENTRY));

                export->ExportSymbol = TRUE;
                export->Address = entry[i].st_value;
                export->Size = entry[i].st_size;
                export->TypeInfo = entry[i].st_info;

                // function name
                PhCopyStringZFromBytes(
                    PTR_ADD_OFFSET(stringTable, entry[i].st_name),
                    -1,
                    export->Name,
                    sizeof(export->Name),
                    NULL
                    );

                PhAddItemList(symbols, export);
            }
            else
            {   
                PPH_ELF_IMAGE_SYMBOL_ENTRY export;

                export = PhAllocate(sizeof(PH_ELF_IMAGE_SYMBOL_ENTRY));
                memset(export, 0, sizeof(PH_ELF_IMAGE_SYMBOL_ENTRY));

                export->UnknownSymbol = TRUE;
                export->Address = entry[i].st_value;
                export->Size = entry[i].st_size;
                export->TypeInfo = entry[i].st_info;

                // function name
                PhCopyStringZFromBytes(
                    PTR_ADD_OFFSET(stringTable, entry[i].st_name),
                    -1,
                    export->Name,
                    sizeof(export->Name),
                    NULL
                    );

                PhAddItemList(symbols, export);
            }
        }

        if (versionRecords)
            PhpFreeMappedWslImageVersionRecords(versionRecords);
    }

    *ImageSymbols = symbols;

    return TRUE;
}

VOID PhFreeMappedWslImageSymbols(
    _In_ PPH_LIST ImageSymbols
    )
{
    for (ULONG i = 0; i < ImageSymbols->Count; i++)
        PhFree(ImageSymbols->Items[i]);

    PhDereferenceObject(ImageSymbols);
}