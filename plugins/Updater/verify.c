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
    0x07, 0xA4, 0x51, 0xD8, 0xD2, 0xA6, 0xC8, 0x29,
    0x25, 0x66, 0x83, 0x33, 0x6D, 0x66, 0x12, 0xDF,
    0x01, 0xDA, 0x06, 0x5A, 0x2D, 0xFA, 0x7C, 0x9B,
    0x16, 0xFC, 0xA0, 0xA2, 0xB5, 0x2D, 0xAD, 0x2E,
    0xD7, 0x17, 0xD2, 0x1B, 0x44, 0x1A, 0x32, 0x20,
    0x1B, 0x21, 0xDF, 0x4F, 0x0C, 0x28, 0xA9, 0x80,
    0x2E, 0x4B, 0x0B, 0xED, 0xCA, 0xBE, 0x61, 0xB6,
    0x37, 0x62, 0xAB, 0xA5, 0x7E, 0xB5, 0xA4, 0x3D,
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
    0x42, 0xC8, 0x12, 0x8B, 0x37, 0x36, 0x60, 0x0D,
    0xB7, 0x5C, 0x6D, 0x80, 0xB0, 0x1A, 0xBE, 0x74,
    0xFF, 0x29, 0xBF, 0x6E, 0xD2, 0x05, 0x7D, 0x6E,
    0x54, 0x44, 0x23, 0xB0, 0x35, 0x13, 0xB5, 0xBF,
    0xFE, 0xE8, 0xCE, 0xF8, 0x3F, 0x3D, 0xED, 0x95,
    0x0E, 0x12, 0x53, 0xF9, 0xE9, 0x3B, 0x4D, 0x3D,
    0xA8, 0xC3, 0xA0, 0xD1, 0xCD, 0x06, 0xD5, 0xEC,
    0x66, 0x1B, 0x1B, 0x20, 0xE7, 0xFF, 0x6A, 0x46,
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
