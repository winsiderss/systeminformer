/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2013-2016
 *     dmex    2017-2023
 *
 */

#ifndef _PH_VERIFY_H
#define _PH_VERIFY_H

EXTERN_C_START

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

#define PH_VERIFY_UNTRUSTED(x) (x != VrUnknown && x != VrTrusted) 

PHLIBAPI
PPH_STRINGREF
NTAPI
PhVerifyResultToString(
    _In_ VERIFY_RESULT Result
    );

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

typedef struct _CERT_CONTEXT CERT_CONTEXT;
typedef CERT_CONTEXT *PCERT_CONTEXT;

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

PHLIBAPI
BOOLEAN
NTAPI
PhGetSystemComponentFromCertificate(
    _In_ PCERT_CONTEXT Certificate
    );

EXTERN_C_END

#endif
