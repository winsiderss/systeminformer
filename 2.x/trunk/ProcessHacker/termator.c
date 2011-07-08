/*
 * Process Hacker - 
 *   process termination tool
 * 
 * Copyright (C) 2010-2011 wj32
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
#include <kphuser.h>

#define CROSS_INDEX 0
#define TICK_INDEX 1

typedef NTSTATUS (NTAPI *PTEST_PROC)(
    __in HANDLE ProcessId
    );

typedef struct _TEST_ITEM
{
    PWSTR Id;
    PWSTR Description;
    PTEST_PROC TestProc;
} TEST_ITEM, *PTEST_ITEM;

INT_PTR CALLBACK PhpProcessTerminatorDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowProcessTerminatorDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_TERMINATOR),
        ParentWindowHandle,
        PhpProcessTerminatorDlgProc,
        (LPARAM)ProcessItem
        );
}

static PVOID GetExitProcessFunction()
{
    // Vista and above export.
    if (WindowsVersion >= WINDOWS_VISTA)
        return PhGetProcAddress(L"ntdll.dll", "RtlExitUserProcess");
    else
        return PhGetProcAddress(L"kernel32.dll", "ExitProcess");
}

static NTSTATUS NTAPI TerminatorTP1(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_TERMINATE,
        ProcessId
        )))
    {
        // Don't use KPH.
        status = NtTerminateProcess(processHandle, STATUS_SUCCESS);

        NtClose(processHandle);
    }

    return status;
}

static NTSTATUS NTAPI TerminatorTP2(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
        ProcessId
        )))
    {
        status = RtlCreateUserThread(
            processHandle,
            NULL,
            FALSE,
            0,
            0,
            0,
            (PUSER_THREAD_START_ROUTINE)GetExitProcessFunction(),
            NULL,
            NULL,
            NULL
            );

        NtClose(processHandle);
    }

    return status;
}

static NTSTATUS NTAPI TerminatorTTGeneric(
    __in HANDLE ProcessId,
    __in BOOLEAN UseKph,
    __in BOOLEAN UseKphDangerous
    )
{
    NTSTATUS status;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    ULONG i;

    if ((UseKph || UseKphDangerous) && !PhKphHandle)
        return STATUS_NOT_SUPPORTED;

    if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
        return status;

    process = PhFindProcessInformation(processes, ProcessId);

    if (!process)
    {
        PhFree(processes);
        return STATUS_INVALID_CID;
    }

    for (i = 0; i < process->NumberOfThreads; i++)
    {
        HANDLE threadHandle;

        if (NT_SUCCESS(PhOpenThread(
            &threadHandle,
            THREAD_TERMINATE,
            process->Threads[i].ClientId.UniqueThread
            )))
        {
            if (UseKphDangerous)
                KphTerminateThreadUnsafe(PhKphHandle, threadHandle, STATUS_SUCCESS);
            else if (UseKph)
                KphTerminateThread(PhKphHandle, threadHandle, STATUS_SUCCESS);
            else
                NtTerminateThread(threadHandle, STATUS_SUCCESS);

            NtClose(threadHandle);
        }
    }

    PhFree(processes);

    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI TerminatorTT1(
    __in HANDLE ProcessId
    )
{
    return TerminatorTTGeneric(ProcessId, FALSE, FALSE);
}

static NTSTATUS NTAPI TerminatorTT2(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    ULONG i;
    CONTEXT context;
    PVOID exitProcess;

    exitProcess = GetExitProcessFunction();

    if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
        return status;

    process = PhFindProcessInformation(processes, ProcessId);

    if (!process)
    {
        PhFree(processes);
        return STATUS_INVALID_CID;
    }

    for (i = 0; i < process->NumberOfThreads; i++)
    {
        HANDLE threadHandle;

        if (NT_SUCCESS(PhOpenThread(
            &threadHandle,
            THREAD_GET_CONTEXT | THREAD_SET_CONTEXT,
            process->Threads[i].ClientId.UniqueThread
            )))
        {
#ifdef _M_IX86
            context.ContextFlags = CONTEXT_CONTROL;
            PhGetThreadContext(threadHandle, &context);
            context.Eip = (ULONG)exitProcess;
            PhSetThreadContext(threadHandle, &context);
#else
            context.ContextFlags = CONTEXT_CONTROL;
            PhGetThreadContext(threadHandle, &context);
            context.Rip = (ULONG64)exitProcess;
            PhSetThreadContext(threadHandle, &context);
#endif

            NtClose(threadHandle);
        }
    }

    PhFree(processes);

    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI TerminatorTP1a(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle = NtCurrentProcess();
    ULONG i;

    if (!NtGetNextProcess)
        return STATUS_NOT_SUPPORTED;

    if (!NT_SUCCESS(status = NtGetNextProcess(
        NtCurrentProcess(),
        ProcessQueryAccess | PROCESS_TERMINATE,
        0,
        0,
        &processHandle
        )))
        return status;

    for (i = 0; i < 1000; i++) // make sure we don't go into an infinite loop or something
    {
        HANDLE newProcessHandle;
        PROCESS_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhGetProcessBasicInformation(processHandle, &basicInfo)))
        {
            if (basicInfo.UniqueProcessId == ProcessId)
            {
                PhTerminateProcess(processHandle, STATUS_SUCCESS);
                break;
            }
        }

        if (NT_SUCCESS(status = NtGetNextProcess(
            processHandle,
            ProcessQueryAccess | PROCESS_TERMINATE,
            0,
            0,
            &newProcessHandle
            )))
        {
            NtClose(processHandle);
            processHandle = newProcessHandle;
        }
        else
        {
            NtClose(processHandle);
            break;
        }
    }

    return status;
}

static NTSTATUS NTAPI TerminatorTT1a(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE threadHandle;
    ULONG i;

    if (!NtGetNextThread)
        return STATUS_NOT_SUPPORTED;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION, // NtGetNextThread actually requires this access for some reason
        ProcessId
        )))
    {
        if (!NT_SUCCESS(status = NtGetNextThread(
            processHandle,
            NULL,
            THREAD_TERMINATE,
            0,
            0,
            &threadHandle
            )))
        {
            NtClose(processHandle);
            return status;
        }

        for (i = 0; i < 1000; i++)
        {
            HANDLE newThreadHandle;

            PhTerminateThread(threadHandle, STATUS_SUCCESS);

            if (NT_SUCCESS(NtGetNextThread(
                processHandle,
                threadHandle,
                THREAD_TERMINATE,
                0,
                0,
                &newThreadHandle
                )))
            {
                NtClose(threadHandle);
                threadHandle = newThreadHandle;
            }
            else
            {
                NtClose(threadHandle);
                break;
            }
        }

        NtClose(processHandle);
    }

    return status;
}

static NTSTATUS NTAPI TerminatorCH1(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_DUP_HANDLE,
        ProcessId
        )))
    {
        ULONG i;

        for (i = 0; i < 0x1000; i += 4)
        {
            PhDuplicateObject(
                processHandle,
                (HANDLE)i,
                NULL,
                NULL,
                0,
                0,
                DUPLICATE_CLOSE_SOURCE
                );
        }

        NtClose(processHandle);
    }

    return status;
}

static BOOL CALLBACK DestroyProcessWindowsProc(
    __in HWND hwnd,
    __in LPARAM lParam
    )
{
    ULONG processId;

    GetWindowThreadProcessId(hwnd, &processId);

    if (processId == (ULONG)lParam)
    {
        PostMessage(hwnd, WM_DESTROY, 0, 0);
    }

    return TRUE;
}

static NTSTATUS NTAPI TerminatorW1(
    __in HANDLE ProcessId
    )
{
    EnumWindows(DestroyProcessWindowsProc, (LPARAM)ProcessId);
    return STATUS_SUCCESS;
}

static BOOL CALLBACK QuitProcessWindowsProc(
    __in HWND hwnd,
    __in LPARAM lParam
    )
{
    ULONG processId;

    GetWindowThreadProcessId(hwnd, &processId);

    if (processId == (ULONG)lParam)
    {
        PostMessage(hwnd, WM_QUIT, 0, 0);
    }

    return TRUE;
}

static NTSTATUS NTAPI TerminatorW2(
    __in HANDLE ProcessId
    )
{
    EnumWindows(QuitProcessWindowsProc, (LPARAM)ProcessId);
    return STATUS_SUCCESS;
}

static BOOL CALLBACK CloseProcessWindowsProc(
    __in HWND hwnd,
    __in LPARAM lParam
    )
{
    ULONG processId;

    GetWindowThreadProcessId(hwnd, &processId);

    if (processId == (ULONG)lParam)
    {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }

    return TRUE;
}

static NTSTATUS NTAPI TerminatorW3(
    __in HANDLE ProcessId
    )
{
    EnumWindows(CloseProcessWindowsProc, (LPARAM)ProcessId);
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI TerminatorTJ1(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    // TODO: Check if the process is already in a job.

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_QUOTA | PROCESS_TERMINATE,
        ProcessId
        )))
    {
        HANDLE jobHandle;

        status = NtCreateJobObject(&jobHandle, JOB_OBJECT_ALL_ACCESS, NULL); 

        if (NT_SUCCESS(status))
        {
            status = NtAssignProcessToJobObject(jobHandle, processHandle);

            if (NT_SUCCESS(status))
                status = NtTerminateJobObject(jobHandle, STATUS_SUCCESS);

            NtClose(jobHandle);
        }

        NtClose(processHandle);
    }

    return status;
}

static NTSTATUS NTAPI TerminatorTD1(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_SUSPEND_RESUME,
        ProcessId
        )))
    {
        HANDLE debugObjectHandle;
        OBJECT_ATTRIBUTES objectAttributes;

        InitializeObjectAttributes(
            &objectAttributes,
            NULL,
            0,
            NULL,
            NULL
            );

        if (NT_SUCCESS(NtCreateDebugObject(
            &debugObjectHandle,
            DEBUG_PROCESS_ASSIGN,
            &objectAttributes,
            DEBUG_KILL_ON_CLOSE
            )))
        {
            NtDebugActiveProcess(processHandle, debugObjectHandle);
            NtClose(debugObjectHandle);
        }

        NtClose(processHandle);
    }

    return status;
}

static NTSTATUS NTAPI TerminatorTP3(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (!PhKphHandle)
        return STATUS_NOT_SUPPORTED;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        SYNCHRONIZE, // KPH doesn't require any access for this operation
        ProcessId
        )))
    {
        status = KphTerminateProcess(PhKphHandle, processHandle, STATUS_SUCCESS);

        NtClose(processHandle);
    }

    return status;
}

static NTSTATUS NTAPI TerminatorTT3(
    __in HANDLE ProcessId
    )
{
    return TerminatorTTGeneric(ProcessId, TRUE, FALSE);
}

static NTSTATUS NTAPI TerminatorTT4(
    __in HANDLE ProcessId
    )
{
    return TerminatorTTGeneric(ProcessId, FALSE, TRUE);
}

static NTSTATUS NTAPI TerminatorM1(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_WRITE,
        ProcessId
        )))
    {
        PVOID pageOfGarbage;
        SIZE_T pageSize;
        PVOID baseAddress;
        MEMORY_BASIC_INFORMATION basicInfo;

        pageOfGarbage = NULL;
        pageSize = PAGE_SIZE;

        if (!NT_SUCCESS(NtAllocateVirtualMemory(
            NtCurrentProcess(),
            &pageOfGarbage,
            0,
            &pageSize,
            MEM_COMMIT,
            PAGE_READONLY
            )))
        {
            NtClose(processHandle);
            return STATUS_NO_MEMORY;
        }

        baseAddress = (PVOID)0;

        while (NT_SUCCESS(NtQueryVirtualMemory(
            processHandle,
            baseAddress,
            MemoryBasicInformation,
            &basicInfo,
            sizeof(MEMORY_BASIC_INFORMATION),
            NULL
            )))
        {
            ULONG i;

            // Make sure we don't write to views of mapped files. That 
            // could possibly corrupt files!
            if (basicInfo.Type == MEM_PRIVATE)
            {
                for (i = 0; i < basicInfo.RegionSize; i += PAGE_SIZE)
                {
                    PhWriteVirtualMemory(
                        processHandle,
                        PTR_ADD_OFFSET(baseAddress, i),
                        pageOfGarbage,
                        PAGE_SIZE,
                        NULL
                        );
                }
            }

            baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);
        }

        // Size needs to be zero if we're freeing.
        pageSize = 0;
        NtFreeVirtualMemory(
            NtCurrentProcess(),
            &pageOfGarbage,
            &pageSize,
            MEM_RELEASE
            );

        NtClose(processHandle);
    }

    return status;
}

static NTSTATUS NTAPI TerminatorM2(
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION,
        ProcessId
        )))
    {              
        PVOID baseAddress;
        MEMORY_BASIC_INFORMATION basicInfo;
        ULONG oldProtect;

        baseAddress = (PVOID)0;

        while (NT_SUCCESS(NtQueryVirtualMemory(
            processHandle,
            baseAddress,
            MemoryBasicInformation,
            &basicInfo,
            sizeof(MEMORY_BASIC_INFORMATION),
            NULL
            )))
        {
            SIZE_T regionSize;

            regionSize = basicInfo.RegionSize;
            NtProtectVirtualMemory(
                processHandle,
                &basicInfo.BaseAddress,
                &regionSize,
                PAGE_NOACCESS,
                &oldProtect
                );
            baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);
        }

        NtClose(processHandle);
    }

    return status;
}

TEST_ITEM PhTerminatorTests[] =
{
    { L"TP1", L"Terminates the process using NtTerminateProcess", TerminatorTP1 },
    { L"TP2", L"Creates a remote thread in the process which terminates the process", TerminatorTP2 },
    { L"TT1", L"Terminates the process' threads", TerminatorTT1 },
    { L"TT2", L"Modifies the process' threads with contexts which terminate the process", TerminatorTT2 },
    { L"TP1a", L"Terminates the process using NtTerminateProcess (alternative method)", TerminatorTP1a },
    { L"TT1a", L"Terminates the process' threads (alternative method)", TerminatorTT1a },
    { L"CH1", L"Closes the process' handles", TerminatorCH1 },
    { L"W1", L"Sends the WM_DESTROY message to the process' windows", TerminatorW1 },
    { L"W2", L"Sends the WM_QUIT message to the process' windows", TerminatorW2 },
    { L"W3", L"Sends the WM_CLOSE message to the process' windows", TerminatorW3 },
    { L"TJ1", L"Assigns the process to a job object and terminates the job", TerminatorTJ1 },
    { L"TD1", L"Debugs the process and closes the debug object", TerminatorTD1 },
    { L"TP3", L"Terminates the process in kernel-mode", TerminatorTP3 },
    { L"TT3", L"Terminates the process' threads in kernel-mode", TerminatorTT3 },
    { L"TT4", L"Terminates the process' threads using a dangerous kernel-mode method", TerminatorTT4 },
    { L"M1", L"Writes garbage to the process' memory regions", TerminatorM1 },
    { L"M2", L"Sets the page protection of the process' memory regions to PAGE_NOACCESS", TerminatorM2 }
};

static BOOLEAN PhpRunTerminatorTest(
    __in HWND WindowHandle,
    __in INT Index
    )
{
    NTSTATUS status;
    PTEST_ITEM testItem;
    PPH_PROCESS_ITEM processItem;
    HWND lvHandle;
    PVOID processes;
    BOOLEAN success = FALSE;
    LARGE_INTEGER interval;

    processItem = (PPH_PROCESS_ITEM)GetProp(WindowHandle, L"ProcessItem");
    lvHandle = GetDlgItem(WindowHandle, IDC_TERMINATOR_LIST);

    if (!PhGetListViewItemParam(
        lvHandle,
        Index,
        &testItem
        ))
        return FALSE;

    if (WSTR_EQUAL(testItem->Id, L"TT4"))
    {
        if (!PhShowConfirmMessage(
            WindowHandle,
            L"run",
            L"the TT4 test",
            L"The TT4 test may cause the system to crash.",
            TRUE
            ))
            return FALSE;
    }

    status = testItem->TestProc(processItem->ProcessId);
    interval.QuadPart = -1000 * PH_TIMEOUT_MS;
    NtDelayExecution(FALSE, &interval);

    if (status == STATUS_NOT_SUPPORTED)
    {
        PPH_STRING concat;

        concat = PhConcatStrings2(L"(Not available) ", testItem->Description);
        PhSetListViewSubItem(lvHandle, Index, 1, concat->Buffer);
        PhDereferenceObject(concat);
    }

    if (!NT_SUCCESS(PhEnumProcesses(&processes)))
        return FALSE;

    // Check if the process exists.
    if (!PhFindProcessInformation(processes, processItem->ProcessId))
    {
        PhSetListViewItemImageIndex(lvHandle, Index, TICK_INDEX);
        SetDlgItemText(WindowHandle, IDC_TERMINATOR_TEXT, L"The process was terminated.");
        success = TRUE;
    }
    else
    {
        PhSetListViewItemImageIndex(lvHandle, Index, CROSS_INDEX);
    }

    PhFree(processes);

    UpdateWindow(WindowHandle);

    return success;
}

static INT_PTR CALLBACK PhpProcessTerminatorDlgProc(      
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
            PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)lParam;
            PPH_STRING title;
            HWND lvHandle;
            HIMAGELIST imageList;
            ULONG i;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            title = PhFormatString(
                L"Terminator - %s (%u)",
                processItem->ProcessName->Buffer,
                (ULONG)processItem->ProcessId
                );
            SetWindowText(hwndDlg, title->Buffer);
            PhDereferenceObject(title);

            SetProp(hwndDlg, L"ProcessItem", (HANDLE)processItem);

            lvHandle = GetDlgItem(hwndDlg, IDC_TERMINATOR_LIST);
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"ID");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 300, L"Description");
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");

            imageList = ImageList_Create(16, 16, ILC_COLOR32, 0, 0);
            ImageList_SetImageCount(imageList, 2);
            PhSetImageListBitmap(imageList, CROSS_INDEX, PhInstanceHandle, MAKEINTRESOURCE(IDB_CROSS));
            PhSetImageListBitmap(imageList, TICK_INDEX, PhInstanceHandle, MAKEINTRESOURCE(IDB_TICK));

            for (i = 0; i < sizeof(PhTerminatorTests) / sizeof(TEST_ITEM); i++)
            {
                INT itemIndex;

                itemIndex = PhAddListViewItem(
                    lvHandle,
                    MAXINT,
                    PhTerminatorTests[i].Id,
                    &PhTerminatorTests[i]
                    );
                PhSetListViewSubItem(lvHandle, itemIndex, 1, PhTerminatorTests[i].Description);
                PhSetListViewItemImageIndex(lvHandle, itemIndex, -1);
            }

            ListView_SetImageList(lvHandle, imageList, LVSIL_SMALL);

            SetDlgItemText(
                hwndDlg,
                IDC_TERMINATOR_TEXT,
                L"Double-click a termination method or click Run All."
                );
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"ProcessItem");
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case IDCANCEL: // Esc and X button to close
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_RUN_ALL:
                {
                    if (PhShowConfirmMessage(hwndDlg, L"run", L"the terminator tests", NULL, FALSE))
                    {
                        ULONG i;

                        for (i = 0; i < sizeof(PhTerminatorTests) / sizeof(TEST_ITEM); i++)
                        {
                            if (PhpRunTerminatorTest(
                                hwndDlg,
                                i
                                ))
                                break;
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (
                header->hwndFrom == GetDlgItem(hwndDlg, IDC_TERMINATOR_LIST) &&
                header->code == NM_DBLCLK
                )
            {
                LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;

                if (itemActivate->iItem != -1)
                {
                    if (PhShowConfirmMessage(hwndDlg, L"run", L"the selected test", NULL, FALSE))
                    {
                        PhpRunTerminatorTest(
                            hwndDlg,
                            itemActivate->iItem
                            );
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}
