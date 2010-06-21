#ifndef _PH_VERIFYP_H
#define _PH_VERIFYP_H

#include <wintrust.h>
#include <softpub.h>

typedef struct _CATALOG_INFO
{
    DWORD cbStruct;
    WCHAR wszCatalogFile[MAX_PATH];
} CATALOG_INFO, *PCATALOG_INFO;

typedef BOOL (WINAPI *_CryptCATAdminCalcHashFromFileHandle)(
    HANDLE hFile,
    DWORD *pcbHash,
    BYTE *pbHash,
    DWORD dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminAcquireContext)(
    HANDLE *phCatAdmin,
    GUID *pgSubsystem,
    DWORD dwFlags
    );

typedef HANDLE (WINAPI *_CryptCATAdminEnumCatalogFromHash)(
    HANDLE hCatAdmin,
    BYTE *pbHash,
    DWORD cbHash,
    DWORD dwFlags,
    HANDLE *phPrevCatInfo
    );

typedef BOOL (WINAPI *_CryptCATCatalogInfoFromContext)(
    HANDLE hCatInfo,
    CATALOG_INFO *psCatInfo,
    DWORD dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminReleaseCatalogContext)(
    HANDLE hCatAdmin,
    HANDLE hCatInfo,
    DWORD dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminReleaseContext)(
    HANDLE hCatAdmin,
    DWORD dwFlags
    );

typedef PCRYPT_PROVIDER_DATA (WINAPI *_WTHelperProvDataFromStateData)(
    HANDLE hStateData
    );

typedef PCRYPT_PROVIDER_SGNR (WINAPI *_WTHelperGetProvSignerFromChain)(
    CRYPT_PROVIDER_DATA *pProvData,
    DWORD idxSigner,
    BOOL fCounterSigner,
    DWORD idxCounterSigner
    );

typedef LONG (WINAPI *_WinVerifyTrust)(
    HWND hWnd,
    GUID *pgActionID,
    LPVOID pWVTData
    );

typedef DWORD (WINAPI *_CertNameToStr)(
    DWORD dwCertEncodingType,
    PCERT_NAME_BLOB pName,
    DWORD dwStrType,
    LPTSTR psz,
    DWORD csz
    );

#endif
