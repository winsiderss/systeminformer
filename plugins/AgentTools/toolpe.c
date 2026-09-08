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

#define AT_READ_MEMORY_MAX (64 * 1024)
#define AT_SEARCH_DEFAULT_RESULTS 100
#define AT_SEARCH_MAX_RESULTS 1000
#define AT_SEARCH_MAX_PATTERN 256

PCWSTR AtpVerifyResultText(
    _In_ VERIFY_RESULT Result
    )
{
    PCWSTR text = AtVerifyResultString(Result);

    return text ? text : L"Unknown";
}

VOID AtpVerifyFileSignature(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING path;
    VERIFY_RESULT verifyResult;
    PPH_STRING signer = NULL;
    PVOID structured;

    if (!(path = AtGetArgumentString(Call->Arguments, "path")) || path->Length == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"path is required.");
        PhClearReference(&path);
        return;
    }

    verifyResult = PhVerifyFile(PhGetString(path), &signer);

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "path", path);
    AtJsonAddStringZ(structured, "verify_result", AtpVerifyResultText(verifyResult));
    PhAddJsonObjectBoolean(structured, "is_trusted", verifyResult == VrTrusted);
    AtJsonAddString(structured, "signer", signer);

    Result->StructuredContent = structured;

    PhClearReference(&signer);
    PhDereferenceObject(path);
}

VOID AtpGetImageInfo(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    static CONST ULONG fileFlags[] =
    {
        IMAGE_FILE_EXECUTABLE_IMAGE, IMAGE_FILE_DLL, IMAGE_FILE_SYSTEM,
        IMAGE_FILE_LARGE_ADDRESS_AWARE, IMAGE_FILE_RELOCS_STRIPPED, IMAGE_FILE_DEBUG_STRIPPED
    };
    static CONST PWSTR fileNames[] =
    {
        L"executable", L"dll", L"system",
        L"large_address_aware", L"relocs_stripped", L"debug_stripped"
    };
    static CONST ULONG dllFlags[] =
    {
        IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA, IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE,
        IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY, IMAGE_DLLCHARACTERISTICS_NX_COMPAT,
        IMAGE_DLLCHARACTERISTICS_GUARD_CF, IMAGE_DLLCHARACTERISTICS_APPCONTAINER,
        IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE
    };
    static CONST PWSTR dllNames[] =
    {
        L"high_entropy_va", L"dynamic_base", L"force_integrity", L"nx_compat",
        L"guard_cf", L"appcontainer", L"terminal_server_aware"
    };
    static CONST ULONG scnFlags[] =
    {
        IMAGE_SCN_CNT_CODE, IMAGE_SCN_CNT_INITIALIZED_DATA, IMAGE_SCN_CNT_UNINITIALIZED_DATA,
        IMAGE_SCN_MEM_EXECUTE, IMAGE_SCN_MEM_READ, IMAGE_SCN_MEM_WRITE
    };
    static CONST PWSTR scnNames[] =
    {
        L"code", L"initialized_data", L"uninitialized_data", L"execute", L"read", L"write"
    };
    NTSTATUS status;
    PPH_STRING path;
    HANDLE fileHandle;
    PH_MAPPED_IMAGE mappedImage;
    PIMAGE_NT_HEADERS ntHeaders;
    BOOLEAN is64;
    ULONG timeStamp;
    ULONG sizeOfImage;
    ULONG checkSum;
    ULONG entryPoint;
    ULONG64 imageBase;
    USHORT subsystem;
    USHORT dllCharacteristics;
    VERIFY_RESULT verifyResult;
    PPH_STRING signer = NULL;
    PVOID structured;
    PVOID sections;
    USHORT i;

    if (!(path = AtGetArgumentString(Call->Arguments, "path")) || path->Length == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"path is required.");
        PhClearReference(&path);
        return;
    }

    // Open the Win32 path read-only and let the mapper work on the handle; passing a Win32 path
    // straight to PhLoadMappedImageEx would be treated as an NT path.
    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(path),
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Opening the file");
        PhDereferenceObject(path);
        return;
    }

    status = PhLoadMappedImageEx(NULL, fileHandle, &mappedImage);
    NtClose(fileHandle);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Loading the image");
        PhDereferenceObject(path);
        return;
    }

    if (mappedImage.Signature != IMAGE_DOS_SIGNATURE)
    {
        AtSetToolError(Result, "invalid_image", STATUS_INVALID_IMAGE_FORMAT, L"The file is not a PE image.");
        PhUnloadMappedImage(&mappedImage);
        PhDereferenceObject(path);
        return;
    }

    ntHeaders = mappedImage.NtHeaders;
    is64 = mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "path", path);
    AtJsonAddStringZ(structured, "machine", AtMachineString(ntHeaders->FileHeader.Machine));
    PhAddJsonObjectBoolean(structured, "is_64bit", is64);

    if (is64)
    {
        PIMAGE_OPTIONAL_HEADER64 opt = &mappedImage.NtHeaders64->OptionalHeader;

        subsystem = opt->Subsystem;
        dllCharacteristics = opt->DllCharacteristics;
        entryPoint = opt->AddressOfEntryPoint;
        imageBase = opt->ImageBase;
        sizeOfImage = opt->SizeOfImage;
        checkSum = opt->CheckSum;
    }
    else
    {
        PIMAGE_OPTIONAL_HEADER32 opt = &mappedImage.NtHeaders32->OptionalHeader;

        subsystem = opt->Subsystem;
        dllCharacteristics = opt->DllCharacteristics;
        entryPoint = opt->AddressOfEntryPoint;
        imageBase = opt->ImageBase;
        sizeOfImage = opt->SizeOfImage;
        checkSum = opt->CheckSum;
    }

    AtJsonAddStringZ(structured, "subsystem", AtSubsystemString(subsystem));

    timeStamp = ntHeaders->FileHeader.TimeDateStamp;

    if (timeStamp)
    {
        LARGE_INTEGER time;

        PhSecondsSince1970ToTime(timeStamp, &time);
        AtJsonAddTime(structured, "time_date_stamp", &time);
    }
    else
    {
        AtJsonAddNull(structured, "time_date_stamp");
    }

    AtJsonAddPointer(structured, "entry_point", (PVOID)(ULONG_PTR)entryPoint);
    AtJsonAddPointer(structured, "image_base", (PVOID)(ULONG_PTR)imageBase);
    PhAddJsonObjectUInt64(structured, "size_of_image", sizeOfImage);
    PhAddJsonObjectUInt64(structured, "checksum", checkSum);

    AtJsonAddFlagStrings(structured, "characteristics", ntHeaders->FileHeader.Characteristics, fileFlags, (CONST PWSTR*)fileNames, RTL_NUMBER_OF(fileFlags));
    AtJsonAddFlagStrings(structured, "dll_characteristics", dllCharacteristics, dllFlags, (CONST PWSTR*)dllNames, RTL_NUMBER_OF(dllFlags));

    sections = PhCreateJsonArray();

    for (i = 0; i < mappedImage.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER section = &mappedImage.Sections[i];
        CHAR name[IMAGE_SIZEOF_SHORT_NAME + 1];
        PVOID row;

        memcpy(name, section->Name, IMAGE_SIZEOF_SHORT_NAME);
        name[IMAGE_SIZEOF_SHORT_NAME] = ANSI_NULL;

        row = PhCreateJsonObject();
        PhAddJsonObject(row, "name", name);
        AtJsonAddPointer(row, "virtual_address", (PVOID)(ULONG_PTR)section->VirtualAddress);
        PhAddJsonObjectUInt64(row, "virtual_size", section->Misc.VirtualSize);
        PhAddJsonObjectUInt64(row, "raw_size", section->SizeOfRawData);
        AtJsonAddFlagStrings(row, "characteristics", section->Characteristics, scnFlags, (CONST PWSTR*)scnNames, RTL_NUMBER_OF(scnFlags));
        PhAddJsonArrayObject(sections, row);
    }

    PhAddJsonObjectValue(structured, "sections", sections);
    PhAddJsonObjectUInt64(structured, "section_count", mappedImage.NumberOfSections);

    PhUnloadMappedImage(&mappedImage);

    verifyResult = PhVerifyFile(PhGetString(path), &signer);
    AtJsonAddStringZ(structured, "verify_result", AtpVerifyResultText(verifyResult));
    AtJsonAddString(structured, "verify_signer", signer);
    PhClearReference(&signer);

    Result->StructuredContent = structured;

    PhDereferenceObject(path);
}

VOID AtpRenderBytes(
    _In_ PVOID Object,
    _In_reads_bytes_(Size) PUCHAR Bytes,
    _In_ SIZE_T Size
    )
{
    static CONST CHAR digits[] = "0123456789abcdef";
    PSTR hex;
    PSTR ascii;
    SIZE_T i;

    hex = PhAllocate(Size * 2 + 1);
    ascii = PhAllocate(Size + 1);

    for (i = 0; i < Size; i++)
    {
        hex[i * 2] = digits[Bytes[i] >> 4];
        hex[i * 2 + 1] = digits[Bytes[i] & 0xf];
        ascii[i] = (Bytes[i] >= 0x20 && Bytes[i] < 0x7f) ? (CHAR)Bytes[i] : '.';
    }

    hex[Size * 2] = ANSI_NULL;
    ascii[Size] = ANSI_NULL;

    PhAddJsonObject(Object, "hex", hex);
    PhAddJsonObject(Object, "ascii", ascii);

    PhFree(hex);
    PhFree(ascii);
}

VOID AtpReadProcessMemory(
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    ULONG64 address;
    ULONG64 size;
    PVOID buffer;
    SIZE_T bytesRead = 0;
    PVOID structured;

    if (!AtGetArgumentPointer(Call->Arguments, "address", &address) || address == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"address is required.");
        return;
    }

    if (!AtGetArgumentUInt64(Call->Arguments, "size", &size) || size == 0 || size > AT_READ_MEMORY_MAX)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"size is required and must be 1 to %lu.", (ULONG)AT_READ_MEMORY_MAX);
        return;
    }

    buffer = PhAllocate((SIZE_T)size);

    status = PhReadVirtualMemory(Target->ProcessHandle, (PVOID)(ULONG_PTR)address, buffer, (SIZE_T)size, &bytesRead);

    if (!NT_SUCCESS(status) && bytesRead == 0)
    {
        AtSetToolStatusError(Result, status, L"Reading process memory");
        PhFree(buffer);
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    AtJsonAddPointer(structured, "address", (PVOID)(ULONG_PTR)address);
    PhAddJsonObjectUInt64(structured, "size", bytesRead);
    AtpRenderBytes(structured, buffer, bytesRead);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhFree(buffer);
}

BOOLEAN AtpHexDigit(
    _In_ WCHAR Character,
    _Out_ PUCHAR Value
    )
{
    if (Character >= L'0' && Character <= L'9')
        *Value = (UCHAR)(Character - L'0');
    else if (Character >= L'a' && Character <= L'f')
        *Value = (UCHAR)(Character - L'a' + 10);
    else if (Character >= L'A' && Character <= L'F')
        *Value = (UCHAR)(Character - L'A' + 10);
    else
        return FALSE;

    return TRUE;
}

BOOLEAN AtpBuildSearchPattern(
    _In_opt_ PVOID Arguments,
    _Out_writes_bytes_to_(AT_SEARCH_MAX_PATTERN, *Length) PUCHAR Pattern,
    _Out_ PULONG Length,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING hex;
    PPH_STRING ascii;
    PPH_STRING utf16;
    ULONG count = 0;

    hex = AtGetArgumentString(Arguments, "pattern_hex");
    ascii = AtGetArgumentString(Arguments, "ascii");
    utf16 = AtGetArgumentString(Arguments, "utf16");

    if (!!hex + !!ascii + !!utf16 != 1)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"Exactly one of pattern_hex, ascii and utf16 is required.");
        goto CleanupExit;
    }

    if (hex)
    {
        PH_STRINGREF sr = hex->sr;
        SIZE_T i;

        if (sr.Length == 0 || (sr.Length / sizeof(WCHAR)) % 2 != 0)
        {
            AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"pattern_hex must be an even number of hex digits.");
            goto CleanupExit;
        }

        for (i = 0; i < sr.Length / sizeof(WCHAR); i += 2)
        {
            UCHAR high;
            UCHAR low;

            if (count >= AT_SEARCH_MAX_PATTERN)
                break;

            if (!AtpHexDigit(sr.Buffer[i], &high) || !AtpHexDigit(sr.Buffer[i + 1], &low))
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"pattern_hex contains a non-hex character.");
                goto CleanupExit;
            }

            Pattern[count++] = (UCHAR)((high << 4) | low);
        }
    }
    else if (ascii)
    {
        PPH_BYTES bytes = PhConvertUtf16ToUtf8Ex(ascii->Buffer, ascii->Length);

        if (bytes)
        {
            count = (ULONG)min(bytes->Length, AT_SEARCH_MAX_PATTERN);
            memcpy(Pattern, bytes->Buffer, count);
            PhDereferenceObject(bytes);
        }
    }
    else
    {
        count = (ULONG)min(utf16->Length, AT_SEARCH_MAX_PATTERN);
        memcpy(Pattern, utf16->Buffer, count);
    }

    if (count == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"The search pattern is empty.");
        goto CleanupExit;
    }

    *Length = count;

CleanupExit:
    PhClearReference(&hex);
    PhClearReference(&ascii);
    PhClearReference(&utf16);

    return count != 0 && !Result->ErrorCode;
}

VOID AtpSearchProcessMemory(
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UCHAR pattern[AT_SEARCH_MAX_PATTERN];
    ULONG patternLength = 0;
    ULONG64 maxResults;
    ULONG limit = AT_SEARCH_DEFAULT_RESULTS;
    PH_MEMORY_ITEM_LIST list;
    PLIST_ENTRY entry;
    PVOID buffer;
    ULONG bufferSize = 1024 * 1024;
    ULONG64 bytesScanned = 0;
    PVOID structured;
    PVOID matches;
    ULONG count = 0;
    BOOLEAN truncated = FALSE;

    if (!AtpBuildSearchPattern(Call->Arguments, pattern, &patternLength, Result))
        return;

    if (AtGetArgumentUInt64(Call->Arguments, "max_results", &maxResults) && maxResults > 0)
        limit = (ULONG)min(maxResults, AT_SEARCH_MAX_RESULTS);

    if (!NT_SUCCESS(PhQueryMemoryItemList(Target->ProcessItem->ProcessId, PH_QUERY_MEMORY_IGNORE_FREE, &list)))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"The process memory could not be enumerated.");
        return;
    }

    // PhAllocatePage returns NULL on failure (unlike PhAllocate, which raises); bail before building
    // any JSON so nothing leaks.
    if (!(buffer = PhAllocatePage(bufferSize, NULL)))
    {
        AtSetToolError(Result, "failed", STATUS_NO_MEMORY, L"The scan buffer could not be allocated.");
        PhDeleteMemoryItemList(&list);
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    matches = PhCreateJsonArray();

    for (entry = list.ListHead.Flink; entry != &list.ListHead && !truncated; entry = entry->Flink)
    {
        PPH_MEMORY_ITEM item = CONTAINING_RECORD(entry, PH_MEMORY_ITEM, ListEntry);
        ULONG_PTR base;
        SIZE_T remaining;

        // Only committed, readable, non-guard regions.
        if (!(item->State & MEM_COMMIT))
            continue;
        if (item->Protect & (PAGE_NOACCESS | PAGE_GUARD))
            continue;
        if (!(item->Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
            continue;

        base = (ULONG_PTR)item->BaseAddress;
        remaining = item->RegionSize;

        while (remaining > 0 && !truncated)
        {
            SIZE_T chunk = min(remaining, bufferSize);
            SIZE_T read = 0;
            SIZE_T i;

            if (NT_SUCCESS(PhReadVirtualMemory(Target->ProcessHandle, (PVOID)base, buffer, chunk, &read)) && read >= patternLength)
            {
                bytesScanned += read;

                for (i = 0; i + patternLength <= read; i++)
                {
                    if (memcmp((PUCHAR)buffer + i, pattern, patternLength) == 0)
                    {
                        PH_FORMAT format[2];
                        PPH_STRING addressString;
                        PPH_BYTES utf8;

                        PhInitFormatS(&format[0], L"0x");
                        PhInitFormatIX(&format[1], base + i);
                        addressString = PhFormat(format, RTL_NUMBER_OF(format), 20);

                        if (utf8 = PhConvertUtf16ToUtf8Ex(addressString->Buffer, addressString->Length))
                        {
                            PhAddJsonArrayObject(matches, PhCreateJsonStringObject(utf8->Buffer));
                            PhDereferenceObject(utf8);
                        }

                        PhDereferenceObject(addressString);
                        count++;

                        if (count >= limit)
                        {
                            truncated = TRUE;
                            break;
                        }
                    }
                }
            }

            base += chunk;
            remaining -= chunk;
        }
    }

    PhFreePage(buffer);
    PhDeleteMemoryItemList(&list);

    PhAddJsonObjectValue(structured, "matches", matches);
    PhAddJsonObjectUInt64(structured, "count", count);
    PhAddJsonObjectBoolean(structured, "truncated", truncated);
    PhAddJsonObjectUInt64(structured, "bytes_scanned", bytesScanned);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtPeInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    switch (Tool->Action)
    {
    case AtActionVerifyFileSignature:
        AtpVerifyFileSignature(Call, Result);
        break;
    case AtActionGetImageInfo:
        AtpGetImageInfo(Call, Result);
        break;
    case AtActionReadProcessMemory:
        AtpReadProcessMemory(Call, Target, Result);
        break;
    case AtActionSearchProcessMemory:
        AtpSearchProcessMemory(Call, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
