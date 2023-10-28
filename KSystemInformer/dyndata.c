/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2022
 *
 */

#include <kph.h>
#include <kphdyndata.h>

#include <trace.h>

typedef union _KPH_DYN_ATOMIC
{
    PKPH_DYN Dyn;
    KPH_ATOMIC_OBJECT_REF Atomic;
} KPH_DYN_ATOMIC, *PKPH_DYN_ATOMIC;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpDynDataTypeName = RTL_CONSTANT_STRING(L"KphDynData");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphDynDataType = NULL;
KPH_PROTECTED_DATA_SECTION_POP();
static KPH_DYN_ATOMIC KphpDynData = { .Atomic = KPH_ATOMIC_OBJECT_REF_INIT };

/**
 * \brief Reference the dynamic configuration.
 *
 * \return Pointer to the dynamic configuration, NULL if not activated.
 */
_Must_inspect_result_
PKPH_DYN KphReferenceDynData(
    VOID
    )
{
    return KphAtomicReferenceObject(&KphpDynData.Atomic);
}

/**
 * \brief Frees a dynamic configuration object.
 *
 * \param[in] Object Dynamic configuration object to free.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
VOID KSIAPI KphpFreeDynData(
    _In_freesMem_ PVOID Object
    )
{
    KphFree(Object, KPH_TAG_DYNDATA);
}

PAGED_FILE();

/**
 * \brief Allocates a dynamic configuration object.
 *
 * \param[in] Size The size of the object to allocate.
 *
 * \return Pointer to the allocated object, NULL on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateDynData(
    _In_ SIZE_T Size
    )
{
    PAGED_CODE_PASSIVE();

    return KphAllocateNPaged(Size, KPH_TAG_DYNDATA);
}

/**
 * \brief Initializes a dynamic configuration object.
 *
 * \param[in] Object Dynamic configuration object to initialize.
 * \param[in] Parameter Dynamic configuration to load into the object.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitializeDynData(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_DYN dyn;
    PKPH_DYN_CONFIGURATION config;

    PAGED_CODE_PASSIVE();

    NT_ASSERT(Parameter);

    dyn = Object;
    config = Parameter;

#define KPH_LOAD_DYNITEM(x) dyn->##x = C_2sTo4(config->##x)

    KPH_LOAD_DYNITEM(MajorVersion);
    KPH_LOAD_DYNITEM(MinorVersion);
    KPH_LOAD_DYNITEM(BuildNumberMin);
    KPH_LOAD_DYNITEM(RevisionMin);
    KPH_LOAD_DYNITEM(BuildNumberMax);
    KPH_LOAD_DYNITEM(BuildNumberMin);

    KPH_LOAD_DYNITEM(EgeGuid);
    KPH_LOAD_DYNITEM(EpObjectTable);
    KPH_LOAD_DYNITEM(EreGuidEntry);
    KPH_LOAD_DYNITEM(HtHandleContentionEvent);
    KPH_LOAD_DYNITEM(OtName);
    KPH_LOAD_DYNITEM(OtIndex);
    KPH_LOAD_DYNITEM(ObDecodeShift);
    KPH_LOAD_DYNITEM(ObAttributesShift);

    if (config->CiVersion == KPH_DYN_CI_V1)
    {
        dyn->CiFreePolicyInfo = (PCI_FREE_POLICY_INFO)KphGetRoutineAddress(L"ci.dll", "CiFreePolicyInfo");
        dyn->CiCheckSignedFile = (PCI_CHECK_SIGNED_FILE)KphGetRoutineAddress(L"ci.dll", "CiCheckSignedFile");
        dyn->CiVerifyHashInCatalog = (PCI_VERIFY_HASH_IN_CATALOG)KphGetRoutineAddress(L"ci.dll", "CiVerifyHashInCatalog");
    }
    else if (config->CiVersion == KPH_DYN_CI_V2)
    {
        dyn->CiFreePolicyInfo = (PCI_FREE_POLICY_INFO)KphGetRoutineAddress(L"ci.dll", "CiFreePolicyInfo");
        dyn->CiCheckSignedFileEx = (PCI_CHECK_SIGNED_FILE_EX)KphGetRoutineAddress(L"ci.dll", "CiCheckSignedFile");
        dyn->CiVerifyHashInCatalogEx = (PCI_VERIFY_HASH_IN_CATALOG_EX)KphGetRoutineAddress(L"ci.dll", "CiVerifyHashInCatalog");
    }

    if (config->LxVersion == KPH_DYN_LX_V1)
    {
        dyn->LxpThreadGetCurrent = (PLXP_THREAD_GET_CURRENT)KphGetRoutineAddress(L"lxcore.sys", "LxpThreadGetCurrent");
    }

    KPH_LOAD_DYNITEM(AlpcCommunicationInfo);
    KPH_LOAD_DYNITEM(AlpcOwnerProcess);
    KPH_LOAD_DYNITEM(AlpcConnectionPort);
    KPH_LOAD_DYNITEM(AlpcServerCommunicationPort);
    KPH_LOAD_DYNITEM(AlpcClientCommunicationPort);
    KPH_LOAD_DYNITEM(AlpcHandleTable);
    KPH_LOAD_DYNITEM(AlpcHandleTableLock);
    KPH_LOAD_DYNITEM(AlpcAttributes);
    KPH_LOAD_DYNITEM(AlpcAttributesFlags);
    KPH_LOAD_DYNITEM(AlpcPortContext);
    KPH_LOAD_DYNITEM(AlpcPortObjectLock);
    KPH_LOAD_DYNITEM(AlpcSequenceNo);
    KPH_LOAD_DYNITEM(AlpcState);
    KPH_LOAD_DYNITEM(KtReadOperationCount);
    KPH_LOAD_DYNITEM(KtWriteOperationCount);
    KPH_LOAD_DYNITEM(KtOtherOperationCount);
    KPH_LOAD_DYNITEM(KtReadTransferCount);
    KPH_LOAD_DYNITEM(KtWriteTransferCount);
    KPH_LOAD_DYNITEM(KtOtherTransferCount);
    KPH_LOAD_DYNITEM(LxPicoProc);
    KPH_LOAD_DYNITEM(LxPicoProcInfo);
    KPH_LOAD_DYNITEM(LxPicoProcInfoPID);
    KPH_LOAD_DYNITEM(LxPicoThrdInfo);
    KPH_LOAD_DYNITEM(LxPicoThrdInfoTID);
    KPH_LOAD_DYNITEM(MmSectionControlArea);
    KPH_LOAD_DYNITEM(MmControlAreaListHead);
    KPH_LOAD_DYNITEM(MmControlAreaLock);

    return STATUS_SUCCESS;
}

/**
 * \brief Activates dynamic data.
 *
 * \param[in] DynData The dynamic data to activate.
 * \param[in] DynDataLength The length of the dynamic data.
 * \param[in] Signature The signature of the dynamic data.
 * \param[in] SignatureLength The length of the signature.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpActivateDynData(
    _In_ PBYTE DynData,
    _In_ ULONG DynDataLength,
    _In_opt_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;
    PKPH_DYN_CONFIGURATION config;

    PAGED_CODE_PASSIVE();

    dyn = NULL;

    if (Signature)
    {
        status = KphVerifyBuffer(DynData,
                                 DynDataLength,
                                 Signature,
                                 SignatureLength);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "KphVerifyBuffer failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    status = KphDynDataGetConfiguration((PKPH_DYNDATA)DynData,
                                        DynDataLength,
                                        KphKernelVersion.MajorVersion,
                                        KphKernelVersion.MinorVersion,
                                        KphKernelVersion.BuildNumber,
                                        KphKernelVersion.Revision,
                                        &config);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphDynDataGetConfiguration failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphCreateObject(KphDynDataType, sizeof(KPH_DYN), &dyn, config);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphAtomicAssignObjectReference(&KphpDynData.Atomic, dyn);

    KphTracePrint(TRACE_LEVEL_INFORMATION,
                  GENERAL,
                  "Activated Dynamic Configuration "
                  "(%hu.%hu.%hu.%hu - %hu.%hu.%hu.%hu) "
                  "for Windows %lu.%lu.%lu Kernel %hu.%hu.%hu.%hu",
                  dyn->MajorVersion,
                  dyn->MinorVersion,
                  dyn->BuildNumberMin,
                  dyn->RevisionMin,
                  dyn->MajorVersion,
                  dyn->MinorVersion,
                  dyn->BuildNumberMax,
                  dyn->RevisionMax,
                  KphOsVersionInfo.dwMajorVersion,
                  KphOsVersionInfo.dwMinorVersion,
                  KphOsVersionInfo.dwBuildNumber,
                  KphKernelVersion.MajorVersion,
                  KphKernelVersion.MinorVersion,
                  KphKernelVersion.BuildNumber,
                  KphKernelVersion.Revision);

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    return status;
}

/**
 * \brief Activates dynamic data.
 *
 * \param[in] DynData The dynamic data to activate.
 * \param[in] DynDataLength The length of the dynamic data.
 * \param[in] Signature The signature of the dynamic data.
 * \param[in] SignatureLength The length of the signature.
 * \param[in] AccessMode The access mode of the caller.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphActivateDynData(
    _In_ PBYTE DynData,
    _In_ ULONG DynDataLength,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PBYTE dynData;
    PBYTE signature;

    PAGED_CODE_PASSIVE();

    dynData = NULL;
    signature = NULL;

    if (!DynData || !DynDataLength || !Signature || !SignatureLength)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        dynData = KphAllocatePaged(DynDataLength, KPH_TAG_DYNDATA);
        if (!dynData)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        signature = KphAllocatePaged(SignatureLength, KPH_TAG_DYNDATA);
        if (!signature)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        __try
        {
            ProbeForRead(DynData, DynDataLength, 1);
            RtlCopyMemory(dynData, DynData, DynDataLength);

            ProbeForRead(Signature, DynDataLength, 1);
            RtlCopyMemory(signature, Signature, SignatureLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        dynData = DynData;
        signature = Signature;
    }

    status = KphpActivateDynData(dynData,
                                 DynDataLength,
                                 signature,
                                 SignatureLength);

Exit:

    if (dynData && (dynData != DynData))
    {
        KphFree(dynData, KPH_TAG_DYNDATA);
    }

    if (signature && (signature != DynData))
    {
        KphFree(signature, KPH_TAG_DYNDATA);
    }

    return status;
}

/**
 * \brief Initializes the dynamic data infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeDynData(
    VOID
    )
{
    NTSTATUS status;
    KPH_OBJECT_TYPE_INFO typeInfo;

    PAGED_CODE_PASSIVE();

    typeInfo.Allocate = KphpAllocateDynData;
    typeInfo.Initialize = KphpInitializeDynData;
    typeInfo.Delete = NULL;
    typeInfo.Free = KphpFreeDynData;

    KphCreateObjectType(&KphpDynDataTypeName, &typeInfo, &KphDynDataType);

    if (KphParameterFlags.DynDataNoEmbedded)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Embedded Dynamic Configuration disabled");
    }
    else
    {
        status = KphpActivateDynData((PBYTE)KphDynData,
                                     KphDynDataLength,
                                     NULL,
                                     0);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Embedded Dynamic Configuration: %!STATUS!",
                      status);
    }
}

/**
 * \brief Cleans up the dynamic data infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupDynData(
    VOID
    )
{
    PAGED_CODE_PASSIVE();

    KphAtomicAssignObjectReference(&KphpDynData.Atomic, NULL);
}
