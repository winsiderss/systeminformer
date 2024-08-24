/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2016-2023
 *     jxy-s   2024
 *
 */

#include <ph.h>
#include <bcrypt.h>

#define CST_HASH_ALG_HANDLE      BCRYPT_SHA512_ALG_HANDLE
#define CST_HASH_ALGORITHM       BCRYPT_SHA512_ALGORITHM
#define CST_HASH_ALGORITHM_BITS  512
#define CST_HASH_ALGORITHM_BYTES (CST_HASH_ALGORITHM_BITS / 8)
#define CST_SIGN_ALG_HANDLE      BCRYPT_RSA_ALG_HANDLE
#define CST_SIGN_ALGORITHM       BCRYPT_RSA_ALGORITHM
#define CST_SIGN_ALGORITHM_BITS  4096
#define CST_BLOB_PRIVATE         BCRYPT_RSAPRIVATE_BLOB
#define CST_BLOB_PUBLIC          BCRYPT_RSAPUBLIC_BLOB
#define CST_PADDING_FLAGS        BCRYPT_PAD_PSS
#define CST_PADDING_INFO         &CstPaddingInfo

static BCRYPT_PSS_PADDING_INFO CstPaddingInfo =
{
    CST_HASH_ALGORITHM,
    CST_HASH_ALGORITHM_BYTES
};

#define FILE_BUFFER_SIZE PAGE_SIZE

#define ARG_KEY 1
#define ARG_SIG 2
#define ARG_HEX 3

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
static VOID CstpFailWith(
    _In_ _Printf_format_string_ PWSTR Format,
    ...
    )
{
    va_list args;
    va_start(args, Format);
    vfwprintf(stderr, Format, args);
    va_end(args);
    exit(1);
}

#define CstFailWith(Format, ...) CstpFailWith(Format L"\n", __VA_ARGS__)

DECLSPEC_NORETURN
static VOID CstpFailWithStatus(
    _In_ _Printf_format_string_ PWSTR Format,
    ...
    )
{
    va_list args;
    va_start(args, Format);
    vfwprintf(stderr, Format, args);
    va_end(args);
    exit(1);
}

#define CstFailWithStatus(Status, Format, ...)                                 \
    CstpFailWithStatus(                                                        \
        Format L": %ls\n",                                                     \
        __VA_ARGS__,                                                           \
        PhGetStatusMessage(Status, 0)->Buffer                                  \
        )

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

    if (!NT_SUCCESS(status = BCryptExportKey(
        KeyHandle,
        NULL,
        BlobType,
        NULL,
        0,
        &blobSize,
        0)))
    {
        CstFailWithStatus(status, L"failed to export %ls, unable to get blob size", Description);
    }

    blob = PhAllocate(blobSize);

    if (!NT_SUCCESS(status = BCryptExportKey(
        KeyHandle,
        NULL,
        BlobType,
        blob,
        blobSize,
        &blobSize,
        0)))
    {
        CstFailWithStatus(status, L"failed to export %ls, unable to get blob data", Description);
    }

    if (!NT_SUCCESS(status = PhCreateFileWin32Ex(
        &fileHandle,
        FileName,
        FILE_GENERIC_WRITE,
        &(LARGE_INTEGER){.QuadPart = blobSize},
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_OVERWRITE_IF,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
        NULL)))
    {
        CstFailWithStatus(status, L"failed to create \"%ls\"", FileName);
    }

    if (!NT_SUCCESS(status = NtWriteFile(
        fileHandle,
        NULL,
        NULL, NULL,
        &iosb,
        blob,
        blobSize,
        NULL,
        NULL)))
    {
        CstFailWithStatus(status, L"failed to write blob to \"%ls\"", FileName);
    }

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

    if (!NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
        )))
    {
        CstFailWithStatus(status, L"failed to open \"%ls\"", FileName);

    }

    if (!NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize)))
    {
        CstFailWithStatus(status, L"failed to get the size of \"%ls\"", FileName);
    }

    if (fileSize.QuadPart > FileSizeLimit)
    {
        CstFailWith(L"file \"%ls\" is too large", FileName);
    }

    buffer = PhAllocate((ULONG)fileSize.QuadPart);

    if (!NT_SUCCESS(status = NtReadFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &iosb,
        buffer,
        (ULONG)fileSize.QuadPart,
        NULL,
        NULL
        )))
    {
        CstFailWithStatus(status, L"failed to read \"%ls\"", FileName);
    }

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
    ULONG querySize;
    ULONG hashObjectSize;
    ULONG hashSize;
    PVOID hashObject;
    BCRYPT_HASH_HANDLE hashHandle;
    PVOID hash;

    if (!NT_SUCCESS(status = BCryptGetProperty(
        CST_HASH_ALG_HANDLE,
        BCRYPT_OBJECT_LENGTH,
        (PUCHAR)&hashObjectSize,
        sizeof(ULONG),
        &querySize,
        0
        )))
    {
        CstFailWithStatus(status, L"%ls", L"failed to query hash object size");
    }

    if (!NT_SUCCESS(status = BCryptGetProperty(
        CST_HASH_ALG_HANDLE,
        BCRYPT_HASH_LENGTH,
        (PUCHAR)&hashSize,
        sizeof(ULONG),
        &querySize,
        0
        )))
    {
        CstFailWithStatus(status, L"%ls", L"failed to query hash size");
    }

    hashObject = PhAllocate(hashObjectSize);

    if (!NT_SUCCESS(status = BCryptCreateHash(
        CST_HASH_ALG_HANDLE,
        &hashHandle,
        hashObject,
        hashObjectSize,
        NULL,
        0,
        0)))
    {
        CstFailWithStatus(status, L"%ls", L"failed to get hash handle");
    }

    if (!NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
        )))
    {
        CstFailWithStatus(status, L"failed to open \"%ls\"", FileName);
    }

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

                CstFailWithStatus(status, L"failed to read \"%ls\"", FileName);
            }

            if (!NT_SUCCESS(status = BCryptHashData(
                hashHandle,
                buffer,
                (ULONG)iosb.Information,
                0
                )))
            {
                CstFailWithStatus(status, L"failed to hash file \"%ls\"", FileName);
            }
        }

        PhFreePage(buffer);
    }

    NtClose(fileHandle);

    hash = PhAllocate(hashSize);

    if (!NT_SUCCESS(status = BCryptFinishHash(hashHandle, hash, hashSize, 0)))
        CstFailWithStatus(status, L"%ls", L"failed to complete the hash");

    BCryptDestroyHash(hashHandle);
    PhFree(hashObject);

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

    if (!NT_SUCCESS(PhInitializePhLib(L"CustomSignTool", (PVOID)&__ImageBase)))
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
        CstFailWith(L"%ls", CstHelpMessage);

    if (PhEqualString2(CstCommand, L"createkeypair", TRUE))
    {
        BCRYPT_KEY_HANDLE keyHandle;

        if (!CstArgument1 || !CstArgument2)
            CstFailWith(L"%ls", CstHelpMessage);

        if (!NT_SUCCESS(status = BCryptGenerateKeyPair(
            CST_SIGN_ALG_HANDLE,
            &keyHandle,
            CST_SIGN_ALGORITHM_BITS,
            0
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to create the key pair");
        }

        if (!NT_SUCCESS(status = BCryptFinalizeKeyPair(keyHandle, 0)))
        {
            CstFailWithStatus(status, L"%ls", L"failed to finalize the key pair");
        }

        CstExportKey(keyHandle, CST_BLOB_PRIVATE, CstArgument1->Buffer, L"private key");
        CstExportKey(keyHandle, CST_BLOB_PUBLIC, CstArgument2->Buffer, L"public key");

        BCryptDestroyKey(keyHandle);
    }
    else if (PhEqualString2(CstCommand, L"sign", TRUE))
    {
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
            CstFailWith(L"%ls", CstHelpMessage);

        buffer = CstReadFile(CstKeyFileName->Buffer, 1024 * 1024, &bufferSize);

        if (!NT_SUCCESS(status = BCryptImportKeyPair(
            CST_SIGN_ALG_HANDLE,
            NULL,
            CST_BLOB_PRIVATE,
            &keyHandle,
            buffer,
            bufferSize,
            0
            )))
        {
            CstFailWithStatus(status, L"failed to import the private key: %ls", CstKeyFileName->Buffer);
        }

        PhFree(buffer);

        hash = CstHashFile(CstArgument1->Buffer, &hashSize);

        if (!NT_SUCCESS(status = BCryptSignHash(
            keyHandle,
            CST_PADDING_INFO,
            hash,
            hashSize,
            NULL,
            0,
            &signatureSize,
            CST_PADDING_FLAGS
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to get the signature size");
        }

        signature = PhAllocate(signatureSize);

        if (!NT_SUCCESS(status = BCryptSignHash(
            keyHandle,
            CST_PADDING_INFO,
            hash,
            hashSize,
            signature,
            signatureSize,
            &signatureSize,
            CST_PADDING_FLAGS
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to create the signature");
        }

        PhFree(hash);
        BCryptDestroyKey(keyHandle);

        if (CstHex)
        {
            string = PhBufferToHexString(signature, signatureSize);
            wprintf(L"%.*ls", (ULONG)(string->Length / sizeof(WCHAR)), string->Buffer);
            PhDereferenceObject(string);
        }
        else
        {
            if (!NT_SUCCESS(status = PhCreateFileWin32Ex(
                &fileHandle,
                CstSigFileName->Buffer,
                FILE_GENERIC_WRITE,
                &(LARGE_INTEGER){.QuadPart = signatureSize},
                FILE_ATTRIBUTE_NORMAL,
                0,
                FILE_OVERWRITE_IF,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
                NULL
                )))
            {
                CstFailWithStatus(status, L"failed to create \"%ls\"", CstSigFileName->Buffer);
            }

            if (!NT_SUCCESS(status = NtWriteFile(
                fileHandle,
                NULL,
                NULL,
                NULL,
                &iosb,
                signature,
                signatureSize,
                NULL,
                NULL
                )))
            {
                CstFailWithStatus(status, L"failed to write signature to \"%ls\"", CstSigFileName->Buffer);
            }

            NtClose(fileHandle);
        }

        PhFree(signature);
    }
    else if (PhEqualString2(CstCommand, L"verify", TRUE))
    {
        ULONG bufferSize;
        PVOID buffer;
        BCRYPT_KEY_HANDLE keyHandle;
        ULONG hashSize;
        PVOID hash;
        ULONG signatureSize;
        PVOID signature;

        if (!CstArgument1 || !CstKeyFileName || !CstSigFileName)
            CstFailWith(L"%ls", CstHelpMessage);

        if (!CstArgument1 || !CstKeyFileName || !CstSigFileName)
            CstFailWith(L"%ls", CstHelpMessage);

        buffer = CstReadFile(CstKeyFileName->Buffer, 1024 * 1024, &bufferSize);

        if (!NT_SUCCESS(status = BCryptImportKeyPair(
            CST_SIGN_ALG_HANDLE,
            NULL,
            CST_BLOB_PUBLIC,
            &keyHandle,
            buffer,
            bufferSize,
            0
            )))
        {
            CstFailWithStatus(status, L"failed to import the public key: %ls", CstKeyFileName->Buffer);
        }

        PhFree(buffer);

        signature = CstReadFile(CstSigFileName->Buffer, 1024 * 1024, &signatureSize);

        hash = CstHashFile(CstArgument1->Buffer, &hashSize);

        if (!NT_SUCCESS(status = BCryptVerifySignature(
            keyHandle,
            CST_PADDING_INFO,
            hash,
            hashSize,
            signature,
            signatureSize,
            CST_PADDING_FLAGS
            )))
        {
            CstFailWithStatus(status, L"%ls", L"signature verification failed");
        }

        PhFree(signature);
        PhFree(hash);

        wprintf(L"signature is valid\n");
    }

    return 0;
}
