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
#include <kphdyn.h>

#include <trace.h>

PAGED_FILE();

static UNICODE_STRING KphpAltitudeValueName = RTL_CONSTANT_STRING(L"KphAltitude");
static UNICODE_STRING KphpPortNameValueName = RTL_CONSTANT_STRING(L"KphPortName");
static UNICODE_STRING KphpDynDataValueName = RTL_CONSTANT_STRING(L"DynData");
static UNICODE_STRING KphpDynDataSigValueName = RTL_CONSTANT_STRING(L"DynDataSig");
static UNICODE_STRING KphpDisableImageLoadProtectionValueName = RTL_CONSTANT_STRING(L"DisableImageLoadProtection");

typedef struct _KPH_ACTIVE_DYNDATA
{
    ULONG Version;
    ULONG Index;
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
 * \return Successful status or an errant one.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpSetDynamicConfigiration(
    _In_ PKPH_DYN_CONFIGURATION Configuration
    )
{
    PAGED_PASSIVE();

    if ((Configuration->MajorVersion != KphKernelVersion.MajorVersion) ||
        (Configuration->MinorVersion != KphKernelVersion.MinorVersion))
    {
        return STATUS_NOT_SUPPORTED;
    }

    if ((KphKernelVersion.BuildNumber < Configuration->BuildNumberMin) ||
        (KphKernelVersion.BuildNumber > Configuration->BuildNumberMax))
    {
        return STATUS_NOT_SUPPORTED;
    }

    if ((KphKernelVersion.BuildNumber == Configuration->BuildNumberMin) &&
        (KphKernelVersion.Revision < Configuration->RevisionMin))
    {
        return STATUS_NOT_SUPPORTED;
    }

    if ((KphKernelVersion.BuildNumber == Configuration->BuildNumberMax) &&
        (KphKernelVersion.Revision > Configuration->RevisionMax))
    {
        return STATUS_NOT_SUPPORTED;
    }

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

    return STATUS_SUCCESS;
}

/**
 * \brief Opens the driver parameters key.
 *
 * \param[in] RegistryPath Registry path from the entry point.
 * \param[out] KeyHandle Handle to parameters key on success, null on failure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenParametersKey(
    _In_ PUNICODE_STRING RegistryPath,
    _Out_ PHANDLE KeyHandle
    )
{
    NTSTATUS status;
    WCHAR buffer[MAX_PATH];
    UNICODE_STRING parametersKeyName;
    OBJECT_ATTRIBUTES objectAttributes;

    PAGED_PASSIVE();

    *KeyHandle = NULL;

    parametersKeyName.Buffer = buffer;
    parametersKeyName.Length = 0;
    parametersKeyName.MaximumLength = sizeof(buffer);

    status = RtlAppendUnicodeStringToString(&parametersKeyName, RegistryPath);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = RtlAppendUnicodeToString(&parametersKeyName, L"\\Parameters");
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &parametersKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = ZwOpenKey(KeyHandle, KEY_READ, &objectAttributes);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Unable to open Parameters key: %!STATUS!",
                      status);

        *KeyHandle = NULL;
        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Reads dynamic configuration from parameters.
 *
 * \param[in] KeyHandle Handle to the parameters registry key.
 * \param[out] DynData Set to the dynamic data blob, should be freed with
 * KphFreeRegistryBinay.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpReadDynamicConfiguration(
    _In_ HANDLE KeyHandle,
    _Out_ PKPH_DYNDATA* DynData
    )
{
    NTSTATUS status;
    PBYTE dataBuffer;
    ULONG dataLength;
    PBYTE sigBuffer;
    ULONG sigLength;
    ULONG actualLength;
    PKPH_DYNDATA dynData;

    PAGED_PASSIVE();

    *DynData = NULL;

    dataBuffer = NULL;
    sigBuffer = NULL;

    status = KphQueryRegistryBinary(KeyHandle,
                                    &KphpDynDataValueName,
                                    &dataBuffer,
                                    &dataLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphQueryRegistryBinary failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphQueryRegistryBinary(KeyHandle,
                                    &KphpDynDataSigValueName,
                                    &sigBuffer,
                                    &sigLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphQueryRegistryBinary failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphVerifyBuffer(dataBuffer, dataLength, sigBuffer, sigLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_CRITICAL,
                      GENERAL,
                      "KphVerifyBuffer failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (dataLength < sizeof(KPH_DYNDATA))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Dynamic data blob is too small");

        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    dynData = (PKPH_DYNDATA)dataBuffer;

    if (dynData->Version != KPH_DYN_CONFIGURATION_VERSION)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Dynamic configuration version mismatch");

        status = STATUS_REVISION_MISMATCH;
        goto Exit;
    }

    status = RtlULongMult(sizeof(KPH_DYN_CONFIGURATION),
                          dynData->Count,
                          &actualLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlULongAdd(actualLength,
                         RTL_SIZEOF_THROUGH_FIELD(KPH_DYNDATA, Count),
                         &actualLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (dataLength < actualLength)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Dynamic data buffer overflow");

        status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    *DynData = (PKPH_DYNDATA)dataBuffer;
    dataBuffer = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (sigBuffer)
    {
        KphFreeRegistryBinary(sigBuffer);
    }

    if (dataBuffer)
    {
        KphFreeRegistryBinary(dataBuffer);
    }

    return status;
}

/**
 * \brief Initializes the dynamic data from the driver parameters.
 *
 * \param[in] RegistryPath Registry path from the entry point.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphDynamicDataInitialization(
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    PKPH_DYNDATA dynData;

    PAGED_PASSIVE();

    keyHandle = NULL;
    dynData = NULL;

    status = KphOpenParametersKey(RegistryPath, &keyHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphpOpenParametersKey failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphQueryRegistryString(keyHandle,
                                    &KphpAltitudeValueName,
                                    &KphDynAltitude);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphQueryRegistryString failed: %!STATUS!",
                      status);

        KphDynAltitude = NULL;
        goto Exit;
    }

    status = KphQueryRegistryString(keyHandle,
                                    &KphpPortNameValueName,
                                    &KphDynPortName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphQueryRegistryString failed: %!STATUS!",
                      status);

        KphDynPortName = NULL;
        goto Exit;
    }

    status = KphQueryRegistryULong(keyHandle,
                                   &KphpDisableImageLoadProtectionValueName,
                                   &KphDynDisableImageLoadProtection);
    if (!NT_SUCCESS(status))
    {
        KphDynDisableImageLoadProtection = 0;
    }

    status = KphpReadDynamicConfiguration(keyHandle, &dynData);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphpReadDynamicConfiguration failed: %!STATUS!",
                      status);

        goto Exit;
    }

    for (ULONG i = 0; i < dynData->Count; i++)
    {
        PKPH_DYN_CONFIGURATION config;

        config = &dynData->Configs[i];

        status = KphpSetDynamicConfigiration(config);
        if (NT_SUCCESS(status))
        {
            KphpActiveDynData.Version = dynData->Version;
            KphpActiveDynData.Index = i;
            KphpActiveDynData.MajorVersion = config->MajorVersion;
            KphpActiveDynData.MinorVersion = config->MinorVersion;
            KphpActiveDynData.BuildNumberMin = config->BuildNumberMin;
            KphpActiveDynData.RevisionMin = config->RevisionMin;
            KphpActiveDynData.BuildNumberMax = config->BuildNumberMax;
            KphpActiveDynData.RevisionMax = config->RevisionMax;
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphpSetDynamicConfigiration failed: %!STATUS!",
                      status);
    }

Exit:

    if (dynData)
    {
        KphFreeRegistryBinary((PBYTE)dynData);
    }

    if (keyHandle)
    {
        ObCloseHandle(keyHandle, KernelMode);
    }

    return status;
}

/**
 * \brief Cleans up dynamic configuration.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphDynamicDataCleanup(
    VOID
    )
{
    PAGED_PASSIVE();

    if (KphDynAltitude)
    {
        KphFreeRegistryString(KphDynAltitude);
        KphDynAltitude = NULL;
    }

    if (KphDynPortName)
    {
        KphFreeRegistryString(KphDynPortName);
        KphDynPortName = NULL;
    }
}
