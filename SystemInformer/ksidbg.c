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

#include <phapp.h>
#include <kphuser.h>
#include <kphcomms.h>
#include <kphmsgdyn.h>

#include <dpfilter.h>

#ifdef DEBUG

typedef
PPH_STRING
KSI_DEBUG_LOG_GET_LOG_STRING(
    _In_ PCKPH_MESSAGE Message
    );
typedef KSI_DEBUG_LOG_GET_LOG_STRING* PKSI_DEBUG_LOG_GET_LOG_STRING;

typedef struct _KSI_DEBUG_LOG_DEF
{
    PH_STRINGREF Name;
    PKSI_DEBUG_LOG_GET_LOG_STRING GetLogString;
} SI_DEBUG_LOG_DEF, * PSI_DEBUG_LOG_DEF;

static BOOLEAN KsiDebugLogEnabled = FALSE;
static PH_FAST_LOCK KsiDebugLogFileStreamLock = PH_FAST_LOCK_INIT;
static PPH_FILE_STREAM KsiDebugLogFileStream = NULL;
static PH_STRINGREF KsiDebugLogSuffix = PH_STRINGREF_INIT(L"\\Desktop\\ksidbg.log");

static BOOLEAN KsiDebugRawEnabled = FALSE;
static PH_FAST_LOCK KsiDebugRawFileStreamLock = PH_FAST_LOCK_INIT;
static PPH_FILE_STREAM KsiDebugRawFileStream = NULL;
static PH_STRINGREF KsiDebugRawSuffix = PH_STRINGREF_INIT(L"\\Desktop\\ksidbg.bin");
static BOOLEAN KsiDebugRawAligned = FALSE;

static PH_STRINGREF KsiDebugProcFilter = PH_STRINGREF_INIT(L"");

static KPH_INFORMER_SETTINGS KsiDebugInformerSettings =
{
    .ProcessCreate                  = TRUE,
    .ProcessExit                    = TRUE,
    .ThreadCreate                   = TRUE,
    .ThreadExecute                  = TRUE,
    .ThreadExit                     = TRUE,
    .ImageLoad                      = TRUE,
    .DebugPrint                     = FALSE,
    .HandlePreCreateProcess         = TRUE,
    .HandlePostCreateProcess        = TRUE,
    .HandlePreDuplicateProcess      = TRUE,
    .HandlePostDuplicateProcess     = TRUE,
    .HandlePreCreateThread          = TRUE,
    .HandlePostCreateThread         = TRUE,
    .HandlePreDuplicateThread       = TRUE,
    .HandlePostDuplicateThread      = TRUE,
    .HandlePreCreateDesktop         = TRUE,
    .HandlePostCreateDesktop        = TRUE,
    .HandlePreDuplicateDesktop      = TRUE,
    .HandlePostDuplicateDesktop     = TRUE,
    .EnableStackTraces              = FALSE,
    .EnableProcessCreateReply       = FALSE,
    .FileEnablePreCreateReply       = FALSE,
    .FileEnablePostCreateReply      = FALSE,
    .FileEnablePostFileNames        = FALSE,
    .FileEnablePagingIo             = TRUE,
    .FileEnableSyncPagingIo         = TRUE,
    .FileEnableIoControlBuffers     = FALSE,
    .FileEnableFsControlBuffers     = FALSE,
    .FileEnableDirControlBuffers    = FALSE,
    .FilePreCreate                  = TRUE,
    .FilePostCreate                 = TRUE,
    .FilePreCreateNamedPipe         = TRUE,
    .FilePostCreateNamedPipe        = TRUE,
    .FilePreClose                   = TRUE,
    .FilePostClose                  = TRUE,
    .FilePreRead                    = TRUE,
    .FilePostRead                   = TRUE,
    .FilePreWrite                   = TRUE,
    .FilePostWrite                  = TRUE,
    .FilePreQueryInformation        = TRUE,
    .FilePostQueryInformation       = TRUE,
    .FilePreSetInformation          = TRUE,
    .FilePostSetInformation         = TRUE,
    .FilePreQueryEa                 = TRUE,
    .FilePostQueryEa                = TRUE,
    .FilePreSetEa                   = TRUE,
    .FilePostSetEa                  = TRUE,
    .FilePreFlushBuffers            = TRUE,
    .FilePostFlushBuffers           = TRUE,
    .FilePreQueryVolumeInformation  = TRUE,
    .FilePostQueryVolumeInformation = TRUE,
    .FilePreSetVolumeInformation    = TRUE,
    .FilePostSetVolumeInformation   = TRUE,
    .FilePreDirectoryControl        = TRUE,
    .FilePostDirectoryControl       = TRUE,
    .FilePreFileSystemControl       = TRUE,
    .FilePostFileSystemControl      = TRUE,
    .FilePreDeviceControl           = TRUE,
    .FilePostDeviceControl          = TRUE,
    .FilePreInternalDeviceControl   = TRUE,
    .FilePostInternalDeviceControl  = TRUE,
    .FilePreShutdown                = TRUE,
    .FilePostShutdown               = TRUE,
    .FilePreLockControl             = TRUE,
    .FilePostLockControl            = TRUE,
    .FilePreCleanup                 = TRUE,
    .FilePostCleanup                = TRUE,
    .FilePreCreateMailslot          = TRUE,
    .FilePostCreateMailslot         = TRUE,
    .FilePreQuerySecurity           = TRUE,
    .FilePostQuerySecurity          = TRUE,
    .FilePreSetSecurity             = TRUE,
    .FilePostSetSecurity            = TRUE,
    .FilePrePower                   = TRUE,
    .FilePostPower                  = TRUE,
    .FilePreSystemControl           = TRUE,
    .FilePostSystemControl          = TRUE,
    .FilePreDeviceChange            = TRUE,
    .FilePostDeviceChange           = TRUE,
    .FilePreQueryQuota              = TRUE,
    .FilePostQueryQuota             = TRUE,
    .FilePreSetQuota                = TRUE,
    .FilePostSetQuota               = TRUE,
    .FilePrePnp                     = TRUE,
    .FilePostPnp                    = TRUE,
    .FilePreAcquireForSectionSync   = TRUE,
    .FilePostAcquireForSectionSync  = TRUE,
    .FilePreReleaseForSectionSync   = TRUE,
    .FilePostReleaseForSectionSync  = TRUE,
    .FilePreAcquireForModWrite      = TRUE,
    .FilePostAcquireForModWrite     = TRUE,
    .FilePreReleaseForModWrite      = TRUE,
    .FilePostReleaseForModWrite     = TRUE,
    .FilePreAcquireForCcFlush       = TRUE,
    .FilePostAcquireForCcFlush      = TRUE,
    .FilePreReleaseForCcFlush       = TRUE,
    .FilePostReleaseForCcFlush      = TRUE,
    .FilePreQueryOpen               = TRUE,
    .FilePostQueryOpen              = TRUE,
    .FilePreFastIoCheckIfPossible   = TRUE,
    .FilePostFastIoCheckIfPossible  = TRUE,
    .FilePreNetworkQueryOpen        = TRUE,
    .FilePostNetworkQueryOpen       = TRUE,
    .FilePreMdlRead                 = TRUE,
    .FilePostMdlRead                = TRUE,
    .FilePreMdlReadComplete         = TRUE,
    .FilePostMdlReadComplete        = TRUE,
    .FilePrePrepareMdlWrite         = TRUE,
    .FilePostPrepareMdlWrite        = TRUE,
    .FilePreMdlWriteComplete        = TRUE,
    .FilePostMdlWriteComplete       = TRUE,
    .FilePreVolumeMount             = TRUE,
    .FilePostVolumeMount            = TRUE,
    .FilePreVolumeDismount          = TRUE,
    .FilePostVolumeDismount         = TRUE,
    .RegEnablePostObjectNames       = FALSE,
    .RegEnablePostValueNames        = FALSE,
    .RegEnableValueBuffers          = FALSE,
    .RegPreDeleteKey                = TRUE,
    .RegPostDeleteKey               = TRUE,
    .RegPreSetValueKey              = TRUE,
    .RegPostSetValueKey             = TRUE,
    .RegPreDeleteValueKey           = TRUE,
    .RegPostDeleteValueKey          = TRUE,
    .RegPreSetInformationKey        = TRUE,
    .RegPostSetInformationKey       = TRUE,
    .RegPreRenameKey                = TRUE,
    .RegPostRenameKey               = TRUE,
    .RegPreEnumerateKey             = TRUE,
    .RegPostEnumerateKey            = TRUE,
    .RegPreEnumerateValueKey        = TRUE,
    .RegPostEnumerateValueKey       = TRUE,
    .RegPreQueryKey                 = TRUE,
    .RegPostQueryKey                = TRUE,
    .RegPreQueryValueKey            = TRUE,
    .RegPostQueryValueKey           = TRUE,
    .RegPreQueryMultipleValueKey    = TRUE,
    .RegPostQueryMultipleValueKey   = TRUE,
    .RegPreKeyHandleClose           = TRUE,
    .RegPostKeyHandleClose          = TRUE,
    .RegPreCreateKey                = TRUE,
    .RegPostCreateKey               = TRUE,
    .RegPreOpenKey                  = TRUE,
    .RegPostOpenKey                 = TRUE,
    .RegPreFlushKey                 = TRUE,
    .RegPostFlushKey                = TRUE,
    .RegPreLoadKey                  = TRUE,
    .RegPostLoadKey                 = TRUE,
    .RegPreUnLoadKey                = TRUE,
    .RegPostUnLoadKey               = TRUE,
    .RegPreQueryKeySecurity         = TRUE,
    .RegPostQueryKeySecurity        = TRUE,
    .RegPreSetKeySecurity           = TRUE,
    .RegPostSetKeySecurity          = TRUE,
    .RegPreRestoreKey               = TRUE,
    .RegPostRestoreKey              = TRUE,
    .RegPreSaveKey                  = TRUE,
    .RegPostSaveKey                 = TRUE,
    .RegPreReplaceKey               = TRUE,
    .RegPostReplaceKey              = TRUE,
    .RegPreQueryKeyName             = TRUE,
    .RegPostQueryKeyName            = TRUE,
    .RegPreSaveMergedKey            = TRUE,
    .RegPostSaveMergedKey           = TRUE,
    .ImageVerify                    = TRUE,
};

PPH_STRING KsiDebugLogProcessCreate(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING fileName;
    UNICODE_STRING commandLine;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldFileName, &fileName);
    KphMsgDynGetUnicodeString(Message, KphMsgFieldCommandLine, &commandLine);

    return PhFormatString(
        L"%04x:%04x:%016llx created process %lu (%016llx)%ls(parent %lu (%016llx)) \"%wZ\" \"%wZ\"",
        HandleToUlong(Message->Kernel.ProcessCreate.CreatingClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ProcessCreate.CreatingClientId.UniqueThread),
        Message->Kernel.ProcessCreate.CreatingProcessStartKey,
        HandleToUlong(Message->Kernel.ProcessCreate.TargetProcessId),
        Message->Kernel.ProcessCreate.TargetProcessStartKey,
        Message->Kernel.ProcessCreate.IsSubsystemProcess ? L" subsystem process " : L" ",
        HandleToUlong(Message->Kernel.ProcessCreate.ParentProcessId),
        Message->Kernel.ProcessCreate.ParentProcessStartKey,
        &fileName,
        &commandLine
        );
}

PPH_STRING KsiDebugLogProcessExit(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;
    PPH_STRING statusMessage;

    statusMessage = PhGetStatusMessage(Message->Kernel.ProcessExit.ExitStatus, 0);

    result = PhFormatString(
        L"%04x:%04x:%016llx process exited with %ls (0x%08x)",
        HandleToUlong(Message->Kernel.ProcessExit.ClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ProcessExit.ClientId.UniqueThread),
        Message->Kernel.ProcessExit.ProcessStartKey,
        PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
        Message->Kernel.ProcessExit.ExitStatus
        );

    PhMoveReference(&statusMessage, NULL);

    return result;
}

PPH_STRING KsiDebugLogThreadCreate(
    _In_ PCKPH_MESSAGE Message
    )
{
    return PhFormatString(
        L"%04x:%04x:%016llx thread %lu created in process %lu (%016llx)",
        HandleToUlong(Message->Kernel.ThreadCreate.CreatingClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ThreadCreate.CreatingClientId.UniqueThread),
        Message->Kernel.ThreadCreate.CreatingProcessStartKey,
        HandleToUlong(Message->Kernel.ThreadCreate.TargetClientId.UniqueThread),
        HandleToUlong(Message->Kernel.ThreadCreate.TargetClientId.UniqueProcess),
        Message->Kernel.ThreadCreate.TargetProcessStartKey
        );
}

PPH_STRING KsiDebugLogThreadExecute(
    _In_ PCKPH_MESSAGE Message
    )
{
    return PhFormatString(
        L"%04x:%04x:%016llx thread beginning execution",
        HandleToUlong(Message->Kernel.ThreadExecute.ClientId.UniqueThread),
        HandleToUlong(Message->Kernel.ThreadExecute.ClientId.UniqueProcess),
        Message->Kernel.ThreadExecute.ProcessStartKey
        );
}

PPH_STRING KsiDebugLogThreadExit(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;
    PPH_STRING statusMessage;

    statusMessage = PhGetStatusMessage(Message->Kernel.ThreadExit.ExitStatus, 0);

    result = PhFormatString(
        L"%04x:%04x:%016llx thread exited with %ls (0x%08x)",
        HandleToUlong(Message->Kernel.ThreadExit.ClientId.UniqueThread),
        HandleToUlong(Message->Kernel.ThreadExit.ClientId.UniqueProcess),
        Message->Kernel.ThreadExit.ProcessStartKey,
        PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
        Message->Kernel.ThreadExit.ExitStatus
        );

    PhMoveReference(&statusMessage, NULL);

    return result;
}

PPH_STRING KsiDebugLogImageLoad(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING fileName;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldFileName, &fileName);

    return PhFormatString(
        L"%04x:%04x:%016llx image \"%wZ\" loaded into process %lu (%016llx) at %p",
        HandleToUlong(Message->Kernel.ImageLoad.LoadingClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ImageLoad.LoadingClientId.UniqueThread),
        Message->Kernel.ImageLoad.LoadingProcessStartKey,
        &fileName,
        HandleToUlong(Message->Kernel.ImageLoad.TargetProcessId),
        Message->Kernel.ImageLoad.TargetProcessStartKey,
        Message->Kernel.ImageLoad.ImageBase
        );
}

PPH_STRING KsiDebugLogDebugPrint(
    _In_ PCKPH_MESSAGE Message
    )
{
    ANSI_STRING output;

    KphMsgDynGetAnsiString(Message, KphMsgFieldOutput, &output);

    while (output.Length > 0)
    {
        CHAR c = output.Buffer[output.Length - 1];
        if (c != '\r' && c != '\n')
            break;
        output.Length--;
    }

    return PhFormatString(
        L"%04x:%04x 0x%08x 0x%08x\r\n%hZ",
        HandleToUlong(Message->Kernel.DebugPrint.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.DebugPrint.ContextClientId.UniqueThread),
        Message->Kernel.DebugPrint.ComponentId,
        Message->Kernel.DebugPrint.Level,
        &output
        );
}

PPH_STRING KsiDebugLogHandleProcess(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;

    if (Message->Kernel.Handle.PostOperation)
    {
        PPH_STRING statusMessage;

        statusMessage = PhGetStatusMessage(Message->Kernel.Handle.Post.ReturnStatus, 0);

        if (Message->Kernel.Handle.Duplicate)
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p %llu granted 0x%08x (desired 0x%08x, original 0x%08x) to process %lu (duplicate %lu -> %lu) %llu %llu %ls (0x%08x)",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Sequence,
                Message->Kernel.Handle.Post.GrantedAccess,
                Message->Kernel.Handle.Post.DesiredAccess,
                Message->Kernel.Handle.Post.OriginalDesiredAccess,
                HandleToUlong(Message->Kernel.Handle.Post.Duplicate.Process.ProcessId),
                HandleToUlong(Message->Kernel.Handle.Post.Duplicate.SourceProcessId),
                HandleToUlong(Message->Kernel.Handle.Post.Duplicate.TargetProcessId),
                Message->Kernel.Handle.Post.PreSequence,
                (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Handle.Post.PreTimeStamp.QuadPart),
                PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
                Message->Kernel.Handle.Post.ReturnStatus
                );
        }
        else
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p %llu granted 0x%08x (desired 0x%08x, original 0x%08x) to process %lu %llu %llu %ls (0x%08x)",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Sequence,
                Message->Kernel.Handle.Post.GrantedAccess,
                Message->Kernel.Handle.Post.DesiredAccess,
                Message->Kernel.Handle.Post.OriginalDesiredAccess,
                HandleToUlong(Message->Kernel.Handle.Post.Create.Process.ProcessId),
                Message->Kernel.Handle.Post.PreSequence,
                (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Handle.Post.PreTimeStamp.QuadPart),
                PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
                Message->Kernel.Handle.Post.ReturnStatus
                );
        }

        PhClearReference(&statusMessage);
    }
    else
    {
        if (Message->Kernel.Handle.Duplicate)
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p desires 0x%08x (original 0x%08x) to process %lu (duplicate %lu -> %lu)",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Pre.DesiredAccess,
                Message->Kernel.Handle.Pre.OriginalDesiredAccess,
                HandleToUlong(Message->Kernel.Handle.Pre.Duplicate.Process.ProcessId),
                HandleToUlong(Message->Kernel.Handle.Pre.Duplicate.SourceProcessId),
                HandleToUlong(Message->Kernel.Handle.Pre.Duplicate.TargetProcessId)
                );
        }
        else
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p desires 0x%08x (original 0x%08x) to process %lu",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Pre.DesiredAccess,
                Message->Kernel.Handle.Pre.OriginalDesiredAccess,
                HandleToUlong(Message->Kernel.Handle.Pre.Create.Process.ProcessId)
                );
        }
    }

    return result;
}

PPH_STRING KsiDebugLogHandleThread(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;

    if (Message->Kernel.Handle.PostOperation)
    {
        PPH_STRING statusMessage;

        statusMessage = PhGetStatusMessage(Message->Kernel.Handle.Post.ReturnStatus, 0);

        if (Message->Kernel.Handle.Duplicate)
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p %llu granted 0x%08x (desired 0x%08x, original 0x%08x) to thread %lu (process %lu, duplicate %lu -> %lu) %llu %llu %ls (0x%08x)",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Sequence,
                Message->Kernel.Handle.Post.GrantedAccess,
                Message->Kernel.Handle.Post.DesiredAccess,
                Message->Kernel.Handle.Post.OriginalDesiredAccess,
                HandleToUlong(Message->Kernel.Handle.Post.Duplicate.Thread.ClientId.UniqueThread),
                HandleToUlong(Message->Kernel.Handle.Post.Duplicate.Thread.ClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.Post.Duplicate.SourceProcessId),
                HandleToUlong(Message->Kernel.Handle.Post.Duplicate.TargetProcessId),
                Message->Kernel.Handle.Post.PreSequence,
                (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Handle.Post.PreTimeStamp.QuadPart),
                PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
                Message->Kernel.Handle.Post.ReturnStatus
                );
        }
        else
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p %llu granted 0x%08x (desired 0x%08x, original 0x%08x) to thread %lu (process %lu) %llu %llu %ls (0x%08x)",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Sequence,
                Message->Kernel.Handle.Post.GrantedAccess,
                Message->Kernel.Handle.Post.DesiredAccess,
                Message->Kernel.Handle.Post.OriginalDesiredAccess,
                HandleToUlong(Message->Kernel.Handle.Post.Create.Thread.ClientId.UniqueThread),
                HandleToUlong(Message->Kernel.Handle.Post.Create.Thread.ClientId.UniqueProcess),
                Message->Kernel.Handle.Post.PreSequence,
                (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Handle.Post.PreTimeStamp.QuadPart),
                PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
                Message->Kernel.Handle.Post.ReturnStatus
                );
        }

        PhClearReference(&statusMessage);
    }
    else
    {
        if (Message->Kernel.Handle.Duplicate)
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p %llu desires 0x%08x (original 0x%08x) to thread %lu (process %lu, duplicate %lu -> %lu)",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Sequence,
                Message->Kernel.Handle.Pre.DesiredAccess,
                Message->Kernel.Handle.Pre.OriginalDesiredAccess,
                HandleToUlong(Message->Kernel.Handle.Pre.Duplicate.Thread.ClientId.UniqueThread),
                HandleToUlong(Message->Kernel.Handle.Pre.Duplicate.Thread.ClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.Pre.Duplicate.SourceProcessId),
                HandleToUlong(Message->Kernel.Handle.Pre.Duplicate.TargetProcessId)
                );
        }
        else
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p %llu desires 0x%08x (original 0x%08x) to thread %lu (process %lu)",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Sequence,
                Message->Kernel.Handle.Pre.DesiredAccess,
                Message->Kernel.Handle.Pre.OriginalDesiredAccess,
                HandleToUlong(Message->Kernel.Handle.Pre.Create.Thread.ClientId.UniqueThread),
                HandleToUlong(Message->Kernel.Handle.Pre.Create.Thread.ClientId.UniqueProcess)
                );
        }
    }

    return result;
}

PPH_STRING KsiDebugLogHandleDesktop(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;
    UNICODE_STRING objectName;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &objectName);

    if (Message->Kernel.Handle.PostOperation)
    {
        PPH_STRING statusMessage;

        statusMessage = PhGetStatusMessage(Message->Kernel.Handle.Post.ReturnStatus, 0);

        if (Message->Kernel.Handle.Duplicate)
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p %llu granted 0x%08x (desired 0x%08x, original 0x%08x) to desktop \"%wZ\" (duplicate %lu -> %lu) %llu %llu %ls (0x%08x)",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Sequence,
                Message->Kernel.Handle.Post.GrantedAccess,
                Message->Kernel.Handle.Post.DesiredAccess,
                Message->Kernel.Handle.Post.OriginalDesiredAccess,
                &objectName,
                HandleToUlong(Message->Kernel.Handle.Post.Duplicate.SourceProcessId),
                HandleToUlong(Message->Kernel.Handle.Post.Duplicate.TargetProcessId),
                Message->Kernel.Handle.Post.PreSequence,
                (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Handle.Post.PreTimeStamp.QuadPart),
                PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
                Message->Kernel.Handle.Post.ReturnStatus
                );
        }
        else
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p %llu granted 0x%08x (desired 0x%08x, original 0x%08x) to desktop \"%wZ\" %llu %llu %ls (0x%08x)",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Sequence,
                Message->Kernel.Handle.Post.GrantedAccess,
                Message->Kernel.Handle.Post.DesiredAccess,
                Message->Kernel.Handle.Post.OriginalDesiredAccess,
                &objectName,
                Message->Kernel.Handle.Post.PreSequence,
                (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Handle.Post.PreTimeStamp.QuadPart),
                PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
                Message->Kernel.Handle.Post.ReturnStatus
                );
        }

        PhClearReference(&statusMessage);
    }
    else
    {
        if (Message->Kernel.Handle.Duplicate)
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p %llu desires 0x%08x (original 0x%08x) to desktop \"%wZ\" (duplicate %lu -> %lu)",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Sequence,
                Message->Kernel.Handle.Pre.DesiredAccess,
                Message->Kernel.Handle.Pre.OriginalDesiredAccess,
                &objectName,
                HandleToUlong(Message->Kernel.Handle.Pre.Duplicate.SourceProcessId),
                HandleToUlong(Message->Kernel.Handle.Pre.Duplicate.TargetProcessId)
                );
        }
        else
        {
            result = PhFormatString(
                L"%04x:%04x:%016llx %c %p %llu desires 0x%08x (original 0x%08x) to desktop \"%wZ\"",
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueProcess),
                HandleToUlong(Message->Kernel.Handle.ContextClientId.UniqueThread),
                Message->Kernel.Handle.ContextProcessStartKey,
                (Message->Kernel.Handle.KernelHandle ? 'K' : 'U'),
                Message->Kernel.Handle.Object,
                Message->Kernel.Handle.Sequence,
                Message->Kernel.Handle.Pre.DesiredAccess,
                Message->Kernel.Handle.Pre.OriginalDesiredAccess,
                &objectName
                );
        }
    }

    return result;
}

PPH_STRING KsiDebugLogRequiredStateFailure(
    _In_ PCKPH_MESSAGE Message
    )
{
    return PhFormatString(
        L"%04x:%04x required state failure, message %lu, state 0x%08x, required 0x%08x",
        HandleToUlong(Message->Kernel.RequiredStateFailure.ClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.RequiredStateFailure.ClientId.UniqueThread),
        (ULONG)Message->Kernel.RequiredStateFailure.MessageId,
        Message->Kernel.RequiredStateFailure.ClientState,
        Message->Kernel.RequiredStateFailure.RequiredState
        );
}

PPH_STRING KsiDebugLogFileCommon(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING fileName;
    PPH_STRING result;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldFileName, &fileName);

    if (FlagOn(Message->Kernel.File.FltFlags, FLTFL_CALLBACK_DATA_POST_OPERATION))
    {
        PPH_STRING statusMessage;

        statusMessage = PhGetStatusMessage(Message->Kernel.File.Post.IoStatus.Status, 0);

        result = PhFormatString(
            L"%04x:%04x:%016llx %lu %p %llu \"%wZ\" %llu %llu %ls (0x%08x)",
            HandleToUlong(Message->Kernel.File.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.File.ClientId.UniqueThread),
            Message->Kernel.File.ProcessStartKey,
            (ULONG)Message->Kernel.File.Irql,
            Message->Kernel.File.FileObject,
            Message->Kernel.File.Sequence,
            &fileName,
            (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.File.Post.PreTimeStamp.QuadPart),
            Message->Kernel.File.Post.PreSequence,
            PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
            Message->Kernel.File.Post.IoStatus.Status
            );

        PhClearReference(&statusMessage);
    }
    else
    {
        result = PhFormatString(
            L"%04x:%04x:%016llx %lu %p %llu \"%wZ\"",
            HandleToUlong(Message->Kernel.File.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.File.ClientId.UniqueThread),
            Message->Kernel.File.ProcessStartKey,
            (ULONG)Message->Kernel.File.Irql,
            Message->Kernel.File.FileObject,
            Message->Kernel.File.Sequence,
            &fileName
            );
    }

    if (Message->Kernel.File.IsPagingFile)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", paging file"));

    if (Message->Kernel.File.IsSystemPagingFile)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", system paging file"));

    if (Message->Kernel.File.OriginRemote)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", remote origin"));

    if (Message->Kernel.File.IgnoringSharing)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", ignoring sharing"));

    if (Message->Kernel.File.InStackFileObject)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", in stack file object"));

    if (Message->Kernel.File.DeletePending)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", delete pending"));

    if (!Message->Kernel.File.SharedRead && !Message->Kernel.File.SharedWrite && !Message->Kernel.File.SharedDelete)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", opened exclusively"));

    if (Message->Kernel.File.Busy)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", busy"));

    if (Message->Kernel.File.Waiters)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", waiters"));

    if (Message->Kernel.File.OplockKeyContext.Version)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", oplock key"));

    if (Message->Kernel.File.Transaction)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", transaction"));

    if (Message->Kernel.File.DataSectionObject)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", data section"));

    if (Message->Kernel.File.ImageSectionObject)
        PhMoveReference(&result, PhConcatStringRefZ(&result->sr, L", image section"));

    return result;
}

PPH_STRING KsiDebugLogRegCommon(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING objectName;
    PPH_STRING result;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &objectName);

    if (Message->Kernel.Reg.PostOperation)
    {
        PPH_STRING statusMessage;

        statusMessage = PhGetStatusMessage(Message->Kernel.Reg.Post.Status, 0);

        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\" %llu %llu %ls (0x%08x)",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &objectName,
            (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Reg.Post.PreTimeStamp.QuadPart),
            Message->Kernel.Reg.Post.PreSequence,
            PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
            Message->Kernel.Reg.Post.Status
            );

        PhClearReference(&statusMessage);
    }
    else
    {
        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\"",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &objectName
            );
    }

    return result;
}

PPH_STRING KsiDebugLogRegCommonWithValue(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING objectName;
    UNICODE_STRING valueName;
    PPH_STRING result;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &objectName);
    KphMsgDynGetUnicodeString(Message, KphMsgFieldValueName, &valueName);

    if (Message->Kernel.Reg.PostOperation)
    {
        PPH_STRING statusMessage;

        statusMessage = PhGetStatusMessage(Message->Kernel.Reg.Post.Status, 0);

        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\" \"%wZ\" %llu %llu %ls (0x%08x)",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &objectName,
            &valueName,
            (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Reg.Post.PreTimeStamp.QuadPart),
            Message->Kernel.Reg.Post.PreSequence,
            PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
            Message->Kernel.Reg.Post.Status
            );

        PhClearReference(&statusMessage);
    }
    else
    {
        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\" \"%wZ\"",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &objectName,
            &valueName
            );
    }

    return result;
}

PPH_STRING KsiDebugLogRegCommonWithMultipleValues(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING objectName;
    UNICODE_STRING valueNames;
    PPH_STRING values;
    PPH_STRING result;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &objectName);
    KphMsgDynGetUnicodeString(Message, KphMsgFieldMultipleValueNames, &valueNames);

    values = NULL;
    if (valueNames.Length)
    {
        PH_STRING_BUILDER stringBuilder;

        PhInitializeStringBuilder(&stringBuilder, 100);

        for (;;)
        {
            PH_STRINGREF sr;

            PhInitializeStringRef(&sr, valueNames.Buffer);

            if (!sr.Length)
            {
                break;
            }

            PhAppendStringBuilder2(&stringBuilder, L"\"");
            PhAppendStringBuilder(&stringBuilder, &sr);
            PhAppendStringBuilder2(&stringBuilder, L"\", ");

            valueNames.Buffer = PTR_ADD_OFFSET(valueNames.Buffer, sr.Length + sizeof(WCHAR));
            valueNames.Length -= (USHORT)(sr.Length + sizeof(WCHAR));
            valueNames.MaximumLength -= (USHORT)(sr.Length + sizeof(WCHAR));
        }

        if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&stringBuilder, 2);

        values = PhFinalStringBuilderString(&stringBuilder);
    }

    if (Message->Kernel.Reg.PostOperation)
    {
        PPH_STRING statusMessage;

        statusMessage = PhGetStatusMessage(Message->Kernel.Reg.Post.Status, 0);

        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\" %ls %llu %llu %ls (0x%08x)",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &objectName,
            PhGetStringOrDefault(values, L"NULL"),
            (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Reg.Post.PreTimeStamp.QuadPart),
            Message->Kernel.Reg.Post.PreSequence,
            PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
            Message->Kernel.Reg.Post.Status
            );

        PhClearReference(&statusMessage);
    }
    else
    {
        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\" %ls",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &objectName,
            PhGetStringOrDefault(values, L"NULL")
            );
    }

    return result;
}

PPH_STRING KsiDebugLogRegSaveRestoreKey(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING objectName;
    UNICODE_STRING fileName;
    PPH_STRING result;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &objectName);
    KphMsgDynGetUnicodeString(Message, KphMsgFieldFileName, &fileName);

    if (Message->Kernel.Reg.PostOperation)
    {
        PPH_STRING statusMessage;

        statusMessage = PhGetStatusMessage(Message->Kernel.Reg.Post.Status, 0);

        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\" \"%wZ\" %llu %llu %ls (0x%08x)",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &objectName,
            &fileName,
            (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Reg.Post.PreTimeStamp.QuadPart),
            Message->Kernel.Reg.Post.PreSequence,
            PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
            Message->Kernel.Reg.Post.Status
            );

        PhClearReference(&statusMessage);
    }
    else
    {
        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\" \"%wZ\"",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &objectName,
            &fileName
            );
    }

    return result;
}

PPH_STRING KsiDebugLogRegReplaceKey(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING objectName;
    UNICODE_STRING fileName;
    UNICODE_STRING destFileName;
    PPH_STRING result;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &objectName);
    KphMsgDynGetUnicodeString(Message, KphMsgFieldFileName, &fileName);
    KphMsgDynGetUnicodeString(Message, KphMsgFieldDestinationFileName, &destFileName);

    if (Message->Kernel.Reg.PostOperation)
    {
        PPH_STRING statusMessage;

        statusMessage = PhGetStatusMessage(Message->Kernel.Reg.Post.Status, 0);

        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\" -> \"%wZ\" -> \"%wZ\" %llu %llu %ls (0x%08x)",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &fileName,
            &objectName,
            &destFileName,
            (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Reg.Post.PreTimeStamp.QuadPart),
            Message->Kernel.Reg.Post.PreSequence,
            PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
            Message->Kernel.Reg.Post.Status
            );

        PhClearReference(&statusMessage);
    }
    else
    {
        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\" -> \"%wZ\" -> \"%wZ\"",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &fileName,
            &objectName,
            &destFileName
            );
    }

    return result;
}

PPH_STRING KsiDebugLogSaveMergedKey(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING objectName;
    UNICODE_STRING otherObjectName;
    UNICODE_STRING fileName;
    PPH_STRING result;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &objectName);
    KphMsgDynGetUnicodeString(Message, KphMsgFieldOtherObjectName, &otherObjectName);
    KphMsgDynGetUnicodeString(Message, KphMsgFieldFileName, &fileName);

    if (Message->Kernel.Reg.PostOperation)
    {
        PPH_STRING statusMessage;

        statusMessage = PhGetStatusMessage(Message->Kernel.Reg.Post.Status, 0);

        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\" + \"%wZ\" -> \"%wZ\" %llu %llu %ls (0x%08x)",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &objectName,
            &otherObjectName,
            &fileName,
            (ULONG64)(Message->Header.TimeStamp.QuadPart - Message->Kernel.Reg.Post.PreTimeStamp.QuadPart),
            Message->Kernel.Reg.Post.PreSequence,
            PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
            Message->Kernel.Reg.Post.Status
            );

        PhClearReference(&statusMessage);
    }
    else
    {
        result = PhFormatString(
            L"%04x:%04x:%016llx %c %p %llu \"%wZ\" + \"%wZ\" -> \"%wZ\"",
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.Reg.ClientId.UniqueThread),
            Message->Kernel.Reg.ProcessStartKey,
            (Message->Kernel.Reg.PreviousMode ? 'U' : 'K'),
            Message->Kernel.Reg.Object,
            Message->Kernel.Reg.Sequence,
            &objectName,
            &otherObjectName,
            &fileName
            );
    }

    return result;
}

PPH_STRING KsiDebugLogImageVerify(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;
    UNICODE_STRING fileName;
    UNICODE_STRING registryPath;
    UNICODE_STRING certificatePublisher;
    UNICODE_STRING certificateIssuer;
    KPHM_SIZED_BUFFER imageHash;
    KPHM_SIZED_BUFFER thumbprint;
    PPH_STRING imageHashString;
    PPH_STRING thumbprintString;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldFileName, &fileName);
    KphMsgDynGetUnicodeString(Message, KphMsgFieldRegistryPath, &registryPath);
    KphMsgDynGetUnicodeString(Message, KphMsgFieldCertificatePublisher, &certificatePublisher);
    KphMsgDynGetUnicodeString(Message, KphMsgFieldCertificateIssuer, &certificateIssuer);
    KphMsgDynGetSizedBuffer(Message, KphMsgFieldHash, &imageHash);
    KphMsgDynGetSizedBuffer(Message, KphMsgFieldCertificateThumbprint, &thumbprint);

    imageHashString = PhBufferToHexString(imageHash.Buffer, imageHash.Size);
    thumbprintString = PhBufferToHexString(thumbprint.Buffer, thumbprint.Size);

    result = PhFormatString(
        L"%04x:%04x:%016llx %lu %lu 0x%08x %lu %lu \"%wZ\" %ls \"%wZ\" \"%wZ\" \"%wZ\" %ls",
        HandleToUlong(Message->Kernel.ImageVerify.ClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ImageVerify.ClientId.UniqueThread),
        Message->Kernel.ImageVerify.ProcessStartKey,
        Message->Kernel.ImageVerify.ImageType,
        Message->Kernel.ImageVerify.Classification,
        Message->Kernel.ImageVerify.ImageFlags,
        Message->Kernel.ImageVerify.ImageHashAlgorithm,
        Message->Kernel.ImageVerify.ThumbprintHashAlgorithm,
        &fileName,
        PhGetString(imageHashString),
        &registryPath,
        &certificatePublisher,
        &certificateIssuer,
        PhGetString(thumbprintString)
        );

    PhDereferenceObject(imageHashString);
    PhDereferenceObject(thumbprintString);

    return result;
}

static SI_DEBUG_LOG_DEF KsiDebugLogDefs[] =
{
    { PH_STRINGREF_INIT(L"ProcessCreate        "), KsiDebugLogProcessCreate },
    { PH_STRINGREF_INIT(L"ProcessExit          "), KsiDebugLogProcessExit },
    { PH_STRINGREF_INIT(L"ThreadCreate         "), KsiDebugLogThreadCreate },
    { PH_STRINGREF_INIT(L"ThreadExecute        "), KsiDebugLogThreadExecute },
    { PH_STRINGREF_INIT(L"ThreadExit           "), KsiDebugLogThreadExit },
    { PH_STRINGREF_INIT(L"ImageLoad            "), KsiDebugLogImageLoad },
    { PH_STRINGREF_INIT(L"DebugPrint           "), KsiDebugLogDebugPrint },
    { PH_STRINGREF_INIT(L"HandlePreCreateProc  "), KsiDebugLogHandleProcess },
    { PH_STRINGREF_INIT(L"HandlePostCreateProc "), KsiDebugLogHandleProcess },
    { PH_STRINGREF_INIT(L"HandlePreDupeProc    "), KsiDebugLogHandleProcess },
    { PH_STRINGREF_INIT(L"HandlePostDupeProc   "), KsiDebugLogHandleProcess },
    { PH_STRINGREF_INIT(L"HandlePreCreateThrd  "), KsiDebugLogHandleThread },
    { PH_STRINGREF_INIT(L"HandlePostCreateThrd "), KsiDebugLogHandleThread },
    { PH_STRINGREF_INIT(L"HandlePreDupeThrd    "), KsiDebugLogHandleThread },
    { PH_STRINGREF_INIT(L"HandlePostDupeThrd   "), KsiDebugLogHandleThread },
    { PH_STRINGREF_INIT(L"HandlePreCreateDskp  "), KsiDebugLogHandleDesktop },
    { PH_STRINGREF_INIT(L"HandlePostCreateDskp "), KsiDebugLogHandleDesktop },
    { PH_STRINGREF_INIT(L"HandlePreDupeDskp    "), KsiDebugLogHandleDesktop },
    { PH_STRINGREF_INIT(L"HandlePostDupeDskp   "), KsiDebugLogHandleDesktop },
    { PH_STRINGREF_INIT(L"ReqiredStateFailure  "), KsiDebugLogRequiredStateFailure },
    { PH_STRINGREF_INIT(L"PreCreate            "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostCreate           "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreCreateNamedPipe   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostCreateNamedPipe  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreClose             "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostClose            "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreRead              "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostRead             "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreWrite             "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostWrite            "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQueryInfo         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQueryInfo        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSetInfo           "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSetInfo          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQueryEa           "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQueryEa          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSetEa             "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSetEa            "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreFlushBuffs        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostFlushBuffs       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQueryVolumeInfo   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQueryVolumeInfo  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSetVolumeInfo     "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSetVolumeInfo    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreDirControl        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostDirControl       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreFileSystemCtrl    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostFileSystemCtrl   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreDeviceCtrl        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostDeviceCtrl       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreItrnlDeviceCtrl   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PosItrnlDeviceCtrl   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreShutdown          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostShutdown         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreLockCtrl          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostLockCtrl         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreCleanup           "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostCleanup          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreCreateMailslot    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostCreateMailslot   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQuerySecurity     "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQuerySecurity    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSetSecurity       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSetSecurity      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PrePower             "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostPower            "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSystemCtrl        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSystemCtrl       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreDeviceChange      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostDeviceChange     "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQueryQuota        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQueryQuota       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSetQuota          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSetQuota         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PrePnp               "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostPnp              "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreAcqForSecSync     "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostAcqForSecSync    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreRelForSecSync     "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostRelForSecSync    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreAcqForModWrite    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostAcqForModWrite   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreRelForModWrite    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostRelForModWrite   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreAcqForCcFlush     "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostAcqForCcFlush    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreRelForCcFlush     "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostRelForCcFlush    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQueryOpen         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQueryOpen        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreFastIoCheck       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostFastIoCheck      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreNetworkQueryOpen  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostNetworkQueryOpen "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreMdlRead           "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostMdlRead          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreMdlReadComplete   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostMdlReadComplete  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PrePrepareMdlWrite   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostPrepareMdlWrite  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreMdlWriteComplete  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostMdlWriteComplete "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreVolumeMount       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostVolumeMount      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreVolumeDismount    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostVolumeDismount   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"RegPreDeleteKey      "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostDeleteKey     "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreSetValueKey    "), KsiDebugLogRegCommonWithValue },
    { PH_STRINGREF_INIT(L"RegPostSetValueKey   "), KsiDebugLogRegCommonWithValue },
    { PH_STRINGREF_INIT(L"RegPreDeleteValueKey "), KsiDebugLogRegCommonWithValue },
    { PH_STRINGREF_INIT(L"RegPostDeleteValueKey"), KsiDebugLogRegCommonWithValue },
    { PH_STRINGREF_INIT(L"RegPreSetInfoKey     "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostSetInfoKey    "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreRenameKey      "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostRenameKey     "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreEnumKey        "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostEnumKey       "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreEnumValueKey   "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostEnumValueKey  "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreQueryKey       "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostQueryKey      "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreQueryValueKey  "), KsiDebugLogRegCommonWithValue },
    { PH_STRINGREF_INIT(L"RegPostQueryValueKey "), KsiDebugLogRegCommonWithValue },
    { PH_STRINGREF_INIT(L"RegPreQueryMValueKey "), KsiDebugLogRegCommonWithMultipleValues },
    { PH_STRINGREF_INIT(L"RegPostQueryMValueKey"), KsiDebugLogRegCommonWithMultipleValues },
    { PH_STRINGREF_INIT(L"RegPreKeyHandleClose "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostKeyHandleClose"), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreCreateKey      "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostCreateKey     "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreOpenKey        "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostOpenKey       "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreFlushKey       "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostFlushKey      "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreLoadKey        "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostLoadKey       "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreUnLoadKey      "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostUnLoadKey     "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreQueryKeySec    "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostQueryKeySec   "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreSetKeySec      "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostSetKeySec     "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreRestoreKey     "), KsiDebugLogRegSaveRestoreKey },
    { PH_STRINGREF_INIT(L"RegPostRestoreKey    "), KsiDebugLogRegSaveRestoreKey },
    { PH_STRINGREF_INIT(L"RegPreSaveKey        "), KsiDebugLogRegSaveRestoreKey },
    { PH_STRINGREF_INIT(L"RegPostSaveKey       "), KsiDebugLogRegSaveRestoreKey },
    { PH_STRINGREF_INIT(L"RegPreReplaceKey     "), KsiDebugLogRegReplaceKey },
    { PH_STRINGREF_INIT(L"RegPostReplaceKey    "), KsiDebugLogRegReplaceKey },
    { PH_STRINGREF_INIT(L"RegPreQueryKeyName   "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPostQueryKeyName  "), KsiDebugLogRegCommon },
    { PH_STRINGREF_INIT(L"RegPreSaveMergedKey  "), KsiDebugLogSaveMergedKey },
    { PH_STRINGREF_INIT(L"RegPostSaveMergedKey "), KsiDebugLogSaveMergedKey },
    { PH_STRINGREF_INIT(L"ImageVerify          "), KsiDebugLogImageVerify },
};
C_ASSERT((RTL_NUMBER_OF(KsiDebugLogDefs) + (MaxKphMsgClientAllowed + 1)) == MaxKphMsg);

PPH_STRING KsiDebugLogGetTimeString(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;
    SYSTEMTIME systemTime;
    PPH_STRING date;
    PPH_STRING time;

    PhLargeIntegerToSystemTime(&systemTime, (PLARGE_INTEGER)&Message->Header.TimeStamp);

    date = PhFormatDate(&systemTime, L"yyyy-MM-dd ");
    time = PhFormatTime(&systemTime, L"hh':'mm':'ss tt");

    result = PhConcatStringRef2(&date->sr, &time->sr);

    PhDereferenceObject(time);
    PhDereferenceObject(date);

    return result;
}

BOOLEAN KsiDebugLogSkip(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING fileName;

    if (NT_SUCCESS(KphMsgDynGetUnicodeString(Message, KphMsgFieldFileName, &fileName)))
    {
        PH_STRINGREF sr;

        PhUnicodeStringToStringRef(&fileName, &sr);

        if (PhEndsWithStringRef(&sr, &KsiDebugLogSuffix, TRUE))
            return TRUE;
        if (PhEndsWithStringRef(&sr, &KsiDebugRawSuffix, TRUE))
            return TRUE;
    }

    return FALSE;
}

VOID KsiDebugFilterToProcInit(
    VOID
    )
{
    KPH_INFORMER_SETTINGS filter;

    if (!KsiDebugProcFilter.Length)
        return;

    filter.Flags = MAXULONG64;
    filter.Flags2 = MAXULONG64;
    filter.Flags3 = MAXULONG64;
    filter.ProcessCreate = FALSE;

    KphSetInformerProcessFilter(NULL, &filter);
}

VOID KsiDebugFilterToProc(
    _In_ PCKPH_MESSAGE Message
    )
{
    BOOLEAN isTarget;
    UNICODE_STRING fileName;
    HANDLE processHandle;

    if (!KsiDebugProcFilter.Length || Message->Header.MessageId != KphMsgProcessCreate)
    {
        return;
    }

    isTarget = FALSE;
    if (NT_SUCCESS(KphMsgDynGetUnicodeString(Message, KphMsgFieldFileName, &fileName)))
    {
        PH_STRINGREF sr;

        PhUnicodeStringToStringRef(&fileName, &sr);

        if (PhEndsWithStringRef(&sr, &KsiDebugProcFilter, TRUE))
            isTarget = TRUE;
    }

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION,
        Message->Kernel.ProcessCreate.TargetProcessId
        )))
    {
        KPH_INFORMER_SETTINGS filter;

        // unnecessary, but here for testing
        KphGetInformerProcessFilter(processHandle, &filter);

        if (isTarget)
        {
            filter.Flags = 0;
            filter.Flags2 = 0;
            filter.Flags3 = 0;
        }
        else
        {
            filter.Flags = MAXULONG64;
            filter.Flags2 = MAXULONG64;
            filter.Flags3 = MAXULONG64;
            filter.ProcessCreate = FALSE;
        }

        KphSetInformerProcessFilter(processHandle, &filter);

        NtClose(processHandle);
    }
}

VOID KsiDebugLogMessageLog(
    _In_ PCKPH_MESSAGE Message
    )
{
    ULONG index;
    PPH_STRING time;
    PPH_STRING log;
    PH_FORMAT format[7];
    PPH_STRING line;
    KPHM_STACK_TRACE stack;

    KsiDebugFilterToProc(Message);

    if (!KsiDebugLogFileStream || KsiDebugLogSkip(Message))
        return;

    assert(Message->Header.MessageId > MaxKphMsgClientAllowed);
    assert(Message->Header.MessageId < MaxKphMsg);

    index = (Message->Header.MessageId - (MaxKphMsgClientAllowed + 1));

    assert(index < RTL_NUMBER_OF(KsiDebugLogDefs));

    time = KsiDebugLogGetTimeString(Message);

    if (KsiDebugLogDefs[index].GetLogString)
        log = KsiDebugLogDefs[index].GetLogString(Message);
    else
        log = PhReferenceEmptyString();

    PhInitFormatC(&format[0], L'[');
    PhInitFormatSR(&format[1], time->sr);
    PhInitFormatS(&format[2], L" - ");
    PhInitFormatSR(&format[3], KsiDebugLogDefs[index].Name);
    PhInitFormatS(&format[4], L"] ");
    PhInitFormatSR(&format[5], log->sr);
    PhInitFormatS(&format[6], L"\r\n");

    line = PhFormat(format, RTL_NUMBER_OF(format), 32);

    if (NT_SUCCESS(KphMsgDynGetStackTrace(Message, KphMsgFieldStackTrace, &stack)))
    {
        PH_STRING_BUILDER stringBuilder;
        WCHAR buffer[PH_PTR_STR_LEN_1];

        PhInitializeStringBuilder(&stringBuilder, line->Length + 100);

        PhAppendStringBuilder(&stringBuilder, &line->sr);

        for (ULONG i = 0; i < stack.Count; i++)
        {
            // TODO(jxy-s) resolve symbols
            PhPrintPointerPadZeros(buffer, stack.Frames[i]);
            PhAppendStringBuilder2(&stringBuilder, buffer);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
        }

        PhMoveReference(&line, PhFinalStringBuilderString(&stringBuilder));
    }

    PhAcquireFastLockExclusive(&KsiDebugLogFileStreamLock);
    PhWriteStringAsUtf8FileStream(KsiDebugLogFileStream, &line->sr);
    PhReleaseFastLockExclusive(&KsiDebugLogFileStreamLock);

    PhDereferenceObject(line);
    PhDereferenceObject(log);
    PhDereferenceObject(time);
}

VOID KsiDebugLogMessageRaw(
    _In_ PCKPH_MESSAGE Message
    )
{
    if (!KsiDebugRawFileStream || KsiDebugLogSkip(Message))
        return;

    PhAcquireFastLockExclusive(&KsiDebugRawFileStreamLock);
    if (KsiDebugRawAligned)
        PhWriteFileStream(KsiDebugRawFileStream, (PVOID)Message, sizeof(KPH_MESSAGE));
    else
        PhWriteFileStream(KsiDebugRawFileStream, (PVOID)Message, Message->Header.Size);
    PhReleaseFastLockExclusive(&KsiDebugRawFileStreamLock);
}

VOID KsiDebugLogMessage(
    _In_ PCKPH_MESSAGE Message
    )
{
    KsiDebugLogMessageRaw(Message);
    KsiDebugLogMessageLog(Message);
}

VOID KsiDebugLogInitialize(
    VOID
    )
{
    PPH_STRING desktopPath;
    PPH_STRING fileName;

    desktopPath =  PhExpandEnvironmentStringsZ(L"\\??\\%USERPROFILE%");

    if (KsiDebugLogEnabled)
    {
        fileName = PhConcatStringRef2(&desktopPath->sr, &KsiDebugLogSuffix);
        if (fileName)
        {
            if (!NT_SUCCESS(PhCreateFileStream(
                &KsiDebugLogFileStream,
                PhGetString(fileName),
                FILE_GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OVERWRITE_IF,
                0
                )))
            {
                KsiDebugLogFileStream = NULL;
            }

            PhDereferenceObject(fileName);
        }
    }

    if (KsiDebugRawEnabled)
    {
        fileName = PhConcatStringRef2(&desktopPath->sr, &KsiDebugRawSuffix);
        if (fileName)
        {
            if (!NT_SUCCESS(PhCreateFileStream(
                &KsiDebugRawFileStream,
                PhGetString(fileName),
                FILE_GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OVERWRITE_IF,
                0
                )))
            {
                KsiDebugRawFileStream = NULL;
            }

            PhDereferenceObject(fileName);
        }
    }

    if (KsiDebugLogFileStream || KsiDebugRawFileStream)
    {
        KsiDebugFilterToProcInit();
        KphSetInformerSettings(&KsiDebugInformerSettings);

        if (KsiDebugInformerSettings.DebugPrint)
        {
            NtSetDebugFilterState(ULONG_MAX, ULONG_MAX, TRUE);
            for (DPFLTR_TYPE i = 0; i <= DPFLTR_ENDOFTABLE_ID; i++)
                NtSetDebugFilterState(i, ULONG_MAX, TRUE);
        }
    }
}

VOID KsiDebugLogDestroy(
    VOID
    )
{
    PhClearReference(&KsiDebugRawFileStream);
    PhClearReference(&KsiDebugLogFileStream);
}

#endif
