/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2012
 *     dmex    2020-2022
 *
 */

#include <ph.h>
#include <svcsup.h>

#define PH_COMMAND_OPTION_HWND 1
#define PH_ARG_COMMANDMODE 5
#define PH_ARG_COMMANDTYPE 6
#define PH_ARG_COMMANDOBJECT 7
#define PH_ARG_COMMANDACTION 8
#define PH_ARG_COMMANDVALUE 9
#define PH_ARG_PRIORITY 25

EXTERN_C PVOID __ImageBase;
ULONG CommandMode = 0;
PPH_STRING CommandType = NULL;
PPH_STRING CommandObject = NULL;
PPH_STRING CommandAction = NULL;
PPH_STRING CommandValue = NULL;
HWND CommandModeWindowHandle = NULL;

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
        case PH_COMMAND_OPTION_HWND:
            {
                ULONG64 integer;

                if (Value && PhStringToInteger64(&Value->sr, 10, &integer))
                    CommandModeWindowHandle = (HWND)integer;
            }
            break;
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

UINT32 PhCommandModeStart(
    VOID
    )
{
    PH_COMMAND_LINE_OPTION options[] =
    {
        { PH_ARG_COMMANDMODE, L"c", NoArgumentType },
        { PH_ARG_COMMANDTYPE, L"ctype", MandatoryArgumentType },
        { PH_ARG_COMMANDOBJECT, L"cobject", MandatoryArgumentType },
        { PH_ARG_COMMANDACTION, L"caction", MandatoryArgumentType },
        { PH_ARG_COMMANDVALUE, L"cvalue", MandatoryArgumentType },
        { PH_COMMAND_OPTION_HWND, L"hwnd", MandatoryArgumentType }
    };
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PH_STRINGREF commandLine;

    status = PhInitializePhLib(L"PhCmdTool", __ImageBase);

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcessCommandLineStringRef(&commandLine);

    if (!NT_SUCCESS(status))
        return status;

    if (!PhParseCommandLine(
        &commandLine,
        options,
        RTL_NUMBER_OF(options),
        PH_COMMAND_LINE_IGNORE_FIRST_PART,
        PhpCommandLineCallback,
        NULL
        ))
    {
        return STATUS_UNSUCCESSFUL;
    }

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
        return STATUS_UNSUCCESSFUL;
    }

    if (PhEqualString2(CommandType, L"process", TRUE))
    {
        SIZE_T i;
        SIZE_T processIdLength;
        HANDLE processId;
        HANDLE processHandle;

        if (!CommandObject)
            return STATUS_INVALID_PARAMETER;

        processIdLength = CommandObject->Length / sizeof(WCHAR);

        for (i = 0; i < processIdLength; i++)
        {
            if (!PhIsDigitCharacter(CommandObject->Buffer[i]))
                break;
        }

        if (i == processIdLength)
        {
            ULONG64 processId64;

            if (!PhStringToInteger64(&CommandObject->sr, 10, &processId64))
                return STATUS_INVALID_PARAMETER;

            processId = (HANDLE)processId64;
        }
        else
        {
            PVOID processes;
            PSYSTEM_PROCESS_INFORMATION process;

            if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
                return status;

            if (!(process = PhFindProcessInformationByImageName(processes, &CommandObject->sr)))
            {
                PhFree(processes);
                return STATUS_NOT_FOUND;
            }

            processId = process->UniqueProcessId;
            PhFree(processes);
        }

        if (PhEqualString2(CommandAction, L"terminate", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_TERMINATE, processId)))
            {
                status = NtTerminateProcess(processHandle, STATUS_SUCCESS);
                NtClose(processHandle);
            }
        }
        else if (PhEqualString2(CommandAction, L"suspend", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SUSPEND_RESUME, processId)))
            {
                status = NtSuspendProcess(processHandle);
                NtClose(processHandle);
            }
        }
        else if (PhEqualString2(CommandAction, L"resume", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SUSPEND_RESUME, processId)))
            {
                status = NtResumeProcess(processHandle);
                NtClose(processHandle);
            }
        }
        else if (PhEqualString2(CommandAction, L"priority", TRUE))
        {
            UCHAR priority;

            if (!CommandValue)
                return STATUS_INVALID_PARAMETER;

            if (PhEqualString2(CommandValue, L"idle", TRUE))
                priority = PROCESS_PRIORITY_CLASS_IDLE;
            else if (PhEqualString2(CommandValue, L"normal", TRUE))
                priority = PROCESS_PRIORITY_CLASS_NORMAL;
            else if (PhEqualString2(CommandValue, L"high", TRUE))
                priority = PROCESS_PRIORITY_CLASS_HIGH;
            else if (PhEqualString2(CommandValue, L"realtime", TRUE))
                priority = PROCESS_PRIORITY_CLASS_REALTIME;
            else if (PhEqualString2(CommandValue, L"abovenormal", TRUE))
                priority = PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
            else if (PhEqualString2(CommandValue, L"belownormal", TRUE))
                priority = PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
            else
                return STATUS_INVALID_PARAMETER;

            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SET_INFORMATION, processId)))
            {
                status = PhSetProcessPriority(processHandle, priority);

                NtClose(processHandle);
            }
        }
        else if (PhEqualString2(CommandAction, L"iopriority", TRUE))
        {
            ULONG ioPriority;

            if (!CommandValue)
                return STATUS_INVALID_PARAMETER;

            if (PhEqualString2(CommandValue, L"verylow", TRUE))
                ioPriority = 0;
            else if (PhEqualString2(CommandValue, L"low", TRUE))
                ioPriority = 1;
            else if (PhEqualString2(CommandValue, L"normal", TRUE))
                ioPriority = 2;
            else if (PhEqualString2(CommandValue, L"high", TRUE))
                ioPriority = 3;
            else
                return STATUS_INVALID_PARAMETER;

            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SET_INFORMATION, processId)))
            {
                status = PhSetProcessIoPriority(processHandle, ioPriority);
                NtClose(processHandle);
            }
        }
        else if (PhEqualString2(CommandAction, L"pagepriority", TRUE))
        {
            ULONG64 pagePriority64;
            ULONG pagePriority;

            if (!CommandValue)
                return STATUS_INVALID_PARAMETER;

            PhStringToInteger64(&CommandValue->sr, 10, &pagePriority64);
            pagePriority = (ULONG)pagePriority64;

            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SET_INFORMATION, processId)))
            {
                status = PhSetProcessPagePriority(processHandle, pagePriority);

                NtClose(processHandle);
            }
        }
    }
    else if (PhEqualString2(CommandType, L"service", TRUE))
    {
        SC_HANDLE serviceHandle;
        SERVICE_STATUS serviceStatus;

        if (!CommandObject)
            return STATUS_INVALID_PARAMETER;

        if (PhEqualString2(CommandAction, L"start", TRUE))
        {
            if (!(serviceHandle = PhOpenService(
                CommandObject->Buffer,
                SERVICE_START
                )))
                return PhGetLastWin32ErrorAsNtStatus();

            if (!StartService(serviceHandle, 0, NULL))
                status = PhGetLastWin32ErrorAsNtStatus();

            CloseServiceHandle(serviceHandle);
        }
        else if (PhEqualString2(CommandAction, L"continue", TRUE))
        {
            if (!(serviceHandle = PhOpenService(
                CommandObject->Buffer,
                SERVICE_PAUSE_CONTINUE
                )))
                return PhGetLastWin32ErrorAsNtStatus();

            if (!ControlService(serviceHandle, SERVICE_CONTROL_CONTINUE, &serviceStatus))
                status = PhGetLastWin32ErrorAsNtStatus();

            CloseServiceHandle(serviceHandle);
        }
        else if (PhEqualString2(CommandAction, L"pause", TRUE))
        {
            if (!(serviceHandle = PhOpenService(
                CommandObject->Buffer,
                SERVICE_PAUSE_CONTINUE
                )))
                return PhGetLastWin32ErrorAsNtStatus();

            if (!ControlService(serviceHandle, SERVICE_CONTROL_PAUSE, &serviceStatus))
                status = PhGetLastWin32ErrorAsNtStatus();

            CloseServiceHandle(serviceHandle);
        }
        else if (PhEqualString2(CommandAction, L"stop", TRUE))
        {
            if (!(serviceHandle = PhOpenService(
                CommandObject->Buffer,
                SERVICE_STOP
                )))
                return PhGetLastWin32ErrorAsNtStatus();

            if (!ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus))
                status = PhGetLastWin32ErrorAsNtStatus();

            CloseServiceHandle(serviceHandle);
        }
        else if (PhEqualString2(CommandAction, L"delete", TRUE))
        {
            if (!(serviceHandle = PhOpenService(
                CommandObject->Buffer,
                DELETE
                )))
                return PhGetLastWin32ErrorAsNtStatus();

            if (!DeleteService(serviceHandle))
                status = PhGetLastWin32ErrorAsNtStatus();

            CloseServiceHandle(serviceHandle);
        }
    }
    else if (PhEqualString2(CommandType, L"thread", TRUE))
    {
        ULONG64 threadId64;
        HANDLE threadId;
        HANDLE threadHandle;

        if (!CommandObject)
            return STATUS_INVALID_PARAMETER;

        if (!PhStringToInteger64(&CommandObject->sr, 10, &threadId64))
            return STATUS_INVALID_PARAMETER;

        threadId = (HANDLE)threadId64;

        if (PhEqualString2(CommandAction, L"terminate", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_TERMINATE, threadId)))
            {
                status = NtTerminateThread(threadHandle, STATUS_SUCCESS);
                NtClose(threadHandle);
            }
        }
        else if (PhEqualString2(CommandAction, L"suspend", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_SUSPEND_RESUME, threadId)))
            {
                status = NtSuspendThread(threadHandle, NULL);
                NtClose(threadHandle);
            }
        }
        else if (PhEqualString2(CommandAction, L"resume", TRUE))
        {
            if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_SUSPEND_RESUME, threadId)))
            {
                status = NtResumeThread(threadHandle, NULL);
                NtClose(threadHandle);
            }
        }
    }

    return status;
}

int __cdecl wmain(int argc, wchar_t *argv[])
{
    return PhCommandModeStart();
}
