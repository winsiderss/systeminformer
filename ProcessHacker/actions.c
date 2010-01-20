/*
 * Process Hacker - 
 *   UI actions
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
#include <settings.h>
#include <kph.h>

PWSTR DangerousProcesses[] =
{
    L"csrss.exe", L"dwm.exe", L"logonui.exe", L"lsass.exe", L"lsm.exe",
    L"services.exe", L"smss.exe", L"wininit.exe", L"winlogon.exe"
};

static BOOLEAN PhpIsDangerousProcess(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    PPH_STRING fileName;
    PPH_STRING systemDirectory;
    ULONG i;

    if (ProcessId == SYSTEM_PROCESS_ID)
        return TRUE;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        ProcessQueryAccess,
        ProcessId
        )))
        return FALSE;

    status = PhGetProcessImageFileName(processHandle, &fileName);
    CloseHandle(processHandle);

    if (!NT_SUCCESS(status))
        return FALSE;

    PhaDereferenceObject(fileName);
    fileName = PHA_DEREFERENCE(PhGetFileName(fileName));

    systemDirectory = PHA_DEREFERENCE(PhGetSystemDirectory());

    for (i = 0; i < sizeof(DangerousProcesses) / sizeof(PWSTR); i++)
    {
        PPH_STRING fullName;

        fullName = PhaConcatStrings(3, systemDirectory->Buffer, L"\\", DangerousProcesses[i]);

        if (PhStringEquals(fileName, fullName, TRUE))
            return TRUE;
    }

    return FALSE;
}

static BOOLEAN PhpShowContinueMessageProcesses(
    __in HWND hWnd,
    __in PWSTR Verb,
    __in PWSTR Message,
    __in BOOLEAN WarnOnlyIfDangerous,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    )
{
    PWSTR object;
    ULONG i;
    BOOLEAN dangerous = FALSE;
    BOOLEAN cont = FALSE;

    if (NumberOfProcesses == 0)
        return FALSE;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        if (PhpIsDangerousProcess(Processes[i]->ProcessId))
        {
            dangerous = TRUE;
            break;
        }
    }

    if (WarnOnlyIfDangerous && !dangerous)
        return TRUE;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        if (NumberOfProcesses == 1)
        {
            object = Processes[0]->ProcessName->Buffer;
        }
        else if (NumberOfProcesses == 2)
        {
            object = PhaConcatStrings(
                3,
                Processes[0]->ProcessName->Buffer,
                L" and ",
                Processes[1]->ProcessName->Buffer
                )->Buffer;
        }
        else
        {
            object = L"the selected processes";
        }

        if (!dangerous)
        {
            cont = PhShowConfirmMessage(
                hWnd,
                Verb,
                object,
                Message,
                FALSE
                );
        }
        else
        {
            cont = PhShowConfirmMessage(
                hWnd,
                Verb,
                object,
                PhaConcatStrings(
                3,
                L"You are about to ",
                Verb,
                L" one or more system processes."
                )->Buffer,
                TRUE
                );
        }
    }
    else
    {
        cont = TRUE;
    }

    return cont;
}

static BOOLEAN PhpShowErrorProcess(
    __in HWND hWnd,
    __in PWSTR Verb,
    __in PPH_PROCESS_ITEM Process,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    )
{
    if (
        Process->ProcessId != DPCS_PROCESS_ID &&
        Process->ProcessId != INTERRUPTS_PROCESS_ID
        )
    {
        return PhShowContinueStatus(
            hWnd,
            PhaFormatString(
            L"Unable to %s %s (PID %u)",
            Verb,
            Process->ProcessName->Buffer,
            (ULONG)Process->ProcessId
            )->Buffer,
            Status,
            Win32Result
            );
    }
    else
    {
        return PhShowContinueStatus(
            hWnd,
            PhaFormatString(
            L"Unable to %s %s",
            Verb,
            Process->ProcessName->Buffer
            )->Buffer,
            Status,
            Win32Result
            );
    }
}

BOOLEAN PhUiTerminateProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    if (!PhpShowContinueMessageProcesses(
        hWnd,
        L"terminate",
        L"Terminating a process will cause unsaved data to be lost.",
        FALSE,
        Processes,
        NumberOfProcesses
        ))
        return FALSE;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        HANDLE processHandle;

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_TERMINATE,
            Processes[i]->ProcessId
            )))
        {
            status = PhTerminateProcess(processHandle, STATUS_SUCCESS);
            CloseHandle(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorProcess(hWnd, L"terminate", Processes[i], status, 0))
                break;
        }
    }

    return success;
}

BOOLEAN PhUiSuspendProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    if (!PhpShowContinueMessageProcesses(
        hWnd,
        L"suspend",
        NULL,
        TRUE,
        Processes,
        NumberOfProcesses
        ))
        return FALSE;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        HANDLE processHandle;

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_SUSPEND_RESUME,
            Processes[i]->ProcessId
            )))
        {
            status = PhSuspendProcess(processHandle);
            CloseHandle(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorProcess(hWnd, L"suspend", Processes[i], status, 0))
                break;
        }
    }

    return success;
}

BOOLEAN PhUiResumeProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    if (!PhpShowContinueMessageProcesses(
        hWnd,
        L"resume",
        NULL,
        TRUE,
        Processes,
        NumberOfProcesses
        ))
        return FALSE;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        HANDLE processHandle;

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_SUSPEND_RESUME,
            Processes[i]->ProcessId
            )))
        {
            status = PhResumeProcess(processHandle);
            CloseHandle(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorProcess(hWnd, L"resume", Processes[i], status, 0))
                break;
        }
    }

    return success;
}

BOOLEAN PhUiRestartProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    ULONG win32Result = 0;
    BOOLEAN cont = FALSE;
    HANDLE processHandle = NULL;
    BOOLEAN isPosix;
    PPH_STRING commandLine;
    PPH_STRING currentDirectory;
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInformation;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            hWnd,
            L"restart",
            Process->ProcessName->Buffer,
            L"The process will be restarted with the same command line and " 
            L"working directory, but if it is running under a different user it " 
            L"will be restarted under the current user.",
            TRUE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    // Open the process and get the command line and current directory.

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        ProcessQueryAccess | PROCESS_VM_READ,
        Process->ProcessId
        )))
        goto ErrorExit;

    if (!NT_SUCCESS(status = PhGetProcessIsPosix(processHandle, &isPosix)))
        goto ErrorExit;

    if (isPosix)
    {
        PhShowError(hWnd, L"POSIX processes cannot be restarted.");
        goto ErrorExit;
    }

    if (!NT_SUCCESS(status = PhGetProcessCommandLine(
        processHandle,
        &commandLine
        )))
        goto ErrorExit;

    PhaDereferenceObject(commandLine);

    if (!NT_SUCCESS(status = PhGetProcessPebString(
        processHandle,
        PhpoCurrentDirectory,
        &currentDirectory
        )))
        goto ErrorExit;

    PhaDereferenceObject(currentDirectory);

    CloseHandle(processHandle);
    processHandle = NULL;

    // Open the process and terminate it.

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_TERMINATE,
        Process->ProcessId
        )))
        goto ErrorExit;

    if (!NT_SUCCESS(status = PhTerminateProcess(
        processHandle,
        STATUS_SUCCESS
        )))
        goto ErrorExit;

    CloseHandle(processHandle);
    processHandle = NULL;

    // Start the process.

    memset(&startupInfo, 0, sizeof(STARTUPINFO));
    startupInfo.cb = sizeof(STARTUPINFO);

    if (!CreateProcess(
        NULL,
        commandLine->Buffer,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        currentDirectory->Buffer,
        &startupInfo,
        &processInformation
        ))
    {
        win32Result = GetLastError();
        goto ErrorExit;
    }

    CloseHandle(processInformation.hProcess);
    CloseHandle(processInformation.hThread);

ErrorExit:
    if (processHandle)
        CloseHandle(processHandle);

    if (!NT_SUCCESS(status) || win32Result)
    {
        PhpShowErrorProcess(hWnd, L"restart", Process, status, win32Result);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiReduceWorkingSetProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        ULONG win32Result = 0;
        HANDLE processHandle;

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_SET_QUOTA,
            Processes[i]->ProcessId
            )))
        {
            if (!SetProcessWorkingSetSize(processHandle, -1, -1))
                win32Result = GetLastError();

            CloseHandle(processHandle);
        }

        if (!NT_SUCCESS(status) || win32Result)
        {
            success = FALSE;

            if (!PhpShowErrorProcess(hWnd, L"reduce the working set of", Processes[i], status, win32Result))
                break;
        }
    }

    return success;
}

BOOLEAN PhUiSetVirtualizationProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in BOOLEAN Enable
    )
{
    NTSTATUS status;
    BOOLEAN cont = FALSE;
    HANDLE processHandle;
    HANDLE tokenHandle;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            hWnd,
            L"set",
            L"virtualization for the process",
            L"Enabling or disabling virtualization for a process may " 
            L"alter its functionality and produce undesirable effects.",
            FALSE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        ProcessQueryAccess,
        Process->ProcessId
        )))
    {
        if (NT_SUCCESS(status = PhOpenProcessToken(
            &tokenHandle,
            TOKEN_WRITE,
            processHandle
            )))
        {
            status = PhSetTokenIsVirtualizationEnabled(tokenHandle, Enable);

            CloseHandle(tokenHandle);
        }

        CloseHandle(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(hWnd, L"set virtualization for", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiDetachFromDebuggerProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE debugObjectHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME,
        Process->ProcessId
        )))
    {
        if (NT_SUCCESS(status = PhGetProcessDebugObject(
            processHandle,
            &debugObjectHandle
            )))
        {
            status = NtRemoveProcessDebug(processHandle, debugObjectHandle);

            CloseHandle(debugObjectHandle);
        }

        CloseHandle(processHandle);
    }

    if (status == STATUS_PORT_NOT_SET)
    {
        PhShowInformation(hWnd, L"The process is not being debugged.");
        return FALSE;
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(hWnd, L"detach debugger from", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiInjectDllProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    )
{
    PH_FILETYPE_FILTER filters[] =
    {
        { L"DLL files (*.dll)", L"*.dll" },
        { L"All files (*.*)", L"*.*" }
    };

    NTSTATUS status;
    PVOID fileDialog;
    PPH_STRING fileName;
    HANDLE processHandle;

    fileDialog = PhCreateOpenFileDialog();
    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

    if (!PhShowFileDialog(hWnd, fileDialog))
    {
        PhFreeFileDialog(fileDialog);
        return FALSE;
    }

    fileName = PHA_DEREFERENCE(PhGetFileDialogFileName(fileDialog));
    PhFreeFileDialog(fileDialog);

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE,
        Process->ProcessId
        )))
    {
        status = PhInjectDllProcess(
            processHandle,
            fileName->Buffer,
            5000
            );

        CloseHandle(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(hWnd, L"inject the DLL into", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiSetIoPriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG IoPriority
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (!PhKphHandle)
    {
        PhShowError(hWnd, KPH_ERROR_MESSAGE);
        return FALSE;
    }

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        Process->ProcessId
        )))
    {
        status = PhSetProcessIoPriority(processHandle, IoPriority);

        CloseHandle(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(hWnd, L"set the I/O priority of", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiSetPriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG PriorityClassWin32
    )
{
    NTSTATUS status;
    ULONG win32Result = 0;
    HANDLE processHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        Process->ProcessId
        )))
    {
        if (!SetPriorityClass(processHandle, PriorityClassWin32))
            win32Result = GetLastError();

        CloseHandle(processHandle);
    }

    if (!NT_SUCCESS(status) || win32Result)
    {
        PhpShowErrorProcess(hWnd, L"set the priority of", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN PhpShowContinueMessageThreads(
    __in HWND hWnd,
    __in PWSTR Verb,
    __in PWSTR Message,
    __in BOOLEAN Warning,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    )
{
    PWSTR object;
    BOOLEAN cont = FALSE;

    if (NumberOfThreads == 0)
        return FALSE;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        if (NumberOfThreads == 1)
        {
            object = L"the selected thread";
        }
        else
        {
            object = L"the selected threads";
        }

        cont = PhShowConfirmMessage(
            hWnd,
            Verb,
            object,
            Message,
            Warning
            );
    }
    else
    {
        cont = TRUE;
    }

    return cont;
}

static BOOLEAN PhpShowErrorThread(
    __in HWND hWnd,
    __in PWSTR Verb,
    __in PPH_THREAD_ITEM Thread,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    )
{
    return PhShowContinueStatus(
        hWnd,
        PhaFormatString(
        L"Unable to %s thread %u",
        Verb,
        (ULONG)Thread->ThreadId
        )->Buffer,
        Status,
        Win32Result
        );
}

BOOLEAN PhUiTerminateThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    if (!PhpShowContinueMessageThreads(
        hWnd,
        L"terminate",
        L"Terminating a thread may cause the process to stop working.",
        FALSE,
        Threads,
        NumberOfThreads
        ))
        return FALSE;

    for (i = 0; i < NumberOfThreads; i++)
    {
        NTSTATUS status;
        HANDLE threadHandle;

        if (NT_SUCCESS(status = PhOpenThread(
            &threadHandle,
            THREAD_TERMINATE,
            Threads[i]->ThreadId
            )))
        {
            status = PhTerminateThread(threadHandle, STATUS_SUCCESS);
            CloseHandle(threadHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorThread(hWnd, L"terminate", Threads[i], status, 0))
                break;
        }
    }

    return success;
}

BOOLEAN PhUiForceTerminateThreads(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    if (!PhKphHandle)
    {
        PhShowError(hWnd, KPH_ERROR_MESSAGE);
        return FALSE;
    }

    if (ProcessId != SYSTEM_PROCESS_ID)
    {
        if (!PhpShowContinueMessageThreads(
            hWnd,
            L"force terminate",
            L"Forcibly terminating threads may cause the system to crash.",
            TRUE,
            Threads,
            NumberOfThreads
            ))
            return FALSE;
    }
    else
    {
        if (!PhpShowContinueMessageThreads(
            hWnd,
            L"terminate",
            L"Forcibly terminating system threads may cause the system to crash.",
            TRUE,
            Threads,
            NumberOfThreads
            ))
            return FALSE;
    }

    for (i = 0; i < NumberOfThreads; i++)
    {
        NTSTATUS status;
        HANDLE threadHandle;

        if (NT_SUCCESS(status = PhOpenThread(
            &threadHandle,
            THREAD_TERMINATE,
            Threads[i]->ThreadId
            )))
        {
            status = KphDangerousTerminateThread(PhKphHandle, threadHandle, STATUS_SUCCESS);
            CloseHandle(threadHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorThread(hWnd, L"terminate", Threads[i], status, 0))
                break;
        }
    }

    return success;
}

BOOLEAN PhUiSuspendThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    for (i = 0; i < NumberOfThreads; i++)
    {
        NTSTATUS status;
        HANDLE threadHandle;

        if (NT_SUCCESS(status = PhOpenThread(
            &threadHandle,
            THREAD_SUSPEND_RESUME,
            Threads[i]->ThreadId
            )))
        {
            status = PhSuspendThread(threadHandle, NULL);
            CloseHandle(threadHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorThread(hWnd, L"suspend", Threads[i], status, 0))
                break;
        }
    }

    return success;
}

BOOLEAN PhUiResumeThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    for (i = 0; i < NumberOfThreads; i++)
    {
        NTSTATUS status;
        HANDLE threadHandle;

        if (NT_SUCCESS(status = PhOpenThread(
            &threadHandle,
            THREAD_SUSPEND_RESUME,
            Threads[i]->ThreadId
            )))
        {
            status = PhResumeThread(threadHandle, NULL);
            CloseHandle(threadHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorThread(hWnd, L"resume", Threads[i], status, 0))
                break;
        }
    }

    return success;
}

BOOLEAN PhUiSetPriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG ThreadPriorityWin32
    )
{
    NTSTATUS status;
    ULONG win32Result = 0;
    HANDLE threadHandle;

    if (NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        ThreadSetAccess,
        Thread->ThreadId
        )))
    {
        if (!SetThreadPriority(threadHandle, ThreadPriorityWin32))
            win32Result = GetLastError();

        CloseHandle(threadHandle);
    }

    if (!NT_SUCCESS(status) || win32Result)
    {
        PhpShowErrorThread(hWnd, L"set the priority of", Thread, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiSetIoPriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG IoPriority
    )
{
    NTSTATUS status;
    HANDLE threadHandle;

    if (!PhKphHandle)
    {
        PhShowError(hWnd, KPH_ERROR_MESSAGE);
        return FALSE;
    }

    if (NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        ThreadSetAccess,
        Thread->ThreadId
        )))
    {
        status = PhSetThreadIoPriority(threadHandle, IoPriority);

        CloseHandle(threadHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorThread(hWnd, L"set the I/O priority of", Thread, status, 0);
        return FALSE;
    }

    return TRUE;
}
