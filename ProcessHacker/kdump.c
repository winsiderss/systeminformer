/*
 * Process Hacker -
 *   Live kernel dump
 *
 * Copyright (C) 2019-2020 dmex
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
#include <phsettings.h>

typedef struct _LIVE_DUMP_CONFIG
{
    HWND WindowHandle;
    PPH_STRING FileName;
    NTSTATUS LastStatus;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN KernelDumpActive : 1;
            BOOLEAN CompressMemoryPages : 1;
            BOOLEAN IncludeUserSpaceMemory : 1;
            BOOLEAN IncludeHypervisorPages : 1;
            BOOLEAN UseDumpStorageStack : 1;
            BOOLEAN Spare : 3;
        };
    };

    HANDLE FileHandle;
    HANDLE EventHandle;
} LIVE_DUMP_CONFIG, * PLIVE_DUMP_CONFIG;

NTSTATUS PhpCreateLiveKernelDump(
    _In_ PLIVE_DUMP_CONFIG Context
    )
{
    NTSTATUS status;
    SYSDBG_LIVEDUMP_CONTROL liveDumpControl;
    SYSDBG_LIVEDUMP_CONTROL_FLAGS flags;
    SYSDBG_LIVEDUMP_CONTROL_ADDPAGES pages;

     // HACK: Give some time for the progress window to become visible. (dmex)
    PhDelayExecution(2000);

    memset(&liveDumpControl, 0, sizeof(SYSDBG_LIVEDUMP_CONTROL));
    memset(&flags, 0, sizeof(SYSDBG_LIVEDUMP_CONTROL_FLAGS));
    memset(&pages, 0, sizeof(SYSDBG_LIVEDUMP_CONTROL_ADDPAGES));

    if (Context->UseDumpStorageStack)
        flags.UseDumpStorageStack = TRUE;
    if (Context->CompressMemoryPages)
        flags.CompressMemoryPagesData = TRUE;
    if (Context->IncludeUserSpaceMemory)
        flags.IncludeUserSpaceMemoryPages = TRUE;
    if (Context->IncludeHypervisorPages)
        pages.HypervisorPages = TRUE;

    liveDumpControl.Version = SYSDBG_LIVEDUMP_CONTROL_VERSION;
    liveDumpControl.DumpFileHandle = Context->FileHandle;
    liveDumpControl.CancelEventHandle = Context->EventHandle;
    liveDumpControl.AddPagesControl = pages;
    liveDumpControl.Flags = flags;

    status = NtSystemDebugControl(
        SysDbgGetLiveKernelDump,
        &liveDumpControl,
        sizeof(SYSDBG_LIVEDUMP_CONTROL),
        NULL,
        0,
        NULL
        );

    Context->LastStatus = status;
    Context->KernelDumpActive = FALSE;

    PostMessage(Context->WindowHandle, TDM_CLICK_BUTTON, IDIGNORE, 0);

    return status;
}

HRESULT CALLBACK PhpLiveDumpPageCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    return S_OK;
}

HRESULT CALLBACK PhpLiveDumpProgressDialogCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PLIVE_DUMP_CONFIG context = (PLIVE_DUMP_CONFIG)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            context->WindowHandle = hwndDlg;
            context->KernelDumpActive = TRUE;
            context->LastStatus = STATUS_SUCCESS;

            NtCreateEvent(
                &context->EventHandle,
                EVENT_ALL_ACCESS,
                NULL,
                SynchronizationEvent,
                FALSE
                );

            PhCreateThread2(PhpCreateLiveKernelDump, context);
        }
        break;
    case TDN_TIMER:
        {
            LARGE_INTEGER fileSize;

            if (NT_SUCCESS(PhGetFileSize(context->FileHandle, &fileSize)))
            {
                PH_FORMAT format[2];
                WCHAR string[MAX_PATH];

                PhInitFormatS(&format[0], L"Size: ");
                PhInitFormatSize(&format[1], fileSize.QuadPart);

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), string, sizeof(string), NULL))
                {
                    SendMessage(context->WindowHandle, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
                }
            }
            else
            {
                SendMessage(context->WindowHandle, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L" ");
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            ULONG buttonId = (ULONG)wParam;

            if (buttonId == IDIGNORE)
            {
                PPH_STRING statusMessage = NULL;
                TASKDIALOGCONFIG config;

                if (context->FileHandle)
                {
                    if (!NT_SUCCESS(context->LastStatus))
                        PhDeleteFile(context->FileHandle);

                    NtClose(context->FileHandle);
                    context->FileHandle = NULL;
                }

                memset(&config, 0, sizeof(TASKDIALOGCONFIG));
                config.cbSize = sizeof(TASKDIALOGCONFIG);

                if (NT_SUCCESS(context->LastStatus))
                {
                    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION;
                    config.hMainIcon = PhGetApplicationIcon(FALSE);
                    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
                    config.pfCallback = PhpLiveDumpPageCallbackProc;
                    config.lpCallbackData = (LONG_PTR)context;
                    config.pszWindowTitle = PhApplicationName;
                    config.pszMainInstruction = L"Live kernel dump has been created.";
                    config.pszContent = PhGetString(context->FileName);
                }
                else
                {
                    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION;
                    config.hMainIcon = PhGetApplicationIcon(FALSE);
                    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
                    config.pfCallback = PhpLiveDumpPageCallbackProc;
                    config.lpCallbackData = (LONG_PTR)context;
                    config.pszWindowTitle = PhApplicationName;
                    config.pszMainInstruction = L"Unable to save the live kernel dump.";

                    statusMessage = PhGetStatusMessage(context->LastStatus, 0);
                    config.pszContent = PhGetString(statusMessage);
                }

                SendMessage(context->WindowHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);

                if (statusMessage)
                    PhDereferenceObject(statusMessage);

                return S_FALSE;
            }

            if (context->KernelDumpActive)
            {
                if (context->EventHandle)
                    NtSetEvent(context->EventHandle, NULL);

                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

NTSTATUS PhpLiveDumpTaskDialogThread(
    _In_ PVOID ThreadParameter
    )
{
    PLIVE_DUMP_CONFIG context = (PLIVE_DUMP_CONFIG)ThreadParameter;
    NTSTATUS status;
    TASKDIALOGCONFIG config;

    status = PhCreateFileWin32(
        &context->FileHandle,
        PhGetString(context->FileName),
        FILE_ALL_ACCESS,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(NULL, L"Unable to save the live kernel dump.", status, 0);
        return status;
    }

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CALLBACK_TIMER;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pfCallback = PhpLiveDumpProgressDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Processing live kernel dump...";
    config.pszContent = L" ";

    TaskDialogIndirect(&config, NULL, NULL, NULL);

    if (context->EventHandle)
        NtClose(context->EventHandle);
    if (context->FileName)
        PhDereferenceObject(context->FileName);

    PhFree(context);

    return STATUS_SUCCESS;
}

PPH_STRING PhpLiveDumpFileDialogFileName(
    _In_ HWND ParentWindow
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"Windows Memory Dump (*.dmp)", L"*.dmp" },
        { L"All files (*.*)", L"*.*" }
    };
    PPH_STRING fileName = NULL;
    PVOID fileDialog;

    if (fileDialog = PhCreateSaveFileDialog())
    {
        PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));
        PhSetFileDialogFileName(fileDialog, L"kerneldump.dmp");

        if (PhShowFileDialog(ParentWindow, fileDialog))
        {
            fileName = PhGetFileDialogFileName(fileDialog);
        }

        PhFreeFileDialog(fileDialog);
    }

    return fileName;
}

INT_PTR CALLBACK PhpLiveDumpDlgProc(
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
            SYSTEM_KERNEL_DEBUGGER_INFORMATION debugInfo;

            PhSetApplicationWindowIcon(hwndDlg);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            if (NT_SUCCESS(NtQuerySystemInformation(
                SystemKernelDebuggerInformation,
                &debugInfo,
                sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION),
                NULL
                )))
            {
                if (!debugInfo.KernelDebuggerEnabled)
                {
                    Button_Enable(GetDlgItem(hwndDlg, IDC_USERMODE), FALSE);
                    //Button_SetText(GetDlgItem(hwndDlg, IDC_USERMODE), L"Include UserSpace memory pages (requires kernel debugger)");
                }
            }

            if (!PhGetOwnTokenAttributes().Elevated)
            {
                Button_SetElevationRequiredState(GetDlgItem(hwndDlg, IDOK), TRUE);
            }

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDOK));

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PLIVE_DUMP_CONFIG dumpConfig;

                    dumpConfig = PhAllocateZero(sizeof(LIVE_DUMP_CONFIG));
                    dumpConfig->CompressMemoryPages = Button_GetCheck(GetDlgItem(hwndDlg, IDC_COMPRESS)) == BST_CHECKED;
                    dumpConfig->IncludeUserSpaceMemory = Button_GetCheck(GetDlgItem(hwndDlg, IDC_USERMODE)) == BST_CHECKED;
                    dumpConfig->IncludeHypervisorPages = Button_GetCheck(GetDlgItem(hwndDlg, IDC_HYPERVISOR)) == BST_CHECKED;
                    dumpConfig->UseDumpStorageStack = Button_GetCheck(GetDlgItem(hwndDlg, IDC_DUMPSTACK)) == BST_CHECKED;

                    dumpConfig->FileName = PhpLiveDumpFileDialogFileName(hwndDlg);

                    if (!PhIsNullOrEmptyString(dumpConfig->FileName))
                    {
                        PhCreateThread2(PhpLiveDumpTaskDialogThread, dumpConfig);

                        EndDialog(hwndDlg, IDOK);
                    }
                    else
                    {
                        if (dumpConfig->FileName)
                            PhDereferenceObject(dumpConfig->FileName);
                        PhFree(dumpConfig);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID PhShowLiveDumpDialog(
    _In_ HWND ParentWindowHandle
    )
{
    DialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_LIVEDUMP),
        NULL,
        PhpLiveDumpDlgProc
        );
}
