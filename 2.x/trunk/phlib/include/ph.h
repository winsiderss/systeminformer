#ifndef _PH_PH_H
#define _PH_PH_H

#include <phbase.h>
#include <stdarg.h>
#include <dltmgr.h>

// data

#ifndef DATA_PRIVATE

// SIDs

extern SID PhSeNobodySid;

extern SID PhSeEveryoneSid;

extern SID PhSeLocalSid;

extern SID PhSeCreatorOwnerSid;
extern SID PhSeCreatorGroupSid;

extern SID PhSeDialupSid;
extern SID PhSeNetworkSid;
extern SID PhSeBatchSid;
extern SID PhSeInteractiveSid;
extern SID PhSeServiceSid;
extern SID PhSeAnonymousLogonSid;
extern SID PhSeProxySid;
extern SID PhSeAuthenticatedUserSid;
extern SID PhSeRestrictedCodeSid;
extern SID PhSeTerminalServerUserSid;
extern SID PhSeRemoteInteractiveLogonSid;
extern SID PhSeLocalSystemSid;
extern SID PhSeLocalServiceSid;
extern SID PhSeNetworkServiceSid;

// Strings

extern CHAR PhIntegerToChar[16];
extern CHAR PhIntegerToCharUpper[16];

extern WCHAR *PhKThreadStateNames[MaximumThreadState];
extern WCHAR *PhKWaitReasonNames[MaximumWaitReason];

#endif

// native

/** The PID of the idle process. */
#define SYSTEM_IDLE_PROCESS_ID ((HANDLE)0)
/** The PID of the system process. */
#define SYSTEM_PROCESS_ID ((HANDLE)4)

/** Specifies a PEB string. */
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

typedef struct _PH_PROCESS_WS_COUNTERS
{
    ULONG NumberOfPages;
    ULONG NumberOfPrivatePages;
    ULONG NumberOfSharedPages;
    ULONG NumberOfShareablePages;
} PH_PROCESS_WS_COUNTERS, *PPH_PROCESS_WS_COUNTERS;

/** Contains information about an environment variable. */
typedef struct _PH_ENVIRONMENT_VARIABLE
{
    /** A string containing the variable name. */
    PPH_STRING Name;
    /** A string containing the variable value. */
    PPH_STRING Value;
} PH_ENVIRONMENT_VARIABLE, *PPH_ENVIRONMENT_VARIABLE;

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

NTSTATUS PhOpenThreadProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ThreadHandle
    );

NTSTATUS PhOpenProcessToken(
    __out PHANDLE TokenHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessHandle
    );

NTSTATUS PhOpenThreadToken(
    __out PHANDLE TokenHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ThreadHandle,
    __in BOOLEAN OpenAsSelf
    );

NTSTATUS PhGetObjectSecurity(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

NTSTATUS PhSetObjectSecurity(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS PhTerminateProcess(
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    );

NTSTATUS PhSuspendProcess(
    __in HANDLE ProcessHandle
    );

NTSTATUS PhResumeProcess(
    __in HANDLE ProcessHandle
    );

NTSTATUS PhTerminateThread(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

NTSTATUS PhSuspendThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

NTSTATUS PhResumeThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

NTSTATUS PhGetThreadContext(
    __in HANDLE ThreadHandle,
    __inout PCONTEXT Context
    );

NTSTATUS PhSetThreadContext(
    __in HANDLE ThreadHandle,
    __in PCONTEXT Context
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

NTSTATUS PhGetProcessExtendedBasicInformation(
    __in HANDLE ProcessHandle,
    __out PPROCESS_EXTENDED_BASIC_INFORMATION ExtendedBasicInformation
    );

NTSTATUS PhGetProcessTimes(
    __in HANDLE ProcessHandle,
    __out PKERNEL_USER_TIMES Times
    );

NTSTATUS PhGetProcessImageFileName(
    __in HANDLE ProcessHandle,
    __out PPH_STRING *FileName
    );

NTSTATUS PhGetProcessImageFileNameWin32(
    __in HANDLE ProcessHandle,
    __out PPH_STRING *FileName
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

NTSTATUS PhGetProcessPebString(
    __in HANDLE ProcessHandle,
    __in PH_PEB_OFFSET Offset,
    __out PPH_STRING *String
    );

NTSTATUS PhGetProcessSessionId(
    __in HANDLE ProcessHandle,
    __out PULONG SessionId
    );

NTSTATUS PhGetProcessExecuteFlags(
    __in HANDLE ProcessHandle,
    __out PULONG ExecuteFlags
    );

NTSTATUS PhGetProcessIsWow64(
    __in HANDLE ProcessHandle,
    __out PBOOLEAN IsWow64
    );

NTSTATUS PhGetProcessPeb32(
    __in HANDLE ProcessHandle,
    __out PPVOID Peb32
    );

NTSTATUS PhGetProcessIsBeingDebugged(
    __in HANDLE ProcessHandle,
    __out PBOOLEAN IsBeingDebugged
    );

NTSTATUS PhGetProcessDebugObject(
    __in HANDLE ProcessHandle,
    __out PHANDLE DebugObjectHandle
    );

NTSTATUS PhGetProcessIoPriority(
    __in HANDLE ProcessHandle,
    __out PULONG IoPriority
    );

NTSTATUS PhGetProcessPagePriority(
    __in HANDLE ProcessHandle,
    __out PULONG PagePriority
    );

NTSTATUS PhGetProcessIsPosix(
    __in HANDLE ProcessHandle,
    __out PBOOLEAN IsPosix
    );

NTSTATUS PhGetProcessCycleTime(
    __in HANDLE ProcessHandle,
    __out PULONG64 CycleTime
    );

NTSTATUS PhGetProcessConsoleHostProcessId(
    __in HANDLE ProcessHandle,
    __out PHANDLE ConsoleHostProcessId
    );

#define PH_PROCESS_DEP_ENABLED 0x1
#define PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED 0x2
#define PH_PROCESS_DEP_PERMANENT 0x4

NTSTATUS PhGetProcessDepStatus(
    __in HANDLE ProcessHandle,
    __out PULONG DepStatus
    );

NTSTATUS PhGetProcessPosixCommandLine(
    __in HANDLE ProcessHandle,
    __out PPH_STRING *CommandLine
    );

NTSTATUS PhGetProcessEnvironmentVariables(
    __in HANDLE ProcessHandle,
    __out PPH_ENVIRONMENT_VARIABLE *Variables,
    __out PULONG NumberOfVariables
    );

VOID PhFreeProcessEnvironmentVariables(
    __in PPH_ENVIRONMENT_VARIABLE Variables,
    __in ULONG NumberOfVariables
    );

NTSTATUS PhGetProcessMappedFileName(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out PPH_STRING *FileName
    );

NTSTATUS PhGetProcessWorkingSetInformation(
    __in HANDLE ProcessHandle,
    __out PMEMORY_WORKING_SET_INFORMATION *WorkingSetInformation
    );

NTSTATUS PhGetProcessWsCounters(
    __in HANDLE ProcessHandle,
    __out PPH_PROCESS_WS_COUNTERS WsCounters
    );

typedef struct _PROCESS_HANDLE_INFORMATION *PPROCESS_HANDLE_INFORMATION;

NTSTATUS PhEnumProcessHandles(
    __in HANDLE ProcessHandle,
    __out PPROCESS_HANDLE_INFORMATION *Handles
    );

NTSTATUS PhSetProcessAffinityMask(
    __in HANDLE ProcessHandle,
    __in ULONG_PTR AffinityMask
    );

NTSTATUS PhSetProcessIoPriority(
    __in HANDLE ProcessHandle,
    __in ULONG IoPriority
    );

NTSTATUS PhSetProcessExecuteFlags(
    __in HANDLE ProcessHandle,
    __in ULONG ExecuteFlags
    );

NTSTATUS PhSetProcessDepStatus(
    __in HANDLE ProcessHandle,
    __in ULONG DepStatus
    );

NTSTATUS PhSetProcessDepStatusInvasive(
    __in HANDLE ProcessHandle,
    __in ULONG DepStatus,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSTATUS PhInjectDllProcess(
    __in HANDLE ProcessHandle,
    __in PWSTR FileName,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSTATUS PhUnloadDllProcess(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSTATUS PhGetThreadBasicInformation(
    __in HANDLE ThreadHandle,
    __out PTHREAD_BASIC_INFORMATION BasicInformation
    );

NTSTATUS PhGetThreadIoPriority(
    __in HANDLE ThreadHandle,
    __out PULONG IoPriority
    );

NTSTATUS PhGetThreadPagePriority(
    __in HANDLE ThreadHandle,
    __out PULONG PagePriority
    );

NTSTATUS PhGetThreadCycleTime(
    __in HANDLE ThreadHandle,
    __out PULONG64 CycleTime
    );

NTSTATUS PhSetThreadIoPriority(
    __in HANDLE ThreadHandle,
    __in ULONG IoPriority
    );

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
    __in PVOID Context
    );

NTSTATUS PhWalkThreadStack(
    __in HANDLE ThreadHandle,
    __in_opt HANDLE ProcessHandle,
    __in ULONG Flags,
    __in PPH_WALK_THREAD_STACK_CALLBACK Callback,
    __in PVOID Context
    );

NTSTATUS PhGetJobBasicAndIoAccounting(
    __in HANDLE JobHandle,
    __out PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION BasicAndIoAccounting
    );

NTSTATUS PhGetJobBasicLimits(
    __in HANDLE JobHandle,
    __out PJOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimits
    );

NTSTATUS PhGetJobExtendedLimits(
    __in HANDLE JobHandle,
    __out PJOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimits
    );

NTSTATUS PhGetJobBasicUiRestrictions(
    __in HANDLE JobHandle,
    __out PJOBOBJECT_BASIC_UI_RESTRICTIONS BasicUiRestrictions
    );

NTSTATUS PhGetJobProcessIdList(
    __in HANDLE JobHandle,
    __out PJOBOBJECT_BASIC_PROCESS_ID_LIST *ProcessIdList
    );

NTSTATUS PhGetTokenUser(
    __in HANDLE TokenHandle,
    __out PTOKEN_USER *User
    );

NTSTATUS PhGetTokenOwner(
    __in HANDLE TokenHandle,
    __out PTOKEN_OWNER *Owner
    );

NTSTATUS PhGetTokenPrimaryGroup(
    __in HANDLE TokenHandle,
    __out PTOKEN_PRIMARY_GROUP *PrimaryGroup
    );

NTSTATUS PhGetTokenSessionId(
    __in HANDLE TokenHandle,
    __out PULONG SessionId
    );

NTSTATUS PhGetTokenElevationType(
    __in HANDLE TokenHandle,
    __out PTOKEN_ELEVATION_TYPE ElevationType
    );

NTSTATUS PhGetTokenIsElevated(
    __in HANDLE TokenHandle,
    __out PBOOLEAN Elevated
    );

NTSTATUS PhGetTokenStatistics(
    __in HANDLE TokenHandle,
    __out PTOKEN_STATISTICS Statistics
    );

NTSTATUS PhGetTokenSource(
    __in HANDLE TokenHandle,
    __out PTOKEN_SOURCE Source
    );

NTSTATUS PhGetTokenGroups(
    __in HANDLE TokenHandle,
    __out PTOKEN_GROUPS *Groups
    );

NTSTATUS PhGetTokenPrivileges(
    __in HANDLE TokenHandle,
    __out PTOKEN_PRIVILEGES *Privileges
    );

NTSTATUS PhGetTokenLinkedToken(
    __in HANDLE TokenHandle,
    __out PHANDLE LinkedTokenHandle
    );

NTSTATUS PhGetTokenIsVirtualizationAllowed(
    __in HANDLE TokenHandle,
    __out PBOOLEAN IsVirtualizationAllowed
    );

NTSTATUS PhGetTokenIsVirtualizationEnabled(
    __in HANDLE TokenHandle,
    __out PBOOLEAN IsVirtualizationEnabled
    );

NTSTATUS PhSetTokenSessionId(
    __in HANDLE TokenHandle,
    __in ULONG SessionId
    );

BOOLEAN PhSetTokenPrivilege(
    __in HANDLE TokenHandle,
    __in_opt PWSTR PrivilegeName,
    __in_opt PLUID PrivilegeLuid,
    __in ULONG Attributes
    );

NTSTATUS PhSetTokenIsVirtualizationEnabled(
    __in HANDLE TokenHandle,
    __in BOOLEAN IsVirtualizationEnabled
    );

NTSTATUS PhGetTokenIntegrityLevel(
    __in HANDLE TokenHandle,
    __out_opt PPH_INTEGRITY IntegrityLevel, 
    __out_opt PPH_STRING *IntegrityString
    );

NTSTATUS PhGetEventBasicInformation(
    __in HANDLE EventHandle,
    __out PEVENT_BASIC_INFORMATION BasicInformation
    );

NTSTATUS PhGetMutantBasicInformation(
    __in HANDLE MutantHandle,
    __out PMUTANT_BASIC_INFORMATION BasicInformation
    );

NTSTATUS PhGetMutantOwnerInformation(
    __in HANDLE MutantHandle,
    __out PMUTANT_OWNER_INFORMATION OwnerInformation
    );

NTSTATUS PhGetSectionBasicInformation(
    __in HANDLE SectionHandle,
    __out PSECTION_BASIC_INFORMATION BasicInformation
    );

NTSTATUS PhGetSemaphoreBasicInformation(
    __in HANDLE SemaphoreHandle,
    __out PSEMAPHORE_BASIC_INFORMATION BasicInformation
    );

NTSTATUS PhGetTimerBasicInformation(
    __in HANDLE TimerHandle,
    __out PTIMER_BASIC_INFORMATION BasicInformation
    );

NTSTATUS PhGetFileSize(
    __in HANDLE FileHandle,
    __out PLARGE_INTEGER Size
    );

NTSTATUS PhSetFileSize(
    __in HANDLE FileHandle,
    __in PLARGE_INTEGER Size
    );

NTSTATUS PhGetTransactionManagerBasicInformation(
    __in HANDLE TransactionManagerHandle,
    __out PTRANSACTIONMANAGER_BASIC_INFORMATION BasicInformation
    );

NTSTATUS PhGetTransactionManagerLogFileName(
    __in HANDLE TransactionManagerHandle,
    __out PPH_STRING *LogFileName
    );

NTSTATUS PhGetTransactionBasicInformation(
    __in HANDLE TransactionHandle,
    __out PTRANSACTION_BASIC_INFORMATION BasicInformation
    );

NTSTATUS PhGetTransactionPropertiesInformation(
    __in HANDLE TransactionHandle,
    __out_opt PLARGE_INTEGER Timeout,
    __out_opt TRANSACTION_OUTCOME *Outcome,
    __out_opt PPH_STRING *Description
    );

NTSTATUS PhGetResourceManagerBasicInformation(
    __in HANDLE ResourceManagerHandle,
    __out_opt PGUID Guid,
    __out_opt PPH_STRING *Description
    );

NTSTATUS PhGetEnlistmentBasicInformation(
    __in HANDLE EnlistmentHandle,
    __out PENLISTMENT_BASIC_INFORMATION BasicInformation
    );

NTSTATUS PhOpenDriverByBaseAddress(
    __out PHANDLE DriverHandle,
    __in PVOID BaseAddress
    );

NTSTATUS PhGetDriverServiceKeyName(
    __in HANDLE DriverHandle,
    __out PPH_STRING *ServiceKeyName
    );

NTSTATUS PhUnloadDriver(
    __in_opt PVOID BaseAddress,
    __in_opt PWSTR Name
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
    __in PVOID Context
    );

NTSTATUS PhEnumProcessModules(
    __in HANDLE ProcessHandle,
    __in PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    __in PVOID Context
    );

NTSTATUS PhSetProcessModuleLoadCount(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in USHORT LoadCount
    );

PPH_STRING PhGetKernelFileName();

NTSTATUS PhEnumKernelModules(
    __out PRTL_PROCESS_MODULES *Modules
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

NTSTATUS PhEnumProcesses(
    __out PPVOID Processes
    );

PSYSTEM_PROCESS_INFORMATION PhFindProcessInformation(
    __in PVOID Processes,
    __in HANDLE ProcessId
    );

NTSTATUS PhEnumHandles(
    __out PSYSTEM_HANDLE_INFORMATION *Handles
    );

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

NTSTATUS PhEnumPagefiles(
    __out PPVOID Pagefiles
    );

NTSTATUS PhGetProcessImageFileNameByProcessId(
    __in HANDLE ProcessId,
    __out PPH_STRING *FileName
    );

NTSTATUS PhGetProcessIsDotNet(
    __in HANDLE ProcessId,
    __out PBOOLEAN IsDotNet
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
    __in PVOID Context
    );

NTSTATUS PhEnumDirectoryObjects(
    __in HANDLE DirectoryHandle,
    __in PPH_ENUM_DIRECTORY_OBJECTS Callback,
    __in PVOID Context
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_DIRECTORY_FILE)(
    __in PFILE_DIRECTORY_INFORMATION Information,
    __in PVOID Context
    );

NTSTATUS PhEnumDirectoryFile(
    __in HANDLE FileHandle,
    __in_opt PPH_STRINGREF SearchPattern,
    __in PPH_ENUM_DIRECTORY_FILE Callback,
    __in PVOID Context
    );

#define PH_FIRST_STREAM(Streams) ((PFILE_STREAM_INFORMATION)(Streams))
#define PH_NEXT_STREAM(Stream) ( \
    ((PFILE_STREAM_INFORMATION)(Stream))->NextEntryOffset ? \
    (PFILE_STREAM_INFORMATION)((PCHAR)(Stream) + \
    ((PFILE_STREAM_INFORMATION)(Stream))->NextEntryOffset) : \
    NULL \
    )

NTSTATUS PhEnumFileStreams(
    __in HANDLE FileHandle,
    __out PPVOID Streams
    );

VOID PhInitializeDevicePrefixes();

VOID PhRefreshMupDevicePrefixes();

VOID PhRefreshDosDevicePrefixes();

PPH_STRING PhResolveDevicePrefix(
    __in PPH_STRING Name
    );

PPH_STRING PhGetFileName(
    __in PPH_STRING FileName
    );

#define PH_MODULE_TYPE_MODULE 1
#define PH_MODULE_TYPE_MAPPED_FILE 2
#define PH_MODULE_TYPE_WOW64_MODULE 3
#define PH_MODULE_TYPE_KERNEL_MODULE 4

typedef struct _PH_MODULE_INFO
{
    ULONG Type;
    PVOID BaseAddress;
    ULONG Size;
    PVOID EntryPoint;
    ULONG Flags;
    PPH_STRING Name;
    PPH_STRING FileName;
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
    __in PVOID Context
    );

#define PH_ENUM_GENERIC_MAPPED_FILES 0x1

NTSTATUS PhEnumGenericModules(
    __in HANDLE ProcessId,
    __in_opt HANDLE ProcessHandle,
    __in ULONG Flags,
    __in PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    __in PVOID Context
    );

// lsa

NTSTATUS PhOpenLsaPolicy(
    __out PLSA_HANDLE PolicyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt PUNICODE_STRING SystemName
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

NTSTATUS PhLookupSid(
    __in PSID Sid,
    __out_opt PPH_STRING *Name,
    __out_opt PPH_STRING *DomainName,
    __out_opt PSID_NAME_USE NameUse
    );

PPH_STRING PhGetSidFullName(
    __in PSID Sid,
    __in BOOLEAN IncludeDomain,
    __out_opt PSID_NAME_USE NameUse
    );

PPH_STRING PhSidToStringSid(
    __in PSID Sid
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_ACCOUNTS_CALLBACK)(
    __in PSID Sid,
    __in PVOID Context
    );

NTSTATUS PhEnumAccounts(
    __in LSA_HANDLE PolicyHandle,
    __in PPH_ENUM_ACCOUNTS_CALLBACK Callback,
    __in PVOID Context
    );

// hndlinfo

VOID PhHandleInfoInitialization();

typedef PPH_STRING (NTAPI *PPH_GET_CLIENT_ID_NAME)(
    __in PCLIENT_ID ClientId
    );

PPH_GET_CLIENT_ID_NAME PhSetHandleClientIdFunction(
    __in PPH_GET_CLIENT_ID_NAME GetClientIdName
    );

PPH_STRING PhFormatNativeKeyName(
    __in PPH_STRING Name
    );

__callback PPH_STRING PhStdGetClientIdName(
    __in PCLIENT_ID ClientId
    );

NTSTATUS PhGetHandleInformation(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG ObjectTypeNumber,
    __out_opt POBJECT_BASIC_INFORMATION BasicInformation,
    __out_opt PPH_STRING *TypeName,
    __out_opt PPH_STRING *ObjectName,
    __out_opt PPH_STRING *BestObjectName
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

NTSTATUS PhInitializeMappedImage(
    __out PPH_MAPPED_IMAGE MappedImage,
    __in PVOID ViewBase,
    __in SIZE_T Size
    );

NTSTATUS PhLoadMappedImage(
    __in_opt PWSTR FileName,
    __in_opt HANDLE FileHandle,
    __in BOOLEAN ReadOnly,
    __out PPH_MAPPED_IMAGE MappedImage
    );

NTSTATUS PhUnloadMappedImage(
    __inout PPH_MAPPED_IMAGE MappedImage
    );

NTSTATUS PhMapViewOfEntireFile(
    __in_opt PWSTR FileName,
    __in_opt HANDLE FileHandle,
    __in BOOLEAN ReadOnly,
    __out PPVOID ViewBase,
    __out PSIZE_T Size
    );

PIMAGE_SECTION_HEADER PhMappedImageRvaToSection(
    __in PPH_MAPPED_IMAGE MappedImage,
    __in ULONG Rva
    );

PVOID PhMappedImageRvaToVa(
    __in PPH_MAPPED_IMAGE MappedImage,
    __in ULONG Rva,
    __out_opt PIMAGE_SECTION_HEADER *Section
    );

BOOLEAN PhGetMappedImageSectionName(
    __in PIMAGE_SECTION_HEADER Section,
    __out_ecount_z_opt(Count) PSTR Buffer,
    __in ULONG Count,
    __out_opt PULONG ReturnCount
    );

NTSTATUS PhGetMappedImageDataEntry(
    __in PPH_MAPPED_IMAGE MappedImage,
    __in ULONG Index,
    __out PIMAGE_DATA_DIRECTORY *Entry
    );

NTSTATUS PhGetMappedImageLoadConfig32(
    __in PPH_MAPPED_IMAGE MappedImage,
    __out PIMAGE_LOAD_CONFIG_DIRECTORY32 *LoadConfig
    );

NTSTATUS PhGetMappedImageLoadConfig64(
    __in PPH_MAPPED_IMAGE MappedImage,
    __out PIMAGE_LOAD_CONFIG_DIRECTORY64 *LoadConfig
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

NTSTATUS PhInitializeMappedImageExports(
    __out PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in PPH_MAPPED_IMAGE MappedImage
    );

NTSTATUS PhGetMappedImageExportEntry(
    __in PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in ULONG Index,
    __out PPH_MAPPED_IMAGE_EXPORT_ENTRY Entry
    );

NTSTATUS PhGetMappedImageExportFunction(
    __in PPH_MAPPED_IMAGE_EXPORTS Exports,
    __in_opt PSTR Name,
    __in_opt USHORT Ordinal,
    __out PPH_MAPPED_IMAGE_EXPORT_FUNCTION Function
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

NTSTATUS PhInitializeMappedImageImports(
    __out PPH_MAPPED_IMAGE_IMPORTS Imports,
    __in PPH_MAPPED_IMAGE MappedImage
    );

NTSTATUS PhGetMappedImageImportDll(
    __in PPH_MAPPED_IMAGE_IMPORTS Imports,
    __in ULONG Index,
    __out PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll
    );

NTSTATUS PhGetMappedImageImportEntry(
    __in PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll,
    __in ULONG Index,
    __out PPH_MAPPED_IMAGE_IMPORT_ENTRY Entry
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

NTSTATUS PhInitializeMappedArchive(
    __out PPH_MAPPED_ARCHIVE MappedArchive,
    __in PVOID ViewBase,
    __in SIZE_T Size
    );

NTSTATUS PhLoadMappedArchive(
    __in_opt PWSTR FileName,
    __in_opt HANDLE FileHandle,
    __in BOOLEAN ReadOnly,
    __out PPH_MAPPED_ARCHIVE MappedArchive
    );

NTSTATUS PhUnloadMappedArchive(
    __inout PPH_MAPPED_ARCHIVE MappedArchive
    );

NTSTATUS PhGetNextMappedArchiveMember(
    __in PPH_MAPPED_ARCHIVE_MEMBER Member,
    __out PPH_MAPPED_ARCHIVE_MEMBER NextMember
    );

BOOLEAN PhIsMappedArchiveMemberShortFormat(
    __in PPH_MAPPED_ARCHIVE_MEMBER Member
    );

NTSTATUS PhGetMappedArchiveImportEntry(
    __in PPH_MAPPED_ARCHIVE_MEMBER Member,
    __out PPH_MAPPED_ARCHIVE_IMPORT_ENTRY Entry
    );

// iosup

BOOLEAN PhIoSupportInitialization();

NTSTATUS PhCreateFileWin32(
    __out PHANDLE FileHandle,
    __in PWSTR FileName,
    __in ACCESS_MASK DesiredAccess,
    __in_opt ULONG FileAttributes,
    __in ULONG ShareAccess,
    __in ULONG CreateDisposition,
    __in ULONG CreateOptions
    );

NTSTATUS PhDeleteFileWin32(
    __in PWSTR FileName
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

// Internal flags
/** Indicates that at least one write has been issued to the file handle. */
#define PH_FILE_STREAM_WRITTEN 0x00010000

// Higher-level flags (PhCreateFileStream)
#define PH_FILE_STREAM_APPEND 0x01000000

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

NTSTATUS PhCreateFileStream(
    __out PPH_FILE_STREAM *FileStream,
    __in PWSTR FileName,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG ShareMode,
    __in ULONG CreateDisposition,
    __in ULONG Flags
    );

NTSTATUS PhCreateFileStream2(
    __out PPH_FILE_STREAM *FileStream,
    __in HANDLE FileHandle,
    __in ULONG Flags,
    __in ULONG BufferLength
    );

VOID PhFileStreamVerify(
    __in PPH_FILE_STREAM FileStream
    );

NTSTATUS PhFileStreamRead(
    __inout PPH_FILE_STREAM FileStream,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __out_opt PULONG ReadLength
    );

NTSTATUS PhFileStreamWrite(
    __inout PPH_FILE_STREAM FileStream,
    __in_bcount(Length) PVOID Buffer,
    __in ULONG Length
    );

NTSTATUS PhFileStreamFlush(
    __inout PPH_FILE_STREAM FileStream,
    __in BOOLEAN Full
    );

NTSTATUS PhFileStreamSeek(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Offset,
    __in PH_SEEK_ORIGIN Origin
    );

NTSTATUS PhFileStreamLock(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Position,
    __in PLARGE_INTEGER Length,
    __in BOOLEAN Wait,
    __in BOOLEAN Shared
    );

NTSTATUS PhFileStreamUnlock(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Position,
    __in PLARGE_INTEGER Length
    );

#define PH_FILE_STREAM_STRING_BLOCK_SIZE (PAGE_SIZE / 2)

NTSTATUS PhFileStreamWriteStringAsAnsi(
    __inout PPH_FILE_STREAM FileStream,
    __in PPH_STRINGREF String
    );

NTSTATUS PhFileStreamWriteStringAsAnsi2(
    __inout PPH_FILE_STREAM FileStream,
    __in PWSTR String
    );

NTSTATUS PhFileStreamWriteStringFormat_V(
    __inout PPH_FILE_STREAM FileStream,
    __in __format_string PWSTR Format,
    __in va_list ArgPtr
    );

NTSTATUS PhFileStreamWriteStringFormat(
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

VOID PhVerifyInitialization();

VERIFY_RESULT PhVerifyFile(
    __in PWSTR FileName,
    __out_opt PPH_STRING *SignerName
    );

// provider

#if !defined(PROVIDER_PRIVATE) && defined(DEBUG)
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
    __in PPH_PROVIDER_FUNCTION Function,
    __in PVOID Object,
    __out PPH_PROVIDER_REGISTRATION Registration
    );

VOID PhUnregisterProvider(
    __inout PPH_PROVIDER_REGISTRATION Registration
    );

BOOLEAN PhBoostProvider(
    __inout PPH_PROVIDER_REGISTRATION Registration,
    __out_opt PULONG FutureRunId
    );

ULONG PhGetProviderRunId(
    __in PPH_PROVIDER_REGISTRATION Registration
    );

VOID PhSetProviderEnabled(
    __in PPH_PROVIDER_REGISTRATION Registration,
    __in BOOLEAN Enabled
    );

// symprv

#ifndef SYMPRV_PRIVATE
extern PPH_OBJECT_TYPE PhSymbolProviderType;
#endif

//#define PH_SYMBOL_PROVIDER_DELAY_INIT

#define PH_MAX_SYMBOL_NAME_LEN 128

typedef struct _PH_SYMBOL_PROVIDER
{
    PPH_LIST ModulesList;
    PH_QUEUED_LOCK ModulesListLock;
    HANDLE ProcessHandle;
    BOOLEAN IsRealHandle;
#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    PH_INITONCE InitOnce;
#endif
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

PPH_SYMBOL_PROVIDER PhCreateSymbolProvider(
    __in_opt HANDLE ProcessId
    );

BOOLEAN PhGetLineFromAddress(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address,
    __out PPH_STRING *FileName,
    __out_opt PULONG Displacement,
    __out_opt PPH_SYMBOL_LINE_IINFORMATION Information
    );

ULONG64 PhGetModuleFromAddress(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address,
    __out_opt PPH_STRING *FileName
    );

PPH_STRING PhGetSymbolFromAddress(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address,
    __out_opt PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel,
    __out_opt PPH_STRING *FileName,
    __out_opt PPH_STRING *SymbolName,
    __out_opt PULONG64 Displacement
    );

BOOLEAN PhGetSymbolFromName(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in PWSTR Name,
    __out PPH_SYMBOL_INFORMATION Information
    );

BOOLEAN PhSymbolProviderLoadModule(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in PWSTR FileName,
    __in ULONG64 BaseAddress,
    __in ULONG Size
    );

VOID PhSymbolProviderSetSearchPath(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in PWSTR SearchPath
    );

// svcsup

#ifndef SVCSUP_PRIVATE
extern WCHAR *PhServiceTypeStrings[6];
extern WCHAR *PhServiceStartTypeStrings[5];
extern WCHAR *PhServiceErrorControlStrings[4];
#endif

PVOID PhEnumServices(
    __in SC_HANDLE ScManagerHandle,
    __in_opt ULONG Type,
    __in_opt ULONG State,
    __out PULONG Count
    );

SC_HANDLE PhOpenService(
    __in PWSTR ServiceName,
    __in ACCESS_MASK DesiredAccess
    );

PVOID PhGetServiceConfig(
    __in SC_HANDLE ServiceHandle
    );

PPH_STRING PhGetServiceDescription(
    __in SC_HANDLE ServiceHandle
    );

PWSTR PhGetServiceStateString(
    __in ULONG ServiceState
    );

PWSTR PhGetServiceTypeString(
    __in ULONG ServiceType
    );

ULONG PhGetServiceTypeInteger(
    __in PWSTR ServiceType
    );

PWSTR PhGetServiceStartTypeString(
    __in ULONG ServiceStartType
    );

ULONG PhGetServiceStartTypeInteger(
    __in PWSTR ServiceStartType
    );

PWSTR PhGetServiceErrorControlString(
    __in ULONG ServiceErrorControl
    );

ULONG PhGetServiceErrorControlInteger(
    __in PWSTR ServiceErrorControl
    );

PPH_STRING PhGetServiceNameFromTag(
    __in HANDLE ProcessId,
    __in PVOID ServiceTag
    );

NTSTATUS PhGetThreadServiceTag(
    __in HANDLE ThreadHandle,
    __in_opt HANDLE ProcessHandle,
    __out PPVOID ServiceTag
    );

// support

#ifndef SUPPORT_PRIVATE
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

VOID PhAdjustRectangleToBounds(
    __inout PPH_RECTANGLE Rectangle,
    __in PPH_RECTANGLE Bounds
    );

VOID PhCenterRectangle(
    __inout PPH_RECTANGLE Rectangle,
    __in PPH_RECTANGLE Bounds
    );

VOID PhAdjustRectangleToWorkingArea(
    __in HWND hWnd,
    __inout PPH_RECTANGLE Rectangle
    );

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

VOID PhReferenceObjects(
    __in PPVOID Objects,
    __in ULONG NumberOfObjects
    );

VOID PhDereferenceObjects(
    __in PPVOID Objects,
    __in ULONG NumberOfObjects
    );

PPH_STRING PhGetMessage(
    __in PVOID DllHandle,
    __in ULONG MessageTableId,
    __in ULONG MessageLanguageId,
    __in ULONG MessageId
    );

PPH_STRING PhGetNtMessage(
    __in NTSTATUS Status
    );

PPH_STRING PhGetWin32Message(
    __in ULONG Result
    );

#define PH_MAX_MESSAGE_SIZE 800

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
#define PhShowWarning(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONWARNING, Format, __VA_ARGS__)
#define PhShowInformation(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONINFORMATION, Format, __VA_ARGS__)

VOID PhShowStatus(
    __in HWND hWnd,
    __in_opt PWSTR Message,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    );

BOOLEAN PhShowContinueStatus(
    __in HWND hWnd,
    __in_opt PWSTR Message,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    );

BOOLEAN PhShowConfirmMessage(
    __in HWND hWnd,
    __in PWSTR Verb,
    __in PWSTR Object,
    __in_opt PWSTR Message,
    __in BOOLEAN Warning
    );

BOOLEAN PhFindIntegerSiKeyValuePairs(
    __in PPH_KEY_VALUE_PAIR KeyValuePairs,
    __in ULONG SizeOfKeyValuePairs,
    __in PWSTR String,
    __out PULONG Integer
    );

BOOLEAN PhFindStringSiKeyValuePairs(
    __in PPH_KEY_VALUE_PAIR KeyValuePairs,
    __in ULONG SizeOfKeyValuePairs,
    __in ULONG Integer,
    __out PWSTR *String
    );

VOID PhGenerateGuid(
    __out PGUID Guid
    );

VOID PhGenerateRandomAlphaString(
    __out_ecount_z(Count) PWSTR Buffer,
    __in ULONG Count
    );

PPH_STRING PhEllipsisString(
    __in PPH_STRING String,
    __in ULONG DesiredCount
    );

PPH_STRING PhEllipsisStringPath(
    __in PPH_STRING String,
    __in ULONG DesiredCount
    );

PPH_STRING PhFormatDate(
    __in_opt PSYSTEMTIME Date,
    __in_opt PWSTR Format
    );

PPH_STRING PhFormatTime(
    __in_opt PSYSTEMTIME Time,
    __in_opt PWSTR Format
    );

FORCEINLINE PPH_STRING PhFormatDateTime(
    __in_opt PSYSTEMTIME DateTime
    )
{
    PPH_STRING dateTimeString;
    PPH_STRING timeString;
    PPH_STRING dateString;

    timeString = PhFormatTime(DateTime, NULL);
    dateString = PhFormatDate(DateTime, NULL);

    dateTimeString = PhConcatStrings(3, timeString->Buffer, L" ", dateString->Buffer);

    PhDereferenceObject(dateString);
    PhDereferenceObject(timeString);

    return dateTimeString;
}

FORCEINLINE PPH_STRING PhaFormatDateTime(
    __in_opt PSYSTEMTIME DateTime
    )
{
    return PHA_DEREFERENCE(PhFormatDateTime(DateTime));
}

PPH_STRING PhFormatUInt64(
    __in ULONG64 Value,
    __in BOOLEAN GroupDigits
    );

#define PhaFormatUInt64(Value, GroupDigits) \
    ((PPH_STRING)PHA_DEREFERENCE(PhFormatUInt64((Value), (GroupDigits))))

PPH_STRING PhFormatDecimal(
    __in PWSTR Value,
    __in ULONG FractionalDigits,
    __in BOOLEAN GroupDigits
    );

#define PhaFormatDecimal(Value, FractionalDigits, GroupDigits) \
    ((PPH_STRING)PHA_DEREFERENCE(PhFormatDecimal((Value), (FractionalDigits), (GroupDigits))))

PPH_STRING PhFormatSize(
    __in ULONG64 Size,
    __in ULONG MaxSizeUnit
    );

#define PhaFormatSize(Size, MaxSizeUnit) \
    ((PPH_STRING)PHA_DEREFERENCE(PhFormatSize((Size), (MaxSizeUnit))))

PPH_STRING PhFormatGuid(
    __in PGUID Guid
    );

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

typedef struct _PH_IMAGE_VERSION_INFO
{
    PPH_STRING CompanyName;
    PPH_STRING FileDescription;
    PPH_STRING FileVersion;
    PPH_STRING ProductName;
} PH_IMAGE_VERSION_INFO, *PPH_IMAGE_VERSION_INFO;

BOOLEAN PhInitializeImageVersionInfo(
    __out PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    __in PWSTR FileName
    );

VOID PhDeleteImageVersionInfo(
    __inout PPH_IMAGE_VERSION_INFO ImageVersionInfo
    );

PPH_STRING PhFormatImageVersionInfo(
    __in_opt PPH_STRING FileName,
    __in PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    __in_opt PWSTR Indent,
    __in_opt ULONG LineLimit
    );

PPH_STRING PhGetFullPath(
    __in PWSTR FileName,
    __out_opt PULONG IndexOfFileName
    );

PPH_STRING PhExpandEnvironmentStrings(
    __in PWSTR String
    );

PPH_STRING PhGetBaseName(
    __in PPH_STRING FileName
    );

PPH_STRING PhGetSystemDirectory();

PPH_STRING PhGetSystemRoot();

PLDR_DATA_TABLE_ENTRY PhFindLoaderEntry(
    __in PVOID DllHandle
    );

PPH_STRING PhGetDllFileName(
    __in PVOID DllHandle,
    __out_opt PULONG IndexOfFileName
    );

PPH_STRING PhGetApplicationFileName();

PPH_STRING PhGetApplicationDirectory();

PPH_STRING PhGetKnownLocation(
    __in ULONG Folder,
    __in_opt PWSTR AppendPath
    );

FORCEINLINE BOOLEAN PhFileExists(
    __in PWSTR FileName
    )
{
    return GetFileAttributes(FileName) != INVALID_FILE_ATTRIBUTES;
}

NTSTATUS PhWaitForMultipleObjectsAndPump(
    __in_opt HWND hWnd,
    __in ULONG NumberOfHandles,
    __in PHANDLE Handles,
    __in ULONG Timeout
    );

#define PH_CREATE_PROCESS_INHERIT_HANDLES 0x1
#define PH_CREATE_PROCESS_UNICODE_ENVIRONMENT 0x2
#define PH_CREATE_PROCESS_SUSPENDED 0x4
#define PH_CREATE_PROCESS_BREAKAWAY_FROM_JOB 0x8
#define PH_CREATE_PROCESS_NEW_CONSOLE 0x10

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

VOID PhShellExecute(
    __in HWND hWnd,
    __in PWSTR FileName,
    __in PWSTR Parameters
    );

#define PH_SHELL_EXECUTE_ADMIN 0x1
#define PH_SHELL_EXECUTE_PUMP_MESSAGES 0x2

BOOLEAN PhShellExecuteEx(
    __in HWND hWnd,
    __in PWSTR FileName,
    __in PWSTR Parameters,
    __in ULONG ShowWindowType,
    __in ULONG Flags,
    __in_opt ULONG Timeout,
    __out_opt PHANDLE ProcessHandle
    );

VOID PhShellExploreFile(
    __in HWND hWnd,
    __in PWSTR FileName
    );

VOID PhShellProperties(
    __in HWND hWnd,
    __in PWSTR FileName
    );

VOID PhShellOpenKey(
    __in HWND hWnd,
    __in PPH_STRING KeyName
    );

PPH_STRING PhQueryRegistryString(
    __in HKEY KeyHandle,
    __in_opt PWSTR ValueName
    );

typedef struct _PH_FLAG_MAPPING
{
    ULONG Flag1;
    ULONG Flag2;
} PH_FLAG_MAPPING, *PPH_FLAG_MAPPING;

VOID PhMapFlags1(
    __inout PULONG Value2,
    __in ULONG Value1,
    __in PPH_FLAG_MAPPING Mappings,
    __in ULONG NumberOfMappings
    );

VOID PhMapFlags2(
    __inout PULONG Value1,
    __in ULONG Value2,
    __in PPH_FLAG_MAPPING Mappings,
    __in ULONG NumberOfMappings
    );

PVOID PhCreateOpenFileDialog();

PVOID PhCreateSaveFileDialog();

VOID PhFreeFileDialog(
    __in PVOID FileDialog
    );

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

ULONG PhGetFileDialogOptions(
    __in PVOID FileDialog
    );

VOID PhSetFileDialogOptions(
    __in PVOID FileDialog,
    __in ULONG Options
    );

typedef struct _PH_FILETYPE_FILTER
{
    PWSTR Name;
    PWSTR Filter;
} PH_FILETYPE_FILTER, *PPH_FILETYPE_FILTER;

VOID PhSetFileDialogFilter(
    __in PVOID FileDialog,
    __in PPH_FILETYPE_FILTER Filters,
    __in ULONG NumberOfFilters
    );

PPH_STRING PhGetFileDialogFileName(
    __in PVOID FileDialog
    );

VOID PhSetFileDialogFileName(
    __in PVOID FileDialog,
    __in PWSTR FileName
    );

NTSTATUS PhIsExecutablePacked(
    __in PWSTR FileName,
    __out PBOOLEAN IsPacked,
    __out_opt PULONG NumberOfModules,
    __out_opt PULONG NumberOfFunctions
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
    __in PVOID Context
    );

#define PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS 0x1
#define PH_COMMAND_LINE_IGNORE_FIRST_PART 0x2
#define PH_COMMAND_LINE_CALLBACK_ALL_MAIN 0x4

PPH_STRING PhParseCommandLinePart(
    __in PPH_STRINGREF CommandLine,
    __inout PULONG Index
    );

BOOLEAN PhParseCommandLine(
    __in PPH_STRINGREF CommandLine,
    __in PPH_COMMAND_LINE_OPTION Options,
    __in ULONG NumberOfOptions,
    __in ULONG Flags,
    __in PPH_COMMAND_LINE_CALLBACK Callback,
    __in PVOID Context
    );

#endif
