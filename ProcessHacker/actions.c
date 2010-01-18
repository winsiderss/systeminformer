#include <phgui.h>
#include <settings.h>

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

    PHA_DEREFERENCE(fileName);
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
