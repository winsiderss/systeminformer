#ifndef _PH_PHNATIVE_H
#define _PH_PHNATIVE_H

EXTERN_C_START

/** The PID of the idle process. */
#define SYSTEM_IDLE_PROCESS_ID ((HANDLE)0)
/** The PID of the system process. */
#define SYSTEM_PROCESS_ID ((HANDLE)4)

#define SYSTEM_IDLE_PROCESS_NAME ((UNICODE_STRING)RTL_CONSTANT_STRING(L"System Idle Process"))

#define PhNtPathSeperatorString ((PH_STRINGREF)PH_STRINGREF_INIT(L"\\")) // OBJ_NAME_PATH_SEPARATOR // RtlNtPathSeperatorString

// General object-related function types

typedef NTSTATUS (NTAPI *PPH_OPEN_OBJECT)(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    );

typedef NTSTATUS (NTAPI *PPH_CLOSE_OBJECT)(
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

typedef struct _PH_TOKEN_ATTRIBUTES
{
    HANDLE TokenHandle;
    struct
    {
        ULONG Elevated : 1;
        ULONG ElevationType : 2;
        ULONG ReservedBits : 29;
    };
    ULONG Reserved;
    PSID TokenSid;
} PH_TOKEN_ATTRIBUTES, *PPH_TOKEN_ATTRIBUTES;

typedef enum _MANDATORY_LEVEL_RID {
    MandatoryUntrustedRID = SECURITY_MANDATORY_UNTRUSTED_RID,
    MandatoryLowRID = SECURITY_MANDATORY_LOW_RID,
    MandatoryMediumRID = SECURITY_MANDATORY_MEDIUM_RID,
    //MandatoryMediumPlusRID = SECURITY_MANDATORY_MEDIUM_PLUS_RID,
    MandatoryHighRID = SECURITY_MANDATORY_HIGH_RID,
    MandatorySystemRID = SECURITY_MANDATORY_SYSTEM_RID,
    MandatorySecureProcessRID = SECURITY_MANDATORY_PROTECTED_PROCESS_RID
} MANDATORY_LEVEL_RID, *PMANDATORY_LEVEL_RID;

PHLIBAPI
PH_TOKEN_ATTRIBUTES
NTAPI
PhGetOwnTokenAttributes(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE ProcessId
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
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
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

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessImageFileNameById(
    _In_ HANDLE ProcessId,
    _Out_opt_ PPH_STRING *FullFileName,
    _Out_opt_ PPH_STRING *FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessImageFileNameByProcessId(
    _In_opt_ HANDLE ProcessId,
    _Out_ PPH_STRING* FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIsBeingDebugged(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsBeingDebugged
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessDeviceMap(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG DeviceMap
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
PhGetProcessCommandLineStringRef(
    _Out_ PPH_STRINGREF CommandLine
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessCurrentDirectory(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN IsWow64,
    _Out_ PPH_STRING* CurrentDirectory
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessDesktopInfo(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *DesktopInfo
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessWindowTitle(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG WindowFlags,
    _Out_ PPH_STRING *WindowTitle
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
_Success_(return)
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
PhQueryEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_ PPH_STRINGREF Name,
    _Out_opt_ PPH_STRING* Value
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_ PPH_STRINGREF Name,
    _In_opt_ PPH_STRINGREF Value
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
PhLoadDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF FileName,
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
PhGetProcessUnloadedDlls(
    _In_ HANDLE ProcessId,
    _Out_ PVOID *EventTrace,
    _Out_ ULONG *EventTraceSize,
    _Out_ ULONG *EventTraceCount
    );

PHLIBAPI
NTSTATUS
NTAPI
PhTraceControl(
    _In_ TRACE_CONTROL_INFORMATION_CLASS TraceInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_opt_ PVOID* TraceInformation,
    _Out_opt_ PULONG TraceInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetEnvironmentVariableRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF Name,
    _In_opt_ PPH_STRINGREF Value,
    _In_opt_ PLARGE_INTEGER Timeout
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetJobProcessIdList(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_PROCESS_ID_LIST *ProcessIdList
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetJobBasicAndIoAccounting(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION BasicAndIoAccounting
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetJobBasicLimits(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimits
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetJobExtendedLimits(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimits
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetJobBasicUiRestrictions(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_UI_RESTRICTIONS BasicUiRestrictions
    );

PHLIBAPI
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
PhGetTokenRestrictedSids(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_GROUPS* RestrictedSids
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
PhGetTokenTrustLevel(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_PROCESS_TRUST_LEVEL *TrustLevel
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenAppContainerSid(
    _In_ HANDLE TokenHandle,
    _Out_ PSID* AppContainerSid
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenSecurityAttributes(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION* SecurityAttributes
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsTokenFullTrustPackage(
    _In_ HANDLE TokenHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenIsLessPrivilegedAppContainer(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsLessPrivilegedAppContainer
    );

PHLIBAPI
ULONG64
NTAPI
PhGetTokenSecurityAttributeValueUlong64(
    _In_ HANDLE TokenHandle,
    _In_ PPH_STRINGREF Name,
    _In_ ULONG ValueIndex
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetTokenPackageFullName(
    _In_ HANDLE TokenHandle
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetProcessPackageFullName(
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenNamedObjectPath(
    _In_ HANDLE TokenHandle,
    _In_opt_ PSID Sid,
    _Out_ PPH_STRING* ObjectPath
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetAppContainerNamedObjectPath(
    _In_ HANDLE TokenHandle,
    _In_opt_ PSID AppContainerSid,
    _In_ BOOLEAN RelativePath,
    _Out_ PPH_STRING* ObjectPath
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
PhAdjustPrivilege(
    _In_opt_ PWSTR PrivilegeName,
    _In_opt_ LONG Privilege,
    _In_ BOOLEAN Enable
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetTokenGroups(
    _In_ HANDLE TokenHandle,
    _In_opt_ PWSTR GroupName,
    _In_opt_ PSID GroupSid,
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
PhGetTokenIntegrityLevelRID(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PMANDATORY_LEVEL_RID IntegrityLevelRID,
    _Out_opt_ PWSTR *IntegrityString
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
PhGetTokenProcessTrustLevelRID(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PULONG ProtectionType,
    _Out_opt_ PULONG ProtectionLevel,
    _Out_opt_ PPH_STRING* TrustLevelString,
    _Out_opt_ PPH_STRING* TrustLevelSidString
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileBasicInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_BASIC_INFORMATION BasicInfo
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileStandardInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_STANDARD_INFORMATION StandardInfo
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetFileCompletionNotificationMode(
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags
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
PhGetFilePosition(
    _In_ HANDLE FileHandle,
    _Out_ PLARGE_INTEGER Position
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetFilePosition(
    _In_ HANDLE FileHandle,
    _In_opt_ PLARGE_INTEGER Position
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileAllocationSize(
    _In_ HANDLE FileHandle,
    _Out_ PLARGE_INTEGER AllocationSize
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetFileAllocationSize(
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER AllocationSize
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileIndexNumber(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_INTERNAL_INFORMATION IndexNumber
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteFile(
    _In_ HANDLE FileHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileHandleName(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING *FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileAllInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_ALL_INFORMATION *FileInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileId(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_ID_INFORMATION FileId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIdsUsingFile(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_PROCESS_IDS_USING_FILE_INFORMATION *ProcessIdsUsingFile
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileUsn(
    _In_ HANDLE FileHandle,
    _Out_ PLONGLONG Usn
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

PHLIBAPI
NTSTATUS
NTAPI
PhOpenDriverByBaseAddress(
    _Out_ PHANDLE DriverHandle,
    _In_ PVOID BaseAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenDriver(
    _Out_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetDriverName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *Name
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetDriverImageFileName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *Name
    );

PHLIBAPI
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

#define PH_ENUM_PROCESS_MODULES_LIMIT 0x800

/**
 * A callback function passed to PhEnumProcessModules() and called for each process module.
 *
 * \param Module A structure providing information about the module.
 * \param Context A user-defined value passed to PhEnumProcessModules().
 *
 * \return TRUE to continue the enumeration, FALSE to stop.
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
PhGetProcessQuotaLimits(
    _In_ HANDLE ProcessHandle,
    _Out_ PQUOTA_LIMITS QuotaLimits
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessQuotaLimits(
    _In_ HANDLE ProcessHandle,
    _In_ QUOTA_LIMITS QuotaLimits
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessEmptyWorkingSet(
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessEmptyPageWorkingSet(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessPriority(
    _In_ HANDLE ProcessHandle,
    _Out_ PUCHAR PriorityClass
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessPriority(
    _In_ HANDLE ProcessHandle,
    _In_ UCHAR PriorityClass
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessIoPriority(
    _In_ HANDLE ProcessHandle,
    _In_ IO_PRIORITY_HINT IoPriority
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessPagePriority(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG PagePriority
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessPriorityBoost(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN PriorityBoost
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessAffinityMask(
    _In_ HANDLE ProcessHandle,
    _In_ KAFFINITY AffinityMask
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessGroupAffinity(
    _In_ HANDLE ProcessHandle,
    _In_ GROUP_AFFINITY GroupAffinity
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessPowerThrottlingState(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG ControlMask,
    _In_ ULONG StateMask
    );

PHLIBAPI
PVOID
NTAPI
PhGetDllHandle(
    _In_ PWSTR DllName
    );

PHLIBAPI
PVOID
NTAPI
PhGetModuleProcAddress(
    _In_ PWSTR ModuleName,
    _In_opt_ PSTR ProcedureName
    );

PHLIBAPI
PVOID
NTAPI
PhGetProcedureAddress(
    _In_ PVOID DllHandle,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ ULONG ProcedureNumber
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcedureAddressRemoteStringRef(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF FileName,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber,
    _Out_ PVOID *ProcedureAddress,
    _Out_opt_ PVOID *DllBase
    );

FORCEINLINE
NTSTATUS
NTAPI
PhGetProcedureAddressRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PWSTR FileName,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber,
    _Out_ PVOID *ProcedureAddress,
    _Out_opt_ PVOID *DllBase
    )
{
    PH_STRINGREF fileName;

    PhInitializeStringRef(&fileName, FileName);

    return PhGetProcedureAddressRemoteStringRef(
        ProcessHandle,
        &fileName,
        ProcedureName,
        ProcedureNumber,
        ProcedureAddress,
        DllBase
        );
}

PHLIBAPI
NTSTATUS
NTAPI
PhEnumKernelModules(
    _Out_ PRTL_PROCESS_MODULES *Modules
    );

PHLIBAPI
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
 * Gets a pointer to the first process information structure in a buffer returned by
 * PhEnumProcesses().
 *
 * \param Processes A pointer to a buffer returned by PhEnumProcesses().
 */
#define PH_FIRST_PROCESS(Processes) ((PSYSTEM_PROCESS_INFORMATION)(Processes))

/**
 * Gets a pointer to the process information structure after a given structure.
 *
 * \param Process A pointer to a process information structure.
 *
 * \return A pointer to the next process information structure, or NULL if there are no more.
 */
#define PH_NEXT_PROCESS(Process) ( \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset ? \
    (PSYSTEM_PROCESS_INFORMATION)PTR_ADD_OFFSET((Process), \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset) : \
    NULL \
    )

#define PH_PROCESS_EXTENSION(Process) \
    ((PSYSTEM_PROCESS_INFORMATION_EXTENSION)PTR_ADD_OFFSET((Process), \
    UFIELD_OFFSET(SYSTEM_PROCESS_INFORMATION, Threads) + \
    sizeof(SYSTEM_THREAD_INFORMATION) * \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NumberOfThreads))

#define PH_EXTENDED_PROCESS_EXTENSION(Process) \
    ((PSYSTEM_PROCESS_INFORMATION_EXTENSION)PTR_ADD_OFFSET((Process), \
    UFIELD_OFFSET(SYSTEM_PROCESS_INFORMATION, Threads) + \
    sizeof(SYSTEM_EXTENDED_THREAD_INFORMATION) * \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NumberOfThreads))

// rev from PsGetProcessStartKey (dmex)
#define PH_PROCESS_EXTENSION_STARTKEY(SequenceNumber) \
    ((SequenceNumber) | (((ULONGLONG)USER_SHARED_DATA->BootId) << 0x30)) // SYSTEM_PROCESS_INFORMATION_EXTENSION->ProcessSequenceNumber

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

PHLIBAPI
NTSTATUS
NTAPI
PhEnumHandlesEx2(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_HANDLE_SNAPSHOT_INFORMATION *Handles
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
    (PSYSTEM_PAGEFILE_INFORMATION)PTR_ADD_OFFSET((Pagefile), \
    ((PSYSTEM_PAGEFILE_INFORMATION)(Pagefile))->NextEntryOffset) : \
    NULL \
    )

#define PH_FIRST_PAGEFILE_EX(Pagefiles) ( \
    ((PSYSTEM_PAGEFILE_INFORMATION_EX)(Pagefiles))->TotalSize ? \
    (PSYSTEM_PAGEFILE_INFORMATION_EX)(Pagefiles) : \
    NULL \
    )
#define PH_NEXT_PAGEFILE_EX(Pagefile) ( \
    ((PSYSTEM_PAGEFILE_INFORMATION_EX)(Pagefile))->NextEntryOffset ? \
    (PSYSTEM_PAGEFILE_INFORMATION_EX)PTR_ADD_OFFSET((Pagefile), \
    ((PSYSTEM_PAGEFILE_INFORMATION_EX)(Pagefile))->NextEntryOffset) : \
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
PhEnumPagefilesEx(
    _Out_ PVOID *Pagefiles
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
#define PH_CLR_CORE_3_0_ABOVE 0x10
#define PH_CLR_VERSION_MASK 0x20

#define PH_CLR_MSCORLIB_PRESENT 0x10000
#define PH_CLR_CORELIB_PRESENT 0x20000
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
 * A callback function passed to PhEnumDirectoryObjects() and called for each directory object.
 *
 * \param Name The name of the object.
 * \param TypeName The name of the object's type.
 * \param Context A user-defined value passed to PhEnumDirectoryObjects().
 *
 * \return TRUE to continue the enumeration, FALSE to stop.
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
    _In_ PVOID Information,
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

PHLIBAPI
NTSTATUS
NTAPI
PhEnumDirectoryFileEx(
    _In_ HANDLE FileHandle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING SearchPattern,
    _In_ PPH_ENUM_DIRECTORY_FILE Callback,
    _In_opt_ PVOID Context
    );

#define PH_FIRST_FILE_EA(Information) \
    ((PFILE_FULL_EA_INFORMATION)(Information))
#define PH_NEXT_FILE_EA(Information) \
    (((PFILE_FULL_EA_INFORMATION)(Information))->NextEntryOffset ? \
    (PTR_ADD_OFFSET((Information), ((PFILE_FULL_EA_INFORMATION)(Information))->NextEntryOffset)) : \
    NULL \
    )

typedef BOOLEAN (NTAPI *PPH_ENUM_FILE_EA)(
    _In_ PFILE_FULL_EA_INFORMATION Information,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumFileExtendedAttributes(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_FILE_EA Callback,
    _In_opt_ PVOID Context
    );

#define PH_FIRST_STREAM(Streams) ((PFILE_STREAM_INFORMATION)(Streams))
#define PH_NEXT_STREAM(Stream) ( \
    ((PFILE_STREAM_INFORMATION)(Stream))->NextEntryOffset ? \
    (PFILE_STREAM_INFORMATION)(PTR_ADD_OFFSET((Stream), \
    ((PFILE_STREAM_INFORMATION)(Stream))->NextEntryOffset)) : \
    NULL \
    )

PHLIBAPI
NTSTATUS
NTAPI
PhSetFileExtendedAttributes(
    _In_ HANDLE FileHandle,
    _In_ PPH_BYTESREF Name,
    _In_opt_ PPH_BYTESREF Value
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumFileStreams(
    _In_ HANDLE FileHandle,
    _Out_ PVOID *Streams
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_FILE_STREAMS)(
    _In_ PFILE_STREAM_INFORMATION Information,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumFileStreamsEx(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_FILE_STREAMS Callback,
    _In_opt_ PVOID Context
    );

#define PH_FIRST_LINK(Links) ((PFILE_LINK_ENTRY_INFORMATION)(Links))
#define PH_NEXT_LINK(Links) ( \
    ((PFILE_LINK_ENTRY_INFORMATION)(Links))->NextEntryOffset ? \
    (PFILE_LINK_ENTRY_INFORMATION)(PTR_ADD_OFFSET((Links), \
    ((PFILE_LINK_ENTRY_INFORMATION)(Links))->NextEntryOffset)) : \
    NULL \
    )

typedef BOOLEAN (NTAPI *PPH_ENUM_FILE_HARDLINKS)(
    _In_ PFILE_LINK_ENTRY_INFORMATION Information,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumFileHardLinks(
    _In_ HANDLE FileHandle,
    _Out_ PVOID *HardLinks
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumFileHardLinksEx(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_FILE_HARDLINKS Callback,
    _In_opt_ PVOID Context
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
#define PH_MODULE_TYPE_ELF_MAPPED_IMAGE 6

typedef struct _PH_MODULE_INFO
{
    ULONG Type;
    PVOID BaseAddress;
    PVOID ParentBaseAddress;
    PVOID OriginalBaseAddress;
    ULONG Size;
    PVOID EntryPoint;
    ULONG Flags;
    PPH_STRING Name;
    PPH_STRING FileNameWin32;
    PPH_STRING FileName;

    USHORT LoadOrderIndex; // -1 if N/A
    USHORT LoadCount; // -1 if N/A
    USHORT LoadReason; // -1 if N/A
    USHORT Reserved;
    LARGE_INTEGER LoadTime; // 0 if N/A
} PH_MODULE_INFO, *PPH_MODULE_INFO;

/**
 * A callback function passed to PhEnumGenericModules() and called for each process module.
 *
 * \param Module A structure providing information about the module.
 * \param Context A user-defined value passed to PhEnumGenericModules().
 *
 * \return TRUE to continue the enumeration, FALSE to stop.
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

PHLIBAPI
NTSTATUS
NTAPI
PhLoadAppKey(
    _Out_ PHANDLE KeyHandle,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG Flags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_INFORMATION_CLASS KeyInformationClass,
    _Out_ PVOID *Buffer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryKeyLastWriteTime(
    _In_ HANDLE KeyHandle,
    _Out_ PLARGE_INTEGER LastWriteTime
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_ PVOID *Buffer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    );

FORCEINLINE
NTSTATUS
NTAPI
PhSetValueKeyZ(
    _In_ HANDLE KeyHandle,
    _In_opt_ PWSTR ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    if (ValueName)
    {
        PH_STRINGREF valueNameSr;

        PhInitializeStringRef(&valueNameSr, ValueName);

        return PhSetValueKey(
            KeyHandle,
            &valueNameSr,
            ValueType,
            Buffer,
            BufferLength
            );
    }
    else
    {
        return PhSetValueKey(
            KeyHandle,
            NULL,
            ValueType,
            Buffer,
            BufferLength
            );
    }
}

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_KEY_CALLBACK)(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumerateKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_INFORMATION_CLASS InformationClass,
    _In_ PPH_ENUM_KEY_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumerateValueKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_VALUE_INFORMATION_CLASS InformationClass,
    _In_ PPH_ENUM_KEY_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateFileWin32(
    _Out_ PHANDLE FileHandle,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG FileAttributes,
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
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG CreateStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ PPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenFileWin32(
    _Out_ PHANDLE FileHandle,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenFileWin32Ex(
    _Out_ PHANDLE FileHandle,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions,
    _Out_opt_ PULONG OpenStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ PPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions,
    _Out_opt_ PULONG OpenStatus
    );

typedef struct _PH_FILE_ID_DESCRIPTOR
{
    FILE_ID_TYPE Type;
    union
    {
        LARGE_INTEGER FileId;
        GUID ObjectId;
        FILE_ID_128 ExtendedFileId;
    };
} PH_FILE_ID_DESCRIPTOR, *PPH_FILE_ID_DESCRIPTOR;

PHLIBAPI
NTSTATUS
NTAPI
PhOpenFileById(
    _Out_ PHANDLE FileHandle,
    _In_ HANDLE VolumeHandle,
    _In_ PPH_FILE_ID_DESCRIPTOR FileId,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
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
PhQueryFullAttributesFile(
    _In_ PPH_STRINGREF FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryAttributesFileWin32(
    _In_ PWSTR FileName,
    _Out_ PFILE_BASIC_INFORMATION FileInformation
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDoesFileExistWin32(
    _In_ PWSTR FileName
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDoesFileExist(
    _In_ PPH_STRINGREF FileName
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDoesDirectoryExistWin32(
    _In_ PWSTR FileName
    );

PHLIBAPI
RTL_PATH_TYPE
NTAPI
PhDetermineDosPathNameType(
    _In_ PWSTR FileName
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
PhCopyFileWin32(
    _In_ PWSTR OldFileName,
    _In_ PWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    );

PHLIBAPI
NTSTATUS
NTAPI
PhMoveFileWin32(
    _In_ PWSTR OldFileName,
    _In_ PWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateDirectoryWin32(
    _In_ PPH_STRING DirectoryPath
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateDirectoryFullPathWin32(
    _In_ PPH_STRINGREF FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteDirectoryWin32(
    _In_ PPH_STRING DirectoryPath
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreatePipe(
    _Out_ PHANDLE PipeReadHandle,
    _Out_ PHANDLE PipeWriteHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreatePipeEx(
    _Out_ PHANDLE PipeReadHandle,
    _Out_ PHANDLE PipeWriteHandle,
    _In_ BOOLEAN InheritHandles,
    _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateNamedPipe(
    _Out_ PHANDLE PipeHandle,
    _In_ PWSTR PipeName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhConnectPipe(
    _Out_ PHANDLE PipeHandle,
    _In_ PWSTR PipeName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhListenNamedPipe(
    _In_ HANDLE PipeHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDisconnectNamedPipe(
    _In_ HANDLE PipeHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhPeekNamedPipe(
    _In_ HANDLE PipeHandle,
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
    _In_ HANDLE PipeHandle,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWaitForNamedPipe(
    _In_ PWSTR PipeName,
    _In_opt_ ULONG Timeout
    );

PHLIBAPI
NTSTATUS
NTAPI
PhImpersonateClientOfNamedPipe(
    _In_ HANDLE PipeHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDisableImpersonateNamedPipe(
    _In_ HANDLE PipeHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetNamedPipeClientComputerName(
    _In_ HANDLE PipeHandle,
    _In_ ULONG ClientComputerNameLength,
    _Out_ PVOID ClientComputerName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetNamedPipeClientProcessId(
    _In_ HANDLE PipeHandle,
    _Out_ PHANDLE ClientProcessId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetNamedPipeServerProcessId(
    _In_ HANDLE PipeHandle,
    _Out_ PHANDLE ServerProcessId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadName(
    _In_ HANDLE ThreadHandle,
    _Out_ PPH_STRING *ThreadName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetThreadName(
    _In_ HANDLE ThreadHandle,
    _In_ PCWSTR ThreadName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetThreadAffinityMask(
    _In_ HANDLE ThreadHandle,
    _In_ KAFFINITY AffinityMask
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetThreadBasePriority(
    _In_ HANDLE ThreadHandle,
    _In_ KPRIORITY Increment
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetThreadIoPriority(
    _In_ HANDLE ThreadHandle,
    _In_ IO_PRIORITY_HINT IoPriority
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetThreadPagePriority(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG PagePriority
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetThreadPriorityBoost(
    _In_ HANDLE ThreadHandle,
    _In_ BOOLEAN PriorityBoost
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetThreadIdealProcessor(
    _In_ HANDLE ThreadHandle,
    _In_ PPROCESSOR_NUMBER ProcessorNumber,
    _Out_opt_ PPROCESSOR_NUMBER PreviousIdealProcessor
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetThreadGroupAffinity(
    _In_ HANDLE ThreadHandle,
    _In_ GROUP_AFFINITY GroupAffinity
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadLastSystemCall(
    _In_ HANDLE ThreadHandle,
    _Out_ PTHREAD_LAST_SYSCALL_INFORMATION LastSystemCall
    );

PHLIBAPI
NTSTATUS
NTAPI
PhImpersonateToken(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE TokenHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhRevertImpersonationToken(
    _In_ HANDLE ThreadHandle
    );

typedef struct _PH_PROCESS_DEBUG_HEAP_ENTRY
{
    ULONG Flags;
    ULONG Signature;
    UCHAR HeapFrontEndType;
    ULONG NumberOfEntries;
    PVOID BaseAddress;
    SIZE_T BytesAllocated;
    SIZE_T BytesCommitted;
} PH_PROCESS_DEBUG_HEAP_ENTRY, *PPH_PROCESS_DEBUG_HEAP_ENTRY;

typedef struct _PH_PROCESS_DEBUG_HEAP_ENTRY32
{
    ULONG Flags;
    ULONG Signature;
    UCHAR HeapFrontEndType;
    ULONG NumberOfEntries;
    ULONG BaseAddress;
    ULONG BytesAllocated;
    ULONG BytesCommitted;
} PH_PROCESS_DEBUG_HEAP_ENTRY32, *PPH_PROCESS_DEBUG_HEAP_ENTRY32;

typedef struct _PH_PROCESS_DEBUG_HEAP_INFORMATION
{
    ULONG NumberOfHeaps;
    PVOID DefaultHeap;
    PH_PROCESS_DEBUG_HEAP_ENTRY Heaps[1];
} PH_PROCESS_DEBUG_HEAP_INFORMATION, *PPH_PROCESS_DEBUG_HEAP_INFORMATION;

typedef struct _PH_PROCESS_DEBUG_HEAP_INFORMATION32
{
    ULONG NumberOfHeaps;
    ULONG DefaultHeap;
    PH_PROCESS_DEBUG_HEAP_ENTRY32 Heaps[1];
} PH_PROCESS_DEBUG_HEAP_INFORMATION32, *PPH_PROCESS_DEBUG_HEAP_INFORMATION32;

typedef struct _PH_IMAGE_RUNTIME_FUNCTION_ENTRY_AMD64
{
    ULONG BeginAddress;
    ULONG EndAddress;
    union
    {
        ULONG UnwindInfoAddress;
        ULONG UnwindData;
    } DUMMYUNIONNAME;
} PH_IMAGE_RUNTIME_FUNCTION_ENTRY_AMD64, *PPH_IMAGE_RUNTIME_FUNCTION_ENTRY_AMD64;

typedef struct _PH_IMAGE_RUNTIME_FUNCTION_ENTRY_ARM64
{
    ULONG BeginAddress;
    union
    {
        ULONG UnwindData;
        struct
        {
            ULONG Flag : 2;
            ULONG FunctionLength : 11;
            ULONG RegF : 3;
            ULONG RegI : 4;
            ULONG H : 1;
            ULONG CR : 2;
            ULONG FrameSize : 9;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PH_IMAGE_RUNTIME_FUNCTION_ENTRY_ARM64, *PPH_IMAGE_RUNTIME_FUNCTION_ENTRY_ARM64;

PHLIBAPI
NTSTATUS
NTAPI
PhQueryProcessHeapInformation(
    _In_ HANDLE ProcessId,
    _Out_ PPH_PROCESS_DEBUG_HEAP_INFORMATION* HeapInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessArchitecture(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT ProcessArchitecture
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessImageBaseAddress(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* ImageBaseAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessCodePage(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT ProcessCodePage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessConsoleCodePage(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN ConsoleOutputCP,
    _Out_ PUSHORT ConsoleCodePage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessSequenceNumber(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONGLONG SequenceNumber
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessStartKey(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONGLONG ProcessStartKey
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessSystemDllInitBlock(
    _In_ HANDLE ProcessHandle,
    _Out_ PPS_SYSTEM_DLL_INIT_BLOCK* SystemDllInitBlock
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadLastStatusValue(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle,
    _Out_ PNTSTATUS LastStatusValue
    );

typedef enum tagOLETLSFLAGS
{
    OLETLS_LOCALTID = 0x01, // This TID is in the current process.
    OLETLS_UUIDINITIALIZED = 0x02, // This Logical thread is init'd.
    OLETLS_INTHREADDETACH = 0x04, // This is in thread detach.
    OLETLS_CHANNELTHREADINITIALZED = 0x08,// This channel has been init'd
    OLETLS_WOWTHREAD = 0x10, // This thread is a 16-bit WOW thread.
    OLETLS_THREADUNINITIALIZING = 0x20, // This thread is in CoUninitialize.
    OLETLS_DISABLE_OLE1DDE = 0x40, // This thread can't use a DDE window.
    OLETLS_APARTMENTTHREADED = 0x80, // This is an STA apartment thread
    OLETLS_MULTITHREADED = 0x100, // This is an MTA apartment thread
    OLETLS_IMPERSONATING = 0x200, // This thread is impersonating
    OLETLS_DISABLE_EVENTLOGGER = 0x400, // Prevent recursion in event logger
    OLETLS_INNEUTRALAPT = 0x800, // This thread is in the NTA
    OLETLS_DISPATCHTHREAD = 0x1000, // This is a dispatch thread
    OLETLS_HOSTTHREAD = 0x2000, // This is a host thread
    OLETLS_ALLOWCOINIT = 0x4000, // This thread allows inits
    OLETLS_PENDINGUNINIT = 0x8000, // This thread has pending uninit
    OLETLS_FIRSTMTAINIT = 0x10000,// First thread to attempt an MTA init
    OLETLS_FIRSTNTAINIT = 0x20000,// First thread to attempt an NTA init
    OLETLS_APTINITIALIZING = 0x40000, // Apartment Object is initializing
    OLETLS_UIMSGSINMODALLOOP = 0x80000,
    OLETLS_MARSHALING_ERROR_OBJECT = 0x100000,
    OLETLS_WINRT_INITIALIZE = 0x200000,
    OLETLS_APPLICATION_STA = 0x400000,
    OLETLS_IN_SHUTDOWN_CALLBACKS = 0x800000,
    OLETLS_POINTER_INPUT_BLOCKED = 0x1000000,
    OLETLS_IN_ACTIVATION_FILTER = 0x2000000,
    OLETLS_ASTATOASTAEXEMPT_QUIRK = 0x4000000,
    OLETLS_ASTATOASTAEXEMPT_PROXY = 0x8000000,
    OLETLS_ASTATOASTAEXEMPT_INDOUBT = 0x10000000,
    OLETLS_DETECTED_USER_INITIALIZED = 0x20000000,
    OLETLS_BRIDGE_STA = 0x40000000,
    OLETLS_NAINITIALIZING = 0x80000000UL
} OLETLSFLAGS, *POLETLSFLAGS;

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadApartmentState(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle,
    _Out_ POLETLSFLAGS ApartmentState
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadStackLimits(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG_PTR LowPart,
    _Out_ PULONG_PTR HighPart
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadStackSize(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG_PTR StackUsage,
    _Out_ PULONG_PTR StackLimit
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadIsFiber(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN ThreadIsFiber
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsFirmwareSupported(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFirmwareEnvironmentVariable(
    _In_ PPH_STRINGREF VariableName,
    _In_ PPH_STRINGREF VendorGuid,
    _Out_ PVOID* VariableValueBuffer,
    _Out_opt_ PULONG VariableValueLength
    );

#define PH_FIRST_FIRMWARE_VALUE(Variables) ((PVARIABLE_NAME_AND_VALUE)(Variables))
#define PH_NEXT_FIRMWARE_VALUE(Variables) ( \
    ((PVARIABLE_NAME_AND_VALUE)(Variables))->NextEntryOffset ? \
    (PVARIABLE_NAME_AND_VALUE)(PTR_ADD_OFFSET((Variables), \
    ((PVARIABLE_NAME_AND_VALUE)(Variables))->NextEntryOffset)) : \
    NULL \
    )

PHLIBAPI
NTSTATUS
NTAPI
PhEnumFirmwareEnvironmentValues(
    _In_ SYSTEM_ENVIRONMENT_INFORMATION_CLASS InformationClass,
    _Out_ PVOID* Variables
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetSystemEnvironmentBootToFirmware(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateExecutionRequiredRequest(
    _In_ HANDLE ProcessHandle,
    _Out_ PHANDLE PowerRequestHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDestroyExecutionRequiredRequest(
    _In_ HANDLE PowerRequestHandle
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsProcessStateFrozen(
    _In_ HANDLE ProcessId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFreezeProcess(
    _In_ HANDLE ProcessId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhThawProcess(
    _In_ HANDLE ProcessId
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsKnownDllFileName(
    _In_ PPH_STRING FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSystemLogicalProcessorInformation(
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _Out_ PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* Buffer,
    _Out_ PULONG BufferLength
    );

PHLIBAPI
USHORT
NTAPI
PhGetActiveProcessorCount(
    _In_ USHORT ProcessorGroup
    );

typedef struct _PH_PROCESSOR_NUMBER
{
    USHORT Group;
    USHORT Number;
} PH_PROCESSOR_NUMBER, *PPH_PROCESSOR_NUMBER;

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessorNumberFromIndex(
    _In_ ULONG ProcessorIndex,
    _Out_ PPH_PROCESSOR_NUMBER ProcessorNumber
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessorGroupActiveAffinityMask(
    _In_ USHORT ProcessorGroup,
    _Out_ PKAFFINITY ActiveProcessorMask
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessorSystemAffinityMask(
    _Out_ PKAFFINITY ActiveProcessorsAffinityMask
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetNumaHighestNodeNumber(
    _Out_ PUSHORT NodeNumber
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetNumaProcessorNode(
    _In_ PPH_PROCESSOR_NUMBER ProcessorNumber,
    _Out_ PUSHORT NodeNumber
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessValidCallTarget(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    );

typedef struct _PH_SYSTEM_STORE_COMPRESSION_INFORMATION
{
    ULONG CompressionPid;
    ULONG WorkingSetSize;
    SIZE_T TotalDataCompressed;
    SIZE_T TotalCompressedSize;
    SIZE_T TotalUniqueDataCompressed;
} PH_SYSTEM_STORE_COMPRESSION_INFORMATION, *PPH_SYSTEM_STORE_COMPRESSION_INFORMATION;

PHLIBAPI
NTSTATUS
NTAPI
PhGetSystemCompressionStoreInformation(
    _Out_ PPH_SYSTEM_STORE_COMPRESSION_INFORMATION SystemCompressionStoreInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSystemFileCacheSize(
    _Out_ PSYSTEM_FILECACHE_INFORMATION CacheInfo
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetSystemFileCacheSize(
    _In_ SIZE_T MinimumFileCacheSize,
    _In_ SIZE_T MaximumFileCacheSize,
    _In_ ULONG Flags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDeviceIoControlFile(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG ReturnLength
    );

EXTERN_C_END

#endif
