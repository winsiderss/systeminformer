/*
 * Process Hacker -
 *   native wrapper and support functions
 *
 * Copyright (C) 2009-2016 wj32
 * Copyright (C) 2017 dmex
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

#include <apiimport.h>
#include <kphuser.h>
#include <lsasup.h>
#include <mapimg.h>

#define PH_DEVICE_PREFIX_LENGTH 64
#define PH_DEVICE_MUP_PREFIX_MAX_COUNT 16

typedef BOOLEAN (NTAPI *PPHP_ENUM_PROCESS_MODULES_CALLBACK)(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY Entry,
    _In_ PVOID AddressOfEntry,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    );

typedef BOOLEAN (NTAPI *PPHP_ENUM_PROCESS_MODULES32_CALLBACK)(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY32 Entry,
    _In_ ULONG AddressOfEntry,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    );

static PH_INITONCE PhDevicePrefixesInitOnce = PH_INITONCE_INIT;

static UNICODE_STRING PhDevicePrefixes[26];
static PH_QUEUED_LOCK PhDevicePrefixesLock = PH_QUEUED_LOCK_INIT;

static PPH_STRING PhDeviceMupPrefixes[PH_DEVICE_MUP_PREFIX_MAX_COUNT] = { 0 };
static ULONG PhDeviceMupPrefixesCount = 0;
static PH_QUEUED_LOCK PhDeviceMupPrefixesLock = PH_QUEUED_LOCK_INIT;

static PH_INITONCE PhPredefineKeyInitOnce = PH_INITONCE_INIT;
static UNICODE_STRING PhPredefineKeyNames[PH_KEY_MAXIMUM_PREDEFINE] =
{
    RTL_CONSTANT_STRING(L"\\Registry\\Machine"),
    RTL_CONSTANT_STRING(L"\\Registry\\User"),
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Classes"),
    { 0, 0, NULL }
};
static HANDLE PhPredefineKeyHandles[PH_KEY_MAXIMUM_PREDEFINE] = { 0 };

/**
 * Queries information about the token of the current process.
 */
PH_TOKEN_ATTRIBUTES PhGetOwnTokenAttributes(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PH_TOKEN_ATTRIBUTES attributes = { 0 };

    if (PhBeginInitOnce(&initOnce))
    {
        BOOLEAN elevated = TRUE;
        TOKEN_ELEVATION_TYPE elevationType = TokenElevationTypeFull;

        if (WindowsVersion >= WINDOWS_8)
        {
            attributes.TokenHandle = NtCurrentProcessToken();
        }
        else
        {
            HANDLE tokenHandle;

            if (NT_SUCCESS(PhOpenProcessToken(NtCurrentProcess(), TOKEN_QUERY, &tokenHandle)))
                attributes.TokenHandle = tokenHandle;
        }

        if (attributes.TokenHandle)
        {
            PTOKEN_USER tokenUser;

            PhGetTokenIsElevated(attributes.TokenHandle, &elevated);
            PhGetTokenElevationType(attributes.TokenHandle, &elevationType);

            if (NT_SUCCESS(PhGetTokenUser(attributes.TokenHandle, &tokenUser)))
            {
                attributes.TokenSid = PhAllocateCopy(tokenUser->User.Sid, RtlLengthSid(tokenUser->User.Sid));
                PhFree(tokenUser);
            }
        }

        attributes.Elevated = elevated;
        attributes.ElevationType = elevationType;

        PhEndInitOnce(&initOnce);
    }

    return attributes;
}

/**
 * Opens a process.
 *
 * \param ProcessHandle A variable which receives a handle to the process.
 * \param DesiredAccess The desired access to the process.
 * \param ProcessId The ID of the process.
 */
NTSTATUS PhOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    CLIENT_ID clientId;

    clientId.UniqueProcess = ProcessId;
    clientId.UniqueThread = NULL;

#ifdef _DEBUG
    if (ProcessId == NtCurrentProcessId())
    {
        *ProcessHandle = NtCurrentProcess();
        return STATUS_SUCCESS;
    }
#endif

    if (KphIsVerified() && (DesiredAccess & KPH_PROCESS_READ_ACCESS) == DesiredAccess)
    {
        status = KphOpenProcess(
            ProcessHandle,
            DesiredAccess,
            &clientId
            );
    }
    else
    {
        InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
        status = NtOpenProcess(
            ProcessHandle,
            DesiredAccess,
            &objectAttributes,
            &clientId
            );

        if (status == STATUS_ACCESS_DENIED && KphIsVerified())
        {
            status = KphOpenProcess(
                ProcessHandle,
                DesiredAccess,
                &clientId
                );
        }
    }

    return status;
}

/** Limited API for untrusted/external code. */
NTSTATUS PhOpenProcessPublic(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId
    )
{
    OBJECT_ATTRIBUTES objectAttributes;
    CLIENT_ID clientId;

    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
    clientId.UniqueProcess = ProcessId;
    clientId.UniqueThread = NULL;

    return NtOpenProcess(
        ProcessHandle,
        DesiredAccess,
        &objectAttributes,
        &clientId
        );
}

/**
 * Opens a thread.
 *
 * \param ThreadHandle A variable which receives a handle to the thread.
 * \param DesiredAccess The desired access to the thread.
 * \param ThreadId The ID of the thread.
 */
NTSTATUS PhOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    CLIENT_ID clientId;

    clientId.UniqueProcess = NULL;
    clientId.UniqueThread = ThreadId;

#ifdef _DEBUG
    if (ThreadId == NtCurrentThreadId())
    {
        *ThreadHandle = NtCurrentThread();
        return STATUS_SUCCESS;
    }
#endif

    if (KphIsVerified() && (DesiredAccess & KPH_THREAD_READ_ACCESS) == DesiredAccess)
    {
        status = KphOpenThread(
            ThreadHandle,
            DesiredAccess,
            &clientId
            );
    }
    else
    {
        InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
        status = NtOpenThread(
            ThreadHandle,
            DesiredAccess,
            &objectAttributes,
            &clientId
            );

        if (status == STATUS_ACCESS_DENIED && KphIsVerified())
        {
            status = KphOpenThread(
                ThreadHandle,
                DesiredAccess,
                &clientId
                );
        }
    }

    return status;
}

/** Limited API for untrusted/external code. */
NTSTATUS PhOpenThreadPublic(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadId
    )
{
    OBJECT_ATTRIBUTES objectAttributes;
    CLIENT_ID clientId;

    clientId.UniqueProcess = NULL;
    clientId.UniqueThread = ThreadId;

    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);

    return NtOpenThread(
        ThreadHandle,
        DesiredAccess,
        &objectAttributes,
        &clientId
        );
}

NTSTATUS PhOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle
    )
{
    if (KphIsConnected())
    {
        return KphOpenThreadProcess(
            ThreadHandle,
            DesiredAccess,
            ProcessHandle
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
 * \param ProcessHandle A handle to a process.
 * \param DesiredAccess The desired access to the token.
 * \param TokenHandle A variable which receives a handle to the token.
 */
NTSTATUS PhOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
    )
{
    NTSTATUS status;

    if (KphIsVerified() && (DesiredAccess & KPH_TOKEN_READ_ACCESS) == DesiredAccess)
    {
        status = KphOpenProcessToken(
            ProcessHandle,
            DesiredAccess,
            TokenHandle
            );
    }
    else
    {
        status = NtOpenProcessToken(
            ProcessHandle,
            DesiredAccess,
            TokenHandle
            );

        if (status == STATUS_ACCESS_DENIED && KphIsVerified())
        {
            status = KphOpenProcessToken(
                ProcessHandle,
                DesiredAccess,
                TokenHandle
                );
        }
    }

    return status;
}

NTSTATUS PhGetObjectSecurity(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor
    )
{
    NTSTATUS status;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);
    // This is required (especially for File objects) because some drivers don't seem to handle
    // QuerySecurity properly.
    memset(buffer, 0, bufferSize);

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
        memset(buffer, 0, bufferSize);

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
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
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
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_TERMINATE access.
 * \param ExitStatus A status value that indicates why the process is being terminated.
 */
NTSTATUS PhTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    )
{
    NTSTATUS status;

    if (KphIsVerified())
    {
        status = KphTerminateProcess(
            ProcessHandle,
            ExitStatus
            );

        if (status != STATUS_NOT_SUPPORTED)
            return status;
    }

    return NtTerminateProcess(
        ProcessHandle,
        ExitStatus
        );
}

/** Limited API for untrusted/external code. */
NTSTATUS PhTerminateProcessPublic(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    )
{
    return NtTerminateProcess(
        ProcessHandle,
        ExitStatus
        );
}

/**
 * Queries variable-sized information for a process. The function allocates a buffer to contain the
 * information.
 *
 * \param ProcessHandle A handle to a process. The access required depends on the information class
 * specified.
 * \param ProcessInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer containing the information. You
 * must free the buffer using PhFree() when you no longer need it.
 */
NTSTATUS PhpQueryProcessVariableSize(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG returnLength = 0;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessInformationClass,
        NULL,
        0,
        &returnLength
        );

    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL && status != STATUS_INFO_LENGTH_MISMATCH)
        return status;

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
 * Gets the file name of the process' image.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param FileName A variable which receives a pointer to a string containing the file name. You
 * must free the string using PhDereferenceObject() when you no longer need it.
 */
NTSTATUS PhGetProcessImageFileName(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *FileName
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

    *FileName = PhCreateStringFromUnicodeString(fileName);
    PhFree(fileName);

    return status;
}

/**
 * Gets the Win32 file name of the process' image.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param FileName A variable which receives a pointer to a string containing the file name. You
 * must free the string using PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function is only available on Windows Vista and above.
 */
NTSTATUS PhGetProcessImageFileNameWin32(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    PUNICODE_STRING fileName;

    status = PhpQueryProcessVariableSize(
        ProcessHandle,
        ProcessImageFileNameWin32,
        &fileName
        );

    if (!NT_SUCCESS(status))
        return status;

    *FileName = PhCreateStringFromUnicodeString(fileName);
    PhFree(fileName);

    return status;
}

/**
 * Gets a string stored in a process' parameters structure.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param Offset The string to retrieve.
 * \param String A variable which receives a pointer to the requested string. You must free the
 * string using PhDereferenceObject() when you no longer need it.
 *
 * \retval STATUS_INVALID_PARAMETER_2 An invalid value was specified in the Offset parameter.
 */
NTSTATUS PhGetProcessPebString(
    _In_ HANDLE ProcessHandle,
    _In_ PH_PEB_OFFSET Offset,
    _Out_ PPH_STRING *String
    )
{
    NTSTATUS status;
    PPH_STRING string;
    ULONG offset;

#define PEB_OFFSET_CASE(Enum, Field) \
    case Enum: offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Field); break; \
    case Enum | PhpoWow64: offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS32, Field); break

    switch (Offset)
    {
        PEB_OFFSET_CASE(PhpoCurrentDirectory, CurrentDirectory);
        PEB_OFFSET_CASE(PhpoDllPath, DllPath);
        PEB_OFFSET_CASE(PhpoImagePathName, ImagePathName);
        PEB_OFFSET_CASE(PhpoCommandLine, CommandLine);
        PEB_OFFSET_CASE(PhpoWindowTitle, WindowTitle);
        PEB_OFFSET_CASE(PhpoDesktopInfo, DesktopInfo);
        PEB_OFFSET_CASE(PhpoShellInfo, ShellInfo);
        PEB_OFFSET_CASE(PhpoRuntimeData, RuntimeData);
    default:
        return STATUS_INVALID_PARAMETER_2;
    }

    if (!(Offset & PhpoWow64))
    {
        PROCESS_BASIC_INFORMATION basicInfo;
        PVOID processParameters;
        UNICODE_STRING unicodeString;

        // Get the PEB address.
        if (!NT_SUCCESS(status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo)))
            return status;

        // Read the address of the process parameters.
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
            &processParameters,
            sizeof(PVOID),
            NULL
            )))
            return status;

        // Read the string structure.
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters, offset),
            &unicodeString,
            sizeof(UNICODE_STRING),
            NULL
            )))
            return status;

        string = PhCreateStringEx(NULL, unicodeString.Length);

        // Read the string contents.
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
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
    }
    else
    {
        PVOID peb32;
        ULONG processParameters32;
        UNICODE_STRING32 unicodeString32;

        if (!NT_SUCCESS(status = PhGetProcessPeb32(ProcessHandle, &peb32)))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(peb32, FIELD_OFFSET(PEB32, ProcessParameters)),
            &processParameters32,
            sizeof(ULONG),
            NULL
            )))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters32, offset),
            &unicodeString32,
            sizeof(UNICODE_STRING32),
            NULL
            )))
            return status;

        string = PhCreateStringEx(NULL, unicodeString32.Length);

        // Read the string contents.
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            UlongToPtr(unicodeString32.Buffer),
            string->Buffer,
            string->Length,
            NULL
            )))
        {
            PhDereferenceObject(string);
            return status;
        }
    }

    *String = string;

    return status;
}

/**
 * Gets a process' command line.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION. Before Windows 8.1, the handle must also have PROCESS_VM_READ
 * access.
 * \param String A variable which receives a pointer to a string containing the command line. You
 * must free the string using PhDereferenceObject() when you no longer need it.
 */
NTSTATUS PhGetProcessCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *CommandLine
    )
{
    NTSTATUS status;

    if (WindowsVersion >= WINDOWS_8_1)
    {
        PUNICODE_STRING commandLine;

        status = PhpQueryProcessVariableSize(
            ProcessHandle,
            ProcessCommandLineInformation,
            &commandLine
            );

        if (NT_SUCCESS(status))
        {
            *CommandLine = PhCreateStringFromUnicodeString(commandLine);
            PhFree(commandLine);

            return status;
        }
    }

    return PhGetProcessPebString(ProcessHandle, PhpoCommandLine, CommandLine);
}

NTSTATUS PhGetProcessDesktopInfo(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *DesktopInfo
    )
{
    return PhGetProcessPebString(ProcessHandle, PhpoDesktopInfo, DesktopInfo);
}

/**
 * Gets the window flags and window title of a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION. Before Windows 7 SP1, the handle must also have
 * PROCESS_VM_READ access.
 * \param WindowFlags A variable which receives the window flags.
 * \param WindowTitle A variable which receives a pointer to the window title. You must free the
 * string using PhDereferenceObject() when you no longer need it.
 */
NTSTATUS PhGetProcessWindowTitle(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG WindowFlags,
    _Out_ PPH_STRING *WindowTitle
    )
{
    NTSTATUS status;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif
    ULONG windowFlags;
    PPROCESS_WINDOW_INFORMATION windowInfo;

    status = PhpQueryProcessVariableSize(
        ProcessHandle,
        ProcessWindowInformation,
        &windowInfo
        );

    if (NT_SUCCESS(status))
    {
        *WindowFlags = windowInfo->WindowFlags;
        *WindowTitle = PhCreateStringEx(windowInfo->WindowTitle, windowInfo->WindowTitleLength);
        PhFree(windowInfo);

        return status;
    }

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (!isWow64)
#endif
    {
        PROCESS_BASIC_INFORMATION basicInfo;
        PVOID processParameters;

        // Get the PEB address.
        if (!NT_SUCCESS(status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo)))
            return status;

        // Read the address of the process parameters.
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
            &processParameters,
            sizeof(PVOID),
            NULL
            )))
            return status;

        // Read the window flags.
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters, FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, WindowFlags)),
            &windowFlags,
            sizeof(ULONG),
            NULL
            )))
            return status;
    }
#ifdef _WIN64
    else
    {
        PVOID peb32;
        ULONG processParameters32;

        if (!NT_SUCCESS(status = PhGetProcessPeb32(ProcessHandle, &peb32)))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(peb32, FIELD_OFFSET(PEB32, ProcessParameters)),
            &processParameters32,
            sizeof(ULONG),
            NULL
            )))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters32, FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS32, WindowFlags)),
            &windowFlags,
            sizeof(ULONG),
            NULL
            )))
            return status;
    }
#endif

#ifdef _WIN64
    status = PhGetProcessPebString(ProcessHandle, PhpoWindowTitle | (isWow64 ? PhpoWow64 : 0), WindowTitle);
#else
    status = PhGetProcessPebString(ProcessHandle, PhpoWindowTitle, WindowTitle);
#endif

    if (NT_SUCCESS(status))
        *WindowFlags = windowFlags;

    return status;
}

NTSTATUS PhGetProcessDepStatus(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG DepStatus
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
 * Gets a process' environment block.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION and
 * PROCESS_VM_READ access.
 * \param Flags A combination of flags.
 * \li \c PH_GET_PROCESS_ENVIRONMENT_WOW64 Retrieve the environment block from the WOW64 PEB.
 * \param Environment A variable which will receive a pointer to the environment block copied from
 * the process. You must free the block using PhFreePage() when you no longer need it.
 * \param EnvironmentLength A variable which will receive the length of the environment block, in
 * bytes.
 */
NTSTATUS PhGetProcessEnvironment(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Flags,
    _Out_ PVOID *Environment,
    _Out_ PULONG EnvironmentLength
    )
{
    NTSTATUS status;
    PVOID environmentRemote;
    MEMORY_BASIC_INFORMATION mbi;
    PVOID environment;
    ULONG environmentLength;

    if (!(Flags & PH_GET_PROCESS_ENVIRONMENT_WOW64))
    {
        PROCESS_BASIC_INFORMATION basicInfo;
        PVOID processParameters;

        status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo);

        if (!NT_SUCCESS(status))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
            &processParameters,
            sizeof(PVOID),
            NULL
            )))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters, FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Environment)),
            &environmentRemote,
            sizeof(PVOID),
            NULL
            )))
            return status;
    }
    else
    {
        PVOID peb32;
        ULONG processParameters32;
        ULONG environmentRemote32;

        status = PhGetProcessPeb32(ProcessHandle, &peb32);

        if (!NT_SUCCESS(status))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(peb32, FIELD_OFFSET(PEB32, ProcessParameters)),
            &processParameters32,
            sizeof(ULONG),
            NULL
            )))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters32, FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS32, Environment)),
            &environmentRemote32,
            sizeof(ULONG),
            NULL
            )))
            return status;

        environmentRemote = UlongToPtr(environmentRemote32);
    }

    if (!NT_SUCCESS(status = NtQueryVirtualMemory(
        ProcessHandle,
        environmentRemote,
        MemoryBasicInformation,
        &mbi,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
        return status;

    environmentLength = (ULONG)(mbi.RegionSize -
        ((ULONG_PTR)environmentRemote - (ULONG_PTR)mbi.BaseAddress));

    // Read in the entire region of memory.

    environment = PhAllocatePage(environmentLength, NULL);

    if (!environment)
        return STATUS_NO_MEMORY;

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        ProcessHandle,
        environmentRemote,
        environment,
        environmentLength,
        NULL
        )))
    {
        PhFreePage(environment);
        return status;
    }

    *Environment = environment;

    if (EnvironmentLength)
        *EnvironmentLength = environmentLength;

    return status;
}

BOOLEAN PhEnumProcessEnvironmentVariables(
    _In_ PVOID Environment,
    _In_ ULONG EnvironmentLength,
    _Inout_ PULONG EnumerationKey,
    _Out_ PPH_ENVIRONMENT_VARIABLE Variable
    )
{
    ULONG length;
    ULONG startIndex;
    PWCHAR name;
    ULONG nameLength;
    PWCHAR value;
    ULONG valueLength;
    PWCHAR currentChar;
    ULONG currentIndex;

    length = EnvironmentLength / sizeof(WCHAR);

    currentIndex = *EnumerationKey;
    currentChar = (PWCHAR)Environment + currentIndex;
    startIndex = currentIndex;
    name = currentChar;

    // Find the end of the name.
    while (TRUE)
    {
        if (currentIndex >= length)
            return FALSE;
        if (*currentChar == '=')
            break;
        if (*currentChar == 0)
            return FALSE; // no more variables

        currentIndex++;
        currentChar++;
    }

    nameLength = currentIndex - startIndex;

    currentIndex++;
    currentChar++;
    startIndex = currentIndex;
    value = currentChar;

    // Find the end of the value.
    while (TRUE)
    {
        if (currentIndex >= length)
            return FALSE;
        if (*currentChar == 0)
            break;

        currentIndex++;
        currentChar++;
    }

    valueLength = currentIndex - startIndex;

    currentIndex++;
    *EnumerationKey = currentIndex;

    Variable->Name.Buffer = name;
    Variable->Name.Length = nameLength * sizeof(WCHAR);
    Variable->Value.Buffer = value;
    Variable->Value.Length = valueLength * sizeof(WCHAR);

    return TRUE;
}

NTSTATUS PhQueryEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_ PPH_STRINGREF Name,
    _Out_opt_ PPH_STRING* Value
    )
{
    NTSTATUS status;
    UNICODE_STRING variableNameUs;
    UNICODE_STRING variableValueUs;

    PhStringRefToUnicodeString(Name, &variableNameUs);

    if (Value)
    {
        variableValueUs.Length = 0x100 * sizeof(WCHAR);
        variableValueUs.MaximumLength = variableValueUs.Length + sizeof(UNICODE_NULL);
        variableValueUs.Buffer = PhAllocateZero(variableValueUs.MaximumLength);
    }
    else
    {
        RtlInitUnicodeString(&variableValueUs, UNICODE_NULL);
    }

    status = RtlQueryEnvironmentVariable_U(
        Environment,
        &variableNameUs,
        &variableValueUs
        );

    if (Value && status == STATUS_BUFFER_TOO_SMALL)
    {
        if (variableValueUs.Length + sizeof(UNICODE_NULL) > UNICODE_STRING_MAX_BYTES)
            variableValueUs.MaximumLength = variableValueUs.Length;
        else
            variableValueUs.MaximumLength = variableValueUs.Length + sizeof(UNICODE_NULL);

        PhFree(variableValueUs.Buffer);
        variableValueUs.Buffer = PhAllocateZero(variableValueUs.MaximumLength);

        status = RtlQueryEnvironmentVariable_U(
            Environment,
            &variableNameUs,
            &variableValueUs
            );
    }

    if (Value && NT_SUCCESS(status))
    {
        *Value = PhCreateStringFromUnicodeString(&variableValueUs);
    }

    if (Value && variableValueUs.Buffer)
    {
        PhFree(variableValueUs.Buffer);
    }

    return status;
}

/**
 * Gets the file name of a mapped section.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION
 * access.
 * \param BaseAddress The base address of the section view.
 * \param FileName A variable which receives a pointer to a string containing the file name of the
 * section. You must free the string using PhDereferenceObject() when you no longer need it.
 */
NTSTATUS PhGetProcessMappedFileName(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    SIZE_T bufferSize;
    SIZE_T returnLength;
    PUNICODE_STRING buffer;

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

    *FileName = PhCreateStringFromUnicodeString(buffer);
    PhFree(buffer);

    return status;
}

/**
 * Gets working set information for a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION
 * access.
 * \param WorkingSetInformation A variable which receives a pointer to the information. You must
 * free the buffer using PhFree() when you no longer need it.
 */
NTSTATUS PhGetProcessWorkingSetInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PMEMORY_WORKING_SET_INFORMATION *WorkingSetInformation
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

        // Fail if we're resizing the buffer to something very large.
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
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION
 * access.
 * \param WsCounters A variable which receives the counters.
 */
NTSTATUS PhGetProcessWsCounters(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_WS_COUNTERS WsCounters
    )
{
    NTSTATUS status;
    PMEMORY_WORKING_SET_INFORMATION wsInfo;
    PH_PROCESS_WS_COUNTERS wsCounters;
    ULONG_PTR i;

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

NTSTATUS PhGetProcessUnloadedDlls(
    _In_ HANDLE ProcessId,
    _Out_ PVOID *EventTrace,
    _Out_ ULONG *EventTraceSize,
    _Out_ ULONG *EventTraceCount
    )
{
    NTSTATUS status;
    PULONG elementSize;
    PULONG elementCount;
    PVOID eventTrace;
    HANDLE processHandle = NULL;
    ULONG eventTraceSize;
    ULONG capturedElementSize;
    ULONG capturedElementCount;
    PVOID capturedEventTracePointer;
    PVOID capturedEventTrace = NULL;

    RtlGetUnloadEventTraceEx(&elementSize, &elementCount, &eventTrace);

    if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_VM_READ, ProcessId)))
        goto CleanupExit;

    // We have the pointers for the unload event trace information.
    // Since ntdll is loaded at the same base address across all processes,
    // we can read the information in.

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        processHandle,
        elementSize,
        &capturedElementSize,
        sizeof(ULONG),
        NULL
        )))
        goto CleanupExit;

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        processHandle,
        elementCount,
        &capturedElementCount,
        sizeof(ULONG),
        NULL
        )))
        goto CleanupExit;

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        processHandle,
        eventTrace,
        &capturedEventTracePointer,
        sizeof(PVOID),
        NULL
        )))
        goto CleanupExit;

    if (!capturedEventTracePointer)
    {
        status = STATUS_NOT_FOUND; // no events
        goto CleanupExit;
    }

    if (capturedElementCount > 0x4000)
        capturedElementCount = 0x4000;

    eventTraceSize = capturedElementSize * capturedElementCount;
    capturedEventTrace = PhAllocateSafe(eventTraceSize);

    if (!capturedEventTrace)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        processHandle,
        capturedEventTracePointer,
        capturedEventTrace,
        eventTraceSize,
        NULL
        )))
        goto CleanupExit;

CleanupExit:

    if (processHandle)
        NtClose(processHandle);

    if (NT_SUCCESS(status))
    {
        *EventTrace = capturedEventTrace;
        *EventTraceSize = capturedElementSize;
        *EventTraceCount = capturedElementCount;
    }

    return status;
}

/**
 * Causes a process to load a DLL.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_CREATE_THREAD, PROCESS_VM_OPERATION, PROCESS_VM_READ
 * and PROCESS_VM_WRITE access.
 * \param FileName The file name of the DLL to inject.
 * \param Timeout The timeout, in milliseconds, for the process to load the DLL.
 *
 * \remarks If the process does not load the DLL before the timeout expires it may crash. Choose the
 * timeout value carefully.
 */
NTSTATUS PhLoadDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PWSTR FileName,
    _In_opt_ PLARGE_INTEGER Timeout
    ) 
{
#ifdef _WIN64
    static PVOID loadLibraryW32 = NULL;
#endif
    NTSTATUS status;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
    BOOLEAN isModule32 = FALSE;
    PH_MAPPED_IMAGE mappedImage;
#endif
    PVOID threadStart;
    PH_STRINGREF fileName;
    PVOID baseAddress = NULL;
    SIZE_T allocSize;
    HANDLE threadHandle;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        if (!NT_SUCCESS(status = PhLoadMappedImage(FileName, NULL, TRUE, &mappedImage)))
            return status;

        isModule32 = mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC;
        PhUnloadMappedImage(&mappedImage);
    }

    if (!isModule32)
    {
#endif
        threadStart = PhGetModuleProcAddress(L"kernel32.dll", "LoadLibraryW");
#ifdef _WIN64
    }
    else
    {
        threadStart = loadLibraryW32;

        if (!threadStart)
        {
            PH_STRINGREF systemRoot;
            PPH_STRING kernel32FileName;

            PhGetSystemRoot(&systemRoot);
            kernel32FileName = PhConcatStringRefZ(&systemRoot, L"\\SysWow64\\kernel32.dll");

            status = PhGetProcedureAddressRemote(
                ProcessHandle,
                kernel32FileName->Buffer,
                "LoadLibraryW",
                0,
                &loadLibraryW32,
                NULL
                );
            PhDereferenceObject(kernel32FileName);

            if (!NT_SUCCESS(status))
                return status;

            threadStart = loadLibraryW32;
        }
    }
#endif

    PhInitializeStringRefLongHint(&fileName, FileName);
    allocSize = fileName.Length + sizeof(WCHAR);

    if (!NT_SUCCESS(status = NtAllocateVirtualMemory(
        ProcessHandle,
        &baseAddress,
        0,
        &allocSize,
        MEM_COMMIT,
        PAGE_READWRITE
        )))
        return status;

    if (!NT_SUCCESS(status = NtWriteVirtualMemory(
        ProcessHandle,
        baseAddress,
        fileName.Buffer,
        fileName.Length + sizeof(WCHAR),
        NULL
        )))
        goto FreeExit;

    if (!NT_SUCCESS(status = RtlCreateUserThread(
        ProcessHandle,
        NULL,
        FALSE,
        0,
        0,
        0,
        threadStart,
        baseAddress,
        &threadHandle,
        NULL
        )))
        goto FreeExit;
    
    // Wait for the thread to finish.	
    status = NtWaitForSingleObject(threadHandle, FALSE, Timeout);
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
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_CREATE_THREAD, PROCESS_VM_OPERATION, PROCESS_VM_READ
 * and PROCESS_VM_WRITE access.
 * \param BaseAddress The base address of the DLL to unload.
 * \param Timeout The timeout, in milliseconds, for the process to unload the DLL.
 */
NTSTATUS PhUnloadDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
#ifdef _WIN64
    static PVOID ldrUnloadDll32 = NULL;
#endif

    NTSTATUS status;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
    BOOLEAN isModule32 = FALSE;
#endif
    HANDLE threadHandle;
    THREAD_BASIC_INFORMATION basicInfo;
    PVOID threadStart;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);
#endif

    // No point trying to set the load count on Windows 8 and higher, because NT now uses a DAG of
    // loader nodes.
    if (WindowsVersion < WINDOWS_8)
    {
        status = PhSetProcessModuleLoadCount(
            ProcessHandle,
            BaseAddress,
            1
            );

#ifdef _WIN64
        if (isWow64 && status == STATUS_DLL_NOT_FOUND)
        {
            // The DLL might be 32-bit.
            status = PhSetProcessModuleLoadCount32(
                ProcessHandle,
                BaseAddress,
                1
                );

            if (NT_SUCCESS(status))
                isModule32 = TRUE;
        }
#endif

        if (!NT_SUCCESS(status))
            return status;
    }

#ifdef _WIN64
    if (!isModule32)
    {
#endif
        threadStart = PhGetDllProcedureAddress(L"ntdll.dll", "LdrUnloadDll", 0);
#ifdef _WIN64
    }
    else
    {
        threadStart = ldrUnloadDll32;

        if (!threadStart)
        {
            PH_STRINGREF systemRoot;
            PPH_STRING ntdll32FileName;

            PhGetSystemRoot(&systemRoot);
            ntdll32FileName = PhConcatStringRefZ(&systemRoot, L"\\SysWow64\\ntdll.dll");

            status = PhGetProcedureAddressRemote(
                ProcessHandle,
                ntdll32FileName->Buffer,
                "LdrUnloadDll",
                0,
                &ldrUnloadDll32,
                NULL
                );
            PhDereferenceObject(ntdll32FileName);

            if (!NT_SUCCESS(status))
                return status;

            threadStart = ldrUnloadDll32;
        }
    }
#endif

    status = RtlCreateUserThread(
        ProcessHandle,
        NULL,
        FALSE,
        0,
        0,
        0,
        (PUSER_THREAD_START_ROUTINE)threadStart,
        BaseAddress,
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtWaitForSingleObject(threadHandle, FALSE, Timeout);

    if (status == STATUS_WAIT_0)
    {
        status = PhGetThreadBasicInformation(threadHandle, &basicInfo);

        if (NT_SUCCESS(status))
            status = basicInfo.ExitStatus;
    }

    NtClose(threadHandle);

    return status;
}

/**
 * Sets an environment variable in a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_CREATE_THREAD, PROCESS_VM_OPERATION, PROCESS_VM_READ
 * and PROCESS_VM_WRITE access.
 * \param Name The name of the environment variable to set.
 * \param Value The new value of the environment variable. If this parameter is NULL, the
 * environment variable is deleted.
 * \param Timeout The timeout, in milliseconds, for the process to set the environment variable.
 */
NTSTATUS PhSetEnvironmentVariableRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF Name,
    _In_opt_ PPH_STRINGREF Value,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif
    PPH_STRING ntdllFileName = NULL;
    PPH_STRING kernel32FileName = NULL;
    PVOID nameBaseAddress = NULL;
    PVOID valueBaseAddress = NULL;
    SIZE_T nameAllocationSize = 0;
    SIZE_T valueAllocationSize = 0;
    PVOID rtlExitUserThread = NULL;
    PVOID setEnvironmentVariableW = NULL;
    HANDLE threadHandle = NULL;

    nameAllocationSize = Name->Length + sizeof(WCHAR);

    if (Value)
        valueAllocationSize = Value->Length + sizeof(WCHAR);

#ifdef _WIN64
    if (!NT_SUCCESS(status = PhGetProcessIsWow64(ProcessHandle, &isWow64)))
        goto CleanupExit;

    if (isWow64)
    {
        PH_STRINGREF systemRootSr;

        PhGetSystemRoot(&systemRootSr);
        ntdllFileName = PhConcatStringRefZ(&systemRootSr, L"\\SysWow64\\ntdll.dll");
        kernel32FileName = PhConcatStringRefZ(&systemRootSr, L"\\SysWow64\\kernel32.dll");
    }
    else
    {
#endif
        PH_STRINGREF systemRootSr;

        PhGetSystemRoot(&systemRootSr);
        ntdllFileName = PhConcatStringRefZ(&systemRootSr, L"\\System32\\ntdll.dll");
        kernel32FileName = PhConcatStringRefZ(&systemRootSr, L"\\System32\\kernel32.dll");
#ifdef _WIN64
    }
#endif

    if (!NT_SUCCESS(status = PhGetProcedureAddressRemote(
        ProcessHandle,
        ntdllFileName->Buffer,
        "RtlExitUserThread",
        0,
        &rtlExitUserThread,
        NULL
        )))
    {
        goto CleanupExit;
    }
    if (!NT_SUCCESS(status = PhGetProcedureAddressRemote(
        ProcessHandle,
        kernel32FileName->Buffer,
        "SetEnvironmentVariableW",
        0,
        &setEnvironmentVariableW,
        NULL
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = NtAllocateVirtualMemory(
        ProcessHandle,
        &nameBaseAddress,
        0,
        &nameAllocationSize,
        MEM_COMMIT,
        PAGE_READWRITE
        )))
    {
        goto CleanupExit;
    }
    if (!NT_SUCCESS(status = NtWriteVirtualMemory(
        ProcessHandle,
        nameBaseAddress,
        Name->Buffer,
        Name->Length,
        NULL
        )))
    {
        goto CleanupExit;
    }

    if (Value)
    {
        if (!NT_SUCCESS(status = NtAllocateVirtualMemory(
            ProcessHandle,
            &valueBaseAddress,
            0,
            &valueAllocationSize,
            MEM_COMMIT,
            PAGE_READWRITE
            )))
        {
            goto CleanupExit;
        }
        if (!NT_SUCCESS(status = NtWriteVirtualMemory(
            ProcessHandle,
            valueBaseAddress,
            Value->Buffer,
            Value->Length,
            NULL
            )))
        {
            goto CleanupExit;
        }
    }

    if (!NT_SUCCESS(status = RtlCreateUserThread(
        ProcessHandle,
        NULL,
        TRUE,
        0,
        0,
        0,
        (PUSER_THREAD_START_ROUTINE)rtlExitUserThread,
        NULL,
        &threadHandle,
        NULL
        )))
    {
        goto CleanupExit;
    }

#ifdef _WIN64
    if (isWow64)
    {
        if (!NT_SUCCESS(status = RtlQueueApcWow64Thread(
            threadHandle,
            setEnvironmentVariableW,
            nameBaseAddress,
            valueBaseAddress,
            NULL
            )))
        {
            goto CleanupExit;
        }
    }
    else
    {
#endif
        if (!NT_SUCCESS(status = NtQueueApcThreadEx(
            threadHandle,
            NULL,
            setEnvironmentVariableW,
            nameBaseAddress,
            valueBaseAddress,
            NULL
            )))
        {
            goto CleanupExit;
        }
#ifdef _WIN64
    }
#endif

    status = NtAlertResumeThread(threadHandle, NULL);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWaitForSingleObject(threadHandle, FALSE, Timeout);

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (nameBaseAddress)
    {
        nameAllocationSize = 0;
        NtFreeVirtualMemory(
            ProcessHandle,
            &nameBaseAddress,
            &nameAllocationSize,
            MEM_RELEASE
            );
    }

    if (valueBaseAddress)
    {
        valueAllocationSize = 0;
        NtFreeVirtualMemory(
            ProcessHandle,
            &valueBaseAddress,
            &valueAllocationSize,
            MEM_RELEASE
            );
    }

    PhClearReference(&ntdllFileName);
    PhClearReference(&kernel32FileName);

    return status;
}

NTSTATUS PhGetJobProcessIdList(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_PROCESS_ID_LIST *ProcessIdList
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
 * Queries variable-sized information for a token. The function allocates a buffer to contain the
 * information.
 *
 * \param TokenHandle A handle to a token. The access required depends on the information class
 * specified.
 * \param TokenInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer containing the information. You
 * must free the buffer using PhFree() when you no longer need it.
 */
NTSTATUS PhpQueryTokenVariableSize(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_ PVOID *Buffer
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
 * Queries variable-sized information for a token. The function allocates a buffer to contain the
 * information.
 *
 * \param TokenHandle A handle to a token. The access required depends on the information class
 * specified.
 * \param TokenInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer containing the information. You
 * must free the buffer using PhFree() when you no longer need it.
 */
NTSTATUS PhQueryTokenVariableSize(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_ PVOID *Buffer
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenInformationClass,
        Buffer
        );
}

/**
 * Gets a token's user.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param User A variable which receives a pointer to a structure containing the token's user. You
 * must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhGetTokenUser(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_USER *User
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
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Owner A variable which receives a pointer to a structure containing the token's owner. You
 * must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhGetTokenOwner(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_OWNER *Owner
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
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param PrimaryGroup A variable which receives a pointer to a structure containing the token's
 * primary group. You must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhGetTokenPrimaryGroup(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_PRIMARY_GROUP *PrimaryGroup
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenPrimaryGroup,
        PrimaryGroup
        );
}

/**
 * Gets a token's groups.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Groups A variable which receives a pointer to a structure containing the token's groups.
 * You must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhGetTokenGroups(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_GROUPS *Groups
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenGroups,
        Groups
        );
}

/**
 * Get a token's restricted SIDs.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param RestrictedSids A variable which receives a pointer to a structure containing the token's restricted SIDs.
 * You must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhGetTokenRestrictedSids(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_GROUPS* RestrictedSids
)
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenRestrictedSids,
        RestrictedSids
        );
}

/**
 * Gets a token's privileges.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Privileges A variable which receives a pointer to a structure containing the token's
 * privileges. You must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhGetTokenPrivileges(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_PRIVILEGES *Privileges
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenPrivileges,
        Privileges
        );
}

NTSTATUS PhGetTokenTrustLevel(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_PROCESS_TRUST_LEVEL *TrustLevel
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenProcessTrustLevel,
        TrustLevel
        );
}

NTSTATUS PhSetTokenSessionId(
    _In_ HANDLE TokenHandle,
    _In_ ULONG SessionId
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
 * \param TokenHandle A handle to a token. The handle must have TOKEN_ADJUST_PRIVILEGES access.
 * \param PrivilegeName The name of the privilege to modify. If this parameter is NULL, you must
 * specify a LUID in the \a PrivilegeLuid parameter.
 * \param PrivilegeLuid The LUID of the privilege to modify. If this parameter is NULL, you must
 * specify a name in the \a PrivilegeName parameter.
 * \param Attributes The new attributes of the privilege.
 */
BOOLEAN PhSetTokenPrivilege(
    _In_ HANDLE TokenHandle,
    _In_opt_ PWSTR PrivilegeName,
    _In_opt_ PLUID PrivilegeLuid,
    _In_ ULONG Attributes
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
        PH_STRINGREF privilegeName;

        PhInitializeStringRef(&privilegeName, PrivilegeName);

        if (!PhLookupPrivilegeValue(
            &privilegeName,
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

BOOLEAN PhSetTokenPrivilege2(
    _In_ HANDLE TokenHandle,
    _In_ LONG Privilege,
    _In_ ULONG Attributes
    )
{
    LUID privilegeLuid;

    privilegeLuid = RtlConvertLongToLuid(Privilege);

    return PhSetTokenPrivilege(TokenHandle, NULL, &privilegeLuid, Attributes);
}

/**
* Modifies a token group.
*
* \param TokenHandle A handle to a token. The handle must have TOKEN_ADJUST_GROUPS access.
* \param GroupName The name of the group to modify. If this parameter is NULL, you must
* specify a PSID in the \a GroupSid parameter.
* \param GroupSid The PSID of the group to modify. If this parameter is NULL, you must
* specify a group name in the \a GroupName parameter.
* \param Attributes The new attributes of the group.
*/
NTSTATUS PhSetTokenGroups(
    _In_ HANDLE TokenHandle,
    _In_opt_ PWSTR GroupName,
    _In_opt_ PSID GroupSid,
    _In_ ULONG Attributes
    )
{
    NTSTATUS status;
    TOKEN_GROUPS groups;

    groups.GroupCount = 1;
    groups.Groups[0].Attributes = Attributes;

    if (GroupSid)
    {
        groups.Groups[0].Sid = GroupSid;
    }
    else if (GroupName)
    {
        PH_STRINGREF groupName;

        PhInitializeStringRef(&groupName, GroupName);

        if (!NT_SUCCESS(status = PhLookupName(&groupName, &groups.Groups[0].Sid, NULL, NULL)))
            return status;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    status = NtAdjustGroupsToken(
        TokenHandle,
        FALSE,
        &groups,
        0,
        NULL,
        NULL
        );

    if (GroupName && groups.Groups[0].Sid)
        PhFree(groups.Groups[0].Sid);

    return status;
}

/**
 * Sets whether virtualization is enabled for a token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_WRITE access.
 * \param IsVirtualizationEnabled A boolean indicating whether virtualization is to be enabled for
 * the token.
 */
NTSTATUS PhSetTokenIsVirtualizationEnabled(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN IsVirtualizationEnabled
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
* Gets a token's integrity level RID. Can handle custom integrity levels.
*
* \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
* \param IntegrityLevelRID A variable which receives the integrity level of the token.
* \param IntegrityString A variable which receives a pointer to a string containing a string
* representation of the integrity level.
*/
NTSTATUS PhGetTokenIntegrityLevelRID(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PMANDATORY_LEVEL_RID IntegrityLevelRID,
    _Out_opt_ PWSTR *IntegrityString
    )
{
    NTSTATUS status;
    PTOKEN_MANDATORY_LABEL mandatoryLabel;
    ULONG subAuthority;
    PWSTR integrityString;

    status = PhpQueryTokenVariableSize(TokenHandle, TokenIntegrityLevel, &mandatoryLabel);

    if (!NT_SUCCESS(status))
        return status;

    subAuthority = *RtlSubAuthoritySid(mandatoryLabel->Label.Sid, 0);
    PhFree(mandatoryLabel);

    if (IntegrityString)
    {
        switch (subAuthority)
        {
        case SECURITY_MANDATORY_UNTRUSTED_RID:
            integrityString = L"Untrusted";
            break;
        case SECURITY_MANDATORY_LOW_RID:
            integrityString = L"Low";
            break;
        case SECURITY_MANDATORY_MEDIUM_RID:
            integrityString = L"Medium";
            break;
        case SECURITY_MANDATORY_MEDIUM_PLUS_RID:
            integrityString = L"Medium +";
            break;
        case SECURITY_MANDATORY_HIGH_RID:
            integrityString = L"High";
            break;
        case SECURITY_MANDATORY_SYSTEM_RID:
            integrityString = L"System";
            break;
        case SECURITY_MANDATORY_PROTECTED_PROCESS_RID:
            integrityString = L"Protected";
            break;
        default:
            integrityString = L"Other";
            break;
        }

        *IntegrityString = integrityString;
    }

    if (IntegrityLevelRID)
        *IntegrityLevelRID = subAuthority;

    return status;
}

/**
 * Gets a token's integrity level.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param IntegrityLevel A variable which receives the integrity level of the token.
 * If the integrity level is not a well-known one the function fails.
 * \param IntegrityString A variable which receives a pointer to a string containing a string
 * representation of the integrity level.
 */
NTSTATUS PhGetTokenIntegrityLevel(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PMANDATORY_LEVEL IntegrityLevel,
    _Out_opt_ PWSTR *IntegrityString
    )
{
    NTSTATUS status;
    MANDATORY_LEVEL_RID integrityLevelRID;
    MANDATORY_LEVEL integrityLevel;

    status = PhGetTokenIntegrityLevelRID(TokenHandle, &integrityLevelRID, IntegrityString);

    if (!NT_SUCCESS(status))
        return status;

    if (IntegrityLevel)
    {
        switch (integrityLevelRID)
        {
        case SECURITY_MANDATORY_UNTRUSTED_RID:
            integrityLevel = MandatoryLevelUntrusted;
            break;
        case SECURITY_MANDATORY_LOW_RID:
            integrityLevel = MandatoryLevelLow;
            break;
        case SECURITY_MANDATORY_MEDIUM_RID:
            integrityLevel = MandatoryLevelMedium;
            break;
        case SECURITY_MANDATORY_HIGH_RID:
            integrityLevel = MandatoryLevelHigh;
            break;
        case SECURITY_MANDATORY_SYSTEM_RID:
            integrityLevel = MandatoryLevelSystem;
            break;
        case SECURITY_MANDATORY_PROTECTED_PROCESS_RID:
            integrityLevel = MandatoryLevelSecureProcess;
            break;
        default:
            return STATUS_UNSUCCESSFUL;
        }

        *IntegrityLevel = integrityLevel;
    }

    return status;
}

NTSTATUS PhpQueryFileVariableSize(
    _In_ HANDLE FileHandle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_ PVOID *Buffer
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

        if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
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
    _In_ HANDLE FileHandle,
    _Out_ PLARGE_INTEGER Size
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
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER Size
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
    _In_ HANDLE TransactionManagerHandle,
    _In_ TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;

    if (!NtQueryInformationTransactionManager_Import())
        return STATUS_NOT_SUPPORTED;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryInformationTransactionManager_Import()(
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
    _In_ HANDLE TransactionManagerHandle,
    _Out_ PTRANSACTIONMANAGER_BASIC_INFORMATION BasicInformation
    )
{
    if (NtQueryInformationTransactionManager_Import())
    {
        return NtQueryInformationTransactionManager_Import()(
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
    _In_ HANDLE TransactionManagerHandle,
    _Out_ PPH_STRING *LogFileName
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
    _In_ HANDLE TransactionHandle,
    _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;

    if (!NtQueryInformationTransaction_Import())
        return STATUS_NOT_SUPPORTED;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryInformationTransaction_Import()(
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
    _In_ HANDLE TransactionHandle,
    _Out_ PTRANSACTION_BASIC_INFORMATION BasicInformation
    )
{
    if (NtQueryInformationTransaction_Import())
    {
        return NtQueryInformationTransaction_Import()(
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
    _In_ HANDLE TransactionHandle,
    _Out_opt_ PLARGE_INTEGER Timeout,
    _Out_opt_ TRANSACTION_OUTCOME *Outcome,
    _Out_opt_ PPH_STRING *Description
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
    _In_ HANDLE ResourceManagerHandle,
    _In_ RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;

    if (!NtQueryInformationResourceManager_Import())
        return STATUS_NOT_SUPPORTED;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryInformationResourceManager_Import()(
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
    _In_ HANDLE ResourceManagerHandle,
    _Out_opt_ PGUID Guid,
    _Out_opt_ PPH_STRING *Description
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
    _In_ HANDLE EnlistmentHandle,
    _Out_ PENLISTMENT_BASIC_INFORMATION BasicInformation
    )
{
    if (NtQueryInformationEnlistment_Import())
    {
        return NtQueryInformationEnlistment_Import()(
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
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_opt_ PVOID Context
    )
{
    static PH_STRINGREF driverDirectoryName = PH_STRINGREF_INIT(L"\\Driver\\");

    NTSTATUS status;
    POPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT context = Context;
    PPH_STRING driverName;
    UNICODE_STRING driverNameUs;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE driverHandle;
    DRIVER_BASIC_INFORMATION basicInfo;

    driverName = PhConcatStringRef2(&driverDirectoryName, Name);

    if (!PhStringRefToUnicodeString(&driverName->sr, &driverNameUs))
    {
        PhDereferenceObject(driverName);
        return TRUE;
    }

    InitializeObjectAttributes(
        &objectAttributes,
        &driverNameUs,
        0,
        NULL,
        NULL
        );

    status = KphOpenDriver(&driverHandle, SYNCHRONIZE, &objectAttributes);
    PhDereferenceObject(driverName);

    if (!NT_SUCCESS(status))
        return TRUE;

    status = KphQueryInformationDriver(
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
 * \param DriverHandle A variable which receives a handle to the driver object.
 * \param BaseAddress The base address of the driver to open.
 *
 * \retval STATUS_OBJECT_NAME_NOT_FOUND The driver could not be found.
 *
 * \remarks This function requires a valid KProcessHacker handle.
 */
NTSTATUS PhOpenDriverByBaseAddress(
    _Out_ PHANDLE DriverHandle,
    _In_ PVOID BaseAddress
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
 * Queries variable-sized information for a driver. The function allocates a buffer to contain the
 * information.
 *
 * \param DriverHandle A handle to a driver. The access required depends on the information class
 * specified.
 * \param DriverInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer containing the information. You
 * must free the buffer using PhFree() when you no longer need it.
 *
 * \remarks This function requires a valid KProcessHacker handle.
 */
NTSTATUS PhpQueryDriverVariableSize(
    _In_ HANDLE DriverHandle,
    _In_ DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG returnLength = 0;

    KphQueryInformationDriver(
        DriverHandle,
        DriverInformationClass,
        NULL,
        0,
        &returnLength
        );
    buffer = PhAllocate(returnLength);
    status = KphQueryInformationDriver(
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
 * Gets the object name of a driver.
 *
 * \param DriverHandle A handle to a driver.
 * \param Name A variable which receives a pointer to a string containing the object name. You must
 * free the string using PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function requires a valid KProcessHacker handle.
 */
NTSTATUS PhGetDriverName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *Name
    )
{
    NTSTATUS status;
    PUNICODE_STRING unicodeString;

    if (!NT_SUCCESS(status = PhpQueryDriverVariableSize(
        DriverHandle,
        DriverNameInformation,
        &unicodeString
        )))
        return status;

    *Name = PhCreateStringFromUnicodeString(unicodeString);
    PhFree(unicodeString);

    return status;
}

/**
 * Gets the service key name of a driver.
 *
 * \param DriverHandle A handle to a driver.
 * \param ServiceKeyName A variable which receives a pointer to a string containing the service key
 * name. You must free the string using PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function requires a valid KProcessHacker handle.
 */
NTSTATUS PhGetDriverServiceKeyName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *ServiceKeyName
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

    *ServiceKeyName = PhCreateStringFromUnicodeString(unicodeString);
    PhFree(unicodeString);

    return status;
}

NTSTATUS PhpUnloadDriver(
    _In_ PPH_STRING ServiceKeyName
    )
{
    static PH_STRINGREF fullServicesKeyName = PH_STRINGREF_INIT(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");

    NTSTATUS status;
    PPH_STRING fullServiceKeyName;
    UNICODE_STRING fullServiceKeyNameUs;
    HANDLE serviceKeyHandle;
    ULONG disposition;

    fullServiceKeyName = PhConcatStringRef2(&fullServicesKeyName, &ServiceKeyName->sr);

    if (!PhStringRefToUnicodeString(&fullServiceKeyName->sr, &fullServiceKeyNameUs))
    {
        PhDereferenceObject(fullServiceKeyName);
        return STATUS_NAME_TOO_LONG;
    }

    if (NT_SUCCESS(status = PhCreateKey(
        &serviceKeyHandle,
        KEY_WRITE | DELETE,
        NULL,
        &fullServiceKeyName->sr,
        0,
        0,
        &disposition
        )))
    {
        if (disposition == REG_CREATED_NEW_KEY)
        {
            static UNICODE_STRING imagePath = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\drivers\\ntfs.sys");

            UNICODE_STRING valueName;
            ULONG dword;

            // Set up the required values.
            dword = 1;
            RtlInitUnicodeString(&valueName, L"ErrorControl");
            NtSetValueKey(serviceKeyHandle, &valueName, 0, REG_DWORD, &dword, sizeof(ULONG));
            RtlInitUnicodeString(&valueName, L"Start");
            NtSetValueKey(serviceKeyHandle, &valueName, 0, REG_DWORD, &dword, sizeof(ULONG));
            RtlInitUnicodeString(&valueName, L"Type");
            NtSetValueKey(serviceKeyHandle, &valueName, 0, REG_DWORD, &dword, sizeof(ULONG));

            // Use a bogus name.
            RtlInitUnicodeString(&valueName, L"ImagePath");
            NtSetValueKey(serviceKeyHandle, &valueName, 0, REG_SZ, imagePath.Buffer, imagePath.Length + sizeof(WCHAR));
        }

        status = NtUnloadDriver(&fullServiceKeyNameUs);

        if (disposition == REG_CREATED_NEW_KEY)
        {
            // We added values, not subkeys, so this function will work correctly.
            NtDeleteKey(serviceKeyHandle);
        }

        NtClose(serviceKeyHandle);
    }

    PhDereferenceObject(fullServiceKeyName);

    return status;
}

/**
 * Unloads a driver.
 *
 * \param BaseAddress The base address of the driver. This parameter can be NULL if a value is
 * specified in \c Name.
 * \param Name The base name of the driver. This parameter can be NULL if a value is specified in
 * \c BaseAddress and KProcessHacker is loaded.
 *
 * \retval STATUS_INVALID_PARAMETER_MIX Both \c BaseAddress and \c Name were null, or \c Name was
 * not specified and KProcessHacker is not loaded.
 * \retval STATUS_OBJECT_NAME_NOT_FOUND The driver could not be found.
 */
NTSTATUS PhUnloadDriver(
    _In_opt_ PVOID BaseAddress,
    _In_opt_ PWSTR Name
    )
{
    NTSTATUS status;
    HANDLE driverHandle;
    PPH_STRING serviceKeyName = NULL;

    if (!BaseAddress && !Name)
        return STATUS_INVALID_PARAMETER_MIX;
    if (!Name && !KphIsConnected())
        return STATUS_INVALID_PARAMETER_MIX;

    // Try to get the service key name by scanning the Driver directory.

    if (KphIsConnected() && BaseAddress)
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

    // Use the base name if we didn't get the service key name.

    if (!serviceKeyName && Name)
    {
        PPH_STRING name;

        name = PhCreateString(Name);

        // Remove the extension if it is present.
        if (PhEndsWithString2(name, L".sys", TRUE))
        {
            serviceKeyName = PhSubstring(name, 0, name->Length / sizeof(WCHAR) - 4);
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

NTSTATUS PhpEnumProcessModules(
    _In_ HANDLE ProcessHandle,
    _In_ PPHP_ENUM_PROCESS_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION basicInfo;
    PPEB_LDR_DATA ldr;
    PEB_LDR_DATA pebLdrData;
    PLIST_ENTRY startLink;
    PLIST_ENTRY currentLink;
    ULONG dataTableEntrySize;
    LDR_DATA_TABLE_ENTRY currentEntry;
    ULONG i;

    // Get the PEB address.
    status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        return status;

    // Read the address of the loader data.
    status = NtReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, Ldr)),
        &ldr,
        sizeof(PVOID),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Read the loader data.
    status = NtReadVirtualMemory(
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

    if (WindowsVersion >= WINDOWS_8)
        dataTableEntrySize = LDR_DATA_TABLE_ENTRY_SIZE_WIN8;
    else
        dataTableEntrySize = LDR_DATA_TABLE_ENTRY_SIZE_WIN7;

    // Traverse the linked list (in load order).

    i = 0;
    startLink = PTR_ADD_OFFSET(ldr, FIELD_OFFSET(PEB_LDR_DATA, InLoadOrderModuleList));
    currentLink = pebLdrData.InLoadOrderModuleList.Flink;

    while (
        currentLink != startLink &&
        i <= PH_ENUM_PROCESS_MODULES_LIMIT
        )
    {
        PVOID addressOfEntry;

        addressOfEntry = CONTAINING_RECORD(currentLink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        status = NtReadVirtualMemory(
            ProcessHandle,
            addressOfEntry,
            &currentEntry,
            dataTableEntrySize,
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
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY Entry,
    _In_ PVOID AddressOfEntry,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    )
{
    NTSTATUS status;
    PPH_ENUM_PROCESS_MODULES_PARAMETERS parameters;
    BOOLEAN cont;
    PPH_STRING mappedFileName;
    PWSTR fullDllNameOriginal;
    PWSTR fullDllNameBuffer;
    PWSTR baseDllNameOriginal;
    PWSTR baseDllNameBuffer;

    parameters = Context1;
    mappedFileName = NULL;

    if (parameters->Flags & PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME)
    {
        PhGetProcessMappedFileName(ProcessHandle, Entry->DllBase, &mappedFileName);
    }

    if (mappedFileName)
    {
        ULONG_PTR indexOfLastBackslash;

        PhStringRefToUnicodeString(&mappedFileName->sr, &Entry->FullDllName);
        indexOfLastBackslash = PhFindLastCharInString(mappedFileName, 0, OBJ_NAME_PATH_SEPARATOR);

        if (indexOfLastBackslash != -1)
        {
            Entry->BaseDllName.Buffer = PTR_ADD_OFFSET(Entry->FullDllName.Buffer, PTR_ADD_OFFSET(indexOfLastBackslash * sizeof(WCHAR), sizeof(WCHAR)));
            Entry->BaseDllName.Length = Entry->FullDllName.Length - (USHORT)indexOfLastBackslash * sizeof(WCHAR) - sizeof(WCHAR);
            Entry->BaseDllName.MaximumLength = Entry->BaseDllName.Length;
        }
        else
        {
            Entry->BaseDllName = Entry->FullDllName;
        }
    }
    else
    {
        // Read the full DLL name string and add a null terminator.

        fullDllNameOriginal = Entry->FullDllName.Buffer;
        fullDllNameBuffer = PhAllocate(Entry->FullDllName.Length + sizeof(WCHAR));
        Entry->FullDllName.Buffer = fullDllNameBuffer;

        if (NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            fullDllNameOriginal,
            fullDllNameBuffer,
            Entry->FullDllName.Length,
            NULL
            )))
        {
            fullDllNameBuffer[Entry->FullDllName.Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
        else
        {
            fullDllNameBuffer[0] = UNICODE_NULL;
            Entry->FullDllName.Length = 0;
        }

        baseDllNameOriginal = Entry->BaseDllName.Buffer;

        // Try to use the buffer we just read in.
        if (
            NT_SUCCESS(status) &&
            (ULONG_PTR)baseDllNameOriginal >= (ULONG_PTR)fullDllNameOriginal &&
            (ULONG_PTR)PTR_ADD_OFFSET(baseDllNameOriginal, Entry->BaseDllName.Length) >= (ULONG_PTR)baseDllNameOriginal &&
            (ULONG_PTR)PTR_ADD_OFFSET(baseDllNameOriginal, Entry->BaseDllName.Length) <= (ULONG_PTR)PTR_ADD_OFFSET(fullDllNameOriginal, Entry->FullDllName.Length)
            )
        {
            baseDllNameBuffer = NULL;

            Entry->BaseDllName.Buffer = PTR_ADD_OFFSET(Entry->FullDllName.Buffer, PTR_SUB_OFFSET(baseDllNameOriginal, fullDllNameOriginal));
        }
        else
        {
            // Read the base DLL name string and add a null terminator.

            baseDllNameBuffer = PhAllocate(Entry->BaseDllName.Length + sizeof(WCHAR));
            Entry->BaseDllName.Buffer = baseDllNameBuffer;

            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                baseDllNameOriginal,
                baseDllNameBuffer,
                Entry->BaseDllName.Length,
                NULL
                )))
            {
                baseDllNameBuffer[Entry->BaseDllName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            }
            else
            {
                baseDllNameBuffer[0] = UNICODE_NULL;
                Entry->BaseDllName.Length = 0;
            }
        }
    }

    if (WindowsVersion >= WINDOWS_8)
    {
        LDR_DDAG_NODE ldrDagNode;

        memset(&ldrDagNode, 0, sizeof(LDR_DDAG_NODE));

        if (NT_SUCCESS(NtReadVirtualMemory(
            ProcessHandle,
            Entry->DdagNode,
            &ldrDagNode,
            sizeof(LDR_DDAG_NODE),
            NULL
            )))
        {
            // HACK: Fixup the module load count.
            // Temp fix until PhpModuleQueryWorker can be used for 'Stage2'. 
            Entry->ObsoleteLoadCount = (USHORT)ldrDagNode.LoadCount;
        }
    }

    // Execute the callback.
    cont = parameters->Callback(Entry, parameters->Context);

    if (mappedFileName)
    {
        PhDereferenceObject(mappedFileName);
    }
    else
    {
        PhFree(fullDllNameBuffer);

        if (baseDllNameBuffer)
            PhFree(baseDllNameBuffer);
    }

    return cont;
}

/**
 * Enumerates the modules loaded by a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param Callback A callback function which is executed for each process module.
 * \param Context A user-defined value to pass to the callback function.
 */
NTSTATUS PhEnumProcessModules(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PH_ENUM_PROCESS_MODULES_PARAMETERS parameters;

    parameters.Callback = Callback;
    parameters.Context = Context;
    parameters.Flags = 0;

    return PhEnumProcessModulesEx(ProcessHandle, &parameters);
}

/**
 * Enumerates the modules loaded by a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access. If
 * \c PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME is specified in \a Parameters, the handle should
 * have PROCESS_QUERY_INFORMATION access.
 * \param Parameters The enumeration parameters.
 */
NTSTATUS PhEnumProcessModulesEx(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_PARAMETERS Parameters
    )
{
    return PhpEnumProcessModules(
        ProcessHandle,
        PhpEnumProcessModulesCallback,
        Parameters,
        NULL
        );
}

typedef struct _SET_PROCESS_MODULE_LOAD_COUNT_CONTEXT
{
    NTSTATUS Status;
    PVOID BaseAddress;
    ULONG LoadCount;
} SET_PROCESS_MODULE_LOAD_COUNT_CONTEXT, *PSET_PROCESS_MODULE_LOAD_COUNT_CONTEXT;

BOOLEAN NTAPI PhpSetProcessModuleLoadCountCallback(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY Entry,
    _In_ PVOID AddressOfEntry,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    )
{
    PSET_PROCESS_MODULE_LOAD_COUNT_CONTEXT context = Context1;

    if (Entry->DllBase == context->BaseAddress)
    {
        context->Status = NtWriteVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(AddressOfEntry, FIELD_OFFSET(LDR_DATA_TABLE_ENTRY, ObsoleteLoadCount)),
            &context->LoadCount,
            sizeof(USHORT),
            NULL
            );

        return FALSE;
    }

    return TRUE;
}

/**
 * Sets the load count of a process module.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_READ and PROCESS_VM_WRITE access.
 * \param BaseAddress The base address of a module.
 * \param LoadCount The new load count of the module.
 *
 * \retval STATUS_DLL_NOT_FOUND The module was not found.
 */
NTSTATUS PhSetProcessModuleLoadCount(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG LoadCount
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

NTSTATUS PhpEnumProcessModules32(
    _In_ HANDLE ProcessHandle,
    _In_ PPHP_ENUM_PROCESS_MODULES32_CALLBACK Callback,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    )
{
    NTSTATUS status;
    PPEB32 peb;
    ULONG ldr; // PEB_LDR_DATA32 *32
    PEB_LDR_DATA32 pebLdrData;
    ULONG startLink; // LIST_ENTRY32 *32
    ULONG currentLink; // LIST_ENTRY32 *32
    ULONG dataTableEntrySize;
    LDR_DATA_TABLE_ENTRY32 currentEntry;
    ULONG i;

    // Get the 32-bit PEB address.
    status = PhGetProcessPeb32(ProcessHandle, &peb);

    if (!NT_SUCCESS(status))
        return status;

    if (!peb)
        return STATUS_NOT_SUPPORTED; // not a WOW64 process

    // Read the address of the loader data.
    status = NtReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(peb, FIELD_OFFSET(PEB32, Ldr)),
        &ldr,
        sizeof(ULONG),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Read the loader data.
    status = NtReadVirtualMemory(
        ProcessHandle,
        UlongToPtr(ldr),
        &pebLdrData,
        sizeof(PEB_LDR_DATA32),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!pebLdrData.Initialized)
        return STATUS_UNSUCCESSFUL;

    if (WindowsVersion >= WINDOWS_8)
        dataTableEntrySize = LDR_DATA_TABLE_ENTRY_SIZE_WIN8_32;
    else
        dataTableEntrySize = LDR_DATA_TABLE_ENTRY_SIZE_WIN7_32;

    // Traverse the linked list (in load order).

    i = 0;
    startLink = (ULONG)(ldr + FIELD_OFFSET(PEB_LDR_DATA32, InLoadOrderModuleList));
    currentLink = pebLdrData.InLoadOrderModuleList.Flink;

    while (
        currentLink != startLink &&
        i <= PH_ENUM_PROCESS_MODULES_LIMIT
        )
    {
        ULONG addressOfEntry;

        addressOfEntry = PtrToUlong(CONTAINING_RECORD(UlongToPtr(currentLink), LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks));
        status = NtReadVirtualMemory(
            ProcessHandle,
            UlongToPtr(addressOfEntry),
            &currentEntry,
            dataTableEntrySize,
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

BOOLEAN NTAPI PhpEnumProcessModules32Callback(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY32 Entry,
    _In_ ULONG AddressOfEntry,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    )
{
    static PH_STRINGREF system32String = PH_STRINGREF_INIT(L"\\system32\\");

    PPH_ENUM_PROCESS_MODULES_PARAMETERS parameters;
    BOOLEAN cont;
    LDR_DATA_TABLE_ENTRY nativeEntry;
    PPH_STRING mappedFileName;
    PWSTR baseDllNameBuffer;
    PWSTR fullDllNameBuffer;
    PH_STRINGREF fullDllName;
    PH_STRINGREF systemRootString;

    parameters = Context1;

    // Convert the 32-bit entry to a native-sized entry.

    memset(&nativeEntry, 0, sizeof(LDR_DATA_TABLE_ENTRY));
    nativeEntry.DllBase = UlongToPtr(Entry->DllBase);
    nativeEntry.EntryPoint = UlongToPtr(Entry->EntryPoint);
    nativeEntry.SizeOfImage = Entry->SizeOfImage;
    UStr32ToUStr(&nativeEntry.FullDllName, &Entry->FullDllName);
    UStr32ToUStr(&nativeEntry.BaseDllName, &Entry->BaseDllName);
    nativeEntry.Flags = Entry->Flags;
    nativeEntry.ObsoleteLoadCount = Entry->ObsoleteLoadCount;
    nativeEntry.TlsIndex = Entry->TlsIndex;
    nativeEntry.TimeDateStamp = Entry->TimeDateStamp;
    nativeEntry.OriginalBase = Entry->OriginalBase;
    nativeEntry.LoadTime = Entry->LoadTime;
    nativeEntry.BaseNameHashValue = Entry->BaseNameHashValue;
    nativeEntry.LoadReason = Entry->LoadReason;
    nativeEntry.ParentDllBase = UlongToPtr(Entry->ParentDllBase);

    mappedFileName = NULL;

    if (parameters->Flags & PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME)
    {
        PhGetProcessMappedFileName(ProcessHandle, nativeEntry.DllBase, &mappedFileName);
    }

    if (mappedFileName)
    {
        ULONG_PTR indexOfLastBackslash;

        PhStringRefToUnicodeString(&mappedFileName->sr, &nativeEntry.FullDllName);
        indexOfLastBackslash = PhFindLastCharInString(mappedFileName, 0, OBJ_NAME_PATH_SEPARATOR);

        if (indexOfLastBackslash != -1)
        {
            nativeEntry.BaseDllName.Buffer = PTR_ADD_OFFSET(nativeEntry.FullDllName.Buffer, PTR_ADD_OFFSET(indexOfLastBackslash * sizeof(WCHAR), sizeof(WCHAR)));
            nativeEntry.BaseDllName.Length = nativeEntry.FullDllName.Length - (USHORT)indexOfLastBackslash * sizeof(WCHAR) - sizeof(WCHAR);
            nativeEntry.BaseDllName.MaximumLength = nativeEntry.BaseDllName.Length;
        }
        else
        {
            nativeEntry.BaseDllName = nativeEntry.FullDllName;
        }
    }
    else
    {
        // Read the base DLL name string and add a null terminator.

        baseDllNameBuffer = PhAllocate(nativeEntry.BaseDllName.Length + sizeof(WCHAR));

        if (NT_SUCCESS(NtReadVirtualMemory(
            ProcessHandle,
            nativeEntry.BaseDllName.Buffer,
            baseDllNameBuffer,
            nativeEntry.BaseDllName.Length,
            NULL
            )))
        {
            baseDllNameBuffer[nativeEntry.BaseDllName.Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
        else
        {
            baseDllNameBuffer[0] = UNICODE_NULL;
            nativeEntry.BaseDllName.Length = 0;
        }

        nativeEntry.BaseDllName.Buffer = baseDllNameBuffer;

        // Read the full DLL name string and add a null terminator.

        fullDllNameBuffer = PhAllocate(nativeEntry.FullDllName.Length + sizeof(WCHAR));

        if (NT_SUCCESS(NtReadVirtualMemory(
            ProcessHandle,
            nativeEntry.FullDllName.Buffer,
            fullDllNameBuffer,
            nativeEntry.FullDllName.Length,
            NULL
            )))
        {
            fullDllNameBuffer[nativeEntry.FullDllName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            if (!(parameters->Flags & PH_ENUM_PROCESS_MODULES_DONT_RESOLVE_WOW64_FS))
            {
                // WOW64 file system redirection - convert "system32" to "SysWOW64".
                if (!(nativeEntry.FullDllName.Length & 1)) // validate the string length
                {
                    fullDllName.Buffer = fullDllNameBuffer;
                    fullDllName.Length = nativeEntry.FullDllName.Length;

                    PhGetSystemRoot(&systemRootString);

                    if (PhStartsWithStringRef(&fullDllName, &systemRootString, TRUE))
                    {
                        PhSkipStringRef(&fullDllName, systemRootString.Length);

                        if (PhStartsWithStringRef(&fullDllName, &system32String, TRUE))
                        {
                            fullDllName.Buffer[1] = 'S';
                            fullDllName.Buffer[4] = 'W';
                            fullDllName.Buffer[5] = 'O';
                            fullDllName.Buffer[6] = 'W';
                            fullDllName.Buffer[7] = '6';
                            fullDllName.Buffer[8] = '4';
                        }
                    }
                }
            }
        }
        else
        {
            fullDllNameBuffer[0] = 0;
            nativeEntry.FullDllName.Length = 0;
        }

        nativeEntry.FullDllName.Buffer = fullDllNameBuffer;
    }

    if (WindowsVersion >= WINDOWS_8)
    {
        LDR_DDAG_NODE32 ldrDagNode32 = { 0 };

        if (NT_SUCCESS(NtReadVirtualMemory(
            ProcessHandle,
            UlongToPtr(Entry->DdagNode),
            &ldrDagNode32,
            sizeof(LDR_DDAG_NODE32),
            NULL
            )))
        {
            // HACK: Fixup the module load count.
            // Temp fix until PhpModuleQueryWorker can be used for 'Stage2'. 
            nativeEntry.ObsoleteLoadCount = (USHORT)ldrDagNode32.LoadCount;
        }
    }

    // Execute the callback.
    cont = parameters->Callback(&nativeEntry, parameters->Context);

    if (mappedFileName)
    {
        PhDereferenceObject(mappedFileName);
    }
    else
    {
        PhFree(baseDllNameBuffer);
        PhFree(fullDllNameBuffer);
    }

    return cont;
}

/**
 * Enumerates the 32-bit modules loaded by a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param Callback A callback function which is executed for each process module.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \retval STATUS_NOT_SUPPORTED The process is not running under WOW64.
 *
 * \remarks Do not use this function under a 32-bit environment.
 */
NTSTATUS PhEnumProcessModules32(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PH_ENUM_PROCESS_MODULES_PARAMETERS parameters;

    parameters.Callback = Callback;
    parameters.Context = Context;
    parameters.Flags = 0;

    return PhEnumProcessModules32Ex(ProcessHandle, &parameters);
}

/**
 * Enumerates the 32-bit modules loaded by a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access. If
 * \c PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME is specified in \a Parameters, the handle should
 * have PROCESS_QUERY_INFORMATION access.
 * \param Parameters The enumeration parameters.
 *
 * \retval STATUS_NOT_SUPPORTED The process is not running under WOW64.
 *
 * \remarks Do not use this function under a 32-bit environment.
 */
NTSTATUS PhEnumProcessModules32Ex(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_PARAMETERS Parameters
    )
{
    return PhpEnumProcessModules32(
        ProcessHandle,
        PhpEnumProcessModules32Callback,
        Parameters,
        NULL
        );
}

BOOLEAN NTAPI PhpSetProcessModuleLoadCount32Callback(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY32 Entry,
    _In_ ULONG AddressOfEntry,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    )
{
    PSET_PROCESS_MODULE_LOAD_COUNT_CONTEXT context = Context1;

    if (UlongToPtr(Entry->DllBase) == context->BaseAddress)
    {
        context->Status = NtWriteVirtualMemory(
            ProcessHandle,
            UlongToPtr(AddressOfEntry + FIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, ObsoleteLoadCount)),
            &context->LoadCount,
            sizeof(USHORT),
            NULL
            );

        return FALSE;
    }

    return TRUE;
}

/**
 * Sets the load count of a 32-bit process module.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_READ and PROCESS_VM_WRITE access.
 * \param BaseAddress The base address of a module.
 * \param LoadCount The new load count of the module.
 *
 * \retval STATUS_DLL_NOT_FOUND The module was not found.
 * \retval STATUS_NOT_SUPPORTED The process is not running under WOW64.
 *
 * \remarks Do not use this function under a 32-bit environment.
 */
NTSTATUS PhSetProcessModuleLoadCount32(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG LoadCount
    )
{
    NTSTATUS status;
    SET_PROCESS_MODULE_LOAD_COUNT_CONTEXT context;

    context.Status = STATUS_DLL_NOT_FOUND;
    context.BaseAddress = BaseAddress;
    context.LoadCount = LoadCount;

    status = PhpEnumProcessModules32(
        ProcessHandle,
        PhpSetProcessModuleLoadCount32Callback,
        &context,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    return context.Status;
}

typedef struct _GET_PROCEDURE_ADDRESS_REMOTE_CONTEXT
{
    PH_STRINGREF FileName;
    PVOID DllBase;
} GET_PROCEDURE_ADDRESS_REMOTE_CONTEXT, *PGET_PROCEDURE_ADDRESS_REMOTE_CONTEXT;

static BOOLEAN PhpGetProcedureAddressRemoteCallback(
    _In_ PLDR_DATA_TABLE_ENTRY Module,
    _In_opt_ PVOID Context
    )
{
    PGET_PROCEDURE_ADDRESS_REMOTE_CONTEXT context = Context;
    PH_STRINGREF fullDllName;

    PhUnicodeStringToStringRef(&Module->FullDllName, &fullDllName);

    if (PhEqualStringRef(&fullDllName, &context->FileName, TRUE))
    {
        context->DllBase = Module->DllBase;
        return FALSE;
    }

    return TRUE;
}

/**
 * Gets the address of a procedure in a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param FileName The file name of the DLL containing the procedure.
 * \param ProcedureName The name of the procedure.
 * \param ProcedureNumber The ordinal of the procedure.
 * \param ProcedureAddress A variable which receives the address of the procedure in the address
 * space of the process.
 * \param DllBase A variable which receives the base address of the DLL containing the procedure.
 */
NTSTATUS PhGetProcedureAddressRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PWSTR FileName,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID *ProcedureAddress,
    _Out_opt_ PVOID *DllBase
    )
{
    NTSTATUS status;
    PH_MAPPED_IMAGE mappedImage;
    PH_MAPPED_IMAGE_EXPORTS exports;
    GET_PROCEDURE_ADDRESS_REMOTE_CONTEXT context;

    if (!NT_SUCCESS(status = PhLoadMappedImage(FileName, NULL, TRUE, &mappedImage)))
        return status;

    PhInitializeStringRef(&context.FileName, FileName);
    context.DllBase = NULL;

    if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
#ifdef _WIN64
        status = PhEnumProcessModules32(ProcessHandle, PhpGetProcedureAddressRemoteCallback, &context);
#else
        status = PhEnumProcessModules(ProcessHandle, PhpGetProcedureAddressRemoteCallback, &context);
#endif
    }
    else
    {
#ifdef _WIN64
        status = PhEnumProcessModules(ProcessHandle, PhpGetProcedureAddressRemoteCallback, &context);
#else
        status = STATUS_NOT_SUPPORTED;
#endif
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhGetMappedImageExports(&exports, &mappedImage)))
        goto CleanupExit;

    status = PhGetMappedImageExportFunctionRemote(
        &exports,
        ProcedureName,
        (USHORT)ProcedureNumber,
        context.DllBase,
        ProcedureAddress
        );

    if (NT_SUCCESS(status))
    {
        if (DllBase)
            *DllBase = context.DllBase;
    }

CleanupExit:
    PhUnloadMappedImage(&mappedImage);

    return status;
}

/**
 * Enumerates the modules loaded by the kernel.
 *
 * \param Modules A variable which receives a pointer to a structure containing information about
 * the kernel modules. You must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhEnumKernelModules(
    _Out_ PRTL_PROCESS_MODULES *Modules
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
 * Enumerates the modules loaded by the kernel.
 *
 * \param Modules A variable which receives a pointer to a structure containing information about
 * the kernel modules. You must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhEnumKernelModulesEx(
    _Out_ PRTL_PROCESS_MODULE_INFORMATION_EX *Modules
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 2048;

    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemModuleInformationEx,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemModuleInformationEx,
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
 * \return A pointer to a string containing the kernel image file name. You must free the string
 * using PhDereferenceObject() when you no longer need it.
 */
PPH_STRING PhGetKernelFileName(
    VOID
    )
{
    PRTL_PROCESS_MODULES modules;
    PPH_STRING fileName = NULL;

    if (!NT_SUCCESS(PhEnumKernelModules(&modules)))
        return NULL;

    if (modules->NumberOfModules >= 1)
    {
        fileName = PhConvertMultiByteToUtf16(modules->Modules[0].FullPathName);
    }

    PhFree(modules);

    return fileName;
}

/**
 * Enumerates the running processes.
 *
 * \param Processes A variable which receives a pointer to a buffer containing process information.
 * You must free the buffer using PhFree() when you no longer need it.
 *
 * \remarks You can use the \ref PH_FIRST_PROCESS and \ref PH_NEXT_PROCESS macros to process the
 * information contained in the buffer.
 */
NTSTATUS PhEnumProcesses(
    _Out_ PVOID *Processes
    )
{
    return PhEnumProcessesEx(Processes, SystemProcessInformation);
}

/**
 * Enumerates the running processes.
 *
 * \param Processes A variable which receives a pointer to a buffer containing process information.
 * You must free the buffer using PhFree() when you no longer need it.
 *
 * \remarks You can use the \ref PH_FIRST_PROCESS and \ref PH_NEXT_PROCESS macros to process the
 * information contained in the buffer.
 */
NTSTATUS PhEnumProcessesEx(
    _Out_ PVOID *Processes,
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass
    )
{
    static ULONG initialBufferSize[3] = { 0x4000, 0x4000, 0x4000 };
    NTSTATUS status;
    ULONG classIndex;
    PVOID buffer;
    ULONG bufferSize;

    switch (SystemInformationClass)
    {
    case SystemProcessInformation:
        classIndex = 0;
        break;
    case SystemExtendedProcessInformation:
        classIndex = 1;
        break;
    case SystemFullProcessInformation:
        classIndex = 2;
        break;
    default:
        return STATUS_INVALID_INFO_CLASS;
    }

    bufferSize = initialBufferSize[classIndex];
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQuerySystemInformation(
            SystemInformationClass,
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

    if (bufferSize <= 0x100000) initialBufferSize[classIndex] = bufferSize;
    *Processes = buffer;

    return status;
}

/**
 * Enumerates the running processes for a session.
 *
 * \param Processes A variable which receives a pointer to a buffer containing process information.
 * You must free the buffer using PhFree() when you no longer need it.
 * \param SessionId A session ID.
 *
 * \remarks You can use the \ref PH_FIRST_PROCESS and \ref PH_NEXT_PROCESS macros to process the
 * information contained in the buffer.
 */
NTSTATUS PhEnumProcessesForSession(
    _Out_ PVOID *Processes,
    _In_ ULONG SessionId
    )
{
    static ULONG initialBufferSize = 0x4000;
    NTSTATUS status;
    SYSTEM_SESSION_PROCESS_INFORMATION sessionProcessInfo;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    sessionProcessInfo.SessionId = SessionId;

    while (TRUE)
    {
        sessionProcessInfo.SizeOfBuf = bufferSize;
        sessionProcessInfo.Buffer = buffer;

        status = NtQuerySystemInformation(
            SystemSessionProcessInformation,
            &sessionProcessInfo,
            sizeof(SYSTEM_SESSION_PROCESS_INFORMATION),
            &bufferSize // size of the inner buffer gets returned
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

    if (bufferSize <= 0x100000) initialBufferSize = bufferSize;
    *Processes = buffer;

    return status;
}

/**
 * Finds the process information structure for a specific process.
 *
 * \param Processes A pointer to a buffer returned by PhEnumProcesses().
 * \param ProcessId The ID of the process.
 *
 * \return A pointer to the process information structure for the specified process, or NULL if the
 * structure could not be found.
 */
PSYSTEM_PROCESS_INFORMATION PhFindProcessInformation(
    _In_ PVOID Processes,
    _In_ HANDLE ProcessId
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
 * Finds the process information structure for a specific process.
 *
 * \param Processes A pointer to a buffer returned by PhEnumProcesses().
 * \param ImageName The image name to search for.
 *
 * \return A pointer to the process information structure for the specified process, or NULL if the
 * structure could not be found.
 */
PSYSTEM_PROCESS_INFORMATION PhFindProcessInformationByImageName(
    _In_ PVOID Processes,
    _In_ PPH_STRINGREF ImageName
    )
{
    PSYSTEM_PROCESS_INFORMATION process;
    PH_STRINGREF processImageName;

    process = PH_FIRST_PROCESS(Processes);

    do
    {
        PhUnicodeStringToStringRef(&process->ImageName, &processImageName);

        if (PhEqualStringRef(&processImageName, ImageName, TRUE))
            return process;
    } while (process = PH_NEXT_PROCESS(process));

    return NULL;
}

/**
 * Enumerates all open handles.
 *
 * \param Handles A variable which receives a pointer to a structure containing information about
 * all opened handles. You must free the structure using PhFree() when you no longer need it.
 *
 * \retval STATUS_INSUFFICIENT_RESOURCES The handle information returned by the kernel is too large.
 */
NTSTATUS PhEnumHandles(
    _Out_ PSYSTEM_HANDLE_INFORMATION *Handles
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

        // Fail if we're resizing the buffer to something very large.
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
 * \param Handles A variable which receives a pointer to a structure containing information about
 * all opened handles. You must free the structure using PhFree() when you no longer need it.
 *
 * \retval STATUS_INSUFFICIENT_RESOURCES The handle information returned by the kernel is too large.
 *
 * \remarks This function is only available starting with Windows XP.
 */
NTSTATUS PhEnumHandlesEx(
    _Out_ PSYSTEM_HANDLE_INFORMATION_EX *Handles
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

        // Fail if we're resizing the buffer to something very large.
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

/**
 * Enumerates all open handles.
 *
 * \param ProcessHandle A handle to the process. The handle must have PROCESS_QUERY_INFORMATION access.
 * \param Handles A variable which receives a pointer to a structure containing information about
 * handles opened by the process. You must free the structure using PhFree() when you no longer need it.
 *
 * \retval STATUS_INSUFFICIENT_RESOURCES The handle information returned by the kernel is too large.
 *
 * \remarks This function is only available starting with Windows 8.
 */
NTSTATUS PhEnumHandlesEx2(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_HANDLE_SNAPSHOT_INFORMATION *Handles
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength = 0;
    ULONG attempts = 0;

    bufferSize = 0x8000;
    buffer = PhAllocate(bufferSize);
    memset(buffer, 0, bufferSize);

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessHandleInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);
        memset(buffer, 0, bufferSize);

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessHandleInformation,
            buffer,
            bufferSize,
            &returnLength
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Handles = buffer;
    else
        PhFree(buffer);

    return status;
}

/**
 * Enumerates all pagefiles.
 *
 * \param Pagefiles A variable which receives a pointer to a buffer containing information about all
 * active pagefiles. You must free the structure using PhFree() when you no longer need it.
 *
 * \retval STATUS_INSUFFICIENT_RESOURCES The handle information returned by the kernel is too large.
 */
NTSTATUS PhEnumPagefiles(
    _Out_ PVOID *Pagefiles
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

        // Fail if we're resizing the buffer to something very large.
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
 * \param FileName A variable which receives a pointer to a string containing the file name. You
 * must free the string using PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function only works on Windows Vista and above. There does not appear to be any
 * access checking performed by the kernel for this.
 */
NTSTATUS PhGetProcessImageFileNameByProcessId(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;
    SYSTEM_PROCESS_ID_INFORMATION processIdInfo;

    buffer = PhAllocate(bufferSize);

    processIdInfo.ProcessId = ProcessId;
    processIdInfo.ImageName.Length = 0;
    processIdInfo.ImageName.MaximumLength = (USHORT)bufferSize;
    processIdInfo.ImageName.Buffer = buffer;

    status = NtQuerySystemInformation(
        SystemProcessIdInformation,
        &processIdInfo,
        sizeof(SYSTEM_PROCESS_ID_INFORMATION),
        NULL
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        // Required length is stored in MaximumLength.

        PhFree(buffer);
        buffer = PhAllocate(processIdInfo.ImageName.MaximumLength);
        processIdInfo.ImageName.Buffer = buffer;

        status = NtQuerySystemInformation(
            SystemProcessIdInformation,
            &processIdInfo,
            sizeof(SYSTEM_PROCESS_ID_INFORMATION),
            NULL
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *FileName = PhCreateStringFromUnicodeString(&processIdInfo.ImageName);
    PhFree(buffer);

    return status;
}

/**
 * Determines if a process is managed.
 *
 * \param ProcessId The ID of the process.
 * \param IsDotNet A variable which receives a boolean indicating whether the process is managed.
 */
NTSTATUS PhGetProcessIsDotNet(
    _In_ HANDLE ProcessId,
    _Out_ PBOOLEAN IsDotNet
    )
{
    return PhGetProcessIsDotNetEx(ProcessId, NULL, 0, IsDotNet, NULL);
}

BOOLEAN NTAPI PhpIsDotNetEnumProcessModulesCallback(
    _In_ PLDR_DATA_TABLE_ENTRY Module,
    _In_opt_ PVOID Context
    )
{
    static UNICODE_STRING clrString = RTL_CONSTANT_STRING(L"clr.dll");
    static UNICODE_STRING mscorwksString = RTL_CONSTANT_STRING(L"mscorwks.dll");
    static UNICODE_STRING mscorsvrString = RTL_CONSTANT_STRING(L"mscorsvr.dll");
    static UNICODE_STRING mscorlibString = RTL_CONSTANT_STRING(L"mscorlib.dll");
    static UNICODE_STRING mscorlibNiString = RTL_CONSTANT_STRING(L"mscorlib.ni.dll");
    static UNICODE_STRING clrjitString = RTL_CONSTANT_STRING(L"clrjit.dll");
    static UNICODE_STRING frameworkString = RTL_CONSTANT_STRING(L"\\Microsoft.NET\\Framework\\");
    static UNICODE_STRING framework64String = RTL_CONSTANT_STRING(L"\\Microsoft.NET\\Framework64\\");

    if (
        RtlEqualUnicodeString(&Module->BaseDllName, &clrString, TRUE) ||
        RtlEqualUnicodeString(&Module->BaseDllName, &mscorwksString, TRUE) ||
        RtlEqualUnicodeString(&Module->BaseDllName, &mscorsvrString, TRUE)
        )
    {
        UNICODE_STRING fileName;
        PH_STRINGREF systemRootSr;
        UNICODE_STRING systemRoot;
        PUNICODE_STRING frameworkPart;

#ifdef _WIN64
        if (*(PULONG)Context & PH_CLR_PROCESS_IS_WOW64)
        {
#endif
            frameworkPart = &frameworkString;
#ifdef _WIN64
        }
        else
        {
            frameworkPart = &framework64String;
        }
#endif

        fileName = Module->FullDllName;
        PhGetSystemRoot(&systemRootSr);
        PhStringRefToUnicodeString(&systemRootSr, &systemRoot);

        if (RtlPrefixUnicodeString(&systemRoot, &fileName, TRUE))
        {
            fileName.Buffer = (PWCHAR)PTR_ADD_OFFSET(fileName.Buffer, systemRoot.Length);
            fileName.Length -= systemRoot.Length;

            if (RtlPrefixUnicodeString(frameworkPart, &fileName, TRUE))
            {
                fileName.Buffer = (PWCHAR)PTR_ADD_OFFSET(fileName.Buffer, frameworkPart->Length);
                fileName.Length -= frameworkPart->Length;

                if (fileName.Length >= 4 * sizeof(WCHAR)) // vx.x
                {
                    if (fileName.Buffer[1] == '1')
                    {
                        if (fileName.Buffer[3] == '0')
                            *(PULONG)Context |= PH_CLR_VERSION_1_0;
                        else if (fileName.Buffer[3] == '1')
                            *(PULONG)Context |= PH_CLR_VERSION_1_1;
                    }
                    else if (fileName.Buffer[1] == '2')
                    {
                        *(PULONG)Context |= PH_CLR_VERSION_2_0;
                    }
                    else if (fileName.Buffer[1] >= '4' && fileName.Buffer[1] <= '9')
                    {
                        *(PULONG)Context |= PH_CLR_VERSION_4_ABOVE;
                    }
                }
            }
        }
    }
    else if (
        RtlEqualUnicodeString(&Module->BaseDllName, &mscorlibString, TRUE) ||
        RtlEqualUnicodeString(&Module->BaseDllName, &mscorlibNiString, TRUE)
        )
    {
        *(PULONG)Context |= PH_CLR_MSCORLIB_PRESENT;
    }
    else if (RtlEqualUnicodeString(&Module->BaseDllName, &clrjitString, TRUE))
    {
        *(PULONG)Context |= PH_CLR_JIT_PRESENT;
    }

    return TRUE;
}

/**
 * Determines if a process is managed.
 *
 * \param ProcessId The ID of the process.
 * \param ProcessHandle An optional handle to the process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param InFlags A combination of flags.
 * \li \c PH_CLR_USE_SECTION_CHECK Checks for the existence of related section objects to determine
 * whether the process is managed.
 * \li \c PH_CLR_NO_WOW64_CHECK Instead of a separate query, uses the presence of the
 * \c PH_CLR_KNOWN_IS_WOW64 flag to determine whether the process is running under WOW64.
 * \li \c PH_CLR_KNOWN_IS_WOW64 When \c PH_CLR_NO_WOW64_CHECK is specified, indicates that the
 * process is managed.
 * \param IsDotNet A variable which receives a boolean indicating whether the process is managed.
 * \param Flags A variable which receives additional flags.
 */
NTSTATUS PhGetProcessIsDotNetEx(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ ULONG InFlags,
    _Out_opt_ PBOOLEAN IsDotNet,
    _Out_opt_ PULONG Flags
    )
{
    if (InFlags & PH_CLR_USE_SECTION_CHECK)
    {
        NTSTATUS status;
        HANDLE sectionHandle;
        OBJECT_ATTRIBUTES objectAttributes;
        PPH_STRING sectionName;
        UNICODE_STRING sectionNameUs;
        PH_FORMAT format[2];

        // Most .NET processes have a handle open to a section named
        // \BaseNamedObjects\Cor_Private_IPCBlock(_v4)_<ProcessId>. This is the same object used by
        // the ICorPublish::GetProcess function. Instead of calling that function, we simply check
        // for the existence of that section object. This means:
        // * Better performance.
        // * No need for admin rights to get .NET status of processes owned by other users.

        PhInitFormatIU(&format[1], (ULONG_PTR)ProcessId);

        // Version 4 section object

        PhInitFormatS(&format[0], L"\\BaseNamedObjects\\Cor_Private_IPCBlock_v4_");
        sectionName = PhFormat(format, 2, 96);
        PhStringRefToUnicodeString(&sectionName->sr, &sectionNameUs);

        InitializeObjectAttributes(
            &objectAttributes,
            &sectionNameUs,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
        status = NtOpenSection(
            &sectionHandle,
            SECTION_QUERY,
            &objectAttributes
            );
        PhDereferenceObject(sectionName);

        if (NT_SUCCESS(status) || status == STATUS_ACCESS_DENIED)
        {
            if (NT_SUCCESS(status))
                NtClose(sectionHandle);

            if (IsDotNet)
                *IsDotNet = TRUE;

            if (Flags)
                *Flags = PH_CLR_VERSION_4_ABOVE;

            return STATUS_SUCCESS;
        }

        // Version 2 section object

        PhInitFormatS(&format[0], L"\\BaseNamedObjects\\Cor_Private_IPCBlock_");
        sectionName = PhFormat(format, 2, 90);
        PhStringRefToUnicodeString(&sectionName->sr, &sectionNameUs);

        InitializeObjectAttributes(
            &objectAttributes,
            &sectionNameUs,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
        status = NtOpenSection(
            &sectionHandle,
            SECTION_QUERY,
            &objectAttributes
            );
        PhDereferenceObject(sectionName);

        if (NT_SUCCESS(status) || status == STATUS_ACCESS_DENIED)
        {
            if (NT_SUCCESS(status))
                NtClose(sectionHandle);

            if (IsDotNet)
                *IsDotNet = TRUE;

            if (Flags)
                *Flags = PH_CLR_VERSION_2_0;

            return STATUS_SUCCESS;
        }

        return status;
    }
    else
    {
        NTSTATUS status;
        HANDLE processHandle = NULL;
        ULONG flags = 0;
#ifdef _WIN64
        BOOLEAN isWow64;
#endif

        if (!ProcessHandle)
        {
            if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, ProcessId)))
                return status;

            ProcessHandle = processHandle;
        }

#ifdef _WIN64
        if (InFlags & PH_CLR_NO_WOW64_CHECK)
        {
            isWow64 = !!(InFlags & PH_CLR_KNOWN_IS_WOW64);
        }
        else
        {
            isWow64 = FALSE;
            PhGetProcessIsWow64(ProcessHandle, &isWow64);
        }

        if (isWow64)
        {
            flags |= PH_CLR_PROCESS_IS_WOW64;
            status = PhEnumProcessModules32(ProcessHandle, PhpIsDotNetEnumProcessModulesCallback, &flags);
        }
        else
        {
#endif
            status = PhEnumProcessModules(ProcessHandle, PhpIsDotNetEnumProcessModulesCallback, &flags);
#ifdef _WIN64
        }
#endif

        if (processHandle)
            NtClose(processHandle);

        if (IsDotNet)
            *IsDotNet = (flags & PH_CLR_VERSION_MASK) && (flags & (PH_CLR_MSCORLIB_PRESENT | PH_CLR_JIT_PRESENT));

        if (Flags)
            *Flags = flags;

        return status;
    }
}

/**
 * Enumerates the objects in a directory object.
 *
 * \param DirectoryHandle A handle to a directory. The handle must have DIRECTORY_QUERY access.
 * \param Callback A callback function which is executed for each object.
 * \param Context A user-defined value to pass to the callback function.
 */
NTSTATUS PhEnumDirectoryObjects(
    _In_ HANDLE DirectoryHandle,
    _In_ PPH_ENUM_DIRECTORY_OBJECTS Callback,
    _In_opt_ PVOID Context
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
            // Check if we have at least one entry. If not, we'll double the buffer size and try
            // again.
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

        // Read the batch and execute the callback function for each object.

        i = 0;
        cont = TRUE;

        while (TRUE)
        {
            POBJECT_DIRECTORY_INFORMATION info;
            PH_STRINGREF name;
            PH_STRINGREF typeName;

            info = &buffer[i];

            if (!info->Name.Buffer)
                break;

            PhUnicodeStringToStringRef(&info->Name, &name);
            PhUnicodeStringToStringRef(&info->TypeName, &typeName);

            cont = Callback(&name, &typeName, Context);

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
    _In_ HANDLE FileHandle,
    _In_opt_ PUNICODE_STRING SearchPattern,
    _In_ PPH_ENUM_DIRECTORY_FILE Callback,
    _In_opt_ PVOID Context
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
        // Query the directory, doubling the buffer each time NtQueryDirectoryFile fails.
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
                SearchPattern,
                firstTime
                );

            // Our ISB is on the stack, so we have to wait for the operation to complete before
            // continuing.
            if (status == STATUS_PENDING)
            {
                status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

                if (NT_SUCCESS(status))
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

        // Read the batch and execute the callback function for each file.

        i = 0;
        cont = TRUE;

        while (TRUE)
        {
            PFILE_DIRECTORY_INFORMATION information;

            information = (PFILE_DIRECTORY_INFORMATION)PTR_ADD_OFFSET(buffer, i);

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

NTSTATUS PhEnumFileExtendedAttributes(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_FILE_EA Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    BOOLEAN firstTime = TRUE;
    BOOLEAN success = FALSE;
    IO_STATUS_BLOCK isb;
    ULONG bufferSize;
    PVOID buffer;
    PFILE_FULL_EA_INFORMATION i;

    bufferSize = 0x400;
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        while (TRUE)
        {
            status = NtQueryEaFile(
                FileHandle,
                &isb,
                buffer,
                bufferSize,
                FALSE,
                NULL,
                0,
                NULL,
                firstTime
                );

            if (status == STATUS_PENDING)
            {
                status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

                if (NT_SUCCESS(status))
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

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        success = TRUE;

        for (i = PH_FIRST_FILE_EA(buffer); i; i = PH_NEXT_FILE_EA(i))
        {
            if (!Callback(i, Context))
            {
                success = FALSE;
                break;
            }
        }

        if (!success)
            break;

        firstTime = FALSE;
    }

    PhFree(buffer);

    return status;
}

NTSTATUS PhEnumFileStreams(
    _In_ HANDLE FileHandle,
    _Out_ PVOID *Streams
    )
{
    return PhpQueryFileVariableSize(
        FileHandle,
        FileStreamInformation,
        Streams
        );
}

NTSTATUS PhEnumFileStreamsEx(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_FILE_STREAMS Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID buffer;
    PFILE_STREAM_INFORMATION i;

    status = PhpQueryFileVariableSize(
        FileHandle,
        FileStreamInformation,
        &buffer
        );

    if (NT_SUCCESS(status))
    {
        for (i = PH_FIRST_STREAM(buffer); i; i = PH_NEXT_STREAM(i))
        {
            if (!Callback(i, Context))
                break;
        }

        PhFree(buffer);
    }

    return status;
}

/**
 * Initializes the device prefixes module.
 */
VOID PhpInitializeDevicePrefixes(
    VOID
    )
{
    ULONG i;
    PUCHAR buffer;

    // Allocate one buffer for all 26 prefixes to reduce overhead.
    buffer = PhAllocate(PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR) * 26);

    for (i = 0; i < 26; i++)
    {
        PhDevicePrefixes[i].Length = 0;
        PhDevicePrefixes[i].MaximumLength = PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR);
        PhDevicePrefixes[i].Buffer = (PWCHAR)buffer;
        buffer += PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR);
    }
}

VOID PhUpdateMupDevicePrefixes(
    VOID
    )
{
    static PH_STRINGREF orderKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Control\\NetworkProvider\\Order");
    static PH_STRINGREF servicesStringPart = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");
    static PH_STRINGREF networkProviderStringPart = PH_STRINGREF_INIT(L"\\NetworkProvider");

    HANDLE orderKeyHandle;
    PPH_STRING providerOrder = NULL;
    ULONG i;
    PH_STRINGREF remainingPart;
    PH_STRINGREF part;

    // The provider names are stored in the ProviderOrder value in this key:
    // HKLM\System\CurrentControlSet\Control\NetworkProvider\Order
    // Each name can then be looked up, its device name in the DeviceName value in:
    // HKLM\System\CurrentControlSet\Services\<ProviderName>\NetworkProvider

    // Note that we assume the providers only claim their device name. Some providers such as DFS
    // claim an extra part, and are not resolved correctly here.

    if (NT_SUCCESS(PhOpenKey(
        &orderKeyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &orderKeyName,
        0
        )))
    {
        providerOrder = PhQueryRegistryString(orderKeyHandle, L"ProviderOrder");
        NtClose(orderKeyHandle);
    }

    if (!providerOrder)
        return;

    PhAcquireQueuedLockExclusive(&PhDeviceMupPrefixesLock);

    for (i = 0; i < PhDeviceMupPrefixesCount; i++)
    {
        PhDereferenceObject(PhDeviceMupPrefixes[i]);
        PhDeviceMupPrefixes[i] = NULL;
    }

    PhDeviceMupPrefixesCount = 0;

    PhDeviceMupPrefixes[PhDeviceMupPrefixesCount++] = PhCreateString(L"\\Device\\Mup");

    // DFS claims an extra part of file names, which we don't handle.
    // PhDeviceMupPrefixes[PhDeviceMupPrefixesCount++] = PhCreateString(L"\\Device\\DfsClient");

    remainingPart = providerOrder->sr;

    while (remainingPart.Length != 0)
    {
        PPH_STRING serviceKeyName;
        HANDLE networkProviderKeyHandle;
        PPH_STRING deviceName;

        if (PhDeviceMupPrefixesCount == PH_DEVICE_MUP_PREFIX_MAX_COUNT)
            break;

        PhSplitStringRefAtChar(&remainingPart, ',', &part, &remainingPart);

        if (part.Length != 0)
        {
            serviceKeyName = PhConcatStringRef3(&servicesStringPart, &part, &networkProviderStringPart);

            if (NT_SUCCESS(PhOpenKey(
                &networkProviderKeyHandle,
                KEY_READ,
                PH_KEY_LOCAL_MACHINE,
                &serviceKeyName->sr,
                0
                )))
            {
                if (deviceName = PhQueryRegistryString(networkProviderKeyHandle, L"DeviceName"))
                {
                    PhDeviceMupPrefixes[PhDeviceMupPrefixesCount] = deviceName;
                    PhDeviceMupPrefixesCount++;
                }

                NtClose(networkProviderKeyHandle);
            }

            PhDereferenceObject(serviceKeyName);
        }
    }

    PhReleaseQueuedLockExclusive(&PhDeviceMupPrefixesLock);

    PhDereferenceObject(providerOrder);
}

/**
 * Updates the DOS device names array.
 */
VOID PhUpdateDosDevicePrefixes(
    VOID
    )
{
    WCHAR deviceNameBuffer[7] = L"\\??\\ :";
#ifndef _WIN64
    PROCESS_DEVICEMAP_INFORMATION deviceMapInfo;
#else
    PROCESS_DEVICEMAP_INFORMATION_EX deviceMapInfo;
#endif
    memset(&deviceMapInfo, 0, sizeof(deviceMapInfo));

    NtQueryInformationProcess(
        NtCurrentProcess(), 
        ProcessDeviceMap, 
        &deviceMapInfo, 
        sizeof(deviceMapInfo), 
        NULL
        );

    for (ULONG i = 0; i < 0x1A; i++)
    {
        HANDLE linkHandle;
        OBJECT_ATTRIBUTES oa;
        UNICODE_STRING deviceName;

        if (deviceMapInfo.Query.DriveMap)
        {
            if (!(deviceMapInfo.Query.DriveMap & (0x1 << i)))
                continue;
        }

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
            PhAcquireQueuedLockExclusive(&PhDevicePrefixesLock);

            if (!NT_SUCCESS(NtQuerySymbolicLinkObject(
                linkHandle,
                &PhDevicePrefixes[i],
                NULL
                )))
            {
                PhDevicePrefixes[i].Length = 0;
            }

            PhReleaseQueuedLockExclusive(&PhDevicePrefixesLock);

            NtClose(linkHandle);
        }
        else
        {
            PhDevicePrefixes[i].Length = 0;
        }
    }
}

/**
 * Resolves a NT path into a Win32 path.
 *
 * \param Name A string containing the path to resolve.
 *
 * \return A pointer to a string containing the Win32 path. You must free the string using
 * PhDereferenceObject() when you no longer need it.
 */
PPH_STRING PhResolveDevicePrefix(
    _In_ PPH_STRING Name
    )
{
    ULONG i;
    PPH_STRING newName = NULL;

    if (PhBeginInitOnce(&PhDevicePrefixesInitOnce))
    {
        PhpInitializeDevicePrefixes();
        PhUpdateDosDevicePrefixes();
        PhUpdateMupDevicePrefixes();

        PhEndInitOnce(&PhDevicePrefixesInitOnce);
    }

    // Go through the DOS devices and try to find a matching prefix.
    for (i = 0; i < 26; i++)
    {
        BOOLEAN isPrefix = FALSE;
        PH_STRINGREF prefix;

        PhAcquireQueuedLockShared(&PhDevicePrefixesLock);

        PhUnicodeStringToStringRef(&PhDevicePrefixes[i], &prefix);

        if (prefix.Length != 0)
        {
            if (PhStartsWithStringRef(&Name->sr, &prefix, TRUE))
            {
                // To ensure we match the longest prefix, make sure the next character is a
                // backslash or the path is equal to the prefix.
                if (Name->Length == prefix.Length || Name->Buffer[prefix.Length / sizeof(WCHAR)] == OBJ_NAME_PATH_SEPARATOR)
                {
                    isPrefix = TRUE;
                }
            }
        }

        PhReleaseQueuedLockShared(&PhDevicePrefixesLock);

        if (isPrefix)
        {
            // <letter>:path
            newName = PhCreateStringEx(NULL, 2 * sizeof(WCHAR) + Name->Length - prefix.Length);
            newName->Buffer[0] = (WCHAR)('A' + i);
            newName->Buffer[1] = ':';
            memcpy(
                &newName->Buffer[2],
                &Name->Buffer[prefix.Length / sizeof(WCHAR)],
                Name->Length - prefix.Length
                );

            break;
        }
    }

    if (i == 26)
    {
        // Resolve network providers.

        PhAcquireQueuedLockShared(&PhDeviceMupPrefixesLock);

        for (i = 0; i < PhDeviceMupPrefixesCount; i++)
        {
            BOOLEAN isPrefix = FALSE;
            SIZE_T prefixLength;

            prefixLength = PhDeviceMupPrefixes[i]->Length;

            if (prefixLength != 0)
            {
                if (PhStartsWithString(Name, PhDeviceMupPrefixes[i], TRUE))
                {
                    // To ensure we match the longest prefix, make sure the next character is a
                    // backslash. Don't resolve if the name *is* the prefix. Otherwise, we will end
                    // up with a useless string like "\".
                    if (Name->Length != prefixLength && Name->Buffer[prefixLength / sizeof(WCHAR)] == OBJ_NAME_PATH_SEPARATOR)
                    {
                        isPrefix = TRUE;
                    }
                }
            }

            if (isPrefix)
            {
                // \path
                newName = PhCreateStringEx(NULL, 1 * sizeof(WCHAR) + Name->Length - prefixLength);
                newName->Buffer[0] = OBJ_NAME_PATH_SEPARATOR;
                memcpy(
                    &newName->Buffer[1],
                    &Name->Buffer[prefixLength / sizeof(WCHAR)],
                    Name->Length - prefixLength
                    );

                break;
            }
        }

        PhReleaseQueuedLockShared(&PhDeviceMupPrefixesLock);
    }

    if (newName)
        PhTrimToNullTerminatorString(newName);

    return newName;
}

/**
 * Converts a file name into Win32 format.
 *
 * \param FileName A string containing a file name.
 *
 * \return A pointer to a string containing the Win32 file name. You must free the string using
 * PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function may convert NT object name paths to invalid ones. If the path to be
 * converted is not necessarily a file name, use PhResolveDevicePrefix().
 */
PPH_STRING PhGetFileName(
    _In_ PPH_STRING FileName
    )
{
    PPH_STRING newFileName;

    newFileName = FileName;

    // "\??\" refers to \GLOBAL??\. Just remove it.
    if (PhStartsWithString2(FileName, L"\\??\\", FALSE))
    {
        newFileName = PhCreateStringEx(NULL, FileName->Length - 4 * sizeof(WCHAR));
        memcpy(newFileName->Buffer, &FileName->Buffer[4], FileName->Length - 4 * sizeof(WCHAR));
    }
    // "\SystemRoot" means "C:\Windows".
    else if (PhStartsWithString2(FileName, L"\\SystemRoot", TRUE))
    {
        PH_STRINGREF systemRoot;

        PhGetSystemRoot(&systemRoot);
        newFileName = PhCreateStringEx(NULL, systemRoot.Length + FileName->Length - 11 * sizeof(WCHAR));
        memcpy(newFileName->Buffer, systemRoot.Buffer, systemRoot.Length);
        memcpy(PTR_ADD_OFFSET(newFileName->Buffer, systemRoot.Length), &FileName->Buffer[11], FileName->Length - 11 * sizeof(WCHAR));
    }
    // "system32\" means "C:\Windows\system32\".
    else if (PhStartsWithString2(FileName, L"system32\\", TRUE))
    {
        PH_STRINGREF systemRoot;

        PhGetSystemRoot(&systemRoot);
        newFileName = PhCreateStringEx(NULL, systemRoot.Length + sizeof(WCHAR) + FileName->Length);
        memcpy(newFileName->Buffer, systemRoot.Buffer, systemRoot.Length);
        newFileName->Buffer[systemRoot.Length / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
        memcpy(PTR_ADD_OFFSET(newFileName->Buffer, systemRoot.Length + sizeof(WCHAR)), FileName->Buffer, FileName->Length);
    }
    else if (FileName->Length != 0 && FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
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
            // If the file name starts with "\Windows", prepend the system drive.
            if (PhStartsWithString2(newFileName, L"\\Windows", TRUE))
            {
                newFileName = PhCreateStringEx(NULL, FileName->Length + sizeof(WCHAR) * sizeof(WCHAR));
                newFileName->Buffer[0] = USER_SHARED_DATA->NtSystemRoot[0];
                newFileName->Buffer[1] = ':';
                memcpy(&newFileName->Buffer[2], FileName->Buffer, FileName->Length);
            }
            else
            {
                PhReferenceObject(newFileName);
            }
        }
    }
    else
    {
        // Just return the supplied file name. Note that we need to add a reference.
        PhReferenceObject(newFileName);
    }

    return newFileName;
}

typedef struct _ENUM_GENERIC_PROCESS_MODULES_CONTEXT
{
    PPH_ENUM_GENERIC_MODULES_CALLBACK Callback;
    PVOID Context;
    ULONG Type;
    PPH_HASHTABLE BaseAddressHashtable;

    ULONG LoadOrderIndex;
} ENUM_GENERIC_PROCESS_MODULES_CONTEXT, *PENUM_GENERIC_PROCESS_MODULES_CONTEXT;

static BOOLEAN EnumGenericProcessModulesCallback(
    _In_ PLDR_DATA_TABLE_ENTRY Module,
    _In_opt_ PVOID Context
    )
{
    PENUM_GENERIC_PROCESS_MODULES_CONTEXT context;
    PH_MODULE_INFO moduleInfo;
    BOOLEAN cont;

    context = (PENUM_GENERIC_PROCESS_MODULES_CONTEXT)Context;

    // Check if we have a duplicate base address.
    if (PhFindEntryHashtable(context->BaseAddressHashtable, &Module->DllBase))
    {
        return TRUE;
    }
    else
    {
        PhAddEntryHashtable(context->BaseAddressHashtable, &Module->DllBase);
    }

    moduleInfo.Type = context->Type;
    moduleInfo.BaseAddress = Module->DllBase;
    moduleInfo.Size = Module->SizeOfImage;
    moduleInfo.EntryPoint = Module->EntryPoint;
    moduleInfo.Flags = Module->Flags;
    moduleInfo.LoadOrderIndex = (USHORT)(context->LoadOrderIndex++);
    moduleInfo.LoadCount = Module->ObsoleteLoadCount;

    moduleInfo.Name = PhCreateStringFromUnicodeString(&Module->BaseDllName);
    moduleInfo.OriginalFileName = PhCreateStringFromUnicodeString(&Module->FullDllName);
    moduleInfo.FileName = PhGetFileName(moduleInfo.OriginalFileName);

    if (WindowsVersion >= WINDOWS_8)
    {
        moduleInfo.ParentBaseAddress = Module->ParentDllBase;
        moduleInfo.LoadReason = (USHORT)Module->LoadReason;
        moduleInfo.LoadTime = Module->LoadTime;
    }
    else
    {
        moduleInfo.ParentBaseAddress = NULL;
        moduleInfo.LoadReason = USHRT_MAX;
        moduleInfo.LoadTime.QuadPart = 0;
    }

    cont = context->Callback(&moduleInfo, context->Context);

    PhDereferenceObject(moduleInfo.Name);
    PhDereferenceObject(moduleInfo.FileName);
    PhDereferenceObject(moduleInfo.OriginalFileName);

    return cont;
}

VOID PhpRtlModulesToGenericModules(
    _In_ PRTL_PROCESS_MODULES Modules,
    _In_ PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _In_ PPH_HASHTABLE BaseAddressHashtable
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
        if (PhFindEntryHashtable(BaseAddressHashtable, &module->ImageBase))
        {
            continue;
        }
        else
        {
            PhAddEntryHashtable(BaseAddressHashtable, &module->ImageBase);
        }

        fileName = PhConvertMultiByteToUtf16(module->FullPathName);

        if ((ULONG_PTR)module->ImageBase <= PhSystemBasicInformation.MaximumUserModeAddress)
            moduleInfo.Type = PH_MODULE_TYPE_MODULE;
        else
            moduleInfo.Type = PH_MODULE_TYPE_KERNEL_MODULE;

        moduleInfo.BaseAddress = module->ImageBase;
        moduleInfo.Size = module->ImageSize;
        moduleInfo.EntryPoint = NULL;
        moduleInfo.Flags = module->Flags;
        moduleInfo.Name = PhConvertMultiByteToUtf16(&module->FullPathName[module->OffsetToFileName]);
        moduleInfo.FileName = PhGetFileName(fileName); // convert to DOS file name
        moduleInfo.OriginalFileName = fileName;
        moduleInfo.LoadOrderIndex = module->LoadOrderIndex;
        moduleInfo.LoadCount = module->LoadCount;
        moduleInfo.LoadReason = USHRT_MAX;
        moduleInfo.LoadTime.QuadPart = 0;
        moduleInfo.ParentBaseAddress = NULL;

        if (module->OffsetToFileName == 0)
        {
            static PH_STRINGREF driversString = PH_STRINGREF_INIT(L"\\System32\\Drivers\\");
            PH_STRINGREF systemRoot;
            PPH_STRING newFileName;

            // We only have the file name, without a path. The driver must be in the default drivers
            // directory.
            PhGetSystemRoot(&systemRoot);
            newFileName = PhConcatStringRef3(&systemRoot, &driversString, &moduleInfo.Name->sr);
            PhDereferenceObject(moduleInfo.FileName);
            moduleInfo.FileName = newFileName;
        }

        cont = Callback(&moduleInfo, Context);

        PhDereferenceObject(moduleInfo.Name);
        PhDereferenceObject(moduleInfo.FileName);
        PhDereferenceObject(moduleInfo.OriginalFileName);

        if (!cont)
            break;
    }
}

VOID PhpRtlModulesExToGenericModules(
    _In_ PRTL_PROCESS_MODULE_INFORMATION_EX Modules,
    _In_ PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _In_ PPH_HASHTABLE BaseAddressHashtable
    )
{
    PRTL_PROCESS_MODULE_INFORMATION_EX module;
    PH_MODULE_INFO moduleInfo;
    BOOLEAN cont;

    module = Modules;

    while (module->NextOffset != 0)
    {
        PPH_STRING fileName;

        // Check if we have a duplicate base address.
        if (PhFindEntryHashtable(BaseAddressHashtable, &module->BaseInfo.ImageBase))
        {
            continue;
        }
        else
        {
            PhAddEntryHashtable(BaseAddressHashtable, &module->BaseInfo.ImageBase);
        }

        fileName = PhConvertMultiByteToUtf16(module->BaseInfo.FullPathName);

        if ((ULONG_PTR)module->BaseInfo.ImageBase <= PhSystemBasicInformation.MaximumUserModeAddress)
            moduleInfo.Type = PH_MODULE_TYPE_MODULE;
        else
            moduleInfo.Type = PH_MODULE_TYPE_KERNEL_MODULE;

        moduleInfo.BaseAddress = module->BaseInfo.ImageBase;
        moduleInfo.Size = module->BaseInfo.ImageSize;
        moduleInfo.EntryPoint = NULL;
        moduleInfo.Flags = module->BaseInfo.Flags;
        moduleInfo.Name = PhConvertMultiByteToUtf16(&module->BaseInfo.FullPathName[module->BaseInfo.OffsetToFileName]);
        moduleInfo.FileName = PhGetFileName(fileName); // convert to DOS file name
        moduleInfo.LoadOrderIndex = module->BaseInfo.LoadOrderIndex;
        moduleInfo.LoadCount = module->BaseInfo.LoadCount;
        moduleInfo.LoadReason = USHRT_MAX;
        moduleInfo.LoadTime.QuadPart = 0;
        moduleInfo.ParentBaseAddress = NULL;

        PhDereferenceObject(fileName);

        cont = Callback(&moduleInfo, Context);

        PhDereferenceObject(moduleInfo.Name);
        PhDereferenceObject(moduleInfo.FileName);

        if (!cont)
            break;

        module = PTR_ADD_OFFSET(module, module->NextOffset);
    }
}

BOOLEAN PhpCallbackMappedFileOrImage(
    _In_ PVOID AllocationBase,
    _In_ SIZE_T AllocationSize,
    _In_ ULONG Type,
    _In_ PPH_STRING FileName,
    _In_ PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _In_ PPH_HASHTABLE BaseAddressHashtable
    )
{
    PH_MODULE_INFO moduleInfo;
    BOOLEAN cont;

    moduleInfo.Type = Type;
    moduleInfo.BaseAddress = AllocationBase;
    moduleInfo.Size = (ULONG)AllocationSize;
    moduleInfo.EntryPoint = NULL;
    moduleInfo.Flags = 0;
    moduleInfo.FileName = PhGetFileName(FileName);
    moduleInfo.OriginalFileName = FileName;
    moduleInfo.Name = PhGetBaseName(moduleInfo.FileName);
    moduleInfo.LoadOrderIndex = USHRT_MAX;
    moduleInfo.LoadCount = USHRT_MAX;
    moduleInfo.LoadReason = USHRT_MAX;
    moduleInfo.LoadTime.QuadPart = 0;
    moduleInfo.ParentBaseAddress = NULL;

    cont = Callback(&moduleInfo, Context);

    PhDereferenceObject(moduleInfo.FileName);
    PhDereferenceObject(moduleInfo.OriginalFileName);
    PhDereferenceObject(moduleInfo.Name);

    return cont;
}

VOID PhpEnumGenericMappedFilesAndImages(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Flags,
    _In_ PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _In_ PPH_HASHTABLE BaseAddressHashtable
    )
{
    BOOLEAN querySucceeded;
    PVOID baseAddress;
    MEMORY_BASIC_INFORMATION basicInfo;

    baseAddress = (PVOID)0;

    if (!NT_SUCCESS(NtQueryVirtualMemory(
        ProcessHandle,
        baseAddress,
        MemoryBasicInformation,
        &basicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        return;
    }

    querySucceeded = TRUE;

    while (querySucceeded)
    {
        PVOID allocationBase;
        SIZE_T allocationSize;
        ULONG type;
        PPH_STRING fileName;
        BOOLEAN cont;

        if (basicInfo.Type == MEM_MAPPED || basicInfo.Type == MEM_IMAGE)
        {
            if (basicInfo.Type == MEM_MAPPED)
                type = PH_MODULE_TYPE_MAPPED_FILE;
            else
                type = PH_MODULE_TYPE_MAPPED_IMAGE;

            // Find the total allocation size.

            allocationBase = basicInfo.AllocationBase;
            allocationSize = 0;

            do
            {
                baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);
                allocationSize += basicInfo.RegionSize;

                if (!NT_SUCCESS(NtQueryVirtualMemory(
                    ProcessHandle,
                    baseAddress,
                    MemoryBasicInformation,
                    &basicInfo,
                    sizeof(MEMORY_BASIC_INFORMATION),
                    NULL
                    )))
                {
                    querySucceeded = FALSE;
                    break;
                }
            } while (basicInfo.AllocationBase == allocationBase);

            if ((type == PH_MODULE_TYPE_MAPPED_FILE && !(Flags & PH_ENUM_GENERIC_MAPPED_FILES)) ||
                (type == PH_MODULE_TYPE_MAPPED_IMAGE && !(Flags & PH_ENUM_GENERIC_MAPPED_IMAGES)))
            {
                // The user doesn't want this type of entry.
                continue;
            }

            // Check if we have a duplicate base address.
            if (PhFindEntryHashtable(BaseAddressHashtable, &allocationBase))
            {
                continue;
            }

            if (!NT_SUCCESS(PhGetProcessMappedFileName(
                ProcessHandle,
                allocationBase,
                &fileName
                )))
            {
                continue;
            }

            PhAddEntryHashtable(BaseAddressHashtable, &allocationBase);

            cont = PhpCallbackMappedFileOrImage(
                allocationBase,
                allocationSize,
                type,
                fileName,
                Callback,
                Context,
                BaseAddressHashtable
                );

            if (!cont)
                break;
        }
        else
        {
            baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);

            if (!NT_SUCCESS(NtQueryVirtualMemory(
                ProcessHandle,
                baseAddress,
                MemoryBasicInformation,
                &basicInfo,
                sizeof(MEMORY_BASIC_INFORMATION),
                NULL
                )))
            {
                querySucceeded = FALSE;
            }
        }
    }
}

BOOLEAN NTAPI PhpBaseAddressHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return *(PVOID *)Entry1 == *(PVOID *)Entry2;
}

ULONG NTAPI PhpBaseAddressHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)*(PVOID *)Entry);
}

/**
 * Enumerates the modules loaded by a process.
 *
 * \param ProcessId The ID of a process. If \ref SYSTEM_PROCESS_ID is specified the function
 * enumerates the kernel modules.
 * \param ProcessHandle A handle to the process.
 * \param Flags Flags controlling the information to retrieve.
 * \li \c PH_ENUM_GENERIC_MAPPED_FILES Enumerate mapped files.
 * \li \c PH_ENUM_GENERIC_MAPPED_IMAGES Enumerate mapped images (those which are not mapped by the
 * loader).
 * \param Callback A callback function which is executed for each module.
 * \param Context A user-defined value to pass to the callback function.
 */
NTSTATUS PhEnumGenericModules(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ ULONG Flags,
    _In_ PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PPH_HASHTABLE baseAddressHashtable;

    baseAddressHashtable = PhCreateHashtable(
        sizeof(PVOID),
        PhpBaseAddressHashtableEqualFunction,
        PhpBaseAddressHashtableHashFunction,
        100
        );

    if (ProcessId == SYSTEM_PROCESS_ID)
    {
        // Kernel modules

        PVOID modules;

        if (NT_SUCCESS(status = PhEnumKernelModules((PRTL_PROCESS_MODULES *)&modules)))
        {
            PhpRtlModulesToGenericModules(
                modules,
                Callback,
                Context,
                baseAddressHashtable
                );
            PhFree(modules);
        }
    }
    else
    {
        // Process modules

        BOOLEAN opened = FALSE;
#ifdef _WIN64
        BOOLEAN isWow64 = FALSE;
#endif
        ENUM_GENERIC_PROCESS_MODULES_CONTEXT context;
        PH_ENUM_PROCESS_MODULES_PARAMETERS parameters;

        if (!ProcessHandle)
        {
            if (!NT_SUCCESS(status = PhOpenProcess(
                &ProcessHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, // needed for enumerating mapped files
                ProcessId
                )))
            {
                if (!NT_SUCCESS(status = PhOpenProcess(
                    &ProcessHandle,
                    PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
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
        context.Type = PH_MODULE_TYPE_MODULE;
        context.BaseAddressHashtable = baseAddressHashtable;
        context.LoadOrderIndex = 0;

        parameters.Callback = EnumGenericProcessModulesCallback;
        parameters.Context = &context;
        parameters.Flags = PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME;

        status = PhEnumProcessModulesEx(
            ProcessHandle,
            &parameters
            );

#ifdef _WIN64
        PhGetProcessIsWow64(ProcessHandle, &isWow64);

        // 32-bit process modules
        if (isWow64)
        {
            context.Callback = Callback;
            context.Context = Context;
            context.Type = PH_MODULE_TYPE_WOW64_MODULE;
            context.BaseAddressHashtable = baseAddressHashtable;
            context.LoadOrderIndex = 0;

            status = PhEnumProcessModules32Ex(
                ProcessHandle,
                &parameters
                );
        }
#endif

        // Mapped files and mapped images
        // This is done last because it provides the least amount of information.

        if (Flags & (PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES))
        {
            PhpEnumGenericMappedFilesAndImages(
                ProcessHandle,
                Flags,
                Callback,
                Context,
                baseAddressHashtable
                );
        }

        if (opened)
            NtClose(ProcessHandle);
    }

CleanupExit:
    PhDereferenceObject(baseAddressHashtable);

    return status;
}

/**
 * Initializes usage of predefined keys.
 */
VOID PhpInitializePredefineKeys(
    VOID
    )
{
    static UNICODE_STRING currentUserPrefix = RTL_CONSTANT_STRING(L"\\Registry\\User\\");
    NTSTATUS status;
    PTOKEN_USER tokenUser;
    UNICODE_STRING stringSid;
    WCHAR stringSidBuffer[SECURITY_MAX_SID_STRING_CHARACTERS];
    PUNICODE_STRING currentUserKeyName;

    // Get the string SID of the current user.

    if (NT_SUCCESS(status = PhGetTokenUser(PhGetOwnTokenAttributes().TokenHandle, &tokenUser)))
    {
        stringSid.Buffer = stringSidBuffer;
        stringSid.MaximumLength = sizeof(stringSidBuffer);

        status = RtlConvertSidToUnicodeString(
            &stringSid,
            tokenUser->User.Sid,
            FALSE
            );

        PhFree(tokenUser);
    }

    // Construct the current user key name.

    if (NT_SUCCESS(status))
    {
        currentUserKeyName = &PhPredefineKeyNames[PH_KEY_CURRENT_USER_NUMBER];
        currentUserKeyName->Length = currentUserPrefix.Length + stringSid.Length;
        currentUserKeyName->Buffer = PhAllocate(currentUserKeyName->Length + sizeof(WCHAR));
        memcpy(currentUserKeyName->Buffer, currentUserPrefix.Buffer, currentUserPrefix.Length);
        memcpy(&currentUserKeyName->Buffer[currentUserPrefix.Length / sizeof(WCHAR)], stringSid.Buffer, stringSid.Length);
    }
}

/**
 * Initializes the attributes of a key object for creating/opening.
 *
 * \param RootDirectory A handle to a root key, or one of the predefined keys. See PhCreateKey() for
 * details.
 * \param ObjectName The path to the key.
 * \param Attributes Additional object flags.
 * \param ObjectAttributes The OBJECT_ATTRIBUTES structure to initialize.
 * \param NeedsClose A variable which receives a handle that must be closed when the create/open
 * operation is finished. The variable may be set to NULL if no handle needs to be closed.
 */
NTSTATUS PhpInitializeKeyObjectAttributes(
    _In_opt_ HANDLE RootDirectory,
    _In_ PUNICODE_STRING ObjectName,
    _In_ ULONG Attributes,
    _Out_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PHANDLE NeedsClose
    )
{
    NTSTATUS status;
    ULONG predefineIndex;
    HANDLE predefineHandle;
    OBJECT_ATTRIBUTES predefineObjectAttributes;

    InitializeObjectAttributes(
        ObjectAttributes,
        ObjectName,
        Attributes | OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    *NeedsClose = NULL;

    if (RootDirectory && PH_KEY_IS_PREDEFINED(RootDirectory))
    {
        predefineIndex = PH_KEY_PREDEFINE_TO_NUMBER(RootDirectory);

        if (predefineIndex < PH_KEY_MAXIMUM_PREDEFINE)
        {
            if (PhBeginInitOnce(&PhPredefineKeyInitOnce))
            {
                PhpInitializePredefineKeys();
                PhEndInitOnce(&PhPredefineKeyInitOnce);
            }

            predefineHandle = PhPredefineKeyHandles[predefineIndex];

            if (!predefineHandle)
            {
                // The predefined key has not been opened yet. Do so now.

                if (!PhPredefineKeyNames[predefineIndex].Buffer) // we may have failed in getting the current user key name
                    return STATUS_UNSUCCESSFUL;

                InitializeObjectAttributes(
                    &predefineObjectAttributes,
                    &PhPredefineKeyNames[predefineIndex],
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );

                status = NtOpenKey(
                    &predefineHandle,
                    KEY_READ,
                    &predefineObjectAttributes
                    );

                if (!NT_SUCCESS(status))
                    return status;

                if (_InterlockedCompareExchangePointer(
                    &PhPredefineKeyHandles[predefineIndex],
                    predefineHandle,
                    NULL
                    ) != NULL)
                {
                    // Someone else already opened the key and cached it. Indicate that the caller
                    // needs to close the handle later, since it isn't shared.
                    *NeedsClose = predefineHandle;
                }
            }

            ObjectAttributes->RootDirectory = predefineHandle;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Creates or opens a registry key.
 *
 * \param KeyHandle A variable which receives a handle to the key.
 * \param DesiredAccess The desired access to the key.
 * \param RootDirectory A handle to a root key, or one of the following predefined keys:
 * \li \c PH_KEY_LOCAL_MACHINE Represents \\Registry\\Machine.
 * \li \c PH_KEY_USERS Represents \\Registry\\User.
 * \li \c PH_KEY_CLASSES_ROOT Represents \\Registry\\Machine\\Software\\Classes.
 * \li \c PH_KEY_CURRENT_USER Represents \\Registry\\User\\[SID of current user].
 * \param ObjectName The path to the key.
 * \param Attributes Additional object flags.
 * \param CreateOptions The options to apply when creating or opening the key.
 * \param Disposition A variable which receives a value indicating whether a new key was created or
 * an existing key was opened:
 * \li \c REG_CREATED_NEW_KEY A new key was created.
 * \li \c REG_OPENED_EXISTING_KEY An existing key was opened.
 */
NTSTATUS PhCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName,
    _In_ ULONG Attributes,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE needsClose;

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    if (!NT_SUCCESS(status = PhpInitializeKeyObjectAttributes(
        RootDirectory,
        &objectName,
        Attributes,
        &objectAttributes,
        &needsClose
        )))
    {
        return status;
    }

    status = NtCreateKey(
        KeyHandle,
        DesiredAccess,
        &objectAttributes,
        0,
        NULL,
        CreateOptions,
        Disposition
        );

    if (needsClose)
        NtClose(needsClose);

    return status;
}

/**
 * Opens a registry key.
 *
 * \param KeyHandle A variable which receives a handle to the key.
 * \param DesiredAccess The desired access to the key.
 * \param RootDirectory A handle to a root key, or one of the predefined keys. See PhCreateKey() for
 * details.
 * \param ObjectName The path to the key.
 * \param Attributes Additional object flags.
 */
NTSTATUS PhOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName,
    _In_ ULONG Attributes
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE needsClose;

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    if (!NT_SUCCESS(status = PhpInitializeKeyObjectAttributes(
        RootDirectory,
        &objectName,
        Attributes,
        &objectAttributes,
        &needsClose
        )))
    {
        return status;
    }

    status = NtOpenKey(
        KeyHandle,
        DesiredAccess,
        &objectAttributes
        );

    if (needsClose)
        NtClose(needsClose);

    return status;
}

// rev from RegLoadAppKey
/**
 * Loads the specified registry hive file into a private application hive.
 *
 * \param KeyHandle A variable which receives a handle to the key.
 * \param FileName The Win32 file name.
 * \param DesiredAccess The desired access to the key.
 * \param Flags Optional flags for loading the hive.
 */
NTSTATUS PhLoadAppKey(
    _Out_ PHANDLE KeyHandle,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG Flags
    )
{
    NTSTATUS status;
    GUID guid;
    UNICODE_STRING fileName;
    UNICODE_STRING objectName; 
    UNICODE_STRING guidStringUs;
    OBJECT_ATTRIBUTES targetAttributes;
    OBJECT_ATTRIBUTES sourceAttributes;
    WCHAR objectNameBuffer[MAX_PATH];

    RtlInitEmptyUnicodeString(&objectName, objectNameBuffer, sizeof(objectNameBuffer));

    PhGenerateGuid(&guid);

    if (!NT_SUCCESS(status = RtlStringFromGUID(&guid, &guidStringUs)))
        return status;

    if (!NT_SUCCESS(status = RtlAppendUnicodeToString(&objectName, L"\\REGISTRY\\A\\")))
        goto CleanupExit;

    if (!NT_SUCCESS(status = RtlAppendUnicodeStringToString(&objectName, &guidStringUs)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = RtlDosPathNameToNtPathName_U_WithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        )))
    {
        goto CleanupExit;
    }

    InitializeObjectAttributes(
        &targetAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    InitializeObjectAttributes(
        &sourceAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtLoadKeyEx(
        &targetAttributes,
        &sourceAttributes,
        REG_APP_HIVE | Flags,
        NULL,
        NULL,
        DesiredAccess,
        KeyHandle,
        NULL
        );

    RtlFreeUnicodeString(&fileName);

CleanupExit:
    RtlFreeUnicodeString(&guidStringUs);

    return status;
}

/**
 * Gets information about a registry key.
 *
 * \param KeyHandle A handle to the key.
 * \param KeyInformationClass The information class to query.
 * \param Buffer A variable which receives a pointer to a buffer containing information about the
 * registry key. You must free the buffer with PhFree() when you no longer need it.
 */
NTSTATUS PhQueryKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_INFORMATION_CLASS KeyInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts = 16;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    do
    {
        status = NtQueryKey(
            KeyHandle,
            KeyInformationClass,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            PhFree(buffer);
            return status;
        }
    } while (--attempts);

    *Buffer = buffer;

    return status;
}

/**
 * Gets a registry value of any type.
 *
 * \param KeyHandle A handle to the key.
 * \param ValueName The name of the value.
 * \param KeyValueInformationClass The information class to query.
 * \param Buffer A variable which receives a pointer to a buffer containing information about the
 * registry value. You must free the buffer with PhFree() when you no longer need it.
 */
NTSTATUS PhQueryValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    UNICODE_STRING valueName;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts = 16;

    if (ValueName)
    {
        if (!PhStringRefToUnicodeString(ValueName, &valueName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitUnicodeString(&valueName, NULL);
    }

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    do
    {
        status = NtQueryValueKey(
            KeyHandle,
            &valueName,
            KeyValueInformationClass,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            PhFree(buffer);
            return status;
        }
    } while (--attempts);

    *Buffer = buffer;

    return status;
}

NTSTATUS PhEnumerateKey(
    _In_ HANDLE KeyHandle,
    _In_ PPH_ENUM_KEY_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG index = 0;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtEnumerateKey(
            KeyHandle,
            index,
            KeyBasicInformation,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_NO_MORE_ENTRIES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);

            status = NtEnumerateKey(
                KeyHandle,
                index,
                KeyBasicInformation,
                buffer,
                bufferSize,
                &bufferSize
                );
        }

        if (!NT_SUCCESS(status))
            break;

        if (!Callback(buffer, Context))
            break;

        index++;
    }

    PhFree(buffer);

    return status;
}

/**
 * Creates or opens a file.
 *
 * \param FileHandle A variable that receives the file handle.
 * \param FileName The Win32 file name.
 * \param DesiredAccess The desired access to the file.
 * \param FileAttributes File attributes applied if the file is created or overwritten.
 * \param ShareAccess The file access granted to other threads.
 * \li \c FILE_SHARE_READ Allows other threads to read from the file.
 * \li \c FILE_SHARE_WRITE Allows other threads to write to the file.
 * \li \c FILE_SHARE_DELETE Allows other threads to delete the file.
 * \param CreateDisposition The action to perform if the file does or does not exist.
 * \li \c FILE_SUPERSEDE If the file exists, replace it. Otherwise, create the file.
 * \li \c FILE_CREATE If the file exists, fail. Otherwise, create the file.
 * \li \c FILE_OPEN If the file exists, open it. Otherwise, fail.
 * \li \c FILE_OPEN_IF If the file exists, open it. Otherwise, create the file.
 * \li \c FILE_OVERWRITE If the file exists, open and overwrite it. Otherwise, fail.
 * \li \c FILE_OVERWRITE_IF If the file exists, open and overwrite it. Otherwise, create the file.
 * \param CreateOptions The options to apply when the file is opened or created.
 */
NTSTATUS PhCreateFileWin32(
    _Out_ PHANDLE FileHandle,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions
    )
{
    return PhCreateFileWin32Ex(
        FileHandle,
        FileName,
        DesiredAccess,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL
        );
}

/**
 * Creates or opens a file.
 *
 * \param FileHandle A variable that receives the file handle.
 * \param FileName The Win32 file name.
 * \param DesiredAccess The desired access to the file.
 * \param FileAttributes File attributes applied if the file is created or overwritten.
 * \param ShareAccess The file access granted to other threads.
 * \li \c FILE_SHARE_READ Allows other threads to read from the file.
 * \li \c FILE_SHARE_WRITE Allows other threads to write to the file.
 * \li \c FILE_SHARE_DELETE Allows other threads to delete the file.
 * \param CreateDisposition The action to perform if the file does or does not exist.
 * \li \c FILE_SUPERSEDE If the file exists, replace it. Otherwise, create the file.
 * \li \c FILE_CREATE If the file exists, fail. Otherwise, create the file.
 * \li \c FILE_OPEN If the file exists, open it. Otherwise, fail.
 * \li \c FILE_OPEN_IF If the file exists, open it. Otherwise, create the file.
 * \li \c FILE_OVERWRITE If the file exists, open and overwrite it. Otherwise, fail.
 * \li \c FILE_OVERWRITE_IF If the file exists, open and overwrite it. Otherwise, create the file.
 * \param CreateOptions The options to apply when the file is opened or created.
 * \param CreateStatus A variable that receives creation information.
 * \li \c FILE_SUPERSEDED The file was replaced because \c FILE_SUPERSEDE was specified in
 * \a CreateDisposition.
 * \li \c FILE_OPENED The file was opened because \c FILE_OPEN or \c FILE_OPEN_IF was specified in
 * \a CreateDisposition.
 * \li \c FILE_CREATED The file was created because \c FILE_CREATE or \c FILE_OPEN_IF was specified
 * in \a CreateDisposition.
 * \li \c FILE_OVERWRITTEN The file was overwritten because \c FILE_OVERWRITE or
 * \c FILE_OVERWRITE_IF was specified in \a CreateDisposition.
 * \li \c FILE_EXISTS The file was not opened because it already existed and \c FILE_CREATE was
 * specified in \a CreateDisposition.
 * \li \c FILE_DOES_NOT_EXIST The file was not opened because it did not exist and \c FILE_OPEN or
 * \c FILE_OVERWRITE was specified in \a CreateDisposition.
 */
NTSTATUS PhCreateFileWin32Ex(
    _Out_ PHANDLE FileHandle,
    _In_ PWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG CreateStatus
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;

    if (!FileAttributes)
        FileAttributes = FILE_ATTRIBUTE_NORMAL;

    if (!NT_SUCCESS(status = RtlDosPathNameToNtPathName_U_WithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        )))
        return status;

    InitializeObjectAttributes(
        &oa,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &oa,
        &isb,
        NULL,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0
        );

    RtlFreeUnicodeString(&fileName);

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (CreateStatus)
        *CreateStatus = (ULONG)isb.Information;

    return status;
}

/**
 * Queries file attributes.
 *
 * \param FileName The Win32 file name.
 * \param FileInformation A variable that receives the file information.
 */
NTSTATUS PhQueryFullAttributesFileWin32(
    _In_ PWSTR FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES oa;

    if (!NT_SUCCESS(status = RtlDosPathNameToNtPathName_U_WithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        )))
        return status;

    InitializeObjectAttributes(
        &oa,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryFullAttributesFile(&oa, FileInformation);

    RtlFreeUnicodeString(&fileName);

    return status;
}

/**
 * Deletes a file.
 *
 * \param FileName The Win32 file name.
 */
NTSTATUS PhDeleteFileWin32(
    _In_ PWSTR FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        DELETE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DELETE_ON_CLOSE
        );

    if (!NT_SUCCESS(status))
        return status;

    NtClose(fileHandle);

    return status;
}

/**
* Creates a directory path recursively.
*
* \param DirectoryPath The Win32 directory path.
*/
NTSTATUS PhCreateDirectory(
    _In_ PPH_STRING DirectoryPath
    )
{
    static PH_STRINGREF directorySeparator = PH_STRINGREF_INIT(L"\\");
    PPH_STRING directoryPath = NULL;
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    if (PhIsNullOrEmptyString(DirectoryPath))
        return STATUS_FAIL_CHECK;

    if (RtlDoesFileExists_U(PhGetString(DirectoryPath)))
        return STATUS_SUCCESS;

    remainingPart = PhGetStringRef(DirectoryPath);

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &part, &remainingPart);

        if (part.Length != 0)
        {
            if (PhIsNullOrEmptyString(directoryPath))
                directoryPath = PhCreateString2(&part);
            else
            {
                PPH_STRING tempPathString;

                tempPathString = PhConcatStringRef3(
                    &directoryPath->sr,
                    &directorySeparator,
                    &part
                    );

                // Check if the directory already exists.
                if (!RtlDoesFileExists_U(PhGetString(tempPathString)))
                {
                    HANDLE directoryHandle;

                    // Create the directory.
                    if (NT_SUCCESS(PhCreateFileWin32(
                        &directoryHandle,
                        PhGetString(tempPathString),
                        FILE_GENERIC_READ,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_CREATE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                        )))
                    {
                        NtClose(directoryHandle);
                    }
                }

                PhMoveReference(&directoryPath, tempPathString);
            }
        }
    }

    if (directoryPath)
        PhDereferenceObject(directoryPath);

    if (RtlDoesFileExists_U(PhGetString(DirectoryPath)))
        return STATUS_SUCCESS;
    else
        return STATUS_NOT_FOUND;
}

static BOOLEAN PhpDeleteDirectoryCallback(
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    static PH_STRINGREF directorySeparator = PH_STRINGREF_INIT(L"\\");
    PPH_STRING parentDirectory = Context;
    PPH_STRING fullName;
    PH_STRINGREF baseName;

    baseName.Buffer = Information->FileName;
    baseName.Length = Information->FileNameLength;

    if (PhEqualStringRef2(&baseName, L".", TRUE) || PhEqualStringRef2(&baseName, L"..", TRUE))
        return TRUE;

    fullName = PhConcatStringRef3(
        &parentDirectory->sr,
        &directorySeparator,
        &baseName
        );

    if (Information->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        HANDLE directoryHandle;

        if (NT_SUCCESS(PhCreateFileWin32(
            &directoryHandle,
            PhGetString(fullName),
            FILE_GENERIC_READ | DELETE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            IO_STATUS_BLOCK isb;
            FILE_DISPOSITION_INFORMATION fileInfo;

            PhEnumDirectoryFile(directoryHandle, NULL, PhpDeleteDirectoryCallback, fullName);

            // Delete the directory. 
            fileInfo.DeleteFile = TRUE;
            NtSetInformationFile(
                directoryHandle,
                &isb,
                &fileInfo,
                sizeof(FILE_DISPOSITION_INFORMATION),
                FileDispositionInformation
                );

            NtClose(directoryHandle);
        }
    }
    else
    {
        if (Information->FileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            HANDLE fileHandle;

            if (NT_SUCCESS(PhCreateFileWin32(
                &fileHandle,
                PhGetString(fullName),
                FILE_GENERIC_READ | FILE_WRITE_ATTRIBUTES,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                )))
            {
                IO_STATUS_BLOCK isb;
                FILE_BASIC_INFORMATION fileInfo;

                memset(&fileInfo, 0, sizeof(FILE_BASIC_INFORMATION));

                // Clear the read-only flag.
                fileInfo.FileAttributes = Information->FileAttributes &= ~FILE_ATTRIBUTE_READONLY;

                NtSetInformationFile(
                    fileHandle,
                    &isb,
                    &fileInfo,
                    sizeof(FILE_BASIC_INFORMATION),
                    FileBasicInformation
                    );

                NtClose(fileHandle);
            }
        }

        // Delete the file. 
        PhDeleteFileWin32(PhGetString(fullName));
    }

    PhDereferenceObject(fullName);
    return TRUE;
}

/**
* Deletes a directory path recursively.
*
* \param DirectoryPath The Win32 directory path.
*/
NTSTATUS PhDeleteDirectory(
    _In_ PPH_STRING DirectoryPath
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;

    status = PhCreateFileWin32(
        &directoryHandle,
        PhGetString(DirectoryPath),
        FILE_GENERIC_READ | DELETE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        IO_STATUS_BLOCK isb;
        FILE_DISPOSITION_INFORMATION fileInfo;

        // Remove any files or folders inside the directory.
        status = PhEnumDirectoryFile(
            directoryHandle, 
            NULL, 
            PhpDeleteDirectoryCallback, 
            DirectoryPath
            );

        // Remove the directory. 
        fileInfo.DeleteFile = TRUE;
        status = NtSetInformationFile(
            directoryHandle,
            &isb,
            &fileInfo,
            sizeof(FILE_DISPOSITION_INFORMATION),
            FileDispositionInformation
            );

        NtClose(directoryHandle);
    }

    if (!RtlDoesFileExists_U(PhGetString(DirectoryPath)))
        return STATUS_SUCCESS;

    return status;
}

/**
* Creates an anonymous pipe.
*
* \param PipeReadHandle The pipe read handle.
* \param PipeWriteHandle The pipe write handle.
*/
NTSTATUS PhCreatePipe(
    _Out_ PHANDLE PipeReadHandle,
    _Out_ PHANDLE PipeWriteHandle
    )
{
    NTSTATUS status;
    PACL pipeAcl = NULL;
    HANDLE pipeDirectoryHandle;
    HANDLE pipeReadHandle;
    HANDLE pipeWriteHandle;
    LARGE_INTEGER pipeTimeout;
    UNICODE_STRING pipeNameUs;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;

    RtlInitUnicodeString(&pipeNameUs, DEVICE_NAMED_PIPE);
    InitializeObjectAttributes(
        &oa,
        &pipeNameUs,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &pipeDirectoryHandle,
        GENERIC_READ | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    RtlInitUnicodeString(&pipeNameUs, UNICODE_NULL);
    InitializeObjectAttributes(
        &oa,
        &pipeNameUs,
        OBJ_CASE_INSENSITIVE,
        pipeDirectoryHandle,
        NULL
        );

    if (NT_SUCCESS(RtlDefaultNpAcl(&pipeAcl)))
    {
        SECURITY_DESCRIPTOR securityDescriptor;

        RtlCreateSecurityDescriptor(&securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
        RtlSetDaclSecurityDescriptor(&securityDescriptor, TRUE, pipeAcl, FALSE);

        oa.SecurityDescriptor = &securityDescriptor;
    }

    status = NtCreateNamedPipeFile(
        &pipeReadHandle,
        FILE_WRITE_ATTRIBUTES | GENERIC_READ | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_CREATE,
        FILE_PIPE_INBOUND | FILE_SYNCHRONOUS_IO_NONALERT,
        FILE_PIPE_BYTE_STREAM_TYPE,
        FILE_PIPE_BYTE_STREAM_MODE,
        FILE_PIPE_QUEUE_OPERATION,
        ULONG_MAX,
        PAGE_SIZE,
        PAGE_SIZE,
        PhTimeoutFromMilliseconds(&pipeTimeout, 500)
        );

    if (!NT_SUCCESS(status))
    {
        if (pipeAcl)
            RtlFreeHeap(RtlProcessHeap(), 0, pipeAcl);

        NtClose(pipeDirectoryHandle);
        return status;
    }

    RtlInitUnicodeString(&pipeNameUs, UNICODE_NULL);
    InitializeObjectAttributes(
        &oa,
        &pipeNameUs,
        OBJ_CASE_INSENSITIVE,
        pipeReadHandle,
        NULL
        );

    status = NtOpenFile(
        &pipeWriteHandle,
        FILE_READ_ATTRIBUTES | GENERIC_WRITE | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        *PipeReadHandle = pipeReadHandle;
        *PipeWriteHandle = pipeWriteHandle;
    }

    if (pipeAcl)
        RtlFreeHeap(RtlProcessHeap(), 0, pipeAcl);

    NtClose(pipeDirectoryHandle);
    return status;
}

/**
* Creates an named pipe.
*
* \param PipeHandle The pipe read/write handle.
* \param PipeName The pipe name.
*/
NTSTATUS PhCreateNamedPipe(
    _Out_ PHANDLE PipeHandle,
    _In_ PWSTR PipeName
    )
{
    NTSTATUS status;
    PACL pipeAcl = NULL;
    HANDLE pipeHandle;
    PPH_STRING pipeName;
    LARGE_INTEGER pipeTimeout;
    UNICODE_STRING pipeNameUs;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;

    pipeName = PhConcatStrings2(DEVICE_NAMED_PIPE, PipeName);
    PhStringRefToUnicodeString(&pipeName->sr, &pipeNameUs);
    PhTimeoutFromMilliseconds(&pipeTimeout, 500);

    InitializeObjectAttributes(
        &oa,
        &pipeNameUs,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    if (NT_SUCCESS(RtlDefaultNpAcl(&pipeAcl)))
    {
        SECURITY_DESCRIPTOR securityDescriptor;

        RtlCreateSecurityDescriptor(&securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
        RtlSetDaclSecurityDescriptor(&securityDescriptor, TRUE, pipeAcl, FALSE);

        oa.SecurityDescriptor = &securityDescriptor;
    }

    status = NtCreateNamedPipeFile(
        &pipeHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_PIPE_FULL_DUPLEX | FILE_SYNCHRONOUS_IO_NONALERT,
        FILE_PIPE_MESSAGE_TYPE,
        FILE_PIPE_MESSAGE_MODE,
        FILE_PIPE_QUEUE_OPERATION,
        ULONG_MAX,
        PAGE_SIZE,
        PAGE_SIZE,
        &pipeTimeout
        );

    if (NT_SUCCESS(status))
    {
        *PipeHandle = pipeHandle;
    }

    if (pipeAcl)
        RtlFreeHeap(RtlProcessHeap(), 0, pipeAcl);

    PhDereferenceObject(pipeName);
    return status;
}

NTSTATUS PhConnectPipe(
    _Out_ PHANDLE PipeHandle,
    _In_ PWSTR PipeName
    )
{
    NTSTATUS status;
    HANDLE pipeHandle;
    PPH_STRING pipeName;
    UNICODE_STRING pipeNameUs;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;

    pipeName = PhConcatStrings2(DEVICE_NAMED_PIPE, PipeName);
    PhStringRefToUnicodeString(&pipeName->sr, &pipeNameUs);

    InitializeObjectAttributes(
        &oa,
        &pipeNameUs,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &pipeHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        &oa,
        &isb,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (NT_SUCCESS(status))
    {
        *PipeHandle = pipeHandle;
    }

    PhDereferenceObject(pipeName);
    return status;
}

NTSTATUS PhListenNamedPipe(
    _In_ HANDLE PipeHandle
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_LISTEN,
        NULL,
        0,
        NULL,
        0
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    return status;
}

NTSTATUS PhDisconnectNamedPipe(
    _In_ HANDLE PipeHandle
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_DISCONNECT,
        NULL,
        0,
        NULL,
        0
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    return status;
}

NTSTATUS PhPeekNamedPipe(
    _In_ HANDLE PipeHandle,
    _Out_writes_bytes_opt_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG NumberOfBytesRead,
    _Out_opt_ PULONG NumberOfBytesAvailable,
    _Out_opt_ PULONG NumberOfBytesLeftInMessage
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PFILE_PIPE_PEEK_BUFFER peekBuffer;
    ULONG peekBufferLength;

    peekBufferLength = FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data) + Length;
    peekBuffer = PhAllocate(peekBufferLength);

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_PEEK,
        NULL,
        0,
        peekBuffer,
        peekBufferLength
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    // STATUS_BUFFER_OVERFLOW means that there is data remaining; this is normal.
    if (status == STATUS_BUFFER_OVERFLOW)
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        ULONG numberOfBytesRead = 0;

        if (Buffer || NumberOfBytesRead || NumberOfBytesLeftInMessage)
            numberOfBytesRead = (ULONG)(isb.Information - FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data));

        if (Buffer)
            memcpy(Buffer, peekBuffer->Data, numberOfBytesRead);

        if (NumberOfBytesRead)
            *NumberOfBytesRead = numberOfBytesRead;

        if (NumberOfBytesAvailable)
            *NumberOfBytesAvailable = peekBuffer->ReadDataAvailable;

        if (NumberOfBytesLeftInMessage)
            *NumberOfBytesLeftInMessage = peekBuffer->MessageLength - numberOfBytesRead;
    }

    PhFree(peekBuffer);

    return status;
}

NTSTATUS PhTransceiveNamedPipe(
    _In_ HANDLE PipeHandle,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_TRANSCEIVE,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    return status;
}

NTSTATUS PhWaitForNamedPipe(
    _In_ PWSTR PipeName,
    _In_opt_ ULONG Timeout
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PH_STRINGREF localNpfsNameSr;
    UNICODE_STRING localNpfsName;
    HANDLE fileSystemHandle;
    OBJECT_ATTRIBUTES oa;
    PFILE_PIPE_WAIT_FOR_BUFFER waitForBuffer;
    ULONG waitForBufferLength;

    RtlInitUnicodeString(&localNpfsName, DEVICE_NAMED_PIPE);
    InitializeObjectAttributes(
        &oa,
        &localNpfsName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &fileSystemHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &oa,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    PhInitializeStringRefLongHint(&localNpfsNameSr, PipeName);
    waitForBufferLength = FIELD_OFFSET(FILE_PIPE_WAIT_FOR_BUFFER, Name) + (ULONG)localNpfsNameSr.Length;
    waitForBuffer = PhAllocate(waitForBufferLength);

    if (Timeout)
    {
        PhTimeoutFromMilliseconds(&waitForBuffer->Timeout, Timeout);
        waitForBuffer->TimeoutSpecified = TRUE;
    }
    else
    {
        waitForBuffer->Timeout.LowPart = 0;
        waitForBuffer->Timeout.HighPart = MINLONG; // a very long time
        waitForBuffer->TimeoutSpecified = TRUE;
    }

    waitForBuffer->NameLength = (ULONG)localNpfsNameSr.Length;
    memcpy(waitForBuffer->Name, localNpfsNameSr.Buffer, localNpfsNameSr.Length);

    status = NtFsControlFile(
        fileSystemHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_WAIT,
        waitForBuffer,
        waitForBufferLength,
        NULL,
        0
        );

    PhFree(waitForBuffer);
    NtClose(fileSystemHandle);

    return status;
}

NTSTATUS PhImpersonateClientOfNamedPipe(
    _In_ HANDLE PipeHandle
    )
{
    IO_STATUS_BLOCK isb;

    return NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_IMPERSONATE,
        NULL,
        0,
        NULL,
        0
        );
}

NTSTATUS PhGetThreadName(
    _In_ HANDLE ThreadHandle,
    _Out_ PPH_STRING *ThreadName
    )
{
    NTSTATUS status;
    PTHREAD_NAME_INFORMATION buffer;
    ULONG bufferSize;
    ULONG returnLength;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadNameInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = NtQueryInformationThread(
            ThreadHandle,
            ThreadNameInformation,
            buffer,
            bufferSize,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        *ThreadName = PhCreateStringFromUnicodeString(&buffer->ThreadName);
    }

    PhFree(buffer);

    return status;
}
