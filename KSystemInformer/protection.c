/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2020
 *
 */

#include <kph.h>
#include <dyndata.h>

#include <trace.h>

static UNICODE_STRING KphpImageLoadApcTypeName = RTL_CONSTANT_STRING(L"KphImageLoadApc");
static PKPH_OBJECT_TYPE KphpImageLoadApcType = NULL;

PAGED_FILE();

typedef struct _KPH_ENUM_FOR_PROTECTION
{
    PKPH_PROCESS_CONTEXT ProcessEnum;
    PKPH_PROCESS_CONTEXT Process;
    NTSTATUS Status;
} KPH_ENUM_FOR_PROTECTION, *PKPH_ENUM_FOR_PROTECTION;

typedef struct _KPH_IMAGE_LOAD_APC
{
    KSI_KAPC Apc;
    PKPH_PROCESS_CONTEXT Process;
    PVOID ImageBase;
    PFILE_OBJECT FileObject;
} KPH_IMAGE_LOAD_APC, *PKPH_IMAGE_LOAD_APC;

typedef struct _KPH_IMAGE_LOAD_APC_INIT
{
    PKPH_PROCESS_CONTEXT Process;
    PVOID ImageBase;
    PFILE_OBJECT FileObject;
} KPH_IMAGE_LOAD_APC_INIT, *PKPH_IMAGE_LOAD_APC_INIT;

/**
 * \brief Allocates an image load APC object.
 *
 * \param[in] Size The size to allocate.
 *
 * \return Allocates image load APC object, null on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateImageLoadApc(
    _In_ SIZE_T Size
    )
{
    PAGED_CODE();

    return KphAllocateNPaged(Size, KPH_TAG_IMAGE_LOAD_APC);
}

/**
 * \brief Initializes image load APC object.
 *
 * \param[in,out] Object The image load APC object to initialize.
 * \param[in] Parameter The image load ACP initialization structure.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitializeImageLoadApc(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_IMAGE_LOAD_APC apc;
    PKPH_IMAGE_LOAD_APC_INIT init;

    PAGED_CODE();

    NT_ASSERT(Parameter);

    apc = Object;
    init = Parameter;

    apc->Process = init->Process;
    KphReferenceObject(apc->Process);

    apc->ImageBase = init->ImageBase;

    apc->FileObject = init->FileObject;
    if (apc->FileObject)
    {
        ObReferenceObject(apc->FileObject);
    }

    KphReferenceHashingInfrastructure();
    KphReferenceSigningInfrastructure();

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes image load APC object.
 *
 * \param[in,out] Object The image load APC object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
VOID KSIAPI KphpDeleteImageLoadApc(
    _Inout_ PVOID Object
    )
{
    PKPH_IMAGE_LOAD_APC apc;

    PAGED_CODE();

    apc = Object;

    KphDereferenceObject(apc->Process);

    if (apc->FileObject)
    {
        ObDereferenceObject(apc->FileObject);
    }

    KphDereferenceSigningInfrastructure();
    KphDereferenceHashingInfrastructure();
}

/**
 * \brief Frees an image load APC object.
 *
 * \param[in] Object Image load APC object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
VOID KSIAPI KphpFreeImageLoadApc(
    _In_freesMem_ PVOID Object
    )
{
    PAGED_CODE();

    KphFree(Object, KPH_TAG_IMAGE_LOAD_APC);
}

/**
 * \brief Initializes the protection infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeProtection(
    VOID
    )
{
    KPH_OBJECT_TYPE_INFO typeInfo;

    PAGED_PASSIVE();

    typeInfo.Allocate = KphpAllocateImageLoadApc;
    typeInfo.Initialize = KphpInitializeImageLoadApc;
    typeInfo.Delete = KphpDeleteImageLoadApc;
    typeInfo.Free = KphpFreeImageLoadApc;

    KphCreateObjectType(&KphpImageLoadApcTypeName,
                        &typeInfo,
                        &KphpImageLoadApcType);
}

/**
 * \brief Checks if object protections should be suppressed.
 *
 * \param[in] Actor The actor process.
 * \param[in] Target The target process.
 *
 * \return TRUE if object protections should be suppressed, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphpShouldSuppressObjectProtections(
    _In_ PKPH_PROCESS_CONTEXT Actor,
    _In_ PKPH_PROCESS_CONTEXT Target
    )
{
    NTSTATUS status;

    PAGED_PASSIVE();

    status = KphDominationCheck(Target->EProcess, Actor->EProcess, UserMode);
    if (!NT_SUCCESS(status))
    {
        //
        // Grant access when the actor is a protected process and the target is
        // not protected at a higher level.
        //
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      PROTECTION,
                      "Protected process %lu access grated to PPL process %lu",
                      HandleToULong(Target->ProcessId),
                      HandleToULong(Actor->ProcessId));

        return TRUE;
    }

    if (Actor->IsLsass)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      PROTECTION,
                      "Protected process %lu access grated to lsass process %lu",
                      HandleToULong(Target->ProcessId),
                      HandleToULong(Actor->ProcessId));

        return TRUE;
    }

    return FALSE;
}

/**
 * \brief Handle enumeration callback for protecting a process.
 *
 * \param[in,out] HandleTableEntry The handle table entry being enumerated.
 * \param[in] Handle The handle being enumerated.
 * \param[in] Parameter The enumerate for protection context.
 *
 * \return FALSE to continue enumerating, TRUE to stop.
 */
_Function_class_(KPH_ENUM_PROCESS_HANDLES_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KSIAPI KphEnumProcessHandlesForProtection(
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_ENUM_FOR_PROTECTION parameter;
    POBJECT_HEADER objectHeader;
    PVOID object;
    POBJECT_TYPE objectType;
    ACCESS_MASK grantedAccess;
    ACCESS_MASK allowedAccessMask;

    PAGED_PASSIVE();

    NT_ASSERT(Parameter);

    parameter = Parameter;

    if (IsKernelHandle(Handle))
    {
        //
        // Don't screw with kernel handles.
        //
        return FALSE;
    }

    objectHeader = ObpDecodeObject(HandleTableEntry->Object);
    if (!objectHeader)
    {
        //
        // We don't have the dynamic data necessary to do work.
        //
        parameter->Status = STATUS_NOINTERFACE;
        return TRUE;
    }

    object = (PVOID)&objectHeader->Body;
    objectType = ObGetObjectType(object);

    if (objectType == *PsProcessType)
    {
        if (parameter->Process->EProcess != object)
        {
            return FALSE;
        }

        grantedAccess = ObpDecodeGrantedAccess(HandleTableEntry->GrantedAccess);
        allowedAccessMask = parameter->Process->ProcessAllowedMask;

        if ((grantedAccess & allowedAccessMask) != grantedAccess)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          PROTECTION,
                          "Modifying process handle (0x%04x, 0x%08x -> 0x%08x) "
                          "permissions in process %lu for process %lu",
                          HandleToULong(Handle),
                          grantedAccess,
                          (grantedAccess & allowedAccessMask),
                          HandleToULong(parameter->ProcessEnum->ProcessId),
                          HandleToULong(parameter->Process->ProcessId));

            ObpSetGrantedAccess(&HandleTableEntry->GrantedAccess,
                                (grantedAccess & allowedAccessMask));
        }

        return FALSE;
    }

    if (objectType == *PsThreadType)
    {
        PEPROCESS process;

        process = PsGetThreadProcess(object);

        if (parameter->Process->EProcess != process)
        {
            return FALSE;
        }

        grantedAccess = ObpDecodeGrantedAccess(HandleTableEntry->GrantedAccess);
        allowedAccessMask = parameter->Process->ThreadAllowedMask;

        if ((grantedAccess & allowedAccessMask) != grantedAccess)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          PROTECTION,
                          "Modifying thread handle (0x%04x, 0x%08x -> 0x%08x) "
                          "permissions in process %lu for process %lu",
                          HandleToULong(Handle),
                          grantedAccess,
                          (grantedAccess & allowedAccessMask),
                          HandleToULong(parameter->ProcessEnum->ProcessId),
                          HandleToULong(parameter->Process->ProcessId));

            ObpSetGrantedAccess(&HandleTableEntry->GrantedAccess,
                                (grantedAccess & allowedAccessMask));
        }
    }

    return FALSE;
}

/**
 * \brief Process context enumeration callback for process protection.
 *
 * \param[in] Process The process context being enumerated.
 * \param[in] Parameter The enumerate for protection context.
 *
 * \return FALSE to continue enumerating, TRUE to stop.
 */
_Function_class_(KPH_ENUM_PROCESS_CONTEXTS_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KSIAPI KphEnumProcessContextsForProtection(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    PKPH_ENUM_FOR_PROTECTION parameter;

    PAGED_PASSIVE();

    NT_ASSERT(Parameter);

    parameter = Parameter;

    if (KphpShouldSuppressObjectProtections(Process, parameter->Process))
    {
        return FALSE;
    }

    parameter->ProcessEnum = Process;

    status = KphEnumerateProcessHandlesEx(Process->EProcess,
                                          KphEnumProcessHandlesForProtection,
                                          parameter);
    if (status == STATUS_NOINTERFACE)
    {
        //
        // This means we don't have the dynamic data necessary to do work.
        // All other errors are failure to access process objects because
        // the process is exiting or has no object table.
        //
        parameter->Status = status;
    }
    else if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      PROTECTION,
                      "KphEnumerateProcessHandlesEx failed: %!STATUS!",
                      status);
    }

    return (!NT_SUCCESS(parameter->Status));
}

/**
 * \brief Stops protecting a process.
 *
 * \param[in] Process The process to stop protecting.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphStopProtectingProcess(
    _In_ PKPH_PROCESS_CONTEXT Process
    )
{
    PAGED_PASSIVE();

    KphAcquireRWLockExclusive(&Process->ProtectionLock);

    Process->Protected = FALSE;
    Process->ProcessAllowedMask = 0;
    Process->ThreadAllowedMask = 0;

    KphReleaseRWLock(&Process->ProtectionLock);

    KphTracePrint(TRACE_LEVEL_INFORMATION,
                  PROTECTION,
                  "Stopped protecting process %lu",
                  HandleToULong(Process->ProcessId));
}

/**
 * \brief Determines if a process is protected.
 *
 * \param[in] Process The process to check.
 *
 * \return TRUE if the process is protected, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphIsProtectedProcess(
    _In_ PKPH_PROCESS_CONTEXT Process
    )
{
    BOOLEAN isProtectedProcess;

    PAGED_PASSIVE();

    KphAcquireRWLockShared(&Process->ProtectionLock);
    isProtectedProcess = Process->Protected ? TRUE : FALSE;
    KphReleaseRWLock(&Process->ProtectionLock);

    return isProtectedProcess;
}

/**
 * \brief Starts protecting a process.
 *
 * \param[in] Process The process to start protecting.
 * \param[in] ProcessAllowedMask Access mask containing the allowed access to
 * the process.
 * \param[in] ThreadAllowedMask Access mask containing the allowed access to
 * the process threads.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphStartProtectingProcess(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_ ACCESS_MASK ProcessAllowedMask,
    _In_ ACCESS_MASK ThreadAllowedMask
    )
{
#if KPH_PROTECTION_SUPPRESSED

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(ProcessAllowedMask);
    UNREFERENCED_PARAMETER(ThreadAllowedMask);

    return STATUS_NOT_SUPPORTED;

#else

    NTSTATUS status;
    SECURITY_SUBJECT_CONTEXT subjectContext;
    BOOLEAN accessGranted;
    KPH_ENUM_FOR_PROTECTION context;

    PAGED_PASSIVE();

    SeCaptureSubjectContextEx(NULL, Process->EProcess, &subjectContext);

    accessGranted = KphSinglePrivilegeCheckEx(SeExports->SeDebugPrivilege,
                                              &subjectContext,
                                              UserMode);

    SeReleaseSubjectContext(&subjectContext);

    if (!accessGranted)
    {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    KphAcquireRWLockExclusive(&Process->ProtectionLock);

    if (Process->Protected)
    {
        KphTracePrint(TRACE_LEVEL_INFORMATION,
                      PROTECTION,
                      "Already protecting process %lu",
                      HandleToULong(Process->ProcessId));

        status = STATUS_ALREADY_COMPLETE;
        goto Exit;
    }

    KphTracePrint(TRACE_LEVEL_INFORMATION,
                  PROTECTION,
                  "Started protecting process %lu",
                  HandleToULong(Process->ProcessId));

    Process->Protected = TRUE;
    Process->ProcessAllowedMask = ProcessAllowedMask;
    Process->ThreadAllowedMask = ThreadAllowedMask;

    context.Status = STATUS_SUCCESS;
    context.Process = Process;

    KphEnumerateProcessContexts(KphEnumProcessContextsForProtection, &context);

    status = context.Status;

    if (!NT_SUCCESS(status))
    {
        Process->Protected = FALSE;
        Process->ProcessAllowedMask = 0;
        Process->ThreadAllowedMask = 0;
    }

Exit:

    KphReleaseRWLock(&Process->ProtectionLock);

    return status;

#endif
}

/**
 * \brief Returns true if we should permit extending handle permissions for
 * the creator process.
 *
 * \param[in] Info Object pre operation information.
 * \param[in] Actor The actor thread.
 * \param[in] ProcessTarget The process to check against.
 *
 * \return TRUE if we should permit, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
KphpShouldPermitCreatorProcess(
    _In_ POB_PRE_OPERATION_INFORMATION Info,
    _In_ PKPH_THREAD_CONTEXT Actor,
    _In_ PKPH_PROCESS_CONTEXT Process
    )
{
    KPH_PROCESS_STATE processState;

    PAGED_PASSIVE();

    NT_ASSERT(Process->VerifiedProcess);

    if (Info->Operation == OB_OPERATION_HANDLE_DUPLICATE)
    {
        return FALSE;
    }

    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_CREATE);

    if (!Actor->IsCreatingProcess ||
        (Actor->IsCreatingProcessId != Process->ProcessId))
    {
        return FALSE;
    }

    processState = KphGetProcessState(Actor->ProcessContext);

    return ((processState & KPH_PROCESS_STATE_MINIMUM) == KPH_PROCESS_STATE_MINIMUM);
}

/**
 * \brief Applies object protections to protected processes. Invoked by the
 * object manager callbacks.
 *
 * \param[in,out] Info Object pre operation information to apply protections.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphApplyObProtections(
    _Inout_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    PKPH_PROCESS_CONTEXT process;
    PKPH_THREAD_CONTEXT actor;
    BOOLEAN releaseLock;
    ACCESS_MASK allowedAccessMask;
    ACCESS_MASK desiredAccess;
    PACCESS_MASK access;

    PAGED_PASSIVE();

    process = NULL;
    actor = NULL;
    releaseLock = FALSE;

    NT_ASSERT((Info->ObjectType == *PsProcessType) ||
              (Info->ObjectType == *PsThreadType));

    if (Info->KernelHandle)
    {
        goto Exit;
    }

    actor = KphGetCurrentThreadContext();
    if (!actor || !actor->ProcessContext)
    {
        goto Exit;
    }

    if (Info->ObjectType == *PsProcessType)
    {
        process = KphGetProcessContext(PsGetProcessId(Info->Object));
    }
    else
    {
        PKPH_THREAD_CONTEXT thread;

        NT_ASSERT(Info->ObjectType == *PsThreadType);

        thread = KphGetThreadContext(PsGetThreadId(Info->Object));
        if (thread)
        {
            process = thread->ProcessContext;
            if (process)
            {
                KphReferenceObject(process);
            }

            KphDereferenceObject(thread);
        }
    }

    if (!process || (process->EProcess == actor->ProcessContext->EProcess))
    {
        goto Exit;
    }

    KphAcquireRWLockShared(&process->ProtectionLock);
    releaseLock = TRUE;

    if (!process->Protected)
    {
        goto Exit;
    }

    if (KphpShouldSuppressObjectProtections(actor->ProcessContext, process))
    {
        goto Exit;
    }

    if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
    {
        access = &Info->Parameters->CreateHandleInformation.DesiredAccess;
        desiredAccess = *access;
    }
    else
    {
        NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

        access = &Info->Parameters->DuplicateHandleInformation.DesiredAccess;
        desiredAccess = *access;
    }

    if (Info->ObjectType == *PsProcessType)
    {
        allowedAccessMask = process->ProcessAllowedMask;

        if (KphpShouldPermitCreatorProcess(Info, actor, process))
        {
            //
            // add permissions which are not present in the usual KPH_PROTECED_PROCESS_MASK
            //

            allowedAccessMask |= (PROCESS_VM_OPERATION |
                                  PROCESS_VM_WRITE);

            if ((allowedAccessMask & process->ProcessAllowedMask)
                != allowedAccessMask)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              PROTECTION,
                              "Permitting extra process handle access "
                              "(0x%08x -> 0x%08x) in creator process %lu for "
                              "process %lu",
                              process->ProcessAllowedMask,
                              allowedAccessMask,
                              HandleToULong(actor->ProcessContext->ProcessId),
                              HandleToULong(process->ProcessId));
            }
        }

        if ((desiredAccess & allowedAccessMask) != desiredAccess)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          PROTECTION,
                          "Stripping process handle (0x%08x -> 0x%08x) "
                          "permissions in process %lu for process %lu (0x%08x)",
                          desiredAccess,
                          (desiredAccess & allowedAccessMask),
                          HandleToULong(actor->ProcessContext->ProcessId),
                          HandleToULong(process->ProcessId),
                          allowedAccessMask);

            *access = (desiredAccess & allowedAccessMask);
        }
    }
    else
    {
        NT_ASSERT(Info->ObjectType == *PsThreadType);

        allowedAccessMask = process->ThreadAllowedMask;

        //if (KphpShouldPermitCreatorProcess(Info, actor, process))
        //{
        //    //
        //    // add permissions which are not present in the usual KPH_PROTECED_THREAD_MASK
        //    // currently none
        //    //
        //
        //    //allowedAccessMask |= 
        //
        //    if ((allowedAccessMask & process->ThreadAllowedMask)
        //        != allowedAccessMask)
        //    {
        //        KphTracePrint(TRACE_LEVEL_VERBOSE,
        //                      PROTECTION,
        //                      "Permitting extra thread handle access "
        //                      "(0x%08x -> 0x%08x) in creator process %lu for "
        //                      "process %lu",
        //                      process->ThreadAllowedMask,
        //                      allowedAccessMask,
        //                      HandleToULong(actor->ProcessContext->ProcessId),
        //                      HandleToULong(process->ProcessId));
        //    }
        //}

        if ((desiredAccess & allowedAccessMask) != desiredAccess)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          PROTECTION,
                          "Stripping thread handle (0x%08x -> 0x%08x) "
                          "permissions in process %lu for process %lu (0x%08x)",
                          desiredAccess,
                          (desiredAccess & allowedAccessMask),
                          HandleToULong(actor->ProcessContext->ProcessId),
                          HandleToULong(process->ProcessId),
                          allowedAccessMask);

            *access = (desiredAccess & allowedAccessMask);
        }
    }

Exit:

    if (process)
    {
        if (releaseLock)
        {
            KphReleaseRWLock(&process->ProtectionLock);
        }

        KphDereferenceObject(process);
    }

    if (actor)
    {
        KphDereferenceObject(actor);
    }

}

/**
 * \brief Cleanup routine for the image load APC.
 *
 * \param[in] Apc The APC to clean up.
 * \param[in] Reason Unused.
 */
_Function_class_(KSI_KCLEANUP_ROUTINE)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpImageLoadCleanupRoutine(
    _In_ PKSI_KAPC Apc,
    _In_ KSI_KAPC_CLEANUP_REASON Reason
    )
{
    PKPH_IMAGE_LOAD_APC apc;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Reason);

    apc = CONTAINING_RECORD(Apc, KPH_IMAGE_LOAD_APC, Apc);

    KphDereferenceObject(apc);
}

/**
 * \brief Normal kernel APC routine where we carry out image load denial.
 *
 * \param[in] NormalContext Unused.
 * \param[in] SystemArgument1 Image load APC object.
 * \param[in] SystemArgument2 Unused.
 */
_Function_class_(KNORMAL_ROUTINE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID NTAPI KphpImageLoadKernelNormalRoutine(
    _In_opt_ PVOID NormalContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )
{
    NTSTATUS status;
    PKPH_IMAGE_LOAD_APC apc;
    KAPC_STATE apcState;
    BOOLEAN attachToTarget;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument2);

    NT_ASSERT(SystemArgument1);

    apc = SystemArgument1;

    NT_ASSERT(apc->ImageBase <= MmHighestUserAddress);

    attachToTarget = (apc->Process->EProcess != PsGetCurrentProcess());
    if (attachToTarget)
    {
        KeStackAttachProcess(apc->Process->EProcess, &apcState);
    }

    status = ZwUnmapViewOfSection(ZwCurrentProcess(), apc->ImageBase);

    if (attachToTarget)
    {
        KeUnstackDetachProcess(&apcState);
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      PROTECTION,
                      "ZwUnmapViewOfSection failed (%lu, %p): %!STATUS!",
                      HandleToULong(apc->Process->ProcessId),
                      apc->ImageBase,
                      status);

        InterlockedIncrementSizeT(&apc->Process->NumberOfUntrustedImageLoads);
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      PROTECTION,
                      "Unmapped %p from process %lu",
                      apc->ImageBase,
                      HandleToULong(apc->Process->ProcessId));
    }
}

/**
 * \param Second kernel APC routine, say hi on the way down to passive :).
 *
 * \param[in] Apc Unused.
 * \param[in,out] NormalRoutine Pointer to the normal kernel APC routine.
 * \param[in,out] NormalContext Unused.
 * \param[in,out] SystemArgument1 Pointer to the image load APC object.
 * \param[in,out] SystemArgument2 Unused.
 */
_Function_class_(KSI_KKERNEL_ROUTINE)
_IRQL_requires_(APC_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpImageLoadKernelRoutineSecond(
    _In_ PKSI_KAPC Apc,
    _Inout_ _Deref_pre_maybenull_ PKNORMAL_ROUTINE* NormalRoutine,
    _Inout_ _Deref_pre_maybenull_ PVOID* NormalContext,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument1,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument2
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Apc);
    DBG_UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    DBG_UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    NT_ASSERT(*NormalRoutine == KphpImageLoadKernelNormalRoutine);
    NT_ASSERT(*SystemArgument1);
    NT_ASSERT(KphGetObjectType(*SystemArgument1) == KphpImageLoadApcType);
}

/**
 * \brief First kernel APC routine, fired by the thread returning from the
 * system. We will stage another normal kernel APC to fire at passive here.
 *
 * \param[in] Apc The APC object that is executing.
 * \param[in,out] NormalRoutine Points to the faked routine, we'll cancel it.
 * \param[in,out] NormalContext Unused.
 * \param[in,out] SystemArgument1 Unused.
 * \param[in,out] SystemArgument2 Unused.
 */
_Function_class_(KSI_KKERNEL_ROUTINE)
_IRQL_requires_(APC_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpImageLoadKernelRoutineFirst(
    _In_ PKSI_KAPC Apc,
    _Inout_ _Deref_pre_maybenull_ PKNORMAL_ROUTINE* NormalRoutine,
    _Inout_ _Deref_pre_maybenull_ PVOID* NormalContext,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument1,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument2
    )
{
    NTSTATUS status;
    PKPH_IMAGE_LOAD_APC firstApc;
    PKPH_IMAGE_LOAD_APC secondApc;
    KPH_IMAGE_LOAD_APC_INIT init;
#if DBG
    PKPH_THREAD_CONTEXT actor;
#endif

    PAGED_CODE();

    secondApc = NULL;

    firstApc = CONTAINING_RECORD(Apc, KPH_IMAGE_LOAD_APC, Apc);

    DBG_UNREFERENCED_PARAMETER(NormalRoutine);
#if DBG
    actor = KphGetCurrentThreadContext();
    NT_ASSERT(actor && actor->ProcessContext);
    NT_ASSERT(actor->ProcessContext->ApcNoopRoutine);
    NT_ASSERT(*NormalRoutine == actor->ProcessContext->ApcNoopRoutine);
    KphDereferenceObject(actor);
    actor = NULL;
#endif

    *NormalContext = NULL;
    *SystemArgument1 = NULL;
    *SystemArgument2 = NULL;

    init.Process = firstApc->Process;
    init.ImageBase = firstApc->ImageBase;
    init.FileObject = NULL;
    status = KphCreateObject(KphpImageLoadApcType,
                             sizeof(KPH_IMAGE_LOAD_APC),
                             &secondApc,
                             &init);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      PROTECTION,
                      "KphCreateObject failed: %!STATUS!",
                      status);


        secondApc = NULL;
        goto Exit;
    }

    //
    // Initialize a normal kernel ACP to drop down to passive.
    //
    KsiInitializeApc(&secondApc->Apc,
                     KphDriverObject,
                     KeGetCurrentThread(),
                     OriginalApcEnvironment,
                     KphpImageLoadKernelRoutineSecond,
                     KphpImageLoadCleanupRoutine,
                     KphpImageLoadKernelNormalRoutine,
                     KernelMode,
                     NULL);

    if (!KsiInsertQueueApc(&secondApc->Apc, secondApc, NULL, IO_NO_INCREMENT))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      PROTECTION,
                      "KsiInsertQueueApc failed");

        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    secondApc = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (secondApc)
    {
        KphDereferenceObject(secondApc);
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      PROTECTION,
                      "Untrusted image load (%lu, %p): %!STATUS!",
                      HandleToULong(firstApc->Process->ProcessId),
                      firstApc->ImageBase,
                      status);

        //
        // No choice other than to just track that there is an untrusted image.
        //
        InterlockedIncrementSizeT(&firstApc->Process->NumberOfUntrustedImageLoads);
    }
}

/**
 * \brief Handles an untrusted image load in a verified process.
 *
 * \details If a failure is encountered in this path or if image load denial
 * is disabled we will denote in the target process that an untrusted image
 * was loaded. The approach here is to queue a user APC to execute when
 * the thread returns from the system. Then, we issue another APC to drop
 * down to passive level and remove the image mapping from the process.
 *
 * \param[in,out] Process The process where the image is being loaded.
 * \param[in] ImageBase The base address of the untrusted image.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHandleUntrustedImageLoad(
    _Inout_ PKPH_PROCESS_CONTEXT Process,
    _In_ PVOID ImageBase
    )
{
    NTSTATUS status;
    PKPH_THREAD_CONTEXT actor;
    KPH_IMAGE_LOAD_APC_INIT init;
    PKPH_IMAGE_LOAD_APC apc;

    PAGED_PASSIVE();

    NT_ASSERT(Process->VerifiedProcess);

    status = STATUS_UNSUCCESSFUL;
    actor = NULL;
    apc = NULL;

    if (KphDynDisableImageLoadProtection)
    {
        goto Exit;
    }

    actor = KphGetCurrentThreadContext();
    if (!actor || !actor->ProcessContext)
    {
        KphTracePrint(TRACE_LEVEL_ERROR, PROTECTION, "Insufficient tracking.");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphCheckProcessApcNoopRoutine(actor->ProcessContext);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      PROTECTION,
                      "KphCheckProcessApcNoopRoutine failed: %!STATUS!",
                      status);
        goto Exit;
    }

    if (ImageBase > MmHighestUserAddress)
    {
        KphTracePrint(TRACE_LEVEL_ERROR, PROTECTION, "Invalid image base!");

        //
        // This isn't an error, we just check for safety downstream.
        //
        status = STATUS_SUCCESS;
        goto Exit;
    }

    init.Process = Process;
    init.ImageBase = ImageBase;
    init.FileObject = NULL;
    status = KphCreateObject(KphpImageLoadApcType,
                             sizeof(KPH_IMAGE_LOAD_APC),
                             &apc,
                             &init);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      PROTECTION,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        apc = NULL;
        goto Exit;
    }

    KsiInitializeApc(&apc->Apc,
                     KphDriverObject,
                     actor->EThread,
                     OriginalApcEnvironment,
                     KphpImageLoadKernelRoutineFirst,
                     KphpImageLoadCleanupRoutine,
                     actor->ProcessContext->ApcNoopRoutine,
                     UserMode,
                     NULL);

    if (!KsiInsertQueueApc(&apc->Apc, NULL, NULL, IO_NO_INCREMENT))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      PROTECTION,
                      "KsiInsertQueueApc failed");

        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    //
    // Stage the thread for user mode APC delivery.
    //
    KeTestAlertThread(UserMode);

    apc = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (apc)
    {
        KphDereferenceObject(apc);
    }

    if (actor)
    {
        KphDereferenceObject(actor);
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      PROTECTION,
                      "Untrusted image load (%lu, %p): %!STATUS!",
                      HandleToULong(Process->ProcessId),
                      ImageBase,
                      status);

        //
        // No choice other than to just track that there is an untrusted image.
        //
        InterlockedIncrementSizeT(&Process->NumberOfUntrustedImageLoads);
    }
}

/**
 * \brief Applies image protections on verified processes.
 *
 * \param[in,out] Process The process where the image is being loaded.
 * \param[in] ImageBase The base address of the image.
 * \param[in] FileObject The file object for the image being loaded.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpApplyImageProtections(
    _Inout_ PKPH_PROCESS_CONTEXT Process,
    _In_ PVOID ImageBase,
    _In_ PFILE_OBJECT FileObject
    )
{
    NTSTATUS status;
    KPH_SIGNING_INFO info;
    ANSI_STRING issuer;
    ANSI_STRING subject;
    PUNICODE_STRING fileName;

    PAGED_PASSIVE();

    NT_ASSERT(!KeAreAllApcsDisabled());

    fileName = NULL;

    if (!FileObject->SectionObjectPointer ||
        MmDoesFileHaveUserWritableReferences(FileObject->SectionObjectPointer))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      PROTECTION,
                      "%lu image has user writable references",
                      HandleToULong(Process->ProcessId));

        KphpHandleUntrustedImageLoad(Process, ImageBase);
        goto Exit;
    }

    status = KphGetNameFileObject(FileObject, &fileName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      PROTECTION,
                      "%lu KphGetNameFileObject failed: %!STATUS!",
                      HandleToULong(Process->ProcessId),
                      status);

        fileName = NULL;
        KphpHandleUntrustedImageLoad(Process, ImageBase);
        goto Exit;
    }

    //
    // Do the verification since it will be a faster check than asking CI.
    //

    status = KphVerifyFile(fileName);

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  PROTECTION,
                  "KphVerifyFile: %lu \"%wZ\": %!STATUS!",
                  HandleToULong(Process->ProcessId),
                  fileName,
                  status);

    if (NT_SUCCESS(status))
    {
        InterlockedIncrementSizeT(&Process->NumberOfVerifiedImageLoads);
        goto Exit;
    }

    //
    // We have to ask CI for the signing information.
    //

    RtlZeroMemory(&issuer, sizeof(issuer));
    RtlZeroMemory(&subject, sizeof(subject));

    status = KphGetSigningInfoByFileName(fileName, &info);

    //
    // Pull out the issuer and subject if it's there. The size check for
    // PolicyInfo.Size is correct. Checking for when the ChainInfo started
    // exposing the chain elements (Win10).
    //
    if ((info.PolicyInfo.Size >=
         RTL_SIZEOF_THROUGH_FIELD(MINCRYPT_POLICY_INFO, ValidToTime)) &&
        info.PolicyInfo.ChainInfo &&
        info.PolicyInfo.ChainInfo->ChainElements &&
        (info.PolicyInfo.ChainInfo->NumberOfChainElements > 0))
    {
        issuer.Buffer = info.PolicyInfo.ChainInfo->ChainElements[0].Issuer.Buffer;
        issuer.Length = info.PolicyInfo.ChainInfo->ChainElements[0].Issuer.Length;
        issuer.MaximumLength = issuer.Length;

        subject.Buffer = info.PolicyInfo.ChainInfo->ChainElements[0].Subject.Buffer;
        subject.Length = info.PolicyInfo.ChainInfo->ChainElements[0].Subject.Length;
        subject.MaximumLength = subject.Length;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  PROTECTION,
                  "KphGetSigningInfoByFileName: \"%wZ\" 0x%08lx \"%wZ\" "
                  "\"%Z\" \"%Z\" %!STATUS! %!STATUS!",
                  fileName,
                  info.PolicyInfo.PolicyBits,
                  &info.CatalogName,
                  &subject,
                  &issuer,
                  info.PolicyInfo.VerificationStatus,
                  status);

    if (!NT_SUCCESS(status) ||
        !NT_SUCCESS(info.PolicyInfo.VerificationStatus) ||
        BooleanFlagOn(info.PolicyInfo.PolicyBits,
                      (MINCRYPT_POLICY_ERROR_FLAGS |
                       MINCRYPT_POLICY_3RD_PARTY_ROOT |
                       MINCRYPT_POLICY_NO_ROOT |
                       MINCRYPT_POLICY_OTHER_ROOT)))
    {
        KphpHandleUntrustedImageLoad(Process, ImageBase);
    }
    else
    {
        InterlockedIncrementSizeT(&Process->NumberOfMicrosoftImageLoads);
    }

    KphFreeSigningInfo(&info);

Exit:

    if (fileName)
    {
        KphFreeNameFileObject(fileName);
    }

}

/**
 * \brief Normal kernel APC routine for when we must get outside of APC
 * disablement.
 *
 * \param[in] NormalContext Unused.
 * \param[in] SystemArgument1 Image load APC object.
 * \param[in] SystemArgument2 Unused.
 */
_Function_class_(KNORMAL_ROUTINE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID NTAPI KphpImageLoadKernelNormalRoutineApcsDisabled(
    _In_opt_ PVOID NormalContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )
{
    PKPH_IMAGE_LOAD_APC apc;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument2);

    NT_ASSERT(SystemArgument1);

    apc = SystemArgument1;

    NT_ASSERT(apc->FileObject);

    KphpApplyImageProtections(apc->Process, apc->ImageBase, apc->FileObject);
}

/**
 * \brief Kernel APC routine for when we must get outside of APC disablement.
 *
 * \param[in] Apc Unused.
 * \param[in,out] NormalRoutine Pointer to the normal kernel APC routine.
 * \param[in,out] NormalContext Unused.
 * \param[in,out] SystemArgument1 Pointer to the image load APC object.
 * \param[in,out] SystemArgument2 Unused.
 */
_Function_class_(KSI_KKERNEL_ROUTINE)
_IRQL_requires_(APC_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpImageLoadKernelRoutineApcsDisabled(
    _In_ PKSI_KAPC Apc,
    _Inout_ _Deref_pre_maybenull_ PKNORMAL_ROUTINE* NormalRoutine,
    _Inout_ _Deref_pre_maybenull_ PVOID* NormalContext,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument1,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument2
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Apc);
    DBG_UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    DBG_UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    NT_ASSERT(*NormalRoutine == KphpImageLoadKernelNormalRoutineApcsDisabled);
    NT_ASSERT(*SystemArgument1);
    NT_ASSERT(KphGetObjectType(*SystemArgument1) == KphpImageLoadApcType);
}

/**
 * \brief Applies image protections on verified processes.
 *
 * \param[in,out] Process The process where the image is being loaded.
 * \param[in] ImageBase The base address of the image.
 * \param[in] FileObject The file object for the image being loaded.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpApplyImageProtectionsApcsDisabled(
    _Inout_ PKPH_PROCESS_CONTEXT Process,
    _In_ PVOID ImageBase,
    _In_ PFILE_OBJECT FileObject
    )
{
    NTSTATUS status;
    PKPH_IMAGE_LOAD_APC apc;
    KPH_IMAGE_LOAD_APC_INIT init;

    PAGED_PASSIVE();

    NT_ASSERT(KeAreAllApcsDisabled());

    init.Process = Process;
    init.ImageBase = ImageBase;
    init.FileObject = FileObject;
    status = KphCreateObject(KphpImageLoadApcType,
                             sizeof(KPH_IMAGE_LOAD_APC),
                             &apc,
                             &init);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      PROTECTION,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        apc = NULL;
        goto Exit;
    }

    //
    // Initialize a normal kernel ACP to handle this outside of the callback.
    //
    KsiInitializeApc(&apc->Apc,
                     KphDriverObject,
                     KeGetCurrentThread(),
                     OriginalApcEnvironment,
                     KphpImageLoadKernelRoutineApcsDisabled,
                     KphpImageLoadCleanupRoutine,
                     KphpImageLoadKernelNormalRoutineApcsDisabled,
                     KernelMode,
                     NULL);

    if (!KsiInsertQueueApc(&apc->Apc, apc, NULL, IO_NO_INCREMENT))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      PROTECTION,
                      "KsiInsertQueueApc failed");

        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    apc = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (apc)
    {
        KphDereferenceObject(apc);
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      PROTECTION,
                      "Untrusted image load (%lu, %p): %!STATUS!",
                      HandleToULong(Process->ProcessId),
                      ImageBase,
                      status);

        //
        // No choice other than to just track that there is an untrusted image.
        //
        InterlockedIncrementSizeT(&Process->NumberOfUntrustedImageLoads);
    }
}

/**
 * \brief Applies image protections on verified processes.
 *
 * \param[in,out] Process The process where the image is being loaded.
 * \param[in] ImageInfo The image info from the notification routine.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphApplyImageProtections(
    _Inout_ PKPH_PROCESS_CONTEXT Process,
    _In_ PIMAGE_INFO_EX ImageInfo
    )
{
    PAGED_PASSIVE();

    KphAcquireRWLockShared(&Process->ProtectionLock);

    if (!Process->VerifiedProcess || !Process->Protected)
    {
        goto Exit;
    }

    if ((ImageInfo->ImageInfo.ImageSignatureLevel == SE_SIGNING_LEVEL_MICROSOFT) ||
        (ImageInfo->ImageInfo.ImageSignatureLevel == SE_SIGNING_LEVEL_WINDOWS) ||
        (ImageInfo->ImageInfo.ImageSignatureLevel == SE_SIGNING_LEVEL_WINDOWS_TCB))
    {
        InterlockedIncrementSizeT(&Process->NumberOfMicrosoftImageLoads);
        goto Exit;
    }

    if (ImageInfo->ImageInfo.ImageSignatureLevel == SE_SIGNING_LEVEL_ANTIMALWARE)
    {
        InterlockedIncrementSizeT(&Process->NumberOfAntimalwareImageLoads);
        goto Exit;
    }

    //
    // We have to do a signing check ourselves which involves looking up the
    // file name. If we can do it now, do so, otherwise handle it later.
    //

    if (KeAreAllApcsDisabled())
    {
        //
        // We can't safely (or accurately) look up the file name. We have to
        // special case it.
        //
        KphpApplyImageProtectionsApcsDisabled(Process,
                                              ImageInfo->ImageInfo.ImageBase,
                                              ImageInfo->FileObject);
    }
    else
    {
        KphpApplyImageProtections(Process,
                                  ImageInfo->ImageInfo.ImageBase,
                                  ImageInfo->FileObject);
    }

Exit:

    KphReleaseRWLock(&Process->ProtectionLock);

}
