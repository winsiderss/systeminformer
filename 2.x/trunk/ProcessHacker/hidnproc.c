/*
 * Process Hacker - 
 *   hidden processes detection
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
#include <hidnproc.h>
#include <windowsx.h>

INT_PTR CALLBACK PhpHiddenProcessesDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HWND PhHiddenProcessesWindowHandle = NULL;
HWND PhHiddenProcessesListViewHandle = NULL;
static PH_LAYOUT_MANAGER WindowLayoutManager;

VOID PhShowHiddenProcessesDialog()
{
    if (!PhHiddenProcessesWindowHandle)
    {
        PhHiddenProcessesWindowHandle = CreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_HIDDENPROCESSES),
            PhMainWndHandle,
            PhpHiddenProcessesDlgProc
            );
    }

    if (!IsWindowVisible(PhHiddenProcessesWindowHandle))
        ShowWindow(PhHiddenProcessesWindowHandle, SW_SHOW);
    else
        BringWindowToTop(PhHiddenProcessesWindowHandle);
}

static INT_PTR CALLBACK PhpHiddenProcessesDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            PhHiddenProcessesListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_PROCESSES);

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_INTRO),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_FORCE_INVALIDATE);
            PhAddLayoutItem(&WindowLayoutManager, lvHandle,
                NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_DESCRIPTION),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM | PH_FORCE_INVALIDATE);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_METHOD),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_TERMINATE),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SAVE),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SCAN),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhRegisterDialog(hwndDlg);

            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 320, L"Process");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 60, L"PID");

            PhSetExtendedListView(lvHandle);
            ExtendedListView_AddFallbackColumn(lvHandle, 0);
            ExtendedListView_AddFallbackColumn(lvHandle, 1);

            ComboBox_AddString(GetDlgItem(hwndDlg, IDC_METHOD), L"Brute Force");
            ComboBox_AddString(GetDlgItem(hwndDlg, IDC_METHOD), L"CSR Handles");
            ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_METHOD), -1, L"CSR Handles");
        }
        break;
    case WM_CLOSE:
        {
            // Hide, don't close.
            ShowWindow(hwndDlg, SW_HIDE);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);
        }
        return TRUE;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    SendMessage(hwndDlg, WM_CLOSE, 0, 0);
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&WindowLayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 476, 380);
        }
        break;
    }

    return FALSE;
}

NTSTATUS PhEnumProcessHandles(
    __in HANDLE ProcessHandle,
    __out PPROCESS_HANDLE_INFORMATION *Handles
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 2048;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = KphQueryProcessHandles(
            PhKphHandle,
            ProcessHandle,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Handles = buffer;

    return status;
}

NTSTATUS PhGetProcessKnownType(
    __in HANDLE ProcessHandle,
    __out PPH_KNOWN_PROCESS_TYPE KnownProcessType
    )
{
    NTSTATUS status;
    PH_KNOWN_PROCESS_TYPE knownProcessType;
    PROCESS_BASIC_INFORMATION basicInfo;
    PPH_STRING systemDirectory;
    PPH_STRING fileName;
    PPH_STRING newFileName;
    PPH_STRING baseName;

    if (!NT_SUCCESS(status = PhGetProcessBasicInformation(
        ProcessHandle,
        &basicInfo
        )))
        return status;

    if (basicInfo.UniqueProcessId == SYSTEM_PROCESS_ID)
    {
        *KnownProcessType = SystemProcessType;
        return STATUS_SUCCESS;
    }

    systemDirectory = PhGetSystemDirectory();

    if (!systemDirectory)
        return STATUS_UNSUCCESSFUL;

    if (!NT_SUCCESS(status = PhGetProcessImageFileName(
        ProcessHandle,
        &fileName
        )))
    {
        PhDereferenceObject(systemDirectory);
        return status;
    }

    newFileName = PhGetFileName(fileName);
    PhDereferenceObject(fileName);

    knownProcessType = UnknownProcessType;

    if (PhStringStartsWith(newFileName, systemDirectory, TRUE))
    {
        baseName = PhSubstring(newFileName, 0, systemDirectory->Length / 2);

        // The system directory string never ends in a backslash, unless 
        // it is a drive root. We're not going to account for that case.

        if (!baseName)
            ; // Dummy
        else if (PhStringEquals2(baseName, L"\\smss.exe", TRUE))
            knownProcessType = SessionManagerProcessType;
        else if (PhStringEquals2(baseName, L"\\csrss.exe", TRUE))
            knownProcessType = WindowsSubsystemProcessType;
        else if (PhStringEquals2(baseName, L"\\wininit.exe", TRUE))
            knownProcessType = WindowsStartupProcessType;
        else if (PhStringEquals2(baseName, L"\\services.exe", TRUE))
            knownProcessType = ServiceControlManagerProcessType;
        else if (PhStringEquals2(baseName, L"\\lsass.exe", TRUE))
            knownProcessType = LocalSecurityAuthorityProcessType;
        else if (PhStringEquals2(baseName, L"\\lsm.exe", TRUE))
            knownProcessType = LocalSessionManagerProcessType;

        PhDereferenceObject(baseName);
    }

    PhDereferenceObject(systemDirectory);
    PhDereferenceObject(newFileName);

    *KnownProcessType = knownProcessType;

    return status;
}

NTSTATUS PhpOpenCsrProcesses(
    __out PHANDLE *ProcessHandles,
    __out PULONG NumberOfProcessHandles
    )
{
    NTSTATUS status;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    PPH_LIST processHandleList;

    if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
        return status;

    processHandleList = PhCreateList(8);

    process = PH_FIRST_PROCESS(processes);

    do
    {
        HANDLE processHandle;
        PH_KNOWN_PROCESS_TYPE knownProcessType;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            ProcessQueryAccess | PROCESS_DUP_HANDLE,
            process->UniqueProcessId
            )))
        {
            if (NT_SUCCESS(PhGetProcessKnownType(
                processHandle,
                &knownProcessType
                )) &&
                knownProcessType == WindowsSubsystemProcessType)
            {
                PhAddListItem(processHandleList, processHandle);
            }
            else
            {
                CloseHandle(processHandle);
            }
        }
    } while (process = PH_NEXT_PROCESS(process));

    *ProcessHandles = PhAllocateCopy(processHandleList->Items, processHandleList->Count * sizeof(HANDLE));
    *NumberOfProcessHandles = processHandleList->Count;

    PhDereferenceObject(processHandleList);

    return status;
}

NTSTATUS PhpGetCsrHandleProcessId(
    __inout PPH_CSR_HANDLE_INFO Handle
    )
{
    NTSTATUS status;

    Handle->IsThreadHandle = FALSE;
    Handle->ProcessId = NULL;

    // Assume the handle is a process handle, and get the 
    // process ID.

    status = KphGetProcessId(
        PhKphHandle,
        Handle->CsrProcessHandle,
        Handle->Handle,
        &Handle->ProcessId
        );

    if (!Handle->ProcessId)
        status = STATUS_UNSUCCESSFUL;

    if (!NT_SUCCESS(status))
    {
        HANDLE threadId;

        // We failed to get the process ID. Assume the handle 
        // is a thread handle, and get the process ID.

        status = KphGetThreadId(
            PhKphHandle,
            Handle->CsrProcessHandle,
            Handle->Handle,
            &threadId,
            &Handle->ProcessId
            );

        if (!Handle->ProcessId)
            status = STATUS_UNSUCCESSFUL;

        if (NT_SUCCESS(status))
            Handle->IsThreadHandle = TRUE;
    }

    return status;
}

NTSTATUS PhEnumCsrProcessHandles(
    __in PPH_ENUM_CSR_PROCESS_HANDLES_CALLBACK Callback,
    __in PVOID Context
    )
{
    NTSTATUS status;
    PHANDLE csrProcessHandles;
    ULONG numberOfCsrProcessHandles;
    ULONG i;
    BOOLEAN stop = FALSE;
    PPH_LIST pids;

    if (!NT_SUCCESS(status = PhpOpenCsrProcesses(
        &csrProcessHandles,
        &numberOfCsrProcessHandles
        )))
        return status;

    pids = PhCreateList(40);

    for (i = 0; i < numberOfCsrProcessHandles; i++)
    {
        PPROCESS_HANDLE_INFORMATION handles;
        ULONG j;

        if (stop)
            break;

        if (NT_SUCCESS(PhEnumProcessHandles(csrProcessHandles[i], &handles)))
        {
            for (j = 0; j < handles->HandleCount; j++)
            {
                PH_CSR_HANDLE_INFO handle;

                handle.CsrProcessHandle = csrProcessHandles[i];
                handle.Handle = handles->Handles[j].Handle;

                // Get the process ID associated with the handle. 
                // This call will fail if the handle is not a 
                // process or thread handle.
                if (!NT_SUCCESS(PhpGetCsrHandleProcessId(&handle)))
                    continue;

                // Avoid duplicate PIDs.
                if (PhIndexOfListItem(pids, handle.ProcessId) != -1)
                    continue;

                PhAddListItem(pids, handle.ProcessId);

                if (!Callback(&handle, Context))
                {
                    stop = TRUE;
                    break;
                }
            }

            PhFree(handles);
        }
    }

    PhDereferenceObject(pids);

    for (i = 0; i < numberOfCsrProcessHandles; i++)
        CloseHandle(csrProcessHandles[i]);

    PhFree(csrProcessHandles);

    return status;
}

NTSTATUS PhOpenProcessByCsrHandle(
    __out PHANDLE ProcessHandle,
    __in PPH_CSR_HANDLE_INFO Handle,
    __in ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS status;

    if (!Handle->IsThreadHandle)
    {
        status = PhDuplicateObject(
            Handle->CsrProcessHandle,
            Handle->Handle,
            NtCurrentProcess(),
            ProcessHandle,
            DesiredAccess,
            0,
            0
            );
    }
    else
    {
        HANDLE threadHandle;

        if (!NT_SUCCESS(status = PhDuplicateObject(
            Handle->CsrProcessHandle,
            Handle->Handle,
            NtCurrentProcess(),
            &threadHandle,
            ThreadQueryAccess,
            0,
            0
            )))
            return status;

        status = KphOpenThreadProcess(
            PhKphHandle,
            ProcessHandle,
            threadHandle,
            DesiredAccess
            );
        CloseHandle(threadHandle);
    }

    return status;
}
