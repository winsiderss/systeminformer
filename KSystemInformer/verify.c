/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     jxy-s   2020-2022
 *
 */

#include <kph.h>

#include <trace.h>

typedef enum _KPH_KEY_TYPE
{
    KphKeyTypeTest,
    KphKeyTypeProd,
} KPH_KEY_TYPE, *PKPH_KEY_TYPE;

#define KPH_KEY_MATERIAL_SIZE ((ULONG)72)

typedef struct _KPH_KEY
{
    KPH_KEY_TYPE Type;
    BYTE Material[KPH_KEY_MATERIAL_SIZE];
} KPH_KEY, *PKPH_KEY;
typedef const KPH_KEY* PCKPH_KEY;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const KPH_KEY KphpPublicKeys[] =
{
    {
        KphKeyTypeProd,
        {
            0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00,
            0x7C, 0x32, 0xAB, 0xA6, 0x40, 0x79, 0x9C, 0x00,
            0x67, 0xE2, 0x54, 0x7E, 0x0E, 0x4F, 0x55, 0x4A,
            0xE1, 0x78, 0xC2, 0x62, 0x9A, 0x96, 0x81, 0x63,
            0xCD, 0x4E, 0x3A, 0x28, 0x52, 0xB2, 0x4D, 0x28,
            0x6A, 0x96, 0xE5, 0x7D, 0x1B, 0xCB, 0x9B, 0x27,
            0xC8, 0xFD, 0x53, 0xDC, 0x00, 0x0D, 0x96, 0x62,
            0xA2, 0x55, 0x38, 0x71, 0xF0, 0x0F, 0xCC, 0x8F,
            0x84, 0xF4, 0x2B, 0x60, 0x38, 0xA6, 0xE7, 0x37,
        }
    }
};
static const UNICODE_STRING KphpSigExtension = RTL_CONSTANT_STRING(L".sig");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static BCRYPT_ALG_HANDLE KphpBCryptSigningProvider = NULL;
static BCRYPT_KEY_HANDLE KphpPublicKeyHandles[ARRAYSIZE(KphpPublicKeys)] = { 0 };
static ULONG KphpPublicKeyHandlesCount = 0;
C_ASSERT(ARRAYSIZE(KphpPublicKeyHandles) == ARRAYSIZE(KphpPublicKeys));
KPH_PROTECTED_DATA_SECTION_POP();

PAGED_FILE();

/**
 * \brief Verifies that the specified signature matches the specified hash by
 * using one of the known public keys.
 *
 * \param[in] KeyHandle The key handle to use for verification, if NULL the
 * pinned keys are used to verify the signature.
 * \param[in] Hash The hash to verify.
 * \param[in] Signature The signature to check.
 * \param[in] SignatureLength The length of the signature.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpVerifyHash(
    _In_opt_ BCRYPT_KEY_HANDLE KeyHandle,
    _In_ PKPH_HASH Hash,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    NTSTATUS status;

    PAGED_CODE_PASSIVE();

    status = STATUS_UNSUCCESSFUL;

    if (KeyHandle)
    {
        status = BCryptVerifySignature(KeyHandle,
                                       NULL,
                                       Hash->Buffer,
                                       Hash->Size,
                                       Signature,
                                       SignatureLength,
                                       0);
    }
    else
    {
        for (ULONG i = 0; i < KphpPublicKeyHandlesCount; i++)
        {
            status = BCryptVerifySignature(KphpPublicKeyHandles[i],
                                           NULL,
                                           Hash->Buffer,
                                           Hash->Size,
                                           Signature,
                                           SignatureLength,
                                           0);
            if (NT_SUCCESS(status))
            {
                return status;
            }
        }
    }

    return status;
}

/**
 * \brief Closes a verification key handle.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphVerifyCloseKey(
    _In_ BCRYPT_KEY_HANDLE KeyHandle
    )
{
    PAGED_CODE_PASSIVE();

    BCryptDestroyKey(KeyHandle);
}

/**
 * \brief Creates a verification key handle.
 *
 * \param[out] KeyHandle The created key handle.
 * \param[in] KeyMaterial The key material to use.
 * \param[in] KeyMaterialLength The length of the key material.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyCreateKey(
    _Out_ BCRYPT_KEY_HANDLE* KeyHandle,
    _In_ PBYTE KeyMaterial,
    _In_ ULONG KeyMaterialLength
    )
{
    NTSTATUS status;

    PAGED_CODE_PASSIVE();

    NT_ASSERT(KphpBCryptSigningProvider);

    status = BCryptImportKeyPair(KphpBCryptSigningProvider,
                                 NULL,
                                 BCRYPT_ECCPUBLIC_BLOB,
                                 KeyHandle,
                                 (PUCHAR)KeyMaterial,
                                 KeyMaterialLength,
                                 0);
    if (!NT_SUCCESS(status))
    {
        *KeyHandle = NULL;
    }

    return status;
}

/**
 * \brief Verifies a buffer matches the provided signature.
 *
 * \param[in] KeyHandle The key handle to use for verification, if NULL the
 * pinned keys are used to verify the signature.
 * \param[in] Buffer The buffer to verify.
 * \param[in] BufferLength The length of the buffer.
 * \param[in] Signature The signature to check.
 * \param[in] SignatureLength The length of the signature.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyBufferEx(
    _In_opt_ BCRYPT_KEY_HANDLE KeyHandle,
    _In_ PBYTE Buffer,
    _In_ ULONG BufferLength,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    NTSTATUS status;
    KPH_HASH hash;

    PAGED_CODE_PASSIVE();

    status = KphHashBuffer(Buffer, BufferLength, CALG_SHA_256, &hash);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "KphHashBuffer failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphpVerifyHash(KeyHandle, &hash, Signature, SignatureLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "KphpVerifyHash failed: %!STATUS!",
                      status);

        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Verifies a buffer matches the provided signature.
 *
 * \param[in] Buffer The buffer to verify.
 * \param[in] BufferLength The length of the buffer.
 * \param[in] Signature The signature to check.
 * \param[in] SignatureLength The length of the signature.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyBuffer(
    _In_ PBYTE Buffer,
    _In_ ULONG BufferLength,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    PAGED_CODE_PASSIVE();

    return KphVerifyBufferEx(NULL,
                             Buffer,
                             BufferLength,
                             Signature,
                             SignatureLength);
}

/**
 * \brief Verifies that a file matches the provided signature.
 *
 * \param[in] FileName File name of file to verify.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyFile(
    _In_ PCUNICODE_STRING FileName
    )
{
    NTSTATUS status;
    KPH_HASH hash;
    UNICODE_STRING signatureFileName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE signatureFileHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    BYTE signature[MINCRYPT_MAX_HASH_LEN];

    PAGED_CODE_PASSIVE();

    signatureFileHandle = NULL;
    RtlZeroMemory(&signatureFileName, sizeof(signatureFileName));

    if ((FileName->Length <= KphpSigExtension.Length) ||
        (FileName->Buffer[0] != L'\\'))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "File name is invalid \"%wZ\"",
                      FileName);

        status = STATUS_OBJECT_NAME_INVALID;
        goto Exit;
    }

    status = RtlDuplicateUnicodeString(0, FileName, &signatureFileName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "RtlDuplicateUnicodeString failed: %!STATUS!",
                      status);

        RtlZeroMemory(&signatureFileName, sizeof(signatureFileName));
        goto Exit;
    }

    //
    // Replace the file extension with ".sig". Our verification requires that
    // the signature file be placed alongside the file we're verifying.
    //
    if (signatureFileName.Length < KphpSigExtension.Length)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "Signature file name length invalid \"%wZ\"",
                      &signatureFileName);

        status = STATUS_OBJECT_NAME_INVALID;
        goto Exit;
    }

    signatureFileName.Length -= KphpSigExtension.Length;

    status = RtlAppendUnicodeStringToString(&signatureFileName,
                                            &KphpSigExtension);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "RtlAppendUnicodeStringToString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &signatureFileName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = KphCreateFile(&signatureFileHandle,
                           FILE_READ_ACCESS | SYNCHRONIZE,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0,
                           IO_IGNORE_SHARE_ACCESS_CHECK,
                           KernelMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "Failed to open signature file \"%wZ\": %!STATUS!",
                      &signatureFileName,
                      status);

        signatureFileHandle = NULL;
        goto Exit;
    }

    status = ZwReadFile(signatureFileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &ioStatusBlock,
                        signature,
                        ARRAYSIZE(signature),
                        NULL,
                        NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "Failed to read signature file \"%wZ\": %!STATUS!",
                      &signatureFileName,
                      status);

        goto Exit;
    }

    NT_ASSERT(ioStatusBlock.Information <= ARRAYSIZE(signature));

    status = KphHashFileByName(FileName, CALG_SHA_256, &hash);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "KphHashFileByName failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpVerifyHash(NULL,
                            &hash,
                            signature,
                            (ULONG)ioStatusBlock.Information);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "KphpVerifyHash failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    RtlFreeUnicodeString(&signatureFileName);

    if (signatureFileHandle)
    {
        ObCloseHandle(signatureFileHandle, KernelMode);
    }

    return status;
}

/**
 * \brief Initializes verification infrastructure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeVerify(
    VOID
    )
{
    NTSTATUS status;
    BOOLEAN testSigning;

    PAGED_CODE_PASSIVE();

    status = BCryptOpenAlgorithmProvider(&KphpBCryptSigningProvider,
                                         BCRYPT_ECDSA_P256_ALGORITHM,
                                         NULL,
                                         0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "BCryptOpenAlgorithmProvider failed: %!STATUS!",
                      status);

        KphpBCryptSigningProvider = NULL;
        return status;
    }

    testSigning = KphTestSigningEnabled();

    for (ULONG i = 0; i < ARRAYSIZE(KphpPublicKeys); i++)
    {
        PCKPH_KEY key;
        BCRYPT_KEY_HANDLE keyHandle;

        key = &KphpPublicKeys[i];

        if (!testSigning && (key->Type == KphKeyTypeTest))
        {
            continue;
        }

        status = KphVerifyCreateKey(&keyHandle,
                                    (PBYTE)key->Material,
                                    KPH_KEY_MATERIAL_SIZE);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          VERIFY,
                          "KphVerifyCreateKey failed: %!STATUS!",
                          status);

            return status;
        }

        KphpPublicKeyHandles[KphpPublicKeyHandlesCount++] = keyHandle;
    }

    return status;
}

/**
 * \brief Cleans up verification infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupVerify(
    VOID
    )
{
    PAGED_CODE_PASSIVE();

    for (ULONG i = 0; i < KphpPublicKeyHandlesCount; i++)
    {
        KphVerifyCloseKey(KphpPublicKeyHandles[i]);
    }

    if (KphpBCryptSigningProvider)
    {
        BCryptCloseAlgorithmProvider(KphpBCryptSigningProvider, 0);
    }
}
