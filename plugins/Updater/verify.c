/*
 * Process Hacker Plugins -
 *   Update Checker Plugin
 *
 * Copyright (C) 2016 dmex
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

#include "updater.h"

static UCHAR UpdaterTrustedPublicKey[] =
{
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00, 0x5f, 0xe8, 0xab, 0xac, 0x01, 0xad, 0x6b, 0x48,
    0xfd, 0x84, 0x7f, 0x43, 0x70, 0xb6, 0x57, 0xb0, 0x76, 0xe3, 0x10, 0x07, 0x19, 0xbd, 0x0e, 0xd4,
    0x10, 0x5c, 0x1f, 0xfc, 0x40, 0x91, 0xb6, 0xed, 0x94, 0x37, 0x76, 0xb7, 0x86, 0x88, 0xf7, 0x34,
    0x12, 0x91, 0xf6, 0x65, 0x23, 0x58, 0xc9, 0xeb, 0x2f, 0xcb, 0x96, 0x13, 0x8f, 0xca, 0x57, 0x7a,
    0xd0, 0x7a, 0xbf, 0x22, 0xde, 0xd2, 0x15, 0xfc
};

BOOLEAN UpdaterInitializeHash(
    _Out_ PUPDATER_HASH_CONTEXT *Context
    )
{
    BOOLEAN success = FALSE;
    ULONG querySize;
    PUPDATER_HASH_CONTEXT hashContext;

    hashContext = PhAllocate(sizeof(UPDATER_HASH_CONTEXT));
    memset(hashContext, 0, sizeof(UPDATER_HASH_CONTEXT));

    __try
    {
        // Import the trusted public key.

        if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(
            &hashContext->SignAlgHandle, 
            BCRYPT_ECDSA_P256_ALGORITHM, 
            NULL, 
            0
            )))
        {
            __leave;
        }

        if (!NT_SUCCESS(BCryptImportKeyPair(
            hashContext->SignAlgHandle, 
            NULL, 
            BCRYPT_ECCPUBLIC_BLOB, 
            &hashContext->KeyHandle, 
            UpdaterTrustedPublicKey, 
            sizeof(UpdaterTrustedPublicKey), 
            0
            )))
        {
            __leave;
        }

        // Open the hash algorithm and allocate memory for the hash object.

        if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(
            &hashContext->HashAlgHandle,
            BCRYPT_SHA256_ALGORITHM, 
            NULL, 
            0
            )))
        {
            __leave;
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
            __leave;
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
            __leave;
        }

        if (!(hashContext->HashObject = PhAllocate(hashContext->HashObjectSize)))
        {
            __leave;
        }

        if (!(hashContext->Hash = PhAllocate(hashContext->HashSize)))
        {
            __leave;
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
            __leave;
        }

        success = TRUE;
    }
    __finally
    {
        if (!success)
        {
            UpdaterDestroyHash(hashContext);
        }
    }

    if (success)
        *Context = hashContext;

    return success;
}

BOOLEAN UpdaterUpdateHash(
    _Inout_ PUPDATER_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    // Update the hash.

    return NT_SUCCESS(BCryptHashData(Context->HashHandle, Buffer, Length, 0));
}

BOOLEAN UpdaterFinalHash(
    _Inout_ PUPDATER_HASH_CONTEXT Context,
    _In_ ULONG SignatureSize,
    _In_ PVOID Signature
    )
{
    // Compute the final hash.

    if (!NT_SUCCESS(BCryptFinishHash(
        Context->HashHandle, 
        Context->Hash, 
        Context->HashSize, 
        0
        )))
    {
        return FALSE;
    }

    // Verify the hash.

    if (!NT_SUCCESS(BCryptVerifySignature(
        Context->KeyHandle, 
        NULL, 
        Context->Hash, 
        Context->HashSize, 
        Signature, 
        SignatureSize, 
        0
        )))
    {
        return FALSE;
    }

    return TRUE;
}

VOID UpdaterDestroyHash(
    _Inout_ PUPDATER_HASH_CONTEXT Context
    )
{
    if (Context->HashAlgHandle)
        BCryptCloseAlgorithmProvider(Context->HashAlgHandle, 0);

    if (Context->SignAlgHandle)
        BCryptCloseAlgorithmProvider(Context->SignAlgHandle, 0);

    if (Context->HashHandle)
        BCryptDestroyHash(Context->HashHandle);

    if (Context->KeyHandle)
        BCryptDestroyKey(Context->KeyHandle);

    if (Context->HashObject)
        PhFree(Context->HashObject);

    if (Context->Hash)
        PhFree(Context->Hash);
}