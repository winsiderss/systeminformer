/*
 * Process Hacker -
 *   LXSS support helpers
 *
 * Copyright (C) 2019 dmex
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
#include <wslsup.h>

BOOLEAN NTAPI PhpWslDistributionNamesCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_BASIC_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PhAddItemList(Context, PhCreateStringEx(Information->Name, Information->NameLength));
    return TRUE;
}

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
                PPH_STRING lxssBasePathName = PhQueryRegistryString(subKeyHandle, L"BasePath");

                if (PhStartsWithString(FileName, lxssBasePathName, TRUE))
                {
                    lxssDistributionName = PhQueryRegistryString(subKeyHandle, L"DistributionName");

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
    {
        *LxssDistroPath = lxssDistroPath;
    }

    if (LxssFileName)
    {
        if (lxssFileName)
        {
            for (i = 0; i < lxssFileName->Length / sizeof(WCHAR); i++)
            {
                if (lxssFileName->Buffer[i] == OBJ_NAME_PATH_SEPARATOR) // RtlNtPathSeperatorString
                    lxssFileName->Buffer[i] = L'/'; // RtlAlternateDosPathSeperatorString
            }
        }

        *LxssFileName = lxssFileName;
    }

    if (LxssDistroName)
    {
        if (lxssDistributionName)
        {
            *LxssDistroName = lxssDistributionName;
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN PhInitializeLxssImageVersionInfo(
    _Out_ PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    _In_ PPH_STRING FileName
    )
{
    ULONG status;
    PPH_STRING lxssCommandLine = NULL;
    PPH_STRING lxssPackageName = NULL;
    PPH_STRING lxssDistroName;
    PPH_STRING lxssDistroPath;
    PPH_STRING lxssFileName;
    PPH_STRING result;

    if (!PhGetWslDistributionFromPath(
        FileName,
        &lxssDistroName,
        &lxssDistroPath,
        &lxssFileName
        ))
    {
        return FALSE;
    }

    if (PhIsNullOrEmptyString(lxssDistroName) || PhIsNullOrEmptyString(lxssDistroPath) || PhIsNullOrEmptyString(lxssFileName))
    {
        if (lxssDistroName) PhDereferenceObject(lxssDistroName);
        if (lxssDistroPath) PhDereferenceObject(lxssDistroPath);
        if (lxssFileName) PhDereferenceObject(lxssFileName);
        return FALSE;
    }

    if (PhEqualString2(lxssFileName, L"/init", FALSE))
    {
        PhMoveReference(&lxssFileName, PhCreateString(L"init"));
    }

    PhMoveReference(&lxssCommandLine, PhFormatString(
        L"rpm -qf %s --queryformat \"%%{VERSION}|%%{VENDOR}|%%{SUMMARY}\"",
        lxssFileName->Buffer
        ));

    status = PhCreateProcessLxss(
        lxssDistroName->Buffer,
        lxssCommandLine->Buffer,
        NULL,
        &result
        );

    if (status == 0)
    {
        goto ParseResult;
    }

    PhMoveReference(&lxssCommandLine, PhConcatStrings2(
        L"dpkg -S ",
        lxssFileName->Buffer
        ));

    status = PhCreateProcessLxss(
        lxssDistroName->Buffer,
        lxssCommandLine->Buffer,
        NULL,
        &result
        );

    if (status != 0)
    {
        PhDereferenceObject(lxssCommandLine);
        PhDereferenceObject(lxssDistroName);
        PhDereferenceObject(lxssDistroPath);
        PhDereferenceObject(lxssFileName);
        return FALSE;
    }
    else
    {
        PH_STRINGREF remainingPart;
        PH_STRINGREF packagePart;

        if (PhSplitStringRefAtChar(&result->sr, ':', &packagePart, &remainingPart))
        {
            lxssPackageName = PhCreateString2(&packagePart);
        }

        PhDereferenceObject(result);
    }

    if (PhIsNullOrEmptyString(lxssPackageName))
    {
        PhDereferenceObject(lxssCommandLine);
        PhDereferenceObject(lxssDistroName);
        PhDereferenceObject(lxssDistroPath);
        PhDereferenceObject(lxssFileName);
        return FALSE;
    }

    PhMoveReference(&lxssCommandLine, PhConcatStrings2(
        L"dpkg-query -W -f=${Version}|${Maintainer}|${binary:Summary} ",
        lxssPackageName->Buffer
        ));

    status = PhCreateProcessLxss(
        lxssDistroName->Buffer,
        lxssCommandLine->Buffer,
        NULL,
        &result
        );

    if (status != 0)
    {
        PhDereferenceObject(lxssCommandLine);
        PhDereferenceObject(lxssDistroName);
        PhDereferenceObject(lxssDistroPath);
        PhDereferenceObject(lxssFileName);
        return FALSE;
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

        remainingPart = PhGetStringRef(result);
        PhSplitStringRefAtChar(&remainingPart, '|', &versionPart, &remainingPart);
        PhSplitStringRefAtChar(&remainingPart, '|', &companyPart, &remainingPart);
        PhSplitStringRefAtChar(&remainingPart, '|', &descriptionPart, &remainingPart);

        if (versionPart.Length != 0)
            ImageVersionInfo->FileVersion = PhCreateString2(&versionPart);
        if (companyPart.Length != 0)
            ImageVersionInfo->CompanyName = PhCreateString2(&companyPart);
        if (descriptionPart.Length != 0)
            ImageVersionInfo->FileDescription = PhCreateString2(&descriptionPart);
    }

    if (result) PhDereferenceObject(result);
    if (lxssCommandLine) PhDereferenceObject(lxssCommandLine);
    if (lxssPackageName) PhDereferenceObject(lxssPackageName);
    if (lxssDistroName) PhDereferenceObject(lxssDistroName);
    if (lxssDistroPath) PhDereferenceObject(lxssDistroPath);
    if (lxssFileName) PhDereferenceObject(lxssFileName);

    return TRUE;
}

ULONG PhCreateProcessLxss(
    _In_ PWSTR LxssDistribution,
    _In_ PWSTR LxssCommandLine,
    _In_opt_ PWSTR LxssCurrentDirectory,
    _Out_ PPH_STRING *Result
    )
{
    NTSTATUS status;
    PPH_STRING lxssOutputString;
    PPH_STRING lxssCommandLine;
    PPH_STRING systemDirectory;
    HANDLE processHandle;
    HANDLE outputReadHandle, outputWriteHandle;
    HANDLE inputReadHandle, inputWriteHandle;
    PROCESS_BASIC_INFORMATION basicInfo;
    STARTUPINFO startupInfo;

    lxssCommandLine = PhFormatString(
        L"wsl.exe -d %s -e %s",
        LxssDistribution,
        LxssCommandLine
        );

    if (systemDirectory = PhGetSystemDirectory())
    {
        PhMoveReference(&lxssCommandLine, PhConcatStrings(
            3,
            systemDirectory->Buffer,
            L"\\",
            lxssCommandLine->Buffer
            ));
        PhDereferenceObject(systemDirectory);
    }

    status = PhCreatePipeEx(
        &outputReadHandle,
        &outputWriteHandle,
        TRUE,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhCreatePipeEx(
        &inputReadHandle,
        &inputWriteHandle,
        TRUE,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(outputWriteHandle);
        NtClose(outputReadHandle);
        return status;
    }

    memset(&startupInfo, 0, sizeof(STARTUPINFO));
    startupInfo.cb = sizeof(STARTUPINFO);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startupInfo.wShowWindow = SW_HIDE;
    startupInfo.hStdInput = inputReadHandle;
    startupInfo.hStdOutput = outputWriteHandle;
    startupInfo.hStdError = outputWriteHandle;

    status = PhCreateProcessWin32Ex(
        NULL,
        lxssCommandLine->Buffer,
        NULL,
        LxssCurrentDirectory,
        &startupInfo,
        PH_CREATE_PROCESS_INHERIT_HANDLES | PH_CREATE_PROCESS_NEW_CONSOLE,
        NULL,
        NULL,
        &processHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(outputWriteHandle);
        NtClose(outputReadHandle);
        NtClose(inputReadHandle);
        NtClose(inputWriteHandle);
        return status;
    }

    // Note: Close the write handles or the child process
    // won't exit and ReadFile will block indefinitely. (dmex)
    NtClose(inputReadHandle);
    NtClose(outputWriteHandle);

    // Read the pipe data. (dmex)
    lxssOutputString = PhGetFileText(outputReadHandle, TRUE);

    // Get the exit code after we finish reading the data from the pipe. (dmex)
    if (NT_SUCCESS(status = PhGetProcessBasicInformation(processHandle, &basicInfo)))
    {
        status = basicInfo.ExitStatus;
    }

    // Note: Don't use NTSTATUS now that we have the lxss exit code. (dmex)
    if (status == 0)
    {
        *Result = lxssOutputString;
    }
    else
    {
        PhSetReference(&lxssOutputString, NULL);
    }

    NtClose(processHandle);
    NtClose(outputReadHandle);
    NtClose(inputWriteHandle);

    return status;
}
