#ifndef _PH_PH_H
#define _PH_PH_H

#pragma once

#include <phbase.h>
#include <stdarg.h>
#include <phnatinl.h>

#ifdef __cplusplus
extern "C" {
#endif

// native

/** The PID of the idle process. */
#define SYSTEM_IDLE_PROCESS_ID ((HANDLE)0)
/** The PID of the system process. */
#define SYSTEM_PROCESS_ID ((HANDLE)4)

#define SYSTEM_IDLE_PROCESS_NAME (L"System Idle Process")

// General object-related function types

typedef NTSTATUS (NTAPI *PPH_OPEN_OBJECT)(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    );

typedef NTSTATUS (NTAPI *PPH_GET_OBJECT_SECURITY)(
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    );

typedef NTSTATUS (NTAPI *PPH_SET_OBJECT_SECURITY)(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenThreadProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenProcessToken(
    _Out_ PHANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenThreadToken(
    _Out_ PHANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadHandle,
    _In_ BOOLEAN OpenAsSelf
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetObjectSecurity(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetObjectSecurity(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

PHLIBAPI
NTSTATUS
NTAPI
PhTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSuspendProcess(
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhResumeProcess(
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhTerminateThread(
    _In_ HANDLE ThreadHandle,
    _In_ NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSuspendThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG PreviousSuspendCount
    );

PHLIBAPI
NTSTATUS
NTAPI
PhResumeThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG PreviousSuspendCount
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadContext(
    _In_ HANDLE ThreadHandle,
    _Inout_ PCONTEXT Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetThreadContext(
    _In_ HANDLE ThreadHandle,
    _In_ PCONTEXT Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhReadVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesWritten
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessImageFileName(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessImageFileNameWin32(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *FileName
    );

/** Specifies a PEB string. */
typedef enum _PH_PEB_OFFSET
{
    PhpoCurrentDirectory,
    PhpoDllPath,
    PhpoImagePathName,
    PhpoCommandLine,
    PhpoWindowTitle,
    PhpoDesktopInfo,
    PhpoShellInfo,
    PhpoRuntimeData,
    PhpoTypeMask = 0xffff,

    PhpoWow64 = 0x10000
} PH_PEB_OFFSET;

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessPebString(
    _In_ HANDLE ProcessHandle,
    _In_ PH_PEB_OFFSET Offset,
    _Out_ PPH_STRING *String
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *CommandLine
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessWindowTitle(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG WindowFlags,
    _Out_ PPH_STRING *WindowTitle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIsPosix(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsPosix
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessExecuteFlags(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ExecuteFlags
    );

#define PH_PROCESS_DEP_ENABLED 0x1
#define PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED 0x2
#define PH_PROCESS_DEP_PERMANENT 0x4

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessDepStatus(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG DepStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessPosixCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *CommandLine
    );

#define PH_GET_PROCESS_ENVIRONMENT_WOW64 0x1 // retrieve the WOW64 environment

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessEnvironment(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Flags,
    _Out_ PVOID *Environment,
    _Out_ PULONG EnvironmentLength
    );

typedef struct _PH_ENVIRONMENT_VARIABLE
{
    PH_STRINGREF Name;
    PH_STRINGREF Value;
} PH_ENVIRONMENT_VARIABLE, *PPH_ENVIRONMENT_VARIABLE;

PHLIBAPI
BOOLEAN
NTAPI
PhEnumProcessEnvironmentVariables(
    _In_ PVOID Environment,
    _In_ ULONG EnvironmentLength,
    _Inout_ PULONG EnumerationKey,
    _Out_ PPH_ENVIRONMENT_VARIABLE Variable
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessMappedFileName(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_ PPH_STRING *FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessWorkingSetInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PMEMORY_WORKING_SET_INFORMATION *WorkingSetInformation
    );

typedef struct _PH_PROCESS_WS_COUNTERS
{
    SIZE_T NumberOfPages;
    SIZE_T NumberOfPrivatePages;
    SIZE_T NumberOfSharedPages;
    SIZE_T NumberOfShareablePages;
} PH_PROCESS_WS_COUNTERS, *PPH_PROCESS_WS_COUNTERS;

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessWsCounters(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_WS_COUNTERS WsCounters
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessIoPriority(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG IoPriority
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessExecuteFlags(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG ExecuteFlags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessDepStatus(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG DepStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessDepStatusInvasive(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG DepStatus,
    _In_opt_ PLARGE_INTEGER Timeout
    );

PHLIBAPI
NTSTATUS
NTAPI
PhInjectDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PWSTR FileName,
    _In_opt_ PLARGE_INTEGER Timeout
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUnloadDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_opt_ PLARGE_INTEGER Timeout
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetThreadIoPriority(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG IoPriority
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetJobProcessIdList(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_PROCESS_ID_LIST *ProcessIdList
    );

NTSTATUS
NTAPI
PhQueryTokenVariableSize(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_ PVOID *Buffer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenUser(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_USER *User
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenOwner(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_OWNER *Owner
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenPrimaryGroup(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_PRIMARY_GROUP *PrimaryGroup
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenGroups(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_GROUPS *Groups
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenPrivileges(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_PRIVILEGES *Privileges
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetTokenSessionId(
    _In_ HANDLE TokenHandle,
    _In_ ULONG SessionId
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSetTokenPrivilege(
    _In_ HANDLE TokenHandle,
    _In_opt_ PWSTR PrivilegeName,
    _In_opt_ PLUID PrivilegeLuid,
    _In_ ULONG Attributes
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSetTokenPrivilege2(
    _In_ HANDLE TokenHandle,
    _In_ LONG Privilege,
    _In_ ULONG Attributes
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetTokenIsVirtualizationEnabled(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN IsVirtualizationEnabled
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenIntegrityLevel(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PMANDATORY_LEVEL IntegrityLevel,
    _Out_opt_ PWSTR *IntegrityString
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileSize(
    _In_ HANDLE FileHandle,
    _Out_ PLARGE_INTEGER Size
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetFileSize(
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER Size
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTransactionManagerBasicInformation(
    _In_ HANDLE TransactionManagerHandle,
    _Out_ PTRANSACTIONMANAGER_BASIC_INFORMATION BasicInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTransactionManagerLogFileName(
    _In_ HANDLE TransactionManagerHandle,
    _Out_ PPH_STRING *LogFileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTransactionBasicInformation(
    _In_ HANDLE TransactionHandle,
    _Out_ PTRANSACTION_BASIC_INFORMATION BasicInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTransactionPropertiesInformation(
    _In_ HANDLE TransactionHandle,
    _Out_opt_ PLARGE_INTEGER Timeout,
    _Out_opt_ TRANSACTION_OUTCOME *Outcome,
    _Out_opt_ PPH_STRING *Description
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetResourceManagerBasicInformation(
    _In_ HANDLE ResourceManagerHandle,
    _Out_opt_ PGUID Guid,
    _Out_opt_ PPH_STRING *Description
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetEnlistmentBasicInformation(
    _In_ HANDLE EnlistmentHandle,
    _Out_ PENLISTMENT_BASIC_INFORMATION BasicInformation
    );

NTSTATUS
NTAPI
PhOpenDriverByBaseAddress(
    _Out_ PHANDLE DriverHandle,
    _In_ PVOID BaseAddress
    );

NTSTATUS
NTAPI
PhGetDriverName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *Name
    );

NTSTATUS
NTAPI
PhGetDriverServiceKeyName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *ServiceKeyName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUnloadDriver(
    _In_opt_ PVOID BaseAddress,
    _In_opt_ PWSTR Name
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDuplicateObject(
    _In_ HANDLE SourceProcessHandle,
    _In_ HANDLE SourceHandle,
    _In_opt_ HANDLE TargetProcessHandle,
    _Out_opt_ PHANDLE TargetHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Options
    );

#define PH_ENUM_PROCESS_MODULES_LIMIT 0x800

/**
 * A callback function passed to PhEnumProcessModules()
 * and called for each process module.
 *
 * \param Module A structure providing information about
 * the module.
 * \param Context A user-defined value passed to
 * PhEnumProcessModules().
 *
 * \return TRUE to continue the enumeration, FALSE to
 * stop.
 */
typedef BOOLEAN (NTAPI *PPH_ENUM_PROCESS_MODULES_CALLBACK)(
    _In_ PLDR_DATA_TABLE_ENTRY Module,
    _In_opt_ PVOID Context
    );

#define PH_ENUM_PROCESS_MODULES_DONT_RESOLVE_WOW64_FS 0x1
#define PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME 0x2

typedef struct _PH_ENUM_PROCESS_MODULES_PARAMETERS
{
    PPH_ENUM_PROCESS_MODULES_CALLBACK Callback;
    PVOID Context;
    ULONG Flags;
} PH_ENUM_PROCESS_MODULES_PARAMETERS, *PPH_ENUM_PROCESS_MODULES_PARAMETERS;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumProcessModules(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumProcessModulesEx(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_PARAMETERS Parameters
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessModuleLoadCount(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG LoadCount
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumProcessModules32(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumProcessModules32Ex(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_PARAMETERS Parameters
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessModuleLoadCount32(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG LoadCount
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcedureAddressRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PWSTR FileName,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID *ProcedureAddress,
    _Out_opt_ PVOID *DllBase
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumKernelModules(
    _Out_ PRTL_PROCESS_MODULES *Modules
    );

NTSTATUS
NTAPI
PhEnumKernelModulesEx(
    _Out_ PRTL_PROCESS_MODULE_INFORMATION_EX *Modules
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetKernelFileName(
    VOID
    );

/**
 * Gets a pointer to the first process information
 * structure in a buffer returned by PhEnumProcesses().
 *
 * \param Processes A pointer to a buffer returned
 * by PhEnumProcesses().
 */
#define PH_FIRST_PROCESS(Processes) ((PSYSTEM_PROCESS_INFORMATION)(Processes))

/**
 * Gets a pointer to the process information structure
 * after a given structure.
 *
 * \param Process A pointer to a process information
 * structure.
 *
 * \return A pointer to the next process information
 * structure, or NULL if there are no more.
 */
#define PH_NEXT_PROCESS(Process) ( \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset ? \
    (PSYSTEM_PROCESS_INFORMATION)((PCHAR)(Process) + \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset) : \
    NULL \
    )

PHLIBAPI
NTSTATUS
NTAPI
PhEnumProcesses(
    _Out_ PVOID *Processes
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumProcessesEx(
    _Out_ PVOID *Processes,
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumProcessesForSession(
    _Out_ PVOID *Processes,
    _In_ ULONG SessionId
    );

PHLIBAPI
PSYSTEM_PROCESS_INFORMATION
NTAPI
PhFindProcessInformation(
    _In_ PVOID Processes,
    _In_ HANDLE ProcessId
    );

PHLIBAPI
PSYSTEM_PROCESS_INFORMATION
NTAPI
PhFindProcessInformationByImageName(
    _In_ PVOID Processes,
    _In_ PPH_STRINGREF ImageName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumHandles(
    _Out_ PSYSTEM_HANDLE_INFORMATION *Handles
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumHandlesEx(
    _Out_ PSYSTEM_HANDLE_INFORMATION_EX *Handles
    );

#define PH_FIRST_PAGEFILE(Pagefiles) ( \
    /* The size of a pagefile can never be 0. A TotalSize of 0
     * is used to indicate that there are no pagefiles.
     */ ((PSYSTEM_PAGEFILE_INFORMATION)(Pagefiles))->TotalSize ? \
    (PSYSTEM_PAGEFILE_INFORMATION)(Pagefiles) : \
    NULL \
    )
#define PH_NEXT_PAGEFILE(Pagefile) ( \
    ((PSYSTEM_PAGEFILE_INFORMATION)(Pagefile))->NextEntryOffset ? \
    (PSYSTEM_PAGEFILE_INFORMATION)((PCHAR)(Pagefile) + \
    ((PSYSTEM_PAGEFILE_INFORMATION)(Pagefile))->NextEntryOffset) : \
    NULL \
    )

PHLIBAPI
NTSTATUS
NTAPI
PhEnumPagefiles(
    _Out_ PVOID *Pagefiles
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessImageFileNameByProcessId(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING *FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIsDotNet(
    _In_ HANDLE ProcessId,
    _Out_ PBOOLEAN IsDotNet
    );

#define PH_CLR_USE_SECTION_CHECK 0x1
#define PH_CLR_NO_WOW64_CHECK 0x2
#define PH_CLR_KNOWN_IS_WOW64 0x4

#define PH_CLR_VERSION_1_0 0x1
#define PH_CLR_VERSION_1_1 0x2
#define PH_CLR_VERSION_2_0 0x4
#define PH_CLR_VERSION_4_ABOVE 0x8
#define PH_CLR_VERSION_MASK 0xf
#define PH_CLR_MSCORLIB_PRESENT 0x10000
#define PH_CLR_PROCESS_IS_WOW64 0x100000

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIsDotNetEx(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ ULONG InFlags,
    _Out_opt_ PBOOLEAN IsDotNet,
    _Out_opt_ PULONG Flags
    );

/**
 * A callback function passed to PhEnumDirectoryObjects()
 * and called for each directory object.
 *
 * \param Name The name of the object.
 * \param TypeName The name of the object's type.
 * \param Context A user-defined value passed to
 * PhEnumDirectoryObjects().
 *
 * \return TRUE to continue the enumeration, FALSE to
 * stop.
 */
typedef BOOLEAN (NTAPI *PPH_ENUM_DIRECTORY_OBJECTS)(
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumDirectoryObjects(
    _In_ HANDLE DirectoryHandle,
    _In_ PPH_ENUM_DIRECTORY_OBJECTS Callback,
    _In_opt_ PVOID Context
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_DIRECTORY_FILE)(
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumDirectoryFile(
    _In_ HANDLE FileHandle,
    _In_opt_ PUNICODE_STRING SearchPattern,
    _In_ PPH_ENUM_DIRECTORY_FILE Callback,
    _In_opt_ PVOID Context
    );

#define PH_FIRST_STREAM(Streams) ((PFILE_STREAM_INFORMATION)(Streams))
#define PH_NEXT_STREAM(Stream) ( \
    ((PFILE_STREAM_INFORMATION)(Stream))->NextEntryOffset ? \
    (PFILE_STREAM_INFORMATION)((PCHAR)(Stream) + \
    ((PFILE_STREAM_INFORMATION)(Stream))->NextEntryOffset) : \
    NULL \
    )

PHLIBAPI
NTSTATUS
NTAPI
PhEnumFileStreams(
    _In_ HANDLE FileHandle,
    _Out_ PVOID *Streams
    );

VOID
NTAPI
PhInitializeDevicePrefixes(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhUpdateMupDevicePrefixes(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhUpdateDosDevicePrefixes(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhResolveDevicePrefix(
    _In_ PPH_STRING Name
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetFileName(
    _In_ PPH_STRING FileName
    );

#define PH_MODULE_TYPE_MODULE 1
#define PH_MODULE_TYPE_MAPPED_FILE 2
#define PH_MODULE_TYPE_WOW64_MODULE 3
#define PH_MODULE_TYPE_KERNEL_MODULE 4
#define PH_MODULE_TYPE_MAPPED_IMAGE 5

typedef struct _PH_MODULE_INFO
{
    ULONG Type;
    PVOID BaseAddress;
    ULONG Size;
    PVOID EntryPoint;
    ULONG Flags;
    PPH_STRING Name;
    PPH_STRING FileName;

    USHORT LoadOrderIndex; // -1 if N/A
    USHORT LoadCount; // -1 if N/A
    USHORT LoadReason; // -1 if N/A
    USHORT Reserved;
    LARGE_INTEGER LoadTime; // 0 if N/A
} PH_MODULE_INFO, *PPH_MODULE_INFO;

/**
 * A callback function passed to PhEnumGenericModules()
 * and called for each process module.
 *
 * \param Module A structure providing information about
 * the module.
 * \param Context A user-defined value passed to
 * PhEnumGenericModules().
 *
 * \return TRUE to continue the enumeration, FALSE to
 * stop.
 */
typedef BOOLEAN (NTAPI *PPH_ENUM_GENERIC_MODULES_CALLBACK)(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    );

#define PH_ENUM_GENERIC_MAPPED_FILES 0x1
#define PH_ENUM_GENERIC_MAPPED_IMAGES 0x2

PHLIBAPI
NTSTATUS
NTAPI
PhEnumGenericModules(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ ULONG Flags,
    _In_ PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

#define PH_KEY_PREDEFINE(Number) ((HANDLE)(LONG_PTR)(-3 - (Number) * 2))
#define PH_KEY_IS_PREDEFINED(Predefine) (((LONG_PTR)(Predefine) < 0) && ((LONG_PTR)(Predefine) & 0x1))
#define PH_KEY_PREDEFINE_TO_NUMBER(Predefine) (ULONG)(((-(LONG_PTR)(Predefine) - 3) >> 1))

#define PH_KEY_LOCAL_MACHINE PH_KEY_PREDEFINE(0) // \Registry\Machine
#define PH_KEY_USERS PH_KEY_PREDEFINE(1) // \Registry\User
#define PH_KEY_CLASSES_ROOT PH_KEY_PREDEFINE(2) // \Registry\Machine\Software\Classes
#define PH_KEY_CURRENT_USER PH_KEY_PREDEFINE(3) // \Registry\User\<SID>
#define PH_KEY_CURRENT_USER_NUMBER 3
#define PH_KEY_MAXIMUM_PREDEFINE 4

PHLIBAPI
NTSTATUS
NTAPI
PhCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName,
    _In_ ULONG Attributes,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName,
    _In_ ULONG Attributes
    );

// lsa

PHLIBAPI
NTSTATUS
NTAPI
PhOpenLsaPolicy(
    _Out_ PLSA_HANDLE PolicyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PUNICODE_STRING SystemName
    );

LSA_HANDLE
NTAPI
PhGetLookupPolicyHandle(
    VOID
    );

PHLIBAPI
BOOLEAN
NTAPI
PhLookupPrivilegeName(
    _In_ PLUID PrivilegeValue,
    _Out_ PPH_STRING *PrivilegeName
    );

PHLIBAPI
BOOLEAN
NTAPI
PhLookupPrivilegeDisplayName(
    _In_ PPH_STRINGREF PrivilegeName,
    _Out_ PPH_STRING *PrivilegeDisplayName
    );

PHLIBAPI
BOOLEAN
NTAPI
PhLookupPrivilegeValue(
    _In_ PPH_STRINGREF PrivilegeName,
    _Out_ PLUID PrivilegeValue
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLookupSid(
    _In_ PSID Sid,
    _Out_opt_ PPH_STRING *Name,
    _Out_opt_ PPH_STRING *DomainName,
    _Out_opt_ PSID_NAME_USE NameUse
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLookupName(
    _In_ PPH_STRINGREF Name,
    _Out_opt_ PSID *Sid,
    _Out_opt_ PPH_STRING *DomainName,
    _Out_opt_ PSID_NAME_USE NameUse
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetSidFullName(
    _In_ PSID Sid,
    _In_ BOOLEAN IncludeDomain,
    _Out_opt_ PSID_NAME_USE NameUse
    );

PHLIBAPI
PPH_STRING
NTAPI
PhSidToStringSid(
    _In_ PSID Sid
    );

// hndlinfo

#define MAX_OBJECT_TYPE_NUMBER 257

typedef PPH_STRING (NTAPI *PPH_GET_CLIENT_ID_NAME)(
    _In_ PCLIENT_ID ClientId
    );

PPH_GET_CLIENT_ID_NAME
NTAPI
PhSetHandleClientIdFunction(
    _In_ PPH_GET_CLIENT_ID_NAME GetClientIdName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatNativeKeyName(
    _In_ PPH_STRING Name
    );

NTSTATUS
NTAPI
PhGetSectionFileName(
    _In_ HANDLE SectionHandle,
    _Out_ PPH_STRING *FileName
    );

PHLIBAPI
_Callback_ PPH_STRING
NTAPI
PhStdGetClientIdName(
    _In_ PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetHandleInformation(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ ULONG ObjectTypeNumber,
    _Out_opt_ POBJECT_BASIC_INFORMATION BasicInformation,
    _Out_opt_ PPH_STRING *TypeName,
    _Out_opt_ PPH_STRING *ObjectName,
    _Out_opt_ PPH_STRING *BestObjectName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetHandleInformationEx(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ ULONG ObjectTypeNumber,
    _Reserved_ ULONG Flags,
    _Out_opt_ PNTSTATUS SubStatus,
    _Out_opt_ POBJECT_BASIC_INFORMATION BasicInformation,
    _Out_opt_ PPH_STRING *TypeName,
    _Out_opt_ PPH_STRING *ObjectName,
    _Out_opt_ PPH_STRING *BestObjectName,
    _Reserved_ PVOID *ExtraInformation
    );

#define PH_FIRST_OBJECT_TYPE(ObjectTypes) \
    (POBJECT_TYPE_INFORMATION)((PCHAR)(ObjectTypes) + ALIGN_UP(sizeof(OBJECT_TYPES_INFORMATION), ULONG_PTR))

#define PH_NEXT_OBJECT_TYPE(ObjectType) \
    (POBJECT_TYPE_INFORMATION)((PCHAR)(ObjectType) + sizeof(OBJECT_TYPE_INFORMATION) + \
    ALIGN_UP(ObjectType->TypeName.MaximumLength, ULONG_PTR))

PHLIBAPI
NTSTATUS
NTAPI
PhEnumObjectTypes(
    _Out_ POBJECT_TYPES_INFORMATION *ObjectTypes
    );

ULONG
NTAPI
PhGetObjectTypeNumber(
    _In_ PUNICODE_STRING TypeName
    );

NTSTATUS
NTAPI
PhCallWithTimeout(
    _In_ PUSER_THREAD_START_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_opt_ PLARGE_INTEGER AcquireTimeout,
    _In_ PLARGE_INTEGER CallTimeout
    );

NTSTATUS
NTAPI
PhCallNtQueryObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSTATUS
NTAPI
PhCallNtQuerySecurityObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_writes_bytes_opt_(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Length,
    _Out_ PULONG LengthNeeded
    );

NTSTATUS
NTAPI
PhCallNtSetSecurityObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS
NTAPI
PhCallKphDuplicateObjectWithTimeout(
    _In_ HANDLE SourceProcessHandle,
    _In_ HANDLE SourceHandle,
    _In_opt_ HANDLE TargetProcessHandle,
    _Out_opt_ PHANDLE TargetHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Options
    );

// mapimg

typedef struct _PH_MAPPED_IMAGE
{
    PVOID ViewBase;
    SIZE_T Size;

    PIMAGE_NT_HEADERS NtHeaders;
    ULONG NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
    USHORT Magic;
} PH_MAPPED_IMAGE, *PPH_MAPPED_IMAGE;

PHLIBAPI
NTSTATUS
NTAPI
PhInitializeMappedImage(
    _Out_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID ViewBase,
    _In_ SIZE_T Size
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadMappedImage(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _In_ BOOLEAN ReadOnly,
    _Out_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUnloadMappedImage(
    _Inout_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhMapViewOfEntireFile(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _In_ BOOLEAN ReadOnly,
    _Out_ PVOID *ViewBase,
    _Out_ PSIZE_T Size
    );

PHLIBAPI
PIMAGE_SECTION_HEADER
NTAPI
PhMappedImageRvaToSection(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva
    );

PHLIBAPI
PVOID
NTAPI
PhMappedImageRvaToVa(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva,
    _Out_opt_ PIMAGE_SECTION_HEADER *Section
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetMappedImageSectionName(
    _In_ PIMAGE_SECTION_HEADER Section,
    _Out_writes_opt_z_(Count) PSTR Buffer,
    _In_ ULONG Count,
    _Out_opt_ PULONG ReturnCount
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageDataEntry(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Index,
    _Out_ PIMAGE_DATA_DIRECTORY *Entry
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageLoadConfig32(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PIMAGE_LOAD_CONFIG_DIRECTORY32 *LoadConfig
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageLoadConfig64(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PIMAGE_LOAD_CONFIG_DIRECTORY64 *LoadConfig
    );

typedef struct _PH_REMOTE_MAPPED_IMAGE
{
    PVOID ViewBase;

    PIMAGE_NT_HEADERS NtHeaders;
    ULONG NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
    USHORT Magic;
} PH_REMOTE_MAPPED_IMAGE, *PPH_REMOTE_MAPPED_IMAGE;

NTSTATUS
NTAPI
PhLoadRemoteMappedImage(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ViewBase,
    _Out_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    );

NTSTATUS
NTAPI
PhUnloadRemoteMappedImage(
    _Inout_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    );

typedef struct _PH_MAPPED_IMAGE_EXPORTS
{
    PPH_MAPPED_IMAGE MappedImage;
    ULONG NumberOfEntries;

    PIMAGE_DATA_DIRECTORY DataDirectory;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PULONG AddressTable;
    PULONG NamePointerTable;
    PUSHORT OrdinalTable;
} PH_MAPPED_IMAGE_EXPORTS, *PPH_MAPPED_IMAGE_EXPORTS;

typedef struct _PH_MAPPED_IMAGE_EXPORT_ENTRY
{
    USHORT Ordinal;
    PSTR Name;
} PH_MAPPED_IMAGE_EXPORT_ENTRY, *PPH_MAPPED_IMAGE_EXPORT_ENTRY;

typedef struct _PH_MAPPED_IMAGE_EXPORT_FUNCTION
{
    PVOID Function;
    PSTR ForwardedName;
} PH_MAPPED_IMAGE_EXPORT_FUNCTION, *PPH_MAPPED_IMAGE_EXPORT_FUNCTION;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExports(
    _Out_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExportEntry(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_EXPORT_ENTRY Entry
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExportFunction(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_opt_ PSTR Name,
    _In_opt_ USHORT Ordinal,
    _Out_ PPH_MAPPED_IMAGE_EXPORT_FUNCTION Function
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExportFunctionRemote(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_opt_ PSTR Name,
    _In_opt_ USHORT Ordinal,
    _In_ PVOID RemoteBase,
    _Out_ PVOID *Function
    );

#define PH_MAPPED_IMAGE_DELAY_IMPORTS 0x1

typedef struct _PH_MAPPED_IMAGE_IMPORTS
{
    PPH_MAPPED_IMAGE MappedImage;
    ULONG Flags;
    ULONG NumberOfDlls;

    union
    {
        PIMAGE_IMPORT_DESCRIPTOR DescriptorTable;
        PVOID DelayDescriptorTable;
    };
} PH_MAPPED_IMAGE_IMPORTS, *PPH_MAPPED_IMAGE_IMPORTS;

typedef struct _PH_MAPPED_IMAGE_IMPORT_DLL
{
    PPH_MAPPED_IMAGE MappedImage;
    ULONG Flags;
    PSTR Name;
    ULONG NumberOfEntries;

    union
    {
        PIMAGE_IMPORT_DESCRIPTOR Descriptor;
        PVOID DelayDescriptor;
    };
    PVOID *LookupTable;
} PH_MAPPED_IMAGE_IMPORT_DLL, *PPH_MAPPED_IMAGE_IMPORT_DLL;

typedef struct _PH_MAPPED_IMAGE_IMPORT_ENTRY
{
    PSTR Name;
    union
    {
        USHORT Ordinal;
        USHORT NameHint;
    };
} PH_MAPPED_IMAGE_IMPORT_ENTRY, *PPH_MAPPED_IMAGE_IMPORT_ENTRY;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageImports(
    _Out_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageImportDll(
    _In_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageImportEntry(
    _In_ PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_IMPORT_ENTRY Entry
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageDelayImports(
    _Out_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

USHORT
NTAPI
PhCheckSum(
    _In_ ULONG Sum,
    _In_reads_(Count) PUSHORT Buffer,
    _In_ ULONG Count
    );

PHLIBAPI
ULONG
NTAPI
PhCheckSumMappedImage(
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

// maplib

struct _PH_MAPPED_ARCHIVE;
typedef struct _PH_MAPPED_ARCHIVE *PPH_MAPPED_ARCHIVE;

typedef enum _PH_MAPPED_ARCHIVE_MEMBER_TYPE
{
    NormalArchiveMemberType,
    LinkerArchiveMemberType,
    LongnamesArchiveMemberType
} PH_MAPPED_ARCHIVE_MEMBER_TYPE;

typedef struct _PH_MAPPED_ARCHIVE_MEMBER
{
    PPH_MAPPED_ARCHIVE MappedArchive;
    PH_MAPPED_ARCHIVE_MEMBER_TYPE Type;
    PSTR Name;
    ULONG Size;
    PVOID Data;

    PIMAGE_ARCHIVE_MEMBER_HEADER Header;
    CHAR NameBuffer[20];
} PH_MAPPED_ARCHIVE_MEMBER, *PPH_MAPPED_ARCHIVE_MEMBER;

typedef struct _PH_MAPPED_ARCHIVE
{
    PVOID ViewBase;
    SIZE_T Size;

    PH_MAPPED_ARCHIVE_MEMBER FirstLinkerMember;
    PH_MAPPED_ARCHIVE_MEMBER SecondLinkerMember;
    PH_MAPPED_ARCHIVE_MEMBER LongnamesMember;
    BOOLEAN HasLongnamesMember;

    PPH_MAPPED_ARCHIVE_MEMBER FirstStandardMember;
    PPH_MAPPED_ARCHIVE_MEMBER LastStandardMember;
} PH_MAPPED_ARCHIVE, *PPH_MAPPED_ARCHIVE;

typedef struct _PH_MAPPED_ARCHIVE_IMPORT_ENTRY
{
    PSTR Name;
    PSTR DllName;
    union
    {
        USHORT Ordinal;
        USHORT NameHint;
    };
    BYTE Type;
    BYTE NameType;
    USHORT Machine;
} PH_MAPPED_ARCHIVE_IMPORT_ENTRY, *PPH_MAPPED_ARCHIVE_IMPORT_ENTRY;

PHLIBAPI
NTSTATUS
NTAPI
PhInitializeMappedArchive(
    _Out_ PPH_MAPPED_ARCHIVE MappedArchive,
    _In_ PVOID ViewBase,
    _In_ SIZE_T Size
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadMappedArchive(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _In_ BOOLEAN ReadOnly,
    _Out_ PPH_MAPPED_ARCHIVE MappedArchive
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUnloadMappedArchive(
    _Inout_ PPH_MAPPED_ARCHIVE MappedArchive
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetNextMappedArchiveMember(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member,
    _Out_ PPH_MAPPED_ARCHIVE_MEMBER NextMember
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsMappedArchiveMemberShortFormat(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedArchiveImportEntry(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member,
    _Out_ PPH_MAPPED_ARCHIVE_IMPORT_ENTRY Entry
    );

// iosup

extern PPH_OBJECT_TYPE PhFileStreamType;

BOOLEAN
NTAPI
PhIoSupportInitialization(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateFileWin32(
    _Out_ PHANDLE FileHandle,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateFileWin32Ex(
    _Out_ PHANDLE FileHandle,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG CreateStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryFullAttributesFileWin32(
    _In_ PWSTR FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteFileWin32(
    _In_ PWSTR FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhListenNamedPipe(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDisconnectNamedPipe(
    _In_ HANDLE FileHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhPeekNamedPipe(
    _In_ HANDLE FileHandle,
    _Out_writes_bytes_opt_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG NumberOfBytesRead,
    _Out_opt_ PULONG NumberOfBytesAvailable,
    _Out_opt_ PULONG NumberOfBytesLeftInMessage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhTransceiveNamedPipe(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWaitForNamedPipe(
    _In_opt_ PUNICODE_STRING FileSystemName,
    _In_ PUNICODE_STRING Name,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ BOOLEAN UseDefaultTimeout
    );

PHLIBAPI
NTSTATUS
NTAPI
PhImpersonateClientOfNamedPipe(
    _In_ HANDLE FileHandle
    );

// Core flags (PhCreateFileStream2)
/** Indicates that the file stream object should not close the file handle
 * upon deletion. */
#define PH_FILE_STREAM_HANDLE_UNOWNED 0x1
/** Indicates that the file stream object should not buffer I/O operations.
 * Note that this does not prevent the operating system from buffering I/O. */
#define PH_FILE_STREAM_UNBUFFERED 0x2
/** Indicates that the file handle supports asynchronous operations.
 * The file handle must not have been opened with FILE_SYNCHRONOUS_IO_ALERT
 * or FILE_SYNCHRONOUS_IO_NONALERT. */
#define PH_FILE_STREAM_ASYNCHRONOUS 0x4
/** Indicates that the file stream object should maintain the file position
 * and not use the file object's own file position. */
#define PH_FILE_STREAM_OWN_POSITION 0x8

// Higher-level flags (PhCreateFileStream)
#define PH_FILE_STREAM_APPEND 0x00010000

// Internal flags
/** Indicates that at least one write has been issued to the file handle. */
#define PH_FILE_STREAM_WRITTEN 0x80000000

// Seek
typedef enum _PH_SEEK_ORIGIN
{
    SeekStart,
    SeekCurrent,
    SeekEnd
} PH_SEEK_ORIGIN;

typedef struct _PH_FILE_STREAM
{
    HANDLE FileHandle;
    ULONG Flags;
    LARGE_INTEGER Position; // file object position, *not* the actual position

    PVOID Buffer;
    ULONG BufferLength;

    ULONG ReadPosition; // read position in buffer
    ULONG ReadLength; // how much available to read from buffer
    ULONG WritePosition; // write position in buffer
} PH_FILE_STREAM, *PPH_FILE_STREAM;

PHLIBAPI
NTSTATUS
NTAPI
PhCreateFileStream(
    _Out_ PPH_FILE_STREAM *FileStream,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareMode,
    _In_ ULONG CreateDisposition,
    _In_ ULONG Flags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateFileStream2(
    _Out_ PPH_FILE_STREAM *FileStream,
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags,
    _In_ ULONG BufferLength
    );

PHLIBAPI
VOID
NTAPI
PhVerifyFileStream(
    _In_ PPH_FILE_STREAM FileStream
    );

PHLIBAPI
NTSTATUS
NTAPI
PhReadFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReadLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFlushFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ BOOLEAN Full
    );

PHLIBAPI
VOID
NTAPI
PhGetPositionFileStream(
    _In_ PPH_FILE_STREAM FileStream,
    _Out_ PLARGE_INTEGER Position
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSeekFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Offset,
    _In_ PH_SEEK_ORIGIN Origin
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLockFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Position,
    _In_ PLARGE_INTEGER Length,
    _In_ BOOLEAN Wait,
    _In_ BOOLEAN Shared
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUnlockFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Position,
    _In_ PLARGE_INTEGER Length
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteStringAsUtf8FileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteStringAsUtf8FileStream2(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PWSTR String
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteStringAsUtf8FileStreamEx(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PWSTR Buffer,
    _In_ SIZE_T Length
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteStringFormatAsUtf8FileStream_V(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ _Printf_format_string_ PWSTR Format,
    _In_ va_list ArgPtr
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteStringFormatAsUtf8FileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ _Printf_format_string_ PWSTR Format,
    ...
    );

// provider

#if defined(DEBUG)
extern PPH_LIST PhDbgProviderList;
extern PH_QUEUED_LOCK PhDbgProviderListLock;
#endif

typedef enum _PH_PROVIDER_THREAD_STATE
{
    ProviderThreadRunning,
    ProviderThreadStopped,
    ProviderThreadStopping
} PH_PROVIDER_THREAD_STATE;

typedef VOID (NTAPI *PPH_PROVIDER_FUNCTION)(
    _In_ PVOID Object
    );

struct _PH_PROVIDER_THREAD;
typedef struct _PH_PROVIDER_THREAD *PPH_PROVIDER_THREAD;

typedef struct _PH_PROVIDER_REGISTRATION
{
    LIST_ENTRY ListEntry;
    PPH_PROVIDER_THREAD ProviderThread;
    PPH_PROVIDER_FUNCTION Function;
    PVOID Object;
    ULONG RunId;
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

    PH_QUEUED_LOCK Lock;
    LIST_ENTRY ListHead;
    ULONG BoostCount;
} PH_PROVIDER_THREAD, *PPH_PROVIDER_THREAD;

PHLIBAPI
VOID
NTAPI
PhInitializeProviderThread(
    _Out_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ ULONG Interval
    );

PHLIBAPI
VOID
NTAPI
PhDeleteProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID
NTAPI
PhStartProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID
NTAPI
PhStopProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID
NTAPI
PhSetIntervalProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ ULONG Interval
    );

PHLIBAPI
VOID
NTAPI
PhRegisterProvider(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ PPH_PROVIDER_FUNCTION Function,
    _In_opt_ PVOID Object,
    _Out_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
VOID
NTAPI
PhUnregisterProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
BOOLEAN
NTAPI
PhBoostProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration,
    _Out_opt_ PULONG FutureRunId
    );

PHLIBAPI
ULONG
NTAPI
PhGetRunIdProvider(
    _In_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetEnabledProvider(
    _In_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
VOID
NTAPI
PhSetEnabledProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration,
    _In_ BOOLEAN Enabled
    );

// svcsup

extern WCHAR *PhServiceTypeStrings[10];
extern WCHAR *PhServiceStartTypeStrings[5];
extern WCHAR *PhServiceErrorControlStrings[4];

PHLIBAPI
PVOID
NTAPI
PhEnumServices(
    _In_ SC_HANDLE ScManagerHandle,
    _In_opt_ ULONG Type,
    _In_opt_ ULONG State,
    _Out_ PULONG Count
    );

PHLIBAPI
SC_HANDLE
NTAPI
PhOpenService(
    _In_ PWSTR ServiceName,
    _In_ ACCESS_MASK DesiredAccess
    );

PHLIBAPI
PVOID
NTAPI
PhGetServiceConfig(
    _In_ SC_HANDLE ServiceHandle
    );

PHLIBAPI
PVOID
NTAPI
PhQueryServiceVariableSize(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG InfoLevel
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServiceDescription(
    _In_ SC_HANDLE ServiceHandle
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetServiceDelayedAutoStart(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ PBOOLEAN DelayedAutoStart
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSetServiceDelayedAutoStart(
    _In_ SC_HANDLE ServiceHandle,
    _In_ BOOLEAN DelayedAutoStart
    );

PHLIBAPI
PWSTR
NTAPI
PhGetServiceStateString(
    _In_ ULONG ServiceState
    );

PHLIBAPI
PWSTR
NTAPI
PhGetServiceTypeString(
    _In_ ULONG ServiceType
    );

PHLIBAPI
ULONG
NTAPI
PhGetServiceTypeInteger(
    _In_ PWSTR ServiceType
    );

PHLIBAPI
PWSTR
NTAPI
PhGetServiceStartTypeString(
    _In_ ULONG ServiceStartType
    );

PHLIBAPI
ULONG
NTAPI
PhGetServiceStartTypeInteger(
    _In_ PWSTR ServiceStartType
    );

PHLIBAPI
PWSTR
NTAPI
PhGetServiceErrorControlString(
    _In_ ULONG ServiceErrorControl
    );

PHLIBAPI
ULONG
NTAPI
PhGetServiceErrorControlInteger(
    _In_ PWSTR ServiceErrorControl
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetServiceNameFromTag(
    _In_ HANDLE ProcessId,
    _In_ PVOID ServiceTag
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadServiceTag(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle,
    _Out_ PVOID *ServiceTag
    );

NTSTATUS
NTAPI
PhGetServiceDllParameter(
    _In_ PPH_STRINGREF ServiceName,
    _Out_ PPH_STRING *ServiceDll
    );

// support

extern WCHAR *PhSizeUnitNames[7];
extern ULONG PhMaxSizeUnit;

typedef struct _PH_INTEGER_PAIR
{
    LONG X;
    LONG Y;
} PH_INTEGER_PAIR, *PPH_INTEGER_PAIR;

typedef struct _PH_RECTANGLE
{
    union
    {
        PH_INTEGER_PAIR Position;
        struct
        {
            LONG Left;
            LONG Top;
        };
    };
    union
    {
        PH_INTEGER_PAIR Size;
        struct
        {
            LONG Width;
            LONG Height;
        };
    };
} PH_RECTANGLE, *PPH_RECTANGLE;

FORCEINLINE
PH_RECTANGLE
PhRectToRectangle(
    _In_ RECT Rect
    )
{
    PH_RECTANGLE rectangle;

    rectangle.Left = Rect.left;
    rectangle.Top = Rect.top;
    rectangle.Width = Rect.right - Rect.left;
    rectangle.Height = Rect.bottom - Rect.top;

    return rectangle;
}

FORCEINLINE
RECT
PhRectangleToRect(
    _In_ PH_RECTANGLE Rectangle
    )
{
    RECT rect;

    rect.left = Rectangle.Left;
    rect.top = Rectangle.Top;
    rect.right = Rectangle.Left + Rectangle.Width;
    rect.bottom = Rectangle.Top + Rectangle.Height;

    return rect;
}

FORCEINLINE
VOID
PhConvertRect(
    _Inout_ PRECT Rect,
    _In_ PRECT ParentRect
    )
{
    Rect->right = ParentRect->right - ParentRect->left - Rect->right;
    Rect->bottom = ParentRect->bottom - ParentRect->top - Rect->bottom;
}

FORCEINLINE
RECT
PhMapRect(
    _In_ RECT InnerRect,
    _In_ RECT OuterRect
    )
{
    RECT rect;

    rect.left = InnerRect.left - OuterRect.left;
    rect.top = InnerRect.top - OuterRect.top;
    rect.right = InnerRect.right - OuterRect.left;
    rect.bottom = InnerRect.bottom - OuterRect.top;

    return rect;
}

PHLIBAPI
VOID
NTAPI
PhAdjustRectangleToBounds(
    _Inout_ PPH_RECTANGLE Rectangle,
    _In_ PPH_RECTANGLE Bounds
    );

PHLIBAPI
VOID
NTAPI
PhCenterRectangle(
    _Inout_ PPH_RECTANGLE Rectangle,
    _In_ PPH_RECTANGLE Bounds
    );

PHLIBAPI
VOID
NTAPI
PhAdjustRectangleToWorkingArea(
    _In_ HWND hWnd,
    _Inout_ PPH_RECTANGLE Rectangle
    );

PHLIBAPI
VOID
NTAPI
PhCenterWindow(
    _In_ HWND WindowHandle,
    _In_opt_ HWND ParentWindowHandle
    );

FORCEINLINE
VOID
PhLargeIntegerToSystemTime(
    _Out_ PSYSTEMTIME SystemTime,
    _In_ PLARGE_INTEGER LargeInteger
    )
{
    FILETIME fileTime;

    fileTime.dwLowDateTime = LargeInteger->LowPart;
    fileTime.dwHighDateTime = LargeInteger->HighPart;
    FileTimeToSystemTime(&fileTime, SystemTime);
}

FORCEINLINE
VOID
PhLargeIntegerToLocalSystemTime(
    _Out_ PSYSTEMTIME SystemTime,
    _In_ PLARGE_INTEGER LargeInteger
    )
{
    FILETIME fileTime;
    FILETIME newFileTime;

    fileTime.dwLowDateTime = LargeInteger->LowPart;
    fileTime.dwHighDateTime = LargeInteger->HighPart;
    FileTimeToLocalFileTime(&fileTime, &newFileTime);
    FileTimeToSystemTime(&newFileTime, SystemTime);
}

PHLIBAPI
VOID
NTAPI
PhReferenceObjects(
    _In_reads_(NumberOfObjects) PVOID *Objects,
    _In_ ULONG NumberOfObjects
    );

PHLIBAPI
VOID
NTAPI
PhDereferenceObjects(
    _In_reads_(NumberOfObjects) PVOID *Objects,
    _In_ ULONG NumberOfObjects
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetMessage(
    _In_ PVOID DllHandle,
    _In_ ULONG MessageTableId,
    _In_ ULONG MessageLanguageId,
    _In_ ULONG MessageId
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetNtMessage(
    _In_ NTSTATUS Status
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetWin32Message(
    _In_ ULONG Result
    );

PHLIBAPI
INT
NTAPI
PhShowMessage(
    _In_ HWND hWnd,
    _In_ ULONG Type,
    _In_ PWSTR Format,
    ...
    );

PHLIBAPI
INT
NTAPI
PhShowMessage_V(
    _In_ HWND hWnd,
    _In_ ULONG Type,
    _In_ PWSTR Format,
    _In_ va_list ArgPtr
    );

#define PhShowError(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONERROR, Format, __VA_ARGS__)
#define PhShowWarning(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONWARNING, Format, __VA_ARGS__)
#define PhShowInformation(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONINFORMATION, Format, __VA_ARGS__)

PPH_STRING
NTAPI
PhGetStatusMessage(
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    );

PHLIBAPI
VOID
NTAPI
PhShowStatus(
    _In_ HWND hWnd,
    _In_opt_ PWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    );

PHLIBAPI
BOOLEAN
NTAPI
PhShowContinueStatus(
    _In_ HWND hWnd,
    _In_opt_ PWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    );

PHLIBAPI
BOOLEAN
NTAPI
PhShowConfirmMessage(
    _In_ HWND hWnd,
    _In_ PWSTR Verb,
    _In_ PWSTR Object,
    _In_opt_ PWSTR Message,
    _In_ BOOLEAN Warning
    );

PHLIBAPI
BOOLEAN
NTAPI
PhFindIntegerSiKeyValuePairs(
    _In_ PPH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ PWSTR String,
    _Out_ PULONG Integer
    );

PHLIBAPI
BOOLEAN
NTAPI
PhFindStringSiKeyValuePairs(
    _In_ PPH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ ULONG Integer,
    _Out_ PWSTR *String
    );

#define GUID_VERSION_MAC 1
#define GUID_VERSION_DCE 2
#define GUID_VERSION_MD5 3
#define GUID_VERSION_RANDOM 4
#define GUID_VERSION_SHA1 5

#define GUID_VARIANT_NCS_MASK 0x80
#define GUID_VARIANT_NCS 0x00
#define GUID_VARIANT_STANDARD_MASK 0xc0
#define GUID_VARIANT_STANDARD 0x80
#define GUID_VARIANT_MICROSOFT_MASK 0xe0
#define GUID_VARIANT_MICROSOFT 0xc0
#define GUID_VARIANT_RESERVED_MASK 0xe0
#define GUID_VARIANT_RESERVED 0xe0

typedef union _GUID_EX
{
    GUID Guid;
    UCHAR Data[16];
    struct
    {
        ULONG TimeLowPart;
        USHORT TimeMidPart;
        USHORT TimeHighPart;
        UCHAR ClockSequenceHigh;
        UCHAR ClockSequenceLow;
        UCHAR Node[6];
    } s;
    struct
    {
        ULONG Part0;
        USHORT Part32;
        UCHAR Part48;
        UCHAR Part56 : 4;
        UCHAR Version : 4;
        UCHAR Variant;
        UCHAR Part72;
        USHORT Part80;
        ULONG Part96;
    } s2;
} GUID_EX, *PGUID_EX;

PHLIBAPI
VOID
NTAPI
PhGenerateGuid(
    _Out_ PGUID Guid
    );

PHLIBAPI
VOID
NTAPI
PhGenerateGuidFromName(
    _Out_ PGUID Guid,
    _In_ PGUID Namespace,
    _In_ PCHAR Name,
    _In_ ULONG NameLength,
    _In_ UCHAR Version
    );

PHLIBAPI
VOID
NTAPI
PhGenerateRandomAlphaString(
    _Out_writes_z_(Count) PWSTR Buffer,
    _In_ ULONG Count
    );

PHLIBAPI
PPH_STRING
NTAPI
PhEllipsisString(
    _In_ PPH_STRING String,
    _In_ ULONG DesiredCount
    );

PHLIBAPI
PPH_STRING
NTAPI
PhEllipsisStringPath(
    _In_ PPH_STRING String,
    _In_ ULONG DesiredCount
    );

PHLIBAPI
BOOLEAN
NTAPI
PhMatchWildcards(
    _In_ PWSTR Pattern,
    _In_ PWSTR String,
    _In_ BOOLEAN IgnoreCase
    );

PHLIBAPI
PPH_STRING
NTAPI
PhEscapeStringForMenuPrefix(
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
LONG
NTAPI
PhCompareUnicodeStringZIgnoreMenuPrefix(
    _In_ PWSTR A,
    _In_ PWSTR B,
    _In_ BOOLEAN IgnoreCase,
    _In_ BOOLEAN MatchIfPrefix
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatDate(
    _In_opt_ PSYSTEMTIME Date,
    _In_opt_ PWSTR Format
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatTime(
    _In_opt_ PSYSTEMTIME Time,
    _In_opt_ PWSTR Format
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatDateTime(
    _In_opt_ PSYSTEMTIME DateTime
    );

#define PhaFormatDateTime(DateTime) \
    ((PPH_STRING)PhAutoDereferenceObject(PhFormatDateTime(DateTime)))

PHLIBAPI
PPH_STRING
NTAPI
PhFormatTimeSpanRelative(
    _In_ ULONG64 TimeSpan
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatUInt64(
    _In_ ULONG64 Value,
    _In_ BOOLEAN GroupDigits
    );

#define PhaFormatUInt64(Value, GroupDigits) \
    ((PPH_STRING)PhAutoDereferenceObject(PhFormatUInt64((Value), (GroupDigits))))

PHLIBAPI
PPH_STRING
NTAPI
PhFormatDecimal(
    _In_ PWSTR Value,
    _In_ ULONG FractionalDigits,
    _In_ BOOLEAN GroupDigits
    );

#define PhaFormatDecimal(Value, FractionalDigits, GroupDigits) \
    ((PPH_STRING)PhAutoDereferenceObject(PhFormatDecimal((Value), (FractionalDigits), (GroupDigits))))

PHLIBAPI
PPH_STRING
NTAPI
PhFormatSize(
    _In_ ULONG64 Size,
    _In_ ULONG MaxSizeUnit
    );

#define PhaFormatSize(Size, MaxSizeUnit) \
    ((PPH_STRING)PhAutoDereferenceObject(PhFormatSize((Size), (MaxSizeUnit))))

PHLIBAPI
PPH_STRING
NTAPI
PhFormatGuid(
    _In_ PGUID Guid
    );

PHLIBAPI
PVOID
NTAPI
PhGetFileVersionInfo(
    _In_ PWSTR FileName
    );

PHLIBAPI
ULONG
NTAPI
PhGetFileVersionInfoLangCodePage(
    _In_ PVOID VersionInfo
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetFileVersionInfoString(
    _In_ PVOID VersionInfo,
    _In_ PWSTR SubBlock
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetFileVersionInfoString2(
    _In_ PVOID VersionInfo,
    _In_ ULONG LangCodePage,
    _In_ PWSTR StringName
    );

typedef struct _PH_IMAGE_VERSION_INFO
{
    PPH_STRING CompanyName;
    PPH_STRING FileDescription;
    PPH_STRING FileVersion;
    PPH_STRING ProductName;
} PH_IMAGE_VERSION_INFO, *PPH_IMAGE_VERSION_INFO;

PHLIBAPI
BOOLEAN
NTAPI
PhInitializeImageVersionInfo(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PWSTR FileName
    );

PHLIBAPI
VOID
NTAPI
PhDeleteImageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo
    );

PHLIBAPI
PPH_STRING
NTAPI
PhFormatImageVersionInfo(
    _In_opt_ PPH_STRING FileName,
    _In_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_opt_ PPH_STRINGREF Indent,
    _In_opt_ ULONG LineLimit
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetFullPath(
    _In_ PWSTR FileName,
    _Out_opt_ PULONG IndexOfFileName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhExpandEnvironmentStrings(
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetBaseName(
    _In_ PPH_STRING FileName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetSystemDirectory(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhGetSystemRoot(
    _Out_ PPH_STRINGREF SystemRoot
    );

PHLIBAPI
PLDR_DATA_TABLE_ENTRY
NTAPI
PhFindLoaderEntry(
    _In_opt_ PVOID DllBase,
    _In_opt_ PPH_STRINGREF FullDllName,
    _In_opt_ PPH_STRINGREF BaseDllName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetDllFileName(
    _In_ PVOID DllHandle,
    _Out_opt_ PULONG IndexOfFileName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetApplicationFileName(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetApplicationDirectory(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetKnownLocation(
    _In_ ULONG Folder,
    _In_opt_ PWSTR AppendPath
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWaitForMultipleObjectsAndPump(
    _In_opt_ HWND hWnd,
    _In_ ULONG NumberOfHandles,
    _In_ PHANDLE Handles,
    _In_ ULONG Timeout
    );

typedef struct _PH_CREATE_PROCESS_INFO
{
    PUNICODE_STRING DllPath;
    PUNICODE_STRING WindowTitle;
    PUNICODE_STRING DesktopInfo;
    PUNICODE_STRING ShellInfo;
    PUNICODE_STRING RuntimeData;
} PH_CREATE_PROCESS_INFO, *PPH_CREATE_PROCESS_INFO;

#define PH_CREATE_PROCESS_INHERIT_HANDLES 0x1
#define PH_CREATE_PROCESS_UNICODE_ENVIRONMENT 0x2
#define PH_CREATE_PROCESS_SUSPENDED 0x4
#define PH_CREATE_PROCESS_BREAKAWAY_FROM_JOB 0x8
#define PH_CREATE_PROCESS_NEW_CONSOLE 0x10

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcess(
    _In_ PWSTR FileName,
    _In_opt_ PPH_STRINGREF CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PPH_STRINGREF CurrentDirectory,
    _In_opt_ PPH_CREATE_PROCESS_INFO Information,
    _In_ ULONG Flags,
    _In_opt_ HANDLE ParentProcessHandle,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessWin32(
    _In_opt_ PWSTR FileName,
    _In_opt_ PWSTR CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PWSTR CurrentDirectory,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessWin32Ex(
    _In_opt_ PWSTR FileName,
    _In_opt_ PWSTR CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PWSTR CurrentDirectory,
    _In_opt_ STARTUPINFO *StartupInfo,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TokenHandle,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    );

typedef struct _PH_CREATE_PROCESS_AS_USER_INFO
{
    _In_opt_ PWSTR ApplicationName;
    _In_opt_ PWSTR CommandLine;
    _In_opt_ PWSTR CurrentDirectory;
    _In_opt_ PVOID Environment;
    _In_opt_ PWSTR DesktopName;
    _In_opt_ ULONG SessionId; // use PH_CREATE_PROCESS_SET_SESSION_ID
    union
    {
        struct
        {
            _In_ PWSTR DomainName;
            _In_ PWSTR UserName;
            _In_ PWSTR Password;
            _In_opt_ ULONG LogonType;
        };
        _In_ HANDLE ProcessIdWithToken; // use PH_CREATE_PROCESS_USE_PROCESS_TOKEN
        _In_ ULONG SessionIdWithToken; // use PH_CREATE_PROCESS_USE_SESSION_TOKEN
    };
} PH_CREATE_PROCESS_AS_USER_INFO, *PPH_CREATE_PROCESS_AS_USER_INFO;

#define PH_CREATE_PROCESS_USE_PROCESS_TOKEN 0x1000
#define PH_CREATE_PROCESS_USE_SESSION_TOKEN 0x2000
#define PH_CREATE_PROCESS_USE_LINKED_TOKEN 0x10000
#define PH_CREATE_PROCESS_SET_SESSION_ID 0x20000
#define PH_CREATE_PROCESS_WITH_PROFILE 0x40000

PHLIBAPI
NTSTATUS
NTAPI
PhCreateProcessAsUser(
    _In_ PPH_CREATE_PROCESS_AS_USER_INFO Information,
    _In_ ULONG Flags,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    );

NTSTATUS
NTAPI
PhFilterTokenForLimitedUser(
    _In_ HANDLE TokenHandle,
    _Out_ PHANDLE NewTokenHandle
    );

PHLIBAPI
VOID
NTAPI
PhShellExecute(
    _In_ HWND hWnd,
    _In_ PWSTR FileName,
    _In_opt_ PWSTR Parameters
    );

#define PH_SHELL_EXECUTE_ADMIN 0x1
#define PH_SHELL_EXECUTE_PUMP_MESSAGES 0x2

PHLIBAPI
BOOLEAN
NTAPI
PhShellExecuteEx(
    _In_opt_ HWND hWnd,
    _In_ PWSTR FileName,
    _In_opt_ PWSTR Parameters,
    _In_ ULONG ShowWindowType,
    _In_ ULONG Flags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    );

PHLIBAPI
VOID
NTAPI
PhShellExploreFile(
    _In_ HWND hWnd,
    _In_ PWSTR FileName
    );

PHLIBAPI
VOID
NTAPI
PhShellProperties(
    _In_ HWND hWnd,
    _In_ PWSTR FileName
    );

PPH_STRING
NTAPI
PhExpandKeyName(
    _In_ PPH_STRING KeyName,
    _In_ BOOLEAN Computer
    );

PHLIBAPI
VOID
NTAPI
PhShellOpenKey(
    _In_ HWND hWnd,
    _In_ PPH_STRING KeyName
    );

PKEY_VALUE_PARTIAL_INFORMATION
NTAPI
PhQueryRegistryValue(
    _In_ HANDLE KeyHandle,
    _In_opt_ PWSTR ValueName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhQueryRegistryString(
    _In_ HANDLE KeyHandle,
    _In_opt_ PWSTR ValueName
    );

typedef struct _PH_FLAG_MAPPING
{
    ULONG Flag1;
    ULONG Flag2;
} PH_FLAG_MAPPING, *PPH_FLAG_MAPPING;

PHLIBAPI
VOID
NTAPI
PhMapFlags1(
    _Inout_ PULONG Value2,
    _In_ ULONG Value1,
    _In_ const PH_FLAG_MAPPING *Mappings,
    _In_ ULONG NumberOfMappings
    );

PHLIBAPI
VOID
NTAPI
PhMapFlags2(
    _Inout_ PULONG Value1,
    _In_ ULONG Value2,
    _In_ const PH_FLAG_MAPPING *Mappings,
    _In_ ULONG NumberOfMappings
    );

PHLIBAPI
PVOID
NTAPI
PhCreateOpenFileDialog(
    VOID
    );

PHLIBAPI
PVOID
NTAPI
PhCreateSaveFileDialog(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhFreeFileDialog(
    _In_ PVOID FileDialog
    );

PHLIBAPI
BOOLEAN
NTAPI
PhShowFileDialog(
    _In_ HWND hWnd,
    _In_ PVOID FileDialog
    );

#define PH_FILEDIALOG_CREATEPROMPT 0x1
#define PH_FILEDIALOG_PATHMUSTEXIST 0x2 // default both
#define PH_FILEDIALOG_FILEMUSTEXIST 0x4 // default open
#define PH_FILEDIALOG_SHOWHIDDEN 0x8
#define PH_FILEDIALOG_NODEREFERENCELINKS 0x10
#define PH_FILEDIALOG_OVERWRITEPROMPT 0x20 // default save
#define PH_FILEDIALOG_DEFAULTEXPANDED 0x40
#define PH_FILEDIALOG_STRICTFILETYPES 0x80
#define PH_FILEDIALOG_PICKFOLDERS 0x100

PHLIBAPI
ULONG
NTAPI
PhGetFileDialogOptions(
    _In_ PVOID FileDialog
    );

PHLIBAPI
VOID
NTAPI
PhSetFileDialogOptions(
    _In_ PVOID FileDialog,
    _In_ ULONG Options
    );

PHLIBAPI
ULONG
NTAPI
PhGetFileDialogFilterIndex(
    _In_ PVOID FileDialog
    );

typedef struct _PH_FILETYPE_FILTER
{
    PWSTR Name;
    PWSTR Filter;
} PH_FILETYPE_FILTER, *PPH_FILETYPE_FILTER;

PHLIBAPI
VOID
NTAPI
PhSetFileDialogFilter(
    _In_ PVOID FileDialog,
    _In_ PPH_FILETYPE_FILTER Filters,
    _In_ ULONG NumberOfFilters
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetFileDialogFileName(
    _In_ PVOID FileDialog
    );

PHLIBAPI
VOID
NTAPI
PhSetFileDialogFileName(
    _In_ PVOID FileDialog,
    _In_ PWSTR FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhIsExecutablePacked(
    _In_ PWSTR FileName,
    _Out_ PBOOLEAN IsPacked,
    _Out_opt_ PULONG NumberOfModules,
    _Out_opt_ PULONG NumberOfFunctions
    );

ULONG
NTAPI
PhCrc32(
    _In_ ULONG Crc,
    _In_reads_(Length) PCHAR Buffer,
    _In_ SIZE_T Length
    );

typedef enum _PH_HASH_ALGORITHM
{
    Md5HashAlgorithm,
    Sha1HashAlgorithm,
    Crc32HashAlgorithm
} PH_HASH_ALGORITHM;

typedef struct _PH_HASH_CONTEXT
{
    PH_HASH_ALGORITHM Algorithm;
    ULONG Context[64];
} PH_HASH_CONTEXT, *PPH_HASH_CONTEXT;

PHLIBAPI
VOID
NTAPI
PhInitializeHash(
    _Out_ PPH_HASH_CONTEXT Context,
    _In_ PH_HASH_ALGORITHM Algorithm
    );

PHLIBAPI
VOID
NTAPI
PhUpdateHash(
    _Inout_ PPH_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

PHLIBAPI
BOOLEAN
NTAPI
PhFinalHash(
    _Inout_ PPH_HASH_CONTEXT Context,
    _Out_writes_bytes_(HashLength) PVOID Hash,
    _In_ ULONG HashLength,
    _Out_opt_ PULONG ReturnLength
    );

typedef enum _PH_COMMAND_LINE_OPTION_TYPE
{
    NoArgumentType,
    MandatoryArgumentType,
    OptionalArgumentType
} PH_COMMAND_LINE_OPTION_TYPE, *PPH_COMMAND_LINE_OPTION_TYPE;

typedef struct _PH_COMMAND_LINE_OPTION
{
    ULONG Id;
    PWSTR Name;
    PH_COMMAND_LINE_OPTION_TYPE Type;
} PH_COMMAND_LINE_OPTION, *PPH_COMMAND_LINE_OPTION;

typedef BOOLEAN (NTAPI *PPH_COMMAND_LINE_CALLBACK)(
    _In_opt_ PPH_COMMAND_LINE_OPTION Option,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PVOID Context
    );

#define PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS 0x1
#define PH_COMMAND_LINE_IGNORE_FIRST_PART 0x2

PHLIBAPI
PPH_STRING
NTAPI
PhParseCommandLinePart(
    _In_ PPH_STRINGREF CommandLine,
    _Inout_ PULONG_PTR Index
    );

PHLIBAPI
BOOLEAN
NTAPI
PhParseCommandLine(
    _In_ PPH_STRINGREF CommandLine,
    _In_opt_ PPH_COMMAND_LINE_OPTION Options,
    _In_ ULONG NumberOfOptions,
    _In_ ULONG Flags,
    _In_ PPH_COMMAND_LINE_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
PPH_STRING
NTAPI
PhEscapeCommandLinePart(
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
BOOLEAN
NTAPI
PhParseCommandLineFuzzy(
    _In_ PPH_STRINGREF CommandLine,
    _Out_ PPH_STRINGREF FileName,
    _Out_ PPH_STRINGREF Arguments,
    _Out_opt_ PPH_STRING *FullFileName
    );

#ifdef __cplusplus
}
#endif

#endif
