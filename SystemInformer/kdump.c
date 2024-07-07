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

typedef struct _PH_LIVE_DUMP_CONFIG
{
    HWND WindowHandle;
    PPH_STRING FileName;
    NTSTATUS LastStatus;
    BOOLEAN KernelDumpActive;
    PH_LIVE_DUMP_OPTIONS Options;
    HANDLE FileHandle;
    HANDLE EventHandle;
} PH_LIVE_DUMP_CONFIG, *PPH_LIVE_DUMP_CONFIG;

NTSTATUS PhpCreateLiveKernelDump(
    _In_ PPH_LIVE_DUMP_CONFIG Context
    )
{
    NTSTATUS status;
    SYSDBG_LIVEDUMP_CONTROL liveDumpControl;
    SYSDBG_LIVEDUMP_CONTROL_FLAGS flags;
    SYSDBG_LIVEDUMP_CONTROL_ADDPAGES pages;
    SYSDBG_LIVEDUMP_SELECTIVE_CONTROL selective;
    ULONG length;

    memset(&liveDumpControl, 0, sizeof(SYSDBG_LIVEDUMP_CONTROL));
    memset(&flags, 0, sizeof(SYSDBG_LIVEDUMP_CONTROL_FLAGS));
    memset(&pages, 0, sizeof(SYSDBG_LIVEDUMP_CONTROL_ADDPAGES));
    memset(&selective, 0, sizeof(SYSDBG_LIVEDUMP_SELECTIVE_CONTROL));

    if (Context->Options.UseDumpStorageStack)
        flags.UseDumpStorageStack = TRUE;
    if (Context->Options.CompressMemoryPages)
        flags.CompressMemoryPagesData = TRUE;
    if (Context->Options.IncludeUserSpaceMemory)
        flags.IncludeUserSpaceMemoryPages = TRUE;
    if (Context->Options.IncludeHypervisorPages)
        pages.HypervisorPages = TRUE;
    if (Context->Options.IncludeNonEssentialHypervisorPages)
        pages.NonEssentialHypervisorPages = TRUE;

    if (Context->Options.OnlyKernelThreadStacks)
    {
        liveDumpControl.Version = SYSDBG_LIVEDUMP_CONTROL_VERSION_2;
        flags.SelectiveDump = TRUE;
        length = sizeof(SYSDBG_LIVEDUMP_CONTROL);

        selective.Version = SYSDBG_LIVEDUMP_SELECTIVE_CONTROL_VERSION;
        selective.Size = sizeof(SYSDBG_LIVEDUMP_SELECTIVE_CONTROL);
        selective.ThreadKernelStacks = TRUE;

        liveDumpControl.SelectiveControl = &selective;
    }
    else
    {
        liveDumpControl.Version = SYSDBG_LIVEDUMP_CONTROL_VERSION_1;
        length = RTL_SIZEOF_THROUGH_FIELD(SYSDBG_LIVEDUMP_CONTROL, AddPagesControl);
    }

    liveDumpControl.DumpFileHandle = Context->FileHandle;
    liveDumpControl.CancelEventHandle = Context->EventHandle;
    liveDumpControl.Flags = flags;
    liveDumpControl.AddPagesControl = pages;

    status = NtSystemDebugControl(
        SysDbgGetLiveKernelDump,
        &liveDumpControl,
        length,
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
    PPH_LIVE_DUMP_CONFIG context = (PPH_LIVE_DUMP_CONFIG)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            context->WindowHandle = hwndDlg;
            context->KernelDumpActive = TRUE;
            context->LastStatus = STATUS_SUCCESS;

            PhCreateEvent(
                &context->EventHandle,
                EVENT_ALL_ACCESS,
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
    PPH_LIVE_DUMP_CONFIG context = (PPH_LIVE_DUMP_CONFIG)ThreadParameter;
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

VOID PhUiCreateLiveDump(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_LIVE_DUMP_OPTIONS Options
    )
{
    PPH_LIVE_DUMP_CONFIG dumpConfig;

    dumpConfig = PhAllocateZero(sizeof(PH_LIVE_DUMP_CONFIG));
    dumpConfig->Options = *Options;
    dumpConfig->FileName = PhpLiveDumpFileDialogFileName(ParentWindowHandle);

    if (!PhIsNullOrEmptyString(dumpConfig->FileName))
    {
        PhCreateThread2(PhpLiveDumpTaskDialogThread, dumpConfig);
    }
    else
    {
        if (dumpConfig->FileName)
            PhDereferenceObject(dumpConfig->FileName);
        PhFree(dumpConfig);
    }
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
            PhSetApplicationWindowIcon(hwndDlg);

            PhCenterWindow(hwndDlg, NULL);

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
                    PH_LIVE_DUMP_OPTIONS options;

                    if (!PhGetOwnTokenAttributes().Elevated)
                    {
                        PhShowStatus(hwndDlg, L"Unable to create live kernel dump.", 0, ERROR_ELEVATION_REQUIRED);
                        break;
                    }

                    memset(&options, 0, sizeof(PH_LIVE_DUMP_OPTIONS));

                    options.CompressMemoryPages = Button_GetCheck(GetDlgItem(hwndDlg, IDC_COMPRESS)) == BST_CHECKED;
                    options.IncludeUserSpaceMemory = Button_GetCheck(GetDlgItem(hwndDlg, IDC_USERMODE)) == BST_CHECKED;
                    options.IncludeHypervisorPages = Button_GetCheck(GetDlgItem(hwndDlg, IDC_HYPERVISOR)) == BST_CHECKED;
                    options.OnlyKernelThreadStacks = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ONLYKERNELTHREADSTACKS)) == BST_CHECKED;
                    options.UseDumpStorageStack = Button_GetCheck(GetDlgItem(hwndDlg, IDC_DUMPSTACK)) == BST_CHECKED;
                    options.IncludeNonEssentialHypervisorPages = Button_GetCheck(GetDlgItem(hwndDlg, IDC_HYPERVISORNONESSENTIAL)) == BST_CHECKED;

                    PhUiCreateLiveDump(hwndDlg, &options);

                    EndDialog(hwndDlg, IDCANCEL);
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
        ParentWindowHandle,
        PhpLiveDumpDlgProc,
        NULL
        );
}
