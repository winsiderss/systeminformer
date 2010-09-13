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
static PPH_STRING OptionRunAsServiceName = NULL;
static HANDLE OptionProcessId = NULL;
static PVOID OptionBaseAddress = NULL;

#define PH_COMMAND_OPTION_HWND 1
#define PH_COMMAND_OPTION_RUNASSERVICENAME 2
#define PH_COMMAND_OPTION_PROCESSID 3
#define PH_COMMAND_OPTION_BASEADDRESS 4

BOOLEAN NTAPI PhpCommandModeOptionCallback(
    __in_opt PPH_COMMAND_LINE_OPTION Option,
    __in_opt PPH_STRING Value,
    __in_opt PVOID Context
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
            PhSwapReference(&OptionRunAsServiceName, Value);
            break;
        case PH_COMMAND_OPTION_PROCESSID:
            if (PhStringToInteger64(&Value->sr, 10, &integer))
                OptionProcessId = (HANDLE)integer;
            break;
        case PH_COMMAND_OPTION_BASEADDRESS:
            if (PhStringToInteger64(&Value->sr, 0, &integer))
                OptionBaseAddress = (PVOID)integer;
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
        { PH_COMMAND_OPTION_RUNASSERVICENAME, L"servicename", MandatoryArgumentType },
        { PH_COMMAND_OPTION_PROCESSID, L"processid", MandatoryArgumentType },
        { PH_COMMAND_OPTION_BASEADDRESS, L"baseaddress", MandatoryArgumentType }
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
            if (!OptionRunAsServiceName || !PhStartupParameters.CommandObject)
                return STATUS_INVALID_PARAMETER;

            status = PhRunAsCommandStart(
                PhStartupParameters.CommandObject->Buffer,
                OptionRunAsServiceName->Buffer
                );
        }
        else if (PhEqualString2(PhStartupParameters.CommandAction, L"unloaddriver", TRUE))
        {
            if (!OptionBaseAddress || !PhStartupParameters.CommandObject)
                return STATUS_INVALID_PARAMETER;

            status = PhUnloadDriver(OptionBaseAddress, PhStartupParameters.CommandObject->Buffer);
        }
    }
    else if (PhEqualString2(PhStartupParameters.CommandType, L"process", TRUE))
    {
        ULONG64 processId64;
        HANDLE processId;
        HANDLE processHandle;

        if (!PhStartupParameters.CommandObject)
            return STATUS_INVALID_PARAMETER;

        if (!PhStringToInteger64(&PhStartupParameters.CommandObject->sr, 10, &processId64))
            return STATUS_INVALID_PARAMETER;

        processId = (HANDLE)processId64;

        if (PhEqualString2(PhStartupParameters.CommandAction, L"terminate", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_TERMINATE, processId)))
            {
                status = PhTerminateProcess(processHandle, STATUS_SUCCESS);
                NtClose(processHandle);
            }
        }
        else if (PhEqualString2(PhStartupParameters.CommandAction, L"suspend", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SUSPEND_RESUME, processId)))
            {
                status = PhSuspendProcess(processHandle);
                NtClose(processHandle);
            }
        }
        else if (PhEqualString2(PhStartupParameters.CommandAction, L"resume", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SUSPEND_RESUME, processId)))
            {
                status = PhResumeProcess(processHandle);
                NtClose(processHandle);
            }
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
    else if (PhEqualString2(PhStartupParameters.CommandType, L"thread", TRUE))
    {
        ULONG64 threadId64;
        HANDLE threadId;
        HANDLE threadHandle;

        if (!PhStartupParameters.CommandObject)
            return STATUS_INVALID_PARAMETER;

        if (!PhStringToInteger64(&PhStartupParameters.CommandObject->sr, 10, &threadId64))
            return STATUS_INVALID_PARAMETER;

        threadId = (HANDLE)threadId64;

        if (PhEqualString2(PhStartupParameters.CommandAction, L"terminate", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_TERMINATE, threadId)))
            {
                status = PhTerminateThread(threadHandle, STATUS_SUCCESS);
                NtClose(threadHandle);
            }
        }
        else if (PhEqualString2(PhStartupParameters.CommandAction, L"suspend", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_SUSPEND_RESUME, threadId)))
            {
                status = PhSuspendThread(threadHandle, NULL);
                NtClose(threadHandle);
            }
        }
        else if (PhEqualString2(PhStartupParameters.CommandAction, L"resume", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_SUSPEND_RESUME, threadId)))
            {
                status = PhResumeThread(threadHandle, NULL);
                NtClose(threadHandle);
            }
        }
    }

    return status;
}
