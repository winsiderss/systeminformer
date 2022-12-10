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

    if ((Configuration->MajorVersion != KphOsVersionInfo.dwMajorVersion) ||
        (Configuration->MinorVersion != KphOsVersionInfo.dwMinorVersion))
    {
        return STATUS_NOT_SUPPORTED;
    }

    if ((KphOsVersionInfo.dwBuildNumber < Configuration->BuildNumberMin) ||
        (KphOsVersionInfo.dwBuildNumber > Configuration->BuildNumberMax))
    {
        return STATUS_NOT_SUPPORTED;
    }

    if ((KphOsVersionInfo.dwBuildNumber == Configuration->BuildNumberMin) &&
        (KphOsRevision < Configuration->RevisionMin))
    {
        return STATUS_NOT_SUPPORTED;
    }

    if ((KphOsVersionInfo.dwBuildNumber == Configuration->BuildNumberMax) &&
        (KphOsRevision > Configuration->RevisionMax))
    {
        return STATUS_NOT_SUPPORTED;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  GENERAL,
                  "Setting dynamic configuration for Windows %lu.%lu.%lu.%lu "
                  "(%lu.%lu.%lu.%lu - %lu.%lu.%lu.%lu)",
                  KphOsVersionInfo.dwMajorVersion,
                  KphOsVersionInfo.dwMinorVersion,
                  KphOsVersionInfo.dwBuildNumber,
                  KphOsRevision,
                  Configuration->MajorVersion,
                  Configuration->MinorVersion,
                  Configuration->BuildNumberMin,
                  Configuration->RevisionMin,
                  Configuration->MajorVersion,
                  Configuration->MinorVersion,
                  Configuration->BuildNumberMax,
                  Configuration->RevisionMax);

    KphDynEgeGuid = C_2sTo4(Configuration->EgeGuid);
    KphDynEpObjectTable = C_2sTo4(Configuration->EpObjectTable);
    KphDynEreGuidEntry = C_2sTo4(Configuration->EreGuidEntry);
    KphDynHtHandleContentionEvent = C_2sTo4(Configuration->HtHandleContentionEvent);
    KphDynOtName = C_2sTo4(Configuration->OtName);
    KphDynOtIndex = C_2sTo4(Configuration->OtIndex);
    KphDynObDecodeShift = C_2sTo4(Configuration->ObDecodeShift);
    KphDynObAttributesShift = C_2sTo4(Configuration->ObAttributesShift);

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

    KphDynAlpcCommunicationInfo = C_2sTo4(Configuration->AlpcCommunicationInfo);
    KphDynAlpcOwnerProcess = C_2sTo4(Configuration->AlpcOwnerProcess);
    KphDynAlpcConnectionPort = C_2sTo4(Configuration->AlpcConnectionPort);
    KphDynAlpcServerCommunicationPort = C_2sTo4(Configuration->AlpcServerCommunicationPort);
    KphDynAlpcClientCommunicationPort = C_2sTo4(Configuration->AlpcClientCommunicationPort);
    KphDynAlpcHandleTable = C_2sTo4(Configuration->AlpcHandleTable);
    KphDynAlpcHandleTableLock = C_2sTo4(Configuration->AlpcHandleTableLock);
    KphDynAlpcAttributes = C_2sTo4(Configuration->AlpcAttributes);
    KphDynAlpcAttributesFlags = C_2sTo4(Configuration->AlpcAttributesFlags);
    KphDynAlpcPortContext = C_2sTo4(Configuration->AlpcPortContext);
    KphDynAlpcSequenceNo = C_2sTo4(Configuration->AlpcSequenceNo);
    KphDynAlpcState = C_2sTo4(Configuration->AlpcState);

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
NTSTATUS KphpOpenParametersKey(
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

    status = KphpOpenParametersKey(RegistryPath, &keyHandle);
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
