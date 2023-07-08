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
#include <comms.h>

KPHM_DEFINE_HANDLER(KphpCommsGetInformerSettings);
KPHM_DEFINE_HANDLER(KphpCommsSetInformerSettings);
KPHM_DEFINE_HANDLER(KphpCommsOpenProcess);
KPHM_DEFINE_HANDLER(KphpCommsOpenProcessToken);
KPHM_DEFINE_HANDLER(KphpCommsOpenProcessJob);
KPHM_DEFINE_HANDLER(KphpCommsTerminateProcess);
KPHM_DEFINE_HANDLER(KphpCommsReadVirtualMemoryUnsafe);
KPHM_DEFINE_HANDLER(KphpCommsOpenThread);
KPHM_DEFINE_HANDLER(KphpCommsOpenThreadProcess);
KPHM_DEFINE_HANDLER(KphpCommsCaptureStackBackTraceThread);
KPHM_DEFINE_HANDLER(KphpCommsEnumerateProcessHandles);
KPHM_DEFINE_HANDLER(KphpCommsQueryInformationObject);
KPHM_DEFINE_HANDLER(KphpCommsSetInformationObject);
KPHM_DEFINE_HANDLER(KphpCommsOpenDriver);
KPHM_DEFINE_HANDLER(KphpCommsQueryInformationDriver);
KPHM_DEFINE_HANDLER(KphpCommsQueryInformationProcess);
KPHM_DEFINE_HANDLER(KphpCommsSetInformationProcess);
KPHM_DEFINE_HANDLER(KphpCommsSetInformationThread);
KPHM_DEFINE_HANDLER(KphpCommsSystemControl);
KPHM_DEFINE_HANDLER(KphpCommsAlpcQueryInformation);
KPHM_DEFINE_HANDLER(KphpCommsQueryInformationFile);
KPHM_DEFINE_HANDLER(KphpCommsQueryVolumeInformationFile);
KPHM_DEFINE_HANDLER(KphpCommsDuplicateObject);
KPHM_DEFINE_HANDLER(KphpCommsQueryPerformanceCounter);
KPHM_DEFINE_HANDLER(KphpCommsCreateFile);
KPHM_DEFINE_HANDLER(KphpCommsQueryInformationThread);
KPHM_DEFINE_HANDLER(KphpCommsQuerySection);
KPHM_DEFINE_HANDLER(KphpCommsCompareObjects);

KPHM_DEFINE_REQUIRED_STATE(KphpCommsRequireMaximum);
KPHM_DEFINE_REQUIRED_STATE(KphpCommsRequireMedium);
KPHM_DEFINE_REQUIRED_STATE(KphpCommsRequireLow);
KPHM_DEFINE_REQUIRED_STATE(KphpCommsOpenProcessRequires);
KPHM_DEFINE_REQUIRED_STATE(KphpCommsOpenProcessTokenRequires);
KPHM_DEFINE_REQUIRED_STATE(KphpCommsOpenProcessJobRequires);
KPHM_DEFINE_REQUIRED_STATE(KphpCommsOpenThreadRequires);
KPHM_DEFINE_REQUIRED_STATE(KphpCommsOpenThreadProcessRequires);
KPHM_DEFINE_REQUIRED_STATE(KphpCommsQueryInformationProcessRequires);
KPHM_DEFINE_REQUIRED_STATE(KphpCommsCreateFileRequires);

KPH_PROTECTED_DATA_SECTION_PUSH();

KPH_MESSAGE_HANDLER KphCommsMessageHandlers[] =
{
{ InvalidKphMsg,                     NULL,                                 NULL },
{ KphMsgGetInformerSettings,         KphpCommsGetInformerSettings,         KphpCommsRequireLow },
{ KphMsgSetInformerSettings,         KphpCommsSetInformerSettings,         KphpCommsRequireLow },
{ KphMsgOpenProcess,                 KphpCommsOpenProcess,                 KphpCommsOpenProcessRequires },
{ KphMsgOpenProcessToken,            KphpCommsOpenProcessToken,            KphpCommsOpenProcessTokenRequires },
{ KphMsgOpenProcessJob,              KphpCommsOpenProcessJob,              KphpCommsOpenProcessJobRequires },
{ KphMsgTerminateProcess,            KphpCommsTerminateProcess,            KphpCommsRequireMaximum },
{ KphMsgReadVirtualMemoryUnsafe,     KphpCommsReadVirtualMemoryUnsafe,     KphpCommsRequireMaximum },
{ KphMsgOpenThread,                  KphpCommsOpenThread,                  KphpCommsOpenThreadRequires },
{ KphMsgOpenThreadProcess,           KphpCommsOpenThreadProcess,           KphpCommsOpenThreadProcessRequires },
{ KphMsgCaptureStackBackTraceThread, KphpCommsCaptureStackBackTraceThread, KphpCommsRequireMedium },
{ KphMsgEnumerateProcessHandles,     KphpCommsEnumerateProcessHandles,     KphpCommsRequireMedium },
{ KphMsgQueryInformationObject,      KphpCommsQueryInformationObject,      KphpCommsRequireMedium },
{ KphMsgSetInformationObject,        KphpCommsSetInformationObject,        KphpCommsRequireMaximum },
{ KphMsgOpenDriver,                  KphpCommsOpenDriver,                  KphpCommsRequireMaximum },
{ KphMsgQueryInformationDriver,      KphpCommsQueryInformationDriver,      KphpCommsRequireMaximum },
{ KphMsgQueryInformationProcess,     KphpCommsQueryInformationProcess,     KphpCommsQueryInformationProcessRequires },
{ KphMsgSetInformationProcess,       KphpCommsSetInformationProcess,       KphpCommsRequireMaximum },
{ KphMsgSetInformationThread,        KphpCommsSetInformationThread,        KphpCommsRequireMaximum },
{ KphMsgSystemControl,               KphpCommsSystemControl,               KphpCommsRequireMaximum },
{ KphMsgAlpcQueryInformation,        KphpCommsAlpcQueryInformation,        KphpCommsRequireMedium },
{ KphMsgQueryInformationFile,        KphpCommsQueryInformationFile,        KphpCommsRequireMedium },
{ KphMsgQueryVolumeInformationFile,  KphpCommsQueryVolumeInformationFile,  KphpCommsRequireMedium },
{ KphMsgDuplicateObject,             KphpCommsDuplicateObject,             KphpCommsRequireMaximum },
{ KphMsgQueryPerformanceCounter,     KphpCommsQueryPerformanceCounter,     KphpCommsRequireLow },
{ KphMsgCreateFile,                  KphpCommsCreateFile,                  KphpCommsCreateFileRequires },
{ KphMsgQueryInformationThread,      KphpCommsQueryInformationThread,      KphpCommsRequireMedium },
{ KphMsgQuerySection,                KphpCommsQuerySection,                KphpCommsRequireMedium },
{ KphMsgCompareObjects,              KphpCommsCompareObjects,              KphpCommsRequireMedium },
};

ULONG KphCommsMessageHandlerCount = ARRAYSIZE(KphCommsMessageHandlers);

KPH_PROTECTED_DATA_SECTION_POP();

PAGED_FILE();

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsRequireMaximum(
    _In_ PCKPH_MESSAGE Message
    )
{
    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);

    UNREFERENCED_PARAMETER(Message);

    return KPH_PROCESS_STATE_MAXIMUM;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsRequireMedium(
    _In_ PCKPH_MESSAGE Message
    )
{
    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);

    UNREFERENCED_PARAMETER(Message);

    return KPH_PROCESS_STATE_MEDIUM;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsRequireLow(
    _In_ PCKPH_MESSAGE Message
    )
{
    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);

    UNREFERENCED_PARAMETER(Message);

    return KPH_PROCESS_STATE_LOW;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsGetInformerSettings(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_GET_INFORMER_SETTINGS msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgGetInformerSettings);

    msg = &Message->User.GetInformerSettings;

    msg->Settings.Flags = KphInformerSettings.Flags;
    msg->Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsSetInformerSettings(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SET_INFORMER_SETTINGS msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSetInformerSettings);

    msg = &Message->User.SetInformerSettings;

    InterlockedExchange64((volatile LONG64*)&KphInformerSettings.Flags,
                          msg->Settings.Flags);
    msg->Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsOpenProcessRequires(
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcess);

    desiredAccess = Message->User.OpenProcess.DesiredAccess;

    if ((desiredAccess & KPH_PROCESS_READ_ACCESS) != desiredAccess)
    {
        return KPH_PROCESS_STATE_MAXIMUM;
    }

    return KPH_PROCESS_STATE_MEDIUM;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsOpenProcess(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_PROCESS msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcess);

    msg = &Message->User.OpenProcess;

    msg->Status = KphOpenProcess(msg->ProcessHandle,
                                 msg->DesiredAccess,
                                 msg->ClientId,
                                 UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsOpenProcessTokenRequires(
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcessToken);

    desiredAccess = Message->User.OpenProcessToken.DesiredAccess;

    if ((desiredAccess & KPH_TOKEN_READ_ACCESS) != desiredAccess)
    {
        return KPH_PROCESS_STATE_MAXIMUM;
    }

    return KPH_PROCESS_STATE_MEDIUM;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsOpenProcessToken(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_PROCESS_TOKEN msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcessToken);

    msg = &Message->User.OpenProcessToken;

    msg->Status = KphOpenProcessToken(msg->ProcessHandle,
                                      msg->DesiredAccess,
                                      msg->TokenHandle,
                                      UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsOpenProcessJobRequires(
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcessJob);

    desiredAccess = Message->User.OpenProcessJob.DesiredAccess;

    if ((desiredAccess & KPH_JOB_READ_ACCESS) != desiredAccess)
    {
        return KPH_PROCESS_STATE_MAXIMUM;
    }

    return KPH_PROCESS_STATE_MEDIUM;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsOpenProcessJob(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_PROCESS_JOB msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcessJob);

    msg = &Message->User.OpenProcessJob;

    msg->Status = KphOpenProcessJob(msg->ProcessHandle,
                                    msg->DesiredAccess,
                                    msg->JobHandle,
                                    UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsTerminateProcess(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_TERMINATE_PROCESS msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgTerminateProcess);

    msg = &Message->User.TerminateProcess;

    msg->Status = KphTerminateProcess(msg->ProcessHandle,
                                      msg->ExitStatus,
                                      UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsReadVirtualMemoryUnsafe(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_READ_VIRTUAL_MEMORY_UNSAFE msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgReadVirtualMemoryUnsafe);

    msg = &Message->User.ReadVirtualMemoryUnsafe;

    msg->Status = KphReadVirtualMemoryUnsafe(msg->ProcessHandle,
                                             msg->BaseAddress,
                                             msg->Buffer,
                                             msg->BufferSize,
                                             msg->NumberOfBytesRead,
                                             UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsOpenThreadRequires(
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenThread);

    desiredAccess = Message->User.OpenThread.DesiredAccess;

    if ((desiredAccess & KPH_THREAD_READ_ACCESS) != desiredAccess)
    {
        return KPH_PROCESS_STATE_MAXIMUM;
    }

    return KPH_PROCESS_STATE_MEDIUM;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsOpenThread(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_THREAD msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenThread);

    msg = &Message->User.OpenThread;

    msg->Status = KphOpenThread(msg->ThreadHandle,
                                msg->DesiredAccess,
                                msg->ClientId,
                                UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsOpenThreadProcessRequires(
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenThreadProcess);

    desiredAccess = Message->User.OpenThreadProcess.DesiredAccess;

    if ((desiredAccess & KPH_PROCESS_READ_ACCESS) != desiredAccess)
    {
        return KPH_PROCESS_STATE_MAXIMUM;
    }

    return KPH_PROCESS_STATE_MEDIUM;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsOpenThreadProcess(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_THREAD_PROCESS msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenThreadProcess);

    msg = &Message->User.OpenThreadProcess;

    msg->Status = KphOpenThreadProcess(msg->ThreadHandle,
                                       msg->DesiredAccess,
                                       msg->ProcessHandle,
                                       UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsCaptureStackBackTraceThread(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_CAPTURE_STACK_BACKTRACE_THREAD msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgCaptureStackBackTraceThread);

    msg = &Message->User.CaptureStackBackTraceThread;

    msg->Status = KphCaptureStackBackTraceThreadByHandle(msg->ThreadHandle,
                                                         msg->FramesToSkip,
                                                         msg->FramesToCapture,
                                                         msg->BackTrace,
                                                         msg->CapturedFrames,
                                                         msg->BackTraceHash,
                                                         UserMode,
                                                         KPH_STACK_TRACE_CAPTURE_USER_STACK,
                                                         msg->Timeout);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsEnumerateProcessHandles(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_ENUMERATE_PROCESS_HANDLES msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgEnumerateProcessHandles);

    msg = &Message->User.EnumerateProcessHandles;

    msg->Status = KphEnumerateProcessHandles(msg->ProcessHandle,
                                             msg->Buffer,
                                             msg->BufferLength,
                                             msg->ReturnLength,
                                             UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsQueryInformationObject(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_INFORMATION_OBJECT msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationObject);

    msg = &Message->User.QueryInformationObject;

    msg->Status = KphQueryInformationObject(msg->ProcessHandle,
                                            msg->Handle,
                                            msg->ObjectInformationClass,
                                            msg->ObjectInformation,
                                            msg->ObjectInformationLength,
                                            msg->ReturnLength,
                                            UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsSetInformationObject(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SET_INFORMATION_OBJECT msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSetInformationObject);

    msg = &Message->User.SetInformationObject;

    msg->Status = KphSetInformationObject(msg->ProcessHandle,
                                          msg->Handle,
                                          msg->ObjectInformationClass,
                                          msg->ObjectInformation,
                                          msg->ObjectInformationLength,
                                          UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsOpenDriver(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_DRIVER msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenDriver);

    msg = &Message->User.OpenDriver;

    msg->Status = KphOpenDriver(msg->DriverHandle,
                                msg->DesiredAccess,
                                msg->ObjectAttributes,
                                UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsQueryInformationDriver(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_INFORMATION_DRIVER msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationDriver);

    msg = &Message->User.QueryInformationDriver;

    msg->Status = KphQueryInformationDriver(msg->DriverHandle,
                                            msg->DriverInformationClass,
                                            msg->DriverInformation,
                                            msg->DriverInformationLength,
                                            msg->ReturnLength,
                                            UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsQueryInformationProcessRequires(
    _In_ PCKPH_MESSAGE Message
    )
{
    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationProcess);

    switch (Message->User.QueryInformationProcess.ProcessInformationClass)
    {
        case KphProcessBasicInformation:
        {
            return KPH_PROCESS_STATE_MEDIUM;
        }
        case KphProcessStateInformation:
        {
            return KPH_PROCESS_STATE_LOW;
        }
        default:
        {
            return KPH_PROCESS_STATE_MAXIMUM;
        }
    }
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsQueryInformationProcess(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_INFORMATION_PROCESS msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationProcess);

    msg = &Message->User.QueryInformationProcess;

    msg->Status = KphQueryInformationProcess(msg->ProcessHandle,
                                             msg->ProcessInformationClass,
                                             msg->ProcessInformation,
                                             msg->ProcessInformationLength,
                                             msg->ReturnLength,
                                             UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsSetInformationProcess(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SET_INFORMATION_PROCESS msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSetInformationProcess);

    msg = &Message->User.SetInformationProcess;

    msg->Status = KphSetInformationProcess(msg->ProcessHandle,
                                           msg->ProcessInformationClass,
                                           msg->ProcessInformation,
                                           msg->ProcessInformationLength,
                                           UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsSetInformationThread(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SET_INFORMATION_THREAD msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSetInformationThread);

    msg = &Message->User.SetInformationThread;

    msg->Status = KphSetInformationThread(msg->ThreadHandle,
                                          msg->ThreadInformationClass,
                                          msg->ThreadInformation,
                                          msg->ThreadInformationLength,
                                          UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsSystemControl(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SYSTEM_CONTROL msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSystemControl);

    msg = &Message->User.SystemControl;

    msg->Status = KphSystemControl(msg->SystemControlClass,
                                   msg->SystemControlInfo,
                                   msg->SystemControlInfoLength,
                                   UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsAlpcQueryInformation(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_ALPC_QUERY_INFORMATION msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgAlpcQueryInformation);

    msg = &Message->User.AlpcQueryInformation;

    msg->Status = KphAlpcQueryInformation(msg->ProcessHandle,
                                          msg->PortHandle,
                                          msg->AlpcInformationClass,
                                          msg->AlpcInformation,
                                          msg->AlpcInformationLength,
                                          msg->ReturnLength,
                                          UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsQueryInformationFile(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_INFORMATION_FILE msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationFile);

    msg = &Message->User.QueryInformationFile;

    msg->Status = KphQueryInformationFile(msg->ProcessHandle,
                                          msg->FileHandle,
                                          msg->FileInformationClass,
                                          msg->FileInformation,
                                          msg->FileInformationLength,
                                          msg->IoStatusBlock,
                                          UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsQueryVolumeInformationFile(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_VOLUME_INFORMATION_FILE msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryVolumeInformationFile);

    msg = &Message->User.QueryVolumeInformationFile;

    msg->Status = KphQueryVolumeInformationFile(msg->ProcessHandle,
                                                msg->FileHandle,
                                                msg->FsInformationClass,
                                                msg->FsInformation,
                                                msg->FsInformationLength,
                                                msg->IoStatusBlock,
                                                UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsDuplicateObject(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_DUPLICATE_OBJECT msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgDuplicateObject);

    msg = &Message->User.DuplicateObject;

    msg->Status = KphDuplicateObject(msg->ProcessHandle,
                                     msg->SourceHandle,
                                     msg->DesiredAccess,
                                     msg->TargetHandle,
                                     UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsQueryPerformanceCounter(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_PERFORMANCE_COUNTER msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryPerformanceCounter);

    msg = &Message->User.QueryPerformanceCounter;

    msg->PerformanceCounter = KeQueryPerformanceCounter(&msg->PerformanceFrequency);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsCreateFileRequires(
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;
    ULONG createDisposition;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgCreateFile);

    desiredAccess = Message->User.CreateFile.DesiredAccess;

    if ((desiredAccess & KPH_FILE_READ_ACCESS) != desiredAccess)
    {
        return KPH_PROCESS_STATE_MAXIMUM;
    }

    createDisposition = Message->User.CreateFile.CreateDisposition;

    if (createDisposition != KPH_FILE_READ_DISPOSITION)
    {
        return KPH_PROCESS_STATE_MAXIMUM;
    }

    return KPH_PROCESS_STATE_MEDIUM;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsCreateFile(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_CREATE_FILE msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgCreateFile);

    msg = &Message->User.CreateFile;

    msg->Status = KphCreateFile(msg->FileHandle,
                                msg->DesiredAccess,
                                msg->ObjectAttributes,
                                msg->IoStatusBlock,
                                msg->AllocationSize,
                                msg->FileAttributes,
                                msg->ShareAccess,
                                msg->CreateDisposition,
                                msg->CreateOptions,
                                msg->EaBuffer,
                                msg->EaLength,
                                msg->Options,
                                UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsQueryInformationThread(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_INFORMATION_THREAD msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationThread);

    msg = &Message->User.QueryInformationThread;

    msg->Status = KphQueryInformationThread(msg->ThreadHandle,
                                            msg->ThreadInformationClass,
                                            msg->ThreadInformation,
                                            msg->ThreadInformationLength,
                                            msg->ReturnLength,
                                            UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsQuerySection(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_SECTION msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQuerySection);

    msg = &Message->User.QuerySection;

    msg->Status = KphQuerySection(msg->SectionHandle,
                                  msg->SectionInformationClass,
                                  msg->SectionInformation,
                                  msg->SectionInformationLength,
                                  msg->ReturnLength,
                                  UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsCompareObjects(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_COMPARE_OBJECTS msg;

    PAGED_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgCompareObjects);

    msg = &Message->User.CompareObjects;

    msg->Status = KphCompareObjects(msg->ProcessHandle,
                                    msg->FirstObjectHandle,
                                    msg->SecondObjectHandle,
                                    UserMode);

    return STATUS_SUCCESS;
}
