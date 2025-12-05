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

/**
 * Attach this process to the console of another process.
 *
 * This function first frees any existing console association for this process by calling
 * `PhFreeConsole` to avoid leaking console handles stored in the process environment block (PEB).
 * It then calls the Win32 `AttachConsole` API using the numeric process identifier derived from
 * `ProcessId`. If `AttachConsole` succeeds, the function returns `STATUS_SUCCESS`. Otherwise
 * it converts the last Win32 error into an NTSTATUS via `PhGetLastWin32ErrorAsNtStatus`.
 * \param[in] ProcessId Handle representing the target process whose console we want to attach to.
 * The handle value is converted to an unsigned long before calling `AttachConsole`.
 * \return NTSTATUS STATUS_SUCCESS on success; translated Win32 error as NTSTATUS on failure.
 */
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

/**
 * Free the current console and clear PEB console-related fields.
 *
 * \remarks The FreeConsole API doesn't zero handles it created and cached in the current
 * process environment block (PEB). It'll close the handle but does not zero the fields.
 * When these values are later recycled by the kernel with a new handle, the values in the PEB
 * are now pointing to valid objects and start getting passed to console functions despite
 * these handles are some other object. This causes all sorts of issues including crashes
 * and data corruption. To avoid these issues, we manually zero the PEB fields.
 */
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
    processParameters->ConsoleHandle = NULL;
}

/**
 * Retrieve a standard I/O handle for the current process.
 *
 * \param[in] StdHandle One of `STD_INPUT_HANDLE`, `STD_OUTPUT_HANDLE` or `STD_ERROR_HANDLE`.
 * \return HANDLE The appropriate standard handle. On older Windows versions, returns the value
 * from the PEB's `ProcessParameters`. If `StdHandle` is invalid on those versions,
 * returns `INVALID_HANDLE_VALUE`. On newer versions, returns the value returned by `GetStdHandle`.
 */
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

/**
 * Query the list of process identifiers attached to a console.
 *
 * \param[in] ProcessId Handle to the process whose console should be queried.
 * \param[in] ProcessCount Number of entries the `ProcessList` buffer can hold.
 * \param[out] ProcessList Buffer that receives the list of process identifiers (ULONG values).
 * \param[out] ProcessTotal Receives the total number of processes attached to the console.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Update the console foreground window for a console process.
 *
 * \param[in] ProcessHandle Handle to the target console process.
 * \param[in] Foreground Non-zero to set the foreground flag; zero to clear it.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Notify the console subsystem that a console application created a new window.
 *
 * \param[in] ProcessID Handle representing the process that created the new window.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Updates the psuedo owner of the console window.
 *
 * \param[in] ProcessID Handle representing the process whose console window owner is being set.
 * \param[in] ThreadId Handle representing the thread associated with the window owner.
 * \param[in] WindowHandle Window handle (HWND) to associate with the console process/thread.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * \brief Request the console subsystem to end a console task (send console event).
 *
 * Builds a `CONSOLEENDTASK` structure and invokes the `ConsoleEndTask` `ConsoleControl` operation.
 * The `ConsoleEventCode` is set to `CTRL_C_EVENT` and `ConsoleFlags` is zeroed.
 *
 * \param[in] ProcessId Handle of the process whose console task should be ended.
 * \param[in] WindowHandle Window handle (HWND) associated with the console to be signaled.
 * \return NTSTATUS Successful or errant status.
 * \remarks The semantics of ending a console task are determined by the console subsystem; callers
 * should ensure they understand the impact of sending `CTRL_C_EVENT` to the target console.
 */
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
