/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2021-2022
 *
 */

#include <kph.h>
#include <dyndata.h>
#include <informer.h>

#include <trace.h>

PDRIVER_OBJECT KphDriverObject = NULL;
KPH_INFORMER_SETTINGS KphInformerSettings;
BOOLEAN KphIgnoreDebuggerPresence = FALSE;
SYSTEM_SECUREBOOT_INFORMATION KphSecureBootInfo = { 0 };

PAGED_FILE();

/**
 * \brief Cleans up the driver state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID DriverCleanup(
    VOID
    )
{
    PAGED_PASSIVE();

    KphObjectInformerStop();
    KphDebugInformerStop();
    KphImageInformerStop();
    KphCidMarkPopulated();
    KphThreadInformerStop();
    KphProcessInformerStop();
    KphFltUnregister();
    KphCidCleanup();
    KphCleanupSigning();
    KphCleanupHashing();
    KphCleanupVerify();
    KphDynamicDataCleanup();
}

/**
 * \brief Driver unload routine.
 *
 * \param[in] DriverObject Driver object of this driver.
 */
_Function_class_(DRIVER_UNLOAD)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID DriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    PAGED_PASSIVE();

    KphTracePrint(TRACE_LEVEL_INFORMATION, GENERAL, "Driver Unloading...");

    DriverCleanup();

    KphTracePrint(TRACE_LEVEL_INFORMATION, GENERAL, "Driver Unloaded");

    WPP_CLEANUP(DriverObject);
}

/**
 * \brief Driver entry point.
 *
 * \param[in] DriverObject Driver object for this driver.
 * \param[in] RegistryPath Registry path for this driver.
 *
 * \return Successful or errant status.
 */
_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;

    PAGED_PASSIVE();

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    KphTracePrint(TRACE_LEVEL_INFORMATION, GENERAL, "Driver Loading...");

    KphDriverObject = DriverObject;
    KphDriverObject->DriverUnload = DriverUnload;

    RtlZeroMemory(&KphInformerSettings, sizeof(KphInformerSettings));

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    if (!NT_SUCCESS(ZwQuerySystemInformation(SystemSecureBootInformation,
                                             &KphSecureBootInfo,
                                             sizeof(KphSecureBootInfo),
                                             NULL)))
    {
        RtlZeroMemory(&KphSecureBootInfo, sizeof(KphSecureBootInfo));
    }

    KphInitializeAlloc();

    KphDynamicImport();

    status = KphDynamicDataInitialization(RegistryPath);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Dynamic data initialization failed: %!STATUS!",
                      status);
        goto Exit;
    }

    KphInitializeProtection();
    KphInitializeStackBackTrace();

    status = KphInitializeVerify();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to initialize signing: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphInitializeHashing();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to initialize hashing: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphInitializeSigning();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to initialize signing: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphFltRegister(DriverObject, RegistryPath);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to register mini-filter: %!STATUS!",
                      status);
        goto Exit;
    }

    status = KphCidInitialize();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to initialize CID tracking: %!STATUS!",
                      status);
        goto Exit;
    }

    status = KphProcessInformerStart();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to start process informer: %!STATUS!",
                      status);
        goto Exit;
    }

    status = KphThreadInformerStart();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to start thread informer: %!STATUS!",
                      status);
        goto Exit;
    }

    status = KphCidPopulate();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to populate CID tracking: %!STATUS!",
                      status);
        goto Exit;
    }

    KphCidMarkPopulated();

    status = KphFltInformerStart();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to start mini-filter informer: %!STATUS!",
                      status);
        goto Exit;
    }

    status = KphImageInformerStart();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to start image informer: %!STATUS!",
                      status);
        goto Exit;
    }

    status = KphDebugInformerStart();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to start debug informer: %!STATUS!",
                      status);
        goto Exit;
    }

    status = KphObjectInformerStart();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to start object informer: %!STATUS!",
                      status);
        goto Exit;
    }

Exit:

    if (NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_INFORMATION, GENERAL, "Driver Loaded");
    }
    else
    {
        DriverCleanup();

        KphTracePrint(TRACE_LEVEL_INFORMATION,
                      GENERAL,
                      "Driver Load Failed: %!STATUS!",
                      status);

        WPP_CLEANUP(DriverObject);
    }

    return status;
}
