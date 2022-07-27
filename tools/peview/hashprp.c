/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2022
 *
 */

#include <peview.h>
#include <bcrypt.h>
#include "..\thirdparty\tlsh\tlsh_wrapper.h"

typedef struct _PV_PE_HASH_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_PE_HASH_CONTEXT, *PPV_PE_HASH_CONTEXT;

typedef struct _PVP_HASH_CONTEXT
{
    BCRYPT_ALG_HANDLE HashAlgHandle;
    BCRYPT_KEY_HANDLE KeyHandle;
    BCRYPT_HASH_HANDLE HashHandle;
    ULONG HashObjectSize;
    ULONG HashSize;
    PVOID HashObject;
    PVOID Hash;
} PVP_HASH_CONTEXT, *PPVP_HASH_CONTEXT;

typedef enum _PV_HASHLIST_CATEGORY
{
    PV_HASHLIST_CATEGORY_FILEHASH,
    PV_HASHLIST_CATEGORY_IMPORTHASH,
    PV_HASHLIST_CATEGORY_FUZZYHASH,
    PV_HASHLIST_CATEGORY_MAXIMUM
} PV_HASHLIST_CATEGORY;

typedef enum _PV_HASHLIST_INDEX
{
    PV_HASHLIST_INDEX_CRC32,
    PV_HASHLIST_INDEX_MD5,
    PV_HASHLIST_INDEX_SHA1,
    PV_HASHLIST_INDEX_SHA256,
    PV_HASHLIST_INDEX_SHA348,
    PV_HASHLIST_INDEX_SHA512,
    PV_HASHLIST_INDEX_AUTHENTIHASH,
    PV_HASHLIST_INDEX_IMPHASH,
    PV_HASHLIST_INDEX_IMPHASHMSFT,
    PV_HASHLIST_INDEX_IMPFUZZY,
    PV_HASHLIST_INDEX_SSDEEP,
    PV_HASHLIST_INDEX_TLSH,
    PV_HASHLIST_INDEX_MAXIMUM
} PV_HASHLIST_INDEX;

NTSTATUS fuzzy_hash_file(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING* HashResult
    );

BOOLEAN fuzzy_hash_buffer(
    _In_ PBYTE Buffer,
    _In_ ULONGLONG BufferLength,
    _Out_ PPH_STRING* HashResult
    );

PPVP_HASH_CONTEXT PvpCreateHashHandle(
    _In_ PCWSTR AlgorithmId
    )
{
    ULONG querySize;
    PPVP_HASH_CONTEXT hashContext;

    hashContext = PhAllocate(sizeof(PVP_HASH_CONTEXT));
    memset(hashContext, 0, sizeof(PVP_HASH_CONTEXT));

    if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(
        &hashContext->HashAlgHandle,
        AlgorithmId,
        NULL,
        0
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(BCryptGetProperty(
        hashContext->HashAlgHandle,
        BCRYPT_OBJECT_LENGTH,
        (PUCHAR)&hashContext->HashObjectSize,
        sizeof(ULONG),
        &querySize,
        0
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(BCryptGetProperty(
        hashContext->HashAlgHandle,
        BCRYPT_HASH_LENGTH,
        (PUCHAR)&hashContext->HashSize,
        sizeof(ULONG),
        &querySize,
        0
        )))
    {
        goto CleanupExit;
    }

    if (!(hashContext->HashObject = PhAllocate(hashContext->HashObjectSize)))
    {
        goto CleanupExit;
    }

    if (!(hashContext->Hash = PhAllocate(hashContext->HashSize)))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(BCryptCreateHash(
        hashContext->HashAlgHandle,
        &hashContext->HashHandle,
        hashContext->HashObject,
        hashContext->HashObjectSize,
        NULL,
        0,
        0
        )))
    {
        goto CleanupExit;
    }

    return hashContext;

CleanupExit:

    if (hashContext->HashObject)
        PhFree(hashContext->HashObject);
    if (hashContext->Hash)
        PhFree(hashContext->Hash);
    if (hashContext)
        PhFree(hashContext);

    return NULL;
}

VOID PvpDestroyHashHandle(
    _In_ PPVP_HASH_CONTEXT Context
    )
{
    if (Context->HashAlgHandle)
        BCryptCloseAlgorithmProvider(Context->HashAlgHandle, 0);

    if (Context->HashHandle)
        BCryptDestroyHash(Context->HashHandle);

    if (Context->KeyHandle)
        BCryptDestroyKey(Context->KeyHandle);

    if (Context->HashObject)
        PhFree(Context->HashObject);

    if (Context->Hash)
        PhFree(Context->Hash);

    PhFree(Context);
}

PPH_STRING PvpGetFinalHash(
    _In_ PPVP_HASH_CONTEXT HashContext
    )
{
    if (NT_SUCCESS(BCryptFinishHash(
        HashContext->HashHandle,
        HashContext->Hash,
        HashContext->HashSize,
        0
        )))
    {
        return PhBufferToHexString(HashContext->Hash, HashContext->HashSize);
    }

    return NULL;
}

VOID PvpGetFileHashes(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING* CrcHashString,
    _Out_ PPH_STRING* Md5HashString,
    _Out_ PPH_STRING* Sha1HashString,
    _Out_ PPH_STRING* Sha2HashString,
    _Out_ PPH_STRING* Sha384HashString,
    _Out_ PPH_STRING* Sha512HashString
    )
{
    PPVP_HASH_CONTEXT hashContextMd5;
    PPVP_HASH_CONTEXT hashContextSha1;
    PPVP_HASH_CONTEXT hashContextSha256;
    PPVP_HASH_CONTEXT hashContextSha384;
    PPVP_HASH_CONTEXT hashContextSha512;
    ULONG crc32Hash = 0;
    ULONG returnLength;
    IO_STATUS_BLOCK isb;
    BYTE buffer[PAGE_SIZE];

    hashContextMd5 = PvpCreateHashHandle(BCRYPT_MD5_ALGORITHM);
    hashContextSha1 = PvpCreateHashHandle(BCRYPT_SHA1_ALGORITHM);
    hashContextSha256 = PvpCreateHashHandle(BCRYPT_SHA256_ALGORITHM);
    hashContextSha384 = PvpCreateHashHandle(BCRYPT_SHA384_ALGORITHM);
    hashContextSha512 = PvpCreateHashHandle(BCRYPT_SHA512_ALGORITHM);

    while (NT_SUCCESS(NtReadFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        buffer,
        PAGE_SIZE,
        NULL,
        NULL
        )))
    {
        returnLength = (ULONG)isb.Information;

        if (returnLength == 0)
            break;

        crc32Hash = PhCrc32(crc32Hash, buffer, returnLength);
        BCryptHashData(hashContextMd5->HashHandle, buffer, returnLength, 0);
        BCryptHashData(hashContextSha1->HashHandle, buffer, returnLength, 0);
        BCryptHashData(hashContextSha256->HashHandle, buffer, returnLength, 0);
        BCryptHashData(hashContextSha384->HashHandle, buffer, returnLength, 0);
        BCryptHashData(hashContextSha512->HashHandle, buffer, returnLength, 0);
    }

    crc32Hash = _byteswap_ulong(crc32Hash);
    *CrcHashString = PhBufferToHexString((PUCHAR)&crc32Hash, sizeof(ULONG));
    *Md5HashString = PvpGetFinalHash(hashContextMd5);
    *Sha1HashString = PvpGetFinalHash(hashContextSha1);
    *Sha2HashString = PvpGetFinalHash(hashContextSha256);
    *Sha384HashString = PvpGetFinalHash(hashContextSha384);
    *Sha512HashString = PvpGetFinalHash(hashContextSha512);

    PvpDestroyHashHandle(hashContextSha512);
    PvpDestroyHashHandle(hashContextSha256);
    PvpDestroyHashHandle(hashContextSha1);
    PvpDestroyHashHandle(hashContextMd5);
}

PPH_STRING PvpGetMappedImageImphashMsft(
    _In_ HANDLE FileHandle
    )
{
    PPH_STRING hashString = NULL;
    BYTE importTableMd5Hash[16];

    if (NT_SUCCESS(RtlComputeImportTableHash(
        FileHandle,
        importTableMd5Hash,
        RTL_IMPORT_TABLE_HASH_REVISION
        )))
    {
        hashString = PhBufferToHexString(importTableMd5Hash, 16);
    }

    return hashString;
}

PPH_STRING PvpGetMappedImageAuthentihash(
    VOID
    )
{
    PPH_STRING hashString = NULL;
    PIMAGE_DOS_HEADER imageDosHeader;
    ULONG64 offset = 0;
    ULONG imageChecksumOffset;
    ULONG imageSecurityOffset;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PPVP_HASH_CONTEXT hashContext;

    imageDosHeader = (PIMAGE_DOS_HEADER)PvMappedImage.ViewBase;

    if (imageDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return NULL;

    if (!NT_SUCCESS(PhGetMappedImageDataEntry(
        &PvMappedImage,
        IMAGE_DIRECTORY_ENTRY_SECURITY,
        &dataDirectory
        )))
    {
        return NULL;
    }

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        imageChecksumOffset = imageDosHeader->e_lfanew + UFIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader.CheckSum);
        imageSecurityOffset = imageDosHeader->e_lfanew + UFIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]);
    }
    else if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        imageChecksumOffset = imageDosHeader->e_lfanew + UFIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader.CheckSum);
        imageSecurityOffset = imageDosHeader->e_lfanew + UFIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]);
    }
    else
    {
        return NULL;
    }

    if (!(hashContext = PvpCreateHashHandle(BCRYPT_SHA256_ALGORITHM)))
        return NULL;

    while (offset < imageChecksumOffset)
    {
        BCryptHashData(hashContext->HashHandle, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), sizeof(BYTE), 0);
        offset++;
    }

    offset += RTL_FIELD_SIZE(IMAGE_OPTIONAL_HEADER, CheckSum);

    while (offset < imageSecurityOffset)
    {
        BCryptHashData(hashContext->HashHandle, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), sizeof(BYTE), 0);
        offset++;
    }

    offset += sizeof(IMAGE_DATA_DIRECTORY);

    while (offset < dataDirectory->VirtualAddress)
    {
        BCryptHashData(hashContext->HashHandle, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), sizeof(BYTE), 0);
        offset++;
    }

    offset += dataDirectory->Size;

    while (offset < PvMappedImage.Size)
    {
        BCryptHashData(hashContext->HashHandle, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), sizeof(BYTE), 0);
        offset++;
    }

    hashString = PvpGetFinalHash(hashContext);
    PvpDestroyHashHandle(hashContext);

    return hashString;
}

PPH_HASHTABLE PvGenerateOrdinalHashtable(
    _In_ PPH_STRING FileName
    )
{
    PPH_HASHTABLE ordinalHashtable = NULL;
    PH_MAPPED_IMAGE mappedImage;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
    PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;

    if (NT_SUCCESS(PhLoadMappedImageEx(&FileName->sr, NULL, &mappedImage)))
    {
        if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
        {
            ordinalHashtable = PhCreateSimpleHashtable(exports.ExportDirectory->NumberOfNames);

            for (ULONG i = 0; i < exports.NumberOfEntries; i++)
            {
                if (
                    NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)) &&
                    NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction))
                    )
                {
                    if (exportEntry.Name)
                    {
                        PhAddItemSimpleHashtable(
                            ordinalHashtable,
                            UlongToPtr(exportEntry.Ordinal),
                            PhZeroExtendToUtf16(exportEntry.Name)
                            );
                    }
                }
            }
        }

        PhUnloadMappedImage(&mappedImage);
    }

    return ordinalHashtable;
}

typedef struct _PV_IMPHASH_ORDINAL_CACHE
{
    PPH_HASHTABLE Oleaut32Hashtable;
    PPH_HASHTABLE Ws232Hashtable;
    PPH_HASHTABLE WinsockHashtable;
} PV_IMPHASH_ORDINAL_CACHE, *PPV_IMPHASH_ORDINAL_CACHE;

PPV_IMPHASH_ORDINAL_CACHE PvImpHashCreateOrdinalCache(
    VOID
    )
{
    static PH_STRINGREF filenameOleAut32Sr = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\oleaut32.dll");
    static PH_STRINGREF filenameWs232Sr = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ws2_32.dll");
    static PH_STRINGREF filenameWsock32Sr = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\wsock32.dll");
    PPV_IMPHASH_ORDINAL_CACHE ordinalCache;
    PPH_STRING fileName;

    ordinalCache = PhAllocate(sizeof(PV_IMPHASH_ORDINAL_CACHE));
    memset(ordinalCache, 0, sizeof(PV_IMPHASH_ORDINAL_CACHE));

    fileName = PhCreateString2(&filenameOleAut32Sr);
    ordinalCache->Oleaut32Hashtable = PvGenerateOrdinalHashtable(fileName);
    PhDereferenceObject(fileName);

    fileName = PhCreateString2(&filenameWs232Sr);
    ordinalCache->Ws232Hashtable = PvGenerateOrdinalHashtable(fileName);
    PhDereferenceObject(fileName);

    fileName = PhCreateString2(&filenameWsock32Sr);
    ordinalCache->WinsockHashtable = PvGenerateOrdinalHashtable(fileName);
    PhDereferenceObject(fileName);

    return ordinalCache;
}

VOID PvImpHashDestroyHashTable(
    _In_ PPH_HASHTABLE Hashtable
    )
{
    PPH_KEY_VALUE_PAIR entry;
    ULONG enumerationKey;

    enumerationKey = 0;

    while (PhEnumHashtable(Hashtable, &entry, &enumerationKey))
    {
        if ((*entry).Value)
        {
            PhDereferenceObject((*entry).Value);
        }
    }

    PhDereferenceObject(Hashtable);
}

VOID PvImpHashDestroyOrdinalCache(
    _In_ PPV_IMPHASH_ORDINAL_CACHE OrdinalCache
    )
{
    if (OrdinalCache->Oleaut32Hashtable)
        PvImpHashDestroyHashTable(OrdinalCache->Oleaut32Hashtable);
    if (OrdinalCache->Ws232Hashtable)
        PvImpHashDestroyHashTable(OrdinalCache->Ws232Hashtable);
    if (OrdinalCache->WinsockHashtable)
        PvImpHashDestroyHashTable(OrdinalCache->WinsockHashtable);
}

PPH_STRING PvImpHashQueryOrdinalName(
    _In_ PPV_IMPHASH_ORDINAL_CACHE OrdinalCache,
    _In_ PPH_STRING FileName,
    _In_ USHORT Ordinal
    )
{
    PPH_STRING exportName;

    // ImpHash will only lookup the ordinal names for imports from these 3 DLLs.
    // Online implementations and other tools hard-code every string... (dmex)

    if (PhStartsWithString2(FileName, L"oleaut32.dll", TRUE) && OrdinalCache->Oleaut32Hashtable)
    {
        if (exportName = PhFindItemSimpleHashtable2(OrdinalCache->Oleaut32Hashtable, UlongToPtr(Ordinal)))
            return PhReferenceObject(exportName);
    }

    if (PhStartsWithString2(FileName, L"ws2_32.dll", TRUE) && OrdinalCache->Ws232Hashtable)
    {
        if (exportName = PhFindItemSimpleHashtable2(OrdinalCache->Ws232Hashtable, UlongToPtr(Ordinal)))
            return PhReferenceObject(exportName);
    }

    if (PhStartsWithString2(FileName, L"wsock32.dll", TRUE) && OrdinalCache->WinsockHashtable)
    {
        if (exportName = PhFindItemSimpleHashtable2(OrdinalCache->WinsockHashtable, UlongToPtr(Ordinal)))
            return PhReferenceObject(exportName);
    }

    //PH_MAPPED_IMAGE mappedImage;
    //PPH_STRING filePath;
    //PPH_STRING exportName = NULL;
    //
    //if (!(filePath = PhSearchFilePath(PhGetString(FileName), L".dll")))
    //{
    //    PhSetReference(&filePath, FileName);
    //}
    //
    //if (NT_SUCCESS(PhLoadMappedImage(PhGetString(filePath), NULL, &mappedImage)))
    //{
    //    PH_MAPPED_IMAGE_EXPORTS exports;
    //    PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
    //    PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;
    //    ULONG i;
    //
    //    if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
    //    {
    //        for (i = 0; i < exports.NumberOfEntries; i++)
    //        {
    //            if (
    //                NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)) &&
    //                NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction))
    //                )
    //            {
    //                if (exportEntry.Ordinal == Ordinal && exportEntry.Name)
    //                {
    //                    exportName = PhZeroExtendToUtf16(exportEntry.Name);
    //                    break;
    //                }
    //            }
    //        }
    //    }
    //
    //    PhUnloadMappedImage(&mappedImage);
    //}
    //
    //PhDereferenceObject(filePath);

    return NULL;
}

BOOLEAN PvpGetMappedImageImphash(
    _Out_opt_ PPH_STRING *ImpHash,
    _Out_opt_ PPH_STRING *ImpHashFuzzy
    )
{
    static PH_STRINGREF separator = PH_STRINGREF_INIT(L".");
    PPH_STRING hashString = NULL;
    PPH_STRING hashFuzzyString = NULL;
    PH_STRING_BUILDER stringBuilder;
    PH_MAPPED_IMAGE_IMPORTS imports;
    PPV_IMPHASH_ORDINAL_CACHE ordinals;

    PhInitializeStringBuilder(&stringBuilder, 0x100);
    ordinals = PvImpHashCreateOrdinalCache();

    if (NT_SUCCESS(PhGetMappedImageImports(&imports, &PvMappedImage)))
    {
        PH_MAPPED_IMAGE_IMPORT_DLL importDll;
        PH_MAPPED_IMAGE_IMPORT_ENTRY importEntry;

        for (ULONG i = 0; i < imports.NumberOfDlls; i++)
        {
            if (NT_SUCCESS(PhGetMappedImageImportDll(&imports, i, &importDll)))
            {
                for (ULONG ii = 0; ii < importDll.NumberOfEntries; ii++)
                {
                    if (NT_SUCCESS(PhGetMappedImageImportEntry(&importDll, ii, &importEntry)))
                    {
                        ULONG_PTR indexOfExtension = SIZE_MAX;
                        PPH_STRING importDllString;
                        PPH_STRING importDllName;
                        PPH_STRING importFuncName;
                        PPH_STRING importImphashName;

                        importDllString = PhZeroExtendToUtf16(importDll.Name);

                        // I dont know why but online implementations only trim the import extensions for dll, sys and ocx
                        // ignoring everything else which is dumb because lots of things import from exe and other formats
                        // like our plugin dlls. Sigh. Do the same so the imphash matches theirs. (dmex)
                        if (
                            PhEndsWithString2(importDllString, L".dll", TRUE) ||
                            PhEndsWithString2(importDllString, L".sys", TRUE) ||
                            PhEndsWithString2(importDllString, L".ocx", TRUE)
                            )
                        {
                            indexOfExtension = PhFindLastCharInString(importDllString, 0, L'.');
                        }

                        if (indexOfExtension != SIZE_MAX)
                            importDllName = PhSubstring(importDllString, 0, indexOfExtension);
                        else
                            importDllName = PhReferenceObject(importDllString);

                        if (importEntry.Name)
                            importFuncName = PhZeroExtendToUtf16(importEntry.Name);
                        else
                        {
                            PPH_STRING ordinalImportFuncName;

                            if (ordinalImportFuncName = PvImpHashQueryOrdinalName(ordinals, importDllString, importEntry.Ordinal))
                                importFuncName = ordinalImportFuncName;
                            else
                                importFuncName = PhFormatString(L"ord%u", importEntry.Ordinal);
                        }

                        importImphashName = PhConcatStringRef3(
                            &importDllName->sr,
                            &separator,
                            &importFuncName->sr);
                        _wcslwr(importImphashName->Buffer);

                        PhAppendFormatStringBuilder(
                            &stringBuilder,
                            L"%s,",
                            importImphashName->Buffer
                            );

                        PhDereferenceObject(importDllString);
                        PhDereferenceObject(importDllName);
                        PhDereferenceObject(importFuncName);
                        PhDereferenceObject(importImphashName);
                    }
                }
            }
        }
    }

    // Ignore the delay imports just like everyone else...

    if (PhEndsWithString2(stringBuilder.String, L",", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    if (!PhIsNullOrEmptyString(stringBuilder.String))
    {
        PPH_STRING importStringFinal;
        PPH_STRING importStringFuzzy;
        PPH_BYTES importStringUtf8;
        PH_HASH_CONTEXT hashContext;
        UCHAR hash[32];

        importStringFinal = PhFinalStringBuilderString(&stringBuilder);
        importStringUtf8 = PhConvertUtf16ToUtf8Ex(importStringFinal->Buffer, importStringFinal->Length);

        PhInitializeHash(&hashContext, Md5HashAlgorithm);
        PhUpdateHash(&hashContext, importStringUtf8->Buffer, (ULONG)importStringUtf8->Length);

        if (PhFinalHash(&hashContext, hash, 16, NULL))
        {
            hashString = PhBufferToHexString(hash, 16);
        }

        // Generate the "impfuzzy" hash:
        // https://blogs.jpcert.or.jp/ja/2016/05/impfuzzy.html
        if (fuzzy_hash_buffer((PBYTE)importStringUtf8->Buffer, importStringUtf8->Length, &importStringFuzzy))
        {
            hashFuzzyString = importStringFuzzy;
        }

        PhDereferenceObject(importStringUtf8);
    }

    PhDeleteStringBuilder(&stringBuilder);

    if (ImpHash)
        *ImpHash = hashString;
    else
        PhDereferenceObject(hashString);

    if (ImpHashFuzzy)
        *ImpHashFuzzy = hashFuzzyString;
    else
        PhDereferenceObject(hashFuzzyString);

    PvImpHashDestroyOrdinalCache(ordinals);

    return TRUE;
}

VOID PvpPeEnumFileHashes(
    _In_ HWND ListViewHandle
    )
{
    ULONG count = 0;
    HANDLE fileHandle;
    INT lvItemIndex;
    PPH_STRING crc32HashString = NULL;
    PPH_STRING md5HashString = NULL;
    PPH_STRING sha1HashString = NULL;
    PPH_STRING sha2HashString = NULL;
    PPH_STRING sha384HashString = NULL;
    PPH_STRING sha512HashString = NULL;
    PPH_STRING authentihashString = NULL;
    PPH_STRING imphashString = NULL;
    PPH_STRING imphashFuzzyString = NULL;
    PPH_STRING impMsftHashString = NULL;
    PPH_STRING ssdeepHashString = NULL;
    PPH_STRING tlshHashString = NULL;
    WCHAR number[PH_PTR_STR_LEN_1];

    ListView_EnableGroupView(ListViewHandle, TRUE);
    PhAddListViewGroup(ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, L"File hashes");
    PhAddListViewGroup(ListViewHandle, PV_HASHLIST_CATEGORY_IMPORTHASH, L"Import hashes");
    PhAddListViewGroup(ListViewHandle, PV_HASHLIST_CATEGORY_FUZZYHASH, L"Fuzzy hashes");

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY
        )))
    {
        impMsftHashString = PvpGetMappedImageImphashMsft(fileHandle);

        PvpGetFileHashes(
            fileHandle,
            &crc32HashString,
            &md5HashString,
            &sha1HashString,
            &sha2HashString,
            &sha384HashString,
            &sha512HashString
            );

        PhSetFilePosition(fileHandle, NULL);
        fuzzy_hash_file(fileHandle, &ssdeepHashString);

        PhSetFilePosition(fileHandle, NULL);
        PvGetTlshFileHash(fileHandle, &tlshHashString);

        NtClose(fileHandle);
    }

    if (PhIsNullOrEmptyString(ssdeepHashString))
    {
        fuzzy_hash_buffer(PvMappedImage.ViewBase, PvMappedImage.Size, &ssdeepHashString);
    }

    if (PhIsNullOrEmptyString(tlshHashString))
    {
        PvGetTlshBufferHash(PvMappedImage.ViewBase, PvMappedImage.Size, &tlshHashString);
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_CRC32, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"CRC32");

    if (!PhIsNullOrEmptyString(crc32HashString))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(crc32HashString));
        PhDereferenceObject(crc32HashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_MD5, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"MD5");

    if (!PhIsNullOrEmptyString(md5HashString))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(md5HashString));
        PhDereferenceObject(md5HashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_SHA1, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"SHA-1");

    if (!PhIsNullOrEmptyString(sha1HashString))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(sha1HashString));
        PhDereferenceObject(sha1HashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_SHA256, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"SHA-256");

    if (!PhIsNullOrEmptyString(sha2HashString))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(sha2HashString));
        PhDereferenceObject(sha2HashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_SHA348, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"SHA-384");

    if (!PhIsNullOrEmptyString(sha384HashString))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(sha384HashString));
        PhDereferenceObject(sha384HashString);
    }
    else
    {

        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_SHA512, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"SHA-512");

    if (!PhIsNullOrEmptyString(sha512HashString))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(sha512HashString));
        PhDereferenceObject(sha512HashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_AUTHENTIHASH, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"Authentihash");

    if (authentihashString = PvpGetMappedImageAuthentihash())
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(authentihashString));
        PhDereferenceObject(authentihashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_IMPORTHASH, PV_HASHLIST_INDEX_IMPHASH, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"Imphash");

    if (PvpGetMappedImageImphash(&imphashString, &imphashFuzzyString))
    {    
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetStringOrDefault(imphashString, L"ERROR"));
        PhClearReference(&imphashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_IMPORTHASH, PV_HASHLIST_INDEX_IMPHASHMSFT, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"Imphash (msft)");

    if (!PhIsNullOrEmptyString(impMsftHashString))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(impMsftHashString));
        PhDereferenceObject(impMsftHashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_FUZZYHASH, PV_HASHLIST_INDEX_IMPFUZZY, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"Impfuzzy");

    if (!PhIsNullOrEmptyString(imphashFuzzyString)) // HACK
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(imphashFuzzyString));
        PhClearReference(&imphashFuzzyString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_FUZZYHASH, PV_HASHLIST_INDEX_SSDEEP, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"SSDEEP");

    if (!PhIsNullOrEmptyString(ssdeepHashString))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(ssdeepHashString));
        PhDereferenceObject(ssdeepHashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_FUZZYHASH, PV_HASHLIST_INDEX_TLSH, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"TLSH");

    if (!PhIsNullOrEmptyString(tlshHashString))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(tlshHashString));
        PhDereferenceObject(tlshHashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }
}

INT_PTR CALLBACK PvpPeHashesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PE_HASH_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_PE_HASH_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HIMAGELIST listViewImageList;

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Hash");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageHashesListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            if (listViewImageList = PhImageListCreate(2, 20, ILC_MASK | ILC_COLOR, 1, 1))
                ListView_SetImageList(context->ListViewHandle, listViewImageList, LVSIL_SMALL);

            PvpPeEnumFileHashes(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageHashesListViewColumns", context->ListViewHandle);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
