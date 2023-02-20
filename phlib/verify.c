/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2013
 *     dmex    2016-2023
 *
 */

#define PH_ENABLE_VERIFY_CACHE
#include <ph.h>
#include <appresolver.h>
#include <mapldr.h>
#include <verify.h>
#include <verifyp.h>

_CryptCATAdminCalcHashFromFileHandle CryptCATAdminCalcHashFromFileHandle;
_CryptCATAdminCalcHashFromFileHandle2 CryptCATAdminCalcHashFromFileHandle2;
_CryptCATAdminAcquireContext CryptCATAdminAcquireContext;
_CryptCATAdminAcquireContext2 CryptCATAdminAcquireContext2;
_CryptCATAdminEnumCatalogFromHash CryptCATAdminEnumCatalogFromHash;
_CryptCATCatalogInfoFromContext CryptCATCatalogInfoFromContext;
_CryptCATAdminReleaseCatalogContext CryptCATAdminReleaseCatalogContext;
_CryptCATAdminReleaseContext CryptCATAdminReleaseContext;
_WTHelperProvDataFromStateData WTHelperProvDataFromStateData_I;
_WTHelperGetProvSignerFromChain WTHelperGetProvSignerFromChain_I;
_WinVerifyTrust WinVerifyTrust_I;
_CertNameToStr CertNameToStr_I;
_CertDuplicateCertificateContext CertDuplicateCertificateContext_I;
_CertFreeCertificateContext CertFreeCertificateContext_I;

static PH_INITONCE PhpVerifyInitOnce = PH_INITONCE_INIT;
static GUID WinTrustActionGenericVerifyV2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
static GUID DriverActionVerify = DRIVER_ACTION_VERIFY;
#ifdef PH_ENABLE_VERIFY_CACHE
static PPH_HASHTABLE PhpVerifyCacheHashTable = NULL;
static PH_QUEUED_LOCK PhpVerifyCacheLock = PH_QUEUED_LOCK_INIT;
#endif

static VOID PhpVerifyInitialization(
    VOID
    )
{
    PVOID wintrust;
    PVOID crypt32;

    wintrust = PhLoadLibrary(L"wintrust.dll");
    crypt32 = PhLoadLibrary(L"crypt32.dll");

    if (wintrust)
    {
        CryptCATAdminCalcHashFromFileHandle = PhGetDllBaseProcedureAddress(wintrust, "CryptCATAdminCalcHashFromFileHandle", 0);
        CryptCATAdminCalcHashFromFileHandle2 = PhGetDllBaseProcedureAddress(wintrust, "CryptCATAdminCalcHashFromFileHandle2", 0);
        CryptCATAdminAcquireContext = PhGetDllBaseProcedureAddress(wintrust, "CryptCATAdminAcquireContext", 0);
        CryptCATAdminAcquireContext2 = PhGetDllBaseProcedureAddress(wintrust, "CryptCATAdminAcquireContext2", 0);
        CryptCATAdminEnumCatalogFromHash = PhGetDllBaseProcedureAddress(wintrust, "CryptCATAdminEnumCatalogFromHash", 0);
        CryptCATCatalogInfoFromContext = PhGetDllBaseProcedureAddress(wintrust, "CryptCATCatalogInfoFromContext", 0);
        CryptCATAdminReleaseCatalogContext = PhGetDllBaseProcedureAddress(wintrust, "CryptCATAdminReleaseCatalogContext", 0);
        CryptCATAdminReleaseContext = PhGetDllBaseProcedureAddress(wintrust, "CryptCATAdminReleaseContext", 0);
        WTHelperProvDataFromStateData_I = PhGetDllBaseProcedureAddress(wintrust, "WTHelperProvDataFromStateData", 0);
        WTHelperGetProvSignerFromChain_I = PhGetDllBaseProcedureAddress(wintrust, "WTHelperGetProvSignerFromChain", 0);
        WinVerifyTrust_I = PhGetDllBaseProcedureAddress(wintrust, "WinVerifyTrust", 0);
    }

    if (crypt32)
    {
        CertNameToStr_I = PhGetDllBaseProcedureAddress(crypt32, "CertNameToStrW", 0);
        CertDuplicateCertificateContext_I = PhGetDllBaseProcedureAddress(crypt32, "CertDuplicateCertificateContext", 0);
        CertFreeCertificateContext_I = PhGetDllBaseProcedureAddress(crypt32, "CertFreeCertificateContext", 0);
    }

    if (
        CryptCATAdminCalcHashFromFileHandle &&
        CryptCATAdminAcquireContext &&
        CryptCATAdminEnumCatalogFromHash &&
        CryptCATCatalogInfoFromContext &&
        CryptCATAdminReleaseCatalogContext &&
        CryptCATAdminReleaseContext &&
        WTHelperProvDataFromStateData_I &&
        WTHelperGetProvSignerFromChain_I &&
        WinVerifyTrust_I &&
        CertNameToStr_I &&
        CertDuplicateCertificateContext_I &&
        CertFreeCertificateContext_I
        )
    {
        PhpVerifyCacheHashTable = PhCreateHashtable(
            sizeof(PH_VERIFY_CACHE_ENTRY),
            PhpVerifyCacheHashtableEqualFunction,
            PhpVerifyCacheHashtableHashFunction,
            100
            );
    }
}

VERIFY_RESULT PhpStatusToVerifyResult(
    _In_ LONG Status
    )
{
    switch (Status)
    {
    case 0:
        return VrTrusted;
    case TRUST_E_NOSIGNATURE:
        return VrNoSignature;
    case CERT_E_EXPIRED:
        return VrExpired;
    case CERT_E_REVOKED:
        return VrRevoked;
    case TRUST_E_EXPLICIT_DISTRUST:
        return VrDistrust;
    case CRYPT_E_SECURITY_SETTINGS:
        return VrSecuritySettings;
    case TRUST_E_BAD_DIGEST:
        return VrBadSignature;
    default:
        return VrSecuritySettings;
    }
}

_Success_(return)
BOOLEAN PhpGetSignaturesFromStateData(
    _In_ HANDLE StateData,
    _Out_ PCERT_CONTEXT **Signatures,
    _Out_ PULONG NumberOfSignatures
    )
{
    PCRYPT_PROVIDER_DATA provData;
    PCRYPT_PROVIDER_SGNR sgnr;
    PCERT_CONTEXT *signatures;
    ULONG i;
    ULONG numberOfSignatures;
    ULONG index;

    provData = WTHelperProvDataFromStateData_I(StateData);

    if (!provData)
    {
        *Signatures = NULL;
        *NumberOfSignatures = 0;
        return FALSE;
    }

    i = 0;
    numberOfSignatures = 0;

    while (sgnr = WTHelperGetProvSignerFromChain_I(provData, i, FALSE, 0))
    {
        if (sgnr->csCertChain != 0)
            numberOfSignatures++;

        i++;
    }

    if (numberOfSignatures != 0)
    {
        signatures = PhAllocate(numberOfSignatures * sizeof(PCERT_CONTEXT));
        i = 0;
        index = 0;

        while (sgnr = WTHelperGetProvSignerFromChain_I(provData, i, FALSE, 0))
        {
            if (sgnr->csCertChain != 0)
                signatures[index++] = (PCERT_CONTEXT)CertDuplicateCertificateContext_I(sgnr->pasCertChain[0].pCert);

            i++;
        }
    }
    else
    {
        signatures = NULL;
    }

    *Signatures = signatures;
    *NumberOfSignatures = numberOfSignatures;

    return TRUE;
}

VOID PhpViewSignerInfo(
    _In_ PPH_VERIFY_FILE_INFO Information,
    _In_ HANDLE StateData
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static _CryptUIDlgViewSignerInfo cryptUIDlgViewSignerInfo;

    if (PhBeginInitOnce(&initOnce))
    {
        HMODULE cryptui;

        if (cryptui = PhLoadLibrary(L"cryptui.dll"))
        {
            cryptUIDlgViewSignerInfo = PhGetDllBaseProcedureAddress(cryptui, "CryptUIDlgViewSignerInfoW", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (cryptUIDlgViewSignerInfo)
    {
        CRYPTUI_VIEWSIGNERINFO_STRUCT viewSignerInfo = { sizeof(CRYPTUI_VIEWSIGNERINFO_STRUCT) };
        PCRYPT_PROVIDER_DATA provData;
        PCRYPT_PROVIDER_SGNR sgnr;

        if (!(provData = WTHelperProvDataFromStateData_I(StateData)))
            return;
        if (!(sgnr = WTHelperGetProvSignerFromChain_I(provData, 0, FALSE, 0)))
            return;

        viewSignerInfo.hwndParent = Information->hWnd;
        viewSignerInfo.pSignerInfo = sgnr->psSigner;
        viewSignerInfo.hMsg = provData->hMsg;
        viewSignerInfo.pszOID = szOID_PKIX_KP_CODE_SIGNING;
        cryptUIDlgViewSignerInfo(&viewSignerInfo);
    }
}

VERIFY_RESULT PhpVerifyFile(
    _In_ PPH_VERIFY_FILE_INFO Information,
    _In_ ULONG UnionChoice,
    _In_ PVOID UnionData,
    _In_ PGUID ActionId,
    _In_opt_ PVOID PolicyCallbackData,
    _Out_ PCERT_CONTEXT **Signatures,
    _Out_ PULONG NumberOfSignatures
    )
{
    LONG status;
    WINTRUST_DATA trustData;

    memset(&trustData, 0, sizeof(WINTRUST_DATA));
    trustData.cbStruct = sizeof(WINTRUST_DATA);
    trustData.pPolicyCallbackData = PolicyCallbackData;
    trustData.dwUIChoice = WTD_UI_NONE;
    trustData.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
    trustData.dwUnionChoice = UnionChoice;
    trustData.dwStateAction = WTD_STATEACTION_VERIFY;
    trustData.dwProvFlags = WTD_SAFER_FLAG;

    trustData.pFile = UnionData;

    if (UnionChoice == WTD_CHOICE_CATALOG)
        trustData.pCatalog = UnionData;

    if (Information->Flags & PH_VERIFY_PREVENT_NETWORK_ACCESS)
    {
        trustData.fdwRevocationChecks = WTD_REVOKE_NONE;
        trustData.dwProvFlags |= WTD_CACHE_ONLY_URL_RETRIEVAL;
    }

    status = WinVerifyTrust_I(INVALID_HANDLE_VALUE, ActionId, &trustData);
    PhpGetSignaturesFromStateData(trustData.hWVTStateData, Signatures, NumberOfSignatures);

    if (status == 0 && (Information->Flags & PH_VERIFY_VIEW_PROPERTIES))
        PhpViewSignerInfo(Information, trustData.hWVTStateData);

    // Close the state data.
    trustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust_I(INVALID_HANDLE_VALUE, ActionId, &trustData);

    return PhpStatusToVerifyResult(status);
}

_Success_(return)
BOOLEAN PhpCalculateFileHash(
    _In_ HANDLE FileHandle,
    _In_opt_ PWSTR HashAlgorithm,
    _Out_ PUCHAR *FileHash,
    _Out_ PULONG FileHashLength,
    _Out_ HANDLE *CatAdminHandle
    )
{
    HANDLE catAdminHandle;
    PUCHAR fileHash;
    ULONG fileHashLength;

    if (CryptCATAdminAcquireContext2)
    {
        if (!CryptCATAdminAcquireContext2(&catAdminHandle, &DriverActionVerify, HashAlgorithm, NULL, 0))
            return FALSE;
    }
    else
    {
        if (!CryptCATAdminAcquireContext(&catAdminHandle, &DriverActionVerify, 0))
            return FALSE;
    }

    fileHashLength = 32;
    fileHash = PhAllocate(fileHashLength);

    if (CryptCATAdminCalcHashFromFileHandle2)
    {
        if (!CryptCATAdminCalcHashFromFileHandle2(catAdminHandle, FileHandle, &fileHashLength, fileHash, 0))
        {
            PhFree(fileHash);
            fileHash = PhAllocate(fileHashLength);

            if (!CryptCATAdminCalcHashFromFileHandle2(catAdminHandle, FileHandle, &fileHashLength, fileHash, 0))
            {
                CryptCATAdminReleaseContext(catAdminHandle, 0);
                PhFree(fileHash);
                return FALSE;
            }
        }
    }
    else
    {
        if (!CryptCATAdminCalcHashFromFileHandle(FileHandle, &fileHashLength, fileHash, 0))
        {
            PhFree(fileHash);
            fileHash = PhAllocate(fileHashLength);

            if (!CryptCATAdminCalcHashFromFileHandle(FileHandle, &fileHashLength, fileHash, 0))
            {
                CryptCATAdminReleaseContext(catAdminHandle, 0);
                PhFree(fileHash);
                return FALSE;
            }
        }
    }

    *FileHash = fileHash;
    *FileHashLength = fileHashLength;
    *CatAdminHandle = catAdminHandle;

    return TRUE;
}

VERIFY_RESULT PhpVerifyFileFromCatalog(
    _In_ PPH_VERIFY_FILE_INFO Information,
    _In_ HANDLE FileHandle,
    _In_opt_ PWSTR HashAlgorithm,
    _Out_ PCERT_CONTEXT **Signatures,
    _Out_ PULONG NumberOfSignatures
    )
{
    VERIFY_RESULT verifyResult = VrNoSignature;
    PCERT_CONTEXT *signatures;
    ULONG numberOfSignatures;
    WINTRUST_CATALOG_INFO catalogInfo;
    LARGE_INTEGER fileSize;
    ULONG fileSizeLimit;
    PUCHAR fileHash;
    ULONG fileHashLength;
    PPH_STRING fileHashTag;
    HANDLE catAdminHandle;
    HANDLE catInfoHandle;
    ULONG i;

    *Signatures = NULL;
    *NumberOfSignatures = 0;

    if (!NT_SUCCESS(PhGetFileSize(FileHandle, &fileSize)))
        return VrNoSignature;

    signatures = NULL;
    numberOfSignatures = 0;

    if (Information->FileSizeLimitForHash != ULONG_MAX)
    {
        fileSizeLimit = PH_VERIFY_DEFAULT_SIZE_LIMIT;

        if (Information->FileSizeLimitForHash != 0)
            fileSizeLimit = Information->FileSizeLimitForHash;

        if (fileSize.QuadPart > fileSizeLimit)
            return VrNoSignature;
    }

    if (PhpCalculateFileHash(FileHandle, HashAlgorithm, &fileHash, &fileHashLength, &catAdminHandle))
    {
        fileHashTag = PhBufferToHexStringEx(fileHash, fileHashLength, TRUE);

        // Search the system catalogs.

        catInfoHandle = CryptCATAdminEnumCatalogFromHash(
            catAdminHandle,
            fileHash,
            fileHashLength,
            0,
            NULL
            );

        if (catInfoHandle)
        {
            CATALOG_INFO ci = { sizeof(CATALOG_INFO) };
            DRIVER_VER_INFO verInfo = { 0 };

            if (CryptCATCatalogInfoFromContext(catInfoHandle, &ci, 0))
            {
                // Disable OS version checking by passing in a DRIVER_VER_INFO structure.
                verInfo.cbStruct = sizeof(DRIVER_VER_INFO);

                memset(&catalogInfo, 0, sizeof(catalogInfo));
                catalogInfo.cbStruct = sizeof(catalogInfo);
                catalogInfo.pcwszCatalogFilePath = ci.wszCatalogFile;
                catalogInfo.pcwszMemberFilePath = NULL; // Information->FileName
                catalogInfo.hMemberFile = FileHandle;
                catalogInfo.pcwszMemberTag = fileHashTag->Buffer;
                catalogInfo.pbCalculatedFileHash = fileHash;
                catalogInfo.cbCalculatedFileHash = fileHashLength;
                catalogInfo.hCatAdmin = catAdminHandle;
                verifyResult = PhpVerifyFile(Information, WTD_CHOICE_CATALOG, &catalogInfo, &DriverActionVerify, &verInfo, &signatures, &numberOfSignatures);

                if (verInfo.pcSignerCertContext)
                    CertFreeCertificateContext_I(verInfo.pcSignerCertContext);
            }

            CryptCATAdminReleaseCatalogContext(catAdminHandle, catInfoHandle, 0);
        }
        else
        {
            // Search any user-supplied catalogs.

            for (i = 0; i < Information->NumberOfCatalogFileNames; i++)
            {
                PhFreeVerifySignatures(signatures, numberOfSignatures);

                memset(&catalogInfo, 0, sizeof(catalogInfo));
                catalogInfo.cbStruct = sizeof(catalogInfo);
                catalogInfo.pcwszCatalogFilePath = Information->CatalogFileNames[i];
                catalogInfo.pcwszMemberFilePath = NULL; // Information->FileName
                catalogInfo.hMemberFile = FileHandle;
                catalogInfo.pcwszMemberTag = fileHashTag->Buffer;
                catalogInfo.pbCalculatedFileHash = fileHash;
                catalogInfo.cbCalculatedFileHash = fileHashLength;
                catalogInfo.hCatAdmin = catAdminHandle;
                verifyResult = PhpVerifyFile(Information, WTD_CHOICE_CATALOG, &catalogInfo, &WinTrustActionGenericVerifyV2, NULL, &signatures, &numberOfSignatures);

                if (verifyResult == VrTrusted)
                    break;
            }
        }

        PhDereferenceObject(fileHashTag);
        PhFree(fileHash);
        CryptCATAdminReleaseContext(catAdminHandle, 0);
    }

    *Signatures = signatures;
    *NumberOfSignatures = numberOfSignatures;

    return verifyResult;
}

NTSTATUS PhVerifyFileEx(
    _In_ PPH_VERIFY_FILE_INFO Information,
    _Out_ VERIFY_RESULT *VerifyResult,
    _Out_opt_ PCERT_CONTEXT **Signatures,
    _Out_opt_ PULONG NumberOfSignatures
    )
{
    VERIFY_RESULT verifyResult;
    PCERT_CONTEXT *signatures;
    ULONG numberOfSignatures;
    WINTRUST_FILE_INFO fileInfo;

    if (PhBeginInitOnce(&PhpVerifyInitOnce))
    {
        PhpVerifyInitialization();
        PhEndInitOnce(&PhpVerifyInitOnce);
    }

    if (!PhpVerifyCacheHashTable)
        return STATUS_NOT_SUPPORTED;

    memset(&fileInfo, 0, sizeof(WINTRUST_FILE_INFO));
    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileInfo.hFile = Information->FileHandle;

    verifyResult = PhpVerifyFile(
        Information,
        WTD_CHOICE_FILE,
        &fileInfo,
        &WinTrustActionGenericVerifyV2,
        NULL,
        &signatures,
        &numberOfSignatures
        );

    if (verifyResult == VrNoSignature)
    {
        if (CryptCATAdminAcquireContext2 && CryptCATAdminCalcHashFromFileHandle2)
        {
            PhFreeVerifySignatures(signatures, numberOfSignatures);

            verifyResult = PhpVerifyFileFromCatalog(
                Information,
                Information->FileHandle,
                BCRYPT_SHA256_ALGORITHM,
                &signatures,
                &numberOfSignatures
                );
        }

        if (verifyResult != VrTrusted)
        {
            PhFreeVerifySignatures(signatures, numberOfSignatures);

            verifyResult = PhpVerifyFileFromCatalog(
                Information,
                Information->FileHandle,
                NULL,
                &signatures,
                &numberOfSignatures
                );
        }
    }

    *VerifyResult = verifyResult;

    if (Signatures)
        *Signatures = signatures;
    else
        PhFreeVerifySignatures(signatures, numberOfSignatures);

    if (NumberOfSignatures)
        *NumberOfSignatures = numberOfSignatures;

    return STATUS_SUCCESS;
}

VOID PhFreeVerifySignatures(
    _In_opt_ PCERT_CONTEXT *Signatures,
    _In_ ULONG NumberOfSignatures
    )
{
    ULONG i;

    if (Signatures)
    {
        for (i = 0; i < NumberOfSignatures; i++)
            CertFreeCertificateContext_I(Signatures[i]);

        PhFree(Signatures);
    }
}

PPH_STRING PhpGetCertNameString(
    _In_ PCERT_NAME_BLOB Blob
    )
{
    PPH_STRING string;
    ULONG bufferSize;

    // CertNameToStr doesn't give us the correct buffer size unless we don't provide a buffer at
    // all.
    bufferSize = CertNameToStr_I(
        X509_ASN_ENCODING,
        Blob,
        CERT_X500_NAME_STR,
        NULL,
        0
        );

    string = PhCreateStringEx(NULL, bufferSize * sizeof(WCHAR));
    CertNameToStr_I(
        X509_ASN_ENCODING,
        Blob,
        CERT_X500_NAME_STR,
        string->Buffer,
        bufferSize
        );

    PhTrimToNullTerminatorString(string);

    return string;
}

PPH_STRING PhpGetX500Value(
    _In_ PPH_STRINGREF String,
    _In_ PPH_STRINGREF KeyName
    )
{
    WCHAR keyNamePlusEqualsBuffer[10];
    PH_STRINGREF keyNamePlusEquals;
    SIZE_T keyNameLength;
    PH_STRINGREF firstPart;
    PH_STRINGREF remainingPart;

    keyNameLength = KeyName->Length / sizeof(WCHAR);
    assert(!(keyNameLength > sizeof(keyNamePlusEquals) / sizeof(WCHAR) - 1));
    keyNamePlusEquals.Buffer = keyNamePlusEqualsBuffer;
    keyNamePlusEquals.Length = (keyNameLength + 1) * sizeof(WCHAR);

    memcpy(keyNamePlusEquals.Buffer, KeyName->Buffer, KeyName->Length);
    keyNamePlusEquals.Buffer[keyNameLength] = L'=';

    // Find "Key=".

    if (!PhSplitStringRefAtString(String, &keyNamePlusEquals, FALSE, &firstPart, &remainingPart))
        return NULL;
    if (remainingPart.Length == 0)
        return NULL;

    // Is the value quoted? If so, return the part inside the quotes.
    if (remainingPart.Buffer[0] == L'"')
    {
        PhSkipStringRef(&remainingPart, sizeof(WCHAR));

        if (!PhSplitStringRefAtChar(&remainingPart, L'"', &firstPart, &remainingPart))
            return NULL;

        return PhCreateString2(&firstPart);
    }
    else
    {
        PhSplitStringRefAtChar(&remainingPart, L',', &firstPart, &remainingPart);

        return PhCreateString2(&firstPart);
    }
}

PPH_STRING PhGetSignerNameFromCertificate(
    _In_ PCERT_CONTEXT Certificate
    )
{
    PCERT_INFO certInfo;
    PH_STRINGREF keyName;
    PPH_STRING name;
    PPH_STRING value;

    // Cert context -> Cert info

    certInfo = Certificate->pCertInfo;

    if (!certInfo)
        return NULL;

    // Cert info subject -> Subject X.500 string

    name = PhpGetCertNameString(&certInfo->Subject);

    // Subject X.500 string -> CN or OU value

    PhInitializeStringRef(&keyName, L"CN");
    value = PhpGetX500Value(&name->sr, &keyName);

    if (!value)
    {
        PhInitializeStringRef(&keyName, L"OU");
        value = PhpGetX500Value(&name->sr, &keyName);
    }

    PhDereferenceObject(name);

    return value;
}

/**
 * Verifies a file's digital signature.
 *
 * \param FileName A file name.
 * \param SignerName A variable which receives a pointer to a string containing the signer name. You
 * must free the string using PhDereferenceObject() when you no longer need it. Note that the signer
 * name may be NULL if it is not valid.
 *
 * \return A VERIFY_RESULT value.
 */
VERIFY_RESULT PhVerifyFile(
    _In_ PWSTR FileName,
    _Out_opt_ PPH_STRING *SignerName
    )
{
    PH_VERIFY_FILE_INFO info = { 0 };
    HANDLE fileHandle;
    VERIFY_RESULT verifyResult;
    PCERT_CONTEXT *signatures;
    ULONG numberOfSignatures;

    if (!NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        if (SignerName)
            *SignerName = NULL;

        return VrNoSignature;
    }

    info.Flags = PH_VERIFY_PREVENT_NETWORK_ACCESS;
    info.FileHandle = fileHandle;

    if (NT_SUCCESS(PhVerifyFileEx(&info, &verifyResult, &signatures, &numberOfSignatures)))
    {
        if (SignerName)
        {
            *SignerName = NULL;

            if (numberOfSignatures != 0)
                *SignerName = PhGetSignerNameFromCertificate(signatures[0]);
        }

        PhFreeVerifySignatures(signatures, numberOfSignatures);

        NtClose(fileHandle);

        return verifyResult;
    }
    else
    {
        if (SignerName)
            *SignerName = NULL;

        NtClose(fileHandle);

        return VrNoSignature;
    }
}

BOOLEAN PhpVerifyCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_VERIFY_CACHE_ENTRY entry1 = Entry1;
    PPH_VERIFY_CACHE_ENTRY entry2 = Entry2;

    return entry1->SequenceNumber == entry2->SequenceNumber && PhEqualString(entry1->FileName, entry2->FileName, FALSE);
}

ULONG PhpVerifyCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_VERIFY_CACHE_ENTRY entry = Entry;

    return PhHashInt64(entry->SequenceNumber) ^ PhHashStringRefEx(&entry->FileName->sr, FALSE, PH_STRING_HASH_X65599);
}

VOID PhFlushVerifyCache(
    VOID
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_VERIFY_CACHE_ENTRY entry;

    if (!PhpVerifyCacheHashTable)
        return;

    PhAcquireQueuedLockExclusive(&PhpVerifyCacheLock);

    PhBeginEnumHashtable(PhpVerifyCacheHashTable, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
    {
        if (entry->FileName)
            PhDereferenceObject(entry->FileName);
        if (entry->VerifySignerName)
            PhDereferenceObject(entry->VerifySignerName);
    }

    PhDereferenceObject(PhpVerifyCacheHashTable);
    PhpVerifyCacheHashTable = PhCreateHashtable(
        sizeof(PH_VERIFY_CACHE_ENTRY),
        PhpVerifyCacheHashtableEqualFunction,
        PhpVerifyCacheHashtableHashFunction,
        100
        );

    PhReleaseQueuedLockExclusive(&PhpVerifyCacheLock);
}

VERIFY_RESULT PhVerifyFileWithAdditionalCatalog(
    _In_ PPH_VERIFY_FILE_INFO Information,
    _In_opt_ PPH_STRING PackageFullName,
    _Out_opt_ PPH_STRING *SignerName
    )
{
    static PH_STRINGREF codeIntegrityFileName = PH_STRINGREF_INIT(L"\\AppxMetadata\\CodeIntegrity.cat");
    VERIFY_RESULT result;
    PPH_STRING additionalCatalogFileName = NULL;
    PCERT_CONTEXT *signatures;
    ULONG numberOfSignatures;

    if (PackageFullName)
    {
        PPH_STRING packagePath;

        if (packagePath = PhGetPackagePath(PackageFullName))
        {
            additionalCatalogFileName = PhConcatStringRef2(&packagePath->sr, &codeIntegrityFileName);
            PhDereferenceObject(packagePath);
        }
    }

    if (additionalCatalogFileName)
    {
        Information->NumberOfCatalogFileNames = 1;
        Information->CatalogFileNames = &additionalCatalogFileName->Buffer;
    }

    if (!NT_SUCCESS(PhVerifyFileEx(Information, &result, &signatures, &numberOfSignatures)))
    {
        result = VrNoSignature;
        signatures = NULL;
        numberOfSignatures = 0;
    }

    if (additionalCatalogFileName)
        PhDereferenceObject(additionalCatalogFileName);

    if (SignerName)
    {
        if (numberOfSignatures != 0)
            *SignerName = PhGetSignerNameFromCertificate(signatures[0]);
        else
            *SignerName = NULL;
    }

    PhFreeVerifySignatures(signatures, numberOfSignatures);

    return result;
}

/**
 * Verifies a file's digital signature, using a cached result if possible.
 *
 * \param FileName A file name.
 * \param PackageFullName An associated package name.
 * \param SignerName A variable which receives a pointer to a string containing the signer name. You
 * must free the string using PhDereferenceObject() when you no longer need it. Note that the signer
 * name may be NULL if it is not valid.
 * \param CachedOnly Specify TRUE to fail the function when no cached result exists.
 *
 * \return A VERIFY_RESULT value.
 */
VERIFY_RESULT PhVerifyFileCached(
    _In_ PPH_STRING FileName,
    _In_opt_ PPH_STRING PackageFullName,
    _Out_opt_ PPH_STRING *SignerName,
    _In_ BOOLEAN NativeFileName,
    _In_ BOOLEAN CachedOnly
    )
{
#ifdef PH_ENABLE_VERIFY_CACHE
    HANDLE fileHandle;
    LONGLONG sequenceNumber = 0;
    PPH_VERIFY_CACHE_ENTRY entry;

    if (PhBeginInitOnce(&PhpVerifyInitOnce))
    {
        PhpVerifyInitialization();
        PhEndInitOnce(&PhpVerifyInitOnce);
    }

    if (!PhpVerifyCacheHashTable)
    {
        if (SignerName) *SignerName = NULL;
        return VrNoSignature;
    }

    if (NativeFileName)
    {
        if (!NT_SUCCESS(PhCreateFile(
            &fileHandle,
            &FileName->sr,
            FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            if (SignerName) *SignerName = NULL;
            return VrNoSignature;
        }
    }
    else
    {
        if (!NT_SUCCESS(PhCreateFileWin32(
            &fileHandle,
            FileName->Buffer,
            FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            if (SignerName) *SignerName = NULL;
            return VrNoSignature;
        }
    }

    {
        PH_VERIFY_CACHE_ENTRY lookupEntry;

        PhGetFileUsn(fileHandle, &sequenceNumber);
        lookupEntry.FileName = FileName;
        lookupEntry.SequenceNumber = sequenceNumber;

        PhAcquireQueuedLockShared(&PhpVerifyCacheLock);
        entry = PhFindEntryHashtable(PhpVerifyCacheHashTable, &lookupEntry);
        PhReleaseQueuedLockShared(&PhpVerifyCacheLock);
    }

    if (entry)
    {
        if (SignerName)
            PhSetReference(SignerName, entry->VerifySignerName);

        NtClose(fileHandle);

        return entry->VerifyResult;
    }
    else
    {
        VERIFY_RESULT result;
        PPH_STRING signerName;

        if (!CachedOnly)
        {
            PH_VERIFY_FILE_INFO info;

            memset(&info, 0, sizeof(PH_VERIFY_FILE_INFO));
            info.Flags = PH_VERIFY_PREVENT_NETWORK_ACCESS;
            info.FileHandle = fileHandle;
            result = PhVerifyFileWithAdditionalCatalog(&info, PackageFullName, &signerName);

            if (result != VrTrusted)
                PhClearReference(&signerName);
        }
        else
        {
            result = VrUnknown;
            signerName = NULL;
        }

        if (result != VrUnknown)
        {
            PH_VERIFY_CACHE_ENTRY newEntry;

            newEntry.FileName = FileName;
            newEntry.SequenceNumber = sequenceNumber;
            newEntry.VerifyResult = result;
            newEntry.VerifySignerName = signerName;

            PhAcquireQueuedLockExclusive(&PhpVerifyCacheLock);

            if (PhAddEntryHashtable(PhpVerifyCacheHashTable, &newEntry))
            {
                // We successfully added the cache entry. Add references.
                PhReferenceObject(FileName);

                if (signerName)
                    PhReferenceObject(signerName);
            }

            PhReleaseQueuedLockExclusive(&PhpVerifyCacheLock);
        }

        if (SignerName)
        {
            *SignerName = signerName;
        }
        else
        {
            if (signerName)
                PhDereferenceObject(signerName);
        }

        NtClose(fileHandle);

        return result;
    }
#else
    VERIFY_RESULT result;
    PPH_STRING signerName;
    PH_VERIFY_FILE_INFO info;

    memset(&info, 0, sizeof(PH_VERIFY_FILE_INFO));
    info.FileName = FileName->Buffer;
    info.Flags = PH_VERIFY_PREVENT_NETWORK_ACCESS;
    result = PhVerifyFileWithAdditionalCatalog(&info, PackageFullName, &signerName);

    if (result != VrTrusted)
        PhClearReference(&signerName);

    if (SignerName)
    {
        *SignerName = signerName;
    }
    else
    {
        if (signerName)
            PhDereferenceObject(signerName);
    }

    return result;
#endif
}

VERIFY_RESULT PhpSignatureStateToVerifyResult(
    _In_ LONG Status
    )
{
    switch (Status)
    {
    case SIGNATURE_STATE_VALID:
    case SIGNATURE_STATE_TRUSTED:
        return VrTrusted;
    case SIGNATURE_STATE_UNSIGNED_MISSING:
        return VrNoSignature; // TRUST_E_NOSIGNATURE
    case SIGNATURE_STATE_UNSIGNED_UNSUPPORTED:
        return VrNoSignature; // TRUST_E_NOSIGNATURE
    case SIGNATURE_STATE_UNSIGNED_POLICY:
        return VrNoSignature; // TRUST_E_NOSIGNATURE
    case SIGNATURE_STATE_INVALID_CORRUPT:
        return VrBadSignature; // TRUST_E_BAD_DIGEST
    case SIGNATURE_STATE_INVALID_POLICY:
        return VrSecuritySettings; // CRYPT_E_BAD_MSG
    case SIGNATURE_STATE_UNTRUSTED:
        return VrDistrust; // TRUST_E_EXPLICIT_DISTRUST
    default:
        return VrSecuritySettings;
    }
}

VERIFY_RESULT PhVerifyFileSignatureInfo(
    _In_ PPH_VERIFY_FILE_INFO Information,
    _In_opt_ PCWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PCERT_CONTEXT** Signatures,
    _Out_ PULONG NumberOfSignatures
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static _WTGetSignatureInfo WTGetSignatureInfo_I = NULL;
    VERIFY_RESULT verifyResult = VrNoSignature;
    SIGNATURE_INFO signatureInfo = { sizeof(SIGNATURE_INFO) };
    PVOID certificateContext = NULL;
    HANDLE verifyTrustStateData = NULL;
    HRESULT status;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID wintrust;

        if (wintrust = PhLoadLibrary(L"wintrust.dll"))
        {
            WTGetSignatureInfo_I = PhGetDllBaseProcedureAddress(wintrust, "WTGetSignatureInfo", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!WTGetSignatureInfo_I)
    {
        *Signatures = NULL;
        *NumberOfSignatures = 0;
        return VrUnknown;
    }

    status = WTGetSignatureInfo_I(
        FileName,
        FileHandle,
        SIF_AUTHENTICODE_SIGNED | SIF_CATALOG_SIGNED | SIF_BASE_VERIFICATION,
        &signatureInfo,
        &certificateContext,
        &verifyTrustStateData
        );

    if (!SUCCEEDED(status))
    {
        *Signatures = NULL;
        *NumberOfSignatures = 0;
        return VrUnknown;
    }

    verifyResult = PhpSignatureStateToVerifyResult(signatureInfo.nSignatureState);
    PhpGetSignaturesFromStateData(verifyTrustStateData, Signatures, NumberOfSignatures);

    if (status == S_OK && verifyTrustStateData && (Information->Flags & PH_VERIFY_VIEW_PROPERTIES))
        PhpViewSignerInfo(Information, verifyTrustStateData);

    if (verifyTrustStateData && WinVerifyTrust_I)
    {
        WINTRUST_DATA verifyTrustData;

        memset(&verifyTrustData, 0, sizeof(WINTRUST_DATA));
        verifyTrustData.cbStruct = sizeof(WINTRUST_DATA);
        verifyTrustData.dwUIChoice = WTD_UI_NONE;
        verifyTrustData.dwUnionChoice = WTD_CHOICE_BLOB;
        verifyTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
        verifyTrustData.hWVTStateData = verifyTrustStateData;

        WinVerifyTrust_I(INVALID_HANDLE_VALUE, &WinTrustActionGenericVerifyV2, &verifyTrustData);
    }

    if (certificateContext && CertFreeCertificateContext_I)
    {
        CertFreeCertificateContext_I(certificateContext);
    }

    return verifyResult;
}
