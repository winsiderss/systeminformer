#ifndef PHSVCCL_H
#define PHSVCCL_H

#include <phsvcapi.h>

NTSTATUS PhSvcConnectToServer(
    __in PUNICODE_STRING PortName,
    __in_opt SIZE_T PortSectionSize
    );

VOID PhSvcDisconnectFromServer();

NTSTATUS PhSvcCallClose(
    __in HANDLE Handle
    );

NTSTATUS PhSvcCallExecuteRunAsCommand(
    __in PWSTR ServiceCommandLine,
    __in PWSTR ServiceName
    );

NTSTATUS PhSvcCallUnloadDriver(
    __in_opt PVOID BaseAddress,
    __in_opt PWSTR Name
    );

NTSTATUS PhSvcCallControlProcess(
    __in HANDLE ProcessId,
    __in PHSVC_API_CONTROLPROCESS_COMMAND Command
    );

NTSTATUS PhSvcCallControlService(
    __in PWSTR ServiceName,
    __in PHSVC_API_CONTROLSERVICE_COMMAND Command
    );

NTSTATUS PhSvcCallCreateService(
    __in PWSTR ServiceName,
    __in_opt PWSTR DisplayName,
    __in ULONG ServiceType,
    __in ULONG StartType,
    __in ULONG ErrorControl,
    __in_opt PWSTR BinaryPathName,
    __in_opt PWSTR LoadOrderGroup,
    __out_opt PULONG TagId,
    __in_opt PWSTR Dependencies,
    __in_opt PWSTR ServiceStartName,
    __in_opt PWSTR Password
    );

NTSTATUS PhSvcCallChangeServiceConfig(
    __in PWSTR ServiceName,
    __in ULONG ServiceType,
    __in ULONG StartType,
    __in ULONG ErrorControl,
    __in_opt PWSTR BinaryPathName,
    __in_opt PWSTR LoadOrderGroup,
    __out_opt PULONG TagId,
    __in_opt PWSTR Dependencies,
    __in_opt PWSTR ServiceStartName,
    __in_opt PWSTR Password,
    __in_opt PWSTR DisplayName
    );

NTSTATUS PhSvcCallChangeServiceConfig2(
    __in PWSTR ServiceName,
    __in ULONG InfoLevel,
    __in PVOID Info
    );

NTSTATUS PhSvcCallSetTcpEntry(
    __in PVOID TcpRow
    );

#endif
