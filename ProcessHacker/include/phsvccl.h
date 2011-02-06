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

#endif
