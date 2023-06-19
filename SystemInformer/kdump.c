/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2019-2022
 *
 */

#include <phapp.h>

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
            BOOLEAN IncludeKernelThreadStack : 1;
            BOOLEAN UseDumpStorageStack : 1;
            BOOLEAN Spare : 2;
        };
    };

    HANDLE FileHandle;
    HANDLE EventHandle;
} LIVE_DUMP_CONFIG, *PLIVE_DUMP_CONFIG;

NTSTATUS PhpCreateLiveKernelDump(
    _In_ PLIVE_DUMP_CONFIG Context
    )
{
    NTSTATUS status;
    SYSDBG_LIVEDUMP_CONTROL liveDumpControl;
    SYSDBG_LIVEDUMP_CONTROL_FLAGS flags;
    SYSDBG_LIVEDUMP_CONTROL_ADDPAGES pages;
    SYSDBG_LIVEDUMP_SELECTIVE_CONTROL selective;

     // HACK: Give some time for the progress window to become visible. (dmex)
    PhDelayExecution(2000);

    memset(&liveDumpControl, 0, sizeof(SYSDBG_LIVEDUMP_CONTROL));
    memset(&flags, 0, sizeof(SYSDBG_LIVEDUMP_CONTROL_FLAGS));
    memset(&pages, 0, sizeof(SYSDBG_LIVEDUMP_CONTROL_ADDPAGES));
    memset(&selective, 0, sizeof(SYSDBG_LIVEDUMP_SELECTIVE_CONTROL));

    if (Context->UseDumpStorageStack)
        flags.UseDumpStorageStack = TRUE;
    if (Context->CompressMemoryPages)
        flags.CompressMemoryPagesData = TRUE;
    if (Context->IncludeUserSpaceMemory)
        flags.IncludeUserSpaceMemoryPages = TRUE;
    if (Context->IncludeHypervisorPages)
        pages.HypervisorPages = TRUE;

    if (Context->IncludeKernelThreadStack)
    {
        selective.Version = SYSDBG_LIVEDUMP_SELECTIVE_CONTROL_VERSION;
        selective.Size = sizeof(SYSDBG_LIVEDUMP_SELECTIVE_CONTROL);
        selective.ThreadKernelStacks = TRUE;

        liveDumpControl.SelectiveControl = &selective;
    }

    liveDumpControl.Version = SYSDBG_LIVEDUMP_CONTROL_VERSION;
    liveDumpControl.DumpFileHandle = Context->FileHandle;
    liveDumpControl.CancelEventHandle = Context->EventHandle;
    liveDumpControl.Flags = flags;
    liveDumpControl.AddPagesControl = pages;

    status = NtSystemDebugControl(
        SysDbgGetLiveKernelDump,
        &liveDumpControl,
        WindowsVersion >= WINDOWS_11 ? SYSDBG_LIVEDUMP_CONTROL_SIZE_WIN11 : SYSDBG_LIVEDUMP_CONTROL_SIZE,
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
                        PhSetFileDelete(context->FileHandle);

                    NtClose(context->FileHandle);
                    context->FileHandle = NULL;
                }

                memset(&config, 0, sizeof(TASKDIALOGCONFIG));
                config.cbSize = sizeof(TASKDIALOGCONFIG);

                if (NT_SUCCESS(context->LastStatus))
                {
                    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
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
                    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
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
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CALLBACK_TIMER | TDF_CAN_BE_MINIMIZED;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pfCallback = PhpLiveDumpProgressDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Processing live kernel dump...";
    config.pszContent = L" ";
    config.cxWidth = 200;

    TaskDialogIndirect(&config, NULL, NULL, NULL);

    if (context->EventHandle)
        NtClose(context->EventHandle);
    if (context->FileName)
        PhDereferenceObject(context->FileName);

    PhFree(context);

    return STATUS_SUCCESS;
}

PPH_STRING PhpLiveDumpFileDialogFileName(
    _In_ HWND WindowHandle
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"Dump files (*.dmp)", L"*.dmp" },
        { L"All files (*.*)", L"*.*" }
    };
    PPH_STRING fileName = NULL;
    PVOID fileDialog;
    LARGE_INTEGER time;
    SYSTEMTIME systemTime;
    PPH_STRING dateString;
    PPH_STRING timeString;
    PPH_STRING suggestedFileName;

    PhQuerySystemTime(&time);
    PhLargeIntegerToLocalSystemTime(&systemTime, &time);
    dateString = PH_AUTO_T(PH_STRING, PhFormatDate(&systemTime, L"yyyy-MM-dd"));
    timeString = PH_AUTO_T(PH_STRING, PhFormatTime(&systemTime, L"HH-mm-ss"));
    suggestedFileName = PH_AUTO_T(PH_STRING, PhFormatString(
        L"%s_%s_%s.dmp",
        L"kerneldump",
        PhGetString(dateString),
        PhGetString(timeString)
        ));

    if (fileDialog = PhCreateSaveFileDialog())
    {
        PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));
        PhSetFileDialogFileName(fileDialog, PhGetString(suggestedFileName));
        PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_DONTADDTORECENT);

        if (PhShowFileDialog(WindowHandle, fileDialog))
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
            BOOLEAN kernelDebuggerEnabled;

            PhSetApplicationWindowIcon(hwndDlg);

            PhCenterWindow(hwndDlg, NULL);

            if (NT_SUCCESS(PhGetKernelDebuggerInformation(&kernelDebuggerEnabled, NULL)) && !kernelDebuggerEnabled)
            {
                Button_Enable(GetDlgItem(hwndDlg, IDC_USERMODE), FALSE);
                Button_SetText(GetDlgItem(hwndDlg, IDC_USERMODE), L"Include UserSpace (requires kernel debug enabled)");
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

                    if (!PhGetOwnTokenAttributes().Elevated)
                    {
                        PhShowStatus(hwndDlg, L"Unable to create live kernel dump.", 0, ERROR_ELEVATION_REQUIRED);
                        break;
                    }

                    dumpConfig = PhAllocateZero(sizeof(LIVE_DUMP_CONFIG));
                    dumpConfig->CompressMemoryPages = Button_GetCheck(GetDlgItem(hwndDlg, IDC_COMPRESS)) == BST_CHECKED;
                    dumpConfig->IncludeUserSpaceMemory = Button_GetCheck(GetDlgItem(hwndDlg, IDC_USERMODE)) == BST_CHECKED;
                    dumpConfig->IncludeHypervisorPages = Button_GetCheck(GetDlgItem(hwndDlg, IDC_HYPERVISOR)) == BST_CHECKED;
                    dumpConfig->IncludeKernelThreadStack = Button_GetCheck(GetDlgItem(hwndDlg, IDC_DUMPKERNELTHREADSTACKS)) == BST_CHECKED;
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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID PhShowLiveDumpDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_LIVEDUMP),
        NULL,
        PhpLiveDumpDlgProc,
        ParentWindowHandle
        );
}
