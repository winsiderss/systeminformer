/*
 * Process Hacker -
 *   application support functions
 *
 * Copyright (C) 2010-2016 wj32
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

#include <phapp.h>
#include <settings.h>
#include <symprv.h>
#include <cpysave.h>
#include <phappres.h>
#include <emenu.h>
#include <phsvccl.h>
#include "mxml/mxml.h"
#include "pcre/pcre2.h"
#include <winsta.h>

#pragma warning(push)
#pragma warning(disable: 4091) // Ignore 'no variable declared on typedef'
#include <dbghelp.h>
#pragma warning(pop)

#include <appmodel.h>

typedef LONG (WINAPI *_GetPackageFullName)(
    _In_ HANDLE hProcess,
    _Inout_ UINT32 *packageFullNameLength,
    _Out_opt_ PWSTR packageFullName
    );

typedef LONG (WINAPI *_GetPackagePath)(
    _In_ PACKAGE_ID *packageId,
    _Reserved_ UINT32 reserved,
    _Inout_ UINT32 *pathLength,
    _Out_opt_ PWSTR path
    );

typedef LONG (WINAPI *_PackageIdFromFullName)(
    _In_ PCWSTR packageFullName,
    _In_ UINT32 flags,
    _Inout_ UINT32 *bufferLength,
    _Out_opt_ BYTE *buffer
    );

GUID XP_CONTEXT_GUID = { 0xbeb1b341, 0x6837, 0x4c83, { 0x83, 0x66, 0x2b, 0x45, 0x1e, 0x7c, 0xe6, 0x9b } };
GUID VISTA_CONTEXT_GUID = { 0xe2011457, 0x1546, 0x43c5, { 0xa5, 0xfe, 0x00, 0x8d, 0xee, 0xe3, 0xd3, 0xf0 } };
GUID WIN7_CONTEXT_GUID = { 0x35138b9a, 0x5d96, 0x4fbd, { 0x8e, 0x2d, 0xa2, 0x44, 0x02, 0x25, 0xf9, 0x3a } };
GUID WIN8_CONTEXT_GUID = { 0x4a2f28e3, 0x53b9, 0x4441, { 0xba, 0x9c, 0xd6, 0x9d, 0x4a, 0x4a, 0x6e, 0x38 } };
GUID WINBLUE_CONTEXT_GUID = { 0x1f676c76, 0x80e1, 0x4239, { 0x95, 0xbb, 0x83, 0xd0, 0xf6, 0xd0, 0xda, 0x78 } };
GUID WINTHRESHOLD_CONTEXT_GUID = { 0x8e0f7a12, 0xbfb3, 0x4fe8, { 0xb9, 0xa5, 0x48, 0xfd, 0x50, 0xa1, 0x5a, 0x9a } };

/**
 * Determines whether a process is suspended.
 *
 * \param Process The SYSTEM_PROCESS_INFORMATION structure
 * of the process.
 */
BOOLEAN PhGetProcessIsSuspended(
    _In_ PSYSTEM_PROCESS_INFORMATION Process
    )
{
    ULONG i;

    for (i = 0; i < Process->NumberOfThreads; i++)
    {
        if (
            Process->Threads[i].ThreadState != Waiting ||
            Process->Threads[i].WaitReason != Suspended
            )
            return FALSE;
    }

    return Process->NumberOfThreads != 0;
}

/**
 * Determines the OS compatibility context of a process.
 *
 * \param ProcessHandle A handle to a process.
 * \param Guid A variable which receives a GUID identifying an
 * operating system version.
 */
NTSTATUS PhGetProcessSwitchContext(
    _In_ HANDLE ProcessHandle,
    _Out_ PGUID Guid
    )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION basicInfo;
#ifdef _WIN64
    PVOID peb32;
    ULONG data32;
#endif
    PVOID data;

    // Reverse-engineered from WdcGetProcessSwitchContext (wdc.dll).
    // On Windows 8, the function is now SdbGetAppCompatData (apphelp.dll).
    // On Windows 10, the function is again WdcGetProcessSwitchContext.

#ifdef _WIN64
    if (NT_SUCCESS(PhGetProcessPeb32(ProcessHandle, &peb32)) && peb32)
    {
        if (WindowsVersion >= WINDOWS_8)
        {
            if (!NT_SUCCESS(status = PhReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(peb32, FIELD_OFFSET(PEB32, pShimData)),
                &data32,
                sizeof(ULONG),
                NULL
                )))
                return status;
        }
        else
        {
            if (!NT_SUCCESS(status = PhReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(peb32, FIELD_OFFSET(PEB32, pContextData)),
                &data32,
                sizeof(ULONG),
                NULL
                )))
                return status;
        }

        data = UlongToPtr(data32);
    }
    else
    {
#endif
        if (!NT_SUCCESS(status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo)))
            return status;

        if (WindowsVersion >= WINDOWS_8)
        {
            if (!NT_SUCCESS(status = PhReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, pShimData)),
                &data,
                sizeof(PVOID),
                NULL
                )))
                return status;
        }
        else
        {
            if (!NT_SUCCESS(status = PhReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, pContextData)),
                &data,
                sizeof(PVOID),
                NULL
                )))
                return status;
        }
#ifdef _WIN64
    }
#endif

    if (!data)
        return STATUS_UNSUCCESSFUL; // no compatibility context data

    if (WindowsVersion >= WINDOWS_10)
    {
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(data, 2040 + 24), // Magic value from SbReadProcContextByHandle
            Guid,
            sizeof(GUID),
            NULL
            )))
            return status;
    }
    else if (WindowsVersion >= WINDOWS_8_1)
    {
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(data, 2040 + 16), // Magic value from SbReadProcContextByHandle
            Guid,
            sizeof(GUID),
            NULL
            )))
            return status;
    }
    else if (WindowsVersion >= WINDOWS_8)
    {
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(data, 2040), // Magic value from SbReadProcContextByHandle
            Guid,
            sizeof(GUID),
            NULL
            )))
            return status;
    }
    else
    {
        if (!NT_SUCCESS(status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(data, 32), // Magic value from WdcGetProcessSwitchContext
            Guid,
            sizeof(GUID),
            NULL
            )))
            return status;
    }

    return STATUS_SUCCESS;
}

PPH_STRING PhGetProcessPackageFullName(
    _In_ HANDLE ProcessHandle
    )
{
    static _GetPackageFullName getPackageFullName = NULL;

    LONG result;
    PPH_STRING name;
    ULONG nameLength;

    if (!getPackageFullName)
        getPackageFullName = PhGetModuleProcAddress(L"kernel32.dll", "GetPackageFullName");
    if (!getPackageFullName)
        return NULL;

    nameLength = 101;
    name = PhCreateStringEx(NULL, (nameLength - 1) * 2);

    result = getPackageFullName(ProcessHandle, &nameLength, name->Buffer);

    if (result == ERROR_INSUFFICIENT_BUFFER)
    {
        PhDereferenceObject(name);
        name = PhCreateStringEx(NULL, (nameLength - 1) * 2);

        result = getPackageFullName(ProcessHandle, &nameLength, name->Buffer);
    }

    if (result == ERROR_SUCCESS)
    {
        PhTrimToNullTerminatorString(name);
        return name;
    }
    else
    {
        PhDereferenceObject(name);
        return NULL;
    }
}

PACKAGE_ID *PhPackageIdFromFullName(
    _In_ PWSTR PackageFullName
    )
{
    static _PackageIdFromFullName packageIdFromFullName = NULL;

    LONG result;
    PVOID packageIdBuffer;
    ULONG packageIdBufferSize;

    if (!packageIdFromFullName)
        packageIdFromFullName = PhGetModuleProcAddress(L"kernel32.dll", "PackageIdFromFullName");
    if (!packageIdFromFullName)
        return NULL;

    packageIdBufferSize = 100;
    packageIdBuffer = PhAllocate(packageIdBufferSize);

    result = packageIdFromFullName(PackageFullName, PACKAGE_INFORMATION_BASIC, &packageIdBufferSize, (PBYTE)packageIdBuffer);

    if (result == ERROR_INSUFFICIENT_BUFFER)
    {
        PhFree(packageIdBuffer);
        packageIdBuffer = PhAllocate(packageIdBufferSize);

        result = packageIdFromFullName(PackageFullName, PACKAGE_INFORMATION_BASIC, &packageIdBufferSize, (PBYTE)packageIdBuffer);
    }

    if (result == ERROR_SUCCESS)
    {
        return packageIdBuffer;
    }
    else
    {
        PhFree(packageIdBuffer);
        return NULL;
    }
}

PPH_STRING PhGetPackagePath(
    _In_ PACKAGE_ID *PackageId
    )
{
    static _GetPackagePath getPackagePath = NULL;

    LONG result;
    PPH_STRING path;
    ULONG pathLength;

    if (!getPackagePath)
        getPackagePath = PhGetModuleProcAddress(L"kernel32.dll", "GetPackagePath");
    if (!getPackagePath)
        return NULL;

    pathLength = 101;
    path = PhCreateStringEx(NULL, (pathLength - 1) * 2);

    result = getPackagePath(PackageId, 0, &pathLength, path->Buffer);

    if (result == ERROR_INSUFFICIENT_BUFFER)
    {
        PhDereferenceObject(path);
        path = PhCreateStringEx(NULL, (pathLength - 1) * 2);

        result = getPackagePath(PackageId, 0, &pathLength, path->Buffer);
    }

    if (result == ERROR_SUCCESS)
    {
        PhTrimToNullTerminatorString(path);
        return path;
    }
    else
    {
        PhDereferenceObject(path);
        return NULL;
    }
}

/**
 * Determines the type of a process based on its image file name.
 *
 * \param ProcessHandle A handle to a process.
 * \param KnownProcessType A variable which receives the process
 * type.
 */
NTSTATUS PhGetProcessKnownType(
    _In_ HANDLE ProcessHandle,
    _Out_ PH_KNOWN_PROCESS_TYPE *KnownProcessType
    )
{
    NTSTATUS status;
    PH_KNOWN_PROCESS_TYPE knownProcessType;
    PROCESS_BASIC_INFORMATION basicInfo;
    PH_STRINGREF systemRootPrefix;
    PPH_STRING fileName;
    PPH_STRING newFileName;
    PH_STRINGREF name;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    if (!NT_SUCCESS(status = PhGetProcessBasicInformation(
        ProcessHandle,
        &basicInfo
        )))
        return status;

    if (basicInfo.UniqueProcessId == SYSTEM_PROCESS_ID)
    {
        *KnownProcessType = SystemProcessType;
        return STATUS_SUCCESS;
    }

    PhGetSystemRoot(&systemRootPrefix);

    if (!NT_SUCCESS(status = PhGetProcessImageFileName(
        ProcessHandle,
        &fileName
        )))
    {
        return status;
    }

    newFileName = PhGetFileName(fileName);
    PhDereferenceObject(fileName);
    name = newFileName->sr;

    knownProcessType = UnknownProcessType;

    if (PhStartsWithStringRef(&name, &systemRootPrefix, TRUE))
    {
        // Skip the system root, and we now have three cases:
        // 1. \\xyz.exe - Windows executable.
        // 2. \\System32\\xyz.exe - system32 executable.
        // 3. \\SysWow64\\xyz.exe - system32 executable + WOW64.
        PhSkipStringRef(&name, systemRootPrefix.Length);

        if (PhEqualStringRef2(&name, L"\\explorer.exe", TRUE))
        {
            knownProcessType = ExplorerProcessType;
        }
        else if (
            PhStartsWithStringRef2(&name, L"\\System32", TRUE)
#ifdef _WIN64
            || (PhStartsWithStringRef2(&name, L"\\SysWow64", TRUE) && (isWow64 = TRUE, TRUE)) // ugly but necessary
#endif
            )
        {
            // SysTem32 and SysWow64 are both 8 characters long.
            PhSkipStringRef(&name, 9 * sizeof(WCHAR));

            if (FALSE)
                ; // Dummy
            else if (PhEqualStringRef2(&name, L"\\smss.exe", TRUE))
                knownProcessType = SessionManagerProcessType;
            else if (PhEqualStringRef2(&name, L"\\csrss.exe", TRUE))
                knownProcessType = WindowsSubsystemProcessType;
            else if (PhEqualStringRef2(&name, L"\\wininit.exe", TRUE))
                knownProcessType = WindowsStartupProcessType;
            else if (PhEqualStringRef2(&name, L"\\services.exe", TRUE))
                knownProcessType = ServiceControlManagerProcessType;
            else if (PhEqualStringRef2(&name, L"\\lsass.exe", TRUE))
                knownProcessType = LocalSecurityAuthorityProcessType;
            else if (PhEqualStringRef2(&name, L"\\lsm.exe", TRUE))
                knownProcessType = LocalSessionManagerProcessType;
            else if (PhEqualStringRef2(&name, L"\\winlogon.exe", TRUE))
                knownProcessType = WindowsLogonProcessType;
            else if (PhEqualStringRef2(&name, L"\\svchost.exe", TRUE))
                knownProcessType = ServiceHostProcessType;
            else if (PhEqualStringRef2(&name, L"\\rundll32.exe", TRUE))
                knownProcessType = RunDllAsAppProcessType;
            else if (PhEqualStringRef2(&name, L"\\dllhost.exe", TRUE))
                knownProcessType = ComSurrogateProcessType;
            else if (PhEqualStringRef2(&name, L"\\taskeng.exe", TRUE))
                knownProcessType = TaskHostProcessType;
            else if (PhEqualStringRef2(&name, L"\\taskhost.exe", TRUE))
                knownProcessType = TaskHostProcessType;
            else if (PhEqualStringRef2(&name, L"\\taskhostex.exe", TRUE))
                knownProcessType = TaskHostProcessType;
            else if (PhEqualStringRef2(&name, L"\\taskhostw.exe", TRUE))
                knownProcessType = TaskHostProcessType;
            else if (PhEqualStringRef2(&name, L"\\wudfhost.exe", TRUE))
                knownProcessType = UmdfHostProcessType;
        }
    }

    PhDereferenceObject(newFileName);

#ifdef _WIN64
    if (isWow64)
        knownProcessType |= KnownProcessWow64;
#endif

    *KnownProcessType = knownProcessType;

    return status;
}

static BOOLEAN NTAPI PhpSvchostCommandLineCallback(
    _In_opt_ PPH_COMMAND_LINE_OPTION Option,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PVOID Context
    )
{
    PPH_KNOWN_PROCESS_COMMAND_LINE knownCommandLine = Context;

    if (Option && Option->Id == 1)
    {
        PhSwapReference(&knownCommandLine->ServiceHost.GroupName, Value);
    }

    return TRUE;
}

BOOLEAN PhaGetProcessKnownCommandLine(
    _In_ PPH_STRING CommandLine,
    _In_ PH_KNOWN_PROCESS_TYPE KnownProcessType,
    _Out_ PPH_KNOWN_PROCESS_COMMAND_LINE KnownCommandLine
    )
{
    switch (KnownProcessType & KnownProcessTypeMask)
    {
    case ServiceHostProcessType:
        {
            // svchost.exe -k <GroupName>

            static PH_COMMAND_LINE_OPTION options[] =
            {
                { 1, L"k", MandatoryArgumentType }
            };

            KnownCommandLine->ServiceHost.GroupName = NULL;

            PhParseCommandLine(
                &CommandLine->sr,
                options,
                sizeof(options) / sizeof(PH_COMMAND_LINE_OPTION),
                PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS,
                PhpSvchostCommandLineCallback,
                KnownCommandLine
                );

            if (KnownCommandLine->ServiceHost.GroupName)
            {
                PhAutoDereferenceObject(KnownCommandLine->ServiceHost.GroupName);
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        break;
    case RunDllAsAppProcessType:
        {
            // rundll32.exe <DllName>,<ProcedureName> ...

            SIZE_T i;
            PH_STRINGREF dllNamePart;
            PH_STRINGREF procedureNamePart;
            PPH_STRING dllName;
            PPH_STRING procedureName;

            i = 0;

            // Get the rundll32.exe part.

            dllName = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!dllName)
                return FALSE;

            PhDereferenceObject(dllName);

            // Get the DLL name part.

            while (i < CommandLine->Length / 2 && CommandLine->Buffer[i] == ' ')
                i++;

            dllName = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!dllName)
                return FALSE;

            PhAutoDereferenceObject(dllName);

            // The procedure name begins after the last comma.

            if (!PhSplitStringRefAtLastChar(&dllName->sr, ',', &dllNamePart, &procedureNamePart))
                return FALSE;

            dllName = PhAutoDereferenceObject(PhCreateString2(&dllNamePart));
            procedureName = PhAutoDereferenceObject(PhCreateString2(&procedureNamePart));

            // If the DLL name isn't an absolute path, assume it's in system32.
            // TODO: Use a proper search function.

            if (RtlDetermineDosPathNameType_U(dllName->Buffer) == RtlPathTypeRelative)
            {
                dllName = PhaConcatStrings(
                    3,
                    ((PPH_STRING)PhAutoDereferenceObject(PhGetSystemDirectory()))->Buffer,
                    L"\\",
                    dllName->Buffer
                    );
            }

            KnownCommandLine->RunDllAsApp.FileName = dllName;
            KnownCommandLine->RunDllAsApp.ProcedureName = procedureName;
        }
        break;
    case ComSurrogateProcessType:
        {
            // dllhost.exe /processid:<Guid>

            static PH_STRINGREF inprocServer32Name = PH_STRINGREF_INIT(L"InprocServer32");

            SIZE_T i;
            ULONG_PTR indexOfProcessId;
            PPH_STRING argPart;
            PPH_STRING guidString;
            UNICODE_STRING guidStringUs;
            GUID guid;
            HANDLE clsidKeyHandle;
            HANDLE inprocServer32KeyHandle;
            PPH_STRING fileName;

            i = 0;

            // Get the dllhost.exe part.

            argPart = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!argPart)
                return FALSE;

            PhDereferenceObject(argPart);

            // Get the argument part.

            while (i < (ULONG)CommandLine->Length / 2 && CommandLine->Buffer[i] == ' ')
                i++;

            argPart = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!argPart)
                return FALSE;

            PhAutoDereferenceObject(argPart);

            // Find "/processid:"; the GUID is just after that.

            PhUpperString(argPart);
            indexOfProcessId = PhFindStringInString(argPart, 0, L"/PROCESSID:");

            if (indexOfProcessId == -1)
                return FALSE;

            guidString = PhaSubstring(
                argPart,
                indexOfProcessId + 11,
                (ULONG)argPart->Length / 2 - indexOfProcessId - 11
                );
            PhStringRefToUnicodeString(&guidString->sr, &guidStringUs);

            if (!NT_SUCCESS(RtlGUIDFromString(
                &guidStringUs,
                &guid
                )))
                return FALSE;

            KnownCommandLine->ComSurrogate.Guid = guid;
            KnownCommandLine->ComSurrogate.Name = NULL;
            KnownCommandLine->ComSurrogate.FileName = NULL;

            // Lookup the GUID in the registry to determine the name and file name.

            if (NT_SUCCESS(PhOpenKey(
                &clsidKeyHandle,
                KEY_READ,
                PH_KEY_CLASSES_ROOT,
                &PhaConcatStrings2(L"CLSID\\", guidString->Buffer)->sr,
                0
                )))
            {
                KnownCommandLine->ComSurrogate.Name =
                    PhAutoDereferenceObject(PhQueryRegistryString(clsidKeyHandle, NULL));

                if (NT_SUCCESS(PhOpenKey(
                    &inprocServer32KeyHandle,
                    KEY_READ,
                    clsidKeyHandle,
                    &inprocServer32Name,
                    0
                    )))
                {
                    KnownCommandLine->ComSurrogate.FileName =
                        PhAutoDereferenceObject(PhQueryRegistryString(inprocServer32KeyHandle, NULL));

                    if (fileName = PhAutoDereferenceObject(PhExpandEnvironmentStrings(
                        &KnownCommandLine->ComSurrogate.FileName->sr
                        )))
                    {
                        KnownCommandLine->ComSurrogate.FileName = fileName;
                    }

                    NtClose(inprocServer32KeyHandle);
                }

                NtClose(clsidKeyHandle);
            }
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

VOID PhEnumChildWindows(
    _In_opt_ HWND hWnd,
    _In_ ULONG Limit,
    _In_ WNDENUMPROC Callback,
    _In_ LPARAM lParam
    )
{
    HWND childWindow = NULL;
    ULONG i = 0;

    while (i < Limit && (childWindow = FindWindowEx(hWnd, childWindow, NULL, NULL)))
    {
        if (!Callback(childWindow, lParam))
            return;

        i++;
    }
}

typedef struct _GET_PROCESS_MAIN_WINDOW_CONTEXT
{
    HWND Window;
    HWND ImmersiveWindow;
    HANDLE ProcessId;
    BOOLEAN IsImmersive;
} GET_PROCESS_MAIN_WINDOW_CONTEXT, *PGET_PROCESS_MAIN_WINDOW_CONTEXT;

BOOL CALLBACK PhpGetProcessMainWindowEnumWindowsProc(
    _In_ HWND hwnd,
    _In_ LPARAM lParam
    )
{
    PGET_PROCESS_MAIN_WINDOW_CONTEXT context = (PGET_PROCESS_MAIN_WINDOW_CONTEXT)lParam;
    ULONG processId;
    HWND parentWindow;
    WINDOWINFO windowInfo;

    if (!IsWindowVisible(hwnd))
        return TRUE;

    GetWindowThreadProcessId(hwnd, &processId);

    if (UlongToHandle(processId) == context->ProcessId &&
        !((parentWindow = GetParent(hwnd)) && IsWindowVisible(parentWindow)) && // skip windows with a visible parent
        PhGetWindowTextEx(hwnd, PH_GET_WINDOW_TEXT_INTERNAL | PH_GET_WINDOW_TEXT_LENGTH_ONLY, NULL) != 0) // skip windows with no title
    {
        if (!context->ImmersiveWindow && context->IsImmersive &&
            GetProp(hwnd, L"Windows.ImmersiveShell.IdentifyAsMainCoreWindow"))
        {
            context->ImmersiveWindow = hwnd;
        }

        windowInfo.cbSize = sizeof(WINDOWINFO);

        if (!context->Window && GetWindowInfo(hwnd, &windowInfo) && (windowInfo.dwStyle & WS_DLGFRAME))
        {
            context->Window = hwnd;

            // If we're not looking at an immersive process, there's no need to search any more windows.
            if (!context->IsImmersive)
                return FALSE;
        }
    }

    return TRUE;
}

HWND PhGetProcessMainWindow(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle
    )
{
    GET_PROCESS_MAIN_WINDOW_CONTEXT context;
    HANDLE processHandle = NULL;

    memset(&context, 0, sizeof(GET_PROCESS_MAIN_WINDOW_CONTEXT));
    context.ProcessId = ProcessId;

    if (ProcessHandle)
        processHandle = ProcessHandle;
    else
        PhOpenProcess(&processHandle, ProcessQueryAccess, ProcessId);

    if (processHandle && IsImmersiveProcess_I)
        context.IsImmersive = IsImmersiveProcess_I(processHandle);

    PhEnumChildWindows(NULL, 0x800, PhpGetProcessMainWindowEnumWindowsProc, (LPARAM)&context);

    if (!ProcessHandle && processHandle)
        NtClose(processHandle);

    return context.ImmersiveWindow ? context.ImmersiveWindow : context.Window;
}

PPH_STRING PhGetServiceRelevantFileName(
    _In_ PPH_STRINGREF ServiceName,
    _In_ SC_HANDLE ServiceHandle
    )
{
    PPH_STRING fileName = NULL;
    LPQUERY_SERVICE_CONFIG config;

    if (config = PhGetServiceConfig(ServiceHandle))
    {
        PhGetServiceDllParameter(ServiceName, &fileName);

        if (!fileName)
        {
            PPH_STRING commandLine;

            commandLine = PhCreateString(config->lpBinaryPathName);

            if (config->dwServiceType & SERVICE_WIN32)
            {
                PH_STRINGREF dummyFileName;
                PH_STRINGREF dummyArguments;

                PhParseCommandLineFuzzy(&commandLine->sr, &dummyFileName, &dummyArguments, &fileName);

                if (!fileName)
                    PhSwapReference(&fileName, commandLine);
            }
            else
            {
                fileName = PhGetFileName(commandLine);
            }

            PhDereferenceObject(commandLine);
        }

        PhFree(config);
    }

    return fileName;
}

PPH_STRING PhEscapeStringForDelimiter(
    _In_ PPH_STRING String,
    _In_ WCHAR Delimiter
    )
{
    PH_STRING_BUILDER stringBuilder;
    SIZE_T length;
    SIZE_T i;
    WCHAR temp[2];

    length = String->Length / 2;
    PhInitializeStringBuilder(&stringBuilder, String->Length / 2 * 3);

    temp[0] = '\\';

    for (i = 0; i < length; i++)
    {
        if (String->Buffer[i] == '\\' || String->Buffer[i] == Delimiter)
        {
            temp[1] = String->Buffer[i];
            PhAppendStringBuilderEx(&stringBuilder, temp, 4);
        }
        else
        {
            PhAppendCharStringBuilder(&stringBuilder, String->Buffer[i]);
        }
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PhUnescapeStringForDelimiter(
    _In_ PPH_STRING String,
    _In_ WCHAR Delimiter
    )
{
    PH_STRING_BUILDER stringBuilder;
    SIZE_T length;
    SIZE_T i;

    length = String->Length / 2;
    PhInitializeStringBuilder(&stringBuilder, String->Length / 2 * 3);

    for (i = 0; i < length; i++)
    {
        if (String->Buffer[i] == '\\')
        {
            if (i != length - 1)
            {
                PhAppendCharStringBuilder(&stringBuilder, String->Buffer[i + 1]);
                i++;
            }
            else
            {
                // Trailing backslash. Just ignore it.
                break;
            }
        }
        else
        {
            PhAppendCharStringBuilder(&stringBuilder, String->Buffer[i]);
        }
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PhGetOpaqueXmlNodeText(
    _In_ mxml_node_t *node
    )
{
    if (node->child && node->child->type == MXML_OPAQUE && node->child->value.opaque)
    {
        return PhConvertUtf8ToUtf16(node->child->value.opaque);
    }
    else
    {
        return PhReferenceEmptyString();
    }
}

VOID PhSearchOnlineString(
    _In_ HWND hWnd,
    _In_ PWSTR String
    )
{
    PhShellExecuteUserString(hWnd, L"SearchEngine", String, TRUE, NULL);
}

VOID PhShellExecuteUserString(
    _In_ HWND hWnd,
    _In_ PWSTR Setting,
    _In_ PWSTR String,
    _In_ BOOLEAN UseShellExecute,
    _In_opt_ PWSTR ErrorMessage
    )
{
    static PH_STRINGREF replacementToken = PH_STRINGREF_INIT(L"%s");

    PPH_STRING executeString;
    PH_STRINGREF stringBefore;
    PH_STRINGREF stringMiddle;
    PH_STRINGREF stringAfter;
    PPH_STRING newString;
    PPH_STRING ntMessage;

    executeString = PhGetStringSetting(Setting);

    // Make sure the user executable string is absolute.
    // We can't use RtlDetermineDosPathNameType_U here because the string
    // may be a URL.
    if (PhFindCharInString(executeString, 0, ':') == -1)
    {
        newString = PhConcatStringRef2(&PhApplicationDirectory->sr, &executeString->sr);
        PhDereferenceObject(executeString);
        executeString = newString;
    }

    // Replace "%s" with the string, or use the original string if "%s" is not present.
    if (PhSplitStringRefAtString(&executeString->sr, &replacementToken, FALSE, &stringBefore, &stringAfter))
    {
        PhInitializeStringRef(&stringMiddle, String);
        newString = PhConcatStringRef3(&stringBefore, &stringMiddle, &stringAfter);
    }
    else
    {
        PhSetReference(&newString, executeString);
    }

    PhDereferenceObject(executeString);

    if (UseShellExecute)
    {
        PhShellExecute(hWnd, newString->Buffer, NULL);
    }
    else
    {
        NTSTATUS status;

        status = PhCreateProcessWin32(NULL, newString->Buffer, NULL, NULL, 0, NULL, NULL, NULL);

        if (!NT_SUCCESS(status))
        {
            if (ErrorMessage)
            {
                ntMessage = PhGetNtMessage(status);
                PhShowError(hWnd, L"Unable to execute the command: %s\n%s", PhGetStringOrDefault(ntMessage, L"An unknown error occurred."), ErrorMessage);
                PhDereferenceObject(ntMessage);
            }
            else
            {
                PhShowStatus(hWnd, L"Unable to execute the command", status, 0);
            }
        }
    }

    PhDereferenceObject(newString);
}

VOID PhLoadSymbolProviderOptions(
    _Inout_ PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    PPH_STRING searchPath;

    PhSetOptionsSymbolProvider(
        SYMOPT_UNDNAME,
        PhGetIntegerSetting(L"DbgHelpUndecorate") ? SYMOPT_UNDNAME : 0
        );

    searchPath = PhGetStringSetting(L"DbgHelpSearchPath");

    if (searchPath->Length != 0)
        PhSetSearchPathSymbolProvider(SymbolProvider, searchPath->Buffer);

    PhDereferenceObject(searchPath);
}

PWSTR PhMakeContextAtom(
    VOID
    )
{
    PH_DEFINE_MAKE_ATOM(L"PH2_Context");
}

/**
 * Copies a string into a NMLVGETINFOTIP structure.
 *
 * \param GetInfoTip The NMLVGETINFOTIP structure.
 * \param Tip The string to copy.
 *
 * \remarks The text is truncated if it is too long.
 */
VOID PhCopyListViewInfoTip(
    _Inout_ LPNMLVGETINFOTIP GetInfoTip,
    _In_ PPH_STRINGREF Tip
    )
{
    ULONG copyIndex;
    ULONG bufferRemaining;
    ULONG copyLength;

    if (GetInfoTip->dwFlags == 0)
    {
        copyIndex = (ULONG)PhCountStringZ(GetInfoTip->pszText) + 1; // plus one for newline

        if (GetInfoTip->cchTextMax - copyIndex < 2) // need at least two bytes
            return;

        bufferRemaining = GetInfoTip->cchTextMax - copyIndex - 1;
        GetInfoTip->pszText[copyIndex - 1] = '\n';
    }
    else
    {
        copyIndex = 0;
        bufferRemaining = GetInfoTip->cchTextMax;
    }

    copyLength = min((ULONG)Tip->Length / 2, bufferRemaining - 1);
    memcpy(
        &GetInfoTip->pszText[copyIndex],
        Tip->Buffer,
        copyLength * 2
        );
    GetInfoTip->pszText[copyIndex + copyLength] = 0;
}

VOID PhCopyListView(
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING text;

    text = PhGetListViewText(ListViewHandle);
    PhSetClipboardString(ListViewHandle, &text->sr);
    PhDereferenceObject(text);
}

VOID PhHandleListViewNotifyForCopy(
    _In_ LPARAM lParam,
    _In_ HWND ListViewHandle
    )
{
    PhHandleListViewNotifyBehaviors(lParam, ListViewHandle, PH_LIST_VIEW_CTRL_C_BEHAVIOR);
}

VOID PhHandleListViewNotifyBehaviors(
    _In_ LPARAM lParam,
    _In_ HWND ListViewHandle,
    _In_ ULONG Behaviors
    )
{
    if (((LPNMHDR)lParam)->hwndFrom == ListViewHandle && ((LPNMHDR)lParam)->code == LVN_KEYDOWN)
    {
        LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)lParam;

        switch (keyDown->wVKey)
        {
        case 'C':
            if (Behaviors & PH_LIST_VIEW_CTRL_C_BEHAVIOR)
            {
                if (GetKeyState(VK_CONTROL) < 0)
                    PhCopyListView(ListViewHandle);
            }
            break;
        case 'A':
            if (Behaviors & PH_LIST_VIEW_CTRL_A_BEHAVIOR)
            {
                if (GetKeyState(VK_CONTROL) < 0)
                    PhSetStateAllListViewItems(ListViewHandle, LVIS_SELECTED, LVIS_SELECTED);
            }
            break;
        }
    }
}

BOOLEAN PhGetListViewContextMenuPoint(
    _In_ HWND ListViewHandle,
    _Out_ PPOINT Point
    )
{
    INT selectedIndex;
    RECT bounds;
    RECT clientRect;

    // The user pressed a key to display the context menu.
    // Suggest where the context menu should display.

    if ((selectedIndex = ListView_GetNextItem(ListViewHandle, -1, LVNI_SELECTED)) != -1)
    {
        if (ListView_GetItemRect(ListViewHandle, selectedIndex, &bounds, LVIR_BOUNDS))
        {
            Point->x = bounds.left + PhSmallIconSize.X / 2;
            Point->y = bounds.top + PhSmallIconSize.Y / 2;

            GetClientRect(ListViewHandle, &clientRect);

            if (Point->x < 0 || Point->y < 0 || Point->x >= clientRect.right || Point->y >= clientRect.bottom)
            {
                // The menu is going to be outside of the control. Just put it at the top-left.
                Point->x = 0;
                Point->y = 0;
            }

            ClientToScreen(ListViewHandle, Point);

            return TRUE;
        }
    }

    Point->x = 0;
    Point->y = 0;
    ClientToScreen(ListViewHandle, Point);

    return FALSE;
}

HFONT PhDuplicateFontWithNewWeight(
    _In_ HFONT Font,
    _In_ LONG NewWeight
    )
{
    LOGFONT logFont;

    if (GetObject(Font, sizeof(LOGFONT), &logFont))
    {
        logFont.lfWeight = NewWeight;
        return CreateFontIndirect(&logFont);
    }
    else
    {
        return NULL;
    }
}

VOID PhSetWindowOpacity(
    _In_ HWND WindowHandle,
    _In_ ULONG OpacityPercent
    )
{
    if (OpacityPercent == 0)
    {
        // Make things a bit faster by removing the WS_EX_LAYERED bit.
        PhSetWindowExStyle(WindowHandle, WS_EX_LAYERED, 0);
        RedrawWindow(WindowHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
        return;
    }

    PhSetWindowExStyle(WindowHandle, WS_EX_LAYERED, WS_EX_LAYERED);

    // Disallow opacity values of less than 10%.
    OpacityPercent = min(OpacityPercent, 90);

    // The opacity value is backwards - 0 means opaque, 100 means transparent.
    SetLayeredWindowAttributes(
        WindowHandle,
        0,
        (BYTE)(255 * (100 - OpacityPercent) / 100),
        LWA_ALPHA
        );
}

VOID PhLoadWindowPlacementFromSetting(
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName,
    _In_ HWND WindowHandle
    )
{
    PH_RECTANGLE windowRectangle;

    if (PositionSettingName && SizeSettingName)
    {
        RECT rectForAdjust;

        windowRectangle.Position = PhGetIntegerPairSetting(PositionSettingName);
        windowRectangle.Size = PhGetIntegerPairSetting(SizeSettingName);

        PhAdjustRectangleToWorkingArea(
            WindowHandle,
            &windowRectangle
            );

        // Let the window adjust for the minimum size if needed.
        rectForAdjust = PhRectangleToRect(windowRectangle);
        SendMessage(WindowHandle, WM_SIZING, WMSZ_BOTTOMRIGHT, (LPARAM)&rectForAdjust);
        windowRectangle = PhRectToRectangle(rectForAdjust);

        MoveWindow(WindowHandle, windowRectangle.Left, windowRectangle.Top,
            windowRectangle.Width, windowRectangle.Height, FALSE);
    }
    else
    {
        PH_INTEGER_PAIR position;
        PH_INTEGER_PAIR size;
        ULONG flags;

        flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER;

        if (PositionSettingName)
        {
            position = PhGetIntegerPairSetting(PositionSettingName);
            flags &= ~SWP_NOMOVE;
        }
        else
        {
            position.X = 0;
            position.Y = 0;
        }

        if (SizeSettingName)
        {
            size = PhGetIntegerPairSetting(SizeSettingName);
            flags &= ~SWP_NOSIZE;
        }
        else
        {
            size.X = 16;
            size.Y = 16;
        }

        SetWindowPos(WindowHandle, NULL, position.X, position.Y, size.X, size.Y, flags);
    }
}

VOID PhSaveWindowPlacementToSetting(
    _In_opt_ PWSTR PositionSettingName,
    _In_opt_ PWSTR SizeSettingName,
    _In_ HWND WindowHandle
    )
{
    WINDOWPLACEMENT placement = { sizeof(placement) };
    PH_RECTANGLE windowRectangle;
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };

    GetWindowPlacement(WindowHandle, &placement);
    windowRectangle = PhRectToRectangle(placement.rcNormalPosition);

    // The rectangle is in workspace coordinates. Convert the values back to screen coordinates.
    if (GetMonitorInfo(MonitorFromRect(&placement.rcNormalPosition, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
    {
        windowRectangle.Left += monitorInfo.rcWork.left - monitorInfo.rcMonitor.left;
        windowRectangle.Top += monitorInfo.rcWork.top - monitorInfo.rcMonitor.top;
    }

    if (PositionSettingName)
        PhSetIntegerPairSetting(PositionSettingName, windowRectangle.Position);
    if (SizeSettingName)
        PhSetIntegerPairSetting(SizeSettingName, windowRectangle.Size);
}

VOID PhLoadListViewColumnsFromSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING string;

    string = PhGetStringSetting(Name);
    PhLoadListViewColumnSettings(ListViewHandle, string);
    PhDereferenceObject(string);
}

VOID PhSaveListViewColumnsToSetting(
    _In_ PWSTR Name,
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING string;

    string = PhSaveListViewColumnSettings(ListViewHandle);
    PhSetStringSetting2(Name, &string->sr);
    PhDereferenceObject(string);
}

PPH_STRING PhGetPhVersion(
    VOID
    )
{
    PH_FORMAT format[3];

    PhInitFormatU(&format[0], PHAPP_VERSION_MAJOR);
    PhInitFormatC(&format[1], '.');
    PhInitFormatU(&format[2], PHAPP_VERSION_MINOR);

    return PhFormat(format, 3, 16);
}

VOID PhGetPhVersionNumbers(
    _Out_opt_ PULONG MajorVersion,
    _Out_opt_ PULONG MinorVersion,
    _Reserved_ PULONG Reserved,
    _Out_opt_ PULONG RevisionNumber
    )
{
    if (MajorVersion)
        *MajorVersion = PHAPP_VERSION_MAJOR;
    if (MinorVersion)
        *MinorVersion = PHAPP_VERSION_MINOR;
    if (RevisionNumber)
        *RevisionNumber = PHAPP_VERSION_REVISION;
}

VOID PhWritePhTextHeader(
    _Inout_ PPH_FILE_STREAM FileStream
    )
{
    PPH_STRING version;
    LARGE_INTEGER time;
    SYSTEMTIME systemTime;
    PPH_STRING dateString;
    PPH_STRING timeString;

    PhWriteStringAsUtf8FileStream2(FileStream, L"Process Hacker ");

    if (version = PhGetPhVersion())
    {
        PhWriteStringAsUtf8FileStream(FileStream, &version->sr);
        PhDereferenceObject(version);
    }

    PhWriteStringFormatAsUtf8FileStream(FileStream, L"\r\nWindows NT %u.%u", PhOsVersion.dwMajorVersion, PhOsVersion.dwMinorVersion);

    if (PhOsVersion.szCSDVersion[0] != 0)
        PhWriteStringFormatAsUtf8FileStream(FileStream, L" %s", PhOsVersion.szCSDVersion);

#ifdef _WIN64
    PhWriteStringAsUtf8FileStream2(FileStream, L" (64-bit)");
#else
    PhWriteStringAsUtf8FileStream2(FileStream, L" (32-bit)");
#endif

    PhQuerySystemTime(&time);
    PhLargeIntegerToLocalSystemTime(&systemTime, &time);

    dateString = PhFormatDate(&systemTime, NULL);
    timeString = PhFormatTime(&systemTime, NULL);
    PhWriteStringFormatAsUtf8FileStream(FileStream, L"\r\n%s %s\r\n\r\n", dateString->Buffer, timeString->Buffer);
    PhDereferenceObject(dateString);
    PhDereferenceObject(timeString);
}

BOOLEAN PhShellProcessHacker(
    _In_opt_ HWND hWnd,
    _In_opt_ PWSTR Parameters,
    _In_ ULONG ShowWindowType,
    _In_ ULONG Flags,
    _In_ ULONG AppFlags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    )
{
    return PhShellProcessHackerEx(
        hWnd,
        NULL,
        Parameters,
        ShowWindowType,
        Flags,
        AppFlags,
        Timeout,
        ProcessHandle
        );
}

BOOLEAN PhShellProcessHackerEx(
    _In_opt_ HWND hWnd,
    _In_opt_ PWSTR FileName,
    _In_opt_ PWSTR Parameters,
    _In_ ULONG ShowWindowType,
    _In_ ULONG Flags,
    _In_ ULONG AppFlags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    )
{
    BOOLEAN result;
    PH_STRING_BUILDER sb;
    PWSTR parameters;
    PPH_STRING temp;

    if (AppFlags & PH_SHELL_APP_PROPAGATE_PARAMETERS)
    {
        PhInitializeStringBuilder(&sb, 128);

        // Propagate parameters.

        if (PhStartupParameters.NoSettings)
        {
            PhAppendStringBuilder2(&sb, L" -nosettings");
        }
        else if (PhStartupParameters.SettingsFileName && (PhSettingsFileName || (AppFlags & PH_SHELL_APP_PROPAGATE_PARAMETERS_FORCE_SETTINGS)))
        {
            PhAppendStringBuilder2(&sb, L" -settings \"");

            if (PhSettingsFileName)
                temp = PhEscapeCommandLinePart(&PhSettingsFileName->sr);
            else
                temp = PhEscapeCommandLinePart(&PhStartupParameters.SettingsFileName->sr);

            PhAppendStringBuilder(&sb, &temp->sr);
            PhDereferenceObject(temp);
            PhAppendCharStringBuilder(&sb, '\"');
        }

        if (PhStartupParameters.NoKph)
        {
            PhAppendStringBuilder2(&sb, L" -nokph");
        }

        if (PhStartupParameters.NoPlugins)
        {
            PhAppendStringBuilder2(&sb, L" -noplugins");
        }

        if (PhStartupParameters.NewInstance)
        {
            PhAppendStringBuilder2(&sb, L" -newinstance");
        }

        if (PhStartupParameters.SelectTab)
        {
            PhAppendStringBuilder2(&sb, L" -selecttab \"");
            temp = PhEscapeCommandLinePart(&PhStartupParameters.SelectTab->sr);
            PhAppendStringBuilder(&sb, &temp->sr);
            PhDereferenceObject(temp);
            PhAppendCharStringBuilder(&sb, '\"');
        }

        if (!(AppFlags & PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY))
        {
            if (PhStartupParameters.ShowVisible)
            {
                PhAppendStringBuilder2(&sb, L" -v");
            }

            if (PhStartupParameters.ShowHidden)
            {
                PhAppendStringBuilder2(&sb, L" -hide");
            }
        }

        // Add user-specified parameters last so they can override the propagated parameters.
        if (Parameters)
        {
            PhAppendCharStringBuilder(&sb, ' ');
            PhAppendStringBuilder2(&sb, Parameters);
        }

        if (sb.String->Length != 0 && sb.String->Buffer[0] == ' ')
            parameters = sb.String->Buffer + 1;
        else
            parameters = sb.String->Buffer;
    }
    else
    {
        parameters = Parameters;
    }

    result = PhShellExecuteEx(
        hWnd,
        FileName ? FileName : PhApplicationFileName->Buffer,
        parameters,
        ShowWindowType,
        Flags,
        Timeout,
        ProcessHandle
        );

    if (AppFlags & PH_SHELL_APP_PROPAGATE_PARAMETERS)
        PhDeleteStringBuilder(&sb);

    return result;
}

BOOLEAN PhCreateProcessIgnoreIfeoDebugger(
    _In_ PWSTR FileName
    )
{
    BOOLEAN result;
    BOOL (NTAPI *debugSetProcessKillOnExit)(BOOL);
    BOOL (NTAPI *debugActiveProcessStop)(DWORD);
    BOOLEAN originalValue;
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;

    if (!(debugSetProcessKillOnExit = PhGetModuleProcAddress(L"kernel32.dll", "DebugSetProcessKillOnExit")) ||
        !(debugActiveProcessStop = PhGetModuleProcAddress(L"kernel32.dll", "DebugActiveProcessStop")))
        return FALSE;

    result = FALSE;

    // This is NOT thread-safe.
    originalValue = NtCurrentPeb()->ReadImageFileExecOptions;
    NtCurrentPeb()->ReadImageFileExecOptions = FALSE;

    memset(&startupInfo, 0, sizeof(STARTUPINFO));
    startupInfo.cb = sizeof(STARTUPINFO);
    memset(&processInfo, 0, sizeof(PROCESS_INFORMATION));

    // The combination of ReadImageFileExecOptions = FALSE and the DEBUG_PROCESS flag
    // allows us to skip the Debugger IFEO value.
    if (CreateProcess(FileName, NULL, NULL, NULL, FALSE, DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &startupInfo, &processInfo))
    {
        // Stop debugging taskmgr.exe now.
        debugSetProcessKillOnExit(FALSE);
        debugActiveProcessStop(processInfo.dwProcessId);
        result = TRUE;
    }

    if (processInfo.hProcess)
        NtClose(processInfo.hProcess);
    if (processInfo.hThread)
        NtClose(processInfo.hThread);

    NtCurrentPeb()->ReadImageFileExecOptions = originalValue;

    return result;
}

VOID PhInitializeTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    )
{
    PhInitializeTreeNewColumnMenuEx(Data, 0);
}

VOID PhInitializeTreeNewColumnMenuEx(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data,
    _In_ ULONG Flags
    )
{
    PPH_EMENU_ITEM resetSortMenuItem = NULL;
    PPH_EMENU_ITEM sizeColumnToFitMenuItem;
    PPH_EMENU_ITEM sizeAllColumnsToFitMenuItem;
    PPH_EMENU_ITEM hideColumnMenuItem;
    PPH_EMENU_ITEM chooseColumnsMenuItem;
    ULONG minimumNumberOfColumns;

    Data->Menu = PhCreateEMenu();
    Data->Selection = NULL;
    Data->ProcessedId = 0;

    sizeColumnToFitMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_SIZE_COLUMN_TO_FIT_ID, L"Size Column to Fit", NULL, NULL);
    sizeAllColumnsToFitMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_SIZE_ALL_COLUMNS_TO_FIT_ID, L"Size All Columns to Fit", NULL, NULL);

    if (!(Flags & PH_TN_COLUMN_MENU_NO_VISIBILITY))
    {
        hideColumnMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_HIDE_COLUMN_ID, L"Hide Column", NULL, NULL);
        chooseColumnsMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID, L"Choose Columns...", NULL, NULL);
    }

    if (Flags & PH_TN_COLUMN_MENU_SHOW_RESET_SORT)
    {
        ULONG sortColumn;
        PH_SORT_ORDER sortOrder;

        TreeNew_GetSort(Data->TreeNewHandle, &sortColumn, &sortOrder);

        if (sortOrder != Data->DefaultSortOrder || (Data->DefaultSortOrder != NoSortOrder && sortColumn != Data->DefaultSortColumn))
            resetSortMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_RESET_SORT_ID, L"Reset Sort", NULL, NULL);
    }

    PhInsertEMenuItem(Data->Menu, sizeColumnToFitMenuItem, -1);
    PhInsertEMenuItem(Data->Menu, sizeAllColumnsToFitMenuItem, -1);

    if (!(Flags & PH_TN_COLUMN_MENU_NO_VISIBILITY))
    {
        PhInsertEMenuItem(Data->Menu, hideColumnMenuItem, -1);

        if (resetSortMenuItem)
            PhInsertEMenuItem(Data->Menu, resetSortMenuItem, -1);

        PhInsertEMenuItem(Data->Menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, L"", NULL, NULL), -1);
        PhInsertEMenuItem(Data->Menu, chooseColumnsMenuItem, -1);

        if (TreeNew_GetFixedColumn(Data->TreeNewHandle))
            minimumNumberOfColumns = 2; // don't allow user to remove all normal columns (the fixed column can never be removed)
        else
            minimumNumberOfColumns = 1;

        if (!Data->MouseEvent || !Data->MouseEvent->Column ||
            Data->MouseEvent->Column->Fixed || // don't allow the fixed column to be hidden
            TreeNew_GetVisibleColumnCount(Data->TreeNewHandle) < minimumNumberOfColumns + 1
            )
        {
            hideColumnMenuItem->Flags |= PH_EMENU_DISABLED;
        }
    }
    else
    {
        if (resetSortMenuItem)
            PhInsertEMenuItem(Data->Menu, resetSortMenuItem, -1);
    }

    if (!Data->MouseEvent || !Data->MouseEvent->Column)
    {
        sizeColumnToFitMenuItem->Flags |= PH_EMENU_DISABLED;
    }
}

VOID PhpEnsureValidSortColumnTreeNew(
    _Inout_ HWND TreeNewHandle,
    _In_ ULONG DefaultSortColumn,
    _In_ PH_SORT_ORDER DefaultSortOrder
    )
{
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    // Make sure the column we're sorting by is actually visible, and if not, don't sort anymore.

    TreeNew_GetSort(TreeNewHandle, &sortColumn, &sortOrder);

    if (sortOrder != NoSortOrder)
    {
        PH_TREENEW_COLUMN column;

        TreeNew_GetColumn(TreeNewHandle, sortColumn, &column);

        if (!column.Visible)
        {
            if (DefaultSortOrder != NoSortOrder)
            {
                // Make sure the default sort column is visible.
                TreeNew_GetColumn(TreeNewHandle, DefaultSortColumn, &column);

                if (!column.Visible)
                {
                    ULONG maxId;
                    ULONG id;
                    BOOLEAN found;

                    // Use the first visible column.
                    maxId = TreeNew_GetMaxId(TreeNewHandle);
                    id = 0;
                    found = FALSE;

                    while (id <= maxId)
                    {
                        if (TreeNew_GetColumn(TreeNewHandle, id, &column))
                        {
                            if (column.Visible)
                            {
                                DefaultSortColumn = id;
                                found = TRUE;
                                break;
                            }
                        }

                        id++;
                    }

                    if (!found)
                    {
                        DefaultSortColumn = 0;
                        DefaultSortOrder = NoSortOrder;
                    }
                }
            }

            TreeNew_SetSort(TreeNewHandle, DefaultSortColumn, DefaultSortOrder);
        }
    }
}

BOOLEAN PhHandleTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    )
{
    if (!Data->Selection)
        return FALSE;

    switch (Data->Selection->Id)
    {
    case PH_TN_COLUMN_MENU_RESET_SORT_ID:
        {
            TreeNew_SetSort(Data->TreeNewHandle, Data->DefaultSortColumn, Data->DefaultSortOrder);
        }
        break;
    case PH_TN_COLUMN_MENU_SIZE_COLUMN_TO_FIT_ID:
        {
            if (Data->MouseEvent && Data->MouseEvent->Column)
            {
                TreeNew_AutoSizeColumn(Data->TreeNewHandle, Data->MouseEvent->Column->Id, 0);
            }
        }
        break;
    case PH_TN_COLUMN_MENU_SIZE_ALL_COLUMNS_TO_FIT_ID:
        {
            ULONG maxId;
            ULONG id;

            maxId = TreeNew_GetMaxId(Data->TreeNewHandle);
            id = 0;

            while (id <= maxId)
            {
                TreeNew_AutoSizeColumn(Data->TreeNewHandle, id, 0);
                id++;
            }
        }
        break;
    case PH_TN_COLUMN_MENU_HIDE_COLUMN_ID:
        {
            PH_TREENEW_COLUMN column;

            if (Data->MouseEvent && Data->MouseEvent->Column && !Data->MouseEvent->Column->Fixed)
            {
                column.Id = Data->MouseEvent->Column->Id;
                column.Visible = FALSE;
                TreeNew_SetColumn(Data->TreeNewHandle, TN_COLUMN_FLAG_VISIBLE, &column);
                PhpEnsureValidSortColumnTreeNew(Data->TreeNewHandle, Data->DefaultSortColumn, Data->DefaultSortOrder);
                InvalidateRect(Data->TreeNewHandle, NULL, FALSE);
            }
        }
        break;
    case PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID:
        {
            PhShowChooseColumnsDialog(Data->TreeNewHandle, Data->TreeNewHandle, PH_CONTROL_TYPE_TREE_NEW);
            PhpEnsureValidSortColumnTreeNew(Data->TreeNewHandle, Data->DefaultSortColumn, Data->DefaultSortOrder);
        }
        break;
    default:
        return FALSE;
    }

    Data->ProcessedId = Data->Selection->Id;

    return TRUE;
}

VOID PhDeleteTreeNewColumnMenu(
    _In_ PPH_TN_COLUMN_MENU_DATA Data
    )
{
    if (Data->Menu)
    {
        PhDestroyEMenu(Data->Menu);
        Data->Menu = NULL;
    }
}

VOID PhInitializeTreeNewFilterSupport(
    _Out_ PPH_TN_FILTER_SUPPORT Support,
    _In_ HWND TreeNewHandle,
    _In_ PPH_LIST NodeList
    )
{
    Support->FilterList = NULL;
    Support->TreeNewHandle = TreeNewHandle;
    Support->NodeList = NodeList;
}

VOID PhDeleteTreeNewFilterSupport(
    _In_ PPH_TN_FILTER_SUPPORT Support
    )
{
    PhDereferenceObject(Support->FilterList);
}

PPH_TN_FILTER_ENTRY PhAddTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_FUNCTION Filter,
    _In_opt_ PVOID Context
    )
{
    PPH_TN_FILTER_ENTRY entry;

    entry = PhAllocate(sizeof(PH_TN_FILTER_ENTRY));
    entry->Filter = Filter;
    entry->Context = Context;

    if (!Support->FilterList)
        Support->FilterList = PhCreateList(2);

    PhAddItemList(Support->FilterList, entry);

    return entry;
}

VOID PhRemoveTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_ENTRY Entry
    )
{
    ULONG index;

    if (!Support->FilterList)
        return;

    index = PhFindItemList(Support->FilterList, Entry);

    if (index != -1)
    {
        PhRemoveItemList(Support->FilterList, index);
        PhFree(Entry);
    }
}

BOOLEAN PhApplyTreeNewFiltersToNode(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TREENEW_NODE Node
    )
{
    BOOLEAN show;
    ULONG i;

    show = TRUE;

    if (Support->FilterList)
    {
        for (i = 0; i < Support->FilterList->Count; i++)
        {
            PPH_TN_FILTER_ENTRY entry;

            entry = Support->FilterList->Items[i];

            if (!entry->Filter(Node, entry->Context))
            {
                show = FALSE;
                break;
            }
        }
    }

    return show;
}

VOID PhApplyTreeNewFilters(
    _In_ PPH_TN_FILTER_SUPPORT Support
    )
{
    ULONG i;

    for (i = 0; i < Support->NodeList->Count; i++)
    {
        PPH_TREENEW_NODE node;

        node = Support->NodeList->Items[i];
        node->Visible = PhApplyTreeNewFiltersToNode(Support, node);

        if (!node->Visible && node->Selected)
        {
            node->Selected = FALSE;
        }
    }

    TreeNew_NodesStructured(Support->TreeNewHandle);
}

VOID NTAPI PhpCopyCellEMenuItemDeleteFunction(
    _In_ struct _PH_EMENU_ITEM *Item
    )
{
    PPH_COPY_CELL_CONTEXT context;

    context = Item->Context;
    PhDereferenceObject(context->MenuItemText);
    PhFree(context);
}

BOOLEAN PhInsertCopyCellEMenuItem(
    _In_ struct _PH_EMENU_ITEM *Menu,
    _In_ ULONG InsertAfterId,
    _In_ HWND TreeNewHandle,
    _In_ PPH_TREENEW_COLUMN Column
    )
{
    PPH_EMENU_ITEM parentItem;
    ULONG indexInParent;
    PPH_COPY_CELL_CONTEXT context;
    PH_STRINGREF columnText;
    PPH_STRING escapedText;
    PPH_STRING menuItemText;
    PPH_EMENU_ITEM copyCellItem;

    if (!Column)
        return FALSE;

    if (!PhFindEMenuItemEx(Menu, 0, NULL, InsertAfterId, &parentItem, &indexInParent))
        return FALSE;

    indexInParent++;

    context = PhAllocate(sizeof(PH_COPY_CELL_CONTEXT));
    context->TreeNewHandle = TreeNewHandle;
    context->Id = Column->Id;

    PhInitializeStringRef(&columnText, Column->Text);
    escapedText = PhEscapeStringForMenuPrefix(&columnText);
    menuItemText = PhFormatString(L"Copy \"%s\"", escapedText->Buffer);
    PhDereferenceObject(escapedText);
    copyCellItem = PhCreateEMenuItem(0, ID_COPY_CELL, menuItemText->Buffer, NULL, context);
    copyCellItem->DeleteFunction = PhpCopyCellEMenuItemDeleteFunction;
    context->MenuItemText = menuItemText;

    if (Column->CustomDraw)
        copyCellItem->Flags |= PH_EMENU_DISABLED;

    PhInsertEMenuItem(parentItem, copyCellItem, indexInParent);

    return TRUE;
}

BOOLEAN PhHandleCopyCellEMenuItem(
    _In_ struct _PH_EMENU_ITEM *SelectedItem
    )
{
    PPH_COPY_CELL_CONTEXT context;
    PH_STRING_BUILDER stringBuilder;
    ULONG count;
    ULONG selectedCount;
    ULONG i;
    PPH_TREENEW_NODE node;
    PH_TREENEW_GET_CELL_TEXT getCellText;

    if (!SelectedItem)
        return FALSE;
    if (SelectedItem->Id != ID_COPY_CELL)
        return FALSE;

    context = SelectedItem->Context;

    PhInitializeStringBuilder(&stringBuilder, 0x100);
    count = TreeNew_GetFlatNodeCount(context->TreeNewHandle);
    selectedCount = 0;

    for (i = 0; i < count; i++)
    {
        node = TreeNew_GetFlatNode(context->TreeNewHandle, i);

        if (node && node->Selected)
        {
            selectedCount++;

            getCellText.Flags = 0;
            getCellText.Node = node;
            getCellText.Id = context->Id;
            PhInitializeEmptyStringRef(&getCellText.Text);
            TreeNew_GetCellText(context->TreeNewHandle, &getCellText);

            PhAppendStringBuilder(&stringBuilder, &getCellText.Text);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
        }
    }

    if (stringBuilder.String->Length != 0 && selectedCount == 1)
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhSetClipboardString(context->TreeNewHandle, &stringBuilder.String->sr);
    PhDeleteStringBuilder(&stringBuilder);

    return TRUE;
}

BOOLEAN PhpSelectFavoriteInRegedit(
    _In_ HWND RegeditWindow,
    _In_ PPH_STRINGREF FavoriteName,
    _In_ BOOLEAN UsePhSvc
    )
{
    HMENU menu;
    HMENU favoritesMenu;
    ULONG count;
    ULONG i;
    ULONG id = -1;

    if (!(menu = GetMenu(RegeditWindow)))
        return FALSE;

    // Cause the Registry Editor to refresh the Favorites menu.
    if (UsePhSvc)
        PhSvcCallSendMessage(RegeditWindow, WM_MENUSELECT, MAKEWPARAM(3, MF_POPUP), (LPARAM)menu);
    else
        SendMessage(RegeditWindow, WM_MENUSELECT, MAKEWPARAM(3, MF_POPUP), (LPARAM)menu);

    if (!(favoritesMenu = GetSubMenu(menu, 3)))
        return FALSE;

    // Find our entry.

    count = GetMenuItemCount(favoritesMenu);

    if (count == -1)
        return FALSE;
    if (count > 1000)
        count = 1000;

    for (i = 3; i < count; i++)
    {
        MENUITEMINFO info = { sizeof(MENUITEMINFO) };
        WCHAR buffer[32];

        info.fMask = MIIM_ID | MIIM_STRING;
        info.dwTypeData = buffer;
        info.cch = sizeof(buffer) / sizeof(WCHAR);
        GetMenuItemInfo(favoritesMenu, i, TRUE, &info);

        if (info.cch == FavoriteName->Length / 2)
        {
            PH_STRINGREF text;

            text.Buffer = buffer;
            text.Length = info.cch * 2;

            if (PhEqualStringRef(&text, FavoriteName, TRUE))
            {
                id = info.wID;
                break;
            }
        }
    }

    if (id == -1)
        return FALSE;

    // Activate our entry.
    if (UsePhSvc)
        PhSvcCallSendMessage(RegeditWindow, WM_COMMAND, MAKEWPARAM(id, 0), 0);
    else
        SendMessage(RegeditWindow, WM_COMMAND, MAKEWPARAM(id, 0), 0);

    // "Close" the Favorites menu and restore normal status bar text.
    if (UsePhSvc)
        PhSvcCallPostMessage(RegeditWindow, WM_MENUSELECT, MAKEWPARAM(0, 0xffff), 0);
    else
        PostMessage(RegeditWindow, WM_MENUSELECT, MAKEWPARAM(0, 0xffff), 0);

    // Bring regedit to the top.
    if (IsIconic(RegeditWindow))
    {
        ShowWindow(RegeditWindow, SW_RESTORE);
        SetForegroundWindow(RegeditWindow);
    }
    else
    {
        SetForegroundWindow(RegeditWindow);
    }

    return TRUE;
}

/**
 * Opens a key in the Registry Editor. If the Registry Editor is already open,
 * the specified key is selected in the Registry Editor.
 *
 * \param hWnd A handle to the parent window.
 * \param KeyName The key name to open.
 */
BOOLEAN PhShellOpenKey2(
    _In_ HWND hWnd,
    _In_ PPH_STRING KeyName
    )
{
    static PH_STRINGREF favoritesKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit\\Favorites");

    BOOLEAN result = FALSE;
    HWND regeditWindow;
    HANDLE favoritesKeyHandle;
    WCHAR favoriteName[32];
    UNICODE_STRING valueName;
    PH_STRINGREF valueNameSr;
    PPH_STRING expandedKeyName;

    regeditWindow = FindWindow(L"RegEdit_RegEdit", NULL);

    if (!regeditWindow)
    {
        PhShellOpenKey(hWnd, KeyName);
        return TRUE;
    }

    if (!PhElevated)
    {
        if (!PhUiConnectToPhSvc(hWnd, FALSE))
            return FALSE;
    }

    // Create our entry in Favorites.

    if (!NT_SUCCESS(PhCreateKey(
        &favoritesKeyHandle,
        KEY_WRITE,
        PH_KEY_CURRENT_USER,
        &favoritesKeyName,
        0,
        0,
        NULL
        )))
        goto CleanupExit;

    memcpy(favoriteName, L"A_ProcessHacker", 15 * sizeof(WCHAR));
    PhGenerateRandomAlphaString(&favoriteName[15], 16);
    RtlInitUnicodeString(&valueName, favoriteName);
    PhUnicodeStringToStringRef(&valueName, &valueNameSr);

    expandedKeyName = PhExpandKeyName(KeyName, TRUE);
    NtSetValueKey(favoritesKeyHandle, &valueName, 0, REG_SZ, expandedKeyName->Buffer, (ULONG)expandedKeyName->Length + 2);
    PhDereferenceObject(expandedKeyName);

    // Select our entry in regedit.
    result = PhpSelectFavoriteInRegedit(regeditWindow, &valueNameSr, !PhElevated);

    NtDeleteValueKey(favoritesKeyHandle, &valueName);
    NtClose(favoritesKeyHandle);

CleanupExit:
    if (!PhElevated)
        PhUiDisconnectFromPhSvc();

    return result;
}

PPH_STRING PhPcre2GetErrorMessage(
    _In_ INT ErrorCode
    )
{
    PPH_STRING buffer;
    SIZE_T bufferLength;
    INT_PTR returnLength;

    bufferLength = 128 * sizeof(WCHAR);
    buffer = PhCreateStringEx(NULL, bufferLength);

    while (TRUE)
    {
        if ((returnLength = pcre2_get_error_message(ErrorCode, buffer->Buffer, bufferLength / sizeof(WCHAR) + 1)) >= 0)
            break;

        PhDereferenceObject(buffer);
        bufferLength *= 2;

        if (bufferLength > 0x1000 * sizeof(WCHAR))
            break;

        buffer = PhCreateStringEx(NULL, bufferLength);
    }

    if (returnLength < 0)
        return NULL;

    buffer->Length = returnLength * sizeof(WCHAR);
    return buffer;
}
