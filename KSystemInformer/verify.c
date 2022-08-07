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
#include <dyndata.h>

#include <trace.h>

PAGED_FILE();

static UCHAR KphpTrustedPublicKey[] =
{
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00, 0x5f, 0xe8, 0xab, 0xac, 0x01, 0xad, 0x6b, 0x48,
    0xfd, 0x84, 0x7f, 0x43, 0x70, 0xb6, 0x57, 0xb0, 0x76, 0xe3, 0x10, 0x07, 0x19, 0xbd, 0x0e, 0xd4,
    0x10, 0x5c, 0x1f, 0xfc, 0x40, 0x91, 0xb6, 0xed, 0x94, 0x37, 0x76, 0xb7, 0x86, 0x88, 0xf7, 0x34,
    0x12, 0x91, 0xf6, 0x65, 0x23, 0x58, 0xc9, 0xeb, 0x2f, 0xcb, 0x96, 0x13, 0x8f, 0xca, 0x57, 0x7a,
    0xd0, 0x7a, 0xbf, 0x22, 0xde, 0xd2, 0x15, 0xfc
};

static UNICODE_STRING KphpSigExtension = RTL_CONSTANT_STRING(L".sig");

static BCRYPT_ALG_HANDLE KphpBCryptSigningProvider = NULL;
static BCRYPT_KEY_HANDLE KphpTrustedPublicKeyHandle = NULL;

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

    PAGED_PASSIVE();

    status = BCryptOpenAlgorithmProvider(&KphpBCryptSigningProvider,
                                         BCRYPT_ECDSA_P256_ALGORITHM,
                                         NULL,
                                         0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      VERIFY,
                      "BCryptOpenAlgorithmProvider failed: %!STATUS!",
                      status);

        KphpBCryptSigningProvider = NULL;
        return status;
    }

    status = BCryptImportKeyPair(KphpBCryptSigningProvider,
                                 NULL,
                                 BCRYPT_ECCPUBLIC_BLOB,
                                 &KphpTrustedPublicKeyHandle,
                                 KphpTrustedPublicKey,
                                 sizeof(KphpTrustedPublicKey),
                                 0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      VERIFY,
                      "BCryptImportKeyPair failed: %!STATUS!",
                      status);

        KphpTrustedPublicKeyHandle = NULL;
        return status;
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
    PAGED_PASSIVE();

    if (KphpTrustedPublicKeyHandle)
    {
        BCryptDestroyKey(KphpTrustedPublicKeyHandle);
        KphpTrustedPublicKeyHandle = NULL;
    }

    if (KphpBCryptSigningProvider)
    {
        BCryptCloseAlgorithmProvider(KphpBCryptSigningProvider, 0);
        KphpBCryptSigningProvider = NULL;
    }
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
    _In_ PUNICODE_STRING FileName
    )
{
    NTSTATUS status;
    KPH_HASH hash;
    UNICODE_STRING signatureFileName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE signatureFileHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    UCHAR signature[MINCRYPT_MAX_HASH_LEN];

    PAGED_PASSIVE();

    NT_ASSERT(KphpTrustedPublicKeyHandle);

    signatureFileHandle = NULL;

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

    status = ZwCreateFile(&signatureFileHandle,
                          FILE_READ_DATA,
                          &objectAttributes,
                          &ioStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ,
                          FILE_OPEN,
                          (FILE_NON_DIRECTORY_FILE |
                           FILE_SYNCHRONOUS_IO_NONALERT),
                          NULL,
                          0);
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

    status = BCryptVerifySignature(KphpTrustedPublicKeyHandle,
                                   NULL,
                                   hash.Buffer,
                                   hash.Size,
                                   signature,
                                   (ULONG)ioStatusBlock.Information,
                                   0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "BCryptVerifySignature failed: %!STATUS!",
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
 * \brief Performs a domination check between a calling process and a target process.
 *
 * \details A process dominates the other when the protected level of the
 * process exceeds the other. This domination check is not ideal, it is overly
 * strict and lacks enough information from the kernel to fully understand the
 * protected process state.
 *
 * \param[in] Client - Client information.
 * \param[in] Process - Calling process.
 * \param[in] ProcessTarget - Target process to check that the calling process
 * dominates.
 * \param[in] AccessMode - Access mode of the request, if KernelMode the
 * domination check is bypassed.
 *
 * \return Appropriate status:
 * STATUS_SUCCESS - The calling process dominates the target.
 * STATUS_ACCESS_DENIED - The calling process does not dominate the target.
*/
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphDominationCheck(
    _In_ PEPROCESS Process,
    _In_ PEPROCESS ProcessTarget,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    PS_PROTECTION processProtection;
    PS_PROTECTION targetProtection;

    PAGED_CODE();

    if (AccessMode == KernelMode)
    {
        //
        // Give the kernel what it wants...
        //
        return STATUS_SUCCESS;
    }

    //
    // Until Microsoft gives us more insight into protected process domination
    // we'll do a very strict check here:
    //

    if (NT_SUCCESS(KphGetProcessProtection(Process, &processProtection)) &&
        NT_SUCCESS(KphGetProcessProtection(ProcessTarget, &targetProtection)) &&
        (targetProtection.Type != PsProtectedTypeNone) &&
        (targetProtection.Type >= processProtection.Type))
    {
        //
        // Calling process protection does not dominate the other, deny access.
        // We could do our own domination check/mapping here with the signing
        // level, but it won't be great and Microsoft might change it, so we'll
        // do this strict check until Microsoft exports:
        // PsTestProtectedProcessIncompatibility
        // RtlProtectedAccess/RtlTestProtectedAccess
        //
        return STATUS_ACCESS_DENIED;
    }

    //
    // Either the protected process check is not exported or the verified
    // process dominates the target. Our domination check succeeded.
    //

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves the process state mask from a process.
 *
 * \param[in] Process The process to get the state from. 
 *
 * \return State mask describing what state the process is in.
 */
_IRQL_requires_max_(APC_LEVEL)
KPH_PROCESS_STATE KphGetProcessState(
    _In_ PKPH_PROCESS_CONTEXT Process
    )
{
    KPH_PROCESS_STATE processState;

    PAGED_CODE();

    processState = 0;

    if (Process->SecurelyCreated)
    {
        processState |= KPH_PROCESS_SECURELY_CREATED;
    }

    if (Process->VerifiedProcess)
    {
        processState |= KPH_PROCESS_VERIFIED_PROCESS;
    }

    if (Process->Protected)
    {
        processState |= KPH_PROCESS_PROTECTED_PROCESS;
    }

    if (Process->NumberOfUntrustedImageLoads == 0)
    {
        processState |= KPH_PROCESS_NO_UNTRUSTED_IMAGES;
    }

    if (!PsIsProcessBeingDebugged(Process->EProcess))
    {
        processState |= KPH_PROCESS_NOT_BEING_DEBUGGED;
    }

    if (!Process->FileObject)
    {
        return processState;
    }

    processState |= KPH_PROCESS_HAS_FILE_OBJECT;

    if (!IoGetTransactionParameterBlock(Process->FileObject))
    {
        processState |= KPH_PROCESS_NO_FILE_TRANSACTION;
    }

    if (!Process->FileObject->SectionObjectPointer)
    {
        return processState;
    }

    processState |= KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS;

    if (!MmDoesFileHaveUserWritableReferences(Process->FileObject->SectionObjectPointer))
    {
        processState |= KPH_PROCESS_NO_USER_WRITABLE_REFERENCES;
    }

    return processState;
}
