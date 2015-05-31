/*
 * Process Hacker -
 *   image verification
 *
 * Copyright (C) 2009-2013 wj32
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ph.h>
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

static VOID PhpVerifyInitialization(
    VOID
    )
{
    HMODULE wintrust;
    HMODULE crypt32;

    wintrust = LoadLibrary(L"wintrust.dll");
    crypt32 = LoadLibrary(L"crypt32.dll");

    CryptCATAdminCalcHashFromFileHandle = (PVOID)GetProcAddress(wintrust, "CryptCATAdminCalcHashFromFileHandle");
    CryptCATAdminCalcHashFromFileHandle2 = (PVOID)GetProcAddress(wintrust, "CryptCATAdminCalcHashFromFileHandle2");
    CryptCATAdminAcquireContext = (PVOID)GetProcAddress(wintrust, "CryptCATAdminAcquireContext");
    CryptCATAdminAcquireContext2 = (PVOID)GetProcAddress(wintrust, "CryptCATAdminAcquireContext2");
    CryptCATAdminEnumCatalogFromHash = (PVOID)GetProcAddress(wintrust, "CryptCATAdminEnumCatalogFromHash");
    CryptCATCatalogInfoFromContext = (PVOID)GetProcAddress(wintrust, "CryptCATCatalogInfoFromContext");
    CryptCATAdminReleaseCatalogContext = (PVOID)GetProcAddress(wintrust, "CryptCATAdminReleaseCatalogContext");
    CryptCATAdminReleaseContext = (PVOID)GetProcAddress(wintrust, "CryptCATAdminReleaseContext");
    WTHelperProvDataFromStateData_I = (PVOID)GetProcAddress(wintrust, "WTHelperProvDataFromStateData");
    WTHelperGetProvSignerFromChain_I = (PVOID)GetProcAddress(wintrust, "WTHelperGetProvSignerFromChain");
    WinVerifyTrust_I = (PVOID)GetProcAddress(wintrust, "WinVerifyTrust");
    CertNameToStr_I = (PVOID)GetProcAddress(crypt32, "CertNameToStrW");
    CertDuplicateCertificateContext_I = (PVOID)GetProcAddress(crypt32, "CertDuplicateCertificateContext");
    CertFreeCertificateContext_I = (PVOID)GetProcAddress(crypt32, "CertFreeCertificateContext");
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
        HMODULE cryptui = LoadLibrary(L"cryptui.dll");

        cryptUIDlgViewSignerInfo = (PVOID)GetProcAddress(cryptui, "CryptUIDlgViewSignerInfoW");
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
    _In_ HANDLE FileHandle,
    _In_ ULONG UnionChoice,
    _In_ PVOID UnionData,
    _In_ PGUID ActionId,
    _In_opt_ PVOID PolicyCallbackData,
    _Out_ PCERT_CONTEXT **Signatures,
    _Out_ PULONG NumberOfSignatures
    )
{
    LONG status;
    WINTRUST_DATA trustData = { 0 };

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

        if (WindowsVersion >= WINDOWS_VISTA)
            trustData.dwProvFlags |= WTD_CACHE_ONLY_URL_RETRIEVAL;
        else
            trustData.dwProvFlags |= WTD_REVOCATION_CHECK_NONE;
    }

    status = WinVerifyTrust_I(NULL, ActionId, &trustData);
    PhpGetSignaturesFromStateData(trustData.hWVTStateData, Signatures, NumberOfSignatures);

    if (status == 0 && (Information->Flags & PH_VERIFY_VIEW_PROPERTIES))
        PhpViewSignerInfo(Information, trustData.hWVTStateData);

    // Close the state data.
    trustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust_I(NULL, ActionId, &trustData);

    return PhpStatusToVerifyResult(status);
}

BOOLEAN PhpCalculateFileHash(
    _In_ HANDLE FileHandle,
    _In_ PWSTR HashAlgorithm,
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
    WINTRUST_CATALOG_INFO catalogInfo = { 0 };
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

    if (Information->FileSizeLimitForHash != -1)
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
            CATALOG_INFO ci = { 0 };
            DRIVER_VER_INFO verInfo = { 0 };

            if (CryptCATCatalogInfoFromContext(catInfoHandle, &ci, 0))
            {
                // Disable OS version checking by passing in a DRIVER_VER_INFO structure.
                verInfo.cbStruct = sizeof(DRIVER_VER_INFO);

                catalogInfo.cbStruct = sizeof(catalogInfo);
                catalogInfo.pcwszCatalogFilePath = ci.wszCatalogFile;
                catalogInfo.pcwszMemberFilePath = Information->FileName;
                catalogInfo.pcwszMemberTag = fileHashTag->Buffer;
                catalogInfo.pbCalculatedFileHash = fileHash;
                catalogInfo.cbCalculatedFileHash = fileHashLength;
                catalogInfo.hCatAdmin = catAdminHandle;
                verifyResult = PhpVerifyFile(Information, FileHandle, WTD_CHOICE_CATALOG, &catalogInfo, &DriverActionVerify, &verInfo, &signatures, &numberOfSignatures);

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

                catalogInfo.cbStruct = sizeof(catalogInfo);
                catalogInfo.pcwszCatalogFilePath = Information->CatalogFileNames[i];
                catalogInfo.pcwszMemberFilePath = Information->FileName;
                catalogInfo.pcwszMemberTag = fileHashTag->Buffer;
                catalogInfo.pbCalculatedFileHash = fileHash;
                catalogInfo.cbCalculatedFileHash = fileHashLength;
                catalogInfo.hCatAdmin = catAdminHandle;
                verifyResult = PhpVerifyFile(Information, FileHandle, WTD_CHOICE_CATALOG, &catalogInfo, &WinTrustActionGenericVerifyV2, NULL, &signatures, &numberOfSignatures);

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
    NTSTATUS status;
    HANDLE fileHandle;
    VERIFY_RESULT verifyResult;
    PCERT_CONTEXT *signatures;
    ULONG numberOfSignatures;
    WINTRUST_FILE_INFO fileInfo = { 0 };

    if (PhBeginInitOnce(&PhpVerifyInitOnce))
    {
        PhpVerifyInitialization();
        PhEndInitOnce(&PhpVerifyInitOnce);
    }

    // Make sure we have successfully imported
    // the required functions.
    if (
        !CryptCATAdminCalcHashFromFileHandle ||
        !CryptCATAdminAcquireContext ||
        !CryptCATAdminEnumCatalogFromHash ||
        !CryptCATCatalogInfoFromContext ||
        !CryptCATAdminReleaseCatalogContext ||
        !CryptCATAdminReleaseContext ||
        !WinVerifyTrust_I ||
        !WTHelperProvDataFromStateData_I ||
        !WTHelperGetProvSignerFromChain_I ||
        !CertNameToStr_I ||
        !CertDuplicateCertificateContext_I ||
        !CertFreeCertificateContext_I
        )
        return STATUS_NOT_SUPPORTED;

    if (!NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        Information->FileName,
        FILE_GENERIC_READ,
        0,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
        return status;

    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = Information->FileName;
    fileInfo.hFile = fileHandle;

    verifyResult = PhpVerifyFile(Information, fileHandle, WTD_CHOICE_FILE, &fileInfo, &WinTrustActionGenericVerifyV2, NULL, &signatures, &numberOfSignatures);

    if (verifyResult == VrNoSignature)
    {
        if (CryptCATAdminAcquireContext2 && CryptCATAdminCalcHashFromFileHandle2)
        {
            PhFreeVerifySignatures(signatures, numberOfSignatures);
            verifyResult = PhpVerifyFileFromCatalog(Information, fileHandle, BCRYPT_SHA256_ALGORITHM, &signatures, &numberOfSignatures);
        }

        if (verifyResult != VrTrusted)
        {
            PhFreeVerifySignatures(signatures, numberOfSignatures);
            verifyResult = PhpVerifyFileFromCatalog(Information, fileHandle, NULL, &signatures, &numberOfSignatures);
        }
    }

    *VerifyResult = verifyResult;

    if (Signatures)
        *Signatures = signatures;
    else
        PhFreeVerifySignatures(signatures, numberOfSignatures);

    if (NumberOfSignatures)
        *NumberOfSignatures = numberOfSignatures;

    NtClose(fileHandle);

    return STATUS_SUCCESS;
}

VOID PhFreeVerifySignatures(
    _In_ PCERT_CONTEXT *Signatures,
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

    // CertNameToStr doesn't give us the correct buffer size unless we
    // don't provide a buffer at all.
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
    keyNamePlusEquals.Buffer[keyNameLength] = '=';

    // Find "Key=".

    if (!PhSplitStringRefAtString(String, &keyNamePlusEquals, FALSE, &firstPart, &remainingPart))
        return NULL;
    if (remainingPart.Length == 0)
        return NULL;

    // Is the value quoted? If so, return the part inside the quotes.
    if (remainingPart.Buffer[0] == '"')
    {
        PhSkipStringRef(&remainingPart, sizeof(WCHAR));

        if (!PhSplitStringRefAtChar(&remainingPart, '"', &firstPart, &remainingPart))
            return NULL;

        return PhCreateString2(&firstPart);
    }
    else
    {
        PhSplitStringRefAtChar(&remainingPart, ',', &firstPart, &remainingPart);

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
 * \param SignerName A variable which receives a pointer
 * to a string containing the signer name. You must free
 * the string using PhDereferenceObject() when you no
 * longer need it. Note that the signer name may be NULL
 * if it is not valid.
 *
 * \return A VERIFY_RESULT value.
 */
VERIFY_RESULT PhVerifyFile(
    _In_ PWSTR FileName,
    _Out_opt_ PPH_STRING *SignerName
    )
{
    PH_VERIFY_FILE_INFO info = { 0 };
    VERIFY_RESULT verifyResult;
    PCERT_CONTEXT *signatures;
    ULONG numberOfSignatures;

    info.FileName = FileName;
    info.Flags = PH_VERIFY_PREVENT_NETWORK_ACCESS;

    if (NT_SUCCESS(PhVerifyFileEx(&info, &verifyResult, &signatures, &numberOfSignatures)))
    {
        if (SignerName)
        {
            *SignerName = NULL;

            if (numberOfSignatures != 0)
                *SignerName = PhGetSignerNameFromCertificate(signatures[0]);
        }

        PhFreeVerifySignatures(signatures, numberOfSignatures);
        return verifyResult;
    }
    else
    {
        if (SignerName)
            *SignerName = NULL;

        return VrNoSignature;
    }
}
