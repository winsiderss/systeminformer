/*
 * KProcessHacker
 *
 * Copyright (C) 2016 wj32
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

#include <kph.h>

#define FILE_BUFFER_SIZE (2 * PAGE_SIZE)
#define FILE_MAX_SIZE (32 * 1024 * 1024) // 32 MB

static UCHAR KphpTrustedPublicKey[] =
{
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00, 0x5f, 0xe8, 0xab, 0xac, 0x01, 0xad, 0x6b, 0x48,
    0xfd, 0x84, 0x7f, 0x43, 0x70, 0xb6, 0x57, 0xb0, 0x76, 0xe3, 0x10, 0x07, 0x19, 0xbd, 0x0e, 0xd4,
    0x10, 0x5c, 0x1f, 0xfc, 0x40, 0x91, 0xb6, 0xed, 0x94, 0x37, 0x76, 0xb7, 0x86, 0x88, 0xf7, 0x34,
    0x12, 0x91, 0xf6, 0x65, 0x23, 0x58, 0xc9, 0xeb, 0x2f, 0xcb, 0x96, 0x13, 0x8f, 0xca, 0x57, 0x7a,
    0xd0, 0x7a, 0xbf, 0x22, 0xde, 0xd2, 0x15, 0xfc
};

NTSTATUS KphHashFile(
    __in PUNICODE_STRING FileName,
    __out PVOID *Hash,
    __out PULONG HashSize
    )
{
    NTSTATUS status;
    BCRYPT_ALG_HANDLE hashAlgHandle = NULL;
    ULONG querySize;
    ULONG hashObjectSize;
    ULONG hashSize;
    PVOID hashObject = NULL;
    PVOID hash = NULL;
    BCRYPT_HASH_HANDLE hashHandle = NULL;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    HANDLE fileHandle = NULL;
    FILE_END_OF_FILE_INFORMATION endOfFileInfo;
    ULONG remainingBytes;
    ULONG bytesToRead;
    PVOID buffer;

    // Open the hash algorithm and allocate memory for the hash object.

    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&hashAlgHandle, KPH_HASH_ALGORITHM, NULL, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = BCryptGetProperty(hashAlgHandle, BCRYPT_OBJECT_LENGTH,
        (PUCHAR)&hashObjectSize, sizeof(ULONG), &querySize, 0)))
    {
        goto CleanupExit;
    }
    if (!NT_SUCCESS(status = BCryptGetProperty(hashAlgHandle, BCRYPT_HASH_LENGTH, (PUCHAR)&hashSize,
        sizeof(ULONG), &querySize, 0)))
    {
        goto CleanupExit;
    }

    if (!(hashObject = ExAllocatePoolWithTag(PagedPool, hashObjectSize, 'vhpK')))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }
    if (!(hash = ExAllocatePoolWithTag(PagedPool, hashSize, 'vhpK')))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = BCryptCreateHash(hashAlgHandle, &hashHandle, hashObject, hashObjectSize,
        NULL, 0, 0)))
    {
        goto CleanupExit;
    }

    // Open the file and compute the hash.

    InitializeObjectAttributes(&objectAttributes, FileName, OBJ_KERNEL_HANDLE, NULL, NULL);

    if (!NT_SUCCESS(status = ZwCreateFile(&fileHandle, FILE_GENERIC_READ, &objectAttributes,
        &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0)))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = ZwQueryInformationFile(fileHandle, &iosb, &endOfFileInfo,
        sizeof(FILE_END_OF_FILE_INFORMATION), FileEndOfFileInformation)))
    {
        goto CleanupExit;
    }

    if (endOfFileInfo.EndOfFile.QuadPart <= 0)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }
    if (endOfFileInfo.EndOfFile.QuadPart > FILE_MAX_SIZE)
    {
        status = STATUS_FILE_TOO_LARGE;
        goto CleanupExit;
    }

    if (!(buffer = ExAllocatePoolWithTag(PagedPool, FILE_BUFFER_SIZE, 'vhpK')))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    remainingBytes = (ULONG)endOfFileInfo.EndOfFile.QuadPart;

    while (remainingBytes != 0)
    {
        bytesToRead = FILE_BUFFER_SIZE;
        if (bytesToRead > remainingBytes)
            bytesToRead = remainingBytes;

        if (!NT_SUCCESS(status = ZwReadFile(fileHandle, NULL, NULL, NULL, &iosb, buffer, bytesToRead,
            NULL, NULL)))
        {
            goto CleanupExit;
        }
        if ((ULONG)iosb.Information != bytesToRead)
        {
            status = STATUS_INTERNAL_ERROR;
            goto CleanupExit;
        }

        if (!NT_SUCCESS(status = BCryptHashData(hashHandle, buffer, bytesToRead, 0)))
            goto CleanupExit;

        remainingBytes -= bytesToRead;
    }

    if (!NT_SUCCESS(status = BCryptFinishHash(hashHandle, hash, hashSize, 0)))
        goto CleanupExit;

    if (NT_SUCCESS(status))
    {
        *Hash = hash;
        *HashSize = hashSize;

        hash = NULL; // Don't free this in the cleanup section
    }

CleanupExit:
    if (buffer)
        ExFreePoolWithTag(buffer, 'vhpK');
    if (fileHandle)
        ZwClose(fileHandle);
    if (hashHandle)
        BCryptDestroyHash(hashHandle);
    if (hash)
        ExFreePoolWithTag(hash, 'vhpK');
    if (hashObject)
        ExFreePoolWithTag(hashObject, 'vhpK');
    if (hashAlgHandle)
        BCryptCloseAlgorithmProvider(hashAlgHandle, 0);

    return status;
}

NTSTATUS KphVerifyFile(
    __in PUNICODE_STRING FileName,
    __in_bcount(SignatureSize) PVOID Signature,
    __in ULONG SignatureSize
    )
{
    NTSTATUS status;
    BCRYPT_ALG_HANDLE signAlgHandle = NULL;
    BCRYPT_KEY_HANDLE keyHandle = NULL;
    PVOID hash = NULL;
    ULONG hashSize;

    // Import the trusted public key.

    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&signAlgHandle, KPH_SIGN_ALGORITHM, NULL, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = BCryptImportKeyPair(signAlgHandle, NULL, KPH_BLOB_PUBLIC, &keyHandle,
        KphpTrustedPublicKey, sizeof(KphpTrustedPublicKey), 0)))
    {
        goto CleanupExit;
    }

    // Hash the file.

    if (!NT_SUCCESS(status = KphHashFile(FileName, &hash, &hashSize)))
        goto CleanupExit;

    // Verify the hash.

    if (!NT_SUCCESS(status = BCryptVerifySignature(keyHandle, NULL, hash, hashSize, Signature,
        SignatureSize, 0)))
    {
        goto CleanupExit;
    }

CleanupExit:
    if (hash)
        ExFreePoolWithTag(hash, 'vhpK');
    if (keyHandle)
        BCryptDestroyKey(keyHandle);
    if (signAlgHandle)
        BCryptCloseAlgorithmProvider(signAlgHandle, 0);

    return status;
}
