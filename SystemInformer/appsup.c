/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#include <phapp.h>

#include <cpysave.h>
#include <emenu.h>
#include <symprv.h>
#include <settings.h>

#include <actions.h>
#include <mainwnd.h>
#include <lsasup.h>
#include <phappres.h>
#include <phsvccl.h>
#include <thirdparty.h>
#include <phconsole.h>

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

    return Process->UserTime.QuadPart + Process->KernelTime.QuadPart != 0 && Process->NumberOfThreads != 0;
}

BOOLEAN PhIsProcessSuspended(
    _In_ HANDLE ProcessId
    )
{
    BOOLEAN suspended = FALSE;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;

    if (NT_SUCCESS(PhEnumProcesses(&processes)))
    {
        if (process = PhFindProcessInformation(processes, ProcessId))
        {
            suspended = PhGetProcessIsSuspended(process);
        }

        PhFree(processes);
    }

    return suspended;
}

BOOLEAN PhIsProcessBackground(
    _In_ ULONG PriorityClass
    )
{
    // This is similar to PROCESS_MODE_BACKGROUND_BEGIN (dmex)
    if (
        PriorityClass == PROCESS_PRIORITY_CLASS_BELOW_NORMAL ||
        PriorityClass == PROCESS_PRIORITY_CLASS_IDLE
        )
    {
        return TRUE;
    }

    return FALSE;
}

static CONST PH_KEY_VALUE_PAIR ProcessPriorityClassTypePairs[] =
{
    SIP(SREF(L"Unknown"), PROCESS_PRIORITY_CLASS_UNKNOWN),
    SIP(SREF(L"Idle"), PROCESS_PRIORITY_CLASS_IDLE),
    SIP(SREF(L"Normal"), PROCESS_PRIORITY_CLASS_NORMAL),
    SIP(SREF(L"High"), PROCESS_PRIORITY_CLASS_HIGH),
    SIP(SREF(L"Real time"), PROCESS_PRIORITY_CLASS_REALTIME),
    SIP(SREF(L"Below normal"), PROCESS_PRIORITY_CLASS_BELOW_NORMAL),
    SIP(SREF(L"Above normal"), PROCESS_PRIORITY_CLASS_ABOVE_NORMAL),
};

PCPH_STRINGREF PhGetProcessPriorityClassString(
    _In_ ULONG PriorityClass
    )
{
    PCPH_STRINGREF string;

    if (PhIndexStringRefSiKeyValuePairs(
        ProcessPriorityClassTypePairs,
        sizeof(ProcessPriorityClassTypePairs),
        PriorityClass,
        &string
        ))
    {
        return string;
    }

    static_assert(ARRAYSIZE(ProcessPriorityClassTypePairs) == PROCESS_PRIORITY_CLASS_ABOVE_NORMAL + 1, "PriorityClassString must equal PROCESS_PRIORITY_CLASS_MAX");

    return (PCPH_STRINGREF)ProcessPriorityClassTypePairs[PROCESS_PRIORITY_CLASS_UNKNOWN].Key;

    //switch (PriorityClass)
    //{
    //case PROCESS_PRIORITY_CLASS_REALTIME:
    //    return L"Real time";
    //case PROCESS_PRIORITY_CLASS_HIGH:
    //    return L"High";
    //case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL:
    //    return L"Above normal";
    //case PROCESS_PRIORITY_CLASS_NORMAL:
    //    return L"Normal";
    //case PROCESS_PRIORITY_CLASS_BELOW_NORMAL:
    //    return L"Below normal";
    //case PROCESS_PRIORITY_CLASS_IDLE:
    //    return L"Idle";
    //case PROCESS_PRIORITY_CLASS_UNKNOWN:
    //default:
    //    return L"Unknown";
    //}
}

static CONST PH_KEY_VALUE_PAIR PhProtectedTypeStrings[] =
{
    SIP(L"None", NULL), // PsProtectedTypeNone
    SIP(L"Light", PsProtectedTypeProtectedLight),
    SIP(L"Full", PsProtectedTypeProtected),
};

static CONST PH_KEY_VALUE_PAIR PhProtectedSignerStrings[] =
{
    SIP(L"None", NULL), // PsProtectedSignerNone
    SIP(L"Authenticode", PsProtectedSignerAuthenticode),
    SIP(L"CodeGen", PsProtectedSignerCodeGen),
    SIP(L"Antimalware", PsProtectedSignerAntimalware),
    SIP(L"Lsa", PsProtectedSignerLsa),
    SIP(L"Windows", PsProtectedSignerWindows),
    SIP(L"WinTcb", PsProtectedSignerWinTcb),
    SIP(L"WinSystem", PsProtectedSignerWinSystem),
    SIP(L"StoreApp", PsProtectedSignerApp),
};

static_assert(RTL_NUMBER_OF(PhProtectedTypeStrings) == PsProtectedTypeMax, "PsProtectedTypeStrings must equal PsProtectedTypeMax (value offsets)");
static_assert(RTL_NUMBER_OF(PhProtectedSignerStrings) == PsProtectedSignerMax, "PhProtectedSignerStrings must equal PsProtectedSignerMax (value offsets)");

PPH_STRING PhGetProcessProtectionString(
    _In_ PS_PROTECTION Protection,
    _In_ BOOLEAN IsSecureProcess
    )
{
    if (Protection.Level)
    {
        static PPH_STRING PhpProtectionNoneString = NULL;
        PH_FORMAT format[6];
        ULONG count = 0;
        PCWSTR type;
        PCWSTR signer;

        if (!PhpProtectionNoneString)
            PhpProtectionNoneString = PhCreateString(L"None");

        if (IsSecureProcess)
        {
            PhInitFormatS(&format[count++], L"Secure ");
        }

        if (PhIndexStringSiKeyValuePairs(
            PhProtectedTypeStrings,
            sizeof(PhProtectedTypeStrings),
            Protection.Type,
            &type
            ))
        {
            PhInitFormatS(&format[count++], type);
        }
        else
        {
            PhInitFormatS(&format[count++], L"Unknown");
        }

        if (PhIndexStringSiKeyValuePairs(
            PhProtectedSignerStrings,
            sizeof(PhProtectedSignerStrings),
            Protection.Signer,
            &signer
            ))
        {
            PhInitFormatS(&format[count++], L" (");
            PhInitFormatS(&format[count++], signer);
            PhInitFormatS(&format[count++], L")");
        }
        else
        {
            PhInitFormatS(&format[count++], L" (");
            PhInitFormatS(&format[count++], L"Unknown");
            PhInitFormatS(&format[count++], L")");
        }

        if (Protection.Audit)
        {
            PhInitFormatS(&format[count++], L" (Audit)");
        }

        return PhFormat(format, count, 10);
    }
    else
    {
        static PPH_STRING PhpProtectionSecureIUMString = NULL;

        if (!PhpProtectionSecureIUMString)
            PhpProtectionSecureIUMString = PhCreateString(L"Secure (IUM)");

        if (IsSecureProcess)
        {
            return PhReferenceObject(PhpProtectionSecureIUMString);
        }
    }

    return NULL;
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
            if (!NT_SUCCESS(status = NtReadVirtualMemory(
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
            if (!NT_SUCCESS(status = NtReadVirtualMemory(
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
            if (!NT_SUCCESS(status = NtReadVirtualMemory(
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
            if (!NT_SUCCESS(status = NtReadVirtualMemory(
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

    if (WindowsVersion >= WINDOWS_10_RS5)
    {
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(data, 2040 + 24), // Magic value from SbReadProcContextByHandle
            Guid,
            sizeof(GUID),
            NULL
            )))
            return status;
    }
    else if (WindowsVersion >= WINDOWS_10_RS2)
    {
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(data, 1544),
            Guid,
            sizeof(GUID),
            NULL
            )))
            return status;
    }
    else if (WindowsVersion >= WINDOWS_10)
    {
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
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
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
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
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
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
        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(data, 32), // Magic value from WdcGetProcessSwitchContext
            Guid,
            sizeof(GUID),
            NULL
            )))
            return status;
    }

    //static ULONG (WINAPI *BaseReadAppCompatDataForProcess_I)(
    //    _In_ HANDLE ProcessHandle,
    //    _Out_ PULONG_PTR ShimData,
    //    _Out_opt_ PULONG_PTR ShimDataBaseAddress
    //    ) = NULL;
    //static ULONG (WINAPI *BaseFreeAppCompatDataForProcess_I)(
    //    _In_ ULONG_PTR ShimData
    //    ) = NULL;
    //
    //if (!BaseReadAppCompatDataForProcess_I)
    //    BaseReadAppCompatDataForProcess_I = PhGetDllProcedureAddress(L"kernelbase.dll", "BaseReadAppCompatDataForProcess", 0);
    //if (!BaseFreeAppCompatDataForProcess_I)
    //    BaseFreeAppCompatDataForProcess_I = PhGetDllProcedureAddress(L"kernelbase.dll", "BaseFreeAppCompatDataForProcess", 0);
    //
    //if (BaseReadAppCompatDataForProcess_I && BaseFreeAppCompatDataForProcess_I)
    //{
    //    ULONG_PTR pShimData;
    //
    //    if (BaseReadAppCompatDataForProcess_I(ProcessHandle, &pShimData, NULL) == ERROR_SUCCESS)
    //    {
    //        if (WindowsVersion >= WINDOWS_10_RS5)
    //        {
    //            *Guid = *(PGUID)PTR_ADD_OFFSET(pShimData, 2040 + 24);
    //            BaseFreeAppCompatDataForProcess_I(pShimData);
    //            return STATUS_SUCCESS;
    //        }
    //
    //        BaseFreeAppCompatDataForProcess_I(pShimData);
    //    }
    //
    //    return STATUS_UNSUCCESSFUL;
    //}

    return STATUS_SUCCESS;
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
    PROCESS_BASIC_INFORMATION basicInfo;
    PPH_STRING fileName;

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

    if (!NT_SUCCESS(status = PhGetProcessImageFileName(
        ProcessHandle,
        &fileName
        )))
    {
        return status;
    }

    *KnownProcessType = PhGetProcessKnownTypeEx(
        basicInfo.UniqueProcessId,
        fileName
        );

    PhDereferenceObject(fileName);

    return status;
}

PH_KNOWN_PROCESS_TYPE PhGetProcessKnownTypeEx(
    _In_opt_ HANDLE ProcessId,
    _In_ PPH_STRING FileName
    )
{
    PH_KNOWN_PROCESS_TYPE knownProcessType;
    PH_STRINGREF systemRootPrefix;
    PPH_STRING fileName;
    PH_STRINGREF name;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    if (ProcessId == SYSTEM_PROCESS_ID || ProcessId == SYSTEM_IDLE_PROCESS_ID)
        return SystemProcessType;

    if (PhIsNullOrEmptyString(FileName))
        return UnknownProcessType;

    PhGetNtSystemRoot(&systemRootPrefix);

    fileName = PhReferenceObject(FileName);
    name = fileName->sr;

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
            || (PhStartsWithStringRef2(&name, L"\\SysWOW64", TRUE) && (isWow64 = TRUE, TRUE)) // ugly but necessary
#ifdef _M_ARM64
            || (PhStartsWithStringRef2(&name, L"\\SysArm32", TRUE) && (isWow64 = TRUE, TRUE)) // ugly but necessary
            || (PhStartsWithStringRef2(&name, L"\\SyChpe32", TRUE) && (isWow64 = TRUE, TRUE)) // ugly but necessary
#endif
#endif
            )
        {
            // System32, SysWow64, SysArm32, and SyChpe32 are all 8 characters long.
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
            else if (PhEqualStringRef2(&name, L"\\wbem\\WmiPrvSE.exe", TRUE))
                knownProcessType = WmiProviderHostType;
            //else if (PhEqualStringRef2(&name, L"\\MicrosoftEdgeCP.exe", TRUE)) // RS5
            //    knownProcessType = EdgeProcessType;
            //else if (PhEqualStringRef2(&name, L"\\MicrosoftEdgeSH.exe", TRUE)) // RS5
            //    knownProcessType = EdgeProcessType;
#ifdef _M_IX86
            else if (PhEqualStringRef2(&name, L"\\ntvdm.exe", TRUE))
                knownProcessType = NtVdmHostProcessType;
#endif
        }
        //else
        //{
        //    if (PhEndsWithStringRef2(&name, L"\\MicrosoftEdgeCP.exe", TRUE)) // RS4
        //        knownProcessType = EdgeProcessType;
        //    else if (PhEndsWithStringRef2(&name, L"\\MicrosoftEdge.exe", TRUE))
        //        knownProcessType = EdgeProcessType;
        //    else if (PhEndsWithStringRef2(&name, L"\\ServiceWorkerHost.exe", TRUE))
        //        knownProcessType = EdgeProcessType;
        //    else if (PhEndsWithStringRef2(&name, L"\\Windows.WARP.JITService.exe", TRUE))
        //        knownProcessType = EdgeProcessType;
        //}
    }

    PhDereferenceObject(fileName);

#ifdef _WIN64
    if (isWow64)
        knownProcessType |= KnownProcessWow64;
#endif

    return knownProcessType;
}

static BOOLEAN NTAPI PhpSvchostCommandLineCallback(
    _In_opt_ PCPH_COMMAND_LINE_OPTION Option,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PVOID Context
    )
{
    PPH_KNOWN_PROCESS_COMMAND_LINE knownCommandLine = Context;

    if (knownCommandLine && Option && Option->Id == 1)
    {
        PhSwapReference(&knownCommandLine->ServiceHost.GroupName, Value);
    }

    return TRUE;
}

_Success_(return)
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

            static CONST PH_COMMAND_LINE_OPTION options[] =
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
                PH_AUTO(KnownCommandLine->ServiceHost.GroupName);
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

            while (i < CommandLine->Length / sizeof(WCHAR) && CommandLine->Buffer[i] == L' ')
                i++;

            dllName = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!dllName)
                return FALSE;

            PH_AUTO(dllName);

            // The procedure name begins after the last comma.

            if (!PhSplitStringRefAtLastChar(&dllName->sr, L',', &dllNamePart, &procedureNamePart))
                return FALSE;

            dllName = PH_AUTO(PhCreateString2(&dllNamePart));
            procedureName = PH_AUTO(PhCreateString2(&procedureNamePart));

            // If the DLL name isn't an absolute path, assume it's in system32.
            // TODO: Use a proper search function.

            if (PhDetermineDosPathNameType(&dllName->sr) == RtlPathTypeRelative)
            {
                dllName = PhGetSystemDirectoryWin32(&dllName->sr);
            }

            KnownCommandLine->RunDllAsApp.FileName = dllName;
            KnownCommandLine->RunDllAsApp.ProcedureName = procedureName;
        }
        break;
    case ComSurrogateProcessType:
        {
            // dllhost.exe /processid:<Guid>

            static CONST PH_STRINGREF inprocServer32Name = PH_STRINGREF_INIT(L"InprocServer32");

            SIZE_T i;
            ULONG_PTR indexOfProcessId;
            PPH_STRING argPart;
            PPH_STRING guidString;
            GUID guid;
            HANDLE rootKeyHandle;
            HANDLE inprocServer32KeyHandle;
            PPH_STRING fileName;

            i = 0;

            // Get the dllhost.exe part.

            argPart = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!argPart)
                return FALSE;

            PhDereferenceObject(argPart);

            // Get the argument part.

            while (i < (ULONG)CommandLine->Length / sizeof(WCHAR) && CommandLine->Buffer[i] == L' ')
                i++;

            argPart = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!argPart)
                return FALSE;

            PH_AUTO(argPart);

            // Find "/processid:"; the GUID is just after that.

            PhUpperStringRef(&argPart->sr);
            indexOfProcessId = PhFindStringInString(argPart, 0, L"/PROCESSID:");

            if (indexOfProcessId == SIZE_MAX)
                return FALSE;

            guidString = PhaSubstring(
                argPart,
                indexOfProcessId + 11,
                argPart->Length / sizeof(WCHAR) - indexOfProcessId - 11
                );

            if (!NT_SUCCESS(PhStringToGuid(&guidString->sr, &guid)))
                return FALSE;

            KnownCommandLine->ComSurrogate.Guid = guid;
            KnownCommandLine->ComSurrogate.Name = NULL;
            KnownCommandLine->ComSurrogate.FileName = NULL;

            // Lookup the GUID in the registry to determine the name and file name.

            if (NT_SUCCESS(PhOpenKey(
                &rootKeyHandle,
                KEY_READ,
                PH_KEY_CLASSES_ROOT,
                &PhaConcatStrings2(L"CLSID\\", guidString->Buffer)->sr,
                0
                )))
            {
                KnownCommandLine->ComSurrogate.Name = PH_AUTO(PhQueryRegistryString(rootKeyHandle, NULL));

                if (NT_SUCCESS(PhOpenKey(
                    &inprocServer32KeyHandle,
                    KEY_READ,
                    rootKeyHandle,
                    &inprocServer32Name,
                    0
                    )))
                {
                    KnownCommandLine->ComSurrogate.FileName = PH_AUTO(PhQueryRegistryString(inprocServer32KeyHandle, NULL));

                    if (fileName = PH_AUTO(PhExpandEnvironmentStrings(&KnownCommandLine->ComSurrogate.FileName->sr)))
                    {
                        KnownCommandLine->ComSurrogate.FileName = fileName;
                    }

                    NtClose(inprocServer32KeyHandle);
                }

                NtClose(rootKeyHandle);
            }
            else if (NT_SUCCESS(PhOpenKey(
                &rootKeyHandle,
                KEY_READ,
                PH_KEY_CLASSES_ROOT,
                &PhaConcatStrings2(L"AppID\\", guidString->Buffer)->sr,
                0
                )))
            {
                KnownCommandLine->ComSurrogate.Name = PH_AUTO(PhQueryRegistryString(rootKeyHandle, NULL));
                NtClose(rootKeyHandle);
            }
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
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

    length = String->Length / sizeof(WCHAR);
    PhInitializeStringBuilder(&stringBuilder, String->Length / sizeof(WCHAR) * 3);

    temp[0] = L'\\';

    for (i = 0; i < length; i++)
    {
        if (String->Buffer[i] == L'\\' || String->Buffer[i] == Delimiter)
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

    length = String->Length / sizeof(WCHAR);
    PhInitializeStringBuilder(&stringBuilder, String->Length / sizeof(WCHAR) * 3);

    for (i = 0; i < length; i++)
    {
        if (String->Buffer[i] == L'\\')
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

VOID PhSearchOnlineString(
    _In_ HWND WindowHandle,
    _In_ PCWSTR String
    )
{
    PhShellExecuteUserString(WindowHandle, L"SearchEngine", String, TRUE, NULL);
}

VOID PhShellExecuteUserString(
    _In_ HWND WindowHandle,
    _In_ PCWSTR Setting,
    _In_ PCWSTR String,
    _In_ BOOLEAN UseShellExecute,
    _In_opt_ PCWSTR ErrorMessage
    )
{
    static CONST PH_STRINGREF replacementToken = PH_STRINGREF_INIT(L"%s");
    PPH_STRING applicationDirectory;
    PPH_STRING executeString;
    PH_STRINGREF stringBefore;
    PH_STRINGREF stringAfter;
    PPH_STRING ntMessage;

    if (!(applicationDirectory = PhGetApplicationDirectoryWin32()))
    {
        PhShowStatus(WindowHandle, L"Unable to locate the application directory.", STATUS_NOT_FOUND, 0);
        return;
    }

    // Get the execute command. (dmex)
    executeString = PhGetStringSetting(Setting);

    // Expand environment strings. (dmex)
    PhMoveReference(&executeString, PhExpandEnvironmentStrings(&executeString->sr));

    // Make sure the user executable string is absolute. We can't use PhDetermineDosPathNameType
    // here because the string may be a URL. (dmex)
    if (PhFindCharInString(executeString, 0, L':') == SIZE_MAX)
    {
        static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"\"");
        static CONST PH_STRINGREF space = PH_STRINGREF_INIT(L" ");
        PPH_LIST stringArgList;
        PPH_STRING fileName = NULL;
        PPH_STRING fileArgs = NULL;

        // HACK: Escape the individual executeString components. (dmex)

        if (stringArgList = PhCommandLineToList(executeString->Buffer))
        {
            fileName = PhReferenceObject(stringArgList->Items[0]);

            if (stringArgList->Count == 2)
            {
                fileArgs = PhReferenceObject(stringArgList->Items[1]);
            }

            PhDereferenceObjects(stringArgList->Items, stringArgList->Count);
            PhDereferenceObject(stringArgList);
        }

        if (fileName && fileArgs)
        {
            // Make sure the string is absolute and escape the filename.
            if (PhDetermineDosPathNameType(&fileName->sr) == RtlPathTypeRelative)
            {
                PhMoveReference(&fileName, PhConcatStringRef3(&separator, &applicationDirectory->sr, &fileName->sr));
                PhMoveReference(&fileName, PhConcatStringRef2(&fileName->sr, &separator));
            }
            else
            {
                PhMoveReference(&fileName, PhConcatStringRef3(&separator, &fileName->sr, &separator));
            }

            // Escape the parameters.
            PhMoveReference(&fileArgs, PhConcatStringRef3(&separator, &fileArgs->sr, &separator));

            // Create the escaped execute string.
            PhMoveReference(&executeString, PhConcatStringRef3(&fileName->sr, &space, &fileArgs->sr));
        }
        else
        {
            if (PhDetermineDosPathNameType(&executeString->sr) == RtlPathTypeRelative)
            {
                PhMoveReference(&executeString, PhConcatStringRef3(&separator, &applicationDirectory->sr, &executeString->sr));
                PhMoveReference(&executeString, PhConcatStringRef2(&executeString->sr, &separator));
            }
            else
            {
                PhMoveReference(&executeString, PhConcatStringRef3(&separator, &executeString->sr, &separator));
            }
        }

        PhClearReference(&fileArgs);
        PhClearReference(&fileName);
    }

    // Replace the token with the string, or use the original string if the token is not present.
    if (PhSplitStringRefAtString(&executeString->sr, &replacementToken, FALSE, &stringBefore, &stringAfter))
    {
        // Note: See PhGetProcessImageFileNameWin32 for a description of
        // the issue with some filenames and faulty RamDisk software (dmex)

        if (String[0] == OBJ_NAME_PATH_SEPARATOR) // Workaround faulty software (dmex)
        {
            PPH_STRING stringTemp;
            PPH_STRING stringMiddle;

            stringTemp = PhCreateString(String);
            stringMiddle = PhGetFileName(stringTemp);

            PhMoveReference(&executeString, PhConcatStringRef3(&stringBefore, &stringMiddle->sr, &stringAfter));

            PhDereferenceObject(stringMiddle);
            PhDereferenceObject(stringTemp);
        }
        else
        {
            PH_STRINGREF stringMiddle;

            PhInitializeStringRefLongHint(&stringMiddle, String);

            PhMoveReference(&executeString, PhConcatStringRef3(&stringBefore, &stringMiddle, &stringAfter));
        }
    }

    if (UseShellExecute)
    {
        PhShellExecute(WindowHandle, executeString->Buffer, NULL);
    }
    else
    {
        NTSTATUS status;
        HANDLE processHandle;

        status = PhCreateProcessWin32(
            NULL,
            executeString->Buffer,
            NULL,
            NULL,
            0,
            NULL,
            &processHandle,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            PhConsoleSetForeground(processHandle, TRUE);
            NtClose(processHandle);
        }
        else
        {
            if (ErrorMessage)
            {
                ntMessage = PhGetNtMessage(status);
                PhShowError2(
                    WindowHandle,
                    L"Unable to execute the command.",
                    L"%s\n%s",
                    PhGetStringOrDefault(ntMessage, L"An unknown error occurred."),
                    ErrorMessage
                    );
                PhDereferenceObject(ntMessage);
            }
            else
            {
                PhShowStatus(WindowHandle, L"Unable to execute the command.", status, 0);
            }
        }
    }

    PhDereferenceObject(executeString);
    PhDereferenceObject(applicationDirectory);
}

VOID PhLoadSymbolProviderOptions(
    _Inout_ PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    static CONST PH_STRINGREF symbolPath = PH_STRINGREF_INIT(L"_NT_SYMBOL_PATH");
    PPH_STRING searchPath = NULL;

    PhSetOptionsSymbolProvider(
        PH_SYMOPT_UNDNAME,
        PhGetIntegerSetting(L"DbgHelpUndecorate") ? PH_SYMOPT_UNDNAME : 0
        );

    PhQueryEnvironmentVariable(NULL, &symbolPath, &searchPath);

    if (PhIsNullOrEmptyString(searchPath))
        searchPath = PhGetStringSetting(L"DbgHelpSearchPath");
    if (!PhIsNullOrEmptyString(searchPath))
        PhSetSearchPathSymbolProvider(SymbolProvider, searchPath->Buffer);
    if (searchPath)
        PhDereferenceObject(searchPath);
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
        GetInfoTip->pszText[copyIndex - 1] = L'\n';
    }
    else
    {
        copyIndex = 0;
        bufferRemaining = GetInfoTip->cchTextMax;
    }

    copyLength = min((ULONG)Tip->Length / sizeof(WCHAR), bufferRemaining - 1);
    memcpy(
        &GetInfoTip->pszText[copyIndex],
        Tip->Buffer,
        copyLength * sizeof(WCHAR)
        );
    GetInfoTip->pszText[copyIndex + copyLength] = UNICODE_NULL;
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

VOID PhCopyIListView(
    _In_ HWND ListViewHandle,
    _In_ IListView* ListView
    )
{
    PPH_STRING text;

    text = PhGetIListViewText(ListView);
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
#pragma warning(push)
#pragma warning(disable:26454) // disable Windows SDK warning (dmex)
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
#pragma warning(pop)
}

BOOLEAN PhGetListViewContextMenuPoint(
    _In_ HWND ListViewHandle,
    _Out_ PPOINT Point
    )
{
    LONG selectedIndex;
    RECT bounds;
    RECT clientRect;

    // The user pressed a key to display the context menu.
    // Suggest where the context menu should display.

    if ((selectedIndex = PhFindListViewItemByFlags(ListViewHandle, INT_ERROR, LVNI_SELECTED)) != INT_ERROR)
    {
        if (ListView_GetItemRect(ListViewHandle, selectedIndex, &bounds, LVIR_BOUNDS))
        {
            LONG dpiValue = PhGetWindowDpi(ListViewHandle);

            Point->x = bounds.left + PhGetSystemMetrics(SM_CXSMICON, dpiValue) / 2;
            Point->y = bounds.top + PhGetSystemMetrics(SM_CYSMICON, dpiValue) / 2;

            if (!PhGetClientRect(ListViewHandle, &clientRect))
                return FALSE;

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

BOOLEAN PhGetIListViewContextMenuPoint(
    _In_ IListView* ListView,
    _Out_ PPOINT Point
    )
{
    LONG selectedIndex;
    RECT bounds;
    RECT clientRect;

    // The user pressed a key to display the context menu.
    // Suggest where the context menu should display.

    if ((selectedIndex = PhFindIListViewItemByFlags(ListView, INT_ERROR, LVNI_SELECTED)) != INT_ERROR)
    {
        if (PhGetIListViewItemRect(ListView, selectedIndex, LVIR_BOUNDS, &bounds))
        {
            //LONG dpiValue = PhGetWindowDpi(ListViewHandle);

            //Point->x = bounds.left + PhGetSystemMetrics(SM_CXSMICON, dpiValue) / 2;
            //Point->y = bounds.top + PhGetSystemMetrics(SM_CYSMICON, dpiValue) / 2;

            PhGetIListViewClientRect(ListView, &clientRect);

            if (Point->x < 0 || Point->y < 0 || Point->x >= clientRect.right || Point->y >= clientRect.bottom)
            {
                // The menu is going to be outside of the control. Just put it at the top-left.
                Point->x = 0;
                Point->y = 0;
            }

            //ClientToScreen(ListViewHandle, Point);

            return TRUE;
        }
    }

    Point->x = 0;
    Point->y = 0;
    //ClientToScreen(ListViewHandle, Point);

    return FALSE;
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

PPH_STRING PhGetPhVersion(
    VOID
    )
{
    PH_FORMAT format[5];

    PhInitFormatU(&format[0], PHAPP_VERSION_MAJOR);
    PhInitFormatC(&format[1], L'.');
    PhInitFormatU(&format[2], PHAPP_VERSION_MINOR);
    PhInitFormatC(&format[3], L'.');
    PhInitFormatU(&format[4], PHAPP_VERSION_BUILD);
    PhInitFormatC(&format[3], L'.');
    PhInitFormatU(&format[4], PHAPP_VERSION_REVISION);

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

VOID PhGetPhVersionNumbers(
    _Out_opt_ PULONG MajorVersion,
    _Out_opt_ PULONG MinorVersion,
    _Out_opt_ PULONG BuildNumber,
    _Out_opt_ PULONG RevisionNumber
    )
{
    if (MajorVersion)
        *MajorVersion = PHAPP_VERSION_MAJOR;
    if (MinorVersion)
        *MinorVersion = PHAPP_VERSION_MINOR;
    if (BuildNumber)
        *BuildNumber = PHAPP_VERSION_BUILD;
    if (RevisionNumber)
        *RevisionNumber = PHAPP_VERSION_REVISION;
}

PPH_STRING PhGetPhVersionHash(
    VOID
    )
{
    return PhConvertUtf8ToUtf16(PHAPP_VERSION_COMMIT);
}

PH_RELEASE_CHANNEL PhGetPhReleaseChannel(
    VOID
    )
{
    return PhGetIntegerSetting(L"ReleaseChannel");
}

PCWSTR PhGetPhReleaseChannelString(
    VOID
    )
{
    switch (PhGetIntegerSetting(L"ReleaseChannel"))
    {
    case PhReleaseChannel:
        return L"Release";
    case PhPreviewChannel:
        return L"Preview";
    case PhCanaryChannel:
        return L"Canary";
    case PhDeveloperChannel:
        return L"Developer";
    }

    return L"Unknown";
}

VOID PhWritePhTextHeader(
    _Inout_ PPH_FILE_STREAM FileStream
    )
{
    PPH_STRING version;
    LARGE_INTEGER time;
    SYSTEMTIME systemTime;
    PPH_STRING timeString;

    PhWriteStringAsUtf8FileStream2(FileStream, L"System Informer ");

    if (version = PhGetPhVersion())
    {
        PhWriteStringAsUtf8FileStream(FileStream, &version->sr);
        PhDereferenceObject(version);
    }

    PhWriteStringFormatAsUtf8FileStream(FileStream, L"\r\nWindows NT %lu.%lu", PhOsVersion.MajorVersion, PhOsVersion.MinorVersion);

    if (PhOsVersion.CSDVersion[0] != UNICODE_NULL)
        PhWriteStringFormatAsUtf8FileStream(FileStream, L" %s", PhOsVersion.CSDVersion);

#ifdef _WIN64
    PhWriteStringAsUtf8FileStream2(FileStream, L" (64-bit)");
#else
    PhWriteStringAsUtf8FileStream2(FileStream, L" (32-bit)");
#endif

    PhQuerySystemTime(&time);
    PhLargeIntegerToLocalSystemTime(&systemTime, &time);

    timeString = PhFormatDateTime(&systemTime);
    PhWriteStringFormatAsUtf8FileStream(FileStream, L"\r\n%s\r\n\r\n", timeString->Buffer);
    PhDereferenceObject(timeString);
}

NTSTATUS PhShellProcessHacker(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PCWSTR Parameters,
    _In_ ULONG ShowWindowType,
    _In_ ULONG Flags,
    _In_ ULONG AppFlags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    )
{
    return PhShellProcessHackerEx(
        WindowHandle,
        NULL,
        Parameters,
        ShowWindowType,
        Flags,
        AppFlags,
        Timeout,
        ProcessHandle
        );
}

VOID PhpAppendCommandLineArgument(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ PCWSTR Name,
    _In_ PPH_STRINGREF Value
    )
{
    PPH_STRING temp;

    PhAppendStringBuilder2(StringBuilder, L" -");
    PhAppendStringBuilder2(StringBuilder, Name);
    PhAppendStringBuilder2(StringBuilder, L" \"");
    temp = PhEscapeCommandLinePart(Value);
    PhAppendStringBuilder(StringBuilder, &temp->sr);
    PhDereferenceObject(temp);
    PhAppendCharStringBuilder(StringBuilder, L'\"');
}

NTSTATUS PhShellProcessHackerEx(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PCWSTR FileName,
    _In_opt_ PCWSTR Parameters,
    _In_ ULONG ShowWindowType,
    _In_ ULONG Flags,
    _In_ ULONG AppFlags,
    _In_opt_ ULONG Timeout,
    _Out_opt_ PHANDLE ProcessHandle
    )
{
    NTSTATUS status;
    PPH_STRING applicationFileName;
    PH_STRING_BUILDER sb;
    PCWSTR parameters;

    if (!(applicationFileName = PhGetApplicationFileNameWin32()))
        return FALSE;

    if (AppFlags & PH_SHELL_APP_PROPAGATE_PARAMETERS)
    {
        PhInitializeStringBuilder(&sb, 128);

        // Propagate parameters.

        if (PhStartupParameters.NoSettings)
        {
            PhAppendStringBuilder2(&sb, L" -nosettings");
        }
        else
        {
            if (!PhIsNullOrEmptyString(PhSettingsFileName))
            {
                PhAppendStringBuilder2(&sb, L" -settings \"");
                PhAppendStringBuilder(&sb, &PhSettingsFileName->sr);
                PhAppendStringBuilder2(&sb, L"\"");
            }
        }

        if (PhStartupParameters.NoKph)
            PhAppendStringBuilder2(&sb, L" -nokph");
        if (PhStartupParameters.Debug)
            PhAppendStringBuilder2(&sb, L" -debug");
        if (PhStartupParameters.NoPlugins)
            PhAppendStringBuilder2(&sb, L" -noplugins");
        if (PhStartupParameters.NewInstance)
            PhAppendStringBuilder2(&sb, L" -newinstance");

        if (PhStartupParameters.SelectPid != 0)
            PhAppendFormatStringBuilder(&sb, L" -selectpid %lu", PhStartupParameters.SelectPid);

        if (PhStartupParameters.PriorityClass != 0)
        {
            CHAR value = 0;

            switch (PhStartupParameters.PriorityClass)
            {
            case PROCESS_PRIORITY_CLASS_REALTIME:
                value = L'r';
                break;
            case PROCESS_PRIORITY_CLASS_HIGH:
                value = L'h';
                break;
            case PROCESS_PRIORITY_CLASS_NORMAL:
                value = L'n';
                break;
            case PROCESS_PRIORITY_CLASS_IDLE:
                value = L'l';
                break;
            }

            if (value != 0)
            {
                PhAppendStringBuilder2(&sb, L" -priority ");
                PhAppendCharStringBuilder(&sb, value);
            }
        }

        if (PhStartupParameters.PluginParameters)
        {
            ULONG i;

            for (i = 0; i < PhStartupParameters.PluginParameters->Count; i++)
            {
                PPH_STRING value = PhStartupParameters.PluginParameters->Items[i];
                PhpAppendCommandLineArgument(&sb, L"plugin", &value->sr);
            }
        }

        if (PhStartupParameters.SelectTab)
            PhpAppendCommandLineArgument(&sb, L"selecttab", &PhStartupParameters.SelectTab->sr);

        if (!(AppFlags & PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY))
        {
            if (PhStartupParameters.ShowVisible)
                PhAppendStringBuilder2(&sb, L" -v");
            if (PhStartupParameters.ShowHidden)
                PhAppendStringBuilder2(&sb, L" -hide");
        }

        // Add user-specified parameters last so they can override the propagated parameters.
        if (Parameters)
        {
            PhAppendCharStringBuilder(&sb, L' ');
            PhAppendStringBuilder2(&sb, Parameters);
        }

        if (sb.String->Length != 0 && sb.String->Buffer[0] == L' ')
            parameters = sb.String->Buffer + 1;
        else
            parameters = sb.String->Buffer;
    }
    else
    {
        parameters = Parameters;
    }

    status = PhShellExecuteEx(
        WindowHandle,
        FileName ? FileName : PhGetString(applicationFileName),
        parameters,
        NULL,
        ShowWindowType,
        Flags,
        Timeout,
        ProcessHandle
        );

    if (AppFlags & PH_SHELL_APP_PROPAGATE_PARAMETERS)
        PhDeleteStringBuilder(&sb);

    PhDereferenceObject(applicationFileName);

    return status;
}

BOOLEAN PhCreateProcessIgnoreIfeoDebugger(
    _In_ PCWSTR FileName,
    _In_opt_ PCWSTR CommandLine
    )
{
    BOOLEAN result;
    BOOLEAN originalValue;
    HANDLE processHandle;

    result = FALSE;

    RtlAcquirePebLock();
    originalValue = NtCurrentPeb()->ReadImageFileExecOptions;
    NtCurrentPeb()->ReadImageFileExecOptions = FALSE;
    RtlReleasePebLock();

    // The combination of ReadImageFileExecOptions = FALSE and the DEBUG_PROCESS flag
    // allows us to skip the Debugger IFEO value. (wj32)

    if (NT_SUCCESS(PhCreateProcessWin32(
        FileName,
        CommandLine,
        NULL,
        NULL,
        PH_CREATE_PROCESS_DEBUG | PH_CREATE_PROCESS_DEBUG_ONLY_THIS_PROCESS,
        NULL,
        &processHandle,
        NULL
        )))
    {
        HANDLE debugObjectHandle;

        if (NT_SUCCESS(PhGetProcessDebugObject(
            processHandle,
            &debugObjectHandle
            )))
        {
            // Disable kill-on-close.
            if (NT_SUCCESS(PhSetDebugKillProcessOnExit(
                debugObjectHandle,
                FALSE
                )))
            {
                // Stop debugging the process now.
                NtRemoveProcessDebug(processHandle, debugObjectHandle);
            }

            NtClose(debugObjectHandle);
        }

        // Ignore the debug object status.
        result = TRUE;

        PhConsoleSetForeground(processHandle, TRUE);
        NtClose(processHandle);
    }

    if (originalValue)
    {
        RtlAcquirePebLock();
        NtCurrentPeb()->ReadImageFileExecOptions = originalValue;
        RtlReleasePebLock();
    }

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
    PPH_EMENU_ITEM hideColumnMenuItem = NULL;
    PPH_EMENU_ITEM chooseColumnsMenuItem = NULL;
    ULONG minimumNumberOfColumns;

    Data->Menu = PhCreateEMenu();
    Data->Selection = NULL;
    Data->ProcessedId = 0;

    sizeColumnToFitMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_SIZE_COLUMN_TO_FIT_ID, L"Size column to fit", NULL, NULL);
    sizeAllColumnsToFitMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_SIZE_ALL_COLUMNS_TO_FIT_ID, L"Size all columns to fit", NULL, NULL);

    if (!(Flags & PH_TN_COLUMN_MENU_NO_VISIBILITY))
    {
        hideColumnMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_HIDE_COLUMN_ID, L"Hide column", NULL, NULL);
        chooseColumnsMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID, L"Choose columns...", NULL, NULL);
    }

    if (Flags & PH_TN_COLUMN_MENU_SHOW_RESET_SORT)
    {
        ULONG sortColumn = 0;
        PH_SORT_ORDER sortOrder = NoSortOrder;

        TreeNew_GetSort(Data->TreeNewHandle, &sortColumn, &sortOrder);

        if (sortOrder != Data->DefaultSortOrder || (Data->DefaultSortOrder != NoSortOrder && sortColumn != Data->DefaultSortColumn))
            resetSortMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_RESET_SORT_ID, L"Reset sort", NULL, NULL);
    }

    PhInsertEMenuItem(Data->Menu, sizeColumnToFitMenuItem, ULONG_MAX);
    PhInsertEMenuItem(Data->Menu, sizeAllColumnsToFitMenuItem, ULONG_MAX);

    if (!(Flags & PH_TN_COLUMN_MENU_NO_VISIBILITY))
    {
        if (hideColumnMenuItem) PhInsertEMenuItem(Data->Menu, hideColumnMenuItem, ULONG_MAX);
        if (resetSortMenuItem) PhInsertEMenuItem(Data->Menu, resetSortMenuItem, ULONG_MAX);
        PhInsertEMenuItem(Data->Menu, PhCreateEMenuSeparator(), ULONG_MAX);
        if (chooseColumnsMenuItem) PhInsertEMenuItem(Data->Menu, chooseColumnsMenuItem, ULONG_MAX);

        if (TreeNew_GetFixedColumn(Data->TreeNewHandle))
            minimumNumberOfColumns = 2; // don't allow user to remove all normal columns (the fixed column can never be removed)
        else
            minimumNumberOfColumns = 1;

        if (!Data->MouseEvent || !Data->MouseEvent->Column ||
            Data->MouseEvent->Column->Fixed || // don't allow the fixed column to be hidden
            TreeNew_GetVisibleColumnCount(Data->TreeNewHandle) < minimumNumberOfColumns + 1
            )
        {
            if (hideColumnMenuItem)
                PhSetDisabledEMenuItem(hideColumnMenuItem);
        }
    }
    else
    {
        if (resetSortMenuItem)
            PhInsertEMenuItem(Data->Menu, resetSortMenuItem, ULONG_MAX);
    }

    if (!Data->MouseEvent || !Data->MouseEvent->Column)
    {
        PhSetDisabledEMenuItem(sizeColumnToFitMenuItem);
    }
}

VOID PhpEnsureValidSortColumnTreeNew(
    _Inout_ HWND TreeNewHandle,
    _In_ ULONG DefaultSortColumn,
    _In_ PH_SORT_ORDER DefaultSortOrder
    )
{
    ULONG sortColumn = 0;
    PH_SORT_ORDER sortOrder = NoSortOrder;

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
    if (!Support->FilterList)
        return;

    PhDereferenceObject(Support->FilterList);
    Support->FilterList = NULL;
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

    if (index != ULONG_MAX)
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

    if (Support->NodeList->Count)
    {
        TreeNew_NodesStructured(Support->TreeNewHandle);
    }
}

VOID NTAPI PhpCopyCellEMenuItemDeleteFunction(
    _In_ PPH_EMENU_ITEM Item
    )
{
    PPH_COPY_CELL_CONTEXT context;

    context = Item->Context;
    PhDereferenceObject(context->MenuItemText);
    PhFree(context);
}

BOOLEAN PhInsertCopyCellEMenuItem(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG InsertAfterId,
    _In_ HWND TreeNewHandle,
    _In_ PPH_TREENEW_COLUMN Column
    )
{
    PPH_EMENU_ITEM parentItem = NULL;
    ULONG indexInParent = 0;
    PPH_COPY_CELL_CONTEXT context;
    PH_STRINGREF columnText;
    PPH_STRING escapedText;
    PPH_STRING menuItemText;
    PPH_EMENU_ITEM copyCellItem;
    PH_FORMAT format[3];

    if (!Column)
        return FALSE;

    if (!PhFindEMenuItemEx(Menu, 0, NULL, InsertAfterId, &parentItem, &indexInParent))
        return FALSE;

    indexInParent++;

    PhInitializeStringRefLongHint(&columnText, Column->Text);
    escapedText = PhEscapeStringForMenuPrefix(&columnText);
    PhInitFormatS(&format[0], L"Copy \""); // Copy \"%s\"
    PhInitFormatSR(&format[1], escapedText->sr);
    PhInitFormatS(&format[2], L"\"");
    menuItemText = PhFormat(format, RTL_NUMBER_OF(format), 0);
    PhDereferenceObject(escapedText);

    context = PhAllocate(sizeof(PH_COPY_CELL_CONTEXT));
    context->TreeNewHandle = TreeNewHandle;
    context->Id = Column->Id;
    context->MenuItemText = menuItemText;

    copyCellItem = PhCreateEMenuItem(0, ID_COPY_CELL, menuItemText->Buffer, NULL, context);
    copyCellItem->DeleteFunction = PhpCopyCellEMenuItemDeleteFunction;

    if (Column->CustomDraw)
        copyCellItem->Flags |= PH_EMENU_DISABLED;

    PhInsertEMenuItem(parentItem, copyCellItem, indexInParent);

    return TRUE;
}

BOOLEAN PhHandleCopyCellEMenuItem(
    _In_ PPH_EMENU_ITEM SelectedItem
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

_Function_class_(PH_EMENU_ITEM_DELETE_FUNCTION)
VOID NTAPI PhpCopyListViewEMenuItemDeleteFunction(
    _In_ PPH_EMENU_ITEM Item
    )
{
    PPH_COPY_ITEM_CONTEXT context;

    context = Item->Context;
    PhDereferenceObject(context->MenuItemText);
    PhFree(context);
}

BOOLEAN PhInsertCopyListViewEMenuItem(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG InsertAfterId,
    _In_ HWND ListViewHandle
    )
{
    PPH_EMENU_ITEM parentItem = NULL;
    ULONG indexInParent = 0;
    PPH_COPY_ITEM_CONTEXT context;
    PH_STRINGREF columnText;
    PPH_STRING escapedText;
    PPH_STRING menuItemText;
    PPH_EMENU_ITEM copyMenuItem;
    POINT location;
    LVHITTESTINFO lvHitInfo;
    HDITEM headerItem;
    HWND headerHandle;
    PH_FORMAT format[3];
    WCHAR headerText[MAX_PATH] = L"";

    if (!PhGetClientPos(ListViewHandle, &location))
        return FALSE;

    memset(&lvHitInfo, 0, sizeof(LVHITTESTINFO));
    lvHitInfo.pt = location;

    if (ListView_SubItemHitTest(ListViewHandle, &lvHitInfo) == INT_ERROR)
        return FALSE;

    memset(&headerItem, 0, sizeof(HDITEM));
    headerItem.mask = HDI_TEXT;
    headerItem.cchTextMax = RTL_NUMBER_OF(headerText);
    headerItem.pszText = headerText;
    headerHandle = ListView_GetHeader(ListViewHandle);

    if (!Header_GetItem(headerHandle, lvHitInfo.iSubItem, &headerItem))
        return FALSE;

    PhInitializeStringRefLongHint(&columnText, headerText);

    if (PhIsNullOrEmptyStringRef(&columnText))
        return FALSE;

    if (!PhFindEMenuItemEx(Menu, 0, NULL, InsertAfterId, &parentItem, &indexInParent))
        return FALSE;

    indexInParent++;

    escapedText = PhEscapeStringForMenuPrefix(&columnText);
    PhInitFormatS(&format[0], L"Copy \""); // Copy \"%s\"
    PhInitFormatSR(&format[1], escapedText->sr);
    PhInitFormatS(&format[2], L"\"");
    menuItemText = PhFormat(format, RTL_NUMBER_OF(format), 0);
    PhDereferenceObject(escapedText);

    context = PhAllocate(sizeof(PH_COPY_ITEM_CONTEXT));
    context->ListViewHandle = ListViewHandle;
    context->ListViewClass = NULL;
    context->Id = lvHitInfo.iItem;
    context->SubId = lvHitInfo.iSubItem;
    context->MenuItemText = menuItemText;

    copyMenuItem = PhCreateEMenuItem(0, ID_COPY_CELL, menuItemText->Buffer, NULL, context);
    copyMenuItem->DeleteFunction = PhpCopyListViewEMenuItemDeleteFunction;

    PhInsertEMenuItem(parentItem, copyMenuItem, indexInParent);

    return TRUE;
}

BOOLEAN PhInsertCopyIListViewEMenuItem(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG InsertAfterId,
    _In_ HWND ListViewHandle,
    _In_ IListView* ListView
    )
{
    PPH_EMENU_ITEM parentItem = NULL;
    ULONG indexInParent = 0;
    PPH_COPY_ITEM_CONTEXT context;
    PH_STRINGREF columnText;
    PPH_STRING escapedText;
    PPH_STRING menuItemText;
    PPH_EMENU_ITEM copyMenuItem;
    POINT location;
    LVHITTESTINFO lvHitInfo;
    HDITEM headerItem;
    HWND headerHandle;
    PH_FORMAT format[3];
    WCHAR headerText[MAX_PATH] = L"";

    if (!PhGetClientPos(ListViewHandle, &location))
        return FALSE;

    memset(&lvHitInfo, 0, sizeof(LVHITTESTINFO));
    lvHitInfo.pt = location;

    if (IListView_HitTestSubItem(ListView, &lvHitInfo) != S_OK)
        return FALSE;
    if (IListView_GetHeaderControl(ListView, &headerHandle) != S_OK)
        return FALSE;

    memset(&headerItem, 0, sizeof(HDITEM));
    headerItem.mask = HDI_TEXT;
    headerItem.cchTextMax = RTL_NUMBER_OF(headerText);
    headerItem.pszText = headerText;

    if (!Header_GetItem(headerHandle, lvHitInfo.iSubItem, &headerItem))
        return FALSE;

    PhInitializeStringRefLongHint(&columnText, headerText);

    if (PhIsNullOrEmptyStringRef(&columnText))
        return FALSE;

    if (!PhFindEMenuItemEx(Menu, 0, NULL, InsertAfterId, &parentItem, &indexInParent))
        return FALSE;

    indexInParent++;

    escapedText = PhEscapeStringForMenuPrefix(&columnText);
    PhInitFormatS(&format[0], L"Copy \""); // Copy \"%s\"
    PhInitFormatSR(&format[1], escapedText->sr);
    PhInitFormatS(&format[2], L"\"");
    menuItemText = PhFormat(format, RTL_NUMBER_OF(format), 0);
    PhDereferenceObject(escapedText);

    context = PhAllocate(sizeof(PH_COPY_ITEM_CONTEXT));
    context->ListViewHandle = ListViewHandle;
    context->ListViewClass = ListView;
    context->Id = lvHitInfo.iItem;
    context->SubId = lvHitInfo.iSubItem;
    context->MenuItemText = menuItemText;

    copyMenuItem = PhCreateEMenuItem(0, ID_COPY_CELL, menuItemText->Buffer, NULL, context);
    copyMenuItem->DeleteFunction = PhpCopyListViewEMenuItemDeleteFunction;

    PhInsertEMenuItem(parentItem, copyMenuItem, indexInParent);

    return TRUE;
}

BOOLEAN PhHandleCopyListViewEMenuItem(
    _In_ PPH_EMENU_ITEM SelectedItem
    )
{
    PPH_COPY_ITEM_CONTEXT context;
    PH_STRING_BUILDER stringBuilder;
    ULONG state = 0;
    ULONG count = 0;
    ULONG selectedCount;
    ULONG i;
    PPH_STRING getItemText;

    if (!SelectedItem)
        return FALSE;
    if (SelectedItem->Id != ID_COPY_CELL)
        return FALSE;

    context = SelectedItem->Context;

    PhInitializeStringBuilder(&stringBuilder, 0x100);

    if (context->ListViewClass)
        IListView_GetItemCount(context->ListViewClass, &count);
    else
        count = ListView_GetItemCount(context->ListViewHandle);

    selectedCount = 0;

    for (i = 0; i < count; i++)
    {
        if (context->ListViewClass)
            IListView_GetItemState(context->ListViewClass, i, 0, LVIS_SELECTED, &state);
        else
            state = ListView_GetItemState(context->ListViewHandle, i, LVIS_SELECTED);

        if (!FlagOn(state, LVIS_SELECTED))
            continue;

        if (context->ListViewClass)
            getItemText = PhGetIListViewItemText(context->ListViewClass, i, context->SubId);
        else
            getItemText = PhaGetListViewItemText(context->ListViewHandle, i, context->SubId);

        PhAppendStringBuilder(&stringBuilder, &getItemText->sr);
        PhAppendStringBuilder2(&stringBuilder, L"\r\n");

        selectedCount++;
    }

    if (stringBuilder.String->Length != 0 && selectedCount == 1)
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhSetClipboardString(context->ListViewHandle, &stringBuilder.String->sr);
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
    ULONG id = ULONG_MAX;

    if (!(menu = GetMenu(RegeditWindow)))
        return FALSE;

    // Cause the Registry Editor to refresh the Favorites menu.
    if (UsePhSvc)
        PhSvcCallSendMessage(RegeditWindow, WM_MENUSELECT, MAKEWPARAM(3, MF_POPUP), (LPARAM)menu);
    else
        PhSendMessageTimeout(RegeditWindow, WM_MENUSELECT, MAKEWPARAM(3, MF_POPUP), (LPARAM)menu, 5000, NULL);

    if (!(favoritesMenu = GetSubMenu(menu, 3)))
        return FALSE;

    // Find our entry.

    count = GetMenuItemCount(favoritesMenu);

    if (count == ULONG_MAX)
        return FALSE;
    if (count > 1000)
        count = 1000;

    for (i = 0; i < count; i++)
    {
        MENUITEMINFO info;
        WCHAR buffer[MAX_PATH];

        memset(&info, 0, sizeof(MENUITEMINFO));
        info.cbSize = sizeof(MENUITEMINFO);
        info.fMask = MIIM_ID | MIIM_STRING;
        info.dwTypeData = buffer;
        info.cch = RTL_NUMBER_OF(buffer);

        if (!GetMenuItemInfo(favoritesMenu, i, TRUE, &info))
            continue;

        if (info.cch == FavoriteName->Length / sizeof(WCHAR))
        {
            PH_STRINGREF text;

            text.Buffer = buffer;
            text.Length = info.cch * sizeof(WCHAR);

            if (PhEqualStringRef(&text, FavoriteName, TRUE))
            {
                id = info.wID;
                break;
            }
        }
    }

    if (id == ULONG_MAX)
        return FALSE;

    // Activate our entry.
    if (UsePhSvc)
        PhSvcCallSendMessage(RegeditWindow, WM_COMMAND, MAKEWPARAM(id, 0), 0);
    else
        PhSendMessageTimeout(RegeditWindow, WM_COMMAND, MAKEWPARAM(id, 0), 0, 5000, NULL);

    // "Close" the Favorites menu and restore normal status bar text.
    if (UsePhSvc)
        PhSvcCallPostMessage(RegeditWindow, WM_MENUSELECT, MAKEWPARAM(0, 0xffff), 0);
    else
        PostMessage(RegeditWindow, WM_MENUSELECT, MAKEWPARAM(0, 0xffff), 0);

    // Bring regedit to the top.
    if (IsMinimized(RegeditWindow))
    {
        ShowWindow(RegeditWindow, SW_RESTORE);
    }

    SetForegroundWindow(RegeditWindow);

    return TRUE;
}

/**
 * Opens a key in the Registry Editor.
 *
 * \param WindowHandle A handle to the parent window.
 * \param KeyName The key name to open.
 */
VOID PhShellOpenKey(
    _In_ HWND WindowHandle,
    _In_ PPH_STRING KeyName
    )
{
    static CONST PH_STRINGREF regeditKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit");
    NTSTATUS status;
    HANDLE regeditKeyHandle;
    PPH_STRING lastKey;
    PPH_STRING regeditFileName;
    PH_STRINGREF systemRootString;

    status = PhCreateKey(
        &regeditKeyHandle,
        KEY_WRITE,
        PH_KEY_CURRENT_USER,
        &regeditKeyName,
        0,
        0,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(WindowHandle, L"Unable to execute the program.", status, 0);
        return;
    }

    lastKey = PhExpandKeyName(KeyName, FALSE);
    PhSetValueKeyZ(regeditKeyHandle, L"LastKey", REG_SZ, lastKey->Buffer, (ULONG)lastKey->Length + sizeof(UNICODE_NULL));
    NtClose(regeditKeyHandle);
    PhDereferenceObject(lastKey);

    // Start regedit. If we aren't elevated, request that regedit be elevated. This is so we can get
    // the consent dialog in the center of the specified window. (wj32)

    PhGetSystemRoot(&systemRootString);
    regeditFileName = PhConcatStringRefZ(&systemRootString, L"\\regedit.exe");

    if (PhGetOwnTokenAttributes().Elevated)
    {
        status = PhShellExecuteEx(
            WindowHandle,
            regeditFileName->Buffer,
            NULL,
            NULL,
            SW_SHOW,
            0,
            0,
            NULL
            );

        if (!NT_SUCCESS(status))
        {
            PhShowStatus(WindowHandle, L"Unable to execute the program.", status, 0);
        }
    }
    else
    {
        status = PhShellExecuteEx(
            WindowHandle,
            regeditFileName->Buffer,
            NULL,
            NULL,
            SW_SHOW,
            PH_SHELL_EXECUTE_ADMIN,
            0,
            NULL
            );

        if (!NT_SUCCESS(status))
        {
            PhShowStatus(WindowHandle, L"Unable to execute the program.", status, 0);
        }
    }

    PhDereferenceObject(regeditFileName);
}

NTSTATUS PhRegeditOpenUserFavoritesKey(
    _In_ HWND RegeditWindow,
    _Out_ PHANDLE KeyHandle
    )
{
    static CONST PH_STRINGREF favoritesKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit\\Favorites");
    NTSTATUS status;
    CLIENT_ID clientId;
    HANDLE processHandle;
    HANDLE tokenHandle;
    HANDLE keyUserHandle;
    HANDLE keyFavoritesHandle;
    PH_TOKEN_USER tokenUser;
    PPH_STRING tokenUserSid;

    status = PhGetWindowClientId(RegeditWindow, &clientId);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhOpenProcessClientId(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION,
        &clientId
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhOpenProcessToken(
        processHandle,
        TOKEN_QUERY,
        &tokenHandle
        );

    NtClose(processHandle);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetTokenUser(
        tokenHandle,
        &tokenUser
        );

    NtClose(tokenHandle);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!(tokenUserSid = PhSidToStringSid(tokenUser.User.Sid)))
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    status = PhOpenKey(
        &keyUserHandle,
        KEY_READ,
        PH_KEY_USERS,
        &tokenUserSid->sr,
        0
        );

    PhDereferenceObject(tokenUserSid);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateKey(
        &keyFavoritesHandle,
        KEY_WRITE,
        keyUserHandle,
        &favoritesKeyName,
        0,
        0,
        NULL
        );

    NtClose(keyUserHandle);

    if (NT_SUCCESS(status))
    {
        *KeyHandle = keyFavoritesHandle;
        return STATUS_SUCCESS;
    }

CleanupExit:
    status = PhCreateKey(
        &keyFavoritesHandle,
        KEY_WRITE,
        PH_KEY_CURRENT_USER,
        &favoritesKeyName,
        0,
        0,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *KeyHandle = keyFavoritesHandle;
        return STATUS_SUCCESS;
    }

    return status;
}

/**
 * Opens a key in the Registry Editor. If the Registry Editor is already open,
 * the specified key is selected in the Registry Editor.
 *
 * \param WindowHandle A handle to the parent window.
 * \param KeyName The key name to open.
 */
BOOLEAN PhShellOpenKey2(
    _In_ HWND WindowHandle,
    _In_ PPH_STRING KeyName
    )
{
    BOOLEAN result = FALSE;
    HWND regeditWindow;
    HANDLE favoritesKeyHandle;
    WCHAR favoriteName[33];
    PH_STRINGREF valueName;
    PPH_STRING expandedKeyName;

    regeditWindow = FindWindow(L"RegEdit_RegEdit", NULL);

    if (!regeditWindow)
    {
        PhShellOpenKey(WindowHandle, KeyName);
        return TRUE;
    }

    if (!PhGetOwnTokenAttributes().Elevated)
    {
        if (!PhUiConnectToPhSvc(WindowHandle, FALSE))
            return FALSE;
    }

    // Create our entry in Favorites.

    if (!NT_SUCCESS(PhRegeditOpenUserFavoritesKey(regeditWindow, &favoritesKeyHandle)))
        goto CleanupExit;

    memcpy(favoriteName, L"A_SystemInformer", 16 * sizeof(WCHAR));
    PhGenerateRandomAlphaString(&favoriteName[16], ARRAYSIZE(favoriteName) - 16);
    valueName.Buffer = favoriteName;
    valueName.Length = sizeof(favoriteName) - sizeof(UNICODE_NULL);

    expandedKeyName = PhExpandKeyName(KeyName, FALSE);
    PhSetValueKey(favoritesKeyHandle, &valueName, REG_SZ, expandedKeyName->Buffer, (ULONG)expandedKeyName->Length + sizeof(UNICODE_NULL));
    PhDereferenceObject(expandedKeyName);

    // Select our entry in regedit.
    result = PhpSelectFavoriteInRegedit(regeditWindow, &valueName, !PhGetOwnTokenAttributes().Elevated);

    PhDeleteValueKey(favoritesKeyHandle, &valueName);
    NtClose(favoritesKeyHandle);

CleanupExit:
    if (!PhGetOwnTokenAttributes().Elevated)
        PhUiDisconnectFromPhSvc();

    return result;
}

PPH_STRING PhPcre2GetErrorMessage(
    _In_ LONG ErrorCode
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

HBITMAP PhGetShieldBitmap(
    _In_ LONG WindowDpi,
    _In_opt_ LONG Width,
    _In_opt_ LONG Height
    )
{
    HICON shieldIcon;
    HBITMAP shieldBitmap = NULL;

    shieldIcon = PhLoadIcon(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDI_UACSHIELD),
        0,
        Width,
        Height,
        WindowDpi
        );

    if (!shieldIcon)
    {
        shieldIcon = PhLoadIcon(
            NULL,
            IDI_SHIELD,
            0,
            Width,
            Height,
            WindowDpi
            );
    }

    if (shieldIcon)
    {
        shieldBitmap = PhIconToBitmap(
            shieldIcon,
            Width,
            Height
            );
        DestroyIcon(shieldIcon);
    }

    return shieldBitmap;
}

HICON PhGetApplicationIcon(
    _In_ BOOLEAN SmallIcon
    )
{
    static HICON smallIcon = NULL;
    static HICON largeIcon = NULL;
    static LONG systemDpi = 0;

    if (systemDpi != PhSystemDpi)
    {
        if (smallIcon)
        {
            DestroyIcon(smallIcon);
            smallIcon = NULL;
        }
        if (largeIcon)
        {
            DestroyIcon(largeIcon);
            largeIcon = NULL;
        }

        systemDpi = PhSystemDpi;
    }

    if (!smallIcon || !largeIcon)
    {
        if (!smallIcon)
            smallIcon = PhLoadIcon(NtCurrentImageBase(), MAKEINTRESOURCE(IDI_PROCESSHACKER), PH_LOAD_ICON_SIZE_SMALL, 0, 0, systemDpi);
        if (!largeIcon)
            largeIcon = PhLoadIcon(NtCurrentImageBase(), MAKEINTRESOURCE(IDI_PROCESSHACKER), PH_LOAD_ICON_SIZE_LARGE, 0, 0, systemDpi);
    }

    return SmallIcon ? smallIcon : largeIcon;
}

HICON PhGetApplicationIconEx(
    _In_ BOOLEAN SmallIcon,
    _In_opt_ LONG WindowDpi
    )
{
    if (SmallIcon)
        return PhLoadIcon(NtCurrentImageBase(), MAKEINTRESOURCE(IDI_PROCESSHACKER), PH_LOAD_ICON_SIZE_SMALL, 0, 0, WindowDpi);
    return PhLoadIcon(NtCurrentImageBase(), MAKEINTRESOURCE(IDI_PROCESSHACKER), PH_LOAD_ICON_SIZE_LARGE, 0, 0, WindowDpi);
}

VOID PhSetWindowIcon(
    _In_ HWND WindowHandle,
    _In_opt_ HICON SmallIcon,
    _In_opt_ HICON LargeIcon,
    _In_ BOOLEAN CleanupIcon
    )
{
    if (SmallIcon)
    {
        HICON iconHandle = (HICON)SendMessage(WindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)SmallIcon);

        if (iconHandle && CleanupIcon)
        {
            DestroyIcon(iconHandle);
        }
    }

    if (LargeIcon)
    {
        HICON iconHandle = (HICON)SendMessage(WindowHandle, WM_SETICON, ICON_BIG, (LPARAM)LargeIcon);

        if (iconHandle && CleanupIcon)
        {
            DestroyIcon(iconHandle);
        }
    }
}

VOID PhDestroyWindowIcon(
    _In_ HWND WindowHandle
    )
{
    HICON iconHandle;

    if (iconHandle = (HICON)SendMessage(WindowHandle, WM_SETICON, ICON_SMALL, 0))
    {
        DestroyIcon(iconHandle);
    }

    if (iconHandle = (HICON)SendMessage(WindowHandle, WM_SETICON, ICON_BIG, 0))
    {
        DestroyIcon(iconHandle);
    }
}

VOID PhSetApplicationWindowIcon(
    _In_ HWND WindowHandle
    )
{
    PhSetWindowIcon(
        WindowHandle,
        PhGetApplicationIcon(TRUE),
        PhGetApplicationIcon(FALSE),
        TRUE
        );
}

VOID PhSetApplicationWindowIconEx(
    _In_ HWND WindowHandle,
    _In_opt_ LONG WindowDpi
    )
{
    PhSetWindowIcon(
        WindowHandle,
        PhGetApplicationIconEx(TRUE, WindowDpi),
        PhGetApplicationIconEx(FALSE, WindowDpi),
        TRUE
        );
}

VOID PhSetStaticWindowIcon(
    _In_ HWND WindowHandle,
    _In_opt_ LONG WindowDpi
    )
{
    HICON largeIcon;
    HICON destroyIcon;

    if (largeIcon = PhGetApplicationIconEx(FALSE, WindowDpi))
    {
        if (destroyIcon = Static_SetIcon(WindowHandle, largeIcon))
        {
            DestroyIcon(destroyIcon);
        }
    }
}

VOID PhDeleteStaticWindowIcon(
    _In_ HWND WindowHandle
    )
{
    HICON destroyIcon;

    if (destroyIcon = Static_GetIcon(WindowHandle, 0))
    {
        DestroyIcon(destroyIcon);
    }
}

BOOLEAN PhWordMatchStringRef(
    _In_ PPH_STRINGREF SearchText,
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = *SearchText;

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, L'|', &part, &remainingPart);

        if (part.Length)
        {
            if (PhFindStringInStringRef(Text, &part, TRUE) != SIZE_MAX)
                return TRUE;
        }
    }

    return FALSE;
}

//BOOLEAN PhIsSystemRebootRequired(
//    VOID
//    )
//{
//    static PH_STRINGREF keyRootPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate");
//    static PH_STRINGREF keyAuPath = PH_STRINGREF_INIT(L"Auto Update\\RebootRequired");
//    static PH_STRINGREF keyOrcPath = PH_STRINGREF_INIT(L"Orchestrator\\RebootRequired");
//    BOOLEAN keyRebootRequired = FALSE;
//    HANDLE keyRootHandle;
//    HANDLE keyHandle;
//
//    if (NT_SUCCESS(PhOpenKey(
//        &keyRootHandle,
//        KEY_READ,
//        PH_KEY_LOCAL_MACHINE,
//        &keyRootPath,
//        0
//        )))
//    {
//        if (NT_SUCCESS(PhOpenKey(
//            &keyHandle,
//            KEY_READ,
//            keyRootHandle,
//            &keyAuPath,
//            0
//            )))
//        {
//            keyRebootRequired = TRUE;
//            NtClose(keyHandle);
//        }
//
//        if (NT_SUCCESS(PhOpenKey(
//            &keyHandle,
//            KEY_READ,
//            keyRootHandle,
//            &keyOrcPath,
//            0
//            )))
//        {
//            keyRebootRequired = TRUE;
//            NtClose(keyHandle);
//        }
//
//        NtClose(keyRootHandle);
//    }
//
//    //#include <wuapi.h>
//    //ISystemInformation* systemInformation = NULL;
//    //
//    //if (SUCCEEDED(PhGetClassObject(
//    //    L"wuapi.dll",
//    //    &CLSID_SystemInformation,
//    //    &IID_ISystemInformation,
//    //    &systemInformation
//    //    )))
//    //{
//    //    VARIANT_BOOL rebootRequiredVariant = VARIANT_FALSE;
//    //
//    //    ISystemInformation_get_RebootRequired(systemInformation, &rebootRequiredVariant);
//    //    ISystemInformation_Release(systemInformation);
//    //
//    //    keyRebootRequired = rebootRequiredVariant == VARIANT_TRUE;
//    //}
//
//    return keyRebootRequired;
//}
