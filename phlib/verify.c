/*
 * Process Hacker - 
 *   image verification
 * 
 * Copyright (C) 2009-2010 wj32
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
#include <verifyp.h>

_CryptCATAdminCalcHashFromFileHandle CryptCATAdminCalcHashFromFileHandle;
_CryptCATAdminAcquireContext CryptCATAdminAcquireContext;
_CryptCATAdminEnumCatalogFromHash CryptCATAdminEnumCatalogFromHash;
_CryptCATCatalogInfoFromContext CryptCATCatalogInfoFromContext;
_CryptCATAdminReleaseCatalogContext CryptCATAdminReleaseCatalogContext;
_CryptCATAdminReleaseContext CryptCATAdminReleaseContext;
_WTHelperProvDataFromStateData WTHelperProvDataFromStateData_I;
_WTHelperGetProvSignerFromChain WTHelperGetProvSignerFromChain_I;
_WinVerifyTrust WinVerifyTrust_I;
_CertNameToStr CertNameToStr_I;
static PH_INITONCE PhpVerifyInitOnce = PH_INITONCE_INIT;

static VOID PhpVerifyInitialization()
{
    LoadLibrary(L"wintrust.dll");
    LoadLibrary(L"crypt32.dll");

    CryptCATAdminCalcHashFromFileHandle = 
        PhGetProcAddress(L"wintrust.dll", "CryptCATAdminCalcHashFromFileHandle");
    CryptCATAdminAcquireContext = 
        PhGetProcAddress(L"wintrust.dll", "CryptCATAdminAcquireContext");
    CryptCATAdminEnumCatalogFromHash =
        PhGetProcAddress(L"wintrust.dll", "CryptCATAdminEnumCatalogFromHash");
    CryptCATCatalogInfoFromContext =
        PhGetProcAddress(L"wintrust.dll", "CryptCATCatalogInfoFromContext");
    CryptCATAdminReleaseCatalogContext =
        PhGetProcAddress(L"wintrust.dll", "CryptCATAdminReleaseCatalogContext");
    CryptCATAdminReleaseContext =
        PhGetProcAddress(L"wintrust.dll", "CryptCATAdminReleaseContext");
    WTHelperProvDataFromStateData_I =
        PhGetProcAddress(L"wintrust.dll", "WTHelperProvDataFromStateData");
    WTHelperGetProvSignerFromChain_I =
        PhGetProcAddress(L"wintrust.dll", "WTHelperGetProvSignerFromChain");
    WinVerifyTrust_I =
        PhGetProcAddress(L"wintrust.dll", "WinVerifyTrust");
    CertNameToStr_I =
        PhGetProcAddress(L"crypt32.dll", "CertNameToStrW");
}

VERIFY_RESULT PhpStatusToVerifyResult(
    __in LONG Status
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
    default:
        return VrSecuritySettings;
    }
}

PPH_STRING PhpGetCertNameString(
    __in PCERT_NAME_BLOB Blob
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
    __in PPH_STRING String,
    __in PPH_STRINGREF KeyName
    )
{
    WCHAR keyNamePlusEquals[10];
    ULONG keyNameLength;
    ULONG startIndex;
    ULONG endIndex;

    keyNameLength = KeyName->Length / sizeof(WCHAR);
    assert(!(keyNameLength > sizeof(keyNamePlusEquals) / sizeof(WCHAR) - 2));

    memcpy(keyNamePlusEquals, KeyName->Buffer, KeyName->Length);
    keyNamePlusEquals[keyNameLength] = '=';
    keyNamePlusEquals[keyNameLength + 1] = 0;

    // Find "Key=".
    startIndex = PhFindStringInString(String, 0, keyNamePlusEquals);

    if (startIndex == -1)
        return NULL;

    startIndex += keyNameLength + 1;

    if (startIndex * sizeof(WCHAR) >= String->Length)
        return NULL;

    // Is the value quoted?
    if (String->Buffer[startIndex] == '"')
    {
        startIndex++;

        if (startIndex * sizeof(WCHAR) >= String->Length)
            return NULL;

        endIndex = PhFindCharInString(String, startIndex, '"');

        // It's an error if we didn't find the matching quotation mark.
        if (endIndex == -1)
            return NULL;
    }
    else
    {
        endIndex = PhFindCharInString(String, startIndex, ',');

        // If we didn't find a comma, it means the key/value pair is 
        // the last one in the string.
        if (endIndex == -1)
            endIndex = String->Length / sizeof(WCHAR);
    }

    return PhSubstring(String, startIndex, endIndex - startIndex);
}

PPH_STRING PhpGetSignerNameFromStateData(
    __in HANDLE StateData
    )
{
    PCRYPT_PROVIDER_DATA provData;
    PCRYPT_PROVIDER_SGNR sgnr;
    PCRYPT_PROVIDER_CERT cert;
    PCCERT_CONTEXT certContext;
    PCERT_INFO certInfo;
    PH_STRINGREF keyName;
    PPH_STRING name;
    PPH_STRING value;

    // 1. State data -> provider data.

    provData = WTHelperProvDataFromStateData_I(StateData);

    if (!provData)
        return NULL;

    // 2. Provider data -> Provider signer

    sgnr = WTHelperGetProvSignerFromChain_I(provData, 0, FALSE, 0);

    if (!sgnr)
        return NULL;
    if (!sgnr->pasCertChain)
        return NULL;
    if (sgnr->csCertChain == 0)
        return NULL;

    // 3. Provider signer -> Provider cert

    cert = &sgnr->pasCertChain[0];

    // 4. Provider cert -> Cert context

    certContext = cert->pCert;

    if (!certContext)
        return NULL;

    // 5. Cert context -> Cert info

    certInfo = certContext->pCertInfo;

    if (!certInfo)
        return NULL;

    // 6. Cert info subject -> Subject X.500 string

    name = PhpGetCertNameString(&certInfo->Subject);

    // 7. Subject X.500 string -> CN or OU value

    PhInitializeStringRef(&keyName, L"CN");
    value = PhpGetX500Value(name, &keyName);

    if (!value)
    {
        PhInitializeStringRef(&keyName, L"OU");
        value = PhpGetX500Value(name, &keyName);
    }

    PhDereferenceObject(name);

    return value;
}

VERIFY_RESULT PhpVerifyFileBasic(
    __in PWSTR FileName,
    __out_opt PPH_STRING *SignerName
    )
{
    LONG status;
    WINTRUST_DATA trustData = { 0 };
    WINTRUST_FILE_INFO fileInfo = { 0 };
    GUID actionGenericVerifyV2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = FileName;

    trustData.cbStruct = sizeof(WINTRUST_DATA);
    trustData.dwUIChoice = WTD_UI_NONE;
    trustData.dwProvFlags = WTD_SAFER_FLAG;
    trustData.dwUnionChoice = WTD_CHOICE_FILE;
    trustData.dwStateAction = WTD_STATEACTION_VERIFY;
    trustData.pFile = &fileInfo;

    if (WindowsVersion >= WINDOWS_VISTA)
        trustData.dwProvFlags |= WTD_CACHE_ONLY_URL_RETRIEVAL;
    else
        trustData.dwProvFlags |= WTD_REVOCATION_CHECK_NONE;

    status = WinVerifyTrust_I(NULL, &actionGenericVerifyV2, &trustData);

    if (SignerName)
    {
        if (status != TRUST_E_NOSIGNATURE)
            *SignerName = PhpGetSignerNameFromStateData(trustData.hWVTStateData);
        else
            *SignerName = NULL;
    }

    // Close the state data.
    trustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust_I(NULL, &actionGenericVerifyV2, &trustData);

    return PhpStatusToVerifyResult(status);
}

VERIFY_RESULT PhpVerifyFileFromCatalog(
    __in PWSTR FileName,
    __out_opt PPH_STRING *SignerName
    )
{
    LONG status = TRUST_E_NOSIGNATURE;
    WINTRUST_DATA trustData = { 0 };
    WINTRUST_CATALOG_INFO catalogInfo = { 0 };
    GUID driverActionVerify = DRIVER_ACTION_VERIFY;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    PUCHAR fileHash = NULL;
    ULONG fileHashLength;
    PWSTR fileHashTag = NULL;
    HANDLE catAdminHandle = NULL;
    HANDLE catInfoHandle = NULL;
    ULONG i;

    if (!NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        0,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
        return VrNoSignature;

    // Don't try to hash files over 32 MB in size.
    if (!NT_SUCCESS(PhGetFileSize(fileHandle, &fileSize)) ||
        (fileSize.QuadPart > 32 * 1024 * 1024))
    {
        NtClose(fileHandle);
        return VrNoSignature;
    }

    fileHashLength = 256;
    fileHash = PhAllocate(fileHashLength);

    if (!CryptCATAdminCalcHashFromFileHandle(fileHandle, &fileHashLength, fileHash, 0))
    {
        PhFree(fileHash);
        fileHash = PhAllocate(fileHashLength);

        if (!CryptCATAdminCalcHashFromFileHandle(fileHandle, &fileHashLength, fileHash, 0))
        {
            NtClose(fileHandle);
            PhFree(fileHash);
            return VrNoSignature;
        }
    }

    NtClose(fileHandle);

    if (!CryptCATAdminAcquireContext(&catAdminHandle, &driverActionVerify, 0))
    {
        PhFree(fileHash);
        return VrNoSignature;
    }

    fileHashTag = PhAllocate((fileHashLength * 2 + 1) * sizeof(WCHAR));

    for (i = 0; i < fileHashLength; i++)
    {
        fileHashTag[i * 2] = PhIntegerToCharUpper[fileHash[i] >> 4];
        fileHashTag[i * 2 + 1] = PhIntegerToCharUpper[fileHash[i] & 0xf];
    }

    fileHashTag[fileHashLength * 2] = 0;

    catInfoHandle = CryptCATAdminEnumCatalogFromHash(
        catAdminHandle,
        fileHash,
        fileHashLength,
        0,
        NULL
        );

    PhFree(fileHash);

    if (catInfoHandle)
    {
        CATALOG_INFO ci = { 0 };

        if (CryptCATCatalogInfoFromContext(catInfoHandle, &ci, 0))
        {
            catalogInfo.cbStruct = sizeof(catalogInfo);
            catalogInfo.pcwszCatalogFilePath = ci.wszCatalogFile;
            catalogInfo.pcwszMemberFilePath = FileName;
            catalogInfo.pcwszMemberTag = fileHashTag;

            trustData.cbStruct = sizeof(trustData);
            trustData.dwUIChoice = WTD_UI_NONE;
            trustData.fdwRevocationChecks = WTD_STATEACTION_VERIFY;
            trustData.dwUnionChoice = WTD_CHOICE_CATALOG;
            trustData.dwStateAction = WTD_STATEACTION_VERIFY;
            trustData.pCatalog = &catalogInfo;

            if (WindowsVersion >= WINDOWS_VISTA)
                trustData.dwProvFlags |= WTD_CACHE_ONLY_URL_RETRIEVAL;
            else
                trustData.dwProvFlags |= WTD_REVOCATION_CHECK_NONE;

            status = WinVerifyTrust_I(NULL, &driverActionVerify, &trustData);

            if (SignerName)
            {
                if (status != TRUST_E_NOSIGNATURE)
                    *SignerName = PhpGetSignerNameFromStateData(trustData.hWVTStateData);
                else
                    *SignerName = NULL;
            }

            // Close the state data.
            trustData.dwStateAction = WTD_STATEACTION_CLOSE;
            WinVerifyTrust_I(NULL, &driverActionVerify, &trustData);
        }

        CryptCATAdminReleaseCatalogContext(catAdminHandle, catInfoHandle, 0);
    }

    PhFree(fileHashTag);
    CryptCATAdminReleaseContext(catAdminHandle, 0);

    return PhpStatusToVerifyResult(status);
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
    __in PWSTR FileName,
    __out_opt PPH_STRING *SignerName
    )
{
    VERIFY_RESULT result;

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
        !CertNameToStr_I
        )
        return VrUnknown;

    result = PhpVerifyFileBasic(FileName, SignerName);

    if (result == VrNoSignature)
        result = PhpVerifyFileFromCatalog(FileName, SignerName);

    return result;
}
