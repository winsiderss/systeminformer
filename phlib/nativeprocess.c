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
#include <kphuser.h>

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
    _In_ PCLIENT_ID ClientId
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
            ClientId
            );
    }
    else
    {
        InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
        status = NtOpenProcess(
            ProcessHandle,
            DesiredAccess,
            &objectAttributes,
            ClientId
            );

        if (status == STATUS_ACCESS_DENIED && (level == KphLevelMax))
        {
            status = KphOpenProcess(
                ProcessHandle,
                DesiredAccess,
                ClientId
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

NTSTATUS PhGetProcessModifiedCookie(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG ProcessModifiedCookie
    )
{
    NTSTATUS status;
    ULONG processCookie;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessCookie,
        &processCookie,
        sizeof(ULONG),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ProcessModifiedCookie = (0x7FFFFFED * processCookie + 0x7FFFFFC3) % 0x7FFFFFFF;
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
    _In_ ULONG Offset,
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
    return PhGetProcessPebString(ProcessHandle, PhpoCurrentDirectory | (IsWow64 ? PhpoWow64 : PhpoNone), CurrentDirectory);
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
    status = PhGetProcessPebString(ProcessHandle, PhpoWindowTitle | (isWow64 ? PhpoWow64 : PhpoNone), WindowTitle);
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
 * \param IsWow64Process A variable which receives a boolean indicating whether the process is 32-bit.
 * \li \c PH_GET_PROCESS_ENVIRONMENT_WOW64 Retrieve the environment block from the WOW64 PEB.
 * \param Environment A variable which will receive a pointer to the environment block copied from
 * the process. You must free the block using PhFreePage() when you no longer need it.
 * \param EnvironmentLength A variable which will receive the length of the environment block, in
 * bytes.
 */
NTSTATUS PhGetProcessEnvironment(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN IsWow64Process,
    _Out_ PVOID *Environment,
    _Out_ PULONG EnvironmentLength
    )
{
    NTSTATUS status;
    PVOID environmentRemote;
    MEMORY_BASIC_INFORMATION mbi;
    PVOID environment;
    SIZE_T environmentLength;

    if (IsWow64Process)
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
    else
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

NTSTATUS PhGetProcessPriorityClass(
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

NTSTATUS PhSetProcessPriorityClass(
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
    // See also PhSetVirtualMemoryPagePriority (dmex)
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

NTSTATUS PhGetProcessActivityModerationState(
    _In_ PCPH_STRINGREF ModerationIdentifier,
    _Out_ PSYSTEM_ACTIVITY_MODERATION_APP_SETTINGS ModerationSettings
    )
{
    NTSTATUS status;
    SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS activityModeration;
    PKEY_VALUE_PARTIAL_INFORMATION keyValueInfo;

    RtlZeroMemory(&activityModeration, sizeof(SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS));

    status = NtQuerySystemInformation(
        SystemActivityModerationUserSettings,
        &activityModeration,
        sizeof(SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS),
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhQueryValueKey(
        activityModeration.UserKeyHandle,
        ModerationIdentifier,
        KeyValuePartialInformation,
        &keyValueInfo
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (
        keyValueInfo->Type != REG_BINARY ||
        keyValueInfo->DataLength != sizeof(SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS)
        )
    {
        status = STATUS_INVALID_KERNEL_INFO_VERSION;
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    RtlMoveMemory(
        ModerationSettings,
        keyValueInfo->Data,
        sizeof(activityModeration)
        );

CleanupExit:
    if (activityModeration.UserKeyHandle)
    {
        NtClose(activityModeration.UserKeyHandle);
    }

    return status;
}

NTSTATUS PhSetProcessActivityModerationState(
    _In_ PCPH_STRINGREF ModerationIdentifier,
    _In_ SYSTEM_ACTIVITY_MODERATION_APP_TYPE ModerationType,
    _In_ SYSTEM_ACTIVITY_MODERATION_STATE ModerationState
    )
{
    NTSTATUS status;
    SYSTEM_ACTIVITY_MODERATION_INFO moderationInfo;

    memset(&moderationInfo, 0, sizeof(SYSTEM_ACTIVITY_MODERATION_INFO));
    moderationInfo.AppType = ModerationType;
    moderationInfo.ModerationState = ModerationState;
    PhStringRefToUnicodeString(ModerationIdentifier, &moderationInfo.Identifier);

    status = NtSetSystemInformation(
        SystemActivityModerationExeState,
        &moderationInfo,
        sizeof(SYSTEM_ACTIVITY_MODERATION_INFO)
        );

    if (NT_SUCCESS(status))
    {
        SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS activityModeration;

        RtlZeroMemory(&activityModeration, sizeof(SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS));

        status = NtQuerySystemInformation(
            SystemActivityModerationUserSettings,
            &activityModeration,
            sizeof(SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS moderationSettings;

            RtlZeroMemory(&moderationSettings, sizeof(SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS));
            PhQuerySystemTime(&moderationSettings.LastUpdatedTime);
            moderationSettings.AppType = ModerationType;
            moderationSettings.ModerationState = ModerationState;

            status = PhSetValueKey(
                activityModeration.UserKeyHandle,
                ModerationIdentifier,
                REG_BINARY,
                &moderationSettings,
                sizeof(SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS)
                );
        }
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
 * Query the power throttling state for a specified process.
 *
 * \param ProcessHandle The handle to the target process.
 * \param PowerThrottlingState The control and state masks indicating the current power throttling of the process.
 * \return The NTSTATUS code indicating the success or failure of the operation.
 */
NTSTATUS PhGetProcessPowerThrottlingState(
    _In_ HANDLE ProcessHandle,
    _Out_ PPOWER_THROTTLING_PROCESS_STATE PowerThrottlingState
    )
{
    NTSTATUS status;

    memset(PowerThrottlingState, 0, sizeof(POWER_THROTTLING_PROCESS_STATE));
    PowerThrottlingState->Version = POWER_THROTTLING_PROCESS_CURRENT_VERSION;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessPowerThrottlingState,
        PowerThrottlingState,
        sizeof(POWER_THROTTLING_PROCESS_STATE),
        NULL
        );

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
        ULONGLONG sequenceNumber = 0;

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessSequenceNumber,
            &sequenceNumber,
            sizeof(ULONGLONG),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            *SequenceNumber = sequenceNumber;
        }
        else if (status == STATUS_INVALID_INFO_CLASS && WindowsVersion >= WINDOWS_10)
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

            if (
                status == STATUS_BUFFER_OVERFLOW &&
                RTL_CONTAINS_FIELD(&telemetryInfo, telemetryInfo.HeaderSize, ProcessSequenceNumber)
                )
            {
                status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(status))
            {
                *SequenceNumber = telemetryInfo.ProcessSequenceNumber;
            }
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
        telemetryLength = returnLength;
        telemetryBuffer = PhReAllocate(telemetryBuffer, telemetryLength);

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
