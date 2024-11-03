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

#include <ph.h>
#include <apiimport.h>
#include <kphuser.h>
#include <lsasup.h>
#include <mapldr.h>

#define PH_DEVICE_PREFIX_LENGTH 64
#define PH_DEVICE_MUP_PREFIX_MAX_COUNT 16

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

        if (WindowsVersion >= WINDOWS_8_1)
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
            PH_TOKEN_USER tokenUser;

            PhGetTokenElevation(attributes.TokenHandle, &elevated);
            PhGetTokenElevationType(attributes.TokenHandle, &elevationType);

            if (NT_SUCCESS(PhGetTokenUser(attributes.TokenHandle, &tokenUser)))
            {
                attributes.TokenSid = PhAllocateCopy(tokenUser.User.Sid, PhLengthSid(tokenUser.User.Sid));
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
    KPH_LEVEL level;

    clientId.UniqueProcess = ProcessId;
    clientId.UniqueThread = NULL;

#ifdef _DEBUG
    if (ProcessId == NtCurrentProcessId())
    {
        *ProcessHandle = NtCurrentProcess();
        return STATUS_SUCCESS;
    }
#endif

    level = KsiLevel();

    if ((level >= KphLevelMed) && (DesiredAccess & KPH_PROCESS_READ_ACCESS) == DesiredAccess)
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

        if (status == STATUS_ACCESS_DENIED && (level == KphLevelMax))
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

NTSTATUS PhOpenProcessClientId(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ CLIENT_ID ClientId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    KPH_LEVEL level;

    level = KsiLevel();

    if ((level >= KphLevelMed) && (DesiredAccess & KPH_PROCESS_READ_ACCESS) == DesiredAccess)
    {
        status = KphOpenProcess(
            ProcessHandle,
            DesiredAccess,
            &ClientId
            );
    }
    else
    {
        InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
        status = NtOpenProcess(
            ProcessHandle,
            DesiredAccess,
            &objectAttributes,
            &ClientId
            );

        if (status == STATUS_ACCESS_DENIED && (level == KphLevelMax))
        {
            status = KphOpenProcess(
                ProcessHandle,
                DesiredAccess,
                &ClientId
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
    KPH_LEVEL level;

    level = KsiLevel();

    if ((level >= KphLevelMed) && (DesiredAccess & KPH_TOKEN_READ_ACCESS) == DesiredAccess)
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

        if (status == STATUS_ACCESS_DENIED && (level == KphLevelMax))
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

/** Limited API for untrusted/external code. */
NTSTATUS PhOpenProcessTokenPublic(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
    )
{
    return NtOpenProcessToken(
        ProcessHandle,
        DesiredAccess,
        TokenHandle
        );
}

NTSTATUS PhOpenThreadToken(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _Out_ PHANDLE TokenHandle
    )
{
    NTSTATUS status;

    status = NtOpenThreadToken(
        ThreadHandle,
        DesiredAccess,
        OpenAsSelf,
        TokenHandle
        );

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
    // QuerySecurity properly. (wj32)
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

    if (KsiLevel() == KphLevelMax)
    {
        status = KphTerminateProcess(
            ProcessHandle,
            ExitStatus
            );

        if (NT_SUCCESS(status))
            return status;
    }

    status = NtTerminateProcess(
        ProcessHandle,
        ExitStatus
        );

    return status;
}

NTSTATUS PhGetProcessRuntimeLibrary(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_RUNTIME_LIBRARY* RuntimeLibrary,
    _Out_opt_ PBOOLEAN IsWow64Process
    )
{
    static PH_PROCESS_RUNTIME_LIBRARY NativeRuntime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\user32.dll"),
    };
#ifdef _WIN64
    static PH_PROCESS_RUNTIME_LIBRARY Wow64Runtime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\user32.dll"),
    };
#ifdef _M_ARM64
    static PH_PROCESS_RUNTIME_LIBRARY Arm32Runtime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\user32.dll"),
    };
    static PH_PROCESS_RUNTIME_LIBRARY Chpe32Runtime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\user32.dll"),
    };
#endif
#endif

    *RuntimeLibrary = &NativeRuntime;

    if (IsWow64Process)
        *IsWow64Process = FALSE;

#ifdef _WIN64
    NTSTATUS status;
#ifdef _M_ARM64
    USHORT machine;

    status = PhGetProcessArchitecture(ProcessHandle, &machine);

    if (!NT_SUCCESS(status))
        return status;

    if (machine != IMAGE_FILE_MACHINE_TARGET_HOST)
    {
        switch (machine)
        {
        case IMAGE_FILE_MACHINE_I386:
        case IMAGE_FILE_MACHINE_CHPE_X86:
            {
                *RuntimeLibrary = &Chpe32Runtime;

                if (IsWow64Process)
                    *IsWow64Process = TRUE;
            }
            break;
        case IMAGE_FILE_MACHINE_ARMNT:
            {
                *RuntimeLibrary = &Arm32Runtime;

                if (IsWow64Process)
                    *IsWow64Process = TRUE;
            }
            break;
        case IMAGE_FILE_MACHINE_AMD64:
        case IMAGE_FILE_MACHINE_ARM64:
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }
    }
#else
    BOOLEAN isWow64 = FALSE;

    status = PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (!NT_SUCCESS(status))
        return status;

    if (isWow64)
    {
        *RuntimeLibrary = &Wow64Runtime;

        if (IsWow64Process)
            *IsWow64Process = TRUE;
    }
#endif
#endif

    return STATUS_SUCCESS;
}

// based on https://www.drdobbs.com/a-safer-alternative-to-terminateprocess/184416547 (dmex)
NTSTATUS PhTerminateProcessAlternative(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;
    PVOID rtlExitUserProcess = NULL;
    HANDLE powerRequestHandle = NULL;
    HANDLE threadHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserProcess",
        0,
        &rtlExitUserProcess,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        0,
        0,
        0,
        0,
        rtlExitUserProcess,
        LongToPtr(ExitStatus),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, Timeout);

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
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
    ULONG bufferLength;
    ULONG returnLength = 0;

    bufferLength = sizeof(UNICODE_STRING) + DOS_MAX_PATH_LENGTH;
    fileName = PhAllocate(bufferLength);

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessImageFileName,
        fileName,
        bufferLength,
        &returnLength
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(fileName);
        bufferLength = returnLength;
        fileName = PhAllocate(bufferLength);

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessImageFileName,
            fileName,
            bufferLength,
            &returnLength
            );
    }

    if (!NT_SUCCESS(status))
        return status;

    // Note: Some minimal/pico processes have UNICODE_NULL as their filename. (dmex)
    if (RtlIsNullOrEmptyUnicodeString(fileName))
    {
        PhFree(fileName);
        return STATUS_UNSUCCESSFUL;
    }

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
    ULONG bufferLength;
    ULONG returnLength = 0;

    bufferLength = sizeof(UNICODE_STRING) + DOS_MAX_PATH_LENGTH;
    fileName = PhAllocate(bufferLength);

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessImageFileNameWin32,
        fileName,
        bufferLength,
        &returnLength
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(fileName);
        bufferLength = returnLength;
        fileName = PhAllocate(bufferLength);

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessImageFileNameWin32,
            fileName,
            bufferLength,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        PPH_STRING fileNameWin32;

        // Note: Some minimal/pico processes have UNICODE_NULL as their filename. (dmex)
        if (RtlIsNullOrEmptyUnicodeString(fileName))
        {
            PhFree(fileName);
            return STATUS_UNSUCCESSFUL;
        }

        fileNameWin32 = PhCreateStringFromUnicodeString(fileName);

        // Note: ProcessImageFileNameWin32 returns the NT device path
        // instead of the Win32 path in some cases were drivers haven't
        // registered with the volume manager or have ignored the mount
        // manager (e.g. ImDisk). We workaround these issues by calling
        // PhGetFileName and resolving the NT device prefix. (dmex)

        if (fileNameWin32->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
        {
            PhMoveReference(&fileNameWin32, PhGetFileName(fileNameWin32));
        }

        *FileName = fileNameWin32;
    }

    PhFree(fileName);

    return status;
}

/**
 * Gets the file name of the process' image by a PID.
 *
 * \param ProcessId A unique identifier of the process.
 * \param FullFileName A variable which receives a pointer to a string containing the full name
 * of the file. You must free the string using PhDereferenceObject() when you no longer need it.
 * \param FileName A variable which receives a pointer to a string containing the file name without
 * the path. You must free the string using PhDereferenceObject() when you no longer need it.
 */
NTSTATUS PhGetProcessImageFileNameById(
    _In_ HANDLE ProcessId,
    _Out_opt_ PPH_STRING *FullFileName,
    _Out_opt_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    SYSTEM_PROCESS_ID_INFORMATION data;

    if (!FullFileName && !FileName)
        return STATUS_INVALID_PARAMETER_MIX;

    // On input, specify the PID and a buffer to hold the string.
    data.ProcessId = ProcessId;
    data.ImageName.Length = 0;
    data.ImageName.MaximumLength = 0x100;

    do
    {
        data.ImageName.Buffer = PhAllocateSafe(data.ImageName.MaximumLength);

        if (!data.ImageName.Buffer)
            return STATUS_NO_MEMORY;

        status = NtQuerySystemInformation(
            SystemProcessIdInformation,
            &data,
            sizeof(SYSTEM_PROCESS_ID_INFORMATION),
            NULL
            );

        if (!NT_SUCCESS(status))
            PhFree(data.ImageName.Buffer);

        // Repeat using the correct value the system put into MaximumLength
    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    if (!NT_SUCCESS(status))
        return status;

    if (FullFileName)
        *FullFileName = PhCreateStringFromUnicodeString(&data.ImageName);

    if (FileName)
    {
        PH_STRINGREF stringRef;
        ULONG_PTR index;

        stringRef.Length = data.ImageName.Length;
        stringRef.Buffer = data.ImageName.Buffer;

        // Find where the name starts
        index = PhFindLastCharInStringRef(&stringRef, L'\\', FALSE);

        if (index == SIZE_MAX)
            *FileName = PhCreateStringFromUnicodeString(&data.ImageName);
        else
        {
            // Reference the tail only
            stringRef.Buffer = PTR_ADD_OFFSET(stringRef.Buffer, index * sizeof(WCHAR) + sizeof(UNICODE_NULL));
            stringRef.Length = stringRef.Length - index * sizeof(WCHAR) - sizeof(UNICODE_NULL);

            *FileName = PhCreateString2(&stringRef);
        }
    }

    PhFree(data.ImageName.Buffer);

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
    _In_opt_ HANDLE ProcessId,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    PVOID buffer;
    USHORT bufferSize = 0x100;
    SYSTEM_PROCESS_ID_INFORMATION processIdInfo;

    buffer = PhAllocate(bufferSize);

    processIdInfo.ProcessId = ProcessId;
    processIdInfo.ImageName.Length = 0;
    processIdInfo.ImageName.MaximumLength = bufferSize;
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

    // Note: Some minimal/pico processes have UNICODE_NULL as their filename. (dmex)
    if (RtlIsNullOrEmptyUnicodeString(&processIdInfo.ImageName))
    {
        PhFree(buffer);
        return STATUS_UNSUCCESSFUL;
    }

    *FileName = PhCreateStringFromUnicodeString(&processIdInfo.ImageName);
    PhFree(buffer);

    return status;
}

/**
 * Gets whether a process is being debugged.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION
 * access.
 * \param IsBeingDebugged A variable which receives a boolean indicating whether the process is
 * being debugged.
 */
NTSTATUS PhGetProcessIsBeingDebugged(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsBeingDebugged
    )
{
    NTSTATUS status;
    PVOID debugHandle;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessDebugPort,
        &debugHandle,
        sizeof(PVOID),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *IsBeingDebugged = !!debugHandle;
        return status;
    }

    if (KsiLevel() >= KphLevelLow)
    {
        KPH_PROCESS_STATE processState;

        status = KphQueryInformationProcess(
            ProcessHandle,
            KphProcessStateInformation,
            &processState,
            sizeof(processState),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            *IsBeingDebugged = !BooleanFlagOn(processState, KPH_PROCESS_NOT_BEING_DEBUGGED);
        }
    }

    return status;
}

NTSTATUS PhGetProcessDeviceMap(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG DeviceMap
    )
{
    NTSTATUS status;
#ifndef _WIN64
    PROCESS_DEVICEMAP_INFORMATION deviceMapInfo;
#else
    PROCESS_DEVICEMAP_INFORMATION_EX deviceMapInfo;
#endif
    memset(&deviceMapInfo, 0, sizeof(deviceMapInfo));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessDeviceMap,
        &deviceMapInfo,
        sizeof(deviceMapInfo),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (DeviceMap)
        {
            *DeviceMap = deviceMapInfo.Query.DriveMap;
        }
    }

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
        PVOID pebBaseAddress;
        PVOID processParameters;
        UNICODE_STRING unicodeString;

        // Get the PEB address.
        if (!NT_SUCCESS(status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress)))
            return status;

        // Read the address of the process parameters.
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
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

        if (RtlIsNullOrEmptyUnicodeString(&unicodeString))
        {
            *String = PhReferenceEmptyString();
            return status;
        }

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
        PVOID pebBaseAddress32;
        ULONG processParameters32;
        UNICODE_STRING32 unicodeString32;

        if (!NT_SUCCESS(status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress32)))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress32, FIELD_OFFSET(PEB32, ProcessParameters)),
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

        if (unicodeString32.Length == 0)
        {
            *String = PhReferenceEmptyString();
            return status;
        }

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
 * \param CommandLine A variable which receives a pointer to a string containing the command line. You
 * must free the string using PhDereferenceObject() when you no longer need it.
 */
NTSTATUS PhGetProcessCommandLine(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *CommandLine
    )
{
    if (WindowsVersion >= WINDOWS_8_1)
    {
        NTSTATUS status;
        PUNICODE_STRING buffer;
        ULONG bufferLength;
        ULONG returnLength = 0;

        bufferLength = sizeof(UNICODE_STRING) + DOS_MAX_PATH_LENGTH;
        buffer = PhAllocate(bufferLength);

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessCommandLineInformation,
            buffer,
            bufferLength,
            &returnLength
            );

        if (status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            bufferLength = returnLength;
            buffer = PhAllocate(bufferLength);

            status = NtQueryInformationProcess(
                ProcessHandle,
                ProcessCommandLineInformation,
                buffer,
                bufferLength,
                &returnLength
                );
        }

        if (NT_SUCCESS(status))
        {
            *CommandLine = PhCreateStringFromUnicodeString(buffer);
        }

        PhFree(buffer);

        return status;
    }

    return PhGetProcessPebString(ProcessHandle, PhpoCommandLine, CommandLine);
}

NTSTATUS PhGetProcessCommandLineStringRef(
    _Out_ PPH_STRINGREF CommandLine
    )
{
    PhUnicodeStringToStringRef(&NtCurrentPeb()->ProcessParameters->CommandLine, CommandLine);
    return STATUS_SUCCESS;
}

NTSTATUS PhGetProcessCurrentDirectory(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN IsWow64,
    _Out_ PPH_STRING *CurrentDirectory
    )
{
    return PhGetProcessPebString(ProcessHandle, PhpoCurrentDirectory | (IsWow64 ? PhpoWow64 : 0), CurrentDirectory);
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
    ULONG bufferLength;
    ULONG returnLength = 0;

    bufferLength = UFIELD_OFFSET(PROCESS_WINDOW_INFORMATION, WindowTitle[DOS_MAX_PATH_LENGTH]) + sizeof(UNICODE_NULL);
    windowInfo = PhAllocate(bufferLength);

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessWindowInformation,
        windowInfo,
        bufferLength,
        &returnLength
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(windowInfo);
        bufferLength = returnLength;
        windowInfo = PhAllocate(bufferLength);

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessWindowInformation,
            windowInfo,
            bufferLength,
            &returnLength
            );
    }

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
        PVOID pebBaseAddress;
        PVOID processParameters;

        // Get the PEB address.
        if (!NT_SUCCESS(status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress)))
            return status;

        // Read the address of the process parameters.
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
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
        PVOID pebBaseAddress32;
        ULONG processParameters32;

        if (!NT_SUCCESS(status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress32)))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress32, FIELD_OFFSET(PEB32, ProcessParameters)),
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
        depStatus = PH_PROCESS_DEP_ENABLED;
    else
        depStatus = 0;

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
    SIZE_T environmentLength;

    if (!(Flags & PH_GET_PROCESS_ENVIRONMENT_WOW64))
    {
        PVOID pebBaseAddress;
        PVOID processParameters;

        status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, ProcessParameters)),
            &processParameters,
            sizeof(PVOID),
            NULL
            )))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters, UFIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Environment)),
            &environmentRemote,
            sizeof(PVOID),
            NULL
            )))
            return status;
    }
    else
    {
        PVOID pebBaseAddress32;
        ULONG processParameters32;
        ULONG environmentRemote32;

        status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress32);

        if (!NT_SUCCESS(status))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress32, UFIELD_OFFSET(PEB32, ProcessParameters)),
            &processParameters32,
            sizeof(ULONG),
            NULL
            )))
            return status;

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(processParameters32, UFIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS32, Environment)),
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

    // Read in the entire region of memory.

    environmentLength = (SIZE_T)PTR_SUB_OFFSET(mbi.RegionSize,
        PTR_SUB_OFFSET(environmentRemote, mbi.BaseAddress));

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
        *EnvironmentLength = (ULONG)environmentLength;

    return status;
}

_Success_(return)
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
    currentChar = PTR_ADD_OFFSET(Environment, currentIndex * sizeof(WCHAR));
    startIndex = currentIndex;
    name = currentChar;

    // Find the end of the name.
    while (TRUE)
    {
        if (currentIndex >= length)
            return FALSE;
        if (*currentChar == L'=' && startIndex != currentIndex)
            break; // equality sign is considered as a delimiter unless it is the first character (diversenok)
        if (*currentChar == UNICODE_NULL)
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
        if (*currentChar == UNICODE_NULL)
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

NTSTATUS PhQueryEnvironmentVariableStringRef(
    _In_opt_ PVOID Environment,
    _In_ PPH_STRINGREF Name,
    _Inout_opt_ PPH_STRINGREF Value
    )
{
    NTSTATUS status;
    SIZE_T returnLength;

    status = RtlQueryEnvironmentVariable(
        Environment,
        Name->Buffer,
        Name->Length / sizeof(WCHAR),
        Value ? Value->Buffer : NULL,
        Value ? Value->Length / sizeof(WCHAR) : 0,
        &returnLength
        );

    if (Value && NT_SUCCESS(status))
    {
        Value->Length = returnLength * sizeof(WCHAR);
    }

    return status;
}

NTSTATUS PhQueryEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_ PPH_STRINGREF Name,
    _Out_opt_ PPH_STRING* Value
    )
{
#ifdef PHNT_UNICODESTRING_ENVIRONMENTVARIABLE
    NTSTATUS status;
    UNICODE_STRING variableName;
    UNICODE_STRING variableValue;

    PhStringRefToUnicodeString(Name, &variableName);

    if (Value)
    {
        variableValue.Length = 0x100 * sizeof(WCHAR);
        variableValue.MaximumLength = variableValue.Length + sizeof(UNICODE_NULL);
        variableValue.Buffer = PhAllocate(variableValue.MaximumLength);
    }
    else
    {
        RtlInitEmptyUnicodeString(&variableValue, NULL, 0);
    }

    status = RtlQueryEnvironmentVariable_U(
        Environment,
        &variableName,
        &variableValue
        );

    if (Value && status == STATUS_BUFFER_TOO_SMALL)
    {
        if (variableValue.Length + sizeof(UNICODE_NULL) > UNICODE_STRING_MAX_BYTES)
            variableValue.MaximumLength = variableValue.Length;
        else
            variableValue.MaximumLength = variableValue.Length + sizeof(UNICODE_NULL);

        PhFree(variableValue.Buffer);
        variableValue.Buffer = PhAllocate(variableValue.MaximumLength);

        status = RtlQueryEnvironmentVariable_U(
            Environment,
            &variableName,
            &variableValue
            );
    }

    if (Value && NT_SUCCESS(status))
    {
        *Value = PhCreateStringFromUnicodeString(&variableValue);
    }

    if (Value && variableValue.Buffer)
    {
        PhFree(variableValue.Buffer);
    }

    return status;
#else
    NTSTATUS status;
    PPH_STRING buffer;
    SIZE_T returnLength;

    status = RtlQueryEnvironmentVariable(
        Environment,
        Name->Buffer,
        Name->Length / sizeof(WCHAR),
        NULL,
        0,
        &returnLength
        );

    if (Value && status == STATUS_BUFFER_TOO_SMALL)
    {
        buffer = PhCreateStringEx(NULL, returnLength * sizeof(WCHAR));

        status = RtlQueryEnvironmentVariable(
            Environment,
            Name->Buffer,
            Name->Length / sizeof(WCHAR),
            buffer->Buffer,
            buffer->Length / sizeof(WCHAR),
            &returnLength
            );

        if (NT_SUCCESS(status))
        {
            buffer->Length = returnLength * sizeof(WCHAR);
            *Value = buffer;
        }
        else
        {
            PhDereferenceObject(buffer);
        }
    }
    else
    {
        if (Value)
            *Value = NULL;
    }

    return status;
#endif
}

NTSTATUS PhSetEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_ PPH_STRINGREF Name,
    _In_opt_ PPH_STRINGREF Value
    )
{
    NTSTATUS status;
    UNICODE_STRING variableName;
    UNICODE_STRING variableValue;

    PhStringRefToUnicodeString(Name, &variableName);

    if (Value)
        PhStringRefToUnicodeString(Value, &variableValue);
    else
        RtlInitEmptyUnicodeString(&variableValue, NULL, 0);

    status = RtlSetEnvironmentVariable(
        Environment,
        &variableName,
        &variableValue
        );

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

    returnLength = 0;
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

    if (status == STATUS_BUFFER_OVERFLOW && returnLength > 0) // returnLength > 0 required for MemoryMappedFilename on Windows 7 SP1 (dmex)
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

NTSTATUS PhGetProcessMappedImageInformation(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_ PMEMORY_IMAGE_INFORMATION ImageInformation
    )
{
    NTSTATUS status;
    MEMORY_IMAGE_INFORMATION imageInformation;

    status = NtQueryVirtualMemory(
        ProcessHandle,
        BaseAddress,
        MemoryImageInformation,
        &imageInformation,
        sizeof(MEMORY_IMAGE_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ImageInformation = imageInformation;
    }

    return status;
}

NTSTATUS PhGetProcessMappedImageBaseFromAddress(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID Address,
    _Out_ PVOID* ImageBaseAddress,
    _Out_opt_ PSIZE_T SizeOfImage
    )
{
    NTSTATUS status;
    MEMORY_IMAGE_INFORMATION imageInfo;

    status = PhGetProcessMappedImageInformation(
        ProcessHandle,
        Address,
        &imageInfo
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!imageInfo.ImageBase ||
        imageInfo.ImageNotExecutable ||
        imageInfo.ImagePartialMap ||
        Address < imageInfo.ImageBase)
    {
        return STATUS_UNSUCCESSFUL;
    }

    *ImageBaseAddress = imageInfo.ImageBase;

    if (SizeOfImage)
    {
        *SizeOfImage = imageInfo.SizeOfImage;
    }

    return STATUS_SUCCESS;
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
    PMEMORY_WORKING_SET_INFORMATION buffer;
    SIZE_T bufferSize;
    ULONG attempts = 0;

    bufferSize = 0x8000;
    buffer = PhAllocate(bufferSize);

    status = NtQueryVirtualMemory(
        ProcessHandle,
        NULL,
        MemoryWorkingSetInformation,
        buffer,
        bufferSize,
        NULL
        );

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        bufferSize = UFIELD_OFFSET(MEMORY_WORKING_SET_INFORMATION, WorkingSetInfo[buffer->NumberOfEntries]);
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQueryVirtualMemory(
            ProcessHandle,
            NULL,
            MemoryWorkingSetInformation,
            buffer,
            bufferSize,
            NULL
            );

        attempts++;
    }

    if (!NT_SUCCESS(status))
    {
        // Fall back to using the previous code that we've used since Windows 7 (dmex)
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
    ULONG capturedElementSize = 0;
    ULONG capturedElementCount = 0;
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
    else
    {
        if (capturedEventTrace)
            PhFree(capturedEventTrace);
    }

    return status;
}

NTSTATUS PhTraceControl(
    _In_ ETWTRACECONTROLCODE TraceInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(*OutputBufferLength) PVOID* OutputBuffer,
    _Out_opt_ PULONG OutputBufferLength
    )
{
    NTSTATUS status;
    PVOID buffer = NULL;
    ULONG bufferLength = 0;
    ULONG returnLength = 0;

    status = NtTraceControl(
        TraceInformationClass,
        InputBuffer,
        InputBufferLength,
        NULL,
        0,
        &returnLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        bufferLength = returnLength;
        buffer = PhAllocate(bufferLength);

        status = NtTraceControl(
            TraceInformationClass,
            InputBuffer,
            InputBufferLength,
            buffer,
            bufferLength,
            &bufferLength
            );
    }

    if (NT_SUCCESS(status))
    {
        if (OutputBuffer)
            *OutputBuffer = buffer;
        if (OutputBufferLength)
            *OutputBufferLength = bufferLength;
    }
    else
    {
        if (buffer)
            PhFree(buffer);
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
    _In_ PPH_STRINGREF FileName,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;
    SIZE_T fileNameAllocationSize = 0;
    PVOID fileNameBaseAddress = NULL;
    PVOID loadLibraryW = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    if (KphProcessLevel(ProcessHandle) > KphLevelMed)
    {
        return STATUS_ACCESS_DENIED;
    }

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "LoadLibraryW",
        0,
        &loadLibraryW,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    fileNameAllocationSize = FileName->Length + sizeof(UNICODE_NULL);
    status = NtAllocateVirtualMemory(
        ProcessHandle,
        &fileNameBaseAddress,
        0,
        &fileNameAllocationSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWriteVirtualMemory(
        ProcessHandle,
        fileNameBaseAddress,
        FileName->Buffer,
        FileName->Length + sizeof(UNICODE_NULL),
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        0,
        0,
        0,
        0,
        loadLibraryW,
        fileNameBaseAddress,
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, Timeout);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

CleanupExit:
    if (threadHandle)
        NtClose(threadHandle);

    if (powerRequestHandle)
        PhDestroyExecutionRequiredRequest(powerRequestHandle);

    fileNameAllocationSize = 0;
    NtFreeVirtualMemory(
        ProcessHandle,
        &fileNameBaseAddress,
        &fileNameAllocationSize,
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
    NTSTATUS status;
    HANDLE threadHandle;
    HANDLE powerRequestHandle = NULL;
    THREAD_BASIC_INFORMATION basicInfo;
    PVOID threadStart;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // No point trying to set the load count on Windows 8 and higher, because NT now uses a DAG of
    // loader nodes.
    if (WindowsVersion < WINDOWS_8)
    {
#ifdef _WIN64
        BOOLEAN isWow64 = FALSE;
        BOOLEAN isModule32 = FALSE;
#endif
        status = PhSetProcessModuleLoadCount(
            ProcessHandle,
            BaseAddress,
            1
            );

#ifdef _WIN64
        PhGetProcessIsWow64(ProcessHandle, &isWow64);
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

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "LdrUnloadDll",
        0,
        &threadStart,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            return status;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        0,
        0,
        0,
        0,
        threadStart,
        BaseAddress,
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhWaitForSingleObject(threadHandle, Timeout);

    if (status == STATUS_WAIT_0)
    {
        status = PhGetThreadBasicInformation(threadHandle, &basicInfo);

        if (NT_SUCCESS(status))
            status = basicInfo.ExitStatus;
    }

    NtClose(threadHandle);

    if (powerRequestHandle)
        PhDestroyExecutionRequiredRequest(powerRequestHandle);

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
    THREAD_BASIC_INFORMATION basicInformation;
    PVOID nameBaseAddress = NULL;
    PVOID valueBaseAddress = NULL;
    SIZE_T nameAllocationSize = 0;
    SIZE_T valueAllocationSize = 0;
    PVOID rtlExitUserThread = NULL;
    PVOID setEnvironmentVariableW = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    nameAllocationSize = Name->Length + sizeof(UNICODE_NULL);

    if (Value)
        valueAllocationSize = Value->Length + sizeof(UNICODE_NULL);

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        &isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        0,
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "SetEnvironmentVariableW",
        0,
        &setEnvironmentVariableW,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtAllocateVirtualMemory(
        ProcessHandle,
        &nameBaseAddress,
        0,
        &nameAllocationSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWriteVirtualMemory(
        ProcessHandle,
        nameBaseAddress,
        Name->Buffer,
        Name->Length,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (Value)
    {
        status = NtAllocateVirtualMemory(
            ProcessHandle,
            &valueBaseAddress,
            0,
            &valueAllocationSize,
            MEM_COMMIT,
            PAGE_READWRITE
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = NtWriteVirtualMemory(
            ProcessHandle,
            valueBaseAddress,
            Value->Buffer,
            Value->Length,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            setEnvironmentVariableW,
            nameBaseAddress,
            valueBaseAddress,
            NULL
            );
    }
    else
    {
#endif
        status = NtQueueApcThread(
            threadHandle,
            setEnvironmentVariableW,
            nameBaseAddress,
            valueBaseAddress,
            NULL
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL); // Execute the pending APC (dmex)

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWaitForSingleObject(threadHandle, FALSE, Timeout);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetThreadBasicInformation(threadHandle, &basicInformation);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = basicInformation.ExitStatus;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
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

    return status;
}

NTSTATUS PhGetWindowClientId(
    _In_ HWND WindowHandle,
    _Out_ PCLIENT_ID ClientId
    )
{
    ULONG windowProcessId;
    ULONG windowThreadId;

    if (windowThreadId = GetWindowThreadProcessId(WindowHandle, &windowProcessId))
    {
        ClientId->UniqueProcess = UlongToHandle(windowProcessId);
        ClientId->UniqueThread = UlongToHandle(windowThreadId);
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

/**
 * Destroys the specified window in a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_SET_LIMITED_INFORMATION access.
 * \param WindowHandle A handle to the window to be destroyed.
 *
 * \return Successful or errant status.
 *
 * \remarks A thread cannot call DestroyWindow for a window created by a different thread,
 * unless we queue a special APC to the owner thread.
 */
NTSTATUS PhDestroyWindowRemote(
    _In_ HANDLE ProcessHandle,
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;
    PVOID destroyWindow = NULL;
    HANDLE threadId = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->User32FileName,
        "DestroyWindow",
        0,
        &destroyWindow,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    threadId = UlongToHandle(GetWindowThreadProcessId(WindowHandle, NULL));

    if (!threadId)
    {
        status = STATUS_NOT_FOUND;
        goto CleanupExit;
    }

    status = PhOpenThread(
        &threadHandle,
        THREAD_SET_CONTEXT, // THREAD_ALERT
        threadId
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtQueueApcThreadEx(
        threadHandle,
        QUEUE_USER_APC_SPECIAL_USER_APC,
        destroyWindow,
        (PVOID)WindowHandle,
        NULL,
        NULL
        );

    //PostThreadMessage(HandleToUlong(threadId), WM_NULL, 0, 0);
    //NtAlertThread(threadHandle);
    NtClose(threadHandle);

CleanupExit:

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

NTSTATUS PhPostWindowQuitMessageRemote(
    _In_ HANDLE ProcessHandle,
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;
    PVOID postQuitMessage = NULL;
    HANDLE threadId = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->User32FileName,
        "PostQuitMessage",
        0,
        &postQuitMessage,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    threadId = UlongToHandle(GetWindowThreadProcessId(WindowHandle, NULL));

    if (!threadId)
    {
        status = STATUS_NOT_FOUND;
        goto CleanupExit;
    }

    status = PhOpenThread(
        &threadHandle,
        THREAD_SET_CONTEXT, // THREAD_ALERT
        threadId
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtQueueApcThreadEx(
        threadHandle,
        QUEUE_USER_APC_SPECIAL_USER_APC,
        postQuitMessage,
        (PVOID)WindowHandle,
        NULL,
        NULL
        );

    //PostThreadMessage(HandleToUlong(threadId), WM_NULL, 0, 0);
    //NtAlertThread(threadHandle);
    NtClose(threadHandle);

CleanupExit:

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

/*
 * Invokes a procedure in the context of the owning thread for the window.
 *
 * \param WindowHandle The handle of the window.
 * \param ApcRoutine The procedure to be invoked.
 * \param ApcArgument1 The first argument to be passed to the procedure.
 * \param ApcArgument2 The second argument to be passed to the procedure.
 * \param ApcArgument3 The third argument to be passed to the procedure.
 *
 * \return The status of the operation.
 */
NTSTATUS PhInvokeWindowThreadProcedureRemote(
    _In_ HWND WindowHandle,
    _In_ PVOID ApcRoutine,
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    )
{
    NTSTATUS status;
    HANDLE processHande = NULL;
    HANDLE threadHande = NULL;
    HANDLE powerHandle = NULL;
    CLIENT_ID clientId;

    // Get the client ID of the window.

    status = PhGetWindowClientId(WindowHandle, &clientId);

    if (!NT_SUCCESS(status))
        return status;

    // Open the process associated with the window.

    status = PhOpenProcessClientId(
        &processHande,
        PROCESS_ALL_ACCESS,
        clientId
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Open the thread associated with the window.

    if (!NT_SUCCESS(status = PhOpenThread(
        &threadHande,
        THREAD_ALL_ACCESS, // THREAD_ALERT
        clientId.UniqueThread
        )))
    {
        goto CleanupExit;
    }

    // Create an execution required request for the process (Windows 8 and above)

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(processHande, &powerHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    // Queue a Special user-mode APC to execute within the context of the message loop.

    status = NtQueueApcThreadEx(
        threadHande,
        QUEUE_USER_APC_SPECIAL_USER_APC,
        ApcRoutine,
        ApcArgument1,
        ApcArgument2,
        ApcArgument3
        );

CleanupExit:
    if (threadHande)
        NtClose(threadHande);
    if (processHande)
        NtClose(processHande);
    if (powerHandle)
        PhDestroyExecutionRequiredRequest(powerHandle);

    return status;
}

NTSTATUS PhGetJobProcessIdList(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_PROCESS_ID_LIST *ProcessIdList
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;

    do
    {
        buffer = PhAllocate(bufferSize);

        status = NtQueryInformationJobObject(
            JobHandle,
            JobObjectBasicProcessIdList,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
        {
            *ProcessIdList = (PJOBOBJECT_BASIC_PROCESS_ID_LIST)buffer;
        }
        else
        {
            PhFree(buffer);
        }

    } while (status == STATUS_BUFFER_OVERFLOW);

    return status;
}

NTSTATUS PhGetJobBasicAndIoAccounting(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION BasicAndIoAccounting
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
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimits
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
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimits
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
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_UI_RESTRICTIONS BasicUiRestrictions
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
    ULONG bufferSize;
    ULONG returnLength;

    returnLength = 0;
    bufferSize = 0x80;
    buffer = PhAllocate(bufferSize);

    status = NtQueryInformationToken(
        TokenHandle,
        TokenInformationClass,
        buffer,
        bufferSize,
        &returnLength
        );

    if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = NtQueryInformationToken(
            TokenHandle,
            TokenInformationClass,
            buffer,
            bufferSize,
            &returnLength
            );
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
NTSTATUS PhGetTokenUserCopy(
    _In_ HANDLE TokenHandle,
    _Out_ PSID* User
    )
{
    NTSTATUS status;
    ULONG returnLength;
    UCHAR tokenUserBuffer[TOKEN_USER_MAX_SIZE];
    PTOKEN_USER tokenUser = (PTOKEN_USER)tokenUserBuffer;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenUser,
        tokenUser,
        sizeof(tokenUserBuffer),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *User = PhAllocateCopy(tokenUser->User.Sid, PhLengthSid(tokenUser->User.Sid));
    }

    return status;
}

NTSTATUS PhGetTokenUser(
    _In_ HANDLE TokenHandle,
    _Out_ PPH_TOKEN_USER User
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenUser,
        User,
        sizeof(PH_TOKEN_USER), // SE_TOKEN_USER
        &returnLength
        );
}

/**
 * Gets a token's owner.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Owner A variable which receives a pointer to a structure containing the token's owner. You
 * must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhGetTokenOwnerCopy(
    _In_ HANDLE TokenHandle,
    _Out_ PSID* Owner
    )
{
    NTSTATUS status;
    UCHAR tokenOwnerBuffer[TOKEN_OWNER_MAX_SIZE];
    PTOKEN_OWNER tokenOwner = (PTOKEN_OWNER)tokenOwnerBuffer;
    ULONG returnLength;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenOwner,
        tokenOwner,
        sizeof(tokenOwnerBuffer),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *Owner = PhAllocateCopy(tokenOwner->Owner, PhLengthSid(tokenOwner->Owner));
    }

    return status;
}

NTSTATUS PhGetTokenOwner(
    _In_ HANDLE TokenHandle,
    _Out_ PPH_TOKEN_OWNER Owner
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenOwner,
        Owner,
        sizeof(PH_TOKEN_OWNER),
        &returnLength
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
 * Gets a token's discretionary access control list (DACL).
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param DefaultDacl A pointer to an ACL structure assigned by default to any objects created
 * by the user. You must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhGetTokenDefaultDacl(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_DEFAULT_DACL* DefaultDacl
    )
{
    NTSTATUS status;
    PTOKEN_DEFAULT_DACL defaultDacl;

    status = PhQueryTokenVariableSize(
        TokenHandle,
        TokenDefaultDacl,
        &defaultDacl
        );

    if (NT_SUCCESS(status))
    {
        if (defaultDacl->DefaultDacl)
        {
            *DefaultDacl = defaultDacl;
        }
        else
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            PhFree(defaultDacl);
        }
    }

    return status;
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

NTSTATUS PhGetTokenAppContainerSidCopy(
    _In_ HANDLE TokenHandle,
    _Out_ PSID* AppContainerSid
    )
{
    NTSTATUS status;
    UCHAR tokenAppContainerSidBuffer[TOKEN_APPCONTAINER_SID_MAX_SIZE];
    PTOKEN_APPCONTAINER_INFORMATION tokenAppContainerSid = (PTOKEN_APPCONTAINER_INFORMATION)tokenAppContainerSidBuffer;
    ULONG returnLength;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenAppContainerSid,
        tokenAppContainerSid,
        sizeof(tokenAppContainerSidBuffer),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        if (tokenAppContainerSid->TokenAppContainer)
        {
            *AppContainerSid = PhAllocateCopy(tokenAppContainerSid->TokenAppContainer, PhLengthSid(tokenAppContainerSid->TokenAppContainer));
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}

NTSTATUS PhGetTokenAppContainerSid(
    _In_ HANDLE TokenHandle,
    _Out_ PPH_TOKEN_APPCONTAINER AppContainerSid
    )
{
    NTSTATUS status;
    ULONG returnLength;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenAppContainerSid,
        AppContainerSid,
        sizeof(PH_TOKEN_APPCONTAINER),
        &returnLength
        );

    if (NT_SUCCESS(status) && !AppContainerSid->AppContainer.Sid)
    {
        status = STATUS_NOT_FOUND;
    }

    return status;
}

NTSTATUS PhGetTokenSecurityAttributes(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION* SecurityAttributes
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenSecurityAttributes,
        SecurityAttributes
        );
}

NTSTATUS PhGetTokenSecurityAttribute(
    _In_ HANDLE TokenHandle,
    _In_ PPH_STRINGREF AttributeName,
    _Out_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION* SecurityAttributes
    )
{
    NTSTATUS status;
    UNICODE_STRING attributeName;
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION buffer;
    ULONG bufferLength;
    ULONG returnLength;

    if (!PhStringRefToUnicodeString(AttributeName, &attributeName))
        return STATUS_NAME_TOO_LONG;

    returnLength = 0;
    bufferLength = 0x200;
    buffer = PhAllocate(bufferLength);
    memset(buffer, 0, bufferLength);

    status = NtQuerySecurityAttributesToken(
        TokenHandle,
        &attributeName,
        1,
        buffer,
        bufferLength,
        &returnLength
        );

    if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
    {
        bufferLength = returnLength;
        buffer = PhAllocate(bufferLength);
        memset(buffer, 0, bufferLength);

        status = NtQuerySecurityAttributesToken(
            TokenHandle,
            &attributeName,
            1,
            buffer,
            bufferLength,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        if (returnLength == sizeof(TOKEN_SECURITY_ATTRIBUTES_INFORMATION))
        {
            PhFree(buffer);
            return STATUS_NOT_FOUND;
        }

        *SecurityAttributes = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

BOOLEAN PhDoesTokenSecurityAttributeExist(
    _In_ HANDLE TokenHandle,
    _In_ PPH_STRINGREF AttributeName
    )
{
    NTSTATUS status;
    UNICODE_STRING attributeName;
    ULONG returnLength;

    if (!PhStringRefToUnicodeString(AttributeName, &attributeName))
        return FALSE;

    status = NtQuerySecurityAttributesToken(
        TokenHandle,
        &attributeName,
        1,
        NULL,
        0,
        &returnLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
        return TRUE;

    return FALSE;
}

PTOKEN_SECURITY_ATTRIBUTE_V1 PhFindTokenSecurityAttributeName(
    _In_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION Attributes,
    _In_ PPH_STRINGREF AttributeName
    )
{
    for (ULONG i = 0; i < Attributes->AttributeCount; i++)
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = &Attributes->Attribute.pAttributeV1[i];
        PH_STRINGREF attributeName;

        PhUnicodeStringToStringRef(&attribute->Name, &attributeName);

        if (PhEqualStringRef(&attributeName, AttributeName, FALSE))
        {
            return attribute;
        }
    }

    return NULL;
}

BOOLEAN PhGetTokenIsFullTrustPackage(
    _In_ HANDLE TokenHandle
    )
{
    static PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://SYSAPPID");
    BOOLEAN tokenIsAppContainer = FALSE;

    if (NT_SUCCESS(PhDoesTokenSecurityAttributeExist(TokenHandle, &attributeName)))
    {
        if (NT_SUCCESS(PhGetTokenIsAppContainer(TokenHandle, &tokenIsAppContainer)))
        {
            if (tokenIsAppContainer)
                return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

NTSTATUS PhGetProcessIsStronglyNamed(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsStronglyNamed
    )
{
    NTSTATUS status;
    PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

    status = PhGetProcessExtendedBasicInformation(ProcessHandle, &basicInfo);

    if (NT_SUCCESS(status))
    {
        *IsStronglyNamed = !!basicInfo.IsStronglyNamed;
    }

    return status;
}

BOOLEAN PhGetProcessIsFullTrustPackage(
    _In_ HANDLE ProcessHandle
    )
{
    BOOLEAN processIsStronglyNamed = FALSE;
    BOOLEAN tokenIsAppContainer = FALSE;

    if (NT_SUCCESS(PhGetProcessIsStronglyNamed(ProcessHandle, &processIsStronglyNamed)))
    {
        if (processIsStronglyNamed)
        {
            HANDLE tokenHandle;

            if (NT_SUCCESS(PhOpenProcessToken(ProcessHandle, TOKEN_QUERY, &tokenHandle)))
            {
                PhGetTokenIsAppContainer(tokenHandle, &tokenIsAppContainer);
                NtClose(tokenHandle);
            }

            if (tokenIsAppContainer)
                return FALSE;

            return TRUE;
        }
    }

    return FALSE;
}

// rev from PackageIdFromFullName (dmex)
PPH_STRING PhGetProcessPackageFullName(
    _In_ HANDLE ProcessHandle
    )
{
    HANDLE tokenHandle;
    PPH_STRING packageName = NULL;

    if (NT_SUCCESS(PhOpenProcessToken(ProcessHandle, TOKEN_QUERY, &tokenHandle)))
    {
        packageName = PhGetTokenPackageFullName(tokenHandle);
        NtClose(tokenHandle);
    }

    return packageName;
}

NTSTATUS PhGetTokenIsLessPrivilegedAppContainer(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsLessPrivilegedAppContainer
    )
{
    static PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://NOALLAPPPKG");

    if (PhDoesTokenSecurityAttributeExist(TokenHandle, &attributeName))
        *IsLessPrivilegedAppContainer = TRUE;
    else
        *IsLessPrivilegedAppContainer = FALSE;

    // TODO: NtQueryInformationToken(TokenIsLessPrivilegedAppContainer);

    return STATUS_SUCCESS;
}

ULONG64 PhGetTokenSecurityAttributeValueUlong64(
    _In_ HANDLE TokenHandle,
    _In_ PPH_STRINGREF Name,
    _In_ ULONG ValueIndex
    )
{
    ULONG64 value = MAXULONG64;
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;

    if (NT_SUCCESS(PhGetTokenSecurityAttribute(TokenHandle, Name, &info)))
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = PhFindTokenSecurityAttributeName(info, Name);

        if (attribute && attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_UINT64 && ValueIndex < attribute->ValueCount)
        {
            value = attribute->Values.pUint64[ValueIndex];
        }

        PhFree(info);
    }

    return value;
}

PPH_STRING PhGetTokenSecurityAttributeValueString(
    _In_ HANDLE TokenHandle,
    _In_ PPH_STRINGREF Name,
    _In_ ULONG ValueIndex
    )
{
    PPH_STRING value = NULL;
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;

    if (NT_SUCCESS(PhGetTokenSecurityAttribute(TokenHandle, Name, &info)))
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = PhFindTokenSecurityAttributeName(info, Name);

        if (attribute && attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING && ValueIndex < attribute->ValueCount)
        {
            value = PhCreateStringFromUnicodeString(&attribute->Values.pString[ValueIndex]);
        }

        PhFree(info);
    }

    return value;
}

// rev from GetApplicationUserModelId/GetApplicationUserModelIdFromToken (dmex)
PPH_STRING PhGetTokenPackageApplicationUserModelId(
    _In_ HANDLE TokenHandle
    )
{
    static PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://SYSAPPID");
    static PH_STRINGREF seperator = PH_STRINGREF_INIT(L"!");
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;
    PPH_STRING applicationUserModelId = NULL;

    if (NT_SUCCESS(PhGetTokenSecurityAttribute(TokenHandle, &attributeName, &info)))
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = PhFindTokenSecurityAttributeName(info, &attributeName);

        if (attribute && attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING && attribute->ValueCount >= 3)
        {
            PPH_STRING relativeIdName;
            PPH_STRING packageFamilyName;

            relativeIdName = PhCreateStringFromUnicodeString(&attribute->Values.pString[1]);
            packageFamilyName = PhCreateStringFromUnicodeString(&attribute->Values.pString[2]);

            applicationUserModelId = PhConcatStringRef3(
                &packageFamilyName->sr,
                &seperator,
                &relativeIdName->sr
                );

            PhDereferenceObject(packageFamilyName);
            PhDereferenceObject(relativeIdName);
        }

        PhFree(info);
    }

    return applicationUserModelId;
}

PPH_STRING PhGetTokenPackageFullName(
    _In_ HANDLE TokenHandle
    )
{
    static PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://SYSAPPID");
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;
    PPH_STRING packageFullName = NULL;

    if (NT_SUCCESS(PhGetTokenSecurityAttribute(TokenHandle, &attributeName, &info)))
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = PhFindTokenSecurityAttributeName(info, &attributeName);

        if (attribute && attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING)
        {
            packageFullName = PhCreateStringFromUnicodeString(&attribute->Values.pString[0]);
        }

        PhFree(info);
    }

    return packageFullName;
}

NTSTATUS PhGetTokenNamedObjectPath(
    _In_ HANDLE TokenHandle,
    _In_opt_ PSID Sid,
    _Out_ PPH_STRING* ObjectPath
    )
{
    NTSTATUS status;
    UNICODE_STRING objectPath;

    if (!RtlGetTokenNamedObjectPath_Import())
        return STATUS_NOT_SUPPORTED;

    RtlInitEmptyUnicodeString(&objectPath, NULL, 0);

    status = RtlGetTokenNamedObjectPath_Import()(
        TokenHandle,
        Sid,
        &objectPath
        );

    if (NT_SUCCESS(status))
    {
        *ObjectPath = PhCreateStringFromUnicodeString(&objectPath);
        RtlFreeUnicodeString(&objectPath);
    }

    return status;
}

NTSTATUS PhGetAppContainerNamedObjectPath(
    _In_ HANDLE TokenHandle,
    _In_opt_ PSID AppContainerSid,
    _In_ BOOLEAN RelativePath,
    _Out_ PPH_STRING* ObjectPath
    )
{
    NTSTATUS status;
    UNICODE_STRING objectPath;

    if (!RtlGetAppContainerNamedObjectPath_Import())
        return STATUS_UNSUCCESSFUL;

    RtlInitEmptyUnicodeString(&objectPath, NULL, 0);

    status = RtlGetAppContainerNamedObjectPath_Import()(
        TokenHandle,
        AppContainerSid,
        RelativePath,
        &objectPath
        );

    if (NT_SUCCESS(status))
    {
        *ObjectPath = PhCreateStringFromUnicodeString(&objectPath);
        RtlFreeUnicodeString(&objectPath);
    }

    return status;
}

BOOLEAN PhPrivilegeCheck(
    _In_ HANDLE TokenHandle,
    _In_ ULONG Privilege
    )
{
    CHAR privilegesBuffer[FIELD_OFFSET(PRIVILEGE_SET, Privilege) + sizeof(LUID_AND_ATTRIBUTES) * 1];
    PPRIVILEGE_SET requiredPrivileges;
    BOOLEAN result = FALSE;

    requiredPrivileges = (PPRIVILEGE_SET)privilegesBuffer;
    requiredPrivileges->PrivilegeCount = 1;
    requiredPrivileges->Control = PRIVILEGE_SET_ALL_NECESSARY;
    requiredPrivileges->Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
    requiredPrivileges->Privilege[0].Luid = RtlConvertUlongToLuid(Privilege);

    NtPrivilegeCheck(TokenHandle, requiredPrivileges, &result);

    return result;
}

BOOLEAN PhPrivilegeCheckAny(
    _In_ HANDLE TokenHandle,
    _In_ ULONG Privilege
    )
{
    CHAR privilegesBuffer[FIELD_OFFSET(PRIVILEGE_SET, Privilege) + sizeof(LUID_AND_ATTRIBUTES) * 1];
    PPRIVILEGE_SET requiredPrivileges;
    BOOLEAN result = FALSE;

    requiredPrivileges = (PPRIVILEGE_SET)privilegesBuffer;
    requiredPrivileges->PrivilegeCount = 1;
    requiredPrivileges->Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
    requiredPrivileges->Privilege[0].Luid = RtlConvertUlongToLuid(Privilege);

    NtPrivilegeCheck(TokenHandle, requiredPrivileges, &result);

    if (requiredPrivileges->Privilege[0].Attributes == SE_PRIVILEGE_USED_FOR_ACCESS)
        return TRUE;

    return FALSE;
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
    _In_opt_ PCWSTR PrivilegeName,
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

        PhInitializeStringRefLongHint(&privilegeName, PrivilegeName);

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

NTSTATUS PhAdjustPrivilege(
    _In_opt_ PCWSTR PrivilegeName,
    _In_opt_ LONG Privilege,
    _In_ BOOLEAN Enable
    )
{
    NTSTATUS status;
    HANDLE tokenHandle;
    TOKEN_PRIVILEGES privileges;

    status = NtOpenProcessToken(
        NtCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES,
        &tokenHandle
        );

    if (!NT_SUCCESS(status))
        return status;

    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    if (Privilege)
    {
        LUID privilegeLuid;

        privilegeLuid = RtlConvertLongToLuid(Privilege);

        privileges.Privileges[0].Luid = privilegeLuid;
    }
    else if (PrivilegeName)
    {
        PH_STRINGREF privilegeName;

        PhInitializeStringRefLongHint(&privilegeName, PrivilegeName);

        if (!PhLookupPrivilegeValue(
            &privilegeName,
            &privileges.Privileges[0].Luid
            ))
        {
            NtClose(tokenHandle);
            return STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        NtClose(tokenHandle);
        return STATUS_INVALID_PARAMETER_1;
    }

    status = NtAdjustPrivilegesToken(
        tokenHandle,
        FALSE,
        &privileges,
        0,
        NULL,
        NULL
        );

    NtClose(tokenHandle);

    if (status == STATUS_NOT_ALL_ASSIGNED)
        return STATUS_PRIVILEGE_NOT_HELD;

    return status;
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
    _In_opt_ PCWSTR GroupName,
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

        PhInitializeStringRefLongHint(&groupName, GroupName);

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
    UCHAR mandatoryLabelBuffer[TOKEN_INTEGRITY_LEVEL_MAX_SIZE];
    PTOKEN_MANDATORY_LABEL mandatoryLabel = (PTOKEN_MANDATORY_LABEL)mandatoryLabelBuffer;
    ULONG returnLength;
    ULONG subAuthoritiesCount;
    ULONG subAuthority;
    PWSTR integrityString;
    BOOLEAN tokenIsAppContainer;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenIntegrityLevel,
        mandatoryLabel,
        sizeof(mandatoryLabelBuffer),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    subAuthoritiesCount = *PhSubAuthorityCountSid(mandatoryLabel->Label.Sid);

    if (subAuthoritiesCount > 0)
    {
        subAuthority = *PhSubAuthoritySid(mandatoryLabel->Label.Sid, subAuthoritiesCount - 1);
    }
    else
    {
        subAuthority = SECURITY_MANDATORY_UNTRUSTED_RID;
    }

    if (IntegrityString)
    {
        if (NT_SUCCESS(PhGetTokenIsAppContainer(TokenHandle, &tokenIsAppContainer)) && tokenIsAppContainer)
        {
            integrityString = L"AppContainer";
        }
        else
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
        case SECURITY_MANDATORY_MEDIUM_PLUS_RID:
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

NTSTATUS PhGetProcessMandatoryPolicy(
    _In_ HANDLE ProcessHandle,
    _Out_ PACCESS_MASK Mask
    )
{
    NTSTATUS status;
    BOOLEAN found = FALSE;
    ACCESS_MASK currentMask = 0;
    PSYSTEM_MANDATORY_LABEL_ACE currentAce;
    PSECURITY_DESCRIPTOR currentSecurityDescriptor;
    BOOLEAN currentSaclPresent;
    BOOLEAN currentSaclDefaulted;
    PACL currentSacl;

    status = PhGetObjectSecurity(
        ProcessHandle,
        LABEL_SECURITY_INFORMATION,
        &currentSecurityDescriptor
        );

    if (!NT_SUCCESS(status))
        return status;

    status = RtlGetSaclSecurityDescriptor(
        currentSecurityDescriptor,
        &currentSaclPresent,
        &currentSacl,
        &currentSaclDefaulted
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!(currentSaclPresent && currentSacl))
        goto CleanupExit;

    for (USHORT i = 0; i < currentSacl->AceCount; i++)
    {
        status = RtlGetAce(currentSacl, i, &currentAce);

        if (!NT_SUCCESS(status))
            break;

        if (currentAce->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
        {
            currentMask = currentAce->Mask;
            found = TRUE;
            break;
        }
    }

CleanupExit:
    PhFree(currentSecurityDescriptor);

    if (NT_SUCCESS(status))
    {
        if (found)
        {
            *Mask = currentMask;
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}

NTSTATUS PhSetProcessMandatoryPolicy(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK Mask
    )
{
    NTSTATUS status;
    PSYSTEM_MANDATORY_LABEL_ACE currentAce;
    PSECURITY_DESCRIPTOR currentSecurityDescriptor;
    BOOLEAN currentSaclPresent;
    BOOLEAN currentSaclDefaulted;
    PACL currentSacl;

    status = PhGetObjectSecurity(
        ProcessHandle,
        LABEL_SECURITY_INFORMATION,
        &currentSecurityDescriptor
        );

    if (!NT_SUCCESS(status))
        return status;

    status = RtlGetSaclSecurityDescriptor(
        currentSecurityDescriptor,
        &currentSaclPresent,
        &currentSacl,
        &currentSaclDefaulted
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = STATUS_UNSUCCESSFUL;

    if (!(currentSaclPresent && currentSacl))
        goto CleanupExit;

    for (USHORT i = 0; i < currentSacl->AceCount; i++)
    {
        status = RtlGetAce(currentSacl, i, &currentAce);

        if (!NT_SUCCESS(status))
            break;

        if (currentAce->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
        {
            currentAce->Mask = Mask;

            status = PhSetObjectSecurity(
                ProcessHandle,
                LABEL_SECURITY_INFORMATION,
                currentSecurityDescriptor
                );
            break;
        }
    }

CleanupExit:
    PhFree(currentSecurityDescriptor);

    return status;
}

NTSTATUS PhGetTokenProcessTrustLevelRID(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PULONG ProtectionType,
    _Out_opt_ PULONG ProtectionLevel,
    _Out_opt_ PPH_STRING* TrustLevelString,
    _Out_opt_ PPH_STRING* TrustLevelSidString
    )
{
    NTSTATUS status;
    PTOKEN_PROCESS_TRUST_LEVEL trustLevel;
    ULONG subAuthoritiesCount;
    ULONG protectionType;
    ULONG protectionLevel;

    status = PhGetTokenTrustLevel(TokenHandle, &trustLevel);

    if (!NT_SUCCESS(status))
        return status;

    if (!trustLevel->TrustLevelSid)
        return STATUS_UNSUCCESSFUL;

    if (!PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(trustLevel->TrustLevelSid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_PROCESS_TRUST_AUTHORITY))
        return STATUS_INVALID_SUB_AUTHORITY;

    subAuthoritiesCount = *PhSubAuthorityCountSid(trustLevel->TrustLevelSid);

    if (subAuthoritiesCount == SECURITY_PROCESS_TRUST_AUTHORITY_RID_COUNT)
    {
        protectionType = *PhSubAuthoritySid(trustLevel->TrustLevelSid, 0);
        protectionLevel = *PhSubAuthoritySid(trustLevel->TrustLevelSid, 1);
    }
    else
    {
        protectionType = SECURITY_PROCESS_PROTECTION_TYPE_NONE_RID;
        protectionLevel = SECURITY_PROCESS_PROTECTION_LEVEL_NONE_RID;
    }

    if (ProtectionType)
        *ProtectionType = protectionType;
    if (ProtectionLevel)
        *ProtectionLevel = protectionLevel;

    if (TrustLevelString)
    {
        static PH_STRINGREF ProtectionTypeString[] =
        {
            PH_STRINGREF_INIT(L"None"),
            PH_STRINGREF_INIT(L"Full"),
            PH_STRINGREF_INIT(L"Lite"),
        };
        static PH_STRINGREF ProtectionLevelString[] =
        {
            PH_STRINGREF_INIT(L" (None)"),
            PH_STRINGREF_INIT(L" (WinTcb)"),
            PH_STRINGREF_INIT(L" (Windows)"),
            PH_STRINGREF_INIT(L" (StoreApp)"),
            PH_STRINGREF_INIT(L" (Antimalware)"),
            PH_STRINGREF_INIT(L" (Authenticode)"),
        };
        PPH_STRINGREF protectionTypeString = NULL;
        PPH_STRINGREF protectionLevelString = NULL;

        switch (protectionType)
        {
        case SECURITY_PROCESS_PROTECTION_TYPE_NONE_RID:
            protectionTypeString = &ProtectionTypeString[0];
            break;
        case SECURITY_PROCESS_PROTECTION_TYPE_FULL_RID:
            protectionTypeString = &ProtectionTypeString[1];
            break;
        case SECURITY_PROCESS_PROTECTION_TYPE_LITE_RID:
            protectionTypeString = &ProtectionTypeString[2];
            break;
        }

        switch (protectionLevel)
        {
        case SECURITY_PROCESS_PROTECTION_LEVEL_NONE_RID:
            protectionLevelString = &ProtectionLevelString[0];
            break;
        case SECURITY_PROCESS_PROTECTION_LEVEL_WINTCB_RID:
            protectionLevelString = &ProtectionLevelString[1];
            break;
        case SECURITY_PROCESS_PROTECTION_LEVEL_WINDOWS_RID:
            protectionLevelString = &ProtectionLevelString[2];
            break;
        case SECURITY_PROCESS_PROTECTION_LEVEL_APP_RID:
            protectionLevelString = &ProtectionLevelString[3];
            break;
        case SECURITY_PROCESS_PROTECTION_LEVEL_ANTIMALWARE_RID:
            protectionLevelString = &ProtectionLevelString[4];
            break;
        case SECURITY_PROCESS_PROTECTION_LEVEL_AUTHENTICODE_RID:
            protectionLevelString = &ProtectionLevelString[5];
            break;
        }

        if (protectionTypeString && protectionLevelString)
            *TrustLevelString = PhConcatStringRef2(protectionTypeString, protectionLevelString);
        else
            *TrustLevelString = PhCreateStringZ(L"Unknown");
    }

    if (TrustLevelSidString)
    {
        *TrustLevelSidString = PhSidToStringSid(trustLevel->TrustLevelSid);
    }

    PhFree(trustLevel);

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

NTSTATUS PhGetFileBasicInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_BASIC_INFORMATION BasicInfo
    )
{
    IO_STATUS_BLOCK isb;

    return NtQueryInformationFile(
        FileHandle,
        &isb,
        BasicInfo,
        sizeof(FILE_BASIC_INFORMATION),
        FileBasicInformation
        );
}

NTSTATUS PhSetFileBasicInformation(
    _In_ HANDLE FileHandle,
    _In_ PFILE_BASIC_INFORMATION BasicInfo
    )
{
    IO_STATUS_BLOCK isb;

    return NtSetInformationFile(
        FileHandle,
        &isb,
        BasicInfo,
        sizeof(FILE_BASIC_INFORMATION),
        FileBasicInformation
        );
}

NTSTATUS PhGetFileFullAttributesInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    FILE_NETWORK_OPEN_INFORMATION fullAttributesInfo;

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        &fullAttributesInfo,
        sizeof(FILE_NETWORK_OPEN_INFORMATION),
        FileNetworkOpenInformation
        );

    if (NT_SUCCESS(status))
    {
        *FileInformation = fullAttributesInfo;
    }

    return status;
}

NTSTATUS PhGetFileStandardInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_STANDARD_INFORMATION StandardInfo
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    FILE_STANDARD_INFORMATION standardInfo;

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        &standardInfo,
        sizeof(FILE_STANDARD_INFORMATION),
        FileStandardInformation
        );

    if (NT_SUCCESS(status))
    {
        *StandardInfo = standardInfo;
    }

    return status;
}

// rev from SetFileCompletionNotificationModes (dmex)
NTSTATUS PhSetFileCompletionNotificationMode(
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags
    )
{
    FILE_IO_COMPLETION_NOTIFICATION_INFORMATION completionMode;
    IO_STATUS_BLOCK isb;

    completionMode.Flags = Flags;

    return NtSetInformationFile(
        FileHandle,
        &isb,
        &completionMode,
        sizeof(FILE_IO_COMPLETION_NOTIFICATION_INFORMATION),
        FileIoCompletionNotificationInformation
        );
}

NTSTATUS PhGetFileSize(
    _In_ HANDLE FileHandle,
    _Out_ PLARGE_INTEGER Size
    )
{
    NTSTATUS status;
    FILE_STANDARD_INFORMATION standardInfo;

    status = PhGetFileStandardInformation(
        FileHandle,
        &standardInfo
        );

    if (NT_SUCCESS(status))
    {
        *Size = standardInfo.EndOfFile;
    }

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

NTSTATUS PhGetFilePosition(
    _In_ HANDLE FileHandle,
    _Out_ PLARGE_INTEGER Position
    )
{
    NTSTATUS status;
    FILE_POSITION_INFORMATION positionInfo;
    IO_STATUS_BLOCK isb;

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        &positionInfo,
        sizeof(FILE_POSITION_INFORMATION),
        FilePositionInformation
        );

    if (!NT_SUCCESS(status))
        return status;

    *Position = positionInfo.CurrentByteOffset;

    return status;
}

NTSTATUS PhSetFilePosition(
    _In_ HANDLE FileHandle,
    _In_opt_ PLARGE_INTEGER Position
    )
{
    FILE_POSITION_INFORMATION positionInfo;
    IO_STATUS_BLOCK isb;

    if (Position)
        positionInfo.CurrentByteOffset.QuadPart = Position->QuadPart;
    else
        positionInfo.CurrentByteOffset.QuadPart = 0;

    return NtSetInformationFile(
        FileHandle,
        &isb,
        &positionInfo,
        sizeof(FILE_POSITION_INFORMATION),
        FilePositionInformation
        );
}

NTSTATUS PhGetFileAllocationSize(
    _In_ HANDLE FileHandle,
    _Out_ PLARGE_INTEGER AllocationSize
    )
{
    NTSTATUS status;
    FILE_ALLOCATION_INFORMATION allocationInfo;
    IO_STATUS_BLOCK isb;

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        &allocationInfo,
        sizeof(FILE_ALLOCATION_INFORMATION),
        FileAllocationInformation
        );

    if (!NT_SUCCESS(status))
        return status;

    *AllocationSize = allocationInfo.AllocationSize;

    return status;
}

NTSTATUS PhSetFileAllocationSize(
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER AllocationSize
    )
{
    FILE_ALLOCATION_INFORMATION allocationInfo;
    IO_STATUS_BLOCK isb;

    allocationInfo.AllocationSize = *AllocationSize;

    return NtSetInformationFile(
        FileHandle,
        &isb,
        &allocationInfo,
        sizeof(FILE_ALLOCATION_INFORMATION),
        FileAllocationInformation
        );
}

NTSTATUS PhGetFileIndexNumber(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_INTERNAL_INFORMATION IndexNumber
    )
{
    IO_STATUS_BLOCK isb;

    return NtQueryInformationFile(
        FileHandle,
        &isb,
        IndexNumber,
        sizeof(FILE_INTERNAL_INFORMATION),
        FileInternalInformation
        );
}

NTSTATUS PhGetFileIsRemoteDevice(
    _In_ HANDLE FileHandle,
    _Out_ PBOOLEAN FileIsRemoteDevice
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_IS_REMOTE_DEVICE_INFORMATION fileRemoteInfo;

    status = NtQueryInformationFile(
        FileHandle,
        &ioStatusBlock,
        &fileRemoteInfo,
        sizeof(FILE_IS_REMOTE_DEVICE_INFORMATION),
        FileIsRemoteDeviceInformation
        );

    if (NT_SUCCESS(status))
    {
        *FileIsRemoteDevice = !!fileRemoteInfo.IsRemote;
    }

    return status;
}

NTSTATUS PhSetFileDelete(
    _In_ HANDLE FileHandle
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    if (WindowsVersion >= WINDOWS_10_RS5)
    {
        FILE_DISPOSITION_INFO_EX dispositionInfo;
        IO_STATUS_BLOCK ioStatusBlock;

        dispositionInfo.Flags = FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_POSIX_SEMANTICS | FILE_DISPOSITION_FLAG_IGNORE_READONLY_ATTRIBUTE;
        status = NtSetInformationFile(
            FileHandle,
            &ioStatusBlock,
            &dispositionInfo,
            sizeof(FILE_DISPOSITION_INFO_EX),
            FileDispositionInformationEx
            );
    }

    if (!NT_SUCCESS(status))
    {
        FILE_DISPOSITION_INFORMATION dispositionInfo;
        IO_STATUS_BLOCK ioStatusBlock;

        dispositionInfo.DeleteFile = TRUE;
        status = NtSetInformationFile(
            FileHandle,
            &ioStatusBlock,
            &dispositionInfo,
            sizeof(FILE_DISPOSITION_INFORMATION),
            FileDispositionInformation
            );
    }

    if (!NT_SUCCESS(status))
    {
        HANDLE deleteHandle;

        if (NT_SUCCESS(PhReOpenFile(
            &deleteHandle,
            FileHandle,
            DELETE,
            FILE_SHARE_DELETE,
            FILE_DELETE_ON_CLOSE
            )))
        {
            NtClose(deleteHandle);
            status = STATUS_SUCCESS;
        }
    }

    return status;
}

NTSTATUS PhSetFileRename(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE RootDirectory,
    _In_ BOOLEAN ReplaceIfExists,
    _In_ PPH_STRINGREF NewFileName
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG renameInfoLength;

    if (WindowsVersion < WINDOWS_10_RS1)
    {
        PFILE_RENAME_INFORMATION renameInfo;

        renameInfoLength = sizeof(FILE_RENAME_INFORMATION) + (ULONG)NewFileName->Length + sizeof(UNICODE_NULL);
        renameInfo = _malloca(renameInfoLength); if (!renameInfo) return STATUS_NO_MEMORY;
        renameInfo->ReplaceIfExists = ReplaceIfExists;
        renameInfo->RootDirectory = RootDirectory;
        renameInfo->FileNameLength = (ULONG)NewFileName->Length;
        memcpy(renameInfo->FileName, NewFileName->Buffer, NewFileName->Length);

        status = NtSetInformationFile(
            FileHandle,
            &ioStatusBlock,
            renameInfo,
            renameInfoLength,
            FileRenameInformation
            );

        _freea(renameInfo);
    }
    else
    {
        PFILE_RENAME_INFORMATION_EX renameInfo;

        renameInfoLength = sizeof(FILE_RENAME_INFORMATION_EX) + (ULONG)NewFileName->Length + sizeof(UNICODE_NULL);
        renameInfo = _malloca(renameInfoLength); if (!renameInfo) return STATUS_NO_MEMORY;
        renameInfo->Flags = FILE_RENAME_POSIX_SEMANTICS | FILE_RENAME_IGNORE_READONLY_ATTRIBUTE | (ReplaceIfExists ? FILE_RENAME_REPLACE_IF_EXISTS : 0);
        renameInfo->RootDirectory = RootDirectory;
        renameInfo->FileNameLength = (ULONG)NewFileName->Length;
        memcpy(renameInfo->FileName, NewFileName->Buffer, NewFileName->Length);

        status = NtSetInformationFile(
            FileHandle,
            &ioStatusBlock,
            renameInfo,
            renameInfoLength,
            FileRenameInformationEx
            );

        _freea(renameInfo);
    }

    return status;
}

/**
 * Flushes the buffers of a file.
 * - Write any modified data for the given file from the Windows in-memory cache.
 * - Commit all pending metadata changes for the given file from the Windows in-memory cache.
 * - Send a SYNC command to the storage device to commit in-memory cache to persistent storage.
 * @param FileHandle Handle to the file.
 * @return NTSTATUS Status code.
 */
NTSTATUS PhFlushBuffersFile(
    _In_ HANDLE FileHandle
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    //if (WindowsVersion >= WINDOWS_8)
    //{
    //    status = NtFlushBuffersFileEx(volumeHandle, 0, 0, 0, &ioStatusBlock);
    //}
    //else
    {
        status = NtFlushBuffersFile(FileHandle, &ioStatusBlock);
    }

    return status;
}

NTSTATUS PhGetFileHandleName(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    ULONG bufferSize;
    PFILE_NAME_INFORMATION buffer;
    IO_STATUS_BLOCK isb;

    bufferSize = sizeof(FILE_NAME_INFORMATION) + MAX_PATH;
    buffer = PhAllocateZero(bufferSize);

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        buffer,
        bufferSize,
        FileNameInformation
        );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        bufferSize = sizeof(FILE_NAME_INFORMATION) + buffer->FileNameLength + sizeof(UNICODE_NULL);
        PhFree(buffer);
        buffer = PhAllocateZero(bufferSize);

        status = NtQueryInformationFile(
            FileHandle,
            &isb,
            buffer,
            bufferSize,
            FileNameInformation
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *FileName = PhCreateStringEx(buffer->FileName, buffer->FileNameLength);
    PhFree(buffer);

    return status;
}

NTSTATUS PhGetFileNetworkPhysicalName(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING* FileName
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG bufferLength;
    PFILE_NETWORK_PHYSICAL_NAME_INFORMATION buffer;

    bufferLength = UFIELD_OFFSET(FILE_NETWORK_PHYSICAL_NAME_INFORMATION, FileName[DOS_MAX_PATH_LENGTH]) + sizeof(UNICODE_NULL);
    buffer = PhAllocate(bufferLength);

    status = NtQueryInformationFile(
        FileHandle,
        &ioStatusBlock,
        buffer,
        bufferLength,
        FileNetworkPhysicalNameInformation
        );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        bufferLength = sizeof(FILE_NETWORK_PHYSICAL_NAME_INFORMATION) + buffer->FileNameLength;
        PhFree(buffer);
        buffer = PhAllocate(bufferLength);

        status = NtQueryInformationFile(
            FileHandle,
            &ioStatusBlock,
            buffer,
            bufferLength,
            FileNetworkPhysicalNameInformation
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *FileName = PhCreateStringEx(buffer->FileName, buffer->FileNameLength);
    PhFree(buffer);

    return status;
}

NTSTATUS PhGetFileAllInformation(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_ALL_INFORMATION *FileInformation
    )
{
    return PhpQueryFileVariableSize(
        FileHandle,
        FileAllInformation,
        FileInformation
        );
}

NTSTATUS PhGetFileId(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_ID_INFORMATION FileId
    )
{
    IO_STATUS_BLOCK isb;

    return NtQueryInformationFile(
        FileHandle,
        &isb,
        FileId,
        sizeof(FILE_ID_INFORMATION),
        FileIdInformation
        );
}

NTSTATUS PhGetProcessIdsUsingFile(
    _In_ HANDLE FileHandle,
    _Out_ PFILE_PROCESS_IDS_USING_FILE_INFORMATION *ProcessIdsUsingFile
    )
{
    return PhpQueryFileVariableSize(
        FileHandle,
        FileProcessIdsUsingFileInformation,
        ProcessIdsUsingFile
        );
}

NTSTATUS PhGetFileUsn(
    _In_ HANDLE FileHandle,
    _Out_ PLONGLONG Usn
    )
{
    NTSTATUS status;
    ULONG recordLength;
    PUSN_RECORD_V2 recordBuffer; // USN_RECORD_UNION
    UCHAR buffer[sizeof(USN_RECORD_V2) + MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR)];
    IO_STATUS_BLOCK isb;

    recordLength = sizeof(buffer);
    recordBuffer = (PUSN_RECORD_V2)buffer;

    status = NtFsControlFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_READ_FILE_USN_DATA, // FSCTL_WRITE_USN_CLOSE_RECORD
        NULL, // READ_FILE_USN_DATA
        0,
        recordBuffer,
        recordLength
        );

    if (NT_SUCCESS(status))
    {
        *Usn = recordBuffer->Usn;

        //switch (recordBuffer->Header.MajorVersion)
        //{
        //case 2:
        //    *Usn = recordBuffer->V2.Usn;
        //    break;
        //case 3:
        //    *Usn = recordBuffer->V3.Usn;
        //    break;
        //case 4:
        //    *Usn = recordBuffer->V4.Usn;
        //    break;
        //}
    }

    return status;
}

NTSTATUS PhSetFileBypassIO(
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN Enable
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    FS_BPIO_INPUT bypassIoInput;
    FS_BPIO_OUTPUT bypassIoOutput;

    // https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/bypassio
    memset(&bypassIoInput, 0, sizeof(FS_BPIO_INPUT));
    bypassIoInput.Operation = Enable ? FS_BPIO_OP_ENABLE : FS_BPIO_OP_DISABLE;
    memset(&bypassIoOutput, 0, sizeof(FS_BPIO_OUTPUT));

    status = NtFsControlFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        FSCTL_MANAGE_BYPASS_IO,
        &bypassIoInput,
        sizeof(bypassIoInput),
        &bypassIoOutput,
        sizeof(bypassIoOutput)
        );

#ifdef DEBUG
    if (bypassIoOutput.OutFlags != FSBPIO_OUTFL_COMPATIBLE_STORAGE_DRIVER) // NT_SUCCESS(bypassIoOutput.Enable.OpStatus)
    {
        dprintf("BypassIO failed: (%S) %S\n", bypassIoOutput.Enable.FailingDriverName, bypassIoOutput.Enable.FailureReason);
    }
#endif

    return status;
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
    memset(BasicInformation, 0, sizeof(TRANSACTIONMANAGER_BASIC_INFORMATION));

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

    if (logPathInfo->LogPathLength == 0)
    {
        *LogFileName = PhReferenceEmptyString();
    }
    else
    {
        *LogFileName = PhCreateStringEx(
            logPathInfo->LogPath,
            logPathInfo->LogPathLength
            );
    }

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
    memset(BasicInformation, 0, sizeof(TRANSACTION_BASIC_INFORMATION));

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
        if (basicInfo->DescriptionLength == 0)
        {
            *Description = PhReferenceEmptyString();
        }
        else
        {
            *Description = PhCreateStringEx(
                basicInfo->Description,
                basicInfo->DescriptionLength
                );
        }
    }

    PhFree(basicInfo);

    return status;
}

NTSTATUS PhGetEnlistmentBasicInformation(
    _In_ HANDLE EnlistmentHandle,
    _Out_ PENLISTMENT_BASIC_INFORMATION BasicInformation
    )
{
    memset(BasicInformation, 0, sizeof(ENLISTMENT_BASIC_INFORMATION));

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
    KPH_DRIVER_BASIC_INFORMATION basicInfo;

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
        KphDriverBasicInformation,
        &basicInfo,
        sizeof(KPH_DRIVER_BASIC_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (context && basicInfo.DriverStart == context->BaseAddress)
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
 * \remarks This function requires a valid KSystemInformer handle.
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

NTSTATUS PhOpenDriver(
    _Out_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName
    )
{
    if (KsiLevel() == KphLevelMax)
    {
        UNICODE_STRING objectName;
        OBJECT_ATTRIBUTES objectAttributes;

        if (!PhStringRefToUnicodeString(ObjectName, &objectName))
            return STATUS_NAME_TOO_LONG;

        InitializeObjectAttributes(
            &objectAttributes,
            &objectName,
            OBJ_CASE_INSENSITIVE,
            RootDirectory,
            NULL
            );

        return KphOpenDriver(
            DriverHandle,
            DesiredAccess,
            &objectAttributes
            );
    }
    else
    {
        return STATUS_NOT_IMPLEMENTED;
    }
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
 * \remarks This function requires a valid KSystemInformer handle.
 */
NTSTATUS PhpQueryDriverVariableSize(
    _In_ HANDLE DriverHandle,
    _In_ KPH_DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer = NULL;
    ULONG bufferSize;
    ULONG returnLength = 0;

    status = KphQueryInformationDriver(
        DriverHandle,
        DriverInformationClass,
        NULL,
        0,
        &returnLength
        );

    if (NT_SUCCESS(status) && returnLength == 0)
    {
        return STATUS_NOT_FOUND;
    }

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = KphQueryInformationDriver(
            DriverHandle,
            DriverInformationClass,
            buffer,
            bufferSize,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        if (returnLength == 0)
        {
            status = STATUS_NOT_FOUND;
            PhFree(buffer);
        }
        else
        {
            *Buffer = buffer;
        }
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
 * \remarks This function requires a valid KSystemInformer handle.
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
        KphDriverNameInformation,
        &unicodeString
        )))
        return status;

    *Name = PhCreateStringFromUnicodeString(unicodeString);
    PhFree(unicodeString);

    return status;
}

/**
 * Gets the object name of a driver.
 *
 * \param DriverHandle A handle to a driver.
 * \param Name A variable which receives a pointer to a string containing the driver image file name.
 * You must free the string using PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function requires a valid KSystemInformer handle.
 */
NTSTATUS PhGetDriverImageFileName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *Name
    )
{
    NTSTATUS status;
    PUNICODE_STRING unicodeString;

    if (!NT_SUCCESS(status = PhpQueryDriverVariableSize(
        DriverHandle,
        KphDriverImageFileNameInformation,
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
 * \remarks This function requires a valid KSystemInformer handle.
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
        KphDriverServiceKeyNameInformation,
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
            static PH_STRINGREF imagePath = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\drivers\\ntfs.sys");
            PH_STRINGREF valueName;
            ULONG dword;

            // Set up the required values.
            dword = 1;
            PhInitializeStringRef(&valueName, L"ErrorControl");
            PhSetValueKey(serviceKeyHandle, &valueName, REG_DWORD, &dword, sizeof(ULONG));
            PhInitializeStringRef(&valueName, L"Start");
            PhSetValueKey(serviceKeyHandle, &valueName, REG_DWORD, &dword, sizeof(ULONG));
            PhInitializeStringRef(&valueName, L"Type");
            PhSetValueKey(serviceKeyHandle, &valueName, REG_DWORD, &dword, sizeof(ULONG));

            // Use a bogus name.
            PhInitializeStringRef(&valueName, L"ImagePath");
            PhSetValueKey(serviceKeyHandle, &valueName, REG_SZ, imagePath.Buffer, (ULONG)imagePath.Length + sizeof(UNICODE_NULL));
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
 * \c BaseAddress and KSystemInformer is loaded.
 *
 * \retval STATUS_INVALID_PARAMETER_MIX Both \c BaseAddress and \c Name were null, or \c Name was
 * not specified and KSystemInformer is not loaded.
 * \retval STATUS_OBJECT_NAME_NOT_FOUND The driver could not be found.
 */
NTSTATUS PhUnloadDriver(
    _In_opt_ PVOID BaseAddress,
    _In_opt_ PCWSTR Name
    )
{
    NTSTATUS status;
    HANDLE driverHandle;
    PPH_STRING serviceKeyName = NULL;
    KPH_LEVEL level;

    level = KsiLevel();

    if (!BaseAddress && !Name)
        return STATUS_INVALID_PARAMETER_MIX;
    if (!Name && (level != KphLevelMax))
        return STATUS_INVALID_PARAMETER_MIX;

    // Try to get the service key name by scanning the Driver directory.

    if ((level == KphLevelMax) && BaseAddress)
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

NTSTATUS PhGetProcessQuotaLimits(
    _In_ HANDLE ProcessHandle,
    _Out_ PQUOTA_LIMITS QuotaLimits
    )
{
    NTSTATUS status;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessQuotaLimits,
        QuotaLimits,
        sizeof(QUOTA_LIMITS),
        NULL
        );

    // Not implemented (dmex)
    //if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    //{
    //    status = KphQueryInformationProcess(
    //        ProcessHandle,
    //        KphProcessQuotaLimits,
    //        QuotaLimits,
    //        sizeof(QUOTA_LIMITS),
    //        NULL
    //        );
    //}

    return status;
}

NTSTATUS PhSetProcessQuotaLimits(
    _In_ HANDLE ProcessHandle,
    _In_ QUOTA_LIMITS QuotaLimits
    )
{
    NTSTATUS status;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessQuotaLimits,
        &QuotaLimits,
        sizeof(QUOTA_LIMITS)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessQuotaLimits,
            &QuotaLimits,
            sizeof(QUOTA_LIMITS)
            );
    }

    return status;
}

NTSTATUS PhSetProcessEmptyWorkingSet(
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status;
    QUOTA_LIMITS_EX quotaLimits;

    memset(&quotaLimits, 0, sizeof(QUOTA_LIMITS_EX));
    quotaLimits.MinimumWorkingSetSize = SIZE_MAX;
    quotaLimits.MaximumWorkingSetSize = SIZE_MAX;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessQuotaLimits,
        &quotaLimits,
        sizeof(QUOTA_LIMITS_EX)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessEmptyWorkingSet,
            &quotaLimits,
            sizeof(QUOTA_LIMITS_EX)
            );
    }

    return status;
}

NTSTATUS PhSetProcessEmptyPageWorkingSet(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size
    )
{
    NTSTATUS status;
    PVOID baseAddress;
    SIZE_T regionSize;

    baseAddress = BaseAddress;
    regionSize = Size;

    // Calling VirtualUnlock on a range of memory that is not locked
    // releases the pages from the process's working set. (MSDN)

    status = NtUnlockVirtualMemory(
        ProcessHandle,
        &baseAddress,
        &regionSize,
        MAP_PROCESS
        );

    // Note: STATUS_SUCCESS is a bad status in this case. (dmex)
    assert(status == STATUS_NOT_LOCKED);

    if (status == STATUS_NOT_LOCKED)
        status = STATUS_SUCCESS;

    return status;
}

NTSTATUS PhGetProcessPriority(
    _In_ HANDLE ProcessHandle,
    _Out_ PUCHAR PriorityClass
    )
{
    NTSTATUS status;
    PROCESS_PRIORITY_CLASS processPriorityClass;

    memset(&processPriorityClass, 0, sizeof(PROCESS_PRIORITY_CLASS));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessPriorityClass,
        &processPriorityClass,
        sizeof(PROCESS_PRIORITY_CLASS),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *PriorityClass = processPriorityClass.PriorityClass;
    }

    return status;
}

NTSTATUS PhSetProcessPriority(
    _In_ HANDLE ProcessHandle,
    _In_ UCHAR PriorityClass
    )
{
    NTSTATUS status;

    if (WindowsVersion >= WINDOWS_11_22H2)
    {
        PROCESS_PRIORITY_CLASS_EX processPriorityClassEx;

        memset(&processPriorityClassEx, 0, sizeof(PROCESS_PRIORITY_CLASS_EX));
        processPriorityClassEx.PriorityClassValid = TRUE;
        processPriorityClassEx.PriorityClass = PriorityClass;

        status = NtSetInformationProcess(
            ProcessHandle,
            ProcessPriorityClassEx,
            &processPriorityClassEx,
            sizeof(PROCESS_PRIORITY_CLASS_EX)
            );

        if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
        {
            status = KphSetInformationProcess(
                ProcessHandle,
                KphProcessPriorityClassEx,
                &processPriorityClassEx,
                sizeof(PROCESS_PRIORITY_CLASS_EX)
                );
        }
    }
    else
    {
        PROCESS_PRIORITY_CLASS processPriorityClass;

        memset(&processPriorityClass, 0, sizeof(PROCESS_PRIORITY_CLASS));
        processPriorityClass.Foreground = FALSE;
        processPriorityClass.PriorityClass = PriorityClass;

        status = NtSetInformationProcess(
            ProcessHandle,
            ProcessPriorityClass,
            &processPriorityClass,
            sizeof(PROCESS_PRIORITY_CLASS)
            );

        if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
        {
            status = KphSetInformationProcess(
                ProcessHandle,
                KphProcessPriorityClass,
                &processPriorityClass,
                sizeof(PROCESS_PRIORITY_CLASS)
                );
        }
    }

    return status;
}

/**
 * Sets a process' I/O priority.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_SET_INFORMATION access.
 * \param IoPriority The new I/O priority.
 */
NTSTATUS PhSetProcessIoPriority(
    _In_ HANDLE ProcessHandle,
    _In_ IO_PRIORITY_HINT IoPriority
    )
{
    NTSTATUS status;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessIoPriority,
        &IoPriority,
        sizeof(IO_PRIORITY_HINT)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessIoPriority,
            &IoPriority,
            sizeof(IO_PRIORITY_HINT)
            );
    }

    return status;
}

NTSTATUS PhSetProcessPagePriority(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG PagePriority
    )
{
    NTSTATUS status;
    PAGE_PRIORITY_INFORMATION pagePriorityInfo;

    pagePriorityInfo.PagePriority = PagePriority;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessPagePriority,
        &pagePriorityInfo,
        sizeof(PAGE_PRIORITY_INFORMATION)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessPagePriority,
            &pagePriorityInfo,
            sizeof(PAGE_PRIORITY_INFORMATION)
            );
    }

    return status;
}

NTSTATUS PhSetProcessPriorityBoost(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN DisablePriorityBoost
    )
{
    NTSTATUS status;
    ULONG priorityBoost;

    priorityBoost = DisablePriorityBoost ? 1 : 0;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessPriorityBoost,
        &priorityBoost,
        sizeof(ULONG)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessPriorityBoost,
            &priorityBoost,
            sizeof(ULONG)
            );
    }

    return status;
}

/**
 * Sets a process' affinity mask.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_SET_INFORMATION access.
 * \param AffinityMask The new affinity mask.
 */
NTSTATUS PhSetProcessAffinityMask(
    _In_ HANDLE ProcessHandle,
    _In_ KAFFINITY AffinityMask
    )
{
    NTSTATUS status;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessAffinityMask,
        &AffinityMask,
        sizeof(KAFFINITY)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessAffinityMask,
            &AffinityMask,
            sizeof(KAFFINITY)
            );
    }

    return status;
}

NTSTATUS PhSetProcessGroupAffinity(
    _In_ HANDLE ProcessHandle,
    _In_ GROUP_AFFINITY GroupAffinity
    )
{
    NTSTATUS status;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessAffinityMask,
        &GroupAffinity,
        sizeof(GROUP_AFFINITY)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessAffinityMask,
            &GroupAffinity,
            sizeof(GROUP_AFFINITY)
            );
    }

    return status;
}

/**
 * Sets the power throttling state for a specified process.
 *
 * \param ProcessHandle The handle to the target process.
 * \param ControlMask The control mask specifying the power throttling control actions to perform.
 * \param StateMask The state mask specifying the power throttling states to set.
 * \return The NTSTATUS code indicating the success or failure of the operation.
 */
NTSTATUS PhSetProcessPowerThrottlingState(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG ControlMask,
    _In_ ULONG StateMask
    )
{
    NTSTATUS status;
    POWER_THROTTLING_PROCESS_STATE powerThrottlingState;

    memset(&powerThrottlingState, 0, sizeof(POWER_THROTTLING_PROCESS_STATE));
    powerThrottlingState.Version = POWER_THROTTLING_PROCESS_CURRENT_VERSION;
    powerThrottlingState.ControlMask = ControlMask;
    powerThrottlingState.StateMask = StateMask;

    status = NtSetInformationProcess(
        ProcessHandle,
        ProcessPowerThrottlingState,
        &powerThrottlingState,
        sizeof(POWER_THROTTLING_PROCESS_STATE)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationProcess(
            ProcessHandle,
            KphProcessPowerThrottlingState,
            &powerThrottlingState,
            sizeof(POWER_THROTTLING_PROCESS_STATE)
            );
    }

    return status;
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
 * \param SystemInformationClass A variable which indicates the kind of system information to be retrieved.
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
    buffer = PhAllocateSafe(bufferSize);
    if (!buffer) return STATUS_NO_MEMORY;

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
            buffer = PhAllocateSafe(bufferSize);
            if (!buffer) return STATUS_NO_MEMORY;
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
 * Enumerates the next process.
 *
 * \param ProcessHandle The handle to the current process. Pass NULL to start enumeration from the beginning.
 * \param DesiredAccess The desired access rights for the process handle.
 * \param Callback The callback function to be called for each enumerated process.
 * \param Context An optional context parameter to be passed to the callback function.
 *
 * \return Returns the status of the enumeration operation.
 *         If the enumeration is successful, it returns STATUS_SUCCESS.
 *         If there are no more processes to enumerate, it returns STATUS_NO_MORE_ENTRIES.
 *         Otherwise, it returns an appropriate NTSTATUS error code.
 */
NTSTATUS PhEnumNextProcess(
    _In_opt_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_ENUM_NEXT_PROCESS Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE newProcessHandle;

    status = NtGetNextProcess(
        ProcessHandle,
        DesiredAccess,
        0,
        0,
        &processHandle
        );

    if (!NT_SUCCESS(status))
        return status;

    while (TRUE)
    {
        status = Callback(processHandle, Context);

        if (status == STATUS_NO_MORE_ENTRIES)
            break;

        if (!NT_SUCCESS(status))
        {
            NtClose(processHandle);
            break;
        }

        status = NtGetNextProcess(
            processHandle,
            DesiredAccess,
            0,
            0,
            &newProcessHandle
            );

        if (NT_SUCCESS(status))
        {
            NtClose(processHandle);
            processHandle = newProcessHandle;
        }
        else
        {
            NtClose(processHandle);
            break;
        }
    }

    if (status == STATUS_NO_MORE_ENTRIES)
        status = STATUS_SUCCESS;

    return status;
}

/**
 * Enumerates the next thread.
 *
 * \param ProcessHandle The handle to the process.
 * \param ThreadHandle The handle to the current thread. Pass NULL to start enumeration from the beginning.
 * \param DesiredAccess The desired access rights for the thread handle.
 * \param Callback The callback function to be called for each enumerated thread.
 * \param Context An optional context parameter to be passed to the callback function.
 *
 * \return Returns the status of the enumeration operation.
 *         If the enumeration is successful, it returns STATUS_SUCCESS.
 *         If there are no more threads to enumerate, it returns STATUS_NO_MORE_ENTRIES.
 *         Otherwise, it returns an appropriate NTSTATUS error code.
 */
NTSTATUS PhEnumNextThread(
    _In_ HANDLE ProcessHandle,
    _In_opt_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_ENUM_NEXT_THREAD Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    HANDLE newThreadHandle;

    status = NtGetNextThread(
        ProcessHandle,
        ThreadHandle,
        DesiredAccess,
        0,
        0,
        &threadHandle
        );

    if (!NT_SUCCESS(status))
        return status;

    while (TRUE)
    {
        status = Callback(threadHandle, Context);

        if (status == STATUS_NO_MORE_ENTRIES)
            break;

        if (!NT_SUCCESS(status))
        {
            NtClose(threadHandle);
            break;
        }

        status = NtGetNextThread(
            ProcessHandle,
            threadHandle,
            DesiredAccess,
            0,
            0,
            &newThreadHandle
            );

        if (NT_SUCCESS(status))
        {
            NtClose(threadHandle);
            threadHandle = newThreadHandle;
        }
        else
        {
            NtClose(threadHandle);
            break;
        }
    }

    if (status == STATUS_NO_MORE_ENTRIES)
        status = STATUS_SUCCESS;

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
    static ULONG initialBufferSize = 0x10000;
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

    if (bufferSize <= 0x200000) initialBufferSize = bufferSize;
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
    ULONG returnLength = 0;
    ULONG attempts = 0;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemExtendedHandleInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 10)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemExtendedHandleInformation,
            buffer,
            bufferSize,
            &returnLength
            );

        attempts++;
    }

    if (!NT_SUCCESS(status))
    {
        // Fall back to using the previous code that we've used since Windows XP (dmex)
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
    PPROCESS_HANDLE_SNAPSHOT_INFORMATION buffer;
    ULONG bufferSize;
    ULONG returnLength = 0;
    ULONG attempts = 0;

    bufferSize = 0x8000;
    buffer = PhAllocate(bufferSize);
    buffer->NumberOfHandles = 0;

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
        buffer->NumberOfHandles = 0;

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
    {
        // NOTE: This is needed to workaround minimal processes on Windows 10
        // returning STATUS_SUCCESS with invalid handle data. (dmex)
        // NOTE: 21H1 and above no longer set NumberOfHandles to zero before returning
        // STATUS_SUCCESS so we first zero the entire buffer using PhAllocateZero. (dmex)
        if (buffer->NumberOfHandles == 0)
        {
            status = STATUS_UNSUCCESSFUL;
            PhFree(buffer);
        }
        else
        {
            *Handles = buffer;
        }
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

/**
 * Enumerates all handles in a process.
 *
 * \param ProcessId The ID of the process.
 * \param ProcessHandle A handle to the process.
 * \param Handles A variable which receives a pointer to a buffer containing
 * information about the handles.
 * \param FilterNeeded A variable which receives a boolean indicating
 * whether the handle information needs to be filtered by process ID.
 */
NTSTATUS PhEnumHandlesGeneric(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN EnableHandleSnapshot,
    _Out_ PSYSTEM_HANDLE_INFORMATION_EX *Handles,
    _Out_ PBOOLEAN FilterNeeded
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // There are three ways of enumerating handles:
    // * On Windows 8 and later, NtQueryInformationProcess with ProcessHandleInformation is the most efficient method.
    // * On Windows XP and later, NtQuerySystemInformation with SystemExtendedHandleInformation.
    // * Otherwise, NtQuerySystemInformation with SystemHandleInformation can be used.

    if ((KsiLevel() >= KphLevelMed) && ProcessHandle)
    {
        PKPH_PROCESS_HANDLE_INFORMATION handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG i;

        // Enumerate handles using KSystemInformer. Unlike with NtQuerySystemInformation,
        // this only enumerates handles for a single process and saves a lot of processing.

        if (NT_SUCCESS(status = KsiEnumerateProcessHandles(ProcessHandle, &handles)))
        {
            convertedHandles = PhAllocate(UFIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[handles->HandleCount]));
            convertedHandles->NumberOfHandles = handles->HandleCount;

            for (i = 0; i < handles->HandleCount; i++)
            {
                convertedHandles->Handles[i].Object = handles->Handles[i].Object;
                convertedHandles->Handles[i].UniqueProcessId = (ULONG_PTR)ProcessId;
                convertedHandles->Handles[i].HandleValue = (ULONG_PTR)handles->Handles[i].Handle;
                convertedHandles->Handles[i].GrantedAccess = (ULONG)handles->Handles[i].GrantedAccess;
                convertedHandles->Handles[i].CreatorBackTraceIndex = 0;
                convertedHandles->Handles[i].ObjectTypeIndex = handles->Handles[i].ObjectTypeIndex;
                convertedHandles->Handles[i].HandleAttributes = handles->Handles[i].HandleAttributes;
            }

            PhFree(handles);

            *Handles = convertedHandles;
            *FilterNeeded = FALSE;
        }
    }

    if (!NT_SUCCESS(status) && WindowsVersion >= WINDOWS_8 && ProcessHandle && EnableHandleSnapshot)
    {
        PPROCESS_HANDLE_SNAPSHOT_INFORMATION handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG i;

        if (NT_SUCCESS(status = PhEnumHandlesEx2(ProcessHandle, &handles)))
        {
            convertedHandles = PhAllocate(UFIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[handles->NumberOfHandles]));
            convertedHandles->NumberOfHandles = handles->NumberOfHandles;

            for (i = 0; i < handles->NumberOfHandles; i++)
            {
                convertedHandles->Handles[i].Object = 0;
                convertedHandles->Handles[i].UniqueProcessId = (ULONG_PTR)ProcessId;
                convertedHandles->Handles[i].HandleValue = (ULONG_PTR)handles->Handles[i].HandleValue;
                convertedHandles->Handles[i].GrantedAccess = handles->Handles[i].GrantedAccess;
                convertedHandles->Handles[i].CreatorBackTraceIndex = 0;
                convertedHandles->Handles[i].ObjectTypeIndex = (USHORT)handles->Handles[i].ObjectTypeIndex;
                convertedHandles->Handles[i].HandleAttributes = handles->Handles[i].HandleAttributes;
            }

            PhFree(handles);

            *Handles = convertedHandles;
            *FilterNeeded = FALSE;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PSYSTEM_HANDLE_INFORMATION_EX handles;

        if (!NT_SUCCESS(status = PhEnumHandlesEx(&handles)))
            return status;

        *Handles = handles;
        *FilterNeeded = TRUE;
    }

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

NTSTATUS PhEnumPagefilesEx(
    _Out_ PVOID *Pagefiles
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x200;

    buffer = PhAllocate(bufferSize);

    while ((status = NtQuerySystemInformation(
        SystemPageFileInformationEx,
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
 * Enumerates pool tag information.
 *
 * \param Buffer A pointer to a buffer that will receive the pool tag information.
 * \return The status of the operation.
 */
NTSTATUS PhEnumPoolTagInformation(
    _Out_ PVOID* Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemPoolTagInformation,
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemPoolTagInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
        PhFree(buffer);

    return status;
}

/**
 * Enumerates information about the big pool allocations in the system.
 *
 * \param Buffer A pointer to a variable that receives the buffer containing the big pool information.
 *
 * \return The status of the operation.
 */
NTSTATUS PhEnumBigPoolInformation(
    _Out_ PVOID* Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemBigPoolInformation,
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemBigPoolInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
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
    _In_ PVOID Context
    )
{
    static PH_STRINGREF clrString = PH_STRINGREF_INIT(L"clr.dll");
    static PH_STRINGREF clrcoreString = PH_STRINGREF_INIT(L"coreclr.dll");
    static PH_STRINGREF mscorwksString = PH_STRINGREF_INIT(L"mscorwks.dll");
    static PH_STRINGREF mscorsvrString = PH_STRINGREF_INIT(L"mscorsvr.dll");
    static PH_STRINGREF mscorlibString = PH_STRINGREF_INIT(L"mscorlib.dll");
    static PH_STRINGREF mscorlibNiString = PH_STRINGREF_INIT(L"mscorlib.ni.dll");
    static PH_STRINGREF frameworkString = PH_STRINGREF_INIT(L"\\Microsoft.NET\\Framework\\");
    static PH_STRINGREF framework64String = PH_STRINGREF_INIT(L"\\Microsoft.NET\\Framework64\\");
    PH_STRINGREF baseDllName;

    PhUnicodeStringToStringRef(&Module->BaseDllName, &baseDllName);

    if (
        PhEqualStringRef(&baseDllName, &clrString, TRUE) ||
        PhEqualStringRef(&baseDllName, &mscorwksString, TRUE) ||
        PhEqualStringRef(&baseDllName, &mscorsvrString, TRUE)
        )
    {
        PH_STRINGREF fileName;
        PH_STRINGREF systemRoot;
        PPH_STRINGREF frameworkPart;

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

        PhUnicodeStringToStringRef(&Module->FullDllName, &fileName);
        PhGetSystemRoot(&systemRoot);

        if (PhStartsWithStringRef(&fileName, &systemRoot, TRUE))
        {
            fileName.Buffer = PTR_ADD_OFFSET(fileName.Buffer, systemRoot.Length);
            fileName.Length -= systemRoot.Length;

            if (PhStartsWithStringRef(&fileName, frameworkPart, TRUE))
            {
                fileName.Buffer = PTR_ADD_OFFSET(fileName.Buffer, frameworkPart->Length);
                fileName.Length -= frameworkPart->Length;

                if (fileName.Length >= 4 * sizeof(WCHAR)) // vx.x
                {
                    if (fileName.Buffer[1] == L'1')
                    {
                        if (fileName.Buffer[3] == L'0')
                            *(PULONG)Context |= PH_CLR_VERSION_1_0;
                        else if (fileName.Buffer[3] == L'1')
                            *(PULONG)Context |= PH_CLR_VERSION_1_1;
                    }
                    else if (fileName.Buffer[1] == L'2')
                    {
                        *(PULONG)Context |= PH_CLR_VERSION_2_0;
                    }
                    else if (fileName.Buffer[1] >= L'4' && fileName.Buffer[1] <= L'9')
                    {
                        *(PULONG)Context |= PH_CLR_VERSION_4_ABOVE;
                    }
                }
            }
        }
    }
    else if (
        PhEqualStringRef(&baseDllName, &mscorlibString, TRUE) ||
        PhEqualStringRef(&baseDllName, &mscorlibNiString, TRUE)
        )
    {
        *(PULONG)Context |= PH_CLR_MSCORLIB_PRESENT;
    }
    else if (PhEqualStringRef(&baseDllName, &clrcoreString, TRUE))
    {
        *(PULONG)Context |= PH_CLR_CORELIB_PRESENT;
    }

    return TRUE;
}

typedef struct _PHP_PIPE_NAME_HASH
{
    PPH_STRINGREF Name;
    ULONG Found;
} PHP_PIPE_NAME_HASH, *PPHP_PIPE_NAME_HASH;

static BOOLEAN NTAPI PhpDotNetCorePipeHashCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PPHP_PIPE_NAME_HASH context = Context;
    PH_STRINGREF objectName;

    objectName.Length = Information->FileNameLength;
    objectName.Buffer = Information->FileName;

    if (
        PhHashStringRefEx(context->Name, FALSE, PH_STRING_HASH_X65599) ==
        PhHashStringRefEx(&objectName, FALSE, PH_STRING_HASH_X65599)
        )
    {
        context->Found = TRUE;
        return FALSE;
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
        SIZE_T returnLength;
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING objectName;
        PH_STRINGREF objectNameSr;
        PH_FORMAT format[2];
        WCHAR formatBuffer[0x80];

        // Most .NET processes have a handle open to a section named
        // \BaseNamedObjects\Cor_Private_IPCBlock(_v4)_<ProcessId>. This is the same object used by
        // the ICorPublish::GetProcess function. Instead of calling that function, we simply check
        // for the existence of that section object. This means:
        // * Better performance.
        // * No need for admin rights to get .NET status of processes owned by other users.

        // Version 4 section object

        PhInitFormatS(&format[0], L"\\BaseNamedObjects\\Cor_Private_IPCBlock_v4_");
        PhInitFormatU(&format[1], HandleToUlong(ProcessId));

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            objectNameSr.Length = returnLength - sizeof(UNICODE_NULL);
            objectNameSr.Buffer = formatBuffer;

            PhStringRefToUnicodeString(&objectNameSr, &objectName);
        }
        else
        {
            RtlInitEmptyUnicodeString(&objectName, NULL, 0);
        }

        InitializeObjectAttributes(
            &objectAttributes,
            &objectName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
        status = NtOpenSection(
            &sectionHandle,
            SECTION_QUERY,
            &objectAttributes
            );

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
        PhInitFormatU(&format[1], HandleToUlong(ProcessId));

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            objectNameSr.Length = returnLength - sizeof(UNICODE_NULL);
            objectNameSr.Buffer = formatBuffer;

            PhStringRefToUnicodeString(&objectNameSr, &objectName);
        }
        else
        {
            RtlInitEmptyUnicodeString(&objectName, NULL, 0);
        }

        InitializeObjectAttributes(
            &objectAttributes,
            &objectName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
        status = NtOpenSection(
            &sectionHandle,
            SECTION_QUERY,
            &objectAttributes
            );

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

        // .NET Core 3.0 and above objects

        PhInitFormatS(&format[0], L"dotnet-diagnostic-");
        PhInitFormatU(&format[1], HandleToUlong(ProcessId));

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            PHP_PIPE_NAME_HASH context;

            objectNameSr.Length = returnLength - sizeof(UNICODE_NULL);
            objectNameSr.Buffer = formatBuffer;
            PhStringRefToUnicodeString(&objectNameSr, &objectName);
            context.Name = &objectNameSr;
            context.Found = FALSE;

            status = PhEnumDirectoryNamedPipe(
                &objectName,
                PhpDotNetCorePipeHashCallback,
                &context
                );

            if (NT_SUCCESS(status))
            {
                if (!context.Found)
                {
                    status = STATUS_OBJECT_NAME_NOT_FOUND;
                }
            }

            // NOTE: NtQueryAttributesFile and other query functions connect to the pipe and should be avoided. (dmex)
            //
            //FILE_BASIC_INFORMATION fileInfo;
            //
            //objectNameSr.Length = returnLength - sizeof(UNICODE_NULL);
            //objectNameSr.Buffer = formatBuffer;
            //
            //PhStringRefToUnicodeString(&objectNameSr, &objectName);
            //InitializeObjectAttributes(
            //    &objectAttributes,
            //    &objectName,
            //    OBJ_CASE_INSENSITIVE,
            //    NULL,
            //    NULL
            //    );
            //
            //status = NtQueryAttributesFile(&objectAttributes, &fileInfo)
        }

        if (NT_SUCCESS(status))
        {
            if (IsDotNet)
                *IsDotNet = TRUE;
            if (Flags)
                *Flags = PH_CLR_VERSION_4_ABOVE | PH_CLR_CORE_3_0_ABOVE;

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
            *IsDotNet = (flags & PH_CLR_VERSION_MASK) && (flags & (PH_CLR_MSCORLIB_PRESENT | PH_CLR_CORELIB_PRESENT));

        if (Flags)
            *Flags = flags;

        return status;
    }
}

/*
 * Opens a directory object.
 *
 * \param DirectoryHandle A variable which receives a handle to the directory object.
 * \param DesiredAccess The desired access to the directory object.
 * \param RootDirectory A handle to the root directory of the object.
 * \param ObjectName The name of the directory object.
 *
 * \return Returns the status of the operation.
 */
NTSTATUS PhOpenDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenDirectoryObject(
        DirectoryHandle,
        DesiredAccess,
        &objectAttributes
        );

    return status;
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
    return PhEnumDirectoryFileEx(
        FileHandle,
        FileDirectoryInformation,
        FALSE,
        SearchPattern,
        Callback,
        Context
        );
}

NTSTATUS PhEnumDirectoryFileEx(
    _In_ HANDLE FileHandle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
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
        // Query the directory, doubling the buffer each time NtQueryDirectoryFile fails. (wj32)
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
                FileInformationClass,
                ReturnSingleEntry,
                SearchPattern,
                firstTime
                );

            // Our ISB is on the stack, so we have to wait for the operation to complete before continuing. (wj32)
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

        // If we don't have any entries to read, exit. (wj32)
        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        // Read the batch and execute the callback function for each file. (wj32)

        i = 0;
        cont = TRUE;

        while (TRUE)
        {
            PFILE_DIRECTORY_NEXT_INFORMATION information = PTR_ADD_OFFSET(buffer, i);

            if (!Callback(FileHandle, information, Context))
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

/**
 * \brief Enumerates the volume Reparse Points using the \\$Extend\\$Reparse:$R:$INDEX_ALLOCATION directory.
 *
 * \param FileHandle A handle to the volume.
 * \param Callback A pointer to a callback function.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status
 */
NTSTATUS PhEnumReparsePointInformation(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_REPARSE_POINT Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    BOOLEAN firstTime = TRUE;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = sizeof(FILE_REPARSE_POINT_INFORMATION[512]);
    buffer = PhAllocate(bufferSize);

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
            FileReparsePointInformation,
            FALSE,
            NULL,
            firstTime
            );

        if (status == STATUS_PENDING)
        {
            status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

            if (NT_SUCCESS(status))
                status = isb.Status;
        }

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        status = Callback(FileHandle, buffer, isb.Information, Context);

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        firstTime = FALSE;
    }

    PhFree(buffer);

    return status;
}

/**
 * \brief Enumerates the volume ObjectIDs using the \\$Extend\\$ObjId:$O:$INDEX_ALLOCATION directory.
 *
 * \param FileHandle A handle to the volume.
 * \param Callback A pointer to a callback function.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status
 */
NTSTATUS PhEnumObjectIdInformation(
    _In_ HANDLE FileHandle,
    _In_ PPH_ENUM_OBJECT_ID Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    BOOLEAN firstTime = TRUE;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = sizeof(FILE_OBJECTID_INFORMATION[128]);
    buffer = PhAllocate(bufferSize);

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
            FileObjectIdInformation,
            FALSE,
            NULL,
            firstTime
            );

        if (status == STATUS_PENDING)
        {
            status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

            if (NT_SUCCESS(status))
                status = isb.Status;
        }

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        status = Callback(FileHandle, buffer, isb.Information, Context);

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
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
    IO_STATUS_BLOCK isb;
    ULONG bufferSize;
    PVOID buffer;

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

        status = Callback(FileHandle, buffer, Context);

        if (status == STATUS_NO_MORE_FILES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            break;

        firstTime = FALSE;
    }

    PhFree(buffer);

    return status;
}

NTSTATUS PhSetFileExtendedAttributes(
    _In_ HANDLE FileHandle,
    _In_ PPH_BYTESREF Name,
    _In_opt_ PPH_BYTESREF Value
    )
{
    NTSTATUS status;
    ULONG infoLength;
    PFILE_FULL_EA_INFORMATION info;
    IO_STATUS_BLOCK isb;

    infoLength = sizeof(FILE_FULL_EA_INFORMATION) + (ULONG)Name->Length + sizeof(ANSI_NULL);
    if (Value) infoLength += (ULONG)Value->Length + sizeof(ANSI_NULL);

    info = PhAllocateZero(infoLength);
    info->EaNameLength = (UCHAR)Name->Length;
    memcpy(
        info->EaName,
        Name->Buffer,
        Name->Length
        );

    if (Value)
    {
        info->EaValueLength = (USHORT)Value->Length;
        memcpy(
            PTR_ADD_OFFSET(info->EaName, info->EaNameLength + sizeof(ANSI_NULL)),
            Value->Buffer,
            Value->Length
            );
    }

    status = NtSetEaFile(
        FileHandle,
        &isb,
        info,
        infoLength
        );

    PhFree(info);

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

NTSTATUS PhEnumFileHardLinks(
    _In_ HANDLE FileHandle,
    _Out_ PVOID *HardLinks
    )
{
    return PhpQueryFileVariableSize(
        FileHandle,
        FileHardLinkInformation,
        HardLinks
        );
}

NTSTATUS PhCreateSymbolicLinkObject(
    _Out_ PHANDLE LinkHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_STRINGREF FileName,
    _In_ PPH_STRINGREF LinkName
    )
{
    NTSTATUS status;
    HANDLE linkHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    UNICODE_STRING objectTarget;

    if (!PhStringRefToUnicodeString(FileName, &objectName))
        return STATUS_NAME_TOO_LONG;
    if (!PhStringRefToUnicodeString(LinkName, &objectTarget))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateSymbolicLinkObject(
        &linkHandle,
        DesiredAccess,
        &objectAttributes,
        &objectTarget
        );

    if (NT_SUCCESS(status))
    {
        *LinkHandle = linkHandle;
    }

    return status;
}

NTSTATUS PhQuerySymbolicLinkObject(
    _Out_ PPH_STRING* LinkTarget,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName
    )
{
    NTSTATUS status;
    HANDLE linkHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    UNICODE_STRING targetName;
    WCHAR targetNameBuffer[DOS_MAX_PATH_LENGTH];

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenSymbolicLinkObject(
        &linkHandle,
        SYMBOLIC_LINK_QUERY,
        &objectAttributes
        );

    if (!NT_SUCCESS(status))
        return status;

    RtlInitEmptyUnicodeString(&targetName, targetNameBuffer, sizeof(targetNameBuffer));

    status = NtQuerySymbolicLinkObject(
        linkHandle,
        &targetName,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *LinkTarget = PhCreateStringFromUnicodeString(&targetName);
    }

    NtClose(linkHandle);

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
    PWCHAR buffer;

    // Allocate one buffer for all 26 prefixes to reduce overhead.
    buffer = PhAllocate(PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR) * 26);

    for (i = 0; i < 26; i++)
    {
        PhDevicePrefixes[i].Length = 0;
        PhDevicePrefixes[i].MaximumLength = PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR);
        PhDevicePrefixes[i].Buffer = buffer;
        buffer = PTR_ADD_OFFSET(buffer, PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR));
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
        providerOrder = PhQueryRegistryStringZ(orderKeyHandle, L"ProviderOrder");
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

        PhSplitStringRefAtChar(&remainingPart, L',', &part, &remainingPart);

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
                if (deviceName = PhQueryRegistryStringZ(networkProviderKeyHandle, L"DeviceName"))
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
    ULONG deviceMap = 0;

    PhGetProcessDeviceMap(NtCurrentProcess(), &deviceMap);

    PhAcquireQueuedLockExclusive(&PhDevicePrefixesLock);

    for (ULONG i = 0; i < 0x1A; i++)
    {
        HANDLE linkHandle;
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING deviceName;

        if (deviceMap)
        {
            if (!(deviceMap & (0x1 << i)))
            {
                PhDevicePrefixes[i].Length = 0;
                continue;
            }
        }

        deviceNameBuffer[4] = (WCHAR)('A' + i);
        deviceName.Buffer = deviceNameBuffer;
        deviceName.Length = sizeof(deviceNameBuffer) - sizeof(UNICODE_NULL);
        deviceName.MaximumLength = deviceName.Length + sizeof(UNICODE_NULL);

        InitializeObjectAttributes(
            &objectAttributes,
            &deviceName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        if (NT_SUCCESS(NtOpenSymbolicLinkObject(
            &linkHandle,
            SYMBOLIC_LINK_QUERY,
            &objectAttributes
            )))
        {
            if (!NT_SUCCESS(NtQuerySymbolicLinkObject(
                linkHandle,
                &PhDevicePrefixes[i],
                NULL
                )))
            {
                PhDevicePrefixes[i].Length = 0;
            }

            NtClose(linkHandle);
        }
        else
        {
            PhDevicePrefixes[i].Length = 0;
        }
    }

    PhReleaseQueuedLockExclusive(&PhDevicePrefixesLock);
}

// rev from FindFirstVolumeW (dmex)
/**
 * \brief Retrieves the mount points of volumes.
 *
 * \param DeviceHandle A handle to the MountPointManager.
 * \param MountPoints An array of mounts.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhGetVolumeMountPoints(
    _In_ HANDLE DeviceHandle,
    _Out_ PMOUNTMGR_MOUNT_POINTS* MountPoints
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    MOUNTMGR_MOUNT_POINT inputBuffer = { 0 };
    PMOUNTMGR_MOUNT_POINTS outputBuffer;
    ULONG inputBufferLength = sizeof(inputBuffer);
    ULONG outputBufferLength;
    ULONG attempts = 16;

    outputBufferLength = 0x800;
    outputBuffer = PhAllocate(outputBufferLength);

    do
    {
        status = NtDeviceIoControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            IOCTL_MOUNTMGR_QUERY_POINTS,
            &inputBuffer,
            inputBufferLength,
            outputBuffer,
            outputBufferLength
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            outputBufferLength = outputBuffer->Size;
            PhFree(outputBuffer);
            outputBuffer = PhAllocate(outputBufferLength);
        }
        else
        {
            PhFree(outputBuffer);
            return status;
        }
    } while (--attempts);

    if (NT_SUCCESS(status))
    {
        *MountPoints = outputBuffer;
    }
    else
    {
        PhFree(outputBuffer);
    }

    return status;
}

// rev from GetVolumePathNamesForVolumeNameW (dmex)
/**
 * \brief Retrieves a list of drive letters and mounted folder paths for the specified volume.
 *
 * \param DeviceHandle A handle to the MountPointManager.
 * \param VolumeName A volume GUID path for the volume.
 * \param VolumePathNames A pointer to a buffer that receives the list of drive letters and mounted folder paths.
 * \a The list is an array of null-terminated strings terminated by an additional NULL character.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhGetVolumePathNamesForVolumeName(
    _In_ HANDLE DeviceHandle,
    _In_ PPH_STRINGREF VolumeName,
    _Out_ PMOUNTMGR_VOLUME_PATHS* VolumePathNames
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PMOUNTMGR_TARGET_NAME inputBuffer;
    PMOUNTMGR_VOLUME_PATHS outputBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    ULONG attempts = 16;

    inputBufferLength = UFIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName[VolumeName->Length]) + sizeof(UNICODE_NULL);
    inputBuffer = PhAllocate(inputBufferLength); // Volume{guid}, CM_Get_Device_Interface_List, SymbolicLinks, [??]
    inputBuffer->DeviceNameLength = (USHORT)VolumeName->Length;
    RtlCopyMemory(inputBuffer->DeviceName, VolumeName->Buffer, VolumeName->Length);

    outputBufferLength = UFIELD_OFFSET(MOUNTMGR_VOLUME_PATHS, MultiSz[DOS_MAX_PATH_LENGTH]) + sizeof(UNICODE_NULL);
    outputBuffer = PhAllocate(outputBufferLength);

    do
    {
        status = NtDeviceIoControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS,
            inputBuffer,
            inputBufferLength,
            outputBuffer,
            outputBufferLength
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            outputBufferLength = (outputBuffer->MultiSzLength * sizeof(WCHAR)) + sizeof(UNICODE_NULL);
            PhFree(outputBuffer);
            outputBuffer = PhAllocate(outputBufferLength);
        }
        else
        {
            PhFree(inputBuffer);
            PhFree(outputBuffer);
            return status;
        }
    } while (--attempts);

    if (NT_SUCCESS(status))
    {
        *VolumePathNames = outputBuffer;
    }
    else
    {
        PhFree(outputBuffer);
    }

    PhFree(inputBuffer);

    return status;
}

/**
 * Flush file caches on all volumes.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhFlushVolumeCache(
    VOID
    )
{
    NTSTATUS status;
    HANDLE deviceHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    PMOUNTMGR_MOUNT_POINTS objectMountPoints;

    RtlInitUnicodeString(&objectName, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &deviceHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetVolumeMountPoints(
        deviceHandle,
        &objectMountPoints
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    for (ULONG i = 0; i < objectMountPoints->NumberOfMountPoints; i++)
    {
        PMOUNTMGR_MOUNT_POINT mountPoint = &objectMountPoints->MountPoints[i];
        objectName.Length = mountPoint->SymbolicLinkNameLength;
        objectName.MaximumLength = mountPoint->SymbolicLinkNameLength + sizeof(UNICODE_NULL);
        objectName.Buffer = PTR_ADD_OFFSET(objectMountPoints, mountPoint->SymbolicLinkNameOffset);

        if (MOUNTMGR_IS_VOLUME_NAME(&objectName)) // \\??\\Volume{1111-2222}
        {
            HANDLE volumeHandle;

            InitializeObjectAttributes(
                &objectAttributes,
                &objectName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );

            status = NtCreateFile(
                &volumeHandle,
                FILE_WRITE_DATA | SYNCHRONIZE,
                &objectAttributes,
                &ioStatusBlock,
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
                //if (WindowsVersion >= WINDOWS_8)
                //{
                //    status = NtFlushBuffersFileEx(volumeHandle, 0, 0, 0, &ioStatusBlock);
                //}
                //else
                {
                    status = NtFlushBuffersFile(volumeHandle, &ioStatusBlock);
                }

                NtClose(volumeHandle);
            }
        }
    }

    PhFree(objectMountPoints);

CleanupExit:
    NtClose(deviceHandle);

    return status;
}

NTSTATUS PhUpdateDosDeviceMountPrefixes(
    VOID
    )
{
    NTSTATUS status;
    HANDLE deviceHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    PMOUNTMGR_MOUNT_POINTS deviceMountPoints;

    RtlInitUnicodeString(&objectName, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &deviceHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetVolumeMountPoints(
        deviceHandle,
        &deviceMountPoints
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhDevicePrefixes); i++)
    {
        PhDevicePrefixes[i].Length = 0;
    }

    for (ULONG i = 0; i < deviceMountPoints->NumberOfMountPoints; i++)
    {
        PMOUNTMGR_MOUNT_POINT entry = &deviceMountPoints->MountPoints[i];
        UNICODE_STRING linkName =
        {
            entry->SymbolicLinkNameLength,
            entry->SymbolicLinkNameLength + sizeof(UNICODE_NULL),
            PTR_ADD_OFFSET(deviceMountPoints, entry->SymbolicLinkNameOffset)
        };
        UNICODE_STRING deviceName =
        {
            entry->DeviceNameLength,
            entry->DeviceNameLength + sizeof(UNICODE_NULL),
            PTR_ADD_OFFSET(deviceMountPoints, entry->DeviceNameOffset)
        };

        if (MOUNTMGR_IS_DRIVE_LETTER(&linkName)) // \\DosDevices\\C:
        {
            USHORT index = (USHORT)(linkName.Buffer[12] - L'A');

            if (index >= RTL_NUMBER_OF(PhDevicePrefixes))
                continue;
            if (deviceName.Length >= PhDevicePrefixes[index].MaximumLength - sizeof(UNICODE_NULL))
                continue;

            PhDevicePrefixes[index].Length = deviceName.Length;
            memcpy_s(
                PhDevicePrefixes[index].Buffer,
                PhDevicePrefixes[index].MaximumLength,
                deviceName.Buffer,
                deviceName.Length
                );
        }

        //if (MOUNTMGR_IS_VOLUME_NAME(&linkName)) // \\??\\Volume{1111-2222}
        //{
        //    PH_STRINGREF volumeLinkName;
        //    PMOUNTMGR_VOLUME_PATHS volumePaths;
        //
        //    PhUnicodeStringToStringRef(&linkName, &volumeLinkName);
        //
        //    if (NT_SUCCESS(PhGetVolumePathNamesForVolumeName(deviceHandle, &volumeLinkName, &volumePaths)))
        //    {
        //        for (PWSTR path = volumePaths->MultiSz; *path; path += PhCountStringZ(path) + 1)
        //        {
        //            dprintf("%S\n", path); // C:\\Mounted\\Folders
        //        }
        //    }
        //}
    }

    PhFree(deviceMountPoints);

CleanupExit:
    NtClose(deviceHandle);

    return status;
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
    _In_ PPH_STRINGREF Name
    )
{
    ULONG i;
    PPH_STRING newName = NULL;

    if (PhBeginInitOnce(&PhDevicePrefixesInitOnce))
    {
        PhpInitializeDevicePrefixes();
        PhUpdateDosDevicePrefixes();
        PhUpdateMupDevicePrefixes();

        //PhUpdateDosDeviceMountPrefixes();

        PhEndInitOnce(&PhDevicePrefixesInitOnce);
    }

    PhAcquireQueuedLockShared(&PhDevicePrefixesLock);

    // Go through the DOS devices and try to find a matching prefix.
    for (i = 0; i < 26; i++)
    {
        BOOLEAN isPrefix = FALSE;
        PH_STRINGREF prefix;

        PhUnicodeStringToStringRef(&PhDevicePrefixes[i], &prefix);

        if (prefix.Length != 0)
        {
            if (PhStartsWithStringRef(Name, &prefix, TRUE))
            {
                // To ensure we match the longest prefix, make sure the next character is a
                // backslash or the path is equal to the prefix.
                if (Name->Length == prefix.Length || Name->Buffer[prefix.Length / sizeof(WCHAR)] == OBJ_NAME_PATH_SEPARATOR)
                {
                    isPrefix = TRUE;
                }
            }
        }

        if (isPrefix)
        {
            // <letter>:path
            newName = PhCreateStringEx(NULL, 2 * sizeof(WCHAR) + Name->Length - prefix.Length);
            newName->Buffer[0] = (WCHAR)('A' + i);
            newName->Buffer[1] = L':';
            memcpy(
                &newName->Buffer[2],
                &Name->Buffer[prefix.Length / sizeof(WCHAR)],
                Name->Length - prefix.Length
                );

            break;
        }
    }

    PhReleaseQueuedLockShared(&PhDevicePrefixesLock);

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
                if (PhStartsWithStringRef(Name, &PhDeviceMupPrefixes[i]->sr, TRUE))
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
    // System32, SysWOW64, SysArm32, and SyChpe32 are all identical length, fixup is the same
    else if (
        // "System32\" means "C:\Windows\System32\".
        PhStartsWithString2(FileName, L"System32\\", TRUE)
#if _WIN64
        // "SysWOW64\" means "C:\Windows\SysWOW64\".
        || PhStartsWithString2(FileName, L"SysWOW64\\", TRUE)
#if _M_ARM64
        // "SysArm32\" means "C:\Windows\SysArm32\".
        || PhStartsWithString2(FileName, L"SysArm32\\", TRUE)
        // "SyChpe32\" means "C:\Windows\SyChpe32\".
        || PhStartsWithString2(FileName, L"SyChpe32\\", TRUE)
#endif
#endif
        )
    {
        PH_STRINGREF systemRoot;

        PhGetSystemRoot(&systemRoot);
        newFileName = PhCreateStringEx(NULL, systemRoot.Length + sizeof(UNICODE_NULL) + FileName->Length);
        memcpy(newFileName->Buffer, systemRoot.Buffer, systemRoot.Length);
        newFileName->Buffer[systemRoot.Length / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
        memcpy(PTR_ADD_OFFSET(newFileName->Buffer, systemRoot.Length + sizeof(UNICODE_NULL)), FileName->Buffer, FileName->Length);
    }
    else if (FileName->Length != 0 && FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        PPH_STRING resolvedName;

        resolvedName = PhResolveDevicePrefix(&FileName->sr);

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
                PH_STRINGREF systemRoot;

                PhGetSystemRoot(&systemRoot);
                newFileName = PhCreateStringEx(NULL, FileName->Length + 2 * sizeof(WCHAR));
                newFileName->Buffer[0] = systemRoot.Buffer[0];
                newFileName->Buffer[1] = L':';
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

PPH_STRING PhDosPathNameToNtPathName(
    _In_ PPH_STRINGREF Name
    )
{
    PPH_STRING newName = NULL;
    PH_STRINGREF prefix;
    ULONG index;

    if (PhBeginInitOnce(&PhDevicePrefixesInitOnce))
    {
        PhpInitializeDevicePrefixes();
        PhUpdateDosDevicePrefixes();
        PhUpdateMupDevicePrefixes();

        PhEndInitOnce(&PhDevicePrefixesInitOnce);
    }

    if (PATH_IS_WIN32_DRIVE_PREFIX(Name))
    {
        index = (ULONG)(PhUpcaseUnicodeChar(Name->Buffer[0]) - L'A');

        if (index >= RTL_NUMBER_OF(PhDevicePrefixes))
            return NULL;

        PhAcquireQueuedLockShared(&PhDevicePrefixesLock);
        PhUnicodeStringToStringRef(&PhDevicePrefixes[index], &prefix);

        if (prefix.Length != 0)
        {
            // C:\\Name -> \\Device\\HardDiskVolumeX\\Name
            newName = PhCreateStringEx(NULL, prefix.Length + Name->Length - sizeof(WCHAR[2]));
            memcpy(
                newName->Buffer,
                prefix.Buffer,
                prefix.Length
                );
            memcpy(
                PTR_ADD_OFFSET(newName->Buffer, prefix.Length),
                PTR_ADD_OFFSET(Name->Buffer, sizeof(WCHAR[2])),
                Name->Length - sizeof(WCHAR[2])
                );
        }

        PhReleaseQueuedLockShared(&PhDevicePrefixesLock);
    }
    else if (PhStartsWithStringRef2(Name, L"\\SystemRoot", TRUE))
    {
        PhAcquireQueuedLockShared(&PhDevicePrefixesLock);
        PhUnicodeStringToStringRef(&PhDevicePrefixes[(ULONG)'C'-'A'], &prefix);

        if (prefix.Length != 0)
        {
            static PH_STRINGREF systemRoot = PH_STRINGREF_INIT(L"\\Windows");

            // \\SystemRoot\\Name -> \\Device\\HardDiskVolumeX\\Windows\\Name
            newName = PhCreateStringEx(NULL, prefix.Length + Name->Length + systemRoot.Length - sizeof(L"SystemRoot"));
            memcpy(
                newName->Buffer,
                prefix.Buffer,
                prefix.Length
                );
            memcpy(
                PTR_ADD_OFFSET(newName->Buffer, prefix.Length),
                systemRoot.Buffer,
                systemRoot.Length
                );
            memcpy(
                PTR_ADD_OFFSET(newName->Buffer, prefix.Length + systemRoot.Length),
                PTR_ADD_OFFSET(Name->Buffer, sizeof(L"SystemRoot")),
                Name->Length - sizeof(L"SystemRoot")
                );
        }

        PhReleaseQueuedLockShared(&PhDevicePrefixesLock);
    }

    return newName;
}

NTSTATUS PhDosLongPathNameToNtPathNameWithStatus(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Outptr_opt_result_z_ PWSTR* FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static NTSTATUS (NTAPI* RtlDosLongPathNameToNtPathName_I_WithStatus)(
        _In_ PCWSTR DosFileName,
        _Out_ PUNICODE_STRING NtFileName,
        _Out_opt_ PWSTR *FilePart,
        _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
        ) = NULL;
    NTSTATUS status;

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion >= WINDOWS_10_RS1 && RtlAreLongPathsEnabled())
        {
            PVOID baseAddress;

            if (baseAddress = PhGetLoaderEntryDllBaseZ(L"ntdll.dll"))
            {
                RtlDosLongPathNameToNtPathName_I_WithStatus = PhGetDllBaseProcedureAddress(baseAddress, "RtlDosLongPathNameToNtPathName_U_WithStatus", 0);
            }
        }

        PhEndInitOnce(&initOnce);
    }

    if (RtlDosLongPathNameToNtPathName_I_WithStatus)
    {
        status = RtlDosLongPathNameToNtPathName_I_WithStatus(
            DosFileName,
            NtFileName,
            FilePart,
            RelativeName
            );
    }
    else
    {
        status = RtlDosPathNameToNtPathName_U_WithStatus(
            DosFileName,
            NtFileName,
            FilePart,
            RelativeName
            );
    }

    return status;
}

PPH_STRING PhGetNtPathRootPrefix(
    _In_ PPH_STRINGREF Name
    )
{
    PPH_STRING pathDevicePrefix = NULL;
    PH_STRINGREF prefix;

    PhAcquireQueuedLockShared(&PhDevicePrefixesLock);

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhDevicePrefixes); i++)
    {
        PhUnicodeStringToStringRef(&PhDevicePrefixes[i], &prefix);

        if (prefix.Length && PhStartsWithStringRef(Name, &prefix, FALSE))
        {
            pathDevicePrefix = PhCreateString2(&prefix);
            break;
        }
    }

    PhReleaseQueuedLockShared(&PhDevicePrefixesLock);

    return pathDevicePrefix;
}

PPH_STRING PhGetExistingPathPrefix(
    _In_ PPH_STRINGREF Name
    )
{
    PPH_STRING existingPathPrefix = NULL;
    PH_STRINGREF remainingPart;
    PH_STRINGREF directoryPart;
    PH_STRINGREF baseNamePart;

    if (PATH_IS_WIN32_DRIVE_PREFIX(Name))
    {
        assert(FALSE);
        return NULL;
    }

    if (PhDoesDirectoryExist(Name))
    {
        return PhCreateString2(Name);
    }

    remainingPart = *Name;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtLastChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &baseNamePart);

        if (directoryPart.Length != 0)
        {
            if (PhDoesDirectoryExist(&directoryPart))
            {
                existingPathPrefix = PhCreateString2(&directoryPart);
                break;
            }
        }

        remainingPart = directoryPart;
    }

    //if (PhEqualStringRef(&existingPathPrefix, PhGetNtPathRootPrefix(Name), FALSE))
    //    return NULL;

    return existingPathPrefix;
}

PPH_STRING PhGetExistingPathPrefixWin32(
    _In_ PPH_STRINGREF Name
    )
{
    PPH_STRING existingPathPrefix = NULL;
    PH_STRINGREF remainingPart;
    PH_STRINGREF directoryPart;
    PH_STRINGREF baseNamePart;

    if (!PATH_IS_WIN32_DRIVE_PREFIX(Name))
    {
        assert(FALSE);
        return NULL;
    }

    if (PhDoesDirectoryExistWin32(PhGetStringRefZ(Name)))
    {
        return PhCreateString2(Name);
    }

    remainingPart = *Name;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtLastChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &baseNamePart);

        if (directoryPart.Length != 0)
        {
            existingPathPrefix = PhCreateString2(&directoryPart);

            if (PhDoesDirectoryExistWin32(PhGetString(existingPathPrefix)))
                break;

            PhClearReference(&existingPathPrefix);
        }

        remainingPart = directoryPart;
    }

    return existingPathPrefix;
}

// rev from GetLongPathNameW (dmex)
PPH_STRING PhGetLongPathName(
    _In_ PPH_STRINGREF FileName
    )
{
    PPH_STRING longPathName = NULL;
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    PFILE_BOTH_DIR_INFORMATION directoryInfoBuffer;
    ULONG directoryInfoLength;
    PH_STRINGREF baseNamePart;
    UNICODE_STRING baseNameUs;

    status = PhOpenFile(
        &fileHandle,
        FileName,
        FILE_READ_DATA | FILE_LIST_DIRECTORY | SYNCHRONIZE,
        NULL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return NULL;

    if (!PhGetBasePath(FileName, NULL, &baseNamePart))
        goto CleanupExit;
    if (!PhStringRefToUnicodeString(&baseNamePart, &baseNameUs))
        goto CleanupExit;

    directoryInfoLength = PAGE_SIZE;
    directoryInfoBuffer = PhAllocate(directoryInfoLength);

    status = NtQueryDirectoryFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        directoryInfoBuffer,
        directoryInfoLength,
        FileBothDirectoryInformation,
        TRUE,
        &baseNameUs,
        FALSE
        );

    if (NT_SUCCESS(status))
    {
        longPathName = PhCreateStringEx(directoryInfoBuffer->FileName, directoryInfoBuffer->FileNameLength);
    }

    PhFree(directoryInfoBuffer);

CleanupExit:
    NtClose(fileHandle);

    return longPathName;
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
    PH_TOKEN_USER tokenUser;
    UNICODE_STRING stringSid;
    WCHAR stringSidBuffer[SECURITY_MAX_SID_STRING_CHARACTERS];
    PUNICODE_STRING currentUserKeyName;

    // Get the string SID of the current user.

    if (NT_SUCCESS(status = PhGetTokenUser(PhGetOwnTokenAttributes().TokenHandle, &tokenUser)))
    {
        RtlInitEmptyUnicodeString(&stringSid, stringSidBuffer, sizeof(stringSidBuffer));

        if (PhEqualSid(tokenUser.User.Sid, (PSID)&PhSeLocalSystemSid))
        {
            status = RtlInitUnicodeStringEx(&stringSid, L".DEFAULT");
        }
        else
        {
            status = RtlConvertSidToUnicodeString(&stringSid, tokenUser.User.Sid, FALSE);
        }
    }

    // Construct the current user key name.

    if (NT_SUCCESS(status))
    {
        currentUserKeyName = &PhPredefineKeyNames[PH_KEY_CURRENT_USER_NUMBER];
        currentUserKeyName->Length = currentUserPrefix.Length + stringSid.Length;
        currentUserKeyName->MaximumLength = currentUserKeyName->Length + sizeof(UNICODE_NULL);
        currentUserKeyName->Buffer = PhAllocate(currentUserKeyName->MaximumLength);
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

            predefineHandle = ReadPointerAcquire(&PhPredefineKeyHandles[predefineIndex]);

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
 * \param FileName A string containing a file name.
 * \param DesiredAccess The desired access to the key.
 * \param Flags Optional flags for loading the hive.
 */
NTSTATUS PhLoadAppKey(
    _Out_ PHANDLE KeyHandle,
    _In_ PPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG Flags
    )
{
    NTSTATUS status;
    GUID guid;
    UNICODE_STRING fileName;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES targetAttributes;
    OBJECT_ATTRIBUTES sourceAttributes;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    PhGenerateGuid(&guid);

#if (PHNT_USE_NATIVE_APPEND)
    UNICODE_STRING guidStringUs;
    WCHAR objectNameBuffer[MAXIMUM_FILENAME_LENGTH];

    RtlInitEmptyUnicodeString(&objectName, objectNameBuffer, sizeof(objectNameBuffer));

    if (!NT_SUCCESS(status = RtlStringFromGUID(&guid, &guidStringUs)))
        return status;

    if (!NT_SUCCESS(status = RtlAppendUnicodeToString(&objectName, L"\\REGISTRY\\A\\")))
        goto CleanupExit;

    if (!NT_SUCCESS(status = RtlAppendUnicodeStringToString(&objectName, &guidStringUs)))
        goto CleanupExit;
#else
    static PH_STRINGREF namespaceString = PH_STRINGREF_INIT(L"\\REGISTRY\\A\\");
    PPH_STRING guidString;

    if (!(guidString = PhFormatGuid(&guid)))
        return STATUS_UNSUCCESSFUL;

    PhMoveReference(&guidString, PhConcatStringRef2(&namespaceString, &guidString->sr));

    if (!PhStringRefToUnicodeString(&guidString->sr, &objectName))
    {
        PhDereferenceObject(guidString);
        return STATUS_NAME_TOO_LONG;
    }
#endif

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

#if (PHNT_USE_NATIVE_APPEND)
    RtlFreeUnicodeString(&guidStringUs);
#else
    PhDereferenceObject(guidString);
#endif

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
 * Gets the information for a registry key.
 *
 * \param KeyHandle A handle to the key.
 * \param Information The registry key information, including information
 * about its subkeys and the maximum length for their names and value entries.
 */
NTSTATUS PhQueryKeyInformation(
    _In_ HANDLE KeyHandle,
    _Out_opt_ PKEY_FULL_INFORMATION Information
    )
{
    NTSTATUS status;
    ULONG bufferLength = 0;

    status = NtQueryKey(
        KeyHandle,
        KeyFullInformation,
        Information,
        sizeof(KEY_FULL_INFORMATION),
        &bufferLength
        );

    return status;
}

/**
 * Gets the last write time for a registry key without allocating memory. (dmex)
 *
 * \param KeyHandle A handle to the key.
 * \param LastWriteTime The last write time of the key.
 */
NTSTATUS PhQueryKeyLastWriteTime(
    _In_ HANDLE KeyHandle,
    _Out_ PLARGE_INTEGER LastWriteTime
    )
{
    NTSTATUS status;
    KEY_BASIC_INFORMATION basicInfo = { 0 };
    ULONG bufferLength = 0;

    status = NtQueryKey(
        KeyHandle,
        KeyBasicInformation,
        &basicInfo,
        UFIELD_OFFSET(KEY_BASIC_INFORMATION, Name),
        &bufferLength
        );

    if (status == STATUS_BUFFER_OVERFLOW && basicInfo.LastWriteTime.QuadPart != 0)
    {
        *LastWriteTime = basicInfo.LastWriteTime;
        return STATUS_SUCCESS;
    }
    else
    {
        PKEY_BASIC_INFORMATION buffer;

        status = PhQueryKey(
            KeyHandle,
            KeyBasicInformation,
            &buffer
            );

        if (NT_SUCCESS(status))
        {
            memcpy(LastWriteTime, &buffer->LastWriteTime, sizeof(LARGE_INTEGER));
            PhFree(buffer);
        }
    }

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

    if (ValueName && ValueName->Length)
    {
        if (!PhStringRefToUnicodeString(ValueName, &valueName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&valueName, NULL, 0);
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

NTSTATUS PhSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    NTSTATUS status;
    UNICODE_STRING valueName;

    if (ValueName)
    {
        if (!PhStringRefToUnicodeString(ValueName, &valueName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&valueName, NULL, 0);
    }

    status = NtSetValueKey(
        KeyHandle,
        &valueName,
        0,
        ValueType,
        Buffer,
        BufferLength
        );

    return status;
}

NTSTATUS PhDeleteValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName
    )
{
    UNICODE_STRING valueName;

    if (ValueName)
    {
        if (!PhStringRefToUnicodeString(ValueName, &valueName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&valueName, NULL, 0);
    }

    return NtDeleteValueKey(KeyHandle, &valueName);
}

NTSTATUS PhEnumerateKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_INFORMATION_CLASS InformationClass,
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
            InformationClass,
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
                InformationClass,
                buffer,
                bufferSize,
                &bufferSize
                );
        }

        if (!NT_SUCCESS(status))
            break;

        if (!Callback(KeyHandle, buffer, Context))
            break;

        index++;
    }

    PhFree(buffer);

    return status;
}

NTSTATUS PhEnumerateValueKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_VALUE_INFORMATION_CLASS InformationClass,
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
        status = NtEnumerateValueKey(
            KeyHandle,
            index,
            InformationClass,
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

            status = NtEnumerateValueKey(
                KeyHandle,
                index,
                InformationClass,
                buffer,
                bufferSize,
                &bufferSize
                );
        }

        if (!NT_SUCCESS(status))
            break;

        if (!Callback(KeyHandle, buffer, Context))
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
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions
    )
{
    return PhCreateFileWin32Ex(
        FileHandle,
        FileName,
        DesiredAccess,
        NULL,
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
 * \param AllocationSize The initial allocation size if the file is being created, overwritten, or superseded.
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
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG CreateStatus
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0
        );

    if (status == STATUS_SHARING_VIOLATION &&
        KsiLevel() >= KphLevelMed &&
        (DesiredAccess & KPH_FILE_READ_ACCESS) == DesiredAccess &&
        CreateDisposition == KPH_FILE_READ_DISPOSITION)
    {
        status = KphCreateFile(
            &fileHandle,
            DesiredAccess,
            &objectAttributes,
            &ioStatusBlock,
            AllocationSize,
            FileAttributes,
            ShareAccess,
            CreateDisposition,
            CreateOptions,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK
            );
    }

    RtlFreeUnicodeString(&fileName);

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (CreateStatus)
        *CreateStatus = (ULONG)ioStatusBlock.Information;

    return status;
}

NTSTATUS PhCreateFileWin32ExAlt(
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
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    EXTENDED_CREATE_INFORMATION extendedInfo;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    memset(&extendedInfo, 0, sizeof(EXTENDED_CREATE_INFORMATION));
    extendedInfo.ExtendedCreateFlags = CreateFlags;

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions | FILE_CONTAINS_EXTENDED_CREATE_INFORMATION,
        &extendedInfo,
        sizeof(EXTENDED_CREATE_INFORMATION)
        );

    if (status == STATUS_SHARING_VIOLATION &&
        KsiLevel() >= KphLevelMed &&
        (DesiredAccess & KPH_FILE_READ_ACCESS) == DesiredAccess &&
        CreateDisposition == KPH_FILE_READ_DISPOSITION)
    {
        status = KphCreateFile(
            &fileHandle,
            DesiredAccess,
            &objectAttributes,
            &ioStatusBlock,
            AllocationSize,
            FileAttributes,
            ShareAccess,
            CreateDisposition,
            CreateOptions | FILE_CONTAINS_EXTENDED_CREATE_INFORMATION,
            &extendedInfo,
            sizeof(EXTENDED_CREATE_INFORMATION),
            IO_IGNORE_SHARE_ACCESS_CHECK
            );
    }

    RtlFreeUnicodeString(&fileName);

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (CreateStatus)
        *CreateStatus = (ULONG)ioStatusBlock.Information;

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
 * \return Successful or errant status.
 */
NTSTATUS PhCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ PPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0
        );

    if (status == STATUS_SHARING_VIOLATION &&
        KsiLevel() >= KphLevelMed &&
        (DesiredAccess & KPH_FILE_READ_ACCESS) == DesiredAccess &&
        CreateDisposition == KPH_FILE_READ_DISPOSITION)
    {
        status = KphCreateFile(
            &fileHandle,
            DesiredAccess,
            &objectAttributes,
            &ioStatusBlock,
            NULL,
            FileAttributes,
            ShareAccess,
            CreateDisposition,
            CreateOptions,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK
            );
    }

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    return status;
}

/**
 * Creates or opens a file.
 *
 * \param FileHandle A variable that receives the file handle.
 * \param FileName The Win32 file name.
 * \param DesiredAccess The desired access to the file.
 * \param RootDirectory The root object directory for the file.
 * \param AllocationSize The initial allocation size if the file is being created, overwritten, or superseded.
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
 * \return Successful or errant status.
 */
NTSTATUS PhCreateFileEx(
    _Out_ PHANDLE FileHandle,
    _In_ PPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG CreateStatus
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0
        );

    if (status == STATUS_SHARING_VIOLATION &&
        KsiLevel() >= KphLevelMed &&
        (DesiredAccess & KPH_FILE_READ_ACCESS) == DesiredAccess &&
        CreateDisposition == KPH_FILE_READ_DISPOSITION)
    {
        status = KphCreateFile(
            &fileHandle,
            DesiredAccess,
            &objectAttributes,
            &ioStatusBlock,
            AllocationSize,
            FileAttributes,
            ShareAccess,
            CreateDisposition,
            CreateOptions,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK
            );
    }

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (CreateStatus)
        *CreateStatus = (ULONG)ioStatusBlock.Information;

    return status;
}

NTSTATUS PhOpenFileWin32(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
    )
{
    return PhOpenFileWin32Ex(
        FileHandle,
        FileName,
        DesiredAccess,
        ShareAccess,
        OpenOptions,
        NULL
        );
}

NTSTATUS PhOpenFileWin32Ex(
    _Out_ PHANDLE FileHandle,
    _In_ PCWSTR FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions,
    _Out_opt_ PULONG OpenStatus
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        ShareAccess,
        OpenOptions
        );

    RtlFreeUnicodeString(&fileName);

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (OpenStatus)
        *OpenStatus = (ULONG)ioStatusBlock.Information;

    return status;
}

NTSTATUS PhOpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ PPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions,
    _Out_opt_ PULONG OpenStatus
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        ShareAccess,
        OpenOptions
        );

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    if (OpenStatus)
    {
        *OpenStatus = (ULONG)ioStatusBlock.Information;
    }

    return status;
}

// rev from OpenFileById
NTSTATUS PhOpenFileById(
    _Out_ PHANDLE FileHandle,
    _In_ HANDLE VolumeHandle,
    _In_ PPH_FILE_ID_DESCRIPTOR FileId,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    switch (FileId->Type)
    {
    case FileIdType:
        {
            fileName.Length = sizeof(LONGLONG);
            fileName.MaximumLength = sizeof(LONGLONG);
            fileName.Buffer = (PWSTR)&FileId->FileId.QuadPart;
        }
        break;
    case ObjectIdType:
        {
            fileName.Length = sizeof(GUID);
            fileName.MaximumLength = sizeof(GUID);
            fileName.Buffer = (PWSTR)&FileId->ObjectId;
        }
        break;
    case ExtendedFileIdType:
        {
            fileName.Length = sizeof(FILE_ID_128);
            fileName.MaximumLength = sizeof(FILE_ID_128);
            fileName.Buffer = (PWSTR)&FileId->ExtendedFileId.Identifier;
        }
        break;
    default:
        return STATUS_UNSUCCESSFUL;
    }

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        VolumeHandle,
        NULL
        );

    status = NtOpenFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        ShareAccess,
        OpenOptions | FILE_OPEN_BY_FILE_ID
        );

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    return status;
}

// rev from ReOpenFile (dmex)
/**
 * Reopens the specified file handle with different access rights, sharing mode, and flags.
 * Note: This function creates new FILE_OBJECTs compared to other functions simply referencing the existing object.
 *
 * \param FileHandle A variable that receives the file handle.
 * \param OriginalFileHandle A handle to the object to be reopened.
 * \param DesiredAccess The desired access to the file.
 * \param ShareAccess The file access granted to other threads.
 * \param OpenOptions The options to apply when the file is opened.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhReOpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ HANDLE OriginalFileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    RtlInitEmptyUnicodeString(&fileName, NULL, 0);
    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        OriginalFileHandle,
        NULL
        );

    status = NtCreateFile(
        &fileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        0,
        ShareAccess,
        FILE_OPEN,
        OpenOptions,
        NULL,
        0
        );

    if (status == STATUS_SHARING_VIOLATION &&
        KsiLevel() >= KphLevelMed &&
        (DesiredAccess & KPH_FILE_READ_ACCESS) == DesiredAccess)
    {
        assert(KPH_FILE_READ_DISPOSITION == FILE_OPEN);

        status = KphCreateFile(
            &fileHandle,
            DesiredAccess,
            &objectAttributes,
            &ioStatusBlock,
            NULL,
            0,
            ShareAccess,
            FILE_OPEN,
            OpenOptions,
            NULL,
            0,
            IO_IGNORE_SHARE_ACCESS_CHECK
            );
    }

    if (NT_SUCCESS(status))
    {
        *FileHandle = fileHandle;
    }

    return status;
}

NTSTATUS PhReadFile(
    _In_ HANDLE FileHandle,
    _In_ PVOID Buffer,
    _In_opt_ ULONG NumberOfBytesToRead,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _Out_opt_ PULONG NumberOfBytesRead
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtReadFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        Buffer,
        NumberOfBytesToRead,
        ByteOffset,
        NULL
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
        {
            status = isb.Status;
        }
    }

    if (NT_SUCCESS(status))
    {
        if (NumberOfBytesRead)
        {
            *NumberOfBytesRead = (ULONG)isb.Information;
        }
    }

    return status;
}

NTSTATUS PhWriteFile(
    _In_ HANDLE FileHandle,
    _In_ PVOID Buffer,
    _In_opt_ ULONG NumberOfBytesToWrite,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _Out_opt_ PULONG NumberOfBytesWritten
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtWriteFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        Buffer,
        NumberOfBytesToWrite,
        ByteOffset,
        NULL
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(FileHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
        {
            status = isb.Status;
        }
    }

    if (NT_SUCCESS(status))
    {
        if (NumberOfBytesWritten)
        {
            *NumberOfBytesWritten = (ULONG)isb.Information;
        }
    }

    return status;
}

/**
 * Queries file attributes.
 *
 * \param FileName The Win32 file name.
 * \param FileInformation A variable that receives the file information.
 */
NTSTATUS PhQueryFullAttributesFileWin32(
    _In_ PCWSTR FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryFullAttributesFile(&objectAttributes, FileInformation);

    RtlFreeUnicodeString(&fileName);

    return status;
}

NTSTATUS PhQueryFullAttributesFile(
    _In_ PPH_STRINGREF FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryFullAttributesFile(&objectAttributes, FileInformation);

    return status;
}

NTSTATUS PhQueryAttributesFileWin32(
    _In_ PCWSTR FileName,
    _Out_ PFILE_BASIC_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryAttributesFile(&objectAttributes, FileInformation);

    RtlFreeUnicodeString(&fileName);

    return status;
}

NTSTATUS PhQueryAttributesFile(
    _In_ PPH_STRINGREF FileName,
    _Out_ PFILE_BASIC_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryAttributesFile(&objectAttributes, FileInformation);

    return status;
}

// rev from RtlDoesFileExists_U (dmex)
BOOLEAN PhDoesFileExistWin32(
    _In_ PCWSTR FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFileWin32(FileName, &basicInfo);

    if (
        NT_SUCCESS(status) ||
        status == STATUS_SHARING_VIOLATION ||
        status == STATUS_ACCESS_DENIED
        )
    {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN PhDoesFileExist(
    _In_ PPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFile(FileName, &basicInfo);

    if (
        NT_SUCCESS(status) ||
        status == STATUS_SHARING_VIOLATION ||
        status == STATUS_ACCESS_DENIED
        )
    {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN PhDoesDirectoryExistWin32(
    _In_ PCWSTR FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFileWin32(FileName, &basicInfo);

    if (NT_SUCCESS(status))
    {
        if (basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            return TRUE;
    }

    return FALSE;
}

BOOLEAN PhDoesDirectoryExist(
    _In_ PPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFile(FileName, &basicInfo);

    if (NT_SUCCESS(status))
    {
        if (basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            return TRUE;
    }

    return FALSE;
}

// rev from RtlDetermineDosPathNameType_U (dmex)
RTL_PATH_TYPE PhDetermineDosPathNameType(
    _In_ PPH_STRINGREF FileName
    )
{
#if (PHNT_USE_NATIVE_PATHTYPE)
    return RtlDetermineDosPathNameType_U(FileName);
#else
    if (FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[0] == OBJ_NAME_ALTPATH_SEPARATOR)
    {
        if (FileName->Buffer[1] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[1] == OBJ_NAME_ALTPATH_SEPARATOR)
        {
            if (FileName->Buffer[2] == L'?' || FileName->Buffer[2] == L'.')
            {
                if (FileName->Buffer[3] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[3] == OBJ_NAME_ALTPATH_SEPARATOR)
                    return RtlPathTypeLocalDevice;

                if (FileName->Buffer[3] != UNICODE_NULL)
                    return RtlPathTypeUncAbsolute;

                return RtlPathTypeRootLocalDevice;
            }

            return RtlPathTypeUncAbsolute;
        }

        return RtlPathTypeRooted;
    }
    else if (FileName->Buffer[0] != UNICODE_NULL && FileName->Buffer[1] == L':')
    {
        if (FileName->Buffer[2] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[2] == OBJ_NAME_ALTPATH_SEPARATOR)
            return RtlPathTypeDriveAbsolute;

        return RtlPathTypeDriveRelative;
    }

    return RtlPathTypeRelative;
#endif
}

/**
 * Deletes a file.
 *
 * \param FileName The Win32 file name.
 */
NTSTATUS PhDeleteFileWin32(
    _In_ PCWSTR FileName
    )
{
    NTSTATUS status;
    //UNICODE_STRING fileName;
    //OBJECT_ATTRIBUTES objectAttributes;
    //
    //status = PhDosLongPathNameToNtPathNameWithStatus(
    //    FileName,
    //    &fileName,
    //    NULL,
    //    NULL
    //    );
    //
    //if (!NT_SUCCESS(status))
    //    return status;
    //
    //InitializeObjectAttributes(
    //    &objectAttributes,
    //    &fileName,
    //    OBJ_CASE_INSENSITIVE,
    //    NULL,
    //    NULL
    //    );
    //
    //status = NtDeleteFile(&objectAttributes);
    //
    //RtlFreeUnicodeString(&fileName);
    //
    //if (!NT_SUCCESS(status))
    {
        HANDLE fileHandle;

        status = PhCreateFileWin32(
            &fileHandle,
            FileName,
            DELETE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DELETE_ON_CLOSE
            );

        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_OBJECT_NAME_NOT_FOUND)
                status = STATUS_SUCCESS;
            return status;
        }

        //PhSetFileDelete(fileHandle);

        NtClose(fileHandle);
    }

    return status;
}

NTSTATUS PhDeleteFile(
    _In_ PPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    status = PhCreateFile(
        &fileHandle,
        FileName,
        DELETE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DELETE_ON_CLOSE
        );

    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
            status = STATUS_SUCCESS;
        return status;
    }

    NtClose(fileHandle);

    return status;
}

/**
* Creates a directory path recursively.
*
* \param DirectoryPath The directory path.
*/
NTSTATUS PhCreateDirectory(
    _In_ PPH_STRINGREF DirectoryPath
    )
{
    PPH_STRING directoryPath;
    PPH_STRING directoryName;
    PH_STRINGREF directoryPart;
    PH_STRINGREF remainingPart;

    if (PhDoesDirectoryExist(DirectoryPath))
        return STATUS_SUCCESS;

    directoryPath = PhGetExistingPathPrefix(DirectoryPath);

    if (PhIsNullOrEmptyString(directoryPath))
        return STATUS_UNSUCCESSFUL;

    remainingPart.Length = DirectoryPath->Length - directoryPath->Length - sizeof(OBJ_NAME_PATH_SEPARATOR);
    remainingPart.Buffer = PTR_ADD_OFFSET(DirectoryPath->Buffer, directoryPath->Length + sizeof(OBJ_NAME_PATH_SEPARATOR));

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

        if (directoryPart.Length != 0)
        {
            directoryName = PhConcatStringRef3(
                &directoryPath->sr,
                &PhNtPathSeperatorString,
                &directoryPart
                );

            if (!PhDoesDirectoryExist(&directoryName->sr))
            {
                HANDLE directoryHandle;

                if (NT_SUCCESS(PhCreateFile(
                    &directoryHandle,
                    &directoryName->sr,
                    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_CREATE,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT //| FILE_OPEN_REPARSE_POINT
                    )))
                {
                    NtClose(directoryHandle);
                }
            }

            PhMoveReference(&directoryPath, directoryName);
        }
    }

    PhClearReference(&directoryPath);

    if (!PhDoesDirectoryExist(DirectoryPath))
        return STATUS_NOT_FOUND;

    return STATUS_SUCCESS;
}

/**
* Creates a directory path recursively.
*
* \param DirectoryPath The Win32 directory path.
*/
NTSTATUS PhCreateDirectoryWin32(
    _In_ PPH_STRINGREF DirectoryPath
    )
{
    PPH_STRING directoryPath;
    PPH_STRING directoryName;
    PH_STRINGREF directoryPart;
    PH_STRINGREF remainingPart;

    if (PhDoesDirectoryExistWin32(PhGetStringRefZ(DirectoryPath)))
        return STATUS_SUCCESS;

    directoryPath = PhGetExistingPathPrefixWin32(DirectoryPath);

    if (PhIsNullOrEmptyString(directoryPath))
        return STATUS_UNSUCCESSFUL;

    remainingPart.Length = DirectoryPath->Length - directoryPath->Length - sizeof(OBJ_NAME_PATH_SEPARATOR);
    remainingPart.Buffer = PTR_ADD_OFFSET(DirectoryPath->Buffer, directoryPath->Length + sizeof(OBJ_NAME_PATH_SEPARATOR));

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

        if (directoryPart.Length != 0)
        {
            if (PhIsNullOrEmptyString(directoryPath))
            {
                PhMoveReference(&directoryPath, PhCreateString2(&directoryPart));
            }
            else
            {
                directoryName = PhConcatStringRef3(&directoryPath->sr, &PhNtPathSeperatorString, &directoryPart);

                // Check if the directory already exists. (dmex)

                if (!PhDoesDirectoryExistWin32(PhGetString(directoryName)))
                {
                    HANDLE directoryHandle;

                    // Create the directory. (dmex)

                    if (NT_SUCCESS(PhCreateFileWin32(
                        &directoryHandle,
                        PhGetString(directoryName),
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_CREATE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT //| FILE_OPEN_REPARSE_POINT
                        )))
                    {
                        NtClose(directoryHandle);
                    }
                }

                PhMoveReference(&directoryPath, directoryName);
            }
        }
    }

    PhClearReference(&directoryPath);

    if (!PhDoesDirectoryExistWin32(PhGetStringRefZ(DirectoryPath)))
        return STATUS_NOT_FOUND;

    return STATUS_SUCCESS;
}

NTSTATUS PhCreateDirectoryFullPathWin32(
    _In_ PPH_STRINGREF FileName
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PH_STRINGREF directoryPart;
    PPH_STRING directoryPath;
    PPH_STRING directory;

    if (PhGetBasePath(FileName, &directoryPart, NULL))
    {
        if (directory = PhCreateString2(&directoryPart))
        {
            if (NT_SUCCESS(PhGetFullPath(PhGetString(directory), &directoryPath, NULL)))
            {
                status = PhCreateDirectoryWin32(&directoryPath->sr);
                PhDereferenceObject(directoryPath);
            }

            PhDereferenceObject(directory);
        }
    }

    return status;
}

NTSTATUS PhCreateDirectoryFullPath(
    _In_ PPH_STRINGREF FileName
    )
{
    PH_STRINGREF directoryPart;

    if (PhGetBasePath(FileName, &directoryPart, NULL))
    {
        return PhCreateDirectory(&directoryPart);
    }

    return STATUS_UNSUCCESSFUL;
}

// NOTE: This callback handles both Native and Win32 filenames
// since they're both relative to the parent RootDirectory. (dmex)
static BOOLEAN PhDeleteDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PH_STRINGREF fileName;
    HANDLE fileHandle;

    fileName.Buffer = Information->FileName;
    fileName.Length = Information->FileNameLength;

    if (FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
    {
        if (PATH_IS_WIN32_RELATIVE_PREFIX(&fileName))
            return TRUE;

        if (NT_SUCCESS(PhCreateFileEx(
            &fileHandle,
            &fileName,
            FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
            RootDirectory,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT, // | FILE_OPEN_REPARSE_POINT
            NULL
            )))
        {
            PhEnumDirectoryFile(fileHandle, NULL, PhDeleteDirectoryCallback, NULL);

            PhSetFileDelete(fileHandle);

            NtClose(fileHandle);
        }
    }
    else
    {
        if (NT_SUCCESS(PhCreateFileEx(
            &fileHandle,
            &fileName,
            FILE_WRITE_ATTRIBUTES | DELETE | SYNCHRONIZE,
            RootDirectory,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, // | FILE_OPEN_REPARSE_POINT
            NULL
            )))
        {
            if (FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_READONLY) && WindowsVersion < WINDOWS_10_RS5)
            {
                FILE_BASIC_INFORMATION fileBasicInfo;

                memset(&fileBasicInfo, 0, sizeof(FILE_BASIC_INFORMATION));
                fileBasicInfo.FileAttributes = ClearFlag(Information->FileAttributes, FILE_ATTRIBUTE_READONLY);

                PhSetFileBasicInformation(fileHandle, &fileBasicInfo);
            }

            PhSetFileDelete(fileHandle);

            NtClose(fileHandle);
        }
    }

    return TRUE;
}

/**
* Deletes a directory path recursively.
*
* \param DirectoryPath The directory path.
*/
NTSTATUS PhDeleteDirectory(
    _In_ PPH_STRINGREF DirectoryPath
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;

    status = PhCreateFile(
        &directoryHandle,
        DirectoryPath,
        FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
        );

    if (NT_SUCCESS(status))
    {
        // Remove any files or folders inside the directory. (dmex)
        status = PhEnumDirectoryFile(
            directoryHandle,
            NULL,
            PhDeleteDirectoryCallback,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            // Remove the directory. (dmex)
            status = PhSetFileDelete(directoryHandle);
        }

        NtClose(directoryHandle);
    }

    if (!PhDoesDirectoryExist(DirectoryPath))
        return STATUS_SUCCESS;

    return status;
}

/**
* Deletes a directory path recursively.
*
* \param DirectoryPath The Win32 directory path.
*/
NTSTATUS PhDeleteDirectoryWin32(
    _In_ PPH_STRINGREF DirectoryPath
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;

    status = PhCreateFileWin32(
        &directoryHandle,
        PhGetStringRefZ(DirectoryPath),
        FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
        );

    if (NT_SUCCESS(status))
    {
        // Remove any files or folders inside the directory. (dmex)
        status = PhEnumDirectoryFile(
            directoryHandle,
            NULL,
            PhDeleteDirectoryCallback,
            DirectoryPath
            );

        if (NT_SUCCESS(status))
        {
            // Remove the directory. (dmex)
            status = PhSetFileDelete(directoryHandle);
        }

        NtClose(directoryHandle);
    }

    if (!PhDoesDirectoryExistWin32(PhGetStringRefZ(DirectoryPath)))
        return STATUS_SUCCESS;

    return status;
}

NTSTATUS PhDeleteDirectoryFullPath(
    _In_ PPH_STRINGREF FileName
    )
{
    PH_STRINGREF directoryPart;

    if (PhGetBasePath(FileName, &directoryPart, NULL))
    {
        return PhDeleteDirectory(&directoryPart);
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS PhCopyFileWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    HANDLE newFileHandle;
    FILE_BASIC_INFORMATION basicInfo;
    LARGE_INTEGER newFileSize;
    IO_STATUS_BLOCK isb;
    PBYTE buffer;

    status = PhCreateFileWin32(
        &fileHandle,
        OldFileName,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileBasicInformation(fileHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetFileSize(fileHandle, &newFileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateFileWin32Ex(
        &newFileHandle,
        NewFileName,
        FILE_GENERIC_WRITE,
        &newFileSize,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    buffer = PhAllocatePage(PAGE_SIZE * 2, NULL);

    if (!buffer)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    while (TRUE)
    {
        status = NtReadFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            PAGE_SIZE * 2,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;
        if (isb.Information == 0)
            break;

        status = NtWriteFile(
            newFileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            (ULONG)isb.Information,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;
        if (isb.Information == 0)
            break;
    }

    PhFreePage(buffer);

    if (status == STATUS_END_OF_FILE)
    {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status))
    {
        PhSetFileBasicInformation(
            newFileHandle,
            &basicInfo
            );
    }
    else
    {
        PhSetFileDelete(newFileHandle);
    }

    NtClose(newFileHandle);

CleanupExit:
    NtClose(fileHandle);

    return status;
}

NTSTATUS PhCopyFileChunkDirectIoWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE sourceHandle;
    HANDLE destinationHandle;
    FILE_BASIC_INFORMATION basicInfo;
    FILE_FS_SECTOR_SIZE_INFORMATION sourceSectorInfo = { 0 };
    FILE_FS_SECTOR_SIZE_INFORMATION destinationSectorInfo = { 0 };
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER sourceOffset = { 0 };
    LARGE_INTEGER destinationOffset = { 0 };
    LARGE_INTEGER fileSize;
    SIZE_T numberOfBytes;
    ULONG alignSize;
    ULONG blockSize;

    if (!NtCopyFileChunk_Import())
        return STATUS_NOT_SUPPORTED;

    status = PhCreateFileWin32ExAlt(
        &sourceHandle,
        OldFileName,
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING,
        EX_CREATE_FLAG_FILE_SOURCE_OPEN_FOR_COPY,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileBasicInformation(sourceHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetFileSize(sourceHandle, &fileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateFileWin32ExAlt(
        &destinationHandle,
        NewFileName,
        FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING,
        EX_CREATE_FLAG_FILE_DEST_OPEN_FOR_COPY,
        &fileSize,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // https://learn.microsoft.com/en-us/windows/win32/w8cookbook/advanced-format--4k--disk-compatibility-update
    NtQueryVolumeInformationFile(
        sourceHandle,
        &ioStatusBlock,
        &sourceSectorInfo,
        sizeof(FILE_FS_SECTOR_SIZE_INFORMATION),
        FileFsSectorSizeInformation
        );

    NtQueryVolumeInformationFile(
        destinationHandle,
        &ioStatusBlock,
        &destinationSectorInfo,
        sizeof(FILE_FS_SECTOR_SIZE_INFORMATION),
        FileFsSectorSizeInformation
        );

    // Non-cached I/O requires 'blockSize' be sector-aligned with whichever file is opened as non-cached.
    // If both, the length should be aligned with the larger sector size of the two. (dmex)
    alignSize = max(max(sourceSectorInfo.PhysicalBytesPerSectorForPerformance, destinationSectorInfo.PhysicalBytesPerSectorForPerformance),
        max(sourceSectorInfo.PhysicalBytesPerSectorForAtomicity, destinationSectorInfo.PhysicalBytesPerSectorForAtomicity));

    // Enable BypassIO (skip error checking since might be disabled) (dmex)
    PhSetFileBypassIO(sourceHandle, TRUE);
    PhSetFileBypassIO(destinationHandle, TRUE);

    blockSize = PAGE_SIZE;
    numberOfBytes = (SIZE_T)fileSize.QuadPart;

    while (numberOfBytes != 0)
    {
        if (blockSize > numberOfBytes)
            blockSize = (ULONG)numberOfBytes;
        blockSize = ALIGN_UP_BY(blockSize, alignSize);

        status = NtCopyFileChunk_Import()(
            sourceHandle,
            destinationHandle,
            NULL,
            &ioStatusBlock,
            blockSize,
            &sourceOffset,
            &destinationOffset,
            NULL,
            NULL,
            0
            );

        if (!NT_SUCCESS(status))
            break;

        destinationOffset.QuadPart += blockSize;
        sourceOffset.QuadPart += blockSize;
        numberOfBytes -= blockSize;
    }

    if (status == STATUS_END_OF_FILE)
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        status = PhSetFileSize(destinationHandle, &fileSize); // Required (dmex)
    }

    if (NT_SUCCESS(status))
    {
        status = PhSetFileBasicInformation(destinationHandle, &basicInfo);
    }

    if (!NT_SUCCESS(status))
    {
        PhSetFileDelete(destinationHandle);
    }

    NtClose(destinationHandle);

CleanupExit:
    NtClose(sourceHandle);

    return status;
}

NTSTATUS PhCopyFileChunkWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE sourceHandle;
    HANDLE destinationHandle;
    FILE_BASIC_INFORMATION basicInfo;
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER sourceOffset = { 0 };
    LARGE_INTEGER destinationOffset = { 0 };
    LARGE_INTEGER fileSize;

    if (!NtCopyFileChunk_Import())
        return STATUS_NOT_SUPPORTED;

    status = PhCreateFileWin32ExAlt(
        &sourceHandle,
        OldFileName,
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        EX_CREATE_FLAG_FILE_SOURCE_OPEN_FOR_COPY,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileBasicInformation(sourceHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetFileSize(sourceHandle, &fileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateFileWin32ExAlt(
        &destinationHandle,
        NewFileName,
        FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        EX_CREATE_FLAG_FILE_DEST_OPEN_FOR_COPY,
        &fileSize,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (fileSize.QuadPart >= ULONG_MAX)
    {
        SIZE_T numberOfBytes = (SIZE_T)fileSize.QuadPart;
        ULONG blockSize = ULONG_MAX;

        // Split into smaller blocks when the length
        // overflows the maximum chunk size. (dmex)

        while (numberOfBytes != 0)
        {
            if (blockSize > numberOfBytes)
                blockSize = (ULONG)numberOfBytes;

            status = NtCopyFileChunk_Import()(
                sourceHandle,
                destinationHandle,
                NULL,
                &ioStatusBlock,
                blockSize,
                &sourceOffset,
                &destinationOffset,
                NULL,
                NULL,
                0
                );

            if (!NT_SUCCESS(status))
                break;

            destinationOffset.QuadPart += blockSize;
            sourceOffset.QuadPart += blockSize;
            numberOfBytes -= blockSize;
        }
    }
    else
    {
        status = NtCopyFileChunk_Import()(
            sourceHandle,
            destinationHandle,
            NULL,
            &ioStatusBlock,
            (ULONG)fileSize.QuadPart,
            &sourceOffset,
            &destinationOffset,
            NULL,
            NULL,
            0
            );
    }

    if (NT_SUCCESS(status))
    {
        PhSetFileBasicInformation(
            destinationHandle,
            &basicInfo
            );
    }
    else
    {
        PhSetFileDelete(destinationHandle);
    }

    NtClose(destinationHandle);

CleanupExit:
    NtClose(sourceHandle);

    return status;
}

NTSTATUS PhMoveFileWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    ULONG renameInfoLength;
    UNICODE_STRING newFileName;
    PFILE_RENAME_INFORMATION renameInfo;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        NewFileName,
        &newFileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhCreateFileWin32(
        &fileHandle,
        OldFileName,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | DELETE | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY
        );

    if (!NT_SUCCESS(status))
    {
        RtlFreeUnicodeString(&newFileName);
        return status;
    }

    renameInfoLength = sizeof(FILE_RENAME_INFORMATION) + newFileName.Length + sizeof(UNICODE_NULL);
    renameInfo = PhAllocateZero(renameInfoLength);
    renameInfo->ReplaceIfExists = FailIfExists ? FALSE : TRUE;
    renameInfo->RootDirectory = NULL;
    renameInfo->FileNameLength = newFileName.Length;
    memcpy(renameInfo->FileName, newFileName.Buffer, newFileName.Length);
    RtlFreeUnicodeString(&newFileName);

    status = NtSetInformationFile(
        fileHandle,
        &isb,
        renameInfo,
        renameInfoLength,
        FileRenameInformation
        );

    if (status == STATUS_NOT_SAME_DEVICE)
    {
        HANDLE newFileHandle;
        LARGE_INTEGER newFileSize;
        PBYTE buffer;

        status = PhGetFileSize(fileHandle, &newFileSize);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhCreateFileWin32Ex(
            &newFileHandle,
            NewFileName,
            FILE_GENERIC_WRITE,
            &newFileSize,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            buffer = PhAllocatePage(PAGE_SIZE * 2, NULL);

            if (!buffer)
            {
                status = STATUS_NO_MEMORY;
                goto CleanupExit;
            }

            while (TRUE)
            {
                status = NtReadFile(
                    fileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    buffer,
                    PAGE_SIZE * 2,
                    NULL,
                    NULL
                    );

                if (!NT_SUCCESS(status))
                    break;
                if (isb.Information == 0)
                    break;

                status = NtWriteFile(
                    newFileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    buffer,
                    (ULONG)isb.Information,
                    NULL,
                    NULL
                    );

                if (!NT_SUCCESS(status))
                    break;
                if (isb.Information == 0)
                    break;
            }

            PhFreePage(buffer);

            if (status == STATUS_END_OF_FILE)
            {
                status = STATUS_SUCCESS;
            }

            if (status != STATUS_SUCCESS)
            {
                PhSetFileDelete(newFileHandle);
            }

            NtClose(newFileHandle);
        }
    }

CleanupExit:
    NtClose(fileHandle);
    PhFree(renameInfo);

    return status;
}

NTSTATUS PhGetProcessHeapSignature(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID HeapAddress,
    _In_ ULONG IsWow64,
    _Out_ ULONG *HeapSignature
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG heapSignature = ULONG_MAX;

    if (WindowsVersion >= WINDOWS_7)
    {
        // dt _HEAP SegmentSignature
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x8 : 0x10),
            &heapSignature,
            sizeof(ULONG),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        if (HeapSignature)
            *HeapSignature = heapSignature;
    }

    return status;
}

NTSTATUS PhGetProcessHeapFrontEndType(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID HeapAddress,
    _In_ ULONG IsWow64,
    _Out_ UCHAR *HeapFrontEndType
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    UCHAR heapFrontEndType = UCHAR_MAX;

    if (WindowsVersion >= WINDOWS_10)
    {
        // dt _HEAP FrontEndHeapType
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x0ea : 0x1a2),
            &heapFrontEndType,
            sizeof(UCHAR),
            NULL
            );
    }
    else if (WindowsVersion >= WINDOWS_8_1)
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x0d6 : 0x17a),
            &heapFrontEndType,
            sizeof(UCHAR),
            NULL
            );
    }
    else if (WindowsVersion >= WINDOWS_7)
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x0da : 0x182),
            &heapFrontEndType,
            sizeof(UCHAR),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        if (HeapFrontEndType)
            *HeapFrontEndType = heapFrontEndType;
    }

    return status;
}

NTSTATUS PhQueryProcessHeapInformation(
    _In_ HANDLE ProcessId,
    _Out_ PPH_PROCESS_DEBUG_HEAP_INFORMATION* HeapInformation
    )
{
    NTSTATUS status;
    PRTL_DEBUG_INFORMATION debugBuffer = NULL;
    PPH_PROCESS_DEBUG_HEAP_INFORMATION heapDebugInfo = NULL;

    for (ULONG i = 0x400000; ; i *= 2) // rev from Heap32First/Heap32Next (dmex)
    {
        if (!(debugBuffer = RtlCreateQueryDebugBuffer(i, FALSE)))
            return STATUS_UNSUCCESSFUL;

        status = RtlQueryProcessDebugInformation(
            ProcessId,
            RTL_QUERY_PROCESS_HEAP_SUMMARY | RTL_QUERY_PROCESS_HEAP_ENTRIES | RTL_QUERY_PROCESS_NONINVASIVE,
            debugBuffer
            );

        if (!NT_SUCCESS(status))
        {
            RtlDestroyQueryDebugBuffer(debugBuffer);
            debugBuffer = NULL;
        }

        if (NT_SUCCESS(status) || status != STATUS_NO_MEMORY)
            break;

        if (2 * i <= i)
        {
            status = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    if (!NT_SUCCESS(status))
        return status;

    if (!debugBuffer->Heaps)
    {
        // The RtlQueryProcessDebugInformation function has two bugs on some versions
        // when querying the ProcessId for a frozen (suspended) immersive process. (dmex)
        //
        // 1) It'll deadlock the current thread for 30 seconds.
        // 2) It'll return STATUS_SUCCESS but with a NULL Heaps buffer.
        //
        // A workaround was implemented using PhCreateExecutionRequiredRequest() (dmex)

        RtlDestroyQueryDebugBuffer(debugBuffer);
        return STATUS_UNSUCCESSFUL;
    }

    if (WindowsVersion > WINDOWS_11)
    {
        heapDebugInfo = PhAllocateZero(sizeof(PH_PROCESS_DEBUG_HEAP_INFORMATION) + ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->NumberOfHeaps * sizeof(PH_PROCESS_DEBUG_HEAP_ENTRY));
        heapDebugInfo->NumberOfHeaps = ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->NumberOfHeaps;
    }
    else
    {
        heapDebugInfo = PhAllocateZero(sizeof(PH_PROCESS_DEBUG_HEAP_INFORMATION) + ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->NumberOfHeaps * sizeof(PH_PROCESS_DEBUG_HEAP_ENTRY));
        heapDebugInfo->NumberOfHeaps = ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->NumberOfHeaps;
    }

    heapDebugInfo->DefaultHeap = debugBuffer->ProcessHeap;

    for (ULONG i = 0; i < heapDebugInfo->NumberOfHeaps; i++)
    {
        RTL_HEAP_INFORMATION_V2 heapInfo = { 0 };
        HANDLE processHandle;
        SIZE_T allocated = 0;
        SIZE_T committed = 0;

        if (WindowsVersion > WINDOWS_11)
        {
            heapInfo = ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->Heaps[i];
        }
        else
        {
            RTL_HEAP_INFORMATION_V1 heapInfoV1 = ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->Heaps[i];
            heapInfo.NumberOfEntries = heapInfoV1.NumberOfEntries;
            heapInfo.Entries = heapInfoV1.Entries;
            heapInfo.BytesCommitted = heapInfoV1.BytesCommitted;
            heapInfo.Flags = heapInfoV1.Flags;
            heapInfo.BaseAddress = heapInfoV1.BaseAddress;
        }

        // go through all heap entries and compute amount of allocated and committed bytes (ge0rdi)
        for (ULONG e = 0; e < heapInfo.NumberOfEntries; e++)
        {
            PRTL_HEAP_ENTRY entry = &heapInfo.Entries[e];

            if (entry->Flags & RTL_HEAP_BUSY)
                allocated += entry->Size;
            if (entry->Flags & RTL_HEAP_SEGMENT)
                committed += entry->u.s2.CommittedSize;
        }

        // sometimes computed number if committed bytes is few pages smaller than the one reported by API, lets use the higher value (ge0rdi)
        if (committed < heapInfo.BytesCommitted)
            committed = heapInfo.BytesCommitted;

        // make sure number of allocated bytes is not higher than number of committed bytes (as that would make no sense) (ge0rdi)
        if (allocated > committed)
            allocated = committed;

        heapDebugInfo->Heaps[i].Flags = heapInfo.Flags;
        heapDebugInfo->Heaps[i].Signature = ULONG_MAX;
        heapDebugInfo->Heaps[i].HeapFrontEndType = UCHAR_MAX;
        heapDebugInfo->Heaps[i].NumberOfEntries = heapInfo.NumberOfEntries;
        heapDebugInfo->Heaps[i].BaseAddress = heapInfo.BaseAddress;
        heapDebugInfo->Heaps[i].BytesAllocated = allocated;
        heapDebugInfo->Heaps[i].BytesCommitted = committed;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
            ProcessId
            )))
        {
            ULONG signature = ULONG_MAX;
            UCHAR frontEndType = UCHAR_MAX;
#ifndef _WIN64
            BOOLEAN isWow64 = TRUE;
#else
            BOOLEAN isWow64 = FALSE;

            PhGetProcessIsWow64(processHandle, &isWow64);
#endif
            if (NT_SUCCESS(PhGetProcessHeapSignature(
                processHandle,
                heapInfo.BaseAddress,
                isWow64,
                &signature
                )))
            {
                heapDebugInfo->Heaps[i].Signature = signature;
            }

            if (NT_SUCCESS(PhGetProcessHeapFrontEndType(
                processHandle,
                heapInfo.BaseAddress,
                isWow64,
                &frontEndType
                )))
            {
                heapDebugInfo->Heaps[i].HeapFrontEndType = frontEndType;
            }

            NtClose(processHandle);
        }
    }

    if (HeapInformation)
        *HeapInformation = heapDebugInfo;
    else
        PhFree(heapDebugInfo);

    if (debugBuffer)
        RtlDestroyQueryDebugBuffer(debugBuffer);

    return STATUS_SUCCESS;
}

// Queries if the specified architecture is supported on the current system,
// either natively or by any form of compatibility or emulation layer.
// rev from kernelbase!GetMachineTypeAttributes (dmex)
NTSTATUS PhGetMachineTypeAttributes(
    _In_ USHORT Machine,
    _Out_ MACHINE_ATTRIBUTES* Attributes
    )
{
    NTSTATUS status;
    HANDLE input[1] = { 0 };
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION output[6] = { 0 };
    ULONG returnLength;

    status = NtQuerySystemInformationEx(
        SystemSupportedProcessorArchitectures2,
        input,
        sizeof(input),
        output,
        sizeof(output),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        MACHINE_ATTRIBUTES attributes;

        memset(&attributes, 0, sizeof(MACHINE_ATTRIBUTES));

        for (ULONG i = 0; i < returnLength / sizeof(SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION); i++)
        {
            if (output[i].Machine == Machine)
            {
                if (output[i].KernelMode)
                    SetFlag(attributes, KernelEnabled);
                if (output[i].UserMode)
                    SetFlag(attributes, UserEnabled);
                if (output[i].WoW64Container)
                    SetFlag(attributes, Wow64Container);
                break;
            }
        }

        *Attributes = attributes;
    }

    return status;
}

// rev from KernelBase!QueryProcessMachine (jxy-s)
NTSTATUS PhGetProcessArchitecture(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT ProcessArchitecture
    )
{
    NTSTATUS status;
    HANDLE input[1] = { ProcessHandle };
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION output[6];
    ULONG returnLength;

    status = NtQuerySystemInformationEx(
        SystemSupportedProcessorArchitectures2,
        input,
        sizeof(input),
        output,
        sizeof(output),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        for (ULONG i = 0; i < returnLength / sizeof(SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION); i++)
        {
            if (output[i].Process)
            {
                *ProcessArchitecture = (USHORT)output[i].Machine;
                return STATUS_SUCCESS;
            }
        }

        status = STATUS_NOT_FOUND;
    }

    return status;
}

NTSTATUS PhGetProcessImageBaseAddress(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* ImageBaseAddress
    )
{
    NTSTATUS status;
    PVOID pebAddress;
    PVOID baseAddress;
#ifdef _WIN64
    BOOLEAN isWow64;

    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        ULONG imageBaseAddress32;

        status = PhGetProcessPeb32(ProcessHandle, &pebAddress);

        if (!NT_SUCCESS(status))
            return status;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebAddress, UFIELD_OFFSET(PEB32, ImageBaseAddress)),
            &imageBaseAddress32,
            sizeof(ULONG),
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;

        baseAddress = UlongToPtr(imageBaseAddress32);
    }
    else
#endif
    {
        PVOID imageBaseAddress;

        status = PhGetProcessPeb(ProcessHandle, &pebAddress);

        if (!NT_SUCCESS(status))
            return status;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebAddress, UFIELD_OFFSET(PEB, ImageBaseAddress)),
            &imageBaseAddress,
            sizeof(PVOID),
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;

        baseAddress = imageBaseAddress;
    }

    if (NT_SUCCESS(status))
    {
        *ImageBaseAddress = baseAddress;
    }

    return status;
}

NTSTATUS PhGetProcessCodePage(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT ProcessCodePage
    )
{
    NTSTATUS status;
    USHORT codePage = 0;

    if (WindowsVersion >= WINDOWS_11)
    {
        PVOID pebBaseAddress;
#ifdef _WIN64
        BOOLEAN isWow64 = FALSE;

        PhGetProcessIsWow64(ProcessHandle, &isWow64);

        if (isWow64)
        {
            status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB32, ActiveCodePage)),
                &codePage,
                sizeof(USHORT),
                NULL
                );
        }
        else
#endif
        {
            status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, ActiveCodePage)),
                &codePage,
                sizeof(USHORT),
                NULL
                );
        }
    }
    else
    {
        PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
        PVOID nlsAnsiCodePage;

        status = PhGetProcessRuntimeLibrary(
            ProcessHandle,
            &runtimeLibrary,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetProcedureAddressRemote(
            ProcessHandle,
            &runtimeLibrary->NtdllFileName,
            "NlsAnsiCodePage",
            0,
            &nlsAnsiCodePage,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = NtReadVirtualMemory(
            ProcessHandle,
            nlsAnsiCodePage,
            &codePage,
            sizeof(USHORT),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        *ProcessCodePage = codePage;
    }

CleanupExit:
    return status;
}

NTSTATUS PhGetProcessConsoleCodePage(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN ConsoleOutputCP,
    _Out_ PUSHORT ConsoleCodePage
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInformation;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PVOID getConsoleCP = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        ConsoleOutputCP ? "GetConsoleOutputCP" : "GetConsoleCP",
        0,
        &getConsoleCP,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            return status;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        0,
        0,
        0,
        0,
        getConsoleCP,
        NULL,
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, PhTimeoutFromMillisecondsEx(1000));

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetThreadBasicInformation(threadHandle, &basicInformation);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    *ConsoleCodePage = (USHORT)basicInformation.ExitStatus;

CleanupExit:
    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

NTSTATUS PhGetProcessSecurityDomain(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONGLONG SecurityDomain
    )
{
    NTSTATUS status;
    PROCESS_SECURITY_DOMAIN_INFORMATION processSecurityDomainInfo;

    memset(&processSecurityDomainInfo, 0, sizeof(PROCESS_SECURITY_DOMAIN_INFORMATION));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessSecurityDomainInformation,
        &processSecurityDomainInfo,
        sizeof(PROCESS_SECURITY_DOMAIN_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *SecurityDomain = processSecurityDomainInfo.SecurityDomain;
    }

    return status;
}

NTSTATUS PhGetProcessServerSilo(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ServerSilo
    )
{
    NTSTATUS status;
    PROCESS_MEMBERSHIP_INFORMATION processMembershipInfo;

    memset(&processMembershipInfo, 0, sizeof(PROCESS_MEMBERSHIP_INFORMATION));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessMembershipInformation,
        &processMembershipInfo,
        sizeof(PROCESS_MEMBERSHIP_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ServerSilo = processMembershipInfo.ServerSiloId;
    }

    return status;
}

NTSTATUS PhGetProcessSequenceNumber(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONGLONG SequenceNumber
    )
{
    NTSTATUS status;

    if (KsiLevel() >= KphLevelLow)
    {
        // The driver exposes this information earlier than ProcessSequenceNumber was introduced.
        // Where not available it synthesizes it for informer messages, for consistency use it if
        // it's enabled.
        status = KphQueryInformationProcess(
            ProcessHandle,
            KphProcessSequenceNumber,
            SequenceNumber,
            sizeof(ULONGLONG),
            NULL
            );
    }
    else
    {
        ULONGLONG sequenceNumber;

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessSequenceNumber,
            &sequenceNumber,
            sizeof(ULONGLONG),
            NULL
            );

        if (status == STATUS_INVALID_INFO_CLASS)
        {
            PROCESS_TELEMETRY_ID_INFORMATION telemetryInfo;

            memset(&telemetryInfo, 0, sizeof(PROCESS_TELEMETRY_ID_INFORMATION));

            // ProcessTelemetryIdInformation exposes the process sequence number (and process start
            // key) earlier than ProcessSequenceNumber was introduced.
            status = NtQueryInformationProcess(
                ProcessHandle,
                ProcessTelemetryIdInformation,
                &telemetryInfo,
                sizeof(PROCESS_TELEMETRY_ID_INFORMATION),
                NULL
                );

            if (status == STATUS_BUFFER_OVERFLOW)
                status = STATUS_SUCCESS;

            if (NT_SUCCESS(status))
            {
                if (RTL_CONTAINS_FIELD(&telemetryInfo, telemetryInfo.HeaderSize, ProcessSequenceNumber))
                {
                    sequenceNumber = telemetryInfo.ProcessSequenceNumber;
                }
                else
                {
                    status = STATUS_INVALID_INFO_CLASS;
                }
            }
        }

        if (NT_SUCCESS(status))
        {
            *SequenceNumber = sequenceNumber;
        }
    }

    return status;
}

NTSTATUS PhGetProcessStartKey(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONGLONG ProcessStartKey
    )
{
    NTSTATUS status;

    if (KsiLevel() >= KphLevelLow)
    {
        // The driver exposes this information earlier than ProcessSequenceNumber was introduced.
        // Where not available it synthesizes it for informer messages, for consistency use it if
        // it's enabled.
        status = KphQueryInformationProcess(
            ProcessHandle,
            KphProcessStartKey,
            ProcessStartKey,
            sizeof(ULONGLONG),
            NULL
            );
    }
    else
    {
        ULONGLONG processSequenceNumber;

        status = PhGetProcessSequenceNumber(
            ProcessHandle,
            &processSequenceNumber
            );

        if (NT_SUCCESS(status))
        {
            *ProcessStartKey = PH_PROCESS_EXTENSION_STARTKEY(processSequenceNumber);
        }
    }

    return status;
}

NTSTATUS PhGetProcessSystemDllInitBlock(
    _In_ HANDLE ProcessHandle,
    _Out_ PPS_SYSTEM_DLL_INIT_BLOCK* SystemDllInitBlock
    )
{
    NTSTATUS status;
    PPS_SYSTEM_DLL_INIT_BLOCK ldrInitBlock;
    PVOID ldrInitBlockAddress;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "LdrSystemDllInitBlock",
        0,
        &ldrInitBlockAddress,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    ldrInitBlock = PhAllocate(sizeof(PS_SYSTEM_DLL_INIT_BLOCK));
    memset(ldrInitBlock, 0, sizeof(PS_SYSTEM_DLL_INIT_BLOCK));

    status = NtReadVirtualMemory(
        ProcessHandle,
        ldrInitBlockAddress,
        ldrInitBlock,
        sizeof(PS_SYSTEM_DLL_INIT_BLOCK),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (RTL_CONTAINS_FIELD(ldrInitBlock, ldrInitBlock->Size, MitigationAuditOptionsMap))
        {
            *SystemDllInitBlock = ldrInitBlock;
        }
        else
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
            PhFree(ldrInitBlock);
        }
    }
    else
    {
        PhFree(ldrInitBlock);
    }

    return status;
}

NTSTATUS PhGetProcessTelemetryAppSessionGuid(
    _In_ HANDLE ProcessHandle,
    _Out_ PGUID TelemetrySessionGuid
    )
{
    NTSTATUS status;
    PROCESS_TELEMETRY_ID_INFORMATION telemetryInfo;
    ULONG returnLength;

    memset(&telemetryInfo, 0, sizeof(PROCESS_TELEMETRY_ID_INFORMATION));

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessTelemetryIdInformation,
        &telemetryInfo,
        sizeof(PROCESS_TELEMETRY_ID_INFORMATION),
        &returnLength
        );

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW && returnLength)
    {
        TelemetrySessionGuid->Data1 = telemetryInfo.ProcessId;
        TelemetrySessionGuid->Data2 = (USHORT)telemetryInfo.SessionId;
        TelemetrySessionGuid->Data3 = (USHORT)telemetryInfo.BootId;
        memcpy(TelemetrySessionGuid->Data4, &telemetryInfo.CreateTime, sizeof(telemetryInfo.CreateTime));
    }

    return status;
}

NTSTATUS PhGetProcessTelemetryIdInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_TELEMETRY_ID_INFORMATION* TelemetryInformation,
    _Out_opt_ PULONG TelemetryInformationLength
    )
{
    NTSTATUS status;
    PPROCESS_TELEMETRY_ID_INFORMATION telemetryBuffer;
    ULONG telemetryLength;
    ULONG returnLength;
    ULONG attempts;

    telemetryLength = sizeof(PROCESS_TELEMETRY_ID_INFORMATION) + SECURITY_MAX_SID_SIZE + (DOS_MAX_PATH_LENGTH * sizeof(WCHAR)) + (DOS_MAX_PATH_LENGTH * sizeof(WCHAR));
    telemetryBuffer = PhAllocateZero(telemetryLength);

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessTelemetryIdInformation,
        telemetryBuffer,
        telemetryLength,
        &returnLength
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(telemetryBuffer);
        telemetryLength = returnLength;
        telemetryBuffer = PhAllocate(telemetryLength);

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessTelemetryIdInformation,
            telemetryBuffer,
            telemetryLength,
            &returnLength
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
    {
        *TelemetryInformation = telemetryBuffer;

        if (TelemetryInformationLength)
        {
            *TelemetryInformationLength = telemetryLength;
        }
    }

    return status;
}

NTSTATUS PhGetProcessTlsBitMapCounters(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG TlsBitMapCount,
    _Out_ PULONG TlsExpansionBitMapCount
    )
{
    NTSTATUS status;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif
    PVOID pebBaseAddress;
    RTL_BITMAP tlsBitMap;
    RTL_BITMAP tlsExpansionBitMap;
    ULONG bitmapBits[2] = { 0 };
    ULONG bitmapExpansionBits[32] = { 0 };

    static_assert(sizeof(bitmapBits) == RTL_FIELD_SIZE(PEB, TlsBitmapBits), "Buffer must equal TlsBitmapBits");
    static_assert(sizeof(bitmapExpansionBits) == RTL_FIELD_SIZE(PEB, TlsExpansionBitmapBits), "Buffer must equal TlsExpansionBitmapBits");

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB32, TlsBitmapBits)),
            bitmapBits,
            sizeof(bitmapBits),
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB32, TlsExpansionBitmapBits)),
            bitmapExpansionBits,
            sizeof(bitmapExpansionBits),
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }
    else
#endif
    {
        status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, TlsBitmapBits)),
            bitmapBits,
            sizeof(bitmapBits),
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, TlsExpansionBitmapBits)),
            bitmapExpansionBits,
            sizeof(bitmapExpansionBits),
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    RtlInitializeBitMap(&tlsBitMap, bitmapBits, TLS_MINIMUM_AVAILABLE);
    RtlInitializeBitMap(&tlsExpansionBitMap, bitmapExpansionBits, TLS_EXPANSION_SLOTS);

    *TlsBitMapCount = RtlNumberOfSetBits(&tlsBitMap);
    *TlsExpansionBitMapCount = RtlNumberOfSetBits(&tlsExpansionBitMap);

CleanupExit:
    return status;
}

/**
 * Gets whether the process is running under the POSIX subsystem.
 *
 * \param ProcessHandle A handle to a process. The handle
 * must have PROCESS_QUERY_LIMITED_INFORMATION and
 * PROCESS_VM_READ access.
 * \param IsPosix A variable which receives a boolean
 * indicating whether the process is running under the
 * POSIX subsystem.
 */
NTSTATUS PhGetProcessIsPosix(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsPosix
    )
{
    NTSTATUS status;
    PVOID pebBaseAddress;
    ULONG imageSubsystem;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    if (WindowsVersion >= WINDOWS_10)
    {
        *IsPosix = FALSE; // Not supported (dmex)
        return STATUS_SUCCESS;
    }

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            return status;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB32, ImageSubsystem)),
            &imageSubsystem,
            sizeof(ULONG),
            NULL
            );
    }
    else
#endif
    {
        status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress);

        if (!NT_SUCCESS(status))
            return status;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, ImageSubsystem)),
            &imageSubsystem,
            sizeof(ULONG),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        *IsPosix = imageSubsystem == IMAGE_SUBSYSTEM_POSIX_CUI;
    }

    return status;
}

BOOLEAN PhIsFirmwareSupported(
    VOID
    )
{
    UNICODE_STRING variableName = RTL_CONSTANT_STRING(L" ");
    ULONG variableValueLength = 0;
    GUID vendorGuid = { 0 };

    if (NtQuerySystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        NULL,
        &variableValueLength,
        NULL
        ) == STATUS_VARIABLE_NOT_FOUND)
    {
        return TRUE;
    }

    return FALSE;
}

// rev from GetFirmwareEnvironmentVariableW (dmex)
NTSTATUS PhGetFirmwareEnvironmentVariable(
    _In_ PPH_STRINGREF VariableName,
    _In_ PPH_STRINGREF VendorGuid,
    _Out_writes_bytes_opt_(*ValueLength) PVOID* ValueBuffer,
    _Out_opt_ PULONG ValueLength,
    _Out_opt_ PULONG ValueAttributes
    )
{
    NTSTATUS status;
    GUID vendorGuid;
    UNICODE_STRING variableName;
    PVOID valueBuffer;
    ULONG valueLength = 0;
    ULONG valueAttributes = 0;

    PhStringRefToUnicodeString(VariableName, &variableName);

    status = PhStringToGuid(
        VendorGuid,
        &vendorGuid
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtQuerySystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        NULL,
        &valueLength,
        &valueAttributes
        );

    if (status != STATUS_BUFFER_TOO_SMALL)
        return STATUS_UNSUCCESSFUL;

    valueBuffer = PhAllocate(valueLength);
    memset(valueBuffer, 0, valueLength);

    status = NtQuerySystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        valueBuffer,
        &valueLength,
        &valueAttributes
        );

    if (NT_SUCCESS(status))
    {
        if (ValueBuffer)
            *ValueBuffer = valueBuffer;
        else
            PhFree(valueBuffer);

        if (ValueLength)
            *ValueLength = valueLength;

        if (ValueAttributes)
            *ValueAttributes = valueAttributes;
    }
    else
    {
        PhFree(valueBuffer);
    }

    return status;
}

NTSTATUS PhSetFirmwareEnvironmentVariable(
    _In_ PPH_STRINGREF VariableName,
    _In_ PPH_STRINGREF VendorGuid,
    _In_reads_bytes_opt_(ValueLength) PVOID ValueBuffer,
    _In_ ULONG ValueLength,
    _In_ ULONG Attributes
    )
{
    NTSTATUS status;
    GUID vendorGuid;
    UNICODE_STRING variableName;

    PhStringRefToUnicodeString(VariableName, &variableName);

    status = PhStringToGuid(
        VendorGuid,
        &vendorGuid
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtSetSystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        ValueBuffer,
        ValueLength,
        Attributes
        );

    return status;
}

NTSTATUS PhEnumFirmwareEnvironmentValues(
    _In_ SYSTEM_ENVIRONMENT_INFORMATION_CLASS InformationClass,
    _Out_ PVOID* Variables
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferLength;

    bufferLength = PAGE_SIZE;
    buffer = PhAllocate(bufferLength);

    while (TRUE)
    {
        status = NtEnumerateSystemEnvironmentValuesEx(
            InformationClass,
            buffer,
            &bufferLength
            );

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferLength);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        *Variables = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

NTSTATUS PhSetSystemEnvironmentBootToFirmware(
    VOID
    )
{
    static const GUID EFI_GLOBAL_VARIABLE_GUID = { 0x8be4df61, 0x93ca, 0x11d2, { 0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c } };
    static UNICODE_STRING OsIndicationsSupportedName = RTL_CONSTANT_STRING(L"OsIndicationsSupported");
    static UNICODE_STRING OsIndicationsName = RTL_CONSTANT_STRING(L"OsIndications");
    const ULONG64 EFI_OS_INDICATIONS_BOOT_TO_FW_UI = 0x0000000000000001ULL;
    ULONG osIndicationsLength = sizeof(ULONG64);
    ULONG osIndicationsAttributes = 0;
    ULONG64 osIndicationsSupported = 0;
    ULONG64 osIndicationsValue = 0;
    NTSTATUS status;

    status = NtQuerySystemEnvironmentValueEx(
        &OsIndicationsSupportedName,
        &EFI_GLOBAL_VARIABLE_GUID,
        &osIndicationsSupported,
        &osIndicationsLength,
        NULL
        );

    if (status == STATUS_VARIABLE_NOT_FOUND || !(osIndicationsSupported & EFI_OS_INDICATIONS_BOOT_TO_FW_UI))
    {
        status = STATUS_NOT_SUPPORTED;
    }

    if (NT_SUCCESS(status))
    {
        status = NtQuerySystemEnvironmentValueEx(
            &OsIndicationsName,
            &EFI_GLOBAL_VARIABLE_GUID,
            &osIndicationsValue,
            &osIndicationsLength,
            &osIndicationsAttributes
            );

        if (NT_SUCCESS(status) || status == STATUS_VARIABLE_NOT_FOUND)
        {
            osIndicationsValue |= EFI_OS_INDICATIONS_BOOT_TO_FW_UI;

            if (status == STATUS_VARIABLE_NOT_FOUND)
            {
                osIndicationsAttributes = EFI_VARIABLE_NON_VOLATILE;
            }

            status = NtSetSystemEnvironmentValueEx(
                &OsIndicationsName,
                &EFI_GLOBAL_VARIABLE_GUID,
                &osIndicationsValue,
                osIndicationsLength,
                osIndicationsAttributes
                );
        }
    }

    return status;
}

// rev from RtlpCreateExecutionRequiredRequest (dmex)
/**
 * Creates a PLM execution request. This is mandatory on Windows 8 and above to prevent
 * processes freezing while querying process information and deadlocking the calling process.
 *
 * \param ProcessHandle A handle to the process for which the power request is to be created.
 * \param PowerRequestHandle A pointer to a variable that receives a handle to the new power request.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhCreateExecutionRequiredRequest(
    _In_ HANDLE ProcessHandle,
    _Out_ PHANDLE PowerRequestHandle
    )
{
    NTSTATUS status;
    HANDLE powerRequestHandle = NULL;
    COUNTED_REASON_CONTEXT powerRequestReason;
    POWER_REQUEST_ACTION powerRequestAction;

    memset(&powerRequestReason, 0, sizeof(COUNTED_REASON_CONTEXT));
    powerRequestReason.Version = POWER_REQUEST_CONTEXT_VERSION;
    powerRequestReason.Flags = POWER_REQUEST_CONTEXT_NOT_SPECIFIED;

    status = NtPowerInformation(
        PlmPowerRequestCreate,
        &powerRequestReason,
        sizeof(COUNTED_REASON_CONTEXT),
        &powerRequestHandle,
        sizeof(HANDLE)
        );

    if (!NT_SUCCESS(status))
        return status;

    memset(&powerRequestAction, 0, sizeof(POWER_REQUEST_ACTION));
    powerRequestAction.PowerRequestHandle = powerRequestHandle;
    powerRequestAction.RequestType = PowerRequestExecutionRequiredInternal;
    powerRequestAction.SetAction = TRUE;
    powerRequestAction.ProcessHandle = ProcessHandle;

    status = NtPowerInformation(
        PowerRequestAction,
        &powerRequestAction,
        sizeof(POWER_REQUEST_ACTION),
        NULL,
        0
        );

    if (NT_SUCCESS(status))
    {
        *PowerRequestHandle = powerRequestHandle;
    }
    else
    {
        NtClose(powerRequestHandle);
    }

    return status;
}

// rev from RtlpDestroyExecutionRequiredRequest
NTSTATUS PhDestroyExecutionRequiredRequest(
    _In_ HANDLE PowerRequestHandle
    )
{
    POWER_REQUEST_ACTION requestPowerAction;

    memset(&requestPowerAction, 0, sizeof(POWER_REQUEST_ACTION));
    requestPowerAction.PowerRequestHandle = PowerRequestHandle;
    requestPowerAction.RequestType = PowerRequestExecutionRequiredInternal;
    requestPowerAction.SetAction = FALSE;
    requestPowerAction.ProcessHandle = NULL;

    NtPowerInformation(
        PowerRequestAction,
        &requestPowerAction,
        sizeof(POWER_REQUEST_ACTION),
        NULL,
        0
        );

    return NtClose(PowerRequestHandle);
}

NTSTATUS PhGetProcessorNominalFrequency(
    _In_ PH_PROCESSOR_NUMBER ProcessorNumber,
    _Out_ PULONG NominalFrequency
    )
{
    NTSTATUS status;
    POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT frequencyInput;
    POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT frequencyOutput;

    memset(&frequencyInput, 0, sizeof(frequencyInput));
    frequencyInput.InternalType = PowerInternalProcessorBrandedFrequency;
    frequencyInput.ProcessorNumber.Group = ProcessorNumber.Group; // USHRT_MAX for max
    frequencyInput.ProcessorNumber.Number = (BYTE)ProcessorNumber.Number; // UCHAR_MAX for max
    frequencyInput.ProcessorNumber.Reserved = 0; // UCHAR_MAX

    memset(&frequencyOutput, 0, sizeof(frequencyOutput));
    frequencyOutput.Version = POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION;

    status = NtPowerInformation(
        PowerInformationInternal,
        &frequencyInput,
        sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT),
        &frequencyOutput,
        sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT)
        );

    if (NT_SUCCESS(status))
    {
        if (frequencyOutput.Version == POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION)
        {
            *NominalFrequency = frequencyOutput.NominalFrequency;
        }
        else
        {
            status = STATUS_INVALID_KERNEL_INFO_VERSION;
        }
    }

    return status;
}

// Process freeze/thaw support

NTSTATUS PhFreezeProcess(
    _Out_ PHANDLE FreezeHandle,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE processHandle;
    HANDLE stateHandle;

    if (!(NtCreateProcessStateChange_Import() && NtChangeProcessState_Import()))
        return STATUS_UNSUCCESSFUL;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME,
        ProcessId
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    status = NtCreateProcessStateChange_Import()(
        &stateHandle,
        STATECHANGE_SET_ATTRIBUTES,
        &objectAttributes,
        processHandle,
        0
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(processHandle);
        return status;
    }

    status = NtChangeProcessState_Import()(
        stateHandle,
        processHandle,
        ProcessStateChangeSuspend,
        NULL,
        0,
        0
        );

    if (NT_SUCCESS(status))
    {
        *FreezeHandle = stateHandle;
    }

    NtClose(processHandle);

    return status;
}

NTSTATUS PhThawProcess(
    _In_ HANDLE FreezeHandle,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (!NtChangeProcessState_Import())
        return STATUS_UNSUCCESSFUL;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME,
        ProcessId
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtChangeProcessState_Import()(
        FreezeHandle,
        processHandle,
        ProcessStateChangeResume,
        NULL,
        0,
        0
        );

    NtClose(processHandle);

    return status;
}

// Process execution request support

static PH_INITONCE PhExecutionRequestInitOnce = PH_INITONCE_INIT;
static PPH_HASHTABLE PhExecutionRequestHashtable = NULL;

typedef struct _PH_EXECUTIONREQUEST_CACHE_ENTRY
{
    HANDLE ProcessId;
    HANDLE ExecutionRequestHandle;
} PH_EXECUTIONREQUEST_CACHE_ENTRY, *PPH_EXECUTIONREQUEST_CACHE_ENTRY;

static BOOLEAN NTAPI PhExecutionRequestHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return
        ((PPH_EXECUTIONREQUEST_CACHE_ENTRY)Entry1)->ProcessId ==
        ((PPH_EXECUTIONREQUEST_CACHE_ENTRY)Entry2)->ProcessId;
}

static ULONG NTAPI PhExecutionRequestHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return HandleToUlong(((PPH_EXECUTIONREQUEST_CACHE_ENTRY)Entry)->ProcessId) / 4;
}

BOOLEAN PhInitializeExecutionRequestTable(
    VOID
    )
{
    if (PhBeginInitOnce(&PhExecutionRequestInitOnce))
    {
        PhExecutionRequestHashtable = PhCreateHashtable(
            sizeof(PH_EXECUTIONREQUEST_CACHE_ENTRY),
            PhExecutionRequestHashtableEqualFunction,
            PhExecutionRequestHashtableHashFunction,
            1
            );

        PhEndInitOnce(&PhExecutionRequestInitOnce);
    }

    return TRUE;
}

BOOLEAN PhIsProcessExecutionRequired(
    _In_ HANDLE ProcessId
    )
{
    if (PhInitializeExecutionRequestTable())
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        entry.ProcessId = ProcessId;

        if (PhFindEntryHashtable(PhExecutionRequestHashtable, &entry))
        {
            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS PhProcessExecutionRequiredEnable(
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE requestHandle = NULL;

    if (PhInitializeExecutionRequestTable())
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        entry.ProcessId = ProcessId;

        if (PhFindEntryHashtable(PhExecutionRequestHashtable, &entry))
        {
            return STATUS_SUCCESS;
        }
    }

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_LIMITED_INFORMATION,
        ProcessId
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = PhCreateExecutionRequiredRequest(processHandle, &requestHandle);

    if (NT_SUCCESS(status))
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        entry.ProcessId = ProcessId;
        entry.ExecutionRequestHandle = requestHandle;

        PhAddEntryHashtable(PhExecutionRequestHashtable, &entry);
    }

    NtClose(processHandle);

    return status;
}

NTSTATUS PhProcessExecutionRequiredDisable(
    _In_ HANDLE ProcessId
    )
{
    HANDLE requestHandle = NULL;

    if (PhInitializeExecutionRequestTable())
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY lookupEntry;
        PPH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        lookupEntry.ProcessId = ProcessId;

        if (entry = PhFindEntryHashtable(PhExecutionRequestHashtable, &lookupEntry))
        {
            requestHandle = entry->ExecutionRequestHandle;
        }
    }

    if (requestHandle)
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        entry.ProcessId = ProcessId;

        if (PhRemoveEntryHashtable(PhExecutionRequestHashtable, &entry))
        {
            PhDestroyExecutionRequiredRequest(requestHandle);
        }

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

// KnownDLLs cache support

static PH_INITONCE PhKnownDllsInitOnce = PH_INITONCE_INIT;
static PPH_HASHTABLE PhKnownDllsHashtable = NULL;

typedef struct _PH_KNOWNDLL_CACHE_ENTRY
{
    PPH_STRING FileName;
} PH_KNOWNDLL_CACHE_ENTRY, *PPH_KNOWNDLL_CACHE_ENTRY;

static BOOLEAN NTAPI PhKnownDllsHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return PhEqualStringRef(&((PPH_KNOWNDLL_CACHE_ENTRY)Entry1)->FileName->sr, &((PPH_KNOWNDLL_CACHE_ENTRY)Entry2)->FileName->sr, FALSE);
}

static ULONG NTAPI PhKnownDllsHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashStringRefEx(&((PPH_KNOWNDLL_CACHE_ENTRY)Entry)->FileName->sr, FALSE, PH_STRING_HASH_X65599);
}

static BOOLEAN NTAPI PhpKnownDllObjectsCallback(
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE sectionHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    PVOID baseAddress = NULL;
    SIZE_T viewSize = PAGE_SIZE;
    PPH_STRING fileName;

    if (!PhStringRefToUnicodeString(Name, &objectName))
        return TRUE;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        Context,
        NULL
        );

    status = NtOpenSection(
        &sectionHandle,
        SECTION_MAP_READ,
        &objectAttributes
        );

    if (!NT_SUCCESS(status))
        return TRUE;

    status = NtMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &baseAddress,
        0,
        0,
        NULL,
        &viewSize,
        ViewUnmap,
        WindowsVersion < WINDOWS_10_RS2 ? 0 : MEM_MAPPED,
        PAGE_READONLY
        );

    NtClose(sectionHandle);

    if (!NT_SUCCESS(status))
        return TRUE;

    status = PhGetProcessMappedFileName(
        NtCurrentProcess(),
        baseAddress,
        &fileName
        );

    NtUnmapViewOfSection(NtCurrentProcess(), baseAddress);

    if (NT_SUCCESS(status))
    {
        PH_KNOWNDLL_CACHE_ENTRY entry;

        entry.FileName = fileName;

        PhAddEntryHashtable(PhKnownDllsHashtable, &entry);
    }

    return TRUE;
}

VOID PhInitializeKnownDlls(
    _In_ PCWSTR ObjectName
    )
{
    UNICODE_STRING directoryName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE directoryHandle;

    RtlInitUnicodeString(&directoryName, ObjectName);
    InitializeObjectAttributes(
        &objectAttributes,
        &directoryName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    if (NT_SUCCESS(NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        )))
    {
        PhEnumDirectoryObjects(
            directoryHandle,
            PhpKnownDllObjectsCallback,
            directoryHandle
            );
        NtClose(directoryHandle);
    }
}

BOOLEAN PhInitializeKnownDllsTable(
    VOID
    )
{
    if (PhBeginInitOnce(&PhKnownDllsInitOnce))
    {
        PhKnownDllsHashtable = PhCreateHashtable(
            sizeof(PH_KNOWNDLL_CACHE_ENTRY),
            PhKnownDllsHashtableEqualFunction,
            PhKnownDllsHashtableHashFunction,
            100
            );

        PhInitializeKnownDlls(L"\\KnownDlls");
        PhInitializeKnownDlls(L"\\KnownDlls32");
#ifdef _ARM64_
        PhInitializeKnownDlls(L"\\KnownDllsArm32");
        PhInitializeKnownDlls(L"\\KnownDllsChpe32");
#endif
        PhEndInitOnce(&PhKnownDllsInitOnce);
    }

    return TRUE;
}

BOOLEAN PhIsKnownDllFileName(
    _In_ PPH_STRING FileName
    )
{
    if (PhInitializeKnownDllsTable())
    {
        PH_KNOWNDLL_CACHE_ENTRY entry;

        entry.FileName = FileName;

        if (PhFindEntryHashtable(PhKnownDllsHashtable, &entry))
        {
            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS PhGetSystemLogicalProcessorInformation(
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _Out_ PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *Buffer,
    _Out_ PULONG BufferLength
    )
{
    static ULONG initialBufferSize[] = { 0x200, 0x80, 0x100, 0x1000 };
    NTSTATUS status;
    ULONG classIndex;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    switch (RelationshipType)
    {
    case RelationProcessorCore:
        classIndex = 0;
        break;
    case RelationProcessorPackage:
        classIndex = 1;
        break;
    case RelationGroup:
        classIndex = 2;
        break;
    case RelationAll:
        classIndex = 3;
        break;
    default:
        return STATUS_INVALID_INFO_CLASS;
    }

    bufferSize = initialBufferSize[classIndex];
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformationEx(
        SystemLogicalProcessorAndGroupInformation,
        &RelationshipType,
        sizeof(LOGICAL_PROCESSOR_RELATIONSHIP),
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformationEx(
            SystemLogicalProcessorAndGroupInformation,
            &RelationshipType,
            sizeof(LOGICAL_PROCESSOR_RELATIONSHIP),
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x100000) initialBufferSize[classIndex] = bufferSize;
    *Buffer = buffer;
    *BufferLength = bufferSize;

    return status;
}

NTSTATUS PhGetSystemLogicalProcessorRelationInformation(
    _Out_ PPH_LOGICAL_PROCESSOR_INFORMATION LogicalProcessorInformation
    )
{
    NTSTATUS status;
    ULONG logicalInformationLength = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalInformation;

    status = PhGetSystemLogicalProcessorInformation(
        RelationAll,
        &logicalInformation,
        &logicalInformationLength
        );

    if (NT_SUCCESS(status))
    {
        ULONG processorCoreCount = 0;
        ULONG processorNumaCount = 0;
        ULONG processorLogicalCount = 0;
        ULONG processorPackageCount = 0;

        for (
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInfo = logicalInformation;
            (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, logicalInformationLength);
            processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
            )
        {
            switch (processorInfo->Relationship)
            {
            case RelationProcessorCore:
                {
                    processorCoreCount++;

                    for (USHORT j = 0; j < processorInfo->Processor.GroupCount; j++)
                    {
                        processorLogicalCount += PhCountBitsUlongPtr(processorInfo->Processor.GroupMask[j].Mask); // RtlNumberOfSetBitsUlongPtr
                    }
                }
                break;
            case RelationNumaNode:
                processorNumaCount++;
                break;
            case RelationProcessorPackage:
                processorPackageCount++;
                break;
            }
        }

        memset(LogicalProcessorInformation, 0, sizeof(PH_LOGICAL_PROCESSOR_INFORMATION));
        LogicalProcessorInformation->ProcessorCoreCount = processorCoreCount;
        LogicalProcessorInformation->ProcessorNumaCount = processorNumaCount;
        LogicalProcessorInformation->ProcessorLogicalCount = processorLogicalCount;
        LogicalProcessorInformation->ProcessorPackageCount = processorPackageCount;

        PhFree(logicalInformation);
    }

    return status;
}

// based on RtlIsProcessorFeaturePresent (dmex)
BOOLEAN PhIsProcessorFeaturePresent(
    _In_ ULONG ProcessorFeature
    )
{
    if (WindowsVersion < WINDOWS_NEW && ProcessorFeature < PROCESSOR_FEATURE_MAX)
    {
        return USER_SHARED_DATA->ProcessorFeatures[ProcessorFeature];
    }

    return !!IsProcessorFeaturePresent(ProcessorFeature); // RtlIsProcessorFeaturePresent
}

VOID PhGetCurrentProcessorNumber(
    _Out_ PPROCESSOR_NUMBER ProcessorNumber
    )
{
    //if (PhIsProcessorFeaturePresent(PF_RDPID_INSTRUCTION_AVAILABLE))
    //    _rdpid_u32();
    //if (PhIsProcessorFeaturePresent(PF_RDTSCP_INSTRUCTION_AVAILABLE))
    //    __rdtscp();

    memset(ProcessorNumber, 0, sizeof(PROCESSOR_NUMBER));

    RtlGetCurrentProcessorNumberEx(ProcessorNumber);
}

// based on GetActiveProcessorCount (dmex)
USHORT PhGetActiveProcessorCount(
    _In_ USHORT ProcessorGroup
    )
{
    if (PhSystemProcessorInformation.ActiveProcessorCount)
    {
        USHORT numberOfProcessors = 0;

        if (ProcessorGroup == ALL_PROCESSOR_GROUPS)
        {
            for (USHORT i = 0; i < PhSystemProcessorInformation.NumberOfProcessorGroups; i++)
            {
                numberOfProcessors += PhSystemProcessorInformation.ActiveProcessorCount[i];
            }
        }
        else
        {
            if (ProcessorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups)
            {
                numberOfProcessors = PhSystemProcessorInformation.ActiveProcessorCount[ProcessorGroup];
            }
        }

        return numberOfProcessors;
    }
    else
    {
        return PhSystemProcessorInformation.NumberOfProcessors;
    }
}

NTSTATUS PhGetProcessorNumberFromIndex(
    _In_ ULONG ProcessorIndex,
    _Out_ PPH_PROCESSOR_NUMBER ProcessorNumber
    )
{
    USHORT processorIndex = 0;

    for (USHORT processorGroup = 0; processorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups; processorGroup++)
    {
        USHORT processorCount = PhGetActiveProcessorCount(processorGroup);

        for (USHORT processorNumber = 0; processorNumber < processorCount; processorNumber++)
        {
            if (processorIndex++ == ProcessorIndex)
            {
                memset(ProcessorNumber, 0, sizeof(PH_PROCESSOR_NUMBER));
                ProcessorNumber->Group = processorGroup;
                ProcessorNumber->Number = processorNumber;
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS PhGetProcessorGroupActiveAffinityMask(
    _In_ USHORT ProcessorGroup,
    _Out_ PKAFFINITY ActiveProcessorMask
    )
{
    NTSTATUS status;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInformation;
    ULONG processorInformationLength;

    status = PhGetSystemLogicalProcessorInformation(
        RelationGroup,
        &processorInformation,
        &processorInformationLength
        );

    if (NT_SUCCESS(status))
    {
        if (ProcessorGroup < processorInformation->Group.ActiveGroupCount)
        {
            *ActiveProcessorMask = processorInformation->Group.GroupInfo[ProcessorGroup].ActiveProcessorMask;
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }

        PhFree(processorInformation);
    }

    return status;
}

NTSTATUS PhGetProcessorSystemAffinityMask(
    _Out_ PKAFFINITY ActiveProcessorsAffinityMask
    )
{
    if (PhSystemProcessorInformation.SingleProcessorGroup)
    {
        *ActiveProcessorsAffinityMask = PhSystemBasicInformation.ActiveProcessorsAffinityMask;
        return STATUS_SUCCESS;
    }
    else
    {
        PROCESSOR_NUMBER processorNumber;

        PhGetCurrentProcessorNumber(&processorNumber);

        return PhGetProcessorGroupActiveAffinityMask(processorNumber.Group, ActiveProcessorsAffinityMask);
    }
}

// rev from GetNumaHighestNodeNumber (dmex)
NTSTATUS PhGetNumaHighestNodeNumber(
    _Out_ PUSHORT NodeNumber
    )
{
    NTSTATUS status;
    SYSTEM_NUMA_INFORMATION numaProcessorMap;

    status = NtQuerySystemInformation(
        SystemNumaProcessorMap,
        &numaProcessorMap,
        sizeof(SYSTEM_NUMA_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *NodeNumber = (USHORT)numaProcessorMap.HighestNodeNumber;
    }

    return status;
}

// rev from GetNumaProcessorNodeEx (dmex)
BOOLEAN PhGetNumaProcessorNode(
    _In_ PPH_PROCESSOR_NUMBER ProcessorNumber,
    _Out_ PUSHORT NodeNumber
    )
{
    NTSTATUS status;
    SYSTEM_NUMA_INFORMATION numaProcessorMap;
    USHORT processorNode = 0;

    if (ProcessorNumber->Group >= 20 || ProcessorNumber->Number >= MAXIMUM_PROC_PER_GROUP)
    {
        *NodeNumber = USHRT_MAX;
        return FALSE;
    }

    status = NtQuerySystemInformation(
        SystemNumaProcessorMap,
        &numaProcessorMap,
        sizeof(SYSTEM_NUMA_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        *NodeNumber = USHRT_MAX;
        return FALSE;
    }

    while (
        numaProcessorMap.ActiveProcessorsGroupAffinity[processorNode].Group != ProcessorNumber->Group ||
        (numaProcessorMap.ActiveProcessorsGroupAffinity[processorNode].Mask & AFFINITY_MASK(ProcessorNumber->Number)) == 0
        )
    {
        if (++processorNode > numaProcessorMap.HighestNodeNumber)
        {
            *NodeNumber = USHRT_MAX;
            return FALSE;
        }
    }

    *NodeNumber = processorNode;
    return TRUE;
}

// rev from GetNumaProximityNodeEx (dmex)
NTSTATUS PhGetNumaProximityNode(
    _In_ ULONG ProximityId,
    _Out_ PUSHORT NodeNumber
    )
{
    NTSTATUS status;
    SYSTEM_NUMA_PROXIMITY_MAP numaProximityMap;

    memset(&numaProximityMap, 0, sizeof(SYSTEM_NUMA_PROXIMITY_MAP));
    numaProximityMap.NodeProximityId = ProximityId;

    status = NtQuerySystemInformation(
        SystemNumaProximityNodeInformation,
        &numaProximityMap,
        sizeof(SYSTEM_NUMA_PROXIMITY_MAP),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *NodeNumber = numaProximityMap.NodeNumber;
    }

    return status;
}

NTSTATUS
DECLSPEC_GUARDNOCF
PhpSetInformationVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ VIRTUAL_MEMORY_INFORMATION_CLASS VmInformationClass,
    _In_ ULONG_PTR NumberOfEntries,
    _In_reads_(NumberOfEntries) PMEMORY_RANGE_ENTRY VirtualAddresses,
    _In_reads_bytes_(VmInformationLength) PVOID VmInformation,
    _In_ ULONG VmInformationLength
    )
{
    assert(NtSetInformationVirtualMemory_Import());

    return NtSetInformationVirtualMemory_Import()(
        ProcessHandle,
        VmInformationClass,
        NumberOfEntries,
        VirtualAddresses,
        VmInformation,
        VmInformationLength
        );
}

// rev from PrefetchVirtualMemory (dmex)
/**
 * Provides an efficient mechanism to bring into memory potentially discontiguous virtual address ranges in a process address space.
 *
 * \param ProcessHandle A handle to the process whose virtual address ranges are to be prefetched.
 * \param NumberOfEntries Number of entries in the array pointed to by the VirtualAddresses parameter.
 * \param VirtualAddresses A pointer to an array of MEMORY_RANGE_ENTRY structures which each specify a virtual address range
 * to be prefetched. The virtual address ranges may cover any part of the process address space accessible by the target process.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhPrefetchVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG_PTR NumberOfEntries,
    _In_ PMEMORY_RANGE_ENTRY VirtualAddresses
    )
{
    NTSTATUS status;
    ULONG prefetchInformationFlags;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&prefetchInformationFlags, 0, sizeof(prefetchInformationFlags));

    status = PhpSetInformationVirtualMemory(
        ProcessHandle,
        VmPrefetchInformation,
        NumberOfEntries,
        VirtualAddresses,
        &prefetchInformationFlags,
        sizeof(prefetchInformationFlags)
        );

    return status;
}

// rev from OfferVirtualMemory (dmex)
//NTSTATUS PhOfferVirtualMemory(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID VirtualAddress,
//    _In_ SIZE_T NumberOfBytes,
//    _In_ OFFER_PRIORITY Priority
//    )
//{
//    NTSTATUS status;
//    MEMORY_RANGE_ENTRY virtualMemoryRange;
//    ULONG virtualMemoryFlags;
//
//    if (!NtSetInformationVirtualMemory_Import())
//        return STATUS_PROCEDURE_NOT_FOUND;
//
//    // TODO: NtQueryVirtualMemory (dmex)
//
//    memset(&virtualMemoryRange, 0, sizeof(MEMORY_RANGE_ENTRY));
//    virtualMemoryRange.VirtualAddress = VirtualAddress;
//    virtualMemoryRange.NumberOfBytes = NumberOfBytes;
//
//    memset(&virtualMemoryFlags, 0, sizeof(virtualMemoryFlags));
//    virtualMemoryFlags = Priority;
//
//    status = PhpSetInformationVirtualMemory(
//        ProcessHandle,
//        VmPagePriorityInformation,
//        1,
//        &virtualMemoryRange,
//        &virtualMemoryFlags,
//        sizeof(virtualMemoryFlags)
//        );
//
//    return status;
//}
//
// rev from DiscardVirtualMemory (dmex)
//NTSTATUS PhDiscardVirtualMemory(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID VirtualAddress,
//    _In_ SIZE_T NumberOfBytes
//    )
//{
//    NTSTATUS status;
//    MEMORY_RANGE_ENTRY virtualMemoryRange;
//    ULONG virtualMemoryFlags;
//
//    if (!NtSetInformationVirtualMemory_Import())
//        return STATUS_PROCEDURE_NOT_FOUND;
//
//    memset(&virtualMemoryRange, 0, sizeof(MEMORY_RANGE_ENTRY));
//    virtualMemoryRange.VirtualAddress = VirtualAddress;
//    virtualMemoryRange.NumberOfBytes = NumberOfBytes;
//
//    memset(&virtualMemoryFlags, 0, sizeof(virtualMemoryFlags));
//
//    status = PhpSetInformationVirtualMemory(
//        ProcessHandle,
//        VmPagePriorityInformation,
//        1,
//        &virtualMemoryRange,
//        &virtualMemoryFlags,
//        sizeof(virtualMemoryFlags)
//        );
//
//    return status;
//}
//
// rev from SetProcessValidCallTargets (dmex)
//NTSTATUS PhSetProcessValidCallTarget(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID VirtualAddress
//    )
//{
//    NTSTATUS status;
//    MEMORY_BASIC_INFORMATION basicInfo;
//    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
//    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
//    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
//    ULONG numberOfEntriesProcessed = 0;
//
//    if (!NtSetInformationVirtualMemory_Import())
//        return STATUS_PROCEDURE_NOT_FOUND;
//
//    status = NtQueryVirtualMemory(
//        ProcessHandle,
//        VirtualAddress,
//        MemoryBasicInformation,
//        &basicInfo,
//        sizeof(MEMORY_BASIC_INFORMATION),
//        NULL
//        );
//
//    if (!NT_SUCCESS(status))
//        return status;
//
//    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
//    cfgCallTargetInfo.Offset = (ULONG_PTR)VirtualAddress - (ULONG_PTR)basicInfo.AllocationBase;
//    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_VALID;
//
//    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
//    cfgCallTargetRangeInfo.VirtualAddress = basicInfo.AllocationBase;
//    cfgCallTargetRangeInfo.NumberOfBytes = basicInfo.RegionSize;
//
//    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
//    cfgCallTargetListInfo.NumberOfEntries = 1;
//    cfgCallTargetListInfo.Reserved = 0;
//    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
//    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;
//
//    status = PhpSetInformationVirtualMemory(
//        ProcessHandle,
//        VmCfgCallTargetInformation,
//        1,
//        &cfgCallTargetRangeInfo,
//        &cfgCallTargetListInfo,
//        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
//        );
//
//    if (status == STATUS_INVALID_PAGE_PROTECTION)
//        status = STATUS_SUCCESS;
//
//    return status;
//}

// rev from RtlGuardGrantSuppressedCallAccess (dmex)
NTSTATUS PhGuardGrantSuppressedCallAccess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    )
{
    NTSTATUS status;
    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
    ULONG numberOfEntriesProcessed = 0;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
    cfgCallTargetRangeInfo.VirtualAddress = PAGE_ALIGN(VirtualAddress);
    cfgCallTargetRangeInfo.NumberOfBytes = PAGE_SIZE;

    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
    cfgCallTargetInfo.Offset = BYTE_OFFSET(VirtualAddress);
    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_VALID;

    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
    cfgCallTargetListInfo.NumberOfEntries = 1;
    cfgCallTargetListInfo.Reserved = 0;
    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;

    status = PhpSetInformationVirtualMemory(
        ProcessHandle,
        VmCfgCallTargetInformation,
        1,
        &cfgCallTargetRangeInfo,
        &cfgCallTargetListInfo,
        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
        );

    if (status == STATUS_INVALID_PAGE_PROTECTION)
        status = STATUS_SUCCESS;

    return status;
}

// rev from RtlDisableXfgOnTarget (dmex)
NTSTATUS PhDisableXfgOnTarget(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    )
{
    NTSTATUS status;
    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
    ULONG numberOfEntriesProcessed = 0;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
    cfgCallTargetRangeInfo.VirtualAddress = PAGE_ALIGN(VirtualAddress);
    cfgCallTargetRangeInfo.NumberOfBytes = PAGE_SIZE;

    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
    cfgCallTargetInfo.Offset = BYTE_OFFSET(VirtualAddress);
    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_CONVERT_XFG_TO_CFG;

    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
    cfgCallTargetListInfo.NumberOfEntries = 1;
    cfgCallTargetListInfo.Reserved = 0;
    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;

    status = PhpSetInformationVirtualMemory(
        ProcessHandle,
        VmCfgCallTargetInformation,
        1,
        &cfgCallTargetRangeInfo,
        &cfgCallTargetListInfo,
        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
        );

    if (status == STATUS_INVALID_PAGE_PROTECTION)
        status = STATUS_SUCCESS;

    return status;
}

NTSTATUS PhGetSystemCompressionStoreInformation(
    _Out_ PPH_SYSTEM_STORE_COMPRESSION_INFORMATION SystemCompressionStoreInformation
    )
{
    NTSTATUS status;
    SYSTEM_STORE_INFORMATION storeInfo;
    SM_MEM_COMPRESSION_INFO_REQUEST compressionInfo;

    memset(&compressionInfo, 0, sizeof(SM_MEM_COMPRESSION_INFO_REQUEST));
    compressionInfo.Version = SYSTEM_STORE_COMPRESSION_INFORMATION_VERSION;

    memset(&storeInfo, 0, sizeof(SYSTEM_STORE_INFORMATION));
    storeInfo.Version = SYSTEM_STORE_INFORMATION_VERSION;
    storeInfo.StoreInformationClass = MemCompressionInfoRequest;
    storeInfo.Data = &compressionInfo;
    storeInfo.Length = sizeof(compressionInfo);

    status = NtQuerySystemInformation(
        SystemStoreInformation,
        &storeInfo,
        sizeof(SYSTEM_STORE_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        memset(SystemCompressionStoreInformation, 0, sizeof(PH_SYSTEM_STORE_COMPRESSION_INFORMATION));
        SystemCompressionStoreInformation->CompressionPid = compressionInfo.CompressionPid;
        SystemCompressionStoreInformation->WorkingSetSize = compressionInfo.WorkingSetSize;
        SystemCompressionStoreInformation->TotalDataCompressed = compressionInfo.TotalDataCompressed;
        SystemCompressionStoreInformation->TotalCompressedSize = compressionInfo.TotalCompressedSize;
        SystemCompressionStoreInformation->TotalUniqueDataCompressed = compressionInfo.TotalUniqueDataCompressed;
    }

    return status;
}

NTSTATUS PhGetSystemFileCacheSize(
    _Out_ PSYSTEM_FILECACHE_INFORMATION CacheInfo
    )
{
    return NtQuerySystemInformation(
        SystemFileCacheInformationEx,
        CacheInfo,
        sizeof(SYSTEM_FILECACHE_INFORMATION),
        0
        );
}

// rev from SetSystemFileCacheSize (MSDN) (dmex)
NTSTATUS PhSetSystemFileCacheSize(
    _In_ SIZE_T MinimumFileCacheSize,
    _In_ SIZE_T MaximumFileCacheSize,
    _In_ ULONG Flags
    )
{
    NTSTATUS status;
    SYSTEM_FILECACHE_INFORMATION cacheInfo;

    memset(&cacheInfo, 0, sizeof(SYSTEM_FILECACHE_INFORMATION));
    cacheInfo.MinimumWorkingSet = MinimumFileCacheSize;
    cacheInfo.MaximumWorkingSet = MaximumFileCacheSize;
    cacheInfo.Flags = Flags;

    status = NtSetSystemInformation(
        SystemFileCacheInformationEx,
        &cacheInfo,
        sizeof(SYSTEM_FILECACHE_INFORMATION)
        );

    return status;
}

NTSTATUS PhCreateEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ EVENT_TYPE EventType,
    _In_ BOOLEAN InitialState
    )
{
    NTSTATUS status;
    HANDLE eventHandle;
    OBJECT_ATTRIBUTES objectAttributes;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    status = NtCreateEvent(
        &eventHandle,
        DesiredAccess,
        &objectAttributes,
        EventType,
        InitialState
        );

    if (NT_SUCCESS(status))
    {
        *EventHandle = eventHandle;
    }

    return status;
}

// rev from DeviceIoControl (dmex)
NTSTATUS PhDeviceIoControlFile(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    if (DEVICE_TYPE_FROM_CTL_CODE(IoControlCode) == FILE_DEVICE_FILE_SYSTEM)
    {
        status = NtFsControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &ioStatusBlock,
            IoControlCode,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            OutputBufferLength
            );
    }
    else
    {
        status = NtDeviceIoControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &ioStatusBlock,
            IoControlCode,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            OutputBufferLength
            );
    }

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(DeviceHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
        {
            status = ioStatusBlock.Status;
        }
    }

    if (ReturnLength)
    {
        *ReturnLength = (ULONG)ioStatusBlock.Information;
    }

    return status;
}

// rev from RtlpWow64SelectSystem32PathInternal (dmex)
NTSTATUS PhWow64SelectSystem32Path(
    _In_ USHORT Machine,
    _In_ BOOLEAN IncludePathSeperator,
    _Out_ PPH_STRINGREF SystemPath
    )
{
    PWSTR WithSeperators;
    PWSTR WithoutSeperators;

    if (Machine != IMAGE_FILE_MACHINE_TARGET_HOST)
    {
        switch (Machine)
        {
        case IMAGE_FILE_MACHINE_I386:
            WithoutSeperators = L"SysWOW64";
            WithSeperators = L"\\SysWOW64\\";
            goto CreateResult;
        case IMAGE_FILE_MACHINE_ARMNT:
            WithoutSeperators = L"SysArm32";
            WithSeperators = L"\\SysArm32\\";
            goto CreateResult;
        case IMAGE_FILE_MACHINE_CHPE_X86:
            WithoutSeperators = L"SyChpe32";
            WithSeperators = L"\\SyChpe32\\";
            goto CreateResult;
        }

        if (Machine != IMAGE_FILE_MACHINE_AMD64 && Machine != IMAGE_FILE_MACHINE_ARM64)
            return STATUS_INVALID_PARAMETER;
    }

    WithSeperators = L"\\System32\\";
    WithoutSeperators = L"System32";

CreateResult:
    if (!IncludePathSeperator)
        WithSeperators = WithoutSeperators;

    PhInitializeStringRefLongHint(SystemPath, WithSeperators); // RtlInitUnicodeString
    return STATUS_SUCCESS;
}

/**
 * Retrieves information about a range of pages within the virtual address space of a specified process.
 *
 * \param ProcessHandle A handle to a process.
 * \param Callback A callback function which is executed for each memory region.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_MEMORY_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID baseAddress;
    MEMORY_BASIC_INFORMATION basicInfo;

    baseAddress = (PVOID)0;

    while (TRUE)
    {
        status = NtQueryVirtualMemory(
            ProcessHandle,
            baseAddress,
            MemoryBasicInformation,
            &basicInfo,
            sizeof(MEMORY_BASIC_INFORMATION),
            NULL
            );

        if (!NT_SUCCESS(status))
            break;

        if (basicInfo.State & MEM_FREE)
        {
            basicInfo.AllocationBase = basicInfo.BaseAddress;
            basicInfo.AllocationProtect = basicInfo.Protect;
        }

        status = Callback(ProcessHandle, &basicInfo, Context);

        if (!NT_SUCCESS(status))
            break;

        baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);

        if ((ULONG_PTR)baseAddress >= PhSystemBasicInformation.MaximumUserModeAddress)
            break;
    }

    return status;
}

/**
 * Retrieves information about a range of pages within the virtual address space of a specified process in batches for improved performance.
 *
 * \param ProcessHandle A handle to a process.
 * \param BaseAddress The base address at which to begin retrieving information.
 * \param BulkQuery A boolean indicating the mode of bulk query (accuracy vs reliability).
 * \param Callback A callback function which is executed for each memory region.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryBulk(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ BOOLEAN BulkQuery,
    _In_ PPH_ENUM_MEMORY_BULK_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;

    if (!NtPssCaptureVaSpaceBulk_Import())
    {
        return STATUS_NOT_SUPPORTED;
    }

    // BulkQuery... TRUE:
    // * Faster.
    // * More accurate snapshots.
    // * Copies the entire VA space into local memory.
    // * Wastes large amounts of heap memory due to buffer doubling.
    // * Unsuitable for low-memory situations and fails with insufficient system resources.
    // * ...
    //
    // BulkQuery... FALSE:
    // * Slightly slower.
    // * Slightly less accurate snapshots.
    // * Does not copy the VA space.
    // * Does not waste heap memory.
    // * Suitable for low-memory situations and doesn't fail with insufficient system resources.
    // * ...

    if (BulkQuery)
    {
        SIZE_T bufferLength;
        PNTPSS_MEMORY_BULK_INFORMATION buffer;
        PMEMORY_BASIC_INFORMATION information;

        bufferLength = sizeof(NTPSS_MEMORY_BULK_INFORMATION) + sizeof(MEMORY_BASIC_INFORMATION[20]);
        buffer = PhAllocate(bufferLength);
        buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;

        // Allocate a large buffer and copy all entries.

        while ((status = NtPssCaptureVaSpaceBulk(
            ProcessHandle,
            BaseAddress,
            buffer,
            bufferLength,
            NULL
            )) == STATUS_MORE_ENTRIES)
        {
            PhFree(buffer);
            bufferLength *= 2;

            if (bufferLength > PH_LARGE_BUFFER_SIZE)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferLength);
            buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;
        }

        if (NT_SUCCESS(status))
        {
            // Skip the enumeration header.

            information = PTR_ADD_OFFSET(buffer, RTL_SIZEOF_THROUGH_FIELD(NTPSS_MEMORY_BULK_INFORMATION, NextValidAddress));

            // Execute the callback.

            Callback(ProcessHandle, information, buffer->NumberOfEntries, Context);
        }

        PhFree(buffer);
    }
    else
    {
        UCHAR stackBuffer[sizeof(NTPSS_MEMORY_BULK_INFORMATION) + sizeof(MEMORY_BASIC_INFORMATION[20])];
        SIZE_T bufferLength;
        PNTPSS_MEMORY_BULK_INFORMATION buffer;
        PMEMORY_BASIC_INFORMATION information;

        bufferLength = sizeof(stackBuffer);
        buffer = (PNTPSS_MEMORY_BULK_INFORMATION)stackBuffer;
        buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;
        buffer->NextValidAddress = BaseAddress;

        while (TRUE)
        {
            // Get a batch of entries.

            status = NtPssCaptureVaSpaceBulk_Import()(
                ProcessHandle,
                buffer->NextValidAddress,
                buffer,
                bufferLength,
                NULL
                );

            if (!NT_SUCCESS(status))
                break;

            // Skip the enumeration header.

            information = PTR_ADD_OFFSET(buffer, RTL_SIZEOF_THROUGH_FIELD(NTPSS_MEMORY_BULK_INFORMATION, NextValidAddress));

            // Execute the callback.

            if (!NT_SUCCESS(Callback(ProcessHandle, information, buffer->NumberOfEntries, Context)))
                break;

            // Get the next batch.

            if (status != STATUS_MORE_ENTRIES)
                break;
        }
    }

    return status;
}

/**
 * Retrieves information about the pages currently added to the working set of the specified process.
 *
 * \param ProcessHandle A handle to a process.
 * \param Callback A callback function which is executed for each memory page.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryPages(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_MEMORY_PAGE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PMEMORY_WORKING_SET_INFORMATION pageInfo;

    status = PhGetProcessWorkingSetInformation(
        ProcessHandle,
        &pageInfo
        );

    if (NT_SUCCESS(status))
    {
        status = Callback(
            ProcessHandle,
            pageInfo->NumberOfEntries,
            pageInfo->WorkingSetInfo,
            Context
            );

        //for (ULONG_PTR i = 0; i < pageInfo->NumberOfEntries; i++)
        //{
        //    PMEMORY_WORKING_SET_BLOCK workingSetBlock = &pageInfo->WorkingSetInfo[i];
        //    PVOID virtualAddress = (PVOID)(workingSetBlock->VirtualPage << PAGE_SHIFT);
        //}

        PhFree(pageInfo);
    }

    return status;
}

/**
 * Retrieves extended information about the pages currently added to the working set at specific virtual addresses in the address space of the specified process.
 *
 * \param ProcessHandle A handle to a process.
 * \param BaseAddress The base address at which to begin retrieving information.
 * \param Size The total number of pages to query from the base address.
 * \param Callback A callback function which is executed for each memory page.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryAttributes(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size,
    _In_ PPH_ENUM_MEMORY_ATTRIBUTE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SIZE_T numberOfPages;
    ULONG_PTR virtualAddress;
    PMEMORY_WORKING_SET_EX_INFORMATION info;
    SIZE_T i;

    numberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, Size);
    virtualAddress = (ULONG_PTR)PAGE_ALIGN(BaseAddress);

    if (!numberOfPages)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    info = PhAllocatePage(numberOfPages * sizeof(MEMORY_WORKING_SET_EX_INFORMATION), NULL);

    if (!info)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    for (i = 0; i < numberOfPages; i++)
    {
        info[i].VirtualAddress = (PVOID)virtualAddress;
        virtualAddress += PAGE_SIZE;
    }

    status = NtQueryVirtualMemory(
        ProcessHandle,
        NULL,
        MemoryWorkingSetExInformation,
        info,
        numberOfPages * sizeof(MEMORY_WORKING_SET_EX_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        status = Callback(
            ProcessHandle,
            BaseAddress,
            Size,
            numberOfPages,
            info,
            Context
            );
    }

    PhFreePage(info);

CleanupExit:
    return status;
}

NTSTATUS PhGetKernelDebuggerInformation(
    _Out_opt_ PBOOLEAN KernelDebuggerEnabled,
    _Out_opt_ PBOOLEAN KernelDebuggerPresent
    )
{
    NTSTATUS status;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION debugInfo;

    status = NtQuerySystemInformation(
        SystemKernelDebuggerInformation,
        &debugInfo,
        sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (KernelDebuggerEnabled)
            *KernelDebuggerEnabled = debugInfo.KernelDebuggerEnabled;
        if (KernelDebuggerPresent)
            *KernelDebuggerPresent = !debugInfo.KernelDebuggerNotPresent;
    }

    return status;
}

// rev from BasepIsDebugPortPresent (dmex)
BOOLEAN PhIsDebugPortPresent(
    VOID
    )
{
    BOOLEAN isBeingDebugged;

    if (NT_SUCCESS(PhGetProcessIsBeingDebugged(NtCurrentProcess(), &isBeingDebugged)))
    {
        if (isBeingDebugged)
        {
            return TRUE;
        }
    }

    return FALSE;
}

// rev from IsDebuggerPresent (dmex)
/**
 * Determines whether the calling process is being debugged by a user-mode debugger.
 *
 * \return TRUE if the current process is running in the context of a debugger, otherwise the return value is FALSE.
 */
BOOLEAN PhIsDebuggerPresent(
    VOID
    )
{
#ifdef PHNT_NATIVE_DEBUGGER
    return !!IsDebuggerPresent();
#else
    return NtCurrentPeb()->BeingDebugged;
#endif
}

/**
 * Queries information about the volume associated with a given file, directory, storage device, or volume.
 *
 * \param ProcessHandle A handle to a process.
 * \param FileHandle A handle to the volume.
 * \param FsInformationClass Type of information to be returned about the volume.
 * \param FsInformation A pointer to a caller-allocated buffer.
 * \param FsInformationLength Size in bytes of the buffer pointed to by FsInformation.
 * \param IoStatusBlock A pointer to an IO_STATUS_BLOCK structure that receives the final completion status and information about the query operation.
 */
NTSTATUS PhQueryVolumeInformationFile(
    _In_opt_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _In_ FS_INFORMATION_CLASS FsInformationClass,
    _Out_writes_bytes_(FsInformationLength) PVOID FsInformation,
    _In_ ULONG FsInformationLength,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status;

    if (ProcessHandle)
    {
        if (KsiLevel() >= KphLevelMed)
        {
            status = KphQueryVolumeInformationFile(
                ProcessHandle,
                FileHandle,
                FsInformationClass,
                FsInformation,
                FsInformationLength,
                IoStatusBlock
                );
        }
        else if (ProcessHandle == NtCurrentProcess())
        {
            status = NtQueryVolumeInformationFile(
                FileHandle,
                IoStatusBlock,
                FsInformation,
                FsInformationLength,
                FsInformationClass
                );
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        status = NtQueryVolumeInformationFile(
            FileHandle,
            IoStatusBlock,
            FsInformation,
            FsInformationLength,
            FsInformationClass
            );
    }

    return status;
}

// rev from GetFileType (dmex)
/**
 * Retrieves the type of the specified file handle.
 *
 * \param ProcessHandle A handle to the process.
 * \param FileHandle A handle to the file.
 * \param DeviceType The type of the specified file
 *
 * \return Successful or errant status.
 */
NTSTATUS PhGetDeviceType(
    _In_opt_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _Out_ DEVICE_TYPE* DeviceType
    )
{
    NTSTATUS status;
    FILE_FS_DEVICE_INFORMATION debugInfo;
    IO_STATUS_BLOCK isb;

    status = PhQueryVolumeInformationFile(
        ProcessHandle,
        FileHandle,
        FileFsDeviceInformation,
        &debugInfo,
        sizeof(FILE_FS_DEVICE_INFORMATION),
        &isb
        );

    if (NT_SUCCESS(status))
    {
        *DeviceType = debugInfo.DeviceType;
    }

    return status;
}

BOOLEAN PhIsAppExecutionAliasTarget(
    _In_ PPH_STRING FileName
    )
{
    PPH_STRING targetFileName = NULL;
    PREPARSE_DATA_BUFFER reparseBuffer;
    ULONG reparseLength;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;

    if (PhIsNullOrEmptyString(FileName))
        return FALSE;

    if (!NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(FileName),
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT
        )))
    {
        return FALSE;
    }

    reparseLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    reparseBuffer = PhAllocateZero(reparseLength);

    if (NT_SUCCESS(NtFsControlFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_GET_REPARSE_POINT,
        NULL,
        0,
        reparseBuffer,
        reparseLength
        )))
    {
        if (
            IsReparseTagMicrosoft(reparseBuffer->ReparseTag) &&
            reparseBuffer->ReparseTag == IO_REPARSE_TAG_APPEXECLINK
            )
        {
            PCWSTR string;

            string = (PCWSTR)reparseBuffer->AppExecLinkReparseBuffer.StringList;

            for (ULONG i = 0; i < reparseBuffer->AppExecLinkReparseBuffer.StringCount; i++)
            {
                if (i == 2 && PhDoesFileExistWin32(string))
                {
                    targetFileName = PhCreateString(string);
                    break;
                }

                string += PhCountStringZ(string) + 1;
            }
        }
    }

    PhFree(reparseBuffer);
    NtClose(fileHandle);

    if (targetFileName)
    {
        if (PhDoesFileExistWin32(targetFileName->Buffer))
        {
            PhDereferenceObject(targetFileName);
            return TRUE;
        }

        PhDereferenceObject(targetFileName);
    }

    return FALSE;
}

NTSTATUS PhEnumProcessEnclaves(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID LdrEnclaveList,
    _In_ PPH_ENUM_PROCESS_ENCLAVES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    LIST_ENTRY enclaveList;
    LDR_SOFTWARE_ENCLAVE enclave;

    status = NtReadVirtualMemory(
        ProcessHandle,
        LdrEnclaveList,
        &enclaveList,
        sizeof(LIST_ENTRY),
        NULL
        );
    if (!NT_SUCCESS(status))
        return status;

    for (PLIST_ENTRY link = enclaveList.Flink;
         link != LdrEnclaveList;
         link = enclave.Links.Flink)
    {
        PVOID enclaveAddress;

        enclaveAddress = CONTAINING_RECORD(link, LDR_SOFTWARE_ENCLAVE, Links);

        status = NtReadVirtualMemory(
            ProcessHandle,
            link,
            &enclave,
            sizeof(enclave),
            NULL
            );
        if (!NT_SUCCESS(status))
            return status;

        if (!Callback(ProcessHandle, enclaveAddress, &enclave, Context))
            break;
    }

    return status;
}

#ifdef _M_ARM64
// rev from ntdll!RtlEcContextToNativeContext (jxy-s)
VOID PhEcContextToNativeContext(
    _Out_ PCONTEXT Context,
    _In_ PARM64EC_NT_CONTEXT EcContext
    )
{
    Context->ContextFlags = 0;

    //#define CONTEXT_AMD64   0x00100000L

    //#define CONTEXT_CONTROL         (CONTEXT_AMD64 | 0x00000001L)
    if (BooleanFlagOn(EcContext->ContextFlags, 0x00100000L | 0x00000001L))
        SetFlag(Context->ContextFlags, CONTEXT_CONTROL);

    //#define CONTEXT_INTEGER         (CONTEXT_AMD64 | 0x00000002L)
    if (BooleanFlagOn(EcContext->ContextFlags, 0x00100000L | 0x00000002L))
        SetFlag(Context->ContextFlags, CONTEXT_INTEGER);

    //#define CONTEXT_FLOATING_POINT  (CONTEXT_AMD64 | 0x00000008L)
    if (BooleanFlagOn(EcContext->ContextFlags, 0x00100000L | 0x00000008L))
        SetFlag(Context->ContextFlags, CONTEXT_FLOATING_POINT);

    SetFlag(Context->ContextFlags,
            EcContext->ContextFlags & (
                CONTEXT_EXCEPTION_ACTIVE |
                CONTEXT_SERVICE_ACTIVE |
                CONTEXT_EXCEPTION_REQUEST |
                CONTEXT_EXCEPTION_REPORTING
                ));

    Context->Cpsr = (EcContext->AMD64_EFlags & 0x00000100);         // Overflow Flag
    Context->Cpsr |= ((EcContext->AMD64_EFlags & 0x00000800) << 4); // Direction Flag
    Context->Cpsr |= ((EcContext->AMD64_EFlags & 0xFFFFFFC0) << 7); // Other Flags
    Context->Cpsr |= ((EcContext->AMD64_EFlags & 0x00000001) << 5); // Carry Flag
    Context->Cpsr <<= 13;

    Context->X0 = EcContext->X0;
    Context->X2 = EcContext->X2;
    Context->X4 = EcContext->X4;
    Context->X6 = EcContext->X6;
    Context->X7 = EcContext->X7;
    Context->X8 = EcContext->X8;
    Context->X9 = EcContext->X9;
    Context->X10 = EcContext->X10;
    Context->X11 = EcContext->X11;
    Context->X12 = EcContext->X12;
    Context->X14 = 0;
    Context->X15 = EcContext->X15;

    Context->X16 = EcContext->X16_0;
    Context->X16 |= ((ULONG64)EcContext->X16_1 << 16);
    Context->X16 |= ((ULONG64)EcContext->X16_2 << 32);
    Context->X16 |= ((ULONG64)EcContext->X16_3 << 48);

    Context->X17 = EcContext->X17_0;
    Context->X17 |= ((ULONG64)EcContext->X17_1 << 16);
    Context->X17 |= ((ULONG64)EcContext->X17_2 << 32);
    Context->X17 |= ((ULONG64)EcContext->X17_3 << 48);

    Context->X19 = EcContext->X19;
    Context->X21 = EcContext->X21;
    Context->X23 = 0;
    Context->X25 = EcContext->X25;
    Context->X27 = EcContext->X27;

    Context->Fp = EcContext->Fp;
    Context->Lr = EcContext->Lr;
    Context->Sp = EcContext->Sp;
    Context->Pc = EcContext->Pc;

    Context->V[0] = EcContext->V[0];
    Context->V[1] = EcContext->V[1];
    Context->V[2] = EcContext->V[2];
    Context->V[3] = EcContext->V[3];
    Context->V[4] = EcContext->V[4];
    Context->V[5] = EcContext->V[5];
    Context->V[6] = EcContext->V[6];
    Context->V[7] = EcContext->V[7];
    Context->V[8] = EcContext->V[8];
    Context->V[9] = EcContext->V[9];
    Context->V[10] = EcContext->V[10];
    Context->V[11] = EcContext->V[11];
    Context->V[12] = EcContext->V[12];
    Context->V[13] = EcContext->V[13];
    Context->V[14] = EcContext->V[14];
    Context->V[15] = EcContext->V[15];
    RtlZeroMemory(&Context->V[16], sizeof(ARM64_NT_NEON128));

    Context->Fpcr = (EcContext->AMD64_MxCsr & 0x00000080) == 0;         // IM: Invalid Operation Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000200) == 0) << 1; // ZM: Divide-by-Zero Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000400) == 0) << 2; // OM: Overflow Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000800) == 0) << 3; // UM: Underflow Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00001000) == 0) << 4; // PM: Precision Mask
    Context->Fpcr |= (EcContext->AMD64_MxCsr & 0x00000040) << 5;        // DAZ: Denormals Are Zero Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000100) == 0) << 7; // DM: Denormal Operation Mask
    Context->Fpcr |= (EcContext->AMD64_MxCsr & 0x00002000);             // FZ: Flush to Zero Mask
    Context->Fpcr |= (EcContext->AMD64_MxCsr & 0x0000C000);             // RC: Rounding Control
    Context->Fpcr <<= 8;

    Context->Fpsr = EcContext->AMD64_MxCsr & 1;            // IE: Invalid Operation Flag
    Context->Fpsr |= (EcContext->AMD64_MxCsr & 2) << 6;    // DE: Denormal Flag
    Context->Fpsr |= (EcContext->AMD64_MxCsr >> 1) & 0x1E; // ZE | OE | UE | PE: Zero, Overflow, Underflow, Precision Flags

    RtlZeroMemory(Context->Bcr, sizeof(Context->Bcr));
    RtlZeroMemory(Context->Bvr, sizeof(Context->Bvr));
    RtlZeroMemory(Context->Wcr, sizeof(Context->Wcr));
    RtlZeroMemory(Context->Wvr, sizeof(Context->Wvr));
}

// rev from ntdll!RtlNativeContextToEcContext (jxy-s)
VOID PhNativeContextToEcContext(
    _When_(InitializeEc, _Out_) _When_(!InitializeEc, _Inout_) PARM64EC_NT_CONTEXT EcContext,
    _In_ PCONTEXT Context,
    _In_ BOOLEAN InitializeEc
    )
{
    if (InitializeEc)
    {
        EcContext->ContextFlags = 0;

        //#define CONTEXT_AMD64   0x00100000L

        //#define CONTEXT_CONTROL         (CONTEXT_AMD64 | 0x00000001L)
        if (BooleanFlagOn(Context->ContextFlags, CONTEXT_CONTROL))
            SetFlag(EcContext->ContextFlags, (0x00100000L | 0x00000001L));

        //#define CONTEXT_INTEGER         (CONTEXT_AMD64 | 0x00000002L)
        if (BooleanFlagOn(Context->ContextFlags, CONTEXT_INTEGER))
            SetFlag(EcContext->ContextFlags, (0x00100000L | 0x00000002L));

        //#define CONTEXT_FLOATING_POINT  (CONTEXT_AMD64 | 0x00000008L)
        if (BooleanFlagOn(Context->ContextFlags, CONTEXT_FLOATING_POINT))
            SetFlag(EcContext->ContextFlags, (0x00100000L | 0x00000008L));

        EcContext->AMD64_P1Home = 0;
        EcContext->AMD64_P2Home = 0;
        EcContext->AMD64_P3Home = 0;
        EcContext->AMD64_P4Home = 0;
        EcContext->AMD64_P5Home = 0;
        EcContext->AMD64_P6Home = 0;

        EcContext->AMD64_Dr0 = 0;
        EcContext->AMD64_Dr1 = 0;
        EcContext->AMD64_Dr3 = 0;
        EcContext->AMD64_Dr6 = 0;
        EcContext->AMD64_Dr7 = 0;

        EcContext->AMD64_MxCsr_copy = 0;

        EcContext->AMD64_SegCs = 0x0033;
        EcContext->AMD64_SegDs = 0x002B;
        EcContext->AMD64_SegEs = 0x002B;
        EcContext->AMD64_SegFs = 0x0053;
        EcContext->AMD64_SegGs = 0x002B;
        EcContext->AMD64_SegSs = 0x002B;

        EcContext->AMD64_ControlWord = 0;
        EcContext->AMD64_StatusWord = 0;
        EcContext->AMD64_TagWord = 0;
        EcContext->AMD64_Reserved1 = 0;
        EcContext->AMD64_ErrorOpcode = 0;
        EcContext->AMD64_ErrorOffset = 0;
        EcContext->AMD64_ErrorSelector = 0;
        EcContext->AMD64_Reserved2= 0;
        EcContext->AMD64_DataOffset = 0;
        EcContext->AMD64_DataSelector = 0;
        EcContext->AMD64_Reserved3 = 0;

        EcContext->AMD64_MxCsr = 0;
        EcContext->AMD64_MxCsr_Mask = 0;

        EcContext->AMD64_St0_Reserved1 = 0;
        EcContext->AMD64_St0_Reserved2 = 0;
        EcContext->AMD64_St1_Reserved1 = 0;
        EcContext->AMD64_St1_Reserved2 = 0;
        EcContext->AMD64_St2_Reserved1 = 0;
        EcContext->AMD64_St2_Reserved2 = 0;
        EcContext->AMD64_St3_Reserved1 = 0;
        EcContext->AMD64_St3_Reserved2 = 0;
        EcContext->AMD64_St4_Reserved1 = 0;
        EcContext->AMD64_St4_Reserved2 = 0;
        EcContext->AMD64_St5_Reserved1 = 0;
        EcContext->AMD64_St5_Reserved2 = 0;
        EcContext->AMD64_St6_Reserved1 = 0;
        EcContext->AMD64_St6_Reserved2 = 0;
        EcContext->AMD64_St7_Reserved1 = 0;
        EcContext->AMD64_St7_Reserved2 = 0;

        EcContext->AMD64_EFlags = 0x202; // IF(EI) | RESERVED_1(always set)
    }

    EcContext->ContextFlags &= 0x7ffffffff;
    SetFlag(EcContext->ContextFlags,
            Context->ContextFlags & (
                CONTEXT_EXCEPTION_ACTIVE |
                CONTEXT_SERVICE_ACTIVE |
                CONTEXT_EXCEPTION_REQUEST |
                CONTEXT_EXCEPTION_REPORTING
                ));

    EcContext->AMD64_MxCsr_copy &= 0xFFFF0000;
    EcContext->AMD64_MxCsr_copy |= (Context->Fpsr & 0x00000080) >> 6;         // IM: Invalid Operation Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x00400000) >> 8;         // ZM: Divide-by-Zero Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x01000000) >> 9;         // OM: Overflow Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x00080000) >> 7;         // UM: Underflow Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x00800000) >> 7;         // PM: Precision Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpsr & 0x0000001E) << 1;         // Other Flags
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000100) == 0) << 7;  // DAZ: Denormals Are Zeros
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00008000) == 0) << 8;  // DM: Denormal Operation Mask
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000200) == 0) << 9;  // FZ: Flush to Zero
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000400) == 0) << 10; // RC: Rounding Control
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000800) == 0) << 11; // RC: Rounding Control
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00001000) == 0) << 12; // RC: Rounding Control
    EcContext->AMD64_MxCsr_copy |= Context->Fpsr & 0x00000001;

    EcContext->AMD64_EFlags &= 0xFFFFF63E;
    EcContext->AMD64_EFlags |= ((Context->Cpsr & 0x200000) >> 13);         // Overflow Flag
    EcContext->AMD64_EFlags |= ((Context->Cpsr & 0x10000000) >> 17);       // Direction Flag
    EcContext->AMD64_EFlags |= (((Context->Cpsr >> 5) & 0x1000000) >> 20); // Carry Flag
    EcContext->AMD64_EFlags |= ((Context->Cpsr & 0xC0FFFFFF) >> 20);       // Other Flags

    EcContext->Pc = Context->Pc;

    EcContext->X8 = Context->X8;
    EcContext->X0 = Context->X0;
    EcContext->X1 = Context->X1;
    EcContext->X27 = Context->X27;
    EcContext->Sp = Context->Sp;
    EcContext->Fp = Context->Fp;
    EcContext->X25 = Context->X25;
    EcContext->X26 = Context->X26;
    EcContext->X2 = Context->X2;
    EcContext->X3 = Context->X3;
    EcContext->X4 = Context->X4;
    EcContext->X5 = Context->X5;
    EcContext->X19 = Context->X19;
    EcContext->X20 = Context->X20;
    EcContext->X21 = Context->X21;
    EcContext->X22 = Context->X22;

    EcContext->Lr = Context->Lr;
    EcContext->X16_0 = LOWORD(Context->X16);
    EcContext->X6 = Context->X6;
    EcContext->X16_1 = HIWORD(Context->X16);
    EcContext->X7 = Context->X7;
    EcContext->X16_2 = LOWORD(Context->X16 >> 32);
    EcContext->X9 = Context->X9;
    EcContext->X16_3 = HIWORD(Context->X16 >> 32);
    EcContext->X10 = Context->X10;
    EcContext->X17_0 = LOWORD(Context->X17);
    EcContext->X11 = Context->X11;
    EcContext->X17_1 = HIWORD(Context->X17);
    EcContext->X12 = Context->X12;
    EcContext->X17_2 = LOWORD(Context->X17 >> 32);
    EcContext->X15 = Context->X15;
    EcContext->X17_3 = HIWORD(Context->X17 >> 32);

    EcContext->V[0] = Context->V[0];
    EcContext->V[1] = Context->V[1];
    EcContext->V[2] = Context->V[2];
    EcContext->V[3] = Context->V[3];
    EcContext->V[4] = Context->V[4];
    EcContext->V[5] = Context->V[5];
    EcContext->V[6] = Context->V[6];
    EcContext->V[7] = Context->V[7];
    EcContext->V[8] = Context->V[8];
    EcContext->V[9] = Context->V[9];
    EcContext->V[10] = Context->V[10];
    EcContext->V[11] = Context->V[11];
    EcContext->V[12] = Context->V[12];
    EcContext->V[13] = Context->V[13];
    EcContext->V[14] = Context->V[14];
    EcContext->V[15] = Context->V[15];
}

// rev from ntdll!RtlIsEcCode (jxy-s)
NTSTATUS PhIsEcCode(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 CodePointer,
    _Out_ PBOOLEAN IsEcCode
    )
{
    NTSTATUS status;
    PVOID pebBaseAddress;
    PVOID ecCodeBitMap;
    ULONG64 bitmap;

    *IsEcCode = FALSE;

    // hack (jxy-s)
    // 0x00007ffffffeffff = LdrpEcBitmapData.HighestAddress (MmHighestUserAddress)
    // 0x0000000000010000 = MM_LOWEST_USER_ADDRESS
    if (CodePointer > 0x00007ffffffeffff || CodePointer < 0x0000000000010000)
        return STATUS_INVALID_PARAMETER_2;

    if (!NT_SUCCESS(status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress)))
        return status;

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(pebBaseAddress, FIELD_OFFSET(PEB, EcCodeBitMap)),
        &ecCodeBitMap,
        sizeof(PVOID),
        NULL
        )))
        return status;

    if (!ecCodeBitMap)
        return STATUS_INVALID_PARAMETER_1;

    // each byte of bitmap indexes 8*4K = 2^15 byte span
    ecCodeBitMap = PTR_ADD_OFFSET(ecCodeBitMap, CodePointer >> 15);

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        ProcessHandle,
        ecCodeBitMap,
        &bitmap,
        sizeof(ULONG64),
        NULL
        )))
        return status;

    // index to the 4k page within the 8*4K span
    bitmap >>= ((CodePointer >> PAGE_SHIFT) & 7);

    // test the specific page
    if (bitmap & 1)
        *IsEcCode = TRUE;

    return STATUS_SUCCESS;
}
#endif

NTSTATUS PhFlushProcessHeapsRemote(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInformation;
    PVOID rtlExitUserThread = NULL;
    PVOID rtlFlushHeaps = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        &isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        0,
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlFlushHeaps",
        0,
        &rtlFlushHeaps,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            rtlFlushHeaps,
            NULL,
            NULL,
            NULL
            );
    }
    else
    {
#endif
        status = NtQueueApcThread(
            threadHandle,
            rtlFlushHeaps,
            NULL,
            NULL,
            NULL
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL); // Execute the pending APC (dmex)

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWaitForSingleObject(threadHandle, FALSE, Timeout);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetThreadBasicInformation(threadHandle, &basicInformation);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = basicInformation.ExitStatus;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}
