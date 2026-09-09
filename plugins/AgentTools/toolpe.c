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
#include <mapldr.h>
#include <wincrypt.h>
#include <phcrypt.h>
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

typedef DWORD (WINAPI* AT_CERT_GET_NAME_STRING_W)(
    _In_ PCCERT_CONTEXT CertContext,
    _In_ DWORD Type,
    _In_ DWORD Flags,
    _In_opt_ PVOID TypePara,
    _Out_writes_opt_(NameStringSize) PWSTR NameString,
    _In_ DWORD NameStringSize
    );

typedef BOOL (WINAPI* AT_CERT_GET_CONTEXT_PROPERTY)(
    _In_ PCCERT_CONTEXT CertContext,
    _In_ DWORD PropId,
    _Out_writes_bytes_to_opt_(*DataSize, *DataSize) PVOID Data,
    _Inout_ PDWORD DataSize
    );

PPH_STRING AtpGetCertificateName(
    _In_ PCERT_CONTEXT Certificate,
    _In_ ULONG Flags
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static AT_CERT_GET_NAME_STRING_W CertGetNameStringW_I = NULL;
    WCHAR buffer[256];
    ULONG length;

    if (PhBeginInitOnce(&initOnce))
    {
        CertGetNameStringW_I = PhGetModuleProcAddress(L"crypt32.dll", "CertGetNameStringW");
        PhEndInitOnce(&initOnce);
    }

    if (!CertGetNameStringW_I)
        return NULL;

    length = CertGetNameStringW_I(
        (PCCERT_CONTEXT)Certificate,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        Flags,
        NULL,
        buffer,
        RTL_NUMBER_OF(buffer)
        );

    // One character back means the empty string plus its terminator.
    if (length <= 1)
        return NULL;

    return PhCreateStringEx(buffer, (length - 1) * sizeof(WCHAR));
}

PPH_STRING AtpGetCertificateThumbprint(
    _In_ PCERT_CONTEXT Certificate
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static AT_CERT_GET_CONTEXT_PROPERTY CertGetCertificateContextProperty_I = NULL;
    UCHAR hash[32];
    ULONG hashLength = sizeof(hash);

    if (PhBeginInitOnce(&initOnce))
    {
        CertGetCertificateContextProperty_I =
            PhGetModuleProcAddress(L"crypt32.dll", "CertGetCertificateContextProperty");
        PhEndInitOnce(&initOnce);
    }

    if (!CertGetCertificateContextProperty_I)
        return NULL;

    if (!CertGetCertificateContextProperty_I((PCCERT_CONTEXT)Certificate, CERT_SHA1_HASH_PROP_ID, hash, &hashLength))
        return NULL;

    return PhBufferToHexString(hash, hashLength);
}

PPH_STRING AtpGetCertificateSerialNumber(
    _In_ PCERT_CONTEXT Certificate
    )
{
    static CONST WCHAR digits[] = L"0123456789ABCDEF";
    PCRYPT_INTEGER_BLOB serial = &((PCCERT_CONTEXT)Certificate)->pCertInfo->SerialNumber;
    PPH_STRING string;
    ULONG i;

    if (serial->cbData == 0)
        return NULL;

    string = PhCreateStringEx(NULL, serial->cbData * 2 * sizeof(WCHAR));

    for (i = 0; i < serial->cbData; i++)
    {
        UCHAR value = serial->pbData[serial->cbData - i - 1];

        string->Buffer[i * 2] = digits[value >> 4];
        string->Buffer[i * 2 + 1] = digits[value & 0xf];
    }

    return string;
}

VOID AtpAddCertificate(
    _In_ PAT_ROWS Rows,
    _In_ PCERT_CONTEXT Certificate,
    _In_ BOOLEAN IsPrimary
    )
{
    PVOID row;
    PPH_STRING string;
    PCERT_INFO certificateInfo = ((PCCERT_CONTEXT)Certificate)->pCertInfo;

    row = PhCreateJsonObject();
    PhAddJsonObjectBoolean(row, "is_primary", IsPrimary);

    string = PhGetSignerNameFromCertificate(Certificate);
    AtJsonAddString(row, "signer", string);
    PhClearReference(&string);

    string = AtpGetCertificateName(Certificate, 0);
    AtJsonAddString(row, "subject", string);
    PhClearReference(&string);

    string = AtpGetCertificateName(Certificate, CERT_NAME_ISSUER_FLAG);
    AtJsonAddString(row, "issuer", string);
    PhClearReference(&string);

    string = AtpGetCertificateThumbprint(Certificate);
    AtJsonAddString(row, "thumbprint", string);
    PhClearReference(&string);

    string = AtpGetCertificateSerialNumber(Certificate);
    AtJsonAddString(row, "serial_number", string);
    PhClearReference(&string);

    AtJsonAddTime(row, "not_before", (PLARGE_INTEGER)&certificateInfo->NotBefore);
    AtJsonAddTime(row, "not_after", (PLARGE_INTEGER)&certificateInfo->NotAfter);

    AtAddRow(Rows, row);
}

BOOLEAN AtpHasEmbeddedSignature(
    _In_ HANDLE FileHandle,
    _Out_ PBOOLEAN IsImage
    )
{
    PH_MAPPED_IMAGE mappedImage;
    BOOLEAN embedded = FALSE;

    *IsImage = FALSE;

    if (!NT_SUCCESS(PhLoadMappedImageEx(NULL, FileHandle, &mappedImage)))
        return FALSE;

    if (mappedImage.Signature == IMAGE_DOS_SIGNATURE)
    {
        *IsImage = TRUE;

        if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        {
            embedded = mappedImage.NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size != 0;
        }
        else if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            embedded = mappedImage.NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size != 0;
        }
    }

    PhUnloadMappedImage(&mappedImage);

    return embedded;
}

VOID AtpVerifyFileSignature(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PPH_STRING path;
    PH_VERIFY_FILE_INFO info;
    HANDLE fileHandle;
    VERIFY_RESULT verifyResult = VrUnknown;
    PCERT_CONTEXT *signatures = NULL;
    ULONG numberOfSignatures = 0;
    PPH_STRING signer = NULL;
    BOOLEAN includeChain;
    BOOLEAN embedded;
    BOOLEAN isImage;
    PVOID structured;

    if (!(path = AtGetArgumentString(Call->Arguments, "path")) || path->Length == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"path is required.");
        PhClearReference(&path);
        return;
    }

    includeChain = AtJsonGetObjectBoolean(Call->Arguments, "include_chain");

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

    // Never let a signature check reach the network: a revocation lookup on an attacker-chosen file
    // is an outbound request the caller did not ask for.
    memset(&info, 0, sizeof(PH_VERIFY_FILE_INFO));
    info.FileHandle = fileHandle;
    info.Flags = PH_VERIFY_PREVENT_NETWORK_ACCESS;

    status = PhVerifyFileEx(&info, &verifyResult, &signatures, &numberOfSignatures);

    if (NT_SUCCESS(status) && numberOfSignatures != 0)
        signer = PhGetSignerNameFromCertificate(signatures[0]);

    embedded = AtpHasEmbeddedSignature(fileHandle, &isImage);

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "path", path);
    AtJsonAddStringZ(structured, "verify_result", AtpVerifyResultText(verifyResult));
    PhAddJsonObjectBoolean(structured, "is_trusted", verifyResult == VrTrusted);
    AtJsonAddString(structured, "signer", signer);
    PhAddJsonObjectBoolean(structured, "has_embedded_signature", embedded);

    // A trusted file with no signature of its own was vouched for by a catalog.
    if (embedded)
        AtJsonAddStringZ(structured, "signature_source", L"embedded");
    else if (verifyResult == VrTrusted)
        AtJsonAddStringZ(structured, "signature_source", L"catalog");
    else
        AtJsonAddNull(structured, "signature_source");

    PhAddJsonObjectBoolean(structured, "is_pe_image", isImage);

    // A second pass, and the one bit that matters most: whether the chain ends at Microsoft's root
    // rather than at any root the machine happens to trust.
    AtJsonAddMicrosoftSigned(structured, "is_microsoft_signed", path);

    if (includeChain)
    {
        AT_ROWS rows;
        ULONG i;

        AtInitializeRows(&rows, Call->Arguments);

        for (i = 0; i < numberOfSignatures; i++)
            AtpAddCertificate(&rows, signatures[i], i == 0);

        AtAddRows(structured, "signatures", &rows);
        AtDeleteRows(&rows);
    }

    Result->StructuredContent = structured;

    PhFreeVerifySignatures(signatures, numberOfSignatures);
    PhClearReference(&signer);
    NtClose(fileHandle);
    PhDereferenceObject(path);
}

#define AT_HASH_CHUNK_SIZE (1024 * 1024)
#define AT_HASH_MAXIMUM_SIZE (2ULL * 1024 * 1024 * 1024)

typedef struct _AT_HASH_REQUEST
{
    PCSTR Key;
    PCWSTR Name;
    PH_SYMCRYPT_HASH_ALGORITHM Algorithm;
    ULONG Size;
    BOOLEAN Wanted;
    PH_SYMCRYPT_HASH_CONTEXT Context;
} AT_HASH_REQUEST, *PAT_HASH_REQUEST;

PPH_STRING AtHashFileSha256(
    _In_ PPH_STRING FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    PH_SYMCRYPT_HASH_CONTEXT hashContext;
    UCHAR hash[PH_SYMCRYPT_SHA256_RESULT_SIZE];
    PVOID buffer;
    LARGE_INTEGER fileSize;
    LARGE_INTEGER offset;
    PPH_STRING result = NULL;

    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(FileName),
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return NULL;

    if (!NT_SUCCESS(PhGetFileSize(fileHandle, &fileSize)) ||
        (ULONG64)fileSize.QuadPart > AT_HASH_MAXIMUM_SIZE ||
        !NT_SUCCESS(PhSymCryptHashInit(PH_SYMCRYPT_SHA256_ALGORITHM, &hashContext)))
    {
        NtClose(fileHandle);
        return NULL;
    }

    buffer = PhAllocate(AT_HASH_CHUNK_SIZE);
    offset.QuadPart = 0;

    while (offset.QuadPart < fileSize.QuadPart)
    {
        ULONG read = 0;

        if (!NT_SUCCESS(PhReadFile(fileHandle, buffer, AT_HASH_CHUNK_SIZE, &offset, &read)) || read == 0)
            break;

        PhSymCryptHashData(&hashContext, buffer, read);
        offset.QuadPart += read;
    }

    PhFree(buffer);

    if (offset.QuadPart == fileSize.QuadPart &&
        NT_SUCCESS(PhSymCryptHashFinal(&hashContext, hash, sizeof(hash))))
    {
        result = PhBufferToHexStringEx(hash, sizeof(hash), FALSE);
    }

    PhSymCryptDestroyHash(&hashContext, sizeof(hash));
    NtClose(fileHandle);

    return result;
}

VOID AtpGetFileHashes(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_HASH_REQUEST requests[] =
    {
        { "md5", L"md5", PH_SYMCRYPT_MD5_ALGORITHM, PH_SYMCRYPT_MD5_RESULT_SIZE, FALSE },
        { "sha1", L"sha1", PH_SYMCRYPT_SHA1_ALGORITHM, PH_SYMCRYPT_SHA1_RESULT_SIZE, FALSE },
        { "sha256", L"sha256", PH_SYMCRYPT_SHA256_ALGORITHM, PH_SYMCRYPT_SHA256_RESULT_SIZE, FALSE },
        { "sha512", L"sha512", PH_SYMCRYPT_SHA512_ALGORITHM, PH_SYMCRYPT_SHA512_RESULT_SIZE, FALSE },
    };
    NTSTATUS status;
    PPH_STRING path;
    HANDLE fileHandle;
    PVOID algorithms;
    PVOID buffer;
    PVOID structured;
    LARGE_INTEGER fileSize;
    LARGE_INTEGER offset;
    PH_MAPPED_IMAGE mappedImage;
    BOOLEAN isImage = FALSE;
    ULONG wanted = 0;
    ULONG i;

    if (!(path = AtGetArgumentString(Call->Arguments, "path")) || path->Length == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"path is required.");
        PhClearReference(&path);
        return;
    }

    if (algorithms = AtJsonGetObjectMember(Call->Arguments, "algorithms", PH_JSON_OBJECT_TYPE_ARRAY))
    {
        ULONG count = PhGetJsonArrayLength(algorithms);
        ULONG j;

        for (j = 0; j < count; j++)
        {
            PPH_STRING name = PhGetJsonObjectString(PhGetJsonArrayIndexObject(algorithms, j));

            if (!name)
                continue;

            for (i = 0; i < RTL_NUMBER_OF(requests); i++)
            {
                if (PhEqualString2(name, requests[i].Name, TRUE))
                {
                    requests[i].Wanted = TRUE;
                    break;
                }
            }

            if (i == RTL_NUMBER_OF(requests))
            {
                AtSetToolError(
                    Result,
                    "invalid_arguments",
                    STATUS_INVALID_PARAMETER,
                    L"%s is not one of md5, sha1, sha256 or sha512.",
                    PhGetString(name)
                    );
                PhDereferenceObject(name);
                PhDereferenceObject(path);
                return;
            }

            PhDereferenceObject(name);
        }
    }
    else
    {
        // The three every lookup service is keyed on.
        requests[0].Wanted = TRUE;
        requests[1].Wanted = TRUE;
        requests[2].Wanted = TRUE;
    }

    for (i = 0; i < RTL_NUMBER_OF(requests); i++)
    {
        if (requests[i].Wanted)
            wanted++;
    }

    if (wanted == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"algorithms was empty.");
        PhDereferenceObject(path);
        return;
    }

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

    if (!NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize)))
    {
        AtSetToolStatusError(Result, status, L"Reading the file size");
        NtClose(fileHandle);
        PhDereferenceObject(path);
        return;
    }

    // The whole file has to be read to hash it and the server answers one call at a time, so there
    // is a size past which this is not a reasonable thing to ask for.
    if ((ULONG64)fileSize.QuadPart > AT_HASH_MAXIMUM_SIZE)
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_FILE_TOO_LARGE,
            L"The file is %llu bytes; hashing is limited to %llu.",
            (ULONG64)fileSize.QuadPart,
            (ULONG64)AT_HASH_MAXIMUM_SIZE
            );
        NtClose(fileHandle);
        PhDereferenceObject(path);
        return;
    }

    for (i = 0; i < RTL_NUMBER_OF(requests); i++)
    {
        if (requests[i].Wanted && !NT_SUCCESS(status = PhSymCryptHashInit(requests[i].Algorithm, &requests[i].Context)))
        {
            AtSetToolStatusError(Result, status, L"Starting the hash");
            NtClose(fileHandle);
            PhDereferenceObject(path);
            return;
        }
    }

    // One pass over the file feeding every requested hash, rather than a pass each.
    buffer = PhAllocate(AT_HASH_CHUNK_SIZE);
    offset.QuadPart = 0;

    while (offset.QuadPart < fileSize.QuadPart)
    {
        ULONG read = 0;

        status = PhReadFile(fileHandle, buffer, AT_HASH_CHUNK_SIZE, &offset, &read);

        if (!NT_SUCCESS(status) || read == 0)
            break;

        for (i = 0; i < RTL_NUMBER_OF(requests); i++)
        {
            if (requests[i].Wanted)
                PhSymCryptHashData(&requests[i].Context, buffer, read);
        }

        offset.QuadPart += read;
    }

    PhFree(buffer);

    if (offset.QuadPart != fileSize.QuadPart)
    {
        AtSetToolStatusError(Result, NT_SUCCESS(status) ? STATUS_END_OF_FILE : status, L"Reading the file");

        for (i = 0; i < RTL_NUMBER_OF(requests); i++)
        {
            if (requests[i].Wanted)
                PhSymCryptDestroyHash(&requests[i].Context, requests[i].Size);
        }

        NtClose(fileHandle);
        PhDereferenceObject(path);
        return;
    }

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "path", path);
    PhAddJsonObjectUInt64(structured, "size", fileSize.QuadPart);

    for (i = 0; i < RTL_NUMBER_OF(requests); i++)
    {
        UCHAR hash[PH_SYMCRYPT_SHA512_RESULT_SIZE];
        PPH_STRING string;

        if (!requests[i].Wanted)
        {
            AtJsonAddNull(structured, requests[i].Key);
            continue;
        }

        if (NT_SUCCESS(PhSymCryptHashFinal(&requests[i].Context, hash, requests[i].Size)) &&
            (string = PhBufferToHexStringEx(hash, requests[i].Size, FALSE)))
        {
            AtJsonAddString(structured, requests[i].Key, string);
            PhDereferenceObject(string);
        }
        else
        {
            AtJsonAddNull(structured, requests[i].Key);
        }

        PhSymCryptDestroyHash(&requests[i].Context, requests[i].Size);
    }

    // The hashes that only mean anything for a PE image, and only the driver of the format knows
    // which bytes they cover.
    if (NT_SUCCESS(PhLoadMappedImageEx(NULL, fileHandle, &mappedImage)))
    {
        if (mappedImage.Signature == IMAGE_DOS_SIGNATURE)
        {
            PPH_STRING string;

            isImage = TRUE;

            if (NT_SUCCESS(PhGetMappedImageAuthenticodeHash(&mappedImage, Sha256HashAlgorithm, &string)))
            {
                AtJsonAddString(structured, "authenticode_sha256", string);
                PhDereferenceObject(string);
            }
            else
            {
                AtJsonAddNull(structured, "authenticode_sha256");
            }

            if (NT_SUCCESS(PhGetMappedImageWdacHash(&mappedImage, Sha256HashAlgorithm, &string)))
            {
                AtJsonAddString(structured, "wdac_sha256", string);
                PhDereferenceObject(string);
            }
            else
            {
                AtJsonAddNull(structured, "wdac_sha256");
            }

            // Not a hash of the file: a hash of what it imports.
            string = AtGetImageImphash(&mappedImage);
            AtJsonAddString(structured, "imphash", string);
            PhClearReference(&string);
        }

        PhUnloadMappedImage(&mappedImage);
    }

    if (!isImage)
    {
        AtJsonAddNull(structured, "authenticode_sha256");
        AtJsonAddNull(structured, "wdac_sha256");
        AtJsonAddNull(structured, "imphash");
    }

    PhAddJsonObjectBoolean(structured, "is_pe_image", isImage);

    Result->StructuredContent = structured;

    NtClose(fileHandle);
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
    PVOID sectionArray;
    ULONG sections;
    USHORT i;

    if (!(path = AtGetArgumentString(Call->Arguments, "path")) || path->Length == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"path is required.");
        PhClearReference(&path);
        return;
    }

    // Open the Win32 path read-only and let the mapper work on the handle; passing a Win32 path
    // straight to PhLoadMappedImageEx would be treated as an NT path.
    {
        PPH_STRING invalid;

        sections = AtpParseImageSections(
            AtJsonGetObjectMember(Call->Arguments, "sections", PH_JSON_OBJECT_TYPE_ARRAY),
            &invalid
            );

        if (invalid)
        {
            AtSetToolError(
                Result,
                "invalid_arguments",
                STATUS_INVALID_PARAMETER,
                L"%s is not a section of a PE image this tool knows about.",
                PhGetString(invalid)
                );
            PhDereferenceObject(invalid);
            PhDereferenceObject(path);
            return;
        }
    }

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

    sectionArray = PhCreateJsonArray();

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
        PhAddJsonArrayObject(sectionArray, row);
    }

    PhAddJsonObjectValue(structured, "sections", sectionArray);
    PhAddJsonObjectUInt64(structured, "section_count", mappedImage.NumberOfSections);

    // Everything past the summary, and only what was asked for.
    AtAddImageSections(structured, &mappedImage, path, sections, Call);

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
    case AtActionGetImageStrings:
        AtpGetImageStrings(Call, Result);
        break;
    case AtActionGetFileInfo:
        AtpGetFileInfo(Call, Result);
        break;
    case AtActionListDirectory:
        AtpListDirectory(Call, Result);
        break;
    case AtActionGetFileScanResultCached:
        AtpGetFileScanResultCached(Call, Result);
        break;
    case AtActionLookupFileHashVirusTotal:
        AtpLookupFileHashVirusTotal(Call, Result);
        break;
    case AtActionLookupFileHashHybridAnalysis:
        AtpLookupFileHashHybridAnalysis(Call, Result);
        break;
    case AtActionReadRegistryKey:
        AtpReadRegistryKey(Call, Result);
        break;
    case AtActionGetFileHashes:
        AtpGetFileHashes(Call, Result);
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
