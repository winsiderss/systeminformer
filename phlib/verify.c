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

#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS
#include <wintrust.h>
#include <softpub.h>

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
//_WTHelperProvDataFromStateData WTHelperProvDataFromStateData_I;
//_WTHelperGetProvSignerFromChain WTHelperGetProvSignerFromChain_I;
_WinVerifyTrust WinVerifyTrust_I;
_CertNameToStr CertNameToStr_I;
_CertGetEnhancedKeyUsage CertGetEnhancedKeyUsage_I;
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
        //WTHelperProvDataFromStateData_I = PhGetDllBaseProcedureAddress(wintrust, "WTHelperProvDataFromStateData", 0);
        //WTHelperGetProvSignerFromChain_I = PhGetDllBaseProcedureAddress(wintrust, "WTHelperGetProvSignerFromChain", 0);
        WinVerifyTrust_I = PhGetDllBaseProcedureAddress(wintrust, "WinVerifyTrust", 0);
    }

    if (crypt32)
    {
        CertNameToStr_I = PhGetDllBaseProcedureAddress(crypt32, "CertNameToStrW", 0);
        CertGetEnhancedKeyUsage_I = PhGetDllBaseProcedureAddress(crypt32, "CertGetEnhancedKeyUsage", 0);
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

// WTHelperProvDataFromStateData (dmex)
PCRYPT_PROVIDER_DATA PhGetCryptProviderDataFromStateData(
    _In_ HANDLE StateData
    )
{
    return (PCRYPT_PROVIDER_DATA)StateData;
}

// WTHelperGetProvSignerFromChain (dmex)
PCRYPT_PROVIDER_SGNR PhGetCryptProviderSignerFromChain(
    _In_ PCRYPT_PROVIDER_DATA ProvData,
    _In_ ULONG SignerIndex,
    _In_ BOOLEAN CounterSigner,
    _In_ ULONG CounterSignerIndex
    )
{
    PCRYPT_PROVIDER_SGNR signer;

    if (!ProvData || SignerIndex >= ProvData->csSigners)
        return NULL;

    signer = &ProvData->pasSigners[SignerIndex];

    if (CounterSigner)
    {
        if (CounterSignerIndex < signer->csCounterSigners)
        {
            return &signer->pasCounterSigners[CounterSignerIndex];
        }
    }

    return signer;
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

    provData = PhGetCryptProviderDataFromStateData(StateData);

    if (!provData)
    {
        *Signatures = NULL;
        *NumberOfSignatures = 0;
        return FALSE;
    }

    i = 0;
    numberOfSignatures = 0;

    while (sgnr = PhGetCryptProviderSignerFromChain(provData, i, FALSE, 0))
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

        while (sgnr = PhGetCryptProviderSignerFromChain(provData, i, FALSE, 0))
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

        if (!(provData = PhGetCryptProviderDataFromStateData(StateData)))
            return;
        if (!(sgnr = PhGetCryptProviderSignerFromChain(provData, 0, FALSE, 0)))
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
    trustData.dwProvFlags = WTD_SAFER_FLAG | WTD_DISABLE_MD2_MD4;

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
    _In_opt_ PCWSTR HashAlgorithm,
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

_Success_(return)
BOOLEAN PhpVerifyGetHashFromFileHandle(
    _In_ HANDLE FileHandle,
    _In_opt_ PCWSTR HashAlgorithm,
    _Out_writes_bytes_(HashTagLength) PWSTR HashTagBuffer,
    _In_ ULONG HashTagLength,
    _Out_writes_bytes_to_(*FileHashLength, *FileHashLength) PUCHAR FileHashBuffer,
    _Inout_ PULONG FileHashLength,
    _Out_ HANDLE *CatAdminHandle
    )
{
    HANDLE catAdminHandle;

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

    if (CryptCATAdminCalcHashFromFileHandle2)
    {
        if (!CryptCATAdminCalcHashFromFileHandle2(catAdminHandle, FileHandle, FileHashLength, FileHashBuffer, 0))
        {
            CryptCATAdminReleaseContext(catAdminHandle, 0);
            return FALSE;
        }
    }
    else
    {
        if (!CryptCATAdminCalcHashFromFileHandle(FileHandle, FileHashLength, FileHashBuffer, 0))
        {
            CryptCATAdminReleaseContext(catAdminHandle, 0);
            return FALSE;
        }
    }

    if (!PhBufferToHexStringBuffer(FileHashBuffer, *FileHashLength, TRUE, HashTagBuffer, HashTagLength, NULL))
    {
        CryptCATAdminReleaseContext(catAdminHandle, 0);
        return FALSE;
    }

    *CatAdminHandle = catAdminHandle;

    return TRUE;
}

VERIFY_RESULT PhpVerifyFileFromCatalog(
    _In_ PPH_VERIFY_FILE_INFO Information,
    _In_ HANDLE FileHandle,
    _In_opt_ PCWSTR HashAlgorithm,
    _Out_ PCERT_CONTEXT **Signatures,
    _Out_ PULONG NumberOfSignatures
    )
{
    VERIFY_RESULT verifyResult = VrBadSignature;
    PCERT_CONTEXT *signatures;
    ULONG numberOfSignatures;
    WINTRUST_CATALOG_INFO catalogInfo;
    LARGE_INTEGER fileSize;
    ULONG fileSizeLimit;
    ULONG fileHashLength = 32; // bytes
    UCHAR fileHash[32];
    ULONG fileHashTagLength = 128; // wchar
    WCHAR fileHashTag[64 + 1];
    HANDLE catAdminHandle;
    HANDLE catInfoHandle;

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

    if (PhpVerifyGetHashFromFileHandle(
        FileHandle,
        HashAlgorithm,
        fileHashTag,
        fileHashTagLength,
        fileHash,
        &fileHashLength,
        &catAdminHandle
        ))
    {
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
                catalogInfo.pcwszMemberTag = fileHashTag;
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

            for (ULONG i = 0; i < Information->NumberOfCatalogFileNames; i++)
            {
                PhFreeVerifySignatures(signatures, numberOfSignatures);

                memset(&catalogInfo, 0, sizeof(catalogInfo));
                catalogInfo.cbStruct = sizeof(catalogInfo);
                catalogInfo.pcwszCatalogFilePath = Information->CatalogFileNames[i];
                catalogInfo.pcwszMemberFilePath = NULL; // Information->FileName
                catalogInfo.hMemberFile = FileHandle;
                catalogInfo.pcwszMemberTag = fileHashTag;
                catalogInfo.pbCalculatedFileHash = fileHash;
                catalogInfo.cbCalculatedFileHash = fileHashLength;
                catalogInfo.hCatAdmin = catAdminHandle;
                verifyResult = PhpVerifyFile(Information, WTD_CHOICE_CATALOG, &catalogInfo, &WinTrustActionGenericVerifyV2, NULL, &signatures, &numberOfSignatures);

                if (verifyResult == VrTrusted)
                    break;
            }
        }

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
    fileInfo.pgKnownSubject = (PGUID)&WINTRUST_KNOWN_SUBJECT_PE_IMAGE;
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
                BCRYPT_SHA1_ALGORITHM,
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

    if (string->Length > sizeof(UNICODE_NULL))
        string->Length -= sizeof(UNICODE_NULL);
    // PhTrimToNullTerminatorString(string);

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

BOOLEAN PhGetSystemComponentFromCertificate(
    _In_ PCERT_CONTEXT Certificate
    )
{
    BOOLEAN found;
    UCHAR usageBuffer[256];
    ULONG usageLength = sizeof(usageBuffer);
    PCERT_ENHKEY_USAGE usage = (PCERT_ENHKEY_USAGE)usageBuffer;

    if (!CertGetEnhancedKeyUsage_I(Certificate, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, usage, &usageLength))
    {
        assert(FALSE);
        return FALSE;
    }

    found = FALSE;

    for (ULONG i = 0; i < usage->cUsageIdentifier; i++)
    {
        if (PhEqualBytesZ(usage->rgpszUsageIdentifier[i], szOID_NT5_CRYPTO, FALSE)) // Windows System Component Verification (dmex)
        {
            found = TRUE;
            break;
        }
    }

    return found;
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

#if (PH_VERIFY_FUTURE)
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

PPH_STRING PhGetProgramNameFromMessage(
    _In_ HCRYPTMSG CryptMsgHandle
    )
{
    PPH_STRING signerName = NULL;
    ULONG signerInfoLength = 0;
    PCMSG_SIGNER_INFO signerInfo;

    if (!CryptMsgGetParam(
        CryptMsgHandle,
        CMSG_SIGNER_INFO_PARAM,
        0,
        NULL,
        &signerInfoLength
        ))
    {
        return NULL;
    }

    signerInfo = PhAllocate(signerInfoLength);

    if (!CryptMsgGetParam(
        CryptMsgHandle,
        CMSG_SIGNER_INFO_PARAM,
        0,
        signerInfo,
        &signerInfoLength
        ))
    {
        PhFree(signerInfo);
        return NULL;
    }

    for (ULONG i = 0; i < signerInfo->AuthAttrs.cAttr; i++)
    {
        if (PhEqualBytesZ(SPC_SP_OPUS_INFO_OBJID, signerInfo->AuthAttrs.rgAttr[i].pszObjId, TRUE))
        {
            ULONG opusInfoLength = 0;
            PSPC_SP_OPUS_INFO opusInfo;

            if (!CryptDecodeObjectEx(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                SPC_SP_OPUS_INFO_OBJID,
                signerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
                signerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData,
                CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG,
                NULL,
                &opusInfo,
                &opusInfoLength
                ))
            {
                goto CleanupExit;
            }

            if (opusInfo->pwszProgramName)
            {
                signerName = PhCreateString((PWSTR)opusInfo->pwszProgramName);
            }

            LocalFree(opusInfo);
            break;
        }
    }

CleanupExit:
    PhFree(signerInfo);

    return signerName;
}

// rev from WTHelperIsChainedToMicrosoft (dmex)
BOOLEAN PhIsChainedToMicrosoft(
    _In_ PCCERT_CONTEXT Certificate,
    _In_ HCERTSTORE SiblingStore,
    _In_ BOOLEAN IncludeMicrosoftTestRootCerts
    )
{
    BOOLEAN status = FALSE;
    HCERTSTORE cryptStoreHandle;
    CERT_CHAIN_POLICY_PARA policyPara = { sizeof(CERT_CHAIN_POLICY_PARA) };
    CERT_CHAIN_POLICY_STATUS policyStatus = { sizeof(CERT_CHAIN_POLICY_STATUS) };
    CERT_CHAIN_PARA chainPara = { sizeof(CERT_CHAIN_PARA) };
    PCCERT_CHAIN_CONTEXT chainContext;

    if (!(Certificate && SiblingStore))
        return FALSE;

    if (cryptStoreHandle = CertOpenStore(CERT_STORE_PROV_COLLECTION, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, 0, 0))
    {
        if (CertAddStoreToCollection(cryptStoreHandle, SiblingStore, 0, 0))
        {
            if (IncludeMicrosoftTestRootCerts)
            {
                PVOID wintrust = PhGetLoaderEntryDllBaseZ(L"wintrust.dll");
                ULONG resourceLength;
                PVOID resourceBuffer;
            
                policyPara.dwFlags = MICROSOFT_ROOT_CERT_CHAIN_POLICY_ENABLE_TEST_ROOT_FLAG;
            
                if (PhLoadResource(wintrust, MAKEINTRESOURCE(1), L"MSTESTROOT", &resourceLength, &resourceBuffer))
                {
                    CERT_BLOB certificateBlob = { resourceLength, resourceBuffer };
                    HCERTSTORE siblingStore = NULL;

                    if (CryptQueryObject(
                        CERT_QUERY_OBJECT_BLOB,
                        &certificateBlob,
                        CERT_QUERY_CONTENT_FLAG_CERT,
                        CERT_QUERY_FORMAT_FLAG_ALL,
                        0, 0, 0, 0,
                        &siblingStore,
                        0,
                        0
                        ))
                    {
                        CertAddStoreToCollection(cryptStoreHandle, siblingStore, 0, 0);
                    }
                }

                if (PhLoadResource(wintrust, MAKEINTRESOURCE(2), L"MSTESTROOT", &resourceLength, &resourceBuffer))
                {
                    CERT_BLOB certificateBlob = { resourceLength, resourceBuffer };
                    HCERTSTORE siblingStore = NULL;

                    if (CryptQueryObject(
                        CERT_QUERY_OBJECT_BLOB,
                        &certificateBlob,
                        CERT_QUERY_CONTENT_FLAG_CERT,
                        CERT_QUERY_FORMAT_FLAG_ALL,
                        0, 0, 0, 0,
                        &siblingStore,
                        0,
                        0
                        ))
                    {
                        CertAddStoreToCollection(cryptStoreHandle, siblingStore, 0, 0);
                    }
                }
            }

            if (CertGetCertificateChain(
                HCCE_CURRENT_USER,
                Certificate,
                0, 
                cryptStoreHandle, 
                &chainPara,
                0,
                0, 
                &chainContext
                ))
            {
                if (CertVerifyCertificateChainPolicy(
                    CERT_CHAIN_POLICY_MICROSOFT_ROOT, 
                    chainContext,
                    &policyPara,
                    &policyStatus
                    ))
                {
                    status = (policyStatus.dwError == ERROR_SUCCESS);
                }

                CertFreeCertificateChain(chainContext);
            }
        }

        CertCloseStore(cryptStoreHandle, 0);
    }

    return status;
}

// rev from WTHelperIsChainedToMicrosoftFromStateData (dmex)
BOOLEAN PhIsChainedToMicrosoftFromStateData(
    _In_ HANDLE StateData,
    _In_ BOOLEAN IncludeMicrosoftTestRootCerts
    )
{
    BOOLEAN status = FALSE;
    PCRYPT_PROVIDER_DATA provData;
    PCRYPT_PROVIDER_SGNR provSigner;
    HCERTSTORE cryptStoreHandle;

    provData = PhGetCryptProviderDataFromStateData(StateData);

    if (!provData)
        return FALSE;

    provSigner = PhGetCryptProviderSignerFromChain(provData, 0, FALSE, 0);

    if (!provSigner)
        return FALSE;

    cryptStoreHandle = CertOpenStore(
        CERT_STORE_PROV_MSG,
        provData->dwEncoding,
        0,
        0,
        provData->hMsg
        );

    if (!cryptStoreHandle)
        return FALSE;

    status = PhIsChainedToMicrosoft(
        provSigner->pasCertChain->pCert, 
        cryptStoreHandle, 
        IncludeMicrosoftTestRootCerts
        );

    CertCloseStore(cryptStoreHandle, 0);

    return status;
}

// rev from ChainToMicrosoftRoot (dmex)
BOOLEAN PhVerifyCertificateChainToMicrosoftRoot(
    _In_ PCCERT_CHAIN_CONTEXT ChainContext,
    _In_ BOOLEAN CheckOsBinary
    )
{
    CERT_CHAIN_POLICY_PARA policyPara = { sizeof(CERT_CHAIN_POLICY_PARA) };
    CERT_CHAIN_POLICY_STATUS policyStatus = { sizeof(CERT_CHAIN_POLICY_STATUS) };

    if (CheckOsBinary)
    {
        SetFlag(policyPara.dwFlags, MICROSOFT_ROOT_CERT_CHAIN_POLICY_CHECK_APPLICATION_ROOT_FLAG); // MicrosoftWindows vs MicrosoftCorporation
    }

    if (CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_MICROSOFT_ROOT, ChainContext, &policyPara, &policyStatus))
    {
        if (policyStatus.dwError == ERROR_SUCCESS)
            return TRUE;
    }

    return FALSE;
}

// rev from IsMicrosoftRootChain (dmex)
BOOLEAN PhVerifyCertificateIsMicrosoftRootChain(
    _In_ PCCERT_CHAIN_CONTEXT ChainContext
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN InsiderBuild = FALSE;
    CERT_CHAIN_POLICY_PARA policyPara = { sizeof(CERT_CHAIN_POLICY_PARA) };
    CERT_CHAIN_POLICY_STATUS policyStatus = { sizeof(CERT_CHAIN_POLICY_STATUS) };
    
    if (PhBeginInitOnce(&initOnce))
    {
        SYSTEM_CODEINTEGRITY_INFORMATION integrityInfo;

        if (NT_SUCCESS(NtQuerySystemInformation(
            SystemCodeIntegrityInformation,
            &integrityInfo,
            sizeof(SYSTEM_CODEINTEGRITY_INFORMATION),
            NULL
            )))
        {
            if (BooleanFlagOn(integrityInfo.CodeIntegrityOptions, CODEINTEGRITY_OPTION_FLIGHTING_ENABLED) ||
                BooleanFlagOn(integrityInfo.CodeIntegrityOptions, CODEINTEGRITY_OPTION_TEST_BUILD))
            {
                InsiderBuild = TRUE;
            }
        }

        PhEndInitOnce(&initOnce);
    }

    if (InsiderBuild)
    {
        policyPara.dwFlags = MICROSOFT_ROOT_CERT_CHAIN_POLICY_ENABLE_TEST_ROOT_FLAG; // required
    }

    if (CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_MICROSOFT_ROOT, ChainContext, &policyPara, &policyStatus))
    {
        if (policyStatus.dwError == ERROR_SUCCESS)
            return TRUE;
    }

    return FALSE;
}

#include <mssip.h>
#pragma comment(lib, "crypt32.lib")

#include <pshpack1.h>
typedef struct _SPC_PE_IMAGE_PAGE_HASHES_V1
{
    ULONG PageOffset;
    BYTE PageHash[20]; // SHA-1
} SPC_PE_IMAGE_PAGE_HASHES_V1, *PSPC_PE_IMAGE_PAGE_HASHES_V1;

typedef struct _SPC_PE_IMAGE_PAGE_HASHES_V2
{
    ULONG PageOffset;
    BYTE PageHash[32]; // SHA-256
} SPC_PE_IMAGE_PAGE_HASHES_V2, *PSPC_PE_IMAGE_PAGE_HASHES_V2;
#include <poppack.h>

// Based on peview PvEnumSpcAuthenticodePageHashes (dmex)
PPH_LIST PhEnumSpcAuthenticodePageHashesFromStateData(
    _In_ HANDLE StateData
    )
{
    PPH_LIST pageHashList = NULL;
    PCRYPT_PROVIDER_DATA provData;
    PPROVDATA_SIP provSipData;
    PSIP_INDIRECT_DATA indirectData;
    PSPC_PE_IMAGE_DATA spcPeImageDataBuffer = NULL;
    ULONG spcPeImageDataLength = 0;

    // WintrustSetDefaultIncludePEPageHashes(TRUE);

    if (!(provData = PhGetCryptProviderDataFromStateData(StateData)))
        return FALSE;
    if (!(provSipData = provData->pPDSip))
        return FALSE;
    if (!(indirectData = provSipData->psIndirectData))
        return FALSE;

    if (!PhEqualBytesZ(indirectData->Data.pszObjId, SPC_PE_IMAGE_DATA_OBJID, FALSE))
        return FALSE;

    if (!CryptDecodeObjectEx(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        SPC_PE_IMAGE_DATA_STRUCT,
        indirectData->Data.Value.pbData,
        indirectData->Data.Value.cbData,
        CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG,
        NULL,
        &spcPeImageDataBuffer,
        &spcPeImageDataLength
        ))
    {
        return FALSE;
    }

    if (
        spcPeImageDataBuffer->pFile->dwLinkChoice == SPC_MONIKER_LINK_CHOICE &&
        RtlEqualMemory(spcPeImageDataBuffer->pFile->Moniker.ClassId, (SPC_UUID)SpcSerializedObjectAttributesClassId, sizeof((SPC_UUID)SpcSerializedObjectAttributesClassId))
        )
    {
        ULONG spcSerializedObjectAttributesLength = 0;
        PCRYPT_ATTRIBUTES spcSerializedObjectAttributesBuffer = NULL;

        if (CryptDecodeObjectEx(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            PKCS_ATTRIBUTES,
            spcPeImageDataBuffer->pFile->Moniker.SerializedData.pbData,
            spcPeImageDataBuffer->pFile->Moniker.SerializedData.cbData,
            CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG,
            NULL,
            &spcSerializedObjectAttributesBuffer,
            &spcSerializedObjectAttributesLength
            ))
        {
            for (ULONG i = 0; i < spcSerializedObjectAttributesBuffer->cAttr; i++)
            {
                CRYPT_ATTRIBUTE spcSerializedObjectBuffer = spcSerializedObjectAttributesBuffer->rgAttr[i];
                PCRYPT_DATA_BLOB spcImagePageHashesBuffer = NULL;
                ULONG spcImagePageHashesLength = 0;

                // for (ULONG j = 0; j < spcSerializedObjectBuffer.cValue; j++)

                if (CryptDecodeObjectEx(
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    X509_OCTET_STRING,
                    spcSerializedObjectBuffer.rgValue->pbData,
                    spcSerializedObjectBuffer.rgValue->cbData,
                    CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG,
                    NULL,
                    &spcImagePageHashesBuffer,
                    &spcImagePageHashesLength
                    ))
                {
                    if (PhEqualBytesZ(spcSerializedObjectBuffer.pszObjId, SPC_PE_IMAGE_PAGE_HASHES_V1_OBJID, FALSE))
                    {
                        ULONG count = spcImagePageHashesBuffer->cbData / sizeof(SPC_PE_IMAGE_PAGE_HASHES_V1);
                        pageHashList = PhCreateList(count);

                        for (ULONG k = 0; k < count; k++)
                        {
                            PSPC_PE_IMAGE_PAGE_HASHES_V1 entry = PTR_ADD_OFFSET(spcImagePageHashesBuffer->pbData, sizeof(SPC_PE_IMAGE_PAGE_HASHES_V1) * k);
                            PSPC_PE_IMAGE_PAGE_HASHES_V2 thunk;

                            thunk = PhAllocateZero(sizeof(SPC_PE_IMAGE_PAGE_HASHES_V2));
                            thunk->PageOffset = entry->PageOffset;
                            memcpy_s(thunk->PageHash, sizeof(thunk->PageHash), entry->PageHash, sizeof(entry->PageHash));
                            PhAddItemList(pageHashList, thunk);
                        }
                    }
                    else if (PhEqualBytesZ(spcSerializedObjectBuffer.pszObjId, SPC_PE_IMAGE_PAGE_HASHES_V2_OBJID, FALSE))
                    {
                        ULONG count = spcImagePageHashesBuffer->cbData / sizeof(SPC_PE_IMAGE_PAGE_HASHES_V2);
                        pageHashList = PhCreateList(count);

                        for (ULONG k = 0; k < count; k++)
                        {
                            PSPC_PE_IMAGE_PAGE_HASHES_V2 entry = PTR_ADD_OFFSET(spcImagePageHashesBuffer->pbData, sizeof(SPC_PE_IMAGE_PAGE_HASHES_V2) * k);

                            PhAddItemList(pageHashList, PhAllocateCopy(entry, sizeof(SPC_PE_IMAGE_PAGE_HASHES_V2)));
                        }
                    }

                    LocalFree(spcImagePageHashesBuffer);
                }
            }

            LocalFree(spcSerializedObjectAttributesBuffer);
        }
    }

    LocalFree(spcPeImageDataBuffer);

    return pageHashList;
}

#endif
