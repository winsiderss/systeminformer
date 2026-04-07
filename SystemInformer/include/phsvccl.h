/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#ifndef PH_PHSVCCL_H
#define PH_PHSVCCL_H

#include <phsvcapi.h>

extern HANDLE PhSvcClServerProcessId;

NTSTATUS PhSvcConnectToServer(
    _In_ PUNICODE_STRING PortName,
    _In_opt_ SIZE_T PortSectionSize
    );

VOID PhSvcDisconnectFromServer(
    VOID
    );

VOID PhSvcpFreeHeap(
    _In_ PVOID Memory
    );

_Success_(return != NULL)
PVOID PhSvcpCreateString(
    _In_opt_ PCWSTR String,
    _In_ SIZE_T Length,
    _Out_ PPH_RELATIVE_STRINGREF StringRef
    );

NTSTATUS PhSvcCallPlugin(
    _In_ PPH_STRINGREF ApiId,
    _In_reads_bytes_opt_(InLength) PVOID InBuffer,
    _In_ ULONG InLength,
    _Out_writes_bytes_opt_(OutLength) PVOID OutBuffer,
    _In_ ULONG OutLength
    );

NTSTATUS PhSvcCallExecuteRunAsCommand(
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    );

NTSTATUS PhSvcCallUnloadDriver(
    _In_opt_ PVOID BaseAddress,
    _In_opt_ PCWSTR Name,
    _In_opt_ PCWSTR FileName
    );

NTSTATUS PhSvcCallControlProcess(
    _In_opt_ HANDLE ProcessId,
    _In_ PHSVC_API_CONTROLPROCESS_COMMAND Command,
    _In_ ULONG Argument
    );

NTSTATUS PhSvcCallControlService(
    _In_ PCWSTR ServiceName,
    _In_ PHSVC_API_CONTROLSERVICE_COMMAND Command
    );

NTSTATUS PhSvcCallCreateService(
    _In_ PCWSTR ServiceName,
    _In_opt_ PCWSTR DisplayName,
    _In_ ULONG ServiceType,
    _In_ ULONG StartType,
    _In_ ULONG ErrorControl,
    _In_opt_ PCWSTR BinaryPathName,
    _In_opt_ PCWSTR LoadOrderGroup,
    _Out_opt_ PULONG TagId,
    _In_opt_ PCWSTR Dependencies,
    _In_opt_ PCWSTR ServiceStartName,
    _In_opt_ PCWSTR Password
    );

// begin_phapppub
PHAPPAPI
NTSTATUS PhSvcCallChangeServiceConfig(
    _In_ PCWSTR ServiceName,
    _In_ ULONG ServiceType,
    _In_ ULONG StartType,
    _In_ ULONG ErrorControl,
    _In_opt_ PCWSTR BinaryPathName,
    _In_opt_ PCWSTR LoadOrderGroup,
    _Out_opt_ PULONG TagId,
    _In_opt_ PCWSTR Dependencies,
    _In_opt_ PCWSTR ServiceStartName,
    _In_opt_ PCWSTR Password,
    _In_opt_ PCWSTR DisplayName
    );

PHAPPAPI
NTSTATUS PhSvcCallChangeServiceConfig2(
    _In_ PCWSTR ServiceName,
    _In_ ULONG InfoLevel,
    _In_ PVOID Info
    );
// end_phapppub

NTSTATUS PhSvcCallSetTcpEntry(
    _In_ PVOID TcpRow
    );

NTSTATUS PhSvcCallControlThread(
    _In_ HANDLE ThreadId,
    _In_ PHSVC_API_CONTROLTHREAD_COMMAND Command,
    _In_ ULONG Argument
    );

NTSTATUS PhSvcCallAddAccountRight(
    _In_ PSID AccountSid,
    _In_ PUNICODE_STRING UserRight
    );

NTSTATUS PhSvcCallInvokeRunAsService(
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    );

NTSTATUS PhSvcCallIssueMemoryListCommand(
    _In_ SYSTEM_MEMORY_LIST_COMMAND Command
    );

// begin_phapppub
PHAPPAPI
NTSTATUS PhSvcCallPostMessage(
    _In_opt_ HWND hWnd,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

PHAPPAPI
NTSTATUS PhSvcCallSendMessage(
    _In_opt_ HWND hWnd,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );
// end_phapppub

NTSTATUS PhSvcCallCreateProcessIgnoreIfeoDebugger(
    _In_ PCWSTR FileName,
    _In_opt_ PCWSTR CommandLine
    );

NTSTATUS PhSvcCallSetServiceSecurity(
    _In_ PCWSTR ServiceName,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS PhSvcCallWriteMiniDumpProcess(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE FileHandle,
    _In_ ULONG DumpType
    );

NTSTATUS PhSvcCallQueryProcessHeapInformation(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING* HeapInformation
    );

NTSTATUS PhSvcCallCreateProcessForKsi(
    _In_ PCWSTR CommandLine,
    _In_ ULONG64 MitigationFlags[2]
    );

#endif
