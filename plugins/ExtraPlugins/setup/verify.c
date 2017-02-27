#include "..\main.h"

static UCHAR ExtraPluginsPublicKey[] =
{
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00, 0x63, 0x35, 0x3E, 0x56, 0x42,
    0x64, 0xB3, 0x2F, 0xEE, 0x07, 0x85, 0x8A, 0x3D, 0x6E, 0x11, 0x98, 0x78, 0xE2,
    0xC7, 0x25, 0xD4, 0x15, 0x75, 0xD1, 0xDB, 0x63, 0x69, 0x56, 0x71, 0xCD, 0x86,
    0x05, 0xFD, 0xDE, 0xD8, 0x95, 0x89, 0xC4, 0xBC, 0x9B, 0x9E, 0x73, 0xE9, 0x74,
    0x7D, 0xB7, 0xAF, 0x07, 0x34, 0x57, 0x62, 0x72, 0x1A, 0x31, 0x46, 0x3E, 0x91,
    0x8A, 0x0B, 0x29, 0x8C, 0x97, 0xA4, 0x29
};

BOOLEAN UpdaterInitializeHash(
    _Out_ PUPDATER_HASH_CONTEXT *Context
    )
{
    ULONG querySize;
    PUPDATER_HASH_CONTEXT hashContext;

    hashContext = PhAllocate(sizeof(UPDATER_HASH_CONTEXT));
    memset(hashContext, 0, sizeof(UPDATER_HASH_CONTEXT));

    if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(
        &hashContext->SignAlgHandle,
        BCRYPT_ECDSA_P256_ALGORITHM,
        NULL,
        0
        )))
    {
        goto error;
    }

    if (!NT_SUCCESS(BCryptImportKeyPair(
        hashContext->SignAlgHandle,
        NULL,
        BCRYPT_ECCPUBLIC_BLOB,
        &hashContext->KeyHandle,
        ExtraPluginsPublicKey,
        sizeof(ExtraPluginsPublicKey),
        0
        )))
    {
        goto error;
    }

    if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(
        &hashContext->HashAlgHandle,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        0
        )))
    {
        goto error;
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
        goto error;
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
        goto error;
    }

    if (!(hashContext->HashObject = PhAllocate(hashContext->HashObjectSize)))
        goto error;
    if (!(hashContext->Hash = PhAllocate(hashContext->HashSize)))
        goto error;

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
        goto error;
    }

    *Context = hashContext;
    return TRUE;

error:
    UpdaterDestroyHash(hashContext);
    return FALSE;
}

BOOLEAN UpdaterUpdateHash(
    _Inout_ PUPDATER_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
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

    if (!PhEqualString2(sha2HexString, PhGetStringOrEmpty(Sha2Hash), TRUE))
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

    PhFree(Context);
}