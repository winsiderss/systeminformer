#ifndef PH_H
#define PH_H

#include <phbase.h>
#include <stdarg.h>

// process

typedef enum _PH_PEB_OFFSET
{
    PhpoCurrentDirectory,
    PhpoDllPath,
    PhpoImagePathName,
    PhpoCommandLine,
    PhpoWindowTitle,
    PhpoDesktopName,
    PhpoShellInfo,
    PhpoRuntimeData
} PH_PEB_OFFSET;

NTSTATUS PhOpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessId
    );

NTSTATUS PhOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ThreadId
    );

NTSTATUS PhOpenProcessToken(
    __out PHANDLE TokenHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessHandle
    );

NTSTATUS PhReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead
    );

NTSTATUS PhWriteVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesWritten
    );

NTSTATUS PhGetProcessBasicInformation(
    __in HANDLE ProcessHandle,
    __out PPROCESS_BASIC_INFORMATION BasicInformation
    );

NTSTATUS PhGetProcessImageFileName(
    __in HANDLE ProcessHandle,
    __out PPH_STRING *FileName
    );

#define PhGetProcessCommandLine(ProcessHandle, String) \
    PhGetProcessPebString(ProcessHandle, PhpoCommandLine, String)

NTSTATUS PhGetProcessPebString(
    __in HANDLE ProcessHandle,
    __in PH_PEB_OFFSET Offset,
    __out PPH_STRING *String
    );

NTSTATUS PhGetTokenUser(
    __in HANDLE TokenHandle,
    __out PTOKEN_USER *User
    );

NTSTATUS PhGetTokenElevationType(
    __in HANDLE TokenHandle,
    __out PTOKEN_ELEVATION_TYPE ElevationType
    );

NTSTATUS PhGetTokenIsElevated(
    __in HANDLE TokenHandle,
    __out PBOOLEAN Elevated
    );

BOOLEAN PhSetTokenPrivilege(
    __in HANDLE TokenHandle,
    __in_opt PWSTR PrivilegeName,
    __in_opt PLUID PrivilegeLuid,
    __in ULONG Attributes
    );

BOOLEAN PhLookupPrivilegeName(
    __in PLUID PrivilegeValue,
    __out PPH_STRING *PrivilegeName
    );

BOOLEAN PhLookupPrivilegeDisplayName(
    __in PWSTR PrivilegeName,
    __out PPH_STRING *PrivilegeDisplayName
    );

BOOLEAN PhLookupPrivilegeValue(
    __in PWSTR PrivilegeName,
    __out PLUID PrivilegeValue
    );

BOOLEAN PhLookupSid(
    __in PSID Sid,
    __out_opt PPH_STRING *Name,
    __out_opt PPH_STRING *DomainName,
    __out_opt PSID_NAME_USE NameUse
    );

#define PH_FIRST_PROCESS(Processes) ((PSYSTEM_PROCESS_INFORMATION)(Processes))
#define PH_NEXT_PROCESS(Process) ( \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset ? \
    (PSYSTEM_PROCESS_INFORMATION)((PCHAR)(Process) + \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset) : \
    NULL \
    )

NTSTATUS PhEnumProcesses(
    __out PPVOID Processes
    );

VOID PhInitializeDosDeviceNames();

VOID PhRefreshDosDeviceNames();

PPH_STRING PhGetFileName(
    __in PPH_STRING FileName
    );

// procprv

#ifndef PROCPRV_PRIVATE
extern PPH_OBJECT_TYPE PhProcessItemType;

extern PH_CALLBACK PhProcessAddedEvent;
extern PH_CALLBACK PhProcessModifiedEvent;
extern PH_CALLBACK PhProcessRemovedEvent;
#endif

typedef struct _PH_PROCESS_ITEM
{
    HANDLE ProcessId;
    HANDLE ParentProcessId;
    PPH_STRING ProcessName;
    ULONG SessionId;

    HICON SmallIcon;
    HICON LargeIcon;

    PPH_STRING FileName;
    PPH_STRING CommandLine;

    LARGE_INTEGER CreateTime;

    PPH_STRING UserName;
    ULONG IntegrityLevel;
    PPH_STRING IntegrityString;

    ULONG HasParent : 1;
    ULONG IsBeingDebugged : 1;
    ULONG IsDotNet : 1;
    ULONG IsElevated : 1;
    ULONG IsInJob : 1;
    ULONG IsInSignificantJob : 1;
    ULONG IsPacked : 1;
    ULONG IsPosix : 1;
    ULONG IsWow64 : 1;

    WCHAR ProcessIdString[PH_INT_STR_LEN_1];
    WCHAR ParentProcessIdString[PH_INT_STR_LEN_1];
    WCHAR SessionIdString[PH_INT_STR_LEN_1];

    FLOAT CpuUsage; // from 0 to 1
} PH_PROCESS_ITEM, *PPH_PROCESS_ITEM;

BOOLEAN PhInitializeProcessProvider();

BOOLEAN PhInitializeProcessItem();

PPH_PROCESS_ITEM PhCreateProcessItem(
    __in HANDLE ProcessId
    );

PPH_PROCESS_ITEM PhReferenceProcessItem(
    __in HANDLE ProcessId
    );

VOID PhUpdateProcesses();

VOID NTAPI PhProcessProviderUpdate(
    __in PVOID Parameter,
    __in PVOID Context
    );

// provider

typedef enum _PH_PROVIDER_THREAD_STATE
{
    ProviderThreadRunning,
    ProviderThreadStopped,
    ProviderThreadStopping
} PH_PROVIDER_THREAD_STATE;

typedef struct _PH_PROVIDER_THREAD
{
    HANDLE ThreadHandle;
    HANDLE TimerHandle;
    ULONG Interval;
    PH_CALLBACK RunEvent;
    PH_PROVIDER_THREAD_STATE State;
} PH_PROVIDER_THREAD, *PPH_PROVIDER_THREAD;

VOID PhInitializeProviderThread(
    __out PPH_PROVIDER_THREAD ProviderThread,
    __in ULONG Interval
    );

VOID PhDeleteProviderThread(
    __inout PPH_PROVIDER_THREAD ProviderThread
    );

VOID PhStartProviderThread(
    __inout PPH_PROVIDER_THREAD ProviderThread
    );

VOID PhStopProviderThread(
    __inout PPH_PROVIDER_THREAD ProviderThread
    );

VOID PhSetProviderThreadInterval(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __in ULONG Interval
    );

VOID PhRegisterProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __in PPH_CALLBACK_FUNCTION Function,
    __out PPH_CALLBACK_REGISTRATION Registration
    );

VOID PhUnregisterProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __inout PPH_CALLBACK_REGISTRATION Registration
    );

// support

#define PH_MAX_MESSAGE_SIZE 400

INT PhShowMessage(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    ...
    );

INT PhShowMessage_V(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    __in va_list ArgPtr
    );

#define PhShowError(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONERROR, Format, __VA_ARGS__)

PPH_STRING PhGetSystemDirectory(); 

#endif
