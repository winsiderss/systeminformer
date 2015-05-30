#ifndef _PH_PH_H
#define _PH_PH_H

#pragma once

#include <phbase.h>
#include <stdarg.h>
#include <dltmgr.h>
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

PHLIBAPI
NTSTATUS PhOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId
    );

PHLIBAPI
NTSTATUS PhOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadId
    );

PHLIBAPI
NTSTATUS PhOpenThreadProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS PhOpenProcessToken(
    _Out_ PHANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS PhOpenThreadToken(
    _Out_ PHANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadHandle,
    _In_ BOOLEAN OpenAsSelf
    );

PHLIBAPI
NTSTATUS PhGetObjectSecurity(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

PHLIBAPI
NTSTATUS PhSetObjectSecurity(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

PHLIBAPI
NTSTATUS PhTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS PhSuspendProcess(
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS PhResumeProcess(
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS PhTerminateThread(
    _In_ HANDLE ThreadHandle,
    _In_ NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS PhSuspendThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG PreviousSuspendCount
    );

PHLIBAPI
NTSTATUS PhResumeThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG PreviousSuspendCount
    );

PHLIBAPI
NTSTATUS PhGetThreadContext(
    _In_ HANDLE ThreadHandle,
    _Inout_ PCONTEXT Context
    );

PHLIBAPI
NTSTATUS PhSetThreadContext(
    _In_ HANDLE ThreadHandle,
    _In_ PCONTEXT Context
    );

PHLIBAPI
NTSTATUS PhReadVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead
    );

PHLIBAPI
NTSTATUS PhWriteVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesWritten
    );

PHLIBAPI
NTSTATUS PhGetProcessImageFileName(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *FileName
    );

PHLIBAPI
NTSTATUS PhGetProcessImageFileNameWin32(
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
NTSTATUS PhGetProcessPebString(
    _In_ HANDLE ProcessHandle,
    _In_ PH_PEB_OFFSET Offset,
    _Out_ PPH_STRING *String
    );

PHLIBAPI
NTSTATUS PhGetProcessCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *CommandLine
    );

PHLIBAPI
NTSTATUS PhGetProcessWindowTitle(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG WindowFlags,
    _Out_ PPH_STRING *WindowTitle
    );

PHLIBAPI
NTSTATUS PhGetProcessIsPosix(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsPosix
    );

PHLIBAPI
NTSTATUS PhGetProcessExecuteFlags(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ExecuteFlags
    );

#define PH_PROCESS_DEP_ENABLED 0x1
#define PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED 0x2
#define PH_PROCESS_DEP_PERMANENT 0x4

PHLIBAPI
NTSTATUS PhGetProcessDepStatus(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG DepStatus
    );

PHLIBAPI
NTSTATUS PhGetProcessPosixCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *CommandLine
    );

#define PH_GET_PROCESS_ENVIRONMENT_WOW64 0x1 // retrieve the WOW64 environment

PHLIBAPI
NTSTATUS PhGetProcessEnvironment(
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
BOOLEAN PhEnumProcessEnvironmentVariables(
    _In_ PVOID Environment,
    _In_ ULONG EnvironmentLength,
    _Inout_ PULONG EnumerationKey,
    _Out_ PPH_ENVIRONMENT_VARIABLE Variable
    );

PHLIBAPI
NTSTATUS PhGetProcessMappedFileName(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_ PPH_STRING *FileName
    );

PHLIBAPI
NTSTATUS PhGetProcessWorkingSetInformation(
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
NTSTATUS PhGetProcessWsCounters(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_WS_COUNTERS WsCounters
    );

typedef struct _KPH_PROCESS_HANDLE_INFORMATION *PKPH_PROCESS_HANDLE_INFORMATION;

PHLIBAPI
NTSTATUS PhEnumProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_ PKPH_PROCESS_HANDLE_INFORMATION *Handles
    );

PHLIBAPI
NTSTATUS PhSetProcessAffinityMask(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG_PTR AffinityMask
    );

PHLIBAPI
NTSTATUS PhSetProcessIoPriority(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG IoPriority
    );

PHLIBAPI
NTSTATUS PhSetProcessExecuteFlags(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG ExecuteFlags
    );

PHLIBAPI
NTSTATUS PhSetProcessDepStatus(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG DepStatus
    );

PHLIBAPI
NTSTATUS PhSetProcessDepStatusInvasive(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG DepStatus,
    _In_opt_ PLARGE_INTEGER Timeout
    );

PHLIBAPI
NTSTATUS PhInjectDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PWSTR FileName,
    _In_opt_ PLARGE_INTEGER Timeout
    );

PHLIBAPI
NTSTATUS PhUnloadDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_opt_ PLARGE_INTEGER Timeout
    );

PHLIBAPI
NTSTATUS PhSetThreadAffinityMask(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG_PTR AffinityMask
    );

PHLIBAPI
NTSTATUS PhSetThreadIoPriority(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG IoPriority
    );

/** Contains information about a thread stack frame. */
typedef struct _PH_THREAD_STACK_FRAME
{
    PVOID PcAddress;
    PVOID ReturnAddress;
    PVOID FrameAddress;
    PVOID StackAddress;
    PVOID BStoreAddress;
    PVOID Params[4];
} PH_THREAD_STACK_FRAME, *PPH_THREAD_STACK_FRAME;

#define PH_WALK_I386_STACK 0x1
#define PH_WALK_AMD64_STACK 0x2
#define PH_WALK_KERNEL_STACK 0x10

/**
 * A callback function passed to PhWalkThreadStack()
 * and called for each stack frame.
 *
 * \param StackFrame A structure providing information about
 * the stack frame.
 * \param Context A user-defined value passed to
 * PhWalkThreadStack().
 *
 * \return TRUE to continue the stack walk, FALSE to
 * stop.
 */
typedef BOOLEAN (NTAPI *PPH_WALK_THREAD_STACK_CALLBACK)(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS PhWalkThreadStack(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle,
    _In_opt_ PCLIENT_ID ClientId,
    _In_ ULONG Flags,
    _In_ PPH_WALK_THREAD_STACK_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS PhGetJobProcessIdList(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_PROCESS_ID_LIST *ProcessIdList
    );

NTSTATUS PhQueryTokenVariableSize(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_ PVOID *Buffer
    );

PHLIBAPI
NTSTATUS PhGetTokenUser(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_USER *User
    );

PHLIBAPI
NTSTATUS PhGetTokenOwner(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_OWNER *Owner
    );

PHLIBAPI
NTSTATUS PhGetTokenPrimaryGroup(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_PRIMARY_GROUP *PrimaryGroup
    );

PHLIBAPI
NTSTATUS PhGetTokenGroups(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_GROUPS *Groups
    );

PHLIBAPI
NTSTATUS PhGetTokenPrivileges(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_PRIVILEGES *Privileges
    );

PHLIBAPI
NTSTATUS PhSetTokenSessionId(
    _In_ HANDLE TokenHandle,
    _In_ ULONG SessionId
    );

PHLIBAPI
BOOLEAN PhSetTokenPrivilege(
    _In_ HANDLE TokenHandle,
    _In_opt_ PWSTR PrivilegeName,
    _In_opt_ PLUID PrivilegeLuid,
    _In_ ULONG Attributes
    );

PHLIBAPI
BOOLEAN PhSetTokenPrivilege2(
    _In_ HANDLE TokenHandle,
    _In_ LONG Privilege,
    _In_ ULONG Attributes
    );

PHLIBAPI
NTSTATUS PhSetTokenIsVirtualizationEnabled(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN IsVirtualizationEnabled
    );

PHLIBAPI
NTSTATUS PhGetTokenIntegrityLevel(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PMANDATORY_LEVEL IntegrityLevel,
    _Out_opt_ PWSTR *IntegrityString
    );

PHLIBAPI
NTSTATUS PhGetFileSize(
    _In_ HANDLE FileHandle,
    _Out_ PLARGE_INTEGER Size
    );

PHLIBAPI
NTSTATUS PhSetFileSize(
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER Size
    );

PHLIBAPI
NTSTATUS PhGetTransactionManagerBasicInformation(
    _In_ HANDLE TransactionManagerHandle,
    _Out_ PTRANSACTIONMANAGER_BASIC_INFORMATION BasicInformation
    );

PHLIBAPI
NTSTATUS PhGetTransactionManagerLogFileName(
    _In_ HANDLE TransactionManagerHandle,
    _Out_ PPH_STRING *LogFileName
    );

PHLIBAPI
NTSTATUS PhGetTransactionBasicInformation(
    _In_ HANDLE TransactionHandle,
    _Out_ PTRANSACTION_BASIC_INFORMATION BasicInformation
    );

PHLIBAPI
NTSTATUS PhGetTransactionPropertiesInformation(
    _In_ HANDLE TransactionHandle,
    _Out_opt_ PLARGE_INTEGER Timeout,
    _Out_opt_ TRANSACTION_OUTCOME *Outcome,
    _Out_opt_ PPH_STRING *Description
    );

PHLIBAPI
NTSTATUS PhGetResourceManagerBasicInformation(
    _In_ HANDLE ResourceManagerHandle,
    _Out_opt_ PGUID Guid,
    _Out_opt_ PPH_STRING *Description
    );

PHLIBAPI
NTSTATUS PhGetEnlistmentBasicInformation(
    _In_ HANDLE EnlistmentHandle,
    _Out_ PENLISTMENT_BASIC_INFORMATION BasicInformation
    );

PHLIBAPI
NTSTATUS PhOpenDriverByBaseAddress(
    _Out_ PHANDLE DriverHandle,
    _In_ PVOID BaseAddress
    );

PHLIBAPI
NTSTATUS PhGetDriverServiceKeyName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *ServiceKeyName
    );

PHLIBAPI
NTSTATUS PhUnloadDriver(
    _In_opt_ PVOID BaseAddress,
    _In_opt_ PWSTR Name
    );

PHLIBAPI
NTSTATUS PhDuplicateObject(
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
NTSTATUS PhEnumProcessModules(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS PhEnumProcessModulesEx(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_PARAMETERS Parameters
    );

PHLIBAPI
NTSTATUS PhSetProcessModuleLoadCount(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG LoadCount
    );

PHLIBAPI
NTSTATUS PhEnumProcessModules32(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS PhEnumProcessModules32Ex(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_PARAMETERS Parameters
    );

PHLIBAPI
NTSTATUS PhSetProcessModuleLoadCount32(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG LoadCount
    );

PHLIBAPI
NTSTATUS PhGetProcedureAddressRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PWSTR FileName,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID *ProcedureAddress,
    _Out_opt_ PVOID *DllBase
    );

PHLIBAPI
NTSTATUS PhEnumKernelModules(
    _Out_ PRTL_PROCESS_MODULES *Modules
    );

NTSTATUS PhEnumKernelModulesEx(
    _Out_ PRTL_PROCESS_MODULE_INFORMATION_EX *Modules
    );

PHLIBAPI
PPH_STRING PhGetKernelFileName(
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
NTSTATUS PhEnumProcesses(
    _Out_ PVOID *Processes
    );

PHLIBAPI
NTSTATUS PhEnumProcessesEx(
    _Out_ PVOID *Processes,
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass
    );

PHLIBAPI
NTSTATUS PhEnumProcessesForSession(
    _Out_ PVOID *Processes,
    _In_ ULONG SessionId
    );

PHLIBAPI
PSYSTEM_PROCESS_INFORMATION PhFindProcessInformation(
    _In_ PVOID Processes,
    _In_ HANDLE ProcessId
    );

PHLIBAPI
PSYSTEM_PROCESS_INFORMATION PhFindProcessInformationByImageName(
    _In_ PVOID Processes,
    _In_ PPH_STRINGREF ImageName
    );

PHLIBAPI
NTSTATUS PhEnumHandles(
    _Out_ PSYSTEM_HANDLE_INFORMATION *Handles
    );

PHLIBAPI
NTSTATUS PhEnumHandlesEx(
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
NTSTATUS PhEnumPagefiles(
    _Out_ PVOID *Pagefiles
    );

PHLIBAPI
NTSTATUS PhGetProcessImageFileNameByProcessId(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING *FileName
    );

PHLIBAPI
NTSTATUS PhGetProcessIsDotNet(
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
NTSTATUS PhGetProcessIsDotNetEx(
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
    _In_ PPH_STRING Name,
    _In_ PPH_STRING TypeName,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS PhEnumDirectoryObjects(
    _In_ HANDLE DirectoryHandle,
    _In_ PPH_ENUM_DIRECTORY_OBJECTS Callback,
    _In_opt_ PVOID Context
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_DIRECTORY_FILE)(
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS PhEnumDirectoryFile(
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
NTSTATUS PhEnumFileStreams(
    _In_ HANDLE FileHandle,
    _Out_ PVOID *Streams
    );

VOID PhInitializeDevicePrefixes(
    VOID
    );

PHLIBAPI
VOID PhUpdateMupDevicePrefixes(
    VOID
    );

PHLIBAPI
VOID PhUpdateDosDevicePrefixes(
    VOID
    );

PHLIBAPI
PPH_STRING PhResolveDevicePrefix(
    _In_ PPH_STRING Name
    );

PHLIBAPI
PPH_STRING PhGetFileName(
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
NTSTATUS PhEnumGenericModules(
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
NTSTATUS PhCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName,
    _In_ ULONG Attributes,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
    );

PHLIBAPI
NTSTATUS PhOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName,
    _In_ ULONG Attributes
    );

// lsa

PHLIBAPI
NTSTATUS PhOpenLsaPolicy(
    _Out_ PLSA_HANDLE PolicyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PUNICODE_STRING SystemName
    );

LSA_HANDLE PhGetLookupPolicyHandle(
    VOID
    );

PHLIBAPI
BOOLEAN PhLookupPrivilegeName(
    _In_ PLUID PrivilegeValue,
    _Out_ PPH_STRING *PrivilegeName
    );

PHLIBAPI
BOOLEAN PhLookupPrivilegeDisplayName(
    _In_ PPH_STRINGREF PrivilegeName,
    _Out_ PPH_STRING *PrivilegeDisplayName
    );

PHLIBAPI
BOOLEAN PhLookupPrivilegeValue(
    _In_ PPH_STRINGREF PrivilegeName,
    _Out_ PLUID PrivilegeValue
    );

PHLIBAPI
NTSTATUS PhLookupSid(
    _In_ PSID Sid,
    _Out_opt_ PPH_STRING *Name,
    _Out_opt_ PPH_STRING *DomainName,
    _Out_opt_ PSID_NAME_USE NameUse
    );

PHLIBAPI
NTSTATUS PhLookupName(
    _In_ PPH_STRINGREF Name,
    _Out_opt_ PSID *Sid,
    _Out_opt_ PPH_STRING *DomainName,
    _Out_opt_ PSID_NAME_USE NameUse
    );

PHLIBAPI
PPH_STRING PhGetSidFullName(
    _In_ PSID Sid,
    _In_ BOOLEAN IncludeDomain,
    _Out_opt_ PSID_NAME_USE NameUse
    );

PHLIBAPI
PPH_STRING PhSidToStringSid(
    _In_ PSID Sid
    );

// hndlinfo

#define MAX_OBJECT_TYPE_NUMBER 257

VOID PhHandleInfoInitialization(
    VOID
    );

typedef PPH_STRING (NTAPI *PPH_GET_CLIENT_ID_NAME)(
    _In_ PCLIENT_ID ClientId
    );

PPH_GET_CLIENT_ID_NAME PhSetHandleClientIdFunction(
    _In_ PPH_GET_CLIENT_ID_NAME GetClientIdName
    );

PHLIBAPI
PPH_STRING PhFormatNativeKeyName(
    _In_ PPH_STRING Name
    );

NTSTATUS PhGetSectionFileName(
    _In_ HANDLE SectionHandle,
    _Out_ PPH_STRING *FileName
    );

PHLIBAPI
_Callback_ PPH_STRING PhStdGetClientIdName(
    _In_ PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS PhGetHandleInformation(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ ULONG ObjectTypeNumber,
    _Out_opt_ POBJECT_BASIC_INFORMATION BasicInformation,
    _Out_opt_ PPH_STRING *TypeName,
    _Out_opt_ PPH_STRING *ObjectName,
    _Out_opt_ PPH_STRING *BestObjectName
    );

PHLIBAPI
NTSTATUS PhGetHandleInformationEx(
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

NTSTATUS PhEnumObjectTypes(
    _Out_ POBJECT_TYPES_INFORMATION *ObjectTypes
    );

ULONG PhGetObjectTypeNumber(
    _In_ PUNICODE_STRING TypeName
    );

NTSTATUS PhCallWithTimeout(
    _In_ PUSER_THREAD_START_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_opt_ PLARGE_INTEGER AcquireTimeout,
    _In_ PLARGE_INTEGER CallTimeout
    );

NTSTATUS PhCallNtQueryObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSTATUS PhCallNtQuerySecurityObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_writes_bytes_opt_(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Length,
    _Out_ PULONG LengthNeeded
    );

NTSTATUS PhCallNtSetSecurityObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS PhCallKphDuplicateObjectWithTimeout(
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
NTSTATUS PhInitializeMappedImage(
    _Out_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID ViewBase,
    _In_ SIZE_T Size
    );

PHLIBAPI
NTSTATUS PhLoadMappedImage(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _In_ BOOLEAN ReadOnly,
    _Out_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS PhUnloadMappedImage(
    _Inout_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS PhMapViewOfEntireFile(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _In_ BOOLEAN ReadOnly,
    _Out_ PVOID *ViewBase,
    _Out_ PSIZE_T Size
    );

PHLIBAPI
PIMAGE_SECTION_HEADER PhMappedImageRvaToSection(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva
    );

PHLIBAPI
PVOID PhMappedImageRvaToVa(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva,
    _Out_opt_ PIMAGE_SECTION_HEADER *Section
    );

PHLIBAPI
BOOLEAN PhGetMappedImageSectionName(
    _In_ PIMAGE_SECTION_HEADER Section,
    _Out_writes_opt_z_(Count) PSTR Buffer,
    _In_ ULONG Count,
    _Out_opt_ PULONG ReturnCount
    );

PHLIBAPI
NTSTATUS PhGetMappedImageDataEntry(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Index,
    _Out_ PIMAGE_DATA_DIRECTORY *Entry
    );

PHLIBAPI
NTSTATUS PhGetMappedImageLoadConfig32(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PIMAGE_LOAD_CONFIG_DIRECTORY32 *LoadConfig
    );

PHLIBAPI
NTSTATUS PhGetMappedImageLoadConfig64(
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

NTSTATUS PhLoadRemoteMappedImage(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ViewBase,
    _Out_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    );

NTSTATUS PhUnloadRemoteMappedImage(
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
NTSTATUS PhGetMappedImageExports(
    _Out_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS PhGetMappedImageExportEntry(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_EXPORT_ENTRY Entry
    );

PHLIBAPI
NTSTATUS PhGetMappedImageExportFunction(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_opt_ PSTR Name,
    _In_opt_ USHORT Ordinal,
    _Out_ PPH_MAPPED_IMAGE_EXPORT_FUNCTION Function
    );

PHLIBAPI
NTSTATUS PhGetMappedImageExportFunctionRemote(
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
NTSTATUS PhGetMappedImageImports(
    _Out_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS PhGetMappedImageImportDll(
    _In_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll
    );

PHLIBAPI
NTSTATUS PhGetMappedImageImportEntry(
    _In_ PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_IMPORT_ENTRY Entry
    );

PHLIBAPI
NTSTATUS PhGetMappedImageDelayImports(
    _Out_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
ULONG PhCheckSumMappedImage(
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
NTSTATUS PhInitializeMappedArchive(
    _Out_ PPH_MAPPED_ARCHIVE MappedArchive,
    _In_ PVOID ViewBase,
    _In_ SIZE_T Size
    );

PHLIBAPI
NTSTATUS PhLoadMappedArchive(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _In_ BOOLEAN ReadOnly,
    _Out_ PPH_MAPPED_ARCHIVE MappedArchive
    );

PHLIBAPI
NTSTATUS PhUnloadMappedArchive(
    _Inout_ PPH_MAPPED_ARCHIVE MappedArchive
    );

PHLIBAPI
NTSTATUS PhGetNextMappedArchiveMember(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member,
    _Out_ PPH_MAPPED_ARCHIVE_MEMBER NextMember
    );

PHLIBAPI
BOOLEAN PhIsMappedArchiveMemberShortFormat(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member
    );

PHLIBAPI
NTSTATUS PhGetMappedArchiveImportEntry(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member,
    _Out_ PPH_MAPPED_ARCHIVE_IMPORT_ENTRY Entry
    );

// iosup

extern PPH_OBJECT_TYPE PhFileStreamType;

BOOLEAN PhIoSupportInitialization(
    VOID
    );

PHLIBAPI
NTSTATUS PhCreateFileWin32(
    _Out_ PHANDLE FileHandle,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions
    );

PHLIBAPI
NTSTATUS PhCreateFileWin32Ex(
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
NTSTATUS PhQueryFullAttributesFileWin32(
    _In_ PWSTR FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

PHLIBAPI
NTSTATUS PhDeleteFileWin32(
    _In_ PWSTR FileName
    );

PHLIBAPI
NTSTATUS PhListenNamedPipe(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );

PHLIBAPI
NTSTATUS PhDisconnectNamedPipe(
    _In_ HANDLE FileHandle
    );

PHLIBAPI
NTSTATUS PhPeekNamedPipe(
    _In_ HANDLE FileHandle,
    _Out_writes_bytes_opt_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG NumberOfBytesRead,
    _Out_opt_ PULONG NumberOfBytesAvailable,
    _Out_opt_ PULONG NumberOfBytesLeftInMessage
    );

PHLIBAPI
NTSTATUS PhTransceiveNamedPipe(
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
NTSTATUS PhWaitForNamedPipe(
    _In_opt_ PUNICODE_STRING FileSystemName,
    _In_ PUNICODE_STRING Name,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ BOOLEAN UseDefaultTimeout
    );

PHLIBAPI
NTSTATUS PhImpersonateClientOfNamedPipe(
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
NTSTATUS PhCreateFileStream(
    _Out_ PPH_FILE_STREAM *FileStream,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareMode,
    _In_ ULONG CreateDisposition,
    _In_ ULONG Flags
    );

PHLIBAPI
NTSTATUS PhCreateFileStream2(
    _Out_ PPH_FILE_STREAM *FileStream,
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags,
    _In_ ULONG BufferLength
    );

PHLIBAPI
VOID PhVerifyFileStream(
    _In_ PPH_FILE_STREAM FileStream
    );

PHLIBAPI
NTSTATUS PhReadFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReadLength
    );

PHLIBAPI
NTSTATUS PhWriteFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

PHLIBAPI
NTSTATUS PhFlushFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ BOOLEAN Full
    );

PHLIBAPI
VOID PhGetPositionFileStream(
    _In_ PPH_FILE_STREAM FileStream,
    _Out_ PLARGE_INTEGER Position
    );

PHLIBAPI
NTSTATUS PhSeekFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Offset,
    _In_ PH_SEEK_ORIGIN Origin
    );

PHLIBAPI
NTSTATUS PhLockFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Position,
    _In_ PLARGE_INTEGER Length,
    _In_ BOOLEAN Wait,
    _In_ BOOLEAN Shared
    );

PHLIBAPI
NTSTATUS PhUnlockFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Position,
    _In_ PLARGE_INTEGER Length
    );

PHLIBAPI
NTSTATUS PhWriteStringAsUtf8FileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
NTSTATUS PhWriteStringAsUtf8FileStream2(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PWSTR String
    );

PHLIBAPI
NTSTATUS PhWriteStringAsUtf8FileStreamEx(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PWSTR Buffer,
    _In_ SIZE_T Length
    );

PHLIBAPI
NTSTATUS PhWriteStringFormatAsUtf8FileStream_V(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ _Printf_format_string_ PWSTR Format,
    _In_ va_list ArgPtr
    );

PHLIBAPI
NTSTATUS PhWriteStringFormatAsUtf8FileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ _Printf_format_string_ PWSTR Format,
    ...
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
    VrSecuritySettings,
    VrBadSignature
} VERIFY_RESULT, *PVERIFY_RESULT;

PHLIBAPI
VERIFY_RESULT PhVerifyFile(
    _In_ PWSTR FileName,
    _Out_opt_ PPH_STRING *SignerName
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
VOID PhInitializeProviderThread(
    _Out_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ ULONG Interval
    );

PHLIBAPI
VOID PhDeleteProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID PhStartProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID PhStopProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID PhSetIntervalProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ ULONG Interval
    );

PHLIBAPI
VOID PhRegisterProvider(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ PPH_PROVIDER_FUNCTION Function,
    _In_opt_ PVOID Object,
    _Out_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
VOID PhUnregisterProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
BOOLEAN PhBoostProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration,
    _Out_opt_ PULONG FutureRunId
    );

PHLIBAPI
ULONG PhGetRunIdProvider(
    _In_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
BOOLEAN PhGetEnabledProvider(
    _In_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
VOID PhSetEnabledProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration,
    _In_ BOOLEAN Enabled
    );

// symprv

extern PPH_OBJECT_TYPE PhSymbolProviderType;

//#define PH_SYMBOL_PROVIDER_DELAY_INIT

#define PH_MAX_SYMBOL_NAME_LEN 128

typedef struct _PH_SYMBOL_PROVIDER
{
    LIST_ENTRY ModulesListHead;
    PH_QUEUED_LOCK ModulesListLock;
    HANDLE ProcessHandle;
    BOOLEAN IsRealHandle;
    BOOLEAN IsRegistered;
#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    PH_INITONCE InitOnce;
#endif
    PH_AVL_TREE ModulesSet;
    PH_CALLBACK EventCallback;
} PH_SYMBOL_PROVIDER, *PPH_SYMBOL_PROVIDER;

typedef enum _PH_SYMBOL_RESOLVE_LEVEL
{
    PhsrlFunction,
    PhsrlModule,
    PhsrlAddress,
    PhsrlInvalid
} PH_SYMBOL_RESOLVE_LEVEL, *PPH_SYMBOL_RESOLVE_LEVEL;

typedef struct _PH_SYMBOL_INFORMATION
{
    ULONG64 Address;
    ULONG64 ModuleBase;
    ULONG Index;
    ULONG Size;
} PH_SYMBOL_INFORMATION, *PPH_SYMBOL_INFORMATION;

typedef struct _PH_SYMBOL_LINE_INFORMATION
{
    ULONG LineNumber;
    ULONG64 Address;
} PH_SYMBOL_LINE_INFORMATION, *PPH_SYMBOL_LINE_INFORMATION;

typedef enum _PH_SYMBOL_EVENT_TYPE
{
    SymbolDeferredSymbolLoadStart = 1,
    SymbolDeferredSymbolLoadComplete = 2,
    SymbolDeferredSymbolLoadFailure = 3,
    SymbolSymbolsUnloaded = 4,
    SymbolDeferredSymbolLoadCancel = 7
} PH_SYMBOL_EVENT_TYPE;

typedef struct _PH_SYMBOL_EVENT_DATA
{
    PPH_SYMBOL_PROVIDER SymbolProvider;
    PH_SYMBOL_EVENT_TYPE Type;

    ULONG64 BaseAddress;
    ULONG CheckSum;
    ULONG TimeStamp;
    PPH_STRING FileName;
} PH_SYMBOL_EVENT_DATA, *PPH_SYMBOL_EVENT_DATA;

BOOLEAN PhSymbolProviderInitialization(
    VOID
    );

VOID PhSymbolProviderDynamicImport(
    VOID
    );

PHLIBAPI
PPH_SYMBOL_PROVIDER PhCreateSymbolProvider(
    _In_opt_ HANDLE ProcessId
    );

PHLIBAPI
BOOLEAN PhGetLineFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address,
    _Out_ PPH_STRING *FileName,
    _Out_opt_ PULONG Displacement,
    _Out_opt_ PPH_SYMBOL_LINE_INFORMATION Information
    );

PHLIBAPI
ULONG64 PhGetModuleFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address,
    _Out_opt_ PPH_STRING *FileName
    );

PHLIBAPI
PPH_STRING PhGetSymbolFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address,
    _Out_opt_ PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel,
    _Out_opt_ PPH_STRING *FileName,
    _Out_opt_ PPH_STRING *SymbolName,
    _Out_opt_ PULONG64 Displacement
    );

PHLIBAPI
BOOLEAN PhGetSymbolFromName(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PWSTR Name,
    _Out_ PPH_SYMBOL_INFORMATION Information
    );

PHLIBAPI
BOOLEAN PhLoadModuleSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PWSTR FileName,
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Size
    );

PHLIBAPI
VOID PhSetOptionsSymbolProvider(
    _In_ ULONG Mask,
    _In_ ULONG Value
    );

PHLIBAPI
VOID PhSetSearchPathSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PWSTR Path
    );

// svcsup

extern WCHAR *PhServiceTypeStrings[6];
extern WCHAR *PhServiceStartTypeStrings[5];
extern WCHAR *PhServiceErrorControlStrings[4];

PHLIBAPI
PVOID PhEnumServices(
    _In_ SC_HANDLE ScManagerHandle,
    _In_opt_ ULONG Type,
    _In_opt_ ULONG State,
    _Out_ PULONG Count
    );

PHLIBAPI
SC_HANDLE PhOpenService(
    _In_ PWSTR ServiceName,
    _In_ ACCESS_MASK DesiredAccess
    );

PHLIBAPI
PVOID PhGetServiceConfig(
    _In_ SC_HANDLE ServiceHandle
    );

PHLIBAPI
PVOID PhQueryServiceVariableSize(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG InfoLevel
    );

PHLIBAPI
PPH_STRING PhGetServiceDescription(
    _In_ SC_HANDLE ServiceHandle
    );

PHLIBAPI
BOOLEAN PhGetServiceDelayedAutoStart(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ PBOOLEAN DelayedAutoStart
    );

PHLIBAPI
BOOLEAN PhSetServiceDelayedAutoStart(
    _In_ SC_HANDLE ServiceHandle,
    _In_ BOOLEAN DelayedAutoStart
    );

PHLIBAPI
PWSTR PhGetServiceStateString(
    _In_ ULONG ServiceState
    );

PHLIBAPI
PWSTR PhGetServiceTypeString(
    _In_ ULONG ServiceType
    );

PHLIBAPI
ULONG PhGetServiceTypeInteger(
    _In_ PWSTR ServiceType
    );

PHLIBAPI
PWSTR PhGetServiceStartTypeString(
    _In_ ULONG ServiceStartType
    );

PHLIBAPI
ULONG PhGetServiceStartTypeInteger(
    _In_ PWSTR ServiceStartType
    );

PHLIBAPI
PWSTR PhGetServiceErrorControlString(
    _In_ ULONG ServiceErrorControl
    );

PHLIBAPI
ULONG PhGetServiceErrorControlInteger(
    _In_ PWSTR ServiceErrorControl
    );

PHLIBAPI
PPH_STRING PhGetServiceNameFromTag(
    _In_ HANDLE ProcessId,
    _In_ PVOID ServiceTag
    );

PHLIBAPI
NTSTATUS PhGetThreadServiceTag(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle,
    _Out_ PVOID *ServiceTag
    );

NTSTATUS PhGetServiceDllParameter(
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

FORCEINLINE PH_RECTANGLE PhRectToRectangle(
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

FORCEINLINE RECT PhRectangleToRect(
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

FORCEINLINE VOID PhConvertRect(
    _Inout_ PRECT Rect,
    _In_ PRECT ParentRect
    )
{
    Rect->right = ParentRect->right - ParentRect->left - Rect->right;
    Rect->bottom = ParentRect->bottom - ParentRect->top - Rect->bottom;
}

FORCEINLINE RECT PhMapRect(
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
VOID PhAdjustRectangleToBounds(
    _Inout_ PPH_RECTANGLE Rectangle,
    _In_ PPH_RECTANGLE Bounds
    );

PHLIBAPI
VOID PhCenterRectangle(
    _Inout_ PPH_RECTANGLE Rectangle,
    _In_ PPH_RECTANGLE Bounds
    );

PHLIBAPI
VOID PhAdjustRectangleToWorkingArea(
    _In_ HWND hWnd,
    _Inout_ PPH_RECTANGLE Rectangle
    );

PHLIBAPI
VOID PhCenterWindow(
    _In_ HWND WindowHandle,
    _In_opt_ HWND ParentWindowHandle
    );

FORCEINLINE VOID PhLargeIntegerToSystemTime(
    _Out_ PSYSTEMTIME SystemTime,
    _In_ PLARGE_INTEGER LargeInteger
    )
{
    FILETIME fileTime;

    fileTime.dwLowDateTime = LargeInteger->LowPart;
    fileTime.dwHighDateTime = LargeInteger->HighPart;
    FileTimeToSystemTime(&fileTime, SystemTime);
}

FORCEINLINE VOID PhLargeIntegerToLocalSystemTime(
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

FORCEINLINE FILETIME PhSubtractFileTime(
    _Inout_ FILETIME Value1,
    _In_ FILETIME Value2
    )
{
    ULARGE_INTEGER value1;
    ULARGE_INTEGER value2;
    ULARGE_INTEGER value3;
    FILETIME result;

    value1.LowPart = Value1.dwLowDateTime;
    value1.HighPart = Value1.dwHighDateTime;
    value2.LowPart = Value2.dwLowDateTime;
    value2.HighPart = Value2.dwHighDateTime;

    value3.QuadPart = value1.QuadPart - value2.QuadPart;
    result.dwLowDateTime = value3.LowPart;
    result.dwHighDateTime = value3.HighPart;

    return result;
}

PHLIBAPI
VOID PhReferenceObjects(
    _In_reads_(NumberOfObjects) PVOID *Objects,
    _In_ ULONG NumberOfObjects
    );

PHLIBAPI
VOID PhDereferenceObjects(
    _In_reads_(NumberOfObjects) PVOID *Objects,
    _In_ ULONG NumberOfObjects
    );

PHLIBAPI
PPH_STRING PhGetMessage(
    _In_ PVOID DllHandle,
    _In_ ULONG MessageTableId,
    _In_ ULONG MessageLanguageId,
    _In_ ULONG MessageId
    );

PHLIBAPI
PPH_STRING PhGetNtMessage(
    _In_ NTSTATUS Status
    );

PHLIBAPI
PPH_STRING PhGetWin32Message(
    _In_ ULONG Result
    );

#define PH_MAX_MESSAGE_SIZE 800

PHLIBAPI
INT PhShowMessage(
    _In_ HWND hWnd,
    _In_ ULONG Type,
    _In_ PWSTR Format,
    ...
    );

PHLIBAPI
INT PhShowMessage_V(
    _In_ HWND hWnd,
    _In_ ULONG Type,
    _In_ PWSTR Format,
    _In_ va_list ArgPtr
    );

#define PhShowError(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONERROR, Format, __VA_ARGS__)
#define PhShowWarning(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONWARNING, Format, __VA_ARGS__)
#define PhShowInformation(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONINFORMATION, Format, __VA_ARGS__)

PPH_STRING PhGetStatusMessage(
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    );

PHLIBAPI
VOID PhShowStatus(
    _In_ HWND hWnd,
    _In_opt_ PWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    );

PHLIBAPI
BOOLEAN PhShowContinueStatus(
    _In_ HWND hWnd,
    _In_opt_ PWSTR Message,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    );

PHLIBAPI
BOOLEAN PhShowConfirmMessage(
    _In_ HWND hWnd,
    _In_ PWSTR Verb,
    _In_ PWSTR Object,
    _In_opt_ PWSTR Message,
    _In_ BOOLEAN Warning
    );

PHLIBAPI
BOOLEAN PhFindIntegerSiKeyValuePairs(
    _In_ PPH_KEY_VALUE_PAIR KeyValuePairs,
    _In_ ULONG SizeOfKeyValuePairs,
    _In_ PWSTR String,
    _Out_ PULONG Integer
    );

PHLIBAPI
BOOLEAN PhFindStringSiKeyValuePairs(
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
VOID PhGenerateGuid(
    _Out_ PGUID Guid
    );

PHLIBAPI
VOID PhGenerateGuidFromName(
    _Out_ PGUID Guid,
    _In_ PGUID Namespace,
    _In_ PCHAR Name,
    _In_ ULONG NameLength,
    _In_ UCHAR Version
    );

PHLIBAPI
VOID PhGenerateRandomAlphaString(
    _Out_writes_z_(Count) PWSTR Buffer,
    _In_ ULONG Count
    );

PHLIBAPI
PPH_STRING PhEllipsisString(
    _In_ PPH_STRING String,
    _In_ ULONG DesiredCount
    );

PHLIBAPI
PPH_STRING PhEllipsisStringPath(
    _In_ PPH_STRING String,
    _In_ ULONG DesiredCount
    );

PHLIBAPI
BOOLEAN PhMatchWildcards(
    _In_ PWSTR Pattern,
    _In_ PWSTR String,
    _In_ BOOLEAN IgnoreCase
    );

PHLIBAPI
PPH_STRING PhEscapeStringForMenuPrefix(
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
LONG PhCompareUnicodeStringZIgnoreMenuPrefix(
    _In_ PWSTR A,
    _In_ PWSTR B,
    _In_ BOOLEAN IgnoreCase,
    _In_ BOOLEAN MatchIfPrefix
    );

PHLIBAPI
PPH_STRING PhFormatDate(
    _In_opt_ PSYSTEMTIME Date,
    _In_opt_ PWSTR Format
    );

PHLIBAPI
PPH_STRING PhFormatTime(
    _In_opt_ PSYSTEMTIME Time,
    _In_opt_ PWSTR Format
    );

PHLIBAPI
PPH_STRING PhFormatDateTime(
    _In_opt_ PSYSTEMTIME DateTime
    );

FORCEINLINE PPH_STRING PhaFormatDateTime(
    _In_opt_ PSYSTEMTIME DateTime
    )
{
    return (PPH_STRING)PhAutoDereferenceObject(PhFormatDateTime(DateTime));
}

PHLIBAPI
PPH_STRING PhFormatTimeSpanRelative(
    _In_ ULONG64 TimeSpan
    );

PHLIBAPI
PPH_STRING PhFormatUInt64(
    _In_ ULONG64 Value,
    _In_ BOOLEAN GroupDigits
    );

#define PhaFormatUInt64(Value, GroupDigits) \
    ((PPH_STRING)PhAutoDereferenceObject(PhFormatUInt64((Value), (GroupDigits))))

PHLIBAPI
PPH_STRING PhFormatDecimal(
    _In_ PWSTR Value,
    _In_ ULONG FractionalDigits,
    _In_ BOOLEAN GroupDigits
    );

#define PhaFormatDecimal(Value, FractionalDigits, GroupDigits) \
    ((PPH_STRING)PhAutoDereferenceObject(PhFormatDecimal((Value), (FractionalDigits), (GroupDigits))))

PHLIBAPI
PPH_STRING PhFormatSize(
    _In_ ULONG64 Size,
    _In_ ULONG MaxSizeUnit
    );

#define PhaFormatSize(Size, MaxSizeUnit) \
    ((PPH_STRING)PhAutoDereferenceObject(PhFormatSize((Size), (MaxSizeUnit))))

PHLIBAPI
PPH_STRING PhFormatGuid(
    _In_ PGUID Guid
    );

PHLIBAPI
PVOID PhGetFileVersionInfo(
    _In_ PWSTR FileName
    );

PHLIBAPI
ULONG PhGetFileVersionInfoLangCodePage(
    _In_ PVOID VersionInfo
    );

PHLIBAPI
PPH_STRING PhGetFileVersionInfoString(
    _In_ PVOID VersionInfo,
    _In_ PWSTR SubBlock
    );

PHLIBAPI
PPH_STRING PhGetFileVersionInfoString2(
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
BOOLEAN PhInitializeImageVersionInfo(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PWSTR FileName
    );

PHLIBAPI
VOID PhDeleteImageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo
    );

PHLIBAPI
PPH_STRING PhFormatImageVersionInfo(
    _In_opt_ PPH_STRING FileName,
    _In_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_opt_ PWSTR Indent,
    _In_opt_ ULONG LineLimit
    );

PHLIBAPI
PPH_STRING PhGetFullPath(
    _In_ PWSTR FileName,
    _Out_opt_ PULONG IndexOfFileName
    );

PHLIBAPI
PPH_STRING PhExpandEnvironmentStrings(
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
PPH_STRING PhGetBaseName(
    _In_ PPH_STRING FileName
    );

PHLIBAPI
PPH_STRING PhGetSystemDirectory(
    VOID
    );

PHLIBAPI
VOID PhGetSystemRoot(
    _Out_ PPH_STRINGREF SystemRoot
    );

PHLIBAPI
PLDR_DATA_TABLE_ENTRY PhFindLoaderEntry(
    _In_opt_ PVOID DllBase,
    _In_opt_ PPH_STRINGREF FullDllName,
    _In_opt_ PPH_STRINGREF BaseDllName
    );

PHLIBAPI
PPH_STRING PhGetDllFileName(
    _In_ PVOID DllHandle,
    _Out_opt_ PULONG IndexOfFileName
    );

PHLIBAPI
PPH_STRING PhGetApplicationFileName(
    VOID
    );

PHLIBAPI
PPH_STRING PhGetApplicationDirectory(
    VOID
    );

PHLIBAPI
PPH_STRING PhGetKnownLocation(
    _In_ ULONG Folder,
    _In_opt_ PWSTR AppendPath
    );

PHLIBAPI
NTSTATUS PhWaitForMultipleObjectsAndPump(
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
NTSTATUS PhCreateProcess(
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
NTSTATUS PhCreateProcessWin32(
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
NTSTATUS PhCreateProcessWin32Ex(
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
NTSTATUS PhCreateProcessAsUser(
    _In_ PPH_CREATE_PROCESS_AS_USER_INFO Information,
    _In_ ULONG Flags,
    _Out_opt_ PCLIENT_ID ClientId,
    _Out_opt_ PHANDLE ProcessHandle,
    _Out_opt_ PHANDLE ThreadHandle
    );

NTSTATUS PhFilterTokenForLimitedUser(
    _In_ HANDLE TokenHandle,
    _Out_ PHANDLE NewTokenHandle
    );

PHLIBAPI
VOID PhShellExecute(
    _In_ HWND hWnd,
    _In_ PWSTR FileName,
    _In_opt_ PWSTR Parameters
    );

#define PH_SHELL_EXECUTE_ADMIN 0x1
#define PH_SHELL_EXECUTE_PUMP_MESSAGES 0x2

PHLIBAPI
BOOLEAN PhShellExecuteEx(
    _In_opt_ HWND hWnd,
    _In_ PWSTR FileName,
    _In_opt_ PWSTR Parameters,
    _In_ ULONG ShowWindowType,
    _In_ ULONG Flags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    );

PHLIBAPI
VOID PhShellExploreFile(
    _In_ HWND hWnd,
    _In_ PWSTR FileName
    );

PHLIBAPI
VOID PhShellProperties(
    _In_ HWND hWnd,
    _In_ PWSTR FileName
    );

PPH_STRING PhExpandKeyName(
    _In_ PPH_STRING KeyName,
    _In_ BOOLEAN Computer
    );

PHLIBAPI
VOID PhShellOpenKey(
    _In_ HWND hWnd,
    _In_ PPH_STRING KeyName
    );

PKEY_VALUE_PARTIAL_INFORMATION PhQueryRegistryValue(
    _In_ HANDLE KeyHandle,
    _In_opt_ PWSTR ValueName
    );

PHLIBAPI
PPH_STRING PhQueryRegistryString(
    _In_ HANDLE KeyHandle,
    _In_opt_ PWSTR ValueName
    );

typedef struct _PH_FLAG_MAPPING
{
    ULONG Flag1;
    ULONG Flag2;
} PH_FLAG_MAPPING, *PPH_FLAG_MAPPING;

PHLIBAPI
VOID PhMapFlags1(
    _Inout_ PULONG Value2,
    _In_ ULONG Value1,
    _In_ const PH_FLAG_MAPPING *Mappings,
    _In_ ULONG NumberOfMappings
    );

PHLIBAPI
VOID PhMapFlags2(
    _Inout_ PULONG Value1,
    _In_ ULONG Value2,
    _In_ const PH_FLAG_MAPPING *Mappings,
    _In_ ULONG NumberOfMappings
    );

PHLIBAPI
PVOID PhCreateOpenFileDialog(
    VOID
    );

PHLIBAPI
PVOID PhCreateSaveFileDialog(
    VOID
    );

PHLIBAPI
VOID PhFreeFileDialog(
    _In_ PVOID FileDialog
    );

PHLIBAPI
BOOLEAN PhShowFileDialog(
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
ULONG PhGetFileDialogOptions(
    _In_ PVOID FileDialog
    );

PHLIBAPI
VOID PhSetFileDialogOptions(
    _In_ PVOID FileDialog,
    _In_ ULONG Options
    );

PHLIBAPI
ULONG PhGetFileDialogFilterIndex(
    _In_ PVOID FileDialog
    );

typedef struct _PH_FILETYPE_FILTER
{
    PWSTR Name;
    PWSTR Filter;
} PH_FILETYPE_FILTER, *PPH_FILETYPE_FILTER;

PHLIBAPI
VOID PhSetFileDialogFilter(
    _In_ PVOID FileDialog,
    _In_ PPH_FILETYPE_FILTER Filters,
    _In_ ULONG NumberOfFilters
    );

PHLIBAPI
PPH_STRING PhGetFileDialogFileName(
    _In_ PVOID FileDialog
    );

PHLIBAPI
VOID PhSetFileDialogFileName(
    _In_ PVOID FileDialog,
    _In_ PWSTR FileName
    );

PHLIBAPI
NTSTATUS PhIsExecutablePacked(
    _In_ PWSTR FileName,
    _Out_ PBOOLEAN IsPacked,
    _Out_opt_ PULONG NumberOfModules,
    _Out_opt_ PULONG NumberOfFunctions
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
VOID PhInitializeHash(
    _Out_ PPH_HASH_CONTEXT Context,
    _In_ PH_HASH_ALGORITHM Algorithm
    );

PHLIBAPI
VOID PhUpdateHash(
    _Inout_ PPH_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

PHLIBAPI
BOOLEAN PhFinalHash(
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
PPH_STRING PhParseCommandLinePart(
    _In_ PPH_STRINGREF CommandLine,
    _Inout_ PULONG_PTR Index
    );

PHLIBAPI
BOOLEAN PhParseCommandLine(
    _In_ PPH_STRINGREF CommandLine,
    _In_opt_ PPH_COMMAND_LINE_OPTION Options,
    _In_ ULONG NumberOfOptions,
    _In_ ULONG Flags,
    _In_ PPH_COMMAND_LINE_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
PPH_STRING PhEscapeCommandLinePart(
    _In_ PPH_STRINGREF String
    );

PHLIBAPI
BOOLEAN PhParseCommandLineFuzzy(
    _In_ PPH_STRINGREF CommandLine,
    _Out_ PPH_STRINGREF FileName,
    _Out_ PPH_STRINGREF Arguments,
    _Out_opt_ PPH_STRING *FullFileName
    );

#ifdef __cplusplus
}
#endif

#endif
