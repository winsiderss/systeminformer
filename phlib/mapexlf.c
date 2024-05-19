/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017
 *
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
    MappedWslImage->ViewSize = Size;
    MappedWslImage->Header = (PELF_IMAGE_HEADER)ViewBase;

    __try
    {
        PhProbeAddress(
            MappedWslImage->Header,
            sizeof(ELF_IMAGE_HEADER),
            MappedWslImage->ViewBase,
            MappedWslImage->ViewSize,
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
                MappedWslImage->ViewSize,
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

    //if (MappedWslImage->Headers64->e_phentsize != sizeof(ELF64_IMAGE_SEGMENT_HEADER))
    //    return STATUS_FAIL_CHECK;
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

            if (PhEqualBytesZ(Name, PTR_ADD_OFFSET(stringTable, section->sh_name), FALSE))
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

PELF64_IMAGE_SECTION_HEADER PhGetMappedWslImageSectionByName(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _In_ PSTR Name
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

            if (PhEqualBytesZ(Name, PTR_ADD_OFFSET(stringTable, section->sh_name), FALSE))
            {
                return section;
            }
        }
    }

    return NULL;
}

ULONG64 PhGetMappedWslImageBaseAddress(
    _In_ PPH_MAPPED_IMAGE MappedWslImage
    )
{
    ULONG64 baseAddress = MAXULONG64;
    PELF64_IMAGE_SEGMENT_HEADER segment;
    ULONG loadBias = 0;
    USHORT i;

    segment = IMAGE_FIRST_ELF64_SEGMENT(MappedWslImage);

    for (i = 0; i < MappedWslImage->Headers64->e_phnum; i++)
    {
        if (segment[i].p_type == PT_LOAD)
        {
            loadBias = (ULONG)ALIGN_DOWN_BY(segment[i].p_vaddr, PAGE_SIZE);
            break;
        }
    }

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
            PhCopyStringZFromUtf8(
                PTR_ADD_OFFSET(stringTableAddress, sectionHeader[i].sh_name),
                SIZE_MAX,
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

            versionInfo = PhAllocateZero(sizeof(PH_ELF_VERSION_RECORD));
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

BOOLEAN PhGetMappedWslImageSymbols(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _Out_ PPH_LIST *ImageSymbols
    )
{
    PELF64_IMAGE_SECTION_HEADER sectionHeader;
    PELF64_IMAGE_SECTION_HEADER section;
    PPH_LIST sectionSymbols;
    USHORT i;

    sectionSymbols = PhCreateList(0x800);
    sectionHeader = IMAGE_FIRST_ELF64_SECTION(MappedWslImage);

    for (i = 0; i < MappedWslImage->Headers64->e_shnum; i++)
    {
        ULONGLONG count;
        ULONGLONG ii;
        PELF_IMAGE_SYMBOL_ENTRY entry;
        PVOID stringTable;
        PELF_VERSION_TABLE versionTable;
        PPH_LIST versionRecords;

        section = IMAGE_ELF64_SECTION_BY_INDEX(sectionHeader, i);

        // NOTE: The below code needs some improvements for SHT_SYMTAB -dmex
        if (section->sh_type != SHT_SYMTAB && section->sh_type != SHT_DYNSYM)
            continue;

        if (section->sh_entsize != sizeof(ELF_IMAGE_SYMBOL_ENTRY))
            return FALSE;

        count = section->sh_size / sizeof(ELF_IMAGE_SYMBOL_ENTRY);
        entry = PTR_ADD_OFFSET(MappedWslImage->Header, section->sh_offset);
        stringTable = PhGetMappedWslImageSectionData(MappedWslImage, NULL, section->sh_link);

        // NOTE: SHT_DYNSYM entries include the version in the symbol name (e.g. printf@GLIBC_2.2.5)
        // instead of using a version record entry from the SHT_SUNW_versym section -dmex
        versionTable = PhGetMappedWslImageSectionDataByType(MappedWslImage, SHT_SUNW_versym);
        versionRecords = PhpParseMappedWslImageVersionRecords(MappedWslImage, stringTable);

        for (ii = 1; ii < count; ii++)
        {
            if (entry[ii].st_shndx == SHN_UNDEF)
            {
                PPH_ELF_IMAGE_SYMBOL_ENTRY import;

                import = PhAllocateZero(sizeof(PH_ELF_IMAGE_SYMBOL_ENTRY));
                import->ImportSymbol = TRUE;
                import->Address = entry[ii].st_value;
                import->Size = entry[ii].st_size;
                import->TypeInfo = entry[ii].st_info;
                import->OtherInfo = entry[ii].st_other;
                import->SectionIndex = entry[ii].st_shndx;

                // function name
                PhCopyStringZFromBytes(
                    PTR_ADD_OFFSET(stringTable, entry[ii].st_name),
                    SIZE_MAX,
                    import->Name,
                    sizeof(import->Name),
                    NULL
                    );

                // import library name
                if (versionTable && versionRecords)
                {
                    PSTR moduleName;

                    if (moduleName = PhpFindWslImageVersionRecordName(versionRecords, versionTable[ii].vs_vers))
                    {
                        PhCopyStringZFromUtf8(
                            moduleName,
                            SIZE_MAX,
                            import->Module,
                            sizeof(import->Module),
                            NULL
                            );
                    }
                }

                PhAddItemList(sectionSymbols, import);
            }
            else if (entry[ii].st_shndx != SHN_UNDEF && entry[ii].st_value != 0)
            {
                PPH_ELF_IMAGE_SYMBOL_ENTRY export;

                if (ELF_ST_TYPE(entry[ii].st_info) == STT_SECTION) // Ignore section symbol types.
                    continue;

                export = PhAllocateZero(sizeof(PH_ELF_IMAGE_SYMBOL_ENTRY));
                export->ExportSymbol = TRUE;
                export->Address = entry[ii].st_value;
                export->Size = entry[ii].st_size;
                export->TypeInfo = entry[ii].st_info;
                export->OtherInfo = entry[ii].st_other;
                export->SectionIndex = entry[ii].st_shndx;

                // function name
                PhCopyStringZFromUtf8(
                    PTR_ADD_OFFSET(stringTable, entry[ii].st_name),
                    SIZE_MAX,
                    export->Name,
                    sizeof(export->Name),
                    NULL
                    );

                PhAddItemList(sectionSymbols, export);
            }
            else
            {
                PPH_ELF_IMAGE_SYMBOL_ENTRY export;

                export = PhAllocateZero(sizeof(PH_ELF_IMAGE_SYMBOL_ENTRY));
                export->UnknownSymbol = TRUE;
                export->Address = entry[ii].st_value;
                export->Size = entry[ii].st_size;
                export->TypeInfo = entry[ii].st_info;
                export->OtherInfo = entry[ii].st_other;
                export->SectionIndex = entry[ii].st_shndx;

                // function name
                PhCopyStringZFromUtf8(
                    PTR_ADD_OFFSET(stringTable, entry[ii].st_name),
                    SIZE_MAX,
                    export->Name,
                    sizeof(export->Name),
                    NULL
                    );

                PhAddItemList(sectionSymbols, export);
            }
        }

        if (versionRecords)
            PhpFreeMappedWslImageVersionRecords(versionRecords);
    }

    *ImageSymbols = sectionSymbols;

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

BOOLEAN PhGetMappedWslImageDynamic(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _Out_ PPH_LIST *ImageDynamic
    )
{
    PELF64_IMAGE_SECTION_HEADER sectionHeader;
    PELF64_IMAGE_SECTION_HEADER section;
    PPH_LIST dynamicSymbols;
    USHORT i;

    dynamicSymbols = PhCreateList(0x40);
    sectionHeader = IMAGE_FIRST_ELF64_SECTION(MappedWslImage);

    for (i = 0; i < MappedWslImage->Headers64->e_shnum; i++)
    {
        ULONGLONG count;
        ULONGLONG ii;
        PELF64_IMAGE_DYNAMIC_ENTRY entry;
        PVOID stringTable;

        section = IMAGE_ELF64_SECTION_BY_INDEX(sectionHeader, i);

        if (section->sh_type != SHT_DYNAMIC)
            continue;

        if (section->sh_entsize != sizeof(ELF64_IMAGE_DYNAMIC_ENTRY))
            return FALSE;

        count = section->sh_size / sizeof(ELF64_IMAGE_DYNAMIC_ENTRY);
        entry = PTR_ADD_OFFSET(MappedWslImage->Header, section->sh_offset);
        stringTable = PhGetMappedWslImageSectionData(MappedWslImage, NULL, section->sh_link);

        for (ii = 0; ii < count; ii++)
        {
            PPH_ELF_IMAGE_DYNAMIC_ENTRY dynamic;

            dynamic = PhAllocateZero(sizeof(PH_ELF_IMAGE_DYNAMIC_ENTRY));
            dynamic->Tag = entry[ii].d_tag;

            switch (dynamic->Tag)
            {
            case DT_NEEDED:
            case DT_SONAME:
            case DT_RPATH:
            case DT_RUNPATH:
                dynamic->Value = PhConvertUtf8ToUtf16(PTR_ADD_OFFSET(stringTable, entry[ii].d_val));
                break;
            case DT_PLTRELSZ:
            case DT_RELASZ:
            case DT_RELAENT:
            case DT_STRSZ:
            case DT_SYMENT:
            case DT_RELSZ:
            case DT_RELENT:
            case DT_INIT_ARRAYSZ:
            case DT_FINI_ARRAYSZ:
            case DT_PREINIT_ARRAYSZ:
                dynamic->Value = PhFormatSize(entry[ii].d_val, ULONG_MAX);
                break;
            case DT_RELACOUNT:
            case DT_RELCOUNT:
            case DT_VERDEFNUM:
            case DT_VERNEEDNUM:
                dynamic->Value = PhFormatUInt64(entry[ii].d_val, TRUE);
                break;
            default:
                dynamic->Value = PhFormatString(L"0x%llx", entry[ii].d_val);
                break;
            }

            PhAddItemList(dynamicSymbols, dynamic);

            if (entry[ii].d_tag == DT_NULL)
                break;
        }
    }

    *ImageDynamic = dynamicSymbols;

    return TRUE;
}

VOID PhFreeMappedWslImageDynamic(
    _In_ PPH_LIST ImageDynamic
    )
{
    for (ULONG i = 0; i < ImageDynamic->Count; i++)
    {
        PPH_ELF_IMAGE_DYNAMIC_ENTRY dynamic = ImageDynamic->Items[i];

        PhDereferenceObject(dynamic->Value);
        PhFree(dynamic);
    }

    PhDereferenceObject(ImageDynamic);
}
