#ifndef PH_H
#define PH_H

#include <phbase.h>
#include <stdarg.h>

// process

#define SYSTEM_IDLE_PROCESS_ID ((HANDLE)0)
#define SYSTEM_PROCESS_ID ((HANDLE)4)

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

typedef enum _PH_INTEGRITY
{
    PiUntrusted = 0,
    PiLow = 1,
    PiMedium = 2,
    PiHigh = 3,
    PiSystem = 4,
    PiInstaller = 5
} PH_INTEGRITY, *PPH_INTEGRITY;

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

NTSTATUS PhGetProcessExecuteFlags(
    __in HANDLE ProcessHandle,
    __out PULONG ExecuteFlags
    );

NTSTATUS PhGetProcessIsWow64(
    __in HANDLE ProcessHandle,
    __out PBOOLEAN IsWow64
    );

NTSTATUS PhGetProcessIsPosix(
    __in HANDLE ProcessHandle,
    __out PBOOLEAN IsPosix
    );

NTSTATUS PhGetProcessPosixCommandLine(
    __in HANDLE ProcessHandle,
    __out PPH_STRING *CommandLine
    );

NTSTATUS PhSetProcessExecuteFlags(
    __in HANDLE ProcessHandle,
    __in ULONG ExecuteFlags
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

NTSTATUS PhGetTokenGroups(
    __in HANDLE TokenHandle,
    __out PTOKEN_GROUPS *Groups
    );

NTSTATUS PhGetTokenPrivileges(
    __in HANDLE TokenHandle,
    __out PTOKEN_PRIVILEGES *Privileges
    );

BOOLEAN PhSetTokenPrivilege(
    __in HANDLE TokenHandle,
    __in_opt PWSTR PrivilegeName,
    __in_opt PLUID PrivilegeLuid,
    __in ULONG Attributes
    );

NTSTATUS PhGetTokenIntegrityLevel(
    __in HANDLE TokenHandle,
    __out_opt PPH_INTEGRITY IntegrityLevel, 
    __out_opt PPH_STRING *IntegrityString
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

NTSTATUS PhDuplicateObject(
    __in HANDLE SourceProcessHandle,
    __in HANDLE SourceHandle,
    __in_opt HANDLE TargetProcessHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options
    );

#define PH_ENUM_PROCESS_MODULES_ITERS 0x800

typedef BOOLEAN (NTAPI *PPH_ENUM_PROCESS_MODULES_CALLBACK)(
    __in PLDR_DATA_TABLE_ENTRY Module
    );

NTSTATUS PhEnumProcessModules(
    __in HANDLE ProcessHandle,
    __in PPH_ENUM_PROCESS_MODULES_CALLBACK Callback
    );

PPH_STRING PhGetKernelFileName();

NTSTATUS PhEnumKernelModules(
    __out PRTL_PROCESS_MODULES *Modules
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

// verify

typedef enum _VERIFY_RESULT
{
    VrUnknown = 0,
    VrNoSignature,
    VrTrusted,
    VrExpired,
    VrRevoked,
    VrDistrust,
    VrSecuritySettings
} VERIFY_RESULT, *PVERIFY_RESULT;

VOID PhVerifyInitialization();

VERIFY_RESULT PhVerifyFile(
    __in PWSTR FileName,
    __out_opt PPH_STRING *SignerName
    );

// provider

typedef enum _PH_PROVIDER_THREAD_STATE
{
    ProviderThreadRunning,
    ProviderThreadStopped,
    ProviderThreadStopping
} PH_PROVIDER_THREAD_STATE;

typedef VOID (NTAPI *PPH_PROVIDER_FUNCTION)(
    __in PVOID Context
    );

typedef struct _PH_PROVIDER_REGISTRATION
{
    LIST_ENTRY ListEntry;
    PPH_PROVIDER_FUNCTION Function;
    PVOID Context;
    BOOLEAN Enabled;
    BOOLEAN Unregistering;
    BOOLEAN Boosting;
} PH_PROVIDER_REGISTRATION, *PPH_PROVIDER_REGISTRATION;

typedef struct _PH_PROVIDER_THREAD
{
    HANDLE ThreadHandle;
    HANDLE TimerHandle;
    ULONG Interval;
    PH_PROVIDER_THREAD_STATE State;

    PH_MUTEX Mutex;
    LIST_ENTRY ListHead;
    ULONG BoostCount;
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

VOID PhBoostProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __inout PPH_PROVIDER_REGISTRATION Registration
    );

VOID PhSetProviderEnabled(
    __in PPH_PROVIDER_REGISTRATION Registration,
    __in BOOLEAN Enabled
    );

VOID PhRegisterProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __in PPH_PROVIDER_FUNCTION Function,
    __in PVOID Context,
    __out PPH_PROVIDER_REGISTRATION Registration
    );

VOID PhUnregisterProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __inout PPH_PROVIDER_REGISTRATION Registration
    );

// procprv

#ifndef PROCPRV_PRIVATE
extern PPH_OBJECT_TYPE PhProcessItemType;

extern PH_CALLBACK PhProcessAddedEvent;
extern PH_CALLBACK PhProcessModifiedEvent;
extern PH_CALLBACK PhProcessRemovedEvent;
#endif

typedef struct _PH_IMAGE_VERSION_INFO
{
    PPH_STRING CompanyName;
    PPH_STRING FileDescription;
    PPH_STRING FileVersion;
    PPH_STRING ProductName;
} PH_IMAGE_VERSION_INFO, *PPH_IMAGE_VERSION_INFO;

#define PH_INTEGRITY_STR_LEN 10
#define PH_INTEGRITY_STR_LEN_1 (PH_INTEGRITY_STR_LEN + 1)

typedef struct _PH_PROCESS_ITEM
{
    HANDLE ProcessId;
    HANDLE ParentProcessId;
    PPH_STRING ProcessName;
    ULONG SessionId;

    PPH_STRING FileName;
    PPH_STRING CommandLine;

    HICON SmallIcon;
    HICON LargeIcon;
    PH_IMAGE_VERSION_INFO VersionInfo;

    LARGE_INTEGER CreateTime;

    PPH_STRING UserName;
    PH_INTEGRITY IntegrityLevel;

    PPH_STRING JobName;

    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;

    ULONG ImportFunctions;
    ULONG ImportModules;

    BOOLEAN HasParent : 1;
    BOOLEAN IsBeingDebugged : 1;
    BOOLEAN IsDotNet : 1;
    BOOLEAN IsElevated : 1;
    BOOLEAN IsInJob : 1;
    BOOLEAN IsInSignificantJob : 1;
    BOOLEAN IsPacked : 1;
    BOOLEAN IsPosix : 1;
    BOOLEAN IsWow64 : 1;

    BOOLEAN JustProcessed;
    PH_EVENT Stage1Event;

    WCHAR ProcessIdString[PH_INT_STR_LEN_1];
    WCHAR ParentProcessIdString[PH_INT_STR_LEN_1];
    WCHAR SessionIdString[PH_INT_STR_LEN_1];
    WCHAR IntegrityString[PH_INTEGRITY_STR_LEN_1];

    FLOAT CpuUsage; // from 0 to 1
} PH_PROCESS_ITEM, *PPH_PROCESS_ITEM;

BOOLEAN PhInitializeProcessProvider();

BOOLEAN PhInitializeImageVersionInfo(
    __out PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    __in PWSTR FileName
    );

VOID PhDeleteImageVersionInfo(
    __inout PPH_IMAGE_VERSION_INFO ImageVersionInfo
    );

PPH_PROCESS_ITEM PhCreateProcessItem(
    __in HANDLE ProcessId
    );

PPH_PROCESS_ITEM PhReferenceProcessItem(
    __in HANDLE ProcessId
    );

VOID PhProcessProviderUpdate(
    __in PVOID Context
    );

// srvprv

#ifndef SRVPRV_PRIVATE
extern PPH_OBJECT_TYPE PhServiceItemType;

extern PH_CALLBACK PhServiceAddedEvent;
extern PH_CALLBACK PhServiceModifiedEvent;
extern PH_CALLBACK PhServiceRemovedEvent;
#endif

typedef struct _PH_SERVICE_ITEM
{
    PPH_STRING Name;
    PPH_STRING DisplayName;

    // State
    ULONG Type;
    ULONG State;
    ULONG ControlsAccepted;
    ULONG ProcessId;

    // Config
    ULONG StartType;
    ULONG ErrorControl;

    WCHAR ProcessIdString[PH_INT_STR_LEN_1];
} PH_SERVICE_ITEM, *PPH_SERVICE_ITEM;

typedef struct _PH_SERVICE_MODIFIED_DATA
{
    PPH_SERVICE_ITEM Service;
    PH_SERVICE_ITEM OldService;
} PH_SERVICE_MODIFIED_DATA, *PPH_SERVICE_MODIFIED_DATA;

BOOLEAN PhInitializeServiceProvider();

PPH_SERVICE_ITEM PhReferenceServiceItem(
    __in PWSTR Name
    );

PVOID PhEnumServices(
    __in SC_HANDLE ScManagerHandle,
    __in_opt ULONG Type,
    __in_opt ULONG State,
    __out PULONG Count
    );

PVOID PhQueryServiceConfig(
    __in SC_HANDLE ServiceHandle
    );

VOID PhServiceProviderUpdate(
    __in PVOID Context
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

PVOID PhGetFileVersionInfo(
    __in PWSTR FileName
    );

ULONG PhGetFileVersionInfoLangCodePage(
    __in PVOID VersionInfo
    );

PPH_STRING PhGetFileVersionInfoString(
    __in PVOID VersionInfo,
    __in PWSTR SubBlock
    );

PPH_STRING PhGetFileVersionInfoString2(
    __in PVOID VersionInfo,
    __in ULONG LangCodePage,
    __in PWSTR StringName
    );

PPH_STRING PhGetFullPath(
    __in PWSTR FileName,
    __out_opt PULONG IndexOfFileName
    );

PPH_STRING PhGetSystemDirectory();

PPH_STRING PhGetApplicationFileName();

PPH_STRING PhGetApplicationDirectory();

FORCEINLINE PVOID PhAllocateCopy(
    __in PVOID Data,
    __in ULONG Size
    )
{
    PVOID copy;

    copy = PhAllocate(Size);
    memcpy(copy, Data, Size);

    return copy;
}

#endif
