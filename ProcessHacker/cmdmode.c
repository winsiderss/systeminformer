/*
 * Process Hacker - 
 *   command line action mode
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

#include <phgui.h>

static HWND CommandModeWindowHandle;
static PPH_STRING RunAsServiceName = NULL;

#define PH_COMMAND_OPTION_HWND 1
#define PH_COMMAND_OPTION_RUNASSERVICENAME 2 

BOOLEAN NTAPI PhpCommandModeOptionCallback(
    __in_opt PPH_COMMAND_LINE_OPTION Option,
    __in_opt PPH_STRING Value,
    __in PVOID Context
    )
{
    ULONG64 integer;

    if (Option)
    {
        switch (Option->Id)
        {
        case PH_COMMAND_OPTION_HWND:
            if (PhStringToInteger64(Value->Buffer, 10, &integer))
                CommandModeWindowHandle = (HWND)integer;
            break;
        case PH_COMMAND_OPTION_RUNASSERVICENAME:
            PhSwapReference(&RunAsServiceName, Value);
            break;
        }
    }

    return TRUE;
}

NTSTATUS PhCommandModeStart()
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { PH_COMMAND_OPTION_HWND, L"hwnd", MandatoryArgumentType },
        { PH_COMMAND_OPTION_RUNASSERVICENAME, L"servicename", MandatoryArgumentType }
    };
    NTSTATUS status = STATUS_SUCCESS;
    PH_STRINGREF commandLine;

    commandLine.us = NtCurrentPeb()->ProcessParameters->CommandLine;

    PhParseCommandLine(
        &commandLine,
        options,
        sizeof(options) / sizeof(PH_COMMAND_LINE_OPTION),
        PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS,
        PhpCommandModeOptionCallback,
        NULL
        );

    if (PhStringEquals2(PhStartupParameters.CommandType, L"processhacker", TRUE))
    {
        if (PhStringEquals2(PhStartupParameters.CommandAction, L"runas", TRUE))
        {
            if (!RunAsServiceName || !PhStartupParameters.CommandObject)
                return STATUS_INVALID_PARAMETER;

            status = PhRunAsCommandStart(
                PhStartupParameters.CommandObject->Buffer,
                RunAsServiceName->Buffer
                );
        }
    }

    return status;
}
