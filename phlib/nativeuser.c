/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2024
 *
 */

#include <ph.h>
#include <apiimport.h>
#include <ntuser.h>
#include <phconsole.h>

NTSTATUS PhAttachConsole(
    _In_ HANDLE ProcessId
    )
{
    PhFreeConsole();

    if (AttachConsole(HandleToUlong(ProcessId)))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

VOID PhFreeConsole(
    VOID
    )
{
    PRTL_USER_PROCESS_PARAMETERS processParameters;

    FreeConsole();

    // Note: FreeConsole leaks the StandardInput, StandardOutput, StandardError and ConsoleHandle.
    // The console API is not thread-safe and these values must be set to null or we run into issues
    // with all the other console functions since they continue using these values, we also run into
    // issues with strict handle checking. (dmex)

    processParameters = NtCurrentPeb()->ProcessParameters;
    processParameters->StandardInput = NULL;
    processParameters->StandardOutput = NULL;
    processParameters->StandardError = NULL;
}

HANDLE PhGetStdHandle(
    _In_ ULONG StdHandle
    )
{
    if (WindowsVersion < WINDOWS_NEW)
    {
        switch (StdHandle)
        {
        case STD_INPUT_HANDLE:
            return NtCurrentPeb()->ProcessParameters->StandardInput;
        case STD_OUTPUT_HANDLE:
            return NtCurrentPeb()->ProcessParameters->StandardOutput;
        case STD_ERROR_HANDLE:
            return NtCurrentPeb()->ProcessParameters->StandardError;
        }

        return INVALID_HANDLE_VALUE;
    }
    else
    {
        return GetStdHandle(StdHandle);
    }
}

NTSTATUS PhGetConsoleProcessList(
    _In_ HANDLE ProcessId,
    _In_ ULONG ProcessCount,
    _Out_writes_(ProcessCount) PULONG ProcessList,
    _Out_ PULONG ProcessTotal
    )
{
    NTSTATUS status;

    status = PhAttachConsole(ProcessId);

    if (NT_SUCCESS(status))
    {
        if (*ProcessTotal = GetConsoleProcessList(ProcessList, ProcessCount))
            status = STATUS_SUCCESS;
        else
            status = PhGetLastWin32ErrorAsNtStatus();
    }

    PhFreeConsole();

    return status;
}

NTSTATUS PhConsoleSetForeground(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN Foreground
    )
{
    NTSTATUS status;
    CONSOLESETFOREGROUND consoleInfo;

    if (!ConsoleControl_Import())
        return STATUS_NOT_SUPPORTED;

    consoleInfo.ProcessHandle = ProcessHandle;
    consoleInfo.Foreground = !!Foreground;

    status = ConsoleControl_Import()(
        ConsoleSetForeground,
        &consoleInfo,
        sizeof(CONSOLESETFOREGROUND)
        );

    return status;
}

NTSTATUS PhConsoleNotifyWindow(
    _In_ HANDLE ProcessID
    )
{
    NTSTATUS status;
    CONSOLE_PROCESS_INFO consoleProcessInfo;

    if (!ConsoleControl_Import())
        return STATUS_NOT_SUPPORTED;

    consoleProcessInfo.Flags = CPI_NEWPROCESSWINDOW;
    consoleProcessInfo.ProcessID = HandleToUlong(ProcessID);

    status = ConsoleControl_Import()(
        ConsoleNotifyConsoleApplication,
        &consoleProcessInfo,
        sizeof(CONSOLE_PROCESS_INFO)
        );

    return status;
}

NTSTATUS PhConsoleSetWindow(
    _In_ HANDLE ProcessID,
    _In_ HANDLE ThreadId,
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;
    CONSOLEWINDOWOWNER consoleInfo;

    if (!ConsoleControl_Import())
        return STATUS_NOT_SUPPORTED;

    consoleInfo.ProcessId = HandleToUlong(ProcessID);
    consoleInfo.ThreadId = HandleToUlong(ThreadId);
    consoleInfo.WindowHandle = WindowHandle;

    status = ConsoleControl_Import()(
        ConsoleSetWindowOwner,
        &consoleInfo,
        sizeof(CONSOLEWINDOWOWNER)
        );

    return status;
}

NTSTATUS PhConsoleEndTask(
    _In_ HANDLE ProcessId,
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;
    CONSOLEENDTASK consoleInfo;

    if (!ConsoleControl_Import())
        return STATUS_NOT_SUPPORTED;

    consoleInfo.ProcessId = ProcessId;
    consoleInfo.WindowHandle = WindowHandle;
    consoleInfo.ConsoleEventCode = CTRL_C_EVENT;
    consoleInfo.ConsoleFlags = 0;

    status = ConsoleControl_Import()(
        ConsoleEndTask,
        &consoleInfo,
        sizeof(CONSOLEENDTASK)
        );

    return status;
}
