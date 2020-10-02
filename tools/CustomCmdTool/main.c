/*
 * Process Hacker -
 *   Console command tool
 *
 * Copyright (C) 2020 dmex
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

#include "main.h"

#define PH_ARG_COMMANDMODE 5
#define PH_ARG_COMMANDTYPE 6
#define PH_ARG_COMMANDOBJECT 7
#define PH_ARG_COMMANDACTION 8
#define PH_ARG_COMMANDVALUE 9
#define PH_ARG_PRIORITY 25

ULONG CommandMode = 0;
PPH_STRING CommandType = NULL;
PPH_STRING CommandObject = NULL;
PPH_STRING CommandAction = NULL;
PPH_STRING CommandValue = NULL;

BOOLEAN NTAPI PhpCommandLineCallback(
    _In_opt_ PPH_COMMAND_LINE_OPTION Option,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PVOID Context
    )
{
    if (Option)
    {
        switch (Option->Id)
        {
        case PH_ARG_COMMANDMODE:
            CommandMode = TRUE;
            break;
        case PH_ARG_COMMANDTYPE:
            PhSwapReference(&CommandType, Value);
            break;
        case PH_ARG_COMMANDOBJECT:
            PhSwapReference(&CommandObject, Value);
            break;
        case PH_ARG_COMMANDACTION:
            PhSwapReference(&CommandAction, Value);
            break;
        case PH_ARG_COMMANDVALUE:
            PhSwapReference(&CommandValue, Value);
            break;
        }
    }

    return TRUE;
}

int __cdecl wmain(int argc, wchar_t *argv[])
{
    PH_COMMAND_LINE_OPTION options[] =
    {
        { PH_ARG_COMMANDMODE, L"c", NoArgumentType },
        { PH_ARG_COMMANDTYPE, L"ctype", MandatoryArgumentType },
        { PH_ARG_COMMANDOBJECT, L"cobject", MandatoryArgumentType },
        { PH_ARG_COMMANDACTION, L"caction", MandatoryArgumentType },
        { PH_ARG_COMMANDVALUE, L"cvalue", MandatoryArgumentType },
    };
    PH_STRINGREF commandLine;

    if (!NT_SUCCESS(PhInitializePhLibEx(L"PhCmdTool", ULONG_MAX, __ImageBase, 0, 0)))
        return EXIT_FAILURE;

    PhUnicodeStringToStringRef(&NtCurrentPeb()->ProcessParameters->CommandLine, &commandLine);
    PhParseCommandLine(
        &commandLine,
        options,
        RTL_NUMBER_OF(options),
        PH_COMMAND_LINE_IGNORE_FIRST_PART,
        PhpCommandLineCallback,
        NULL
        );

    if (!CommandMode)
    {
        wprintf(
            L"%s",
            L"Command line options:\n\n"
            L"-c\n"
            L"-ctype command-type\n"
            L"-cobject command-object\n"
            L"-caction command-action\n"
            L"-cvalue command-value\n"
            );
        return 1;
    }

    return PhCommandModeStart();
}
