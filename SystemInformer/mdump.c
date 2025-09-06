/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2016-2023
 *
 */

#include <phapp.h>
#include <appresolver.h>
#include <settings.h>

#include <dbghelp.h>
#include <symprv.h>

#include <actions.h>
#include <phsvccl.h>
#include <procprv.h>

#define WM_PH_MINIDUMP_STATUS_UPDATE (WM_APP + 301)
#define WM_PH_MINIDUMP_COMPLETED (WM_APP + 302)
#define WM_PH_MINIDUMP_ERROR (WM_APP + 303)

typedef struct _PH_PROCESS_MINIDUMP_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;

    HANDLE ProcessId;
    PPH_PROCESS_ITEM ProcessItem;
    PPH_STRING FileName;
    PPH_STRING ErrorMessage;
    ULONG DumpType;

    HANDLE ProcessHandle;
    HANDLE FileHandle;
    HANDLE KernelFileHandle;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN IsWow64Process : 1;
            BOOLEAN IsProcessSnapshot : 1;
            BOOLEAN Stop : 1;
            BOOLEAN Succeeded : 1;
            BOOLEAN EnableProcessSnapshot : 1;
            BOOLEAN EnableKernelSnapshot : 1;
            BOOLEAN Spare : 2;
        };
    };

    ULONG64 LastTickCount;
    WNDPROC DefaultTaskDialogWindowProc;
} PH_PROCESS_MINIDUMP_CONTEXT, *PPH_PROCESS_MINIDUMP_CONTEXT;

INT_PTR CALLBACK PhpProcessMiniDumpDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpProcessMiniDumpTaskDialogThread(
    _In_ PVOID ThreadParameter
    );

PH_INITONCE PhpProcessMiniDumpContextTypeInitOnce = PH_INITONCE_INIT;
PPH_OBJECT_TYPE PhpProcessMiniDumpContextType = NULL;

PPH_STRING PhpProcessMiniDumpGetFileName(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
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
        PhGetString(ProcessItem->ProcessName),
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

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID PhpProcessMiniDumpContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_PROCESS_MINIDUMP_CONTEXT context = Object;

    if (context->KernelFileHandle)
    {
        LARGE_INTEGER fileSize;

        if (NT_SUCCESS(PhGetFileSize(context->KernelFileHandle, &fileSize)))
        {
            if (fileSize.QuadPart == 0)
            {
                PhSetFileDelete(context->KernelFileHandle);
            }
        }

        NtClose(context->KernelFileHandle);
    }

    if (context->FileHandle)
        NtClose(context->FileHandle);
    if (context->ProcessHandle)
        NtClose(context->ProcessHandle);
    if (context->FileName)
        PhDereferenceObject(context->FileName);
    if (context->ErrorMessage)
        PhDereferenceObject(context->ErrorMessage);
    if (context->ProcessItem)
        PhDereferenceObject(context->ProcessItem);
}

PPH_PROCESS_MINIDUMP_CONTEXT PhpCreateProcessMiniDumpContext(
    VOID
    )
{
    PPH_PROCESS_MINIDUMP_CONTEXT context;

    if (PhBeginInitOnce(&PhpProcessMiniDumpContextTypeInitOnce))
    {
        PhpProcessMiniDumpContextType = PhCreateObjectType(L"ProcessMiniDumpContextObjectType", 0, PhpProcessMiniDumpContextDeleteProcedure);
        PhEndInitOnce(&PhpProcessMiniDumpContextTypeInitOnce);
    }

    context = PhCreateObject(sizeof(PH_PROCESS_MINIDUMP_CONTEXT), PhpProcessMiniDumpContextType);
    memset(context, 0, sizeof(PH_PROCESS_MINIDUMP_CONTEXT));

    return context;
}

VOID PhUiCreateDumpFileProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ MINIDUMP_TYPE DumpType
    )
{
    static ACCESS_MASK processAccess[] =
    {
        PROCESS_ALL_ACCESS,
        PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE | PROCESS_VM_READ,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        PROCESS_QUERY_INFORMATION
    };
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PPH_PROCESS_MINIDUMP_CONTEXT context;
    PPH_STRING fileName;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    fileName = PhpProcessMiniDumpGetFileName(WindowHandle, ProcessItem);

    if (PhIsNullOrEmptyString(fileName))
        return;

    context = PhpCreateProcessMiniDumpContext();
    context->EnableProcessSnapshot = !!PhGetIntegerSetting(L"EnableMinidumpSnapshot");
    context->EnableKernelSnapshot = !!PhGetIntegerSetting(L"EnableMinidumpKernelMinidump");
    context->ParentWindowHandle = WindowHandle;
    context->ProcessId = ProcessItem->ProcessId;
    context->ProcessItem = PhReferenceObject(ProcessItem);
    context->FileName = fileName;
    context->DumpType = DumpType;

    for (ULONG i = 0; i < RTL_NUMBER_OF(processAccess); i++)
    {
        if (NT_SUCCESS(status = PhOpenProcess(
            &context->ProcessHandle,
            processAccess[i],
            context->ProcessId
            )))
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(WindowHandle, L"Unable to open the process", status, 0);
        PhDereferenceObject(context);
        return;
    }

#ifdef _WIN64
    PhGetProcessIsWow64(context->ProcessHandle, &isWow64);
    context->IsWow64Process = !!isWow64;
#endif

    status = PhCreateFileWin32(
        &context->FileHandle,
        PhGetString(fileName),
        FILE_GENERIC_WRITE | DELETE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(WindowHandle, L"Unable to access the dump file", status, 0);
        PhDereferenceObject(context);
        return;
    }

    PhCreateThread2(PhpProcessMiniDumpTaskDialogThread, context);

    //PhDialogBox(
    //    PhInstanceHandle,
    //    MAKEINTRESOURCE(IDD_PROGRESS),
    //    NULL,
    //    PhpProcessMiniDumpDlgProc,
    //    context
    //    );
}

static BOOL CALLBACK PhpProcessMiniDumpCallback(
    _Inout_ PVOID CallbackParam,
    _In_ PMINIDUMP_CALLBACK_INPUT CallbackInput,
    _Inout_ PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
    )
{
    PPH_PROCESS_MINIDUMP_CONTEXT context = CallbackParam;
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

            CallbackOutput->CheckCancel = TRUE;
        }
        break;
    case IsProcessSnapshotCallback:
        {
            if (context->EnableProcessSnapshot && context->IsProcessSnapshot)
                CallbackOutput->Status = S_FALSE;
        }
        break;
    //case VmStartCallback:
    //    {
    //        CallbackOutput->Status = S_FALSE;
    //    }
    //    break;
    //case IncludeVmRegionCallback:
    //    {
    //        CallbackOutput->Continue = TRUE;
    //    }
    //    break;
    case ReadMemoryFailureCallback:
        {
            CallbackOutput->Status = S_OK;
        }
        break;
    case ModuleCallback:
        {
            PH_FORMAT format[3];
            PPH_STRING baseName = NULL;

            if (CallbackInput->Module.FullPath)
            {
                if (baseName = PhCreateString(CallbackInput->Module.FullPath))
                {
                    PhMoveReference(&baseName, PhGetBaseName(baseName));
                    PhMoveReference(&baseName, PhEllipsisStringPath(baseName, 10));
                }
            }

            // Processing module %s...
            PhInitFormatS(&format[0], L"Processing module ");
            if (baseName)
                PhInitFormatSR(&format[1], baseName->sr);
            else
                PhInitFormatS(&format[1], L"");
            PhInitFormatS(&format[2], L"...");

            message = PhFormat(format, RTL_NUMBER_OF(format), 0);
            PhClearReference(&baseName);
        }
        break;
    case ThreadCallback:
    case ThreadExCallback:
        {
            PH_FORMAT format[3];

            // Processing thread %lu...
            PhInitFormatS(&format[0], L"Processing thread ");
            PhInitFormatU(&format[1], CallbackInput->Thread.ThreadId);
            PhInitFormatS(&format[2], L"...");

            message = PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
        break;
    case IncludeVmRegionCallback:
        {
            PH_FORMAT format[2];

            //CallbackOutput->Continue = TRUE;

            // Processing memory %lu...
            PhInitFormatS(&format[0], L"Processing memory regions");
            //PhInitFormatI64X(&format[1], CallbackOutput->VmRegion.BaseAddress);
            PhInitFormatS(&format[1], L"...");

            message = PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
        break;
    case WriteKernelMinidumpCallback:
        {
            static CONST PH_STRINGREF kernelDumpFileExt = PH_STRINGREF_INIT(L".kernel.dmp");
            HANDLE kernelDumpFileHandle;
            PPH_STRING kernelDumpFileName;

            if (!context->EnableKernelSnapshot)
                break;
            if (!PhGetOwnTokenAttributes().Elevated)
                break;

            if (kernelDumpFileName = PhGetBaseNameChangeExtension(&context->FileName->sr, &kernelDumpFileExt))
            {
                if (NT_SUCCESS(PhCreateFileWin32(
                    &kernelDumpFileHandle,
                    PhGetString(kernelDumpFileName),
                    FILE_GENERIC_WRITE | DELETE,
                    FILE_ATTRIBUTE_NORMAL,
                    0,
                    FILE_OVERWRITE_IF,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                    )))
                {
                    context->KernelFileHandle = kernelDumpFileHandle;
                    CallbackOutput->Handle = kernelDumpFileHandle;
                }

                PhDereferenceObject(kernelDumpFileName);
            }
        }
        break;
    case KernelMinidumpStatusCallback:
        {
            PH_FORMAT format[2];

            if (!context->EnableKernelSnapshot)
                break;

            PhInitFormatS(&format[0], L"Processing kernel minidump");
            PhInitFormatS(&format[1], L"...");

            message = PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
        break;
    }

    if (message)
    {
        SendMessage(context->WindowHandle, WM_PH_MINIDUMP_STATUS_UPDATE, 0, (LPARAM)message->Buffer);
        PhDereferenceObject(message);
    }

    return TRUE;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpProcessMiniDumpThreadStart(
    _In_ PVOID Parameter
    )
{
    PPH_PROCESS_MINIDUMP_CONTEXT context = Parameter;
    MINIDUMP_CALLBACK_INFORMATION callbackInfo;
    HANDLE processSnapshotHandle = NULL;
    HANDLE packageTaskHandle = NULL;
    HANDLE processHandle = NULL;
    LONG status;

    callbackInfo.CallbackRoutine = PhpProcessMiniDumpCallback;
    callbackInfo.CallbackParam = context;

#ifdef _WIN64
    if (context->IsWow64Process)
    {
        if (PhUiConnectToPhSvcEx(NULL, Wow64PhSvcMode, FALSE))
        {
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
                SendMessage(context->WindowHandle, WM_PH_MINIDUMP_ERROR, 0, (LPARAM)PhNtStatusToDosError(status));
            }

            PhUiDisconnectFromPhSvc();

            goto Completed;
        }
        else
        {
            if (PhShowMessage2(
                context->WindowHandle,
                TD_YES_BUTTON | TD_NO_BUTTON,
                TD_WARNING_ICON,
                L"The 32-bit version of System Informer could not be located.",
                L"A 64-bit dump will be created instead. Do you want to continue?"
                ) == IDNO)
            {
                goto Completed;
            }
        }
    }
#endif

    if (context->ProcessItem->IsPackagedProcess)
    {
        // Set the task completion notification (based on taskmgr.exe) (dmex)
        //PhAppResolverPackageStopSessionRedirection(context->ProcessItem->PackageFullName);
        PhAppResolverBeginCrashDumpTaskByHandle(context->ProcessHandle, &packageTaskHandle);
    }

    if (context->EnableKernelSnapshot)
    {
        if (!PhGetOwnTokenAttributes().Elevated)
        {
            PhShowWarning2(
                context->WindowHandle,
                L"Unable to create kernel minidump.",
                L"%s",
                L"Kernel minidump of processes require administrative privileges. "
                L"Make sure System Informer is running with administrative privileges."
                );
        }
    }
    else
    {
        if (context->EnableProcessSnapshot && context->ProcessId != NtCurrentProcessId()) // Don't use snapshots for the current process (dmex)
        {
            HANDLE snapshotHandle;

            if (NT_SUCCESS(PhCreateProcessSnapshot(
                &snapshotHandle,
                context->ProcessHandle
                )))
            {
                processSnapshotHandle = snapshotHandle;
                context->IsProcessSnapshot = TRUE;
            }
        }
    }

    if (context->EnableProcessSnapshot && context->IsProcessSnapshot && processSnapshotHandle)
        processHandle = processSnapshotHandle;
    else
        processHandle = context->ProcessHandle;

    status = PhWriteMiniDumpProcess(
        processHandle,
        context->ProcessId,
        context->FileHandle,
        context->DumpType,
        NULL,
        NULL,
        &callbackInfo
        );

    if (HR_SUCCESS(status))
    {
        context->Succeeded = TRUE;
    }
    else
    {
        SendMessage(context->WindowHandle, WM_PH_MINIDUMP_ERROR, 0, (LPARAM)status);
    }

    if (processSnapshotHandle)
        PhFreeProcessSnapshot(processSnapshotHandle, context->ProcessHandle);
    if (packageTaskHandle)
        PhAppResolverEndCrashDumpTask(packageTaskHandle);

#ifdef _WIN64
Completed:
#endif
    if (context->Succeeded)
    {
        SendMessage(context->WindowHandle, WM_PH_MINIDUMP_COMPLETED, 0, 0);
    }
    else
    {
        PhSetFileDelete(context->FileHandle);
    }

    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PhpProcessMiniDumpDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PROCESS_MINIDUMP_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_PROCESS_MINIDUMP_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;

            PhSetApplicationWindowIcon(hwndDlg);
            PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            PhSetWindowText(hwndDlg, L"Creating the dump file...");
            PhSetDialogItemText(hwndDlg, IDC_PROGRESSTEXT, L"Creating the dump file...");
            PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_PROGRESS), PBS_MARQUEE, PBS_MARQUEE);
            SendMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 75);

            PhReferenceObject(context);
            PhCreateThread2(PhpProcessMiniDumpThreadStart, context);

            PhSetTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT, 500, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PhKillTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhDereferenceObject(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);
                    context->Stop = TRUE;
                }
                break;
            }
        }
        break;
    case WM_TIMER:
        {
            if (wParam == PH_WINDOW_TIMER_DEFAULT)
            {
                ULONG64 currentTickCount;

                currentTickCount = NtGetTickCount64();

                if (currentTickCount - context->LastTickCount >= 2000)
                {
                    // No status message update for 2 seconds.

                    PhSetDialogItemText(hwndDlg, IDC_PROGRESSTEXT, L"Creating the dump file...");

                    context->LastTickCount = currentTickCount;
                }
            }
        }
        break;
    case WM_PH_MINIDUMP_STATUS_UPDATE:
        PhSetDialogItemText(hwndDlg, IDC_PROGRESSTEXT, (PWSTR)lParam);
        context->LastTickCount = NtGetTickCount64();
        break;
    case WM_PH_MINIDUMP_ERROR:
        PhShowStatus(hwndDlg, L"Unable to create the minidump", 0, (ULONG)lParam);
        break;
    case WM_PH_MINIDUMP_COMPLETED:
        EndDialog(hwndDlg, IDOK);
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

HRESULT CALLBACK PhpProcessMiniDumpErrorPageCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    return S_OK;
}

LRESULT CALLBACK PhpProcessMiniDumpTaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PROCESS_MINIDUMP_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hwndDlg, 0xF)))
        return 0;

    oldWndProc = context->DefaultTaskDialogWindowProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            PhSetWindowProcedure(hwndDlg, oldWndProc);
            PhRemoveWindowContext(hwndDlg, 0xF);
        }
        break;
    case WM_PH_MINIDUMP_STATUS_UPDATE:
        {
            SendMessage(hwndDlg, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, lParam);
            context->LastTickCount = NtGetTickCount64();
        }
        break;
    case WM_PH_MINIDUMP_ERROR:
        {
            TASKDIALOGCONFIG config;
            PPH_STRING statusMessage;

            if (context->Stop)
            {
                SendMessage(hwndDlg, TDM_CLICK_BUTTON, IDOK, 0);
                break;
            }

            if (statusMessage = PhGetStatusMessage(0, (ULONG)lParam))
            {
                PhMoveReference(&context->ErrorMessage, statusMessage);
            }

            memset(&config, 0, sizeof(TASKDIALOGCONFIG));
            config.cbSize = sizeof(TASKDIALOGCONFIG);
            config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
            config.hMainIcon = PhGetApplicationIcon(FALSE);
            config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
            config.pfCallback = PhpProcessMiniDumpErrorPageCallbackProc;
            config.lpCallbackData = (LONG_PTR)context;
            config.pszWindowTitle = PhApplicationName;
            config.pszMainInstruction = L"Unable to create the minidump.";
            config.pszContent = PhGetStringOrDefault(context->ErrorMessage, L"Unknown error.");

            PhTaskDialogNavigatePage(context->WindowHandle, &config);
        }
        break;
    case WM_PH_MINIDUMP_COMPLETED:
        SendMessage(hwndDlg, TDM_CLICK_BUTTON, IDOK, 0);
        break;
    }

    return CallWindowProc(oldWndProc, hwndDlg, uMsg, wParam, lParam);
}

HRESULT CALLBACK PhpProcessMiniDumpTaskDialogCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_PROCESS_MINIDUMP_CONTEXT context = (PPH_PROCESS_MINIDUMP_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_DIALOG_CONSTRUCTED:
        {
            context->WindowHandle = hwndDlg;

            PhSetApplicationWindowIcon(hwndDlg);
            PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            context->DefaultTaskDialogWindowProc = PhGetWindowProcedure(hwndDlg);
            PhSetWindowContext(hwndDlg, 0xF, context);
            PhSetWindowProcedure(hwndDlg, PhpProcessMiniDumpTaskDialogSubclassProc);

            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            PhReferenceObject(context);
            PhCreateThread2(PhpProcessMiniDumpThreadStart, context);
        }
        break;
    case TDN_TIMER:
        {
            ULONG64 currentTickCount;

            currentTickCount = NtGetTickCount64();

            if (currentTickCount - context->LastTickCount >= 2000)
            {
                // No status message update for 2 seconds.

                //SendMessage(hwndDlg, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"Creating the minidump file...");

                context->LastTickCount = currentTickCount;
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            ULONG buttonId = (ULONG)wParam;

            if (buttonId == IDCANCEL)
            {
                context->Stop = TRUE;
                SendMessage(hwndDlg, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"Cancelling...");
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpProcessMiniDumpTaskDialogThread(
    _In_ PVOID ThreadParameter
    )
{
    PPH_PROCESS_MINIDUMP_CONTEXT context = (PPH_PROCESS_MINIDUMP_CONTEXT)ThreadParameter;
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CALLBACK_TIMER | TDF_CAN_BE_MINIMIZED;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pfCallback = PhpProcessMiniDumpTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)context;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Creating the minidump file...";
    config.pszContent = L"Creating the minidump file...";
    config.cxWidth = 200;

    PhShowTaskDialog(&config, NULL, NULL, NULL);

    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

typedef struct _PH_MINIDUMP_OPTION_ENTRY
{
    ULONG Id;
    MINIDUMP_TYPE Type;
} PH_MINIDUMP_OPTION_ENTRY, *PPH_MINIDUMP_OPTION_ENTRY;

INT_PTR CALLBACK PhpProcDumpDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static CONST PH_MINIDUMP_OPTION_ENTRY options[] =
    {
        { IDC_MINIDUMP_NORMAL, MiniDumpNormal },
        { IDC_MINIDUMP_WITH_DATA_SEGS, MiniDumpWithDataSegs },
        { IDC_MINIDUMP_WITH_FULL_MEM, MiniDumpWithFullMemory },
        { IDC_MINIDUMP_WITH_HANDLE_DATA, MiniDumpWithHandleData },
        { IDC_MINIDUMP_FILTER_MEMORY, MiniDumpFilterMemory },
        { IDC_MINIDUMP_SCAN_MEMORY, MiniDumpScanMemory },
        { IDC_MINIDUMP_WITH_UNLOADED_MODULES, MiniDumpWithUnloadedModules },
        { IDC_MINIDUMP_WITH_INDIRECT_REFERENCED_MEM, MiniDumpWithIndirectlyReferencedMemory },
        { IDC_MINIDUMP_FILTER_MODULE_PATHS, MiniDumpFilterModulePaths },
        { IDC_MINIDUMP_WITH_PROC_THRD_DATA, MiniDumpWithProcessThreadData },
        { IDC_MINIDUMP_WITH_PRIVATE_RW_MEM, MiniDumpWithPrivateReadWriteMemory },
        { IDC_MINIDUMP_WITHOUT_OPTIONAL_DATA, MiniDumpWithoutOptionalData },
        { IDC_MINIDUMP_WITH_FULL_MEM_INFO, MiniDumpWithFullMemoryInfo },
        { IDC_MINIDUMP_WITH_THRD_INFO, MiniDumpWithThreadInfo },
        { IDC_MINIDUMP_WITH_CODE_SEGS, MiniDumpWithCodeSegs },
        { IDC_MINIDUMP_WITHOUT_AUXILIARY_STATE, MiniDumpWithoutAuxiliaryState },
        { IDC_MINIDUMP_WITH_FULL_AUXILIARY_STATE, MiniDumpWithFullAuxiliaryState },
        { IDC_MINIDUMP_WITH_PRIVATE_WRITE_COPY_MEM, MiniDumpWithPrivateWriteCopyMemory },
        { IDC_MINIDUMP_IGNORE_INACCESSIBLE_MEM, MiniDumpIgnoreInaccessibleMemory },
        { IDC_MINIDUMP_WITH_TOKEN_INFO, MiniDumpWithTokenInformation },
        { IDC_MINIDUMP_WITH_MODULE_HEADERS, MiniDumpWithModuleHeaders },
        { IDC_MINIDUMP_FILTER_TRIAGE, MiniDumpFilterTriage },
        { IDC_MINIDUMP_WITH_AVXX_STATE_CONTEXT, MiniDumpWithAvxXStateContext },
        { IDC_MINIDUMP_WITH_IPT_TRACE, MiniDumpWithIptTrace },
        { IDC_MINIDUMP_SCAN_INACCESSIBLE_PARTIAL_PAGES, MiniDumpScanInaccessiblePartialPages },
        { IDC_MINIDUMP_FILTER_WRITE_COMBINE_MEM, MiniDumpFilterWriteCombinedMemory },
    };
    PPH_PROCESS_ITEM processItem;

    if (uMsg == WM_INITDIALOG)
    {
        processItem = (PPH_PROCESS_ITEM)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, processItem);
    }
    else
    {
        processItem = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhSetApplicationWindowIcon(hwndDlg);

            PhCenterWindow(hwndDlg, NULL);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_MINIDUMP_NORMAL), BST_CHECKED);

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
                    MINIDUMP_TYPE dumpType;

                    dumpType = 0;

                    for (ULONG i = 0; i < RTL_NUMBER_OF(options); i++)
                    {
                        if (Button_GetCheck(GetDlgItem(hwndDlg, options[i].Id)) == BST_CHECKED)
                            dumpType |= options[i].Type;
                    }

                    PhUiCreateDumpFileProcess(hwndDlg, processItem, dumpType);
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

VOID PhShowCreateDumpFileProcessDialog(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PROCDUMP),
        WindowHandle,
        PhpProcDumpDlgProc,
        ProcessItem
        );
}
