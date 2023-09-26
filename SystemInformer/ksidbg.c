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

static KPH_INFORMER_SETTINGS KsiDebugInformerSettings =
{
    .ProcessCreate              = TRUE,
    .ProcessExit                = TRUE,
    .ThreadCreate               = TRUE,
    .ThreadExecute              = TRUE,
    .ThreadExit                 = TRUE,
    .ImageLoad                  = TRUE,
    .DebugPrint                 = FALSE,
    .ProcessHandlePreCreate     = TRUE,
    .ProcessHandlePostCreate    = TRUE,
    .ProcessHandlePreDuplicate  = TRUE,
    .ProcessHandlePostDuplicate = TRUE,
    .ThreadHandlePreCreate      = TRUE,
    .ThreadHandlePostCreate     = TRUE,
    .ThreadHandlePreDuplicate   = TRUE,
    .ThreadHandlePostDuplicate  = TRUE,
    .DesktopHandlePreCreate     = TRUE,
    .DesktopHandlePostCreate    = TRUE,
    .DesktopHandlePreDuplicate  = TRUE,
    .DesktopHandlePostDuplicate = TRUE,
    .EnableStackTraces          = FALSE,
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

static SI_DEBUG_LOG_DEF KsiDebugLogDefs[] =
{
    { PH_STRINGREF_INIT(L"ProcessCreate  "), KsiDebugLogProcessCreate },
    { PH_STRINGREF_INIT(L"ProcessExit    "), KsiDebugLogProcessExit },
    { PH_STRINGREF_INIT(L"ThreadCreate   "), KsiDebugLogThreadCreate },
    { PH_STRINGREF_INIT(L"ThreadExecute  "), KsiDebugLogThreadExecute },
    { PH_STRINGREF_INIT(L"ThreadExit     "), KsiDebugLogThreadExit },
    { PH_STRINGREF_INIT(L"ImageLoad      "), KsiDebugLogImageLoad },
    { PH_STRINGREF_INIT(L"DebugPrint     "), KsiDebugLogDebugPrint },
    { PH_STRINGREF_INIT(L"ProcHPreCreate "), KsiDebugLogProcessHandlePreCreate },
    { PH_STRINGREF_INIT(L"ProcHPostCreate"), KsiDebugLogProcessHandlePostCreate },
    { PH_STRINGREF_INIT(L"ProcHPreDupe   "), KsiDebugLogProcessHandlePreDuplicate },
    { PH_STRINGREF_INIT(L"ProcHPostDupe  "), KsiDebugLogProcessHandlePostDuplicate },
    { PH_STRINGREF_INIT(L"ThrdHPreCreate "), KsiDebugLogThreadHandlePreCreate },
    { PH_STRINGREF_INIT(L"ThrdHPostCreate"), KsiDebugLogThreadHandlePostCreate },
    { PH_STRINGREF_INIT(L"ThrdHPreDupe   "), KsiDebugLogThreadHandlePreDuplicate },
    { PH_STRINGREF_INIT(L"ThrdHPostDupe  "), KsiDebugLogThreadHandlePostDuplicate },
    { PH_STRINGREF_INIT(L"DskpHPreCreate "), KsiDebugLogDesktopHandlePreCreate },
    { PH_STRINGREF_INIT(L"DskpHPostCreate"), KsiDebugLogDesktopHandlePostCreate },
    { PH_STRINGREF_INIT(L"DskpHPreDupe   "), KsiDebugLogDesktopHandlePreDuplicate },
    { PH_STRINGREF_INIT(L"DsktpHPostDupe "), KsiDebugLogDesktopHandlePostDuplicate },
    { PH_STRINGREF_INIT(L"ReqStateFailure"), KsiDebugLogRequiredStateFailure },
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

VOID KsiDebugLogMessageLog(
    _In_ PCKPH_MESSAGE Message
    )
{
    ULONG index;
    PPH_STRING time;
    PPH_STRING log;
    PH_FORMAT format[7];
    PPH_STRING line;
    KPH_STACK_TRACE stack;

    if (!KsiDebugLogFileStream || KsiDebugLogSkip(Message))
        return;

    assert(Message->Header.MessageId > MaxKphMsgClientAllowed);
    assert(Message->Header.MessageId < MaxKphMsg);

    index = (Message->Header.MessageId - (MaxKphMsgClientAllowed + 1));

    assert(index < RTL_NUMBER_OF(KsiDebugLogDefs));

    time = KsiDebugLogGetTimeString(Message);
    log = KsiDebugLogDefs[index].GetLogString(Message);

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
