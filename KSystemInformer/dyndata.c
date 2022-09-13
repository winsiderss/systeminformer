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

#include <trace.h>

PAGED_FILE();

static UNICODE_STRING KphpAltitudeValueName = RTL_CONSTANT_STRING(L"KphAltitude");
static UNICODE_STRING KphpPortNameValueName = RTL_CONSTANT_STRING(L"KphPortName");
static UNICODE_STRING KphpDynConfigurationValueName = RTL_CONSTANT_STRING(L"DynConfiguration");
static UNICODE_STRING KphpDisableImageLoadProtectionValueName = RTL_CONSTANT_STRING(L"DisableImageLoadProtection");

/**
 * \brief Verifies the PHNT version from dynamic configuration.
 *
 * \param[in] Version The version reported by OS.
 * \param[in] NtVersion PHNT version from dynamic configuration.
 *
 * \return TRUE if the version makes sense, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpIsValidNtVersion(
    _In_ PRTL_OSVERSIONINFOEXW Version,
    _In_ ULONG NtVersion
    )
{
    PAGED_PASSIVE();

    //
    // N.B. If a piece of the code relies on the PHNT version being accurate
    // then checks should be added here for it.
    //

    if (Version->dwMajorVersion == 6)
    {
        switch (Version->dwMinorVersion)
        {
            case 1:
            {
                return (NtVersion == PHNT_WIN7);
            }
            case 2:
            {
                return (NtVersion == PHNT_WIN8);
            }
            case 3:
            {
                return (NtVersion == PHNT_WINBLUE);
            }
            default:
            {
                return FALSE;
            }
        }
    }

    if (Version->dwMajorVersion >= 10)
    {
        return (NtVersion >= PHNT_THRESHOLD);
    }

    //
    // Driver doesn't support pre Win7.
    //
    return FALSE;
}

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
    NTSTATUS status;
    RTL_OSVERSIONINFOEXW version;

    PAGED_PASSIVE();

    if (Configuration->Version != KPH_DYN_CONFIGURATION_VERSION)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Dynamic configuration version mismatch!");

        return STATUS_REVISION_MISMATCH;
    }

    RtlZeroMemory(&version, sizeof(RTL_OSVERSIONINFOEXW));
    version.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    status = RtlGetVersion((PRTL_OSVERSIONINFOW)&version);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "RtlGetVersion failed: %!STATUS!",
                      status);

        return status;
    }

    if ((Configuration->MajorVersion != version.dwMajorVersion) ||
        (Configuration->MinorVersion != version.dwMinorVersion) ||
        ((Configuration->ServicePackMajor != USHORT_MAX) &&
         (Configuration->ServicePackMajor != version.wServicePackMajor)) ||
        ((Configuration->BuildNumber != USHORT_MAX) &&
         (Configuration->BuildNumber != version.dwBuildNumber)))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Dynamic configuration not supported on this OS!");

        return STATUS_NOT_SUPPORTED;
    }

    if (!KphpIsValidNtVersion(&version, Configuration->ResultingNtVersion))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ResultingNtVersion is invalid!");

        return STATUS_BAD_DATA;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  GENERAL,
                  "Setting dynamic configuration for Windows %u.%u",
                  Configuration->MajorVersion,
                  Configuration->MinorVersion);

    KphDynNtVersion = Configuration->ResultingNtVersion;

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
 * \param[out] Configuration Populated with the dynamic configuration.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpReadDynamicConfiguration(
    _In_ HANDLE KeyHandle,
    _Out_ PKPH_DYN_CONFIGURATION Configuration
    )
{
    NTSTATUS status;
    PUCHAR buffer;
    ULONG length;

    PAGED_PASSIVE();

    buffer = NULL;

    RtlZeroMemory(Configuration, sizeof(*Configuration));

    status = KphQueryRegistryBinary(KeyHandle,
                                    &KphpDynConfigurationValueName,
                                    &buffer,
                                    &length);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    if (length < sizeof(KPH_DYN_CONFIGURATION))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
        goto Exit;
    }

    RtlCopyMemory(Configuration, buffer, sizeof(*Configuration));
    status = STATUS_SUCCESS;

Exit:

    if (buffer)
    {
        KphFreeRegistryBinary(buffer);
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
    KPH_DYN_CONFIGURATION configuration;

    PAGED_PASSIVE();

    keyHandle = NULL;

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

    status = KphpReadDynamicConfiguration(keyHandle, &configuration);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphpReadDynamicConfiguration failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpSetDynamicConfigiration(&configuration);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphpSetDynamicConfigiration failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

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
