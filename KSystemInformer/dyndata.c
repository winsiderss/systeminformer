/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2022-2024
 *
 */

#include <kph.h>
#include <kphdyndata.h>

#include <trace.h>

typedef struct _KPH_DYN_INIT
{
    PKPH_DYN_CONFIG Config;
    PKPH_DYN_KERNEL_FIELDS Kernel;
    PKPH_DYN_LXCORE_FIELDS Lxcore;
} KPH_DYN_INIT, *PKPH_DYN_INIT;

typedef union _KPH_DYN_ATOMIC
{
    PKPH_DYN Dyn;
    KPH_ATOMIC_OBJECT_REF Atomic;
} KPH_DYN_ATOMIC, *PKPH_DYN_ATOMIC;

typedef struct _KPH_DYN_MODULE
{
    USHORT Class;
    UNICODE_STRING Name;
    BOOLEAN Valid;
    USHORT Machine;
    ULONG TimeDateStamp;
    ULONG SizeOfImage;
} KPH_DYN_MODULE, *PKPH_DYN_MODULE;

typedef enum _KPH_DYN_MODULE_CLASS
{
    KphDynNtoskrnl,
    KphDynNtkrla57,
    KphDynLxcore,
} KPH_DYN_MODULE_CLASS, *PKPH_DYN_MODULE_CLASS;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpDynDataTypeName = RTL_CONSTANT_STRING(L"KphDynData");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphpDynDataType = NULL;
static KPH_DYN_MODULE KphpDynModules[] =
{
    { KPH_DYN_CLASS_NTOSKRNL, RTL_CONSTANT_STRING(L"ntoskrnl.exe"), FALSE, 0, 0, 0 },
    { KPH_DYN_CLASS_NTKRLA57, RTL_CONSTANT_STRING(L"ntkrla57.exe"), FALSE, 0, 0, 0 },
    { KPH_DYN_CLASS_LXCORE,   RTL_CONSTANT_STRING(L"lxcore.sys"),   FALSE, 0, 0, 0 },
};
KPH_PROTECTED_DATA_SECTION_POP();
static KPH_DYN_ATOMIC KphpDynData = { .Atomic = KPH_ATOMIC_OBJECT_REF_INIT };

/**
 * \brief Reference the dynamic configuration.
 *
 * \return Pointer to the dynamic configuration, NULL if not activated. The
 * caller must eventually dereference the object.
 */
_Must_inspect_result_
PKPH_DYN KphReferenceDynData(
    VOID
    )
{
    return KphAtomicReferenceObject(&KphpDynData.Atomic);
}

KPH_PAGED_FILE();

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
    KPH_PAGED_CODE_PASSIVE();

    return KphAllocateNPaged(Size, KPH_TAG_DYNDATA);
}

/**
 * \brief Initializes a dynamic configuration object.
 *
 * \param[in] Object Dynamic configuration object to initialize.
 * \param[in] Parameter Initialization parameters for dynamic configuration.
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
    NTSTATUS status;
    PKPH_DYN dyn;
    PKPH_DYN_INIT init;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Parameter);

    dyn = Object;
    init = Parameter;

#define KPH_LOAD_DYNITEM_KERNEL(x) dyn->x = C_2sTo4(init->Kernel->x)
#define KPH_LOAD_DYNITEM_LXCORE(x) dyn->x = init->Lxcore ? C_2sTo4(init->Lxcore->x) : ULONG_MAX;

    KPH_LOAD_DYNITEM_KERNEL(EgeGuid);
    KPH_LOAD_DYNITEM_KERNEL(EpObjectTable);
    KPH_LOAD_DYNITEM_KERNEL(EreGuidEntry);
    KPH_LOAD_DYNITEM_KERNEL(HtHandleContentionEvent);
    KPH_LOAD_DYNITEM_KERNEL(OtName);
    KPH_LOAD_DYNITEM_KERNEL(OtIndex);
    KPH_LOAD_DYNITEM_KERNEL(ObDecodeShift);
    KPH_LOAD_DYNITEM_KERNEL(ObAttributesShift);
    KPH_LOAD_DYNITEM_KERNEL(AlpcCommunicationInfo);
    KPH_LOAD_DYNITEM_KERNEL(AlpcOwnerProcess);
    KPH_LOAD_DYNITEM_KERNEL(AlpcConnectionPort);
    KPH_LOAD_DYNITEM_KERNEL(AlpcServerCommunicationPort);
    KPH_LOAD_DYNITEM_KERNEL(AlpcClientCommunicationPort);
    KPH_LOAD_DYNITEM_KERNEL(AlpcHandleTable);
    KPH_LOAD_DYNITEM_KERNEL(AlpcHandleTableLock);
    KPH_LOAD_DYNITEM_KERNEL(AlpcAttributes);
    KPH_LOAD_DYNITEM_KERNEL(AlpcAttributesFlags);
    KPH_LOAD_DYNITEM_KERNEL(AlpcPortContext);
    KPH_LOAD_DYNITEM_KERNEL(AlpcPortObjectLock);
    KPH_LOAD_DYNITEM_KERNEL(AlpcSequenceNo);
    KPH_LOAD_DYNITEM_KERNEL(AlpcState);
    KPH_LOAD_DYNITEM_KERNEL(KtInitialStack);
    KPH_LOAD_DYNITEM_KERNEL(KtStackLimit);
    KPH_LOAD_DYNITEM_KERNEL(KtStackBase);
    KPH_LOAD_DYNITEM_KERNEL(KtKernelStack);
    KPH_LOAD_DYNITEM_KERNEL(KtReadOperationCount);
    KPH_LOAD_DYNITEM_KERNEL(KtWriteOperationCount);
    KPH_LOAD_DYNITEM_KERNEL(KtOtherOperationCount);
    KPH_LOAD_DYNITEM_KERNEL(KtReadTransferCount);
    KPH_LOAD_DYNITEM_KERNEL(KtWriteTransferCount);
    KPH_LOAD_DYNITEM_KERNEL(KtOtherTransferCount);
    KPH_LOAD_DYNITEM_KERNEL(MmSectionControlArea);
    KPH_LOAD_DYNITEM_KERNEL(MmControlAreaListHead);
    KPH_LOAD_DYNITEM_KERNEL(MmControlAreaLock);
    KPH_LOAD_DYNITEM_KERNEL(EpSectionObject);

    KPH_LOAD_DYNITEM_LXCORE(LxPicoProc);
    KPH_LOAD_DYNITEM_LXCORE(LxPicoProcInfo);
    KPH_LOAD_DYNITEM_LXCORE(LxPicoProcInfoPID);
    KPH_LOAD_DYNITEM_LXCORE(LxPicoThrdInfo);
    KPH_LOAD_DYNITEM_LXCORE(LxPicoThrdInfoTID);

    status = KphVerifyCreateKey(&dyn->SessionTokenPublicKeyHandle,
                                init->Config->SessionTokenPublicKey,
                                KPH_DYN_SESSION_TOKEN_PUBLIC_KEY_LENGTH);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphVerifyCreateKey failed: %!STATUS!",
                      status);

        dyn->SessionTokenPublicKeyHandle = NULL;
    }

    return status;
}

/**
 * \brief Deletes a dynamic configuration object.
 *
 * \param[in] Object Dynamic configuration object to delete.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpDeleteDynData(
    _In_freesMem_ PVOID Object
    )
{
    PKPH_DYN dyn;

    KPH_PAGED_CODE_PASSIVE();

    dyn = Object;

    if (dyn->SessionTokenPublicKeyHandle)
    {
        KphVerifyCloseKey(dyn->SessionTokenPublicKeyHandle);
    }
}

/**
 * \brief Frees a dynamic configuration object.
 *
 * \param[in] Object Dynamic configuration object to free.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpFreeDynData(
    _In_freesMem_ PVOID Object
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphFree(Object, KPH_TAG_DYNDATA);
}

/**
 * \brief Activates dynamic data.
 *
 * \param[in] DynConfig The dynamic configuration to activate.
 * \param[in] DynConfigLength The length of the dynamic configuration.
 * \param[in] Signature The signature of the dynamic data.
 * \param[in] SignatureLength The length of the signature.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpActivateDynData(
    _In_ PBYTE DynConfig,
    _In_ ULONG DynConfigLength,
    _In_opt_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;
    KPH_DYN_INIT init;
    PKPH_DYN_KERNEL_FIELDS kernel;
    PKPH_DYN_LXCORE_FIELDS lxcore;

    KPH_PAGED_CODE_PASSIVE();

    dyn = NULL;
    kernel = NULL;
    lxcore = NULL;

    if (Signature)
    {
        status = KphVerifyBuffer(DynConfig,
                                 DynConfigLength,
                                 Signature,
                                 SignatureLength);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "KphVerifyBuffer failed: %!STATUS!",
                          status);

            status = STATUS_SI_DYNDATA_INVALID_SIGNATURE;
            goto Exit;
        }
    }

    if (KphpDynModules[KphDynNtoskrnl].Valid)
    {
        status = KphDynDataLookup((PKPH_DYN_CONFIG)DynConfig,
                                  DynConfigLength,
                                  KphpDynModules[KphDynNtoskrnl].Class,
                                  KphpDynModules[KphDynNtoskrnl].Machine,
                                  KphpDynModules[KphDynNtoskrnl].TimeDateStamp,
                                  KphpDynModules[KphDynNtoskrnl].SizeOfImage,
                                  NULL,
                                  &kernel);
    }
    else if (KphpDynModules[KphDynNtkrla57].Valid)
    {
        status = KphDynDataLookup((PKPH_DYN_CONFIG)DynConfig,
                                  DynConfigLength,
                                  KphpDynModules[KphDynNtkrla57].Class,
                                  KphpDynModules[KphDynNtkrla57].Machine,
                                  KphpDynModules[KphDynNtkrla57].TimeDateStamp,
                                  KphpDynModules[KphDynNtkrla57].SizeOfImage,
                                  NULL,
                                  &kernel);
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphDynDataLookup failed: %!STATUS!",
                      status);

        //
        // Activating dynamic data requires at least kernel data.
        //
        goto Exit;
    }

    if (KphpDynModules[KphDynLxcore].Valid)
    {
        status = KphDynDataLookup((PKPH_DYN_CONFIG)DynConfig,
                                  DynConfigLength,
                                  KphpDynModules[KphDynLxcore].Class,
                                  KphpDynModules[KphDynLxcore].Machine,
                                  KphpDynModules[KphDynLxcore].TimeDateStamp,
                                  KphpDynModules[KphDynLxcore].SizeOfImage,
                                  NULL,
                                  &lxcore);
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphDynDataLookup failed: %!STATUS!",
                      status);

        //
        // Activating dynamic data does not require lxcore data.
        //
        lxcore = NULL;
    }

    init.Config = (PKPH_DYN_CONFIG)DynConfig;
    init.Kernel = kernel;
    init.Lxcore = lxcore;

    status = KphCreateObject(KphpDynDataType, sizeof(KPH_DYN), &dyn, &init);
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
                  "for Windows %lu.%lu.%lu Kernel %hu.%hu.%hu.%hu",
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

    KPH_PAGED_CODE_PASSIVE();

    dynData = NULL;
    signature = NULL;

    if (!DynData || !DynDataLength)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!Signature || !SignatureLength)
    {
        status = STATUS_SI_DYNDATA_INVALID_SIGNATURE;
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
            ProbeInputBytes(DynData, DynDataLength);
            RtlCopyVolatileMemory(dynData, DynData, DynDataLength);

            ProbeInputBytes(Signature, DynDataLength);
            RtlCopyVolatileMemory(signature, Signature, SignatureLength);
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
 * \brief Initializes the dynamic modules.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpInitializeDynModules(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KeEnterCriticalRegion();
    if (!ExAcquireResourceSharedLite(PsLoadedModuleResource, TRUE))
    {
        KeLeaveCriticalRegion();

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Failed to acquire PsLoadedModuleResource");

        return;
    }

    for (ULONG i = 0; i < ARRAYSIZE(KphpDynModules); i++)
    {
        PKPH_DYN_MODULE dynModule;

        dynModule = &KphpDynModules[i];

        for (PLIST_ENTRY link = PsLoadedModuleList->Flink;
             link != PsLoadedModuleList;
             link = link->Flink)
        {
            NTSTATUS status;
            PKLDR_DATA_TABLE_ENTRY entry;
            KPH_IMAGE_NT_HEADERS image;

            entry = CONTAINING_RECORD(link, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

            if (!RtlEqualUnicodeString(&entry->BaseDllName, &dynModule->Name, TRUE))
            {
                continue;
            }

            __try
            {
                status = KphImageNtHeader(entry->DllBase, entry->SizeOfImage, &image);
                if (!NT_SUCCESS(status))
                {
                    KphTracePrint(TRACE_LEVEL_VERBOSE,
                                  GENERAL,
                                  "KphImageNtHeader failed: %!STATUS!",
                                  status);

                    break;
                }

                dynModule->Machine = image.Headers->FileHeader.Machine;
                dynModule->TimeDateStamp = image.Headers->FileHeader.TimeDateStamp;
                if (RTL_CONTAINS_FIELD(&image.Headers->OptionalHeader,
                                       image.Headers->FileHeader.SizeOfOptionalHeader,
                                       SizeOfImage))
                {
                    dynModule->SizeOfImage = image.Headers->OptionalHeader.SizeOfImage;
                    dynModule->Valid = TRUE;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "Failed to read module headers: %!STATUS!",
                              GetExceptionCode());
            }

            break;
        }
    }

    ExReleaseResourceLite(PsLoadedModuleResource);
    KeLeaveCriticalRegion();
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

    KPH_PAGED_CODE_PASSIVE();

    KphpInitializeDynModules();

    typeInfo.Allocate = KphpAllocateDynData;
    typeInfo.Initialize = KphpInitializeDynData;
    typeInfo.Delete = KphpDeleteDynData;
    typeInfo.Free = KphpFreeDynData;
    typeInfo.Flags = 0;
    typeInfo.DeferDelete = TRUE;

    KphCreateObjectType(&KphpDynDataTypeName, &typeInfo, &KphpDynDataType);

    if (KphParameterFlags.DynDataNoEmbedded)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Embedded Dynamic Configuration disabled");
    }
    else
    {
        status = KphpActivateDynData((PBYTE)KphDynConfig,
                                     KphDynConfigLength,
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
    KPH_PAGED_CODE_PASSIVE();

    KphAtomicAssignObjectReference(&KphpDynData.Atomic, NULL);
}
