/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kph.h>

#include <trace.h>

typedef struct _KPH_SIGNING_INFRASTRUCTURE
{
    KPH_AUTHENTICODE_INFO HalAuthenticode;
    volatile SIZE_T CatalogsAreLoadedCalls;
} KPH_SIGNING_INFRASTRUCTURE, *PKPH_SIGNING_INFRASTRUCTURE;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpSigningInfraName = RTL_CONSTANT_STRING(L"KphSigningInfrastructure");
static const UNICODE_STRING KphpHalFileName = RTL_CONSTANT_STRING(L"\\SystemRoot\\System32\\hal.dll");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_SIGNING_INFRASTRUCTURE KphpSigningInfra = NULL;
static PKPH_OBJECT_TYPE KphpSigningInfraType = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

PAGED_FILE();

/**
 * \brief Allocates signing infrastructure object.
 *
 * \param[in] Size The size to allocate.
 *
 * \return Allocated signing infrastructure object, null on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateSigningInfra(
    _In_ SIZE_T Size
    )
{
    PAGED_CODE();

    return KphAllocatePaged(Size, KPH_TAG_SIGNING_INFRA);
}

/**
 * \brief Initializes signing infrastructure.
 *
 * \param[in,out] Object The signing infrastructure to initialize.
 * \param[in] Parameter Unused
 *
 * \return Successful or errant status.
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitSigningInfra(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_SIGNING_INFRASTRUCTURE infra;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Parameter);

    infra = Object;

    return KphGetAuthenticodeInfoByFileName(&KphpHalFileName,
                                            &infra->HalAuthenticode);
}

/**
 * \brief Deletes signing infrastructure.
 *
 * \param[in,out] Object The signing infrastructure to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
VOID KSIAPI KphpDeleteSigningInfra(
    _Inout_ PVOID Object
    )
{
    PKPH_SIGNING_INFRASTRUCTURE infra;

    PAGED_CODE();

    infra = Object;

    KphFreeAuthenticodeInfo(&infra->HalAuthenticode);
}

/**
 * \brief Frees signing infrastructure object.
 *
 * \param[in] Object The object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
VOID KSIAPI KphpFreeSigningInfra(
    _In_freesMem_ PVOID Object
    )
{
    PAGED_CODE();

    KphFree(Object, KPH_TAG_SIGNING_INFRA);
}

/**
 * \brief Internal function resetting signing info between calls.
 *
 * \param[in] Dyn Dynamic configuration.
 * \param[in] PolicyInfo The policy info to reset.
 * \param[in] SigningTime The signing time to reset.
 * \param[in] TimeStampPolicyInfo The time stamp policy info to reset.
 * \param[in] TimeStampPolicyInfo The time stamp policy info to reset.
 * \param[in] CatalogName The catalog name to reset.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpResetSigningInfo(
    _In_ PKPH_DYN Dyn,
    _Inout_opt_ PMINCRYPT_POLICY_INFO PolicyInfo,
    _Inout_opt_ PLARGE_INTEGER SigningTime,
    _Inout_opt_ PMINCRYPT_POLICY_INFO TimeStampPolicyInfo,
    _Inout_opt_ PUNICODE_STRING CatalogName
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(Dyn->CiFreePolicyInfo);

    if (PolicyInfo)
    {
        Dyn->CiFreePolicyInfo(PolicyInfo);
        RtlZeroMemory(PolicyInfo, sizeof(*PolicyInfo));
    }

    if (TimeStampPolicyInfo)
    {
        Dyn->CiFreePolicyInfo(TimeStampPolicyInfo);
        RtlZeroMemory(TimeStampPolicyInfo, sizeof(*TimeStampPolicyInfo));
    }

    if (SigningTime)
    {
        SigningTime->QuadPart = 0;
    }

    if (CatalogName)
    {
        RtlFreeUnicodeString(CatalogName);
        RtlZeroMemory(CatalogName, sizeof(*CatalogName));
    }
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpCiCheckSignedFile(
    _In_ PKPH_DYN Dyn,
    _In_ PKPH_AUTHENTICODE_INFO Info,
    _Inout_opt_ PMINCRYPT_POLICY_INFO PolicyInfo,
    _Out_opt_ PLARGE_INTEGER SigningTime,
    _Inout_opt_ PMINCRYPT_POLICY_INFO TimeStampPolicyInfo
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(Dyn->CiFreePolicyInfo);
    NT_ASSERT(Info->Signature);
    NT_ASSERT(Info->SignatureSize > 0);

    if (Dyn->CiCheckSignedFileEx)
    {
        NTSTATUS status;

        status = Dyn->CiCheckSignedFileEx(Info->SHA256,
                                          ARRAYSIZE(Info->SHA256),
                                          CALG_SHA_256,
                                          Info->Signature,
                                          Info->SignatureSize,
                                          PolicyInfo,
                                          SigningTime,
                                          TimeStampPolicyInfo);
        if (status == STATUS_INVALID_IMAGE_HASH)
        {
            KphpResetSigningInfo(Dyn,
                                 PolicyInfo,
                                 SigningTime,
                                 TimeStampPolicyInfo,
                                 NULL);

            status = Dyn->CiCheckSignedFileEx(Info->SHA1,
                                              ARRAYSIZE(Info->SHA1),
                                              CALG_SHA1,
                                              Info->Signature,
                                              Info->SignatureSize,
                                              PolicyInfo,
                                              SigningTime,
                                              TimeStampPolicyInfo);
        }

        return status;
    }

    if (Dyn->CiCheckSignedFile)
    {
        return Dyn->CiCheckSignedFile(Info->SHA1,
                                      Info->Signature,
                                      Info->SignatureSize,
                                      PolicyInfo,
                                      SigningTime,
                                      TimeStampPolicyInfo);
    }

    return STATUS_NOINTERFACE;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpCiVerifyHashInCatalog(
    _In_ PKPH_DYN Dyn,
    _In_ PKPH_AUTHENTICODE_INFO Info,
    _In_ BOOLEAN ReloadCatalogs,
    _In_ BOOLEAN SecureProcess,
    _In_ ULONG AcceptRoots,
    _Inout_opt_ PMINCRYPT_POLICY_INFO PolicyInfo,
    _Out_opt_ PUNICODE_STRING CatalogName,
    _Out_opt_ PLARGE_INTEGER SigningTime,
    _Inout_opt_ PMINCRYPT_POLICY_INFO TimeStampPolicyInfo
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(Dyn->CiFreePolicyInfo);

    if (Dyn->CiVerifyHashInCatalogEx)
    {
        NTSTATUS status;

        status = Dyn->CiVerifyHashInCatalogEx(Info->SHA256,
                                              ARRAYSIZE(Info->SHA256),
                                              CALG_SHA_256,
                                              ReloadCatalogs,
                                              SecureProcess,
                                              AcceptRoots,
                                              PolicyInfo,
                                              CatalogName,
                                              SigningTime,
                                              TimeStampPolicyInfo);
        if (status == STATUS_INVALID_IMAGE_HASH)
        {
            KphpResetSigningInfo(Dyn,
                                 PolicyInfo,
                                 SigningTime,
                                 TimeStampPolicyInfo,
                                 CatalogName);

            status = Dyn->CiVerifyHashInCatalogEx(Info->SHA1,
                                                  ARRAYSIZE(Info->SHA1),
                                                  CALG_SHA1,
                                                  ReloadCatalogs,
                                                  SecureProcess,
                                                  AcceptRoots,
                                                  PolicyInfo,
                                                  CatalogName,
                                                  SigningTime,
                                                  TimeStampPolicyInfo);
        }

        return status;
    }

    if (Dyn->CiVerifyHashInCatalog)
    {
        return Dyn->CiVerifyHashInCatalog(Info->SHA1,
                                          ReloadCatalogs,
                                          SecureProcess,
                                          AcceptRoots,
                                          PolicyInfo,
                                          CatalogName,
                                          SigningTime,
                                          TimeStampPolicyInfo);
    }

    return STATUS_NOINTERFACE;
}

/**
 * \brief Initializes signing infrastructure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeSigning(
    VOID
    )
{
    NTSTATUS status;
    KPH_OBJECT_TYPE_INFO typeInfo;

    PAGED_CODE_PASSIVE();

    typeInfo.Allocate = KphpAllocateSigningInfra;
    typeInfo.Initialize = KphpInitSigningInfra;
    typeInfo.Delete = KphpDeleteSigningInfra;
    typeInfo.Free = KphpFreeSigningInfra;

    KphCreateObjectType(&KphpSigningInfraName,
                        &typeInfo,
                        &KphpSigningInfraType);

    status = KphCreateObject(KphpSigningInfraType,
                             sizeof(KPH_SIGNING_INFRASTRUCTURE),
                             &KphpSigningInfra,
                             NULL);
    if (!NT_SUCCESS(status))
    {
        KphpSigningInfra = NULL;
    }

    return status;
}

/**
 * \brief Cleans up signing infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupSigning(
    VOID
    )
{
    PAGED_CODE_PASSIVE();

    if (KphpSigningInfra)
    {
        KphDereferenceObject(KphpSigningInfra);
    }
}

/**
 * \brief References the signing infrastructure.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphReferenceSigningInfrastructure(
    VOID
    )
{
    PAGED_CODE();

    NT_ASSERT(KphpSigningInfra);

    KphReferenceHashingInfrastructure();
    KphReferenceObject(KphpSigningInfra);
}

/**
 * \brief Dereferences the signing infrastructure.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphDereferenceSigningInfrastructure(
    VOID
    )
{
    PAGED_CODE();

    NT_ASSERT(KphpSigningInfra);

    KphDereferenceObject(KphpSigningInfra);
    KphDereferenceHashingInfrastructure();
}

/**
 * \brief Determines if the signing catalogs are loaded.
 *
 * \param[in] Dyn Dynamic configuration.
 *
 * \return TRUE if they are loaded, FALSE otherwise.
 */
BOOLEAN KphpSigningCatalogsAreLoaded(
    _In_ PKPH_DYN Dyn
    )
{
    NTSTATUS status;
    MINCRYPT_POLICY_INFO policyInfo;
    MINCRYPT_POLICY_INFO timeStampPolicyInfo;

    PAGED_CODE_PASSIVE();

    NT_ASSERT(Dyn->CiFreePolicyInfo);
    NT_ASSERT(KphpSigningInfra);

    if (InterlockedIncrementSizeT(&KphpSigningInfra->CatalogsAreLoadedCalls) == 1)
    {
        //
        // Force false on the first time through here.
        //
        return FALSE;
    }

    RtlZeroMemory(&policyInfo, sizeof(policyInfo));
    RtlZeroMemory(&timeStampPolicyInfo, sizeof(timeStampPolicyInfo));

    status = KphpCiVerifyHashInCatalog(Dyn,
                                       &KphpSigningInfra->HalAuthenticode,
                                       FALSE,
                                       FALSE,
                                       0xffffffff,
                                       &policyInfo,
                                       NULL,
                                       NULL,
                                       &timeStampPolicyInfo);

    Dyn->CiFreePolicyInfo(&policyInfo);
    Dyn->CiFreePolicyInfo(&timeStampPolicyInfo);

    return NT_SUCCESS(status);
}

/**
 * \brief Populated signing information with info from code integrity.
 *
 * \param[in] Dyn Dynamic configuration.
 * \param[in,out] Info Signing info to populate.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpPopulateCiInfo(
    _In_ PKPH_DYN Dyn,
    _Inout_ PKPH_SIGNING_INFO Info
    )
{
    NTSTATUS status;

    PAGED_CODE_PASSIVE();

    NT_ASSERT(Dyn->CiFreePolicyInfo);

    if (Info->Authenticode.Signature)
    {
        NT_ASSERT(Info->Authenticode.SignatureSize > 0);

        status = KphpCiCheckSignedFile(Dyn,
                                       &Info->Authenticode,
                                       &Info->PolicyInfo,
                                       &Info->SigningTime,
                                       &Info->TimeStampPolicyInfo);
    }
    else
    {
        status = KphpCiVerifyHashInCatalog(Dyn,
                                           &Info->Authenticode,
                                           FALSE,
                                           FALSE,
                                           0xffffffff,
                                           &Info->PolicyInfo,
                                           &Info->CatalogName,
                                           &Info->SigningTime,
                                           &Info->TimeStampPolicyInfo);
        if ((status == STATUS_INVALID_IMAGE_HASH) &&
            !KphpSigningCatalogsAreLoaded(Dyn))
        {
            //
            // Try again with ReloadCatalogs TRUE.
            //

            KphpResetSigningInfo(Dyn,
                                 &Info->PolicyInfo,
                                 &Info->SigningTime,
                                 &Info->TimeStampPolicyInfo,
                                 &Info->CatalogName);

            status = KphpCiVerifyHashInCatalog(Dyn,
                                               &Info->Authenticode,
                                               TRUE,
                                               FALSE,
                                               0xffffffff,
                                               &Info->PolicyInfo,
                                               &Info->CatalogName,
                                               &Info->SigningTime,
                                               &Info->TimeStampPolicyInfo);
        }
    }

    //
    // Callers of these APIs are required to *always* free the returned info
    // regardless of the return status.
    //
    Info->Dyn = Dyn;
    KphReferenceObject(Info->Dyn);

    return status;
}

/**
 * \brief Retrieves the signing information about a file, if any. The caller
 * *must always* free the returned information to KphFreeSigningInfo regardless
 * of the return status. The signing information may be partially-filled.
 *
 * \param[in] FileHandle File handle to the file to signing info signing of.
 * \param[out] Info Signing information output.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetSigningInfo(
    _In_ HANDLE FileHandle,
    _Out_allocatesMem_ PKPH_SIGNING_INFO Info
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;

    PAGED_CODE_PASSIVE();

    RtlZeroMemory(Info, sizeof(*Info));

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->CiFreePolicyInfo)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = KphGetAuthenticodeInfo(FileHandle, &Info->Authenticode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = KphpPopulateCiInfo(dyn, Info);

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    return status;
}

/**
 * \brief Retrieves the signing information about a file, if any. The caller
 * *must always* free the returned information to KphFreeSigningInfo regardless
 * of the return status. The signing information may be partially-filled.
 *
 * \param[in] FileName File name of the file to signing info signing of.
 * \param[out] Info Signing information output.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetSigningInfoByFileName(
    _In_ PCUNICODE_STRING FileName,
    _Out_allocatesMem_ PKPH_SIGNING_INFO Info
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;

    PAGED_CODE_PASSIVE();

    RtlZeroMemory(Info, sizeof(*Info));

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->CiFreePolicyInfo)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = KphGetAuthenticodeInfoByFileName(FileName, &Info->Authenticode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = KphpPopulateCiInfo(dyn, Info);

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    return status;
}

/**
 * \brief Frees signing information.
 *
 * \param[in] Info Signing information to free.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphFreeSigningInfo(
    _In_freesMem_ PKPH_SIGNING_INFO Info
    )
{
    PAGED_CODE_PASSIVE();

    RtlFreeUnicodeString(&Info->CatalogName);

    if (Info->Dyn)
    {
        if (Info->Dyn->CiFreePolicyInfo)
        {
            Info->Dyn->CiFreePolicyInfo(&Info->PolicyInfo);
            Info->Dyn->CiFreePolicyInfo(&Info->TimeStampPolicyInfo);
        }

        KphDereferenceObject(Info->Dyn);
    }

    KphFreeAuthenticodeInfo(&Info->Authenticode);
}
