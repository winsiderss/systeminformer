#ifndef PHSVCCL_H
#define PHSVCCL_H

#include <phsvcapi.h>

NTSTATUS PhSvcConnectToServer(
    _In_ PUNICODE_STRING PortName,
    _In_opt_ SIZE_T PortSectionSize
    );

VOID PhSvcDisconnectFromServer(
    VOID
    );

NTSTATUS PhSvcCallExecuteRunAsCommand(
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    );

NTSTATUS PhSvcCallUnloadDriver(
    _In_opt_ PVOID BaseAddress,
    _In_opt_ PWSTR Name
    );

NTSTATUS PhSvcCallControlProcess(
    _In_ HANDLE ProcessId,
    _In_ PHSVC_API_CONTROLPROCESS_COMMAND Command,
    _In_ ULONG Argument
    );

NTSTATUS PhSvcCallControlService(
    _In_ PWSTR ServiceName,
    _In_ PHSVC_API_CONTROLSERVICE_COMMAND Command
    );

NTSTATUS PhSvcCallCreateService(
    _In_ PWSTR ServiceName,
    _In_opt_ PWSTR DisplayName,
    _In_ ULONG ServiceType,
    _In_ ULONG StartType,
    _In_ ULONG ErrorControl,
    _In_opt_ PWSTR BinaryPathName,
    _In_opt_ PWSTR LoadOrderGroup,
    _Out_opt_ PULONG TagId,
    _In_opt_ PWSTR Dependencies,
    _In_opt_ PWSTR ServiceStartName,
    _In_opt_ PWSTR Password
    );

NTSTATUS PhSvcCallChangeServiceConfig(
    _In_ PWSTR ServiceName,
    _In_ ULONG ServiceType,
    _In_ ULONG StartType,
    _In_ ULONG ErrorControl,
    _In_opt_ PWSTR BinaryPathName,
    _In_opt_ PWSTR LoadOrderGroup,
    _Out_opt_ PULONG TagId,
    _In_opt_ PWSTR Dependencies,
    _In_opt_ PWSTR ServiceStartName,
    _In_opt_ PWSTR Password,
    _In_opt_ PWSTR DisplayName
    );

NTSTATUS PhSvcCallChangeServiceConfig2(
    _In_ PWSTR ServiceName,
    _In_ ULONG InfoLevel,
    _In_ PVOID Info
    );

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

NTSTATUS PhSvcCallPostMessage(
    _In_opt_ HWND hWnd,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

NTSTATUS PhSvcCallSendMessage(
    _In_opt_ HWND hWnd,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif
