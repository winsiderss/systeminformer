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

#endif
