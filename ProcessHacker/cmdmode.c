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

#include <phapp.h>

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
            if (PhStringToInteger64(&Value->sr, 10, &integer))
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

    if (PhEqualString2(PhStartupParameters.CommandType, L"processhacker", TRUE))
    {
        if (PhEqualString2(PhStartupParameters.CommandAction, L"runas", TRUE))
        {
            if (!RunAsServiceName || !PhStartupParameters.CommandObject)
                return STATUS_INVALID_PARAMETER;

            status = PhRunAsCommandStart(
                PhStartupParameters.CommandObject->Buffer,
                RunAsServiceName->Buffer
                );
        }
    }
    else if (PhEqualString2(PhStartupParameters.CommandType, L"service", TRUE))
    {
        SC_HANDLE serviceHandle;
        SERVICE_STATUS serviceStatus;

        if (!PhStartupParameters.CommandObject)
            return STATUS_INVALID_PARAMETER;

        if (PhEqualString2(PhStartupParameters.CommandAction, L"start", TRUE))
        {
            if (!(serviceHandle = PhOpenService(
                PhStartupParameters.CommandObject->Buffer,
                SERVICE_START
                )))
                return NTSTATUS_FROM_WIN32(GetLastError());

            if (!StartService(serviceHandle, 0, NULL))
                status = NTSTATUS_FROM_WIN32(GetLastError());

            CloseServiceHandle(serviceHandle);
        }
        else if (PhEqualString2(PhStartupParameters.CommandAction, L"continue", TRUE))
        {
            if (!(serviceHandle = PhOpenService(
                PhStartupParameters.CommandObject->Buffer,
                SERVICE_PAUSE_CONTINUE
                )))
                return NTSTATUS_FROM_WIN32(GetLastError());

            if (!ControlService(serviceHandle, SERVICE_CONTROL_CONTINUE, &serviceStatus))
                status = NTSTATUS_FROM_WIN32(GetLastError());

            CloseServiceHandle(serviceHandle);
        }
        else if (PhEqualString2(PhStartupParameters.CommandAction, L"pause", TRUE))
        {
            if (!(serviceHandle = PhOpenService(
                PhStartupParameters.CommandObject->Buffer,
                SERVICE_PAUSE_CONTINUE
                )))
                return NTSTATUS_FROM_WIN32(GetLastError());

            if (!ControlService(serviceHandle, SERVICE_CONTROL_PAUSE, &serviceStatus))
                status = NTSTATUS_FROM_WIN32(GetLastError());

            CloseServiceHandle(serviceHandle);
        }
        else if (PhEqualString2(PhStartupParameters.CommandAction, L"stop", TRUE))
        {
            if (!(serviceHandle = PhOpenService(
                PhStartupParameters.CommandObject->Buffer,
                SERVICE_STOP
                )))
                return NTSTATUS_FROM_WIN32(GetLastError());

            if (!ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus))
                status = NTSTATUS_FROM_WIN32(GetLastError());

            CloseServiceHandle(serviceHandle);
        }
        else if (PhEqualString2(PhStartupParameters.CommandAction, L"delete", TRUE))
        {
            if (!(serviceHandle = PhOpenService(
                PhStartupParameters.CommandObject->Buffer,
                DELETE
                )))
                return NTSTATUS_FROM_WIN32(GetLastError());

            if (!DeleteService(serviceHandle))
                status = NTSTATUS_FROM_WIN32(GetLastError());

            CloseServiceHandle(serviceHandle);
        }
    }

    return status;
}
