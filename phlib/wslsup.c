/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2019-2024
 *
 */

#include <ph.h>
#include <wslsup.h>
#include <mapldr.h>

typedef struct _PH_WSL_ENUM_CONTEXT
{
    BOOLEAN Found;
    PPH_STRINGREF FileName;
    PPH_STRING DistributionName;
    PPH_STRING DistributionPath;
    PPH_STRING TranslatedPath;
} PH_WSL_ENUM_CONTEXT, *PPH_WSL_ENUM_CONTEXT;

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI PhWslDistributionNamesCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_ PVOID Context
    )
{
    PKEY_BASIC_INFORMATION information = (PKEY_BASIC_INFORMATION)Information;
    PPH_WSL_ENUM_CONTEXT context = (PPH_WSL_ENUM_CONTEXT)Context;
    HANDLE keyHandle;
    PH_STRINGREF keyName;

    keyName.Buffer = information->Name;
    keyName.Length = information->NameLength;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        RootDirectory,
        &keyName,
        0
        )))
    {
        PPH_STRING lxssBasePathName;

        if (lxssBasePathName = PhQueryRegistryStringZ(keyHandle, L"BasePath"))
        {
            PhMoveReference(&lxssBasePathName, PhDosPathNameToNtPathName(&lxssBasePathName->sr));

            if (lxssBasePathName && PhStartsWithStringRef(context->FileName, &lxssBasePathName->sr, TRUE))
            {
                PPH_STRING lxssDistributionName;

                if (lxssDistributionName = PhQueryRegistryStringZ(keyHandle, L"DistributionName"))
                {
                    context->Found = TRUE;
                    context->DistributionName = lxssDistributionName;
                    context->DistributionPath = lxssBasePathName;

                    if (context->TranslatedPath = PhCreateString2(context->FileName))
                    {
                        PhSkipStringRef(&context->TranslatedPath->sr, lxssBasePathName->Length);
                        PhSkipStringRef(&context->TranslatedPath->sr, sizeof(L"rootfs"));
                    }

                    NtClose(keyHandle);
                    return FALSE;
                }
            }

            PhClearReference(&lxssBasePathName);
        }

        NtClose(keyHandle);
    }

    return TRUE;
}

NTSTATUS PhGetWslDistributionFromPath(
    _In_ PPH_STRINGREF FileName,
    _Out_opt_ PPH_STRING *DistributionName,
    _Out_opt_ PPH_STRING *DistributionPath,
    _Out_opt_ PPH_STRING *TranslatedPath
    )
{
    static CONST PH_STRINGREF lxssKeyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Lxss");
    PH_WSL_ENUM_CONTEXT enumContext;
    NTSTATUS status;
    HANDLE keyHandle;

    memset(&enumContext, 0, sizeof(PH_WSL_ENUM_CONTEXT));
    enumContext.Found = FALSE;
    enumContext.FileName = FileName;

    status = PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &lxssKeyPath,
        0
        );

    if (NT_SUCCESS(status))
    {
        status = PhEnumerateKey(
            keyHandle,
            KeyBasicInformation,
            PhWslDistributionNamesCallback,
            &enumContext
            );

        NtClose(keyHandle);
    }

    if (NT_SUCCESS(status))
    {
        if (enumContext.Found)
        {
            if (DistributionName)
                *DistributionName = enumContext.DistributionName;
            else
                PhClearReference(&enumContext.DistributionName);

            if (DistributionPath)
                *DistributionPath = enumContext.DistributionPath;
            else
                PhClearReference(&enumContext.DistributionPath);

            if (TranslatedPath)
                *TranslatedPath = PhConvertNtPathSeperatorToAltSeperator(enumContext.TranslatedPath);
            else
                PhClearReference(&enumContext.TranslatedPath);
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}

PPH_STRING PhGetWslDistributionCommandLine(
    _In_ PPH_STRING DistributionName,
    _In_ PPH_STRING CommandLine
    )
{
    PH_FORMAT format[4];
    PPH_STRING systemDirectory;
    PPH_STRING commandLine;

    // "wsl.exe -u root -d %s -e %s"
    PhInitFormatS(&format[0], L"wsl.exe -u root -d ");
    PhInitFormatSR(&format[1], DistributionName->sr);
    PhInitFormatS(&format[2], L" -e ");
    PhInitFormatSR(&format[3], CommandLine->sr);

    if (commandLine = PhFormat(format, RTL_NUMBER_OF(format), 0))
    {
        if (systemDirectory = PhGetSystemDirectory())
        {
            PhMoveReference(&commandLine, PhConcatStringRef3(
                &systemDirectory->sr,
                &PhNtPathSeparatorString,
                &commandLine->sr
                ));
            PhDereferenceObject(systemDirectory);
        }

        return commandLine;
    }

    return NULL;
}

PPH_STRING PhGetWslPathCommandLine(
    _In_ PPH_STRING win32FileName
    )
{
    PH_FORMAT format[3];

    PhInitFormatS(&format[0], L"wslpath -a -u \"");
    PhInitFormatSR(&format[1], win32FileName->sr);
    PhInitFormatC(&format[2], L'\"');

    return PhFormat(format, RTL_NUMBER_OF(format), 0x100);
}

NTSTATUS PhDosPathNameToWslPathName(
    _In_ PPH_STRINGREF FileName,
    _Out_ PPH_STRING* LxssFileName
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PPH_STRING win32FileName = NULL;
    PPH_STRING distributionName = NULL;
    PPH_STRING lxssCommandLine = NULL;
    PPH_STRING lxssCommandResult = NULL;

    win32FileName = PhCreateString2(FileName);
    PhMoveReference(&win32FileName, PhGetFileName(win32FileName));

    if (PhIsNullOrEmptyString(win32FileName))
        goto CleanupExit;

    lxssCommandLine = PhGetWslPathCommandLine(win32FileName);

    if (PhIsNullOrEmptyString(win32FileName))
        goto CleanupExit;

    status = PhGetWslDistributionFromPath(
        FileName,
        &distributionName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateProcessLxss(
        distributionName,
        lxssCommandLine,
        &lxssCommandResult
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (PhEndsWithString2(lxssCommandResult, L"\n", FALSE))
    {
        lxssCommandResult->Length -= sizeof(WCHAR[1]);
        lxssCommandResult->Buffer[lxssCommandResult->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    *LxssFileName = PhReferenceObject(lxssCommandResult);
    status = STATUS_SUCCESS;

CleanupExit:

    PhClearReference(&lxssCommandResult);
    PhClearReference(&lxssCommandLine);
    PhClearReference(&distributionName);
    PhClearReference(&win32FileName);

    return status;
}

NTSTATUS PhWslQueryDistroProcessCommandLine(
    _In_ PPH_STRINGREF FileName,
    _In_ ULONG LxssProcessId,
    _Out_ PPH_STRING* Result
    )
{
    NTSTATUS status;
    PPH_STRING distributionCommand = NULL;
    PPH_STRING distributionResult = NULL;
    PPH_STRING distributionName = NULL;
    PH_FORMAT format[5];

    status = PhGetWslDistributionFromPath(
        FileName,
        &distributionName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    PhInitFormatS(&format[0], L"\\Device\\Mup\\wsl.localhost\\");
    PhInitFormatSR(&format[1], distributionName->sr);
    PhInitFormatS(&format[2], L"\\proc\\");
    PhInitFormatIU(&format[3], LxssProcessId);
    PhInitFormatS(&format[4], L"\\cmdline");

    if (distributionCommand = PhFormat(format, RTL_NUMBER_OF(format), 0x100))
    {
        HANDLE fileHandle;

        status = PhCreateFile(
            &fileHandle,
            &distributionCommand->sr,
            FILE_GENERIC_READ,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (NT_SUCCESS(status))
        {
            status = PhGetFileText(
                &distributionResult,
                fileHandle,
                TRUE
                );
            NtClose(fileHandle);
        }

        if (status == STATUS_ACCESS_DENIED)
        {
            PPH_STRING commandLine;
            PPH_STRING commandResult;

            // Note: The WSL P9 Multiple UNC Provider (MUP) doesn't allow administrators to read /proc unless the distro wsl.conf default user is root.
            // Changing the default user to root fixes permission issues when accessing files from Windows but results in everything owned by root and inaccessible from WSL.
            // We can workaround the issue by creating a root process using 'wsl.exe -u root' and pipe the /proc content via standard output.
            // This is significantly slower (very slow) but works, and avoids hard requirements on user preferences and distro configuration.

            PhInitFormatS(&format[0], L"cat /proc/");
            PhInitFormatIU(&format[1], LxssProcessId);
            PhInitFormatS(&format[2], L"/cmdline");

            if (commandLine = PhFormat(format, 3, 0x100))
            {
                status = PhCreateProcessLxss(
                    distributionName,
                    commandLine,
                    &commandResult
                    );

                if (NT_SUCCESS(status))
                {
                    distributionResult = commandResult;
                }

                PhDereferenceObject(commandLine);
            }
        }

        PhDereferenceObject(distributionCommand);
    }

    PhDereferenceObject(distributionName);

    if (distributionResult)
    {
        *Result = distributionResult;
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS PhWslQueryDistroProcessEnvironment(
    _In_ PPH_STRINGREF FileName,
    _In_ ULONG LxssProcessId,
    _Out_ PPH_STRING* Result
    )
{
    NTSTATUS status;
    PPH_STRING distributionCommand = NULL;
    PPH_STRING distributionResult = NULL;
    PPH_STRING distributionName = NULL;
    PH_FORMAT format[5];

    status = PhGetWslDistributionFromPath(
        FileName,
        &distributionName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    PhInitFormatS(&format[0], L"\\Device\\Mup\\wsl.localhost\\");
    PhInitFormatSR(&format[1], distributionName->sr);
    PhInitFormatS(&format[2], L"\\proc\\");
    PhInitFormatIU(&format[3], LxssProcessId);
    PhInitFormatS(&format[4], L"\\environ");

    if (distributionCommand = PhFormat(format, RTL_NUMBER_OF(format), 0x100))
    {
        HANDLE fileHandle;

        status = PhCreateFile(
            &fileHandle,
            &distributionCommand->sr,
            FILE_GENERIC_READ,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (NT_SUCCESS(status))
        {
            status = PhGetFileText(
                &distributionResult,
                fileHandle,
                TRUE
                );
            NtClose(fileHandle);
        }

        if (status == STATUS_ACCESS_DENIED)
        {
            PPH_STRING commandLine;
            PPH_STRING commandResult;

            // Note: The WSL P9 Multiple UNC Provider (MUP) doesn't allow administrators to read /proc unless the distro wsl.conf default user is root.
            // Changing the default user to root fixes permission issues when accessing files from Windows but results in everything owned by root and inaccessible from WSL.
            // We can workaround the issue by creating a root process using 'wsl.exe -u root' and pipe the /proc content via standard output.
            // This is significantly slower (very slow) but works, and avoids hard requirements on user preferences and distro configuration.

            PhInitFormatS(&format[0], L"cat /proc/");
            PhInitFormatIU(&format[1], LxssProcessId);
            PhInitFormatS(&format[2], L"/environ");

            if (commandLine = PhFormat(format, 3, 0x100))
            {
                status = PhCreateProcessLxss(
                    distributionName,
                    commandLine,
                    &commandResult
                    );

                if (NT_SUCCESS(status))
                {
                    distributionResult = commandResult;
                }

                PhDereferenceObject(commandLine);
            }
        }

        PhDereferenceObject(distributionCommand);
    }

    PhDereferenceObject(distributionName);

    if (distributionResult)
    {
        *Result = distributionResult;
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS PhWslQueryRpmPackageFromFileName(
    _In_ PPH_STRING DistributionName,
    _In_ PPH_STRING LxssFileName,
    _Out_ PPH_STRING* Result
    )
{
    NTSTATUS status;
    PPH_STRING commandLine;
    PPH_STRING commandResult;
    PH_FORMAT format[3];

    // "rpm -qf %s --queryformat \"%%{VERSION}|%%{VENDOR}|%%{SUMMARY}\""
    PhInitFormatS(&format[0], L"rpm -qf \"");
    PhInitFormatSR(&format[1], LxssFileName->sr);
    PhInitFormatS(&format[2], L"\" --queryformat \"%%{VERSION}|%%{VENDOR}|%%{SUMMARY}\"");
    commandLine = PhFormat(format, RTL_NUMBER_OF(format), 0x100);

    status = PhCreateProcessLxss(
        DistributionName,
        commandLine,
        &commandResult
        );

    if (NT_SUCCESS(status))
    {
        *Result = commandResult;
    }

    PhDereferenceObject(commandLine);
    return status;
}

NTSTATUS PhWslQueryDebianPackageFromFileName(
    _In_ PPH_STRING LxssDistribution,
    _In_ PPH_STRING LxssFileName,
    _Out_ PPH_STRING* Result
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PPH_STRING commandLine;
    PPH_STRING lxssCommandResult = NULL;
    PPH_STRING lxssPackageName = NULL;
    PH_FORMAT format[3];

    // "dpkg -S %s"
    PhInitFormatS(&format[0], L"dpkg -S \"");
    PhInitFormatSR(&format[1], LxssFileName->sr);
    PhInitFormatS(&format[2], L"\"");
    commandLine = PhFormat(format, RTL_NUMBER_OF(format), 0x100);

    status = PhCreateProcessLxss(
        LxssDistribution,
        commandLine,
        &lxssCommandResult
        );

    if (!NT_SUCCESS(status))
    {
        // The dpkg metadata doesn't contain information for symbolic links
        // from "/usr/bin -> /bin" so remove the /usr prefix and try again. (dmex)

        if (PhStartsWithString2(LxssFileName, L"/usr", FALSE))
        {
            PhSkipStringRef(&LxssFileName->sr, sizeof(WCHAR[4]));

            // "dpkg -S %s"
            PhInitFormatS(&format[0], L"dpkg -S \"");
            PhInitFormatSR(&format[1], LxssFileName->sr);
            PhInitFormatS(&format[2], L"\"");
            PhMoveReference(&commandLine, PhFormat(format, RTL_NUMBER_OF(format), 0x100));

            status = PhCreateProcessLxss(
                LxssDistribution,
                commandLine,
                &lxssCommandResult
                );

            if (!NT_SUCCESS(status))
            {
                goto CleanupExit;
            }
        }
        else
        {
            goto CleanupExit;
        }
    }

    if (!PhIsNullOrEmptyString(lxssCommandResult))
    {
        PH_STRINGREF remainingPart;
        PH_STRINGREF packagePart;

        if (PhSplitStringRefAtChar(&lxssCommandResult->sr, L':', &packagePart, &remainingPart))
        {
            lxssPackageName = PhCreateString2(&packagePart);
        }

        PhClearReference(&lxssCommandResult);
    }

    if (PhIsNullOrEmptyString(lxssPackageName))
        goto CleanupExit;

    // "dpkg-query -W -f=${Version}|${Maintainer}|${binary:Summary} %s"
    PhInitFormatS(&format[0], L"dpkg-query -W -f=${Version}|${Maintainer}|${binary:Summary} \"");
    PhInitFormatSR(&format[1], lxssPackageName->sr);
    PhInitFormatS(&format[2], L"\"");
    PhMoveReference(&commandLine, PhFormat(format, RTL_NUMBER_OF(format), 0x100));

    status = PhCreateProcessLxss(
        LxssDistribution,
        commandLine,
        &lxssCommandResult
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    *Result = PhReferenceObject(lxssCommandResult);

CleanupExit:
    if (lxssPackageName) PhDereferenceObject(lxssPackageName);
    if (lxssCommandResult) PhDereferenceObject(lxssCommandResult);
    if (commandLine) PhDereferenceObject(commandLine);

    return status;
}

NTSTATUS PhWslParsePackageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRING LxssCommandResult
    )
{
    PH_STRINGREF remainingPart;
    PH_STRINGREF companyPart;
    PH_STRINGREF descriptionPart;
    PH_STRINGREF versionPart;

    if (PhIsNullOrEmptyString(LxssCommandResult))
        return STATUS_UNSUCCESSFUL;

    remainingPart = PhGetStringRef(LxssCommandResult);
    PhSplitStringRefAtChar(&remainingPart, L'|', &versionPart, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'|', &companyPart, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'|', &descriptionPart, &remainingPart);

    if (versionPart.Length != 0 && !ImageVersionInfo->FileVersion)
    {
        ImageVersionInfo->FileVersion = PhCreateString2(&versionPart);
    }

    if (companyPart.Length != 0 && !ImageVersionInfo->CompanyName)
    {
        ImageVersionInfo->CompanyName = PhCreateString2(&companyPart);
    }

    if (descriptionPart.Length != 0 && !ImageVersionInfo->FileDescription)
    {
        ImageVersionInfo->FileDescription = PhCreateString2(&descriptionPart);
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhInitializeLxssImageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRINGREF FileName
    )
{
    NTSTATUS success;
    PPH_STRING distributionCommand = NULL;
    PPH_STRING distributionName = NULL;
    PPH_STRING translatedPath = NULL;

    success = PhGetWslDistributionFromPath(
        FileName,
        &distributionName,
        NULL,
        &translatedPath
        );

    if (!NT_SUCCESS(success))
        return success;

    if (PhEqualString2(translatedPath, L"/init", FALSE))
    {
        success = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    success = PhWslQueryDebianPackageFromFileName(
        distributionName,
        translatedPath,
        &distributionCommand
        );

    if (NT_SUCCESS(success))
    {
        success = PhWslParsePackageVersionInfo(ImageVersionInfo, distributionCommand);

        if (NT_SUCCESS(success))
            goto CleanupExit;
    }

    success = PhWslQueryRpmPackageFromFileName(
        distributionName,
        translatedPath,
        &distributionCommand
        );

    if (NT_SUCCESS(success))
    {
        success = PhWslParsePackageVersionInfo(ImageVersionInfo, distributionCommand);

        if (NT_SUCCESS(success))
            goto CleanupExit;
    }

CleanupExit:
    PhClearReference(&distributionCommand);
    PhClearReference(&distributionName);
    PhClearReference(&translatedPath);

    return success;
}

NTSTATUS PhCreateProcessLxss(
    _In_ PPH_STRING LxssDistribution,
    _In_ PPH_STRING LxssCommandLine,
    _Out_ PPH_STRING *Result
    )
{
    static SECURITY_ATTRIBUTES securityAttributes = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
#if defined(PH_WSL_PSEUDOCONSOLE)
    static __typeof__(&CreatePseudoConsole) CreatePseudoConsole_I = NULL;
    static __typeof__(&ClosePseudoConsole) ClosePseudoConsole_I = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    HPCON pseudoConsoleHandle = NULL;
#endif
    NTSTATUS status;
    PPH_STRING distributionCommand = NULL;
    PPH_STRING distributionCommandLine;
    HANDLE processHandle = NULL;
    HANDLE outputReadHandle = NULL, outputWriteHandle = NULL;
    HANDLE inputReadHandle = NULL, inputWriteHandle = NULL;
    PPROC_THREAD_ATTRIBUTE_LIST attributeList = NULL;
    HANDLE handleList[2];
    STARTUPINFOEX startupInfo = { 0 };
    PROCESS_BASIC_INFORMATION basicInfo;

#if defined(PH_WSL_PSEUDOCONSOLE)
    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PVOID baseAddress;

            if (baseAddress = PhGetLoaderEntryDllBaseZ(L"kernelbase.dll"))
            {
                CreatePseudoConsole_I = PhGetDllBaseProcedureAddress(baseAddress, "CreatePseudoConsole", 0);
                ClosePseudoConsole_I = PhGetDllBaseProcedureAddress(baseAddress, "ClosePseudoConsole", 0);
            }
        }

        PhEndInitOnce(&initOnce);
    }

    if (!CreatePseudoConsole_I || !ClosePseudoConsole_I)
        return STATUS_PROCEDURE_NOT_FOUND;
#endif

    distributionCommandLine = PhGetWslDistributionCommandLine(LxssDistribution, LxssCommandLine);

    if (PhIsNullOrEmptyString(distributionCommandLine))
        return STATUS_OBJECT_NAME_NOT_FOUND;

    status = PhCreatePipeEx(
        &outputReadHandle,
        &outputWriteHandle,
        NULL,
        &securityAttributes
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreatePipeEx(
        &inputReadHandle,
        &inputWriteHandle,
        &securityAttributes,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#if defined(PH_WSL_PSEUDOCONSOLE)
    COORD coord = { 80, 25 };
    if (HR_FAILED(CreatePseudoConsole_I(coord, inputReadHandle, outputWriteHandle, 0, &pseudoConsoleHandle)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }
#endif

    status = PhInitializeProcThreadAttributeList(&attributeList, 2);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    handleList[0] = inputReadHandle;
    handleList[1] = outputWriteHandle;

    status = PhUpdateProcThreadAttribute(
        attributeList,
        PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        handleList,
        sizeof(handleList)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#if defined(PH_WSL_PSEUDOCONSOLE)
    status = PhUpdateProcThreadAttribute(
        attributeList,
        PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
        pseudoConsoleHandle,
        sizeof(pseudoConsoleHandle)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;
#endif

    memset(&startupInfo, 0, sizeof(STARTUPINFOEX));
    startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
    startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;
    startupInfo.StartupInfo.wShowWindow = SW_HIDE;
    startupInfo.StartupInfo.hStdInput = inputReadHandle;
    startupInfo.StartupInfo.hStdOutput = outputWriteHandle;
    startupInfo.StartupInfo.hStdError = outputWriteHandle;
    startupInfo.lpAttributeList = attributeList;

    status = PhCreateProcessWin32Ex(
        NULL,
        PhGetString(distributionCommandLine),
        NULL,
        NULL,
        &startupInfo,
        PH_CREATE_PROCESS_INHERIT_HANDLES | PH_CREATE_PROCESS_NEW_CONSOLE |
        PH_CREATE_PROCESS_DEFAULT_ERROR_MODE | PH_CREATE_PROCESS_EXTENDED_STARTUPINFO,
        NULL,
        NULL,
        &processHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Close the handles. (dmex)
    NtClose(inputReadHandle); inputReadHandle = NULL;
    NtClose(outputWriteHandle); outputWriteHandle = NULL;

    // Read the pipe data. (dmex)
    status = PhGetFileText(&distributionCommand, outputReadHandle, TRUE);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Get the exit code after we finish reading the data from the pipe. (dmex)
    status = PhGetProcessBasicInformation(processHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = basicInfo.ExitStatus;

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    *Result = PhReferenceObject(distributionCommand);

CleanupExit:

    if (processHandle)
        NtClose(processHandle);

#if defined(PH_WSL_PSEUDOCONSOLE)
    if (pseudoConsoleHandle)
        ClosePseudoConsole_I(pseudoConsoleHandle);
#endif

    if (outputWriteHandle)
        NtClose(outputWriteHandle);
    if (outputReadHandle)
        NtClose(outputReadHandle);
    if (inputReadHandle)
        NtClose(inputReadHandle);
    if (inputWriteHandle)
        NtClose(inputWriteHandle);

    if (attributeList)
        PhDeleteProcThreadAttributeList(attributeList);

    if (distributionCommand)
        PhDereferenceObject(distributionCommand);
    if (distributionCommandLine)
        PhDereferenceObject(distributionCommandLine);

    return status;
}
