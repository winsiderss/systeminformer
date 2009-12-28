#include <ph.h>

static PWSTR PhDosDeviceNames[26];

NTSTATUS PhOpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessId
    )
{
    OBJECT_ATTRIBUTES objectAttributes = { 0 };
    CLIENT_ID clientId;

    clientId.UniqueProcess = ProcessId;
    clientId.UniqueThread = NULL;

    return NtOpenProcess(
        ProcessHandle,
        DesiredAccess,
        &objectAttributes,
        &clientId
        );
}

NTSTATUS PhOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ThreadId
    )
{
    OBJECT_ATTRIBUTES objectAttributes = { 0 };
    CLIENT_ID clientId;

    clientId.UniqueProcess = NULL;
    clientId.UniqueThread = ThreadId;

    return NtOpenThread(
        ThreadHandle,
        DesiredAccess,
        &objectAttributes,
        &clientId
        );
}

NTSTATUS PhOpenProcessToken(
    __out PHANDLE TokenHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessHandle
    )
{
    return NtOpenProcessToken(
        ProcessHandle,
        DesiredAccess,
        TokenHandle
        );
}

NTSTATUS PhReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead
    )
{
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
    PVOID address;
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
        &address,
        sizeof(PVOID),
        NULL
        )))
        return status;

    // Read the string structure.
    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(address, offset),
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

NTSTATUS PhEnumProcesses(
    __out PPVOID Processes
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 2048;

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

    return STATUS_SUCCESS;
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
    if (wcsncmp(FileName->Buffer, L"\\??\\", 4) == 0)
    {
        newFileName = PhCreateStringEx(NULL, FileName->Length - 8);
        memcpy(newFileName->Buffer, &FileName->Buffer[4], FileName->Length - 8);
    }
    // "\SystemRoot" means "C:\Windows".
    else if (wcsnicmp(FileName->Buffer, L"\\SystemRoot", 11) == 0)
    {
        PPH_STRING systemDirectory = PhGetSystemDirectory();

        if (systemDirectory)
        {
            ULONG indexOfLastBackslash = (ULONG)(wcsrchr(systemDirectory->Buffer, '\\') - systemDirectory->Buffer);

            newFileName = PhCreateStringEx(NULL, indexOfLastBackslash * 2 + FileName->Length - 11);
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
            ULONG prefixLength = wcslen(prefix);

            if (prefixLength > 0)
            {
                if (wcsnicmp(FileName->Buffer, prefix, prefixLength) == 0)
                {
                    newFileName = PhCreateStringEx(NULL, 4 + FileName->Length - prefixLength * 2);
                    newFileName->Buffer[0] = (WCHAR)('A' + i);
                    newFileName->Buffer[1] = ':';
                    memcpy(&newFileName->Buffer[2], &FileName->Buffer[prefixLength], FileName->Length - prefixLength * 2);

                    break;
                }
            }
        }
    }

    return newFileName;
}
