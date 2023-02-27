/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2019-2022
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
    _In_ PPH_STRING FileName,
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

                if (PhStartsWithString(FileName, lxssBasePathName, TRUE))
                {
                    lxssDistributionName = PhQueryRegistryStringZ(subKeyHandle, L"DistributionName");

                    if (LxssDistroPath)
                    {
                        PhSetReference(&lxssDistroPath, lxssBasePathName);
                    }

                    if (LxssFileName)
                    {
                        lxssFileName = PhDuplicateString(FileName);
                        PhSkipStringRef(&lxssFileName->sr, lxssBasePathName->Length);
                        PhSkipStringRef(&lxssFileName->sr, sizeof(L"rootfs"));
                    }
                }

                PhDereferenceObject(lxssBasePathName);
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
BOOLEAN PhInitializeLxssImageVersionInfo(
    _Inout_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRING FileName
    )
{
    static PH_STRINGREF lxssDpkgCommandLine = PH_STRINGREF_INIT(L"dpkg -S ");
    static PH_STRINGREF lxssDpkgQueryCommandLine = PH_STRINGREF_INIT(L"dpkg-query -W -f=${Version}|${Maintainer}|${binary:Summary} ");
    BOOLEAN success = FALSE;
    PPH_STRING lxssCommandResult = NULL;
    PPH_STRING lxssCommandLine = NULL;
    PPH_STRING lxssPackageName = NULL;
    PPH_STRING lxssDistroName;
    PPH_STRING lxssFileName;
    PH_FORMAT format[3];

    if (!PhGetWslDistributionFromPath(FileName, &lxssDistroName, NULL, &lxssFileName))
        return FALSE;
    if (PhIsNullOrEmptyString(lxssDistroName) || PhIsNullOrEmptyString(lxssFileName))
        goto CleanupExit;
    if (PhEqualString2(lxssFileName, L"/init", FALSE))
        goto CleanupExit;

    // "rpm -qf %s --queryformat \"%%{VERSION}|%%{VENDOR}|%%{SUMMARY}\""
    PhInitFormatS(&format[0], L"rpm -qf ");
    PhInitFormatSR(&format[1], lxssFileName->sr);
    PhInitFormatS(&format[2], L" --queryformat \"%%{VERSION}|%%{VENDOR}|%%{SUMMARY}\"");

    lxssCommandLine = PhFormat(
        format,
        RTL_NUMBER_OF(format),
        0x100
        );

    if (PhCreateProcessLxss(
        lxssDistroName,
        lxssCommandLine,
        &lxssCommandResult
        ))
    {
        goto ParseResult;
    }

    PhMoveReference(&lxssCommandLine, PhConcatStringRef2(
        &lxssDpkgCommandLine,
        &lxssFileName->sr
        ));

    if (!PhCreateProcessLxss(
        lxssDistroName,
        lxssCommandLine,
        &lxssCommandResult
        ))
    {
        // The dpkg metadata for some packages doesn't contain /usr for
        // /usr/bin/ paths. Try lookup the package again without /usr. (dmex)
        if (PhStartsWithString2(lxssFileName, L"/usr", TRUE))
        {
            PhSkipStringRef(&lxssFileName->sr, 4 * sizeof(WCHAR));

            PhMoveReference(&lxssCommandLine, PhConcatStringRef2(
                &lxssDpkgCommandLine,
                &lxssFileName->sr
                ));

            PhCreateProcessLxss(
                lxssDistroName,
                lxssCommandLine,
                &lxssCommandResult
                );
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

    PhMoveReference(&lxssCommandLine, PhConcatStringRef2(
        &lxssDpkgQueryCommandLine,
        &lxssPackageName->sr
        ));

    if (!PhCreateProcessLxss(
        lxssDistroName,
        lxssCommandLine,
        &lxssCommandResult
        ))
    {
        goto CleanupExit;
    }

ParseResult:
    if (
        !ImageVersionInfo->FileVersion &&
        !ImageVersionInfo->CompanyName &&
        !ImageVersionInfo->FileDescription
        )
    {
        PH_STRINGREF remainingPart;
        PH_STRINGREF companyPart;
        PH_STRINGREF descriptionPart;
        PH_STRINGREF versionPart;

        remainingPart = PhGetStringRef(lxssCommandResult);
        PhSplitStringRefAtChar(&remainingPart, L'|', &versionPart, &remainingPart);
        PhSplitStringRefAtChar(&remainingPart, L'|', &companyPart, &remainingPart);
        PhSplitStringRefAtChar(&remainingPart, L'|', &descriptionPart, &remainingPart);

        if (versionPart.Length != 0)
            ImageVersionInfo->FileVersion = PhCreateString2(&versionPart);
        if (companyPart.Length != 0)
            ImageVersionInfo->CompanyName = PhCreateString2(&companyPart);
        if (descriptionPart.Length != 0)
            ImageVersionInfo->FileDescription = PhCreateString2(&descriptionPart);
    }

    success = TRUE;

CleanupExit:

    if (lxssCommandResult) PhDereferenceObject(lxssCommandResult);
    if (lxssCommandLine) PhDereferenceObject(lxssCommandLine);
    if (lxssPackageName) PhDereferenceObject(lxssPackageName);
    if (lxssDistroName) PhDereferenceObject(lxssDistroName);
    if (lxssFileName) PhDereferenceObject(lxssFileName);

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
        if (basicInfo.ExitStatus == 0)
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
