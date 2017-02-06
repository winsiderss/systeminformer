#include "..\main.h"

static UCHAR ExtraPluginsPublicKey[] =
{
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00, 0x24, 0x33, 0x0F, 0x97,
    0x6B, 0xBA, 0x8B, 0xDD, 0x18, 0x2F, 0x7A, 0xAC, 0x53, 0x8F, 0xE3, 0x2A,
    0x30, 0xA8, 0x8A, 0x47, 0x12, 0x3A, 0x4D, 0xCB, 0x11, 0x6A, 0x7E, 0x61,
    0x32, 0xEE, 0xF8, 0xE9, 0x6A, 0x9B, 0x85, 0x23, 0x9F, 0x08, 0xEB, 0xC3,
    0x8C, 0x60, 0xFD, 0xDE, 0xD3, 0x73, 0xCB, 0xFE, 0x53, 0xF6, 0x08, 0xCB,
    0xE6, 0xF9, 0x34, 0x4B, 0x14, 0x37, 0x17, 0x74, 0xAF, 0xFF, 0xCC, 0xAB
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

    PhFree(Context);
}