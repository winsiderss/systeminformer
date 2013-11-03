#ifndef _PH_VERIFY_H
#define _PH_VERIFY_H

#include <wintrust.h>
#include <softpub.h>

#define PH_VERIFY_DEFAULT_SIZE_LIMIT (32 * 1024 * 1024)

#define PH_VERIFY_PREVENT_NETWORK_ACCESS 0x1
#define PH_VERIFY_VIEW_PROPERTIES 0x2

typedef struct _PH_VERIFY_FILE_INFO
{
    PWSTR FileName;
    ULONG Flags;

    ULONG FileSizeLimitForHash; // 0 for PH_VERIFY_DEFAULT_SIZE_LIMIT, -1 for unlimited
    ULONG NumberOfCatalogFileNames;
    PWSTR *CatalogFileNames;

    HWND hWnd; // for PH_VERIFY_VIEW_PROPERTIES
} PH_VERIFY_FILE_INFO, *PPH_VERIFY_FILE_INFO;

NTSTATUS PhVerifyFileEx(
    __in PPH_VERIFY_FILE_INFO Information,
    __out VERIFY_RESULT *VerifyResult,
    __out_opt PCERT_CONTEXT **Signatures,
    __out_opt PULONG NumberOfSignatures
    );

VOID PhFreeVerifySignatures(
    __in PCERT_CONTEXT *Signatures,
    __in ULONG NumberOfSignatures
    );

PPH_STRING PhGetSignerNameFromCertificate(
    __in PCERT_CONTEXT Certificate
    );

#endif
