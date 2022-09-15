/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2019-2021
 *
 */

/*
 * There are two methods of hidden process detection implemented in this module.
 *
 * Brute Force. This attempts to open all possible PIDs within a certain range
 * in order to find processes which have been unlinked from the active process
 * list (EPROCESS.ActiveProcessLinks). This method is not effective when
 * either NtOpenProcess is hooked or PsLookupProcessByProcessId is hooked
 * (KSystemInformer cannot bypass this).
 *
 * CSR Handles. This enumerates handles in all running CSR processes, and works
 * even when a process has been unlinked from the active process list and
 * has been removed from the client ID table (PspCidTable). However, the method
 * does not detect native executables since CSR is not notified about them.
 * Some rootkits hook NtQuerySystemInformation in order to modify the returned
 * handle information; Process Hacker bypasses this by using KSystemInformer,
 * which calls ExEnumHandleTable directly. Note that both process and thread
 * handles are examined.
 */

#include <phapp.h>
#include <apiimport.h>
#include <hidnproc.h>

#include <evntcons.h>

#include <kphuser.h>

#include <mainwnd.h>
#include <procprv.h>
#include <settings.h>
#include <phsettings.h>

INT_PTR CALLBACK PhpHiddenProcessesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

COLORREF NTAPI PhpHiddenProcessesColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    );

BOOLEAN NTAPI PhpHiddenProcessesCallback(
    _In_ PPH_HIDDEN_PROCESS_ENTRY Process,
    _In_opt_ PVOID Context
    );

PPH_PROCESS_ITEM PhpCreateProcessItemForHiddenProcess(
    _In_ PPH_HIDDEN_PROCESS_ENTRY Entry
    );

static HWND PhHiddenProcessesWindowHandle = NULL;
static HWND PhHiddenProcessesListViewHandle = NULL;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static RECT MinimumSize;

static PH_HIDDEN_PROCESS_METHOD ProcessesMethod;
static PPH_LIST ProcessesList = NULL;
static ULONG NumberOfHiddenProcesses;
static ULONG NumberOfTerminatedProcesses;

VOID PhShowHiddenProcessesDialog(
    VOID
    )
{
    if (!PhHiddenProcessesWindowHandle)
    {
        PhHiddenProcessesWindowHandle = CreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_HIDDENPROCESSES),
            NULL,
            PhpHiddenProcessesDlgProc
            );
    }

    if (!IsWindowVisible(PhHiddenProcessesWindowHandle))
        ShowWindow(PhHiddenProcessesWindowHandle, SW_SHOW);
    else
        SetForegroundWindow(PhHiddenProcessesWindowHandle);
}

INT_PTR CALLBACK PhpHiddenProcessesDlgProc(
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
            HWND lvHandle;
            HWND methodHandle;

            PhSetApplicationWindowIcon(hwndDlg);

            PhHiddenProcessesListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_PROCESSES);
            methodHandle = GetDlgItem(hwndDlg, IDC_METHOD);

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_INTRO), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&WindowLayoutManager, lvHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_DESCRIPTION), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&WindowLayoutManager, methodHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_TERMINATE), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SAVE), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SCAN), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 320, L"Process");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 60, L"PID");

            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"HiddenProcessesListViewColumns", lvHandle);
            ExtendedListView_AddFallbackColumn(lvHandle, 0);
            ExtendedListView_AddFallbackColumn(lvHandle, 1);
            ExtendedListView_SetItemColorFunction(lvHandle, PhpHiddenProcessesColorFunction);

            ComboBox_AddString(methodHandle, L"Brute force");
            ComboBox_AddString(methodHandle, L"CSR handles");
            ComboBox_AddString(methodHandle, L"ETW handles");
            ComboBox_AddString(methodHandle, L"Process handles");
            ComboBox_AddString(methodHandle, L"Registry handles");
            ComboBox_AddString(methodHandle, L"Ntdll handles");
            PhSelectComboBoxString(methodHandle, L"Process handles", FALSE);

            MinimumSize.left = 0;
            MinimumSize.top = 0;
            MinimumSize.right = 330;
            MinimumSize.bottom = 140;
            MapDialogRect(hwndDlg, &MinimumSize);

            if (PhGetIntegerPairSetting(L"HiddenProcessesWindowPosition").X)
                PhLoadWindowPlacementFromSetting(L"HiddenProcessesWindowPosition", L"HiddenProcessesWindowSize", hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            EnableWindow(GetDlgItem(hwndDlg, IDC_TERMINATE), FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveWindowPlacementToSetting(L"HiddenProcessesWindowPosition", L"HiddenProcessesWindowSize", hwndDlg);
            PhSaveListViewColumnsToSetting(L"HiddenProcessesListViewColumns", PhHiddenProcessesListViewHandle);

            PhHiddenProcessesWindowHandle = NULL;
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    DestroyWindow(hwndDlg);
                }
                break;
            case IDC_SCAN:
                {
                    NTSTATUS status;
                    PPH_STRING method;

                    method = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_METHOD)));

                    if (ProcessesList)
                    {
                        ULONG i;

                        for (i = 0; i < ProcessesList->Count; i++)
                        {
                            PPH_HIDDEN_PROCESS_ENTRY entry = ProcessesList->Items[i];

                            if (entry->FileName)
                                PhDereferenceObject(entry->FileName);

                            PhFree(entry);
                        }

                        PhDereferenceObject(ProcessesList);
                    }

                    ProcessesList = PhCreateList(40);

                    if (PhEqualString2(method, L"Brute force", TRUE))
                        ProcessesMethod = BruteForceScanMethod;
                    else if (PhEqualString2(method, L"CSR handles", TRUE))
                        ProcessesMethod = CsrHandlesScanMethod;
                    else if (PhEqualString2(method, L"Process handles", TRUE))
                        ProcessesMethod = ProcessHandleScanMethod;
                    else if (PhEqualString2(method, L"Registry handles", TRUE))
                        ProcessesMethod = RegistryScanMethod;
                    else if (PhEqualString2(method, L"ETW handles", TRUE))
                        ProcessesMethod = EtwGuidScanMethod;
                    else if (PhEqualString2(method, L"Ntdll handles", TRUE))
                        ProcessesMethod = NtdllScanMethod;

                    NumberOfHiddenProcesses = 0;
                    NumberOfTerminatedProcesses = 0;

                    ExtendedListView_SetRedraw(PhHiddenProcessesListViewHandle, FALSE);
                    ListView_DeleteAllItems(PhHiddenProcessesListViewHandle);
                    status = PhEnumHiddenProcesses(
                        ProcessesMethod,
                        PhpHiddenProcessesCallback,
                        NULL
                        );
                    ExtendedListView_SortItems(PhHiddenProcessesListViewHandle);
                    ExtendedListView_SetRedraw(PhHiddenProcessesListViewHandle, TRUE);

                    if (NT_SUCCESS(status))
                    {
                        PhSetDialogItemText(hwndDlg, IDC_DESCRIPTION,
                            PhaFormatString(L"%u hidden process(es), %u terminated process(es).",
                            NumberOfHiddenProcesses, NumberOfTerminatedProcesses)->Buffer
                            );
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_DESCRIPTION), NULL, TRUE);
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to perform the scan", status, 0);
                    }
                }
                break;
            case IDC_TERMINATE:
                {
                    PPH_HIDDEN_PROCESS_ENTRY *entries;
                    ULONG numberOfEntries;
                    ULONG i;

                    PhGetSelectedListViewItemParams(PhHiddenProcessesListViewHandle, &entries, &numberOfEntries);

                    if (numberOfEntries != 0)
                    {
                        if (!PhGetIntegerSetting(L"EnableWarnings") ||
                            PhShowConfirmMessage(
                            hwndDlg,
                            L"terminate",
                            L"the selected process(es)",
                            L"Terminating a hidden process may cause the system to become unstable "
                            L"or crash.",
                            TRUE
                            ))
                        {
                            NTSTATUS status;
                            HANDLE processHandle;
                            BOOLEAN refresh;

                            refresh = FALSE;

                            for (i = 0; i < numberOfEntries; i++)
                            {
                                if (ProcessesMethod == BruteForceScanMethod)
                                {
                                    status = PhOpenProcess(
                                        &processHandle,
                                        PROCESS_TERMINATE,
                                        entries[i]->ProcessId
                                        );
                                }
                                else
                                {
                                    status = PhOpenProcessByCsrHandles(
                                        &processHandle,
                                        PROCESS_TERMINATE,
                                        entries[i]->ProcessId
                                        );
                                }

                                if (NT_SUCCESS(status))
                                {
                                    status = PhTerminateProcess(processHandle, STATUS_SUCCESS);
                                    NtClose(processHandle);

                                    if (NT_SUCCESS(status))
                                        refresh = TRUE;
                                }
                                else
                                {
                                    PhShowStatus(hwndDlg, L"Unable to terminate the process", status, 0);
                                }
                            }

                            if (refresh)
                            {
                                // Sleep for a bit before continuing. It seems to help avoid BSODs.

                                PhDelayExecution(250);

                                SendMessage(hwndDlg, WM_COMMAND, IDC_SCAN, 0);
                            }
                        }
                    }

                    PhFree(entries);
                }
                break;
            case IDC_SAVE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Text files (*.txt)", L"*.txt" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;

                    fileDialog = PhCreateSaveFileDialog();

                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
                    PhSetFileDialogFileName(fileDialog, L"Hidden Processes.txt");

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        NTSTATUS status;
                        PPH_STRING fileName;
                        PPH_FILE_STREAM fileStream;

                        fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));

                        if (NT_SUCCESS(status = PhCreateFileStream(
                            &fileStream,
                            fileName->Buffer,
                            FILE_GENERIC_WRITE,
                            FILE_SHARE_READ,
                            FILE_OVERWRITE_IF,
                            0
                            )))
                        {
                            PhWriteStringAsUtf8FileStream(fileStream, &PhUnicodeByteOrderMark);
                            PhWritePhTextHeader(fileStream);
                            PhWriteStringAsUtf8FileStream2(fileStream, L"Method: ");
                            PhWriteStringAsUtf8FileStream2(fileStream,
                                ProcessesMethod == BruteForceScanMethod ? L"Brute Force\r\n" : L"CSR Handles\r\n");
                            PhWriteStringFormatAsUtf8FileStream(
                                fileStream,
                                L"Hidden: %u\r\nTerminated: %u\r\n\r\n",
                                NumberOfHiddenProcesses,
                                NumberOfTerminatedProcesses
                                );

                            if (ProcessesList)
                            {
                                ULONG i;

                                for (i = 0; i < ProcessesList->Count; i++)
                                {
                                    PPH_HIDDEN_PROCESS_ENTRY entry = ProcessesList->Items[i];

                                    if (entry->Type == HiddenProcess)
                                        PhWriteStringAsUtf8FileStream2(fileStream, L"[HIDDEN] ");
                                    else if (entry->Type == TerminatedProcess)
                                        PhWriteStringAsUtf8FileStream2(fileStream, L"[Terminated] ");
                                    else if (entry->Type != NormalProcess)
                                        continue;

                                    PhWriteStringFormatAsUtf8FileStream(
                                        fileStream,
                                        L"%s (%u)\r\n",
                                        entry->FileName->Buffer,
                                        HandleToUlong(entry->ProcessId)
                                        );
                                }
                            }

                            PhDereferenceObject(fileStream);
                        }

                        if (!NT_SUCCESS(status))
                            PhShowStatus(hwndDlg, L"Unable to create the file", status, 0);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            PhHandleListViewNotifyBehaviors(lParam, PhHiddenProcessesListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);

            switch (header->code)
            {
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == PhHiddenProcessesListViewHandle)
                    {
                        EnableWindow(
                            GetDlgItem(hwndDlg, IDC_TERMINATE),
                            ListView_GetSelectedCount(PhHiddenProcessesListViewHandle) > 0
                            );
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == PhHiddenProcessesListViewHandle)
                    {
                        PPH_HIDDEN_PROCESS_ENTRY entry;

                        entry = PhGetSelectedListViewItemParam(PhHiddenProcessesListViewHandle);

                        if (entry)
                        {
                            PPH_PROCESS_ITEM processItem;

                            if (processItem = PhpCreateProcessItemForHiddenProcess(entry))
                            {
                                ProcessHacker_ShowProcessProperties(processItem);
                                PhDereferenceObject(processItem);
                            }
                            else
                            {
                                PhShowError(hwndDlg, L"%s", L"Unable to create a process structure for the selected process.");
                            }
                        }
                    }
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
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_DESCRIPTION))
            {
                if (NumberOfHiddenProcesses != 0)
                {
                    SetTextColor((HDC)wParam, RGB(0xff, 0x00, 0x00));
                }

                SetBkColor((HDC)wParam, GetSysColor(COLOR_3DFACE));

                return (INT_PTR)GetSysColorBrush(COLOR_3DFACE);
            }
        }
        break;
    }

    REFLECT_MESSAGE_DLG(hwndDlg, PhHiddenProcessesListViewHandle, uMsg, wParam, lParam);

    return FALSE;
}

static COLORREF NTAPI PhpHiddenProcessesColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    )
{
    PPH_HIDDEN_PROCESS_ENTRY entry = Param;

    switch (entry->Type)
    {
    case UnknownProcess:
    case HiddenProcess:
        return RGB(0xff, 0x00, 0x00);
    case TerminatedProcess:
        return RGB(0x77, 0x77, 0x77);
    }

    return GetSysColor(COLOR_WINDOW);
}

static BOOLEAN NTAPI PhpHiddenProcessesCallback(
    _In_ PPH_HIDDEN_PROCESS_ENTRY Process,
    _In_opt_ PVOID Context
    )
{
    PPH_HIDDEN_PROCESS_ENTRY entry;
    INT lvItemIndex;
    WCHAR pidString[PH_INT32_STR_LEN_1];

    entry = PhAllocateCopy(Process, sizeof(PH_HIDDEN_PROCESS_ENTRY));

    if (entry->FileName)
        PhReferenceObject(entry->FileName);

    PhAddItemList(ProcessesList, entry);

    lvItemIndex = PhAddListViewItem(PhHiddenProcessesListViewHandle, MAXINT,
        PhGetStringOrDefault(entry->FileName, L"(unknown)"), entry);
    PhPrintUInt32(pidString, HandleToUlong(entry->ProcessId));
    PhSetListViewSubItem(PhHiddenProcessesListViewHandle, lvItemIndex, 1, pidString);

    if (entry->Type == HiddenProcess)
        NumberOfHiddenProcesses++;
    else if (entry->Type == TerminatedProcess)
        NumberOfTerminatedProcesses++;

    return TRUE;
}

static PPH_PROCESS_ITEM PhpCreateProcessItemForHiddenProcess(
    _In_ PPH_HIDDEN_PROCESS_ENTRY Entry
    )
{
    NTSTATUS status;
    PPH_PROCESS_ITEM processItem;
    PPH_PROCESS_ITEM idleProcessItem;
    HANDLE processHandle;
    PROCESS_BASIC_INFORMATION basicInfo;
    KERNEL_USER_TIMES times;
    UCHAR priorityClass;
    PROCESS_HANDLE_INFORMATION handleInfo;
    HANDLE processHandle2;
    LONG dpiValue;

    if (Entry->Type == NormalProcess)
    {
        processItem = PhReferenceProcessItem(Entry->ProcessId);

        if (processItem)
            return processItem;
    }

    processItem = PhCreateProcessItem(Entry->ProcessId);

    // Mark the process as terminated if necessary.
    if (Entry->Type == TerminatedProcess)
        processItem->State |= PH_PROCESS_ITEM_REMOVED;

    // We need a process record. Just use the record of System Idle Process.
    if (idleProcessItem = PhReferenceProcessItem(SYSTEM_IDLE_PROCESS_ID))
    {
        processItem->Record = idleProcessItem->Record;
        PhReferenceProcessRecord(processItem->Record);
    }
    else
    {
        PhDereferenceObject(processItem);
        return NULL;
    }

    // Set up the file name and process name.

    PhSwapReference(&processItem->FileName, Entry->FileName);

    if (processItem->FileName)
    {
        processItem->ProcessName = PhGetBaseName(processItem->FileName);
    }
    else
    {
        processItem->ProcessName = PhCreateString(L"Unknown");
    }

    if (ProcessesMethod == BruteForceScanMethod)
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            Entry->ProcessId
            );
    }
    else
    {
        status = PhOpenProcessByCsrHandles(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            Entry->ProcessId
            );
    }

    if (NT_SUCCESS(status))
    {
        // Basic information and not-so-dynamic information

        processItem->QueryHandle = processHandle;

        if (NT_SUCCESS(PhGetProcessBasicInformation(processHandle, &basicInfo)))
        {
            processItem->ParentProcessId = basicInfo.InheritedFromUniqueProcessId;
            processItem->BasePriority = basicInfo.BasePriority;
        }

        PhGetProcessSessionId(processHandle, &processItem->SessionId);

        //PhPrintUInt32(processItem->ParentProcessIdString, HandleToUlong(processItem->ParentProcessId));
        //PhPrintUInt32(processItem->SessionIdString, processItem->SessionId);

        if (NT_SUCCESS(PhGetProcessTimes(processHandle, &times)))
        {
            processItem->CreateTime = times.CreateTime;
            processItem->KernelTime = times.KernelTime;
            processItem->UserTime = times.UserTime;
        }

        // TODO: Token information?

        if (NT_SUCCESS(PhGetProcessPriority(processHandle, &priorityClass)))
        {
            processItem->PriorityClass = priorityClass;
        }

        if (NT_SUCCESS(PhGetProcessHandleCount(processHandle, &handleInfo)))
        {
            processItem->NumberOfHandles = handleInfo.HandleCount;
        }
    }

    // Stage 1
    // Some copy and paste magic here...

    if (processItem->FileName)
    {
        dpiValue = PhGetWindowDpi(PhGetMainWindowHandle());

        // Small icon, large icon.
        if (processItem->IconEntry = PhImageListExtractIcon(processItem->FileName, TRUE, dpiValue))
        {
            processItem->SmallIconIndex = processItem->IconEntry->SmallIconIndex;
            processItem->LargeIconIndex = processItem->IconEntry->LargeIconIndex;
        }

        // Version info.
        PhInitializeImageVersionInfoEx(&processItem->VersionInfo, &processItem->FileName->sr, PhEnableVersionShortText);
    }

    // Command line

    status = PhOpenProcess(
        &processHandle2,
        PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
        Entry->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        PPH_STRING commandLine;
        ULONG i;

        if (NT_SUCCESS(status = PhGetProcessCommandLine(processHandle2, &commandLine)))
        {
            // Some command lines (e.g. from taskeng.exe) have nulls in them.
            // Since Windows can't display them, we'll replace them with
            // spaces.
            for (i = 0; i < (ULONG)commandLine->Length / sizeof(WCHAR); i++)
            {
                if (commandLine->Buffer[i] == UNICODE_NULL)
                    commandLine->Buffer[i] = ' ';
            }
        }

        if (NT_SUCCESS(status))
        {
            processItem->CommandLine = commandLine;
        }

        NtClose(processHandle2);
    }

    // TODO: Other stage 1 tasks.

    PhSetEvent(&processItem->Stage1Event);

    return processItem;
}

NTSTATUS PhpEnumHiddenProcessesBruteForce(
    _In_ PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    PPH_LIST pids;
    ULONG pid;
    BOOLEAN stop = FALSE;

    if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
        return status;

    pids = PhCreateList(40);

    process = PH_FIRST_PROCESS(processes);

    do
    {
        PhAddItemList(pids, process->UniqueProcessId);
    } while (process = PH_NEXT_PROCESS(process));

    PhFree(processes);

    for (pid = 8; pid <= 65536; pid += 4)
    {
        NTSTATUS status2;
        HANDLE processHandle;
        PH_HIDDEN_PROCESS_ENTRY entry;
        KERNEL_USER_TIMES times;
        PPH_STRING fileName;

        status2 = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            UlongToHandle(pid)
            );

        if (NT_SUCCESS(status2))
        {
            entry.ProcessId = UlongToHandle(pid);

            if (NT_SUCCESS(status2 = PhGetProcessTimes(
                processHandle,
                &times
                )) &&
                NT_SUCCESS(status2 = PhGetProcessImageFileName(
                processHandle,
                &fileName
                )))
            {
                entry.FileName = PhGetFileName(fileName);
                PhDereferenceObject(fileName);

                if (times.ExitTime.QuadPart != 0)
                    entry.Type = TerminatedProcess;
                else if (PhFindItemList(pids, UlongToHandle(pid)) != -1)
                    entry.Type = NormalProcess;
                else
                    entry.Type = HiddenProcess;

                if (!Callback(&entry, Context))
                    stop = TRUE;

                PhDereferenceObject(entry.FileName);
            }

            NtClose(processHandle);
        }

        // Use an alternative method if we don't have sufficient access.
        if (status2 == STATUS_ACCESS_DENIED)
        {
            if (NT_SUCCESS(status2 = PhGetProcessImageFileNameByProcessId(UlongToHandle(pid), &fileName)))
            {
                entry.ProcessId = UlongToHandle(pid);
                entry.FileName = PhGetFileName(fileName);
                PhDereferenceObject(fileName);

                if (PhFindItemList(pids, UlongToHandle(pid)) != -1)
                    entry.Type = NormalProcess;
                else
                    entry.Type = HiddenProcess;

                if (!Callback(&entry, Context))
                    stop = TRUE;

                PhDereferenceObject(entry.FileName);
            }
        }

        if (status2 == STATUS_INVALID_CID || status2 == STATUS_INVALID_PARAMETER)
            status2 = STATUS_SUCCESS;

        if (!NT_SUCCESS(status2))
        {
            entry.ProcessId = UlongToHandle(pid);
            entry.FileName = NULL;
            entry.Type = UnknownProcess;

            if (!Callback(&entry, Context))
                stop = TRUE;
        }

        if (stop)
            break;
    }

    PhDereferenceObject(pids);

    return status;
}

typedef struct _CSR_HANDLES_CONTEXT
{
    PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback;
    PVOID Context;
    PPH_LIST Pids;
} CSR_HANDLES_CONTEXT, *PCSR_HANDLES_CONTEXT;

static BOOLEAN NTAPI PhpCsrProcessHandlesCallback(
    _In_ PPH_CSR_HANDLE_INFO Handle,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    BOOLEAN cont = TRUE;
    PCSR_HANDLES_CONTEXT context = Context;
    HANDLE processHandle;
    KERNEL_USER_TIMES times;
    PPH_STRING fileName;
    PH_HIDDEN_PROCESS_ENTRY entry;

    entry.ProcessId = Handle->ProcessId;

    if (NT_SUCCESS(status = PhOpenProcessByCsrHandle(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION,
        Handle
        )))
    {
        if (NT_SUCCESS(status = PhGetProcessTimes(
            processHandle,
            &times
            )) &&
            NT_SUCCESS(status = PhGetProcessImageFileName(
            processHandle,
            &fileName
            )))
        {
            entry.FileName = PhGetFileName(fileName);
            PhDereferenceObject(fileName);

            if (times.ExitTime.QuadPart != 0)
                entry.Type = TerminatedProcess;
            else if (context && PhFindItemList(context->Pids, Handle->ProcessId) != -1)
                entry.Type = NormalProcess;
            else
                entry.Type = HiddenProcess;

            if (context && !context->Callback(&entry, context->Context))
                cont = FALSE;

            PhDereferenceObject(entry.FileName);
        }

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        entry.FileName = NULL;
        entry.Type = UnknownProcess;

        if (context && !context->Callback(&entry, context->Context))
            cont = FALSE;
    }

    return cont;
}

NTSTATUS PhpEnumHiddenProcessesCsrHandles(
    _In_ PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    PPH_LIST pids;
    CSR_HANDLES_CONTEXT context;

    if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
        return status;

    pids = PhCreateList(40);

    process = PH_FIRST_PROCESS(processes);

    do
    {
        PhAddItemList(pids, process->UniqueProcessId);
    } while (process = PH_NEXT_PROCESS(process));

    PhFree(processes);

    context.Callback = Callback;
    context.Context = Context;
    context.Pids = pids;

    status = PhEnumCsrProcessHandles(PhpCsrProcessHandlesCallback, &context);

    PhDereferenceObject(pids);

    return status;
}

NTSTATUS PhpEnumHiddenProcessHandles(
    _In_ PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
#if (PHNT_VERSION >= PHNT_WIN7)
    NTSTATUS status;
    HANDLE processHandle;

    if (!NT_SUCCESS(status = NtGetNextProcess(
        NULL,
        PROCESS_QUERY_LIMITED_INFORMATION,
        0,
        0,
        &processHandle
        )))
        return status;

    while (TRUE)
    {
        PVOID processes;
        HANDLE enumProcessHandle;
        PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(processHandle, &basicInfo)))
        {
            if (NT_SUCCESS(PhEnumProcesses(&processes)))
            {
                if (!PhFindProcessInformation(processes, basicInfo.BasicInfo.UniqueProcessId))
                {
                    PH_HIDDEN_PROCESS_ENTRY entry;
                    PPH_STRING fileName;

                    entry.ProcessId = basicInfo.BasicInfo.UniqueProcessId;

                    if (NT_SUCCESS(PhGetProcessImageFileName(processHandle, &fileName)))
                    {
                        entry.FileName = PhGetFileName(fileName);
                        PhDereferenceObject(fileName);
                        entry.Type = HiddenProcess;

                        if (basicInfo.IsProcessDeleting)
                            entry.Type = TerminatedProcess;

                        if (!Callback(&entry, Context))
                            break;

                        PhDereferenceObject(entry.FileName);
                    }
                    else
                    {
                        entry.FileName = NULL;
                        entry.Type = UnknownProcess;

                        if (!Callback(&entry, Context))
                            break;
                    }
                }

                PhFree(processes);
            }
        }

        if (NT_SUCCESS(status = NtGetNextProcess(
            processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            0,
            0,
            &enumProcessHandle
            )))
        {
            NtClose(processHandle);
            processHandle = enumProcessHandle;
        }
        else
        {
            NtClose(processHandle);
            break;
        }
    }

    if (status == STATUS_NO_MORE_ENTRIES)
        status = STATUS_SUCCESS; // HACK

    return status;
#else
    return STATUS_UNSUCCESSFUL;
#endif
}

NTSTATUS PhpEnumHiddenSubKeyHandles(
    _In_ PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    ULONG bufferSize;
    PKEY_OPEN_SUBKEYS_INFORMATION buffer;
    UNICODE_STRING subkeyName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!NtQueryOpenSubKeysEx_Import())
        return STATUS_UNSUCCESSFUL;

    bufferSize = 0x200;
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        RtlInitUnicodeString(&subkeyName, L"\\REGISTRY");
        InitializeObjectAttributes(
            &objectAttributes,
            &subkeyName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = NtQueryOpenSubKeysEx_Import()(
            &objectAttributes,
            bufferSize,
            buffer,
            &bufferSize
            );

        if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            bufferSize *= 2;
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        for (ULONG i = 0; i < buffer->Count; i++)
        {
            KEY_PID_ARRAY entry = buffer->KeyArray[i];
            HANDLE processHandle;

            if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, entry.ProcessId)))
            {
                PVOID processes;

                if (NT_SUCCESS(PhEnumProcesses(&processes)))
                {
                    if (!PhFindProcessInformation(processes, entry.ProcessId))
                    {
                        PH_HIDDEN_PROCESS_ENTRY process;
                        PPH_STRING fileName;

                        process.ProcessId = entry.ProcessId;

                        if (NT_SUCCESS(PhGetProcessImageFileName(processHandle, &fileName)))
                        {
                            PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

                            process.FileName = PhGetFileName(fileName);
                            PhDereferenceObject(fileName);
                            process.Type = HiddenProcess;

                            if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(processHandle, &basicInfo)))
                            {
                                if (basicInfo.IsProcessDeleting)
                                    process.Type = TerminatedProcess;
                            }

                            if (!Callback(&process, Context))
                                break;

                            PhDereferenceObject(process.FileName);
                        }
                        else
                        {
                            process.FileName = NULL;
                            process.Type = UnknownProcess;

                            if (!Callback(&process, Context))
                                break;
                        }
                    }

                    PhFree(processes);
                }

                NtClose(processHandle);
            }
            else
            {
                PH_HIDDEN_PROCESS_ENTRY process;
                PPH_STRING fileName;

                process.ProcessId = entry.ProcessId;

                if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(process.ProcessId, &fileName)))
                {
                    process.FileName = PhGetFileName(fileName);
                    PhDereferenceObject(fileName);
                    process.Type = HiddenProcess;

                    if (!Callback(&process, Context))
                        break;

                    PhDereferenceObject(process.FileName);
                }
                else
                {
                    process.FileName = NULL;
                    process.Type = UnknownProcess;

                    if (!Callback(&process, Context))
                        break;
                }
            }
        }
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

#define PH_FIRST_ETW_GUID(TraceGuid) \
    (((PTRACE_GUID_INFO)(TraceGuid))->InstanceCount ? \
    ((PTRACE_PROVIDER_INSTANCE_INFO)PTR_ADD_OFFSET(TraceGuid, \
    sizeof(TRACE_GUID_INFO))) : NULL)
#define PH_NEXT_ETW_GUID(TraceGuid) \
    (((PTRACE_PROVIDER_INSTANCE_INFO)(TraceGuid))->NextOffset ? \
    (PTRACE_PROVIDER_INSTANCE_INFO)PTR_ADD_OFFSET((TraceGuid), \
    ((PTRACE_PROVIDER_INSTANCE_INFO)(TraceGuid))->NextOffset) : NULL)

NTSTATUS PhpEnumEtwGuidHandles(
    _In_ PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PGUID traceGuidList = NULL;
    ULONG traceGuidListLength = 0;

    status = PhTraceControl(
        TraceControlEnumTraceGuidList,
        NULL,
        0,
        &traceGuidList,
        &traceGuidListLength
        );

    if (NT_SUCCESS(status))
    {
        for (ULONG i = 0; i < traceGuidListLength / sizeof(GUID); i++)
        {
            GUID providerGuid = traceGuidList[i];
            PTRACE_GUID_INFO traceGuidInfo;

            status = PhTraceControl(
                TraceControlGetTraceGuidInfo,
                &providerGuid,
                sizeof(GUID),
                &traceGuidInfo,
                NULL
                );

            if (NT_SUCCESS(status))
            {
                PTRACE_PROVIDER_INSTANCE_INFO instance;
                HANDLE processHandle;
                PVOID processes;

                for (instance = PH_FIRST_ETW_GUID(traceGuidInfo);
                    instance;
                    instance = PH_NEXT_ETW_GUID(instance))
                {
                    if (instance->Pid == 0)
                        continue;

                    if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, UlongToHandle(instance->Pid))))
                    {
                        if (NT_SUCCESS(PhEnumProcesses(&processes)))
                        {
                            if (!PhFindProcessInformation(processes, UlongToHandle(instance->Pid)))
                            {
                                PH_HIDDEN_PROCESS_ENTRY process;
                                PPH_STRING fileName;

                                process.ProcessId = UlongToHandle(instance->Pid);

                                if (NT_SUCCESS(PhGetProcessImageFileName(processHandle, &fileName)))
                                {
                                    PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

                                    process.FileName = PhGetFileName(fileName);
                                    PhDereferenceObject(fileName);
                                    process.Type = HiddenProcess;

                                    if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(processHandle, &basicInfo)))
                                    {
                                        if (basicInfo.IsProcessDeleting)
                                            process.Type = TerminatedProcess;
                                    }

                                    if (!Callback(&process, Context))
                                        break;

                                    PhDereferenceObject(process.FileName);
                                }
                                else
                                {
                                    process.FileName = NULL;
                                    process.Type = UnknownProcess;

                                    if (!Callback(&process, Context))
                                        break;
                                }
                            }

                            PhFree(processes);
                        }

                        NtClose(processHandle);
                    }
                    else
                    {
                        PH_HIDDEN_PROCESS_ENTRY process;
                        PPH_STRING fileName;

                        process.ProcessId = UlongToHandle(instance->Pid);

                        if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(process.ProcessId, &fileName)))
                        {
                            process.FileName = PhGetFileName(fileName);
                            PhDereferenceObject(fileName);
                            process.Type = HiddenProcess;

                            if (!Callback(&process, Context))
                                break;

                            PhDereferenceObject(process.FileName);
                        }
                        else
                        {
                            process.FileName = NULL;
                            process.Type = UnknownProcess;

                            if (!Callback(&process, Context))
                                break;
                        }
                    }
                }

                PhFree(traceGuidInfo);
            }
        }

        PhFree(traceGuidList);
    }

    return status;
}


NTSTATUS PhpEnumNtdllHandles(
    _In_ PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    static PH_STRINGREF ntdllPath = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntdll.dll");
    static PH_STRINGREF ntfsPath = PH_STRINGREF_INIT(L"\\ntfs");
    NTSTATUS status;
    HANDLE fileHandle;

    status = PhCreateFile(
        &fileHandle,
        &ntfsPath,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
    {
        status = PhCreateFile(
            &fileHandle,
            &ntdllPath,
            FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );
    }

    if (NT_SUCCESS(status))
    {
        PFILE_PROCESS_IDS_USING_FILE_INFORMATION processIds;

        status = PhGetProcessIdsUsingFile(
            fileHandle,
            &processIds
            );

        if (NT_SUCCESS(status))
        {
            for (ULONG i = 0; i < processIds->NumberOfProcessIdsInList; i++)
            {
                HANDLE processId;
                HANDLE processHandle;

                processId = (HANDLE)processIds->ProcessIdList[i];

                if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, processId)))
                {
                    PVOID processes;

                    if (NT_SUCCESS(PhEnumProcesses(&processes)))
                    {
                        if (!PhFindProcessInformation(processes, processId))
                        {
                            PH_HIDDEN_PROCESS_ENTRY process;
                            PPH_STRING fileName;

                            process.ProcessId = processId;

                            if (NT_SUCCESS(PhGetProcessImageFileName(processHandle, &fileName)))
                            {
                                PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

                                process.FileName = PhGetFileName(fileName);
                                PhDereferenceObject(fileName);
                                process.Type = HiddenProcess;

                                if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(processHandle, &basicInfo)))
                                {
                                    if (basicInfo.IsProcessDeleting)
                                        process.Type = TerminatedProcess;
                                }

                                if (!Callback(&process, Context))
                                    break;

                                PhDereferenceObject(process.FileName);
                            }
                            else
                            {
                                process.FileName = NULL;
                                process.Type = UnknownProcess;

                                if (!Callback(&process, Context))
                                    break;
                            }
                        }

                        PhFree(processes);
                    }

                    NtClose(processHandle);
                }
                else
                {
                    PH_HIDDEN_PROCESS_ENTRY process;
                    PPH_STRING fileName;

                    process.ProcessId = processId;

                    if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(process.ProcessId, &fileName)))
                    {
                        process.FileName = PhGetFileName(fileName);
                        PhDereferenceObject(fileName);
                        process.Type = HiddenProcess;

                        if (!Callback(&process, Context))
                            break;

                        PhDereferenceObject(process.FileName);
                    }
                    else
                    {
                        process.FileName = NULL;
                        process.Type = UnknownProcess;

                        if (!Callback(&process, Context))
                            break;
                    }
                }
            }

            PhFree(processIds);
        }

        NtClose(fileHandle);
    }

    return status;
}

NTSTATUS PhEnumHiddenProcesses(
    _In_ PH_HIDDEN_PROCESS_METHOD Method,
    _In_ PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    switch (Method)
    {
    case BruteForceScanMethod:
        return PhpEnumHiddenProcessesBruteForce(Callback, Context);
    case CsrHandlesScanMethod:
        return PhpEnumHiddenProcessesCsrHandles(Callback, Context);
    case ProcessHandleScanMethod:
        return PhpEnumHiddenProcessHandles(Callback, Context);
    case RegistryScanMethod:
        return PhpEnumHiddenSubKeyHandles(Callback, Context);
    case EtwGuidScanMethod:
        return PhpEnumEtwGuidHandles(Callback, Context);
    case NtdllScanMethod:
        return PhpEnumNtdllHandles(Callback, Context);
    }

    return STATUS_FAIL_CHECK;
}

NTSTATUS PhpOpenCsrProcesses(
    _Out_ PHANDLE *ProcessHandles,
    _Out_ PULONG NumberOfProcessHandles
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
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_DUP_HANDLE,
            process->UniqueProcessId
            )))
        {
            if (NT_SUCCESS(PhGetProcessKnownType(
                processHandle,
                &knownProcessType
                )) &&
                (knownProcessType & KnownProcessTypeMask) == WindowsSubsystemProcessType)
            {
                PhAddItemList(processHandleList, processHandle);
            }
            else
            {
                NtClose(processHandle);
            }
        }
    } while (process = PH_NEXT_PROCESS(process));

    PhFree(processes);


    if (processHandleList->Count)
    {
        *ProcessHandles = PhAllocateCopy(processHandleList->Items, processHandleList->Count * sizeof(HANDLE));
        *NumberOfProcessHandles = processHandleList->Count;

        PhDereferenceObject(processHandleList);

        return status;
    }

    PhDereferenceObject(processHandleList);

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS PhpGetCsrHandleProcessId(
    _Inout_ PPH_CSR_HANDLE_INFO Handle
    )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION processBasicInfo;
    THREAD_BASIC_INFORMATION threadBasicInfo;

    Handle->IsThreadHandle = FALSE;
    Handle->ProcessId = NULL;

    // Assume the handle is a process handle, and get the
    // process ID.

    status = KphQueryInformationObject(
        Handle->CsrProcessHandle,
        Handle->Handle,
        KphObjectProcessBasicInformation,
        &processBasicInfo,
        sizeof(PROCESS_BASIC_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        Handle->ProcessId = processBasicInfo.UniqueProcessId;
    }
    else
    {
        // We failed to get the process ID. Assume the handle
        // is a thread handle, and get the process ID.

        status = KphQueryInformationObject(
            Handle->CsrProcessHandle,
            Handle->Handle,
            KphObjectThreadBasicInformation,
            &threadBasicInfo,
            sizeof(THREAD_BASIC_INFORMATION),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            Handle->ProcessId = threadBasicInfo.ClientId.UniqueProcess;
            Handle->IsThreadHandle = TRUE;
        }
    }

    return status;
}

NTSTATUS PhEnumCsrProcessHandles(
    _In_ PPH_ENUM_CSR_PROCESS_HANDLES_CALLBACK Callback,
    _In_opt_ PVOID Context
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
        PKPH_PROCESS_HANDLE_INFORMATION handles;
        ULONG j;

        if (stop)
            break;

        if (NT_SUCCESS(KphEnumerateProcessHandles2(csrProcessHandles[i], &handles)))
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
                if (PhFindItemList(pids, handle.ProcessId) != -1)
                    continue;

                PhAddItemList(pids, handle.ProcessId);

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
        NtClose(csrProcessHandles[i]);

    PhFree(csrProcessHandles);

    return status;
}

NTSTATUS PhOpenProcessByCsrHandle(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_CSR_HANDLE_INFO Handle
    )
{
    NTSTATUS status;

    if (!Handle->IsThreadHandle)
    {
        status = NtDuplicateObject(
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

        if (!NT_SUCCESS(status = NtDuplicateObject(
            Handle->CsrProcessHandle,
            Handle->Handle,
            NtCurrentProcess(),
            &threadHandle,
            THREAD_QUERY_LIMITED_INFORMATION,
            0,
            0
            )))
            return status;

        status = KphOpenThreadProcess(
            threadHandle,
            DesiredAccess,
            ProcessHandle
            );
        NtClose(threadHandle);
    }

    return status;
}

typedef struct _OPEN_PROCESS_BY_CSR_CONTEXT
{
    NTSTATUS Status;
    PHANDLE ProcessHandle;
    ACCESS_MASK DesiredAccess;
    HANDLE ProcessId;
} OPEN_PROCESS_BY_CSR_CONTEXT, *POPEN_PROCESS_BY_CSR_CONTEXT;

static BOOLEAN NTAPI PhpOpenProcessByCsrHandlesCallback(
    _In_ PPH_CSR_HANDLE_INFO Handle,
    _In_opt_ PVOID Context
    )
{
    POPEN_PROCESS_BY_CSR_CONTEXT context = Context;

    if (context && Handle->ProcessId == context->ProcessId)
    {
        context->Status = PhOpenProcessByCsrHandle(
            context->ProcessHandle,
            context->DesiredAccess,
            Handle
            );

        return FALSE;
    }

    return TRUE;
}

NTSTATUS PhOpenProcessByCsrHandles(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    OPEN_PROCESS_BY_CSR_CONTEXT context;

    context.Status = STATUS_INVALID_CID;
    context.ProcessHandle = ProcessHandle;
    context.DesiredAccess = DesiredAccess;
    context.ProcessId = ProcessId;

    if (!NT_SUCCESS(status = PhEnumCsrProcessHandles(
        PhpOpenProcessByCsrHandlesCallback,
        &context
        )))
        return status;

    return context.Status;
}
