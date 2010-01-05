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

static PWSTR PhDosDeviceNames[26];

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
            return STATUS_UNSUCCESSFUL;
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

NTSTATUS PhGetTokenElevationType(
    __in HANDLE TokenHandle,
    __out PTOKEN_ELEVATION_TYPE ElevationType
    )
{
    return NtQueryInformationToken(
        TokenHandle,
        TokenElevationType,
        ElevationType,
        sizeof(TOKEN_ELEVATION_TYPE),
        NULL
        );
}

NTSTATUS PhGetTokenIsElevated(
    __in HANDLE TokenHandle,
    __out PBOOLEAN Elevated
    )
{
    NTSTATUS status;
    TOKEN_ELEVATION elevation;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenElevation,
        &elevation,
        sizeof(TOKEN_ELEVATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *Elevated = !!elevation.TokenIsElevated;
    }

    return status;
}

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
        if (!buffer)
            return STATUS_INSUFFICIENT_RESOURCES;

        status = NtQuerySystemInformation(
            SystemProcessInformation,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            PhFree(buffer);
            return status;
        }
    }

    *Processes = buffer;

    return status;
}

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

VOID PhInitializeDosDeviceNames()
{
    ULONG i;

    for (i = 0; i < 26; i++)
        PhDosDeviceNames[i] = PhAllocate(64 * sizeof(WCHAR));
}

VOID PhRefreshDosDeviceNames()
{
    WCHAR deviceName[3];
    ULONG i;

    deviceName[1] = ':';
    deviceName[2] = 0;

    for (i = 0; i < 26; i++)
    {
        deviceName[0] = (WCHAR)('A' + i);

        if (!QueryDosDevice(deviceName, PhDosDeviceNames[i], 64))
            PhDosDeviceNames[i][0] = 0;
    }
}

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
    // Go through the DOS devices and try to find a matching prefix.
    else
    {
        ULONG i;

        for (i = 0; i < 26; i++)
        {
            PWSTR prefix = PhDosDeviceNames[i];
            ULONG prefixLength = (ULONG)wcslen(prefix);

            if (prefixLength > 0)
            {
                if (PhStringStartsWith2(FileName, prefix, TRUE))
                {
                    newFileName = PhCreateStringEx(NULL, 4 + FileName->Length - prefixLength * 2);
                    newFileName->Buffer[0] = (WCHAR)('A' + i);
                    newFileName->Buffer[1] = ':';
                    memcpy(&newFileName->Buffer[2], &FileName->Buffer[prefixLength], FileName->Length - prefixLength * 2);

                    break;
                }
            }
        }

        if (newFileName == FileName)
        {
            // We didn't find a match. If the file name starts with 
            // a backslash, prepend the system drive.
            if (newFileName->Buffer[0] == '\\')
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
            return TRUE;
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
            ULONG indexOfFileName;
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

            newFileName = PhGetDosFullPath(fileName, &indexOfFileName);
            PhDereferenceObject(fileName);

            if (!newFileName)
                goto ContinueLoop;

            moduleInfo.BaseAddress = baseAddress;
            moduleInfo.Size = basicInfo.RegionSize;
            moduleInfo.EntryPoint = NULL;
            moduleInfo.Flags = 0;
            moduleInfo.FileName = newFileName;
            moduleInfo.Name = PhSubstring(
                newFileName,
                indexOfFileName,
                newFileName->Length / 2 - indexOfFileName
                );

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

        if (!NT_SUCCESS(status))
        {
            goto CleanupExit;
        }

#ifndef _M_X64
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
