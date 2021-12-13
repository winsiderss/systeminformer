#ifndef _PH_VERIFY_H
#define _PH_VERIFY_H

#include <wintrust.h>
#include <softpub.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PH_VERIFY_DEFAULT_SIZE_LIMIT (32 * 1024 * 1024)

typedef enum _VERIFY_RESULT
{
    VrUnknown = 0,
    VrNoSignature,
    VrTrusted,
    VrExpired,
    VrRevoked,
    VrDistrust,
    VrSecuritySettings,
    VrBadSignature
} VERIFY_RESULT, *PVERIFY_RESULT;

#define PH_VERIFY_PREVENT_NETWORK_ACCESS 0x1
#define PH_VERIFY_VIEW_PROPERTIES 0x2

typedef struct _PH_VERIFY_FILE_INFO
{
    HANDLE FileHandle;
    ULONG Flags; // PH_VERIFY_*

    ULONG FileSizeLimitForHash; // 0 for PH_VERIFY_DEFAULT_SIZE_LIMIT, -1 for unlimited
    ULONG NumberOfCatalogFileNames;
    PWSTR *CatalogFileNames;

    HWND hWnd; // for PH_VERIFY_VIEW_PROPERTIES
} PH_VERIFY_FILE_INFO, *PPH_VERIFY_FILE_INFO;

PHLIBAPI
VERIFY_RESULT
NTAPI
PhVerifyFile(
    _In_ PWSTR FileName,
    _Out_opt_ PPH_STRING *SignerName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhVerifyFileEx(
    _In_ PPH_VERIFY_FILE_INFO Information,
    _Out_ VERIFY_RESULT *VerifyResult,
    _Out_opt_ PCERT_CONTEXT **Signatures,
    _Out_opt_ PULONG NumberOfSignatures
    );

PHLIBAPI
VOID
NTAPI
PhFreeVerifySignatures(
    _In_opt_ PCERT_CONTEXT *Signatures,
    _In_ ULONG NumberOfSignatures
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetSignerNameFromCertificate(
    _In_ PCERT_CONTEXT Certificate
    );

#ifdef __cplusplus
}
#endif

#endif
