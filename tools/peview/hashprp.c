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

#include <wincrypt.h>
#include <wintrust.h>

#define WM_PV_HASH_FINISHED (WM_APP + 701)

extern NTSTATUS fuzzy_hash_file(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING* HashResult
    );
extern BOOLEAN fuzzy_hash_buffer(
    _In_ PBYTE Buffer,
    _In_ ULONGLONG BufferLength,
    _Out_ PPH_STRING* HashResult
    );

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

// rev from "signtool verify /ph /v C:\\Windows\\explorer.exe" (dmex)
VOID PvEnumSpcAuthenticodePageHashes(
    _In_ HWND ListViewHandle,
    _In_ PULONG Count
    )
{
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
        return;

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
                        PPH_STRING hashString;
                        INT lvItemIndex;
                        WCHAR number[PH_PTR_STR_LEN_1];

                        PhPrintUInt32(number, ++(*Count));
                        lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_PAGEHASH, MAXINT, number, NULL);
                        PhPrintPointer(number, UlongToPtr(entry->PageOffset));
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, number);

                        hashString = PhBufferToHexString(entry->PageHash, sizeof(entry->PageHash));
                        _wcsupr(hashString->Buffer);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(hashString));
                        PhDereferenceObject(hashString);
                    }
                }
                else if (PhEqualBytesZ(spcSerializedObjectBuffer.pszObjId, SPC_PE_IMAGE_PAGE_HASHES_V2_OBJID, FALSE))
                {
                    ULONG count = spcImagePageHashesBuffer->cbData / sizeof(SPC_PE_IMAGE_PAGE_HASHES_V2);

                    for (ULONG k = 0; k < count; k++)
                    {
                        PSPC_PE_IMAGE_PAGE_HASHES_V2 entry = PTR_ADD_OFFSET(spcImagePageHashesBuffer->pbData, sizeof(SPC_PE_IMAGE_PAGE_HASHES_V2) * k);
                        PPH_STRING hashString;
                        INT lvItemIndex;
                        WCHAR number[PH_PTR_STR_LEN_1];

                        PhPrintUInt32(number, ++(*Count));
                        lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_PAGEHASH, MAXINT, number, NULL);
                        PhPrintPointer(number, UlongToPtr(entry->PageOffset));
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, number);

                        hashString = PhBufferToHexString(entry->PageHash, sizeof(entry->PageHash));
                        _wcsupr(hashString->Buffer);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(hashString));
                        PhDereferenceObject(hashString);
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
    BYTE buffer[PAGE_SIZE];

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
        PAGE_SIZE,
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

VOID PvEnumFileHashes(
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
        PvGetFileHashes(
            fileHandle,
            &crc32HashString,
            &md5HashString,
            &sha1HashString,
            &sha2HashString,
            &sha384HashString,
            &sha512HashString
            );

        PhSetFilePosition(fileHandle, NULL);
        PvGetMappedImageMicrosoftImpHash(fileHandle, &impMsftHashString);

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

    // File hashes

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_FILEHASH, PV_HASHLIST_INDEX_CRC32, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"CRC32");

    if (!PhIsNullOrEmptyString(crc32HashString))
    {
        _wcsupr(crc32HashString->Buffer);
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

    // Import hashes

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_IMPORTHASH, PV_HASHLIST_INDEX_IMPHASH, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"Imphash");

    if (PvGetMappedImageImphash(&imphashString, &imphashFuzzyString))
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
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"Imphash (Microsoft)");

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

    // SSDEEP

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

    // TLSH

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

    // Authentihash (Authenticode)

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_AUTHENTIHASH, PV_HASHLIST_INDEX_AUTHENTIHASH_SHA1, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"Authentihash (SHA1)");

    if (authentihashString = PvGetMappedImageAuthentihash(BCRYPT_SHA1_ALGORITHM))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(authentihashString));
        PhDereferenceObject(authentihashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_AUTHENTIHASH, PV_HASHLIST_INDEX_AUTHENTIHASH_SHA256, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"Authentihash (SHA2)");

    if (authentihashString = PvGetMappedImageAuthentihash(BCRYPT_SHA256_ALGORITHM))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(authentihashString));
        PhDereferenceObject(authentihashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    // Page hash (WDAC)

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_WDACPAGEHASH, PV_HASHLIST_INDEX_WDACPAGEHASH_SHA1, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"WDAC (SHA1)");

    if (authentihashString = PvGetMappedImageWdacPageHash(BCRYPT_SHA1_ALGORITHM))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(authentihashString));
        PhDereferenceObject(authentihashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    PhPrintUInt32(number, ++count);
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, PV_HASHLIST_CATEGORY_WDACPAGEHASH, PV_HASHLIST_INDEX_WDACPAGEHASH_SHA256, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"WDAC (SHA2)");

    if (authentihashString = PvGetMappedImageWdacPageHash(BCRYPT_SHA256_ALGORITHM))
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(authentihashString));
        PhDereferenceObject(authentihashString);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ERROR");
    }

    // Page hash (Authenticode)

    PvEnumSpcAuthenticodePageHashes(ListViewHandle, &count);
}

NTSTATUS PvPeFileHashThread(
    _In_ PPV_PE_HASHWND_CONTEXT Context
    )
{
    PvEnumFileHashes(Context->ListViewHandle);

    PostMessage(Context->WindowHandle, WM_PV_HASH_FINISHED, 0, 0);
    return STATUS_SUCCESS;
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

            ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
            ListView_DeleteAllItems(context->ListViewHandle);
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
            ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
        }
        break;
    }

    return FALSE;
}
