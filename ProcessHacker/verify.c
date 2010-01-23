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

VOID PhVerifyInitialization()
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
    PVOID buffer;
    ULONG bufferSize = 0x100;
    ULONG returnLength;

    buffer = PhAllocate(bufferSize * 2);

    returnLength = CertNameToStr_I(
        X509_ASN_ENCODING,
        Blob,
        CERT_X500_NAME_STR,
        buffer,
        bufferSize
        );

    if (returnLength > bufferSize)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize * 2);

        returnLength = CertNameToStr_I(
            X509_ASN_ENCODING,
            Blob,
            CERT_X500_NAME_STR,
            buffer,
            bufferSize
            );
    }

    string = PhCreateString((PWSTR)buffer);
    PhFree(buffer);

    return string;
}

PPH_STRING PhpGetX500Value(
    __in PPH_STRING String,
    __in PWSTR KeyName
    )
{
    WCHAR keyNamePlusEquals[10];
    ULONG keyNameLength;
    ULONG startIndex;
    ULONG endIndex;

    keyNameLength = (ULONG)wcslen(KeyName);
    wcsncpy(keyNamePlusEquals, KeyName, 8);
    keyNamePlusEquals[keyNameLength] = '=';
    keyNamePlusEquals[keyNameLength + 1] = 0;

    // Find "Key=".
    startIndex = PhStringIndexOfString(String, 0, keyNamePlusEquals);

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

        endIndex = PhStringIndexOfChar(String, startIndex, '"');

        // It's an error if we didn't find the matching quotation mark.
        if (endIndex == -1)
            return NULL;
    }
    else
    {
        endIndex = PhStringIndexOfChar(String, startIndex, ',');

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

    value = PhpGetX500Value(name, L"CN");

    if (!value)
        value = PhpGetX500Value(name, L"OU");

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

    status = WinVerifyTrust_I(NULL, &actionGenericVerifyV2, &trustData);

    if (status != TRUST_E_NOSIGNATURE)
        *SignerName = PhpGetSignerNameFromStateData(trustData.hWVTStateData);

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
    PBYTE fileHash = NULL;
    ULONG fileHashLength;
    PWSTR fileHashTag = NULL;
    HANDLE catAdminHandle = NULL;
    HANDLE catInfoHandle = NULL;
    ULONG i;

    fileHandle = CreateFile(
        FileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (fileHandle == INVALID_HANDLE_VALUE)
        return VrNoSignature;

    fileHashLength = 256;
    fileHash = (PBYTE)PhAllocate(fileHashLength);

    if (!CryptCATAdminCalcHashFromFileHandle(fileHandle, &fileHashLength, fileHash, 0))
    {
        fileHash = (PBYTE)PhReAlloc(fileHash, fileHashLength);

        if (!CryptCATAdminCalcHashFromFileHandle(fileHandle, &fileHashLength, fileHash, 0))
        {
            CloseHandle(fileHandle);
            PhFree(fileHash);
            return VrNoSignature;
        }
    }

    CloseHandle(fileHandle);

    if (!CryptCATAdminAcquireContext(&catAdminHandle, &driverActionVerify, 0))
    {
        PhFree(fileHash);
        return VrNoSignature;
    }

    fileHashTag = (PWSTR)PhAllocate((fileHashLength * 2 + 1) * sizeof(WCHAR));

    for (i = 0; i < fileHashLength; i++)
        wsprintfW(&fileHashTag[i * 2], L"%02X", fileHash[i]);

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

            status = WinVerifyTrust_I(NULL, &driverActionVerify, &trustData);

            if (status != TRUST_E_NOSIGNATURE)
                *SignerName = PhpGetSignerNameFromStateData(trustData.hWVTStateData);

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

VERIFY_RESULT PhVerifyFile(
    __in PWSTR FileName,
    __out_opt PPH_STRING *SignerName
    )
{
    VERIFY_RESULT result;

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
