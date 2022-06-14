/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2012
 *     dmex    2020
 *
 */

#include "main.h"

NTSTATUS PhpGetDllBaseRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF BaseDllName,
    _Out_ PVOID *DllBase
    );

static HWND CommandModeWindowHandle;

#define PH_COMMAND_OPTION_HWND 1

BOOLEAN NTAPI PhpCommandModeOptionCallback(
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
        }
    }

    return TRUE;
}

NTSTATUS PhCommandModeStart(
    VOID
    )
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { PH_COMMAND_OPTION_HWND, L"hwnd", MandatoryArgumentType }
    };
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PH_STRINGREF commandLine;

    PhUnicodeStringToStringRef(&NtCurrentPeb()->ProcessParameters->CommandLine, &commandLine);
    PhParseCommandLine(
        &commandLine,
        options,
        RTL_NUMBER_OF(options),
        PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS,
        PhpCommandModeOptionCallback,
        NULL
        );

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
                PROCESS_PRIORITY_CLASS priorityClass;

                priorityClass.Foreground = FALSE;
                priorityClass.PriorityClass = priority;

                status = PhSetProcessPriority(processHandle, priorityClass);

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

typedef struct _GET_DLL_BASE_REMOTE_CONTEXT
{
    PH_STRINGREF BaseDllName;
    PVOID DllBase;
} GET_DLL_BASE_REMOTE_CONTEXT, *PGET_DLL_BASE_REMOTE_CONTEXT;

static BOOLEAN PhpGetDllBaseRemoteCallback(
    _In_ PLDR_DATA_TABLE_ENTRY Module,
    _In_opt_ PVOID Context
    )
{
    PGET_DLL_BASE_REMOTE_CONTEXT context = Context;
    PH_STRINGREF baseDllName;

    PhUnicodeStringToStringRef(&Module->BaseDllName, &baseDllName);

    if (context && PhEqualStringRef(&baseDllName, &context->BaseDllName, TRUE))
    {
        context->DllBase = Module->DllBase;
        return FALSE;
    }

    return TRUE;
}

NTSTATUS PhpGetDllBaseRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF BaseDllName,
    _Out_ PVOID *DllBase
    )
{
    NTSTATUS status;
    GET_DLL_BASE_REMOTE_CONTEXT context;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    context.BaseDllName = *BaseDllName;
    context.DllBase = NULL;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
        status = PhEnumProcessModules32(ProcessHandle, PhpGetDllBaseRemoteCallback, &context);
    if (!context.DllBase)
#endif
        status = PhEnumProcessModules(ProcessHandle, PhpGetDllBaseRemoteCallback, &context);

    if (NT_SUCCESS(status))
        *DllBase = context.DllBase;

    return status;
}
