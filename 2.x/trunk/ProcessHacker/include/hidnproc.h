#ifndef HIDNPROC_H
#define HIDNPROC_H

#include <kph.h>

typedef enum _PH_KNOWN_PROCESS_TYPE
{
    UnknownProcessType,
    SystemProcessType,
    SessionManagerProcessType,
    WindowsSubsystemProcessType,
    WindowsStartupProcessType,
    ServiceControlManagerProcessType,
    LocalSecurityAuthorityProcessType,
    LocalSessionManagerProcessType
} PH_KNOWN_PROCESS_TYPE, *PPH_KNOWN_PROCESS_TYPE;

typedef struct _PH_CSR_HANDLE_INFO
{
    HANDLE CsrProcessHandle;
    HANDLE Handle;
    BOOLEAN IsThreadHandle;

    HANDLE ProcessId;
} PH_CSR_HANDLE_INFO, *PPH_CSR_HANDLE_INFO;

NTSTATUS PhGetProcessKnownType(
    __in HANDLE ProcessHandle,
    __out PPH_KNOWN_PROCESS_TYPE KnownProcessType
    );

NTSTATUS PhEnumProcessHandles(
    __in HANDLE ProcessHandle,
    __out PPROCESS_HANDLE_INFORMATION *Handles
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_CSR_PROCESS_HANDLES_CALLBACK)(
    __in PPH_CSR_HANDLE_INFO Handle,
    __in PVOID Context
    );

NTSTATUS PhEnumCsrProcessHandles(
    __in PPH_ENUM_CSR_PROCESS_HANDLES_CALLBACK Callback,
    __in PVOID Context
    );

NTSTATUS PhOpenProcessByCsrHandle(
    __out PHANDLE ProcessHandle,
    __in PPH_CSR_HANDLE_INFO Handle,
    __in ACCESS_MASK DesiredAccess
    );

#endif
