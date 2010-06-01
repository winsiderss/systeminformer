/*
 * Process Hacker - 
 *   native support functions
 * 
 * Copyright (C) 2009-2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ph.h>
#include <kph.h>
#include <symprv.h>

#define PH_DEVICE_PREFIX_LENGTH 64
#define PH_DEVICE_MUP_PREFIX_MAX_COUNT 16

typedef BOOLEAN (NTAPI *PPHP_ENUM_PROCESS_MODULES_CALLBACK)(
    __in HANDLE ProcessHandle,
    __in PLDR_DATA_TABLE_ENTRY Entry,
    __in PVOID AddressOfEntry,
    __in PVOID Context1,
    __in PVOID Context2
    );

PH_INITONCE PhDevicePrefixesInitOnce = PH_INITONCE_INIT;

PH_STRINGREF PhDevicePrefixes[26];
PH_QUEUED_LOCK PhDevicePrefixesLock = PH_QUEUED_LOCK_INIT;

PPH_STRING PhDeviceMupPrefixes[PH_DEVICE_MUP_PREFIX_MAX_COUNT] = { 0 };
ULONG PhDeviceMupPrefixesCount = 0;
PH_QUEUED_LOCK PhDeviceMupPrefixesLock = PH_QUEUED_LOCK_INIT;

/**
 * Opens a process.
 *
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param DesiredAccess The desired access to the process.
 * \param ProcessId The ID of the process.
 */
NTSTATUS PhOpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessId
    )
{
    OBJECT_ATTRIBUTES objectAttributes = { 0 };
    CLIENT_ID clientId;

    if (PhKphHandle)
    {
        return KphOpenProcess(
            PhKphHandle,
            ProcessHandle,
            ProcessId,
            DesiredAccess
            );
    }
    else
    {
        clientId.UniqueProcess = ProcessId;
        clientId.UniqueThread = NULL;

        return NtOpenProcess(
            ProcessHandle,
            DesiredAccess,
            &objectAttributes,
            &clientId
            );
    }
}

/**
 * Opens a thread.
 *
 * \param ThreadHandle A variable which receives a handle to the thread.
 * \param DesiredAccess The desired access to the thread.
 * \param ThreadId The ID of the thread.
 */
NTSTATUS PhOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ThreadId
    )
{
    OBJECT_ATTRIBUTES objectAttributes = { 0 };
    CLIENT_ID clientId;

    if (PhKphHandle)
    {
        return KphOpenThread(
            PhKphHandle,
            ThreadHandle,
            ThreadId,
            DesiredAccess
            );
    }
    else
    {
        clientId.UniqueProcess = NULL;
        clientId.UniqueThread = ThreadId;

        return NtOpenThread(
            ThreadHandle,
            DesiredAccess,
            &objectAttributes,
            &clientId
            );
    }
}

NTSTATUS PhOpenThreadProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ThreadHandle
    )
{
    if (PhKphHandle)
    {
        return KphOpenThreadProcess(
            PhKphHandle,
            ProcessHandle,
            ThreadHandle,
            DesiredAccess
            );
    }
    else
    {
        NTSTATUS status;
        THREAD_BASIC_INFORMATION basicInfo;

        if (!NT_SUCCESS(status = PhGetThreadBasicInformation(
            ThreadHandle,
            &basicInfo
            )))
            return status;

        return PhOpenProcess(
            ProcessHandle,
            DesiredAccess,
            basicInfo.ClientId.UniqueProcess
            );
    }
}

/**
 * Opens a process token.
 *
 * \param TokenHandle A variable which receives a handle to the token.
 * \param DesiredAccess The desired access to the token.
 * \param ProcessHandle A handle to a process.
 */
NTSTATUS PhOpenProcessToken(
    __out PHANDLE TokenHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessHandle
    )
{
    if (PhKphHandle)
    {
        return KphOpenProcessToken(
            PhKphHandle,
            TokenHandle,
            ProcessHandle,
            DesiredAccess
            );
    }
    else
    {
        return NtOpenProcessToken(
            ProcessHandle,
            DesiredAccess,
            TokenHandle
            );
    }
}

/**
 * Opens a thread token.
 *
 * \param TokenHandle A variable which receives a handle to the token.
 * \param DesiredAccess The desired access to the token.
 * \param ThreadHandle A handle to a thread.
 * \param OpenAsSelf TRUE to use the primary token for access checks, 
 * FALSE to use the impersonation token.
 */
NTSTATUS PhOpenThreadToken(
    __out PHANDLE TokenHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ThreadHandle,
    __in BOOLEAN OpenAsSelf
    )
{
    return NtOpenThreadToken(
        ThreadHandle,
        DesiredAccess,
        OpenAsSelf,
        TokenHandle
        );
}

NTSTATUS PhGetObjectSecurity(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor
    )
{
    NTSTATUS status;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySecurityObject(
        Handle,
        SecurityInformation,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySecurityObject(
            Handle,
            SecurityInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *SecurityDescriptor = (PSECURITY_DESCRIPTOR)buffer;

    return status;
}

NTSTATUS PhSetObjectSecurity(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    return NtSetSecurityObject(
        Handle,
        SecurityInformation,
        SecurityDescriptor
        );
}

/**
 * Terminates a process.
 *
 * \param ProcessHandle A handle to a process. The handle must 
 * have PROCESS_TERMINATE access.
 * \param ExitStatus A status value that indicates why the 
 * process is being terminated.
 */
NTSTATUS PhTerminateProcess(
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    )
{
    if (PhKphHandle)
    {
        return KphTerminateProcess(
            PhKphHandle,
            ProcessHandle,
            ExitStatus
            );
    }
    else
    {
        return NtTerminateProcess(
            ProcessHandle,
            ExitStatus
            );
    }
}

/**
 * Suspends a process' threads.
 *
 * \param ProcessHandle A handle to a process. The handle must 
 * have PROCESS_SUSPEND_RESUME access.
 */
NTSTATUS PhSuspendProcess(
    __in HANDLE ProcessHandle
    )
{
    if (PhKphHandle && WINDOWS_HAS_PSSUSPENDRESUMEPROCESS)
    {
        return KphSuspendProcess(PhKphHandle, ProcessHandle);
    }
    else
    {
        return NtSuspendProcess(ProcessHandle);
    }
}

/**
 * Resumes a process' threads.
 *
 * \param ProcessHandle A handle to a process. The handle must 
 * have PROCESS_SUSPEND_RESUME access.
 */
NTSTATUS PhResumeProcess(
    __in HANDLE ProcessHandle
    )
{
    if (PhKphHandle && WINDOWS_HAS_PSSUSPENDRESUMEPROCESS)
    {
        return KphResumeProcess(PhKphHandle, ProcessHandle);
    }
    else
    {
        return NtResumeProcess(ProcessHandle);
    }
}

/**
 * Terminates a thread.
 *
 * \param ThreadHandle A handle to a thread. The handle must 
 * have THREAD_TERMINATE access.
 * \param ExitStatus A status value that indicates why the 
 * thread is being terminated.
 */
NTSTATUS PhTerminateThread(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    )
{
    if (PhKphHandle)
    {
        return KphTerminateThread(
            PhKphHandle,
            ThreadHandle,
            ExitStatus
            );
    }
    else
    {
        return NtTerminateThread(
            ThreadHandle,
            ExitStatus
            );
    }
}

/**
 * Suspends a thread.
 *
 * \param ThreadHandle A handle to a thread. The handle must 
 * have THREAD_SUSPEND_RESUME access.
 * \param PreviousSuspendCount A variable which receives the 
 * number of times the thread had been suspended.
 */
NTSTATUS PhSuspendThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    )
{
    return NtSuspendThread(ThreadHandle, PreviousSuspendCount);
}

/**
 * Resumes a thread.
 *
 * \param ThreadHandle A handle to a thread. The handle must 
 * have THREAD_SUSPEND_RESUME access.
 * \param PreviousSuspendCount A variable which receives the 
 * number of times the thread had been suspended.
 */
NTSTATUS PhResumeThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    )
{
    return NtResumeThread(ThreadHandle, PreviousSuspendCount);
}

/**
 * Gets the processor context of a thread.
 *
 * \param ThreadHandle A handle to a thread. The handle must 
 * have THREAD_GET_CONTEXT access.
 * \param Context A variable which receives the context 
 * structure.
 */
NTSTATUS PhGetThreadContext(
    __in HANDLE ThreadHandle,
    __inout PCONTEXT Context
    )
{
    if (PhKphHandle)
    {
        return KphGetContextThread(PhKphHandle, ThreadHandle, Context);
    }
    else
    {
        return NtGetContextThread(ThreadHandle, Context);
    }
}

/**
 * Sets the processor context of a thread.
 *
 * \param ThreadHandle A handle to a thread. The handle must 
 * have THREAD_SET_CONTEXT access.
 * \param Context The new context structure.
 */
NTSTATUS PhSetThreadContext(
    __in HANDLE ThreadHandle,
    __in PCONTEXT Context
    )
{
    if (PhKphHandle)
    {
        return KphSetContextThread(PhKphHandle, ThreadHandle, Context);
    }
    else
    {
        return NtSetContextThread(ThreadHandle, Context);
    }
}

/**
 * Copies memory from another process into the current process.
 *
 * \param ProcessHandle A handle to a process. The handle must 
 * have PROCESS_VM_READ access.
 * \param BaseAddress The address from which memory is to be copied.
 * \param Buffer A buffer which receives the copied memory.
 * \param BufferSize The number of bytes to copy.
 * \param NumberOfBytesRead A variable which receives the number 
 * of bytes copied to the buffer.
 */
NTSTATUS PhReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead
    )
{
    // KphReadVirtualMemory is much slower than 
    // NtReadVirtualMemory, so we'll stick to 
    // the using the original system call.

    return NtReadVirtualMemory(
        ProcessHandle,
        BaseAddress,
        Buffer,
        BufferSize,
        NumberOfBytesRead
        );
}

/**
 * Copies memory from the current process into another process.
 *
 * \param ProcessHandle A handle to a process. The handle must 
 * have PROCESS_VM_WRITE access.
 * \param BaseAddress The address to which memory is to be copied.
 * \param Buffer A buffer which contains the memory to copy.
 * \param BufferSize The number of bytes to copy.
 * \param NumberOfBytesWritten A variable which receives the number 
 * of bytes copied from the buffer.
 */
NTSTATUS PhWriteVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesWritten
    )
{
    return NtWriteVirtualMemory(
        ProcessHandle,
        BaseAddress,
        Buffer,
        BufferSize,
        NumberOfBytesWritten
        );
}

/**
 * Queries variable-sized information for a process.
 * The function allocates a buffer to contain the information.
 *
 * \param ProcessHandle A handle to a process. The access required 
 * depends on the information class specified.
 * \param ProcessInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer 
 * containing the information. You must free the buffer using 
 * PhFree() when you no longer need it.
 */
NTSTATUS PhpQueryProcessVariableSize(
    __in HANDLE ProcessHandle,
    __in PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __out PPVOID Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG returnLength = 0;

    NtQueryInformationProcess(
        ProcessHandle,
        ProcessInformationClass,
        NULL,
        0,
        &returnLength
        );
    buffer = PhAllocate(returnLength);
    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessInformationClass,
        buffer,
        returnLength,
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

/**
 * Gets basic information for a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have 
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param BasicInformation A variable which receives the information.
 */
NTSTATUS PhGetProcessBasicInformation(
    __in HANDLE ProcessHandle,
    __out PPROCESS_BASIC_INFORMATION BasicInformation
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessBasicInformation,
        BasicInformation,
        sizeof(PROCESS_BASIC_INFORMATION),
        NULL
        );
}

/**
 * Gets extended basic information for a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have 
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param ExtendedBasicInformation A variable which receives the information.
 */
NTSTATUS PhGetProcessExtendedBasicInformation(
    __in HANDLE ProcessHandle,
    __out PPROCESS_EXTENDED_BASIC_INFORMATION ExtendedBasicInformation
    )
{
    ExtendedBasicInformation->Size = sizeof(PROCESS_EXTENDED_BASIC_INFORMATION);

    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessBasicInformation,
        ExtendedBasicInformation,
        sizeof(PROCESS_EXTENDED_BASIC_INFORMATION),
        NULL
        );
}

/**
 * Gets the file name of the process' image.
 *
 * \param ProcessHandle A handle to a process. The handle must 
 * have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param FileName A variable which receives a pointer to a 
 * string containing the file name. You must free the string 
 * using PhDereferenceObject() when you no longer need it.
 */
NTSTATUS PhGetProcessImageFileName(
    __in HANDLE ProcessHandle,
    __out PPH_STRING *FileName
    )
{
    NTSTATUS status;
    PUNICODE_STRING fileName;

    status = PhpQueryProcessVariableSize(
        ProcessHandle,
        ProcessImageFileName,
        &fileName
        );

    if (!NT_SUCCESS(status))
        return status;

    *FileName = PhCreateStringEx(fileName->Buffer, fileName->Length);
    PhFree(fileName);

    return status;
}

/**
 * Gets a string stored in a process' parameters structure.
 *
 * \param ProcessHandle A handle to a process. The handle must 
 * have PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ 
 * access.
 * \param Offset The string to retrieve.
 * \param String A variable which receives a pointer to the 
 * requested string. You must free the string using 
 * PhDereferenceObject() when you no longer need it.
 *
 * \retval STATUS_INVALID_PARAMETER_2 An invalid value was 
 * specified in the Offset parameter.
 */
NTSTATUS PhGetProcessPebString(
    __in HANDLE ProcessHandle,
    __in PH_PEB_OFFSET Offset,
    __out PPH_STRING *String
    )
{
    NTSTATUS status;
    PPH_STRING string;
    ULONG offset;
    PROCESS_BASIC_INFORMATION basicInfo;
    PVOID processParameters;
    UNICODE_STRING unicodeString;

    switch (Offset)
    {
    case PhpoCurrentDirectory:
        offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, CurrentDirectory);
        break;
    case PhpoDllPath:
        offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, DllPath);
        break;
    case PhpoImagePathName:
        offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, ImagePathName);
        break;
    case PhpoCommandLine:
        offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, CommandLine);
        break;
    case PhpoWindowTitle:
        offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, WindowTitle);
        break;
    case PhpoDesktopName:
        offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, DesktopInfo);
        break;
    case PhpoShellInfo:
        offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, ShellInfo);
        break;
    case PhpoRuntimeData:
        offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, RuntimeData);
        break;
    default:
        return STATUS_INVALID_PARAMETER_2;
    }

    // Get the PEB address.
    if (!NT_SUCCESS(status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo)))
        return status;

    // Read the address of the process parameters.
    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
        &processParameters,
        sizeof(PVOID),
        NULL
        )))
        return status;

    // Read the string structure.
    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(processParameters, offset),
        &unicodeString,
        sizeof(UNICODE_STRING),
        NULL
        )))
        return status;

    string = PhCreateStringEx(NULL, unicodeString.Length);

    // Read the string contents.
    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        unicodeString.Buffer,
        string->Buffer,
        string->Length,
        NULL
        )))
    {
        PhDereferenceObject(string);
        return status;
    }

    *String = string;

    return status;
}

/**
 * Gets a process' session ID.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param SessionId A variable which receives the 
 * process' session ID.
 */
NTSTATUS PhGetProcessSessionId(
    __in HANDLE ProcessHandle,
    __out PULONG SessionId
    )
{
    NTSTATUS status;
    PROCESS_SESSION_INFORMATION sessionInfo;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessSessionInformation,
        &sessionInfo,
        sizeof(PROCESS_SESSION_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *SessionId = sessionInfo.SessionId;
    }

    return status;
}

/**
 * Gets a process' no-execute status.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_INFORMATION access.
 * \param ExecuteFlags A variable which receives the 
 * no-execute flags.
 */
NTSTATUS PhGetProcessExecuteFlags(
    __in HANDLE ProcessHandle,
    __out PULONG ExecuteFlags
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessExecuteFlags,
        ExecuteFlags,
        sizeof(ULONG),
        NULL
        );
}

/**
 * Gets whether a process is running under 32-bit 
 * emulation.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param IsWow64 A variable which receives a boolean 
 * indicating whether the process is 32-bit.
 *
 * \remarks Do not use this function under a 32-bit 
 * environment.
 */
NTSTATUS PhGetProcessIsWow64(
    __in HANDLE ProcessHandle,
    __out PBOOLEAN IsWow64
    )
{
    NTSTATUS status;
    PVOID wow64;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessWow64Information,
        &wow64,
        sizeof(PVOID),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *IsWow64 = !!wow64;
    }

    return status;
}

/**
 * Gets whether a process is being debugged.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_INFORMATION access.
 * \param IsBeingDebugged A variable which receives a boolean 
 * indicating whether the process is being debugged.
 */
NTSTATUS PhGetProcessIsBeingDebugged(
    __in HANDLE ProcessHandle,
    __out PBOOLEAN IsBeingDebugged
    )
{
    NTSTATUS status;
    PVOID debugPort;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessDebugPort,
        &debugPort,
        sizeof(PVOID),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *IsBeingDebugged = !!debugPort;
    }

    return status;
}

/**
 * Gets a handle to a process' debug object.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_INFORMATION access.
 * \param DebugObjectHandle A variable which receives a 
 * handle to the debug object associated with the process. 
 * You must close the handle when you no longer need it.
 *
 * \retval STATUS_PORT_NOT_SET The process is not being 
 * debugged and has no associated debug object.
 */
NTSTATUS PhGetProcessDebugObject(
    __in HANDLE ProcessHandle,
    __out PHANDLE DebugObjectHandle
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessDebugObjectHandle,
        DebugObjectHandle,
        sizeof(HANDLE),
        NULL
        );
}

/**
 * Gets a process' I/O priority.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param IoPriority A variable which receives the I/O 
 * priority of the process.
 */
NTSTATUS PhGetProcessIoPriority(
    __in HANDLE ProcessHandle,
    __out PULONG IoPriority
    )
{
    return NtQueryInformationProcess(
        ProcessHandle,
        ProcessIoPriority,
        IoPriority,
        sizeof(ULONG),
        NULL
        );
}

/**
 * Gets a process' page priority.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param PagePriority A variable which receives the page 
 * priority of the process.
 */
NTSTATUS PhGetProcessPagePriority(
    __in HANDLE ProcessHandle,
    __out PULONG PagePriority
    )
{
    NTSTATUS status;
    PAGE_PRIORITY_INFORMATION pagePriorityInfo;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessPagePriority,
        &pagePriorityInfo,
        sizeof(PAGE_PRIORITY_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *PagePriority = pagePriorityInfo.PagePriority;
    }

    return status;
}

/**
 * Gets whether the process is running under the POSIX 
 * subsystem.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_LIMITED_INFORMATION and 
 * PROCESS_VM_READ access.
 * \param IsPosix A variable which receives a boolean 
 * indicating whether the process is running under the 
 * POSIX subsystem.
 */
NTSTATUS PhGetProcessIsPosix(
    __in HANDLE ProcessHandle,
    __out PBOOLEAN IsPosix
    )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION basicInfo;
    ULONG imageSubsystem;

    status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        return status;

    // No PEB for processes like System.
    if (!basicInfo.PebBaseAddress)
        return STATUS_UNSUCCESSFUL;

    status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ImageSubsystem)),
        &imageSubsystem,
        sizeof(ULONG),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *IsPosix = imageSubsystem == IMAGE_SUBSYSTEM_POSIX_CUI;
    }

    return status;
}

/**
 * Gets a process' cycle count.
 *
 * \param ProcessHandle A handle to a process. The handle must have 
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param CycleTime A variable which receives the 64-bit cycle 
 * time.
 */
NTSTATUS PhGetProcessCycleTime(
    __in HANDLE ProcessHandle,
    __out PULONG64 CycleTime
    )
{
    NTSTATUS status;
    PROCESS_CYCLE_TIME_INFORMATION cycleTimeInfo;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessCycleTime,
        &cycleTimeInfo,
        sizeof(PROCESS_CYCLE_TIME_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    *CycleTime = cycleTimeInfo.AccumulatedCycles.QuadPart;

    return status;
}

NTSTATUS PhGetProcessConsoleHostProcessId(
    __in HANDLE ProcessHandle,
    __out PHANDLE ConsoleHostProcessId
    )
{
    NTSTATUS status;
    PROCESS_CONSOLE_HOST_PROCESS_INFORMATION consoleHostProcessInfo;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessConsoleHostProcess,
        &consoleHostProcessInfo,
        sizeof(PROCESS_CONSOLE_HOST_PROCESS_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    *ConsoleHostProcessId = (HANDLE)consoleHostProcessInfo.ConsoleHostProcess;

    return status;
}

NTSTATUS PhGetProcessDepStatus(
    __in HANDLE ProcessHandle,
    __out PULONG DepStatus
    )
{
    NTSTATUS status;
    ULONG executeFlags;
    ULONG depStatus;

    if (!NT_SUCCESS(status = PhGetProcessExecuteFlags(
        ProcessHandle,
        &executeFlags
        )))
        return status;

    // Check if execution of data pages is enabled.
    if (executeFlags & MEM_EXECUTE_OPTION_ENABLE)
        depStatus = 0;
    // Check if execution of data pages is disabled.
    else if (executeFlags & MEM_EXECUTE_OPTION_DISABLE)
        depStatus = PH_PROCESS_DEP_ENABLED;
    // Neither flag is set in OptOut mode.
    else
        depStatus = PH_PROCESS_DEP_ENABLED;

    if (executeFlags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
        depStatus |= PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED;
    if (executeFlags & MEM_EXECUTE_OPTION_PERMANENT)
        depStatus |= PH_PROCESS_DEP_PERMANENT;

    *DepStatus = depStatus;

    return status;
}

/**
 * Gets the POSIX command line of a process.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_LIMITED_INFORMATION and 
 * PROCESS_VM_READ access.
 * \param CommandLine A variable which receives a pointer 
 * to a string containing the POSIX command line. You must 
 * free the string using PhDereferenceObject() when you no 
 * longer need it.
 *
 * \retval STATUS_UNSUCCESSFUL The command line of the 
 * process could not be retrieved because it is too large.
 *
 * \remarks Do not use this function on a non-POSIX process. 
 * Use the PhGetProcessIsPosix() function to determine 
 * whether a process is running under the POSIX subsystem.
 */
NTSTATUS PhGetProcessPosixCommandLine(
    __in HANDLE ProcessHandle,
    __out PPH_STRING *CommandLine
    )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION basicInfo;
    PVOID processParameters;
    UNICODE_STRING commandLine;

    status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        return status;

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
        &processParameters,
        sizeof(PVOID),
        NULL
        )))
        return status;

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(processParameters, FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, CommandLine)),
        &commandLine,
        sizeof(UNICODE_STRING),
        NULL
        )))
        return status;

    // See ProcessHandle.cs from PH 1.x for how POSIX command lines work.
    {
        PVOID pointer = NULL;
        PVOID firstPointer = NULL;
        PVOID lastPointer = NULL;
        BOOLEAN zeroReached = FALSE;
        ULONG i;
        ULONG commandLineChunkSize;
        PCHAR commandLineChunk;

        i = 0;

        // Read the first command line pointer + the first environment pointer.

        while (i < sizeof(PVOID) * 100) // reasonable limit
        {
            PhReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(commandLine.Buffer, i),
                &pointer,
                sizeof(PVOID),
                NULL
                );

            if (pointer && !firstPointer)
                firstPointer = pointer;
            if (zeroReached)
                lastPointer = pointer;

            i += sizeof(PVOID);

            if (zeroReached)
                break;
            else if (!pointer)
                zeroReached = TRUE;

            pointer = NULL;
        }

        commandLineChunkSize = (ULONG)((PBYTE)lastPointer - (PBYTE)firstPointer);

        // Set a limit on how much we're going to read.
        if (commandLineChunkSize > 0x1000)
            return STATUS_UNSUCCESSFUL;

        commandLineChunk = PhAllocate(commandLineChunkSize);

        // Read the chunk.
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            firstPointer,
            commandLineChunk,
            commandLineChunkSize,
            NULL
            )))
        {
            PhFree(commandLineChunk);
            return status;
        }

        // Replace the nulls in the chunk with spaces.
        for (i = 0; i < commandLineChunkSize; i++)
        {
            if (commandLineChunk[i] == 0)
            {
                commandLineChunk[i] = ' ';

                // Trim the last null/space.
                if (i == commandLineChunkSize - 1)
                {
                    commandLineChunkSize--;
                    break;
                }
            }
        }

        *CommandLine = PhCreateStringFromAnsiEx(
            commandLineChunk,
            commandLineChunkSize
            );
        PhFree(commandLineChunk);

        return status;
    }
}

/**
 * Gets a process' environment variables.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_INFORMATION and PROCESS_VM_READ 
 * access.
 * \param Variables A variable which will receive a pointer 
 * to an array of \ref PH_ENVIRONMENT_VARIABLE structures, one 
 * for each variable. You must free the array using 
 * PhFreeProcessEnvironmentVariables() when you no longer 
 * need it.
 * \param NumberOfVariables A variable which will receive 
 * the number of environment variables returned in the 
 * array.
 */
NTSTATUS PhGetProcessEnvironmentVariables(
    __in HANDLE ProcessHandle,
    __out PPH_ENVIRONMENT_VARIABLE *Variables,
    __out PULONG NumberOfVariables
    )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION basicInfo;
    PVOID processParameters;
    PVOID environment;
    ULONG environmentLength;
    PWSTR buffer;
    PPH_LIST pairsList;
    ULONG i;
    PPH_ENVIRONMENT_VARIABLE variables;

    status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        return status;

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
        &processParameters,
        sizeof(PVOID),
        NULL
        )))
        return status;

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(processParameters, FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Environment)),
        &environment,
        sizeof(PVOID),
        NULL
        )))
        return status;

    {
        MEMORY_BASIC_INFORMATION mbi;

        if (!NT_SUCCESS(status = NtQueryVirtualMemory(
            ProcessHandle,
            environment,
            MemoryBasicInformation,
            &mbi,
            sizeof(MEMORY_BASIC_INFORMATION),
            NULL
            )))
            return status;

        environmentLength = (ULONG)(mbi.RegionSize -
            ((ULONG_PTR)environment - (ULONG_PTR)mbi.BaseAddress));
    }

    // Read in the entire region of memory.

    buffer = PhAllocate(environmentLength);

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        environment,
        buffer,
        environmentLength,
        NULL
        )))
    {
        PhFree(buffer);
        return status;
    }

    // Create a list of pairs.

    pairsList = PhCreateList(20);
    i = 0;
    environmentLength /= 2;

    while (TRUE)
    {
        ULONG oldIndex;

        // Go through the buffer and stop when we reach 
        // the end of the pair string.

        oldIndex = i;

        while (i < environmentLength && buffer[i] != 0)
            i++;

        // Check if the environment block has ended 
        // (the last string is zero-length) or if we 
        // overran the buffer.
        if (i == oldIndex || i >= environmentLength)
            break;

        // Save a pointer to the current pair string.
        PhAddListItem(pairsList, &buffer[oldIndex]);
        i++; // skip the null terminator
    }

    // Create the output array.

    variables = PhAllocate(sizeof(PH_ENVIRONMENT_VARIABLE) * pairsList->Count);

    for (i = 0; i < pairsList->Count; i++)
    {
        PWSTR pairPointer;
        PWSTR valuePointer;

        pairPointer = (PWSTR)pairsList->Items[i];
        valuePointer = wcschr(pairPointer, '=');

        variables[i].Name = PhCreateStringEx(pairPointer, (valuePointer - pairPointer) * 2);
        variables[i].Value = PhCreateString(valuePointer + 1);
    }

    *Variables = variables;
    *NumberOfVariables = pairsList->Count;

    PhDereferenceObject(pairsList);
    PhFree(buffer);

    return STATUS_SUCCESS;
}

/**
 * Frees an array of environment variables returned 
 * by PhGetProcessEnvironmentVariables().
 *
 * \param Variables A pointer to an array of 
 * environment variables.
 * \param NumberOfVariables The number of environment 
 * variables in the array.
 */
VOID PhFreeProcessEnvironmentVariables(
    __in PPH_ENVIRONMENT_VARIABLE Variables,
    __in ULONG NumberOfVariables
    )
{
    ULONG i;

    for (i = 0; i < NumberOfVariables; i++)
    {
        PhDereferenceObject(Variables[i].Name);
        PhDereferenceObject(Variables[i].Value);
    }

    PhFree(Variables);
}

/**
 * Gets the file name of a mapped section.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_INFORMATION access.
 * \param BaseAddress The base address of the section view.
 * \param FileName A variable which receives a pointer to 
 * a string containing the file name of the section. You 
 * must free the string using PhDereferenceObject() when 
 * you no longer need it.
 */
NTSTATUS PhGetProcessMappedFileName(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out PPH_STRING *FileName
    )
{
    NTSTATUS status;
    PVOID buffer;
    SIZE_T bufferSize;
    SIZE_T returnLength;
    PUNICODE_STRING unicodeString;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQueryVirtualMemory(
        ProcessHandle,
        BaseAddress,
        MemoryMappedFilenameInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = NtQueryVirtualMemory(
            ProcessHandle,
            BaseAddress,
            MemoryMappedFilenameInformation,
            buffer,
            bufferSize,
            &returnLength
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    unicodeString = (PUNICODE_STRING)buffer;
    *FileName = PhCreateStringEx(
        unicodeString->Buffer,
        unicodeString->Length
        );
    PhFree(buffer);

    return status;
}

/**
 * Gets working set information for a process.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_INFORMATION and PROCESS_VM_READ 
 * access.
 * \param WorkingSetInformation A variable which receives a
 * pointer to the information. You must free the buffer using
 * PhFree() when you no longer need it.
 */
NTSTATUS PhGetProcessWorkingSetInformation(
    __in HANDLE ProcessHandle,
    __out PMEMORY_WORKING_SET_INFORMATION *WorkingSetInformation
    )
{
    NTSTATUS status;
    PVOID buffer;
    SIZE_T bufferSize;

    bufferSize = 0x8000;
    buffer = PhAllocate(bufferSize);

    while ((status = NtQueryVirtualMemory(
        ProcessHandle,
        NULL,
        MemoryWorkingSetInformation,
        buffer,
        bufferSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to over 
        // 16 MB.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *WorkingSetInformation = (PMEMORY_WORKING_SET_INFORMATION)buffer;

    return status;
}

/**
 * Gets working set counters for a process.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_INFORMATION and PROCESS_VM_READ 
 * access.
 * \param WsCounters A variable which receives the 
 * counters.
 */
NTSTATUS PhGetProcessWsCounters(
    __in HANDLE ProcessHandle,
    __out PPH_PROCESS_WS_COUNTERS WsCounters
    )
{
    NTSTATUS status;
    PMEMORY_WORKING_SET_INFORMATION wsInfo;
    PH_PROCESS_WS_COUNTERS wsCounters;
    ULONG i;

    if (!NT_SUCCESS(status = PhGetProcessWorkingSetInformation(
        ProcessHandle,
        &wsInfo
        )))
        return status;

    memset(&wsCounters, 0, sizeof(PH_PROCESS_WS_COUNTERS));

    for (i = 0; i < wsInfo->NumberOfEntries; i++)
    {
        wsCounters.NumberOfPages++;

        if (wsInfo->WorkingSetInfo[i].ShareCount > 1)
            wsCounters.NumberOfSharedPages++;
        if (wsInfo->WorkingSetInfo[i].ShareCount == 0)
            wsCounters.NumberOfPrivatePages++;
        if (wsInfo->WorkingSetInfo[i].Shared)
            wsCounters.NumberOfShareablePages++;
    }

    PhFree(wsInfo);

    *WsCounters = wsCounters;

    return status;
}

NTSTATUS PhEnumProcessHandles(
    __in HANDLE ProcessHandle,
    __out PPROCESS_HANDLE_INFORMATION *Handles
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 2048;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = KphQueryProcessHandles(
            PhKphHandle,
            ProcessHandle,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Handles = buffer;

    return status;
}

/**
 * Sets a process' affinity mask.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_SET_INFORMATION access.
 * \param AffinityMask The new affinity mask.
 */
NTSTATUS PhSetProcessAffinityMask(
    __in HANDLE ProcessHandle,
    __in ULONG_PTR AffinityMask
    )
{
    return NtSetInformationProcess(
        ProcessHandle,
        ProcessAffinityMask,
        &AffinityMask,
        sizeof(ULONG_PTR)
        );
}

/**
 * Sets a process' I/O priority.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_SET_INFORMATION access.
 * \param IoPriority The new I/O priority.
 *
 * \remarks This function requires a valid KProcessHacker 
 * handle.
 */
NTSTATUS PhSetProcessIoPriority(
    __in HANDLE ProcessHandle,
    __in ULONG IoPriority
    )
{
    return KphSetInformationProcess(
        PhKphHandle,
        ProcessHandle,
        ProcessIoPriority,
        &IoPriority,
        sizeof(ULONG)
        );
}

/**
 * Sets a process' no-execute status.
 *
 * \param ProcessHandle A handle to a process.
 * \param ExecuteFlags The new no-execute flags.
 *
 * \remarks This function requires a valid KProcessHacker 
 * handle.
 */
NTSTATUS PhSetProcessExecuteFlags(
    __in HANDLE ProcessHandle,
    __in ULONG ExecuteFlags
    )
{
    return KphSetExecuteOptions(
        PhKphHandle,
        ProcessHandle,
        ExecuteFlags
        );
}

NTSTATUS PhSetProcessDepStatus(
    __in HANDLE ProcessHandle,
    __in ULONG DepStatus
    )
{
    ULONG executeFlags;

    if (DepStatus & PH_PROCESS_DEP_ENABLED)
        executeFlags = MEM_EXECUTE_OPTION_DISABLE;
    else
        executeFlags = MEM_EXECUTE_OPTION_ENABLE;

    if (DepStatus & PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED)
        executeFlags |= MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION;
    if (DepStatus & PH_PROCESS_DEP_PERMANENT)
        executeFlags |= MEM_EXECUTE_OPTION_PERMANENT;

    return PhSetProcessExecuteFlags(ProcessHandle, executeFlags);
}

NTSTATUS PhSetProcessDepStatusInvasive(
    __in HANDLE ProcessHandle,
    __in ULONG DepStatus,
    __in ULONG Timeout
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    PVOID setProcessDepPolicy;
    ULONG flags;

    setProcessDepPolicy = PhGetProcAddress(L"kernel32.dll", "SetProcessDEPPolicy");

    if (!setProcessDepPolicy)
        return STATUS_NOT_SUPPORTED;

    flags = 0;

    if (DepStatus & PH_PROCESS_DEP_ENABLED)
        flags |= PROCESS_DEP_ENABLE;
    if (DepStatus & PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED)
        flags |= PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION;

    if (WindowsVersion >= WINDOWS_VISTA)
    {
        status = RtlCreateUserThread(
            ProcessHandle,
            NULL,
            FALSE,
            0,
            0,
            0,
            (PUSER_THREAD_START_ROUTINE)setProcessDepPolicy,
            (PVOID)flags,
            &threadHandle,
            NULL
            );
    }
    else
    {
        if (!(threadHandle = CreateRemoteThread(
            ProcessHandle,
            NULL,
            0,
            (PTHREAD_START_ROUTINE)setProcessDepPolicy,
            (PVOID)flags,
            0,
            NULL
            )))
        {
            status = NTSTATUS_FROM_WIN32(GetLastError());
        }
    }

    if (!NT_SUCCESS(status))
        return status;

    // Wait for the thread to finish.
    WaitForSingleObject(threadHandle, Timeout);
    NtClose(threadHandle);

    return status;
}

/**
 * Causes a process to load a DLL.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_CREATE_THREAD, PROCESS_VM_OPERATION 
 * and PROCESS_VM_WRITE access.
 * \param FileName The file name of the DLL to inject.
 * \param Timeout The timeout, in milliseconds, for the 
 * process to load the DLL.
 *
 * \remarks If the process does not load the DLL before 
 * the timeout expires it may crash. Choose the timeout 
 * value carefully.
 */
NTSTATUS PhInjectDllProcess(
    __in HANDLE ProcessHandle,
    __in PWSTR FileName,
    __in ULONG Timeout
    )
{
    NTSTATUS status;
    PVOID baseAddress = NULL;
    SIZE_T stringSize;
    SIZE_T allocSize;
    HANDLE threadHandle;

    stringSize = (wcslen(FileName) + 1) * 2;
    allocSize = stringSize;

    if (!NT_SUCCESS(status = NtAllocateVirtualMemory(
        ProcessHandle,
        &baseAddress,
        0,
        &allocSize,
        MEM_COMMIT,
        PAGE_READWRITE
        )))
        return status;

    if (!NT_SUCCESS(status = PhWriteVirtualMemory(
        ProcessHandle,
        baseAddress,
        FileName,
        stringSize,
        NULL
        )))
        goto FreeExit;

    // Vista seems to support native threads better than XP.
    if (WindowsVersion >= WINDOWS_VISTA)
    {
        if (!NT_SUCCESS(status = RtlCreateUserThread(
            ProcessHandle,
            NULL,
            FALSE,
            0,
            0,
            0,
            (PUSER_THREAD_START_ROUTINE)PhGetProcAddress(L"kernel32.dll", "LoadLibraryW"),
            baseAddress,
            &threadHandle,
            NULL
            )))
            goto FreeExit;
    }
    else
    {
        if (!CreateRemoteThread(
            ProcessHandle,
            NULL,
            0,
            (PTHREAD_START_ROUTINE)PhGetProcAddress(L"kernel32.dll", "LoadLibraryW"),
            baseAddress,
            0,
            NULL
            ))
        {
            status = NTSTATUS_FROM_WIN32(GetLastError());
            goto FreeExit;
        }
    }

    // Wait for the thread to finish.
    WaitForSingleObject(threadHandle, Timeout);
    NtClose(threadHandle);

FreeExit:
    // Size needs to be zero if we're freeing.
    allocSize = 0;
    NtFreeVirtualMemory(
        ProcessHandle,
        &baseAddress,
        &allocSize,
        MEM_RELEASE
        );

    return status;
}

/**
 * Causes a process to unload a DLL.
 *
 * \param ProcessHandle A handle to a process. The handle 
 * must have PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_CREATE_THREAD,
 * PROCESS_VM_OPERATION, PROCESS_VM_READ and PROCESS_VM_WRITE access.
 * \param BaseAddress The base address of the DLL to unload.
 * \param Timeout The timeout, in milliseconds, for the 
 * process to unload the DLL.
 */
NTSTATUS PhUnloadDllProcess(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in ULONG Timeout
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    THREAD_BASIC_INFORMATION basicInfo;

    status = PhSetProcessModuleLoadCount(
        ProcessHandle,
        BaseAddress,
        1
        );

    if (!NT_SUCCESS(status))
        return status;

    if (WindowsVersion >= WINDOWS_VISTA)
    {
        status = RtlCreateUserThread(
            ProcessHandle,
            NULL,
            FALSE,
            0,
            0,
            0,
            (PUSER_THREAD_START_ROUTINE)PhGetProcAddress(L"ntdll.dll", "LdrUnloadDll"),
            BaseAddress,
            &threadHandle,
            NULL
            );
    }
    else
    {
        if (!(threadHandle = CreateRemoteThread(
            ProcessHandle,
            NULL,
            0,
            (PTHREAD_START_ROUTINE)PhGetProcAddress(L"kernel32.dll", "FreeLibrary"),
            BaseAddress,
            0,
            NULL
            )))
        {
            status = NTSTATUS_FROM_WIN32(GetLastError());
        }
    }

    if (!NT_SUCCESS(status))
        return status;

    if (WaitForSingleObject(threadHandle, Timeout) == STATUS_WAIT_0)
    {
        status = PhGetThreadBasicInformation(threadHandle, &basicInfo);

        if (NT_SUCCESS(status))
        {
            if (WindowsVersion >= WINDOWS_VISTA)
                status = basicInfo.ExitStatus;
            else
                status = basicInfo.ExitStatus != 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        status = STATUS_TIMEOUT;
    }

    NtClose(threadHandle);

    return status;
}

/**
 * Gets basic information for a thread.
 *
 * \param ThreadHandle A handle to a thread. The handle must have 
 * THREAD_QUERY_LIMITED_INFORMATION access.
 * \param BasicInformation A variable which receives the information.
 */
NTSTATUS PhGetThreadBasicInformation(
    __in HANDLE ThreadHandle,
    __out PTHREAD_BASIC_INFORMATION BasicInformation
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadBasicInformation,
        BasicInformation,
        sizeof(THREAD_BASIC_INFORMATION),
        NULL
        );
}

/**
 * Gets a thread's I/O priority.
 *
 * \param ThreadHandle A handle to a thread. The handle 
 * must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param IoPriority A variable which receives the I/O 
 * priority of the thread.
 */
NTSTATUS PhGetThreadIoPriority(
    __in HANDLE ThreadHandle,
    __out PULONG IoPriority
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadIoPriority,
        IoPriority,
        sizeof(ULONG),
        NULL
        );
}

/**
 * Gets a thread's page priority.
 *
 * \param ThreadHandle A handle to a thread. The handle 
 * must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param PagePriority A variable which receives the page 
 * priority of the thread.
 */
NTSTATUS PhGetThreadPagePriority(
    __in HANDLE ThreadHandle,
    __out PULONG PagePriority
    )
{
    NTSTATUS status;
    PAGE_PRIORITY_INFORMATION pagePriorityInfo;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadPagePriority,
        &pagePriorityInfo,
        sizeof(PAGE_PRIORITY_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *PagePriority = pagePriorityInfo.PagePriority;
    }

    return status;
}

/**
 * Gets a thread's cycle count.
 *
 * \param ThreadHandle A handle to a thread. The handle must have 
 * THREAD_QUERY_LIMITED_INFORMATION access.
 * \param CycleTime A variable which receives the 64-bit cycle 
 * time.
 */
NTSTATUS PhGetThreadCycleTime(
    __in HANDLE ThreadHandle,
    __out PULONG64 CycleTime
    )
{
    NTSTATUS status;
    THREAD_CYCLE_TIME_INFORMATION cycleTimeInfo;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadCycleTime,
        &cycleTimeInfo,
        sizeof(THREAD_CYCLE_TIME_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    *CycleTime = cycleTimeInfo.AccumulatedCycles.QuadPart;

    return status;
}

/**
 * Sets a thread's I/O priority.
 *
 * \param ThreadHandle A handle to a thread. The handle 
 * must have THREAD_SET_LIMITED_INFORMATION access.
 * \param IoPriority The new I/O priority.
 *
 * \remarks This function requires a valid KProcessHacker 
 * handle.
 */
NTSTATUS PhSetThreadIoPriority(
    __in HANDLE ThreadHandle,
    __in ULONG IoPriority
    )
{
    return KphSetInformationThread(
        PhKphHandle,
        ThreadHandle,
        ThreadIoPriority,
        &IoPriority,
        sizeof(ULONG)
        );
}

/**
 * Converts a STACKFRAME64 structure to a 
 * PH_THREAD_STACK_FRAME structure.
 *
 * \param StackFrame64 A pointer to the STACKFRAME64 structure
 * to convert.
 * \param ThreadStackFrame A pointer to the resulting 
 * PH_THREAD_STACK_FRAME structure.
 */
VOID PhpConvertStackFrame(
    __in STACKFRAME64 *StackFrame64,
    __out PPH_THREAD_STACK_FRAME ThreadStackFrame
    )
{
    ULONG i;

    ThreadStackFrame->PcAddress = (PVOID)StackFrame64->AddrPC.Offset;
    ThreadStackFrame->ReturnAddress = (PVOID)StackFrame64->AddrReturn.Offset;
    ThreadStackFrame->FrameAddress = (PVOID)StackFrame64->AddrFrame.Offset;
    ThreadStackFrame->StackAddress = (PVOID)StackFrame64->AddrStack.Offset;
    ThreadStackFrame->BStoreAddress = (PVOID)StackFrame64->AddrBStore.Offset;

    for (i = 0; i < 4; i++)
        ThreadStackFrame->Params[i] = (PVOID)StackFrame64->Params[i];
}

/**
 * Walks a thread's stack.
 *
 * \param ThreadHandle A handle to a thread. The handle 
 * must have THREAD_GET_CONTEXT and THREAD_SUSPEND_RESUME 
 * access. The handle can have any access for kernel stack 
 * walking.
 * \param ProcessHandle A handle to the thread's parent 
 * process. The handle must have PROCESS_QUERY_INFORMATION 
 * and PROCESS_VM_READ access. If a symbol provider is 
 * being used, pass its process handle.
 * \param Flags A combination of flags.
 * \li \c PH_WALK_I386_STACK Walks the x86 stack. On AMD64 
 * systems this flag walks the WOW64 stack.
 * \li \c PH_WALK_AMD64_STACK Walks the AMD64 stack. On x86 
 * systems this flag is ignored.
 * \li \c PH_WALK_KERNEL_STACK Walks the kernel stack. This 
 * flag is ignored if there is no active KProcessHacker
 * connection.
 * \param Callback A callback function which is executed 
 * for each stack frame.
 * \param Context A user-defined value to pass to the 
 * callback function.
 */
NTSTATUS PhWalkThreadStack(
    __in HANDLE ThreadHandle,
    __in_opt HANDLE ProcessHandle,
    __in ULONG Flags,
    __in PPH_WALK_THREAD_STACK_CALLBACK Callback,
    __in PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN suspended = FALSE;
    BOOLEAN processOpened = FALSE;

    // Open a handle to the process if we weren't given one.
    if (!ProcessHandle)
    {
        if (!NT_SUCCESS(status = PhOpenThreadProcess(
            &ProcessHandle,
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            ThreadHandle
            )))
            return status;

        processOpened = TRUE;
    }

    // Suspend the thread to avoid inaccurate results.
    if (NT_SUCCESS(NtSuspendThread(ThreadHandle, NULL)))
        suspended = TRUE;

    // Kernel stack walk.
    if ((Flags & PH_WALK_KERNEL_STACK) && PhKphHandle)
    {
        PVOID stack[62 - 1]; // 62 limit for XP and Server 2003.
        ULONG capturedFrames;
        ULONG i;

        if (NT_SUCCESS(KphCaptureStackBackTraceThread(
            PhKphHandle,
            ThreadHandle,
            1,
            sizeof(stack) / sizeof(PVOID),
            stack,
            &capturedFrames,
            NULL
            )))
        {
            PH_THREAD_STACK_FRAME threadStackFrame;

            memset(&threadStackFrame, 0, sizeof(PH_THREAD_STACK_FRAME));

            for (i = 0; i < capturedFrames; i++)
            {
                threadStackFrame.PcAddress = stack[i];

                if (!Callback(&threadStackFrame, Context))
                {
                    goto ResumeExit;
                }
            }
        }
    }

#ifdef _M_X64
    if (Flags & PH_WALK_AMD64_STACK)
    {
        STACKFRAME64 stackFrame;
        PH_THREAD_STACK_FRAME threadStackFrame;
        CONTEXT context;

        context.ContextFlags = CONTEXT_ALL;

        if (!NT_SUCCESS(status = PhGetThreadContext(
            ThreadHandle,
            &context
            )))
            goto SkipAmd64Stack;

        memset(&stackFrame, 0, sizeof(STACKFRAME64));
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = context.Rip;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Rsp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Rbp;

        while (TRUE)
        {
            if (!PhStackWalk(
                IMAGE_FILE_MACHINE_AMD64,
                ProcessHandle,
                ThreadHandle,
                &stackFrame,
                &context,
                NULL,
                NULL,
                NULL,
                NULL
                ))
                break;

            // If we have an invalid instruction pointer, break.
            if (!stackFrame.AddrPC.Offset || stackFrame.AddrPC.Offset == -1)
                break;

            // Convert the stack frame and execute the callback.

            PhpConvertStackFrame(&stackFrame, &threadStackFrame);

            if (!Callback(&threadStackFrame, Context))
                goto ResumeExit;
        }
    }

SkipAmd64Stack:
#endif

    // x86/WOW64 stack walk.
    if (Flags & PH_WALK_I386_STACK)
    {
        STACKFRAME64 stackFrame;
        PH_THREAD_STACK_FRAME threadStackFrame;
#ifndef _M_X64
        CONTEXT context;

        context.ContextFlags = CONTEXT_ALL;

        if (!NT_SUCCESS(status = PhGetThreadContext(
            ThreadHandle,
            &context
            )))
            goto SkipI386Stack;
#else
        WOW64_CONTEXT context;

        context.ContextFlags = WOW64_CONTEXT_ALL;

        if (!NT_SUCCESS(status = NtQueryInformationThread(
            ThreadHandle,
            ThreadWow64Context,
            &context,
            sizeof(WOW64_CONTEXT),
            NULL
            )))
            goto SkipI386Stack;
#endif

        memset(&stackFrame, 0, sizeof(STACKFRAME64));
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = context.Eip;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Esp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Ebp;

        while (TRUE)
        {
            if (!PhStackWalk(
                IMAGE_FILE_MACHINE_I386,
                ProcessHandle,
                ThreadHandle,
                &stackFrame,
                &context,
                NULL,
                NULL,
                NULL,
                NULL
                ))
                break;

            // If we have an invalid instruction pointer, break.
            if (!stackFrame.AddrPC.Offset || stackFrame.AddrPC.Offset == -1)
                break;

            // Convert the stack frame and execute the callback.

            PhpConvertStackFrame(&stackFrame, &threadStackFrame);

            if (!Callback(&threadStackFrame, Context))
                goto ResumeExit;
        }
    }

SkipI386Stack:

ResumeExit:
    if (suspended)
        NtResumeThread(ThreadHandle, NULL);

    if (processOpened)
        NtClose(ProcessHandle);

    return status;
}

NTSTATUS PhGetJobBasicAndIoAccounting(
    __in HANDLE JobHandle,
    __out PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION BasicAndIoAccounting
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectBasicAndIoAccountingInformation,
        BasicAndIoAccounting,
        sizeof(JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION),
        NULL
        );
}

NTSTATUS PhGetJobBasicLimits(
    __in HANDLE JobHandle,
    __out PJOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimits
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectBasicLimitInformation,
        BasicLimits,
        sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION),
        NULL
        );
}

NTSTATUS PhGetJobExtendedLimits(
    __in HANDLE JobHandle,
    __out PJOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimits
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectExtendedLimitInformation,
        ExtendedLimits,
        sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION),
        NULL
        );
}

NTSTATUS PhGetJobBasicUiRestrictions(
    __in HANDLE JobHandle,
    __out PJOBOBJECT_BASIC_UI_RESTRICTIONS BasicUiRestrictions
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectBasicUIRestrictions,
        BasicUiRestrictions,
        sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS),
        NULL
        );
}

NTSTATUS PhGetJobProcessIdList(
    __in HANDLE JobHandle,
    __out PJOBOBJECT_BASIC_PROCESS_ID_LIST *ProcessIdList
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQueryInformationJobObject(
        JobHandle,
        JobObjectBasicProcessIdList,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQueryInformationJobObject(
            JobHandle,
            JobObjectBasicProcessIdList,
            buffer,
            bufferSize,
            NULL
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *ProcessIdList = (PJOBOBJECT_BASIC_PROCESS_ID_LIST)buffer;

    return status;
}

/**
 * Queries variable-sized information for a token.
 * The function allocates a buffer to contain the information.
 *
 * \param TokenHandle A handle to a token. The access required 
 * depends on the information class specified.
 * \param TokenInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer 
 * containing the information. You must free the buffer using 
 * PhFree() when you no longer need it.
 */
NTSTATUS PhpQueryTokenVariableSize(
    __in HANDLE TokenHandle,
    __in TOKEN_INFORMATION_CLASS TokenInformationClass,
    __out PPVOID Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG returnLength = 0;

    NtQueryInformationToken(
        TokenHandle,
        TokenInformationClass,
        NULL,
        0,
        &returnLength
        );
    buffer = PhAllocate(returnLength);
    status = NtQueryInformationToken(
        TokenHandle,
        TokenInformationClass,
        buffer,
        returnLength,
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

/**
 * Gets a token's user.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param User A variable which receives a pointer to 
 * a structure containing the token's user. You must 
 * free the structure using PhFree() when you no longer 
 * need it.
 */
NTSTATUS PhGetTokenUser(
    __in HANDLE TokenHandle,
    __out PTOKEN_USER *User
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenUser,
        User
        );
}

/**
 * Gets a token's owner.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param Owner A variable which receives a pointer to 
 * a structure containing the token's owner. You must 
 * free the structure using PhFree() when you no longer 
 * need it.
 */
NTSTATUS PhGetTokenOwner(
    __in HANDLE TokenHandle,
    __out PTOKEN_OWNER *Owner
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenOwner,
        Owner
        );
}

/**
 * Gets a token's primary group.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param PrimaryGroup A variable which receives a pointer to 
 * a structure containing the token's primary group. You must 
 * free the structure using PhFree() when you no longer 
 * need it.
 */
NTSTATUS PhGetTokenPrimaryGroup(
    __in HANDLE TokenHandle,
    __out PTOKEN_PRIMARY_GROUP *PrimaryGroup
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenPrimaryGroup,
        PrimaryGroup
        );
}

/**
 * Gets a token's session ID.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param SessionId A variable which receives the 
 * session ID.
 */
NTSTATUS PhGetTokenSessionId(
    __in HANDLE TokenHandle,
    __out PULONG SessionId
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenSessionId,
        SessionId,
        sizeof(ULONG),
        &returnLength
        );
}

/**
 * Gets a token's elevation type.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param ElevationType A variable which receives the 
 * elevation type.
 */
NTSTATUS PhGetTokenElevationType(
    __in HANDLE TokenHandle,
    __out PTOKEN_ELEVATION_TYPE ElevationType
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenElevationType,
        ElevationType,
        sizeof(TOKEN_ELEVATION_TYPE),
        &returnLength
        );
}

/**
 * Gets whether a token is elevated.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param Elevated A variable which receives a 
 * boolean indicating whether the token is elevated.
 */
NTSTATUS PhGetTokenIsElevated(
    __in HANDLE TokenHandle,
    __out PBOOLEAN Elevated
    )
{
    NTSTATUS status;
    TOKEN_ELEVATION elevation;
    ULONG returnLength;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenElevation,
        &elevation,
        sizeof(TOKEN_ELEVATION),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *Elevated = !!elevation.TokenIsElevated;
    }

    return status;
}

/**
 * Gets a token's statistics.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param Statistics A variable which receives the 
 * token's statistics.
 */
NTSTATUS PhGetTokenStatistics(
    __in HANDLE TokenHandle,
    __out PTOKEN_STATISTICS Statistics
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenStatistics,
        Statistics,
        sizeof(TOKEN_STATISTICS),
        &returnLength
        );
}

/**
 * Gets a token's source.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY_SOURCE access.
 * \param Source A variable which receives the 
 * token's source.
 */
NTSTATUS PhGetTokenSource(
    __in HANDLE TokenHandle,
    __out PTOKEN_SOURCE Source
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenSource,
        Source,
        sizeof(TOKEN_SOURCE),
        &returnLength
        );
}

/**
 * Gets a token's groups.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param Groups A variable which receives a pointer to 
 * a structure containing the token's groups. You must 
 * free the structure using PhFree() when you no longer 
 * need it.
 */
NTSTATUS PhGetTokenGroups(
    __in HANDLE TokenHandle,
    __out PTOKEN_GROUPS *Groups
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenGroups,
        Groups
        );
}

/**
 * Gets a token's privileges.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param Privileges A variable which receives a pointer to 
 * a structure containing the token's privileges. You must 
 * free the structure using PhFree() when you no longer 
 * need it.
 */
NTSTATUS PhGetTokenPrivileges(
    __in HANDLE TokenHandle,
    __out PTOKEN_PRIVILEGES *Privileges
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenPrivileges,
        Privileges
        );
}

/**
 * Gets a handle to a token's linked token.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param LinkedTokenHandle A variable which receives a 
 * handle to the linked token. You must close the handle 
 * using NtClose() when you no longer need it.
 */
NTSTATUS PhGetTokenLinkedToken(
    __in HANDLE TokenHandle,
    __out PHANDLE LinkedTokenHandle
    )
{
    NTSTATUS status;
    ULONG returnLength;
    TOKEN_LINKED_TOKEN linkedToken;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenLinkedToken,
        &linkedToken,
        sizeof(TOKEN_LINKED_TOKEN),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    *LinkedTokenHandle = linkedToken.LinkedToken;

    return status;
}

/**
 * Gets whether virtualization is allowed for a token.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param IsVirtualizationAllowed A variable which receives 
 * a boolean indicating whether virtualization is allowed 
 * for the token.
 */
NTSTATUS PhGetTokenIsVirtualizationAllowed(
    __in HANDLE TokenHandle,
    __out PBOOLEAN IsVirtualizationAllowed
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG virtualizationAllowed;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenVirtualizationAllowed,
        &virtualizationAllowed,
        sizeof(ULONG),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    *IsVirtualizationAllowed = !!virtualizationAllowed;

    return status;
}

/**
 * Gets whether virtualization is enabled for a token.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param IsVirtualizationEnabled A variable which receives 
 * a boolean indicating whether virtualization is enabled 
 * for the token.
 */
NTSTATUS PhGetTokenIsVirtualizationEnabled(
    __in HANDLE TokenHandle,
    __out PBOOLEAN IsVirtualizationEnabled
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG virtualizationEnabled;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenVirtualizationEnabled,
        &virtualizationEnabled,
        sizeof(ULONG),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    *IsVirtualizationEnabled = !!virtualizationEnabled;

    return status;
}

NTSTATUS PhSetTokenSessionId(
    __in HANDLE TokenHandle,
    __in ULONG SessionId
    )
{
    return NtSetInformationToken(
        TokenHandle,
        TokenSessionId,
        &SessionId,
        sizeof(ULONG)
        );
}

/**
 * Modifies a token privilege.
 *
 * \param TokenHandle A handle to a token. The handle
 * must have TOKEN_ADJUST_PRIVILEGES access.
 * \param PrivilegeName The name of the privilege to 
 * modify. If this parameter is NULL, you must specify 
 * a LUID in the \a PrivilegeLuid parameter.
 * \param PrivilegeLuid The LUID of the privilege to 
 * modify. If this parameter is NULL, you must specify 
 * a name in the \a PrivilegeName parameter.
 * \param Attributes The new attributes of the privilege.
 */
BOOLEAN PhSetTokenPrivilege(
    __in HANDLE TokenHandle,
    __in_opt PWSTR PrivilegeName,
    __in_opt PLUID PrivilegeLuid,
    __in ULONG Attributes
    )
{
    NTSTATUS status;
    TOKEN_PRIVILEGES privileges;

    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = Attributes;

    if (PrivilegeLuid)
    {
        privileges.Privileges[0].Luid = *PrivilegeLuid;
    }
    else if (PrivilegeName)
    {
        if (!PhLookupPrivilegeValue(
            PrivilegeName,
            &privileges.Privileges[0].Luid
            ))
            return FALSE;
    }
    else
    {
        return FALSE;
    }

    if (!NT_SUCCESS(status = NtAdjustPrivilegesToken(
        TokenHandle,
        FALSE,
        &privileges,
        0,
        NULL,
        NULL
        )))
        return FALSE;

    if (status == STATUS_NOT_ALL_ASSIGNED)
        return FALSE;

    return TRUE;
}

/**
 * Sets whether virtualization is enabled for a token.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_WRITE access.
 * \param IsVirtualizationEnabled A boolean indicating 
 * whether virtualization is to be enabled for the token.
 */
NTSTATUS PhSetTokenIsVirtualizationEnabled(
    __in HANDLE TokenHandle,
    __in BOOLEAN IsVirtualizationEnabled
    )
{
    ULONG virtualizationEnabled;

    virtualizationEnabled = IsVirtualizationEnabled;

    return NtSetInformationToken(
        TokenHandle,
        TokenVirtualizationEnabled,
        &virtualizationEnabled,
        sizeof(ULONG)
        );
}

/**
 * Gets a token's integrity level.
 *
 * \param TokenHandle A handle to a token. The handle 
 * must have TOKEN_QUERY access.
 * \param IntegrityLevel A variable which receives 
 * the integrity level of the token.
 * \param IntegrityString A variable which receives a 
 * pointer to a string containing a string representation 
 * of the integrity level. You must free the string 
 * using PhDereferenceObject() when you no longer need it.
 */
NTSTATUS PhGetTokenIntegrityLevel(
    __in HANDLE TokenHandle,
    __out_opt PPH_INTEGRITY IntegrityLevel, 
    __out_opt PPH_STRING *IntegrityString
    )
{
    NTSTATUS status;
    PTOKEN_GROUPS groups;
    ULONG i;
    PPH_STRING sidName = NULL;
    PH_INTEGRITY integrityLevel;
    PPH_STRING integrityString;

    status = PhGetTokenGroups(TokenHandle, &groups);

    if (!NT_SUCCESS(status))
        return status;

    // Look for an integrity SID.
    for (i = 0; i < groups->GroupCount; i++)
    {
        if (groups->Groups[i].Attributes & SE_GROUP_INTEGRITY_ENABLED)
        {
            PhLookupSid(groups->Groups[i].Sid, &sidName, NULL, NULL);
            break;
        }
    }

    PhFree(groups);

    // Did we get the SID name successfully?
    if (!sidName)
        return STATUS_UNSUCCESSFUL;

    // Look for " Mandatory Level".
    i = PhStringIndexOfString(sidName, 0, L" Mandatory Level");

    if (i == -1)
    {
        PhDereferenceObject(sidName);
        return STATUS_UNSUCCESSFUL;
    }

    // Get the string before the suffix.
    integrityString = PhSubstring(sidName, 0, i);
    PhDereferenceObject(sidName);

    // Compute the integer integrity level.
    if (PhStringEquals2(integrityString, L"Untrusted", FALSE))
        integrityLevel = PiUntrusted;
    else if (PhStringEquals2(integrityString, L"Low", FALSE))
        integrityLevel = PiLow;
    else if (PhStringEquals2(integrityString, L"Medium", FALSE))
        integrityLevel = PiMedium;
    else if (PhStringEquals2(integrityString, L"High", FALSE))
        integrityLevel = PiHigh;
    else if (PhStringEquals2(integrityString, L"System", FALSE))
        integrityLevel = PiSystem;
    else if (PhStringEquals2(integrityString, L"Installer", FALSE))
        integrityLevel = PiInstaller;
    else
        integrityLevel = -1;

    if (integrityLevel == -1)
    {
        PhDereferenceObject(integrityString);
        return STATUS_UNSUCCESSFUL;
    }

    if (IntegrityLevel)
    {
        *IntegrityLevel = integrityLevel;
    }

    if (IntegrityString)
    {
        *IntegrityString = integrityString;
    }
    else
    {
        PhDereferenceObject(integrityString);
    }

    return status;
}

NTSTATUS PhGetEventBasicInformation(
    __in HANDLE EventHandle,
    __out PEVENT_BASIC_INFORMATION BasicInformation
    )
{
    return NtQueryEvent(
        EventHandle,
        EventBasicInformation,
        BasicInformation,
        sizeof(EVENT_BASIC_INFORMATION),
        NULL
        );
}

NTSTATUS PhGetMutantBasicInformation(
    __in HANDLE MutantHandle,
    __out PMUTANT_BASIC_INFORMATION BasicInformation
    )
{
    return NtQueryMutant(
        MutantHandle,
        MutantBasicInformation,
        BasicInformation,
        sizeof(MUTANT_BASIC_INFORMATION),
        NULL
        );
}

NTSTATUS PhGetMutantOwnerInformation(
    __in HANDLE MutantHandle,
    __out PMUTANT_OWNER_INFORMATION OwnerInformation
    )
{
    return NtQueryMutant(
        MutantHandle,
        MutantOwnerInformation,
        OwnerInformation,
        sizeof(MUTANT_OWNER_INFORMATION),
        NULL
        );
}

NTSTATUS PhGetSectionBasicInformation(
    __in HANDLE SectionHandle,
    __out PSECTION_BASIC_INFORMATION BasicInformation
    )
{
    return NtQuerySection(
        SectionHandle,
        SectionBasicInformation,
        BasicInformation,
        sizeof(SECTION_BASIC_INFORMATION),
        NULL
        );
}

NTSTATUS PhGetSemaphoreBasicInformation(
    __in HANDLE SemaphoreHandle,
    __out PSEMAPHORE_BASIC_INFORMATION BasicInformation
    )
{
    return NtQuerySemaphore(
        SemaphoreHandle,
        SemaphoreBasicInformation,
        BasicInformation,
        sizeof(SEMAPHORE_BASIC_INFORMATION),
        NULL
        );
}

NTSTATUS PhGetTimerBasicInformation(
    __in HANDLE TimerHandle,
    __out PTIMER_BASIC_INFORMATION BasicInformation
    )
{
    return NtQueryTimer(
        TimerHandle,
        TimerBasicInformation,
        BasicInformation,
        sizeof(TIMER_BASIC_INFORMATION),
        NULL
        );
}

NTSTATUS PhpQueryFileVariableSize(
    __in HANDLE FileHandle,
    __in FILE_INFORMATION_CLASS FileInformationClass,
    __out PPVOID Buffer
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PVOID buffer;
    ULONG bufferSize = 0x200;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryInformationFile(
            FileHandle,
            &isb,
            buffer,
            bufferSize,
            FileInformationClass
            );

        if (
            status == STATUS_BUFFER_OVERFLOW ||
            status == STATUS_BUFFER_TOO_SMALL ||
            status == STATUS_INFO_LENGTH_MISMATCH
            )
        {
            PhFree(buffer);
            bufferSize *= 2;
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

NTSTATUS PhGetFileSize(
    __in HANDLE FileHandle,
    __out PLARGE_INTEGER Size
    )
{
    NTSTATUS status;
    FILE_STANDARD_INFORMATION standardInfo;
    IO_STATUS_BLOCK isb;

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        &standardInfo,
        sizeof(FILE_STANDARD_INFORMATION),
        FileStandardInformation
        );

    if (!NT_SUCCESS(status))
        return status;

    *Size = standardInfo.EndOfFile;

    return status;
}

NTSTATUS PhSetFileSize(
    __in HANDLE FileHandle,
    __in PLARGE_INTEGER Size
    )
{
    FILE_END_OF_FILE_INFORMATION endOfFileInfo;
    IO_STATUS_BLOCK isb;

    endOfFileInfo.EndOfFile = *Size;

    return NtSetInformationFile(
        FileHandle,
        &isb,
        &endOfFileInfo,
        sizeof(FILE_END_OF_FILE_INFORMATION),
        FileEndOfFileInformation
        );
}

NTSTATUS PhpQueryTransactionManagerVariableSize(
    __in HANDLE TransactionManagerHandle,
    __in TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
    __out PPVOID Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;

    if (!NtQueryInformationTransactionManager)
        return STATUS_NOT_SUPPORTED;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryInformationTransactionManager(
            TransactionManagerHandle,
            TransactionManagerInformationClass,
            buffer,
            bufferSize,
            NULL
            );

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            bufferSize *= 2;

            if (bufferSize > 1 * 1024 * 1024)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

NTSTATUS PhGetTransactionManagerBasicInformation(
    __in HANDLE TransactionManagerHandle,
    __out PTRANSACTIONMANAGER_BASIC_INFORMATION BasicInformation
    )
{
    if (NtQueryInformationTransactionManager)
    {
        return NtQueryInformationTransactionManager(
            TransactionManagerHandle,
            TransactionManagerBasicInformation,
            BasicInformation,
            sizeof(TRANSACTIONMANAGER_BASIC_INFORMATION),
            NULL
            );
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS PhGetTransactionManagerLogFileName(
    __in HANDLE TransactionManagerHandle,
    __out PPH_STRING *LogFileName
    )
{
    NTSTATUS status;
    PTRANSACTIONMANAGER_LOGPATH_INFORMATION logPathInfo;

    status = PhpQueryTransactionManagerVariableSize(
        TransactionManagerHandle,
        TransactionManagerLogPathInformation,
        &logPathInfo
        );

    if (!NT_SUCCESS(status))
        return status;

    *LogFileName = PhCreateStringEx(
        logPathInfo->LogPath,
        logPathInfo->LogPathLength
        );
    PhFree(logPathInfo);

    return status;
}

NTSTATUS PhpQueryTransactionVariableSize(
    __in HANDLE TransactionHandle,
    __in TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
    __out PPVOID Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;

    if (!NtQueryInformationTransaction)
        return STATUS_NOT_SUPPORTED;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryInformationTransaction(
            TransactionHandle,
            TransactionInformationClass,
            buffer,
            bufferSize,
            NULL
            );

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            bufferSize *= 2;

            if (bufferSize > 1 * 1024 * 1024)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

NTSTATUS PhGetTransactionBasicInformation(
    __in HANDLE TransactionHandle,
    __out PTRANSACTION_BASIC_INFORMATION BasicInformation
    )
{
    if (NtQueryInformationTransaction)
    {
        return NtQueryInformationTransaction(
            TransactionHandle,
            TransactionBasicInformation,
            BasicInformation,
            sizeof(TRANSACTION_BASIC_INFORMATION),
            NULL
            );
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS PhGetTransactionPropertiesInformation(
    __in HANDLE TransactionHandle,
    __out_opt PLARGE_INTEGER Timeout,
    __out_opt TRANSACTION_OUTCOME *Outcome,
    __out_opt PPH_STRING *Description
    )
{
    NTSTATUS status;
    PTRANSACTION_PROPERTIES_INFORMATION propertiesInfo;

    status = PhpQueryTransactionVariableSize(
        TransactionHandle,
        TransactionPropertiesInformation,
        &propertiesInfo
        );

    if (!NT_SUCCESS(status))
        return status;

    if (Timeout)
    {
        *Timeout = propertiesInfo->Timeout;
    }

    if (Outcome)
    {
        *Outcome = propertiesInfo->Outcome;
    }

    if (Description)
    {
        *Description = PhCreateStringEx(
            propertiesInfo->Description,
            propertiesInfo->DescriptionLength
            );
    }

    PhFree(propertiesInfo);

    return status;
}

NTSTATUS PhpQueryResourceManagerVariableSize(
    __in HANDLE ResourceManagerHandle,
    __in RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
    __out PPVOID Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;

    if (!NtQueryInformationResourceManager)
        return STATUS_NOT_SUPPORTED;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryInformationResourceManager(
            ResourceManagerHandle,
            ResourceManagerInformationClass,
            buffer,
            bufferSize,
            NULL
            );

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            bufferSize *= 2;

            if (bufferSize > 1 * 1024 * 1024)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

NTSTATUS PhGetResourceManagerBasicInformation(
    __in HANDLE ResourceManagerHandle,
    __out_opt PGUID Guid,
    __out_opt PPH_STRING *Description
    )
{
    NTSTATUS status;
    PRESOURCEMANAGER_BASIC_INFORMATION basicInfo;

    status = PhpQueryResourceManagerVariableSize(
        ResourceManagerHandle,
        ResourceManagerBasicInformation,
        &basicInfo
        );

    if (!NT_SUCCESS(status))
        return status;

    if (Guid)
    {
        *Guid = basicInfo->ResourceManagerId;
    }

    if (Description)
    {
        *Description = PhCreateStringEx(
            basicInfo->Description,
            basicInfo->DescriptionLength
            );
    }

    PhFree(basicInfo);

    return status;
}

NTSTATUS PhGetEnlistmentBasicInformation(
    __in HANDLE EnlistmentHandle,
    __out PENLISTMENT_BASIC_INFORMATION BasicInformation
    )
{
    if (NtQueryInformationEnlistment)
    {
        return NtQueryInformationEnlistment(
            EnlistmentHandle,
            EnlistmentBasicInformation,
            BasicInformation,
            sizeof(ENLISTMENT_BASIC_INFORMATION),
            NULL
            );
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }
}

typedef struct _OPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT
{
    NTSTATUS Status;
    PVOID BaseAddress;
    HANDLE DriverHandle;
} OPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT, *POPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT;

BOOLEAN NTAPI PhpOpenDriverByBaseAddressCallback(
    __in PPH_STRING Name,
    __in PPH_STRING TypeName,
    __in PVOID Context
    )
{
    NTSTATUS status;
    POPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT context = Context;
    PPH_STRING driverName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE driverHandle;
    DRIVER_BASIC_INFORMATION basicInfo;

    driverName = PhConcatStrings2(L"\\Driver\\", Name->Buffer);
    InitializeObjectAttributes(
        &objectAttributes,
        &driverName->us,
        0,
        NULL,
        NULL
        );

    status = KphOpenDriver(PhKphHandle, &driverHandle, &objectAttributes);
    PhDereferenceObject(driverName);

    if (!NT_SUCCESS(status))
        return TRUE;

    status = KphQueryInformationDriver(
        PhKphHandle,
        driverHandle,
        DriverBasicInformation,
        &basicInfo,
        sizeof(DRIVER_BASIC_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (basicInfo.DriverStart == context->BaseAddress)
        {
            context->Status = STATUS_SUCCESS;
            context->DriverHandle = driverHandle;

            return FALSE;
        }
    }

    NtClose(driverHandle);

    return TRUE;
}

/**
 * Opens a driver object using a base address.
 *
 * \param DriverHandle A variable which receives a 
 * handle to the driver object.
 * \param BaseAddress The base address of the driver 
 * to open.
 *
 * \retval STATUS_OBJECT_NAME_NOT_FOUND The driver could 
 * not be found.
 *
 * \remarks This function requires a valid KProcessHacker 
 * handle. 
 */
NTSTATUS PhOpenDriverByBaseAddress(
    __out PHANDLE DriverHandle,
    __in PVOID BaseAddress
    )
{
    NTSTATUS status;
    UNICODE_STRING driverDirectoryName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE driverDirectoryHandle;
    OPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT context;

    RtlInitUnicodeString(
        &driverDirectoryName,
        L"\\Driver"
        );
    InitializeObjectAttributes(
        &objectAttributes,
        &driverDirectoryName,
        0,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status = NtOpenDirectoryObject(
        &driverDirectoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        )))
        return status;

    context.Status = STATUS_OBJECT_NAME_NOT_FOUND;
    context.BaseAddress = BaseAddress;

    status = PhEnumDirectoryObjects(
        driverDirectoryHandle,
        PhpOpenDriverByBaseAddressCallback,
        &context
        );
    NtClose(driverDirectoryHandle);

    if (!NT_SUCCESS(status) && !NT_SUCCESS(context.Status))
        return status;

    if (NT_SUCCESS(context.Status))
    {
        *DriverHandle = context.DriverHandle;
    }

    return context.Status;
}

/**
 * Queries variable-sized information for a driver.
 * The function allocates a buffer to contain the information.
 *
 * \param DriverHandle A handle to a driver. The access required 
 * depends on the information class specified.
 * \param DriverInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer 
 * containing the information. You must free the buffer using 
 * PhFree() when you no longer need it.
 *
 * \remarks This function requires a valid KProcessHacker 
 * handle.
 */
NTSTATUS PhpQueryDriverVariableSize(
    __in HANDLE DriverHandle,
    __in DRIVER_INFORMATION_CLASS DriverInformationClass,
    __out PPVOID Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG returnLength = 0;

    KphQueryInformationDriver(
        PhKphHandle,
        DriverHandle,
        DriverInformationClass,
        NULL,
        0,
        &returnLength
        );
    buffer = PhAllocate(returnLength);
    status = KphQueryInformationDriver(
        PhKphHandle,
        DriverHandle,
        DriverInformationClass,
        buffer,
        returnLength,
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

/**
 * Gets the service key name of a driver.
 *
 * \param DriverHandle A handle to a driver.
 * \param ServiceKeyName A variable which receives a pointer 
 * to a string containing the service key name. You must 
 * free the string using PhDereferenceObject() when you no 
 * longer need it.
 *
 * \remarks This function requires a valid KProcessHacker 
 * handle.
 */
NTSTATUS PhGetDriverServiceKeyName(
    __in HANDLE DriverHandle,
    __out PPH_STRING *ServiceKeyName
    )
{
    NTSTATUS status;
    PUNICODE_STRING unicodeString;

    if (!NT_SUCCESS(status = PhpQueryDriverVariableSize(
        DriverHandle,
        DriverServiceKeyNameInformation,
        &unicodeString
        )))
        return status;

    *ServiceKeyName = PhCreateStringEx(
        unicodeString->Buffer,
        unicodeString->Length
        );
    PhFree(unicodeString);

    return status;
}

NTSTATUS PhpUnloadDriver(
    __in PPH_STRING ServiceKeyName
    )
{
    NTSTATUS status;
    ULONG win32Result;
    HKEY servicesKeyHandle;
    HKEY serviceKeyHandle;
    ULONG disposition;
    PPH_STRING servicePath;

    if ((win32Result = RegCreateKey(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services",
        &servicesKeyHandle
        )) != ERROR_SUCCESS)
    {
        return NTSTATUS_FROM_WIN32(win32Result);
    }

    if ((win32Result = RegCreateKeyEx(
        servicesKeyHandle,
        ServiceKeyName->Buffer,
        0,
        NULL,
        0,
        KEY_WRITE,
        NULL,
        &serviceKeyHandle,
        &disposition
        )) != ERROR_SUCCESS)
    {
        RegCloseKey(servicesKeyHandle);
        return NTSTATUS_FROM_WIN32(win32Result);
    }

    if (disposition == REG_CREATED_NEW_KEY)
    {
        ULONG dword;
        PPH_STRING string;

        // Set up the required values.
        dword = 1;
        RegSetValueEx(serviceKeyHandle, L"ErrorControl", 0, REG_DWORD, (PBYTE)&dword, sizeof(ULONG));
        RegSetValueEx(serviceKeyHandle, L"Start", 0, REG_DWORD, (PBYTE)&dword, sizeof(ULONG));
        RegSetValueEx(serviceKeyHandle, L"Type", 0, REG_DWORD, (PBYTE)&dword, sizeof(ULONG));

        // Use a bogus name.
        string = PhCreateString(L"\\SystemRoot\\system32\\drivers\\ntfs.sys");
        RegSetValueEx(serviceKeyHandle, L"ImagePath", 0, REG_SZ, (PBYTE)string->Buffer, string->Length + 2);
        PhDereferenceObject(string);
    }

    servicePath = PhConcatStrings2(
        L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\",
        ServiceKeyName->Buffer
        );
    status = NtUnloadDriver(&servicePath->us);
    PhDereferenceObject(servicePath);

    if (disposition == REG_CREATED_NEW_KEY)
    {
        RegDeleteKey(servicesKeyHandle, ServiceKeyName->Buffer);
    }

    RegCloseKey(serviceKeyHandle);
    RegCloseKey(servicesKeyHandle);

    return status;
}

/**
 * Unloads a driver.
 *
 * \param BaseAddress The base address of the driver. 
 * This parameter can be NULL if a value is specified 
 * in \c Name.
 * \param Name The base name of the driver. This 
 * parameter can be NULL if a value is specified in 
 * \c BaseAddress and KProcessHacker is loaded.
 *
 * \retval STATUS_INVALID_PARAMETER_MIX Both 
 * \c BaseAddress and \c Name were null, or \c Name 
 * was not specified and KProcessHacker is not loaded.
 * \retval STATUS_OBJECT_NAME_NOT_FOUND The driver 
 * could not be found.
 */
NTSTATUS PhUnloadDriver(
    __in_opt PVOID BaseAddress,
    __in_opt PWSTR Name
    )
{
    NTSTATUS status;
    HANDLE driverHandle;
    PPH_STRING serviceKeyName = NULL;

    if (!BaseAddress && !Name)
        return STATUS_INVALID_PARAMETER_MIX;
    if (!Name && !PhKphHandle)
        return STATUS_INVALID_PARAMETER_MIX;

    // Try to get the service key name by scanning the 
    // Driver directory.

    if (PhKphHandle && BaseAddress)
    {
        if (NT_SUCCESS(PhOpenDriverByBaseAddress(
            &driverHandle,
            BaseAddress
            )))
        {
            PhGetDriverServiceKeyName(driverHandle, &serviceKeyName);
            NtClose(driverHandle);
        }
    }

    // Use the base name if we didn't get the service 
    // key name.

    if (!serviceKeyName && Name)
    {
        PPH_STRING name;

        name = PhCreateString(Name);

        // Remove the extension if it is present.
        if (PhStringEndsWith2(name, L".sys", TRUE))
        {
            serviceKeyName = PhSubstring(name, 0, name->Length / 2 - 4);
            PhDereferenceObject(name);
        }
        else
        {
            serviceKeyName = name;
        }
    }

    if (!serviceKeyName)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    status = PhpUnloadDriver(serviceKeyName);
    PhDereferenceObject(serviceKeyName);

    return status;
}

/**
 * Duplicates a handle.
 *
 * \param SourceProcessHandle A handle to the source 
 * process. The handle must have PROCESS_DUP_HANDLE 
 * access.
 * \param SourceHandle The handle to duplicate from 
 * the source process.
 * \param TargetProcessHandle A handle to the target 
 * process. If DUPLICATE_CLOSE_SOURCE is specified 
 * in the \a Options parameter, this parameter can be 
 * NULL.
 * \param TargetHandle A variable which receives 
 * the new handle in the target process. If 
 * DUPLICATE_CLOSE_SOURCE is specified in the \a Options 
 * parameter, this parameter can be NULL.
 * \param DesiredAccess The desired access to the 
 * object referenced by the source handle.
 * \param HandleAttributes The attributes to apply 
 * to the new handle.
 * \param Options The options to use when duplicating 
 * the handle.
 */
NTSTATUS PhDuplicateObject(
    __in HANDLE SourceProcessHandle,
    __in HANDLE SourceHandle,
    __in_opt HANDLE TargetProcessHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options
    )
{
    if (PhKphHandle)
    {
        return KphDuplicateObject(
            PhKphHandle,
            SourceProcessHandle,
            SourceHandle,
            TargetProcessHandle,
            TargetHandle,
            DesiredAccess,
            HandleAttributes,
            Options
            );
    }
    else
    {
        return NtDuplicateObject(
            SourceProcessHandle,
            SourceHandle,
            TargetProcessHandle,
            TargetHandle,
            DesiredAccess,
            HandleAttributes,
            Options
            );
    }
}

NTSTATUS PhpEnumProcessModules(
    __in HANDLE ProcessHandle,
    __in PPHP_ENUM_PROCESS_MODULES_CALLBACK Callback,
    __in PVOID Context1,
    __in PVOID Context2
    )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION basicInfo;
    PVOID ldr;
    PEB_LDR_DATA pebLdrData;
    PLIST_ENTRY startLink;
    PLIST_ENTRY currentLink;
    LDR_DATA_TABLE_ENTRY currentEntry;
    ULONG i;

    // Get the PEB address.
    status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        return status;

    // Read the address of the loader data.
    status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, Ldr)),
        &ldr,
        sizeof(PVOID),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Read the loader data.
    status = PhReadVirtualMemory(
        ProcessHandle,
        ldr,
        &pebLdrData,
        sizeof(PEB_LDR_DATA),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!pebLdrData.Initialized)
        return STATUS_UNSUCCESSFUL;

    // Traverse the linked list (in load order).

    i = 0;
    startLink = PTR_ADD_OFFSET(ldr, FIELD_OFFSET(PEB_LDR_DATA, InLoadOrderModuleList));
    currentLink = pebLdrData.InLoadOrderModuleList.Flink;

    while (
        currentLink != startLink &&
        i <= PH_ENUM_PROCESS_MODULES_ITERS
        )
    {
        PVOID addressOfEntry;

        addressOfEntry = CONTAINING_RECORD(currentLink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        status = PhReadVirtualMemory(
            ProcessHandle,
            addressOfEntry,
            &currentEntry,
            LDR_DATA_TABLE_ENTRY_SIZE,
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;

        // Make sure the entry is valid.
        if (currentEntry.DllBase)
        {
            // Execute the callback.
            if (!Callback(
                ProcessHandle,
                &currentEntry,
                addressOfEntry,
                Context1,
                Context2
                ))
                break;
        }

        currentLink = currentEntry.InLoadOrderLinks.Flink;
        i++;
    }

    return status;
}

BOOLEAN NTAPI PhpEnumProcessModulesCallback(
    __in HANDLE ProcessHandle,
    __in PLDR_DATA_TABLE_ENTRY Entry,
    __in PVOID AddressOfEntry,
    __in PVOID Context1,
    __in PVOID Context2
    )
{
    BOOLEAN cont;
    PWSTR baseDllNameBuffer;
    PWSTR fullDllNameBuffer;

    // Read the base DLL name string and add a null terminator.

    baseDllNameBuffer = PhAllocate(Entry->BaseDllName.Length + 2);

    if (NT_SUCCESS(PhReadVirtualMemory(
        ProcessHandle,
        Entry->BaseDllName.Buffer,
        baseDllNameBuffer,
        Entry->BaseDllName.Length,
        NULL
        )))
    {
        baseDllNameBuffer[Entry->BaseDllName.Length / 2] = 0;
        Entry->BaseDllName.Buffer = baseDllNameBuffer;
    }

    // Read the full DLL name string and add a null terminator.

    fullDllNameBuffer = PhAllocate(Entry->FullDllName.Length + 2);

    if (NT_SUCCESS(PhReadVirtualMemory(
        ProcessHandle,
        Entry->FullDllName.Buffer,
        fullDllNameBuffer,
        Entry->FullDllName.Length,
        NULL
        )))
    {
        fullDllNameBuffer[Entry->FullDllName.Length / 2] = 0;
        Entry->FullDllName.Buffer = fullDllNameBuffer;
    }

    // Execute the callback.
    cont = ((PPH_ENUM_PROCESS_MODULES_CALLBACK)Context1)(Entry, Context2);

    PhFree(baseDllNameBuffer);
    PhFree(fullDllNameBuffer);

    return cont;
}

/**
 * Enumerates the modules loaded by a process.
 *
 * \param ProcessHandle A handle to a process. The 
 * handle must have PROCESS_QUERY_LIMITED_INFORMATION 
 * and PROCESS_VM_READ access.
 * \param Callback A callback function which is 
 * executed for each process module.
 * \param Context A user-defined value to pass to the 
 * callback function.
 */
NTSTATUS PhEnumProcessModules(
    __in HANDLE ProcessHandle,
    __in PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    __in PVOID Context
    )
{
    return PhpEnumProcessModules(
        ProcessHandle,
        PhpEnumProcessModulesCallback,
        Callback,
        Context
        );
}

typedef struct _SET_PROCESS_MODULE_LOAD_COUNT_CONTEXT
{
    NTSTATUS Status;
    PVOID BaseAddress;
    USHORT LoadCount;
} SET_PROCESS_MODULE_LOAD_COUNT_CONTEXT, *PSET_PROCESS_MODULE_LOAD_COUNT_CONTEXT;

BOOLEAN NTAPI PhpSetProcessModuleLoadCountCallback(
    __in HANDLE ProcessHandle,
    __in PLDR_DATA_TABLE_ENTRY Entry,
    __in PVOID AddressOfEntry,
    __in PVOID Context1,
    __in PVOID Context2
    )
{
    PSET_PROCESS_MODULE_LOAD_COUNT_CONTEXT context = Context1;

    if (Entry->DllBase == context->BaseAddress)
    {
        Entry->LoadCount = context->LoadCount;

        context->Status = PhWriteVirtualMemory(
            ProcessHandle,
            AddressOfEntry,
            Entry,
            LDR_DATA_TABLE_ENTRY_SIZE,
            NULL
            );

        return FALSE;
    }

    return TRUE;
}

/**
 * Sets the load count of a process module.
 *
 * \param ProcessHandle A handle to a process. The 
 * handle must have PROCESS_QUERY_LIMITED_INFORMATION, 
 * PROCESS_VM_READ and PROCESS_VM_WRITE access.
 * \param BaseAddress The base address of a module.
 * \param LoadCount The new load count of the module.
 *
 * \retval STATUS_DLL_NOT_FOUND The module was not found.
 */
NTSTATUS PhSetProcessModuleLoadCount(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in USHORT LoadCount
    )
{
    NTSTATUS status;
    SET_PROCESS_MODULE_LOAD_COUNT_CONTEXT context;

    context.Status = STATUS_DLL_NOT_FOUND;
    context.BaseAddress = BaseAddress;
    context.LoadCount = LoadCount;

    status = PhpEnumProcessModules(
        ProcessHandle,
        PhpSetProcessModuleLoadCountCallback,
        &context,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    return context.Status;
}

/**
 * Enumerates the modules loaded by the kernel.
 *
 * \param Modules A variable which receives a pointer 
 * to a structure containing information about 
 * the kernel modules. You must free the structure 
 * using PhFree() when you no longer need it.
 */
NTSTATUS PhEnumKernelModules(
    __out PRTL_PROCESS_MODULES *Modules
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 2048;

    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemModuleInformation,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemModuleInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
    }

    if (!NT_SUCCESS(status))
        return status;

    *Modules = buffer;

    return status;
}

/**
 * Gets the file name of the kernel image.
 *
 * \return A pointer to a string containing the 
 * kernel image file name. You must free the string 
 * using PhDereferenceObject() when you no longer 
 * need it.
 */
PPH_STRING PhGetKernelFileName()
{
    PRTL_PROCESS_MODULES modules;
    PPH_STRING fileName = NULL;

    if (!NT_SUCCESS(PhEnumKernelModules(&modules)))
        return NULL;

    if (modules->NumberOfModules >= 1)
    {
        fileName = PhCreateStringFromAnsi(modules->Modules[0].FullPathName);
    }

    PhFree(modules);

    return fileName;
}

/**
 * Enumerates the running processes.
 *
 * \param Processes A variable which receives a 
 * pointer to a buffer containing process 
 * information. You must free the buffer using 
 * PhFree() when you no longer need it.
 *
 * \remarks You can use the \ref PH_FIRST_PROCESS 
 * and \ref PH_NEXT_PROCESS macros to process the 
 * information contained in the buffer.
 */
NTSTATUS PhEnumProcesses(
    __out PPVOID Processes
    )
{
    static ULONG initialBufferSize = 0x4000;
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQuerySystemInformation(
            SystemProcessInformation,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x20000) initialBufferSize = bufferSize;
    *Processes = buffer;

    return status;
}

/**
 * Finds the process information structure for a 
 * specific process.
 *
 * \param Processes A pointer to a buffer returned 
 * by PhEnumProcesses().
 * \param ProcessId The ID of the process.
 *
 * \return A pointer to the process information 
 * structure for the specified process, or NULL if 
 * the structure could not be found.
 */
PSYSTEM_PROCESS_INFORMATION PhFindProcessInformation(
    __in PVOID Processes,
    __in HANDLE ProcessId
    )
{
    PSYSTEM_PROCESS_INFORMATION process;

    process = PH_FIRST_PROCESS(Processes);

    do
    {
        if (process->UniqueProcessId == ProcessId)
            return process;
    } while (process = PH_NEXT_PROCESS(process));

    return NULL;
}

/**
 * Enumerates all open handles.
 *
 * \param Handles A variable which receives a pointer 
 * to a structure containing information about all 
 * opened handles. You must free the structure using 
 * PhFree() when you no longer need it.
 *
 * \retval STATUS_INSUFFICIENT_RESOURCES The 
 * handle information returned by the kernel is too 
 * large.
 */
NTSTATUS PhEnumHandles(
    __out PSYSTEM_HANDLE_INFORMATION *Handles
    )
{
    static ULONG initialBufferSize = 0x4000;
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    while ((status = NtQuerySystemInformation(
        SystemHandleInformation,
        buffer,
        bufferSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something 
        // very large.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x100000) initialBufferSize = bufferSize;
    *Handles = (PSYSTEM_HANDLE_INFORMATION)buffer;

    return status;
}

/**
 * Enumerates all open handles.
 *
 * \param Handles A variable which receives a pointer 
 * to a structure containing information about all 
 * opened handles. You must free the structure using 
 * PhFree() when you no longer need it.
 *
 * \retval STATUS_INSUFFICIENT_RESOURCES The 
 * handle information returned by the kernel is too 
 * large.
 *
 * \remarks This function is only available starting 
 * with Windows XP.
 */
NTSTATUS PhEnumHandlesEx(
    __out PSYSTEM_HANDLE_INFORMATION_EX *Handles
    )
{
    static ULONG initialBufferSize = 0x10000;
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    while ((status = NtQuerySystemInformation(
        SystemExtendedHandleInformation,
        buffer,
        bufferSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something 
        // very large.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x200000) initialBufferSize = bufferSize;
    *Handles = (PSYSTEM_HANDLE_INFORMATION_EX)buffer;

    return status;
}

NTSTATUS PhEnumPagefiles(
    __out PPVOID Pagefiles
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x200;

    buffer = PhAllocate(bufferSize);

    while ((status = NtQuerySystemInformation(
        SystemPageFileInformation,
        buffer,
        bufferSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something 
        // very large.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Pagefiles = buffer;

    return status;
}

/**
 * Gets the file name of a process' image.
 *
 * \param ProcessId The ID of the process.
 * \param FileName A variable which receives a pointer to a 
 * string containing the file name. You must free the string 
 * using PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function only works on Windows Vista and 
 * above. There does not appear to be any access checking 
 * performed by the kernel for this.
 */
NTSTATUS PhGetProcessImageFileNameByProcessId(
    __in HANDLE ProcessId,
    __out PPH_STRING *FileName
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;
    SYSTEM_PROCESS_IMAGE_NAME_INFORMATION imageNameInfo;

    buffer = PhAllocate(bufferSize);

    imageNameInfo.ProcessId = ProcessId;
    imageNameInfo.ImageName.Length = 0;
    imageNameInfo.ImageName.MaximumLength = (USHORT)bufferSize;
    imageNameInfo.ImageName.Buffer = buffer;

    // This info class allows you to get the file name of any process, 
    // with no security checks!
    // The only issue is that it's completely undocumented (not in *any* 
    // public header files).
    status = NtQuerySystemInformation(
        SystemProcessImageNameInformation,
        &imageNameInfo,
        sizeof(SYSTEM_PROCESS_IMAGE_NAME_INFORMATION),
        NULL
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        // Required length is stored in MaximumLength.

        PhFree(buffer);
        buffer = PhAllocate(imageNameInfo.ImageName.MaximumLength);
        imageNameInfo.ImageName.Buffer = buffer;

        status = NtQuerySystemInformation(
            SystemProcessImageNameInformation,
            &imageNameInfo,
            sizeof(SYSTEM_PROCESS_IMAGE_NAME_INFORMATION),
            NULL
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *FileName = PhCreateStringEx(imageNameInfo.ImageName.Buffer, imageNameInfo.ImageName.Length);
    PhFree(buffer);

    return status;
}

typedef struct _GET_PROCESS_IS_DOT_NET_CONTEXT
{
    HANDLE ProcessId;
    PPH_STRING SectionName;
    PPH_STRING SectionNameV4;
    BOOLEAN Found;
} GET_PROCESS_IS_DOT_NET_CONTEXT, *PGET_PROCESS_IS_DOT_NET_CONTEXT;

BOOLEAN NTAPI PhpGetProcessIsDotNetCallback(
    __in PPH_STRING Name,
    __in PPH_STRING TypeName,
    __in PVOID Context
    )
{
    PGET_PROCESS_IS_DOT_NET_CONTEXT context = Context;

    if (PhStringEquals2(TypeName, L"Section", TRUE) &&
        (PhStringEquals(Name, context->SectionName, TRUE) ||
        PhStringEquals(Name, context->SectionNameV4, TRUE))
        )
    {
        context->Found = TRUE;
        return FALSE;
    }

    return TRUE;
}

NTSTATUS PhGetProcessIsDotNet(
    __in HANDLE ProcessId,
    __out PBOOLEAN IsDotNet
    )
{
    // All .NET processes have a handle open to a section named 
    // \BaseNamedObjects\Cor_Private_IPCBlock(_v4)_<ProcessId>.
    // This is the same object used by the ICorPublish::GetProcess 
    // function. Instead of calling that function, we simply check 
    // for the existence of that section object. This means:
    // * Better performance.
    // * No need for admin rights to get .NET status of processes 
    //   owned by other users.

    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING directoryName;
    HANDLE directoryHandle;
    GET_PROCESS_IS_DOT_NET_CONTEXT context;

    RtlInitUnicodeString(&directoryName, L"\\BaseNamedObjects");
    InitializeObjectAttributes(
        &oa,
        &directoryName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    status = NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &oa
        );

    if (!NT_SUCCESS(status))
        return status;

    context.ProcessId = ProcessId;
    context.SectionName = PhFormatString(L"Cor_Private_IPCBlock_%u", (ULONG)ProcessId);
    context.SectionNameV4 = PhFormatString(L"Cor_Private_IPCBlock_v4_%u", (ULONG)ProcessId);
    context.Found = FALSE;

    PhEnumDirectoryObjects(
        directoryHandle,
        PhpGetProcessIsDotNetCallback,
        &context
        );

    PhDereferenceObject(context.SectionName);
    PhDereferenceObject(context.SectionNameV4);

    NtClose(directoryHandle);

    *IsDotNet = context.Found;

    return status;
}

/**
 * Enumerates the objects in a directory object.
 *
 * \param DirectoryHandle A handle to a directory. The 
 * handle must have DIRECTORY_QUERY access.
 * \param Callback A callback function which is 
 * executed for each object.
 * \param Context A user-defined value to pass to the 
 * callback function.
 */
NTSTATUS PhEnumDirectoryObjects(
    __in HANDLE DirectoryHandle,
    __in PPH_ENUM_DIRECTORY_OBJECTS Callback,
    __in PVOID Context
    )
{
    NTSTATUS status;
    ULONG context = 0;
    BOOLEAN firstTime = TRUE;
    ULONG bufferSize;
    POBJECT_DIRECTORY_INFORMATION buffer;
    ULONG i;
    BOOLEAN cont;

    bufferSize = 0x200;
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        // Get a batch of entries.

        while ((status = NtQueryDirectoryObject(
            DirectoryHandle,
            buffer,
            bufferSize,
            FALSE,
            firstTime,
            &context,
            NULL
            )) == STATUS_MORE_ENTRIES)
        {
            // Check if we have at least one entry. If not, 
            // we'll double the buffer size and try again.
            if (buffer[0].Name.Buffer)
                break;

            // Make sure we don't use too much memory.
            if (bufferSize > PH_LARGE_BUFFER_SIZE)
            {
                PhFree(buffer);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            PhFree(buffer);
            bufferSize *= 2;
            buffer = PhAllocate(bufferSize);
        }

        if (!NT_SUCCESS(status))
        {
            PhFree(buffer);
            return status;
        }

        // Read the batch and execute the callback function 
        // for each object.

        i = 0;
        cont = TRUE;

        while (TRUE)
        {
            POBJECT_DIRECTORY_INFORMATION info;
            PPH_STRING name;
            PPH_STRING typeName;

            info = &buffer[i];

            if (!info->Name.Buffer)
                break;

            name = PhCreateStringEx(info->Name.Buffer, info->Name.Length);
            typeName = PhCreateStringEx(info->TypeName.Buffer, info->TypeName.Length);  

            cont = Callback(name, typeName, Context);

            PhDereferenceObject(name);
            PhDereferenceObject(typeName);

            if (!cont)
                break;

            i++;
        }

        if (!cont)
            break;

        if (status != STATUS_MORE_ENTRIES)
            break;

        firstTime = FALSE;
    }

    PhFree(buffer);

    return STATUS_SUCCESS;
}

NTSTATUS PhEnumDirectoryFile(
    __in HANDLE FileHandle,
    __in_opt PPH_STRINGREF SearchPattern,
    __in PPH_ENUM_DIRECTORY_FILE Callback,
    __in PVOID Context
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    BOOLEAN firstTime = TRUE;
    PVOID buffer;
    ULONG bufferSize = 0x400;
    ULONG i;
    BOOLEAN cont;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        // Query the directory, doubling the buffer each time NtQueryDirectoryFile 
        // fails.
        while (TRUE)
        {
            status = NtQueryDirectoryFile(
                FileHandle,
                NULL,
                NULL,
                NULL,
                &isb,
                buffer,
                bufferSize,
                FileDirectoryInformation,
                FALSE,
                SearchPattern ? &SearchPattern->us : NULL,
                firstTime
                );

            // Our ISB is on the stack, so we have to wait for the operation to 
            // complete before continuing.
            if (status == STATUS_PENDING)
            {
                NtWaitForSingleObject(FileHandle, FALSE, NULL);
                status = isb.Status;
            }

            if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_INFO_LENGTH_MISMATCH)
            {
                PhFree(buffer);
                bufferSize *= 2;
                buffer = PhAllocate(bufferSize);
            }
            else
            {
                break;
            }
        }

        // If we don't have any entries to read, exit.
        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        // Read the batch and execute the callback function 
        // for each file.

        i = 0;
        cont = TRUE;

        while (TRUE)
        {
            PFILE_DIRECTORY_INFORMATION information;

            information = (PFILE_DIRECTORY_INFORMATION)(PTR_ADD_OFFSET(buffer, i));

            if (!Callback(
                information,
                Context
                ))
            {
                cont = FALSE;
                break;
            }

            if (information->NextEntryOffset != 0)
                i += information->NextEntryOffset;
            else
                break;
        }

        if (!cont)
            break;

        firstTime = FALSE;
    }

    PhFree(buffer);

    return status;
}

NTSTATUS PhEnumFileStreams(
    __in HANDLE FileHandle,
    __out PPVOID Streams
    )
{
    return PhpQueryFileVariableSize(
        FileHandle,
        FileStreamInformation,
        Streams
        );
}

/**
 * Initializes the device prefixes module.
 */
VOID PhInitializeDevicePrefixes()
{
    ULONG i;

    for (i = 0; i < 26; i++)
    {
        PhDevicePrefixes[i].Length = 0;
        PhDevicePrefixes[i].us.MaximumLength = PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR);
        PhDevicePrefixes[i].Buffer = PhAllocate(PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR));
    }
}

VOID PhRefreshMupDevicePrefixes()
{
    HKEY orderKeyHandle;
    PPH_STRING providerOrder = NULL;
    ULONG i;
    ULONG indexOfComma;

    // The provider names are stored in the ProviderOrder value in this key:
    // HKLM\System\CurrentControlSet\Control\NetworkProvider\Order
    // Each name can then be looked up, its device name in the DeviceName value in:
    // HKLM\System\CurrentControlSet\Services\<ProviderName>\NetworkProvider

    if (RegOpenKey(
        HKEY_LOCAL_MACHINE,
        L"System\\CurrentControlSet\\Control\\NetworkProvider\\Order",
        &orderKeyHandle
        ) == ERROR_SUCCESS)
    {
        providerOrder = PhQueryRegistryString(orderKeyHandle, L"ProviderOrder");
        RegCloseKey(orderKeyHandle);
    }

    if (!providerOrder)
        return;

    PhAcquireQueuedLockExclusiveFast(&PhDeviceMupPrefixesLock);

    for (i = 0; i < PhDeviceMupPrefixesCount; i++)
    {
        PhDereferenceObject(PhDeviceMupPrefixes[i]);
        PhDeviceMupPrefixes[i] = 0;
    }

    PhDeviceMupPrefixesCount = 0;

    while (i < providerOrder->Length / sizeof(WCHAR))
    {
        PPH_STRING serviceName;
        PPH_STRING serviceKeyName;
        HKEY networkProviderKeyHandle;
        PPH_STRING deviceName;

        if (PhDeviceMupPrefixesCount == PH_DEVICE_MUP_PREFIX_MAX_COUNT)
            break;

        indexOfComma = PhStringIndexOfChar(providerOrder, i, ',');

        if (indexOfComma == -1) // last provider name
            indexOfComma = providerOrder->Length / sizeof(WCHAR);

        serviceName = PhSubstring(
            providerOrder,
            i,
            indexOfComma - i
            );
        serviceKeyName = PhConcatStrings(
            3,
            L"System\\CurrentControlSet\\Services\\",
            serviceName->Buffer,
            L"\\NetworkProvider"
            );

        if (RegOpenKey(
            HKEY_LOCAL_MACHINE,
            serviceKeyName->Buffer,
            &networkProviderKeyHandle
            ) == ERROR_SUCCESS)
        {
            if (deviceName = PhQueryRegistryString(networkProviderKeyHandle, L"DeviceName"))
            {
                PhDeviceMupPrefixes[PhDeviceMupPrefixesCount] = deviceName;
                PhDeviceMupPrefixesCount++;
            }

            RegCloseKey(networkProviderKeyHandle);
        }

        PhDereferenceObject(serviceKeyName);
        PhDereferenceObject(serviceName);

        i = indexOfComma + 1;
    }

    PhReleaseQueuedLockExclusiveFast(&PhDeviceMupPrefixesLock);

    PhDereferenceObject(providerOrder);
}

/**
 * Refreshes the DOS device names array.
 */
VOID PhRefreshDosDevicePrefixes()
{
    WCHAR deviceNameBuffer[7] = L"\\??\\ :";
    ULONG i;

    for (i = 0; i < 26; i++)
    {
        HANDLE linkHandle;
        OBJECT_ATTRIBUTES oa;
        UNICODE_STRING deviceName;

        deviceNameBuffer[4] = (WCHAR)('A' + i);
        deviceName.Buffer = deviceNameBuffer;
        deviceName.Length = 6 * sizeof(WCHAR);

        InitializeObjectAttributes(
            &oa,
            &deviceName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        if (NT_SUCCESS(NtOpenSymbolicLinkObject(
            &linkHandle,
            SYMBOLIC_LINK_QUERY,
            &oa
            )))
        {
            PhAcquireQueuedLockExclusiveFast(&PhDevicePrefixesLock);

            if (!NT_SUCCESS(NtQuerySymbolicLinkObject(
                linkHandle,
                &PhDevicePrefixes[i].us,
                NULL
                )))
            {
                PhDevicePrefixes[i].Length = 0;
            }

            PhReleaseQueuedLockExclusiveFast(&PhDevicePrefixesLock);

            NtClose(linkHandle);
        }
        else
        {
            PhDevicePrefixes[i].Length = 0;
        }
    }
}

/**
 * Resolves a NT path into a DOS path.
 *
 * \param Name A string containing the path to resolve.
 *
 * \return A pointer to a string containing the DOS 
 * path. You must free the string using PhDereferenceObject() 
 * when you no longer need it.
 */
PPH_STRING PhResolveDevicePrefix(
    __in PPH_STRING Name
    )
{
    ULONG i;
    PPH_STRING newName = NULL;

    if (PhBeginInitOnce(&PhDevicePrefixesInitOnce))
    {
        PhInitializeDevicePrefixes();
        PhRefreshDosDevicePrefixes();
        PhRefreshMupDevicePrefixes();

        PhEndInitOnce(&PhDevicePrefixesInitOnce);
    }

    // Go through the DOS devices and try to find a matching prefix.
    for (i = 0; i < 26; i++)
    {
        BOOLEAN isPrefix = FALSE;
        ULONG prefixLength;

        PhAcquireQueuedLockSharedFast(&PhDevicePrefixesLock);

        prefixLength = PhDevicePrefixes[i].Length;

        if (prefixLength != 0)
            isPrefix = PhStringRefStartsWith(&Name->sr, &PhDevicePrefixes[i], TRUE);

        PhReleaseQueuedLockSharedFast(&PhDevicePrefixesLock);

        if (isPrefix)
        {
            // <letter>:path
            newName = PhCreateStringEx(NULL, 2 * sizeof(WCHAR) + Name->Length - prefixLength);
            newName->Buffer[0] = (WCHAR)('A' + i);
            newName->Buffer[1] = ':';
            memcpy(
                &newName->Buffer[2],
                &Name->Buffer[prefixLength / sizeof(WCHAR)],
                Name->Length - prefixLength
                );

            break;
        }
    }

    if (i == 26)
    {
        // "\Device\Mup" is the UNC provider.
        if (PhStringStartsWith2(Name, L"\\Device\\Mup", TRUE))
        {
#define DEVICE_MUP_LENGTH (11 * sizeof(WCHAR))

            // \path
            newName = PhCreateStringEx(NULL, 1 * sizeof(WCHAR) + Name->Length - DEVICE_MUP_LENGTH);
            newName->Buffer[0] = '\\';
            memcpy(
                &newName->Buffer[1],
                &Name->Buffer[DEVICE_MUP_LENGTH / sizeof(WCHAR)],
                Name->Length - DEVICE_MUP_LENGTH
                );

            return newName;
        }

        // Resolve network providers.

        PhAcquireQueuedLockSharedFast(&PhDeviceMupPrefixesLock);

        for (i = 0; i < PhDeviceMupPrefixesCount; i++)
        {
            BOOLEAN isPrefix = FALSE;
            ULONG prefixLength;

            prefixLength = PhDeviceMupPrefixes[i]->Length;

            if (prefixLength != 0)
                isPrefix = PhStringStartsWith(Name, PhDeviceMupPrefixes[i], TRUE);

            if (isPrefix)
            {
                // \path
                newName = PhCreateStringEx(NULL, 1 * sizeof(WCHAR) + Name->Length - prefixLength);
                newName->Buffer[0] = '\\';
                memcpy(
                    &newName->Buffer[1],
                    &Name->Buffer[prefixLength / sizeof(WCHAR)],
                    Name->Length - prefixLength
                    );

                break;
            }
        }

        PhReleaseQueuedLockSharedFast(&PhDeviceMupPrefixesLock);
    }

    return newName;
}

/**
 * Converts a file name into DOS format.
 *
 * \param FileName A string containing a file name.
 *
 * \return A pointer to a string containing the DOS 
 * file name. You must free the string using 
 * PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function may convert NT object 
 * name paths to invalid ones. If the path to be 
 * converted is not necessarily a file name, use 
 * PhResolveDevicePrefix().
 */
PPH_STRING PhGetFileName(
    __in PPH_STRING FileName
    )
{
    PPH_STRING newFileName;

    newFileName = FileName;

    // "\??\" refers to \GLOBAL??. Just remove it.
    if (PhStringStartsWith2(FileName, L"\\??\\", FALSE))
    {
        newFileName = PhCreateStringEx(NULL, FileName->Length - 8);
        memcpy(newFileName->Buffer, &FileName->Buffer[4], FileName->Length - 8);
    }
    // "\SystemRoot" means "C:\Windows".
    else if (PhStringStartsWith2(FileName, L"\\SystemRoot", TRUE))
    {
        PPH_STRING systemDirectory = PhGetSystemDirectory();

        if (systemDirectory)
        {
            ULONG indexOfLastBackslash = PhStringLastIndexOfChar(systemDirectory, 0, '\\');

            newFileName = PhCreateStringEx(NULL, indexOfLastBackslash * 2 + FileName->Length - 22);
            memcpy(newFileName->Buffer, systemDirectory->Buffer, indexOfLastBackslash * 2);
            memcpy(&newFileName->Buffer[indexOfLastBackslash], &FileName->Buffer[11], FileName->Length - 22);

            PhDereferenceObject(systemDirectory);
        }
    }
    else if (FileName->Length != 0 && FileName->Buffer[0] == '\\')
    {
        PPH_STRING resolvedName;

        resolvedName = PhResolveDevicePrefix(FileName);

        if (resolvedName)
        {
            newFileName = resolvedName;
        }
        else
        {
            // We didn't find a match.
            // If the file name starts with a backslash, prepend the system drive.
            if (PhStringStartsWith2(newFileName, L"\\Windows", TRUE))
            {
                PPH_STRING systemDirectory = PhGetSystemDirectory();

                newFileName = PhCreateStringEx(NULL, FileName->Length + 4);
                newFileName->Buffer[0] = systemDirectory->Buffer[0];
                newFileName->Buffer[1] = ':';
                memcpy(&newFileName->Buffer[2], FileName->Buffer, FileName->Length);

                PhDereferenceObject(systemDirectory);
            }
            else
            {
                // Just return the supplied file name. Note that we need 
                // to add a reference.
                PhReferenceObject(newFileName);
            }
        }
    }

    return newFileName;
}

typedef struct _ENUM_GENERIC_PROCESS_MODULES_CONTEXT
{
    PPH_ENUM_GENERIC_MODULES_CALLBACK Callback;
    PVOID Context;
    PPH_LIST BaseAddressList;
} ENUM_GENERIC_PROCESS_MODULES_CONTEXT, *PENUM_GENERIC_PROCESS_MODULES_CONTEXT;

static BOOLEAN EnumGenericProcessModulesCallback(
    __in PLDR_DATA_TABLE_ENTRY Module,
    __in PVOID Context
    )
{
    PENUM_GENERIC_PROCESS_MODULES_CONTEXT context;
    PH_MODULE_INFO moduleInfo;
    PPH_STRING fileName;
    BOOLEAN cont;

    context = (PENUM_GENERIC_PROCESS_MODULES_CONTEXT)Context;

    // Check if we have a duplicate base address.
    if (PhIndexOfListItem(context->BaseAddressList, Module->DllBase) != -1)
    {
        return TRUE;
    }
    else
    {
        PhAddListItem(context->BaseAddressList, Module->DllBase);
    }

    fileName = PhCreateStringEx(
        Module->FullDllName.Buffer,
        Module->FullDllName.Length
        );

    moduleInfo.BaseAddress = Module->DllBase;
    moduleInfo.Size = Module->SizeOfImage;
    moduleInfo.EntryPoint = Module->EntryPoint;
    moduleInfo.Flags = Module->Flags;
    moduleInfo.Name = PhCreateStringEx(
        Module->BaseDllName.Buffer,
        Module->BaseDllName.Length
        );
    moduleInfo.FileName = PhGetFileName(fileName);

    PhDereferenceObject(fileName);

    cont = context->Callback(&moduleInfo, context->Context);

    PhDereferenceObject(moduleInfo.Name);
    PhDereferenceObject(moduleInfo.FileName);

    return cont;
}

VOID PhpRtlModulesToGenericModules(
    __in PRTL_PROCESS_MODULES Modules,
    __in PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    __in PVOID Context,
    __in PPH_LIST BaseAddressList
    )
{
    PRTL_PROCESS_MODULE_INFORMATION module;
    ULONG i;
    PH_MODULE_INFO moduleInfo;
    BOOLEAN cont;

    for (i = 0; i < Modules->NumberOfModules; i++)
    {
        PPH_STRING fileName;

        module = &Modules->Modules[i];

        // Check if we have a duplicate base address.
        if (PhIndexOfListItem(BaseAddressList, module->ImageBase) != -1)
        {
            continue;
        }
        else
        {
            PhAddListItem(BaseAddressList, module->ImageBase);
        }

        fileName = PhCreateStringFromAnsi(module->FullPathName);

        moduleInfo.BaseAddress = module->ImageBase;
        moduleInfo.Size = module->ImageSize;
        moduleInfo.EntryPoint = NULL;
        moduleInfo.Flags = module->Flags;
        moduleInfo.Name = PhCreateStringFromAnsi(&module->FullPathName[module->OffsetToFileName]);
        moduleInfo.FileName = PhGetFileName(fileName); // convert to DOS file name

        PhDereferenceObject(fileName);

        cont = Callback(&moduleInfo, Context);

        PhDereferenceObject(moduleInfo.Name);
        PhDereferenceObject(moduleInfo.FileName);

        if (!cont)
            break;
    }
}

VOID PhpEnumGenericMappedFiles(
    __in HANDLE ProcessHandle,
    __in PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    __in PVOID Context,
    __in PPH_LIST BaseAddressList
    )
{
    PVOID baseAddress;
    MEMORY_BASIC_INFORMATION basicInfo;

    baseAddress = (PVOID)0;

    while (NT_SUCCESS(NtQueryVirtualMemory(
        ProcessHandle,
        baseAddress,
        MemoryBasicInformation,
        &basicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        if (basicInfo.Type == MEM_MAPPED)
        {
            PPH_STRING fileName;
            PPH_STRING newFileName;
            PH_MODULE_INFO moduleInfo;
            BOOLEAN cont;

            // Check if we have a duplicate base address.
            if (PhIndexOfListItem(BaseAddressList, baseAddress) != -1)
            {
                goto ContinueLoop;
            }
            else
            {
                PhAddListItem(BaseAddressList, baseAddress);
            }

            if (!NT_SUCCESS(PhGetProcessMappedFileName(
                ProcessHandle,
                baseAddress,
                &fileName
                )))
                goto ContinueLoop;

            // Get the DOS file name and then get the base name.

            newFileName = PhGetFileName(fileName);
            PhDereferenceObject(fileName);

            moduleInfo.BaseAddress = baseAddress;
            moduleInfo.Size = (ULONG)basicInfo.RegionSize;
            moduleInfo.EntryPoint = NULL;
            moduleInfo.Flags = 0;
            moduleInfo.FileName = newFileName;
            moduleInfo.Name = PhGetBaseName(newFileName);

            cont = Callback(&moduleInfo, Context);

            PhDereferenceObject(moduleInfo.FileName);
            PhDereferenceObject(moduleInfo.Name);

            if (!cont)
                break;
        }

ContinueLoop:
        baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);
    }
}

/**
 * Enumerates the modules loaded by a process.
 *
 * \param ProcessId The ID of a process. If 
 * \ref SYSTEM_PROCESS_ID is specified the function 
 * enumerates the kernel modules.
 * \param ProcessHandle A handle to the process.
 * \param Flags Flags controlling the information 
 * to retrieve.
 * \li \c PH_ENUM_GENERIC_MAPPED_FILES Enumerate mapped 
 * files.
 * \param Callback A callback function which is executed 
 * for each module.
 * \param Context A user-defined value to pass 
 * to the callback function.
 */
NTSTATUS PhEnumGenericModules(
    __in HANDLE ProcessId,
    __in_opt HANDLE ProcessHandle,
    __in ULONG Flags,
    __in PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    __in PVOID Context
    )
{
    NTSTATUS status;
    PPH_LIST baseAddressList;

    baseAddressList = PhCreateList(20);

    if (ProcessId == SYSTEM_PROCESS_ID)
    {
        // Kernel modules

        PRTL_PROCESS_MODULES modules;

        if (!NT_SUCCESS(status = PhEnumKernelModules(&modules)))
        {
            goto CleanupExit;
        }

        PhpRtlModulesToGenericModules(
            modules,
            Callback,
            Context,
            baseAddressList
            );

        PhFree(modules);
    }
    else
    {
        // 32-bit process modules

        BOOLEAN opened = FALSE;
        BOOLEAN isWow64 = FALSE;
        ENUM_GENERIC_PROCESS_MODULES_CONTEXT context;

        if (!ProcessHandle)
        {
            if (!NT_SUCCESS(status = PhOpenProcess(
                &ProcessHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                ProcessId
                )))
            {
                if (!NT_SUCCESS(status = PhOpenProcess(
                    &ProcessHandle,
                    ProcessQueryAccess | PROCESS_VM_READ,
                    ProcessId
                    )))
                {
                    goto CleanupExit;
                }
            }

            opened = TRUE;
        }

        context.Callback = Callback;
        context.Context = Context;
        context.BaseAddressList = baseAddressList;

        status = PhEnumProcessModules(
            ProcessHandle,
            EnumGenericProcessModulesCallback,
            &context
            );

        // Mapped files

        if (Flags & PH_ENUM_GENERIC_MAPPED_FILES)
        {
            PhpEnumGenericMappedFiles(
                ProcessHandle,
                Callback,
                Context,
                baseAddressList
                );
        }

#ifdef _M_X64
        // And just before we close the process handle, find out 
        // if the process is running under WOW64.
        PhGetProcessIsWow64(ProcessHandle, &isWow64);
#endif

        if (opened)
            NtClose(ProcessHandle);

#ifdef _M_X64
        // 64-bit process modules
        if (isWow64)
        {
            PRTL_DEBUG_INFORMATION debugBuffer;

            debugBuffer = RtlCreateQueryDebugBuffer(0, FALSE);

            if (debugBuffer)
            {
                if (NT_SUCCESS(RtlQueryProcessDebugInformation(
                    ProcessId,
                    RTL_QUERY_PROCESS_MODULES32 | RTL_QUERY_PROCESS_NONINVASIVE,
                    debugBuffer
                    )))
                {
                    PhpRtlModulesToGenericModules(
                        debugBuffer->Modules,
                        Callback,
                        Context,
                        baseAddressList
                        );
                }

                RtlDestroyQueryDebugBuffer(debugBuffer);
            }
        }
#endif
    }

CleanupExit:
    PhDereferenceObject(baseAddressList);

    return status;
}
