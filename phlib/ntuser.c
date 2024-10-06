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
#include <user.h>

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
    _Out_writes_(ProcessCount) PULONG ProcessList,
    _In_ ULONG ProcessCount
    )
{
    NTSTATUS status;

    status = PhAttachConsole(ProcessId);

    if (NT_SUCCESS(status))
    {
        if (GetConsoleProcessList(ProcessList, ProcessCount))
            status = STATUS_SUCCESS;
        else
            status = PhGetLastWin32ErrorAsNtStatus();
    }

    PhFreeConsole();

    return status;
}

NTSTATUS PhConsoleControlSetForeground(
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

NTSTATUS PhConsoleControlEndTask(
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
