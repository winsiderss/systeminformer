/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2019
 *
 */

#include "updater.h"

static UCHAR UpdaterTrustedPublicKeyReleaseLegacy[] =
{
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00,
    0x7C, 0x8B, 0xE0, 0x83, 0x9A, 0x3D, 0x7B, 0x02,
    0xCB, 0xE9, 0xFC, 0x4B, 0x31, 0x71, 0xBC, 0xF6,
    0x8A, 0xE7, 0xA3, 0x36, 0x40, 0x59, 0x44, 0x10,
    0x8E, 0xC1, 0x22, 0x7B, 0x62, 0xDB, 0x6A, 0x8D,
    0xEA, 0x3E, 0x8F, 0xCE, 0x4C, 0x20, 0x29, 0xF0,
    0x5A, 0x7D, 0xBC, 0x24, 0x70, 0x91, 0xBE, 0x97,
    0x1F, 0x28, 0xBB, 0x77, 0x2A, 0x37, 0x42, 0xF8,
    0x74, 0xEB, 0x64, 0x29, 0x02, 0xF9, 0xB3, 0xD3,
};

static UCHAR UpdaterTrustedPublicKeyRelease[] =
{
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00,
    0x97, 0x77, 0xEC, 0xCB, 0x96, 0x2E, 0xE6, 0xC1,
    0x99, 0xC2, 0xA7, 0x9B, 0x77, 0xD1, 0x7B, 0x92,
    0x40, 0xB8, 0x01, 0xEF, 0x8D, 0x9D, 0x8C, 0x75,
    0x7F, 0x49, 0x5B, 0xA1, 0x61, 0x53, 0x9D, 0xF9,
    0x09, 0xEB, 0xA5, 0xFE, 0xE1, 0x46, 0x31, 0x74,
    0x0F, 0xF0, 0x6D, 0x70, 0x5A, 0x04, 0x0C, 0xC8,
    0xC3, 0xBD, 0x73, 0x9F, 0x75, 0x62, 0x52, 0x10,
    0xFB, 0xDF, 0x46, 0x79, 0xEC, 0xB1, 0x6D, 0x13,
};

static UCHAR UpdaterTrustedPublicKeyNightlyLegacy[] =
{
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00,
    0x99, 0xCA, 0xBD, 0x8E, 0xBC, 0x27, 0x39, 0x7B, 
    0x99, 0xE9, 0x5B, 0x19, 0x70, 0x59, 0xFD, 0x0B, 
    0x28, 0x24, 0x9F, 0xF8, 0xCF, 0x36, 0xA5, 0x61, 
    0x82, 0x58, 0x00, 0xC7, 0xE7, 0xD8, 0xF8, 0x9E, 
    0x73, 0x75, 0x38, 0x61, 0xB1, 0x88, 0xC1, 0x13, 
    0xE8, 0x38, 0x85, 0x92, 0x4C, 0x82, 0xF0, 0xBE, 
    0xE5, 0x8B, 0x5E, 0x47, 0x95, 0x90, 0x13, 0xD4, 
    0xBF, 0xA2, 0xC0, 0x4D, 0xB0, 0xA9, 0x72, 0xEB,
};

static UCHAR UpdaterTrustedPublicKeyNightly[] =
{
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00,
    0x27, 0x1D, 0xDD, 0x24, 0x9C, 0xA9, 0xBD, 0xDD,
    0x1A, 0x79, 0x31, 0xD2, 0xC8, 0xFB, 0xDD, 0xBC,
    0xE3, 0x99, 0x94, 0xB4, 0xCA, 0x68, 0x8A, 0x07,
    0x4E, 0xC7, 0x4B, 0x3E, 0x9E, 0xD4, 0x50, 0x18,
    0xAD, 0x6F, 0xC9, 0xC1, 0xE8, 0xF8, 0x04, 0x78,
    0x4A, 0x0A, 0x77, 0x25, 0x25, 0x5A, 0xDC, 0x0B,
    0x61, 0xF5, 0xE0, 0x05, 0x0D, 0xED, 0x66, 0x85,
    0x74, 0x05, 0x8B, 0xA9, 0x38, 0x6B, 0xAB, 0xE6,
};

PUPDATER_HASH_CONTEXT UpdaterInitializeHash(
    _In_ UPDATER_TYPE Type
    )
{
    ULONG querySize;
    PUPDATER_HASH_CONTEXT hashContext;
    PUCHAR publicKey;
    ULONG publicKeySize;

    if (Type == UpdaterTypeNightly)
    {
        publicKey = UpdaterTrustedPublicKeyNightly;
        publicKeySize = ARRAYSIZE(UpdaterTrustedPublicKeyNightly);
    }
    else if (Type == UpdaterTypeNightlyLegacy)
    {
        publicKey = UpdaterTrustedPublicKeyNightlyLegacy;
        publicKeySize = ARRAYSIZE(UpdaterTrustedPublicKeyNightlyLegacy);
    }
    else if (Type == UpdaterTypeRelease)
    {
        publicKey = UpdaterTrustedPublicKeyRelease;
        publicKeySize = ARRAYSIZE(UpdaterTrustedPublicKeyRelease);
    }
    else if (Type == UpdaterTypeReleaseLegacy)
    {
        publicKey = UpdaterTrustedPublicKeyReleaseLegacy;
        publicKeySize = ARRAYSIZE(UpdaterTrustedPublicKeyReleaseLegacy);
    }
    else
    {
        return NULL;
    }

    hashContext = PhAllocate(sizeof(UPDATER_HASH_CONTEXT));
    memset(hashContext, 0, sizeof(UPDATER_HASH_CONTEXT));

    // Import the trusted public key.
    if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(
        &hashContext->SignAlgHandle,
        BCRYPT_ECDSA_P256_ALGORITHM,
        NULL,
        0
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(BCryptImportKeyPair(
        hashContext->SignAlgHandle,
        NULL,
        BCRYPT_ECCPUBLIC_BLOB,
        &hashContext->KeyHandle,
        publicKey,
        publicKeySize,
        0
        )))
    {
        goto CleanupExit;
    }

    // Open the hash algorithm and allocate memory for the hash object.

    if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(
        &hashContext->HashAlgHandle,
        BCRYPT_SHA256_ALGORITHM,
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

    UpdaterDestroyHash(hashContext);
    return NULL;
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

    if (!HexSignature)
        return FALSE;

    signatureLength = (ULONG)HexSignature->Length / sizeof(WCHAR) / 2;
    signatureBuffer = PhAllocate(signatureLength);

    if (!PhHexStringToBufferEx(&HexSignature->sr, signatureLength, signatureBuffer))
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
    _In_ PUPDATER_HASH_CONTEXT Context
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

    PhFree(Context);
}
