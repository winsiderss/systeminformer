/*
 * Process Hacker - 
 *   minidump writer
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
#include <symprvp.h>

#define WM_PH_MINIDUMP_STATUS_UPDATE (WM_APP + 301)

#define PH_MINIDUMP_STATUS_UPDATE 1
#define PH_MINIDUMP_COMPLETED 2
#define PH_MINIDUMP_ERROR 3

typedef struct _PROCESS_MINIDUMP_CONTEXT
{
    HANDLE ProcessId;
    PWSTR FileName;
    MINIDUMP_TYPE DumpType;

    HANDLE ProcessHandle;
    HANDLE FileHandle;

    HWND WindowHandle;
    HANDLE ThreadHandle;
    BOOLEAN Stop;
    BOOLEAN Succeeded;

    ULONG64 LastTickCount;
} PROCESS_MINIDUMP_CONTEXT, *PPROCESS_MINIDUMP_CONTEXT;

BOOLEAN PhpCreateProcessMiniDumpWithProgress(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PWSTR FileName,
    __in MINIDUMP_TYPE DumpType
    );

INT_PTR CALLBACK PhpProcessMiniDumpDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

BOOLEAN PhUiCreateDumpFileProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"Dump files (*.dmp)", L"*.dmp" },
        { L"All files (*.*)", L"*.*" }
    };
    PVOID fileDialog;
    PPH_STRING fileName;

    fileDialog = PhCreateSaveFileDialog();
    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

    if (!PhShowFileDialog(hWnd, fileDialog))
    {
        PhFreeFileDialog(fileDialog);
        return FALSE;
    }

    fileName = PHA_DEREFERENCE(PhGetFileDialogFileName(fileDialog));
    PhFreeFileDialog(fileDialog);

    return PhpCreateProcessMiniDumpWithProgress(
        hWnd,
        Process->ProcessId,
        fileName->Buffer,
        // task manager uses these flags
        MiniDumpWithFullMemory |
        MiniDumpWithHandleData |
        MiniDumpWithUnloadedModules |
        MiniDumpWithFullMemoryInfo |
        MiniDumpWithThreadInfo
        );
}

BOOLEAN PhpCreateProcessMiniDumpWithProgress(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PWSTR FileName,
    __in MINIDUMP_TYPE DumpType
    )
{
    NTSTATUS status;
    PROCESS_MINIDUMP_CONTEXT context;

    context.ProcessId = ProcessId;
    context.FileName = FileName;
    context.DumpType = DumpType;

    context.Stop = FALSE;
    context.Succeeded = FALSE;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &context.ProcessHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessId
        )))
    {
        PhShowStatus(hWnd, L"Unable to open the process", status, 0);
        return FALSE;
    }

    context.FileHandle = CreateFile(
        FileName,
        FILE_GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (context.FileHandle == INVALID_HANDLE_VALUE)
    {
        PhShowStatus(hWnd, L"Unable to access the dump file", 0, GetLastError());
        NtClose(context.ProcessHandle);
        return FALSE;
    }

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PROGRESS),
        hWnd,
        PhpProcessMiniDumpDlgProc,
        (LPARAM)&context
        );

    NtClose(context.FileHandle);
    NtClose(context.ProcessHandle);

    return context.Succeeded;
}

static BOOL CALLBACK PhpProcessMiniDumpCallback(
    __in PVOID CallbackParam,
    __in const PMINIDUMP_CALLBACK_INPUT CallbackInput,
    __inout PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
    )
{
    PPROCESS_MINIDUMP_CONTEXT context = CallbackParam;
    PPH_STRING message = NULL;

    // MiniDumpWriteDump seems to get bored of calling the callback
    // after it begins dumping the process handles. The code is 
    // still here in case they fix this problem in the future.

    switch (CallbackInput->CallbackType)
    {
    case CancelCallback:
        {
            if (context->Stop)
                CallbackOutput->Cancel = TRUE;
        }
        break;
    case ModuleCallback:
        {
            message = PhFormatString(L"Processing module %s...", CallbackInput->Module.FullPath);
        }
        break;
    case ThreadCallback:
        {
            message = PhFormatString(L"Processing thread %u...", CallbackInput->Thread.ThreadId);
        }
        break;
    }

    if (message)
    {
        SendMessage(
            context->WindowHandle,
            WM_PH_MINIDUMP_STATUS_UPDATE,
            PH_MINIDUMP_STATUS_UPDATE,
            (LPARAM)message->Buffer
            );
        PhDereferenceObject(message);
    }

    return TRUE;
}

NTSTATUS PhpProcessMiniDumpThreadStart(
    __in PVOID Parameter
    )
{
    PPROCESS_MINIDUMP_CONTEXT context = Parameter;
    MINIDUMP_CALLBACK_INFORMATION callbackInfo;

    callbackInfo.CallbackRoutine = PhpProcessMiniDumpCallback;
    callbackInfo.CallbackParam = context;

    if (MiniDumpWriteDump_I(
        context->ProcessHandle,
        (ULONG)context->ProcessId,
        context->FileHandle,
        context->DumpType,
        NULL,
        NULL,
        &callbackInfo
        ))
    {
        context->Succeeded = TRUE;
    }
    else
    {
        SendMessage(
            context->WindowHandle,
            WM_PH_MINIDUMP_STATUS_UPDATE,
            PH_MINIDUMP_ERROR,
            (LPARAM)GetLastError()
            );
    }

    SendMessage(
        context->WindowHandle,
        WM_PH_MINIDUMP_STATUS_UPDATE,
        PH_MINIDUMP_COMPLETED,
        0
        );

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PhpProcessMiniDumpDlgProc(
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
            PPROCESS_MINIDUMP_CONTEXT context = (PPROCESS_MINIDUMP_CONTEXT)lParam;

            SetProp(hwndDlg, L"Context", (HANDLE)context);

            SetDlgItemText(hwndDlg, IDC_PROGRESSTEXT, L"Creating the dump file...");

            if (WindowsVersion >= WINDOWS_XP)
                SendMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 250);
            else
                ShowWindow(GetDlgItem(hwndDlg, IDC_PROGRESS), SW_HIDE);

            context->WindowHandle = hwndDlg;
            context->ThreadHandle = PhCreateThread(0, PhpProcessMiniDumpThreadStart, context);

            if (!context->ThreadHandle)
            {
                PhShowStatus(hwndDlg, L"Unable to create the minidump thread", 0, GetLastError());
                EndDialog(hwndDlg, IDCANCEL);
            }

            SetTimer(hwndDlg, 1, 500, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PPROCESS_MINIDUMP_CONTEXT context;

            context = (PPROCESS_MINIDUMP_CONTEXT)GetProp(hwndDlg, L"Context");

            NtClose(context->ThreadHandle);

            RemoveProp(hwndDlg, L"Context");
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    PPROCESS_MINIDUMP_CONTEXT context =
                        (PPROCESS_MINIDUMP_CONTEXT)GetProp(hwndDlg, L"Context");

                    EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);
                    context->Stop = TRUE;
                }
                break;
            }
        }
        break;
    case WM_TIMER:
        {
            if (wParam == 1)
            {
                PPROCESS_MINIDUMP_CONTEXT context =
                    (PPROCESS_MINIDUMP_CONTEXT)GetProp(hwndDlg, L"Context");
                ULONG64 currentTickCount;

                currentTickCount = NtGetTickCount64();

                if (currentTickCount - context->LastTickCount >= 2000)
                {
                    // No status message update for 2 seconds.

                    SetDlgItemText(hwndDlg, IDC_PROGRESSTEXT,
                        (PWSTR)L"Creating the dump file...");
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PROGRESSTEXT), NULL, FALSE);

                    context->LastTickCount = currentTickCount;
                }
            }
        }
        break;
    case WM_PH_MINIDUMP_STATUS_UPDATE:
        {
            PPROCESS_MINIDUMP_CONTEXT context;

            context = (PPROCESS_MINIDUMP_CONTEXT)GetProp(hwndDlg, L"Context");

            switch (wParam)
            {
            case PH_MINIDUMP_STATUS_UPDATE:
                SetDlgItemText(hwndDlg, IDC_PROGRESSTEXT, (PWSTR)lParam);
                InvalidateRect(GetDlgItem(hwndDlg, IDC_PROGRESSTEXT), NULL, FALSE);
                context->LastTickCount = NtGetTickCount64();
                break;
            case PH_MINIDUMP_ERROR:
                PhShowStatus(hwndDlg, L"Unable to create the minidump", 0, (ULONG)lParam);
                break;
            case PH_MINIDUMP_COMPLETED:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}
