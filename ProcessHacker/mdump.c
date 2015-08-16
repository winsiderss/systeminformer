/*
 * Process Hacker -
 *   minidump writer
 *
 * Copyright (C) 2010-2015 wj32
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

#pragma warning(push)
#pragma warning(disable: 4091) // Ignore 'no variable declared on typedef'
#include <dbghelp.h>
#pragma warning(pop)

#include <symprv.h>
#include <settings.h>
#include <phsvccl.h>

#define WM_PH_MINIDUMP_STATUS_UPDATE (WM_APP + 301)

#define PH_MINIDUMP_STATUS_UPDATE 1
#define PH_MINIDUMP_COMPLETED 2
#define PH_MINIDUMP_ERROR 3

typedef struct _PROCESS_MINIDUMP_CONTEXT
{
    HANDLE ProcessId;
    PWSTR FileName;
    MINIDUMP_TYPE DumpType;
    BOOLEAN IsWow64;

    HANDLE ProcessHandle;
    HANDLE FileHandle;

    HWND WindowHandle;
    HANDLE ThreadHandle;
    BOOLEAN Stop;
    BOOLEAN Succeeded;

    ULONG LastTickCount;
} PROCESS_MINIDUMP_CONTEXT, *PPROCESS_MINIDUMP_CONTEXT;

BOOLEAN PhpCreateProcessMiniDumpWithProgress(
    _In_ HWND hWnd,
    _In_ HANDLE ProcessId,
    _In_ PWSTR FileName,
    _In_ MINIDUMP_TYPE DumpType
    );

INT_PTR CALLBACK PhpProcessMiniDumpDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

BOOLEAN PhUiCreateDumpFileProcess(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process
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
    PhSetFileDialogFileName(fileDialog, PhaConcatStrings2(Process->ProcessName->Buffer, L".dmp")->Buffer);

    if (!PhShowFileDialog(hWnd, fileDialog))
    {
        PhFreeFileDialog(fileDialog);
        return FALSE;
    }

    fileName = PhAutoDereferenceObject(PhGetFileDialogFileName(fileDialog));
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
    _In_ HWND hWnd,
    _In_ HANDLE ProcessId,
    _In_ PWSTR FileName,
    _In_ MINIDUMP_TYPE DumpType
    )
{
    NTSTATUS status;
    PROCESS_MINIDUMP_CONTEXT context;

    memset(&context, 0, sizeof(PROCESS_MINIDUMP_CONTEXT));
    context.ProcessId = ProcessId;
    context.FileName = FileName;
    context.DumpType = DumpType;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &context.ProcessHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessId
        )))
    {
        PhShowStatus(hWnd, L"Unable to open the process", status, 0);
        return FALSE;
    }

#ifdef _WIN64
    PhGetProcessIsWow64(context.ProcessHandle, &context.IsWow64);
#endif

    status = PhCreateFileWin32(
        &context.FileHandle,
        FileName,
        FILE_GENERIC_WRITE | DELETE,
        0,
        0,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(hWnd, L"Unable to access the dump file", status, 0);
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
    _In_ PVOID CallbackParam,
    _In_ const PMINIDUMP_CALLBACK_INPUT CallbackInput,
    _Inout_ PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
    )
{
    PPROCESS_MINIDUMP_CONTEXT context = CallbackParam;
    PPH_STRING message = NULL;

    // Don't try to send status updates if we're creating a dump of the current process.
    if (context->ProcessId == NtCurrentProcessId())
        return TRUE;

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
    _In_ PVOID Parameter
    )
{
    PPROCESS_MINIDUMP_CONTEXT context = Parameter;
    MINIDUMP_CALLBACK_INFORMATION callbackInfo;

    callbackInfo.CallbackRoutine = PhpProcessMiniDumpCallback;
    callbackInfo.CallbackParam = context;

#ifdef _WIN64
    if (context->IsWow64)
    {
        if (PhUiConnectToPhSvcEx(NULL, Wow64PhSvcMode, FALSE))
        {
            NTSTATUS status;
            PPH_STRING dbgHelpPath;

            dbgHelpPath = PhGetStringSetting(L"DbgHelpPath");
            PhSvcCallLoadDbgHelp(dbgHelpPath->Buffer);
            PhDereferenceObject(dbgHelpPath);

            if (NT_SUCCESS(status = PhSvcCallWriteMiniDumpProcess(
                context->ProcessHandle,
                context->ProcessId,
                context->FileHandle,
                context->DumpType
                )))
            {
                context->Succeeded = TRUE;
            }
            else
            {
                // We may have an old version of dbghelp - in that case, try using minimal dump flags.
                if (status == STATUS_INVALID_PARAMETER && NT_SUCCESS(status = PhSvcCallWriteMiniDumpProcess(
                    context->ProcessHandle,
                    context->ProcessId,
                    context->FileHandle,
                    MiniDumpWithFullMemory | MiniDumpWithHandleData
                    )))
                {
                    context->Succeeded = TRUE;
                }
                else
                {
                    SendMessage(
                        context->WindowHandle,
                        WM_PH_MINIDUMP_STATUS_UPDATE,
                        PH_MINIDUMP_ERROR,
                        (LPARAM)PhNtStatusToDosError(status)
                        );
                }
            }

            PhUiDisconnectFromPhSvc();

            goto Completed;
        }
        else
        {
            if (PhShowMessage(
                context->WindowHandle,
                MB_YESNO | MB_ICONWARNING,
                L"The process is 32-bit, but the 32-bit version of Process Hacker could not be located. "
                L"A 64-bit dump will be created instead. Do you want to continue?"
                ) == IDNO)
            {
                FILE_DISPOSITION_INFORMATION dispositionInfo;
                IO_STATUS_BLOCK isb;

                dispositionInfo.DeleteFile = TRUE;
                NtSetInformationFile(
                    context->FileHandle,
                    &isb,
                    &dispositionInfo,
                    sizeof(FILE_DISPOSITION_INFORMATION),
                    FileDispositionInformation
                    );

                goto Completed;
            }
        }
    }
#endif

    if (PhWriteMiniDumpProcess(
        context->ProcessHandle,
        context->ProcessId,
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
        // We may have an old version of dbghelp - in that case, try using minimal dump flags.
        if (GetLastError() == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) && PhWriteMiniDumpProcess(
            context->ProcessHandle,
            context->ProcessId,
            context->FileHandle,
            MiniDumpWithFullMemory | MiniDumpWithHandleData,
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
    }

#ifdef _WIN64
Completed:
#endif
    SendMessage(
        context->WindowHandle,
        WM_PH_MINIDUMP_STATUS_UPDATE,
        PH_MINIDUMP_COMPLETED,
        0
        );

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PhpProcessMiniDumpDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPROCESS_MINIDUMP_CONTEXT context = (PPROCESS_MINIDUMP_CONTEXT)lParam;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);

            SetDlgItemText(hwndDlg, IDC_PROGRESSTEXT, L"Creating the dump file...");

            PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_PROGRESS), PBS_MARQUEE, PBS_MARQUEE);
            SendMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 75);

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

            context = (PPROCESS_MINIDUMP_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

            NtClose(context->ThreadHandle);

            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    PPROCESS_MINIDUMP_CONTEXT context =
                        (PPROCESS_MINIDUMP_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

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
                    (PPROCESS_MINIDUMP_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());
                ULONG currentTickCount;

                currentTickCount = GetTickCount();

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

            context = (PPROCESS_MINIDUMP_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

            switch (wParam)
            {
            case PH_MINIDUMP_STATUS_UPDATE:
                SetDlgItemText(hwndDlg, IDC_PROGRESSTEXT, (PWSTR)lParam);
                InvalidateRect(GetDlgItem(hwndDlg, IDC_PROGRESSTEXT), NULL, FALSE);
                context->LastTickCount = GetTickCount();
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
