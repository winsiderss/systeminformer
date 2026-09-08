/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"
#include <mapimg.h>
#include <wintrust.h>

// The parts of a PE that get_image_info returns only when asked for. The default answer stays the
// summary, because an agent that wants the imports of one file should not have to read the exports
// of every file it looked at first.

#define AT_IMAGE_SECTION_HEADERS      0x0001
#define AT_IMAGE_SECTION_DIRECTORIES  0x0002
#define AT_IMAGE_SECTION_IMPORTS      0x0004
#define AT_IMAGE_SECTION_EXPORTS      0x0008
#define AT_IMAGE_SECTION_LOAD_CONFIG  0x0010
#define AT_IMAGE_SECTION_MANIFEST     0x0020
#define AT_IMAGE_SECTION_VERSION_INFO 0x0040
#define AT_IMAGE_SECTION_DEBUG        0x0080
#define AT_IMAGE_SECTION_CERTIFICATES 0x0100
#define AT_IMAGE_SECTION_RICH_HEADER  0x0200
#define AT_IMAGE_SECTION_TLS          0x0400
#define AT_IMAGE_SECTION_RESOURCES    0x0800
#define AT_IMAGE_SECTION_CLR          0x1000
#define AT_IMAGE_SECTION_ENTROPY      0x2000
#define AT_IMAGE_SECTION_ALL          0x3fff

// A manifest is XML and an image can carry a big one; more than this is a file to look at with
// something else.
#define AT_IMAGE_MANIFEST_MAX_SIZE (256 * 1024)
// Enough for any real import table; ntdll exports about 2400 functions.
#define AT_IMAGE_MAX_FUNCTIONS 4096

typedef struct _AT_IMAGE_SECTION_NAME
{
    PCWSTR Name;
    ULONG Flag;
} AT_IMAGE_SECTION_NAME;

ULONG AtpParseImageSections(
    _In_opt_ PVOID Sections,
    _Out_ PPH_STRING* Invalid
    )
{
    static CONST AT_IMAGE_SECTION_NAME names[] =
    {
        { L"headers", AT_IMAGE_SECTION_HEADERS },
        { L"directories", AT_IMAGE_SECTION_DIRECTORIES },
        { L"imports", AT_IMAGE_SECTION_IMPORTS },
        { L"exports", AT_IMAGE_SECTION_EXPORTS },
        { L"load_config", AT_IMAGE_SECTION_LOAD_CONFIG },
        { L"manifest", AT_IMAGE_SECTION_MANIFEST },
        { L"version_info", AT_IMAGE_SECTION_VERSION_INFO },
        { L"debug", AT_IMAGE_SECTION_DEBUG },
        { L"certificates", AT_IMAGE_SECTION_CERTIFICATES },
        { L"rich_header", AT_IMAGE_SECTION_RICH_HEADER },
        { L"tls", AT_IMAGE_SECTION_TLS },
        { L"resources", AT_IMAGE_SECTION_RESOURCES },
        { L"clr", AT_IMAGE_SECTION_CLR },
        { L"entropy", AT_IMAGE_SECTION_ENTROPY },
        { L"all", AT_IMAGE_SECTION_ALL },
    };
    ULONG flags = 0;
    ULONG count;
    ULONG i;
    ULONG j;

    *Invalid = NULL;

    if (!Sections)
        return 0;

    count = PhGetJsonArrayLength(Sections);

    for (i = 0; i < count; i++)
    {
        PPH_STRING name = PhGetJsonObjectString(PhGetJsonArrayIndexObject(Sections, i));

        if (!name)
            continue;

        for (j = 0; j < RTL_NUMBER_OF(names); j++)
        {
            if (PhEqualString2(name, names[j].Name, TRUE))
            {
                flags |= names[j].Flag;
                break;
            }
        }

        if (j == RTL_NUMBER_OF(names))
        {
            *Invalid = name;
            return 0;
        }

        PhDereferenceObject(name);
    }

    return flags;
}

VOID AtpAddImageHeaders(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    static CONST ULONG fileFlags[] =
    {
        IMAGE_FILE_RELOCS_STRIPPED, IMAGE_FILE_EXECUTABLE_IMAGE, IMAGE_FILE_LINE_NUMS_STRIPPED,
        IMAGE_FILE_LOCAL_SYMS_STRIPPED, IMAGE_FILE_LARGE_ADDRESS_AWARE, IMAGE_FILE_32BIT_MACHINE,
        IMAGE_FILE_DEBUG_STRIPPED, IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP, IMAGE_FILE_NET_RUN_FROM_SWAP,
        IMAGE_FILE_SYSTEM, IMAGE_FILE_DLL, IMAGE_FILE_UP_SYSTEM_ONLY
    };
    static CONST PWSTR fileNames[] =
    {
        L"relocs_stripped", L"executable", L"line_nums_stripped",
        L"local_syms_stripped", L"large_address_aware", L"32bit_machine",
        L"debug_stripped", L"removable_run_from_swap", L"net_run_from_swap",
        L"system", L"dll", L"up_system_only"
    };
    PVOID headers;
    PVOID entry;
    PIMAGE_NT_HEADERS ntHeaders = MappedImage->NtHeaders;
    BOOLEAN is64 = MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;

    headers = PhCreateJsonObject();

    entry = PhCreateJsonObject();
    PhAddJsonObjectUInt64(entry, "e_lfanew", (ULONG_PTR)PTR_SUB_OFFSET(ntHeaders, MappedImage->ViewBase));
    PhAddJsonObjectValue(headers, "dos", entry);

    entry = PhCreateJsonObject();
    AtJsonAddStringZ(entry, "machine", AtMachineString(ntHeaders->FileHeader.Machine));
    AtJsonAddHex(entry, "machine_value", ntHeaders->FileHeader.Machine);
    PhAddJsonObjectUInt64(entry, "number_of_sections", ntHeaders->FileHeader.NumberOfSections);
    PhAddJsonObjectUInt64(entry, "number_of_symbols", ntHeaders->FileHeader.NumberOfSymbols);
    PhAddJsonObjectUInt64(entry, "size_of_optional_header", ntHeaders->FileHeader.SizeOfOptionalHeader);
    AtJsonAddFlagStrings(entry, "characteristics", ntHeaders->FileHeader.Characteristics,
        fileFlags, (CONST PWSTR*)fileNames, RTL_NUMBER_OF(fileFlags));
    PhAddJsonObjectValue(headers, "file", entry);

    entry = PhCreateJsonObject();

#define AT_ADD_OPTIONAL(opt) \
    AtJsonAddHex(entry, "magic", (opt)->Magic); \
    PhAddJsonObjectUInt64(entry, "linker_version_major", (opt)->MajorLinkerVersion); \
    PhAddJsonObjectUInt64(entry, "linker_version_minor", (opt)->MinorLinkerVersion); \
    PhAddJsonObjectUInt64(entry, "size_of_code", (opt)->SizeOfCode); \
    PhAddJsonObjectUInt64(entry, "size_of_initialized_data", (opt)->SizeOfInitializedData); \
    PhAddJsonObjectUInt64(entry, "size_of_uninitialized_data", (opt)->SizeOfUninitializedData); \
    AtJsonAddHex(entry, "address_of_entry_point", (opt)->AddressOfEntryPoint); \
    AtJsonAddHex(entry, "base_of_code", (opt)->BaseOfCode); \
    AtJsonAddHex(entry, "image_base", (opt)->ImageBase); \
    PhAddJsonObjectUInt64(entry, "section_alignment", (opt)->SectionAlignment); \
    PhAddJsonObjectUInt64(entry, "file_alignment", (opt)->FileAlignment); \
    PhAddJsonObjectUInt64(entry, "operating_system_version_major", (opt)->MajorOperatingSystemVersion); \
    PhAddJsonObjectUInt64(entry, "operating_system_version_minor", (opt)->MinorOperatingSystemVersion); \
    PhAddJsonObjectUInt64(entry, "image_version_major", (opt)->MajorImageVersion); \
    PhAddJsonObjectUInt64(entry, "image_version_minor", (opt)->MinorImageVersion); \
    PhAddJsonObjectUInt64(entry, "subsystem_version_major", (opt)->MajorSubsystemVersion); \
    PhAddJsonObjectUInt64(entry, "subsystem_version_minor", (opt)->MinorSubsystemVersion); \
    PhAddJsonObjectUInt64(entry, "size_of_image", (opt)->SizeOfImage); \
    PhAddJsonObjectUInt64(entry, "size_of_headers", (opt)->SizeOfHeaders); \
    AtJsonAddHex(entry, "checksum", (opt)->CheckSum); \
    AtJsonAddStringZ(entry, "subsystem", AtSubsystemString((opt)->Subsystem)); \
    PhAddJsonObjectUInt64(entry, "size_of_stack_reserve", (opt)->SizeOfStackReserve); \
    PhAddJsonObjectUInt64(entry, "size_of_stack_commit", (opt)->SizeOfStackCommit); \
    PhAddJsonObjectUInt64(entry, "size_of_heap_reserve", (opt)->SizeOfHeapReserve); \
    PhAddJsonObjectUInt64(entry, "size_of_heap_commit", (opt)->SizeOfHeapCommit); \
    PhAddJsonObjectUInt64(entry, "number_of_rva_and_sizes", (opt)->NumberOfRvaAndSizes)

    if (is64)
    {
        PIMAGE_OPTIONAL_HEADER64 opt = &MappedImage->NtHeaders->OptionalHeader;

        AT_ADD_OPTIONAL(opt);
    }
    else
    {
        PIMAGE_OPTIONAL_HEADER32 opt = &MappedImage->NtHeaders32->OptionalHeader;

        AT_ADD_OPTIONAL(opt);
    }

    PhAddJsonObjectValue(headers, "optional", entry);
    PhAddJsonObjectValue(Structured, "headers", headers);
}

PCWSTR AtpImageDirectoryName(
    _In_ ULONG Index
    )
{
    static CONST PWSTR names[] =
    {
        L"export", L"import", L"resource", L"exception", L"security", L"basereloc", L"debug",
        L"architecture", L"globalptr", L"tls", L"load_config", L"bound_import", L"iat",
        L"delay_import", L"com_descriptor", L"reserved"
    };

    if (Index < RTL_NUMBER_OF(names))
        return names[Index];

    return NULL;
}

VOID AtpAddImageDirectories(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    PVOID array;
    PIMAGE_DATA_DIRECTORY directories;
    ULONG count;
    ULONG i;

    if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        directories = MappedImage->NtHeaders->OptionalHeader.DataDirectory;
        count = MappedImage->NtHeaders->OptionalHeader.NumberOfRvaAndSizes;
    }
    else
    {
        directories = MappedImage->NtHeaders32->OptionalHeader.DataDirectory;
        count = MappedImage->NtHeaders32->OptionalHeader.NumberOfRvaAndSizes;
    }

    count = min(count, IMAGE_NUMBEROF_DIRECTORY_ENTRIES);
    array = PhCreateJsonArray();

    for (i = 0; i < count; i++)
    {
        PVOID entry = PhCreateJsonObject();

        PhAddJsonObjectUInt64(entry, "index", i);
        AtJsonAddStringZ(entry, "name", AtpImageDirectoryName(i));
        AtJsonAddHex(entry, "virtual_address", directories[i].VirtualAddress);
        PhAddJsonObjectUInt64(entry, "size", directories[i].Size);
        PhAddJsonArrayObject(array, entry);
    }

    PhAddJsonObjectValue(Structured, "directories", array);
}

VOID AtpAddImportDllRows(
    _In_ PAT_ROWS Rows,
    _In_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ BOOLEAN Delay
    )
{
    ULONG i;

    for (i = 0; i < Imports->NumberOfDlls; i++)
    {
        PH_MAPPED_IMAGE_IMPORT_DLL importDll;
        PVOID row;
        PVOID functions;
        ULONG count;
        ULONG j;

        if (!NT_SUCCESS(PhGetMappedImageImportDll(Imports, i, &importDll)))
            continue;

        row = PhCreateJsonObject();
        AtJsonAddStringZ(row, "name", NULL);

        if (importDll.Name)
        {
            PPH_STRING name = PhZeroExtendToUtf16((PSTR)importDll.Name);

            AtJsonAddString(row, "name", name);
            PhClearReference(&name);
        }

        PhAddJsonObjectBoolean(row, "delay_loaded", Delay);
        PhAddJsonObjectUInt64(row, "function_count", importDll.NumberOfEntries);

        count = min(importDll.NumberOfEntries, AT_IMAGE_MAX_FUNCTIONS);
        PhAddJsonObjectBoolean(row, "functions_truncated", count != importDll.NumberOfEntries);
        functions = PhCreateJsonArray();

        for (j = 0; j < count; j++)
        {
            PH_MAPPED_IMAGE_IMPORT_ENTRY entry;
            PVOID function;

            if (!NT_SUCCESS(PhGetMappedImageImportEntry(&importDll, j, &entry)))
                continue;

            function = PhCreateJsonObject();

            // An import is by name or by ordinal, never both: a null name means the ordinal is what
            // the loader will bind on, which is the harder one to attribute.
            if (entry.Name)
            {
                PPH_STRING name = PhZeroExtendToUtf16((PSTR)entry.Name);

                AtJsonAddString(function, "name", name);
                PhAddJsonObjectUInt64(function, "hint", entry.NameHint);
                AtJsonAddNull(function, "ordinal");
                PhClearReference(&name);
            }
            else
            {
                AtJsonAddNull(function, "name");
                AtJsonAddNull(function, "hint");
                PhAddJsonObjectUInt64(function, "ordinal", entry.Ordinal);
            }

            PhAddJsonArrayObject(functions, function);
        }

        PhAddJsonObjectValue(row, "functions", functions);
        AtAddRow(Rows, row);
    }
}

VOID AtpAddImageImports(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PAT_TOOL_CALL Call
    )
{
    PH_MAPPED_IMAGE_IMPORTS imports;
    AT_ROWS rows;

    AtInitializeRows(&rows, Call->Arguments);

    if (NT_SUCCESS(PhGetMappedImageImports(&imports, MappedImage)))
        AtpAddImportDllRows(&rows, &imports, FALSE);

    if (NT_SUCCESS(PhGetMappedImageDelayImports(&imports, MappedImage)))
        AtpAddImportDllRows(&rows, &imports, TRUE);

    AtAddRows(Structured, "imports", &rows);
    AtDeleteRows(&rows);
}

VOID AtpAddImageExports(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PAT_TOOL_CALL Call
    )
{
    PH_MAPPED_IMAGE_EXPORTS exports;
    AT_ROWS rows;
    ULONG i;

    AtInitializeRows(&rows, Call->Arguments);

    if (NT_SUCCESS(PhGetMappedImageExports(&exports, MappedImage)))
    {
        for (i = 0; i < exports.NumberOfEntries; i++)
        {
            PH_MAPPED_IMAGE_EXPORT_ENTRY entry;
            PH_MAPPED_IMAGE_EXPORT_FUNCTION function;
            PVOID row;

            if (!NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &entry)))
                continue;

            row = PhCreateJsonObject();

            if (entry.Name)
            {
                PPH_STRING name = PhZeroExtendToUtf16((PSTR)entry.Name);

                AtJsonAddString(row, "name", name);
                PhClearReference(&name);
            }
            else
            {
                AtJsonAddNull(row, "name");
            }

            PhAddJsonObjectUInt64(row, "ordinal", entry.Ordinal);
            PhAddJsonObjectUInt64(row, "hint", entry.Hint);

            if (NT_SUCCESS(PhGetMappedImageExportFunction(&exports, entry.Name, entry.Ordinal, &function)))
            {
                AtJsonAddPointer(row, "address", function.Function);

                // A forwarder is an export that is really another module's export, which is how a
                // name can be exported by a DLL that does not implement it.
                if (function.ForwardedName)
                {
                    PPH_STRING forwarded = PhZeroExtendToUtf16((PSTR)function.ForwardedName);

                    AtJsonAddString(row, "forwarded_to", forwarded);
                    PhClearReference(&forwarded);
                }
                else
                {
                    AtJsonAddNull(row, "forwarded_to");
                }
            }
            else
            {
                AtJsonAddNull(row, "address");
                AtJsonAddNull(row, "forwarded_to");
            }

            AtAddRow(&rows, row);
        }
    }

    AtAddRows(Structured, "exports", &rows);
    AtDeleteRows(&rows);
}

VOID AtpAddImageLoadConfig(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    static CONST ULONG guardFlags[] =
    {
        IMAGE_GUARD_CF_INSTRUMENTED, IMAGE_GUARD_CFW_INSTRUMENTED,
        IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT, IMAGE_GUARD_SECURITY_COOKIE_UNUSED,
        IMAGE_GUARD_PROTECT_DELAYLOAD_IAT, IMAGE_GUARD_DELAYLOAD_IAT_IN_ITS_OWN_SECTION,
        IMAGE_GUARD_CF_EXPORT_SUPPRESSION_INFO_PRESENT, IMAGE_GUARD_CF_ENABLE_EXPORT_SUPPRESSION,
        IMAGE_GUARD_CF_LONGJUMP_TABLE_PRESENT, IMAGE_GUARD_RF_INSTRUMENTED,
        IMAGE_GUARD_RF_ENABLE, IMAGE_GUARD_RF_STRICT, IMAGE_GUARD_RETPOLINE_PRESENT,
        IMAGE_GUARD_XFG_ENABLED
    };
    static CONST PWSTR guardNames[] =
    {
        L"cf_instrumented", L"cfw_instrumented",
        L"cf_function_table_present", L"security_cookie_unused",
        L"protect_delayload_iat", L"delayload_iat_in_its_own_section",
        L"cf_export_suppression_info_present", L"cf_enable_export_suppression",
        L"cf_longjump_table_present", L"rf_instrumented",
        L"rf_enable", L"rf_strict", L"retpoline_present",
        L"xfg_enabled"
    };
    PVOID entry;

    entry = PhCreateJsonObject();

    if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        PIMAGE_LOAD_CONFIG_DIRECTORY64 config;

        if (!NT_SUCCESS(PhGetMappedImageLoadConfig64(MappedImage, &config)))
        {
            PhFreeJsonObject(entry);
            AtJsonAddNull(Structured, "load_config");
            return;
        }

        PhAddJsonObjectUInt64(entry, "size", config->Size);
        AtJsonAddHex(entry, "security_cookie", config->SecurityCookie);
        AtJsonAddHex(entry, "guard_cf_check_function_pointer", config->GuardCFCheckFunctionPointer);
        AtJsonAddHex(entry, "guard_cf_dispatch_function_pointer", config->GuardCFDispatchFunctionPointer);
        PhAddJsonObjectUInt64(entry, "guard_cf_function_count", config->GuardCFFunctionCount);
        AtJsonAddHex(entry, "guard_flags", config->GuardFlags);
        AtJsonAddFlagStrings(entry, "guard_flag_names", config->GuardFlags,
            guardFlags, (CONST PWSTR*)guardNames, RTL_NUMBER_OF(guardFlags));
        PhAddJsonObjectUInt64(entry, "dependent_load_flags", config->DependentLoadFlags);
    }
    else
    {
        PIMAGE_LOAD_CONFIG_DIRECTORY32 config;

        if (!NT_SUCCESS(PhGetMappedImageLoadConfig32(MappedImage, &config)))
        {
            PhFreeJsonObject(entry);
            AtJsonAddNull(Structured, "load_config");
            return;
        }

        PhAddJsonObjectUInt64(entry, "size", config->Size);
        AtJsonAddHex(entry, "security_cookie", config->SecurityCookie);
        AtJsonAddHex(entry, "guard_cf_check_function_pointer", config->GuardCFCheckFunctionPointer);
        AtJsonAddHex(entry, "guard_cf_dispatch_function_pointer", config->GuardCFDispatchFunctionPointer);
        PhAddJsonObjectUInt64(entry, "guard_cf_function_count", config->GuardCFFunctionCount);
        AtJsonAddHex(entry, "guard_flags", config->GuardFlags);
        AtJsonAddFlagStrings(entry, "guard_flag_names", config->GuardFlags,
            guardFlags, (CONST PWSTR*)guardNames, RTL_NUMBER_OF(guardFlags));
        PhAddJsonObjectUInt64(entry, "dependent_load_flags", config->DependentLoadFlags);
    }

    PhAddJsonObjectValue(Structured, "load_config", entry);
}

VOID AtpAddImageManifest(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    PVOID entry;
    PVOID buffer;
    ULONG length;

    entry = PhCreateJsonObject();

    if (NT_SUCCESS(PhGetMappedImageResource(MappedImage, MAKEINTRESOURCE(1), RT_MANIFEST, 0, &length, &buffer)) &&
        length != 0)
    {
        PPH_STRING xml;

        PhAddJsonObjectBoolean(entry, "present", TRUE);
        PhAddJsonObjectUInt64(entry, "size", length);
        PhAddJsonObjectBoolean(entry, "truncated", length > AT_IMAGE_MANIFEST_MAX_SIZE);

        // A manifest is UTF-8 by contract, whatever it declares in its own prologue.
        xml = PhConvertUtf8ToUtf16Ex(buffer, min(length, AT_IMAGE_MANIFEST_MAX_SIZE));
        AtJsonAddString(entry, "xml", xml);
        PhClearReference(&xml);
    }
    else
    {
        PhAddJsonObjectBoolean(entry, "present", FALSE);
        PhAddJsonObjectUInt64(entry, "size", 0);
        PhAddJsonObjectBoolean(entry, "truncated", FALSE);
        AtJsonAddNull(entry, "xml");
    }

    PhAddJsonObjectValue(Structured, "manifest", entry);
}

VOID AtpAddImageVersionInfo(
    _In_ PVOID Structured,
    _In_ PPH_STRING FileName
    )
{
    PH_IMAGE_VERSION_INFO versionInfo;
    PVOID entry;

    if (!NT_SUCCESS(PhInitializeImageVersionInfo(&versionInfo, PhGetString(FileName))))
    {
        AtJsonAddNull(Structured, "version_info");
        return;
    }

    entry = PhCreateJsonObject();
    AtJsonAddString(entry, "company", versionInfo.CompanyName);
    AtJsonAddString(entry, "description", versionInfo.FileDescription);
    AtJsonAddString(entry, "file_version", versionInfo.FileVersion);
    AtJsonAddString(entry, "product", versionInfo.ProductName);
    PhAddJsonObjectValue(Structured, "version_info", entry);
    PhDeleteImageVersionInfo(&versionInfo);
}

PCWSTR AtpImageDebugTypeString(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case IMAGE_DEBUG_TYPE_COFF:
        return L"coff";
    case IMAGE_DEBUG_TYPE_CODEVIEW:
        return L"codeview";
    case IMAGE_DEBUG_TYPE_FPO:
        return L"fpo";
    case IMAGE_DEBUG_TYPE_MISC:
        return L"misc";
    case IMAGE_DEBUG_TYPE_EXCEPTION:
        return L"exception";
    case IMAGE_DEBUG_TYPE_FIXUP:
        return L"fixup";
    case IMAGE_DEBUG_TYPE_BORLAND:
        return L"borland";
    case IMAGE_DEBUG_TYPE_CLSID:
        return L"clsid";
    case IMAGE_DEBUG_TYPE_VC_FEATURE:
        return L"vc_feature";
    case IMAGE_DEBUG_TYPE_POGO:
        return L"pogo";
    case IMAGE_DEBUG_TYPE_ILTCG:
        return L"iltcg";
    case IMAGE_DEBUG_TYPE_MPX:
        return L"mpx";
    case IMAGE_DEBUG_TYPE_REPRO:
        return L"repro";
    }

    return NULL;
}

VOID AtpAddImageDebug(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    PH_MAPPED_IMAGE_DEBUG debug;
    PVOID entry;
    PVOID array;
    PVOID codeView;
    ULONG codeViewLength;
    ULONG i;

    entry = PhCreateJsonObject();
    array = PhCreateJsonArray();

    if (NT_SUCCESS(PhGetMappedImageDebug(MappedImage, &debug)))
    {
        for (i = 0; i < debug.NumberOfEntries; i++)
        {
            PPH_IMAGE_DEBUG_ENTRY debugEntry = &debug.DebugEntries[i];
            PVOID row = PhCreateJsonObject();
            LARGE_INTEGER time;

            AtJsonAddStringZ(row, "type", AtpImageDebugTypeString(debugEntry->Type));
            PhAddJsonObjectUInt64(row, "type_value", debugEntry->Type);
            PhAddJsonObjectUInt64(row, "size", debugEntry->SizeOfData);
            AtJsonAddHex(row, "address_of_raw_data", debugEntry->AddressOfRawData);
            AtJsonAddHex(row, "pointer_to_raw_data", debugEntry->PointerToRawData);

            if (debugEntry->TimeDateStamp)
            {
                PhSecondsSince1970ToTime(debugEntry->TimeDateStamp, &time);
                AtJsonAddTime(row, "time_date_stamp", &time);
            }
            else
            {
                AtJsonAddNull(row, "time_date_stamp");
            }

            PhAddJsonArrayObject(array, row);
        }

        PhFree(debug.DebugEntries);
    }

    PhAddJsonObjectValue(entry, "entries", array);

    // The CodeView record is the one anybody wants: it names the PDB that matches this exact build.
    if (NT_SUCCESS(PhGetMappedImageDebugEntryByType(MappedImage, IMAGE_DEBUG_TYPE_CODEVIEW, &codeViewLength, &codeView)) &&
        codeViewLength >= sizeof(CODEVIEW_INFO_PDB70))
    {
        PCODEVIEW_INFO_PDB70 pdb = codeView;

        if (pdb->Signature == CODEVIEW_SIGNATURE_RSDS)
        {
            PVOID pdbEntry = PhCreateJsonObject();
            PPH_STRING guid = PhFormatGuid(&pdb->PdbGuid);
            PPH_STRING name = PhZeroExtendToUtf16(pdb->ImageName);

            AtJsonAddString(pdbEntry, "guid", guid);
            PhAddJsonObjectUInt64(pdbEntry, "age", pdb->PdbAge);
            AtJsonAddString(pdbEntry, "path", name);
            PhAddJsonObjectValue(entry, "codeview", pdbEntry);
            PhClearReference(&guid);
            PhClearReference(&name);
        }
        else
        {
            AtJsonAddNull(entry, "codeview");
        }
    }
    else
    {
        AtJsonAddNull(entry, "codeview");
    }

    PhAddJsonObjectValue(Structured, "debug", entry);
}

PCWSTR AtpCertificateTypeString(
    _In_ USHORT Type
    )
{
    switch (Type)
    {
    case WIN_CERT_TYPE_X509:
        return L"x509";
    case WIN_CERT_TYPE_PKCS_SIGNED_DATA:
        return L"pkcs_signed_data";
    case WIN_CERT_TYPE_TS_STACK_SIGNED:
        return L"ts_stack_signed";
    }

    return NULL;
}

VOID AtpAddImageCertificates(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    PIMAGE_DATA_DIRECTORY directory;
    PVOID array;
    ULONG offset;
    ULONG end;

    array = PhCreateJsonArray();

    // The security directory is the one that does not hold an RVA: its VirtualAddress is a file
    // offset, because the certificate is not mapped when the image is loaded.
    if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        directory = &MappedImage->NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
    else
        directory = &MappedImage->NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];

    offset = directory->VirtualAddress;
    end = directory->VirtualAddress + directory->Size;

    while (directory->Size != 0 && offset + sizeof(WIN_CERTIFICATE) <= end && offset + sizeof(WIN_CERTIFICATE) <= MappedImage->ViewSize)
    {
        LPWIN_CERTIFICATE certificate = PTR_ADD_OFFSET(MappedImage->ViewBase, offset);
        PVOID row;

        if (certificate->dwLength < sizeof(WIN_CERTIFICATE) || offset + certificate->dwLength > end)
            break;

        row = PhCreateJsonObject();
        PhAddJsonObjectUInt64(row, "length", certificate->dwLength);
        AtJsonAddHex(row, "revision", certificate->wRevision);
        AtJsonAddStringZ(row, "type", AtpCertificateTypeString(certificate->wCertificateType));
        AtJsonAddHex(row, "type_value", certificate->wCertificateType);
        AtJsonAddHex(row, "offset", offset);
        PhAddJsonArrayObject(array, row);

        // Every entry is padded to an eight byte boundary.
        offset += ALIGN_UP_BY(certificate->dwLength, 8);
    }

    PhAddJsonObjectValue(Structured, "certificates", array);
}

// The Rich header is the linker's own record of what built the file: a list of tool ids and build
// numbers, obfuscated with a checksum key, that no compiler documents and every compiler writes. Two
// binaries built on the same machine with the same toolchain carry the same one, which is why it is
// used to group samples that share nothing else.
VOID AtpAddImageRichHeader(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    PH_MAPPED_IMAGE_PRODID prodId;
    PVOID entry;
    PVOID array;
    ULONG i;

    if (!NT_SUCCESS(PhGetMappedImageProdIdHeader(MappedImage, &prodId)))
    {
        AtJsonAddNull(Structured, "rich_header");
        return;
    }

    entry = PhCreateJsonObject();
    PhAddJsonObjectBoolean(entry, "valid", !!prodId.Valid);
    AtJsonAddString(entry, "checksum", prodId.Key);
    AtJsonAddString(entry, "hash", prodId.Hash);
    AtJsonAddString(entry, "raw_hash", prodId.RawHash);

    array = PhCreateJsonArray();

    for (i = 0; i < prodId.NumberOfEntries; i++)
    {
        PVOID row = PhCreateJsonObject();

        PhAddJsonObjectUInt64(row, "product_id", prodId.ProdIdEntries[i].ProductId);
        PhAddJsonObjectUInt64(row, "product_build", prodId.ProdIdEntries[i].ProductBuild);
        PhAddJsonObjectUInt64(row, "count", prodId.ProdIdEntries[i].ProductCount);
        PhAddJsonArrayObject(array, row);
    }

    PhAddJsonObjectValue(entry, "entries", array);
    PhAddJsonObjectValue(Structured, "rich_header", entry);

    PhClearReference(&prodId.Key);
    PhClearReference(&prodId.Hash);
    PhClearReference(&prodId.RawHash);

    if (prodId.ProdIdEntries)
        PhFree(prodId.ProdIdEntries);
}

// TLS callbacks run before the entry point does, on every thread. That makes them the quietest place
// in a PE to put code, so what is in the list matters more than that there is a list.
VOID AtpAddImageTls(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    PH_MAPPED_IMAGE_TLS_CALLBACKS tls;
    PVOID entry;
    PVOID array;
    ULONG i;

    if (!NT_SUCCESS(PhGetMappedImageTlsCallbacks(&tls, MappedImage)))
    {
        AtJsonAddNull(Structured, "tls");
        return;
    }

    entry = PhCreateJsonObject();

    if (MappedImage->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        AtJsonAddHex(entry, "start_address_of_raw_data", tls.TlsDirectory64->StartAddressOfRawData);
        AtJsonAddHex(entry, "end_address_of_raw_data", tls.TlsDirectory64->EndAddressOfRawData);
        AtJsonAddHex(entry, "address_of_index", tls.TlsDirectory64->AddressOfIndex);
        AtJsonAddHex(entry, "address_of_callbacks", tls.TlsDirectory64->AddressOfCallBacks);
        PhAddJsonObjectUInt64(entry, "size_of_zero_fill", tls.TlsDirectory64->SizeOfZeroFill);
        AtJsonAddHex(entry, "characteristics", tls.TlsDirectory64->Characteristics);
    }
    else
    {
        AtJsonAddHex(entry, "start_address_of_raw_data", tls.TlsDirectory32->StartAddressOfRawData);
        AtJsonAddHex(entry, "end_address_of_raw_data", tls.TlsDirectory32->EndAddressOfRawData);
        AtJsonAddHex(entry, "address_of_index", tls.TlsDirectory32->AddressOfIndex);
        AtJsonAddHex(entry, "address_of_callbacks", tls.TlsDirectory32->AddressOfCallBacks);
        PhAddJsonObjectUInt64(entry, "size_of_zero_fill", tls.TlsDirectory32->SizeOfZeroFill);
        AtJsonAddHex(entry, "characteristics", tls.TlsDirectory32->Characteristics);
    }

    array = PhCreateJsonArray();

    for (i = 0; i < tls.NumberOfEntries; i++)
    {
        PVOID row = PhCreateJsonObject();

        PhAddJsonObjectUInt64(row, "index", tls.Entries[i].Index);
        AtJsonAddHex(row, "address", tls.Entries[i].Address);
        PhAddJsonArrayObject(array, row);
    }

    PhAddJsonObjectValue(entry, "callbacks", array);
    PhAddJsonObjectValue(Structured, "tls", entry);

    if (tls.Entries)
        PhFree(tls.Entries);
}

PCWSTR AtpResourceTypeString(
    _In_ ULONG_PTR Type
    )
{
    if (!IS_INTRESOURCE(Type))
        return NULL;

    switch ((ULONG)Type)
    {
    case 1: return L"cursor";
    case 2: return L"bitmap";
    case 3: return L"icon";
    case 4: return L"menu";
    case 5: return L"dialog";
    case 6: return L"string";
    case 7: return L"fontdir";
    case 8: return L"font";
    case 9: return L"accelerator";
    case 10: return L"rcdata";
    case 11: return L"messagetable";
    case 12: return L"group_cursor";
    case 14: return L"group_icon";
    case 16: return L"version";
    case 17: return L"dlginclude";
    case 19: return L"plugplay";
    case 20: return L"vxd";
    case 21: return L"anicursor";
    case 22: return L"aniicon";
    case 23: return L"html";
    case 24: return L"manifest";
    }

    return NULL;
}

// A resource type or name is either a small integer or a pointer into the image to a counted string.
// The pointer comes from the file, so it is checked against the view before it is followed.
VOID AtpAddResourceIdentifier(
    _In_ PVOID Row,
    _In_ PCSTR Key,
    _In_ PCSTR NameKey,
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG_PTR Value
    )
{
    if (IS_INTRESOURCE(Value))
    {
        PhAddJsonObjectUInt64(Row, Key, (ULONG)Value);
        AtJsonAddNull(Row, NameKey);
        return;
    }

    AtJsonAddNull(Row, Key);
    AtJsonAddNull(Row, NameKey);

    if ((PVOID)Value >= MappedImage->ViewBase &&
        PTR_ADD_OFFSET(Value, sizeof(IMAGE_RESOURCE_DIR_STRING_U)) <= PTR_ADD_OFFSET(MappedImage->ViewBase, MappedImage->ViewSize))
    {
        PIMAGE_RESOURCE_DIR_STRING_U string = (PIMAGE_RESOURCE_DIR_STRING_U)Value;
        SIZE_T length = string->Length * sizeof(WCHAR);

        if (PTR_ADD_OFFSET(string->NameString, length) <= PTR_ADD_OFFSET(MappedImage->ViewBase, MappedImage->ViewSize))
        {
            PH_STRINGREF name;

            name.Buffer = string->NameString;
            name.Length = length;
            AtJsonAddStringRef(Row, NameKey, &name);
        }
    }
}

VOID AtpAddImageResources(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PAT_TOOL_CALL Call
    )
{
    PH_MAPPED_IMAGE_RESOURCES resources;
    AT_ROWS rows;
    SIZE_T i;

    AtInitializeRows(&rows, Call->Arguments);

    if (NT_SUCCESS(PhGetMappedImageResources(&resources, MappedImage)))
    {
        for (i = 0; i < resources.NumberOfEntries; i++)
        {
            PPH_IMAGE_RESOURCE_ENTRY resource = &resources.ResourceEntries[i];
            PVOID row = PhCreateJsonObject();

            AtpAddResourceIdentifier(row, "type_id", "type_name", MappedImage, resource->Type);
            AtJsonAddStringZ(row, "type", AtpResourceTypeString(resource->Type));
            AtpAddResourceIdentifier(row, "name_id", "name", MappedImage, resource->Name);
            PhAddJsonObjectUInt64(row, "language", resource->Language);
            AtJsonAddHex(row, "offset", resource->Offset);
            PhAddJsonObjectUInt64(row, "size", resource->Size);
            PhAddJsonObjectUInt64(row, "code_page", resource->CodePage);
            AtAddRow(&rows, row);
        }

        if (resources.ResourceEntries)
            PhFree(resources.ResourceEntries);
    }

    AtAddRows(Structured, "resources", &rows);
    AtDeleteRows(&rows);
}

VOID AtpAddImageClr(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    static CONST ULONG clrFlags[] =
    {
        COMIMAGE_FLAGS_ILONLY, COMIMAGE_FLAGS_32BITREQUIRED, COMIMAGE_FLAGS_IL_LIBRARY,
        COMIMAGE_FLAGS_STRONGNAMESIGNED, COMIMAGE_FLAGS_NATIVE_ENTRYPOINT,
        COMIMAGE_FLAGS_TRACKDEBUGDATA, COMIMAGE_FLAGS_32BITPREFERRED
    };
    static CONST PWSTR clrNames[] =
    {
        L"il_only", L"32bit_required", L"il_library",
        L"strong_name_signed", L"native_entry_point",
        L"track_debug_data", L"32bit_preferred"
    };
    PIMAGE_COR20_HEADER cor20;
    PVOID entry;

    // The COM descriptor directory is what makes a PE a managed assembly; a native binary has none.
    cor20 = PhGetMappedImageDirectoryEntry(MappedImage, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if (!cor20)
    {
        AtJsonAddNull(Structured, "clr");
        return;
    }

    entry = PhCreateJsonObject();
    PhAddJsonObjectUInt64(entry, "runtime_version_major", cor20->MajorRuntimeVersion);
    PhAddJsonObjectUInt64(entry, "runtime_version_minor", cor20->MinorRuntimeVersion);
    AtJsonAddHex(entry, "flags", cor20->Flags);
    AtJsonAddFlagStrings(entry, "flag_names", cor20->Flags,
        clrFlags, (CONST PWSTR*)clrNames, RTL_NUMBER_OF(clrFlags));
    AtJsonAddHex(entry, "entry_point_token", cor20->EntryPointToken);
    AtJsonAddHex(entry, "metadata_address", cor20->MetaData.VirtualAddress);
    PhAddJsonObjectUInt64(entry, "metadata_size", cor20->MetaData.Size);
    AtJsonAddHex(entry, "strong_name_signature_address", cor20->StrongNameSignature.VirtualAddress);
    PhAddJsonObjectUInt64(entry, "strong_name_signature_size", cor20->StrongNameSignature.Size);
    PhAddJsonObjectValue(Structured, "clr", entry);
}

// High entropy means compressed or encrypted, which is what a packer leaves behind - and also what a
// legitimately compressed resource section looks like, so it is a question rather than an answer.
VOID AtpAddImageEntropy(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    FLOAT entropy;
    FLOAT mean;
    FLOAT variance;
    PVOID entry;

    if (!PhGetMappedImageEntropy(MappedImage, &entropy, &mean, &variance))
    {
        AtJsonAddNull(Structured, "entropy");
        return;
    }

    entry = PhCreateJsonObject();
    PhAddJsonObjectDouble(entry, "entropy", entropy);
    PhAddJsonObjectDouble(entry, "mean", mean);
    PhAddJsonObjectDouble(entry, "variance", variance);
    PhAddJsonObjectValue(Structured, "entropy", entry);
}

VOID AtAddImageSections(
    _In_ PVOID Structured,
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PPH_STRING FileName,
    _In_ ULONG Sections,
    _In_ PAT_TOOL_CALL Call
    )
{
    if (Sections & AT_IMAGE_SECTION_HEADERS)
        AtpAddImageHeaders(Structured, MappedImage);

    if (Sections & AT_IMAGE_SECTION_DIRECTORIES)
        AtpAddImageDirectories(Structured, MappedImage);

    if (Sections & AT_IMAGE_SECTION_IMPORTS)
        AtpAddImageImports(Structured, MappedImage, Call);

    if (Sections & AT_IMAGE_SECTION_EXPORTS)
        AtpAddImageExports(Structured, MappedImage, Call);

    if (Sections & AT_IMAGE_SECTION_LOAD_CONFIG)
        AtpAddImageLoadConfig(Structured, MappedImage);

    if (Sections & AT_IMAGE_SECTION_MANIFEST)
        AtpAddImageManifest(Structured, MappedImage);

    if (Sections & AT_IMAGE_SECTION_VERSION_INFO)
        AtpAddImageVersionInfo(Structured, FileName);

    if (Sections & AT_IMAGE_SECTION_DEBUG)
        AtpAddImageDebug(Structured, MappedImage);

    if (Sections & AT_IMAGE_SECTION_CERTIFICATES)
        AtpAddImageCertificates(Structured, MappedImage);

    if (Sections & AT_IMAGE_SECTION_RICH_HEADER)
        AtpAddImageRichHeader(Structured, MappedImage);

    if (Sections & AT_IMAGE_SECTION_TLS)
        AtpAddImageTls(Structured, MappedImage);

    if (Sections & AT_IMAGE_SECTION_RESOURCES)
        AtpAddImageResources(Structured, MappedImage, Call);

    if (Sections & AT_IMAGE_SECTION_CLR)
        AtpAddImageClr(Structured, MappedImage);

    if (Sections & AT_IMAGE_SECTION_ENTROPY)
        AtpAddImageEntropy(Structured, MappedImage);
}
