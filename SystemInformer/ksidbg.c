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
    .ProcessHandlePreCreate         = TRUE,
    .ProcessHandlePostCreate        = TRUE,
    .ProcessHandlePreDuplicate      = TRUE,
    .ProcessHandlePostDuplicate     = TRUE,
    .ThreadHandlePreCreate          = TRUE,
    .ThreadHandlePostCreate         = TRUE,
    .ThreadHandlePreDuplicate       = TRUE,
    .ThreadHandlePostDuplicate      = TRUE,
    .DesktopHandlePreCreate         = TRUE,
    .DesktopHandlePostCreate        = TRUE,
    .DesktopHandlePreDuplicate      = TRUE,
    .DesktopHandlePostDuplicate     = TRUE,
    .EnableStackTraces              = FALSE,
    .FileEnablePagingIo             = TRUE,
    .FileEnableSyncPagingIo         = TRUE,
    .FileEnableIoControlBuffers     = FALSE,
    .FileEnableFsControlBuffers     = FALSE,
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
        L"process %lu (thread %lu) created process %lu%ls(parent %lu) \"%wZ\" \"%wZ\"",
        HandleToUlong(Message->Kernel.ProcessCreate.CreatingClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ProcessCreate.CreatingClientId.UniqueThread),
        HandleToUlong(Message->Kernel.ProcessCreate.TargetProcessId),
        Message->Kernel.ProcessCreate.IsSubsystemProcess ? L" subsystem process " : L" ",
        HandleToUlong(Message->Kernel.ProcessCreate.ParentProcessId),
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

    statusMessage = PhGetStatusMessage(0, Message->Kernel.ProcessExit.ExitStatus);

    result = PhFormatString(
        L"process %lu exited with %ls (0x%08x)",
        HandleToUlong(Message->Kernel.ProcessExit.ExitingClientId.UniqueProcess),
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
        L"thread %lu created in process %lu by process %lu (thread %lu)",
        HandleToUlong(Message->Kernel.ThreadCreate.TargetClientId.UniqueThread),
        HandleToUlong(Message->Kernel.ThreadCreate.TargetClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ThreadCreate.CreatingClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ThreadCreate.CreatingClientId.UniqueThread)
        );
}

PPH_STRING KsiDebugLogThreadExecute(
    _In_ PCKPH_MESSAGE Message
    )
{
    return PhFormatString(
        L"thread %lu in process %lu is beginning execution",
        HandleToUlong(Message->Kernel.ThreadExecute.ExecutingClientId.UniqueThread),
        HandleToUlong(Message->Kernel.ThreadExecute.ExecutingClientId.UniqueProcess)
        );
}

PPH_STRING KsiDebugLogThreadExit(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;
    PPH_STRING statusMessage;

    statusMessage = PhGetStatusMessage(0, Message->Kernel.ThreadExit.ExitStatus);

    result = PhFormatString(
        L"thread %lu in process %lu exited with %ls (0x%08x)",
        HandleToUlong(Message->Kernel.ThreadExit.ExitingClientId.UniqueThread),
        HandleToUlong(Message->Kernel.ThreadExit.ExitingClientId.UniqueProcess),
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
        L"image \"%wZ\" loaded into process %lu at %p by process %lu (thread %lu)",
        &fileName,
        HandleToUlong(Message->Kernel.ImageLoad.TargetProcessId),
        Message->Kernel.ImageLoad.ImageBase,
        HandleToUlong(Message->Kernel.ImageLoad.LoadingClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ImageLoad.LoadingClientId.UniqueThread)
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

PPH_STRING KsiDebugLogProcessHandlePreCreate(
    _In_ PCKPH_MESSAGE Message
    )
{
    return PhFormatString(
        L"%ls to process %lu for process %lu (thread %lu) desires 0x%08x (original 0x%08x)",
        Message->Kernel.ProcessHandlePreCreate.KernelHandle ? L"kernel handle" : L"handle",
        HandleToUlong(Message->Kernel.ProcessHandlePreCreate.ObjectProcessId),
        HandleToUlong(Message->Kernel.ProcessHandlePreCreate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ProcessHandlePreCreate.ContextClientId.UniqueThread),
        Message->Kernel.ProcessHandlePreCreate.DesiredAccess,
        Message->Kernel.ProcessHandlePreCreate.OriginalDesiredAccess
        );
}

PPH_STRING KsiDebugLogProcessHandlePostCreate(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;
    PPH_STRING statusMessage;

    statusMessage = PhGetStatusMessage(0, Message->Kernel.ProcessHandlePostCreate.ReturnStatus);

    result = PhFormatString(
        L"%ls to process %lu for process %lu (thread %lu) granted 0x%08x with %ls (0x%08x)",
        Message->Kernel.ProcessHandlePostCreate.KernelHandle ? L"kernel handle" : L"handle",
        HandleToUlong(Message->Kernel.ProcessHandlePostCreate.ObjectProcessId),
        HandleToUlong(Message->Kernel.ProcessHandlePostCreate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ProcessHandlePostCreate.ContextClientId.UniqueThread),
        Message->Kernel.ProcessHandlePostCreate.GrantedAccess,
        PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
        Message->Kernel.ProcessHandlePostCreate.ReturnStatus
        );

    PhClearReference(&statusMessage);

    return result;
}

PPH_STRING KsiDebugLogProcessHandlePreDuplicate(
    _In_ PCKPH_MESSAGE Message
    )
{
    return PhFormatString(
        L"%ls to process %lu from %lu to %lu for process %lu (thread %lu) desires 0x%08x (original 0x%08x)",
        Message->Kernel.ProcessHandlePreDuplicate.KernelHandle ? L"kernel handle" : L"handle",
        HandleToUlong(Message->Kernel.ProcessHandlePreDuplicate.ObjectProcessId),
        HandleToUlong(Message->Kernel.ProcessHandlePreDuplicate.SourceProcessId),
        HandleToUlong(Message->Kernel.ProcessHandlePreDuplicate.TargetProcessId),
        HandleToUlong(Message->Kernel.ProcessHandlePreDuplicate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ProcessHandlePreDuplicate.ContextClientId.UniqueThread),
        Message->Kernel.ProcessHandlePreDuplicate.DesiredAccess,
        Message->Kernel.ProcessHandlePreDuplicate.OriginalDesiredAccess
        );
}

PPH_STRING KsiDebugLogProcessHandlePostDuplicate(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;
    PPH_STRING statusMessage;

    statusMessage = PhGetStatusMessage(0, Message->Kernel.ProcessHandlePostDuplicate.ReturnStatus);

    result = PhFormatString(
        L"%ls to process %lu for process %lu (thread %lu) granted 0x%08x with %ls (0x%08x)",
        Message->Kernel.ProcessHandlePostDuplicate.KernelHandle ? L"kernel handle" : L"handle",
        HandleToUlong(Message->Kernel.ProcessHandlePostDuplicate.ObjectProcessId),
        HandleToUlong(Message->Kernel.ProcessHandlePostDuplicate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ProcessHandlePostDuplicate.ContextClientId.UniqueThread),
        Message->Kernel.ProcessHandlePostDuplicate.GrantedAccess,
        PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
        Message->Kernel.ProcessHandlePostDuplicate.ReturnStatus
        );

    PhClearReference(&statusMessage);

    return result;
}

PPH_STRING KsiDebugLogThreadHandlePreCreate(
    _In_ PCKPH_MESSAGE Message
    )
{
    return PhFormatString(
        L"%ls to thread %lu for process %lu (thread %lu) desires 0x%08x (original 0x%08x)",
        Message->Kernel.ThreadHandlePreCreate.KernelHandle ? L"kernel handle" : L"handle",
        HandleToUlong(Message->Kernel.ThreadHandlePreCreate.ObjectThreadId),
        HandleToUlong(Message->Kernel.ThreadHandlePreCreate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ThreadHandlePreCreate.ContextClientId.UniqueThread),
        Message->Kernel.ThreadHandlePreCreate.DesiredAccess,
        Message->Kernel.ThreadHandlePreCreate.OriginalDesiredAccess
        );
}

PPH_STRING KsiDebugLogThreadHandlePostCreate(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;
    PPH_STRING statusMessage;

    statusMessage = PhGetStatusMessage(0, Message->Kernel.ThreadHandlePostCreate.ReturnStatus);

    result = PhFormatString(
        L"%ls to thread %lu for process %lu (thread %lu) granted 0x%08x with %ls (0x%08x)",
        Message->Kernel.ThreadHandlePostCreate.KernelHandle ? L"kernel handle" : L"handle",
        HandleToUlong(Message->Kernel.ThreadHandlePostCreate.ObjectThreadId),
        HandleToUlong(Message->Kernel.ThreadHandlePostCreate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ThreadHandlePostCreate.ContextClientId.UniqueThread),
        Message->Kernel.ThreadHandlePostCreate.GrantedAccess,
        PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
        Message->Kernel.ThreadHandlePostCreate.ReturnStatus
        );

    PhClearReference(&statusMessage);

    return result;
}

PPH_STRING KsiDebugLogThreadHandlePreDuplicate(
    _In_ PCKPH_MESSAGE Message
    )
{
    return PhFormatString(
        L"%ls to thread %lu from %lu to %lu for process %lu (thread %lu) desires 0x%08x (original 0x%08x)",
        Message->Kernel.ThreadHandlePreDuplicate.KernelHandle ? L"kernel handle" : L"handle",
        HandleToUlong(Message->Kernel.ThreadHandlePreDuplicate.ObjectThreadId),
        HandleToUlong(Message->Kernel.ThreadHandlePreDuplicate.SourceProcessId),
        HandleToUlong(Message->Kernel.ThreadHandlePreDuplicate.TargetProcessId),
        HandleToUlong(Message->Kernel.ThreadHandlePreDuplicate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ThreadHandlePreDuplicate.ContextClientId.UniqueThread),
        Message->Kernel.ThreadHandlePreDuplicate.DesiredAccess,
        Message->Kernel.ThreadHandlePreDuplicate.OriginalDesiredAccess
        );
}

PPH_STRING KsiDebugLogThreadHandlePostDuplicate(
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_STRING result;
    PPH_STRING statusMessage;

    statusMessage = PhGetStatusMessage(0, Message->Kernel.ThreadHandlePostDuplicate.ReturnStatus);

    result = PhFormatString(
        L"%ls to thread %lu for process %lu (thread %lu) granted 0x%08x with %ls (0x%08x)",
        Message->Kernel.ThreadHandlePostDuplicate.KernelHandle ? L"kernel handle" : L"handle",
        HandleToUlong(Message->Kernel.ThreadHandlePostDuplicate.ObjectThreadId),
        HandleToUlong(Message->Kernel.ThreadHandlePostDuplicate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.ThreadHandlePostDuplicate.ContextClientId.UniqueThread),
        Message->Kernel.ThreadHandlePostDuplicate.GrantedAccess,
        PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
        Message->Kernel.ThreadHandlePostDuplicate.ReturnStatus
        );

    PhClearReference(&statusMessage);

    return result;
}

PPH_STRING KsiDebugLogDesktopHandlePreCreate(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING objectName;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &objectName);

    return PhFormatString(
        L"%ls to \"%wZ\" for process %lu (thread %lu) desires 0x%08x (original 0x%08x)",
        Message->Kernel.DesktopHandlePreCreate.KernelHandle ? L"kernel handle" : L"handle",
        &objectName,
        HandleToUlong(Message->Kernel.DesktopHandlePreCreate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.DesktopHandlePreCreate.ContextClientId.UniqueThread),
        Message->Kernel.DesktopHandlePreCreate.DesiredAccess,
        Message->Kernel.DesktopHandlePreCreate.OriginalDesiredAccess
        );
}

PPH_STRING KsiDebugLogDesktopHandlePostCreate(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING objectName;
    PPH_STRING result;
    PPH_STRING statusMessage;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &objectName);

    statusMessage = PhGetStatusMessage(0, Message->Kernel.DesktopHandlePostCreate.ReturnStatus);

    result = PhFormatString(
        L"%ls to \"%wZ\" for process %lu (thread %lu) granted 0x%08x with %ls (0x%08x)",
        Message->Kernel.DesktopHandlePostCreate.KernelHandle ? L"kernel handle" : L"handle",
        &objectName,
        HandleToUlong(Message->Kernel.DesktopHandlePostCreate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.DesktopHandlePostCreate.ContextClientId.UniqueThread),
        Message->Kernel.DesktopHandlePostCreate.GrantedAccess,
        PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
        Message->Kernel.DesktopHandlePostCreate.ReturnStatus
        );

    PhClearReference(&statusMessage);

    return result;
}

PPH_STRING KsiDebugLogDesktopHandlePreDuplicate(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING objectName;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &objectName);

    return PhFormatString(
        L"%ls to \"%wZ\" from %lu to %lu for process %lu (thread %lu) desires 0x%08x (original 0x%08x)",
        Message->Kernel.DesktopHandlePreDuplicate.KernelHandle ? L"kernel handle" : L"handle",
        &objectName,
        HandleToUlong(Message->Kernel.DesktopHandlePreDuplicate.SourceProcessId),
        HandleToUlong(Message->Kernel.DesktopHandlePreDuplicate.TargetProcessId),
        HandleToUlong(Message->Kernel.DesktopHandlePreDuplicate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.DesktopHandlePreDuplicate.ContextClientId.UniqueThread),
        Message->Kernel.DesktopHandlePreDuplicate.DesiredAccess,
        Message->Kernel.DesktopHandlePreDuplicate.OriginalDesiredAccess
        );
}

PPH_STRING KsiDebugLogDesktopHandlePostDuplicate(
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING objectName;
    PPH_STRING result;
    PPH_STRING statusMessage;

    KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &objectName);

    statusMessage = PhGetStatusMessage(0, Message->Kernel.DesktopHandlePostDuplicate.ReturnStatus);

    result = PhFormatString(
        L"%ls to \"%wZ\" for process %lu (thread %lu) granted 0x%08x with %ls (0x%08x)",
        Message->Kernel.DesktopHandlePostDuplicate.KernelHandle ? L"kernel handle" : L"handle",
        &objectName,
        HandleToUlong(Message->Kernel.DesktopHandlePostDuplicate.ContextClientId.UniqueProcess),
        HandleToUlong(Message->Kernel.DesktopHandlePostDuplicate.ContextClientId.UniqueThread),
        Message->Kernel.DesktopHandlePostDuplicate.GrantedAccess,
        PhGetStringOrDefault(statusMessage, L"UNKNOWN"),
        Message->Kernel.DesktopHandlePostDuplicate.ReturnStatus
        );

    PhClearReference(&statusMessage);

    return result;
}

PPH_STRING KsiDebugLogRequiredStateFailure(
    _In_ PCKPH_MESSAGE Message
    )
{
    return PhFormatString(
        L"required state failure for process %lu (thread %lu), message %lu, state 0x%08x, required 0x%08x",
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

        statusMessage = PhGetStatusMessage(0, Message->Kernel.File.Post.IoStatus.Status);

        result = PhFormatString(
            L"%04x:%04x %lu %p %llu \"%wZ\" %llu %llu %ls (0x%08x)",
            HandleToUlong(Message->Kernel.File.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.File.ClientId.UniqueThread),
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
            L"%04x:%04x %lu %p %llu \"%wZ\"",
            HandleToUlong(Message->Kernel.File.ClientId.UniqueProcess),
            HandleToUlong(Message->Kernel.File.ClientId.UniqueThread),
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

static SI_DEBUG_LOG_DEF KsiDebugLogDefs[] =
{
    { PH_STRINGREF_INIT(L"ProcessCreate       "), KsiDebugLogProcessCreate },
    { PH_STRINGREF_INIT(L"ProcessExit         "), KsiDebugLogProcessExit },
    { PH_STRINGREF_INIT(L"ThreadCreate        "), KsiDebugLogThreadCreate },
    { PH_STRINGREF_INIT(L"ThreadExecute       "), KsiDebugLogThreadExecute },
    { PH_STRINGREF_INIT(L"ThreadExit          "), KsiDebugLogThreadExit },
    { PH_STRINGREF_INIT(L"ImageLoad           "), KsiDebugLogImageLoad },
    { PH_STRINGREF_INIT(L"DebugPrint          "), KsiDebugLogDebugPrint },
    { PH_STRINGREF_INIT(L"ProcHandlePreCreate "), KsiDebugLogProcessHandlePreCreate },
    { PH_STRINGREF_INIT(L"ProcHandlePostCreate"), KsiDebugLogProcessHandlePostCreate },
    { PH_STRINGREF_INIT(L"ProcHandlePreDupe   "), KsiDebugLogProcessHandlePreDuplicate },
    { PH_STRINGREF_INIT(L"ProcHandlePostDupe  "), KsiDebugLogProcessHandlePostDuplicate },
    { PH_STRINGREF_INIT(L"ThrdHandlePreCreate "), KsiDebugLogThreadHandlePreCreate },
    { PH_STRINGREF_INIT(L"ThrdHandlePostCreate"), KsiDebugLogThreadHandlePostCreate },
    { PH_STRINGREF_INIT(L"ThrdHandlePreDupe   "), KsiDebugLogThreadHandlePreDuplicate },
    { PH_STRINGREF_INIT(L"ThrdHandlePostDupe  "), KsiDebugLogThreadHandlePostDuplicate },
    { PH_STRINGREF_INIT(L"DskpHandlePreCreate "), KsiDebugLogDesktopHandlePreCreate },
    { PH_STRINGREF_INIT(L"DskpHandlePostCreate"), KsiDebugLogDesktopHandlePostCreate },
    { PH_STRINGREF_INIT(L"DskpHandlePreDupe   "), KsiDebugLogDesktopHandlePreDuplicate },
    { PH_STRINGREF_INIT(L"DskpHandlePostDupe  "), KsiDebugLogDesktopHandlePostDuplicate },
    { PH_STRINGREF_INIT(L"ReqiredStateFailure "), KsiDebugLogRequiredStateFailure },
    { PH_STRINGREF_INIT(L"PreCreate           "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostCreate          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreCreateNamedPipe  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostCreateNamedPipe "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreClose            "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostClose           "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreRead             "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostRead            "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreWrite            "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostWrite           "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQueryInfo        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQueryInfo       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSetInfo          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSetInfo         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQueryEa          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQueryEa         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSetEa            "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSetEa           "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreFlushBuffs       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostFlushBuffs      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQueryVolumeInfo  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQueryVolumeInfo "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSetVolumeInfo    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSetVolumeInfo   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreDirControl       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostDirControl      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreFileSystemCtrl   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostFileSystemCtrl  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreDeviceCtrl       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostDeviceCtrl      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreItrnlDeviceCtrl  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PosItrnlDeviceCtrl  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreShutdown         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostShutdown        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreLockCtrl         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostLockCtrl        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreCleanup          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostCleanup         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreCreateMailslot   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostCreateMailslot  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQuerySecurity    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQuerySecurity   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSetSecurity      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSetSecurity     "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PrePower            "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostPower           "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSystemCtrl       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSystemCtrl      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreDeviceChange     "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostDeviceChange    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQueryQuota       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQueryQuota      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreSetQuota         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostSetQuota        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PrePnp              "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostPnp             "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreAcqForSecSync    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostAcqForSecSync   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreRelForSecSync    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostRelForSecSync   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreAcqForModWrite   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostAcqForModWrite  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreRelForModWrite   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostRelForModWrite  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreAcqForCcFlush    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostAcqForCcFlush   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreRelForCcFlush    "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostRelForCcFlush   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreQueryOpen        "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostQueryOpen       "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreFastIoCheck      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostFastIoCheck     "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreNetworkQueryOpen "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostNetworkQueryOpen"), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreMdlRead          "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostMdlRead         "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreMdlReadComplete  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostMdlReadComplete "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PrePrepareMdlWrite  "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostPrepareMdlWrite "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreMdlWriteComplete "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostMdlWriteComplete"), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreVolumeMount      "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostVolumeMount     "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PreVolumeDismount   "), KsiDebugLogFileCommon },
    { PH_STRINGREF_INIT(L"PostVolumeDismount  "), KsiDebugLogFileCommon },
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
        }
        else
        {
            filter.Flags = MAXULONG64;
            filter.Flags2 = MAXULONG64;
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
    // Ignore Message->Header.Size here and write the entire message block for consistent alignment.
    PhWriteFileStream(KsiDebugRawFileStream, (PVOID)Message, sizeof(KPH_MESSAGE));
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
