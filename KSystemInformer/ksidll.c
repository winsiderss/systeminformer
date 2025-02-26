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
#include <process.h>
#include <sistatus.h>

//
// This library is intended to be an extension of core OS functionality which
// enables drivers to perform operations within the system that would otherwise
// be dangerous or impossible.
//

KPH_PROTECTED_DATA_SECTION_PUSH();
static BYTE KsipProtectedSection = 0;
static PMM_PROTECT_DRIVER_SECTION KsipMmProtectDriverSection = NULL;
static PKE_REMOVE_QUEUE_APC KsipKeRemoveQueueApc = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KsipGetSystemRoutineAddress(
    _In_z_ PCWSTR SystemRoutineName
    )
{
    UNICODE_STRING systemRoutineName;

    RtlInitUnicodeString(&systemRoutineName, SystemRoutineName);

    return MmGetSystemRoutineAddress(&systemRoutineName);
}

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
// Either due to normal APC rundown, removal, immediately after the kernel
// routine is executed, or (in the case of normal kernel APC execution) after
// the normal kernel routine executes. KSI_KAPC_CLEANUP_REASON indicates the
// context in which this is called. Users may choose to implement their own
// mechanism in the cleanup routine to synchronize with DriverUnload.
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

VOID KSIAPI KsiInitializeApc(
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

BOOLEAN KSIAPI KsiInsertQueueApc(
    _Inout_ PKSI_KAPC Apc,
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

NTSTATUS KSIAPI KsiRemoveQueueApc(
    _Inout_ PKSI_KAPC Apc
    )
{
    PDRIVER_OBJECT driverObject;
    PKSI_KCLEANUP_ROUTINE cleanupRoutine;

    if (!KsipKeRemoveQueueApc)
    {
        return STATUS_NOINTERFACE;
    }

    if (!KsipKeRemoveQueueApc(&Apc->Apc))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    driverObject = Apc->DriverObject;
    cleanupRoutine = (PKSI_KCLEANUP_ROUTINE)Apc->InternalCleanup;

    cleanupRoutine(Apc, KsiApcCleanupRemoved);

    ObDereferenceObjectDeferDelete(driverObject);

    return STATUS_SUCCESS;
}

//
// This is an extension of the work queue functionality in Windows to enable a
// driver to be unloaded while there are outstanding queued work items on the
// system with which reference it.
//
// N.B. WORK_QUEUE_ITEM and IO_WORKITEM are not equivalent. WORK_QUEUE_ITEM does
// not provide a mechanism to reference an object associated with the driver
// that queued the work item. IO_WORKITEM does, but its implementation for work
// scheduling and accounting differs. They are not interchangeable. Therefore,
// this implementation adds support for associating a driver object with a
// WORK_QUEUE_ITEM.
//
// N.B. While this library guarantees the driver will not be unmapped, it does
// not prevent DriverUnload from being invoked. Drivers using this library may
// check DIRVER_OBJECT.Flags for DRVO_UNLOAD_INVOKED to know if the system has
// or is about to invoke DriverUnload. If applicable the driver should act
// accordingly in their routines.
//

_Function_class_(WORKER_THREAD_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID KsipWorkItemRoutine(
    _In_ PVOID Parameter
    )
{
    PKSI_WORK_QUEUE_ITEM workItem;
    PDRIVER_OBJECT driverObject;
    PKSI_WORK_QUEUE_ROUTINE routine;

    workItem = Parameter;
    driverObject = workItem->DriverObject;
    routine = workItem->Routine;

    routine(workItem->Parameter);

    ObDereferenceObjectDeferDelete(driverObject);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KSIAPI KsiInitializeWorkItem(
    _Out_ PKSI_WORK_QUEUE_ITEM WorkItem,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PKSI_WORK_QUEUE_ROUTINE Routine,
    _In_opt_ PVOID Parameter
    )
{
    WorkItem->DriverObject = DriverObject;
    WorkItem->Routine = Routine;
    WorkItem->Parameter = Parameter;

#pragma warning(suppress: 4996)
    ExInitializeWorkItem(&WorkItem->WorkItem, KsipWorkItemRoutine, WorkItem);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KSIAPI KsiQueueWorkItem(
    _Inout_ PKSI_WORK_QUEUE_ITEM WorkItem,
    _In_ WORK_QUEUE_TYPE QueueType
    )
{
    ObReferenceObject(WorkItem->DriverObject);

#pragma prefast(push)
#pragma prefast(disable: 28159) // obsolete function
#pragma warning(suppress: 4996)
    ExQueueWorkItem(&WorkItem->WorkItem, QueueType);
#pragma prefast(pop)
}

//
// General Library Functions
//

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KsiInitialize(
    _In_ ULONG Version,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PVOID Reserved
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(Reserved);

    if (Version != KSIDLL_CURRENT_VERSION)
    {
        return STATUS_SI_KSIDLL_VERSION_MISMATCH;
    }

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KsiUninitialize(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ ULONG Reserved
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(Reserved);
}

NTSTATUS DllUnload(
    VOID
    )
{
    //
    // N.B. It used to be that not specifying a DllUnload routine would
    // enforce that your export driver can not be unloaded by not activating
    // the reference count mechanism. 10.0.25997.1010 changed this so if you
    // do not specify DllUnload the driver can be unmapped. This is resolved
    // by implementing DllUnload and returning an error.
    //
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS DllInitialize(
    _In_ PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(RegistryPath);

    __security_init_cookie();

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    KsipMmProtectDriverSection = (PMM_PROTECT_DRIVER_SECTION)KsipGetSystemRoutineAddress(L"MmProtectDriverSection");
    KsipKeRemoveQueueApc = (PKE_REMOVE_QUEUE_APC)KsipGetSystemRoutineAddress(L"KeRemoveQueueApc");

    if (KsipMmProtectDriverSection)
    {
        KsipMmProtectDriverSection(&KsipProtectedSection, 0, 0);
    }

    return STATUS_SUCCESS;
}
