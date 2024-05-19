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

static UCHAR UpdaterTrustedPublicKeyPreview[] =
{
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00,
    0x66, 0x85, 0x77, 0x37, 0xAC, 0x02, 0x77, 0x9A,
    0x29, 0x05, 0xA8, 0x36, 0xBE, 0xF2, 0x6A, 0x54,
    0x33, 0xA7, 0xAE, 0xD5, 0x45, 0x7B, 0xB7, 0x2D,
    0x16, 0xBB, 0x4A, 0x7F, 0x1F, 0x24, 0x97, 0x52,
    0xCD, 0x5F, 0xDE, 0xA4, 0xF4, 0xC9, 0x0E, 0x9C,
    0xA9, 0xE2, 0x48, 0xF3, 0xE2, 0x9C, 0x02, 0x88,
    0xAB, 0xF5, 0xA7, 0xFB, 0x9E, 0x65, 0x46, 0xBF,
    0x1C, 0x02, 0xDC, 0x7C, 0x6E, 0x0C, 0x89, 0xA8,
};

static UCHAR UpdaterTrustedPublicKeyCanary[] =
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

static UCHAR UpdaterTrustedPublicKeyDeveloper[] =
{
	0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00,
    0xD1, 0x77, 0x0B, 0xBF, 0x8C, 0xA3, 0xF6, 0x20,
    0x60, 0x29, 0x3E, 0x70, 0xE8, 0xC3, 0x1B, 0x10,
	0xA4, 0x42, 0x21, 0xAD, 0x73, 0x8B, 0x8A, 0x31,
    0x3D, 0xC0, 0xD0, 0x8C, 0xD5, 0x1C, 0xC7, 0x33,
    0xA2, 0x00, 0x20, 0x0E, 0x24, 0xB5, 0x1A, 0xC8,
	0xC8, 0xDA, 0xCF, 0x2E, 0x2E, 0xD5, 0x9F, 0xEF,
    0xA7, 0x89, 0xFB, 0x99, 0x94, 0x14, 0x57, 0x5C,
    0x36, 0x04, 0x44, 0x8B, 0xA2, 0x92, 0xF8, 0x0E,
};

PUPDATER_HASH_CONTEXT UpdaterInitializeHash(
    _In_ PH_RELEASE_CHANNEL Channel
    )
{
    ULONG querySize;
    PUPDATER_HASH_CONTEXT hashContext;
    PUCHAR publicKey;
    ULONG publicKeySize;

    switch (Channel)
    {
    case PhReleaseChannel:
        publicKey = UpdaterTrustedPublicKeyRelease;
        publicKeySize = ARRAYSIZE(UpdaterTrustedPublicKeyRelease);
        break;
    case PhPreviewChannel:
        publicKey = UpdaterTrustedPublicKeyPreview;
        publicKeySize = ARRAYSIZE(UpdaterTrustedPublicKeyPreview);
        break;
    case PhCanaryChannel:
        publicKey = UpdaterTrustedPublicKeyCanary;
        publicKeySize = ARRAYSIZE(UpdaterTrustedPublicKeyCanary);
        break;
    case PhDeveloperChannel:
        publicKey = UpdaterTrustedPublicKeyDeveloper;
        publicKeySize = ARRAYSIZE(UpdaterTrustedPublicKeyDeveloper);
        break;
    default:
        return NULL;
    }

    hashContext = PhAllocateZero(sizeof(UPDATER_HASH_CONTEXT));

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
