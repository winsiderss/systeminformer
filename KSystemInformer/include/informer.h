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

#pragma once
#include <kph.h>
#include <kphmsg.h>

#ifdef KPH_INFORMER_P
#define KPH_DEFINE_INFORMER_SETTING(name)                                      \
    const KPH_INFORMER_SETTINGS KphInformer##name = { .name = TRUE }
#else
#define KPH_DEFINE_INFORMER_SETTING(name)                                      \
    extern const KPH_INFORMER_SETTINGS KphInformer##name
#endif

#ifdef KPH_INFORMER_P
KPH_PROTECTED_DATA_SECTION_RO_PUSH();
#endif

KPH_DEFINE_INFORMER_SETTING(ProcessCreate);
KPH_DEFINE_INFORMER_SETTING(ProcessExit);
KPH_DEFINE_INFORMER_SETTING(ThreadCreate);
KPH_DEFINE_INFORMER_SETTING(ThreadExecute);
KPH_DEFINE_INFORMER_SETTING(ThreadExit);
KPH_DEFINE_INFORMER_SETTING(ImageLoad);
KPH_DEFINE_INFORMER_SETTING(DebugPrint);
KPH_DEFINE_INFORMER_SETTING(HandlePreCreateProcess);
KPH_DEFINE_INFORMER_SETTING(HandlePostCreateProcess);
KPH_DEFINE_INFORMER_SETTING(HandlePreDuplicateProcess);
KPH_DEFINE_INFORMER_SETTING(HandlePostDuplicateProcess);
KPH_DEFINE_INFORMER_SETTING(HandlePreCreateThread);
KPH_DEFINE_INFORMER_SETTING(HandlePostCreateThread);
KPH_DEFINE_INFORMER_SETTING(HandlePreDuplicateThread);
KPH_DEFINE_INFORMER_SETTING(HandlePostDuplicateThread);
KPH_DEFINE_INFORMER_SETTING(HandlePreCreateDesktop);
KPH_DEFINE_INFORMER_SETTING(HandlePostCreateDesktop);
KPH_DEFINE_INFORMER_SETTING(HandlePreDuplicateDesktop);
KPH_DEFINE_INFORMER_SETTING(HandlePostDuplicateDesktop);
KPH_DEFINE_INFORMER_SETTING(EnableStackTraces);
KPH_DEFINE_INFORMER_SETTING(EnableProcessCreateReply);
KPH_DEFINE_INFORMER_SETTING(FileEnablePreCreateReply);
KPH_DEFINE_INFORMER_SETTING(FileEnablePostCreateReply);
KPH_DEFINE_INFORMER_SETTING(FileEnablePostFileNames);
KPH_DEFINE_INFORMER_SETTING(FileEnablePagingIo);
KPH_DEFINE_INFORMER_SETTING(FileEnableSyncPagingIo);
KPH_DEFINE_INFORMER_SETTING(FileEnableIoControlBuffers);
KPH_DEFINE_INFORMER_SETTING(FileEnableFsControlBuffers);
KPH_DEFINE_INFORMER_SETTING(FileEnableDirControlBuffers);
KPH_DEFINE_INFORMER_SETTING(FilePreCreate);
KPH_DEFINE_INFORMER_SETTING(FilePostCreate);
KPH_DEFINE_INFORMER_SETTING(FilePreCreateNamedPipe);
KPH_DEFINE_INFORMER_SETTING(FilePostCreateNamedPipe);
KPH_DEFINE_INFORMER_SETTING(FilePreClose);
KPH_DEFINE_INFORMER_SETTING(FilePostClose);
KPH_DEFINE_INFORMER_SETTING(FilePreRead);
KPH_DEFINE_INFORMER_SETTING(FilePostRead);
KPH_DEFINE_INFORMER_SETTING(FilePreWrite);
KPH_DEFINE_INFORMER_SETTING(FilePostWrite);
KPH_DEFINE_INFORMER_SETTING(FilePreQueryInformation);
KPH_DEFINE_INFORMER_SETTING(FilePostQueryInformation);
KPH_DEFINE_INFORMER_SETTING(FilePreSetInformation);
KPH_DEFINE_INFORMER_SETTING(FilePostSetInformation);
KPH_DEFINE_INFORMER_SETTING(FilePreQueryEa);
KPH_DEFINE_INFORMER_SETTING(FilePostQueryEa);
KPH_DEFINE_INFORMER_SETTING(FilePreSetEa);
KPH_DEFINE_INFORMER_SETTING(FilePostSetEa);
KPH_DEFINE_INFORMER_SETTING(FilePreFlushBuffers);
KPH_DEFINE_INFORMER_SETTING(FilePostFlushBuffers);
KPH_DEFINE_INFORMER_SETTING(FilePreQueryVolumeInformation);
KPH_DEFINE_INFORMER_SETTING(FilePostQueryVolumeInformation);
KPH_DEFINE_INFORMER_SETTING(FilePreSetVolumeInformation);
KPH_DEFINE_INFORMER_SETTING(FilePostSetVolumeInformation);
KPH_DEFINE_INFORMER_SETTING(FilePreDirectoryControl);
KPH_DEFINE_INFORMER_SETTING(FilePostDirectoryControl);
KPH_DEFINE_INFORMER_SETTING(FilePreFileSystemControl);
KPH_DEFINE_INFORMER_SETTING(FilePostFileSystemControl);
KPH_DEFINE_INFORMER_SETTING(FilePreDeviceControl);
KPH_DEFINE_INFORMER_SETTING(FilePostDeviceControl);
KPH_DEFINE_INFORMER_SETTING(FilePreInternalDeviceControl);
KPH_DEFINE_INFORMER_SETTING(FilePostInternalDeviceControl);
KPH_DEFINE_INFORMER_SETTING(FilePreShutdown);
KPH_DEFINE_INFORMER_SETTING(FilePostShutdown);
KPH_DEFINE_INFORMER_SETTING(FilePreLockControl);
KPH_DEFINE_INFORMER_SETTING(FilePostLockControl);
KPH_DEFINE_INFORMER_SETTING(FilePreCleanup);
KPH_DEFINE_INFORMER_SETTING(FilePostCleanup);
KPH_DEFINE_INFORMER_SETTING(FilePreCreateMailslot);
KPH_DEFINE_INFORMER_SETTING(FilePostCreateMailslot);
KPH_DEFINE_INFORMER_SETTING(FilePreQuerySecurity);
KPH_DEFINE_INFORMER_SETTING(FilePostQuerySecurity);
KPH_DEFINE_INFORMER_SETTING(FilePreSetSecurity);
KPH_DEFINE_INFORMER_SETTING(FilePostSetSecurity);
KPH_DEFINE_INFORMER_SETTING(FilePrePower);
KPH_DEFINE_INFORMER_SETTING(FilePostPower);
KPH_DEFINE_INFORMER_SETTING(FilePreSystemControl);
KPH_DEFINE_INFORMER_SETTING(FilePostSystemControl);
KPH_DEFINE_INFORMER_SETTING(FilePreDeviceChange);
KPH_DEFINE_INFORMER_SETTING(FilePostDeviceChange);
KPH_DEFINE_INFORMER_SETTING(FilePreQueryQuota);
KPH_DEFINE_INFORMER_SETTING(FilePostQueryQuota);
KPH_DEFINE_INFORMER_SETTING(FilePreSetQuota);
KPH_DEFINE_INFORMER_SETTING(FilePostSetQuota);
KPH_DEFINE_INFORMER_SETTING(FilePrePnp);
KPH_DEFINE_INFORMER_SETTING(FilePostPnp);
KPH_DEFINE_INFORMER_SETTING(FilePreAcquireForSectionSync);
KPH_DEFINE_INFORMER_SETTING(FilePostAcquireForSectionSync);
KPH_DEFINE_INFORMER_SETTING(FilePreReleaseForSectionSync);
KPH_DEFINE_INFORMER_SETTING(FilePostReleaseForSectionSync);
KPH_DEFINE_INFORMER_SETTING(FilePreAcquireForModWrite);
KPH_DEFINE_INFORMER_SETTING(FilePostAcquireForModWrite);
KPH_DEFINE_INFORMER_SETTING(FilePreReleaseForModWrite);
KPH_DEFINE_INFORMER_SETTING(FilePostReleaseForModWrite);
KPH_DEFINE_INFORMER_SETTING(FilePreAcquireForCcFlush);
KPH_DEFINE_INFORMER_SETTING(FilePostAcquireForCcFlush);
KPH_DEFINE_INFORMER_SETTING(FilePreReleaseForCcFlush);
KPH_DEFINE_INFORMER_SETTING(FilePostReleaseForCcFlush);
KPH_DEFINE_INFORMER_SETTING(FilePreQueryOpen);
KPH_DEFINE_INFORMER_SETTING(FilePostQueryOpen);
KPH_DEFINE_INFORMER_SETTING(FilePreFastIoCheckIfPossible);
KPH_DEFINE_INFORMER_SETTING(FilePostFastIoCheckIfPossible);
KPH_DEFINE_INFORMER_SETTING(FilePreNetworkQueryOpen);
KPH_DEFINE_INFORMER_SETTING(FilePostNetworkQueryOpen);
KPH_DEFINE_INFORMER_SETTING(FilePreMdlRead);
KPH_DEFINE_INFORMER_SETTING(FilePostMdlRead);
KPH_DEFINE_INFORMER_SETTING(FilePreMdlReadComplete);
KPH_DEFINE_INFORMER_SETTING(FilePostMdlReadComplete);
KPH_DEFINE_INFORMER_SETTING(FilePrePrepareMdlWrite);
KPH_DEFINE_INFORMER_SETTING(FilePostPrepareMdlWrite);
KPH_DEFINE_INFORMER_SETTING(FilePreMdlWriteComplete);
KPH_DEFINE_INFORMER_SETTING(FilePostMdlWriteComplete);
KPH_DEFINE_INFORMER_SETTING(FilePreVolumeMount);
KPH_DEFINE_INFORMER_SETTING(FilePostVolumeMount);
KPH_DEFINE_INFORMER_SETTING(FilePreVolumeDismount);
KPH_DEFINE_INFORMER_SETTING(FilePostVolumeDismount);
KPH_DEFINE_INFORMER_SETTING(RegEnablePostObjectNames);
KPH_DEFINE_INFORMER_SETTING(RegEnablePostValueNames);
KPH_DEFINE_INFORMER_SETTING(RegEnableValueBuffers);
KPH_DEFINE_INFORMER_SETTING(RegPreDeleteKey);
KPH_DEFINE_INFORMER_SETTING(RegPostDeleteKey);
KPH_DEFINE_INFORMER_SETTING(RegPreSetValueKey);
KPH_DEFINE_INFORMER_SETTING(RegPostSetValueKey);
KPH_DEFINE_INFORMER_SETTING(RegPreDeleteValueKey);
KPH_DEFINE_INFORMER_SETTING(RegPostDeleteValueKey);
KPH_DEFINE_INFORMER_SETTING(RegPreSetInformationKey);
KPH_DEFINE_INFORMER_SETTING(RegPostSetInformationKey);
KPH_DEFINE_INFORMER_SETTING(RegPreRenameKey);
KPH_DEFINE_INFORMER_SETTING(RegPostRenameKey);
KPH_DEFINE_INFORMER_SETTING(RegPreEnumerateKey);
KPH_DEFINE_INFORMER_SETTING(RegPostEnumerateKey);
KPH_DEFINE_INFORMER_SETTING(RegPreEnumerateValueKey);
KPH_DEFINE_INFORMER_SETTING(RegPostEnumerateValueKey);
KPH_DEFINE_INFORMER_SETTING(RegPreQueryKey);
KPH_DEFINE_INFORMER_SETTING(RegPostQueryKey);
KPH_DEFINE_INFORMER_SETTING(RegPreQueryValueKey);
KPH_DEFINE_INFORMER_SETTING(RegPostQueryValueKey);
KPH_DEFINE_INFORMER_SETTING(RegPreQueryMultipleValueKey);
KPH_DEFINE_INFORMER_SETTING(RegPostQueryMultipleValueKey);
KPH_DEFINE_INFORMER_SETTING(RegPreKeyHandleClose);
KPH_DEFINE_INFORMER_SETTING(RegPostKeyHandleClose);
KPH_DEFINE_INFORMER_SETTING(RegPreCreateKey);
KPH_DEFINE_INFORMER_SETTING(RegPostCreateKey);
KPH_DEFINE_INFORMER_SETTING(RegPreOpenKey);
KPH_DEFINE_INFORMER_SETTING(RegPostOpenKey);
KPH_DEFINE_INFORMER_SETTING(RegPreFlushKey);
KPH_DEFINE_INFORMER_SETTING(RegPostFlushKey);
KPH_DEFINE_INFORMER_SETTING(RegPreLoadKey);
KPH_DEFINE_INFORMER_SETTING(RegPostLoadKey);
KPH_DEFINE_INFORMER_SETTING(RegPreUnLoadKey);
KPH_DEFINE_INFORMER_SETTING(RegPostUnLoadKey);
KPH_DEFINE_INFORMER_SETTING(RegPreQueryKeySecurity);
KPH_DEFINE_INFORMER_SETTING(RegPostQueryKeySecurity);
KPH_DEFINE_INFORMER_SETTING(RegPreSetKeySecurity);
KPH_DEFINE_INFORMER_SETTING(RegPostSetKeySecurity);
KPH_DEFINE_INFORMER_SETTING(RegPreRestoreKey);
KPH_DEFINE_INFORMER_SETTING(RegPostRestoreKey);
KPH_DEFINE_INFORMER_SETTING(RegPreSaveKey);
KPH_DEFINE_INFORMER_SETTING(RegPostSaveKey);
KPH_DEFINE_INFORMER_SETTING(RegPreReplaceKey);
KPH_DEFINE_INFORMER_SETTING(RegPostReplaceKey);
KPH_DEFINE_INFORMER_SETTING(RegPreQueryKeyName);
KPH_DEFINE_INFORMER_SETTING(RegPostQueryKeyName);
KPH_DEFINE_INFORMER_SETTING(RegPreSaveMergedKey);
KPH_DEFINE_INFORMER_SETTING(RegPostSaveMergedKey);
KPH_DEFINE_INFORMER_SETTING(ImageVerify);

#ifdef KPH_INFORMER_P
KPH_PROTECTED_DATA_SECTION_RO_POP();
#endif

extern KPH_INFORMER_SETTINGS KphDefaultInformerProcessFilter;

FORCEINLINE
VOID KphSetInformerSettings(
    _Out_ PKPH_INFORMER_SETTINGS Destination,
    _In_ PCKPH_INFORMER_SETTINGS Source
    )
{
    InterlockedExchangeU64(&Destination->Flags, Source->Flags);
    InterlockedExchangeU64(&Destination->Flags2, Source->Flags2);
    InterlockedExchangeU64(&Destination->Flags3, Source->Flags3);
}

FORCEINLINE
VOID KphGetInformerSettings(
    _Out_ PKPH_INFORMER_SETTINGS Destination,
    _In_ PCKPH_INFORMER_SETTINGS Source
    )
{
    Destination->Flags = Source->Flags;
    Destination->Flags2 = Source->Flags2;
    Destination->Flags3 = Source->Flags3;
}

FORCEINLINE
BOOLEAN KphCheckInformerSettings(
    _In_ PCKPH_INFORMER_SETTINGS Target,
    _In_ PCKPH_INFORMER_SETTINGS Settings
    )
{
    if (FlagOn(Target->Flags, Settings->Flags) ||
        FlagOn(Target->Flags2, Settings->Flags2) ||
        FlagOn(Target->Flags3, Settings->Flags3))
    {
        return TRUE;
    }

    return FALSE;
}

_Must_inspect_result_
PCKPH_INFORMER_SETTINGS KphInformerForMessageId(
    _In_ KPH_MESSAGE_ID MessageId
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN KphInformerIsEnabled(
    _In_ PCKPH_INFORMER_SETTINGS Settings,
    _In_opt_ PKPH_PROCESS_CONTEXT Process
    );

#define KphInformerEnabled(name, proc)                                        \
    KphInformerIsEnabled(&KphInformer##name, proc)

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN KphInformerIsEnabled2(
    _In_ PCKPH_INFORMER_SETTINGS Settings,
    _In_opt_ PKPH_PROCESS_CONTEXT ActorProcess,
    _In_opt_ PKPH_PROCESS_CONTEXT TargetProcess
    );

#define KphInformerEnabled2(name, actor, target)                               \
    KphInformerIsEnabled2(&KphInformer##name, actor, target)

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetInformerProcessFilter(
    _In_ HANDLE ProcessHandle,
    _Out_ PKPH_INFORMER_SETTINGS Filter,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformerProcessFilter(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PKPH_INFORMER_SETTINGS Filter,
    _In_ KPROCESSOR_MODE AccessMode
    );

// mini-filter informer

extern PFLT_FILTER KphFltFilter;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphFltRegister(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphFltUnregister(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphFltInformerStart(
    VOID
    );

// process informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphProcessInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphProcessInformerStop(
    VOID
    );

// thread informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphThreadInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphThreadInformerStop(
    VOID
    );

// image informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphImageInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphImageInformerStop(
    VOID
    );

// object informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphObjectInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphObjectInformerStop(
    VOID
    );

// registry informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphRegistryInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphRegistryInformerStop(
    VOID
    );

// debug informer

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphDebugInformerStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphDebugInformerStop(
    VOID
    );
