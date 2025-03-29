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

BOOLEAN NTAPI PhpWslDistributionNamesCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_BASIC_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PhAddItemList(Context, PhCreateStringEx(Information->Name, Information->NameLength));
    return TRUE;
}

_Success_(return)
BOOLEAN PhGetWslDistributionFromPath(
    _In_ PPH_STRINGREF FileName,
    _Out_opt_ PPH_STRING *LxssDistroName,
    _Out_opt_ PPH_STRING *LxssDistroPath,
    _Out_opt_ PPH_STRING *LxssFileName
    )
{
    static CONST PH_STRINGREF lxssKeyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Lxss");
    PPH_STRING lxssDistributionName = NULL;
    PPH_STRING lxssDistroPath = NULL;
    PPH_STRING lxssFileName = NULL;
    HANDLE keyHandle;
    ULONG i;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &lxssKeyPath,
        0
        )))
    {
        PPH_LIST distributionGuidList;

        distributionGuidList = PhCreateList(1);
        PhEnumerateKey(keyHandle, KeyBasicInformation, PhpWslDistributionNamesCallback, distributionGuidList);

        for (i = 0; i < distributionGuidList->Count; i++)
        {
            HANDLE subKeyHandle;
            PPH_STRING subKeyName;

            subKeyName = distributionGuidList->Items[i];

            if (NT_SUCCESS(PhOpenKey(
                &subKeyHandle,
                KEY_READ,
                keyHandle,
                &subKeyName->sr,
                0
                )))
            {
                PPH_STRING lxssBasePathName;

                if (lxssBasePathName = PhQueryRegistryStringZ(subKeyHandle, L"BasePath"))
                {
                    PhMoveReference(&lxssBasePathName, PhDosPathNameToNtPathName(&lxssBasePathName->sr));
                }

                if (lxssBasePathName && PhStartsWithStringRef(FileName, &lxssBasePathName->sr, TRUE))
                {
                    lxssDistributionName = PhQueryRegistryStringZ(subKeyHandle, L"DistributionName");

                    if (LxssDistroPath)
                    {
                        PhSetReference(&lxssDistroPath, lxssBasePathName);
                    }

                    if (LxssFileName)
                    {
                        lxssFileName = PhCreateString2(FileName);
                        PhSkipStringRef(&lxssFileName->sr, lxssBasePathName->Length);
                        PhSkipStringRef(&lxssFileName->sr, sizeof(L"rootfs"));
                    }
                }

                PhClearReference(&lxssBasePathName);
                NtClose(subKeyHandle);
            }

            if (lxssDistributionName)
                break;
        }

        PhDereferenceObjects(distributionGuidList->Items, distributionGuidList->Count);
        PhDereferenceObject(distributionGuidList);

        NtClose(keyHandle);
    }

    if (LxssDistroPath)
        *LxssDistroPath = lxssDistroPath;
    else
    {
        if (lxssDistroPath)
            PhDereferenceObject(lxssDistroPath);
    }

    if (LxssFileName)
    {
        if (lxssFileName)
        {
            for (i = 0; i < lxssFileName->Length / sizeof(WCHAR); i++)
            {
                if (lxssFileName->Buffer[i] == OBJ_NAME_PATH_SEPARATOR) // RtlNtPathSeperatorString
                    lxssFileName->Buffer[i] = OBJ_NAME_ALTPATH_SEPARATOR; // RtlAlternateDosPathSeperatorString
            }
        }

        *LxssFileName = lxssFileName;
    }
    else
    {
        if (lxssFileName)
            PhDereferenceObject(lxssFileName);
    }

    if (LxssDistroName)
    {
        *LxssDistroName = lxssDistributionName;
        return TRUE;
    }
    else
    {
        if (lxssDistributionName)
            PhDereferenceObject(lxssDistributionName);
    }

    return FALSE;
}

_Success_(return)
BOOLEAN PhDosPathNameToWslPathName(
    _In_ PPH_STRINGREF FileName,
    _Out_ PPH_STRING* LxssFileName
    )
{
    BOOLEAN success = FALSE;
    PPH_STRING win32FileName = NULL;
    PPH_STRING lxssDistroName = NULL;
    PPH_STRING lxssCommandLine = NULL;
    PPH_STRING lxssCommandResult = NULL;
    PH_FORMAT format[3];

    win32FileName = PhCreateString2(FileName);
    PhMoveReference(&win32FileName, PhGetFileName(win32FileName));

    if (PhIsNullOrEmptyString(win32FileName))
        return FALSE;

    if (!PhGetWslDistributionFromPath(FileName, &lxssDistroName, NULL, NULL))
        goto CleanupExit;

    PhInitFormatS(&format[0], L"wslpath -a -u \"");
    PhInitFormatSR(&format[1], win32FileName->sr);
    PhInitFormatC(&format[2], L'\"');
    lxssCommandLine = PhFormat(format, RTL_NUMBER_OF(format), 0x100);

    if (!PhCreateProcessLxss(lxssDistroName, lxssCommandLine, &lxssCommandResult))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(lxssCommandResult))
        goto CleanupExit;

    if (PhEndsWithString2(lxssCommandResult, L"\n", FALSE))
    {
        lxssCommandResult->Length -= sizeof(WCHAR[1]);
        lxssCommandResult->Buffer[lxssCommandResult->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    *LxssFileName = PhReferenceObject(lxssCommandResult);
    success = TRUE;

CleanupExit:

    if (lxssCommandResult) PhDereferenceObject(lxssCommandResult);
    if (lxssCommandLine) PhDereferenceObject(lxssCommandLine);
    if (lxssDistroName) PhDereferenceObject(lxssDistroName);
    if (win32FileName) PhDereferenceObject(win32FileName);

    return success;
}

_Success_(return)
BOOLEAN PhWslQueryDistroProcessCommandLine(
    _In_ PPH_STRINGREF FileName,
    _In_ ULONG LxssProcessId,
    _Out_ PPH_STRING* Result
    )
{
    PPH_STRING lxssCommandResult = NULL;
    PPH_STRING lxssCommandLine = NULL;
    PPH_STRING lxssDistroName = NULL;
    PH_FORMAT format[5];

    if (!PhGetWslDistributionFromPath(FileName, &lxssDistroName, NULL, NULL))
        return FALSE;

    PhInitFormatS(&format[0], L"\\Device\\Mup\\wsl.localhost\\");
    PhInitFormatSR(&format[1], lxssDistroName->sr);
    PhInitFormatS(&format[2], L"\\proc\\");
    PhInitFormatIU(&format[3], LxssProcessId);
    PhInitFormatS(&format[4], L"\\cmdline");

    if (lxssCommandLine = PhFormat(format, RTL_NUMBER_OF(format), 0x100))
    {
        NTSTATUS status;
        HANDLE fileHandle;

        status = PhCreateFile(
            &fileHandle,
            &lxssCommandLine->sr,
            FILE_GENERIC_READ,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (NT_SUCCESS(status))
        {
            status = PhGetFileText(
                &lxssCommandResult,
                fileHandle,
                TRUE
                );
            NtClose(fileHandle);
        }
        else if (status == STATUS_ACCESS_DENIED)
        {
            PPH_STRING lxssRootCommandLine;
            PPH_STRING lxssRootCommandResult;

            // Note: The WSL P9 Multiple UNC Provider (MUP) doesn't allow administrators to read /proc unless the distro wsl.conf default user is root.
            // Changing the default user to root fixes permission issues when accessing files from Windows but results in everything owned by root and inaccessible from WSL.
            // We can workaround the issue by creating a root process using 'wsl.exe -u root' and pipe the /proc content via standard output.
            // This is significantly slower (very slow) but works, and avoids hard requirements on user preferences and distro configuration.

            PhInitFormatS(&format[0], L"cat /proc/");
            PhInitFormatIU(&format[1], LxssProcessId);
            PhInitFormatS(&format[2], L"/cmdline");

            if (lxssRootCommandLine = PhFormat(format, 3, 0x100))
            {
                if (PhCreateProcessLxss(lxssDistroName, lxssRootCommandLine, &lxssRootCommandResult))
                {
                    lxssCommandResult = lxssRootCommandResult;
                }

                PhDereferenceObject(lxssRootCommandLine);
            }
        }

        PhDereferenceObject(lxssCommandLine);
    }

    PhDereferenceObject(lxssDistroName);

    if (lxssCommandResult)
    {
        *Result = lxssCommandResult;
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN PhWslQueryDistroProcessEnvironment(
    _In_ PPH_STRINGREF FileName,
    _In_ ULONG LxssProcessId,
    _Out_ PPH_STRING* Result
    )
{
    PPH_STRING lxssCommandResult = NULL;
    PPH_STRING lxssCommandLine = NULL;
    PPH_STRING lxssDistroName = NULL;
    PH_FORMAT format[5];

    if (!PhGetWslDistributionFromPath(FileName, &lxssDistroName, NULL, NULL))
        return FALSE;

    PhInitFormatS(&format[0], L"\\Device\\Mup\\wsl.localhost\\");
    PhInitFormatSR(&format[1], lxssDistroName->sr);
    PhInitFormatS(&format[2], L"\\proc\\");
    PhInitFormatIU(&format[3], LxssProcessId);
    PhInitFormatS(&format[4], L"\\environ");

    if (lxssCommandLine = PhFormat(format, RTL_NUMBER_OF(format), 0x100))
    {
        NTSTATUS status;
        HANDLE fileHandle;

        status = PhCreateFile(
            &fileHandle,
            &lxssCommandLine->sr,
            FILE_GENERIC_READ,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (NT_SUCCESS(status))
        {
            status = PhGetFileText(
                &lxssCommandResult,
                fileHandle,
                TRUE
                );
            NtClose(fileHandle);
        }
        else if (status == STATUS_ACCESS_DENIED)
        {
            PPH_STRING lxssRootCommandLine;
            PPH_STRING lxssRootCommandResult;

            // Note: The WSL P9 Multiple UNC Provider (MUP) doesn't allow administrators to read /proc unless the distro wsl.conf default user is root.
            // Changing the default user to root fixes permission issues when accessing files from Windows but results in everything owned by root and inaccessible from WSL.
            // We can workaround the issue by creating a root process using 'wsl.exe -u root' and pipe the /proc content via standard output.
            // This is significantly slower (very slow) but works, and avoids hard requirements on user preferences and distro configuration.

            PhInitFormatS(&format[0], L"cat /proc/");
            PhInitFormatIU(&format[1], LxssProcessId);
            PhInitFormatS(&format[2], L"/environ");

            if (lxssRootCommandLine = PhFormat(format, 3, 0x100))
            {
                if (PhCreateProcessLxss(lxssDistroName, lxssRootCommandLine, &lxssRootCommandResult))
                {
                    lxssCommandResult = lxssRootCommandResult;
                }

                PhDereferenceObject(lxssRootCommandLine);
            }
        }

        PhDereferenceObject(lxssCommandLine);
    }

    PhDereferenceObject(lxssDistroName);

    if (lxssCommandResult)
    {
        *Result = lxssCommandResult;
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN PhWslQueryRpmPackageFromFileName(
    _In_ PPH_STRING LxssDistribution,
    _In_ PPH_STRING LxssFileName,
    _Out_ PPH_STRING* Result
    )
{
    PPH_STRING lxssCommandLine;
    PPH_STRING lxssCommandResult;
    PH_FORMAT format[3];

    // "rpm -qf %s --queryformat \"%%{VERSION}|%%{VENDOR}|%%{SUMMARY}\""
    PhInitFormatS(&format[0], L"rpm -qf \"");
    PhInitFormatSR(&format[1], LxssFileName->sr);
    PhInitFormatS(&format[2], L"\" --queryformat \"%%{VERSION}|%%{VENDOR}|%%{SUMMARY}\"");
    lxssCommandLine = PhFormat(format, RTL_NUMBER_OF(format), 0x100);

    if (PhCreateProcessLxss(LxssDistribution, lxssCommandLine, &lxssCommandResult))
    {
        *Result = lxssCommandResult;
        PhDereferenceObject(lxssCommandLine);
        return TRUE;
    }

    PhDereferenceObject(lxssCommandLine);
    return FALSE;
}

_Success_(return)
BOOLEAN PhWslQueryDebianPackageFromFileName(
    _In_ PPH_STRING LxssDistribution,
    _In_ PPH_STRING LxssFileName,
    _Out_ PPH_STRING* Result
    )
{
    BOOLEAN success = FALSE;
    PPH_STRING lxssCommandLine;
    PPH_STRING lxssCommandResult = NULL;
    PPH_STRING lxssPackageName = NULL;
    PH_FORMAT format[3];

    // "dpkg -S %s"
    PhInitFormatS(&format[0], L"dpkg -S \"");
    PhInitFormatSR(&format[1], LxssFileName->sr);
    PhInitFormatS(&format[2], L"\"");
    lxssCommandLine = PhFormat(format, RTL_NUMBER_OF(format), 0x100);

    if (!PhCreateProcessLxss(LxssDistribution, lxssCommandLine, &lxssCommandResult))
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
            PhMoveReference(&lxssCommandLine, PhFormat(format, RTL_NUMBER_OF(format), 0x100));

            if (!PhCreateProcessLxss(LxssDistribution, lxssCommandLine, &lxssCommandResult))
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
    PhMoveReference(&lxssCommandLine, PhFormat(format, RTL_NUMBER_OF(format), 0x100));

    if (!PhCreateProcessLxss(LxssDistribution, lxssCommandLine, &lxssCommandResult))
    {
        goto CleanupExit;
    }

    *Result = PhReferenceObject(lxssCommandResult);
    success = TRUE;

CleanupExit:
    if (lxssPackageName) PhDereferenceObject(lxssPackageName);
    if (lxssCommandResult) PhDereferenceObject(lxssCommandResult);
    if (lxssCommandLine) PhDereferenceObject(lxssCommandLine);

    return success;
}

_Success_(return)
BOOLEAN PhWslParsePackageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRING LxssCommandResult
    )
{
    PH_STRINGREF remainingPart;
    PH_STRINGREF companyPart;
    PH_STRINGREF descriptionPart;
    PH_STRINGREF versionPart;

    if (PhIsNullOrEmptyString(LxssCommandResult))
        return FALSE;

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

    return TRUE;
}

BOOLEAN PhInitializeLxssImageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRINGREF FileName
    )
{
    BOOLEAN success = FALSE;
    PPH_STRING lxssCommandResult = NULL;
    PPH_STRING lxssDistroName = NULL;
    PPH_STRING lxssFileName = NULL;

    if (!PhGetWslDistributionFromPath(FileName, &lxssDistroName, NULL, &lxssFileName))
        return FALSE;
    if (PhIsNullOrEmptyString(lxssFileName))
        goto CleanupExit;
    if (PhEqualString2(lxssFileName, L"/init", FALSE))
        goto CleanupExit;

    if (PhWslQueryDebianPackageFromFileName(lxssDistroName, lxssFileName, &lxssCommandResult))
    {
        if (PhWslParsePackageVersionInfo(ImageVersionInfo, lxssCommandResult))
        {
            success = TRUE;
            goto CleanupExit;
        }
    }

    if (PhWslQueryRpmPackageFromFileName(lxssDistroName, lxssFileName, &lxssCommandResult))
    {
        if (PhWslParsePackageVersionInfo(ImageVersionInfo, lxssCommandResult))
        {
            success = TRUE;
            goto CleanupExit;
        }
    }

CleanupExit:
    PhClearReference(&lxssCommandResult);
    PhClearReference(&lxssDistroName);
    PhClearReference(&lxssFileName);

    return success;
}

BOOLEAN PhCreateProcessLxss(
    _In_ PPH_STRING LxssDistribution,
    _In_ PPH_STRING LxssCommandLine,
    _Out_ PPH_STRING *Result
    )
{
    static SECURITY_ATTRIBUTES securityAttributes = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    static HRESULT (WINAPI* CreatePseudoConsole_I)(
    _In_ COORD size,
    _In_ HANDLE hInput,
    _In_ HANDLE hOutput,
    _In_ DWORD dwFlags,
    _Out_ HPCON* phPC
    ) = NULL;
    static VOID (WINAPI* ClosePseudoConsole_I)(_In_ HPCON hPC) = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    NTSTATUS status;
    BOOLEAN result = FALSE;
    PPH_STRING lxssOutputString = NULL;
    PPH_STRING lxssCommandLine;
    PPH_STRING systemDirectory;
    HANDLE processHandle = NULL;
    HPCON pseudoConsoleHandle = NULL;
    HANDLE outputReadHandle = NULL, outputWriteHandle = NULL;
    HANDLE inputReadHandle = NULL, inputWriteHandle = NULL;
    PPROC_THREAD_ATTRIBUTE_LIST attributeList = NULL;
    HANDLE handleList[2];
    STARTUPINFOEX startupInfo = { 0 };
    PROCESS_BASIC_INFORMATION basicInfo;
    PH_FORMAT format[4];

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

    *Result = NULL;

    if (!CreatePseudoConsole_I || !ClosePseudoConsole_I)
        return FALSE;

    // "wsl.exe -u root -d %s -e %s"
    PhInitFormatS(&format[0], L"wsl.exe -u root -d ");
    PhInitFormatSR(&format[1], LxssDistribution->sr);
    PhInitFormatS(&format[2], L" -e ");
    PhInitFormatSR(&format[3], LxssCommandLine->sr);

    lxssCommandLine = PhFormat(
        format,
        RTL_NUMBER_OF(format),
        0x100
        );

    if (systemDirectory = PhGetSystemDirectory())
    {
        PhMoveReference(&lxssCommandLine, PhConcatStringRef3(
            &systemDirectory->sr,
            &PhNtPathSeperatorString,
            &lxssCommandLine->sr
            ));
        PhDereferenceObject(systemDirectory);
    }

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

    COORD coord = { 80, 25 };
    if (HR_FAILED(CreatePseudoConsole_I(coord, inputReadHandle, outputWriteHandle, 0, &pseudoConsoleHandle)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

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

    status = PhUpdateProcThreadAttribute(
        attributeList,
        PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
        pseudoConsoleHandle,
        sizeof(pseudoConsoleHandle)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

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
        PhGetString(lxssCommandLine),
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
    status = PhGetFileText(&lxssOutputString, outputReadHandle, TRUE);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Get the exit code after we finish reading the data from the pipe. (dmex)
    status = PhGetProcessBasicInformation(processHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = basicInfo.ExitStatus;

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(lxssOutputString))
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    *Result = PhReferenceObject(lxssOutputString);
    result = TRUE;

CleanupExit:

    if (processHandle)
        NtClose(processHandle);

    if (pseudoConsoleHandle)
        ClosePseudoConsole_I(pseudoConsoleHandle);

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

    if (lxssOutputString)
        PhDereferenceObject(lxssOutputString);
    if (lxssCommandLine)
        PhDereferenceObject(lxssCommandLine);

    return result;
}
