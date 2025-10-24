/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2024
 *
 */

#include <kph.h>
#include <comms.h>
#include <informer.h>

#include <trace.h>

KPHM_DEFINE_HANDLER(KphpCommsGetInformerSettings);
KPHM_DEFINE_HANDLER(KphpCommsSetInformerSettings);
KPHM_DEFINE_HANDLER(KphpCommsOpenProcess);
KPHM_DEFINE_HANDLER(KphpCommsOpenProcessToken);
KPHM_DEFINE_HANDLER(KphpCommsOpenProcessJob);
KPHM_DEFINE_HANDLER(KphpCommsTerminateProcess);
KPHM_DEFINE_HANDLER(KphpCommsReadVirtualMemory);
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
KPHM_DEFINE_HANDLER(KphpCommsGetMessageTimeouts);
KPHM_DEFINE_HANDLER(KphpCommsSetMessageTimeouts);
KPHM_DEFINE_HANDLER(KphpCommsAcquireDriverUnloadProtection);
KPHM_DEFINE_HANDLER(KphpCommsReleaseDriverUnloadProtection);
KPHM_DEFINE_HANDLER(KphpCommsGetConnectedClientCount);
KPHM_DEFINE_HANDLER(KphpCommsActivateDynData);
KPHM_DEFINE_HANDLER(KphpCommsRequestSessionAccessToken);
KPHM_DEFINE_HANDLER(KphpCommsAssignProcessSessionToken);
KPHM_DEFINE_HANDLER(KphpCommsAssignThreadSessionToken);
KPHM_DEFINE_HANDLER(KphpCommsGetInformerProcessFilter);
KPHM_DEFINE_HANDLER(KphpCommsSetInformerProcessFilter);
KPHM_DEFINE_HANDLER(KphpCommsStripProtectedProcessMasks);
KPHM_DEFINE_HANDLER(KphpCommsQueryVirtualMemory);
KPHM_DEFINE_HANDLER(KphpCommsQueryHashInformationFile);
KPHM_DEFINE_HANDLER(KphpCommsOpenDevice);
KPHM_DEFINE_HANDLER(KphpCommsOpenDeviceDriver);
KPHM_DEFINE_HANDLER(KphpCommsOpenDeviceBaseDevice);

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
KPHM_DEFINE_REQUIRED_STATE(KphpCommsQueryInformationThreadRequires);
KPHM_DEFINE_REQUIRED_STATE(KphpCommsQueryVirtualMemoryRequires);

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
const KPH_MESSAGE_HANDLER KphCommsMessageHandlers[] =
{
{ InvalidKphMsg,                       NULL,                                   NULL },
{ KphMsgGetInformerSettings,           KphpCommsGetInformerSettings,           KphpCommsRequireLow },
{ KphMsgSetInformerSettings,           KphpCommsSetInformerSettings,           KphpCommsRequireLow },
{ KphMsgOpenProcess,                   KphpCommsOpenProcess,                   KphpCommsOpenProcessRequires },
{ KphMsgOpenProcessToken,              KphpCommsOpenProcessToken,              KphpCommsOpenProcessTokenRequires },
{ KphMsgOpenProcessJob,                KphpCommsOpenProcessJob,                KphpCommsOpenProcessJobRequires },
{ KphMsgTerminateProcess,              KphpCommsTerminateProcess,              KphpCommsRequireMaximum },
{ KphMsgReadVirtualMemory,             KphpCommsReadVirtualMemory,             KphpCommsRequireMaximum },
{ KphMsgOpenThread,                    KphpCommsOpenThread,                    KphpCommsOpenThreadRequires },
{ KphMsgOpenThreadProcess,             KphpCommsOpenThreadProcess,             KphpCommsOpenThreadProcessRequires },
{ KphMsgCaptureStackBackTraceThread,   KphpCommsCaptureStackBackTraceThread,   KphpCommsRequireMedium },
{ KphMsgEnumerateProcessHandles,       KphpCommsEnumerateProcessHandles,       KphpCommsRequireMedium },
{ KphMsgQueryInformationObject,        KphpCommsQueryInformationObject,        KphpCommsRequireMedium },
{ KphMsgSetInformationObject,          KphpCommsSetInformationObject,          KphpCommsRequireMaximum },
{ KphMsgOpenDriver,                    KphpCommsOpenDriver,                    KphpCommsRequireMaximum },
{ KphMsgQueryInformationDriver,        KphpCommsQueryInformationDriver,        KphpCommsRequireMaximum },
{ KphMsgQueryInformationProcess,       KphpCommsQueryInformationProcess,       KphpCommsQueryInformationProcessRequires },
{ KphMsgSetInformationProcess,         KphpCommsSetInformationProcess,         KphpCommsRequireMaximum },
{ KphMsgSetInformationThread,          KphpCommsSetInformationThread,          KphpCommsRequireMaximum },
{ KphMsgSystemControl,                 KphpCommsSystemControl,                 KphpCommsRequireMaximum },
{ KphMsgAlpcQueryInformation,          KphpCommsAlpcQueryInformation,          KphpCommsRequireMedium },
{ KphMsgQueryInformationFile,          KphpCommsQueryInformationFile,          KphpCommsRequireMedium },
{ KphMsgQueryVolumeInformationFile,    KphpCommsQueryVolumeInformationFile,    KphpCommsRequireMedium },
{ KphMsgDuplicateObject,               KphpCommsDuplicateObject,               KphpCommsRequireMaximum },
{ KphMsgQueryPerformanceCounter,       KphpCommsQueryPerformanceCounter,       KphpCommsRequireLow },
{ KphMsgCreateFile,                    KphpCommsCreateFile,                    KphpCommsCreateFileRequires },
{ KphMsgQueryInformationThread,        KphpCommsQueryInformationThread,        KphpCommsQueryInformationThreadRequires },
{ KphMsgQuerySection,                  KphpCommsQuerySection,                  KphpCommsRequireMedium },
{ KphMsgCompareObjects,                KphpCommsCompareObjects,                KphpCommsRequireMedium },
{ KphMsgGetMessageTimeouts,            KphpCommsGetMessageTimeouts,            KphpCommsRequireLow },
{ KphMsgSetMessageTimeouts,            KphpCommsSetMessageTimeouts,            KphpCommsRequireLow },
{ KphMsgAcquireDriverUnloadProtection, KphpCommsAcquireDriverUnloadProtection, KphpCommsRequireMaximum },
{ KphMsgReleaseDriverUnloadProtection, KphpCommsReleaseDriverUnloadProtection, KphpCommsRequireMaximum },
{ KphMsgGetConnectedClientCount,       KphpCommsGetConnectedClientCount,       KphpCommsRequireLow },
{ KphMsgActivateDynData,               KphpCommsActivateDynData,               KphpCommsRequireLow },
{ KphMsgRequestSessionAccessToken,     KphpCommsRequestSessionAccessToken,     KphpCommsRequireMaximum },
{ KphMsgAssignProcessSessionToken,     KphpCommsAssignProcessSessionToken,     KphpCommsRequireMaximum },
{ KphMsgAssignThreadSessionToken,      KphpCommsAssignThreadSessionToken,      KphpCommsRequireMaximum },
{ KphMsgGetInformerProcessFilter,      KphpCommsGetInformerProcessFilter,      KphpCommsRequireLow },
{ KphMsgSetInformerProcessFilter,      KphpCommsSetInformerProcessFilter,      KphpCommsRequireLow },
{ KphMsgStripProtectedProcessMasks,    KphpCommsStripProtectedProcessMasks,    KphpCommsRequireMaximum },
{ KphMsgQueryVirtualMemory,            KphpCommsQueryVirtualMemory,            KphpCommsQueryVirtualMemoryRequires },
{ KphMsgQueryHashInformationFile,      KphpCommsQueryHashInformationFile,      KphpCommsRequireMaximum },
{ KphMsgOpenDevice,                    KphpCommsOpenDevice,                    KphpCommsRequireMaximum },
{ KphMsgOpenDeviceDriver,              KphpCommsOpenDeviceDriver,              KphpCommsRequireMaximum },
{ KphMsgOpenDeviceBaseDevice,          KphpCommsOpenDeviceBaseDevice,          KphpCommsRequireMaximum },
};
const ULONG KphCommsMessageHandlerCount = ARRAYSIZE(KphCommsMessageHandlers);
C_ASSERT(ARRAYSIZE(KphCommsMessageHandlers) == MaxKphMsgClient);
KPH_PROTECTED_DATA_SECTION_RO_POP();

KPH_PAGED_FILE();

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsRequireMaximum(
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);

    UNREFERENCED_PARAMETER(Client);
    UNREFERENCED_PARAMETER(Message);

    return KPH_PROCESS_STATE_MAXIMUM;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsRequireMedium(
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);

    UNREFERENCED_PARAMETER(Client);
    UNREFERENCED_PARAMETER(Message);

    return KPH_PROCESS_STATE_MEDIUM;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsRequireLow(
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);

    UNREFERENCED_PARAMETER(Client);
    UNREFERENCED_PARAMETER(Message);

    return KPH_PROCESS_STATE_LOW;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsGetInformerSettings(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_GET_INFORMER_SETTINGS msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgGetInformerSettings);

    msg = &Message->User.GetInformerSettings;

    KphGetInformerSettings(&msg->Settings, &Client->InformerSettings);

    msg->Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsSetInformerSettings(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SET_INFORMER_SETTINGS msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSetInformerSettings);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.SetInformerSettings;

    KphSetInformerSettings(&Client->InformerSettings, &msg->Settings);

    msg->Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsOpenProcessRequires(
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcess);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_PROCESS msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcess);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcessToken);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_PROCESS_TOKEN msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcessToken);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcessJob);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_PROCESS_JOB msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenProcessJob);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_TERMINATE_PROCESS msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgTerminateProcess);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.TerminateProcess;

    msg->Status = KphTerminateProcess(msg->ProcessHandle,
                                      msg->ExitStatus,
                                      UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsReadVirtualMemory(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_READ_VIRTUAL_MEMORY msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgReadVirtualMemory);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.ReadVirtualMemory;

    msg->Status = KphReadVirtualMemory(msg->ProcessHandle,
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
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenThread);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_THREAD msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenThread);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenThreadProcess);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_THREAD_PROCESS msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenThreadProcess);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_CAPTURE_STACK_BACKTRACE_THREAD msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgCaptureStackBackTraceThread);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.CaptureStackBackTraceThread;

    msg->Status = KphCaptureStackBackTraceThreadByHandle(msg->ThreadHandle,
                                                         msg->FramesToSkip,
                                                         msg->FramesToCapture,
                                                         msg->BackTrace,
                                                         msg->CapturedFrames,
                                                         msg->BackTraceHash,
                                                         msg->Flags,
                                                         msg->Timeout,
                                                         UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsEnumerateProcessHandles(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_ENUMERATE_PROCESS_HANDLES msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgEnumerateProcessHandles);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_INFORMATION_OBJECT msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationObject);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SET_INFORMATION_OBJECT msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSetInformationObject);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_DRIVER msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenDriver);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_INFORMATION_DRIVER msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationDriver);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationProcess);

    UNREFERENCED_PARAMETER(Client);

    switch (Message->User.QueryInformationProcess.ProcessInformationClass)
    {
        case KphProcessWSLProcessId:
        case KphProcessStateInformation:
        case KphProcessSequenceNumber:
        case KphProcessStartKey:
        {
            return KPH_PROCESS_STATE_LOW;
        }
        case KphProcessBasicInformation:
        case KphProcessImageSection:
        {
            return KPH_PROCESS_STATE_MEDIUM;
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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_INFORMATION_PROCESS msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationProcess);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SET_INFORMATION_PROCESS msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSetInformationProcess);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SET_INFORMATION_THREAD msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSetInformationThread);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SYSTEM_CONTROL msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSystemControl);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_ALPC_QUERY_INFORMATION msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgAlpcQueryInformation);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_INFORMATION_FILE msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationFile);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_VOLUME_INFORMATION_FILE msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryVolumeInformationFile);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_DUPLICATE_OBJECT msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgDuplicateObject);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_PERFORMANCE_COUNTER msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryPerformanceCounter);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.QueryPerformanceCounter;

    msg->PerformanceCounter = KeQueryPerformanceCounter(&msg->PerformanceFrequency);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsCreateFileRequires(
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    ACCESS_MASK desiredAccess;
    ULONG createDisposition;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgCreateFile);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_CREATE_FILE msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgCreateFile);

    UNREFERENCED_PARAMETER(Client);

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

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsQueryInformationThreadRequires(
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationThread);

    UNREFERENCED_PARAMETER(Client);

    switch (Message->User.QueryInformationThread.ThreadInformationClass)
    {
        case KphThreadIoCounters:
        case KphThreadWSLThreadId:
        {
            return KPH_PROCESS_STATE_LOW;
        }
        case KphThreadKernelStackInformation:
        {
            return KPH_PROCESS_STATE_MAXIMUM;
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
NTSTATUS KSIAPI KphpCommsQueryInformationThread(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_INFORMATION_THREAD msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryInformationThread);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_SECTION msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQuerySection);

    UNREFERENCED_PARAMETER(Client);

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
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_COMPARE_OBJECTS msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgCompareObjects);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.CompareObjects;

    msg->Status = KphCompareObjects(msg->ProcessHandle,
                                    msg->FirstObjectHandle,
                                    msg->SecondObjectHandle,
                                    UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsGetMessageTimeouts(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_GET_MESSAGE_TIMEOUTS msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgGetMessageTimeouts);

    msg = &Message->User.GetMessageTimeouts;

    KphGetMessageTimeouts(Client, &msg->Timeouts);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsSetMessageTimeouts(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SET_MESSAGE_TIMEOUTS msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSetMessageTimeouts);

    msg = &Message->User.SetMessageTimeouts;

    msg->Status = KphSetMessageTimeouts(Client, &msg->Timeouts);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsAcquireDriverUnloadProtection(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_ACQUIRE_DRIVER_UNLOAD_PROTECTION msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgAcquireDriverUnloadProtection);

    msg = &Message->User.AcquireDriverUnloadProtection;

    msg->Status = KphAcquireReference(&Client->DriverUnloadProtectionRef,
                                      &msg->ClientPreviousCount);
    if (NT_SUCCESS(msg->Status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      PROTECTION,
                      "Client %wZ (%lu) "
                      "acquired driver unload protection (%ld)",
                      &Client->Process->ImageName,
                      HandleToULong(Client->Process->ProcessId),
                      msg->ClientPreviousCount + 1);

        if (msg->ClientPreviousCount == 0)
        {
            msg->Status = KphAcquireDriverUnloadProtection(&msg->PreviousCount);
        }
        else
        {
            msg->PreviousCount = KphGetDriverUnloadProtectionCount();
        }
    }

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsReleaseDriverUnloadProtection(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_RELEASE_DRIVER_UNLOAD_PROTECTION msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgReleaseDriverUnloadProtection);

    msg = &Message->User.ReleaseDriverUnloadProtection;

    msg->Status = KphReleaseReference(&Client->DriverUnloadProtectionRef,
                                      &msg->ClientPreviousCount);
    if (NT_SUCCESS(msg->Status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      PROTECTION,
                      "Client %wZ (%lu) "
                      "released driver unload protection (%ld)",
                      &Client->Process->ImageName,
                      HandleToULong(Client->Process->ProcessId),
                      msg->ClientPreviousCount - 1);

        if (msg->ClientPreviousCount == 1)
        {
            msg->Status = KphReleaseDriverUnloadProtection(&msg->PreviousCount);
        }
        else
        {
            msg->PreviousCount = KphGetDriverUnloadProtectionCount();
        }
    }

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsGetConnectedClientCount(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_GET_CONNECTED_CLIENT_COUNT msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgGetConnectedClientCount);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.GetConnectedClientCount;

    msg->Count = KphGetConnectedClientCount();

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsActivateDynData(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_ACTIVATE_DYNDATA msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgActivateDynData);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.ActivateDynData;

    msg->Status = KphActivateDynData(msg->DynData,
                                     msg->DynDataLength,
                                     msg->Signature,
                                     msg->SignatureLength,
                                     UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsRequestSessionAccessToken(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_REQUEST_SESSION_ACCESS_TOKEN msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgRequestSessionAccessToken);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.RequestSessionAccessToken;

    msg->Status = KphRequestSessionAccessToken(&msg->AccessToken,
                                               &msg->Expiry,
                                               msg->Privileges,
                                               msg->Uses);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsAssignProcessSessionToken(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_ASSIGN_PROCESS_SESSION_TOKEN msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgAssignProcessSessionToken);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.AssignProcessSessionToken;

    msg->Status = KphAssignProcessSessionToken(msg->ProcessHandle,
                                               msg->Signature,
                                               msg->SignatureLength,
                                               UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsAssignThreadSessionToken(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_ASSIGN_THREAD_SESSION_TOKEN msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgAssignThreadSessionToken);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.AssignThreadSessionToken;

    msg->Status = KphAssignThreadSessionToken(msg->ThreadHandle,
                                              msg->Signature,
                                              msg->SignatureLength,
                                              UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsGetInformerProcessFilter(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_GET_INFORMER_PROCESS_FILTER msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgGetInformerProcessFilter);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.GetInformerProcessFilter;

    msg->Status = KphGetInformerProcessFilter(msg->ProcessHandle,
                                              msg->Filter,
                                              UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsSetInformerProcessFilter(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_SET_INFORMER_PROCESS_FILTER msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgSetInformerProcessFilter);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.SetInformerProcessFilter;

    msg->Status = KphSetInformerProcessFilter(msg->ProcessHandle,
                                              msg->Filter,
                                              UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsStripProtectedProcessMasks(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_STRIP_PROTECTED_PROCESS_MASKS msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgStripProtectedProcessMasks);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.StripProtectedProcessMasks;

    msg->Status = KphStripProtectedProcessMasks(msg->ProcessHandle,
                                                msg->ProcessAllowedMask,
                                                msg->ThreadAllowedMask,
                                                UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE KSIAPI KphpCommsQueryVirtualMemoryRequires(
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryVirtualMemory);

    UNREFERENCED_PARAMETER(Client);

    switch (Message->User.QueryVirtualMemory.MemoryInformationClass)
    {
        case KphMemoryImageSection:
        case KphMemoryDataSection:
        {
            return KPH_PROCESS_STATE_MEDIUM;
        }
        case KphMemoryMappedInformation:
        default:
        {
            return KPH_PROCESS_STATE_MAXIMUM;
        }
    }
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsQueryVirtualMemory(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_VIRTUAL_MEMORY msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryVirtualMemory);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.QueryVirtualMemory;

    msg->Status = KphQueryVirtualMemory(msg->ProcessHandle,
                                        msg->BaseAddress,
                                        msg->MemoryInformationClass,
                                        msg->MemoryInformation,
                                        msg->MemoryInformationLength,
                                        msg->ReturnLength,
                                        UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsQueryHashInformationFile(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUERY_HASH_INFORMATION_FILE msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgQueryHashInformationFile);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.QueryHashInformationFile;

    msg->Status = KphQueryHashInformationFile(msg->FileHandle,
                                              msg->HashingInformation,
                                              msg->HashingInformationLength,
                                              UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsOpenDevice(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_DEVICE msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenDevice);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.OpenDevice;

    msg->Status = KphOpenDevice(msg->DeviceHandle,
                                msg->DesiredAccess,
                                msg->ObjectAttributes,
                                UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsOpenDeviceDriver(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_DEVICE_DRIVER msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenDeviceDriver);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.OpenDeviceDriver;

    msg->Status = KphOpenDeviceDriver(msg->DeviceHandle,
                                      msg->DesiredAccess,
                                      msg->DriverHandle,
                                      UserMode);

    return STATUS_SUCCESS;
}

_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpCommsOpenDeviceBaseDevice(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPHM_OPEN_DEVICE_BASE_DEVICE msg;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ExGetPreviousMode() == UserMode);
    NT_ASSERT(Message->Header.MessageId == KphMsgOpenDeviceBaseDevice);

    UNREFERENCED_PARAMETER(Client);

    msg = &Message->User.OpenDeviceBaseDevice;

    msg->Status = KphOpenDeviceBaseDevice(msg->DeviceHandle,
                                          msg->DesiredAccess,
                                          msg->BaseDeviceHandle,
                                          UserMode);

    return STATUS_SUCCESS;
}
