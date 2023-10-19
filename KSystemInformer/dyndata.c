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
#define _DYNDATA_PRIVATE
#include <dyndata.h>
#include <kphdyndata.h>

#include <trace.h>

PAGED_FILE();

typedef struct _KPH_ACTIVE_DYNDATA
{
    ULONG Version;
    USHORT MajorVersion;
    USHORT MinorVersion;
    USHORT BuildNumberMin;
    USHORT RevisionMin;
    USHORT BuildNumberMax;
    USHORT RevisionMax;
} KPH_ACTIVE_DYNDATA, *PKPH_ACTIVE_DYNDATA;

static KPH_ACTIVE_DYNDATA KphpActiveDynData = { 0 };

#define KPH_LOAD_DYNITEM(x) KphDyn##x = C_2sTo4(Configuration->##x)

/**
 * \brief Sets the dynamic configuration.
 *
 * \param[in] Configuration The configuration to set.
 *
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpSetDynamicConfigiration(
    _In_ PKPH_DYN_CONFIGURATION Configuration
    )
{
    PAGED_CODE_PASSIVE();

    KphTracePrint(TRACE_LEVEL_INFORMATION,
                  GENERAL,
                  "Setting dynamic configuration "
                  "(%lu.%lu.%lu.%lu - %lu.%lu.%lu.%lu) "
                  "for Windows %lu.%lu.%lu Kernel %lu.%lu.%lu.%lu",
                  Configuration->MajorVersion,
                  Configuration->MinorVersion,
                  Configuration->BuildNumberMin,
                  Configuration->RevisionMin,
                  Configuration->MajorVersion,
                  Configuration->MinorVersion,
                  Configuration->BuildNumberMax,
                  Configuration->RevisionMax,
                  KphOsVersionInfo.dwMajorVersion,
                  KphOsVersionInfo.dwMinorVersion,
                  KphOsVersionInfo.dwBuildNumber,
                  KphKernelVersion.MajorVersion,
                  KphKernelVersion.MinorVersion,
                  KphKernelVersion.BuildNumber,
                  KphKernelVersion.Revision);

    KPH_LOAD_DYNITEM(EgeGuid);
    KPH_LOAD_DYNITEM(EpObjectTable);
    KPH_LOAD_DYNITEM(EreGuidEntry);
    KPH_LOAD_DYNITEM(HtHandleContentionEvent);
    KPH_LOAD_DYNITEM(OtName);
    KPH_LOAD_DYNITEM(OtIndex);
    KPH_LOAD_DYNITEM(ObDecodeShift);
    KPH_LOAD_DYNITEM(ObAttributesShift);

    if (Configuration->CiVersion == KPH_DYN_CI_V1)
    {
        KphTracePrint(TRACE_LEVEL_INFORMATION, GENERAL, "CI V1");
        KphDynCiFreePolicyInfo = (PCI_FREE_POLICY_INFO)KphGetRoutineAddress(L"ci.dll", "CiFreePolicyInfo");
        KphDynCiCheckSignedFile = (PCI_CHECK_SIGNED_FILE)KphGetRoutineAddress(L"ci.dll", "CiCheckSignedFile");
        KphDynCiVerifyHashInCatalog = (PCI_VERIFY_HASH_IN_CATALOG)KphGetRoutineAddress(L"ci.dll", "CiVerifyHashInCatalog");
    }
    else if (Configuration->CiVersion == KPH_DYN_CI_V2)
    {
        KphTracePrint(TRACE_LEVEL_INFORMATION, GENERAL, "CI V2");
        KphDynCiFreePolicyInfo = (PCI_FREE_POLICY_INFO)KphGetRoutineAddress(L"ci.dll", "CiFreePolicyInfo");
        KphDynCiCheckSignedFileEx = (PCI_CHECK_SIGNED_FILE_EX)KphGetRoutineAddress(L"ci.dll", "CiCheckSignedFile");
        KphDynCiVerifyHashInCatalogEx = (PCI_VERIFY_HASH_IN_CATALOG_EX)KphGetRoutineAddress(L"ci.dll", "CiVerifyHashInCatalog");
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_ERROR, GENERAL, "CI INVALID");
    }

    if (Configuration->LxVersion == KPH_DYN_LX_V1)
    {
        KphTracePrint(TRACE_LEVEL_INFORMATION, GENERAL, "LX V1");
        KphDynLxpThreadGetCurrent = (PLXP_THREAD_GET_CURRENT)KphGetRoutineAddress(L"lxcore.sys", "LxpThreadGetCurrent");
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_ERROR, GENERAL, "LX INVALID");
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
}

/**
 * \brief Initializes the dynamic data from the driver parameters.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphDynamicDataInitialization(
    VOID
    )
{
    PAGED_CODE_PASSIVE();

    return STATUS_NOT_IMPLEMENTED;
}
