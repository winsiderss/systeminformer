/*
 * Process Hacker - 
 *   application support functions
 * 
 * Copyright (C) 2010 wj32
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
#include <wtsapi32.h>

NTSTATUS PhGetProcessKnownType(
    __in HANDLE ProcessHandle,
    __out PPH_KNOWN_PROCESS_TYPE KnownProcessType
    )
{
    NTSTATUS status;
    PH_KNOWN_PROCESS_TYPE knownProcessType;
    PROCESS_BASIC_INFORMATION basicInfo;
    PPH_STRING systemDirectory;
    PPH_STRING fileName;
    PPH_STRING newFileName;
    PPH_STRING baseName;

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

    systemDirectory = PhGetSystemDirectory();

    if (!systemDirectory)
        return STATUS_UNSUCCESSFUL;

    if (!NT_SUCCESS(status = PhGetProcessImageFileName(
        ProcessHandle,
        &fileName
        )))
    {
        PhDereferenceObject(systemDirectory);
        return status;
    }

    newFileName = PhGetFileName(fileName);
    PhDereferenceObject(fileName);

    knownProcessType = UnknownProcessType;

    if (PhStringStartsWith(newFileName, systemDirectory, TRUE))
    {
        baseName = PhSubstring(
            newFileName,
            systemDirectory->Length / 2,
            (newFileName->Length - systemDirectory->Length) / 2
            );

        // The system directory string never ends in a backslash, unless 
        // it is a drive root. We're not going to account for that case.

        if (!baseName)
            ; // Dummy
        else if (PhStringEquals2(baseName, L"\\smss.exe", TRUE))
            knownProcessType = SessionManagerProcessType;
        else if (PhStringEquals2(baseName, L"\\csrss.exe", TRUE))
            knownProcessType = WindowsSubsystemProcessType;
        else if (PhStringEquals2(baseName, L"\\wininit.exe", TRUE))
            knownProcessType = WindowsStartupProcessType;
        else if (PhStringEquals2(baseName, L"\\services.exe", TRUE))
            knownProcessType = ServiceControlManagerProcessType;
        else if (PhStringEquals2(baseName, L"\\lsass.exe", TRUE))
            knownProcessType = LocalSecurityAuthorityProcessType;
        else if (PhStringEquals2(baseName, L"\\lsm.exe", TRUE))
            knownProcessType = LocalSessionManagerProcessType;
        else if (PhStringEquals2(baseName, L"\\svchost.exe", TRUE))
            knownProcessType = ServiceHostProcessType;
        else if (PhStringEquals2(baseName, L"\\rundll32.exe", TRUE))
            knownProcessType = RunDllAsAppProcessType;
        else if (PhStringEquals2(baseName, L"\\dllhost.exe", TRUE))
            knownProcessType = ComSurrogateProcessType;

        PhDereferenceObject(baseName);
    }

    PhDereferenceObject(systemDirectory);
    PhDereferenceObject(newFileName);

    *KnownProcessType = knownProcessType;

    return status;
}

static BOOLEAN NTAPI PhpSvchostCommandLineCallback(
    __in_opt PPH_COMMAND_LINE_OPTION Option,
    __in_opt PPH_STRING Value,
    __in PVOID Context
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
    __in PPH_STRING CommandLine,
    __in PH_KNOWN_PROCESS_TYPE KnownProcessType,
    __out PPH_KNOWN_PROCESS_COMMAND_LINE KnownCommandLine
    )
{
    switch (KnownProcessType)
    {
    case ServiceHostProcessType:
        {
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

            return KnownCommandLine->ServiceHost.GroupName != NULL;
        }
        break;
    case RunDllAsAppProcessType:
        {
            ULONG i;
            ULONG lastIndexOfComma;
            PPH_STRING dllName;
            PPH_STRING procedureName;

            i = 0;

            // Get the rundll32.exe part.

            dllName = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!dllName)
                return FALSE;

            PhDereferenceObject(dllName);

            // Get the DLL name part.

            while (i < (ULONG)CommandLine->Length / 2 && CommandLine->Buffer[i] == ' ')
                i++;

            dllName = PhParseCommandLinePart(&CommandLine->sr, &i);

            if (!dllName)
                return FALSE;

            PhaDereferenceObject(dllName);

            // The procedure name begins after the last comma.

            lastIndexOfComma = PhStringLastIndexOfChar(dllName, 0, ',');

            if (lastIndexOfComma == -1)
                return FALSE;

            procedureName = PhaSubstring(
                dllName,
                lastIndexOfComma + 1,
                dllName->Length / 2 - lastIndexOfComma - 1
                );
            dllName = PhaSubstring(dllName, 0, lastIndexOfComma);

            // If the DLL name isn't an absolute path, assume it's in system32.

            if (PhStringIndexOfChar(dllName, 0, ':') == -1)
            {
                dllName = PhaConcatStrings(
                    3,
                    ((PPH_STRING)PHA_DEREFERENCE(PhGetSystemDirectory()))->Buffer,
                    L"\\",
                    dllName
                    );
            }

            KnownCommandLine->RunDllAsApp.FileName = dllName;
            KnownCommandLine->RunDllAsApp.ProcedureName = procedureName;
        }
        break;
    case ComSurrogateProcessType:
        {
            // TODO
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

PPH_STRING PhGetSessionInformationString(
    __in HANDLE ServerHandle,
    __in ULONG SessionId,
    __in ULONG InformationClass
    )
{
    PPH_STRING string;
    PWSTR buffer;
    ULONG length;

    if (WTSQuerySessionInformation(
        ServerHandle,
        SessionId,
        (WTS_INFO_CLASS)InformationClass,
        &buffer,
        &length
        ))
    {
        string = PhCreateString(buffer);
        WTSFreeMemory(buffer);

        return string;
    }
    else
    {
        return NULL;
    }
}

VOID PhSearchOnlineString(
    __in HWND hWnd,
    __in PWSTR String
    )
{
    PPH_STRING searchEngine = PhGetStringSetting(L"SearchEngine");
    ULONG indexOfReplacement = PhStringIndexOfString(searchEngine, 0, L"%s");

    if (indexOfReplacement != -1)
    {
        PPH_STRING stringBefore;
        PPH_STRING stringAfter;
        PPH_STRING newString;

        // Replace "%s" with the string.

        stringBefore = PhSubstring(searchEngine, 0, indexOfReplacement);
        stringAfter = PhSubstring(
            searchEngine,
            indexOfReplacement + 2,
            searchEngine->Length / 2 - indexOfReplacement - 2
            );

        newString = PhConcatStrings(
            3,
            stringBefore->Buffer,
            String,
            stringAfter->Buffer
            );
        PhShellExecute(hWnd, newString->Buffer, NULL);

        PhDereferenceObject(newString);
        PhDereferenceObject(stringAfter);
        PhDereferenceObject(stringBefore);
    }

    PhDereferenceObject(searchEngine);
}
