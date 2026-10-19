/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2016-2026
 *     jxy-s   2024
 *
 */

#include <ph.h>
#include <phcrypt.h>
#include <base64.h>
#include <bcrypt.h>

#define CST_HASH_ALGORITHM       BCRYPT_SHA512_ALGORITHM
#define CST_HASH_ALGORITHM_BYTES (512 / 8)
#define CST_CRYPT_DERIVED_BYTES  48
#define CST_CRYPT_AES_KEY_BYTES  32
#define CST_CRYPT_AES_IV_BYTES   16
#define CST_SIGN_ALGORITHM       BCRYPT_RSA_ALGORITHM
#define CST_SIGN_ALGORITHM_BITS  4096
#define CST_BLOB_PRIVATE         BCRYPT_RSAPRIVATE_BLOB
#define CST_BLOB_PUBLIC          BCRYPT_RSAPUBLIC_BLOB
#define CST_PADDING_FLAGS        BCRYPT_PAD_PSS
#define CST_PADDING_INFO         &CstPaddingInfo
#define CST_SIGNATURE_RSA_LENGTH     512
#define CST_SIGNATURE_MLDSA_LENGTH   4627
#define CST_SIGNATURE_LENGTH         5139
#define CST_KEY_RSA_PUBLIC_LENGTH    539
#define CST_KEY_MLDSA_PUBLIC_LENGTH  2592
#define CST_KEY_PUBLIC_LENGTH        3131
#define CST_KEY_RSA_PRIVATE_LENGTH   1051
#define CST_KEY_MLDSA_PRIVATE_LENGTH 4896
#define CST_KEY_PRIVATE_LENGTH       5947

static BCRYPT_PSS_PADDING_INFO CstPaddingInfo =
{
    CST_HASH_ALGORITHM,
    CST_HASH_ALGORITHM_BYTES
};

typedef enum _CST_PROVIDER {
    CstProviderSymCrypt,
    CstProviderBCrypt
} CST_PROVIDER;

CST_PROVIDER CstProvider = CstProviderSymCrypt;
BCRYPT_ALG_HANDLE CstBCryptAlgHandle = NULL;

#define FILE_BUFFER_SIZE PAGE_SIZE

#define ARG_KEY 1
#define ARG_SIG 2
#define ARG_HEX 3
#define ARG_BCRYPT 4
#define ARG_SYMCRYPT 5

PPH_STRING CstCommand = NULL;
PPH_STRING CstArgument1 = NULL;
PPH_STRING CstArgument2 = NULL;
PPH_STRING CstArgument3 = NULL;
PPH_STRING CstArgument4 = NULL;
PPH_STRING CstArgument5 = NULL;
PPH_STRING CstKeyFileName = NULL;
PPH_STRING CstSigFileName = NULL;
BOOLEAN CstHex = FALSE;

PWSTR CstHelpMessage =
    L"Usage: CustomSignTool.exe command ...\n"
    L"Commands:\n"
    L"createkeypair\tprivatekeyfile publickeyfile\n"
    L"\t\t(Key files are hybrid containers: RSA-4096 followed by ML-DSA-87.\n"
    L"\t\t Prints the public key container as a C array.)\n"
    L"encrypt\t\tinputfile outputfile secret base64salt iterations\n"
    L"decrypt\t\tinputfile outputfile secret base64salt iterations\n"
    L"sign\t\t-k privatekeyfile [-s outputsigfile] [-h] [-bcrypt|-symcrypt] inputfile\n"
    L"\t\t(privatekeyfile is a hybrid container; the signature is RSA then ML-DSA,\n"
    L"\t\t -bcrypt|-symcrypt selects the RSA provider only)\n"
    L"verify\t\t-k publickeyfile -s inputsigfile [-bcrypt|-symcrypt] inputfile\n"
    L"\t\t(publickeyfile is a hybrid container; both signatures must verify)\n"
    L"testrun\t\t(No arguments needed. Generates and cross-verifies using both providers)\n"
    ;

_Function_class_(PH_COMMAND_LINE_CALLBACK)
static BOOLEAN NTAPI CstCommandLineCallback(
    _In_opt_ PCPH_COMMAND_LINE_OPTION Option,
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
        case ARG_BCRYPT:
            CstProvider = CstProviderBCrypt;
            break;
        case ARG_SYMCRYPT:
            CstProvider = CstProviderSymCrypt;
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
        else if (!CstArgument3)
            PhSwapReference(&CstArgument3, Value);
        else if (!CstArgument4)
            PhSwapReference(&CstArgument4, Value);
        else if (!CstArgument5)
            PhSwapReference(&CstArgument5, Value);
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
    _exit(EXIT_FAILURE);
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
    _exit(EXIT_FAILURE);
}

#define CstFailWithStatus(Status, Format, ...) \
    CstpFailWithStatus( \
        Format L": %ls\n", \
        __VA_ARGS__, \
        PhGetStatusMessage(Status, 0)->Buffer \
        )

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

static VOID CstWriteFile(
    _In_ PWSTR FileName,
    _In_reads_bytes_(BufferSize) PCVOID Buffer,
    _In_ ULONG BufferSize
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER allocationSize;

    allocationSize.QuadPart = BufferSize;

    if (!NT_SUCCESS(status = PhCreateFileWin32Ex(
        &fileHandle,
        FileName,
        FILE_GENERIC_WRITE,
        &allocationSize,
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
        NULL,
        NULL,
        &iosb,
        (PVOID)Buffer,
        BufferSize,
        NULL,
        NULL)))
    {
        CstFailWithStatus(status, L"failed to write \"%ls\"", FileName);
    }

    NtClose(fileHandle);
}

static PVOID CstDecodeBase64String(
    _In_ PPH_STRING String,
    _Out_ PSIZE_T DecodedLength
    )
{
    PPH_BYTES utf8;
    PVOID decoded;
    SIZE_T decodedLength;
    SIZE_T decodedCapacity;

    utf8 = PhConvertStringToUtf8(String);
    decodedCapacity = utf8->Length ? utf8->Length : 1;
    decoded = PhAllocate(decodedCapacity);

    if (!PhBase64Decode(utf8->Buffer, utf8->Length, decoded, decodedCapacity, &decodedLength))
    {
        PhDereferenceObject(utf8);
        PhFree(decoded);
        CstFailWith(L"%ls", L"invalid base64 salt");
    }

    PhDereferenceObject(utf8);

    *DecodedLength = decodedLength;
    return decoded;
}

static ULONG64 CstParseIterations(
    _In_ PPH_STRING String
    )
{
    ULONG64 iterations;

    if (!PhStringToUInt64(&String->sr, 10, &iterations) || iterations == 0 || iterations > 0x7fffffffull)
        CstFailWith(L"%ls", L"invalid iterations value");

    return iterations;
}

static VOID CstDeriveCustomBuildToolKey(
    _In_ PPH_STRING Secret,
    _In_ PPH_STRING Salt,
    _In_ PPH_STRING Iterations,
    _Out_writes_bytes_(CST_CRYPT_DERIVED_BYTES) PBYTE KeyMaterial
    )
{
    NTSTATUS status;
    PPH_BYTES secretUtf8;
    PVOID saltBytes;
    SIZE_T saltLength;
    ULONG64 iterationCount;

    secretUtf8 = PhConvertStringToUtf8(Secret);
    saltBytes = CstDecodeBase64String(Salt, &saltLength);
    iterationCount = CstParseIterations(Iterations);

    status = PhSymCryptPbkdf2(
        BCRYPT_SHA512_ALGORITHM,
        secretUtf8->Buffer,
        secretUtf8->Length,
        saltBytes,
        saltLength,
        iterationCount,
        KeyMaterial,
        CST_CRYPT_DERIVED_BYTES
        );

    PhDereferenceObject(secretUtf8);
    RtlSecureZeroMemory(saltBytes, saltLength);
    PhFree(saltBytes);

    if (!NT_SUCCESS(status))
        CstFailWithStatus(status, L"%ls", L"failed to derive encryption key");
}

static VOID CstCryptFile(
    _In_ BOOLEAN Encrypt,
    _In_ PWSTR InputFileName,
    _In_ PWSTR OutputFileName,
    _In_ PPH_STRING Secret,
    _In_ PPH_STRING Salt,
    _In_ PPH_STRING Iterations
    )
{
    NTSTATUS status;
    PVOID input;
    PVOID output;
    ULONG inputSize;
    ULONG outputSize;
    SIZE_T outputCapacity;
    SIZE_T resultSize;
    BYTE keyMaterial[CST_CRYPT_DERIVED_BYTES];

    input = CstReadFile(InputFileName, MAXULONG - CST_CRYPT_AES_IV_BYTES, &inputSize);
    CstDeriveCustomBuildToolKey(Secret, Salt, Iterations, keyMaterial);

    if (Encrypt)
    {
        outputCapacity = inputSize + CST_CRYPT_AES_IV_BYTES;
        output = PhAllocate(outputCapacity);

        status = PhSymCryptAesCbcEncryptPkcs7(
            keyMaterial,
            CST_CRYPT_AES_KEY_BYTES,
            keyMaterial + CST_CRYPT_AES_KEY_BYTES,
            input,
            inputSize,
            output,
            outputCapacity,
            &resultSize
            );
    }
    else
    {
        outputCapacity = inputSize ? inputSize : 1;
        output = PhAllocate(outputCapacity);

        status = PhSymCryptAesCbcDecryptPkcs7(
            keyMaterial,
            CST_CRYPT_AES_KEY_BYTES,
            keyMaterial + CST_CRYPT_AES_KEY_BYTES,
            input,
            inputSize,
            output,
            &resultSize
            );
    }

    RtlSecureZeroMemory(keyMaterial, sizeof(keyMaterial));
    RtlSecureZeroMemory(input, inputSize);
    PhFree(input);

    if (!NT_SUCCESS(status))
    {
        RtlSecureZeroMemory(output, outputCapacity);
        PhFree(output);
        CstFailWithStatus(status, L"failed to %ls \"%ls\"", Encrypt ? L"encrypt" : L"decrypt", InputFileName);
    }

    if (resultSize > MAXULONG)
    {
        RtlSecureZeroMemory(output, outputCapacity);
        PhFree(output);
        CstFailWith(L"%ls", L"output is too large");
    }

    outputSize = (ULONG)resultSize;
    CstWriteFile(OutputFileName, output, outputSize);

    RtlSecureZeroMemory(output, outputCapacity);
    PhFree(output);
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
    PH_SYMCRYPT_HASH_CONTEXT hashContext;
    PVOID hash;

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

    PhSymCryptHashInit(PH_SYMCRYPT_SHA512_ALGORITHM, &hashContext);

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

            if (!NT_SUCCESS(status = PhSymCryptHashData(&hashContext, buffer, (SIZE_T)iosb.Information)))
                CstFailWithStatus(status, L"%ls", L"failed to hash the input");
        }

        PhFreePage(buffer);
    }

    NtClose(fileHandle);

    hash = PhAllocate(64); // SHA512 size

    if (!NT_SUCCESS(status = PhSymCryptHashFinal(&hashContext, hash, 64)))
        CstFailWithStatus(status, L"%ls", L"failed to finalize the hash");

    *HashSize = 64;

    return hash;
}

NTSTATUS CstInitializeBCrypt(
    VOID
    )
{
    if (!CstBCryptAlgHandle)
    {
        return BCryptOpenAlgorithmProvider(&CstBCryptAlgHandle, CST_SIGN_ALGORITHM, NULL, 0);
    }
    return STATUS_SUCCESS;
}

NTSTATUS CstGenerateKeyPair(
    _In_ CST_PROVIDER Provider,
    _Out_ PVOID *KeyHandle
    )
{
    if (Provider == CstProviderSymCrypt)
    {
        return PhSymCryptGenerateKeyPair(CST_SIGN_ALGORITHM, KeyHandle, CST_SIGN_ALGORITHM_BITS);
    }
    else
    {
        NTSTATUS status = CstInitializeBCrypt();
        if (!NT_SUCCESS(status)) return status;
        return BCryptGenerateKeyPair(CstBCryptAlgHandle, KeyHandle, CST_SIGN_ALGORITHM_BITS, 0);
    }
}

NTSTATUS CstFinalizeKeyPair(
    _In_ CST_PROVIDER Provider,
    _In_ PVOID KeyHandle
    )
{
    if (Provider == CstProviderSymCrypt)
    {
        return PhSymCryptFinalizeKeyPair(KeyHandle);
    }
    else
    {
        return BCryptFinalizeKeyPair(KeyHandle, 0);
    }
}

// SymCrypt exports the public exponent as a fixed 8 bytes; BCrypt uses the
// minimal length (3 bytes for 65537). Strip leading zero bytes to match.
static ULONG CstCanonicalizeRsaBlob(
    _Inout_updates_bytes_(Length) PVOID Blob,
    _In_ ULONG Length
    )
{
    BCRYPT_RSAKEY_BLOB* header = Blob;
    PBYTE exponent;
    ULONG strip;

    if (Length < sizeof(BCRYPT_RSAKEY_BLOB) || Length - sizeof(BCRYPT_RSAKEY_BLOB) < header->cbPublicExp)
        return Length;

    exponent = PTR_ADD_OFFSET(Blob, sizeof(BCRYPT_RSAKEY_BLOB));

    for (strip = 0; strip + 1 < header->cbPublicExp && exponent[strip] == 0; strip++)
        NOTHING;

    if (strip)
    {
        memmove(exponent, exponent + strip, Length - sizeof(BCRYPT_RSAKEY_BLOB) - strip);
        RtlSecureZeroMemory(PTR_ADD_OFFSET(Blob, Length - strip), strip);
        header->cbPublicExp -= strip;
        Length -= strip;
    }

    return Length;
}

static PVOID CstExportKey(
    _In_ CST_PROVIDER Provider,
    _In_ PVOID KeyHandle,
    _In_ PCWSTR BlobType,
    _In_ PCWSTR Name,
    _Out_ PULONG BlobLength
    )
{
    NTSTATUS status;
    ULONG blobSize;
    PVOID blob;

    if (Provider == CstProviderSymCrypt)
        status = PhSymCryptExportKey(KeyHandle, (PWSTR)BlobType, NULL, 0, &blobSize);
    else
        status = BCryptExportKey(KeyHandle, NULL, BlobType, NULL, 0, &blobSize, 0);

    if (!NT_SUCCESS(status))
    {
        CstFailWithStatus(status, L"failed to get %ls size", Name);
    }

    blob = PhAllocate(blobSize);

    if (Provider == CstProviderSymCrypt)
        status = PhSymCryptExportKey(KeyHandle, (PWSTR)BlobType, blob, blobSize, &blobSize);
    else
        status = BCryptExportKey(KeyHandle, NULL, BlobType, blob, blobSize, &blobSize, 0);

    if (!NT_SUCCESS(status))
    {
        CstFailWithStatus(status, L"failed to export %ls", Name);
    }

    *BlobLength = CstCanonicalizeRsaBlob(blob, blobSize);

    return blob;
}

static BOOLEAN CstValidateRsaBlob(
    _In_reads_bytes_(Length) PVOID Blob,
    _In_ ULONG Length,
    _In_ BOOLEAN Private
    )
{
    BCRYPT_RSAKEY_BLOB* header = Blob;

    if (Length < sizeof(BCRYPT_RSAKEY_BLOB))
        return FALSE;

    if (header->Magic != (Private ? BCRYPT_RSAPRIVATE_MAGIC : BCRYPT_RSAPUBLIC_MAGIC))
        return FALSE;
    if (header->BitLength != CST_SIGN_ALGORITHM_BITS)
        return FALSE;
    if (header->cbPublicExp != 3)
        return FALSE;
    if (header->cbModulus != CST_SIGN_ALGORITHM_BITS / 8)
        return FALSE;

    if (Private)
    {
        if (header->cbPrime1 != CST_SIGN_ALGORITHM_BITS / 16)
            return FALSE;
        if (header->cbPrime2 != CST_SIGN_ALGORITHM_BITS / 16)
            return FALSE;
    }

    return TRUE;
}

static VOID CstSplitKeyContainer(
    _In_reads_bytes_(Length) PVOID Container,
    _In_ ULONG Length,
    _In_ BOOLEAN Private,
    _Out_ PVOID* Rsa,
    _Out_ PULONG RsaLength,
    _Out_ PVOID* MlDsa,
    _Out_ PULONG MlDsaLength
    )
{
    ULONG expectedLength;
    ULONG rsaLength;
    ULONG mlDsaLength;

    if (Private)
    {
        expectedLength = CST_KEY_PRIVATE_LENGTH;
        rsaLength = CST_KEY_RSA_PRIVATE_LENGTH;
        mlDsaLength = CST_KEY_MLDSA_PRIVATE_LENGTH;
    }
    else
    {
        expectedLength = CST_KEY_PUBLIC_LENGTH;
        rsaLength = CST_KEY_RSA_PUBLIC_LENGTH;
        mlDsaLength = CST_KEY_MLDSA_PUBLIC_LENGTH;
    }

    if (Length != expectedLength)
    {
        CstFailWith(
            L"invalid %ls key container length: expected %lu bytes, actual %lu bytes",
            Private ? L"private" : L"public",
            expectedLength,
            Length
            );
    }

    if (!CstValidateRsaBlob(Container, rsaLength, Private))
    {
        CstFailWith(
            L"invalid RSA header in the %ls key container (expected RSA-%lu, %lu byte blob)",
            Private ? L"private" : L"public",
            (ULONG)CST_SIGN_ALGORITHM_BITS,
            rsaLength
            );
    }

    *Rsa = Container;
    *RsaLength = rsaLength;
    *MlDsa = PTR_ADD_OFFSET(Container, rsaLength);
    *MlDsaLength = mlDsaLength;
}

static VOID CstPrintCArray(
    _In_reads_bytes_(Length) PBYTE Buffer,
    _In_ ULONG Length
    )
{
    ULONG i;

    for (i = 0; i < Length; i++)
    {
        if ((i % 12) == 0)
            wprintf(L"            ");

        wprintf(L"0x%02X,", Buffer[i]);

        if ((i % 12) == 11 || i + 1 == Length)
            wprintf(L"\n");
        else
            wprintf(L" ");
    }
}

NTSTATUS CstImportKeyPair(
    _In_ CST_PROVIDER Provider,
    _Out_ PVOID *KeyHandle,
    _In_ PWSTR BlobType,
    _In_reads_bytes_(BlobLength) PVOID Blob,
    _In_ ULONG BlobLength
    )
{
    if (Provider == CstProviderSymCrypt)
    {
        return PhSymCryptImportKeyPair(CST_SIGN_ALGORITHM, KeyHandle, BlobType, Blob, BlobLength);
    }
    else
    {
        NTSTATUS status = CstInitializeBCrypt();
        if (!NT_SUCCESS(status)) return status;
        return BCryptImportKeyPair(CstBCryptAlgHandle, NULL, BlobType, KeyHandle, Blob, BlobLength, 0);
    }
}

NTSTATUS CstSignHash(
    _In_ CST_PROVIDER Provider,
    _In_ PVOID KeyHandle,
    _In_opt_ PVOID PaddingInfo,
    _In_reads_bytes_(HashLength) PVOID Hash,
    _In_ ULONG HashLength,
    _Out_writes_bytes_to_opt_(SignatureLength, *ResultLength) PVOID Signature,
    _In_ ULONG SignatureLength,
    _Out_ PULONG ResultLength,
    _In_ ULONG Flags
    )
{
    if (Provider == CstProviderSymCrypt)
    {
        return PhSymCryptSignHash(KeyHandle, PaddingInfo, Hash, HashLength, Signature, SignatureLength, ResultLength, Flags);
    }
    else
    {
        return BCryptSignHash(KeyHandle, PaddingInfo, Hash, HashLength, Signature, SignatureLength, ResultLength, Flags);
    }
}

NTSTATUS CstVerifyHash(
    _In_ CST_PROVIDER Provider,
    _In_ PVOID KeyHandle,
    _In_opt_ PVOID PaddingInfo,
    _In_reads_bytes_(HashLength) PVOID Hash,
    _In_ ULONG HashLength,
    _In_reads_bytes_(SignatureLength) PVOID Signature,
    _In_ ULONG SignatureLength,
    _In_ ULONG Flags
    )
{
    if (Provider == CstProviderSymCrypt)
    {
        return PhSymCryptVerifyHash(KeyHandle, PaddingInfo, Hash, HashLength, Signature, SignatureLength, Flags);
    }
    else
    {
        return BCryptVerifySignature(KeyHandle, PaddingInfo, Hash, HashLength, Signature, SignatureLength, Flags);
    }
}

NTSTATUS CstDestroyKey(
    _In_ CST_PROVIDER Provider,
    _In_ PVOID KeyHandle
    )
{
    if (Provider == CstProviderSymCrypt)
    {
        return PhSymCryptDestroyKey(KeyHandle);
    }
    else
    {
        return BCryptDestroyKey(KeyHandle);
    }
}

int __cdecl wmain(int argc, wchar_t *argv[])
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { ARG_KEY, L"k", MandatoryArgumentType },
        { ARG_SIG, L"s", MandatoryArgumentType },
        { ARG_HEX, L"h", NoArgumentType },
        { ARG_BCRYPT, L"bcrypt", NoArgumentType },
        { ARG_SYMCRYPT, L"symcrypt", NoArgumentType }
    };

    NTSTATUS status;
    PH_STRINGREF commandLine;

    if (!NT_SUCCESS(PhInitializePhLib(L"CustomSignTool")))
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
        PVOID keyHandle;
        PVOID mlDsaHandle;
        PVOID rsaPrivate;
        PVOID rsaPublic;
        PVOID privateContainer;
        PVOID publicContainer;
        ULONG rsaPrivateLength;
        ULONG rsaPublicLength;
        SIZE_T mlDsaLength;

        if (!CstArgument1 || !CstArgument2)
            CstFailWith(L"%ls", CstHelpMessage);

        if (!NT_SUCCESS(status = CstGenerateKeyPair(
            CstProvider,
            &keyHandle
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to create the key pair");
        }

        if (!NT_SUCCESS(status = CstFinalizeKeyPair(CstProvider, keyHandle)))
        {
            CstFailWithStatus(status, L"%ls", L"failed to finalize the key pair");
        }

        rsaPrivate = CstExportKey(CstProvider, keyHandle, CST_BLOB_PRIVATE, L"RSA private key", &rsaPrivateLength);
        rsaPublic = CstExportKey(CstProvider, keyHandle, CST_BLOB_PUBLIC, L"RSA public key", &rsaPublicLength);

        if (rsaPrivateLength != CST_KEY_RSA_PRIVATE_LENGTH)
        {
            CstFailWith(L"invalid RSA private key blob length: expected %lu bytes, actual %lu bytes",
                (ULONG)CST_KEY_RSA_PRIVATE_LENGTH, rsaPrivateLength);
        }
        if (!CstValidateRsaBlob(rsaPrivate, rsaPrivateLength, TRUE))
        {
            CstFailWith(L"%ls", L"invalid RSA private key blob header");
        }
        if (rsaPublicLength != CST_KEY_RSA_PUBLIC_LENGTH)
        {
            CstFailWith(L"invalid RSA public key blob length: expected %lu bytes, actual %lu bytes",
                (ULONG)CST_KEY_RSA_PUBLIC_LENGTH, rsaPublicLength);
        }
        if (!CstValidateRsaBlob(rsaPublic, rsaPublicLength, FALSE))
        {
            CstFailWith(L"%ls", L"invalid RSA public key blob header");
        }

        CstDestroyKey(CstProvider, keyHandle);

        if (!NT_SUCCESS(status = PhSymCryptMlDsaGenerateKey(PH_SYMCRYPT_MLDSA_87, &mlDsaHandle)))
        {
            CstFailWithStatus(status, L"%ls", L"failed to create the ML-DSA key pair");
        }

        privateContainer = PhAllocate(CST_KEY_PRIVATE_LENGTH);
        publicContainer = PhAllocate(CST_KEY_PUBLIC_LENGTH);
        memcpy(privateContainer, rsaPrivate, CST_KEY_RSA_PRIVATE_LENGTH);
        memcpy(publicContainer, rsaPublic, CST_KEY_RSA_PUBLIC_LENGTH);

        if (!NT_SUCCESS(status = PhSymCryptMlDsaExportKey(
            mlDsaHandle,
            PH_SYMCRYPT_MLDSA_PRIVATE_KEY,
            PTR_ADD_OFFSET(privateContainer, CST_KEY_RSA_PRIVATE_LENGTH),
            CST_KEY_MLDSA_PRIVATE_LENGTH,
            &mlDsaLength
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to export the ML-DSA private key");
        }
        if (mlDsaLength != CST_KEY_MLDSA_PRIVATE_LENGTH)
        {
            CstFailWith(L"invalid ML-DSA private key: expected %lu bytes, actual %lu bytes",
                (ULONG)CST_KEY_MLDSA_PRIVATE_LENGTH, (ULONG)mlDsaLength);
        }

        if (!NT_SUCCESS(status = PhSymCryptMlDsaExportKey(
            mlDsaHandle,
            PH_SYMCRYPT_MLDSA_PUBLIC_KEY,
            PTR_ADD_OFFSET(publicContainer, CST_KEY_RSA_PUBLIC_LENGTH),
            CST_KEY_MLDSA_PUBLIC_LENGTH,
            &mlDsaLength
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to export the ML-DSA public key");
        }
        if (mlDsaLength != CST_KEY_MLDSA_PUBLIC_LENGTH)
        {
            CstFailWith(L"invalid ML-DSA public key: expected %lu bytes, actual %lu bytes",
                (ULONG)CST_KEY_MLDSA_PUBLIC_LENGTH, (ULONG)mlDsaLength);
        }

        PhSymCryptMlDsaDestroyKey(mlDsaHandle);

        CstWriteFile(CstArgument1->Buffer, privateContainer, CST_KEY_PRIVATE_LENGTH);
        CstWriteFile(CstArgument2->Buffer, publicContainer, CST_KEY_PUBLIC_LENGTH);

        CstPrintCArray(publicContainer, CST_KEY_PUBLIC_LENGTH);

        RtlSecureZeroMemory(rsaPrivate, rsaPrivateLength);
        RtlSecureZeroMemory(privateContainer, CST_KEY_PRIVATE_LENGTH);
        PhFree(rsaPrivate);
        PhFree(rsaPublic);
        PhFree(privateContainer);
        PhFree(publicContainer);
    }
    else if (PhEqualString2(CstCommand, L"encrypt", TRUE))
    {
        if (!CstArgument1 || !CstArgument2 || !CstArgument3 || !CstArgument4 || !CstArgument5)
            CstFailWith(L"%ls", CstHelpMessage);

        CstCryptFile(
            TRUE,
            CstArgument1->Buffer,
            CstArgument2->Buffer,
            CstArgument3,
            CstArgument4,
            CstArgument5
            );
    }
    else if (PhEqualString2(CstCommand, L"decrypt", TRUE))
    {
        if (!CstArgument1 || !CstArgument2 || !CstArgument3 || !CstArgument4 || !CstArgument5)
            CstFailWith(L"%ls", CstHelpMessage);

        CstCryptFile(
            FALSE,
            CstArgument1->Buffer,
            CstArgument2->Buffer,
            CstArgument3,
            CstArgument4,
            CstArgument5
            );
    }
    else if (PhEqualString2(CstCommand, L"sign", TRUE))
    {
        ULONG bufferSize;
        PVOID buffer;
        ULONG hashSize;
        PVOID hash;
        ULONG rsaSignatureSize;
        PBYTE signature;
        PVOID rsaKey;
        ULONG rsaKeyLength;
        PVOID mlDsaKey;
        ULONG mlDsaKeyLength;
        PVOID keyHandle;
        PVOID mlDsaHandle;
        SIZE_T mlDsaSignatureSize;

        if (!CstKeyFileName)
            CstFailWith(L"%ls", L"no key file specified");
        if (!CstArgument1)
            CstFailWith(L"%ls", L"no input file specified");
        if (!CstSigFileName)
            PhSwapReference(&CstSigFileName, PhConcatStrings2(CstArgument1->Buffer, L".sig"));

        buffer = CstReadFile(CstKeyFileName->Buffer, 64 * 1024, &bufferSize);

        CstSplitKeyContainer(buffer, bufferSize, TRUE, &rsaKey, &rsaKeyLength, &mlDsaKey, &mlDsaKeyLength);

        if (!NT_SUCCESS(status = CstImportKeyPair(
            CstProvider,
            &keyHandle,
            CST_BLOB_PRIVATE,
            rsaKey,
            rsaKeyLength
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to import the RSA private key");
        }

        if (!NT_SUCCESS(status = PhSymCryptMlDsaImportKey(
            PH_SYMCRYPT_MLDSA_87,
            PH_SYMCRYPT_MLDSA_PRIVATE_KEY,
            mlDsaKey,
            mlDsaKeyLength,
            &mlDsaHandle
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to import the ML-DSA private key");
        }

        hash = CstHashFile(CstArgument1->Buffer, &hashSize);

        signature = PhAllocate(CST_SIGNATURE_LENGTH);

        if (!NT_SUCCESS(status = CstSignHash(
            CstProvider,
            keyHandle,
            CST_PADDING_INFO,
            hash,
            hashSize,
            NULL,
            0,
            &rsaSignatureSize,
            CST_PADDING_FLAGS
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to get signature size");
        }

        if (rsaSignatureSize != CST_SIGNATURE_RSA_LENGTH)
        {
            CstFailWith(L"invalid RSA signature size: expected %lu bytes, actual %lu bytes",
                (ULONG)CST_SIGNATURE_RSA_LENGTH, rsaSignatureSize);
        }

        if (!NT_SUCCESS(status = CstSignHash(
            CstProvider,
            keyHandle,
            CST_PADDING_INFO,
            hash,
            hashSize,
            signature,
            CST_SIGNATURE_RSA_LENGTH,
            &rsaSignatureSize,
            CST_PADDING_FLAGS
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to sign hash");
        }

        if (rsaSignatureSize != CST_SIGNATURE_RSA_LENGTH)
        {
            CstFailWith(L"invalid RSA signature size: expected %lu bytes, actual %lu bytes",
                (ULONG)CST_SIGNATURE_RSA_LENGTH, rsaSignatureSize);
        }

        if (!NT_SUCCESS(status = PhSymCryptMlDsaSignPreHash(
            mlDsaHandle,
            PH_SYMCRYPT_SHA512_ALGORITHM,
            hash,
            hashSize,
            NULL,
            0,
            signature + CST_SIGNATURE_RSA_LENGTH,
            CST_SIGNATURE_MLDSA_LENGTH,
            &mlDsaSignatureSize
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to sign hash with ML-DSA");
        }

        if (mlDsaSignatureSize != CST_SIGNATURE_MLDSA_LENGTH)
        {
            CstFailWith(L"invalid ML-DSA signature size: expected %lu bytes, actual %lu bytes",
                (ULONG)CST_SIGNATURE_MLDSA_LENGTH, (ULONG)mlDsaSignatureSize);
        }

        CstWriteFile(CstSigFileName->Buffer, signature, CST_SIGNATURE_LENGTH);

        if (CstHex)
        {
            PPH_STRING hex;

            hex = PhBufferToHexString(signature, CST_SIGNATURE_LENGTH);
            wprintf(L"%ls\n", hex->Buffer);
            PhDereferenceObject(hex);
        }

        CstDestroyKey(CstProvider, keyHandle);
        PhSymCryptMlDsaDestroyKey(mlDsaHandle);
        PhFree(signature);
        PhFree(hash);
        RtlSecureZeroMemory(buffer, bufferSize);
        PhFree(buffer);
    }
    else if (PhEqualString2(CstCommand, L"verify", TRUE))
    {
        ULONG bufferSize;
        PVOID buffer;
        ULONG hashSize;
        PVOID hash;
        ULONG signatureSize;
        PBYTE signature;
        PVOID rsaKey;
        ULONG rsaKeyLength;
        PVOID mlDsaKey;
        ULONG mlDsaKeyLength;
        PVOID keyHandle;
        PVOID mlDsaHandle;

        if (!CstKeyFileName)
            CstFailWith(L"%ls", L"no key file specified");
        if (!CstSigFileName)
            CstFailWith(L"%ls", L"no signature file specified");
        if (!CstArgument1)
            CstFailWith(L"%ls", L"no input file specified");

        buffer = CstReadFile(CstKeyFileName->Buffer, 64 * 1024, &bufferSize);

        CstSplitKeyContainer(buffer, bufferSize, FALSE, &rsaKey, &rsaKeyLength, &mlDsaKey, &mlDsaKeyLength);

        if (!NT_SUCCESS(status = CstImportKeyPair(
            CstProvider,
            &keyHandle,
            CST_BLOB_PUBLIC,
            rsaKey,
            rsaKeyLength
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to import the RSA public key");
        }

        if (!NT_SUCCESS(status = PhSymCryptMlDsaImportKey(
            PH_SYMCRYPT_MLDSA_87,
            PH_SYMCRYPT_MLDSA_PUBLIC_KEY,
            mlDsaKey,
            mlDsaKeyLength,
            &mlDsaHandle
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to import the ML-DSA public key");
        }

        signature = CstReadFile(CstSigFileName->Buffer, 64 * 1024, &signatureSize);

        if (signatureSize != CST_SIGNATURE_LENGTH)
        {
            CstFailWith(L"invalid signature length: expected %lu bytes, actual %lu bytes",
                (ULONG)CST_SIGNATURE_LENGTH, signatureSize);
        }

        hash = CstHashFile(CstArgument1->Buffer, &hashSize);

        if (!NT_SUCCESS(status = CstVerifyHash(
            CstProvider,
            keyHandle,
            CST_PADDING_INFO,
            hash,
            hashSize,
            signature,
            CST_SIGNATURE_RSA_LENGTH,
            CST_PADDING_FLAGS
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to verify RSA signature");
        }

        if (!NT_SUCCESS(status = PhSymCryptMlDsaVerifyPreHash(
            mlDsaHandle,
            PH_SYMCRYPT_SHA512_ALGORITHM,
            hash,
            hashSize,
            NULL,
            0,
            signature + CST_SIGNATURE_RSA_LENGTH,
            CST_SIGNATURE_MLDSA_LENGTH
            )))
        {
            CstFailWithStatus(status, L"%ls", L"failed to verify ML-DSA signature");
        }

        wprintf(L"Signature is valid\n");

        CstDestroyKey(CstProvider, keyHandle);
        PhSymCryptMlDsaDestroyKey(mlDsaHandle);
        PhFree(hash);
        PhFree(signature);
        PhFree(buffer);
    }
    else if (PhEqualString2(CstCommand, L"testrun", TRUE))
    {
        PVOID symKey = NULL;
        PVOID bcKey = NULL;
        ULONG pubSize, privSize;
        ULONG symPubSize, bcPubSize;
        PVOID symPubBlob = NULL, symPrivBlob = NULL;
        PVOID bcPubBlob = NULL, bcPrivBlob = NULL;
        BYTE dummyHash[64] = { 0xAB }; // SHA512 size
        ULONG sigSize;
        PVOID symSignature = NULL;
        PVOID bcSignature = NULL;
        PVOID importSymKeyInBc = NULL;
        PVOID importBcKeyInSym = NULL;

        wprintf(L"--- Starting TestRun ---\n");

        // 1. Generate keys
        wprintf(L"[+] Generating keys with SymCrypt... ");
        if (!NT_SUCCESS(status = CstGenerateKeyPair(CstProviderSymCrypt, &symKey)))
            CstFailWithStatus(status, L"%ls", L"failed");
        if (!NT_SUCCESS(status = CstFinalizeKeyPair(CstProviderSymCrypt, symKey)))
            CstFailWithStatus(status, L"%ls", L"failed");
        wprintf(L"OK\n");

        wprintf(L"[+] Generating keys with BCrypt... ");
        if (!NT_SUCCESS(status = CstGenerateKeyPair(CstProviderBCrypt, &bcKey)))
            CstFailWithStatus(status, L"%ls", L"failed");
        if (!NT_SUCCESS(status = CstFinalizeKeyPair(CstProviderBCrypt, bcKey)))
            CstFailWithStatus(status, L"%ls", L"failed");
        wprintf(L"OK\n");

        // 2. Export keys
        PhSymCryptExportKey(symKey, CST_BLOB_PUBLIC, NULL, 0, &pubSize);
        symPubBlob = PhAllocate(pubSize);
        PhSymCryptExportKey(symKey, CST_BLOB_PUBLIC, symPubBlob, pubSize, &pubSize);
        symPubSize = CstCanonicalizeRsaBlob(symPubBlob, pubSize);

        PhSymCryptExportKey(symKey, CST_BLOB_PRIVATE, NULL, 0, &privSize);
        symPrivBlob = PhAllocate(privSize);
        PhSymCryptExportKey(symKey, CST_BLOB_PRIVATE, symPrivBlob, privSize, &privSize);

        BCryptExportKey(bcKey, NULL, CST_BLOB_PUBLIC, NULL, 0, &pubSize, 0);
        bcPubBlob = PhAllocate(pubSize);
        BCryptExportKey(bcKey, NULL, CST_BLOB_PUBLIC, bcPubBlob, pubSize, &pubSize, 0);
        bcPubSize = pubSize;

        BCryptExportKey(bcKey, NULL, CST_BLOB_PRIVATE, NULL, 0, &privSize, 0);
        bcPrivBlob = PhAllocate(privSize);
        BCryptExportKey(bcKey, NULL, CST_BLOB_PRIVATE, bcPrivBlob, privSize, &privSize, 0);

        // 3. Sign
        wprintf(L"[+] Signing with SymCrypt... ");
        CstSignHash(CstProviderSymCrypt, symKey, CST_PADDING_INFO, dummyHash, 64, NULL, 0, &sigSize, CST_PADDING_FLAGS);
        symSignature = PhAllocate(sigSize);
        if (!NT_SUCCESS(status = CstSignHash(CstProviderSymCrypt, symKey, CST_PADDING_INFO, dummyHash, 64, symSignature, sigSize, &sigSize, CST_PADDING_FLAGS)))
            CstFailWithStatus(status, L"%ls", L"failed");
        wprintf(L"OK\n");

        wprintf(L"[+] Signing with BCrypt... ");
        CstSignHash(CstProviderBCrypt, bcKey, CST_PADDING_INFO, dummyHash, 64, NULL, 0, &sigSize, CST_PADDING_FLAGS);
        bcSignature = PhAllocate(sigSize);
        if (!NT_SUCCESS(status = CstSignHash(CstProviderBCrypt, bcKey, CST_PADDING_INFO, dummyHash, 64, bcSignature, sigSize, &sigSize, CST_PADDING_FLAGS)))
            CstFailWithStatus(status, L"%ls", L"failed");
        wprintf(L"OK\n");

        // 4. Cross-Verify
        wprintf(L"[+] Cross-Verifying (BCrypt signature with SymCrypt public key)... ");
        if (!NT_SUCCESS(status = CstImportKeyPair(CstProviderSymCrypt, &importBcKeyInSym, CST_BLOB_PUBLIC, bcPubBlob, bcPubSize)))
            CstFailWithStatus(status, L"%ls", L"failed to import BCrypt public key into SymCrypt");
        if (!NT_SUCCESS(status = CstVerifyHash(CstProviderSymCrypt, importBcKeyInSym, CST_PADDING_INFO, dummyHash, 64, bcSignature, sigSize, CST_PADDING_FLAGS)))
            CstFailWithStatus(status, L"%ls", L"failed verification");
        wprintf(L"OK\n");

        wprintf(L"[+] Cross-Verifying (SymCrypt signature with BCrypt public key)... ");
        if (!NT_SUCCESS(status = CstImportKeyPair(CstProviderBCrypt, &importSymKeyInBc, CST_BLOB_PUBLIC, symPubBlob, symPubSize)))
            CstFailWithStatus(status, L"%ls", L"failed to import SymCrypt public key into BCrypt");
        if (!NT_SUCCESS(status = CstVerifyHash(CstProviderBCrypt, importSymKeyInBc, CST_PADDING_INFO, dummyHash, 64, symSignature, sigSize, CST_PADDING_FLAGS)))
            CstFailWithStatus(status, L"%ls", L"failed verification");
        wprintf(L"OK\n");

        // 5. ML-DSA-87 (HashML-DSA over the same SHA-512 digest)
        wprintf(L"[+] ML-DSA-87 sign/verify... ");
        {
            PVOID mlDsaKey = NULL;
            PBYTE mlDsaSignature;
            SIZE_T mlDsaSignatureSize;

            if (!NT_SUCCESS(status = PhSymCryptMlDsaGenerateKey(PH_SYMCRYPT_MLDSA_87, &mlDsaKey)))
                CstFailWithStatus(status, L"%ls", L"failed to generate ML-DSA key");

            mlDsaSignature = PhAllocate(CST_SIGNATURE_MLDSA_LENGTH);

            if (!NT_SUCCESS(status = PhSymCryptMlDsaSignPreHash(mlDsaKey, PH_SYMCRYPT_SHA512_ALGORITHM, dummyHash, 64, NULL, 0, mlDsaSignature, CST_SIGNATURE_MLDSA_LENGTH, &mlDsaSignatureSize)))
                CstFailWithStatus(status, L"%ls", L"failed to sign with ML-DSA");
            if (mlDsaSignatureSize != CST_SIGNATURE_MLDSA_LENGTH)
                CstFailWith(L"invalid ML-DSA signature size: expected %lu bytes, actual %lu bytes", (ULONG)CST_SIGNATURE_MLDSA_LENGTH, (ULONG)mlDsaSignatureSize);

            if (!NT_SUCCESS(status = PhSymCryptMlDsaVerifyPreHash(mlDsaKey, PH_SYMCRYPT_SHA512_ALGORITHM, dummyHash, 64, NULL, 0, mlDsaSignature, mlDsaSignatureSize)))
                CstFailWithStatus(status, L"%ls", L"ML-DSA verification failed");

            mlDsaSignature[0] ^= 0x01;

            if (NT_SUCCESS(PhSymCryptMlDsaVerifyPreHash(mlDsaKey, PH_SYMCRYPT_SHA512_ALGORITHM, dummyHash, 64, NULL, 0, mlDsaSignature, mlDsaSignatureSize)))
                CstFailWith(L"%ls", L"ML-DSA accepted a corrupted signature");

            PhSymCryptMlDsaDestroyKey(mlDsaKey);
            PhFree(mlDsaSignature);
        }
        wprintf(L"OK\n");

        wprintf(L"--- TestRun Successful ---\n");

        CstDestroyKey(CstProviderSymCrypt, symKey);
        CstDestroyKey(CstProviderBCrypt, bcKey);
        CstDestroyKey(CstProviderSymCrypt, importBcKeyInSym);
        CstDestroyKey(CstProviderBCrypt, importSymKeyInBc);
        PhFree(symPubBlob); PhFree(symPrivBlob);
        PhFree(bcPubBlob); PhFree(bcPrivBlob);
        PhFree(symSignature); PhFree(bcSignature);
    }

    return 0;
}
