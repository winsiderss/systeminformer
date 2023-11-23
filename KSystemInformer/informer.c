/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#include <kph.h>
#include <comms.h>
#define KPH_INFORMER_P
#include <informer.h>

#include <trace.h>

typedef struct _KPH_INFORMER_MAP_ENTRY
{
    KPH_MESSAGE_ID MessageId;
    PCKPH_INFORMER_SETTINGS Setting;
} KPH_INFORMER_MAP_ENTRY, *PKPH_INFORMER_MAP_ENTRY;

#define KPH_INFORMER_MAP_SETTING(name) { KphMsg##name, &KphInformer##name }

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
const KPH_INFORMER_MAP_ENTRY KphpInformerMap[] =
{
    //
    // N.B. Order here matters for KphInformerForMessageId.
    //

    KPH_INFORMER_MAP_SETTING(ProcessCreate),
    KPH_INFORMER_MAP_SETTING(ProcessExit),
    KPH_INFORMER_MAP_SETTING(ThreadCreate),
    KPH_INFORMER_MAP_SETTING(ThreadExecute),
    KPH_INFORMER_MAP_SETTING(ThreadExit),
    KPH_INFORMER_MAP_SETTING(ImageLoad),
    KPH_INFORMER_MAP_SETTING(DebugPrint),
    KPH_INFORMER_MAP_SETTING(ProcessHandlePreCreate),
    KPH_INFORMER_MAP_SETTING(ProcessHandlePostCreate),
    KPH_INFORMER_MAP_SETTING(ProcessHandlePreDuplicate),
    KPH_INFORMER_MAP_SETTING(ProcessHandlePostDuplicate),
    KPH_INFORMER_MAP_SETTING(ThreadHandlePreCreate),
    KPH_INFORMER_MAP_SETTING(ThreadHandlePostCreate),
    KPH_INFORMER_MAP_SETTING(ThreadHandlePreDuplicate),
    KPH_INFORMER_MAP_SETTING(ThreadHandlePostDuplicate),
    KPH_INFORMER_MAP_SETTING(DesktopHandlePreCreate),
    KPH_INFORMER_MAP_SETTING(DesktopHandlePostCreate),
    KPH_INFORMER_MAP_SETTING(DesktopHandlePreDuplicate),
    KPH_INFORMER_MAP_SETTING(DesktopHandlePostDuplicate),
    { KphMsgRequiredStateFailure, NULL },
    KPH_INFORMER_MAP_SETTING(FilePreCreate),
    KPH_INFORMER_MAP_SETTING(FilePostCreate),
    KPH_INFORMER_MAP_SETTING(FilePreCreateNamedPipe),
    KPH_INFORMER_MAP_SETTING(FilePostCreateNamedPipe),
    KPH_INFORMER_MAP_SETTING(FilePreClose),
    KPH_INFORMER_MAP_SETTING(FilePostClose),
    KPH_INFORMER_MAP_SETTING(FilePreRead),
    KPH_INFORMER_MAP_SETTING(FilePostRead),
    KPH_INFORMER_MAP_SETTING(FilePreWrite),
    KPH_INFORMER_MAP_SETTING(FilePostWrite),
    KPH_INFORMER_MAP_SETTING(FilePreQueryInformation),
    KPH_INFORMER_MAP_SETTING(FilePostQueryInformation),
    KPH_INFORMER_MAP_SETTING(FilePreSetInformation),
    KPH_INFORMER_MAP_SETTING(FilePostSetInformation),
    KPH_INFORMER_MAP_SETTING(FilePreQueryEa),
    KPH_INFORMER_MAP_SETTING(FilePostQueryEa),
    KPH_INFORMER_MAP_SETTING(FilePreSetEa),
    KPH_INFORMER_MAP_SETTING(FilePostSetEa),
    KPH_INFORMER_MAP_SETTING(FilePreFlushBuffers),
    KPH_INFORMER_MAP_SETTING(FilePostFlushBuffers),
    KPH_INFORMER_MAP_SETTING(FilePreQueryVolumeInformation),
    KPH_INFORMER_MAP_SETTING(FilePostQueryVolumeInformation),
    KPH_INFORMER_MAP_SETTING(FilePreSetVolumeInformation),
    KPH_INFORMER_MAP_SETTING(FilePostSetVolumeInformation),
    KPH_INFORMER_MAP_SETTING(FilePreDirectoryControl),
    KPH_INFORMER_MAP_SETTING(FilePostDirectoryControl),
    KPH_INFORMER_MAP_SETTING(FilePreFileSystemControl),
    KPH_INFORMER_MAP_SETTING(FilePostFileSystemControl),
    KPH_INFORMER_MAP_SETTING(FilePreDeviceControl),
    KPH_INFORMER_MAP_SETTING(FilePostDeviceControl),
    KPH_INFORMER_MAP_SETTING(FilePreInternalDeviceControl),
    KPH_INFORMER_MAP_SETTING(FilePostInternalDeviceControl),
    KPH_INFORMER_MAP_SETTING(FilePreShutdown),
    KPH_INFORMER_MAP_SETTING(FilePostShutdown),
    KPH_INFORMER_MAP_SETTING(FilePreLockControl),
    KPH_INFORMER_MAP_SETTING(FilePostLockControl),
    KPH_INFORMER_MAP_SETTING(FilePreCleanup),
    KPH_INFORMER_MAP_SETTING(FilePostCleanup),
    KPH_INFORMER_MAP_SETTING(FilePreCreateMailslot),
    KPH_INFORMER_MAP_SETTING(FilePostCreateMailslot),
    KPH_INFORMER_MAP_SETTING(FilePreQuerySecurity),
    KPH_INFORMER_MAP_SETTING(FilePostQuerySecurity),
    KPH_INFORMER_MAP_SETTING(FilePreSetSecurity),
    KPH_INFORMER_MAP_SETTING(FilePostSetSecurity),
    KPH_INFORMER_MAP_SETTING(FilePrePower),
    KPH_INFORMER_MAP_SETTING(FilePostPower),
    KPH_INFORMER_MAP_SETTING(FilePreSystemControl),
    KPH_INFORMER_MAP_SETTING(FilePostSystemControl),
    KPH_INFORMER_MAP_SETTING(FilePreDeviceChange),
    KPH_INFORMER_MAP_SETTING(FilePostDeviceChange),
    KPH_INFORMER_MAP_SETTING(FilePreQueryQuota),
    KPH_INFORMER_MAP_SETTING(FilePostQueryQuota),
    KPH_INFORMER_MAP_SETTING(FilePreSetQuota),
    KPH_INFORMER_MAP_SETTING(FilePostSetQuota),
    KPH_INFORMER_MAP_SETTING(FilePrePnp),
    KPH_INFORMER_MAP_SETTING(FilePostPnp),
    KPH_INFORMER_MAP_SETTING(FilePreAcquireForSectionSync),
    KPH_INFORMER_MAP_SETTING(FilePostAcquireForSectionSync),
    KPH_INFORMER_MAP_SETTING(FilePreReleaseForSectionSync),
    KPH_INFORMER_MAP_SETTING(FilePostReleaseForSectionSync),
    KPH_INFORMER_MAP_SETTING(FilePreAcquireForModWrite),
    KPH_INFORMER_MAP_SETTING(FilePostAcquireForModWrite),
    KPH_INFORMER_MAP_SETTING(FilePreReleaseForModWrite),
    KPH_INFORMER_MAP_SETTING(FilePostReleaseForModWrite),
    KPH_INFORMER_MAP_SETTING(FilePreAcquireForCcFlush),
    KPH_INFORMER_MAP_SETTING(FilePostAcquireForCcFlush),
    KPH_INFORMER_MAP_SETTING(FilePreReleaseForCcFlush),
    KPH_INFORMER_MAP_SETTING(FilePostReleaseForCcFlush),
    KPH_INFORMER_MAP_SETTING(FilePreQueryOpen),
    KPH_INFORMER_MAP_SETTING(FilePostQueryOpen),
    KPH_INFORMER_MAP_SETTING(FilePreFastIoCheckIfPossible),
    KPH_INFORMER_MAP_SETTING(FilePostFastIoCheckIfPossible),
    KPH_INFORMER_MAP_SETTING(FilePreNetworkQueryOpen),
    KPH_INFORMER_MAP_SETTING(FilePostNetworkQueryOpen),
    KPH_INFORMER_MAP_SETTING(FilePreMdlRead),
    KPH_INFORMER_MAP_SETTING(FilePostMdlRead),
    KPH_INFORMER_MAP_SETTING(FilePreMdlReadComplete),
    KPH_INFORMER_MAP_SETTING(FilePostMdlReadComplete),
    KPH_INFORMER_MAP_SETTING(FilePrePrepareMdlWrite),
    KPH_INFORMER_MAP_SETTING(FilePostPrepareMdlWrite),
    KPH_INFORMER_MAP_SETTING(FilePreMdlWriteComplete),
    KPH_INFORMER_MAP_SETTING(FilePostMdlWriteComplete),
    KPH_INFORMER_MAP_SETTING(FilePreVolumeMount),
    KPH_INFORMER_MAP_SETTING(FilePostVolumeMount),
    KPH_INFORMER_MAP_SETTING(FilePreVolumeDismount),
    KPH_INFORMER_MAP_SETTING(FilePostVolumeDismount),
    KPH_INFORMER_MAP_SETTING(RegPreDeleteKey),
    KPH_INFORMER_MAP_SETTING(RegPostDeleteKey),
    KPH_INFORMER_MAP_SETTING(RegPreSetValueKey),
    KPH_INFORMER_MAP_SETTING(RegPostSetValueKey),
    KPH_INFORMER_MAP_SETTING(RegPreDeleteValueKey),
    KPH_INFORMER_MAP_SETTING(RegPostDeleteValueKey),
    KPH_INFORMER_MAP_SETTING(RegPreSetInformationKey),
    KPH_INFORMER_MAP_SETTING(RegPostSetInformationKey),
    KPH_INFORMER_MAP_SETTING(RegPreRenameKey),
    KPH_INFORMER_MAP_SETTING(RegPostRenameKey),
    KPH_INFORMER_MAP_SETTING(RegPreEnumerateKey),
    KPH_INFORMER_MAP_SETTING(RegPostEnumerateKey),
    KPH_INFORMER_MAP_SETTING(RegPreEnumerateValueKey),
    KPH_INFORMER_MAP_SETTING(RegPostEnumerateValueKey),
    KPH_INFORMER_MAP_SETTING(RegPreQueryKey),
    KPH_INFORMER_MAP_SETTING(RegPostQueryKey),
    KPH_INFORMER_MAP_SETTING(RegPreQueryValueKey),
    KPH_INFORMER_MAP_SETTING(RegPostQueryValueKey),
    KPH_INFORMER_MAP_SETTING(RegPreQueryMultipleValueKey),
    KPH_INFORMER_MAP_SETTING(RegPostQueryMultipleValueKey),
    KPH_INFORMER_MAP_SETTING(RegPreKeyHandleClose),
    KPH_INFORMER_MAP_SETTING(RegPostKeyHandleClose),
    KPH_INFORMER_MAP_SETTING(RegPreCreateKey),
    KPH_INFORMER_MAP_SETTING(RegPostCreateKey),
    KPH_INFORMER_MAP_SETTING(RegPreOpenKey),
    KPH_INFORMER_MAP_SETTING(RegPostOpenKey),
    KPH_INFORMER_MAP_SETTING(RegPreFlushKey),
    KPH_INFORMER_MAP_SETTING(RegPostFlushKey),
    KPH_INFORMER_MAP_SETTING(RegPreLoadKey),
    KPH_INFORMER_MAP_SETTING(RegPostLoadKey),
    KPH_INFORMER_MAP_SETTING(RegPreUnLoadKey),
    KPH_INFORMER_MAP_SETTING(RegPostUnLoadKey),
    KPH_INFORMER_MAP_SETTING(RegPreQueryKeySecurity),
    KPH_INFORMER_MAP_SETTING(RegPostQueryKeySecurity),
    KPH_INFORMER_MAP_SETTING(RegPreSetKeySecurity),
    KPH_INFORMER_MAP_SETTING(RegPostSetKeySecurity),
    KPH_INFORMER_MAP_SETTING(RegPreRestoreKey),
    KPH_INFORMER_MAP_SETTING(RegPostRestoreKey),
    KPH_INFORMER_MAP_SETTING(RegPreSaveKey),
    KPH_INFORMER_MAP_SETTING(RegPostSaveKey),
    KPH_INFORMER_MAP_SETTING(RegPreReplaceKey),
    KPH_INFORMER_MAP_SETTING(RegPostReplaceKey),
    KPH_INFORMER_MAP_SETTING(RegPreQueryKeyName),
    KPH_INFORMER_MAP_SETTING(RegPostQueryKeyName),
    KPH_INFORMER_MAP_SETTING(RegPreSaveMergedKey),
    KPH_INFORMER_MAP_SETTING(RegPostSaveMergedKey),
};
C_ASSERT((ARRAYSIZE(KphpInformerMap) + (MaxKphMsgClientAllowed + 1)) == MaxKphMsg);
KPH_PROTECTED_DATA_SECTION_RO_POP();

_Must_inspect_result_
PCKPH_INFORMER_SETTINGS KphInformerForMessageId(
    _In_ KPH_MESSAGE_ID MessageId
    )
{
    if (NT_VERIFY(MessageId > MaxKphMsgClientAllowed) && (MessageId < MaxKphMsg))
    {
        return KphpInformerMap[MessageId - (MaxKphMsgClientAllowed + 1)].Setting;
    }

    return NULL;
}

/**
 * \brief Checks if the informer is filtered for a process.
 *
 * \param[in] Settings The settings to check.
 * \param[in] Process The process to check.
 *
 * \return TRUE if the informer is filtered, FALSE otherwise.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
BOOLEAN KphpInformerProcessIsFiltered(
    _In_ PCKPH_INFORMER_SETTINGS Settings,
    _In_ PKPH_PROCESS_CONTEXT Process
    )
{
    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    if (FlagOn(Process->InformerFilter.Flags, Settings->Flags) ||
        FlagOn(Process->InformerFilter.Flags2, Settings->Flags2) ||
        FlagOn(Process->InformerFilter.Flags3, Settings->Flags3))
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * \brief Checks if the informer is enabled.
 *
 * \param[in] Settings The settings to check.
 * \param[in] Process The process to check.
 *
 * \return TRUE if the informer is enabled, FALSE otherwise.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
BOOLEAN KphInformerIsEnabled(
    _In_ PCKPH_INFORMER_SETTINGS Settings,
    _In_opt_ PKPH_PROCESS_CONTEXT Process
    )
{
    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    if (!KphCommsInformerEnabled(Settings))
    {
        return FALSE;
    }

    if (Process && KphpInformerProcessIsFiltered(Settings, Process))
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief Checks if the informer is enabled.
 *
 * \param[in] Settings The settings to check.
 * \param[in] ActorProcess The actor process check for filtering.
 * \param[in] TargetProcess The target process check for filtering.
 *
 * \return TRUE if the informer is enabled, FALSE otherwise.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
BOOLEAN KphInformerIsEnabled2(
    _In_ PCKPH_INFORMER_SETTINGS Settings,
    _In_opt_ PKPH_PROCESS_CONTEXT ActorProcess,
    _In_opt_ PKPH_PROCESS_CONTEXT TargetProcess
    )
{
    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    if (!KphCommsInformerEnabled(Settings))
    {
        return FALSE;
    }

    if ((ActorProcess && KphpInformerProcessIsFiltered(Settings, ActorProcess)) &&
        (TargetProcess && KphpInformerProcessIsFiltered(Settings, TargetProcess)))
    {
        return FALSE;
    }

    return TRUE;
}

PAGED_FILE();

/**
 * \brief Gets informer filtering for a process.
 *
 * \param[in] Process Handle to the process to get filtering for.
 * \param[in] Filter Populated with the filter settings.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetInformerProcessFilter(
    _In_ HANDLE ProcessHandle,
    _Out_ PKPH_INFORMER_SETTINGS Filter,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS processObject;
    PKPH_PROCESS_CONTEXT processContext;

    PAGED_CODE_PASSIVE();

    processObject = NULL;
    processContext = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(Filter, KPH_INFORMER_SETTINGS);
            RtlZeroMemory(Filter, sizeof(KPH_INFORMER_SETTINGS));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        RtlZeroMemory(Filter, sizeof(KPH_INFORMER_SETTINGS));
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &processObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        processObject = NULL;
        goto Exit;
    }

    processContext = KphGetEProcessContext(processObject);
    if (!processContext)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphGetEProcessContext return null.");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            Filter->Flags = processContext->InformerFilter.Flags;
            Filter->Flags2 = processContext->InformerFilter.Flags2;
            Filter->Flags3 = processContext->InformerFilter.Flags3;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        Filter->Flags = processContext->InformerFilter.Flags;
        Filter->Flags2 = processContext->InformerFilter.Flags2;
        Filter->Flags3 = processContext->InformerFilter.Flags3;
    }

    status = STATUS_SUCCESS;

Exit:

    if (processContext)
    {
        KphDereferenceObject(processContext);
    }

    if (processObject)
    {
        ObDereferenceObject(processObject);
    }

    return status;
}

/**
 * \brief Callback for setting all process informer filters.
 *
 * \param[in] Process The process to set the informer filter for.
 * \param[in] Parameter The filter settings to apply.
 *
 * \return FALSE
 */
_Function_class_(KPH_ENUM_PROCESS_CONTEXTS_CALLBACK)
_Must_inspect_result_
BOOLEAN KSIAPI KphpSetInformerProcessFilter(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_INFORMER_SETTINGS filter;

    PAGED_CODE_PASSIVE();

    NT_ASSERT(Parameter);

    filter = Parameter;

    InterlockedExchangeU64(&Process->InformerFilter.Flags, filter->Flags);
    InterlockedExchangeU64(&Process->InformerFilter.Flags2, filter->Flags2);
    InterlockedExchangeU64(&Process->InformerFilter.Flags3, filter->Flags3);

    return FALSE;
}

/**
 * \brief Sets informer filtering for a process.
 *
 * \param[in] Filter The filter settings to apply.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpSetInformerProcessFilterAll(
    _In_ PKPH_INFORMER_SETTINGS Filter
    )
{
    PAGED_CODE_PASSIVE();

    KphEnumerateProcessContexts(KphpSetInformerProcessFilter, Filter);
}

/**
 * \brief Sets informer filtering for a process.
 *
 * \param[in] Process Handle to the process to set filtering for.
 * \param[in] Filter The filter settings to apply.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformerProcessFilter(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PKPH_INFORMER_SETTINGS Filter,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS processObject;
    PKPH_PROCESS_CONTEXT processContext;
    KPH_INFORMER_SETTINGS filter;

    PAGED_CODE_PASSIVE();

    processObject = NULL;
    processContext = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeInputType(Filter, KPH_INFORMER_SETTINGS);
            filter = *Filter;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        filter = *Filter;
    }

    if (ProcessHandle)
    {
        status = ObReferenceObjectByHandle(ProcessHandle,
                                           0,
                                           *PsProcessType,
                                           AccessMode,
                                           &processObject,
                                           NULL);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "ObReferenceObjectByHandle failed: %!STATUS!",
                          status);

            processObject = NULL;
            goto Exit;
        }

        processContext = KphGetEProcessContext(processObject);
        if (!processContext)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "KphGetEProcessContext return null.");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        (VOID)KphpSetInformerProcessFilter(processContext, &filter);
        status = STATUS_SUCCESS;
    }
    else
    {
        KphpSetInformerProcessFilterAll(&filter);
        status = STATUS_SUCCESS;
    }

Exit:

    if (processContext)
    {
        KphDereferenceObject(processContext);
    }

    if (processObject)
    {
        ObDereferenceObject(processObject);
    }

    return status;
}
