/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2019-2023
 *
 */

#include <ph.h>
#include <wslsup.h>

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
    static PH_STRINGREF lxssKeyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Lxss");
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
                PPH_STRING lxssBasePathName = PhQueryRegistryStringZ(subKeyHandle, L"BasePath");
                PhMoveReference(&lxssBasePathName, PhDosPathNameToNtPathName(&lxssBasePathName->sr));

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

_Success_(return)
BOOLEAN PhCreateProcessLxss(
    _In_ PPH_STRING LxssDistribution,
    _In_ PPH_STRING LxssCommandLine,
    _Out_ PPH_STRING *Result
    )
{
    BOOLEAN result = FALSE;
    PPH_STRING lxssOutputString = NULL;
    PPH_STRING lxssCommandLine;
    PPH_STRING systemDirectory;
    HANDLE processHandle;
    HANDLE outputReadHandle = NULL, outputWriteHandle = NULL;
    HANDLE inputReadHandle = NULL, inputWriteHandle = NULL;
    STARTUPINFOEX startupInfo = { 0 };
    PROCESS_BASIC_INFORMATION basicInfo;
    PH_FORMAT format[4];

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

    if (!NT_SUCCESS(PhCreatePipeEx(
        &outputReadHandle,
        &outputWriteHandle,
        TRUE,
        NULL
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(PhCreatePipeEx(
        &inputReadHandle,
        &inputWriteHandle,
        TRUE,
        NULL
        )))
    {
        goto CleanupExit;
    }

    memset(&startupInfo, 0, sizeof(STARTUPINFOEX));
    startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
    startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;
    startupInfo.StartupInfo.wShowWindow = SW_HIDE;
    startupInfo.StartupInfo.hStdInput = inputReadHandle;
    startupInfo.StartupInfo.hStdOutput = outputWriteHandle;
    startupInfo.StartupInfo.hStdError = outputWriteHandle;

    if (!NT_SUCCESS(PhInitializeProcThreadAttributeList(&startupInfo.lpAttributeList, 1)))
        goto CleanupExit;

    if (!NT_SUCCESS(PhUpdateProcThreadAttribute(
        startupInfo.lpAttributeList,
        PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        &(HANDLE[2]){ inputReadHandle, outputWriteHandle },
        sizeof(HANDLE[2])
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(PhCreateProcessWin32Ex(
        NULL,
        PhGetString(lxssCommandLine),
        NULL,
        NULL,
        &startupInfo.StartupInfo,
        PH_CREATE_PROCESS_INHERIT_HANDLES | PH_CREATE_PROCESS_NEW_CONSOLE |
        PH_CREATE_PROCESS_DEFAULT_ERROR_MODE | PH_CREATE_PROCESS_EXTENDED_STARTUPINFO,
        NULL,
        NULL,
        &processHandle,
        NULL
        )))
    {
        goto CleanupExit;
    }

    // Close the handles. (dmex)
    NtClose(inputReadHandle); inputReadHandle = NULL;
    NtClose(outputWriteHandle); outputWriteHandle = NULL;

    // Read the pipe data. (dmex)
    lxssOutputString = PhGetFileText(outputReadHandle, TRUE);

    // Get the exit code after we finish reading the data from the pipe. (dmex)
    if (NT_SUCCESS(PhGetProcessBasicInformation(processHandle, &basicInfo)))
    {
        if (basicInfo.ExitStatus == 0 && !PhIsNullOrEmptyString(lxssOutputString))
        {
            *Result = PhReferenceObject(lxssOutputString);
            result = TRUE;
        }
    }

    NtClose(processHandle);

CleanupExit:

    if (lxssOutputString)
        PhDereferenceObject(lxssOutputString);
    if (lxssCommandLine)
        PhDereferenceObject(lxssCommandLine);

    if (outputWriteHandle)
        NtClose(outputWriteHandle);
    if (outputReadHandle)
        NtClose(outputReadHandle);
    if (inputReadHandle)
        NtClose(inputReadHandle);
    if (inputWriteHandle)
        NtClose(inputWriteHandle);

    if (startupInfo.lpAttributeList)
        PhDeleteProcThreadAttributeList(startupInfo.lpAttributeList);

    return result;
}
