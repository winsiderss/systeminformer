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

#include <phapp.h>
#include <settings.h>
#include <kph.h>
#include <winsta.h>
#include <iphlpapi.h>
#include <wtsapi32.h>

typedef DWORD (WINAPI *_SetTcpEntry)(
    __in PMIB_TCPROW pTcpRow
    );

static PWSTR DangerousProcesses[] =
{
    L"csrss.exe", L"dwm.exe", L"logonui.exe", L"lsass.exe", L"lsm.exe",
    L"services.exe", L"smss.exe", L"wininit.exe", L"winlogon.exe"
};
static PPH_STRING DebuggerCommand = NULL;
static PH_INITONCE DebuggerCommandInitOnce = PH_INITONCE_INIT;

HRESULT CALLBACK PhpElevateActionCallbackProc(
    __in HWND hwnd,
    __in UINT uNotification,
    __in WPARAM wParam,
    __in LPARAM lParam,
    __in LONG_PTR dwRefData
    )
{
    switch (uNotification)
    {
    case TDN_CREATED:
        SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDYES, TRUE);
        break;
    }

    return S_OK;
}

BOOLEAN PhpShowErrorAndElevateAction(
    __in HWND hWnd,
    __in PWSTR Message,
    __in NTSTATUS Status,
    __in PWSTR Command,
    __out PBOOLEAN Success
    )
{
    PH_ACTION_ELEVATION_LEVEL elevationLevel;
    INT button = IDNO;

    if (
        !(Status == STATUS_ACCESS_DENIED ||
        (NT_NTWIN32(Status) && WIN32_FROM_NTSTATUS(Status) == ERROR_ACCESS_DENIED))
        )
        return FALSE;

    if (!WINDOWS_HAS_UAC || PhElevated)
        return FALSE;

    elevationLevel = PhGetIntegerSetting(L"ElevationLevel");

    if (elevationLevel == NeverElevateAction)
        return FALSE;

    if (elevationLevel == PromptElevateAction && TaskDialogIndirect_I)
    {
        TASKDIALOGCONFIG config = { sizeof(config) };
        TASKDIALOG_BUTTON buttons[1];

        config.hwndParent = hWnd;
        config.hInstance = PhInstanceHandle;
        config.pszWindowTitle = L"Process Hacker";
        config.pszMainIcon = TD_ERROR_ICON;
        config.pszMainInstruction = PhaConcatStrings2(Message, L".")->Buffer;
        /*config.pszContent = PhaConcatStrings(
            3,
            L"Unable to perform the action: ",
            ((PPH_STRING)PHA_DEREFERENCE(PhGetNtMessage(Status)))->Buffer,
            L"\nYou will need to provide administrator permission. "
            L"Click Continue to complete this operation."
            )->Buffer;*/
        config.pszContent = L"You will need to provide administrator permission. "
            L"Click Continue to complete this operation.";
        config.dwCommonButtons = TDCBF_CANCEL_BUTTON;

        buttons[0].nButtonID = IDYES;
        buttons[0].pszButtonText = L"Continue";

        config.cButtons = 1;
        config.pButtons = buttons;
        config.nDefaultButton = IDYES;

        config.pfCallback = PhpElevateActionCallbackProc;

        if (TaskDialogIndirect_I(
            &config,
            &button,
            NULL,
            NULL
            ) != S_OK)
        {
            return FALSE;
        }
    }

    if (elevationLevel == AlwaysElevateAction || button == IDYES)
    {
        NTSTATUS status;
        HANDLE processHandle;
        LARGE_INTEGER timeout;
        PROCESS_BASIC_INFORMATION basicInfo;

        if (PhShellExecuteEx(
            hWnd,
            PhApplicationFileName->Buffer,
            Command,
            SW_SHOW,
            PH_SHELL_EXECUTE_ADMIN,
            0,
            &processHandle
            ))
        {
            timeout.QuadPart = -10 * PH_TIMEOUT_SEC;
            status = NtWaitForSingleObject(processHandle, FALSE, &timeout);

            if (
                status == STATUS_WAIT_0 &&
                NT_SUCCESS(status = PhGetProcessBasicInformation(processHandle, &basicInfo))
                )
            {
                status = basicInfo.ExitStatus; 
            }

            NtClose(processHandle);

            if (NT_SUCCESS(status))
            {
                *Success = TRUE;
            }
            else
            {
                *Success = FALSE;
                PhShowStatus(hWnd, Message, status, 0);
            }
        }
    }

    return TRUE;
}

BOOLEAN PhUiLockComputer(
    __in HWND hWnd
    )
{
    if (LockWorkStation())
        return TRUE;
    else
        PhShowStatus(hWnd, L"Unable to lock the computer", 0, GetLastError());

    return FALSE;
}

BOOLEAN PhUiLogoffComputer(
    __in HWND hWnd
    )
{
    if (ExitWindowsEx(EWX_LOGOFF, 0))
        return TRUE;
    else
        PhShowStatus(hWnd, L"Unable to logoff the computer", 0, GetLastError());

    return FALSE;
}

BOOLEAN PhUiSleepComputer(
    __in HWND hWnd
    )
{
    NTSTATUS status;

    if (NT_SUCCESS(status = NtInitiatePowerAction(
        PowerActionSleep,
        PowerSystemSleeping1,
        0,
        FALSE
        )))
        return TRUE;
    else
        PhShowStatus(hWnd, L"Unable to sleep the computer", status, 0);

    return FALSE;
}

BOOLEAN PhUiHibernateComputer(
    __in HWND hWnd
    )
{
    NTSTATUS status;

    if (NT_SUCCESS(status = NtInitiatePowerAction(
        PowerActionHibernate,
        PowerSystemSleeping1,
        0,
        FALSE
        )))
        return TRUE;
    else
        PhShowStatus(hWnd, L"Unable to hibernate the computer", status, 0);

    return FALSE;
}

BOOLEAN PhUiRestartComputer(
    __in HWND hWnd
    )
{
    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
        hWnd,
        L"restart",
        L"the computer",
        NULL,
        FALSE
        ))
    {
        if (ExitWindowsEx(EWX_REBOOT, 0))
            return TRUE;
        else
            PhShowStatus(hWnd, L"Unable to restart the computer", 0, GetLastError());
    }

    return FALSE;
}

BOOLEAN PhUiShutdownComputer(
    __in HWND hWnd
    )
{
    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
        hWnd,
        L"shutdown",
        L"the computer",
        NULL,
        FALSE
        ))
    {
        if (ExitWindowsEx(EWX_SHUTDOWN, 0))
            return TRUE;
        else
            PhShowStatus(hWnd, L"Unable to shutdown the computer", 0, GetLastError());
    }

    return FALSE;
}

BOOLEAN PhUiPoweroffComputer(
    __in HWND hWnd
    )
{
    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
        hWnd,
        L"poweroff",
        L"the computer",
        NULL,
        FALSE
        ))
    {
        if (ExitWindowsEx(EWX_POWEROFF, 0))
            return TRUE;
        else
            PhShowStatus(hWnd, L"Unable to poweroff the computer", 0, GetLastError());
    }

    return FALSE;
}

BOOLEAN PhUiConnectSession(
    __in HWND hWnd,
    __in ULONG SessionId
    )
{
    static _WinStationConnectW WinStationConnectW_I = NULL;
    PPH_STRING selectedChoice = NULL;

    if (!WinStationConnectW_I)
        WinStationConnectW_I = PhGetProcAddress(L"winsta.dll", "WinStationConnectW");

    if (!WinStationConnectW_I)
    {
        PhShowError(hWnd, L"This feature is not supported on your operating system.");
        return FALSE;
    }

    // Try once with no password.
    if (WinStationConnectW_I(NULL, SessionId, -1, L"", TRUE))
        return TRUE;

    while (PhaChoiceDialog(
        hWnd,
        L"Connect to session",
        L"Password:",
        NULL,
        0,
        NULL,
        PH_CHOICE_DIALOG_PASSWORD,
        &selectedChoice,
        NULL,
        NULL
        ))
    {
        if (WinStationConnectW_I(NULL, SessionId, -1, selectedChoice->Buffer, TRUE))
        {
            return TRUE;
        }
        else
        {
            if (!PhShowContinueStatus(hWnd, L"Unable to connect to the session", 0, GetLastError()))
                break;
        }
    }

    return FALSE;
}

BOOLEAN PhUiDisconnectSession(
    __in HWND hWnd,
    __in ULONG SessionId
    )
{
    if (WTSDisconnectSession(WTS_CURRENT_SERVER_HANDLE, SessionId, FALSE))
        return TRUE;
    else
        PhShowStatus(hWnd, L"Unable to disconnect the session", 0, GetLastError());

    return FALSE;
}

BOOLEAN PhUiLogoffSession(
    __in HWND hWnd,
    __in ULONG SessionId
    )
{
    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
        hWnd,
        L"logoff",
        L"the user",
        NULL,
        FALSE
        ))
    {
        if (WTSLogoffSession(WTS_CURRENT_SERVER_HANDLE, SessionId, FALSE))
            return TRUE;
        else
            PhShowStatus(hWnd, L"Unable to logoff the session", 0, GetLastError());
    }

    return FALSE;
}

/**
 * Determines if a process is a system process.
 *
 * \param ProcessId The PID of the process to check.
 */
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

    if (WINDOWS_HAS_IMAGE_FILE_NAME_BY_PROCESS_ID)
    {
        status = PhGetProcessImageFileNameByProcessId(ProcessId, &fileName);
    }
    else
    {
        if (!NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            ProcessQueryAccess,
            ProcessId
            )))
            return FALSE;

        status = PhGetProcessImageFileName(processHandle, &fileName);
        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
        return FALSE;

    PhSwapReference2(&fileName, PhGetFileName(fileName));
    PhaDereferenceObject(fileName);

    systemDirectory = PHA_DEREFERENCE(PhGetSystemDirectory());

    for (i = 0; i < sizeof(DangerousProcesses) / sizeof(PWSTR); i++)
    {
        PPH_STRING fullName;

        fullName = PhaConcatStrings(3, systemDirectory->Buffer, L"\\", DangerousProcesses[i]);

        if (PhEqualString(fileName, fullName, TRUE))
            return TRUE;
    }

    return FALSE;
}

/**
 * Checks if the user wants to proceed with an operation.
 *
 * \param hWnd A handle to the parent window.
 * \param Verb A verb describing the action.
 * \param Message A message containing additional information 
 * about the action.
 * \param WarnOnlyIfDangerous TRUE to skip the confirmation 
 * dialog if none of the processes are system processes, 
 * FALSE to always show the confirmation dialog.
 * \param Processes An array of pointers to process items.
 * \param NumberOfProcesses The number of process items.
 *
 * \return TRUE if the user wants to proceed with the operation, 
 * otherwise FALSE.
 */
static BOOLEAN PhpShowContinueMessageProcesses(
    __in HWND hWnd,
    __in PWSTR Verb,
    __in_opt PWSTR Message,
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

/**
 * Shows an error message to the user and checks 
 * if the user wants to continue.
 *
 * \param hWnd A handle to the parent window.
 * \param Verb A verb describing the action which 
 * resulted in an error.
 * \param Process The process item which the action 
 * was performed on.
 * \param Status A NT status value representing the 
 * error.
 * \param Win32Result A Win32 error code representing 
 * the error.
 *
 * \return TRUE if the user wants to continue, otherwise 
 * FALSE. The result is typically only useful when 
 * executing an action on multiple processes.
 */
static BOOLEAN PhpShowErrorProcess(
    __in HWND hWnd,
    __in PWSTR Verb,
    __in PPH_PROCESS_ITEM Process,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    )
{
    if (!PH_IS_FAKE_PROCESS_ID(Process->ProcessId))
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
            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (NumberOfProcesses != 1 || !PhpShowErrorAndElevateAction(
                hWnd,
                PhaConcatStrings2(L"Unable to terminate ", Processes[i]->ProcessName->Buffer)->Buffer,
                status,
                PhaFormatString(L"-c -ctype process -caction terminate -cobject %u", (ULONG)Processes[i]->ProcessId)->Buffer,
                &success
                ))
            {
                if (!PhpShowErrorProcess(hWnd, L"terminate", Processes[i], status, 0))
                    break;
            }
        }
    }

    return success;
}

BOOLEAN PhpUiTerminateTreeProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in PVOID Processes,
    __inout PBOOLEAN Success
    )
{
    NTSTATUS status;
    PSYSTEM_PROCESS_INFORMATION process;
    HANDLE processHandle;
    PPH_PROCESS_ITEM processItem;

    // Note:
    // FALSE should be written to Success if any part of the operation failed.
    // The return value of this function indicates whether to continue with 
    // the operation (FALSE if user cancelled).

    // Terminate the process.

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_TERMINATE,
        Process->ProcessId
        )))
    {
        status = PhTerminateProcess(processHandle, STATUS_SUCCESS);
        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        *Success = FALSE;

        if (!PhpShowErrorProcess(hWnd, L"terminate", Process, status, 0))
            return FALSE;
    }

    // Terminate the process' children.

    process = PH_FIRST_PROCESS(Processes);

    do
    {
        if (
            process->UniqueProcessId != Process->ProcessId &&
            process->InheritedFromUniqueProcessId == Process->ProcessId
            )
        {
            if (processItem = PhReferenceProcessItem(process->UniqueProcessId))
            {
                // Check the creation time to make sure it is a descendant.
                if (processItem->CreateTime.QuadPart >= Process->CreateTime.QuadPart)
                {
                    if (!PhpUiTerminateTreeProcess(hWnd, processItem, Processes, Success))
                    {
                        PhDereferenceObject(processItem);
                        return FALSE;
                    }
                }

                PhDereferenceObject(processItem);
            }
        }
    } while (process = PH_NEXT_PROCESS(process));

    return TRUE;
}

BOOLEAN PhUiTerminateTreeProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    BOOLEAN success = TRUE;
    BOOLEAN cont = FALSE;
    PVOID processes;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            hWnd,
            L"terminate",
            PhaConcatStrings2(Process->ProcessName->Buffer, L" and its descendants")->Buffer,
            L"Terminating a process tree will cause the process and its descendants to be terminated.",
            FALSE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
    {
        PhShowStatus(hWnd, L"Unable to enumerate processes", status, 0);
        return FALSE;
    }

    PhpUiTerminateTreeProcess(hWnd, Process, processes, &success);
    PhFree(processes);

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
            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (NumberOfProcesses != 1 || !PhpShowErrorAndElevateAction(
                hWnd,
                PhaConcatStrings2(L"Unable to suspend ", Processes[i]->ProcessName->Buffer)->Buffer,
                status,
                PhaFormatString(L"-c -ctype process -caction suspend -cobject %u", (ULONG)Processes[i]->ProcessId)->Buffer,
                &success
                ))
            {
                if (!PhpShowErrorProcess(hWnd, L"suspend", Processes[i], status, 0))
                    break;
            }
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
            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (NumberOfProcesses != 1 || !PhpShowErrorAndElevateAction(
                hWnd,
                PhaConcatStrings2(L"Unable to resume ", Processes[i]->ProcessName->Buffer)->Buffer,
                status,
                PhaFormatString(L"-c -ctype process -caction resume -cobject %u", (ULONG)Processes[i]->ProcessId)->Buffer,
                &success
                ))
            {
                if (!PhpShowErrorProcess(hWnd, L"resume", Processes[i], status, 0))
                    break;
            }
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
    BOOLEAN cont = FALSE;
    HANDLE processHandle = NULL;
    BOOLEAN isPosix;
    PPH_STRING commandLine;
    PPH_STRING currentDirectory;

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

    NtClose(processHandle);
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

    NtClose(processHandle);
    processHandle = NULL;

    // Start the process.

    status = PhCreateProcessWin32(
        PhGetString(Process->FileName), // we didn't wait for S1 processing
        commandLine->Buffer, // string may be modified, but it's OK in this case
        NULL,
        currentDirectory->Buffer,
        0,
        NULL,
        NULL,
        NULL
        );

ErrorExit:
    if (processHandle)
        NtClose(processHandle);

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(hWnd, L"restart", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

// Contributed by evilpie (#2981421)
BOOLEAN PhUiDebugProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    BOOLEAN cont = FALSE;
    PH_STRING_BUILDER commandLineBuilder;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            hWnd,
            L"debug",
            Process->ProcessName->Buffer,
            L"Debugging a process may result in loss of data.",
            FALSE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    if (PhBeginInitOnce(&DebuggerCommandInitOnce))
    {
        HKEY keyHandle;
        PPH_STRING debugger;
        ULONG firstIndex;
        ULONG secondIndex;

        if (RegOpenKey(
            HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug",
            &keyHandle
            ) == ERROR_SUCCESS)
        {
            debugger = PhQueryRegistryString(keyHandle, L"Debugger");

            if (debugger)
            {
                firstIndex = PhFindCharInString(debugger, 0, L'"');

                if (firstIndex != -1)
                {
                    firstIndex += 1;
                    secondIndex = PhFindCharInString(debugger, firstIndex, L'"');

                    if (secondIndex != -1)
                    {
                        DebuggerCommand = PhSubstring(debugger, firstIndex, secondIndex - firstIndex);
                    }
                }

                PhDereferenceObject(debugger);
            }

            RegCloseKey(keyHandle);
        }

        PhEndInitOnce(&DebuggerCommandInitOnce);
    }

    if (!DebuggerCommand)
    {
        PhShowError(hWnd, L"Unable to locate the debugger.");
        return FALSE;
    }

    PhInitializeStringBuilder(&commandLineBuilder, DebuggerCommand->Length + 30);

    PhAppendCharStringBuilder(&commandLineBuilder, '"');
    PhAppendStringBuilder(&commandLineBuilder, DebuggerCommand);
    PhAppendCharStringBuilder(&commandLineBuilder, '"');
    PhAppendFormatStringBuilder(&commandLineBuilder, L" -p %u", (ULONG)Process->ProcessId);

    status = PhCreateProcessWin32(
        NULL,
        commandLineBuilder.String->Buffer,
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        NULL
        );

    PhDeleteStringBuilder(&commandLineBuilder);

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(hWnd, L"debug", Process, status, 0);
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
        HANDLE processHandle;

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_SET_QUOTA,
            Processes[i]->ProcessId
            )))
        {
            QUOTA_LIMITS quotaLimits;

            memset(&quotaLimits, 0, sizeof(QUOTA_LIMITS));
            quotaLimits.MinimumWorkingSetSize = -1;
            quotaLimits.MaximumWorkingSetSize = -1;

            status = NtSetInformationProcess(
                processHandle,
                ProcessQuotaLimits,
                &quotaLimits,
                sizeof(QUOTA_LIMITS)
                );

            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorProcess(hWnd, L"reduce the working set of", Processes[i], status, 0))
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

            NtClose(tokenHandle);
        }

        NtClose(processHandle);
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
            ULONG flags;

            // Disable kill-on-close.
            flags = 0;
            status = NtSetInformationDebugObject(
                debugObjectHandle,
                DebugObjectFlags,
                &flags,
                sizeof(ULONG),
                NULL
                );

            status = NtRemoveProcessDebug(processHandle, debugObjectHandle);

            NtClose(debugObjectHandle);
        }

        NtClose(processHandle);
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
    static PH_FILETYPE_FILTER filters[] =
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
        ProcessQueryAccess | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
        PROCESS_VM_READ | PROCESS_VM_WRITE,
        Process->ProcessId
        )))
    {
        LARGE_INTEGER timeout;

        timeout.QuadPart = -5 * PH_TIMEOUT_SEC;
        status = PhInjectDllProcess(
            processHandle,
            fileName->Buffer,
            &timeout
            );

        NtClose(processHandle);
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

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        Process->ProcessId
        )))
    {
        status = PhSetProcessIoPriority(processHandle, IoPriority);

        NtClose(processHandle);
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

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status) || win32Result)
    {
        PhpShowErrorProcess(hWnd, L"set the priority of", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiSetDepStatusProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    )
{
    static WCHAR *choices[] = { L"Disabled", L"Enabled", L"Enabled, DEP-ATL thunk emulation disabled" };
    NTSTATUS status;
    HANDLE processHandle;
    ULONG depStatus;
    PPH_STRING selectedChoice;
    BOOLEAN selectedOption;

    // Get the initial DEP status of the process.

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION,
        Process->ProcessId
        )))
    {
        if (NT_SUCCESS(status = PhGetProcessDepStatus(processHandle, &depStatus)))
        {
            ULONG choiceIndex;

            if (depStatus & PH_PROCESS_DEP_ENABLED)
            {
                if (depStatus & PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED)
                    choiceIndex = 2;
                else
                    choiceIndex = 1;
            }
            else
            {
                choiceIndex = 0;
            }

            selectedChoice = PhaCreateString(choices[choiceIndex]);
            selectedOption = FALSE;
        }

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(hWnd, L"set the DEP status of", Process, status, 0);
        return FALSE;
    }

    while (PhaChoiceDialog(
        hWnd,
        L"DEP",
        L"DEP status:",
        choices,
        sizeof(choices) / sizeof(PWSTR),
        PhKphHandle ? L"Permanent" : NULL, // if no KPH, SetProcessDEPPolicy determines permanency
        0,
        &selectedChoice,
        &selectedOption,
        NULL
        ))
    {
        // Build the new DEP status from the selected choice and option.

        depStatus = 0;

        if (PhEqualString2(selectedChoice, choices[0], FALSE))
            depStatus = 0;
        else if (PhEqualString2(selectedChoice, choices[1], FALSE))
            depStatus = PH_PROCESS_DEP_ENABLED;
        else if (PhEqualString2(selectedChoice, choices[2], FALSE))
            depStatus = PH_PROCESS_DEP_ENABLED | PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED;

        if (selectedOption)
            depStatus |= PH_PROCESS_DEP_PERMANENT;

        // Try to set the DEP status.

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PhKphHandle ? ProcessQueryAccess : (PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE),
            Process->ProcessId
            )))
        {
            if (PhKphHandle)
            {
                status = PhSetProcessDepStatus(processHandle, depStatus);
            }
            else
            {
                LARGE_INTEGER timeout;

                timeout.QuadPart = -5 * PH_TIMEOUT_SEC;
                status = PhSetProcessDepStatusInvasive(processHandle, depStatus, &timeout);
            }

            NtClose(processHandle);
        }

        if (NT_SUCCESS(status))
        {
            return TRUE;
        }
        else if (status == STATUS_NOT_SUPPORTED)
        {
            if (PhShowError(
                hWnd,
                L"This feature is not supported by your operating system. "
                L"The minimum supported versions are Windows XP SP3 and Windows Vista SP1."
                ) != IDOK)
                break;
        }
        else
        {
            if (!PhpShowErrorProcess(hWnd, L"set the DEP status of", Process, status, 0))
                break;
        }
    }

    return FALSE;
}

BOOLEAN PhUiSetProtectionProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    )
{
    static WCHAR *choices[] = { L"Protected", L"Not Protected" };
    NTSTATUS status;
    BOOLEAN isProtected;
    PPH_STRING selectedChoice;

    if (!WINDOWS_HAS_LIMITED_ACCESS)
        return FALSE;

    if (!PhKphHandle)
    {
        PhShowError(hWnd, KPH_ERROR_MESSAGE);
        return FALSE;
    }

    if (NT_SUCCESS(KphGetProcessProtected(PhKphHandle, Process->ProcessId, &isProtected)))
    {
        selectedChoice = PhaCreateString(isProtected ? L"Protected" : L"Not Protected");

        while (PhaChoiceDialog(hWnd, L"Protection", L"Protection:", choices, sizeof(choices) / sizeof(PWSTR),
            NULL, 0, &selectedChoice, NULL, NULL))
        {
            status = KphSetProcessProtected(PhKphHandle, Process->ProcessId,
                PhEqualString2(selectedChoice, L"Protected", FALSE));

            if (NT_SUCCESS(status))
            {
                return TRUE;
            }
            else
            {
                if (!PhpShowErrorProcess(hWnd, L"set the protection of", Process, status, 0))
                    break;
            }
        }
    }

    return FALSE;
}

static VOID PhpShowErrorService(
    __in HWND hWnd,
    __in PWSTR Verb,
    __in PPH_SERVICE_ITEM Service,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    )
{
    PhShowStatus(
        hWnd,
        PhaFormatString(
        L"Unable to %s %s",
        Verb,
        Service->Name->Buffer
        )->Buffer,
        Status,
        Win32Result
        );
}

BOOLEAN PhUiStartService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    )
{
    SC_HANDLE serviceHandle;
    BOOLEAN success = FALSE;

    serviceHandle = PhOpenService(Service->Name->Buffer, SERVICE_START);

    if (serviceHandle)
    {
        if (StartService(serviceHandle, 0, NULL))
            success = TRUE;

        CloseServiceHandle(serviceHandle);
    }

    if (!success)
    {
        NTSTATUS status;

        status = NTSTATUS_FROM_WIN32(GetLastError());

        if (!PhpShowErrorAndElevateAction(
            hWnd,
            PhaConcatStrings2(L"Unable to start ", Service->Name->Buffer)->Buffer,
            status,
            PhaConcatStrings(3, L"-c -ctype service -caction start -cobject \"", Service->Name->Buffer, L"\"")->Buffer,
            &success
            ))
        {
            PhpShowErrorService(hWnd, L"start", Service, status, 0);
        }
    }

    return success;
}

BOOLEAN PhUiContinueService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    )
{
    SC_HANDLE serviceHandle;
    BOOLEAN success = FALSE;

    serviceHandle = PhOpenService(Service->Name->Buffer, SERVICE_PAUSE_CONTINUE);

    if (serviceHandle)
    {
        SERVICE_STATUS serviceStatus;

        if (ControlService(serviceHandle, SERVICE_CONTROL_CONTINUE, &serviceStatus))
            success = TRUE;

        CloseServiceHandle(serviceHandle);
    }

    if (!success)
    {
        NTSTATUS status;

        status = NTSTATUS_FROM_WIN32(GetLastError());

        if (!PhpShowErrorAndElevateAction(
            hWnd,
            PhaConcatStrings2(L"Unable to continue ", Service->Name->Buffer)->Buffer,
            status,
            PhaConcatStrings(3, L"-c -ctype service -caction continue -cobject \"", Service->Name->Buffer, L"\"")->Buffer,
            &success
            ))
        {
            PhpShowErrorService(hWnd, L"continue", Service, status, 0);
        }
    }

    return success;
}

BOOLEAN PhUiPauseService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    )
{
    SC_HANDLE serviceHandle;
    BOOLEAN success = FALSE;

    serviceHandle = PhOpenService(Service->Name->Buffer, SERVICE_PAUSE_CONTINUE);

    if (serviceHandle)
    {
        SERVICE_STATUS serviceStatus;

        if (ControlService(serviceHandle, SERVICE_CONTROL_PAUSE, &serviceStatus))
            success = TRUE;

        CloseServiceHandle(serviceHandle);
    }

    if (!success)
    {
        NTSTATUS status;

        status = NTSTATUS_FROM_WIN32(GetLastError());

        if (!PhpShowErrorAndElevateAction(
            hWnd,
            PhaConcatStrings2(L"Unable to pause ", Service->Name->Buffer)->Buffer,
            status,
            PhaConcatStrings(3, L"-c -ctype service -caction pause -cobject \"", Service->Name->Buffer, L"\"")->Buffer,
            &success
            ))
        {
            PhpShowErrorService(hWnd, L"pause", Service, status, 0);
        }
    }

    return success;
}

BOOLEAN PhUiStopService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    )
{
    SC_HANDLE serviceHandle;
    BOOLEAN success = FALSE;

    serviceHandle = PhOpenService(Service->Name->Buffer, SERVICE_STOP);

    if (serviceHandle)
    {
        SERVICE_STATUS serviceStatus;

        if (ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus))
            success = TRUE;

        CloseServiceHandle(serviceHandle);
    }

    if (!success)
    {
        NTSTATUS status;

        status = NTSTATUS_FROM_WIN32(GetLastError());

        if (!PhpShowErrorAndElevateAction(
            hWnd,
            PhaConcatStrings2(L"Unable to stop ", Service->Name->Buffer)->Buffer,
            status,
            PhaConcatStrings(3, L"-c -ctype service -caction stop -cobject \"", Service->Name->Buffer, L"\"")->Buffer,
            &success
            ))
        {
            PhpShowErrorService(hWnd, L"stop", Service, status, 0);
        }
    }

    return success;
}

BOOLEAN PhUiDeleteService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    )
{
    SC_HANDLE serviceHandle;
    BOOLEAN success = FALSE;

    // Warnings cannot be disabled for service deletion.
    if (!PhShowConfirmMessage(
        hWnd,
        L"delete",
        Service->Name->Buffer,
        L"Deleting a service can prevent the system from starting "
        L"or functioning properly.",
        TRUE
        ))
        return FALSE;

    serviceHandle = PhOpenService(Service->Name->Buffer, DELETE);

    if (serviceHandle)
    {
        if (DeleteService(serviceHandle))
            success = TRUE;

        CloseServiceHandle(serviceHandle);
    }

    if (!success)
    {
        NTSTATUS status;

        status = NTSTATUS_FROM_WIN32(GetLastError());

        if (!PhpShowErrorAndElevateAction(
            hWnd,
            PhaConcatStrings2(L"Unable to delete ", Service->Name->Buffer)->Buffer,
            status,
            PhaConcatStrings(3, L"-c -ctype service -caction delete -cobject \"", Service->Name->Buffer, L"\"")->Buffer,
            &success
            ))
        {
            PhpShowErrorService(hWnd, L"delete", Service, status, 0);
        }
    }

    return success;
}

BOOLEAN PhUiCloseConnections(
    __in HWND hWnd,
    __in PPH_NETWORK_ITEM *Connections,
    __in ULONG NumberOfConnections
    )
{
    BOOLEAN success = TRUE;
    ULONG i;
    _SetTcpEntry SetTcpEntry_I;
    MIB_TCPROW tcpRow;

    SetTcpEntry_I = PhGetProcAddress(L"iphlpapi.dll", "SetTcpEntry");

    if (!SetTcpEntry_I)
    {
        PhShowError(
            hWnd,
            L"This feature is not supported by your operating system."
            );
        return FALSE;
    }

    for (i = 0; i < NumberOfConnections; i++)
    {
        if (
            Connections[i]->ProtocolType != PH_TCP4_NETWORK_PROTOCOL ||
            Connections[i]->State != MIB_TCP_STATE_ESTAB
            )
            continue;

        tcpRow.dwState = MIB_TCP_STATE_DELETE_TCB;
        tcpRow.dwLocalAddr = Connections[i]->LocalEndpoint.Address.Ipv4;
        tcpRow.dwLocalPort = _byteswap_ushort((USHORT)Connections[i]->LocalEndpoint.Port);
        tcpRow.dwRemoteAddr = Connections[i]->RemoteEndpoint.Address.Ipv4;
        tcpRow.dwRemotePort = _byteswap_ushort((USHORT)Connections[i]->RemoteEndpoint.Port);

        if (SetTcpEntry_I(&tcpRow) != 0)
        {
            success = FALSE;

            if (PhShowMessage(
                hWnd,
                MB_ICONERROR | MB_OKCANCEL,
                L"Unable to close the TCP connection (from %s:%u). "
                L"Make sure Process Hacker is running with administrative privileges.",
                Connections[i]->LocalAddressString,
                Connections[i]->LocalEndpoint.Port
                ) != IDOK)
                break;
        }
    }

    return success;
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
            NtClose(threadHandle);
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
            L"Forcibly terminating threads may cause the system to crash or become unstable.",
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
            L"Forcibly terminating system threads may cause the system to crash or become unstable.",
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
            NtClose(threadHandle);
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
            NtClose(threadHandle);
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
            NtClose(threadHandle);
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

        NtClose(threadHandle);
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

    if (NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_SET_INFORMATION,
        Thread->ThreadId
        )))
    {
        status = PhSetThreadIoPriority(threadHandle, IoPriority);

        NtClose(threadHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorThread(hWnd, L"set the I/O priority of", Thread, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiUnloadModule(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_MODULE_ITEM Module
    )
{
    NTSTATUS status;
    BOOLEAN cont = FALSE;
    HANDLE processHandle;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            hWnd,
            L"unload",
            Module->Name->Buffer,
            ProcessId != SYSTEM_PROCESS_ID ?
            L"Unloading a module may cause the process to crash." :
            L"Unloading a driver may cause system instability.",
            TRUE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    if (ProcessId != SYSTEM_PROCESS_ID)
    {
        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            ProcessQueryAccess | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
            PROCESS_VM_READ | PROCESS_VM_WRITE,
            ProcessId
            )))
        {
            LARGE_INTEGER timeout;

            timeout.QuadPart = -5 * PH_TIMEOUT_SEC;
            status = PhUnloadDllProcess(
                processHandle,
                Module->BaseAddress,
                &timeout
                );

            NtClose(processHandle);
        }

        if (status == STATUS_DLL_NOT_FOUND)
        {
            PhShowError(hWnd, L"Unable to find the module to unload. This may be "
                L"due to an attempt to unload a mapped file.");
            return FALSE;
        }

        if (!NT_SUCCESS(status))
        {
            PhShowStatus(
                hWnd,
                PhaConcatStrings2(L"Unable to unload ", Module->Name->Buffer)->Buffer,
                status,
                0
                );
            return FALSE;
        }
    }
    else
    {
        status = PhUnloadDriver(Module->BaseAddress, Module->Name->Buffer);

        if (!NT_SUCCESS(status))
        {
            PhShowStatus(
                hWnd,
                PhaConcatStrings(
                3,
                L"Unable to unload ",
                Module->Name->Buffer,
                L". Make sure Process Hacker is running with "
                L"administrative privileges. Error"
                )->Buffer,
                status,
                0
                );
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN PhUiFreeMemory(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_MEMORY_ITEM MemoryItem,
    __in BOOLEAN Free
    )
{
    NTSTATUS status;
    BOOLEAN cont = FALSE;
    HANDLE processHandle;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            hWnd,
            Free ? L"free" : L"decommit",
            L"the memory region",
            Free ?
            L"Freeing memory regions may cause the process to crash." :
            L"Decommitting memory regions may cause the process to crash.",
            TRUE
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
        PROCESS_VM_OPERATION,
        ProcessId
        )))
    {
        PVOID baseAddress;
        SIZE_T regionSize;

        baseAddress = MemoryItem->BaseAddress;

        // The size needs to be 0 if we're freeing.
        if (Free)
            regionSize = 0;
        else
            regionSize = MemoryItem->Size;

        status = NtFreeVirtualMemory(
            processHandle,
            &baseAddress,
            &regionSize,
            Free ? MEM_RELEASE : MEM_DECOMMIT
            );
        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(
            hWnd,
            Free ? L"Unable to free the memory region" : L"Unable to decommit the memory region",
            status,
            0
            );
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN PhpShowErrorHandle(
    __in HWND hWnd,
    __in PWSTR Verb,
    __in PPH_HANDLE_ITEM Handle,
    __in NTSTATUS Status,
    __in_opt ULONG Win32Result
    )
{
    if (!PhIsNullOrEmptyString(Handle->BestObjectName))
    {
        return PhShowContinueStatus(
            hWnd,
            PhaFormatString(
            L"Unable to %s handle \"%s\" (0x%Ix)",
            Verb,
            Handle->BestObjectName->Buffer,
            (ULONG)Handle->Handle
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
            L"Unable to %s handle 0x%Ix",
            Verb,
            (ULONG)Handle->Handle
            )->Buffer,
            Status,
            Win32Result
            );
    }
}

BOOLEAN PhUiCloseHandles(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM *Handles,
    __in ULONG NumberOfHandles,
    __in BOOLEAN Warn
    )
{
    NTSTATUS status;
    BOOLEAN cont = FALSE;
    BOOLEAN success = TRUE;
    HANDLE processHandle;

    if (NumberOfHandles == 0)
        return FALSE;

    if (Warn && PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            hWnd,
            L"close",
            NumberOfHandles == 1 ? L"the selected handle" : L"the selected handles",
            L"Closing handles may cause system instability and data corruption.",
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
        PROCESS_DUP_HANDLE,
        ProcessId
        )))
    {
        ULONG i;

        for (i = 0; i < NumberOfHandles; i++)
        {
            status = PhDuplicateObject(
                processHandle,
                Handles[i]->Handle,
                NULL,
                NULL,
                0,
                0,
                DUPLICATE_CLOSE_SOURCE
                );

            if (!NT_SUCCESS(status))
            {
                success = FALSE;

                if (!PhpShowErrorHandle(
                    hWnd,
                    L"close",
                    Handles[i],
                    status,
                    0
                    ))
                    break;
            }
        }

        NtClose(processHandle);
    }
    else
    {
        PhShowStatus(hWnd, L"Unable to open the process", status, 0);
        return FALSE;
    }

    return success;
}

BOOLEAN PhUiSetAttributesHandle(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM Handle,
    __in ULONG Attributes
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
        ProcessQueryAccess,
        ProcessId
        )))
    {
        status = KphSetHandleAttributes(
            PhKphHandle,
            processHandle,
            Handle->Handle,
            Attributes
            );

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorHandle(hWnd, L"set attributes of", Handle, status, 0);
        return FALSE;
    }

    return TRUE;
}
