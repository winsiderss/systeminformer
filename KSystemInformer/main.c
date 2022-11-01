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
RTL_OSVERSIONINFOEXW KphOsVersionInfo = { 0 };
USHORT KphOsRevision = 0;
KPH_OSVERSION KphOsVersion = KphWinUnknown;

PAGED_FILE();

/**
 * \brief Protects select sections.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpProtectSections(
    VOID
    )
{
    NTSTATUS status;

    if (!KphDynMmProtectDriverSection)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "MmProtectDriverSection not found");

        return;
    }

    status = KphDynMmProtectDriverSection(&KphProtectedSection,
                                          0,
                                          MM_PROTECT_DRIVER_SECTION_ALLOW_UNLOAD);

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  GENERAL,
                  "MmProtectDriverSection %!STATUS!",
                  status);
}

/**
 * \brief Cleans up the driver state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpDriverCleanup(
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
    KphDynamicDataCleanup();
    KphCleanupHashing();
    KphCleanupVerify();
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

    KphpDriverCleanup();

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

    KphOsVersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    status = RtlGetVersion((PRTL_OSVERSIONINFOW)&KphOsVersionInfo);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "RtlGetVersion failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphOsVersion = KphWinUnknown;

    if (KphOsVersionInfo.dwMajorVersion == 6)
    {
        if (KphOsVersionInfo.dwMinorVersion == 1)
        {
            KphOsVersion = KphWin7;
        }
        else if (KphOsVersionInfo.dwMinorVersion == 2)
        {
            KphOsVersion = KphWin8;

        }
        else if (KphOsVersionInfo.dwMinorVersion == 3)
        {
            KphOsVersion = KphWin81;
        }
    }
    else if (KphOsVersionInfo.dwMajorVersion >= 10)
    {
        KphOsVersion = KphWin10;
    }

    if (KphOsVersion == KphWinUnknown)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Unknown OS Version %lu.%lu.%lu",
                      KphOsVersionInfo.dwMajorVersion,
                      KphOsVersionInfo.dwMinorVersion,
                      KphOsVersionInfo.dwBuildNumber);

        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    if (!NT_SUCCESS(ZwQuerySystemInformation(SystemSecureBootInformation,
                                             &KphSecureBootInfo,
                                             sizeof(KphSecureBootInfo),
                                             NULL)))
    {
        RtlZeroMemory(&KphSecureBootInfo, sizeof(KphSecureBootInfo));
    }

    KphInitializeAlloc();

    KphDynamicImport();

    status = KphLocateKernelRevision(&KphOsRevision);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphLocateKernelRevision failed: %!STATUS!",
                      status);

        goto Exit;
    }

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

    status = KphDynamicDataInitialization(RegistryPath);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Dynamic data initialization failed: %!STATUS!",
                      status);
        goto Exit;
    }

    KphpProtectSections();

    KphInitializeProtection();
    KphInitializeStackBackTrace();

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
        KphpDriverCleanup();

        KphTracePrint(TRACE_LEVEL_INFORMATION,
                      GENERAL,
                      "Driver Load Failed: %!STATUS!",
                      status);

        WPP_CLEANUP(DriverObject);
    }

    return status;
}
