#ifndef PH_HIDNPROC_H
#define PH_HIDNPROC_H

typedef enum _PH_HIDDEN_PROCESS_METHOD
{
    BruteForceScanMethod,
    CsrHandlesScanMethod,
    ProcessHandleScanMethod,
    RegistryScanMethod,
    EtwGuidScanMethod,
    NtdllScanMethod,
} PH_HIDDEN_PROCESS_METHOD;

typedef enum _PH_HIDDEN_PROCESS_TYPE
{
    UnknownProcess,
    NormalProcess,
    HiddenProcess,
    TerminatedProcess
} PH_HIDDEN_PROCESS_TYPE;

typedef struct _PH_HIDDEN_PROCESS_ENTRY
{
    HANDLE ProcessId;
    PPH_STRING FileName;
    PPH_STRING FileNameWin32;
    PH_HIDDEN_PROCESS_TYPE Type;
} PH_HIDDEN_PROCESS_ENTRY, *PPH_HIDDEN_PROCESS_ENTRY;

typedef struct _PH_CSR_HANDLE_INFO
{
    HANDLE CsrProcessHandle;
    HANDLE Handle;
    BOOLEAN IsThreadHandle;

    HANDLE ProcessId;
} PH_CSR_HANDLE_INFO, *PPH_CSR_HANDLE_INFO;

typedef BOOLEAN (NTAPI *PPH_ENUM_HIDDEN_PROCESSES_CALLBACK)(
    _In_ PPH_HIDDEN_PROCESS_ENTRY Process,
    _In_opt_ PVOID Context
    );

NTSTATUS
NTAPI
PhEnumHiddenProcesses(
    _In_ PH_HIDDEN_PROCESS_METHOD Method,
    _In_ PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_CSR_PROCESS_HANDLES_CALLBACK)(
    _In_ PPH_CSR_HANDLE_INFO Handle,
    _In_opt_ PVOID Context
    );

NTSTATUS
NTAPI
PhEnumCsrProcessHandles(
    _In_ PPH_ENUM_CSR_PROCESS_HANDLES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

NTSTATUS
NTAPI
PhOpenProcessByCsrHandle(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_CSR_HANDLE_INFO Handle
    );

NTSTATUS
NTAPI
PhOpenProcessByCsrHandles(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId
    );

#endif
