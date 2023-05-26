/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2016-2023
 *
 */

#include <ph.h>
#include <bcrypt.h>

#define CST_SIGN_ALGORITHM BCRYPT_ECDSA_P256_ALGORITHM
#define CST_SIGN_ALGORITHM_BITS 256
#define CST_HASH_ALGORITHM BCRYPT_SHA256_ALGORITHM
#define CST_BLOB_PRIVATE BCRYPT_ECCPRIVATE_BLOB
#define CST_BLOB_PUBLIC BCRYPT_ECCPUBLIC_BLOB

#define FILE_BUFFER_SIZE PAGE_SIZE

#define ARG_KEY 1
#define ARG_SIG 2
#define ARG_HEX 3

EXTERN_C PVOID __ImageBase;
PPH_STRING CstCommand = NULL;
PPH_STRING CstArgument1 = NULL;
PPH_STRING CstArgument2 = NULL;
PPH_STRING CstKeyFileName = NULL;
PPH_STRING CstSigFileName = NULL;
BOOLEAN CstHex = FALSE;

PWSTR CstHelpMessage =
    L"Usage: CustomSignTool.exe command ...\n"
    L"Commands:\n"
    L"createkeypair\tprivatekeyfile publickeyfile\n"
    L"sign\t\t-k privatekeyfile [-s outputsigfile] [-h] inputfile\n"
    L"verify\t\t-k publickeyfile -s inputsigfile inputfile\n"
    ;

static BOOLEAN NTAPI CstCommandLineCallback(
    _In_opt_ PPH_COMMAND_LINE_OPTION Option,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PVOID Context
    )
{
    if (Option)
    {
        switch (Option->Id)
        {
        case ARG_KEY:
            PhSwapReference(&CstKeyFileName, Value);
            break;
        case ARG_SIG:
            PhSwapReference(&CstSigFileName, Value);
            break;
        case ARG_HEX:
            CstHex = TRUE;
            break;
        }
    }
    else
    {
        if (!CstCommand)
            PhSwapReference(&CstCommand, Value);
        else if (!CstArgument1)
            PhSwapReference(&CstArgument1, Value);
        else if (!CstArgument2)
            PhSwapReference(&CstArgument2, Value);
    }

    return TRUE;
}

DECLSPEC_NORETURN
static VOID CstFailWith(
    _In_ PWSTR Message
    )
{
    wprintf(L"%s\n", Message);
    exit(1);
}

DECLSPEC_NORETURN
static VOID CstFailWithStatus(
    _In_ PWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    wprintf(L"%s: %s\n", Message, PhGetStatusMessage(Status, Win32Result)->Buffer);
    exit(1);
}

static VOID CstExportKey(
    _In_ BCRYPT_KEY_HANDLE KeyHandle,
    _In_ PWSTR BlobType,
    _In_ PWSTR FileName,
    _In_ PWSTR Description
    )
{
    NTSTATUS status;
    ULONG blobSize;
    PVOID blob;
    HANDLE fileHandle;
    IO_STATUS_BLOCK iosb;

    if (!NT_SUCCESS(status = BCryptExportKey(KeyHandle, NULL, BlobType, NULL, 0, &blobSize, 0)))
        CstFailWithStatus(PhFormatString(L"Unable to export %s: Unable to get blob size", Description)->Buffer, status, 0);
    blob = PhAllocate(blobSize);
    if (!NT_SUCCESS(status = BCryptExportKey(KeyHandle, NULL, BlobType, blob, blobSize, &blobSize, 0)))
        CstFailWithStatus(PhFormatString(L"Unable to export %s: Unable to get blob data", Description)->Buffer, status, 0);

    if (!NT_SUCCESS(status = PhCreateFileWin32Ex(&fileHandle, FileName, FILE_GENERIC_WRITE, &(LARGE_INTEGER){.QuadPart = blobSize}, FILE_ATTRIBUTE_NORMAL, 0, FILE_OVERWRITE_IF, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, NULL)))
        CstFailWithStatus(PhFormatString(L"Unable to create '%s'", FileName)->Buffer, status, 0);
    if (!NT_SUCCESS(status = NtWriteFile(fileHandle, NULL, NULL, NULL, &iosb, blob, blobSize, NULL, NULL)))
        CstFailWithStatus(PhFormatString(L"Unable to write blob to '%s'", FileName)->Buffer, status, 0);
    NtClose(fileHandle);

    RtlSecureZeroMemory(blob, blobSize);
    PhFree(blob);
}

static PVOID CstReadFile(
    _In_ PWSTR FileName,
    _In_ ULONG FileSizeLimit,
    _Out_ PULONG FileSize
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    PVOID buffer;
    IO_STATUS_BLOCK iosb;

    if (!NT_SUCCESS(status = PhCreateFileWin32(&fileHandle, FileName, FILE_GENERIC_READ, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE)))
        CstFailWithStatus(PhFormatString(L"Unable to open '%s'", FileName)->Buffer, status, 0);
    if (!NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize)))
        CstFailWithStatus(PhFormatString(L"Unable to get the size of '%s'", FileName)->Buffer, status, 0);
    if (fileSize.QuadPart > FileSizeLimit)
        CstFailWith(PhFormatString(L"The file '%s' is too large", FileName)->Buffer);
    buffer = PhAllocate((ULONG)fileSize.QuadPart);
    if (!NT_SUCCESS(status = NtReadFile(fileHandle, NULL, NULL, NULL, &iosb, buffer, (ULONG)fileSize.QuadPart, NULL, NULL)))
        CstFailWithStatus(PhFormatString(L"Unable to read '%s'", FileName)->Buffer, status, 0);
    NtClose(fileHandle);

    *FileSize = (ULONG)fileSize.QuadPart;

    return buffer;
}

static PVOID CstHashFile(
    _In_ PWSTR FileName,
    _Out_ PULONG HashSize
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    PVOID buffer;
    IO_STATUS_BLOCK iosb;
    BCRYPT_ALG_HANDLE hashAlgHandle;
    ULONG querySize;
    ULONG hashObjectSize;
    ULONG hashSize;
    PVOID hashObject;
    BCRYPT_HASH_HANDLE hashHandle;
    PVOID hash;

    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&hashAlgHandle, CST_HASH_ALGORITHM, NULL, 0)))
        CstFailWithStatus(L"Unable to open the hashing algorithm provider", status, 0);
    if (!NT_SUCCESS(status = BCryptGetProperty(hashAlgHandle, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjectSize, sizeof(ULONG), &querySize, 0)))
        CstFailWithStatus(L"Unable to query hash object size", status, 0);
    if (!NT_SUCCESS(status = BCryptGetProperty(hashAlgHandle, BCRYPT_HASH_LENGTH, (PUCHAR)&hashSize, sizeof(ULONG), &querySize, 0)))
        CstFailWithStatus(L"Unable to query hash size", status, 0);
    hashObject = PhAllocate(hashObjectSize);
    if (!NT_SUCCESS(status = BCryptCreateHash(hashAlgHandle, &hashHandle, hashObject, hashObjectSize, NULL, 0, 0)))
        CstFailWithStatus(L"Unable to get hash handle", status, 0);

    if (!NT_SUCCESS(status = PhCreateFileWin32(&fileHandle, FileName, FILE_GENERIC_READ, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE)))
        CstFailWithStatus(PhFormatString(L"Unable to open '%s'", FileName)->Buffer, status, 0);

    if (buffer = PhAllocatePage(FILE_BUFFER_SIZE, NULL))
    {
        while (TRUE)
        {
            status = NtReadFile(
                fileHandle,
                NULL,
                NULL,
                NULL,
                &iosb,
                buffer,
                FILE_BUFFER_SIZE,
                NULL,
                NULL
                );

            if (!NT_SUCCESS(status))
            {
                if (status == STATUS_END_OF_FILE)
                    break;

                CstFailWithStatus(PhFormatString(L"Unable to read '%s'", FileName)->Buffer, status, 0);
            }

            if (!NT_SUCCESS(status = BCryptHashData(hashHandle, buffer, (ULONG)iosb.Information, 0)))
                CstFailWithStatus(L"Unable to hash file", status, 0);
        }

        PhFreePage(buffer);
    }

    NtClose(fileHandle);

    hash = PhAllocate(hashSize);
    if (!NT_SUCCESS(status = BCryptFinishHash(hashHandle, hash, hashSize, 0)))
        CstFailWithStatus(L"Unable to complete the hash", status, 0);
    BCryptDestroyHash(hashHandle);
    PhFree(hashObject); // must be freed after destroying hash object
    BCryptCloseAlgorithmProvider(hashAlgHandle, 0);

    *HashSize = hashSize;

    return hash;
}

int __cdecl wmain(int argc, wchar_t *argv[])
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { ARG_KEY, L"k", MandatoryArgumentType },
        { ARG_SIG, L"s", MandatoryArgumentType },
        { ARG_HEX, L"h", NoArgumentType }
    };

    NTSTATUS status;
    PH_STRINGREF commandLine;

    if (!NT_SUCCESS(PhInitializePhLib(L"CustomSignTool", __ImageBase)))
        return EXIT_FAILURE;

    if (!NT_SUCCESS(PhGetProcessCommandLineStringRef(&commandLine)))
        return EXIT_FAILURE;

    if (!PhParseCommandLine(
        &commandLine,
        options,
        RTL_NUMBER_OF(options),
        PH_COMMAND_LINE_IGNORE_FIRST_PART,
        CstCommandLineCallback,
        NULL
        ))
    {
        return EXIT_FAILURE;
    }

    if (!CstCommand)
        CstFailWith(CstHelpMessage);

    if (PhEqualString2(CstCommand, L"createkeypair", TRUE))
    {
        BCRYPT_ALG_HANDLE signAlgHandle;
        BCRYPT_KEY_HANDLE keyHandle;

        if (!CstArgument1 || !CstArgument2)
            CstFailWith(CstHelpMessage);

        if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&signAlgHandle, CST_SIGN_ALGORITHM, NULL, 0)))
            CstFailWithStatus(L"Unable to open the signing algorithm provider", status, 0);
        if (!NT_SUCCESS(status = BCryptGenerateKeyPair(signAlgHandle, &keyHandle, CST_SIGN_ALGORITHM_BITS, 0)))
            CstFailWithStatus(L"Unable to create the key", status, 0);
        if (!NT_SUCCESS(status = BCryptFinalizeKeyPair(keyHandle, 0)))
            CstFailWithStatus(L"Unable to finalize the key", status, 0);

        CstExportKey(keyHandle, CST_BLOB_PRIVATE, CstArgument1->Buffer, L"private key");
        CstExportKey(keyHandle, CST_BLOB_PUBLIC, CstArgument2->Buffer, L"public key");

        BCryptDestroyKey(keyHandle);
        BCryptCloseAlgorithmProvider(signAlgHandle, 0);
    }
    else if (PhEqualString2(CstCommand, L"sign", TRUE))
    {
        BCRYPT_ALG_HANDLE signAlgHandle;
        ULONG bufferSize;
        PVOID buffer;
        IO_STATUS_BLOCK iosb;
        BCRYPT_KEY_HANDLE keyHandle;
        ULONG hashSize;
        PVOID hash;
        ULONG signatureSize;
        PVOID signature;
        PPH_STRING string;
        HANDLE fileHandle;

        if (!CstArgument1 || !CstKeyFileName || (!CstSigFileName && !CstHex))
            CstFailWith(CstHelpMessage);

        // Import the key.

        if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&signAlgHandle, CST_SIGN_ALGORITHM, NULL, 0)))
            CstFailWithStatus(L"Unable to open the signing algorithm provider", status, 0);
        buffer = CstReadFile(CstKeyFileName->Buffer, 1024 * 1024, &bufferSize);
        if (!NT_SUCCESS(status = BCryptImportKeyPair(signAlgHandle, NULL, CST_BLOB_PRIVATE, &keyHandle, buffer, bufferSize, 0)))
            CstFailWithStatus(PhFormatString(L"Unable to import the private key: %s", CstKeyFileName->Buffer)->Buffer, status, 0);
        PhFree(buffer);

        // Hash the file.

        hash = CstHashFile(CstArgument1->Buffer, &hashSize);

        // Sign the hash.

        if (!NT_SUCCESS(status = BCryptSignHash(keyHandle, NULL, hash, hashSize, NULL, 0, &signatureSize, 0)))
            CstFailWithStatus(L"Unable to get the signature size", status, 0);
        signature = PhAllocate(signatureSize);
        if (!NT_SUCCESS(status = BCryptSignHash(keyHandle, NULL, hash, hashSize, signature, signatureSize, &signatureSize, 0)))
            CstFailWithStatus(L"Unable to create the signature", status, 0);

        PhFree(hash);
        BCryptDestroyKey(keyHandle);
        BCryptCloseAlgorithmProvider(signAlgHandle, 0);

        if (CstHex)
        {
            // Output the signature as a hex string.

            string = PhBufferToHexString(signature, signatureSize);
            wprintf(L"%.*s", (ULONG)(string->Length / sizeof(WCHAR)), string->Buffer);
            PhDereferenceObject(string);
        }
        else
        {
            // Write the signature to the output file.

            if (!NT_SUCCESS(status = PhCreateFileWin32Ex(&fileHandle, CstSigFileName->Buffer, FILE_GENERIC_WRITE, &(LARGE_INTEGER){.QuadPart = signatureSize}, FILE_ATTRIBUTE_NORMAL, 0, FILE_OVERWRITE_IF, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, NULL)))
                CstFailWithStatus(PhFormatString(L"Unable to create '%s'", CstSigFileName->Buffer)->Buffer, status, 0);
            if (!NT_SUCCESS(status = NtWriteFile(fileHandle, NULL, NULL, NULL, &iosb, signature, signatureSize, NULL, NULL)))
                CstFailWithStatus(PhFormatString(L"Unable to write signature to '%s'", CstSigFileName->Buffer)->Buffer, status, 0);
            NtClose(fileHandle);
        }

        PhFree(signature);
    }
    else if (PhEqualString2(CstCommand, L"verify", TRUE))
    {
        BCRYPT_ALG_HANDLE signAlgHandle;
        ULONG bufferSize;
        PVOID buffer;
        BCRYPT_KEY_HANDLE keyHandle;
        ULONG hashSize;
        PVOID hash;
        ULONG signatureSize;
        PVOID signature;

        if (!CstArgument1 || !CstKeyFileName || !CstSigFileName)
            CstFailWith(CstHelpMessage);

        if (!CstArgument1 || !CstKeyFileName || !CstSigFileName)
            CstFailWith(CstHelpMessage);

        // Import the key.

        if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&signAlgHandle, CST_SIGN_ALGORITHM, NULL, 0)))
            CstFailWithStatus(L"Unable to open the signing algorithm provider", status, 0);
        buffer = CstReadFile(CstKeyFileName->Buffer, 1024 * 1024, &bufferSize);
        if (!NT_SUCCESS(status = BCryptImportKeyPair(signAlgHandle, NULL, CST_BLOB_PUBLIC, &keyHandle, buffer, bufferSize, 0)))
            CstFailWithStatus(PhFormatString(L"Unable to import the public key: %s", CstKeyFileName->Buffer)->Buffer, status, 0);
        PhFree(buffer);

        // Read the signature.

        signature = CstReadFile(CstSigFileName->Buffer, 1024 * 1024, &signatureSize);

        // Hash the file.

        hash = CstHashFile(CstArgument1->Buffer, &hashSize);

        // Verify the hash.

        if (!NT_SUCCESS(status = BCryptVerifySignature(keyHandle, NULL, hash, hashSize, signature, signatureSize, 0)))
            CstFailWithStatus(PhFormatString(L"Signature verification failed: %s", CstKeyFileName->Buffer)->Buffer, status, 0);

        PhFree(signature);
        PhFree(hash);

        wprintf(L"The signature is valid.\n");
    }

    return 0;
}
