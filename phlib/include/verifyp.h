#ifndef _PH_VERIFYP_H
#define _PH_VERIFYP_H

#include <commdlg.h>

typedef struct _CATALOG_INFO
{
    ULONG cbStruct;
    WCHAR wszCatalogFile[MAX_PATH];
} CATALOG_INFO, *PCATALOG_INFO;

typedef struct tagCRYPTUI_VIEWSIGNERINFO_STRUCT {
    ULONG dwSize;
    HWND hwndParent;
    ULONG dwFlags;
    LPCTSTR szTitle;
    CMSG_SIGNER_INFO *pSignerInfo;
    HCRYPTMSG hMsg;
    LPCSTR pszOID;
    ULONG_PTR dwReserved;
    ULONG cStores;
    HCERTSTORE *rghStores;
    ULONG cPropSheetPages;
    LPCPROPSHEETPAGE rgPropSheetPages;
} CRYPTUI_VIEWSIGNERINFO_STRUCT, *PCRYPTUI_VIEWSIGNERINFO_STRUCT;

typedef BOOL (WINAPI *_CryptCATAdminCalcHashFromFileHandle)(
    HANDLE hFile,
    ULONG *pcbHash,
    BYTE *pbHash,
    ULONG dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminCalcHashFromFileHandle2)(
    HCATADMIN hCatAdmin,
    HANDLE hFile,
    ULONG *pcbHash,
    BYTE *pbHash,
    ULONG dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminAcquireContext)(
    HANDLE *phCatAdmin,
    GUID *pgSubsystem,
    ULONG dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminAcquireContext2)(
    HCATADMIN *phCatAdmin,
    const GUID *pgSubsystem,
    PCWSTR pwszHashAlgorithm,
    PCCERT_STRONG_SIGN_PARA pStrongHashPolicy,
    ULONG dwFlags
    );

typedef HANDLE (WINAPI *_CryptCATAdminEnumCatalogFromHash)(
    HANDLE hCatAdmin,
    BYTE *pbHash,
    ULONG cbHash,
    ULONG dwFlags,
    HANDLE *phPrevCatInfo
    );

typedef BOOL (WINAPI *_CryptCATCatalogInfoFromContext)(
    HANDLE hCatInfo,
    CATALOG_INFO *psCatInfo,
    ULONG dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminReleaseCatalogContext)(
    HANDLE hCatAdmin,
    HANDLE hCatInfo,
    ULONG dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminReleaseContext)(
    HANDLE hCatAdmin,
    ULONG dwFlags
    );

typedef PCRYPT_PROVIDER_DATA (WINAPI *_WTHelperProvDataFromStateData)(
    HANDLE hStateData
    );

typedef PCRYPT_PROVIDER_SGNR (WINAPI *_WTHelperGetProvSignerFromChain)(
    CRYPT_PROVIDER_DATA *pProvData,
    ULONG idxSigner,
    BOOL fCounterSigner,
    ULONG idxCounterSigner
    );

typedef LONG (WINAPI *_WinVerifyTrust)(
    HWND hWnd,
    GUID *pgActionID,
    LPVOID pWVTData
    );

typedef ULONG (WINAPI *_CertNameToStr)(
    ULONG dwCertEncodingType,
    PCERT_NAME_BLOB pName,
    ULONG dwStrType,
    LPTSTR psz,
    ULONG csz
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
