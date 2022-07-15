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

//
// This library is intended to be an extension of core OS functionality which 
// enables drivers to preform operations within the system that would otherwise
// be dangerous or impossible.
//

//
// This is an extension of the APC functionality in Windows to enable a driver
// to be unloaded while there are outstanding APCs on the system which
// reference it.
//
// N.B. While this library guarantees the driver will not be unmapped, it does
// not prevent DriverUnload from being invoked. Drivers using this library may
// check DIRVER_OBJECT.Flags for DRVO_UNLOAD_INVOKED to know if the system has
// or is about to invoke DriverUnload. If applicable the driver should act
// accordingly in their routines.
//
// This extension guarantees that the cleanup routine will always be invoked.
// Either due to normal APC rundown, immediately after the kernel routine is
// executed, or (in the case of normal kernel APC execution) after the normal
// kernel routine executes. KSI_KAPC_CLEANUP_REASON indicates the context
// in which this is called. Users may choose to implement their own mechanism
// in the cleanup routine to synchronize with DriverUnload.
//

_Function_class_(KRUNDOWN_ROUTINE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID NTAPI KsipApcRundownRoutine(
    _In_ PKAPC Apc
    )
{
    PKSI_KAPC apc;
    PDRIVER_OBJECT driverObject;
    PKSI_KCLEANUP_ROUTINE cleanupRoutine;

    apc = (PKSI_KAPC)Apc;
    driverObject = apc->DriverObject;
    cleanupRoutine = (PKSI_KCLEANUP_ROUTINE)apc->InternalCleanup;

    cleanupRoutine(apc, KsiApcCleanupRundown);

    ObDereferenceObjectDeferDelete(driverObject);
}

_Function_class_(KNORMAL_ROUTINE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID NTAPI KsipApcNormalRoutine(
    _In_opt_ PVOID NormalContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )
{
    PKSI_KAPC apc;
    PDRIVER_OBJECT driverObject;
    PKNORMAL_ROUTINE normalRoutine;
    PKSI_KCLEANUP_ROUTINE cleanupRoutine;

    NT_ASSERT(NormalContext);

    apc = (PKSI_KAPC)NormalContext;
    driverObject = apc->DriverObject;
    normalRoutine = (PKNORMAL_ROUTINE)apc->InternalRoutine;
    cleanupRoutine = (PKSI_KCLEANUP_ROUTINE)apc->InternalCleanup;

    normalRoutine(apc->InternalContext, SystemArgument1, SystemArgument2);

    cleanupRoutine(apc, KsiApcCleanupNormal);

    ObDereferenceObjectDeferDelete(driverObject);
}

_Function_class_(KKERNEL_ROUTINE)
_IRQL_requires_(APC_LEVEL)
_IRQL_requires_same_
VOID NTAPI KsipApcKernelRoutine(
    _In_ PKAPC Apc,
    _Inout_ _Deref_pre_maybenull_ PKNORMAL_ROUTINE *NormalRoutine,
    _Inout_ _Deref_pre_maybenull_ PVOID* NormalContext,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument1,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument2
    )
{
    PKSI_KAPC apc;
    PDRIVER_OBJECT driverObject;
    PKSI_KKERNEL_ROUTINE kernelRoutine;
    PKSI_KCLEANUP_ROUTINE cleanupRoutine;

    apc = (PKSI_KAPC)Apc;
    driverObject = apc->DriverObject;
    kernelRoutine = (PKSI_KKERNEL_ROUTINE)apc->InternalRoutine;
    cleanupRoutine = (PKSI_KCLEANUP_ROUTINE)apc->InternalCleanup;

    kernelRoutine(apc,
                  NormalRoutine,
                  NormalContext,
                  SystemArgument1,
                  SystemArgument2);

    if ((apc->Apc.ApcMode == KernelMode) && *NormalRoutine)
    {
        apc->InternalRoutine = (PVOID)*NormalRoutine;
        apc->InternalContext = *NormalContext;
        *NormalRoutine = KsipApcNormalRoutine;
        *NormalContext = apc;
        return;
    }

    cleanupRoutine(apc, KsiApcCleanupKernel);

    ObDereferenceObjectDeferDelete(driverObject);
}

VOID KsiInitializeApc(
    _Out_ PKSI_KAPC Apc,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PRKTHREAD Thread,
    _In_ KAPC_ENVIRONMENT Environment,
    _In_ PKSI_KKERNEL_ROUTINE KernelRoutine,
    _In_ PKSI_KCLEANUP_ROUTINE CleanupRoutine,
    _In_opt_ PKNORMAL_ROUTINE NormalRoutine,
    _In_ KPROCESSOR_MODE Mode,
    _In_opt_ PVOID NormalContext
    )
{
    KeInitializeApc(&Apc->Apc,
                    Thread,
                    Environment,
                    KsipApcKernelRoutine,
                    KsipApcRundownRoutine,
                    NormalRoutine,
                    Mode,
                    NormalContext);

    Apc->DriverObject = DriverObject;
    Apc->InternalRoutine = (PVOID)KernelRoutine;
    Apc->InternalCleanup = (PVOID)CleanupRoutine;
    Apc->InternalContext = NULL;
}

BOOLEAN KsiInsertQueueApc(
    _In_ PKSI_KAPC Apc,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2,
    _In_ KPRIORITY PriorityBoost
    )
{
    BOOLEAN result;

    ObReferenceObject(Apc->DriverObject);

    result = KeInsertQueueApc(&Apc->Apc, 
                              SystemArgument1, 
                              SystemArgument2, 
                              PriorityBoost);
    if (!result)
    {
        ObDereferenceObject(Apc->DriverObject);
    }

    return result;
}

NTSTATUS DllInitialize(
    _In_ PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(RegistryPath);

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    return STATUS_SUCCESS;
}

_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    return STATUS_SUCCESS;
}
