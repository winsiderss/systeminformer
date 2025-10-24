/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PAGED_FILE();

/**
 * \brief Performs generic system control actions.
 *
 * \param[in] SystemControlClass System control classification.
 * \param[in] SystemControlInfo Control input buffer.
 * \param[in] SystemControlInfoLength Length of control input buffer.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSystemControl(
    _In_ KPH_SYSTEM_CONTROL_CLASS SystemControlClass,
    _In_reads_bytes_(SystemControlInfoLength) PVOID SystemControlInfo,
    _In_ ULONG SystemControlInfoLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(SystemControlInfo);
    UNREFERENCED_PARAMETER(SystemControlInfoLength);
    UNREFERENCED_PARAMETER(AccessMode);

    processHandle = NULL;

    switch (SystemControlClass)
    {
        case KphSystemControlEmptyCompressionStore:
        {
            SYSTEM_STORE_INFORMATION storeInfo;
            SM_STORE_COMPRESSION_INFORMATION_REQUEST compressionInfo;
            CLIENT_ID clientId;
            OBJECT_ATTRIBUTES objectAttributes;
            QUOTA_LIMITS_EX quotaLimits;

            RtlZeroMemory(&compressionInfo, sizeof(SM_STORE_COMPRESSION_INFORMATION_REQUEST));
            compressionInfo.Version = SYSTEM_STORE_COMPRESSION_INFORMATION_VERSION_V1;

            RtlZeroMemory(&storeInfo, sizeof(SYSTEM_STORE_INFORMATION));
            storeInfo.Version = SYSTEM_STORE_INFORMATION_VERSION;
            storeInfo.StoreInformationClass = MemCompressionInfoRequest;
            storeInfo.Data = &compressionInfo;
            storeInfo.Length = SYSTEM_STORE_COMPRESSION_INFORMATION_SIZE_V1;

            status = ZwQuerySystemInformation(SystemStoreInformation,
                                              &storeInfo,
                                              sizeof(storeInfo),
                                              NULL);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ZwQuerySystemInformation failed: %!STATUS!",
                              status);

                goto Exit;
            }

            if (compressionInfo.CompressionPid == 0)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "Compression PID is zero");

                status = STATUS_INVALID_CID;
                goto Exit;
            }

            InitializeObjectAttributes(&objectAttributes,
                                       NULL,
                                       OBJ_KERNEL_HANDLE,
                                       NULL,
                                       NULL);

            clientId.UniqueProcess = ULongToHandle(compressionInfo.CompressionPid);
            clientId.UniqueThread = NULL;

            status = ZwOpenProcess(&processHandle,
                                   PROCESS_SET_QUOTA,
                                   &objectAttributes,
                                   &clientId);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ZwOpenProcess failed: %!STATUS!",
                              status);

                processHandle = NULL;
                goto Exit;
            }

            RtlZeroMemory(&quotaLimits, sizeof(quotaLimits));
            quotaLimits.MinimumWorkingSetSize = SIZE_T_MAX;
            quotaLimits.MaximumWorkingSetSize = SIZE_T_MAX;

            status = ZwSetInformationProcess(processHandle,
                                             ProcessQuotaLimits,
                                             &quotaLimits,
                                             sizeof(quotaLimits));
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ZwSetInformationProcess failed: %!STATUS!",
                              status);
            }

            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            goto Exit;
        }
    }

Exit:

    if (processHandle)
    {
        ObCloseHandle(processHandle, KernelMode);
    }

    return status;
}
