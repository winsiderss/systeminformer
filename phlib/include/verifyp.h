#ifndef _PH_VERIFYP_H
#define _PH_VERIFYP_H

#include <commdlg.h>

typedef struct _CATALOG_INFO
{
    DWORD cbStruct;
    WCHAR wszCatalogFile[MAX_PATH];
} CATALOG_INFO, *PCATALOG_INFO;

typedef struct tagCRYPTUI_VIEWSIGNERINFO_STRUCT {
    DWORD dwSize;
    HWND hwndParent;
    DWORD dwFlags;
    LPCTSTR szTitle;
    CMSG_SIGNER_INFO *pSignerInfo;
    HCRYPTMSG hMsg;
    LPCSTR pszOID;
    DWORD_PTR dwReserved;
    DWORD cStores;
    HCERTSTORE *rghStores;
    DWORD cPropSheetPages;
    LPCPROPSHEETPAGE rgPropSheetPages;
} CRYPTUI_VIEWSIGNERINFO_STRUCT, *PCRYPTUI_VIEWSIGNERINFO_STRUCT;

typedef BOOL (WINAPI *_CryptCATAdminCalcHashFromFileHandle)(
    HANDLE hFile,
    DWORD *pcbHash,
    BYTE *pbHash,
    DWORD dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminCalcHashFromFileHandle2)(
    HCATADMIN hCatAdmin,
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

typedef BOOL (WINAPI *_CryptCATAdminAcquireContext2)(
    HCATADMIN *phCatAdmin,
    const GUID *pgSubsystem,
    PCWSTR pwszHashAlgorithm,
    PCCERT_STRONG_SIGN_PARA pStrongHashPolicy,
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

typedef PCCERT_CONTEXT (WINAPI *_CertDuplicateCertificateContext)(
    _In_ PCCERT_CONTEXT pCertContext
    );

typedef BOOL (WINAPI *_CertFreeCertificateContext)(
    _In_ PCCERT_CONTEXT pCertContext
    );

typedef BOOL (WINAPI *_CryptUIDlgViewSignerInfo)(
    _In_ CRYPTUI_VIEWSIGNERINFO_STRUCT *pcvsi
    );

#endif
