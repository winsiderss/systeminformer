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
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00,
    0x91, 0x7C, 0xDF, 0x3D, 0x09, 0xBF, 0xF9, 0xCA,
    0xA7, 0x5E, 0x96, 0xEA, 0x38, 0x91, 0x24, 0xCA,
    0xF3, 0x5D, 0x87, 0xA4, 0x4A, 0x96, 0x48, 0xA7,
    0xC1, 0xB2, 0x07, 0x1C, 0x8E, 0x09, 0x3A, 0x37,
    0x41, 0x9B, 0xC5, 0x70, 0x64, 0xAD, 0xAE, 0x24,
    0x46, 0xA1, 0xB4, 0xE9, 0x1C, 0xE0, 0x79, 0x6D,
    0x57, 0x71, 0xD0, 0xD0, 0xF3, 0x72, 0x3D, 0x67,
    0x31, 0xAB, 0x77, 0xF8, 0xFD, 0xE6, 0x28, 0xF0,
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

BOOLEAN UpdaterVerifyHash(
    _Inout_ PUPDATER_HASH_CONTEXT Context,
    _In_ PPH_STRING Sha2Hash
    )
{
    PPH_STRING sha2HexString;

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

    if (!(sha2HexString = PhBufferToHexString(Context->Hash, Context->HashSize)))
        return FALSE;

    if (!PhEqualString(sha2HexString, Sha2Hash, TRUE))
    {
        PhDereferenceObject(sha2HexString);
        return FALSE;
    }

    PhDereferenceObject(sha2HexString);
    return TRUE;
}

BOOLEAN UpdaterVerifySignature(
    _Inout_ PUPDATER_HASH_CONTEXT Context,
    _In_ PPH_STRING HexSignature
    )
{
    ULONG signatureLength;
    PUCHAR signatureBuffer;

    signatureLength = (ULONG)HexSignature->Length / sizeof(WCHAR) / 2;
    signatureBuffer = PhAllocate(signatureLength);

    if (!PhHexStringToBuffer(&HexSignature->sr, signatureBuffer))
    {
        PhFree(signatureBuffer);
        return FALSE;
    }

    // Verify the signature.

    if (!NT_SUCCESS(BCryptVerifySignature(
        Context->KeyHandle,
        NULL,
        Context->Hash,
        Context->HashSize,
        signatureBuffer,
        signatureLength,
        0
        )))
    {
        PhFree(signatureBuffer);
        return FALSE;
    }

    PhFree(signatureBuffer);
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