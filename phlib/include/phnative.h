/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#ifndef _PH_PHNATIVE_H
#define _PH_PHNATIVE_H

EXTERN_C_START

/** The PID of the interrupt process. */
#define INTERRUPTS_PROCESS_ID ((HANDLE)(LONG_PTR)-3)
/** The PID of the dpc process. */
#define DPCS_PROCESS_ID ((HANDLE)(LONG_PTR)-2)
/** The PID of the current process. */
#define CURRENT_PROCESS_ID ((HANDLE)(LONG_PTR)-1)
/** The PID of the idle process. */
#define SYSTEM_IDLE_PROCESS_ID ((HANDLE)0)
/** The PID of the system process. */
#define SYSTEM_PROCESS_ID ((HANDLE)4)

/** The name of the system idle process. */
#define SYSTEM_IDLE_PROCESS_NAME ((UNICODE_STRING)RTL_CONSTANT_STRING(L"System Idle Process"))

#define PhNtPathSeparatorString ((PH_STRINGREF)PH_STRINGREF_INIT(L"\\")) // OBJ_NAME_PATH_SEPARATOR // RtlNtPathSeperatorString
#define PhNtDosDevicesPrefix ((PH_STRINGREF)PH_STRINGREF_INIT(L"\\??\\")) // RtlDosDevicesPrefix
#define PhNtDevicePathPrefix ((PH_STRINGREF)PH_STRINGREF_INIT(L"\\Device\\"))
#define PhWin32ExtendedPathPrefix ((PH_STRINGREF)PH_STRINGREF_INIT(L"\\\\?\\")) // extended-length paths, disable path normalization

FORCEINLINE
BOOLEAN
PhIsNullOrInvalidHandle(
    _In_ HANDLE Handle
    )
{
    return (((ULONG_PTR)Handle + 1) & 0xFFFFFFFFFFFFFFFEuLL) == 0;
}

//
// General object-related function types
//

typedef _Function_class_(PH_OPEN_OBJECT)
NTSTATUS NTAPI PH_OPEN_OBJECT(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    );
typedef PH_OPEN_OBJECT* PPH_OPEN_OBJECT;

typedef _Function_class_(PH_CLOSE_OBJECT)
NTSTATUS NTAPI PH_CLOSE_OBJECT(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Release,
    _In_opt_ PVOID Context
    );
typedef PH_CLOSE_OBJECT* PPH_CLOSE_OBJECT;

typedef _Function_class_(PH_GET_OBJECT_SECURITY)
NTSTATUS NTAPI PH_GET_OBJECT_SECURITY(
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    );
typedef PH_GET_OBJECT_SECURITY* PPH_GET_OBJECT_SECURITY;

typedef _Function_class_(PH_SET_OBJECT_SECURITY)
NTSTATUS NTAPI PH_SET_OBJECT_SECURITY(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    );
typedef PH_SET_OBJECT_SECURITY* PPH_SET_OBJECT_SECURITY;

typedef struct _PH_TOKEN_ATTRIBUTES
{
    HANDLE TokenHandle;
    struct
    {
        ULONG Elevated : 1;
        ULONG ElevationType : 2;
        ULONG SpareBits : 29;
    };
    ULONG Spare;
    PSID TokenSid;
} PH_TOKEN_ATTRIBUTES, *PPH_TOKEN_ATTRIBUTES;

typedef enum _MANDATORY_LEVEL_RID
{
    MandatoryUntrustedRID = SECURITY_MANDATORY_UNTRUSTED_RID,
    MandatoryLowRID = SECURITY_MANDATORY_LOW_RID,
    MandatoryMediumRID = SECURITY_MANDATORY_MEDIUM_RID,
    MandatoryMediumPlusRID = SECURITY_MANDATORY_MEDIUM_PLUS_RID,
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
    _In_ HANDLE ProcessId
    );

/**
 * Opens a process handle with the best available query access.
 *
 * The function tries the following access combinations in order:
 *   1. PROCESS_QUERY_INFORMATION | DesiredAccess
 *   2. PROCESS_QUERY_LIMITED_INFORMATION | DesiredAccess
 *   3. PROCESS_QUERY_LIMITED_INFORMATION
 *
 * The first successful attempt is returned to the caller. If all attempts
 * fail, the final NTSTATUS code is returned.
 *
 * \param ProcessHandle Receives the resulting process handle on success.
 * \param DesiredAccess Additional access rights the caller wishes to request.
 * \param ProcessId The process identifier of the target process.
 * \return NTSTATUS Successful or errant status.
 */
FORCEINLINE
NTSTATUS
NTAPI
PhOpenProcessWithQueryAccess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;

    status = PhOpenProcess(
        ProcessHandle,
        PROCESS_QUERY_INFORMATION | DesiredAccess,
        ProcessId
        );

    if (!NT_SUCCESS(status))
    {
        status = PhOpenProcess(
            ProcessHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | DesiredAccess,
            ProcessId
            );

        if (!NT_SUCCESS(status))
        {
            status = PhOpenProcess(
                ProcessHandle,
                PROCESS_QUERY_LIMITED_INFORMATION,
                ProcessId
                );
        }
    }

    return status;
}

PHLIBAPI
NTSTATUS
NTAPI
PhOpenProcessClientId(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenProcessPublic(
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
PhOpenThreadClientId(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenThreadPublic(
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
PhGetThreadBasicInformation(
    _In_ HANDLE ThreadHandle,
    _Out_ PTHREAD_BASIC_INFORMATION BasicInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadBasePriority(
    _In_ HANDLE ThreadHandle,
    _Out_ PKPRIORITY Increment
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadTeb(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG_PTR TebBaseAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadTeb32(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG_PTR TebBaseAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadStartAddress(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG_PTR StartAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadIoPriority(
    _In_ HANDLE ThreadHandle,
    _Out_ IO_PRIORITY_HINT* IoPriority
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadPagePriority(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG PagePriority
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadPriorityBoost(
    _In_ HANDLE ThreadHandle,
    _Out_ PBOOLEAN PriorityBoostDisabled
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadPerformanceCounter(
    _In_ HANDLE ThreadHandle,
    _Out_ PLARGE_INTEGER PerformanceCounter
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadCycleTime(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG64 CycleTime
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadIdealProcessor(
    _In_ HANDLE ThreadHandle,
    _Out_ PPROCESSOR_NUMBER ProcessorNumber
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadSuspendCount(
    _In_ HANDLE ThreadHandle,
    _Out_ PULONG SuspendCount
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadWow64Context(
    _In_ HANDLE ThreadHandle,
    _Out_ PWOW64_CONTEXT Context
    );

#if defined(_ARM64_)
PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadArm32Context(
    _In_ HANDLE ThreadHandle,
    _Out_ PARM_NT_CONTEXT Context
    );
#endif

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadBreakOnTermination(
    _In_ HANDLE ThreadHandle,
    _Out_ PBOOLEAN BreakOnTermination
);

PHLIBAPI
NTSTATUS
NTAPI
PhSetThreadBreakOnTermination(
    _In_ HANDLE ThreadHandle,
    _In_ BOOLEAN BreakOnTermination
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadContainerId(
    _In_ HANDLE ThreadHandle,
    _In_ PGUID ContainerId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadIsIoPending(
    _In_ HANDLE ThreadHandle,
    _Out_ PBOOLEAN IsIoPending
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadTimes(
    _In_ HANDLE ThreadHandle,
    _Out_ PKERNEL_USER_TIMES Times
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadIsTerminated(
    _In_ HANDLE ThreadHandle,
    _Out_ PBOOLEAN IsTerminated
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetThreadIsTerminated2(
    _In_ HANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadGroupAffinity(
    _In_ HANDLE ThreadHandle,
    _Out_ PGROUP_AFFINITY GroupAffinity
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadIndexInformation(
    _In_ HANDLE ThreadHandle,
    _Out_ PTHREAD_INDEX_INFORMATION ThreadIndex
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
PhOpenProcessTokenPublic(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenThreadToken(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
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

#define AUDIT_ALARM_ACE_TYPE_MASK ( \
    (1 << SYSTEM_AUDIT_ACE_TYPE) | \
    (1 << SYSTEM_ALARM_ACE_TYPE) | \
    (1 << SYSTEM_AUDIT_OBJECT_ACE_TYPE) | \
    (1 << SYSTEM_ALARM_OBJECT_ACE_TYPE) | \
    (1 << SYSTEM_AUDIT_CALLBACK_ACE_TYPE) | \
    (1 << SYSTEM_ALARM_CALLBACK_ACE_TYPE) | \
    (1 << SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE) | \
    (1 << SYSTEM_ALARM_CALLBACK_OBJECT_ACE_TYPE))

#define MANDATORY_LABEL_ACE_TYPE_MASK (1 << SYSTEM_MANDATORY_LABEL_ACE_TYPE)
#define RESOURCE_ATTRIBUTE_ACE_TYPE_MASK (1 << SYSTEM_RESOURCE_ATTRIBUTE_ACE_TYPE)
#define SCOPED_POLICY_ACE_TYPE_MASK (1 << SYSTEM_SCOPED_POLICY_ID_ACE_TYPE)
#define PROCESS_TRUST_ACE_TYPE_MASK (1 << SYSTEM_PROCESS_TRUST_LABEL_ACE_TYPE)
#define ACCESS_FILTER_ACE_TYPE_MASK (1 << SYSTEM_ACCESS_FILTER_ACE_TYPE)

PHLIBAPI
NTSTATUS
NTAPI
PhMergeSystemAcls(
    _In_opt_ PACL LowerSacl,
    _In_opt_ PACL HigherSacl,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Outptr_result_maybenull_ PACL* MergedSacl
    );

PHLIBAPI
NTSTATUS
NTAPI
PhMergeSecurityDescriptors(
    _In_ PSECURITY_DESCRIPTOR LowerSecurityDescriptor,
    _In_ PSECURITY_DESCRIPTOR HigherSecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Outptr_ PSECURITY_DESCRIPTOR* MergedSecurityDescriptor
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
PhGetProcessBasicInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_BASIC_INFORMATION BasicInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessExtendedBasicInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_EXTENDED_BASIC_INFORMATION ExtendedBasicInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessTimes(
    _In_ HANDLE ProcessHandle,
    _Out_ PKERNEL_USER_TIMES Times
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessSessionId(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG SessionId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIsWow64(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsWow64Process
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessPeb32(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* Peb32
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessPeb(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* PebBaseAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessDebugObject(
    _In_ HANDLE ProcessHandle,
    _Out_ PHANDLE DebugObjectHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessEnergyValues(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_EXTENDED_ENERGY_VALUES EnergyValues
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessErrorMode(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ErrorMode
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessErrorMode(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG ErrorMode
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessExecuteFlags(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ExecuteFlags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIoPriority(
    _In_ HANDLE ProcessHandle,
    _Out_ IO_PRIORITY_HINT *IoPriority
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessPagePriority(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG PagePriority
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessPriorityBoost(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN PriorityBoostDisabled
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessCycleTime(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG64 CycleTime
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessUptime(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_UPTIME_INFORMATION Uptime
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessConsoleHostProcessId(
    _In_ HANDLE ProcessHandle,
    _Out_ PHANDLE ConsoleHostProcessId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessConsoleHostProcess(
    _In_ HANDLE ProcessHandle,
    _Out_ PHANDLE ConsoleHostProcessId,
    _Out_opt_ PBOOLEAN ConsoleApplication
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessProtection(
    _In_ HANDLE ProcessHandle,
    _Out_ PPS_PROTECTION Protection
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessAffinityMask(
    _In_ HANDLE ProcessHandle,
    _Out_ PKAFFINITY AffinityMask
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessGroupInformation(
    _In_ HANDLE ProcessHandle,
    _Inout_ PUSHORT GroupCount,
    _Out_ PUSHORT GroupArray
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessGroupAffinity(
    _In_ HANDLE ProcessHandle,
    _Out_ PGROUP_AFFINITY GroupAffinity
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIsCFGuardEnabled(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsControlFlowGuardEnabled
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIsXFGuardEnabled(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsXFGuardEnabled,
    _Out_ PBOOLEAN IsXFGuardAuditEnabled
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessHandleCount(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_HANDLE_INFORMATION HandleInfo
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessBreakOnTermination(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN BreakOnTermination
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessBreakOnTermination(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN BreakOnTermination
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessAppMemoryInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_JOB_MEMORY_INFO JobMemoryInfo
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessMitigationPolicy(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESS_MITIGATION_POLICY Policy,
    _Out_ PPROCESS_MITIGATION_POLICY_INFORMATION MitigationPolicy
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessNetworkIoCounters(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_NETWORK_COUNTERS NetworkIoCounters
    );

typedef struct _PH_PROCESS_RUNTIME_LIBRARY
{
    PH_STRINGREF NtdllFileName;
    PH_STRINGREF Kernel32FileName;
    PH_STRINGREF User32FileName;
} PH_PROCESS_RUNTIME_LIBRARY, *PPH_PROCESS_RUNTIME_LIBRARY;

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessRuntimeLibrary(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_RUNTIME_LIBRARY* RuntimeLibrary,
    _Out_opt_ PBOOLEAN IsWow64Process
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
PhGetProcessIsTerminating(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsTerminated
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIsTerminated(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsTerminated
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
    PhpoNone,
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
    _In_ BOOLEAN IsWow64Process,
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
#define PH_PROCESS_DEP_EXECUTE_ENABLED 0x8
#define PH_PROCESS_DEP_IMAGE_ENABLED 0x10
#define PH_PROCESS_DEP_DISABLE_EXCEPTION_CHAIN 0x20

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
PhGetProcessEnvironment(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN IsWow64Process,
    _Out_ PVOID *Environment,
    _Out_ PULONG EnvironmentLength
    );

typedef struct _PH_ENVIRONMENT_VARIABLE
{
    PH_STRINGREF Name;
    PH_STRINGREF Value;
} PH_ENVIRONMENT_VARIABLE, *PPH_ENVIRONMENT_VARIABLE;

PHLIBAPI
NTSTATUS
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
PhQueryEnvironmentVariableStringRef(
    _In_opt_ PVOID Environment,
    _In_ PCPH_STRINGREF Name,
    _Inout_opt_ PPH_STRINGREF Value
    );

FORCEINLINE
NTSTATUS
NTAPI
PhQueryEnvironmentVariableToBufferZ(
    _In_opt_ PVOID Environment,
    _In_ PCWSTR Name,
    _Out_writes_opt_(BufferLength) PWSTR Buffer,
    _In_opt_ SIZE_T BufferLength,
    _Out_ PSIZE_T ReturnLength
    )
{
    NTSTATUS status;
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    status = RtlQueryEnvironmentVariable(
        Environment,
        name.Buffer,
        name.Length / sizeof(WCHAR),
        Buffer,
        BufferLength,
        ReturnLength
        );

    return status;
}

PHLIBAPI
NTSTATUS
NTAPI
PhQueryEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_ PCPH_STRINGREF Name,
    _Out_opt_ PPH_STRING* Value
    );

FORCEINLINE
NTSTATUS
NTAPI
PhQueryEnvironmentVariableZ(
    _In_opt_ PVOID Environment,
    _In_ PCWSTR Name,
    _Out_opt_ PPH_STRING* Value
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, Name);

    return PhQueryEnvironmentVariable(Environment, &name, Value);
}

PHLIBAPI
NTSTATUS
NTAPI
PhSetEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_ PCPH_STRINGREF Name,
    _In_opt_ PCPH_STRINGREF Value
    );

FORCEINLINE
NTSTATUS
NTAPI
PhSetEnvironmentVariableZ(
    _In_opt_ PVOID Environment,
    _In_ PCWSTR Name,
    _In_opt_ PCWSTR Value
    )
{
    if (Value)
    {
        PH_STRINGREF name;
        PH_STRINGREF value;

        PhInitializeStringRef(&name, Name);
        PhInitializeStringRef(&value, Value);

        return PhSetEnvironmentVariable(Environment, &name, &value);
    }
    else
    {
        PH_STRINGREF name;

        PhInitializeStringRef(&name, Name);

        return PhSetEnvironmentVariable(Environment, &name, NULL);
    }
}

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
PhGetProcessMappedImageInformation(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_ PMEMORY_IMAGE_INFORMATION ImageInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessMappedImageBaseFromAddress(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID Address,
    _Out_ PVOID* ImageBase,
    _Out_opt_ PSIZE_T ImageSize
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
    _In_opt_ ULONG Timeout
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadDllProcessApcThread(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF FileName,
    _In_opt_ ULONG Timeout
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUnloadDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_opt_ ULONG Timeout
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
    _In_ ETWTRACECONTROLCODE TraceInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhTraceControlVariableSize(
    _In_ ETWTRACECONTROLCODE TraceInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(*OutputBufferLength) PVOID* OutputBuffer,
    _Out_opt_ PULONG OutputBufferLength
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
PhGetWindowClientId(
    _In_ HWND WindowHandle,
    _Out_ PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDestroyWindowRemote(
    _In_ HANDLE ProcessHandle,
    _In_ HWND WindowHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhInvokeWindowProcedureRemote(
    _In_ HWND WindowHandle,
    _In_ PPS_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetHandleInformationRemote(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE RemoteHandle,
    _In_ ULONG Mask,
    _In_ ULONG Flags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenJobObject(
    _Out_ PHANDLE JobHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName
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
PhGetTokenType(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_TYPE Type
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenSessionId(
    _In_ HANDLE TokenHandle,
    _Out_ PULONG SessionId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenElevationType(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_ELEVATION_TYPE ElevationType
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenElevation(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN TokenIsElevated
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenStatistics(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_STATISTICS Statistics
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenSource(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_SOURCE Source
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenLinkedToken(
    _In_ HANDLE TokenHandle,
    _Out_ PHANDLE LinkedTokenHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenIsRestricted(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsRestricted
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenIsVirtualizationAllowed(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsVirtualizationAllowed
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenIsVirtualizationEnabled(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsVirtualizationEnabled
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenUIAccess(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsUIAccessEnabled
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetTokenUIAccess(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN IsUIAccessEnabled
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenIsSandBoxInert(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsSandBoxInert
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenMandatoryPolicy(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_MANDATORY_POLICY MandatoryPolicy
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenOrigin(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_ORIGIN Origin
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenIsAppContainer(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsAppContainer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenAppContainerNumber(
    _In_ HANDLE TokenHandle,
    _Out_ PULONG AppContainerNumber
    );

// rev from SE_TOKEN_USER (dmex)
typedef struct _PH_TOKEN_USER
{
    union
    {
        TOKEN_USER TokenUser;
        SID_AND_ATTRIBUTES User;
    };
    union
    {
        SID Sid;
        BYTE Buffer[SECURITY_MAX_SID_SIZE];
    };
} PH_TOKEN_USER, *PPH_TOKEN_USER;

C_ASSERT(sizeof(PH_TOKEN_USER) >= TOKEN_USER_MAX_SIZE);

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenUser(
    _In_ HANDLE TokenHandle,
    _Out_ PPH_TOKEN_USER User
    );

typedef struct _PH_TOKEN_OWNER
{
    union
    {
        TOKEN_OWNER TokenOwner;
        SID_AND_ATTRIBUTES Owner;
    };
    union
    {
        SID Sid;
        BYTE Buffer[SECURITY_MAX_SID_SIZE];
    };
} PH_TOKEN_OWNER, *PPH_TOKEN_OWNER;

C_ASSERT(sizeof(PH_TOKEN_OWNER) >= TOKEN_OWNER_MAX_SIZE);

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenOwner(
    _In_ HANDLE TokenHandle,
    _Out_ PPH_TOKEN_OWNER Owner
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
PhGetTokenDefaultDacl(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_DEFAULT_DACL* DefaultDacl
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

typedef struct _PH_TOKEN_APPCONTAINER
{
    union
    {
        TOKEN_APPCONTAINER_INFORMATION TokenAppContainer;
        SID_AND_ATTRIBUTES AppContainer;
    };
    union
    {
        SID Sid;
        BYTE Buffer[SECURITY_MAX_SID_SIZE];
    };
} PH_TOKEN_APPCONTAINER, *PPH_TOKEN_APPCONTAINER;

C_ASSERT(sizeof(PH_TOKEN_APPCONTAINER) >= TOKEN_APPCONTAINER_SID_MAX_SIZE);

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenAppContainerSid(
    _In_ HANDLE TokenHandle,
    _Out_ PPH_TOKEN_APPCONTAINER AppContainerSid
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenSecurityAttributes(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION* SecurityAttributes
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenSecurityAttribute(
    _In_ HANDLE TokenHandle,
    _In_ PCPH_STRINGREF AttributeName,
    _Out_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION* SecurityAttributes
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDoesTokenSecurityAttributeExist(
    _In_ HANDLE TokenHandle,
    _In_ PCPH_STRINGREF AttributeName,
    _Out_ PBOOLEAN SecurityAttributeExists
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetTokenIsFullTrustPackage(
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
    _In_ PCPH_STRINGREF Name,
    _In_ ULONG ValueIndex
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetTokenPackageFullName(
    _In_ HANDLE TokenHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIsStronglyNamed(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsStronglyNamed
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetProcessIsFullTrustPackage(
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetProcessPackageFullName(
    _In_ HANDLE ProcessHandle
    );

// rev from RtlInitializeSid (dmex)
FORCEINLINE
BOOLEAN
NTAPI
PhInitializeSid(
    _Out_ PSID Sid,
    _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    _In_ UCHAR SubAuthorityCount
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return NT_SUCCESS(RtlInitializeSid(Sid, IdentifierAuthority, SubAuthorityCount));
#else
    ((PISID)Sid)->Revision = SID_REVISION;
    ((PISID)Sid)->IdentifierAuthority = *IdentifierAuthority;
    ((PISID)Sid)->SubAuthorityCount = SubAuthorityCount;

    for (UCHAR i = 0; i < SubAuthorityCount; i++)
    {
        ((PISID)Sid)->SubAuthority[i] = 0;
    }

    return TRUE;
#endif
}

// rev from RtlLengthSid (dmex)
FORCEINLINE
ULONG
NTAPI
PhLengthSid(
    _In_ PCSID Sid
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlLengthSid((PSID)Sid);
#else
    //return UFIELD_OFFSET(SID, SubAuthority) + (((PISID)Sid)->SubAuthorityCount * sizeof(ULONG));
    return UFIELD_OFFSET(SID, SubAuthority[((PISID)Sid)->SubAuthorityCount]);
#endif
}

// rev from RtlLengthRequiredSid (dmex)
FORCEINLINE
ULONG
NTAPI
PhLengthRequiredSid(
    _In_ ULONG SubAuthorityCount
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlLengthRequiredSid(SubAuthorityCount);
#else
    return UFIELD_OFFSET(SID, SubAuthority[SubAuthorityCount]);
#endif
}

// rev from RtlEqualSid (dmex)
FORCEINLINE
BOOLEAN
NTAPI
PhEqualSid(
    _In_ PCSID Sid1,
    _In_ PCSID Sid2
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlEqualSid((PSID)Sid1, (PSID)Sid2);
#else
    if (!(Sid1 && Sid2))
        return FALSE;

    if (!(
        ((PISID)Sid1)->Revision == ((PISID)Sid2)->Revision &&
        ((PISID)Sid1)->SubAuthorityCount == ((PISID)Sid2)->SubAuthorityCount
        ))
    {
        return FALSE;
    }

    ULONG length1 = PhLengthSid(Sid1);
    ULONG length2 = PhLengthSid(Sid2);

    if (length1 != length2)
        return FALSE;

    return (BOOLEAN)RtlEqualMemory(Sid1, Sid2, length1);
#endif
}

// rev from RtlValidSid (dmex)
FORCEINLINE
BOOLEAN
NTAPI
PhValidSid(
    _In_ PCSID Sid
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlValidSid((PSID)Sid);
#else
    if (
        ((PISID)Sid) &&
        ((PISID)Sid)->Revision == SID_REVISION &&
        ((PISID)Sid)->SubAuthorityCount <= SID_MAX_SUB_AUTHORITIES
        )
    {
        return TRUE;
    }

    return FALSE;
#endif
}

// rev from RtlSubAuthoritySid (dmex)
FORCEINLINE
PULONG
NTAPI
PhSubAuthoritySid(
    _In_ PCSID Sid,
    _In_ ULONG SubAuthority
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlSubAuthoritySid((PSID)Sid, SubAuthority);
#else
    return &((PISID)Sid)->SubAuthority[SubAuthority];
#endif
}

// rev from RtlSubAuthorityCountSid (dmex)
FORCEINLINE
PUCHAR
NTAPI
PhSubAuthorityCountSid(
    _In_ PCSID Sid
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlSubAuthorityCountSid((PSID)Sid);
#else
    return &((PISID)Sid)->SubAuthorityCount;
#endif
}

// rev from RtlIdentifierAuthoritySid (dmex)
FORCEINLINE
PSID_IDENTIFIER_AUTHORITY
NTAPI
PhIdentifierAuthoritySid(
    _In_ PCSID Sid
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlIdentifierAuthoritySid((PSID)Sid);
#else
    return &((PISID)Sid)->IdentifierAuthority;
#endif
}

FORCEINLINE
BOOLEAN
NTAPI
PhEqualIdentifierAuthoritySid(
    _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthoritySid1,
    _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthoritySid2
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlEqualMemory(RtlIdentifierAuthoritySid(IdentifierAuthoritySid1), RtlIdentifierAuthoritySid(IdentifierAuthoritySid2), sizeof(SID_IDENTIFIER_AUTHORITY));
#else
    return (BOOLEAN)RtlEqualMemory(IdentifierAuthoritySid1, IdentifierAuthoritySid2, sizeof(SID_IDENTIFIER_AUTHORITY));
#endif
}

// rev from RtlCreateSecurityDescriptor (dmex)
FORCEINLINE
NTSTATUS
NTAPI
PhCreateSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Revision
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlCreateSecurityDescriptor(SecurityDescriptor, Revision);
#else
    memset(SecurityDescriptor, 0, sizeof(SECURITY_DESCRIPTOR));
    ((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Revision = (BYTE)Revision;
    return STATUS_SUCCESS;
#endif
}

// rev from RtlValidAcl (dmex)
FORCEINLINE
BOOLEAN
NTAPI
PhValidAcl(
    _In_opt_ PACL Acl
    )
{
    if (!Acl || Acl->AclRevision < MIN_ACL_REVISION || Acl->AclRevision > MAX_ACL_REVISION)
        return FALSE;
    if (Acl->AclSize < sizeof(ACL) || ((Acl->AclSize & 3U) != 0)) // enforce alignment
        return FALSE;

    return RtlValidAcl(Acl);
}

FORCEINLINE
UCHAR
NTAPI
PhRequiredAclRevision(
    _In_ UCHAR AceType
    )
{
    switch (AceType)
    {
    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
    case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_CALLBACK_OBJECT_ACE_TYPE:
        return ACL_REVISION4;

    case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        return ACL_REVISION3;

    default:
        return MIN_ACL_REVISION;
    }
}

FORCEINLINE
VOID
NTAPI
PhEnsureAclRevision(
    _Inout_ PULONG_PTR AclRevision,
    _In_ UCHAR AceType
    )
{
    UCHAR requiredRevision = PhRequiredAclRevision(AceType);

    if (requiredRevision > *AclRevision)
    {
        *AclRevision = requiredRevision;
    }
}

FORCEINLINE
PVOID
NTAPI
PhFirstAce(
    _In_ PACL Acl
    )
{
    return RTL_PTR_ADD(Acl, sizeof(ACL));
}

FORCEINLINE
PVOID
NTAPI
PhNextAce(
    _In_ PACL Ace
    )
{
    PACE_HEADER ace = (PACE_HEADER)Ace;

    if (ace->AceSize < sizeof(ACE_HEADER) || (ace->AceSize & 3U) != 0) // enforce alignment
        return NULL;

    return RTL_PTR_ADD(Ace, ace->AceSize);
}

FORCEINLINE
BOOLEAN
NTAPI
PhFirstFreeAce(
    _In_ PACL Acl,
    _Out_ PVOID* FirstFree
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlFirstFreeAce(Acl, FirstFree);
#else
    if (!PhValidAcl(Acl))
    {
        *FirstFree = NULL;
        return FALSE;
    }

    ULONG_PTR current = (ULONG_PTR)PhFirstAce(Acl);
    const ULONG_PTR last = (ULONG_PTR)RTL_PTR_ADD(Acl, Acl->AclSize);

    for (USHORT i = 0; i < Acl->AceCount; i++)
    {
        if (current >= last)
            goto InvalidAcl;

        //ULONG_PTR headerEnd = (ULONG_PTR)RTL_PTR_ADD(current, sizeof(ACE_HEADER));
        //if (headerEnd > last)
        //    goto InvalidAcl;

        ULONG_PTR next = (ULONG_PTR)PhNextAce((PACL)current);

        if (next > last)
            goto InvalidAcl;

        current = next;
    }

    *FirstFree = (PVOID)current;
    return TRUE;

InvalidAcl:
    *FirstFree = NULL;
    return FALSE;
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhGetAce(
    _In_ PACL Acl,
    _In_ ULONG AceIndex,
    _Out_ PVOID* Ace
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlGetAce(Acl, AceIndex, Ace);
#else
    PVOID current;
    PVOID lastace;

    if (Acl->AclRevision < MIN_ACL_REVISION ||
        Acl->AclRevision > MAX_ACL_REVISION ||
        AceIndex >= Acl->AceCount)
    {
        return STATUS_INVALID_ACL;
    }

    current = PhFirstAce(Acl);
    lastace = RTL_PTR_ADD(Acl, Acl->AclSize);

    for (ULONG i = 0; i < AceIndex; i++)
    {
        if ((ULONG_PTR)current >= (ULONG_PTR)lastace)
            return STATUS_INVALID_ACL;

        current = PhNextAce((PACL)current);
    }

    if (current >= lastace)
        return STATUS_INVALID_ACL;

    *Ace = current;
    return STATUS_SUCCESS;
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhAddAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG StartingAceIndex,
    _In_reads_bytes_(AceListLength) PVOID AceList,
    _In_ ULONG AceListLength
    )
{
    PVOID firstFree = NULL;

    if (!PhValidAcl(Acl))
        return STATUS_INVALID_PARAMETER;
    if (AceListLength == 0)
        return STATUS_SUCCESS;

    if (!PhFirstFreeAce(Acl, &firstFree) || !firstFree)
        return STATUS_INVALID_ACL;

    // Determine final revision (max of current ACL revision and requested).
    ULONG_PTR finalRevision = Acl->AclRevision;

    if ((ULONG_PTR)AceRevision > finalRevision)
    {
        finalRevision = (ULONG_PTR)AceRevision;
    }

    // Validate the incoming ACE list and compute newAceCount, while checking
    // the ACL revision supports the ACE types to be inserted.
    ULONG_PTR src = (ULONG_PTR)AceList;
    ULONG_PTR const srcEnd = src + AceListLength;
    USHORT newAceCount = 0;

    while (src < srcEnd)
    {
        if (src + sizeof(ACE_HEADER) > srcEnd)
            return STATUS_INVALID_PARAMETER;

        PACE_HEADER aceHdr = (PACE_HEADER)src;
        USHORT inSize = aceHdr->AceSize;

        if (inSize == 0 || src + inSize > srcEnd)
            return STATUS_INVALID_PARAMETER;

        // Ensure the ACL revision can host this ACE type.
        PhEnsureAclRevision(&finalRevision, aceHdr->AceType);

        src += inSize;
        ++newAceCount;
    }

    if (src != srcEnd)
        return STATUS_INVALID_PARAMETER;

    // Determine insertion point.
    PVOID insertAce = firstFree;
    ULONG existingAceCount = Acl->AceCount;

    if (StartingAceIndex < existingAceCount)
    {
        NTSTATUS status;

        status = PhGetAce(
            Acl,
            StartingAceIndex,
            &insertAce
            );

        if (!NT_SUCCESS(status))
            return status;
    }
    // else insert at end (firstFree)

    // Ensure AceListLength bytes free between end of ACL buffer and current firstFree.
    ULONG_PTR const aclStart = (ULONG_PTR)Acl;
    ULONG_PTR const aclEnd = aclStart + Acl->AclSize;

    if ((ULONG_PTR)PTR_ADD_OFFSET(firstFree, AceListLength) > (ULONG_PTR)aclEnd)
        return STATUS_BUFFER_TOO_SMALL;

    // Shift tail [insertPtr, firstFree) forward to make room.
    ULONG_PTR tailBytes = (ULONG_PTR)PTR_SUB_OFFSET(firstFree, insertAce);

    if (tailBytes > 0)
    {
        RtlMoveMemory(PTR_ADD_OFFSET(insertAce, AceListLength), insertAce, tailBytes);
    }

    // Copy new ACEs.
    RtlCopyMemory(insertAce, AceList, AceListLength);

    // Update counts and revision.
    Acl->AceCount = (USHORT)(Acl->AceCount + newAceCount);
    Acl->AclRevision = (UCHAR)finalRevision;

    return STATUS_SUCCESS;
}

FORCEINLINE
NTSTATUS
NTAPI
PhCreateAcl(
    _Out_ PACL Acl,
    _In_ ULONG Length,
    _In_ ULONG Revision
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlCreateAcl(Acl, Length, Revision);
#else
    if (Length < sizeof(ACL))
        return STATUS_BUFFER_TOO_SMALL;
    if (Length > USHRT_MAX)
        return STATUS_INVALID_PARAMETER;
    if (Revision < MIN_ACL_REVISION || Revision > MAX_ACL_REVISION)
        return STATUS_REVISION_MISMATCH;

    memset(Acl, 0, sizeof(ACL));
    Acl->AclRevision = (BYTE)Revision;
    Acl->AclSize = (USHORT)(Length & ~0x0003);
    return STATUS_SUCCESS;
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhGetDaclSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PBOOLEAN DaclPresent,
    _Outptr_result_maybenull_ PACL* Dacl,
    _Out_ PBOOLEAN DaclDefaulted
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlGetDaclSecurityDescriptor(SecurityDescriptor, DaclPresent, Dacl, DaclDefaulted);
#else
    PISECURITY_DESCRIPTOR securityDescriptor = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    BOOLEAN present = FALSE;
    BOOLEAN defaulted = FALSE;
    PACL dacl = NULL;

    if (securityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
        return STATUS_UNKNOWN_REVISION;

    if (present = BooleanFlagOn(securityDescriptor->Control, SE_DACL_PRESENT))
    {
        defaulted = BooleanFlagOn(securityDescriptor->Control, SE_DACL_DEFAULTED);

        if (BooleanFlagOn(securityDescriptor->Control, SE_SELF_RELATIVE))
        {
            PISECURITY_DESCRIPTOR_RELATIVE securityDescriptorRelative = (PISECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptor;

            if (securityDescriptorRelative->Dacl)
            {
                dacl = (PACL)RTL_PTR_ADD(SecurityDescriptor, securityDescriptorRelative->Dacl);
            }
        }
        else
        {
            dacl = securityDescriptor->Dacl;
        }
    }

    *DaclPresent = present;
    *DaclDefaulted = defaulted;
    *Dacl = dacl;
    return STATUS_SUCCESS;
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhSetDaclSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN DaclPresent,
    _In_opt_ PACL Dacl,
    _In_opt_ BOOLEAN DaclDefaulted
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlSetDaclSecurityDescriptor(SecurityDescriptor, DaclPresent, Dacl, DaclDefaulted);
#else
    if (((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Revision != SECURITY_DESCRIPTOR_REVISION)
        return STATUS_UNKNOWN_REVISION;
    if (FlagOn(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_SELF_RELATIVE))
        return STATUS_INVALID_SECURITY_DESCR;

    if (DaclPresent)
        SetFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_DACL_PRESENT);
    else
        ClearFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_DACL_PRESENT);

    if (DaclDefaulted)
        SetFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_DACL_DEFAULTED);
    else
        ClearFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_DACL_DEFAULTED);

    ((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Dacl = Dacl;
    return STATUS_SUCCESS;
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhGetDaclSecurityDescriptorNotNull(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PBOOLEAN DaclPresent,
    _Out_ PBOOLEAN DaclDefaulted,
    _Outptr_result_maybenull_ PACL* Dacl
    )
{
    NTSTATUS status;
    BOOLEAN present = FALSE;
    BOOLEAN defaulted = FALSE;
    PACL dacl = NULL;

    status = PhGetDaclSecurityDescriptor(
        SecurityDescriptor,
        &present,
        &dacl,
        &defaulted
        );

    if (NT_SUCCESS(status))
    {
        if (dacl)
        {
            *DaclPresent = present;
            *DaclDefaulted = defaulted;
            *Dacl = dacl;
        }
        else
        {
            status = STATUS_INVALID_SECURITY_DESCR;
        }
    }

    return status;
}

FORCEINLINE
NTSTATUS
NTAPI
PhGetSaclSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PBOOLEAN SaclPresent,
    _Outptr_result_maybenull_ PACL* Sacl,
    _Out_ PBOOLEAN SaclDefaulted
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlGetSaclSecurityDescriptor(SecurityDescriptor, SaclPresent, Sacl, SaclDefaulted);
#else
    PISECURITY_DESCRIPTOR securityDescriptor = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    BOOLEAN present = FALSE;
    BOOLEAN defaulted = FALSE;
    PACL sacl = NULL;

    if (securityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
        return STATUS_UNKNOWN_REVISION;

    if (present = BooleanFlagOn(securityDescriptor->Control, SE_SACL_PRESENT))
    {
        defaulted = BooleanFlagOn(securityDescriptor->Control, SE_SACL_DEFAULTED);

        if (BooleanFlagOn(securityDescriptor->Control, SE_SELF_RELATIVE))
        {
            PISECURITY_DESCRIPTOR_RELATIVE securityDescriptorRelative = (PISECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptor;

            if (securityDescriptorRelative->Sacl)
            {
                sacl = (PACL)RTL_PTR_ADD(SecurityDescriptor, securityDescriptorRelative->Sacl);
            }
        }
        else
        {
            sacl = securityDescriptor->Sacl;
        }
    }

    *SaclPresent = present;
    *SaclDefaulted = defaulted;
    *Sacl = sacl;
    return STATUS_SUCCESS;
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhSetSaclSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN SaclPresent,
    _In_opt_ PACL Sacl,
    _In_opt_ BOOLEAN SaclDefaulted
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlSetSaclSecurityDescriptor(SecurityDescriptor, SaclPresent, Sacl, SaclDefaulted);
#else
    if (((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Revision != SECURITY_DESCRIPTOR_REVISION)
        return STATUS_UNKNOWN_REVISION;
    if (FlagOn(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_SELF_RELATIVE))
        return STATUS_INVALID_SECURITY_DESCR;

    if (SaclPresent)
        SetFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_SACL_PRESENT);
    else
        ClearFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_SACL_PRESENT);

    if (SaclDefaulted)
        SetFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_SACL_DEFAULTED);
    else
        ClearFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_SACL_DEFAULTED);

    ((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Sacl = Sacl;
    return STATUS_SUCCESS;
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhGetOwnerSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Outptr_result_maybenull_ PSID* Owner,
    _Out_ PBOOLEAN OwnerDefaulted
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlGetOwnerSecurityDescriptor(SecurityDescriptor, Owner, OwnerDefaulted);
#else
    PISECURITY_DESCRIPTOR securityDescriptor = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    BOOLEAN present = FALSE;
    BOOLEAN defaulted = FALSE;
    PSID owner = NULL;

    if (securityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
        return STATUS_UNKNOWN_REVISION;

    defaulted = BooleanFlagOn(securityDescriptor->Control, SE_OWNER_DEFAULTED);

    if (BooleanFlagOn(securityDescriptor->Control, SE_SELF_RELATIVE))
    {
        PISECURITY_DESCRIPTOR_RELATIVE securityDescriptorRelative = (PISECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptor;

        if (securityDescriptorRelative->Owner)
        {
            owner = RTL_PTR_ADD(SecurityDescriptor, securityDescriptorRelative->Owner);
        }
    }
    else
    {
        owner = securityDescriptor->Sacl;
    }

    *OwnerDefaulted = defaulted;
    *Owner = owner;
    return STATUS_SUCCESS;
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhSetOwnerSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID Owner,
    _In_opt_ BOOLEAN OwnerDefaulted
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlSetOwnerSecurityDescriptor(SecurityDescriptor, Owner, OwnerDefaulted);
#else
    if (((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Revision != SECURITY_DESCRIPTOR_REVISION)
        return STATUS_UNKNOWN_REVISION;

    if (FlagOn(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_SELF_RELATIVE))
    {
        return STATUS_INVALID_SECURITY_DESCR;
    }
    else
    {
        if (OwnerDefaulted)
            SetFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_OWNER_DEFAULTED);
        else
            ClearFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_OWNER_DEFAULTED);

        ((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Owner = Owner;

        return STATUS_SUCCESS;
    }
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhGetGroupSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Outptr_result_maybenull_ PSID* Group,
    _Out_ PBOOLEAN GroupDefaulted
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlGetGroupSecurityDescriptor(SecurityDescriptor, Group, GroupDefaulted);
#else
    PISECURITY_DESCRIPTOR securityDescriptor = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    BOOLEAN present = FALSE;
    BOOLEAN defaulted = FALSE;
    PSID group = NULL;

    if (securityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
        return STATUS_UNKNOWN_REVISION;

    defaulted = BooleanFlagOn(securityDescriptor->Control, SE_GROUP_DEFAULTED);

    if (BooleanFlagOn(securityDescriptor->Control, SE_SELF_RELATIVE))
    {
        PISECURITY_DESCRIPTOR_RELATIVE securityDescriptorRelative = (PISECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptor;

        if (securityDescriptorRelative->Group)
        {
            group = RTL_PTR_ADD(SecurityDescriptor, securityDescriptorRelative->Group);
        }
    }
    else
    {
        group = securityDescriptor->Group;
    }

    *GroupDefaulted = defaulted;
    *Group = group;
    return STATUS_SUCCESS;
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhSetGroupSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID Group,
    _In_opt_ BOOLEAN GroupDefaulted
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlSetGroupSecurityDescriptor(SecurityDescriptor, Group, GroupDefaulted);
#else
    if (((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Revision != SECURITY_DESCRIPTOR_REVISION)
        return STATUS_UNKNOWN_REVISION;

    if (FlagOn(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_SELF_RELATIVE))
    {
        return STATUS_INVALID_SECURITY_DESCR;
    }
    else
    {
        if (GroupDefaulted)
            SetFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_GROUP_DEFAULTED);
        else
            ClearFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_GROUP_DEFAULTED);

        ((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Group = Group;

        return STATUS_SUCCESS;
    }
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhGetControlSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PSECURITY_DESCRIPTOR_CONTROL Control,
    _Out_ PULONG Revision
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlGetControlSecurityDescriptor(SecurityDescriptor, Control, Revision);
#else
    if (((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Revision != SECURITY_DESCRIPTOR_REVISION)
        return STATUS_UNKNOWN_REVISION;

    if (FlagOn(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_SELF_RELATIVE))
    {
        return STATUS_INVALID_SECURITY_DESCR;
    }
    else
    {
        *Control = ((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control;
        *Revision = ((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Revision;
        return STATUS_SUCCESS;
    }
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhSetControlSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    _In_ SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlSetControlSecurityDescriptor(SecurityDescriptor, ControlBitsOfInterest, ControlBitsToSet);
#else
    if (((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Revision != SECURITY_DESCRIPTOR_REVISION)
        return STATUS_UNKNOWN_REVISION;

    if (FlagOn(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, SE_SELF_RELATIVE))
    {
        return STATUS_INVALID_SECURITY_DESCR;
    }
    else
    {
        ClearFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, ControlBitsOfInterest);
        SetFlag(((PISECURITY_DESCRIPTOR)SecurityDescriptor)->Control, ControlBitsToSet);
        return STATUS_SUCCESS;
    }
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhAddAccessAllowedAceEx(
    _In_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ UCHAR AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_ PCSID Sid
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlAddAccessAllowedAceEx(Acl, AceRevision, AceFlags, AccessMask, (PSID)Sid);
#else
    PVOID offset;

    if (!PhValidSid(Sid))
        return STATUS_INVALID_SID;
    if (!PhValidAcl(Acl))
        return STATUS_INVALID_ACL;

    // Allow caller to pass any revision <= current ACL revision (matches RtlAddAce semantics). (dmex)
    if (AceRevision > Acl->AclRevision)
        return STATUS_REVISION_MISMATCH;
    if (!PhFirstFreeAce(Acl, &offset))
        return STATUS_INVALID_ACL;

    ULONG sidLength = PhLengthSid(Sid);
    ULONG aceSize = UFIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + sidLength;

    // Ensure fits into USHORT and inside ACL buffer. (dmex)
    if (aceSize >= USHRT_MAX)
        return STATUS_INVALID_BUFFER_SIZE;
    if ((ULONG_PTR)RTL_PTR_ADD(offset, aceSize) > (ULONG_PTR)RTL_PTR_ADD(Acl, Acl->AclSize))
        return STATUS_ALLOTTED_SPACE_EXCEEDED;

    PACCESS_ALLOWED_ACE ace = (PACCESS_ALLOWED_ACE)offset;
    PACE_HEADER header = &ace->Header;
    header->AceType = ACCESS_ALLOWED_ACE_TYPE;
    header->AceFlags = AceFlags;
    header->AceSize = (USHORT)aceSize;
    ace->Mask = AccessMask;
    RtlCopyMemory(&ace->SidStart, Sid, sidLength);
    Acl->AceCount++;
    return STATUS_SUCCESS;
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhAddAccessAllowedAce(
    _In_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ACCESS_MASK AccessMask,
    _In_ CONST SID* Sid
    )
{
    return PhAddAccessAllowedAceEx(Acl, AceRevision, 0, AccessMask, Sid);
}

FORCEINLINE
NTSTATUS
NTAPI
PhAcquirePebLock(
    VOID
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlAcquirePebLock();
#else
    return RtlEnterCriticalSection(NtCurrentPeb()->FastPebLock);
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhReleasePebLock(
    VOID
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlReleasePebLock();
#else
    return RtlLeaveCriticalSection(NtCurrentPeb()->FastPebLock);
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhAcquireLoaderLock(
    VOID
    )
{
    return RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);
}

FORCEINLINE
NTSTATUS
NTAPI
PhReleaseLoaderLock(
    VOID
    )
{
    return RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
}

FORCEINLINE
USHORT
NTAPI
PhGetCurrentThreadPrimaryGroup(
    VOID
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlGetCurrentThreadPrimaryGroup();
#else
    return NtCurrentTeb()->PrimaryGroupAffinity.Group;
#endif
}

FORCEINLINE
ULONG
NTAPI
PhGetCurrentServiceSessionId(
    VOID
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlGetCurrentServiceSessionId();
#else
    if (NtCurrentPeb()->SharedData && NtCurrentPeb()->SharedData->ServiceSessionId)
        return NtCurrentPeb()->SharedData->ServiceSessionId;
    else
        return 0;
#endif
}

FORCEINLINE
ULONG
NTAPI
PhGetActiveConsoleId(
    VOID
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlGetActiveConsoleId();
#else
    if (NtCurrentPeb()->SharedData && NtCurrentPeb()->SharedData->ServiceSessionId)
        return NtCurrentPeb()->SharedData->ActiveConsoleId;
    else
        return USER_SHARED_DATA->ActiveConsoleId;
#endif
}

FORCEINLINE
LONGLONG
NTAPI
PhGetConsoleSessionForegroundProcessId(
    VOID
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlGetConsoleSessionForegroundProcessId();
#else
    if (NtCurrentPeb()->SharedData && NtCurrentPeb()->SharedData->ServiceSessionId)
        return NtCurrentPeb()->SharedData->ConsoleSessionForegroundProcessId;
    else
        return USER_SHARED_DATA->ConsoleSessionForegroundProcessId;
#endif
}

FORCEINLINE
PWSTR
NTAPI
PhRtlGetNtSystemRoot(
    VOID
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlGetNtSystemRoot();
#else
    if (NtCurrentPeb()->SharedData && NtCurrentPeb()->SharedData->ServiceSessionId) // RtlGetCurrentServiceSessionId
        return NtCurrentPeb()->SharedData->NtSystemRoot;
    else
        return USER_SHARED_DATA->NtSystemRoot;
#endif
}

FORCEINLINE
BOOLEAN
NTAPI
PhAreLongPathsEnabled(
    VOID
    )
{
//#if defined(PHNT_NATIVE_INLINE)
//    return RtlAreLongPathsEnabled();
//#else
    return NtCurrentPeb()->IsLongPathAwareProcess;
//#endif
}

FORCEINLINE
VOID
NTAPI
PhFreeUnicodeString(
    _Inout_ _At_(UnicodeString->Buffer, _Frees_ptr_opt_) PUNICODE_STRING UnicodeString
    )
{
#if defined(PHNT_NATIVE_INLINE)
    RtlFreeUnicodeString(UnicodeString);
#else
    if (UnicodeString->Buffer)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeString->Buffer);
        memset(UnicodeString, 0, sizeof(UNICODE_STRING));
    }
#endif
}

FORCEINLINE
VOID
NTAPI
PhFreeAnsiString(
    _Inout_ _At_(AnsiString->Buffer, _Frees_ptr_opt_) PANSI_STRING AnsiString
    )
{
#if defined(PHNT_NATIVE_INLINE)
    RtlFreeAnsiString(AnsiString);
#else
    if (AnsiString->Buffer)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, AnsiString->Buffer);
        memset(AnsiString, 0, sizeof(ANSI_STRING));
    }
#endif
}

FORCEINLINE
VOID
NTAPI
PhFreeUTF8String(
    _Inout_ _At_(Utf8String->Buffer, _Frees_ptr_opt_) PUTF8_STRING Utf8String
    )
{
#if defined(PHNT_NATIVE_INLINE)
    RtlFreeUTF8String(Utf8String);
#else
    if (Utf8String->Buffer)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, Utf8String->Buffer);
        memset(Utf8String, 0, sizeof(UTF8_STRING));
    }
#endif
}

FORCEINLINE
PVOID
NTAPI
PhFreeSid(
    _In_ _Post_invalid_ PSID Sid
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlFreeSid(Sid);
#else
    RtlFreeHeap(RtlProcessHeap(), 0, Sid);
    return NULL;
#endif
}

FORCEINLINE
VOID
NTAPI
PhDeleteBoundaryDescriptor(
    _In_ _Post_invalid_ POBJECT_BOUNDARY_DESCRIPTOR BoundaryDescriptor
    )
{
#if defined(PHNT_NATIVE_INLINE)
    RtlDeleteBoundaryDescriptor(BoundaryDescriptor);
#else
    RtlFreeHeap(RtlProcessHeap(), 0, BoundaryDescriptor);
#endif
}

//#define RtlDeleteSecurityObject(ObjectDescriptor) RtlFreeHeap(RtlProcessHeap(), 0, *(ObjectDescriptor))
//FORCEINLINE
//NTSTATUS
//RtlDeleteSecurityObject(
//    _Inout_ PSECURITY_DESCRIPTOR *ObjectDescriptor
//    )
//{
//    RtlFreeHeap(RtlProcessHeap(), 0, *ObjectDescriptor);
//    return STATUS_SUCCESS;
//}

FORCEINLINE
NTSTATUS
NTAPI
PhDestroyEnvironment(
    _In_ _Post_invalid_ PVOID Environment
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlDestroyEnvironment(Environment);
#else
    RtlFreeHeap(RtlProcessHeap(), 0, Environment);
    return STATUS_SUCCESS;
#endif
}

FORCEINLINE
NTSTATUS
NTAPI
PhDestroyProcessParameters(
    _In_ _Post_invalid_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    )
{
#if defined(PHNT_NATIVE_INLINE)
    return RtlDestroyProcessParameters(ProcessParameters);
#else
    RtlFreeHeap(RtlProcessHeap(), 0, ProcessParameters);
    return STATUS_SUCCESS;
#endif
}

#define RtlAcquirePebLock PhAcquirePebLock
#define RtlReleasePebLock PhReleasePebLock
#define RtlGetCurrentThreadPrimaryGroup PhGetCurrentThreadPrimaryGroup
#define RtlGetCurrentServiceSessionId PhGetCurrentServiceSessionId
#define RtlGetActiveConsoleId PhGetActiveConsoleId
#define RtlGetConsoleSessionForegroundProcessId PhGetConsoleSessionForegroundProcessId
#define RtlGetNtSystemRoot PhRtlGetNtSystemRoot
#define RtlAreLongPathsEnabled PhAreLongPathsEnabled
//#define RtlFreeUnicodeString(UnicodeString) { if ((UnicodeString)->Buffer) RtlFreeHeap(RtlProcessHeap(), 0, (UnicodeString)->Buffer); memset(UnicodeString, 0, sizeof(UNICODE_STRING)); }
#define RtlFreeUnicodeString PhFreeUnicodeString
//#define RtlFreeAnsiString(UnicodeString) {if ((AnsiString)->Buffer) RtlFreeHeap(RtlProcessHeap(), 0, (AnsiString)->Buffer); memset(AnsiString, 0, sizeof(ANSI_STRING));}
#define RtlFreeAnsiString PhFreeAnsiString
//#define RtlFreeUTF8String(Utf8String) {if ((Utf8String)->Buffer) RtlFreeHeap(RtlProcessHeap(), 0, (Utf8String)->Buffer); memset(Utf8String, 0, sizeof(UTF8_STRING));}
#define RtlFreeUTF8String PhFreeUTF8String
//#define RtlFreeSid(Sid) RtlFreeHeap(RtlProcessHeap(), 0, (Sid))
#define RtlFreeSid PhFreeSid
//#define RtlDeleteBoundaryDescriptor(BoundaryDescriptor) RtlFreeHeap(RtlProcessHeap(), 0, (BoundaryDescriptor))
#define RtlDeleteBoundaryDescriptor PhDeleteBoundaryDescriptor
//#define RtlDestroyEnvironment(Environment) RtlFreeHeap(RtlProcessHeap(), 0, (Environment))
#define RtlDestroyEnvironment PhDestroyEnvironment
//#define RtlDestroyProcessParameters(ProcessParameters) RtlFreeHeap(RtlProcessHeap(), 0, (ProcessParameters))
#define RtlDestroyProcessParameters PhDestroyProcessParameters

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
NTSTATUS
NTAPI
PhSetTokenPrivilege(
    _In_ HANDLE TokenHandle,
    _In_opt_ PCWSTR PrivilegeName,
    _In_opt_ PLUID PrivilegeLuid,
    _In_ ULONG Attributes
    );

PHLIBAPI
NTSTATUS
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
    _In_opt_ PCWSTR PrivilegeName,
    _In_opt_ LONG Privilege,
    _In_ BOOLEAN Enable
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetTokenGroups(
    _In_ HANDLE TokenHandle,
    _In_opt_ PCWSTR GroupName,
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

typedef union _PH_INTEGRITY_LEVEL
{
    struct
    {
        //
        // Lower bits describe amendments to the MANDATOR_LEVEL which are a
        // combination of additional features closely related to integrity on
        // the system.
        //
        USHORT Plus : 1;
        USHORT AppContainer : 1;
        USHORT Spare : 10;

        USHORT Mandatory : 4; // MANDATORY_LEVEL
    };

    USHORT Level;
} PH_INTEGRITY_LEVEL, *PPH_INTEGRITY_LEVEL;

PHLIBAPI
NTSTATUS
NTAPI
PhGetTokenIntegrityLevelEx(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PPH_INTEGRITY_LEVEL IntegrityLevel,
    _Out_opt_ PCPH_STRINGREF* IntegrityString
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessMandatoryPolicy(
    _In_ HANDLE ProcessHandle,
    _Out_ PACCESS_MASK Mask
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessMandatoryPolicy(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK Mask
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
PhSetFileBasicInformation(
    _In_ HANDLE FileHandle,
    _In_ PFILE_BASIC_INFORMATION BasicInfo
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileFullAttributesInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
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
PhSetFileDelete(
    _In_ HANDLE FileHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetFileRename(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE RootDirectory,
    _In_ BOOLEAN ReplaceIfExists,
    _In_ PCPH_STRINGREF NewFileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileIoPriorityHint(
    _In_ HANDLE FileHandle,
    _Out_ IO_PRIORITY_HINT* IoPriorityHint
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetFileIoPriorityHint(
    _In_ HANDLE FileHandle,
    _In_ IO_PRIORITY_HINT IoPriorityHint
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFlushBuffersFile(
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
PhGetFileNetworkPhysicalName(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING* FileName
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

typedef USN *PUSN;

PHLIBAPI
NTSTATUS
NTAPI
PhGetFileUsn(
    _In_ HANDLE FileHandle,
    _Out_ PUSN Usn
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetFileBypassIO(
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN Enable
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
    _In_ PCPH_STRINGREF ObjectName
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
    _In_opt_ PCPH_STRINGREF Name,
    _In_ PCPH_STRINGREF FileName
    );

typedef _Function_class_(PH_ENUM_PROCESS_MODULES_LIMITED_CALLBACK)
NTSTATUS NTAPI PH_ENUM_PROCESS_MODULES_LIMITED_CALLBACK(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress,
    _In_ PVOID ImageBase,
    _In_ SIZE_T ImageSize,
    _In_ PPH_STRING FileName,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_PROCESS_MODULES_LIMITED_CALLBACK* PPH_ENUM_PROCESS_MODULES_LIMITED_CALLBACK;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumProcessModulesLimited(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_LIMITED_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

typedef _Function_class_(PH_ENUM_PROCESS_MODULES_RUNDOWN_CALLBACK)
NTSTATUS NTAPI PH_ENUM_PROCESS_MODULES_RUNDOWN_CALLBACK(
    _In_ PVOID ImageBase,
    _In_ SIZE_T ImageSize,
    _In_ PPH_STRING FileName,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_PROCESS_MODULES_RUNDOWN_CALLBACK* PPH_ENUM_PROCESS_MODULES_RUNDOWN_CALLBACK;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumProcessModulesRundown(
    _In_opt_ HANDLE ProcessId,
    _In_ PPH_ENUM_PROCESS_MODULES_RUNDOWN_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

/**
 * A callback function passed to PhEnumProcessModules() and called for each process module.
 *
 * \param Module A structure providing information about the module.
 * \param Context A user-defined value passed to PhEnumProcessModules().
 *
 * \return TRUE to continue the enumeration, FALSE to stop.
 */
typedef _Function_class_(PH_ENUM_PROCESS_MODULES_CALLBACK)
BOOLEAN NTAPI PH_ENUM_PROCESS_MODULES_CALLBACK(
    _In_ PLDR_DATA_TABLE_ENTRY Module,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_PROCESS_MODULES_CALLBACK* PPH_ENUM_PROCESS_MODULES_CALLBACK;

#define PH_ENUM_PROCESS_MODULES_DONT_RESOLVE_WOW64_FS 0x1
#define PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME 0x2
#define PH_ENUM_PROCESS_MODULES_LIMIT 0x800

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
PhSetProcessWorkingSetEmpty(
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
PhGetProcessPriorityClass(
    _In_ HANDLE ProcessHandle,
    _Out_ PUCHAR PriorityClass
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessPriorityClass(
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
    _In_ BOOLEAN DisablePriorityBoost
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
PhGetProcessActivityModerationState(
    _In_ PCPH_STRINGREF ModerationIdentifier,
    _Out_ PSYSTEM_ACTIVITY_MODERATION_APP_SETTINGS ModerationSettings
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetProcessActivityModerationState(
    _In_ PCPH_STRINGREF ModerationIdentifier,
    _In_ SYSTEM_ACTIVITY_MODERATION_APP_TYPE ModerationType,
    _In_ SYSTEM_ACTIVITY_MODERATION_STATE ModerationState
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
PhGetProcessPowerThrottlingState(
    _In_ HANDLE ProcessHandle,
    _Out_ PPOWER_THROTTLING_PROCESS_STATE PowerThrottlingState
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
NTSTATUS
NTAPI
PhOpenSection(
    _Out_ PHANDLE SectionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF SectionName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateSection(
    _Out_ PHANDLE SectionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PLARGE_INTEGER MaximumSize,
    _In_ ULONG SectionPageProtection,
    _In_ ULONG AllocationAttributes,
    _In_opt_ HANDLE FileHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhMapViewOfSection(
    _In_ HANDLE SectionHandle,
    _In_ HANDLE ProcessHandle,
    _Inout_ _At_(*BaseAddress, _Readable_bytes_(*ViewSize) _Writable_bytes_(*ViewSize) _Post_readable_byte_size_(*ViewSize)) PVOID *BaseAddress,
    _In_ SIZE_T CommitSize,
    _In_opt_ PLARGE_INTEGER SectionOffset,
    _Inout_ PSIZE_T ViewSize,
    _In_ SECTION_INHERIT InheritDisposition,
    _In_ ULONG AllocationType,
    _In_ ULONG PageProtection
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUnmapViewOfSection(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress
    );

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

PHLIBAPI
NTSTATUS
NTAPI
PhGetKernelFileNameEx(
    _Out_opt_ PPH_STRING* FileName,
    _Out_ PVOID* ImageBase,
    _Out_ ULONG* ImageSize
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetSecureKernelFileName(
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

typedef _Function_class_(PH_ENUM_PROCESS_THREADS)
NTSTATUS NTAPI PH_ENUM_PROCESS_THREADS(
    _In_ ULONG NumberOfThreads,
    _In_ PSYSTEM_THREAD_INFORMATION Threads,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_PROCESS_THREADS* PPH_ENUM_PROCESS_THREADS;

NTSTATUS PhEnumProcessThreads(
    _In_ HANDLE ProcessId,
    _In_ PPH_ENUM_PROCESS_THREADS Callback,
    _In_opt_ PVOID Context
    );

typedef _Function_class_(PH_ENUM_NEXT_PROCESS)
NTSTATUS NTAPI PH_ENUM_NEXT_PROCESS(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_NEXT_PROCESS* PPH_ENUM_NEXT_PROCESS;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumNextProcess(
    _In_opt_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_ENUM_NEXT_PROCESS Callback,
    _In_opt_ PVOID Context
    );

typedef _Function_class_(PH_ENUM_NEXT_THREAD)
NTSTATUS NTAPI PH_ENUM_NEXT_THREAD(
    _In_ HANDLE ThreadHandle,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_NEXT_THREAD* PPH_ENUM_NEXT_THREAD;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumNextThread(
    _In_ HANDLE ProcessHandle,
    _In_opt_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_ENUM_NEXT_THREAD Callback,
    _In_opt_ PVOID Context
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
    _In_ PCPH_STRINGREF ImageName
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
PhEnumProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_HANDLE_SNAPSHOT_INFORMATION *Handles
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumHandlesGeneric(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN EnableHandleSnapshot,
    _Out_ PSYSTEM_HANDLE_INFORMATION_EX* Handles
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
PhEnumPoolTagInformation(
    _Out_ PVOID* Buffer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumBigPoolInformation(
    _Out_ PVOID* Buffer
    );

_Must_inspect_result_
PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessIsContainer(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _Out_opt_ PBOOLEAN IsContainer
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
#define PH_CLR_VERSION_MASK 0x1f

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

PHLIBAPI
NTSTATUS
NTAPI
PhOpenDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName
    );

/**
 * A callback function passed to PhEnumDirectoryObjects() and called for each directory object.
 *
 * \param RootDirectory The handle to the current object directory.
 * \param Name The name of the object.
 * \param TypeName The name of the object's type.
 * \param Context A user-defined value passed to PhEnumDirectoryObjects().
 * \return TRUE to continue the enumeration, FALSE to stop.
 */
typedef _Function_class_(PH_ENUM_DIRECTORY_OBJECTS)
NTSTATUS NTAPI PH_ENUM_DIRECTORY_OBJECTS(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_DIRECTORY_OBJECTS* PPH_ENUM_DIRECTORY_OBJECTS;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumDirectoryObjects(
    _In_ HANDLE DirectoryHandle,
    _In_ PPH_ENUM_DIRECTORY_OBJECTS Callback,
    _In_opt_ PVOID Context
    );

typedef _Function_class_(PH_ENUM_DIRECTORY_FILE)
BOOLEAN NTAPI PH_ENUM_DIRECTORY_FILE(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_DIRECTORY_FILE* PPH_ENUM_DIRECTORY_FILE;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumDirectoryFile(
    _In_ HANDLE FileHandle,
    _In_opt_ PCPH_STRINGREF SearchPattern,
    _In_ PPH_ENUM_DIRECTORY_FILE Callback,
    _In_opt_ PVOID Context
    );

FORCEINLINE
NTSTATUS
NTAPI
PhEnumDirectoryFileZ(
    _In_ HANDLE FileHandle,
    _In_ PCWSTR SearchPattern,
    _In_ PPH_ENUM_DIRECTORY_FILE Callback,
    _In_opt_ PVOID Context
    )
{
    PH_STRINGREF searchPattern;

    PhInitializeStringRef(&searchPattern, SearchPattern);

    return PhEnumDirectoryFile(
        FileHandle,
        &searchPattern,
        Callback,
        Context
        );
}

PHLIBAPI
NTSTATUS
NTAPI
PhEnumDirectoryFileEx(
    _In_ HANDLE FileHandle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PCPH_STRINGREF SearchPattern,
    _In_ PPH_ENUM_DIRECTORY_FILE Callback,
    _In_opt_ PVOID Context
    );

FORCEINLINE
NTSTATUS
NTAPI
PhEnumDirectoryFileExZ(
    _In_ HANDLE FileHandle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_ PCWSTR SearchPattern,
    _In_ PPH_ENUM_DIRECTORY_FILE Callback,
    _In_opt_ PVOID Context
    )
{
    PH_STRINGREF searchPattern;

    PhInitializeStringRef(&searchPattern, SearchPattern);

    return PhEnumDirectoryFileEx(
        FileHandle,
        FileInformationClass,
        ReturnSingleEntry,
        &searchPattern,
        Callback,
        Context
        );
}

typedef _Function_class_(PH_ENUM_REPARSE_POINT)
NTSTATUS NTAPI PH_ENUM_REPARSE_POINT(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_ SIZE_T InformationLength,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_REPARSE_POINT* PPH_ENUM_REPARSE_POINT;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumReparsePointInformation(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_REPARSE_POINT Callback,
    _In_opt_ PVOID Context
    );

typedef _Function_class_(PH_ENUM_OBJECT_ID)
NTSTATUS NTAPI PH_ENUM_OBJECT_ID(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_ SIZE_T InformationLength,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_OBJECT_ID* PPH_ENUM_OBJECT_ID;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumObjectIdInformation(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_OBJECT_ID Callback,
    _In_opt_ PVOID Context
    );

#define PH_FIRST_FILE_EA(Information) \
    ((PFILE_FULL_EA_INFORMATION)(Information))
#define PH_NEXT_FILE_EA(Information) \
    (((PFILE_FULL_EA_INFORMATION)(Information))->NextEntryOffset ? \
    (PTR_ADD_OFFSET((Information), ((PFILE_FULL_EA_INFORMATION)(Information))->NextEntryOffset)) : \
    NULL \
    )

typedef _Function_class_(PH_ENUM_FILE_EA)
NTSTATUS NTAPI PH_ENUM_FILE_EA(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_FULL_EA_INFORMATION Information,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_FILE_EA* PPH_ENUM_FILE_EA;

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

#define PH_FIRST_LINK(Links) ((PFILE_LINK_ENTRY_INFORMATION)(Links))
#define PH_NEXT_LINK(Links) ( \
    ((PFILE_LINK_ENTRY_INFORMATION)(Links))->NextEntryOffset ? \
    (PFILE_LINK_ENTRY_INFORMATION)(PTR_ADD_OFFSET((Links), \
    ((PFILE_LINK_ENTRY_INFORMATION)(Links))->NextEntryOffset)) : \
    NULL \
    )

typedef _Function_class_(PH_ENUM_FILE_HARDLINKS)
BOOLEAN NTAPI PH_ENUM_FILE_HARDLINKS(
    _In_ PFILE_LINK_ENTRY_INFORMATION Information,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_FILE_HARDLINKS* PPH_ENUM_FILE_HARDLINKS;

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
PhQuerySymbolicLinkObject(
    _Out_ PPH_STRING* LinkTarget,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName
    );

FORCEINLINE
NTSTATUS
NTAPI
PhQuerySymbolicLinkObjectZ(
    _Out_ PPH_STRING* LinkTarget,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCWSTR ObjectName
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, ObjectName);

    return PhQuerySymbolicLinkObject(LinkTarget, RootDirectory, &name);
}

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
NTSTATUS
NTAPI
PhFlushVolumeCache(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhResolveDevicePrefix(
    _In_ PCPH_STRINGREF Name
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetFileName(
    _In_ PPH_STRING FileName
    );

// "X:\"
#define PATH_IS_WIN32_DRIVE_PREFIX(s) ( \
    (s)->Length >= (3 * sizeof(WCHAR)) && \
    (((s)->Buffer[0] >= L'A' && \
      (s)->Buffer[0] <= L'Z') || \
     ((s)->Buffer[0] >= L'a' && \
      (s)->Buffer[0] <= L'z')) && \
    (s)->Buffer[1] == L':' && \
    (s)->Buffer[2] == OBJ_NAME_PATH_SEPARATOR)

// "\??\" or "\\?\" or "\\.\"
#define PATH_IS_WIN32_DOSDEVICES_PREFIX(s) ( \
    (s)->Length >= (4 * sizeof(WCHAR)) && \
    (s)->Buffer[0] == '\\' && \
    ((s)->Buffer[1] == '?' || (s)->Buffer[1] == '\\') && \
    (s)->Buffer[2] == '?' || (s)->Buffer[2] == '.'&& \
    (s)->Buffer[3] == '\\')

// "." or ".."
#define PATH_IS_WIN32_RELATIVE_PREFIX(s) ( \
    (s)->Length == (1 * sizeof(WCHAR)) && (s)->Buffer[0] == L'.' || \
    (s)->Length == (2 * sizeof(WCHAR)) && (s)->Buffer[0] == L'.' && (s)->Buffer[1] == L'.')

PHLIBAPI
PPH_STRING
NTAPI
PhDosPathNameToNtPathName(
    _In_ PCPH_STRINGREF Name
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDosLongPathNameToNtPathNameWithStatus(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR* FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetNtPathRootPrefix(
    _In_ PCPH_STRINGREF Name
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetExistingPathPrefix(
    _In_ PCPH_STRINGREF Name
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetExistingPathPrefixWin32(
    _In_ PCPH_STRINGREF Name
    );

typedef enum _PH_MODULE_TYPE
{
    PH_MODULE_TYPE_UNKNOWN = 0,
    PH_MODULE_TYPE_MODULE = 1,
    PH_MODULE_TYPE_MAPPED_FILE = 2,
    PH_MODULE_TYPE_WOW64_MODULE = 3,
    PH_MODULE_TYPE_KERNEL_MODULE = 4,
    PH_MODULE_TYPE_MAPPED_IMAGE = 5,
    PH_MODULE_TYPE_ELF_MAPPED_IMAGE = 6,
    PH_MODULE_TYPE_ENCLAVE_MODULE = 7
} PH_MODULE_TYPE;

typedef struct _PH_MODULE_INFO
{
    ULONG Type;
    ULONG Flags;
    ULONG Size;
    ULONG EnclaveType;

    PVOID BaseAddress;
    PVOID ParentBaseAddress;
    PVOID OriginalBaseAddress;
    PVOID EntryPoint;

    PPH_STRING Name;
    PPH_STRING FileName;

    USHORT LoadOrderIndex; // -1 if N/A
    USHORT LoadCount; // -1 if N/A
    USHORT LoadReason; // -1 if N/A
    USHORT Reserved;
    LARGE_INTEGER LoadTime; // 0 if N/A

    PVOID EnclaveBaseAddress;
    SIZE_T EnclaveSize;
} PH_MODULE_INFO, *PPH_MODULE_INFO;

/**
 * A callback function passed to PhEnumGenericModules() and called for each process module.
 *
 * \param Module A structure providing information about the module.
 * \param Context A user-defined value passed to PhEnumGenericModules().
 * \return TRUE to continue the enumeration, FALSE to stop.
 */
typedef _Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
BOOLEAN NTAPI PH_ENUM_GENERIC_MODULES_CALLBACK(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_GENERIC_MODULES_CALLBACK* PPH_ENUM_GENERIC_MODULES_CALLBACK;

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
    _In_ PCPH_STRINGREF ObjectName,
    _In_ ULONG Attributes,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
    );

FORCEINLINE
NTSTATUS
NTAPI
PhCreateKeyZ(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCWSTR ObjectName,
    _In_ ULONG Attributes,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, ObjectName);

    return PhCreateKey(KeyHandle, DesiredAccess, RootDirectory, &name, Attributes, CreateOptions, Disposition);
}

PHLIBAPI
NTSTATUS
NTAPI
PhOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName,
    _In_ ULONG Attributes
    );

FORCEINLINE
NTSTATUS
NTAPI
PhOpenKeyZ(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCWSTR ObjectName,
    _In_ ULONG Attributes
    )
{
    PH_STRINGREF name;

    PhInitializeStringRef(&name, ObjectName);

    return PhOpenKey(KeyHandle, DesiredAccess, RootDirectory, &name, Attributes);
}

PHLIBAPI
NTSTATUS
NTAPI
PhLoadAppKey(
    _Out_ PHANDLE KeyHandle,
    _In_ PCPH_STRINGREF FileName,
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
PhQueryKeyInformation(
    _In_ HANDLE KeyHandle,
    _Out_opt_ PKEY_FULL_INFORMATION Information
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
    _In_opt_ PCPH_STRINGREF ValueName,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_ PVOID *Buffer
    );

FORCEINLINE
NTSTATUS
NTAPI
PhQueryValueKeyZ(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_ PVOID* Buffer
    )
{
    PH_STRINGREF valueName;

    PhInitializeStringRef(&valueName, ValueName);

    return PhQueryValueKey(
        KeyHandle,
        &valueName,
        KeyValueInformationClass,
        Buffer
        );
}

PHLIBAPI
NTSTATUS
NTAPI
PhSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PCPH_STRINGREF ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    );

FORCEINLINE
NTSTATUS
NTAPI
PhSetValueKeyZ(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    PH_STRINGREF valueName;

    PhInitializeStringRef(&valueName, ValueName);

    return PhSetValueKey(
        KeyHandle,
        &valueName,
        ValueType,
        Buffer,
        BufferLength
        );
}

FORCEINLINE
NTSTATUS
NTAPI
PhSetValueKeyStringZ(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName,
    _In_ PCPH_STRINGREF String
    )
{
    PH_STRINGREF valueName;

    if (String->Length > ULONG_MAX)
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    PhInitializeStringRef(&valueName, ValueName);

    return PhSetValueKey(
        KeyHandle,
        &valueName,
        REG_SZ,
        String->Buffer,
        (ULONG)String->Length + sizeof(UNICODE_NULL)
        );
}

FORCEINLINE
NTSTATUS
NTAPI
PhSetValueKeyString2Z(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName,
    _In_ PCWSTR String
    )
{
    PH_STRINGREF valueName;
    PH_STRINGREF valueString;

    PhInitializeStringRef(&valueName, ValueName);
    PhInitializeStringRef(&valueString, String);

    return PhSetValueKey(
        KeyHandle,
        &valueName,
        REG_SZ,
        valueString.Buffer,
        (ULONG)valueString.Length + sizeof(UNICODE_NULL)
        );
}

FORCEINLINE
NTSTATUS
NTAPI
PhSetValueKeyUlong(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName,
    _In_ ULONG Value
    )
{
    PH_STRINGREF valueName;

    PhInitializeStringRef(&valueName, ValueName);

    return PhSetValueKey(
        KeyHandle,
        &valueName,
        REG_DWORD,
        &Value,
        sizeof(ULONG)
        );
}

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PCPH_STRINGREF ValueName
    );

FORCEINLINE
NTSTATUS
NTAPI
PhDeleteValueKeyZ(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName
    )
{
    PH_STRINGREF valueName;

    PhInitializeStringRef(&valueName, ValueName);

    return PhDeleteValueKey(KeyHandle, &valueName);
}

typedef _Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI PH_ENUM_KEY_CALLBACK(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_KEY_CALLBACK* PPH_ENUM_KEY_CALLBACK;

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
PhEnumerateValueKeyEx(
    _In_ HANDLE KeyHandle,
    _In_ ULONG Index,
    _In_ KEY_VALUE_INFORMATION_CLASS InformationClass,
    _Out_ PVOID* Buffer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateFileWin32(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
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
    _In_ PCWSTR FileName,
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
PhCreateFileWin32ExAlt(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_ ULONG CreateFlags,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _Out_opt_ PULONG CreateStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ PCPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateFileEx(
    _Out_ PHANDLE FileHandle,
    _In_ PCPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
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
PhOpenFileWin32(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenFileWin32Ex(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
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
    _In_ PCPH_STRINGREF FileName,
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
PhReOpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ HANDLE OriginalFileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
    );

PHLIBAPI
NTSTATUS
NTAPI
PhReadFile(
    _In_ HANDLE FileHandle,
    _In_ PVOID Buffer,
    _In_opt_ ULONG NumberOfBytesToRead,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _Out_opt_ PULONG NumberOfBytesRead
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWriteFile(
    _In_ HANDLE FileHandle,
    _In_ PVOID Buffer,
    _In_opt_ ULONG NumberOfBytesToWrite,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _Out_opt_ PULONG NumberOfBytesWritten
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryFullAttributesFileWin32(
    _In_ PCWSTR FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryFullAttributesFile(
    _In_ PCPH_STRINGREF FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryAttributesFileWin32(
    _In_ PCWSTR FileName,
    _Out_ PFILE_BASIC_INFORMATION FileInformation
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryAttributesFile(
    _In_ PCPH_STRINGREF FileName,
    _Out_ PFILE_BASIC_INFORMATION FileInformation
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDoesFileExistWin32(
    _In_ PCWSTR FileName
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDoesFileExist(
    _In_ PCPH_STRINGREF FileName
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDoesDirectoryExistWin32(
    _In_ PCWSTR FileName
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDoesDirectoryExist(
    _In_ PCPH_STRINGREF FileName
    );

PHLIBAPI
RTL_PATH_TYPE
NTAPI
PhDetermineDosPathNameType(
    _In_ PCPH_STRINGREF FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteFileWin32(
    _In_ PCWSTR FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteFile(
    _In_ PCPH_STRINGREF FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCopyFileWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCopyFileChunkWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    );

PHLIBAPI
NTSTATUS
NTAPI
PhMoveFile(
    _In_ PCPH_STRINGREF OldFileName,
    _In_ PCPH_STRINGREF NewFileName,
    _In_ BOOLEAN FailIfExists
    );

PHLIBAPI
NTSTATUS
NTAPI
PhMoveFileWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateDirectoryWin32(
    _In_ PCPH_STRINGREF DirectoryPath
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateDirectory(
    _In_ PCPH_STRINGREF DirectoryPath
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateDirectoryFullPathWin32(
    _In_ PCPH_STRINGREF FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateDirectoryFullPath(
    _In_ PCPH_STRINGREF FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteDirectory(
    _In_ PCPH_STRINGREF DirectoryPath
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteDirectoryWin32(
    _In_ PCPH_STRINGREF DirectoryPath
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDeleteDirectoryFullPath(
    _In_ PCPH_STRINGREF FileName
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
    _In_opt_ PSECURITY_ATTRIBUTES PipeReadAttributes,
    _In_opt_ PSECURITY_ATTRIBUTES PipeWriteAttributes
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateNamedPipe(
    _Out_ PHANDLE PipeHandle,
    _In_ PCPH_STRINGREF PipeName
    );

FORCEINLINE
NTSTATUS
NTAPI
PhCreateNamedPipeZ(
    _Out_ PHANDLE PipeHandle,
    _In_ PCWSTR PipeName
    )
{
    PH_STRINGREF pipeName;

    PhInitializeStringRef(&pipeName, PipeName);

    return PhCreateNamedPipe(PipeHandle, &pipeName);
}

PHLIBAPI
NTSTATUS
NTAPI
PhConnectPipe(
    _Out_ PHANDLE PipeHandle,
    _In_ PCPH_STRINGREF PipeName
    );

FORCEINLINE
NTSTATUS
NTAPI
PhConnectPipeZ(
    _Out_ PHANDLE PipeHandle,
    _In_ PCWSTR PipeName
    )
{
    PH_STRINGREF pipeName;

    PhInitializeStringRef(&pipeName, PipeName);

    return PhConnectPipe(PipeHandle, &pipeName);
}

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
    _In_ PCWSTR PipeName,
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
PhEnumDirectoryNamedPipe(
    _In_opt_ PCPH_STRINGREF SearchPattern,
    _In_ PPH_ENUM_DIRECTORY_FILE Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhDefaultNpAcl(
    _Out_ PACL* DefaultNpAc
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetContextThread(
    _In_ HANDLE ThreadHandle,
    _Inout_ PCONTEXT ThreadContext
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
PhGetThreadAffinityMask(
    _In_ HANDLE ThreadHandle,
    _Out_ PKAFFINITY AffinityMask
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
PhSetThreadBasePriorityClientId(
    _In_ CLIENT_ID ClientId,
    _In_ KPRIORITY Increment
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
    _In_ BOOLEAN DisablePriorityBoost
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadPowerThrottlingState(
    _In_ HANDLE ThreadHandle,
    _Out_ PPOWER_THROTTLING_THREAD_STATE PowerThrottlingState
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
PhCreateImpersonationToken(
    _In_ HANDLE ThreadHandle,
    _Out_ PHANDLE TokenHandle
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

PHLIBAPI
NTSTATUS
NTAPI
PhQueryProcessHeapInformation(
    _In_ HANDLE ProcessId,
    _Out_ PPH_PROCESS_DEBUG_HEAP_INFORMATION* HeapInformation
    );

typedef _Function_class_(PH_ENUM_PROCESS_LOCKS)
NTSTATUS NTAPI PH_ENUM_PROCESS_LOCKS(
    _In_ ULONG NumberOfLocks,
    _In_ PRTL_PROCESS_LOCK_INFORMATION Locks,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_PROCESS_LOCKS* PPH_ENUM_PROCESS_LOCKS;

PHLIBAPI
NTSTATUS
NTAPI
PhQueryProcessLockInformation(
    _In_ HANDLE ProcessId,
    _In_ PPH_ENUM_PROCESS_LOCKS Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQueryVolumeInformationFile(
    _In_opt_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _In_ FS_INFORMATION_CLASS FsInformationClass,
    _Out_writes_bytes_(FsInformationLength) PVOID FsInformation,
    _In_ ULONG FsInformationLength,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMachineTypeAttributes(
    _In_ USHORT Machine,
    _Out_ MACHINE_ATTRIBUTES* Attributes
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
PhGetProcessSecurityDomain(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONGLONG SecurityDomain
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessServerSilo(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ServerSilo
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
    _Out_ PPS_SYSTEM_DLL_INIT_BLOCK SystemDllInitBlock
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessTelemetryIdInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_TELEMETRY_ID_INFORMATION* TelemetryInformation,
    _Out_opt_ PULONG TelemetryInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessTlsBitMapCounters(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG TlsBitMapCount,
    _Out_ PULONG TlsExpansionBitMapCount
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadLastStatusValue(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PNTSTATUS LastStatusValue
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessMTAUsage(
    _In_ HANDLE ProcessHandle,
    _Out_opt_ PULONG MTAInits,
    _Out_opt_ PULONG MTAIncInits
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadApartmentFlags(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ApartmentState,
    _Out_opt_ PULONG ComInits
    );

typedef enum _PH_APARTMENT_TYPE
{
    PH_APARTMENT_TYPE_INVALID = 0,
    PH_APARTMENT_TYPE_MAIN_STA,
    PH_APARTMENT_TYPE_STA,
    PH_APARTMENT_TYPE_APPLICATION_STA,
    PH_APARTMENT_TYPE_MTA,
    PH_APARTMENT_TYPE_IMPLICIT_MTA
} PH_APARTMENT_TYPE;

typedef struct _PH_APARTMENT_INFO
{
    PH_APARTMENT_TYPE Type;
    BOOLEAN InNeutral;
    ULONG ComInits;
    ULONG Flags;
} PH_APARTMENT_INFO, *PPH_APARTMENT_INFO;

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadApartment(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_APARTMENT_INFO ApartmentInfo
    );

typedef struct _PH_COM_CALLSTATE
{
    ULONG ClientPID;
    ULONG ServerPID;
    ULONG ServerTID;
    GUID ServerGuid;
} PH_COM_CALLSTATE, *PPH_COM_CALLSTATE;

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadApartmentCallState(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_COM_CALLSTATE ApartmentCallState
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadRpcState(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN HasRpcState
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadCriticalSectionOwnerThread(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessId,
    _Out_ PULONG ThreadId
    );

typedef enum _PH_THREAD_SOCKET_STATE
{
    PH_THREAD_SOCKET_STATE_NONE,
    PH_THREAD_SOCKET_STATE_SHARED,
    PH_THREAD_SOCKET_STATE_DISCONNECTED,
    PH_THREAD_SOCKET_STATE_NOT_TCPIP
} PH_THREAD_SOCKET_STATE, *PPH_THREAD_SOCKET_STATE;

PHLIBAPI
NTSTATUS
NTAPI
PhGetThreadSocketState(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_THREAD_SOCKET_STATE ThreadSocketState
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
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN ThreadIsFiber
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSwitchToThread(
    VOID
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
    _In_ PCPH_STRINGREF VariableName,
    _In_ PCPH_STRINGREF VendorGuid,
    _Out_writes_bytes_opt_(*ValueLength) PVOID* ValueBuffer,
    _Out_opt_ PULONG ValueLength,
    _Out_opt_ PULONG ValueAttributes
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetFirmwareEnvironmentVariable(
    _In_ PCPH_STRINGREF VariableName,
    _In_ PCPH_STRINGREF VendorGuid,
    _In_reads_bytes_opt_(ValueLength) PVOID ValueBuffer,
    _In_ ULONG ValueLength,
    _In_ ULONG Attributes
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
    _In_opt_ _Post_ptr_invalid_ HANDLE PowerRequestHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFreezeProcess(
    _Out_ PHANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFreezeProcessById(
    _Out_ PHANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhThawProcess(
    _In_ HANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhThawProcessById(
    _In_ HANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFreezeThread(
    _Out_ PHANDLE ThreadStateChangeHandle,
    _In_ HANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFreezeThreadById(
    _Out_ PHANDLE ThreadStateChangeHandle,
    _In_ HANDLE ThreadId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhThawThread(
    _In_ HANDLE ThreadStateChangeHandle,
    _In_ HANDLE ThreadHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhThawThreadById(
    _In_ HANDLE ThreadStateChangeHandle,
    _In_ HANDLE ThreadId
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsProcessExecutionRequired(
    _In_ HANDLE ProcessId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhProcessExecutionRequiredEnable(
    _In_ HANDLE ProcessId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhProcessExecutionRequiredDisable(
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
PhGetSystemProcessorPerformanceDistribution(
    _Out_ PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION* Buffer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSystemProcessorPerformanceDistributionEx(
    _In_ USHORT ProcessorGroup,
    _Out_ PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION* Buffer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSystemLogicalProcessorInformation(
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _Out_ PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* Buffer,
    _Out_ PULONG BufferLength
    );

typedef struct _PH_LOGICAL_PROCESSOR_INFORMATION
{
    ULONG ProcessorCoreCount;
    ULONG ProcessorNumaCount;
    ULONG ProcessorLogicalCount;
    ULONG ProcessorPackageCount;
} PH_LOGICAL_PROCESSOR_INFORMATION, *PPH_LOGICAL_PROCESSOR_INFORMATION;

PHLIBAPI
NTSTATUS
NTAPI
PhGetSystemLogicalProcessorRelationInformation(
    _Out_ PPH_LOGICAL_PROCESSOR_INFORMATION LogicalProcessorInformation
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsProcessorFeaturePresent(
    _In_ ULONG ProcessorFeature
    );

PHLIBAPI
VOID
NTAPI
PhGetCurrentProcessorNumber(
    _Out_ PPROCESSOR_NUMBER ProcessorNumber
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
PhPrefetchVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ SIZE_T NumberOfEntries,
    _In_ PMEMORY_RANGE_ENTRY VirtualAddresses
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetVirtualMemoryPagePriority(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG PagePriority,
    _In_ PVOID VirtualAddress,
    _In_ SIZE_T NumberOfBytes
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGuardGrantSuppressedCallAccess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessorNominalFrequency(
    _In_ PPH_PROCESSOR_NUMBER ProcessorNumber,
    _Out_ PULONG NominalFrequency
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
PhCreateEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ EVENT_TYPE EventType,
    _In_ BOOLEAN InitialState
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

typedef _Function_class_(PH_ENUM_MEMORY_CALLBACK)
NTSTATUS NTAPI PH_ENUM_MEMORY_CALLBACK(
    _In_ HANDLE ProcessHandle,
    _In_ PMEMORY_BASIC_INFORMATION BasicInformation,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_MEMORY_CALLBACK *PPH_ENUM_MEMORY_CALLBACK;

typedef _Function_class_(PH_ENUM_MEMORY_BULK_CALLBACK)
NTSTATUS NTAPI PH_ENUM_MEMORY_BULK_CALLBACK(
    _In_ HANDLE ProcessHandle,
    _In_ PMEMORY_BASIC_INFORMATION BasicInfo,
    _In_ SIZE_T Count,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_MEMORY_BULK_CALLBACK *PPH_ENUM_MEMORY_BULK_CALLBACK;

typedef _Function_class_(PH_ENUM_MEMORY_PAGE_CALLBACK)
NTSTATUS NTAPI PH_ENUM_MEMORY_PAGE_CALLBACK(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG_PTR NumberOfEntries,
    _In_ PMEMORY_WORKING_SET_BLOCK Blocks,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_MEMORY_PAGE_CALLBACK *PPH_ENUM_MEMORY_PAGE_CALLBACK;

typedef _Function_class_(PH_ENUM_MEMORY_ATTRIBUTE_CALLBACK)
NTSTATUS NTAPI PH_ENUM_MEMORY_ATTRIBUTE_CALLBACK(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T SizeOfImage,
    _In_ ULONG_PTR NumberOfEntries,
    _In_ PMEMORY_WORKING_SET_EX_INFORMATION Blocks,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_MEMORY_ATTRIBUTE_CALLBACK *PPH_ENUM_MEMORY_ATTRIBUTE_CALLBACK;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_MEMORY_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumVirtualMemoryBulk(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ BOOLEAN BulkQuery,
    _In_ PPH_ENUM_MEMORY_BULK_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumVirtualMemoryPages(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_MEMORY_PAGE_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumVirtualMemoryAttributes(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size,
    _In_ PPH_ENUM_MEMORY_ATTRIBUTE_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetKernelDebuggerInformation(
    _Out_opt_ PBOOLEAN KernelDebuggerEnabled,
    _Out_opt_ PBOOLEAN KernelDebuggerPresent
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsDebuggerPresent(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetDeviceType(
    _In_opt_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _Out_ DEVICE_TYPE* DeviceType
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsAppExecutionAliasTarget(
    _In_ PPH_STRING FileName
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_PROCESS_ENCLAVES_CALLBACK)(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID EnclaveAddress,
    _In_ PLDR_SOFTWARE_ENCLAVE Enclave,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumProcessEnclaves(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID LdrEnclaveList,
    _In_ PPH_ENUM_PROCESS_ENCLAVES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_PROCESS_ENCLAVE_MODULES_CALLBACK)(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_SOFTWARE_ENCLAVE Enclave,
    _In_ PVOID EntryAddress,
    _In_ PLDR_DATA_TABLE_ENTRY Entry,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumProcessEnclaveModules(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID EnclaveAddress,
    _In_ PLDR_SOFTWARE_ENCLAVE Enclave,
    _In_ PPH_ENUM_PROCESS_ENCLAVE_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessLdrTableEntryNames(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY Entry,
    _Out_ PPH_STRING* Name,
    _Out_ PPH_STRING* FileName
    );

#ifdef _M_ARM64
PHLIBAPI
VOID
NTAPI
PhEcContextToNativeContext(
    _Out_ PCONTEXT Context,
    _In_ PARM64EC_NT_CONTEXT EcContext
    );

PHLIBAPI
VOID
NTAPI
PhNativeContextToEcContext(
    _When_(InitializeEc, _Out_) _When_(!InitializeEc, _Inout_) PARM64EC_NT_CONTEXT EcContext,
    _In_ PCONTEXT Context,
    _In_ BOOLEAN InitializeEc
    );

PHLIBAPI
NTSTATUS
NTAPI
PhIsEcCode(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID CodePointer,
    _Out_ PBOOLEAN IsEcCode
    );
#endif

typedef enum _PH_MOTW_ZONE_ID
{
    PhMotwZoneIdLocalComputer,
    PhMotwZoneIdLocalIntranet,
    PhMotwZoneIdTrustedSites,
    PhMotwZoneIdInternet,
    PhMotwZoneIdRestrictedSites,
    PhMotwZoneIdUnknown,
} PH_MOTW_ZONE_ID, *PPH_MOTW_ZONE_ID;

PHLIBAPI
NTSTATUS
PhGetFileMotw(
    _In_ PCPH_STRINGREF FileName,
    _Out_opt_ PPH_MOTW_ZONE_ID ZoneId,
    _Out_opt_ PPH_STRING* ReferrerUrl,
    _Out_opt_ PPH_STRING* HostUrl
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFlushProcessHeapsRemote(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PLARGE_INTEGER Timeout
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFilterLoadUnload(
    _In_ PCPH_STRINGREF ServiceName,
    _In_ BOOLEAN LoadDriver
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFilterSendMessage(
    _In_ HANDLE Port,
    _In_reads_bytes_(InBufferSize) PVOID InBuffer,
    _In_ ULONG InBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesReturned) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_ PULONG BytesReturned
    );

typedef struct _FILTER_MESSAGE_HEADER
{
    //
    //  OUT
    //
    //  Total buffer length in bytes, including the FILTER_REPLY_HEADER, of
    //  the expected reply.  If no reply is expected, 0 is returned.
    //

    ULONG ReplyLength;

    //
    //  OUT
    //
    //  Unique Id for this message.  This will be set when the kernel message
    //  satisfies this FilterGetMessage or FilterInstanceGetMessage request.
    //  If replying to this message, this is the MessageId that should be used.
    //

    ULONGLONG MessageId;

    //
    //  General filter-specific buffer data follows...
    //

} FILTER_MESSAGE_HEADER, *PFILTER_MESSAGE_HEADER;

typedef struct _FILTER_REPLY_HEADER
{
    //
    //  IN.
    //
    //  Status of this reply. This status will be returned back to the filter
    //  driver who is waiting for a reply.
    //

    NTSTATUS Status;

    //
    //  IN
    //
    //  Unique Id for this message.  This id was returned in the
    //  FILTER_MESSAGE_HEADER from the kernel message to which we are replying.
    //

    ULONGLONG MessageId;

    //
    //  General filter-specific buffer data follows...
    //

} FILTER_REPLY_HEADER, *PFILTER_REPLY_HEADER;

PHLIBAPI
NTSTATUS
NTAPI
PhFilterGetMessage(
    _In_ HANDLE Port,
    _Out_writes_bytes_(MessageBufferSize) PFILTER_MESSAGE_HEADER MessageBuffer,
    _In_ ULONG MessageBufferSize,
    _Inout_ LPOVERLAPPED Overlapped
    );

PHLIBAPI
NTSTATUS
NTAPI
PhFilterReplyMessage(
    _In_ HANDLE Port,
    _In_reads_bytes_(ReplyBufferSize) PFILTER_REPLY_HEADER ReplyBuffer,
    _In_ ULONG ReplyBufferSize
    );

// Filter connect options: Windows 8 and above
#define FLT_PORT_FLAG_SYNC_HANDLE 0x00000001

PHLIBAPI
NTSTATUS
NTAPI
PhFilterConnectCommunicationPort(
    _In_ PCPH_STRINGREF PortName,
    _In_ ULONG Options,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ USHORT SizeOfContext,
    _In_opt_ PSECURITY_ATTRIBUTES SecurityAttributes,
    _Outptr_ PHANDLE Port
    );

EXTERN_C_END

#endif
