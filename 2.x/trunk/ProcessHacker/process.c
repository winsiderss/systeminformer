/*
 * Process Hacker - 
 *   process support functions
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
#include <symprvp.h>
#include <sddl.h>

PWSTR PhDosDeviceNames[26];
PH_FAST_LOCK PhDosDeviceNamesLock;

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

NTSTATUS PhSuspendProcess(
    __in HANDLE ProcessHandle
    )
{
    if (PhKphHandle && WindowsVersion >= WINDOWS_VISTA)
    {
        return KphSuspendProcess(PhKphHandle, ProcessHandle);
    }
    else
    {
        return NtSuspendProcess(ProcessHandle);
    }
}

NTSTATUS PhResumeProcess(
    __in HANDLE ProcessHandle
    )
{
    if (PhKphHandle && WindowsVersion >= WINDOWS_VISTA)
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
    ULONG returnLength;

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
    PVOID buffer;
    PUNICODE_STRING fileName;

    status = PhpQueryProcessVariableSize(
        ProcessHandle,
        ProcessImageFileName,
        &buffer
        );

    if (!NT_SUCCESS(status))
        return status;

    fileName = (PUNICODE_STRING)buffer;
    *FileName = PhCreateStringEx(fileName->Buffer, fileName->Length);
    PhFree(buffer);

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
    UOCTA cycleTime;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessCycleTime,
        &cycleTime,
        sizeof(UOCTA),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    *CycleTime = cycleTime.LowPart;

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

        if (!VirtualQueryEx(
            ProcessHandle,
            environment,
            &mbi,
            sizeof(MEMORY_BASIC_INFORMATION)
            ))
            return STATUS_UNSUCCESSFUL;

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
    UOCTA cycleTime;

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadCycleTime,
        &cycleTime,
        sizeof(UOCTA),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    *CycleTime = cycleTime.LowPart;

    return status;
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
        if (PhKphHandle)
        {
            if (!NT_SUCCESS(status = KphOpenThreadProcess(
                PhKphHandle,
                &ProcessHandle,
                ThreadHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ
                )))
                return status;
        }
        else
        {
            THREAD_BASIC_INFORMATION basicInfo;

            if (!NT_SUCCESS(status = PhGetThreadBasicInformation(
                ThreadHandle,
                &basicInfo
                )))
                return status;

            if (!NT_SUCCESS(status = PhOpenProcess(
                &ProcessHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                basicInfo.ClientId.UniqueProcess
                )))
                return status;
        }

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
            if (!stackFrame.AddrPC.Offset)
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
            if (!stackFrame.AddrPC.Offset)
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
        CloseHandle(ProcessHandle);

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
    ULONG returnLength;

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
    TOKEN_PRIVILEGES privileges = { 0 };

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

    if (!AdjustTokenPrivileges(
        TokenHandle,
        FALSE,
        &privileges,
        0,
        NULL,
        NULL
        ))
        return FALSE;

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
        return FALSE;

    return TRUE;
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

/**
 * Gets the name of a privilege from its LUID.
 *
 * \param PrivilegeValue The LUID of a privilege.
 * \param PrivilegeName A variable which receives 
 * a pointer to a string containing the privilege 
 * name. You must free the string using 
 * PhDereferenceObject() when you no longer need it.
 */
BOOLEAN PhLookupPrivilegeName(
    __in PLUID PrivilegeValue,
    __out PPH_STRING *PrivilegeName
    )
{
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = 0x10;
    buffer = PhAllocate(bufferSize * 2);

    if (!LookupPrivilegeName(NULL, PrivilegeValue, buffer, &bufferSize))
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize * 2);

        if (!LookupPrivilegeName(NULL, PrivilegeValue, buffer, &bufferSize))
        {
            PhFree(buffer);
            return FALSE;
        }
    }

    *PrivilegeName = PhCreateString(buffer);

    return TRUE;
}

/**
 * Gets the display name of a privilege from its name.
 *
 * \param PrivilegeName The name of a privilege.
 * \param PrivilegeDisplayName A variable which receives 
 * a pointer to a string containing the privilege's 
 * display name. You must free the string using 
 * PhDereferenceObject() when you no longer need it.
 */
BOOLEAN PhLookupPrivilegeDisplayName(
    __in PWSTR PrivilegeName,
    __out PPH_STRING *PrivilegeDisplayName
    )
{
    PVOID buffer;
    ULONG bufferSize;
    ULONG languageId;

    bufferSize = 0x40;
    buffer = PhAllocate(bufferSize * 2);

    if (!LookupPrivilegeDisplayName(NULL, PrivilegeName, buffer, &bufferSize, &languageId))
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize * 2);

        if (!LookupPrivilegeDisplayName(NULL, PrivilegeName, buffer, &bufferSize, &languageId))
        {
            PhFree(buffer);
            return FALSE;
        }
    }

    *PrivilegeDisplayName = PhCreateString(buffer);

    return TRUE;
}

/**
 * Gets the LUID of a privilege from its name.
 *
 * \param PrivilegeName The name of a privilege.
 * \param PrivilegeValue A variable which receives 
 * the LUID of the privilege.
 */
BOOLEAN PhLookupPrivilegeValue(
    __in PWSTR PrivilegeName,
    __out PLUID PrivilegeValue
    )
{
    return !!LookupPrivilegeValue(
        NULL,
        PrivilegeName,
        PrivilegeValue
        );
}

/**
 * Gets information about a SID.
 *
 * \param Sid A SID to query.
 * \param Name A variable which receives a pointer 
 * to a string containing the SID's name. You must 
 * free the string using PhDereferenceObject() when 
 * you no longer need it.
 * \param DomainName A variable which receives a pointer 
 * to a string containing the SID's domain name. You must 
 * free the string using PhDereferenceObject() when 
 * you no longer need it.
 * \param NameUse A variable which receives the 
 * SID's usage.
 */
BOOLEAN PhLookupSid(
    __in PSID Sid,
    __out_opt PPH_STRING *Name,
    __out_opt PPH_STRING *DomainName,
    __out_opt PSID_NAME_USE NameUse
    )
{
    PVOID nameBuffer;
    ULONG nameBufferSize;
    PVOID domainNameBuffer;
    ULONG domainNameBufferSize;
    SID_NAME_USE nameUse;

    nameBufferSize = 0x40;
    nameBuffer = PhAllocate(nameBufferSize * 2);
    domainNameBufferSize = 0x40;
    domainNameBuffer = PhAllocate(domainNameBufferSize * 2);

    if (!LookupAccountSid(
        NULL,
        Sid,
        nameBuffer,
        &nameBufferSize,
        domainNameBuffer,
        &domainNameBufferSize,
        &nameUse
        ))
    {
        PhFree(nameBuffer);
        nameBuffer = PhAllocate(nameBufferSize * 2);
        PhFree(domainNameBuffer);
        domainNameBuffer = PhAllocate(domainNameBufferSize * 2);

        if (!LookupAccountSid(
            NULL,
            Sid,
            nameBuffer,
            &nameBufferSize,
            domainNameBuffer,
            &domainNameBufferSize,
            &nameUse
            ))
        {
            PhFree(nameBuffer);
            PhFree(domainNameBuffer);

            return FALSE;
        }
    }

    if (Name)
        *Name = PhCreateString(nameBuffer);
    if (DomainName)
        *DomainName = PhCreateString(domainNameBuffer);
    if (NameUse)
        *NameUse = nameUse;

    PhFree(nameBuffer);
    PhFree(domainNameBuffer);

    return TRUE;
}

/**
 * Gets the name of a SID.
 *
 * \param Sid A SID to query.
 *
 * \return A pointer to a string containing 
 * the name of the SID in the following 
 * format: domain\\name. You must free the string 
 * using PhDereferenceObject() when you no longer 
 * need it. If an error occurs, the function 
 * returns NULL.
 */
PPH_STRING PhGetSidFullName(
    __in PSID Sid
    )
{
    PPH_STRING fullName;
    PPH_STRING name;
    PPH_STRING domainName;

    if (!PhLookupSid(Sid, &name, &domainName, NULL))
        return NULL;

    if (domainName->Length != 0)
    {
        fullName = PhConcatStrings(3, domainName->Buffer, L"\\", name->Buffer);
    }
    else
    {
        fullName = name;
        PhReferenceObject(name);
    }

    PhDereferenceObject(name);
    PhDereferenceObject(domainName);

    return fullName;
}

/**
 * Gets a SDDL string representation of a SID.
 *
 * \param Sid A SID to query.
 *
 * \return A pointer to a string containing the 
 * SDDL representation of the SID. You must 
 * free the string using PhDereferenceObject() 
 * when you no longer need it. If an error occurs, 
 * the function returns NULL.
 */
PPH_STRING PhConvertSidToStringSid(
    __in PSID Sid
    )
{
    PWSTR stringSid;
    PPH_STRING string;

    if (!ConvertSidToStringSid(Sid, &stringSid))
        return NULL;

    string = PhCreateString(stringSid);
    LocalFree(stringSid);

    return string;
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
        PWSTR baseDllNameBuffer;
        PWSTR fullDllNameBuffer;
        BOOLEAN cont;

        status = PhReadVirtualMemory(
            ProcessHandle,
            CONTAINING_RECORD(currentLink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks),
            &currentEntry,
            sizeof(LDR_DATA_TABLE_ENTRY),
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;

        // Make sure the entry is valid.
        if (currentEntry.DllBase)
        {
            // Read the base DLL name string and add a null terminator.

            baseDllNameBuffer = PhAllocate(currentEntry.BaseDllName.Length + 2);

            if (NT_SUCCESS(PhReadVirtualMemory(
                ProcessHandle,
                currentEntry.BaseDllName.Buffer,
                baseDllNameBuffer,
                currentEntry.BaseDllName.Length,
                NULL
                )))
            {
                baseDllNameBuffer[currentEntry.BaseDllName.Length / 2] = 0;
                currentEntry.BaseDllName.Buffer = baseDllNameBuffer;
            }

            // Read the full DLL name string and add a null terminator.

            fullDllNameBuffer = PhAllocate(currentEntry.FullDllName.Length + 2);

            if (NT_SUCCESS(PhReadVirtualMemory(
                ProcessHandle,
                currentEntry.FullDllName.Buffer,
                fullDllNameBuffer,
                currentEntry.FullDllName.Length,
                NULL
                )))
            {
                fullDllNameBuffer[currentEntry.FullDllName.Length / 2] = 0;
                currentEntry.FullDllName.Buffer = fullDllNameBuffer;
            }

            // Execute the callback.
            cont = Callback(&currentEntry, Context);

            PhFree(baseDllNameBuffer);
            PhFree(fullDllNameBuffer);

            if (!cont)
                break;
        }

        currentLink = currentEntry.InLoadOrderLinks.Flink;
        i++;
    }

    return status;
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
    static ULONG bufferSize = 2048;

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
    NTSTATUS status;
    PVOID buffer;
    static ULONG bufferSize = 2048; // keep the last successful buffer size between calls

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
    NTSTATUS status;
    PVOID buffer;
    static ULONG bufferSize = 0x1000;

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

        // Fail if we're resizing the buffer to over 
        // 16 MB.
        if (bufferSize > 16 * 1024 * 1024)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Handles = (PSYSTEM_HANDLE_INFORMATION)buffer;

    return status;
}

/**
 * Initializes the DOS device names module.
 */
VOID PhInitializeDosDeviceNames()
{
    ULONG i;

    for (i = 0; i < 26; i++)
        PhDosDeviceNames[i] = PhAllocate(64 * sizeof(WCHAR));

    PhInitializeFastLock(&PhDosDeviceNamesLock);
}

/**
 * Refreshes the DOS device names array.
 */
VOID PhRefreshDosDeviceNames()
{
    WCHAR deviceName[3];
    ULONG i;

    deviceName[1] = ':';
    deviceName[2] = 0;

    for (i = 0; i < 26; i++)
    {
        deviceName[0] = (WCHAR)('A' + i);

        PhAcquireFastLockExclusive(&PhDosDeviceNamesLock);

        if (!QueryDosDevice(deviceName, PhDosDeviceNames[i], 64))
            PhDosDeviceNames[i][0] = 0;

        PhReleaseFastLockExclusive(&PhDosDeviceNamesLock);
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

    // Go through the DOS devices and try to find a matching prefix.
    for (i = 0; i < 26; i++)
    {
        PWSTR prefix;
        ULONG prefixLength;
        BOOLEAN isPrefix = FALSE;

        PhAcquireFastLockShared(&PhDosDeviceNamesLock);

        prefix = PhDosDeviceNames[i];
        prefixLength = (ULONG)wcslen(prefix);

        if (prefixLength > 0)
            isPrefix = PhStringStartsWith2(Name, prefix, TRUE);

        PhReleaseFastLockShared(&PhDosDeviceNamesLock);

        if (isPrefix)
        {
            newName = PhCreateStringEx(NULL, 4 + Name->Length - prefixLength * 2);
            newName->Buffer[0] = (WCHAR)('A' + i);
            newName->Buffer[1] = ':';
            memcpy(
                &newName->Buffer[2],
                &Name->Buffer[prefixLength],
                Name->Length - prefixLength * 2
                );

            break;
        }
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
    else
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

    while (VirtualQueryEx(
        ProcessHandle,
        baseAddress,
        &basicInfo,
        sizeof(MEMORY_BASIC_INFORMATION)
        ))
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

        if (opened)
            CloseHandle(ProcessHandle);

#ifdef _M_X64
        // 64-bit process modules
        {
            PRTL_DEBUG_INFORMATION debugBuffer;

            debugBuffer = RtlCreateQueryDebugBuffer(0, TRUE);

            if (debugBuffer)
            {
                if (NT_SUCCESS(RtlQueryProcessDebugInformation(
                    ProcessId,
                    RTL_QUERY_PROCESS_MODULES32,
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
