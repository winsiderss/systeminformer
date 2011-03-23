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
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessId
    );

PHLIBAPI
NTSTATUS PhOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ThreadId
    );

PHLIBAPI
NTSTATUS PhOpenThreadProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS PhOpenProcessToken(
    __out PHANDLE TokenHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS PhOpenThreadToken(
    __out PHANDLE TokenHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ThreadHandle,
    __in BOOLEAN OpenAsSelf
    );

PHLIBAPI
NTSTATUS PhGetObjectSecurity(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

PHLIBAPI
NTSTATUS PhSetObjectSecurity(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
    );

PHLIBAPI
NTSTATUS PhTerminateProcess(
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS PhSuspendProcess(
    __in HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS PhResumeProcess(
    __in HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS PhTerminateThread(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS PhSuspendThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

PHLIBAPI
NTSTATUS PhResumeThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

PHLIBAPI
NTSTATUS PhGetThreadContext(
    __in HANDLE ThreadHandle,
    __inout PCONTEXT Context
    );

PHLIBAPI
NTSTATUS PhSetThreadContext(
    __in HANDLE ThreadHandle,
    __in PCONTEXT Context
    );

PHLIBAPI
NTSTATUS PhReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead
    );

PHLIBAPI
NTSTATUS PhWriteVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesWritten
    );

PHLIBAPI
NTSTATUS PhGetProcessImageFileName(
    __in HANDLE ProcessHandle,
    __out PPH_STRING *FileName
    );

PHLIBAPI
NTSTATUS PhGetProcessImageFileNameWin32(
    __in HANDLE ProcessHandle,
    __out PPH_STRING *FileName
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
    __in HANDLE ProcessHandle,
    __in PH_PEB_OFFSET Offset,
    __out PPH_STRING *String
    );

/**
 * Gets a process' command line.
 *
 * \param ProcessHandle A handle to a process. The handle must 
 * have PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ 
 * access.
 * \param String A variable which receives a pointer to a 
 * string containing the command line. You must free the string 
 * using PhDereferenceObject() when you no longer need it.
 */
#define PhGetProcessCommandLine(ProcessHandle, String) \
    PhGetProcessPebString(ProcessHandle, PhpoCommandLine, String)

PHLIBAPI
NTSTATUS PhGetProcessIsPosix(
    __in HANDLE ProcessHandle,
    __out PBOOLEAN IsPosix
    );

PHLIBAPI
NTSTATUS PhGetProcessExecuteFlags(
    __in HANDLE ProcessHandle,
    __out PULONG ExecuteFlags
    );

#define PH_PROCESS_DEP_ENABLED 0x1
#define PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED 0x2
#define PH_PROCESS_DEP_PERMANENT 0x4

PHLIBAPI
NTSTATUS PhGetProcessDepStatus(
    __in HANDLE ProcessHandle,
    __out PULONG DepStatus
    );

PHLIBAPI
NTSTATUS PhGetProcessPosixCommandLine(
    __in HANDLE ProcessHandle,
    __out PPH_STRING *CommandLine
    );

/** Contains information about an environment variable. */
typedef struct _PH_ENVIRONMENT_VARIABLE
{
    /** A string containing the variable name. */
    PPH_STRING Name;
    /** A string containing the variable value. */
    PPH_STRING Value;
} PH_ENVIRONMENT_VARIABLE, *PPH_ENVIRONMENT_VARIABLE;

PHLIBAPI
NTSTATUS PhGetProcessEnvironmentVariables(
    __in HANDLE ProcessHandle,
    __out PPH_ENVIRONMENT_VARIABLE *Variables,
    __out PULONG NumberOfVariables
    );

#define PH_GET_PROCESS_ENVIRONMENT_WOW64 0x1 // retrieve the WOW64 environment

PHLIBAPI
NTSTATUS PhGetProcessEnvironmentVariablesEx(
    __in HANDLE ProcessHandle,
    __in ULONG Flags,
    __out PPH_ENVIRONMENT_VARIABLE *Variables,
    __out PULONG NumberOfVariables
    );

PHLIBAPI
VOID PhFreeProcessEnvironmentVariables(
    __in PPH_ENVIRONMENT_VARIABLE Variables,
    __in ULONG NumberOfVariables
    );

PHLIBAPI
NTSTATUS PhGetProcessMappedFileName(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out PPH_STRING *FileName
    );

PHLIBAPI
NTSTATUS PhGetProcessWorkingSetInformation(
    __in HANDLE ProcessHandle,
    __out PMEMORY_WORKING_SET_INFORMATION *WorkingSetInformation
    );

typedef struct _PH_PROCESS_WS_COUNTERS
{
    ULONG NumberOfPages;
    ULONG NumberOfPrivatePages;
    ULONG NumberOfSharedPages;
    ULONG NumberOfShareablePages;
} PH_PROCESS_WS_COUNTERS, *PPH_PROCESS_WS_COUNTERS;

PHLIBAPI
NTSTATUS PhGetProcessWsCounters(
    __in HANDLE ProcessHandle,
    __out PPH_PROCESS_WS_COUNTERS WsCounters
    );

typedef struct _KPH_PROCESS_HANDLE_INFORMATION *PKPH_PROCESS_HANDLE_INFORMATION;

PHLIBAPI
NTSTATUS PhEnumProcessHandles(
    __in HANDLE ProcessHandle,
    __out PKPH_PROCESS_HANDLE_INFORMATION *Handles
    );

PHLIBAPI
NTSTATUS PhSetProcessAffinityMask(
    __in HANDLE ProcessHandle,
    __in ULONG_PTR AffinityMask
    );

PHLIBAPI
NTSTATUS PhSetProcessIoPriority(
    __in HANDLE ProcessHandle,
    __in ULONG IoPriority
    );

PHLIBAPI
NTSTATUS PhSetProcessExecuteFlags(
    __in HANDLE ProcessHandle,
    __in ULONG ExecuteFlags
    );

PHLIBAPI
NTSTATUS PhSetProcessDepStatus(
    __in HANDLE ProcessHandle,
    __in ULONG DepStatus
    );

PHLIBAPI
NTSTATUS PhSetProcessDepStatusInvasive(
    __in HANDLE ProcessHandle,
    __in ULONG DepStatus,
    __in_opt PLARGE_INTEGER Timeout
    );

PHLIBAPI
NTSTATUS PhInjectDllProcess(
    __in HANDLE ProcessHandle,
    __in PWSTR FileName,
    __in_opt PLARGE_INTEGER Timeout
    );

PHLIBAPI
NTSTATUS PhUnloadDllProcess(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_opt PLARGE_INTEGER Timeout
    );

PHLIBAPI
NTSTATUS PhSetThreadAffinityMask(
    __in HANDLE ThreadHandle,
    __in ULONG_PTR AffinityMask
    );

PHLIBAPI
NTSTATUS PhSetThreadIoPriority(
    __in HANDLE ThreadHandle,
    __in ULONG IoPriority
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
    __in PPH_THREAD_STACK_FRAME StackFrame,
    __in_opt PVOID Context
    );

PHLIBAPI
NTSTATUS PhWalkThreadStack(
    __in HANDLE ThreadHandle,
    __in_opt HANDLE ProcessHandle,
    __in ULONG Flags,
    __in PPH_WALK_THREAD_STACK_CALLBACK Callback,
    __in_opt PVOID Context
    );

PHLIBAPI
NTSTATUS PhGetJobProcessIdList(
    __in HANDLE JobHandle,
    __out PJOBOBJECT_BASIC_PROCESS_ID_LIST *ProcessIdList
    );

PHLIBAPI
NTSTATUS PhGetTokenUser(
    __in HANDLE TokenHandle,
    __out PTOKEN_USER *User
    );

PHLIBAPI
NTSTATUS PhGetTokenOwner(
    __in HANDLE TokenHandle,
    __out PTOKEN_OWNER *Owner
    );

PHLIBAPI
NTSTATUS PhGetTokenPrimaryGroup(
    __in HANDLE TokenHandle,
    __out PTOKEN_PRIMARY_GROUP *PrimaryGroup
    );

PHLIBAPI
NTSTATUS PhGetTokenGroups(
    __in HANDLE TokenHandle,
    __out PTOKEN_GROUPS *Groups
    );

PHLIBAPI
NTSTATUS PhGetTokenPrivileges(
    __in HANDLE TokenHandle,
    __out PTOKEN_PRIVILEGES *Privileges
    );

PHLIBAPI
NTSTATUS PhSetTokenSessionId(
    __in HANDLE TokenHandle,
    __in ULONG SessionId
    );

PHLIBAPI
BOOLEAN PhSetTokenPrivilege(
    __in HANDLE TokenHandle,
    __in_opt PWSTR PrivilegeName,
    __in_opt PLUID PrivilegeLuid,
    __in ULONG Attributes
    );

PHLIBAPI
BOOLEAN PhSetTokenPrivilege2(
    __in HANDLE TokenHandle,
    __in LONG Privilege,
    __in ULONG Attributes
    );

PHLIBAPI
NTSTATUS PhSetTokenIsVirtualizationEnabled(
    __in HANDLE TokenHandle,
    __in BOOLEAN IsVirtualizationEnabled
    );

/** Specifies a token integrity level. */
typedef enum _PH_INTEGRITY
{
    PiUntrusted = 0,
    PiLow = 1,
    PiMedium = 2,
    PiMediumPlus = 3, // unused
    PiHigh = 4,
    PiSystem = 5,
    PiInstaller = 6,
    PiProtected = 7, // unused
    PiSecure = 8 // unused
} PH_INTEGRITY, *PPH_INTEGRITY;

PHLIBAPI
NTSTATUS PhGetTokenIntegrityLevel(
    __in HANDLE TokenHandle,
    __out_opt PPH_INTEGRITY IntegrityLevel, 
    __out_opt PWSTR *IntegrityString
    );

PHLIBAPI
NTSTATUS PhGetFileSize(
    __in HANDLE FileHandle,
    __out PLARGE_INTEGER Size
    );

PHLIBAPI
NTSTATUS PhSetFileSize(
    __in HANDLE FileHandle,
    __in PLARGE_INTEGER Size
    );

PHLIBAPI
NTSTATUS PhGetTransactionManagerBasicInformation(
    __in HANDLE TransactionManagerHandle,
    __out PTRANSACTIONMANAGER_BASIC_INFORMATION BasicInformation
    );

PHLIBAPI
NTSTATUS PhGetTransactionManagerLogFileName(
    __in HANDLE TransactionManagerHandle,
    __out PPH_STRING *LogFileName
    );

PHLIBAPI
NTSTATUS PhGetTransactionBasicInformation(
    __in HANDLE TransactionHandle,
    __out PTRANSACTION_BASIC_INFORMATION BasicInformation
    );

PHLIBAPI
NTSTATUS PhGetTransactionPropertiesInformation(
    __in HANDLE TransactionHandle,
    __out_opt PLARGE_INTEGER Timeout,
    __out_opt TRANSACTION_OUTCOME *Outcome,
    __out_opt PPH_STRING *Description
    );

PHLIBAPI
NTSTATUS PhGetResourceManagerBasicInformation(
    __in HANDLE ResourceManagerHandle,
    __out_opt PGUID Guid,
    __out_opt PPH_STRING *Description
    );

PHLIBAPI
NTSTATUS PhGetEnlistmentBasicInformation(
    __in HANDLE EnlistmentHandle,
    __out PENLISTMENT_BASIC_INFORMATION BasicInformation
    );

PHLIBAPI
NTSTATUS PhOpenDriverByBaseAddress(
    __out PHANDLE DriverHandle,
    __in PVOID BaseAddress
    );

PHLIBAPI
NTSTATUS PhGetDriverServiceKeyName(
    __in HANDLE DriverHandle,
    __out PPH_STRING *ServiceKeyName
    );

PHLIBAPI
NTSTATUS PhUnloadDriver(
    __in_opt PVOID BaseAddress,
    __in_opt PWSTR Name
    );

PHLIBAPI
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
    __in PLDR_DATA_TABLE_ENTRY Module,
    __in_opt PVOID Context
    );

PHLIBAPI
NTSTATUS PhEnumProcessModules(
    __in HANDLE ProcessHandle,
    __in PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    __in_opt PVOID Context
    );

PHLIBAPI
NTSTATUS PhSetProcessModuleLoadCount(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in USHORT LoadCount
    );

PHLIBAPI
NTSTATUS PhEnumProcessModules32(
    __in HANDLE ProcessHandle,
    __in PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    __in_opt PVOID Context
    );

PHLIBAPI
NTSTATUS PhSetProcessModuleLoadCount32(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in USHORT LoadCount
    );

PHLIBAPI
NTSTATUS PhGetProcedureAddressRemote(
    __in HANDLE ProcessHandle,
    __in PWSTR FileName,
    __in_opt PSTR ProcedureName,
    __in_opt ULONG ProcedureNumber,
    __out PVOID *ProcedureAddress,
    __out_opt PVOID *DllBase
    );

PHLIBAPI
NTSTATUS PhEnumKernelModules(
    __out PRTL_PROCESS_MODULES *Modules
    );

NTSTATUS PhEnumKernelModulesEx(
    __out PRTL_PROCESS_MODULE_INFORMATION_EX *Modules
    );

PHLIBAPI
PPH_STRING PhGetKernelFileName();

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
    __out PPVOID Processes
    );

PHLIBAPI
NTSTATUS PhEnumProcessesForSession(
    __out PPVOID Processes,
    __in ULONG SessionId
    );

PHLIBAPI
NTSTATUS PhEnumProcessesEx(
    __out PPVOID Processes
    );

PHLIBAPI
PSYSTEM_PROCESS_INFORMATION PhFindProcessInformation(
    __in PVOID Processes,
    __in HANDLE ProcessId
    );

PHLIBAPI
NTSTATUS PhEnumHandles(
    __out PSYSTEM_HANDLE_INFORMATION *Handles
    );

PHLIBAPI
NTSTATUS PhEnumHandlesEx(
    __out PSYSTEM_HANDLE_INFORMATION_EX *Handles
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
    __out PPVOID Pagefiles
    );

PHLIBAPI
NTSTATUS PhGetProcessImageFileNameByProcessId(
    __in HANDLE ProcessId,
    __out PPH_STRING *FileName
    );

PHLIBAPI
NTSTATUS PhGetProcessIsDotNet(
    __in HANDLE ProcessId,
    __out PBOOLEAN IsDotNet
    );

#define PH_IS_DOT_NET_VERSION_4 0x1

typedef struct _PH_IS_DOT_NET_CONTEXT *PPH_IS_DOT_NET_CONTEXT;

PHLIBAPI
NTSTATUS PhCreateIsDotNetContext(
    __out PPH_IS_DOT_NET_CONTEXT *IsDotNetContext,
    __in_opt PPH_STRINGREF DirectoryNames,
    __in ULONG NumberOfDirectoryNames
    );

PHLIBAPI
VOID PhFreeIsDotNetContext(
    __inout PPH_IS_DOT_NET_CONTEXT IsDotNetContext
    );

PHLIBAPI
BOOLEAN PhGetProcessIsDotNetFromContext(
    __in PPH_IS_DOT_NET_CONTEXT IsDotNetContext,
    __in HANDLE ProcessId,
    __out_opt PULONG Flags
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
    __in PPH_STRING Name,
    __in PPH_STRING TypeName,
    __in_opt PVOID Context
    );

PHLIBAPI
NTSTATUS PhEnumDirectoryObjects(
    __in HANDLE DirectoryHandle,
    __in PPH_ENUM_DIRECTORY_OBJECTS Callback,
    __in_opt PVOID Context
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_DIRECTORY_FILE)(
    __in PFILE_DIRECTORY_INFORMATION Information,
    __in_opt PVOID Context
    );

PHLIBAPI
NTSTATUS PhEnumDirectoryFile(
    __in HANDLE FileHandle,
    __in_opt PPH_STRINGREF SearchPattern,
    __in PPH_ENUM_DIRECTORY_FILE Callback,
    __in_opt PVOID Context
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
    __in HANDLE FileHandle,
    __out PPVOID Streams
    );

VOID PhInitializeDevicePrefixes();

PHLIBAPI
VOID PhRefreshMupDevicePrefixes();

PHLIBAPI
VOID PhRefreshDosDevicePrefixes();

PHLIBAPI
PPH_STRING PhResolveDevicePrefix(
    __in PPH_STRING Name
    );

PHLIBAPI
PPH_STRING PhGetFileName(
    __in PPH_STRING FileName
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
    __in PPH_MODULE_INFO Module,
    __in_opt PVOID Context
    );

#define PH_ENUM_GENERIC_MAPPED_FILES 0x1

PHLIBAPI
NTSTATUS PhEnumGenericModules(
    __in HANDLE ProcessId,
    __in_opt HANDLE ProcessHandle,
    __in ULONG Flags,
    __in PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    __in_opt PVOID Context
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
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt HANDLE RootDirectory,
    __in PPH_STRINGREF ObjectName,
    __in ULONG Attributes,
    __in ULONG CreateOptions,
    __out_opt PULONG Disposition
    );

PHLIBAPI
NTSTATUS PhOpenKey(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt HANDLE RootDirectory,
    __in PPH_STRINGREF ObjectName,
    __in ULONG Attributes
    );

// lsa

PHLIBAPI
NTSTATUS PhOpenLsaPolicy(
    __out PLSA_HANDLE PolicyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt PUNICODE_STRING SystemName
    );

LSA_HANDLE PhGetLookupPolicyHandle();

PHLIBAPI
BOOLEAN PhLookupPrivilegeName(
    __in PLUID PrivilegeValue,
    __out PPH_STRING *PrivilegeName
    );

PHLIBAPI
BOOLEAN PhLookupPrivilegeDisplayName(
    __in PPH_STRINGREF PrivilegeName,
    __out PPH_STRING *PrivilegeDisplayName
    );

PHLIBAPI
BOOLEAN PhLookupPrivilegeValue(
    __in PPH_STRINGREF PrivilegeName,
    __out PLUID PrivilegeValue
    );

PHLIBAPI
NTSTATUS PhLookupSid(
    __in PSID Sid,
    __out_opt PPH_STRING *Name,
    __out_opt PPH_STRING *DomainName,
    __out_opt PSID_NAME_USE NameUse
    );

PHLIBAPI
NTSTATUS PhLookupName(
    __in PPH_STRINGREF Name,
    __out_opt PSID *Sid,
    __out_opt PPH_STRING *DomainName,
    __out_opt PSID_NAME_USE NameUse
    );

PHLIBAPI
PPH_STRING PhGetSidFullName(
    __in PSID Sid,
    __in BOOLEAN IncludeDomain,
    __out_opt PSID_NAME_USE NameUse
    );

PHLIBAPI
PPH_STRING PhSidToStringSid(
    __in PSID Sid
    );

// hndlinfo

#define MAX_OBJECT_TYPE_NUMBER 257

VOID PhHandleInfoInitialization();

typedef PPH_STRING (NTAPI *PPH_GET_CLIENT_ID_NAME)(
    __in PCLIENT_ID ClientId
    );

PPH_GET_CLIENT_ID_NAME PhSetHandleClientIdFunction(
    __in PPH_GET_CLIENT_ID_NAME GetClientIdName
    );

PHLIBAPI
PPH_STRING PhFormatNativeKeyName(
    __in PPH_STRING Name
    );

PHLIBAPI
__callback PPH_STRING PhStdGetClientIdName(
    __in PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS PhGetHandleInformation(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG ObjectTypeNumber,
    __out_opt POBJECT_BASIC_INFORMATION BasicInformation,
    __out_opt PPH_STRING *TypeName,
    __out_opt PPH_STRING *ObjectName,
    __out_opt PPH_STRING *BestObjectName
    );

PHLIBAPI
NTSTATUS PhGetHandleInformationEx(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG ObjectTypeNumber,
    __reserved ULONG Flags,
    __out_opt PNTSTATUS SubStatus,
    __out_opt POBJECT_BASIC_INFORMATION BasicInformation,
    __out_opt PPH_STRING *TypeName,
    __out_opt PPH_STRING *ObjectName,
    __out_opt PPH_STRING *BestObjectName,
    __reserved PVOID *ExtraInformation
    );

NTSTATUS PhQueryObjectNameHack(
    __in HANDLE Handle,
    __out_bcount(ObjectNameInformationLength) POBJECT_NAME_INFORMATION ObjectNameInformation,
    __in ULONG ObjectNameInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS PhQueryObjectSecurityHack(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __out_opt PULONG ReturnLength
    );

NTSTATUS PhSetObjectSecurityHack(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Buffer
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
    __out PPH_MAPPED_IMAGE MappedImage,
    __in PVOID ViewBase,
    __in SIZE_T Size
    );

PHLIBAPI
NTSTATUS PhLoadMappedImage(
    __in_opt PWSTR FileName,
    __in_opt HANDLE FileHandle,
    __in BOOLEAN ReadOnly,
    __out PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS PhUnloadMappedImage(
    __inout PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS PhMapViewOfEntireFile(
    __in_opt PWSTR FileName,
    __in_opt HANDLE FileHandle,
    __in BOOLEAN ReadOnly,
    __out PPVOID ViewBase,
    __out PSIZE_T Size
    );

PHLIBAPI
PIMAGE_SECTION_HEADER PhMappedImageRvaToSection(
    __in PPH_MAPPED_IMAGE MappedImage,
    __in ULONG Rva
    );

PHLIBAPI
PVOID PhMappedImageRvaToVa(
    __in PPH_MAPPED_IMAGE MappedImage,
    __in ULONG Rva,
    __out_opt PIMAGE_SECTION_HEADER *Section
    );

PHLIBAPI
BOOLEAN PhGetMappedImageSectionName(
    __in PIMAGE_SECTION_HEADER Section,
    __out_ecount_z_opt(Count) PSTR Buffer,
    __in ULONG Count,
    __out_opt PULONG ReturnCount
    );

PHLIBAPI
NTSTATUS PhGetMappedImageDataEntry(
    __in PPH_MAPPED_IMAGE MappedImage,
    __in ULONG Index,
    __out PIMAGE_DATA_DIRECTORY *Entry
    );

PHLIBAPI
NTSTATUS PhGetMappedImageLoadConfig32(
    __in PPH_MAPPED_IMAGE MappedImage,
    __out PIMAGE_LOAD_CONFIG_DIRECTORY32 *LoadConfig
    );

PHLIBAPI
NTSTATUS PhGetMappedImageLoadConfig64(
    __in PPH_MAPPED_IMAGE MappedImage,
    __out PIMAGE_LOAD_CONFIG_DIRECTORY64 *LoadConfig
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
    __in HANDLE ProcessHandle,
    __in PVOID ViewBase,
    __out PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    );

NTSTATUS PhUnloadRemoteMappedImage(
    __inout PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
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
    __out PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS PhGetMappedImageExportEntry(
    __in PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in ULONG Index,
    __out PPH_MAPPED_IMAGE_EXPORT_ENTRY Entry
    );

PHLIBAPI
NTSTATUS PhGetMappedImageExportFunction(
    __in PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in_opt PSTR Name,
    __in_opt USHORT Ordinal,
    __out PPH_MAPPED_IMAGE_EXPORT_FUNCTION Function
    );

PHLIBAPI
NTSTATUS PhGetMappedImageExportFunctionRemote(
    __in PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in_opt PSTR Name,
    __in_opt USHORT Ordinal,
    __in PVOID RemoteBase,
    __out PPVOID Function
    );

typedef struct _PH_MAPPED_IMAGE_IMPORTS
{
    PPH_MAPPED_IMAGE MappedImage;
    ULONG NumberOfDlls;

    PIMAGE_IMPORT_DESCRIPTOR DescriptorTable;
} PH_MAPPED_IMAGE_IMPORTS, *PPH_MAPPED_IMAGE_IMPORTS;

typedef struct _PH_MAPPED_IMAGE_IMPORT_DLL
{
    PPH_MAPPED_IMAGE MappedImage;
    PSTR Name;
    ULONG NumberOfEntries;

    PIMAGE_IMPORT_DESCRIPTOR Descriptor;
    PPVOID LookupTable;
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
    __out PPH_MAPPED_IMAGE_IMPORTS Imports,
    __in PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS PhGetMappedImageImportDll(
    __in PPH_MAPPED_IMAGE_IMPORTS Imports,
    __in ULONG Index,
    __out PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll
    );

PHLIBAPI
NTSTATUS PhGetMappedImageImportEntry(
    __in PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll,
    __in ULONG Index,
    __out PPH_MAPPED_IMAGE_IMPORT_ENTRY Entry
    );

PHLIBAPI
ULONG PhCheckSumMappedImage(
    __in PPH_MAPPED_IMAGE MappedImage
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
    __out PPH_MAPPED_ARCHIVE MappedArchive,
    __in PVOID ViewBase,
    __in SIZE_T Size
    );

PHLIBAPI
NTSTATUS PhLoadMappedArchive(
    __in_opt PWSTR FileName,
    __in_opt HANDLE FileHandle,
    __in BOOLEAN ReadOnly,
    __out PPH_MAPPED_ARCHIVE MappedArchive
    );

PHLIBAPI
NTSTATUS PhUnloadMappedArchive(
    __inout PPH_MAPPED_ARCHIVE MappedArchive
    );

PHLIBAPI
NTSTATUS PhGetNextMappedArchiveMember(
    __in PPH_MAPPED_ARCHIVE_MEMBER Member,
    __out PPH_MAPPED_ARCHIVE_MEMBER NextMember
    );

PHLIBAPI
BOOLEAN PhIsMappedArchiveMemberShortFormat(
    __in PPH_MAPPED_ARCHIVE_MEMBER Member
    );

PHLIBAPI
NTSTATUS PhGetMappedArchiveImportEntry(
    __in PPH_MAPPED_ARCHIVE_MEMBER Member,
    __out PPH_MAPPED_ARCHIVE_IMPORT_ENTRY Entry
    );

// iosup

#ifndef _PH_IOSUP_PRIVATE
extern PPH_OBJECT_TYPE PhFileStreamType;
#endif

BOOLEAN PhIoSupportInitialization();

PHLIBAPI
NTSTATUS PhCreateFileWin32(
    __out PHANDLE FileHandle,
    __in PWSTR FileName,
    __in ACCESS_MASK DesiredAccess,
    __in_opt ULONG FileAttributes,
    __in ULONG ShareAccess,
    __in ULONG CreateDisposition,
    __in ULONG CreateOptions
    );

PHLIBAPI
NTSTATUS PhCreateFileWin32Ex(
    __out PHANDLE FileHandle,
    __in PWSTR FileName,
    __in ACCESS_MASK DesiredAccess,
    __in_opt ULONG FileAttributes,
    __in ULONG ShareAccess,
    __in ULONG CreateDisposition,
    __in ULONG CreateOptions,
    __out_opt PULONG CreateStatus
    );

PHLIBAPI
NTSTATUS PhDeleteFileWin32(
    __in PWSTR FileName
    );

PHLIBAPI
NTSTATUS PhListenNamedPipe(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock
    );

PHLIBAPI
NTSTATUS PhDisconnectNamedPipe(
    __in HANDLE FileHandle
    );

PHLIBAPI
NTSTATUS PhPeekNamedPipe(
    __in HANDLE FileHandle,
    __out_bcount_opt(Length) PVOID Buffer,
    __in ULONG Length,
    __out_opt PULONG NumberOfBytesRead,
    __out_opt PULONG NumberOfBytesAvailable,
    __out_opt PULONG NumberOfBytesLeftInMessage
    );

PHLIBAPI
NTSTATUS PhTransceiveNamedPipe(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in_bcount(InputBufferLength) PVOID InputBuffer,
    __in ULONG InputBufferLength,
    __out_bcount(OutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferLength
    );

PHLIBAPI
NTSTATUS PhWaitForNamedPipe(
    __in_opt PPH_STRINGREF FileSystemName,
    __in PPH_STRINGREF Name,
    __in_opt PLARGE_INTEGER Timeout,
    __in BOOLEAN UseDefaultTimeout
    );

PHLIBAPI
NTSTATUS PhImpersonateClientOfNamedPipe(
    __in HANDLE FileHandle
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
    __out PPH_FILE_STREAM *FileStream,
    __in PWSTR FileName,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG ShareMode,
    __in ULONG CreateDisposition,
    __in ULONG Flags
    );

PHLIBAPI
NTSTATUS PhCreateFileStream2(
    __out PPH_FILE_STREAM *FileStream,
    __in HANDLE FileHandle,
    __in ULONG Flags,
    __in ULONG BufferLength
    );

PHLIBAPI
VOID PhVerifyFileStream(
    __in PPH_FILE_STREAM FileStream
    );

PHLIBAPI
NTSTATUS PhReadFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __out_opt PULONG ReadLength
    );

PHLIBAPI
NTSTATUS PhWriteFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in_bcount(Length) PVOID Buffer,
    __in ULONG Length
    );

PHLIBAPI
NTSTATUS PhFlushFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in BOOLEAN Full
    );

PHLIBAPI
VOID PhGetPositionFileStream(
    __in PPH_FILE_STREAM FileStream,
    __out PLARGE_INTEGER Position
    );

PHLIBAPI
NTSTATUS PhSeekFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Offset,
    __in PH_SEEK_ORIGIN Origin
    );

PHLIBAPI
NTSTATUS PhLockFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Position,
    __in PLARGE_INTEGER Length,
    __in BOOLEAN Wait,
    __in BOOLEAN Shared
    );

PHLIBAPI
NTSTATUS PhUnlockFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Position,
    __in PLARGE_INTEGER Length
    );

#define PH_FILE_STREAM_STRING_BLOCK_SIZE (PAGE_SIZE / 2)

PHLIBAPI
NTSTATUS PhWriteStringAsAnsiFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in PPH_STRINGREF String
    );

PHLIBAPI
NTSTATUS PhWriteStringAsAnsiFileStream2(
    __inout PPH_FILE_STREAM FileStream,
    __in PWSTR String
    );

PHLIBAPI
NTSTATUS PhWriteStringAsAnsiFileStreamEx(
    __inout PPH_FILE_STREAM FileStream,
    __in PWSTR Buffer,
    __in SIZE_T Length
    );

PHLIBAPI
NTSTATUS PhWriteStringFormatFileStream_V(
    __inout PPH_FILE_STREAM FileStream,
    __in __format_string PWSTR Format,
    __in va_list ArgPtr
    );

PHLIBAPI
NTSTATUS PhWriteStringFormatFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in __format_string PWSTR Format,
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
    VrSecuritySettings
} VERIFY_RESULT, *PVERIFY_RESULT;

PHLIBAPI
VERIFY_RESULT PhVerifyFile(
    __in PWSTR FileName,
    __out_opt PPH_STRING *SignerName
    );

// provider

#if !defined(_PH_PROVIDER_PRIVATE) && defined(DEBUG)
extern LIST_ENTRY PhDbgProviderListHead;
extern PH_QUEUED_LOCK PhDbgProviderListLock;
#endif

typedef enum _PH_PROVIDER_THREAD_STATE
{
    ProviderThreadRunning,
    ProviderThreadStopped,
    ProviderThreadStopping
} PH_PROVIDER_THREAD_STATE;

typedef VOID (NTAPI *PPH_PROVIDER_FUNCTION)(
    __in PVOID Object
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
#ifdef DEBUG
    LIST_ENTRY DbgListEntry;
#endif
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
    __out PPH_PROVIDER_THREAD ProviderThread,
    __in ULONG Interval
    );

PHLIBAPI
VOID PhDeleteProviderThread(
    __inout PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID PhStartProviderThread(
    __inout PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID PhStopProviderThread(
    __inout PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID PhSetIntervalProviderThread(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __in ULONG Interval
    );

PHLIBAPI
VOID PhRegisterProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __in PPH_PROVIDER_FUNCTION Function,
    __in_opt PVOID Object,
    __out PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
VOID PhUnregisterProvider(
    __inout PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
BOOLEAN PhBoostProvider(
    __inout PPH_PROVIDER_REGISTRATION Registration,
    __out_opt PULONG FutureRunId
    );

PHLIBAPI
ULONG PhGetRunIdProvider(
    __in PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
BOOLEAN PhGetEnabledProvider(
    __in PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
VOID PhSetEnabledProvider(
    __inout PPH_PROVIDER_REGISTRATION Registration,
    __in BOOLEAN Enabled
    );

// symprv

#ifndef _PH_SYMPRV_PRIVATE
extern PPH_OBJECT_TYPE PhSymbolProviderType;
#endif

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
} PH_SYMBOL_LINE_INFORMATION, *PPH_SYMBOL_LINE_IINFORMATION;

BOOLEAN PhSymbolProviderInitialization();

VOID PhSymbolProviderDynamicImport();

PHLIBAPI
PPH_SYMBOL_PROVIDER PhCreateSymbolProvider(
    __in_opt HANDLE ProcessId
    );

PHLIBAPI
BOOLEAN PhGetLineFromAddress(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address,
    __out PPH_STRING *FileName,
    __out_opt PULONG Displacement,
    __out_opt PPH_SYMBOL_LINE_IINFORMATION Information
    );

PHLIBAPI
ULONG64 PhGetModuleFromAddress(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address,
    __out_opt PPH_STRING *FileName
    );

PHLIBAPI
PPH_STRING PhGetSymbolFromAddress(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address,
    __out_opt PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel,
    __out_opt PPH_STRING *FileName,
    __out_opt PPH_STRING *SymbolName,
    __out_opt PULONG64 Displacement
    );

PHLIBAPI
BOOLEAN PhGetSymbolFromName(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in PWSTR Name,
    __out PPH_SYMBOL_INFORMATION Information
    );

PHLIBAPI
BOOLEAN PhLoadModuleSymbolProvider(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in PWSTR FileName,
    __in ULONG64 BaseAddress,
    __in ULONG Size
    );

PHLIBAPI
VOID PhSetOptionsSymbolProvider(
    __in ULONG Mask,
    __in ULONG Value
    );

PHLIBAPI
VOID PhSetSearchPathSymbolProvider(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in PWSTR Path
    );

// svcsup

#ifndef _PH_SVCSUP_PRIVATE
extern WCHAR *PhServiceTypeStrings[6];
extern WCHAR *PhServiceStartTypeStrings[5];
extern WCHAR *PhServiceErrorControlStrings[4];
#endif

PHLIBAPI
PVOID PhEnumServices(
    __in SC_HANDLE ScManagerHandle,
    __in_opt ULONG Type,
    __in_opt ULONG State,
    __out PULONG Count
    );

PHLIBAPI
SC_HANDLE PhOpenService(
    __in PWSTR ServiceName,
    __in ACCESS_MASK DesiredAccess
    );

PHLIBAPI
PVOID PhGetServiceConfig(
    __in SC_HANDLE ServiceHandle
    );

PHLIBAPI
PVOID PhQueryServiceVariableSize(
    __in SC_HANDLE ServiceHandle,
    __in ULONG InfoLevel
    );

PHLIBAPI
PPH_STRING PhGetServiceDescription(
    __in SC_HANDLE ServiceHandle
    );

PHLIBAPI
BOOLEAN PhGetServiceDelayedAutoStart(
    __in SC_HANDLE ServiceHandle,
    __out PBOOLEAN DelayedAutoStart
    );

PHLIBAPI
BOOLEAN PhSetServiceDelayedAutoStart(
    __in SC_HANDLE ServiceHandle,
    __in BOOLEAN DelayedAutoStart
    );

PHLIBAPI
PWSTR PhGetServiceStateString(
    __in ULONG ServiceState
    );

PHLIBAPI
PWSTR PhGetServiceTypeString(
    __in ULONG ServiceType
    );

PHLIBAPI
ULONG PhGetServiceTypeInteger(
    __in PWSTR ServiceType
    );

PHLIBAPI
PWSTR PhGetServiceStartTypeString(
    __in ULONG ServiceStartType
    );

PHLIBAPI
ULONG PhGetServiceStartTypeInteger(
    __in PWSTR ServiceStartType
    );

PHLIBAPI
PWSTR PhGetServiceErrorControlString(
    __in ULONG ServiceErrorControl
    );

PHLIBAPI
ULONG PhGetServiceErrorControlInteger(
    __in PWSTR ServiceErrorControl
    );

PHLIBAPI
PPH_STRING PhGetServiceNameFromTag(
    __in HANDLE ProcessId,
    __in PVOID ServiceTag
    );

PHLIBAPI
NTSTATUS PhGetThreadServiceTag(
    __in HANDLE ThreadHandle,
    __in_opt HANDLE ProcessHandle,
    __out PPVOID ServiceTag
    );

// support

#ifndef _PH_SUPPORT_PRIVATE
extern WCHAR *PhSizeUnitNames[7];
extern ULONG PhMaxSizeUnit;
#endif

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
    __in RECT Rect
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
    __in PH_RECTANGLE Rectangle
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
    __inout PRECT Rect,
    __in PRECT ParentRect
    )
{
    Rect->right = ParentRect->right - ParentRect->left - Rect->right;
    Rect->bottom = ParentRect->bottom - ParentRect->top - Rect->bottom;
}

FORCEINLINE RECT PhMapRect(
    __in RECT InnerRect,
    __in RECT OuterRect
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
    __inout PPH_RECTANGLE Rectangle,
    __in PPH_RECTANGLE Bounds
    );

PHLIBAPI
VOID PhCenterRectangle(
    __inout PPH_RECTANGLE Rectangle,
    __in PPH_RECTANGLE Bounds
    );

PHLIBAPI
VOID PhAdjustRectangleToWorkingArea(
    __in HWND hWnd,
    __inout PPH_RECTANGLE Rectangle
    );

PHLIBAPI
VOID PhCenterWindow(
    __in HWND WindowHandle,
    __in_opt HWND ParentWindowHandle
    );

FORCEINLINE VOID PhLargeIntegerToSystemTime(
    __out PSYSTEMTIME SystemTime,
    __in PLARGE_INTEGER LargeInteger
    )
{
    FILETIME fileTime;

    fileTime.dwLowDateTime = LargeInteger->LowPart;
    fileTime.dwHighDateTime = LargeInteger->HighPart;
    FileTimeToSystemTime(&fileTime, SystemTime);
}

FORCEINLINE VOID PhLargeIntegerToLocalSystemTime(
    __out PSYSTEMTIME SystemTime,
    __in PLARGE_INTEGER LargeInteger
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
    __inout FILETIME Value1,
    __in FILETIME Value2
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
    __in_ecount(NumberOfObjects) PPVOID Objects,
    __in ULONG NumberOfObjects
    );

PHLIBAPI
VOID PhDereferenceObjects(
    __in_ecount(NumberOfObjects) PPVOID Objects,
    __in ULONG NumberOfObjects
    );

PHLIBAPI
PPH_STRING PhGetMessage(
    __in PVOID DllHandle,
    __in ULONG MessageTableId,
    __in ULONG MessageLanguageId,
    __in ULONG MessageId
    );

PHLIBAPI
PPH_STRING PhGetNtMessage(
    __in NTSTATUS Status
    );

PHLIBAPI
PPH_STRING PhGetWin32Message(
    __in ULONG Result
    );

#define PH_MAX_MESSAGE_SIZE 800

PHLIBAPI
INT PhShowMessage(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    ...
    );

PHLIBAPI
INT PhShowMessage_V(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    __in va_list ArgPtr
    );

#define PhShowError(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONERROR, Format, __VA_ARGS__)
#define PhShowWarning(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONWARNING, Format, __VA_ARGS__)
#define PhShowInformation(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONINFORMATION, Format, __VA_ARGS__)

PHLIBAPI
VOID PhShowStatus(
    __in HWND hWnd,
    __in_opt PWSTR Message,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    );

PHLIBAPI
BOOLEAN PhShowContinueStatus(
    __in HWND hWnd,
    __in_opt PWSTR Message,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    );

PHLIBAPI
BOOLEAN PhShowConfirmMessage(
    __in HWND hWnd,
    __in PWSTR Verb,
    __in PWSTR Object,
    __in_opt PWSTR Message,
    __in BOOLEAN Warning
    );

PHLIBAPI
BOOLEAN PhFindIntegerSiKeyValuePairs(
    __in PPH_KEY_VALUE_PAIR KeyValuePairs,
    __in ULONG SizeOfKeyValuePairs,
    __in PWSTR String,
    __out PULONG Integer
    );

PHLIBAPI
BOOLEAN PhFindStringSiKeyValuePairs(
    __in PPH_KEY_VALUE_PAIR KeyValuePairs,
    __in ULONG SizeOfKeyValuePairs,
    __in ULONG Integer,
    __out PWSTR *String
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
    __out PGUID Guid
    );

PHLIBAPI
VOID PhGenerateGuidFromName(
    __out PGUID Guid,
    __in PGUID Namespace,
    __in PCHAR Name,
    __in ULONG NameLength,
    __in UCHAR Version
    );

PHLIBAPI
VOID PhGenerateRandomAlphaString(
    __out_ecount_z(Count) PWSTR Buffer,
    __in ULONG Count
    );

PHLIBAPI
PPH_STRING PhEllipsisString(
    __in PPH_STRING String,
    __in ULONG DesiredCount
    );

PHLIBAPI
PPH_STRING PhEllipsisStringPath(
    __in PPH_STRING String,
    __in ULONG DesiredCount
    );

PHLIBAPI
BOOLEAN PhMatchWildcards(
    __in PWSTR Pattern,
    __in PWSTR String,
    __in BOOLEAN IgnoreCase
    );

PHLIBAPI
PPH_STRING PhEscapeStringForMenuPrefix(
    __in PPH_STRINGREF String
    );

PHLIBAPI
LONG PhCompareUnicodeStringZIgnoreMenuPrefix(
    __in PWSTR A,
    __in PWSTR B,
    __in BOOLEAN IgnoreCase,
    __in BOOLEAN MatchIfPrefix
    );

PHLIBAPI
PPH_STRING PhFormatDate(
    __in_opt PSYSTEMTIME Date,
    __in_opt PWSTR Format
    );

PHLIBAPI
PPH_STRING PhFormatTime(
    __in_opt PSYSTEMTIME Time,
    __in_opt PWSTR Format
    );

PHLIBAPI
PPH_STRING PhFormatDateTime(
    __in_opt PSYSTEMTIME DateTime
    );

FORCEINLINE PPH_STRING PhaFormatDateTime(
    __in_opt PSYSTEMTIME DateTime
    )
{
    return (PPH_STRING)PHA_DEREFERENCE(PhFormatDateTime(DateTime));
}

PHLIBAPI
PPH_STRING PhFormatTimeSpanRelative(
    __in ULONG64 TimeSpan
    );

PHLIBAPI
PPH_STRING PhFormatUInt64(
    __in ULONG64 Value,
    __in BOOLEAN GroupDigits
    );

#define PhaFormatUInt64(Value, GroupDigits) \
    ((PPH_STRING)PHA_DEREFERENCE(PhFormatUInt64((Value), (GroupDigits))))

PHLIBAPI
PPH_STRING PhFormatDecimal(
    __in PWSTR Value,
    __in ULONG FractionalDigits,
    __in BOOLEAN GroupDigits
    );

#define PhaFormatDecimal(Value, FractionalDigits, GroupDigits) \
    ((PPH_STRING)PHA_DEREFERENCE(PhFormatDecimal((Value), (FractionalDigits), (GroupDigits))))

PHLIBAPI
PPH_STRING PhFormatSize(
    __in ULONG64 Size,
    __in ULONG MaxSizeUnit
    );

#define PhaFormatSize(Size, MaxSizeUnit) \
    ((PPH_STRING)PHA_DEREFERENCE(PhFormatSize((Size), (MaxSizeUnit))))

PHLIBAPI
PPH_STRING PhFormatGuid(
    __in PGUID Guid
    );

PHLIBAPI
PVOID PhGetFileVersionInfo(
    __in PWSTR FileName
    );

PHLIBAPI
ULONG PhGetFileVersionInfoLangCodePage(
    __in PVOID VersionInfo
    );

PHLIBAPI
PPH_STRING PhGetFileVersionInfoString(
    __in PVOID VersionInfo,
    __in PWSTR SubBlock
    );

PHLIBAPI
PPH_STRING PhGetFileVersionInfoString2(
    __in PVOID VersionInfo,
    __in ULONG LangCodePage,
    __in PWSTR StringName
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
    __out PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    __in PWSTR FileName
    );

PHLIBAPI
VOID PhDeleteImageVersionInfo(
    __inout PPH_IMAGE_VERSION_INFO ImageVersionInfo
    );

PHLIBAPI
PPH_STRING PhFormatImageVersionInfo(
    __in_opt PPH_STRING FileName,
    __in PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    __in_opt PWSTR Indent,
    __in_opt ULONG LineLimit
    );

PHLIBAPI
PPH_STRING PhGetFullPath(
    __in PWSTR FileName,
    __out_opt PULONG IndexOfFileName
    );

PHLIBAPI
PPH_STRING PhExpandEnvironmentStrings(
    __in PPH_STRINGREF String
    );

PHLIBAPI
PPH_STRING PhGetBaseName(
    __in PPH_STRING FileName
    );

PHLIBAPI
PPH_STRING PhGetSystemDirectory();

PHLIBAPI
PPH_STRING PhGetSystemRoot();

PHLIBAPI
PLDR_DATA_TABLE_ENTRY PhFindLoaderEntry(
    __in_opt PVOID DllBase,
    __in_opt PPH_STRINGREF FullDllName,
    __in_opt PPH_STRINGREF BaseDllName
    );

PHLIBAPI
PPH_STRING PhGetDllFileName(
    __in PVOID DllHandle,
    __out_opt PULONG IndexOfFileName
    );

PHLIBAPI
PPH_STRING PhGetApplicationFileName();

PHLIBAPI
PPH_STRING PhGetApplicationDirectory();

PHLIBAPI
PPH_STRING PhGetKnownLocation(
    __in ULONG Folder,
    __in_opt PWSTR AppendPath
    );

PHLIBAPI
NTSTATUS PhWaitForMultipleObjectsAndPump(
    __in_opt HWND hWnd,
    __in ULONG NumberOfHandles,
    __in PHANDLE Handles,
    __in ULONG Timeout
    );

typedef struct _PH_CREATE_PROCESS_INFO
{
    PPH_STRINGREF DllPath;
    PPH_STRINGREF WindowTitle;
    PPH_STRINGREF DesktopInfo;
    PPH_STRINGREF ShellInfo;
    PPH_STRINGREF RuntimeData;
} PH_CREATE_PROCESS_INFO, *PPH_CREATE_PROCESS_INFO;

#define PH_CREATE_PROCESS_INHERIT_HANDLES 0x1
#define PH_CREATE_PROCESS_UNICODE_ENVIRONMENT 0x2
#define PH_CREATE_PROCESS_SUSPENDED 0x4
#define PH_CREATE_PROCESS_BREAKAWAY_FROM_JOB 0x8
#define PH_CREATE_PROCESS_NEW_CONSOLE 0x10

PHLIBAPI
NTSTATUS PhCreateProcess(
    __in PWSTR FileName,
    __in_opt PPH_STRINGREF CommandLine,
    __in_opt PVOID Environment,
    __in_opt PPH_STRINGREF CurrentDirectory,
    __in_opt PPH_CREATE_PROCESS_INFO Information,
    __in ULONG Flags,
    __in_opt HANDLE ParentProcessHandle,
    __out_opt PCLIENT_ID ClientId,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS PhCreateProcessWin32(
    __in_opt PWSTR FileName,
    __in_opt PWSTR CommandLine,
    __in_opt PVOID Environment,
    __in_opt PWSTR CurrentDirectory,
    __in ULONG Flags,
    __in_opt HANDLE TokenHandle,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS PhCreateProcessWin32Ex(
    __in_opt PWSTR FileName,
    __in_opt PWSTR CommandLine,
    __in_opt PVOID Environment,
    __in_opt PWSTR CurrentDirectory,
    __in_opt STARTUPINFO *StartupInfo,
    __in ULONG Flags,
    __in_opt HANDLE TokenHandle,
    __out_opt PCLIENT_ID ClientId,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
    );

typedef struct _PH_CREATE_PROCESS_AS_USER_INFO
{
    __in_opt PWSTR ApplicationName;
    __in_opt PWSTR CommandLine;
    __in_opt PWSTR CurrentDirectory;
    __in_opt PVOID Environment;
    __in_opt PWSTR DesktopName;
    __in_opt ULONG SessionId; // use PH_CREATE_PROCESS_SET_SESSION_ID
    union
    {
        struct
        {
            __in PWSTR DomainName;
            __in PWSTR UserName;
            __in PWSTR Password;
            __in_opt ULONG LogonType;
        };
        __in HANDLE ProcessIdWithToken; // use PH_CREATE_PROCESS_USE_PROCESS_TOKEN
        __in ULONG SessionIdWithToken; // use PH_CREATE_PROCESS_USE_SESSION_TOKEN
    };
} PH_CREATE_PROCESS_AS_USER_INFO, *PPH_CREATE_PROCESS_AS_USER_INFO;

#define PH_CREATE_PROCESS_USE_PROCESS_TOKEN 0x1000
#define PH_CREATE_PROCESS_USE_SESSION_TOKEN 0x2000
#define PH_CREATE_PROCESS_USE_LINKED_TOKEN 0x10000
#define PH_CREATE_PROCESS_SET_SESSION_ID 0x20000
#define PH_CREATE_PROCESS_WITH_PROFILE 0x40000

PHLIBAPI
NTSTATUS PhCreateProcessAsUser(
    __in PPH_CREATE_PROCESS_AS_USER_INFO Information,
    __in ULONG Flags,
    __out_opt PCLIENT_ID ClientId,
    __out_opt PHANDLE ProcessHandle,
    __out_opt PHANDLE ThreadHandle
    );

NTSTATUS PhFilterTokenForLimitedUser(
    __in HANDLE TokenHandle,
    __out PHANDLE NewTokenHandle
    );

PHLIBAPI
VOID PhShellExecute(
    __in HWND hWnd,
    __in PWSTR FileName,
    __in_opt PWSTR Parameters
    );

#define PH_SHELL_EXECUTE_ADMIN 0x1
#define PH_SHELL_EXECUTE_PUMP_MESSAGES 0x2

PHLIBAPI
BOOLEAN PhShellExecuteEx(
    __in HWND hWnd,
    __in PWSTR FileName,
    __in_opt PWSTR Parameters,
    __in ULONG ShowWindowType,
    __in ULONG Flags,
    __in_opt ULONG Timeout,
    __out_opt PHANDLE ProcessHandle
    );

PHLIBAPI
VOID PhShellExploreFile(
    __in HWND hWnd,
    __in PWSTR FileName
    );

PHLIBAPI
VOID PhShellProperties(
    __in HWND hWnd,
    __in PWSTR FileName
    );

PHLIBAPI
VOID PhShellOpenKey(
    __in HWND hWnd,
    __in PPH_STRING KeyName
    );

PKEY_VALUE_PARTIAL_INFORMATION PhQueryRegistryValue(
    __in HANDLE KeyHandle,
    __in_opt PWSTR ValueName
    );

PHLIBAPI
PPH_STRING PhQueryRegistryString(
    __in HANDLE KeyHandle,
    __in_opt PWSTR ValueName
    );

typedef struct _PH_FLAG_MAPPING
{
    ULONG Flag1;
    ULONG Flag2;
} PH_FLAG_MAPPING, *PPH_FLAG_MAPPING;

PHLIBAPI
VOID PhMapFlags1(
    __inout PULONG Value2,
    __in ULONG Value1,
    __in const PH_FLAG_MAPPING *Mappings,
    __in ULONG NumberOfMappings
    );

PHLIBAPI
VOID PhMapFlags2(
    __inout PULONG Value1,
    __in ULONG Value2,
    __in const PH_FLAG_MAPPING *Mappings,
    __in ULONG NumberOfMappings
    );

PHLIBAPI
PVOID PhCreateOpenFileDialog();

PHLIBAPI
PVOID PhCreateSaveFileDialog();

PHLIBAPI
VOID PhFreeFileDialog(
    __in PVOID FileDialog
    );

PHLIBAPI
BOOLEAN PhShowFileDialog(
    __in HWND hWnd,
    __in PVOID FileDialog
    );

#define PH_FILEDIALOG_CREATEPROMPT 0x1
#define PH_FILEDIALOG_PATHMUSTEXIST 0x2 // default both
#define PH_FILEDIALOG_FILEMUSTEXIST 0x4 // default open
#define PH_FILEDIALOG_SHOWHIDDEN 0x8
#define PH_FILEDIALOG_NODEREFERENCELINKS 0x10
#define PH_FILEDIALOG_OVERWRITEPROMPT 0x20 // default save
#define PH_FILEDIALOG_DEFAULTEXPANDED 0x40
#define PH_FILEDIALOG_STRICTFILETYPES 0x80

PHLIBAPI
ULONG PhGetFileDialogOptions(
    __in PVOID FileDialog
    );

PHLIBAPI
VOID PhSetFileDialogOptions(
    __in PVOID FileDialog,
    __in ULONG Options
    );

typedef struct _PH_FILETYPE_FILTER
{
    PWSTR Name;
    PWSTR Filter;
} PH_FILETYPE_FILTER, *PPH_FILETYPE_FILTER;

PHLIBAPI
VOID PhSetFileDialogFilter(
    __in PVOID FileDialog,
    __in PPH_FILETYPE_FILTER Filters,
    __in ULONG NumberOfFilters
    );

PHLIBAPI
PPH_STRING PhGetFileDialogFileName(
    __in PVOID FileDialog
    );

PHLIBAPI
VOID PhSetFileDialogFileName(
    __in PVOID FileDialog,
    __in PWSTR FileName
    );

PHLIBAPI
NTSTATUS PhIsExecutablePacked(
    __in PWSTR FileName,
    __out PBOOLEAN IsPacked,
    __out_opt PULONG NumberOfModules,
    __out_opt PULONG NumberOfFunctions
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
    __out PPH_HASH_CONTEXT Context,
    __in PH_HASH_ALGORITHM Algorithm
    );

PHLIBAPI
VOID PhUpdateHash(
    __inout PPH_HASH_CONTEXT Context,
    __in_bcount(Length) PVOID Buffer,
    __in ULONG Length
    );

PHLIBAPI
BOOLEAN PhFinalHash(
    __inout PPH_HASH_CONTEXT Context,
    __out_bcount(HashLength) PVOID Hash,
    __in ULONG HashLength,
    __out_opt PULONG ReturnLength
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
    __in_opt PPH_COMMAND_LINE_OPTION Option,
    __in_opt PPH_STRING Value,
    __in_opt PVOID Context
    );

#define PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS 0x1
#define PH_COMMAND_LINE_IGNORE_FIRST_PART 0x2

PHLIBAPI
PPH_STRING PhParseCommandLinePart(
    __in PPH_STRINGREF CommandLine,
    __inout PULONG Index
    );

PHLIBAPI
BOOLEAN PhParseCommandLine(
    __in PPH_STRINGREF CommandLine,
    __in_opt PPH_COMMAND_LINE_OPTION Options,
    __in ULONG NumberOfOptions,
    __in ULONG Flags,
    __in PPH_COMMAND_LINE_CALLBACK Callback,
    __in_opt PVOID Context
    );

PHLIBAPI
PPH_STRING PhEscapeCommandLinePart(
    __in PPH_STRINGREF String
    );

#ifdef __cplusplus
}
#endif

#endif
