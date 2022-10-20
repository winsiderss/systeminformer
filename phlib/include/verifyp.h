#ifndef _PH_VERIFYP_H
#define _PH_VERIFYP_H

typedef struct _PH_VERIFY_CACHE_ENTRY
{
    PPH_STRING FileName;
    ULONGLONG SequenceNumber;
    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;
} PH_VERIFY_CACHE_ENTRY, *PPH_VERIFY_CACHE_ENTRY;

BOOLEAN PhpVerifyCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG PhpVerifyCacheHashtableHashFunction(
    _In_ PVOID Entry
    );

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
    PVOID rgPropSheetPages;
} CRYPTUI_VIEWSIGNERINFO_STRUCT, *PCRYPTUI_VIEWSIGNERINFO_STRUCT;

typedef BOOL (WINAPI *_CryptCATAdminCalcHashFromFileHandle)(
    _In_ HANDLE hFile,
    _Out_ PULONG pcbHash,
    _Out_ PBYTE pbHash,
    _In_ ULONG dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminCalcHashFromFileHandle2)(
    _In_ HCATADMIN hCatAdmin,
    _In_ HANDLE hFile,
    _Out_ PULONG pcbHash,
    _Out_ PBYTE pbHash,
    _In_ ULONG dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminAcquireContext)(
    _Out_ HCATADMIN *phCatAdmin,
    _In_ PGUID pgSubsystem,
    _In_ ULONG dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminAcquireContext2)(
    _Out_ HCATADMIN *phCatAdmin,
    _In_ PGUID pgSubsystem,
    _In_ PCWSTR pwszHashAlgorithm,
    _In_opt_ PCCERT_STRONG_SIGN_PARA pStrongHashPolicy,
    _In_ ULONG dwFlags
    );

typedef HANDLE (WINAPI *_CryptCATAdminEnumCatalogFromHash)(
    _In_ HANDLE hCatAdmin,
    _Out_ PBYTE pbHash,
    _In_ ULONG cbHash,
    _In_ ULONG dwFlags,
    _Out_opt_ PHANDLE phPrevCatInfo
    );

typedef BOOL (WINAPI *_CryptCATCatalogInfoFromContext)(
    _In_ HANDLE hCatInfo,
    _Out_ CATALOG_INFO *psCatInfo,
    _In_ ULONG dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminReleaseCatalogContext)(
    _In_ HANDLE hCatAdmin,
    _In_ HANDLE hCatInfo,
    _In_ ULONG dwFlags
    );

typedef BOOL (WINAPI *_CryptCATAdminReleaseContext)(
    _In_ HANDLE hCatAdmin,
    _In_ ULONG dwFlags
    );

typedef PCRYPT_PROVIDER_DATA (WINAPI *_WTHelperProvDataFromStateData)(
    _In_ HANDLE hStateData
    );

typedef PCRYPT_PROVIDER_SGNR (WINAPI *_WTHelperGetProvSignerFromChain)(
    _In_ CRYPT_PROVIDER_DATA *pProvData,
    _In_ ULONG idxSigner,
    _In_ BOOL fCounterSigner,
    _In_ ULONG idxCounterSigner
    );

typedef LONG (WINAPI *_WinVerifyTrust)(
    _In_ HWND hWnd,
    _In_ GUID *pgActionID,
    _In_ PVOID pWVTData
    );

typedef VOID (WINAPI* _WintrustSetDefaultIncludePEPageHashes)(
    _In_ BOOL fIncludePEPageHashes
    );

typedef ULONG (WINAPI *_CertNameToStr)(
    _In_ ULONG dwCertEncodingType,
    _In_ PCERT_NAME_BLOB pName,
    _In_ ULONG dwStrType,
    _Out_ PWSTR psz,
    _In_ ULONG csz
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

typedef enum _SIGNATURE_STATE
{
    SIGNATURE_STATE_UNSIGNED_MISSING,
    SIGNATURE_STATE_UNSIGNED_UNSUPPORTED,
    SIGNATURE_STATE_UNSIGNED_POLICY,
    SIGNATURE_STATE_INVALID_CORRUPT,
    SIGNATURE_STATE_INVALID_POLICY,
    SIGNATURE_STATE_VALID,
    SIGNATURE_STATE_TRUSTED,
    SIGNATURE_STATE_UNTRUSTED
} SIGNATURE_STATE;

typedef enum _SIGNATURE_INFO_TYPE
{
    SIT_UNKNOWN,
    SIT_AUTHENTICODE,
    SIT_CATALOG
} SIGNATURE_INFO_TYPE;

typedef enum _SIGNATURE_INFO_FLAGS
{
    SIF_NONE = 0,
    SIF_AUTHENTICODE_SIGNED = 1,
    SIF_CATALOG_SIGNED = 2,
    SIF_VERSION_INFO = 4,
    SIF_CHECK_OS_BINARY = 0x800,
    SIF_BASE_VERIFICATION = 0x1000,
    SIF_CATALOG_FIRST = 0x2000,
    SIF_MOTW = 0x4000
} SIGNATURE_INFO_FLAGS;

typedef struct _SIGNATURE_INFO
{
    ULONG cbSize;
    SIGNATURE_STATE nSignatureState;
    SIGNATURE_INFO_TYPE nSignatureType;
    ULONG dwSignatureInfoAvailability;
    ULONG dwInfoAvailability;
    PWSTR pszDisplayName;
    ULONG cchDisplayName;
    PWSTR pszPublisherName;
    ULONG cchPublisherName;
    PWSTR pszMoreInfoURL;
    ULONG cchMoreInfoURL;
    PBYTE prgbHash;
    ULONG cbHash;
    BOOL fOSBinary;
} SIGNATURE_INFO, *PSIGNATURE_INFO;

typedef HRESULT (WINAPI* _WTGetSignatureInfo)(
    _In_opt_ PCWSTR pszFile,
    _In_opt_ HANDLE hFile,
    _In_ SIGNATURE_INFO_FLAGS sigInfoFlags,
    _Inout_ PSIGNATURE_INFO psiginfo,
    _Out_ PVOID ppCertContext,
    _Out_ PHANDLE phWVTStateData
    );

#endif
