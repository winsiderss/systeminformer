/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     jxy-s   2020-2023
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
        KphKeyTypeProd, // kph
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
    },
    {
        KphKeyTypeTest, // kph-dev
        {
           0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00,
           0xCA, 0x54, 0x8F, 0x81, 0x2D, 0xB5, 0x8A, 0x0A,
           0x8A, 0x71, 0xF2, 0x6F, 0x3E, 0xA2, 0x54, 0x7A,
           0x1C, 0xF6, 0xB8, 0x6A, 0x39, 0xAD, 0x6F, 0xED,
           0xFC, 0x84, 0x46, 0x7A, 0x29, 0x11, 0xE7, 0x7D,
           0x19, 0xA8, 0x5F, 0xEB, 0x8C, 0x4A, 0xBB, 0x88,
           0xFE, 0x19, 0x40, 0xE9, 0xAE, 0xA7, 0xD6, 0x0C,
           0xED, 0x87, 0x67, 0xB6, 0xD7, 0x57, 0x75, 0x7F,
           0x96, 0xF2, 0xB0, 0xFF, 0x77, 0x5F, 0x04, 0x41,
        }
    },
    {
        KphKeyTypeTest, // kph-test
        {
	        0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00,
            0x25, 0x64, 0x8F, 0xD2, 0x9B, 0x6D, 0x16, 0x6C,
            0x30, 0x3C, 0x0A, 0x1E, 0xD0, 0x63, 0x78, 0x45,
            0xD3, 0xDB, 0x13, 0xAD, 0xE2, 0xFD, 0xCF, 0xDC,
            0xEC, 0x23, 0xF4, 0x26, 0x19, 0xDF, 0x97, 0xBD,
            0xC8, 0xF0, 0x61, 0x2A, 0xCB, 0x59, 0xAB, 0x1D,
            0x96, 0xAA, 0xAC, 0x6D, 0x59, 0x45, 0x62, 0xA5,
            0x13, 0x6C, 0xF0, 0x42, 0x99, 0xF8, 0x90, 0x60,
            0x47, 0x9C, 0xAD, 0x3B, 0xC6, 0xB5, 0x82, 0x25,
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
    _In_ PKPH_HASH_INFORMATION HashInformation,
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
                                       HashInformation->Hash,
                                       HashInformation->Length,
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
                                           HashInformation->Hash,
                                           HashInformation->Length,
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
    KPH_HASH_INFORMATION hashInfo;

    PAGED_CODE_PASSIVE();

    status = KphHashBuffer(Buffer,
                           BufferLength,
                           KphHashAlgorithmSha256,
                           &hashInfo);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "KphHashBuffer failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphpVerifyHash(KeyHandle, &hashInfo, Signature, SignatureLength);
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

#define KphpVerifyValidFileName(FileName)                                      \
    (((FileName)->Length > KphpSigExtension.Length) &&                         \
     ((FileName)->Buffer[0] == L'\\'))

/**
 * \brief Verifies a file object.
 *
 * \param[in] FileObject File object to verify.
 * \param[in] FileName Optional file name to use for verification, if not
 * provided the file name is looked up from the file object.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyFileObject(
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PCUNICODE_STRING FileName
    )
{
    NTSTATUS status;
    PCUNICODE_STRING fileName;
    PUNICODE_STRING localFileName;
    UNICODE_STRING signatureFileName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE signatureFileHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    BYTE signature[MINCRYPT_MAX_HASH_LEN];
    ULONG signatureLength;
    KPH_HASH_INFORMATION hashInfo;

    PAGED_CODE_PASSIVE();

    localFileName = NULL;
    RtlZeroMemory(&signatureFileName, sizeof(signatureFileName));
    signatureFileHandle = NULL;

    if (FileName)
    {
        fileName = FileName;
    }
    else
    {
        status = KphGetNameFileObject(FileObject, &localFileName);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          VERIFY,
                          "KphGetNameFileObject failed: %!STATUS!",
                          status);

            goto Exit;
        }

        fileName = localFileName;
    }

    if (!KphpVerifyValidFileName(fileName))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "File name is invalid \"%wZ\"",
                      fileName);

        status = STATUS_OBJECT_NAME_INVALID;
        goto Exit;
    }

    status = RtlDuplicateUnicodeString(0, fileName, &signatureFileName);
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
                               OBJ_KERNEL_HANDLE | OBJ_DONT_REPARSE,
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
                           (FILE_NON_DIRECTORY_FILE |
                            FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_COMPLETE_IF_OPLOCKED),
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
    else if (status == STATUS_OPLOCK_BREAK_IN_PROGRESS)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "Failed to open signature file \"%wZ\": %!STATUS!",
                      &signatureFileName,
                      status);

        status = STATUS_SHARING_VIOLATION;
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

    signatureLength = (ULONG)ioStatusBlock.Information;

    hashInfo.Algorithm = KphHashAlgorithmSha256;

    status = KphQueryHashInformationFileObject(FileObject,
                                               &hashInfo,
                                               sizeof(hashInfo));
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "KphQueryHashInformationFileObject failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpVerifyHash(NULL, &hashInfo, signature, signatureLength);
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

    if (signatureFileHandle)
    {
        ObCloseHandle(signatureFileHandle, KernelMode);
    }

    RtlFreeUnicodeString(&signatureFileName);

    if (localFileName)
    {
        KphFreeNameFileObject(localFileName);
    }

    return status;
}

/**
 * \brief Verifies a file by name.
 *
 * \param[in] FileName File name to verify.
 * \param[in] FileObject Optional file object to use for verification. If
 * provided the opened file object is checked to match. This is useful when the
 * file object is known but not in a state where you can use it directly.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyFile(
    _In_ PCUNICODE_STRING FileName,
    _In_opt_ PFILE_OBJECT FileObject
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE fileHandle;
    PFILE_OBJECT fileObject;

    PAGED_CODE_PASSIVE();

    fileObject = NULL;
    fileHandle = NULL;

    if (!KphpVerifyValidFileName(FileName))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "File name is invalid \"%wZ\"",
                      FileName);

        status = STATUS_OBJECT_NAME_INVALID;
        goto Exit;
    }

    InitializeObjectAttributes(&objectAttributes,
                               (PUNICODE_STRING)FileName,
                               OBJ_KERNEL_HANDLE | OBJ_DONT_REPARSE,
                               NULL,
                               NULL);

    status = KphCreateFile(&fileHandle,
                           FILE_READ_ACCESS | SYNCHRONIZE,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           (FILE_NON_DIRECTORY_FILE |
                            FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_COMPLETE_IF_OPLOCKED),
                           NULL,
                           0,
                           IO_IGNORE_SHARE_ACCESS_CHECK,
                           KernelMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "KphCreateFile failed: %!STATUS!",
                      status);

        fileHandle = NULL;
        goto Exit;
    }
    else if (status == STATUS_OPLOCK_BREAK_IN_PROGRESS)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "KphCreateFile failed: %!STATUS!",
                      status);

        status = STATUS_SHARING_VIOLATION;
        goto Exit;
    }

    status = ObReferenceObjectByHandle(fileHandle,
                                       0,
                                       *IoFileObjectType,
                                       KernelMode,
                                       &fileObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        fileObject = NULL;
        goto Exit;
    }

    if (FileObject && !KphIsSameFile(FileObject, fileObject))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "File objects do not match!");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = KphVerifyFileObject(fileObject, FileName);

Exit:

    if (fileObject)
    {
        ObDereferenceObject(fileObject);
    }

    if (fileHandle)
    {
        ObCloseHandle(fileHandle, KernelMode);
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
