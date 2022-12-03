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
#include "..\thirdparty\ssdeep\fuzzy.h"

#include <wincrypt.h>
#include <wintrust.h>

#define WM_PV_HASH_FINISHED (WM_APP + 701)

typedef struct _PV_PE_HASHWND_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_PE_HASHWND_CONTEXT, *PPV_PE_HASHWND_CONTEXT;

typedef struct _PV_HASH_CONTEXT
{
    BCRYPT_ALG_HANDLE HashAlgHandle;
    BCRYPT_KEY_HANDLE KeyHandle;
    BCRYPT_HASH_HANDLE HashHandle;
    ULONG HashObjectSize;
    ULONG HashSize;
    PVOID HashObject;
    PVOID Hash;
} PV_HASH_CONTEXT, *PPV_HASH_CONTEXT;

typedef struct _PV_PE_HASH_RESULTS
{
    BOOLEAN ImageHasImports;

    PPH_STRING Crc32HashString;
    PPH_STRING Md5HashString;
    PPH_STRING Sha1HashString;
    PPH_STRING Sha2HashString;
    PPH_STRING Sha384HashString;
    PPH_STRING Sha512HashString;
    PPH_STRING AuthentihashSha1String;
    PPH_STRING AuthentihashSha256String;
    PPH_STRING WdaghashSha1String;
    PPH_STRING WdaghashSha256String;
    PPH_STRING ImphashString;
    PPH_STRING ImphashFuzzyString;
    PPH_STRING ImpMsftHashString;
    PPH_STRING SsdeepHashString;
    PPH_STRING TlshHashString;

    PPH_LIST PageHashList;
} PV_PE_HASH_RESULTS, *PPV_PE_HASH_RESULTS;

#include <pshpack1.h>
typedef struct _SPC_PE_IMAGE_PAGE_HASHES_V1
{
    ULONG PageOffset;
    BYTE PageHash[20]; // SHA-1
} SPC_PE_IMAGE_PAGE_HASHES_V1, *PSPC_PE_IMAGE_PAGE_HASHES_V1;

typedef struct _SPC_PE_IMAGE_PAGE_HASHES_V2
{
    ULONG PageOffset;
    BYTE PageHash[32]; // SHA-256
} SPC_PE_IMAGE_PAGE_HASHES_V2, *PSPC_PE_IMAGE_PAGE_HASHES_V2;
#include <poppack.h>

typedef enum _PV_HASHLIST_CATEGORY
{
    PV_HASHLIST_CATEGORY_FILEHASH,
    PV_HASHLIST_CATEGORY_IMPORTHASH,
    PV_HASHLIST_CATEGORY_FUZZYHASH,
    PV_HASHLIST_CATEGORY_AUTHENTIHASH,
    PV_HASHLIST_CATEGORY_WDACPAGEHASH,
    PV_HASHLIST_CATEGORY_PAGEHASH,
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
    PV_HASHLIST_INDEX_IMPHASH,
    PV_HASHLIST_INDEX_IMPHASHMSFT,
    PV_HASHLIST_INDEX_IMPFUZZY,
    PV_HASHLIST_INDEX_SSDEEP,
    PV_HASHLIST_INDEX_TLSH,
    PV_HASHLIST_INDEX_AUTHENTIHASH_SHA1,
    PV_HASHLIST_INDEX_AUTHENTIHASH_SHA256,
    PV_HASHLIST_INDEX_WDACPAGEHASH_SHA1,
    PV_HASHLIST_INDEX_WDACPAGEHASH_SHA256,
    PV_HASHLIST_INDEX_MAXIMUM
} PV_HASHLIST_INDEX;

PPV_HASH_CONTEXT PvCreateHashHandle(
    _In_ PCWSTR AlgorithmId
    )
{
    ULONG querySize;
    PPV_HASH_CONTEXT hashContext;

    hashContext = PhAllocate(sizeof(PV_HASH_CONTEXT));
    memset(hashContext, 0, sizeof(PV_HASH_CONTEXT));

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

VOID PvDestroyHashHandle(
    _In_ PPV_HASH_CONTEXT Context
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

PPH_STRING PvGetFinalHash(
    _In_ PPV_HASH_CONTEXT HashContext
    )
{
    if (NT_SUCCESS(BCryptFinishHash(
        HashContext->HashHandle,
        HashContext->Hash,
        HashContext->HashSize,
        0
        )))
    {
        PPH_STRING string;

        string = PhBufferToHexString(HashContext->Hash, HashContext->HashSize);
        _wcsupr(string->Buffer);

        return string;
    }

    return NULL;
}

NTSTATUS PvHashMappedImageData(
    _In_ PPV_HASH_CONTEXT HashContext,
    _In_ PVOID Buffer,
    _In_ ULONG64 BufferLength
    )
{
    NTSTATUS status;

    if (BufferLength >= ULONG_MAX)
    {
        PBYTE address;
        ULONG64 numberOfBytes;
        ULONG blockSize;

        // Chunk the data into smaller blocks when the buffer length
        // overflows the maximum length of the BCryptHashData function.

        address = (PBYTE)Buffer;
        numberOfBytes = BufferLength;
        blockSize = PAGE_SIZE * 64;

        while (numberOfBytes != 0)
        {
            if (blockSize > numberOfBytes)
                blockSize = (ULONG)numberOfBytes;

            status = BCryptHashData(HashContext->HashHandle, address, blockSize, 0);

            if (!NT_SUCCESS(status))
                break;

            address += blockSize;
            numberOfBytes -= blockSize;
        }
    }
    else
    {
        status = BCryptHashData(HashContext->HashHandle, Buffer, (ULONG)BufferLength, 0);
    }

    return status;
}

// rev from "signtool verify /ph /v C:\\Windows\\explorer.exe" (dmex)
PPH_LIST PvEnumSpcAuthenticodePageHashes(
    VOID
    )
{
    PPH_LIST pageHashList = NULL;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    HCRYPTMSG cryptMessageHandle = NULL;
    PVOID cryptInnerContentBuffer = NULL;
    ULONG cryptInnerContentLength = 0;
    PSPC_INDIRECT_DATA_CONTENT spcIndirectDataContentBuffer = NULL;
    ULONG spcIndirectDataContentLength = 0;
    PSPC_PE_IMAGE_DATA spcPeImageDataBuffer = NULL;
    ULONG spcPeImageDataLength = 0;

    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_SECURITY, &dataDirectory)))
    {
        LPWIN_CERTIFICATE certificateDirectory = PTR_ADD_OFFSET(PvMappedImage.ViewBase, dataDirectory->VirtualAddress);
        CERT_BLOB certificateBlob = { certificateDirectory->dwLength, certificateDirectory->bCertificate };

        if (certificateDirectory->wCertificateType == WIN_CERT_TYPE_PKCS_SIGNED_DATA)
        {
            CryptQueryObject(
                CERT_QUERY_OBJECT_BLOB,
                &certificateBlob,
                CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                CERT_QUERY_FORMAT_FLAG_BINARY,
                0,
                NULL,
                NULL,
                NULL,
                NULL,
                &cryptMessageHandle,
                NULL
                );
        }
    }

    if (!cryptMessageHandle)
    {
        CryptQueryObject(
            CERT_QUERY_OBJECT_FILE,
            PhGetString(PvFileName),
            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
            CERT_QUERY_FORMAT_FLAG_BINARY,
            0,
            NULL,
            NULL,
            NULL,
            NULL,
            &cryptMessageHandle,
            NULL
            );
    }

    if (!cryptMessageHandle)
        goto CleanupExit;

    if (!CryptMsgGetParam(
        cryptMessageHandle,
        CMSG_CONTENT_PARAM,
        0,
        NULL,
        &cryptInnerContentLength
        ))
    {
        goto CleanupExit;
    }

    cryptInnerContentBuffer = PhAllocateZero(cryptInnerContentLength);

    if (!CryptMsgGetParam(
        cryptMessageHandle,
        CMSG_CONTENT_PARAM,
        0,
        cryptInnerContentBuffer,
        &cryptInnerContentLength
        ))
    {
        goto CleanupExit;
    }

    if (!CryptDecodeObjectEx(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        SPC_INDIRECT_DATA_CONTENT_STRUCT,
        cryptInnerContentBuffer,
        cryptInnerContentLength,
        CRYPT_DECODE_ALLOC_FLAG,
        NULL,
        &spcIndirectDataContentBuffer,
        &spcIndirectDataContentLength
        ))
    {
        goto CleanupExit;
    }

    if (!PhEqualBytesZ(spcIndirectDataContentBuffer->Data.pszObjId, SPC_PE_IMAGE_DATA_OBJID, FALSE))
        goto CleanupExit;

    if (!CryptDecodeObjectEx(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        SPC_PE_IMAGE_DATA_STRUCT,
        spcIndirectDataContentBuffer->Data.Value.pbData,
        spcIndirectDataContentBuffer->Data.Value.cbData,
        CRYPT_DECODE_ALLOC_FLAG,
        NULL,
        &spcPeImageDataBuffer,
        &spcPeImageDataLength
        ))
    {
        goto CleanupExit;
    }

    if (
        spcPeImageDataBuffer->pFile->dwLinkChoice == SPC_MONIKER_LINK_CHOICE &&
        RtlEqualMemory(spcPeImageDataBuffer->pFile->Moniker.ClassId, (SPC_UUID)SpcSerializedObjectAttributesClassId, sizeof((SPC_UUID)SpcSerializedObjectAttributesClassId))
        )
    {
        ULONG spcSerializedObjectAttributesLength = 0;
        PCRYPT_ATTRIBUTES spcSerializedObjectAttributesBuffer = NULL;

        pageHashList = PhCreateList(100);

        if (CryptDecodeObjectEx(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            PKCS_ATTRIBUTES,
            spcPeImageDataBuffer->pFile->Moniker.SerializedData.pbData,
            spcPeImageDataBuffer->pFile->Moniker.SerializedData.cbData,
            CRYPT_DECODE_ALLOC_FLAG,
            NULL,
            &spcSerializedObjectAttributesBuffer,
            &spcSerializedObjectAttributesLength
            ))
        {
            for (ULONG i = 0; i < spcSerializedObjectAttributesBuffer->cAttr; i++)
            {
                CRYPT_ATTRIBUTE spcSerializedObjectBuffer = spcSerializedObjectAttributesBuffer->rgAttr[i];
                PCRYPT_DATA_BLOB spcImagePageHashesBuffer = NULL;
                ULONG spcImagePageHashesLength = 0;

                // for (ULONG j = 0; j < spcSerializedObjectBuffer.cValue; j++)

                if (!CryptDecodeObjectEx(
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    X509_OCTET_STRING,
                    spcSerializedObjectBuffer.rgValue->pbData,
                    spcSerializedObjectBuffer.rgValue->cbData,
                    CRYPT_DECODE_ALLOC_FLAG,
                    NULL,
                    &spcImagePageHashesBuffer,
                    &spcImagePageHashesLength
                    ))
                {
                    continue;
                }

                if (PhEqualBytesZ(spcSerializedObjectBuffer.pszObjId, SPC_PE_IMAGE_PAGE_HASHES_V1_OBJID, FALSE))
                {
                    ULONG count = spcImagePageHashesBuffer->cbData / sizeof(SPC_PE_IMAGE_PAGE_HASHES_V1);

                    for (ULONG k = 0; k < count; k++)
                    {
                        PSPC_PE_IMAGE_PAGE_HASHES_V1 entry = PTR_ADD_OFFSET(spcImagePageHashesBuffer->pbData, sizeof(SPC_PE_IMAGE_PAGE_HASHES_V1) * k);
                        PSPC_PE_IMAGE_PAGE_HASHES_V2 thunk;

                        thunk = PhAllocateZero(sizeof(SPC_PE_IMAGE_PAGE_HASHES_V2));
                        thunk->PageOffset = entry->PageOffset;
                        memcpy_s(thunk->PageHash, sizeof(thunk->PageHash), entry->PageHash, sizeof(entry->PageHash));
                        PhAddItemList(pageHashList, thunk);
                    }
                }
                else if (PhEqualBytesZ(spcSerializedObjectBuffer.pszObjId, SPC_PE_IMAGE_PAGE_HASHES_V2_OBJID, FALSE))
                {
                    ULONG count = spcImagePageHashesBuffer->cbData / sizeof(SPC_PE_IMAGE_PAGE_HASHES_V2);

                    for (ULONG k = 0; k < count; k++)
                    {
                        PSPC_PE_IMAGE_PAGE_HASHES_V2 entry = PTR_ADD_OFFSET(spcImagePageHashesBuffer->pbData, sizeof(SPC_PE_IMAGE_PAGE_HASHES_V2) * k);

                        PhAddItemList(pageHashList, PhAllocateCopy(entry, sizeof(SPC_PE_IMAGE_PAGE_HASHES_V2)));
                    }
                }

                LocalFree(spcImagePageHashesBuffer);
            }

            LocalFree(spcSerializedObjectAttributesBuffer);
        }
    }

CleanupExit:
    if (spcPeImageDataBuffer)
    {
        LocalFree(spcPeImageDataBuffer);
    }

    if (spcIndirectDataContentBuffer)
    {
        LocalFree(spcIndirectDataContentBuffer);
    }

    if (cryptInnerContentBuffer)
    {
        PhFree(cryptInnerContentBuffer);
    }

    if (cryptMessageHandle)
    {
        CryptMsgClose(cryptMessageHandle);
    }

    return pageHashList;
}

VOID PvGetFileHashes(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING* CrcHashString,
    _Out_ PPH_STRING* Md5HashString,
    _Out_ PPH_STRING* Sha1HashString,
    _Out_ PPH_STRING* Sha2HashString,
    _Out_ PPH_STRING* Sha384HashString,
    _Out_ PPH_STRING* Sha512HashString
    )
{
    PPV_HASH_CONTEXT hashContextMd5;
    PPV_HASH_CONTEXT hashContextSha1;
    PPV_HASH_CONTEXT hashContextSha256;
    PPV_HASH_CONTEXT hashContextSha384;
    PPV_HASH_CONTEXT hashContextSha512;
    ULONG crc32Hash = 0;
    ULONG returnLength;
    IO_STATUS_BLOCK isb;
    PBYTE buffer;

    buffer = PhAllocate(PAGE_SIZE * 2);
    hashContextMd5 = PvCreateHashHandle(BCRYPT_MD5_ALGORITHM);
    hashContextSha1 = PvCreateHashHandle(BCRYPT_SHA1_ALGORITHM);
    hashContextSha256 = PvCreateHashHandle(BCRYPT_SHA256_ALGORITHM);
    hashContextSha384 = PvCreateHashHandle(BCRYPT_SHA384_ALGORITHM);
    hashContextSha512 = PvCreateHashHandle(BCRYPT_SHA512_ALGORITHM);

    while (NT_SUCCESS(NtReadFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        buffer,
        PAGE_SIZE * 2,
        NULL,
        NULL
        )))
    {
        returnLength = (ULONG)isb.Information;

        if (returnLength == 0)
            break;

        crc32Hash = PhCrc32(crc32Hash, buffer, returnLength);
        PvHashMappedImageData(hashContextMd5, buffer, returnLength);
        PvHashMappedImageData(hashContextSha1, buffer, returnLength);
        PvHashMappedImageData(hashContextSha256, buffer, returnLength);
        PvHashMappedImageData(hashContextSha384, buffer, returnLength);
        PvHashMappedImageData(hashContextSha512, buffer, returnLength);
    }

    crc32Hash = _byteswap_ulong(crc32Hash);
    *CrcHashString = PhBufferToHexString((PUCHAR)&crc32Hash, sizeof(ULONG));
    *Md5HashString = PvGetFinalHash(hashContextMd5);
    *Sha1HashString = PvGetFinalHash(hashContextSha1);
    *Sha2HashString = PvGetFinalHash(hashContextSha256);
    *Sha384HashString = PvGetFinalHash(hashContextSha384);
    *Sha512HashString = PvGetFinalHash(hashContextSha512);

    PvDestroyHashHandle(hashContextSha512);
    PvDestroyHashHandle(hashContextSha256);
    PvDestroyHashHandle(hashContextSha1);
    PvDestroyHashHandle(hashContextMd5);
    PhFree(buffer);
}

NTSTATUS PvGetMappedImageMicrosoftImpHash(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING* HashResult
    )
{
    NTSTATUS status;
    BYTE importTableMd5Hash[16];

    status = RtlComputeImportTableHash(
        FileHandle,
        importTableMd5Hash,
        RTL_IMPORT_TABLE_HASH_REVISION
        );

    if (NT_SUCCESS(status))
    {
        PPH_STRING string;

        string = PhBufferToHexString(importTableMd5Hash, sizeof(importTableMd5Hash));
        _wcsupr(string->Buffer);

        *HashResult = string;
        return STATUS_SUCCESS;
    }

    return status;
}

PPH_STRING PvGetMappedImageAuthentihash(
    _In_ PCWSTR AlgorithmId
    )
{
    PIMAGE_DOS_HEADER imageDosHeader = (PIMAGE_DOS_HEADER)PvMappedImage.ViewBase;
    PPH_STRING hashString = NULL;
    ULONG imageChecksumOffset;
    ULONG imageSecurityOffset;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PPV_HASH_CONTEXT hashContext;

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

    if (!(hashContext = PvCreateHashHandle(AlgorithmId)))
        return NULL;

    {
        ULONG64 offset = 0;
        ULONG64 length = imageChecksumOffset - offset;

        if (!NT_SUCCESS(PvHashMappedImageData(hashContext, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), length)))
            return NULL;
    }

    {
        ULONG64 offset = imageChecksumOffset + RTL_FIELD_SIZE(IMAGE_OPTIONAL_HEADER, CheckSum);
        ULONG64 length = imageSecurityOffset - offset;

        if (!NT_SUCCESS(PvHashMappedImageData(hashContext, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), length)))
            return NULL;
    }

    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_SECURITY, &dataDirectory)))
    {
        {
            ULONG64 offset = imageSecurityOffset + sizeof(IMAGE_DATA_DIRECTORY);
            ULONG64 length = dataDirectory->VirtualAddress - offset;

            if (!NT_SUCCESS(PvHashMappedImageData(hashContext, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), length)))
                return NULL;
        }

        {
            ULONG64 offset = dataDirectory->VirtualAddress + dataDirectory->Size;
            ULONG64 length = PvMappedImage.Size - offset;

            if (!NT_SUCCESS(PvHashMappedImageData(hashContext, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), length)))
                return NULL;
        }
    }
    else
    {
        ULONG64 offset = imageSecurityOffset + sizeof(IMAGE_DATA_DIRECTORY);
        ULONG64 length = PvMappedImage.Size - offset;

        if (!NT_SUCCESS(PvHashMappedImageData(hashContext, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), length)))
            return NULL;
    }

    hashString = PvGetFinalHash(hashContext);
    PvDestroyHashHandle(hashContext);

    return hashString;
}

PPH_STRING PvGetMappedImageAuthentihashLegacy(
    _In_ PCWSTR AlgorithmId
    )
{
    PIMAGE_DOS_HEADER imageDosHeader = (PIMAGE_DOS_HEADER)PvMappedImage.ViewBase;
    PPH_STRING hashString = NULL;
    ULONG64 offset = 0;
    ULONG imageChecksumOffset;
    ULONG imageSecurityOffset;
    ULONG directoryAddress = 0;
    ULONG directorySize= 0;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PPV_HASH_CONTEXT hashContext;

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

    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_SECURITY, &dataDirectory)))
    {
        directoryAddress = dataDirectory->VirtualAddress;
        directorySize = dataDirectory->Size;
    }

    if (!(hashContext = PvCreateHashHandle(AlgorithmId)))
        return NULL;

    while (offset < imageChecksumOffset)
    {
        PvHashMappedImageData(hashContext, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), sizeof(BYTE));
        offset++;
    }

    offset += RTL_FIELD_SIZE(IMAGE_OPTIONAL_HEADER, CheckSum);

    while (offset < imageSecurityOffset)
    {
        PvHashMappedImageData(hashContext, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), sizeof(BYTE));
        offset++;
    }

    offset += sizeof(IMAGE_DATA_DIRECTORY);

    while (offset < directoryAddress)
    {
        PvHashMappedImageData(hashContext, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), sizeof(BYTE));
        offset++;
    }

    offset += directorySize;

    while (offset < PvMappedImage.Size)
    {
        PvHashMappedImageData(hashContext, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), sizeof(BYTE));
        offset++;
    }

    hashString = PvGetFinalHash(hashContext);
    PvDestroyHashHandle(hashContext);

    return hashString;
}

PPH_STRING PvGetMappedImageWdacPageHash(
    _In_ PCWSTR AlgorithmId
    )
{
    PIMAGE_DOS_HEADER imageDosHeader = (PIMAGE_DOS_HEADER)PvMappedImage.ViewBase;
    PPH_STRING hashString = NULL;
    ULONG offset = 0;
    ULONG imageChecksumOffset;
    ULONG imageSecurityOffset;
    ULONG imageSizeOfHeaders;
    PPV_HASH_CONTEXT hashContext;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        imageChecksumOffset = imageDosHeader->e_lfanew + UFIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader.CheckSum);
        imageSecurityOffset = imageDosHeader->e_lfanew + UFIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]);
        imageSizeOfHeaders = ((PIMAGE_OPTIONAL_HEADER32)&PvMappedImage.NtHeaders32->OptionalHeader)->SizeOfHeaders;
    }
    else if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        imageChecksumOffset = imageDosHeader->e_lfanew + UFIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader.CheckSum);
        imageSecurityOffset = imageDosHeader->e_lfanew + UFIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]);
        imageSizeOfHeaders = ((PIMAGE_OPTIONAL_HEADER64)&PvMappedImage.NtHeaders->OptionalHeader)->SizeOfHeaders;
    }
    else
    {
        return NULL;
    }

    if (!(hashContext = PvCreateHashHandle(AlgorithmId)))
        return NULL;

    while (offset < PAGE_SIZE)
    {
        if (offset == imageChecksumOffset)
            offset += RTL_FIELD_SIZE(IMAGE_OPTIONAL_HEADER, CheckSum);
        if (offset == imageSecurityOffset)
            offset += sizeof(IMAGE_DATA_DIRECTORY);
        if (offset >= imageSizeOfHeaders)
            break;

        PvHashMappedImageData(hashContext, PTR_ADD_OFFSET(PvMappedImage.ViewBase, offset), sizeof(BYTE));
        offset++;
    }

    if (offset < PAGE_SIZE)
    {
        ULONG paddingLength;
        PVOID paddingBuffer;

        paddingLength = PAGE_SIZE - offset;
        paddingBuffer = PhAllocateZero(paddingLength);

        PvHashMappedImageData(hashContext, paddingBuffer, paddingLength);
        PhFree(paddingBuffer);
    }

    hashString = PvGetFinalHash(hashContext);
    PvDestroyHashHandle(hashContext);

    return hashString;
}

PPH_HASHTABLE PvGenerateOrdinalHashtableStringRef(
    _In_ PPH_STRINGREF FileName
    )
{
    PPH_HASHTABLE ordinalHashtable = NULL;
    PH_MAPPED_IMAGE mappedImage;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
    PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;

    if (NT_SUCCESS(PhLoadMappedImageEx(FileName, NULL, &mappedImage)))
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

FORCEINLINE
PPH_HASHTABLE
NTAPI
PvGenerateOrdinalHashtable(
    _In_ PWSTR FileName
    )
{
    PH_STRINGREF fileName;

    PhInitializeStringRef(&fileName, FileName);

    return PvGenerateOrdinalHashtableStringRef(&fileName);
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
    PPV_IMPHASH_ORDINAL_CACHE ordinalCache;

    ordinalCache = PhAllocateZero(sizeof(PV_IMPHASH_ORDINAL_CACHE));
    ordinalCache->Oleaut32Hashtable = PvGenerateOrdinalHashtable(L"\\SystemRoot\\System32\\oleaut32.dll");
    ordinalCache->Ws232Hashtable = PvGenerateOrdinalHashtable(L"\\SystemRoot\\System32\\ws2_32.dll");
    ordinalCache->WinsockHashtable = PvGenerateOrdinalHashtable(L"\\SystemRoot\\System32\\wsock32.dll");

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

BOOLEAN PvGetMappedImageImphash(
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

                        // "%s,"
                        PhAppendStringBuilder(&stringBuilder, &importImphashName->sr);
                        PhAppendStringBuilder2(&stringBuilder,L",");

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
            _wcsupr(hashString->Buffer);
        }

        // Generate the "impfuzzy" hash:
        // https://blogs.jpcert.or.jp/en/2016/05/classifying-mal-a988.html
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

NTSTATUS PvPeFileHashThread(
    _In_ PPV_PE_HASHWND_CONTEXT Context
    )
{
    HANDLE fileHandle;
    BOOLEAN checkImports = !!PhGetMappedImageDirectoryEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_IMPORT);
    PPH_LIST pagehashesList = NULL;
    PPH_STRING crc32HashString = NULL;
    PPH_STRING md5HashString = NULL;
    PPH_STRING sha1HashString = NULL;
    PPH_STRING sha2HashString = NULL;
    PPH_STRING sha384HashString = NULL;
    PPH_STRING sha512HashString = NULL;
    PPH_STRING authentihashSha1String = NULL;
    PPH_STRING authentihashSha256String = NULL;
    PPH_STRING wdaghashSha1String = NULL;
    PPH_STRING wdaghashSha256String = NULL;
    PPH_STRING imphashString = NULL;
    PPH_STRING imphashFuzzyString = NULL;
    PPH_STRING impMsftHashString = NULL;
    PPH_STRING ssdeepHashString = NULL;
    PPH_STRING tlshHashString = NULL;

    // File hashes

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY
        )))
    {
        PvGetFileHashes(
            fileHandle,
            &crc32HashString,
            &md5HashString,
            &sha1HashString,
            &sha2HashString,
            &sha384HashString,
            &sha512HashString
            );

        if (checkImports)
        {
            PhSetFilePosition(fileHandle, NULL);
            PvGetMappedImageMicrosoftImpHash(fileHandle, &impMsftHashString);
        }

        PhSetFilePosition(fileHandle, NULL);
        fuzzy_hash_file(fileHandle, &ssdeepHashString);

        PhSetFilePosition(fileHandle, NULL);
        PvGetTlshFileHash(fileHandle, &tlshHashString);

        NtClose(fileHandle);
    }

    // Import hashes

    if (checkImports)
    {
        PvGetMappedImageImphash(&imphashString, &imphashFuzzyString);
    }

    // Fuzzy hashes

    if (PhIsNullOrEmptyString(ssdeepHashString))
    {
        fuzzy_hash_buffer(PvMappedImage.ViewBase, PvMappedImage.Size, &ssdeepHashString);
    }

    if (PhIsNullOrEmptyString(tlshHashString))
    {
        PvGetTlshBufferHash(PvMappedImage.ViewBase, PvMappedImage.Size, &tlshHashString);
    }

    // Authentihash (Authenticode)

    authentihashSha1String = PvGetMappedImageAuthentihash(BCRYPT_SHA1_ALGORITHM);
    authentihashSha256String = PvGetMappedImageAuthentihash(BCRYPT_SHA256_ALGORITHM);

    // Page hash (WDAC)

    wdaghashSha1String = PvGetMappedImageWdacPageHash(BCRYPT_SHA1_ALGORITHM);
    wdaghashSha256String = PvGetMappedImageWdacPageHash(BCRYPT_SHA256_ALGORITHM);

    // Page hash (Authenticode)

    pagehashesList = PvEnumSpcAuthenticodePageHashes();

    // Update results...
    {
        PPV_PE_HASH_RESULTS results;

        results = PhAllocateZero(sizeof(PV_PE_HASH_RESULTS));
        results->ImageHasImports = checkImports;
        results->Crc32HashString = crc32HashString;
        results->Md5HashString = md5HashString;
        results->Sha1HashString = sha1HashString;
        results->Sha2HashString = sha2HashString;
        results->Sha384HashString = sha384HashString;
        results->Sha512HashString = sha512HashString;
        results->AuthentihashSha1String = authentihashSha1String;
        results->AuthentihashSha256String = authentihashSha256String;
        results->WdaghashSha1String = wdaghashSha1String;
        results->WdaghashSha256String = wdaghashSha256String;
        results->ImphashString = imphashString;
        results->ImphashFuzzyString = imphashFuzzyString;
        results->ImpMsftHashString = impMsftHashString;
        results->SsdeepHashString = ssdeepHashString;
        results->TlshHashString = tlshHashString;
        results->PageHashList = pagehashesList;

        PostMessage(Context->WindowHandle, WM_PV_HASH_FINISHED, 0, (LPARAM)results);
    }

    return STATUS_SUCCESS;
}

VOID PvPeHashesAddListViewItem(
    _In_ HWND ListViewHandle,
    _In_ INT ListViewGroup,
    _In_ INT ListViewIndex,
    _In_ PULONG Count,
    _In_ BOOLEAN UpperCase,
    _In_opt_ PWSTR ErrorText,
    _In_ PWSTR Text,
    _In_opt_ PPH_STRING Result
    )
{
    INT lvItemIndex;
    WCHAR number[PH_PTR_STR_LEN_1];

    PhPrintUInt32(number, ++(*Count));
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, ListViewGroup, ListViewIndex, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, Text);

    if (!PhIsNullOrEmptyString(Result))
    {
        if (UpperCase) _wcsupr(Result->Buffer);

        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(Result));
        PhDereferenceObject(Result);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, ErrorText ? ErrorText : L"ERROR");
    }
}

INT_PTR CALLBACK PvpPeHashesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PE_HASHWND_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_PE_HASHWND_CONTEXT));
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
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Hash");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageHashesListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, L"File hashes");
            PhAddListViewGroup(context->ListViewHandle, PV_HASHLIST_CATEGORY_IMPORTHASH, L"Import hashes");
            PhAddListViewGroup(context->ListViewHandle, PV_HASHLIST_CATEGORY_FUZZYHASH, L"Fuzzy hashes");
            PhAddListViewGroup(context->ListViewHandle, PV_HASHLIST_CATEGORY_AUTHENTIHASH, L"Authenticode hashes");
            PhAddListViewGroup(context->ListViewHandle, PV_HASHLIST_CATEGORY_WDACPAGEHASH, L"Page hashes (WDAC)");
            PhAddListViewGroup(context->ListViewHandle, PV_HASHLIST_CATEGORY_PAGEHASH, L"Page hashes (Authenticode)");

            PhCreateThread2(PvPeFileHashThread, context);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageHashesListViewColumns", context->ListViewHandle);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);
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
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case LVN_GETEMPTYMARKUP:
                {
                    NMLVEMPTYMARKUP* listview = (NMLVEMPTYMARKUP*)lParam;

                    listview->dwFlags = EMF_CENTERED;
                    wcsncpy_s(listview->szMarkup, RTL_NUMBER_OF(listview->szMarkup), L"Generating hashes...", _TRUNCATE);

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                break;
            }

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
    case WM_PV_HASH_FINISHED:
        {
            PPV_PE_HASH_RESULTS results = (PPV_PE_HASH_RESULTS)lParam;
            ULONG count = 0;

            ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
            ListView_DeleteAllItems(context->ListViewHandle);

            // File hashes

            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_CRC32, &count, TRUE, NULL, L"CRC32", results->Crc32HashString);
            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_MD5, &count, TRUE, NULL, L"MD5", results->Md5HashString);
            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_SHA1, &count, TRUE, NULL, L"SHA-1", results->Sha1HashString);
            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_SHA256, &count, TRUE, NULL, L"SHA-256", results->Sha2HashString);
            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_SHA348, &count, TRUE, NULL, L"SHA-384", results->Sha384HashString);
            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_SHA512, &count, TRUE, NULL, L"SHA-512", results->Sha512HashString);

            // Import hashes

            if (results->ImageHasImports)
            {
                PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_IMPORTHASH, PV_HASHLIST_INDEX_IMPHASH, &count, FALSE, NULL, L"Imphash", results->ImphashString);
                PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_IMPORTHASH, PV_HASHLIST_INDEX_IMPHASHMSFT, &count, FALSE, NULL, L"Imphash (Microsoft)", results->ImpMsftHashString);
                PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_FUZZYHASH, PV_HASHLIST_INDEX_IMPFUZZY, &count, FALSE, NULL, L"Impfuzzy", results->ImphashFuzzyString);
            }
            else
            {
                PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_IMPORTHASH, PV_HASHLIST_INDEX_IMPHASH, &count, FALSE, L"N/A", L"Imphash", NULL);
                PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_IMPORTHASH, PV_HASHLIST_INDEX_IMPHASHMSFT, &count, FALSE, L"N/A", L"Imphash (Microsoft)", NULL);
                PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_FUZZYHASH, PV_HASHLIST_INDEX_IMPFUZZY, &count, FALSE, L"N/A", L"Impfuzzy", NULL);
            }

            // Fuzzy hashes

            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_FUZZYHASH, PV_HASHLIST_INDEX_SSDEEP, &count, FALSE, NULL, L"SSDEEP", results->SsdeepHashString);
            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_FUZZYHASH, PV_HASHLIST_INDEX_TLSH, &count, FALSE, NULL, L"TLSH", results->TlshHashString);

            // Authentihash (Authenticode)

            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_AUTHENTIHASH, PV_HASHLIST_INDEX_AUTHENTIHASH_SHA1, &count, TRUE, NULL, L"Authentihash (SHA-1)", results->AuthentihashSha1String);
            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_AUTHENTIHASH, PV_HASHLIST_INDEX_AUTHENTIHASH_SHA256, &count, TRUE, NULL, L"Authentihash (SHA-256)", results->AuthentihashSha256String);

            // Page hash (WDAC)

            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_WDACPAGEHASH, PV_HASHLIST_INDEX_WDACPAGEHASH_SHA1, &count, TRUE, NULL, L"WDAC (SHA-1)", results->WdaghashSha1String);
            PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_WDACPAGEHASH, PV_HASHLIST_INDEX_WDACPAGEHASH_SHA256, &count, TRUE, NULL, L"WDAC (SHA-256)", results->WdaghashSha256String);

            // Page hash (Authenticode)

            if (results->PageHashList)
            {
                for (ULONG i = 0; i < results->PageHashList->Count; i++)
                {
                    PSPC_PE_IMAGE_PAGE_HASHES_V2 entry = results->PageHashList->Items[i];
                    PPH_STRING hashString;
                    WCHAR number[PH_PTR_STR_LEN_1];

                    PhPrintPointer(number, UlongToPtr(entry->PageOffset));
                    hashString = PhBufferToHexString(entry->PageHash, sizeof(entry->PageHash));

                    PvPeHashesAddListViewItem(context->ListViewHandle, PV_HASHLIST_CATEGORY_PAGEHASH, MAXINT, &count, TRUE, NULL, number, hashString);
                    PhFree(entry);
                }

                PhDereferenceObject(results->PageHashList);
            }

            ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
        }
        break;
    }

    return FALSE;
}
